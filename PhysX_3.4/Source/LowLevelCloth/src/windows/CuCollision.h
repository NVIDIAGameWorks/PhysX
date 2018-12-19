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

#ifndef CU_SOLVER_KERNEL_CU
#error include CuCollision.h only from CuSolverKernel.cu
#endif

#include "IndexPair.h"

namespace
{
struct CuCollision
{
	struct ShapeMask
	{
		uint32_t mSpheres;
		uint32_t mCones;

		__device__ friend ShapeMask& operator&=(ShapeMask& left, const ShapeMask& right)
		{
			left.mSpheres = left.mSpheres & right.mSpheres;
			left.mCones = left.mCones & right.mCones;
			return left;
		}
	};

	struct CollisionData
	{
		Pointer<Shared, float> mSphereX;
		Pointer<Shared, float> mSphereY;
		Pointer<Shared, float> mSphereZ;
		Pointer<Shared, float> mSphereW;

		Pointer<Shared, float> mConeCenterX;
		Pointer<Shared, float> mConeCenterY;
		Pointer<Shared, float> mConeCenterZ;
		Pointer<Shared, float> mConeRadius;
		Pointer<Shared, float> mConeAxisX;
		Pointer<Shared, float> mConeAxisY;
		Pointer<Shared, float> mConeAxisZ;
		Pointer<Shared, float> mConeSlope;
		Pointer<Shared, float> mConeSqrCosine;
		Pointer<Shared, float> mConeHalfLength;
	};

  public:
	__device__ CuCollision(Pointer<Shared, uint32_t>);

	template <typename CurrentT, typename PreviousT>
	__device__ void operator()(CurrentT& current, PreviousT& previous, float alpha);

  private:
	__device__ void buildSphereAcceleration(const CollisionData&);
	__device__ void buildConeAcceleration();
	__device__ void mergeAcceleration();

	template <typename CurrentT>
	__device__ bool buildAcceleration(const CurrentT&, float);

	__device__ static ShapeMask readShapeMask(const float&, Pointer<Shared, const uint32_t>);
	template <typename CurPos>
	__device__ ShapeMask getShapeMask(const CurPos&) const;
	template <typename PrevPos, typename CurPos>
	__device__ ShapeMask getShapeMask(const PrevPos&, const CurPos&) const;

	template <typename CurPos>
	__device__ int32_t collideCapsules(const CurPos&, float3&, float3&) const;
	template <typename PrevPos, typename CurPos>
	__device__ int32_t collideCapsules(const PrevPos&, CurPos&, float3&, float3&) const;

	template <typename CurrentT, typename PreviousT>
	__device__ void collideCapsules(CurrentT& current, PreviousT& previous) const;
	template <typename CurrentT, typename PreviousT>
	__device__ void collideVirtualCapsules(CurrentT& current, PreviousT& previous) const;
	template <typename CurrentT, typename PreviousT>
	__device__ void collideContinuousCapsules(CurrentT& current, PreviousT& previous) const;

	template <typename CurrentT, typename PreviousT>
	__device__ void collideConvexes(CurrentT& current, PreviousT& previous, float alpha);
	template <typename CurPos>
	__device__ int32_t collideConvexes(const CurPos&, float3&) const;

	template <typename CurrentT>
	__device__ void collideTriangles(CurrentT& current, float alpha);
	template <typename CurrentT>
	__device__ void collideTriangles(CurrentT& current, int32_t i);

  public:
	Pointer<Shared, uint32_t> mCapsuleIndices;
	Pointer<Shared, uint32_t> mCapsuleMasks;
	Pointer<Shared, uint32_t> mConvexMasks;

	CollisionData mPrevData;
	CollisionData mCurData;

	// acceleration structure
	Pointer<Shared, uint32_t> mShapeGrid;
	float mGridScale[3];
	float mGridBias[3];
	static const uint32_t sGridSize = 8;
};

template <typename T>
__device__ void swap(T& a, T& b)
{
	T c = a;
	a = b;
	b = c;
}
}

__shared__ uninitialized<CuCollision> gCollideParticles;

namespace
{
// initializes one pointer past data!
__device__ void allocate(CuCollision::CollisionData& data)
{
	if(threadIdx.x < 15)
	{
		Pointer<Shared, float>* ptr = &data.mSphereX;
		ptr[threadIdx.x] = *ptr + threadIdx.x * gClothData.mNumCapsules +
		                   min(threadIdx.x, 4) * (gClothData.mNumSpheres - gClothData.mNumCapsules);
	}
}

__device__ void generateSpheres(CuCollision::CollisionData& data, float alpha)
{
	// interpolate spheres and transpose
	if(threadIdx.x < gClothData.mNumSpheres * 4)
	{
		float start = __ldg(gFrameData.mStartCollisionSpheres + threadIdx.x);
		float target = __ldg(gFrameData.mTargetCollisionSpheres + threadIdx.x);
		float value = start + (target - start) * alpha;
		if(threadIdx.x % 4 == 3)
			value = max(value, 0.0f);
		int32_t j = threadIdx.x % 4 * gClothData.mNumSpheres + threadIdx.x / 4;
		data.mSphereX[j] = value;
	}

	__syncthreads();
}

__device__ void generateCones(CuCollision::CollisionData& data, Pointer<Shared, const uint32_t> iIt)
{
	// generate cones
	if(threadIdx.x < gClothData.mNumCapsules)
	{
		uint32_t firstIndex = iIt[0];
		uint32_t secondIndex = iIt[1];

		float firstX = data.mSphereX[firstIndex];
		float firstY = data.mSphereY[firstIndex];
		float firstZ = data.mSphereZ[firstIndex];
		float firstW = data.mSphereW[firstIndex];

		float secondX = data.mSphereX[secondIndex];
		float secondY = data.mSphereY[secondIndex];
		float secondZ = data.mSphereZ[secondIndex];
		float secondW = data.mSphereW[secondIndex];

		float axisX = (secondX - firstX) * 0.5f;
		float axisY = (secondY - firstY) * 0.5f;
		float axisZ = (secondZ - firstZ) * 0.5f;
		float axisW = (secondW - firstW) * 0.5f;

		float sqrAxisLength = axisX * axisX + axisY * axisY + axisZ * axisZ;
		float sqrConeLength = sqrAxisLength - axisW * axisW;

		float invAxisLength = rsqrtf(sqrAxisLength);
		float invConeLength = rsqrtf(sqrConeLength);

		if(sqrConeLength <= 0.0f)
			invAxisLength = invConeLength = 0.0f;

		float axisLength = sqrAxisLength * invAxisLength;

		data.mConeCenterX[threadIdx.x] = (secondX + firstX) * 0.5f;
		data.mConeCenterY[threadIdx.x] = (secondY + firstY) * 0.5f;
		data.mConeCenterZ[threadIdx.x] = (secondZ + firstZ) * 0.5f;
		data.mConeRadius[threadIdx.x] = (axisW + firstW) * invConeLength * axisLength;

		data.mConeAxisX[threadIdx.x] = axisX * invAxisLength;
		data.mConeAxisY[threadIdx.x] = axisY * invAxisLength;
		data.mConeAxisZ[threadIdx.x] = axisZ * invAxisLength;
		data.mConeSlope[threadIdx.x] = axisW * invConeLength;

		float sine = axisW * invAxisLength;
		data.mConeSqrCosine[threadIdx.x] = 1 - sine * sine;
		data.mConeHalfLength[threadIdx.x] = axisLength;
	}

	__syncthreads();
}
}

