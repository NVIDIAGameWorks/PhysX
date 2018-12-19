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
#include "GroundEmitterAsset.h"
#include "SceneIntl.h"
#include "RenderDebugInterface.h"
#include "InstancedObjectSimulationIntl.h"
#include "GroundEmitterAssetImpl.h"
#include "GroundEmitterActorImpl.h"
#include "EmitterScene.h"
#include "ApexUsingNamespace.h"
#include "ApexRand.h"

#include "ScopedPhysXLock.h"
#include "PxRigidDynamic.h"

namespace nvidia
{
namespace emitter
{

using namespace physx;

void InjectorData::InjectTask::run()
{
	mActor->injectParticles(*mData);
}

// this function will 'a'+'b' and wrap it around the max value back to 0 if necessary
// not safe when 'a' + 'b' > 2*max, or something like that
static inline int32_t INCREMENT_CELL(int32_t& a, int32_t b, int32_t max)
{
	if (a + b > max)
	{
		return ((a + b) - max - 1);
	}
	else
	{
		return a + b;
	}
}

#pragma warning(disable: 4355)

GroundEmitterActorImpl::GroundEmitterActorImpl(const GroundEmitterActorDesc& desc, GroundEmitterAssetImpl& asset, ResourceList& list, EmitterScene& scene)
	: mAsset(&asset)
	, mScene(&scene)
	, mLocalUpDirection(PxVec3(0.0f, 1.0f, 0.0f))
	, mGridCellSize(0.0f)
	, mTotalElapsedTimeMs(0)
	, mShouldUseGroupsMask(false)
	, mTickTask(*this)
{
	mRand.setSeed(scene.mApexScene->getSeed());

	GroundEmitterActorDesc defaults;

	/* Read default values from descriptor or authored asset */
	if (desc.raycastCollisionGroups == defaults.raycastCollisionGroups)
	{
		setRaycastCollisionGroups(mAsset->mRaycastCollisionGroups);
	}
	else
	{
		setRaycastCollisionGroups(desc.raycastCollisionGroups);
	}
	if (desc.radius == defaults.radius)
	{
		setRadius(mAsset->getRadius());
	}
	else
	{
		setRadius(desc.radius);
	}
	if (desc.maxRaycastsPerFrame == 0)
	{
		setMaxRaycastsPerFrame(mAsset->getMaxRaycastsPerFrame());
	}
	else
	{
		setMaxRaycastsPerFrame(desc.maxRaycastsPerFrame);
	}
	if (desc.raycastHeight == 0.0f)
	{
		setRaycastHeight(mAsset->getRaycastHeight());
	}
	else
	{
		setRaycastHeight(desc.raycastHeight);
	}
	if (desc.density == 0.0f)
	{
		mDensity = mAsset->getDensity();
	}
	else
	{
		mDensity = desc.density;
	}
	if (desc.attachActor)
	{
		setAttachActor(desc.attachActor);
		setAttachRelativePosition(desc.attachRelativePosition);
	}
	else
	{
		mAttachActor = NULL;
	}
	if (desc.spawnHeight >= 0.0f)
	{
		setSpawnHeight(desc.spawnHeight);
	}
	else
	{
		setSpawnHeight(mAsset->getSpawnHeight());
	}

	mOldLocalPlayerPosition = PxVec3(0.0f);
	mPose = PxTransform(PxIdentity);
	setRotation(desc.rotation);
	setPosition(desc.initialPosition);

	mMaterialCallback = desc.materialCallback;

	mRaycastCollisionGroupsMask	= PxFilterData(0, 0, 0, 0);

	mRefreshFullCircle = true;
	mSimulationSteps = 0;

	for (uint32_t i = 0 ; i < mAsset->mMaterialFactoryMaps->size() ; i++)
	{
		MaterialFactoryMappingDesc factoryDesc;
		factoryDesc.instancedObjectEffectsAssetName = (*mAsset->mMaterialFactoryMaps)[i].iofxAssetName->name();
		factoryDesc.instancedObjectSimulationTypeName = (*mAsset->mMaterialFactoryMaps)[i].iosAssetName->className();
		factoryDesc.instancedObjectSimulationAssetName = (*mAsset->mMaterialFactoryMaps)[i].iosAssetName->name();
		factoryDesc.physicalMaterialName = (*mAsset->mMaterialFactoryMaps)[i].physMatName;
		factoryDesc.weight = (*mAsset->mMaterialFactoryMaps)[i].weight;
		factoryDesc.maxSlopeAngle = (*mAsset->mMaterialFactoryMaps)[i].maxSlopeAngle;
		EmitterLodParamDesc lodParamDesc;
		GroundEmitterAssetImpl::copyLodDesc(lodParamDesc, (*mAsset->mMaterialFactoryMaps)[i].lodParamDesc);
		if (!addMeshForGroundMaterial(factoryDesc, lodParamDesc))
		{
			return;
		}
	}

	list.add(*this);            // Add self to asset's list of actors
	addSelfToContext(*scene.mApexScene->getApexContext());    // Add self to ApexScene
	addSelfToContext(scene);    // Add self to EmitterScene's list of actors

	mValid = true;
}

GroundEmitterActorImpl::~GroundEmitterActorImpl()
{
}

void GroundEmitterActorImpl::submitTasks()
{
	float dt;
	bool stepPhysX = mScene->mApexScene->physXElapsedTime(dt);

	PxTaskManager* tm = mScene->mApexScene->getTaskManager();

	for (uint32_t i = 0; i < mInjectorList.size(); i++)
	{
		tm->submitUnnamedTask(mInjectorList[i]->mTask);
	}

	if (stepPhysX)
	{
		tm->submitUnnamedTask(mTickTask);
	}
}

void GroundEmitterActorImpl::setTaskDependencies()
{
	float dt;
	bool stepPhysX = mScene->mApexScene->physXElapsedTime(dt);

	PxTaskManager* tm = mScene->mApexScene->getTaskManager();

	for (uint32_t i = 0; i < mInjectorList.size(); i++)
	{
		PxTask* injectTask = &mInjectorList[i]->mTask;

		injectTask->finishBefore(mInjectorList[i]->mInjector->getCompletionTaskID());

		if (stepPhysX)
		{
			mTickTask.startAfter(injectTask->getTaskID());
		}
	}

	if (stepPhysX)
	{
		mTickTask.finishBefore(tm->getNamedTask(AST_PHYSX_SIMULATE));
	}
}


Asset*            GroundEmitterActorImpl::getOwner() const
{
	READ_ZONE();
	return (Asset*) mAsset;
}
GroundEmitterAsset*   GroundEmitterActorImpl::getEmitterAsset() const
{
	READ_ZONE();
	return mAsset;
}

void GroundEmitterActorImpl::setRaycastCollisionGroupsMask(physx::PxFilterData* m)
{
	WRITE_ZONE();
	if (!m)
	{
		mShouldUseGroupsMask = false;
	}
	else
	{
		mRaycastCollisionGroupsMask = *m;
		mShouldUseGroupsMask = true;
	}
}


void GroundEmitterActorImpl::setPreferredRenderVolume(nvidia::apex::RenderVolume* vol)
{
	WRITE_ZONE();
	for (uint32_t i = 0 ; i < mInjectorList.size() ; i++)
	{
		InjectorData* data = mInjectorList[i];
		if (data->mInjector)
		{
			data->mInjector->setPreferredRenderVolume(vol);
		}
	}
}

void GroundEmitterActorImpl::setPhysXScene(physx::PxScene* s)
{
	mPxScene = s;
}

void GroundEmitterActorImpl::getLodRange(float& min, float& max, bool& intOnly) const
{
	PX_UNUSED(min);
	PX_UNUSED(max);
	PX_UNUSED(intOnly);
	APEX_INVALID_OPERATION("not implemented");
}


float GroundEmitterActorImpl::getActiveLod() const
{
	APEX_INVALID_OPERATION("GroundEmitterActor does not support this operation");
	return -1.0f;
}


void GroundEmitterActorImpl::forceLod(float lod)
{
	PX_UNUSED(lod);
	APEX_INVALID_OPERATION("not implemented");
}


void GroundEmitterActorImpl::release()
{
	if (mInRelease)
	{
		return;
	}
	mInRelease = true;
	mAsset->releaseActor(*this);
}


void GroundEmitterActorImpl::destroy()
{
	/* Order is important here, pay attention */

	// Remove ourselves from all contexts, so they don't get stuck trying to release us
	ApexActor::destroy();

	for (uint32_t i = 0; i < mInjectorList.size(); i++)
	{
		// Release our actor first
		if (mInjectorList[i]->mInjector)
		{
			mInjectorList[i]->mInjector->release();
		}

		delete mInjectorList[i];
	}
	mInjectorList.clear();

	delete this;
}


void GroundEmitterActorImpl::removeActorAtIndex(uint32_t index)
{
	// One of our injectors has been released
	for (uint32_t i = 0; i < mInjectorList.size(); i++)
	{
		if (mInjectorList[i]->mInjector == mActorArray[ index ])
		{
			mInjectorList[i]->mInjector = NULL;
		}
	}

	ApexContext::removeActorAtIndex(index);
	release();
}

const PxMat44 GroundEmitterActorImpl::getPose() const
{
	READ_ZONE();
	PxMat44 mat44 = PxMat44(mPose);
	mat44.setPosition(mWorldPlayerPosition);
	return mat44;
}

void GroundEmitterActorImpl::setPose(const PxMat44& pos)
{
	WRITE_ZONE();
	PxMat44 rotation = pos;
	rotation.setPosition(PxVec3(0.0f));

	mWorldPlayerPosition = pos.getPosition();

	setRotation(PxTransform(rotation));
}


void GroundEmitterActorImpl::setRotation(const PxTransform& rotation)
{
	PX_ASSERT(mPose.p == PxVec3(0.0f));
	mPose.q = rotation.q;
	mInversePose = mPose.getInverse();
	setPosition(mWorldPlayerPosition);
}

void GroundEmitterActorImpl::setPosition(const PxVec3& worldPlayerPosition)
{
	// put the world player position in the ground emitter's space
	mLocalPlayerPosition = mInversePose.transform(worldPlayerPosition);

	mHorizonPlane = PxPlane(mLocalPlayerPosition, mLocalUpDirection);
	PxVec3 dir = mLocalPlayerPosition - mHorizonPlane.project(mLocalPlayerPosition);
	mStepsize = dir.normalize();

	mWorldPlayerPosition = worldPlayerPosition;

	// keep track of when we move 1/2 a grid cell in distance
	float dist = (mOldLocalPlayerPosition - mLocalPlayerPosition).magnitudeSquared();
	if (dist > (mGridCellSize * mGridCellSize * 0.25))
	{
		mOldLocalPlayerPosition = mLocalPlayerPosition;
	}
}


void GroundEmitterActorImpl::setRadius(float r)
{
	WRITE_ZONE();
	mRadius = r;
	mRadius2 = mRadius * mRadius;
	mMaxStepSize = 2 * mRadius;
	mCircleArea = PxPi * mRadius2;
}

#ifdef WITHOUT_DEBUG_VISUALIZE
void GroundEmitterActorImpl::visualize(RenderDebugInterface&)
{
}
#else
void GroundEmitterActorImpl::visualize(RenderDebugInterface& renderDebug)
{
	if (!mScene->mEmitterDebugRenderParams->groundEmitterParameters.VISUALIZE_GROUND_EMITTER_ACTOR)
	{
		return;
	}
	if ( !mEnableDebugVisualization ) return;

	using RENDER_DEBUG::DebugColors;

	RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();

	RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Green), RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Yellow));

	if (mScene->mEmitterDebugRenderParams->groundEmitterParameters.VISUALIZE_GROUND_EMITTER_SPHERE)
	{
		RENDER_DEBUG_IFACE(&renderDebug)->debugSphere(mWorldPlayerPosition, mRadius);
	}


	// Determine bounds of grid cells around the player
	PxVec3 pos = mHorizonPlane.project(mLocalPlayerPosition);
	PxVec3 max = mHorizonPlane.project(mLocalPlayerPosition + PxVec3(mRadius));
	PxVec3 min = mHorizonPlane.project(mLocalPlayerPosition - PxVec3(mRadius));

	if (mGridCellSize == 0.0f)
	{
		RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
		return;
	}

	if (mScene->mEmitterDebugRenderParams->groundEmitterParameters.VISUALIZE_GROUND_EMITTER_RAYCAST)
	{

		if (mVisualizeRaycastsList.size() > 0)
		{
			PxVec3 rayOffset = mRaycastHeight * mLocalUpDirection;

			//NOTE: this array should be processed whenever it has stuff in it, even if vis has been disabled, otherwise it may never get cleared!
			for (physx::Array<RaycastVisInfo>::Iterator i = mVisualizeRaycastsList.begin(); i != mVisualizeRaycastsList.end(); i++)
			{
				//was it deleted less than a second ago?  If yes, draw it, otherwise remove it from the array.
				uint32_t timeAdded = (*i).timeSubmittedMs;
				if (timeAdded + 1000 < mTotalElapsedTimeMs)
				{
					mVisualizeRaycastsList.replaceWithLast(static_cast<uint32_t>(i - mVisualizeRaycastsList.begin()));
					i--;
				}
				else
				{
					PxVec3 worldStart, worldStop, localStop;
					worldStart = mPose.transform((*i).rayStart + rayOffset);
					localStop = (*i).rayStart;
					localStop.y = 0.0f;
					worldStop = mPose.transform(localStop);
					RENDER_DEBUG_IFACE(&renderDebug)->debugLine(worldStart, worldStop);
				}
			}
		}

		// put the raycast direction on top of the grid (2 around the center)
#define GE_DEBUG_RAY_THICKNESS (0.05f)

		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentArrowSize(GE_DEBUG_RAY_THICKNESS * 4);

		PxVec3 localPlayerZeroedPosition(mLocalPlayerPosition.x, 0.0f, mLocalPlayerPosition.z);
		PxVec3 worldRayStart, worldRayStop;
		PxVec3 localRayStart(localPlayerZeroedPosition + PxVec3(mGridCellSize, mRaycastHeight + mLocalPlayerPosition.y, mGridCellSize));
		PxVec3 localRayStop(localPlayerZeroedPosition + PxVec3(mGridCellSize, 0.0f, mGridCellSize));

		worldRayStart = mPose.transform(localRayStart);
		worldRayStop = mPose.transform(localRayStop);
		RENDER_DEBUG_IFACE(&renderDebug)->debugThickRay(worldRayStart, worldRayStop, GE_DEBUG_RAY_THICKNESS);

		localRayStart = PxVec3(localPlayerZeroedPosition + PxVec3(-mGridCellSize, mRaycastHeight + mLocalPlayerPosition.y, -mGridCellSize));
		localRayStop = PxVec3(localPlayerZeroedPosition + PxVec3(-mGridCellSize, 0.0f, -mGridCellSize));

		worldRayStart = mPose.transform(localRayStart);
		worldRayStop = mPose.transform(localRayStop);
		RENDER_DEBUG_IFACE(&renderDebug)->debugThickRay(worldRayStart, worldRayStop, GE_DEBUG_RAY_THICKNESS);
	}

	if (mScene->mEmitterDebugRenderParams->groundEmitterParameters.VISUALIZE_GROUND_EMITTER_GRID)
	{
		// draw a grid on the ground plane representing the emitter grid and one at mRaycastHeight + playerHeight
		// draw two grids, one at 0
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Yellow), RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Yellow));
		for (uint32_t i = 0; i < 2; i++)
		{
			float gridY = i * (mRaycastHeight + mLocalPlayerPosition.y);
			for (float x = min.x; x <= max.x; x += mGridCellSize)
			{
				// draw "vertical lines" (on a piece of paper)
				PxVec3 p0(x, gridY, min.z), p1(x, gridY, max.z), worldP0, worldP1;

				worldP0 = mPose.transform(p0);
				worldP1 = mPose.transform(p1);

				RENDER_DEBUG_IFACE(&renderDebug)->debugLine(worldP0, worldP1);
			}
			for (float z = min.z; z <= max.z; z += mGridCellSize)
			{
				// draw "horizontal lines" (on a piece of paper)
				PxVec3 p0(min.x, gridY, z), p1(max.x, gridY, z), worldP0, worldP1;

				worldP0 = mPose.transform(p0);
				worldP1 = mPose.transform(p1);

				RENDER_DEBUG_IFACE(&renderDebug)->debugLine(worldP0, worldP1);
			}
		}

		// draw a big box around the grids
		PxVec3	bmin(min.x, 0.0f, min.z),
		        bmax(max.x, mRaycastHeight + mLocalPlayerPosition.y, max.z),
		        bWorldMin,
		        bWorldMax;

		bWorldMin = mPose.transform(bmin);
		bWorldMax = mPose.transform(bmax);

		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Yellow));
		RENDER_DEBUG_IFACE(&renderDebug)->debugBound(PxBounds3(bWorldMin, bWorldMax));
	}

	if (mScene->mEmitterDebugRenderParams->groundEmitterParameters.VISUALIZE_GROUND_EMITTER_ACTOR_NAME)
	{
		PxVec3 bLocalMax(max.x, mRaycastHeight, max.z);
		PxVec3 bWorldMax;
		bWorldMax = mPose.transform(bLocalMax);

		RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentTextScale(2.0f);

		PxVec3 textLocation = bWorldMax;
		RENDER_DEBUG_IFACE(&renderDebug)->debugText(textLocation, " %s %s", this->getOwner()->getObjTypeName(), this->getOwner()->getName());

		RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
	}

	if (mScene->mEmitterDebugRenderParams->groundEmitterParameters.VISUALIZE_GROUND_EMITTER_ACTOR_POSE)
	{
		PxMat44 groundEmitterAxes = getRotation();
		groundEmitterAxes.setPosition(getPosition());
		RENDER_DEBUG_IFACE(&renderDebug)->debugAxes(groundEmitterAxes, 1);
	}

	RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
}
#endif


