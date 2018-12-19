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

#include "PxPhysXConfig.h"

#if PX_USE_PARTICLE_SYSTEM_API

#include "ScbParticleSystem.h"

using namespace physx;

//----------------------------------------------------------------------------//

// lazy allocate. doesn't reset hasUpdates
void Scb::ParticleSystem::ForceUpdates::initialize(PxU32 maxParticles)
{
	PX_ASSERT((map == NULL) == (values == NULL));
	if (values)
		return;

	values = reinterpret_cast<PxVec3*>(PX_ALLOC(maxParticles*sizeof(PxVec3), "PxVec3"));
	map = PX_NEW(Cm::BitMap)();
	map->resizeAndClear(maxParticles);
}

//----------------------------------------------------------------------------//

void Scb::ParticleSystem::ForceUpdates::destroy()
{
	PX_ASSERT((map == NULL) == (values == NULL));

	if (map)
	{
		PX_DELETE(map);
		map = NULL;
		PX_FREE(values);
		values = NULL;
	}
	hasUpdates = false;
}

//----------------------------------------------------------------------------//

Scb::ParticleSystem::ParticleSystem(const PxActorType::Enum& actorType, PxU32 maxParticles, bool perParticleRestOffset) :
	mParticleSystem			(actorType, maxParticles, perParticleRestOffset),
	mReadParticleFluidData	(NULL)
{
	setScbType(ScbType::ePARTICLE_SYSTEM);
}

//----------------------------------------------------------------------------//

Scb::ParticleSystem::~ParticleSystem()
{
	if (mReadParticleFluidData)
		PX_DELETE_AND_RESET(mReadParticleFluidData);
}

//----------------------------------------------------------------------------//

bool Scb::ParticleSystem::createParticles(const PxParticleCreationData& creationData)
{
	if (isBuffering())
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Particle operations are not allowed while simulation is running.");
		return false;
	}
	else
	{
		LOCK_PARTICLE_USER_BUFFERS("PxParticleBase::createParticles()")
		
		bool ret = mParticleSystem.createParticles(creationData);
		
		return ret;
	}
}

//----------------------------------------------------------------------------//

void Scb::ParticleSystem::releaseParticles(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer)
{
	LOCK_PARTICLE_USER_BUFFERS("PxParticleBase::releaseParticles()")

	if (numParticles == 0)
		return;

	if (isBuffering())
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Particle operations are not allowed while simulation is running.");
		return;
	}
	else
	{
		mParticleSystem.releaseParticles(numParticles, indexBuffer);
	}

	if (mForceUpdatesAcc.hasUpdates)
		for (PxU32 i=0; i < numParticles; i++)
			mForceUpdatesAcc.clear(indexBuffer[i]);

	if (mForceUpdatesVel.hasUpdates)
		for (PxU32 i=0; i < numParticles; i++)
			mForceUpdatesVel.clear(indexBuffer[i]);
}

//----------------------------------------------------------------------------//

void Scb::ParticleSystem::releaseParticles()
{
	LOCK_PARTICLE_USER_BUFFERS("PxParticleBase::releaseParticles()")

	if (isBuffering())
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Particle operations are not allowed while simulation is running.");
		return;
	}
	else
	{
		mParticleSystem.releaseParticles();
	}

	mForceUpdatesAcc.clear();
	mForceUpdatesVel.clear();
}

//----------------------------------------------------------------------------//

void Scb::ParticleSystem::setPositions(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer, 
											const PxStrideIterator<const PxVec3>& positionBuffer)
{
	LOCK_PARTICLE_USER_BUFFERS("PxParticleBase::setPositions()")

	if (isBuffering())
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Particle operations are not allowed while simulation is running.");
		return;
	}
	else
	{
		mParticleSystem.setPositions(numParticles, indexBuffer, positionBuffer);
	}
}

//----------------------------------------------------------------------------//

void Scb::ParticleSystem::setVelocities(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer, 
											 const PxStrideIterator<const PxVec3>& velocityBuffer)
{
	LOCK_PARTICLE_USER_BUFFERS("PxParticleBase::setVelocities()")

	if (isBuffering())
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Particle operations are not allowed while simulation is running.");
		return;
	}
	else
	{
		mParticleSystem.setVelocities(numParticles, indexBuffer, velocityBuffer);
	}
}

