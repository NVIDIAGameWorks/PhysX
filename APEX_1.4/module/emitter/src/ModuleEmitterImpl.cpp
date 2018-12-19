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
#include "ModuleIntl.h"
#include "ModuleEmitterImpl.h"
#include "ModuleEmitterRegistration.h"
#include "EmitterAssetImpl.h"
#include "GroundEmitterAssetImpl.h"
#include "ImpactEmitterAssetImpl.h"
#include "SceneIntl.h"
#include "EmitterScene.h"
#include "PsMemoryBuffer.h"
#include "EmitterActorImpl.h"
#include "GroundEmitterActorImpl.h"
#include "ImpactEmitterActorImpl.h"
#include "ModulePerfScope.h"
using namespace emitter;

#include "Apex.h"
#include "ApexSDKIntl.h"
#include "ApexUsingNamespace.h"

namespace nvidia
{
namespace apex
{

#if defined(_USRDLL)

/* Modules don't have to link against the framework, they keep their own */
ApexSDKIntl* gApexSdk = 0;
ApexSDK* GetApexSDK()
{
	return gApexSdk;
}
ApexSDKIntl* GetInternalApexSDK()
{
	return gApexSdk;
}

APEX_API Module*  CALL_CONV createModule(
    ApexSDKIntl* inSdk,
    ModuleIntl** niRef,
    uint32_t APEXsdkVersion,
    uint32_t PhysXsdkVersion,
    ApexCreateError* errorCode)
{
	if (APEXsdkVersion != APEX_SDK_VERSION)
	{
		if (errorCode)
		{
			*errorCode = APEX_CE_WRONG_VERSION;
		}
		return NULL;
	}

	if (PhysXsdkVersion != PX_PHYSICS_VERSION)
	{
		if (errorCode)
		{
			*errorCode = APEX_CE_WRONG_VERSION;
		}
		return NULL;
	}

	gApexSdk = inSdk;
	initModuleProfiling(inSdk, "Emitter");
	ModuleEmitter* impl = PX_NEW(ModuleEmitter)(inSdk);
	*niRef  = (ModuleIntl*) impl;
	return (Module*) impl;
}

#else /* !_USRDLL */

/* Statically linking entry function */
void instantiateModuleEmitter()
{
	ApexSDKIntl* sdk = GetInternalApexSDK();
	emitter::ModuleEmitterImpl* impl = PX_NEW(emitter::ModuleEmitterImpl)(sdk);
	sdk->registerExternalModule((Module*) impl, (ModuleIntl*) impl);
}

#endif
}
namespace emitter
{
/* === ModuleEmitter Implementation === */

AuthObjTypeID EmitterAssetImpl::mAssetTypeID;
AuthObjTypeID GroundEmitterAssetImpl::mAssetTypeID;
AuthObjTypeID ImpactEmitterAssetImpl::mAssetTypeID;

AuthObjTypeID ModuleEmitterImpl::getModuleID() const
{
	return EmitterAssetImpl::mAssetTypeID;
}
AuthObjTypeID ModuleEmitterImpl::getEmitterAssetTypeID() const
{
	return EmitterAssetImpl::mAssetTypeID;
}

#ifdef WITHOUT_APEX_AUTHORING

class ApexEmitterAssetDummyAuthoring : public AssetAuthoring, public UserAllocated
{
public:
	ApexEmitterAssetDummyAuthoring(ModuleEmitterImpl* module, ResourceList& list, NvParameterized::Interface* params, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(params);
		PX_UNUSED(name);
	}

	ApexEmitterAssetDummyAuthoring(ModuleEmitterImpl* module, ResourceList& list, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(name);
	}

	ApexEmitterAssetDummyAuthoring(ModuleEmitterImpl* module, ResourceList& list)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
	}

	virtual void setToolString(const char* /*toolName*/, const char* /*toolVersion*/, uint32_t /*toolChangelist*/)
	{

	}


	virtual void release()
	{
		destroy();
	}

	// internal
	void destroy()
	{
		delete this;
	}

	/**
	* \brief Returns the name of this APEX authorable object type
	*/
	virtual const char* getObjTypeName() const
	{
		return EmitterAssetImpl::getClassName();
	}

	/**
	 * \brief Prepares a fully authored Asset Authoring object for a specified platform
	*/
	virtual bool prepareForPlatform(nvidia::apex::PlatformTag)
	{
		PX_ASSERT(0);
		return false;
	}

	const char* getName(void) const
	{
		return NULL;
	}

	/**
	* \brief Save asset's NvParameterized interface, may return NULL
	*/
	virtual NvParameterized::Interface* getNvParameterized() const
	{
		PX_ASSERT(0);
		return NULL;
	}

