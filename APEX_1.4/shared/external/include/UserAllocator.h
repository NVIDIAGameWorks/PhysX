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



#ifndef USERALLOCATOR_H
#define USERALLOCATOR_H

#include "ApexDefs.h"
#include "PxAllocatorCallback.h"
#include "ApexUsingNamespace.h"

#pragma warning(push)
#pragma warning(disable:4512)

#if PX_CHECKED || defined(_DEBUG)
#if PX_WINDOWS_FAMILY
#define USE_MEM_TRACKER
#endif
#endif

namespace MEM_TRACKER
{
	class MemTracker;
};

/* User allocator for APEX and 3.0 PhysX SDK */
class UserPxAllocator : public physx::PxAllocatorCallback
{
public:
	UserPxAllocator(const char* context, const char* dllName, bool useTrackerIfSupported = true);
	virtual		   ~UserPxAllocator();

	uint32_t	getHandle(const char* name);


	void*			allocate(size_t size, const char* typeName, const char* filename, int line);
	void			deallocate(void* ptr);

	size_t			getAllocatedMemoryBytes()
	{
		return mMemoryAllocated;
	}

	static bool dumpMemoryLeaks(const char* filename);

private:
	bool				trackerEnabled() const { return mUseTracker && (NULL != mMemoryTracker); }

	const char*			mContext;
	size_t				mMemoryAllocated;
	const bool			mUseTracker;

	static MEM_TRACKER::MemTracker	*mMemoryTracker;
	static int gMemoryTrackerClients;

	// Poor man's memory leak check
	static unsigned int mNumAllocations;
	static unsigned int mNumFrees;
};

#pragma warning(pop)

#endif