__device__ CuCollision::CuCollision(Pointer<Shared, uint32_t> scratchPtr)
{
	int32_t numCapsules2 = 2 * gClothData.mNumCapsules;
	int32_t numCapsules4 = 4 * gClothData.mNumCapsules;
	int32_t numConvexes = gClothData.mNumConvexes;

	if(threadIdx.x < 3)
	{
		(&mCapsuleIndices)[threadIdx.x] = scratchPtr + threadIdx.x * numCapsules2;
		(&mShapeGrid)[-14 * int32_t(threadIdx.x)] = scratchPtr + numCapsules4 + numConvexes;
	}

	Pointer<Shared, uint32_t> indexPtr = scratchPtr + threadIdx.x;
	if(threadIdx.x < numCapsules2)
	{
		uint32_t index = (&gClothData.mCapsuleIndices->first)[threadIdx.x];
		*indexPtr = index;

		volatile uint32_t* maskPtr = generic(indexPtr + numCapsules2);
		*maskPtr = 1u << index;
		*maskPtr |= maskPtr[-int32_t(threadIdx.x & 1)];
	}
	indexPtr += numCapsules4;

	if(threadIdx.x < numConvexes)
		*indexPtr = gClothData.mConvexMasks[threadIdx.x];

	if(gClothData.mEnableContinuousCollision || gClothData.mFrictionScale > 0.0f)
	{
		allocate(mPrevData);

		__syncthreads(); // mPrevData raw hazard

		generateSpheres(mPrevData, 0.0f);
		generateCones(mPrevData, mCapsuleIndices + 2 * threadIdx.x);
	}

	allocate(mCurData); // also initializes mShapeGrid (!)
}

template <typename CurrentT, typename PreviousT>
__device__ void CuCollision::operator()(CurrentT& current, PreviousT& previous, float alpha)
{
	// if(current.w > 0) current.w = previous.w (see SwSolverKernel::computeBounds())
	for(int32_t i = threadIdx.x; i < gClothData.mNumParticles; i += blockDim.x)
	{
		if(current(i, 3) > 0.0f)
			current(i, 3) = previous(i, 3);
	}

	collideConvexes(current, previous, alpha);
	collideTriangles(current, alpha);

	if(buildAcceleration(current, alpha))
	{
		if(gClothData.mEnableContinuousCollision)
			collideContinuousCapsules(current, previous);
		else
			collideCapsules(current, previous);

		collideVirtualCapsules(current, previous);
	}

	// sync otherwise first threads overwrite sphere data before
	// remaining ones have had a chance to use it leading to incorrect
	// velocity calculation for friction / ccd

	__syncthreads();

	if(gClothData.mEnableContinuousCollision || gClothData.mFrictionScale > 0.0f)
	{
		// store current collision data for next iteration
		Pointer<Shared, float> dstIt = mPrevData.mSphereX + threadIdx.x;
		Pointer<Shared, const float> srcIt = mCurData.mSphereX + threadIdx.x;
		for(; dstIt < mCurData.mSphereX; dstIt += blockDim.x, srcIt += blockDim.x)
			*dstIt = *srcIt;
	}

	// __syncthreads() called in updateSleepState()
}

// build per-axis mask arrays of spheres on the right/left of grid cell
__device__ void CuCollision::buildSphereAcceleration(const CollisionData& data)
{
	if(threadIdx.x >= 192)
		return;

	int32_t sphereIdx = threadIdx.x & 31;
	int32_t axisIdx = threadIdx.x >> 6;             // coordinate index (x, y, or z)
	int32_t signi = threadIdx.x << 26 & 0x80000000; // sign bit (min or max)

	float signf = copysignf(1.0f, reinterpret_cast<const float&>(signi));
	float pos = signf * data.mSphereW[sphereIdx] + data.mSphereX[sphereIdx + gClothData.mNumSpheres * axisIdx];

	// use overflow so we can test for non-positive
	uint32_t index = signi - uint32_t(floorf(pos * mGridScale[axisIdx] + mGridBias[axisIdx]));

	axisIdx += (uint32_t(signi) >> 31) * 3;
	Pointer<Shared, uint32_t> dst = mShapeGrid + sGridSize * axisIdx;
	// #pragma unroll
	for(int32_t i = 0; i < sGridSize; ++i, ++index)
		dst[i] |= __ballot(int32_t(index) <= 0);
}

// generate cone masks from sphere masks
__device__ void CuCollision::buildConeAcceleration()
{
	if(threadIdx.x >= 192)
		return;

	int32_t coneIdx = threadIdx.x & 31;

	uint32_t sphereMask =
	    mCurData.mConeRadius[coneIdx] && coneIdx < gClothData.mNumCapsules ? mCapsuleMasks[2 * coneIdx + 1] : 0;

	int32_t offset = threadIdx.x / 32 * sGridSize;
	Pointer<Shared, uint32_t> src = mShapeGrid + offset;
	Pointer<Shared, uint32_t> dst = src + 6 * sGridSize;

	// #pragma unroll
	for(int32_t i = 0; i < sGridSize; ++i)
		dst[i] |= __ballot(src[i] & sphereMask);
}

// convert right/left mask arrays into single overlap array
__device__ void CuCollision::mergeAcceleration()
{
	if(threadIdx.x < sGridSize * 12)
	{
		Pointer<Shared, uint32_t> dst = mShapeGrid + threadIdx.x;
		if(!(gClothData.mEnableContinuousCollision || threadIdx.x * 43 & 1024))
			*dst &= dst[sGridSize * 3]; // above is same as 'threadIdx.x/24 & 1'

		// mask garbage bits from build*Acceleration
		int32_t shapeIdx = threadIdx.x >= sGridSize * 6; // spheres=0, cones=1
		*dst &= (1 << (&gClothData.mNumSpheres)[shapeIdx]) - 1;
	}
}

