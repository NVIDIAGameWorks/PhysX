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


#include "NpCast.h"
#include "NpFactory.h"
#include "NpPhysics.h"
#include "ScPhysics.h"
#include "ScbScene.h"
#include "ScbActor.h"
#include "GuHeightField.h"
#include "GuTriangleMesh.h"
#include "GuConvexMesh.h"

#if PX_USE_PARTICLE_SYSTEM_API
	#include "NpParticleSystem.h"
#endif

#if PX_USE_CLOTH_API
	#include "NpClothFabric.h"
	#include "PxClothCollisionData.h"
#endif

#include "NpConnector.h"
#include "NpPtrTableStorageManager.h"
#include "CmCollection.h"

using namespace physx;

NpFactory::NpFactory()
: GuMeshFactory()
, mConnectorArrayPool(PX_DEBUG_EXP("connectorArrayPool"))
, mPtrTableStorageManager(PX_NEW(NpPtrTableStorageManager))
, mMaterialPool(PX_DEBUG_EXP("MaterialPool"))
#if PX_USE_CLOTH_API
	, mClothFabricArray(PX_DEBUG_EXP("clothFabricArray"))
#endif
#if PX_SUPPORT_PVD
	, mNpFactoryListener(NULL)
#endif	
{
}

namespace
{
	template <typename T> void releaseAll(Ps::HashSet<T*>& container)
	{
		// a bit tricky: release will call the factory back to remove the object from
		// the tracking array, immediately invalidating the iterator. Reconstructing the
		// iterator per delete can be expensive. So, we use a temporary object.
		//
		// a coalesced hash would be efficient too, but we only ever iterate over it
		// here so it's not worth the 2x remove penalty over the normal hash.

		Ps::Array<T*, Ps::ReflectionAllocator<T*> > tmp;
		tmp.reserve(container.size());
		for(typename Ps::HashSet<T*>::Iterator iter = container.getIterator(); !iter.done(); ++iter)
			tmp.pushBack(*iter);

		PX_ASSERT(tmp.size() == container.size());
		for(PxU32 i=0;i<tmp.size();i++)
			tmp[i]->release();
	}
}



NpFactory::~NpFactory()
{
	PX_DELETE(mPtrTableStorageManager);
}


void NpFactory::release()
{
	releaseAll(mAggregateTracking);
	releaseAll(mConstraintTracking);
	releaseAll(mArticulationTracking);
	releaseAll(mActorTracking);
	while(mShapeTracking.size())
		static_cast<NpShape*>(mShapeTracking.getEntries()[0])->releaseInternal();

#if PX_USE_CLOTH_API
	while(mClothFabricArray.size())
	{
		mClothFabricArray[0]->release();
	}
#endif

	GuMeshFactory::release();  // deletes the class
}

void NpFactory::createInstance()
{
	PX_ASSERT(!mInstance);
	mInstance = PX_NEW(NpFactory)();
}


void NpFactory::destroyInstance()
{
	PX_ASSERT(mInstance);
	mInstance->release();
	mInstance = NULL;
}


NpFactory* NpFactory::mInstance = NULL;

///////////////////////////////////////////////////////////////////////////////

namespace
{
	template <typename T> void addToTracking(Ps::HashSet<T*>& set, T* element, Ps::Mutex& mutex, bool lock=true)
	{
		if(!element)
			return;

		if(lock)
			mutex.lock();

		set.insert(element);

		if(lock)
			mutex.unlock();
	}
}

/////////////////////////////////////////////////////////////////////////////// Actors

void NpFactory::addRigidStatic(PxRigidStatic* npActor, bool lock)
{
	addToTracking<PxActor>(mActorTracking, npActor, mTrackingMutex, lock);
}

void NpFactory::addRigidDynamic(PxRigidDynamic* npBody, bool lock)
{
	addToTracking<PxActor>(mActorTracking, npBody, mTrackingMutex, lock);
}

void NpFactory::addShape(PxShape* shape, bool lock)
{
	// this uses a coalesced hash set rather than a normal hash set so that we can iterate over it efficiently
	if(!shape)
		return;

	if(lock)
		mTrackingMutex.lock();

	mShapeTracking.insert(shape);

	if(lock)
		mTrackingMutex.unlock();
}


#if PX_USE_PARTICLE_SYSTEM_API

namespace
{
	NpParticleSystem* createParticleSystem(PxU32 maxParticles, bool perParticleRestOffset)
	{		
		return NpFactory::getInstance().createNpParticleSystem(maxParticles, perParticleRestOffset);
	}

	NpParticleFluid* createParticleFluid(PxU32 maxParticles, bool perParticleRestOffset)
	{		
		return NpFactory::getInstance().createNpParticleFluid(maxParticles, perParticleRestOffset);
	}

	// pointers to function above, initialized during subsystem registration
	NpParticleSystem* (*sCreateParticleSystemFn)(PxU32 maxParticles, bool perParticleRestOffset) = 0;
	NpParticleFluid* (*sCreateParticleFluidFn)(PxU32 maxParticles, bool perParticleRestOffset) = 0;
}

void NpFactory::registerParticles()
{
	sCreateParticleSystemFn = &::createParticleSystem;
	sCreateParticleFluidFn = &::createParticleFluid;
}

void NpFactory::addParticleSystem(PxParticleSystem* ps, bool lock)
{	
	addToTracking<PxActor>(mActorTracking, ps, mTrackingMutex, lock);
}

void NpFactory::addParticleFluid(PxParticleFluid* fluid, bool lock)
{
	addToTracking<PxActor>(mActorTracking, fluid, mTrackingMutex, lock);
}