/**
 * Add an IOFX/IOS pair to a material.
 */
bool GroundEmitterActorImpl::addMeshForGroundMaterial(
	const MaterialFactoryMappingDesc& desc,
	const EmitterLodParamDesc& lodDesc)
{
	ApexSDKIntl* sdk = mAsset->mModule->mSdk;
	ResourceProviderIntl* nrp = sdk->getInternalResourceProvider();

	/* Resolve the actual MaterialIndex from the provided name */
	ResID pmns = sdk->getPhysicalMaterialNameSpace();
	ResID matresid = nrp->createResource(pmns, desc.physicalMaterialName);
	PxMaterialTableIndex groundMaterialIndex = (PxMaterialTableIndex)(size_t) nrp->getResource(matresid);

	uint32_t injectorIndex;
	for (injectorIndex = 0; injectorIndex < mInjectorList.size(); injectorIndex++)
	{
		if (mInjectorList[injectorIndex]->iofxAssetName == desc.instancedObjectEffectsAssetName &&
		        mInjectorList[injectorIndex]->iosAssetName == desc.instancedObjectSimulationAssetName)
		{
			break;
		}
	}

	if (injectorIndex >= mInjectorList.size())
	{
		InjectorData* data = PX_NEW(InjectorData)();
		data->initTask(*this, *data);

		const char* iofxAssetName = desc.instancedObjectEffectsAssetName;
		IofxAsset* iofxAsset = static_cast<IofxAsset*>(mAsset->mIofxAssetTracker.getAssetFromName(iofxAssetName));
		IosAsset* iosAsset = mAsset->mIosAssetTracker.getIosAssetFromName(
			desc.instancedObjectSimulationTypeName,
			desc.instancedObjectSimulationAssetName);

		if (!iosAsset || !iofxAsset)
		{
			delete data;
			return false;
		}

		Actor* nxActor = iosAsset->createIosActor(*mScene->mApexScene, iofxAsset);
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
			APEX_DEBUG_INFO("IOS asset retrieval failure: %s", desc.instancedObjectSimulationAssetName);
			delete data;
			return false;
		}

		/* Keep list of unique ios pointers */
		uint32_t i;
		for (i = 0 ; i < mIosList.size() ; i++)
		{
			if (mIosList[i] == ios)
			{
				break;
			}
		}
		if (i == mIosList.size())
		{
			mIosList.pushBack(ios);
		}

		data->mInjector = ios->allocateInjector(iofxAsset);
		if (!data->mInjector)
		{
			delete data;
			return false;
		}

		data->mInjector->addSelfToContext(*this);
		data->mObjectRadius = ios->getObjectRadius();
		if (!data->mInjector)
		{
			APEX_DEBUG_INFO("IOS injector allocation failure");
			delete data;
			return false;
		}

		data->mInjector->setLODWeights(lodDesc.maxDistance, lodDesc.distanceWeight, lodDesc.speedWeight,
			lodDesc.lifeWeight, lodDesc.separationWeight, lodDesc.bias);

		mInjectorList.pushBack(data);
	}

	mPerMaterialData.use(groundMaterialIndex);
	MaterialData& data = mPerMaterialData.direct(groundMaterialIndex);

	data.injectorIndices.pushBack(injectorIndex);
	float weight = desc.weight;
	if (data.accumWeights.size() > 0)
	{
		weight += data.accumWeights.back();
	}
	data.accumWeights.pushBack(weight);
	// Store the sine of the complimentary angle for comparison
	float angleCompSin = PxSin((90.0f - desc.maxSlopeAngle) * PxPi / 180.0f);
	data.maxSlopeAngles.pushBack(angleCompSin);

	return true;
}