namespace
{
#if __CUDA_ARCH__ >= 300
__device__ float mergeBounds(Pointer<Shared, float> buffer)
{
	float value = *buffer;
	value = max(value, __shfl_down(value, 1));
	value = max(value, __shfl_down(value, 2));
	value = max(value, __shfl_down(value, 4));
	value = max(value, __shfl_down(value, 8));
	return max(value, __shfl_down(value, 16));
}
#else
__device__ float mergeBounds(Pointer<Shared, float> buffer)
{
	// ensure that writes to buffer are visible to all threads
	__threadfence_block();

	volatile float* ptr = generic(buffer);
	*ptr = max(*ptr, ptr[16]);
	*ptr = max(*ptr, ptr[8]);
	*ptr = max(*ptr, ptr[4]);
	*ptr = max(*ptr, ptr[2]);
	return max(*ptr, ptr[1]);
}
#endif
// computes maxX, -minX, maxY, ... with a stride of 32, threadIdx.x must be < 192
__device__ float computeSphereBounds(const CuCollision::CollisionData& data, Pointer<Shared, float> buffer)
{
	assert(threadIdx.x < 192);

	int32_t sphereIdx = min(threadIdx.x & 31, gClothData.mNumSpheres - 1); // sphere index
	int32_t axisIdx = threadIdx.x >> 6;                                    // coordinate index (x, y, or z)
	int32_t signi = threadIdx.x << 26;                                     // sign bit (min or max)
	float signf = copysignf(1.0f, reinterpret_cast<const float&>(signi));

	*buffer = data.mSphereW[sphereIdx] + signf * data.mSphereX[sphereIdx + gClothData.mNumSpheres * axisIdx];

	return mergeBounds(buffer);
}

#if __CUDA_ARCH__ >= 300
template <typename CurrentT>
__device__ float computeParticleBounds(const CurrentT& current, Pointer<Shared, float> buffer)
{
	int32_t numThreadsPerAxis = blockDim.x * 342 >> 10 & ~31; // same as / 3
	int32_t axis = (threadIdx.x >= numThreadsPerAxis) + (threadIdx.x >= 2 * numThreadsPerAxis);
	int32_t threadIdxInAxis = threadIdx.x - axis * numThreadsPerAxis;
	int laneIdx = threadIdx.x & 31;

	if(threadIdxInAxis < numThreadsPerAxis)
	{
		typename CurrentT::ConstPointerType posIt = current[axis];
		int32_t i = min(threadIdxInAxis, gClothData.mNumParticles - 1);
		float minX = posIt[i], maxX = minX;
		while(i += numThreadsPerAxis, i < gClothData.mNumParticles)
		{
			float posX = posIt[i];
			minX = min(minX, posX);
			maxX = max(maxX, posX);
		}

		minX = min(minX, __shfl_down(minX, 1));
		maxX = max(maxX, __shfl_down(maxX, 1));
		minX = min(minX, __shfl_down(minX, 2));
		maxX = max(maxX, __shfl_down(maxX, 2));
		minX = min(minX, __shfl_down(minX, 4));
		maxX = max(maxX, __shfl_down(maxX, 4));
		minX = min(minX, __shfl_down(minX, 8));
		maxX = max(maxX, __shfl_down(maxX, 8));
		minX = min(minX, __shfl_down(minX, 16));
		maxX = max(maxX, __shfl_down(maxX, 16));

		if(!laneIdx)
		{
			Pointer<Shared, float> dst = buffer - threadIdx.x + (threadIdxInAxis >> 5) + (axis << 6);
			dst[0] = maxX;
			dst[32] = -minX;
		}
	}

	__syncthreads();

	if(threadIdx.x >= 192)
		return 0.0f;

	float value = *buffer;
	if(laneIdx >= (numThreadsPerAxis >> 5))
		value = -FLT_MAX;

	// blockDim.x <= 3*512, increase to 3*1024 by adding a shfl by 16
	assert(numThreadsPerAxis <= 16 * 32);

	value = max(value, __shfl_down(value, 1));
	value = max(value, __shfl_down(value, 2));
	value = max(value, __shfl_down(value, 4));
	return max(value, __shfl_down(value, 8));
}
#else
template <typename CurrentT>
__device__ float computeParticleBounds(const CurrentT& current, Pointer<Shared, float> buffer)
{
	if(threadIdx.x >= 192)
		return 0.0f;

	int32_t axisIdx = threadIdx.x >> 6; // x, y, or z
	int32_t signi = threadIdx.x << 26;  // sign bit (min or max)
	float signf = copysignf(1.0f, reinterpret_cast<const float&>(signi));

	typename CurrentT::ConstPointerType pIt = current[axisIdx];
	typename CurrentT::ConstPointerType pEnd = pIt + gClothData.mNumParticles;
	pIt += min(threadIdx.x & 31, gClothData.mNumParticles - 1);

	*buffer = *pIt * signf;
	while(pIt += 32, pIt < pEnd)
		*buffer = max(*buffer, *pIt * signf);

	return mergeBounds(buffer);
}
#endif
}

// build mask of spheres/cones touching a regular grid along each axis
template <typename CurrentT>
__device__ bool CuCollision::buildAcceleration(const CurrentT& current, float alpha)
{
	// use still unused cone data as buffer for bounds computation
	Pointer<Shared, float> buffer = mCurData.mConeCenterX + threadIdx.x;
	float curParticleBounds = computeParticleBounds(current, buffer);
	int32_t warpIdx = threadIdx.x >> 5;

	if(!gClothData.mNumSpheres)
	{
		if(threadIdx.x < 192 && !(threadIdx.x & 31))
			gFrameData.mParticleBounds[warpIdx] = curParticleBounds;
		return false;
	}

	generateSpheres(mCurData, alpha);

	if(threadIdx.x < 192)
	{
		float sphereBounds = computeSphereBounds(mCurData, buffer);
		float particleBounds = curParticleBounds;
		if(gClothData.mEnableContinuousCollision)
		{
			sphereBounds = max(sphereBounds, computeSphereBounds(mPrevData, buffer));
			float prevParticleBounds = gFrameData.mParticleBounds[warpIdx];
			particleBounds = max(particleBounds, prevParticleBounds);
		}

		float bounds = min(sphereBounds, particleBounds);
		float expandedBounds = bounds + abs(bounds) * 1e-4f;

		// store bounds data in shared memory
		if(!(threadIdx.x & 31))
		{
			mGridScale[warpIdx] = expandedBounds;
			gFrameData.mParticleBounds[warpIdx] = curParticleBounds;
		}
	}

	__syncthreads(); // mGridScale raw hazard

	if(threadIdx.x < 3)
	{
		float negativeLower = mGridScale[threadIdx.x * 2 + 1];
		float edgeLength = mGridScale[threadIdx.x * 2] + negativeLower;
		float divisor = max(edgeLength, FLT_EPSILON);
		mGridScale[threadIdx.x] = __fdividef(sGridSize - 1e-3, divisor);
		mGridBias[threadIdx.x] = negativeLower * mGridScale[threadIdx.x];
		if(edgeLength < 0.0f)
			mGridScale[0] = 0.0f; // mark empty intersection
	}

	// initialize sphere *and* cone grid to 0
	if(threadIdx.x < 2 * 6 * sGridSize)
		mShapeGrid[threadIdx.x] = 0;

	__syncthreads(); // mGridScale raw hazard

	// generate cones even if test below fails because
	// continuous collision might need it in next iteration
	generateCones(mCurData, mCapsuleIndices + 2 * threadIdx.x);

	if(mGridScale[0] == 0.0f)
		return false; // early out for empty intersection

	if(gClothData.mEnableContinuousCollision)
		buildSphereAcceleration(mPrevData);
	buildSphereAcceleration(mCurData);
	__syncthreads(); // mCurData raw hazard

	buildConeAcceleration();
	__syncthreads(); // mShapeGrid raw hazard

	mergeAcceleration();
	__syncthreads(); // mShapeGrid raw hazard

	return true;
}

__device__ CuCollision::ShapeMask CuCollision::readShapeMask(const float& position,
                                                             Pointer<Shared, const uint32_t> sphereGrid)
{
	ShapeMask result;
	int32_t index = int32_t(floorf(position));
	uint32_t outMask = (index < sGridSize) - 1;

	Pointer<Shared, const uint32_t> gridPtr = sphereGrid + (index & sGridSize - 1);
	result.mSpheres = gridPtr[0] & ~outMask;
	result.mCones = gridPtr[sGridSize * 6] & ~outMask;

	return result;
}

// lookup acceleration structure and return mask of potential intersectors
template <typename CurPos>
__device__ CuCollision::ShapeMask CuCollision::getShapeMask(const CurPos& positions) const
{
	ShapeMask result;

	result = readShapeMask(positions.x * mGridScale[0] + mGridBias[0], mShapeGrid);
	result &= readShapeMask(positions.y * mGridScale[1] + mGridBias[1], mShapeGrid + 8);
	result &= readShapeMask(positions.z * mGridScale[2] + mGridBias[2], mShapeGrid + 16);

	return result;
}