NpParticleSystem* NpFactory::createNpParticleSystem(PxU32 maxParticles, bool perParticleRestOffset)
{
    NpParticleSystem* npParticleSystem;
	{
		Ps::Mutex::ScopedLock lock(mParticleSystemPoolLock);		
		npParticleSystem = mParticleSystemPool.construct(maxParticles, perParticleRestOffset);
	}
	return npParticleSystem;	
}

void NpFactory::releaseParticleSystemToPool(NpParticleSystem& particleSystem)
{
	PX_ASSERT(particleSystem.getBaseFlags() & PxBaseFlag::eOWNS_MEMORY);
	Ps::Mutex::ScopedLock lock(mParticleSystemPoolLock);
	mParticleSystemPool.destroy(&particleSystem);
}

NpParticleFluid* NpFactory::createNpParticleFluid(PxU32 maxParticles, bool perParticleRestOffset)
{
    NpParticleFluid* npParticleFluid;
	{
		Ps::Mutex::ScopedLock lock(mParticleFluidPoolLock);		
		npParticleFluid = mParticleFluidPool.construct(maxParticles, perParticleRestOffset);
	}
	return npParticleFluid;	
}

void NpFactory::releaseParticleFluidToPool(NpParticleFluid& particleFluid)
{
	PX_ASSERT(particleFluid.getBaseFlags() & PxBaseFlag::eOWNS_MEMORY);
	Ps::Mutex::ScopedLock lock(mParticleFluidPoolLock);
	mParticleFluidPool.destroy(&particleFluid);
}

PxParticleFluid* NpFactory::createParticleFluid(PxU32 maxParticles, bool perParticleRestOffset)
{
	if (!sCreateParticleFluidFn)
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Particle fluid creation failed. Use PxRegisterParticles to register particle module: returned NULL.");
		return NULL;
	}

	PxParticleFluid* fluid = sCreateParticleFluidFn(maxParticles, perParticleRestOffset);
	if (!fluid)
	{
		Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, 
			"Particle fluid initialization failed: returned NULL.");
		return NULL;
	}

	addParticleFluid(fluid);
	return fluid;
}

PxParticleSystem* NpFactory::createParticleSystem(PxU32 maxParticles, bool perParticleRestOffset)
{
	if (!sCreateParticleSystemFn)
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Particle system creation failed. Use PxRegisterParticles to register particle module: returned NULL.");
		return NULL;
	}

	PxParticleSystem* ps = sCreateParticleSystemFn(maxParticles, perParticleRestOffset);
	if (!ps)
	{
		Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, 
			"Particle system initialization failed: returned NULL.");
		return NULL;
	}

	addParticleSystem(ps);
	return ps;
}

#endif


#if PX_USE_CLOTH_API

namespace 
{
	NpCloth* createCloth(const PxTransform& globalPose, PxClothFabric& fabric, const PxClothParticle* particles, PxClothFlags flags)
	{		
		return NpFactory::getInstance().createNpCloth(globalPose, static_cast<NpClothFabric&>(fabric), particles, flags);
	}

	NpClothFabric* createClothFabric(PxInputStream& stream)
	{
		NpClothFabric* fabric = NpFactory::getInstance().createNpClothFabric();
		if(fabric)
		{
			if(fabric->load(stream))
				return fabric;
			fabric->decRefCount();
		}
		return NULL;
	}

	NpClothFabric* createClothFabric(const PxClothFabricDesc& desc)
	{
		NpClothFabric* fabric = NpFactory::getInstance().createNpClothFabric();
		if(fabric)
		{
			if(fabric->load(desc))
				return fabric;
			fabric->decRefCount();
		}
		return NULL;
	}

	// pointers to functions above, initialized during subsystem registration
	NpCloth* (*sCreateClothFn)(const PxTransform&, PxClothFabric&, const PxClothParticle*, PxClothFlags) = 0;
	NpClothFabric* (*sCreateClothFabricFromStreamFn)(PxInputStream&) = 0;
	NpClothFabric* (*sCreateClothFabricFromDescFn)(const PxClothFabricDesc&) = 0;
}

void NpFactory::registerCloth()
{
	sCreateClothFn = &::createCloth;
	sCreateClothFabricFromStreamFn = &::createClothFabric;
	sCreateClothFabricFromDescFn = &::createClothFabric;

	Sc::Physics::getInstance().registerCloth();	
}

void NpFactory::addCloth(PxCloth* cloth, bool lock)
{
	addToTracking<PxActor>(mActorTracking, cloth, mTrackingMutex, lock);
}

NpCloth* NpFactory::createNpCloth(const PxTransform& globalPose, PxClothFabric& fabric, const PxClothParticle* particles, PxClothFlags flags)
{
    NpCloth* npCloth;
	{
		Ps::Mutex::ScopedLock lock(mClothPoolLock);		
		npCloth = mClothPool.construct(globalPose, static_cast<NpClothFabric&>(fabric), particles, flags);
	}
	return npCloth;	
}

void NpFactory::releaseClothToPool(NpCloth& cloth)
{
	PX_ASSERT(cloth.getBaseFlags() & PxBaseFlag::eOWNS_MEMORY);
	Ps::Mutex::ScopedLock lock(mClothPoolLock);
	mClothPool.destroy(&cloth);
}

NpClothFabric* NpFactory::createNpClothFabric()
{
    NpClothFabric* npClothFabric;
	{
		Ps::Mutex::ScopedLock lock(mClothFabricPoolLock);		
		npClothFabric = mClothFabricPool.construct();
	}
	return npClothFabric;	
}