/**
 * Submit up to mMaxNumRaycastsPerFrame raycasts to the PhysX engine.  These will be
 * processed asynchronously and we'll get the results in fetchResults().
 */
void GroundEmitterActorImpl::submitRaycasts()
{
	PX_PROFILE_ZONE("GroundParticlesEmitterRaycasts", GetInternalApexSDK()->getContextId());

	/* Avoid raycasts if we have material callback and spawn height */
	if (mMaterialCallback && mSpawnHeight > 0.0f && mToRaycast.size())
	{
		uint32_t nbHits = mToRaycast.size();
		mMaterialRequestArray.resize(nbHits);
		MaterialLookupCallback::MaterialRequest* matRequest = &mMaterialRequestArray[0];
		for (uint32_t i = 0 ; i < nbHits ; i++)
		{
			PxVec3 position;
			if (mToRaycast.popFront(position))
			{
				matRequest[i].samplePosition = mPose.transform(position);
			}
		}

		mMaterialCallback->requestMaterialLookups(nbHits, matRequest);

		nvidia::Mutex::ScopedLock scopeLock(mInjectorDataLock);

		for (uint32_t i = 0 ; i < nbHits ; i++)
		{
			uint16_t matIndex = (uint16_t) matRequest[i].outMaterialID;

			if (!mPerMaterialData.isValid(matIndex))
			{
				continue;
			}
			if (!mPerMaterialData.isUsed(matIndex))
			{
				continue;
			}

			MaterialData& data = mPerMaterialData.direct(matIndex);
			float maxSlopeAngle;
			uint32_t particleFactoryIndex = data.chooseIOFX(maxSlopeAngle, mRand);
			InjectorData& injectorData = *mInjectorList[ particleFactoryIndex ];

			PX_ASSERT(mLocalUpDirection == PxVec3(0, 1, 0));

			IosNewObject particle;

			// the spawn height must be added to the "up axis", so transform pos back to local,
			// offset, then back to world
			PxVec3 localPosition, worldPosition;
			localPosition = mInversePose.transform(matRequest[i].samplePosition);
			localPosition += PxVec3(0.0f, mSpawnHeight, 0.0f);
			particle.initialPosition = mPose.transform(localPosition);

			particle.lodBenefit = 0;
			particle.iofxActorID = IofxActorIDIntl(0);
			particle.userData = 0;

			particle.initialVelocity = mRand.getScaled(getVelocityLow(), getVelocityHigh());
			particle.lifetime = mRand.getScaled(getLifetimeLow(), getLifetimeHigh());
			injectorData.particles.pushBack(particle);
		}
	}
	else
	{
		physx::PxFilterData* groupsMask = (mShouldUseGroupsMask) ? &mRaycastCollisionGroupsMask : NULL;
		PxVec3 rayOffset = mRaycastHeight * mLocalUpDirection;
		uint32_t numRaycastsDone = 0;

		PxVec3	orig;
		PxVec3	dir;
		PxRaycastHit hit;
		PxVec3 worldUpDirection;
		worldUpDirection = mPose.transform(mLocalUpDirection);
		dir = -worldUpDirection;

		PxVec3	newPos;
		static const int DefaultRaycastHitNum	= 256;
		int				raycastHitNum			= DefaultRaycastHitNum;
		int				nbHits;
		PxRaycastHit	hitsStatck[DefaultRaycastHitNum];
		PxRaycastHit*	hits = hitsStatck;
		PxQueryFilterData	filterData(PxQueryFlag::eSTATIC);
		if (groupsMask)
			filterData.data		= *groupsMask;
		while (mToRaycast.popFront(newPos))
		{
			PxVec3 rotatedPosition;
			rotatedPosition = mPose.transform(newPos + rayOffset);
			orig = rotatedPosition;

			for(;;)
			{
				PxRaycastBuffer rcBuffer(hits, DefaultRaycastHitNum);
				mPxScene->raycast(orig, dir, PX_MAX_F32, rcBuffer,
													PxHitFlag::ePOSITION|PxHitFlag::eNORMAL|PxHitFlag::eDISTANCE|PxHitFlag::eUV,
													filterData);
				nbHits = (int32_t)rcBuffer.getNbAnyHits();
				if (nbHits != -1)
					break;

				if (hitsStatck != hits)
				{
					PX_FREE(hits);
				}
				raycastHitNum	<<= 1;	// *2
				hits			= (PxRaycastHit*)PX_ALLOC(sizeof(PxRaycastHit)*raycastHitNum, PX_DEBUG_EXP("GroundEmitterActor_PxRaycastHit"));					
			}

			if (!onRaycastQuery((uint32_t)nbHits, hits))
				break;

			if (++numRaycastsDone >= mMaxNumRaycastsPerFrame)
			{
				break;
			}
		}
		if (hits != hitsStatck)
		{
			PX_FREE(hits);
		}
	}
}


