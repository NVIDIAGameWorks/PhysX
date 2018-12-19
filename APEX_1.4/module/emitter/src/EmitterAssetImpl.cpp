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
#include "ModuleEmitterImpl.h"
#include "EmitterAssetImpl.h"
#include "EmitterActorImpl.h"
//#include "ApexSharedSerialization.h"

#include "EmitterGeomBoxImpl.h"
#include "EmitterGeomSphereImpl.h"
#include "EmitterGeomSphereShellImpl.h"
#include "EmitterGeomCylinderImpl.h"
#include "EmitterGeomExplicitImpl.h"
#include "EmitterAssetPreview.h"

#include "EmitterGeomBoxParams.h"
#include "EmitterGeomSphereParams.h"
#include "EmitterGeomSphereShellParams.h"
#include "EmitterGeomCylinderParams.h"
#include "EmitterGeomExplicitParams.h"

#include "IofxAsset.h"

namespace nvidia
{
namespace emitter
{


struct ApexEmitterGeomTypes
{
	enum Enum
	{
		GEOM_GROUND = 0,
		GEOM_BOX,
		GEOM_SPHERE,
		GEOM_SPHERE_SHELL,
		GEOM_EXLICIT,
	};
};

void EmitterAssetImpl::copyLodDesc2(EmitterLodParamDesc& dst, const ApexEmitterAssetParametersNS::emitterLodParamDesc_Type& src)
{
	PX_ASSERT(src.version == dst.current);

	dst.bias			= src.bias;
	dst.distanceWeight	= src.distanceWeight;
	dst.lifeWeight		= src.lifeWeight;
	dst.maxDistance		= src.maxDistance;
	dst.separationWeight = src.separationWeight;
	dst.speedWeight		= src.speedWeight;
}

void EmitterAssetImpl::copyLodDesc2(ApexEmitterAssetParametersNS::emitterLodParamDesc_Type& dst, const EmitterLodParamDesc& src)
{
	dst.version			= src.current;
	dst.bias			= src.bias;
	dst.distanceWeight	= src.distanceWeight;
	dst.lifeWeight		= src.lifeWeight;
	dst.maxDistance		= src.maxDistance;
	dst.separationWeight = src.separationWeight;
	dst.speedWeight		= src.speedWeight;
}

void EmitterAssetImpl::postDeserialize(void* userData_)
{
	PX_UNUSED(userData_);

	ApexSimpleString tmpStr;
	NvParameterized::Handle h(*mParams);
	NvParameterized::Interface* refPtr;

	if (mParams->iofxAssetName == NULL)
	{
		NvParameterized::Handle h(mParams);
		h.getParameter("iofxAssetName");
		h.initParamRef(h.parameterDefinition()->refVariantVal(0), true);
	}

	if (mParams->iosAssetName == NULL)
	{
		NvParameterized::Handle h(mParams);
		h.getParameter("iosAssetName");
		h.initParamRef(h.parameterDefinition()->refVariantVal(0), true);
	}

	if (mParams->geometryType == NULL)
	{
		NvParameterized::Handle h(mParams);
		h.getParameter("geometryType");
		h.initParamRef(h.parameterDefinition()->refVariantVal(0), true);
	}


	ApexSimpleString iofxName(mParams->iofxAssetName->name());
	ApexSimpleString iosName(mParams->iosAssetName->name());
	ApexSimpleString iosTypeName(mParams->iosAssetName->className());

	if (!iofxName.len() || !iosName.len() || !iosTypeName.len())
	{
		APEX_INTERNAL_ERROR("IOFX, IOS, or IOS type not initialized");
		return;
	}

	if (mGeom)
	{
		mGeom->destroy();
	}

	mParams->getParameterHandle("geometryType", h);
	mParams->getParamRef(h, refPtr);
	PX_ASSERT(refPtr);
	if (!refPtr)
	{
		APEX_INTERNAL_ERROR("No emitter geometry specified");
		return;
	}

	tmpStr = refPtr->className();

	if (tmpStr == EmitterGeomBoxParams::staticClassName())
	{
		EmitterGeomBoxImpl* exp = PX_NEW(EmitterGeomBoxImpl)(refPtr);
		mGeom = exp;
	}
	else if (tmpStr == EmitterGeomSphereParams::staticClassName())
	{
		EmitterGeomSphereImpl* exp = PX_NEW(EmitterGeomSphereImpl)(refPtr);
		mGeom = exp;
	}
	else if (tmpStr == EmitterGeomSphereShellParams::staticClassName())
	{
		EmitterGeomSphereShellImpl* exp = PX_NEW(EmitterGeomSphereShellImpl)(refPtr);
		mGeom = exp;
	}
	else if (tmpStr == EmitterGeomCylinderParams::staticClassName())
	{
		EmitterGeomCylinderImpl* exp = PX_NEW(EmitterGeomCylinderImpl)(refPtr);
		mGeom = exp;
	}
	else if (tmpStr == EmitterGeomExplicitParams::staticClassName())
	{
		EmitterGeomExplicitImpl* exp = PX_NEW(EmitterGeomExplicitImpl)(refPtr);
		mGeom = exp;
	}
	else
	{
		PX_ASSERT(0 && "Invalid geometry type for APEX emitter");
		return;
	}

	copyLodDesc2(mLodDesc, mParams->lodParamDesc);

	initializeAssetNameTable();
}


void EmitterAssetImpl::initializeAssetNameTable()
{
	// clean up asset tracker list
	mIosAssetTracker.removeAllAssetNames();
	mIofxAssetTracker.removeAllAssetNames();

	mIosAssetTracker.addAssetName(mParams->iosAssetName->className(),
	                              mParams->iosAssetName->name());

	mIofxAssetTracker.addAssetName(mParams->iofxAssetName->name(), false);

	ParamArray<Vec2R> cp(mParams, "rateVsTimeCurvePoints", (ParamDynamicArrayStruct*)&mParams->rateVsTimeCurvePoints);
	for (uint32_t i = 0; i < cp.size(); i++)
	{
		mRateVsTimeCurve.addControlPoint(cp[i]);
	}
}

EmitterAssetImpl::EmitterAssetImpl(ModuleEmitterImpl* module, ResourceList& list, const char* name):
	mDefaultActorParams(NULL),
	mDefaultPreviewParams(NULL),
	mModule(module),
	mName(name),
	mGeom(NULL),
	mIofxAssetTracker(module->mSdk, IOFX_AUTHORING_TYPE_NAME),
	mIosAssetTracker(module->mSdk)
{
	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
	mParams = static_cast<ApexEmitterAssetParameters*>(traits->createNvParameterized(ApexEmitterAssetParameters::staticClassName()));

	PX_ASSERT(mParams);

	mParams->setSerializationCallback(this);
	list.add(*this);
}

EmitterAssetImpl::EmitterAssetImpl(ModuleEmitterImpl* module,
                                   ResourceList& list,
                                   NvParameterized::Interface* params,
                                   const char* name) :
	mParams(static_cast<ApexEmitterAssetParameters*>(params)),
	mDefaultActorParams(NULL),
	mDefaultPreviewParams(NULL),
	mModule(module),
	mName(name),
	mGeom(NULL),
	mIofxAssetTracker(module->mSdk, IOFX_AUTHORING_TYPE_NAME),
	mIosAssetTracker(module->mSdk)
{
	// this may no longer make any sense
	mParams->setSerializationCallback(this);

	// call this now to "initialize" the asset
	postDeserialize();

	list.add(*this);
}

EmitterAssetImpl::~EmitterAssetImpl()
{
}

void EmitterAssetImpl::release()
{
	mModule->mSdk->releaseAsset(*this);
}

void EmitterAssetImpl::destroy()
{
	/* Assets that were forceloaded or loaded by actors will be automatically
	 * released by the ApexAssetTracker member destructors.
	 */

	if (mGeom)
	{
		mGeom->destroy();
	}
	mGeom = NULL;

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

	if (mParams)
	{
		mParams->destroy();
		mParams = NULL;
	}

	/* Actors are automatically cleaned up on deletion by ResourceList dtor */
	delete this;
}

EmitterGeomExplicit* EmitterAssetImpl::isExplicitGeom()
{
	READ_ZONE();
	return const_cast<EmitterGeomExplicit*>(mGeom->getEmitterGeom()->isExplicitGeom());
}

const EmitterGeomExplicit* EmitterAssetImpl::isExplicitGeom() const
{
	return mGeom->getEmitterGeom()->isExplicitGeom();
}

NvParameterized::Interface* EmitterAssetImpl::getDefaultActorDesc()
{
	WRITE_ZONE();
	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
	PX_ASSERT(traits);
	if (!traits)
	{
		return NULL;
	}

	// create if not yet created
	if (!mDefaultActorParams)
	{
		const char* className = ApexEmitterActorParameters::staticClassName();
		NvParameterized::Interface* param = traits->createNvParameterized(className);
		mDefaultActorParams = static_cast<ApexEmitterActorParameters*>(param);

		PX_ASSERT(param);
		if (!param)
		{
			return NULL;
		}
	}

	if (mDefaultActorParams)
	{
		mDefaultActorParams->emitterDuration = mParams->emitterDuration;
	}

	return mDefaultActorParams;
}

NvParameterized::Interface* EmitterAssetImpl::getDefaultAssetPreviewDesc()
{
	WRITE_ZONE();
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

Actor* EmitterAssetImpl::createApexActor(const NvParameterized::Interface& parms, Scene& apexScene)
{
	WRITE_ZONE();
	if (!isValidForActorCreation(parms, apexScene))
	{
		return NULL;
	}

	Actor* ret = 0;

	const char* className = parms.className();
	if (nvidia::strcmp(className, ApexEmitterActorParameters::staticClassName()) == 0)
	{
		EmitterActorDesc desc;
		const ApexEmitterActorParameters* pDesc = static_cast<const ApexEmitterActorParameters*>(&parms);

		desc.attachRelativePose	= pDesc->attachRelativePose;
		desc.initialPose		= pDesc->initialPose;
		desc.emitAssetParticles	= pDesc->emitAssetParticles;
		desc.emitterDuration	= pDesc->emitterDuration;
		desc.initialScale		= pDesc->initialScale;

		/* Resolve the authored collision group mask name into the actual ID */
		if (pDesc->overlapTestGroupMaskName != NULL &&
		        pDesc->overlapTestGroupMaskName[0] != 0)
		{
			ResourceProviderIntl* nrp = mModule->mSdk->getInternalResourceProvider();
			ResID cgmns = mModule->mSdk->getCollisionGroupMaskNameSpace();
			ResID cgresid = nrp->createResource(cgmns, pDesc->overlapTestGroupMaskName);
			desc.overlapTestCollisionGroups = (uint32_t)(size_t) nrp->getResource(cgresid);
		}

		ret = createEmitterActor(desc, apexScene);
	}

	return ret;
}



AssetPreview* EmitterAssetImpl::createApexAssetPreview(const NvParameterized::Interface& parms, AssetPreviewScene* previewScene)
{
	WRITE_ZONE();
	AssetPreview* ret = 0;

	const char* className = parms.className();
	if (nvidia::strcmp(className, EmitterAssetPreviewParameters::staticClassName()) == 0)
	{
		EmitterPreviewDesc desc;
		const EmitterAssetPreviewParameters* pDesc = static_cast<const EmitterAssetPreviewParameters*>(&parms);

		desc.mPose	= pDesc->pose;
		desc.mScale = pDesc->scale;

		ret = createEmitterPreview(desc, previewScene);
	}

	return ret;
}


EmitterActor* EmitterAssetImpl::createEmitterActor(const EmitterActorDesc& desc, const Scene& scene)
{
	if (!desc.isValid())
	{
		return NULL;
	}

	EmitterScene* es = mModule->getEmitterScene(scene);
	EmitterActorImpl* actor = PX_NEW(EmitterActorImpl)(desc, *this, mEmitterActors, *es);
	if (!actor->isValid())
	{
		actor->destroy();
		return NULL;
	}
	return actor;
}

void EmitterAssetImpl::releaseEmitterActor(EmitterActor& nxactor)
{
	EmitterActorImpl* actor = DYNAMIC_CAST(EmitterActorImpl*)(&nxactor);
	actor->destroy();
}

EmitterPreview* EmitterAssetImpl::createEmitterPreview(const EmitterPreviewDesc& desc, AssetPreviewScene* previewScene)
{
	if (!desc.isValid())
	{
		return NULL;
	}

	EmitterAssetPreview* p = PX_NEW(EmitterAssetPreview)(desc, *this, previewScene, GetApexSDK());
	if (p && !p->isValid())
	{
		p->destroy();
		p = NULL;
	}
	return p;
}

void EmitterAssetImpl::releaseEmitterPreview(EmitterPreview& nxpreview)
{
	EmitterAssetPreview* preview = DYNAMIC_CAST(EmitterAssetPreview*)(&nxpreview);
	preview->destroy();
}


uint32_t EmitterAssetImpl::forceLoadAssets()
{
	uint32_t assetLoadedCount = 0;

	assetLoadedCount += mIofxAssetTracker.forceLoadAssets();
	assetLoadedCount += mIosAssetTracker.forceLoadAssets();

	return assetLoadedCount;
}

bool EmitterAssetImpl::isValidForActorCreation(const ::NvParameterized::Interface& /*actorParams*/, Scene& /*apexScene*/) const
{
	READ_ZONE();
	if (!mGeom)
	{
		return false;
	}

	const EmitterGeomExplicit* explicitGeom = isExplicitGeom();
	if (explicitGeom)
	{
		// Velocity array must be same size as positions or 0

		const EmitterGeomExplicit::PointParams* points;
		const PxVec3* velocities;
		uint32_t numPoints, numVelocities;
		explicitGeom->getParticleList(points, numPoints, velocities, numVelocities);
		if (!(numPoints == numVelocities || 0 == numVelocities))
		{
			return false;
		}

		const EmitterGeomExplicit::SphereParams* spheres;
		uint32_t numSpheres;
		explicitGeom->getSphereList(spheres, numSpheres, velocities, numVelocities);
		if (!(numSpheres == numVelocities || 0 == numVelocities))
		{
			return false;
		}

		const EmitterGeomExplicit::EllipsoidParams* ellipsoids;
		uint32_t numEllipsoids;
		explicitGeom->getEllipsoidList(ellipsoids, numEllipsoids, velocities, numVelocities);
		if (!(numEllipsoids == numVelocities || 0 == numVelocities))
		{
			return false;
		}

		// Radiuses are > 0

		for (uint32_t i = 0; i < numSpheres; ++i)
			if (explicitGeom->getSphereRadius(i) <= 0)
			{
				return false;
			}

		for (uint32_t i = 0; i < numEllipsoids; ++i)
			if (explicitGeom->getEllipsoidRadius(i) <= 0)
			{
				return false;
			}

		/*		// Normals are normalized

				for(uint32_t i = 0; i < numEllipsoids; ++i)
					if( !explicitGeom->getEllipsoidNormal(i).isNormalized() )
						return false;
		*/
		// Distance >= 0

		if (explicitGeom->getDistance() < 0)
		{
			return false;
		}

		// Distance > 0 if we have shapes

		if ((numSpheres || numEllipsoids) && !explicitGeom->getDistance())
		{
			return false;
		}
	}

	return true;
}

/*===============  Asset Authoring =================*/
#ifndef WITHOUT_APEX_AUTHORING
void EmitterAssetAuthoringImpl::setInstancedObjectEffectsAssetName(const char* iofxname)
{
	NvParameterized::Handle h(*mParams);
	NvParameterized::Interface* refPtr;

	mParams->getParameterHandle("iofxAssetName", h);
	mParams->initParamRef(h, NULL, true);
	mParams->getParamRef(h, refPtr);
	PX_ASSERT(refPtr);
	if (refPtr)
	{
		refPtr->setName(iofxname);
	}

}


void EmitterAssetAuthoringImpl::setInstancedObjectSimulatorAssetName(const char* iosname)
{
	NvParameterized::Handle h(*mParams);
	NvParameterized::Interface* refPtr;

	mParams->getParameterHandle("iosAssetName", h);
	mParams->initParamRef(h, NULL, true);
	mParams->getParamRef(h, refPtr);
	PX_ASSERT(refPtr);
	if (refPtr)
	{
		refPtr->setName(iosname);
	}
}


void EmitterAssetAuthoringImpl::setInstancedObjectSimulatorTypeName(const char* iostname)
{
	NvParameterized::Handle h(*mParams);
	NvParameterized::Interface* refPtr;

	mParams->getParameterHandle("iosAssetName", h);
	mParams->initParamRef(h, iostname, true);
	mParams->getParamRef(h, refPtr);
	PX_ASSERT(refPtr);
	if (refPtr)
	{
		refPtr->setClassName(iostname);
	}
}


void EmitterAssetAuthoringImpl::release()
{
	mModule->mSdk->releaseAssetAuthoring(*this);
}


EmitterGeomBox* EmitterAssetAuthoringImpl::setBoxGeom()
{
	if (mGeom)
	{
		mGeom->destroy();
		if (mParams->geometryType)
		{
			mParams->geometryType->destroy();
			mParams->geometryType = NULL;
		}
	}

	NvParameterized::Handle h(*mParams);
	NvParameterized::Interface* refPtr = 0;
	mParams->getParameterHandle("geometryType", h);

	mParams->getParamRef(h, refPtr);
	if (refPtr)
	{
		refPtr->destroy();
		mParams->setParamRef(h, 0);
	}

	mParams->initParamRef(h, EmitterGeomBoxParams::staticClassName(), true);

	refPtr = 0;
	mParams->getParamRef(h, refPtr);
	PX_ASSERT(refPtr);
	if (!refPtr)
	{
		return NULL;
	}

	EmitterGeomBoxImpl* box = PX_NEW(EmitterGeomBoxImpl)(refPtr);
	mGeom = box;
	return box;
}

EmitterGeomSphere* EmitterAssetAuthoringImpl::setSphereGeom()
{
	if (mGeom)
	{
		mGeom->destroy();
		if (mParams->geometryType)
		{
			mParams->geometryType->destroy();
			mParams->geometryType = NULL;
		}
	}

	NvParameterized::Handle h(*mParams);
	NvParameterized::Interface* refPtr = 0;
	mParams->getParameterHandle("geometryType", h);

	mParams->getParamRef(h, refPtr);
	if (refPtr)
	{
		refPtr->destroy();
		mParams->setParamRef(h, 0);
	}

	mParams->initParamRef(h, EmitterGeomSphereParams::staticClassName(), true);

	refPtr = 0;
	mParams->getParamRef(h, refPtr);
	PX_ASSERT(refPtr);
	if (!refPtr)
	{
		return NULL;
	}

	EmitterGeomSphereImpl* sphere = PX_NEW(EmitterGeomSphereImpl)(refPtr);
	mGeom = sphere;
	return sphere;
}

EmitterGeomSphereShell* 	EmitterAssetAuthoringImpl::setSphereShellGeom()
{
	if (mGeom)
	{
		mGeom->destroy();
		if (mParams->geometryType)
		{
			mParams->geometryType->destroy();
			mParams->geometryType = NULL;
		}
	}

	NvParameterized::Handle h(*mParams);
	NvParameterized::Interface* refPtr = 0;
	mParams->getParameterHandle("geometryType", h);

	mParams->getParamRef(h, refPtr);
	if (refPtr)
	{
		refPtr->destroy();
		mParams->setParamRef(h, 0);
	}

	mParams->initParamRef(h, EmitterGeomSphereShellParams::staticClassName(), true);

	refPtr = 0;
	mParams->getParamRef(h, refPtr);
	PX_ASSERT(refPtr);
	if (!refPtr)
	{
		return NULL;
	}

	EmitterGeomSphereShellImpl* sphereshell = PX_NEW(EmitterGeomSphereShellImpl)(refPtr);
	mGeom = sphereshell;
	return sphereshell;
}

EmitterGeomExplicit* 	EmitterAssetAuthoringImpl::setExplicitGeom()
{
	if (mGeom)
	{
		mGeom->destroy();
		if (mParams->geometryType)
		{
			mParams->geometryType->destroy();
			mParams->geometryType = NULL;
		}
	}

	NvParameterized::Handle h(*mParams);
	NvParameterized::Interface* refPtr = 0;
	mParams->getParameterHandle("geometryType", h);

	mParams->getParamRef(h, refPtr);
	if (refPtr)
	{
		refPtr->destroy();
		mParams->setParamRef(h, 0);
	}

	mParams->initParamRef(h, EmitterGeomExplicitParams::staticClassName(), true);

	refPtr = 0;
	mParams->getParamRef(h, refPtr);
	PX_ASSERT(refPtr);
	if (!refPtr)
	{
		return NULL;
	}

	EmitterGeomExplicitImpl* exp = PX_NEW(EmitterGeomExplicitImpl)(refPtr);
	mGeom = exp;
	return exp;
}
#endif

}
} // namespace nvidia::apex
