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

#include "foundation/Px.h"
#include "Types.h"

namespace physx
{
namespace simd
{
}
}

namespace physx
{
namespace cloth
{

class SwCloth;
class SwFabric;
struct PhaseConfig;
struct IndexPair;
struct SwTether;

// reference to cloth instance bulk data (POD)
struct SwClothData
{
	SwClothData(SwCloth&, const SwFabric&);
	void reconcile(SwCloth&) const;
	void verify() const;

	// particle data
	uint32_t mNumParticles;
	float* mCurParticles;
	float* mPrevParticles;

	float mCurBounds[6]; // lower[3], upper[3]
	float mPrevBounds[6];
	float mPadding; // write as simd

	// distance constraints
	const PhaseConfig* mConfigBegin;
	const PhaseConfig* mConfigEnd;

	const uint32_t* mPhases;
	uint32_t mNumPhases;

	const uint32_t* mSets;
	uint32_t mNumSets;

	const float* mRestvalues;
	uint32_t mNumRestvalues;

	const uint16_t* mIndices;
	uint32_t mNumIndices;

	const SwTether* mTethers;
	uint32_t mNumTethers;
	float mTetherConstraintStiffness;
	float mTetherConstraintScale;

	// wind data
	const uint16_t* mTriangles;
	uint32_t mNumTriangles;
	float mDragCoefficient;
	float mLiftCoefficient;

	// motion constraint data
	const float* mStartMotionConstraints;
	const float* mTargetMotionConstraints;
	float mMotionConstraintStiffness;

	// separation constraint data
	const float* mStartSeparationConstraints;
	const float* mTargetSeparationConstraints;

	// particle acceleration data
	const float* mParticleAccelerations;

	// collision stuff
	const float* mStartCollisionSpheres;
	const float* mTargetCollisionSpheres;
	uint32_t mNumSpheres;

	const IndexPair* mCapsuleIndices;
	uint32_t mNumCapsules;

	const float* mStartCollisionPlanes;
	const float* mTargetCollisionPlanes;
	uint32_t mNumPlanes;

	const uint32_t* mConvexMasks;
	uint32_t mNumConvexes;

	const float* mStartCollisionTriangles;
	const float* mTargetCollisionTriangles;
	uint32_t mNumCollisionTriangles;

	const uint16_t* mVirtualParticlesBegin;
	const uint16_t* mVirtualParticlesEnd;

	const float* mVirtualParticleWeights;
	uint32_t mNumVirtualParticleWeights;

	bool mEnableContinuousCollision;
	float mFrictionScale;
	float mCollisionMassScale;

	float mSelfCollisionDistance;
	float mSelfCollisionStiffness;

	uint32_t mNumSelfCollisionIndices;
	const uint32_t* mSelfCollisionIndices;

	float* mRestPositions;

	// sleep data
	uint32_t mSleepPassCounter;
	uint32_t mSleepTestCounter;

} PX_ALIGN_SUFFIX(16);
}
}
