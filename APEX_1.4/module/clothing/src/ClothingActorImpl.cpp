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



#include "ApexDefs.h"
#include "PsIntrinsics.h"
#include "SimdMath.h"

#include "ClothingActorImpl.h"
#include "ClothingActorProxy.h"
#include "ClothingCooking.h"
#include "ClothingPhysicalMesh.h"
#include "ClothingPreviewProxy.h"
#include "ClothingScene.h"
#include "CookingPhysX.h"
#include "ModuleClothing.h"
#include "ModulePerfScope.h"
#include "ClothingVelocityCallback.h"
#include "RenderMeshActorDesc.h"
#include "SimulationAbstract.h"

#include "ClothingGraphicalLodParameters.h"
#include "DebugRenderParams.h"

#include "SceneIntl.h"
#include "ApexSDKIntl.h"
#include "ApexUsingNamespace.h"
#include "PsAtomic.h"

#include "ApexMath.h"

#if PX_PHYSICS_VERSION_MAJOR == 3
#include "PxScene.h"
//#include "ApexReadWriteLock.h"
#endif

#include "PsVecMath.h"

#include "PxStrideIterator.h"

#include "Simulation.h"

#include "ApexPvdClient.h"

namespace nvidia
{
namespace clothing
{

ClothingActorImpl::ClothingActorImpl(const NvParameterized::Interface& descriptor, ClothingActorProxy* actorProxy,
                             		ClothingPreviewProxy* previewProxy, ClothingAssetImpl* asset, ClothingScene* scene) :
	mActorProxy(actorProxy),
	mPreviewProxy(previewProxy),
	mAsset(asset),
	mClothingScene(scene),
#if PX_PHYSICS_VERSION_MAJOR == 3
	mPhysXScene(NULL),
#endif
	mActorDesc(NULL),
	mBackendName(NULL),
	mInternalGlobalPose(PxVec4(1.0f)),
	mOldInternalGlobalPose(PxVec4(0.0f)),
	mInternalInterpolatedGlobalPose(PxVec4(1.0f)),
	mInternalInterpolatedBoneMatrices(NULL),
	mCurrentSolverIterations(0),
	mInternalScaledGravity(0.0f, 0.0f, 0.0f),
	mInternalMaxDistanceBlendTime(0.0f),
	mMaxDistReduction(0.0f),
	mBufferedGraphicalLod(0),
	mCurrentGraphicalLodId(0),
	mRenderProxyReady(NULL),
	mRenderProxyURR(NULL),
	mClothingSimulation(NULL),
	mCurrentMaxDistanceBias(0.0f),
	bIsSimulationOn(false),
	mForceSimulation(-1),
	mLodCentroid(0.0f, 0.0f, 0.0f),
	mLodRadiusSquared(0.0f),
	mVelocityCallback(NULL),
	mInterCollisionChannels(0),
	mIsAllowedHalfPrecisionSolver(false),
	mBeforeTickTask(this),
	mDuringTickTask(this),
	mFetchResultsTask(this),
	mActiveCookingTask(NULL),
	mFetchResultsRunning(false),
	bGlobalPoseChanged(1),
	bBoneMatricesChanged(1),
	bBoneBufferDirty(0),
	bMaxDistanceScaleChanged(0),
	bBlendingAllowed(1),
	bDirtyActorTemplate(0),
	bDirtyShapeTemplate(0),
	bDirtyClothingTemplate(0),
	bBufferedVisible(1),
	bInternalVisible(1),
	bUpdateFrozenFlag(0),
	bBufferedFrozen(0),
	bInternalFrozen(0),
	bPressureWarning(0),
	bUnsucessfullCreation(0),
	bInternalTeleportDue(ClothingTeleportMode::Continuous),
	bInternalScaledGravityChanged(1),
	bReinitActorData(0),
	bInternalLocalSpaceSim(0),
	bActorCollisionChanged(0)
{
	//mBufferedBoneMatrices = NULL;

	if ((((size_t)this) & 0xf) != 0)
	{
		APEX_INTERNAL_ERROR("ClothingActorImpl is not 16 byte aligned");
	}
	// make sure the alignment is ok
	if ((((size_t)&mInternalGlobalPose) & 0xf) != 0)
	{
		APEX_INTERNAL_ERROR("Matrix ClothingActorImpl::mInternalGlobalPose is not 16 byte aligned");
	}
	if ((((size_t)&mOldInternalGlobalPose) & 0xf) != 0)
	{
		APEX_INTERNAL_ERROR("Matrix ClothingActorImpl::mOldInternalGlobalPose is not 16 byte aligned");
	}
	if ((((size_t)&mInternalInterpolatedGlobalPose) & 0xf) != 0)
	{
		APEX_INTERNAL_ERROR("Matrix ClothingActorImpl::mInternalInterpolatedGlobalPose is not 16 byte aligned");
	}

	if (::strcmp(descriptor.className(), ClothingActorParam::staticClassName()) == 0)
	{
		PX_ASSERT(mActorProxy != NULL);

		mActorDesc = static_cast<ClothingActorParam*>(GetInternalApexSDK()->getParameterizedTraits()->createNvParameterized(ClothingActorParam::staticClassName()));
		PX_ASSERT(mActorDesc != NULL);
		mActorDesc->copy(descriptor);
		PX_ASSERT(mActorDesc->equals(descriptor, NULL, 0));

		const ClothingActorParamNS::ParametersStruct& actorDesc = static_cast<const ClothingActorParamNS::ParametersStruct&>(*mActorDesc);

		// initialize these too
		mInternalWindParams = mActorDesc->windParams;
		mInternalMaxDistanceScale = mActorDesc->maxDistanceScale;
		mInternalFlags = mActorDesc->flags;
		bInternalLocalSpaceSim = mActorDesc->localSpaceSim ? 1u : 0u;

		mInterCollisionChannels = mAsset->getInterCollisionChannels();

		// Physics is turned off initially
		mCurrentSolverIterations = 0;

		if (actorDesc.slowStart)
		{
			mMaxDistReduction = mAsset->getBiggestMaxDistance();
		}


		mNewBounds.setEmpty();

		// prepare some runtime data for each graphical mesh
		mGraphicalMeshes.reserve(mAsset->getNumGraphicalMeshes());
		uint32_t vertexOffset = 0;
		for (uint32_t i = 0; i < mAsset->getNumGraphicalMeshes(); i++)
		{
			ClothingGraphicalMeshActor actor;

			RenderMeshAssetIntl* renderMeshAsset = mAsset->getGraphicalMesh(i);
			// it can be NULL if ClothingAsset::releaseGraphicalData has beend called to do skinning externally
			if (renderMeshAsset != NULL) 
			{
				const uint32_t numSubmeshes = renderMeshAsset->getSubmeshCount();

				for (uint32_t si = 0; si < numSubmeshes; ++si)
				{
					actor.morphTargetVertexOffsets.pushBack(vertexOffset);
					vertexOffset += renderMeshAsset->getSubmesh(si).getVertexCount(0);
				}
			}
			else
			{
				actor.morphTargetVertexOffsets.pushBack(0);
			}

			actor.active = i == 0;
			mGraphicalMeshes.pushBack(actor);
		}

		// When we add ourselves to the ApexScene, it will call us back with setPhysXScene
		addSelfToContext(*mClothingScene->mApexScene->getApexContext());

		// Add ourself to our ClothingScene
		addSelfToContext(*static_cast<ApexContext*>(mClothingScene));

		// make sure the clothing material gets initialized when
		// applyClothingMaterial is called the first time
		mClothingMaterial.solverIterations = uint32_t(-1);

		if (::strcmp(mActorDesc->simulationBackend, "Default") == 0)
		{
			mBackendName = "Embedded";
		}
		else if (::strcmp(mActorDesc->simulationBackend, "ForceEmbedded") == 0)
		{
			mBackendName = "Embedded";
		}
		else
		{
			mBackendName = "Native";
		}

		if (mActorDesc->morphDisplacements.arraySizes[0] > 0)
		{
			PX_PROFILE_ZONE("ClothingActorImpl::morphTarget", GetInternalApexSDK()->getContextId());

			if (mActorDesc->morphPhysicalMeshNewPositions.buf == NULL)
			{
				ParamArray<PxVec3> morphPhysicalNewPos(mActorDesc, "morphPhysicalMeshNewPositions", reinterpret_cast<ParamDynamicArrayStruct*>(&mActorDesc->morphPhysicalMeshNewPositions));
				mAsset->getDisplacedPhysicalMeshPositions(mActorDesc->morphDisplacements.buf, morphPhysicalNewPos);

				CookingAbstract* cookingJob = mAsset->getModuleClothing()->getBackendFactory(mBackendName)->createCookingJob();
				PX_ASSERT(cookingJob != NULL);

				if (cookingJob != NULL)
				{
					PxVec3 gravity = scene->mApexScene->getGravity();
					gravity = mActorDesc->globalPose.inverseRT().rotate(gravity);
					mAsset->prepareCookingJob(*cookingJob, mActorDesc->actorScale, &gravity, morphPhysicalNewPos.begin());

					if (cookingJob->isValid())
					{
						if (mAsset->getModuleClothing()->allowAsyncCooking())
						{
							mActiveCookingTask = PX_NEW(ClothingCookingTask)(mClothingScene, *cookingJob);
							mActiveCookingTask->lockObject(mAsset);
							mClothingScene->submitCookingTask(mActiveCookingTask);
						}
						else
						{
							mActorDesc->runtimeCooked = cookingJob->execute();
							PX_DELETE_AND_RESET(cookingJob);
						}
					}
					else
					{
						PX_DELETE_AND_RESET(cookingJob);
					}
				}
			}

			if (mActorDesc->morphGraphicalMeshNewPositions.buf == NULL)
			{
				ParamArray<PxVec3> morphGraphicalNewPos(mActorDesc, "morphGraphicalMeshNewPositions", reinterpret_cast<ParamDynamicArrayStruct*>(&mActorDesc->morphGraphicalMeshNewPositions));

				uint32_t graphicalVertexCount = 0;
				for (uint32_t gi = 0; gi < mGraphicalMeshes.size(); ++gi)
				{
					RenderMeshAssetIntl* renderMeshAsset = mAsset->getGraphicalMesh(gi);

					const ClothingGraphicalMeshAssetWrapper meshAsset(renderMeshAsset);
					graphicalVertexCount += meshAsset.getNumTotalVertices();
				}

				morphGraphicalNewPos.resize(graphicalVertexCount);

				uint32_t vertexOffset = 0;
				for (uint32_t gi = 0; gi < mGraphicalMeshes.size(); ++gi)
				{
					RenderMeshAssetIntl* renderMeshAsset = mAsset->getGraphicalMesh(gi);
					if (renderMeshAsset == NULL)
						continue;

					const uint32_t numSubmeshes = renderMeshAsset->getSubmeshCount();
					for (uint32_t si = 0; si < numSubmeshes; ++si)
					{
						const RenderSubmesh& submesh = renderMeshAsset->getSubmesh(si);
						const VertexFormat& format = submesh.getVertexBuffer().getFormat();

						uint32_t* morphMapping = mAsset->getMorphMapping(gi, si);

						const int32_t positionIndex = format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::POSITION));
						if (positionIndex != -1)
						{
							RenderDataFormat::Enum bufferFormat = RenderDataFormat::UNSPECIFIED;
							const PxVec3* positions = reinterpret_cast<const PxVec3*>(submesh.getVertexBuffer().getBufferAndFormat(bufferFormat, (uint32_t)positionIndex));
							PX_ASSERT(bufferFormat == RenderDataFormat::FLOAT3);
							if (bufferFormat == RenderDataFormat::FLOAT3)
							{
								const uint32_t vertexCount = submesh.getVertexCount(0);
								for (uint32_t i = 0; i < vertexCount; i++)
								{
									const PxVec3 disp = morphMapping != NULL ? mActorDesc->morphDisplacements.buf[morphMapping[i]] : PxVec3(0.0f);
									morphGraphicalNewPos[i + vertexOffset] = positions[i] + disp;
								}
							}
						}

						vertexOffset += submesh.getVertexCount(0);
					}
				}
			}
		}

		// default render proxy to handle pre simulate case
		mRenderProxyReady = mClothingScene->getRenderProxy(mAsset->getGraphicalMesh(0), mActorDesc->fallbackSkinning, false, mOverrideMaterials, mActorDesc->morphPhysicalMeshNewPositions.buf, &mGraphicalMeshes[0].morphTargetVertexOffsets[0]);

		if (getRuntimeCookedDataPhysX() != NULL && mActorDesc->actorScale != getRuntimeCookedDataPhysX()->actorScale)
		{
			mActorDesc->runtimeCooked->destroy();
			mActorDesc->runtimeCooked = NULL;
		}

		// PH: So if backend name is 'embedded', i won't get an asset cooked data ever, so it also won't complain about the cooked version, good
		// if backend is native, it might, but that's only when you force native with the 2.8.x sdk on a 3.2 asset
		// const char* cookingDataType = mAsset->getModuleClothing()->getBackendFactory(mBackendName)->getCookingJobType();
		NvParameterized::Interface* assetCookedData = mAsset->getCookedData(mActorDesc->actorScale);
		NvParameterized::Interface* actorCookedData = mActorDesc->runtimeCooked;

		BackendFactory* factory = mAsset->getModuleClothing()->getBackendFactory(mBackendName);

		uint32_t assetCookedDataVersion = factory->getCookedDataVersion(assetCookedData);
		uint32_t actorCookedDataVersion = factory->getCookedDataVersion(actorCookedData);

		if (assetCookedData != NULL && !factory->isMatch(assetCookedData->className()))
		{
			APEX_DEBUG_WARNING("Asset (%s) cooked data type (%s) does not match the current backend (%s). Recooking.",
			                   mAsset->getName(), assetCookedData->className(), mBackendName);
			assetCookedData = NULL;
		}
		// If the PhysX3 cooking format changes from 3.0 to 3.x, then APEX needs to store something other than the
		// _SDK_VERSION_NUMBER.  Perhaps _PHYSICS_SDK_VERSION (which is something like 0x03010000 or 0x02080400).
		// Currently, _SDK_VERSION_NUMBER does not change for P3, it is fixed at 300.  The PhysX2 path will continue to use
		// _SDK_VERSION_NUMBER so existing assets cooked with PhysX2 data aren't recooked by default.

		else if (assetCookedData && assetCookedDataVersion != factory->getCookingVersion())
		{
			APEX_DEBUG_WARNING("Asset (%s) cooked data version (%d/0x%08x) does not match the current sdk version. Recooking.",
			                   mAsset->getName(),
			                   assetCookedDataVersion,
			                   assetCookedDataVersion);
			assetCookedData = NULL;
		}

		if (actorCookedData != NULL && !factory->isMatch(actorCookedData->className()))
		{
			APEX_DEBUG_WARNING("Asset (%s) cooked data type (%s) does not match the current backend (%s). Recooking.",
			                   mAsset->getName(), assetCookedData->className(), mBackendName);
			actorCookedData = NULL;
		}

		if (actorCookedData && actorCookedDataVersion != factory->getCookingVersion())
		{
			APEX_DEBUG_WARNING("Actor (%s) cooked data version (%d/0x%08x) does not match the current sdk version. Recooking.",
			                   mAsset->getName(),
			                   actorCookedDataVersion,
			                   actorCookedDataVersion);
			actorCookedData = NULL;
		}

		if (assetCookedData == NULL && actorCookedData == NULL && mActiveCookingTask == NULL)
		{
			CookingAbstract* cookingJob = mAsset->getModuleClothing()->getBackendFactory(mBackendName)->createCookingJob();
			PX_ASSERT(cookingJob != NULL);

			if (cookingJob != NULL)
			{
				PxVec3 gravity = scene->mApexScene->getGravity();
				gravity = mActorDesc->globalPose.inverseRT().rotate(gravity);
				mAsset->prepareCookingJob(*cookingJob, mActorDesc->actorScale, &gravity, NULL);

				if (cookingJob->isValid())
				{
					if (mAsset->getModuleClothing()->allowAsyncCooking())
					{
						mActiveCookingTask = PX_NEW(ClothingCookingTask)(mClothingScene, *cookingJob);
						mActiveCookingTask->lockObject(mAsset);
						mClothingScene->submitCookingTask(mActiveCookingTask);
					}
					else
					{
						mActorDesc->runtimeCooked = cookingJob->execute();
						PX_DELETE_AND_RESET(cookingJob);
					}
				}
				else
				{
					PX_DELETE_AND_RESET(cookingJob);
				}
			}

		}

		mActorProxy->userData = reinterpret_cast<void*>(mActorDesc->userData);
	}
	else if (::strcmp(descriptor.className(), ClothingPreviewParam::staticClassName()) == 0)
	{
		PX_ASSERT(mPreviewProxy != NULL);

		const ClothingPreviewParam& previewDesc = static_cast<const ClothingPreviewParam&>(descriptor);
		mActorDesc = static_cast<ClothingActorParam*>(GetInternalApexSDK()->getParameterizedTraits()->createNvParameterized(ClothingActorParam::staticClassName()));
		PX_ASSERT(mActorDesc != NULL);
		mActorDesc->globalPose = previewDesc.globalPose;
		{
			NvParameterized::Handle handle(mActorDesc);
			handle.getParameter("boneMatrices");
			handle.resizeArray(previewDesc.boneMatrices.arraySizes[0]);
			for (int32_t i = 0; i < previewDesc.boneMatrices.arraySizes[0]; i++)
			{
				mActorDesc->boneMatrices.buf[i] = previewDesc.boneMatrices.buf[i];
			}
		}
		mActorDesc->useInternalBoneOrder = previewDesc.useInternalBoneOrder;
		mActorDesc->updateStateWithGlobalMatrices = previewDesc.updateStateWithGlobalMatrices;
		mActorDesc->fallbackSkinning = previewDesc.fallbackSkinning;

		//mActorDesc->copy(descriptor);
		//PX_ASSERT(mActorDesc->equals(descriptor, NULL, 0));

		// prepare some runtime data for each graphical mesh
		mGraphicalMeshes.reserve(mAsset->getNumGraphicalMeshes());
		for (uint32_t i = 0; i < mAsset->getNumGraphicalMeshes(); i++)
		{
			ClothingGraphicalMeshActor actor;
			RenderMeshAssetIntl* renderMeshAsset = mAsset->getGraphicalMesh(i);
			if (renderMeshAsset == NULL)
				continue;

			actor.active = i == 0;
			actor.morphTargetVertexOffsets.pushBack(0);
			mGraphicalMeshes.pushBack(actor);
		}
		// default render proxy to handle pre simulate case
		mRenderProxyReady = PX_NEW(ClothingRenderProxyImpl)(mAsset->getGraphicalMesh(0), mActorDesc->fallbackSkinning, false, mOverrideMaterials, mActorDesc->morphPhysicalMeshNewPositions.buf, &mGraphicalMeshes[0].morphTargetVertexOffsets[0], NULL);

		mPreviewProxy->userData = reinterpret_cast<void*>(mActorDesc->userData);
	}
	else
	{
		APEX_INVALID_PARAMETER("%s is not a valid descriptor class", descriptor.className());

		PX_ASSERT(mActorProxy == NULL);
		PX_ASSERT(mPreviewProxy == NULL);
	}

	if (mActorDesc != NULL)
	{
		// initialize overrideMaterialMap with data from actor desc
		for (uint32_t i = 0; i < (uint32_t)mActorDesc->overrideMaterialNames.arraySizes[0]; ++i)
		{
			mOverrideMaterials[i] = mActorDesc->overrideMaterialNames.buf[i];
		}

		if (mActorDesc->updateStateWithGlobalMatrices)
		{
			mAsset->setupInvBindMatrices();
		}

		uint32_t numMeshesWithTangents = 0;
		uint32_t maxVertexCount = 0;

		for (uint32_t i = 0; i < mGraphicalMeshes.size(); i++)
		{
			RenderMeshAssetIntl* renderMeshAsset = mAsset->getGraphicalMesh(i);

			const ClothingGraphicalMeshAssetWrapper meshAsset(renderMeshAsset);

			if (meshAsset.hasChannel(NULL, RenderVertexSemantic::TANGENT) && meshAsset.hasChannel(NULL, RenderVertexSemantic::BINORMAL))
			{
				PX_ALWAYS_ASSERT();
				// need to compress them into one semantic
				//APEX_INVALID_PARAMETER("RenderMeshAsset must have either TANGENT and BINORMAL semantics, or none. But not only one!");
			}

			if (meshAsset.hasChannel(NULL, RenderVertexSemantic::TANGENT) && meshAsset.hasChannel(NULL, RenderVertexSemantic::TEXCOORD0))
			{
				numMeshesWithTangents++;
				mGraphicalMeshes[i].needsTangents = true;
			}
			maxVertexCount = PxMax(maxVertexCount, meshAsset.getNumTotalVertices());
		}

		const uint32_t numBones = mActorDesc->useInternalBoneOrder ? mAsset->getNumUsedBones() : mActorDesc->boneMatrices.arraySizes[0];
		updateState(mActorDesc->globalPose, mActorDesc->boneMatrices.buf, sizeof(PxMat44), numBones, ClothingTeleportMode::Continuous);
		updateStateInternal_NoPhysX(false);
		updateBoneBuffer(mRenderProxyReady);
	}

	mRenderBounds = getRenderMeshAssetBoundsTransformed();
	bool bHasBones = mActorDesc->boneMatrices.arraySizes[0] > 0 || mAsset->getNumBones() > 0;
	if (bHasBones && bInternalLocalSpaceSim == 1)
	{
		PX_ASSERT(!mRenderBounds.isEmpty());
		mRenderBounds = PxBounds3::transformFast(PxTransform(mInternalGlobalPose), mRenderBounds);
	}

	PX_ASSERT(mRenderBounds.isFinite());
	PX_ASSERT(!mRenderBounds.isEmpty());
}



void ClothingActorImpl::release()
{
	if (mInRelease)
	{
		return;
	}

	if (isSimulationRunning())
	{
		APEX_INVALID_OPERATION("Cannot release ClothingActorImpl while simulation is still running");
		return;
	}

	waitForFetchResults();

	mInRelease = true;

	if (mActorProxy != NULL)
	{
		mAsset->releaseClothingActor(*mActorProxy);
	}
	else
	{
		PX_ASSERT(mPreviewProxy != NULL);
		mAsset->releaseClothingPreview(*mPreviewProxy);
	}
}



Renderable* ClothingActorImpl::getRenderable()
{
	// make sure the result is ready
	// this is mainly for legacy kind of rendering
	// with the renderable iterator. note that the
	// user does not acquire the render proxy here
	waitForFetchResults();

	return mRenderProxyReady;
}



void ClothingActorImpl::dispatchRenderResources(UserRenderer& api)
{
	mRenderProxyMutex.lock();
	if (mRenderProxyURR != NULL)
	{
		mRenderProxyURR->dispatchRenderResources(api);
	}
	mRenderProxyMutex.unlock();
}



void ClothingActorImpl::updateRenderResources(bool rewriteBuffers, void* userRenderData)
{
	waitForFetchResults();

	ClothingRenderProxyImpl* newRenderProxy = static_cast<ClothingRenderProxyImpl*>(acquireRenderProxy());
	if (newRenderProxy != NULL)
	{
		if (mRenderProxyURR != NULL)
		{
			mRenderProxyURR->release();
			mRenderProxyURR = NULL;
		}
		mRenderProxyURR = newRenderProxy;
	}
	if (mRenderProxyURR != NULL)
	{
		mRenderProxyURR->updateRenderResources(rewriteBuffers, userRenderData);
	}
}