void NpFactory::releaseClothFabricToPool(NpClothFabric& clothFabric)
{
	PX_ASSERT(clothFabric.getBaseFlags() & PxBaseFlag::eOWNS_MEMORY);
	Ps::Mutex::ScopedLock lock(mClothFabricPoolLock);
	mClothFabricPool.destroy(&clothFabric);
}

void NpFactory::addClothFabric(NpClothFabric* cf, bool lock)
{
	if(lock)
	{
		Ps::Mutex::ScopedLock lock_(mTrackingMutex);
		if(!mClothFabricArray.size())
			mClothFabricArray.reserve(64);

		mClothFabricArray.pushBack(cf);
	}
	else
	{
		if(!mClothFabricArray.size())
			mClothFabricArray.reserve(64);

		mClothFabricArray.pushBack(cf);
	}
}

PxClothFabric* NpFactory::createClothFabric(PxInputStream& stream)
{
	if(!sCreateClothFabricFromStreamFn)
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Cloth not registered: returned NULL.");
		return NULL;
	}

	NpClothFabric* result = (*sCreateClothFabricFromStreamFn)(stream);

	if(result)
		addClothFabric(result);

	return result;
}

PxClothFabric* NpFactory::createClothFabric(const PxClothFabricDesc& desc)
{
	if(!sCreateClothFabricFromDescFn)
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Cloth not registered: returned NULL.");
		return NULL;
	}

	NpClothFabric* result = (*sCreateClothFabricFromDescFn)(desc);

	if(result)
		addClothFabric(result);

	return result;
}

bool NpFactory::removeClothFabric(PxClothFabric& cf)
{
	NpClothFabric* npClothFabric = &static_cast<NpClothFabric&>(cf);

	Ps::Mutex::ScopedLock lock(mTrackingMutex);

	// remove the cloth fabric from the array
	for(PxU32 i=0; i<mClothFabricArray.size(); i++)
	{
		if(mClothFabricArray[i]==npClothFabric)
		{
			mClothFabricArray.replaceWithLast(i);
#if PX_SUPPORT_PVD
			if(mNpFactoryListener)
				mNpFactoryListener->onNpFactoryBufferRelease(cf);
#endif
			return true;
		}
	}
	return false;
}

PxU32 NpFactory::getNbClothFabrics() const
{
	return mClothFabricArray.size();
}

PxU32 NpFactory::getClothFabrics(PxClothFabric** userBuffer, PxU32 bufferSize) const
{
	const PxU32 size = mClothFabricArray.size();

	const PxU32 writeCount = PxMin(size, bufferSize);
	for(PxU32 i=0; i<writeCount; i++)
		userBuffer[i] = mClothFabricArray[i];

	return writeCount;
}

PxCloth* NpFactory::createCloth(const PxTransform& globalPose, PxClothFabric& fabric, const PxClothParticle* particles, PxClothFlags flags)
{
	PX_CHECK_AND_RETURN_NULL(globalPose.isValid(),"globalPose is not valid.  createCloth returns NULL.");
	PX_CHECK_AND_RETURN_NULL((particles != NULL) && fabric.getNbParticles(), "No particles supplied. createCloth returns NULL.");

#if PX_CHECKED
	PX_CHECK_AND_RETURN_NULL(NpCloth::checkParticles(fabric.getNbParticles(), particles), "PxPhysics::createCloth: particle values must be finite and inverse weight must not be negative");
#endif

	if(!sCreateClothFn)
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Cloth not registered: returned NULL.");
		return NULL;
	}

	// create the internal cloth object
	NpCloth* npCloth = (*sCreateClothFn)(globalPose, fabric, particles, flags);
	if (npCloth)
	{
		addToTracking<PxActor>(mActorTracking, npCloth, mTrackingMutex);
		return npCloth;
	}
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, 
			"Cloth initialization failed: returned NULL.");
		return NULL;
	}
}
#endif

void NpFactory::onActorRelease(PxActor* a)
{
	Ps::Mutex::ScopedLock lock(mTrackingMutex);
	mActorTracking.erase(a);
}

void NpFactory::onShapeRelease(PxShape* a)
{
	Ps::Mutex::ScopedLock lock(mTrackingMutex);
	mShapeTracking.erase(a);
}


void NpFactory::addArticulation(PxArticulation* npArticulation, bool lock)
{
	addToTracking<PxArticulation>(mArticulationTracking, npArticulation, mTrackingMutex, lock);
}

namespace
{
	NpArticulation* createArticulation()
	{
		NpArticulation* npArticulation = NpFactory::getInstance().createNpArticulation();
		if (!npArticulation)
			Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "Articulation initialization failed: returned NULL.");

