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



#include "Apex.h"

#include "ImpactEmitterAsset.h"
#include "ImpactEmitterActor.h"
#include "SceneIntl.h"
#include "RenderDebugInterface.h"
#include "InstancedObjectSimulationIntl.h"
#include "ImpactEmitterAssetImpl.h"
#include "ImpactEmitterActorImpl.h"
#include "EmitterScene.h"
#include "IofxAsset.h"

#include "nvparameterized/NvParameterized.h"
#include "nvparameterized/NvParamUtils.h"

#include "ImpactObjectEvent.h"
#include "ParamArray.h"
#include "PsMathUtils.h"

#include "ReadCheck.h"
#include "WriteCheck.h"

////////////////////////////////////////////////////////////////////////////////

#define PI (3.1415926535897932384626433832795f)

////////////////////////////////////////////////////////////////////////////////
namespace nvidia
{
namespace emitter
{

bool ImpactEmitterActorImpl::QueuedImpactEvent::trigger(float time)
{
	if (time > triggerTime)
	{
		eventDef->trigger(triggerParams);
		return true;
	}
	else
	{
		return false;
	}
}

////////////////////////////////////////////////////////////////////////////////
class ParticleEventTask : public PxTask, public UserAllocated
{
public:
	ParticleEventTask(ImpactEmitterParticleEvent* owner) : mOwner(owner) {}

