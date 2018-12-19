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
#include "foundation/PxVec4.h"
#include "Range.h"
#include "PhaseConfig.h"
#include "MovingAverage.h"
#include "IndexPair.h"
#include "BoundingBox.h"
#include "Vec4T.h"
#include "CuPhaseConfig.h"
#include "CuPinnedAllocator.h"
#include "CuContextLock.h"
#include "CuDeviceVector.h"

namespace physx
{
namespace cloth
{

class CuFabric;
class CuFactory;
struct CuClothData;

struct CuConstraints
{
	CuConstraints(physx::PxCudaContextManager* ctx)
	: mStart(ctx), mTarget(ctx), mHostCopy(CuHostAllocator(ctx, cudaHostAllocMapped))
	{
	}

	void pop()
	{
		if(!mTarget.empty())
		{
			mStart.swap(mTarget);
			mTarget.resize(0);
		}
	}

	CuDeviceVector<PxVec4> mStart;
	CuDeviceVector<PxVec4> mTarget;
	CuPinnedVector<PxVec4>::Type mHostCopy;
};

class CuCloth : protected CuContextLock
{
	CuCloth& operator=(const CuCloth&);

public:
	typedef CuFactory FactoryType;
	typedef CuFabric FabricType;
	typedef CuContextLock ContextLockType;

	typedef CuPinnedVector<PxVec4>::Type& MappedVec4fVectorType;
	typedef CuPinnedVector<IndexPair>::Type& MappedIndexVectorType;

	CuCloth(CuFactory&, CuFabric&, Range<const PxVec4>);
	CuCloth(CuFactory&, const CuCloth&);
	~CuCloth(); // not virtual on purpose

  public:
	bool isSleeping() const
	{
		return mSleepPassCounter >= mSleepAfterCount;
	}
	void wakeUp()
	{
		mSleepPassCounter = 0;
	}

	void notifyChanged();

	bool updateClothData(CuClothData&);   // expects acquired context
	uint32_t getSharedMemorySize() const; // without particle data

	// expects transformed configs, doesn't call notifyChanged()
	void setPhaseConfig(Range<const PhaseConfig>);

	Range<PxVec4> push(CuConstraints&);
	void clear(CuConstraints&);

	void syncDeviceParticles();
	void syncHostParticles();

	Range<const PxVec3> clampTriangleCount(Range<const PxVec3>, uint32_t);

  public:
	CuFactory& mFactory;
	CuFabric& mFabric;

	bool mClothDataDirty;

	// particle data
	uint32_t mNumParticles;
	CuDeviceVector<PxVec4> mParticles; // cur, prev
	CuPinnedVector<PxVec4>::Type mParticlesHostCopy;
	bool mDeviceParticlesDirty;
	bool mHostParticlesDirty;

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

	CuDeviceVector<CuPhaseConfig> mPhaseConfigs; // transformed!
	Vector<PhaseConfig>::Type mHostPhaseConfigs; // transformed!

	// tether constraints stuff
	float mTetherConstraintLogStiffness;
	float mTetherConstraintScale;

	// motion constraints stuff
	CuConstraints mMotionConstraints;
	float mMotionConstraintScale;
	float mMotionConstraintBias;
	float mMotionConstraintLogStiffness;

	// separation constraints stuff
	CuConstraints mSeparationConstraints;

	// particle acceleration stuff
	CuDeviceVector<PxVec4> mParticleAccelerations;
	CuPinnedVector<PxVec4>::Type mParticleAccelerationsHostCopy;

	// wind
	PxVec3 mWind;
	float mDragLogCoefficient;
	float mLiftLogCoefficient;

	// collision stuff
	CuPinnedVector<IndexPair>::Type mCapsuleIndices;
	CuPinnedVector<PxVec4>::Type mStartCollisionSpheres;
	CuPinnedVector<PxVec4>::Type mTargetCollisionSpheres;
	CuPinnedVector<uint32_t>::Type mConvexMasks;
	CuPinnedVector<PxVec4>::Type mStartCollisionPlanes;
	CuPinnedVector<PxVec4>::Type mTargetCollisionPlanes;
	CuPinnedVector<PxVec3>::Type mStartCollisionTriangles;
	CuPinnedVector<PxVec3>::Type mTargetCollisionTriangles;
	bool mEnableContinuousCollision;
	float mCollisionMassScale;
	float mFriction;

	// virtual particles
	CuDeviceVector<uint32_t> mVirtualParticleSetSizes;
	CuDeviceVector<Vec4us> mVirtualParticleIndices;
	CuDeviceVector<PxVec4> mVirtualParticleWeights;

	// self collision
	float mSelfCollisionDistance;
	float mSelfCollisionLogStiffness;

	CuDeviceVector<PxVec4> mRestPositions;
	CuDeviceVector<uint32_t> mSelfCollisionIndices;
	Vector<uint32_t>::Type mSelfCollisionIndicesHost;

	// 4 (position) + 2 (key) per particle + cellStart (8322)
	CuDeviceVector<float> mSelfCollisionData;

	// sleeping (see SwCloth for comments)
	uint32_t mSleepTestInterval;
	uint32_t mSleepAfterCount;
	float mSleepThreshold;
	uint32_t mSleepPassCounter;
	uint32_t mSleepTestCounter;

	uint32_t mSharedMemorySize;

	void* mUserData;
};
}
}
