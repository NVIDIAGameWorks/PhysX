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

#include "Range.h"
#include "PhaseConfig.h"

struct ID3D11Buffer;

namespace physx
{
namespace cloth
{

class Factory;
class Fabric;
class Cloth;

template <typename T>
struct MappedRange : public Range<T>
{
	MappedRange(T* first, T* last, const Cloth& cloth, void (Cloth::*lock)() const, void (Cloth::*unlock)() const)
	: Range<T>(first, last), mCloth(cloth), mLock(lock), mUnlock(unlock)
	{
	}

	MappedRange(const MappedRange& other)
	: Range<T>(other), mCloth(other.mCloth), mLock(other.mLock), mUnlock(other.mUnlock)
	{
		(mCloth.*mLock)();
	}

	~MappedRange()
	{
		(mCloth.*mUnlock)();
	}

  private:
	MappedRange& operator=(const MappedRange&);

	const Cloth& mCloth;
	void (Cloth::*mLock)() const;
	void (Cloth::*mUnlock)() const;
};

struct GpuParticles
{
	PxVec4* mCurrent;
	PxVec4* mPrevious;
	ID3D11Buffer* mBuffer;
};

// abstract cloth instance
class Cloth
{
	Cloth& operator=(const Cloth&);

  protected:
	Cloth()
	{
	}
	Cloth(const Cloth&)
	{
	}

  public:
	virtual ~Cloth()
	{
	}

	// same as factory.clone(*this)
	virtual Cloth* clone(Factory& factory) const = 0;

	virtual Fabric& getFabric() const = 0;
	virtual Factory& getFactory() const = 0;

	/* particle properties */

	virtual uint32_t getNumParticles() const = 0;
	virtual void lockParticles() const = 0;
	virtual void unlockParticles() const = 0;
	// return particle data for current and previous frame
	// setting current invMass to zero locks particle.
	virtual MappedRange<PxVec4> getCurrentParticles() = 0;
	virtual MappedRange<const PxVec4> getCurrentParticles() const = 0;
	virtual MappedRange<PxVec4> getPreviousParticles() = 0;
	virtual MappedRange<const PxVec4> getPreviousParticles() const = 0;
	virtual GpuParticles getGpuParticles() = 0;

	// set position of cloth after next call to simulate()
	virtual void setTranslation(const PxVec3& trans) = 0;
	virtual void setRotation(const PxQuat& rot) = 0;

	// get current position of cloth
	virtual const PxVec3& getTranslation() const = 0;
	virtual const PxQuat& getRotation() const = 0;

	// zero inertia derived from method calls above (once)
	virtual void clearInertia() = 0;

	// adjust the position of the cloth without affecting the dynamics (to call after a world origin shift, for example)
	virtual void teleport(const PxVec3& delta) = 0;

	/* solver parameters */

	// return delta time used for previous iteration
	virtual float getPreviousIterationDt() const = 0;

	// gravity in global coordinates
	virtual void setGravity(const PxVec3&) = 0;
	virtual PxVec3 getGravity() const = 0;

	// damping of local particle velocity (1/stiffnessFrequency)
	// 0 (default): velocity is unaffected, 1: velocity is zero'ed
	virtual void setDamping(const PxVec3&) = 0;
	virtual PxVec3 getDamping() const = 0;

	// portion of local frame velocity applied to particles
	// 0 (default): particles are unaffected
	// same as damping: damp global particle velocity
	virtual void setLinearDrag(const PxVec3&) = 0;
	virtual PxVec3 getLinearDrag() const = 0;
	virtual void setAngularDrag(const PxVec3&) = 0;
	virtual PxVec3 getAngularDrag() const = 0;

	// portion of local frame accelerations applied to particles
	// 0: particles are unaffected, 1 (default): physically correct
	virtual void setLinearInertia(const PxVec3&) = 0;
	virtual PxVec3 getLinearInertia() const = 0;
	virtual void setAngularInertia(const PxVec3&) = 0;
	virtual PxVec3 getAngularInertia() const = 0;
	virtual void setCentrifugalInertia(const PxVec3&) = 0;
	virtual PxVec3 getCentrifugalInertia() const = 0;

	// target solver iterations per second
	virtual void setSolverFrequency(float) = 0;
	virtual float getSolverFrequency() const = 0;

	// damp, drag, stiffness exponent per second
	virtual void setStiffnessFrequency(float) = 0;
	virtual float getStiffnessFrequency() const = 0;

	// filter width for averaging dt^2 factor of gravity and
	// external acceleration, in numbers of iterations (default=30).
	virtual void setAcceleationFilterWidth(uint32_t) = 0;
	virtual uint32_t getAccelerationFilterWidth() const = 0;

	// setup edge constraint solver iteration
	virtual void setPhaseConfig(Range<const PhaseConfig> configs) = 0;

	/* collision parameters */

	virtual void setSpheres(Range<const PxVec4>, uint32_t first, uint32_t last) = 0;
	virtual uint32_t getNumSpheres() const = 0;

	virtual void setCapsules(Range<const uint32_t>, uint32_t first, uint32_t last) = 0;
	virtual uint32_t getNumCapsules() const = 0;

	virtual void setPlanes(Range<const PxVec4>, uint32_t first, uint32_t last) = 0;
	virtual uint32_t getNumPlanes() const = 0;