	const char* getName() const
	{
		return "ImpactEmitterActorImpl::ParticleEventTask";
	}
	void run()
	{
		mOwner->run();
	}

protected:
	ImpactEmitterParticleEvent* mOwner;
};


void ImpactEmitterParticleEvent::trigger(const ImpactEventTriggerParams& params)
{
	if (mEventTask != NULL)
	{
		mTriggerQueue.pushBack(params);
	}
}

void ImpactEmitterParticleEvent::run()
{
	for (TriggerQueue::ConstIterator it = mTriggerQueue.begin(); it != mTriggerQueue.end(); ++it)
	{
		const ImpactEventTriggerParams& params = *it;
		trigger(params.hitPos, params.hitDir, params.hitNorm);
	}
	mTriggerQueue.clear();
}

void ImpactEmitterParticleEvent::submitTasks(PxTaskManager* tm)
{
	if (mEventTask != NULL && mTriggerQueue.size() > 0)
	{
		tm->submitUnnamedTask(*mEventTask);
	}
}

void ImpactEmitterParticleEvent::setTaskDependencies(PxTask*)
{
	if (mEventTask != NULL && mTriggerQueue.size() > 0)
	{
		//mEventTask->startAfter( tickTask->getTaskID() );
		mEventTask->finishBefore(mParticleInjector->getCompletionTaskID());
	}
}

ImpactEmitterParticleEvent::ImpactEmitterParticleEvent(NvParameterized::Interface* eventParamPtr, ImpactEmitterAssetImpl& asset, ImpactEmitterActorImpl* emitterActor): 
	ImpactEmitterBaseEvent(emitterActor),
	mParticleInjector(NULL)

{
	PX_ASSERT(eventParamPtr);
	PX_ASSERT(!nvidia::strcmp(eventParamPtr->className(), ImpactObjectEvent::staticClassName()));

	mEventTask = NULL;

	ImpactObjectEvent* eventPtr = (ImpactObjectEvent*)eventParamPtr;
	NvParameterized::Handle hEnum(*eventPtr);

	mAxis = AXIS_REFLECTION;
	eventPtr->getParameterHandle("impactAxis", hEnum);
	PX_ASSERT(hEnum.isValid());

	// this assumes that the enums line up correctly, they do
	int32_t axisInt = hEnum.parameterDefinition()->enumValIndex(eventPtr->impactAxis);
	if (-1 != axisInt)
	{
		mAxis = (eAxisType)axisInt;
	}

	mMinAngle = physx::shdfnd::degToRad(eventPtr->parameters().angleLow);
	mMaxAngle = physx::shdfnd::degToRad(eventPtr->parameters().angleHigh);
	mMinSpeed = eventPtr->parameters().speedLow;
	mMaxSpeed = eventPtr->parameters().speedHigh;
	mMinLife = eventPtr->parameters().lifeLow;
	mMaxLife = eventPtr->parameters().lifeHigh;
	mParticleSpawnCount = eventPtr->parameters().particleSpawnCount;


	const char* iofxAssetName = eventPtr->parameters().iofxAssetName->name();
	IofxAsset* iofxAsset = static_cast<IofxAsset*>(asset.mIofxAssetTracker.getAssetFromName(iofxAssetName));
	IosAsset* iosAsset = asset.mIosAssetTracker.getIosAssetFromName(eventPtr->parameters().iosAssetName->className(),
	                       eventPtr->parameters().iosAssetName->name());
	if (!iosAsset || !iofxAsset)
	{
		return;
	}

	Actor* nxActor = iosAsset->createIosActor(*(mOwner->getApexScene()), iofxAsset);
	InstancedObjectSimulationIntl* iosActor = NULL;
	if (nxActor)
	{
		ApexActor* aa = GetInternalApexSDK()->getApexActor(nxActor);
		if (aa)
		{
			iosActor = DYNAMIC_CAST(InstancedObjectSimulationIntl*)(aa);
		}
	}
	if (!iosActor)
	{
		return;
	}

	mParticleInjector = iosActor->allocateInjector(iofxAsset);
	if (mParticleInjector)
	{
		mParticleInjector->setLODWeights(	eventPtr->parameters().lodParamDesc.maxDistance,
											eventPtr->parameters().lodParamDesc.distanceWeight,
											eventPtr->parameters().lodParamDesc.speedWeight,
											eventPtr->parameters().lodParamDesc.lifeWeight,
											eventPtr->parameters().lodParamDesc.separationWeight,
											eventPtr->parameters().lodParamDesc.bias);

		mParticleInjector->addSelfToContext(*mOwner);
		mEventTask = PX_NEW(ParticleEventTask)(this);
		PX_ASSERT(mEventTask);

		mOwner->mActiveParticleInjectors.pushBack(mParticleInjector);
	}
	else
	{
		return;
	}

	mValid = true;
}

ImpactEmitterParticleEvent::~ImpactEmitterParticleEvent()
{
	if (mEventTask)
	{
		delete mEventTask;
	}
	mEventTask = NULL;

	if (mParticleInjector)
	{
		mOwner->mActiveParticleInjectors.findAndReplaceWithLast(mParticleInjector);

		mParticleInjector->release();
	}
	mParticleInjector = NULL;
}

void ImpactEmitterParticleEvent::trigger(const PxVec3& hitPos, const PxVec3& hitDir, const PxVec3& hitNorm)
{
	if (!mParticleInjector)
	{
		return;
	}

	PxVec3 eventBasis[3];
	if (mAxis == AXIS_INCIDENT)
	{
		eventBasis[0] = -hitDir;
	}
	else if (mAxis == AXIS_NORMAL)
	{
		eventBasis[0] = hitNorm;
	}
	else if (mAxis == AXIS_REFLECTION)
	{
		// this is also found in PxRay::ComputeReflexionVector()
		eventBasis[0] = hitDir - (2 * hitDir.dot(hitNorm) * hitNorm);
	}
	else
	{
		// Error
		return;
	}
	eventBasis[0].normalize();
	eventBasis[1] = PxVec3(eventBasis[0].y, eventBasis[0].z, eventBasis[0].x).cross(eventBasis[0]);
	eventBasis[1].normalize();
	eventBasis[2] = eventBasis[1].cross(eventBasis[0]);
	eventBasis[2].normalize();

	IosNewObject* particles = (IosNewObject*) PX_ALLOC(sizeof(IosNewObject) * mParticleSpawnCount, PX_DEBUG_EXP("IosNewObject"));
	IosNewObject* currParticle = particles;
	for (int i = 0; i < (int)mParticleSpawnCount; ++i, ++currParticle)
	{
		initParticle(hitPos, eventBasis, currParticle->initialPosition, currParticle->initialVelocity, currParticle->lifetime);

		currParticle->iofxActorID	= IofxActorIDIntl(0);
		currParticle->lodBenefit	= 0.0f;
		currParticle->userData		= 0;
	}
	mParticleInjector->createObjects(mParticleSpawnCount, particles);

	PX_FREE(particles);
}

void ImpactEmitterParticleEvent::initParticle(const PxVec3& pos, const PxVec3 basis[], PxVec3& outPos, PxVec3& outVel, float& outLife)
{
	float theta = mOwner->mRand.getScaled(0.0f, 2.0f * PI);
	float phi = mOwner->mRand.getScaled(mMinAngle, mMaxAngle);
	float speed = mOwner->mRand.getScaled(mMinSpeed, mMaxSpeed);

	PxVec3 vel = speed * (PxCos(phi) * basis[0] + PxSin(phi) * PxCos(theta) * basis[1] + PxSin(phi) * PxSin(theta) * basis[2]);

	outPos = pos;
	outVel = vel;
	outLife = mOwner->mRand.getScaled(mMinLife, mMaxLife);
}

void ImpactEmitterParticleEvent::setPreferredRenderVolume(nvidia::apex::RenderVolume* vol)
{
	if (mParticleInjector)
	{
		mParticleInjector->setPreferredRenderVolume(vol);
	}
}

////////////////////////////////////////////////////////////////////////////////

ImpactEmitterEventSet::~ImpactEmitterEventSet()
{
	for (uint32_t e = 0; e < entries.size(); ++e)
	{
		delete entries[e].evnt;
	}
}

////////////////////////////////////////////////////////////////////////////////

bool ImpactEmitterEventSet::AddEvent(NvParameterized::Interface* eventParamPtr, ImpactEmitterAssetImpl& asset, ImpactEmitterActorImpl* emitterActor)
{
	ImpactEmitterBaseEvent* e = NULL;
	float delay = 0;

	PX_ASSERT(eventParamPtr);

	if(!nvidia::strcmp(eventParamPtr->className(), ImpactObjectEvent::staticClassName()))
	{
		e = PX_NEW(ImpactEmitterParticleEvent)(eventParamPtr, asset, emitterActor);
		if (!e->isValid())
		{
			PX_DELETE(e);
			return false;
		}
		ImpactObjectEvent* objEvent = (ImpactObjectEvent*)eventParamPtr;
		delay = objEvent->parameters().delay;
	}

	if (e != NULL)
	{
		entries.pushBack(EventSetEntry(delay, e));
	}
	else
	{
		return false;
	}

	return true;
}


////////////////////////////////////////////////////////////////////////////////

class ImpactEmitterTickTask : public PxTask, public UserAllocated
{
public:
	ImpactEmitterTickTask(ImpactEmitterActorImpl* actor) : mActor(actor) {}

