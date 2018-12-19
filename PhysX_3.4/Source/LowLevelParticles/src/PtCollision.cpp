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

#include "PtCollision.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "PtConfig.h"
#include "PtParticleSystemSimCpu.h"
#include "PtParticleShapeCpu.h"
#include "PtContext.h"
#include "PtBodyTransformVault.h"
#include "PtCollisionHelper.h"
#include "PtParticleContactManagerStream.h"
#include "GuConvexMeshData.h"
#include "CmFlushPool.h"
#include "PxvGeometry.h"

using namespace physx;
using namespace Pt;

namespace physx
{

namespace Pt
{

class CollisionTask : public Cm::Task
{
  public:
	CollisionTask(Collision& context, PxU32 taskDataIndex) : Cm::Task(0), mCollisionContext(context), mTaskDataIndex(taskDataIndex)
	{
	}

	virtual void runInternal()
	{
		mCollisionContext.processShapeListWithFilter(mTaskDataIndex);
	}

	virtual const char* getName() const
	{
		return "Collision.fluidCollision";
	}

  private:
	CollisionTask& operator=(const CollisionTask&);
	Collision& mCollisionContext;
	PxU32 mTaskDataIndex;
};

} // namespace Pt
} // namespace physx

/*
how to support dominance-driven one/two-way collision (search for 'todo dominance'):
- add 2-bit flag to PxsBodyShapeRef which stores the dominance matrix values
- store this flag when creating the shape ref in updateFluidBodyContactPair()
- use this flag when copying impulse to collData.shapeImpulse
*/
Collision::Collision(ParticleSystemSimCpu& particleSystem)
: mParticleSystem(particleSystem), mMergeTask(0, this, "Collision.mergeResults")
{
}

Collision::~Collision()
{
}

void PX_FORCE_INLINE Collision::addTempW2STransform(TaskData& taskData, const ParticleStreamContactManager& cm)
{
	W2STransformTemp& cmTemp = taskData.tempContactManagers.insert();

	if(cm.isDynamic)
	{
		const PxsBodyCore* bodyCore = static_cast<const PxsBodyCore*>(cm.rigidCore);
		cmTemp.w2sOld = cm.shapeCore->transform.transformInv(bodyCore->getBody2Actor()).transform(cm.w2sOld->getInverse());
		cmTemp.w2sNew =
		    cm.shapeCore->transform.transformInv(bodyCore->getBody2Actor()).transform(bodyCore->body2World.getInverse());
	}
	else
	{
		const PxTransform tmp = cm.shapeCore->transform.getInverse() * cm.rigidCore->body2World.getInverse();
		cmTemp.w2sOld = tmp;
		cmTemp.w2sNew = tmp;
	}
}