	virtual NvParameterized::Interface* releaseAndReturnNvParameterizedInterface(void)
	{
		PX_ALWAYS_ASSERT();
		return NULL;
	}
};

class GroundEmitterAssetDummyAuthoring : public AssetAuthoring, public UserAllocated
{
public:
	GroundEmitterAssetDummyAuthoring(ModuleEmitterImpl* module, ResourceList& list, NvParameterized::Interface* params, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(params);
		PX_UNUSED(name);
	}

	GroundEmitterAssetDummyAuthoring(ModuleEmitterImpl* module, ResourceList& list, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(name);
	}

	GroundEmitterAssetDummyAuthoring(ModuleEmitterImpl* module, ResourceList& list)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
	}

	virtual void setToolString(const char* /*toolName*/, const char* /*toolVersion*/, uint32_t /*toolChangelist*/)
	{

	}


	virtual void release()
	{
		destroy();
	}

	// internal
	void destroy()
	{
		delete this;
	}

	/**
	* \brief Returns the name of this APEX authorable object type
	*/
	virtual const char* getObjTypeName() const
	{
		return GroundEmitterAssetImpl::getClassName();
	}

	/**
	 * \brief Prepares a fully authored Asset Authoring object for a specified platform
	*/
	virtual bool prepareForPlatform(nvidia::apex::PlatformTag)
	{
		PX_ASSERT(0);
		return false;
	}

	const char* getName(void) const
	{
		return NULL;
	}

	/**
	* \brief Save asset's NvParameterized interface, may return NULL
	*/
	virtual NvParameterized::Interface* getNvParameterized() const
	{
		PX_ASSERT(0);
		return NULL;
	}

	virtual NvParameterized::Interface* releaseAndReturnNvParameterizedInterface(void)
	{
		PX_ALWAYS_ASSERT();
		return NULL;
	}
};

class ImpactEmitterAssetDummyAuthoring : public AssetAuthoring, public UserAllocated
{
public:
	ImpactEmitterAssetDummyAuthoring(ModuleEmitterImpl* module, ResourceList& list, NvParameterized::Interface* params, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(params);
		PX_UNUSED(name);
	}

	ImpactEmitterAssetDummyAuthoring(ModuleEmitterImpl* module, ResourceList& list, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(name);
	}

	ImpactEmitterAssetDummyAuthoring(ModuleEmitterImpl* module, ResourceList& list)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
	}

	virtual void setToolString(const char* /*toolName*/, const char* /*toolVersion*/, uint32_t /*toolChangelist*/)
	{

	}


	virtual void release()
	{
		destroy();
	}

	// internal
	void destroy()
	{
		delete this;
	}

	const char* getName(void) const
	{
		return NULL;
	}

	/**
	* \brief Returns the name of this APEX authorable object type
	*/
	virtual const char* getObjTypeName() const
	{
		return ImpactEmitterAssetImpl::getClassName();
	}

	/**
	 * \brief Prepares a fully authored Asset Authoring object for a specified platform
	*/
	virtual bool prepareForPlatform(nvidia::apex::PlatformTag)
	{
		PX_ASSERT(0);
		return false;
	}

	/**
	* \brief Save asset's NvParameterized interface, may return NULL
	*/
	virtual NvParameterized::Interface* getNvParameterized() const
	{
		PX_ASSERT(0);
		return NULL;
	}

	virtual NvParameterized::Interface* releaseAndReturnNvParameterizedInterface(void)
	{
		PX_ALWAYS_ASSERT();
		return NULL;
	}
};



typedef ApexAuthorableObject<ModuleEmitterImpl, EmitterAssetImpl, ApexEmitterAssetDummyAuthoring> ApexEmitterAO;
typedef ApexAuthorableObject<ModuleEmitterImpl, GroundEmitterAssetImpl, GroundEmitterAssetDummyAuthoring> GroundEmitterAO;
typedef ApexAuthorableObject<ModuleEmitterImpl, ImpactEmitterAssetImpl, ImpactEmitterAssetDummyAuthoring> ImpactEmitterAO;

#else
typedef ApexAuthorableObject<ModuleEmitterImpl, EmitterAssetImpl, EmitterAssetAuthoringImpl> ApexEmitterAO;
typedef ApexAuthorableObject<ModuleEmitterImpl, GroundEmitterAssetImpl, GroundEmitterAssetAuthoringImpl> GroundEmitterAO;
typedef ApexAuthorableObject<ModuleEmitterImpl, ImpactEmitterAssetImpl, ImpactEmitterAssetAuthoringImpl> ImpactEmitterAO;
#endif

