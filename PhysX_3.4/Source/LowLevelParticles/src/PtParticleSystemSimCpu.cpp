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

#include "PtParticleSystemSimCpu.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "foundation/PxProfiler.h"
#include "PxvGeometry.h"
#include "PtContext.h"
#include "PtParticleShapeCpu.h"

//----------------------------------------------------------------------------//

// Standard value for particle resolution
#define PXN_FLUID_REST_PARTICLE_PER_UNIT_STD 10.0f

// Macros to clamp restitution and adhesion (particle collision) to values that give stable results.
#define DYNAMIC_FRICTION_CLAMP 0.001f
#define RESTITUTION_CLAMP 0.05f

#define CLAMP_DYNAMIC_FRICTION(t) PxClamp(t, DYNAMIC_FRICTION_CLAMP, 1.0f)
#define CLAMP_RESTITUTION(t) PxClamp(t, 0.0f, 1.0f - RESTITUTION_CLAMP)

using namespace physx;
using namespace Pt;

//----------------------------------------------------------------------------//

ParticleSystemState& ParticleSystemSimCpu::getParticleStateV()
{
	PX_ASSERT(mParticleState);
	return *mParticleState;
}

//----------------------------------------------------------------------------//

void ParticleSystemSimCpu::getSimParticleDataV(ParticleSystemSimDataDesc& simParticleData, bool) const
{
	simParticleData.densities = PxStrideIterator<const PxF32>();
	simParticleData.collisionNormals = PxStrideIterator<const PxVec3>();
	simParticleData.collisionVelocities = PxStrideIterator<const PxVec3>();
	simParticleData.twoWayImpluses = PxStrideIterator<const PxVec3>();
	simParticleData.twoWayBodies = PxStrideIterator<BodyHandle>();

	if(mParticleState->getParticleCount() > 0 && mSimulated)
	{
		if(mParameter->particleReadDataFlags & PxParticleReadDataFlag::eDENSITY_BUFFER)
			simParticleData.densities =
			    PxStrideIterator<const PxF32>(&mParticleState->getParticleBuffer()->density, sizeof(Particle));

		if(mParameter->particleReadDataFlags & PxParticleReadDataFlag::eCOLLISION_NORMAL_BUFFER)
			simParticleData.collisionNormals = PxStrideIterator<const PxVec3>(mTransientBuffer, sizeof(PxVec3));

		if(mParameter->particleReadDataFlags & PxParticleReadDataFlag::eCOLLISION_VELOCITY_BUFFER)
			simParticleData.collisionVelocities = PxStrideIterator<const PxVec3>(mCollisionVelocities);

		if(mFluidTwoWayData)
		{
			simParticleData.twoWayImpluses =
			    PxStrideIterator<const PxVec3>(&mFluidTwoWayData->impulse, sizeof(TwoWayData));
			simParticleData.twoWayBodies =
			    PxStrideIterator<BodyHandle>(reinterpret_cast<BodyHandle*>(&mFluidTwoWayData->body), sizeof(TwoWayData));
		}
	}
}

//----------------------------------------------------------------------------//

/**
Will be called from HL twice per step. Once after the shape update (at the start of the frame) has been executed,
and once after the particle pipeline has finished.
*/
void ParticleSystemSimCpu::getShapesUpdateV(ParticleShapeUpdateResults& updateResults) const
{
	PX_ASSERT(mIsSimulated);

	updateResults.destroyedShapeCount = mNumDeletedParticleShapes;
	updateResults.destroyedShapes = mCreatedDeletedParticleShapes;

	updateResults.createdShapeCount = mNumCreatedParticleShapes;
	updateResults.createdShapes = mCreatedDeletedParticleShapes + mNumDeletedParticleShapes;
}

//----------------------------------------------------------------------------//

physx::PxBaseTask& ParticleSystemSimCpu::schedulePacketShapesUpdate(const ParticleShapesUpdateInput& input,
                                                                    physx::PxBaseTask& continuation)
{
	mPacketShapesFinalizationTask.setContinuation(&continuation);
	mPacketShapesUpdateTask.setContinuation(&mPacketShapesFinalizationTask);
	mPacketShapesFinalizationTask.removeReference();
	mPacketShapesUpdateTaskInput = input;
	return mPacketShapesUpdateTask;
}

//----------------------------------------------------------------------------//

