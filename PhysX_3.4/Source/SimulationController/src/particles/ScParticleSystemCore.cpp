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


#include "ScParticleSystemCore.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "ScParticleSystemSim.h"
#include "ScPhysics.h"

#include "PsBitUtils.h"
#include "PsMathUtils.h"

#include "PxParticleReadData.h"
#include "PtParticleData.h"

using namespace physx;

//----------------------------------------------------------------------------//

#if PX_SUPPORT_GPU_PHYSX
namespace physx
{
	struct PxCudaReadWriteParticleBuffers;
}
#endif

//----------------------------------------------------------------------------//

PxU32 computePacketSizeMultLog2(PxReal packetSize, PxReal cellSize)
{
	PX_FPU_GUARD; // unknown osx issue with ceil function (throwing fp exception), printf makes it go away.
	const PxU32 mult = PxU32(Ps::ceil(packetSize / cellSize));
	PxU32 multPow2 = Ps::nextPowerOfTwo(mult-1);
	multPow2 = PxMax(PxU32(4), multPow2);
	return Ps::ilog2(multPow2);
}

//----------------------------------------------------------------------------//

Sc::ParticleSystemCore::ParticleSystemCore(const PxActorType::Enum& actorType, PxU32 maxParticles, bool perParticleRestOffset) : 
	ActorCore(actorType, PxActorFlag::eVISUALIZATION, PX_DEFAULT_CLIENT, 0, 0),
	mStandaloneData(0),
	mParticleMass(0.001f)
{	
	// set simulation parameters
	mSimulationFilterData.setToDefault();
	setExternalAcceleration(PxVec3(0));

	mLLParameter.particleReadDataFlags = PxParticleReadDataFlag::ePOSITION_BUFFER | PxParticleReadDataFlag::eFLAGS_BUFFER;//desc.particleReadDataFlags;
	mLLParameter.flags = PxParticleBaseFlag::eENABLED | 
						 PxParticleBaseFlag::eCOLLISION_WITH_DYNAMIC_ACTORS | 
						 PxParticleBaseFlag::ePER_PARTICLE_COLLISION_CACHE_HINT;
	if(perParticleRestOffset) 
		mLLParameter.flags |= PxParticleBaseFlag::ePER_PARTICLE_REST_OFFSET;

	bool isFluid = actorType != PxActorType::ePARTICLE_SYSTEM;
	
	if (isFluid)
	{
		PX_ASSERT(actorType == PxActorType::ePARTICLE_FLUID);
			
		mLLParameter.flags |= Pt::InternalParticleSystemFlag::eSPH;
		mLLParameter.restParticleDistance = 0.02f;
		mLLParameter.kernelRadiusMultiplier = SPH_KERNEL_RADIUS_MULT;
		mLLParameter.packetSizeMultiplierLog2 = computePacketSizeMultLog2(0.6f/*gridSize*/, mLLParameter.restParticleDistance*SPH_KERNEL_RADIUS_MULT);
		mLLParameter.stiffness = 20.0f;
		mLLParameter.viscosity = 6.0f;
		mLLParameter.restDensity = SPH_REST_DENSITY;
	}
	else
	{
		mLLParameter.restParticleDistance = 0.06f;
		mLLParameter.kernelRadiusMultiplier = 1.0f;
		mLLParameter.packetSizeMultiplierLog2 = computePacketSizeMultLog2(0.6f/*gridSize*/, mLLParameter.restParticleDistance);
		mLLParameter.stiffness = 0.0f;
		mLLParameter.viscosity = 0.0f;
		mLLParameter.restDensity = 0.0f;
	}

	mLLParameter.maxMotionDistance = 0.06f;
	mLLParameter.restOffset = 0.004f;
	mLLParameter.contactOffset = 0.008f;
	mLLParameter.damping = 0.0f;
	mLLParameter.noiseCounter = 0;

	// Simon: This is a bit hacky. We should have better support for non sph in LL 
	mLLParameter.restitution = 0.5f;
	mLLParameter.dynamicFriction = 0.05f;
	mLLParameter.staticFriction = 0.0f;

	mLLParameter.projectionPlane = PxPlane(PxVec3(0.0f, 0.0f, 1.0f), 0.0f);	

	// Create the low level standalone core
	maxParticles = PxMin(maxParticles, MAX_PARTICLES_PER_PARTICLE_SYSTEM);
	mStandaloneData = Pt::ParticleData::create(maxParticles, perParticleRestOffset);
	if(perParticleRestOffset)
	{
		PxF32* restOffsetBuf = mStandaloneData->getRestOffsetBuffer();
		for(PxU32 i = 0; i < maxParticles; i++)
			*restOffsetBuf++ = 0.f;
	}
}