void Collision::updateCollision(const PxU8* contactManagerStream, physx::PxBaseTask& continuation)
{
	mMergeTask.setContinuation(&continuation);
	PxU32 maxTasks = PT_NUM_PACKETS_PARALLEL_COLLISION;
	PxU32 packetParticleIndicesCount = mParticleSystem.mNumPacketParticlesIndices;

	// would be nice to get available thread count to decide on task decomposition
	// mParticleSystem.getContext().getTaskManager().getCpuDispatcher();

	// use number of particles for task decomposition

	PxU32 targetParticleCountPerTask =
	    PxMax(PxU32(packetParticleIndicesCount / maxTasks), PxU32(PT_SUBPACKET_PARTICLE_LIMIT_COLLISION));
	ParticleContactManagerStreamReader cmStreamReader(contactManagerStream);
	ParticleContactManagerStreamIterator cmStreamEnd = cmStreamReader.getEnd();
	ParticleContactManagerStreamIterator cmStream = cmStreamReader.getBegin();
	ParticleContactManagerStreamIterator cmStreamLast;

	PxU32 numTasks = 0;
	for(PxU32 i = 0; i < PT_NUM_PACKETS_PARALLEL_COLLISION; ++i)
	{
		TaskData& taskData = mTaskData[i];
		taskData.bounds.setEmpty();

		// if this is the last interation, we need to gather all remaining packets
		if(i == maxTasks - 1)
			targetParticleCountPerTask = 0xffffffff;

		cmStreamLast = cmStream;
		PxU32 currentParticleCount = 0;

		while(currentParticleCount < targetParticleCountPerTask && cmStream != cmStreamEnd)
		{
			ParticleStreamShape streamShape;
			cmStream.getNext(streamShape);
			const ParticleShapeCpu* particleShape = static_cast<const ParticleShapeCpu*>(streamShape.particleShape);
			currentParticleCount += particleShape->getFluidPacket()->numParticles;
		}

		if(currentParticleCount > 0)
		{
			PX_ASSERT(cmStreamLast != cmStream);
			taskData.packetBegin = cmStreamLast;
			taskData.packetEnd = cmStream;
			numTasks++;
		}
	}
	PX_ASSERT(cmStream == cmStreamEnd);

	// spawn tasks
	for(PxU32 i = 0; i < numTasks; ++i)
	{
		void* ptr = mParticleSystem.getContext().getTaskPool().allocate(sizeof(CollisionTask));
		CollisionTask* task = PX_PLACEMENT_NEW(ptr, CollisionTask)(*this, i);
		task->setContinuation(&mMergeTask);
		task->removeReference();
	}

	mMergeTask.removeReference();
}

void Collision::updateOverflowParticles()
{
	// if no particles are present, the hash shouldn't be accessed, as it hasn't been updated.
	if(mParticleSystem.mParticleState->getValidParticleRange() > 0)
	{
		const Pt::ParticleCell& overflowCell =
		    mParticleSystem.mSpatialHash->getPackets()[PT_PARTICLE_SYSTEM_OVERFLOW_INDEX];
		Pt::Particle* particles = mParticleSystem.mParticleState->getParticleBuffer();
		PxU32* indices = mParticleSystem.mPacketParticlesIndices;
		for(PxU32 i = overflowCell.firstParticle; i < overflowCell.firstParticle + overflowCell.numParticles; i++)
		{
			PxU32 index = indices[i];
			Pt::Particle& particle = particles[index];
			PX_ASSERT((particle.flags.api & PxParticleFlag::eSPATIAL_DATA_STRUCTURE_OVERFLOW) != 0);

			// update velocity and position
			// world bounds are not updated for overflow particles, to make it more consistent with GPU.
			{
				PxVec3 acceleration = mParams.externalAcceleration;
				integrateParticleVelocity(particle, mParams.maxMotionDistance, acceleration, mParams.dampingDtComp,
				                          mParams.timeStep);
				particle.position = particle.position + particle.velocity * mParams.timeStep;

				// adapted from updateParticle(...) in PxsFluidCollisionHelper.h
				bool projection = (mParams.flags & PxParticleBaseFlag::ePROJECT_TO_PLANE) != 0;
				if(projection)
				{
					const PxReal dist = mParams.projectionPlane.n.dot(particle.velocity);
					particle.velocity = particle.velocity - (mParams.projectionPlane.n * dist);
					particle.position = mParams.projectionPlane.project(particle.position);
				}
				PX_ASSERT(particle.position.isFinite());
			}
		}
	}
}

void Collision::processShapeListWithFilter(PxU32 taskDataIndex, const PxU32 skipNum)
{
	TaskData& taskData = mTaskData[taskDataIndex];

	ParticleContactManagerStreamIterator it = taskData.packetBegin;
	while(it != taskData.packetEnd)
	{
		ParticleStreamShape streamShape;
		it.getNext(streamShape);

		if(streamShape.numContactManagers < skipNum)
			continue;

		const ParticleShapeCpu* particleShape = static_cast<const ParticleShapeCpu*>(streamShape.particleShape);
		PX_ASSERT(particleShape);
		PX_UNUSED(particleShape);

		// Collect world to shape space transforms for all colliding rigid body shapes
		taskData.tempContactManagers.clear();
		for(PxU32 i = 0; i < streamShape.numContactManagers; i++)
		{
			const ParticleStreamContactManager& cm = streamShape.contactManagers[i];
			addTempW2STransform(taskData, cm);
		}

		updateFluidShapeCollision(
		    mParticleSystem.mParticleState->getParticleBuffer(), mParticleSystem.mFluidTwoWayData,
		    mParticleSystem.mTransientBuffer, mParticleSystem.mCollisionVelocities, mParticleSystem.mConstraintBuffers,
		    mParticleSystem.mOpcodeCacheBuffer, taskData.bounds, mParticleSystem.mPacketParticlesIndices,
		    mParticleSystem.mParticleState->getRestOffsetBuffer(), taskData.tempContactManagers.begin(), streamShape);
	}
}