NvParameterized::Interface* ClothingActorImpl::getActorDesc()
{
	if (mActorDesc != NULL && isValidDesc(*mActorDesc))
	{
		return mActorDesc;
	}

	return NULL;
}



void ClothingActorImpl::updateState(const PxMat44& globalPose, const PxMat44* newBoneMatrices, uint32_t boneMatricesByteStride, uint32_t numBoneMatrices, ClothingTeleportMode::Enum teleportMode)
{
	PX_PROFILE_ZONE("ClothingActorImpl::updateState", GetInternalApexSDK()->getContextId());

	PX_ASSERT(mActorDesc);
	const bool useInternalBoneOrder = mActorDesc->useInternalBoneOrder;

	uint32_t numElements = useInternalBoneOrder ? mAsset->getNumUsedBones() : mAsset->getNumBones();
	if (useInternalBoneOrder && (numBoneMatrices > numElements))
	{
		APEX_DEBUG_WARNING("numMatrices too big.");
		return;
	}

	mActorDesc->globalPose = globalPose;
	if (!PxEquals(globalPose.column0.magnitude(), mActorDesc->actorScale, 1e-5))
	{
		APEX_DEBUG_WARNING("Actor Scale wasn't set properly, it doesn't equal to the Global Pose scale: %f != %f",
			mActorDesc->actorScale,
			globalPose.column0.magnitude());
	}

	PX_ASSERT(newBoneMatrices == NULL || boneMatricesByteStride >= sizeof(PxMat44));
	if (boneMatricesByteStride >= sizeof(PxMat44) && newBoneMatrices != NULL)
	{
		if (mActorDesc->boneMatrices.arraySizes[0] != (int32_t)numBoneMatrices)
		{
			// PH: aligned alloc?
			NvParameterized::Handle handle(mActorDesc);
			handle.getParameter("boneMatrices");
			handle.resizeArray((int32_t)numBoneMatrices);
		}

		for (uint32_t i = 0; i < numBoneMatrices; i++)
		{
			const PxMat44* source = (const PxMat44*)(((const uint8_t*)newBoneMatrices) + boneMatricesByteStride * i);
			mActorDesc->boneMatrices.buf[i] = *source;
		}
	}
	else
	{
		NvParameterized::Handle handle(mActorDesc);
		handle.getParameter("boneMatrices");
		handle.resizeArray(0);
	}

	mActorDesc->teleportMode = teleportMode;

	if (mClothingScene == NULL)
	{
		// In Preview mode!
		updateStateInternal_NoPhysX(false);
		updateBoneBuffer(mRenderProxyReady);
	}
}



void ClothingActorImpl::updateMaxDistanceScale(float scale, bool multipliable)
{
	PX_ASSERT(mActorDesc != NULL);
	mActorDesc->maxDistanceScale.Scale = PxClamp(scale, 0.0f, 1.0f);
	mActorDesc->maxDistanceScale.Multipliable = multipliable;
}



const PxMat44& ClothingActorImpl::getGlobalPose() const
{
	PX_ASSERT(mActorDesc != NULL);
	return mActorDesc->globalPose;
}



void ClothingActorImpl::setWind(float windAdaption, const PxVec3& windVelocity)
{
	if (windAdaption < 0.0f)
	{
		APEX_INVALID_PARAMETER("windAdaption must be bigger or equal than 0.0 (is %f)", windAdaption);
		windAdaption = 0.0f;
	}

	PX_ASSERT(mActorDesc);
	mActorDesc->windParams.Adaption = windAdaption;
	mActorDesc->windParams.Velocity = windVelocity;
}



void ClothingActorImpl::setMaxDistanceBlendTime(float blendTime)
{
	PX_ASSERT(mActorDesc);
	mActorDesc->maxDistanceBlendTime = blendTime;
}




float ClothingActorImpl::getMaxDistanceBlendTime() const
{
	PX_ASSERT(mActorDesc);
	return mActorDesc->maxDistanceBlendTime;
}



void ClothingActorImpl::setVisible(bool enable)
{
	// buffer enable
	bBufferedVisible = enable ? 1u : 0u;

	// disable immediately
	if (!enable)
	{
		bInternalVisible = 0;
	}
}



bool ClothingActorImpl::isVisibleBuffered() const
{
	return bBufferedVisible == 1;
}



bool ClothingActorImpl::isVisible() const
{
	return bInternalVisible == 1;
}



bool ClothingActorImpl::shouldComputeRenderData() const
{
	return mInternalFlags.ComputeRenderData && bInternalVisible == 1 && mGraphicalMeshes[mCurrentGraphicalLodId].renderProxy != NULL;
}



void ClothingActorImpl::setFrozen(bool enable)
{
	bUpdateFrozenFlag = 1;
	bBufferedFrozen = enable ? 1u : 0u;
}



bool ClothingActorImpl::isFrozenBuffered() const
{
	return bBufferedFrozen == 1;
}



ClothSolverMode::Enum ClothingActorImpl::getClothSolverMode() const
{
	return ClothSolverMode::v3;
}



void ClothingActorImpl::freeze_LocksPhysX(bool on)
{
	if (mClothingSimulation != NULL)
	{
		mClothingSimulation->setStatic(on);
	}
}



void ClothingActorImpl::setGraphicalLOD(uint32_t lod)
{
	mBufferedGraphicalLod = PxMin(lod, mAsset->getNumGraphicalLodLevels()-1);
}



uint32_t ClothingActorImpl::getGraphicalLod()
{
	return mBufferedGraphicalLod;
}



bool ClothingActorImpl::rayCast(const PxVec3& worldOrigin, const PxVec3& worldDirection, float& time, PxVec3& normal, uint32_t& vertexIndex)
{
	if (mClothingSimulation != NULL)
	{
		PxVec3 origin(worldOrigin);
		PxVec3 dir(worldDirection);
		if (bInternalLocalSpaceSim == 1)
		{
#if _DEBUG
			bool ok = true;
			ok &= mInternalGlobalPose.column0.isNormalized();
			ok &= mInternalGlobalPose.column1.isNormalized();
			ok &= mInternalGlobalPose.column2.isNormalized();
			if (!ok)
			{
				APEX_DEBUG_WARNING("Internal Global Pose is not normalized (Scale: %f %f %f). Raycast could be wrong.", mInternalGlobalPose.column0.magnitude(), mInternalGlobalPose.column1.magnitude(), mInternalGlobalPose.column2.magnitude());
			}
#endif
			PxMat44 invGlobalPose = mInternalGlobalPose.inverseRT();
			origin = invGlobalPose.transform(worldOrigin);
			dir = invGlobalPose.rotate(worldDirection);
		}

		bool hit = mClothingSimulation->raycast(origin, dir, time, normal, vertexIndex);

		if (hit && bInternalLocalSpaceSim == 1)
		{
			mInternalGlobalPose.rotate(normal);
		}
		return hit;

	}

	return false;
}



void ClothingActorImpl::attachVertexToGlobalPosition(uint32_t vertexIndex, const PxVec3& worldPosition)
{
	if (mClothingSimulation != NULL)
	{
		ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId);
		const ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type* const PX_RESTRICT coeffs = physicalMesh->constrainCoefficients.buf;
		PX_ASSERT((int32_t)vertexIndex < physicalMesh->constrainCoefficients.arraySizes[0]);

		const float linearScale = (mInternalMaxDistanceScale.Multipliable ? mInternalMaxDistanceScale.Scale : 1.0f) * mActorDesc->actorScale;
		const float absoluteScale = mInternalMaxDistanceScale.Multipliable ? 0.0f : (physicalMesh->maximumMaxDistance * (1.0f - mInternalMaxDistanceScale.Scale));
		const float reduceMaxDistance = mMaxDistReduction + absoluteScale;

		const float maxDistance = PxMax(0.0f, coeffs[vertexIndex].maxDistance - reduceMaxDistance) * linearScale;
		const PxVec3 skinnedPosition = mClothingSimulation->skinnedPhysicsPositions[vertexIndex];

		PxVec3 restrictedWorldPosition = worldPosition;
		if (bInternalLocalSpaceSim == 1)
		{
#if _DEBUG
			bool ok = true;
			ok &= mInternalGlobalPose.column0.isNormalized();
			ok &= mInternalGlobalPose.column1.isNormalized();
			ok &= mInternalGlobalPose.column2.isNormalized();
			if (!ok)
			{
				APEX_DEBUG_WARNING("Internal Global Pose is not normalized (Scale: %f %f %f). attachVertexToGlobalPosition could be wrong.", mInternalGlobalPose.column0.magnitude(), mInternalGlobalPose.column1.magnitude(), mInternalGlobalPose.column2.magnitude());
			}
#endif
			restrictedWorldPosition = mInternalGlobalPose.inverseRT().transform(restrictedWorldPosition);
		}

		PxVec3 dir = restrictedWorldPosition - skinnedPosition;
		if (dir.magnitude() > maxDistance)
		{
			dir.normalize();
			restrictedWorldPosition = skinnedPosition + dir * maxDistance;
		}

		mClothingSimulation->attachVertexToGlobalPosition(vertexIndex, restrictedWorldPosition);
	}
}



void ClothingActorImpl::freeVertex(uint32_t vertexIndex)
{
	if (mClothingSimulation != NULL)
	{
		mClothingSimulation->freeVertex(vertexIndex);
	}
}



uint32_t ClothingActorImpl::getClothingMaterial() const
{
	const ClothingAssetParameters* clothingAsset = static_cast<const ClothingAssetParameters*>(mAsset->getAssetNvParameterized());
	ClothingMaterialLibraryParameters* materialLib = static_cast<ClothingMaterialLibraryParameters*>(clothingAsset->materialLibrary);

	PX_ASSERT(materialLib != NULL);
	if (materialLib == NULL)
	{
		return 0;
	}

	PX_ASSERT(materialLib->materials.buf != NULL);
	PX_ASSERT(materialLib->materials.arraySizes[0] > 0);

	PX_ASSERT(mActorDesc->clothingMaterialIndex < (uint32_t)materialLib->materials.arraySizes[0]);

	return mActorDesc->clothingMaterialIndex;
}



void ClothingActorImpl::setClothingMaterial(uint32_t index)
{
	mActorDesc->clothingMaterialIndex = index;
}



void ClothingActorImpl::setOverrideMaterial(uint32_t submeshIndex, const char* overrideMaterialName)
{
	mOverrideMaterials[submeshIndex] = ApexSimpleString(overrideMaterialName);
	
	for (uint32_t i = 0; i < mGraphicalMeshes.size(); ++i)
	{
		ClothingRenderProxyImpl* renderProxy = mGraphicalMeshes[i].renderProxy;
		if (renderProxy != NULL)
		{
			renderProxy->setOverrideMaterial(i, overrideMaterialName);
		}
	}
}



void ClothingActorImpl::getLodRange(float& min, float& max, bool& intOnly) const
{
	min = 0.f;
	max = 1.f;
	intOnly = true;
}



float ClothingActorImpl::getActiveLod() const
{
	return bIsSimulationOn ? 0.f : 1.f;
}



void ClothingActorImpl::forceLod(float lod)
{
	if (lod < 0.0f)
	{
		mForceSimulation = -1;
	}
	else if (lod >= 1.f)
	{
		mForceSimulation = 1;
	}
	else
	{
		mForceSimulation = (int32_t)(lod + 0.5f);
	}

	if (mClothingSimulation == NULL && mForceSimulation > 0)
	{
		mMaxDistReduction = mAsset->getBiggestMaxDistance();
	}
}



void ClothingActorImpl::getPhysicalMeshPositions(void* buffer, uint32_t byteStride)
{
	if (isSimulationRunning())
	{
		APEX_INTERNAL_ERROR("Cannot be called while the scene is running");
		return;
	}

	PX_ASSERT(buffer != NULL);
	if (byteStride == 0)
	{
		byteStride = sizeof(PxVec3);
	}

	if (byteStride < sizeof(PxVec3))
	{
		APEX_INTERNAL_ERROR("Bytestride is too small (%d, but must be >= %d)", byteStride, sizeof(PxVec3));
		return;
	}

	uint32_t numSimulatedVertices = (mClothingSimulation == NULL) ? 0 : mClothingSimulation->sdkNumDeformableVertices;
	PxStrideIterator<PxVec3> it((PxVec3*)buffer, byteStride);
	for (uint32_t i = 0; i < numSimulatedVertices; i++, ++it)
	{
		*it = mClothingSimulation->sdkWritebackPosition[i];
	}

	ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* pmesh = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId);
	const PxVec3* skinPosePosition = pmesh->vertices.buf;
	const PxMat44* matrices = mData.mInternalBoneMatricesCur;

	const uint8_t* const PX_RESTRICT optimizationData = pmesh->optimizationData.buf;
	PX_ASSERT(optimizationData != NULL);

	if (matrices != NULL)
	{
		const uint16_t* boneIndices = pmesh->boneIndices.buf;
		const float* boneWeights = pmesh->boneWeights.buf;
		const uint32_t numBonesPerVertex = pmesh->numBonesPerVertex;
		const uint32_t numVertices = pmesh->numVertices;
		for (uint32_t vertexIndex = numSimulatedVertices; vertexIndex < numVertices; ++vertexIndex, ++it)
		{
			const uint8_t shift = 4 * (vertexIndex % 2);
			const uint8_t numBones = uint8_t((optimizationData[vertexIndex / 2] >> shift) & 0x7);

			Simd4f temp = gSimd4fZero;
			for (uint32_t j = 0; j < numBones; j++)
			{
				const Simd4f boneWeightV = Simd4fScalarFactory(boneWeights[vertexIndex * numBonesPerVertex + j]);
				const uint32_t boneIndex = boneIndices[vertexIndex * numBonesPerVertex + j];
				const PxMat44& mat = (PxMat44&)(matrices[boneIndex]);

				Simd4f transformedPosV = applyAffineTransform(mat, createSimd3f(skinPosePosition[vertexIndex]));
				transformedPosV = transformedPosV * boneWeightV;
				temp = temp + transformedPosV;
			}
			store3(&(*it).x, temp);
		}
	}
	else
	{
		const uint32_t numVertices = pmesh->numVertices;
		const PxMat44& mat = (PxMat44&)(mInternalGlobalPose);
		for (uint32_t vertexIndex = numSimulatedVertices; vertexIndex < numVertices; ++vertexIndex, ++it)
		{
			Simd4f transformedPosV = applyAffineTransform(mat, createSimd3f(skinPosePosition[vertexIndex]));
			store3(&(*it).x, transformedPosV);
		}
	}
}



void ClothingActorImpl::getPhysicalMeshNormals(void* buffer, uint32_t byteStride)
{
	if (isSimulationRunning())
	{
		APEX_INTERNAL_ERROR("Cannot be called while the scene is running");
		return;
	}

	PX_ASSERT(buffer != NULL);
	if (byteStride == 0)
	{
		byteStride = sizeof(PxVec3);
	}

	if (byteStride < sizeof(PxVec3))
	{
		APEX_INTERNAL_ERROR("Bytestride is too small (%d, but must be >= %d)", byteStride, sizeof(PxVec3));
		return;
	}

	if (mClothingSimulation == NULL)
	{
		APEX_INTERNAL_ERROR("No simulation data available");
		return;
	}

	if (mClothingSimulation->sdkWritebackNormal == NULL)
	{
		APEX_INTERNAL_ERROR("No simulation normals for softbodies");
		return;
	}

	PxStrideIterator<PxVec3> it((PxVec3*)buffer, byteStride);
	for (uint32_t i = 0; i < mClothingSimulation->sdkNumDeformableVertices; i++, ++it)
	{
		*it = mClothingSimulation->sdkWritebackNormal[i];
	}


	ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* pmesh = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId);
	const PxVec3* skinPoseNormal = pmesh->normals.buf;
	const PxMat44* matrices = mData.mInternalBoneMatricesCur;
	const uint32_t numVertices = pmesh->numVertices;

	const uint8_t* const PX_RESTRICT optimizationData = pmesh->optimizationData.buf;
	PX_ASSERT(optimizationData != NULL);

	if (matrices != NULL)
	{
		const uint16_t* boneIndices = pmesh->boneIndices.buf;
		const float* boneWeights = pmesh->boneWeights.buf;
		const uint32_t numBonesPerVertex = pmesh->numBonesPerVertex;
		for (uint32_t vertexIndex = mClothingSimulation->sdkNumDeformableVertices; vertexIndex < numVertices; ++vertexIndex, ++it)
		{
			const uint8_t shift = 4 * (vertexIndex % 2);
			const uint8_t numBones = uint8_t((optimizationData[vertexIndex / 2] >> shift) & 0x7);

			Simd4f temp = gSimd4fZero;
			for (uint32_t j = 0; j < numBones; j++)
			{
				const Simd4f boneWeightV = Simd4fScalarFactory(boneWeights[vertexIndex * numBonesPerVertex + j]);
				const uint32_t boneIndex = boneIndices[vertexIndex * numBonesPerVertex + j];
				const PxMat44& mat = (PxMat44&)(matrices[boneIndex]);

				Simd4f transformedNormalV = applyAffineTransform(mat, createSimd3f(skinPoseNormal[vertexIndex]));
				transformedNormalV = transformedNormalV * boneWeightV;
				temp = temp + transformedNormalV;
			}
			store3(&(*it).x, temp);
		}
	}
	else
	{
		const PxMat44& mat = (PxMat44&)(mInternalGlobalPose);
		for (uint32_t vertexIndex = mClothingSimulation->sdkNumDeformableVertices; vertexIndex < numVertices; ++vertexIndex, ++it)
		{
			Simd4f transformedNormalV = applyLinearTransform(mat, createSimd3f(skinPoseNormal[vertexIndex]));
			store3(&(*it).x, transformedNormalV);
		}
	}
}



float ClothingActorImpl::getMaximumSimulationBudget() const
{
	uint32_t solverIterations = 5;

	ClothingMaterialLibraryParametersNS::ClothingMaterial_Type* clothingMaterial = getCurrentClothingMaterial();
	if (clothingMaterial != NULL)
	{
		solverIterations = clothingMaterial->solverIterations;
	}

	return mAsset->getMaximumSimulationBudget(solverIterations);
}



uint32_t ClothingActorImpl::getNumSimulationVertices() const
{
	uint32_t numVerts = 0;
	if (mClothingSimulation != NULL)
	{
		numVerts = mClothingSimulation->sdkNumDeformableVertices;
	}
	return numVerts;
}



const PxVec3* ClothingActorImpl::getSimulationPositions()
{
	if (mClothingSimulation == NULL)
		return NULL;

	waitForFetchResults();

	return mClothingSimulation->sdkWritebackPosition;
}



const PxVec3* ClothingActorImpl::getSimulationNormals()
{
	if (mClothingSimulation == NULL)
		return NULL;

	waitForFetchResults();

	return mClothingSimulation->sdkWritebackNormal;
}



bool ClothingActorImpl::getSimulationVelocities(PxVec3* velocities)
{
	if (mClothingSimulation == NULL)
		return false;

	waitForFetchResults();

	mClothingSimulation->getVelocities(velocities);
	return true;
}



uint32_t ClothingActorImpl::getNumGraphicalVerticesActive(uint32_t submeshIndex) const
{
	if (mClothingSimulation == NULL)
		return 0;

	uint32_t numVertices = 0;

	const ClothingGraphicalLodParameters* graphicalLod = mAsset->getGraphicalLod(mCurrentGraphicalLodId);

	const uint32_t numParts = (uint32_t)graphicalLod->physicsMeshPartitioning.arraySizes[0];
	ClothingGraphicalLodParametersNS::PhysicsMeshPartitioning_Type* parts = graphicalLod->physicsMeshPartitioning.buf;

#if PX_DEBUG || PX_CHECKED
	bool found = false;
#endif
	for (uint32_t c = 0; c < numParts; c++)
	{
		if (parts[c].graphicalSubmesh == submeshIndex)
		{
			numVertices = parts[c].numSimulatedVertices;
#if defined _DEBUG || PX_CHECKED
			found = true;
#endif
			break;
		}
	}
#if PX_DEBUG || PX_CHECKED
	PX_ASSERT(found);
#endif

	return numVertices;
}



PxMat44 ClothingActorImpl::getRenderGlobalPose() const
{
	return (bInternalLocalSpaceSim == 1) ? mInternalGlobalPose : PxMat44(PxIdentity);
}



const PxMat44* ClothingActorImpl::getCurrentBoneSkinningMatrices() const
{
	return mData.mInternalBoneMatricesCur;
}


#if PX_PHYSICS_VERSION_MAJOR == 3
void ClothingActorImpl::setPhysXScene(PxScene* physXscene)
{
	if (isSimulationRunning())
	{
		APEX_INTERNAL_ERROR("Cannot change the physics scene while the simulation is running");
		return;
	}

	if (mPhysXScene != NULL && mPhysXScene != physXscene)
	{
		removePhysX_LocksPhysX();
	}

	mPhysXScene = physXscene;

	if (mPhysXScene != NULL)
	{
		if (isCookedDataReady())
		{
			createPhysX_LocksPhysX(0.0f);
		}
	}
}

PxScene* ClothingActorImpl::getPhysXScene() const
{
	return mPhysXScene;
}
#endif


// this is 2.8.x only
void ClothingActorImpl::updateScaledGravity(float substepSize)
{
	if (mClothingScene != NULL && mClothingScene->mApexScene != NULL)
	{
		PxVec3 oldInternalScaledGravity = mInternalScaledGravity;
		mInternalScaledGravity = mClothingScene->mApexScene->getGravity();

		if (mActorDesc->allowAdaptiveTargetFrequency)
		{
			// disable adaptive frequency if the simulation doesn't require it
			if (mClothingSimulation && !mClothingSimulation->needsAdaptiveTargetFrequency())
			{
				substepSize = 0.0f;
			}

			const float targetFrequency = mClothingScene->getAverageSimulationFrequency(); // will return 0 if the module is not set to compute it!
			if (targetFrequency > 0.0f && substepSize > 0.0f)
			{
				// PH: This will scale the gravity to result in fixed velocity deltas (in Cloth/SoftBody)
				const float targetScale = 1.0f / (targetFrequency * substepSize);
				mInternalScaledGravity *= targetScale * targetScale; // need to square this to achieve constant behavior.
			}
		}

		bInternalScaledGravityChanged = (oldInternalScaledGravity != mInternalScaledGravity) ? 1u : 0u;
	}
}



