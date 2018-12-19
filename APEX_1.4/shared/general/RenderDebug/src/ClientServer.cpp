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


#include "ClientServer.h"
#include "GetArgs.h"
#include "PsCommLayer.h"
#include "PsCommStream.h"
#include "PsArray.h"
#include "PsUserAllocated.h"
#include "PxIntrinsics.h"
#include "PsMutex.h"
#include "PsThread.h"
#include "PsString.h"
#include "PsTime.h"
#include <stdio.h>

#define MAX_SERVER_COMMAND_STRING 16384

enum PacketType
{
	PT_PRIMITIVES	= 100,
	PT_COMMAND,
	PT_ACK,
	PT_COMM_VERSION,
	PT_FINALIZE_FRAME,
	PT_APPLICATION_NAME,
	PT_REQUEST_REMOTE_RESOURCE,
	PT_REMOTE_RESOURCE,
	PT_INPUT_EVENT
};

struct NetworkCommmand
{
	NetworkCommmand(void)
	{
		mPacketType = PT_COMMAND;
		mCommandLength = 0;
		mCommand = NULL;
	}
	uint32_t	mPacketType;
	uint32_t	mCommandLength;
	char		*mCommand;
};

struct NetworkFrame
{
	NetworkFrame(void)
	{
		mPacketType = PT_PRIMITIVES;
	}

	uint32_t getPrefixSize(void) const
	{
		return sizeof(NetworkFrame)-(sizeof(void *)*2);
	}

	uint32_t	mPacketType;
	uint32_t	mFrameCount;
	uint32_t	mPrimType;
	uint32_t	mPrimCount;
	uint32_t	mDataLength;
	void		*mData;
	void		*mBaseData;
};

#define MAX_RESOURCE_STRING 256

class RequestRemoteResource
{
public:
	uint32_t	mCommand;
	char		mNameSpace[MAX_RESOURCE_STRING];
	char		mResourceName[MAX_RESOURCE_STRING];
};

class RemoteResourcePacket
{
public:
	uint32_t	mCommand;
	uint32_t	mLength;
	uint32_t	mBigEndian;
	char		mNameSpace[MAX_RESOURCE_STRING];
	char		mResourceName[MAX_RESOURCE_STRING];
};

class RemoteResource : public RemoteResourcePacket
{
public:
	RemoteResource(void)
	{
		mData = NULL;
	}
	void		*mData;
};

typedef physx::shdfnd::Array< NetworkFrame > NetworkFrameArray;
typedef physx::shdfnd::Array< NetworkCommmand > NetworkCommandArray;
typedef physx::shdfnd::Array< RemoteResource > RemoteResourceArray;
typedef physx::shdfnd::Array< RENDER_DEBUG::InputEvent > InputEventArray;


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


struct ServerCommandString
{
	ServerCommandString(void)
	{
		mCommandString[0] = 0;
	}
	char mCommandString[MAX_SERVER_COMMAND_STRING];
};

class ClientServerImpl : public ClientServer, public physx::shdfnd::UserAllocated
{
	PX_NOCOPY(ClientServerImpl)
public:
	ClientServerImpl(void)
	{

	}

	ClientServerImpl(RENDER_DEBUG::RenderDebug::Desc &desc) :
		mIncomingNetworkFrames(PX_DEBUG_EXP("ClientServer::mIncomingNetworkFrames")),
			mNetworkFrames(PX_DEBUG_EXP("ClientServer::mNetworkFrames")),
			mNetworkCommands(PX_DEBUG_EXP("ClientServer::mNetworkCommands")),
			mRemoteResources(PX_DEBUG_EXP("ClientServer::mRemoteResource"))
	{
		mSkipFrame = false;
		physx::shdfnd::strlcpy(mApplicationName,512,desc.applicationName);
		mStreamName[0] = 0;
		mDataSent = false;
		mServerStallCallback = desc.serverStallCallback;
		mRenderDebugResource = desc.renderDebugResource;
		mBadVersion = true; // the version number is presumed bad until we know otherwise
		mMaxServerWait = desc.maxServerWait;
		mReceivedAck = true;
		mCurrentNetworkFrame = 0;
		mLastProcessNetworkFrame = 0;
		mCommStream = NULL;
		mCommLayer = NULL;
		mCurrentCommLayer = NULL;
		mLastFrame = 0;
		mHaveApplicationName = false;
		mMeshId = 100000; // base mesh id
		mUsedMeshId = false;
		mIsServer = desc.runMode == RENDER_DEBUG::RenderDebug::RM_SERVER;
		mStreamCount = 1;
		mWasConnected = false;
		mCommLayer = createCommLayer(desc.hostName,desc.portNumber,mIsServer);
		setStreamFilename(desc.streamFileName);
		if ( mCommLayer == NULL )
		{
			desc.errorCode = "Failed to create connection";
		}
		else if ( !mIsServer )
		{
			sendVersionPacket();
		}

	}

