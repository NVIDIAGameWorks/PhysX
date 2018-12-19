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


#ifndef PX_PHYSICS_NP_PARTICLESYSTEM_TEMPLATE
#define PX_PHYSICS_NP_PARTICLESYSTEM_TEMPLATE

#include "PxPhysXConfig.h"

#if PX_USE_PARTICLE_SYSTEM_API

#include "NpActorTemplate.h"
#include "PxParticleSystem.h"
#include "ScbParticleSystem.h"
#include "NpWriteCheck.h"
#include "NpReadCheck.h"
#include "PtParticleData.h"
#include "NpScene.h"

#if PX_SUPPORT_GPU_PHYSX
#include "PxParticleDeviceExclusive.h"
#endif

namespace physx
{

template<class APIClass, class LeafClass>
class NpParticleBaseTemplate : public NpActorTemplate<APIClass>
{
public:
// PX_SERIALIZATION
										NpParticleBaseTemplate(PxBaseFlags baseFlags) : NpActorTemplate<APIClass>(baseFlags), mParticleSystem(PxEmpty)	{}
//~PX_SERIALIZATION
										NpParticleBaseTemplate<APIClass, LeafClass>(PxType, PxBaseFlags, const PxActorType::Enum&, PxU32, bool);
	virtual								~NpParticleBaseTemplate();

	//---------------------------------------------------------------------------------
	// PxParticleSystemActor implementation
	//---------------------------------------------------------------------------------
	virtual		void					release();

	virtual		PxActorType::Enum		getType() const { return PxActorType::ePARTICLE_SYSTEM; }

	virtual		bool					createParticles(const PxParticleCreationData& creationData);
	virtual		void					releaseParticles(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer);
	virtual		void					releaseParticles();
	virtual		void					setPositions(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer, 
													 const PxStrideIterator<const PxVec3>& positionBuffer);
	virtual		void					setVelocities(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer,
													  const PxStrideIterator<const PxVec3>& velocityBuffer);
	virtual		void					setRestOffsets(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer,
													  const PxStrideIterator<const PxF32>& restOffsetBuffer);
	virtual		void					addForces(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer,
												  const PxStrideIterator<const PxVec3>& forceBuffer, PxForceMode::Enum forceMode);
	virtual		PxParticleReadData* 	lockParticleReadData(PxDataAccessFlags flags);
	virtual		PxParticleReadData*		lockParticleReadData();

	virtual		PxReal					getDamping()											const;
	virtual		void 					setDamping(PxReal);
	virtual		PxVec3					getExternalAcceleration()								const;
	virtual		void 					setExternalAcceleration(const PxVec3&);

	virtual		PxReal					getParticleMass()										const;
	virtual		void 					setParticleMass(PxReal);
	virtual		PxReal					getRestitution()										const;
	virtual		void 					setRestitution(PxReal);
	virtual		PxReal					getDynamicFriction()									const;
	virtual		void 					setDynamicFriction(PxReal);
	virtual		PxReal					getStaticFriction()										const;
	virtual		void 					setStaticFriction(PxReal);

	virtual		void					setSimulationFilterData(const PxFilterData& data);
	virtual		PxFilterData			getSimulationFilterData()								const;
	virtual		void					resetFiltering();

	virtual		void					setParticleBaseFlag(PxParticleBaseFlag::Enum flag, bool value);
	virtual		PxParticleBaseFlags		getParticleBaseFlags()									const;

	virtual		PxParticleReadDataFlags	getParticleReadDataFlags()								const;
	virtual		void					setParticleReadDataFlag(PxParticleReadDataFlag::Enum, bool);

	virtual		PxU32 					getMaxParticles()										const;

	virtual		PxReal					getMaxMotionDistance()									const;
	virtual		void					setMaxMotionDistance(PxReal);
	virtual		PxReal					getRestOffset()											const;
	virtual		void					setRestOffset(PxReal);
	virtual		PxReal					getContactOffset()										const;
	virtual     void                    setContactOffset(PxReal); 
	virtual		PxReal					getGridSize()											const;
	virtual		void					setGridSize(PxReal);	