/**
 * Called during ApexScene::fetchResults() to get raycast results
 * from the PhysX engine.
 */
void GroundEmitterActorImpl::fetchResults()
{
}


/**
 * Inject all queued particles/objects into their IOS
 */
void GroundEmitterActorImpl::injectParticles(InjectorData& data)
{
	PX_PROFILE_ZONE("GroundParticlesEmitterInjection", GetInternalApexSDK()->getContextId());

	nvidia::Mutex::ScopedLock scopeLock(mInjectorDataLock);

	if (data.particles.size() > 0)
	{
		data.mInjector->createObjects(data.particles.size(), &data.particles[0]);
		data.particles.clear();
	}
}


/**
 * Iterate over all the objects spawned by this emitter actor (by iterating over its IOFX
 * instances) and determine where new objects need to be spawned.  We do this by defining a
 * 2D grid and determining the object density in each cell.  If the cell is below the appropriate
 * density, and the cell is in the appropriate refresh area, we spawn new objects to bring that
 * cell up to our density requirement.
 */
void GroundEmitterActorImpl::refreshCircle(bool edgeOnly)
{
	PX_PROFILE_ZONE("GroundParticlesEmitterRefreshCircle", GetInternalApexSDK()->getContextId());

	// Voluntarily throttle myself if any injector already
	// has a backlog of particles.
	for (uint32_t i = 0 ; i < mInjectorList.size() ; i++)
	{
		if (mInjectorList[ i ]->mInjector->isBacklogged())
		{
			return;
		}
	}

	// Simulation steps required before a grid can be re-refreshed.  There's a trade-off here
	// between constant refresh for areas that have a high slope and cannot support particles
	// and between players out-running the refresh radius.  This number must be > 2 in order to
	// protect against multi-frame particle emission latencies.
#define MIN_CELL_REFRESH_STEPS 12

	// Hack, for now, to simplify the math so we can debug functionality
	// DoD and SimpleVegetation are both Y up.
	PX_ASSERT(mLocalUpDirection == PxVec3(0, 1, 0));

	mCurrentDensity = mAsset->getDensity() * mAsset->mModule->getGroundDensityScale();

	// Calculate grid size based on mCurrentDensity and mRadius
	float gridSize = mRadius / 4;
	while (mCurrentDensity * gridSize * gridSize < 20)
	{
		gridSize *= 1.5;
	}

	while (mCurrentDensity * gridSize * gridSize > 40)
	{
		gridSize *= 0.5;
	}

	// clear the mCellLastRefreshSteps list and the grid/cell loop counters if the grid size changes
	if (gridSize != mGridCellSize)
	{
		for (uint32_t i = 0; i < mCellLastRefreshSteps.size(); i++)
		{
			mCellLastRefreshSteps[i] = 0;
		}
		mNextGridCell.x = 0;
		mNextGridCell.y = 0;
	}

	// clear the mCellLastRefreshSteps list if the player moves more than 1/2 a grid cell in distance
	if (mOldLocalPlayerPosition == mLocalPlayerPosition && mStepsize > 0.0f)
	{
		for (uint32_t i = 0; i < mCellLastRefreshSteps.size(); i++)
		{
			mCellLastRefreshSteps[i] = 0;
		}
	}

	// persist grid size
	mGridCellSize = gridSize;

	// Determine bounds of grid cells around the player
	float heightFudge = mSpawnHeight / 5; // this should be authorable...
	PxVec3 pos = mHorizonPlane.project(mLocalPlayerPosition);
	PxVec3 max = mHorizonPlane.project(mLocalPlayerPosition + PxVec3(mRadius));
	PxVec3 min = mHorizonPlane.project(mLocalPlayerPosition - PxVec3(mRadius));
	int32_t xMin, xMax, yMin, yMax;

	xMin = (int32_t) PxFloor(min.x / gridSize);
	xMax = (int32_t) PxCeil(max.x / gridSize);

	yMin = (int32_t) PxFloor(min.z / gridSize);
	yMax = (int32_t) PxCeil(max.z / gridSize);

	// Allocate grid of uint32_t counters based on grid size and mRadius
	uint32_t gridDim = (uint32_t)PxMax(yMax - yMin, xMax - xMin);
	uint32_t totalGrids = gridDim * gridDim;
	uint32_t* grids = (uint32_t*) mAsset->mModule->mSdk->getTempMemory(totalGrids * sizeof(uint32_t));
	mCellLastRefreshSteps.resize(totalGrids, (uint32_t) - MIN_CELL_REFRESH_STEPS);
	memset(grids, 0, totalGrids * sizeof(uint32_t));

	// This loop should be in a CUDA kernel
	for (uint32_t i = 0;  i < mIosList.size(); i++)
	{
		uint32_t count, stride;
		InstancedObjectSimulationIntl* ios = mIosList[i];
		const PxVec3* positions = ios->getRecentPositions(count, stride);
		const char* ptr = reinterpret_cast<const char*>(positions);
		const PxVec3* worldPositionPtr;
		PxVec3 localPosition;

		for (uint32_t j = 0 ; j < count ; j++)
		{
			worldPositionPtr = reinterpret_cast<const PxVec3*>(ptr);
			localPosition = mInversePose.transform(*worldPositionPtr);
			ptr += stride;
			PxVec3 proj = mHorizonPlane.project(localPosition);
			int32_t cx = (int32_t) PxFloor(proj.x / gridSize) - xMin;
			if (cx < 0 || cx >= (int32_t) gridDim)
			{
				continue;
			}

			int32_t cy = (int32_t) PxFloor(proj.z / gridSize) - yMin;
			if (cy < 0 || cy >= (int32_t) gridDim)
			{
				continue;
			}

			float dist = localPosition.y - mLocalPlayerPosition.y;
			if ((mSpawnHeight > 0.0f) &&
			        ((dist < (mSpawnHeight - heightFudge)) || (dist > (mSpawnHeight + heightFudge))))
			{
				continue;
			}

			grids[ cy * gridDim + cx ]++;
		}
	}

	{
		PX_PROFILE_ZONE("GroundParticlesEmitterGridInspect", GetInternalApexSDK()->getContextId());
		// Iterate over grid.  For each under density threshold, generate
		// new particles to bring it up to spec.
		uint32_t neededOccupancy = (uint32_t) PxCeil(mCurrentDensity * gridSize * gridSize * 0.10f);

		bool stopScanningGridCells = false;

		for (int32_t x = 0 ; x < (int32_t) gridDim ; x++)
		{
			if (stopScanningGridCells)
			{
				break;
			}

			int32_t cellx = INCREMENT_CELL(mNextGridCell.x, x, (int32_t)(gridDim - 1));

			float fx = (cellx + xMin) * gridSize;
			for (int32_t y = 0 ; y < (int32_t) gridDim ; y++)
			{
				int32_t celly = INCREMENT_CELL(mNextGridCell.y, y, (int32_t)(gridDim - 1));
				float fy = (celly + yMin) * gridSize;

				if (edgeOnly)
				{
					// Ignore grids that do not include the radius.  This is a horseshoe calculation
					// that tests whether the grid center is more than gridSize from the radius.
					float cx = (fx + gridSize * 0.5f) - pos.x;
					float cy = (fy + gridSize * 0.5f) - pos.z;
					float distsq = cx * cx + cy * cy;
					if (fabs(distsq - mRadius2) > gridSize * gridSize)
					{
						continue;
					}
				}

				uint32_t gridID = (uint32_t) celly * gridDim + cellx;
				uint32_t gridOccupancy = grids[ gridID ];
				if (gridOccupancy >= neededOccupancy)
				{
					continue;
				}

				// Do not refresh a grid more often than once every half second
				if (mSimulationSteps - mCellLastRefreshSteps[ gridID ] < MIN_CELL_REFRESH_STEPS)
				{
					continue;
				}

				// Refresh this grid
				uint32_t numRaycasts = (uint32_t) PxCeil(mCurrentDensity * gridSize * gridSize) - gridOccupancy;

				// If this cell pushes us over the max raycast count, take what we can from the cell and run
				// it again next frame.  This does not apply to the first frame (edgeOnly)
				if (!mRefreshFullCircle && ((mToRaycast.size() + numRaycasts) > getMaxRaycastsPerFrame()))
				{
					if (mToRaycast.size() > getMaxRaycastsPerFrame())
					{
						numRaycasts = 0;
					}
					else
					{
						numRaycasts = getMaxRaycastsPerFrame() - mToRaycast.size();
					}
				}

				// compute the positions of the new raycasts
				bool visualizeRaycasts = mScene->mEmitterDebugRenderParams->groundEmitterParameters.VISUALIZE_GROUND_EMITTER_RAYCAST &&
				                         mScene->mDebugRenderParams->Enable;
				// safety valve, in case no one is actually rendering
				if (mVisualizeRaycastsList.size() > 16 * 1024)
				{
					visualizeRaycasts = false;
				}

				for (uint32_t j = 0; j < numRaycasts; j++)
				{
					float tmpx = mRand.getScaled(0, gridSize);
					float tmpy = mRand.getScaled(0, gridSize);
					mToRaycast.pushBack(PxVec3(fx + tmpx, mLocalPlayerPosition.y, fy + tmpy));

					if (visualizeRaycasts)
					{
						RaycastVisInfo& raycastInfo = mVisualizeRaycastsList.insert();
						raycastInfo.rayStart = PxVec3(fx + tmpx, mLocalPlayerPosition.y, fy + tmpy);
						raycastInfo.timeSubmittedMs = mTotalElapsedTimeMs;
					}
				}

				// break out if the raycast buffer will grow beyond the max raycasts per frame
				if (!mRefreshFullCircle && (mToRaycast.size() >= getMaxRaycastsPerFrame()))
				{
					// save the next cell in the grid to continue scanning
					if (cellx == (int32_t)gridDim - 1 && celly == (int32_t)gridDim - 1)
					{
						mNextGridCell.x = mNextGridCell.y = 0;
					}
					else if (cellx == (int32_t)gridDim - 1)
					{
						mNextGridCell.x = INCREMENT_CELL(cellx, 1, (int32_t)(gridDim - 1));
					}
					else
					{
						mNextGridCell.x = cellx;
					}

					mNextGridCell.y = INCREMENT_CELL(celly, 1, (int32_t)(gridDim - 1));

					stopScanningGridCells = true;
					break;
				}

				mCellLastRefreshSteps[ gridID ] = mSimulationSteps;
			}
		}
	}

	mAsset->mModule->mSdk->releaseTempMemory(grids);
}