void ClothingActorImpl::tickSynchBeforeSimulate_LocksPhysX(float simulationDelta, float substepSize, uint32_t substepNumber, uint32_t numSubSteps)
{
	PX_PROFILE_ZONE("ClothingActorImpl::beforeSimulate", GetInternalApexSDK()->getContextId());

	// PH: simulationDelta can be 0 for subsequent substeps (substepNumber > 0 and numSubSteps > 1

	if (mClothingSimulation != NULL && simulationDelta > 0.0f)
	{
		mClothingSimulation->verifyTimeStep(substepSize);
	}

	if (substepNumber == 0)
	{
		PX_PROFILE_ZONE("ClothingActorImpl::updateStateInternal", GetInternalApexSDK()->getContextId());

		updateStateInternal_NoPhysX(numSubSteps > 1);
		updateScaledGravity(substepSize); // moved after updateStateInternal, cause it reads the localSpace state
		freeze_LocksPhysX(bInternalFrozen == 1);
	}

	/// interpolate matrices
	if (numSubSteps > 1)
	{
		PX_PROFILE_ZONE("ClothingActorImpl::interpolateMatrices", GetInternalApexSDK()->getContextId());

		const uint32_t numBones = mActorDesc->useInternalBoneOrder ? mAsset->getNumUsedBones() : mAsset->getNumBones();
		PX_ASSERT((numBones != 0) == (mInternalInterpolatedBoneMatrices != NULL));
		PX_ASSERT((numBones != 0) == (mData.mInternalBoneMatricesPrev != NULL));
		if (substepNumber == (numSubSteps - 1))
		{
			bool matrixChanged = false;
			for (uint32_t i = 0; i < numBones; i++)
			{
				mInternalInterpolatedBoneMatrices[i] = mData.mInternalBoneMatricesCur[i];
				matrixChanged |= mData.mInternalBoneMatricesCur[i] != mData.mInternalBoneMatricesPrev[i];
			}

			mInternalInterpolatedGlobalPose = mInternalGlobalPose;
			bGlobalPoseChanged |= mOldInternalGlobalPose != mInternalGlobalPose ? 1 : 0;
			bBoneMatricesChanged |= matrixChanged ? 1 : 0;
		}
		else
		{
			const float ratio = bInternalTeleportDue == ClothingTeleportMode::TeleportAndReset ? 0.0f : (1.0f - float(substepNumber + 1) / float(numSubSteps));

			bool matrixChanged = false;
			for (uint32_t i = 0; i < numBones; i++)
			{
				mInternalInterpolatedBoneMatrices[i] = interpolateMatrix(ratio, mData.mInternalBoneMatricesPrev[i], mData.mInternalBoneMatricesCur[i]);
				matrixChanged |= mData.mInternalBoneMatricesCur[i] != mData.mInternalBoneMatricesPrev[i];
			}
			mInternalInterpolatedGlobalPose = interpolateMatrix(ratio, mOldInternalGlobalPose, mInternalGlobalPose);
			bGlobalPoseChanged |= mOldInternalGlobalPose != mInternalGlobalPose ? 1 : 0;
			bBoneMatricesChanged |= matrixChanged ? 1 : 0;
		}
	}

	if (mClothingSimulation != NULL)
	{
		mClothingSimulation->setGlobalPose(substepNumber == 0 ? mInternalGlobalPose : mInternalInterpolatedGlobalPose);
	}

	// PH: this is done before createPhysX mesh is called to make sure it's not executed on the freshly generated skeleton
	// see ClothignAsset::createCollisionBulk for more info
	{
		PX_PROFILE_ZONE("ClothingActorImpl::updateCollision", GetInternalApexSDK()->getContextId());
		updateCollision_LocksPhysX(numSubSteps > 1);
	}

	// PH: Done before skinPhysicsMesh to use the same buffers
	if (mClothingSimulation != NULL && substepNumber == 0 && (bInternalFrozen == 0))
	{
		PX_PROFILE_ZONE("ClothingActorImpl::applyVelocityChanges", GetInternalApexSDK()->getContextId());
		applyVelocityChanges_LocksPhysX(simulationDelta);
	}

	if (substepNumber == 0)
	{
		lodTick_LocksPhysX(simulationDelta);
	}

	if (mCurrentSolverIterations > 0)
	{
		PX_ASSERT(mClothingSimulation != NULL); // after lodTick there should be a simulation mesh

		applyTeleport(false, substepNumber);

		if (bDirtyClothingTemplate == 1)
		{
			bDirtyClothingTemplate = 0;
			mClothingSimulation->applyClothingDesc(mActorDesc->clothDescTemplate);
		}
	}

	initializeActorData();
	if (mClothingSimulation != NULL)
	{
		skinPhysicsMesh(numSubSteps > 1, (float)(substepNumber + 1) / (float)numSubSteps);
	
		applyLockingTasks();

		mClothingSimulation->setInterCollisionChannels(mInterCollisionChannels);

		mClothingSimulation->setHalfPrecisionOption(mIsAllowedHalfPrecisionSolver);
	}
}



void ClothingActorImpl::applyLockingTasks()
{
	if (mClothingSimulation != NULL)
	{
		// depends on nothing
		applyClothingMaterial_LocksPhysX();

		// depends on skinning
		applyTeleport(true, 0);

		// depends on applyTeleport
		applyGlobalPose_LocksPhysX();

		// depends on skinning and applyTeleport
		updateConstrainPositions_LocksPhysX();

		// depends on lod tick (and maybe applyTeleport eventually)
		applyCollision_LocksPhysX();
	}
}



bool ClothingActorImpl::isSkinningDirty()
{
	return bBoneMatricesChanged == 1 || ((!bInternalLocalSpaceSim) == 1 && bGlobalPoseChanged == 1);
}



void ClothingActorImpl::skinPhysicsMesh(bool useInterpolatedMatrices, float substepFraction)
{
	if (mClothingSimulation == NULL || mClothingSimulation->skinnedPhysicsPositions == NULL)
	{
		return;
	}

	const bool skinningDirty = isSkinningDirty();
	if (skinningDirty)
	{
		ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId);

		if (physicalMesh->hasNegativeBackstop)
		{
			skinPhysicsMeshInternal<true>(useInterpolatedMatrices, substepFraction);
		}
		else
		{
			skinPhysicsMeshInternal<false>(useInterpolatedMatrices, substepFraction);
		}
	}
}



void ClothingActorImpl::updateConstrainPositions_LocksPhysX()
{
	PX_PROFILE_ZONE("ClothingActorImpl::updateConstrainPositions", GetInternalApexSDK()->getContextId());

	if (mClothingSimulation == NULL || mClothingSimulation->skinnedPhysicsPositions == NULL)
	{
		return;
	}

	const bool skinningDirty = isSkinningDirty();
	mClothingSimulation->updateConstrainPositions(skinningDirty);

	bBoneMatricesChanged = 0;
	bGlobalPoseChanged = 0;
}



void ClothingActorImpl::applyCollision_LocksPhysX()
{
	PX_PROFILE_ZONE("ClothingActorImpl::applyCollision", GetInternalApexSDK()->getContextId());

	if (mClothingSimulation != NULL)
	{
		mClothingSimulation->applyCollision();
	}
}



ClothingMaterialLibraryParametersNS::ClothingMaterial_Type* ClothingActorImpl::getCurrentClothingMaterial() const
{
	const ClothingAssetParameters* clothingAsset = static_cast<const ClothingAssetParameters*>(mAsset->getAssetNvParameterized());
	const ClothingMaterialLibraryParameters* materialLib = static_cast<const ClothingMaterialLibraryParameters*>(clothingAsset->materialLibrary);
	if (materialLib == NULL)
	{
		APEX_DEBUG_WARNING("No Clothing Material Library present in asset");
		return NULL;
	}

	PX_ASSERT(materialLib->materials.buf != NULL);
	PX_ASSERT(materialLib->materials.arraySizes[0] > 0);

	uint32_t index = mActorDesc->clothingMaterialIndex;
	if (index >= (uint32_t)materialLib->materials.arraySizes[0])
	{
		APEX_INVALID_PARAMETER("Index must be smaller than materials array: %d < %d", index, materialLib->materials.arraySizes[0]);

		PX_ASSERT(nvidia::strcmp(clothingAsset->className(), ClothingAssetParameters::staticClassName()) == 0);
		index = clothingAsset->materialIndex;
		mActorDesc->clothingMaterialIndex = index;
	}

	return &materialLib->materials.buf[index];
}


bool ClothingActorImpl::clothingMaterialsEqual(ClothingMaterialLibraryParametersNS::ClothingMaterial_Type& a, ClothingMaterialLibraryParametersNS::ClothingMaterial_Type& b)
{
	// update this compare function in case the struct has changed
	// PH: Let's hope that we bump the version number when modifying the materials in the .pl
	PX_COMPILE_TIME_ASSERT(ClothingMaterialLibraryParameters::ClassVersion == 14);

	return
	    a.verticalStretchingStiffness == b.verticalStretchingStiffness &&
	    a.horizontalStretchingStiffness == b.horizontalStretchingStiffness &&
	    a.bendingStiffness == b.bendingStiffness &&
	    a.shearingStiffness == b.shearingStiffness &&
	    a.tetherStiffness == b.tetherStiffness &&
	    a.tetherLimit == b.tetherLimit &&
	    a.orthoBending == b.orthoBending &&
		a.verticalStiffnessScaling.compressionRange == b.verticalStiffnessScaling.compressionRange &&
		a.verticalStiffnessScaling.stretchRange == b.verticalStiffnessScaling.stretchRange &&
	    a.verticalStiffnessScaling.scale == b.verticalStiffnessScaling.scale &&
		a.horizontalStiffnessScaling.compressionRange == b.horizontalStiffnessScaling.compressionRange &&
		a.horizontalStiffnessScaling.stretchRange == b.horizontalStiffnessScaling.stretchRange &&
	    a.horizontalStiffnessScaling.scale == b.horizontalStiffnessScaling.scale &&
		a.bendingStiffnessScaling.compressionRange == b.bendingStiffnessScaling.compressionRange &&
		a.bendingStiffnessScaling.stretchRange == b.bendingStiffnessScaling.stretchRange &&
	    a.bendingStiffnessScaling.scale == b.bendingStiffnessScaling.scale &&
		a.shearingStiffnessScaling.compressionRange == b.shearingStiffnessScaling.compressionRange &&
		a.shearingStiffnessScaling.stretchRange == b.shearingStiffnessScaling.stretchRange &&
	    a.shearingStiffnessScaling.scale == b.shearingStiffnessScaling.scale &&
	    a.damping == b.damping &&
	    a.stiffnessFrequency == b.stiffnessFrequency &&
	    a.drag == b.drag &&
	    a.comDamping == b.comDamping &&
	    a.friction == b.friction &&
	    a.massScale == b.massScale &&
	    a.solverIterations == b.solverIterations &&
	    a.solverFrequency == b.solverFrequency &&
	    a.gravityScale == b.gravityScale &&
	    a.inertiaScale == b.inertiaScale &&
	    a.hardStretchLimitation == b.hardStretchLimitation &&
	    a.maxDistanceBias == b.maxDistanceBias &&
		a.hierarchicalSolverIterations == b.hierarchicalSolverIterations &&
		a.selfcollisionThickness == b.selfcollisionThickness &&
		a.selfcollisionSquashScale == b.selfcollisionSquashScale &&
		a.selfcollisionStiffness == b.selfcollisionStiffness;
}



void ClothingActorImpl::applyClothingMaterial_LocksPhysX()
{
	PX_PROFILE_ZONE("ClothingActorImpl::applyClothingMaterial", GetInternalApexSDK()->getContextId());

	ClothingMaterialLibraryParametersNS::ClothingMaterial_Type* currentMaterial  = getCurrentClothingMaterial();
	bool clothingMaterialDirty = !clothingMaterialsEqual(*currentMaterial, mClothingMaterial) || (bInternalScaledGravityChanged == 1);

	if (mClothingSimulation != NULL && clothingMaterialDirty)
	{
		if (mClothingSimulation->applyClothingMaterial(currentMaterial, mInternalScaledGravity))
		{
			mClothingMaterial = *currentMaterial;
			bInternalScaledGravityChanged = 0;
		}
	}
}



void ClothingActorImpl::tickAsynch_NoPhysX()
{
	if (mClothingSimulation != NULL)
	{
		if (shouldComputeRenderData() /*&& bInternalFrozen == 0*/)
		{
			// perform mesh-to-mesh skinning if using skin cloth
			if (mInternalFlags.ParallelCpuSkinning)
			{
				mData.skinToAnimation_NoPhysX(false);
			}
		}
	}
}



bool ClothingActorImpl::needsManualSubstepping()
{
	return mClothingSimulation != NULL && mClothingSimulation->needsManualSubstepping();
}


#ifndef WITHOUT_PVD
void ClothingActorImpl::initPvdInstances(pvdsdk::PvdDataStream& pvdStream)
{
	ApexResourceInterface* pvdInstance = static_cast<ApexResourceInterface*>(mActorProxy);

	// Actor Params
	pvdStream.createInstance(pvdsdk::NamespacedName(APEX_PVD_NAMESPACE, "ClothingActorParam"), mActorDesc);
	pvdStream.setPropertyValue(pvdInstance, "ActorParams", pvdsdk::DataRef<const uint8_t>((const uint8_t*)&mActorDesc, sizeof(ClothingActorParam*)), pvdsdk::getPvdNamespacedNameForType<pvdsdk::ObjectRef>());

	pvdsdk::ApexPvdClient* client = GetInternalApexSDK()->getApexPvdClient();
	PX_ASSERT(client != NULL);
	client->updatePvd(mActorDesc, *mActorDesc);
}


void ClothingActorImpl::destroyPvdInstances()
{
	pvdsdk::ApexPvdClient* client = GetInternalApexSDK()->getApexPvdClient();
	if (client != NULL)
	{
		if (client->isConnected() && client->getPxPvd().getInstrumentationFlags() & PxPvdInstrumentationFlag::eDEBUG)
		{
			pvdsdk::PvdDataStream* pvdStream = client->getDataStream();
			{
				if (pvdStream != NULL)
				{
					client->updatePvd(mActorDesc, *mActorDesc, pvdsdk::PvdAction::DESTROY);
					pvdStream->destroyInstance(mActorDesc);
					// the actor instance is destroyed in ResourceList::remove
				}
			}
		}
	}
}


void ClothingActorImpl::updatePvd()
{
	// update pvd
	pvdsdk::ApexPvdClient* client = GetInternalApexSDK()->getApexPvdClient();
	if (client != NULL)
	{
		if (client->isConnected() && client->getPxPvd().getInstrumentationFlags() & PxPvdInstrumentationFlag::eDEBUG)
		{	
			pvdsdk::PvdDataStream* pvdStream = client->getDataStream();
			pvdsdk::PvdUserRenderer* pvdRenderer = client->getUserRender();

			if (pvdStream != NULL && pvdRenderer != NULL)
			{
				ApexResourceInterface* pvdInstance = static_cast<ApexResourceInterface*>(mActorProxy);

				client->updatePvd(mActorDesc, *mActorDesc);

				if (mClothingSimulation)
				{
					mClothingSimulation->updatePvd(*pvdStream, *pvdRenderer, pvdInstance, bInternalLocalSpaceSim == 1);
				}
			}
		}
	}
}
#endif


void ClothingActorImpl::visualize()
{
#ifdef WITHOUT_DEBUG_VISUALIZE
#else

	using RENDER_DEBUG::DebugColors;
	using RENDER_DEBUG::DebugRenderState;

	if (mClothingScene == NULL || mClothingScene->mRenderDebug == NULL)
	{
		return;
	}
	if ( !mEnableDebugVisualization ) return;

	RenderDebugInterface& renderDebug = *mClothingScene->mRenderDebug;
	const physx::PxMat44& savedPose = *RENDER_DEBUG_IFACE(&renderDebug)->getPoseTyped();
	RENDER_DEBUG_IFACE(&renderDebug)->setIdentityPose();

	const float visualizationScale = mClothingScene->mDebugRenderParams->Scale;

	PX_ASSERT(mActorDesc != NULL);

	if (visualizationScale == 0.0f || !mActorDesc->flags.Visualize)
	{
		return;
	}

	if (mClothingScene->mClothingDebugRenderParams->GlobalPose)
	{
#if 1
		// PH: This uses only lines, not triangles, hence wider engine support
		const uint32_t colorRed = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Red);
		const uint32_t colorGreen = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Green);
		const uint32_t colorBlue = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Blue);
		PxMat44 absPose(mInternalGlobalPose);
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorRed);
		RENDER_DEBUG_IFACE(&renderDebug)->debugLine(absPose.getPosition(), absPose.getPosition() + absPose.column0.getXYZ() * visualizationScale);
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorGreen);
		RENDER_DEBUG_IFACE(&renderDebug)->debugLine(absPose.getPosition(), absPose.getPosition() + absPose.column1.getXYZ() * visualizationScale);
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorBlue);
		RENDER_DEBUG_IFACE(&renderDebug)->debugLine(absPose.getPosition(), absPose.getPosition() + absPose.column2.getXYZ() * visualizationScale);
#else
		// PH: But this one looks a bit nicer
		RENDER_DEBUG_IFACE(&renderDebug)->debugAxes(mInternalGlobalPose, visualizationScale);
#endif
	}

	// transform debug rendering to global space
	if (bInternalLocalSpaceSim == 1 && !mClothingScene->mClothingDebugRenderParams->ShowInLocalSpace)
	{
		RENDER_DEBUG_IFACE(&renderDebug)->setPose(mInternalGlobalPose);
	}

	const uint32_t colorRed = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Red);
	const uint32_t colorBlue = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Blue);
	const uint32_t colorWhite = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::White);

	renderDataLock();

#if 0
	static bool turnOn = true;
	if (turnOn)
	{
		RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();

		RenderMeshAssetIntl* rma = mAsset->getGraphicalMesh(mCurrentGraphicalLodId);
		if (false && mActorDesc->morphDisplacements.arraySizes[0] > 0 && rma != NULL)
		{
			RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorBlue);

			uint32_t* morphMap = mAsset->getMorphMapping(mCurrentGraphicalLodId);

			for (uint32_t s = 0; s < rma->getSubmeshCount(); s++)
			{
				const uint32_t numVertices = rma->getSubmesh(s).getVertexCount(0);
				const VertexFormat& format = rma->getSubmesh(s).getVertexBuffer().getFormat();
				const uint32_t positionIndex = format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::POSITION));
				if (format.getBufferFormat(positionIndex) == RenderDataFormat::FLOAT3)
				{
					PxVec3* positions = (PxVec3*)rma->getSubmesh(s).getVertexBuffer().getBuffer(positionIndex);
					for (uint32_t v = 0; v < numVertices; v++)
					{
						PxVec3 from = positions[v];
						PX_ASSERT((int)morphMap[v] < mActorDesc->morphDisplacements.arraySizes[0]);
						PxVec3 to = from + mActorDesc->morphDisplacements.buf[morphMap[v]];

						RENDER_DEBUG_IFACE(&renderDebug)->debugLine(from, to);
					}
				}
			}

		}

		if (mActorDesc->morphPhysicalMeshNewPositions.buf != NULL)
		{
			mClothingScene-> mRENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorRed);

			ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId);
			uint32_t offset = mAsset->getPhysicalMeshOffset(mAsset->getPhysicalMeshID(mCurrentGraphicalLodId));
			PX_ASSERT((int32_t)(offset + physicalMesh->numVertices) <= mActorDesc->morphPhysicalMeshNewPositions.arraySizes[0]);
			for (uint32_t p = 0; p < physicalMesh->numVertices; p++)
			{
				const PxVec3 renderDisp(0.0f, 0.0f, 0.001f);
				PxVec3 from = physicalMesh->vertices.buf[p] + renderDisp;
				PxVec3 to = mActorDesc->morphPhysicalMeshNewPositions.buf[offset + p] + renderDisp;

				RENDER_DEBUG_IFACE(&renderDebug)->debugLine(from, to);
			}
		}

		RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
	}
