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

#ifdef _M_ARM
#include <arm_neon.h>
#endif

namespace physx
{
namespace cloth
{

uint32_t findBitSet(uint32_t mask)
{
#ifdef _M_ARM
	__n64 t = { mask };
	return 31 - (vclz_u32(t)).n64_u32[0];
#else
	return 31 - __builtin_clz(mask);
#endif
}

Simd4i intFloor(const Simd4f& v)
{
	int32x4_t neg = vreinterpretq_s32_u32(vshrq_n_u32(v.u4, 31));
	return vsubq_s32(vcvtq_s32_f32(v.f4), neg);
}

Simd4i horizontalOr(const Simd4i& mask)
{
	uint32x2_t hi = vget_high_u32(mask.u4);
	uint32x2_t lo = vget_low_u32(mask.u4);
	uint32x2_t tmp = vorr_u32(lo, hi);
	uint32x2_t rev = vrev64_u32(tmp);
	uint32x2_t res = vorr_u32(tmp, rev);
	return vcombine_u32(res, res);
}

Gather<Simd4i>::Gather(const Simd4i& index)
{
	PX_ALIGN(16, uint8x8x2_t) byteIndex = reinterpret_cast<const uint8x8x2_t&>(sPack);
	uint8x8x2_t lohiIndex = reinterpret_cast<const uint8x8x2_t&>(index);
	byteIndex.val[0] = vtbl2_u8(lohiIndex, byteIndex.val[0]);
	byteIndex.val[1] = vtbl2_u8(lohiIndex, byteIndex.val[1]);
	mPermute = vshlq_n_u32(reinterpret_cast<const uint32x4_t&>(byteIndex), 2);
	mPermute = mPermute | sOffset | vcgtq_u32(index.u4, sMask.u4);
}

Simd4i Gather<Simd4i>::operator()(const Simd4i* ptr) const
{
	PX_ALIGN(16, uint8x8x2_t) result = reinterpret_cast<const uint8x8x2_t&>(mPermute);
	const uint8x8x4_t* table = reinterpret_cast<const uint8x8x4_t*>(ptr);
	result.val[0] = vtbl4_u8(*table, result.val[0]);
	result.val[1] = vtbl4_u8(*table, result.val[1]);
	return reinterpret_cast<const Simd4i&>(result);
}

} // namespace cloth
} // namespace physx