	virtual ~ClientServerImpl(void)
	{
		if ( mCommLayer )
		{
			mCommLayer->release();
		}
		if ( mCommStream )
		{
			mCommStream->release();
		}
		releaseNetworkFrames();
		releaseIncomingNetworkFrames();
		releaseRemoteResources();
		releaseNetworkCommands();
	}

	void sendVersionPacket(void)
	{
		if ( !mCurrentCommLayer ) return;
		uint32_t packet[2];
		packet[0] = PT_COMM_VERSION;
		packet[1] = RENDER_DEBUG_COMM_VERSION;
		mCurrentCommLayer->sendMessage(packet,sizeof(packet));
	}

	void sendApplicationNamePacket(void)
	{
		if ( !mCurrentCommLayer ) return;
		uint32_t packet[(512/4)+1];
		packet[0] = PT_APPLICATION_NAME;
		physx::intrinsics::memCopy(&packet[1],mApplicationName,512);
		mCurrentCommLayer->sendMessage(packet,sizeof(packet));
	}

	virtual void release(void)
	{
		delete this;
	}

	virtual bool isServer(void) const
	{
		return mIsServer;
	}

	virtual void recordPrimitives(uint32_t frameCount,
								  uint32_t primType,
								  uint32_t primCount,
								  uint32_t dataLength,
								  const void *data)
	{
		mLastFrame = frameCount;

		if ( mCurrentCommLayer && !mIsServer )
		{
			if ( mCurrentCommLayer->isConnected() ) // if we still have a connection..
			{
				// If we haven't caught up in
				uint32_t sendSize = sizeof(uint32_t)*5+dataLength;
				void *sendData = PX_ALLOC(sendSize,"sendData");
				uint32_t *sendDataHeader = static_cast<uint32_t *>(sendData);
				sendDataHeader[0] = PT_PRIMITIVES;
				sendDataHeader[1] = frameCount;
				sendDataHeader[2] = primType;
				sendDataHeader[3] = primCount;
				sendDataHeader[4] = dataLength;
				sendDataHeader+=5;
				physx::intrinsics::memCopy(sendDataHeader,data,dataLength);
				mCurrentCommLayer->sendMessage(sendData,sendSize);
				PX_FREE(sendData);
				mDataSent = true;
			}
		}

	}