physx::PxBaseTask& ParticleSystemSimCpu::scheduleDynamicsUpdate(physx::PxBaseTask& continuation)
{
	if(mParameter->flags & InternalParticleSystemFlag::eSPH)
	{
		mDynamicsUpdateTask.setContinuation(&continuation);
		return mDynamicsUpdateTask;
	}
	else
	{
		continuation.addReference();
		return continuation;
	}
}

//----------------------------------------------------------------------------//

physx::PxBaseTask& ParticleSystemSimCpu::scheduleCollisionUpdate(physx::PxBaseTask& continuation)
{
	mCollisionFinalizationTask.setContinuation(&continuation);
	mCollisionUpdateTask.setContinuation(&mCollisionFinalizationTask);
	mCollisionFinalizationTask.removeReference();
	return mCollisionUpdateTask;
}

//----------------------------------------------------------------------------//

void ParticleSystemSimCpu::spatialHashUpdateSections(physx::PxBaseTask* continuation)
{
	PX_ASSERT(mParameter->flags & InternalParticleSystemFlag::eSPH);

	// Split each packet into sections and reorder particles of a packet according to these sections

	mSpatialHash->updatePacketSections(mPacketParticlesIndices, mParticleState->getParticleBuffer(), continuation);
}

void ParticleSystemSimCpu::packetShapesUpdate(physx::PxBaseTask*)
{
	PX_ASSERT(mIsSimulated);
	PX_ASSERT(mSpatialHash);

	// Init parameters for tracking of new/deleted fluid shapes
	mNumCreatedParticleShapes = 0;
	mNumDeletedParticleShapes = 0;

	if(mParticleState->getValidParticleRange() > 0)
	{
		if(!mPacketParticlesIndices)
			mPacketParticlesIndices = reinterpret_cast<PxU32*>(
			    mAlign16.allocate(mParticleState->getMaxParticles() * sizeof(PxU32), __FILE__, __LINE__));

		physx::PxBaseTask* cont;
		if(mParameter->flags & InternalParticleSystemFlag::eSPH)
		{
			cont = &mSpatialHashUpdateSectionsTask;
			mSpatialHashUpdateSectionsTask.setContinuation(&mPacketShapesFinalizationTask);
		}
		else
		{
			cont = &mPacketShapesFinalizationTask;
			cont->addReference();
		}

		// Hash particles to packets and reorder particle indices

		mSpatialHash->updatePacketHash(mNumPacketParticlesIndices, mPacketParticlesIndices,
		                               mParticleState->getParticleBuffer(), mParticleState->getParticleMap(),
		                               mParticleState->getValidParticleRange(), cont);
	}
}

//----------------------------------------------------------------------------//

void ParticleSystemSimCpu::packetShapesFinalization(physx::PxBaseTask*)
{
	// - Find for each packet shape the related packet and adjust the mapping.
	// - Track created / deleted packets.
	remapShapesToPackets(mPacketShapesUpdateTaskInput.shapes, mPacketShapesUpdateTaskInput.shapeCount);

	// release the shapes, since their ownership was tranferred to us.
	if(mPacketShapesUpdateTaskInput.shapes)
		PX_FREE(mPacketShapesUpdateTaskInput.shapes);
}

//----------------------------------------------------------------------------//

void ParticleSystemSimCpu::dynamicsUpdate(physx::PxBaseTask* continuation)
{
	PX_ASSERT(mParameter->flags & InternalParticleSystemFlag::eSPH);
	PX_ASSERT(mIsSimulated);
	PX_ASSERT(mSpatialHash);
	PX_ASSERT(continuation);

	if(mNumPacketParticlesIndices > 0)
	{
		updateDynamicsParameter();

		if(mParameter->flags & InternalParticleSystemFlag::eSPH)
		{
			mDynamics.updateSph(*continuation);
		}
	}
}

//----------------------------------------------------------------------------//

void ParticleSystemSimCpu::collisionUpdate(physx::PxBaseTask* continuation)
{
	PX_ASSERT(mIsSimulated);
	PX_ASSERT(mSpatialHash);
	PX_ASSERT(mCollisionUpdateTaskInput.contactManagerStream);
	PX_ASSERT(continuation);

	updateCollisionParameter();

	mParticleState->getWorldBounds().setEmpty();

	mCollision.updateCollision(mCollisionUpdateTaskInput.contactManagerStream, *continuation);
	mCollision.updateOverflowParticles();
}

//----------------------------------------------------------------------------//

void ParticleSystemSimCpu::collisionFinalization(physx::PxBaseTask*)
{
	PX_FREE(mCollisionUpdateTaskInput.contactManagerStream);
	mCollisionUpdateTaskInput.contactManagerStream = NULL;

	mSimulated = true;

	// clear shape update
	mNumDeletedParticleShapes = 0;
	mNumCreatedParticleShapes = 0;
}

