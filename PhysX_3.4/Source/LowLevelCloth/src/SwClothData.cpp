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

#include "SwClothData.h"
#include "SwCloth.h"
#include "SwFabric.h"
#include "PsUtilities.h"
#include "PsMathUtils.h"
#include "CmPhysXCommon.h"

using namespace physx;

cloth::SwClothData::SwClothData(SwCloth& cloth, const SwFabric& fabric)
{
	mNumParticles = uint32_t(cloth.mCurParticles.size());
	mCurParticles = array(cloth.mCurParticles.front());
	mPrevParticles = array(cloth.mPrevParticles.front());

	const float* center = array(cloth.mParticleBoundsCenter);
	const float* extent = array(cloth.mParticleBoundsHalfExtent);
	for(uint32_t i = 0; i < 3; ++i)
	{
		mCurBounds[i] = center[i] - extent[i];
		mCurBounds[i + 3] = center[i] + extent[i];
	}

	// avoid reading uninitialized data into mCurBounds, even though it's never used.
	mPrevBounds[0] = 0.0f;

	mConfigBegin = cloth.mPhaseConfigs.empty() ? 0 : &cloth.mPhaseConfigs.front();
	mConfigEnd = mConfigBegin + cloth.mPhaseConfigs.size();

	mPhases = &fabric.mPhases.front();
	mNumPhases = uint32_t(fabric.mPhases.size());

	mSets = &fabric.mSets.front();
	mNumSets = uint32_t(fabric.mSets.size());

	mRestvalues = &fabric.mRestvalues.front();
	mNumRestvalues = uint32_t(fabric.mRestvalues.size());

	mIndices = &fabric.mIndices.front();
	mNumIndices = uint32_t(fabric.mIndices.size());

	float stiffnessExponent = cloth.mStiffnessFrequency * cloth.mPrevIterDt * 0.69314718055994531f; // logf(2.0f);

	mTethers = fabric.mTethers.begin();
	mNumTethers = uint32_t(fabric.mTethers.size());
	mTetherConstraintStiffness = 1.0f - Ps::exp(stiffnessExponent * cloth.mTetherConstraintLogStiffness);
	mTetherConstraintScale = cloth.mTetherConstraintScale * fabric.mTetherLengthScale;

	mTriangles = fabric.mTriangles.begin();
	mNumTriangles = uint32_t(fabric.mTriangles.size()) / 3;
	mDragCoefficient = 1.0f - Ps::exp(stiffnessExponent * cloth.mDragLogCoefficient);
	mLiftCoefficient = 1.0f - Ps::exp(stiffnessExponent * cloth.mLiftLogCoefficient);

	mStartMotionConstraints = cloth.mMotionConstraints.mStart.size() ? array(cloth.mMotionConstraints.mStart.front()) : 0;
	mTargetMotionConstraints =
	    !cloth.mMotionConstraints.mTarget.empty() ? array(cloth.mMotionConstraints.mTarget.front()) : 0;
	mMotionConstraintStiffness = 1.0f - Ps::exp(stiffnessExponent * cloth.mMotionConstraintLogStiffness);

	mStartSeparationConstraints =
	    cloth.mSeparationConstraints.mStart.size() ? array(cloth.mSeparationConstraints.mStart.front()) : 0;
	mTargetSeparationConstraints =
	    !cloth.mSeparationConstraints.mTarget.empty() ? array(cloth.mSeparationConstraints.mTarget.front()) : 0;

	mParticleAccelerations = cloth.mParticleAccelerations.size() ? array(cloth.mParticleAccelerations.front()) : 0;

	mStartCollisionSpheres = cloth.mStartCollisionSpheres.empty() ? 0 : array(cloth.mStartCollisionSpheres.front());
	mTargetCollisionSpheres =
	    cloth.mTargetCollisionSpheres.empty() ? mStartCollisionSpheres : array(cloth.mTargetCollisionSpheres.front());
	mNumSpheres = uint32_t(cloth.mStartCollisionSpheres.size());

	mCapsuleIndices = cloth.mCapsuleIndices.empty() ? 0 : &cloth.mCapsuleIndices.front();
	mNumCapsules = uint32_t(cloth.mCapsuleIndices.size());

	mStartCollisionPlanes = cloth.mStartCollisionPlanes.empty() ? 0 : array(cloth.mStartCollisionPlanes.front());
	mTargetCollisionPlanes =
	    cloth.mTargetCollisionPlanes.empty() ? mStartCollisionPlanes : array(cloth.mTargetCollisionPlanes.front());
	mNumPlanes = uint32_t(cloth.mStartCollisionPlanes.size());

	mConvexMasks = cloth.mConvexMasks.empty() ? 0 : &cloth.mConvexMasks.front();
	mNumConvexes = uint32_t(cloth.mConvexMasks.size());

	mStartCollisionTriangles = cloth.mStartCollisionTriangles.empty() ? 0 : array(cloth.mStartCollisionTriangles.front());
	mTargetCollisionTriangles = cloth.mTargetCollisionTriangles.empty() ? mStartCollisionTriangles
	                                                                    : array(cloth.mTargetCollisionTriangles.front());
	mNumCollisionTriangles = uint32_t(cloth.mStartCollisionTriangles.size()) / 3;

	mVirtualParticlesBegin = cloth.mVirtualParticleIndices.empty() ? 0 : array(cloth.mVirtualParticleIndices.front());
	mVirtualParticlesEnd = mVirtualParticlesBegin + 4 * cloth.mVirtualParticleIndices.size();
	mVirtualParticleWeights = cloth.mVirtualParticleWeights.empty() ? 0 : array(cloth.mVirtualParticleWeights.front());
	mNumVirtualParticleWeights = uint32_t(cloth.mVirtualParticleWeights.size());

	mEnableContinuousCollision = cloth.mEnableContinuousCollision;
	mCollisionMassScale = cloth.mCollisionMassScale;
	mFrictionScale = cloth.mFriction;

	mSelfCollisionDistance = cloth.mSelfCollisionDistance;
	mSelfCollisionStiffness = 1.0f - Ps::exp(stiffnessExponent * cloth.mSelfCollisionLogStiffness);

	mSelfCollisionIndices = cloth.mSelfCollisionIndices.empty() ? 0 : cloth.mSelfCollisionIndices.begin();
	mNumSelfCollisionIndices = mSelfCollisionIndices ? cloth.mSelfCollisionIndices.size() : mNumParticles;

	mRestPositions = cloth.mRestPositions.size() ? array(cloth.mRestPositions.front()) : 0;

	mSleepPassCounter = cloth.mSleepPassCounter;
	mSleepTestCounter = cloth.mSleepTestCounter;
}

void cloth::SwClothData::reconcile(SwCloth& cloth) const
{
	cloth.setParticleBounds(mCurBounds);
	cloth.mSleepTestCounter = mSleepTestCounter;
	cloth.mSleepPassCounter = mSleepPassCounter;
}

void cloth::SwClothData::verify() const
{
	// checks needs to be run after the constructor because
	// data isn't immediately available on SPU at that stage
	// perhaps a good reason to construct SwClothData on PPU instead

	PX_ASSERT(!mNumCapsules ||
	          mNumSpheres > *shdfnd::maxElement(&mCapsuleIndices->first, &(mCapsuleIndices + mNumCapsules)->first));

	PX_ASSERT(!mNumConvexes || (1u << mNumPlanes) - 1 >= *shdfnd::maxElement(mConvexMasks, mConvexMasks + mNumConvexes));
}