	void processNetworkData(void)
	{
		bool isBigEndianPacket;
		uint32_t newDataSize = mCurrentCommLayer->peekMessage(isBigEndianPacket);
		bool endOfFrame = false;
		while ( newDataSize && !endOfFrame )
		{
			void *readData = PX_ALLOC(newDataSize,"readData");
			bool freeReadData = true;
			uint32_t rlen = mCurrentCommLayer->getMessage(readData,newDataSize,isBigEndianPacket);

			if ( rlen == newDataSize )
			{
				bool needEndianSwap = false;
				uint32_t type = *static_cast<uint32_t *>(readData);
				if ( isBigEndianPacket != isBigEndian() )
				{
					needEndianSwap = true;
				}
				if ( needEndianSwap )
				{
					swap4Bytes(&type);
				}
				switch ( type )
				{
					case PT_APPLICATION_NAME:
						{
							uint32_t *packet = static_cast<uint32_t *>(readData);
							physx::intrinsics::memCopy(mApplicationName,packet+1,512);
							mHaveApplicationName = true;
						}
						break;
					case PT_COMM_VERSION:
						{
							if ( mIsServer ) // if we are the server, tell the client our version number
							{
								sendVersionPacket(); // inform the client that we are of the same version
							}
							else
							{
								sendApplicationNamePacket(); // if we are the client, tell the server the name of our application
							}
							uint32_t *packet = static_cast<uint32_t *>(readData);
							if ( needEndianSwap )
							{
								swap4Bytes(&packet[1]);
							}
							if ( packet[1] == RENDER_DEBUG_COMM_VERSION ) // if it is the same version as us, no problems!
							{
								mBadVersion = false;
							}
							else
							{
								mBadVersion = true;
							}
						}
						break;
					case PT_PRIMITIVES:
						if ( !mBadVersion ) // if we have agreed upon each other's version number then we can process the data stream, otherwise..not so much
						{
							// as expected..
							NetworkFrame nframe;
							uint32_t *header = static_cast<uint32_t *>(readData);

							nframe.mFrameCount = header[1];
							nframe.mPrimType = header[2];
							nframe.mPrimCount = header[3];
							nframe.mDataLength = header[4];
							nframe.mData = &header[5];

							if ( needEndianSwap )
							{
								swap4Bytes(&nframe.mFrameCount);
								swap4Bytes(&nframe.mPrimType);
								swap4Bytes(&nframe.mPrimCount);
								swap4Bytes(&nframe.mDataLength);
							}

							nframe.mBaseData = readData;
							freeReadData = false;

							if ( nframe.mFrameCount != mCurrentNetworkFrame )
							{
								for (uint32_t j=0; j<mIncomingNetworkFrames.size(); j++)
								{
									NetworkFrame &nf = mIncomingNetworkFrames[j];
									uint32_t pcount = nf.mPrimCount;
									const uint8_t *scan = static_cast<const uint8_t *>(nf.mData);
									for (uint32_t i=0; i<pcount; i++)
									{
										RENDER_DEBUG::DebugPrimitive *prim = const_cast<RENDER_DEBUG::DebugPrimitive *>(reinterpret_cast<const RENDER_DEBUG::DebugPrimitive *>(scan));
										if ( needEndianSwap )
										{
											endianSwap(prim);
										}

										switch ( uint32_t(prim->mCommand) )
										{
											case RENDER_DEBUG::DebugCommand::DEBUG_CREATE_TRIANGLE_MESH:
												{
													RENDER_DEBUG::DebugCreateTriangleMesh *d = static_cast< RENDER_DEBUG::DebugCreateTriangleMesh *>(prim);
													uint32_t *hackMeshId = reinterpret_cast<uint32_t *>(d);
													hackMeshId[2]+=mMeshId;
													mUsedMeshId = true;
												}
												break;
											case RENDER_DEBUG::DebugCommand::DEBUG_REFRESH_TRIANGLE_MESH_VERTICES:
												{
													RENDER_DEBUG::DebugRefreshTriangleMeshVertices *d = static_cast< RENDER_DEBUG::DebugRefreshTriangleMeshVertices *>(prim);
													uint32_t *hackMeshId = reinterpret_cast<uint32_t *>(d);
													hackMeshId[2]+=mMeshId;
													mUsedMeshId = true;
												}
												break;
											case RENDER_DEBUG::DebugCommand::DEBUG_RENDER_TRIANGLE_MESH_INSTANCES:
												{
													RENDER_DEBUG::DebugRenderTriangleMeshInstances *d = static_cast<RENDER_DEBUG::DebugRenderTriangleMeshInstances *>(prim);
													uint32_t *hackMeshId = reinterpret_cast<uint32_t *>(d);
													hackMeshId[2]+=mMeshId;
													mUsedMeshId = true;
												}
												break;
											case RENDER_DEBUG::DebugCommand::DEBUG_RENDER_POINTS:
												{
													RENDER_DEBUG::DebugRenderPoints *d = static_cast<RENDER_DEBUG::DebugRenderPoints *>(prim);
													uint32_t *hackMeshId = reinterpret_cast<uint32_t *>(d);
													hackMeshId[2]+=mMeshId;
													mUsedMeshId = true;
												}
												break;
											case RENDER_DEBUG::DebugCommand::DEBUG_CONVEX_HULL:
												printf("Debug me");
												break;
											case RENDER_DEBUG::DebugCommand::DEBUG_RELEASE_TRIANGLE_MESH:
												{
													RENDER_DEBUG::DebugReleaseTriangleMesh *d = static_cast<RENDER_DEBUG::DebugReleaseTriangleMesh *>(prim);
													d->mMeshId+=mMeshId;
													mUsedMeshId = true;
												}
												break;
											case RENDER_DEBUG::DebugCommand::DEBUG_SKIP_FRAME:
												mSkipFrame = true;
												break;
										}
										uint32_t plen = RENDER_DEBUG::DebugCommand::getPrimtiveSize(*prim);
										scan+=plen;
									}
								}
								endOfFrame = true;
								if ( mSkipFrame )
								{
									mSkipFrame = false;
									releaseIncomingNetworkFrames();
								}
								else
								{
									releaseNetworkFrames(); // release previous network frames
									mNetworkFrames = mIncomingNetworkFrames;
									mIncomingNetworkFrames.clear();
								}
								mCurrentNetworkFrame = nframe.mFrameCount;
								mIncomingNetworkFrames.pushBack(nframe);

								if ( mIsServer )
								{
									// if we are the server, we should send an ACK to the client so that they know that we processed their frame of data.
									uint32_t ack = PT_ACK;
									mCurrentCommLayer->sendMessage(&ack,sizeof(ack));
								}
							}
							else
							{
								mIncomingNetworkFrames.pushBack(nframe);
							}
						}
						break;
					case PT_COMMAND:
						if ( !mBadVersion )
						{
							NetworkCommmand c;
							uint32_t *header = static_cast<uint32_t *>(readData);
							c.mCommandLength = header[1];
							if ( needEndianSwap )
							{
								swap4Bytes(&c.mCommandLength);
							}
							c.mCommand = static_cast<char*>(PX_ALLOC(c.mCommandLength+1,"NetworkCommand"));
							physx::intrinsics::memCopy(c.mCommand,&header[2],c.mCommandLength+1);
							mNetworkCommandMutex.lock();
							mNetworkCommands.pushBack(c);
							mNetworkCommandMutex.unlock();
						}
						break;
					case PT_REMOTE_RESOURCE:
						if ( !mBadVersion )
						{
							RemoteResourcePacket *rr = static_cast<RemoteResourcePacket *>(readData);
							if ( needEndianSwap )
							{
								swap4Bytes(&rr->mLength);
								swap4Bytes(&rr->mBigEndian);
							}
							uint32_t bigEndianPacket = isBigEndianPacket ? uint32_t(1) : uint32_t(0);
							PX_UNUSED(bigEndianPacket);
							PX_ASSERT( bigEndianPacket == rr->mBigEndian );
							RemoteResource sr;
							RemoteResourcePacket *prr = static_cast< RemoteResource *>(&sr);
							*prr = *rr;
							sr.mData = PX_ALLOC(sr.mLength,"RemoteResource");
							rr++;
							physx::intrinsics::memCopy(sr.mData,rr,sr.mLength);
							mRemoteResources.pushBack(sr);
						}
						break;
					case PT_INPUT_EVENT:
						{
							RENDER_DEBUG::InputEvent e = *static_cast<const RENDER_DEBUG::InputEvent *>(readData);
							if ( needEndianSwap )
							{
								swap4Bytes(&e.mId);
								swap4Bytes(&e.mCommunicationsFrame);
								swap4Bytes(&e.mRenderFrame);
								swap4Bytes(&e.mEventType);
								swap4Bytes(&e.mSensitivity);
								swap4Bytes(&e.mDigitalValue);
								swap4Bytes(&e.mAnalogValue);
								swap4Bytes(&e.mMouseX);
								swap4Bytes(&e.mMouseY);
								swap4Bytes(&e.mMouseDX);
								swap4Bytes(&e.mMouseDY);
								swap4Bytes(&e.mWindowSizeX);
								swap4Bytes(&e.mWindowSizeY);
								swap4Bytes(&e.mEyePosition[0]);
								swap4Bytes(&e.mEyePosition[1]);
								swap4Bytes(&e.mEyePosition[2]);
								swap4Bytes(&e.mEyeDirection[0]);
								swap4Bytes(&e.mEyeDirection[1]);
								swap4Bytes(&e.mEyeDirection[2]);
							}
							mInputEvents.pushBack(e);
						}
						break;
					case PT_REQUEST_REMOTE_RESOURCE:
						if ( !mBadVersion )
						{
							const RequestRemoteResource *rr = static_cast<const RequestRemoteResource *>(readData);
							if ( mRenderDebugResource )
							{
								uint32_t len;
								const void *data = mRenderDebugResource->requestResource(rr->mNameSpace,rr->mResourceName,len);
								if ( data )
								{
									sendRemoteResource(rr->mNameSpace,rr->mResourceName,data,len);
									mRenderDebugResource->releaseResource(data,len,rr->mNameSpace,rr->mResourceName);
								}
							}
						}
						break;
					case PT_FINALIZE_FRAME:
						{
							uint32_t *header = static_cast<uint32_t *>(readData);
							if ( needEndianSwap )
							{
								swap4Bytes(&header[1]);
							}
							mCurrentNetworkFrame = header[1];
							if ( mIsServer )
							{
								// if we are the server, we should send an ACK to the client so that they know that we processed their frame of data.
								uint32_t ack = PT_ACK;
								mCurrentCommLayer->sendMessage(&ack,sizeof(ack));
							}
						}
						break;
					case PT_ACK:
						mReceivedAck = true;
						break;
				}
			}
			if ( freeReadData )
			{
				PX_FREE(readData);
			}
			newDataSize = mCurrentCommLayer->peekMessage(isBigEndianPacket);
		}

	}

