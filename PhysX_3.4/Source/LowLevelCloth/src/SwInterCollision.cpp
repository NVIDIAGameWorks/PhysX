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
#include "foundation/PxMemory.h"
#include "SwInterCollision.h"
#include "SwCollisionHelpers.h"
#include "BoundingBox.h"
#include "PsIntrinsics.h"
#include "PsSort.h"

using namespace physx;

namespace
{

const Simd4fTupleFactory sMaskXYZ = simd4f(simd4i(~0, ~0, ~0, 0));
const Simd4fTupleFactory sMaskW = simd4f(simd4i(0, 0, 0, ~0));
const Simd4fScalarFactory sEpsilon = simd4f(FLT_EPSILON);
const Simd4fTupleFactory sZeroW = simd4f(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f);

// returns sorted indices, output needs to be at least 2*(last-first)+1024
void radixSort(const uint32_t* first, const uint32_t* last, uint32_t* out)
{
	uint32_t n = uint32_t(last - first);

	uint32_t* buffer = out + 2 * n;
	uint32_t* __restrict histograms[] = { buffer, buffer + 256, buffer + 512, buffer + 768 };

	PxMemZero(buffer, 1024 * sizeof(uint32_t));

	// build 3 histograms in one pass
	for(const uint32_t* __restrict it = first; it != last; ++it)
	{
		uint32_t key = *it;
		++histograms[0][0xff & key];
		++histograms[1][0xff & (key >> 8)];
		++histograms[2][0xff & (key >> 16)];
		++histograms[3][key >> 24];
	}

	// convert histograms to offset tables in-place
	uint32_t sums[4] = {};
	for(uint32_t i = 0; i < 256; ++i)
	{
		uint32_t temp0 = histograms[0][i] + sums[0];
		histograms[0][i] = sums[0];
		sums[0] = temp0;

		uint32_t temp1 = histograms[1][i] + sums[1];
		histograms[1][i] = sums[1];
		sums[1] = temp1;

		uint32_t temp2 = histograms[2][i] + sums[2];
		histograms[2][i] = sums[2];
		sums[2] = temp2;

		uint32_t temp3 = histograms[3][i] + sums[3];
		histograms[3][i] = sums[3];
		sums[3] = temp3;
	}

	PX_ASSERT(sums[0] == n && sums[1] == n && sums[2] == n && sums[3] == n);

#if PX_DEBUG
	memset(out, 0xff, 2 * n * sizeof(uint32_t));
#endif

	// sort 8 bits per pass

	uint32_t* __restrict indices[] = { out, out + n };

	for(uint32_t i = 0; i != n; ++i)
		indices[1][histograms[0][0xff & first[i]]++] = i;

	for(uint32_t i = 0, index; i != n; ++i)
	{
		index = indices[1][i];
		indices[0][histograms[1][0xff & (first[index] >> 8)]++] = index;
	}

	for(uint32_t i = 0, index; i != n; ++i)
	{
		index = indices[0][i];
		indices[1][histograms[2][0xff & (first[index] >> 16)]++] = index;
	}

	for(uint32_t i = 0, index; i != n; ++i)
	{
		index = indices[1][i];
		indices[0][histograms[3][first[index] >> 24]++] = index;
	}
}

template <typename Simd4f>
uint32_t longestAxis(const Simd4f& edgeLength)
{
	const float* e = array(edgeLength);

	if(e[0] > e[1])
		return uint32_t(e[0] > e[2] ? 0 : 2);
	else
		return uint32_t(e[1] > e[2] ? 1 : 2);
}
}

template <typename Simd4f>
cloth::SwInterCollision<Simd4f>::SwInterCollision(const cloth::SwInterCollisionData* instances, uint32_t n,
                                                  float colDist, float stiffness, uint32_t iterations,
                                                  InterCollisionFilter filter, cloth::SwKernelAllocator& alloc)
