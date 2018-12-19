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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.



#include "MirrorSceneImpl.h"

#if PX_PHYSICS_VERSION_MAJOR == 3

#include "PxScene.h"
#include "PxRigidDynamic.h"
#include "PxMaterial.h"
#include "PxSphereGeometry.h"
#include "PxRigidDynamic.h"
#include "PxRigidStatic.h"
#include "PxShape.h"
#include "ApexSDKIntl.h"
#include "PsInlineArray.h"
#include "PxPhysics.h"

#pragma warning(disable:4100)

namespace nvidia
{

namespace apex
{

	using namespace physx;

bool copyStaticProperties(PxRigidActor& to, const PxRigidActor& from,MirrorScene::MirrorFilter &mirrorFilter)
{
	shdfnd::InlineArray<PxShape*, 64> shapes;
	shapes.resize(from.getNbShapes());

	uint32_t shapeCount = from.getNbShapes();
	from.getShapes(shapes.begin(), shapeCount);

	shdfnd::InlineArray<PxMaterial*, 64> materials;
	for(uint32_t i = 0; i < shapeCount; i++)
	{
		PxShape* s = shapes[i];

		if ( mirrorFilter.shouldMirror(*s) )
		{
			uint32_t materialCount = s->getNbMaterials();
			materials.resize(materialCount);
			s->getMaterials(materials.begin(), materialCount);
			PxShape* shape = to.createShape(s->getGeometry().any(), materials.begin(), static_cast<uint16_t>(materialCount));
			shape->setLocalPose( s->getLocalPose());
			shape->setContactOffset(s->getContactOffset());
			shape->setRestOffset(s->getRestOffset());
			shape->setFlags(s->getFlags());
			shape->setSimulationFilterData(s->getSimulationFilterData());
			shape->setQueryFilterData(s->getQueryFilterData());
			mirrorFilter.reviseMirrorShape(*shape);
		}
	}

	to.setActorFlags(from.getActorFlags());
	to.setOwnerClient(from.getOwnerClient());
	to.setDominanceGroup(from.getDominanceGroup());

	if ( to.getNbShapes() )
	{
		mirrorFilter.reviseMirrorActor(to);
	}

	return to.getNbShapes() != 0;
}

PxRigidStatic* CloneStatic(PxPhysics& physicsSDK, 
							 const PxTransform& transform, 
							 const PxRigidActor& from,
							 MirrorScene::MirrorFilter &mirrorFilter)
{
	PxRigidStatic* to = physicsSDK.createRigidStatic(transform);
	if(!to)
		return NULL;

	if ( !copyStaticProperties(*to, from,mirrorFilter) )
	{
		to->release();
		to = NULL;
	}

	return to;
}

PxRigidDynamic* CloneDynamic(PxPhysics& physicsSDK, 
							   const PxTransform& transform,
							   const PxRigidDynamic& from,
							   MirrorScene::MirrorFilter &mirrorFilter)
{
	PxRigidDynamic* to = physicsSDK.createRigidDynamic(transform);
	if(!to)
		return NULL;

	if ( !copyStaticProperties(*to, from, mirrorFilter) )
	{
		to->release();
		to = NULL;
		return NULL;
	}

	to->setRigidBodyFlags(from.getRigidBodyFlags());

	to->setMass(from.getMass());
	to->setMassSpaceInertiaTensor(from.getMassSpaceInertiaTensor());
	to->setCMassLocalPose(from.getCMassLocalPose());

	if ( !(to->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC) )
	{
		to->setLinearVelocity(from.getLinearVelocity());
		to->setAngularVelocity(from.getAngularVelocity());
	}

	to->setLinearDamping(from.getAngularDamping());
	to->setAngularDamping(from.getAngularDamping());

	to->setMaxAngularVelocity(from.getMaxAngularVelocity());

	uint32_t posIters, velIters;
	from.getSolverIterationCounts(posIters, velIters);
	to->setSolverIterationCounts(posIters, velIters);

	to->setSleepThreshold(from.getSleepThreshold());

	to->setContactReportThreshold(from.getContactReportThreshold());

	return to;
}


MirrorSceneImpl::MirrorSceneImpl(physx::PxScene &primaryScene,
	physx::PxScene &mirrorScene,
	MirrorScene::MirrorFilter &mirrorFilter,
	float mirrorStaticDistance,
	float mirrorDynamicDistance,
	float mirrorDistanceThreshold) 
	: mPrimaryScene(primaryScene)
	, mMirrorScene(mirrorScene)
	, mMirrorFilter(mirrorFilter)
	, mMirrorStaticDistance(mirrorStaticDistance)
	, mMirrorDynamicDistance(mirrorDynamicDistance)
	, mMirrorDistanceThreshold(mirrorDistanceThreshold*mirrorDistanceThreshold)
	, mTriggerActor(NULL)
	, mTriggerMaterial(NULL)
	, mTriggerShapeStatic(NULL)
	, mTriggerShapeDynamic(NULL)
	, mSimulationEventCallback(NULL)
{
	mLastCameraLocation = PxVec3(1e9,1e9,1e9);
	primaryScene.getPhysics().registerDeletionListener(*this,physx::PxDeletionEventFlag::eMEMORY_RELEASE | physx::PxDeletionEventFlag::eUSER_RELEASE);
}

MirrorSceneImpl::~MirrorSceneImpl(void)
{
	if ( mTriggerActor )
	{
		mPrimaryScene.lockWrite(__FILE__,__LINE__);
		mTriggerActor->release();
		mPrimaryScene.unlockWrite();
	}
	if ( mTriggerMaterial )
	{
		mTriggerMaterial->release();
	}
	mPrimaryScene.getPhysics().unregisterDeletionListener(*this);
}

void MirrorSceneImpl::createTriggerActor(const PxVec3 &cameraPosition)
{
	PX_ASSERT( mTriggerActor == NULL );
	mTriggerActor = mPrimaryScene.getPhysics().createRigidDynamic( PxTransform(cameraPosition) );
	PX_ASSERT(mTriggerActor);
	if ( mTriggerActor )
	{
		mTriggerActor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC,true);
		physx::PxSphereGeometry staticSphere;
		physx::PxSphereGeometry dynamicSphere;
		staticSphere.radius = mMirrorStaticDistance;
		dynamicSphere.radius = mMirrorDynamicDistance;
		mTriggerMaterial = mPrimaryScene.getPhysics().createMaterial(1,1,1);
		PX_ASSERT(mTriggerMaterial);
		if ( mTriggerMaterial )
		{
			mTriggerShapeStatic = mTriggerActor->createShape(staticSphere,*mTriggerMaterial);
			mTriggerShapeDynamic = mTriggerActor->createShape(dynamicSphere,*mTriggerMaterial);
			PX_ASSERT(mTriggerShapeStatic);
			PX_ASSERT(mTriggerShapeDynamic);
			if ( mTriggerShapeStatic && mTriggerShapeDynamic )
			{
				mPrimaryScene.lockWrite(__FILE__,__LINE__);

				mTriggerActor->setOwnerClient(0);
				mTriggerShapeStatic->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE,false);
				mTriggerShapeStatic->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE,false);
				mTriggerShapeStatic->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE,true);