	virtual		void					getProjectionPlane(PxVec3& normal, PxReal& d)			const;
	virtual		void 					setProjectionPlane(const PxVec3& normal, PxReal d);

	virtual		PxBounds3				getWorldBounds(float inflation=1.01f)					const;

	typedef NpActorTemplate<APIClass> ActorTemplateClass;

	//---------------------------------------------------------------------------------
	// Miscellaneous
	//---------------------------------------------------------------------------------
	PX_INLINE	Scb::Actor&					getScbActor()			const	{ return const_cast<Scb::ParticleSystem&>(mParticleSystem); }

	PX_INLINE	const Scb::ParticleSystem&	getScbParticleSystem() const	{ return mParticleSystem;				}
	PX_INLINE	Scb::ParticleSystem&		getScbParticleSystem()			{ return mParticleSystem;				}
	PX_INLINE	NpScene* getNpScene() const									{ return NpActor::getAPIScene(*this);	}

#if PX_SUPPORT_GPU_PHYSX
	// Helper for reporting error conditions.
	class PxParticleDeviceExclusiveAccess* getDeviceExclusiveAccessGpu(const char* callerName) const;

	void		enableDeviceExclusiveModeGpu();
	bool		isDeviceExclusiveModeEnabledGpu() const;
	void		getReadWriteCudaBuffersGpu(struct PxCudaReadWriteParticleBuffers& buffers) const;
	void		setValidParticleRangeGpu(PxU32 validParticleRange);
	void		setDeviceExclusiveModeFlagsGpu(PxU32 flags);
	PxBaseTask*	getLaunchTaskGpu() const;
	void		addLaunchTaskDependentGpu(class physx::PxBaseTask& dependent);
	size_t		getCudaStreamGpu() const;
#endif

private:
	bool		checkAllocatedIndices(PxU32 numIndices, const PxStrideIterator<const PxU32>& indices);
	bool		checkFreeIndices(PxU32 numIndices, const PxStrideIterator<const PxU32>& indices);

//private:
public:	// PT: sorry, needs to be accessible to serialization macro
				Scb::ParticleSystem	mParticleSystem;
};


template<class APIClass, class LeafClass>
NpParticleBaseTemplate<APIClass, LeafClass>::NpParticleBaseTemplate(PxType concreteType, PxBaseFlags baseFlags, const PxActorType::Enum& actorType, PxU32 maxParticles, bool perParticleRestOffset)
: ActorTemplateClass(concreteType, baseFlags, NULL, NULL)
, mParticleSystem(actorType, maxParticles, perParticleRestOffset)
{
	// don't ref Scb actor here, it hasn't been assigned yet
}


template<class APIClass, class LeafClass>
NpParticleBaseTemplate<APIClass, LeafClass>::~NpParticleBaseTemplate()
{
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::release()
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	NpPhysics::getInstance().notifyDeletionListenersUserRelease(this, this->userData);

// PX_AGGREGATE
	ActorTemplateClass::release();	// PT: added for PxAggregate
//~PX_AGGREGATE

	NpScene* npScene = NpActor::getAPIScene(*this);
	if (npScene)
	{
		npScene->removeFromParticleBaseList(*this);
		npScene->getScene().removeParticleSystem(static_cast<LeafClass&>(*this).getScbParticleSystem(), true);
	}
	mParticleSystem.destroy();
}

#if PX_CHECKED
namespace
{
	bool checkUniqueIndices(PxU32 numIndices, const PxStrideIterator<const PxU32>& indices)
	{
		Cm::BitMap bitmap;
		PxU32 i = 0;
		for (; i != numIndices; ++i)
		{
			PxU32 index = indices[i];
			if (bitmap.boundedTest(index))
				break;
			bitmap.growAndSet(index);
		}

		if (i != numIndices)
		{
			Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, 
				"PxParticleBase::releaseParticles(): indexBuffer contains duplicate particle index %d.", indices[i]);
			return false;
		}
		return true;
	}
	
