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


#include "RenderDebugTyped.h"


#include "RenderDebugImpl.h"
#include "ClientServer.h"
#include "ProcessRenderDebug.h"
#include "InternalRenderDebug.h"
#include "FileRenderDebug.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "PsUserAllocated.h"
#include "PsMutex.h"
#include "PsString.h"
#include "PxErrorCallback.h"
#include "PxAllocatorCallback.h"
#include "PsArray.h"
#include "PsFoundation.h"

namespace RENDER_DEBUG
{
class RenderDebugIImpl;
void finalRelease(RenderDebugIImpl *nv);
}

#include "GetArgs.h"
#include "PsFileBuffer.h"

#if PX_WINDOWS_FAMILY
#pragma warning(disable:4986)
#endif

#define SERVER_FILE_SIZE sizeof(ServerHeader)+(1024*1024)

class EchoRemoteCommand
{
public:
	char	mCommand[16384];
};

typedef physx::shdfnd::Array< EchoRemoteCommand > EchoRemoteCommandVector;

#ifdef USE_PX_FOUNDATION
#else
class DefaultErrorCallback : public physx::PxErrorCallback
{
public:
	DefaultErrorCallback(void)
	{
	}

	virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line)
	{
		PX_UNUSED(code);
		PX_UNUSED(file);
		PX_UNUSED(line);
		printf("PhysX: %s\r\n", message );
	}
private:
};

#if PX_WINDOWS_FAMILY
// on win32 we only have 8-byte alignment guaranteed, but the CRT provides special aligned allocation
// fns
#include <malloc.h>
#include <crtdbg.h>

static void* platformAlignedAlloc(size_t size)
{
	return _aligned_malloc(size, 16);	
}

static void platformAlignedFree(void* ptr)
{
	_aligned_free(ptr);
}
#elif PX_LINUX || PX_ANDROID
static void* platformAlignedAlloc(size_t size)
{
	return ::memalign(16, size);
}

static void platformAlignedFree(void* ptr)
{
	::free(ptr);
}
#elif PX_WIIU
static void* platformAlignedAlloc(size_t size)
{
	size_t pad = 15 + sizeof(size_t); // store offset for delete.
	uint8_t* base = (uint8_t*)::malloc(size+pad);
	if(!base)
		return NULL;

	uint8_t* ptr = (uint8_t*)(size_t(base + pad) & ~(15)); // aligned pointer
	((size_t*)ptr)[-1] = ptr - base; // store offset

	return ptr;
}

static void platformAlignedFree(void* ptr)
{
	if(ptr == NULL)
		return;

	uint8_t* base = ((uint8_t*)ptr) - ((size_t*)ptr)[-1];
	::free(base);
}
#else

// on PS3, XBox and Win64 we get 16-byte alignment by default
static void* platformAlignedAlloc(size_t size)
{
	void *ptr = ::malloc(size);	
	PX_ASSERT((reinterpret_cast<size_t>(ptr) & 15)==0);
	return ptr;
}

static void platformAlignedFree(void* ptr)
{
	::free(ptr);			
}
#endif


class DefaultAllocator : public physx::PxAllocatorCallback
{
public:
	DefaultAllocator(void)
	{
	}

	~DefaultAllocator(void)
	{
	}


	virtual void* allocate(size_t size, const char* typeName, const char* filename, int line)
	{
		PX_UNUSED(typeName);
		PX_UNUSED(filename);
		PX_UNUSED(line);
		void *ret = platformAlignedAlloc(size);
		return ret;
	}

	virtual void deallocate(void* ptr)
	{
		platformAlignedFree(ptr);
	}
private:
};


DefaultAllocator		gDefaultAllocator;
DefaultErrorCallback	gDefaultErrorCallback;
#endif

#define RECORD_REMOTE_COMMANDS_VERSION 100

static char magic[4] = { 'R', 'R', 'C', 'V' };