		return npArticulation;
	}

	NpArticulationLink* createArticulationLink(NpArticulation&root, NpArticulationLink* parent, const PxTransform& pose)
	{
		PX_CHECK_AND_RETURN_NULL(pose.isValid(),"Supplied PxArticulation pose is not valid. Articulation link creation method returns NULL.");
		PX_CHECK_AND_RETURN_NULL((!parent || (&parent->getRoot() == &root)), "specified parent link is not part of the destination articulation. Articulation link creation method returns NULL.");
	
		NpArticulationLink* npArticulationLink = NpFactory::getInstance().createNpArticulationLink(root, parent, pose);
		if (!npArticulationLink)
		{
			Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, 
				"Articulation link initialization failed: returned NULL.");
			return NULL;
		}

		if (parent)
		{
			PxTransform parentPose = parent->getCMassLocalPose().transformInv(pose);
			PxTransform childPose = PxTransform(PxIdentity);
						
			NpArticulationJoint* npArticulationJoint = NpFactory::getInstance().createNpArticulationJoint(*parent, parentPose, *npArticulationLink, childPose);
			if (!npArticulationJoint)
			{
				PX_DELETE(npArticulationLink);
	
				Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, 
				"Articulation link initialization failed due to joint creation failure: returned NULL.");
				return NULL;
			}

			npArticulationLink->setInboundJoint(*npArticulationJoint);
		}

		return npArticulationLink;
	}

	// pointers to functions above, initialized during subsystem registration
	static NpArticulation* (*sCreateArticulationFn)() = 0;
	static NpArticulationLink* (*sCreateArticulationLinkFn)(NpArticulation&, NpArticulationLink* parent, const PxTransform& pose) = 0;
}

void NpFactory::registerArticulations()
{
	sCreateArticulationFn = &::createArticulation;
	sCreateArticulationLinkFn = &::createArticulationLink;
}

NpArticulation* NpFactory::createNpArticulation()
{
    NpArticulation* npArticulation;
	{
		Ps::Mutex::ScopedLock lock(mArticulationPoolLock);		
		npArticulation = mArticulationPool.construct();
	}
	return npArticulation;	
}

void NpFactory::releaseArticulationToPool(NpArticulation& articulation)
{
	PX_ASSERT(articulation.getBaseFlags() & PxBaseFlag::eOWNS_MEMORY);
	Ps::Mutex::ScopedLock lock(mArticulationPoolLock);
	mArticulationPool.destroy(&articulation);
}

PxArticulation* NpFactory::createArticulation()
{
	if(!sCreateArticulationFn)
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Articulations not registered: returned NULL.");
		return NULL;
	}

	NpArticulation* npArticulation = (*sCreateArticulationFn)();
	if(npArticulation)
		addArticulation(npArticulation);

	return npArticulation;
}

void NpFactory::onArticulationRelease(PxArticulation* a)
{
	Ps::Mutex::ScopedLock lock(mTrackingMutex);
	mArticulationTracking.erase(a);
}

NpArticulationLink* NpFactory::createNpArticulationLink(NpArticulation&root, NpArticulationLink* parent, const PxTransform& pose)
{
	 NpArticulationLink* npArticulationLink;
	{
		Ps::Mutex::ScopedLock lock(mArticulationLinkPoolLock);		
		npArticulationLink = mArticulationLinkPool.construct(pose, root, parent);
	}
	return npArticulationLink;	
}

void NpFactory::releaseArticulationLinkToPool(NpArticulationLink& articulationLink)
{
	PX_ASSERT(articulationLink.getBaseFlags() & PxBaseFlag::eOWNS_MEMORY);
	Ps::Mutex::ScopedLock lock(mArticulationLinkPoolLock);
	mArticulationLinkPool.destroy(&articulationLink);
}

PxArticulationLink* NpFactory::createArticulationLink(NpArticulation& root, NpArticulationLink* parent, const PxTransform& pose)
{
	if(!sCreateArticulationLinkFn)
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Articulations not registered: returned NULL.");
		return NULL;
	}

	return (*sCreateArticulationLinkFn)(root, parent, pose);
}

NpArticulationJoint* NpFactory::createNpArticulationJoint(NpArticulationLink& parent, const PxTransform& parentFrame, NpArticulationLink& child, const PxTransform& childFrame)
{
	 NpArticulationJoint* npArticulationJoint;
	{
		Ps::Mutex::ScopedLock lock(mArticulationJointPoolLock);		
		npArticulationJoint = mArticulationJointPool.construct(parent, parentFrame, child, childFrame);
	}
	return npArticulationJoint;	
}

void NpFactory::releaseArticulationJointToPool(NpArticulationJoint& articulationJoint)
{
	PX_ASSERT(articulationJoint.getBaseFlags() & PxBaseFlag::eOWNS_MEMORY);
	Ps::Mutex::ScopedLock lock(mArticulationJointPoolLock);
	mArticulationJointPool.destroy(&articulationJoint);
}

/////////////////////////////////////////////////////////////////////////////// constraint

void NpFactory::addConstraint(PxConstraint* npConstraint, bool lock)
{
	addToTracking<PxConstraint>(mConstraintTracking, npConstraint, mTrackingMutex, lock);
}

PxConstraint* NpFactory::createConstraint(PxRigidActor* actor0, PxRigidActor* actor1, PxConstraintConnector& connector, const PxConstraintShaderTable& shaders, PxU32 dataSize)
{
	PX_CHECK_AND_RETURN_NULL((actor0 && !actor0->is<PxRigidStatic>()) || (actor1 && !actor1->is<PxRigidStatic>()), "createConstraint: At least one actor must be dynamic or an articulation link");

	NpConstraint* npConstraint;
	{
		Ps::Mutex::ScopedLock lock(mConstraintPoolLock);
		npConstraint = mConstraintPool.construct(actor0, actor1, connector, shaders, dataSize);
	}
	addConstraint(npConstraint);
	return npConstraint;
}

void NpFactory::releaseConstraintToPool(NpConstraint& constraint)
{
	PX_ASSERT(constraint.getBaseFlags() & PxBaseFlag::eOWNS_MEMORY);
	Ps::Mutex::ScopedLock lock(mConstraintPoolLock);
	mConstraintPool.destroy(&constraint);
}

