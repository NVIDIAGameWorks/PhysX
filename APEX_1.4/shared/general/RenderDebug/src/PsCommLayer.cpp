//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.


#include "PsCommLayer.h"

#include "PsArray.h"
#include "PsUserAllocated.h"
#include "PsSocket.h"
#include "PsThread.h"
#include "PsAtomic.h"
#include "PsMutex.h"
#include "PxIntrinsics.h"

namespace COMM_LAYER
{

// a simple hashing function
static uint32_t simpleHash(const void *data,uint32_t dlen) 
{
	uint32_t h = 5381;
	const uint8_t *string = static_cast<const uint8_t *>(data);
	for(const uint8_t *ptr = string; dlen; ptr++)
	{
		h = ((h<<5)+h)^(uint32_t(*ptr));
		dlen--;
	}
	return h;
}


static PX_INLINE void swap4Bytes(void* _data) 
{
	char *data = static_cast<char *>(_data);
	char one_byte;
	one_byte = data[0]; data[0] = data[3]; data[3] = one_byte;
	one_byte = data[1]; data[1] = data[2]; data[2] = one_byte;
}

static PX_INLINE bool isBigEndian()
{
	int32_t i = 1;
	return *(reinterpret_cast<char*>(&i))==0;
}

// This is a little helper class that contains the packets waiting to be sent.
class CommPacket
{
public:
	CommPacket(void)
	{
		if ( isBigEndian() )
		{
			mHeader[0] = 'B';
			mBigEndianPacket = true;
		}
		else
		{
			mHeader[0] = 'C';
			mBigEndianPacket = false;
		}
		mHeader[1] = 'P';
		mHeader[2] = 'A';
		mHeader[3] = 'C';
		mHeader[4] = 'K';
		mHeader[5] = 'E';
		mHeader[6] = 'T';
		mHeader[7] =  0;

		mLength = 0;
		mHash = 0;
		mData = NULL;
	}

	void computeHash(void)
	{
		mHash = simpleHash(mHeader,sizeof(mHeader)+sizeof(mLength)); // compute the hash of the header + the hash of the length
	}

	bool isHeaderPacket(void) 
	{
		bool ret = false;

		if ( mHeader[0] == 'C' &&
			 mHeader[1] == 'P' &&
			 mHeader[2] == 'A' &&
			 mHeader[3] == 'C' &&
			 mHeader[4] == 'K' &&
			 mHeader[5] == 'E' &&
			 mHeader[6] == 'T' &&
			 mHeader[7] == 0 )
		{
			uint32_t hash = simpleHash(mHeader,sizeof(mHeader)+sizeof(mLength)); // compute the hash of the header + the hash of the length
			uint32_t bhash = mHash;
			if ( isBigEndian() )
			{
				swap4Bytes(&bhash);
			}
			if ( hash == bhash )
			{
				mBigEndianPacket = false;
				ret = true;
			}
		}
		else if ( mHeader[0] == 'B' &&
				mHeader[1] == 'P' &&
				mHeader[2] == 'A' &&
				mHeader[3] == 'C' &&
				mHeader[4] == 'K' &&
				mHeader[5] == 'E' &&
				mHeader[6] == 'T' &&
				mHeader[7] == 0 )
		{
			uint32_t hash = simpleHash(mHeader,sizeof(mHeader)+sizeof(mLength)); // compute the hash of the header + the hash of the length
			uint32_t bhash = mHash;
			if ( !isBigEndian() )
			{
				swap4Bytes(&bhash);
			}
			if ( hash == bhash )
			{
				mBigEndianPacket = true;
				ret = true;
			}
		}

		return ret;
	}

	uint32_t getSize(void) const
	{
		return sizeof(mHeader)+sizeof(mLength)+sizeof(mHash);
	}

	uint32_t getLength(void) const
	{
		uint32_t l = mLength;
		if ( isBigEndian() )
		{
			swap4Bytes(&l);
		}
		return l;
	}

	void setLength(uint32_t l)
	{
		mLength = l;
		if ( isBigEndian() )
		{
			swap4Bytes(&mLength);
		}
	}

