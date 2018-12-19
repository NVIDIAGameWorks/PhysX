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

namespace physx
{
namespace cloth
{

#if !NV_SIMD_SIMD
uint32_t findBitSet(uint32_t mask)
{
	uint32_t result = 0;
	while(mask >>= 1)
		++result;
	return result;
}
#endif

inline Scalar4i intFloor(const Scalar4f& v)
{
	return Scalar4i(int(floor(v.f4[0])), int(floor(v.f4[1])), int(floor(v.f4[2])), int(floor(v.f4[3])));
}

inline Scalar4i horizontalOr(const Scalar4i& mask)
{
	return simd4i(mask.i4[0] | mask.i4[1] | mask.i4[2] | mask.i4[3]);
}

template <>
struct Gather<Scalar4i>
{
	inline Gather(const Scalar4i& index);
	inline Scalar4i operator()(const Scalar4i*) const;

	Scalar4i mIndex;
	Scalar4i mOutOfRange;
};

Gather<Scalar4i>::Gather(const Scalar4i& index)
{
	uint32_t mask = /* sGridSize */ 8 - 1;

	mIndex.u4[0] = index.u4[0] & mask;
	mIndex.u4[1] = index.u4[1] & mask;
	mIndex.u4[2] = index.u4[2] & mask;
	mIndex.u4[3] = index.u4[3] & mask;

	mOutOfRange.i4[0] = index.u4[0] & ~mask ? 0 : -1;
	mOutOfRange.i4[1] = index.u4[1] & ~mask ? 0 : -1;
	mOutOfRange.i4[2] = index.u4[2] & ~mask ? 0 : -1;
	mOutOfRange.i4[3] = index.u4[3] & ~mask ? 0 : -1;
}

Scalar4i Gather<Scalar4i>::operator()(const Scalar4i* ptr) const
{
	const int32_t* base = ptr->i4;
	const int32_t* index = mIndex.i4;
	const int32_t* mask = mOutOfRange.i4;
	return Scalar4i(base[index[0]] & mask[0], base[index[1]] & mask[1], base[index[2]] & mask[2],
	                base[index[3]] & mask[3]);
}

} // namespace cloth
} // namespace physx