	bool checkVectors(PxU32 numVectors, const PxStrideIterator<const PxVec3>& vectors)
	{
		for (PxU32 i = 0; i < numVectors; ++i)
		{
			if(!vectors[i].isFinite())
				return false;
		}
		return true;
	}

	bool checkRestOffsets(PxU32 numParticles, const PxStrideIterator<const PxF32>& restOffsetBuffer, PxF32 restOffset)
	{
		for (PxU32 i = 0; i < numParticles; ++i)
		{
			if (restOffsetBuffer[i] < 0.0f || restOffsetBuffer[i] > restOffset)
				return false;
		}
		return true;
	}

	bool checkRestOffset(Pt::ParticleData& standaloneData, PxF32 restOffset)
	{
		const PxF32* restOffsetBuffer = standaloneData.getRestOffsetBuffer();
		PxU32 numParticles = standaloneData.getMaxParticles();
		for (PxU32 i = 0; i < numParticles; ++i)
		{
			if (restOffsetBuffer[i] > restOffset)
				return false;
		}
		return true;
	}
}
#endif // PX_CHECKED

//----------------------------------------------------------------------------//

template<class APIClass, class LeafClass>
bool NpParticleBaseTemplate<APIClass, LeafClass>::checkAllocatedIndices(PxU32 numIndices, const PxStrideIterator<const PxU32>& indices)
{
	const Cm::BitMap& map = mParticleSystem.getParticleMap(); 
	PxU32 maxParticles = getMaxParticles();

	for (PxU32 i = 0; i < numIndices; ++i)
	{
		PxU32 index = indices[i];

		if (index >= maxParticles)
		{
			Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, 
				"Operation on particle index %d not in valid range [0, %d]", index, getMaxParticles() - 1);
			return false;
		}

		if (!map.boundedTest(index))
		{
			Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, 
				"Operation on invalid particle with index %d", index);
			return false;
		}
	}
	return true;
}

//----------------------------------------------------------------------------//

template<class APIClass, class LeafClass>
bool NpParticleBaseTemplate<APIClass, LeafClass>::checkFreeIndices(PxU32 numIndices, const PxStrideIterator<const PxU32>& indices)
{
	const Cm::BitMap& map = mParticleSystem.getParticleMap(); 
	PxU32 maxParticles = getMaxParticles();

	for (PxU32 i = 0; i < numIndices; ++i)
	{
		PxU32 index = indices[i];
		
		if (index >= maxParticles)
		{
			Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, 
				"Provided particle index %d is not in valid range [0, %d]", index, getMaxParticles() - 1);
			return false;
		}
		
		if (map.boundedTest(index))
		{
			Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, 
				"Provided particle index %d is already in use", index);
			return false;
		}
	}
	return true;
}

//----------------------------------------------------------------------------//

template<class APIClass, class LeafClass>
bool NpParticleBaseTemplate<APIClass, LeafClass>::createParticles(const PxParticleCreationData& creationData)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PX_CHECK_AND_RETURN_NULL(creationData.isValid(), "PxParticleCreationData submitted with PxParticleBase::createParticles() invalid.");
	if (creationData.numParticles == 0)
		return true;
	
	bool supportsPerParticleRestOffset = getScbParticleSystem().getFlags() & PxParticleBaseFlag::ePER_PARTICLE_REST_OFFSET;
	bool perParticleRestOffset = creationData.restOffsetBuffer.ptr() != NULL;

	PX_UNUSED(supportsPerParticleRestOffset);
	PX_UNUSED(perParticleRestOffset);

	PX_CHECK_AND_RETURN_NULL(!perParticleRestOffset || supportsPerParticleRestOffset, "PxParticleCreationData.restOffsetBuffer set but PxParticleBaseFlag::ePER_PARTICLE_REST_OFFSET not set.")
	PX_CHECK_AND_RETURN_NULL(!supportsPerParticleRestOffset || perParticleRestOffset, "PxParticleBaseFlag::ePER_PARTICLE_REST_OFFSET set, but PxParticleCreationData.restOffsetBuffer not set.")