void NpFactory::onConstraintRelease(PxConstraint* c)
{
	Ps::Mutex::ScopedLock lock(mTrackingMutex);
	mConstraintTracking.erase(c);
}

/////////////////////////////////////////////////////////////////////////////// aggregate

// PX_AGGREGATE
void NpFactory::addAggregate(PxAggregate* npAggregate, bool lock)
{
	addToTracking<PxAggregate>(mAggregateTracking, npAggregate, mTrackingMutex, lock);
}

PxAggregate* NpFactory::createAggregate(PxU32 maxActors, bool selfCollisions)
{
	PX_CHECK_AND_RETURN_NULL(maxActors<=128, "maxActors limited to 128. createAggregate method returns NULL.");

	NpAggregate* npAggregate;

	{
		Ps::Mutex::ScopedLock lock(mAggregatePoolLock);
		npAggregate = mAggregatePool.construct(maxActors, selfCollisions);
	}

	addAggregate(npAggregate);
	return npAggregate;
}

void NpFactory::releaseAggregateToPool(NpAggregate& aggregate)
{
	PX_ASSERT(aggregate.getBaseFlags() & PxBaseFlag::eOWNS_MEMORY);
	Ps::Mutex::ScopedLock lock(mAggregatePoolLock);
	mAggregatePool.destroy(&aggregate);
}

void NpFactory::onAggregateRelease(PxAggregate* a)
{
	Ps::Mutex::ScopedLock lock(mTrackingMutex);
	mAggregateTracking.erase(a);
}
//~PX_AGGREGATE


///////////////////////////////////////////////////////////////////////////////

PxMaterial* NpFactory::createMaterial(PxReal staticFriction, PxReal dynamicFriction, PxReal restitution)
{
	PX_CHECK_AND_RETURN_NULL(dynamicFriction >= 0.0f, "createMaterial: dynamicFriction must be >= 0.");
	PX_CHECK_AND_RETURN_NULL(staticFriction >= 0.0f, "createMaterial: staticFriction must be >= 0.");
	PX_CHECK_AND_RETURN_NULL(restitution >= 0.0f || restitution <= 1.0f, "createMaterial: restitution must be between 0 and 1.");
	
	Sc::MaterialData data;
	data.staticFriction = staticFriction;
	data.dynamicFriction = dynamicFriction;
	data.restitution = restitution;

	NpMaterial* npMaterial;
	{
		Ps::Mutex::ScopedLock lock(mMaterialPoolLock);		
		npMaterial = mMaterialPool.construct(data);
	}
	return npMaterial;	
}

void NpFactory::releaseMaterialToPool(NpMaterial& material)
{
	PX_ASSERT(material.getBaseFlags() & PxBaseFlag::eOWNS_MEMORY);
	Ps::Mutex::ScopedLock lock(mMaterialPoolLock);
	mMaterialPool.destroy(&material);
}

///////////////////////////////////////////////////////////////////////////////

NpConnectorArray* NpFactory::acquireConnectorArray()
{
	Ps::MutexT<>::ScopedLock l(mConnectorArrayPoolLock);
	return mConnectorArrayPool.construct();
}

void NpFactory::releaseConnectorArray(NpConnectorArray* array)
{
	Ps::MutexT<>::ScopedLock l(mConnectorArrayPoolLock);
	mConnectorArrayPool.destroy(array);
}

///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////

NpShape* NpFactory::createShape(const PxGeometry& geometry,
								PxShapeFlags shapeFlags,
								PxMaterial*const* materials,
								PxU16 materialCount,
								bool isExclusive)
{	
	switch(geometry.getType())
	{
		case PxGeometryType::eBOX:
			PX_CHECK_AND_RETURN_NULL(static_cast<const PxBoxGeometry&>(geometry).isValid(), "Supplied PxGeometry is not valid. Shape creation method returns NULL.");
			break;
		case PxGeometryType::eSPHERE:
			PX_CHECK_AND_RETURN_NULL(static_cast<const PxSphereGeometry&>(geometry).isValid(), "Supplied PxGeometry is not valid. Shape creation method returns NULL.");
			break;
		case PxGeometryType::eCAPSULE:
			PX_CHECK_AND_RETURN_NULL(static_cast<const PxCapsuleGeometry&>(geometry).isValid(), "Supplied PxGeometry is not valid. Shape creation method returns NULL.");
			break;
		case PxGeometryType::eCONVEXMESH:
			PX_CHECK_AND_RETURN_NULL(static_cast<const PxConvexMeshGeometry&>(geometry).isValid(), "Supplied PxGeometry is not valid. Shape creation method returns NULL.");
			break;
		case PxGeometryType::ePLANE:
			PX_CHECK_AND_RETURN_NULL(static_cast<const PxPlaneGeometry&>(geometry).isValid(), "Supplied PxGeometry is not valid. Shape creation method returns NULL.");
			break;
		case PxGeometryType::eHEIGHTFIELD:
			PX_CHECK_AND_RETURN_NULL(static_cast<const PxHeightFieldGeometry&>(geometry).isValid(), "Supplied PxGeometry is not valid. Shape creation method returns NULL.");
			break;
		case PxGeometryType::eTRIANGLEMESH:
			PX_CHECK_AND_RETURN_NULL(static_cast<const PxTriangleMeshGeometry&>(geometry).isValid(), "Supplied PxGeometry is not valid. Shape creation method returns NULL.");
			break;
		case PxGeometryType::eGEOMETRY_COUNT:
		case PxGeometryType::eINVALID:
			PX_ASSERT(0);
	}

	//
	// Check for invalid material table setups
	//

#if PX_CHECKED
	if (!NpShape::checkMaterialSetup(geometry, "Shape creation", materials, materialCount))
		return NULL;
#endif

	Ps::InlineArray<PxU16, 4> materialIndices("NpFactory::TmpMaterialIndexBuffer");
	materialIndices.resize(materialCount);
	if (materialCount == 1)
	{
		materialIndices[0] = Ps::to16((static_cast<NpMaterial*>(materials[0]))->getHandle());
	}
	else
	{
		NpMaterial::getMaterialIndices(materials, materialIndices.begin(), materialCount);
	}

	NpShape* npShape;
	{
		Ps::Mutex::ScopedLock lock(mShapePoolLock);
		PxU16* mi = materialIndices.begin(); // required to placate pool constructor arg passing
		npShape = mShapePool.construct(geometry, shapeFlags, mi, materialCount, isExclusive);
	}

	if(!npShape)
		return NULL;

	for (PxU32 i=0; i < materialCount; i++)
		static_cast<NpMaterial*>(npShape->getMaterial(i))->incRefCount();

	addShape(npShape);

	return npShape;
}