//----------------------------------------------------------------------------//

void ParticleSystemSimCpu::setExternalAccelerationV(const PxVec3& v)
{
	mExternalAcceleration = v;
}

//----------------------------------------------------------------------------//

const PxVec3& ParticleSystemSimCpu::getExternalAccelerationV() const
{
	return mExternalAcceleration;
}

//----------------------------------------------------------------------------//

void ParticleSystemSimCpu::setSimulationTimeStepV(PxReal value)
{
	PX_ASSERT(value >= 0.0f);

	mSimulationTimeStep = value;
}

//----------------------------------------------------------------------------//

PxReal ParticleSystemSimCpu::getSimulationTimeStepV() const
{
	return mSimulationTimeStep;
}

//----------------------------------------------------------------------------//

void ParticleSystemSimCpu::setSimulatedV(bool isSimulated)
{
	mIsSimulated = isSimulated;
	if(!isSimulated)
		clearParticleConstraints();
}

//----------------------------------------------------------------------------//

Ps::IntBool ParticleSystemSimCpu::isSimulatedV() const
{
	return mIsSimulated;
}

//----------------------------------------------------------------------------//

ParticleSystemSimCpu::ParticleSystemSimCpu(ContextCpu* context, PxU32 index)
: mContext(*context)
, mParticleState(NULL)
, mSimulated(false)
, mFluidTwoWayData(NULL)
, mCreatedDeletedParticleShapes(NULL)
, mPacketParticlesIndices(NULL)
, mNumPacketParticlesIndices(0)
, mOpcodeCacheBuffer(NULL)
, mTransientBuffer(NULL)
, mCollisionVelocities(NULL)
, mDynamics(*this)
, mCollision(*this)
, mIndex(index)
, mPacketShapesUpdateTask(0, this, "Pt::ParticleSystemSimCpu.packetShapesUpdate")
, mPacketShapesFinalizationTask(0, this, "Pt::ParticleSystemSimCpu.packetShapesFinalization")
, mDynamicsUpdateTask(0, this, "Pt::ParticleSystemSimCpu.dynamicsUpdate")
, mCollisionUpdateTask(0, this, "Pt::ParticleSystemSimCpu.collisionUpdate")
, mCollisionFinalizationTask(0, this, "Pt::ParticleSystemSimCpu.collisionFinalization")
, mSpatialHashUpdateSectionsTask(0, this, "Pt::ParticleSystemSimCpu.spatialHashUpdateSections")
{
}

//----------------------------------------------------------------------------//

ParticleSystemSimCpu::~ParticleSystemSimCpu()
{
}

//----------------------------------------------------------------------------//

