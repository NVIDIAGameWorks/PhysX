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

#include "ApexAuthorableObject.h"
#include "ApexIsoMesh.h"
#include "ApexSubdivider.h"
#include "ApexSharedUtils.h"

#include "RenderMeshAssetIntl.h"
#include <limits.h>
#include <new>

#include "PxCudaContextManager.h"
#include "Factory.h"

#include "ModuleClothingImpl.h"
#include "ModuleClothingRegistration.h"

#include "ClothingAssetImpl.h"
#include "ClothingAssetAuthoringImpl.h"
#include "ClothingPhysicalMeshImpl.h"
#include "SceneIntl.h"

#include "ClothingScene.h"
#include "ModulePerfScope.h"

#include "CookingPhysX.h"
#include "Cooking.h"
#include "Simulation.h"

#include "ApexSDKIntl.h"
#include "ApexUsingNamespace.h"

#if APEX_CUDA_SUPPORT
#include "CuFactory.h"
#endif

#include "ApexPvdClient.h"

using namespace clothing;
using namespace physx::pvdsdk;

#define INIT_PVD_CLASSES_PARAMETERIZED( parameterizedClassName ) { \
	pvdStream.createClass(NamespacedName(APEX_PVD_NAMESPACE, #parameterizedClassName)); \
	parameterizedClassName* params = DYNAMIC_CAST(parameterizedClassName*)(GetInternalApexSDK()->getParameterizedTraits()->createNvParameterized(#parameterizedClassName)); \
	client->initPvdClasses(*params->rootParameterDefinition(), #parameterizedClassName); \
	params->destroy(); }


namespace nvidia
{
namespace apex
{

#if defined(_USRDLL) || PX_OSX || (PX_LINUX && APEX_LINUX_SHARED_LIBRARIES)

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
	clothing::ModuleClothingImpl* impl = PX_NEW(clothing::ModuleClothingImpl)(inSdk);
	*niRef  = (ModuleIntl*) impl;
	return (Module*) impl;
}

#else // !defined(_USRDLL) && !PX_OSX && (!PX_LINUX || !APEX_LINUX_SHARED_LIBRARIES)

/* Statically linking entry function */
void instantiateModuleClothing()
{
	using namespace clothing;
	ApexSDKIntl* sdk = GetInternalApexSDK();
	clothing::ModuleClothingImpl* impl = PX_NEW(clothing::ModuleClothingImpl)(sdk);
	sdk->registerExternalModule((Module*) impl, (ModuleIntl*) impl);
}
#endif  // !defined(_USRDLL) && !PX_OSX && (!PX_LINUX || !APEX_LINUX_SHARED_LIBRARIES)
}

namespace clothing
{

// This is needed to have an actor assigned to every PxActor, even if they currently don't belong to an ClothingActor
class DummyActor : public Actor, public UserAllocated, public ApexRWLockable
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	DummyActor(Asset* owner) : mOwner(owner) {}
	void release()
	{
		PX_DELETE(this);
	}
	Asset* getOwner() const
	{
		return mOwner;
	}

	void getLodRange(float& min, float& max, bool& intOnly) const
	{
		PX_UNUSED(min);
		PX_UNUSED(max);
		PX_UNUSED(intOnly);
	}

	float getActiveLod() const
	{
		return -1.0f;
	}

	void forceLod(float lod)
	{
		PX_UNUSED(lod);
	}

	/**
	\brief Selectively enables/disables debug visualization of a specific APEX actor.  Default value it true.
	*/
	virtual void setEnableDebugVisualization(bool state)
	{
		PX_UNUSED(state);
	}

private:
	Asset* mOwner;
};



// This is needed to for every dummy actor to point to an asset that in turn has the right object type id.
class DummyAsset : public Asset, public UserAllocated, public ApexRWLockable
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	DummyAsset(AuthObjTypeID assetTypeID) : mAssetTypeID(assetTypeID) {};

	void release()
	{
		PX_DELETE(this);
	}

	virtual const char* getName() const
	{
		return NULL;
	}
	virtual AuthObjTypeID getObjTypeID() const
	{
		return mAssetTypeID;
	}
	virtual const char* getObjTypeName() const
	{
		return NULL;
	}
	virtual uint32_t forceLoadAssets()
	{
		return 0;
	}
	virtual const NvParameterized::Interface* getAssetNvParameterized() const
	{
		return NULL;
	}

	NvParameterized::Interface* getDefaultActorDesc()
	{
		PX_ALWAYS_ASSERT();
		return NULL;
	};
	NvParameterized::Interface* getDefaultAssetPreviewDesc()
	{
		PX_ALWAYS_ASSERT();
		return NULL;
	};

	virtual Actor* createApexActor(const NvParameterized::Interface& /*parms*/, Scene& /*apexScene*/)
	{
		PX_ALWAYS_ASSERT();
		return NULL;
	}

	virtual AssetPreview* createApexAssetPreview(const ::NvParameterized::Interface& /*params*/, AssetPreviewScene* /*previewScene*/)
	{
		PX_ALWAYS_ASSERT();
		return NULL;
	}

	virtual bool isValidForActorCreation(const ::NvParameterized::Interface& /*parms*/, Scene& /*apexScene*/) const
	{
		return true; // TODO implement this method
	}

	virtual bool isDirty() const
	{
		return false;
	}


	/**
	 * \brief Releases the ApexAsset but returns the NvParameterized::Interface and *ownership* to the caller.
	 */
	virtual NvParameterized::Interface* releaseAndReturnNvParameterizedInterface(void)
	{
		return NULL;
	}

private:
	AuthObjTypeID mAssetTypeID;
};

ModuleClothingImpl::ModuleClothingImpl(ApexSDKIntl* inSdk)
	: mDummyActor(NULL)
	, mDummyAsset(NULL)
	, mModuleParams(NULL)
	, mApexClothingActorParams(NULL)
	, mApexClothingPreviewParams(NULL)
	, mCpuFactory(NULL)
	, mCpuFactoryReferenceCount(0)
#if APEX_CUDA_SUPPORT
	, mGpuDllHandle(NULL)
	, mPxCreateCuFactoryFunc(NULL)
#endif
{
	mInternalModuleParams.maxNumCompartments = 4;
	mInternalModuleParams.maxUnusedPhysXResources = 5;
	mInternalModuleParams.allowAsyncCooking = true;
	mInternalModuleParams.avgSimFrequencyWindow = 60;
	mInternalModuleParams.allowApexWorkBetweenSubsteps = true;
	mInternalModuleParams.interCollisionDistance = 0.0f;
	mInternalModuleParams.interCollisionStiffness = 1.0f;
	mInternalModuleParams.interCollisionIterations = 1;
	mInternalModuleParams.sparseSelfCollision = false;
	mInternalModuleParams.maxTimeRenderProxyInPool = 100;
	PX_COMPILE_TIME_ASSERT(sizeof(mInternalModuleParams) == 40); // don't forget to init the new param here (and then update this assert)

	mName = "Clothing";
	mSdk = inSdk;
	mApiProxy = this;

	NvParameterized::Traits* traits = mSdk->getParameterizedTraits();
	if (traits)
	{
		ModuleClothingRegistration::invokeRegistration(traits);
		mApexClothingActorParams = traits->createNvParameterized(ClothingActorParam::staticClassName());
		mApexClothingPreviewParams = traits->createNvParameterized(ClothingPreviewParam::staticClassName());
	}

#if APEX_CUDA_SUPPORT
	// Since we split out the GPU code, we load the module and create the CuFactory ourselves
	ApexSimpleString gpuClothingDllName;
	PX_COMPILE_TIME_ASSERT(sizeof(HMODULE) == sizeof(mGpuDllHandle));
	{
		ModuleUpdateLoader moduleLoader(UPDATE_LOADER_DLL_NAME);

#define APEX_CLOTHING_GPU_DLL_PREFIX "APEX_ClothingGPU"

#ifdef PX_PHYSX_DLL_NAME_POSTFIX
# if PX_X86
		static const char*	gpuClothingDllPrefix = APEX_CLOTHING_GPU_DLL_PREFIX PX_STRINGIZE(PX_PHYSX_DLL_NAME_POSTFIX) "_x86";
# elif PX_X64
		static const char*	gpuClothingDllPrefix = APEX_CLOTHING_GPU_DLL_PREFIX PX_STRINGIZE(PX_PHYSX_DLL_NAME_POSTFIX) "_x64";
# endif
#else
# if PX_X86
		static const char*	gpuClothingDllPrefix = APEX_CLOTHING_GPU_DLL_PREFIX "_x86";
# elif PX_X64
		static const char*	gpuClothingDllPrefix = APEX_CLOTHING_GPU_DLL_PREFIX "_x64";
# endif
#endif

#undef APEX_CLOTHING_GPU_DLL_PREFIX

		gpuClothingDllName = ApexSimpleString(gpuClothingDllPrefix);

		// applications can append strings to the APEX DLL filenames, support this with getCustomDllNamePostfix()
		gpuClothingDllName += ApexSimpleString(GetInternalApexSDK()->getCustomDllNamePostfix());
		gpuClothingDllName += ApexSimpleString(".dll");

		mGpuDllHandle = moduleLoader.loadModule(gpuClothingDllName.c_str(), GetInternalApexSDK()->getAppGuid());
	}

	if (mGpuDllHandle)
	{
		mPxCreateCuFactoryFunc = (PxCreateCuFactory_FUNC*)GetProcAddress((HMODULE)mGpuDllHandle, "PxCreateCuFactory");
		if (mPxCreateCuFactoryFunc == NULL)
		{
			APEX_DEBUG_WARNING("Failed to find method PxCreateCuFactory in dll \'%s\'", gpuClothingDllName.c_str());
			FreeLibrary((HMODULE)mGpuDllHandle);
			mGpuDllHandle = NULL;
		}
	}
	else if (!gpuClothingDllName.empty())
	{
		APEX_DEBUG_WARNING("Failed to load the GPU dll \'%s\'", gpuClothingDllName.c_str());
	}
#endif
}



AuthObjTypeID ModuleClothingImpl::getModuleID() const
{
	return ClothingAssetImpl::mAssetTypeID;
}



RenderableIterator* ModuleClothingImpl::createRenderableIterator(const Scene& apexScene)
{
	ClothingScene* cs = getClothingScene(apexScene);
	if (cs)
	{
		return cs->createRenderableIterator();
	}

	return NULL;
}

#ifdef WITHOUT_APEX_AUTHORING

class ClothingAssetDummyAuthoring : public AssetAuthoring, public UserAllocated
{
public:
	ClothingAssetDummyAuthoring(ModuleClothingImpl* module, ResourceList& list, NvParameterized::Interface* params, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(params);
		PX_UNUSED(name);
	}

	ClothingAssetDummyAuthoring(ModuleClothingImpl* module, ResourceList& list, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(name);
	}

	ClothingAssetDummyAuthoring(ModuleClothingImpl* module, ResourceList& list)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
	}

	virtual ~ClothingAssetDummyAuthoring() {}

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
		PX_DELETE(this);
	}

	/**
	* \brief Returns the name of this APEX authorable object type
	*/
	virtual const char* getObjTypeName() const
	{
		return CLOTHING_AUTHORING_TYPE_NAME;
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
		return NULL; //ClothingAssetImpl::getAssetNvParameterized();
	}

	virtual NvParameterized::Interface* releaseAndReturnNvParameterizedInterface(void)
	{
		PX_ALWAYS_ASSERT();
		return NULL;
	}

};

typedef ApexAuthorableObject<ModuleClothingImpl, ClothingAssetImpl, ClothingAssetDummyAuthoring> ClothingAO;
#else
typedef ApexAuthorableObject<ModuleClothingImpl, ClothingAssetImpl, ClothingAssetAuthoringImpl> ClothingAO;
#endif

NvParameterized::Interface* ModuleClothingImpl::getDefaultModuleDesc()
{
	NvParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (!mModuleParams)
	{
		mModuleParams = DYNAMIC_CAST(ClothingModuleParameters*)
		                (traits->createNvParameterized("ClothingModuleParameters"));
		PX_ASSERT(mModuleParams);
	}
	else
	{
		mModuleParams->initDefaults();
	}

	const NvParameterized::Hint* hint = NULL;
	NvParameterized::Handle h(mModuleParams);

	h.getParameter("maxNumCompartments");
	PX_ASSERT(h.isValid());
#if PX_WINDOWS_FAMILY
	hint = h.parameterDefinition()->hint("defaultValueWindows");
#else
	hint = h.parameterDefinition()->hint("defaultValueConsoles");
#endif
	PX_ASSERT(hint);
	if (hint)
	{
		mModuleParams->maxNumCompartments = (uint32_t)hint->asUInt();
	}

	return mModuleParams;
}



void ModuleClothingImpl::init(NvParameterized::Interface& desc)
{
	if (::strcmp(desc.className(), ClothingModuleParameters::staticClassName()) == 0)
	{
		ClothingModuleParameters* params = DYNAMIC_CAST(ClothingModuleParameters*)(&desc);
		mInternalModuleParams = *params;
	}
	else
	{
		APEX_INVALID_PARAMETER("The NvParameterized::Interface object is of the wrong type");
	}


	ClothingAO* AOClothingAsset = PX_NEW(ClothingAO)(this, mAssetAuthorableObjectFactories, ClothingAssetParameters::staticClassName());
	ClothingAssetImpl::mAssetTypeID = AOClothingAsset->getResID();
	registerBackendFactory(&mBackendFactory);


#ifndef WITHOUT_PVD
	AOClothingAsset->mAssets.setupForPvd(mApiProxy, "ClothingAssets", "ClothingAsset");

	// handle case if module is created after pvd connection
	pvdsdk::ApexPvdClient* client = mSdk->getApexPvdClient();
	if (client != NULL)
	{
		if (client->isConnected() && client->getPxPvd().getInstrumentationFlags() & PxPvdInstrumentationFlag::eDEBUG)
		{
			pvdsdk::PvdDataStream* pvdStream = client->getDataStream();
			if (pvdStream != NULL)
			{
				pvdsdk::NamespacedName pvdModuleName(APEX_PVD_NAMESPACE, getName());
				pvdStream->createClass(pvdModuleName);
				initPvdClasses(*pvdStream);

				ModuleBase* nxModule = static_cast<ModuleBase*>(this);
				pvdStream->createInstance(pvdModuleName, nxModule);
				pvdStream->pushBackObjectRef(mSdk, "Modules", nxModule);
				initPvdInstances(*pvdStream);
			}
		}
	}
#endif
}



ClothingPhysicalMesh* ModuleClothingImpl::createEmptyPhysicalMesh()
{
	WRITE_ZONE();
	return createPhysicalMeshInternal(NULL);
}



ClothingPhysicalMesh* ModuleClothingImpl::createSingleLayeredMesh(RenderMeshAssetAuthoring* asset, uint32_t subdivisionSize, bool mergeVertices, bool closeHoles, IProgressListener* progress)
{
	WRITE_ZONE();
	return createSingleLayeredMeshInternal(DYNAMIC_CAST(RenderMeshAssetAuthoringIntl*)(asset), subdivisionSize, mergeVertices, closeHoles, progress);
}



void ModuleClothingImpl::destroy()
{
	mClothingSceneList.clear();

	if (mApexClothingActorParams != NULL)
	{
		mApexClothingActorParams->destroy();
		mApexClothingActorParams = NULL;
	}

	if (mApexClothingPreviewParams != NULL)
	{
		mApexClothingPreviewParams->destroy();
		mApexClothingPreviewParams = NULL;
	}

#if APEX_CUDA_SUPPORT
	for (PxU32 i = 0; i < mGpuFactories.size(); i++)
	{
		//APEX_DEBUG_INFO("Release Gpu factory %d", i);
		PX_DELETE(mGpuFactories[i].factoryGpu);
		mGpuFactories.replaceWithLast(i);
	}
#endif
	PX_DELETE(mCpuFactory);
	mCpuFactory = NULL;

#ifndef WITHOUT_PVD
	destroyPvdInstances();
#endif
	if (mModuleParams != NULL)
	{
		mModuleParams->destroy();
		mModuleParams = NULL;
	}

	if (mDummyActor != NULL)
	{
		mDummyActor->release();
		mDummyActor = NULL;
	}

	if (mDummyAsset != NULL)
	{
		mDummyAsset->release();
		mDummyAsset = NULL;
	}

	NvParameterized::Traits* traits = mSdk->getParameterizedTraits();

	ModuleBase::destroy();

	mAssetAuthorableObjectFactories.clear(); // needs to be done before destructor!

#if APEX_CUDA_SUPPORT
	if (mGpuDllHandle != NULL)
	{
		FreeLibrary((HMODULE)mGpuDllHandle);
	}
#endif

	if (traits)
	{
		ModuleClothingRegistration::invokeUnregistration(traits);
	}
	PX_DELETE(this);
}



ModuleSceneIntl* ModuleClothingImpl::createInternalModuleScene(SceneIntl& scene, RenderDebugInterface* renderDebug)
{
	return PX_NEW(ClothingScene)(*this, scene, renderDebug, mClothingSceneList);
}



void ModuleClothingImpl::releaseModuleSceneIntl(ModuleSceneIntl& scene)
{
	ClothingScene* clothingScene = DYNAMIC_CAST(ClothingScene*)(&scene);
	clothingScene->destroy();
}



uint32_t ModuleClothingImpl::forceLoadAssets()
{
	uint32_t loadedAssetCount = 0;
	for (uint32_t i = 0; i < mAssetAuthorableObjectFactories.getSize(); i++)
	{
		AuthorableObjectIntl* ao = static_cast<AuthorableObjectIntl*>(mAssetAuthorableObjectFactories.getResource(i));
		loadedAssetCount += ao->forceLoadAssets();
	}
	return loadedAssetCount;
}



#ifndef WITHOUT_PVD
void ModuleClothingImpl::initPvdClasses(pvdsdk::PvdDataStream& pvdStream)
{
	NamespacedName objRef = getPvdNamespacedNameForType<ObjectRef>();

	// ---------------------------------------
	// Hierarchy

	// ModuleBase holds ClothingAssets
	pvdStream.createClass(NamespacedName(APEX_PVD_NAMESPACE, "ClothingAsset"));
	pvdStream.createProperty(NamespacedName(APEX_PVD_NAMESPACE, getName()), "ClothingAssets", "children", objRef, PropertyType::Array);
	
	// ClothingAsset holds ClothingActors
	pvdStream.createClass(NamespacedName(APEX_PVD_NAMESPACE, "ClothingActor"));
	pvdStream.createProperty(NamespacedName(APEX_PVD_NAMESPACE, "ClothingAsset"), "ClothingActors", "children", objRef, PropertyType::Array);


	// ---------------------------------------
	// NvParameterized
	pvdsdk::ApexPvdClient* client = GetInternalApexSDK()->getApexPvdClient();
	PX_ASSERT(client != NULL);

	// ModuleBase Params
	INIT_PVD_CLASSES_PARAMETERIZED(ClothingModuleParameters);
	pvdStream.createProperty(NamespacedName(APEX_PVD_NAMESPACE, getName()), "ModuleParams", "", objRef, PropertyType::Scalar);

	// Asset Params

	INIT_PVD_CLASSES_PARAMETERIZED(ClothingPhysicalMeshParameters);
	INIT_PVD_CLASSES_PARAMETERIZED(ClothingGraphicalLodParameters);
	INIT_PVD_CLASSES_PARAMETERIZED(ClothingCookedParam);
	INIT_PVD_CLASSES_PARAMETERIZED(ClothingCookedPhysX3Param);
	INIT_PVD_CLASSES_PARAMETERIZED(ClothingMaterialLibraryParameters);
	INIT_PVD_CLASSES_PARAMETERIZED(ClothingAssetParameters);
	pvdStream.createProperty(NamespacedName(APEX_PVD_NAMESPACE, "ClothingAsset"), "AssetParams", "", objRef, PropertyType::Scalar);

	// Actor Params
	INIT_PVD_CLASSES_PARAMETERIZED(ClothingActorParam);
	pvdStream.createProperty(NamespacedName(APEX_PVD_NAMESPACE, "ClothingActor"), "ActorParams", "", objRef, PropertyType::Scalar);


	// ---------------------------------------
	// Additional Properties

	// ---------------------------------------
}



void ModuleClothingImpl::initPvdInstances(pvdsdk::PvdDataStream& pvdStream)
{
	// if there's more than one AOFactory we don't know any more for sure that its a clothing asset factory, so we have to adapt the code below
	PX_ASSERT(mAssetAuthorableObjectFactories.getSize() == 1 && "Adapt the code below");

	pvdsdk::ApexPvdClient* client = GetInternalApexSDK()->getApexPvdClient();
	PX_ASSERT(client != NULL);

	// ModuleBase Params
	pvdStream.createInstance(NamespacedName(APEX_PVD_NAMESPACE, "ClothingModuleParameters"), mModuleParams);
	pvdStream.setPropertyValue(mApiProxy, "ModuleParams", DataRef<const uint8_t>((const uint8_t*)&mModuleParams, sizeof(ClothingModuleParameters*)), getPvdNamespacedNameForType<ObjectRef>());
	// update module properties (should we do this per frame? if so, how?)
	client->updatePvd(mModuleParams, *mModuleParams);

	// prepare asset list and forward init calls
	AuthorableObjectIntl* ao = static_cast<AuthorableObjectIntl*>(mAssetAuthorableObjectFactories.getResource(0));
	ao->mAssets.initPvdInstances(pvdStream);
}



void ModuleClothingImpl::destroyPvdInstances()
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
					client->updatePvd(mModuleParams, *mModuleParams, pvdsdk::PvdAction::DESTROY);
					pvdStream->destroyInstance(mModuleParams);
				}
			}
		}
	}
}
#endif


