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
#include "StackAllocator.h"
#include "Simd.h"

namespace physx
{
namespace cloth
{

class SwCloth;
struct SwClothData;

typedef StackAllocator<16> SwKernelAllocator;

template <typename Simd4f>
class SwSelfCollision
{
	typedef typename Simd4fToSimd4i<Simd4f>::Type Simd4i;

  public:
	SwSelfCollision(SwClothData& clothData, SwKernelAllocator& alloc);
	~SwSelfCollision();

	void operator()();

	static size_t estimateTemporaryMemory(const SwCloth&);

  private:
	SwSelfCollision& operator=(const SwSelfCollision&); // not implemented
	static size_t getBufferSize(uint32_t);

	template <bool useRestParticles>
	void collideParticles(Simd4f&, Simd4f&, const Simd4f&, const Simd4f&);

	template <bool useRestParticles>
	void collideParticles(const uint32_t*, uint16_t, const uint16_t*, uint32_t);

	Simd4f mCollisionDistance;
	Simd4f mCollisionSquareDistance;
	Simd4f mStiffness;

	SwClothData& mClothData;
	SwKernelAllocator& mAllocator;

  public:
	mutable uint32_t mNumTests;
	mutable uint32_t mNumCollisions;
};

#if PX_SUPPORT_EXTERN_TEMPLATE
//explicit template instantiation declaration
#if NV_SIMD_SIMD
extern template class SwSelfCollision<Simd4f>;
#endif
#if NV_SIMD_SCALAR
extern template class SwSelfCollision<Scalar4f>;
#endif
#endif

} // namespace cloth

} // namespace physx