				mTriggerShapeDynamic->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE,false);
				mTriggerShapeDynamic->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE,false);
				mTriggerShapeDynamic->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE,true);

				mSimulationEventCallback = mPrimaryScene.getSimulationEventCallback(); // get a copy of the original callback
				mPrimaryScene.setSimulationEventCallback(this,0);
				mPrimaryScene.addActor(*mTriggerActor);

				mPrimaryScene.unlockWrite();
			}
		}
	}
}

// Each frame, we do a shape query for static and dynamic objects
// If this is the first time the synchronize has been called, then we create
// a trigger actor with two spheres in the primary scene.  This trigger
// actor is used to detect when objects move in and outside of the static and dynamic
// mirror range specified.
void MirrorSceneImpl::synchronizePrimaryScene(const PxVec3 &cameraPos)
{
	PxVec3 diff = cameraPos - mLastCameraLocation;
	float dist = diff.magnitudeSquared();
	if ( dist > mMirrorDistanceThreshold )
	{
		mLastCameraLocation = cameraPos;
		if ( mTriggerActor == NULL )
		{
			createTriggerActor(cameraPos);	// Create the scene mirroring trigger actor 
		}
		if ( mTriggerActor )
		{
			mPrimaryScene.lockWrite(__FILE__,__LINE__);
			mTriggerActor->setKinematicTarget( PxTransform(cameraPos) );	// Update the position of the trigger actor to be the current camera location
			mPrimaryScene.unlockWrite();
		}
	}
	// Now, iterate on all of the current actors which are being mirrored
	// Only the primary scene after modifies this hash, so it is safe to do this
	// without any concerns of thread locking.
	// The mirrored scene thread does access the contents of this hash (MirrorActor)
	{
		mPrimaryScene.lockRead(__FILE__,__LINE__);
		for (ActorHash::Iterator i=mActors.getIterator(); !i.done(); ++i)
		{
			MirrorActor *ma = i->second;
			ma->synchronizePose();	// check to see if the position of this object in the primary
			// scene has changed.  If it has, then we create a command for the mirror scene to update
			// it's mirror actor to that new position.
		}
		mPrimaryScene.unlockRead();
	}
}

