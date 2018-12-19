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

#include "Types.h"

namespace physx
{

namespace cloth
{

// acts as a poor mans random access iterator
template <typename Simd4f, typename BaseIterator>
class LerpIterator
{

	LerpIterator& operator=(const LerpIterator&); // not implemented

  public:
	LerpIterator(BaseIterator start, BaseIterator target, float alpha)
	: mAlpha(simd4f(alpha)), mStart(start), mTarget(target)
	{
	}

	// return the interpolated point at a given index
	inline Simd4f operator[](size_t index) const
	{
		return mStart[index] + (mTarget[index] - mStart[index]) * mAlpha;
	}

	inline Simd4f operator*() const
	{
		return (*this)[0];
	}

	// prefix increment only
	inline LerpIterator& operator++()
	{
		++mStart;
		++mTarget;
		return *this;
	}

  private:
	// interpolation parameter
	const Simd4f mAlpha;

	BaseIterator mStart;
	BaseIterator mTarget;
};

template <typename Simd4f, size_t Stride>
class UnalignedIterator
{

	UnalignedIterator& operator=(const UnalignedIterator&); // not implemented

  public:
	UnalignedIterator(const float* pointer) : mPointer(pointer)
	{
	}

	inline Simd4f operator[](size_t index) const
	{
		return load(mPointer + index * Stride);
	}

	inline Simd4f operator*() const
	{
		return (*this)[0];
	}

	// prefix increment only
	inline UnalignedIterator& operator++()
	{
		mPointer += Stride;
		return *this;
	}

  private:
	const float* mPointer;
};

// acts as an iterator but returns a constant
template <typename Simd4f>
class ConstantIterator
{
  public:
	ConstantIterator(const Simd4f& value) : mValue(value)
	{
	}

	inline Simd4f operator*() const
	{
		return mValue;
	}

	inline ConstantIterator& operator++()
	{
		return *this;
	}

  private:
	ConstantIterator& operator=(const ConstantIterator&);
	const Simd4f mValue;
};

// wraps an iterator with constant scale and bias
template <typename Simd4f, typename BaseIterator>
class ScaleBiasIterator
{
  public:
	ScaleBiasIterator(BaseIterator base, const Simd4f& scale, const Simd4f& bias)
	: mScale(scale), mBias(bias), mBaseIterator(base)
	{
	}

	inline Simd4f operator*() const
	{
		return (*mBaseIterator) * mScale + mBias;
	}

	inline ScaleBiasIterator& operator++()
	{
		++mBaseIterator;
		return *this;
	}

  private:
	ScaleBiasIterator& operator=(const ScaleBiasIterator&);

	const Simd4f mScale;
	const Simd4f mBias;

	BaseIterator mBaseIterator;
};

} // namespace cloth

} // namespace physx