ClothingScene* ModuleClothingImpl::getClothingScene(const Scene& apexScene)
{
	const SceneIntl* niScene = DYNAMIC_CAST(const SceneIntl*)(&apexScene);
	for (uint32_t i = 0 ; i < mClothingSceneList.getSize() ; i++)
	{
		ClothingScene* clothingScene = DYNAMIC_CAST(ClothingScene*)(mClothingSceneList.getResource(i));
		if (clothingScene->mApexScene == niScene)
		{
			return clothingScene;
		}
	}

	PX_ASSERT(!"Unable to locate an appropriate ClothingScene");
	return NULL;
}



ClothingPhysicalMeshImpl* ModuleClothingImpl::createPhysicalMeshInternal(ClothingPhysicalMeshParameters* mesh)
{
	ClothingPhysicalMeshImpl* result = PX_NEW(ClothingPhysicalMeshImpl)(this, mesh, &mPhysicalMeshes);
	return result;
}



void ModuleClothingImpl::releasePhysicalMesh(ClothingPhysicalMeshImpl* physicalMesh)
{
	physicalMesh->destroy();
}



void ModuleClothingImpl::unregisterAssetWithScenes(ClothingAssetImpl* asset)
{
	for (uint32_t i = 0; i < mClothingSceneList.getSize(); i++)
	{
		ClothingScene* clothingScene = static_cast<ClothingScene*>(mClothingSceneList.getResource(i));
		clothingScene->unregisterAsset(asset);
	}
}


