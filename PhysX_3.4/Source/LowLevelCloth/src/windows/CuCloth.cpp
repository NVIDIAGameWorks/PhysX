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

#include "CuCloth.h"
#include "CuFabric.h"
#include "CuFactory.h"
#include "CuContextLock.h"
#include "CuCheckSuccess.h"
#include "CuClothData.h"
#include "CuSolver.h"
#include "TripletScheduler.h"
#include "ClothBase.h"
#include "Array.h"
#include "PsFoundation.h"

#if PX_VC
#pragma warning(disable : 4365) // 'action' : conversion from 'type_1' to 'type_2', signed/unsigned mismatch
#endif

namespace physx
{
namespace cloth
{
PhaseConfig transform(const PhaseConfig&); // from PhaseConfig.cpp
}
}

using namespace physx;

namespace
{
bool isSelfCollisionEnabled(const cloth::CuCloth& cloth)
{
	return PxMin(cloth.mSelfCollisionDistance, -cloth.mSelfCollisionLogStiffness) > 0.0f;
}
}

cloth::CuCloth::CuCloth(CuFactory& factory, CuFabric& fabric, Range<const PxVec4> particles)
: CuContextLock(factory)
, mFactory(factory)
, mFabric(fabric)
, mClothDataDirty(false)
, mNumParticles(uint32_t(particles.size()))
, mParticles(mFactory.mContextManager)
, mParticlesHostCopy(CuHostAllocator(mFactory.mContextManager, cudaHostAllocMapped))
, mDeviceParticlesDirty(false)
, mHostParticlesDirty(true)
, mPhaseConfigs(mFactory.mContextManager)
, mMotionConstraints(mFactory.mContextManager)
, mSeparationConstraints(mFactory.mContextManager)
, mParticleAccelerations(mFactory.mContextManager)
, mParticleAccelerationsHostCopy(CuHostAllocator(mFactory.mContextManager, cudaHostAllocMapped))
, mCapsuleIndices(getMappedAllocator<IndexPair>(mFactory.mContextManager))
, mStartCollisionSpheres(getMappedAllocator<PxVec4>(mFactory.mContextManager))
, mTargetCollisionSpheres(getMappedAllocator<PxVec4>(mFactory.mContextManager))
, mConvexMasks(getMappedAllocator<uint32_t>(mFactory.mContextManager))
, mStartCollisionPlanes(getMappedAllocator<PxVec4>(mFactory.mContextManager))
, mTargetCollisionPlanes(getMappedAllocator<PxVec4>(mFactory.mContextManager))
, mStartCollisionTriangles(getMappedAllocator<PxVec3>(mFactory.mContextManager))
, mTargetCollisionTriangles(getMappedAllocator<PxVec3>(mFactory.mContextManager))
, mVirtualParticleSetSizes(mFactory.mContextManager)
, mVirtualParticleIndices(mFactory.mContextManager)
, mVirtualParticleWeights(mFactory.mContextManager)
, mRestPositions(mFactory.mContextManager)
, mSelfCollisionIndices(mFactory.mContextManager)
, mSelfCollisionData(mFactory.mContextManager)
, mSharedMemorySize(0)
, mUserData(0)
{
	PX_ASSERT(!particles.empty());

	initialize(*this, particles.begin(), particles.end());

	mParticles.reserve(2 * mNumParticles);
	mParticles.push_back(particles.begin(), particles.end());
	mParticles.push_back(particles.begin(), particles.end());
	mParticlesHostCopy.resizeUninitialized(2 * mNumParticles);

	mFabric.incRefCount();

	CuContextLock::release();
}

cloth::CuCloth::CuCloth(CuFactory& factory, const CuCloth& cloth)
: CuContextLock(factory)
, mFactory(factory)
, mFabric(cloth.mFabric)
, mNumParticles(cloth.mNumParticles)
, mParticles(cloth.mParticles)
, mParticlesHostCopy(cloth.mParticlesHostCopy)
, mDeviceParticlesDirty(cloth.mDeviceParticlesDirty)
, mHostParticlesDirty(cloth.mHostParticlesDirty)
, mPhaseConfigs(cloth.mPhaseConfigs)
, mHostPhaseConfigs(cloth.mHostPhaseConfigs)
, mMotionConstraints(cloth.mMotionConstraints)
, mSeparationConstraints(cloth.mSeparationConstraints)
, mParticleAccelerations(cloth.mParticleAccelerations)
, mParticleAccelerationsHostCopy(cloth.mParticleAccelerationsHostCopy)
, mCapsuleIndices(cloth.mCapsuleIndices)
, mStartCollisionSpheres(cloth.mStartCollisionSpheres)
, mTargetCollisionSpheres(cloth.mTargetCollisionSpheres)
, mStartCollisionPlanes(cloth.mStartCollisionPlanes)
, mTargetCollisionPlanes(cloth.mTargetCollisionPlanes)
, mStartCollisionTriangles(cloth.mStartCollisionTriangles)
, mTargetCollisionTriangles(cloth.mTargetCollisionTriangles)
, mVirtualParticleSetSizes(cloth.mVirtualParticleSetSizes)
, mVirtualParticleIndices(cloth.mVirtualParticleIndices)
, mVirtualParticleWeights(cloth.mVirtualParticleWeights)
, mRestPositions(cloth.mRestPositions)
, mSelfCollisionIndices(cloth.mSelfCollisionIndices)
, mSelfCollisionData(mFactory.mContextManager)
, mSharedMemorySize(cloth.mSharedMemorySize)
, mUserData(cloth.mUserData)
{
	copy(*this, cloth);

	mFabric.incRefCount();

	CuContextLock::release();
}

