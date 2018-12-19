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

#include "Cloth.h"
#include "Fabric.h"
#include "Allocator.h"
#include "PsMathUtils.h"

namespace physx
{
namespace cloth
{

// SwCloth or CuCloth aggregate implementing the Cloth interface
// Member specializations are implemented in Sw/CuCloth.cpp
template <typename T>
class ClothImpl : public UserAllocated, public Cloth
{
	ClothImpl(const ClothImpl&);
	ClothImpl& operator=(const ClothImpl&);

  public:

	typedef T ClothType;
	typedef typename ClothType::FactoryType FactoryType;
	typedef typename ClothType::FabricType FabricType;
	typedef typename ClothType::ContextLockType ContextLockType;

	ClothImpl(Factory&, Fabric&, Range<const PxVec4>);
	ClothImpl(Factory&, const ClothImpl&);

	virtual Cloth* clone(Factory& factory) const;

	virtual Fabric& getFabric() const;
	virtual Factory& getFactory() const;

	virtual uint32_t getNumParticles() const;
	virtual void lockParticles() const;
	virtual void unlockParticles() const;
	virtual MappedRange<PxVec4> getCurrentParticles();
	virtual MappedRange<const PxVec4> getCurrentParticles() const;
	virtual MappedRange<PxVec4> getPreviousParticles();
	virtual MappedRange<const PxVec4> getPreviousParticles() const;
	virtual GpuParticles getGpuParticles();

	virtual void setTranslation(const PxVec3& trans);
	virtual void setRotation(const PxQuat& rot);

	virtual const PxVec3& getTranslation() const;
	virtual const PxQuat& getRotation() const;

	virtual void clearInertia();

	virtual void teleport(const PxVec3& delta);

	virtual float getPreviousIterationDt() const;
	virtual void setGravity(const PxVec3& gravity);
	virtual PxVec3 getGravity() const;
	virtual void setDamping(const PxVec3& damping);
	virtual PxVec3 getDamping() const;
	virtual void setLinearDrag(const PxVec3& drag);
	virtual PxVec3 getLinearDrag() const;
	virtual void setAngularDrag(const PxVec3& drag);
	virtual PxVec3 getAngularDrag() const;
	virtual void setLinearInertia(const PxVec3& inertia);
	virtual PxVec3 getLinearInertia() const;
	virtual void setAngularInertia(const PxVec3& inertia);
	virtual PxVec3 getAngularInertia() const;
	virtual void setCentrifugalInertia(const PxVec3& inertia);
	virtual PxVec3 getCentrifugalInertia() const;

	virtual void setSolverFrequency(float frequency);
	virtual float getSolverFrequency() const;

	virtual void setStiffnessFrequency(float frequency);
	virtual float getStiffnessFrequency() const;

	virtual void setAcceleationFilterWidth(uint32_t);
	virtual uint32_t getAccelerationFilterWidth() const;

	virtual void setPhaseConfig(Range<const PhaseConfig> configs);

	virtual void setSpheres(Range<const PxVec4>, uint32_t first, uint32_t last);
	virtual uint32_t getNumSpheres() const;

	virtual void setCapsules(Range<const uint32_t>, uint32_t first, uint32_t last);
	virtual uint32_t getNumCapsules() const;

	virtual void setPlanes(Range<const PxVec4>, uint32_t first, uint32_t last);
	virtual uint32_t getNumPlanes() const;

	virtual void setConvexes(Range<const uint32_t>, uint32_t first, uint32_t last);
	virtual uint32_t getNumConvexes() const;

	virtual void setTriangles(Range<const PxVec3>, uint32_t first, uint32_t last);
	virtual void setTriangles(Range<const PxVec3>, Range<const PxVec3>, uint32_t first);
	virtual uint32_t getNumTriangles() const;

	virtual bool isContinuousCollisionEnabled() const;
	virtual void enableContinuousCollision(bool);

	virtual float getCollisionMassScale() const;
	virtual void setCollisionMassScale(float);
	virtual void setFriction(float friction);
	virtual float getFriction() const;

	virtual void setVirtualParticles(Range<const uint32_t[4]>, Range<const PxVec3>);
	virtual uint32_t getNumVirtualParticles() const;
	virtual uint32_t getNumVirtualParticleWeights() const;

	virtual void setTetherConstraintScale(float scale);
	virtual float getTetherConstraintScale() const;
	virtual void setTetherConstraintStiffness(float stiffness);
	virtual float getTetherConstraintStiffness() const;

	virtual Range<PxVec4> getMotionConstraints();
	virtual void clearMotionConstraints();
	virtual uint32_t getNumMotionConstraints() const;
	virtual void setMotionConstraintScaleBias(float scale, float bias);
	virtual float getMotionConstraintScale() const;
	virtual float getMotionConstraintBias() const;
	virtual void setMotionConstraintStiffness(float stiffness);
	virtual float getMotionConstraintStiffness() const;

	virtual Range<PxVec4> getSeparationConstraints();
	virtual void clearSeparationConstraints();
	virtual uint32_t getNumSeparationConstraints() const;

