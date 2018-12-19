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

#include "foundation/PxTransform.h"
#include "Cloth.h"
#include "Range.h"
#include "MovingAverage.h"
#include "PhaseConfig.h"
#include "IndexPair.h"
#include "Vec4T.h"
#include "Array.h"

namespace physx
{

namespace cloth
{

class SwFabric;
class SwFactory;

typedef AlignedVector<PxVec4, 16>::Type Vec4fAlignedVector;

struct SwConstraints
{
	void pop()
	{
		if(!mTarget.empty())
		{
			mStart.swap(mTarget);
			mTarget.resize(0);
		}
	}

	Vec4fAlignedVector mStart;
	Vec4fAlignedVector mTarget;
};

class SwCloth
{
	SwCloth& operator=(const SwCloth&); // not implemented
	struct SwContextLock
	{
		SwContextLock(const SwFactory&)
		{
		}
	};

  public:
	typedef SwFactory FactoryType;
	typedef SwFabric FabricType;
	typedef SwContextLock ContextLockType;

	typedef Vec4fAlignedVector& MappedVec4fVectorType;
	typedef Vector<IndexPair>::Type& MappedIndexVectorType;

	SwCloth(SwFactory&, SwFabric&, Range<const PxVec4>);
	SwCloth(SwFactory&, const SwCloth&);
	~SwCloth(); // not virtual on purpose

  public:
	bool isSleeping() const
	{
		return mSleepPassCounter >= mSleepAfterCount;
	}
	void wakeUp()
	{
		mSleepPassCounter = 0;
	}

	void notifyChanged()
	{
	}

	void setParticleBounds(const float*);

	Range<PxVec4> push(SwConstraints&);
	static void clear(SwConstraints&);

	static Range<const PxVec3> clampTriangleCount(Range<const PxVec3>, uint32_t);

  public:
	SwFactory& mFactory;
	SwFabric& mFabric;

	// current and previous-iteration particle positions
	Vec4fAlignedVector mCurParticles;
	Vec4fAlignedVector mPrevParticles;

	PxVec3 mParticleBoundsCenter;
	PxVec3 mParticleBoundsHalfExtent;

	PxVec3 mGravity;
	PxVec3 mLogDamping;
	PxVec3 mLinearLogDrag;
	PxVec3 mAngularLogDrag;
	PxVec3 mLinearInertia;
	PxVec3 mAngularInertia;
	PxVec3 mCentrifugalInertia;
	float mSolverFrequency;
	float mStiffnessFrequency;

	PxTransform mTargetMotion;
	PxTransform mCurrentMotion;
	PxVec3 mLinearVelocity;
	PxVec3 mAngularVelocity;

	float mPrevIterDt;
	MovingAverage mIterDtAvg;

	Vector<PhaseConfig>::Type mPhaseConfigs; // transformed!

	// tether constraints stuff
	float mTetherConstraintLogStiffness;
	float mTetherConstraintScale;

	// motion constraints stuff
	SwConstraints mMotionConstraints;
	float mMotionConstraintScale;
	float mMotionConstraintBias;
	float mMotionConstraintLogStiffness;

	// separation constraints stuff
	SwConstraints mSeparationConstraints;

	// particle acceleration stuff
	Vec4fAlignedVector mParticleAccelerations;

	// wind
	PxVec3 mWind;
	float mDragLogCoefficient;
	float mLiftLogCoefficient;

	// collision stuff
	Vector<IndexPair>::Type mCapsuleIndices;
	Vec4fAlignedVector mStartCollisionSpheres;
	Vec4fAlignedVector mTargetCollisionSpheres;
	Vector<uint32_t>::Type mConvexMasks;
	Vec4fAlignedVector mStartCollisionPlanes;
	Vec4fAlignedVector mTargetCollisionPlanes;
	Vector<PxVec3>::Type mStartCollisionTriangles;
	Vector<PxVec3>::Type mTargetCollisionTriangles;
	bool mEnableContinuousCollision;
	float mCollisionMassScale;
	float mFriction;

	// virtual particles
	Vector<Vec4us>::Type mVirtualParticleIndices;
	Vec4fAlignedVector mVirtualParticleWeights;
	uint32_t mNumVirtualParticles;

	// self collision
	float mSelfCollisionDistance;
	float mSelfCollisionLogStiffness;

	Vector<uint32_t>::Type mSelfCollisionIndices;

	Vec4fAlignedVector mRestPositions;

	// sleeping
	uint32_t mSleepTestInterval; // how often to test for movement
	uint32_t mSleepAfterCount;   // number of tests to pass before sleep
	float mSleepThreshold;       // max movement delta to pass test
	uint32_t mSleepPassCounter;  // how many tests passed
	uint32_t mSleepTestCounter;  // how many iterations since tested

	void* mUserData;

} PX_ALIGN_SUFFIX(16);

} // namespace cloth

// bounds = lower[3], upper[3]
inline void cloth::SwCloth::setParticleBounds(const float* bounds)
{
	for(uint32_t i = 0; i < 3; ++i)
	{
		mParticleBoundsCenter[i] = (bounds[3 + i] + bounds[i]) * 0.5f;
		mParticleBoundsHalfExtent[i] = (bounds[3 + i] - bounds[i]) * 0.5f;
	}
}
}