void ModuleClothingImpl::notifyReleaseGraphicalData(ClothingAssetImpl* asset)
{
	for (uint32_t i = 0; i < mClothingSceneList.getSize(); i++)
	{
		ClothingScene* clothingScene = static_cast<ClothingScene*>(mClothingSceneList.getResource(i));
		clothingScene->removeRenderProxies(asset);
	}
}


Actor* ModuleClothingImpl::getDummyActor()
{
	mDummyProtector.lock();
	if (mDummyActor == NULL)
	{
		PX_ASSERT(mDummyAsset == NULL);
		mDummyAsset = PX_NEW(DummyAsset)(getModuleID());
		mDummyActor = PX_NEW(DummyActor)(mDummyAsset);
	}
	mDummyProtector.unlock();

	return mDummyActor;
}



void ModuleClothingImpl::registerBackendFactory(BackendFactory* factory)
{
	for (uint32_t i = 0; i < mBackendFactories.size(); i++)
	{
		if (::strcmp(mBackendFactories[i]->getName(), factory->getName()) == 0)
		{
			return;
		}
	}

	mBackendFactories.pushBack(factory);
}



void ModuleClothingImpl::unregisterBackendFactory(BackendFactory* factory)
{
	uint32_t read = 0, write = 0;

	while (read < mBackendFactories.size())
	{
		mBackendFactories[write] = mBackendFactories[read];

		if (mBackendFactories[read] == factory)
		{
			read++;
		}
		else
		{
			read++, write++;
		}
	}

	while (read < write)
	{
		mBackendFactories.popBack();
	}
}