	void processFrame(RENDER_DEBUG::ProcessRenderDebug *processRenderDebug,RENDER_DEBUG::RenderDebugInterface *iface)
	{
		if ( !mCurrentCommLayer ) return;

		if ( mIsServer )
		{
			if ( mCommStream && !mCommStream->isValid() )
			{
				mCommStream->release();
				mCommStream = NULL;
				mCurrentCommLayer = mCommLayer;
			}
			if ( mCurrentCommLayer->hasClient() )
			{
				mWasConnected = true;
			}
			else if ( mWasConnected )
			{
				if ( mCommStream )
				{
					internalSetStreamFilename(mStreamName);
				}
				mWasConnected = false;
			}
		}


		if ( !mCurrentCommLayer->isConnected() ) return;

		processNetworkData();

		if ( !mNetworkFrames.empty() )
		{
			uint32_t lastDisplayType = 0;

			for (uint32_t j=0; j<mNetworkFrames.size(); j++)
			{
				NetworkFrame &nf = mNetworkFrames[j];

				if ( j == 0 )
				{
					lastDisplayType = nf.mPrimType;
				}

				uint32_t ipcount = nf.mPrimCount;
				if ( ipcount > MAX_SEND_BUFFER )
				{
					ipcount = MAX_SEND_BUFFER;
				}
				const uint8_t *scan = static_cast<const uint8_t *>(nf.mData);
				uint32_t pcount = 0;
				for (uint32_t i=0; i<ipcount; i++)
				{
					RENDER_DEBUG::DebugPrimitive *prim = const_cast<RENDER_DEBUG::DebugPrimitive *>(reinterpret_cast<const RENDER_DEBUG::DebugPrimitive *>(scan));
					bool includePrimitive = true;
					if ( mCurrentNetworkFrame == mLastProcessNetworkFrame )
					{
						switch ( uint32_t(prim->mCommand) )
						{
							case RENDER_DEBUG::DebugCommand::DEBUG_MESSAGE:
							case RENDER_DEBUG::DebugCommand::DEBUG_CREATE_TRIANGLE_MESH:
							case RENDER_DEBUG::DebugCommand::DEBUG_REFRESH_TRIANGLE_MESH_VERTICES:
							case RENDER_DEBUG::DebugCommand::DEBUG_RELEASE_TRIANGLE_MESH:
							case RENDER_DEBUG::DebugCommand::DEBUG_CREATE_CUSTOM_TEXTURE:
							case RENDER_DEBUG::DebugCommand::DEBUG_REGISTER_INPUT_EVENT:
							case RENDER_DEBUG::DebugCommand::DEBUG_RESET_INPUT_EVENTS:
							case RENDER_DEBUG::DebugCommand::DEBUG_UNREGISTER_INPUT_EVENT:
							case RENDER_DEBUG::DebugCommand::DEBUG_SKIP_FRAME:
								includePrimitive = false;
								break;
						}
					}
					if ( includePrimitive )
					{
						mDebugPrimitives[pcount] = prim;
						pcount++;
					}
					uint32_t plen = RENDER_DEBUG::DebugCommand::getPrimtiveSize(*prim);
					scan+=plen;
				}
				if ( lastDisplayType != nf.mPrimType )
				{
					processRenderDebug->flush(iface,static_cast<RENDER_DEBUG::ProcessRenderDebug::DisplayType>(lastDisplayType));
					lastDisplayType = nf.mPrimType;
				}
				processRenderDebug->processRenderDebug(const_cast<const RENDER_DEBUG::DebugPrimitive **>(reinterpret_cast<RENDER_DEBUG::DebugPrimitive **>(mDebugPrimitives)),
														pcount,
														iface,
														static_cast<RENDER_DEBUG::ProcessRenderDebug::DisplayType>(nf.mPrimType));
			}
			processRenderDebug->flush(iface,static_cast<RENDER_DEBUG::ProcessRenderDebug::DisplayType>(lastDisplayType));
			processRenderDebug->flushFrame(iface);
			mLastProcessNetworkFrame = mCurrentNetworkFrame;
		}
	}

