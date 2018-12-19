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

#include "foundation/PxProfiler.h"
#include "foundation/PxAssert.h"
#include "SwCollision.h"
#include "SwCloth.h"
#include "SwClothData.h"
#include "IterationState.h"
#include "BoundingBox.h"
#include "PointInterpolator.h"
#include "SwCollisionHelpers.h"
#include <cstring> // for memset

using namespace physx;

// the particle trajectory needs to penetrate more than 0.2 * radius to trigger continuous collision
template <typename Simd4f>
const Simd4f cloth::SwCollision<Simd4f>::sSkeletonWidth = simd4f(cloth::sqr(1 - 0.2f) - 1);

#if NV_SIMD_SSE2
const Simd4i cloth::Gather<Simd4i>::sIntSignBit = simd4i(0x80000000);
const Simd4i cloth::Gather<Simd4i>::sSignedMask = sIntSignBit | simd4i(0x7);
#elif NV_SIMD_NEON
const Simd4i cloth::Gather<Simd4i>::sPack = simd4i(0x00000000, 0x04040404, 0x08080808, 0x0c0c0c0c);
const Simd4i cloth::Gather<Simd4i>::sOffset = simd4i(0x03020100);
const Simd4i cloth::Gather<Simd4i>::sShift = simd4i(2);
const Simd4i cloth::Gather<Simd4i>::sMask = simd4i(7);
#endif

namespace
{
const Simd4fTupleFactory sMaskX = simd4f(simd4i(~0, 0, 0, 0));
const Simd4fTupleFactory sMaskZ = simd4f(simd4i(0, 0, ~0, 0));
const Simd4fTupleFactory sMaskW = simd4f(simd4i(0, 0, 0, ~0));
const Simd4fTupleFactory gSimd4fOneXYZ = simd4f(1.0f, 1.0f, 1.0f, 0.0f);
const Simd4fScalarFactory sGridLength = simd4f(8 - 1e-3f); // sGridSize
const Simd4fScalarFactory sGridExpand = simd4f(1e-4f);
const Simd4fTupleFactory sMinusFloatMaxXYZ = simd4f(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f);

#if PX_PROFILE || PX_DEBUG
template <typename Simd4f>
uint32_t horizontalSum(const Simd4f& x)
{
	const float* p = array(x);
	return uint32_t(0.5f + p[0] + p[1] + p[2] + p[3]);
}
#endif

// 7 elements are written to ptr!
template <typename Simd4f>
void storeBounds(float* ptr, const cloth::BoundingBox<Simd4f>& bounds)
{
	store(ptr, bounds.mLower);
	store(ptr + 3, bounds.mUpper);
}
}

struct cloth::SphereData
{
	PxVec3 center;
	float radius;
};

struct cloth::ConeData
{
	PxVec3 center;
	float radius; // cone radius at center
	PxVec3 axis;
	float slope; // tan(alpha)

	float sqrCosine; // cos^2(alpha)
	float halfLength;

	uint32_t firstMask;
	uint32_t bothMask;
};

struct cloth::TriangleData
{
	PxVec3 base;
	float edge0DotEdge1;

	PxVec3 edge0;
	float edge0SqrLength;

	PxVec3 edge1;
	float edge1SqrLength;

	PxVec3 normal;
	float padding;

	float det;
	float denom;

	float edge0InvSqrLength;
	float edge1InvSqrLength;
};

namespace physx
{
namespace cloth
{
template <typename Simd4f>
BoundingBox<Simd4f> expandBounds(const BoundingBox<Simd4f>& bbox, const SphereData* sIt, const SphereData* sEnd)
{
	BoundingBox<Simd4f> result = bbox;
	for(; sIt != sEnd; ++sIt)
	{
		Simd4f p = loadAligned(array(sIt->center));
		Simd4f r = splat<3>(p);
		result.mLower = min(result.mLower, p - r);
		result.mUpper = max(result.mUpper, p + r);
	}
	return result;
}
}
}

namespace
{
template <typename Simd4f, typename SrcIterator>
void generateSpheres(Simd4f* dIt, const SrcIterator& src, uint32_t count)
{
	// have to copy out iterator to ensure alignment is maintained
	for(SrcIterator sIt = src; 0 < count--; ++sIt, ++dIt)
		*dIt = max(sMinusFloatMaxXYZ, *sIt); // clamp radius to 0
}

void generateCones(cloth::ConeData* dst, const cloth::SphereData* sourceSpheres, const cloth::IndexPair* capsuleIndices,
                   uint32_t numCones)
{
	cloth::ConeData* cIt = dst;
	for(const cloth::IndexPair* iIt = capsuleIndices, *iEnd = iIt + numCones; iIt != iEnd; ++iIt, ++cIt)
	{
		PxVec4 first = reinterpret_cast<const PxVec4&>(sourceSpheres[iIt->first]);
		PxVec4 second = reinterpret_cast<const PxVec4&>(sourceSpheres[iIt->second]);

		PxVec4 center = (second + first) * 0.5f;
		PxVec4 axis = (second - first) * 0.5f;

		float sqrAxisLength = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
		float sqrConeLength = sqrAxisLength - cloth::sqr(axis.w);

		float invAxisLength = 1 / sqrtf(sqrAxisLength);
		float invConeLength = 1 / sqrtf(sqrConeLength);

		if(sqrConeLength <= 0.0f)
			invAxisLength = invConeLength = 0.0f;

		float axisLength = sqrAxisLength * invAxisLength;
		float slope = axis.w * invConeLength;

		cIt->center = PxVec3(center.x, center.y, center.z);
		cIt->radius = (axis.w + first.w) * invConeLength * axisLength;
		cIt->axis = PxVec3(axis.x, axis.y, axis.z) * invAxisLength;
		cIt->slope = slope;

		cIt->sqrCosine = 1.0f - cloth::sqr(axis.w * invAxisLength);
		cIt->halfLength = axisLength;

		uint32_t firstMask = 0x1u << iIt->first;
		cIt->firstMask = firstMask;
		cIt->bothMask = firstMask | 0x1u << iIt->second;
	}
}

template <typename Simd4f, typename SrcIterator>
void generatePlanes(Simd4f* dIt, const SrcIterator& src, uint32_t count)
{
	// have to copy out iterator to ensure alignment is maintained
	for(SrcIterator sIt = src; 0 < count--; ++sIt, ++dIt)
		*dIt = *sIt;
}

template <typename Simd4f, typename SrcIterator>
void generateTriangles(cloth::TriangleData* dIt, const SrcIterator& src, uint32_t count)
{
	// have to copy out iterator to ensure alignment is maintained
	for(SrcIterator sIt = src; 0 < count--; ++dIt)
	{
		Simd4f p0 = *sIt;
		++sIt;
		Simd4f p1 = *sIt;
		++sIt;
		Simd4f p2 = *sIt;
		++sIt;

		Simd4f edge0 = p1 - p0;
		Simd4f edge1 = p2 - p0;
		Simd4f normal = cross3(edge0, edge1);

		Simd4f edge0SqrLength = dot3(edge0, edge0);
		Simd4f edge1SqrLength = dot3(edge1, edge1);
		Simd4f edge0DotEdge1 = dot3(edge0, edge1);
		Simd4f normalInvLength = rsqrt(dot3(normal, normal));

		Simd4f det = edge0SqrLength * edge1SqrLength - edge0DotEdge1 * edge0DotEdge1;
		Simd4f denom = edge0SqrLength + edge1SqrLength - edge0DotEdge1 - edge0DotEdge1;

		// there are definitely faster ways...
		Simd4f aux = select(sMaskX, det, denom);
		aux = select(sMaskZ, edge0SqrLength, aux);
		aux = select(sMaskW, edge1SqrLength, aux);

		storeAligned(&dIt->base.x, select(sMaskW, edge0DotEdge1, p0));
		storeAligned(&dIt->edge0.x, select(sMaskW, edge0SqrLength, edge0));
		storeAligned(&dIt->edge1.x, select(sMaskW, edge1SqrLength, edge1));
		storeAligned(&dIt->normal.x, normal * normalInvLength);
		storeAligned(&dIt->det, recip<1>(aux));
	}
}

} // namespace

template <typename Simd4f>
cloth::SwCollision<Simd4f>::CollisionData::CollisionData()
: mSpheres(0), mCones(0)
{
}

template <typename Simd4f>
cloth::SwCollision<Simd4f>::SwCollision(SwClothData& clothData, SwKernelAllocator& alloc)
: mClothData(clothData), mAllocator(alloc)
{
	allocate(mCurData);

	if(mClothData.mEnableContinuousCollision || mClothData.mFrictionScale > 0.0f)
	{
		allocate(mPrevData);

		generateSpheres(reinterpret_cast<Simd4f*>(mPrevData.mSpheres),
		                reinterpret_cast<const Simd4f*>(clothData.mStartCollisionSpheres), clothData.mNumSpheres);

		generateCones(mPrevData.mCones, mPrevData.mSpheres, clothData.mCapsuleIndices, clothData.mNumCapsules);
	}
}

template <typename Simd4f>
cloth::SwCollision<Simd4f>::~SwCollision()
{
	deallocate(mCurData);
	deallocate(mPrevData);
}

template <typename Simd4f>
void cloth::SwCollision<Simd4f>::operator()(const IterationState<Simd4f>& state)
{
	mNumCollisions = 0;

	collideConvexes(state);  // discrete convex collision, no friction
	collideTriangles(state); // discrete triangle collision, no friction

	computeBounds();

	if(!mClothData.mNumSpheres)
		return;

	bool lastIteration = state.mRemainingIterations == 1;

	const Simd4f* targetSpheres = reinterpret_cast<const Simd4f*>(mClothData.mTargetCollisionSpheres);

	// generate sphere and cone collision data
	if(!lastIteration)
	{
		// interpolate spheres
		LerpIterator<Simd4f, const Simd4f*> pIter(reinterpret_cast<const Simd4f*>(mClothData.mStartCollisionSpheres),
		                                          targetSpheres, state.getCurrentAlpha());
		generateSpheres(reinterpret_cast<Simd4f*>(mCurData.mSpheres), pIter, mClothData.mNumSpheres);
	}
	else
	{
		// otherwise use the target spheres directly
		generateSpheres(reinterpret_cast<Simd4f*>(mCurData.mSpheres), targetSpheres, mClothData.mNumSpheres);
	}

	// generate cones even if test below fails because
	// continuous collision might need it in next iteration
	generateCones(mCurData.mCones, mCurData.mSpheres, mClothData.mCapsuleIndices, mClothData.mNumCapsules);

	if(buildAcceleration())
	{
		if(mClothData.mEnableContinuousCollision)
			collideContinuousParticles();

		mergeAcceleration(reinterpret_cast<uint32_t*>(mSphereGrid));
		mergeAcceleration(reinterpret_cast<uint32_t*>(mConeGrid));

		if(!mClothData.mEnableContinuousCollision)
			collideParticles();

		collideVirtualParticles();
	}

	if(mPrevData.mSpheres)
		shdfnd::swap(mCurData, mPrevData);
}

