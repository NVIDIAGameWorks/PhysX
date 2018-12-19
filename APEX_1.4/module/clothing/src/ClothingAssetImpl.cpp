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

#include "ClothingAsset.h"

#include "ApexAuthorableObject.h"
#include "ClothingActorProxy.h"
#include "ClothingAssetImpl.h"
#include "ClothingAssetData.h"
#include "ClothingGlobals.h"
#include "ClothingPhysicalMeshImpl.h"
#include "ClothingPreviewProxy.h"
#include "ClothingScene.h"
#include "CookingPhysX.h"
#include "ModulePerfScope.h"
#include "nvparameterized/NvParamUtils.h"

#include "ApexMath.h"
#include "ClothingActorParam.h"
#include "ClothingAssetParameters.h"
#include "ClothingGraphicalLodParameters.h"
#include "ModuleClothingHelpers.h"

#include "PxStrideIterator.h"
#include "PsSort.h"
#include "PsThread.h"
#include "ApexUsingNamespace.h"
#include "PxMat33.h"
#include "PsVecMath.h"

#include "SimulationAbstract.h"
#include "ApexPermute.h"

#include "ApexPvdClient.h"
#include "PxPvdDataStream.h"

#define PX_SIMD_SKINNING 1

#pragma warning( disable: 4101 ) // PX_COMPILE_TIME_ASSERT causes these warnings since they are
// used within our apex namespace

namespace nvidia
{
namespace clothing
{

AuthObjTypeID ClothingAssetImpl::mAssetTypeID = 0xffffffff;

ClothingAssetImpl::ClothingAssetImpl(ModuleClothingImpl* module, ResourceList& list, const char* name) :
	mModule(module),
	mParams(DYNAMIC_CAST(ClothingAssetParameters*)(GetInternalApexSDK()->getParameterizedTraits()->createNvParameterized(ClothingAssetParameters::staticClassName()))),
	mPhysicalMeshes(mParams, "physicalMeshes", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->physicalMeshes)),
	mGraphicalLods(mParams, "graphicalLods", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->graphicalLods)),
	mBones(mParams, "bones", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->bones)),
	mBoneSpheres(mParams, "boneSpheres", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->boneSpheres)),
	mSpherePairs(mParams, "boneSphereConnections", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->boneSphereConnections)),
	mBoneActors(mParams, "boneActors", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->boneActors)),
	mBoneVertices(mParams, "boneVertices", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->boneVertices)),
	mBonePlanes(mParams, "bonePlanes", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->bonePlanes)),
	mCollisionConvexes(mParams, "collisionConvexes", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->collisionConvexes)),
	mName(name),
	mExt2IntMorphMappingMaxValue(0),
	mDirty(false),
	mMorphMappingWarning(false)
{
	// this constructor is only executed when initializing the authoring asset
	list.add(*this);

#ifndef WITHOUT_PVD
	mActors.setupForPvd(static_cast<ApexResourceInterface*>(this), "ClothingActors", "ClothingActor");
#endif

	// make sure these two methods are compiled!
	AbstractMeshDescription pcm;
	pcm.avgEdgeLength = 0.1f;
}


ClothingAssetImpl::ClothingAssetImpl(ModuleClothingImpl* module, ResourceList& list, NvParameterized::Interface* params, const char* name) :
	mModule(module),
	mParams(NULL),
	mName(name),
	mExt2IntMorphMappingMaxValue(0),
	mDirty(false),
	mMorphMappingWarning(false)
{

#ifndef WITHOUT_PVD
	mActors.setupForPvd(static_cast<ApexResourceInterface*>(this), "ClothingActors", "ClothingActor");
#endif

	// wrong name?
	if (params != NULL && ::strcmp(params->className(), ClothingAssetParameters::staticClassName()) != 0)
	{
		APEX_INTERNAL_ERROR(
		    "The parameterized interface is of type <%s> instead of <%s>.  "
		    "This object will be initialized by an empty one instead!",
		    params->className(),
		    ClothingAssetParameters::staticClassName());

		params->destroy();
		params = NULL;
	}
	else if (params != NULL)
	{
		ClothingAssetParameters* checkParams = DYNAMIC_CAST(ClothingAssetParameters*)(params);

		uint32_t boneRefsMesh = 0, boneRefsRB = 0;
		for (int i = 0; i < checkParams->bones.arraySizes[0]; i++)
		{
			boneRefsMesh += checkParams->bones.buf[i].numMeshReferenced;
			boneRefsRB += checkParams->bones.buf[i].numRigidBodiesReferenced;
		}

		if (checkParams->bones.arraySizes[0] > 0 && (boneRefsRB + boneRefsMesh == 0))
		{
			APEX_INTERNAL_ERROR(
			    "This parameterized object has not been prepared before serialization.  "
			    "It will not be able to work and has been replaced by an empty one instead.  "
			    "See NvParameterized::Interface::callPreSerializeCallback()");

			params->destroy();
			params = NULL;
		}
	}

	if (params == NULL)
	{
		params = DYNAMIC_CAST(ClothingAssetParameters*)(GetInternalApexSDK()->getParameterizedTraits()->createNvParameterized(ClothingAssetParameters::staticClassName()));
	}


	PX_ASSERT(nvidia::strcmp(params->className(), ClothingAssetParameters::staticClassName()) == 0);
	if (::strcmp(params->className(), ClothingAssetParameters::staticClassName()) == 0)
	{
		mParams = static_cast<ClothingAssetParameters*>(params);
		mParams->setSerializationCallback(this, NULL);

		bool ok = false;
		PX_UNUSED(ok);

		ok = mPhysicalMeshes.init(mParams, "physicalMeshes", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->physicalMeshes));
		PX_ASSERT(ok);
		ok = mGraphicalLods.init(mParams, "graphicalLods", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->graphicalLods));
		PX_ASSERT(ok);
		ok = mBones.init(mParams, "bones", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->bones));
		PX_ASSERT(ok);
		ok = mBoneSpheres.init(mParams, "boneSpheres", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->boneSpheres));
		PX_ASSERT(ok);
		ok = mSpherePairs.init(mParams, "boneSphereConnections", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->boneSphereConnections));
		PX_ASSERT(ok);
		ok = mBoneActors.init(mParams, "boneActors", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->boneActors));
		PX_ASSERT(ok);
		ok = mBoneVertices.init(mParams, "boneVertices", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->boneVertices));
		PX_ASSERT(ok);
		ok = mBonePlanes.init(mParams, "bonePlanes", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->bonePlanes));
		PX_ASSERT(ok);
		ok = mCollisionConvexes.init(mParams, "collisionConvexes", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->collisionConvexes));
		PX_ASSERT(ok);

		for (uint32_t i = 0; i < mGraphicalLods.size(); i++)
		{
			if (mGraphicalLods[i]->renderMeshAsset == NULL)
				continue;

			char buf[128];
			size_t len = PxMin(mName.len(), 120u);
			buf[0] = 0;
			nvidia::strlcat(buf, len + 1, mName.c_str());
			buf[len] = '_';
			nvidia::snprintf(buf + len + 1, 128 - len - 1, "%d", i);
			Asset* asset = GetApexSDK()->createAsset(mGraphicalLods[i]->renderMeshAsset, buf);
			PX_ASSERT(::strcmp(asset->getObjTypeName(), RENDER_MESH_AUTHORING_TYPE_NAME) == 0);

			RenderMeshAssetIntl* rma = static_cast<RenderMeshAssetIntl*>(asset);
			if (rma->mergeBinormalsIntoTangents())
			{
				mDirty = true;
				APEX_DEBUG_INFO("Performance warning. This asset <%s> has to be re-saved to speed up loading", name);
			}

			mGraphicalLods[i]->renderMeshAssetPointer = rma;
			mDirty |= reorderGraphicsVertices(i, i == 0); // only warn the first time
		}

		bool cookingInvalid = false;

		for (uint32_t i = 0; i < mPhysicalMeshes.size(); i++)
		{
			bool reorderVerts = false;
			if (mPhysicalMeshes[i]->physicalMesh.numMaxDistance0Vertices == 0)
			{
				reorderVerts = true;
			}

			if (reorderVerts)
			{
				ClothingPhysicalMeshImpl* mesh = mModule->createPhysicalMeshInternal(mPhysicalMeshes[i]);

				if (mesh != NULL)
				{
					const bool changed = reorderDeformableVertices(*mesh);

					mesh->release();

					cookingInvalid |= changed;
					mDirty |= changed;
				}
			}
		}

		if (mParams->materialLibrary != NULL && cookingInvalid)
		{
			// So, if we turn on zerostretch also for the new solver, we should make sure the values are set soft enough not to introduce too much ghost forces
			ClothingMaterialLibraryParameters* matLib = static_cast<ClothingMaterialLibraryParameters*>(mParams->materialLibrary);

			for (int32_t i = 0; i < matLib->materials.arraySizes[0]; i++)
			{
				float& limit = matLib->materials.buf[i].hardStretchLimitation;

				if (limit >= 1.0f)
				{
					limit = PxMax(limit, 1.1f); // must be either 0 (disabled) or > 1.1 for stability
				}
			}
		}

		if (mParams->boundingBox.minimum.isZero() && mParams->boundingBox.maximum.isZero())
		{
			updateBoundingBox();
		}

		uint32_t cookNow = 0;
		const char* cookedDataClass = "Embedded";

		ParamArray<ClothingAssetParametersNS::CookedEntry_Type> cookedEntries(mParams, "cookedData", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->cookedData));
		for (uint32_t i = 0; i < cookedEntries.size(); i++)
		{
			if (cookedEntries[i].cookedData != NULL)
			{
				BackendFactory* cookedDataBackend = mModule->getBackendFactory(cookedEntries[i].cookedData->className());
				PX_ASSERT(cookedDataBackend);
				if (cookedDataBackend != NULL)
				{
					// compare data version with current version of the data backend
					uint32_t cookedDataVersion = cookedDataBackend->getCookedDataVersion(cookedEntries[i].cookedData);
					uint32_t cookingVersion = cookedDataBackend->getCookingVersion();

					if (cookingVersion != cookedDataVersion || cookingInvalid
#if PX_PHYSICS_VERSION_MAJOR == 3
					        // don't use native CookingPhysX, only use embedded solver
					        || cookedDataVersion == CookingPhysX::getCookingVersion()
#endif
					   )
					{
						cookNow = cookedDataVersion;
						cookedEntries[i].cookedData->destroy();
						cookedEntries[i].cookedData = NULL;
					}
				}
			}
		}
		if (cookNow != 0)
		{
			APEX_DEBUG_WARNING("Asset (%s) cooked data version (%d/0x%08x) does not match the current sdk version. Recooking.", name, cookNow, cookNow);
		}

		if (cookedEntries.isEmpty())
		{
			ClothingAssetParametersNS::CookedEntry_Type entry;
			entry.scale = 1.0f;
			entry.cookedData = NULL;
			cookedEntries.pushBack(entry);
			APEX_DEBUG_INFO("Asset (%s) has no cooked data and will be re-cooked every time it's loaded, asset needs to be resaved!", name);
			cookNow = 1;
		}

		if (cookNow != 0)
		{
			BackendFactory* cookingBackend = mModule->getBackendFactory(cookedDataClass);

			PX_ASSERT(cookingBackend != NULL);

			for (uint32_t i = 0; i < cookedEntries.size(); i++)
			{
				if (cookedEntries[i].cookedData == NULL && cookingBackend != NULL)
				{
					ClothingAssetParametersNS::CookedEntry_Type& entry = cookedEntries[i];

					CookingAbstract* cookingJob = cookingBackend->createCookingJob();
					prepareCookingJob(*cookingJob, entry.scale, NULL, NULL);

					if (cookingJob->isValid())
					{
						entry.cookedData = cookingJob->execute();
					}
					PX_DELETE_AND_RESET(cookingJob);
				}
			}

			mDirty = true;
		}
	}

	list.add(*this);
}



uint32_t ClothingAssetImpl::forceLoadAssets()
{
	uint32_t assetLoadedCount = 0;

	for (uint32_t i = 0; i < mGraphicalLods.size(); i++)
	{
		if (mGraphicalLods[i]->renderMeshAssetPointer == NULL)
			continue;

		RenderMeshAssetIntl* rma = reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[i]->renderMeshAssetPointer);
		assetLoadedCount += rma->forceLoadAssets();
	}

	return assetLoadedCount;

}



NvParameterized::Interface* ClothingAssetImpl::getDefaultActorDesc()
{
	NvParameterized::Interface* ret = NULL;

	if (mModule != NULL)
	{
		ret = mModule->getApexClothingActorParams();

		if (ret != NULL)
		{
			ret->initDefaults();

			// need to customize it per asset
			PX_ASSERT(nvidia::strcmp(ret->className(), ClothingActorParam::staticClassName()) == 0);
			ClothingActorParam* actorDesc = static_cast<ClothingActorParam*>(ret);
			actorDesc->clothingMaterialIndex = mParams->materialIndex;
		}
	}

	return ret;
}



NvParameterized::Interface* ClothingAssetImpl::getDefaultAssetPreviewDesc()
{
	NvParameterized::Interface* ret = NULL;

	if (mModule != NULL)
	{
		ret = mModule->getApexClothingPreviewParams();

		if (ret != NULL)
		{
			ret->initDefaults();
		}
	}

	return ret;
}



Actor* ClothingAssetImpl::createApexActor(const NvParameterized::Interface& params, Scene& apexScene)
{
	if (!isValidForActorCreation(params, apexScene))
	{
		return NULL;
	}
	ClothingActorProxy* proxy = NULL;
	ClothingScene* clothingScene =	mModule->getClothingScene(apexScene);
	proxy = PX_NEW(ClothingActorProxy)(params, this, *clothingScene, &mActors);
	PX_ASSERT(proxy != NULL);
	return proxy;
}


NvParameterized::Interface* ClothingAssetImpl::releaseAndReturnNvParameterizedInterface()
{
	NvParameterized::Interface* ret = mParams;
	mParams->setSerializationCallback(NULL, NULL);
	mParams = NULL;
	release();
	return ret;
}



bool ClothingAssetImpl::isValidForActorCreation(const NvParameterized::Interface& params, Scene& apexScene) const
{
	bool ret = false;

	if (ClothingActorImpl::isValidDesc(params))
	{
		ClothingScene* clothingScene =	mModule->getClothingScene(apexScene);
		if (clothingScene)
		{
			if (!clothingScene->isSimulating())
			{
				ret = true;
			}
			else
			{
				APEX_INTERNAL_ERROR("Cannot create ClothingActor while simulation is running");
			}
		}
	}
	else
	{
		APEX_INVALID_PARAMETER("ClothingActorDesc is invalid");
	}
	return ret;
}


bool ClothingAssetImpl::isDirty() const
{
	return mDirty;
}



void ClothingAssetImpl::release()
{
	mModule->mSdk->releaseAsset(*this);
}



ClothingActor* ClothingAssetImpl::getActor(uint32_t index)
{
	READ_ZONE();
	if (index < mActors.getSize())
	{
		return DYNAMIC_CAST(ClothingActorProxy*)(mActors.getResource(index));
	}
	return NULL;
}



float ClothingAssetImpl::getMaximumSimulationBudget(uint32_t solverIterations) const
{
	uint32_t maxCost = 0;

	for (uint32_t i = 0; i < mPhysicalMeshes.size(); i++)
	{
		uint32_t iterations = (uint32_t)(mPhysicalMeshes[i]->physicalMesh.numSimulatedVertices * solverIterations);
		maxCost = PxMax(maxCost, iterations); 
	}

	return static_cast<float>(maxCost);
}



uint32_t ClothingAssetImpl::getNumGraphicalLodLevels() const
{
	READ_ZONE();
	return mGraphicalLods.size();
}



uint32_t ClothingAssetImpl::getGraphicalLodValue(uint32_t lodLevel) const
{
	READ_ZONE();
	if (lodLevel < mGraphicalLods.size())
	{
		return mGraphicalLods[lodLevel]->lod;
	}

	return uint32_t(-1);
}



float ClothingAssetImpl::getBiggestMaxDistance() const
{
	READ_ZONE();
	float maxValue = 0.0f;
	for (uint32_t i = 0; i < mPhysicalMeshes.size(); i++)
	{
		maxValue = PxMax(maxValue, mPhysicalMeshes[i]->physicalMesh.maximumMaxDistance);
	}

	return maxValue;
}



bool ClothingAssetImpl::remapBoneIndex(const char* name, uint32_t newIndex)
{
	WRITE_ZONE();
	uint32_t found = 0;
	const uint32_t numBones = mBones.size();
	for (uint32_t i = 0; i < numBones; i++)
	{
		if (mBones[i].name != NULL && (::strcmp(mBones[i].name, name) == 0))
		{
			mBones[i].externalIndex = (int32_t)newIndex;
			found++;
		}
	}

	if (found > 1)
	{
		APEX_DEBUG_WARNING("The asset contains %i bones with name %s. All occurences were mapped.", found, name);
	}
	else if (found == 0)
	{
		APEX_DEBUG_INFO("The asset does not contain a bone with name %s", name);
	}

	return (found == 1); // sanity
}



const char*	ClothingAssetImpl::getBoneName(uint32_t internalIndex) const
{
	READ_ZONE();
	if (internalIndex >= mBones.size())
	{
		return "";
	}

	return mBones[internalIndex].name;
}



bool ClothingAssetImpl::getBoneBasePose(uint32_t internalIndex, PxMat44& result) const
{
	READ_ZONE();
	if (internalIndex < mBones.size())
	{
		result = mBones[internalIndex].bindPose;
		return true;
	}
	return false;
}



void ClothingAssetImpl::getBoneMapping(uint32_t* internal2externalMap) const
{
	READ_ZONE();
	for (uint32_t i = 0; i < mBones.size(); i++)
	{
		internal2externalMap[(uint32_t)mBones[i].internalIndex] = (uint32_t)mBones[i].externalIndex;
	}
}