static uint32_t isBigEndian(void)
{
	int32_t i = 1;
	bool b = *(reinterpret_cast<char*>(&i))==0;
	return b ? uint32_t(1) : uint32_t(0);
}


namespace RENDER_DEBUG
{

class PlaybackRemoteCommand
{
public:
	PlaybackRemoteCommand(void)
	{
		mFrameNumber = 0;
		mArgc = 0;
		for (uint32_t i=0; i<256; i++)
		{
			mArgv[i] = NULL;
		}
	}
	void release(void)
	{
		mFrameNumber = 0;
		for (uint32_t i=0; i<mArgc; i++)
		{
			PX_FREE(static_cast<void *>(const_cast<char *>(mArgv[i])));
			mArgv[i] = NULL;
		}
		mArgc = 0;
	}
	uint32_t	mFrameNumber;
	uint32_t	mArgc;
	const char *mArgv[256];
};

class RenderDebugIImpl : public physx::shdfnd::UserAllocated, public RenderDebugHook
{
	PX_NOCOPY(RenderDebugIImpl)
public:
	RenderDebugIImpl(RENDER_DEBUG::RenderDebug::Desc &desc) :
		mEchoRemoteCommandBuffer(PX_DEBUG_EXP("PxRenderDebug::mEchoRemoteCommandBuffer"))
	{
		mEndianSwap = false;
		mRecordRemoteCommands = NULL;
		mPlaybackRemoteCommands = NULL;
		mCurrentFrame = 0;
		mMeshId = 0;
		mFileRenderDebug = NULL;
		mIsValid = true;
		mApexRenderDebug = NULL;
		mInternalProcessRenderDebug = NULL;
		mRenderDebug = NULL;
		mClientServer = NULL;
		mReadNext = false;
		switch ( desc.runMode )
		{
			case RenderDebug::RM_CLIENT:
			case RenderDebug::RM_CLIENT_OR_FILE:
			case RenderDebug::RM_SERVER:
				{
					if ( desc.runMode == RenderDebug::RM_SERVER ) // if running in server mode.
					{
						mClientServer = createClientServer(desc);
					}
					else 
					{
						mClientServer = createClientServer(desc);
						if ( mClientServer == NULL )
						{
							if ( desc.runMode != RenderDebug::RM_CLIENT_OR_FILE )
							{
								desc.errorCode = "Unable to locate server";
							}
							else
							{
								desc.errorCode = NULL;
							}
						}
					}
				}
				break;
			case RenderDebug::RM_LOCAL:
				break;
			case RenderDebug::RM_FILE:
				break;
		}

		if ( desc.runMode == RenderDebug::RM_FILE || desc.runMode == RenderDebug::RM_CLIENT_OR_FILE || desc.runMode == RENDER_DEBUG::RenderDebug::RM_CLIENT )
		{
			if ( desc.echoFileLocally )
			{
				mInternalProcessRenderDebug = RENDER_DEBUG::createProcessRenderDebug();
			}
			mFileRenderDebug = RENDER_DEBUG::createFileRenderDebug(desc.recordFileName,false,desc.echoFileLocally ? mInternalProcessRenderDebug : NULL, mClientServer);
			if ( mFileRenderDebug == NULL )
			{
				mIsValid = false;
				desc.errorCode = "Failed to open recording file";
			}
		}
		if ( mIsValid )
		{
			if ( mFileRenderDebug )
			{
				mMeshId = 1000000; // 
				mRenderDebug = RENDER_DEBUG::createInternalRenderDebug(mFileRenderDebug,this);
			}
			else
			{
				if ( mInternalProcessRenderDebug == NULL )
				{
					mInternalProcessRenderDebug = RENDER_DEBUG::createProcessRenderDebug();
				}
				mRenderDebug = RENDER_DEBUG::createInternalRenderDebug(mInternalProcessRenderDebug,this);
			}
		}
		if ( desc.errorCode )
		{
			mIsValid = false;
		}
		if ( mIsValid && desc.recordRemoteCommands )
		{
			mRecordRemoteCommands = PX_NEW(physx::PsFileBuffer)(desc.recordRemoteCommands, physx::PsFileBuffer::OPEN_WRITE_ONLY);
			if ( mRecordRemoteCommands->isOpen() )
			{
				mRecordRemoteCommands->write(magic, 4 );
				mRecordRemoteCommands->storeDword(RECORD_REMOTE_COMMANDS_VERSION);
				uint32_t bigEndian = isBigEndian();
				mRecordRemoteCommands->storeDword(bigEndian);
				mRecordRemoteCommands->flush();
			}
			else
			{
				delete mRecordRemoteCommands;
				mRecordRemoteCommands = NULL;
			}
		}
		if ( mIsValid && desc.playbackRemoteCommands )
		{
			mPlaybackRemoteCommands = PX_NEW(physx::PsFileBuffer)(desc.playbackRemoteCommands, physx::PsFileBuffer::OPEN_READ_ONLY);
			if ( mPlaybackRemoteCommands->isOpen() )
			{
				char temp[4];
				uint32_t r = mPlaybackRemoteCommands->read(temp,4);
				uint32_t version = mPlaybackRemoteCommands->readDword();
				uint32_t bigEndian = mPlaybackRemoteCommands->readDword();

				if ( r == 4 && magic[0] == temp[0] && magic[1] == temp[1] && magic[2] == temp[2] && magic[3] == temp[3] && version == RECORD_REMOTE_COMMANDS_VERSION )
				{
					if ( bigEndian != isBigEndian() )
					{
						mEndianSwap = true;
					}
					mReadNext = true;
				}
				else
				{
					delete mPlaybackRemoteCommands;
					mPlaybackRemoteCommands = NULL;
				}
			}
			else
			{
				delete mPlaybackRemoteCommands;
				mPlaybackRemoteCommands = NULL;
			}
		}
		mDesc = desc;
	}