void NpFactory::releaseShapeToPool(NpShape& shape)
{
	PX_ASSERT(shape.getBaseFlags() & PxBaseFlag::eOWNS_MEMORY);
	Ps::Mutex::ScopedLock lock(mShapePoolLock);
	mShapePool.destroy(&shape);
}



PxU32 NpFactory::getNbShapes() const
{
	return mShapeTracking.size();
}

PxU32 NpFactory::getShapes(PxShape** userBuffer, PxU32 bufferSize, PxU32 startIndex)	const
{
	if(mShapeTracking.size()<startIndex)
		return 0;
	PxU32 count = PxMax(bufferSize, mShapeTracking.size()-startIndex);
	PxShape*const *shapes = mShapeTracking.getEntries();
	for(PxU32 i=0;i<count;i++)
		userBuffer[i] = shapes[startIndex+i];
	return count;
}


PxRigidStatic* NpFactory::createRigidStatic(const PxTransform& pose)
{
	PX_CHECK_AND_RETURN_NULL(pose.isValid(), "pose is not valid. createRigidStatic returns NULL.");

	NpRigidStatic* npActor;

	{
		Ps::Mutex::ScopedLock lock(mRigidStaticPoolLock);
		npActor = mRigidStaticPool.construct(pose);
	}

	addRigidStatic(npActor);
	return npActor;
}

void NpFactory::releaseRigidStaticToPool(NpRigidStatic& rigidStatic)
{
	PX_ASSERT(rigidStatic.getBaseFlags() & PxBaseFlag::eOWNS_MEMORY);
	Ps::Mutex::ScopedLock lock(mRigidStaticPoolLock);
	mRigidStaticPool.destroy(&rigidStatic);
}

PxRigidDynamic* NpFactory::createRigidDynamic(const PxTransform& pose)
{
	PX_CHECK_AND_RETURN_NULL(pose.isValid(), "pose is not valid. createRigidDynamic returns NULL.");

	NpRigidDynamic* npBody;
	{
		Ps::Mutex::ScopedLock lock(mRigidDynamicPoolLock);
		npBody = mRigidDynamicPool.construct(pose);
	}
	addRigidDynamic(npBody);
	return npBody;
}


void NpFactory::releaseRigidDynamicToPool(NpRigidDynamic& rigidDynamic)
{
	PX_ASSERT(rigidDynamic.getBaseFlags() & PxBaseFlag::eOWNS_MEMORY);
	Ps::Mutex::ScopedLock lock(mRigidDynamicPoolLock);
	mRigidDynamicPool.destroy(&rigidDynamic);
}

///////////////////////////////////////////////////////////////////////////////