	void releaseNetworkCommands(void)
	{
		for (uint32_t i=0; i<mNetworkCommands.size(); i++)
		{
			NetworkCommmand nc = mNetworkCommands[i];
			PX_FREE(nc.mCommand);
		}
		mNetworkCommands.clear();
	}

	void releaseNetworkFrames(void)
	{
		for (uint32_t i=0; i<mNetworkFrames.size(); i++)
		{
			NetworkFrame &nf = mNetworkFrames[i];
			PX_FREE(nf.mBaseData);
		}
		mNetworkFrames.clear();
	}

	void releaseIncomingNetworkFrames(void)
	{
		for (uint32_t i=0; i<mIncomingNetworkFrames.size(); i++)
		{
			NetworkFrame &nf = mIncomingNetworkFrames[i];
			PX_FREE(nf.mBaseData);
		}
		mIncomingNetworkFrames.clear();
	}


	virtual bool serverWait(void)
	{
		bool ret = false;

		// Here we wait up until the maximum wait count to receive an ack from the server.
		if ( !mIsServer && mCurrentCommLayer && mCurrentCommLayer->isConnected() )
		{
			if ( mReceivedAck )
			{
				mReceivedAck = false;
			}
			else
			{
				bool ok = false;
				for (uint32_t i=0; i<mMaxServerWait; i++)
				{
					if ( !isConnected() )
					{
						ok = false;
						break;
					}
					processNetworkData();
					if ( mReceivedAck )
					{
						mReceivedAck = false;
						ok = true;
						break;
					}
					else
					{
						mTime.getElapsedSeconds();
						physx::shdfnd::Time::Second diff = mTime.peekElapsedSeconds();
						while ( diff < 0.001 )
						{
							diff = mTime.peekElapsedSeconds();
							processNetworkData();
							if ( mReceivedAck )
							{
								mReceivedAck = false;
								ok = true;
								break;
							}
							physx::shdfnd::Thread::sleep(0); // we are low priority since we are in a wait loop, give up timeslice to other thread if necessary
						}
						if ( ok )
						{
							break;
						}
						if ( mServerStallCallback )
						{
							if ( !mServerStallCallback->continueWaitingForServer(i+1) )
							{
								ok = false;
								break;
							}
						}
					}
				}
				if ( !ok )
				{
					mCurrentCommLayer = NULL;
					if ( mCommLayer )
					{
						mCommLayer->release();
						mCommLayer = NULL;
					}
					if ( mCommStream )
					{
						mCommStream->release();
						mCommStream = NULL;
					}

					releaseNetworkFrames();
				}
			}
		}

		return ret;
	}

