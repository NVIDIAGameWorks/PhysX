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
#include "ApexUsingNamespace.h"
#include "InstancedObjectSimulationIntl.h"
#include "EmitterActorImpl.h"
#include "EmitterAssetImpl.h"
#include "EmitterScene.h"
#include "EmitterGeomExplicitImpl.h"
#include "ApexSDKIntl.h"
#include "SceneIntl.h"
#include "RenderDebugInterface.h"

#include "PxRigidDynamic.h"

#include "ScopedPhysXLock.h"
#include "ReadCheck.h"

#include "PsMathUtils.h"

namespace nvidia
{
namespace emitter
{

#pragma warning(disable: 4355) // 'this' : used in base member initializer list

EmitterActorImpl::EmitterActorImpl(const EmitterActorDesc& desc, EmitterAssetImpl& asset, ResourceList& list, EmitterScene& scene)
	: mPxScene(NULL)
	, mAttachActor(NULL)
	, mInjector(NULL)
	, mAsset(&asset)
	, mScene(&scene)
	, mLastNonEmptyOverlapAABB(PxBounds3::empty())
	, mPose(desc.initialPose)
	, mFirstStartEmitCall(true)
	, mEmitAssetParticles(desc.emitAssetParticles)
	, mPersist(false)
	, mRemainder(0)
	, mIsOldPoseInitialized(false)
	, mOverlapTestCollisionGroups(desc.overlapTestCollisionGroups)
	, mShouldUseGroupsMask(false)
	, mExplicitGeom(NULL)
	, mTickTask(*this)
	, mObjectScale(desc.initialScale)
{
	mEmitterValidateCallback = NULL;
	mIOS = NULL;
	mRand.setSeed(scene.mApexScene->getSeed());

	list.add(*this);            // Add self to asset's list of actors

	mPoses.pushBack(mPose);

	/* Get initial values from authored asset */
	mDensity = mAsset->getDensity();
	mRate = mAsset->getRate();
	mVelocityLow = mAsset->getVelocityLow();
	mVelocityHigh = mAsset->getVelocityHigh();
	mLifetimeLow = mAsset->getLifetimeLow();
	mLifetimeHigh = mAsset->getLifetimeHigh();
	mAttachActor = desc.attachActor;
	mAttachRelativePose = desc.attachRelativePose;
	mDoEmit = mAsset->mGeom->getEmitterGeom()->getEmitterType() == EmitterType::ET_DENSITY_ONCE;
	mEmitterVolume = mAsset->mGeom->computeEmitterVolume();

	mEmitDuration = mDescEmitDuration = desc.emitterDuration;

	// create an instance of the explicit geometry for this actor's particles
	// if the asset is an explicit emitter
	if (mAsset->mGeom->getEmitterGeom()->isExplicitGeom())
	{
		mExplicitGeom = PX_NEW(EmitterGeomExplicitImpl)();
		PX_ASSERT(mExplicitGeom);
	}

	const char* iofxAssetName = mAsset->getInstancedObjectEffectsAssetName();
	IofxAsset* iofxAsset = static_cast<IofxAsset*>(mAsset->mIofxAssetTracker.getAssetFromName(iofxAssetName));
	IosAsset* iosAsset = mAsset->mIosAssetTracker.getIosAssetFromName(mAsset->getInstancedObjectSimulatorTypeName(),
	                       mAsset->getInstancedObjectSimulatorAssetName());
	if (!iosAsset || !iofxAsset)
	{
		return;
	}

	Actor* nxActor = iosAsset->createIosActor(*scene.mApexScene, iofxAsset);
	InstancedObjectSimulationIntl* ios = NULL;
	if (nxActor)
	{
		ApexActor* aa = GetInternalApexSDK()->getApexActor(nxActor);
		if (aa)
		{
			ios = DYNAMIC_CAST(InstancedObjectSimulationIntl*)(aa);
		}
	}
	if (!ios)
	{
		return;
	}
	
	mObjectRadius = ios->getObjectRadius();
	mInjector = ios->allocateInjector(iofxAsset);
	if (!mInjector)
	{
		return;
	}
	mIOS = ios;
	mInjector->addSelfToContext(*this);
	setLodParamDesc(mAsset->getLodParamDesc());

	mInjector->setObjectScale(mObjectScale);

	addSelfToContext(*scene.mApexScene->getApexContext());    // Add self to ApexScene
	addSelfToContext(scene);    // Add self to EmitterScene's list of actors

	mValid = true;
}

EmitterActorImpl::~EmitterActorImpl()
{
}

void EmitterActorImpl::setPreferredRenderVolume(nvidia::apex::RenderVolume* vol)
{
	WRITE_ZONE();
	if (mInjector)
	{
		mInjector->setPreferredRenderVolume(vol);
	}
}

void EmitterActorImpl::submitTasks()
{
	if (mInjector != 0)
	{
		mScene->mApexScene->getTaskManager()->submitUnnamedTask(mTickTask);
	}
}

void EmitterActorImpl::setTaskDependencies()
{
	if (mInjector != 0)
	{
		mTickTask.finishBefore(mInjector->getCompletionTaskID());
		mTickTask.finishBefore(mScene->mApexScene->getTaskManager()->getNamedTask(AST_PHYSX_SIMULATE));
	}
}

void EmitterActorImpl::fetchResults()
{
}

void EmitterActorImpl::removeActorAtIndex(uint32_t index)
{
	// An injector has been deleted
	PX_ASSERT(mInjector == (IosInjectorIntl*) mActorArray[index]);
	mInjector = NULL;
	ApexContext::removeActorAtIndex(index);
	release();
}

/* Must be defined inside CPP file, since they require knowledge of asset class */
Asset*            EmitterActorImpl::getOwner() const
{
	READ_ZONE();
	return (Asset*) mAsset;
}
EmitterAsset*     EmitterActorImpl::getEmitterAsset() const
{
	READ_ZONE();
	return mAsset;
}

void EmitterActorImpl::release()
{
	if (mInRelease)
	{
		return;
	}
	mInRelease = true;
	mAsset->releaseEmitterActor(*this);
}


void EmitterActorImpl::destroy()
{
	ApexActor::destroy();

	if (mExplicitGeom)
	{
		mExplicitGeom->destroy();
		mExplicitGeom = 0;
	}

	setPhysXScene(NULL);

	if (mInjector)
	{
		mInjector->release();
	}
	mInjector = NULL;
	mIOS = NULL;

	delete this;
}

void EmitterActorImpl::setLodParamDesc(const EmitterLodParamDesc& d)
{
	WRITE_ZONE();
	mLodParams = d;
	PX_ASSERT(mInjector);
	if (mInjector)
		mInjector->setLODWeights(d.maxDistance, d.distanceWeight, d.speedWeight,
		d.lifeWeight, d.separationWeight, d.bias);
}

void EmitterActorImpl::getLodRange(float& min, float& max, bool& intOnly) const
{
	READ_ZONE();
	PX_UNUSED(min);
	PX_UNUSED(max);
	PX_UNUSED(intOnly);
	APEX_INVALID_OPERATION("not implemented");
}


float EmitterActorImpl::getActiveLod() const
{
	READ_ZONE();
	APEX_INVALID_OPERATION("EmitterActor does not support this operation");
	return -1.0f;
}


void EmitterActorImpl::forceLod(float lod)
{
	WRITE_ZONE();
	PX_UNUSED(lod);
	APEX_INVALID_OPERATION("not implemented");
}

#ifdef WITHOUT_DEBUG_VISUALIZE
void EmitterActorImpl::visualize(RenderDebugInterface&)
{
}
#else
void EmitterActorImpl::visualize(RenderDebugInterface& renderDebug)
{
	if (!mScene->mEmitterDebugRenderParams->apexEmitterParameters.VISUALIZE_APEX_EMITTER_ACTOR)
	{
		return;
	}
	if ( !mEnableDebugVisualization ) return;

	using RENDER_DEBUG::DebugColors;

	if (mAsset->mGeom)
	{
		mAsset->mGeom->visualize(mPose, renderDebug);
	}

	RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();
	RENDER_DEBUG_IFACE(&renderDebug)->setCurrentTextScale(2.0f);

	PxVec3 textLocation = this->getGlobalPose().column3.getXYZ();
	//visualize actor name
	if (mScene->mEmitterDebugRenderParams->apexEmitterParameters.VISUALIZE_APEX_EMITTER_ACTOR_NAME &&
	        mScene->mEmitterDebugRenderParams->apexEmitterParameters.THRESHOLD_DISTANCE_APEX_EMITTER_ACTOR_NAME >
	        (-mScene->mApexScene->getEyePosition(0) + textLocation).magnitude())
	{
		RENDER_DEBUG_IFACE(&renderDebug)->debugText(textLocation, " %s %s", this->getOwner()->getObjTypeName(), this->getOwner()->getName());
	}
	//visualize actor pose
	if (mScene->mEmitterDebugRenderParams->apexEmitterParameters.VISUALIZE_APEX_EMITTER_ACTOR_POSE &&
	        mScene->mEmitterDebugRenderParams->apexEmitterParameters.THRESHOLD_DISTANCE_APEX_EMITTER_ACTOR_POSE >
	        (-mScene->mApexScene->getEyePosition(0) + textLocation).magnitude())
	{
		RENDER_DEBUG_IFACE(&renderDebug)->debugAxes(this->getGlobalPose(), 1);
	}

	if (mScene->mEmitterDebugRenderParams->apexEmitterParameters.VISUALIZE_TOTAL_INJECTED_AABB)
	{
		if (!mOverlapAABB.isEmpty())
		{
			mLastNonEmptyOverlapAABB.include(mOverlapAABB);
		}
		RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Red));
		RENDER_DEBUG_IFACE(&renderDebug)->debugBound(mLastNonEmptyOverlapAABB);
		RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
	}

	RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
}
#endif


EmitterGeomExplicit* EmitterActorImpl::isExplicitGeom()
{
	WRITE_ZONE();
	if (mAsset->mGeom->getEmitterGeom()->isExplicitGeom())
	{
		// return the actor's explicit geometry
		return const_cast<EmitterGeomExplicit*>(mExplicitGeom->getEmitterGeom()->isExplicitGeom());
	}
	else
	{
		return NULL;
	}
}


void EmitterActorImpl::startEmit(bool persistent)
{
	WRITE_ZONE();
	// persistent remains a global setting
	// simply store the current pose, emit all particles in the list for every pose
	// fix bug: we don't want two poses on the first frame unless startEmit is called twice
	if (mFirstStartEmitCall)
	{
		mFirstStartEmitCall = false;
		mPoses.clear();
	}

	mPoses.pushBack(mPose);

	mEmitDuration = mDescEmitDuration;

	mDoEmit = true;
	mPersist = persistent;
}


void EmitterActorImpl::stopEmit()
{
	WRITE_ZONE();
	mDoEmit = false;
	mPersist = false;
}


void EmitterActorImpl::tick()
{
	mOverlapAABB = PxBounds3::empty();

	// Clamp emission time to a min of 15FPS, to prevent extremely long frames
	// from generating too many particles when rate emitters are in use.
	float dt = PxMin(1 / 15.0f, mScene->mApexScene->getElapsedTime());

	if (!mInjector || !mAsset->mGeom)
	{
		return;
	}

	if (mAttachActor)
	{
		SCOPED_PHYSX_LOCK_WRITE(mScene->mApexScene);

		const PxTransform t = mAttachActor->is<physx::PxRigidDynamic>()->getGlobalPose();
		setCurrentPose(t * mAttachRelativePose);
	}

	float curdensity = mAsset->getDensity() * mAsset->mModule->getDensityScale();
	float currate = mAsset->getRate() * mAsset->mModule->getRateScale();
	if (!mPersist && mDescEmitDuration > 0.0f && mDescEmitDuration < PX_MAX_F32)
	{
		uint32_t outCount;
		if (mAsset->mRateVsTimeCurve.getControlPoints(outCount))
		{
			const float time = 1.0f - (mEmitDuration / mDescEmitDuration);
			currate *= mAsset->mRateVsTimeCurve.evaluate(time);
		}
	}

	EmitterGeom* nxGeom = mAsset->mGeom->getEmitterGeom();
	if (mDoEmit && nxGeom->isExplicitGeom())
	{
		mNewPositions.clear();
		mNewVelocities.clear();
		mNewUserData.clear();

		// compute the fill positions from both the asset's and actor's explicit geom

		for (uint32_t i = 0; i < mPoses.size(); i++)
		{
			PxTransform poseWithScale = mPoses[i];
			uint32_t velIdx = mNewVelocities.size();

			if (mEmitAssetParticles)
			{
				DYNAMIC_CAST(EmitterGeomExplicitImpl*)(mAsset->mGeom)->computeFillPositions(mNewPositions, mNewVelocities, &mNewUserData, poseWithScale, PxVec3(mObjectScale), curdensity, mOverlapAABB, mRand);
			}
			DYNAMIC_CAST(EmitterGeomExplicitImpl*)(mExplicitGeom)->computeFillPositions(mNewPositions, mNewVelocities, &mNewUserData, poseWithScale, PxVec3(mObjectScale), curdensity, mOverlapAABB, mRand);

			for (; velIdx < mNewVelocities.size(); ++velIdx)
			{
				if (mNewVelocities[velIdx].isZero())
				{
					mNewVelocities[velIdx] = poseWithScale.rotate(mRand.getScaled(mVelocityLow, mVelocityHigh));
				}
			}
		}
		mPoses.clear();

		if (mNewPositions.size() > 0)
		{
			uint32_t positionsSize = mNewPositions.size();
			PX_ASSERT(mNewVelocities.size() == positionsSize);
			uint32_t userDataSize = mNewUserData.size();

			mNewObjectArray.clear();
			mNewObjectArray.reserve(positionsSize);
			for (uint32_t i = 0; i < positionsSize; i++)
			{
				IosNewObject& obj = mNewObjectArray.insert();

				obj.initialPosition = mNewPositions[i];
				obj.initialVelocity = mNewVelocities[i];

				obj.lifetime = mRand.getScaled(mLifetimeLow, mLifetimeHigh);
				obj.iofxActorID	= IofxActorIDIntl(0);
				obj.lodBenefit	= 0.0f;

				obj.userData = 0;
				if (i < userDataSize)
				{
					obj.userData = mNewUserData[i];
				}
			}
			if (mNewObjectArray.size())
			{
				mInjector->createObjects(mNewObjectArray.size(), &mNewObjectArray[0]);
			}
		}

		if (mPersist)
		{
			mDoEmit = true;
		}
		else if (mEmitDuration > 0.0f)
		{
			mEmitDuration = PxMax(0.0f, mEmitDuration - dt);
			mDoEmit = (mEmitDuration > 0.0f);
		}
		else
		{
			mDoEmit = false;
		}

		if (mDoEmit)
		{
			// set the last pose as the single pose in the list if we're persisting
			mPoses.pushBack(mPose);
		}

		return;
	}

	if (mDoEmit)
	{
		uint32_t fillNumParticles;
		fillNumParticles = (uint32_t)(curdensity * mEmitterVolume);

		switch (nxGeom->getEmitterType())
		{
		case EmitterType::ET_RATE:
		{
			const uint32_t minSamplingFPS = mAsset->mParams->minSamplingFPS;

			uint32_t numSubSteps = 1;
			if (minSamplingFPS > 0)
			{
				numSubSteps = uint32_t(physx::shdfnd::ceil(dt * minSamplingFPS));
			}
			if (numSubSteps > 1)
			{
				const float dtSubStep = dt / numSubSteps;

				const PxQuat& originQ = mIsOldPoseInitialized ? mOldPose.q : mPose.q;
				const PxVec3& originT = mIsOldPoseInitialized ? mOldPose.p : mPose.p;
				for (uint32_t i = 0; i < numSubSteps; ++i)
				{
					const float s = (i + 1) / float(numSubSteps);

					PxTransform pose(physx::shdfnd::slerp(s, originQ, mPose.q));
					pose.p = originT * (1.0f - s) + mPose.p * s;

					emitObjects(pose, computeNbEmittedFromRate(dtSubStep, currate), true);
				}
			}
			else
			{
				emitObjects(mPose, computeNbEmittedFromRate(dt, currate), true);
			}
		}
		break;

		case EmitterType::ET_DENSITY_ONCE:
		{
			emitObjects(mPose, fillNumParticles, true);
		}
		break;

		case EmitterType::ET_DENSITY_BRUSH:
			emitObjects(mPose, fillNumParticles, !mIsOldPoseInitialized);
			break;

		case EmitterType::ET_FILL:
		{
			mNewPositions.clear();
			mNewVelocities.clear();

			mAsset->mGeom->computeFillPositions(mNewPositions, mNewVelocities, mPose, PxVec3(mObjectScale), mObjectRadius, mOverlapAABB, mRand);
			if (mNewPositions.size() > mAsset->getMaxSamples())
			{
				mNewPositions.resize(mAsset->getMaxSamples());
			}
			emitObjects(mNewPositions);
		}
		break;

		default:
			PX_ASSERT(!"emitterType not supported.");
			break;
		}

		if (mPersist)
		{
			mDoEmit = true;
		}
		else if (mEmitDuration > 0.0f)
		{
			mEmitDuration = PxMax(0.0f, mEmitDuration - dt);
			mDoEmit = (mEmitDuration > 0.0f);
		}
		else
		{
			mDoEmit = false;
		}
	}

	mOldPose = mPose;
	mIsOldPoseInitialized = true;
}