#endif

	// save the rendering state.
	RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();

	RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorRed);
	RENDER_DEBUG_IFACE(&renderDebug)->setCurrentState(DebugRenderState::SolidShaded);


	// visualize velocities
	const float velocityScale = mClothingScene->mClothingDebugRenderParams->Velocities * visualizationScale;
	if (velocityScale > 0.0f && mClothingSimulation != NULL)
	{
		PxVec3* velocities = (PxVec3*)GetInternalApexSDK()->getTempMemory(sizeof(PxVec3) * mClothingSimulation->sdkNumDeformableVertices);
		if (velocities != NULL)
		{
			mClothingSimulation->getVelocities(velocities);

			const bool useVelocityClamp = mActorDesc->useVelocityClamping;
			PxBounds3 velocityClamp(mActorDesc->vertexVelocityClamp);

			for (uint32_t i = 0; i < mClothingSimulation->sdkNumDeformableVertices; i++)
			{
				const PxVec3 pos = mClothingSimulation->sdkWritebackPosition[i];
				const PxVec3 vel = velocities[i];
				bool clamped = false;
				if (useVelocityClamp)
				{
					clamped  = (vel.x < velocityClamp.minimum.x) || (vel.x > velocityClamp.maximum.x);
					clamped |= (vel.y < velocityClamp.minimum.y) || (vel.y > velocityClamp.maximum.y);
					clamped |= (vel.z < velocityClamp.minimum.z) || (vel.z > velocityClamp.maximum.z);
				}
				const PxVec3 dest = pos + vel * velocityScale;
				RENDER_DEBUG_IFACE(&renderDebug)->debugGradientLine(pos, dest, clamped ? colorRed : colorBlue, colorWhite);
			}

			GetInternalApexSDK()->releaseTempMemory(velocities);
		}
	}

	// visualize Skeleton
	const bool skeleton = mClothingScene->mClothingDebugRenderParams->Skeleton;
	const float boneFramesScale = mClothingScene->mClothingDebugRenderParams->BoneFrames;
	const float boneNamesScale = mClothingScene->mClothingDebugRenderParams->BoneNames;
	if (skeleton || boneFramesScale + boneNamesScale > 0.0f)
	{
		const PxMat44* matrices = mData.mInternalBoneMatricesCur;

		if (matrices != NULL)
		{
			mAsset->visualizeBones(renderDebug, matrices, skeleton, boneFramesScale, boneNamesScale);
		}
	}

	// visualization of the physical mesh
	if (mClothingScene->mClothingDebugRenderParams->Backstop)
	{
		visualizeBackstop(renderDebug);
	}

	const float backstopPrecise = mClothingScene->mClothingDebugRenderParams->BackstopPrecise;
	if (backstopPrecise > 0.0f)
	{
		visualizeBackstopPrecise(renderDebug, backstopPrecise);
	}

	const float skinnedPositionsScale = mClothingScene->mClothingDebugRenderParams->SkinnedPositions;
	const bool drawMaxDistance = mClothingScene->mClothingDebugRenderParams->MaxDistance;
	const bool drawMaxDistanceIn = mClothingScene->mClothingDebugRenderParams->MaxDistanceInwards;
	if (skinnedPositionsScale > 0.0f || drawMaxDistance || drawMaxDistanceIn)
	{
		visualizeSkinnedPositions(renderDebug, skinnedPositionsScale, drawMaxDistance, drawMaxDistanceIn);
	}

	// visualize vertex - bone connections for skinning
	for (uint32_t g = 0; g < mGraphicalMeshes.size(); g++)
	{
		if (!mGraphicalMeshes[g].active)
		{
			continue;
		}

		//const ClothingGraphicalLodParameters* graphicalLod = mAsset->getGraphicalLod(g);
		ClothingGraphicalMeshAssetWrapper meshAsset(mAsset->getRenderMeshAsset(g));

		const float graphicalVertexBonesScale = mClothingScene->mClothingDebugRenderParams->GraphicalVertexBones;
		if (graphicalVertexBonesScale > 0.0f)
		{
			for (uint32_t submeshIndex = 0; submeshIndex < meshAsset.getSubmeshCount(); submeshIndex++)
			{
				RenderDataFormat::Enum outFormat;
				const PxVec3* positions = (const PxVec3*)meshAsset.getVertexBuffer(submeshIndex, RenderVertexSemantic::POSITION, outFormat);
				PX_ASSERT(outFormat == RenderDataFormat::FLOAT3);
				const uint16_t* boneIndices = (const uint16_t*)meshAsset.getVertexBuffer(submeshIndex, RenderVertexSemantic::BONE_INDEX, outFormat);
				const float* boneWeights = (const float*)meshAsset.getVertexBuffer(submeshIndex, RenderVertexSemantic::BONE_WEIGHT, outFormat);
				visualizeBoneConnections(renderDebug, positions, boneIndices, boneWeights,
				                         meshAsset.getNumBonesPerVertex(submeshIndex), meshAsset.getNumVertices(submeshIndex));
			}
		}

		const float physicalVertexBonesScale = mClothingScene->mClothingDebugRenderParams->PhysicalVertexBones;
		if (physicalVertexBonesScale > 0.0f)
		{
			ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh = mAsset->getPhysicalMeshFromLod(g);
			visualizeBoneConnections(renderDebug, physicalMesh->vertices.buf, physicalMesh->boneIndices.buf,
			                         physicalMesh->boneWeights.buf, physicalMesh->numBonesPerVertex, physicalMesh->numVertices);
		}
	}

	if (mClothingSimulation != NULL)
	{
		// visualization of the physical mesh
		const float physicsMeshWireScale = mClothingScene->mClothingDebugRenderParams->PhysicsMeshWire;
		const float physicsMeshSolidScale = mClothingScene->mClothingDebugRenderParams->PhysicsMeshSolid;
		ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId);

		const float physicsMeshNormalScale = mClothingScene->mClothingDebugRenderParams->PhysicsMeshNormals;
		if (physicsMeshNormalScale > 0.0f)
		{
			const PxVec3* positions = mClothingSimulation->sdkWritebackPosition;
			const PxVec3* normals = mClothingSimulation->sdkWritebackNormal;

			RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();
			RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Blue));

			const uint32_t numVertices = mClothingSimulation->sdkNumDeformableVertices;
			for (uint32_t i = 0; i < numVertices; i++)
			{
				const PxVec3 dest = positions[i] + normals[i] * physicsMeshNormalScale;
				RENDER_DEBUG_IFACE(&renderDebug)->debugLine(positions[i], dest);
			}

			RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
		}

		if (mClothingScene->mClothingDebugRenderParams->PhysicsMeshIndices)
		{
			RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();
			const PxVec3 upAxis = -mInternalScaledGravity.getNormalized();
			const float avgEdgeLength = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId)->averageEdgeLength;
			RENDER_DEBUG_IFACE(&renderDebug)->setCurrentTextScale(avgEdgeLength);
			RENDER_DEBUG_IFACE(&renderDebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::CameraFacing);

			const uint32_t numVertices = mClothingSimulation->sdkNumDeformableVertices;
			const PxVec3* const positions = mClothingSimulation->sdkWritebackPosition;
			const PxVec3* const normals = mClothingSimulation->sdkWritebackNormal;
			for (uint32_t i = 0; i < numVertices; ++i)
			{
				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(0xFFFFFFFF);
				const PxVec3 pos = positions[i];
				const PxVec3 normal = normals[i].getNormalized();
				PxVec3 rightAxis = upAxis.cross(normal).getNormalized();
				PxVec3 realUpAxis = normal.cross(rightAxis).getNormalized();

				RENDER_DEBUG_IFACE(&renderDebug)->debugText(pos + normal * (avgEdgeLength * 0.1f), "%d", i);
			}
			RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
		}

		if (physicsMeshWireScale > 0.0f || physicsMeshSolidScale > 0.0f)
		{
			RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();
			RENDER_DEBUG_IFACE(&renderDebug)->setIdentityPose();
			const float actorScale = mActorDesc->actorScale;

			const float linearScale = (mInternalMaxDistanceScale.Multipliable ? mInternalMaxDistanceScale.Scale : 1.0f) * actorScale;
			const float absoluteScale = mInternalMaxDistanceScale.Multipliable ? 0.0f : (physicalMesh->maximumMaxDistance * (1.0f - mInternalMaxDistanceScale.Scale));

			const float reduceMaxDistance = mMaxDistReduction + absoluteScale;

			const uint32_t numIndices = mClothingSimulation->sdkNumDeformableIndices;
			const uint32_t* const indices = physicalMesh->indices.buf;
			const PxVec3* const positions = mClothingSimulation->sdkWritebackPosition;
			const PxVec3* const skinnedPositions = mClothingSimulation->skinnedPhysicsPositions;

			const ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type* const PX_RESTRICT coeffs = physicalMesh->constrainCoefficients.buf;

			RENDER_DEBUG_IFACE(&renderDebug)->removeFromCurrentState(DebugRenderState::SolidWireShaded);

			union BlendColors
			{
				unsigned char chars[4];
				uint32_t color;
			} blendColors[3];

			blendColors[0].color = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Yellow);
			blendColors[1].color = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Gold);
			blendColors[2].color = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Red);
			const uint32_t lightBlue = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::LightBlue);
			const uint32_t darkBlue = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Blue);

			if (bInternalFrozen)
			{
				blendColors[0].color = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Purple);
				blendColors[1].color = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::DarkPurple);
			}
			else if (bInternalTeleportDue == ClothingTeleportMode::TeleportAndReset)
			{
				blendColors[0].color = darkBlue;
				blendColors[1].color = darkBlue;
				blendColors[2].color = darkBlue;
			}
			else if (bInternalTeleportDue == ClothingTeleportMode::Teleport)
			{
				blendColors[0].color = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Red);
				blendColors[1].color = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Red);
				blendColors[2].color = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Red);
			}

			const uint8_t sides[4][3] = {{2, 1, 0}, {0, 1, 3}, {1, 2, 3}, {2, 0, 3}};

			const uint32_t numIndicesPerPrim = physicalMesh->isTetrahedralMesh ? 4u : 3u;
			const float numindicesPerPrimF = (float)numIndicesPerPrim;

			for (uint32_t i = 0; i < numIndices; i += numIndicesPerPrim)
			{
				PxVec3 vecs[8];
				PxVec3 center(0.0f, 0.0f, 0.0f);
				uint32_t colors[8];
				for (uint32_t j = 0; j < numIndicesPerPrim; j++)
				{
					const uint32_t index = indices[i + j];
					vecs[j] = positions[index];
					center += vecs[j];
					const float maxDistance = (coeffs[index].maxDistance - reduceMaxDistance) * linearScale;
					if (maxDistance <= 0.0f)
					{
						colors[j                    ] = lightBlue;
						colors[j + numIndicesPerPrim] = darkBlue;
					}
					else
					{
						const float distance = (positions[index] - skinnedPositions[index]).magnitude() / maxDistance;

						union
						{
							unsigned char tempChars[8];
							uint32_t tempColor[2];
						};

						if (distance > 1.0f)
						{
							colors[j                    ] = blendColors[2].color;
							colors[j + numIndicesPerPrim] = blendColors[2].color;
						}
						else
						{
							for (uint32_t k = 0; k < 4; k++)
							{
								tempChars[k    ] = (unsigned char)(blendColors[2].chars[k] * distance + blendColors[0].chars[k] * (1.0f - distance));
								tempChars[k + 4] = (unsigned char)(blendColors[2].chars[k] * distance + blendColors[1].chars[k] * (1.0f - distance));
							}
							colors[j                    ] = tempColor[0];
							colors[j + numIndicesPerPrim] = tempColor[1];
						}
					}
				}

				center /= numindicesPerPrimF;

				for (uint32_t j = 0; j < numIndicesPerPrim; j++)
				{
					vecs[j + numIndicesPerPrim] = vecs[j] * physicsMeshSolidScale + center * (1.0f - physicsMeshSolidScale);
					vecs[j] = vecs[j] * physicsMeshWireScale + center * (1.0f - physicsMeshWireScale);
				}

				if (physicsMeshWireScale > 0.0f)
				{
					RENDER_DEBUG_IFACE(&renderDebug)->removeFromCurrentState(DebugRenderState::SolidShaded);

					if (numIndicesPerPrim == 3)
					{
						RENDER_DEBUG_IFACE(&renderDebug)->debugGradientLine(vecs[0], vecs[1], colors[0], colors[1]);
						RENDER_DEBUG_IFACE(&renderDebug)->debugGradientLine(vecs[1], vecs[2], colors[1], colors[2]);
						RENDER_DEBUG_IFACE(&renderDebug)->debugGradientLine(vecs[2], vecs[0], colors[2], colors[0]);
					}
					else
					{
						for (uint32_t j = 0; j < 4; j++)
						{
							uint32_t triIndices[3] = { sides[j][0], sides[j][1], sides[j][2] };
							RENDER_DEBUG_IFACE(&renderDebug)->debugGradientLine(vecs[triIndices[0]], vecs[triIndices[2]], colors[triIndices[0]], colors[triIndices[2]]);
							RENDER_DEBUG_IFACE(&renderDebug)->debugGradientLine(vecs[triIndices[2]], vecs[triIndices[1]], colors[triIndices[2]], colors[triIndices[1]]);
							RENDER_DEBUG_IFACE(&renderDebug)->debugGradientLine(vecs[triIndices[1]], vecs[triIndices[0]], colors[triIndices[1]], colors[triIndices[0]]);
						}
					}
				}
				if (physicsMeshSolidScale > 0.0f)
				{
					RENDER_DEBUG_IFACE(&renderDebug)->addToCurrentState(DebugRenderState::SolidShaded);

					if (numIndicesPerPrim == 3)
					{
						// culling is active for these, so we need both of them
						RENDER_DEBUG_IFACE(&renderDebug)->debugGradientTri(vecs[3], vecs[4], vecs[5], colors[3], colors[4], colors[5]);
						RENDER_DEBUG_IFACE(&renderDebug)->debugGradientTri(vecs[3], vecs[5], vecs[4], colors[3], colors[5], colors[4]);
					}
					else
					{
						for (uint32_t j = 0; j < 4; j++)
						{
							uint32_t triIndices[3] = { (uint32_t)sides[j][0] + 4, (uint32_t)sides[j][1] + 4, (uint32_t)sides[j][2] + 4 };
							RENDER_DEBUG_IFACE(&renderDebug)->debugGradientTri(
							    vecs[triIndices[0]], vecs[triIndices[2]], vecs[triIndices[1]],
							    colors[triIndices[0]], colors[triIndices[2]], colors[triIndices[1]]);
						}
					}
				}
			}
			RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
		}

		// self collision visualization
		if (	(mClothingScene->mClothingDebugRenderParams->SelfCollision || mClothingScene->mClothingDebugRenderParams->SelfCollisionWire)
			&& (!mAsset->getModuleClothing()->useSparseSelfCollision() || mClothingSimulation->getType() != SimulationType::CLOTH3x))
		{
			const PxVec3* const positions = mClothingSimulation->sdkWritebackPosition;
			ClothingMaterialLibraryParametersNS::ClothingMaterial_Type* material = getCurrentClothingMaterial();
			
			if (material->selfcollisionThickness > 0.0f && material->selfcollisionStiffness > 0.0f)
			{
				visualizeSpheres(renderDebug, positions, mClothingSimulation->sdkNumDeformableVertices, 0.5f*material->selfcollisionThickness*mActorDesc->actorScale, colorRed, mClothingScene->mClothingDebugRenderParams->SelfCollisionWire);
			}
		}
		/*
		// inter collision visualization
		if (	(mClothingScene->mClothingDebugRenderParams->InterCollision || mClothingScene->mClothingDebugRenderParams->InterCollisionWire)
			&&	mClothingSimulation->getType() == SimulationType::CLOTH3x)
		{
			const PxVec3* const positions = mClothingSimulation->sdkWritebackPosition;
			float distance = mAsset->getModuleClothing()->getInterCollisionDistance();
			float stiffness = mAsset->getModuleClothing()->getInterCollisionStiffness();
			if (distance > 0.0f && stiffness > 0.0f)
			{
				visualizeSpheres(renderDebug, positions, mClothingSimulation->sdkNumDeformableVertices, 0.5f*distance, colorBlue, mClothingScene->mClothingDebugRenderParams->InterCollisionWire);
			}
		}
		*/
		// visualization of the graphical mesh
		for (uint32_t g = 0; g < mGraphicalMeshes.size(); g++)
		{
			if (!mGraphicalMeshes[g].active)
			{
				continue;
			}

			const ClothingGraphicalLodParameters* graphicalLod = mAsset->getGraphicalLod(g);

			if (graphicalLod != NULL && (graphicalLod->skinClothMapB.buf != NULL || graphicalLod->skinClothMap.buf != NULL))
			{
				AbstractMeshDescription pcm;
				pcm.numVertices		= mClothingSimulation->sdkNumDeformableVertices;
				pcm.numIndices		= mClothingSimulation->sdkNumDeformableIndices;
				pcm.pPosition		= mClothingSimulation->sdkWritebackPosition;
				pcm.pNormal			= mClothingSimulation->sdkWritebackNormal;
				pcm.pIndices		= physicalMesh->indices.buf;
				pcm.avgEdgeLength	= graphicalLod->skinClothMapThickness;

				if (mClothingScene->mClothingDebugRenderParams->SkinMapAll)
				{
					mAsset->visualizeSkinCloth(renderDebug, pcm, graphicalLod->skinClothMapB.buf != NULL, mActorDesc->actorScale);
				}
				if (mClothingScene->mClothingDebugRenderParams->SkinMapBad)
				{
					mAsset->visualizeSkinClothMap(renderDebug, pcm,
												graphicalLod->skinClothMapB.buf, (uint32_t)graphicalLod->skinClothMapB.arraySizes[0],
												graphicalLod->skinClothMap.buf, (uint32_t)graphicalLod->skinClothMap.arraySizes[0], mActorDesc->actorScale, true, false);
				}
				if (mClothingScene->mClothingDebugRenderParams->SkinMapActual)
				{
					mAsset->visualizeSkinClothMap(renderDebug, pcm,
												graphicalLod->skinClothMapB.buf, (uint32_t)graphicalLod->skinClothMapB.arraySizes[0],
												graphicalLod->skinClothMap.buf, (uint32_t)graphicalLod->skinClothMap.arraySizes[0], mActorDesc->actorScale, false, false);
				}
				if (mClothingScene->mClothingDebugRenderParams->SkinMapInvalidBary)
				{
					mAsset->visualizeSkinClothMap(renderDebug, pcm,
												graphicalLod->skinClothMapB.buf, (uint32_t)graphicalLod->skinClothMapB.arraySizes[0],
												graphicalLod->skinClothMap.buf, (uint32_t)graphicalLod->skinClothMap.arraySizes[0], mActorDesc->actorScale, false, true);
				}

			}

			RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();

			RENDER_DEBUG_IFACE(&renderDebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::CenterText);
			RENDER_DEBUG_IFACE(&renderDebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::CameraFacing);

			// Probably should be removed
			if (graphicalLod != NULL && mClothingScene->mClothingDebugRenderParams->RecomputeSubmeshes)
			{
				const RenderMeshAsset* rma = mAsset->getRenderMeshAsset(mCurrentGraphicalLodId);

				RENDER_DEBUG_IFACE(&renderDebug)->addToCurrentState(DebugRenderState::SolidShaded);

				uint32_t submeshVertexOffset = 0;
				for (uint32_t s = 0; s < rma->getSubmeshCount(); s++)
				{
					const RenderSubmesh&	submesh		= rma->getSubmesh(s);
					const uint32_t			vertexCount = submesh.getVertexCount(0);
					const uint32_t*			indices		= submesh.getIndexBuffer(0);
					const uint32_t			numIndices	= submesh.getIndexCount(0);

					if(mGraphicalMeshes[mCurrentGraphicalLodId].renderProxy == NULL)
						continue;
					const PxVec3* positions = mGraphicalMeshes[mCurrentGraphicalLodId].renderProxy->renderingDataPosition + submeshVertexOffset;


					uint32_t maxForColor = (uint32_t)graphicalLod->physicsMeshPartitioning.arraySizes[0] + 2;
					{
						const uint8_t colorPart = (uint8_t)((255 / maxForColor) & 0xff);
						RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(uint32_t(colorPart | colorPart << 8 | colorPart << 16));
					}

					for (uint32_t i = 0; i < numIndices; i += 3)
					{
						RENDER_DEBUG_IFACE(&renderDebug)->debugTri(positions[indices[i]], positions[indices[i + 1]], positions[indices[i + 2]]);
					}
					const uint8_t colorPart = (uint8_t)((2 * 255 / maxForColor) & 0xff);
					RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(uint32_t(colorPart | colorPart << 8 | colorPart << 16));

					submeshVertexOffset += vertexCount;
				}
			}
			if (graphicalLod != NULL && mClothingScene->mClothingDebugRenderParams->RecomputeVertices)
			{
				uint32_t color1 = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Orange);
				uint32_t color2 = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Purple);

				const PxVec3 upAxis = -mInternalScaledGravity.getNormalized();
				PX_UNUSED(upAxis);

				const float avgEdgeLength = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId)->averageEdgeLength;
				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentTextScale(0.2f * avgEdgeLength);

				const RenderMeshAsset* rma = mAsset->getRenderMeshAsset(mCurrentGraphicalLodId);

				uint32_t submeshVertexOffset = 0;
				for (uint32_t s = 0; s < rma->getSubmeshCount(); s++)
				{
					const RenderSubmesh& submesh = rma->getSubmesh(s);
					const uint32_t vertexCount = submesh.getVertexCount(0);

					ClothingRenderProxyImpl* renderProxy = mGraphicalMeshes[mCurrentGraphicalLodId].renderProxy;

					if (renderProxy == NULL)
					{
						renderProxy = mRenderProxyReady;
					}

					if (renderProxy == NULL)
					{
						renderProxy = mRenderProxyURR;
					}

					if(renderProxy == NULL)
						continue;

					const PxVec3* positions = renderProxy->renderingDataPosition + submeshVertexOffset;

					uint32_t simulatedVertices = graphicalLod->physicsMeshPartitioning.buf[s].numSimulatedVertices;
					uint32_t simulatedVerticesAdditional = graphicalLod->physicsMeshPartitioning.buf[s].numSimulatedVerticesAdditional;

					for (uint32_t i = 0; i < simulatedVerticesAdditional && i < vertexCount; i++)
					{
						RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(i < simulatedVertices ? color1 : color2);
						const PxVec3 pos = positions[i];

						RENDER_DEBUG_IFACE(&renderDebug)->debugText(pos, "%d", submeshVertexOffset + i);
					}

					submeshVertexOffset += vertexCount;
				}
			}

			RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
		}
	}

	if (mClothingSimulation != NULL)
	{
		mClothingSimulation->visualize(renderDebug, *mClothingScene->mClothingDebugRenderParams);
	}

	// restore the rendering state.
	RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();

	const float windScale = mClothingScene->mClothingDebugRenderParams->Wind;
	if (mClothingSimulation != NULL && windScale != 0.0f)
	{
		//PX_ASSERT(mWindDebugRendering.size() == mClothingSimulation->sdkNumDeformableVertices);
		const uint32_t numVertices = PxMin(mClothingSimulation->sdkNumDeformableVertices, mWindDebugRendering.size());
		const PxVec3* positions = mClothingSimulation->sdkWritebackPosition;
		const uint32_t red = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Red);
		const uint32_t white = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::White);
		for (uint32_t i = 0; i < numVertices; i++)
		{
			RENDER_DEBUG_IFACE(&renderDebug)->debugGradientLine(positions[i], positions[i] + mWindDebugRendering[i] * windScale, red, white);
		}
	}

	// fetchresults must be completed before Normal/Tangent debug rendering
	waitForFetchResults();

	// render mesh actor debug rendering
	for (uint32_t i = 0; i < mGraphicalMeshes.size(); i++)
	{
		if (mGraphicalMeshes[i].active && mGraphicalMeshes[i].renderProxy != NULL)
		{
			mGraphicalMeshes[i].renderProxy->getRenderMeshActor()->visualize(renderDebug, mClothingScene->mDebugRenderParams);
		}
	}
	if (mRenderProxyReady != NULL)
	{
		mRenderProxyReady->getRenderMeshActor()->visualize(renderDebug, mClothingScene->mDebugRenderParams);
	}
	else if (mRenderProxyURR != NULL)
	{
		mRenderProxyURR->getRenderMeshActor()->visualize(renderDebug, mClothingScene->mDebugRenderParams);
	}

	// transform debug rendering to global space
	if (bInternalLocalSpaceSim == 1)
	{
		RENDER_DEBUG_IFACE(&renderDebug)->setPose(PxMat44(PxIdentity));
	}


	if (mClothingScene->mClothingDebugRenderParams->Wind != 0.0f && mActorDesc->windParams.Adaption > 0.0f)
	{
		const PxVec3 center = mRenderBounds.getCenter();
		const float radius = mRenderBounds.getExtents().magnitude() * 0.02f;
		RENDER_DEBUG_IFACE(&renderDebug)->debugThickRay(center, center + mActorDesc->windParams.Velocity * mClothingScene->mClothingDebugRenderParams->Wind, radius);
	}

	if (mClothingScene->mClothingDebugRenderParams->SolverMode)
	{
		RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();
		RENDER_DEBUG_IFACE(&renderDebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::CenterText);
		RENDER_DEBUG_IFACE(&renderDebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::CameraFacing);

		ApexSimpleString solverString;

		if (mClothingSimulation != NULL)
		{
			solverString = (mClothingSimulation->getType() == SimulationType::CLOTH3x) ? "3.x" : "2.x";
#if APEX_CUDA_SUPPORT
			ApexSimpleString gpu(mClothingSimulation->isGpuSim() ? " GPU" : " CPU");

			if (mClothingSimulation->getGpuSimMemType() == GpuSimMemType::GLOBAL)
			{
				gpu += ApexSimpleString(", Global");
			}
			else if(mClothingSimulation->getGpuSimMemType() == GpuSimMemType::MIXED)
			{
				gpu += ApexSimpleString(", Mixed"); 
			}
			else if (mClothingSimulation->getGpuSimMemType() == GpuSimMemType::SHARED)
			{
				gpu += ApexSimpleString(", Shared"); 
			}

			solverString += gpu;
#endif
			solverString += ApexSimpleString(", ");
			ApexSimpleString solverCount;
			ApexSimpleString::itoa(mClothingSimulation->getNumSolverIterations(), solverCount);
			solverString += solverCount;
		}
		else
		{
			solverString = "Disabled";
		}

		PxVec3 up(0.0f, 1.0f, 0.0f);
		up = mData.mInternalGlobalPose.transform(up) - mClothingScene->getApexScene()->getGravity();
		up.normalize();

		const uint32_t white = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::White);
		const uint32_t gray = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::DarkGray);
		up = mRenderBounds.getDimensions().multiply(up) * 1.1f;
		const PxVec3 center = mRenderBounds.getCenter();
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentTextScale(mRenderBounds.getDimensions().magnitude());
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(gray);
		RENDER_DEBUG_IFACE(&renderDebug)->debugText(center + up * 1.1f, solverString.c_str());
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(white);
		RENDER_DEBUG_IFACE(&renderDebug)->debugText(center + up * 1.12f, solverString.c_str());

		RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
	}


	RENDER_DEBUG_IFACE(&renderDebug)->setPose(savedPose);
	renderDataUnLock();
#endif
}