uint32_t ClothingAssetImpl::prepareMorphTargetMapping(const PxVec3* originalPositions, uint32_t numPositions, float epsilon)
{
	WRITE_ZONE();
	uint32_t numInternalVertices = 0;
	for (uint32_t i = 0; i < mGraphicalLods.size(); i++)
	{
		RenderMeshAssetIntl* rma = getGraphicalMesh(i);
		if (rma == NULL)
			continue;

		for (uint32_t s = 0; s < rma->getSubmeshCount(); s++)
		{
			numInternalVertices += rma->getSubmesh(s).getVertexCount(0);
		}
	}
	numInternalVertices += mBoneVertices.size();

	mExt2IntMorphMapping.resize(numInternalVertices);
	uint32_t indexWritten = 0;

	uint32_t numLarger = 0;
	float epsilon2 = epsilon * epsilon;

	for (uint32_t i = 0; i < mGraphicalLods.size(); i++)
	{
		RenderMeshAssetIntl* rma = getGraphicalMesh(i);
		if (rma == NULL)
			continue;

		for (uint32_t s = 0; s < rma->getSubmeshCount(); s++)
		{
			const uint32_t numVertices = rma->getSubmesh(s).getVertexCount(0);
			const VertexFormat& format = rma->getSubmesh(s).getVertexBuffer().getFormat();
			const uint32_t positionIndex = (uint32_t)format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::POSITION));
			if (format.getBufferFormat(positionIndex) == RenderDataFormat::FLOAT3)
			{
				PxVec3* positions = (PxVec3*)rma->getSubmesh(s).getVertexBuffer().getBuffer(positionIndex);
				for (uint32_t v = 0; v < numVertices; v++)
				{
					float closestDist2 = PX_MAX_F32;
					uint32_t closestIndex = (uint32_t) - 1;
					for (uint32_t iv = 0; iv < numPositions; iv++)
					{
						float dist2 = (originalPositions[iv] - positions[v]).magnitudeSquared();
						if (dist2 < closestDist2)
						{
							closestDist2 = dist2;
							closestIndex = iv;
						}
					}
					PX_ASSERT(closestIndex != (uint32_t) - 1);
					mExt2IntMorphMapping[indexWritten++] = closestIndex;
					numLarger += closestDist2 < epsilon2 ? 0 : 1;
				}
			}
			else
			{
				PX_ALWAYS_ASSERT();
			}
		}
	}

	for (uint32_t i = 0; i < mBoneVertices.size(); i++)
	{
		float closestDist2 = PX_MAX_F32;
		uint32_t closestIndex = (uint32_t) - 1;
		for (uint32_t iv = 0; iv < numPositions; iv++)
		{
			float dist2 = (originalPositions[iv] - mBoneVertices[i]).magnitudeSquared();
			if (dist2 < closestDist2)
			{
				closestDist2 = dist2;
				closestIndex = iv;
			}
		}
		PX_ASSERT(closestIndex != (uint32_t) - 1);
		mExt2IntMorphMapping[indexWritten++] = closestIndex;
		numLarger += closestDist2 < epsilon2 ? 0 : 1;
	}

	PX_ASSERT(indexWritten == numInternalVertices);
	mExt2IntMorphMappingMaxValue = numPositions;

	return numLarger;
}



void ClothingAssetImpl::preSerialize(void* /*userData*/)
{
	mDirty = false;
}



RenderMeshAssetIntl* ClothingAssetImpl::getGraphicalMesh(uint32_t index)
{
	if (index < mGraphicalLods.size())
	{
		return reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[index]->renderMeshAssetPointer);
	}

	return NULL;
}



const ClothingGraphicalLodParameters* ClothingAssetImpl::getGraphicalLod(uint32_t index) const
{
	if (index < mGraphicalLods.size())
	{
		return mGraphicalLods[index];
	}

	return NULL;
}


void ClothingAssetImpl::releaseClothingActor(ClothingActor& actor)
{
	ClothingActorProxy* proxy = DYNAMIC_CAST(ClothingActorProxy*)(&actor);
	proxy->destroy();
}



void ClothingAssetImpl::releaseClothingPreview(ClothingPreview& preview)
{
	ClothingPreviewProxy* proxy = DYNAMIC_CAST(ClothingPreviewProxy*)(&preview);
	proxy->destroy();
}



bool ClothingAssetImpl::writeBoneMatrices(PxMat44 localPose, const PxMat44* newBoneMatrices, const uint32_t byteStride,
                                      const uint32_t numBones, PxMat44* dest, bool isInternalOrder, bool multInvBindPose)
{
	PX_PROFILE_ZONE("ClothingAssetImpl::writeBoneMatrices", GetInternalApexSDK()->getContextId());

	PX_ASSERT(byteStride >= sizeof(PxMat44));

	bool changed = false;

	if (mBones.isEmpty())
	{
		APEX_INTERNAL_ERROR("bone map is empty, this is a error condition");
	}
	else
	{
		// PH: if bones are present, but not set, we just have to write them with global pose
		const uint8_t* src = (const uint8_t*)newBoneMatrices;
		const uint32_t numBonesReferenced = mParams->bonesReferenced;
		for (uint32_t i = 0; i < mBones.size(); i++)
		{
			PX_ASSERT(mBones[i].internalIndex < (int32_t)mBones.size());
			PX_ASSERT(isInternalOrder || mBones[i].externalIndex >= 0);
			const uint32_t internalIndex = i;
			const uint32_t externalIndex = isInternalOrder ? i : mBones[i].externalIndex;
			if (internalIndex < numBonesReferenced && mBones[i].internalIndex >= 0)
			{
				PxMat44& oldMat = dest[internalIndex];
				PX_ALIGN(16, PxMat44) newMat;

				if (src != NULL && externalIndex < numBones)
				{
					PxMat44 skinningTransform = *(const PxMat44*)(src + byteStride * externalIndex);
					if (multInvBindPose)
					{
						skinningTransform = skinningTransform * mInvBindPoses[internalIndex];
					}
					newMat = localPose * skinningTransform;
				}
				else
				{
					newMat = localPose;
				}

				if (newMat != oldMat) // PH: let's hope this comparison is not too slow
				{
					changed |= (i < numBonesReferenced);
					oldMat = newMat;
				}
			}
		}
	}
	return changed;
}



ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* ClothingAssetImpl::getPhysicalMeshFromLod(uint32_t graphicalLodId) const
{
	const uint32_t physicalMeshId = mGraphicalLods[graphicalLodId]->physicalMeshId;

	// -1 is also bigger than mPhysicalMeshes.size() for an unsigned
	if (physicalMeshId >= mPhysicalMeshes.size())
	{
		return NULL;
	}

	return &mPhysicalMeshes[physicalMeshId]->physicalMesh;
}


ClothingPhysicalMeshParametersNS::SkinClothMapB_Type* ClothingAssetImpl::getTransitionMapB(uint32_t dstPhysicalMeshId, uint32_t srcPhysicalMeshId, float& thickness, float& offset)
{
	if (srcPhysicalMeshId + 1 == dstPhysicalMeshId)
	{
		thickness = mPhysicalMeshes[dstPhysicalMeshId]->transitionDownThickness;
		offset = mPhysicalMeshes[dstPhysicalMeshId]->transitionDownOffset;
		return mPhysicalMeshes[dstPhysicalMeshId]->transitionDownB.buf;
	}
	else if (srcPhysicalMeshId == dstPhysicalMeshId + 1)
	{
		thickness = mPhysicalMeshes[dstPhysicalMeshId]->transitionUpThickness;
		offset = mPhysicalMeshes[dstPhysicalMeshId]->transitionUpOffset;
		return mPhysicalMeshes[dstPhysicalMeshId]->transitionUpB.buf;
	}

	thickness = 0.0f;
	offset = 0.0f;
	return NULL;
}



ClothingPhysicalMeshParametersNS::SkinClothMapD_Type* ClothingAssetImpl::getTransitionMap(uint32_t dstPhysicalMeshId, uint32_t srcPhysicalMeshId, float& thickness, float& offset)
{
	if (srcPhysicalMeshId + 1 == dstPhysicalMeshId)
	{
		thickness = mPhysicalMeshes[dstPhysicalMeshId]->transitionDownThickness;
		offset = mPhysicalMeshes[dstPhysicalMeshId]->transitionDownOffset;
		return mPhysicalMeshes[dstPhysicalMeshId]->transitionDown.buf;
	}
	else if (srcPhysicalMeshId == dstPhysicalMeshId + 1)
	{
		thickness = mPhysicalMeshes[dstPhysicalMeshId]->transitionUpThickness;
		offset = mPhysicalMeshes[dstPhysicalMeshId]->transitionUpOffset;
		return mPhysicalMeshes[dstPhysicalMeshId]->transitionUp.buf;
	}

	thickness = 0.0f;
	offset = 0.0f;
	return NULL;
}



NvParameterized::Interface* ClothingAssetImpl::getCookedData(float actorScale)
{
	NvParameterized::Interface* closest = NULL;
	float closestDiff = PX_MAX_F32;

	for (int32_t i = 0; i < mParams->cookedData.arraySizes[0]; i++)
	{
		NvParameterized::Interface* cookedData = mParams->cookedData.buf[i].cookedData;
		const float cookedDataScale = mParams->cookedData.buf[i].scale;

		if (cookedData == NULL)
		{
			continue;
		}

#ifdef _DEBUG
		if (::strcmp(cookedData->className(), ClothingCookedParam::staticClassName()) == 0)
		{
			// silly debug verification
			PX_ASSERT(cookedDataScale == ((ClothingCookedParam*)cookedData)->actorScale);
		}
#endif

		const float scaleDiff = PxAbs(actorScale - cookedDataScale);
		if (scaleDiff < closestDiff)
		{
			closest = cookedData;
			closestDiff = scaleDiff;
		}
	}

	if (closest != NULL && closestDiff < 0.01f)
	{
		return closest;
	}

	return NULL;
}



uint32_t ClothingAssetImpl::getCookedPhysXVersion() const
{
	uint32_t version = 0;

	for (int32_t i = 0; i < mParams->cookedData.arraySizes[0]; i++)
	{
		const NvParameterized::Interface* cookedData = mParams->cookedData.buf[i].cookedData;
		if (cookedData != NULL)
		{
			const char* className = cookedData->className();
			BackendFactory* factory = mModule->getBackendFactory(className);

			uint32_t v = factory->getCookedDataVersion(cookedData);
			PX_ASSERT(version == 0 || version == v);
			version = v;
		}
	}

	// version == 0 usually means that there is no maxdistance > 0 at all!
	//PX_ASSERT(version != 0);
	return version;
}



ClothSolverMode::Enum ClothingAssetImpl::getClothSolverMode() const
{
	return ClothSolverMode::v3;
}



SimulationAbstract* ClothingAssetImpl::getSimulation(uint32_t physicalMeshId, NvParameterized::Interface* cookedParam, ClothingScene* clothingScene)
{
	SimulationAbstract* result = NULL;

	mUnusedSimulationMutex.lock();

	for (uint32_t i = 0; i < mUnusedSimulation.size(); i++)
	{
		PX_ASSERT(mUnusedSimulation[i] != NULL);
		if (mUnusedSimulation[i]->physicalMeshId != physicalMeshId)
		{
			continue;
		}

		if (mUnusedSimulation[i]->getCookedData() != cookedParam)
		{
			continue;
		}

		if (mUnusedSimulation[i]->getClothingScene() != clothingScene)
		{
			continue;
		}

		// we found one
		result = mUnusedSimulation[i];
		for (uint32_t j = i + 1; j < mUnusedSimulation.size(); j++)
		{
			mUnusedSimulation[j - 1] = mUnusedSimulation[j];
		}

		mUnusedSimulation.popBack();
		break;
	}

	mUnusedSimulationMutex.unlock();

	return result;
}



void ClothingAssetImpl::returnSimulation(SimulationAbstract* simulation)
{
	PX_ASSERT(simulation != NULL);

	bool isAssetParam = false;
	for (int32_t i = 0; i < mParams->cookedData.arraySizes[0]; i++)
	{
		isAssetParam |= mParams->cookedData.buf[i].cookedData == simulation->getCookedData();
	}

	nvidia::Mutex::ScopedLock scopeLock(mUnusedSimulationMutex);

	if (mModule->getMaxUnusedPhysXResources() == 0 || !isAssetParam || !simulation->needsExpensiveCreation())
	{
		destroySimulation(simulation);
		return;
	}

	if (mUnusedSimulation.size() > mModule->getMaxUnusedPhysXResources())
	{
		destroySimulation(mUnusedSimulation[0]);

		for (uint32_t i = 1; i < mUnusedSimulation.size(); i++)
		{
			mUnusedSimulation[i - 1] = mUnusedSimulation[i];
		}

		mUnusedSimulation[mUnusedSimulation.size() - 1] = simulation;
	}
	else
	{
		mUnusedSimulation.pushBack(simulation);
	}

	simulation->disablePhysX(mModule->getDummyActor());
}



void ClothingAssetImpl::destroySimulation(SimulationAbstract* simulation)
{
	PX_ASSERT(simulation != NULL);

	simulation->unregisterPhysX();
	PX_DELETE_AND_RESET(simulation);
}



void ClothingAssetImpl::initCollision(SimulationAbstract* simulation, const PxMat44* boneTansformations,
								  ResourceList& actorPlanes,
								  ResourceList& actorConvexes,
								  ResourceList& actorSpheres,
								  ResourceList& actorCapsules,
								  ResourceList& actorTriangleMeshes,
								  const ClothingActorParam* actorParam,
								  const PxMat44& globalPose, bool localSpaceSim)
{
	simulation->initCollision(	mBoneActors.begin(), mBoneActors.size(), mBoneSpheres.begin(), mBoneSpheres.size(), mSpherePairs.begin(), mSpherePairs.size(), mBonePlanes.begin(), mBonePlanes.size(), mCollisionConvexes.begin(), mCollisionConvexes.size(), mBones.begin(), boneTansformations,
								actorPlanes, actorConvexes, actorSpheres, actorCapsules, actorTriangleMeshes, 
								actorParam->actorDescTemplate, actorParam->shapeDescTemplate, actorParam->actorScale,
								globalPose, localSpaceSim);
}



void ClothingAssetImpl::updateCollision(SimulationAbstract* simulation, const PxMat44* boneTansformations,
									ResourceList& actorPlanes,
									ResourceList& actorConvexes,
									ResourceList& actorSpheres,
									ResourceList& actorCapsules,
									ResourceList& actorTriangleMeshes,
									bool teleport)
{
	simulation->updateCollision(mBoneActors.begin(), mBoneActors.size(), mBoneSpheres.begin(), mBoneSpheres.size(), mBonePlanes.begin(), mBonePlanes.size(), mBones.begin(), boneTansformations,
								actorPlanes, actorConvexes, actorSpheres, actorCapsules, actorTriangleMeshes, teleport);
}



uint32_t ClothingAssetImpl::getPhysicalMeshID(uint32_t graphicalLodId) const
{
	if (graphicalLodId >= mGraphicalLods.size())
	{
		return (uint32_t) - 1;
	}

	return mGraphicalLods[graphicalLodId]->physicalMeshId;
}



void ClothingAssetImpl::visualizeSkinCloth(RenderDebugInterface& renderDebug, AbstractMeshDescription& srcPM, bool showTets, float actorScale) const
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(renderDebug);
	PX_UNUSED(srcPM);
	PX_UNUSED(showTets);
	PX_UNUSED(actorScale);
#else
	using RENDER_DEBUG::DebugColors;
	using RENDER_DEBUG::DebugRenderState;

	if ((srcPM.pPosition != NULL) && (srcPM.pIndices != NULL) && (srcPM.pNormal != NULL))
	{
		RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();

		RENDER_DEBUG_IFACE(&renderDebug)->removeFromCurrentState(DebugRenderState::SolidShaded);

		const uint32_t colorWhite = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::White);
		const uint32_t colorRed = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Red);
		const uint32_t colorDarkRed = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::DarkRed);
		const uint32_t colorGreen = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Green);
		const uint32_t colorPurple = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Purple);

		const float meshThickness = srcPM.avgEdgeLength * actorScale;
		// for each triangle in the Physical Mesh draw lines delimiting all the tetras
		for (uint32_t i = 0; i < srcPM.numIndices; i += 3)
		{
			if (showTets)
			{
				PxVec3 vtx[3], nrm[3];
				getNormalsAndVerticesForFace(vtx, nrm, i, srcPM);

				// draw lines for all edges of each tetrahedron (sure, there are redundant lines, but this is for debugging purposes)
				for (uint32_t tIdx = 0; tIdx < TETRA_LUT_SIZE; tIdx++)
				{
					// compute the tetra vertices based on the index
					const TetraEncoding& tetEnc = tetraTable[tIdx];
					PxVec3 tv0 = vtx[0] + (tetEnc.sign[0] * nrm[0] * meshThickness);
					PxVec3 tv1 = vtx[1] + (tetEnc.sign[1] * nrm[1] * meshThickness);
					PxVec3 tv2 = vtx[2] + (tetEnc.sign[2] * nrm[2] * meshThickness);
					PxVec3 tv3 = vtx[tetEnc.lastVtxIdx] + (tetEnc.sign[3] * nrm[tetEnc.lastVtxIdx] * meshThickness);

					uint32_t color  = tIdx < 3 ? colorGreen : colorPurple;

					RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(color);
					RENDER_DEBUG_IFACE(&renderDebug)->debugLine(tv1, tv2);
					RENDER_DEBUG_IFACE(&renderDebug)->debugLine(tv2, tv3);
					RENDER_DEBUG_IFACE(&renderDebug)->debugLine(tv3, tv1);
					RENDER_DEBUG_IFACE(&renderDebug)->debugLine(tv0, tv1);
					RENDER_DEBUG_IFACE(&renderDebug)->debugLine(tv0, tv2);
					RENDER_DEBUG_IFACE(&renderDebug)->debugLine(tv0, tv3);
				}
			}
			else
			{
				uint32_t idx[3] =
				{
					srcPM.pIndices[i + 0],
					srcPM.pIndices[i + 1],
					srcPM.pIndices[i + 2],
				};

				PxVec3 vtx[3], nrm[3];
				for (uint32_t u = 0; u < 3; u++)
				{
					vtx[u] = srcPM.pPosition[idx[u]];
					nrm[u] = srcPM.pNormal[idx[u]] * meshThickness;
				}

				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorWhite);
				RENDER_DEBUG_IFACE(&renderDebug)->debugTri(vtx[0], vtx[1], vtx[2]);

				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorGreen);
				RENDER_DEBUG_IFACE(&renderDebug)->debugTri(vtx[0] + nrm[0], vtx[1] + nrm[1], vtx[2] + nrm[2]);
				for (uint32_t u = 0; u < 3; u++)
				{
					RENDER_DEBUG_IFACE(&renderDebug)->debugLine(vtx[u], vtx[u] + nrm[u]);
				}

				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorPurple);
				RENDER_DEBUG_IFACE(&renderDebug)->debugTri(vtx[0] - nrm[0], vtx[1] - nrm[1], vtx[2] - nrm[2]);
				for (uint32_t u = 0; u < 3; u++)
				{
					RENDER_DEBUG_IFACE(&renderDebug)->debugLine(vtx[u], vtx[u] - nrm[u]);
				}
			}
		}