uint32_t EmitterActorImpl::computeNbEmittedFromRate(float dt, float currate)
{
	// compute number of particles to be spawned
	float nbEmittedReal = currate * dt;
	nbEmittedReal += mRemainder;
	uint32_t currentNbEmitted = (uint32_t) PxFloor(nbEmittedReal);
	mRemainder = nbEmittedReal - (float) currentNbEmitted;

	return currentNbEmitted;
}


void EmitterActorImpl::emitObjects(const PxTransform& pose, uint32_t toEmitNum, bool useFullVolume)
{
	if (toEmitNum == 0)
	{
		return;
	}

	if (!useFullVolume)
	{
		// compute newly covered volume
		// SJB: Notes
		// This is bizarre: we do 100 random samples to guess the amount of new area
		// we have to work with, then generate that percentage of new particles?
		float percent = mAsset->mGeom->computeNewlyCoveredVolume(mOldPose, pose, mObjectScale, mRand);
		toEmitNum = (uint32_t)((float) toEmitNum * percent);
	}

	PxVec3 emitterOrigin = getGlobalPose().getPosition();
	// TODO: This could obviously be more efficient
	uint32_t emittedCount = 0;
	mNewObjectArray.clear();
	mNewObjectArray.reserve(mAsset->getMaxSamples());
	for (uint32_t i = 0; i < mAsset->getMaxSamples(); i++)
	{
		PxVec3 pos;
		if (useFullVolume)
		{
			pos = mAsset->mGeom->randomPosInFullVolume(pose, mRand);
		}
		else
		{
			pos = mAsset->mGeom->randomPosInNewlyCoveredVolume(pose, mOldPose, mRand);
		}

		if ( mEmitterValidateCallback )
		{
			if ( !mEmitterValidateCallback->validateEmitterPosition(emitterOrigin, pos))
			{
				continue;
			}
		}


		mOverlapAABB.include(pos);

		IosNewObject& obj = mNewObjectArray.insert();
		obj.initialPosition = pos;
		obj.initialVelocity = pose.rotate(mRand.getScaled(mVelocityLow, mVelocityHigh));

		obj.lifetime = mRand.getScaled(mLifetimeLow, mLifetimeHigh);
		obj.iofxActorID	= IofxActorIDIntl(0);
		obj.lodBenefit	= 0.0f;
		obj.userData = 0;

		if (++emittedCount >= toEmitNum)
		{
			break;
		}
	}

	if (mNewObjectArray.size())
	{
		mInjector->createObjects(mNewObjectArray.size(), &mNewObjectArray[0]);
	}

	if (emittedCount < toEmitNum)
	{
		APEX_DEBUG_WARNING(
			"\n Emitter asset name: %s."
			"\n Ios asset name: %s."
			"\n Iofx asset name: %s."
			"\nMessage: Not all objects have been emitted. Possible reasons: "
			"\n * Emitter may be failing overlap tests"
			"\n * The individual object radius may be too big to fit in the emitter"
			"\n * The emitter rate or density may have requested more than the asset's 'maxSamples' parameter\n", 
			mAsset->getName(), 
			mAsset->getInstancedObjectEffectsAssetName(),
			mAsset->getInstancedObjectSimulatorAssetName());
	}
}


