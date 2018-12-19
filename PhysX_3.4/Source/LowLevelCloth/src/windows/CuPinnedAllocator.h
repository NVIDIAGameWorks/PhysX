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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#pragma once

#include "cudamanager/PxCudaContextManager.h"
#include "cudamanager/PxCudaMemoryManager.h"
#include "Allocator.h"
#include "CuCheckSuccess.h"
#include <cuda.h>

namespace physx
{

namespace cloth
{

struct CuHostAllocator
{
	CuHostAllocator(physx::PxCudaContextManager* ctx = NULL, unsigned int flags = cudaHostAllocDefault)
	: mDevicePtr(0), mFlags(flags), mManager(0)
	{
		PX_ASSERT(ctx);

		if(ctx)
			mManager = ctx->getMemoryManager();
	}

	void* allocate(size_t n, const char*, int)
	{
		physx::PxCudaBufferPtr bufferPtr;

		PX_ASSERT(mManager);

		if(mFlags & cudaHostAllocWriteCombined)
			bufferPtr = mManager->alloc(physx::PxCudaBufferMemorySpace::T_WRITE_COMBINED, n,
			                            PX_ALLOC_INFO("cloth::CuHostAllocator::T_WRITE_COMBINED", CLOTH));
		else if(mFlags & cudaHostAllocMapped)
			bufferPtr = mManager->alloc(physx::PxCudaBufferMemorySpace::T_PINNED_HOST, n,
			                            PX_ALLOC_INFO("cloth::CuHostAllocator::T_PINNED_HOST", CLOTH));
		else
			bufferPtr = mManager->alloc(physx::PxCudaBufferMemorySpace::T_HOST, n,
			                            PX_ALLOC_INFO("cloth::CuHostAllocator::T_HOST", CLOTH));

		if(mFlags & cudaHostAllocMapped)
			checkSuccess(cuMemHostGetDevicePointer(&mDevicePtr, reinterpret_cast<void*>(bufferPtr), 0));

		return reinterpret_cast<void*>(bufferPtr);
	}

	void deallocate(void* p)
	{
		PX_ASSERT(mManager);

		if(mFlags & cudaHostAllocWriteCombined)
			mManager->free(physx::PxCudaBufferMemorySpace::T_WRITE_COMBINED, physx::PxCudaBufferPtr(p));
		else if(mFlags & cudaHostAllocMapped)
			mManager->free(physx::PxCudaBufferMemorySpace::T_PINNED_HOST, physx::PxCudaBufferPtr(p));
		else
			mManager->free(physx::PxCudaBufferMemorySpace::T_HOST, physx::PxCudaBufferPtr(p));

		// don't reset mDevicePtr because Array::recreate deallocates last
	}

	CUdeviceptr mDevicePtr; // device pointer of last allocation
	unsigned int mFlags;
	physx::PxCudaMemoryManager* mManager;
};

template <typename T>
CuHostAllocator getMappedAllocator(physx::PxCudaContextManager* ctx)
{
	return CuHostAllocator(ctx, cudaHostAllocMapped | cudaHostAllocWriteCombined);
}

template <typename T>
struct CuPinnedVector
{
	// note: always use shdfnd::swap() instead of Array::swap()
	// in order to keep cached device pointer consistent
	typedef shdfnd::Array<T, typename physx::cloth::CuHostAllocator> Type;
};

template <typename T>
T* getDevicePointer(shdfnd::Array<T, typename physx::cloth::CuHostAllocator>& vector)
{
	// cached device pointer only valid if non-empty
	return vector.empty() ? 0 : reinterpret_cast<T*>(vector.getAllocator().mDevicePtr);
}

} // namespace cloth

} // namespace physx

namespace physx
{
namespace shdfnd
{
template <typename T>
void swap(Array<T, typename physx::cloth::CuHostAllocator>& left, Array<T, typename physx::cloth::CuHostAllocator>& right)
{
	swap(left.getAllocator(), right.getAllocator());
	left.swap(right);
}
}
}