: mInstances(instances)
, mNumInstances(n)
, mClothIndices(NULL)
, mParticleIndices(NULL)
, mNumParticles(0)
, mTotalParticles(0)
, mFilter(filter)
, mAllocator(alloc)
{
	PX_ASSERT(mFilter);

	mCollisionDistance = simd4f(colDist, colDist, colDist, 0.0f);
	mCollisionSquareDistance = mCollisionDistance * mCollisionDistance;
	mStiffness = simd4f(stiffness);
	mNumIterations = iterations;

	// calculate particle size
	for(uint32_t i = 0; i < n; ++i)
		mTotalParticles += instances[i].mNumParticles;
}

template <typename Simd4f>
cloth::SwInterCollision<Simd4f>::~SwInterCollision()
{
}

namespace
{
// multiple x by m leaving w component of x intact
template <typename Simd4f>
PX_INLINE Simd4f transform(const Simd4f m[4], const Simd4f& x)
{
	const Simd4f a = m[3] + splat<0>(x) * m[0] + splat<1>(x) * m[1] + splat<2>(x) * m[2];
	return select(sMaskXYZ, a, x);
}

// rotate x by m leaving w component intact
template <typename Simd4f>
PX_INLINE Simd4f rotate(const Simd4f m[4], const Simd4f& x)
{
	const Simd4f a = splat<0>(x) * m[0] + splat<1>(x) * m[1] + splat<2>(x) * m[2];
	return select(sMaskXYZ, a, x);
}

template <typename Simd4f>
struct ClothSorter
{
	typedef cloth::BoundingBox<Simd4f> BoundingBox;

	ClothSorter(BoundingBox* bounds, uint32_t n, uint32_t axis) : mBounds(bounds), mNumBounds(n), mAxis(axis)
	{
	}

	bool operator()(uint32_t i, uint32_t j) const
	{
		PX_ASSERT(i < mNumBounds);
		PX_ASSERT(j < mNumBounds);

		return array(mBounds[i].mLower)[mAxis] < array(mBounds[j].mLower)[mAxis];
	}