// When the mirrored scene is synchronized, we grab the mirror command buffer
// And then despool all of the create/release/update commands that got posted previously by the
// primary scene thread.  A mutex is used to safe brief access to the command buffer.
// A copy of the command buffer is made so that we only grab the mutex for the shorted period
// of time possible.
void MirrorSceneImpl::synchronizeMirrorScene(void)
{
	MirrorCommandArray temp;
	mMirrorCommandMutex.lock();
	temp = mMirrorCommands;
	mMirrorCommands.clear();
	mMirrorCommandMutex.unlock();
	if ( !temp.empty() )
	{
		mMirrorScene.lockWrite(__FILE__,__LINE__);
		for (uint32_t i=0; i<temp.size(); i++)
		{
			MirrorCommand &mc = temp[i];
			switch ( mc.mType )
			{
				case MCT_CREATE_ACTOR:
					{
						mc.mMirrorActor->createActor(mMirrorScene);
					}
					break;
				case MCT_RELEASE_ACTOR:
					{
						delete mc.mMirrorActor;
					}
					break;
				case MCT_UPDATE_POSE:
					{
						mc.mMirrorActor->updatePose(mc.mPose);
					}
					break;
				default:
					break;
			}
		}
		mMirrorScene.unlockWrite();
	}
}

void MirrorSceneImpl::release(void)
{
	delete this;
}


/**
\brief This is called when a breakable constraint breaks.

\note The user should not release the constraint shader inside this call!

\param[in] constraints - The constraints which have been broken.
\param[in] count       - The number of constraints

@see PxConstraint PxConstraintDesc.linearBreakForce PxConstraintDesc.angularBreakForce
*/
void MirrorSceneImpl::onConstraintBreak(PxConstraintInfo* constraints, uint32_t count)
{
	if ( mSimulationEventCallback )
	{
		mSimulationEventCallback->onConstraintBreak(constraints,count);
	}
}

/**
\brief This is called during PxScene::fetchResults with the actors which have just been woken up.

\note Only supported by rigid bodies yet.
\note Only called on actors for which the PxActorFlag eSEND_SLEEP_NOTIFIES has been set.

\param[in] actors - The actors which just woke up.
\param[in] count  - The number of actors

@see PxScene.setSimulationEventCallback() PxSceneDesc.simulationEventCallback PxActorFlag PxActor.setActorFlag()
*/
void MirrorSceneImpl::onWake(PxActor** actors, uint32_t count)
{
	if ( mSimulationEventCallback )
	{
		mSimulationEventCallback->onWake(actors,count);
	}
}

/**
\brief This is called during PxScene::fetchResults with the actors which have just been put to sleep.

\note Only supported by rigid bodies yet.
\note Only called on actors for which the PxActorFlag eSEND_SLEEP_NOTIFIES has been set.  

\param[in] actors - The actors which have just been put to sleep.
\param[in] count  - The number of actors

@see PxScene.setSimulationEventCallback() PxSceneDesc.simulationEventCallback PxActorFlag PxActor.setActorFlag()
*/
void MirrorSceneImpl::onSleep(PxActor** actors, uint32_t count)
{
	if ( mSimulationEventCallback )
	{
		mSimulationEventCallback->onSleep(actors,count);
	}

}