template <typename Simd4f>
size_t cloth::SwCollision<Simd4f>::estimateTemporaryMemory(const SwCloth& cloth)
{
	size_t numTriangles = cloth.mStartCollisionTriangles.size();
	size_t numPlanes = cloth.mStartCollisionPlanes.size();

	const size_t kTriangleDataSize = sizeof(TriangleData) * numTriangles;
	const size_t kPlaneDataSize = sizeof(PxVec4) * numPlanes * 2;

	return PxMax(kTriangleDataSize, kPlaneDataSize);
}

template <typename Simd4f>
size_t cloth::SwCollision<Simd4f>::estimatePersistentMemory(const SwCloth& cloth)
{
	size_t numCapsules = cloth.mCapsuleIndices.size();
	size_t numSpheres = cloth.mStartCollisionSpheres.size();

	size_t sphereDataSize = sizeof(SphereData) * numSpheres * 2;
	size_t coneDataSize = sizeof(ConeData) * numCapsules * 2;

	return sphereDataSize + coneDataSize;
}

template <typename Simd4f>
void cloth::SwCollision<Simd4f>::allocate(CollisionData& data)
{
	data.mSpheres = static_cast<SphereData*>(mAllocator.allocate(sizeof(SphereData) * mClothData.mNumSpheres));

	data.mCones = static_cast<ConeData*>(mAllocator.allocate(sizeof(ConeData) * mClothData.mNumCapsules));
}

template <typename Simd4f>
void cloth::SwCollision<Simd4f>::deallocate(const CollisionData& data)
{
	mAllocator.deallocate(data.mSpheres);
	mAllocator.deallocate(data.mCones);
}

template <typename Simd4f>
void cloth::SwCollision<Simd4f>::computeBounds()
{
	PX_PROFILE_ZONE("cloth::SwSolverKernel::computeBounds", 0);

	Simd4f* prevIt = reinterpret_cast<Simd4f*>(mClothData.mPrevParticles);
	Simd4f* curIt = reinterpret_cast<Simd4f*>(mClothData.mCurParticles);
	Simd4f* curEnd = curIt + mClothData.mNumParticles;
	Simd4f floatMaxXYZ = -static_cast<Simd4f>(sMinusFloatMaxXYZ);

	Simd4f lower = simd4f(FLT_MAX), upper = -lower;
	for(; curIt < curEnd; ++curIt, ++prevIt)
	{
		Simd4f current = *curIt;
		lower = min(lower, current);
		upper = max(upper, current);
		// if(current.w > 0) current.w = previous.w
		*curIt = select(current > floatMaxXYZ, *prevIt, current);
	}

	BoundingBox<Simd4f> curBounds;
	curBounds.mLower = lower;
	curBounds.mUpper = upper;

	// don't change this order, storeBounds writes 7 floats
	BoundingBox<Simd4f> prevBounds = loadBounds<Simd4f>(mClothData.mCurBounds);
	storeBounds(mClothData.mCurBounds, curBounds);
	storeBounds(mClothData.mPrevBounds, prevBounds);
}

namespace
{
template <typename Simd4i>
Simd4i andNotIsZero(const Simd4i& left, const Simd4i& right)
{
	return (left & ~right) == gSimd4iZero;
}
}

// build per-axis mask arrays of spheres on the right/left of grid cell
template <typename Simd4f>
void cloth::SwCollision<Simd4f>::buildSphereAcceleration(const SphereData* sIt)
{
	static const int maxIndex = sGridSize - 1;

	const SphereData* sEnd = sIt + mClothData.mNumSpheres;
	for(uint32_t mask = 0x1; sIt != sEnd; ++sIt, mask <<= 1)
	{
		Simd4f sphere = loadAligned(array(sIt->center));
		Simd4f radius = splat<3>(sphere);

		Simd4i first = intFloor(max((sphere - radius) * mGridScale + mGridBias, gSimd4fZero));
		Simd4i last = intFloor(min((sphere + radius) * mGridScale + mGridBias, sGridLength));

		const int* firstIdx = array(first);
		const int* lastIdx = array(last);

		uint32_t* firstIt = reinterpret_cast<uint32_t*>(mSphereGrid);
		uint32_t* lastIt = firstIt + 3 * sGridSize;

		for(uint32_t i = 0; i < 3; ++i, firstIt += sGridSize, lastIt += sGridSize)
		{
			for(int j = firstIdx[i]; j <= maxIndex; ++j)
				firstIt[j] |= mask;

			for(int j = lastIdx[i]; j >= 0; --j)
				lastIt[j] |= mask;
		}
	}
}

// generate cone masks from sphere masks
template <typename Simd4f>
void cloth::SwCollision<Simd4f>::buildConeAcceleration()
{
	const ConeData* coneIt = mCurData.mCones;
	const ConeData* coneEnd = coneIt + mClothData.mNumCapsules;
	for(uint32_t coneMask = 0x1; coneIt != coneEnd; ++coneIt, coneMask <<= 1)
	{
		if(coneIt->radius == 0.0f)
			continue;

		uint32_t spheresMask = coneIt->bothMask;

		uint32_t* sphereIt = reinterpret_cast<uint32_t*>(mSphereGrid);
		uint32_t* sphereEnd = sphereIt + 6 * sGridSize;
		uint32_t* gridIt = reinterpret_cast<uint32_t*>(mConeGrid);
		for(; sphereIt != sphereEnd; ++sphereIt, ++gridIt)
			if(*sphereIt & spheresMask)
				*gridIt |= coneMask;
	}
}

// convert right/left mask arrays into single overlap array
template <typename Simd4f>
void cloth::SwCollision<Simd4f>::mergeAcceleration(uint32_t* firstIt)
{
	uint32_t* firstEnd = firstIt + 3 * sGridSize;
	uint32_t* lastIt = firstEnd;
	for(; firstIt != firstEnd; ++firstIt, ++lastIt)
		*firstIt &= *lastIt;
}

// build mask of spheres/cones touching a regular grid along each axis
template <typename Simd4f>
bool cloth::SwCollision<Simd4f>::buildAcceleration()
{
	// determine sphere bbox
	BoundingBox<Simd4f> sphereBounds =
	    expandBounds(emptyBounds<Simd4f>(), mCurData.mSpheres, mCurData.mSpheres + mClothData.mNumSpheres);
	BoundingBox<Simd4f> particleBounds = loadBounds<Simd4f>(mClothData.mCurBounds);
	if(mClothData.mEnableContinuousCollision)
	{
		sphereBounds = expandBounds(sphereBounds, mPrevData.mSpheres, mPrevData.mSpheres + mClothData.mNumSpheres);
		particleBounds = expandBounds(particleBounds, loadBounds<Simd4f>(mClothData.mPrevBounds));
	}

	BoundingBox<Simd4f> bounds = intersectBounds(sphereBounds, particleBounds);
	Simd4f edgeLength = (bounds.mUpper - bounds.mLower) & ~static_cast<Simd4f>(sMaskW);
	if(!allGreaterEqual(edgeLength, gSimd4fZero))
		return false;

	// calculate an expanded bounds to account for numerical inaccuracy
	const Simd4f expandedLower = bounds.mLower - abs(bounds.mLower) * sGridExpand;
	const Simd4f expandedUpper = bounds.mUpper + abs(bounds.mUpper) * sGridExpand;
	const Simd4f expandedEdgeLength = max(expandedUpper - expandedLower, gSimd4fEpsilon);

	// make grid minimal thickness and strict upper bound of spheres
	mGridScale = sGridLength * recip<1>(expandedEdgeLength);
	mGridBias = -expandedLower * mGridScale;
	array(mGridBias)[3] = 1.0f; // needed for collideVirtualParticles()

	PX_ASSERT(allTrue(((bounds.mLower * mGridScale + mGridBias) >= simd4f(0.0f)) | sMaskW));
	PX_ASSERT(allTrue(((bounds.mUpper * mGridScale + mGridBias) < simd4f(8.0f)) | sMaskW));

	memset(mSphereGrid, 0, sizeof(uint32_t) * 6 * (sGridSize));
	if(mClothData.mEnableContinuousCollision)
		buildSphereAcceleration(mPrevData.mSpheres);
	buildSphereAcceleration(mCurData.mSpheres);

	memset(mConeGrid, 0, sizeof(uint32_t) * 6 * (sGridSize));
	buildConeAcceleration();

	return true;
}

#ifdef _MSC_VER
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE inline __attribute__((always_inline))
#endif

template <typename Simd4f>
FORCE_INLINE typename cloth::SwCollision<Simd4f>::ShapeMask& cloth::SwCollision<Simd4f>::ShapeMask::
operator=(const ShapeMask& right)
{
	mCones = right.mCones;
	mSpheres = right.mSpheres;
	return *this;
}

template <typename Simd4f>
FORCE_INLINE typename cloth::SwCollision<Simd4f>::ShapeMask& cloth::SwCollision<Simd4f>::ShapeMask::
operator&=(const ShapeMask& right)
{
	mCones = mCones & right.mCones;
	mSpheres = mSpheres & right.mSpheres;
	return *this;
}

template <typename Simd4f>
FORCE_INLINE typename cloth::SwCollision<Simd4f>::ShapeMask
cloth::SwCollision<Simd4f>::getShapeMask(const Simd4f& position, const Simd4i* __restrict sphereGrid,
                                         const Simd4i* __restrict coneGrid)
{
	Gather<Simd4i> gather(intFloor(position));

	ShapeMask result;
	result.mCones = gather(coneGrid);
	result.mSpheres = gather(sphereGrid);
	return result;
}

// lookup acceleration structure and return mask of potential intersectors
template <typename Simd4f>
FORCE_INLINE typename cloth::SwCollision<Simd4f>::ShapeMask
cloth::SwCollision<Simd4f>::getShapeMask(const Simd4f* __restrict positions) const
{
	Simd4f posX = positions[0] * splat<0>(mGridScale) + splat<0>(mGridBias);
	Simd4f posY = positions[1] * splat<1>(mGridScale) + splat<1>(mGridBias);
	Simd4f posZ = positions[2] * splat<2>(mGridScale) + splat<2>(mGridBias);

	ShapeMask result = getShapeMask(posX, mSphereGrid, mConeGrid);
	result &= getShapeMask(posY, mSphereGrid + 2, mConeGrid + 2);
	result &= getShapeMask(posZ, mSphereGrid + 4, mConeGrid + 4);

	return result;
}

