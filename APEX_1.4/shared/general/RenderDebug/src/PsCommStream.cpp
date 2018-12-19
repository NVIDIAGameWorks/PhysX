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


#include "PsCommStream.h"
#include "PsUserAllocated.h"
#include "RenderDebug.h"
#include "PsString.h"
#include "PxIntrinsics.h"
#include "PsIntrinsics.h"
#include <stdio.h>

#define MAX_STREAM_RECORD (1024*1024*2047)  // no more than one gig
#define STREAM_VERSION_NUMBER 100
static char magic[4] = { 'N', 'V', 'C', 'S' };

class CommStreamImpl : public CommStream, public physx::shdfnd::UserAllocated
{
public:
	CommStreamImpl(const char *streamFile,bool recordFile,CommLayer *c)
	{
		mCommLayer = c;
		mIsRecord = recordFile;
		mFirstFrame = true;
		mIsValid = true;
		mMaxPacketLength = (1024*1024)*16;
		mPacketLength = 0;
		mPacketLoc = 0;
		mPacketData = static_cast<uint8_t *>(PX_ALLOC(mMaxPacketLength,"PacketData"));
		mPacketBigEndian = false;
		physx::shdfnd::strlcpy(mFilename,256,streamFile);
		if ( mIsRecord )
		{
			mFph = NULL;
		}
		else
		{
			mFph = fopen(streamFile,"rb");
			mIsValid = false;
			if ( mFph )
			{
				char id[4] = { 0, 0, 0, 0};
				uint32_t v1=0;
				uint32_t v2=0;
				uint32_t v3=0;
				size_t res = fread(id,sizeof(id),1,mFph);
				res = fread(&v1,sizeof(v1),1,mFph);
				res = fread(&v2,sizeof(v2),1,mFph);
				res = fread(&v3,sizeof(v3),1,mFph);
				PX_UNUSED(res);
				if ( id[0] == magic[0] &&
					 id[1] == magic[1] &&
					 id[2] == magic[2] &&
					 id[3] == magic[3] &&
					 v1 == STREAM_VERSION_NUMBER &&
					 v2 == RENDER_DEBUG_VERSION &&
					 v3 == RENDER_DEBUG_COMM_VERSION )
				{
					mIsValid = true;
					readNextPacket();
				}
			}
		}
	}

	virtual ~CommStreamImpl(void)
	{
		close();
	}

	void close(void)
	{
		PX_FREE(mPacketData);
		mPacketData = NULL;
		if ( mFph )
		{
			fclose(mFph);
			mFph = NULL;
		}
		mPacketLength = 0;
		mPacketLoc = 0;
		mIsValid = false;
	}

	FILE *getFile(void)
	{
		if ( mFirstFrame )
		{
			mFph = fopen(mFilename,"wb");
			if ( mFph )
			{
				uint32_t version = STREAM_VERSION_NUMBER;
				fwrite(magic,sizeof(magic),1,mFph);
				fwrite(&version,sizeof(version),1,mFph);
				version = RENDER_DEBUG_VERSION;
				fwrite(&version,sizeof(version),1,mFph);
				version = RENDER_DEBUG_COMM_VERSION;
				fwrite(&version,sizeof(version),1,mFph);
				fflush(mFph);
			}
			mFirstFrame = false;
		}
		return mFph;
	}

	virtual void release(void)
	{
		delete this;
	}

	virtual CommLayer *getCommLayer(void) 
	{
		return mCommLayer;
	}

	bool isValid(void) const
	{
		return mIsValid;
	}

	virtual bool isServer(void) const // return true if we are in server mode.
	{
		return mCommLayer->isServer();
	}

	virtual bool hasClient(void) const 	// return true if a client connection is currently established
	{
		bool ret = false;
		if ( mIsRecord )
		{
			ret = mCommLayer->hasClient();
		}
		else
		{
			ret = mIsValid;
		}
		return ret;
	}

	virtual bool isConnected(void) const // return true if we are still connected to the server.  The server is always in a 'connected' state.
	{
		return mCommLayer->isConnected();
	}