void ParticleSystemSimCpu::init(ParticleData& particleData, const ParticleSystemParameter& parameter)
{
	mParticleState = &particleData;
	mParticleState->clearSimState();
	mParameter = &parameter;
	mSimulationTimeStep = 0.0f;
	mExternalAcceleration = PxVec3(0);
	mPacketParticlesIndices = NULL;

	initializeParameter();

	PxU32 maxParticles = mParticleState->getMaxParticles();

	// Initialize buffers
	mConstraintBuffers.constraint0Buf =
	    reinterpret_cast<Constraint*>(mAlign16.allocate(maxParticles * sizeof(Constraint), __FILE__, __LINE__));
	mConstraintBuffers.constraint1Buf =
	    reinterpret_cast<Constraint*>(mAlign16.allocate(maxParticles * sizeof(Constraint), __FILE__, __LINE__));
	if(mParameter->flags & PxParticleBaseFlag::eCOLLISION_WITH_DYNAMIC_ACTORS)
	{
		mConstraintBuffers.constraint0DynamicBuf = reinterpret_cast<ConstraintDynamic*>(
		    mAlign16.allocate(maxParticles * sizeof(ConstraintDynamic), __FILE__, __LINE__));
		mConstraintBuffers.constraint1DynamicBuf = reinterpret_cast<ConstraintDynamic*>(
		    mAlign16.allocate(maxParticles * sizeof(ConstraintDynamic), __FILE__, __LINE__));
	}
	else
	{
		mConstraintBuffers.constraint0DynamicBuf = NULL;
		mConstraintBuffers.constraint1DynamicBuf = NULL;
	}

	if((mParameter->flags & PxParticleBaseFlag::eCOLLISION_TWOWAY) &&
	   (mParameter->flags & PxParticleBaseFlag::eCOLLISION_WITH_DYNAMIC_ACTORS))
		mFluidTwoWayData =
		    reinterpret_cast<TwoWayData*>(mAlign16.allocate(maxParticles * sizeof(TwoWayData), __FILE__, __LINE__));

#if PX_CHECKED
	{
		PxU32 numWords = maxParticles * sizeof(Constraint) >> 2;
		for(PxU32 i = 0; i < numWords; ++i)
		{
			reinterpret_cast<PxU32*>(mConstraintBuffers.constraint0Buf)[i] = 0xDEADBEEF;
			reinterpret_cast<PxU32*>(mConstraintBuffers.constraint1Buf)[i] = 0xDEADBEEF;
		}
	}
#endif

	if(mParameter->flags & PxParticleBaseFlag::ePER_PARTICLE_COLLISION_CACHE_HINT)
	{
		mOpcodeCacheBuffer = reinterpret_cast<ParticleOpcodeCache*>(
		    mAlign16.allocate(maxParticles * sizeof(ParticleOpcodeCache), __FILE__, __LINE__));
#if PX_CHECKED
		// sschirm: avoid reading uninitialized mGeom in ParticleOpcodeCache::read in assert statement
		PxMemZero(mOpcodeCacheBuffer, maxParticles * sizeof(ParticleOpcodeCache));
#endif
	}

	if((mParameter->flags & InternalParticleSystemFlag::eSPH) ||
	   (mParameter->particleReadDataFlags & PxParticleReadDataFlag::eCOLLISION_NORMAL_BUFFER))
		mTransientBuffer =
		    reinterpret_cast<PxVec3*>(mAlign16.allocate(maxParticles * sizeof(PxVec3), __FILE__, __LINE__));

	if(mParameter->particleReadDataFlags & PxParticleReadDataFlag::eCOLLISION_VELOCITY_BUFFER)
		mCollisionVelocities =
		    reinterpret_cast<PxVec3*>(mAlign16.allocate(maxParticles * sizeof(PxVec3), __FILE__, __LINE__));

	mCreatedDeletedParticleShapes = reinterpret_cast<ParticleShape**>(
	    PX_ALLOC(2 * PT_PARTICLE_SYSTEM_PACKET_HASH_SIZE * sizeof(ParticleShape*), "ParticleShape*"));
	mNumCreatedParticleShapes = 0;
	mNumDeletedParticleShapes = 0;

	// Create object for spatial hashing.
	mSpatialHash = reinterpret_cast<SpatialHash*>(PX_ALLOC(sizeof(SpatialHash), "SpatialHash"));
	if(mSpatialHash)
	{
		new (mSpatialHash) SpatialHash(PT_PARTICLE_SYSTEM_PACKET_HASH_SIZE, mDynamics.getParameter().cellSizeInv,
		                               mParameter->packetSizeMultiplierLog2,
		                               (mParameter->flags & InternalParticleSystemFlag::eSPH) != 0);
	}

	mCollisionUpdateTaskInput.contactManagerStream = NULL;

	// Make sure we start deactivated.
	mSimulated = false;
}

//----------------------------------------------------------------------------//

void ParticleSystemSimCpu::clear()
{
	mDynamics.clear();

	if(mSpatialHash)
	{
		mSpatialHash->~SpatialHash();
		PX_FREE(mSpatialHash);
		mSpatialHash = NULL;
	}

	// Free particle buffers
	mAlign16.deallocate(mConstraintBuffers.constraint0Buf);
	mConstraintBuffers.constraint0Buf = NULL;

	mAlign16.deallocate(mConstraintBuffers.constraint1Buf);
	mConstraintBuffers.constraint1Buf = NULL;

	if(mConstraintBuffers.constraint0DynamicBuf)
	{
		mAlign16.deallocate(mConstraintBuffers.constraint0DynamicBuf);
		mConstraintBuffers.constraint0DynamicBuf = NULL;
	}

	if(mConstraintBuffers.constraint1DynamicBuf)
	{
		mAlign16.deallocate(mConstraintBuffers.constraint1DynamicBuf);
		mConstraintBuffers.constraint1DynamicBuf = NULL;
	}

	if(mOpcodeCacheBuffer)
	{
		mAlign16.deallocate(mOpcodeCacheBuffer);
		mOpcodeCacheBuffer = NULL;
	}

	if(mTransientBuffer)
	{
		mAlign16.deallocate(mTransientBuffer);
		mTransientBuffer = NULL;
	}

	if(mCollisionVelocities)
	{
		mAlign16.deallocate(mCollisionVelocities);
		mCollisionVelocities = NULL;
	}

	if(mCreatedDeletedParticleShapes)
	{
		PX_FREE(mCreatedDeletedParticleShapes);
		mCreatedDeletedParticleShapes = NULL;
	}

	if(mPacketParticlesIndices)
	{
		mAlign16.deallocate(mPacketParticlesIndices);
		mPacketParticlesIndices = NULL;
	}
	mNumPacketParticlesIndices = 0;

	if(mFluidTwoWayData)
	{
		mAlign16.deallocate(mFluidTwoWayData);
		mFluidTwoWayData = NULL;
	}

	mSimulated = false;

	if(mParticleState)
	{
		mParticleState->release();
		mParticleState = NULL;
	}
}