#if 1
		// display some features of the physical mesh as it's updated at runtime

		srcPM.UpdateDerivedInformation(&renderDebug);

		// draw the mesh's bounding box
		PxBounds3 bounds(srcPM.pMin, srcPM.pMax);
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorRed);
		RENDER_DEBUG_IFACE(&renderDebug)->debugBound(bounds);

		// draw line from the centroid to the top-most corner of the AABB
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorDarkRed);
		RENDER_DEBUG_IFACE(&renderDebug)->debugLine(srcPM.centroid, srcPM.pMax);

		// draw white line along the same direction (but negated) to display the avgEdgeLength
		PxVec3 dirToPMax = (srcPM.pMax - srcPM.centroid);
		dirToPMax.normalize();
		PxVec3 pEdgeLengthAway = srcPM.centroid - (dirToPMax * srcPM.avgEdgeLength);
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorWhite);
		RENDER_DEBUG_IFACE(&renderDebug)->debugLine(srcPM.centroid, pEdgeLengthAway);

		// draw green line along the same direction to display the physical mesh thickness
		PxVec3 pPmThicknessAway = srcPM.centroid + (dirToPMax * meshThickness);
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorGreen);
		RENDER_DEBUG_IFACE(&renderDebug)->debugLine(srcPM.centroid, pPmThicknessAway);
#endif

		RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
	}
#endif
}



void ClothingAssetImpl::visualizeSkinClothMap(RenderDebugInterface& renderDebug, AbstractMeshDescription& srcPM,
		SkinClothMapB* skinClothMapB, uint32_t skinClothMapBSize,
		SkinClothMap* skinClothMap, uint32_t skinClothMapSize,
		float actorScale, bool onlyBad, bool invalidBary) const
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(renderDebug);
	PX_UNUSED(srcPM);
	PX_UNUSED(skinClothMapB);
	PX_UNUSED(skinClothMapBSize);
	PX_UNUSED(skinClothMap);
	PX_UNUSED(skinClothMapSize);
	PX_UNUSED(actorScale);
	PX_UNUSED(onlyBad);
	PX_UNUSED(invalidBary);
#else
	using RENDER_DEBUG::DebugColors;
	using RENDER_DEBUG::DebugRenderState;

	RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();

	RENDER_DEBUG_IFACE(&renderDebug)->removeFromCurrentState(DebugRenderState::SolidShaded);
	const float meshThickness = srcPM.avgEdgeLength * actorScale;

	if ((skinClothMapB != NULL) && (srcPM.pPosition != NULL) && (srcPM.pIndices != NULL) && (srcPM.pNormal != NULL))
	{
		const uint32_t colorRed = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Red);
		const uint32_t colorDarkRed = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::DarkRed);
		const uint32_t colorBlue = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Blue);
		const uint32_t colorYellow = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Yellow);
		const uint32_t colorGreen = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Green);
		const uint32_t colorPurple = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Purple);

		for (uint32_t i = 0; i < skinClothMapBSize; i++)
		{
			const SkinClothMapB& mapping = skinClothMapB[i];

			if (mapping.faceIndex0 >= srcPM.numIndices)
			{
				break;
			}

			const PxVec3 vtb = mapping.vtxTetraBary;
			const float vtb_w = 1.0f - vtb.x - vtb.y - vtb.z;
			const bool badVtx =
			    vtb.x < 0.0f || vtb.x > 1.0f ||
			    vtb.y < 0.0f || vtb.y > 1.0f ||
			    vtb.z < 0.0f || vtb.z > 1.0f ||
			    vtb_w < 0.0f || vtb_w > 1.0f;

			const PxVec3 ntb = mapping.nrmTetraBary;
			const float ntb_w = 1.0f - ntb.x - ntb.y - ntb.z;

			const bool badNrm =
			    ntb.x < 0.0f || ntb.x > 1.0f ||
			    ntb.y < 0.0f || ntb.y > 1.0f ||
			    ntb.z < 0.0f || ntb.z > 1.0f ||
			    ntb_w < 0.0f || ntb_w > 1.0f;

			if (!onlyBad || badVtx || badNrm)
			{
				PxVec3 vtx[3], nrm[3];
				getNormalsAndVerticesForFace(vtx, nrm, mapping.faceIndex0, srcPM);

				const TetraEncoding& tetEnc = tetraTable[mapping.tetraIndex];

				const PxVec3 tv0 = vtx[0] + (tetEnc.sign[0] * nrm[0] * meshThickness);
				const PxVec3 tv1 = vtx[1] + (tetEnc.sign[1] * nrm[1] * meshThickness);
				const PxVec3 tv2 = vtx[2] + (tetEnc.sign[2] * nrm[2] * meshThickness);
				const PxVec3 tv3 = vtx[tetEnc.lastVtxIdx] + (tetEnc.sign[3] * nrm[tetEnc.lastVtxIdx] * meshThickness);

				const PxVec3 centroid = (tv0 + tv1 + tv2 + tv3) * 0.25f;
				const PxVec3 graphicsPos = (vtb.x * tv0) + (vtb.y * tv1) + (vtb.z * tv2) + (vtb_w * tv3);
				const PxVec3 graphicsNrm = (ntb.x * tv0) + (ntb.y * tv1) + (ntb.z * tv2) + (ntb_w * tv3);

				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorYellow);
				RENDER_DEBUG_IFACE(&renderDebug)->debugPoint(graphicsPos, meshThickness * 0.1f);
				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorBlue);
				RENDER_DEBUG_IFACE(&renderDebug)->debugLine(graphicsPos, graphicsNrm);

				if (badVtx && onlyBad)
				{
					RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorRed);
					RENDER_DEBUG_IFACE(&renderDebug)->debugLine(centroid, graphicsPos);
				}

				if (badNrm && onlyBad)
				{
					RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorDarkRed);
					RENDER_DEBUG_IFACE(&renderDebug)->debugLine(centroid, graphicsPos);
				}

				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(mapping.tetraIndex < 3 ? colorGreen : colorPurple);
				RENDER_DEBUG_IFACE(&renderDebug)->debugLine(tv1, tv2);
				RENDER_DEBUG_IFACE(&renderDebug)->debugLine(tv2, tv3);
				RENDER_DEBUG_IFACE(&renderDebug)->debugLine(tv3, tv1);
				RENDER_DEBUG_IFACE(&renderDebug)->debugLine(tv0, tv1);
				RENDER_DEBUG_IFACE(&renderDebug)->debugLine(tv0, tv2);
				RENDER_DEBUG_IFACE(&renderDebug)->debugLine(tv0, tv3);
			}
		}
	}
	else if ((skinClothMap != NULL) && (srcPM.pPosition != NULL) && (srcPM.pIndices != NULL) && (srcPM.pNormal != NULL))
	{
		const uint32_t colorWhite = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::White);
		const uint32_t colorRed = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Red);
		const uint32_t colorDarkRed = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::DarkRed);
		const uint32_t colorBlue = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Blue);
		const uint32_t colorYellow = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Yellow);
		const uint32_t colorGreen = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Green);
		const uint32_t colorPurple = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Purple);

		for (uint32_t i = 0; i < skinClothMapSize; i++)
		{
			const SkinClothMap& mapping = skinClothMap[i];

			if (mapping.vertexIndex0 >= srcPM.numVertices || mapping.vertexIndex1 >= srcPM.numVertices || mapping.vertexIndex2 >= srcPM.numVertices)
			{
				continue;
			}

			PxVec3 baryVtx = mapping.vertexBary;
			const float heightVtx = baryVtx.z;
			baryVtx.z = 1.0f - baryVtx.x - baryVtx.y;

			PxVec3 baryNrm = mapping.normalBary;
			const float heightNrm = baryNrm.z;
			baryNrm.z = 1.0f - baryNrm.x - baryNrm.y;

			bool badVtx =
				baryVtx.x <  0.0f || baryVtx.x > 1.0f ||
				baryVtx.y <  0.0f || baryVtx.y > 1.0f ||
				baryVtx.z <  0.0f || baryVtx.z > 1.0f ||
				heightVtx < -1.0f || heightVtx > 1.0f;

			bool badNrm =
				baryNrm.x < 0.0f  || baryNrm.x > 1.0f ||
				baryNrm.y < 0.0f  || baryNrm.y > 1.0f ||
				baryNrm.z < 0.0f  || baryNrm.z > 1.0f ||
				heightNrm < -1.0f || heightNrm > 1.0f;

			if (!onlyBad || badVtx || badNrm)
			{
				uint32_t idx[3] =
				{
					mapping.vertexIndex0,
					mapping.vertexIndex1,
					mapping.vertexIndex2,
				};

				PxVec3 vtx[3], nrm[3];
				PxVec3 centroid(0.0f);
				for (uint32_t j = 0; j < 3; j++)
				{
					vtx[j] = srcPM.pPosition[idx[j]];
					nrm[j] = srcPM.pNormal[idx[j]] * meshThickness;
					centroid += vtx[j];
				}
				centroid /= 3.0f;


				uint32_t b = (uint32_t)(255 * i / skinClothMapSize);
				uint32_t color = (b << 16) + (b << 8) + b;
				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(color);

				PxVec3 vertexBary = mapping.vertexBary;
				float vertexHeight = vertexBary.z;
				vertexBary.z = 1.0f - vertexBary.x - vertexBary.y;
				const PxVec3 vertexPos = vertexBary.x * vtx[0] + vertexBary.y * vtx[1] + vertexBary.z * vtx[2];
				const PxVec3 vertexNrm = vertexBary.x * nrm[0] + vertexBary.y * nrm[1] + vertexBary.z * nrm[2]; // meshThickness is already in
				const PxVec3 graphicsPos = vertexPos + vertexNrm * vertexHeight;

				if (invalidBary)
				{
					uint32_t invalidColor = 0;
					invalidColor = vertexBary == PxVec3(PX_MAX_F32) ? colorRed : invalidColor;
					invalidColor = mapping.normalBary == PxVec3(PX_MAX_F32) ? colorPurple : invalidColor;
					invalidColor = mapping.tangentBary == PxVec3(PX_MAX_F32) ? colorBlue : invalidColor;

					if (invalidColor != 0)
					{
						RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(invalidColor);
						RENDER_DEBUG_IFACE(&renderDebug)->debugTri(vtx[0], vtx[1], vtx[2]);
						RENDER_DEBUG_IFACE(&renderDebug)->debugPoint(graphicsPos, meshThickness * 0.1f);
						RENDER_DEBUG_IFACE(&renderDebug)->debugLine(centroid, graphicsPos);
					}
					continue;
				}

				RENDER_DEBUG_IFACE(&renderDebug)->debugLine(centroid, graphicsPos);

				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorYellow);
				RENDER_DEBUG_IFACE(&renderDebug)->debugPoint(graphicsPos, meshThickness * 0.1f);

				if (badVtx && onlyBad)
				{
					// draw the projected position as well
					RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorRed);
					RENDER_DEBUG_IFACE(&renderDebug)->debugLine(vertexPos, graphicsPos);
					RENDER_DEBUG_IFACE(&renderDebug)->debugLine(vertexPos, centroid);
				}

				PxVec3 normalBary = mapping.normalBary;
				float normalHeight = normalBary.z;
				normalBary.z = 1.0f - normalBary.x - normalBary.y;
				const PxVec3 normalPos = normalBary.x * vtx[0] + normalBary.y * vtx[1] + normalBary.z * vtx[2];
				const PxVec3 normalNrm = normalBary.x * nrm[0] + normalBary.y * nrm[1] + normalBary.z * nrm[2]; // meshThickness is already in
				const PxVec3 graphicsNrm = normalPos + normalNrm * normalHeight;
				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorBlue);
				RENDER_DEBUG_IFACE(&renderDebug)->debugLine(graphicsNrm, graphicsPos);
#if 0
				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Black));
				RENDER_DEBUG_IFACE(&renderDebug)->debugLine(graphicsNrm, centroid);
#endif

				if (badNrm && onlyBad)
				{
					// draw the projected normal as well
					RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorDarkRed);
					RENDER_DEBUG_IFACE(&renderDebug)->debugLine(normalPos, graphicsNrm);
					RENDER_DEBUG_IFACE(&renderDebug)->debugLine(normalPos, centroid);
				}

				// turn the rendering on for the rest
				badVtx = badNrm = true;

				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorWhite);
				RENDER_DEBUG_IFACE(&renderDebug)->debugTri(vtx[0], vtx[1], vtx[2]);
				if ((badVtx && heightVtx > 0.0f) || (badNrm && heightNrm > 0.0f))
				{
					RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorGreen);
					RENDER_DEBUG_IFACE(&renderDebug)->debugTri(vtx[0] + nrm[0], vtx[1] + nrm[1], vtx[2] + nrm[2]);
					for (uint32_t u = 0; u < 3; u++)
					{
						RENDER_DEBUG_IFACE(&renderDebug)->debugLine(vtx[u], vtx[u] + nrm[u]);
					}
				}
				else if ((badVtx && heightVtx < 0.0f) || (badNrm && heightNrm < 0.0f))
				{
					RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorPurple);
					RENDER_DEBUG_IFACE(&renderDebug)->debugTri(vtx[0] - nrm[0], vtx[1] - nrm[1], vtx[2] - nrm[2]);
					for (uint32_t u = 0; u < 3; u++)
					{
						RENDER_DEBUG_IFACE(&renderDebug)->debugLine(vtx[u], vtx[u] - nrm[u]);
					}
				}
			}
		}
	}

	RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
#endif
}



void ClothingAssetImpl::visualizeBones(RenderDebugInterface& renderDebug, const PxMat44* matrices, bool skeleton, float boneFramesScale, float boneNamesScale)
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(renderDebug);
	PX_UNUSED(matrices);
	PX_UNUSED(skeleton);
	PX_UNUSED(boneFramesScale);
	PX_UNUSED(boneNamesScale);
#else

	using RENDER_DEBUG::DebugColors;
	using RENDER_DEBUG::DebugRenderState;

	RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();

	const uint32_t activeBoneColor = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Purple);
	const uint32_t passiveBoneColor = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Blue);

	const uint32_t colorWhite = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::White);
	const uint32_t colorRed = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Red);
	const uint32_t colorGreen = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Green);
	const uint32_t colorBlue = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Blue);

	RENDER_DEBUG_IFACE(&renderDebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::CenterText);
	RENDER_DEBUG_IFACE(&renderDebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::CameraFacing);
	RENDER_DEBUG_IFACE(&renderDebug)->setCurrentTextScale(boneNamesScale);

	if ((skeleton || boneFramesScale > 0.0f || boneNamesScale > 0.0f) && mPhysicalMeshes.size() > 0)
	{
		ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh = mPhysicalMeshes[0]->physicalMesh;
		float sphereSize = 0.3f * physicalMesh.averageEdgeLength;
		uint32_t rootIdx = mParams->rootBoneIndex;
		PxMat44 absPose = matrices[rootIdx] * mBones[rootIdx].bindPose;
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::DarkRed));
		RENDER_DEBUG_IFACE(&renderDebug)->debugSphere(absPose.getPosition(), sphereSize);
	}

	for (uint32_t i = 0; i < getNumUsedBones(); i++)
	{
		const int32_t parent = mBones[i].parentIndex;

		PxMat44 absPose = matrices[i] * mBones[i].bindPose;

		if (skeleton && parent >= 0 && parent < (int32_t)getNumUsedBones())
		{
			PX_ASSERT((uint32_t)parent != i);
			PxMat44 absPoseParent = matrices[(uint32_t)parent] * mBones[(uint32_t)parent].bindPose;
			if ((mBones[(uint32_t)parent].numMeshReferenced + mBones[(uint32_t)parent].numRigidBodiesReferenced) == 0)
			{
				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(passiveBoneColor);
			}
			else
			{
				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(activeBoneColor);
			}
			RENDER_DEBUG_IFACE(&renderDebug)->debugLine(absPose.getPosition(), absPoseParent.getPosition());
		}

		if (boneFramesScale > 0.0f)
		{
			RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorRed);
			RENDER_DEBUG_IFACE(&renderDebug)->debugLine(absPose.getPosition(), absPose.getPosition() + absPose.column0.getXYZ() * boneFramesScale);
			RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorGreen);
			RENDER_DEBUG_IFACE(&renderDebug)->debugLine(absPose.getPosition(), absPose.getPosition() + absPose.column1.getXYZ() * boneFramesScale);
			RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorBlue);
			RENDER_DEBUG_IFACE(&renderDebug)->debugLine(absPose.getPosition(), absPose.getPosition() + absPose.column2.getXYZ() * boneFramesScale);
		}

		if (boneNamesScale > 0.0f)
		{
			RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorWhite);
			RENDER_DEBUG_IFACE(&renderDebug)->debugText(absPose.getPosition(), mBones[i].name.buf);
		}
	}

	RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
#endif
}