BackendFactory* ModuleClothingImpl::getBackendFactory(const char* simulationBackend)
{
	PX_ASSERT(simulationBackend != NULL);

	for (uint32_t i = 0; i < mBackendFactories.size(); i++)
	{
		if (mBackendFactories[i]->isMatch(simulationBackend))
		{
			return mBackendFactories[i];
		}
	}

	//APEX_INVALID_OPERATION("Simulation back end \'%s\' not found, using \'PhysX\' instead\n", simulationBackend);

	PX_ASSERT(mBackendFactories.size() >= 1);
	return mBackendFactories[0];
}



ClothFactory ModuleClothingImpl::createClothFactory(PxCudaContextManager* contextManager)
{
	nvidia::Mutex::ScopedLock lock(mFactoryMutex);

#if APEX_CUDA_SUPPORT

	if (contextManager != NULL && contextManager->supportsArchSM20())
	{
		for (uint32_t i = 0; i < mGpuFactories.size(); i++)
		{
			if (mGpuFactories[i].contextManager == contextManager)
			{
				mGpuFactories[i].referenceCount++;
				//APEX_DEBUG_INFO("Found Gpu factory %d (ref = %d)", i, mGpuFactories[i].referenceCount);
				return ClothFactory(mGpuFactories[i].factoryGpu, &mFactoryMutex);
			}
		}

		// nothing found
		if (mPxCreateCuFactoryFunc != NULL)
		{
			GpuFactoryEntry entry(mPxCreateCuFactoryFunc(contextManager), contextManager);
			if (entry.factoryGpu != NULL)
			{
				//APEX_DEBUG_INFO("Create Gpu factory %d", mGpuFactories.size());
				entry.referenceCount = 1;
				mGpuFactories.pushBack(entry);
				return ClothFactory(entry.factoryGpu, &mFactoryMutex);
			}
		}

		return ClothFactory(NULL, &mFactoryMutex);
	}
	else
#else
	PX_UNUSED(contextManager);
#endif
	{
		if (mCpuFactory == NULL)
		{
			mCpuFactory = cloth::Factory::createFactory(cloth::Factory::CPU);
			//APEX_DEBUG_INFO("Create Cpu factory");
			PX_ASSERT(mCpuFactoryReferenceCount == 0);
		}

		mCpuFactoryReferenceCount++;
		//APEX_DEBUG_INFO("Get Cpu factory (ref = %d)", mCpuFactoryReferenceCount);

		return ClothFactory(mCpuFactory, &mFactoryMutex);
	}
}