//----------------------------------------------------------------------------//

ParticleData* ParticleSystemSimCpu::obtainParticleState()
{
	PX_ASSERT(mParticleState);
	ParticleData* tmp = mParticleState;
	mParticleState = NULL;
	return tmp;
}

//----------------------------------------------------------------------------//

void ParticleSystemSimCpu::remapShapesToPackets(ParticleShape* const* shapes, PxU32 numShapes)
{
	PX_ASSERT(mNumCreatedParticleShapes == 0);
	PX_ASSERT(mNumDeletedParticleShapes == 0);

	if(mParticleState->getValidParticleRange() > 0)
	{
		PX_ASSERT(mSpatialHash);

		Cm::BitMap mappedFluidPackets; // Marks the fluid packets that are mapped to a fluid shape.
		mappedFluidPackets.resizeAndClear(PT_PARTICLE_SYSTEM_PACKET_HASH_SIZE);

		// Find for each shape the corresponding packet. If it does not exist the shape has to be deleted.
		for(PxU32 i = 0; i < numShapes; i++)
		{
			ParticleShapeCpu* shape = static_cast<ParticleShapeCpu*>(shapes[i]);

			PxU32 hashIndex;
			const ParticleCell* particlePacket = mSpatialHash->findCell(hashIndex, shape->getPacketCoordinates());
			if(particlePacket)
			{
				shape->setFluidPacket(particlePacket);

				// Mark packet as mapped.
				mappedFluidPackets.set(hashIndex);
			}
			else
			{
				mCreatedDeletedParticleShapes[mNumDeletedParticleShapes++] = shape;
			}
		}

		// Check for each packet whether it is mapped to a fluid shape. If not, a new shape must be created.
		const ParticleCell* fluidPackets = mSpatialHash->getPackets();
		PX_ASSERT((mappedFluidPackets.getWordCount() << 5) >= PT_PARTICLE_SYSTEM_PACKET_HASH_SIZE);
		for(PxU32 p = 0; p < PT_PARTICLE_SYSTEM_PACKET_HASH_SIZE; p++)
		{
			if((!mappedFluidPackets.test(p)) && (fluidPackets[p].numParticles != PX_INVALID_U32))
			{
				ParticleShapeCpu* shape = mContext.createParticleShape(this, &fluidPackets[p]);
				if(shape)
				{
					mCreatedDeletedParticleShapes[mNumDeletedParticleShapes + mNumCreatedParticleShapes++] = shape;
				}
			}
		}
	}
	else
	{
		// Release all shapes.
		for(PxU32 i = 0; i < numShapes; i++)
		{
			ParticleShapeCpu* shape = static_cast<ParticleShapeCpu*>(shapes[i]);
			mCreatedDeletedParticleShapes[mNumDeletedParticleShapes++] = shape;
		}
	}
}

//----------------------------------------------------------------------------//
// Body Shape Reference Invalidation
//----------------------------------------------------------------------------//

/**
Removes all BodyShape references.
Only the info in the Particle (constraint0Info, constraint1Info) need
to be cleared, since they are checked before copying references from the constraints
to the TwoWayData, where it is finally used for dereferencing.
*/
void ParticleSystemSimCpu::clearParticleConstraints()
{
	Particle* particleBuffer = mParticleState->getParticleBuffer();
	Cm::BitMap::Iterator it(mParticleState->getParticleMap());
	for(PxU32 particleIndex = it.getNext(); particleIndex != Cm::BitMap::Iterator::DONE; particleIndex = it.getNext())
	{
		Particle& particle = particleBuffer[particleIndex];
		particle.flags.low &= PxU16(~InternalParticleFlag::eANY_CONSTRAINT_VALID);
	}
}