uint32_t ClothingAssetImpl::initializeAssetData(ClothingAssetData& assetData, const uint32_t uvChannel)
{
	//OK. Stage 1 - need to calculate the sizes required...

	const uint32_t numLods = mGraphicalLods.size();
	uint32_t numSubMeshes = 0;

	for (uint32_t a = 0; a < numLods; ++a)
	{
		ClothingGraphicalMeshAssetWrapper meshAsset(reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[a]->renderMeshAssetPointer));
		numSubMeshes += meshAsset.getSubmeshCount();
	}

	const uint32_t numPhysicalMeshes = mPhysicalMeshes.size();

	const uint32_t requiredSize = ((sizeof(ClothingMeshAssetData) * numLods + sizeof(ClothingAssetSubMesh) * numSubMeshes
		                            + sizeof(ClothingPhysicalMeshData) * numPhysicalMeshes) + 15) & 0xfffffff0;

	void* data = PX_ALLOC(requiredSize, PX_DEBUG_EXP("ClothingAssetData"));
	memset(data, 0, requiredSize);

	assetData.mData = (uint8_t*)data;
	assetData.mAssetSize = requiredSize;

	//assetData.m_pLods = (ClothingMeshAssetData*)data;

	assetData.mGraphicalLodsCount = numLods;

	assetData.mRootBoneIndex = mParams->rootBoneIndex;

	ClothingAssetSubMesh* pMeshes = (ClothingAssetSubMesh*)(((uint8_t*)data) + sizeof(ClothingMeshAssetData) * numLods);

	uint32_t maxVertices = 0;

	for (uint32_t a = 0; a < numLods; ++a)
	{
		ClothingGraphicalMeshAssetWrapper meshAsset(reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[a]->renderMeshAssetPointer));

		const ClothingGraphicalLodParameters* graphicalLod = mGraphicalLods[a];

		const uint32_t subMeshCount = meshAsset.getSubmeshCount();
		ClothingMeshAssetData* pAsset = assetData.GetLod(a);
		PX_PLACEMENT_NEW(pAsset, ClothingMeshAssetData());
		//pAsset->m_pMeshes = pMeshes;
		pAsset->mSubMeshCount = subMeshCount;

		//Params to set
		pAsset->mImmediateClothMap = graphicalLod->immediateClothMap.buf;
		pAsset->mImmediateClothMapCount = (uint32_t)graphicalLod->immediateClothMap.arraySizes[0];


		pAsset->mSkinClothMap = graphicalLod->skinClothMap.buf;
		pAsset->mSkinClothMapCount = (uint32_t)graphicalLod->skinClothMap.arraySizes[0];

		pAsset->mSkinClothMapB = graphicalLod->skinClothMapB.buf;
		pAsset->mSkinClothMapBCount = (uint32_t)graphicalLod->skinClothMapB.arraySizes[0];

		pAsset->mTetraMap = graphicalLod->tetraMap.buf;
		pAsset->mTetraMapCount = (uint32_t)graphicalLod->tetraMap.arraySizes[0];

		pAsset->mPhysicalMeshId = graphicalLod->physicalMeshId;

		pAsset->mSkinClothMapThickness = graphicalLod->skinClothMapThickness;
		pAsset->mSkinClothMapOffset = graphicalLod->skinClothMapOffset;

		pAsset->mBounds = mParams->boundingBox;

		//For now - we'll set this outside again
		pAsset->bActive = true;

		pAsset->mSubmeshOffset = (uint32_t)((uint8_t*)pMeshes - assetData.mData);

		for (uint32_t b = 0; b < subMeshCount; ++b)
		{
			PX_PLACEMENT_NEW(&pMeshes[b], ClothingAssetSubMesh());

			RenderDataFormat::Enum outFormat;
			pMeshes[b].mPositions = (const PxVec3*)meshAsset.getVertexBuffer(b, RenderVertexSemantic::POSITION, outFormat);
			pMeshes[b].mPositionOutFormat = outFormat;
			pMeshes[b].mNormals = (const PxVec3*)meshAsset.getVertexBuffer(b, RenderVertexSemantic::NORMAL, outFormat);
			pMeshes[b].mNormalOutFormat = outFormat;

			pMeshes[b].mTangents = (const PxVec4*)meshAsset.getVertexBuffer(b, RenderVertexSemantic::TANGENT, outFormat);
			pMeshes[b].mTangentOutFormat = outFormat;
			PX_ASSERT(((size_t)pMeshes[b].mTangents & 0xf) == 0);

			pMeshes[b].mBoneWeights = (const float*)meshAsset.getVertexBuffer(b, RenderVertexSemantic::BONE_WEIGHT, outFormat);
			pMeshes[b].mBoneWeightOutFormat = outFormat;
			pMeshes[b].mBoneIndices = (const uint16_t*)meshAsset.getVertexBuffer(b, RenderVertexSemantic::BONE_INDEX, outFormat);
			pMeshes[b].mVertexCount = meshAsset.getNumVertices(b);
			pMeshes[b].mNumBonesPerVertex = meshAsset.getNumBonesPerVertex(b);

			maxVertices = PxMax(maxVertices, meshAsset.getNumVertices(b));

			pMeshes[b].mIndices = (const uint32_t*)meshAsset.getIndexBuffer(b);
			pMeshes[b].mIndicesCount = meshAsset.getNumIndices(b);

			const VertexUV* PX_RESTRICT uvs = NULL;
			RenderDataFormat::Enum uvFormat = RenderDataFormat::UNSPECIFIED;
			switch (uvChannel)
			{
			case 0:
				uvs = (const VertexUV*)meshAsset.getVertexBuffer(b, RenderVertexSemantic::TEXCOORD0, uvFormat);
				break;
			case 1:
				uvs = (const VertexUV*)meshAsset.getVertexBuffer(b, RenderVertexSemantic::TEXCOORD1, uvFormat);
				break;
			case 2:
				uvs = (const VertexUV*)meshAsset.getVertexBuffer(b, RenderVertexSemantic::TEXCOORD2, uvFormat);
				break;
			case 3:
				uvs = (const VertexUV*)meshAsset.getVertexBuffer(b, RenderVertexSemantic::TEXCOORD3, uvFormat);
				break;
			}

			pMeshes[b].mUvs = (VertexUVLocal*)uvs;
			pMeshes[b].mUvFormat = uvFormat;
		}
		pMeshes += subMeshCount;
	}


	ClothingPhysicalMeshData* pPhysicalMeshes = (ClothingPhysicalMeshData*)pMeshes;

	assetData.mPhysicalMeshOffset = (uint32_t)((uint8_t*)pMeshes - assetData.mData);

	for (uint32_t a = 0; a < numPhysicalMeshes; ++a)
	{
		ClothingPhysicalMeshData* pPhysicalMesh = &pPhysicalMeshes[a];
		PX_PLACEMENT_NEW(pPhysicalMesh, ClothingPhysicalMeshData);

		ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh = getPhysicalMeshFromLod(a);
		ClothingPhysicalMeshParameters* pPhysicalMeshParams = mPhysicalMeshes[a];
		PX_UNUSED(pPhysicalMeshParams);

		pPhysicalMesh->mVertices = physicalMesh->vertices.buf;
		pPhysicalMesh->mVertexCount = physicalMesh->numVertices;
		pPhysicalMesh->mSimulatedVertexCount = physicalMesh->numSimulatedVertices;
		pPhysicalMesh->mMaxDistance0VerticesCount = physicalMesh->numMaxDistance0Vertices;

		pPhysicalMesh->mNormals = physicalMesh->normals.buf;
		pPhysicalMesh->mSkinningNormals = physicalMesh->skinningNormals.buf;
		pPhysicalMesh->mSkinningNormalsCount = (uint32_t)physicalMesh->skinningNormals.arraySizes[0];
		pPhysicalMesh->mNumBonesPerVertex = physicalMesh->numBonesPerVertex;

		pPhysicalMesh->mBoneIndices = physicalMesh->boneIndices.buf;
		pPhysicalMesh->mBoneWeights = physicalMesh->boneWeights.buf;
		PX_ASSERT(physicalMesh->boneIndices.arraySizes[0] == physicalMesh->boneWeights.arraySizes[0]);
		pPhysicalMesh->mBoneWeightsCount = (uint32_t)physicalMesh->boneWeights.arraySizes[0];

		pPhysicalMesh->mOptimizationData = physicalMesh->optimizationData.buf;
		pPhysicalMesh->mOptimizationDataCount = (uint32_t)physicalMesh->optimizationData.arraySizes[0];

		pPhysicalMesh->mIndices = physicalMesh->indices.buf;
		pPhysicalMesh->mIndicesCount = (uint32_t)physicalMesh->indices.arraySizes[0];
		pPhysicalMesh->mSimulatedIndicesCount = physicalMesh->numSimulatedIndices;
	}

	//Initialized compressed num bones per vertex...
	mCompressedNumBonesPerVertexMutex.lock();
	if (mCompressedNumBonesPerVertex.empty())
	{
		initializeCompressedNumBonesPerVertex();
	}
	mCompressedNumBonesPerVertexMutex.unlock();

	assetData.mCompressedNumBonesPerVertexCount = mCompressedNumBonesPerVertex.size();
	assetData.mCompressedNumBonesPerVertex = assetData.mCompressedNumBonesPerVertexCount > 0 ? &(mCompressedNumBonesPerVertex.front()) : NULL;

	assetData.mCompressedTangentWCount = mCompressedTangentW.size();
	assetData.mCompressedTangentW = assetData.mCompressedTangentWCount > 0 ? &(mCompressedTangentW.front()) : NULL;

	assetData.mPhysicalMeshesCount = mPhysicalMeshes.size();
	assetData.mExt2IntMorphMappingCount = mExt2IntMorphMapping.size();
	if (mExt2IntMorphMapping.size())
	{
		assetData.mExt2IntMorphMapping = &(mExt2IntMorphMapping.front());
	}
	else
	{
		assetData.mExt2IntMorphMapping = NULL;
	}

	assetData.mBoneCount = mBones.size();

	return maxVertices;

}



const RenderMeshAsset* ClothingAssetImpl::getRenderMeshAsset(uint32_t lodLevel) const
{
	READ_ZONE();

	if (lodLevel < mGraphicalLods.size())
	{
		return reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[lodLevel]->renderMeshAssetPointer);
	}

	return NULL;
}



uint32_t ClothingAssetImpl::getMeshSkinningMapSize(uint32_t lod)
{
	WRITE_ZONE();
	if (lod >= mGraphicalLods.size())
	{
		APEX_INVALID_PARAMETER("lod %i not a valid lod level. There are only %i graphical lods.", lod, mGraphicalLods.size());
		return 0;
	}
	ClothingGraphicalLodParameters* graphicalLod = mGraphicalLods[lod];

	// make sure everything can be skinned using the skinClothMap,
	// so the user doesn't have to care about immediate skinning as well
	mergeMapping(graphicalLod);

	return (uint32_t)mGraphicalLods[lod]->skinClothMap.arraySizes[0];
}



void ClothingAssetImpl::getMeshSkinningMap(uint32_t lod, ClothingMeshSkinningMap* map)
{
	WRITE_ZONE();

	if (lod >= mGraphicalLods.size())
	{
		APEX_INVALID_PARAMETER("lod %i not a valid lod level. There are only %i graphical lods.", lod, mGraphicalLods.size());
		return;
	}
	ClothingGraphicalLodParameters* graphicalLod = mGraphicalLods[lod];

	// make sure everything can be skinned using the skinClothMap,
	// so the user doesn't have to care about immediate skinning as well
	mergeMapping(graphicalLod);

	PX_ASSERT(graphicalLod->skinClothMapOffset > 0.0f); // skinClothMapOffset would only be needed if it's negative, but it's not expected to be

	// copy the values
	int32_t size = mGraphicalLods[lod]->skinClothMap.arraySizes[0];
	for (int32_t i = 0; i < size; ++i)
	{
		const SkinClothMap& skinMap	= mGraphicalLods[lod]->skinClothMap.buf[i];
		map[i].positionBary			= skinMap.vertexBary;
		map[i].vertexIndex0			= skinMap.vertexIndex0;
		map[i].normalBary			= skinMap.normalBary;
		map[i].vertexIndex1			= skinMap.vertexIndex1;
		map[i].tangentBary			= skinMap.tangentBary;
		map[i].vertexIndex2			= skinMap.vertexIndex2;
	}
}



bool ClothingAssetImpl::releaseGraphicalData()
{
	WRITE_ZONE();
	bool ok = true;
	for (uint32_t i = 0; i < mGraphicalLods.size(); ++i)
	{
		if (mGraphicalLods[i]->skinClothMapB.arraySizes[0] > 0 || mGraphicalLods[i]->tetraMap.arraySizes[0] > 0)
		{
			APEX_DEBUG_WARNING("Asset contains data that is not supported for external skinning, graphical data cannot be released. Reexport the asset with a newer APEX version.");
			ok = false;
		}

		if (mGraphicalLods[i]->immediateClothMap.arraySizes[0] > 0)
		{
			APEX_DEBUG_WARNING("Asset contains immediate map data, graphical data cannot be released. Call getMeshSkinningMap first.");
			ok = false;
		}
	}

	if (mActors.getSize() > 0)
	{
		APEX_DEBUG_WARNING("Graphical data in asset cannot be released while there are actors of this asset.");
		ok = false;
	}

	if (ok)
	{
		mModule->notifyReleaseGraphicalData(this);

		for (uint32_t i = 0; i < mGraphicalLods.size(); ++i)
		{
			if (mGraphicalLods[i]->renderMeshAssetPointer != NULL)
			{
				RenderMeshAssetIntl* rma = reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[i]->renderMeshAssetPointer);
				rma->release();
				mGraphicalLods[i]->renderMeshAssetPointer = NULL;

				mGraphicalLods[i]->renderMeshAsset->destroy();
				mGraphicalLods[i]->renderMeshAsset = NULL;
			}

			ParamArray<SkinClothMap> skinClothMap(mGraphicalLods[i], "skinClothMap",
				reinterpret_cast<ParamDynamicArrayStruct*>(&mGraphicalLods[i]->skinClothMap));
			skinClothMap.clear();
		}
	}

	return ok;
}



void ClothingAssetImpl::setupInvBindMatrices()
{
	if (mInvBindPoses.size() == mBones.size())
	{
		return;
	}

	mInvBindPoses.resize(mBones.size());
	for (uint32_t i = 0; i < mBones.size(); i++)
	{
		mInvBindPoses[i] = mBones[i].bindPose.inverseRT();
	}
}



void ClothingAssetImpl::prepareCookingJob(CookingAbstract& job, float scale, PxVec3* gravityDirection, PxVec3* morphedPhysicalMesh)
{
	PxVec3* tempVerticesForScale = NULL;
	uint32_t tempVerticesOffset = 0;
	if (scale != 1.0f || morphedPhysicalMesh != NULL)
	{
		uint32_t numMaxVertices = 0;
		for (uint32_t physicalMeshId = 0; physicalMeshId < mPhysicalMeshes.size(); physicalMeshId++)
		{
			numMaxVertices += mPhysicalMeshes[physicalMeshId]->physicalMesh.numVertices;
		}
		numMaxVertices += mBoneVertices.size();

		tempVerticesForScale = (PxVec3*)PX_ALLOC(sizeof(PxVec3) * numMaxVertices, PX_DEBUG_EXP("tempVerticesForScale"));
		PX_ASSERT(tempVerticesForScale != NULL);

		for (uint32_t physicalMeshId = 0; physicalMeshId < mPhysicalMeshes.size(); physicalMeshId++)
		{
			const uint32_t numVertices = mPhysicalMeshes[physicalMeshId]->physicalMesh.numVertices;
			PxVec3* origVertices = morphedPhysicalMesh != NULL ? morphedPhysicalMesh + tempVerticesOffset : mPhysicalMeshes[physicalMeshId]->physicalMesh.vertices.buf;
			PxVec3* tempVertices = tempVerticesForScale + tempVerticesOffset;
			for (uint32_t i = 0; i < numVertices; i++)
			{
				tempVertices[i] = origVertices[i] * scale;
			}
			tempVerticesOffset += numVertices;
		}
	}

	tempVerticesOffset = 0;

	for (uint32_t physicalMeshId = 0; physicalMeshId < mPhysicalMeshes.size(); physicalMeshId++)
	{
		CookingAbstract::PhysicalMesh physicalMesh;
		physicalMesh.meshID = physicalMeshId;
		if (tempVerticesForScale != NULL)
		{
			physicalMesh.vertices = tempVerticesForScale + tempVerticesOffset;
		}
		else
		{
			physicalMesh.vertices = mPhysicalMeshes[physicalMeshId]->physicalMesh.vertices.buf;
		}
		physicalMesh.numVertices = mPhysicalMeshes[physicalMeshId]->physicalMesh.numVertices;
		physicalMesh.numSimulatedVertices = mPhysicalMeshes[physicalMeshId]->physicalMesh.numSimulatedVertices;
		physicalMesh.numMaxDistance0Vertices = mPhysicalMeshes[physicalMeshId]->physicalMesh.numMaxDistance0Vertices;
		physicalMesh.indices = mPhysicalMeshes[physicalMeshId]->physicalMesh.indices.buf;
		physicalMesh.numIndices = mPhysicalMeshes[physicalMeshId]->physicalMesh.numIndices;
		physicalMesh.numSimulatedIndices = mPhysicalMeshes[physicalMeshId]->physicalMesh.numSimulatedIndices;
		physicalMesh.isTetrahedral = mPhysicalMeshes[physicalMeshId]->physicalMesh.isTetrahedralMesh;

		job.addPhysicalMesh(physicalMesh);

		tempVerticesOffset += physicalMesh.numVertices;
	}

	if (tempVerticesForScale == NULL)
	{
		// verify that there are no invalid matrices
		for (uint32_t i = 0; i < mBoneActors.size(); i++)
		{
			if (mBoneActors[i].capsuleRadius > 0.0f || mBoneActors[i].convexVerticesCount == 0)
			{
				continue;
			}

			uint32_t boneIndex = (uint32_t)mBoneActors[i].boneIndex;

			const PxMat44& bpm44 = mBones[boneIndex].bindPose;
			const PxMat44& lpm44 = mBoneActors[i].localPose;
			const PxMat33 bpm33(bpm44.column0.getXYZ(),
										bpm44.column1.getXYZ(),
										bpm44.column2.getXYZ()),
										lpm33(lpm44.column0.getXYZ(),
										lpm44.column1.getXYZ(),
										lpm44.column2.getXYZ());

			const float det = bpm33.getDeterminant() * lpm33.getDeterminant();
			if (det < 0.0f)
			{
				// invalid matrices found, need to use temporary buffer only for bone vertices now
				tempVerticesForScale = (PxVec3*)GetInternalApexSDK()->getTempMemory(sizeof(PxVec3) * mBoneVertices.size());
				PX_ASSERT(tempVerticesOffset == 0);
				break;
			}
		}
	}

	if (tempVerticesForScale != NULL)
	{
		memset(tempVerticesForScale + tempVerticesOffset, 0xff, sizeof(PxVec3) * mBoneVertices.size());

		PxVec3* boneVertices = morphedPhysicalMesh != NULL ? morphedPhysicalMesh + tempVerticesOffset : mBoneVertices.begin();

		for (uint32_t i = 0; i < mBoneActors.size(); i++)
		{
			uint32_t boneIndex = (uint32_t)mBoneActors[i].boneIndex;
			const PxMat44 bpm44(mBones[boneIndex].bindPose),
										lpm44(mBoneActors[i].localPose);
			const PxMat33 bpm33(bpm44.column0.getXYZ(),
										bpm44.column1.getXYZ(),
										bpm44.column2.getXYZ()),
										lpm33(lpm44.column0.getXYZ(),
										lpm44.column1.getXYZ(),
										lpm44.column2.getXYZ());

			const float det = bpm33.getDeterminant() * lpm33.getDeterminant();
			for (uint32_t j = 0; j < mBoneActors[i].convexVerticesCount; j++)
			{
				uint32_t boneVertexIndex = mBoneActors[i].convexVerticesStart + j;
				PxVec3 val = boneVertices[boneVertexIndex] * scale;
				if (det < 0.0f)
				{
					val.z = -val.z;
				}

				tempVerticesForScale[boneVertexIndex + tempVerticesOffset] = val;
			}
		}

		for (uint32_t i = 0; i < mBoneVertices.size(); i++)
		{
			PX_ASSERT(tempVerticesForScale[i + tempVerticesOffset].isFinite());
		}
	}

	PxVec3* boneVertices = tempVerticesForScale != NULL ? tempVerticesForScale + tempVerticesOffset : mBoneVertices.begin();
	job.setConvexBones(mBoneActors.begin(), mBoneActors.size(), mBones.begin(), mBones.size(), boneVertices, 256);

	job.setScale(scale);
	job.setVirtualParticleDensity(mParams->simulation.virtualParticleDensity);

	if (mParams->materialLibrary != NULL)
	{
		ClothingMaterialLibraryParameters* matLib = static_cast<ClothingMaterialLibraryParameters*>(mParams->materialLibrary);
		float selfcollisionThickness = matLib->materials.buf[mParams->materialIndex].selfcollisionThickness;
		job.setSelfcollisionRadius(selfcollisionThickness);
	}

	PxVec3 gravityDir = mParams->simulation.gravityDirection;
	if (gravityDir.isZero() && gravityDirection != NULL)
	{
		gravityDir = *gravityDirection;
	}
	if (gravityDir.isZero())
	{
		APEX_DEBUG_WARNING("(%s) Gravity direction is zero. Impossible to extract vertical- and zero-stretch fibers.", mName.c_str());
	}
	job.setGravityDirection(gravityDir);

	job.freeTempMemoryWhenDone(tempVerticesForScale);
}