	virtual const char **getCommand(uint32_t &argc)
	{
		const char **ret = NULL;
		argc = 0;

		mNetworkCommandMutex.lock();

		if ( !mNetworkCommands.empty() )
		{
			NetworkCommmand nc = mNetworkCommands[0];
			mNetworkCommands.remove(0);
			physx::intrinsics::memCopy(mCurrentCommand,nc.mCommand,nc.mCommandLength+1);
			PX_FREE(nc.mCommand);
			ret = mParser.getArgs(mCurrentCommand,argc);
		}

		mNetworkCommandMutex.unlock();

		return ret;
	}

	virtual bool sendCommand(const char *cmd)
	{
		bool ret = false;

		if ( cmd && mCurrentCommLayer)
		{
			uint32_t slen = uint32_t(strlen(cmd));
			if ( slen < MAX_SERVER_COMMAND_STRING )
			{
				uint32_t sendSize = sizeof(uint32_t)*2+slen+1;
				uint32_t *sendData = static_cast<uint32_t *>(PX_ALLOC(sendSize,"SendCommand"));
				sendData[0] = PT_COMMAND;
				sendData[1] = slen;
				physx::intrinsics::memCopy(&sendData[2],cmd,slen+1);
				mCurrentCommLayer->sendMessage(sendData,sendSize);
				PX_FREE(sendData);
				ret = true;
			}
		}

		return ret;
	}

	virtual bool isConnected(void)
	{
		if ( !mCurrentCommLayer ) return false;
		if ( mIsServer )
		{
			bool hasClient = mCurrentCommLayer->hasClient();
			if ( !hasClient && mUsedMeshId )
			{
				mUsedMeshId = false;
				mMeshId+=10000; // increment the baseline mesh-id for the next time the client runs.
				mHaveApplicationName = false;
			}
			return hasClient;
		}
		return mCurrentCommLayer->isConnected();
	}

	virtual uint32_t getCommunicationsFrame(void) const
	{
		if ( mIsServer )
		{
			return mCurrentNetworkFrame;
		}
		return mLastFrame;
	}

	virtual void finalizeFrame(uint32_t frameCount)
	{
		if ( mDataSent )
		{
			mDataSent = false;
			return;
		}
		if ( !mCurrentCommLayer )
		{
			return;
		}
		if ( mIsServer )
		{
			return;
		}

		if ( !mCurrentCommLayer->isConnected() )
		{
			return;
		}
		uint32_t packet[2];
		packet[0] = PT_FINALIZE_FRAME;
		packet[1] = frameCount;
		mCurrentCommLayer->sendMessage(packet,sizeof(packet));
	}

	virtual const char * getRemoteApplicationName(void)
	{
		const char *ret = NULL;

		if ( mIsServer && mHaveApplicationName )
		{
			ret = mApplicationName;
		}

		return ret;
	}