// lookup acceleration structure and return mask of potential intersectors
template <typename Simd4f>
FORCE_INLINE typename cloth::SwCollision<Simd4f>::ShapeMask
cloth::SwCollision<Simd4f>::getShapeMask(const Simd4f* __restrict prevPos, const Simd4f* __restrict curPos) const
{
	Simd4f scaleX = splat<0>(mGridScale);
	Simd4f scaleY = splat<1>(mGridScale);
	Simd4f scaleZ = splat<2>(mGridScale);

	Simd4f biasX = splat<0>(mGridBias);
	Simd4f biasY = splat<1>(mGridBias);
	Simd4f biasZ = splat<2>(mGridBias);

	Simd4f prevX = prevPos[0] * scaleX + biasX;
	Simd4f prevY = prevPos[1] * scaleY + biasY;
	Simd4f prevZ = prevPos[2] * scaleZ + biasZ;

	Simd4f curX = curPos[0] * scaleX + biasX;
	Simd4f curY = curPos[1] * scaleY + biasY;
	Simd4f curZ = curPos[2] * scaleZ + biasZ;

	Simd4f maxX = min(max(prevX, curX), sGridLength);
	Simd4f maxY = min(max(prevY, curY), sGridLength);
	Simd4f maxZ = min(max(prevZ, curZ), sGridLength);

	ShapeMask result = getShapeMask(maxX, mSphereGrid, mConeGrid);
	result &= getShapeMask(maxY, mSphereGrid + 2, mConeGrid + 2);
	result &= getShapeMask(maxZ, mSphereGrid + 4, mConeGrid + 4);

	Simd4f zero = gSimd4fZero;
	Simd4f minX = max(min(prevX, curX), zero);
	Simd4f minY = max(min(prevY, curY), zero);
	Simd4f minZ = max(min(prevZ, curZ), zero);

	result &= getShapeMask(minX, mSphereGrid + 6, mConeGrid + 6);
	result &= getShapeMask(minY, mSphereGrid + 8, mConeGrid + 8);
	result &= getShapeMask(minZ, mSphereGrid + 10, mConeGrid + 10);

	return result;
}

template <typename Simd4f>
struct cloth::SwCollision<Simd4f>::ImpulseAccumulator
{
	ImpulseAccumulator()
	: mDeltaX(gSimd4fZero)
	, mDeltaY(mDeltaX)
	, mDeltaZ(mDeltaX)
	, mVelX(mDeltaX)
	, mVelY(mDeltaX)
	, mVelZ(mDeltaX)
	, mNumCollisions(gSimd4fEpsilon)
	{
	}

	void add(const Simd4f& x, const Simd4f& y, const Simd4f& z, const Simd4f& scale, const Simd4f& mask)
	{
		PX_ASSERT(allTrue((mask & x) == (mask & x)));
		PX_ASSERT(allTrue((mask & y) == (mask & y)));
		PX_ASSERT(allTrue((mask & z) == (mask & z)));
		PX_ASSERT(allTrue((mask & scale) == (mask & scale)));

		Simd4f maskedScale = scale & mask;
		mDeltaX = mDeltaX + x * maskedScale;
		mDeltaY = mDeltaY + y * maskedScale;
		mDeltaZ = mDeltaZ + z * maskedScale;
		mNumCollisions = mNumCollisions + (gSimd4fOne & mask);
	}

	void addVelocity(const Simd4f& vx, const Simd4f& vy, const Simd4f& vz, const Simd4f& mask)
	{
		PX_ASSERT(allTrue((mask & vx) == (mask & vx)));
		PX_ASSERT(allTrue((mask & vy) == (mask & vy)));
		PX_ASSERT(allTrue((mask & vz) == (mask & vz)));

		mVelX = mVelX + (vx & mask);
		mVelY = mVelY + (vy & mask);
		mVelZ = mVelZ + (vz & mask);
	}

	void subtract(const Simd4f& x, const Simd4f& y, const Simd4f& z, const Simd4f& scale, const Simd4f& mask)
	{
		PX_ASSERT(allTrue((mask & x) == (mask & x)));
		PX_ASSERT(allTrue((mask & y) == (mask & y)));
		PX_ASSERT(allTrue((mask & z) == (mask & z)));
		PX_ASSERT(allTrue((mask & scale) == (mask & scale)));

		Simd4f maskedScale = scale & mask;
		mDeltaX = mDeltaX - x * maskedScale;
		mDeltaY = mDeltaY - y * maskedScale;
		mDeltaZ = mDeltaZ - z * maskedScale;
		mNumCollisions = mNumCollisions + (gSimd4fOne & mask);
	}

	Simd4f mDeltaX, mDeltaY, mDeltaZ;
	Simd4f mVelX, mVelY, mVelZ;
	Simd4f mNumCollisions;
};

template <typename Simd4f>
FORCE_INLINE void cloth::SwCollision<Simd4f>::collideSpheres(const Simd4i& sphereMask, const Simd4f* positions,
                                                             ImpulseAccumulator& accum) const
{
	const float* __restrict spherePtr = array(mCurData.mSpheres->center);

	bool frictionEnabled = mClothData.mFrictionScale > 0.0f;

	Simd4i mask4 = horizontalOr(sphereMask);
	uint32_t mask = uint32_t(array(mask4)[0]);
	while(mask)
	{
		uint32_t test = mask - 1;
		uint32_t offset = findBitSet(mask & ~test) * sizeof(SphereData);
		mask = mask & test;

		Simd4f sphere = loadAligned(spherePtr, offset);

		Simd4f deltaX = positions[0] - splat<0>(sphere);
		Simd4f deltaY = positions[1] - splat<1>(sphere);
		Simd4f deltaZ = positions[2] - splat<2>(sphere);

		Simd4f sqrDistance = gSimd4fEpsilon + deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ;
		Simd4f negativeScale = gSimd4fOne - rsqrt(sqrDistance) * splat<3>(sphere);

		Simd4f contactMask;
		if(!anyGreater(gSimd4fZero, negativeScale, contactMask))
			continue;

		accum.subtract(deltaX, deltaY, deltaZ, negativeScale, contactMask);

		if(frictionEnabled)
		{
			// load previous sphere pos
			const float* __restrict prevSpherePtr = array(mPrevData.mSpheres->center);

			Simd4f prevSphere = loadAligned(prevSpherePtr, offset);
			Simd4f velocity = sphere - prevSphere;

			accum.addVelocity(splat<0>(velocity), splat<1>(velocity), splat<2>(velocity), contactMask);
		}
	}
}

template <typename Simd4f>
FORCE_INLINE typename cloth::SwCollision<Simd4f>::Simd4i
cloth::SwCollision<Simd4f>::collideCones(const Simd4f* __restrict positions, ImpulseAccumulator& accum) const
{
	const float* __restrict centerPtr = array(mCurData.mCones->center);
	const float* __restrict axisPtr = array(mCurData.mCones->axis);
	const int32_t* __restrict auxiliaryPtr = reinterpret_cast<const int32_t*>(&mCurData.mCones->sqrCosine);

	bool frictionEnabled = mClothData.mFrictionScale > 0.0f;

	ShapeMask shapeMask = getShapeMask(positions);
	Simd4i mask4 = horizontalOr(shapeMask.mCones);
	uint32_t mask = uint32_t(array(mask4)[0]);
	while(mask)
	{
		uint32_t test = mask - 1;
		uint32_t coneIndex = findBitSet(mask & ~test);
		uint32_t offset = coneIndex * sizeof(ConeData);
		mask = mask & test;

		Simd4i test4 = mask4 - gSimd4iOne;
		Simd4f culled = simd4f(andNotIsZero(shapeMask.mCones, test4));
		mask4 = mask4 & test4;

		Simd4f center = loadAligned(centerPtr, offset);

		Simd4f deltaX = positions[0] - splat<0>(center);
		Simd4f deltaY = positions[1] - splat<1>(center);
		Simd4f deltaZ = positions[2] - splat<2>(center);

		Simd4f axis = loadAligned(axisPtr, offset);

		Simd4f axisX = splat<0>(axis);
		Simd4f axisY = splat<1>(axis);
		Simd4f axisZ = splat<2>(axis);
		Simd4f slope = splat<3>(axis);

		Simd4f dot = deltaX * axisX + deltaY * axisY + deltaZ * axisZ;
		Simd4f radius = dot * slope + splat<3>(center);

		// set radius to zero if cone is culled
		radius = max(radius, gSimd4fZero) & ~culled;

		Simd4f sqrDistance = deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ - dot * dot;

		Simd4i auxiliary = loadAligned(auxiliaryPtr, offset);
		Simd4i bothMask = splat<3>(auxiliary);

		Simd4f contactMask;
		if(!anyGreater(radius * radius, sqrDistance, contactMask))
		{
			// cone only culled when spheres culled, ok to clear those too
			shapeMask.mSpheres = shapeMask.mSpheres & ~bothMask;
			continue;
		}

		// clamp to a small positive epsilon to avoid numerical error
		// making sqrDistance negative when point lies on the cone axis
		sqrDistance = max(sqrDistance, gSimd4fEpsilon);

		Simd4f invDistance = rsqrt(sqrDistance);
		Simd4f base = dot + slope * sqrDistance * invDistance;

		// force left/rightMask to false if not inside cone
		base = base & contactMask;

		Simd4f halfLength = splat<1>(simd4f(auxiliary));
		Simd4i leftMask = simd4i(base < -halfLength);
		Simd4i rightMask = simd4i(base > halfLength);

		// we use both mask because of the early out above.
		Simd4i firstMask = splat<2>(auxiliary);
		Simd4i secondMask = firstMask ^ bothMask;
		shapeMask.mSpheres = shapeMask.mSpheres & ~(firstMask & ~leftMask);
		shapeMask.mSpheres = shapeMask.mSpheres & ~(secondMask & ~rightMask);

		deltaX = deltaX - base * axisX;
		deltaY = deltaY - base * axisY;
		deltaZ = deltaZ - base * axisZ;

		Simd4f sqrCosine = splat<0>(simd4f(auxiliary));
		Simd4f scale = radius * invDistance * sqrCosine - sqrCosine;

		contactMask = contactMask & ~simd4f(leftMask | rightMask);

		if(!anyTrue(contactMask))
			continue;

		accum.add(deltaX, deltaY, deltaZ, scale, contactMask);

		if(frictionEnabled)
		{
			uint32_t s0 = mClothData.mCapsuleIndices[coneIndex].first;
			uint32_t s1 = mClothData.mCapsuleIndices[coneIndex].second;

			float* prevSpheres = reinterpret_cast<float*>(mPrevData.mSpheres);
			float* curSpheres = reinterpret_cast<float*>(mCurData.mSpheres);

			// todo: could pre-compute sphere velocities or it might be
			// faster to compute cur/prev sphere positions directly
			Simd4f s0p0 = loadAligned(prevSpheres, s0 * sizeof(SphereData));
			Simd4f s0p1 = loadAligned(curSpheres, s0 * sizeof(SphereData));

			Simd4f s1p0 = loadAligned(prevSpheres, s1 * sizeof(SphereData));
			Simd4f s1p1 = loadAligned(curSpheres, s1 * sizeof(SphereData));

			Simd4f v0 = s0p1 - s0p0;
			Simd4f v1 = s1p1 - s1p0;
			Simd4f vd = v1 - v0;

			// dot is in the range -1 to 1, scale and bias to 0 to 1
			dot = dot * gSimd4fHalf + gSimd4fHalf;

			// interpolate velocity at contact points
			Simd4f vx = splat<0>(v0) + dot * splat<0>(vd);
			Simd4f vy = splat<1>(v0) + dot * splat<1>(vd);
			Simd4f vz = splat<2>(v0) + dot * splat<2>(vd);

			accum.addVelocity(vx, vy, vz, contactMask);
		}
	}

	return shapeMask.mSpheres;
}