//----------------------------------------------------------------------------//

Sc::ParticleSystemCore::~ParticleSystemCore()
{
	PX_ASSERT(getSim() == NULL);

	if (mStandaloneData)
		mStandaloneData->release();
}

//----------------------------------------------------------------------------//

// PX_SERIALIZATION
void Sc::ParticleSystemCore::exportExtraData(PxSerializationContext& stream)
{
	if (mStandaloneData)
	{
		mStandaloneData->exportData(stream);
	}
	else
	{
		//sschirm, this could be made faster and more memory friendly for non-gpu fluids. do we care?
		//if so, we would need to push this functionality into lowlevel and write platform dependent code.
		//for faster gpu export, we would need to push the export into the gpu dll. 
		Pt::ParticleSystemStateDataDesc particles;
		getParticleState().getParticlesV(particles, true, false);
		Pt::ParticleData* tmp = Pt::ParticleData::create(particles, getParticleState().getWorldBoundsV());
		tmp->exportData(stream);
		tmp->release();
	}
}

void Sc::ParticleSystemCore::importExtraData(PxDeserializationContext& context)
{
	PX_ASSERT(!getSim());
	mStandaloneData = Pt::ParticleData::create(context);
}
//~PX_SERIALIZATION

//----------------------------------------------------------------------------//

Sc::ParticleSystemSim* Sc::ParticleSystemCore::getSim() const
{
	return static_cast<ParticleSystemSim*>(Sc::ActorCore::getSim());
}

//----------------------------------------------------------------------------//

Pt::ParticleSystemState&	Sc::ParticleSystemCore::getParticleState()
{ 
	return getSim() ? getSim()->getParticleState() : *mStandaloneData; 
}

//----------------------------------------------------------------------------//

const Pt::ParticleSystemState& Sc::ParticleSystemCore::getParticleState() const 
{ 
	return getSim() ? getSim()->getParticleState() : *mStandaloneData; 
}

//----------------------------------------------------------------------------//