uint32_t* ClothingAssetImpl::getMorphMapping(uint32_t graphicalLod, uint32_t submeshIndex)
{
	if (mExt2IntMorphMapping.empty())
	{
		if (!mMorphMappingWarning)
		{
			APEX_INVALID_OPERATION("A clothing actor with morph displacements was specified, but the Asset <%s> was not prepared for morph displacements", mName.c_str());
			mMorphMappingWarning = true;
		}
		return NULL;
	}

	if (graphicalLod == (uint32_t) - 1 || graphicalLod > mGraphicalLods.size())
	{
		graphicalLod = mGraphicalLods.size();
	}

	uint32_t offset = 0;
	for (uint32_t i = 0; i < graphicalLod; i++)
	{
		RenderMeshAssetIntl* rma = getGraphicalMesh(i);
		if (rma == NULL)
			continue;

		for (uint32_t s = 0; s < rma->getSubmeshCount(); s++)
		{
			offset += rma->getSubmesh(s).getVertexCount(0);
		}
	}


	RenderMeshAssetIntl* rma = getGraphicalMesh(graphicalLod);
	if (rma != NULL)
	{
		PX_ASSERT(submeshIndex < rma->getSubmeshCount());
		submeshIndex = PxMin(submeshIndex, rma->getSubmeshCount());

		for (uint32_t i = 0; i < submeshIndex; ++i)
		{
			offset += rma->getSubmesh(i).getVertexCount(0);
		}
	}

	return mExt2IntMorphMapping.begin() + offset;
}



uint32_t ClothingAssetImpl::getPhysicalMeshOffset(uint32_t physicalMeshId)
{
	physicalMeshId = PxMin(physicalMeshId, mPhysicalMeshes.size());

	uint32_t result = 0;

	for (uint32_t i = 0; i < physicalMeshId; i++)
	{
		result += mPhysicalMeshes[i]->physicalMesh.numVertices;
	}

	return result;
}



class SkinClothMapFacePredicate
{
public:
	bool operator()(const SkinClothMapB& map1, const SkinClothMapB& map2) const
	{
		if (map1.submeshIndex < map2.submeshIndex)
		{
			return true;
		}
		else if (map1.submeshIndex > map2.submeshIndex)
		{
			return false;
		}

		return map1.faceIndex0 < map2.faceIndex0;
	}
};



class SkinClothMapBVertexPredicate
{
public:
	bool operator()(const SkinClothMapB& map1, const SkinClothMapB& map2) const
	{
		return map1.vertexIndexPlusOffset < map2.vertexIndexPlusOffset;
	}
};



void ClothingAssetImpl::getDisplacedPhysicalMeshPositions(PxVec3* morphDisplacements, ParamArray<PxVec3> displacedMeshPositions)
{
	uint32_t numPhysicalVertices = getPhysicalMeshOffset((uint32_t) - 1);
	numPhysicalVertices += mBoneVertices.size();

	if (numPhysicalVertices == 0)
	{
		displacedMeshPositions.clear();
		return;
	}

	displacedMeshPositions.resize(numPhysicalVertices);
	memset(displacedMeshPositions.begin(), 0, sizeof(PxVec3) * numPhysicalVertices);
	float* resultWeights = (float*)PX_ALLOC(sizeof(float) * numPhysicalVertices, PX_DEBUG_EXP("ClothingAssetImpl::getDisplacedPhysicalMeshPositions"));
	memset(resultWeights, 0, sizeof(float) * numPhysicalVertices);

	uint32_t resultOffset = 0;
	for (uint32_t pm = 0; pm < mPhysicalMeshes.size(); pm++)
	{
		uint32_t gm = 0;
		for (; gm < mGraphicalLods.size(); gm++)
		{
			if (mGraphicalLods[gm]->physicalMeshId == pm)
			{
				break;
			}
		}

		ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh = mPhysicalMeshes[pm]->physicalMesh;

		if (gm < mGraphicalLods.size())
		{
			const ClothingGraphicalLodParameters* graphicalLod = mGraphicalLods[gm];

			const RenderMeshAsset* rma = getRenderMeshAsset(gm);
			uint32_t* morphMapping = getMorphMapping(gm, 0);
			uint32_t morphOffset = 0;

			AbstractMeshDescription pMesh;
			pMesh.pIndices = physicalMesh.indices.buf;
			pMesh.numIndices = physicalMesh.numIndices;
			pMesh.pPosition = physicalMesh.vertices.buf;

			if (graphicalLod->immediateClothMap.buf == NULL && graphicalLod->skinClothMapB.buf != NULL)
			{
				// PH: Need to resort the skinMapB buffer to vertex order, will resort back to face order just below
				sort<SkinClothMapB, SkinClothMapBVertexPredicate>(graphicalLod->skinClothMapB.buf, (uint32_t)graphicalLod->skinClothMapB.arraySizes[0], SkinClothMapBVertexPredicate());
			}

			for (uint32_t submeshIndex = 0; submeshIndex < rma->getSubmeshCount(); submeshIndex++)
			{
				const RenderSubmesh& submesh = rma->getSubmesh(submeshIndex);

				const uint32_t vertexCount = submesh.getVertexCount(0);
				for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
				{
					const PxVec3 displacement = morphMapping != NULL ? morphDisplacements[morphMapping[morphOffset + vertexIndex]] : PxVec3(0.0f);

					uint32_t indices[4];
					float weight[4];

					uint32_t numIndices = getCorrespondingPhysicalVertices(*graphicalLod, submeshIndex, vertexIndex, pMesh, morphOffset, indices, weight);

					for (uint32_t i = 0; i < numIndices; i++)
					{
						weight[i] += 0.001f; // none of the weights is 0!
						PX_ASSERT(weight[i] > 0.0f);
						PX_ASSERT(indices[i] < pMesh.numIndices);
						displacedMeshPositions[resultOffset + indices[i]] += (displacement + pMesh.pPosition[indices[i]]) * weight[i];
						resultWeights[resultOffset + indices[i]] += weight[i];
					}
				}

				morphOffset += vertexCount;
			}

			if (graphicalLod->immediateClothMap.buf == NULL && graphicalLod->skinClothMapB.buf != NULL)
			{
				nvidia::sort<SkinClothMapB, SkinClothMapFacePredicate>(graphicalLod->skinClothMapB.buf, (uint32_t)graphicalLod->skinClothMapB.arraySizes[0], SkinClothMapFacePredicate());
			}

			for (uint32_t i = 0; i < physicalMesh.numVertices; i++)
			{
				const uint32_t resultIndex = i + resultOffset;
				if (resultWeights[resultIndex] > 0.0f)
				{
					displacedMeshPositions[resultIndex] /= resultWeights[resultIndex];
				}
				else
				{
					morphOffset = 0;
					const PxVec3 physicsPosition = pMesh.pPosition[i];
					float shortestDistance = PX_MAX_F32;
					for (uint32_t submeshIndex = 0; submeshIndex < rma->getSubmeshCount() && shortestDistance > 0.0f; submeshIndex++)
					{
						const RenderSubmesh& submesh = rma->getSubmesh(submeshIndex);

						const VertexFormat& format = submesh.getVertexBuffer().getFormat();
						uint32_t positionIndex = (uint32_t)format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::POSITION));
						PX_ASSERT(format.getBufferFormat(positionIndex) == RenderDataFormat::FLOAT3);
						PxVec3* positions = (PxVec3*)submesh.getVertexBuffer().getBuffer(positionIndex);


						const uint32_t vertexCount = submesh.getVertexCount(0);
						for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
						{
							float dist2 = (physicsPosition - positions[vertexIndex]).magnitudeSquared();
							if (dist2 < shortestDistance)
							{
								shortestDistance = dist2;
								displacedMeshPositions[resultIndex] = physicsPosition + morphDisplacements[morphMapping[morphOffset + vertexIndex]];
								if (dist2 == 0.0f)
								{
									break;
								}
							}
						}
						morphOffset += submesh.getVertexCount(0);
					}
				}
			}
		}

		resultOffset += physicalMesh.numVertices;
	}

	PX_FREE(resultWeights);
	resultWeights = NULL;

	for (uint32_t ba = 0; ba < mBoneActors.size(); ba++)
	{

		for (uint32_t bi = 0; bi < mBoneActors[ba].convexVerticesCount; bi++)
		{
			uint32_t boneIndex = bi + mBoneActors[ba].convexVerticesStart;
			PX_ASSERT(mBoneActors[ba].localPose == PxMat44(PxIdentity));
			const PxMat44 bindPose = mBones[(uint32_t)mBoneActors[ba].boneIndex].bindPose;
			const PxVec3 physicsPosition =  bindPose.transform(mBoneVertices[boneIndex]);
			PxVec3 resultPosition(0.0f);

			float shortestDistance = PX_MAX_F32;
			for (uint32_t gm = 0; gm < mGraphicalLods.size() && shortestDistance > 0.0f; gm++)
			{
				const RenderMeshAsset* rma = getRenderMeshAsset(gm);
				uint32_t* morphMapping = getMorphMapping(gm, 0);
				uint32_t morphOffset = 0;

				for (uint32_t submeshIndex = 0; submeshIndex < rma->getSubmeshCount() && shortestDistance > 0.0f; submeshIndex++)
				{
					const RenderSubmesh& submesh = rma->getSubmesh(submeshIndex);

					const VertexFormat& format = submesh.getVertexBuffer().getFormat();
					uint32_t positionIndex = (uint32_t)format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::POSITION));
					PX_ASSERT(format.getBufferFormat(positionIndex) == RenderDataFormat::FLOAT3);
					PxVec3* positions = (PxVec3*)submesh.getVertexBuffer().getBuffer(positionIndex);


					const uint32_t vertexCount = submesh.getVertexCount(0);
					for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
					{
						float dist2 = (physicsPosition - positions[vertexIndex]).magnitudeSquared();
						if (dist2 < shortestDistance)
						{
							shortestDistance = dist2;
							resultPosition = physicsPosition + morphDisplacements[morphMapping[morphOffset + vertexIndex]];
							if (dist2 == 0.0f)
							{
								break;
							}
						}
					}
					morphOffset += submesh.getVertexCount(0);
				}
			}

			PxMat33 invBindPose(bindPose.column0.getXYZ(), bindPose.column1.getXYZ(), bindPose.column2.getXYZ());
			invBindPose = invBindPose.getInverse();
			displacedMeshPositions[resultOffset + boneIndex] = invBindPose.transform(resultPosition) + invBindPose.transform(-bindPose.getPosition());
		}
	}
	resultOffset += mBoneVertices.size();


	PX_ASSERT(resultOffset == numPhysicalVertices);
}



void ClothingAssetImpl::initializeCompressedNumBonesPerVertex()
{
	// PH: Merged the tangent w into this code, can be done at the same time
	uint32_t numBonesElementCount = 0;
	uint32_t numTangentElementCount = 0;
	for (uint32_t lodIndex = 0; lodIndex < mGraphicalLods.size(); lodIndex++)
	{
		ClothingGraphicalMeshAssetWrapper meshAsset(reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[lodIndex]->renderMeshAssetPointer));

		for (uint32_t submeshIdx = 0; submeshIdx < meshAsset.getSubmeshCount(); submeshIdx++)
		{
			const uint32_t numVertices = meshAsset.getNumVertices(submeshIdx);

			// 3 bits per entry, 10 entries per U32
			uint32_t numBoneEntries = (numVertices + 15) / 16;

			RenderDataFormat::Enum outFormat = RenderDataFormat::UNSPECIFIED;
			const PxVec4* PX_RESTRICT tangents = (const PxVec4*)meshAsset.getVertexBuffer(submeshIdx, RenderVertexSemantic::TANGENT, outFormat);
			PX_ASSERT(tangents == NULL || outFormat == RenderDataFormat::FLOAT4);

			uint32_t numTangentEntries = tangents != NULL ? (numVertices + 31) / 32 : 0;

			// Round up such that map for all submeshes is 16 byte aligned
			while ((numBoneEntries & 0x3) != 0) // this is a numEntries % 4
			{
				numBoneEntries++;
			}

			while ((numTangentEntries & 0x3) != 0)
			{
				numTangentEntries++;
			}

			numBonesElementCount += numBoneEntries;
			numTangentElementCount += numTangentEntries;
		}
	}

	if (numBonesElementCount > 0)
	{
		mCompressedNumBonesPerVertex.resize(numBonesElementCount, 0);

		uint32_t numNonNormalizedVertices = 0;
		uint32_t numInefficientVertices = 0;

		uint32_t* bonesPerVertex = mCompressedNumBonesPerVertex.begin();
		for (uint32_t lodIndex = 0; lodIndex < mGraphicalLods.size(); lodIndex++)
		{
			ClothingGraphicalMeshAssetWrapper meshAsset(reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[lodIndex]->renderMeshAssetPointer));

			for (uint32_t submeshIdx = 0; submeshIdx < meshAsset.getSubmeshCount(); submeshIdx++)
			{
				uint32_t numVerticesWritten = 0;

				RenderDataFormat::Enum outFormat = RenderDataFormat::UNSPECIFIED;
				const float* PX_RESTRICT boneWeights = (const float*)meshAsset.getVertexBuffer(submeshIdx, RenderVertexSemantic::BONE_WEIGHT, outFormat);

				const uint32_t numVertices = meshAsset.getNumVertices(submeshIdx);
				const uint32_t numBonesPerVertex = meshAsset.getNumBonesPerVertex(submeshIdx);
				PX_ASSERT((numBonesPerVertex > 0) == (boneWeights != NULL));

				PX_ASSERT(((size_t)bonesPerVertex & 0xf) == 0); // make sure we start 16 byte aligned
				for (uint32_t vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
				{
					uint32_t firstZeroBoneAfter = 0;
					if (boneWeights != NULL)
					{
						const float* PX_RESTRICT vertexBoneWeights = boneWeights + (vertexIndex * numBonesPerVertex);
						uint32_t firstZeroBone = numBonesPerVertex;
						float sumWeights = 0.0f;
						for (uint32_t k = 0; k < numBonesPerVertex; k++)
						{
							sumWeights += vertexBoneWeights[k];
							if (vertexBoneWeights[k] == 0.0f)
							{
								firstZeroBone = PxMin(firstZeroBone, k);
							}
							else
							{
								firstZeroBoneAfter = k + 1;
							}
						}
						PX_ASSERT(firstZeroBoneAfter <= numBonesPerVertex);

						numNonNormalizedVertices += PxAbs(sumWeights - 1.0f) > 0.001f ? 1 : 0;
						numInefficientVertices += firstZeroBone < firstZeroBoneAfter ? 1 : 0;
					}
					else
					{
						firstZeroBoneAfter = 1;
					}

					// write the value
					if (numVerticesWritten == 16)
					{
						bonesPerVertex++;
						numVerticesWritten = 0;
					}

					PX_ASSERT(firstZeroBoneAfter > 0);
					PX_ASSERT(firstZeroBoneAfter < 5); // or else it doesn't fit
					(*bonesPerVertex) |= ((firstZeroBoneAfter - 1) & 0x3) << (numVerticesWritten * 2);
					numVerticesWritten++;
				}

				// if *bonesPerVertex contains data, advance
				if (numVerticesWritten > 0)
				{
					bonesPerVertex++;
				}

				// advance until 16 byte aligned
				while (((size_t)bonesPerVertex & 0xf) != 0)
				{
					bonesPerVertex++;
				}
			}
		}

		if (numNonNormalizedVertices > 0)
		{
			APEX_DEBUG_WARNING("The Clothing Asset <%s> has %d vertices with non-normalized bone weights. This may lead to wrongly displayed meshes.", mName.c_str(), numNonNormalizedVertices);
		}

		if (numInefficientVertices > 0)
		{
			APEX_DEBUG_WARNING("The Clothing Asset <%s> has %d vertices with non-sorted bone weights. This can decrease performance of the skinning. Resave the asset!", mName.c_str(), numInefficientVertices);
		}
	}

	if (numTangentElementCount > 0)
	{
		mCompressedTangentW.resize(numTangentElementCount, 0);

		uint32_t* tangentW = mCompressedTangentW.begin();
		for (uint32_t lodIndex = 0; lodIndex < mGraphicalLods.size(); lodIndex++)
		{
			ClothingGraphicalMeshAssetWrapper meshAsset(reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[lodIndex]->renderMeshAssetPointer));

			for (uint32_t submeshIdx = 0; submeshIdx < meshAsset.getSubmeshCount(); submeshIdx++)
			{
				uint32_t numVerticesWritten = 0;

				RenderDataFormat::Enum outFormat = RenderDataFormat::UNSPECIFIED;
				const PxVec4* PX_RESTRICT tangents = (const PxVec4*)meshAsset.getVertexBuffer(submeshIdx, RenderVertexSemantic::TANGENT, outFormat);
				PX_ASSERT(outFormat == RenderDataFormat::FLOAT4);

				PX_ASSERT(((size_t)tangentW & 0xf) == 0); // make sure we start 16 byte aligned

				const uint32_t numVertices = meshAsset.getNumVertices(submeshIdx);
				for (uint32_t vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
				{
					PX_ASSERT(PxAbs(tangents[vertexIndex].w) == 1.0f);
					const uint32_t tangentWpositive = tangents[vertexIndex].w > 0.0f ? 1u : 0u;

					// write the value
					if (numVerticesWritten == 32)
					{
						tangentW++;
						numVerticesWritten = 0;
					}

					(*tangentW) |= tangentWpositive << numVerticesWritten;
					numVerticesWritten++;
				}

				// if *bonesPerVertex contains data, advance
				if (numVerticesWritten > 0)
				{
					tangentW++;
				}

				// advance until 16 byte aligned
				while (((size_t)tangentW & 0xf) != 0)
				{
					tangentW++;
				}
			}
		}
	}
}