	BoundingBox* mBounds;
	uint32_t mNumBounds;
	uint32_t mAxis;
};

// for the given cloth array this function calculates the set of particles
// which potentially interact, the potential colliders are returned with their
// cloth index and particle index in clothIndices and particleIndices, the
// function returns the number of potential colliders
template <typename Simd4f>
uint32_t calculatePotentialColliders(const cloth::SwInterCollisionData* cBegin, const cloth::SwInterCollisionData* cEnd,
                                     const Simd4f& colDist, uint16_t* clothIndices, uint32_t* particleIndices,
                                     cloth::BoundingBox<Simd4f>& bounds, uint32_t* overlapMasks,
                                     cloth::InterCollisionFilter filter, cloth::SwKernelAllocator& allocator)
{
	using namespace cloth;

	typedef BoundingBox<Simd4f> BoundingBox;

	uint32_t numParticles = 0;
	const uint32_t numCloths = uint32_t(cEnd - cBegin);

	// bounds of each cloth objects in world space
	BoundingBox* const clothBounds = static_cast<BoundingBox*>(allocator.allocate(numCloths * sizeof(BoundingBox)));
	BoundingBox* const overlapBounds = static_cast<BoundingBox*>(allocator.allocate(numCloths * sizeof(BoundingBox)));

	// union of all cloth world bounds
	BoundingBox totalClothBounds = emptyBounds<Simd4f>();

	uint32_t* sortedIndices = static_cast<uint32_t*>(allocator.allocate(numCloths * sizeof(uint32_t)));

	for(uint32_t i = 0; i < numCloths; ++i)
	{
		const SwInterCollisionData& c = cBegin[i];

		// transform bounds from b local space to local space of a
		PxBounds3 lcBounds = PxBounds3::centerExtents(c.mBoundsCenter, c.mBoundsHalfExtent + PxVec3(array(colDist)[0]));
		PX_ASSERT(!lcBounds.isEmpty());
		PxBounds3 cWorld = PxBounds3::transformFast(c.mGlobalPose, lcBounds);

		BoundingBox cBounds = { simd4f(cWorld.minimum.x, cWorld.minimum.y, cWorld.minimum.z, 0.0f),
			                    simd4f(cWorld.maximum.x, cWorld.maximum.y, cWorld.maximum.z, 0.0f) };

		sortedIndices[i] = i;
		clothBounds[i] = cBounds;

		totalClothBounds = expandBounds(totalClothBounds, cBounds);
	}

	// sort indices by their minimum extent on the longest axis
	const uint32_t sweepAxis = longestAxis(totalClothBounds.mUpper - totalClothBounds.mLower);

	ClothSorter<Simd4f> predicate(clothBounds, numCloths, sweepAxis);
	shdfnd::sort(sortedIndices, numCloths, predicate);

	for(uint32_t i = 0; i < numCloths; ++i)
	{
		PX_ASSERT(sortedIndices[i] < numCloths);

		const SwInterCollisionData& a = cBegin[sortedIndices[i]];

		// local bounds
		const Simd4f aCenter = load(reinterpret_cast<const float*>(&a.mBoundsCenter));
		const Simd4f aHalfExtent = load(reinterpret_cast<const float*>(&a.mBoundsHalfExtent)) + colDist;
		const BoundingBox aBounds = { aCenter - aHalfExtent, aCenter + aHalfExtent };

		const PxMat44 aToWorld(a.mGlobalPose);
		const PxTransform aToLocal(a.mGlobalPose.getInverse());

		const float axisMin = array(clothBounds[sortedIndices[i]].mLower)[sweepAxis];
		const float axisMax = array(clothBounds[sortedIndices[i]].mUpper)[sweepAxis];

		uint32_t overlapMask = 0;
		uint32_t numOverlaps = 0;

		// scan back to find first intersecting bounding box
		uint32_t startIndex = i;
		while(startIndex > 0 && array(clothBounds[sortedIndices[startIndex]].mUpper)[sweepAxis] > axisMin)
			--startIndex;

		// compute all overlapping bounds
		for(uint32_t j = startIndex; j < numCloths; ++j)
		{
			// ignore self-collision
			if(i == j)
				continue;

			// early out if no more cloths along axis intersect us
			if(array(clothBounds[sortedIndices[j]].mLower)[sweepAxis] > axisMax)
				break;

			const SwInterCollisionData& b = cBegin[sortedIndices[j]];

			// check if collision between these shapes is filtered
			if(!filter(a.mUserData, b.mUserData))
				continue;

			// set mask bit for this cloth
			overlapMask |= 1 << sortedIndices[j];

			// transform bounds from b local space to local space of a
			PxBounds3 lcBounds =
			    PxBounds3::centerExtents(b.mBoundsCenter, b.mBoundsHalfExtent + PxVec3(array(colDist)[0]));
			PX_ASSERT(!lcBounds.isEmpty());
			PxBounds3 bLocal = PxBounds3::transformFast(aToLocal * b.mGlobalPose, lcBounds);

			BoundingBox bBounds = { simd4f(bLocal.minimum.x, bLocal.minimum.y, bLocal.minimum.z, 0.0f),
				                    simd4f(bLocal.maximum.x, bLocal.maximum.y, bLocal.maximum.z, 0.0f) };

			BoundingBox iBounds = intersectBounds(aBounds, bBounds);

			// setup bounding box w to make point containment test cheaper
			Simd4f floatMax = gSimd4fFloatMax & static_cast<Simd4f>(sMaskW);
			iBounds.mLower = (iBounds.mLower & sMaskXYZ) | -floatMax;
			iBounds.mUpper = (iBounds.mUpper & sMaskXYZ) | floatMax;

			if(!isEmptyBounds(iBounds))
				overlapBounds[numOverlaps++] = iBounds;
		}

		//----------------------------------------------------------------
		// cull all particles to overlapping bounds and transform particles to world space

		const uint32_t clothIndex = sortedIndices[i];
		overlapMasks[clothIndex] = overlapMask;

		Simd4f* pBegin = reinterpret_cast<Simd4f*>(a.mParticles);
		Simd4f* qBegin = reinterpret_cast<Simd4f*>(a.mPrevParticles);

		const Simd4f xform[4] = { load(reinterpret_cast<const float*>(&aToWorld.column0)),
			                      load(reinterpret_cast<const float*>(&aToWorld.column1)),
			                      load(reinterpret_cast<const float*>(&aToWorld.column2)),
			                      load(reinterpret_cast<const float*>(&aToWorld.column3)) };

		Simd4f impulseInvScale = recip(Simd4f(simd4f(cBegin[clothIndex].mImpulseScale)));

		for(uint32_t k = 0; k < a.mNumParticles; ++k)
		{
			Simd4f* pIt = a.mIndices ? pBegin + a.mIndices[k] : pBegin + k;
			Simd4f* qIt = a.mIndices ? qBegin + a.mIndices[k] : qBegin + k;

			const Simd4f p = *pIt;

			for(const BoundingBox* oIt = overlapBounds, *oEnd = overlapBounds + numOverlaps; oIt != oEnd; ++oIt)
			{
				// point in box test
				if(anyGreater(oIt->mLower, p) != 0)
					continue;
				if(anyGreater(p, oIt->mUpper) != 0)
					continue;

				// transform particle to world space in-place
				// (will be transformed back after collision)
				*pIt = transform(xform, p);

				Simd4f impulse = (p - *qIt) * impulseInvScale;
				*qIt = rotate(xform, impulse);

				// update world bounds
				bounds = expandBounds(bounds, pIt, pIt + 1);

				// add particle to output arrays
				clothIndices[numParticles] = uint16_t(clothIndex);
				particleIndices[numParticles] = uint32_t(pIt - pBegin);

				// output each particle only once
				++numParticles;
				break;
			}
		}
	}

	allocator.deallocate(sortedIndices);
	allocator.deallocate(overlapBounds);
	allocator.deallocate(clothBounds);

	return numParticles;
}
}