cloth::CuCloth::~CuCloth()
{
	CuContextLock::acquire();

	mFabric.decRefCount();
}

void cloth::CuCloth::notifyChanged()
{
	mClothDataDirty = true;
}

bool cloth::CuCloth::updateClothData(CuClothData& clothData)
{
	// test particle pointer to detect when cloth data array has been reordered
	if(!mClothDataDirty && clothData.mParticles == array(*mParticles.begin().get()))
	{
		PX_ASSERT(mSharedMemorySize == getSharedMemorySize());
		return false;
	}

	mSharedMemorySize = getSharedMemorySize();

	if(mSelfCollisionData.empty() && isSelfCollisionEnabled(*this))
	{
		uint32_t numSelfCollisionIndices =
		    mSelfCollisionIndices.empty() ? mNumParticles : uint32_t(mSelfCollisionIndices.size());

		uint32_t particleSize = 4 * mNumParticles;
		uint32_t keySize = 2 * numSelfCollisionIndices;           // 2x for radix buffer
		uint32_t cellStartSize = (129 + 128 * 128 + 130) / 2 + 1; // half because type is int16_t

		// use 16bit indices for cellStart array (128x128 grid)
		mSelfCollisionData.resize(particleSize + keySize + cellStartSize);
		checkSuccess(cuMemsetD32((mSelfCollisionData.begin() + particleSize + keySize).dev(), 0xffffffff, cellStartSize));
	}

	clothData = CuClothData(*this);
	mClothDataDirty = false;

	return true;
}

uint32_t cloth::CuCloth::getSharedMemorySize() const
{
	uint32_t numPhases = uint32_t(mPhaseConfigs.size());
	uint32_t numSpheres = uint32_t(mStartCollisionSpheres.size());
	uint32_t numCones = uint32_t(mCapsuleIndices.size());
	uint32_t numPlanes = uint32_t(mStartCollisionPlanes.size());
	uint32_t numConvexes = uint32_t(mConvexMasks.size());
	uint32_t numTriangles = uint32_t(mStartCollisionTriangles.size() / 3);

	uint32_t phaseConfigSize = numPhases * sizeof(CuPhaseConfig);

	bool storePrevCollisionData = mEnableContinuousCollision || mFriction > 0.0f;
	uint32_t continuousCollisionSize = storePrevCollisionData ? 4 * numSpheres + 10 * numCones : 0;
	continuousCollisionSize += 4 * numCones + numConvexes; // capsule and convex masks
	uint32_t discreteCollisionSize = 4 * numSpheres + PxMax(10 * numCones + 96, 208u);
	discreteCollisionSize = PxMax(discreteCollisionSize, PxMax(4 * numPlanes, 19 * numTriangles));

	// scratch memory for prefix sum and histogram
	uint32_t selfCollisionSize = isSelfCollisionEnabled(*this) ? 544 : 0;

	// see CuSolverKenel.cu::gSharedMemory comment for details
	return phaseConfigSize + sizeof(float) * (continuousCollisionSize + PxMax(selfCollisionSize, discreteCollisionSize));
}

void cloth::CuCloth::setPhaseConfig(Range<const PhaseConfig> configs)
{
	mHostPhaseConfigs.assign(configs.begin(), configs.end());

	Vector<CuPhaseConfig>::Type deviceConfigs;
	deviceConfigs.reserve(configs.size());
	const PhaseConfig* cEnd = configs.end();
	for(const PhaseConfig* cIt = configs.begin(); cIt != cEnd; ++cIt)
	{
		CuPhaseConfig config;

		config.mStiffness = cIt->mStiffness;
		config.mStiffnessMultiplier = cIt->mStiffnessMultiplier;
		config.mCompressionLimit = cIt->mCompressionLimit;
		config.mStretchLimit = cIt->mStretchLimit;

		uint16_t phaseIndex = cIt->mPhaseIndex;
		config.mNumConstraints = mFabric.mNumConstraintsInPhase[phaseIndex];
		config.mRestvalues = mFabric.mRestvaluesInPhase[phaseIndex].get();
		config.mIndices = mFabric.mIndicesInPhase[phaseIndex].get();

		deviceConfigs.pushBack(config);
	}

	CuContextLock contextLock(mFactory);
	mPhaseConfigs.assign(deviceConfigs.begin(), deviceConfigs.end());
}