uint32_t ClothingAssetImpl::getRootBoneIndex()
{
	PX_ASSERT(mParams->rootBoneIndex < getNumUsedBones());
	return mParams->rootBoneIndex;
}



uint32_t ClothingAssetImpl::getInterCollisionChannels()
{
	return mParams->interCollisionChannels;
}


void ClothingAssetImpl::releaseCookedInstances()
{
	if (mParams != NULL && mActors.getSize() == 0)
	{
		for (int32_t i = 0; i < mParams->cookedData.arraySizes[0]; i++)
		{
			NvParameterized::Interface* cookedData = mParams->cookedData.buf[i].cookedData;
			if (cookedData != NULL)
			{
				BackendFactory* factory = mModule->getBackendFactory(cookedData->className());
				PX_ASSERT(factory != NULL);
				if (factory != NULL)
				{
					factory->releaseCookedInstances(mParams->cookedData.buf[i].cookedData);
				}
			}
		}
	}
}

void ClothingAssetImpl::destroy()
{
	mActors.clear();

	while (numCookingDependencies() > 0)
	{
		nvidia::Thread::sleep(0);
	}

	mModule->unregisterAssetWithScenes(this);

	mUnusedSimulationMutex.lock();
	for (uint32_t i = 0; i < mUnusedSimulation.size(); i++)
	{
		if (mUnusedSimulation[i] == NULL)
		{
			continue;
		}

		destroySimulation(mUnusedSimulation[i]);
		mUnusedSimulation[i] = NULL;
	}
	mUnusedSimulation.clear();
	mUnusedSimulationMutex.unlock();

	releaseCookedInstances();

	for (uint32_t i = 0; i < mPhysicalMeshes.size(); i++)
	{
		if (mParams != NULL)
		{
			// PH: we should only decrement if we don't use releaseAndReturnNvParameterizedInterface
			mPhysicalMeshes[i]->referenceCount--;
		}
	}

	for (uint32_t i = 0; i < mGraphicalLods.size(); i++)
	{
		if (mGraphicalLods[i]->renderMeshAssetPointer == NULL)
			continue;

		reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[i]->renderMeshAssetPointer)->release();
		mGraphicalLods[i]->renderMeshAssetPointer = NULL;
	}

#ifndef WITHOUT_PVD
	destroyPvdInstances();
#endif
	if (mParams != NULL)
	{
		// safety!
		for (uint32_t i = 0; i < mPhysicalMeshes.size(); i++)
		{
			PX_ASSERT(mPhysicalMeshes[i]->referenceCount == 0);
		}

		mParams->setSerializationCallback(NULL, NULL);
		mParams->destroy();
		mParams = NULL;
	}

	mCompressedNumBonesPerVertex.reset();

	delete this;
}



int32_t ClothingAssetImpl::getBoneInternalIndex(const char* boneName) const
{
	if (boneName == NULL)
	{
		return -1;
	}

	int32_t internalBoneIndex = -1;
	for (uint32_t i = 0; i < mBones.size(); i++)
	{
		if (mBones[i].name != NULL && (::strcmp(mBones[i].name, boneName) == 0))
		{
			internalBoneIndex  = (int32_t)i;
			break;
		}
	}

	return internalBoneIndex ;
}



int32_t ClothingAssetImpl::getBoneInternalIndex(uint32_t boneIndex) const
{
	if (boneIndex >= mBones.size())
	{
		return -1;
	}

	int32_t internalBoneIndex  = -1;
	for (uint32_t i = 0; i < mBones.size(); i++)
	{
		if (mBones[i].externalIndex == (int32_t)boneIndex)
		{
			internalBoneIndex  = (int32_t)i;
			break;
		}
	}

	return internalBoneIndex;
}



class SortGraphicalVerts
{
public:
	SortGraphicalVerts(uint32_t numVerts, uint32_t submeshOffset, const uint32_t* indices, uint32_t numIndices, ClothingGraphicalLodParameters* lodParameters,
		ClothingPhysicalMeshParameters* physicsMesh, RenderMeshAssetIntl* renderMeshAsset = NULL) : mNumNotFoundVertices(0), mIndices(indices), mNumIndices(numIndices)
	{
		PX_UNUSED(physicsMesh);
		mNew2Old.resize(numVerts, 0);
		mOld2New.resize(numVerts, 0);
		mVertexInfo.resize(numVerts);
		for (uint32_t i = 0; i < numVerts; i++)
		{
			mNew2Old[i] = i;
			mOld2New[i] = i;
			mVertexInfo[i].idealPosition = (float)i;
		}

		// scale the max distance such that it gives a guess for the 'idealPosition'
		// const float maxDistanceScale = (float)numVerts / physicsMesh->physicalMesh.maximumMaxDistance;
		// ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type* coeffs = physicsMesh->physicalMesh.constrainCoefficients.buf;

		if (lodParameters->immediateClothMap.arraySizes[0] > 0)
		{
			for (uint32_t i = 0; i < numVerts; i++)
			{
				const uint32_t immediateMap = lodParameters->immediateClothMap.buf[i + submeshOffset];
				if (immediateMap != ClothingConstants::ImmediateClothingInvalidValue)
				{
					if ((immediateMap & ClothingConstants::ImmediateClothingInSkinFlag) == 0)
					{
						const uint32_t targetIndex = immediateMap & ClothingConstants::ImmediateClothingReadMask;
						mVertexInfo[i].physicsMeshNumber = getMeshIndex(targetIndex, physicsMesh);
						PX_UNUSED(targetIndex);
					}
				}
			}
		}

		const uint32_t numSkins = (uint32_t)lodParameters->skinClothMap.arraySizes[0];
		for (uint32_t i = 0; i < numSkins; i++)
		{
			const uint32_t vertexIndex = lodParameters->skinClothMap.buf[i].vertexIndexPlusOffset - submeshOffset;
			if (vertexIndex < numVerts) // this also handles underflow
			{
				uint32_t physVertexIndex = PxMax(lodParameters->skinClothMap.buf[i].vertexIndex0, lodParameters->skinClothMap.buf[i].vertexIndex1);
				physVertexIndex = PxMax(physVertexIndex, lodParameters->skinClothMap.buf[i].vertexIndex2);
				const PxI32 submeshNumber = getMeshIndexMaxFromVert(physVertexIndex, physicsMesh);

				mVertexInfo[vertexIndex].physicsMeshNumber = PxMax(submeshNumber, mVertexInfo[vertexIndex].physicsMeshNumber);
			}
		}

		const uint32_t numSkinB = (uint32_t)lodParameters->skinClothMapB.arraySizes[0];
		for (uint32_t i = 0; i < numSkinB; i++)
		{
			const uint32_t faceIndex0 = lodParameters->skinClothMapB.buf[i].faceIndex0;
			PX_UNUSED(faceIndex0);
			const uint32_t vertexIndex = lodParameters->skinClothMapB.buf[i].vertexIndexPlusOffset - submeshOffset;
			if (vertexIndex < numVerts)
			{
				mVertexInfo[vertexIndex].physicsMeshNumber = PxMax(getMeshIndexMaxFromFace(faceIndex0, physicsMesh), mVertexInfo[vertexIndex].physicsMeshNumber);
			}
		}

		const uint32_t numTetras = (uint32_t)lodParameters->tetraMap.arraySizes[0];
		for (uint32_t i = 0; i < numTetras; i++)
		{
			const uint32_t tetraIndex0 = lodParameters->tetraMap.buf[i].tetraIndex0;
			mVertexInfo[i].physicsMeshNumber = PxMax(getMeshIndexMaxFromFace(tetraIndex0, physicsMesh), mVertexInfo[i].physicsMeshNumber);
		}

		if (renderMeshAsset != NULL)
		{
			uint32_t numSkinClothMaps = (uint32_t)lodParameters->skinClothMap.arraySizes[0];
			SkinClothMap* skinClothMaps = lodParameters->skinClothMap.buf;

			uint32_t count = 0;
			uint32_t targetOffset = 0;
			for (uint32_t s = 0; s < renderMeshAsset->getSubmeshCount(); s++)
			{
				const VertexBuffer& vb = renderMeshAsset->getSubmesh(s).getVertexBuffer();
				const VertexFormat& vf = vb.getFormat();

				const uint32_t graphicalMaxDistanceIndex = (uint32_t)vf.getBufferIndexFromID(vf.getID("MAX_DISTANCE"));
				RenderDataFormat::Enum outFormat = vf.getBufferFormat(graphicalMaxDistanceIndex);
				const float* graphicalMaxDistance = outFormat == RenderDataFormat::UNSPECIFIED ? NULL :
					reinterpret_cast<const float*>(vb.getBuffer(graphicalMaxDistanceIndex));
				PX_ASSERT(graphicalMaxDistance == NULL || outFormat == RenderDataFormat::FLOAT1);

				const uint32_t numVertices = renderMeshAsset->getSubmesh(s).getVertexCount(0);
				for (uint32_t vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
				{
					const uint32_t index = vertexIndex + targetOffset;
					if (graphicalMaxDistance != NULL && graphicalMaxDistance[vertexIndex] == 0.0f)
					{
						for (uint32_t i = 0; i < numSkinClothMaps; i++)
						{
							uint32_t grIndex = skinClothMaps[i].vertexIndexPlusOffset;
							if (grIndex == index)
							{
								count++;

								const uint32_t vertexIndex = grIndex - submeshOffset;
								if (vertexIndex < numVerts) // this also handles underflow
								{
									mVertexInfo[vertexIndex].physicsMeshNumber = -1;
								}
							}
						}
					}
				}

				targetOffset += numVertices;
			}
		}

		for (uint32_t i = 0; i < numVerts; i++)
		{
			if (mVertexInfo[i].physicsMeshNumber == -1)
			{
				// give it the largest number, such that it gets sorted to the very end instead of the very beginning. Then at least it's cpu skinned.
				mVertexInfo[i].physicsMeshNumber = PX_MAX_I32;
				mNumNotFoundVertices++;
			}
			PX_ASSERT(mVertexInfo[i].idealPosition != -1);
		}

		// we only know the submesh number for each individual vertex, but we need to make sure that this is consistent for each
		// triangle. So we define the submesh number for a triangle as the min of all the submesh numbers of its vertices.
		// Then we set the vertex submesh number to the min of all its triangle's submesh numbers.

		Array<int32_t> triangleMeshIndex(mNumIndices / 3, 0x7fffffff);
		for (uint32_t i = 0; i < triangleMeshIndex.size(); i++)
		{
			PxU32 index = i * 3;
			PxI32 meshNumber = PxMin(mVertexInfo[mIndices[index]].physicsMeshNumber, mVertexInfo[mIndices[index + 1]].physicsMeshNumber);
			triangleMeshIndex[i] = PxMin(meshNumber, mVertexInfo[mIndices[index + 2]].physicsMeshNumber);
		}

		// now let's redistribute it
		for (uint32_t i = 0; i < triangleMeshIndex.size(); i++)
		{
			for (uint32_t j = 0; j < 3; j++)
			{
				uint32_t index = mIndices[i * 3 + j];
				if (triangleMeshIndex[i] < mVertexInfo[index].physicsMeshNumber)
				{
					mVertexInfo[index].physicsMeshNumber = triangleMeshIndex[i];
					// we need to distinguish the ones that naturally belong to a submesh, or the border ones that got added this late
					mVertexInfo[index].pulledInThroughTriangle = true;
				}
			}
		}
	}

	PxI32 getMeshIndex(PxU32 vertexIndex, const ClothingPhysicalMeshParameters* physicsMesh)
	{
		if (physicsMesh->physicalMesh.numSimulatedVertices > vertexIndex)
		{
			return 0;
		}

		return 1;
	}

	PxI32 getMeshIndexMaxFromFace(PxU32 faceIndex0, const ClothingPhysicalMeshParameters* physicsMesh)
	{
		if (physicsMesh->physicalMesh.numSimulatedIndices > faceIndex0)
		{
			return 0;
		}

		return 1;
	}

	PxI32 getMeshIndexMaxFromVert(PxU32 vertexIndex, const ClothingPhysicalMeshParameters* physicsMesh)
	{

		if (physicsMesh->physicalMesh.numSimulatedVertices > vertexIndex)
		{
			return 0;
		}

		return 1;
	}

	bool operator()(const uint32_t a, const uint32_t b) const
	{
		if (mVertexInfo[a].pulledInThroughTriangle != mVertexInfo[b].pulledInThroughTriangle)
		{
			return mVertexInfo[a].pulledInThroughTriangle < mVertexInfo[b].pulledInThroughTriangle;
		}

		return mVertexInfo[a].idealPosition < mVertexInfo[b].idealPosition;
	}

	void sortVertices()
	{
		nvidia::sort(mNew2Old.begin(), mNew2Old.size(), *this);

		for (uint32_t i = 0; i < mNew2Old.size(); i++)
		{
			mOld2New[mNew2Old[i]] = i;
		}
	}

	uint32_t computeCost()
	{
		uint32_t totalDist = 0;
		// const uint32_t numVertsInCacheLine = 4096 / 12;
		for (uint32_t i = 0; i < mNumIndices; i += 3)
		{
			// create 3 edges
			const uint32_t edges[3] =
			{
				(uint32_t)PxAbs((int32_t)mOld2New[mIndices[i + 0]] - (int32_t)mOld2New[mIndices[i + 1]]),
				(uint32_t)PxAbs((int32_t)mOld2New[mIndices[i + 1]] - (int32_t)mOld2New[mIndices[i + 2]]),
				(uint32_t)PxAbs((int32_t)mOld2New[mIndices[i + 2]] - (int32_t)mOld2New[mIndices[i + 0]]),
			};

			for (uint32_t j = 0; j < 3; j++)
			{
				totalDist += edges[j];
			}

		}

		return totalDist;
	}

	PxI32 getMesh(PxU32 newVertexIndex)
	{
		return mVertexInfo[mNew2Old[newVertexIndex]].physicsMeshNumber;
	}

	bool isAdditional(uint32_t newVertexIndex)
	{
		return mVertexInfo[mNew2Old[newVertexIndex]].pulledInThroughTriangle;
	}

	nvidia::Array<uint32_t> mOld2New;
	nvidia::Array<uint32_t> mNew2Old;

	uint32_t mNumNotFoundVertices;

private:
	SortGraphicalVerts& operator=(const SortGraphicalVerts&);

	struct InternalInfo
	{
		InternalInfo() : physicsMeshNumber(-1), idealPosition(-1), idealChange(0.0f), idealCount(0.0f), pulledInThroughTriangle(false) {}
		int32_t physicsMeshNumber;
		float idealPosition;
		float idealChange;
		float idealCount;
		bool pulledInThroughTriangle;
	};

	nvidia::Array<InternalInfo> mVertexInfo;

	const uint32_t* mIndices;
	const uint32_t mNumIndices;
};



template <typename T>
class SkinClothMapPredicate
{
public:
	bool operator()(const T& map1, const T& map2) const
	{
		return map1.vertexIndexPlusOffset < map2.vertexIndexPlusOffset;
	}
};



class SortGraphicalIndices
{
public:
	SortGraphicalIndices(uint32_t numIndices, uint32_t* indices) : mIndices(indices)
	{
		mTriangleInfo.resize(numIndices / 3);
		for (uint32_t i = 0; i < mTriangleInfo.size(); i++)
		{
			mTriangleInfo[i].mesh = -1;
			mTriangleInfo[i].originalPosition = i;
		}

		mNew2Old.resize(numIndices / 3);
		mOld2New.resize(numIndices / 3);
		for (uint32_t i = 0; i < mNew2Old.size(); i++)
		{
			mNew2Old[i] = i;
			mOld2New[i] = i;
		}
	}

	void setTriangleMesh(PxU32 triangle, PxI32 mesh)
	{
		mTriangleInfo[triangle].mesh = mesh;
	}

	PxI32 getTriangleMesh(PxU32 newTriangleIndex) const
	{
		return mTriangleInfo[mNew2Old[newTriangleIndex]].mesh;
	}

	bool operator()(const uint32_t t1, const uint32_t t2) const
	{
		if (mTriangleInfo[t1].mesh != mTriangleInfo[t2].mesh)
		{
			return mTriangleInfo[t1].mesh < mTriangleInfo[t2].mesh;
		}

		return mTriangleInfo[t1].originalPosition < mTriangleInfo[t2].originalPosition;
	}

	void sort()
	{
		nvidia::sort(mNew2Old.begin(), mNew2Old.size(), *this);

		for (uint32_t i = 0; i < mNew2Old.size(); i++)
		{
			mOld2New[mNew2Old[i]] = i;
		}

		ApexPermute(mIndices, mNew2Old.begin(), mNew2Old.size(), 3);
	}

private:
	nvidia::Array<uint32_t> mNew2Old;
	nvidia::Array<uint32_t> mOld2New;
	uint32_t* mIndices;