#if PX_CHECKED
	PX_CHECK_AND_RETURN_NULL(checkFreeIndices(creationData.numParticles, creationData.indexBuffer), "PxParticleBase::createParticles: failed");
	PX_CHECK_AND_RETURN_NULL(checkUniqueIndices(creationData.numParticles, creationData.indexBuffer), "PxParticleBase::createParticles: failed");

	if (perParticleRestOffset)
		PX_CHECK_AND_RETURN_NULL(checkRestOffsets(creationData.numParticles, creationData.restOffsetBuffer, getScbParticleSystem().getRestOffset()), 
			"PxParticleCreationData.restOffsetBuffer: offsets must be in range [0.0f, restOffset].");

	PX_CHECK_AND_RETURN_NULL(checkVectors(creationData.numParticles, creationData.positionBuffer), "PxParticleBase::createParticles: position vectors shoule be finite");
	
	bool perParticleVelocity = creationData.velocityBuffer.ptr() != NULL;
	if(perParticleVelocity)
	{
		PX_CHECK_AND_RETURN_NULL(checkVectors(creationData.numParticles, creationData.velocityBuffer), "PxParticleBase::createParticles: velocity vectors shoule be finite");
	}
#endif

	return getScbParticleSystem().createParticles(creationData);
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::releaseParticles(PxU32 numParticles, const PxStrideIterator<const PxU32>& indices)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	if (numParticles == 0)
		return;

#if PX_CHECKED
	PX_CHECK_AND_RETURN(indices.ptr() && indices.stride() != 0, "PxParticleBase::releaseParticles: invalid index buffer");	
	PX_CHECK_AND_RETURN(checkAllocatedIndices(numParticles, indices), "PxParticleBase::releaseParticles: failed");
	PX_CHECK_AND_RETURN(checkUniqueIndices(numParticles, indices), "PxParticleBase::releaseParticles: failed");
#endif

	getScbParticleSystem().releaseParticles(numParticles, indices);
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::releaseParticles()
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	getScbParticleSystem().releaseParticles();
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::setPositions(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer,
																 const PxStrideIterator<const PxVec3>& positionBuffer)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	if (numParticles == 0)
		return;

	PX_CHECK_AND_RETURN(indexBuffer.ptr() && indexBuffer.stride() != 0, "PxParticleBase::setPositions: invalid index buffer");
	PX_CHECK_AND_RETURN(positionBuffer.ptr(), "PxParticleBase::setPositions: invalid position buffer");

#if PX_CHECKED
	PX_CHECK_AND_RETURN(checkAllocatedIndices(numParticles, indexBuffer), "PxParticleBase::setPositions: failed");
	PX_CHECK_AND_RETURN(checkVectors(numParticles, positionBuffer), "PxParticleBase::setPositions: position vectors shoule be finite");
#endif

	getScbParticleSystem().setPositions(numParticles, indexBuffer, positionBuffer);
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::setVelocities(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer,
																  const PxStrideIterator<const PxVec3>& velocityBuffer)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	if (numParticles == 0)
		return;

	PX_CHECK_AND_RETURN(indexBuffer.ptr() && indexBuffer.stride() != 0, "PxParticleBase::setVelocities: invalid index buffer");
	PX_CHECK_AND_RETURN(velocityBuffer.ptr(), "PxParticleBase::setVelocities: invalid velocity buffer");

#if PX_CHECKED
	PX_CHECK_AND_RETURN(checkAllocatedIndices(numParticles, indexBuffer), "PxParticleBase::setVelocities: failed");
	PX_CHECK_AND_RETURN(checkVectors(numParticles, velocityBuffer), "PxParticleBase::setVelocities: velocity vectors shoule be finite");
#endif
	getScbParticleSystem().setVelocities(numParticles, indexBuffer, velocityBuffer);
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::setRestOffsets(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer,
																   const PxStrideIterator<const PxF32>& restOffsetBuffer)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	if (numParticles == 0)
		return;

	PX_CHECK_AND_RETURN(indexBuffer.ptr() && indexBuffer.stride() != 0, "PxParticleBase::setRestOffsets: invalid index buffer");
	PX_CHECK_AND_RETURN(restOffsetBuffer.ptr(), "PxParticleBase::restOffsets: invalid restOffset buffer");

	PX_CHECK_AND_RETURN(getScbParticleSystem().getFlags() & PxParticleBaseFlag::ePER_PARTICLE_REST_OFFSET, "PxParticleBase::setRestOffsets: PxParticleBaseFlag::ePER_PARTICLE_REST_OFFSET not set");