template <typename Simd4f>
PX_INLINE Simd4f& cloth::SwInterCollision<Simd4f>::getParticle(uint32_t index)
{
	PX_ASSERT(index < mNumParticles);

	uint16_t clothIndex = mClothIndices[index];
	uint32_t particleIndex = mParticleIndices[index];

	PX_ASSERT(clothIndex < mNumInstances);

	return reinterpret_cast<Simd4f&>(mInstances[clothIndex].mParticles[particleIndex]);
}

template <typename Simd4f>
void cloth::SwInterCollision<Simd4f>::operator()()
{
	mNumTests = mNumCollisions = 0;

	mClothIndices = static_cast<uint16_t*>(mAllocator.allocate(sizeof(uint16_t) * mTotalParticles));
	mParticleIndices = static_cast<uint32_t*>(mAllocator.allocate(sizeof(uint32_t) * mTotalParticles));
	mOverlapMasks = static_cast<uint32_t*>(mAllocator.allocate(sizeof(uint32_t*) * mNumInstances));

	for(uint32_t k = 0; k < mNumIterations; ++k)
	{
		// world bounds of particles
		BoundingBox<Simd4f> bounds = emptyBounds<Simd4f>();

		// calculate potentially colliding set
		{
			PX_PROFILE_ZONE("cloth::SwInterCollision::BroadPhase", 0);

			mNumParticles =
			    calculatePotentialColliders(mInstances, mInstances + mNumInstances, mCollisionDistance, mClothIndices,
			                                mParticleIndices, bounds, mOverlapMasks, mFilter, mAllocator);
		}

		// collide
		if(mNumParticles)
		{
			PX_PROFILE_ZONE("cloth::SwInterCollision::Collide", 0);

			Simd4f lowerBound = bounds.mLower;
			Simd4f edgeLength = max(bounds.mUpper - lowerBound, sEpsilon);

			// sweep along longest axis
			uint32_t sweepAxis = longestAxis(edgeLength);
			uint32_t hashAxis0 = (sweepAxis + 1) % 3;
			uint32_t hashAxis1 = (sweepAxis + 2) % 3;

			// reserve 0, 127, and 65535 for sentinel
			Simd4f cellSize = max(mCollisionDistance, simd4f(1.0f / 253) * edgeLength);
			array(cellSize)[sweepAxis] = array(edgeLength)[sweepAxis] / 65533;

			Simd4f one = gSimd4fOne;
			Simd4f gridSize = simd4f(254.0f);
			array(gridSize)[sweepAxis] = 65534.0f;

			Simd4f gridScale = recip<1>(cellSize);
			Simd4f gridBias = -lowerBound * gridScale + one;

			void* buffer = mAllocator.allocate(getBufferSize(mNumParticles));

			uint32_t* __restrict sortedIndices = reinterpret_cast<uint32_t*>(buffer);
			uint32_t* __restrict sortedKeys = sortedIndices + mNumParticles;
			uint32_t* __restrict keys = PxMax(sortedKeys + mNumParticles, sortedIndices + 2 * mNumParticles + 1024);

			typedef typename Simd4fToSimd4i<Simd4f>::Type Simd4i;

			// create keys
			for(uint32_t i = 0; i < mNumParticles; ++i)
			{
				// grid coordinate
				Simd4f indexf = getParticle(i) * gridScale + gridBias;

				// need to clamp index because shape collision potentially
				// pushes particles outside of their original bounds
				Simd4i indexi = intFloor(max(one, min(indexf, gridSize)));

				const int32_t* ptr = array(indexi);
				keys[i] = uint32_t(ptr[sweepAxis] | (ptr[hashAxis0] << 16) | (ptr[hashAxis1] << 24));
			}

			// compute sorted keys indices
			radixSort(keys, keys + mNumParticles, sortedIndices);

			// snoop histogram: offset of first index with 8 msb > 1 (0 is sentinel)
			uint32_t firstColumnSize = sortedIndices[2 * mNumParticles + 769];

			// sort keys
			for(uint32_t i = 0; i < mNumParticles; ++i)
				sortedKeys[i] = keys[sortedIndices[i]];
			sortedKeys[mNumParticles] = uint32_t(-1); // sentinel

			// calculate the number of buckets we need to search forward
			const Simd4i data = intFloor(gridScale * mCollisionDistance);
			uint32_t collisionDistance = uint32_t(2 + array(data)[sweepAxis]);

			// collide particles
			collideParticles(sortedKeys, firstColumnSize, sortedIndices, mNumParticles, collisionDistance);

			mAllocator.deallocate(buffer);
		}

		/*
		// verify against brute force (disable collision response when testing)
		uint32_t numCollisions = mNumCollisions;
		mNumCollisions = 0;

		for(uint32_t i = 0; i < mNumParticles; ++i)
		    for(uint32_t j = i+1; j < mNumParticles; ++j)
		        if (mOverlapMasks[mClothIndices[i]] & (1 << mClothIndices[j]))
		            collideParticles(getParticle(i), getParticle(j));

		static uint32_t iter = 0; ++iter;
		if(numCollisions != mNumCollisions)
		    printf("%u: %u != %u\n", iter, numCollisions, mNumCollisions);
		*/

		// transform back to local space
		{
			PX_PROFILE_ZONE("cloth::SwInterCollision::PostTransform", 0);

			Simd4f toLocal[4], impulseScale;
			uint16_t lastCloth = uint16_t(0xffff);

			for(uint32_t i = 0; i < mNumParticles; ++i)
			{
				uint16_t clothIndex = mClothIndices[i];
				const SwInterCollisionData* instance = mInstances + clothIndex;

				// todo: could pre-compute these inverses
				if(clothIndex != lastCloth)
				{
					const PxMat44 xform(instance->mGlobalPose.getInverse());

					toLocal[0] = load(reinterpret_cast<const float*>(&xform.column0));
					toLocal[1] = load(reinterpret_cast<const float*>(&xform.column1));
					toLocal[2] = load(reinterpret_cast<const float*>(&xform.column2));
					toLocal[3] = load(reinterpret_cast<const float*>(&xform.column3));

					impulseScale = simd4f(instance->mImpulseScale);

					lastCloth = mClothIndices[i];
				}

				uint32_t particleIndex = mParticleIndices[i];
				Simd4f& particle = reinterpret_cast<Simd4f&>(instance->mParticles[particleIndex]);
				Simd4f& impulse = reinterpret_cast<Simd4f&>(instance->mPrevParticles[particleIndex]);

				particle = transform(toLocal, particle);
				// avoid w becoming negative due to numerical inaccuracies
				impulse = max(sZeroW, particle - rotate(toLocal, Simd4f(impulse * impulseScale)));
			}
		}
	}

	mAllocator.deallocate(mOverlapMasks);
	mAllocator.deallocate(mParticleIndices);
	mAllocator.deallocate(mClothIndices);
}