void GroundEmitterActorImpl::tick()
{
	float dt = mScene->mApexScene->getElapsedTime();

	mTotalElapsedTimeMs = mTotalElapsedTimeMs + (uint32_t)(1000.0f * dt);
	PX_PROFILE_ZONE("GroundParticlesEmitterTick", GetInternalApexSDK()->getContextId());

	mSimulationSteps++;

	//TODO: make more localize locks
	SCOPED_PHYSX_LOCK_WRITE(mScene->mApexScene);

	if (mAttachActor)
	{
		PxTransform t = mAttachActor->is<physx::PxRigidDynamic>()->getGlobalPose();
		setPosition(t.p + mAttachRelativePosition);
	}

	// generate new raycast positions based on refresh requirements
	refreshCircle((mStepsize < mMaxStepSize) && (mSpawnHeight == 0.0f) && !mRefreshFullCircle);
	mRefreshFullCircle = false;

	if (mToRaycast.size())
	{
		submitRaycasts();
	}
}


/**
 * Raycast callback which is triggered by calling mQueryObject->finish()
 */
bool GroundEmitterActorImpl::onRaycastQuery(uint32_t nbHits, const PxRaycastHit* hits)
{
	PX_UNUSED(userData);

	PX_PROFILE_ZONE("GroundParticlesEmitterOnRaycastQuery", GetInternalApexSDK()->getContextId());

	if (!nbHits)
	{
		return true;
	}

	MaterialLookupCallback::MaterialRequest* matRequest = NULL;
	if (mMaterialCallback)
	{
		mMaterialRequestArray.resize(nbHits);
		matRequest = &mMaterialRequestArray[0];
		for (uint32_t i = 0 ; i < nbHits ; i++)
		{
			matRequest[i].samplePosition = hits[i].position;
		}
		mMaterialCallback->requestMaterialLookups(nbHits, matRequest);
	}

	nvidia::Mutex::ScopedLock scopeLock(mInjectorDataLock);

	PxVec3 worldUpDirection;
	worldUpDirection = getPose().rotate(mLocalUpDirection);

	for (uint32_t i = 0; i < nbHits; i++)
	{
		const PxRaycastHit& hit = hits[i];

		// TODO 3.0 apan, check matRequest!!
		//this->mScene->mPhysXScene->getPhysics().getNbMaterials
//		PxMaterial*	materail	= hit.shape->getMaterialFromInternalFaceIndex(hit.faceIndex);
		uint16_t matIndex = matRequest ? (uint16_t) matRequest[i].outMaterialID : (uint16_t) 0;//materail;	// apan, fixme, hard code to 0
		if (mPerMaterialData.isValid(matIndex) &&
		        mPerMaterialData.isUsed(matIndex))
		{
			PX_ASSERT(PxAbs(1.0f - hit.normal.magnitude()) < 0.001f); // assert normal is normalized

			MaterialData& data = mPerMaterialData.direct(matIndex);
			float maxSlopeAngle = 0.0f;
			uint32_t particleFactoryIndex = data.chooseIOFX(maxSlopeAngle, mRand);

			float upNormal = hit.normal.dot(worldUpDirection);
			if (upNormal < maxSlopeAngle)
			{
				continue;
			}

			InjectorData& injectorData = *mInjectorList[particleFactoryIndex];
			IosNewObject particle;

			if (mSpawnHeight <= 0.0f)
			{
				particle.initialPosition	= hit.position;
				particle.initialPosition += injectorData.mObjectRadius * hit.normal;
			}
			else
			{
				PX_ASSERT(mLocalUpDirection == PxVec3(0, 1, 0));
				particle.initialPosition.x = hit.position.x;
				particle.initialPosition.z = hit.position.z;
				particle.initialPosition.y = mLocalPlayerPosition.y + mSpawnHeight;
			}

			particle.initialVelocity = mRand.getScaled(getVelocityLow(), getVelocityHigh());
			particle.lifetime = mRand.getScaled(getLifetimeLow(), getLifetimeHigh());

			particle.lodBenefit = 0;
			particle.iofxActorID = IofxActorIDIntl(0);
			particle.userData = 0;

			injectorData.particles.pushBack(particle);
		}
	}

	return true;
}

}
} // namespace nvidia::apex