template <typename PrevPos, typename CurPos>
__device__ CuCollision::ShapeMask CuCollision::getShapeMask(const PrevPos& prevPos, const CurPos& curPos) const
{
	ShapeMask result;

	float prevX = prevPos.x * mGridScale[0] + mGridBias[0];
	float prevY = prevPos.y * mGridScale[1] + mGridBias[1];
	float prevZ = prevPos.z * mGridScale[2] + mGridBias[2];

	float curX = curPos.x * mGridScale[0] + mGridBias[0];
	float curY = curPos.y * mGridScale[1] + mGridBias[1];
	float curZ = curPos.z * mGridScale[2] + mGridBias[2];

	float maxX = min(max(prevX, curX), 7.0f);
	float maxY = min(max(prevY, curY), 7.0f);
	float maxZ = min(max(prevZ, curZ), 7.0f);

	result = readShapeMask(maxX, mShapeGrid);
	result &= readShapeMask(maxY, mShapeGrid + 8);
	result &= readShapeMask(maxZ, mShapeGrid + 16);

	float minX = max(min(prevX, curX), 0.0f);
	float minY = max(min(prevY, curY), 0.0f);
	float minZ = max(min(prevZ, curZ), 0.0f);

	result &= readShapeMask(minX, mShapeGrid + 24);
	result &= readShapeMask(minY, mShapeGrid + 32);
	result &= readShapeMask(minZ, mShapeGrid + 40);

	return result;
}

template <typename CurPos>
__device__ int32_t CuCollision::collideCapsules(const CurPos& positions, float3& delta, float3& velocity) const
{
	ShapeMask shapeMask = getShapeMask(positions);

	delta.x = delta.y = delta.z = 0.0f;
	velocity.x = velocity.y = velocity.z = 0.0f;

	int32_t numCollisions = 0;

	bool frictionEnabled = gClothData.mFrictionScale > 0.0f;

	// cone collision
	for(; shapeMask.mCones; shapeMask.mCones &= shapeMask.mCones - 1)
	{
		int32_t j = __ffs(shapeMask.mCones) - 1;

		float deltaX = positions.x - mCurData.mConeCenterX[j];
		float deltaY = positions.y - mCurData.mConeCenterY[j];
		float deltaZ = positions.z - mCurData.mConeCenterZ[j];

		float axisX = mCurData.mConeAxisX[j];
		float axisY = mCurData.mConeAxisY[j];
		float axisZ = mCurData.mConeAxisZ[j];
		float slope = mCurData.mConeSlope[j];

		float dot = deltaX * axisX + deltaY * axisY + deltaZ * axisZ;
		float radius = max(dot * slope + mCurData.mConeRadius[j], 0.0f);
		float sqrDistance = deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ - dot * dot;

		Pointer<Shared, const uint32_t> mIt = mCapsuleMasks + 2 * j;
		uint32_t bothMask = mIt[1];

		if(sqrDistance > radius * radius)
		{
			shapeMask.mSpheres &= ~bothMask;
			continue;
		}

		sqrDistance = max(sqrDistance, FLT_EPSILON);
		float invDistance = rsqrtf(sqrDistance);

		float base = dot + slope * sqrDistance * invDistance;

		float halfLength = mCurData.mConeHalfLength[j];
		uint32_t leftMask = base < -halfLength;
		uint32_t rightMask = base > halfLength;

		uint32_t firstMask = mIt[0];
		uint32_t secondMask = firstMask ^ bothMask;

		shapeMask.mSpheres &= ~(firstMask & leftMask - 1);
		shapeMask.mSpheres &= ~(secondMask & rightMask - 1);

		if(!leftMask && !rightMask)
		{
			deltaX = deltaX - base * axisX;
			deltaY = deltaY - base * axisY;
			deltaZ = deltaZ - base * axisZ;

			float sqrCosine = mCurData.mConeSqrCosine[j];
			float scale = radius * invDistance * sqrCosine - sqrCosine;

			delta.x = delta.x + deltaX * scale;
			delta.y = delta.y + deltaY * scale;
			delta.z = delta.z + deltaZ * scale;

			if(frictionEnabled)
			{
				int32_t s0 = mCapsuleIndices[2 * j];
				int32_t s1 = mCapsuleIndices[2 * j + 1];

				// load previous sphere pos
				float s0vx = mCurData.mSphereX[s0] - mPrevData.mSphereX[s0];
				float s0vy = mCurData.mSphereY[s0] - mPrevData.mSphereY[s0];
				float s0vz = mCurData.mSphereZ[s0] - mPrevData.mSphereZ[s0];

				float s1vx = mCurData.mSphereX[s1] - mPrevData.mSphereX[s1];
				float s1vy = mCurData.mSphereY[s1] - mPrevData.mSphereY[s1];
				float s1vz = mCurData.mSphereZ[s1] - mPrevData.mSphereZ[s1];

				// interpolate velocity between the two spheres
				float t = dot * 0.5f + 0.5f;

				velocity.x += s0vx + t * (s1vx - s0vx);
				velocity.y += s0vy + t * (s1vy - s0vy);
				velocity.z += s0vz + t * (s1vz - s0vz);
			}

			++numCollisions;
		}
	}

	// sphere collision
	for(; shapeMask.mSpheres; shapeMask.mSpheres &= shapeMask.mSpheres - 1)
	{
		int32_t j = __ffs(shapeMask.mSpheres) - 1;

		float deltaX = positions.x - mCurData.mSphereX[j];
		float deltaY = positions.y - mCurData.mSphereY[j];
		float deltaZ = positions.z - mCurData.mSphereZ[j];

		float sqrDistance = FLT_EPSILON + deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ;
		float relDistance = rsqrtf(sqrDistance) * mCurData.mSphereW[j];

		if(relDistance > 1.0f)
		{
			float scale = relDistance - 1.0f;

			delta.x = delta.x + deltaX * scale;
			delta.y = delta.y + deltaY * scale;
			delta.z = delta.z + deltaZ * scale;

			if(frictionEnabled)
			{
				velocity.x += mCurData.mSphereX[j] - mPrevData.mSphereX[j];
				velocity.y += mCurData.mSphereY[j] - mPrevData.mSphereY[j];
				velocity.z += mCurData.mSphereZ[j] - mPrevData.mSphereZ[j];
			}

			++numCollisions;
		}
	}

	return numCollisions;
}

static const __device__ float gSkeletonWidth = (1 - 0.2f) * (1 - 0.2f) - 1;