	virtual void clearInterpolation();

	virtual Range<PxVec4> getParticleAccelerations();
	virtual void clearParticleAccelerations();
	virtual uint32_t getNumParticleAccelerations() const;

	virtual void setWindVelocity(PxVec3);
	virtual PxVec3 getWindVelocity() const;
	virtual void setDragCoefficient(float);
	virtual float getDragCoefficient() const;
	virtual void setLiftCoefficient(float);
	virtual float getLiftCoefficient() const;

	virtual void setSelfCollisionDistance(float);
	virtual float getSelfCollisionDistance() const;
	virtual void setSelfCollisionStiffness(float);
	virtual float getSelfCollisionStiffness() const;

	virtual void setSelfCollisionIndices(Range<const uint32_t>);
	virtual uint32_t getNumSelfCollisionIndices() const;

	virtual void setRestPositions(Range<const PxVec4>);
	virtual uint32_t getNumRestPositions() const;

	virtual const PxVec3& getBoundingBoxCenter() const;
	virtual const PxVec3& getBoundingBoxScale() const;

	virtual void setSleepThreshold(float);
	virtual float getSleepThreshold() const;
	virtual void setSleepTestInterval(uint32_t);
	virtual uint32_t getSleepTestInterval() const;
	virtual void setSleepAfterCount(uint32_t);
	virtual uint32_t getSleepAfterCount() const;
	virtual uint32_t getSleepPassCount() const;
	virtual bool isAsleep() const;
	virtual void putToSleep();
	virtual void wakeUp();

	virtual void setUserData(void*);
	virtual void* getUserData() const;

	// helper function
	template <typename U>
	MappedRange<U> getMappedParticles(U* data) const;

