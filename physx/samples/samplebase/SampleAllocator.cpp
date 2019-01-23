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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include <stdio.h>
#include <assert.h>
#include "SampleAllocator.h"
#include "RendererMemoryMacros.h"
#include "foundation/PxAssert.h"
#include "foundation/PxErrorCallback.h"
#include "PsString.h"

PxErrorCallback& getSampleErrorCallback();

#if defined(WIN32)
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
#elif PX_LINUX_FAMILY
	static void* platformAlignedAlloc(size_t size)
	{
		return ::memalign(16, size);
	}

	static void platformAlignedFree(void* ptr)
	{
		::free(ptr);
	}
#else

// on Win64 we get 16-byte alignment by default
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


#define	DEBUG_IDENTIFIER	0xBeefBabe
#define	DEBUG_DEALLOCATED	0xDeadDead
#define	INVALID_ID			0xffffffff
#define MEMBLOCKSTART		64

#if PX_DEBUG || PX_PROFILE
static void print(const char* buffer)
{
	shdfnd::printFormatted("%s", buffer);
#if PX_WINDOWS
	if(buffer)	{ _RPT0(_CRT_WARN, buffer); }
#endif
}
#endif

#if PX_DEBUG || PX_PROFILE
	struct DebugBlock
	{
		const char*		mFilename;
#if !PX_P64_FAMILY
		PxU32			mPad0;
#endif

		const char*		mHandle;
#if !PX_P64_FAMILY
		PxU32			mPadHandle;
#endif
		PxU32			mCheckValue;
		PxU32			mSize;
		PxU32			mSlotIndex;
		PxU32			mLine;
	};

PX_COMPILE_TIME_ASSERT(!(sizeof(DebugBlock)&15));
#endif

PxSampleAllocator::PxSampleAllocator() :
	mMemBlockList		(NULL),
	mMemBlockListSize	(0),
	mFirstFree			(INVALID_ID),
	mMemBlockUsed		(0),
	mNbAllocatedBytes	(0),
	mHighWaterMark		(0),
	mTotalNbAllocs		(0),
	mNbAllocs			(0)
{
#if PX_DEBUG || PX_PROFILE
	// Initialize the Memory blocks list (DEBUG mode only)
	mMemBlockList = (void**)::malloc(MEMBLOCKSTART*sizeof(void*));
	memset(mMemBlockList, 0, MEMBLOCKSTART*sizeof(void*));
	mMemBlockListSize = MEMBLOCKSTART;
#endif
}

PxSampleAllocator::~PxSampleAllocator()
{
#if PX_DEBUG || PX_PROFILE
	char buffer[4096];
	if(mNbAllocatedBytes)
	{
		sprintf(buffer, "Memory leak detected: %d bytes non released\n", mNbAllocatedBytes);
		print(buffer);
	}
	if(mNbAllocs)
	{
		sprintf(buffer, "Remaining allocs: %d\n", mNbAllocs);
		print(buffer);
	}
	sprintf(buffer, "Total nb alloc: %d\n", mTotalNbAllocs);
	print(buffer);
	sprintf(buffer, "High water mark: %d Kb\n", mHighWaterMark/1024);
	print(buffer);

	// Scanning for memory leaks
	if(mMemBlockList && mNbAllocs)
	{
		PxU32 NbLeaks = 0;
		sprintf(buffer, "\n\n  SampleAllocator: Memory leaks detected :\n\n");
		print(buffer);

		for(PxU32 i=0; i<mMemBlockUsed; i++)
		{
			if(size_t(mMemBlockList[i])&1)
				continue;

			const DebugBlock* DB = (const DebugBlock*)mMemBlockList[i];
			sprintf(buffer, " Address 0x%p, %d bytes, allocated in: %s(%d):\n\n", (void*)(DB+1), DB->mSize, DB->mFilename, DB->mLine);
			print(buffer);

			NbLeaks++;
		}

		sprintf(buffer, "\n  Dump complete (%d leaks)\n\n", NbLeaks);
		print(buffer);
	}
	// Free the Memory Block list
	if(mMemBlockList)	::free(mMemBlockList);
	mMemBlockList = NULL;
#endif
}