//----------------------------------------------------------------------------//

/**
Updates shape transform hash from context and removes references to a rigid body that was deleted.
*/
void ParticleSystemSimCpu::removeInteractionV(const ParticleShape& particleShape, ShapeHandle shape, BodyHandle body,
                                              bool isDynamic, bool isDyingRb, bool)
{
	const PxsShapeCore* pxsShape = reinterpret_cast<const PxsShapeCore*>(shape);
	const ParticleShapeCpu& pxsParticleShape = static_cast<const ParticleShapeCpu&>(particleShape);

	if(isDyingRb)
	{
		if(isDynamic)
		{
			if(mFluidTwoWayData)
			{
				// just call when packets cover the same particles when constraints where
				// generated (which is the case with isDyingRb).
				removeTwoWayRbReferences(pxsParticleShape, reinterpret_cast<const PxsBodyCore*>(body));
			}
		}
		else if(mOpcodeCacheBuffer && pxsShape->geometry.getType() == PxGeometryType::eTRIANGLEMESH)
		{
			// just call when packets cover the same particles when cache was used last (must be the last simulation
			// step,
			// since the cache gets invalidated after one step not being used).
			setCollisionCacheInvalid(pxsParticleShape, pxsShape->geometry);
		}
	}
}

//----------------------------------------------------------------------------//

void ParticleSystemSimCpu::onRbShapeChangeV(const ParticleShape& particleShape, ShapeHandle shape)
{
	const PxsShapeCore* pxsShape = reinterpret_cast<const PxsShapeCore*>(shape);
	const ParticleShapeCpu& pxsParticleShape = static_cast<const ParticleShapeCpu&>(particleShape);

	if(mOpcodeCacheBuffer && pxsShape->geometry.getType() == PxGeometryType::eTRIANGLEMESH)
	{
		// just call when packets cover the same particles when cache was used last (must be the last simulation step,
		// since the cache gets invalidated after one step not being used).
		setCollisionCacheInvalid(pxsParticleShape, pxsShape->geometry);
	}
}

//----------------------------------------------------------------------------//

void ParticleSystemSimCpu::passCollisionInputV(ParticleCollisionUpdateInput input)
{
	PX_ASSERT(mCollisionUpdateTaskInput.contactManagerStream == NULL);
	mCollisionUpdateTaskInput = input;
}

//----------------------------------------------------------------------------//

/**
Removes specific PxsShapeCore references from particles belonging to a certain shape.
The constraint data itself needs to be accessed, because it's assumed that if there
is only one constraint, it's in the slot 1 of the constraint pair.

Should only be called when packets cover the same particles when constraints where generated!
*/
void ParticleSystemSimCpu::removeTwoWayRbReferences(const ParticleShapeCpu& particleShape, const PxsBodyCore* rigidBody)
{
	PX_ASSERT(mFluidTwoWayData);
	PX_ASSERT(mConstraintBuffers.constraint0DynamicBuf);
	PX_ASSERT(mConstraintBuffers.constraint1DynamicBuf);
	PX_ASSERT(rigidBody);
	PX_ASSERT(particleShape.getFluidPacket());
	const ParticleCell* packet = particleShape.getFluidPacket();
	Particle* particleBuffer = mParticleState->getParticleBuffer();

	PxU32 endIndex = packet->firstParticle + packet->numParticles;
	for(PxU32 i = packet->firstParticle; i < endIndex; ++i)
	{
		// update particles for shapes that have been deleted!
		PxU32 particleIndex = mPacketParticlesIndices[i];
		Particle& particle = particleBuffer[particleIndex];

		// we need to skip invalid particles
		// it may be that a particle has been deleted prior to the deletion of the RB
		// it may also be that a particle has been re-added to the same index, in which case
		// the particle.flags.low will have been overwritten
		if(!(particle.flags.api & PxParticleFlag::eVALID))
			continue;

		if(!(particle.flags.low & InternalParticleFlag::eANY_CONSTRAINT_VALID))
			continue;

		Constraint& c0 = mConstraintBuffers.constraint0Buf[particleIndex];
		Constraint& c1 = mConstraintBuffers.constraint1Buf[particleIndex];
		ConstraintDynamic& cd0 = mConstraintBuffers.constraint0DynamicBuf[particleIndex];
		ConstraintDynamic& cd1 = mConstraintBuffers.constraint1DynamicBuf[particleIndex];

		if(reinterpret_cast<const PxsBodyCore*>(rigidBody) == cd1.twoWayBody)
		{
			particle.flags.low &=
			    PxU16(~(InternalParticleFlag::eCONSTRAINT_1_VALID | InternalParticleFlag::eCONSTRAINT_1_DYNAMIC));
		}

		if(reinterpret_cast<const PxsBodyCore*>(rigidBody) == cd0.twoWayBody)
		{
			if(!(particle.flags.low & InternalParticleFlag::eCONSTRAINT_1_VALID))
			{
				particle.flags.low &=
				    PxU16(~(InternalParticleFlag::eCONSTRAINT_0_VALID | InternalParticleFlag::eCONSTRAINT_0_DYNAMIC));
			}
			else
			{
				c0 = c1;
				cd0 = cd1;
				particle.flags.low &=
				    PxU16(~(InternalParticleFlag::eCONSTRAINT_1_VALID | InternalParticleFlag::eCONSTRAINT_1_DYNAMIC));
			}
		}
	}
}