	bool isBigEndianPacket(void) const
	{
		return mBigEndianPacket;
	}

private:
	uint8_t		mHeader[8];
	uint32_t	mLength;
	uint32_t	mHash;
public:
	uint8_t		*mData;
	bool		mBigEndianPacket;
};

typedef physx::shdfnd::Array< CommPacket > CommPacketArray;

class CommLayerImp : public CommLayer, public physx::shdfnd::Thread
{
	PX_NOCOPY(CommLayerImp)
public:
	CommLayerImp(const char *ipaddress,uint16_t portNumber,bool isServer) :
		mReadPackets(PX_DEBUG_EXP("PsCommLayer::CommLayerImpl::mReadPackets"))
	{
		mListenPending = false;
		mSendData = NULL;
		mSendDataLength = 0;
		mSendLoc = 0;
		mTotalPendingSize = 0;	// number of bytes of data pending to be sent
		mTotalPendingCount = 0; // number of packets of data pending to be sent; count not size
		mReadSize = 0;
		mReadPacketHeaderLoc = 0;
		mReadPacketLoc = 0;
		mIsServer = isServer;
		mPortNumber = portNumber;
		mSocket = PX_NEW(physx::shdfnd::Socket)(false);
		mClientConnected = false;
		if ( isServer )
		{
			mConnected = mSocket->listen(portNumber);
			mListenPending = true;
		}
		else
		{
			mConnected = mSocket->connect(ipaddress,portNumber);
		}
		if ( mConnected )
		{
			mSocket->setBlocking(false);
			physx::shdfnd::Thread::start(0x4000);
		}
	}

	virtual ~CommLayerImp(void)
	{
		physx::shdfnd::Thread::signalQuit();
		physx::shdfnd::Thread::waitForQuit();
		if ( mSocket )
		{
			mSocket->disconnect();
			delete mSocket;
		}
		releaseAllReadData();
		releaseAllSendData();
	}

	void releaseAllReadData(void)
	{
		PX_FREE(mReadPacketHeader.mData);
		mReadPacketHeader.mData = NULL;
		for (uint32_t i=0; i<mReadPackets.size(); i++)
		{
			PX_FREE(mReadPackets[i].mData);
		}
		mReadPackets.clear();
		mReadSize = 0;
	}

	void releaseAllSendData(void)
	{
		// Free up memory allocated for pending sends not yet sent...
		for (uint32_t i=0; i<mPendingSends.size(); i++)
		{
			CommPacket &sp = mPendingSends[i];
			PX_FREE(sp.mData);
		}
		mPendingSends.clear();
		PX_FREE(mSendData);
		mSendData = NULL;
		mTotalPendingCount = 0;
		mTotalPendingSize = 0;
	}

	virtual bool sendMessage(const void *data,uint32_t len) 
	{
		bool ret = false;
		CommPacket sp;
		sp.setLength(len);
		sp.mData = static_cast<uint8_t *>(PX_ALLOC(len+sp.getSize(),"CommPacket"));
		if ( sp.mData )
		{
			physx::intrinsics::memCopy(sp.mData+sp.getSize(),data,len);
			mSendMutex.lock();
			mPendingSends.pushBack(sp);
			physx::shdfnd::atomicAdd(&mTotalPendingSize,int32_t(len));
			physx::shdfnd::atomicAdd(&mTotalPendingCount,1);
			mSendMutex.unlock();
			ret = true;
		}
		return ret;
	}

	virtual uint32_t peekMessage(bool &isBigEndianPacket) // return the length of the next pending message
	{
		uint32_t ret = 0;

		int32_t readSize = physx::shdfnd::atomicAdd(&mReadSize,0);
		if ( readSize != 0 )
		{
			mReadPacketMutex.lock();
			if ( !mReadPackets.empty() )
			{
				ret = mReadPackets[0].getLength();
				isBigEndianPacket = mReadPackets[0].isBigEndianPacket();
			}
			mReadPacketMutex.unlock();
		}

		return ret;
	}

	virtual uint32_t getMessage(void *msg,uint32_t maxLength,bool &isBigEndianPacket) // receives a pending message
	{
		uint32_t ret = 0;

		int32_t readSize = physx::shdfnd::atomicAdd(&mReadSize,0);	// And data in the read queue?
		if ( readSize != 0 ) // Yes..
		{
			mReadPacketMutex.lock();	// Lock the read packets mutex
			CommPacket cp = mReadPackets[0];	// get the packet
			if ( cp.getLength() <= maxLength )	// make sure it is shorter than the read buffer provided
			{
				physx::shdfnd::atomicAdd(&mReadSize,-int32_t(cp.getLength()));	// subtract the read size semaphore to indicate the amount of data we snarfed from the read buffer.
				mReadPackets.remove(0);	// Remove the packet from the queue
				mReadPacketMutex.unlock(); // Once we have pulled this off of the read queue it is save to release the mutex and add more.
				physx::intrinsics::memCopy(msg,cp.mData,cp.getLength()); // Copy the packet to the callers buffer
				ret = cp.getLength();	// set the return length
				isBigEndianPacket = cp.isBigEndianPacket();
				PX_FREE(cp.mData);	// Free the read-data now that it has been copied into the users buffer.
			}
			else
			{
				mReadPacketMutex.unlock();	// Unlock the mutex
			}
		}
		return ret;
	}