void ModuleClothingImpl::releaseClothFactory(PxCudaContextManager* contextManager)
{
	nvidia::Mutex::ScopedLock lock(mFactoryMutex);

#if APEX_CUDA_SUPPORT
	if (contextManager != NULL)
	{
		for (uint32_t i = 0; i < mGpuFactories.size(); i++)
		{
			if (mGpuFactories[i].contextManager == contextManager)
			{
				PX_ASSERT(mGpuFactories[i].referenceCount > 0);
				mGpuFactories[i].referenceCount--;
				//APEX_DEBUG_INFO("Found Gpu factory %d (ref = %d)", i, mGpuFactories[i].referenceCount);

				if (mGpuFactories[i].referenceCount == 0)
				{
					//APEX_DEBUG_INFO("Release Gpu factory %d", i);
					PX_DELETE(mGpuFactories[i].factoryGpu);
					mGpuFactories.replaceWithLast(i);
				}
			}
		}
	}
	else
#else
	PX_UNUSED(contextManager);
#endif
	{
		PX_ASSERT(mCpuFactoryReferenceCount > 0);

		mCpuFactoryReferenceCount--;
		//APEX_DEBUG_INFO("Release Cpu factory (ref = %d)", mCpuFactoryReferenceCount);

		if (mCpuFactoryReferenceCount == 0)
		{
			PX_DELETE(mCpuFactory);
			mCpuFactory = NULL;
		}
	}
}