void Collision::mergeResults(physx::PxBaseTask* /*continuation*/)
{
	PxBounds3& worldBounds = mParticleSystem.mParticleState->getWorldBounds();
	for(PxU32 i = 0; i < PT_NUM_PACKETS_PARALLEL_COLLISION; ++i)
		worldBounds.include(mTaskData[i].bounds);
}

void Collision::updateFluidShapeCollision(Particle* particles, TwoWayData* fluidTwoWayData, PxVec3* transientBuf,
                                          PxVec3* collisionVelocities, ConstraintBuffers& constraintBufs,
                                          ParticleOpcodeCache* opcodeCache, PxBounds3& worldBounds,
                                          const PxU32* fluidShapeParticleIndices, const PxF32* restOffsets,
                                          const W2STransformTemp* w2sTransforms, const ParticleStreamShape& streamShape)
{
	const ParticleShapeCpu& particleShape = *static_cast<const ParticleShapeCpu*>(streamShape.particleShape);
	PX_ASSERT(particleShape.getFluidPacket());

	const ParticleCell& packet = *particleShape.getFluidPacket();

	PxU32 numParticles = packet.numParticles;
	PxU32 firstParticleIndex = packet.firstParticle;
	const PxU32* packetParticleIndices = fluidShapeParticleIndices + firstParticleIndex;
	const PxU32 numParticlesPerSubpacket = PT_SUBPACKET_PARTICLE_LIMIT_COLLISION;

	PX_ALLOCA(particlesSp, Particle, numParticlesPerSubpacket);
	PxF32 restOffsetsSp[numParticlesPerSubpacket];

	const PxU32 numHashBuckets = PT_LOCAL_HASH_SIZE_MESH_COLLISION;

	PxU32 hashMemCount = numHashBuckets * sizeof(ParticleCell) + numParticlesPerSubpacket * sizeof(PxU32);
	PxU32 cacheMemCount = numParticlesPerSubpacket * sizeof(ParticleOpcodeCache);
	PX_ALLOCA(shareMem, PxU8, PxMax(hashMemCount, cacheMemCount));

	ParticleOpcodeCache* perParticleCacheSp = NULL;
	LocalCellHash localCellHash;
	PxVec3 packetCorner;

	if(opcodeCache)
		perParticleCacheSp = reinterpret_cast<ParticleOpcodeCache*>(shareMem.mPointer);
	else
	{
		// Make sure the number of hash buckets is a power of 2 (requirement for the used hash function)
		PX_ASSERT((((numHashBuckets - 1) ^ numHashBuckets) + 1) == (2 * numHashBuckets));
		PX_ASSERT(numHashBuckets > numParticlesPerSubpacket);
		// Set the buffers for the local cell hash
		localCellHash.particleIndices = reinterpret_cast<PxU32*>(shareMem.mPointer);
		localCellHash.hashEntries =
		    reinterpret_cast<ParticleCell*>(shareMem.mPointer + numParticlesPerSubpacket * sizeof(PxU32));
		packetCorner =
		    PxVec3(PxReal(packet.coords.x), PxReal(packet.coords.y), PxReal(packet.coords.z)) * mParams.packetSize;
	}

	// Divide the packet into subpackets that fit into local memory of processing unit.
	PxU32 particlesRemainder = (numParticles - 1) % numParticlesPerSubpacket + 1;

	PxU32 numProcessedParticles = 0;
	PxU32 numParticlesSp = particlesRemainder; // We start with the smallest subpacket, i.e., the subpacket which does
	// not reach its particle limit.
	while(numProcessedParticles < numParticles)
	{
		const PxU32* particleIndicesSp = packetParticleIndices + numProcessedParticles;

		// load particles (constraints are loaded on demand so far)
		for(PxU32 p = 0; p < numParticlesSp; p++)
		{
			PxU32 particleIndex = particleIndicesSp[p];
			particlesSp[p] = particles[particleIndex];
		}

		if(restOffsets)
		{
			for(PxU32 p = 0; p < numParticlesSp; p++)
			{
				PxU32 particleIndex = particleIndicesSp[p];
				restOffsetsSp[p] = restOffsets[particleIndex];
			}
		}
		else
		{
			for(PxU32 p = 0; p < numParticlesSp; p++)
				restOffsetsSp[p] = mParams.restOffset;
		}

		updateSubPacket(particlesSp, fluidTwoWayData, transientBuf, collisionVelocities, constraintBufs,
		                perParticleCacheSp, opcodeCache, localCellHash, worldBounds, packetCorner, particleIndicesSp,
		                numParticlesSp, streamShape.contactManagers, w2sTransforms, streamShape.numContactManagers,
		                restOffsetsSp);

		// store particles back
		for(PxU32 p = 0; p < numParticlesSp; p++)
		{
			PxU32 particleIndex = particleIndicesSp[p];
			particles[particleIndex] = particlesSp[p];
		}

		// Invalidate cached local cell hash
		localCellHash.isHashValid = false;

		numProcessedParticles += numParticlesSp;
		numParticlesSp = numParticlesPerSubpacket;
	}
}