	/**
	\brief Transmits an arbitrary block of binary data to the remote machine.  The block of data can have a command and id associated with it.

	It is important to note that due to the fact the RenderDebug system is synchronized every single frame, it is strongly recommended
	that you only use this feature for relatively small data items; probably on the order of a few megabytes at most.  If you try to do
	a very large transfer, in theory it would work, but it might take a very long time to complete and look like a hang since it will
	essentially be blocking.

	\param nameSpace The namespace associated with this data transfer, for example this could indicate a remote file request.
	\param resourceName The name of the resource associated with this data transfer, for example the id could be the file name of a file transfer request.
	\param data The block of binary data to transmit, you are responsible for maintaining endian correctness of the internal data if necessary.
	\param dlen The length of the lock of data to transmit.

	\return Returns true if the data was queued to be transmitted, false if it failed.
	*/
	virtual bool sendRemoteResource(const char *nameSpace,
								const char *resourceName,
								const void *data,
								uint32_t dlen)
	{
		bool ret = false;

		if ( mCurrentCommLayer && mCurrentCommLayer->isConnected() && data && dlen )
		{
			uint32_t sendLen = sizeof(RemoteResourcePacket)+dlen;
			RemoteResourcePacket *sendPacket = static_cast<RemoteResourcePacket*>(PX_ALLOC(sendLen,"RemoteResource"));
			RemoteResourcePacket &rr = *sendPacket;

			rr.mCommand = PT_REMOTE_RESOURCE;
			rr.mLength = dlen;
			rr.mBigEndian = isBigEndian() ? uint32_t( 1) : uint32_t( 0);
			physx::shdfnd::strlcpy(rr.mNameSpace,MAX_RESOURCE_STRING,nameSpace);
			physx::shdfnd::strlcpy(rr.mResourceName,MAX_RESOURCE_STRING,resourceName);
			RemoteResourcePacket *dest = sendPacket;
			dest++;
			physx::intrinsics::memCopy(dest,data,dlen);
			ret = mCurrentCommLayer->sendMessage(sendPacket,sendLen);
			PX_FREE(sendPacket);

		}


		return ret;
	}

	/**
	\brief This function allows you to request a file from the remote machine by name.  If successful it will be returned via 'getRemoteData'

	\param nameSpace The namespace field associated with this request which will be returned by 'getRemoteData'
	\param resourceName The resource name being requested from the remote machine.

	\return Returns true if the request was queued to be transmitted, false if it failed.
	*/
	virtual bool requestRemoteResource(const char *nameSpace,
									const char *resourceName)
	{
		bool ret = false;

		if ( mCurrentCommLayer && mCurrentCommLayer->isConnected() )
		{
			RequestRemoteResource rr;
			rr.mCommand = PT_REQUEST_REMOTE_RESOURCE;
			physx::shdfnd::strlcpy(rr.mNameSpace,MAX_RESOURCE_STRING,nameSpace);
			physx::shdfnd::strlcpy(rr.mResourceName,MAX_RESOURCE_STRING,resourceName);
			ret = mCurrentCommLayer->sendMessage(&rr,sizeof(rr));
		}

		return ret;
	}

	/**
	\brief Retrieves a block of remotely transmitted binary data.

	\param nameSpace A a reference to a pointer which will store the namespace associated with this data transfer, for example this could indicate a remote file request.
	\param resourceName A reference to a pointer which will store the resource name associated with this data transfer, for example the id could be the file name of a file transfer request.
	\param dlen A reference that will contain length of the lock of data received.
	\param remoteIsBigEndian A reference to a boolean which will be set to true if the remote machine that sent this data is big endian format.

	\retrun A pointer to the block of data received.
	*/
	virtual const void * getRemoteResource(const char *&nameSpace,
										const char *&resourceName,
										uint32_t &dlen,
										bool &remoteIsBigEndian)
	{
		const void *ret = NULL;

		nameSpace = NULL;
		resourceName = NULL;
		dlen = 0;
		remoteIsBigEndian = 0;
		if ( !mRemoteResources.empty() )
		{
			if ( mCurrentRemoteResource.mData )
			{
				PX_FREE(mCurrentRemoteResource.mData);
				mCurrentRemoteResource.mData = NULL;
			}
			mCurrentRemoteResource = mRemoteResources[0];
			mRemoteResources.remove(0);

			ret = mCurrentRemoteResource.mData;
			nameSpace = mCurrentRemoteResource.mNameSpace;
			resourceName = mCurrentRemoteResource.mResourceName;
			dlen = mCurrentRemoteResource.mLength;
			remoteIsBigEndian = mCurrentRemoteResource.mBigEndian ? true : false;

		}

		return ret;
	}

	void releaseRemoteResources(void)
	{
		if ( mCurrentRemoteResource.mData )
		{
			PX_FREE(mCurrentRemoteResource.mData);
			mCurrentRemoteResource.mData = NULL;
		}
		for (uint32_t i=0; i<mRemoteResources.size(); i++)
		{
			RemoteResource &rr = mRemoteResources[i];
			PX_FREE(rr.mData);
		}
		mRemoteResources.clear();
	}