ClothingPhysicalMesh* ModuleClothingImpl::createSingleLayeredMeshInternal(RenderMeshAssetAuthoringIntl* renderMeshAsset, uint32_t subdivisionSize,
        bool mergeVertices, bool closeHoles, IProgressListener* progressListener)
{
	if (renderMeshAsset->getPartCount() > 1)
	{
		APEX_INVALID_PARAMETER("RenderMeshAssetAuthoring has more than one part (%d)", renderMeshAsset->getPartCount());
		return NULL;
	}

	if (subdivisionSize > 200)
	{
		APEX_INVALID_PARAMETER("subdivisionSize must be smaller or equal to 200 and has been clamped (was %d).", subdivisionSize);
		subdivisionSize = 200;
	}

	HierarchicalProgressListener progress(100, progressListener);


	uint32_t numGraphicalVertices = 0;

	for (uint32_t i = 0; i < renderMeshAsset->getSubmeshCount(); i++)
	{
		const RenderSubmeshIntl& submesh = renderMeshAsset->getInternalSubmesh(i);
		numGraphicalVertices += submesh.getVertexBuffer().getVertexCount();
	}

	ClothingPhysicalMeshImpl* physicalMesh = DYNAMIC_CAST(ClothingPhysicalMeshImpl*)(createEmptyPhysicalMesh());

	// set time for registration, merge, close and subdivision
	uint32_t times[4] = { 20, (uint32_t)(mergeVertices ? 20 : 0), (uint32_t)(closeHoles ? 30 : 0), (uint32_t)(subdivisionSize > 0 ? 30 : 0) };
	uint32_t sum = times[0] + times[1] + times[2] + times[3];
	for (uint32_t i = 0; i < 4; i++)
	{
		times[i] = 100 * times[i] / sum;
	}

	progress.setSubtaskWork((int32_t)times[0], "Creating single layered mesh");
	ApexSubdivider subdivider;

	Array<int32_t> old2New(numGraphicalVertices, -1);
	uint32_t nbVertices = 0;

	uint32_t vertexOffset = 0;

	for (uint32_t submeshNr = 0; submeshNr < renderMeshAsset->getSubmeshCount(); submeshNr++)
	{
		const RenderSubmeshIntl& submesh = renderMeshAsset->getInternalSubmesh(submeshNr);

		// used for physics?
		const VertexFormat& vf = submesh.getVertexBuffer().getFormat();
		uint32_t customIndex = (uint32_t)vf.getBufferIndexFromID(vf.getID("USED_FOR_PHYSICS"));
		RenderDataFormat::Enum outFormat = vf.getBufferFormat(customIndex);
		const uint8_t* usedForPhysics = NULL;
		if (outFormat == RenderDataFormat::UBYTE1)
		{
			usedForPhysics = (const uint8_t*)submesh.getVertexBuffer().getBuffer(customIndex);
		}

		customIndex = (uint32_t)vf.getBufferIndexFromID(vf.getID("LATCH_TO_NEAREST_SLAVE"));
		outFormat = vf.getBufferFormat(customIndex);
		const uint32_t* latchToNearestSlave = outFormat != RenderDataFormat::UINT1 ? NULL : (uint32_t*)submesh.getVertexBuffer().getBuffer(customIndex);

		customIndex = (uint32_t)vf.getBufferIndexFromID(vf.getID("LATCH_TO_NEAREST_MASTER"));
		outFormat = vf.getBufferFormat(customIndex);
		const uint32_t* latchToNearestMaster = outFormat != RenderDataFormat::UINT1 ? NULL : (uint32_t*)submesh.getVertexBuffer().getBuffer(customIndex);
		PX_ASSERT((latchToNearestSlave != NULL) == (latchToNearestMaster != NULL)); // both NULL or not NULL

		// triangles
		const uint32_t* indices = submesh.getIndexBuffer(0); // only 1 part supported!

		// vertices
		RenderDataFormat::Enum format;
		uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::POSITION));
		const PxVec3* positions = (const PxVec3*)submesh.getVertexBuffer().getBufferAndFormat(format, bufferIndex);
		if (format != RenderDataFormat::FLOAT3)
		{
			PX_ALWAYS_ASSERT();
			positions = NULL;
		}

		const uint32_t submeshIndices = submesh.getIndexCount(0);
		for (uint32_t meshIndex = 0; meshIndex < submeshIndices; meshIndex += 3)
		{
			if (latchToNearestSlave != NULL)
			{
				uint32_t numVerticesOk = 0;
				for (uint32_t i = 0; i < 3; i++)
				{
					const uint32_t index = indices[meshIndex + i];
					numVerticesOk += latchToNearestSlave[index] == 0 ? 1u : 0u;
				}
				if (numVerticesOk < 3)
				{
					continue;    // skip this triangle
				}
			}
			else if (usedForPhysics != NULL)
			{
				uint32_t numVerticesOk = 0;
				for (uint32_t i = 0; i < 3; i++)
				{
					const uint32_t index = indices[meshIndex + i];
					numVerticesOk += usedForPhysics[index] == 0 ? 0u : 1u;
				}
				if (numVerticesOk < 3)
				{
					continue;    // skip this triangle
				}
			}

			// add triangle to subdivider
			for (uint32_t i = 0; i < 3; i++)
			{
				const uint32_t localIndex = indices[meshIndex + i];
				const uint32_t index = localIndex + vertexOffset;
				if (old2New[index] == -1)
				{
					old2New[index] = (int32_t)nbVertices++;
					uint32_t master = latchToNearestMaster != NULL ? latchToNearestMaster[localIndex] : 0xffffffffu;
					subdivider.registerVertex(positions[localIndex], master);
				}
			}

			const uint32_t i0 = (uint32_t)old2New[indices[meshIndex + 0] + vertexOffset];
			const uint32_t i1 = (uint32_t)old2New[indices[meshIndex + 1] + vertexOffset];
			const uint32_t i2 = (uint32_t)old2New[indices[meshIndex + 2] + vertexOffset];
			subdivider.registerTriangle(i0, i1, i2);
		}
		vertexOffset += submesh.getVertexBuffer().getVertexCount();
	}

	subdivider.endRegistration();
	progress.completeSubtask();

	if (nbVertices == 0)
	{
		APEX_INVALID_PARAMETER("Mesh has no active vertices (see Physics on/off channel)");
		return NULL;
	}

	// use subdivider
	if (mergeVertices)
	{
		progress.setSubtaskWork((int32_t)times[1], "Merging");
		subdivider.mergeVertices(&progress);
		progress.completeSubtask();
	}

	if (closeHoles)
	{
		progress.setSubtaskWork((int32_t)times[2], "Closing holes");
		subdivider.closeMesh(&progress);
		progress.completeSubtask();
	}

	if (subdivisionSize > 0)
	{
		progress.setSubtaskWork((int32_t)times[3], "Subdividing");
		subdivider.subdivide(subdivisionSize, &progress);
		progress.completeSubtask();
	}

	Array<PxVec3> newVertices(subdivider.getNumVertices());
	Array<uint32_t> newMasterValues(subdivider.getNumVertices());
	for (uint32_t i = 0; i < newVertices.size(); i++)
	{
		subdivider.getVertex(i, newVertices[i], newMasterValues[i]);
	}

	Array<uint32_t> newIndices(subdivider.getNumTriangles() * 3);
	for (uint32_t i = 0; i < newIndices.size(); i += 3)
	{
		subdivider.getTriangle(i / 3, newIndices[i], newIndices[i + 1], newIndices[i + 2]);
	}

	physicalMesh->setGeometry(false, newVertices.size(), sizeof(PxVec3), newVertices.begin(), 
							newMasterValues.begin(), newIndices.size(), sizeof(uint32_t), &newIndices[0]);

	return physicalMesh;
}


