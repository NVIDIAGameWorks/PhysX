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

#ifndef PT_PARTICLE_SYSTEM_SIM_CPU_H
#define PT_PARTICLE_SYSTEM_SIM_CPU_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "PtParticleSystemSim.h"
#include "PtDynamics.h"
#include "PtCollision.h"
#include "PtGridCellVector.h"
#include "PsAllocator.h"
#include "PtParticleData.h"
#include "CmTask.h"
#include "PtContextCpu.h"

namespace physx
{

class PxParticleDeviceExclusiveAccess;
class PxBaseTask;

namespace Pt
{
class Context;
struct ConstraintPair;
class SpatialHash;
class ParticleShapeCpu;

class ParticleSystemSimCpu : public ParticleSystemSim
{
	PX_NOCOPY(ParticleSystemSimCpu)
  public:
	//---------------------------
	// Implements ParticleSystemSim
	virtual ParticleSystemState& getParticleStateV();
	virtual void getSimParticleDataV(ParticleSystemSimDataDesc& simParticleData, bool devicePtr) const;

	virtual void getShapesUpdateV(ParticleShapeUpdateResults& updateResults) const;

	virtual void setExternalAccelerationV(const PxVec3& v);
	virtual const PxVec3& getExternalAccelerationV() const;

	virtual void setSimulationTimeStepV(PxReal value);
	virtual PxReal getSimulationTimeStepV() const;

	virtual void setSimulatedV(bool);
	virtual Ps::IntBool isSimulatedV() const;

	virtual void addInteractionV(const ParticleShape&, ShapeHandle, BodyHandle, bool, bool)
	{
	}
	virtual void removeInteractionV(const ParticleShape& particleShape, ShapeHandle shape, BodyHandle body,
	                                bool isDynamic, bool isDyingRb, bool ccdBroadphase);
	virtual void onRbShapeChangeV(const ParticleShape& particleShape, ShapeHandle shape);

	virtual void flushBufferedInteractionUpdatesV()
	{
	}

	virtual void passCollisionInputV(ParticleCollisionUpdateInput input);
#if PX_SUPPORT_GPU_PHYSX
	virtual Ps::IntBool isGpuV() const
	{
		return false;
	}
	virtual void enableDeviceExclusiveModeGpuV()
	{
		PX_ASSERT(0);
	}
	virtual PxParticleDeviceExclusiveAccess* getDeviceExclusiveAccessGpuV() const
	{
		PX_ASSERT(0);
		return NULL;
	}
#endif

	//~Implements ParticleSystemSim
	//---------------------------

	ParticleSystemSimCpu(ContextCpu* context, PxU32 index);
	virtual ~ParticleSystemSimCpu();
	void init(ParticleData& particleData, const ParticleSystemParameter& parameter);
	void clear();
	ParticleData* obtainParticleState();

	PX_FORCE_INLINE ContextCpu& getContext() const
	{
		return mContext;
	}

	PX_FORCE_INLINE void getPacketBounds(const GridCellVector& coord, PxBounds3& bounds);

	PX_FORCE_INLINE PxReal computeViscosityMultiplier(PxReal viscosityStd, PxReal particleMassStd, PxReal radius6Std);

	PX_FORCE_INLINE PxU32 getIndex() const
	{
		return mIndex;
	}

	void packetShapesUpdate(physx::PxBaseTask* continuation);
	void packetShapesFinalization(physx::PxBaseTask* continuation);
	void dynamicsUpdate(physx::PxBaseTask* continuation);
	void collisionUpdate(physx::PxBaseTask* continuation);
	void collisionFinalization(physx::PxBaseTask* continuation);
	void spatialHashUpdateSections(physx::PxBaseTask* continuation);

	physx::PxBaseTask& schedulePacketShapesUpdate(const ParticleShapesUpdateInput& input,
	                                              physx::PxBaseTask& continuation);
	physx::PxBaseTask& scheduleDynamicsUpdate(physx::PxBaseTask& continuation);
	physx::PxBaseTask& scheduleCollisionUpdate(physx::PxBaseTask& continuation);

  private:
	void remapShapesToPackets(ParticleShape* const* shapes, PxU32 numShapes);
	void clearParticleConstraints();
	void initializeParameter();
	void updateDynamicsParameter();
	void updateCollisionParameter();
	void removeTwoWayRbReferences(const ParticleShapeCpu& particleShape, const PxsBodyCore* rigidBody);
	void setCollisionCacheInvalid(const ParticleShapeCpu& particleShape, const Gu::GeometryUnion& geometry);

