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

#include "ScBodyCore.h"
#include "ScBodySim.h"
#include "ScPhysics.h"
#include "ScScene.h"
#include "PxsSimulationController.h"
#include "PsFoundation.h"

using namespace physx;

Sc::BodyCore::BodyCore(PxActorType::Enum type, const PxTransform& bodyPose) 
:	RigidCore(type)
{
	const PxTolerancesScale& scale = Physics::getInstance().getTolerancesScale();
	// sizeof(BodyCore) = 176 => 160 => 144 => 160 bytes

	mCore.wakeCounter = Sc::Physics::sWakeCounterOnCreation;
	mCore.inverseInertia			= PxVec3(1.f,1.f,1.f);
	mCore.inverseMass				= 1.0f;
	mCore.body2World				= bodyPose;

	PX_ASSERT(mCore.body2World.p.isFinite());
	PX_ASSERT(mCore.body2World.q.isFinite());

	mCore.sleepThreshold			= 5e-5f * scale.speed * scale.speed;
	mCore.freezeThreshold			= 2.5e-5f * scale.speed * scale.speed;
	mSimStateData					= NULL;
	mCore.maxPenBias				= -1e32f;//-PX_MAX_F32;
	mCore.mFlags					= PxRigidBodyFlags();
	mCore.linearVelocity			= PxVec3(0.0f);
	mCore.angularVelocity			= PxVec3(0.0f);
	mCore.linearDamping				= 0.0f;
	mCore.maxLinearVelocitySq		= PX_MAX_F32;
	mCore.solverIterationCounts		= (1 << 8) | 4;	
	mCore.contactReportThreshold	= PX_MAX_F32;	
	mCore.setBody2Actor(PxTransform(PxIdentity));
	mCore.ccdAdvanceCoefficient		= 0.15f;
	mCore.maxContactImpulse			= 1e32f;// PX_MAX_F32;
	mCore.isFastMoving				= false;
	mCore.lockFlags					= PxRigidDynamicLockFlags(0);

	if(type == PxActorType::eRIGID_DYNAMIC)
	{
		mCore.angularDamping			= 0.05f;
		mCore.maxAngularVelocitySq		= 7.0f * 7.0f;
	}
	else
	{
		mCore.angularDamping			= 0.0f;
		mCore.maxAngularVelocitySq		= PX_MAX_F32;
	}
}

Sc::BodyCore::~BodyCore()
{
	PX_ASSERT(getSim() == 0);
	PX_ASSERT(!mSimStateData);
}

Sc::BodySim* Sc::BodyCore::getSim() const
{
	return static_cast<BodySim*>(Sc::ActorCore::getSim());
}

size_t Sc::BodyCore::getSerialCore(PxsBodyCore& serialCore)
{
	serialCore = mCore;
	if(mSimStateData && mSimStateData->isKine())
	{	
		const Kinematic* kine			= mSimStateData->getKinematicData();
		serialCore.inverseMass			= kine->backupInvMass;
		serialCore.inverseInertia		= kine->backupInverseInertia;
		serialCore.linearDamping		= kine->backupLinearDamping;
		serialCore.angularDamping		= kine->backupAngularDamping;
		serialCore.maxAngularVelocitySq	= kine->backupMaxAngVelSq;
		serialCore.maxLinearVelocitySq	= kine->backupMaxLinVelSq;
	}
	return reinterpret_cast<size_t>(&mCore);
}

//--------------------------------------------------------------
//
// BodyCore interface implementation
//
//--------------------------------------------------------------

void Sc::BodyCore::setBody2World(const PxTransform& p)
{
	mCore.body2World = p;
	PX_ASSERT(p.p.isFinite());
	PX_ASSERT(p.q.isFinite());

	BodySim* sim = getSim();
	if (sim)
	{
		sim->postBody2WorldChange();
		//update bodysim
		Sc::Scene& scene = sim->getScene();
		scene.getSimulationController()->addDynamic(&sim->getLowLevelBody(), sim->getNodeIndex().index());
	}
}