template <typename Simd4f>
size_t cloth::SwInterCollision<Simd4f>::estimateTemporaryMemory(SwInterCollisionData* cloths, uint32_t n)
{
	// count total particles
	uint32_t numParticles = 0;
	for(uint32_t i = 0; i < n; ++i)
		numParticles += cloths[i].mNumParticles;

	uint32_t boundsSize = 2 * n * sizeof(BoundingBox<Simd4f>) + n * sizeof(uint32_t);
	uint32_t clothIndicesSize = numParticles * sizeof(uint16_t);
	uint32_t particleIndicesSize = numParticles * sizeof(uint32_t);
	uint32_t masksSize = n * sizeof(uint32_t);

	return boundsSize + clothIndicesSize + particleIndicesSize + masksSize + getBufferSize(numParticles);
}

template <typename Simd4f>
size_t physx::cloth::SwInterCollision<Simd4f>::getBufferSize(uint32_t numParticles)
{
	uint32_t keysSize = numParticles * sizeof(uint32_t);
	uint32_t indicesSize = numParticles * sizeof(uint32_t);
	uint32_t histogramSize = 1024 * sizeof(uint32_t);

	return keysSize + indicesSize + PxMax(indicesSize + histogramSize, keysSize);
}

template <typename Simd4f>
void cloth::SwInterCollision<Simd4f>::collideParticle(uint32_t index)
{
	uint16_t clothIndex = mClothIndices[index];

	if((1 << clothIndex) & ~mClothMask)
		return;

	const SwInterCollisionData* instance = mInstances + clothIndex;

	uint32_t particleIndex = mParticleIndices[index];
	Simd4f& particle = reinterpret_cast<Simd4f&>(instance->mParticles[particleIndex]);

	Simd4f diff = particle - mParticle;
	Simd4f distSqr = dot3(diff, diff);

#if PX_DEBUG
	++mNumTests;
#endif

	if(allGreater(distSqr, mCollisionSquareDistance))
		return;

	Simd4f w0 = splat<3>(mParticle);
	Simd4f w1 = splat<3>(particle);

	Simd4f ratio = mCollisionDistance * rsqrt<1>(distSqr);
	Simd4f scale = mStiffness * recip<1>(sEpsilon + w0 + w1);
	Simd4f delta = (scale * (diff - diff * ratio)) & sMaskXYZ;

	mParticle = mParticle + delta * w0;
	particle = particle - delta * w1;

	Simd4f& impulse = reinterpret_cast<Simd4f&>(instance->mPrevParticles[particleIndex]);

	mImpulse = mImpulse + delta * w0;
	impulse = impulse - delta * w1;

#if PX_DEBUG || PX_PROFILE
	++mNumCollisions;
#endif
}

