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
#include "ModuleDestructibleImpl.h"
#include "ModuleDestructibleRegistration.h"
#include "DestructibleAssetProxy.h"
#include "DestructibleActorProxy.h"
#include "DestructibleActorJointProxy.h"
#include "DestructibleScene.h"
#include "ApexSharedUtils.h"
#include "PsMemoryBuffer.h"

#include "PxPhysics.h"
#include "PxScene.h"

#include "ApexStream.h"
#include "ModulePerfScope.h"
using namespace destructible;

#include "ApexSDKIntl.h"
#include "ApexUsingNamespace.h"
#include "WriteCheck.h"

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
	destructible::ModuleDestructibleImpl* impl = PX_NEW(destructible::ModuleDestructibleImpl)(inSdk);
	*niRef  = (ModuleIntl*) impl;
	return (Module*) impl;
}
#else
/* Statically linking entry function */
void instantiateModuleDestructible()
{
	ApexSDKIntl* sdk = GetInternalApexSDK();
	destructible::ModuleDestructibleImpl* impl = PX_NEW(destructible::ModuleDestructibleImpl)(sdk);
	sdk->registerExternalModule((Module*) impl, (ModuleIntl*) impl);
}
#endif
}

namespace destructible
{

AuthObjTypeID DestructibleAssetImpl::mAssetTypeID;  // Static class member
#ifdef WITHOUT_APEX_AUTHORING

class DestructibleAssetDummyAuthoring : public AssetAuthoring, public UserAllocated
{
public:
	DestructibleAssetDummyAuthoring(ModuleDestructibleImpl* module, ResourceList& list, NvParameterized::Interface* params, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(params);
		PX_UNUSED(name);
	}

	DestructibleAssetDummyAuthoring(ModuleDestructibleImpl* module, ResourceList& list, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(name);
	}

	DestructibleAssetDummyAuthoring(ModuleDestructibleImpl* module, ResourceList& list)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
	}

	virtual ~DestructibleAssetDummyAuthoring() {}

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


#if 0
	/**
	* \brief Save asset configuration to a stream
	*/
	virtual PxFileBuf& serialize(PxFileBuf& stream) const
	{
		PX_ASSERT(0);
		return stream;
	}

	/**
	* \brief Load asset configuration from a stream
	*/
	virtual PxFileBuf& deserialize(PxFileBuf& stream)
	{
		PX_ASSERT(0);
		return stream;
	}
#endif

	const char* getName(void) const
	{
		return NULL;
	}