void Sc::BodyCore::setLinearVelocity(const PxVec3& v)
{
	mCore.linearVelocity = v;

	BodySim* sim = getSim();
	if (sim)
	{
		//update bodysim
		Sc::Scene& scene = sim->getScene();
		scene.getSimulationController()->addDynamic(&sim->getLowLevelBody(), sim->getNodeIndex().index());
	}
}

void Sc::BodyCore::setAngularVelocity(const PxVec3& v)
{
	mCore.angularVelocity = v;

	BodySim* sim = getSim();
	if (sim)
	{
		//update bodysim
		Sc::Scene& scene = sim->getScene();
		scene.getSimulationController()->addDynamic(&sim->getLowLevelBody(), sim->getNodeIndex().index());
	}
}

void Sc::BodyCore::setBody2Actor(const PxTransform& p)
{
	PX_ASSERT(p.p.isFinite());
	PX_ASSERT(p.q.isFinite());

	mCore.setBody2Actor(p);

	BodySim* sim = getSim();
	if (sim)
	{
		sim->notifyShapesOfTransformChange();
		//update bodysim
		Sc::Scene& scene = sim->getScene();
		scene.getSimulationController()->addDynamic(&sim->getLowLevelBody(), sim->getNodeIndex().index());
	}
}


void Sc::BodyCore::addSpatialAcceleration(Ps::Pool<SimStateData>* simStateDataPool, const PxVec3* linAcc, const PxVec3* angAcc)
{
	//The dirty flag is stored separately in the BodySim so that we query the dirty flag before going to 
	//the expense of querying the simStateData for the velmod values.
	BodySim* sim = getSim();
	if(sim)
		sim->notifyAddSpatialAcceleration();

	if(!mSimStateData || !mSimStateData->isVelMod())
		setupSimStateData(simStateDataPool, false);

	VelocityMod* velmod = mSimStateData->getVelocityModData();
	velmod->notifyAddAcceleration();
	if(linAcc) velmod->accumulateLinearVelModPerSec(*linAcc);
	if(angAcc) velmod->accumulateAngularVelModPerSec(*angAcc);
}

void Sc::BodyCore::clearSpatialAcceleration(bool force, bool torque)
{
	PX_ASSERT(force || torque);

	//The dirty flag is stored separately in the BodySim so that we query the dirty flag before going to 
	//the expense of querying the simStateData for the velmod values.
	BodySim* sim = getSim();
	if(sim)
		sim->notifyClearSpatialAcceleration();

	if(mSimStateData)
	{
		PX_ASSERT(mSimStateData->isVelMod());
		VelocityMod* velmod = mSimStateData->getVelocityModData();
		velmod->notifyClearAcceleration();
		if(force)
			velmod->clearLinearVelModPerSec();
		if(torque)
			velmod->clearAngularVelModPerSec();
	}
}

void Sc::BodyCore::addSpatialVelocity(Ps::Pool<SimStateData>* simStateDataPool, const PxVec3* linVelDelta, const PxVec3* angVelDelta)
{
	//The dirty flag is stored separately in the BodySim so that we query the dirty flag before going to 
	//the expense of querying the simStateData for the velmod values.
	BodySim* sim = getSim();
	if(sim)
		sim->notifyAddSpatialVelocity();

	if(!mSimStateData || !mSimStateData->isVelMod())
		setupSimStateData(simStateDataPool, false);

	VelocityMod* velmod = mSimStateData->getVelocityModData();
	velmod->notifyAddVelocity();
	if(linVelDelta)
		velmod->accumulateLinearVelModPerStep(*linVelDelta);
	if(angVelDelta)
		velmod->accumulateAngularVelModPerStep(*angVelDelta);
}

void Sc::BodyCore::clearSpatialVelocity(bool force, bool torque)
{
	PX_ASSERT(force || torque);

	//The dirty flag is stored separately in the BodySim so that we query the dirty flag before going to 
	//the expense of querying the simStateData for the velmod values.
	BodySim* sim = getSim();
	if(sim)
		sim->notifyClearSpatialVelocity();

	if(mSimStateData)
	{
		PX_ASSERT(mSimStateData->isVelMod());
		VelocityMod* velmod = mSimStateData->getVelocityModData();
		velmod->notifyClearVelocity();
		if(force)
			velmod->clearLinearVelModPerStep();
		if(torque)
			velmod->clearAngularVelModPerStep();
	}
}

