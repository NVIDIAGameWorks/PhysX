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

// platform specific helpers

namespace physx
{
namespace cloth
{

inline uint32_t findBitSet(uint32_t mask);

// intFloor(-1.0f) returns -2 on SSE and NEON!
inline Simd4i intFloor(const Simd4f& v);

inline Simd4i horizontalOr(const Simd4i& mask);

template <typename>
struct Gather;

#if NV_SIMD_SIMD
template <>
struct Gather<Simd4i>
{
	inline Gather(const Simd4i& index);
	inline Simd4i operator()(const Simd4i*) const;

#if NV_SIMD_SSE2
	Simd4i mSelectQ, mSelectD, mSelectW;
	static const Simd4i sIntSignBit;
	static const Simd4i sSignedMask;
#elif NV_SIMD_NEON
	Simd4i mPermute;
	static const Simd4i sPack;
	static const Simd4i sOffset;
	static const Simd4i sShift;
	static const Simd4i sMask;
#endif
	Simd4i mOutOfRange;
};
#endif

} // namespace cloth
} // namespace physx

#if NV_SIMD_SSE2
#include "sse2/SwCollisionHelpers.h"
#elif NV_SIMD_NEON
#include "neon/SwCollisionHelpers.h"
#endif

#if NV_SIMD_SCALAR
#include "scalar/SwCollisionHelpers.h"
#endif