PX_FORCE_INLINE void
Collision::updateSubPacket(Particle* particlesSp, TwoWayData* fluidTwoWayData, PxVec3* transientBuf,
                           PxVec3* collisionVelocities, ConstraintBuffers& constraintBufs,
                           ParticleOpcodeCache* perParticleCacheLocal, ParticleOpcodeCache* perParticleCacheGlobal,
                           LocalCellHash& localCellHash, PxBounds3& worldBounds, const PxVec3& packetCorner,
                           const PxU32* particleIndicesSp, const PxU32 numParticlesSp,
                           const ParticleStreamContactManager* contactManagers, const W2STransformTemp* w2sTransforms,
                           const PxU32 numContactManagers, const PxF32* restOffsetsSp)
{
	ParticleCollData* collDataSp =
	    reinterpret_cast<ParticleCollData*>(PX_ALLOC(numParticlesSp * sizeof(ParticleCollData), "ParticleCollData"));
	for(PxU32 p = 0; p < numParticlesSp; p++)
	{
		const PxU32 particleIndex = particleIndicesSp[p];
		Particle& particle = particlesSp[p];
		PX_ASSERT(particle.position.isFinite() && particle.velocity.isFinite());
		ParticleCollData& collData = collDataSp[p];
		Ps::prefetchLine(&collData);
		collData.c0 = &constraintBufs.constraint0Buf[particleIndex];
		collData.c1 = &constraintBufs.constraint1Buf[particleIndex];
		Ps::prefetchLine(collData.c0);
		Ps::prefetchLine(collData.c1);
		const PxVec3 particleOldVel = particle.velocity;

		// integrate velocity
		{
			PxVec3 acceleration = mParams.externalAcceleration;
			if(mParams.flags & InternalParticleSystemFlag::eSPH)
				acceleration += transientBuf[particleIndex];

			integrateParticleVelocity(particle, mParams.maxMotionDistance, acceleration, mParams.dampingDtComp,
			                          mParams.timeStep);
		}

		PxVec3 c0Velocity(0.0f);
		PxVec3 c1Velocity(0.0f);
		const PxsBodyCore* c0TwoWayBody = NULL;
		const PxsBodyCore* c1TwoWayBody = NULL;
		if(particle.flags.low & InternalParticleFlag::eCONSTRAINT_0_DYNAMIC)
		{
			c0Velocity = constraintBufs.constraint0DynamicBuf[particleIndex].velocity;
			if(fluidTwoWayData)
				c0TwoWayBody = constraintBufs.constraint0DynamicBuf[particleIndex].twoWayBody;
		}

		if(particle.flags.low & InternalParticleFlag::eCONSTRAINT_1_DYNAMIC)
		{
			c1Velocity = constraintBufs.constraint1DynamicBuf[particleIndex].velocity;
			if(fluidTwoWayData)
				c1TwoWayBody = constraintBufs.constraint1DynamicBuf[particleIndex].twoWayBody;
		}

		initCollDataAndApplyConstraints(collData, particle, particleOldVel, restOffsetsSp[p], c0Velocity, c1Velocity,
		                                c0TwoWayBody, c1TwoWayBody, particleIndex, mParams);

		collData.particleFlags.low &=
		    PxU16(~(InternalParticleFlag::eCONSTRAINT_0_VALID | InternalParticleFlag::eCONSTRAINT_1_VALID |
		            InternalParticleFlag::eCONSTRAINT_0_DYNAMIC | InternalParticleFlag::eCONSTRAINT_1_DYNAMIC));
	}

	//
	// Collide with dynamic shapes

	PxU32 numDynamicShapes = 0;
	for(PxU32 i = 0; i < numContactManagers; i++)
	{
		const ParticleStreamContactManager& cm = contactManagers[i];
		if(!cm.isDynamic)
			continue;

		updateFluidBodyContactPair(particlesSp, numParticlesSp, collDataSp, constraintBufs, perParticleCacheLocal,
		                           localCellHash, packetCorner, cm, w2sTransforms[i]);

		numDynamicShapes++;
	}

	PxF32 maxMotionDistanceSqr = mParams.maxMotionDistance * mParams.maxMotionDistance;

	if(numDynamicShapes > 0)
	{
		bool isTwoWay = (mParams.flags & PxParticleBaseFlag::eCOLLISION_TWOWAY) != 0;
		for(PxU32 p = 0; p < numParticlesSp; p++)
		{
			ParticleCollData& collData = collDataSp[p];
			collisionResponse(collData, isTwoWay, false, mParams);
			clampToMaxMotion(collData.newPos, collData.oldPos, mParams.maxMotionDistance, maxMotionDistanceSqr);
			collData.flags &= ~ParticleCollisionFlags::CC;
			collData.flags &= ~ParticleCollisionFlags::DC;
			collData.flags |= ParticleCollisionFlags::RESET_SNORMAL;
			collData.surfacePos = PxVec3(0);
			// we need to keep the dynamic surface velocity for providing collision velocities in finalization
			// collData.surfaceVel = PxVec3(0);
			collData.ccTime = 1.0f;
		}
	}

	//
	// Collide with static shapes
	// (Static shapes need to be processed after dynamic shapes to avoid that dynamic shapes push
	//  particles into static shapes)
	//

	bool loadedCache = false;
	for(PxU32 i = 0; i < numContactManagers; i++)
	{
		const ParticleStreamContactManager& cm = contactManagers[i];
		if(cm.isDynamic)
			continue;

		const Gu::GeometryUnion& shape = cm.shapeCore->geometry;
		if(perParticleCacheLocal && (!loadedCache) && (shape.getType() == PxGeometryType::eTRIANGLEMESH))
		{
			for(PxU32 p = 0; p < numParticlesSp; p++)
			{
				PxU32 particleIndex = particleIndicesSp[p];
				perParticleCacheLocal[p] = perParticleCacheGlobal[particleIndex];
			}
			loadedCache = true;
		}

		updateFluidBodyContactPair(particlesSp, numParticlesSp, collDataSp, constraintBufs, perParticleCacheLocal,
		                           localCellHash, packetCorner, cm, w2sTransforms[i]);
	}

	if(loadedCache)
	{
		for(PxU32 p = 0; p < numParticlesSp; p++)
		{
			PxU32 particleIndex = particleIndicesSp[p];
			perParticleCacheGlobal[particleIndex] = perParticleCacheLocal[p];
		}
	}

	for(PxU32 p = 0; p < numParticlesSp; p++)
	{
		ParticleCollData& collData = collDataSp[p];
		Particle& particle = particlesSp[p];

		collisionResponse(collData, false, true, mParams);

		// Clamp new particle position to maximum motion.
		clampToMaxMotion(collData.newPos, collData.oldPos, mParams.maxMotionDistance, maxMotionDistanceSqr);

		// Update particle
		updateParticle(particle, collData, (mParams.flags & PxParticleBaseFlag::ePROJECT_TO_PLANE) != 0,
		               mParams.projectionPlane, worldBounds);
	}

	if(transientBuf)
	{
		for(PxU32 p = 0; p < numParticlesSp; p++)
		{
			ParticleCollData& collData = collDataSp[p];
			transientBuf[collData.origParticleIndex] = collData.surfaceNormal;
		}
	}

	if(collisionVelocities)
	{
		for(PxU32 p = 0; p < numParticlesSp; p++)
		{
			ParticleCollData& collData = collDataSp[p];
			PxVec3 collisionVelocity = particlesSp[p].velocity - collData.surfaceVel;
			collisionVelocities[collData.origParticleIndex] = collisionVelocity;
		}
	}

	if(fluidTwoWayData)
	{
		for(PxU32 p = 0; p < numParticlesSp; p++)
		{
			ParticleCollData& collData = collDataSp[p];
			PX_ASSERT(!collData.twoWayBody || (particlesSp[p].flags.api & PxParticleFlag::eCOLLISION_WITH_DYNAMIC));
			fluidTwoWayData[collData.origParticleIndex].body = collData.twoWayBody;
			fluidTwoWayData[collData.origParticleIndex].impulse = collData.twoWayImpulse;
		}
	}

	PX_FREE(collDataSp);
}

