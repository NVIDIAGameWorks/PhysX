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
#include "ApexScene.h"
#include "ApexSDKImpl.h"
#include "FrameworkPerfScope.h"
#include "ApexRenderMeshAsset.h"
#include "ApexRenderMeshActor.h"
#include "ApexRenderMeshAssetAuthoring.h"
#include "ApexResourceProvider.h"
#include "PsMemoryBuffer.h"
#include "ApexString.h"
#include "ApexDefaultStream.h"
#include "ApexRenderDebug.h"

#include "nvparameterized/NvParameterized.h"
#include "nvparameterized/NvParamUtils.h"

#include "PsMemoryBuffer.h"
#include "PsFoundation.h"
#include "NvDefaultTraits.h"

#include "PsFileBuffer.h"
#include "NvSerializerInternal.h"
#include "UserOpaqueMesh.h"
#include "ApexShape.h"

#include "ApexAssetPreviewScene.h"
#include "RenderResourceManagerWrapper.h"
#include "PsThread.h"

#include "PxProfileEventNames.h"
#include "ApexPvdClient.h"

#ifndef WITHOUT_PVD
#include "PvdNxParamSerializer.h"
#include "ApexStubPxProfileZone.h"
#endif

#define MAX_MSG_SIZE 65536
#define WITH_DEBUG_ASSET 0

#if PX_WINDOWS_FAMILY
#include <windows/PsWindowsInclude.h>
#include <cstdio>
#include "ModuleUpdateLoader.h"

#include <PxPvdImpl.h>

// We require at least Visual Studio 2010 w/ SP1 to compile
#if defined(_MSC_VER)
#	if _MSC_VER >= 1900
	PX_COMPILE_TIME_ASSERT(_MSC_FULL_VER >= 190000000);
#	elif _MSC_VER >= 1800
	PX_COMPILE_TIME_ASSERT(_MSC_FULL_VER >= 180000000);
#	elif _MSC_VER >= 1700
	PX_COMPILE_TIME_ASSERT(_MSC_FULL_VER >= 170000000);
#	elif _MSC_VER >= 1600
	PX_COMPILE_TIME_ASSERT(_MSC_FULL_VER >= 160040219);
#	endif

#	if _MSC_VER > 2000
	#pragma message("Detected compiler newer than Visual Studio 2017, please update min version checking in ApexSDKImpl.cpp")
	PX_COMPILE_TIME_ASSERT(_MSC_VER < 2000);
#	endif
#endif

#endif //PX_WINDOWS_FAMILY

#if PX_OSX
#include <mach-o/dyld.h>
#include <libproc.h>
#include <dlfcn.h>
#endif

#if PX_LINUX && APEX_LINUX_SHARED_LIBRARIES
#include <dlfcn.h>
#endif

#if PX_X86
#define PTR_TO_UINT64(x) ((uint64_t)(uint32_t)(x))
#else
#define PTR_TO_UINT64(x) ((uint64_t)(x))
#endif

#if APEX_CUDA_SUPPORT
#include "windows/PhysXIndicator.h"
#endif

#include "PxErrorCallback.h"
#include "PxCudaContextManager.h"
#include "PxCpuDispatcher.h"
#include "PxProfileZoneManager.h"
#ifdef PHYSX_PROFILE_SDK
#if PX_PHYSICS_VERSION_MAJOR == 3
#include "PxPhysics.h"
#endif

nvidia::profile::PxProfileZone *gProfileZone=NULL;

#endif