#if PX_CHECKED
	PX_CHECK_AND_RETURN(checkAllocatedIndices(numParticles, indexBuffer), "PxParticleBase::setRestOffsets: failed");
	PX_CHECK_AND_RETURN(checkRestOffsets(numParticles, restOffsetBuffer, getScbParticleSystem().getRestOffset()), 
		"PxParticleBase::restOffsets: offsets must be in range [0.0f, restOffset].");
#endif
	getScbParticleSystem().setRestOffsets(numParticles, indexBuffer, restOffsetBuffer);
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::addForces(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer,
															  const PxStrideIterator<const PxVec3>& forceBuffer, PxForceMode::Enum forceMode)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	if (!getNpScene())
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Attempt to add forces on particle system which isn't assigned to any scene.");
		return;
	}

	if (numParticles == 0)
		return;

	PX_CHECK_AND_RETURN(indexBuffer.ptr() && indexBuffer.stride() != 0, "PxParticleBase::addForces: invalid index buffer");
	PX_CHECK_AND_RETURN(forceBuffer.ptr(), "PxParticleBase::addForces: invalid force buffer");

#if PX_CHECKED
	PX_CHECK_AND_RETURN(checkAllocatedIndices(numParticles, indexBuffer), "PxParticleBase::addForces: failed");
	PX_CHECK_AND_RETURN(checkVectors(numParticles, forceBuffer), "PxParticleBase::addForces: force vectors shoule be finite");
#endif

	getScbParticleSystem().addForces(numParticles, indexBuffer, forceBuffer, forceMode);
}

template<class APIClass, class LeafClass>
PxParticleReadData* NpParticleBaseTemplate<APIClass, LeafClass>::lockParticleReadData(PxDataAccessFlags flags)
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return getScbParticleSystem().lockParticleReadData(flags);
}

template<class APIClass, class LeafClass>
PxParticleReadData* NpParticleBaseTemplate<APIClass, LeafClass>::lockParticleReadData()
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return getScbParticleSystem().lockParticleReadData(PxDataAccessFlag::eREADABLE);
}


template<class APIClass, class LeafClass>
PxReal NpParticleBaseTemplate<APIClass, LeafClass>::getDamping() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return getScbParticleSystem().getDamping();
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::setDamping(PxReal d)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));		
	PX_CHECK_AND_RETURN(d >= 0.0f,"Damping is not allowed to be negative, PxParticleBase::setDamping() ignored.");
	getScbParticleSystem().setDamping(d);
}


template<class APIClass, class LeafClass>
PxVec3 NpParticleBaseTemplate<APIClass, LeafClass>::getExternalAcceleration() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return getScbParticleSystem().getExternalAcceleration();
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::setExternalAcceleration(const PxVec3& a)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));		
	getScbParticleSystem().setExternalAcceleration(a);
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::getProjectionPlane(PxVec3& normal, PxReal& d) const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	PxPlane p = getScbParticleSystem().getProjectionPlane();
	normal = p.n;
	d = p.d;
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::setProjectionPlane(const PxVec3& normal, PxReal d)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));		
	PX_CHECK_AND_RETURN(!normal.isZero(),"Plane normal needs to be non zero, PxParticleBase::setProjectionPlane() ignored.");

	PxPlane p(normal, d);
	getScbParticleSystem().setProjectionPlane(p);
}


template<class APIClass, class LeafClass>
PxReal NpParticleBaseTemplate<APIClass, LeafClass>::getParticleMass() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return getScbParticleSystem().getParticleMass();
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::setParticleMass(PxReal m)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));		
	PX_CHECK_AND_RETURN(m > 0.0f, "ParticleMass needs to be positive, PxParticleBase::setParticleMass() ignored.");
	getScbParticleSystem().setParticleMass(m);
}


