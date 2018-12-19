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
#include "Types.h"

namespace physx
{
namespace cloth
{

template <class T>
struct Range
{
	Range();

	Range(T* first, T* last);

	template <typename S>
	Range(const Range<S>& other);

	uint32_t size() const;
	bool empty() const;

	void popFront();
	void popBack();

	T* begin() const;
	T* end() const;

	T& front() const;
	T& back() const;

	T& operator[](uint32_t i) const;

  private:
	T* mFirst;
	T* mLast; // past last element
};

template <typename T>
Range<T>::Range()
: mFirst(0), mLast(0)
{
}

template <typename T>
Range<T>::Range(T* first, T* last)
: mFirst(first), mLast(last)
{
}

template <typename T>
template <typename S>
Range<T>::Range(const Range<S>& other)
: mFirst(other.begin()), mLast(other.end())
{
}

template <typename T>
uint32_t Range<T>::size() const
{
	return uint32_t(mLast - mFirst);
}

template <typename T>
bool Range<T>::empty() const
{
	return mFirst >= mLast;
}

template <typename T>
void Range<T>::popFront()
{
	PX_ASSERT(mFirst < mLast);
	++mFirst;
}

template <typename T>
void Range<T>::popBack()
{
	PX_ASSERT(mFirst < mLast);
	--mLast;
}

template <typename T>
T* Range<T>::begin() const
{
	return mFirst;
}

template <typename T>
T* Range<T>::end() const
{
	return mLast;
}

template <typename T>
T& Range<T>::front() const
{
	PX_ASSERT(mFirst < mLast);
	return *mFirst;
}

template <typename T>
T& Range<T>::back() const
{
	PX_ASSERT(mFirst < mLast);
	return mLast[-1];
}

template <typename T>
T& Range<T>::operator[](uint32_t i) const
{
	PX_ASSERT(mFirst + i < mLast);
	return mFirst[i];
}

} // namespace cloth
} // namespace physx