namespace nvidia
{
namespace apex
{


extern ApexSDKImpl* gApexSdk;

#if defined(_USRDLL) || PX_OSX || (PX_LINUX && APEX_LINUX_SHARED_LIBRARIES)
typedef Module* (NxCreateModule_FUNC)(ApexSDKIntl*, ModuleIntl**, uint32_t, uint32_t, ApexCreateError*);
#else
/* When modules are statically linked, the user must instantiate modules manually before they can be
 * created via the ApexSDK::createModule() method.  Each module must supply an instantiation function.
 */
#endif

ApexSDKImpl::ApexSDKImpl(ApexCreateError* errorCode, uint32_t /*inAPEXsdkVersion*/)
	: mAuthorableObjects(NULL)
	, mBatchSeedSize(128)
	, mErrorString(NULL)
	, mNumTempMemoriesActive(0)
#if PX_PHYSICS_VERSION_MAJOR == 0
	, mApexThreadPool(0)
#endif
	, renderResourceManager(NULL)
	, renderResourceManagerWrapper(NULL)
	, apexResourceProvider(NULL)
	, cookingVersion(0)
	, mURRdepthTLSslot(0xFFFFFFFF)
	, mEnableApexStats(true)
	, mEnableConcurrencyCheck(false)
{
	if (errorCode)
	{
		*errorCode = APEX_CE_NO_ERROR;
	}

#if PX_DEBUG || PX_CHECKED
	mURRdepthTLSslot = shdfnd::TlsAlloc();
#endif
}

AuthObjTypeID	ApexRenderMeshAsset::mObjTypeID;
#include "ModuleFrameworkRegistration.h"
#include "ModuleCommonRegistration.h"

void ModuleFramework::init(NvParameterized::Traits* t)
{
	ModuleFrameworkRegistration::invokeRegistration(t);
	ModuleCommonRegistration::invokeRegistration(t);
}

void ModuleFramework::release(NvParameterized::Traits* t)
{
	ModuleFrameworkRegistration::invokeUnregistration(t);
	ModuleCommonRegistration::invokeUnregistration(t);
}

// Many things can't be initialized in the constructor since they depend on gApexSdk
// being present.
void ApexSDKImpl::init(const ApexSDKDesc& desc)
{

	renderResourceManager = desc.renderResourceManager;
#if PX_DEBUG || PX_CHECKED
	if (renderResourceManagerWrapper != NULL)
	{
		PX_DELETE(renderResourceManagerWrapper);
		renderResourceManagerWrapper = NULL;
	}
	if (renderResourceManager != NULL)
	{
		renderResourceManagerWrapper = PX_NEW(RenderResourceManagerWrapper)(*renderResourceManager);
	}
#endif

	foundation = desc.foundation;

#if PX_PHYSICS_VERSION_MAJOR == 3
	cooking = desc.cooking;
	physXSDK = desc.physXSDK;
	physXsdkVersion = desc.physXSDKVersion;
#endif

	mDllLoadPath = desc.dllLoadPath;
	mCustomDllNamePostfix = desc.dllNamePostfix;
	mWireframeMaterial = desc.wireframeMaterial;
	mSolidShadedMaterial = desc.solidShadedMaterial;
#if PX_WINDOWS_FAMILY
	mAppGuid = desc.appGuid ? desc.appGuid : DEFAULT_APP_GUID;
#endif
	mRMALoadMaterialsLazily = desc.renderMeshActorLoadMaterialsLazily;

	mEnableConcurrencyCheck = desc.enableConcurrencyCheck;

	Framework::initFrameworkProfiling(this);

	apexResourceProvider = PX_NEW(ApexResourceProvider)();
	PX_ASSERT(apexResourceProvider);
	apexResourceProvider->setCaseSensitivity(desc.resourceProviderIsCaseSensitive);
	apexResourceProvider->registerCallback(desc.resourceCallback);

	// The param traits depend on the resource provider, so do this now
	mParameterizedTraits = new NvParameterized::DefaultTraits(NvParameterized::DefaultTraits::BehaviourFlags::DEFAULT_POLICY);
	GetInternalApexSDK()->getInternalResourceProvider()->createNameSpace("NvParameterizedFactories", false);
	PX_ASSERT(mParameterizedTraits);

	/* create global name space of authorable asset types */
	mObjTypeNS = apexResourceProvider->createNameSpace(APEX_AUTHORABLE_ASSETS_TYPES_NAME_SPACE, false);

	/* create global name space of NvParameterized authorable asset types */
	mNxParamObjTypeNS = apexResourceProvider->createNameSpace(APEX_NV_PARAM_AUTH_ASSETS_TYPES_NAME_SPACE, false);

	/* create namespace for user materials */
	mMaterialNS = apexResourceProvider->createNameSpace(APEX_MATERIALS_NAME_SPACE, true);

	/* create namespace for user opaque meshes */
	mOpaqueMeshNS = apexResourceProvider->createNameSpace(APEX_OPAQUE_MESH_NAME_SPACE, false);

	/* create namespace for custom vertex buffer semantics */
	mCustomVBNS = apexResourceProvider->createNameSpace(APEX_CUSTOM_VB_NAME_SPACE, true);

	/* create namespace for novodex collision groups */
	mCollGroupNS = apexResourceProvider->createNameSpace(APEX_COLLISION_GROUP_NAME_SPACE, false);

	/* create namespace for 128-bit GroupsMasks */
	mCollGroup128NS = apexResourceProvider->createNameSpace(APEX_COLLISION_GROUP_128_NAME_SPACE, false);

	/* create namespace for 64-bit GroupsMasks64 */
	mCollGroup64NS = apexResourceProvider->createNameSpace(APEX_COLLISION_GROUP_64_NAME_SPACE, false);

	/* create namespace for novodex collision groups masks */
	mCollGroupMaskNS = apexResourceProvider->createNameSpace(APEX_COLLISION_GROUP_MASK_NAME_SPACE, false);

	/* create namespace for novodex Material IDs (returned by raycasts) */
	mPhysMatNS = apexResourceProvider->createNameSpace(APEX_PHYSICS_MATERIAL_NAME_SPACE, false);

	/* create namespace for RenderMeshAssets */
	mAuthorableObjects = PX_NEW(ResourceList);
	RenderMeshAuthorableObject* AO = PX_NEW(RenderMeshAuthorableObject)(&frameworkModule, *mAuthorableObjects, RenderMeshAssetParameters::staticClassName());
	ApexRenderMeshAsset::mObjTypeID = AO->getResID();

	frameworkModule.init(mParameterizedTraits);

	/* Create mDebugColorParams */
	void* newPtr = mParameterizedTraits->alloc(sizeof(DebugColorParamsEx));
	mDebugColorParams = NV_PARAM_PLACEMENT_NEW(newPtr, DebugColorParamsEx)(mParameterizedTraits, this);

	for (uint32_t i = 0 ; i < DescHashSize ; i++)
	{
		mPhysXObjDescHash[i] = 0;
	}
	mDescFreeList = 0;

	mCachedData = PX_NEW(ApexSDKCachedDataImpl);

	mBatchSeedSize = desc.physXObjDescTableAllocationIncrement;

#if defined(PHYSX_PROFILE_SDK)
	mProfileZone = 0; // &physx::profile::PxProfileZone::createProfileZone(getAllocator(), "ApexSDK"); // TODO: create a profile zone here
	gProfileZone = mProfileZone;
	mApexPvdClient = pvdsdk::ApexPvdClient::create(desc.pvd);
#endif
}


ApexSDKImpl::~ApexSDKImpl()
{
#if PX_DEBUG || PX_CHECKED
	if (mURRdepthTLSslot != 0xFFFFFFFF)
	{
		TlsFree(mURRdepthTLSslot);
		mURRdepthTLSslot = 0xFFFFFFFF;
	}
#endif

	Framework::releaseFrameworkProfiling();
#if PHYSX_PROFILE_SDK
	if ( mProfileZone )
	{
		mProfileZone->release();
		mProfileZone = NULL;
	}
	if (mApexPvdClient != NULL)
	{
		mApexPvdClient->release();
	}
	mApexPvdClient = NULL;
#endif
}

ApexActor* ApexSDKImpl::getApexActor(Actor* nxactor) const
{
	AuthObjTypeID type = nxactor->getOwner()->getObjTypeID();
	if (type == ApexRenderMeshAsset::mObjTypeID)
	{
		return (ApexRenderMeshActor*) nxactor;
	}

	ApexActor* a = NULL;
	for (uint32_t i = 0; i < imodules.size(); i++)
	{
		a = imodules[i]->getApexActor(nxactor, type);
		if (a)
		{
			break;
		}
	}

	return a;
}

Scene* ApexSDKImpl::createScene(const SceneDesc& sceneDesc)
{
	if (!sceneDesc.isValid())
	{
		return 0;
	}

	ApexScene* s = PX_NEW(ApexScene)(sceneDesc, this);
	mScenes.pushBack(s);

	// Trigger ModuleSceneIntl creation for all loaded modules
	for (uint32_t i = 0; i < imodules.size(); i++)
	{
		s->moduleCreated(*imodules[i]);
	}

	return s;
}

AssetPreviewScene*   ApexSDKImpl::createAssetPreviewScene()
{
	ApexAssetPreviewScene* s = PX_NEW(ApexAssetPreviewScene)(this);
	
	return s;
}

void ApexSDKImpl::releaseScene(Scene* nxScene)
{
	ApexScene* scene = DYNAMIC_CAST(ApexScene*)(nxScene);
	mScenes.findAndReplaceWithLast(scene);
	scene->destroy();
}

void ApexSDKImpl::releaseAssetPreviewScene(AssetPreviewScene* nxScene)
{
	ApexAssetPreviewScene* scene = DYNAMIC_CAST(ApexAssetPreviewScene*)(nxScene);
	scene->destroy();
}

/** Map PhysX objects back to their APEX objects, hold flags and pointers **/

uint16_t ApexPhysXObjectDesc::makeHash(size_t hashable)
{
	return static_cast<uint16_t>(UINT16_MAX & (hashable >> 8));
}


PhysXObjectDescIntl* ApexSDKImpl::getGenericPhysXObjectInfo(const void* obj) const
{
	nvidia::Mutex::ScopedLock scopeLock(mPhysXObjDescsLock);

	uint16_t h = (uint16_t)(ApexPhysXObjectDesc::makeHash(reinterpret_cast<size_t>(obj)) & (DescHashSize - 1));
	uint32_t index = mPhysXObjDescHash[h];

	while (index)
	{
		ApexPhysXObjectDesc* desc = const_cast<ApexPhysXObjectDesc*>(&mPhysXObjDescs[index]);
		if ((void*) desc->mPhysXObject == obj)
		{
			return desc;
		}
		else
		{
			index = desc->mNext;
		}
	}
	return NULL;
}

#if PX_PHYSICS_VERSION_MAJOR == 3

PhysXObjectDescIntl* 	ApexSDKImpl::createObjectDesc(const Actor* apexActor, const PxActor* actor)
{
	return createObjectDesc(apexActor, (const void*) actor);
}
PhysXObjectDescIntl* 	ApexSDKImpl::createObjectDesc(const Actor* apexActor, const PxShape* shape)
{
	return createObjectDesc(apexActor, (const void*) shape);
}
PhysXObjectDescIntl* 	ApexSDKImpl::createObjectDesc(const Actor* apexActor, const PxJoint* joint)
{
	return createObjectDesc(apexActor, (const void*) joint);
}
PhysXObjectDescIntl* ApexSDKImpl::createObjectDesc(const Actor* apexActor, const PxCloth* cloth)
{
	return createObjectDesc(apexActor, (const void*)cloth);
}
PhysXObjectDescIntl* 	ApexSDKImpl::createObjectDesc(const Actor* apexActor, const PxParticleSystem* particleSystem)
{
	return createObjectDesc(apexActor, (const void*) particleSystem);
}
PhysXObjectDescIntl* ApexSDKImpl::createObjectDesc(const Actor* apexActor, const PxParticleFluid* particleFluid)
{
	return createObjectDesc(apexActor, (const void*)particleFluid);
}
const PhysXObjectDesc* ApexSDKImpl::getPhysXObjectInfo(const PxActor* actor) const
{
	return getGenericPhysXObjectInfo((void*)actor);
}
const PhysXObjectDesc* ApexSDKImpl::getPhysXObjectInfo(const PxShape* shape) const
{
	return getGenericPhysXObjectInfo((void*)shape);
}
const PhysXObjectDesc* ApexSDKImpl::getPhysXObjectInfo(const PxJoint* joint) const
{
	return getGenericPhysXObjectInfo((void*)joint);
}
const PhysXObjectDesc* ApexSDKImpl::getPhysXObjectInfo(const PxCloth* cloth) const
{
	return getGenericPhysXObjectInfo((void*)cloth);
}
const PhysXObjectDesc* ApexSDKImpl::getPhysXObjectInfo(const PxParticleSystem* particleSystem) const
{
	return getGenericPhysXObjectInfo((void*)particleSystem);
}
const PhysXObjectDesc* ApexSDKImpl::getPhysXObjectInfo(const PxParticleFluid* particleFluid) const
{
	return getGenericPhysXObjectInfo((void*)particleFluid);
}
PxPhysics* ApexSDKImpl::getPhysXSDK()
{
	return physXSDK;
}
PxCooking* ApexSDKImpl::getCookingInterface()
{
	return cooking;
}

#endif

AuthObjTypeID ApexSDKImpl::registerAuthObjType(const char* authTypeName, ResID nsid)
{
	AuthObjTypeID aotid = apexResourceProvider->createResource(mObjTypeNS, authTypeName, false);
	apexResourceProvider->setResource(APEX_AUTHORABLE_ASSETS_TYPES_NAME_SPACE,
	                                  authTypeName,
	                                  (void*)(size_t) nsid, false);
	return aotid;
}

AuthObjTypeID ApexSDKImpl::registerAuthObjType(const char* authTypeName, AuthorableObjectIntl* authObjPtr)
{
	AuthObjTypeID aotid = apexResourceProvider->createResource(mObjTypeNS, authTypeName, false);
	apexResourceProvider->setResource(APEX_AUTHORABLE_ASSETS_TYPES_NAME_SPACE,
	                                  authTypeName,
	                                  (void*) authObjPtr, false);
	return aotid;
}

AuthObjTypeID ApexSDKImpl::registerNvParamAuthType(const char* authTypeName, AuthorableObjectIntl* authObjPtr)
{
	AuthObjTypeID aotid = apexResourceProvider->createResource(mNxParamObjTypeNS, authTypeName, false);
	apexResourceProvider->setResource(APEX_NV_PARAM_AUTH_ASSETS_TYPES_NAME_SPACE,
	                                  authTypeName,
	                                  (void*) authObjPtr, false);
	return aotid;
}

void ApexSDKImpl::unregisterAuthObjType(const char* authTypeName)
{
	apexResourceProvider->setResource(APEX_AUTHORABLE_ASSETS_TYPES_NAME_SPACE,
	                                  authTypeName,
	                                  (void*) NULL, false);
}

void ApexSDKImpl::unregisterNvParamAuthType(const char* authTypeName)
{
	apexResourceProvider->setResource(APEX_NV_PARAM_AUTH_ASSETS_TYPES_NAME_SPACE,
	                                  authTypeName,
	                                  (void*) NULL, false);
}

AuthorableObjectIntl*	ApexSDKImpl::getAuthorableObject(const char* authTypeName)
{
	if (!apexResourceProvider->checkResource(mObjTypeNS, authTypeName))
	{
		return NULL;
	}

	void* ao = apexResourceProvider->getResource(APEX_AUTHORABLE_ASSETS_TYPES_NAME_SPACE, authTypeName);

	return static_cast<AuthorableObjectIntl*>(ao);
}

AuthorableObjectIntl*	ApexSDKImpl::getParamAuthObject(const char* paramName)
{
	if (!apexResourceProvider->checkResource(mNxParamObjTypeNS, paramName))
	{
		return NULL;
	}

	void* ao = apexResourceProvider->getResource(APEX_NV_PARAM_AUTH_ASSETS_TYPES_NAME_SPACE, paramName);

	return static_cast<AuthorableObjectIntl*>(ao);
}

bool ApexSDKImpl::getAuthorableObjectNames(const char** authTypeNames, uint32_t& outCount, uint32_t inCount)
{
	ResID ids[128];

	if (!apexResourceProvider->getResourceIDs(APEX_AUTHORABLE_ASSETS_TYPES_NAME_SPACE, ids, outCount, 128))
	{
		return false;
	}

	if (outCount > inCount)
	{
		return false;
	}

	for (uint32_t i = 0; i < outCount; i++)
	{
		authTypeNames[i] = apexResourceProvider->getResourceName(ids[i]);

#define JUST_A_TEST 0
#if JUST_A_TEST
#include <stdio.h>
		AuthorableObjectIntl* ao = (AuthorableObjectIntl*)getAuthorableObject(authTypeNames[i]);

		Asset* assetList[32];
		uint32_t retCount = 0;
		ao->getAssetList(assetList, retCount, 32);

		if (retCount)
		{
			printf("%s count: %d\n", authTypeNames[i], retCount);
			const NvParameterized::Interface* p = assetList[0]->getAssetNvParameterized();
			if (p)
			{
				printf(" NvParam class name: %s\n", p->className());
			}
		}
#endif
	}

	return true;
}

ResID ApexSDKImpl::getApexMeshNameSpace()
{
	AuthorableObjectIntl* AO = getAuthorableObject(RENDER_MESH_AUTHORING_TYPE_NAME);
	if (AO)
	{
		return AO->getResID();
	}
	else
	{
		return INVALID_RESOURCE_ID;
	}
}



PhysXObjectDescIntl* 	ApexSDKImpl::createObjectDesc(const Actor* apexActor, const void* nxPtr)
{
	nvidia::Mutex::ScopedLock scopeLock(mPhysXObjDescsLock);

	uint16_t h = (uint16_t)(ApexPhysXObjectDesc::makeHash(reinterpret_cast<size_t>(nxPtr)) & (DescHashSize - 1));
	uint32_t index = mPhysXObjDescHash[ h ];

	while (index)
	{
		ApexPhysXObjectDesc* desc = &mPhysXObjDescs[ index ];
		if (desc->mPhysXObject == nxPtr)
		{
			APEX_DEBUG_WARNING("createObjectDesc: Object already registered");
			bool hasActor = false;
			for (uint32_t i = desc->mApexActors.size(); i--;)
			{
				if (desc->mApexActors[i] == apexActor)
				{
					hasActor = true;
					break;
				}
			}
			if (hasActor)
			{
				APEX_DEBUG_WARNING("createObjectDesc: Object already registered with the given Actor");
			}
			else
			{
				desc->mApexActors.pushBack(apexActor);
			}
			return desc;
		}
		else
		{
			index = desc->mNext;
		}
	}

	// Match not found, allocate new object descriptor

	if (!mDescFreeList)
	{
		// Free list is empty, seed it with new batch
		uint32_t size = mPhysXObjDescs.size();
		if (size == 0)  // special initial case, reserve entry 0
		{
			size = 1;
			mPhysXObjDescs.resize(size + mBatchSeedSize);
		}
		else
		{
			PX_PROFILE_ZONE("objDescsResize", GetInternalApexSDK()->getContextId());

			// Instead of doing a straight resize of mPhysXObjDescs the array is resized by swapping. Doing so removes the potential 
			// copying/reallocating of the arrays held in ApexPhysXObjectDesc elements which is costly performance wise.
			physx::Array<ApexPhysXObjectDesc> swapArray;
			swapArray.swap(mPhysXObjDescs);

			mPhysXObjDescs.resize(size + mBatchSeedSize);
			ApexPhysXObjectDesc* src = swapArray.begin();
			ApexPhysXObjectDesc* dst = mPhysXObjDescs.begin();
			for (physx::PxU32 i = 0; i < size; i++)
			{
				src[i].swap(dst[i]);
			}
		}

		for (uint32_t i = size ; i < size + mBatchSeedSize ; i++)
		{
			mPhysXObjDescs[i].mNext = mDescFreeList;
			mDescFreeList = i;
		}
	}

	index = mDescFreeList;
	ApexPhysXObjectDesc* desc = &mPhysXObjDescs[ index ];
	mDescFreeList = desc->mNext;

	desc->mFlags = 0;
	desc->userData = NULL;
	desc->mApexActors.reset();
	desc->mApexActors.pushBack(apexActor);
	desc->mNext = mPhysXObjDescHash[ h ];
	if (desc->mNext)
	{
		PX_ASSERT(mPhysXObjDescs[ desc->mNext ].mPrev == 0);
		mPhysXObjDescs[ desc->mNext ].mPrev = index;
	}
	desc->mPrev = 0;
	desc->mPhysXObject = nxPtr;
	mPhysXObjDescHash[ h ] = index;

	/* Calling function can set mFlags and userData */

	return desc;
}

void ApexSDKImpl::releaseObjectDesc(void* physXObject)
{
	nvidia::Mutex::ScopedLock scopeLock(mPhysXObjDescsLock);

	uint16_t h = (uint16_t)(ApexPhysXObjectDesc::makeHash(reinterpret_cast<size_t>(physXObject)) & (DescHashSize - 1));
	uint32_t index = mPhysXObjDescHash[ h ];

	while (index)
	{
		ApexPhysXObjectDesc* desc = &mPhysXObjDescs[ index ];

		if (desc->mPhysXObject == physXObject)
		{
			if (desc->mPrev)
			{
				mPhysXObjDescs[ desc->mPrev ].mNext = desc->mNext;
			}
			else
			{
				mPhysXObjDescHash[ h ] = desc->mNext;
			}

			if (desc->mNext)
			{
				mPhysXObjDescs[ desc->mNext ].mPrev = desc->mPrev;
			}

			desc->mNext = mDescFreeList;
			mDescFreeList = index;

			desc->mApexActors.reset();
			return;
		}
		else
		{
			index = desc->mNext;
		}
	}

	APEX_DEBUG_WARNING("releaseObjectDesc: Unable to release object descriptor");
}


void ApexSDKImpl::releaseModule(Module* module)
{
	for (uint32_t i = 0; i < modules.size(); i++)
	{
		if (modules[i] != module)
		{
			continue;
		}

		// The module will remove its ModuleScenesIntl from each Scene
		mCachedData->unregisterModuleDataCache(imodules[ i ]->getModuleDataCache());

		ModuleIntl *im = imodules[i];
		imodules[i] = NULL;
		modules[i] = NULL;
		im->destroy();

//		modules.replaceWithLast(i);
//		imodules.replaceWithLast(i);

		break;
	}
}

void ApexSDKImpl::registerModule(Module* newModule, ModuleIntl* newIModule)
{
	
	uint32_t newIndex = modules.size();
	for (uint32_t i=0; i<newIndex; i++)
	{
		if ( imodules[i] == NULL )
		{
			newIndex = i;
			break;
		}
	}
	if ( newIndex == modules.size() )
	{
		modules.pushBack(newModule);
		imodules.pushBack(newIModule);
	}

	// Trigger ModuleSceneIntl creation for all existing scenes
	for (uint32_t i = 0 ; i < mScenes.size(); i++)
	{
		(DYNAMIC_CAST(ApexScene*)(mScenes[i]))->moduleCreated(*newIModule);
	}

	mCachedData->registerModuleDataCache(newIModule->getModuleDataCache());
}

Module* ApexSDKImpl::createModule(const char* name, ApexCreateError* err)
{
	if (err)
	{
		*err = APEX_CE_NO_ERROR;
	}

	// Return existing module if it's already loaded
	for (uint32_t i = 0; i < modules.size(); i++)
	{
		if ( modules[i] && !nvidia::strcmp(modules[ i ]->getName(), name))
		{
			ModuleIntl *imodule = imodules[i];
			if( imodule->isCreateOk() )
			{
				return modules[ i ];
			}
			else
			{
				APEX_DEBUG_WARNING("ApexSDKImpl::createModule(%s) Not allowed.", name );
				if (err)
				{
					*err = APEX_CE_CREATE_NO_ALLOWED;
				}
				return NULL;
			}
		}
	}

	Module* newModule = NULL;
	ModuleIntl* newIModule = NULL;

#if defined(_USRDLL) || PX_OSX || (PX_LINUX && APEX_LINUX_SHARED_LIBRARIES)
	/* Dynamically linked module libraries */

#if defined(WIN32)
	ApexSimpleString dllName = mDllLoadPath + ApexSimpleString("APEX_") + ApexSimpleString(name);
#if _DEBUG
	// Request DEBUG DLL unless the user has explicitly asked for it
	const size_t nameLen = strlen(name);
	if (nameLen <= 5 || nvidia::strcmp(name + nameLen - 5, "DEBUG"))
	{
		dllName += ApexSimpleString("DEBUG");
	}
#elif PX_CHECKED
	dllName += ApexSimpleString("CHECKED");
#elif defined(PHYSX_PROFILE_SDK)
	dllName += ApexSimpleString("PROFILE");
#endif

#if PX_X86
	dllName += ApexSimpleString("_x86");
#elif PX_X64
	dllName += ApexSimpleString("_x64");
#endif

	dllName += mCustomDllNamePostfix;

	dllName += ApexSimpleString(".dll");

	HMODULE library = NULL;
	{
		ModuleUpdateLoader moduleLoader(UPDATE_LOADER_DLL_NAME);
		library = moduleLoader.loadModule(dllName.c_str(), getAppGuid());

		if (NULL == library)
		{
			dllName = ApexSimpleString("APEX/") + dllName;
			library = moduleLoader.loadModule(dllName.c_str(), getAppGuid());
		}
	}

	if (library)
	{
		NxCreateModule_FUNC* createModuleFunc = (NxCreateModule_FUNC*) GetProcAddress(library, "createModule");
		if (createModuleFunc)
		{
			newModule = createModuleFunc((ApexSDKIntl*) this,
			                             &newIModule,
			                             APEX_SDK_VERSION,
			                             PX_PHYSICS_VERSION,
			                             err);
		}
	}
#elif PX_OSX
	ApexSimpleString dylibName = ApexSimpleString("libAPEX_") + ApexSimpleString(name);

#if _DEBUG
	// Request DEBUG DLL unless the user has explicitly asked for it
	const size_t nameLen = strlen(name);
	if (nameLen <= 5 || nvidia::strcmp(name + nameLen - 5, "DEBUG"))
	{
		dylibName += ApexSimpleString("DEBUG");
	}
#elif PX_CHECKED
	dylibName += ApexSimpleString("CHECKED");
#elif defined(PHYSX_PROFILE_SDK)
	dylibName += ApexSimpleString("PROFILE");
#endif

	dylibName += mCustomDllNamePostfix;

	dylibName += ApexSimpleString(".dylib");

	ApexSimpleString dylibPath = mDllLoadPath + dylibName;

	void* library = NULL;
	{
		// Check if dylib is already loaded
		library = dlopen(dylibPath.c_str(), RTLD_NOLOAD | RTLD_LAZY | RTLD_LOCAL);
		if (!library)
		{
			library = dlopen((ApexSimpleString("@rpath/") + dylibName).c_str(), RTLD_NOLOAD | RTLD_LAZY | RTLD_LOCAL);
		}
		if (!library)
		{
			// Not loaded yet, so try to open it
			library = dlopen(dylibPath.c_str(), RTLD_LAZY | RTLD_LOCAL);
		}
	}

	if (library)
	{
		NxCreateModule_FUNC* createModuleFunc = (NxCreateModule_FUNC*)dlsym(library, "createModule");
		if (createModuleFunc)
		{
			newModule = createModuleFunc((ApexSDKIntl*) this,
			                             &newIModule,
			                             APEX_SDK_VERSION,
			                             PX_PHYSICS_VERSION,
			                             err);
		}
	}
#elif (PX_LINUX && APEX_LINUX_SHARED_LIBRARIES)
	ApexSimpleString soName = ApexSimpleString("libAPEX_") + ApexSimpleString(name);

#if _DEBUG
	// Request DEBUG so unless the user has explicitly asked for it
	const size_t nameLen = strlen(name);
	if (nameLen <= 5 || nvidia::strcmp(name + nameLen - 5, "DEBUG"))
	{
		soName += ApexSimpleString("DEBUG");
	}
#elif PX_CHECKED
	soName += ApexSimpleString("CHECKED");
#elif defined(PHYSX_PROFILE_SDK)
	soName += ApexSimpleString("PROFILE");
#endif
	soName += mCustomDllNamePostfix;

	soName += ApexSimpleString(".so");

	ApexSimpleString soPath = mDllLoadPath + soName;

	void* library = NULL;
	{
		// Check if so is already loaded
		library = dlopen(soPath.c_str(), RTLD_NOLOAD | RTLD_LAZY | RTLD_LOCAL);
		if (!library)
		{
			// Not loaded yet, so try to open it
			library = dlopen(soPath.c_str(), RTLD_LAZY | RTLD_LOCAL);
		}
		if (!library)
		{
			// Still not loaded yet, so lets just try the soName
			library = dlopen(soName.c_str(), RTLD_LAZY | RTLD_LOCAL);
		}
	}

	if (library)
	{
		NxCreateModule_FUNC* createModuleFunc = (NxCreateModule_FUNC*)dlsym(library, "createModule");
		if (createModuleFunc)
		{
			newModule = createModuleFunc((ApexSDKIntl*) this,
				&newIModule,
				APEX_SDK_VERSION,
				PX_PHYSICS_VERSION,
				err);
		}
	}
#else
	/* TODO: other platform dynamic linking? */
#endif

#else
	/* Statically linked module libraries */

	/* Modules must supply an instantiation function which calls ApexSDKIntl::registerModule()
	 * The user must call this function after creating ApexSDKImpl and before createModule().
	 */
#endif

	// register new module and its parameters
	if (newModule)
	{
		registerModule(newModule, newIModule);
	}
	else if (err)
	{
		*err = APEX_CE_NOT_FOUND;
	}

	return newModule;
}

ModuleIntl* ApexSDKImpl::getInternalModuleByName(const char* name)
{
	// Return existing module if it's already loaded
	for (uint32_t i = 0; i < modules.size(); i++)
	{
		if (!nvidia::strcmp(modules[ i ]->getName(), name))
		{
			return imodules[ i ];
		}
	}
	return NULL;
}

PxFileBuf* ApexSDKImpl::createStream(const char* filename, PxFileBuf::OpenMode mode)
{
	return PX_NEW(PsFileBuffer)(filename, mode);
}

// deprecated, use getErrorCallback instead
PxErrorCallback* ApexSDKImpl::getOutputStream()
{
	return getErrorCallback();
}

PxFoundation* ApexSDKImpl::getFoundation() const
{
	return foundation;
}

PxErrorCallback* ApexSDKImpl::getErrorCallback() const
{
	PX_ASSERT(foundation);
	return &foundation->getErrorCallback();
}

PxAllocatorCallback* ApexSDKImpl::getAllocator() const
{
	PX_ASSERT(foundation);
	return &foundation->getAllocatorCallback();
}

ResourceProvider* ApexSDKImpl::getNamedResourceProvider()
{
	return apexResourceProvider;
}

ResourceProviderIntl* ApexSDKImpl::getInternalResourceProvider()
{
	return apexResourceProvider;
}

uint32_t ApexSDKImpl::getNbModules()
{
	uint32_t moduleCount = 0;
	for (uint32_t i=0; i<modules.size(); i++)
	{
		if (modules[i] != NULL)
		{
			moduleCount++;
		}
	}

	return moduleCount;
}

Module** ApexSDKImpl::getModules()
{
	if (modules.size() > 0)
	{
		moduleListForAPI.resize(0);
		for (uint32_t i=0; i<modules.size(); i++)
		{
			if (modules[i] != NULL)
			{
				moduleListForAPI.pushBack(modules[i]);
			}
		}
		
		return &moduleListForAPI.front();
	}
	else
	{
		return NULL;
	}
}


ModuleIntl** ApexSDKImpl::getInternalModules()
{
	if (imodules.size() > 0)
	{
		return &imodules.front();
	}
	else
	{
		return NULL;
	}
}

uint32_t ApexSDKImpl::forceLoadAssets()
{
	uint32_t loadedAssetCount = 0;

	// handle render meshes, since they don't live in a module
	if (mAuthorableObjects != NULL)
	{
		for (uint32_t i = 0; i < mAuthorableObjects->getSize(); i++)
		{
			AuthorableObjectIntl* ao = static_cast<AuthorableObjectIntl*>(mAuthorableObjects->getResource(i));
			loadedAssetCount += ao->forceLoadAssets();
		}
	}

	for (uint32_t i = 0; i < imodules.size(); i++)
	{
		loadedAssetCount += imodules[i]->forceLoadAssets();
	}

	return loadedAssetCount;
}


void ApexSDKImpl::debugAsset(Asset* asset, const char* name)
{
	PX_UNUSED(asset);
	PX_UNUSED(name);
#if WITH_DEBUG_ASSET
	if (asset)
	{
		const NvParameterized::Interface* pm = asset->getAssetNvParameterized();
		if (pm)
		{
			NvParameterized::Serializer* s1 = internalCreateSerializer(NvParameterized::Serializer::NST_XML, mParameterizedTraits);
			NvParameterized::Serializer* s2 = internalCreateSerializer(NvParameterized::Serializer::NST_BINARY, mParameterizedTraits);
			if (s1 && s2)
			{
				nvidia::PsMemoryBuffer mb1;
				nvidia::PsMemoryBuffer mb2;
				s1->serialize(mb1, &pm, 1);
				s2->serialize(mb2, &pm, 1);
				{
					char scratch[512];
					nvidia::strlcpy(scratch, 512, name);
					char* dot = NULL;
					char* scan = scratch;
					while (*scan)
					{
						if (*scan == '/')
						{
							*scan = '_';
						}
						if (*scan == '\\')
						{
							*scan = '_';
						}
						if (*scan == '.')
						{
							dot = scan;
						}
						scan++;
					}

					if (dot)
					{
						*dot = 0;
					}

					nvidia::strlcat(scratch, 512, ".apx");
					FILE* fph = fopen(scratch, "wb");
					if (fph)
					{
						fwrite(mb1.getWriteBuffer(), mb1.getWriteBufferSize(), 1, fph);
						fclose(fph);
					}
					if (dot)
					{
						*dot = 0;
					}

					nvidia::strlcat(scratch, 512, ".apb");
					fph = fopen(scratch, "wb");
					if (fph)
					{
						fwrite(mb2.getWriteBuffer(), mb2.getWriteBufferSize(), 1, fph);
						fclose(fph);
					}

				}
				s1->release();
				s2->release();
			}
		}
	}
#endif
}

/**
 *	checkAssetName
 *	-If name is NULL, we'll autogenerate one that won't collide with other names and issue a warning
 *	-If name collides with another name, we'll issue a warning and return NULL, so as not to confuse the
 *	 user by creating an asset authoring with a name that's different from the name specified.
 */
const char* ApexSDKImpl::checkAssetName(AuthorableObjectIntl& ao, const char* inName, ApexSimpleString& autoNameStorage)
{
	ResourceProviderIntl* iNRP = getInternalResourceProvider();

	if (!inName)
	{
		autoNameStorage = ao.getName();
		iNRP->generateUniqueName(ao.getResID(), autoNameStorage);

		APEX_DEBUG_INFO("No name provided for asset, auto-naming <%s>.", autoNameStorage.c_str());
		return autoNameStorage.c_str();
	}

	if (iNRP->checkResource(ao.getResID(), inName))
	{
		// name collides with another asset [author]
		APEX_DEBUG_WARNING("Name provided collides with another asset in the %s namespace: <%s>, no asset created.", ao.getName().c_str(), inName);

		return NULL;
	}

	return inName;
}

/**
 *	createAsset
 *	This method will load *any* APEX asset.
 *  1. Read the APEX serialization header
 *	2. Determine the correct module
 *	3. Pass the remainder of the stream to the module along with the asset type name and asset version
 */

Asset* ApexSDKImpl::createAsset(AssetAuthoring& nxAssetAuthoring, const char* name)
{
	Asset* ret = NULL;
	AuthorableObjectIntl* ao = getAuthorableObject(nxAssetAuthoring.getObjTypeName());
	if (ao)
	{
		ApexSimpleString autoName;
		name = checkAssetName(*ao, name, autoName);
		if (!name)
		{
			return NULL;
		}

		ret = ao->createAsset(nxAssetAuthoring, name);
	}
	else
	{
		APEX_INTERNAL_ERROR("Unknown authorable type: %s, please load all required modules.", nxAssetAuthoring.getObjTypeName());
	}
	debugAsset(ret, name);
	return ret;
}

Asset* ApexSDKImpl::createAsset(NvParameterized::Interface* params, const char* name)
{
	Asset* ret = NULL;
	// params->className() will tell us the name of the parameterized struct
	// there is a mapping of parameterized structs to
	PX_ASSERT(params);
	if (params)
	{
		AuthorableObjectIntl* ao = getParamAuthObject(params->className());
		if (ao)
		{
			ApexSimpleString autoName;
			name = checkAssetName(*ao, name, autoName);
			if (!name)
			{
				return NULL;
			}

			ret = ao->createAsset(params, name);
		}
		else
		{
			APEX_INTERNAL_ERROR("Unknown authorable type: %s, please load all required modules.", params->className());
		}
	}
	debugAsset(ret, name);
	return ret;
}

AssetAuthoring* ApexSDKImpl::createAssetAuthoring(const char* aoTypeName)
{
	AuthorableObjectIntl* ao = getAuthorableObject(aoTypeName);
	if (ao)
	{
		ApexSimpleString autoName;
		const char* name = 0;
		name = checkAssetName(*ao, name, autoName);
		if (!name)
		{
			return NULL;
		}


		return ao->createAssetAuthoring(name);
	}
	else
	{
		APEX_INTERNAL_ERROR("Unknown authorable type: %s, please load all required modules.", aoTypeName);
	}

	return NULL;
}

AssetAuthoring* ApexSDKImpl::createAssetAuthoring(const char* aoTypeName, const char* name)
{
	AuthorableObjectIntl* ao = getAuthorableObject(aoTypeName);
	if (ao)
	{
		ApexSimpleString autoName;
		name = checkAssetName(*ao, name, autoName);
		if (!name)
		{
			return NULL;
		}

		return ao->createAssetAuthoring(name);
	}
	else
	{
		APEX_INTERNAL_ERROR("Unknown authorable type: %s, please load all required modules.", aoTypeName);
	}

	return NULL;
}

AssetAuthoring* ApexSDKImpl::createAssetAuthoring(NvParameterized::Interface* params, const char* name)
{
	PX_ASSERT(params);
	if (!params)
	{
		APEX_DEBUG_WARNING("NULL NvParameterized Interface, no asset author created.");
		return NULL;
	}

	AuthorableObjectIntl* ao = getParamAuthObject(params->className());
	if (ao)
	{
		ApexSimpleString autoName;
		name = checkAssetName(*ao, name, autoName);
		if (!name)
		{
			return NULL;
		}

		return ao->createAssetAuthoring(params, name);
	}
	else
	{
		APEX_INTERNAL_ERROR("Unknown authorable type: %s, please load all required modules.", params->className());
	}

	return NULL;
}

/**
 *	releaseAsset
 *
 */
void ApexSDKImpl::releaseAsset(Asset& nxasset)
{
	AuthorableObjectIntl* ao = getAuthorableObject(nxasset.getObjTypeName());
	if (ao)
	{
		return ao->releaseAsset(nxasset);
	}
	else
	{
		APEX_INTERNAL_ERROR("Unknown authorable type: %s, please load all required modules.", nxasset.getObjTypeName());
	}
}

void ApexSDKImpl::releaseAssetAuthoring(AssetAuthoring& nxAssetAuthoring)
{
	AuthorableObjectIntl* ao = getAuthorableObject(nxAssetAuthoring.getObjTypeName());
	if (ao)
	{
		return ao->releaseAssetAuthoring(nxAssetAuthoring);
	}
	else
	{
		APEX_INTERNAL_ERROR("Unknown authorable type: %s, please load all required modules.", nxAssetAuthoring.getObjTypeName());
	}
}

void ApexSDKImpl::reportError(PxErrorCode::Enum code, const char* file, int line, const char* functionName, const char* msgFormat, ...)
{
	mReportErrorLock.lock();

	if (gApexSdk)
	{
		if (getErrorCallback())
		{
			if (mErrorString == NULL && code != PxErrorCode::eOUT_OF_MEMORY)
			{
				mErrorString = (char*) PX_ALLOC(sizeof(char) * MAX_MSG_SIZE, PX_DEBUG_EXP("char"));
			}
			if (mErrorString != NULL)
			{
				va_list va;
				va_start(va, msgFormat);

				size_t tempLength = 0;
				if (functionName != NULL)
				{
					shdfnd::snprintf(mErrorString,MAX_MSG_SIZE, "%s: ", functionName);
					tempLength = strlen(mErrorString);
				}

				vsprintf(mErrorString + tempLength, msgFormat, va);
				va_end(va);
				getErrorCallback()->reportError(code, mErrorString, file, line);
			}
			else
			{
				// we can't allocate any memory anymore, let's hope the stack has still a bit of space
				char buf[ 100 ];
				va_list va;
				va_start(va, msgFormat);
				vsprintf(buf, msgFormat, va);
				va_end(va);
				getErrorCallback()->reportError(code, buf, file, line);
			}
		}
	}
	mReportErrorLock.unlock();
}



void* ApexSDKImpl::getTempMemory(uint32_t size)
{
	mTempMemoryLock.lock();

	if (size == 0 || mTempMemories.size() > 100)	//later growing a size 0 allocation is not handled gracefully!
	{
		// this is most likely a leak in temp memory consumption
		mTempMemoryLock.unlock();
		return NULL;
	}

	// now find the smallest one that is bigger than 'size'
	int32_t found = -1;
	uint32_t bestSize = 0;
	for (uint32_t i = mNumTempMemoriesActive; i < mTempMemories.size(); i++)
	{
		if (mTempMemories[i].size >= size)
		{
			if (found == -1 || bestSize > mTempMemories[i].size)
			{
				found = (int32_t)i;
				bestSize = mTempMemories[i].size;
			}
		}
	}

	TempMemory result;

	if (found != -1)
	{
		// found
		if ((uint32_t)found > mNumTempMemoriesActive)
		{
			// swap them
			TempMemory temp = mTempMemories[mNumTempMemoriesActive];
			mTempMemories[mNumTempMemoriesActive] = mTempMemories[(uint32_t)found];
			mTempMemories[(uint32_t)found] = temp;
		}
		PX_ASSERT(mTempMemories[mNumTempMemoriesActive].used == 0);
		mTempMemories[mNumTempMemoriesActive].used = size;
		result = mTempMemories[mNumTempMemoriesActive];
		mNumTempMemoriesActive++;
	}
	else if (mNumTempMemoriesActive < mTempMemories.size())
	{
		// not found, use last one

		// swap
		TempMemory temp = mTempMemories.back();
		mTempMemories.back() = mTempMemories[mNumTempMemoriesActive];

		void* nb = PX_ALLOC(size, PX_DEBUG_EXP("ApexSDKImpl::getTempMemory"));
		if (nb)
		{
			memcpy(nb, temp.memory, PxMin(temp.size, size));
		}
		PX_FREE(temp.memory);
		temp.memory = nb;
		temp.size = size;
		PX_ASSERT(temp.used == 0);
		temp.used = size;

		mTempMemories[mNumTempMemoriesActive] = temp;
		result = temp;
		mNumTempMemoriesActive++;
	}
	else
	{
		mNumTempMemoriesActive++;
		TempMemory& newTemp = mTempMemories.insert();
		newTemp.memory = PX_ALLOC(size, PX_DEBUG_EXP("ApexSDKImpl::getTempMemory"));
		newTemp.size = size;
		newTemp.used = size;
		result = newTemp;
	}
	mTempMemoryLock.unlock();

#ifdef _DEBUG
	if (result.used < result.size)
	{
		memset((char*)result.memory + result.used, 0xfd, result.size - result.used);
	}
#endif

	return result.memory;
}


void ApexSDKImpl::releaseTempMemory(void* data)
{
	if (data == NULL)		//this is a valid consequence of permittion 0 sized allocations.
	{
		return;
	}

	mTempMemoryLock.lock();
	uint32_t numReleased = 0;

	for (uint32_t i = 0; i < mNumTempMemoriesActive; i++)
	{
		if (mTempMemories[i].memory == data)
		{
			PX_ASSERT(mTempMemories[i].used > 0);
#ifdef _DEBUG
			if (mTempMemories[i].used < mTempMemories[i].size)
			{
				for (uint32_t j = mTempMemories[i].used; j < mTempMemories[i].size; j++)
				{
					unsigned char cur = ((unsigned char*)mTempMemories[i].memory)[j];
					if (cur != 0xfd)
					{
						PX_ASSERT(cur == 0xfd);
						break; // only hit this assert once per error
					}
				}
			}
			// you should not operate on data that has been released!
			memset(mTempMemories[i].memory, 0xcd, mTempMemories[i].size);
#endif
			mTempMemories[i].used = 0;

			// swap with last valid one
			if (i < mNumTempMemoriesActive - 1)
			{
				TempMemory temp = mTempMemories[mNumTempMemoriesActive - 1];
				mTempMemories[mNumTempMemoriesActive - 1] = mTempMemories[i];
				mTempMemories[i] = temp;
			}
			mNumTempMemoriesActive--;

			numReleased++;
			break;
		}
	}

	PX_ASSERT(numReleased == 1);

	mTempMemoryLock.unlock();
}


void ApexSDKImpl::release()
{
	if (renderResourceManagerWrapper != NULL)
	{
		PX_DELETE(renderResourceManagerWrapper);
		renderResourceManagerWrapper = NULL;
	}

	for (uint32_t i = 0; i < mScenes.size(); i++)
	{
		(DYNAMIC_CAST(ApexScene*)(mScenes[ i ]))->destroy();
	}
	mScenes.clear();

	// Notify all modules that the ApexSDKImpl is getting destructed
	for (uint32_t i = 0; i < modules.size(); i++)
	{
		if ( imodules[i] )
		{
			imodules[ i ]->notifyReleaseSDK();
		}
	}

	// Now we destroy each module; but we make sure to null out each array element before we call the
	// actual destruction routine so that the array of avlie/registered modules contains no pointers to deleted objects
	for (uint32_t i = 0; i < modules.size(); i++)
	{
		ModuleIntl *d = imodules[i];
		imodules[i] = NULL;
		modules[i] = NULL;
		if ( d )
		{
			d->destroy();
		}
	}
	modules.clear();
	imodules.clear();

	/* Free all render meshes created from the SDK, release named resources */
	if (mAuthorableObjects != NULL)
	{
		PX_DELETE(mAuthorableObjects);
		mAuthorableObjects = NULL;
	}

	if (mDebugColorParams)
	{
		mDebugColorParams->destroy();
		mDebugColorParams = NULL;
	}

	frameworkModule.release(mParameterizedTraits);

	delete mParameterizedTraits;
	mParameterizedTraits = 0;

	apexResourceProvider->destroy();
	apexResourceProvider = 0;


	mPhysXObjDescs.clear();

	for (uint32_t i = 0; i < mTempMemories.size(); i++)
	{
		if (mTempMemories[i].memory != NULL)
		{
			PX_FREE(mTempMemories[i].memory);
			mTempMemories[i].memory = NULL;
			mTempMemories[i].size = 0;
		}
	}
	mTempMemories.clear();

	PX_DELETE(mCachedData);
	mCachedData = NULL;

	if (mErrorString)
	{
		PX_FREE_AND_RESET(mErrorString);
	}

#if PX_PHYSICS_VERSION_MAJOR == 0
	if (mApexThreadPool)
	{
		PX_DELETE(mApexThreadPool);
		mApexThreadPool = NULL;
	}
	while (mUserAllocThreadPools.size())
	{
		releaseCpuDispatcher(*mUserAllocThreadPools[0]);
	}
#endif
	
	delete this;
	// be very careful what goes below this line!

	gApexSdk = NULL;

}

RenderDebugInterface* ApexSDKImpl::createApexRenderDebug(RENDER_DEBUG::RenderDebugInterface* interface, bool useRemoteDebugVisualization)
{
	PX_UNUSED(useRemoteDebugVisualization);

#ifdef WITHOUT_DEBUG_VISUALIZE
	return NULL;

#else

	return nvidia::apex::createApexRenderDebug(this, interface, useRemoteDebugVisualization);
#endif
}

void ApexSDKImpl::releaseApexRenderDebug(RenderDebugInterface& debug)
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(debug);
#else
	debug.release();
#endif
}

SphereShape* ApexSDKImpl::createApexSphereShape()
{
	ApexSphereShape* m = PX_NEW(ApexSphereShape);
	return static_cast< SphereShape*>(m);
}

CapsuleShape* ApexSDKImpl::createApexCapsuleShape()
{
	ApexCapsuleShape* m = PX_NEW(ApexCapsuleShape);
	return static_cast< CapsuleShape*>(m);
}

BoxShape* ApexSDKImpl::createApexBoxShape()
{
	ApexBoxShape* m = PX_NEW(ApexBoxShape);
	return static_cast< BoxShape*>(m);
}

HalfSpaceShape* ApexSDKImpl::createApexHalfSpaceShape()
{
	ApexHalfSpaceShape* m = PX_NEW(ApexHalfSpaceShape);
	return static_cast< HalfSpaceShape*>(m);
}

void ApexSDKImpl::releaseApexShape(Shape& shape)
{
	shape.releaseApexShape();
}

const char* ApexSDKImpl::getWireframeMaterial()
{
	return mWireframeMaterial.c_str();
}

const char* ApexSDKImpl::getSolidShadedMaterial()
{
	return mSolidShadedMaterial.c_str();
}

pvdsdk::ApexPvdClient* ApexSDKImpl::getApexPvdClient()
{
#if defined(PHYSX_PROFILE_SDK)
	return mApexPvdClient;
#else
	return NULL;
#endif
}

profile::PxProfileZone * ApexSDKImpl::getProfileZone()
{
#if defined(PHYSX_PROFILE_SDK)
	return mProfileZone;
#else
	return NULL;
#endif
}

profile::PxProfileZoneManager * ApexSDKImpl::getProfileZoneManager()
{
#if defined(PHYSX_PROFILE_SDK)
	return mProfileZone ? mProfileZone->getProfileZoneManager() : NULL;
#else
	return NULL;
#endif
}

#if PX_WINDOWS_FAMILY
const char* ApexSDKImpl::getAppGuid()
{
	return mAppGuid.c_str();
}
#endif

#if APEX_CUDA_SUPPORT
PhysXGpuIndicator* ApexSDKImpl::registerPhysXIndicatorGpuClient()
{
	//allocate memory for the PhysXGpuIndicator
	PhysXGpuIndicator* gpuIndicator = static_cast<PhysXGpuIndicator*>(PX_ALLOC(sizeof(PhysXGpuIndicator), PX_DEBUG_EXP("PhysXGpuIndicator")));
	PX_PLACEMENT_NEW(gpuIndicator, PhysXGpuIndicator);
	
	gpuIndicator->gpuOn();
	return gpuIndicator;
}

void ApexSDKImpl::unregisterPhysXIndicatorGpuClient(PhysXGpuIndicator* gpuIndicator)
{
	if (gpuIndicator != NULL)
	{
		gpuIndicator->~PhysXGpuIndicator();
		PX_FREE(gpuIndicator);
	}
}
#endif

void ApexSDKImpl::updateDebugColorParams(const char* color, uint32_t val)
{
	for (uint32_t i = 0; i < mScenes.size(); i++)
	{
		DYNAMIC_CAST(ApexScene*)(mScenes[ i ])->updateDebugColorParams(color, val);
	}
}


bool ApexSDKImpl::getRMALoadMaterialsLazily()
{
	return mRMALoadMaterialsLazily;
}

///////////////////////////////////////////////////////////////////////////////
// ApexRenderMeshAssetAuthoring
///////////////////////////////////////////////////////////////////////////////


Asset* RenderMeshAuthorableObject::createAsset(AssetAuthoring& author, const char* name)
{
	NvParameterized::Interface* newObj = NULL;
	author.getNvParameterized()->clone(newObj);
	return createAsset(newObj, name);
}

Asset* RenderMeshAuthorableObject::createAsset(NvParameterized::Interface* params, const char* name)
{
	ApexRenderMeshAsset* asset = PX_NEW(ApexRenderMeshAsset)(mAssets, name, 0);
	if (asset)
	{
		asset->createFromParameters((RenderMeshAssetParameters*)params);
		GetInternalApexSDK()->getNamedResourceProvider()->setResource(mAOTypeName.c_str(), name, asset);
	}
	return asset;
}

void RenderMeshAuthorableObject::releaseAsset(Asset& nxasset)
{
	ApexRenderMeshAsset* aa = DYNAMIC_CAST(ApexRenderMeshAsset*)(&nxasset);
	GetInternalApexSDK()->getInternalResourceProvider()->setResource(ApexRenderMeshAsset::getClassName(), nxasset.getName(), NULL, false, false);
	aa->destroy();
}

// this should no longer be called now that we're auto-assigning names in createAssetAuthoring()
AssetAuthoring* RenderMeshAuthorableObject::createAssetAuthoring()
{
	return createAssetAuthoring("");
}

AssetAuthoring* 	RenderMeshAuthorableObject::createAssetAuthoring(NvParameterized::Interface* params, const char* name)
{
#ifdef WITHOUT_APEX_AUTHORING
	PX_UNUSED(params);
	PX_UNUSED(name);
	return NULL;
#else
	ApexRenderMeshAssetAuthoring* assetAuthor = PX_NEW(ApexRenderMeshAssetAuthoring)(mAssetAuthors, (RenderMeshAssetParameters*)params, name);
	if (assetAuthor)
	{
		GetInternalApexSDK()->getNamedResourceProvider()->setResource(mAOTypeName.c_str(), name, assetAuthor);
	}
	return assetAuthor;

#endif
}

AssetAuthoring* RenderMeshAuthorableObject::createAssetAuthoring(const char* name)
{
#ifdef WITHOUT_APEX_AUTHORING
	PX_UNUSED(name);
	return NULL;
#else
	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
	RenderMeshAssetParameters* params = (RenderMeshAssetParameters*)traits->createNvParameterized(RenderMeshAssetParameters::staticClassName());
	ApexRenderMeshAssetAuthoring* assetAuthor = PX_NEW(ApexRenderMeshAssetAuthoring)(mAssetAuthors, params, name);
	if (assetAuthor)
	{
		GetInternalApexSDK()->getNamedResourceProvider()->setResource(mAOTypeName.c_str(), name, assetAuthor);
	}
	return assetAuthor;

#endif
}

void RenderMeshAuthorableObject::releaseAssetAuthoring(AssetAuthoring& nxauthor)
{
#ifdef WITHOUT_APEX_AUTHORING
	PX_UNUSED(nxauthor);
#else
	ApexRenderMeshAssetAuthoring* aa = DYNAMIC_CAST(ApexRenderMeshAssetAuthoring*)(&nxauthor);
	GetInternalApexSDK()->getInternalResourceProvider()->setResource(mAOTypeName.c_str(), aa->getName(), NULL, false, false);

	aa->destroy();
#endif
}


uint32_t RenderMeshAuthorableObject::forceLoadAssets()
{
	uint32_t loadedAssetCount = 0;

	for (uint32_t i = 0; i < mAssets.getSize(); i++)
	{
		ApexRenderMeshAsset* asset = DYNAMIC_CAST(ApexRenderMeshAsset*)(mAssets.getResource(i));
		loadedAssetCount += asset->forceLoadAssets();
	}
	return loadedAssetCount;
}


// Resource methods
void RenderMeshAuthorableObject::release()
{
	// test this by releasing the module before the individual assets

	// remove all assets that we loaded (must do now else we cannot unregister)
	mAssets.clear();
	mAssetAuthors.clear();

	// remove this AO's name from the authorable namespace
	GetInternalApexSDK()->unregisterAuthObjType(mAOTypeName.c_str());
	destroy();
}

void RenderMeshAuthorableObject::destroy()
{
	delete this;
}


/**
Returns a PxFileBuf which reads from a buffer in memory.
*/
PxFileBuf* ApexSDKImpl::createMemoryReadStream(const void* mem, uint32_t len)
{
	PxFileBuf* ret = 0;

	nvidia::PsMemoryBuffer* rb = PX_NEW(nvidia::PsMemoryBuffer)((const uint8_t*)mem, len);
	ret = static_cast< PxFileBuf*>(rb);

	return ret;
}

/**
Returns a PxFileBuf which writes to memory.
*/
PxFileBuf* ApexSDKImpl::createMemoryWriteStream(uint32_t alignment)
{
	PxFileBuf* ret = 0;

	nvidia::PsMemoryBuffer* mb = PX_NEW(nvidia::PsMemoryBuffer)(nvidia::BUFFER_SIZE_DEFAULT, alignment);
	ret = static_cast< PxFileBuf*>(mb);

	return ret;
}

/**
Returns the address and length of the contents of a memory write buffer stream.
*/
const void* ApexSDKImpl::getMemoryWriteBuffer(PxFileBuf& stream, uint32_t& len)
{
	const void* ret = 0;

	nvidia::PsMemoryBuffer* wb = static_cast< nvidia::PsMemoryBuffer*>(&stream);
	len = wb->getWriteBufferSize();
	ret = wb->getWriteBuffer();

	return ret;
}

/**
Releases a previously created PxFileBuf used as a write or read buffer.
*/
void ApexSDKImpl::releaseMemoryReadStream(PxFileBuf& stream)
{
	nvidia::PsMemoryBuffer* rb = static_cast< nvidia::PsMemoryBuffer*>(&stream);
	delete rb;
}

void ApexSDKImpl::releaseMemoryWriteStream(PxFileBuf& stream)
{
	nvidia::PsMemoryBuffer* wb = static_cast< nvidia::PsMemoryBuffer*>(&stream);
	delete wb;
}


NvParameterized::Serializer* ApexSDKImpl::createSerializer(NvParameterized::Serializer::SerializeType type)
{
	return NvParameterized::internalCreateSerializer(type, mParameterizedTraits);
}

NvParameterized::Serializer* ApexSDKImpl::createSerializer(NvParameterized::Serializer::SerializeType type, NvParameterized::Traits* traits)
{
	return NvParameterized::internalCreateSerializer(type, traits);
}

NvParameterized::Serializer::SerializeType ApexSDKImpl::getSerializeType(const void* data, uint32_t dlen)
{
	PsMemoryBuffer stream(data, dlen);
	return NvParameterized::Serializer::peekSerializeType(stream);
}

NvParameterized::Serializer::SerializeType ApexSDKImpl::getSerializeType(PxFileBuf& stream)
{
	return NvParameterized::Serializer::peekSerializeType(stream);
}

NvParameterized::Serializer::ErrorType ApexSDKImpl::getSerializePlatform(const void* data, uint32_t dlen, NvParameterized::SerializePlatform& platform)
{
	if (dlen < 56)
	{
		APEX_INVALID_PARAMETER("At least 56 Bytes are needed to read the platform of a binary file");
	}

	PsMemoryBuffer stream(data, dlen);
	return NvParameterized::Serializer::peekPlatform(stream, platform);
}

NvParameterized::Serializer::ErrorType ApexSDKImpl::getSerializePlatform(PxFileBuf& stream, NvParameterized::SerializePlatform& platform)
{
	return NvParameterized::Serializer::peekPlatform(stream, platform);
}

void ApexSDKImpl::getCurrentPlatform(NvParameterized::SerializePlatform& platform) const
{
	platform = NvParameterized::GetCurrentPlatform();
}

bool ApexSDKImpl::getPlatformFromString(const char* name, NvParameterized::SerializePlatform& platform) const
{
	return NvParameterized::GetPlatform(name, platform);
}

const char* ApexSDKImpl::getPlatformName(const NvParameterized::SerializePlatform& platform) const
{
	return NvParameterized::GetPlatformName(platform);
}

Asset*	ApexSDKImpl::createAsset(const char* opaqueMeshName, UserOpaqueMesh* om)
{
	Asset* ret = NULL;
	NvParameterized::Interface* params = getParameterizedTraits()->createNvParameterized(RenderMeshAssetParameters::staticClassName());
	if (params)
	{
		ret = this->createAsset(params, opaqueMeshName);
		if (ret)
		{
			NvParameterized::setParamBool(*params, "isReferenced", false);
			ApexRenderMeshAsset* nrma = static_cast<ApexRenderMeshAsset*>(ret);
			nrma->setOpaqueMesh(om);
		}
		else
		{
			params->destroy();
		}
	}
	return ret;
}

ModuleIntl *ApexSDKImpl::getInternalModule(Module *module)
{
	ModuleIntl *ret = NULL;

	for (uint32_t i = 0; i < modules.size(); i++)
	{
		if ( modules[ i ] == module )
		{
			ret = imodules[i];
			break;
		}
	}

	return ret;
}

Module *ApexSDKImpl::getModule(ModuleIntl *module)
{
	Module *ret = NULL;

	for (uint32_t i = 0; i < imodules.size(); i++)
	{
		if ( imodules[ i ] == module )
		{
			ret = modules[i];
			break;
		}
	}

	return ret;
}


void ApexSDKImpl::enterURR()
{
	if (mURRdepthTLSslot != 0xFFFFFFFF)
	{
		uint64_t currentDepth = PTR_TO_UINT64(TlsGet(mURRdepthTLSslot));
		++currentDepth;
		TlsSet(mURRdepthTLSslot, (void*)currentDepth);
	}
}


void ApexSDKImpl::leaveURR()
{
	if (mURRdepthTLSslot != 0xFFFFFFFF)
	{
		uint64_t currentDepth = PTR_TO_UINT64(TlsGet(mURRdepthTLSslot));
		if (currentDepth > 0)
		{
			--currentDepth;
			TlsSet(mURRdepthTLSslot, (void*)currentDepth);
		}
		else
		{
			// if this is hit, something is wrong with the
			// URR_SCOPE implementation
			PX_ALWAYS_ASSERT();
		}
	}
}


void ApexSDKImpl::checkURR()
{
	if (mURRdepthTLSslot != 0xFFFFFFFF)
	{
		uint64_t currentDepth = PTR_TO_UINT64(TlsGet(mURRdepthTLSslot));
		if (currentDepth == 0)
		{
			// if this assert is hit it means that
			// - either some render resources are created where it's not allowed
			//   (outside of updateRenderResources or prepareRenderResources)
			//   => change the place in code where the resource is created
			//
			// - or the updateRenderResources call is not marked with a URR_SCOPE.
			//   => add the macro
			PX_ALWAYS_ASSERT();
		}
	}
}

}
} // end namespace nvidia::apex