	struct TriangleInfo
	{
		int32_t mesh;
		uint32_t originalPosition;
	};
	nvidia::Array<TriangleInfo> mTriangleInfo;
};



bool ClothingAssetImpl::reorderGraphicsVertices(uint32_t graphicalLodId, bool perfWarning)
{
	PX_ASSERT(mGraphicalLods[graphicalLodId] != NULL);

	const uint32_t curSortingVersion = 2; // bump this number when the sorting changes!

	if (mGraphicalLods[graphicalLodId]->renderMeshAssetSorting >= curSortingVersion)
	{
		// nothing needs to be done.
		return false;
	}

	mGraphicalLods[graphicalLodId]->renderMeshAssetSorting = curSortingVersion;

	if (perfWarning)
	{
		APEX_DEBUG_INFO("Performance warning. This asset <%s> has to be re-saved to speed up loading", mName.c_str());
	}

	RenderMeshAssetIntl* rma = static_cast<RenderMeshAssetIntl*>(mGraphicalLods[graphicalLodId]->renderMeshAssetPointer);
	PX_ASSERT(rma != NULL);
	if (rma == NULL)
		return false;

	const uint32_t numSubMeshes = rma->getSubmeshCount();
	uint32_t submeshVertexOffset = 0;

	const uint32_t numSkinClothMap = (uint32_t)mGraphicalLods[graphicalLodId]->skinClothMap.arraySizes[0];
	ClothingGraphicalLodParametersNS::SkinClothMapD_Type* skinClothMap = mGraphicalLods[graphicalLodId]->skinClothMap.buf;

	const uint32_t numSkinClothMapB = (uint32_t)mGraphicalLods[graphicalLodId]->skinClothMapB.arraySizes[0];
	ClothingGraphicalLodParametersNS::SkinClothMapB_Type* skinClothMapB = mGraphicalLods[graphicalLodId]->skinClothMapB.buf;

	// allocate enough space
	{
		if (mGraphicalLods[graphicalLodId]->physicsMeshPartitioning.arraySizes[0] != (int32_t)numSubMeshes)
		{
			NvParameterized::Handle handle(*mGraphicalLods[graphicalLodId], "physicsMeshPartitioning");
			PX_ASSERT(handle.isValid());
			PX_ASSERT(handle.parameterDefinition()->type() == NvParameterized::TYPE_ARRAY);
			handle.resizeArray((int32_t)numSubMeshes);
		}
	}

	uint32_t meshPartitioningIndex = 0;
	for (uint32_t s = 0; s < numSubMeshes; s++)
	{
		const RenderSubmesh& submesh = rma->getSubmesh(s);
		const uint32_t numVertices = submesh.getVertexCount(0);

		const uint32_t numIters = 2;
		uint32_t costs[numIters] = { 0 };

		ClothingPhysicalMeshParameters* physicsMesh = mPhysicalMeshes[mGraphicalLods[graphicalLodId]->physicalMeshId];

		SortGraphicalVerts sortedVertices(numVertices, submeshVertexOffset, submesh.getIndexBuffer(0), submesh.getIndexCount(0), mGraphicalLods[graphicalLodId], physicsMesh, rma);

		costs[0] = sortedVertices.computeCost();
		sortedVertices.sortVertices(); // same ordering as before, but grouped by submeshes now

		costs[1] = costs[0]; // stupid warnings
		costs[1] = sortedVertices.computeCost();


#if 0
		// reorder based on triangle distances (into the vertex buffer)
		// disable for now...
		// PH: On the PS3 perf is 30% better if this is not performed! Needs much more further investigation!
		uint32_t numIncreases = 0;
		for (uint32_t i = 2; i < numIters; i++)
		{
			sortedVertices.refineIdealPositions(1.0f);
			sortedVertices.sortVertices();
			costs[i] = sortedVertices.computeCost();
			if (costs[i] >= costs[i - 1])
			{
				numIncreases++;
				if (numIncreases > 40)
				{
					break;
				}
			}
			else if (numIncreases > 0)
			{
				numIncreases--;
			}
		}
#endif

		rma->getInternalSubmesh(s).applyPermutation(sortedVertices.mOld2New, sortedVertices.mNew2Old);

		{
			uint32_t vertexCount = 0;
			uint32_t vertexAdditionalCount = 0;

			while (vertexAdditionalCount < sortedVertices.mNew2Old.size() && sortedVertices.getMesh(vertexAdditionalCount) <= 0)
			{
				if (!sortedVertices.isAdditional(vertexAdditionalCount))
				{
					vertexCount = vertexAdditionalCount + 1;
				}
				vertexAdditionalCount++;
			}

			mGraphicalLods[graphicalLodId]->physicsMeshPartitioning.buf[meshPartitioningIndex].graphicalSubmesh = s;
			mGraphicalLods[graphicalLodId]->physicsMeshPartitioning.buf[meshPartitioningIndex].numSimulatedVertices = vertexCount;
			mGraphicalLods[graphicalLodId]->physicsMeshPartitioning.buf[meshPartitioningIndex].numSimulatedVerticesAdditional = vertexAdditionalCount;
		}

		// also sort the index buffer accordingly
		{
			uint32_t* indices = rma->getInternalSubmesh(s).getIndexBufferWritable(0);

			uint32_t indexCount = submesh.getIndexCount(0);
			uint32_t triCount = indexCount / 3;
			SortGraphicalIndices sortedIndices(indexCount, indices);

			for (uint32_t i = 0; i < triCount; i++)
			{
				int32_t meshIndex = 0;
				for (uint32_t j = 0; j < 3; j++)
				{
					meshIndex = PxMax(meshIndex, sortedVertices.getMesh(indices[i * 3 + j]));
				}
				sortedIndices.setTriangleMesh(i, meshIndex);
			}

			sortedIndices.sort();

			uint32_t startTriangle = 0;
			while (startTriangle < triCount) // && sortedIndices.getTriangleSubmesh(startTriangle) <= i)
			{
				startTriangle++;
			}
			mGraphicalLods[graphicalLodId]->physicsMeshPartitioning.buf[meshPartitioningIndex].numSimulatedIndices = startTriangle * 3;
		}

		// also adapt all mesh-mesh skinning tables

		uint32_t* immediateMap = mGraphicalLods[graphicalLodId]->immediateClothMap.buf;
		if (immediateMap != NULL)
		{
			ApexPermute(immediateMap + submeshVertexOffset, sortedVertices.mNew2Old.begin(), numVertices);
		}

		for (uint32_t i = 0; i < numSkinClothMap; i++)
		{
			if (skinClothMap[i].vertexIndexPlusOffset < submeshVertexOffset)
			{
				continue;
			}
			else if (skinClothMap[i].vertexIndexPlusOffset >= submeshVertexOffset + numVertices)
			{
				break;
			}

			uint32_t oldVertexIndex = skinClothMap[i].vertexIndexPlusOffset - submeshVertexOffset;
			skinClothMap[i].vertexIndexPlusOffset = submeshVertexOffset + sortedVertices.mOld2New[oldVertexIndex];
		}

		for (uint32_t i = 0; i < numSkinClothMapB; i++)
		{
			const uint32_t vertexIndex = skinClothMapB[i].vertexIndexPlusOffset;
			if (vertexIndex >= submeshVertexOffset && vertexIndex < submeshVertexOffset + numVertices)
			{
				skinClothMapB[i].vertexIndexPlusOffset = submeshVertexOffset + sortedVertices.mOld2New[vertexIndex - submeshVertexOffset];
			}
		}

		submeshVertexOffset += numVertices;
		meshPartitioningIndex++;
	}

	// make sure all maps are sorted again, only mapC type!
	nvidia::sort(skinClothMap, numSkinClothMap, SkinClothMapPredicate<ClothingGraphicalLodParametersNS::SkinClothMapD_Type>());

	uint32_t* immediateMap = mGraphicalLods[graphicalLodId]->immediateClothMap.buf;
	if (immediateMap != NULL)
	{
		for (uint32_t i = 0; i < numSkinClothMap; i++)
		{
			PX_ASSERT((immediateMap[skinClothMap[i].vertexIndexPlusOffset] & ClothingConstants::ImmediateClothingInSkinFlag) == ClothingConstants::ImmediateClothingInSkinFlag);
			immediateMap[skinClothMap[i].vertexIndexPlusOffset] = i | ClothingConstants::ImmediateClothingInSkinFlag;
		}

#ifdef _DEBUG
		// sanity check
		for (uint32_t i = 0; i < submeshVertexOffset; i++)
		{
			uint32_t imm = immediateMap[i];
			if (imm != ClothingConstants::ImmediateClothingInvalidValue)
			{
				if ((imm & ClothingConstants::ImmediateClothingInSkinFlag) == ClothingConstants::ImmediateClothingInSkinFlag)
				{
					imm &= ClothingConstants::ImmediateClothingReadMask;

					if (numSkinClothMap > 0)
					{
						PX_ASSERT(imm < numSkinClothMap);
						PX_ASSERT(skinClothMap[imm].vertexIndexPlusOffset == i);
					}
					else if (numSkinClothMapB > 0)
					{
						PX_ASSERT(imm < numSkinClothMapB);
						PX_ASSERT(skinClothMapB[imm].vertexIndexPlusOffset == i);
					}
				}
			}
		}
#endif
	}

	return true;
}



class DeformableVerticesMaxDistancePredicate
{
public:
	DeformableVerticesMaxDistancePredicate(ClothingConstrainCoefficients* constrainCoefficients) : mConstrainCoefficients(constrainCoefficients) {}
	bool operator()(uint32_t oldindex1, uint32_t oldIndex2) const
	{
		return mConstrainCoefficients[oldindex1].maxDistance > mConstrainCoefficients[oldIndex2].maxDistance;
	}

private:
	ClothingConstrainCoefficients* mConstrainCoefficients;
};



bool ClothingAssetImpl::reorderDeformableVertices(ClothingPhysicalMeshImpl& physicalMesh)
{
	ClothingPhysicalMeshParameters* params = static_cast<ClothingPhysicalMeshParameters*>(physicalMesh.getNvParameterized());

	const uint32_t curSortingVersion = 1; // bump this number when the sorting changes!

	if (params->physicalMesh.physicalMeshSorting >= curSortingVersion)
	{
		// nothing needs to be done.
		return false;
	}

	params->physicalMesh.physicalMeshSorting = curSortingVersion;

	uint32_t* indices = physicalMesh.getIndicesBuffer();

	// create mapping arrays
	Array<uint32_t> newIndices(physicalMesh.getNumVertices(), (uint32_t) - 1);
	Array<uint32_t> oldIndices(physicalMesh.getNumVertices(), (uint32_t) - 1);
	uint32_t nextIndex = 0;
	for (uint32_t i = 0; i < physicalMesh.getNumIndices(); i++)
	{
		const uint32_t vertexIndex = indices[i];
		if (newIndices[vertexIndex] == (uint32_t) - 1)
		{
			newIndices[vertexIndex] = nextIndex;
			oldIndices[nextIndex] = vertexIndex;
			nextIndex++;
		}
	}

	uint32_t maxVertexIndex = 0;
	for (uint32_t j = 0; j < params->physicalMesh.numSimulatedIndices; j++)
	{
		const uint32_t newVertexIndex = newIndices[indices[j]];
		maxVertexIndex = PxMax(maxVertexIndex, newVertexIndex);
	}

	maxVertexIndex++;
	params->physicalMesh.numSimulatedVertices = maxVertexIndex;

	DeformableVerticesMaxDistancePredicate predicate(physicalMesh.getConstrainCoefficientBuffer());

	// Sort mesh.
	nvidia::sort(oldIndices.begin(), maxVertexIndex, predicate);

	// fix newIndices, the sort has destroyed them
	for (uint32_t i = 0; i < nextIndex; i++)
	{
		PX_ASSERT(newIndices[oldIndices[i]] != (uint32_t) - 1);
		newIndices[oldIndices[i]] = i;
	}

	// move unused vertices to the end
	if (nextIndex < physicalMesh.getNumVertices())
	{
		// TODO check if ApexPermute works without this
		for (uint32_t i = 0; i < newIndices.size(); i++)
		{
			if (newIndices[i] == (uint32_t) - 1)
			{
				newIndices[i] = nextIndex;
				oldIndices[nextIndex] = i;
				nextIndex++;
			}
		}
	}

	PX_ASSERT(physicalMesh.getNumVertices() == oldIndices.size());
	PX_ASSERT(nextIndex == physicalMesh.getNumVertices()); // at this point we assume that all vertices are referenced

	// do reordering
	physicalMesh.applyPermutation(oldIndices);

	// set max distance 0 vertices
	ClothingConstrainCoefficients* coeffs = physicalMesh.getConstrainCoefficientBuffer();

	const uint32_t numSimulatedVertices = params->physicalMesh.numSimulatedVertices;

	uint32_t vertexIndex = 0;
	float eps = 1e-8;
	for (; vertexIndex < numSimulatedVertices; vertexIndex++)
	{
		if (coeffs[vertexIndex].maxDistance <= eps)
		{
			break;
		}
	}

	params->physicalMesh.numMaxDistance0Vertices = numSimulatedVertices - vertexIndex;

	// safety
	for (; vertexIndex < numSimulatedVertices; vertexIndex++)
	{
		PX_ASSERT(coeffs[vertexIndex].maxDistance <= eps);
	}


	// clean up existing references
	for (uint32_t i = 0; i < physicalMesh.getNumIndices(); i++)
	{
		indices[i] = newIndices[indices[i]];
	}

	// update mappings into deformable vertex buffer
	for (uint32_t i = 0; i < mGraphicalLods.size(); i++)
	{
		if (mPhysicalMeshes[mGraphicalLods[i]->physicalMeshId] == physicalMesh.getNvParameterized())
		{
			ClothingGraphicalMeshAssetWrapper meshAsset(reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[i]->renderMeshAssetPointer));
			ClothingGraphicalLodParameters& graphicalLod = *mGraphicalLods[i];

			const uint32_t numGraphicalVertices = meshAsset.getNumTotalVertices();

			if (graphicalLod.immediateClothMap.arraySizes[0] > 0)
			{
				for (uint32_t j = 0; j < numGraphicalVertices; j++)
				{
					if (graphicalLod.immediateClothMap.buf[j] != ClothingConstants::ImmediateClothingInvalidValue)
					{
						if ((graphicalLod.immediateClothMap.buf[j] & ClothingConstants::ImmediateClothingInSkinFlag) == 0)
						{
							const uint32_t flags = graphicalLod.immediateClothMap.buf[j] & ~ClothingConstants::ImmediateClothingReadMask;
							graphicalLod.immediateClothMap.buf[j] =
							    newIndices[graphicalLod.immediateClothMap.buf[j] & ClothingConstants::ImmediateClothingReadMask] | flags;
						}
					}
				}
			}

			for(int32_t j = 0; j < graphicalLod.skinClothMap.arraySizes[0]; ++j)
			{
				graphicalLod.skinClothMap.buf[j].vertexIndex0 = newIndices[graphicalLod.skinClothMap.buf[j].vertexIndex0];
				graphicalLod.skinClothMap.buf[j].vertexIndex1 = newIndices[graphicalLod.skinClothMap.buf[j].vertexIndex1];
				graphicalLod.skinClothMap.buf[j].vertexIndex2 = newIndices[graphicalLod.skinClothMap.buf[j].vertexIndex2];
			}
		}
	}

	// update transition maps

	if (params->transitionDown.arraySizes[0] > 0)
	{
		for (int32_t i = 0; i < params->transitionDown.arraySizes[0]; i++)
		{
			uint32_t& vertexIndex = params->transitionDown.buf[i].vertexIndexPlusOffset;
			PX_ASSERT(vertexIndex == (uint32_t)i);
			vertexIndex = newIndices[vertexIndex];
		}

		nvidia::sort(params->transitionDown.buf, (uint32_t)params->transitionDown.arraySizes[0], SkinClothMapPredicate<ClothingPhysicalMeshParametersNS::SkinClothMapD_Type>());
	}

	if (params->transitionUp.arraySizes[0] > 0)
	{
		for (int32_t i = 0; i < params->transitionUp.arraySizes[0]; i++)
		{
			uint32_t& vertexIndex = params->transitionUp.buf[i].vertexIndexPlusOffset;
			PX_ASSERT(vertexIndex == (uint32_t)i);
			vertexIndex = newIndices[vertexIndex];
		}
		nvidia::sort(params->transitionUp.buf, (uint32_t)params->transitionUp.arraySizes[0], SkinClothMapPredicate<ClothingPhysicalMeshParametersNS::SkinClothMapD_Type>());
	}

	return true;
}



float ClothingAssetImpl::getMaxMaxDistance(ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh, uint32_t index, uint32_t numIndices) const
{
	uint32_t* indices = physicalMesh.indices.buf;
	ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type* coeffs = physicalMesh.constrainCoefficients.buf;

	float maxDist = coeffs[indices[index]].maxDistance;
	for (uint32_t i = 1; i < numIndices; i++)
	{
		maxDist = PxMax(maxDist, coeffs[indices[index + i]].maxDistance);
	}

	return maxDist;
}