//----------------------------------------------------------------------------//

/**
Should only be called when packets cover the same particles when cache was used last.
I.e. after the last collision update and before the next shape update.
It's ok if particles where replaced or removed from the corresponding packet intervalls,
since the cache updates will not do any harm for those.
*/
void ParticleSystemSimCpu::setCollisionCacheInvalid(const ParticleShapeCpu& particleShape,
                                                    const Gu::GeometryUnion& geometry)
{
	PX_ASSERT(mOpcodeCacheBuffer);
	PX_ASSERT(particleShape.getFluidPacket());
	const ParticleCell* packet = particleShape.getFluidPacket();
	Particle* particleBuffer = mParticleState->getParticleBuffer();

	PxU32 endIndex = packet->firstParticle + packet->numParticles;
	for(PxU32 i = packet->firstParticle; i < endIndex; ++i)
	{
		// update particles for shapes that have been deleted!
		PxU32 particleIndex = mPacketParticlesIndices[i];
		Particle& particle = particleBuffer[particleIndex];

		if((particle.flags.low & InternalParticleFlag::eGEOM_CACHE_MASK) != 0)
		{
			ParticleOpcodeCache& cache = mOpcodeCacheBuffer[particleIndex];
			if(cache.getGeometry() == &geometry)
				particle.flags.low &= ~PxU16(InternalParticleFlag::eGEOM_CACHE_MASK);
		}
	}
}

//----------------------------------------------------------------------------//

void ParticleSystemSimCpu::initializeParameter()
{
	const ParticleSystemParameter& parameter = *mParameter;

	DynamicsParameters& dynamicsParams = mDynamics.getParameter();

	// initialize dynamics parameter
	{
		PxReal restParticlesDistance = parameter.restParticleDistance;
		PxReal restParticlesDistanceStd = 1.0f / PXN_FLUID_REST_PARTICLE_PER_UNIT_STD;
		PxReal restParticlesDistance3 = restParticlesDistance * restParticlesDistance * restParticlesDistance;
		PxReal restParticlesDistanceStd3 = restParticlesDistanceStd * restParticlesDistanceStd * restParticlesDistanceStd;
		PX_UNUSED(restParticlesDistance3);

		dynamicsParams.initialDensity = parameter.restDensity;
		dynamicsParams.particleMassStd = dynamicsParams.initialDensity * restParticlesDistanceStd3;
		dynamicsParams.cellSize = parameter.kernelRadiusMultiplier * restParticlesDistance;
		dynamicsParams.cellSizeInv = 1.0f / dynamicsParams.cellSize;
		dynamicsParams.cellSizeSq = dynamicsParams.cellSize * dynamicsParams.cellSize;
		dynamicsParams.packetSize = dynamicsParams.cellSize * (1 << parameter.packetSizeMultiplierLog2);
		PxReal radiusStd = parameter.kernelRadiusMultiplier * restParticlesDistanceStd;
		PxReal radius2Std = radiusStd * radiusStd;
		PxReal radius6Std = radius2Std * radius2Std * radius2Std;
		PxReal radius9Std = radius6Std * radius2Std * radiusStd;
		PxReal wPoly6ScalarStd = 315.0f / (64.0f * PxPi * radius9Std);
		PxReal wSpikyGradientScalarStd = 1.5f * 15.0f / (PxPi * radius6Std);

		dynamicsParams.radiusStd = radiusStd;
		dynamicsParams.radiusSqStd = radius2Std;
		dynamicsParams.densityMultiplierStd = wPoly6ScalarStd * dynamicsParams.particleMassStd;
		dynamicsParams.stiffMulPressureMultiplierStd =
		    wSpikyGradientScalarStd * dynamicsParams.particleMassStd * parameter.stiffness;
		dynamicsParams.selfDensity = dynamicsParams.densityMultiplierStd * radius2Std * radius2Std * radius2Std;
		dynamicsParams.scaleToStd = restParticlesDistanceStd / restParticlesDistance;
		dynamicsParams.scaleSqToStd = dynamicsParams.scaleToStd * dynamicsParams.scaleToStd;
		dynamicsParams.scaleToWorld = 1.0f / dynamicsParams.scaleToStd;
		dynamicsParams.packetMultLog = parameter.packetSizeMultiplierLog2;

		PxReal densityRestOffset = (dynamicsParams.initialDensity - dynamicsParams.selfDensity);
		dynamicsParams.densityNormalizationFactor = (densityRestOffset > 0.0f) ? (1.0f / densityRestOffset) : 0.0f;

		updateDynamicsParameter();
	}

	CollisionParameters& collisionParams = mCollision.getParameter();

	// initialize collision parameter: these partially depend on dynamics parameters!
	{
		collisionParams.cellSize = dynamicsParams.cellSize;
		collisionParams.cellSizeInv = dynamicsParams.cellSizeInv;
		collisionParams.packetMultLog = parameter.packetSizeMultiplierLog2;
		collisionParams.packetMult = PxU32(1 << parameter.packetSizeMultiplierLog2);
		collisionParams.packetSize = dynamicsParams.packetSize;
		collisionParams.restOffset = parameter.restOffset;
		collisionParams.contactOffset = parameter.contactOffset;
		PX_ASSERT(collisionParams.contactOffset >= collisionParams.restOffset);
		collisionParams.maxMotionDistance = parameter.maxMotionDistance;
		collisionParams.collisionRange =
		    collisionParams.maxMotionDistance + collisionParams.contactOffset + PT_PARTICLE_SYSTEM_COLLISION_SLACK;
		updateCollisionParameter();
	}
}