template<class APIClass, class LeafClass>
PxReal NpParticleBaseTemplate<APIClass, LeafClass>::getRestitution() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return getScbParticleSystem().getRestitution();
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::setRestitution(PxReal r)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));		
	PX_CHECK_AND_RETURN(r >= 0.0f && r <= 1.0f,"Restitution needs to be in [0,1], PxParticleBase::setRestitution() ignored.");
	getScbParticleSystem().setRestitution(r);
}


template<class APIClass, class LeafClass>
PxReal NpParticleBaseTemplate<APIClass, LeafClass>::getDynamicFriction() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return getScbParticleSystem().getDynamicFriction();
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::setDynamicFriction(PxReal a)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));		
	PX_CHECK_AND_RETURN(a >= 0.0f && a <= 1.0f,"Dynamic Friction needs to be in [0,1], PxParticleBase::setDynamicFriction() ignored.");
	getScbParticleSystem().setDynamicFriction(a);
}


template<class APIClass, class LeafClass>
PxReal NpParticleBaseTemplate<APIClass, LeafClass>::getStaticFriction() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return getScbParticleSystem().getStaticFriction();
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::setStaticFriction(PxReal a)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	PX_CHECK_AND_RETURN(a >= 0.0f,"Static Friction needs to be positive, PxParticleBase::setStaticFriction() ignored.");
	getScbParticleSystem().setStaticFriction(a);
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::setSimulationFilterData(const PxFilterData& data)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	getScbParticleSystem().setSimulationFilterData(data);
}


template<class APIClass, class LeafClass>
PxFilterData NpParticleBaseTemplate<APIClass, LeafClass>::getSimulationFilterData() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return getScbParticleSystem().getSimulationFilterData();
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::resetFiltering()
{
	physx::shdfnd::getFoundation().error(physx::PxErrorCode::eDEBUG_INFO, __FILE__, __LINE__, "PxParticleBase::resetFiltering: This method has been deprecated!");

	NpScene* scene = NpActor::getOwnerScene(*this);
	if (scene)
		scene->resetFiltering(*this);
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::setParticleBaseFlag(PxParticleBaseFlag::Enum flag, bool value)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	
	// flags that are generally immutable
	if (flag == PxParticleBaseFlag::ePER_PARTICLE_REST_OFFSET)
	{
		Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, "PxParticleBaseFlag::ePER_PARTICLE_REST_OFFSET flag is not modifiable. Operation ignored.");
		return;
	}
	
	PxParticleBaseFlags flags = getScbParticleSystem().getFlags();
	if(value)
		flags |= flag;
	else
		flags &= ~flag;

	getScbParticleSystem().setFlags(flags);

	if (getNpScene())
		getNpScene()->updatePhysXIndicator();
}


template<class APIClass, class LeafClass>
PxParticleBaseFlags NpParticleBaseTemplate<APIClass, LeafClass>::getParticleBaseFlags() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return getScbParticleSystem().getFlags();
}

template<class APIClass, class LeafClass>
PxParticleReadDataFlags NpParticleBaseTemplate<APIClass, LeafClass>::getParticleReadDataFlags() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return getScbParticleSystem().getParticleReadDataFlags();
}

template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::setParticleReadDataFlag(PxParticleReadDataFlag::Enum flag, bool value)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));	
	PX_CHECK_AND_RETURN(!getScbParticleSystem().getScParticleSystem().getSim(),"PxParticleReadDataFlag immutable when the particle system is part of a scene.");
	PxParticleReadDataFlags flags = getScbParticleSystem().getParticleReadDataFlags();

	PX_CHECK_AND_RETURN(!(flag == PxParticleReadDataFlag::eREST_OFFSET_BUFFER && value)
						|| (getScbParticleSystem().getFlags() & PxParticleBaseFlag::ePER_PARTICLE_REST_OFFSET),
						"PxParticleReadDataFlag::eREST_OFFSET_BUFFER not supported without specifying per particle rest offsets on calling "
						"PxPhysics::createParticleSystem or PxPhysics::createParticleFluid.");

	if(value)
		flags |= flag;
	else
		flags &= ~flag;
	
	getScbParticleSystem().setParticleReadDataFlags(flags);
}