void Collision::updateFluidBodyContactPair(const Particle* particles, PxU32 numParticles,
                                           ParticleCollData* particleCollData, ConstraintBuffers& constraintBufs,
                                           ParticleOpcodeCache* opcodeCacheLocal, LocalCellHash& localCellHash,
                                           const PxVec3& packetCorner, const ParticleStreamContactManager& contactManager,
                                           const W2STransformTemp& w2sTransform)
{
	PX_ASSERT(particles);
	PX_ASSERT(particleCollData);

	bool isStaticMeshType = false;

	const Gu::GeometryUnion& shape = contactManager.shapeCore->geometry;
	const PxsBodyCore* body = contactManager.isDynamic ? static_cast<const PxsBodyCore*>(contactManager.rigidCore) : NULL;

	const PxTransform& world2Shape = w2sTransform.w2sNew;
	const PxTransform& world2ShapeOld = w2sTransform.w2sOld;
	const PxTransform shape2World = world2Shape.getInverse();

	for(PxU32 p = 0; p < numParticles; p++)
	{
		ParticleCollData& collData = particleCollData[p];

		collData.localFlags = (collData.flags & ParticleCollisionFlags::CC);
		// Transform position from world to shape space
		collData.localNewPos = world2Shape.transform(collData.newPos);
		collData.localOldPos = world2ShapeOld.transform(collData.oldPos);
		collData.c0 = constraintBufs.constraint0Buf + collData.origParticleIndex;
		collData.c1 = constraintBufs.constraint1Buf + collData.origParticleIndex;
		collData.localSurfaceNormal = PxVec3(0.0f);
		collData.localSurfacePos = PxVec3(0.0f);
	}

	switch(shape.getType())
	{
	case PxGeometryType::eSPHERE:
	{
		collideWithSphere(particleCollData, numParticles, shape, mParams.contactOffset);
		break;
	}
	case PxGeometryType::ePLANE:
	{
		collideWithPlane(particleCollData, numParticles, shape, mParams.contactOffset);
		break;
	}
	case PxGeometryType::eCAPSULE:
	{
		collideWithCapsule(particleCollData, numParticles, shape, mParams.contactOffset);
		break;
	}
	case PxGeometryType::eBOX:
	{
		collideWithBox(particleCollData, numParticles, shape, mParams.contactOffset);
		break;
	}
	case PxGeometryType::eCONVEXMESH:
	{
		const PxConvexMeshGeometryLL& convexShapeData = shape.get<const PxConvexMeshGeometryLL>();
		const Gu::ConvexHullData* convexHullData = convexShapeData.hullData;
		PX_ASSERT(convexHullData);

		PX_ALLOCA(scaledPlanesBuf, PxPlane, convexHullData->mNbPolygons);
		collideWithConvex(scaledPlanesBuf, particleCollData, numParticles, shape, mParams.contactOffset);
		break;
	}
	case PxGeometryType::eTRIANGLEMESH:
	{
		if(opcodeCacheLocal)
		{
			collideWithStaticMesh(numParticles, particleCollData, opcodeCacheLocal, shape, world2Shape, shape2World,
			                      mParams.cellSize, mParams.collisionRange, mParams.contactOffset);
		}
		else
		{
			// Compute cell hash if needed
			if(!localCellHash.isHashValid)
			{
				PX_ALLOCA(hashKeyArray, PxU16, numParticles * sizeof(PxU16)); // save the hashkey for reorder
				PX_ASSERT(hashKeyArray);
				computeLocalCellHash(localCellHash, hashKeyArray, particles, numParticles, packetCorner,
				                     mParams.cellSizeInv);
			}

			collideCellsWithStaticMesh(particleCollData, localCellHash, shape, world2Shape, shape2World,
			                           mParams.cellSize, mParams.collisionRange, mParams.contactOffset, packetCorner);
		}
		isStaticMeshType = true;
		break;
	}
	case PxGeometryType::eHEIGHTFIELD:
	{
		collideWithStaticHeightField(particleCollData, numParticles, shape, mParams.contactOffset, shape2World);
		isStaticMeshType = true;
		break;
	}
	case PxGeometryType::eGEOMETRY_COUNT:
	case PxGeometryType::eINVALID:
		PX_ASSERT(0);
	}

	if(isStaticMeshType)
	{
		for(PxU32 p = 0; p < numParticles; p++)
		{
			ParticleCollData& collData = particleCollData[p];
			updateCollDataStaticMesh(collData, shape2World, mParams.timeStep);
		}
	}
	else if(body)
	{
		for(PxU32 p = 0; p < numParticles; p++)
		{
			ParticleCollData& collData = particleCollData[p];
			ConstraintDynamic cdTemp;
			ConstraintDynamic& c0Dynamic = constraintBufs.constraint0DynamicBuf
			                                   ? constraintBufs.constraint0DynamicBuf[collData.origParticleIndex]
			                                   : cdTemp;
			ConstraintDynamic& c1Dynamic = constraintBufs.constraint1DynamicBuf
			                                   ? constraintBufs.constraint1DynamicBuf[collData.origParticleIndex]
			                                   : cdTemp;
			c0Dynamic.setEmpty();
			c1Dynamic.setEmpty();
			updateCollDataDynamic(collData, body->body2World, body->linearVelocity, body->angularVelocity, body,
			                      shape2World, mParams.timeStep, c0Dynamic, c1Dynamic);
		}
	}
	else
	{
		for(PxU32 p = 0; p < numParticles; p++)
		{
			ParticleCollData& collData = particleCollData[p];

			updateCollDataStatic(collData, shape2World, mParams.timeStep);
		}
	}

	if(contactManager.isDrain)
	{
		for(PxU32 p = 0; p < numParticles; p++)
		{
			ParticleCollData& collData = particleCollData[p];

			if((collData.localFlags & ParticleCollisionFlags::L_ANY) != 0)
			{
				collData.particleFlags.api |= PxParticleFlag::eCOLLISION_WITH_DRAIN;
			}
		}
	}
}

#endif // PX_USE_PARTICLE_SYSTEM_API