	virtual ~RenderDebugIImpl(void)
	{
		delete mRecordRemoteCommands;
		delete mPlaybackRemoteCommands;
		mPlaybackRemoteCommand.release();
		if ( mFileRenderDebug )
		{
			mFileRenderDebug->release();
		}
		if ( mClientServer )
		{
			mClientServer->release();
		}
		if ( mRenderDebug )
		{
			mRenderDebug->releaseRenderDebug();
		}
		if ( mInternalProcessRenderDebug )
		{
			mInternalProcessRenderDebug->release();
		}
	}

    virtual bool render(float dtime,RENDER_DEBUG::RenderDebugInterface *iface) 
	{
		bool ret = true;

		mCurrentFrame++;

		mApexRenderDebug = iface;

		if ( mFileRenderDebug && mFileRenderDebug->getFrameCount() )
		{
			mFileRenderDebug->processReadFrame(iface);
		}

		if ( mClientServer && mDesc.runMode == RENDER_DEBUG::RenderDebug::RM_SERVER )
		{
			mClientServer->processFrame(mInternalProcessRenderDebug,iface);
		}

		ret = mRenderDebug->renderImpl(dtime,iface);

		return ret;
	}


	virtual void release(void)
	{
		finalRelease(this);
	}

	bool isValid(void) const
	{
		return mIsValid;
	}

	virtual uint32_t setFilePlayback(const char *fileName) 
	{
		uint32_t ret = 0;

		if ( mFileRenderDebug )
		{
			mFileRenderDebug->release();
			mFileRenderDebug = NULL;
		}

		mFileRenderDebug = RENDER_DEBUG::createFileRenderDebug(fileName,true,mInternalProcessRenderDebug, NULL);

		if ( mFileRenderDebug )
		{
			mFileRenderDebug->setFrame(0);
			ret = mFileRenderDebug->getFrameCount();
		}

		return ret;
	}