	ClothType mCloth;
};

class SwCloth;
typedef ClothImpl<SwCloth> SwClothImpl;

class CuCloth;
typedef ClothImpl<CuCloth> CuClothImpl;

class DxCloth;
typedef ClothImpl<DxCloth> DxClothImpl;

template <typename T>
ClothImpl<T>::ClothImpl(Factory& factory, Fabric& fabric, Range<const PxVec4> particles)
: mCloth(static_cast<FactoryType&>(factory), static_cast<FabricType&>(fabric), particles)
{
	// fabric and cloth need to be created by the same factory
	PX_ASSERT(&fabric.getFactory() == &factory);
}

template <typename T>
ClothImpl<T>::ClothImpl(Factory& factory, const ClothImpl& impl)
: mCloth(static_cast<FactoryType&>(factory), impl.mCloth)
{
}

template <typename T>
inline Fabric& ClothImpl<T>::getFabric() const
{
	return mCloth.mFabric;
}

template <typename T>
inline Factory& ClothImpl<T>::getFactory() const
{
	return mCloth.mFactory;
}

template <typename T>
inline void ClothImpl<T>::setTranslation(const PxVec3& trans)
{
	PxVec3 t = reinterpret_cast<const PxVec3&>(trans);
	if(t == mCloth.mTargetMotion.p)
		return;

	mCloth.mTargetMotion.p = t;
	mCloth.wakeUp();
}

template <typename T>
inline void ClothImpl<T>::setRotation(const PxQuat& q)
{
	if((q - mCloth.mTargetMotion.q).magnitudeSquared() == 0.0f)
		return;

	mCloth.mTargetMotion.q = q;
	mCloth.wakeUp();
}

template <typename T>
inline const PxVec3& ClothImpl<T>::getTranslation() const
{
	return mCloth.mTargetMotion.p;
}

template <typename T>
inline const PxQuat& ClothImpl<T>::getRotation() const
{
	return mCloth.mTargetMotion.q;
}

template <typename T>
inline void ClothImpl<T>::clearInertia()
{
	mCloth.mCurrentMotion = mCloth.mTargetMotion;
	mCloth.mLinearVelocity = PxVec3(0.0f);
	mCloth.mAngularVelocity = PxVec3(0.0f);

	mCloth.wakeUp();
}

// Fixed 4505:local function has been removed
template <typename T>
inline void ClothImpl<T>::teleport(const PxVec3& delta)
{
	mCloth.mCurrentMotion.p += delta;
	mCloth.mTargetMotion.p += delta;
}

template <typename T>
inline float ClothImpl<T>::getPreviousIterationDt() const
{
	return mCloth.mPrevIterDt;
}

template <typename T>
inline void ClothImpl<T>::setGravity(const PxVec3& gravity)
{
	PxVec3 value = gravity;
	if(value == mCloth.mGravity)
		return;

	mCloth.mGravity = value;
	mCloth.wakeUp();
}

template <typename T>
inline PxVec3 ClothImpl<T>::getGravity() const
{
	return mCloth.mGravity;
}

inline float safeLog2(float x)
{
	return x ? shdfnd::log2(x) : -FLT_MAX_EXP;
}

inline PxVec3 safeLog2(const PxVec3& v)
{
	return PxVec3(safeLog2(v.x), safeLog2(v.y), safeLog2(v.z));
}

inline float safeExp2(float x)
{
	if(x <= -FLT_MAX_EXP)
		return 0.0f;
	else
		return shdfnd::exp2(x);
}

inline PxVec3 safeExp2(const PxVec3& v)
{
	return PxVec3(safeExp2(v.x), safeExp2(v.y), safeExp2(v.z));
}

template <typename T>
inline void ClothImpl<T>::setDamping(const PxVec3& damping)
{
	PxVec3 value = safeLog2(PxVec3(1.f) - damping);
	if(value == mCloth.mLogDamping)
		return;

	mCloth.mLogDamping = value;
	mCloth.wakeUp();
}

template <typename T>
inline PxVec3 ClothImpl<T>::getDamping() const
{
	return PxVec3(1.f) - safeExp2(mCloth.mLogDamping);
}

template <typename T>
inline void ClothImpl<T>::setLinearDrag(const PxVec3& drag)
{
	PxVec3 value = safeLog2(PxVec3(1.f) - drag);
	if(value == mCloth.mLinearLogDrag)
		return;

	mCloth.mLinearLogDrag = value;
	mCloth.wakeUp();
}

template <typename T>
inline PxVec3 ClothImpl<T>::getLinearDrag() const
{
	return PxVec3(1.f) - safeExp2(mCloth.mLinearLogDrag);
}

template <typename T>
inline void ClothImpl<T>::setAngularDrag(const PxVec3& drag)
{
	PxVec3 value = safeLog2(PxVec3(1.f) - drag);
	if(value == mCloth.mAngularLogDrag)
		return;

	mCloth.mAngularLogDrag = value;
	mCloth.wakeUp();
}

template <typename T>
inline PxVec3 ClothImpl<T>::getAngularDrag() const
{
	return PxVec3(1.f) - safeExp2(mCloth.mAngularLogDrag);
}

template <typename T>
inline void ClothImpl<T>::setLinearInertia(const PxVec3& inertia)
{
	PxVec3 value = inertia;
	if(value == mCloth.mLinearInertia)
		return;

	mCloth.mLinearInertia = value;
	mCloth.wakeUp();
}

template <typename T>
inline PxVec3 ClothImpl<T>::getLinearInertia() const
{
	return mCloth.mLinearInertia;
}

template <typename T>
inline void ClothImpl<T>::setAngularInertia(const PxVec3& inertia)
{
	PxVec3 value = inertia;
	if(value == mCloth.mAngularInertia)
		return;

	mCloth.mAngularInertia = value;
	mCloth.wakeUp();
}

template <typename T>
inline PxVec3 ClothImpl<T>::getAngularInertia() const
{
	return mCloth.mAngularInertia;
}

template <typename T>
inline void ClothImpl<T>::setCentrifugalInertia(const PxVec3& inertia)
{
	PxVec3 value = inertia;
	if(value == mCloth.mCentrifugalInertia)
		return;

	mCloth.mCentrifugalInertia = value;
	mCloth.wakeUp();
}

template <typename T>
inline PxVec3 ClothImpl<T>::getCentrifugalInertia() const
{
	return mCloth.mCentrifugalInertia;
}

template <typename T>
inline void ClothImpl<T>::setSolverFrequency(float frequency)
{
	if(frequency == mCloth.mSolverFrequency)
		return;

	mCloth.mSolverFrequency = frequency;
	mCloth.mIterDtAvg.reset();
	mCloth.wakeUp();
}

template <typename T>
inline float ClothImpl<T>::getSolverFrequency() const
{
	return mCloth.mSolverFrequency;
}

template <typename T>
inline void ClothImpl<T>::setStiffnessFrequency(float frequency)
{
	if(frequency == mCloth.mStiffnessFrequency)
		return;

	mCloth.mStiffnessFrequency = frequency;
	mCloth.wakeUp();
}

template <typename T>
inline float ClothImpl<T>::getStiffnessFrequency() const
{
	return mCloth.mStiffnessFrequency;
}

template <typename T>
inline void ClothImpl<T>::setAcceleationFilterWidth(uint32_t n)
{
	mCloth.mIterDtAvg.resize(n);
}

template <typename T>
inline uint32_t ClothImpl<T>::getAccelerationFilterWidth() const
{
	return mCloth.mIterDtAvg.size();
}

// move a subarray
template <typename Iter>
void move(Iter it, uint32_t first, uint32_t last, uint32_t result)
{
	if(result > first)
	{
		result += last - first;
		while(first < last)
			it[--result] = it[--last];
	}
	else
	{
		while(first < last)
			it[result++] = it[first++];
	}
}

// update capsule index
inline bool updateIndex(uint32_t& index, uint32_t first, int32_t delta)
{
	return index >= first && int32_t(index += delta) < int32_t(first);
}

template <typename T>
inline void ClothImpl<T>::setSpheres(Range<const PxVec4> spheres, uint32_t first, uint32_t last)
{
	uint32_t oldSize = uint32_t(mCloth.mStartCollisionSpheres.size());
	uint32_t newSize = uint32_t(spheres.size()) + oldSize - last + first;

	PX_ASSERT(newSize <= 32);
	PX_ASSERT(first <= oldSize);
	PX_ASSERT(last <= oldSize);

#if PX_DEBUG
	for(const PxVec4* it = spheres.begin(); it < spheres.end(); ++it)
		PX_ASSERT(it->w >= 0.0f);
#endif

	if(!oldSize && !newSize)
		return;

	if(!oldSize)
	{
		ContextLockType contextLock(mCloth.mFactory);
		mCloth.mStartCollisionSpheres.assign(spheres.begin(), spheres.end());
		mCloth.notifyChanged();
	}
	else
	{
		if(PxMax(oldSize, newSize) >
		   PxMin(mCloth.mStartCollisionSpheres.capacity(), mCloth.mTargetCollisionSpheres.capacity()))
		{
			ContextLockType contextLock(mCloth.mFactory);
			mCloth.mStartCollisionSpheres.reserve(newSize);
			mCloth.mTargetCollisionSpheres.reserve(PxMax(oldSize, newSize));
		}

		typename T::MappedVec4fVectorType start = mCloth.mStartCollisionSpheres;
		typename T::MappedVec4fVectorType target = mCloth.mTargetCollisionSpheres;

		// fill target from start
		for(uint32_t i = target.size(); i < oldSize; ++i)
			target.pushBack(start[i]);

		// resize to larger of oldSize and newSize
		start.resize(PxMax(oldSize, newSize), PxVec4(0.0f));
		target.resize(PxMax(oldSize, newSize), PxVec4(0.0f));

		if(int32_t delta = int32_t(newSize - oldSize))
		{
			// move past-range elements to new place
			move(start.begin(), last, oldSize, last + delta);
			move(target.begin(), last, oldSize, last + delta);

			// fill new elements from spheres
			for(uint32_t i = last; i < last + delta; ++i)
				start[i] = spheres[i - first];

			// adjust capsule indices
			typename T::MappedIndexVectorType indices = mCloth.mCapsuleIndices;
			Vector<IndexPair>::Type::Iterator cIt, cEnd = indices.end();
			for(cIt = indices.begin(); cIt != cEnd;)
			{
				bool removed = false;
				removed |= updateIndex(cIt->first, last + PxMin(0, delta), int32_t(delta));
				removed |= updateIndex(cIt->second, last + PxMin(0, delta), int32_t(delta));
				if(!removed)
					++cIt;
				else
				{
					indices.replaceWithLast(cIt);
					cEnd = indices.end();
				}
			}

			start.resize(newSize);
			target.resize(newSize);

			mCloth.notifyChanged();
		}

		// fill target elements with spheres
		for(uint32_t i = 0; i < spheres.size(); ++i)
			target[first + i] = spheres[i];
	}

	mCloth.wakeUp();
}

template <typename T>
inline uint32_t ClothImpl<T>::getNumSpheres() const
{
	return uint32_t(mCloth.mStartCollisionSpheres.size());
}

// Fixed 4505:local function has been removed
template <typename T>
inline void ClothImpl<T>::setCapsules(Range<const uint32_t> capsules, uint32_t first, uint32_t last)
{
	const IndexPair* srcIndices = reinterpret_cast<const IndexPair*>(capsules.begin());
	const uint32_t srcIndicesSize = uint32_t(capsules.size() / 2);
	
	uint32_t oldSize = mCloth.mCapsuleIndices.size();
	uint32_t newSize = srcIndicesSize + oldSize - last + first;

	PX_ASSERT(newSize <= 32);
	PX_ASSERT(first <= oldSize);
	PX_ASSERT(last <= oldSize);

	if(mCloth.mCapsuleIndices.capacity() < newSize)
	{
		ContextLockType contextLock(mCloth.mFactory);
		mCloth.mCapsuleIndices.reserve(newSize);
	}

	// resize to larger of oldSize and newSize
	mCloth.mCapsuleIndices.resize(PxMax(oldSize, newSize));

	typename T::MappedIndexVectorType dstIndices = mCloth.mCapsuleIndices;

	if(uint32_t delta = newSize - oldSize)
	{
		// move past-range elements to new place
		move(dstIndices.begin(), last, oldSize, last + delta);

		// fill new elements from capsules
		for(uint32_t i = last; i < last + delta; ++i)
			dstIndices[i] = srcIndices[i - first];

		dstIndices.resize(newSize);
		mCloth.notifyChanged();
	}

	// fill existing elements from capsules
	for (uint32_t i=0; i < srcIndicesSize; ++i)
		dstIndices[first + i] = srcIndices[i];

	mCloth.wakeUp();
}

template <typename T>
inline uint32_t ClothImpl<T>::getNumCapsules() const
{
	return uint32_t(mCloth.mCapsuleIndices.size());
}

template <typename T>
inline void ClothImpl<T>::setPlanes(Range<const PxVec4> planes, uint32_t first, uint32_t last)
{
	uint32_t oldSize = uint32_t(mCloth.mStartCollisionPlanes.size());
	uint32_t newSize = uint32_t(planes.size()) + oldSize - last + first;

	PX_ASSERT(newSize <= 32);
	PX_ASSERT(first <= oldSize);
	PX_ASSERT(last <= oldSize);

	if(!oldSize && !newSize)
		return;

	if(!oldSize)
	{
		ContextLockType contextLock(mCloth.mFactory);
		mCloth.mStartCollisionPlanes.assign(planes.begin(), planes.end());
		mCloth.notifyChanged();
	}
	else
	{
		if(PxMax(oldSize, newSize) >
		   PxMin(mCloth.mStartCollisionPlanes.capacity(), mCloth.mTargetCollisionPlanes.capacity()))
		{
			ContextLockType contextLock(mCloth.mFactory);
			mCloth.mStartCollisionPlanes.reserve(newSize);
			mCloth.mTargetCollisionPlanes.reserve(PxMax(oldSize, newSize));
		}

		// fill target from start
		for(uint32_t i = mCloth.mTargetCollisionPlanes.size(); i < oldSize; ++i)
			mCloth.mTargetCollisionPlanes.pushBack(mCloth.mStartCollisionPlanes[i]);

		// resize to larger of oldSize and newSize
		mCloth.mStartCollisionPlanes.resize(PxMax(oldSize, newSize), PxZero);
		mCloth.mTargetCollisionPlanes.resize(PxMax(oldSize, newSize), PxZero);

		if(int32_t delta = int32_t(newSize - oldSize))
		{
			// move past-range elements to new place
			move(mCloth.mStartCollisionPlanes.begin(), last, oldSize, last + delta);
			move(mCloth.mTargetCollisionPlanes.begin(), last, oldSize, last + delta);

			// fill new elements from planes
			for(uint32_t i = last; i < last + delta; ++i)
				mCloth.mStartCollisionPlanes[i] = planes[i - first];

			// adjust convex indices
			uint32_t mask = (uint32_t(1) << (last + PxMin(delta, 0))) - 1;
			Vector<uint32_t>::Type::Iterator cIt, cEnd = mCloth.mConvexMasks.end();
			for(cIt = mCloth.mConvexMasks.begin(); cIt != cEnd;)
			{
				uint32_t convex = (*cIt & mask);
				if(delta < 0)
					convex |= *cIt >> -delta & ~mask;
				else
					convex |= (*cIt & ~mask) << delta;
				if(convex)
					*cIt++ = convex;
				else
				{
					mCloth.mConvexMasks.replaceWithLast(cIt);
					cEnd = mCloth.mConvexMasks.end();
				}
			}

			mCloth.mStartCollisionPlanes.resize(newSize);
			mCloth.mTargetCollisionPlanes.resize(newSize);

			mCloth.notifyChanged();
		}

		// fill target elements with planes
		for(uint32_t i = 0; i < planes.size(); ++i)
			mCloth.mTargetCollisionPlanes[first + i] = planes[i];
	}

	mCloth.wakeUp();
}

template <typename T>
inline uint32_t ClothImpl<T>::getNumPlanes() const
{
	return uint32_t(mCloth.mStartCollisionPlanes.size());
}

template <typename T>
inline void ClothImpl<T>::setConvexes(Range<const uint32_t> convexes, uint32_t first, uint32_t last)
{
	uint32_t oldSize = mCloth.mConvexMasks.size();
	uint32_t newSize = uint32_t(convexes.size()) + oldSize - last + first;

	PX_ASSERT(newSize <= 32);
	PX_ASSERT(first <= oldSize);
	PX_ASSERT(last <= oldSize);

	if(mCloth.mConvexMasks.capacity() < newSize)
	{
		ContextLockType contextLock(mCloth.mFactory);
		mCloth.mConvexMasks.reserve(newSize);
	}

	// resize to larger of oldSize and newSize
	mCloth.mConvexMasks.resize(PxMax(oldSize, newSize));

	if(uint32_t delta = newSize - oldSize)
	{
		// move past-range elements to new place
		move(mCloth.mConvexMasks.begin(), last, oldSize, last + delta);

		// fill new elements from capsules
		for(uint32_t i = last; i < last + delta; ++i)
			mCloth.mConvexMasks[i] = convexes[i - first];

		mCloth.mConvexMasks.resize(newSize);
		mCloth.notifyChanged();
	}

	mCloth.wakeUp();
}

template <typename T>
inline uint32_t ClothImpl<T>::getNumConvexes() const
{
	return uint32_t(mCloth.mConvexMasks.size());
}

template <typename T>
inline void ClothImpl<T>::setTriangles(Range<const PxVec3> triangles, uint32_t first, uint32_t last)
{
	// convert from triangle to vertex count
	first *= 3;
	last *= 3;

	triangles = mCloth.clampTriangleCount(triangles, last - first);
	PX_ASSERT(0 == triangles.size() % 3);

	uint32_t oldSize = uint32_t(mCloth.mStartCollisionTriangles.size());
	uint32_t newSize = uint32_t(triangles.size()) + oldSize - last + first;

	PX_ASSERT(first <= oldSize);
	PX_ASSERT(last <= oldSize);

	if(!oldSize && !newSize)
		return;

	if(!oldSize)
	{
		ContextLockType contextLock(mCloth.mFactory);
		mCloth.mStartCollisionTriangles.assign(triangles.begin(), triangles.end());
		mCloth.notifyChanged();
	}
	else
	{
		if(PxMax(oldSize, newSize) >
		   PxMin(mCloth.mStartCollisionTriangles.capacity(), mCloth.mTargetCollisionTriangles.capacity()))
		{
			ContextLockType contextLock(mCloth.mFactory);
			mCloth.mStartCollisionTriangles.reserve(newSize);
			mCloth.mTargetCollisionTriangles.reserve(PxMax(oldSize, newSize));
		}

		// fill target from start
		for(uint32_t i = mCloth.mTargetCollisionTriangles.size(); i < oldSize; ++i)
			mCloth.mTargetCollisionTriangles.pushBack(mCloth.mStartCollisionTriangles[i]);

		// resize to larger of oldSize and newSize
		mCloth.mStartCollisionTriangles.resize(PxMax(oldSize, newSize));
		mCloth.mTargetCollisionTriangles.resize(PxMax(oldSize, newSize));

		if(uint32_t delta = newSize - oldSize)
		{
			// move past-range elements to new place
			move(mCloth.mStartCollisionTriangles.begin(), last, oldSize, last + delta);
			move(mCloth.mTargetCollisionTriangles.begin(), last, oldSize, last + delta);

			// fill new elements from triangles
			for(uint32_t i = last; i < last + delta; ++i)
				mCloth.mStartCollisionTriangles[i] = triangles[i - first];

			mCloth.mStartCollisionTriangles.resize(newSize);
			mCloth.mTargetCollisionTriangles.resize(newSize);

			mCloth.notifyChanged();
		}

		// fill target elements with triangles
		for(uint32_t i = 0; i < triangles.size(); ++i)
			mCloth.mTargetCollisionTriangles[first + i] = triangles[i];
	}

	mCloth.wakeUp();
}

template <typename T>
inline void ClothImpl<T>::setTriangles(Range<const PxVec3> startTriangles, Range<const PxVec3> targetTriangles,
                                       uint32_t first)
{
	PX_ASSERT(startTriangles.size() == targetTriangles.size());

	// convert from triangle to vertex count
	first *= 3;

	uint32_t last = uint32_t(mCloth.mStartCollisionTriangles.size());

	startTriangles = mCloth.clampTriangleCount(startTriangles, last - first);
	targetTriangles = mCloth.clampTriangleCount(targetTriangles, last - first);

	uint32_t oldSize = uint32_t(mCloth.mStartCollisionTriangles.size());
	uint32_t newSize = uint32_t(startTriangles.size()) + oldSize - last + first;

	PX_ASSERT(first <= oldSize);
	PX_ASSERT(last == oldSize); // this path only supports replacing the tail

	if(!oldSize && !newSize)
		return;

	if(newSize > PxMin(mCloth.mStartCollisionTriangles.capacity(), mCloth.mTargetCollisionTriangles.capacity()))
	{
		ContextLockType contextLock(mCloth.mFactory);
		mCloth.mStartCollisionTriangles.reserve(newSize);
		mCloth.mTargetCollisionTriangles.reserve(newSize);
	}

	uint32_t retainSize = oldSize - last + first;
	mCloth.mStartCollisionTriangles.resize(retainSize);
	mCloth.mTargetCollisionTriangles.resize(retainSize);

	for(uint32_t i = 0, n = startTriangles.size(); i < n; ++i)
	{
		mCloth.mStartCollisionTriangles.pushBack(startTriangles[i]);
		mCloth.mTargetCollisionTriangles.pushBack(targetTriangles[i]);
	}

	if(newSize - oldSize)
		mCloth.notifyChanged();

	mCloth.wakeUp();
}

template <typename T>
inline uint32_t ClothImpl<T>::getNumTriangles() const
{
	return uint32_t(mCloth.mStartCollisionTriangles.size()) / 3;
}

template <typename T>
inline bool ClothImpl<T>::isContinuousCollisionEnabled() const
{
	return mCloth.mEnableContinuousCollision;
}

template <typename T>
inline void ClothImpl<T>::enableContinuousCollision(bool enable)
{
	if(enable == mCloth.mEnableContinuousCollision)
		return;

	mCloth.mEnableContinuousCollision = enable;
	mCloth.notifyChanged();
	mCloth.wakeUp();
}

template <typename T>
inline float ClothImpl<T>::getCollisionMassScale() const
{
	return mCloth.mCollisionMassScale;
}

template <typename T>
inline void ClothImpl<T>::setCollisionMassScale(float scale)
{
	if(scale == mCloth.mCollisionMassScale)
		return;

	mCloth.mCollisionMassScale = scale;
	mCloth.notifyChanged();
	mCloth.wakeUp();
}

template <typename T>
inline void ClothImpl<T>::setFriction(float friction)
{
	mCloth.mFriction = friction;
	mCloth.wakeUp();
}

template <typename T>
inline float ClothImpl<T>::getFriction() const
{
	return mCloth.mFriction;
}

template <typename T>
inline uint32_t ClothImpl<T>::getNumVirtualParticleWeights() const
{
	return uint32_t(mCloth.mVirtualParticleWeights.size());
}

template <typename T>
inline void ClothImpl<T>::setTetherConstraintScale(float scale)
{
	if(scale == mCloth.mTetherConstraintScale)
		return;

	mCloth.mTetherConstraintScale = scale;
	mCloth.notifyChanged();
	mCloth.wakeUp();
}

template <typename T>
inline float ClothImpl<T>::getTetherConstraintScale() const
{
	return mCloth.mTetherConstraintScale;
}

template <typename T>
inline void ClothImpl<T>::setTetherConstraintStiffness(float stiffness)
{
	float value = safeLog2(1 - stiffness);
	if(value == mCloth.mTetherConstraintLogStiffness)
		return;

	mCloth.mTetherConstraintLogStiffness = value;
	mCloth.notifyChanged();
	mCloth.wakeUp();
}

template <typename T>
inline float ClothImpl<T>::getTetherConstraintStiffness() const
{
	return 1 - safeExp2(mCloth.mTetherConstraintLogStiffness);
}

template <typename T>
inline Range<PxVec4> ClothImpl<T>::getMotionConstraints()
{
	mCloth.wakeUp();
	return mCloth.push(mCloth.mMotionConstraints);
}

template <typename T>
inline void ClothImpl<T>::clearMotionConstraints()
{
	mCloth.clear(mCloth.mMotionConstraints);
	mCloth.wakeUp();
}

template <typename T>
inline uint32_t ClothImpl<T>::getNumMotionConstraints() const
{
	return uint32_t(mCloth.mMotionConstraints.mStart.size());
}

template <typename T>
inline void ClothImpl<T>::setMotionConstraintScaleBias(float scale, float bias)
{
	if(scale == mCloth.mMotionConstraintScale && bias == mCloth.mMotionConstraintBias)
		return;

	mCloth.mMotionConstraintScale = scale;
	mCloth.mMotionConstraintBias = bias;
	mCloth.notifyChanged();
	mCloth.wakeUp();
}

template <typename T>
inline float ClothImpl<T>::getMotionConstraintScale() const
{
	return mCloth.mMotionConstraintScale;
}

template <typename T>
inline float ClothImpl<T>::getMotionConstraintBias() const
{
	return mCloth.mMotionConstraintBias;
}

template <typename T>
inline void ClothImpl<T>::setMotionConstraintStiffness(float stiffness)
{
	float value = safeLog2(1 - stiffness);
	if(value == mCloth.mMotionConstraintLogStiffness)
		return;

	mCloth.mMotionConstraintLogStiffness = value;
	mCloth.notifyChanged();
	mCloth.wakeUp();
}

template <typename T>
inline float ClothImpl<T>::getMotionConstraintStiffness() const
{
	return 1 - safeExp2(mCloth.mMotionConstraintLogStiffness);
}

template <typename T>
inline Range<PxVec4> ClothImpl<T>::getSeparationConstraints()
{
	mCloth.wakeUp();
	return mCloth.push(mCloth.mSeparationConstraints);
}

template <typename T>
inline void ClothImpl<T>::clearSeparationConstraints()
{
	mCloth.clear(mCloth.mSeparationConstraints);
	mCloth.wakeUp();
}

template <typename T>
inline void ClothImpl<T>::clearInterpolation()
{
	if(!mCloth.mTargetCollisionSpheres.empty())
	{
		physx::shdfnd::swap(mCloth.mStartCollisionSpheres, mCloth.mTargetCollisionSpheres);
		mCloth.mTargetCollisionSpheres.resize(0);
	}
	mCloth.mMotionConstraints.pop();
	mCloth.mSeparationConstraints.pop();
	mCloth.wakeUp();
}

template <typename T>
inline uint32_t ClothImpl<T>::getNumSeparationConstraints() const
{
	return uint32_t(mCloth.mSeparationConstraints.mStart.size());
}

template <typename T>
inline uint32_t ClothImpl<T>::getNumParticleAccelerations() const
{
	return uint32_t(mCloth.mParticleAccelerations.size());
}

template <typename T>
inline void ClothImpl<T>::setWindVelocity(PxVec3 wind)
{
	if(wind == mCloth.mWind)
		return;

	mCloth.mWind = wind;
	mCloth.notifyChanged();
	mCloth.wakeUp();
}

template <typename T>
inline PxVec3 ClothImpl<T>::getWindVelocity() const
{
	return mCloth.mWind;
}

template <typename T>
inline void ClothImpl<T>::setDragCoefficient(float coefficient)
{
	float value = safeLog2(1 - coefficient);
	if(value == mCloth.mDragLogCoefficient)
		return;

	mCloth.mDragLogCoefficient = value;
	mCloth.notifyChanged();
	mCloth.wakeUp();
}

template <typename T>
inline float ClothImpl<T>::getDragCoefficient() const
{
	return 1 - safeExp2(mCloth.mDragLogCoefficient);
}

template <typename T>
inline void ClothImpl<T>::setLiftCoefficient(float coefficient)
{
	float value = safeLog2(1 - coefficient);
	if(value == mCloth.mLiftLogCoefficient)
		return;

	mCloth.mLiftLogCoefficient = value;
	mCloth.notifyChanged();
	mCloth.wakeUp();
}

template <typename T>
inline float ClothImpl<T>::getLiftCoefficient() const
{
	return 1 - safeExp2(mCloth.mLiftLogCoefficient);
}

template <typename T>
inline uint32_t ClothImpl<T>::getNumSelfCollisionIndices() const
{
	return uint32_t(mCloth.mSelfCollisionIndices.size());
}

// Fixed 4505:local function has been removed
template <typename T>
inline void ClothImpl<T>::setRestPositions(Range<const PxVec4> restPositions)
{
	PX_ASSERT(restPositions.empty() || restPositions.size() == getNumParticles());
	ContextLockType contextLock(mCloth.mFactory);
	mCloth.mRestPositions.assign(restPositions.begin(), restPositions.end());
	mCloth.wakeUp();
}

template <typename T>
inline uint32_t ClothImpl<T>::getNumRestPositions() const
{
	return uint32_t(mCloth.mRestPositions.size());
}

template <typename T>
inline void ClothImpl<T>::setSelfCollisionDistance(float distance)
{
	if(distance == mCloth.mSelfCollisionDistance)
		return;

	mCloth.mSelfCollisionDistance = distance;
	mCloth.notifyChanged();
	mCloth.wakeUp();
}

template <typename T>
inline float ClothImpl<T>::getSelfCollisionDistance() const
{
	return mCloth.mSelfCollisionDistance;
}

template <typename T>
inline void ClothImpl<T>::setSelfCollisionStiffness(float stiffness)
{
	float value = safeLog2(1 - stiffness);
	if(value == mCloth.mSelfCollisionLogStiffness)
		return;

	mCloth.mSelfCollisionLogStiffness = value;
	mCloth.notifyChanged();
	mCloth.wakeUp();
}

template <typename T>
inline float ClothImpl<T>::getSelfCollisionStiffness() const
{
	return 1 - safeExp2(mCloth.mSelfCollisionLogStiffness);
}

template <typename T>
inline const PxVec3& ClothImpl<T>::getBoundingBoxCenter() const
{
	return mCloth.mParticleBoundsCenter;
}

template <typename T>
inline const PxVec3& ClothImpl<T>::getBoundingBoxScale() const
{
	return mCloth.mParticleBoundsHalfExtent;
}

template <typename T>
inline void ClothImpl<T>::setSleepThreshold(float threshold)
{
	if(threshold == mCloth.mSleepThreshold)
		return;

	mCloth.mSleepThreshold = threshold;
	mCloth.notifyChanged();
	mCloth.wakeUp();
}

template <typename T>
inline float ClothImpl<T>::getSleepThreshold() const
{
	return mCloth.mSleepThreshold;
}

template <typename T>
inline void ClothImpl<T>::setSleepTestInterval(uint32_t interval)
{
	if(interval == mCloth.mSleepTestInterval)
		return;

	mCloth.mSleepTestInterval = interval;
	mCloth.notifyChanged();
	mCloth.wakeUp();
}

template <typename T>
inline uint32_t ClothImpl<T>::getSleepTestInterval() const
{
	return mCloth.mSleepTestInterval;
}

template <typename T>
inline void ClothImpl<T>::setSleepAfterCount(uint32_t afterCount)
{
	if(afterCount == mCloth.mSleepAfterCount)
		return;

	mCloth.mSleepAfterCount = afterCount;
	mCloth.notifyChanged();
	mCloth.wakeUp();
}

template <typename T>
inline uint32_t ClothImpl<T>::getSleepAfterCount() const
{
	return mCloth.mSleepAfterCount;
}

template <typename T>
inline uint32_t ClothImpl<T>::getSleepPassCount() const
{
	return mCloth.mSleepPassCounter;
}

template <typename T>
inline bool ClothImpl<T>::isAsleep() const
{
	return mCloth.isSleeping();
}

template <typename T>
inline void ClothImpl<T>::putToSleep()
{
	mCloth.mSleepPassCounter = mCloth.mSleepAfterCount;
}

template <typename T>
inline void ClothImpl<T>::wakeUp()
{
	mCloth.wakeUp();
}

template <typename T>
inline void ClothImpl<T>::setUserData(void* data)
{
	mCloth.mUserData = data;
}

template <typename T>
inline void* ClothImpl<T>::getUserData() const
{
	return mCloth.mUserData;
}

template <typename T>
template <typename U>
inline MappedRange<U> ClothImpl<T>::getMappedParticles(U* data) const
{
	return MappedRange<U>(data, data + getNumParticles(), *this, &Cloth::lockParticles, &Cloth::unlockParticles);
}

} // namespace cloth

} // namespace physx