// PT: this function is here to minimize the amount of locks when deserializing a collection
void NpFactory::addCollection(const Cm::Collection& collection)
{
	
	PxU32 nb = collection.getNbObjects();
	const Ps::Pair<PxBase* const, PxSerialObjectId>* entries = collection.internalGetObjects();
	// PT: we take the lock only once, here
	Ps::Mutex::ScopedLock lock(mTrackingMutex);

	for(PxU32 i=0;i<nb;i++)
	{
		PxBase* s = entries[i].first;
		const PxType serialType = s->getConcreteType();
//////////////////////////
		if(serialType==PxConcreteType::eHEIGHTFIELD)
		{
			Gu::HeightField* gu = static_cast<Gu::HeightField*>(s);
			gu->setMeshFactory(this);
			addHeightField(gu, false);
		}
		else if(serialType==PxConcreteType::eCONVEX_MESH)
		{
			Gu::ConvexMesh* gu = static_cast<Gu::ConvexMesh*>(s);
			gu->setMeshFactory(this);
			addConvexMesh(gu, false);
		}
		else if(serialType==PxConcreteType::eTRIANGLE_MESH_BVH33 || serialType==PxConcreteType::eTRIANGLE_MESH_BVH34)
		{
			Gu::TriangleMesh* gu = static_cast<Gu::TriangleMesh*>(s);
			gu->setMeshFactory(this);
			addTriangleMesh(gu, false);
		}
//////////////////////////
#if PX_USE_CLOTH_API
		else if (serialType==PxConcreteType::eCLOTH_FABRIC)
		{
			NpClothFabric* np = static_cast<NpClothFabric*>(s);
			// PT: TODO: investigate why cloth don't need a "setMeshFactory" call here
			addClothFabric(np, false);
		}
#endif
		else if(serialType==PxConcreteType::eRIGID_DYNAMIC)
		{
			NpRigidDynamic* np = static_cast<NpRigidDynamic*>(s);
			addRigidDynamic(np, false);
		}
		else if(serialType==PxConcreteType::eRIGID_STATIC)
		{
			NpRigidStatic* np = static_cast<NpRigidStatic*>(s);
			addRigidStatic(np, false);
		}
		else if(serialType==PxConcreteType::eSHAPE)
		{
			NpShape* np = static_cast<NpShape*>(s);
			addShape(np, false);
		}
		else if(serialType==PxConcreteType::eMATERIAL)
		{
		}
		else if(serialType==PxConcreteType::eCONSTRAINT)
		{
			NpConstraint* np = static_cast<NpConstraint*>(s);
			addConstraint(np, false);
		}
#if PX_USE_CLOTH_API
		else if (serialType==PxConcreteType::eCLOTH)
		{
			NpCloth* np = static_cast<NpCloth*>(s);
			addCloth(np, false);
		}
#endif
#if PX_USE_PARTICLE_SYSTEM_API
		else if(serialType==PxConcreteType::ePARTICLE_SYSTEM)
		{
			NpParticleSystem* np = static_cast<NpParticleSystem*>(s);
			addParticleSystem(np, false);
		}
		else if(serialType==PxConcreteType::ePARTICLE_FLUID)
		{
			NpParticleFluid* np = static_cast<NpParticleFluid*>(s);
			addParticleFluid(np, false);
		}
#endif
		else if(serialType==PxConcreteType::eAGGREGATE)
		{
			NpAggregate* np = static_cast<NpAggregate*>(s);
			addAggregate(np, false);

			// PT: TODO: double-check this.... is it correct?			
			for(PxU32 j=0;j<np->getCurrentSizeFast();j++)
			{
				PxBase* actor = np->getActorFast(j);
				const PxType serialType1 = actor->getConcreteType();

				if(serialType1==PxConcreteType::eRIGID_STATIC)
					addRigidStatic(static_cast<NpRigidStatic*>(actor), false);
				else if(serialType1==PxConcreteType::eRIGID_DYNAMIC)
					addRigidDynamic(static_cast<NpRigidDynamic*>(actor), false);
				else if(serialType1==PxConcreteType::ePARTICLE_SYSTEM)
				{}
				else if(serialType1==PxConcreteType::ePARTICLE_FLUID)
				{}
				else if(serialType1==PxConcreteType::eARTICULATION_LINK)
				{}
				else PX_ASSERT(0);
			}
		}
		else if(serialType==PxConcreteType::eARTICULATION)
		{
			NpArticulation* np = static_cast<NpArticulation*>(s);
			addArticulation(np, false);
		}
		else if(serialType==PxConcreteType::eARTICULATION_LINK)
		{
//			NpArticulationLink* np = static_cast<NpArticulationLink*>(s);
		}
		else if(serialType==PxConcreteType::eARTICULATION_JOINT)
		{
//			NpArticulationJoint* np = static_cast<NpArticulationJoint*>(s);
		}
		else
		{
//			assert(0);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

#if PX_SUPPORT_GPU_PHYSX
void NpFactory::notifyReleaseTriangleMesh(const PxTriangleMesh& tm)
{
	NpPhysics::getInstance().getNpPhysicsGpu().releaseTriangleMeshMirror(tm);
}

void NpFactory::notifyReleaseHeightField(const PxHeightField& hf)
{
	NpPhysics::getInstance().getNpPhysicsGpu().releaseHeightFieldMirror(hf);
}

void NpFactory::notifyReleaseConvexMesh(const PxConvexMesh& cm)
{
	NpPhysics::getInstance().getNpPhysicsGpu().releaseConvexMeshMirror(cm);
}
#endif

#if PX_SUPPORT_PVD
void NpFactory::setNpFactoryListener( NpFactoryListener& inListener)
{
	mNpFactoryListener = &inListener;
	addFactoryListener(inListener);
}
#endif
///////////////////////////////////////////////////////////////////////////////

// these calls are issued from the Scb layer when buffered deletes are issued. 
// TODO: we should really push these down as a virtual interface that is part of Scb's reqs
// to eliminate this link-time dep.


static void NpDestroyRigidActor(Scb::RigidStatic& scb)
{
	NpRigidStatic* np = const_cast<NpRigidStatic*>(getNpRigidStatic(&scb));

	void* ud = np->userData;

	if(np->getBaseFlags() & PxBaseFlag::eOWNS_MEMORY)
		NpFactory::getInstance().releaseRigidStaticToPool(*np);
	else
		np->~NpRigidStatic();

	NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, ud);
}

static void NpDestroyRigidDynamic(Scb::Body& scb)
{
	NpRigidDynamic* np = const_cast<NpRigidDynamic*>(getNpRigidDynamic(&scb));

	void* ud = np->userData;
	if(np->getBaseFlags() & PxBaseFlag::eOWNS_MEMORY)
		NpFactory::getInstance().releaseRigidDynamicToPool(*np);
	else
		np->~NpRigidDynamic();
	NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, ud);
}

#if PX_USE_PARTICLE_SYSTEM_API
static void NpDestroyParticleSystem(Scb::ParticleSystem& scb)
{	
	if(scb.getActorType() == PxActorType::ePARTICLE_SYSTEM)
	{
		NpParticleSystem* np = const_cast<NpParticleSystem*>(getNpParticleSystem(&scb));

		void* ud = np->userData;
		if(np->getBaseFlags() & PxBaseFlag::eOWNS_MEMORY)
			NpFactory::getInstance().releaseParticleSystemToPool(*np);
		else
			np->~NpParticleSystem();	
		NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, ud);
	}
	else
	{
		NpParticleFluid* np = const_cast<NpParticleFluid*>(getNpParticleFluid(&scb));

		void* ud = np->userData;
		if(np->getBaseFlags() & PxBaseFlag::eOWNS_MEMORY)
			NpFactory::getInstance().releaseParticleFluidToPool(*np);
		else
			np->~NpParticleFluid();	
		NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, ud);
	}
}
#endif