  private:
	ContextCpu& mContext;
	ParticleData* mParticleState;
	const ParticleSystemParameter* mParameter;

	Ps::IntBool mSimulated;

	TwoWayData* mFluidTwoWayData;

	ParticleShape** mCreatedDeletedParticleShapes; // Handles of created and deleted particle packet shapes.
	PxU32 mNumCreatedParticleShapes;
	PxU32 mNumDeletedParticleShapes;
	PxU32* mPacketParticlesIndices; // Dense array of sorted particle indices.
	PxU32 mNumPacketParticlesIndices;

	ConstraintBuffers mConstraintBuffers; // Particle constraints.

	ParticleOpcodeCache* mOpcodeCacheBuffer; // Opcode cache.
	PxVec3* mTransientBuffer;                // force in SPH , collision normal
	PxVec3* mCollisionVelocities;

	// Spatial ordering, packet generation
	SpatialHash* mSpatialHash;

	// Dynamics update
	Dynamics mDynamics;

	// Collision update
	Collision mCollision;

	PxReal mSimulationTimeStep;
	bool mIsSimulated;

	PxVec3 mExternalAcceleration; // This includes the gravity of the scene

	PxU32 mIndex;

	// pipeline tasks
	typedef Cm::DelegateTask<ParticleSystemSimCpu, &ParticleSystemSimCpu::packetShapesUpdate> PacketShapesUpdateTask;
	typedef Cm::DelegateTask<ParticleSystemSimCpu, &ParticleSystemSimCpu::packetShapesFinalization> PacketShapesFinalizationTask;
	typedef Cm::DelegateTask<ParticleSystemSimCpu, &ParticleSystemSimCpu::dynamicsUpdate> DynamicsUpdateTask;
	typedef Cm::DelegateTask<ParticleSystemSimCpu, &ParticleSystemSimCpu::collisionUpdate> CollisionUpdateTask;
	typedef Cm::DelegateTask<ParticleSystemSimCpu, &ParticleSystemSimCpu::collisionFinalization> CollisionFinalizationTask;
	typedef Cm::DelegateTask<ParticleSystemSimCpu, &ParticleSystemSimCpu::spatialHashUpdateSections> SpatialHashUpdateSectionsTask;

	PacketShapesUpdateTask mPacketShapesUpdateTask;
	PacketShapesFinalizationTask mPacketShapesFinalizationTask;
	DynamicsUpdateTask mDynamicsUpdateTask;
	CollisionUpdateTask mCollisionUpdateTask;
	CollisionFinalizationTask mCollisionFinalizationTask;
	SpatialHashUpdateSectionsTask mSpatialHashUpdateSectionsTask;

	ParticleShapesUpdateInput mPacketShapesUpdateTaskInput;
	ParticleCollisionUpdateInput mCollisionUpdateTaskInput;

	Ps::AlignedAllocator<16, Ps::ReflectionAllocator<char> > mAlign16;

	friend class Collision;
	friend class Dynamics;
};

//----------------------------------------------------------------------------//

/*!
Compute AABB of a packet given its coordinates.
Enlarge the bounding box such that a particle on the current boundary could
travel the maximum distance and would still be inside the enlarged volume.
*/
PX_FORCE_INLINE void ParticleSystemSimCpu::getPacketBounds(const GridCellVector& coord, PxBounds3& bounds)
{
	PxVec3 gridOrigin(static_cast<PxReal>(coord.x), static_cast<PxReal>(coord.y), static_cast<PxReal>(coord.z));
	gridOrigin *= mCollision.getParameter().packetSize;

	PxVec3 collisionRangeVec(mCollision.getParameter().collisionRange);
	bounds.minimum = gridOrigin - collisionRangeVec;
	bounds.maximum = gridOrigin + PxVec3(mCollision.getParameter().packetSize) + collisionRangeVec;
}

PX_FORCE_INLINE PxReal
ParticleSystemSimCpu::computeViscosityMultiplier(PxReal viscosityStd, PxReal particleMassStd, PxReal radius6Std)
{
	PxReal wViscosityLaplacianScalarStd = 45.0f / (PxPi * radius6Std);
	return (wViscosityLaplacianScalarStd * viscosityStd * particleMassStd);
}

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_PARTICLE_SYSTEM_SIM_CPU_H
