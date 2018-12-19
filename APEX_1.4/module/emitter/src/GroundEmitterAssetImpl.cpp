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

#include "ApexSDKIntl.h"
#include "GroundEmitterAsset.h"
#include "GroundEmitterPreview.h"
#include "IofxAsset.h"
#include "GroundEmitterAssetImpl.h"
#include "GroundEmitterActorImpl.h"
#include "GroundEmitterAssetPreview.h"
//#include "ApexSharedSerialization.h"
#include "EmitterScene.h"
#include "nvparameterized/NvParamUtils.h"

namespace nvidia
{
namespace emitter
{

void GroundEmitterAssetImpl::copyLodDesc(EmitterLodParamDesc& dst, const GroundEmitterAssetParametersNS::emitterLodParamDesc_Type& src)
{
	if (src.version != dst.current)
	{
		APEX_DEBUG_WARNING("EmitterLodParamDesc version mismatch");
	}
	dst.bias			= src.bias;
	dst.distanceWeight	= src.distanceWeight;
	dst.lifeWeight		= src.lifeWeight;
	dst.maxDistance		= src.maxDistance;
	dst.separationWeight = src.separationWeight;
	dst.speedWeight		= src.speedWeight;
}

void GroundEmitterAssetImpl::copyLodDesc(GroundEmitterAssetParametersNS::emitterLodParamDesc_Type& dst, const EmitterLodParamDesc& src)
{
	dst.version			= src.current;
	dst.bias			= src.bias;
	dst.distanceWeight	= src.distanceWeight;
	dst.lifeWeight		= src.lifeWeight;
	dst.maxDistance		= src.maxDistance;
	dst.separationWeight = src.separationWeight;
	dst.speedWeight		= src.speedWeight;
}


void GroundEmitterAssetImpl::postDeserialize(void* userData_)
{
	PX_UNUSED(userData_);

	/* Resolve the authored collision group name into the actual ID */
	if (mParams->raycastCollisionGroupMaskName != NULL &&
	        mParams->raycastCollisionGroupMaskName[0] != 0)
	{
		ResourceProviderIntl* nrp = mModule->mSdk->getInternalResourceProvider();
		ResID cgmns128 = mModule->mSdk->getCollisionGroup128NameSpace();
		ResID cgmresid128 = nrp->createResource(cgmns128, mParams->raycastCollisionGroupMaskName);
		void* tmpCGM = nrp->getResource(cgmresid128);
		if (tmpCGM)
		{
			mRaycastCollisionGroupsMask = *(static_cast<physx::PxFilterData*>(tmpCGM));
			mShouldUseGroupsMask = true;
		}
	}
	else
	{
		mRaycastCollisionGroups = 0xFFFFFFFF;
	}

	initializeAssetNameTable();
}

void GroundEmitterAssetImpl::initializeAssetNameTable()
{
	/* initialize the iofx and ios asset name to resID tables */
	for (uint32_t i = 0; i < (*mMaterialFactoryMaps).size(); i++)
	{
		mIofxAssetTracker.addAssetName((*mMaterialFactoryMaps)[i].iofxAssetName->name(), false);
		mIosAssetTracker.addAssetName((*mMaterialFactoryMaps)[i].iosAssetName->className(),
		                              (*mMaterialFactoryMaps)[i].iosAssetName->name());
	}
}

uint32_t GroundEmitterAssetImpl::forceLoadAssets()
{
	uint32_t assetLoadedCount = 0;

	assetLoadedCount += mIofxAssetTracker.forceLoadAssets();
	assetLoadedCount += mIosAssetTracker.forceLoadAssets();

	return assetLoadedCount;
}

GroundEmitterAssetImpl::GroundEmitterAssetImpl(ModuleEmitterImpl* m, ResourceList& list, const char* name) :
	mIofxAssetTracker(m->mSdk, IOFX_AUTHORING_TYPE_NAME),
	mIosAssetTracker(m->mSdk),
	mModule(m),
	mName(name),
	mDefaultActorParams(NULL),
	mDefaultPreviewParams(NULL),
	mShouldUseGroupsMask(false)
{
	using namespace GroundEmitterAssetParametersNS;

	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
	mParams = (GroundEmitterAssetParameters*)traits->createNvParameterized(GroundEmitterAssetParameters::staticClassName());

	PX_ASSERT(mParams);

	mMaterialFactoryMaps =
	    PX_NEW(ParamArray<materialFactoryMapping_Type>)(mParams,
	            "materialFactoryMapList",
	            (ParamDynamicArrayStruct*) & (mParams->materialFactoryMapList));
	list.add(*this);
}

GroundEmitterAssetImpl::GroundEmitterAssetImpl(ModuleEmitterImpl* m,
                                       ResourceList& list,
                                       NvParameterized::Interface* params,
                                       const char* name) :
	mIofxAssetTracker(m->mSdk, IOFX_AUTHORING_TYPE_NAME),
	mIosAssetTracker(m->mSdk),
	mModule(m),
	mName(name),
	mParams((GroundEmitterAssetParameters*)params),
	mDefaultActorParams(NULL),
	mDefaultPreviewParams(NULL),
	mShouldUseGroupsMask(false)
{
	using namespace GroundEmitterAssetParametersNS;

	mMaterialFactoryMaps =
	    PX_NEW(ParamArray<materialFactoryMapping_Type>)(mParams,
	            "materialFactoryMapList",
	            (ParamDynamicArrayStruct*) & (mParams->materialFactoryMapList));
	PX_ASSERT(mMaterialFactoryMaps);

	postDeserialize();

	list.add(*this);
}


void GroundEmitterAssetImpl::destroy()
{
	/* Assets that were forceloaded or loaded by actors will be automatically
	 * released by the ApexAssetTracker member destructors.
	 */

	delete mMaterialFactoryMaps;
	if (mParams)
	{
		mParams->destroy();
		mParams = NULL;
	}

	if (mDefaultActorParams)
	{
		mDefaultActorParams->destroy();
		mDefaultActorParams = 0;
	}

	if (mDefaultPreviewParams)
	{
		mDefaultPreviewParams->destroy();
		mDefaultPreviewParams = 0;
	}

	delete this;
}

GroundEmitterAssetImpl::~GroundEmitterAssetImpl()
{
}

NvParameterized::Interface* GroundEmitterAssetImpl::getDefaultActorDesc()
{
	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
	PX_ASSERT(traits);
	if (!traits)
	{
		return NULL;
	}

	// create if not yet created
	if (!mDefaultActorParams)
	{
		const char* className = GroundEmitterActorParameters::staticClassName();
		NvParameterized::Interface* param = traits->createNvParameterized(className);
		mDefaultActorParams = static_cast<GroundEmitterActorParameters*>(param);

		PX_ASSERT(param);
		if (!param)
		{
			return NULL;
		}
	}

	// copy the parameters from the asset parameters
	mDefaultActorParams->density			= mParams->density;
	mDefaultActorParams->radius				= mParams->radius;
	mDefaultActorParams->raycastHeight		= mParams->raycastHeight;
	mDefaultActorParams->spawnHeight		= mParams->spawnHeight;
	mDefaultActorParams->maxRaycastsPerFrame = mParams->maxRaycastsPerFrame;

	NvParameterized::setParamString(*mDefaultActorParams, "raycastCollisionGroupMaskName", mParams->raycastCollisionGroupMaskName);

	return mDefaultActorParams;
}

NvParameterized::Interface* GroundEmitterAssetImpl::getDefaultAssetPreviewDesc()
{
	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
	PX_ASSERT(traits);
	if (!traits)
	{
		return NULL;
	}

	// create if not yet created
	if (!mDefaultPreviewParams)
	{
		const char* className = EmitterAssetPreviewParameters::staticClassName();
		NvParameterized::Interface* param = traits->createNvParameterized(className);
		mDefaultPreviewParams = static_cast<EmitterAssetPreviewParameters*>(param);

		PX_ASSERT(param);
		if (!param)
		{
			return NULL;
		}
	}

	return mDefaultPreviewParams;
}

Actor* GroundEmitterAssetImpl::createApexActor(const NvParameterized::Interface& parms, Scene& apexScene)
{
	if (!isValidForActorCreation(parms, apexScene))
	{
		return NULL;
	}

	Actor* ret = 0;

	const char* className = parms.className();
	if (nvidia::strcmp(className, GroundEmitterActorParameters::staticClassName()) == 0)
	{
		GroundEmitterActorDesc desc;
		const GroundEmitterActorParameters* pDesc = static_cast<const GroundEmitterActorParameters*>(&parms);

		desc.density				= pDesc->density;
		desc.radius					= pDesc->radius;
		desc.raycastHeight			= pDesc->raycastHeight;
		desc.spawnHeight			= pDesc->spawnHeight;
		desc.maxRaycastsPerFrame	= pDesc->maxRaycastsPerFrame;
		desc.attachRelativePosition	= pDesc->attachRelativePosition;
		desc.initialPosition		= pDesc->globalPose.p;
		desc.rotation				= pDesc->globalPose;
		desc.rotation.p = PxVec3(0.0f);

		/* Resolve the authored collision group mask name into the actual ID */

		ret = createActor(desc, apexScene);

		/* Resolve the authored collision group name into the actual ID */
		physx::PxFilterData* raycastGroupsMask = 0;
		if (pDesc->raycastCollisionGroupMaskName != NULL &&
		        pDesc->raycastCollisionGroupMaskName[0] != 0)
		{
			ResourceProviderIntl* nrp = mModule->mSdk->getInternalResourceProvider();
			ResID cgmns = mModule->mSdk->getCollisionGroup128NameSpace();
			ResID cgresid = nrp->createResource(cgmns, pDesc->raycastCollisionGroupMaskName);
			raycastGroupsMask = static_cast<physx::PxFilterData*>(nrp->getResource(cgresid));
		}
		else if (mShouldUseGroupsMask)
		{
			raycastGroupsMask = &mRaycastCollisionGroupsMask;
		}

		// check the physx::PxFilterData specified in parms, set in actor if diff than default
		if (raycastGroupsMask && ret )
		{
			GroundEmitterActor* gea = static_cast<GroundEmitterActor*>(ret);
			gea->setRaycastCollisionGroupsMask(raycastGroupsMask);
		}
	}

	return ret;
}


AssetPreview* GroundEmitterAssetImpl::createApexAssetPreview(const NvParameterized::Interface& parms, AssetPreviewScene* previewScene)
{
	AssetPreview* ret = 0;

	const char* className = parms.className();
	if (nvidia::strcmp(className, EmitterAssetPreviewParameters::staticClassName()) == 0)
	{
		GroundEmitterPreviewDesc desc;
		const EmitterAssetPreviewParameters* pDesc = static_cast<const EmitterAssetPreviewParameters*>(&parms);

		desc.mPose	= pDesc->pose;
		desc.mScale = pDesc->scale;

		ret = createEmitterPreview(desc, previewScene);
	}

	return ret;
}


GroundEmitterActor* GroundEmitterAssetImpl::createActor(const GroundEmitterActorDesc& desc, Scene& scene)
{
	if (!desc.isValid())
	{
		return NULL;
	}
	EmitterScene* es = mModule->getEmitterScene(scene);
	GroundEmitterActorImpl* ret = PX_NEW(GroundEmitterActorImpl)(desc, *this, mEmitterActors, *es);

	if (!ret->isValid())
	{
		ret->destroy();
		return NULL;
	}

	return ret;
}

void GroundEmitterAssetImpl::releaseActor(GroundEmitterActor& nxactor)
{
	GroundEmitterActorImpl* actor = DYNAMIC_CAST(GroundEmitterActorImpl*)(&nxactor);
	actor->destroy();
}

GroundEmitterPreview* GroundEmitterAssetImpl::createEmitterPreview(const GroundEmitterPreviewDesc& desc, AssetPreviewScene* previewScene)
{
	if (!desc.isValid())
	{
		return NULL;
	}

	GroundEmitterAssetPreview* p = PX_NEW(GroundEmitterAssetPreview)(desc, *this, GetApexSDK(), previewScene);
	if (p && !p->isValid())
	{
		p->destroy();
		p = NULL;
	}
	return p;
}

void GroundEmitterAssetImpl::releaseEmitterPreview(GroundEmitterPreview& nxpreview)
{
	GroundEmitterAssetPreview* preview = DYNAMIC_CAST(GroundEmitterAssetPreview*)(&nxpreview);
	preview->destroy();
}


void GroundEmitterAssetImpl::release()
{
	mModule->mSdk->releaseAsset(*this);
}

#ifndef WITHOUT_APEX_AUTHORING
void GroundEmitterAssetAuthoringImpl::release()
{
	mModule->mSdk->releaseAssetAuthoring(*this);
}

void GroundEmitterAssetAuthoringImpl::addMeshForGroundMaterial(const MaterialFactoryMappingDesc& desc)
{
	NvParameterized::Handle arrayHandle(*mParams), indexHandle(*mParams), childHandle(*mParams);
	NvParameterized::Interface* refPtr;
	// resize map array
	uint32_t newIdx = (*mMaterialFactoryMaps).size();
	(*mMaterialFactoryMaps).resize(newIdx + 1);

	// copy new desc in place
	// floats
	(*mMaterialFactoryMaps)[ newIdx ].weight = desc.weight;
	(*mMaterialFactoryMaps)[ newIdx ].maxSlopeAngle = desc.maxSlopeAngle;

	// lod params
	copyLodDesc((*mMaterialFactoryMaps)[ newIdx ].lodParamDesc, mCurLodParamDesc);

	// strings
	// get a handle for the material factory map for this index into the dynamic array
	mParams->getParameterHandle("materialFactoryMapList", arrayHandle);
	arrayHandle.getChildHandle((int32_t)newIdx, indexHandle);

	indexHandle.getChildHandle(mParams, "iofxAssetName", childHandle);
	mParams->initParamRef(childHandle, NULL, true);
	mParams->getParamRef(childHandle, refPtr);
	PX_ASSERT(refPtr);
	if (refPtr)
	{
		refPtr->setName(desc.instancedObjectEffectsAssetName);
	}

	indexHandle.getChildHandle(mParams, "iosAssetName", childHandle);
	mParams->initParamRef(childHandle, desc.instancedObjectSimulationTypeName, true);
	mParams->getParamRef(childHandle, refPtr);
	PX_ASSERT(refPtr);
	if (refPtr)
	{
		refPtr->setName(desc.instancedObjectSimulationAssetName);
	}

	//setParamString( childHandle, desc.instancedObjectSimulationAssetName );

	indexHandle.getChildHandle(mParams, "physMatName", childHandle);
	mParams->setParamString(childHandle, desc.physicalMaterialName);
}

GroundEmitterAssetAuthoringImpl::~GroundEmitterAssetAuthoringImpl()
{
}
#endif

}
} // namespace nvidia::apex