void* PxSampleAllocator::allocate(size_t size, const char* typeName, const char* filename, int line)
{
	if(!size)
		return NULL;

#if PX_DEBUG || PX_PROFILE
	Ps::MutexT<Ps::RawAllocator>::ScopedLock lock(mMutex);

	// Allocate one debug block in front of each real allocation
	const size_t neededSize = size + sizeof(DebugBlock);
	void* ptr = platformAlignedAlloc(neededSize);

	if (NULL != ptr)
	{
		// Fill debug block
		DebugBlock* DB = (DebugBlock*)ptr;
		DB->mCheckValue	= DEBUG_IDENTIFIER;
		DB->mSize		= PxU32(size);
		DB->mLine		= line;
		DB->mSlotIndex	= INVALID_ID;
		DB->mFilename	= filename;
		DB->mHandle		= typeName ? typeName : "";

		// Update global stats
		mTotalNbAllocs++;
		mNbAllocs++;
		mNbAllocatedBytes += PxU32(size);
		if(mNbAllocatedBytes>mHighWaterMark)
			mHighWaterMark = mNbAllocatedBytes;

		// Insert the allocated block in the debug memory block list
		if(mMemBlockList)
		{
			if(mFirstFree!=INVALID_ID)
			{
				// Recycle old location
				PxU32 NextFree = (PxU32)(size_t)(mMemBlockList[mFirstFree]);
				if(NextFree!=INVALID_ID)
					NextFree>>=1;

				mMemBlockList[mFirstFree] = ptr;
				DB->mSlotIndex = mFirstFree;

				mFirstFree = NextFree;
			}
			else
			{
				if(mMemBlockUsed==mMemBlockListSize)
				{
					// Allocate a bigger block
					void** tps = (void**)::malloc((mMemBlockListSize+MEMBLOCKSTART)*sizeof(void*));
					// Copy already used part
					memcpy(tps, mMemBlockList, mMemBlockListSize*sizeof(void*));
					// Initialize remaining part
					void* Next = tps + mMemBlockListSize;
					memset(Next, 0, MEMBLOCKSTART*sizeof(void*));

					// Free previous memory, setup new pointer
					::free(mMemBlockList);
					mMemBlockList = tps;
					// Setup new size
					mMemBlockListSize += MEMBLOCKSTART;
				}

				mMemBlockList[mMemBlockUsed] = ptr;
				DB->mSlotIndex = mMemBlockUsed++;
			}
		}
		return ((PxU8*)ptr) + sizeof(DebugBlock);
	}
#else
	void* ptr = platformAlignedAlloc(size);
	if (NULL != ptr)
		return ptr;
#endif
	getSampleErrorCallback().reportError(PxErrorCode::eOUT_OF_MEMORY, "NULL ptr returned\n", __FILE__, __LINE__);
	return NULL;
}

void PxSampleAllocator::deallocate(void* memory)
{
	if(!memory)
		return;

#if PX_DEBUG || PX_PROFILE
	Ps::MutexT<Ps::RawAllocator>::ScopedLock lock(mMutex);

	DebugBlock* DB = ((DebugBlock*)memory)-1;

	// Check we allocated it
	if(DB->mCheckValue!=DEBUG_IDENTIFIER)
	{
		shdfnd::printFormatted("Error: free unknown memory!!\n");
		// ### should we really continue??
		return;
	}

	// Update global stats
	mNbAllocatedBytes -= DB->mSize;
	mNbAllocs--;

	// Remove the block from the Memory block list
	if(mMemBlockList)
	{
		PxU32 FreeSlot = DB->mSlotIndex;
		assert(mMemBlockList[FreeSlot]==DB);

		PxU32 NextFree = mFirstFree;
		if(NextFree!=INVALID_ID)
		{
			NextFree<<=1;
			NextFree|=1;
		}

		mMemBlockList[FreeSlot] = (void*)size_t(NextFree);
		mFirstFree = FreeSlot;
	}

	// ### should be useless since we'll release the memory just afterwards
	DB->mCheckValue = DEBUG_DEALLOCATED;
	DB->mSize		= 0;
	DB->mHandle		= 0;
	DB->mFilename	= NULL;
	DB->mSlotIndex	= INVALID_ID;
	DB->mLine		= INVALID_ID;

	platformAlignedFree(DB);
#else
	platformAlignedFree(memory);
#endif
}

static PxSampleAllocator* gAllocator = NULL;

void initSampleAllocator()
{
	PX_ASSERT(!gAllocator);
	gAllocator = new PxSampleAllocator;
}

void releaseSampleAllocator()
{
	DELETESINGLE(gAllocator);
}

PxSampleAllocator* getSampleAllocator()
{
	PX_ASSERT(gAllocator);
	return gAllocator;
}