	virtual void setConvexes(Range<const uint32_t>, uint32_t first, uint32_t last) = 0;
	virtual uint32_t getNumConvexes() const = 0;

	virtual void setTriangles(Range<const PxVec3>, uint32_t first, uint32_t last) = 0;
	virtual void setTriangles(Range<const PxVec3>, Range<const PxVec3>, uint32_t first) = 0;
	virtual uint32_t getNumTriangles() const = 0;

	// check if we use ccd or not
	virtual bool isContinuousCollisionEnabled() const = 0;
	// set if we use ccd or not (disabled by default)
	virtual void enableContinuousCollision(bool) = 0;

	// controls how quickly mass is increased during collisions
	virtual float getCollisionMassScale() const = 0;
	virtual void setCollisionMassScale(float) = 0;

	// friction
	virtual void setFriction(float) = 0;
	virtual float getFriction() const = 0;

	// set virtual particles for collision handling.
	// each indices element consists of 3 particle
	// indices and an index into the lerp weights array.
	virtual void setVirtualParticles(Range<const uint32_t[4]> indices, Range<const PxVec3> weights) = 0;
	virtual uint32_t getNumVirtualParticles() const = 0;
	virtual uint32_t getNumVirtualParticleWeights() const = 0;

	/* tether constraint parameters */

	virtual void setTetherConstraintScale(float scale) = 0;
	virtual float getTetherConstraintScale() const = 0;
	virtual void setTetherConstraintStiffness(float stiffness) = 0;
	virtual float getTetherConstraintStiffness() const = 0;

	/* motion constraint parameters */

	// return reference to motion constraints (position, radius)
	// The entire range must be written after calling this function.
	virtual Range<PxVec4> getMotionConstraints() = 0;
	virtual void clearMotionConstraints() = 0;
	virtual uint32_t getNumMotionConstraints() const = 0;
	virtual void setMotionConstraintScaleBias(float scale, float bias) = 0;
	virtual float getMotionConstraintScale() const = 0;
	virtual float getMotionConstraintBias() const = 0;
	virtual void setMotionConstraintStiffness(float stiffness) = 0;
	virtual float getMotionConstraintStiffness() const = 0;

	/* separation constraint parameters */

	// return reference to separation constraints (position, radius)
	// The entire range must be written after calling this function.
	virtual Range<PxVec4> getSeparationConstraints() = 0;
	virtual void clearSeparationConstraints() = 0;
	virtual uint32_t getNumSeparationConstraints() const = 0;

	/* clear interpolation */

	// assign current to previous positions for
	// collision spheres, motion, and separation constraints
	virtual void clearInterpolation() = 0;

	/* particle acceleration parameters */

	// return reference to particle accelerations (in local coordinates)
	// The entire range must be written after calling this function.
	virtual Range<PxVec4> getParticleAccelerations() = 0;
	virtual void clearParticleAccelerations() = 0;
	virtual uint32_t getNumParticleAccelerations() const = 0;

	/* wind */

	// Set wind in global coordinates. Acts on the fabric's triangles
	virtual void setWindVelocity(PxVec3) = 0;
	virtual PxVec3 getWindVelocity() const = 0;
	virtual void setDragCoefficient(float) = 0;
	virtual float getDragCoefficient() const = 0;
	virtual void setLiftCoefficient(float) = 0;
	virtual float getLiftCoefficient() const = 0;

	/* self collision */

	virtual void setSelfCollisionDistance(float distance) = 0;
	virtual float getSelfCollisionDistance() const = 0;
	virtual void setSelfCollisionStiffness(float stiffness) = 0;
	virtual float getSelfCollisionStiffness() const = 0;

	virtual void setSelfCollisionIndices(Range<const uint32_t>) = 0;
	virtual uint32_t getNumSelfCollisionIndices() const = 0;

	/* rest positions */

	// set rest particle positions used during self-collision
	virtual void setRestPositions(Range<const PxVec4>) = 0;
	virtual uint32_t getNumRestPositions() const = 0;

	/* bounding box */

	// current particle position bounds in local space
	virtual const PxVec3& getBoundingBoxCenter() const = 0;
	virtual const PxVec3& getBoundingBoxScale() const = 0;

	/* sleeping (disabled by default) */

	// max particle velocity (per axis) to pass sleep test
	virtual void setSleepThreshold(float) = 0;
	virtual float getSleepThreshold() const = 0;
	// test sleep condition every nth millisecond
	virtual void setSleepTestInterval(uint32_t) = 0;
	virtual uint32_t getSleepTestInterval() const = 0;
	// put cloth to sleep when n consecutive sleep tests pass
	virtual void setSleepAfterCount(uint32_t) = 0;
	virtual uint32_t getSleepAfterCount() const = 0;
	virtual uint32_t getSleepPassCount() const = 0;
	virtual bool isAsleep() const = 0;
	virtual void putToSleep() = 0;
	virtual void wakeUp() = 0;

	virtual void setUserData(void*) = 0;
	virtual void* getUserData() const = 0;
};

// wrappers to prevent non-const overload from marking particles dirty
inline MappedRange<const PxVec4> readCurrentParticles(const Cloth& cloth)
{
	return cloth.getCurrentParticles();
}
inline MappedRange<const PxVec4> readPreviousParticles(const Cloth& cloth)
{
	return cloth.getPreviousParticles();
}

} // namespace cloth
} // namespace physx