cloth::Range<PxVec4> cloth::CuCloth::push(cloth::CuConstraints& constraints)
{
	if(!constraints.mTarget.capacity())
	{
		CuContextLock contextLock(mFactory);
		constraints.mTarget.reserve(mNumParticles);
	}
	if(constraints.mHostCopy.empty())
		constraints.mTarget.resize(mNumParticles);

	if(constraints.mStart.empty()) // initialize start first
		constraints.mStart.swap(constraints.mTarget);

	if(!constraints.mHostCopy.capacity())
	{
		CuContextLock contextLock(mFactory);
		constraints.mHostCopy.reserve(mNumParticles);
	}
	constraints.mHostCopy.resizeUninitialized(mNumParticles);

	PxVec4* data = &constraints.mHostCopy.front();
	return Range<PxVec4>(data, data + constraints.mHostCopy.size());
}

void cloth::CuCloth::clear(cloth::CuConstraints& constraints)
{
	CuContextLock contextLock(mFactory);
	CuDeviceVector<PxVec4>(mFactory.mContextManager).swap(constraints.mStart);
	CuDeviceVector<PxVec4>(mFactory.mContextManager).swap(constraints.mTarget);
}

void cloth::CuCloth::syncDeviceParticles()
{
	if(mDeviceParticlesDirty)
	{
		CuContextLock contextLock(mFactory);
		checkSuccess(
		    cuMemcpyHtoD(mParticles.begin().dev(), mParticlesHostCopy.begin(), 2 * mNumParticles * sizeof(PxVec4)));
		mDeviceParticlesDirty = false;
	}
}

void cloth::CuCloth::syncHostParticles()
{
	if(mHostParticlesDirty)
	{
		CuContextLock contextLock(mFactory);
		const PxVec4* src = mParticles.begin().get();
		mFactory.copyToHost(src, src + 2 * mNumParticles, mParticlesHostCopy.begin());
		mHostParticlesDirty = false;
	}
}

cloth::Range<const PxVec3> cloth::CuCloth::clampTriangleCount(Range<const PxVec3> range, uint32_t replaceSize)
{
	// clamp to 500 triangles (1500 vertices) to prevent running out of shared memory
	uint32_t removedSize = mStartCollisionTriangles.size() - replaceSize;
	const PxVec3* clamp = range.begin() + 1500 - removedSize;

	if(range.end() > clamp)
	{
		shdfnd::getFoundation().error(PX_WARN, "Too many collision "
		                                       "triangles specified for cloth, dropping all but first 500.\n");
	}

	return Range<const PxVec3>(range.begin(), PxMin(range.end(), clamp));
}

#include "ClothImpl.h"

