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

#include "foundation/PxMat44.h"
#include "foundation/PxTransform.h"
#include "foundation/PxBounds3.h"

namespace physx
{
namespace cloth
{

class SwCloth;
struct SwClothData;

typedef StackAllocator<16> SwKernelAllocator;

typedef bool (*InterCollisionFilter)(void* cloth0, void* cloth1);

struct SwInterCollisionData
{
	SwInterCollisionData()
	{
	}
	SwInterCollisionData(PxVec4* particles, PxVec4* prevParticles, uint32_t numParticles, uint32_t* indices,
	                     const PxTransform& globalPose, const PxVec3& boundsCenter, const PxVec3& boundsHalfExtents,
	                     float impulseScale, void* userData)
	: mParticles(particles)
	, mPrevParticles(prevParticles)
	, mNumParticles(numParticles)
	, mIndices(indices)
	, mGlobalPose(globalPose)
	, mBoundsCenter(boundsCenter)
	, mBoundsHalfExtent(boundsHalfExtents)
	, mImpulseScale(impulseScale)
	, mUserData(userData)
	{
	}

	PxVec4* mParticles;
	PxVec4* mPrevParticles;
	uint32_t mNumParticles;
	uint32_t* mIndices;
	PxTransform mGlobalPose;
	PxVec3 mBoundsCenter;
	PxVec3 mBoundsHalfExtent;
	float mImpulseScale;
	void* mUserData;
};

template <typename Simd4f>
class SwInterCollision
{

  public:
	SwInterCollision(const SwInterCollisionData* cloths, uint32_t n, float colDist, float stiffness,
	                 uint32_t iterations, InterCollisionFilter filter, cloth::SwKernelAllocator& alloc);

	~SwInterCollision();

	void operator()();

	static size_t estimateTemporaryMemory(SwInterCollisionData* cloths, uint32_t n);

  private:
	SwInterCollision& operator=(const SwInterCollision&); // not implemented

	static size_t getBufferSize(uint32_t);

	void collideParticles(const uint32_t* keys, uint32_t firstColumnSize, const uint32_t* sortedIndices,
	                      uint32_t numParticles, uint32_t collisionDistance);

	Simd4f& getParticle(uint32_t index);

	// better wrap these in a struct
	void collideParticle(uint32_t index);

	Simd4f mParticle;
	Simd4f mImpulse;

	Simd4f mCollisionDistance;
	Simd4f mCollisionSquareDistance;
	Simd4f mStiffness;

	uint16_t mClothIndex;
	uint32_t mClothMask;
	uint32_t mParticleIndex;

	uint32_t mNumIterations;

	const SwInterCollisionData* mInstances;
	uint32_t mNumInstances;

	uint16_t* mClothIndices;
	uint32_t* mParticleIndices;
	uint32_t mNumParticles;
	uint32_t* mOverlapMasks;

	uint32_t mTotalParticles;

	InterCollisionFilter mFilter;

	SwKernelAllocator& mAllocator;

  public:
	mutable uint32_t mNumTests;
	mutable uint32_t mNumCollisions;
};

#if PX_SUPPORT_EXTERN_TEMPLATE
//explicit template instantiation declaration
#if NV_SIMD_SIMD
extern template class SwInterCollision<Simd4f>;
#endif
#if NV_SIMD_SCALAR
extern template class SwInterCollision<Scalar4f>;
#endif
#endif

} // namespace cloth

} // namespace physx