PxReal Sc::BodyCore::getInverseMass() const
{
	return mSimStateData && mSimStateData->isKine() ? mSimStateData->getKinematicData()->backupInvMass : mCore.inverseMass;
}

void Sc::BodyCore::setInverseMass(PxReal m)
{
	if(mSimStateData && mSimStateData->isKine())
		mSimStateData->getKinematicData()->backupInvMass = m;
	else
	{
		mCore.inverseMass = m;
		BodySim* sim = getSim();
		if (sim)
		{
			//update bodysim
			Sc::Scene& scene = sim->getScene();
			scene.getSimulationController()->addDynamic(&sim->getLowLevelBody(), sim->getNodeIndex().index());
		}
	}
}

const PxVec3& Sc::BodyCore::getInverseInertia() const
{
	return mSimStateData && mSimStateData->isKine() ? mSimStateData->getKinematicData()->backupInverseInertia : mCore.inverseInertia;
}

void Sc::BodyCore::setInverseInertia(const PxVec3& i)
{
	if(mSimStateData && mSimStateData->isKine())
		mSimStateData->getKinematicData()->backupInverseInertia = i;
	else
	{
		mCore.inverseInertia = i;
		BodySim* sim = getSim();
		if (sim)
		{
			//update bodysim
			Sc::Scene& scene = sim->getScene();
			scene.getSimulationController()->addDynamic(&sim->getLowLevelBody(), sim->getNodeIndex().index());
		}
	}
}

PxReal Sc::BodyCore::getLinearDamping() const
{
	return mSimStateData && mSimStateData->isKine() ? mSimStateData->getKinematicData()->backupLinearDamping : mCore.linearDamping;
}

void Sc::BodyCore::setLinearDamping(PxReal d)
{
	if(mSimStateData && mSimStateData->isKine())
		mSimStateData->getKinematicData()->backupLinearDamping = d;
	else
	{
		mCore.linearDamping = d;
		BodySim* sim = getSim();
		if (sim)
		{
			//update bodysim
			Sc::Scene& scene = sim->getScene();
			scene.getSimulationController()->addDynamic(&sim->getLowLevelBody(), sim->getNodeIndex().index());
		}
	}
}

PxReal Sc::BodyCore::getAngularDamping() const
{
	return mSimStateData && mSimStateData->isKine() ? mSimStateData->getKinematicData()->backupAngularDamping : mCore.angularDamping;
}

void Sc::BodyCore::setAngularDamping(PxReal v)
{
	if(mSimStateData && mSimStateData->isKine())
		mSimStateData->getKinematicData()->backupAngularDamping = v;
	else
	{
		mCore.angularDamping = v;
		BodySim* sim = getSim();
		if (sim)
		{
			//update bodysim
			Sc::Scene& scene = sim->getScene();
			scene.getSimulationController()->addDynamic(&sim->getLowLevelBody(), sim->getNodeIndex().index());
		}
	}
}

PxReal Sc::BodyCore::getMaxAngVelSq() const
{
	return mSimStateData && mSimStateData->isKine() ? mSimStateData->getKinematicData()->backupMaxAngVelSq : mCore.maxAngularVelocitySq;
}

void Sc::BodyCore::setMaxAngVelSq(PxReal v)
{
	if(mSimStateData && mSimStateData->isKine())
		mSimStateData->getKinematicData()->backupMaxAngVelSq = v;
	else
	{
		mCore.maxAngularVelocitySq = v;
		BodySim* sim = getSim();
		if (sim)
		{
			//update bodysim
			Sc::Scene& scene = sim->getScene();
			scene.getSimulationController()->addDynamic(&sim->getLowLevelBody(), sim->getNodeIndex().index());
		}
	}
}