static void NpDestroyArticulationLink(Scb::Body& scb)
{
	NpArticulationLink* np = const_cast<NpArticulationLink*>(getNpArticulationLink(&scb));

	void* ud = np->userData;
	if(np->getBaseFlags() & PxBaseFlag::eOWNS_MEMORY)
		NpFactory::getInstance().releaseArticulationLinkToPool(*np);
	else
		np->~NpArticulationLink();	
	NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, ud);
}

static void NpDestroyArticulationJoint(Scb::ArticulationJoint& scb)
{
	NpArticulationJoint* np = const_cast<NpArticulationJoint*>(getNpArticulationJoint(&scb));

	if(np->getBaseFlags() & PxBaseFlag::eOWNS_MEMORY)
		NpFactory::getInstance().releaseArticulationJointToPool(*np);
	else
		np->~NpArticulationJoint();	
	NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, NULL);
}

static void NpDestroyArticulation(Scb::Articulation& scb)
{
	NpArticulation* np = const_cast<NpArticulation*>(getNpArticulation(&scb));

	void* ud = np->userData;
	if(np->getBaseFlags() & PxBaseFlag::eOWNS_MEMORY)
		NpFactory::getInstance().releaseArticulationToPool(*np);
	else
		np->~NpArticulation();	
	NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, ud);
}

static void NpDestroyAggregate(Scb::Aggregate& scb)
{
	NpAggregate* np = const_cast<NpAggregate*>(getNpAggregate(&scb));

	if(np->getBaseFlags() & PxBaseFlag::eOWNS_MEMORY)
		NpFactory::getInstance().releaseAggregateToPool(*np);
	else
		np->~NpAggregate();

	NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, NULL);
}

static void NpDestroyShape(Scb::Shape& scb)
{
	NpShape* np = const_cast<NpShape*>(getNpShape(&scb));

	void* ud = np->userData;

	if(np->getBaseFlags() & PxBaseFlag::eOWNS_MEMORY)
		NpFactory::getInstance().releaseShapeToPool(*np);
	else
		np->~NpShape();

	NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, ud);
}

static void NpDestroyConstraint(Scb::Constraint& scb)
{
	const size_t offset = size_t(&(reinterpret_cast<NpConstraint*>(0)->getScbConstraint()));
	NpConstraint* np = reinterpret_cast<NpConstraint*>(reinterpret_cast<char*>(&scb)-offset);
	if(np->getBaseFlags() & PxBaseFlag::eOWNS_MEMORY)
		NpFactory::getInstance().releaseConstraintToPool(*np);
	else
		np->~NpConstraint();
	NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, NULL);
}

#if PX_USE_CLOTH_API
static void NpDestroyCloth(Scb::Cloth& scb)
{
	NpCloth* np = const_cast<NpCloth*>(getNpCloth(&scb));

	void* ud = np->userData;
	if(np->getBaseFlags() & PxBaseFlag::eOWNS_MEMORY)
		NpFactory::getInstance().releaseClothToPool(*np);
	else
		np->~NpCloth();
	NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, ud);
}
#endif

namespace physx
{
	void NpDestroy(Scb::Base& base)
	{
		switch(base.getScbType())
		{
			case ScbType::eSHAPE_EXCLUSIVE:
			case ScbType::eSHAPE_SHARED:				{ NpDestroyShape(static_cast<Scb::Shape&>(base));							}break;
			case ScbType::eBODY:						{ NpDestroyRigidDynamic(static_cast<Scb::Body&>(base));						}break;
			case ScbType::eBODY_FROM_ARTICULATION_LINK:	{ NpDestroyArticulationLink(static_cast<Scb::Body&>(base));					}break;
			case ScbType::eRIGID_STATIC:				{ NpDestroyRigidActor(static_cast<Scb::RigidStatic&>(base));				}break;
			case ScbType::eCONSTRAINT:					{ NpDestroyConstraint(static_cast<Scb::Constraint&>(base));					}break;
#if PX_USE_PARTICLE_SYSTEM_API
			case ScbType::ePARTICLE_SYSTEM:				{ NpDestroyParticleSystem(static_cast<Scb::ParticleSystem&>(base));			}break;
#endif
			case ScbType::eARTICULATION:				{ NpDestroyArticulation(static_cast<Scb::Articulation&>(base));				}break;
			case ScbType::eARTICULATION_JOINT:			{ NpDestroyArticulationJoint(static_cast<Scb::ArticulationJoint&>(base));	}break;
			case ScbType::eAGGREGATE:					{ NpDestroyAggregate(static_cast<Scb::Aggregate&>(base));					}break;
#if PX_USE_CLOTH_API
			case ScbType::eCLOTH:						{ NpDestroyCloth(static_cast<Scb::Cloth&>(base));							}break;
#endif
			case ScbType::eUNDEFINED:
			case ScbType::eTYPE_COUNT:
				PX_ALWAYS_ASSERT_MESSAGE("NpDestroy: missing type!");
				break;
		}
	}
}