	virtual bool setPlaybackFrame(uint32_t playbackFrame) 
	{
		bool ret = false;

		if ( mFileRenderDebug && playbackFrame < mFileRenderDebug->getFrameCount() )
		{
			ret = true;
			mFileRenderDebug->setFrame(playbackFrame);
		}

		return ret;
	}

	virtual uint32_t getPlaybackFrameCount(void) const 
	{
		uint32_t ret = 0;
		if ( mFileRenderDebug )
		{
			ret = mFileRenderDebug->getFrameCount();
		}
		return ret;
	}

	virtual void stopPlayback(void) 
	{
		if ( mFileRenderDebug )
		{
			mFileRenderDebug->release();
			mFileRenderDebug = NULL;
		}
	}

	virtual bool trylock(void)
	{
		return mMutex.trylock();
	}

	virtual void lock(void) 
	{
		mMutex.lock();
	}

	virtual void unlock(void) 
	{
		mMutex.unlock();
	}

	/**
	\brief Convenience method to return a unique mesh id number (simply a global counter to avoid clashing with other ids
	*/
	virtual uint32_t getMeshId(void) 
	{
		mMeshId++;
		return mMeshId;
	}

	/**
	\brief Transmit an actual input event to the remote client

	\param ev The input event data to transmit
	*/
	virtual void sendInputEvent(const InputEvent &ev) 
	{
		if ( mClientServer )
		{
			mClientServer->sendInputEvent(ev);
		}
	}


	virtual bool sendRemoteCommand(const char *fmt,...) 
	{
		bool ret = false;

		EchoRemoteCommand c;
		va_list arg;
		va_start( arg, fmt );
		physx::shdfnd::vsnprintf(c.mCommand,sizeof(c.mCommand)-1, fmt, arg);
		va_end(arg);

		if ( mClientServer )
		{
			ret = mClientServer->sendCommand(c.mCommand);
		}
		else
		{
			mEchoRemoteCommandBuffer.pushBack(c);
		}
		return ret;
	}

	virtual const char ** getRemoteCommand(uint32_t &argc) 
	{
		const char **ret = NULL;
		argc = 0;

		if ( mClientServer )
		{
			ret = mClientServer->getCommand(argc);
		}
		else if ( !mEchoRemoteCommandBuffer.empty() )
		{
			mCurrentCommand = mEchoRemoteCommandBuffer[0];
			mEchoRemoteCommandBuffer.remove(0);
			ret = mParser.getArgs(mCurrentCommand.mCommand,argc);
		}

		if ( mReadNext )
		{
			readNextRemoteCommand();
			mReadNext = false;
		}

		if ( mPlaybackRemoteCommands )
		{
			ret = NULL;
			argc = 0;
			if ( mPlaybackRemoteCommand.mFrameNumber == mCurrentFrame )
			{
				argc = mPlaybackRemoteCommand.mArgc;
				ret = mPlaybackRemoteCommand.mArgv;
				mReadNext = true;
			}
		}

		if ( mRecordRemoteCommands && ret && argc < 256 )
		{
			mRecordRemoteCommands->storeDword(mCurrentFrame);
			mRecordRemoteCommands->storeDword(argc);
			for (uint32_t i=0; i<argc; i++)
			{
				const char *str = ret[i];
				uint32_t len = uint32_t(strlen(str))+1;
				mRecordRemoteCommands->storeDword(len);
				mRecordRemoteCommands->write(str,len);
			}
			mRecordRemoteCommands->flush();
		}

		return ret;
	}

	
	/**
	\brief Transmits an arbitrary block of binary data to the remote machine.  The block of data can have a command and id associated with it.

	It is important to note that due to the fact the RenderDebug system is synchronized every single frame, it is strongly recommended
	that you only use this feature for relatively small data items; probably on the order of a few megabytes at most.  If you try to do
	a very large transfer, in theory it would work, but it might take a very long time to complete and look like a hang since it will
	essentially be blocking.

	\param command An arbitrary command associated with this data transfer, for example this could indicate a remote file request.
	\param id An arbitrary id associated with this data transfer, for example the id could be the file name of a file transfer request.
	\param data The block of binary data to transmit, you are responsible for maintaining endian correctness of the internal data if necessary.
	\param dlen The length of the lock of data to transmit.

	\return Returns true if the data was queued to be transmitted, false if it failed.
	*/
	virtual bool sendRemoteResource(const char *command,
								const char *id,
								const void *data,
								uint32_t dlen) 
	{
		bool ret = false;

		if ( mClientServer )
		{
			ret = mClientServer->sendRemoteResource(command,id,data,dlen);
		}

		return ret;
	}