bool ModuleClothingImpl::ClothingBackendFactory::isMatch(const char* simulationBackend)
{
	if (::strcmp(getName(), simulationBackend) == 0)
	{
		return true;
	}

	if (::strcmp(ClothingCookedPhysX3Param::staticClassName(), simulationBackend) == 0
	|| ::strcmp(ClothingCookedParam::staticClassName(), simulationBackend) == 0)
	{
		return true;
	}

	return false;
}

const char*	ModuleClothingImpl::ClothingBackendFactory::getName()
{
	return "Embedded";
}

uint32_t ModuleClothingImpl::ClothingBackendFactory::getCookingVersion()
{
	return Cooking::getCookingVersion();
}

uint32_t ModuleClothingImpl::ClothingBackendFactory::getCookedDataVersion(const NvParameterized::Interface* cookedData)
{
	if (cookedData != NULL && isMatch(cookedData->className()))
	{
		if 	(::strcmp(ClothingCookedParam::staticClassName(), cookedData->className()) == 0)
		{
			return static_cast<const ClothingCookedParam*>(cookedData)->cookedDataVersion;
		}
		else if (::strcmp(ClothingCookedPhysX3Param::staticClassName(), cookedData->className()) == 0)
		{
			return static_cast<const ClothingCookedPhysX3Param*>(cookedData)->cookedDataVersion;
		}
		else
		{
			PX_ALWAYS_ASSERT_MESSAGE("Mechanism to extract cooked data version is not defined and not implemented");
		}
	}

	return 0;
}

CookingAbstract* ModuleClothingImpl::ClothingBackendFactory::createCookingJob()
{
	bool withFibers = true;
	return PX_NEW(Cooking)(withFibers);
}

void ModuleClothingImpl::ClothingBackendFactory::releaseCookedInstances(NvParameterized::Interface* cookedData)
{
	Simulation::releaseFabric(cookedData);
}

SimulationAbstract* ModuleClothingImpl::ClothingBackendFactory::createSimulation(ClothingScene* clothingScene, bool useHW)
{
	return PX_NEW(Simulation)(clothingScene, useHW);
}

}
} // namespace nvidia
