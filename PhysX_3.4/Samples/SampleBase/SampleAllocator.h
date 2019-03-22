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

#ifndef SAMPLE_ALLOCATOR_H
#define SAMPLE_ALLOCATOR_H

#include "foundation/PxAllocatorCallback.h"
#include "common/PxPhysXCommonConfig.h"
#include "PsMutex.h"
#include "PxTkNamespaceMangle.h"

using namespace physx;

	class PxSampleAllocator : public PxAllocatorCallback
	{
		public:
						PxSampleAllocator();
						~PxSampleAllocator();

		virtual void*	allocate(size_t size, const char* typeName, const char* filename, int line);
				void*	allocate(size_t size, const char* filename, int line)	{ return allocate(size, NULL, filename, line); }
		virtual void	deallocate(void* ptr);

		protected:
				Ps::MutexT<Ps::RawAllocator> mMutex;

				void**		mMemBlockList;
				PxU32		mMemBlockListSize;
				PxU32		mFirstFree;
				PxU32		mMemBlockUsed;

		public:
				PxI32		mNbAllocatedBytes;
				PxI32		mHighWaterMark;
				PxI32		mTotalNbAllocs;
				PxI32		mNbAllocs;
	};

	void				initSampleAllocator();
	void				releaseSampleAllocator();
	PxSampleAllocator*	getSampleAllocator();

	class SampleAllocateable
	{
		public:
		PX_FORCE_INLINE	void*	operator new		(size_t, void* ptr)													{ return ptr;	}
		PX_FORCE_INLINE	void*	operator new		(size_t size, const char* handle, const char * filename, int line)	{ return getSampleAllocator()->allocate(size, handle, filename, line);	}
		PX_FORCE_INLINE	void*	operator new[]		(size_t size, const char* handle, const char * filename, int line)	{ return getSampleAllocator()->allocate(size, handle, filename, line);	}
		PX_FORCE_INLINE	void	operator delete		(void* p)															{ getSampleAllocator()->deallocate(p);	}
		PX_FORCE_INLINE	void	operator delete		(void* p, PxU32, const char*, int)									{ getSampleAllocator()->deallocate(p);	}
		PX_FORCE_INLINE	void	operator delete		(void* p, const char*, const char *, int)							{ getSampleAllocator()->deallocate(p);	}
		PX_FORCE_INLINE	void	operator delete[]	(void* p)															{ getSampleAllocator()->deallocate(p);	}
		PX_FORCE_INLINE	void	operator delete[]	(void* p, PxU32, const char*, int)									{ getSampleAllocator()->deallocate(p);	}
		PX_FORCE_INLINE	void	operator delete[]	(void* p, const char*, const char *, int)							{ getSampleAllocator()->deallocate(p);	}
	};

	#define SAMPLE_ALLOC(x)	getSampleAllocator()->allocate(x, 0, __FILE__, __LINE__)
	#define	SAMPLE_FREE(x)	if(x)	{ getSampleAllocator()->deallocate(x); x = NULL;	}
	#define SAMPLE_NEW(x)	new(#x, __FILE__, __LINE__) x

#endif