template <typename PrevPos, typename CurPos>
__device__ int32_t
CuCollision::collideCapsules(const PrevPos& prevPos, CurPos& curPos, float3& delta, float3& velocity) const
{
	ShapeMask shapeMask = getShapeMask(prevPos, curPos);

	delta.x = delta.y = delta.z = 0.0f;
	velocity.x = velocity.y = velocity.z = 0.0f;

	int32_t numCollisions = 0;
	bool frictionEnabled = gClothData.mFrictionScale > 0.0f;

	// cone collision
	for(; shapeMask.mCones; shapeMask.mCones &= shapeMask.mCones - 1)
	{
		int32_t j = __ffs(shapeMask.mCones) - 1;

		float prevAxisX = mPrevData.mConeAxisX[j];
		float prevAxisY = mPrevData.mConeAxisY[j];
		float prevAxisZ = mPrevData.mConeAxisZ[j];
		float prevSlope = mPrevData.mConeSlope[j];

		float prevX = prevPos.x - mPrevData.mConeCenterX[j];
		float prevY = prevPos.y - mPrevData.mConeCenterY[j];
		float prevZ = prevPos.z - mPrevData.mConeCenterZ[j];
		float prevT = prevY * prevAxisZ - prevZ * prevAxisY;
		float prevU = prevZ * prevAxisX - prevX * prevAxisZ;
		float prevV = prevX * prevAxisY - prevY * prevAxisX;
		float prevDot = prevX * prevAxisX + prevY * prevAxisY + prevZ * prevAxisZ;
		float prevRadius = max(prevDot * prevSlope + mCurData.mConeRadius[j], 0.0f);

		float curAxisX = mCurData.mConeAxisX[j];
		float curAxisY = mCurData.mConeAxisY[j];
		float curAxisZ = mCurData.mConeAxisZ[j];
		float curSlope = mCurData.mConeSlope[j];

		float curX = curPos.x - mCurData.mConeCenterX[j];
		float curY = curPos.y - mCurData.mConeCenterY[j];
		float curZ = curPos.z - mCurData.mConeCenterZ[j];
		float curT = curY * curAxisZ - curZ * curAxisY;
		float curU = curZ * curAxisX - curX * curAxisZ;
		float curV = curX * curAxisY - curY * curAxisX;
		float curDot = curX * curAxisX + curY * curAxisY + curZ * curAxisZ;
		float curRadius = max(curDot * curSlope + mCurData.mConeRadius[j], 0.0f);

		float curSqrDistance = FLT_EPSILON + curT * curT + curU * curU + curV * curV;

		float dotPrevPrev = prevT * prevT + prevU * prevU + prevV * prevV - prevRadius * prevRadius;
		float dotPrevCur = prevT * curT + prevU * curU + prevV * curV - prevRadius * curRadius;
		float dotCurCur = curSqrDistance - curRadius * curRadius;

		float discriminant = dotPrevCur * dotPrevCur - dotCurCur * dotPrevPrev;
		float sqrtD = sqrtf(discriminant);
		float halfB = dotPrevCur - dotPrevPrev;
		float minusA = dotPrevCur - dotCurCur + halfB;

		// time of impact or 0 if prevPos inside cone
		float toi = __fdividef(min(0.0f, halfB + sqrtD), minusA);
		bool hasCollision = toi < 1.0f && halfB < sqrtD;

		// skip continuous collision if the (un-clamped) particle
		// trajectory only touches the outer skin of the cone.
		float rMin = prevRadius + halfB * minusA * (curRadius - prevRadius);
		hasCollision = hasCollision && (discriminant > minusA * rMin * rMin * gSkeletonWidth);

		// a is negative when one cone is contained in the other,
		// which is already handled by discrete collision.
		hasCollision = hasCollision && minusA < -FLT_EPSILON;

		if(hasCollision)
		{
			float deltaX = prevX - curX;
			float deltaY = prevY - curY;
			float deltaZ = prevZ - curZ;

			// interpolate delta at toi
			float posX = prevX - deltaX * toi;
			float posY = prevY - deltaY * toi;
			float posZ = prevZ - deltaZ * toi;

			float curHalfLength = mCurData.mConeHalfLength[j];
			float curScaledAxisX = curAxisX * curHalfLength;
			float curScaledAxisY = curAxisY * curHalfLength;
			float curScaledAxisZ = curAxisZ * curHalfLength;

			float prevHalfLength = mPrevData.mConeHalfLength[j];
			float deltaScaledAxisX = curScaledAxisX - prevAxisX * prevHalfLength;
			float deltaScaledAxisY = curScaledAxisY - prevAxisY * prevHalfLength;
			float deltaScaledAxisZ = curScaledAxisZ - prevAxisZ * prevHalfLength;

			float oneMinusToi = 1.0f - toi;

			// interpolate axis at toi
			float axisX = curScaledAxisX - deltaScaledAxisX * oneMinusToi;
			float axisY = curScaledAxisY - deltaScaledAxisY * oneMinusToi;
			float axisZ = curScaledAxisZ - deltaScaledAxisZ * oneMinusToi;
			float slope = prevSlope * oneMinusToi + curSlope * toi;

			float sqrHalfLength = axisX * axisX + axisY * axisY + axisZ * axisZ;
			float invHalfLength = rsqrtf(sqrHalfLength);
			float dot = (posX * axisX + posY * axisY + posZ * axisZ) * invHalfLength;

			float sqrDistance = posX * posX + posY * posY + posZ * posZ - dot * dot;
			float invDistance = sqrDistance > 0.0f ? rsqrtf(sqrDistance) : 0.0f;

			float base = dot + slope * sqrDistance * invDistance;
			float scale = base * invHalfLength;

			if(abs(scale) < 1.0f)
			{
				deltaX = deltaX + deltaScaledAxisX * scale;
				deltaY = deltaY + deltaScaledAxisY * scale;
				deltaZ = deltaZ + deltaScaledAxisZ * scale;

				// reduce ccd impulse if (clamped) particle trajectory stays in cone skin,
				// i.e. scale by exp2(-k) or 1/(1+k) with k = (tmin - toi) / (1 - toi)
				float minusK = __fdividef(sqrtD, minusA * oneMinusToi);
				oneMinusToi = __fdividef(oneMinusToi, 1 - minusK);

				curX = curX + deltaX * oneMinusToi;
				curY = curY + deltaY * oneMinusToi;
				curZ = curZ + deltaZ * oneMinusToi;

				curDot = curX * curAxisX + curY * curAxisY + curZ * curAxisZ;
				curRadius = max(curDot * curSlope + mCurData.mConeRadius[j], 0.0f);
				curSqrDistance = curX * curX + curY * curY + curZ * curZ - curDot * curDot;

				curPos.x = mCurData.mConeCenterX[j] + curX;
				curPos.y = mCurData.mConeCenterY[j] + curY;
				curPos.z = mCurData.mConeCenterZ[j] + curZ;
			}
		}

		// curPos inside cone (discrete collision)
		bool hasContact = curRadius * curRadius > curSqrDistance;

		Pointer<Shared, const uint32_t> mIt = mCapsuleMasks + 2 * j;
		uint32_t bothMask = mIt[1];

		uint32_t cullMask = bothMask & (hasCollision | hasContact) - 1;
		shapeMask.mSpheres &= ~cullMask;

		if(!hasContact)
			continue;

		float invDistance = curSqrDistance > 0.0f ? rsqrtf(curSqrDistance) : 0.0f;
		float base = curDot + curSlope * curSqrDistance * invDistance;

		float halfLength = mCurData.mConeHalfLength[j];
		uint32_t leftMask = base < -halfLength;
		uint32_t rightMask = base > halfLength;

		// can only skip continuous sphere collision if post-ccd position
		// is on code side *and* particle had cone-ccd collision.
		uint32_t firstMask = mIt[0];
		uint32_t secondMask = firstMask ^ bothMask;
		cullMask = (firstMask & leftMask - 1) | (secondMask & rightMask - 1);
		shapeMask.mSpheres &= ~cullMask | hasCollision - 1;

		if(!leftMask && !rightMask)
		{
			float deltaX = curX - base * curAxisX;
			float deltaY = curY - base * curAxisY;
			float deltaZ = curZ - base * curAxisZ;

			float sqrCosine = mCurData.mConeSqrCosine[j];
			float scale = curRadius * invDistance * sqrCosine - sqrCosine;

			delta.x = delta.x + deltaX * scale;
			delta.y = delta.y + deltaY * scale;
			delta.z = delta.z + deltaZ * scale;

			if(frictionEnabled)
			{
				int32_t s0 = mCapsuleIndices[2 * j];
				int32_t s1 = mCapsuleIndices[2 * j + 1];

				// load previous sphere pos
				float s0vx = mCurData.mSphereX[s0] - mPrevData.mSphereX[s0];
				float s0vy = mCurData.mSphereY[s0] - mPrevData.mSphereY[s0];
				float s0vz = mCurData.mSphereZ[s0] - mPrevData.mSphereZ[s0];

				float s1vx = mCurData.mSphereX[s1] - mPrevData.mSphereX[s1];
				float s1vy = mCurData.mSphereY[s1] - mPrevData.mSphereY[s1];
				float s1vz = mCurData.mSphereZ[s1] - mPrevData.mSphereZ[s1];

				// interpolate velocity between the two spheres
				float t = curDot * 0.5f + 0.5f;

				velocity.x += s0vx + t * (s1vx - s0vx);
				velocity.y += s0vy + t * (s1vy - s0vy);
				velocity.z += s0vz + t * (s1vz - s0vz);
			}

			++numCollisions;
		}
	}

	// sphere collision
	for(; shapeMask.mSpheres; shapeMask.mSpheres &= shapeMask.mSpheres - 1)
	{
		int32_t j = __ffs(shapeMask.mSpheres) - 1;

		float prevX = prevPos.x - mPrevData.mSphereX[j];
		float prevY = prevPos.y - mPrevData.mSphereY[j];
		float prevZ = prevPos.z - mPrevData.mSphereZ[j];
		float prevRadius = mPrevData.mSphereW[j];

		float curX = curPos.x - mCurData.mSphereX[j];
		float curY = curPos.y - mCurData.mSphereY[j];
		float curZ = curPos.z - mCurData.mSphereZ[j];
		float curRadius = mCurData.mSphereW[j];

		float sqrDistance = FLT_EPSILON + curX * curX + curY * curY + curZ * curZ;

		float dotPrevPrev = prevX * prevX + prevY * prevY + prevZ * prevZ - prevRadius * prevRadius;
		float dotPrevCur = prevX * curX + prevY * curY + prevZ * curZ - prevRadius * curRadius;
		float dotCurCur = sqrDistance - curRadius * curRadius;

		float discriminant = dotPrevCur * dotPrevCur - dotCurCur * dotPrevPrev;
		float sqrtD = sqrtf(discriminant);
		float halfB = dotPrevCur - dotPrevPrev;
		float minusA = dotPrevCur - dotCurCur + halfB;

		// time of impact or 0 if prevPos inside sphere
		float toi = __fdividef(min(0.0f, halfB + sqrtD), minusA);
		bool hasCollision = toi < 1.0f && halfB < sqrtD;

		// skip continuous collision if the (un-clamped) particle
		// trajectory only touches the outer skin of the cone.
		float rMin = prevRadius + halfB * minusA * (curRadius - prevRadius);
		hasCollision = hasCollision && (discriminant > minusA * rMin * rMin * gSkeletonWidth);

		// a is negative when one cone is contained in the other,
		// which is already handled by discrete collision.
		hasCollision = hasCollision && minusA < -FLT_EPSILON;

		if(hasCollision)
		{
			float deltaX = prevX - curX;
			float deltaY = prevY - curY;
			float deltaZ = prevZ - curZ;

			float oneMinusToi = 1.0f - toi;

			// reduce ccd impulse if (clamped) particle trajectory stays in cone skin,
			// i.e. scale by exp2(-k) or 1/(1+k) with k = (tmin - toi) / (1 - toi)
			float minusK = __fdividef(sqrtD, minusA * oneMinusToi);
			oneMinusToi = __fdividef(oneMinusToi, 1 - minusK);

			curX = curX + deltaX * oneMinusToi;
			curY = curY + deltaY * oneMinusToi;
			curZ = curZ + deltaZ * oneMinusToi;

			curPos.x = mCurData.mSphereX[j] + curX;
			curPos.y = mCurData.mSphereY[j] + curY;
			curPos.z = mCurData.mSphereZ[j] + curZ;

			sqrDistance = FLT_EPSILON + curX * curX + curY * curY + curZ * curZ;
		}

		float relDistance = rsqrtf(sqrDistance) * curRadius;

		if(relDistance > 1.0f)
		{
			float scale = relDistance - 1.0f;

			delta.x = delta.x + curX * scale;
			delta.y = delta.y + curY * scale;
			delta.z = delta.z + curZ * scale;

			if(frictionEnabled)
			{
				velocity.x += mCurData.mSphereX[j] - mPrevData.mSphereX[j];
				velocity.y += mCurData.mSphereY[j] - mPrevData.mSphereY[j];
				velocity.z += mCurData.mSphereZ[j] - mPrevData.mSphereZ[j];
			}

			++numCollisions;
		}
	}

	return numCollisions;
}