void Sc::BodyCore::setFlags(Ps::Pool<SimStateData>* simStateDataPool, PxRigidBodyFlags f)
{
	PxRigidBodyFlags old = mCore.mFlags;
	if(f != old)
	{
		PxU32 wasKinematic = old & PxRigidBodyFlag::eKINEMATIC;
		PxU32 isKinematic = f & PxRigidBodyFlag::eKINEMATIC;
		bool switchToKinematic = ((!wasKinematic) && isKinematic);
		bool switchToDynamic = (wasKinematic && (!isKinematic));

		mCore.mFlags = f;
		BodySim* sim = getSim();
		if (sim)
		{
			PX_ASSERT(simStateDataPool);

			const PxU32 posePreviewFlag = f & PxRigidBodyFlag::eENABLE_POSE_INTEGRATION_PREVIEW;
			if (PxU32(old & PxRigidBodyFlag::eENABLE_POSE_INTEGRATION_PREVIEW) != posePreviewFlag)
				sim->postPosePreviewChange(posePreviewFlag);

			// for those who might wonder about the complexity here:
			// our current behavior is that you are not allowed to set a kinematic target unless the object is in a scene.
			// Thus, the kinematic data should only be created/destroyed when we know for sure that we are in a scene.

			if (switchToKinematic)
			{
				setupSimStateData(simStateDataPool, true, false);
				sim->postSwitchToKinematic();
			}
			else if (switchToDynamic)
			{
				tearDownSimStateData(simStateDataPool, true);
				sim->postSwitchToDynamic();
			}

			PxU32 wasSpeculativeCCD = old & PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD;
			PxU32 isSpeculativeCCD = f & PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD;

			if (wasSpeculativeCCD ^ isSpeculativeCCD)
			{
				if (wasSpeculativeCCD)
				{
					if (sim->isArticulationLink())
						sim->getScene().resetSpeculativeCCDArticulationLink(sim->getNodeIndex().index());
					else
						sim->getScene().resetSpeculativeCCDRigidBody(sim->getNodeIndex().index());

					sim->getLowLevelBody().mInternalFlags &= (~PxcRigidBody::eSPECULATIVE_CCD);
				}
				else
				{
					if (sim->isArticulationLink())
						sim->getScene().setSpeculativeCCDArticulationLink(sim->getNodeIndex().index());
					else
						sim->getScene().setSpeculativeCCDRigidBody(sim->getNodeIndex().index());

					sim->getLowLevelBody().mInternalFlags |= (PxcRigidBody::eSPECULATIVE_CCD);
				}
			}			
		}

		if (switchToKinematic)
			putToSleep();

		if(sim)
		{
			PxRigidBodyFlags ktFlags(PxRigidBodyFlag::eUSE_KINEMATIC_TARGET_FOR_SCENE_QUERIES | PxRigidBodyFlag::eKINEMATIC);
			bool hadKt = (old & ktFlags) == ktFlags;
			bool hasKt = (f & ktFlags) == ktFlags;
			if(hasKt && !hadKt)
				sim->destroySqBounds();
			else if(hadKt && !hasKt)
				sim->createSqBounds();
		}
	}
}

void Sc::BodyCore::setMaxContactImpulse(PxReal m)	
{ 
	mCore.maxContactImpulse = m; 
	BodySim* sim = getSim();
	if (sim)
	{
		//maxContactImpulse changed, need to trigger dma pxgbodysim data again
		Sc::Scene& scene = sim->getScene();
		scene.getSimulationController()->addDynamic(&sim->getLowLevelBody(), sim->getNodeIndex().index());
	}
}

void Sc::BodyCore::setWakeCounter(PxReal wakeCounter, bool forceWakeUp)
{
	mCore.wakeCounter = wakeCounter;
	BodySim* sim = getSim();
	if(sim)
	{
		//wake counter change, we need to trigger dma pxgbodysim data again
		Sc::Scene& scene = sim->getScene();
		scene.getSimulationController()->addDynamic(&sim->getLowLevelBody(), sim->getNodeIndex().index());
		if ((wakeCounter > 0.0f) || forceWakeUp)
			sim->wakeUp();
		sim->postSetWakeCounter(wakeCounter, forceWakeUp);
	}
}