void EmitterActorImpl::emitObjects(const physx::Array<PxVec3>& positions)
{
	if (positions.empty())
	{
		return;
	}

	// TODO: This could obviously be more efficient
	mNewObjectArray.clear();

	PxVec3 emitterOrigin = getGlobalPose().getPosition();

	for (uint32_t i = 0; i <  positions.size(); i++)
	{
		PxVec3 position = positions[i];
		if ( mEmitterValidateCallback )
		{
			if ( !mEmitterValidateCallback->validateEmitterPosition(emitterOrigin, position))
			{
				continue;
			}
		}
		IosNewObject& obj = mNewObjectArray.insert();
		obj.initialPosition = position;
		obj.initialVelocity = mPose.rotate(mObjectScale * mRand.getScaled(mVelocityLow, mVelocityHigh));

		obj.lifetime = mRand.getScaled(mLifetimeLow, mLifetimeHigh);
		obj.iofxActorID	= IofxActorIDIntl(0);
		obj.lodBenefit	= 0.0f;
		obj.userData = 0;
	}

	if (mNewObjectArray.size())
	{
		mInjector->createObjects(mNewObjectArray.size(), &mNewObjectArray[0]);
	}
}

uint32_t	EmitterActorImpl::getActiveParticleCount() const
{
	READ_ZONE();
	uint32_t ret = 0;

	if ( mInjector )
	{
		ret = mInjector->getActivePaticleCount();
	}

	return ret;
}

void	EmitterActorImpl::setDensityGridPosition(const PxVec3 &pos)
{
	WRITE_ZONE();
	if ( mIOS )
	{
		mIOS->setDensityOrigin(pos);
	}
}

}
} // namespace nvidia::apex