template <typename Simd4f>
FORCE_INLINE void cloth::SwCollision<Simd4f>::collideSpheres(const Simd4i& sphereMask, const Simd4f* __restrict prevPos,
                                                             Simd4f* __restrict curPos, ImpulseAccumulator& accum) const
{
	const float* __restrict prevSpheres = array(mPrevData.mSpheres->center);
	const float* __restrict curSpheres = array(mCurData.mSpheres->center);

	bool frictionEnabled = mClothData.mFrictionScale > 0.0f;

	Simd4i mask4 = horizontalOr(sphereMask);
	uint32_t mask = uint32_t(array(mask4)[0]);
	while(mask)
	{
		uint32_t test = mask - 1;
		uint32_t offset = findBitSet(mask & ~test) * sizeof(SphereData);
		mask = mask & test;

		Simd4f prevSphere = loadAligned(prevSpheres, offset);
		Simd4f prevX = prevPos[0] - splat<0>(prevSphere);
		Simd4f prevY = prevPos[1] - splat<1>(prevSphere);
		Simd4f prevZ = prevPos[2] - splat<2>(prevSphere);
		Simd4f prevRadius = splat<3>(prevSphere);

		Simd4f curSphere = loadAligned(curSpheres, offset);
		Simd4f curX = curPos[0] - splat<0>(curSphere);
		Simd4f curY = curPos[1] - splat<1>(curSphere);
		Simd4f curZ = curPos[2] - splat<2>(curSphere);
		Simd4f curRadius = splat<3>(curSphere);

		Simd4f sqrDistance = gSimd4fEpsilon + curX * curX + curY * curY + curZ * curZ;

		Simd4f dotPrevPrev = prevX * prevX + prevY * prevY + prevZ * prevZ - prevRadius * prevRadius;
		Simd4f dotPrevCur = prevX * curX + prevY * curY + prevZ * curZ - prevRadius * curRadius;
		Simd4f dotCurCur = sqrDistance - curRadius * curRadius;

		Simd4f discriminant = dotPrevCur * dotPrevCur - dotCurCur * dotPrevPrev;
		Simd4f sqrtD = sqrt(discriminant);
		Simd4f halfB = dotPrevCur - dotPrevPrev;
		Simd4f minusA = dotPrevCur - dotCurCur + halfB;

		// time of impact or 0 if prevPos inside sphere
		Simd4f toi = recip(minusA) * min(gSimd4fZero, halfB + sqrtD);
		Simd4f collisionMask = (toi < gSimd4fOne) & (halfB < sqrtD);

		// skip continuous collision if the (un-clamped) particle
		// trajectory only touches the outer skin of the cone.
		Simd4f rMin = prevRadius + halfB * minusA * (curRadius - prevRadius);
		collisionMask = collisionMask & (discriminant > minusA * rMin * rMin * sSkeletonWidth);

		// a is negative when one sphere is contained in the other,
		// which is already handled by discrete collision.
		collisionMask = collisionMask & (minusA < -static_cast<Simd4f>(gSimd4fEpsilon));

		if(!allEqual(collisionMask, gSimd4fZero))
		{
			Simd4f deltaX = prevX - curX;
			Simd4f deltaY = prevY - curY;
			Simd4f deltaZ = prevZ - curZ;

			Simd4f oneMinusToi = (gSimd4fOne - toi) & collisionMask;

			// reduce ccd impulse if (clamped) particle trajectory stays in sphere skin,
			// i.e. scale by exp2(-k) or 1/(1+k) with k = (tmin - toi) / (1 - toi)
			Simd4f minusK = sqrtD * recip(minusA * oneMinusToi) & (oneMinusToi > gSimd4fEpsilon);
			oneMinusToi = oneMinusToi * recip(gSimd4fOne - minusK);

			curX = curX + deltaX * oneMinusToi;
			curY = curY + deltaY * oneMinusToi;
			curZ = curZ + deltaZ * oneMinusToi;

			curPos[0] = splat<0>(curSphere) + curX;
			curPos[1] = splat<1>(curSphere) + curY;
			curPos[2] = splat<2>(curSphere) + curZ;

			sqrDistance = gSimd4fEpsilon + curX * curX + curY * curY + curZ * curZ;
		}

		Simd4f negativeScale = gSimd4fOne - rsqrt(sqrDistance) * curRadius;

		Simd4f contactMask;
		if(!anyGreater(gSimd4fZero, negativeScale, contactMask))
			continue;

		accum.subtract(curX, curY, curZ, negativeScale, contactMask);

		if(frictionEnabled)
		{
			Simd4f velocity = curSphere - prevSphere;
			accum.addVelocity(splat<0>(velocity), splat<1>(velocity), splat<2>(velocity), contactMask);
		}
	}
}