/**
\brief The user needs to implement this interface class in order to be notified when
certain contact events occur.

The method will be called for a pair of actors if one of the colliding shape pairs requested contact notification.
You request which events are reported using the filter shader/callback mechanism (see #PxSimulationFilterShader,
#PxSimulationFilterCallback, #PxPairFlag).

Do not keep references to the passed objects, as they will be 
invalid after this function returns.

\param[in] pairHeader Information on the two actors whose shapes triggered a contact report.
\param[in] pairs The contact pairs of two actors for which contact reports have been requested. See #PxContactPair.
\param[in] nbPairs The number of provided contact pairs.

@see PxScene.setSimulationEventCallback() PxSceneDesc.simulationEventCallback PxContactPair PxPairFlag PxSimulationFilterShader PxSimulationFilterCallback
*/
void MirrorSceneImpl::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, uint32_t nbPairs)
{
	if ( mSimulationEventCallback )
	{
		mSimulationEventCallback->onContact(pairHeader,pairs,nbPairs);
	}

}

/*
\brief This is called during PxScene::fetchResults with the current trigger pair events.

Shapes which have been marked as triggers using PxShapeFlag::eTRIGGER_SHAPE will send events
according to the pair flag specification in the filter shader (see #PxPairFlag, #PxSimulationFilterShader).

\param[in] pairs - The trigger pairs which caused events.
\param[in] count - The number of trigger pairs.

@see PxScene.setSimulationEventCallback() PxSceneDesc.simulationEventCallback PxPairFlag PxSimulationFilterShader PxShapeFlag PxShape.setFlag()
*/
void MirrorSceneImpl::onTrigger(PxTriggerPair* pairs, uint32_t count)
{
	mTriggerPairs.clear();
	for (uint32_t i=0; i<count; i++)
	{
		PxTriggerPair &tp = pairs[i];

		if ( ( tp.triggerShape == mTriggerShapeStatic ) || ( tp.triggerShape == mTriggerShapeDynamic ) )
		{
			if ( tp.flags & PxTriggerPairFlag::eREMOVED_SHAPE_OTHER ) // actor was deleted!
			{
				// handle shape release..
				mirrorShape(tp);
			}
			else
			{
				PxActor *actor = tp.otherActor;
				if( mMirrorFilter.shouldMirror(*actor) ) // let the application telll us whether this is an actor we want to mirror or not
				{
					if ( tp.triggerShape == mTriggerShapeStatic ) 
					{
						if ( actor->getType() == PxActorType::eRIGID_STATIC )
						{
							mirrorShape(tp);
						}
					}
					else if ( tp.triggerShape == mTriggerShapeDynamic )
					{
						if ( actor->getType() == PxActorType::eRIGID_DYNAMIC )
						{
							mirrorShape(tp);
						}
					}
				}
			}
		}
		else
		{
			mTriggerPairs.pushBack(tp);
		}
	}
	if ( !mTriggerPairs.empty() ) // If some of the triggers were for the application; then we pass them on
	{
		mSimulationEventCallback->onTrigger(&mTriggerPairs[0],mTriggerPairs.size());
	}
}

void MirrorSceneImpl::onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count)
{
	PX_UNUSED(bodyBuffer);
	PX_UNUSED(poseBuffer);
	PX_UNUSED(count);
}

void MirrorSceneImpl::mirrorShape(const PxTriggerPair &tp)
{
	size_t hash = (size_t)tp.otherShape;
	const ShapeHash::Entry *found = mShapes.find(hash);
	MirrorActor *ma = found ? found->second : NULL;
	if ( tp.flags & PxTriggerPairFlag::eREMOVED_SHAPE_OTHER )
	{
		if ( found )
		{
			bool kill = ma->removeShape();
			mShapes.erase(hash);
			if ( kill )
			{
				ma->release();
				mActors.erase( ma->mActorHash );
			}
		}
	}
	else if ( tp.status == PxPairFlag::eNOTIFY_TOUCH_FOUND )
	{
		PX_ASSERT( found == NULL );
		size_t actorHash = (size_t) &tp.otherActor;
		const ActorHash::Entry *foundActor = mActors.find(actorHash);
		if ( foundActor == NULL )
		{
			ma = PX_NEW(MirrorActor)(actorHash,*tp.otherActor,*this);
			mActors[actorHash] = ma;
		}
		else
		{
			ma = foundActor->second;
		}
		ma->addShape();
		mShapes[hash] = ma;
	}
	else if ( tp.status == PxPairFlag::eNOTIFY_TOUCH_LOST )
	{
		PX_ASSERT( found );
		if ( ma )
		{
			bool kill = ma->removeShape();
			mShapes.erase(hash);
			if ( kill )
			{
				mActors.erase( ma->mActorHash );
				ma->release();
 			}
		}
	}

}