void Sc::BodyCore::setSleepThreshold(PxReal t)
{
	mCore.sleepThreshold = t;
	BodySim* sim = getSim();
	if(sim)
	{
		//update bodysim
		Sc::Scene& scene = sim->getScene();
		scene.getSimulationController()->addDynamic(&sim->getLowLevelBody(), sim->getNodeIndex().index());
	}
}

void Sc::BodyCore::setFreezeThreshold(PxReal t)
{
	mCore.freezeThreshold = t;
	BodySim* sim = getSim();
	if(sim)
	{
		//update bodysim
		Sc::Scene& scene = sim->getScene();
		scene.getSimulationController()->addDynamic(&sim->getLowLevelBody(), sim->getNodeIndex().index());
	}
}

bool Sc::BodyCore::isSleeping() const
{
	BodySim* sim = getSim();
	return sim ? !sim->isActive() : true;
}

void Sc::BodyCore::putToSleep()
{
	mCore.linearVelocity = PxVec3(0.f);
	mCore.angularVelocity = PxVec3(0.0f);

	//The dirty flag is stored separately in the BodySim so that we query the dirty flag before going to 
	//the expense of querying the simStateData for the velmod values.
	BodySim* sim = getSim();
	if (sim)
	{
		sim->notifyClearSpatialAcceleration();
		sim->notifyClearSpatialVelocity();
	}

	//The velmod data is stored in a separate structure so we can record forces before scene insertion.
	if(mSimStateData && mSimStateData->isVelMod())
	{
		VelocityMod* velmod = mSimStateData->getVelocityModData();
		velmod->clear();
	}

	// important to clear all values before setting the wake counter because the values decide
	// whether an object is ready to go to sleep or not.
	setWakeCounter(0.0f);

	if (sim)
		sim->putToSleep();
}

void Sc::BodyCore::setKinematicTarget(Ps::Pool<SimStateData>* simStateDataPool, const PxTransform& p, PxReal wakeCounter)
{
	PX_ASSERT(mCore.mFlags & PxRigidBodyFlag::eKINEMATIC);
	PX_ASSERT(!mSimStateData || mSimStateData->isKine());

	if(mSimStateData)
	{
		Kinematic* kine = mSimStateData->getKinematicData();
		kine->targetPose = p;
		kine->targetValid = 1;
		
		Sc::BodySim* bSim = getSim();
		if (bSim)
			bSim->postSetKinematicTarget();
	}
	else
	{
		bool success = setupSimStateData(simStateDataPool, true, true);
		if (success)
		{
			PX_ASSERT(!getSim());  // covers the following scenario: kinematic gets added to scene while sim is running and target gets set (at that point the sim object does not yet exist)

			Kinematic* kine = mSimStateData->getKinematicData();
			kine->targetPose = p;
			kine->targetValid = 1;
		}
		else
			Ps::getFoundation().error(PxErrorCode::eOUT_OF_MEMORY, __FILE__, __LINE__, "PxRigidDynamic: setting kinematic target failed, not enough memory.");
	}

	wakeUp(wakeCounter);
}

void Sc::BodyCore::disableInternalCaching(bool disable)
{
	PX_ASSERT(!mSimStateData || mSimStateData->isKine());

	if(mSimStateData)
	{
		PX_ASSERT(getFlags() & PxRigidBodyFlag::eKINEMATIC);
		
		if(disable)
			restore();
		else
			backup(*mSimStateData);
	}
}

bool Sc::BodyCore::setupSimStateData(Ps::Pool<SimStateData>* simStateDataPool, const bool isKinematic, const bool targetValid)
{
	SimStateData* data = mSimStateData;
	if(!data)
	{
		data = simStateDataPool->construct();
		if(!data)
			return false;
	}

	if(isKinematic)
	{
		PX_ASSERT(!mSimStateData || !mSimStateData->isKine());

		new(data) SimStateData(SimStateData::eKine);
		Kinematic* kine = data->getKinematicData();
		kine->targetValid = PxU8(targetValid ? 1 : 0);
		backup(*data);
	}
	else
	{
		PX_ASSERT(!mSimStateData || !mSimStateData->isVelMod());
		PX_ASSERT(!targetValid);

		new(data) SimStateData(SimStateData::eVelMod);
		VelocityMod* velmod = data->getVelocityModData();
		velmod->clear();
		velmod->flags = 0;
	}
	mSimStateData = data;
	return true;
}