Pt::ParticleData* Sc::ParticleSystemCore::obtainStandaloneData()
{
	PX_ASSERT(mStandaloneData);
	Pt::ParticleData* tmp = mStandaloneData; 
	mStandaloneData = NULL;
	return tmp;
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::returnStandaloneData(Pt::ParticleData* particleData)
{
	PX_ASSERT(particleData && !mStandaloneData);
	mStandaloneData = particleData;
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::onOriginShift(const PxVec3& shift)
{
	// hard to come up with a use case for this but we do it for the sake of consistency
	PxReal delta = mLLParameter.projectionPlane.n.dot(shift);
	mLLParameter.projectionPlane.d += delta;

	PX_ASSERT(mStandaloneData);
	mStandaloneData->onOriginShift(shift);
}

//----------------------------------------------------------------------------//

PxParticleBase* Sc::ParticleSystemCore::getPxParticleBase()
{
	if (getActorCoreType() == PxActorType::ePARTICLE_SYSTEM)
		return gOffsetTable.convertScParticleSystem2Px(this);
	else
		return gOffsetTable.convertScParticleSystem2PxParticleFluid(this);
}

//----------------------------------------------------------------------------//

PxReal Sc::ParticleSystemCore::getStiffness() const
{
	return mLLParameter.stiffness;
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::setStiffness(PxReal t)
{
	mLLParameter.stiffness = t;
}

//----------------------------------------------------------------------------//

PxReal Sc::ParticleSystemCore::getViscosity() const
{
	return mLLParameter.viscosity;
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::setViscosity(PxReal t)
{
	mLLParameter.viscosity = t;
}

//----------------------------------------------------------------------------//

PxReal Sc::ParticleSystemCore::getDamping() const
{
	return mLLParameter.damping;
}

//----------------------------------------------------------------------------//

void  Sc::ParticleSystemCore::setDamping(PxReal t)
{
	mLLParameter.damping = t;
}

//----------------------------------------------------------------------------//

PxReal Sc::ParticleSystemCore::getParticleMass() const
{
	return mParticleMass;
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::setParticleMass(PxReal mass)
{
	mParticleMass = mass;
}

//----------------------------------------------------------------------------//

PxReal Sc::ParticleSystemCore::getRestitution() const
{
	return mLLParameter.restitution;
}

//----------------------------------------------------------------------------//

void  Sc::ParticleSystemCore::setRestitution(PxReal t)
{
	mLLParameter.restitution = t;
}

//----------------------------------------------------------------------------//

PxReal Sc::ParticleSystemCore::getDynamicFriction() const			
{ 
	return mLLParameter.dynamicFriction;
}

//----------------------------------------------------------------------------//

void  Sc::ParticleSystemCore::setDynamicFriction(PxReal t)			
{
	mLLParameter.dynamicFriction = t;
}

//----------------------------------------------------------------------------//

PxReal Sc::ParticleSystemCore::getStaticFriction() const			
{ 
	return mLLParameter.staticFriction;
}

//----------------------------------------------------------------------------//

void  Sc::ParticleSystemCore::setStaticFriction(PxReal t)			
{ 	
	mLLParameter.staticFriction = t;
}

//----------------------------------------------------------------------------//

const PxFilterData& Sc::ParticleSystemCore::getSimulationFilterData() const
{
	return mSimulationFilterData;
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::setSimulationFilterData(const PxFilterData& data)
{
	mSimulationFilterData = data;

	if (getSim())
		getSim()->scheduleRefiltering();
	// If no sim object then we have no particle packets anyway and nothing to worry about
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::resetFiltering()
{
	if (getSim())
		getSim()->resetFiltering();
	// If no sim object then we have no particle packets anyway and nothing to worry about
}

//----------------------------------------------------------------------------//

PxParticleBaseFlags Sc::ParticleSystemCore::getFlags() const
{
	return PxParticleBaseFlags(PxU16(mLLParameter.flags));	
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::setFlags(PxParticleBaseFlags flags)
{
	if (getSim())
	{
		// flags that are immutable while in scene
		PxParticleBaseFlags sceneImmutableFlags = 
			PxParticleBaseFlag::eGPU | 
			PxParticleBaseFlag::eCOLLISION_TWOWAY | 
			PxParticleBaseFlag::eCOLLISION_WITH_DYNAMIC_ACTORS | 
			PxParticleBaseFlag::ePER_PARTICLE_COLLISION_CACHE_HINT;

		if (flags & sceneImmutableFlags)
		{
			Sc::Scene* scene = &getSim()->getScene();
			scene->removeParticleSystem(*this, false);
			setInternalFlags(flags);
			scene->addParticleSystem(*this);
		}
		else
		{
			setInternalFlags(flags);
		}
		getSim()->setFlags(flags);
	}
	else
	{
		setInternalFlags(flags);
	}
}

//----------------------------------------------------------------------------//

PxU32 Sc::ParticleSystemCore::getInternalFlags() const
{
	return mLLParameter.flags;
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::setInternalFlags(PxParticleBaseFlags flags)
{
	mLLParameter.flags = (mLLParameter.flags & 0xffff0000) | PxU32(flags);
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::notifyCpuFallback()
{
	setInternalFlags(getFlags() & ~PxParticleBaseFlag::eGPU);
}

//----------------------------------------------------------------------------//

PxParticleReadDataFlags Sc::ParticleSystemCore::getParticleReadDataFlags() const
{
	return mLLParameter.particleReadDataFlags;
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::setParticleReadDataFlags(PxParticleReadDataFlags flags) 
{
	mLLParameter.particleReadDataFlags = flags;
}

//----------------------------------------------------------------------------//

PxU32 Sc::ParticleSystemCore::getMaxParticles() const
{ 	
	return getParticleState().getMaxParticlesV();
}

//----------------------------------------------------------------------------//

PxReal Sc::ParticleSystemCore::getMaxMotionDistance()	const
{
    return mLLParameter.maxMotionDistance;
}

//---------------------------------------------------------------------------//
void Sc::ParticleSystemCore::setMaxMotionDistance(PxReal distance)
{
	PX_ASSERT(!getSim());		
	mLLParameter.maxMotionDistance = distance;
}

//----------------------------------------------------------------------------//

PxReal Sc::ParticleSystemCore::getRestOffset() const
{
	return mLLParameter.restOffset;
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::setRestOffset(PxReal restOffset)
{
	PX_ASSERT(!getSim());		
	mLLParameter.restOffset = restOffset;
}

//----------------------------------------------------------------------------//

PxReal Sc::ParticleSystemCore::getContactOffset() const
{
	return mLLParameter.contactOffset;
}

//---------------------------------------------------------------------------//
void Sc::ParticleSystemCore::setContactOffset(PxReal contactOffset)
{
	PX_ASSERT(!getSim());	
	mLLParameter.contactOffset = contactOffset;
}

//----------------------------------------------------------------------------//

PxReal Sc::ParticleSystemCore::getRestParticleDistance() const				
{ 	
	return mLLParameter.restParticleDistance;
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::setRestParticleDistance(PxReal restParticleDistance)
{
	PX_ASSERT(!getSim());	
	PxReal gridSize = getGridSize();
	mLLParameter.restParticleDistance = restParticleDistance;
	setGridSize(gridSize);
}

//----------------------------------------------------------------------------//

PxReal Sc::ParticleSystemCore::getGridSize() const
{
	const PxReal cellSize = mLLParameter.kernelRadiusMultiplier * mLLParameter.restParticleDistance;
	const PxU32 packetMult =	PxU32(1 << mLLParameter.packetSizeMultiplierLog2);
	return packetMult * cellSize;
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::setGridSize(PxReal gridSize)
{
	PX_ASSERT(!getSim());	
	const PxReal cellSize = mLLParameter.kernelRadiusMultiplier * mLLParameter.restParticleDistance;
	mLLParameter.packetSizeMultiplierLog2 = computePacketSizeMultLog2(gridSize, cellSize);	
}

//----------------------------------------------------------------------------//

/**
Create new particles in the LL and the user particle data.
*/
bool Sc::ParticleSystemCore::createParticles(const PxParticleCreationData& creationData)
{
	PX_ASSERT(creationData.numParticles > 0);
	PX_ASSERT(creationData.numParticles <= (getParticleState().getMaxParticlesV() - getParticleCount()));

	// add particles to lowlevel
	return getParticleState().addParticlesV(creationData);
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::releaseParticles(PxU32 nbIndices, const PxStrideIterator<const PxU32>& indices)
{
	PX_ASSERT(indices.ptr());
	getParticleState().removeParticlesV(nbIndices, indices);
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::releaseParticles()
{
	getParticleState().removeParticlesV();
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::setPositions(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer, const PxStrideIterator<const PxVec3>& positionBuffer)
{
	PX_ASSERT(numParticles > 0 && indexBuffer.ptr() && positionBuffer.ptr());
	getParticleState().setPositionsV(numParticles, indexBuffer, positionBuffer);
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::setVelocities(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer, const PxStrideIterator<const PxVec3>& velocityBuffer)
{
	PX_ASSERT(numParticles > 0 && indexBuffer.ptr() && velocityBuffer.ptr());
	getParticleState().setVelocitiesV(numParticles, indexBuffer, velocityBuffer);
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::setRestOffsets(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer, const PxStrideIterator<const PxF32>& restOffsetBuffer)
{
	PX_ASSERT(numParticles > 0 && indexBuffer.ptr() && restOffsetBuffer.ptr());
	getParticleState().setRestOffsetsV(numParticles, indexBuffer, restOffsetBuffer);
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::addDeltaVelocities(const Cm::BitMap& bufferMap, const PxVec3* buffer, PxReal multiplier)
{
	getParticleState().addDeltaVelocitiesV(bufferMap, buffer, multiplier);
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::getParticleReadData(PxParticleFluidReadData& readData) const
{
	bool wantDeviceData = readData.getDataAccessFlags()&PxDataAccessFlag::eDEVICE;

	Pt::ParticleSystemStateDataDesc particles;
	getParticleState().getParticlesV(particles, false, wantDeviceData);
	PX_ASSERT(particles.maxParticles >= particles.numParticles);
	PX_ASSERT(particles.maxParticles >= particles.validParticleRange);

	readData.nbValidParticles = particles.numParticles;
	readData.validParticleRange = particles.validParticleRange;
	readData.validParticleBitmap = particles.bitMap ? particles.bitMap->getWords() : NULL;

	readData.positionBuffer =			PxStrideIterator<const PxVec3>();
	readData.velocityBuffer =			PxStrideIterator<const PxVec3>();
	readData.restOffsetBuffer =			PxStrideIterator<const PxF32>();
	readData.flagsBuffer =				PxStrideIterator<const PxParticleFlags>();
	readData.collisionNormalBuffer =	PxStrideIterator<const PxVec3>();
	readData.collisionVelocityBuffer =	PxStrideIterator<const PxVec3>();
	readData.densityBuffer =			PxStrideIterator<const PxReal>();

	if (readData.validParticleRange > 0)
	{
		if (mLLParameter.particleReadDataFlags & PxParticleReadDataFlag::ePOSITION_BUFFER)
			readData.positionBuffer = particles.positions;

		if (mLLParameter.particleReadDataFlags & PxParticleReadDataFlag::eVELOCITY_BUFFER)
			readData.velocityBuffer = particles.velocities;

		if (mLLParameter.particleReadDataFlags & PxParticleReadDataFlag::eREST_OFFSET_BUFFER)
			readData.restOffsetBuffer = particles.restOffsets;

		if (mLLParameter.particleReadDataFlags & PxParticleReadDataFlag::eFLAGS_BUFFER)
			readData.flagsBuffer = PxStrideIterator<const PxParticleFlags>(reinterpret_cast<const PxParticleFlags*>(particles.flags.ptr()), particles.flags.stride());

		Sc::ParticleSystemSim* sim = getSim();
		if (sim)
		{
			Pt::ParticleSystemSimDataDesc simParticleData;
			sim->getSimParticleData(simParticleData, wantDeviceData);

			if (mLLParameter.particleReadDataFlags & PxParticleReadDataFlag::eCOLLISION_NORMAL_BUFFER)
				readData.collisionNormalBuffer = simParticleData.collisionNormals;

			if (mLLParameter.particleReadDataFlags & PxParticleReadDataFlag::eCOLLISION_VELOCITY_BUFFER)
				readData.collisionVelocityBuffer = simParticleData.collisionVelocities;

			if (mLLParameter.particleReadDataFlags & PxParticleReadDataFlag::eDENSITY_BUFFER)
				readData.densityBuffer = simParticleData.densities;
		}
	}
}

//----------------------------------------------------------------------------//

const Cm::BitMap& Sc::ParticleSystemCore::getParticleMap() const
{
	Pt::ParticleSystemStateDataDesc particles;
	getParticleState().getParticlesV(particles, false, false);
	return *particles.bitMap;
}

//----------------------------------------------------------------------------//

PxU32 Sc::ParticleSystemCore::getParticleCount() const
{
	return getParticleState().getParticleCountV();
}

//----------------------------------------------------------------------------//

PxBounds3 Sc::ParticleSystemCore::getWorldBounds() const
{
	return getParticleState().getWorldBoundsV();
}

//----------------------------------------------------------------------------//

const PxVec3& Sc::ParticleSystemCore::getExternalAcceleration() const
{	
	return mExternalAcceleration;
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::setExternalAcceleration(const PxVec3& v)
{
	mExternalAcceleration = v;
}

//----------------------------------------------------------------------------//

const PxPlane& Sc::ParticleSystemCore::getProjectionPlane() const
{
	return mLLParameter.projectionPlane;
}

//----------------------------------------------------------------------------//

void Sc::ParticleSystemCore::setProjectionPlane(const PxPlane& plane)
{
	mLLParameter.projectionPlane = plane;
}

//----------------------------------------------------------------------------//

#if PX_SUPPORT_GPU_PHYSX

void Sc::ParticleSystemCore::enableDeviceExclusiveModeGpu()
{
	PX_ASSERT(getSim());
	getSim()->enableDeviceExclusiveModeGpu();
}

PxParticleDeviceExclusiveAccess* Sc::ParticleSystemCore::getDeviceExclusiveAccessGpu() const
{
	PX_ASSERT(getSim());
	return getSim()->getDeviceExclusiveAccessGpu();
}

bool Sc::ParticleSystemCore::isGpu() const
{
	return getSim() && getSim()->isGpu();
}
#endif

//----------------------------------------------------------------------------//

#endif	// PX_USE_PARTICLE_SYSTEM_API