	const char* getName() const
	{
		return "ImpactEmitterActorImpl::TickTask";
	}
	void run()
	{
		mActor->tick();
	}

protected:
	ImpactEmitterActorImpl* mActor;
};

ImpactEmitterActorImpl::ImpactEmitterActorImpl(const NvParameterized::Interface& desc, ImpactEmitterAssetImpl& asset, ResourceList& list, EmitterScene& scene) :
	mTime(0.0f),
	mAsset(&asset),
	mScene(&scene),
	mTickTask(NULL)
{
	mRand.setSeed(scene.mApexScene->getSeed());

	/* Read default values from descriptor or authored asset */
	if (nvidia::strcmp(desc.className(), ImpactEmitterActorParameters::staticClassName()) == 0)
	{
	}
	else
	{
		APEX_INVALID_OPERATION("%s is not a valid descriptor class", desc.className());
	}


	// Insert self into data structures
	list.add(*this);            // Add self to asset's list of actors
	addSelfToContext(*scene.mApexScene->getApexContext());    // Add self to ApexScene
	addSelfToContext(scene);    // Add self to EmitterScene's list of actors

	// Initialize data members
	mTime = 0.f;

	ParamArray<NvParameterized::Interface*> assetEventSets(asset.mParams,
	        "eventSetList",
	        (ParamDynamicArrayStruct*) & (asset.mParams->eventSetList));
	//mEventSets.resize( asset.mEventSets.size() );
	mEventSets.resize(asset.mEventNameIndexMaps.size());

	for (uint32_t s = 0; s < asset.mEventNameIndexMaps.size(); ++s)
	{
		for (uint32_t e = 0; e < asset.mEventNameIndexMaps[s]->eventIndices.size(); ++e)
		{
			uint32_t t = asset.mEventNameIndexMaps[s]->eventIndices[e];

			if (!mEventSets[s].AddEvent(assetEventSets[t], asset, this))
			{
				return;
			}
		}
	}

	mTickTask = PX_NEW(ImpactEmitterTickTask)(this);
	PX_ASSERT(mTickTask);

	mValid = true;
}

ImpactEmitterActorImpl::~ImpactEmitterActorImpl()
{
	delete mTickTask;
}

void ImpactEmitterActorImpl::setPreferredRenderVolume(RenderVolume* vol)
{
	WRITE_ZONE();
	for (uint32_t i = 0 ; i < mEventSets.size() ; i++)
	{
		for (uint32_t e = 0 ; e < mEventSets[i].entries.size() ; e++)
		{
			mEventSets[i].entries[e].evnt->setPreferredRenderVolume(vol);
		}
	}
}

void ImpactEmitterActorImpl::submitTasks()
{
	float dt = mScene->mApexScene->getElapsedTime();
	mTime += dt;

	// Check queued events, triggering and removing those that have reached their trigger time
	for (uint32_t e = 0; e < mPendingEvents.size();)
	{
		if (mPendingEvents[e].trigger(mTime))
		{
			mPendingEvents.replaceWithLast(e);
		}
		else
		{
			e++;
		}
	}

	//
	PxTaskManager* tm = mScene->mApexScene->getTaskManager();

	tm->submitUnnamedTask(*mTickTask);

	// submitTasks for events
	for (uint32_t i = 0 ; i < mEventSets.size() ; i++)
	{
		ImpactEmitterEventSet& set = mEventSets[i];
		for (uint32_t s = 0 ; s < set.entries.size() ; s++)
		{
			ImpactEmitterEventSet::EventSetEntry& entry = set.entries[s];
			entry.evnt->submitTasks(tm);
		}
	}
}

void ImpactEmitterActorImpl::setTaskDependencies()
{
	PxTaskManager* tm = mScene->mApexScene->getTaskManager();

	PxTaskID completionTaskID = tm->getNamedTask(AST_PHYSX_SIMULATE);
	mTickTask->finishBefore(completionTaskID);

	// setTaskDependencies for events
	for (uint32_t i = 0 ; i < mEventSets.size() ; i++)
	{
		ImpactEmitterEventSet& set = mEventSets[i];
		for (uint32_t s = 0 ; s < set.entries.size() ; s++)
		{
			ImpactEmitterEventSet::EventSetEntry& entry = set.entries[s];
			entry.evnt->setTaskDependencies(mTickTask);
		}
	}
}

void ImpactEmitterActorImpl::fetchResults()
{
}


Asset*            ImpactEmitterActorImpl::getOwner() const
{
	READ_ZONE();
	return (Asset*) mAsset;
}

ImpactEmitterAsset*   ImpactEmitterActorImpl::getEmitterAsset() const
{
	READ_ZONE();
	return mAsset;
}

SceneIntl* ImpactEmitterActorImpl::getApexScene() const
{
	return mScene->mApexScene;
}

void ImpactEmitterActorImpl::getLodRange(float& min, float& max, bool& intOnly) const
{
	READ_ZONE();
	PX_UNUSED(min);
	PX_UNUSED(max);
	PX_UNUSED(intOnly);
	APEX_INVALID_OPERATION("not implemented");
}

float ImpactEmitterActorImpl::getActiveLod() const
{
	READ_ZONE();
	APEX_INVALID_OPERATION("ImpactEmitterActor does not support this operation");
	return -1.0f;
}

void ImpactEmitterActorImpl::forceLod(float lod)
{
	WRITE_ZONE();
	PX_UNUSED(lod);
	APEX_INVALID_OPERATION("not implemented");
}

void ImpactEmitterActorImpl::release()
{
	if (mInRelease)
	{
		return;
	}
	mInRelease = true;
	destroy();
}


void ImpactEmitterActorImpl::destroy()
{
	/* Order is important here, pay attention */

	// Remove ourselves from all contexts, so they don't get stuck trying to release us
	ApexActor::destroy();

	ApexContext::removeAllActors();

	delete this;
}

void ImpactEmitterActorImpl::removeActorAtIndex(uint32_t index)
{
	// A particle injector has been removed
	for (uint32_t i = 0 ; i < mEventSets.size() ; i++)
	{
		ImpactEmitterEventSet& set = mEventSets[i];
		for (uint32_t s = 0 ; s < set.entries.size() ; s++)
		{
			ImpactEmitterEventSet::EventSetEntry& entry = set.entries[s];
			entry.evnt->removeActorReference(mActorArray[ index ]);
		}
	}

	ApexContext::removeActorAtIndex(index);

	release();
}

void ImpactEmitterActorImpl::tick()
{
}

void ImpactEmitterActorImpl::registerImpact(const PxVec3& hitPos, const PxVec3& hitDir, const PxVec3& surfNorm, uint32_t surfType)
{
	WRITE_ZONE();
	if (surfType < (uint32_t) mEventSets.size())
	{
		// Check for non finite values (even in release build)
		bool hitPosFinite	= hitPos.isFinite();
		bool hitDirFinite	= hitDir.isFinite();
		bool surfNormFinite = surfNorm.isFinite();
		if (hitPosFinite && hitDirFinite && surfNormFinite)
		{
			ImpactEmitterEventSet& eventSet = mEventSets[surfType];
			for (uint32_t e = 0; e < eventSet.entries.size(); ++e)
			{
				QueuedImpactEvent newEvent(hitPos, hitDir, surfNorm, mTime + eventSet.entries[e].dly, eventSet.entries[e].evnt);
				mPendingEvents.pushBack(newEvent);
			}
		}
		else
		{
			// Release and debug builds should output a warning message

			APEX_INVALID_PARAMETER("Asset Name: %s, hitPos: %s, hitDir: %s, surfNorm: %s, surface type: %d",
			                       mAsset->mName.c_str(),
			                       hitPosFinite	? "finite" : "nonFinite",
			                       hitDirFinite	? "finite" : "nonFinite",
			                       surfNormFinite	? "finite" : "nonFinite",
			                       surfType);

			// We really want debug builds to catch the culprits here
			PX_ASSERT(hitPos.isFinite());
			PX_ASSERT(hitDir.isFinite());
			PX_ASSERT(surfNorm.isFinite());
		}
	}
}

void ImpactEmitterActorImpl::visualize(RenderDebugInterface& renderDebug)
{
	if ( !mEnableDebugVisualization ) return;
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(renderDebug);
#else
	RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();
	RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(RENDER_DEBUG::DebugColors::Orange));
	RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
#endif
}

}
} // namespace nvidia::apex