void ClothingActorImpl::destroy()
{
	PX_ASSERT(!isSimulationRunning());
	if (mActiveCookingTask != NULL)
	{
		mActiveCookingTask->abort();
		PX_ASSERT(mActorDesc->runtimeCooked == NULL);
		mActiveCookingTask = NULL;
	}

	ApexActor::destroy();  // remove self from contexts, prevent re-release()

	removePhysX_LocksPhysX();

	if (mData.mInternalBoneMatricesCur != NULL)
	{
		PX_FREE(mData.mInternalBoneMatricesCur);
		mData.mInternalBoneMatricesCur = NULL;
	}
	if (mData.mInternalBoneMatricesPrev != NULL)
	{
		PX_FREE(mData.mInternalBoneMatricesPrev);
		mData.mInternalBoneMatricesPrev = NULL;
	}

	if (mInternalInterpolatedBoneMatrices != NULL)
	{
		PX_FREE(mInternalInterpolatedBoneMatrices);
		mInternalInterpolatedBoneMatrices = NULL;
	}

	PX_ASSERT(mActorDesc != NULL); // This is a requirement!


#ifndef WITHOUT_PVD
	destroyPvdInstances();
#endif
	if (mActorDesc != NULL)
	{
		if (mActorDesc->runtimeCooked != NULL)
		{
			mAsset->getModuleClothing()->getBackendFactory(mActorDesc->runtimeCooked->className())->releaseCookedInstances(mActorDesc->runtimeCooked);
		}
		mActorDesc->destroy();
		mActorDesc = NULL;
	}

	for (uint32_t i = 0; i < mGraphicalMeshes.size(); i++)
	{
		if (mGraphicalMeshes[i].renderProxy != NULL)
		{
			mGraphicalMeshes[i].renderProxy->release();
			mGraphicalMeshes[i].renderProxy = NULL;
		}
	}
	mGraphicalMeshes.clear();

	mRenderProxyMutex.lock();
	if (mRenderProxyURR != NULL)
	{
		mRenderProxyURR->release();
		mRenderProxyURR = NULL;
	}
	if (mRenderProxyReady != NULL)
	{
		mRenderProxyReady->release();
		mRenderProxyReady = NULL;
	}
	mRenderProxyMutex.unlock();
}


#if APEX_UE4
void ClothingActorImpl::initBeforeTickTasks(PxF32 deltaTime, PxF32 substepSize, PxU32 numSubSteps, PxTaskManager* taskManager, PxTaskID before, PxTaskID after)
#else
void ClothingActorImpl::initBeforeTickTasks(float deltaTime, float substepSize, uint32_t numSubSteps)
#endif
{
#if APEX_UE4
	PX_UNUSED(before);
	PX_UNUSED(after);
	PX_UNUSED(taskManager);
#endif
	mBeforeTickTask.setDeltaTime(deltaTime, substepSize, numSubSteps);
}



void ClothingActorImpl::submitTasksDuring(PxTaskManager* taskManager)
{
	taskManager->submitUnnamedTask(mDuringTickTask);
}



void ClothingActorImpl::setTaskDependenciesBefore(PxBaseTask* after)
{
	mBeforeTickTask.setContinuation(after);
}


void ClothingActorImpl::startBeforeTickTask()
{
	mBeforeTickTask.removeReference();
}



PxTaskID ClothingActorImpl::setTaskDependenciesDuring(PxTaskID before, PxTaskID after)
{
	mDuringTickTask.startAfter(before);
	mDuringTickTask.finishBefore(after);

	return mDuringTickTask.getTaskID();
}


#if !APEX_UE4
void ClothingActorImpl::setFetchContinuation()
{
	PxTaskManager* taskManager = mClothingScene->getApexScene()->getTaskManager();
	taskManager->submitUnnamedTask(mWaitForFetchTask);

	mFetchResultsTask.setContinuation(&mWaitForFetchTask);

	// reduce refcount to 1
	mWaitForFetchTask.removeReference();
}
#endif


void ClothingActorImpl::startFetchTasks()
{
	mFetchResultsRunningMutex.lock();
	mFetchResultsRunning = true;
#if APEX_UE4
	mFetchResultsSync.reset();
#else
	mWaitForFetchTask.mWaiting.reset();
#endif
	mFetchResultsRunningMutex.unlock();

#if APEX_UE4
	PxTaskManager* taskManager = mClothingScene->getApexScene()->getTaskManager();
	taskManager->submitUnnamedTask(mFetchResultsTask);
#else
	setFetchContinuation();
#endif

	mFetchResultsTask.removeReference();
}



void ClothingActorImpl::waitForFetchResults()
{
	mFetchResultsRunningMutex.lock();
	if (mFetchResultsRunning)
	{
		PX_PROFILE_ZONE("ClothingActorImpl::waitForFetchResults", GetInternalApexSDK()->getContextId());
#if APEX_UE4
		mFetchResultsSync.wait();		
#else
		mWaitForFetchTask.mWaiting.wait();
#endif
		syncActorData();
		mFetchResultsRunning = false;

#ifndef WITHOUT_PVD
		updatePvd();
#endif
	}
	mFetchResultsRunningMutex.unlock();
}



#if !APEX_UE4
void ClothingWaitForFetchTask::run()
{
}



void ClothingWaitForFetchTask::release()
{
	PxTask::release();

	mWaiting.set();
}



const char* ClothingWaitForFetchTask::getName() const
{
	return "ClothingWaitForFetchTask";
}
#endif



void ClothingActorImpl::applyTeleport(bool skinningReady, uint32_t substepNumber)
{
	const float teleportWeight = (bInternalTeleportDue != ClothingTeleportMode::Continuous && substepNumber == 0) ? 1.0f : 0.0f;
	const bool teleportReset = bInternalTeleportDue == ClothingTeleportMode::TeleportAndReset;

	// skinninReady is required when the teleportWeight is > 0.0
	// != is the same as XOR, it prevents calling setTeleportWeights twice when using a normal or (||)
	if ((mClothingSimulation != NULL) && ((teleportWeight == 0.0f) != skinningReady))
	{
		mClothingSimulation->setTeleportWeight(teleportWeight, teleportReset, bInternalLocalSpaceSim == 1);
	}
}



void ClothingActorImpl::applyGlobalPose_LocksPhysX()
{
	if (mClothingSimulation)
	{
		mClothingSimulation->applyGlobalPose();
	}
}



bool ClothingActorImpl::isValidDesc(const NvParameterized::Interface& params)
{
	// make this verbose!!!!
	if (::strcmp(params.className(), ClothingActorParam::staticClassName()) == 0)
	{
		const ClothingActorParam& actorDescGeneric = static_cast<const ClothingActorParam&>(params);
		const ClothingActorParamNS::ParametersStruct& actorDesc = static_cast<const ClothingActorParamNS::ParametersStruct&>(actorDescGeneric);

		// commented variables don't need validation.
		// actorDesc.actorDescTemplate
		for (int32_t i = 0; i < actorDesc.boneMatrices.arraySizes[0]; i++)
		{
			if (!actorDesc.boneMatrices.buf[i].isFinite())
			{
				APEX_INVALID_PARAMETER("boneMatrices[%d] is not finite!", i);
				return false;
			}
		}
		// actorDesc.clothDescTemplate
		// actorDesc.fallbackSkinning
		// actorDesc.flags.ParallelCpuSkinning
		// actorDesc.flags.ParallelMeshMeshSkinning
		// actorDesc.flags.ParallelPhysxMeshSkinning
		// actorDesc.flags.RecomputeNormals
		// actorDesc.flags.Visualize
		if (!actorDesc.globalPose.isFinite())
		{
			APEX_INVALID_PARAMETER("globalPose is not finite!");
			return false;
		}

		if (actorDesc.maxDistanceBlendTime < 0.0f)
		{
			APEX_INVALID_PARAMETER("maxDistanceBlendTime must be positive");
			return false;
		}

		if (actorDesc.maxDistanceScale.Scale < 0.0f || actorDesc.maxDistanceScale.Scale > 1.0f)
		{
			APEX_INVALID_PARAMETER("maxDistanceScale.Scale must be in the [0.0, 1.0] interval (is %f)",
			                       actorDesc.maxDistanceScale.Scale);
			return false;
		}

		// actorDesc.shapeDescTemplate
		// actorDesc.slowStart
		// actorDesc.updateStateWithGlobalMatrices
		// actorDesc.useHardwareCloth
		// actorDesc.useInternalBoneOrder
		// actorDesc.userData
		// actorDesc.uvChannelForTangentUpdate
		if (actorDesc.windParams.Adaption < 0.0f)
		{
			APEX_INVALID_PARAMETER("windParams.Adaption must be positive or zero");
			return false;
		}
		// actorDesc.windParams.Velocity

		if (actorDesc.actorScale <= 0.0f)
		{
			APEX_INVALID_PARAMETER("ClothingActorParam::actorScale must be bigger than 0 (is %f)", actorDesc.actorScale);
			return false;
		}

		return true;
	}
	else if (::strcmp(params.className(), ClothingPreviewParam::staticClassName()) == 0)
	{
		const ClothingPreviewParam& previewDescGeneric = static_cast<const ClothingPreviewParam&>(params);
		const ClothingPreviewParamNS::ParametersStruct& previewDesc = static_cast<const ClothingPreviewParamNS::ParametersStruct&>(previewDescGeneric);


		for (int32_t i = 0; i < previewDesc.boneMatrices.arraySizes[0]; i++)
		{
			if (!previewDesc.boneMatrices.buf[i].isFinite())
			{
				APEX_INVALID_PARAMETER("boneMatrices[%d] is not finite!", i);
				return false;
			}
		}

		if (!previewDesc.globalPose.isFinite())
		{
			APEX_INVALID_PARAMETER("globalPose is not finite!");
			return false;
		}

		return true;
	}
	return false;
}



ClothingCookedParam* ClothingActorImpl::getRuntimeCookedDataPhysX()
{
	if (mActorDesc->runtimeCooked != NULL && ::strcmp(mActorDesc->runtimeCooked->className(), ClothingCookedParam::staticClassName()) == 0)
	{
		return static_cast<ClothingCookedParam*>(mActorDesc->runtimeCooked);
	}

	return NULL;
}



//  ------ private methods -------


void ClothingActorImpl::updateBoneBuffer(ClothingRenderProxyImpl* renderProxy)
{
	if (renderProxy == NULL)
		return;

	renderProxy->setPose(getRenderGlobalPose());

	RenderMeshActor* meshActor = renderProxy->getRenderMeshActor();
	if (meshActor == NULL)
		return;

	if (mData.mInternalBoneMatricesCur == NULL)
	{
		// no bones
		PxMat44 pose = PxMat44(PxIdentity);
		if (mClothingSimulation == NULL)
		{
			// no sim
			if (bInternalLocalSpaceSim == 1)
			{
				pose = PxMat44(PxIdentity) * mActorDesc->actorScale;
			}
			else
			{
				pose = mInternalGlobalPose;
			}
		}

		meshActor->setTM(pose, 0);
	}
	else /*if (bBoneBufferDirty)*/				// this dirty flag can only be used if we know that the
												// render mesh asset stays with the clothing actor
												// reactivate when APEX-43 is fixed. Note that currently
												// the flag is set every frame in removePhysX_LocksPhysX
												// when simulation is disabled, so the flag is
												// currently not too useful
	{
		// bones or simulation have changed
		PxMat44* buffer = mData.mInternalBoneMatricesCur;
		PX_ASSERT(buffer != NULL);

		if (mAsset->getNumUsedBonesForMesh() == 1 && mClothingSimulation != NULL)
		{
			meshActor->setTM(PxMat44(PxIdentity), 0);
		}
		else
		{
			const uint32_t numBones = PxMin(mAsset->getNumUsedBonesForMesh(), meshActor->getBoneCount());
			for (uint32_t i = 0; i < numBones; i++)
			{
				meshActor->setTM(buffer[i], i);
			}
		}
	}

	bBoneBufferDirty = 0;
}



PxBounds3 ClothingActorImpl::getRenderMeshAssetBoundsTransformed()
{
	PxBounds3 newBounds = mAsset->getBoundingBox();

	PxMat44 transformation;
	if (mData.mInternalBoneMatricesCur != NULL)
	{
		transformation = mData.mInternalBoneMatricesCur[mAsset->getRootBoneIndex()];
	}
	else
	{
		transformation = mActorDesc->globalPose;
	}

	if (!newBounds.isEmpty())
	{
		const PxVec3 center = transformation.transform(newBounds.getCenter());
		const PxVec3 extent = newBounds.getExtents();
		const PxMat33 basis(transformation.column0.getXYZ(), transformation.column1.getXYZ(), transformation.column2.getXYZ());

		return PxBounds3::basisExtent(center, basis, extent);
	}
	else
	{
		return newBounds;
	}
}



bool ClothingActorImpl::allocateEnoughBoneBuffers_NoPhysX(bool prepareForSubstepping)
{
	PX_ASSERT(mActorDesc != NULL);
	const uint32_t numBones = mActorDesc->useInternalBoneOrder ? mAsset->getNumUsedBones() : mAsset->getNumBones();

	if (prepareForSubstepping && mInternalInterpolatedBoneMatrices == NULL)
	{
		mInternalInterpolatedBoneMatrices = (PxMat44*)PX_ALLOC(sizeof(PxMat44) * numBones, "mInternalInterpolatedBoneMatrices");
	}

	if (mData.mInternalBoneMatricesCur == NULL)
	{
		mData.mInternalBoneMatricesCur = (PxMat44*)PX_ALLOC(sizeof(PxMat44) * numBones, "mInternalBoneMatrices");
		mData.mInternalBoneMatricesPrev = (PxMat44*)PX_ALLOC(sizeof(PxMat44) * numBones, "mInternalBoneMatrices2");
		intrinsics::memSet(mData.mInternalBoneMatricesCur, 0, sizeof(PxMat44) * numBones);
		intrinsics::memSet(mData.mInternalBoneMatricesPrev, 0, sizeof(PxMat44) * numBones);
		return true;
	}

	return false;
}



bool ClothingActorImpl::isSimulationRunning() const
{
	if (mClothingScene != NULL)
	{
		return mClothingScene->isSimulating();    // virtual call
	}

	return false;
}



void ClothingActorImpl::updateStateInternal_NoPhysX(bool prepareForSubstepping)
{
	PX_ASSERT(mActorDesc);
	mInternalFlags = mActorDesc->flags;

	mInternalMaxDistanceBlendTime = (mActorDesc->freezeByLOD) ? 0.0f : mActorDesc->maxDistanceBlendTime;

	mInternalWindParams = mActorDesc->windParams;

	bInternalVisible = bBufferedVisible;
	if (bUpdateFrozenFlag == 1)
	{
		bInternalFrozen = bBufferedFrozen;
		bUpdateFrozenFlag = 0;
	}

	// update teleportation from double buffering
	bInternalTeleportDue = (ClothingTeleportMode::Enum)mActorDesc->teleportMode;
	mActorDesc->teleportMode = ClothingTeleportMode::Continuous;

	if (mActorDesc->localSpaceSim != (bInternalLocalSpaceSim == 1))
	{
		bInternalTeleportDue = ClothingTeleportMode::TeleportAndReset;
	}
	bInternalLocalSpaceSim = mActorDesc->localSpaceSim ? 1u : 0u;

	bMaxDistanceScaleChanged =
	    (mInternalMaxDistanceScale.Scale != mActorDesc->maxDistanceScale.Scale ||
	     mInternalMaxDistanceScale.Multipliable != mActorDesc->maxDistanceScale.Multipliable)
	    ? 1u : 0u;

	mInternalMaxDistanceScale = mActorDesc->maxDistanceScale;

	lockRenderResources();

	PxMat44 globalPose = mActorDesc->globalPose;

	PxMat44 rootBoneTransform = PxMat44(PxIdentity);
	const float invActorScale = 1.0f / mActorDesc->actorScale;
	bool bHasBones = mActorDesc->boneMatrices.arraySizes[0] > 0 || mAsset->getNumBones() > 0;
	bool bMultiplyGlobalPoseIntoBones = mActorDesc->multiplyGlobalPoseIntoBones || mActorDesc->boneMatrices.arraySizes[0] == 0;
	if (bHasBones)
	{
		bool newBuffers = allocateEnoughBoneBuffers_NoPhysX(prepareForSubstepping);

		nvidia::swap(mData.mInternalBoneMatricesCur, mData.mInternalBoneMatricesPrev);

		const uint32_t numBones = (mActorDesc->useInternalBoneOrder) ? mAsset->getNumUsedBones() : mAsset-> getNumBones();

		uint32_t rootNodeExternalIndex = mActorDesc->useInternalBoneOrder ? mAsset->getRootBoneIndex() : mAsset->getBoneExternalIndex(mAsset->getRootBoneIndex());
		if (rootNodeExternalIndex < (uint32_t)mActorDesc->boneMatrices.arraySizes[0])
		{
			// new pose of root bone available
			rootBoneTransform = mActorDesc->boneMatrices.buf[rootNodeExternalIndex];
		}
		else if (mActorDesc->updateStateWithGlobalMatrices)
		{
			// no pose for root bone available, use bind pose
			mAsset->getBoneBasePose(mAsset->getRootBoneIndex(), rootBoneTransform);
		}

		PxMat44 pose = PxMat44(PxIdentity);
		if (bInternalLocalSpaceSim == 1)
		{
			// consider the root bone as local space reference
			// PH: Note that inverseRT does not invert the scale, but preserve it

			// normalize (dividing by actorscale is not precise enough)
			if (!bMultiplyGlobalPoseIntoBones)
			{
				rootBoneTransform.column0.normalize();
				rootBoneTransform.column1.normalize();
				rootBoneTransform.column2.normalize();
			}

			// this transforms the skeleton into origin, and keeps the scale
			PxMat44 invRootBoneTransformTimesScale = rootBoneTransform.inverseRT();

			if (bMultiplyGlobalPoseIntoBones)
			{
				invRootBoneTransformTimesScale *= mActorDesc->actorScale;
			}

			// the result will be transformed back to global space in rendering
			pose = invRootBoneTransformTimesScale;
		}
		else if (bMultiplyGlobalPoseIntoBones)
		{
			pose = globalPose;
		}

		if (mActorDesc->boneMatrices.arraySizes[0] >= (int32_t)numBones)
		{
			// TODO when no globalPose is set and the bones are given internal, there could be a memcpy
			if (mAsset->writeBoneMatrices(pose, mActorDesc->boneMatrices.buf, sizeof(PxMat44), (uint32_t)mActorDesc->boneMatrices.arraySizes[0],
			                              mData.mInternalBoneMatricesCur, mActorDesc->useInternalBoneOrder, mActorDesc->updateStateWithGlobalMatrices))
			{
				bBoneMatricesChanged = 1;
			}
		}
		else
		{
			// no matrices provided. mInternalBoneMatrices (skinningMatrices) should just reflect the
			// the global pose transform

			for (uint32_t i = 0; i < numBones; i++)
			{
				mData.mInternalBoneMatricesCur[i] = pose;
			}
		}

		if (newBuffers)
		{
			memcpy(mData.mInternalBoneMatricesPrev, mData.mInternalBoneMatricesCur, sizeof(PxMat44) * numBones);
		}
	}
	else if (mData.mInternalBoneMatricesCur != NULL)
	{
		PX_FREE(mData.mInternalBoneMatricesCur);
		PX_FREE(mData.mInternalBoneMatricesPrev);
		mData.mInternalBoneMatricesCur = mData.mInternalBoneMatricesPrev = NULL;
	}

	unlockRenderResources();

	PxMat44 newInternalGlobalPose;
	if (bInternalLocalSpaceSim == 1)
	{
		// transform back into global space:
		if (bMultiplyGlobalPoseIntoBones || !bHasBones)
		{
			// we need to remove the scale when transforming back, as we keep the scale in local space
			// hmm, not sure why the adjustments on the translation parts are necessary, but they are..
			globalPose *= invActorScale;
			rootBoneTransform.scale(PxVec4(1.0f, 1.0f, 1.0f, mActorDesc->actorScale));
			newInternalGlobalPose = globalPose * rootBoneTransform;
		}
		else
		{
			newInternalGlobalPose = rootBoneTransform;
		}
	}
	else
	{
		newInternalGlobalPose = globalPose;
	}

	mOldInternalGlobalPose = mOldInternalGlobalPose.column0.isZero() ? newInternalGlobalPose : mInternalGlobalPose;
	mInternalGlobalPose = newInternalGlobalPose;

	bGlobalPoseChanged = (mInternalGlobalPose != mOldInternalGlobalPose) ? 1u : 0u;

	// set bBoneBufferDirty to 1 if any matrices have changed
	bBoneBufferDirty = (bGlobalPoseChanged == 1) || (bBoneMatricesChanged == 1) || (bBoneBufferDirty == 1) ? 1u : 0u;
}



#define RENDER_DEBUG_INTERMEDIATE_STEPS 0