template <typename Simd4f>
FORCE_INLINE typename cloth::SwCollision<Simd4f>::Simd4i
cloth::SwCollision<Simd4f>::collideCones(const Simd4f* __restrict prevPos, Simd4f* __restrict curPos,
                                         ImpulseAccumulator& accum) const
{
	const float* __restrict prevCenterPtr = array(mPrevData.mCones->center);
	const float* __restrict prevAxisPtr = array(mPrevData.mCones->axis);
	const int32_t* __restrict prevAuxiliaryPtr = reinterpret_cast<const int32_t*>(&mPrevData.mCones->sqrCosine);

	const float* __restrict curCenterPtr = array(mCurData.mCones->center);
	const float* __restrict curAxisPtr = array(mCurData.mCones->axis);
	const int32_t* __restrict curAuxiliaryPtr = reinterpret_cast<const int32_t*>(&mCurData.mCones->sqrCosine);

	bool frictionEnabled = mClothData.mFrictionScale > 0.0f;

	ShapeMask shapeMask = getShapeMask(prevPos, curPos);
	Simd4i mask4 = horizontalOr(shapeMask.mCones);
	uint32_t mask = uint32_t(array(mask4)[0]);
	while(mask)
	{
		uint32_t test = mask - 1;
		uint32_t coneIndex = findBitSet(mask & ~test);
		uint32_t offset = coneIndex * sizeof(ConeData);
		mask = mask & test;

		Simd4i test4 = mask4 - gSimd4iOne;
		Simd4f culled = simd4f(andNotIsZero(shapeMask.mCones, test4));
		mask4 = mask4 & test4;

		Simd4f prevCenter = loadAligned(prevCenterPtr, offset);
		Simd4f prevAxis = loadAligned(prevAxisPtr, offset);
		Simd4f prevAxisX = splat<0>(prevAxis);
		Simd4f prevAxisY = splat<1>(prevAxis);
		Simd4f prevAxisZ = splat<2>(prevAxis);
		Simd4f prevSlope = splat<3>(prevAxis);

		Simd4f prevX = prevPos[0] - splat<0>(prevCenter);
		Simd4f prevY = prevPos[1] - splat<1>(prevCenter);
		Simd4f prevZ = prevPos[2] - splat<2>(prevCenter);
		Simd4f prevT = prevY * prevAxisZ - prevZ * prevAxisY;
		Simd4f prevU = prevZ * prevAxisX - prevX * prevAxisZ;
		Simd4f prevV = prevX * prevAxisY - prevY * prevAxisX;
		Simd4f prevDot = prevX * prevAxisX + prevY * prevAxisY + prevZ * prevAxisZ;
		Simd4f prevRadius = prevDot * prevSlope + splat<3>(prevCenter);

		Simd4f curCenter = loadAligned(curCenterPtr, offset);
		Simd4f curAxis = loadAligned(curAxisPtr, offset);
		Simd4f curAxisX = splat<0>(curAxis);
		Simd4f curAxisY = splat<1>(curAxis);
		Simd4f curAxisZ = splat<2>(curAxis);
		Simd4f curSlope = splat<3>(curAxis);
		Simd4i curAuxiliary = loadAligned(curAuxiliaryPtr, offset);

		Simd4f curX = curPos[0] - splat<0>(curCenter);
		Simd4f curY = curPos[1] - splat<1>(curCenter);
		Simd4f curZ = curPos[2] - splat<2>(curCenter);
		Simd4f curT = curY * curAxisZ - curZ * curAxisY;
		Simd4f curU = curZ * curAxisX - curX * curAxisZ;
		Simd4f curV = curX * curAxisY - curY * curAxisX;
		Simd4f curDot = curX * curAxisX + curY * curAxisY + curZ * curAxisZ;
		Simd4f curRadius = curDot * curSlope + splat<3>(curCenter);

		Simd4f curSqrDistance = gSimd4fEpsilon + curT * curT + curU * curU + curV * curV;

		// set radius to zero if cone is culled
		prevRadius = max(prevRadius, gSimd4fZero) & ~culled;
		curRadius = max(curRadius, gSimd4fZero) & ~culled;

		Simd4f dotPrevPrev = prevT * prevT + prevU * prevU + prevV * prevV - prevRadius * prevRadius;
		Simd4f dotPrevCur = prevT * curT + prevU * curU + prevV * curV - prevRadius * curRadius;
		Simd4f dotCurCur = curSqrDistance - curRadius * curRadius;

		Simd4f discriminant = dotPrevCur * dotPrevCur - dotCurCur * dotPrevPrev;
		Simd4f sqrtD = sqrt(discriminant);
		Simd4f halfB = dotPrevCur - dotPrevPrev;
		Simd4f minusA = dotPrevCur - dotCurCur + halfB;

		// time of impact or 0 if prevPos inside cone
		Simd4f toi = recip(minusA) * min(gSimd4fZero, halfB + sqrtD);
		Simd4f collisionMask = (toi < gSimd4fOne) & (halfB < sqrtD);

		// skip continuous collision if the (un-clamped) particle
		// trajectory only touches the outer skin of the cone.
		Simd4f rMin = prevRadius + halfB * minusA * (curRadius - prevRadius);
		collisionMask = collisionMask & (discriminant > minusA * rMin * rMin * sSkeletonWidth);

		// a is negative when one cone is contained in the other,
		// which is already handled by discrete collision.
		collisionMask = collisionMask & (minusA < -static_cast<Simd4f>(gSimd4fEpsilon));

		// test if any particle hits infinite cone (and 0<time of impact<1)
		if(!allEqual(collisionMask, gSimd4fZero))
		{
			Simd4f deltaX = prevX - curX;
			Simd4f deltaY = prevY - curY;
			Simd4f deltaZ = prevZ - curZ;

			// interpolate delta at toi
			Simd4f posX = prevX - deltaX * toi;
			Simd4f posY = prevY - deltaY * toi;
			Simd4f posZ = prevZ - deltaZ * toi;

			Simd4f curScaledAxis = curAxis * splat<1>(simd4f(curAuxiliary));
			Simd4i prevAuxiliary = loadAligned(prevAuxiliaryPtr, offset);
			Simd4f deltaScaledAxis = curScaledAxis - prevAxis * splat<1>(simd4f(prevAuxiliary));

			Simd4f oneMinusToi = gSimd4fOne - toi;

			// interpolate axis at toi
			Simd4f axisX = splat<0>(curScaledAxis) - splat<0>(deltaScaledAxis) * oneMinusToi;
			Simd4f axisY = splat<1>(curScaledAxis) - splat<1>(deltaScaledAxis) * oneMinusToi;
			Simd4f axisZ = splat<2>(curScaledAxis) - splat<2>(deltaScaledAxis) * oneMinusToi;
			Simd4f slope = (prevSlope * oneMinusToi + curSlope * toi);

			Simd4f sqrHalfLength = axisX * axisX + axisY * axisY + axisZ * axisZ;
			Simd4f invHalfLength = rsqrt(sqrHalfLength);
			Simd4f dot = (posX * axisX + posY * axisY + posZ * axisZ) * invHalfLength;

			Simd4f sqrDistance = posX * posX + posY * posY + posZ * posZ - dot * dot;
			Simd4f invDistance = rsqrt(sqrDistance) & (sqrDistance > gSimd4fZero);

			Simd4f base = dot + slope * sqrDistance * invDistance;
			Simd4f scale = base * invHalfLength & collisionMask;

			Simd4f cullMask = (abs(scale) < gSimd4fOne) & collisionMask;

			// test if any impact position is in cone section
			if(!allEqual(cullMask, gSimd4fZero))
			{
				deltaX = deltaX + splat<0>(deltaScaledAxis) * scale;
				deltaY = deltaY + splat<1>(deltaScaledAxis) * scale;
				deltaZ = deltaZ + splat<2>(deltaScaledAxis) * scale;

				oneMinusToi = oneMinusToi & cullMask;

				// reduce ccd impulse if (clamped) particle trajectory stays in cone skin,
				// i.e. scale by exp2(-k) or 1/(1+k) with k = (tmin - toi) / (1 - toi)
				// oneMinusToi = oneMinusToi * recip(gSimd4fOne - sqrtD * recip(minusA * oneMinusToi));
				Simd4f minusK = sqrtD * recip(minusA * oneMinusToi) & (oneMinusToi > gSimd4fEpsilon);
				oneMinusToi = oneMinusToi * recip(gSimd4fOne - minusK);

				curX = curX + deltaX * oneMinusToi;
				curY = curY + deltaY * oneMinusToi;
				curZ = curZ + deltaZ * oneMinusToi;

				curDot = curX * curAxisX + curY * curAxisY + curZ * curAxisZ;
				curRadius = curDot * curSlope + splat<3>(curCenter);
				curRadius = max(curRadius, gSimd4fZero) & ~culled;
				curSqrDistance = curX * curX + curY * curY + curZ * curZ - curDot * curDot;

				curPos[0] = splat<0>(curCenter) + curX;
				curPos[1] = splat<1>(curCenter) + curY;
				curPos[2] = splat<2>(curCenter) + curZ;
			}
		}

		// curPos inside cone (discrete collision)
		Simd4f contactMask;
		int anyContact = anyGreater(curRadius * curRadius, curSqrDistance, contactMask);

		Simd4i bothMask = splat<3>(curAuxiliary);

		// instead of culling continuous collision for ~collisionMask, and discrete
		// collision for ~contactMask, disable both if ~collisionMask & ~contactMask
		Simd4i cullMask = bothMask & ~simd4i(collisionMask | contactMask);
		shapeMask.mSpheres = shapeMask.mSpheres & ~cullMask;

		if(!anyContact)
			continue;

		Simd4f invDistance = rsqrt(curSqrDistance) & (curSqrDistance > gSimd4fZero);
		Simd4f base = curDot + curSlope * curSqrDistance * invDistance;

		Simd4f halfLength = splat<1>(simd4f(curAuxiliary));
		Simd4i leftMask = simd4i(base < -halfLength);
		Simd4i rightMask = simd4i(base > halfLength);

		// can only skip continuous sphere collision if post-ccd position
		// is on code side *and* particle had cone-ccd collision.
		Simd4i firstMask = splat<2>(curAuxiliary);
		Simd4i secondMask = firstMask ^ bothMask;
		cullMask = (firstMask & ~leftMask) | (secondMask & ~rightMask);
		shapeMask.mSpheres = shapeMask.mSpheres & ~(cullMask & simd4i(collisionMask));

		Simd4f deltaX = curX - base * curAxisX;
		Simd4f deltaY = curY - base * curAxisY;
		Simd4f deltaZ = curZ - base * curAxisZ;

		Simd4f sqrCosine = splat<0>(simd4f(curAuxiliary));
		Simd4f scale = curRadius * invDistance * sqrCosine - sqrCosine;

		contactMask = contactMask & ~simd4f(leftMask | rightMask);

		if(!anyTrue(contactMask))
			continue;

		accum.add(deltaX, deltaY, deltaZ, scale, contactMask);

		if(frictionEnabled)
		{
			uint32_t s0 = mClothData.mCapsuleIndices[coneIndex].first;
			uint32_t s1 = mClothData.mCapsuleIndices[coneIndex].second;

			float* prevSpheres = reinterpret_cast<float*>(mPrevData.mSpheres);
			float* curSpheres = reinterpret_cast<float*>(mCurData.mSpheres);

			// todo: could pre-compute sphere velocities or it might be
			// faster to compute cur/prev sphere positions directly
			Simd4f s0p0 = loadAligned(prevSpheres, s0 * sizeof(SphereData));
			Simd4f s0p1 = loadAligned(curSpheres, s0 * sizeof(SphereData));

			Simd4f s1p0 = loadAligned(prevSpheres, s1 * sizeof(SphereData));
			Simd4f s1p1 = loadAligned(curSpheres, s1 * sizeof(SphereData));

			Simd4f v0 = s0p1 - s0p0;
			Simd4f v1 = s1p1 - s1p0;
			Simd4f vd = v1 - v0;

			// dot is in the range -1 to 1, scale and bias to 0 to 1
			curDot = curDot * gSimd4fHalf + gSimd4fHalf;

			// interpolate velocity at contact points
			Simd4f vx = splat<0>(v0) + curDot * splat<0>(vd);
			Simd4f vy = splat<1>(v0) + curDot * splat<1>(vd);
			Simd4f vz = splat<2>(v0) + curDot * splat<2>(vd);

			accum.addVelocity(vx, vy, vz, contactMask);
		}
	}

	return shapeMask.mSpheres;
}

namespace
{

template <typename Simd4f>
PX_INLINE void calculateFrictionImpulse(const Simd4f& deltaX, const Simd4f& deltaY, const Simd4f& deltaZ,
                                        const Simd4f& velX, const Simd4f& velY, const Simd4f& velZ,
                                        const Simd4f* curPos, const Simd4f* prevPos, const Simd4f& scale,
                                        const Simd4f& coefficient, const Simd4f& mask, Simd4f* impulse)
{
	// calculate collision normal
	Simd4f deltaSq = deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ;

	Simd4f rcpDelta = rsqrt(deltaSq + gSimd4fEpsilon);

	Simd4f nx = deltaX * rcpDelta;
	Simd4f ny = deltaY * rcpDelta;
	Simd4f nz = deltaZ * rcpDelta;

	// calculate relative velocity scaled by number of collisions
	Simd4f rvx = curPos[0] - prevPos[0] - velX * scale;
	Simd4f rvy = curPos[1] - prevPos[1] - velY * scale;
	Simd4f rvz = curPos[2] - prevPos[2] - velZ * scale;

	// calculate magnitude of relative normal velocity
	Simd4f rvn = rvx * nx + rvy * ny + rvz * nz;

	// calculate relative tangential velocity
	Simd4f rvtx = rvx - rvn * nx;
	Simd4f rvty = rvy - rvn * ny;
	Simd4f rvtz = rvz - rvn * nz;

	// calculate magnitude of vt
	Simd4f rcpVt = rsqrt(rvtx * rvtx + rvty * rvty + rvtz * rvtz + gSimd4fEpsilon);

	// magnitude of friction impulse (cannot be greater than -vt)
	Simd4f j = max(-coefficient * deltaSq * rcpDelta * rcpVt, gSimd4fMinusOne) & mask;

	impulse[0] = rvtx * j;
	impulse[1] = rvty * j;
	impulse[2] = rvtz * j;
}

} // anonymous namespace

template <typename Simd4f>
void cloth::SwCollision<Simd4f>::collideParticles()
{
	const bool massScalingEnabled = mClothData.mCollisionMassScale > 0.0f;
	const Simd4f massScale = simd4f(mClothData.mCollisionMassScale);

	const bool frictionEnabled = mClothData.mFrictionScale > 0.0f;
	const Simd4f frictionScale = simd4f(mClothData.mFrictionScale);

	Simd4f curPos[4];
	Simd4f prevPos[4];

	float* __restrict prevIt = mClothData.mPrevParticles;
	float* __restrict pIt = mClothData.mCurParticles;
	float* __restrict pEnd = pIt + mClothData.mNumParticles * 4;
	for(; pIt < pEnd; pIt += 16, prevIt += 16)
	{
		curPos[0] = loadAligned(pIt, 0);
		curPos[1] = loadAligned(pIt, 16);
		curPos[2] = loadAligned(pIt, 32);
		curPos[3] = loadAligned(pIt, 48);
		transpose(curPos[0], curPos[1], curPos[2], curPos[3]);

		ImpulseAccumulator accum;
		Simd4i sphereMask = collideCones(curPos, accum);
		collideSpheres(sphereMask, curPos, accum);

		Simd4f mask;
		if(!anyGreater(accum.mNumCollisions, gSimd4fEpsilon, mask))
			continue;

		Simd4f invNumCollisions = recip(accum.mNumCollisions);

		if(frictionEnabled)
		{
			prevPos[0] = loadAligned(prevIt, 0);
			prevPos[1] = loadAligned(prevIt, 16);
			prevPos[2] = loadAligned(prevIt, 32);
			prevPos[3] = loadAligned(prevIt, 48);
			transpose(prevPos[0], prevPos[1], prevPos[2], prevPos[3]);

			Simd4f frictionImpulse[3];
			calculateFrictionImpulse(accum.mDeltaX, accum.mDeltaY, accum.mDeltaZ, accum.mVelX, accum.mVelY, accum.mVelZ,
			                         curPos, prevPos, invNumCollisions, frictionScale, mask, frictionImpulse);

			prevPos[0] = prevPos[0] - frictionImpulse[0];
			prevPos[1] = prevPos[1] - frictionImpulse[1];
			prevPos[2] = prevPos[2] - frictionImpulse[2];

			transpose(prevPos[0], prevPos[1], prevPos[2], prevPos[3]);
			storeAligned(prevIt, 0, prevPos[0]);
			storeAligned(prevIt, 16, prevPos[1]);
			storeAligned(prevIt, 32, prevPos[2]);
			storeAligned(prevIt, 48, prevPos[3]);
		}

		if(massScalingEnabled)
		{
			// calculate the inverse mass scale based on the collision impulse magnitude
			Simd4f dSq = invNumCollisions * invNumCollisions *
			             (accum.mDeltaX * accum.mDeltaX + accum.mDeltaY * accum.mDeltaY + accum.mDeltaZ * accum.mDeltaZ);

			Simd4f scale = recip(gSimd4fOne + massScale * dSq);

			// scale invmass
			curPos[3] = select(mask, curPos[3] * scale, curPos[3]);
		}

		curPos[0] = curPos[0] + accum.mDeltaX * invNumCollisions;
		curPos[1] = curPos[1] + accum.mDeltaY * invNumCollisions;
		curPos[2] = curPos[2] + accum.mDeltaZ * invNumCollisions;

		transpose(curPos[0], curPos[1], curPos[2], curPos[3]);
		storeAligned(pIt, 0, curPos[0]);
		storeAligned(pIt, 16, curPos[1]);
		storeAligned(pIt, 32, curPos[2]);
		storeAligned(pIt, 48, curPos[3]);

#if PX_PROFILE || PX_DEBUG
		mNumCollisions += horizontalSum(accum.mNumCollisions);
#endif
	}
}