template <typename Simd4f>
void cloth::SwInterCollision<Simd4f>::collideParticles(const uint32_t* keys, uint32_t firstColumnSize,
                                                       const uint32_t* indices, uint32_t numParticles,
                                                       uint32_t collisionDistance)
{
	const uint32_t bucketMask = uint16_t(-1);

	const uint32_t keyOffsets[] = { 0, 0x00010000, 0x00ff0000, 0x01000000, 0x01010000 };

	const uint32_t* __restrict kFirst[5];
	const uint32_t* __restrict kLast[5];

	{
		// optimization: scan forward iterator starting points once instead of 9 times
		const uint32_t* __restrict kIt = keys;

		uint32_t key = *kIt;
		uint32_t firstKey = key - PxMin(collisionDistance, key & bucketMask);
		uint32_t lastKey = PxMin(key + collisionDistance, key | bucketMask);

		kFirst[0] = kIt;
		while(*kIt < lastKey)
			++kIt;
		kLast[0] = kIt;

		for(uint32_t k = 1; k < 5; ++k)
		{
			for(uint32_t n = firstKey + keyOffsets[k]; *kIt < n;)
				++kIt;
			kFirst[k] = kIt;

			for(uint32_t n = lastKey + keyOffsets[k]; *kIt < n;)
				++kIt;
			kLast[k] = kIt;

			// jump forward once to second column
			kIt = keys + firstColumnSize;
			firstColumnSize = 0;
		}
	}

	const uint32_t* __restrict iIt = indices;
	const uint32_t* __restrict iEnd = indices + numParticles;

	const uint32_t* __restrict jIt;
	const uint32_t* __restrict jEnd;

	for(; iIt != iEnd; ++iIt, ++kFirst[0])
	{
		// load current particle once outside of inner loop
		uint32_t index = *iIt;
		PX_ASSERT(index < mNumParticles);
		mClothIndex = mClothIndices[index];
		PX_ASSERT(mClothIndex < mNumInstances);
		mClothMask = mOverlapMasks[mClothIndex];

		const SwInterCollisionData* instance = mInstances + mClothIndex;

		mParticleIndex = mParticleIndices[index];
		mParticle = reinterpret_cast<const Simd4f&>(instance->mParticles[mParticleIndex]);
		mImpulse = reinterpret_cast<const Simd4f&>(instance->mPrevParticles[mParticleIndex]);

		uint32_t key = *kFirst[0];

		// range of keys we need to check against for this particle
		uint32_t firstKey = key - PxMin(collisionDistance, key & bucketMask);
		uint32_t lastKey = PxMin(key + collisionDistance, key | bucketMask);

		// scan forward end point
		while(*kLast[0] < lastKey)
			++kLast[0];

		// process potential colliders of same cell
		jEnd = indices + (kLast[0] - keys);
		for(jIt = iIt + 1; jIt != jEnd; ++jIt)
			collideParticle(*jIt);

		// process neighbor cells
		for(uint32_t k = 1; k < 5; ++k)
		{
			// scan forward start point
			for(uint32_t n = firstKey + keyOffsets[k]; *kFirst[k] < n;)
				++kFirst[k];

			// scan forward end point
			for(uint32_t n = lastKey + keyOffsets[k]; *kLast[k] < n;)
				++kLast[k];

			// process potential colliders
			jEnd = indices + (kLast[k] - keys);
			for(jIt = indices + (kFirst[k] - keys); jIt != jEnd; ++jIt)
				collideParticle(*jIt);
		}

		// write back particle and impulse
		reinterpret_cast<Simd4f&>(instance->mParticles[mParticleIndex]) = mParticle;
		reinterpret_cast<Simd4f&>(instance->mPrevParticles[mParticleIndex]) = mImpulse;
	}
}

// explicit template instantiation
#if NV_SIMD_SIMD
template class cloth::SwInterCollision<Simd4f>;
#endif
#if NV_SIMD_SCALAR
template class cloth::SwInterCollision<Scalar4f>;
#endif
