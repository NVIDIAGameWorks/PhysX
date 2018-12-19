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

#ifndef PT_COLLISION_H
#define PT_COLLISION_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "foundation/PxTransform.h"
#include "PsBitUtils.h"
#include "PtConfig.h"
#include "PtCollisionData.h"
#include "PtCollisionMethods.h"
#include "PtParticle.h"
#include "PtTwoWayData.h"
#include "PtCollisionParameters.h"
#include "PsAlignedMalloc.h"
#include "CmTask.h"
#include "PtParticleContactManagerStream.h"

namespace physx
{

class PxsRigidBody;
class PxBaseTask;

namespace Pt
{

class ParticleShape;
class BodyTransformVault;
struct W2STransformTemp;

class Collision
{
  public:
	Collision(class ParticleSystemSimCpu& particleSystem);
	~Collision();

	void updateCollision(const PxU8* contactManagerStream, physx::PxBaseTask& continuation);

	// Update position and velocity of particles that have PxParticleFlag::eSPATIAL_DATA_STRUCTURE_OVERFLOW set.
	void updateOverflowParticles();

	PX_FORCE_INLINE CollisionParameters& getParameter()
	{
		return mParams;
	}

  private:
	typedef Ps::Array<W2STransformTemp, shdfnd::AlignedAllocator<16, Ps::ReflectionAllocator<W2STransformTemp> > >
	TempContactManagerArray;
	struct TaskData
	{
		TempContactManagerArray tempContactManagers;
		ParticleContactManagerStreamIterator packetBegin;
		ParticleContactManagerStreamIterator packetEnd;
		PxBounds3 bounds;
	};

	void processShapeListWithFilter(PxU32 taskDataIndex, const PxU32 skipNum = 0);
	void mergeResults(physx::PxBaseTask* continuation);

	void updateFluidShapeCollision(Particle* particles, TwoWayData* fluidTwoWayData, PxVec3* transientBuf,
	                               PxVec3* collisionVelocities, ConstraintBuffers& constraintBufs,
	                               ParticleOpcodeCache* opcodeCache, PxBounds3& worldBounds,
	                               const PxU32* fluidShapeParticleIndices, const PxF32* restOffsets,
	                               const W2STransformTemp* w2sTransforms, const ParticleStreamShape& streamShape);

	PX_FORCE_INLINE void updateSubPacket(Particle* particlesSp, TwoWayData* fluidTwoWayData, PxVec3* transientBuf,
	                                     PxVec3* collisionVelocities, ConstraintBuffers& constraintBufs,
	                                     ParticleOpcodeCache* perParticleCacheLocal,
	                                     ParticleOpcodeCache* perParticleCacheGlobal, LocalCellHash& localCellHash,
	                                     PxBounds3& worldBounds, const PxVec3& packetCorner,
	                                     const PxU32* particleIndicesSp, const PxU32 numParticlesSp,
	                                     const ParticleStreamContactManager* contactManagers,
	                                     const W2STransformTemp* w2sTransforms, const PxU32 numContactManagers,
	                                     const PxF32* restOffsetsSp);

	void updateFluidBodyContactPair(const Particle* particles, PxU32 numParticles, ParticleCollData* particleCollData,
	                                ConstraintBuffers& constraintBufs, ParticleOpcodeCache* perParticleCacheLocal,
	                                LocalCellHash& localCellHash, const PxVec3& packetCorner,
	                                const ParticleStreamContactManager& contactManager,
	                                const W2STransformTemp& w2sTransform);

	void PX_FORCE_INLINE addTempW2STransform(TaskData& taskData, const ParticleStreamContactManager& cm);

  private:
	Collision& operator=(const Collision&);
	CollisionParameters mParams;
	ParticleSystemSimCpu& mParticleSystem;
	TaskData mTaskData[PT_NUM_PACKETS_PARALLEL_COLLISION];

	typedef Cm::DelegateTask<Collision, &Collision::mergeResults> MergeTask;
	MergeTask mMergeTask;
	friend class CollisionTask;
};

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_COLLISION_H