template <typename Simd4f>
void cloth::SwCollision<Simd4f>::collideVirtualParticles()
{
	const bool massScalingEnabled = mClothData.mCollisionMassScale > 0.0f;
	const Simd4f massScale = simd4f(mClothData.mCollisionMassScale);

	const bool frictionEnabled = mClothData.mFrictionScale > 0.0f;
	const Simd4f frictionScale = simd4f(mClothData.mFrictionScale);

	Simd4f curPos[3];

	const float* __restrict weights = mClothData.mVirtualParticleWeights;
	float* __restrict particles = mClothData.mCurParticles;
	float* __restrict prevParticles = mClothData.mPrevParticles;

	// move dummy particles outside of collision range
	Simd4f* __restrict dummy = mClothData.mNumParticles + reinterpret_cast<Simd4f*>(mClothData.mCurParticles);
	Simd4f invGridScale = recip(mGridScale) & (mGridScale > gSimd4fEpsilon);
	dummy[0] = dummy[1] = dummy[2] = invGridScale * mGridBias - invGridScale;

	const uint16_t* __restrict vpIt = mClothData.mVirtualParticlesBegin;
	const uint16_t* __restrict vpEnd = mClothData.mVirtualParticlesEnd;
	for(; vpIt != vpEnd; vpIt += 16)
	{
		// load 12 particles and 4 weights
		Simd4f p0v0 = loadAligned(particles, vpIt[0] * sizeof(PxVec4));
		Simd4f p0v1 = loadAligned(particles, vpIt[1] * sizeof(PxVec4));
		Simd4f p0v2 = loadAligned(particles, vpIt[2] * sizeof(PxVec4));
		Simd4f w0 = loadAligned(weights, vpIt[3] * sizeof(PxVec4));

		Simd4f p1v0 = loadAligned(particles, vpIt[4] * sizeof(PxVec4));
		Simd4f p1v1 = loadAligned(particles, vpIt[5] * sizeof(PxVec4));
		Simd4f p1v2 = loadAligned(particles, vpIt[6] * sizeof(PxVec4));
		Simd4f w1 = loadAligned(weights, vpIt[7] * sizeof(PxVec4));

		Simd4f p2v0 = loadAligned(particles, vpIt[8] * sizeof(PxVec4));
		Simd4f p2v1 = loadAligned(particles, vpIt[9] * sizeof(PxVec4));
		Simd4f p2v2 = loadAligned(particles, vpIt[10] * sizeof(PxVec4));
		Simd4f w2 = loadAligned(weights, vpIt[11] * sizeof(PxVec4));

		Simd4f p3v1 = loadAligned(particles, vpIt[13] * sizeof(PxVec4));
		Simd4f p3v0 = loadAligned(particles, vpIt[12] * sizeof(PxVec4));
		Simd4f p3v2 = loadAligned(particles, vpIt[14] * sizeof(PxVec4));
		Simd4f w3 = loadAligned(weights, vpIt[15] * sizeof(PxVec4));

		// interpolate particles and transpose
		Simd4f px = p0v0 * splat<0>(w0) + p0v1 * splat<1>(w0) + p0v2 * splat<2>(w0);
		Simd4f py = p1v0 * splat<0>(w1) + p1v1 * splat<1>(w1) + p1v2 * splat<2>(w1);
		Simd4f pz = p2v0 * splat<0>(w2) + p2v1 * splat<1>(w2) + p2v2 * splat<2>(w2);
		Simd4f pw = p3v0 * splat<0>(w3) + p3v1 * splat<1>(w3) + p3v2 * splat<2>(w3);
		transpose(px, py, pz, pw);

		curPos[0] = px;
		curPos[1] = py;
		curPos[2] = pz;

		ImpulseAccumulator accum;
		Simd4i sphereMask = collideCones(curPos, accum);
		collideSpheres(sphereMask, curPos, accum);

		Simd4f mask;
		if(!anyGreater(accum.mNumCollisions, gSimd4fEpsilon, mask))
			continue;

		Simd4f invNumCollisions = recip(accum.mNumCollisions);

		// displacement and transpose back
		Simd4f d0 = accum.mDeltaX * invNumCollisions;
		Simd4f d1 = accum.mDeltaY * invNumCollisions;
		Simd4f d2 = accum.mDeltaZ * invNumCollisions;
		Simd4f d3 = gSimd4fZero;
		transpose(d0, d1, d2, d3);

		// scale weights by 1/dot(w,w)
		Simd4f rw0 = w0 * splat<3>(w0);
		Simd4f rw1 = w1 * splat<3>(w1);
		Simd4f rw2 = w2 * splat<3>(w2);
		Simd4f rw3 = w3 * splat<3>(w3);

		if(frictionEnabled)
		{
			Simd4f q0v0 = loadAligned(prevParticles, vpIt[0] * sizeof(PxVec4));
			Simd4f q0v1 = loadAligned(prevParticles, vpIt[1] * sizeof(PxVec4));
			Simd4f q0v2 = loadAligned(prevParticles, vpIt[2] * sizeof(PxVec4));

			Simd4f q1v0 = loadAligned(prevParticles, vpIt[4] * sizeof(PxVec4));
			Simd4f q1v1 = loadAligned(prevParticles, vpIt[5] * sizeof(PxVec4));
			Simd4f q1v2 = loadAligned(prevParticles, vpIt[6] * sizeof(PxVec4));

			Simd4f q2v0 = loadAligned(prevParticles, vpIt[8] * sizeof(PxVec4));
			Simd4f q2v1 = loadAligned(prevParticles, vpIt[9] * sizeof(PxVec4));
			Simd4f q2v2 = loadAligned(prevParticles, vpIt[10] * sizeof(PxVec4));

			Simd4f q3v0 = loadAligned(prevParticles, vpIt[12] * sizeof(PxVec4));
			Simd4f q3v1 = loadAligned(prevParticles, vpIt[13] * sizeof(PxVec4));
			Simd4f q3v2 = loadAligned(prevParticles, vpIt[14] * sizeof(PxVec4));

			// calculate previous interpolated positions
			Simd4f qx = q0v0 * splat<0>(w0) + q0v1 * splat<1>(w0) + q0v2 * splat<2>(w0);
			Simd4f qy = q1v0 * splat<0>(w1) + q1v1 * splat<1>(w1) + q1v2 * splat<2>(w1);
			Simd4f qz = q2v0 * splat<0>(w2) + q2v1 * splat<1>(w2) + q2v2 * splat<2>(w2);
			Simd4f qw = q3v0 * splat<0>(w3) + q3v1 * splat<1>(w3) + q3v2 * splat<2>(w3);
			transpose(qx, qy, qz, qw);

			Simd4f prevPos[3] = { qx, qy, qz };
			Simd4f frictionImpulse[4];
			frictionImpulse[3] = gSimd4fZero;

			calculateFrictionImpulse(accum.mDeltaX, accum.mDeltaY, accum.mDeltaZ, accum.mVelX, accum.mVelY, accum.mVelZ,
			                         curPos, prevPos, invNumCollisions, frictionScale, mask, frictionImpulse);

			transpose(frictionImpulse[0], frictionImpulse[1], frictionImpulse[2], frictionImpulse[3]);

			q0v0 = q0v0 - (splat<0>(rw0) * frictionImpulse[0]);
			q0v1 = q0v1 - (splat<1>(rw0) * frictionImpulse[0]);
			q0v2 = q0v2 - (splat<2>(rw0) * frictionImpulse[0]);

			q1v0 = q1v0 - (splat<0>(rw1) * frictionImpulse[1]);
			q1v1 = q1v1 - (splat<1>(rw1) * frictionImpulse[1]);
			q1v2 = q1v2 - (splat<2>(rw1) * frictionImpulse[1]);

			q2v0 = q2v0 - (splat<0>(rw2) * frictionImpulse[2]);
			q2v1 = q2v1 - (splat<1>(rw2) * frictionImpulse[2]);
			q2v2 = q2v2 - (splat<2>(rw2) * frictionImpulse[2]);

			q3v0 = q3v0 - (splat<0>(rw3) * frictionImpulse[3]);
			q3v1 = q3v1 - (splat<1>(rw3) * frictionImpulse[3]);
			q3v2 = q3v2 - (splat<2>(rw3) * frictionImpulse[3]);

			// write back prev particles
			storeAligned(prevParticles, vpIt[0] * sizeof(PxVec4), q0v0);
			storeAligned(prevParticles, vpIt[1] * sizeof(PxVec4), q0v1);
			storeAligned(prevParticles, vpIt[2] * sizeof(PxVec4), q0v2);

			storeAligned(prevParticles, vpIt[4] * sizeof(PxVec4), q1v0);
			storeAligned(prevParticles, vpIt[5] * sizeof(PxVec4), q1v1);
			storeAligned(prevParticles, vpIt[6] * sizeof(PxVec4), q1v2);

			storeAligned(prevParticles, vpIt[8] * sizeof(PxVec4), q2v0);
			storeAligned(prevParticles, vpIt[9] * sizeof(PxVec4), q2v1);
			storeAligned(prevParticles, vpIt[10] * sizeof(PxVec4), q2v2);

			storeAligned(prevParticles, vpIt[12] * sizeof(PxVec4), q3v0);
			storeAligned(prevParticles, vpIt[13] * sizeof(PxVec4), q3v1);
			storeAligned(prevParticles, vpIt[14] * sizeof(PxVec4), q3v2);
		}

		if(massScalingEnabled)
		{
			// calculate the inverse mass scale based on the collision impulse
			Simd4f dSq = invNumCollisions * invNumCollisions *
			             (accum.mDeltaX * accum.mDeltaX + accum.mDeltaY * accum.mDeltaY + accum.mDeltaZ * accum.mDeltaZ);

			Simd4f weightScale = recip(gSimd4fOne + massScale * dSq);

			weightScale = weightScale - gSimd4fOne;
			Simd4f s0 = gSimd4fOne + splat<0>(weightScale) * (w0 & splat<0>(mask));
			Simd4f s1 = gSimd4fOne + splat<1>(weightScale) * (w1 & splat<1>(mask));
			Simd4f s2 = gSimd4fOne + splat<2>(weightScale) * (w2 & splat<2>(mask));
			Simd4f s3 = gSimd4fOne + splat<3>(weightScale) * (w3 & splat<3>(mask));

			p0v0 = p0v0 * (gSimd4fOneXYZ | (splat<0>(s0) & sMaskW));
			p0v1 = p0v1 * (gSimd4fOneXYZ | (splat<1>(s0) & sMaskW));
			p0v2 = p0v2 * (gSimd4fOneXYZ | (splat<2>(s0) & sMaskW));

			p1v0 = p1v0 * (gSimd4fOneXYZ | (splat<0>(s1) & sMaskW));
			p1v1 = p1v1 * (gSimd4fOneXYZ | (splat<1>(s1) & sMaskW));
			p1v2 = p1v2 * (gSimd4fOneXYZ | (splat<2>(s1) & sMaskW));

			p2v0 = p2v0 * (gSimd4fOneXYZ | (splat<0>(s2) & sMaskW));
			p2v1 = p2v1 * (gSimd4fOneXYZ | (splat<1>(s2) & sMaskW));
			p2v2 = p2v2 * (gSimd4fOneXYZ | (splat<2>(s2) & sMaskW));

			p3v0 = p3v0 * (gSimd4fOneXYZ | (splat<0>(s3) & sMaskW));
			p3v1 = p3v1 * (gSimd4fOneXYZ | (splat<1>(s3) & sMaskW));
			p3v2 = p3v2 * (gSimd4fOneXYZ | (splat<2>(s3) & sMaskW));
		}

		p0v0 = p0v0 + (splat<0>(rw0) * d0);
		p0v1 = p0v1 + (splat<1>(rw0) * d0);
		p0v2 = p0v2 + (splat<2>(rw0) * d0);

		p1v0 = p1v0 + (splat<0>(rw1) * d1);
		p1v1 = p1v1 + (splat<1>(rw1) * d1);
		p1v2 = p1v2 + (splat<2>(rw1) * d1);

		p2v0 = p2v0 + (splat<0>(rw2) * d2);
		p2v1 = p2v1 + (splat<1>(rw2) * d2);
		p2v2 = p2v2 + (splat<2>(rw2) * d2);

		p3v0 = p3v0 + (splat<0>(rw3) * d3);
		p3v1 = p3v1 + (splat<1>(rw3) * d3);
		p3v2 = p3v2 + (splat<2>(rw3) * d3);

		// write back particles
		storeAligned(particles, vpIt[0] * sizeof(PxVec4), p0v0);
		storeAligned(particles, vpIt[1] * sizeof(PxVec4), p0v1);
		storeAligned(particles, vpIt[2] * sizeof(PxVec4), p0v2);

		storeAligned(particles, vpIt[4] * sizeof(PxVec4), p1v0);
		storeAligned(particles, vpIt[5] * sizeof(PxVec4), p1v1);
		storeAligned(particles, vpIt[6] * sizeof(PxVec4), p1v2);

		storeAligned(particles, vpIt[8] * sizeof(PxVec4), p2v0);
		storeAligned(particles, vpIt[9] * sizeof(PxVec4), p2v1);
		storeAligned(particles, vpIt[10] * sizeof(PxVec4), p2v2);

		storeAligned(particles, vpIt[12] * sizeof(PxVec4), p3v0);
		storeAligned(particles, vpIt[13] * sizeof(PxVec4), p3v1);
		storeAligned(particles, vpIt[14] * sizeof(PxVec4), p3v2);

#if PX_PROFILE || PX_DEBUG
		mNumCollisions += horizontalSum(accum.mNumCollisions);
#endif
	}
}