void MirrorSceneImpl::postCommand(const MirrorCommand &mc)
{
	mMirrorCommandMutex.lock();
	mMirrorCommands.pushBack(mc);
	mMirrorCommandMutex.unlock();
}

MirrorActor::MirrorActor(size_t actorHash,
						 physx::PxRigidActor &actor,
						 MirrorSceneImpl &mirrorScene) : mMirrorScene(mirrorScene), mPrimaryActor(&actor), mActorHash(actorHash) 
{
	mReleasePosted = false;
	mMirrorActor = NULL;
	mShapeCount = 0;
	PxScene *scene = actor.getScene();
	PX_ASSERT(scene);
	if ( scene )
	{
		scene->lockWrite(__FILE__,__LINE__);
		mPrimaryGlobalPose = actor.getGlobalPose();
		PxPhysics *sdk = &scene->getPhysics();
		if ( actor.getType() == physx::PxActorType::eRIGID_STATIC )
		{
			mMirrorActor = CloneStatic(*sdk,actor.getGlobalPose(),actor, mirrorScene.getMirrorFilter());
		}
		else
		{
			physx::PxRigidDynamic *rd = static_cast< physx::PxRigidDynamic *>(&actor);
			mMirrorActor = CloneDynamic(*sdk,actor.getGlobalPose(),*rd, mirrorScene.getMirrorFilter());
			if ( mMirrorActor )
			{
				rd = static_cast< physx::PxRigidDynamic *>(mMirrorActor);
				rd->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC,true);
			}
		}
		scene->unlockWrite();
		if ( mMirrorActor )
		{
			MirrorCommand mc(MCT_CREATE_ACTOR,this);
			mMirrorScene.postCommand(mc);
		}
	}
}

MirrorActor::~MirrorActor(void)
{
	if ( mMirrorActor )
	{
		mMirrorActor->release();
	}
}

void MirrorActor::release(void)
{
	PX_ASSERT( mReleasePosted == false );
	if ( !mReleasePosted )
	{
		if ( mPrimaryActor )
		{
		}
		MirrorCommand mc(MCT_RELEASE_ACTOR,this);
		mMirrorScene.postCommand(mc);
		mReleasePosted = true;
	}
}

void MirrorActor::createActor(PxScene &scene)
{
	if ( mMirrorActor )
	{
		scene.addActor(*mMirrorActor);
	}
}

static bool sameTransform(const PxTransform &a,const PxTransform &b)
{
	if ( a.p == b.p && 
		 a.q.x == b.q.x && 
		 a.q.y == b.q.y &&
		 a.q.z == b.q.z &&
		 a.q.w == b.q.w )
	{
		return true;
	}
	return false;
}

void MirrorActor::synchronizePose(void)
{
	if ( mPrimaryActor )
	{
		PxTransform p = mPrimaryActor->getGlobalPose();
		if ( !sameTransform(p,mPrimaryGlobalPose) )
		{
			mPrimaryGlobalPose = p;
			MirrorCommand mc(MCT_UPDATE_POSE,this,p);
			mMirrorScene.postCommand(mc);
		}
	}
}

void MirrorActor::updatePose(const PxTransform &pose)
{
	if ( mMirrorActor )
	{
		if ( mMirrorActor->getType() == PxActorType::eRIGID_STATIC )
		{
			PxRigidStatic *p = static_cast< PxRigidStatic *>(mMirrorActor);
			p->setGlobalPose(pose);
		}
		else
		{
			PxRigidDynamic *p = static_cast< PxRigidDynamic *>(mMirrorActor);
			p->setKinematicTarget(pose);
		}
	}
}

void MirrorSceneImpl::onRelease(const PxBase* observed,
					   void* /*userData*/,
					   PxDeletionEventFlag::Enum /*deletionEvent*/)
{
	const physx::PxRigidActor *a = observed->is<PxRigidActor>();
	if ( a )
	{
		size_t actorHash = (size_t)a;
		const ActorHash::Entry *foundActor = mActors.find(actorHash);
		if ( foundActor != NULL )
		{
			MirrorActor *ma = foundActor->second;
			ma->mPrimaryActor = NULL;
		}
	}
}

}; // end apex namespace
}; // end physx namespace

#endif