bool Sc::BodyCore::checkSimStateKinematicStatus(const bool isKinematic) const 
{ 
	PX_ASSERT(mSimStateData);
	return mSimStateData->isKine() == isKinematic;
}

void Sc::BodyCore::tearDownSimStateData(Ps::Pool<SimStateData>* simStateDataPool, const bool isKinematic)
{
	PX_ASSERT(!mSimStateData || mSimStateData->isKine() == isKinematic);

	if (mSimStateData)
	{
		if(isKinematic)
		{
			restore();
		}
		simStateDataPool->destroy(mSimStateData);
		mSimStateData=NULL;
	}
}

void Sc::BodyCore::backup(SimStateData& b)
{
	PX_ASSERT(b.isKine());

	Kinematic* kine = b.getKinematicData();
	kine->backupLinearDamping	= mCore.linearDamping;
	kine->backupAngularDamping	= mCore.angularDamping;
	kine->backupInverseInertia	= mCore.inverseInertia;
	kine->backupInvMass			= mCore.inverseMass;
	kine->backupMaxAngVelSq		= mCore.maxAngularVelocitySq;
	kine->backupMaxLinVelSq		= mCore.maxLinearVelocitySq;

	mCore.inverseMass			= 0.0f;
	mCore.inverseInertia		= PxVec3(0.0f);
	mCore.linearDamping			= 0.0f;
	mCore.angularDamping		= 0.0f;
	mCore.maxAngularVelocitySq	= PX_MAX_REAL;
	mCore.maxLinearVelocitySq	= PX_MAX_REAL;
}

void Sc::BodyCore::restore()
{
	PX_ASSERT(mSimStateData && mSimStateData->isKine());

	const Kinematic* kine		= mSimStateData->getKinematicData();
	mCore.inverseMass			= kine->backupInvMass;
	mCore.inverseInertia		= kine->backupInverseInertia;
	mCore.linearDamping			= kine->backupLinearDamping;
	mCore.angularDamping		= kine->backupAngularDamping;
	mCore.maxAngularVelocitySq	= kine->backupMaxAngVelSq;
	mCore.maxLinearVelocitySq	= kine->backupMaxLinVelSq;
}

void Sc::BodyCore::invalidateKinematicTarget()
{ 
	PX_ASSERT(mSimStateData && mSimStateData->isKine()); 
	mSimStateData->getKinematicData()->targetValid = 0; 
}

void Sc::BodyCore::onOriginShift(const PxVec3& shift)
{
	mCore.body2World.p -= shift;
	if (mSimStateData && (getFlags() & PxRigidBodyFlag::eKINEMATIC) && mSimStateData->getKinematicData()->targetValid)
	{
		mSimStateData->getKinematicData()->targetPose.p -= shift;
	}

	BodySim* b = getSim();
	if (b)
		b->onOriginShift(shift);  // BodySim might not exist if actor has simulation disabled (PxActorFlag::eDISABLE_SIMULATION)
}

// PT: TODO: isn't that the same as BodyCore->getPxActor() now?
PxActor* Sc::getPxActorFromBodyCore(Sc::BodyCore* r, PxActorType::Enum& type)
{
	const PxActorType::Enum actorCoretype = r->getActorCoreType();
	type = actorCoretype;
	return Ps::pointerOffset<PxActor*>(r, Sc::gOffsetTable.scCore2PxActor[actorCoretype]);
}

Ps::IntBool Sc::BodyCore::isFrozen() const
{
	return getSim()->isFrozen();
}

void Sc::BodyCore::setFrozen()
{
	getSim()->setFrozen();
}

void Sc::BodyCore::clearFrozen()
{
	getSim()->clearFrozen();
}

void Sc::BodyCore::setSolverIterationCounts(PxU16 c)	
{ 
	mCore.solverIterationCounts = c;	
	if (getSim())
		getSim()->getLowLevelBody().solverIterationCounts = c;
}