	virtual int32_t getPendingReadSize(void) const // returns the number of bytes of data which is pending to be read.
	{
		int32_t ret = 0;
		if ( mIsRecord )
		{
			ret = mCommLayer->getPendingReadSize();
		}
		else
		{
			ret = int32_t(mPacketLength-mPacketLoc);
		}
		return ret;
	}

	virtual int32_t getPendingSendSize(void) const  // return the number of bytes of data pending to be sent.  This can be used for flow control
	{
		return mCommLayer->getPendingSendSize();
	}

	virtual bool sendMessage(const void *msg,uint32_t len) 
	{
		bool ret = true;
		if ( mIsRecord )
		{
			ret = mCommLayer->sendMessage(msg,len);
		}
		return ret;
	}

	virtual uint32_t peekMessage(bool &isBigEndianPacket) // return the length of the next pending message
	{
		uint32_t ret = 0;
		if ( mIsRecord )
		{
			ret = mCommLayer->peekMessage(isBigEndianPacket);
		}
		else
		{
			ret = mPacketLength - mPacketLoc;
			isBigEndianPacket = mPacketBigEndian;
		}
		return ret;
	}

	virtual uint32_t getMessage(void *msg,uint32_t maxLength,bool &isBigEndianPacket) // receives a pending message
	{
		uint32_t ret = 0;
		if ( mIsRecord )
		{
			ret = mCommLayer->getMessage(msg,maxLength,isBigEndianPacket);
			if ( ret && mIsRecord )
			{
				getFile();
				if ( mFph )
				{
					uint32_t be = isBigEndianPacket ? 1U : 0U;
					fwrite(&ret,sizeof(ret),1,mFph);
					fwrite(&be,sizeof(be),1,mFph);
					fwrite(msg,ret,1,mFph);
					fflush(mFph);
					size_t len = size_t(ftell(mFph));
					if ( len > MAX_STREAM_RECORD )
					{
						fclose(mFph);
						mFph = NULL;
					}
				}
			}
		}
		else if ( mIsValid )
		{
			ret = mPacketLength - mPacketLoc;
			if ( ret <= maxLength )
			{
				physx::intrinsics::memCopy(msg,&mPacketData[mPacketLoc],ret);
				readNextPacket();
			}
			else
			{
				ret = maxLength;
				physx::intrinsics::memCopy(msg,&mPacketData[mPacketLoc],ret);
				mPacketLoc+=ret;
			}
		}
		return ret;
	}

	void readNextPacket(void)
	{
		if ( mFph && !mIsRecord )
		{
			mPacketLoc = 0;
			uint32_t be;
			size_t r1 = fread(&mPacketLength,sizeof(mPacketLength),1,mFph);
			size_t r2 = fread(&be,sizeof(be),1,mFph);
			if ( r1 == 1 && r2 == 1 )
			{
				if ( mPacketLength > mMaxPacketLength )
				{
					mMaxPacketLength = mPacketLength + (1024*1024*10);
					PX_FREE(mPacketData);
					mPacketData = static_cast<uint8_t *>(PX_ALLOC(mMaxPacketLength,"PacketData"));
				}
				if ( mPacketData )
				{
					size_t r3 = fread(mPacketData,mPacketLength,1,mFph);
					if ( r3 == 1 )
					{
						mPacketBigEndian = be ? true : false;
					}
					else
					{
						close();
					}
				}
				else
				{
					close();
				}
			}
			else
			{
				close();
			}
		}
	}

	uint32_t	mMaxPacketLength;
	uint32_t	mPacketLoc;
	uint32_t	mPacketLength;
	uint8_t		*mPacketData;
	bool		mPacketBigEndian;

	char		mFilename[256];
	FILE		*mFph;
	bool		mFirstFrame;
	bool		mIsValid;
	bool		mIsRecord;
	CommLayer	*mCommLayer;
};

CommStream *createCommStream(const char *streamFile,bool recordFile,CommLayer *cl)
{
	CommStreamImpl *c = PX_NEW(CommStreamImpl)(streamFile,recordFile,cl);
	if ( !c->isValid() )
	{
		c->release();
		c = NULL;
	}
	return static_cast< CommStream *>(c);
}
