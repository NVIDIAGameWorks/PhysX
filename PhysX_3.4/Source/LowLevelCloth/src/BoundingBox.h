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

#include "Simd.h"

namespace physx
{

namespace cloth
{

template <typename Simd4f>
struct BoundingBox
{
	Simd4f mLower;
	Simd4f mUpper;
};

template <typename Simd4f>
inline BoundingBox<Simd4f> loadBounds(const float* ptr)
{
	BoundingBox<Simd4f> result;
	result.mLower = load(ptr);
	result.mUpper = load(ptr + 3);
	return result;
}

template <typename Simd4f>
inline BoundingBox<Simd4f> emptyBounds()
{
	BoundingBox<Simd4f> result;

	result.mLower = gSimd4fFloatMax;
	result.mUpper = -result.mLower;

	return result;
}

template <typename Simd4f>
inline BoundingBox<Simd4f> expandBounds(const BoundingBox<Simd4f>& bounds, const Simd4f* pIt, const Simd4f* pEnd)
{
	BoundingBox<Simd4f> result = bounds;
	for(; pIt != pEnd; ++pIt)
	{
		result.mLower = min(result.mLower, *pIt);
		result.mUpper = max(result.mUpper, *pIt);
	}
	return result;
}

template <typename Simd4f>
inline BoundingBox<Simd4f> expandBounds(const BoundingBox<Simd4f>& a, const BoundingBox<Simd4f>& b)
{
	BoundingBox<Simd4f> result;
	result.mLower = min(a.mLower, b.mLower);
	result.mUpper = max(a.mUpper, b.mUpper);
	return result;
}

template <typename Simd4f>
inline BoundingBox<Simd4f> intersectBounds(const BoundingBox<Simd4f>& a, const BoundingBox<Simd4f>& b)
{
	BoundingBox<Simd4f> result;
	result.mLower = max(a.mLower, b.mLower);
	result.mUpper = min(a.mUpper, b.mUpper);
	return result;
}

template <typename Simd4f>
inline bool isEmptyBounds(const BoundingBox<Simd4f>& a)
{
	return anyGreater(a.mLower, a.mUpper) != 0;
}
}
}