template<bool withBackstop>
void ClothingActorImpl::skinPhysicsMeshInternal(bool useInterpolatedMatrices, float substepFraction)
{
#if RENDER_DEBUG_INTERMEDIATE_STEPS
	const float RenderDebugIntermediateRadius = 0.8f;
	mClothingScene->mRenderDebug->setCurrentDisplayTime(0.1);
	const uint8_t yellowColor = 255 - (uint8_t)(255.0f * substepFraction);
	const uint32_t color = 0xff0000 | yellowColor << 8;
	mClothingScene->mRenderDebug->setCurrentColor(color);
#else
	PX_UNUSED(substepFraction);
#endif

	uint32_t morphOffset = mAsset->getPhysicalMeshOffset(mAsset->getPhysicalMeshID(mCurrentGraphicalLodId));
	ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId);
	PxVec3* morphedPositions = mActorDesc->morphPhysicalMeshNewPositions.buf;
	const PxVec3* const PX_RESTRICT positions = morphedPositions != NULL ? morphedPositions + morphOffset : physicalMesh->vertices.buf;
	const PxVec3* const PX_RESTRICT normals = physicalMesh->normals.buf;
	const ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type* const PX_RESTRICT coeffs = physicalMesh->constrainCoefficients.buf;
	const float actorScale = mActorDesc->actorScale;

	const uint32_t numVertices = physicalMesh->numSimulatedVertices;
	const uint32_t numBoneIndicesPerVertex = physicalMesh->numBonesPerVertex;

	PxVec3* const PX_RESTRICT targetPositions = mClothingSimulation->skinnedPhysicsPositions;
	PxVec3* const PX_RESTRICT targetNormals = mClothingSimulation->skinnedPhysicsNormals;

	const uint32_t numPrefetches = ((numVertices + NUM_VERTICES_PER_CACHE_BLOCK - 1) / NUM_VERTICES_PER_CACHE_BLOCK);

	if (mData.mInternalBoneMatricesCur == NULL || numBoneIndicesPerVertex == 0)
	{
		PxMat44 matrix = PxMat44(PxIdentity);
		if (bInternalLocalSpaceSim == 1)
		{
			matrix *= actorScale;
		}
		else
		{
			// PH: maybe we can skip the matrix multiplication altogether when using local space sim?
			matrix = useInterpolatedMatrices ? mInternalInterpolatedGlobalPose : mInternalGlobalPose;
		}
		for (uint32_t i = 0; i < numVertices; i++)
		{
			//const PxVec3 untransformedPosition = positions[index] + morphReordering == NULL ? PxVec3(0.0f) : morphDisplacements[morphReordering[index]];
			targetPositions[i] = matrix.transform(positions[i]);
			targetNormals[i] = matrix.rotate(normals[i]);

			if (withBackstop)
			{
				if (coeffs[i].collisionSphereDistance < 0.0f)
				{
					targetPositions[i] -= (coeffs[i].collisionSphereDistance * actorScale) * targetNormals[i];
				}
			}
#if RENDER_DEBUG_INTERMEDIATE_STEPS
			if (RenderDebugIntermediateRadius > 0.0f)
			{
				mClothingScene->mRenderDebug->debugPoint(targetPositions[i], RenderDebugIntermediateRadius);
			}
#endif
		}
	}
	else
	{
		const uint16_t* const PX_RESTRICT simBoneIndices = physicalMesh->boneIndices.buf;
		const float* const PX_RESTRICT simBoneWeights = physicalMesh->boneWeights.buf;
		const PxMat44* matrices = useInterpolatedMatrices ? mInternalInterpolatedBoneMatrices : mData.mInternalBoneMatricesCur;

		const uint8_t* const PX_RESTRICT optimizationData = physicalMesh->optimizationData.buf;
		PX_ASSERT(optimizationData != NULL);
		PX_ASSERT((int32_t)(numPrefetches * NUM_VERTICES_PER_CACHE_BLOCK) / 2 <= physicalMesh->optimizationData.arraySizes[0]);

		uint32_t vertexIndex = 0;
		for (uint32_t i = 0; i < numPrefetches; ++i)
		{
			// HL: i tried to put positions and normals into a 16 byte aligned struct
			// but there was no significant perf benefit, and it caused a lot of adaptations
			// in the code because of the introduction of strides. Had to use
			// a stride iterators in AbstractMeshDescription, which made
			// its usage slower on xbox

			uint8_t* cache = (uint8_t*)((((size_t)(positions + vertexIndex + NUM_VERTICES_PER_CACHE_BLOCK)) >> 7) << 7);
			prefetchLine(cache);
			//prefetchLine(cache + 128);

			cache = (uint8_t*)((((size_t)(normals + vertexIndex + NUM_VERTICES_PER_CACHE_BLOCK)) >> 7) << 7);
			prefetchLine(cache);
			//prefetchLine(cache + 128);

			cache = (uint8_t*)((((size_t)(&simBoneWeights[(vertexIndex + NUM_VERTICES_PER_CACHE_BLOCK) * numBoneIndicesPerVertex])) >> 7) << 7);
			prefetchLine(cache);
			prefetchLine(cache + 128);

			cache = (uint8_t*)((((size_t)(&simBoneIndices[(vertexIndex + NUM_VERTICES_PER_CACHE_BLOCK) * numBoneIndicesPerVertex])) >> 7) << 7);
			prefetchLine(cache);
			//prefetchLine(cache + 128);


			if (withBackstop)
			{
				prefetchLine(&coeffs[vertexIndex + NUM_VERTICES_PER_CACHE_BLOCK]);
			}

			for (uint32_t j = 0; j < NUM_VERTICES_PER_CACHE_BLOCK; ++j)
			{
				//float sumWeights = 0.0f; // this is just for sanity

				Simd4f positionV = gSimd4fZero;
				Simd4f normalV = gSimd4fZero;

				const uint8_t shift = 4 * (vertexIndex % 2);
				const uint8_t numBones = uint8_t((optimizationData[vertexIndex / 2] >> shift) & 0x7);
				for (uint32_t k = 0; k < numBones; k++)
				{
					const float weight = simBoneWeights[vertexIndex * numBoneIndicesPerVertex + k];

					PX_ASSERT(weight <= 1.0f);

					//sumWeights += weight;
					Simd4f weightV = Simd4fScalarFactory(weight);

					const uint32_t index = simBoneIndices[vertexIndex * numBoneIndicesPerVertex + k];
					PX_ASSERT(index < mAsset->getNumUsedBones());

					/// PH: This might be faster without the reference, but on PC I can't tell
					/// HL: Now with SIMD it's significantly faster as reference
					const PxMat44& bone = (PxMat44&)matrices[index];

					Simd4f pV = applyAffineTransform(bone, createSimd3f(positions[vertexIndex]));
					pV = pV * weightV;
					positionV = positionV + pV;

					///todo There are probably cases where we don't need the normal on the physics mesh
					Simd4f nV = applyLinearTransform(bone, createSimd3f(normals[vertexIndex]));
					nV = nV * weightV;
					normalV = normalV + nV;
				}

				// PH: Sanity test. if this is not fulfilled, skinning went awfully wrong anyways
				// TODO do this check only once somewhere at initialization
				//PX_ASSERT(sumWeights == 0.0f || (sumWeights > 0.9999f && sumWeights < 1.0001f));

				normalV = normalV * rsqrt(dot3(normalV, normalV));
				store3(&targetNormals[vertexIndex].x, normalV);
				PX_ASSERT(numBones == 0 || targetNormals[vertexIndex].isFinite());

				// We disabled this in skinToBones as well, cause it's not really a valid case
				//if (sumWeights == 0)
				//	positionV = V3LoadU(positions[vertexIndex]);

				// in case of a negative collision sphere distance we move the animated position upwards
				// along the normal and set the collision sphere distance to zero.
				if (withBackstop)
				{
					if ((optimizationData[vertexIndex / 2] >> shift) & 0x8)
					{
						const float collisionSphereDistance = coeffs[vertexIndex].collisionSphereDistance;
						Simd4f dV = normalV * Simd4fScalarFactory(collisionSphereDistance * actorScale);
						positionV = positionV - dV;
					}
				}

				store3(&targetPositions[vertexIndex].x, positionV);
				PX_ASSERT(targetPositions[vertexIndex].isFinite());

#if RENDER_DEBUG_INTERMEDIATE_STEPS
				if (RenderDebugIntermediateRadius > 0.0f)
				{
					mClothingScene->mRenderDebug->debugPoint(targetPositions[vertexIndex], RenderDebugIntermediateRadius);
				}
#endif

				++vertexIndex;
			}
		}
	}
#if RENDER_DEBUG_INTERMEDIATE_STEPS
	mClothingScene->mRenderDebug->setCurrentDisplayTime();
#endif
}



void ClothingActorImpl::fetchResults()
{
	PX_PROFILE_ZONE("ClothingActorImpl::fetchResults", GetInternalApexSDK()->getContextId());
	if (isVisible() && mClothingSimulation != NULL && bInternalFrozen == 0)
	{
		mClothingSimulation->fetchResults(mInternalFlags.ComputePhysicsMeshNormals);
	}
}



ClothingActorData& ClothingActorImpl::getActorData()
{
	return mData;
}



void ClothingActorImpl::initializeActorData()
{
	PX_PROFILE_ZONE("ClothingActorImpl::initializeActorData", GetInternalApexSDK()->getContextId());

	mData.bIsClothingSimulationNull = mClothingSimulation == NULL;
	const bool bUninit = mData.bIsInitialized && mClothingSimulation == NULL;
	if (bReinitActorData == 1 || bUninit)
	{
		//We need to uninitialize ourselves
		PX_FREE(mData.mAsset.mData);
		mData.mAsset.mData = NULL;
		mData.bIsInitialized = false;
		bReinitActorData = 0;

		if (bUninit)
		{
			return;
		}
	}

	if(mGraphicalMeshes[mCurrentGraphicalLodId].renderProxy != NULL)
	{
		mData.mRenderingDataPosition = mGraphicalMeshes[mCurrentGraphicalLodId].renderProxy->renderingDataPosition;
		mData.mRenderingDataNormal = mGraphicalMeshes[mCurrentGraphicalLodId].renderProxy->renderingDataNormal;
		mData.mRenderingDataTangent = mGraphicalMeshes[mCurrentGraphicalLodId].renderProxy->renderingDataTangent;
	}

	if (!mData.bIsInitialized && mClothingSimulation != NULL)
	{
		mData.bIsInitialized = true;
		//Initialize
		mData.mRenderLock = mRenderDataLock;

		mData.mSdkDeformableVerticesCount = mClothingSimulation->sdkNumDeformableVertices;
		mData.mSdkDeformableIndicesCount = mClothingSimulation->sdkNumDeformableIndices;
		mData.mSdkWritebackPositions = mClothingSimulation->sdkWritebackPosition;
		mData.mSdkWritebackNormal = mClothingSimulation->sdkWritebackNormal;

		mData.mSkinnedPhysicsPositions = mClothingSimulation->skinnedPhysicsPositions;
		mData.mSkinnedPhysicsNormals = mClothingSimulation->skinnedPhysicsNormals;

		//Allocate the clothing asset now...

		mAsset->initializeAssetData(mData.mAsset, mActorDesc->uvChannelForTangentUpdate);

		mData.mMorphDisplacementBuffer = mActorDesc->morphDisplacements.buf;
		mData.mMorphDisplacementBufferCount = (uint32_t)mActorDesc->morphDisplacements.arraySizes[0];
	}

	uint32_t largestSubmesh = 0;
	if (mData.bIsInitialized && mClothingSimulation != NULL)
	{
		mData.bRecomputeNormals = mInternalFlags.RecomputeNormals;
		mData.bRecomputeTangents = mInternalFlags.RecomputeTangents;
		mData.bIsSimulationMeshDirty = mClothingSimulation->isSimulationMeshDirty();

		// this updates per-frame so I need to sync it every frame
		mData.mInternalGlobalPose = mInternalGlobalPose;
		mData.mCurrentGraphicalLodId = mCurrentGraphicalLodId;

		for (uint32_t a = 0; a < mData.mAsset.mGraphicalLodsCount; ++a)
		{
			ClothingMeshAssetData* pLod = mData.mAsset.GetLod(a);
			pLod->bActive = mGraphicalMeshes[a].active;
			pLod->bNeedsTangents = mGraphicalMeshes[a].needsTangents;

			// check if map contains tangent values, otherwise print out warning
			if (!mData.bRecomputeTangents && mGraphicalMeshes[a].needsTangents && pLod->mImmediateClothMap != NULL)
			{
				// tangentBary has been marked invalid during asset update, or asset has immediate map (without tangent info)
				mData.bRecomputeTangents = true;
				mInternalFlags.RecomputeTangents = true;
				mActorDesc->flags.RecomputeTangents = true;

				// hm, let's not spam the user, as RecomputeTangents is off by default
				//APEX_DEBUG_INFO("Asset (%s) does not support tangent skinning. Resetting RecomputeTangents to true.", mAsset->getName());
			}

			//Copy to...

			for (uint32_t b = 0; b < pLod->mSubMeshCount; b++)
			{
				ClothingAssetSubMesh* submesh = mData.mAsset.GetSubmesh(a, b);

				const uint32_t numParts = (uint32_t)mAsset->getGraphicalLod(a)->physicsMeshPartitioning.arraySizes[0];
				ClothingGraphicalLodParametersNS::PhysicsMeshPartitioning_Type* parts = mAsset->getGraphicalLod(a)->physicsMeshPartitioning.buf;

#if defined _DEBUG || PX_CHECKED
				bool found = false;
#endif
				for (uint32_t c = 0; c < numParts; c++)
				{
					if (parts[c].graphicalSubmesh == b)
					{
						submesh->mCurrentMaxVertexSimulation = parts[c].numSimulatedVertices;
						submesh->mCurrentMaxVertexAdditionalSimulation = parts[c].numSimulatedVerticesAdditional;
						submesh->mCurrentMaxIndexSimulation = parts[c].numSimulatedIndices;

						largestSubmesh = PxMax(largestSubmesh, parts[c].numSimulatedVerticesAdditional);
#if defined _DEBUG || PX_CHECKED
						found = true;
#endif
						break;
					}
				}
#if defined _DEBUG || PX_CHECKED
				PX_ASSERT(found);
#endif
			}
		}
		mData.mActorScale = mActorDesc->actorScale;

		mData.bShouldComputeRenderData = shouldComputeRenderData();
		mData.bInternalFrozen = bInternalFrozen;
		mData.bCorrectSimulationNormals = mInternalFlags.CorrectSimulationNormals;
		mData.bParallelCpuSkinning = mInternalFlags.ParallelCpuSkinning;
		mData.mGlobalPose = mActorDesc->globalPose;

		mData.mInternalMatricesCount = mActorDesc->useInternalBoneOrder ? mAsset->getNumUsedBones() : mAsset->getNumBones();
	}

}

void ClothingActorImpl::syncActorData()
{
	PX_PROFILE_ZONE("ClothingActorImpl::syncActorData", GetInternalApexSDK()->getContextId());

	if (mData.bIsInitialized && mData.bShouldComputeRenderData && mClothingSimulation != NULL /*&& bInternalFrozen == 0*/)
	{
		PX_ASSERT(!mData.mNewBounds.isEmpty());
		PX_ASSERT(mData.mNewBounds.isFinite());

		//Write back all the modified variables so that the simulation is consistent
		mNewBounds = mData.mNewBounds;
	}
	else
	{
		mNewBounds = getRenderMeshAssetBoundsTransformed();
	}

	if (bInternalLocalSpaceSim == 1)
	{
#if _DEBUG
		bool ok = true;
		ok &= mInternalGlobalPose.column0.isNormalized();
		ok &= mInternalGlobalPose.column1.isNormalized();
		ok &= mInternalGlobalPose.column2.isNormalized();
		if (!ok)
		{
			APEX_DEBUG_WARNING("Internal Global Pose is not normalized (Scale: %f %f %f). Bounds could be wrong.", mInternalGlobalPose.column0.magnitude(), mInternalGlobalPose.column1.magnitude(), mInternalGlobalPose.column2.magnitude());
		}
#endif
		PX_ASSERT(!mNewBounds.isEmpty());
		mRenderBounds = PxBounds3::transformFast(PxTransform(mInternalGlobalPose), mNewBounds);
	}
	else
	{
		mRenderBounds = mNewBounds;
	}

	markRenderProxyReady();
}



void ClothingActorImpl::markRenderProxyReady()
{
	PX_PROFILE_ZONE("ClothingActorImpl::markRenderProxyReady", GetInternalApexSDK()->getContextId());
	mRenderProxyMutex.lock();
	if (mRenderProxyReady != NULL)
	{
		// user didn't request the renderable after fetchResults,
		// let's release it, so it can be reused
		mRenderProxyReady->release();
	}

	ClothingRenderProxyImpl* renderProxy = mGraphicalMeshes[mCurrentGraphicalLodId].renderProxy;
	if (renderProxy != NULL)
	{
		updateBoneBuffer(renderProxy);
		mGraphicalMeshes[mCurrentGraphicalLodId].renderProxy = NULL;
		renderProxy->setBounds(mRenderBounds);
	}

	mRenderProxyReady = renderProxy;
	mRenderProxyMutex.unlock();
}



void ClothingActorImpl::fillWritebackData_LocksPhysX(const WriteBackInfo& writeBackInfo)
{
	PX_ASSERT(mClothingSimulation != NULL);
	PX_ASSERT(mClothingSimulation->physicalMeshId != 0xffffffff);
	PX_ASSERT(writeBackInfo.simulationDelta >= 0.0f);

	ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* destPhysicalMesh = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId);

	// copy the data from the old mesh
	// use bigger position buffer as temp buffer
	uint32_t numCopyVertices = 0;
	PxVec3* velocities = NULL;

	float transitionMapThickness = 0.0f;
	float transitionMapOffset = 0.0f;
	const ClothingPhysicalMeshParametersNS::SkinClothMapB_Type* pTCMB = NULL;
	const ClothingPhysicalMeshParametersNS::SkinClothMapD_Type* pTCM = NULL;
	if (writeBackInfo.oldSimulation)
	{
		PX_ASSERT(writeBackInfo.oldSimulation->physicalMeshId != 0xffffffff);
		pTCMB = mAsset->getTransitionMapB(mClothingSimulation->physicalMeshId, writeBackInfo.oldSimulation->physicalMeshId, transitionMapThickness, transitionMapOffset);
		pTCM = mAsset->getTransitionMap(mClothingSimulation->physicalMeshId, writeBackInfo.oldSimulation->physicalMeshId, transitionMapThickness, transitionMapOffset);
	}

	if (pTCMB != NULL || pTCM != NULL)
	{
		ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* srcPhysicalMesh = mAsset->getPhysicalMeshFromLod(writeBackInfo.oldGraphicalLodId);
		PX_ASSERT(srcPhysicalMesh != NULL);

		AbstractMeshDescription srcPM;
		srcPM.numIndices = writeBackInfo.oldSimulation->sdkNumDeformableIndices;
		srcPM.numVertices = writeBackInfo.oldSimulation->sdkNumDeformableVertices;
		srcPM.pIndices = srcPhysicalMesh->indices.buf;
		srcPM.pNormal = writeBackInfo.oldSimulation->sdkWritebackNormal;
		srcPM.pPosition = writeBackInfo.oldSimulation->sdkWritebackPosition;
		srcPM.avgEdgeLength = transitionMapThickness;

		// new position and normal buffer will be initialized afterwards, buffer can be used here
		if (mClothingSimulation->sdkNumDeformableVertices > writeBackInfo.oldSimulation->sdkNumDeformableVertices)
		{
			transferVelocities_LocksPhysX(*writeBackInfo.oldSimulation, pTCMB, pTCM, destPhysicalMesh->numVertices, srcPM.pIndices, srcPM.numIndices, srcPM.numVertices,
			                              mClothingSimulation->sdkWritebackPosition, mClothingSimulation->sdkWritebackNormal, writeBackInfo.simulationDelta);
		}

		// PH: sdkWritebackPosition will contain qnans for the values that have not been written.
		intrinsics::memSet(mClothingSimulation->sdkWritebackPosition, 0xff, sizeof(PxVec3) * mClothingSimulation->sdkNumDeformableVertices);

		if (pTCMB != NULL)
		{
			numCopyVertices = mData.mAsset.skinClothMapB(mClothingSimulation->sdkWritebackPosition, mClothingSimulation->sdkWritebackNormal,
			                  mClothingSimulation->sdkNumDeformableVertices, srcPM, (ClothingGraphicalLodParametersNS::SkinClothMapB_Type*)pTCMB, destPhysicalMesh->numVertices, true);
		}
		else
		{
			numCopyVertices = mData.mAsset.skinClothMap<true>(mClothingSimulation->sdkWritebackPosition, mClothingSimulation->sdkWritebackNormal, NULL, mClothingSimulation->sdkNumDeformableVertices,
			                  srcPM, (ClothingGraphicalLodParametersNS::SkinClothMapD_Type*)pTCM, destPhysicalMesh->numVertices, transitionMapOffset, mActorDesc->actorScale);
		}

		// don't need old positions and normals anymore
		if (writeBackInfo.oldSimulation->sdkNumDeformableVertices >= mClothingSimulation->sdkNumDeformableVertices)
		{
			transferVelocities_LocksPhysX(*writeBackInfo.oldSimulation, pTCMB, pTCM, destPhysicalMesh->numVertices, srcPM.pIndices, srcPM.numIndices, srcPM.numVertices,
			                              writeBackInfo.oldSimulation->sdkWritebackPosition, writeBackInfo.oldSimulation->sdkWritebackNormal, writeBackInfo.simulationDelta);
		}
	}
	else if (writeBackInfo.oldSimulation != NULL && writeBackInfo.oldSimulation->physicalMeshId == mClothingSimulation->physicalMeshId)
	{
		if (writeBackInfo.oldSimulation->sdkNumDeformableVertices < mClothingSimulation->sdkNumDeformableVertices)
		{
			// old is smaller
			numCopyVertices = writeBackInfo.oldSimulation->sdkNumDeformableVertices;
			velocities = mClothingSimulation->sdkWritebackPosition;
			copyAndComputeVelocities_LocksPhysX(numCopyVertices, writeBackInfo.oldSimulation, velocities, writeBackInfo.simulationDelta);

			// PH: sdkWritebackPosition will contain qnans for the values that have not been written.
			intrinsics::memSet(mClothingSimulation->sdkWritebackPosition, 0xff, sizeof(PxVec3) * mClothingSimulation->sdkNumDeformableVertices);

			copyPositionAndNormal_NoPhysX(numCopyVertices, writeBackInfo.oldSimulation);
		}
		else
		{
			// new is smaller
			numCopyVertices = mClothingSimulation->sdkNumDeformableVertices;
			velocities = writeBackInfo.oldSimulation->sdkWritebackPosition;

			// PH: sdkWritebackPosition will contain qnans for the values that have not been written.
			intrinsics::memSet(mClothingSimulation->sdkWritebackPosition, 0xff, sizeof(PxVec3) * mClothingSimulation->sdkNumDeformableVertices);

			copyPositionAndNormal_NoPhysX(numCopyVertices, writeBackInfo.oldSimulation);
			copyAndComputeVelocities_LocksPhysX(numCopyVertices, writeBackInfo.oldSimulation, velocities, writeBackInfo.simulationDelta);
		}
	}
	else
	{
		velocities = mClothingSimulation->sdkWritebackPosition;
		copyAndComputeVelocities_LocksPhysX(0, writeBackInfo.oldSimulation, velocities, writeBackInfo.simulationDelta);

		// PH: sdkWritebackPosition will contain qnans for the values that have not been written.
		intrinsics::memSet(mClothingSimulation->sdkWritebackPosition, 0xff, sizeof(PxVec3) * mClothingSimulation->sdkNumDeformableVertices);
	}


	const PxVec3* positions = destPhysicalMesh->vertices.buf;
	const PxVec3* normals = destPhysicalMesh->normals.buf;
	const uint32_t numBoneIndicesPerVertex = destPhysicalMesh->numBonesPerVertex;
	const uint16_t* simBoneIndices = destPhysicalMesh->boneIndices.buf;
	const float* simBoneWeights = destPhysicalMesh->boneWeights.buf;

	// apply an initial skinning on the physical mesh
	// ASSUMPTION: when allocated, mSdkWriteback* buffers will always contain meaningful data, so initialize correctly!
	// All data that is not skinned from the old to the new mesh will have non-finite values (qnan)

	const uint8_t* const PX_RESTRICT optimizationData = destPhysicalMesh->optimizationData.buf;
	PX_ASSERT(optimizationData != NULL);

	const PxMat44* matrices = mData.mInternalBoneMatricesCur;
	if (matrices != NULL)
	{
		// one pass of cpu skinning on the physical mesh
		const bool useNormals = mClothingSimulation->sdkWritebackNormal != NULL;

		const uint32_t numVertices = destPhysicalMesh->numVertices;
		for (uint32_t i = numCopyVertices; i < numVertices; i++)
		{
			const uint8_t shift = 4 * (i % 2);
			const uint8_t numBones = uint8_t((optimizationData[i / 2] >> shift) & 0x7);

			const uint32_t vertexIndex = (pTCMB == NULL) ? i : pTCMB[i].vertexIndexPlusOffset;

			if (vertexIndex >= mClothingSimulation->sdkNumDeformableVertices)
			{
				continue;
			}

			if (PxIsFinite(mClothingSimulation->sdkWritebackPosition[vertexIndex].x))
			{
				continue;
			}

			Simd4f positionV = gSimd4fZero;
			Simd4f normalV = gSimd4fZero;

			const uint32_t numUsedBones = mAsset->getNumUsedBones();
			PX_UNUSED(numUsedBones);

			for (uint32_t j = 0; j < numBones; j++)
			{

				Simd4f weightV = Simd4fScalarFactory(simBoneWeights[vertexIndex * numBoneIndicesPerVertex + j]);
				uint16_t index = simBoneIndices[vertexIndex * numBoneIndicesPerVertex + j];
				PX_ASSERT(index < numUsedBones);

				const PxMat44& bone = (PxMat44&)matrices[index];

				Simd4f pV = applyAffineTransform(bone, createSimd3f(positions[vertexIndex]));
				pV = pV * weightV;
				positionV = positionV + pV;

				if (useNormals)
				{
					Simd4f nV = applyLinearTransform(bone, createSimd3f(normals[vertexIndex]));
					nV = nV * weightV;
					normalV = normalV + nV;
				}

			}

			if (useNormals)
			{
				normalV = normalV * rsqrt(dot3(normalV, normalV));
				store3(&mClothingSimulation->sdkWritebackNormal[vertexIndex].x, normalV);
			}
			store3(&mClothingSimulation->sdkWritebackPosition[vertexIndex].x, positionV);
		}
	}
	else
	{
		// no bone matrices, just move into world space
		const uint32_t numVertices = destPhysicalMesh->numVertices;
		PxMat44 TM = bInternalLocalSpaceSim == 1 ? PxMat44(PxIdentity) * mActorDesc->actorScale : mInternalGlobalPose;
		for (uint32_t i = numCopyVertices; i < numVertices; i++)
		{
			const uint32_t vertexIndex = (pTCMB == NULL) ? i : pTCMB[i].vertexIndexPlusOffset;
			if (vertexIndex >= mClothingSimulation->sdkNumDeformableVertices)
			{
				continue;
			}

			if (PxIsFinite(mClothingSimulation->sdkWritebackPosition[vertexIndex].x))
			{
				continue;
			}

			mClothingSimulation->sdkWritebackPosition[vertexIndex] = TM.transform(positions[vertexIndex]);
			if (mClothingSimulation->sdkWritebackNormal != NULL)
			{
				mClothingSimulation->sdkWritebackNormal[vertexIndex] = TM.rotate(normals[vertexIndex]);
			}
		}
	}
}