	/**
	* \brief Returns the name of this APEX authorable object type
	*/
	virtual const char* getObjTypeName() const
	{
		return DESTRUCTIBLE_AUTHORING_TYPE_NAME;
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

typedef ApexAuthorableObject<ModuleDestructibleImpl, DestructibleAssetProxy, DestructibleAssetDummyAuthoring> DestructionAO;


#else
typedef ApexAuthorableObject<ModuleDestructibleImpl, DestructibleAssetProxy, DestructibleAssetAuthoringProxy> DestructionAO;
#endif


ModuleDestructibleImpl::ModuleDestructibleImpl(ApexSDKIntl* inSdk) :
	m_isInitialized(false),
	m_maxChunkDepthOffset(0),
	m_maxChunkSeparationLOD(0.5f),
	m_maxFracturesProcessedPerFrame(UINT32_MAX),
	m_maxActorsCreateablePerFrame(UINT32_MAX),
	m_dynamicActorFIFOMax(0),
	m_chunkFIFOMax(0),
	m_sortByBenefit(false),
	m_chunkReport(NULL),
	m_impactDamageReport(NULL),
	m_chunkReportBitMask(0xffffffff),
	m_destructiblePhysXActorReport(NULL),
	m_chunkReportMaxFractureEventDepth(0xffffffff),
	m_chunkStateEventCallbackSchedule(DestructibleCallbackSchedule::Disabled),
	m_chunkCrumbleReport(NULL),
	m_chunkDustReport(NULL),
	m_massScale(1.0f),
	m_scaledMassExponent(0.5f),
	mApexDestructiblePreviewParams(NULL),
	mUseLegacyChunkBoundsTesting(false),
	mUseLegacyDamageRadiusSpread(false),
	mChunkCollisionHullCookingScale(1.0f),
	mFractureTools(NULL)
{
	mName = "Destructible";
	mSdk = inSdk;
	mApiProxy = this;
	mModuleParams = NULL;

	NvParameterized::Traits* traits = mSdk->getParameterizedTraits();
	if (traits)
	{
		ModuleDestructibleRegistration::invokeRegistration(traits);
		mApexDestructiblePreviewParams = traits->createNvParameterized(DestructiblePreviewParam::staticClassName());
	}

	/* Register this module's authorable object types and create their namespaces */
	const char* pName = DestructibleAssetParameters::staticClassName();
	DestructionAO* eAO = PX_NEW(DestructionAO)(this, mAuthorableObjects, pName);
	DestructibleAssetImpl::mAssetTypeID = eAO->getResID();

	mCachedData = PX_NEW(DestructibleModuleCachedData)(getModuleID());

#ifndef WITHOUT_APEX_AUTHORING
	mFractureTools = PX_NEW(FractureTools)();
#endif
}

AuthObjTypeID ModuleDestructibleImpl::getModuleID() const
{
	READ_ZONE();
	return DestructibleAssetImpl::mAssetTypeID;
}

ModuleDestructibleImpl::~ModuleDestructibleImpl()
{
	m_destructibleSceneList.clear();

	PX_DELETE(mCachedData);
	mCachedData = NULL;
}

NvParameterized::Interface* ModuleDestructibleImpl::getDefaultModuleDesc()
{
	READ_ZONE();
	NvParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (!mModuleParams)
	{
		mModuleParams = DYNAMIC_CAST(DestructibleModuleParameters*)
		                (traits->createNvParameterized("DestructibleModuleParameters"));
		PX_ASSERT(mModuleParams);
	}
	else
	{
		mModuleParams->initDefaults();
	}

	return mModuleParams;
}

void ModuleDestructibleImpl::init(NvParameterized::Interface& desc)
{
	WRITE_ZONE();
	if (nvidia::strcmp(desc.className(), DestructibleModuleParameters::staticClassName()) == 0)
	{
		DestructibleModuleParameters* params = DYNAMIC_CAST(DestructibleModuleParameters*)(&desc);
		setValidBoundsPadding(params->validBoundsPadding);
		setMaxDynamicChunkIslandCount(params->maxDynamicChunkIslandCount);
		setSortByBenefit(params->sortFIFOByBenefit);
		setMaxChunkSeparationLOD(params->maxChunkSeparationLOD);
		setMaxActorCreatesPerFrame(params->maxActorCreatesPerFrame);
		setMaxChunkDepthOffset(params->maxChunkDepthOffset);

		if (params->massScale > 0.0f)
		{
			m_massScale = params->massScale;
		}

		if (params->scaledMassExponent > 0.0f && params->scaledMassExponent <= 1.0f)
		{
			m_scaledMassExponent = params->scaledMassExponent;
		}

		m_isInitialized = true;
		for (uint32_t i = 0; i < m_destructibleSceneList.getSize(); ++i)
		{
			// when module is created after the scene
			(DYNAMIC_CAST(DestructibleScene*)(m_destructibleSceneList.getResource(i)))->initModuleSettings();
		}
	}
	else
	{
		APEX_INVALID_PARAMETER("The NvParameterized::Interface object is the wrong type");
	}
}

ModuleSceneIntl* ModuleDestructibleImpl::createInternalModuleScene(SceneIntl& scene, RenderDebugInterface* debugRender)
{
	return PX_NEW(DestructibleScene)(*this, scene, debugRender, m_destructibleSceneList);
}

void ModuleDestructibleImpl::releaseModuleSceneIntl(ModuleSceneIntl& scene)
{
	DestructibleScene* ds = DYNAMIC_CAST(DestructibleScene*)(&scene);
	ds->destroy();
}

uint32_t ModuleDestructibleImpl::forceLoadAssets()
{
	uint32_t loadedAssetCount = 0;

	for (uint32_t i = 0; i < mAuthorableObjects.getSize(); i++)
	{
		AuthorableObjectIntl* ao = static_cast<AuthorableObjectIntl*>(mAuthorableObjects.getResource(i));
		loadedAssetCount += ao->forceLoadAssets();
	}
	return loadedAssetCount;
}

DestructibleScene* ModuleDestructibleImpl::getDestructibleScene(const Scene& apexScene) const
{
	for (uint32_t i = 0 ; i < m_destructibleSceneList.getSize() ; i++)
	{
		DestructibleScene* ds = DYNAMIC_CAST(DestructibleScene*)(m_destructibleSceneList.getResource(i));
		if (ds->mApexScene == &apexScene)
		{
			return ds;
		}
	}

	PX_ASSERT(!"Unable to locate an appropriate DestructibleScene");
	return NULL;
}

RenderableIterator* ModuleDestructibleImpl::createRenderableIterator(const Scene& apexScene)
{
	WRITE_ZONE();
	DestructibleScene* ds = getDestructibleScene(apexScene);
	if (ds)
	{
		return ds->createRenderableIterator();
	}

	return NULL;
}

DestructibleActorJoint* ModuleDestructibleImpl::createDestructibleActorJoint(const DestructibleActorJointDesc& destructibleActorJointDesc, Scene& scene)
{
	WRITE_ZONE();
	if (!destructibleActorJointDesc.isValid())
	{
		return NULL;
	}
	DestructibleScene* 	ds = getDestructibleScene(scene);
	if (ds)
	{
		return ds->createDestructibleActorJoint(destructibleActorJointDesc);
	}
	else
	{
		return NULL;
	}
}

bool ModuleDestructibleImpl::isDestructibleActorJointActive(const DestructibleActorJoint* candidateJoint, Scene& apexScene) const
{
	READ_ZONE();
	PX_ASSERT(candidateJoint != NULL);
	PX_ASSERT(&apexScene != NULL);
	DestructibleScene* destructibleScene = NULL;
	destructibleScene = getDestructibleScene(apexScene);
	PX_ASSERT(destructibleScene != NULL);
	bool found = false;
	if (destructibleScene != NULL)
	{
		for (uint32_t index = 0; index < destructibleScene->mDestructibleActorJointList.getSize(); ++index)
		{
			DestructibleActorJoint* activeJoint = NULL;
			activeJoint = static_cast<DestructibleActorJoint*>(static_cast<DestructibleActorJointProxy*>((destructibleScene->mDestructibleActorJointList.getResource(index))));
			PX_ASSERT(activeJoint != NULL);
			if (activeJoint == candidateJoint)
			{
				found = true;
				break;
			}
		}
	}
	return found;
}

void ModuleDestructibleImpl::setMaxDynamicChunkIslandCount(uint32_t maxCount)
{
	WRITE_ZONE();
	m_dynamicActorFIFOMax = maxCount;
}

void ModuleDestructibleImpl::setMaxChunkCount(uint32_t maxCount)
{
	WRITE_ZONE();
	m_chunkFIFOMax = maxCount;
}

void ModuleDestructibleImpl::setSortByBenefit(bool sortByBenefit)
{
	WRITE_ZONE();
	m_sortByBenefit = sortByBenefit;
}

void ModuleDestructibleImpl::setMaxChunkDepthOffset(uint32_t offset)
{
	WRITE_ZONE();
	m_maxChunkDepthOffset = offset;
}

void ModuleDestructibleImpl::setMaxChunkSeparationLOD(float separationLOD)
{
	WRITE_ZONE();
	m_maxChunkSeparationLOD = PxClamp(separationLOD, 0.0f, 1.0f);
}

void ModuleDestructibleImpl::setValidBoundsPadding(float pad)
{
	WRITE_ZONE();
	m_validBoundsPadding = pad;
}

void ModuleDestructibleImpl::setChunkReport(UserChunkReport* chunkReport)
{
	WRITE_ZONE();
	m_chunkReport = chunkReport;
}

void ModuleDestructibleImpl::setImpactDamageReportCallback(UserImpactDamageReport* impactDamageReport)
{
	WRITE_ZONE();
	m_impactDamageReport = impactDamageReport;
}

void ModuleDestructibleImpl::setChunkReportBitMask(uint32_t chunkReportBitMask)
{
	WRITE_ZONE();
	m_chunkReportBitMask = chunkReportBitMask;
}

void ModuleDestructibleImpl::setDestructiblePhysXActorReport(UserDestructiblePhysXActorReport* destructiblePhysXActorReport)
{
	WRITE_ZONE();
	m_destructiblePhysXActorReport = destructiblePhysXActorReport;
}

void ModuleDestructibleImpl::setChunkReportMaxFractureEventDepth(uint32_t chunkReportMaxFractureEventDepth)
{
	WRITE_ZONE();
	m_chunkReportMaxFractureEventDepth = chunkReportMaxFractureEventDepth;
}

void ModuleDestructibleImpl::scheduleChunkStateEventCallback(DestructibleCallbackSchedule::Enum chunkStateEventCallbackSchedule)
{
	WRITE_ZONE();
	if (chunkStateEventCallbackSchedule >= (DestructibleCallbackSchedule::Enum)0 && chunkStateEventCallbackSchedule < DestructibleCallbackSchedule::Count)
	{
		m_chunkStateEventCallbackSchedule = chunkStateEventCallbackSchedule;
	}
}

void ModuleDestructibleImpl::setChunkCrumbleReport(UserChunkParticleReport* chunkCrumbleReport)
{
	WRITE_ZONE();
	m_chunkCrumbleReport = chunkCrumbleReport;
}

void ModuleDestructibleImpl::setChunkDustReport(UserChunkParticleReport* chunkDustReport)
{
	WRITE_ZONE();
	m_chunkDustReport = chunkDustReport;
}

void ModuleDestructibleImpl::setWorldSupportPhysXScene(Scene& apexScene, PxScene* physxScene)
{
	WRITE_ZONE();
	DestructibleScene* ds = getDestructibleScene(apexScene);
	if (ds)
	{
		ds->setWorldSupportPhysXScene(physxScene);
	}
}

bool ModuleDestructibleImpl::owns(const PxRigidActor* actor) const
{
	READ_ZONE();
	const PhysXObjectDescIntl* desc = static_cast<const PhysXObjectDescIntl*>(mSdk->getPhysXObjectInfo(actor));
	if (desc != NULL)
	{
		const uint32_t actorCount = desc->mApexActors.size();
		for (uint32_t i = 0; i < actorCount; ++i)
		{
			const Actor* actor = desc->mApexActors[i];
			if (actor != NULL && actor->getOwner()->getObjTypeID() == DestructibleAssetImpl::mAssetTypeID)
			{
				return true;
			}
		}
	}

	return false;
}

#if APEX_RUNTIME_FRACTURE
bool ModuleDestructibleImpl::isRuntimeFractureShape(const PxShape& shape) const
{
	READ_ZONE();
	for (uint32_t i = 0 ; i < m_destructibleSceneList.getSize() ; i++)
	{
		DestructibleScene* ds = DYNAMIC_CAST(DestructibleScene*)(m_destructibleSceneList.getResource(i));
		nvidia::fracture::SimScene* simScene = ds->getDestructibleRTScene(false);
		if (simScene && simScene->owns(shape))
		{
			return true;
		}
	}
	return false;
}
#endif

DestructibleActor* ModuleDestructibleImpl::getDestructibleAndChunk(const PxShape* shape, int32_t* chunkIndex) const
{
	READ_ZONE();
	const PhysXObjectDescIntl* desc = static_cast<const PhysXObjectDescIntl*>(mSdk->getPhysXObjectInfo(shape));
	if (desc != NULL)
	{
		const uint32_t actorCount = desc->mApexActors.size();
		PX_ASSERT(actorCount == 1);	// Shapes should only be associated with one chunk
		if (actorCount > 0)
		{
			const DestructibleActorProxy* actorProxy = (DestructibleActorProxy*)desc->mApexActors[0];
			if (actorProxy->getOwner()->getObjTypeID() == DestructibleAssetImpl::mAssetTypeID)
			{
				const DestructibleScene* ds = actorProxy->impl.getDestructibleScene();
				DestructibleStructure::Chunk* chunk = (DestructibleStructure::Chunk*)desc->userData;
				if (chunk == NULL || chunk->destructibleID == DestructibleStructure::InvalidID || chunk->destructibleID > ds->mDestructibles.capacity())
				{
					return NULL;
				}

				DestructibleActorImpl* destructible = ds->mDestructibles.direct(chunk->destructibleID);
				if (destructible == NULL)
				{
					return NULL;
				}

				if (chunkIndex)
				{
					*chunkIndex = (int32_t)chunk->indexInAsset;
				}
				return destructible->getAPI();
			}
		}
	}

	return NULL;
}

void ModuleDestructibleImpl::applyRadiusDamage(Scene& scene, float damage, float momentum, const PxVec3& position, float radius, bool falloff)
{
	WRITE_ZONE();
	DestructibleScene* ds = getDestructibleScene(scene);
	if (ds)
	{
		ds->applyRadiusDamage(damage, momentum, position, radius, falloff);
	}
}

void ModuleDestructibleImpl::setMaxActorCreatesPerFrame(uint32_t maxActorsPerFrame)
{
	WRITE_ZONE();
	m_maxActorsCreateablePerFrame = maxActorsPerFrame;
}

void ModuleDestructibleImpl::setMaxFracturesProcessedPerFrame(uint32_t maxFracturesProcessedPerFrame)
{
	WRITE_ZONE();
	m_maxFracturesProcessedPerFrame = maxFracturesProcessedPerFrame;
}

#if 0 // dead code
void ModuleDestructible::releaseBufferedConvexMeshes()
{
	for (uint32_t i = 0; i < convexMeshKillList.size(); i++)
	{
		convexMeshKillList[i]->release();
	}
	convexMeshKillList.clear();
}
#endif

void ModuleDestructibleImpl::destroy()
{
#ifndef WITHOUT_APEX_AUTHORING
	if (mFractureTools != NULL)
	{
		PX_DELETE(mFractureTools);
		mFractureTools = NULL;
	}
#endif

	NvParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (mModuleParams != NULL)
	{
		mModuleParams->destroy();
		mModuleParams = NULL;
	}

	if (mApexDestructiblePreviewParams != NULL)
	{
		mApexDestructiblePreviewParams->destroy();
		mApexDestructiblePreviewParams = NULL;
	}

	// base class
	ModuleBase::destroy();

#if 0 //dead code
	releaseBufferedConvexMeshes();
#endif

	if (traits)
	{
		ModuleDestructibleRegistration::invokeUnregistration(traits);
	}

	delete this;
}

/*** ModuleDestructibleImpl::SyncParams ***/
bool ModuleDestructibleImpl::setSyncParams(UserDamageEventHandler * userDamageEventHandler, UserFractureEventHandler * userFractureEventHandler, UserChunkMotionHandler * userChunkMotionHandler)
{
	WRITE_ZONE();
	bool validEntry = false;
	validEntry = ((NULL != userChunkMotionHandler) ? (NULL != userDamageEventHandler || NULL != userFractureEventHandler) : true);
	if(validEntry)
	{
		mSyncParams.userDamageEventHandler		= userDamageEventHandler;
		mSyncParams.userFractureEventHandler	= userFractureEventHandler;
		mSyncParams.userChunkMotionHandler		= userChunkMotionHandler;
	}
	return validEntry;
}

typedef ModuleDestructibleImpl::SyncParams SyncParams;

SyncParams::SyncParams()
:userDamageEventHandler(NULL)
,userFractureEventHandler(NULL)
,userChunkMotionHandler(NULL)
{
}

SyncParams::~SyncParams()
{
    userChunkMotionHandler = NULL;
	userFractureEventHandler = NULL;
	userDamageEventHandler = NULL;
}

UserDamageEventHandler * SyncParams::getUserDamageEventHandler() const
{
	return userDamageEventHandler;
}

UserFractureEventHandler * SyncParams::getUserFractureEventHandler() const
{
	return userFractureEventHandler;
}

UserChunkMotionHandler * SyncParams::getUserChunkMotionHandler() const
{
	return userChunkMotionHandler;
}

template<typename T> uint32_t SyncParams::getSize() const
{
	return sizeof(T);
}

template uint32_t ModuleDestructibleImpl::SyncParams::getSize<DamageEventHeader>		() const;
template uint32_t ModuleDestructibleImpl::SyncParams::getSize<DamageEventUnit>		() const;
template uint32_t ModuleDestructibleImpl::SyncParams::getSize<FractureEventHeader>	() const;
template uint32_t ModuleDestructibleImpl::SyncParams::getSize<FractureEventUnit>		() const;
template uint32_t ModuleDestructibleImpl::SyncParams::getSize<ChunkTransformHeader>	() const;
template uint32_t ModuleDestructibleImpl::SyncParams::getSize<ChunkTransformUnit>		() const;

const SyncParams & ModuleDestructibleImpl::getSyncParams() const
{
    return mSyncParams;
}

void ModuleDestructibleImpl::setUseLegacyChunkBoundsTesting(bool useLegacyChunkBoundsTesting)
{
	WRITE_ZONE();
	mUseLegacyChunkBoundsTesting = useLegacyChunkBoundsTesting;
}

void ModuleDestructibleImpl::setUseLegacyDamageRadiusSpread(bool useLegacyDamageRadiusSpread)
{
	WRITE_ZONE();
	mUseLegacyDamageRadiusSpread = useLegacyDamageRadiusSpread;
}

bool ModuleDestructibleImpl::setMassScaling(float massScale, float scaledMassExponent, Scene& apexScene)
{
	WRITE_ZONE();
	DestructibleScene* dscene = getDestructibleScene(apexScene);

	if (dscene != NULL)
	{
		return dscene->setMassScaling(massScale, scaledMassExponent);
	}

	return false;
}

void ModuleDestructibleImpl::invalidateBounds(const PxBounds3* bounds, uint32_t boundsCount, Scene& apexScene)
{
	WRITE_ZONE();
	DestructibleScene* dscene = getDestructibleScene(apexScene);

	if (dscene != NULL)
	{
		dscene->invalidateBounds(bounds, boundsCount);
	}
}

void ModuleDestructibleImpl::setDamageApplicationRaycastFlags(nvidia::DestructibleActorRaycastFlags::Enum flags, Scene& apexScene)
{
	WRITE_ZONE();
	DestructibleScene* dscene = getDestructibleScene(apexScene);

	if (dscene != NULL)
	{
		dscene->setDamageApplicationRaycastFlags(flags);
	}
}

bool ModuleDestructibleImpl::setRenderLockMode(RenderLockMode::Enum renderLockMode, Scene& apexScene)
{
	WRITE_ZONE();
	DestructibleScene* dscene = getDestructibleScene(apexScene);

	if (dscene != NULL)
	{
		return dscene->setRenderLockMode(renderLockMode);
	}

	return false;
}

RenderLockMode::Enum ModuleDestructibleImpl::getRenderLockMode(const Scene& apexScene) const
{
	READ_ZONE();
	DestructibleScene* dscene = getDestructibleScene(apexScene);

	if (dscene != NULL)
	{
		return dscene->getRenderLockMode();
	}

	return RenderLockMode::NO_RENDER_LOCK;
}

bool ModuleDestructibleImpl::lockModuleSceneRenderLock(Scene& apexScene)
{	
	DestructibleScene* dscene = getDestructibleScene(apexScene);

	if (dscene != NULL)
	{
		return dscene->lockModuleSceneRenderLock();
	}

	return false;
}

bool ModuleDestructibleImpl::unlockModuleSceneRenderLock(Scene& apexScene)
{
	DestructibleScene* dscene = getDestructibleScene(apexScene);

	if (dscene != NULL)
	{
		return dscene->unlockModuleSceneRenderLock();
	}

	return false;
}

bool ModuleDestructibleImpl::setChunkCollisionHullCookingScale(const PxVec3& scale)
{
	WRITE_ZONE();
	if (scale.x <= 0.0f || scale.y <= 0.0f || scale.z <= 0.0f)
	{
		return false;
	}

	mChunkCollisionHullCookingScale = scale;

	return true;
}


/*******************************
* DestructibleModuleCachedData *
*******************************/

DestructibleModuleCachedData::DestructibleModuleCachedData(AuthObjTypeID moduleID) :
	mModuleID(moduleID)
{
}

DestructibleModuleCachedData::~DestructibleModuleCachedData()
{
	clear();
}

NvParameterized::Interface* DestructibleModuleCachedData::getCachedDataForAssetAtScale(Asset& asset, const PxVec3& scale)
{
	DestructibleAssetImpl& dasset = DYNAMIC_CAST(DestructibleAssetProxy*)(&asset)->impl;

	DestructibleAssetCollision* collisionSet = findAssetCollisionSet(asset.getName());
	if (collisionSet == NULL)
	{
		collisionSet = PX_NEW(DestructibleAssetCollision);
		collisionSet->setDestructibleAssetToCook(&dasset);
		mAssetCollisionSets.pushBack(collisionSet);
	}

	return collisionSet->getCollisionAtScale(scale);
}

PxFileBuf& DestructibleModuleCachedData::serialize(PxFileBuf& stream) const
{
	stream << (uint32_t)Version::Current;

	stream << (uint32_t)mModuleID;

	stream << mAssetCollisionSets.size();
	for (uint32_t i = 0; i < mAssetCollisionSets.size(); ++i)
	{
		mAssetCollisionSets[i]->serialize(stream);
	}

	return stream;
}

PxFileBuf& DestructibleModuleCachedData::deserialize(PxFileBuf& stream)
{
	clear(false);	// false => don't delete cached data for referenced sets

	/*const uint32_t version =*/
	stream.readDword();	// Original version

	mModuleID = stream.readDword();

	const uint32_t dataSetCount = stream.readDword();
	for (uint32_t i = 0; i < dataSetCount; ++i)
	{
		DestructibleAssetCollision* collisionSet = PX_NEW(DestructibleAssetCollision);
		collisionSet->deserialize(stream, NULL);
		// See if we already have a set for the asset
		DestructibleAssetCollision* destinationSet = NULL;
		for (uint32_t j = 0; j < mAssetCollisionSets.size(); ++j)
		{
			if (!nvidia::stricmp(mAssetCollisionSets[j]->getAssetName(), collisionSet->getAssetName()))
			{
				destinationSet = mAssetCollisionSets[j];
				break;
			}
		}
		if (destinationSet == NULL)
		{
			// We don't have a set for this asset.  Simply add it.
			destinationSet = mAssetCollisionSets.pushBack(collisionSet);
		}
		else
		{
			// We already have a set for this asset.  Merge.
			destinationSet->merge(*collisionSet);
			PX_DELETE(collisionSet);
		}
		destinationSet->cookAll();	// This should only cook what isn't already cooked
	}

	return stream;
}

PxFileBuf& DestructibleModuleCachedData::serializeSingleAsset(Asset& asset, PxFileBuf& stream)
{
	DestructibleAssetCollision* collisionSet = findAssetCollisionSet(asset.getName());
	if( collisionSet )
	{
		collisionSet->serialize(stream);
	}

	return stream;
}

PxFileBuf& DestructibleModuleCachedData::deserializeSingleAsset(Asset& asset, PxFileBuf& stream)
{
	DestructibleAssetCollision* collisionSet = findAssetCollisionSet(asset.getName());
	if (collisionSet == NULL)
	{
		collisionSet = PX_NEW(DestructibleAssetCollision);
		mAssetCollisionSets.pushBack(collisionSet);
	}
	collisionSet->deserialize(stream, asset.getName());

	return stream;
}

void DestructibleModuleCachedData::clear(bool force)
{
	if (force)
	{
		// force == true, so delete everything
		for (uint32_t i = mAssetCollisionSets.size(); i--;)
		{
			DestructibleAssetCollision* collisionSet = mAssetCollisionSets[i];
			if (collisionSet != NULL)
			{
				// Spit out warnings to the error stream for any referenced sets
				collisionSet->reportReferencedSets();
				collisionSet->clearUnreferencedSets();
			}
			PX_DELETE(mAssetCollisionSets[i]);
		}
		mAssetCollisionSets.reset();
		
		return;
	}

	// If !force, then we have more work to do:

	for (uint32_t i = mAssetCollisionSets.size(); i--;)
	{
		DestructibleAssetCollision* collisionSet = mAssetCollisionSets[i];
		if (collisionSet != NULL)
		{
			collisionSet->clearUnreferencedSets();
		}
	}
}

void DestructibleModuleCachedData::clearAssetCollisionSet(const DestructibleAssetImpl& asset)
{
	for (uint32_t i = mAssetCollisionSets.size(); i--;)
	{
		if (!mAssetCollisionSets[i] || !mAssetCollisionSets[i]->getAssetName() || !nvidia::stricmp(mAssetCollisionSets[i]->getAssetName(), asset.getName()))
		{
			PX_DELETE(mAssetCollisionSets[i]);
			mAssetCollisionSets.replaceWithLast(i);
		}
	}
}

physx::Array<PxConvexMesh*>* DestructibleModuleCachedData::getConvexMeshesForActor(const DestructibleActorImpl& destructible)
{
	const DestructibleAssetImpl* asset = destructible.getDestructibleAsset();
	if (asset == NULL)
	{
		return NULL;
	}

	DestructibleAssetCollision* collisionSet = getAssetCollisionSet(*asset);
	if (collisionSet == NULL)
	{
		return NULL;
	}

	physx::Array<PxConvexMesh*>* convexMeshes = collisionSet->getConvexMeshesAtScale(destructible.getScale());

	const int scaleIndex = collisionSet->getScaleIndex(destructible.getScale(), kDefaultDestructibleAssetCollisionScaleTolerance);
	collisionSet->incReferenceCount(scaleIndex);

	return convexMeshes;
}

void DestructibleModuleCachedData::releaseReferencesToConvexMeshesForActor(const DestructibleActorImpl& destructible)
{
	const DestructibleAssetImpl* asset = destructible.getDestructibleAsset();
	if (asset == NULL)
	{
		return;
	}

	DestructibleAssetCollision* collisionSet = getAssetCollisionSet(*asset);
	if (collisionSet == NULL)
	{
		return;
	}

	const int scaleIndex = collisionSet->getScaleIndex(destructible.getScale(), kDefaultDestructibleAssetCollisionScaleTolerance);
	collisionSet->decReferenceCount(scaleIndex);
}

physx::Array<PxConvexMesh*>* DestructibleModuleCachedData::getConvexMeshesForScale(const DestructibleAssetImpl& asset, const PxVec3& scale, bool incRef)
{
	DestructibleAssetCollision* collisionSet = getAssetCollisionSet(asset);
	if (collisionSet == NULL)
	{
		return NULL;
	}

	physx::Array<PxConvexMesh*>* convexMeshes =  collisionSet->getConvexMeshesAtScale(scale);

	// The DestructibleActor::cacheModuleData() method needs to avoid incrementing the ref count
	if (incRef)
	{
		const int scaleIndex = collisionSet->getScaleIndex(scale, kDefaultDestructibleAssetCollisionScaleTolerance);
		collisionSet->incReferenceCount(scaleIndex);
	}

	return convexMeshes;
}

DestructibleAssetCollision* DestructibleModuleCachedData::getAssetCollisionSet(const DestructibleAssetImpl& asset)
{
	DestructibleAssetCollision* collisionSet = findAssetCollisionSet(asset.getName());

	if (collisionSet == NULL)
	{
		collisionSet = PX_NEW(DestructibleAssetCollision);
		mAssetCollisionSets.pushBack(collisionSet);
	}
	collisionSet->setDestructibleAssetToCook(const_cast<DestructibleAssetImpl*>(&asset));

	return collisionSet;
}

DestructibleAssetCollision* DestructibleModuleCachedData::findAssetCollisionSet(const char* name)
{
	for (uint32_t i = 0; i < mAssetCollisionSets.size(); ++i)
	{
		if (mAssetCollisionSets[i] && mAssetCollisionSets[i]->getAssetName() && !nvidia::stricmp(mAssetCollisionSets[i]->getAssetName(), name))
		{
			return mAssetCollisionSets[i];
		}
	}

	return NULL;
}

}
} // end namespace nvidia