namespace
{
template <typename PrevPos, typename CurPos>
__device__ inline float3 calcFrictionImpulse(const PrevPos& prevPos, const CurPos& curPos, const float3& shapeVelocity,
                                             float scale, const float3& collisionImpulse)
{
	const float frictionScale = gClothData.mFrictionScale;

	// calculate collision normal
	float deltaSq = collisionImpulse.x * collisionImpulse.x + collisionImpulse.y * collisionImpulse.y +
	                collisionImpulse.z * collisionImpulse.z;

	float rcpDelta = rsqrtf(deltaSq + FLT_EPSILON);

	float nx = collisionImpulse.x * rcpDelta;
	float ny = collisionImpulse.y * rcpDelta;
	float nz = collisionImpulse.z * rcpDelta;

	// calculate relative velocity scaled by number of collision
	float rvx = curPos.x - prevPos.x - shapeVelocity.x * scale;
	float rvy = curPos.y - prevPos.y - shapeVelocity.y * scale;
	float rvz = curPos.z - prevPos.z - shapeVelocity.z * scale;

	// calculate magnitude of relative normal velocity
	float rvn = rvx * nx + rvy * ny + rvz * nz;

	// calculate relative tangential velocity
	float rvtx = rvx - rvn * nx;
	float rvty = rvy - rvn * ny;
	float rvtz = rvz - rvn * nz;

	// calculate magnitude of vt
	float rcpVt = rsqrtf(rvtx * rvtx + rvty * rvty + rvtz * rvtz + FLT_EPSILON);

	// magnitude of friction impulse (cannot be larger than -|vt|)
	float j = max(-frictionScale * deltaSq * rcpDelta * scale * rcpVt, -1.0f);

	return make_float3(rvtx * j, rvty * j, rvtz * j);
}
}

template <typename CurrentT, typename PreviousT>
__device__ void CuCollision::collideCapsules(CurrentT& current, PreviousT& previous) const
{
	bool frictionEnabled = gClothData.mFrictionScale > 0.0f;
	bool massScaleEnabled = gClothData.mCollisionMassScale > 0.0f;

	for(int32_t i = threadIdx.x; i < gClothData.mNumParticles; i += blockDim.x)
	{
		typename CurrentT::VectorType curPos = current(i);

		float3 delta, velocity;
		if(int32_t numCollisions = collideCapsules(curPos, delta, velocity))
		{
			float scale = __fdividef(1.0f, numCollisions);

			if(frictionEnabled)
			{
				typename PreviousT::VectorType prevPos = previous(i);
				float3 frictionImpulse = calcFrictionImpulse(prevPos, curPos, velocity, scale, delta);

				prevPos.x -= frictionImpulse.x;
				prevPos.y -= frictionImpulse.y;
				prevPos.z -= frictionImpulse.z;

				previous(i) = prevPos;
			}

			curPos.x += delta.x * scale;
			curPos.y += delta.y * scale;
			curPos.z += delta.z * scale;

			current(i) = curPos;

			if(massScaleEnabled)
			{
				float deltaLengthSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
				float massScale = 1.0f + gClothData.mCollisionMassScale * deltaLengthSq;
				current(i, 3) = __fdividef(current(i, 3), massScale);
			}
		}
	}
}