template <typename Simd4f>
void cloth::SwCollision<Simd4f>::collideContinuousParticles()
{
	Simd4f curPos[4];
	Simd4f prevPos[4];

	const bool massScalingEnabled = mClothData.mCollisionMassScale > 0.0f;
	const Simd4f massScale = simd4f(mClothData.mCollisionMassScale);

	const bool frictionEnabled = mClothData.mFrictionScale > 0.0f;
	const Simd4f frictionScale = simd4f(mClothData.mFrictionScale);

	float* __restrict prevIt = mClothData.mPrevParticles;
	float* __restrict curIt = mClothData.mCurParticles;
	float* __restrict curEnd = curIt + mClothData.mNumParticles * 4;

	for(; curIt < curEnd; curIt += 16, prevIt += 16)
	{
		prevPos[0] = loadAligned(prevIt, 0);
		prevPos[1] = loadAligned(prevIt, 16);
		prevPos[2] = loadAligned(prevIt, 32);
		prevPos[3] = loadAligned(prevIt, 48);
		transpose(prevPos[0], prevPos[1], prevPos[2], prevPos[3]);

		curPos[0] = loadAligned(curIt, 0);
		curPos[1] = loadAligned(curIt, 16);
		curPos[2] = loadAligned(curIt, 32);
		curPos[3] = loadAligned(curIt, 48);
		transpose(curPos[0], curPos[1], curPos[2], curPos[3]);

		ImpulseAccumulator accum;
		Simd4i sphereMask = collideCones(prevPos, curPos, accum);
		collideSpheres(sphereMask, prevPos, curPos, accum);

		Simd4f mask;
		if(!anyGreater(accum.mNumCollisions, gSimd4fEpsilon, mask))
			continue;

		Simd4f invNumCollisions = recip(accum.mNumCollisions);

		if(frictionEnabled)
		{
			Simd4f frictionImpulse[3];
			calculateFrictionImpulse(accum.mDeltaX, accum.mDeltaY, accum.mDeltaZ, accum.mVelX, accum.mVelY, accum.mVelZ,
			                         curPos, prevPos, invNumCollisions, frictionScale, mask, frictionImpulse);

			prevPos[0] = prevPos[0] - frictionImpulse[0];
			prevPos[1] = prevPos[1] - frictionImpulse[1];
			prevPos[2] = prevPos[2] - frictionImpulse[2];

			transpose(prevPos[0], prevPos[1], prevPos[2], prevPos[3]);
			storeAligned(prevIt, 0, prevPos[0]);
			storeAligned(prevIt, 16, prevPos[1]);
			storeAligned(prevIt, 32, prevPos[2]);
			storeAligned(prevIt, 48, prevPos[3]);
		}

		if(massScalingEnabled)
		{
			// calculate the inverse mass scale based on the collision impulse magnitude
			Simd4f dSq = invNumCollisions * invNumCollisions *
			             (accum.mDeltaX * accum.mDeltaX + accum.mDeltaY * accum.mDeltaY + accum.mDeltaZ * accum.mDeltaZ);

			Simd4f weightScale = recip(gSimd4fOne + massScale * dSq);

			// scale invmass
			curPos[3] = select(mask, curPos[3] * weightScale, curPos[3]);
		}

		curPos[0] = curPos[0] + accum.mDeltaX * invNumCollisions;
		curPos[1] = curPos[1] + accum.mDeltaY * invNumCollisions;
		curPos[2] = curPos[2] + accum.mDeltaZ * invNumCollisions;

		transpose(curPos[0], curPos[1], curPos[2], curPos[3]);
		storeAligned(curIt, 0, curPos[0]);
		storeAligned(curIt, 16, curPos[1]);
		storeAligned(curIt, 32, curPos[2]);
		storeAligned(curIt, 48, curPos[3]);

#if PX_PROFILE || PX_DEBUG
		mNumCollisions += horizontalSum(accum.mNumCollisions);
#endif
	}
}

template <typename Simd4f>
void cloth::SwCollision<Simd4f>::collideConvexes(const IterationState<Simd4f>& state)
{
	if(!mClothData.mNumConvexes)
		return;

	// times 2 for plane equation result buffer
	Simd4f* planes = static_cast<Simd4f*>(mAllocator.allocate(sizeof(Simd4f) * mClothData.mNumPlanes * 2));

	const Simd4f* targetPlanes = reinterpret_cast<const Simd4f*>(mClothData.mTargetCollisionPlanes);

	// generate plane collision data
	if(state.mRemainingIterations != 1)
	{
		// interpolate planes
		LerpIterator<Simd4f, const Simd4f*> planeIter(reinterpret_cast<const Simd4f*>(mClothData.mStartCollisionPlanes),
		                                              targetPlanes, state.getCurrentAlpha());

		// todo: normalize plane equations
		generatePlanes(planes, planeIter, mClothData.mNumPlanes);
	}
	else
	{
		// otherwise use the target planes directly
		generatePlanes(planes, targetPlanes, mClothData.mNumPlanes);
	}

	Simd4f curPos[4], prevPos[4];

	const bool frictionEnabled = mClothData.mFrictionScale > 0.0f;
	const Simd4f frictionScale = simd4f(mClothData.mFrictionScale);

	float* __restrict curIt = mClothData.mCurParticles;
	float* __restrict curEnd = curIt + mClothData.mNumParticles * 4;
	float* __restrict prevIt = mClothData.mPrevParticles;
	for(; curIt < curEnd; curIt += 16, prevIt += 16)
	{
		curPos[0] = loadAligned(curIt, 0);
		curPos[1] = loadAligned(curIt, 16);
		curPos[2] = loadAligned(curIt, 32);
		curPos[3] = loadAligned(curIt, 48);
		transpose(curPos[0], curPos[1], curPos[2], curPos[3]);

		ImpulseAccumulator accum;
		collideConvexes(planes, curPos, accum);

		Simd4f mask;
		if(!anyGreater(accum.mNumCollisions, gSimd4fEpsilon, mask))
			continue;

		Simd4f invNumCollisions = recip(accum.mNumCollisions);

		if(frictionEnabled)
		{
			prevPos[0] = loadAligned(prevIt, 0);
			prevPos[1] = loadAligned(prevIt, 16);
			prevPos[2] = loadAligned(prevIt, 32);
			prevPos[3] = loadAligned(prevIt, 48);
			transpose(prevPos[0], prevPos[1], prevPos[2], prevPos[3]);

			Simd4f frictionImpulse[3];
			calculateFrictionImpulse(accum.mDeltaX, accum.mDeltaY, accum.mDeltaZ, accum.mVelX, accum.mVelY, accum.mVelZ,
			                         curPos, prevPos, invNumCollisions, frictionScale, mask, frictionImpulse);

			prevPos[0] = prevPos[0] - frictionImpulse[0];
			prevPos[1] = prevPos[1] - frictionImpulse[1];
			prevPos[2] = prevPos[2] - frictionImpulse[2];

			transpose(prevPos[0], prevPos[1], prevPos[2], prevPos[3]);
			storeAligned(prevIt, 0, prevPos[0]);
			storeAligned(prevIt, 16, prevPos[1]);
			storeAligned(prevIt, 32, prevPos[2]);
			storeAligned(prevIt, 48, prevPos[3]);
		}

		curPos[0] = curPos[0] + accum.mDeltaX * invNumCollisions;
		curPos[1] = curPos[1] + accum.mDeltaY * invNumCollisions;
		curPos[2] = curPos[2] + accum.mDeltaZ * invNumCollisions;

		transpose(curPos[0], curPos[1], curPos[2], curPos[3]);
		storeAligned(curIt, 0, curPos[0]);
		storeAligned(curIt, 16, curPos[1]);
		storeAligned(curIt, 32, curPos[2]);
		storeAligned(curIt, 48, curPos[3]);

#if PX_PROFILE || PX_DEBUG
		mNumCollisions += horizontalSum(accum.mNumCollisions);
#endif
	}

	mAllocator.deallocate(planes);
}

