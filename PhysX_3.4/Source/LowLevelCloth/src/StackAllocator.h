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

#include "foundation/PxAssert.h"

#if PX_LINUX_FAMILY
#include <stdint.h> // intptr_t
#endif

template <size_t align>
class StackAllocator
{
	typedef unsigned char byte;

	// todo: switch to offsets so size is consistent on x64
	// mSize is just for book keeping so could be 4 bytes
	struct Header
	{
		Header* mPrev;
		size_t mSize : 31;
		size_t mFree : 1;
	};

	StackAllocator(const StackAllocator&);
	StackAllocator& operator=(const StackAllocator&);

  public:
	StackAllocator(void* buffer, size_t bufferSize)
	: mBuffer(reinterpret_cast<byte*>(buffer)), mBufferSize(bufferSize), mFreeStart(mBuffer), mTop(0)
	{
	}

	~StackAllocator()
	{
		PX_ASSERT(userBytes() == 0);
	}

	void* allocate(size_t numBytes)
	{
		// this is non-standard
		if(!numBytes)
			return 0;

		uintptr_t unalignedStart = uintptr_t(mFreeStart) + sizeof(Header);

		byte* allocStart = reinterpret_cast<byte*>((unalignedStart + (align - 1)) & ~(align - 1));
		byte* allocEnd = allocStart + numBytes;

		// ensure there is space for the alloc
		PX_ASSERT(allocEnd <= mBuffer + mBufferSize);

		Header* h = getHeader(allocStart);
		h->mPrev = mTop;
		h->mSize = numBytes;
		h->mFree = false;

		mTop = h;
		mFreeStart = allocEnd;

		return allocStart;
	}

	void deallocate(void* p)
	{
		if(!p)
			return;

		Header* h = getHeader(p);
		h->mFree = true;

		// unwind the stack to the next live alloc
		while(mTop && mTop->mFree)
		{
			mFreeStart = reinterpret_cast<byte*>(mTop);
			mTop = mTop->mPrev;
		}
	}

  private:
	// return the header for an allocation
	inline Header* getHeader(void* p) const
	{
		PX_ASSERT((reinterpret_cast<uintptr_t>(p) & (align - 1)) == 0);
		PX_ASSERT(reinterpret_cast<byte*>(p) >= mBuffer + sizeof(Header));
		PX_ASSERT(reinterpret_cast<byte*>(p) < mBuffer + mBufferSize);

		return reinterpret_cast<Header*>(p) - 1;
	}

  public:
	// total user-allocated bytes not including any overhead
	size_t userBytes() const
	{
		size_t total = 0;
		Header* iter = mTop;
		while(iter)
		{
			total += iter->mSize;
			iter = iter->mPrev;
		}

		return total;
	}

	// total user-allocated bytes + overhead
	size_t totalUsedBytes() const
	{
		return mFreeStart - mBuffer;
	}

	size_t remainingBytes() const
	{
		return mBufferSize - totalUsedBytes();
	}

	size_t wastedBytes() const
	{
		return totalUsedBytes() - userBytes();
	}

  private:
	byte* const mBuffer;
	const size_t mBufferSize;

	byte* mFreeStart; // start of free space
	Header* mTop;     // top allocation header
};