	virtual void sendInputEvent(const RENDER_DEBUG::InputEvent &ev)
	{
		if ( mCurrentCommLayer )
		{
			RENDER_DEBUG::InputEvent e = ev;
			e.mReserved = PT_INPUT_EVENT;
			mCurrentCommLayer->sendMessage(&e,sizeof(RENDER_DEBUG::InputEvent));
		}
	}

	virtual const RENDER_DEBUG::InputEvent *getInputEvent(bool flush)
	{
		const RENDER_DEBUG::InputEvent *ret = NULL;

		if ( !mInputEvents.empty() )
		{
			mCurrentInputEvent = mInputEvents[0];
			ret = &mCurrentInputEvent;
			if ( flush )
			{
				mInputEvents.remove(0);
			}
		}

		return ret;
	}

	/**
	\brief Set the base file name to record communications stream; or NULL to disable it.

	\param fileName The base name of the file to record the communications channel stream to, or NULL to disable it.
	*/
	virtual bool setStreamFilename(const char *fileName)
	{
		if ( fileName )
		{
			physx::shdfnd::strlcpy(mStreamName,256,fileName);
		}
		else
		{
			mStreamName[0] = 0;
			if ( mCommStream )
			{
				mCommStream->release();
				mCommStream = NULL;
			}
		}
		return internalSetStreamFilename(mStreamName);
	}

	bool internalSetStreamFilename(const char *fileName)
	{
		bool ret = true;

		mCurrentCommLayer = mCommLayer;

		if ( mIsServer && fileName[0] )
		{
			if ( mCommStream )
			{
				mCommStream->release();
				mCommStream = NULL;
			}
			if ( fileName )
			{
				char scratch[512];
				sprintf(scratch,"%s%02d.nvcs", fileName, mStreamCount );
				mCommStream = createCommStream(scratch,true,mCommLayer);
				if ( mCommStream )
				{
					mStreamCount++;
					mCurrentCommLayer = static_cast< CommLayer *>(mCommStream);
				}
				else
				{
					ret = false;
				}
			}
		}
		return ret;
	}

	/**
	\brief Begin playing back a communications stream recording

	\param fileName The name of the previously captured communications stream file
	*/
	virtual bool setStreamPlayback(const char *fileName)
	{
		if ( !mIsServer ) return false;
		if ( mCommLayer->hasClient() ) return false;
		if ( mCommStream )
		{
			mCommStream->release();
			mCommStream = NULL;
		}
		bool ret = false;
		mCurrentCommLayer = mCommLayer;
		mCommStream = createCommStream(fileName,false,mCommLayer);
		if ( mCommStream )
		{
			ret = true;
			mCurrentCommLayer = mCommStream;
		}
		return ret;
	}

	bool								mSkipFrame;
	char								mStreamName[256];
	uint32_t							mStreamCount;
	bool								mWasConnected;
	bool								mDataSent;	// whether or not any data was sent this frame
	bool								mBadVersion;
	bool								mReceivedAck;
	uint32_t							mMeshId;
	bool								mUsedMeshId;
	uint32_t							mLastFrame;
	bool								mIsServer;
	RENDER_DEBUG::DebugPrimitive		*mDebugPrimitives[MAX_SEND_BUFFER];
	char								mCurrentCommand[MAX_SERVER_COMMAND_STRING];
	CommLayer							*mCommLayer;
	CommStream							*mCommStream;
	CommLayer							*mCurrentCommLayer;
	uint32_t							mLastProcessNetworkFrame;
	uint32_t							mCurrentNetworkFrame;
	NetworkFrameArray					mIncomingNetworkFrames;
	NetworkFrameArray					mNetworkFrames;
	NetworkCommandArray					mNetworkCommands;
	physx::shdfnd::Mutex				mNetworkCommandMutex;
	uint32_t							mMaxServerWait;
	RENDER_DEBUG::RenderDebug::ServerStallCallback	*mServerStallCallback;
	bool								mHaveApplicationName;
	char								mApplicationName[512];
	GetArgs								mParser;
	RENDER_DEBUG::RenderDebugResource	*mRenderDebugResource;
	RemoteResource						mCurrentRemoteResource;
	RemoteResourceArray					mRemoteResources;
	RENDER_DEBUG::InputEvent			mCurrentInputEvent;
	InputEventArray						mInputEvents;
	physx::shdfnd::Time				mTime;
};

ClientServer *createClientServer(RENDER_DEBUG::RenderDebug::Desc &desc)
{
	ClientServer *ret = NULL;
	desc.errorCode = NULL;
	ClientServerImpl *c = PX_NEW(ClientServerImpl)(desc);
	if ( desc.errorCode )
	{
		c->release();
	}
	else
	{
		ret = static_cast< ClientServer *>(c);
	}
	return ret;
}