namespace
{
template <typename PointerT>
__device__ float lerp(PointerT pos, const int4& indices, const float4& weights)
{
	return pos[indices.x] * weights.x + pos[indices.y] * weights.y + pos[indices.z] * weights.z;
}

template <typename PointerT>
__device__ void apply(PointerT pos, const int4& indices, const float4& weights, float delta)
{
	pos[indices.x] += delta * weights.x;
	pos[indices.y] += delta * weights.y;
	pos[indices.z] += delta * weights.z;
}
}

template <typename CurrentT, typename PreviousT>
__device__ void CuCollision::collideVirtualCapsules(CurrentT& current, PreviousT& previous) const
{
	const uint32_t* __restrict setSizeIt = gClothData.mVirtualParticleSetSizesBegin;

	if(!setSizeIt)
		return;

	if(gClothData.mEnableContinuousCollision)
	{
		// copied from mergeAcceleration
		Pointer<Shared, uint32_t> dst = mShapeGrid + threadIdx.x;
		if(!(threadIdx.x * 43 & 1024) && threadIdx.x < sGridSize * 12)
			*dst &= dst[sGridSize * 3];
		__syncthreads(); // mShapeGrid raw hazard
	}

	const uint32_t* __restrict setSizeEnd = gClothData.mVirtualParticleSetSizesEnd;
	const uint16_t* __restrict indicesEnd = gClothData.mVirtualParticleIndices;
	const float4* __restrict weightsIt = reinterpret_cast<const float4*>(gClothData.mVirtualParticleWeights);

	bool frictionEnabled = gClothData.mFrictionScale > 0.0f;
	bool massScaleEnabled = gClothData.mCollisionMassScale > 0.0f;

	for(; setSizeIt != setSizeEnd; ++setSizeIt)
	{
		__syncthreads();

		const uint16_t* __restrict indicesIt = indicesEnd + threadIdx.x * 4;
		for(indicesEnd += *setSizeIt * 4; indicesIt < indicesEnd; indicesIt += blockDim.x * 4)
		{
			int4 indices = make_int4(indicesIt[0], indicesIt[1], indicesIt[2], indicesIt[3]);

			float4 weights = weightsIt[indices.w];

			float3 curPos;
			curPos.x = lerp(current[0], indices, weights);
			curPos.y = lerp(current[1], indices, weights);
			curPos.z = lerp(current[2], indices, weights);

			float3 delta, velocity;
			if(int32_t numCollisions = collideCapsules(curPos, delta, velocity))
			{
				float scale = __fdividef(1.0f, numCollisions);
				float wscale = weights.w * scale;

				apply(current[0], indices, weights, delta.x * wscale);
				apply(current[1], indices, weights, delta.y * wscale);
				apply(current[2], indices, weights, delta.z * wscale);

				if(frictionEnabled)
				{
					float3 prevPos;
					prevPos.x = lerp(previous[0], indices, weights);
					prevPos.y = lerp(previous[1], indices, weights);
					prevPos.z = lerp(previous[2], indices, weights);

					float3 frictionImpulse = calcFrictionImpulse(prevPos, curPos, velocity, scale, delta);

					apply(previous[0], indices, weights, frictionImpulse.x * -weights.w);
					apply(previous[1], indices, weights, frictionImpulse.y * -weights.w);
					apply(previous[2], indices, weights, frictionImpulse.z * -weights.w);
				}

				if(massScaleEnabled)
				{
					float deltaLengthSq = (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z) * scale * scale;
					float invMassScale = __fdividef(1.0f, 1.0f + gClothData.mCollisionMassScale * deltaLengthSq);

					// not multiplying by weights[3] here because unlike applying velocity
					// deltas where we want the interpolated position to obtain a particular
					// value, we instead just require that the total change is equal to invMassScale
					invMassScale = invMassScale - 1.0f;
					current(indices.x, 3) *= 1.0f + weights.x * invMassScale;
					current(indices.y, 3) *= 1.0f + weights.y * invMassScale;
					current(indices.z, 3) *= 1.0f + weights.z * invMassScale;
				}
			}
		}
	}
}

template <typename CurrentT, typename PreviousT>
__device__ void CuCollision::collideContinuousCapsules(CurrentT& current, PreviousT& previous) const
{
	bool frictionEnabled = gClothData.mFrictionScale > 0.0f;
	bool massScaleEnabled = gClothData.mCollisionMassScale > 0.0f;

	for(int32_t i = threadIdx.x; i < gClothData.mNumParticles; i += blockDim.x)
	{
		typename PreviousT::VectorType prevPos = previous(i);
		typename CurrentT::VectorType curPos = current(i);

		float3 delta, velocity;
		if(int32_t numCollisions = collideCapsules(prevPos, curPos, delta, velocity))
		{
			float scale = __fdividef(1.0f, numCollisions);

			if(frictionEnabled)
			{
				float3 frictionImpulse = calcFrictionImpulse(prevPos, curPos, velocity, scale, delta);

				prevPos.x -= frictionImpulse.x;
				prevPos.y -= frictionImpulse.y;
				prevPos.z -= frictionImpulse.z;

				previous(i) = prevPos;
			}

			curPos.x += delta.x * scale;
			curPos.y += delta.y * scale;
			curPos.z += delta.z * scale;

			current(i) = curPos;

			if(massScaleEnabled)
			{
				float deltaLengthSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
				float massScale = 1.0f + gClothData.mCollisionMassScale * deltaLengthSq;
				current(i, 3) = __fdividef(current(i, 3), massScale);
			}
		}
	}
}

template <typename CurPos>
__device__ int32_t CuCollision::collideConvexes(const CurPos& positions, float3& delta) const
{
	delta.x = delta.y = delta.z = 0.0f;

	Pointer<Shared, const float> planeX = mCurData.mSphereX;
	Pointer<Shared, const float> planeY = planeX + gClothData.mNumPlanes;
	Pointer<Shared, const float> planeZ = planeY + gClothData.mNumPlanes;
	Pointer<Shared, const float> planeW = planeZ + gClothData.mNumPlanes;

	int32_t numCollisions = 0;
	Pointer<Shared, const uint32_t> cIt = mConvexMasks;
	Pointer<Shared, const uint32_t> cEnd = cIt + gClothData.mNumConvexes;
	for(; cIt != cEnd; ++cIt)
	{
		uint32_t mask = *cIt;

		int32_t maxIndex = __ffs(mask) - 1;
		float maxDist = planeW[maxIndex] + positions.z * planeZ[maxIndex] + positions.y * planeY[maxIndex] +
		                positions.x * planeX[maxIndex];

		while((maxDist < 0.0f) && (mask &= mask - 1))
		{
			int32_t i = __ffs(mask) - 1;
			float dist = planeW[i] + positions.z * planeZ[i] + positions.y * planeY[i] + positions.x * planeX[i];
			if(dist > maxDist)
				maxDist = dist, maxIndex = i;
		}

		if(maxDist < 0.0f)
		{
			delta.x -= planeX[maxIndex] * maxDist;
			delta.y -= planeY[maxIndex] * maxDist;
			delta.z -= planeZ[maxIndex] * maxDist;

			++numCollisions;
		}
	}

	return numCollisions;
}