template<class APIClass, class LeafClass>
PxU32 NpParticleBaseTemplate<APIClass, LeafClass>::getMaxParticles() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return getScbParticleSystem().getMaxParticles();
}


template<class APIClass, class LeafClass>
PxReal NpParticleBaseTemplate<APIClass, LeafClass>::getMaxMotionDistance() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return getScbParticleSystem().getMaxMotionDistance();
}

template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::setMaxMotionDistance(PxReal d)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	PX_CHECK_AND_RETURN(d >= 0.0f,"MaxMotionDistance needs to be positive, PxParticleBase::setMaxMotionDistance() ignored.");
	PX_CHECK_AND_RETURN(!getScbParticleSystem().getScParticleSystem().getSim(),"MaxMotionDistance immutable when the particle system is part of a scene.");	
	getScbParticleSystem().setMaxMotionDistance(d);
}

template<class APIClass, class LeafClass>
PxReal NpParticleBaseTemplate<APIClass, LeafClass>::getRestOffset() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return getScbParticleSystem().getRestOffset();
}

template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::setRestOffset(PxReal r)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	PX_CHECK_AND_RETURN(r >= 0.0f,"RestOffset needs to be positive, PxParticleBase::setRestOffset() ignored.");
	PX_CHECK_AND_RETURN(!getScbParticleSystem().getScParticleSystem().getSim(),"RestOffset immutable when the particle system is part of a scene.");
#if PX_CHECKED
	if(getParticleBaseFlags() & PxParticleBaseFlag::ePER_PARTICLE_REST_OFFSET)
	{	
		Pt::ParticleData* standaloneData = getScbParticleSystem().getScParticleSystem().obtainStandaloneData();
		PX_ASSERT(standaloneData);
		bool pass = checkRestOffset(*standaloneData, r);
		getScbParticleSystem().getScParticleSystem().returnStandaloneData(standaloneData);
		PX_CHECK_AND_RETURN(pass, "PxParticleBase::restOffset: offset must be larger than each perParticle restOffset.");
	}
#endif
	getScbParticleSystem().setRestOffset(r);
}

template<class APIClass, class LeafClass>
PxReal NpParticleBaseTemplate<APIClass, LeafClass>::getContactOffset() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return getScbParticleSystem().getContactOffset();
}

template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::setContactOffset(PxReal c)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	PX_CHECK_AND_RETURN(c >= 0.0f,"ContactOffset needs to be positive, PxParticleBase::setContactOffset() ignored.");
	PX_CHECK_AND_RETURN(!getScbParticleSystem().getScParticleSystem().getSim(),"ContactOffset immutable when the particle system is part of a scene.");	
	getScbParticleSystem().setContactOffset(c);
}

template<class APIClass, class LeafClass>
PxReal NpParticleBaseTemplate<APIClass, LeafClass>::getGridSize() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	return getScbParticleSystem().getGridSize();
}

template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::setGridSize(PxReal g)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));
	PX_CHECK_AND_RETURN(g >= 0.0f,"GridSize needs to be positive, PxParticleBase::setGridSize() ignored.");
	PX_CHECK_AND_RETURN(!getScbParticleSystem().getScParticleSystem().getSim(),"GridSize immutable when the particle system is part of a scene.");	
	getScbParticleSystem().setGridSize(g);
}

template<class APIClass, class LeafClass>
PxBounds3 NpParticleBaseTemplate<APIClass, LeafClass>::getWorldBounds(float inflation) const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));
	const PxBounds3 bounds = getScbParticleSystem().getWorldBounds();
	PX_ASSERT(bounds.isValid());

	// PT: unfortunately we can't just scale the min/max vectors, we need to go through center/extents.
	const PxVec3 center = bounds.getCenter();
	const PxVec3 inflatedExtents = bounds.getExtents() * inflation;
	return PxBounds3::centerExtents(center, inflatedExtents);
}