//----------------------------------------------------------------------------//

void Scb::ParticleSystem::setRestOffsets(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer, 
											 const PxStrideIterator<const PxF32>& restOffsetBuffer)
{
	LOCK_PARTICLE_USER_BUFFERS("PxParticleBase::setRestOffsets()")

	if (isBuffering())
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Particle operations are not allowed while simulation is running.");
		return;
	}
	else
	{
		mParticleSystem.setRestOffsets(numParticles, indexBuffer, restOffsetBuffer);
	}
}

//----------------------------------------------------------------------------//

void Scb::ParticleSystem::addForces(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer,
										 const PxStrideIterator<const PxVec3>& forceBuffer, PxForceMode::Enum forceMode)
{
	if (isBuffering())
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Particle operations are not allowed while simulation is running.");
		return;
	}

	PX_ASSERT(numParticles > 0);
	PX_ASSERT(indexBuffer.ptr() && indexBuffer.stride() > 0);

	PxReal particleMass = getParticleMass();
	bool isAcceleration = false;
	PxReal unitMult = 0.0;
	switch(forceMode)
	{
	case PxForceMode::eFORCE:				//!< parameter has unit of mass * distance/ time^2, i.e. a force
		unitMult = 1.0f / particleMass;
		isAcceleration = true;
		break;
	case PxForceMode::eIMPULSE:				//!< parameter has unit of mass * distance /time
		unitMult = 1.0f / particleMass;
		isAcceleration = false;
		break;
	case PxForceMode::eVELOCITY_CHANGE:		//!< parameter has unit of distance / time, i.e. the effect is mass independent: a velocity change.
		unitMult = 1.0f;
		isAcceleration = false;
		break;
	case PxForceMode::eACCELERATION:		//!< parameter has unit of distance/ time^2, i.e. an acceleration. It gets treated just like a force except the mass is not divided out before integration.
		unitMult = 1.0f;
		isAcceleration = true;
		break;
	}

	ForceUpdates& forceUpdates = isAcceleration ? mForceUpdatesAcc : mForceUpdatesVel;
	forceUpdates.initialize(mParticleSystem.getMaxParticles());

	for (PxU32 i=0; i < numParticles; i++)
		forceUpdates.add(indexBuffer[i], forceBuffer[i] * unitMult);
}

//----------------------------------------------------------------------------//

void Scb::ParticleSystem::submitForceUpdates(PxReal timeStep)
{
	LOCK_PARTICLE_USER_BUFFERS("PxParticleBase: Apply forces")
		
	if (mForceUpdatesAcc.hasUpdates)
	{
		mParticleSystem.addDeltaVelocities(*mForceUpdatesAcc.map, mForceUpdatesAcc.values, timeStep);
		mForceUpdatesAcc.clear();
	}

	if (mForceUpdatesVel.hasUpdates)
	{
		mParticleSystem.addDeltaVelocities(*mForceUpdatesVel.map, mForceUpdatesVel.values, 1.0f);
		mForceUpdatesVel.clear();
	}
}

//----------------------------------------------------------------------------//

void Scb::ParticleSystem::syncState()
{
	LOCK_PARTICLE_USER_BUFFERS("PxScene::fetchResults()")

	const PxU32 flags = getBufferFlags();
	if(flags)  // Optimization to avoid all the if-statements below if possible
	{
		const Buf& buffer = *getParticleSystemBuffer();

		flush<Buf::BF_Stiffness>(buffer);
		flush<Buf::BF_Viscosity>(buffer);
		flush<Buf::BF_Damping>(buffer);
		flush<Buf::BF_ExternalAcceleration>(buffer);
		flush<Buf::BF_ProjectionPlane>(buffer);
		flush<Buf::BF_ParticleMass>(buffer);
		flush<Buf::BF_Restitution>(buffer);
		flush<Buf::BF_DynamicFriction>(buffer);
		flush<Buf::BF_StaticFriction>(buffer);

		if(flags & Buf::BF_ResetFiltering)
			mParticleSystem.resetFiltering();

		flush<Buf::BF_SimulationFilterData>(buffer);
		flush<Buf::BF_Flags>(buffer);

		Actor::syncState();
	}

	postSyncState();
}

//----------------------------------------------------------------------------//

#endif // PX_USE_PARTICLE_SYSTEM_API
