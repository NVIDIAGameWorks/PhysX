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
template <typename>
struct IterationState;
struct IndexPair;
struct SphereData;
struct ConeData;
struct TriangleData;

typedef StackAllocator<16> SwKernelAllocator;

/**
   Collision handler for SwSolver.
 */
template <typename Simd4f>
class SwCollision
{
	typedef typename Simd4fToSimd4i<Simd4f>::Type Simd4i;

  public:
	struct ShapeMask
	{
		Simd4i mCones;
		Simd4i mSpheres;

		ShapeMask& operator=(const ShapeMask&);
		ShapeMask& operator&=(const ShapeMask&);
	};

	struct CollisionData
	{
		CollisionData();
		SphereData* mSpheres;
		ConeData* mCones;
	};

	struct ImpulseAccumulator;

  public:
	SwCollision(SwClothData& clothData, SwKernelAllocator& alloc);
	~SwCollision();

	void operator()(const IterationState<Simd4f>& state);

	static size_t estimateTemporaryMemory(const SwCloth& cloth);
	static size_t estimatePersistentMemory(const SwCloth& cloth);

  private:
	SwCollision& operator=(const SwCollision&); // not implemented
	void allocate(CollisionData&);
	void deallocate(const CollisionData&);

	void computeBounds();

	void buildSphereAcceleration(const SphereData*);
	void buildConeAcceleration();
	static void mergeAcceleration(uint32_t*);
	bool buildAcceleration();

	static ShapeMask getShapeMask(const Simd4f&, const Simd4i*, const Simd4i*);
	ShapeMask getShapeMask(const Simd4f*) const;
	ShapeMask getShapeMask(const Simd4f*, const Simd4f*) const;

	void collideSpheres(const Simd4i&, const Simd4f*, ImpulseAccumulator&) const;
	Simd4i collideCones(const Simd4f*, ImpulseAccumulator&) const;

	void collideSpheres(const Simd4i&, const Simd4f*, Simd4f*, ImpulseAccumulator&) const;
	Simd4i collideCones(const Simd4f*, Simd4f*, ImpulseAccumulator&) const;

	void collideParticles();
	void collideVirtualParticles();
	void collideContinuousParticles();

	void collideConvexes(const IterationState<Simd4f>&);
	void collideConvexes(const Simd4f*, Simd4f*, ImpulseAccumulator&);

	void collideTriangles(const IterationState<Simd4f>&);
	void collideTriangles(const TriangleData*, Simd4f*, ImpulseAccumulator&);

  public:
	// acceleration structure
	static const uint32_t sGridSize = 8;
	Simd4i mSphereGrid[6 * sGridSize / 4];
	Simd4i mConeGrid[6 * sGridSize / 4];
	Simd4f mGridScale, mGridBias;

	CollisionData mPrevData;
	CollisionData mCurData;

	SwClothData& mClothData;
	SwKernelAllocator& mAllocator;

	uint32_t mNumCollisions;

	static const Simd4f sSkeletonWidth;
};

#if PX_SUPPORT_EXTERN_TEMPLATE
//explicit template instantiation declaration
#if NV_SIMD_SIMD
extern template class SwCollision<Simd4f>;
#endif
#if NV_SIMD_SCALAR
extern template class SwCollision<Scalar4f>;
#endif
#endif

}
}