/// todo simdify?
void ClothingActorImpl::applyVelocityChanges_LocksPhysX(float simulationDelta)
{
	if (mClothingSimulation == NULL)
	{
		return;
	}

	float pressure = mActorDesc->pressure;
	if (pressure >= 0.0f)
	{
		ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* mesh = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId);
		if (!mesh->isClosed)
		{
			pressure = -1.0f;

			if (bPressureWarning == 0)
			{
				bPressureWarning = 1;
				if (!mesh->isClosed)
				{
					APEX_INTERNAL_ERROR("Pressure requires a closed mesh!\n");
				}
				else
				{
					APEX_INTERNAL_ERROR("Pressure only works on Physics LODs where all vertices are active!\n");
				}
			}
		}
	}

	// did the simulation handle pressure already?
	const bool needsPressure = !mClothingSimulation->applyPressure(pressure) && (pressure > 0.0f);

	if (mInternalWindParams.Adaption > 0.0f || mVelocityCallback != NULL || mActorDesc->useVelocityClamping || needsPressure)
	{
		PX_ASSERT(mClothingScene);
		PX_PROFILE_ZONE("ClothingActorImpl::applyVelocityChanges", GetInternalApexSDK()->getContextId());

		const uint32_t numVertices = mClothingSimulation->sdkNumDeformableVertices;

		// copy velocities to temp array
		PxVec3* velocities = mClothingSimulation->skinnedPhysicsNormals;

		// use the skinnedPhysics* buffers when possible
		const bool doNotUseWritebackMemory = (bBoneMatricesChanged == 0 && bGlobalPoseChanged == 0);

		if (doNotUseWritebackMemory)
		{
			velocities = (PxVec3*)GetInternalApexSDK()->getTempMemory(sizeof(PxVec3) * numVertices);
		}

		if (velocities == NULL)
		{
			return;
		}

		mClothingSimulation->getVelocities(velocities);
		// positions never need to be read!

		bool writeVelocities = false;

		// get pointers
		ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId);
		const PxVec3* assetNormals = physicalMesh->normals.buf;
		const ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type* coeffs = physicalMesh->constrainCoefficients.buf;
		const PxVec3* normals = (mClothingSimulation->sdkWritebackNormal != NULL) ? mClothingSimulation->sdkWritebackNormal : assetNormals;

		PxVec3 windVelocity = mInternalWindParams.Velocity;
		if (windVelocity.magnitudeSquared() > 0.0f && bInternalLocalSpaceSim == 1)
		{
#if _DEBUG
			bool ok = true;
			ok &= mInternalGlobalPose.column0.isNormalized();
			ok &= mInternalGlobalPose.column1.isNormalized();
			ok &= mInternalGlobalPose.column2.isNormalized();
			if (!ok)
			{
				APEX_DEBUG_WARNING("Internal Global Pose is not normalized (Scale: %f %f %f). Velocities could be wrong.", mInternalGlobalPose.column0.magnitude(), mInternalGlobalPose.column1.magnitude(), mInternalGlobalPose.column2.magnitude());
			}
#endif
			PxMat44 invGlobalPose = mInternalGlobalPose.inverseRT();
			windVelocity = invGlobalPose.rotate(windVelocity);
		}

		// modify velocities (2.8.x) or set acceleration (3.x) based on wind
		writeVelocities |= mClothingSimulation->applyWind(velocities, normals, coeffs, windVelocity, mInternalWindParams.Adaption, simulationDelta);

		// clamp velocities
		if (mActorDesc->useVelocityClamping)
		{
			PxBounds3 velocityClamp(mActorDesc->vertexVelocityClamp);
			for (uint32_t i = 0; i < numVertices; i++)
			{
				PxVec3 velocity = velocities[i];
				velocity.x = PxClamp(velocity.x, velocityClamp.minimum.x, velocityClamp.maximum.x);
				velocity.y = PxClamp(velocity.y, velocityClamp.minimum.y, velocityClamp.maximum.y);
				velocity.z = PxClamp(velocity.z, velocityClamp.minimum.z, velocityClamp.maximum.z);
				velocities[i] = velocity;
			}
			writeVelocities = true;
		}

		if (needsPressure)
		{
			PX_ALWAYS_ASSERT();
			//writeVelocities = true;
		}

		if (mVelocityCallback != NULL)
		{
			PX_PROFILE_ZONE("ClothingActorImpl::velocityShader", GetInternalApexSDK()->getContextId());
			writeVelocities |= mVelocityCallback->velocityShader(velocities, mClothingSimulation->sdkWritebackPosition, mClothingSimulation->sdkNumDeformableVertices);
		}

		if (writeVelocities)
		{
			if (mClothingScene->mClothingDebugRenderParams->Wind != 0.0f)
			{
				mWindDebugRendering.clear(); // no memory operation!

				PxVec3* oldVelocities = (PxVec3*)GetInternalApexSDK()->getTempMemory(sizeof(PxVec3) * numVertices);
				mClothingSimulation->getVelocities(oldVelocities);

				for (uint32_t i = 0; i < numVertices; i++)
				{
					mWindDebugRendering.pushBack(velocities[i] - oldVelocities[i]);
				}

				GetInternalApexSDK()->releaseTempMemory(oldVelocities);
			}
			else if (mWindDebugRendering.capacity() > 0)
			{
				mWindDebugRendering.reset();
			}
			mClothingSimulation->setVelocities(velocities);
		}

		if (doNotUseWritebackMemory)
		{
			GetInternalApexSDK()->releaseTempMemory(velocities);
		}
	}
}



// update the renderproxy to which the data of this frame is written
void ClothingActorImpl::updateRenderProxy()
{
	PX_PROFILE_ZONE("ClothingActorImpl::updateRenderProxy", GetInternalApexSDK()->getContextId());
	PX_ASSERT(mGraphicalMeshes[mCurrentGraphicalLodId].renderProxy == NULL);

	if (mGraphicalMeshes[mCurrentGraphicalLodId].renderProxy)
	{
		mGraphicalMeshes[mCurrentGraphicalLodId].renderProxy->release();
	}

	// get a new render proxy from the pool
	RenderMeshAssetIntl* renderMeshAsset = mAsset->getGraphicalMesh(mCurrentGraphicalLodId);
	ClothingRenderProxyImpl* renderProxy = mClothingScene->getRenderProxy(renderMeshAsset, mActorDesc->fallbackSkinning, mClothingSimulation != NULL,
																	mOverrideMaterials, mActorDesc->morphGraphicalMeshNewPositions.buf, 
																	&mGraphicalMeshes[mCurrentGraphicalLodId].morphTargetVertexOffsets[0]);

	mGraphicalMeshes[mCurrentGraphicalLodId].renderProxy = renderProxy;
}



ClothingRenderProxy* ClothingActorImpl::acquireRenderProxy()
{
	PX_PROFILE_ZONE("ClothingActorImpl::acquireRenderProxy", GetInternalApexSDK()->getContextId());
	if (!mClothingScene->isSimulating()) // after fetchResults
	{
		// For consistency, only return the new result after fetchResults.
		// During simulation always return the old result
		// even if the new result might be ready
		waitForFetchResults();
	}

	mRenderProxyMutex.lock();
	ClothingRenderProxyImpl* renderProxy = mRenderProxyReady;
	mRenderProxyReady = NULL;
	mRenderProxyMutex.unlock();

	return renderProxy;
}



void ClothingActorImpl::getSimulation(const WriteBackInfo& writeBackInfo)
{
	const uint32_t physicalMeshId = mAsset->getGraphicalLod(mCurrentGraphicalLodId)->physicalMeshId;

#if defined _DEBUG || PX_CHECKED
	BackendFactory* factory = mAsset->getModuleClothing()->getBackendFactory(mBackendName);
	PX_UNUSED(factory); // Strange warning fix
#endif

	NvParameterized::Interface* cookingInterface = mActorDesc->runtimeCooked;

	if (cookingInterface != NULL)
	{
#if defined _DEBUG || PX_CHECKED
		PX_ASSERT(factory->isMatch(cookingInterface->className()));
#endif
	}
	else
	{
		cookingInterface = mAsset->getCookedData(mActorDesc->actorScale);
#if defined _DEBUG || PX_CHECKED
		PX_ASSERT(factory->isMatch(cookingInterface->className()));
#endif
	}

	mClothingSimulation = mAsset->getSimulation(physicalMeshId, cookingInterface, mClothingScene);
	if (mClothingSimulation != NULL)
	{
		mClothingSimulation->reenablePhysX(mActorProxy, mInternalGlobalPose);

		mAsset->updateCollision(mClothingSimulation, mData.mInternalBoneMatricesCur, mCollisionPlanes, mCollisionConvexes, mCollisionSpheres, mCollisionCapsules, mCollisionTriangleMeshes, true);
		mClothingSimulation->updateCollisionDescs(mActorDesc->actorDescTemplate, mActorDesc->shapeDescTemplate);

		fillWritebackData_LocksPhysX(writeBackInfo);

		mClothingSimulation->setPositions(mClothingSimulation->sdkWritebackPosition);

		PX_ASSERT(mClothingSimulation->physicalMeshId != 0xffffffff);
		PX_ASSERT(mClothingSimulation->submeshId != 0xffffffff);
	}
	else if (cookingInterface != NULL)
	{
		// will call fillWritebackData_LocksPhysX
		createSimulation(physicalMeshId, cookingInterface, writeBackInfo);
	}

	bDirtyClothingTemplate = 1; // updates the clothing desc

	if (mClothingSimulation != NULL)
	{
		// make sure skinPhysicalMesh does something
		bBoneMatricesChanged = 1;
	}
}



bool ClothingActorImpl::isCookedDataReady()
{
	// Move getResult of the Cooking Task outside because it would block the other cooking tasks when the actor stopped the simulation.
	if (mActiveCookingTask == NULL)
	{
		return true;
	}

	PX_ASSERT(mActorDesc->runtimeCooked == NULL);
	mActorDesc->runtimeCooked = mActiveCookingTask->getResult();
	if (mActorDesc->runtimeCooked != NULL)
	{
		mActiveCookingTask = NULL; // will be deleted by the scene
		return true;
	}

	if(mAsset->getModuleClothing()->allowAsyncCooking())
	{
		mClothingScene->submitCookingTask(NULL); //trigger tasks to be submitted
	}

	return false;
}



void ClothingActorImpl::createPhysX_LocksPhysX(float simulationDelta)
{
#if PX_PHYSICS_VERSION_MAJOR == 3
	if (mPhysXScene == NULL)
	{
		return;
	}
#endif
	if (mCurrentSolverIterations == 0)
	{
		return;
	}

	if (mClothingSimulation != NULL)
	{
		APEX_INTERNAL_ERROR("Physics mesh already created!");
		return;
	}

	PX_PROFILE_ZONE("ClothingActorImpl::createPhysX", GetInternalApexSDK()->getContextId());

	WriteBackInfo writeBackInfo;
	writeBackInfo.simulationDelta = simulationDelta;

	getSimulation(writeBackInfo);

	if (mClothingSimulation == NULL)
	{
		mCurrentSolverIterations = 0;
	}

	updateConstraintCoefficients_LocksPhysX();

	bBoneBufferDirty = 1;
}



void ClothingActorImpl::removePhysX_LocksPhysX()
{
	if (mClothingScene != NULL)
	{
		if (mClothingSimulation != NULL)
		{
			mAsset->returnSimulation(mClothingSimulation);
			mClothingSimulation = NULL;
		}
	}
	else
	{
		PX_ASSERT(mClothingSimulation == NULL);
	}

	bBoneBufferDirty = 1;
}



void ClothingActorImpl::changePhysicsMesh_LocksPhysX(uint32_t oldGraphicalLodId, float simulationDelta)
{
	PX_ASSERT(mClothingSimulation != NULL);
	PX_ASSERT(mClothingScene != NULL);

	WriteBackInfo writeBackInfo;
	writeBackInfo.oldSimulation = mClothingSimulation;
	writeBackInfo.oldGraphicalLodId = oldGraphicalLodId;
	writeBackInfo.simulationDelta = simulationDelta;

	getSimulation(writeBackInfo); // sets mClothingSimulation & will register the sim buffers
	writeBackInfo.oldSimulation->swapCollision(mClothingSimulation);

	mAsset->returnSimulation(writeBackInfo.oldSimulation);
	writeBackInfo.oldSimulation = NULL;

	updateConstraintCoefficients_LocksPhysX();

	// make sure skinPhysicalMesh does something
	bBoneMatricesChanged = 1;

	// make sure actorData gets updated
	reinitActorData();
}



void ClothingActorImpl::updateCollision_LocksPhysX(bool useInterpolatedMatrices)
{
	if (mClothingSimulation == NULL || (bBoneMatricesChanged == 0 && bGlobalPoseChanged == 0 && bActorCollisionChanged == 0))
	{
		return;
	}

	PX_ASSERT(mClothingScene != NULL);

	const PxMat44* matrices = useInterpolatedMatrices ? mInternalInterpolatedBoneMatrices : mData.mInternalBoneMatricesCur;
	mAsset->updateCollision(mClothingSimulation, matrices, mCollisionPlanes, mCollisionConvexes, mCollisionSpheres, mCollisionCapsules, mCollisionTriangleMeshes, bInternalTeleportDue != ClothingTeleportMode::Continuous);
	bActorCollisionChanged = 0;

	if (bDirtyActorTemplate == 1 || bDirtyShapeTemplate == 1)
	{
		mClothingSimulation->updateCollisionDescs(mActorDesc->actorDescTemplate, mActorDesc->shapeDescTemplate);

		bDirtyActorTemplate = 0;
		bDirtyShapeTemplate = 0;
	}
}



void ClothingActorImpl::updateConstraintCoefficients_LocksPhysX()
{
	if (mClothingSimulation == NULL)
	{
		return;
	}

	const ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId);
	const ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type* assetCoeffs = physicalMesh->constrainCoefficients.buf;

	const float actorScale = mActorDesc->actorScale;

	const float linearScale = (mInternalMaxDistanceScale.Multipliable ? mInternalMaxDistanceScale.Scale : 1.0f) * actorScale;
	const float absoluteScale = mInternalMaxDistanceScale.Multipliable ? 0.0f : (physicalMesh->maximumMaxDistance * (1.0f - mInternalMaxDistanceScale.Scale));

	const float reduceMaxDistance = mMaxDistReduction + absoluteScale;

	mCurrentMaxDistanceBias = 0.0f;
	ClothingMaterialLibraryParametersNS::ClothingMaterial_Type* clothingMaterial = getCurrentClothingMaterial();
	if (clothingMaterial != NULL)
	{
		mCurrentMaxDistanceBias = clothingMaterial->maxDistanceBias;
	}

	mClothingSimulation->setConstrainCoefficients(assetCoeffs, reduceMaxDistance, linearScale, mCurrentMaxDistanceBias, actorScale);

	// change is applied now
	bMaxDistanceScaleChanged = 0;
}



void ClothingActorImpl::copyPositionAndNormal_NoPhysX(uint32_t numCopyVertices, SimulationAbstract* oldClothingSimulation)
{
	if (oldClothingSimulation == NULL)
	{
		return;
	}

	PX_ASSERT(numCopyVertices <= mClothingSimulation->sdkNumDeformableVertices && numCopyVertices <= oldClothingSimulation->sdkNumDeformableVertices);

	memcpy(mClothingSimulation->sdkWritebackPosition, oldClothingSimulation->sdkWritebackPosition, sizeof(PxVec3) * numCopyVertices);

	if (mClothingSimulation->sdkWritebackNormal != NULL)
	{
		memcpy(mClothingSimulation->sdkWritebackNormal, oldClothingSimulation->sdkWritebackNormal, sizeof(PxVec3) * numCopyVertices);
	}
}



void ClothingActorImpl::copyAndComputeVelocities_LocksPhysX(uint32_t numCopyVertices, SimulationAbstract* oldClothingSimulation, PxVec3* velocities, float simulationDelta) const
{
	PX_ASSERT(mClothingScene != NULL);

	// copy
	if (oldClothingSimulation != NULL && numCopyVertices > 0)
	{
		oldClothingSimulation->getVelocities(velocities);
	}

	// compute velocity from old and current skinned pos
	// TODO only skin when bone matrices have changed -> how to use bBoneMatricesChanged in a safe way?
	const ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId);
	PX_ASSERT(physicalMesh != NULL);

	const uint32_t numVertices = physicalMesh->numSimulatedVertices;
	PX_ASSERT(numVertices == mClothingSimulation->sdkNumDeformableVertices);
	if (mData.mInternalBoneMatricesCur != NULL && mData.mInternalBoneMatricesPrev != NULL && simulationDelta > 0 && bBoneMatricesChanged == 1)
	{
		// cpu skinning on the physical mesh
		for (uint32_t i = numCopyVertices; i < numVertices; i++)
		{
			velocities[i] = computeVertexVelFromAnim(i, physicalMesh, simulationDelta);
		}
	}
	else
	{
		// no bone matrices, just set 0 velocities
		memset(velocities + numCopyVertices, 0, sizeof(PxVec3) * (numVertices - numCopyVertices));
	}

	// set the velocities
	mClothingSimulation->setVelocities(velocities);
}



void ClothingActorImpl::transferVelocities_LocksPhysX(const SimulationAbstract& oldClothingSimulation,
        const ClothingPhysicalMeshParametersNS::SkinClothMapB_Type* pTCMB,
        const ClothingPhysicalMeshParametersNS::SkinClothMapD_Type* pTCM,
        uint32_t numVerticesInMap, const uint32_t* srcIndices, uint32_t numSrcIndices, uint32_t numSrcVertices,
        PxVec3* oldVelocities, PxVec3* newVelocities, float simulationDelta)
{
	oldClothingSimulation.getVelocities(oldVelocities);

	// data for skinning
	const ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId);

	// copy velocities
	uint32_t vertexIndex = (uint32_t) - 1;
	uint32_t idx[3] =
	{
		(uint32_t) - 1,
		(uint32_t) - 1,
		(uint32_t) - 1,
	};
	for (uint32_t i = 0; i < numVerticesInMap; ++i)
	{
		if (pTCMB)
		{
			uint32_t faceIndex = pTCMB[i].faceIndex0;
			idx[0] = (faceIndex >= numSrcIndices) ? srcIndices[faceIndex + 0] : (uint32_t) - 1;
			idx[1] = (faceIndex >= numSrcIndices) ? srcIndices[faceIndex + 1] : (uint32_t) - 1;
			idx[2] = (faceIndex >= numSrcIndices) ? srcIndices[faceIndex + 2] : (uint32_t) - 1;

			vertexIndex = pTCMB[i].vertexIndexPlusOffset;
		}
		else if (pTCM)
		{
			idx[0] = pTCM[i].vertexIndex0;
			idx[1] = pTCM[i].vertexIndex1;
			idx[2] = pTCM[i].vertexIndex2;
			vertexIndex = pTCM[i].vertexIndexPlusOffset;
			//PX_ASSERT(i == pTCM[i].vertexIndexPlusOffset);
		}
		else
		{
			PX_ALWAYS_ASSERT();
		}

		if (vertexIndex >= mClothingSimulation->sdkNumDeformableVertices)
		{
			continue;
		}

		if (idx[0] >= numSrcVertices || idx[1] >= numSrcVertices || idx[2] >= numSrcVertices)
		{
			// compute from anim
			if (mData.mInternalBoneMatricesPrev == NULL || simulationDelta == 0.0f)
			{
				newVelocities[vertexIndex] = PxVec3(0.0f);
			}
			else
			{
				newVelocities[vertexIndex] = computeVertexVelFromAnim(vertexIndex, physicalMesh, simulationDelta);
			}
		}
		else
		{
			// transfer from old mesh
			newVelocities[vertexIndex] = (oldVelocities[idx[0]] + oldVelocities[idx[1]] + oldVelocities[idx[2]]) / 3.0f;
		}
	}

	mClothingSimulation->setVelocities(newVelocities);
}



PxVec3 ClothingActorImpl::computeVertexVelFromAnim(uint32_t vertexIndex, const ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh, float simulationDelta) const
{
	PX_ASSERT(simulationDelta > 0);
	PX_ASSERT(mData.mInternalBoneMatricesCur != NULL);
	PX_ASSERT(mData.mInternalBoneMatricesPrev != NULL);

	const PxVec3* positions = physicalMesh->vertices.buf;
	Simd4f simulationDeltaV = Simd4fScalarFactory(simulationDelta);

	uint32_t numBoneIndicesPerVertex = physicalMesh->numBonesPerVertex;
	const uint16_t* simBoneIndices = physicalMesh->boneIndices.buf;
	const float* simBoneWeights = physicalMesh->boneWeights.buf;

	const uint8_t* const optimizationData = physicalMesh->optimizationData.buf;
	PX_ASSERT(optimizationData != NULL);

	const uint8_t shift = 4 * (vertexIndex % 2);
	const uint8_t numBones = uint8_t((optimizationData[vertexIndex / 2] >> shift) & 0x7);

	Simd4f oldPosV = gSimd4fZero;
	Simd4f newPosV = gSimd4fZero;
	for (uint32_t j = 0; j < numBones; j++)
	{
		const Simd4f weightV = Simd4fScalarFactory(simBoneWeights[vertexIndex * numBoneIndicesPerVertex + j]);

		const uint16_t index = simBoneIndices[vertexIndex * numBoneIndicesPerVertex + j];
		PX_ASSERT(index < mAsset->getNumUsedBones());
		const PxMat44& oldBoneV = (PxMat44&)mData.mInternalBoneMatricesPrev[index];
		const PxMat44& boneV = (PxMat44&)mData.mInternalBoneMatricesCur[index];


		//oldPos += oldBone * positions[vertexIndex] * weight;
		Simd4f pV = applyAffineTransform(oldBoneV, createSimd3f(positions[vertexIndex]));
		pV = pV * weightV;
		oldPosV = oldPosV + pV;

		//newPos += bone * positions[vertexIndex] * weight;
		pV = applyAffineTransform(boneV, createSimd3f(positions[vertexIndex]));
		pV = pV * weightV;
		newPosV = newPosV + pV;

	}

	Simd4f velV = newPosV - oldPosV;
	velV = velV / simulationDeltaV;

	PxVec3 vel;
	store3(&vel.x, velV);
	return vel;
}



