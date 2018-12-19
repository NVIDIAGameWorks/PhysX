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


#include <stdio.h>
#include "UserAllocator.h"

#include "MemTracker.h"

#include "PxAssert.h"

#pragma warning(disable:4100 4152)

#ifdef USE_MEM_TRACKER

static const char* USERALLOCATOR = "UserAllocator";
#endif

/*============  UserPxAllocator  ============*/

#ifdef USE_MEM_TRACKER

#include <windows.h>

class MemMutex
{
public:
	MemMutex(void);
	~MemMutex(void);

public:
	// Blocking Lock.
	void Lock(void);

	// Unlock.
	void Unlock(void);

private:
	CRITICAL_SECTION m_Mutex;
};

//==================================================================================
MemMutex::MemMutex(void)
{
	InitializeCriticalSection(&m_Mutex);
}

//==================================================================================
MemMutex::~MemMutex(void)
{
	DeleteCriticalSection(&m_Mutex);
}

//==================================================================================
// Blocking Lock.
//==================================================================================
void MemMutex::Lock(void)
{
	EnterCriticalSection(&m_Mutex);
}

//==================================================================================
// Unlock.
//==================================================================================
void MemMutex::Unlock(void)
{
	LeaveCriticalSection(&m_Mutex);
}

static inline MemMutex& gMemMutex()
{
	static MemMutex sMemMutex;
	return sMemMutex;
}

static size_t getThreadId(void)
{
	return GetCurrentThreadId();
}

static bool memoryReport(MEM_TRACKER::MemTracker *memTracker,MEM_TRACKER::MemoryReportFormat format,const char *fname,bool reportAllLeaks) // detect memory leaks and, if any, write out a report to the filename specified.
{
	bool ret = false;

	size_t leakCount;
	size_t leaked = memTracker->detectLeaks(leakCount);
	if ( leaked )
	{
		uint32_t dataLen;
		void *mem = memTracker->generateReport(format,fname,dataLen,reportAllLeaks);
		if ( mem )
		{
			FILE *fph = fopen(fname,"wb");
			fwrite(mem,dataLen,1,fph);
			fclose(fph);
			memTracker->releaseReportMemory(mem);
		}
		ret = true; // it leaked memory!
	}
	return ret;
}

class ScopedLock
{
public:
	ScopedLock(MemMutex &mutex, bool enabled) : mMutex(mutex), mEnabled(enabled) 
	{
		if (mEnabled) mMutex.Lock(); 
	}
	~ScopedLock()
	{
		if (mEnabled) mMutex.Unlock(); 
	}

private:
	ScopedLock();
	ScopedLock(const ScopedLock&);
	ScopedLock& operator=(const ScopedLock&);

	MemMutex& mMutex;
	bool mEnabled;
};

#define SCOPED_TRACKER_LOCK_IF(USE_TRACKER) ScopedLock lock(gMemMutex(), (USE_TRACKER))
#define TRACKER_CALL_IF(USE_TRACKER, CALL)  if ((USE_TRACKER)) { (CALL); }

void releaseAndReset(MEM_TRACKER::MemTracker*& memTracker)
{
	MEM_TRACKER::releaseMemTracker(memTracker);
	memTracker = NULL;
}

#else

#define SCOPED_TRACKER_LOCK_IF(USE_TRACKER) 
#define TRACKER_CALL_IF(USE_TRACKER, CALL)  

#endif

int UserPxAllocator::gMemoryTrackerClients = 0;
MEM_TRACKER::MemTracker	*UserPxAllocator::mMemoryTracker=NULL;

unsigned int UserPxAllocator::mNumAllocations = 0;
unsigned int UserPxAllocator::mNumFrees = 0;

UserPxAllocator::UserPxAllocator(const char* context,
                                 const char* /*dllName*/,
                                 bool useTrackerIfSupported /* = true */)
	: mContext(context)
	, mMemoryAllocated(0)
	, mUseTracker(useTrackerIfSupported)
{
	SCOPED_TRACKER_LOCK_IF(mUseTracker);
	TRACKER_CALL_IF(mUseTracker && NULL == mMemoryTracker, mMemoryTracker = MEM_TRACKER::createMemTracker());
	TRACKER_CALL_IF(trackerEnabled(), ++gMemoryTrackerClients);
}

void* UserPxAllocator::allocate(size_t size, const char* typeName, const char* filename, int line)
{
	void* ret = 0;

	PX_UNUSED(typeName);
	PX_UNUSED(filename);
	PX_UNUSED(line);
	SCOPED_TRACKER_LOCK_IF(trackerEnabled());

#if PX_WINDOWS_FAMILY
	ret = ::_aligned_malloc(size, 16);
#elif PX_ANDROID || PX_LINUX_FAMILY || PX_SWITCH
	/* Allocate size + (15 + sizeof(void*)) bytes and shift pointer further, write original address in the beginning of block.*/
	/* Weirdly, memalign sometimes returns unaligned address */
	//	ret = ::memalign(size, 16);
	size_t alignment = 16;
	void* originalRet = ::malloc(size + 2*alignment + sizeof(void*));
	// find aligned location
	ret = ((char*)originalRet) + 2 * alignment - (((size_t)originalRet) & 0xF);
	// write block address prior to aligned position, so it could be possible to free it later
	::memcpy((char*)ret - sizeof(originalRet), &originalRet, sizeof(originalRet));
#else
	ret = ::malloc(size);
#endif

	TRACKER_CALL_IF(trackerEnabled(), mMemoryAllocated += size);
	TRACKER_CALL_IF(trackerEnabled(), mMemoryTracker->trackAlloc(getThreadId(),ret, size, MEM_TRACKER::MT_MALLOC, USERALLOCATOR, typeName, filename, (uint32_t)line));

	// this should probably be a atomic increment
	mNumAllocations++;

	return ret;
}

void UserPxAllocator::deallocate(void* memory)
{
	SCOPED_TRACKER_LOCK_IF(trackerEnabled());

	if (memory)
	{
#if PX_WINDOWS_FAMILY
		::_aligned_free(memory);
#elif PX_ANDROID || PX_LINUX_FAMILY || PX_SWITCH
		// Looks scary, but all it does is getting original unaligned block address back from 4/8 bytes (depends on pointer size) prior to <memory> pointer and frees this memory 
		void* originalPtr = (void*)(*(size_t*)((char*)memory - sizeof(void*)));
		::free(originalPtr);
#else
		::free(memory);
#endif

		// this should probably be a atomic decrement
		mNumFrees++;
	}

	TRACKER_CALL_IF(trackerEnabled(), mMemoryTracker->trackFree(getThreadId(), memory, MEM_TRACKER::MT_FREE, USERALLOCATOR, __FILE__, __LINE__));
}


UserPxAllocator::~UserPxAllocator()
{
	SCOPED_TRACKER_LOCK_IF(trackerEnabled());
	TRACKER_CALL_IF(trackerEnabled() && (0 == --gMemoryTrackerClients), releaseAndReset(mMemoryTracker));
}

bool UserPxAllocator::dumpMemoryLeaks(const char* filename)
{
	bool leaked = false;

	PX_UNUSED(filename);
	TRACKER_CALL_IF(mMemoryTracker, leaked = memoryReport(mMemoryTracker, MEM_TRACKER::MRF_SIMPLE_HTML, filename, true));

	return leaked;
}

#undef SCOPED_TRACKER_LOCK_IF
#undef TRACKER_CALL_IF