#if PX_SUPPORT_GPU_PHYSX

template<class APIClass, class LeafClass>
PxParticleDeviceExclusiveAccess* NpParticleBaseTemplate<APIClass, LeafClass>::getDeviceExclusiveAccessGpu(const char* callerName) const
{
	PxParticleDeviceExclusiveAccess* access = NULL;
	if (getNpScene() && (getParticleBaseFlags() & PxParticleBaseFlag::eGPU))
	{
		access = getScbParticleSystem().getDeviceExclusiveAccessGpu();
	}

	if (!access)
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"%s is only allowed to be called if the particle system is in device exclusive mode. "
			"Please refer to PxParticleDeviceExclusive::enable and PxParticleDeviceExclusive::isEnabled.", callerName);
	}

	return access;
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::enableDeviceExclusiveModeGpu()
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	if (!getNpScene())
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "PxParticleDeviceExclusive::enable is not allowed to be called if the particle system is not in a scene.");
		return;
	}

	if (!(getParticleBaseFlags() & PxParticleBaseFlag::eGPU))
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "PxParticleDeviceExclusive::enable is not allowed for non PxParticleBaseFlag::eGPU particle systems.");
		return;
	}

	getScbParticleSystem().enableDeviceExclusiveModeGpu();
}


template<class APIClass, class LeafClass>
bool NpParticleBaseTemplate<APIClass, LeafClass>::isDeviceExclusiveModeEnabledGpu() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	if (!getNpScene() || !(getParticleBaseFlags() & PxParticleBaseFlag::eGPU))
	{
		return false;
	}

	return getScbParticleSystem().getDeviceExclusiveAccessGpu() != NULL;
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::getReadWriteCudaBuffersGpu(PxCudaReadWriteParticleBuffers& buffers) const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	PxParticleDeviceExclusiveAccess* access = getDeviceExclusiveAccessGpu("PxParticleDeviceExclusive::getReadWriteCudaBuffers");
	if (access)
	{
		access->getReadWriteCudaBuffers(buffers);
	}
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::setValidParticleRangeGpu(PxU32 validParticleRange)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PxParticleDeviceExclusiveAccess* access = getDeviceExclusiveAccessGpu("PxParticleDeviceExclusive::setValidParticleRange");
	if (access)
	{
		access->setValidParticleRange(validParticleRange);
	}
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::setDeviceExclusiveModeFlagsGpu(PxU32 flags)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PxParticleDeviceExclusiveAccess* access = getDeviceExclusiveAccessGpu("PxParticleDeviceExclusive::setFlags");
	if (access)
	{
		access->setFlags(flags);
	}
}


template<class APIClass, class LeafClass>
physx::PxBaseTask* NpParticleBaseTemplate<APIClass, LeafClass>::getLaunchTaskGpu() const
{
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	PxParticleDeviceExclusiveAccess* access = getDeviceExclusiveAccessGpu("PxParticleDeviceExclusive::getLaunchTask");
	if (access)
	{
		return access->getLaunchTask();
	}
	return NULL;
}


template<class APIClass, class LeafClass>
void NpParticleBaseTemplate<APIClass, LeafClass>::addLaunchTaskDependentGpu(class physx::PxBaseTask& dependent)
{
	NP_WRITE_CHECK(NpActor::getOwnerScene(*this));

	PxParticleDeviceExclusiveAccess* access = getDeviceExclusiveAccessGpu("PxParticleDeviceExclusive::addLaunchTaskDependent");
	if (access)
	{
		access->addLaunchTaskDependent(dependent);
	}
}


template<class APIClass, class LeafClass>
size_t NpParticleBaseTemplate<APIClass, LeafClass>::getCudaStreamGpu() const
{	
	NP_READ_CHECK(NpActor::getOwnerScene(*this));

	PxParticleDeviceExclusiveAccess* access = getDeviceExclusiveAccessGpu("PxParticleDeviceExclusive::getCudaStream");
	if (access)
	{
		return access->getCudaStream();
	}
	return 0;
}


#endif
}

#endif // PX_USE_PARTICLE_SYSTEM_API

#endif
