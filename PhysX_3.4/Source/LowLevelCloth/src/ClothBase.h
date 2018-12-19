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

#include "PsMathUtils.h"

namespace physx
{
namespace cloth
{

/* helper functions shared between SwCloth and CuCloth */

template <typename Cloth>
void initialize(Cloth& cloth, const PxVec4* pIt, const PxVec4* pEnd)
{
	// initialize particles bounding box
	PxVec4 lower(FLT_MAX), upper = -lower;
	for(; pIt != pEnd; ++pIt)
	{
		lower = lower.minimum(*pIt);
		upper = upper.maximum(*pIt);
	}
	PxVec4 center = (upper + lower) * 0.5f;
	PxVec4 extent = (upper - lower) * 0.5f;
	cloth.mParticleBoundsCenter = reinterpret_cast<const PxVec3&>(center);
	cloth.mParticleBoundsHalfExtent = reinterpret_cast<const PxVec3&>(extent);

	cloth.mGravity = PxVec3(0.0f);
	cloth.mLogDamping = PxVec3(0.0f);
	cloth.mLinearLogDrag = PxVec3(0.0f);
	cloth.mAngularLogDrag = PxVec3(0.0f);
	cloth.mLinearInertia = PxVec3(1.0f);
	cloth.mAngularInertia = PxVec3(1.0f);
	cloth.mCentrifugalInertia = PxVec3(1.0f);
	cloth.mSolverFrequency = 60.0f;
	cloth.mStiffnessFrequency = 10.0f;
	cloth.mTargetMotion = PxTransform(PxIdentity);
	cloth.mCurrentMotion = PxTransform(PxIdentity);
	cloth.mLinearVelocity = PxVec3(0.0f);
	cloth.mAngularVelocity = PxVec3(0.0f);
	cloth.mPrevIterDt = 0.0f;
	cloth.mIterDtAvg = MovingAverage(30);
	cloth.mTetherConstraintLogStiffness = PxReal(-FLT_MAX_EXP);
	cloth.mTetherConstraintScale = 1.0f;
	cloth.mMotionConstraintScale = 1.0f;
	cloth.mMotionConstraintBias = 0.0f;
	cloth.mMotionConstraintLogStiffness = PxReal(-FLT_MAX_EXP);
	cloth.mWind = PxVec3(0.0f);
	cloth.mDragLogCoefficient = 0.0f;
	cloth.mLiftLogCoefficient = 0.0f;
	cloth.mEnableContinuousCollision = false;
	cloth.mCollisionMassScale = 0.0f;
	cloth.mFriction = 0.0f;
	cloth.mSelfCollisionDistance = 0.0f;
	cloth.mSelfCollisionLogStiffness = PxReal(-FLT_MAX_EXP);
	cloth.mSleepTestInterval = uint32_t(-1);
	cloth.mSleepAfterCount = uint32_t(-1);
	cloth.mSleepThreshold = 0.0f;
	cloth.mSleepPassCounter = 0;
	cloth.mSleepTestCounter = 0;
}

template <typename DstCloth, typename SrcCloth>
void copy(DstCloth& dstCloth, const SrcCloth& srcCloth)
{
	dstCloth.mParticleBoundsCenter = srcCloth.mParticleBoundsCenter;
	dstCloth.mParticleBoundsHalfExtent = srcCloth.mParticleBoundsHalfExtent;
	dstCloth.mGravity = srcCloth.mGravity;
	dstCloth.mLogDamping = srcCloth.mLogDamping;
	dstCloth.mLinearLogDrag = srcCloth.mLinearLogDrag;
	dstCloth.mAngularLogDrag = srcCloth.mAngularLogDrag;
	dstCloth.mLinearInertia = srcCloth.mLinearInertia;
	dstCloth.mAngularInertia = srcCloth.mAngularInertia;
	dstCloth.mCentrifugalInertia = srcCloth.mCentrifugalInertia;
	dstCloth.mSolverFrequency = srcCloth.mSolverFrequency;
	dstCloth.mStiffnessFrequency = srcCloth.mStiffnessFrequency;
	dstCloth.mTargetMotion = srcCloth.mTargetMotion;
	dstCloth.mCurrentMotion = srcCloth.mCurrentMotion;
	dstCloth.mLinearVelocity = srcCloth.mLinearVelocity;
	dstCloth.mAngularVelocity = srcCloth.mAngularVelocity;
	dstCloth.mPrevIterDt = srcCloth.mPrevIterDt;
	dstCloth.mIterDtAvg = srcCloth.mIterDtAvg;
	dstCloth.mTetherConstraintLogStiffness = srcCloth.mTetherConstraintLogStiffness;
	dstCloth.mTetherConstraintScale = srcCloth.mTetherConstraintScale;
	dstCloth.mMotionConstraintScale = srcCloth.mMotionConstraintScale;
	dstCloth.mMotionConstraintBias = srcCloth.mMotionConstraintBias;
	dstCloth.mMotionConstraintLogStiffness = srcCloth.mMotionConstraintLogStiffness;
	dstCloth.mWind = srcCloth.mWind;
	dstCloth.mDragLogCoefficient = srcCloth.mDragLogCoefficient;
	dstCloth.mLiftLogCoefficient = srcCloth.mLiftLogCoefficient;
	dstCloth.mEnableContinuousCollision = srcCloth.mEnableContinuousCollision;
	dstCloth.mCollisionMassScale = srcCloth.mCollisionMassScale;
	dstCloth.mFriction = srcCloth.mFriction;
	dstCloth.mSelfCollisionDistance = srcCloth.mSelfCollisionDistance;
	dstCloth.mSelfCollisionLogStiffness = srcCloth.mSelfCollisionLogStiffness;
	dstCloth.mSleepTestInterval = srcCloth.mSleepTestInterval;
	dstCloth.mSleepAfterCount = srcCloth.mSleepAfterCount;
	dstCloth.mSleepThreshold = srcCloth.mSleepThreshold;
	dstCloth.mSleepPassCounter = srcCloth.mSleepPassCounter;
	dstCloth.mSleepTestCounter = srcCloth.mSleepTestCounter;
	dstCloth.mUserData = srcCloth.mUserData;
}

} // namespace cloth
} // namespace physx