template <typename Simd4f>
void cloth::SwCollision<Simd4f>::collideConvexes(const Simd4f* __restrict planes, Simd4f* __restrict curPos,
                                                 ImpulseAccumulator& accum)
{
	Simd4i result = gSimd4iZero;
	Simd4i mask4 = gSimd4iOne;

	const Simd4f* __restrict pIt, *pEnd = planes + mClothData.mNumPlanes;
	Simd4f* __restrict dIt = const_cast<Simd4f*>(pEnd);
	for(pIt = planes; pIt != pEnd; ++pIt, ++dIt)
	{
		*dIt = splat<3>(*pIt) + curPos[2] * splat<2>(*pIt) + curPos[1] * splat<1>(*pIt) + curPos[0] * splat<0>(*pIt);
		result = result | (mask4 & simd4i(*dIt < gSimd4fZero));
		mask4 = mask4 << 1; // todo: shift by Simd4i on consoles
	}

	if(allEqual(result, gSimd4iZero))
		return;

	const uint32_t* __restrict cIt = mClothData.mConvexMasks;
	const uint32_t* __restrict cEnd = cIt + mClothData.mNumConvexes;
	for(; cIt != cEnd; ++cIt)
	{
		uint32_t mask = *cIt;
		mask4 = simd4i(int(mask));
		if(!anyEqual(mask4 & result, mask4, mask4))
			continue;

		uint32_t test = mask - 1;
		uint32_t planeIndex = findBitSet(mask & ~test);
		Simd4f plane = planes[planeIndex];
		Simd4f planeX = splat<0>(plane);
		Simd4f planeY = splat<1>(plane);
		Simd4f planeZ = splat<2>(plane);
		Simd4f planeD = pEnd[planeIndex];
		while(mask &= test)
		{
			test = mask - 1;
			planeIndex = findBitSet(mask & ~test);
			plane = planes[planeIndex];
			Simd4f dist = pEnd[planeIndex];
			Simd4f closer = dist > planeD;
			planeX = select(closer, splat<0>(plane), planeX);
			planeY = select(closer, splat<1>(plane), planeY);
			planeZ = select(closer, splat<2>(plane), planeZ);
			planeD = max(dist, planeD);
		}

		accum.subtract(planeX, planeY, planeZ, planeD, simd4f(mask4));
	}
}

template <typename Simd4f>
void cloth::SwCollision<Simd4f>::collideTriangles(const IterationState<Simd4f>& state)
{
	if(!mClothData.mNumCollisionTriangles)
		return;

	TriangleData* triangles =
	    static_cast<TriangleData*>(mAllocator.allocate(sizeof(TriangleData) * mClothData.mNumCollisionTriangles));

	UnalignedIterator<Simd4f, 3> targetTriangles(mClothData.mTargetCollisionTriangles);

	// generate triangle collision data
	if(state.mRemainingIterations != 1)
	{
		// interpolate triangles
		LerpIterator<Simd4f, UnalignedIterator<Simd4f, 3> > triangleIter(mClothData.mStartCollisionTriangles,
		                                                                 targetTriangles, state.getCurrentAlpha());

		generateTriangles<Simd4f>(triangles, triangleIter, mClothData.mNumCollisionTriangles);
	}
	else
	{
		// otherwise use the target triangles directly
		generateTriangles<Simd4f>(triangles, targetTriangles, mClothData.mNumCollisionTriangles);
	}

	Simd4f positions[4];

	float* __restrict pIt = mClothData.mCurParticles;
	float* __restrict pEnd = pIt + mClothData.mNumParticles * 4;
	for(; pIt < pEnd; pIt += 16)
	{
		positions[0] = loadAligned(pIt, 0);
		positions[1] = loadAligned(pIt, 16);
		positions[2] = loadAligned(pIt, 32);
		positions[3] = loadAligned(pIt, 48);
		transpose(positions[0], positions[1], positions[2], positions[3]);

		ImpulseAccumulator accum;
		collideTriangles(triangles, positions, accum);

		Simd4f mask;
		if(!anyGreater(accum.mNumCollisions, gSimd4fEpsilon, mask))
			continue;

		Simd4f invNumCollisions = recip(accum.mNumCollisions);

		positions[0] = positions[0] + accum.mDeltaX * invNumCollisions;
		positions[1] = positions[1] + accum.mDeltaY * invNumCollisions;
		positions[2] = positions[2] + accum.mDeltaZ * invNumCollisions;

		transpose(positions[0], positions[1], positions[2], positions[3]);
		storeAligned(pIt, 0, positions[0]);
		storeAligned(pIt, 16, positions[1]);
		storeAligned(pIt, 32, positions[2]);
		storeAligned(pIt, 48, positions[3]);

#if PX_PROFILE || PX_DEBUG
		mNumCollisions += horizontalSum(accum.mNumCollisions);
#endif
	}

	mAllocator.deallocate(triangles);
}

template <typename Simd4f>
void cloth::SwCollision<Simd4f>::collideTriangles(const TriangleData* __restrict triangles, Simd4f* __restrict curPos,
                                                  ImpulseAccumulator& accum)
{
	Simd4f normalX, normalY, normalZ, normalD;
	normalX = normalY = normalZ = normalD = gSimd4fZero;
	Simd4f minSqrLength = gSimd4fFloatMax;

	const TriangleData* __restrict tIt, *tEnd = triangles + mClothData.mNumCollisionTriangles;
	for(tIt = triangles; tIt != tEnd; ++tIt)
	{
		Simd4f base = loadAligned(&tIt->base.x);
		Simd4f edge0 = loadAligned(&tIt->edge0.x);
		Simd4f edge1 = loadAligned(&tIt->edge1.x);
		Simd4f normal = loadAligned(&tIt->normal.x);
		Simd4f aux = loadAligned(&tIt->det);

		Simd4f dx = curPos[0] - splat<0>(base);
		Simd4f dy = curPos[1] - splat<1>(base);
		Simd4f dz = curPos[2] - splat<2>(base);

		Simd4f e0x = splat<0>(edge0);
		Simd4f e0y = splat<1>(edge0);
		Simd4f e0z = splat<2>(edge0);

		Simd4f e1x = splat<0>(edge1);
		Simd4f e1y = splat<1>(edge1);
		Simd4f e1z = splat<2>(edge1);

		Simd4f nx = splat<0>(normal);
		Simd4f ny = splat<1>(normal);
		Simd4f nz = splat<2>(normal);

		Simd4f deltaDotEdge0 = dx * e0x + dy * e0y + dz * e0z;
		Simd4f deltaDotEdge1 = dx * e1x + dy * e1y + dz * e1z;
		Simd4f deltaDotNormal = dx * nx + dy * ny + dz * nz;

		Simd4f edge0DotEdge1 = splat<3>(base);
		Simd4f edge0SqrLength = splat<3>(edge0);
		Simd4f edge1SqrLength = splat<3>(edge1);

		Simd4f s = edge1SqrLength * deltaDotEdge0 - edge0DotEdge1 * deltaDotEdge1;
		Simd4f t = edge0SqrLength * deltaDotEdge1 - edge0DotEdge1 * deltaDotEdge0;

		Simd4f sPositive = s > gSimd4fZero;
		Simd4f tPositive = t > gSimd4fZero;

		Simd4f det = splat<0>(aux);

		s = select(tPositive, s * det, deltaDotEdge0 * splat<2>(aux));
		t = select(sPositive, t * det, deltaDotEdge1 * splat<3>(aux));

		Simd4f clamp = gSimd4fOne < s + t;
		Simd4f numerator = edge1SqrLength - edge0DotEdge1 + deltaDotEdge0 - deltaDotEdge1;

		s = select(clamp, numerator * splat<1>(aux), s);

		s = max(gSimd4fZero, min(gSimd4fOne, s));
		t = max(gSimd4fZero, min(gSimd4fOne - s, t));

		dx = dx - e0x * s - e1x * t;
		dy = dy - e0y * s - e1y * t;
		dz = dz - e0z * s - e1z * t;

		Simd4f sqrLength = dx * dx + dy * dy + dz * dz;

		// slightly increase distance for colliding triangles
		Simd4f slack = (gSimd4fZero > deltaDotNormal) & simd4f(1e-4f);
		sqrLength = sqrLength + sqrLength * slack;

		Simd4f mask = sqrLength < minSqrLength;

		normalX = select(mask, nx, normalX);
		normalY = select(mask, ny, normalY);
		normalZ = select(mask, nz, normalZ);
		normalD = select(mask, deltaDotNormal, normalD);

		minSqrLength = min(sqrLength, minSqrLength);
	}

	Simd4f mask;
	if(!anyGreater(gSimd4fZero, normalD, mask))
		return;

	accum.subtract(normalX, normalY, normalZ, normalD, mask);
}

// explicit template instantiation
#if NV_SIMD_SIMD
template class cloth::SwCollision<Simd4f>;
#endif
#if NV_SIMD_SCALAR
template class cloth::SwCollision<Scalar4f>;
#endif

/*
namespace
{
    using namespace cloth;

    int test()
    {
        Simd4f vertices[] = {
            simd4f(0.0f, 0.0f, 0.0f, 0.0f),
            simd4f(0.1f, 0.0f, 0.0f, 0.0f),
            simd4f(0.0f, 0.1f, 0.0f, 0.0f)
        };
        TriangleData triangle;
        generateTriangles<Simd4f>(&triangle, &*vertices, 1);

        char buffer[1000];
        SwKernelAllocator alloc(buffer, 1000);

        SwClothData* cloth = static_cast<SwClothData*>(malloc(sizeof(SwClothData)));
        memset(cloth, 0, sizeof(SwClothData));
        cloth->mNumTriangles = 1;

        SwCollision<Simd4f> collision(*cloth, alloc);
        SwCollision<Simd4f>::ImpulseAccumulator accum;

        Simd4f particles[4] = {};
        for(float y=-0.1f; y < 0.0f; y += 0.2f)
        {
            for(float x=-0.1f; x < 0.0f; x += 0.2f)
            {
                particles[0] = simd4f(x);
                particles[1] = simd4f(y);
                particles[2] = simd4f(-1.0f);

                collision.collideTriangles(&triangle, particles, accum);
            }
        }

        return 0;
    }

    static int blah = test();
}
*/