	/**
	\brief This function allows you to request a file from the remote machine by name.  If successful it will be returned via 'getRemoteData'

	\param command The command field associated with this request which will be returned by 'getRemoteData'
	\param fileName The filename being requested from the remote machine.

	\return Returns true if the request was queued to be transmitted, false if it failed.
	*/
	virtual bool requestRemoteResource(const char *command,
									const char *fileName)
	{
		bool ret = false;

		if ( mClientServer )
		{
			ret = mClientServer->requestRemoteResource(command,fileName);
		}

		return ret;
	}

	/**
	\brief Retrieves a block of remotely transmitted binary data.

	\param command A a reference to a pointer which will store the arbitrary command associated with this data transfer, for example this could indicate a remote file request.
	\param id A reference to a pointer which will store an arbitrary id associated with this data transfer, for example the id could be the file name of a file transfer request.
	\param dlen A reference that will contain length of the lock of data received.
	\param remoteIsBigEndian A reference to a boolean which will be set to true if the remote machine that sent this data is big endian format.

	\retrun A pointer to the block of data received.
	*/
	virtual const void * getRemoteResource(const char *&command,
										const char *&id,
										uint32_t &dlen,
										bool &remoteIsBigEndian)
	{
		const void *ret = NULL;

		if ( mClientServer )
		{
			ret = mClientServer->getRemoteResource(command,id,dlen,remoteIsBigEndian);
		}

		return ret;
	}


	virtual RENDER_DEBUG::RenderDebug::RunMode getRunMode(void)
	{
		RENDER_DEBUG::RenderDebug::RunMode ret = RENDER_DEBUG::RenderDebug::RM_LOCAL;
		if ( mClientServer )
		{
			if ( mClientServer->isServer() )
			{
				ret = RENDER_DEBUG::RenderDebug::RM_SERVER;
			}
			else
			{
				ret = RENDER_DEBUG::RenderDebug::RM_CLIENT;
			}
		}
		else if ( mFileRenderDebug )
		{
			ret = RENDER_DEBUG::RenderDebug::RM_FILE;
		}
		return ret;
	}

	virtual bool isConnected(void) const
	{
		bool ret = false;

		if ( mClientServer )
		{
			ret = mClientServer->isConnected();
		}

		return ret;
	}

	virtual uint32_t getCommunicationsFrame(void) const 
	{
		return mClientServer ? mClientServer->getCommunicationsFrame() : 0;
	}

	virtual const char *getRemoteApplicationName(void) 
	{
		const char *ret = NULL;

		if ( mClientServer )
		{
			ret = mClientServer->getRemoteApplicationName();
		}

		return ret;
	}

	virtual RenderDebugTyped *getRenderDebugTyped(void) 
	{
		return static_cast< RenderDebugTyped *>(mRenderDebug);
	}