	virtual void release(void)
	{
		delete this;
	}

	bool isConnected(void) const
	{
		return mConnected;
	}

	virtual bool notifySocketConnection(physx::shdfnd::Socket *newConnection) // notify the caller that a new connection has been established.  
	{
		PX_UNUSED(newConnection);
		return true;
	}

	bool processSocketWrite(void)
	{
		while ( mSendData ) // if we are sending data..
		{
			uint32_t sendCount = mSendDataLength-mSendLoc;
			if ( sendCount > 32768 )
			{
				sendCount = 32768;
			}
			const uint8_t *data = &mSendData[mSendLoc];
			uint32_t sendAmount = mSocket->write(data,sendCount);
			if ( sendAmount )
			{
				mSendLoc+=sendAmount;
				if ( mSendLoc == mSendDataLength )
				{
					PX_FREE(mSendData);
					mSendData = NULL;
					mSendDataLength = 0;
					mSendLoc = 0;
				}
			}
			else
			{
				break;
			}
		}
		return mSendData ? true : false;
	}

	void processPendingSends(void)
	{
		int32_t pendingCount = physx::shdfnd::atomicAdd(&mTotalPendingCount,0);
		while ( !processSocketWrite() && pendingCount != 0  )
		{
			mSendMutex.lock();	// Lock the send mutex before we check if there is data to send.
			CommPacket sp = mPendingSends[0];	// Get the first packet to be sent.
			physx::shdfnd::atomicAdd(&mTotalPendingSize,-int32_t(sp.getLength()));
			physx::shdfnd::atomicAdd(&mTotalPendingCount,-1);
			mPendingSends.remove(0);	// Remove this packet from the pending send list.
			mSendMutex.unlock();		// Unlock the mutex because at this point it is safe to post new send packets.
			sp.computeHash();
			physx::intrinsics::memCopy(sp.mData,&sp,sp.getSize());
			mSendData = sp.mData;
			mSendDataLength = sp.getLength()+sp.getSize();
			mSendLoc = 0;
			pendingCount = physx::shdfnd::atomicAdd(&mTotalPendingCount,0);
		}
	}

	void resetServerSocket(void)
	{
		mSocket->disconnect(); // close the previous connection..
		delete mSocket;
		releaseAllReadData();
		releaseAllSendData();
		mSocket = PX_NEW(physx::shdfnd::Socket)(false);
		mSocket->setBlocking(false);
		mConnected = mSocket->listen(mPortNumber);
		if ( !mConnected )
		{
			physx::shdfnd::Thread::signalQuit();
		}
		else
		{
			mListenPending = true;
		}
	}

	//The thread execute function
	virtual void execute()
	{
		setName("CommLayer");	// Set the name of the read

		if ( mIsServer )
		{
			while( !quitIsSignalled()  )
			{
				if ( mSocket->isConnected()  )
				{
					mClientConnected = true;
					processPendingSends();
					bool haveData = processRead();
					while ( haveData )
					{
						haveData = processRead();
					}
					if ( !mSocket->isConnected() ) // if we lost the client connection, then we need to fire up a new listen.
					{
						resetServerSocket();
					}
				}
				else
				{
					mClientConnected = false;
					if ( !mListenPending )
					{
						resetServerSocket();
					}
					bool connected = mSocket->accept(false);
					if ( connected )
					{
						mSocket->setBlocking(false);
						mListenPending = false;
					}
				}
				physx::shdfnd::Thread::sleep(0);
			}
		}
		else
		{
			while( !quitIsSignalled() && mSocket->isConnected() )
			{
				processPendingSends();
				bool haveData = processRead();
				while ( haveData )
				{
					haveData = processRead();
				}
				physx::shdfnd::Thread::sleep(0);
			}
		}
		mClientConnected = false;
		mConnected = false;
		quit();
	}