ModuleEmitterImpl::ModuleEmitterImpl(ApexSDKIntl* inSdk)
{
	mName = "Emitter";
	mSdk = inSdk;
	mApiProxy = this;
	mModuleParams = NULL;

	/* Register asset type and create a namespace for its assets */
	const char* pName = ApexEmitterAssetParameters::staticClassName();
	ApexEmitterAO* eAO = PX_NEW(ApexEmitterAO)(this, mAuthorableObjects, pName);

	pName = GroundEmitterAssetParameters::staticClassName();
	GroundEmitterAO* geAO = PX_NEW(GroundEmitterAO)(this, mAuthorableObjects, pName);

	pName = ImpactEmitterAssetParameters::staticClassName();
	ImpactEmitterAO* ieAO = PX_NEW(ImpactEmitterAO)(this, mAuthorableObjects, pName);

	EmitterAssetImpl::mAssetTypeID = eAO->getResID();
	GroundEmitterAssetImpl::mAssetTypeID = geAO->getResID();
	ImpactEmitterAssetImpl::mAssetTypeID = ieAO->getResID();

	mRateScale = 1.f;
	mDensityScale = 1.f;
	mGroundDensityScale = 1.f;

	/* Register the NvParameterized factories */
	NvParameterized::Traits* traits = mSdk->getParameterizedTraits();
	ModuleEmitterRegistration::invokeRegistration(traits);
}

ApexActor* ModuleEmitterImpl::getApexActor(Actor* nxactor, AuthObjTypeID type) const
{
	if (type == EmitterAssetImpl::mAssetTypeID)
	{
		return (EmitterActorImpl*) nxactor;
	}
	else if (type == GroundEmitterAssetImpl::mAssetTypeID)
	{
		return (GroundEmitterActorImpl*) nxactor;
	}
	else if (type == ImpactEmitterAssetImpl::mAssetTypeID)
	{
		return (ImpactEmitterActorImpl*) nxactor;
	}

	return NULL;
}

NvParameterized::Interface* ModuleEmitterImpl::getDefaultModuleDesc()
{
	WRITE_ZONE();
	NvParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (!mModuleParams)
	{
		mModuleParams = DYNAMIC_CAST(EmitterModuleParameters*)
		                (traits->createNvParameterized("EmitterModuleParameters"));
		PX_ASSERT(mModuleParams);
	}
	else
	{
		mModuleParams->initDefaults();
	}

	return mModuleParams;
}

void ModuleEmitterImpl::init(const ModuleEmitterDesc& desc)
{
	WRITE_ZONE();
	PX_UNUSED(desc);
}

ModuleEmitterImpl::~ModuleEmitterImpl()
{
}

void ModuleEmitterImpl::destroy()
{
	NvParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (mModuleParams)
	{
		mModuleParams->destroy();
		mModuleParams = NULL;
	}

	ModuleBase::destroy();

	if (traits)
	{
		ModuleEmitterRegistration::invokeUnregistration(traits);
	}

	delete this;
}

ModuleSceneIntl* ModuleEmitterImpl::createInternalModuleScene(SceneIntl& scene, RenderDebugInterface* debugRender)
{
	return PX_NEW(EmitterScene)(*this, scene, debugRender, mEmitterScenes);
}

void ModuleEmitterImpl::releaseModuleSceneIntl(ModuleSceneIntl& scene)
{
	EmitterScene* es = DYNAMIC_CAST(EmitterScene*)(&scene);
	es->destroy();
}

uint32_t ModuleEmitterImpl::forceLoadAssets()
{
	uint32_t loadedAssetCount = 0;

	for (uint32_t i = 0; i < mAuthorableObjects.getSize(); i++)
	{
		AuthorableObjectIntl* ao = static_cast<AuthorableObjectIntl*>(mAuthorableObjects.getResource(i));
		loadedAssetCount += ao->forceLoadAssets();
	}

	return loadedAssetCount;
}

EmitterScene* ModuleEmitterImpl::getEmitterScene(const Scene& apexScene)
{
	for (uint32_t i = 0 ; i < mEmitterScenes.getSize() ; i++)
	{
		EmitterScene* es = DYNAMIC_CAST(EmitterScene*)(mEmitterScenes.getResource(i));
		if (es->mApexScene == &apexScene)
		{
			return es;
		}
	}

	PX_ASSERT(!"Unable to locate an appropriate EmitterScene");
	return NULL;
}

RenderableIterator* ModuleEmitterImpl::createRenderableIterator(const Scene& apexScene)
{
	EmitterScene* es = getEmitterScene(apexScene);
	if (es)
	{
		return es->createRenderableIterator();
	}

	return NULL;
}

}
} // namespace nvidia::apex
