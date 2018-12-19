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
#ifndef __CUDACC__
#include "Simd.h"
#endif

namespace physx
{
namespace cloth
{

class CuCloth;
struct CuPhaseConfig;
template <typename>
struct IterationState;
struct IndexPair;
struct CuIterationData;
struct CuTether;

// reference to cloth instance bulk data (POD)
// should not need frequent updates (stored on device)
struct CuClothData
{
	CuClothData()
	{
	}
	CuClothData(CuCloth&);

	// particle data
	uint32_t mNumParticles;
	float* mParticles;
	float* mParticlesHostCopy;

	// fabric constraints
	uint32_t mNumPhases;
	const CuPhaseConfig* mPhaseConfigs;

	const CuTether* mTethers;
	uint32_t mNumTethers;
	float mTetherConstraintScale;

	const uint16_t* mTriangles;
	uint32_t mNumTriangles;

	// motion constraint data
	float mMotionConstraintScale;
	float mMotionConstraintBias;

	// collision data
	uint32_t mNumSpheres;  // don't change this order, it's
	uint32_t mNumCapsules; // needed by mergeAcceleration()
	const IndexPair* mCapsuleIndices;
	uint32_t mNumPlanes;
	uint32_t mNumConvexes;
	const uint32_t* mConvexMasks;
	uint32_t mNumCollisionTriangles;

	// virtual particle data
	const uint32_t* mVirtualParticleSetSizesBegin;
	const uint32_t* mVirtualParticleSetSizesEnd;
	const uint16_t* mVirtualParticleIndices;
	const float* mVirtualParticleWeights;

	bool mEnableContinuousCollision;
	float mCollisionMassScale;
	float mFrictionScale;

	float mSelfCollisionDistance;
	uint32_t mNumSelfCollisionIndices;
	const uint32_t* mSelfCollisionIndices;
	float* mSelfCollisionParticles;
	uint32_t* mSelfCollisionKeys;
	uint16_t* mSelfCollisionCellStart;

	// sleep data
	uint32_t mSleepTestInterval;
	uint32_t mSleepAfterCount;
	float mSleepThreshold;
};

// per-frame data (stored in pinned memory)
struct CuFrameData
{
	CuFrameData()
	{
	} // not initializing pointers to 0!

#ifndef __CUDACC__
	explicit CuFrameData(CuCloth&, uint32_t, const IterationState<Simd4f>&, const CuIterationData*);
#endif

	bool mDeviceParticlesDirty;

	// number of particle copies that fit in shared memory (0, 1, or 2)
	uint32_t mNumSharedPositions;

	// iteration data
	float mIterDt;
	uint32_t mNumIterations;
	const CuIterationData* mIterationData;

	float mTetherConstraintStiffness;

	// wind data
	float mDragCoefficient;
	float mLiftCoefficient;
	float mRotation[9];

	// motion constraint data
	const float* mStartMotionConstraints;
	float* mTargetMotionConstraints;
	const float* mHostMotionConstraints;
	float mMotionConstraintStiffness;

	// separation constraint data
	const float* mStartSeparationConstraints;
	float* mTargetSeparationConstraints;
	const float* mHostSeparationConstraints;

	// particle acceleration data
	float* mParticleAccelerations;
	const float* mHostParticleAccelerations;

	// rest positions
	const float* mRestPositions;

	// collision data
	const float* mStartCollisionSpheres;
	const float* mTargetCollisionSpheres;
	const float* mStartCollisionPlanes;
	const float* mTargetCollisionPlanes;
	const float* mStartCollisionTriangles;
	const float* mTargetCollisionTriangles;

	float mSelfCollisionStiffness;

	float mParticleBounds[6]; // maxX, -minX, maxY, ...

	uint32_t mSleepPassCounter;
	uint32_t mSleepTestCounter;

	float mStiffnessExponent;
};

// per-iteration data (stored in pinned memory)
struct CuIterationData
{
	CuIterationData()
	{
	} // not initializing!

#ifndef __CUDACC__
	explicit CuIterationData(const IterationState<Simd4f>&);
#endif

	float mIntegrationTrafo[24];
	float mWind[3];
	uint32_t mIsTurning;
};
}
}