template <typename CurrentT, typename PreviousT>
__device__ void CuCollision::collideConvexes(CurrentT& current, PreviousT& previous, float alpha)
{
	if(!gClothData.mNumConvexes)
		return;

	// interpolate planes and transpose
	if(threadIdx.x < gClothData.mNumPlanes * 4)
	{
		float start = gFrameData.mStartCollisionPlanes[threadIdx.x];
		float target = gFrameData.mTargetCollisionPlanes[threadIdx.x];
		int32_t j = threadIdx.x % 4 * gClothData.mNumPlanes + threadIdx.x / 4;
		mCurData.mSphereX[j] = start + (target - start) * alpha;
	}

	__syncthreads();

	bool frictionEnabled = gClothData.mFrictionScale > 0.0f;

	for(int32_t i = threadIdx.x; i < gClothData.mNumParticles; i += blockDim.x)
	{
		typename CurrentT::VectorType curPos = current(i);

		float3 delta;
		if(int32_t numCollisions = collideConvexes(curPos, delta))
		{
			float scale = __fdividef(1.0f, numCollisions);

			if(frictionEnabled)
			{
				typename PreviousT::VectorType prevPos = previous(i);

				float3 frictionImpulse =
				    calcFrictionImpulse(prevPos, curPos, make_float3(0.0f, 0.0f, 0.0f), scale, delta);

				prevPos.x -= frictionImpulse.x;
				prevPos.y -= frictionImpulse.y;
				prevPos.z -= frictionImpulse.z;

				previous(i) = prevPos;
			}

			curPos.x += delta.x * scale;
			curPos.y += delta.y * scale;
			curPos.z += delta.z * scale;

			current(i) = curPos;
		}
	}

	__syncthreads();
}

namespace
{
struct TriangleData
{
	float baseX, baseY, baseZ;
	float edge0X, edge0Y, edge0Z;
	float edge1X, edge1Y, edge1Z;
	float normalX, normalY, normalZ;

	float edge0DotEdge1;
	float edge0SqrLength;
	float edge1SqrLength;

	float det;
	float denom;

	float edge0InvSqrLength;
	float edge1InvSqrLength;

	// initialize struct after vertices have been stored in first 9 members
	__device__ void initialize()
	{
		edge0X -= baseX, edge0Y -= baseY, edge0Z -= baseZ;
		edge1X -= baseX, edge1Y -= baseY, edge1Z -= baseZ;

		normalX = edge0Y * edge1Z - edge0Z * edge1Y;
		normalY = edge0Z * edge1X - edge0X * edge1Z;
		normalZ = edge0X * edge1Y - edge0Y * edge1X;

		float normalInvLength = rsqrtf(normalX * normalX + normalY * normalY + normalZ * normalZ);
		normalX *= normalInvLength;
		normalY *= normalInvLength;
		normalZ *= normalInvLength;

		edge0DotEdge1 = edge0X * edge1X + edge0Y * edge1Y + edge0Z * edge1Z;
		edge0SqrLength = edge0X * edge0X + edge0Y * edge0Y + edge0Z * edge0Z;
		edge1SqrLength = edge1X * edge1X + edge1Y * edge1Y + edge1Z * edge1Z;

		det = __fdividef(1.0f, edge0SqrLength * edge1SqrLength - edge0DotEdge1 * edge0DotEdge1);
		denom = __fdividef(1.0f, edge0SqrLength + edge1SqrLength - edge0DotEdge1 - edge0DotEdge1);

		edge0InvSqrLength = __fdividef(1.0f, edge0SqrLength);
		edge1InvSqrLength = __fdividef(1.0f, edge1SqrLength);
	}
};
}

template <typename CurrentT>
__device__ void CuCollision::collideTriangles(CurrentT& current, int32_t i)
{
	float posX = current(i, 0);
	float posY = current(i, 1);
	float posZ = current(i, 2);

	const TriangleData* __restrict tIt = reinterpret_cast<const TriangleData*>(generic(mCurData.mSphereX));
	const TriangleData* __restrict tEnd = tIt + gClothData.mNumCollisionTriangles;

	float normalX, normalY, normalZ, normalD = 0.0f;
	float minSqrLength = FLT_MAX;

	for(; tIt != tEnd; ++tIt)
	{
		float dx = posX - tIt->baseX;
		float dy = posY - tIt->baseY;
		float dz = posZ - tIt->baseZ;

		float deltaDotEdge0 = dx * tIt->edge0X + dy * tIt->edge0Y + dz * tIt->edge0Z;
		float deltaDotEdge1 = dx * tIt->edge1X + dy * tIt->edge1Y + dz * tIt->edge1Z;
		float deltaDotNormal = dx * tIt->normalX + dy * tIt->normalY + dz * tIt->normalZ;

		float s = tIt->edge1SqrLength * deltaDotEdge0 - tIt->edge0DotEdge1 * deltaDotEdge1;
		float t = tIt->edge0SqrLength * deltaDotEdge1 - tIt->edge0DotEdge1 * deltaDotEdge0;

		s = t > 0.0f ? s * tIt->det : deltaDotEdge0 * tIt->edge0InvSqrLength;
		t = s > 0.0f ? t * tIt->det : deltaDotEdge1 * tIt->edge1InvSqrLength;

		if(s + t > 1.0f)
		{
			s = (tIt->edge1SqrLength - tIt->edge0DotEdge1 + deltaDotEdge0 - deltaDotEdge1) * tIt->denom;
		}

		s = fmaxf(0.0f, fminf(1.0f, s));
		t = fmaxf(0.0f, fminf(1.0f - s, t));

		dx = dx - tIt->edge0X * s - tIt->edge1X * t;
		dy = dy - tIt->edge0Y * s - tIt->edge1Y * t;
		dz = dz - tIt->edge0Z * s - tIt->edge1Z * t;

		float sqrLength = dx * dx + dy * dy + dz * dz;

		if(0.0f > deltaDotNormal)
			sqrLength *= 1.0001f;

		if(sqrLength < minSqrLength)
		{
			normalX = tIt->normalX;
			normalY = tIt->normalY;
			normalZ = tIt->normalZ;
			normalD = deltaDotNormal;
			minSqrLength = sqrLength;
		}
	}

	if(normalD < 0.0f)
	{
		current(i, 0) = posX - normalX * normalD;
		current(i, 1) = posY - normalY * normalD;
		current(i, 2) = posZ - normalZ * normalD;
	}
}

namespace
{
static const int32_t sTrianglePadding = sizeof(TriangleData) / sizeof(float) - 9;
}

template <typename CurrentT>
__device__ void CuCollision::collideTriangles(CurrentT& current, float alpha)
{
	if(!gClothData.mNumCollisionTriangles)
		return;

	// interpolate triangle vertices and store in shared memory
	for(int32_t i = threadIdx.x, n = gClothData.mNumCollisionTriangles * 9; i < n; i += blockDim.x)
	{
		float start = gFrameData.mStartCollisionTriangles[i];
		float target = gFrameData.mTargetCollisionTriangles[i];
		int32_t idx = i * 7282 >> 16; // same as i/9
		int32_t offset = i + idx * sTrianglePadding;
		mCurData.mSphereX[offset] = start + (target - start) * alpha;
	}

	__syncthreads();

	for(int32_t i = threadIdx.x; i < gClothData.mNumCollisionTriangles; i += blockDim.x)
	{
		reinterpret_cast<TriangleData*>(generic(mCurData.mSphereX))[i].initialize();
	}

	__syncthreads();

	for(int32_t i = threadIdx.x; i < gClothData.mNumParticles; i += blockDim.x)
		collideTriangles(current, i);

	__syncthreads();
}