namespace physx
{
namespace cloth
{

// ClothImpl<CuCloth>::clone() implemented in CuClothClone.cpp

template <>
uint32_t ClothImpl<CuCloth>::getNumParticles() const
{
	return mCloth.mNumParticles;
}

template <>
void ClothImpl<CuCloth>::lockParticles() const
{
	const_cast<CuCloth&>(mCloth).syncHostParticles();
}

template <>
void ClothImpl<CuCloth>::unlockParticles() const
{
}

template <>
MappedRange<PxVec4> ClothImpl<CuCloth>::getCurrentParticles()
{
	mCloth.wakeUp();
	lockParticles();
	mCloth.mDeviceParticlesDirty = true;
	return getMappedParticles(mCloth.mParticlesHostCopy.begin());
}

template <>
MappedRange<const PxVec4> ClothImpl<CuCloth>::getCurrentParticles() const
{
	lockParticles();
	return getMappedParticles(mCloth.mParticlesHostCopy.begin());
}

template <>
MappedRange<PxVec4> ClothImpl<CuCloth>::getPreviousParticles()
{
	mCloth.wakeUp();
	lockParticles();
	mCloth.mDeviceParticlesDirty = true;
	return getMappedParticles(mCloth.mParticlesHostCopy.begin() + mCloth.mNumParticles);
}

template <>
MappedRange<const PxVec4> ClothImpl<CuCloth>::getPreviousParticles() const
{
	lockParticles();
	return getMappedParticles(mCloth.mParticlesHostCopy.begin() + mCloth.mNumParticles);
}

template <>
GpuParticles ClothImpl<CuCloth>::getGpuParticles()
{
	mCloth.syncDeviceParticles();
	mCloth.mHostParticlesDirty = true;
	PxVec4* particles = mCloth.mParticles.begin().get();
	GpuParticles result = { particles, particles + mCloth.mNumParticles, 0 };
	return result;
}

template <>
void ClothImpl<CuCloth>::setPhaseConfig(Range<const PhaseConfig> configs)
{
	Vector<PhaseConfig>::Type transformedConfigs;
	transformedConfigs.reserve(configs.size());

	// transform phase config to use in solver
	for(; !configs.empty(); configs.popFront())
		if(configs.front().mStiffness > 0.0f)
			transformedConfigs.pushBack(transform(configs.front()));

	mCloth.setPhaseConfig(Range<const PhaseConfig>(transformedConfigs.begin(), transformedConfigs.end()));
	mCloth.notifyChanged();
	mCloth.wakeUp();
}

template <>
void ClothImpl<CuCloth>::setSelfCollisionIndices(Range<const uint32_t> indices)
{
	ContextLockType lock(mCloth.mFactory);
	mCloth.mSelfCollisionIndices.assign(indices.begin(), indices.end());
	mCloth.mSelfCollisionIndicesHost.assign(indices.begin(), indices.end());
	mCloth.notifyChanged();
	mCloth.wakeUp();
}

template <>
uint32_t ClothImpl<CuCloth>::getNumVirtualParticles() const
{
	return uint32_t(mCloth.mVirtualParticleIndices.size());
}

template <>
Range<PxVec4> ClothImpl<CuCloth>::getParticleAccelerations()
{
	if(mCloth.mParticleAccelerations.empty())
	{
		CuContextLock contextLock(mCloth.mFactory);
		mCloth.mParticleAccelerations.resize(mCloth.mNumParticles);
	}

	if(!mCloth.mParticleAccelerationsHostCopy.capacity())
	{
		CuContextLock contextLock(mCloth.mFactory);
		mCloth.mParticleAccelerationsHostCopy.reserve(mCloth.mNumParticles);
	}
	mCloth.mParticleAccelerationsHostCopy.resizeUninitialized(mCloth.mNumParticles);

	mCloth.wakeUp();

	PxVec4* data = mCloth.mParticleAccelerationsHostCopy.begin();
	return Range<PxVec4>(data, mCloth.mParticleAccelerationsHostCopy.end());
}

template <>
void ClothImpl<CuCloth>::clearParticleAccelerations()
{
	CuContextLock contextLock(mCloth.mFactory);
	CuDeviceVector<PxVec4>(mCloth.mFactory.mContextManager).swap(mCloth.mParticleAccelerations);
	mCloth.mParticleAccelerationsHostCopy.reset();
	mCloth.wakeUp();
}

template <>
void ClothImpl<CuCloth>::setVirtualParticles(Range<const uint32_t[4]> indices, Range<const PxVec3> weights)
{
	// shuffle indices to form independent SIMD sets
	TripletScheduler scheduler(indices);
	scheduler.warp(mCloth.mNumParticles, 32);

	// convert to 16bit indices
	Vector<Vec4us>::Type hostIndices;
	hostIndices.reserve(indices.size());
	TripletScheduler::ConstTripletIter tIt = scheduler.mTriplets.begin();
	TripletScheduler::ConstTripletIter tEnd = scheduler.mTriplets.end();
	for(; tIt != tEnd; ++tIt)
		hostIndices.pushBack(Vec4us(*tIt));

	// printf("num sets = %u, num replays = %u\n", scheduler.mSetSizes.size(),
	//	calculateNumReplays(scheduler.mTriplets, scheduler.mSetSizes));

	// add normalization weight
	Vector<PxVec4>::Type hostWeights;
	hostWeights.reserve(weights.size());
	for(; !weights.empty(); weights.popFront())
	{
		PxVec3 w = reinterpret_cast<const PxVec3&>(weights.front());
		PxReal scale = 1 / w.magnitudeSquared();
		hostWeights.pushBack(PxVec4(w.x, w.y, w.z, scale));
	}

	CuContextLock contextLock(mCloth.mFactory);

	// todo: 'swap' these to force reallocation?
	mCloth.mVirtualParticleIndices = hostIndices;
	mCloth.mVirtualParticleSetSizes = scheduler.mSetSizes;
	mCloth.mVirtualParticleWeights = hostWeights;

	mCloth.notifyChanged();
	mCloth.wakeUp();
}

} // namespace cloth
} // namespace physx