void ClothingActorImpl::createSimulation(uint32_t physicalMeshId, NvParameterized::Interface* cookedData, const WriteBackInfo& writeBackInfo)
{
	PX_ASSERT(mClothingSimulation == NULL);
#if PX_PHYSICS_VERSION_MAJOR == 3
	if (mPhysXScene == NULL)
	{
		return;
	}
#endif
	if (bUnsucessfullCreation == 1)
	{
		return;
	}

	PX_PROFILE_ZONE("ClothingActorImpl::SDKCreateClothSoftbody", GetInternalApexSDK()->getContextId());

	mClothingSimulation = mAsset->getModuleClothing()->getBackendFactory(mBackendName)->createSimulation(mClothingScene, mActorDesc->useHardwareCloth);

	bool success = false;

	if (mClothingSimulation != NULL)
	{
		ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId);
		const uint32_t numVertices = physicalMesh->numSimulatedVertices;
		const uint32_t numIndices  = physicalMesh->numSimulatedIndices;

		mClothingSimulation->init(numVertices, numIndices, true);

		PX_ASSERT(nvidia::strcmp(mAsset->getAssetNvParameterized()->className(), ClothingAssetParameters::staticClassName()) == 0);
		const ClothingAssetParameters* assetParams = static_cast<const ClothingAssetParameters*>(mAsset->getAssetNvParameterized());
		mClothingSimulation->initSimulation(assetParams->simulation);

		mClothingSimulation->physicalMeshId = physicalMeshId;
		fillWritebackData_LocksPhysX(writeBackInfo);

		uint32_t* indices = NULL;
		PxVec3* vertices = NULL;
		if (physicalMesh != NULL)
		{
			indices = physicalMesh->indices.buf;
			vertices = physicalMesh->vertices.buf;
		}

		success = mClothingSimulation->setCookedData(cookedData, mActorDesc->actorScale);

		mAsset->initCollision(mClothingSimulation, mData.mInternalBoneMatricesCur, mCollisionPlanes, mCollisionConvexes, mCollisionSpheres, mCollisionCapsules, mCollisionTriangleMeshes, mActorDesc, mInternalGlobalPose, bInternalLocalSpaceSim == 1);

		// sets positions to sdkWritebackPosition
		ClothingMaterialLibraryParametersNS::ClothingMaterial_Type* clothingMaterial = getCurrentClothingMaterial();
		success &= mClothingSimulation->initPhysics(physicalMeshId, indices, vertices, clothingMaterial, mInternalGlobalPose, mInternalScaledGravity, bInternalLocalSpaceSim == 1);

		if (success)
		{
			mClothingSimulation->registerPhysX(mActorProxy);
		}

		mClothingScene->registerAsset(mAsset);
	}

	if (!success)
	{
		bUnsucessfullCreation = 1;

		if (mClothingSimulation != NULL)
		{
			PX_DELETE_AND_RESET(mClothingSimulation);
		}
	}
}



float ClothingActorImpl::getCost() const
{
	ClothingMaterialLibraryParametersNS::ClothingMaterial_Type* clothingMaterial = getCurrentClothingMaterial();
	const uint32_t solverIterations = clothingMaterial != NULL ? clothingMaterial->solverIterations : 5;

	float cost = 0.0f;
	if (mClothingSimulation != NULL)
	{
		cost = static_cast<float>(solverIterations * mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId)->numSimulatedVertices);
	}
	return cost;
}



uint32_t ClothingActorImpl::getGraphicalMeshIndex(uint32_t lod) const
{
	for (uint32_t i = 1; i < mAsset->getNumGraphicalMeshes(); i++)
	{
		if (mAsset->getGraphicalLod(i)->lod > lod)
		{
			return i - 1;
		}
	}

	// not found, return last index
	return mAsset->getNumGraphicalMeshes() - 1;
}



void ClothingActorImpl::lodTick_LocksPhysX(float simulationDelta)
{
	PX_PROFILE_ZONE("ClothingActorImpl::lodTick", GetInternalApexSDK()->getContextId());

	bool actorCooked = isCookedDataReady();

	// update graphics lod
	// hlanker: active needs a lock if parallel active checks are allowed (like parallel updateRenderResource)
	for (uint32_t i = 0; i < mGraphicalMeshes.size(); i++)
	{
		mGraphicalMeshes[i].active = false;
	}

	const uint32_t newGraphicalLodId = getGraphicalMeshIndex(mBufferedGraphicalLod);

	if (newGraphicalLodId >= mGraphicalMeshes.size())
	{
		return;
	}

	mGraphicalMeshes[newGraphicalLodId].active = true;

	uint32_t oldPhysicalMeshId = mAsset->getGraphicalLod(mCurrentGraphicalLodId)->physicalMeshId;
	const bool physicalMeshChanged = oldPhysicalMeshId != mAsset->getGraphicalLod(newGraphicalLodId)->physicalMeshId;
	if (physicalMeshChanged)
	{
		bInternalScaledGravityChanged = 1;
	}

	const bool graphicalLodChanged = newGraphicalLodId != mCurrentGraphicalLodId;
	const uint32_t oldGraphicalLodId = mCurrentGraphicalLodId;
	mCurrentGraphicalLodId = newGraphicalLodId;

	if (mForceSimulation < 0)
	{
		mForceSimulation = 1;
	}
	bIsSimulationOn = mForceSimulation > 0? false : true;

	float maxDistReductionTarget = 0.f;
	if (graphicalLodChanged)
	{
		// interrupt blending when switching graphical lod
		mMaxDistReduction = maxDistReductionTarget;
	}

	// must not enter here if graphical lod changed. otherwise we'll assert in updateConstraintCoefficients because of a pointer mismatch
	if (mAsset->getGraphicalLod(mCurrentGraphicalLodId)->physicalMeshId != uint32_t(-1) && !graphicalLodChanged)
	{
		const ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId);

		// update maxDistReductionTarget
		// if there is no simulation bulk, wait with reducing the maxDistReductionTarget
		if (mMaxDistReduction != maxDistReductionTarget && mClothingSimulation != NULL)
		{
			const float maxBlendDistance = physicalMesh->maximumMaxDistance * (mInternalMaxDistanceBlendTime > 0 ? (simulationDelta / mInternalMaxDistanceBlendTime) : 1.0f);
			if (mMaxDistReduction == -1.0f)
			{
				// initialization
				mMaxDistReduction = maxDistReductionTarget;
			}
			else if (PxAbs(maxDistReductionTarget - mMaxDistReduction) < maxBlendDistance)
			{
				// distance multiplier target reached
				mMaxDistReduction = maxDistReductionTarget;
			}
			else if (bBlendingAllowed == 0)
			{
				// No blending
				mMaxDistReduction = maxDistReductionTarget;
			}
			else
			{
				// move towards distance multiplier target
				mMaxDistReduction += PxSign(maxDistReductionTarget - mMaxDistReduction) * maxBlendDistance;
			}
			updateConstraintCoefficients_LocksPhysX();
		}
		else if (mCurrentMaxDistanceBias != mClothingMaterial.maxDistanceBias)
		{
			// update them if the max distance bias changes
			updateConstraintCoefficients_LocksPhysX();
		}
		else if (bMaxDistanceScaleChanged == 1)
		{
			updateConstraintCoefficients_LocksPhysX();
		}
	}


	// switch immediately when simulation was switched off or when graphical lod has changed, otherwise wait until finished blending
	if (mMaxDistReduction >= maxDistReductionTarget)
	{
		ClothingMaterialLibraryParametersNS::ClothingMaterial_Type* clothingMaterial = getCurrentClothingMaterial();
		const uint32_t solverIterations = clothingMaterial != NULL ? clothingMaterial->solverIterations : 5;

		uint32_t solverIterationsTarget = solverIterations;

		bool solverIterChanged = (mCurrentSolverIterations != solverIterationsTarget);
		mCurrentSolverIterations = solverIterationsTarget;
		if (actorCooked && mCurrentSolverIterations > 0)
		{
			if (mClothingSimulation == NULL)
			{
				createPhysX_LocksPhysX(simulationDelta);
			}
			else if (graphicalLodChanged)
			{
				PX_ASSERT(!physicalMeshChanged || mCurrentGraphicalLodId != oldGraphicalLodId);
				changePhysicsMesh_LocksPhysX(oldGraphicalLodId, simulationDelta);
			}

			if (solverIterChanged && mClothingSimulation != NULL)
			{
				mClothingSimulation->setSolverIterations(mCurrentSolverIterations);
			}

			bInternalFrozen = bBufferedFrozen;
			freeze_LocksPhysX(bInternalFrozen == 1);
			bUpdateFrozenFlag = 0;
		}
		else
		{
			mCurrentSolverIterations = 0;

			if (!mActorDesc->freezeByLOD)
			{
				removePhysX_LocksPhysX();
			}
			else
			{
				freeze_LocksPhysX(true);
				bInternalFrozen = 1;
				bUpdateFrozenFlag = 0;
			}
		}
	}

	// get render proxy for this simulate call
	updateRenderProxy();
}



void ClothingActorImpl::visualizeSkinnedPositions(RenderDebugInterface& renderDebug, float positionRadius, bool maxDistanceOut, bool maxDistanceIn) const
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(renderDebug);
	PX_UNUSED(positionRadius);
	PX_UNUSED(maxDistanceOut);
	PX_UNUSED(maxDistanceIn);
#else
	using RENDER_DEBUG::DebugColors;
	using RENDER_DEBUG::DebugRenderState;

	if (mClothingSimulation != NULL)
	{
		const float pointRadius = positionRadius * 0.1f;
		PX_ASSERT(mClothingSimulation->skinnedPhysicsPositions != NULL);
		ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId);
		ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type* coeffs = physicalMesh->constrainCoefficients.buf;


		const float actorScale = mActorDesc->actorScale;

		const float linearScale = (mInternalMaxDistanceScale.Multipliable ? mInternalMaxDistanceScale.Scale : 1.0f) * actorScale;
		const float absoluteScale = mInternalMaxDistanceScale.Multipliable ? 0.0f : (physicalMesh->maximumMaxDistance * (1.0f - mInternalMaxDistanceScale.Scale));

		const float reduceMaxDistance = mMaxDistReduction + absoluteScale;

		const float maxMotionRadius = (physicalMesh->maximumMaxDistance - reduceMaxDistance) * linearScale;

		const uint32_t colorGreen = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Green);
		const uint32_t colorBlue = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Blue);

		const uint32_t numVertices = physicalMesh->numSimulatedVertices;
		for (uint32_t i = 0; i < numVertices; i++)
		{
			const float maxDistance = PxMax(0.0f, coeffs[i].maxDistance - reduceMaxDistance) * linearScale;
			uint32_t color;
			if (maxDistance < 0.0f)
			{
				color = colorGreen;
			}
			else if (maxDistance == 0.0f)
			{
				color = colorBlue;
			}
			else
			{
				uint32_t b = (uint32_t)(255 * maxDistance / maxMotionRadius);
				color = (b << 16) + (b << 8) + b;
			}

			RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(color);
			//RENDER_DEBUG_IFACE(&renderDebug)->setCurrentDisplayTime(0.1f);
			RENDER_DEBUG_IFACE(&renderDebug)->debugPoint(mClothingSimulation->skinnedPhysicsPositions[i], pointRadius);
			//RENDER_DEBUG_IFACE(&renderDebug)->setCurrentDisplayTime();

			if (maxDistanceOut)
			{
				RENDER_DEBUG_IFACE(&renderDebug)->debugLine(
				    mClothingSimulation->skinnedPhysicsPositions[i],
				    mClothingSimulation->skinnedPhysicsPositions[i] + mClothingSimulation->skinnedPhysicsNormals[i] * maxDistance
				);
			}
			if (maxDistanceIn)
			{
				float collDist = PxMax(0.0f, coeffs[i].collisionSphereDistance * actorScale);
				//float scaledMaxDist = PxMax(0.0f, maxDistance - reduceMaxDistance);
				if (coeffs[i].collisionSphereRadius > 0.0f && collDist < maxDistance)
				{
					RENDER_DEBUG_IFACE(&renderDebug)->debugLine(
					    mClothingSimulation->skinnedPhysicsPositions[i] - mClothingSimulation->skinnedPhysicsNormals[i] * collDist,
					    mClothingSimulation->skinnedPhysicsPositions[i]
					);
				}
				else
				{
					RENDER_DEBUG_IFACE(&renderDebug)->debugLine(
					    mClothingSimulation->skinnedPhysicsPositions[i] - mClothingSimulation->skinnedPhysicsNormals[i] * maxDistance,
					    mClothingSimulation->skinnedPhysicsPositions[i]
					);
				}
			}
		}
	}
#endif
}



void ClothingActorImpl::visualizeSpheres(RenderDebugInterface& renderDebug, const PxVec3* positions, uint32_t numPositions, float radius, uint32_t color, bool wire) const
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(renderDebug);
	PX_UNUSED(positions);
	PX_UNUSED(numPositions);
	PX_UNUSED(radius);
	PX_UNUSED(color);
	PX_UNUSED(wire);
#else
	RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();
	RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(color);

	if (wire)
	{
		PxMat44 cameraPose = mClothingScene->mApexScene->getViewMatrix(0).inverseRT();
		cameraPose = mInternalGlobalPose.inverseRT() * cameraPose;
		PxVec3 cameraPos = cameraPose.getPosition();
		for (uint32_t i = 0; i < numPositions; ++i)
		{
			// face camera
			PxVec3 y = positions[i] - cameraPos;
			y.normalize();
			PxPlane p(y, 0.0f);
			PxVec3 x = p.project(cameraPose.column0.getXYZ());
			x.normalize();
			PxMat44 pose(x, y, x.cross(y), positions[i]);

			RENDER_DEBUG_IFACE(&renderDebug)->setPose(pose);
			RENDER_DEBUG_IFACE(&renderDebug)->debugCircle(PxVec3(0.0f), radius, 2);
		}
	}
	else
	{
		PxMat44 pose(PxIdentity);
		for (uint32_t i = 0; i < numPositions; ++i)
		{
			pose.setPosition(positions[i]);
			RENDER_DEBUG_IFACE(&renderDebug)->debugSphere(pose.getPosition(), radius, 0);
		}
	}
	RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
#endif
}



void ClothingActorImpl::visualizeBackstop(RenderDebugInterface& renderDebug) const
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(renderDebug);
#else
	using RENDER_DEBUG::DebugColors;
	using RENDER_DEBUG::DebugRenderState;

	if (mClothingSimulation != NULL)
	{
		PX_ASSERT(mClothingSimulation->skinnedPhysicsPositions != NULL);
		ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId);
		ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type* coeffs = physicalMesh->constrainCoefficients.buf;
		uint32_t* indices = physicalMesh->indices.buf;
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Red));

		const float actorScale = mActorDesc->actorScale;

		if (!physicalMesh->isTetrahedralMesh)
		{
			// render collision surface as triangle-mesh
			const uint32_t colorDarkRed = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::DarkRed);
			const uint32_t colorDarkBlue = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::DarkBlue);

			RENDER_DEBUG_IFACE(&renderDebug)->setCurrentState(DebugRenderState::SolidShaded);
			for (uint32_t i = 0; i < physicalMesh->numSimulatedIndices; i += 3)
			{
				PxVec3 p[3];

				bool show = true;

				for (uint32_t j = 0; j < 3; j++)
				{
					const uint32_t index = indices[i + j];
					if (coeffs[index].collisionSphereRadius <= 0.0f)
					{
						show = false;
						break;
					}

					const float collisionSphereDistance = coeffs[index].collisionSphereDistance * actorScale;
					if (collisionSphereDistance < 0.0f)
					{
						p[j] = mClothingSimulation->skinnedPhysicsPositions[index];
					}
					else
					{
						p[j] = mClothingSimulation->skinnedPhysicsPositions[index]
						       - (mClothingSimulation->skinnedPhysicsNormals[index] * collisionSphereDistance);
					}
				}

				if (show)
				{
					// frontface
					RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorDarkRed);
					RENDER_DEBUG_IFACE(&renderDebug)->debugTri(p[0], p[2], p[1]);

					// backface
					RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorDarkBlue);
					RENDER_DEBUG_IFACE(&renderDebug)->debugTri(p[0], p[1], p[2]);
				}
			}
		}
	}
#endif
}



void ClothingActorImpl::visualizeBackstopPrecise(RenderDebugInterface& renderDebug, float scale) const
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(renderDebug);
	PX_UNUSED(scale);
#else
	using RENDER_DEBUG::DebugColors;
	using RENDER_DEBUG::DebugRenderState;

	if (mClothingSimulation != NULL)
	{
		PX_ASSERT(mClothingSimulation->skinnedPhysicsPositions != NULL);
		ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId);
		ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type* coeffs = physicalMesh->constrainCoefficients.buf;

		const float shortestEdgeLength = physicalMesh->averageEdgeLength * 0.5f * scale;

		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentState(DebugRenderState::SolidShaded);

		const uint32_t colorRed = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Red);
		const uint32_t colorBlue = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Blue);

		const float actorScale = mActorDesc->actorScale;

		for (uint32_t i = 0; i < mClothingSimulation->sdkNumDeformableVertices; i++)
		{
			if (coeffs[i].collisionSphereRadius <= 0.0f)
			{
				continue;
			}

			PxVec3 skinnedPosition = mClothingSimulation->skinnedPhysicsPositions[i];
			if (coeffs[i].collisionSphereDistance > 0.0f)
			{
				skinnedPosition -= mClothingSimulation->skinnedPhysicsNormals[i] * (coeffs[i].collisionSphereDistance * actorScale);
			}

			const float collisionSphereRadius = coeffs[i].collisionSphereRadius * actorScale;

			const PxVec3 sphereCenter = skinnedPosition - mClothingSimulation->skinnedPhysicsNormals[i] * collisionSphereRadius;

			PxVec3 centerToSim = mClothingSimulation->sdkWritebackPosition[i] - sphereCenter;
			centerToSim.normalize();
			PxVec3 right = centerToSim.cross(PxVec3(0.0f, 1.0f, 0.0f));
			PxVec3 up = right.cross(centerToSim);
			PxVec3 target = sphereCenter + centerToSim * collisionSphereRadius;

			right *= shortestEdgeLength;
			up *= shortestEdgeLength;

			const float r = collisionSphereRadius;
			const float back = r - sqrtf(r * r - shortestEdgeLength * shortestEdgeLength);

			// move the verts a bit back such that they are on the sphere
			centerToSim *= back;

			PxVec3 l1 = target + right - centerToSim;
			PxVec3 l2 = target + up - centerToSim;
			PxVec3 l3 = target - right - centerToSim;
			PxVec3 l4 = target - up - centerToSim;

			RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorRed);
			RENDER_DEBUG_IFACE(&renderDebug)->debugTri(target, l1, l2);
			RENDER_DEBUG_IFACE(&renderDebug)->debugTri(target, l2, l3);
			RENDER_DEBUG_IFACE(&renderDebug)->debugTri(target, l3, l4);
			RENDER_DEBUG_IFACE(&renderDebug)->debugTri(target, l4, l1);
#if 1
			// PH: also render backfaces, in blue
			RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorBlue);
			RENDER_DEBUG_IFACE(&renderDebug)->debugTri(target, l1, l4);
			RENDER_DEBUG_IFACE(&renderDebug)->debugTri(target, l4, l3);
			RENDER_DEBUG_IFACE(&renderDebug)->debugTri(target, l3, l2);
			RENDER_DEBUG_IFACE(&renderDebug)->debugTri(target, l2, l1);
#endif
		}
	}
#endif
}



void ClothingActorImpl::visualizeBoneConnections(RenderDebugInterface& renderDebug, const PxVec3* positions, const uint16_t* boneIndices,
        const float* boneWeights, uint32_t numBonesPerVertex, uint32_t numVertices) const
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(renderDebug);
	PX_UNUSED(positions);
	PX_UNUSED(boneIndices);
	PX_UNUSED(boneWeights);
	PX_UNUSED(numBonesPerVertex);
	PX_UNUSED(numVertices);
#else
	const PxMat44* matrices = mData.mInternalBoneMatricesCur;
	if (matrices == NULL)
	{
		return;
	}

	for (uint32_t i = 0; i < numVertices; i++)
	{
		// skin the vertex
		PxVec3 pos(0.0f);
		for (uint32_t j = 0; j < numBonesPerVertex; j++)
		{
			float boneWeight = (boneWeights == NULL) ? 1.0f : boneWeights[i * numBonesPerVertex + j];
			if (boneWeight > 0.0f)
			{
				uint32_t boneIndex = boneIndices[i * numBonesPerVertex + j];
				pos += matrices[boneIndex].transform(positions[i]) * boneWeight;
			}
		}

		// draw the lines to the bones
		for (uint32_t j = 0; j < numBonesPerVertex; j++)
		{
			float boneWeight = (boneWeights == NULL) ? 1.0f : boneWeights[i * numBonesPerVertex + j];
			if (boneWeight > 0.0f)
			{
				uint32_t boneIndex = boneIndices[i * numBonesPerVertex + j];
				uint32_t b = (uint32_t)(255 * boneWeight);
				uint32_t color = (b << 16) + (b << 8) + b;
				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(color);
				RENDER_DEBUG_IFACE(&renderDebug)->debugLine(pos, matrices[boneIndex].transform(mAsset->getBoneBindPose(boneIndex).getPosition()));
			}
		}
	}
#endif
}



// collision functions
ClothingPlane* ClothingActorImpl::createCollisionPlane(const PxPlane& plane)
{
	ClothingPlane* actorPlane = NULL;
	actorPlane = PX_NEW(ClothingPlaneImpl)(mCollisionPlanes, *this, plane);
	PX_ASSERT(actorPlane != NULL);
	bActorCollisionChanged = true;
	return actorPlane;
}

ClothingConvex* ClothingActorImpl::createCollisionConvex(ClothingPlane** planes, uint32_t numPlanes)
{
	if (numPlanes < 3)
		return NULL;

	ClothingConvex* convex = NULL;
	convex = PX_NEW(ClothingConvexImpl)(mCollisionConvexes, *this, planes, numPlanes);
	PX_ASSERT(convex != NULL);
	bActorCollisionChanged = true;

	return convex;
}

ClothingSphere* ClothingActorImpl::createCollisionSphere(const PxVec3& position, float radius)
{

	ClothingSphere* actorSphere = NULL;
	actorSphere = PX_NEW(ClothingSphereImpl)(mCollisionSpheres, *this, position, radius);
	PX_ASSERT(actorSphere != NULL);
	bActorCollisionChanged = true;
	return actorSphere;
}

ClothingCapsule* ClothingActorImpl::createCollisionCapsule(ClothingSphere& sphere1, ClothingSphere& sphere2)
{
	ClothingCapsule* actorCapsule = NULL;
	actorCapsule = PX_NEW(ClothingCapsuleImpl)(mCollisionCapsules, *this, sphere1, sphere2);
	PX_ASSERT(actorCapsule != NULL);
	bActorCollisionChanged = true;
	return actorCapsule;
}

ClothingTriangleMesh* ClothingActorImpl::createCollisionTriangleMesh()
{
	ClothingTriangleMesh* triMesh = NULL;
	triMesh = PX_NEW(ClothingTriangleMeshImpl)(mCollisionTriangleMeshes, *this);
	PX_ASSERT(triMesh != NULL);
	bActorCollisionChanged = true;
	return triMesh;
}


void ClothingActorImpl::releaseCollision(ClothingCollisionImpl& collision)
{
	bActorCollisionChanged = 1;
	if (mClothingSimulation != NULL)
	{
		mClothingSimulation->releaseCollision(collision);
	}
	collision.destroy();
}

#if APEX_UE4
void ClothingActorImpl::simulate(PxF32 dt)
{
	// before tick task
	tickSynchBeforeSimulate_LocksPhysX(dt, dt, 0, 1);

	if (mClothingSimulation != NULL)
		mClothingSimulation->simulate(dt);

	// during tick task
	tickAsynch_NoPhysX(); // this is a no-op

	// start fetch result task
	mFetchResultsRunningMutex.lock();
	mFetchResultsRunning = true;
	mFetchResultsSync.reset();
	mFetchResultsRunningMutex.unlock();

	// fetch result task
	fetchResults();
	getActorData().tickSynchAfterFetchResults_LocksPhysX(); // this is a no-op
	setFetchResultsSync();
}
#endif

}
} // namespace nvidia