uint32_t ClothingAssetImpl::getCorrespondingPhysicalVertices(const ClothingGraphicalLodParameters& graphLod, uint32_t submeshIndex,
													uint32_t graphicalVertexIndex, const AbstractMeshDescription& pMesh,
													uint32_t submeshVertexOffset, uint32_t indices[4], float trust[4]) const
{
	PX_UNUSED(submeshIndex); // stupid release mode

	PX_ASSERT(pMesh.numIndices > 0);
	PX_ASSERT(pMesh.pIndices != NULL);


	uint32_t result = 0;

	if (graphLod.immediateClothMap.arraySizes[0] > 0)
	{
		indices[0] = graphLod.immediateClothMap.buf[graphicalVertexIndex + submeshVertexOffset];
		trust[0] = 1.0f;

		if (indices[0] != ClothingConstants::ImmediateClothingInvalidValue)
		{
			if ((indices[0] & ClothingConstants::ImmediateClothingInSkinFlag) == 0)
			{
				indices[0] &= ClothingConstants::ImmediateClothingReadMask;
				result = 1;
			}
			else if (graphLod.skinClothMapB.arraySizes[0] > 0)
			{
				const int32_t temp = int32_t(indices[0] & ClothingConstants::ImmediateClothingReadMask);
				if (temp < graphLod.skinClothMapB.arraySizes[0])
				{
					for (uint32_t i = 0; i < 3; i++)
					{
						PX_ASSERT(graphLod.skinClothMapB.buf[temp].faceIndex0 + i < pMesh.numIndices);
						indices[i] = pMesh.pIndices[graphLod.skinClothMapB.buf[temp].faceIndex0 + i];
						trust[i] = 1.0f; // PH: This should be lower for some vertices!
					}
					result = 3;
				}
			}
			else if (graphLod.skinClothMap.arraySizes[0] > 0)
			{
				const int32_t temp = int32_t(indices[0] & ClothingConstants::ImmediateClothingReadMask);
				if (temp < graphLod.skinClothMap.arraySizes[0])
				{
					PxVec3 bary = graphLod.skinClothMap.buf[temp].vertexBary;
					bary.x = PxClamp(bary.x, 0.0f, 1.0f);
					bary.y = PxClamp(bary.y, 0.0f, 1.0f);
					bary.z = PxClamp(1.0f - bary.x - bary.y, 0.0f, 1.0f);
					uint32_t physVertIndex[3] = 
					{
						graphLod.skinClothMap.buf[temp].vertexIndex0,
						graphLod.skinClothMap.buf[temp].vertexIndex1,
						graphLod.skinClothMap.buf[temp].vertexIndex2
					};
					for (uint32_t i = 0; i < 3; i++)
					{
						//PX_ASSERT(graphLod.skinClothMap.buf[temp].faceIndex0 + i < pMesh.numIndices);
						indices[i] = physVertIndex[i];
						trust[i] = bary[i];
					}
					result = 3;
				}
			}
		}
	}
	else if (graphLod.skinClothMapB.arraySizes[0] > 0)
	{
		PX_ASSERT(graphLod.skinClothMapB.buf[graphicalVertexIndex + submeshVertexOffset].submeshIndex == submeshIndex);
		PX_ASSERT(graphLod.skinClothMapB.buf[graphicalVertexIndex + submeshVertexOffset].vertexIndexPlusOffset == graphicalVertexIndex + submeshVertexOffset);

		for (uint32_t i = 0; i < 3; i++)
		{
			PX_ASSERT(graphLod.skinClothMapB.buf[graphicalVertexIndex + submeshVertexOffset].faceIndex0 + i < pMesh.numIndices);
			indices[i] = pMesh.pIndices[graphLod.skinClothMapB.buf[graphicalVertexIndex + submeshVertexOffset].faceIndex0 + i];
			trust[i] = 1.0f;
		}
		result = 3;
	}
	else if (graphLod.skinClothMap.arraySizes[0] > 0)
	{
		// we need to do binary search here
		uint32_t curMin = 0;
		uint32_t curMax = (uint32_t)graphLod.skinClothMap.arraySizes[0];
		const uint32_t searchFor = graphicalVertexIndex + submeshVertexOffset;
		while (curMax > curMin)
		{
			uint32_t middle = (curMin + curMax) >> 1;
			PX_ASSERT(middle == graphLod.skinClothMap.buf[middle].vertexIndexPlusOffset);
			const uint32_t probeResult = middle;
			if (probeResult < searchFor)
			{
				curMin = middle + 1;
			}
			else
			{
				curMax = middle;
			}
		}

		PX_ASSERT(curMin == graphLod.skinClothMap.buf[curMin].vertexIndexPlusOffset);
		if (curMin == searchFor)
		{
			PxVec3 bary = graphLod.skinClothMap.buf[curMin].vertexBary;
			bary.x = PxClamp(bary.x, 0.0f, 1.0f);
			bary.y = PxClamp(bary.y, 0.0f, 1.0f);
			bary.z = PxClamp(1.0f - bary.x - bary.y, 0.0f, 1.0f);

			uint32_t physVertIndex[3] = 
			{
				graphLod.skinClothMap.buf[curMin].vertexIndex0,
				graphLod.skinClothMap.buf[curMin].vertexIndex1,
				graphLod.skinClothMap.buf[curMin].vertexIndex2
			};
			for (uint32_t i = 0; i < 3; i++)
			{
			//	PX_ASSERT(graphLod.skinClothMap.buf[curMin].faceIndex0 + i < pMesh.numIndices);
				indices[i] = physVertIndex[i];
				trust[i] = bary[i];
			}
			result = 3;
		}
	}
	else if (graphLod.tetraMap.arraySizes[0] > 0)
	{
		for (uint32_t i = 0; i < 4; i++)
		{
			PX_ASSERT(graphLod.tetraMap.buf[graphicalVertexIndex + submeshVertexOffset].tetraIndex0 + i < pMesh.numIndices);
			indices[i] = pMesh.pIndices[graphLod.tetraMap.buf[graphicalVertexIndex + submeshVertexOffset].tetraIndex0 + i];
			trust[i] = 1.0f;
		}
		result = 4;
	}

	for (uint32_t i = 0; i < result; i++)
	{
		PX_ASSERT(trust[i] >= 0.0f);
		PX_ASSERT(trust[i] <= 1.0f);
	}

	return result;
}



void ClothingAssetImpl::getNormalsAndVerticesForFace(PxVec3* vtx, PxVec3* nrm, uint32_t i, const AbstractMeshDescription& srcPM) const
{
	// copy indices for convenience
	PX_ASSERT(i < srcPM.numIndices);
	uint32_t di[3];
	for (uint32_t j = 0; j < 3; j++)
	{
		di[j] = srcPM.pIndices[i + j];
	}

	// To guarantee consistency in our implicit tetrahedral mesh definition we must always order vertices
	// idx[0,1,2] = min, max and mid
	uint32_t idx[3];
	idx[0] = PxMin(di[0], PxMin(di[1], di[2]));
	idx[1] = PxMax(di[0], PxMax(di[1], di[2]));
	idx[2] = idx[0];
	for (uint32_t j = 0; j < 3; j++)
	{
		if ((idx[0] != di[j]) && (idx[1] != di[j]))
		{
			idx[2] = di[j];
		}
	}

	for (uint32_t j = 0; j < 3; j++)
	{
		vtx[j] = srcPM.pPosition[idx[j]];
		nrm[j] = srcPM.pNormal[idx[j]];
#ifdef _DEBUG
		// sanity
		// PH: These normals 'should' always be normalized, maybe we can get rid of the normalize completely!
		const float length = nrm[j].magnitudeSquared();
		if (!(length >= 0.99f && length <= 1.01f))
		{
			static bool first = true;
			if (first)
			{
				PX_ALWAYS_ASSERT();
				first = false;
			}
		}
#else
		// PH: let's try and disable it in release mode...
		//nrm[j].normalize();
#endif
	};
}



bool ClothingAssetImpl::setBoneName(uint32_t internalIndex, const char* name)
{
	NvParameterized::Handle bonesHandle(*mParams);
	mParams->getParameterHandle("bones", bonesHandle);

	if (bonesHandle.isValid())
	{
		NvParameterized::Handle boneHandle(*mParams);
		bonesHandle.getChildHandle((int32_t)internalIndex, boneHandle);

		if (boneHandle.isValid())
		{
			NvParameterized::Handle nameHandle(*mParams);
			boneHandle.getChildHandle(mParams, "name", nameHandle);

			if (nameHandle.isValid())
			{
				mParams->setParamString(nameHandle, name);
				return true;
			}
		}
	}
	return false;
}



void ClothingAssetImpl::clearMapping(uint32_t graphicalLodIndex)
{
	if (mGraphicalLods[graphicalLodIndex]->immediateClothMap.buf != NULL)
	{
		ParamArray<uint32_t> immediateClothMap(mGraphicalLods[graphicalLodIndex], "immediateClothMap", reinterpret_cast<ParamDynamicArrayStruct*>(&mGraphicalLods[graphicalLodIndex]->immediateClothMap));
		immediateClothMap.clear();
	}
	if (mGraphicalLods[graphicalLodIndex]->skinClothMapB.buf != NULL)
	{
		ParamArray<ClothingGraphicalLodParametersNS::SkinClothMapB_Type> skinClothMapB(mGraphicalLods[graphicalLodIndex], "skinClothMapB", reinterpret_cast<ParamDynamicArrayStruct*>(&mGraphicalLods[graphicalLodIndex]->skinClothMapB));
		skinClothMapB.clear();
	}
	if (mGraphicalLods[graphicalLodIndex]->tetraMap.buf != NULL)
	{
		ParamArray<ClothingGraphicalLodParametersNS::TetraLink_Type> tetraMap(mGraphicalLods[graphicalLodIndex], "tetraMap", reinterpret_cast<ParamDynamicArrayStruct*>(&mGraphicalLods[graphicalLodIndex]->tetraMap));
		tetraMap.clear();
	}
}



bool ClothingAssetImpl::findTriangleForImmediateVertex(uint32_t& faceIndex, uint32_t& indexInTriangle, uint32_t physVertIndex, ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh) const
{
	// find triangle with smallest faceIndex, indices have been sorted during authoring
	// such that simulated triangles are first
	for (uint32_t physIndex = 0; physIndex < (uint32_t)physicalMesh.indices.arraySizes[0]; physIndex++)
	{
		if (physicalMesh.indices.buf[physIndex] == physVertIndex)
		{
			// this is a triangle that contains the vertex from the immediate map)
			uint32_t currentFaceIndex = physIndex - (physIndex%3);

			faceIndex = currentFaceIndex;
			indexInTriangle = physIndex - faceIndex;
			return true;
		}
	}

	return false;
}



// we can't just regenerate the skinClothMap, because master/slave info is only available during authoring
bool ClothingAssetImpl::mergeMapping(ClothingGraphicalLodParameters* graphicalLod)
{
	if (graphicalLod->immediateClothMap.buf == NULL)
		return false;

	ParamArray<uint32_t> immediateMap(graphicalLod, "immediateClothMap", reinterpret_cast<ParamDynamicArrayStruct*>(&graphicalLod->immediateClothMap));

	// size of immediateMap (equals number of graphical vertices)
	uint32_t immediateCount = (uint32_t)graphicalLod->immediateClothMap.arraySizes[0];

	ParamArray<SkinClothMap> skinClothMap(graphicalLod, "skinClothMap",
		reinterpret_cast<ParamDynamicArrayStruct*>(&graphicalLod->skinClothMap));

	uint32_t oldSkinMapSize = skinClothMap.size();
	skinClothMap.resize(immediateCount);

	SkinClothMap* mapEntry = &skinClothMap[oldSkinMapSize];

	// get RenderMeshAsset
	ClothingGraphicalMeshAssetWrapper renderMesh(reinterpret_cast<RenderMeshAssetIntl*>(graphicalLod->renderMeshAssetPointer));
	uint32_t numSubmeshes = renderMesh.getSubmeshCount();

	ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh = &mPhysicalMeshes[graphicalLod->physicalMeshId]->physicalMesh;
	PX_ASSERT(physicalMesh != NULL);

	// some updates for the case where we didn't have a skinClothMap yet
	if (graphicalLod->skinClothMapThickness == 0.0f)
	{
		graphicalLod->skinClothMapThickness = 1.0f;
	}
	if (graphicalLod->skinClothMapOffset == 0.0f)
	{
		graphicalLod->skinClothMapOffset = DEFAULT_PM_OFFSET_ALONG_NORMAL_FACTOR * physicalMesh->averageEdgeLength;
	}

	uint32_t invalidCount = 0;

	// iterate through immediate map, append verts to skinClothMap that are not yet in there
	uint32_t mapVertIndex = 0;
	for (uint32_t submeshIndex = 0; submeshIndex < numSubmeshes; ++submeshIndex)
	{
		RenderDataFormat::Enum outFormat;
		const PxVec4* tangents = (const PxVec4*)renderMesh.getVertexBuffer(submeshIndex, RenderVertexSemantic::TANGENT, outFormat);
		PX_ASSERT(tangents == NULL || outFormat == RenderDataFormat::FLOAT4);

		const PxVec3* positions = (const PxVec3*)renderMesh.getVertexBuffer(submeshIndex, RenderVertexSemantic::POSITION, outFormat);
		PX_ASSERT(positions == NULL || outFormat == RenderDataFormat::FLOAT3);

		const PxVec3* normals = (const PxVec3*)renderMesh.getVertexBuffer(submeshIndex, RenderVertexSemantic::NORMAL, outFormat);
		PX_ASSERT(normals == NULL || outFormat == RenderDataFormat::FLOAT3);

		for (uint32_t submeshVertIndex = 0; submeshVertIndex < renderMesh.getNumVertices(submeshIndex); ++submeshVertIndex, ++mapVertIndex)
		{
			PX_ASSERT(mapVertIndex < immediateCount);

			uint32_t physVertIndex = 0;
			if (immediateMap[mapVertIndex] == ClothingConstants::ImmediateClothingInvalidValue)
			{
				++invalidCount;
				continue;
			}

			physVertIndex = immediateMap[mapVertIndex] & ClothingConstants::ImmediateClothingReadMask;

			if ((immediateMap[mapVertIndex] & ClothingConstants::ImmediateClothingInSkinFlag) > 0)
			{
				// in that case physVertIndex is the index in the skinClothMap.
				// set it to the current index, so we can sort it afterwards
				skinClothMap[physVertIndex].vertexIndexPlusOffset = mapVertIndex;
				continue;
			}
			
			// find triangle mapping
			uint32_t faceIndex = UINT32_MAX;
			uint32_t indexInTriangle = UINT32_MAX;
			findTriangleForImmediateVertex(faceIndex, indexInTriangle, physVertIndex, *physicalMesh);

			mapEntry->vertexIndex0 = physicalMesh->indices.buf[faceIndex + 0];
			mapEntry->vertexIndex1 = physicalMesh->indices.buf[faceIndex + 1];
			mapEntry->vertexIndex2 = physicalMesh->indices.buf[faceIndex + 2];

			// for immediate skinned verts
			// position, normal and tangent all have the same barycentric coord on the triangle
			PxVec3 bary(0.0f);
			if (indexInTriangle < 2)
			{
				bary[indexInTriangle] = 1.0f;
			}

			mapEntry->vertexBary = bary;
			// mapEntry->vertexBary.z = 0 because it's on the triangle

			// offset the normal
			mapEntry->normalBary = bary;
			mapEntry->normalBary.z = graphicalLod->skinClothMapOffset;

			// we need to compute tangent bary's because there are no tangents on physical mesh
			bary.x = bary.y = bary.z = UINT32_MAX;
			if (positions != NULL && normals != NULL && tangents != NULL)
			{
				PxVec3 dummy(0.0f);
				PxVec3 position = positions[submeshVertIndex];
				PxVec3 tangent = tangents[submeshVertIndex].getXYZ();

				// prepare triangle data
				TriangleWithNormals triangle;
				triangle.valid = 0;
				triangle.faceIndex0 = faceIndex;
				PxVec3* physNormals = physicalMesh->skinningNormals.buf;
				if (physNormals == NULL)
				{
					physNormals = physicalMesh->normals.buf;
				}
				PX_ASSERT(physNormals != NULL);
				for (uint32_t j = 0; j < 3; j++)
				{
					uint32_t triVertIndex = physicalMesh->indices.buf[triangle.faceIndex0 + j];
					triangle.vertices[j] = physicalMesh->vertices.buf[triVertIndex];
					triangle.normals[j] = physNormals[triVertIndex];
				}
				triangle.init();

				ModuleClothingHelpers::computeTriangleBarys(triangle, dummy, dummy, position + tangent, graphicalLod->skinClothMapOffset, 0, true);
				mapEntry->tangentBary = triangle.tempBaryTangent;
			}

			mapEntry->vertexIndexPlusOffset = mapVertIndex;
			mapEntry++;
		}
	}

	skinClothMap.resize(skinClothMap.size() - invalidCount);

	// make sure skinClothMap is sorted by graphical vertex order
	nvidia::sort(graphicalLod->skinClothMap.buf, (uint32_t)graphicalLod->skinClothMap.arraySizes[0], SkinClothMapPredicate<ClothingGraphicalLodParametersNS::SkinClothMapD_Type>());

	immediateMap.clear();

	// notify actors of the asset change
	for (uint32_t i = 0; i < getNumActors(); ++i)
	{
		ClothingActorImpl& actor = DYNAMIC_CAST(ClothingActorProxy*)(mActors.getResource(i))->impl;
		actor.reinitActorData();
	}

	return true;
}


void ClothingAssetImpl::updateBoundingBox()
{
	PxBounds3 tempBounds;
	tempBounds.setEmpty();

	for (uint32_t graphicalMeshId = 0; graphicalMeshId < mGraphicalLods.size(); graphicalMeshId++)
	{
		if (mGraphicalLods[graphicalMeshId]->renderMeshAssetPointer == NULL)
			continue;

		RenderMeshAssetIntl* renderMeshAsset = reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[graphicalMeshId]->renderMeshAssetPointer);
		for (uint32_t submeshIndex = 0; submeshIndex < renderMeshAsset->getSubmeshCount(); submeshIndex++)
		{
			const VertexBuffer& vb = renderMeshAsset->getInternalSubmesh(submeshIndex).getVertexBuffer();
			const VertexFormat& vf = vb.getFormat();
			uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(nvidia::apex::RenderVertexSemantic::POSITION));
			RenderDataFormat::Enum positionFormat;
			const PxVec3* pos = (const PxVec3*)vb.getBufferAndFormat(positionFormat, bufferIndex);
			PX_ASSERT(positionFormat == nvidia::RenderDataFormat::FLOAT3);
			const uint32_t numVertices = renderMeshAsset->getInternalSubmesh(submeshIndex).getVertexCount(0);
			for (uint32_t i = 0; i < numVertices; i++)
			{
				tempBounds.include(pos[i]);
			}
		}
	}
	for (uint32_t i = 0; i < mPhysicalMeshes.size(); i++)
	{
		AbstractMeshDescription other;
		other.pPosition = mPhysicalMeshes[i]->physicalMesh.vertices.buf;

		for (uint32_t j = 0; j < other.numVertices; j++)
		{
			tempBounds.include(other.pPosition[j]);
		}
	}

	mParams->boundingBox = tempBounds;
}



float ClothingAssetImpl::getMaxDistReduction(ClothingPhysicalMeshParameters& physicalMesh, float maxDistanceMultiplier) const
{
	return physicalMesh.physicalMesh.maximumMaxDistance * (1.0f - maxDistanceMultiplier);
}



#ifndef WITHOUT_PVD
void ClothingAssetImpl::initPvdInstances(pvdsdk::PvdDataStream& pvdStream)
{
	ApexResourceInterface* pvdInstance = static_cast<ApexResourceInterface*>(this);

	// Asset Params
	pvdStream.createInstance(pvdsdk::NamespacedName(APEX_PVD_NAMESPACE, "ClothingAssetParameters"), mParams);
	pvdStream.setPropertyValue(pvdInstance, "AssetParams", pvdsdk::DataRef<const uint8_t>((const uint8_t*)&mParams, sizeof(ClothingAssetParameters*)), pvdsdk::getPvdNamespacedNameForType<pvdsdk::ObjectRef>());

	// update asset param properties (should we do this per frame? if so, how?)
	pvdsdk::ApexPvdClient* client = GetInternalApexSDK()->getApexPvdClient();
	PX_ASSERT(client != NULL);
	client->updatePvd(mParams, *mParams);
	
	mActors.initPvdInstances(pvdStream);
}



void ClothingAssetImpl::destroyPvdInstances()
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
					client->updatePvd(mParams, *mParams, pvdsdk::PvdAction::DESTROY);
					pvdStream->destroyInstance(mParams);
				}
			}
		}
	}
}
#endif

}
} // namespace nvidia