	bool processRead(void)
	{
		bool ret = false;

		if ( mReadPacketHeader.mData ) // if we are currently in the process of reading in a packet...
		{
			uint32_t remaining = mReadPacketHeader.getLength() - mReadPacketLoc;
			uint8_t *readData = &mReadPacketHeader.mData[mReadPacketLoc];
			uint32_t readCount = mSocket->read(readData,remaining);
			if ( readCount )
			{
				ret = true;
				mReadPacketLoc+=readCount;
				if ( mReadPacketLoc == mReadPacketHeader.getLength() )
				{
					// The packet has been read!!
					mReadPacketMutex.lock();
					mReadPackets.pushBack(mReadPacketHeader);
					physx::shdfnd::atomicAdd(&mReadSize,int32_t(mReadPacketHeader.getLength()));
					mReadPacketMutex.unlock();
					mReadPacketHeader.mData = NULL;
				}
			}
		}
		else
		{
			uint32_t remaining = mReadPacketHeader.getSize() - mReadPacketHeaderLoc; // the amount of the header we have still yet to read...
			uint8_t *header = reinterpret_cast<uint8_t *>(&mReadPacketHeader);
			uint8_t *readData = &header[mReadPacketHeaderLoc];
			uint32_t readCount = mSocket->read(readData,remaining);
			if ( readCount )
			{
				ret = true;
				mReadPacketHeaderLoc+=readCount;
				if ( mReadPacketHeaderLoc == mReadPacketHeader.getSize() )
				{
					if ( mReadPacketHeader.isHeaderPacket() ) // if it is a valid header packet..
					{
						PX_FREE(mReadPacketHeader.mData);
						mReadPacketHeader.mData = static_cast<uint8_t *>(PX_ALLOC(mReadPacketHeader.getLength(),"ReadPacket"));
						mReadPacketLoc = 0;
						mReadPacketHeaderLoc = 0;
					}
					else
					{
						// We could be connecting somehow 'mid-stream' so we need to scan forward a byte at a time until we get a valid packet header.
						// We do this by copying all of the old packet header + 1 byte, and set the next read location to read just one more byte at the end.
						// This will cause the stream to 'resync' back to the head of the next packet.
						CommPacket cp = mReadPacketHeader;
						const uint8_t *source = reinterpret_cast<const uint8_t *>(&cp);
						uint8_t *dest = reinterpret_cast<uint8_t *>(&mReadPacketHeader);
						physx::intrinsics::memCopy(dest,source+1,cp.getSize()-1);
						mReadPacketHeaderLoc = cp.getSize()-1;
					}
					
				}
			}
		}
		return ret;
	}

	virtual int32_t getPendingReadSize(void) const  // returns the number of bytes of data which is pending to be read.
	{
		return physx::shdfnd::atomicAdd(&mReadSize,0);
	}

	virtual bool isServer(void) const // return true if we are in server mode.
	{
		return mIsServer;
	}

	virtual bool hasClient(void) const	// return true if a client connection is currently established
	{
		return mClientConnected;
	}

	int32_t getPendingSendSize(void) const
	{
		return physx::shdfnd::atomicAdd(&mTotalPendingSize,0);
	}

	physx::shdfnd::Socket	*mSocket;
	uint16_t				mPortNumber;
	bool					mIsServer;
	bool					mConnected;
	mutable int32_t			mReadSize;
	uint32_t				mReadPacketHeaderLoc;	// The current read location from the input stream for the current packet
	uint32_t				mReadPacketLoc;
	CommPacket				mReadPacketHeader;		// The current packet we are reading header
	CommPacketArray			mReadPackets;
	mutable int32_t			mTotalPendingSize;
	mutable int32_t			mTotalPendingCount;
	CommPacketArray			mPendingSends;
	bool					mClientConnected;
	physx::shdfnd::Mutex	mReadPacketMutex;	// Mutex to have thread safety on reads
	physx::shdfnd::Mutex	mSendMutex;			// Mutex to have thread safety on sends
	uint8_t					*mSendData;			// data being sent over the socket connection
	uint32_t				mSendDataLength;	// The length of the data to send over the socket connection
	uint32_t				mSendLoc;			// The current location of the data which has been sent so far.
	bool					mListenPending;
	uint32_t				mReadTimeOut;
};

} // end of namespace

using namespace COMM_LAYER;


CommLayer *createCommLayer(const char *ipaddress,uint16_t portNumber,bool isServer)
{
	CommLayerImp *c = PX_NEW(CommLayerImp)(ipaddress,portNumber,isServer);
	if ( !c->isConnected() )
	{
		c->release();
		c = NULL;
	}
	return static_cast< CommLayer *>(c);
}
