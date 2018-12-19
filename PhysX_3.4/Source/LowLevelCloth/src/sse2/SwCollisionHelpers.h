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

#if PX_GCC_FAMILY
#if PX_EMSCRIPTEN
#include <emmintrin.h>
#endif
#include <xmmintrin.h> // _BitScanForward
#else
#pragma warning(push)
#pragma warning(disable : 4668) //'symbol'  is not defined as a preprocessor macro, replacing with '0' for 'directives'
#pragma warning(disable : 4987) // nonstandard extension used: 'throw (...)'
#include <intrin.h>             // _BitScanForward
#pragma warning(pop)
#endif

namespace physx
{
namespace cloth
{

uint32_t findBitSet(uint32_t mask)
{
#if defined(_MSC_VER)
	unsigned long result;
	_BitScanForward(&result, unsigned long(mask));
	return result;
#else
	return __builtin_ffs(mask) - 1;
#endif
}

Simd4i intFloor(const Simd4f& v)
{
	Simd4i i = _mm_cvttps_epi32(v);
	return _mm_sub_epi32(i, _mm_srli_epi32(simd4i(v), 31));
}

Simd4i horizontalOr(const Simd4i& mask)
{
	Simd4i tmp = mask | _mm_shuffle_epi32(mask, 0xb1); // w z y x -> z w x y
	return tmp | _mm_shuffle_epi32(tmp, 0x4e);         // w z y x -> y x w z
}

Gather<Simd4i>::Gather(const Simd4i& index)
{
	mSelectQ = _mm_srai_epi32(index << 29, 31);
	mSelectD = _mm_srai_epi32(index << 30, 31);
	mSelectW = _mm_srai_epi32(index << 31, 31);
	mOutOfRange = (index ^ sIntSignBit) > sSignedMask;
}

Simd4i Gather<Simd4i>::operator()(const Simd4i* ptr) const
{
	// more efficient with _mm_shuffle_epi8 (SSSE3)
	Simd4i lo = ptr[0], hi = ptr[1];
	Simd4i m01 = select(mSelectW, splat<1>(lo), splat<0>(lo));
	Simd4i m23 = select(mSelectW, splat<3>(lo), splat<2>(lo));
	Simd4i m45 = select(mSelectW, splat<1>(hi), splat<0>(hi));
	Simd4i m67 = select(mSelectW, splat<3>(hi), splat<2>(hi));
	Simd4i m0123 = select(mSelectD, m23, m01);
	Simd4i m4567 = select(mSelectD, m67, m45);
	return select(mSelectQ, m4567, m0123) & ~mOutOfRange;
}

} // namespace cloth
} // namespace physx