//----------------------------------------------------------------------------//

PX_FORCE_INLINE PxF32 computeDampingFactor(PxF32 damping, PxF32 timeStep)
{
	PxF32 dampingDt = damping * timeStep;
	if(dampingDt < 1.0f)
		return 1.0f - dampingDt;
	else
		return 0.0f;
}

void ParticleSystemSimCpu::updateDynamicsParameter()
{
	const ParticleSystemParameter& parameter = *mParameter;
	DynamicsParameters& dynamicsParams = mDynamics.getParameter();

	PxReal restParticlesDistanceStd = 1.0f / PXN_FLUID_REST_PARTICLE_PER_UNIT_STD;
	PxReal radiusStd = parameter.kernelRadiusMultiplier * restParticlesDistanceStd;
	PxReal radius2Std = radiusStd * radiusStd;
	PxReal radius6Std = radius2Std * radius2Std * radius2Std;

	dynamicsParams.viscosityMultiplierStd =
	    computeViscosityMultiplier(parameter.viscosity, dynamicsParams.particleMassStd, radius6Std);
}

//----------------------------------------------------------------------------//

void ParticleSystemSimCpu::updateCollisionParameter()
{
	const ParticleSystemParameter& parameter = *mParameter;
	CollisionParameters& collisionParams = mCollision.getParameter();

	collisionParams.dampingDtComp = computeDampingFactor(parameter.damping, mSimulationTimeStep);
	collisionParams.externalAcceleration = mExternalAcceleration;

	collisionParams.projectionPlane.n = parameter.projectionPlane.n;
	collisionParams.projectionPlane.d = parameter.projectionPlane.d;
	collisionParams.timeStep = mSimulationTimeStep;
	collisionParams.invTimeStep = (mSimulationTimeStep > 0.0f) ? 1.0f / mSimulationTimeStep : 0.0f;

	collisionParams.restitution = CLAMP_RESTITUTION(parameter.restitution);
	collisionParams.dynamicFriction = CLAMP_DYNAMIC_FRICTION(parameter.dynamicFriction);
	collisionParams.staticFrictionSqr = parameter.staticFriction * parameter.staticFriction;
	collisionParams.temporalNoise = (parameter.noiseCounter * parameter.noiseCounter * 4999879) & 0xffff;
	collisionParams.flags = parameter.flags;
}

//----------------------------------------------------------------------------//

#endif // PX_USE_PARTICLE_SYSTEM_API