	void readNextRemoteCommand(void)
	{
		if ( mPlaybackRemoteCommands == NULL ) return;
		mPlaybackRemoteCommand.release(); // release previous command
		mPlaybackRemoteCommand.mFrameNumber = mPlaybackRemoteCommands->readDword();
		if ( mPlaybackRemoteCommand.mFrameNumber == 0 )
		{
			mPlaybackRemoteCommand.release();
			delete mPlaybackRemoteCommands;
			mPlaybackRemoteCommands = NULL;
		}
		else
		{
			mPlaybackRemoteCommand.mArgc = mPlaybackRemoteCommands->readDword();
			for (uint32_t i=0; i<mPlaybackRemoteCommand.mArgc; i++)
			{
				uint32_t len = mPlaybackRemoteCommands->readDword();
				mPlaybackRemoteCommand.mArgv[i] = static_cast<const char *>(PX_ALLOC(len,"PlaybackRemoteCommand"));
				uint32_t rbytes = mPlaybackRemoteCommands->read(reinterpret_cast<void *>(const_cast<char*>(mPlaybackRemoteCommand.mArgv[i])),len);
				if ( rbytes != len )
				{
					mPlaybackRemoteCommand.release();
					delete mPlaybackRemoteCommands;
					mPlaybackRemoteCommands = NULL;
				}
			}
		}
	}

	
	/**
	\brief Returns any incoming input event for processing purposes
	*/
	virtual const InputEvent *getInputEvent(bool flush)
	{
		const InputEvent *ret = NULL;

		if ( mClientServer )
		{
			ret = mClientServer->getInputEvent(flush);
		}

		return ret;
	}

	/**
	\brief Set the base file name to record communications stream; or NULL to disable it.

	\param fileName The base name of the file to record the communications channel stream to, or NULL to disable it.
	*/
	virtual bool setStreamFilename(const char *fileName) 
	{
		bool ret = false;
		if ( mClientServer )
		{
			ret = mClientServer->setStreamFilename(fileName);
		}
		return ret;
	}

		/**
	\brief Begin playing back a communications stream recording

	\param fileName The name of the previously captured communications stream file
	*/
	virtual bool setStreamPlayback(const char *fileName)
	{
		bool ret = false;
		if ( mClientServer )
		{
			ret = mClientServer->setStreamPlayback(fileName);
		}
		return ret;
	}

	uint32_t									mCurrentFrame;
	uint32_t									mMeshId;
	physx::shdfnd::Mutex						mMutex;
	ClientServer								*mClientServer;
	RenderDebug::Desc							mDesc;
	bool										mIsValid;
	RENDER_DEBUG::RenderDebugInterface			*mApexRenderDebug;
	RENDER_DEBUG::RenderDebugImpl				*mRenderDebug;
	RENDER_DEBUG::FileRenderDebug				*mFileRenderDebug;
	RENDER_DEBUG::ProcessRenderDebug			*mInternalProcessRenderDebug;
	EchoRemoteCommand							mCurrentCommand;
	EchoRemoteCommandVector						mEchoRemoteCommandBuffer;
	GetArgs										mParser;
	physx::PsFileBuffer						*mRecordRemoteCommands;
	bool										mEndianSwap;
	physx::PsFileBuffer						*mPlaybackRemoteCommands;
	bool										mReadNext;
	PlaybackRemoteCommand						mPlaybackRemoteCommand;
};

RENDER_DEBUG::RenderDebug *createRenderDebug(RENDER_DEBUG::RenderDebug::Desc &desc)
{
	RENDER_DEBUG::RenderDebug *ret = NULL;

	if ( desc.versionNumber != RENDER_DEBUG_VERSION || desc.foundation == NULL)
	{
		return NULL;
	}

	RENDER_DEBUG::RenderDebugIImpl *c = PX_NEW(RENDER_DEBUG::RenderDebugIImpl)(desc);

	if ( !c->isValid() )
	{
		c->release();
		c = NULL;
	}
	else
	{
		ret = static_cast< RENDER_DEBUG::RenderDebug *>(c->getRenderDebugTyped());
	}

	return ret;
}

void finalRelease(RENDER_DEBUG::RenderDebugIImpl *nv)
{
	delete nv;
}

} // end of namespace


