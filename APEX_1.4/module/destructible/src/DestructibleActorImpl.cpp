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
#include "Apex.h"
#include "ScopedPhysXLock.h"
#include "ModuleDestructibleImpl.h"
#include "DestructibleActorImpl.h"
#include "DestructibleScene.h"
#include "SceneIntl.h"
#include "DestructibleStructure.h"
#include "DestructibleActorProxy.h"
#include "ModulePerfScope.h"

#if APEX_USE_PARTICLES
#include "EmitterAsset.h"
#include "EmitterActor.h"
#endif // APEX_USE_PARTICLES

#include "PxMath.h"
#include <PxMaterial.h>

#include "PsString.h"

#include "RenderMeshAssetIntl.h"

#include "DestructibleActorUtils.h"

namespace nvidia
{
namespace destructible
{
using namespace physx;

///////////////////////////////////////////////////////////////////////////
namespace
{
bool isPxActorSleeping(const physx::PxRigidDynamic & actor)
{
	PX_ASSERT(NULL != &actor);
	SCOPED_PHYSX_LOCK_READ(actor.getScene());
	return actor.isSleeping();
}
} // namespace nameless

template<class type>
PX_INLINE void combsort(type* a, uint32_t num)
{
	uint32_t gap = num;
	bool swapped = false;
	do
	{
		swapped = false;
		gap = (gap * 10) / 13;
		if (gap == 9 || gap == 10)
		{
			gap = 11;
		}
		else if (gap < 1)
		{
			gap = 1;
		}
		for (type* ai = a, *aend = a + (num - gap); ai < aend; ai++)
		{
			type* aj = ai + gap;
			if (*ai > *aj)
			{
				swapped = true;
				nvidia::swap(*ai, *aj);
			}
		}
	}
	while (gap > 1 || swapped);
}

static NvParameterized::Interface* createDefaultState(NvParameterized::Interface* params = NULL)
{
	NvParameterized::Interface* state = GetInternalApexSDK()->getParameterizedTraits()->createNvParameterized(DestructibleActorState::staticClassName());
	PX_ASSERT(NULL != state);
	if(state)
	{
		NvParameterized::Handle handle(*state);
		VERIFY_PARAM(state->getParameterHandle("actorParameters", handle));
		if (params)
			handle.setParamRef(params);
		else
			handle.initParamRef(DestructibleActorParam::staticClassName());

		VERIFY_PARAM(state->getParameterHandle("actorChunks", handle));
		handle.initParamRef(DestructibleActorChunks::staticClassName());
	}
	return state;
}

DestructibleActorImpl::DestructibleActorImpl(DestructibleActor* _api, DestructibleAssetImpl& _asset, DestructibleScene& scene ) :
	mState(NULL),
	mParams(NULL),
	mChunks(NULL),
	mTM(PxMat44(PxIdentity)),
	mRelTM(PxMat44(PxIdentity)),
	mVisibleDynamicChunkShapeCount(0),
	mEssentialVisibleDynamicChunkShapeCount(0),
	mDestructibleScene(&scene),
	mAPI(_api),
	mFlags(0),
	mInternalFlags(0),
	mStructure(NULL),
	mAsset(&_asset),
	mID((uint32_t)InvalidID),
	mLinearSize(0.0f),
	mCrumbleEmitter(NULL),
	mDustEmitter(NULL),
	mCrumbleRenderVolume(NULL),
	mDustRenderVolume(NULL),
	mStartTime(scene.mElapsedTime),
	mBenefit(0.0f),
	mInitializedFromState(false),
	mRenderable(NULL),
	mDescOverrideSkinnedMaterialCount(0),
	mDescOverrideSkinnedMaterials(NULL),
	mDescOverrideStaticMaterialCount(0),
	mPhysXActorBufferAcquired(false),
	mInDeleteChunkMode(false),
	mAwakeActorCount(0),
	mActiveFrames(0),
	mDamageEventReportIndex(0xFFFFFFFF)
#if APEX_RUNTIME_FRACTURE
	//,mRTActor((PX_NEW(DestructibleRTActor)(*this)))
	,mRTActor(NULL)
#endif
#if USE_DESTRUCTIBLE_RWLOCK
	, mLock(NULL)
#endif
	, mWakeForEvent(false)
{
	// The client (DestructibleActorProxy) is responsible for calling initialize()
}

void DestructibleActorImpl::initialize(NvParameterized::Interface* p)
{
	// Given actor state
	if (NULL != p     && isType(p, DestructibleParameterizedType::State))
	{
		initializeFromState(p);
		initializeCommon();
	}
	// Given actor params
	else if(NULL != p && isType(p, DestructibleParameterizedType::Params))
	{
		initializeFromParams(p);
		initializeCommon();
	}
	else
	{
		PX_ALWAYS_ASSERT();
		APEX_INTERNAL_ERROR("Invalid destructible actor creation arguments.");
	}
}

void DestructibleActorImpl::initializeFromState(NvParameterized::Interface* state)
{
	setState(state);
	mInitializedFromState = true;
}

void DestructibleActorImpl::initializeFromParams(NvParameterized::Interface* params)
{
	setState(createDefaultState(params));
	mInitializedFromState                = false;
	mState->enableCrumbleEmitter = APEX_USE_PARTICLES ? true : false;
	mState->enableDustEmitter    = APEX_USE_PARTICLES ? true : false;
	mState->lod                  = PxMax(mAsset->getDepthCount(), (uint32_t)1) - 1;
}

void DestructibleActorImpl::initializeCommon()
{
	setInitialGlobalPose(getParams()->globalPose);

	mFlags = (uint16_t)(getParams()->dynamic ? Dynamic : 0);

	mDescOverrideSkinnedMaterialCount = (uint32_t)getParams()->overrideSkinnedMaterialNames.arraySizes[0];
	mDescOverrideStaticMaterialCount  = (uint32_t)getParams()->overrideStaticMaterialNames.arraySizes[0];
	mDescOverrideSkinnedMaterials	= (const char**)PX_ALLOC(sizeof(const char*)*(mDescOverrideSkinnedMaterialCount > 0 ? mDescOverrideSkinnedMaterialCount : 1), PX_DEBUG_EXP("DestructibleActor::initializeCommon_mDescOverrideSkinnedMaterials"));
	for (uint32_t i = 0; i < mDescOverrideSkinnedMaterialCount; ++i)
	{
		mDescOverrideSkinnedMaterials[i] = getParams()->overrideSkinnedMaterialNames.buf[i].buf;
	}
	PX_ALLOCA(staticMaterialNames, const char*, mDescOverrideStaticMaterialCount > 0 ? mDescOverrideStaticMaterialCount : 1);
	for (uint32_t i = 0; i < mDescOverrideStaticMaterialCount; ++i)
	{
		staticMaterialNames[i] = getParams()->overrideStaticMaterialNames.buf[i].buf;
	}

	DestructibleActorParam* p = getParams();
	DestructibleActorParamNS::ParametersStruct* ps = static_cast<DestructibleActorParamNS::ParametersStruct*>(p);

	PhysX3DescTemplateImpl p3Desc;
	deserialize(p3Desc, ps->p3ActorDescTemplate, ps->p3BodyDescTemplate, ps->p3ShapeDescTemplate);
	setPhysX3Template(&p3Desc);

#if USE_DESTRUCTIBLE_RWLOCK
	if (NULL == mLock)
	{
		mLock = (shdfnd::ReadWriteLock*)PX_ALLOC(sizeof(shdfnd::ReadWriteLock), PX_DEBUG_EXP("DestructibleActor::RWLock"));
		PX_PLACEMENT_NEW(mLock, shdfnd::ReadWriteLock);
	}
#endif

	PxVec3 extents = mAsset->getBounds().getExtents();
	mLinearSize = PxMax(PxMax(extents.x*getScale().x, extents.y*getScale().y), extents.z*getScale().z);

	setDestructibleParameters(getParams()->destructibleParameters, getParams()->depthParameters);

	initializeActor();

	if ((getDestructibleParameters().flags & nvidia::apex::DestructibleParametersFlag::CRUMBLE_VIA_RUNTIME_FRACTURE) != 0)
	{
		initializeRTActor();
	}
}

float DestructibleActorImpl::getCrumbleParticleSpacing() const
{
	if (mParams->crumbleParticleSpacing > 0.0f)
	{
		return mParams->crumbleParticleSpacing;
	}
#if APEX_USE_PARTICLES
	if (getCrumbleEmitter())
	{
		return 2 * getCrumbleEmitter()->getObjectRadius();
	}
#endif // APEX_USE_PARTICLES
	return 0.0f;
}

float DestructibleActorImpl::getDustParticleSpacing() const
{
	if (mParams->dustParticleSpacing > 0.0f)
	{
		return mParams->dustParticleSpacing;
	}
#if APEX_USE
	if (getDustEmitter())
	{
		return 2 * getDustEmitter()->getObjectRadius();
	}
#endif // APEX_USE_PARTICLES
	return 0.0f;
}

void DestructibleActorImpl::setInitialGlobalPose(const PxMat44& pose)
{
	mTM = pose;
	mOriginalBounds = mAsset->getBounds();
	PxMat44 scaledTM = mTM;
	scaledTM.scale(PxVec4(getScale(), 1.f));
	PxBounds3Transform(mOriginalBounds, scaledTM);
}

void DestructibleActorImpl::setState(NvParameterized::Interface* state)
{
	if (mState != state)
	{
		// wrong name?
		if (NULL != state && !isType(state, DestructibleParameterizedType::State))
		{
			APEX_INTERNAL_ERROR(
				"The parameterized interface is of type <%s> instead of <%s>.  "
				"This object will be initialized by an empty one instead!",
				state->className(),
				DestructibleActorState::staticClassName());

			state->destroy();
			state = NULL;
		}
		else if (NULL != state)
		{
			//TODO: Verify parameters
		}

		// If no state was given, create the default state
		if (NULL == state)
		{
			state = GetInternalApexSDK()->getParameterizedTraits()->createNvParameterized(DestructibleActorState::staticClassName());
}
		PX_ASSERT(state);

		// Reset the state if it already exists
		if (mState != NULL)
		{
			mState->setSerializationCallback(NULL);
			PX_ASSERT(mDestructibleScene && "Expected destructible scene with existing actor state.");
			reset();
		}

		mState  = static_cast<DestructibleActorState*>(state);
		mParams = NULL;
		mChunks = NULL;

		if (NULL != mState)
		{
			mState->setSerializationCallback(this, (void*)((intptr_t)DestructibleParameterizedType::State));

			// Cache a handle to the actorParameters
			NvParameterized::Handle handle(*mState);
			NvParameterized::Interface* p = NULL;
			mState->getParameterHandle("actorParameters", handle);
			mState->getParamRef(handle, p);
			if (NULL != p)
			{
				mParams = static_cast<DestructibleActorParam*>(p);
				mParams->setSerializationCallback(this, (void*)((intptr_t)DestructibleParameterizedType::Params));
			}
			else
				APEX_INTERNAL_ERROR("Invalid destructible actor state parameterization of actor params.");

			// Cache a handle to the actorChunks
			p = NULL;
			mState->getParameterHandle("actorChunks", handle);
			mState->getParamRef(handle, p);
			if (NULL != p)
			{
				mChunks = static_cast<DestructibleActorChunks*>(p);
			}
			else
				APEX_INTERNAL_ERROR("Invalid destructible actor state parameterization of actor params.");
		}
	}
}

void DestructibleActorImpl::createRenderable()
{
	RenderMeshActor* renderMeshActors[DestructibleActorMeshType::Count];
	for (int meshN = 0; meshN < DestructibleActorMeshType::Count; ++meshN)
	{
		renderMeshActors[meshN] = NULL;
	}

	// There were three choices here as to the context in which to create this renderMeshActor.
	//   a) Use the global ApexSDK context
	//	 b) Make the destructible actor itself an Context
	//	 c) Use the destructible actor's context (mContext)
	//
	// The decision was made to use option A, the global context, and have the destrucible actor's destructor
	// call the render mesh actor's release() method.  Option B was too much overhead for a single sub-actor
	// and option C would have introduced race conditions at context deletion time.
	PX_ASSERT(mAsset->getRenderMeshAsset());
	RenderMeshActorDesc renderableMeshDesc;
	renderableMeshDesc.visible = false;
	renderableMeshDesc.bufferVisibility = true;
	renderableMeshDesc.indexBufferHint = RenderBufferHint::DYNAMIC;
	renderableMeshDesc.keepVisibleBonesPacked = keepVisibleBonesPacked();
	renderableMeshDesc.forceBoneIndexChannel = !renderableMeshDesc.keepVisibleBonesPacked;
	renderableMeshDesc.overrideMaterials      = mDescOverrideSkinnedMaterials;
	renderableMeshDesc.overrideMaterialCount  = mDescOverrideSkinnedMaterialCount;
	renderableMeshDesc.keepPreviousFrameBoneBuffer = keepPreviousFrameBoneBuffer();

	renderMeshActors[DestructibleActorMeshType::Skinned] = mAsset->getRenderMeshAsset()->createActor(renderableMeshDesc);

	if (drawStaticChunksInSeparateMesh() && !(getFlags() & Dynamic))
	{
		// Create static render mesh
		renderableMeshDesc.renderWithoutSkinning = true;
		renderableMeshDesc.keepPreviousFrameBoneBuffer = false;
		PX_ALLOCA(staticMaterialNames, const char*, PxMax(mAsset->mParams->staticMaterialNames.arraySizes[0], 1));
		if (mDescOverrideStaticMaterialCount > 0)
		{
			// If static override materials are defined, use them
			renderableMeshDesc.overrideMaterialCount = mDescOverrideStaticMaterialCount;
			renderableMeshDesc.overrideMaterials     = mDescOverrideStaticMaterials;
		}
		else
		{
			// Otherwise, use the static materials in the asset, if they're defined
			renderableMeshDesc.overrideMaterialCount = (uint32_t)mAsset->mParams->staticMaterialNames.arraySizes[0];
			renderableMeshDesc.overrideMaterials = (const char**)staticMaterialNames;
			for (int i = 0; i < mAsset->mParams->staticMaterialNames.arraySizes[0]; ++i)
			{
				staticMaterialNames[i] = mAsset->mParams->staticMaterialNames.buf[i].buf;
			}
		}
		renderMeshActors[DestructibleActorMeshType::Static] = mAsset->getRenderMeshAsset()->createActor(renderableMeshDesc);
	}

	//#if APEX_RUNTIME_FRACTURE
	//mRenderable = PX_NEW(DestructibleRTRenderable)(renderMeshActors,mRTActor);
	//#else
	mRenderable = PX_NEW(DestructibleRenderableImpl)(renderMeshActors,getDestructibleAsset(),(int32_t)m_listIndex);
	//#endif
}

void DestructibleActorImpl::initializeActor(void)
{
	if (!mParams->doNotCreateRenderable)
	{
		createRenderable();
	}

	mFirstChunkIndex = DestructibleAssetImpl::InvalidChunkIndex;

	// Allow color channel replacement if requested
	mUseDamageColoring = getParams()->defaultBehaviorGroup.damageColorChange.magnitudeSquared() != 0.0f;
	for (int32_t behaviorGroupN = 0; !mUseDamageColoring && behaviorGroupN < getParams()->behaviorGroups.arraySizes[0]; ++behaviorGroupN)
	{
		mUseDamageColoring = getParams()->behaviorGroups.buf[behaviorGroupN].damageColorChange.magnitudeSquared() != 0.0f;
	}
	if (mUseDamageColoring)
	{
		RenderMeshAsset* rma = mAsset->getRenderMeshAsset();
		mDamageColorArrays.resize(rma->getSubmeshCount());
		for (uint32_t submeshIndex = 0; submeshIndex < rma->getSubmeshCount(); ++submeshIndex)
		{
			physx::Array<ColorRGBA>& damageColorArray = mDamageColorArrays[submeshIndex];
			const RenderSubmesh& submesh = rma->getSubmesh(submeshIndex);
			const VertexBuffer& vb = submesh.getVertexBuffer();
			const VertexFormat& vf = vb.getFormat();
			const int32_t colorBufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::COLOR));
			// Fix asset on &damageColorArray[0]
			if (vb.getVertexCount() != 0)
			{
				damageColorArray.resize(vb.getVertexCount());
				intrinsics::memZero(&damageColorArray[0], sizeof(ColorRGBA)*damageColorArray.size());	// Default to zero in case this array doesn't exist in the asset
				if (colorBufferIndex >= 0)
				{
					vb.getBufferData(&damageColorArray[0], RenderDataFormat::R8G8B8A8, 0, (uint32_t)colorBufferIndex, 0, vb.getVertexCount());
				}
				for (uint32_t typeN = 0; typeN < DestructibleActorMeshType::Count; ++typeN)
				{
					RenderMeshActorIntl* renderMeshActor = (RenderMeshActorIntl*)getRenderMeshActor((DestructibleActorMeshType::Enum)typeN);
					if (renderMeshActor != NULL)
					{
						renderMeshActor->setStaticColorReplacement(submeshIndex, &damageColorArray[0]);
					}
				}
			}
		}
	}

	initializeEmitters();

	mVisibleChunks.reserve(mAsset->getChunkCount());
	mVisibleChunks.lockCapacity(true);

	// This is done to handle old formats, and ensure we have a sufficient # of DepthParameters
	while (mDestructibleParameters.depthParametersCount < mAsset->getDepthCount() && mDestructibleParameters.depthParametersCount < DestructibleParameters::kDepthParametersCountMax)
	{
		DestructibleDepthParameters& params = mDestructibleParameters.depthParameters[mDestructibleParameters.depthParametersCount];
		if (mDestructibleParameters.depthParametersCount == 0)
		{
			// Set level 0 parameters to the more expensive settings
			memset(&params, 0xFF, sizeof(params));
		}
		++mDestructibleParameters.depthParametersCount;
	}

	// When we add ourselves to the ApexScene, it will call us back with setPhysXScene
	if (!findSelfInContext(*mDestructibleScene->mApexScene->getApexContext()))
	addSelfToContext(*mDestructibleScene->mApexScene->getApexContext());

	// Add ourself to our DestructibleScene
	if (!findSelfInContext(*DYNAMIC_CAST(ApexContext*)(mDestructibleScene)))
	{
		addSelfToContext(*DYNAMIC_CAST(ApexContext*)(mDestructibleScene));

		if (mAsset->mParams->chunkInstanceInfo.arraySizes[0] || mAsset->mParams->scatterMeshAssets.arraySizes[0])
		{
			mDestructibleScene->addInstancedActor(this);
		}
	}

	// Set a reasonable initial bounds until the first physics update
	mRenderBounds = getOriginalBounds();
	if (mRenderable != NULL)
	{
		mRenderable->setBounds(mRenderBounds);
	}
	mNonInstancedBounds = mRenderBounds;
	mInstancedBounds.setEmpty();
}

void DestructibleActorImpl::initializeRTActor(void)
{
#if APEX_RUNTIME_FRACTURE
	//mRTActor->initialize();
	PX_DELETE(mRTActor);
	mRTActor = (nvidia::fracture::Actor*)mDestructibleScene->getDestructibleRTScene()->createActor(this);
#endif
}

void DestructibleActorImpl::initializeEmitters(void)
{
#if APEX_USE_PARTICLES
	if (getCrumbleEmitterName() || mAsset->getCrumbleEmitterName())
	{
		const char* name;
		if (getCrumbleEmitterName())
		{
			name = getCrumbleEmitterName();
			mAsset->mCrumbleAssetTracker.addAssetName(name, false);
		}
		else
		{
			name = mAsset->getCrumbleEmitterName();
		}
		initCrumbleSystem(name);
	}
	{
		const char* name;
		if (getDustEmitterName())
		{
			name = getDustEmitterName();
			mAsset->mDustAssetTracker.addAssetName(name, false);
		}
		else
		{
			name = mAsset->getDustEmitterName();
		}
		initDustSystem(name);
	}
#endif // APEX_USE_PARTICLES
}

void DestructibleActorImpl::setDestructibleParameters(const DestructibleActorParamNS::DestructibleParameters_Type& destructibleParameters,
												  const DestructibleActorParamNS::DestructibleDepthParameters_DynamicArray1D_Type& destructibleDepthParameters)
{
	DestructibleParameters parameters;
	deserialize(destructibleParameters, destructibleDepthParameters, parameters);
	setDestructibleParameters(parameters);
}

void DestructibleActorImpl::setDestructibleParameters(const DestructibleParameters& _destructibleParameters)
{
	// If essentialDepth changes, must re-count essential visible chunks
	if(_destructibleParameters.essentialDepth != mDestructibleParameters.essentialDepth)
	{
		mEssentialVisibleDynamicChunkShapeCount = 0;
		const uint16_t* chunkIndexPtr = mVisibleChunks.usedIndices();
		const uint16_t* chunkIndexPtrStop = chunkIndexPtr + mVisibleChunks.usedCount();
		while (chunkIndexPtr < chunkIndexPtrStop)
		{
			uint16_t chunkIndex = *chunkIndexPtr++;
			if(getDynamic(chunkIndex))
			{
				if((uint32_t)getDestructibleAsset()->mParams->chunks.buf[chunkIndex].depth <= _destructibleParameters.essentialDepth)
				{
					mEssentialVisibleDynamicChunkShapeCount += getDestructibleAsset()->getChunkHullCount(chunkIndex);
				}
			}
		}
	}

	// For now we keep a cached copy of the parameters
	//    At some point we may want to move completely into the state
	mDestructibleParameters = _destructibleParameters;


	getParams()->supportDepth = PxClamp(getSupportDepth(), (uint32_t)0, mAsset->getDepthCount() - 1);

	// Make sure the depth params are filled completely
	uint32_t oldSize    = mDestructibleParameters.depthParametersCount;
	uint32_t neededSize = mAsset->getDepthCount();
	if (neededSize > DestructibleParameters::kDepthParametersCountMax)
	{
		neededSize = DestructibleParameters::kDepthParametersCountMax;
	}
	mDestructibleParameters.depthParametersCount = neededSize;
	// Fill in remaining with asset defaults
	for (uint32_t i = oldSize; i < neededSize; ++i)
	{
		mDestructibleParameters.depthParameters[i].setToDefault();
	}
}

/* An (emitter) actor in our context has been released */
void DestructibleActorImpl::removeActorAtIndex(uint32_t index)
{
#if APEX_USE_PARTICLES
	if (mDustEmitter == mActorArray[index]->getActor())
	{
		mDustEmitter = 0;
	}
	if (mCrumbleEmitter == mActorArray[index]->getActor())
	{
		mCrumbleEmitter = 0;
	}
#endif // APEX_USE_PARTICLES

	ApexContext::removeActorAtIndex(index);
}


void DestructibleActorImpl::wakeUp(void)
{
	PX_ASSERT(mDestructibleScene);
	if (mDestructibleScene)
	{
		mDestructibleScene->addToAwakeList(*this);
	}
}

void DestructibleActorImpl::putToSleep(void)
{
	PX_ASSERT(mDestructibleScene);
	if (mDestructibleScene)
	{
		mDestructibleScene->removeFromAwakeList(*this);
	}
}

// TODO:		clean up mAwakeActorCount management with mUsingActiveTransforms
// [APEX-671]	mind that un/referencedByActor() also call these functions

void DestructibleActorImpl::incrementWakeCount(void)
{
	if (!mAwakeActorCount)
	{
		wakeUp();
	}
	mAwakeActorCount++;
}

void DestructibleActorImpl::referencedByActor(PxRigidDynamic* actor)
{
	if (mReferencingActors.find(actor) == mReferencingActors.end())
	{
		// We need to check IS_SLEEPING instead of actor->isSleeping,
		// because the state might have changed between the last callback and now.
		// Here, the state of the last callback is needed.
		PhysXObjectDescIntl* desc = (PhysXObjectDescIntl*)(GetInternalApexSDK()->getPhysXObjectInfo(actor));
		if (mDestructibleScene != NULL && !mDestructibleScene->mUsingActiveTransforms && desc != NULL && !desc->getUserDefinedFlag(PhysXActorFlags::IS_SLEEPING))
		{
			incrementWakeCount();
		}
		mReferencingActors.pushBack(actor);
	}
}

void DestructibleActorImpl::unreferencedByActor(PxRigidDynamic* actor)
{
	if (mReferencingActors.findAndReplaceWithLast(actor))
	{
		PhysXObjectDescIntl* desc = (PhysXObjectDescIntl*)(GetInternalApexSDK()->getPhysXObjectInfo(actor));
		if (mDestructibleScene != NULL && !mDestructibleScene->mUsingActiveTransforms && desc != NULL && !desc->getUserDefinedFlag(PhysXActorFlags::IS_SLEEPING))
		{
			decrementWakeCount();
		}
	}
}

void DestructibleActorImpl::wakeForEvent()
{
	if (!mWakeForEvent)
	{
		mWakeForEvent = true;
		incrementWakeCount();
	}
}

void DestructibleActorImpl::resetWakeForEvent()
{
	if (mWakeForEvent)
	{
		mWakeForEvent = false;
		decrementWakeCount();
	}
}

void DestructibleActorImpl::decrementWakeCount(void)
{
	// this assert shouldn't happen, so if it does it means bad things, tell james.
	// basically we keep a counter of the number of awake bodies in a destructible actor
	// so that when its zero we know no updates are needed (normally updates are really expensive
	// per-destructible). So, if wake count is 0 and its trying to decrement it means
	// this value is completely out of sync and that can result in bad things (performance loss
	// or behavior loss). So, don't remove this assert!

	PX_ASSERT(mAwakeActorCount > 0);
	if (mAwakeActorCount > 0)
	{
		mAwakeActorCount--;
		if (!mAwakeActorCount)
		{
			putToSleep();
		}
	}
}

void DestructibleActorImpl::setChunkVisibility(uint16_t index, bool visibility)
{
	PX_ASSERT((int32_t)index < mAsset->mParams->chunks.arraySizes[0]);
	if (visibility)
	{
		if(mVisibleChunks.use(index))
		{
			if (createChunkEvents())
			{
				mChunkEventBufferLock.lock();
					
				if (mChunkEventBuffer.size() == 0 && mDestructibleScene->getModuleDestructible()->m_chunkStateEventCallbackSchedule != DestructibleCallbackSchedule::Disabled)
				{
					mStructure->dscene->mActorsWithChunkStateEvents.pushBack(this);
				}

				DestructibleChunkEvent& chunkEvent = mChunkEventBuffer.insert();
				chunkEvent.chunkIndex = index;
				chunkEvent.event = DestructibleChunkEvent::VisibilityChanged | DestructibleChunkEvent::ChunkVisible;

				mChunkEventBufferLock.unlock();
			}
			if(getDynamic(index))
			{
				const uint32_t chunkShapeCount = getDestructibleAsset()->getChunkHullCount(index);
				mVisibleDynamicChunkShapeCount += chunkShapeCount;
				if((uint32_t)getDestructibleAsset()->mParams->chunks.buf[index].depth <= mDestructibleParameters.essentialDepth)
				{
					mEssentialVisibleDynamicChunkShapeCount += chunkShapeCount;
				}
			}
		}
	}
	else
	{
		if(mVisibleChunks.free(index))
		{
			if(getDynamic(index))
			{
				const uint32_t chunkShapeCount = getDestructibleAsset()->getChunkHullCount(index);
				mVisibleDynamicChunkShapeCount = mVisibleDynamicChunkShapeCount >= chunkShapeCount ? mVisibleDynamicChunkShapeCount - chunkShapeCount : 0;
				if((uint32_t)getDestructibleAsset()->mParams->chunks.buf[index].depth <= mDestructibleParameters.essentialDepth)
				{
					mEssentialVisibleDynamicChunkShapeCount = mEssentialVisibleDynamicChunkShapeCount >= chunkShapeCount ? mEssentialVisibleDynamicChunkShapeCount - chunkShapeCount : 0;
				}
			}
			if (createChunkEvents())
			{
				mChunkEventBufferLock.lock();

				if (mChunkEventBuffer.size() == 0 && mDestructibleScene->getModuleDestructible()->m_chunkStateEventCallbackSchedule != DestructibleCallbackSchedule::Disabled)
				{
					mStructure->dscene->mActorsWithChunkStateEvents.pushBack(this);
				}

				DestructibleChunkEvent& chunkEvent = mChunkEventBuffer.insert();
				chunkEvent.chunkIndex = index;
				chunkEvent.event = DestructibleChunkEvent::VisibilityChanged;

				mChunkEventBufferLock.unlock();
			}
		}
	}
	DestructibleAssetParametersNS::Chunk_Type& sourceChunk = mAsset->mParams->chunks.buf[index];
	if ((sourceChunk.flags & DestructibleAssetImpl::Instanced) == 0)
	{
		// Not instanced - need to choose the static or dynamic mesh, and set visibility for the render mesh actor
		const DestructibleActorMeshType::Enum typeN = (getDynamic(index) || !drawStaticChunksInSeparateMesh()) ?
				DestructibleActorMeshType::Skinned : DestructibleActorMeshType::Static;
		RenderMeshActor* rma = getRenderMeshActor(typeN);
		if (rma != NULL)
		{
			RenderMeshActorIntl* rmi = static_cast<RenderMeshActorIntl*>(rma);
			const bool visibilityChanged = rmi->setVisibility(visibility, sourceChunk.meshPartIndex);
			if (keepPreviousFrameBoneBuffer() && visibilityChanged && visibility)
			{
				// Visibility changed from false to true.  If we're keeping the previous frame bone buffer, be sure to seed the previous frame buffer
				if (!mStructure->chunks[index + mFirstChunkIndex].isDestroyed())
				{
					rmi->setLastFrameTM(getChunkPose(index), getScale(), sourceChunk.meshPartIndex);
				}
			}
		}
	}
}

void DestructibleActorImpl::cacheModuleData() const
{
	PX_PROFILE_ZONE("DestructibleCacheChunkCookedCollisionMeshes", GetInternalApexSDK()->getContextId());

	if (mAsset == NULL)
	{
		PX_ASSERT(!"cacheModuleData: asset is NULL.\n");
		return;
	}
	// The DestructibleActor::cacheModuleData() method needs to avoid incrementing the ref count
	bool incCacheRefCount = false;
	physx::Array<PxConvexMesh*>* meshes = mStructure->dscene->mModule->mCachedData->getConvexMeshesForScale(*this->mAsset, 
																											getDestructibleScene()->getModuleDestructible()->getChunkCollisionHullCookingScale(),
																											incCacheRefCount);
	if (meshes == NULL)
	{
		PX_ASSERT(!"cacheModuleData: failed to create convex mesh cache for actor.\n");
		return;
	}
}

/* Called by Scene when the actor is added to the scene context or when the Scene's
 * PxScene reference changes.  PxScene can be NULL if the Scene is losing its PxScene reference.
 */
void DestructibleActorImpl::setPhysXScene(PxScene* pxScene)
{
	if (pxScene)
	{
		PX_ASSERT(mStructure == NULL);
		mDestructibleScene->insertDestructibleActor(mAPI);
	}
	else
	{
		PX_ASSERT(mStructure != NULL);
		removeSelfFromStructure();
	}
}

PxScene* DestructibleActorImpl::getPhysXScene() const
{
	return mDestructibleScene->mPhysXScene;
}

void DestructibleActorImpl::removeSelfFromStructure()
{
	if (!mStructure)
	{
		return;
	}

	DestructibleStructure* oldStructure = mStructure;

	mStructure->removeActor(this);

	if (oldStructure->destructibles.size() == 0)
	{
		oldStructure->dscene->removeStructure(oldStructure);
	}

	for (uint32_t typeN = 0; typeN < DestructibleActorMeshType::Count; ++typeN)
	{
		RenderMeshActor* renderMeshActor = getRenderMeshActor((DestructibleActorMeshType::Enum)typeN);
		if (renderMeshActor != NULL)
		{
			while (renderMeshActor->visiblePartCount() > 0)
			{
				renderMeshActor->setVisibility(false, (uint16_t)renderMeshActor->getVisibleParts()[renderMeshActor->visiblePartCount() - 1]);
			}
		}
	}

	mDestructibleScene->mTotalChunkCount -= mVisibleChunks.usedCount();

	mVisibleChunks.clear(mAsset->getChunkCount());
}

void DestructibleActorImpl::removeSelfFromScene()
{
	if (mDestructibleScene == NULL)
	{
		return;
	}

	mDestructibleScene->mDestructibles.direct(mID) = NULL;
	mDestructibleScene->mDestructibles.free(mID);

	mID = (uint32_t)InvalidID;
}

DestructibleActorImpl::~DestructibleActorImpl()
{
#if APEX_RUNTIME_FRACTURE
	PX_DELETE(mRTActor);
	mRTActor = NULL;
#endif
}

void DestructibleActorImpl::release()
{
	if (mInRelease)
	{
		return;
	}
	mInRelease = true;
	
	mAsset->module->mCachedData->releaseReferencesToConvexMeshesForActor(*this);

	// after this call, mAsset and all other members are freed and unusable
	mAsset->releaseDestructibleActor(*mAPI);

	// HC SVNT DRACONES
}

void DestructibleActorImpl::destroy()
{
	mDestructibleScene->removeReferencesToActor(*this);

	if (NULL != mDescOverrideSkinnedMaterials)
	{
		PX_FREE_AND_RESET(mDescOverrideSkinnedMaterials);
	}

	//ApexActor::destroy();
	mInRelease = true;
	renderDataLock();
	for (uint32_t i = 0 ; i < mContexts.size() ; i++)
	{
		ContextTrack& t = mContexts[i];
		t.ctx->removeActorAtIndex(t.index);
	}
	mContexts.clear();
	renderDataUnLock();

	ApexResource::removeSelf();

	// BRG - moving down here from the top of this function - mState seems to be needed
	if (NULL != mState)
	{
		mState->destroy();
		mState = NULL;
	}

#if APEX_RUNTIME_FRACTURE
	//if (mRTActor != NULL)
	//{
	//	mRTActor->notifyParentActorDestroyed();
	//}
#endif

	if (mRenderable != NULL)
	{
		mRenderable->release();
		mRenderable = NULL;
	}

#if APEX_USE_PARTICLES
	if (mDustEmitter)
	{
		mDustEmitter->release();
		mDustEmitter = NULL;
	}

	if (mCrumbleEmitter)
	{
		mCrumbleEmitter->release();
		mCrumbleEmitter = NULL;
	}
#endif // APEX_USE_PARTICLES

#if USE_DESTRUCTIBLE_RWLOCK
	if (mLock)
	{
		mLock->~shdfnd::ReadWriteLock();
		PX_FREE(mLock);
	}
	mLock = NULL;
#endif

	// acquire the buffer so we can properly release it
	const DestructibleChunkEvent* tmpChunkEventBuffer=NULL;
	uint32_t tmpChunkEventBufferSize = 0;
	acquireChunkEventBuffer(tmpChunkEventBuffer, tmpChunkEventBufferSize);

	releaseChunkEventBuffer();

	releasePhysXActorBuffer();

	removeSelfFromScene();

	if(0 != mSyncParams.getUserActorID())
	{
		setSyncParams(0, 0, NULL, NULL);
	}
}

void DestructibleActorImpl::reset()
{
	destroy();

	mInRelease = false;

	///////////////////////////////////////////////////////////////////////////

	mState                            = NULL;
	mParams                           = NULL;
	mTM                               = PxMat44(PxIdentity);
	mFlags                            = 0;
	mInternalFlags                    = 0;
	mStructure                        = NULL;
	mID                               = (uint32_t)InvalidID;
	mLinearSize                       = 0.0f;
	mCrumbleEmitter                   = NULL;
	mDustEmitter                      = NULL;
	mCrumbleRenderVolume              = NULL;
	mDustRenderVolume                 = NULL;
	mStartTime						  = mDestructibleScene->mElapsedTime;
	mInitializedFromState             = false;
	mAwakeActorCount                  = 0;
	mDescOverrideSkinnedMaterialCount = 0;
	mDescOverrideStaticMaterialCount  = 0;

	mStaticRoots.clear();
	mVisibleChunks.clear();

	#if APEX_RUNTIME_FRACTURE
	PX_DELETE(mRTActor);
	mRTActor = NULL;
	#endif
}

void DestructibleActorImpl::initializeChunk(uint32_t index, DestructibleStructure::Chunk& target) const
{
	const DestructibleAssetParametersNS::Chunk_Type& source = getDestructibleAsset()->mParams->chunks.buf[index];

	// Derived parameters
	target.destructibleID		= getID();
	target.reportID				= (uint32_t)DestructibleScene::InvalidReportID;
	target.indexInAsset			= (uint16_t)index;
	target.islandID				= (uint32_t)DestructibleStructure::InvalidID;

	// Serialized parameters
	if (mInitializedFromState && (int32_t)index < mChunks->data.arraySizes[0])
	{
		deserializeChunkData(mChunks->data.buf[index], target);
		if (target.visibleAncestorIndex != (int32_t)DestructibleStructure::InvalidChunkIndex)
			target.visibleAncestorIndex += getFirstChunkIndex();
	}
	// Default parameters
	else
	{
		target.state               = uint8_t(isInitiallyDynamic() ? ChunkDynamic : 0);
		target.flags               = 0;
		target.damage              = 0;
		target.localOffset         = getScale().multiply(getDestructibleAsset()->getChunkPositionOffset(index));
		const PxBounds3& bounds = getDestructibleAsset()->getChunkShapeLocalBounds(index);
		PxVec3 center       = bounds.getCenter();
		PxVec3 extents      = bounds.getExtents();
		center                     = center.multiply(getScale());
		extents                    = extents.multiply(getScale());
		target.localSphereCenter   = center;
		target.localSphereRadius   = extents.magnitude();
		target.clearShapes();
		target.controlledChunk     = NULL;

		if (source.numChildren == 0)
		{
			target.flags |= ChunkMissingChild;
		}
	}

#if USE_CHUNK_RWLOCK
	target.lock					= (shdfnd::ReadWriteLock*)PX_ALLOC(sizeof(shdfnd::ReadWriteLock), PX_DEBUG_EXP("DestructibleActor::RWLock"));
	PX_PLACEMENT_NEW(target.lock, shdfnd::ReadWriteLock);
#endif

	// I have not yet determined if this flag should be set for all actors,
	//    or only for those initialized purely from params
	/*if (source.numChildren == 0)
	{
		target.flags |= ChunkMissingChild;
	}*/
}


const DestructibleStructure::Chunk& DestructibleActorImpl::getChunk(uint32_t index) const
{
	PX_ASSERT(mStructure != NULL);
	PX_ASSERT(index + mFirstChunkIndex < mStructure->chunks.size());
	return mStructure->chunks[index + mFirstChunkIndex];
}

PxTransform DestructibleActorImpl::getChunkPose(uint32_t index) const
{
	SCOPED_PHYSX_LOCK_READ(mDestructibleScene->mApexScene);

	PX_ASSERT(mStructure != NULL);
	PX_ASSERT(index + mFirstChunkIndex < mStructure->chunks.size());
	PX_ASSERT(!mStructure->chunks[index + mFirstChunkIndex].isDestroyed());
	return mStructure->getChunkGlobalPose(mStructure->chunks[index + mFirstChunkIndex]);
}

PxTransform DestructibleActorImpl::getChunkTransform(uint32_t index) const
{
	PX_ASSERT(mStructure != NULL);
	PX_ASSERT(index + mFirstChunkIndex < mStructure->chunks.size());
	PX_ASSERT(!mStructure->chunks[index + mFirstChunkIndex].isDestroyed());
	return mStructure->getChunkGlobalTransform(mStructure->chunks[index + mFirstChunkIndex]);
}

bool DestructibleActorImpl::getInitialChunkDestroyed(uint32_t index) const
{
	PX_ASSERT((int32_t)index < mChunks->data.arraySizes[0]);
	return (int32_t)index < mChunks->data.arraySizes[0]
		? (mChunks->data.buf[index].shapesCount == 0 && mChunks->data.buf[index].visibleAncestorIndex == (int32_t)DestructibleStructure::InvalidChunkIndex)
		: false;
}

bool DestructibleActorImpl::getInitialChunkDynamic(uint32_t index) const
{
	PX_ASSERT((int32_t)index < mChunks->data.arraySizes[0]);
	return ((int32_t)index < mChunks->data.arraySizes[0])
		? ((mChunks->data.buf[index].state & ChunkDynamic) != 0) : true;
}

bool DestructibleActorImpl::getInitialChunkVisible(uint32_t index) const
{
	PX_ASSERT((index == 0 && mChunks->data.arraySizes[0] == 0) ||
		      (int32_t)index < mChunks->data.arraySizes[0]);
	return ((int32_t)index < mChunks->data.arraySizes[0])
		? ((mChunks->data.buf[index].state & ChunkVisible) != 0) : true;
}

PxTransform DestructibleActorImpl::getInitialChunkGlobalPose(uint32_t index) const
{
	PX_ASSERT((int32_t)index < mChunks->data.arraySizes[0]);
	return (int32_t)index < mChunks->data.arraySizes[0]
		? mChunks->data.buf[index].globalPose : PxTransform(PxIdentity);
}

PxTransform DestructibleActorImpl::getInitialChunkLocalPose(uint32_t index) const
{
	return PxTransform(getDestructibleAsset()->getChunkPositionOffset(index));
}

PxVec3 DestructibleActorImpl::getInitialChunkLinearVelocity(uint32_t index) const
{
	PX_ASSERT((int32_t)index < mChunks->data.arraySizes[0]);
	return (int32_t)index < mChunks->data.arraySizes[0]
		? mChunks->data.buf[index].linearVelocity : PxVec3(0);
}

PxVec3 DestructibleActorImpl::getInitialChunkAngularVelocity(uint32_t index) const
{
	PX_ASSERT((int32_t)index < mChunks->data.arraySizes[0]);
	return (int32_t)index < mChunks->data.arraySizes[0]
		? mChunks->data.buf[index].angularVelocity : PxVec3(0);
}

PxVec3 DestructibleActorImpl::getChunkLinearVelocity(uint32_t index) const
{
	PX_ASSERT(mStructure != NULL);
	PX_ASSERT(index + mFirstChunkIndex < mStructure->chunks.size());
	PX_ASSERT(!mStructure->chunks[index + mFirstChunkIndex].isDestroyed());
	SCOPED_PHYSX_LOCK_READ(mDestructibleScene->mApexScene);
	return getChunkActor(index)->getLinearVelocity();
}

PxVec3 DestructibleActorImpl::getChunkAngularVelocity(uint32_t index) const
{
	PX_ASSERT(mStructure != NULL);
	PX_ASSERT(index + mFirstChunkIndex < mStructure->chunks.size());
	PX_ASSERT(!mStructure->chunks[index + mFirstChunkIndex].isDestroyed());
	SCOPED_PHYSX_LOCK_READ(mDestructibleScene->mApexScene);
	return getChunkActor(index)->getAngularVelocity();
}

PxRigidDynamic* DestructibleActorImpl::getChunkActor(uint32_t index)
{
	PX_ASSERT(mStructure != NULL);
	PX_ASSERT(index + mFirstChunkIndex < mStructure->chunks.size());
	DestructibleStructure::Chunk& chunk = mStructure->chunks[index + mFirstChunkIndex];
	return mStructure->getChunkActor(chunk);
}

const PxRigidDynamic* DestructibleActorImpl::getChunkActor(uint32_t index) const
{
	PX_ASSERT(mStructure != NULL);
	PX_ASSERT(index + mFirstChunkIndex < mStructure->chunks.size());
	DestructibleStructure::Chunk& chunk = mStructure->chunks[index + mFirstChunkIndex];
	return mStructure->getChunkActor(chunk);
}

uint32_t DestructibleActorImpl::getChunkPhysXShapes(physx::PxShape**& shapes, uint32_t chunkIndex) const
{
	PX_ASSERT(mStructure != NULL);
	PX_ASSERT(chunkIndex + mFirstChunkIndex < mStructure->chunks.size());
	DestructibleStructure::Chunk& chunk = mStructure->chunks[chunkIndex + mFirstChunkIndex];
	physx::Array<PxShape*>& shapeArray = mStructure->getChunkShapes(chunk);
	shapes = shapeArray.size() ? &shapeArray[0] : NULL;
	return shapeArray.size();
}

uint32_t DestructibleActorImpl::getChunkActorFlags(uint32_t index) const
{
	PX_ASSERT(mStructure != NULL);
	PX_ASSERT(index + mFirstChunkIndex < mStructure->chunks.size());
	DestructibleStructure::Chunk& chunk = mStructure->chunks[index + mFirstChunkIndex];
	uint32_t flags = 0;
	if (chunk.flags & ChunkWorldSupported)
	{
		flags |= DestructibleActorChunkFlags::ChunkIsWorldSupported;
	}
	return flags;
}

void DestructibleActorImpl::applyDamage(float damage, float momentum, const PxVec3& position, const PxVec3& direction, int32_t chunkIndex, void* damageUserData)
{
	DamageEvent& damageEvent = mStructure->dscene->getDamageWriteBuffer().pushBack();
	damageEvent.destructibleID = mID;
	damageEvent.damage = damage;
	damageEvent.momentum = momentum;
	damageEvent.position = position;
	damageEvent.direction = direction;
	damageEvent.radius = 0.0f;
	damageEvent.chunkIndexInAsset = chunkIndex >= 0 ? chunkIndex : ModuleDestructibleConst::INVALID_CHUNK_INDEX;
	damageEvent.flags = 0;
	damageEvent.impactDamageActor = NULL;
	damageEvent.appliedDamageUserData = damageUserData;
}

void DestructibleActorImpl::applyRadiusDamage(float damage, float momentum, const PxVec3& position, float radius, bool falloff, void* damageUserData)
{
	DamageEvent& damageEvent = mStructure->dscene->getDamageWriteBuffer().pushBack();
	damageEvent.destructibleID = mID;
	damageEvent.damage = damage;
	damageEvent.momentum = momentum;
	damageEvent.position = position;
	damageEvent.direction = PxVec3(0.0f);	// not used
	damageEvent.radius = radius;
	damageEvent.chunkIndexInAsset = ModuleDestructibleConst::INVALID_CHUNK_INDEX;
	damageEvent.flags = DamageEvent::UseRadius;
	damageEvent.impactDamageActor = NULL;
	damageEvent.appliedDamageUserData = damageUserData;
	if (falloff)
	{
		damageEvent.flags |= DamageEvent::HasFalloff;
	}
}

void DestructibleActorImpl::takeImpact(const PxVec3& force, const PxVec3& position, uint16_t chunkIndex, PxActor const* damageActor)
{
	if (chunkIndex >= (uint16_t)mAsset->mParams->chunks.arraySizes[0])
	{
		return;
	}
	DestructibleAssetParametersNS::Chunk_Type& source = mAsset->mParams->chunks.buf[chunkIndex];
	if (!takesImpactDamageAtDepth(source.depth))
	{
		return;
	}
	DamageEvent& damageEvent = mStructure->dscene->getDamageWriteBuffer().pushBack();
	damageEvent.direction = force;
	damageEvent.destructibleID = mID;
	const float magnitude = damageEvent.direction.normalize();
	damageEvent.damage = magnitude * mDestructibleParameters.forceToDamage;
	damageEvent.momentum = 0.0f;
	damageEvent.position = position;
	damageEvent.radius = 0.0f;
	damageEvent.chunkIndexInAsset = chunkIndex;
	damageEvent.flags = DamageEvent::IsFromImpact;
	damageEvent.impactDamageActor = damageActor;
	damageEvent.appliedDamageUserData = NULL;

	if (mStructure->dscene->mModule->m_impactDamageReport != NULL)
	{
		ImpactDamageEventData& data = mStructure->dscene->mImpactDamageEventData.insert();
		(DamageEventCoreData&)data = (DamageEventCoreData&)damageEvent;	// Copy core data
		data.destructible = mAPI;
		data.direction = damageEvent.direction;
		data.impactDamageActor = damageActor;
	}
}

int32_t DestructibleActorImpl::pointOrOBBSweep(float& time, PxVec3& normal, const PxVec3& worldBoxCenter, const PxVec3& worldBoxExtents, const PxMat33& worldBoxRT, const PxVec3& pxWorldDisp,
        DestructibleActorRaycastFlags::Enum flags, int32_t parentChunkIndex) const
{
	PX_PROFILE_ZONE("DestructibleActorPointOrOBBSweep", GetInternalApexSDK()->getContextId());

	// use the scene lock to protect chunk data for
	// multi-threaded obbSweep calls on destructible actors
	// (different lock would be good, but mixing locks can cause deadlocks)
	SCOPED_PHYSX_LOCK_READ(mDestructibleScene->mApexScene);

	// TODO: the normal is not always output, for instance, when accurateRaycasts is false, can we generate a decent normal in this case?
	const bool pointSweep = (worldBoxExtents.magnitudeSquared() == 0.0f);

	const uint32_t dynamicStateFlags = ((uint32_t)flags)&DestructibleActorRaycastFlags::AllChunks;
	if (dynamicStateFlags == 0)
	{
		return ModuleDestructibleConst::INVALID_CHUNK_INDEX;
	}

	const PxVec3 worldBoxAxes[3] = { worldBoxRT.column0, worldBoxRT.column1, worldBoxRT.column2 };
	const PxVec3 worldDisp = pxWorldDisp;

	// Project box along displacement direction, for the entire sweep
	const float worldDisp2 = worldDisp.magnitudeSquared();
	const bool sweep = (worldDisp2 != 0.0f);
	const float recipWorldDisp2 = sweep ? 1.0f / worldDisp2 : 0.0f;
	const float boxProjectedRadius = worldBoxExtents.x * PxAbs(worldDisp.dot(worldBoxAxes[0])) +
	                                        worldBoxExtents.y * PxAbs(worldDisp.dot(worldBoxAxes[1])) +
											worldBoxExtents.z * PxAbs(worldDisp.dot(worldBoxAxes[2]));
	const float boxProjectedCenter = worldDisp.dot(worldBoxCenter);
	const float boxBSphereRadius = worldBoxExtents.magnitude();	// May reduce this by projecting the box along worldDisp

	float boxSweptMax;
	float minRayT;
	float maxRayT;

	const float boxProjectedMin = boxProjectedCenter - boxProjectedRadius;
	const float boxProjectedMax = boxProjectedCenter + boxProjectedRadius;
	if ((flags & DestructibleActorRaycastFlags::SegmentIntersect) != 0)
	{
		boxSweptMax = boxProjectedMax + worldDisp2;
		minRayT = 0.0f;
		maxRayT = 1.0f;
	}
	else
	{
		boxSweptMax = PX_MAX_F32;
		minRayT = -PX_MAX_F32;
		maxRayT = PX_MAX_F32;
	}

	PxVec3 rayorig;
	PxVec3 raydir;

//	OverlapLineSegmentAABBCache segmentCache;
	if (pointSweep)
	{
//		computeOverlapLineSegmentAABBCache(segmentCache, worldDisplacement);
		rayorig = worldBoxCenter;
		raydir = worldDisp;
	}

	float minTime = PX_MAX_F32;
	uint32_t totalVisibleChunkCount;
	const uint16_t* visibleChunkIndices;
	uint16_t dummyIndexArray = (uint16_t)parentChunkIndex;
	if (parentChunkIndex == ModuleDestructibleConst::INVALID_CHUNK_INDEX)
	{
		totalVisibleChunkCount = mVisibleChunks.usedCount();
		visibleChunkIndices = mVisibleChunks.usedIndices();
	}
	else
	{
		totalVisibleChunkCount = 1;
		visibleChunkIndices = &dummyIndexArray;
	}

	IndexedReal* chunkProjectedMinima = (IndexedReal*)PxAlloca(totalVisibleChunkCount * sizeof(IndexedReal));
	uint32_t* chunkIndices = (uint32_t*)PxAlloca(totalVisibleChunkCount * sizeof(uint32_t));
	PxTransform* chunkTMs = (PxTransform*)PxAlloca(totalVisibleChunkCount * sizeof(PxTransform));

#if USE_DESTRUCTIBLE_RWLOCK
	mLock->lockReader();
#endif
	{
		// Must find an intersecting visible chunk.
		PX_PROFILE_ZONE("DestructibleRayCastFindVisibleChunk", GetInternalApexSDK()->getContextId());

		// Find candidate chunks
		uint32_t candidateChunkCount = 0;

		for (uint32_t chunkNum = 0; chunkNum < totalVisibleChunkCount; ++chunkNum)
		{
			if (candidateChunkCount >= totalVisibleChunkCount)
			{
				PX_ALWAYS_ASSERT();
				break;
			}
			const uint32_t chunkIndexInAsset = visibleChunkIndices[chunkNum];
			DestructibleStructure::Chunk& chunk = mStructure->chunks[chunkIndexInAsset + mFirstChunkIndex];
			DestructibleAssetParametersNS::Chunk_Type& chunkSource = mAsset->mParams->chunks.buf[chunkIndexInAsset];
			if (mDestructibleParameters.depthParameters[chunkSource.depth].ignoresRaycastCallbacks())
			{
				continue;
			}
			if ((chunk.state & ChunkVisible) == 0)
			{
				continue;	// In case a valid parentChunkIndex was passed in
			}
#if USE_CHUNK_RWLOCK
			DestructibleStructure::ChunkScopedReadLock chunkReadLock(chunk);
#endif
			if (chunk.isDestroyed())
			{
				continue;
			}

			if (dynamicStateFlags != DestructibleActorRaycastFlags::AllChunks)
			{
				if ((chunk.state & ChunkDynamic)==0)
				{
					if ((flags & DestructibleActorRaycastFlags::StaticChunks) == 0)
					{
						continue;
					}
				}
				else
				{
					if ((flags & DestructibleActorRaycastFlags::DynamicChunks) == 0)
					{
						continue;
					}
				}
			}

			PxTransform& tm = chunkTMs[candidateChunkCount];
			tm = mStructure->getChunkGlobalPose(chunk); // Cache this off
			IndexedReal& chunkProjectedMin = chunkProjectedMinima[candidateChunkCount];

			if (sweep)
			{
				// Project chunk bounds along displacement direction
				PxVec3 rM = tm.q.rotateInv(pxWorldDisp);
				const float chunkProjectedCenter = rM.dot(chunk.localSphereCenter) + pxWorldDisp.dot(tm.p);
				PxVec3 chunkLocalExtents = mAsset->getChunkShapeLocalBounds(chunkIndexInAsset).getExtents();
				chunkLocalExtents = chunkLocalExtents.multiply(getScale());
				const float chunkProjectedRadius = chunkLocalExtents.x * PxAbs(rM.x) +
				        chunkLocalExtents.y * PxAbs(rM.y) +
				        chunkLocalExtents.z * PxAbs(rM.z);
				chunkProjectedMin.value = chunkProjectedCenter - chunkProjectedRadius;
				if (boxProjectedMin >= chunkProjectedCenter + chunkProjectedRadius || boxSweptMax <= chunkProjectedMin.value)
				{
					// Beyond or before projected sweep
					continue;
				}

				// Perform "corridor test"
				const PxVec3 X = tm.transform(chunk.localSphereCenter) - worldBoxCenter;
				const float sumRadius = boxBSphereRadius + chunk.localSphereRadius;
				const float Xv = X.dot(pxWorldDisp);
				if (worldDisp2 * (X.magnitudeSquared() - sumRadius * sumRadius) >= Xv * Xv)
				{
					// Outside of corridor
					continue;
				}
			}
			else
			{
				// Overlap test
				const PxVec3 X = tm.transform(chunk.localSphereCenter) - worldBoxCenter;
				const float sumRadius = boxBSphereRadius + chunk.localSphereRadius;
				if (X.magnitudeSquared() >= sumRadius * sumRadius)
				{
					continue;
				}
			}

			// We'll keep this one
			chunkProjectedMin.index = candidateChunkCount;
			chunkIndices[candidateChunkCount++] = chunkIndexInAsset;
#if USE_CHUNK_RWLOCK
			chunk.lock->lockReader();	// Add another read lock, be sure to unlock after use in next loop
#endif
		}

		// Sort chunk bounds projected minima
		if (sweep && candidateChunkCount > 1)
		{
			combsort(chunkProjectedMinima, candidateChunkCount);
		}

		uint32_t candidateChunkNum = 0;
		for (; candidateChunkNum < candidateChunkCount; ++candidateChunkNum)
		{
			IndexedReal& indexedChunkMin = chunkProjectedMinima[candidateChunkNum];
			if (sweep && minTime <= (indexedChunkMin.value - boxProjectedMax)*recipWorldDisp2)
			{
				// Early-out
				break;
			}
			uint16_t chunkIndexInAsset = (uint16_t)chunkIndices[indexedChunkMin.index];
#if USE_CHUNK_RWLOCK
			DestructibleStructure::Chunk& chunk = mStructure->chunks[chunkIndexInAsset + mFirstChunkIndex];
#endif
			for (uint32_t hullIndex = mAsset->getChunkHullIndexStart(chunkIndexInAsset); hullIndex < mAsset->getChunkHullIndexStop(chunkIndexInAsset); ++hullIndex)
			{
				ConvexHullImpl& chunkSourceConvexHull = mAsset->chunkConvexHulls[hullIndex];
				float in = minRayT;
				float out = maxRayT;
				PxVec3 n;
				PxVec3 pxWorldBoxAxes[3];
				pxWorldBoxAxes[0] = worldBoxAxes[0];
				pxWorldBoxAxes[1] = worldBoxAxes[1];
				pxWorldBoxAxes[2] = worldBoxAxes[2];
				const bool hit = pointSweep ? (/* overlapLineSegmentAABBCached(rayorig, segmentCache, chunkSource.bounds) && */
				                     chunkSourceConvexHull.rayCast(in, out, rayorig, raydir, chunkTMs[indexedChunkMin.index], getScale(), &n)) :
				                 chunkSourceConvexHull.obbSweep(in, out, worldBoxCenter, worldBoxExtents, pxWorldBoxAxes, pxWorldDisp, chunkTMs[indexedChunkMin.index], getScale(), &n);
				if (hit)
				{
					if (out > minRayT && in < minTime)
					{
						minTime = in;
						normal = n;
						parentChunkIndex = chunkIndexInAsset != (uint16_t)DestructibleAssetImpl::InvalidChunkIndex ? chunkIndexInAsset : (uint16_t)ModuleDestructibleConst::INVALID_CHUNK_INDEX;
					}
				}
			}
#if USE_CHUNK_RWLOCK
			chunk.lock->unlockReader();	// Releasing lock from end of previous loop
#endif
		}

#if USE_CHUNK_RWLOCK
		// Release remaining locks
		for (; candidateChunkNum < candidateChunkCount; ++candidateChunkNum)
		{
			IndexedReal& indexedChunkMin = chunkProjectedMinima[candidateChunkNum];
			uint16_t chunkIndexInAsset = (uint16_t)chunkIndices[indexedChunkMin.index];
			DestructibleStructure::Chunk& chunk = mStructure->chunks[chunkIndexInAsset + mFirstChunkIndex];
			chunk.lock->unlockReader();
		}
#endif
	}

#if USE_DESTRUCTIBLE_RWLOCK
	mLock->unlockReader();
#endif

	if (minTime != PX_MAX_F32)
	{
		time = minTime;
	}

	bool accurateRaycasts = (mDestructibleParameters.flags & DestructibleParametersFlag::ACCURATE_RAYCASTS) != 0;

	if (((uint32_t)flags)&DestructibleActorRaycastFlags::ForceAccurateRaycastsOn)
	{
		accurateRaycasts = true;
	}

	if (((uint32_t)flags)&DestructibleActorRaycastFlags::ForceAccurateRaycastsOff)
	{
		accurateRaycasts = false;
	}

	if (!accurateRaycasts)
	{
		return parentChunkIndex;
	}

	int32_t chunkIndex = parentChunkIndex;

	{
		PX_PROFILE_ZONE("DestructibleRayCastFindDeepestChunk", GetInternalApexSDK()->getContextId());

		SCOPED_PHYSX_LOCK_READ(mDestructibleScene->mApexScene);

#if USE_DESTRUCTIBLE_RWLOCK
		mLock->lockReader();
#endif

		while (parentChunkIndex != ModuleDestructibleConst::INVALID_CHUNK_INDEX)
		{
			DestructibleAssetParametersNS::Chunk_Type& source = mAsset->mParams->chunks.buf[parentChunkIndex];
			float firstInTime = PX_MAX_F32;
			parentChunkIndex = ModuleDestructibleConst::INVALID_CHUNK_INDEX;
			const uint16_t childStop = source.numChildren;
			if (childStop > 0 && mDestructibleParameters.depthParameters[source.depth + 1].ignoresRaycastCallbacks())
			{
				// Children ignore raycasts.  We may stop here.
				break;
			}
			for (uint16_t childNum = 0; childNum < childStop; ++childNum)
			{
				const uint16_t childIndexInAsset = uint16_t(source.firstChildIndex + childNum);
				DestructibleStructure::Chunk& child = mStructure->chunks[childIndexInAsset + mFirstChunkIndex];
#if USE_CHUNK_RWLOCK
				DestructibleStructure::ChunkScopedReadLock chunkReadLock(child);
#endif
				if (!child.isDestroyed())
				{
					for (uint32_t hullIndex = mAsset->getChunkHullIndexStart(childIndexInAsset); hullIndex < mAsset->getChunkHullIndexStop(childIndexInAsset); ++hullIndex)
					{
						ConvexHullImpl& childSourceConvexHull = mAsset->chunkConvexHulls[hullIndex];
						float in = minRayT;
						float out = maxRayT;
						PxVec3 n;
						PxVec3 pxWorldBoxAxes[3];
						pxWorldBoxAxes[0] = worldBoxAxes[0];
						pxWorldBoxAxes[1] = worldBoxAxes[1];
						pxWorldBoxAxes[2] = worldBoxAxes[2];
						const bool hit = pointSweep ? (/* overlapLineSegmentAABBCached(rayorig, segmentCache, childSource.bounds) && */
						                     childSourceConvexHull.rayCast(in, out, rayorig, raydir, mStructure->getChunkGlobalPose(child), getScale(), &n)) :
						                 childSourceConvexHull.obbSweep(in, out, worldBoxCenter, worldBoxExtents, pxWorldBoxAxes, pxWorldDisp, mStructure->getChunkGlobalPose(child), getScale(), &n);
						if (hit)
						{
							if (out > minRayT && in < firstInTime)
							{
								firstInTime = in;
								normal = n;
								parentChunkIndex = childIndexInAsset != (int32_t)DestructibleAssetImpl::InvalidChunkIndex ? childIndexInAsset : (int32_t)ModuleDestructibleConst::INVALID_CHUNK_INDEX;
								if (child.state & ChunkVisible)
								{
									chunkIndex = parentChunkIndex;
								}
							}
						}
					}
				}
			}
			if (firstInTime != PX_MAX_F32)
			{
				time = firstInTime;
			}
		}

	}

#if USE_DESTRUCTIBLE_RWLOCK
	mLock->unlockReader();
#endif

	return chunkIndex;
}

int32_t DestructibleActorImpl::pointOrOBBSweepStatic(float& time, PxVec3& normal, const PxVec3& worldBoxCenter, const PxVec3& worldBoxExtents, const PxMat33& worldBoxRT, const PxVec3& pxWorldDisp,
        DestructibleActorRaycastFlags::Enum flags, int32_t parentChunkIndex) const
{
	PX_PROFILE_ZONE("DestructibleActorPointOrOBBSweepStatic", GetInternalApexSDK()->getContextId());

	PX_ASSERT((flags & DestructibleActorRaycastFlags::DynamicChunks) == 0);

	// use the scene lock to protect chunk data for
	// multi-threaded obbSweep calls on destructible actors
	// (different lock would be good, but mixing locks can cause deadlocks)
	SCOPED_PHYSX_LOCK_READ(mDestructibleScene->mApexScene);

	// parentChunkIndex out of range
	if (parentChunkIndex >= mAsset->mParams->chunks.arraySizes[0])
	{
		return ModuleDestructibleConst::INVALID_CHUNK_INDEX;
	}

	DestructibleAssetParametersNS::Chunk_Type* sourceChunks = mAsset->mParams->chunks.buf;

	// parentChunkIndex is valid, but the chunk is invisible, beyond the LOD, or ignores raycasts
	if (parentChunkIndex >= 0)
	{
		if ((getStructure()->chunks[parentChunkIndex + getFirstChunkIndex()].state & ChunkVisible) == 0)
		{
			return ModuleDestructibleConst::INVALID_CHUNK_INDEX;
		}
	}
	else
	{
		// From now on, we treat parentChunkIndex < 0 as parentChunkIndex = 0 and iterate
		parentChunkIndex = 0;
	}

	if (sourceChunks[parentChunkIndex].depth > (uint16_t)getLOD() || mDestructibleParameters.depthParameters[sourceChunks[parentChunkIndex].depth].ignoresRaycastCallbacks())
	{
		return ModuleDestructibleConst::INVALID_CHUNK_INDEX;
	}

	// TODO: the normal is not always output, for instance, when accurateRaycasts is false, can we generate a decent normal in this case?
	const bool pointSweep = (worldBoxExtents.magnitudeSquared() == 0.0f);

	const PxVec3 worldBoxAxes[3] = { worldBoxRT.column0,  worldBoxRT.column1,  worldBoxRT.column2 };
	const PxVec3 worldDisp = pxWorldDisp;

	float minRayT;
	float maxRayT;
	if ((flags & DestructibleActorRaycastFlags::SegmentIntersect) != 0)
	{
		minRayT = 0.0f;
		maxRayT = 1.0f;
	}
	else
	{
		minRayT = -PX_MAX_F32;
		maxRayT = PX_MAX_F32;
	}

	PxVec3 rayorig;
	PxVec3 raydir;
	if (pointSweep)
	{
		rayorig = worldBoxCenter;
		raydir = worldDisp;
	}

	// Must find an intersecting visible chunk.

	PxMat44 staticTM;
	bool success = getGlobalPoseForStaticChunks(staticTM);
	if (!success)
	{
		return ModuleDestructibleConst::INVALID_CHUNK_INDEX;
	}
	PxTransform chunkTM(staticTM);	// We'll keep updating the position for this one

	bool accurateRaycasts = (mDestructibleParameters.flags & DestructibleParametersFlag::ACCURATE_RAYCASTS) != 0;
	if (((uint32_t)flags)&DestructibleActorRaycastFlags::ForceAccurateRaycastsOn)
	{
		accurateRaycasts = true;
	}
	if (((uint32_t)flags)&DestructibleActorRaycastFlags::ForceAccurateRaycastsOff)
	{
		accurateRaycasts = false;
	}

	int32_t chunkFoundIndex = ModuleDestructibleConst::INVALID_CHUNK_INDEX;
	float minTime = PX_MAX_F32;
	PxVec3 normalAtFirstHit(0.0f, 0.0f, 1.0f);

	// Start with the parentChunkIndex.  This stack is a stack of chunk ranges, stored LSW = low, MSW = high
	physx::Array<uint32_t> stack;
	stack.pushBack(((uint32_t)parentChunkIndex+1)<<16 | (uint32_t)parentChunkIndex);

	while (stack.size() > 0)
	{
		const uint32_t range = stack.popBack();
		uint32_t chunkIndex = (range&0xFFFF) + getFirstChunkIndex();
		uint32_t chunkIndexStop = (range>>16) + getFirstChunkIndex();
		const uint16_t depth = sourceChunks[range&0xFFFF].depth;
		const bool atLimit = depth >= getLOD() || depth >= mAsset->mParams->depthCount - 1 || mDestructibleParameters.depthParameters[depth+1].ignoresRaycastCallbacks();

		for (; chunkIndex < chunkIndexStop; ++chunkIndex)
		{
			const DestructibleStructure::Chunk& chunk = getStructure()->chunks[chunkIndex];
			if ((chunk.state & ChunkDynamic) != 0)
			{
				continue;
			}
			chunkTM.p = staticTM.transform(chunk.localOffset);
			const uint16_t chunkIndexInAsset = (uint16_t)(chunkIndex - getFirstChunkIndex());
			for (uint32_t hullIndex = mAsset->getChunkHullIndexStart(chunkIndexInAsset); hullIndex < mAsset->getChunkHullIndexStop(chunkIndexInAsset); ++hullIndex)
			{
				ConvexHullImpl& chunkSourceConvexHull = mAsset->chunkConvexHulls[hullIndex];
				float in = minRayT;
				float out = maxRayT;
				PxVec3 n;
				bool hit;
				if (pointSweep)
				{
					hit = chunkSourceConvexHull.rayCast(in, out, rayorig, raydir, chunkTM, getScale(), &n);
				}
				else
				{
					PxVec3 pxWorldBoxAxes[3];
					pxWorldBoxAxes[0] = worldBoxAxes[0];
					pxWorldBoxAxes[1] = worldBoxAxes[1];
					pxWorldBoxAxes[2] = worldBoxAxes[2];
					hit = chunkSourceConvexHull.obbSweep(in, out, worldBoxCenter, worldBoxExtents, pxWorldBoxAxes, pxWorldDisp, chunkTM, getScale(), &n);
				}

				if (hit)
				{
					if (out > 0.0f && in < minTime)
					{
						const uint32_t firstChildIndexInAsset = (uint32_t)sourceChunks[chunkIndexInAsset].firstChildIndex;
						const uint32_t childCount = (uint32_t)sourceChunks[chunkIndexInAsset].numChildren;
						const bool hasChildren = !atLimit && childCount > 0;

						if (!accurateRaycasts)
						{
							if (chunk.state & ChunkVisible)
							{
								minTime = in;
								normalAtFirstHit = n;
								chunkFoundIndex = (int32_t)chunkIndex;
							}
							else
							if (chunk.isDestroyed() && hasChildren)
							{
								stack.pushBack(((uint32_t)firstChildIndexInAsset+childCount)<<16 | (uint32_t)firstChildIndexInAsset);
							}
						}
						else
						{
							if (!hasChildren)
							{
								minTime = in;
								normalAtFirstHit = n;
								chunkFoundIndex = (int32_t)chunkIndex;
							}
							else
							{
								stack.pushBack(((uint32_t)firstChildIndexInAsset+childCount)<<16 | (uint32_t)firstChildIndexInAsset);
							}
						}
					}
				}
			}
		}
	}

	if (chunkFoundIndex == ModuleDestructibleConst::INVALID_CHUNK_INDEX || minTime == PX_MAX_F32)
	{
		return ModuleDestructibleConst::INVALID_CHUNK_INDEX;
	}

	time = minTime;
	normal = normalAtFirstHit;

	if (accurateRaycasts)
	{
		const DestructibleStructure::Chunk& chunkFound = getStructure()->chunks[(uint32_t)chunkFoundIndex];
		if ((chunkFound.state & ChunkVisible) == 0)
		{
			chunkFoundIndex = chunkFound.visibleAncestorIndex;
		}
	}

	return (int32_t)(chunkFoundIndex - getFirstChunkIndex());
}

void DestructibleActorImpl::applyDamage_immediate(DamageEvent& damageEvent)
{
	PX_PROFILE_ZONE("DestructibleApplyDamage_immediate", GetInternalApexSDK()->getContextId());

	SCOPED_PHYSX_LOCK_READ(mDestructibleScene->mApexScene);

	const float paddingFactor = 0.01f;	// To do - expose ?  This is necessary now that we're using exact bounds testing.
	const float padding = paddingFactor * (getOriginalBounds().maximum - getOriginalBounds().minimum).magnitude();

	const uint32_t rayCastFlags = mDestructibleScene->m_damageApplicationRaycastFlags & DestructibleActorRaycastFlags::AllChunks;	// Mask against chunk flags

	float time = 0;
	PxVec3 fractureNormal(0.0f, 0.0f, 1.0f);

	int32_t chunkIndexInAsset = damageEvent.chunkIndexInAsset >= 0 ? damageEvent.chunkIndexInAsset : ModuleDestructibleConst::INVALID_CHUNK_INDEX;

	if (rayCastFlags != 0)
	{
		const PxVec3 worldRayOrig = damageEvent.position.isFinite() ? damageEvent.position : PxVec3(0.0f);
		const PxVec3 worldRayDir = damageEvent.direction.isFinite() && !damageEvent.direction.isZero() ? damageEvent.direction : PxVec3(0.0f, 0.0f, 1.0f);
		// TODO, even the direction isn't always normalized - PxVec3 fractureNormal(-worldRay.dir);
		const int32_t actualHitChunk = rayCast(time, fractureNormal, worldRayOrig, worldRayDir, (DestructibleActorRaycastFlags::Enum)rayCastFlags, chunkIndexInAsset);
		if (actualHitChunk != ModuleDestructibleConst::INVALID_CHUNK_INDEX)
		{
			chunkIndexInAsset = actualHitChunk;
		}
	}
	else
	if (mDestructibleParameters.fractureImpulseScale != 0.0f)
	{
		// Unfortunately, we need to do *some* kind of bounds check to get a good outward-pointing normal, if mDestructibleParameters.fractureImpulseScale != 0
		// We'll make this as inexpensive as possible, and raycast against the depth 0 chunk
		PxTransform tm(getInitialGlobalPose());
		const PxVec3 pos = damageEvent.position.isFinite() ? damageEvent.position : PxVec3(0.0f);
		const PxVec3 dir = damageEvent.direction.isFinite() ? damageEvent.direction : PxVec3(0.0f, 0.0f, 1.0f);
		for (uint32_t hullIndex = mAsset->getChunkHullIndexStart(0); hullIndex < mAsset->getChunkHullIndexStop(0); ++hullIndex)
		{
			ConvexHullImpl& chunkSourceConvexHull = mAsset->chunkConvexHulls[hullIndex];
			float in = -PX_MAX_F32;
			float out = PX_MAX_F32;
			float minTime = PX_MAX_F32;
			PxVec3 n;
			if (chunkSourceConvexHull.rayCast(in, out, pos, dir, tm, getScale(), &n))
			{
				if (in < minTime)
				{
					minTime = in;
					fractureNormal = n;
				}
			}
		}
	}

	PX_ASSERT(fractureNormal.isNormalized());

	if (chunkIndexInAsset == ModuleDestructibleConst::INVALID_CHUNK_INDEX)
	{
		PX_ASSERT(0 == (DamageEvent::UseRadius & damageEvent.flags));
		damageEvent.flags |= DamageEvent::Invalid;
		return;
	}

	damageEvent.chunkIndexInAsset = chunkIndexInAsset;
	damageEvent.position += damageEvent.direction * time;

	float damage = damageEvent.damage;
	if (mDestructibleParameters.damageCap > 0)
	{
		damage = PxMin(damage, mDestructibleParameters.damageCap);
	}

	DestructibleStructure::Chunk& chunk = mStructure->chunks[damageEvent.chunkIndexInAsset + mFirstChunkIndex];
	if (!(chunk.state & ChunkVisible))
	{
		return;
	}

	// For probabilistic chunk deletion
	uint32_t possibleDeleteChunks = 0;
	float totalDeleteChunkRelativeDamage = 0.0f;

	uint32_t totalFractureCount = 0;
	PxRigidDynamic& actor = *mStructure->getChunkActor(chunk);
	for (uint32_t i = 0; i < actor.getNbShapes(); ++i)
	{
		PxShape* shape;
		actor.getShapes(&shape, 1, i);
		DestructibleStructure::Chunk* chunk = mStructure->dscene->getChunk(shape);
		if (chunk != NULL && chunk->isFirstShape(shape))	// BRG OPTIMIZE
		{
			PX_ASSERT(mStructure->dscene->mDestructibles.direct(chunk->destructibleID)->mStructure == mStructure);
			if ((chunk->state & ChunkVisible) != 0)
			{
				// Damage coloring:
				DestructibleActorImpl* destructible = mStructure->dscene->mDestructibles.direct(chunk->destructibleID);
				if (destructible->useDamageColoring())
				{
					if (destructible->applyDamageColoring(chunk->indexInAsset, damageEvent.position, damage, 0.0f))
					{
						destructible->collectDamageColoring(chunk->indexInAsset, damageEvent.position, damage, 0.0f);
					}
				}

				// specify the destructible asset because we're in a structure, it may not be this actor's asset
				DestructibleAssetParametersNS::Chunk_Type& source = destructible->getDestructibleAsset()->mParams->chunks.buf[chunk->indexInAsset];

				const uint16_t stopDepth = destructible->getParams()->destructibleParameters.damageDepthLimit < uint16_t(UINT16_MAX - source.depth) ?
					uint16_t(destructible->getParams()->destructibleParameters.damageDepthLimit + source.depth) : static_cast<uint16_t>(UINT16_MAX);

				totalFractureCount += mStructure->damageChunk(*chunk, damageEvent.position, damageEvent.direction, damageEvent.isFromImpact(), damage, 0.0f,
									  damageEvent.fractures, possibleDeleteChunks, totalDeleteChunkRelativeDamage, damageEvent.maxDepth, (uint32_t)source.depth, stopDepth, padding);
			}
		}
	}

	// For probabilistic chunk deletion
	float deletionFactor = getDestructibleParameters().debrisDestructionProbability;
	if (totalDeleteChunkRelativeDamage > 0.0f)
	{
		deletionFactor *= (float)possibleDeleteChunks/totalDeleteChunkRelativeDamage;
	}

	for (uint32_t depth = 0; depth <= damageEvent.maxDepth; ++depth)
	{
		physx::Array<FractureEvent>& fractureEventBuffer = damageEvent.fractures[depth];
		for (uint32_t i = 0; i < fractureEventBuffer.size(); ++i)
		{
			FractureEvent& fractureEvent = fractureEventBuffer[i];
			DestructibleActorImpl* destructible = mStructure->dscene->mDestructibles.direct(fractureEvent.destructibleID);
			uint32_t affectedIndex = fractureEvent.chunkIndexInAsset + destructible->mFirstChunkIndex;
			DestructibleStructure::Chunk& affectedChunk = mStructure->chunks[affectedIndex];
			if (!affectedChunk.isDestroyed())
			{
				const float deletionProbability = deletionFactor*fractureEvent.deletionWeight;
				if (deletionProbability > 0.0f && mStructure->dscene->mModule->mRandom.getUnit() < deletionProbability)
				{
					fractureEvent.flags |= FractureEvent::CrumbleChunk;
				}

				// Compute impulse
				fractureEvent.impulse = mStructure->getChunkWorldCentroid(affectedChunk) - damageEvent.position;
				fractureEvent.impulse.normalize();
				fractureEvent.impulse *= fractureEvent.damageFraction * damageEvent.momentum;
				fractureEvent.impulse += fractureNormal * mDestructibleParameters.fractureImpulseScale;
				fractureEvent.position = damageEvent.position;
				if (damageEvent.isFromImpact())
				{
					fractureEvent.flags |= FractureEvent::DamageFromImpact;
				}
				if (0 != (DamageEvent::SyncDirect & damageEvent.flags))
				{
					PX_ASSERT(0 == (FractureEvent::SyncDerived & fractureEvent.flags));
					fractureEvent.flags |= FractureEvent::SyncDerived;
				}
			}
		}
	}
}

void DestructibleActorImpl::applyRadiusDamage_immediate(DamageEvent& damageEvent)
{
	PX_PROFILE_ZONE("DestructibleApplyRadiusDamage_immediate", GetInternalApexSDK()->getContextId());

	SCOPED_PHYSX_LOCK_READ(mDestructibleScene->mApexScene);

	const float damageCap = mDestructibleParameters.damageCap > 0 ? mDestructibleParameters.damageCap : PX_MAX_F32;

	uint32_t totalFractureCount = 0;

	const float paddingFactor = 0.01f;	// To do - expose ?  This is necessary now that we're using exact bounds testing.
	const float padding = paddingFactor * (getOriginalBounds().maximum - getOriginalBounds().minimum).magnitude();

	// For probabilistic chunk deletion
	uint32_t possibleDeleteChunks = 0;
	float totalDeleteChunkRelativeDamage = 0.0f;

	const bool useLegacyDamageRadiusSpread = getUseLegacyDamageRadiusSpread();

	// Should use scene query here
	const uint16_t* chunkIndexPtr = mVisibleChunks.usedIndices();
	const uint16_t* chunkIndexPtrStop = chunkIndexPtr + mVisibleChunks.usedCount();
	bool needSaveDamageColor = false;
	while (chunkIndexPtr < chunkIndexPtrStop)
	{
		uint16_t chunkIndex = *chunkIndexPtr++;
		DestructibleStructure::Chunk& chunk = mStructure->chunks[chunkIndex + mFirstChunkIndex];

		// Legacy behavior
		float minRadius = 0.0f;
		float maxRadius = damageEvent.radius;
		float falloff = 1.0f;
		const DestructibleActorParamNS::BehaviorGroup_Type& behaviorGroup = getBehaviorGroup(chunkIndex);
		if (!useLegacyDamageRadiusSpread)
		{
			// New behavior
			minRadius = behaviorGroup.damageSpread.minimumRadius;
			maxRadius = minRadius + damageEvent.radius*behaviorGroup.damageSpread.radiusMultiplier;
			falloff = behaviorGroup.damageSpread.falloffExponent;
		}

		// Damage coloring:
		if (useDamageColoring())
		{
			if (applyDamageColoring(chunk.indexInAsset, damageEvent.position, damageEvent.damage, damageEvent.radius))
			{
				needSaveDamageColor = true;
			}
		}

		const PxVec3 chunkCentroid = mStructure->getChunkWorldCentroid(chunk);
		PxVec3 dir = chunkCentroid - damageEvent.position;
		float dist = dir.normalize() - chunk.localSphereRadius;
		if (dist < maxRadius)
		{
			float damageFraction = 1;
			if (useLegacyDamageRadiusSpread)
			{
				dist = PxMax(dist, 0.0f);
				if (falloff && damageEvent.radius > 0.0f)
				{
					damageFraction -= dist / damageEvent.radius;
				}
			}
			const float effectiveDamage = PxMin(damageEvent.damage * damageFraction, damageCap);
			DestructibleAssetParametersNS::Chunk_Type& source = mAsset->mParams->chunks.buf[chunk.indexInAsset];

			const uint16_t stopDepth = getParams()->destructibleParameters.damageDepthLimit < uint16_t(UINT16_MAX - source.depth) ?
				uint16_t(getParams()->destructibleParameters.damageDepthLimit + source.depth) : uint16_t(UINT16_MAX);

			totalFractureCount += mStructure->damageChunk(chunk, damageEvent.position, damageEvent.direction, damageEvent.isFromImpact(), effectiveDamage, damageEvent.radius,
			                      damageEvent.fractures, possibleDeleteChunks, totalDeleteChunkRelativeDamage, damageEvent.maxDepth, (uint32_t)source.depth, stopDepth, padding);
		}
	}

	if (needSaveDamageColor)
	{
		collectDamageColoring(ModuleDestructibleConst::INVALID_CHUNK_INDEX, damageEvent.position, damageEvent.damage, damageEvent.radius);
	}

	// For probabilistic chunk deletion
	float deletionFactor = getDestructibleParameters().debrisDestructionProbability;
	if (totalDeleteChunkRelativeDamage > 0.0f)
	{
		deletionFactor *= (float)possibleDeleteChunks/totalDeleteChunkRelativeDamage;
	}

	for (uint32_t depth = 0; depth <= damageEvent.maxDepth; ++depth)
	{
		physx::Array<FractureEvent>& fractureEventBuffer = damageEvent.fractures[depth];
		for (uint32_t i = 0; i < fractureEventBuffer.size(); ++i)
		{
			FractureEvent& fractureEvent = fractureEventBuffer[i];
			DestructibleActorImpl* destructible = mStructure->dscene->mDestructibles.direct(fractureEvent.destructibleID);
			uint32_t affectedIndex = fractureEvent.chunkIndexInAsset + destructible->mFirstChunkIndex;
			DestructibleStructure::Chunk& affectedChunk = mStructure->chunks[affectedIndex];
			if (!affectedChunk.isDestroyed())
			{
				const float deletionProbability = deletionFactor*fractureEvent.deletionWeight;
				if (deletionProbability > 0.0f && mStructure->dscene->mModule->mRandom.getUnit() < deletionProbability)
				{
					fractureEvent.flags |= FractureEvent::CrumbleChunk;
				}

				fractureEvent.impulse = mStructure->getChunkWorldCentroid(affectedChunk) - damageEvent.position;
				fractureEvent.impulse.normalize();
				fractureEvent.impulse *= fractureEvent.damageFraction * damageEvent.momentum;

				// Get outward normal for fractureImpulseScale, if this chunk is part of a larger island
				PxVec3 fractureNormal(0.0f);
				if (!mStructure->chunkIsSolitary(affectedChunk))
				{
					PxRigidDynamic& chunkActor = *mStructure->getChunkActor(affectedChunk);
					// Search neighbors
					DestructibleActorImpl* destructible = mStructure->dscene->mDestructibles.direct(affectedChunk.destructibleID);
					const PxVec3 chunkPos = destructible->getChunkPose(affectedChunk.indexInAsset).p.multiply(destructible->getScale());
					const uint32_t indexInStructure = affectedChunk.indexInAsset + destructible->getFirstChunkIndex();
					for (uint32_t overlapN = mStructure->firstOverlapIndices[indexInStructure]; overlapN < mStructure->firstOverlapIndices[indexInStructure + 1]; ++overlapN)
					{
						DestructibleStructure::Chunk& overlapChunk = mStructure->chunks[mStructure->overlaps[overlapN]];
						if (!overlapChunk.isDestroyed() && mStructure->getChunkActor(overlapChunk) == &chunkActor)
						{
							DestructibleActorImpl* overlapDestructible = mStructure->dscene->mDestructibles.direct(overlapChunk.destructibleID);
							fractureNormal += chunkPos - overlapDestructible->getChunkPose(overlapChunk.indexInAsset).p.multiply(overlapDestructible->getScale());
						}
					}
					if (fractureNormal.magnitudeSquared() != 0.0f)
					{
						fractureNormal.normalize();
						fractureEvent.impulse += fractureNormal * mDestructibleParameters.fractureImpulseScale;
					}
				}

				fractureEvent.position = damageEvent.position;
				if (damageEvent.isFromImpact())
				{
					fractureEvent.flags |= FractureEvent::DamageFromImpact;
				}
				if (0 != (DamageEvent::SyncDirect & damageEvent.flags))
				{
					PX_ASSERT(0 == (FractureEvent::SyncDerived & fractureEvent.flags));
					fractureEvent.flags |= FractureEvent::SyncDerived;
				}
			}
		}
	}
}

void DestructibleActorImpl::setSkinnedOverrideMaterial(uint32_t index, const char* overrideMaterialName)
{
	RenderMeshActor* rma = getRenderMeshActor(DestructibleActorMeshType::Skinned);
	if (rma != NULL)
	{
		rma->setOverrideMaterial(index, overrideMaterialName);
	}
}

void DestructibleActorImpl::setStaticOverrideMaterial(uint32_t index, const char* overrideMaterialName)
{
	RenderMeshActor* rma = getRenderMeshActor(DestructibleActorMeshType::Static);
	if (rma != NULL)
	{
		rma->setOverrideMaterial(index, overrideMaterialName);
	}
}

void DestructibleActorImpl::setRuntimeFracturePattern(const char* /*fracturePatternName*/)
{
	// TODO: Implement
	/*
	ResourceProviderIntl* nrp = GetInternalApexSDK()->getInternalResourceProvider();
	if (nrp != NULL)
	{
		// do create before release, so we don't release the resource if the newID is the same as the old
		ResID patternNS = GetInternalApexSDK()->getPatternNamespace();

		ResID newID = nrp->createResource(patternNS, fracturePatternName);
		nrp->releaseResource(mFracturePatternID);

		mFracturePatternID = newID;
		// TODO: Aquire resource for fracture pattern
	}
	*/
}

void DestructibleActorImpl::updateRenderResources(bool rewriteBuffers, void* userRenderData)
{

	PX_PROFILE_ZONE("DestructibleUpdateRenderResources", GetInternalApexSDK()->getContextId());

	//Should not be necessary anymore
	//#if APEX_RUNTIME_FRACTURE
	//	{
	//		mRTActor->updateRenderResources(rewriteBuffers, userRenderData);
	//	}
	//#endif

	// Render non-instanced meshes if we own them
	if (mRenderable != NULL && mRenderable->getReferenceCount() == 1)
	{
		mRenderable->updateRenderResources(rewriteBuffers, userRenderData);
	}
}

void DestructibleActorImpl::dispatchRenderResources(UserRenderer& renderer)
{
	PX_PROFILE_ZONE("DestructibleDispatchRenderResources", GetInternalApexSDK()->getContextId());

	// Dispatch non-instanced render resources if we own them
	if (mRenderable != NULL && mRenderable->getReferenceCount() == 1)
	{
		mRenderable->dispatchRenderResources(renderer);
	}

	// Should not be necessary anymore
	//#if APEX_RUNTIME_FRACTURE
	//	mRTActor->dispatchRenderResources(renderer);
	//#endif
}

void DestructibleActorImpl::fillInstanceBuffers()
{
	if (mAsset->mParams->chunkInstanceInfo.arraySizes[0] == 0 && mAsset->mParams->scatterMeshIndices.arraySizes[0] == 0)
	{
		return;	// No instancing for this asset
	}

	PX_PROFILE_ZONE("DestructibleActor::fillInstanceBuffers", GetInternalApexSDK()->getContextId());

	mInstancedBounds.setEmpty();

	DestructibleAssetParametersNS::Chunk_Type* sourceChunks = mAsset->mParams->chunks.buf;

	Mutex::ScopedLock scopeLock(mAsset->m_chunkInstanceBufferDataLock);

	const float scatterAlpha = PxClamp(2.0f-getLOD(), 0.0f, 1.0f);
    const bool alwaysDrawScatterMesh = mAsset->getScatterMeshAlwaysDrawFlag();
	// Iterate over all visible chunks
	const uint16_t* indexPtr = mVisibleChunks.usedIndices();
	const uint16_t* indexPtrStop = indexPtr + mVisibleChunks.usedCount();
	DestructibleAssetParametersNS::InstanceInfo_Type* instanceDataArray = mAsset->mParams->chunkInstanceInfo.buf;
	while (indexPtr < indexPtrStop)
	{
		const uint16_t index = *indexPtr++;
		if (index < mAsset->getChunkCount())
		{
			DestructibleAssetParametersNS::Chunk_Type& sourceChunk = sourceChunks[index];
			PxMat44 pose(getChunkPose(index));
			const PxMat33 poseScaledRotation = PxMat33(getScale().x*pose.getBasis(0), getScale().y*pose.getBasis(1), getScale().z*pose.getBasis(2));

			// Instanced chunks
			if ((sourceChunk.flags & DestructibleAssetImpl::Instanced) != 0)
			{
				PX_ASSERT(sourceChunk.meshPartIndex < mAsset->mParams->chunkInstanceInfo.arraySizes[0]);
				DestructibleAssetImpl::ChunkInstanceBufferDataElement instanceDataElement;
				const DestructibleAssetParametersNS::InstanceInfo_Type& instanceData = instanceDataArray[sourceChunk.meshPartIndex];
				const uint16_t instancedActorIndex = mAsset->m_instancedChunkActorMap[sourceChunk.meshPartIndex];
				Array<DestructibleAssetImpl::ChunkInstanceBufferDataElement>& instanceBufferData = mAsset->m_chunkInstanceBufferData[instancedActorIndex];
				instanceDataElement.translation = pose.getPosition();// + poseScaledRotation*instanceData.chunkPositionOffset;
				instanceDataElement.scaledRotation = poseScaledRotation;
				instanceDataElement.uvOffset = instanceData.chunkUVOffset;
				instanceDataElement.localOffset = PxVec3(getScale().x*instanceData.chunkPositionOffset.x, getScale().y*instanceData.chunkPositionOffset.y, getScale().z*instanceData.chunkPositionOffset.z);
				
				 // there shouldn't be any allocation here because of the reserve in DestructibleAsset::resetInstanceData
				PX_ASSERT(instanceBufferData.size() < instanceBufferData.capacity());
				instanceBufferData.pushBack(instanceDataElement);

				const PxBounds3& partBounds = mAsset->renderMeshAsset->getBounds(instanceData.partIndex);
				// Transform bounds
				PxVec3 center, extents;
				center = partBounds.getCenter();
				extents = partBounds.getExtents();
				center = poseScaledRotation.transform(center) + instanceDataElement.translation;
				extents = PxVec3(PxAbs(poseScaledRotation(0, 0) * extents.x) + PxAbs(poseScaledRotation(0, 1) * extents.y) + PxAbs(poseScaledRotation(0, 2) * extents.z),
										PxAbs(poseScaledRotation(1, 0) * extents.x) + PxAbs(poseScaledRotation(1, 1) * extents.y) + PxAbs(poseScaledRotation(1, 2) * extents.z),
										PxAbs(poseScaledRotation(2, 0) * extents.x) + PxAbs(poseScaledRotation(2, 1) * extents.y) + PxAbs(poseScaledRotation(2, 2) * extents.z));
				mInstancedBounds.include(PxBounds3::centerExtents(center, extents));
			}

			// Scatter meshes
		    if (scatterAlpha > 0.0f || (sourceChunk.depth == 1 || alwaysDrawScatterMesh))
			{
				const uint16_t scatterMeshStop = uint16_t(sourceChunk.firstScatterMesh + sourceChunk.scatterMeshCount);
				for (uint16_t scatterMeshNum = sourceChunk.firstScatterMesh; scatterMeshNum < scatterMeshStop; ++scatterMeshNum)
				{
					const physx::PxU8 scatterMeshIndex = mAsset->mParams->scatterMeshIndices.buf[scatterMeshNum];
					const PxMat44& scatterMeshTransform = mAsset->mParams->scatterMeshTransforms.buf[scatterMeshNum];
					DestructibleAssetImpl::ScatterMeshInstanceInfo& scatterMeshInstanceInfo = mAsset->m_scatterMeshInstanceInfo[scatterMeshIndex];
					Array<DestructibleAssetImpl::ScatterInstanceBufferDataElement>& instanceBufferData = scatterMeshInstanceInfo.m_instanceBufferData;
					DestructibleAssetImpl::ScatterInstanceBufferDataElement instanceDataElement;
					instanceDataElement.translation = poseScaledRotation*scatterMeshTransform.getPosition() + pose.getPosition();// + poseScaledRotation*instanceData.chunkPositionOffset;
					PxMat33 smTm33(scatterMeshTransform.column0.getXYZ(),
									scatterMeshTransform.column1.getXYZ(),
									scatterMeshTransform.column2.getXYZ());
					instanceDataElement.scaledRotation = poseScaledRotation*smTm33;
					instanceDataElement.alpha = scatterAlpha;
					instanceBufferData.pushBack(instanceDataElement);
					// Not updating bounds for scatter meshes
				}
			}
		}
	}

	mRenderBounds = mNonInstancedBounds;
	mRenderBounds.include(mInstancedBounds);
	if (mRenderable != NULL)
	{
		mRenderable->setBounds(mRenderBounds);
	}
}

void DestructibleActorImpl::setRelativeTMs()
{
	mRelTM = NULL != mStructure ? mTM*mStructure->getActorForStaticChunksPose().inverseRT() : PxMat44(PxIdentity);
}

/* Called during ApexScene::fetchResults(), after PhysXScene::fetchResults().  Sends new bone poses
 * to ApexRenderMeshActor, then retrieves new world AABB.
 */

void DestructibleActorImpl::setRenderTMs(bool processChunkPoseForSyncing /*= false*/)
{
	mNonInstancedBounds.setEmpty();

	if (mRenderable)
	{
		mRenderable->lockRenderResources();
	}

	RenderMeshActor* skinnedRMA = getRenderMeshActor(DestructibleActorMeshType::Skinned);

	const bool syncWriteTM = processChunkPoseForSyncing && mSyncParams.isSyncFlagSet(DestructibleActorSyncFlags::CopyChunkTransform) && (NULL != mSyncParams.getChunkSyncState());
	const bool canSyncReadTM = processChunkPoseForSyncing && mSyncParams.isSyncFlagSet(DestructibleActorSyncFlags::ReadChunkTransform);

	if (skinnedRMA != NULL || syncWriteTM || canSyncReadTM)
	{
		if (skinnedRMA != NULL)
		{
			skinnedRMA->syncVisibility(false);	// Using mRenderable->mRenderable->lockRenderResources() instead
		}
		//	PX_ASSERT(asset->getRenderMeshAsset()->getPartCount() == asset->getChunkCount());

		DestructibleAssetParametersNS::Chunk_Type* sourceChunks = mAsset->mParams->chunks.buf;

		// Iterate over all visible chunks
		const uint16_t* indexPtr = mVisibleChunks.usedIndices();
		const uint16_t* indexPtrStop = indexPtr + mVisibleChunks.usedCount();

		// TODO: Here we update all chunks although we practically know the chunks (active transforms) that have moved. Improve. [APEX-670]

		while (indexPtr < indexPtrStop)
		{
			const uint16_t index = *indexPtr++;
			if (index < mAsset->getChunkCount())
			{
				DestructibleAssetParametersNS::Chunk_Type& sourceChunk = sourceChunks[index];
				DestructibleStructure::Chunk & chunk = mStructure->chunks[mFirstChunkIndex + index];
				if (!chunk.isDestroyed() && (sourceChunk.flags & DestructibleAssetImpl::Instanced) == 0)
				{
					const bool syncReadTM = canSyncReadTM && (NULL != chunk.controlledChunk);
					if (syncReadTM && !getDynamic(index))
					{
						setDynamic(index, true);
					}

					if (getDynamic(index) || !drawStaticChunksInSeparateMesh() || initializedFromState())
					{
						PxTransform chunkPose;
						bool isChunkPoseInit = false;
						if (processChunkPoseForSyncing)
						{
							if (syncWriteTM)
							{
								const DestructibleChunkSyncState & chunkSyncState = *(mSyncParams.getChunkSyncState());
								PX_ASSERT(NULL != &chunkSyncState);
								if(!chunkSyncState.disableTransformBuffering &&
									(chunkSyncState.excludeSleepingChunks ? !isPxActorSleeping(*(mStructure->getChunkActor(chunk))) : true) &&
									(chunkSyncState.chunkTransformCopyDepth >= sourceChunk.depth))
								{
									SCOPED_PHYSX_LOCK_READ(mDestructibleScene->mApexScene);

									const PxTransform calculatedChunkPose = getChunkPose(index);
									mSyncParams.pushCachedChunkTransform(CachedChunk(index, calculatedChunkPose));
									chunkPose = calculatedChunkPose;
									isChunkPoseInit = true;
								}
							}
							if (syncReadTM)
							{
								const PxTransform controlledChunkPose(chunk.controlledChunk->chunkPosition, chunk.controlledChunk->chunkOrientation);
								{
									SCOPED_PHYSX_LOCK_WRITE(getDestructibleScene()->getApexScene());
									setChunkPose(index, controlledChunkPose);
								}
								chunk.controlledChunk = NULL;
								chunkPose = controlledChunkPose;
								isChunkPoseInit = true;
							}
							if (!isChunkPoseInit)
							{
								SCOPED_PHYSX_LOCK_READ(mDestructibleScene->mApexScene);
								chunkPose = getChunkPose(index);
								isChunkPoseInit = true;
							}
						}
						else
						{
							SCOPED_PHYSX_LOCK_READ(mDestructibleScene->mApexScene);
							chunkPose = getChunkPose(index);
							isChunkPoseInit = true;
						}
						PX_ASSERT(isChunkPoseInit);
						if (skinnedRMA != NULL)
						{
							skinnedRMA->setTM(chunkPose, getScale(), sourceChunk.meshPartIndex);
						}
					}
				}
			}
		}


		if (skinnedRMA != NULL)
		{
			skinnedRMA->updateBounds();
			mNonInstancedBounds.include(skinnedRMA->getBounds());
		}
	}

	RenderMeshActor* staticRMA = getRenderMeshActor(DestructibleActorMeshType::Static);

	// If a static mesh exists, set its (single) tm from the destructible's tm
	if (staticRMA != NULL)
	{
		staticRMA->syncVisibility(false);	// Using mRenderable->mRenderable->lockRenderResources() instead
		PxMat44 pose;
		if (!getGlobalPoseForStaticChunks(pose))
		{
			pose = PxMat44(PxIdentity);	// Should not be rendered, but just in case we'll set pose to something sane.
		}
		staticRMA->setTM(pose, getScale());
		staticRMA->updateBounds();
		mNonInstancedBounds.include(staticRMA->getBounds());
	}

#if APEX_RUNTIME_FRACTURE
	if(mRenderable)
	{
		mRenderable->getRTrenderable().updateRenderCache(mRTActor);
	}
#endif

	if (mNonInstancedBounds.isEmpty())	// This can occur if we have no renderable
	{
		const uint16_t* indexPtr = mVisibleChunks.usedIndices();
		const uint16_t* indexPtrStop = indexPtr + mVisibleChunks.usedCount();
		while (indexPtr < indexPtrStop)
		{
			const uint16_t index = *indexPtr++;
			if (index < mAsset->getChunkCount())
			{
				const PxMat44 pose(getChunkPose(index));
				for (uint32_t hullIndex = mAsset->getChunkHullIndexStart(index); hullIndex < mAsset->getChunkHullIndexStop(index); ++hullIndex)
				{
					const ConvexHullImpl& chunkSourceConvexHull = mAsset->chunkConvexHulls[hullIndex];
					PxBounds3 chunkConvexHullBounds = chunkSourceConvexHull.getBounds();
					// Apply scale
					chunkConvexHullBounds.minimum = chunkConvexHullBounds.minimum.multiply(getScale());
					chunkConvexHullBounds.maximum = chunkConvexHullBounds.maximum.multiply(getScale());
					// Apply rotation and translation
					const PxVec3 extent = chunkConvexHullBounds.getExtents();
					const PxVec3 newExtent(
						PxAbs(pose.column0.x*extent.x) + PxAbs(pose.column1.x*extent.y) + PxAbs(pose.column2.x*extent.z),
						PxAbs(pose.column0.y*extent.x) + PxAbs(pose.column1.y*extent.y) + PxAbs(pose.column2.y*extent.z),
						PxAbs(pose.column0.z*extent.x) + PxAbs(pose.column1.z*extent.y) + PxAbs(pose.column2.z*extent.z));
					const PxVec3 center = pose.transform(chunkConvexHullBounds.getCenter());
					mNonInstancedBounds.include(PxBounds3(center - newExtent, center + newExtent));
				}
			}
		}
	}

	mRenderBounds = mNonInstancedBounds;
	mRenderBounds.include(mInstancedBounds);
	if (mRenderable != NULL)
	{
		mRenderable->unlockRenderResources();
		mRenderable->setBounds(mRenderBounds);
	}
}

void DestructibleActorImpl::setActorObjDescFlags(PhysXObjectDescIntl* actorObjDesc, uint32_t depth) const
{
	const DestructibleDepthParameters& depthParameters = mDestructibleParameters.depthParameters[depth];
	actorObjDesc->setIgnoreTransform(depthParameters.ignoresPoseUpdates());
	actorObjDesc->setIgnoreRaycasts(depthParameters.ignoresRaycastCallbacks());
	actorObjDesc->setIgnoreContacts(depthParameters.ignoresContactCallbacks());
	for (uint32_t i = PhysXActorFlags::DEPTH_PARAM_USER_FLAG_0; i <= PhysXActorFlags::DEPTH_PARAM_USER_FLAG_3; ++i)
	{
		actorObjDesc->setUserDefinedFlag(i, depthParameters.hasUserFlagSet(i));
	}
	actorObjDesc->setUserDefinedFlag(PhysXActorFlags::CREATED_THIS_FRAME, true);
	actorObjDesc->setUserDefinedFlag(PhysXActorFlags::IS_SLEEPING, true);
}

void DestructibleActorImpl::setGlobalPose(const PxMat44& pose)
{
	if (!isChunkDestroyed(0) && getDynamic(0))
	{
		setChunkPose(0, PxTransform(pose));
	}
	else
	{
		setGlobalPoseForStaticChunks(pose);
	}
}

void DestructibleActorImpl::setGlobalPoseForStaticChunks(const PxMat44& pose)
{
	if (mStructure != NULL && mStructure->actorForStaticChunks != NULL)
	{
		const PxMat44 actorForStaticChunkPose = mRelTM.inverseRT() * pose;
		{
			SCOPED_PHYSX_LOCK_WRITE(mStructure->actorForStaticChunks->getScene());
			mStructure->actorForStaticChunks->setKinematicTarget(PxTransform(actorForStaticChunkPose));
		}
		for (uint32_t meshType = 0; meshType < DestructibleActorMeshType::Count; ++meshType)
		{
			RenderMeshActor* renderMeshActor = getRenderMeshActor((DestructibleActorMeshType::Enum)meshType);
			if (renderMeshActor != NULL)
			{
				renderMeshActor->updateBounds();
			}
		}

	}

	wakeForEvent();
}

bool DestructibleActorImpl::getGlobalPoseForStaticChunks(PxMat44& pose) const
{
	if (mStructure != NULL && mStructure->actorForStaticChunks != NULL)
	{
		if (NULL != mStructure->actorForStaticChunks)
			pose = mRelTM * mStructure->getActorForStaticChunksPose();
		else
			pose = mTM;
		return true;
	}

	return false;
}

void DestructibleActorImpl::setChunkPose(uint32_t index, PxTransform worldPose)
{
	PX_ASSERT(mStructure != NULL);
	PX_ASSERT(index + mFirstChunkIndex < mStructure->chunks.size());
	PX_ASSERT(!mStructure->chunks[index + mFirstChunkIndex].isDestroyed());
	DestructibleStructure::Chunk & chunk = mStructure->chunks[index + mFirstChunkIndex];
	PX_ASSERT(chunk.state & ChunkDynamic);
	mStructure->setChunkGlobalPose(chunk, worldPose);
}

void DestructibleActorImpl::setLinearVelocity(const PxVec3& linearVelocity)
{
	// Only dynamic actors need their velocity set, and they'll all be in the skinned mesh
	const uint16_t* indexPtr = getVisibleChunks();
	const uint16_t* indexPtrStop = indexPtr + getNumVisibleChunks();
	while (indexPtr < indexPtrStop)
	{
		DestructibleStructure::Chunk& chunk = mStructure->chunks[mFirstChunkIndex + *indexPtr++];
		if (chunk.state & ChunkDynamic)
		{
			PxRigidDynamic* actor = mStructure->getChunkActor(chunk);
			if (actor != NULL)
			{
				SCOPED_PHYSX_LOCK_WRITE(actor->getScene());
				actor->setLinearVelocity(linearVelocity);
			}
		}
	}
}

void DestructibleActorImpl::setAngularVelocity(const PxVec3& angularVelocity)
{
	// Only dynamic actors need their velocity set, and they'll all be in the skinned mesh
	const uint16_t* indexPtr = getVisibleChunks();
	const uint16_t* indexPtrStop = indexPtr + getNumVisibleChunks();
	while (indexPtr < indexPtrStop)
	{
		DestructibleStructure::Chunk& chunk = mStructure->chunks[mFirstChunkIndex + *indexPtr++];
		if (chunk.state & ChunkDynamic)
		{
			PxRigidDynamic* actor = mStructure->getChunkActor(chunk);
			if (actor != NULL)
			{
				SCOPED_PHYSX_LOCK_WRITE(actor->getScene());
				actor->setAngularVelocity(angularVelocity);
			}
		}
	}
}

void DestructibleActorImpl::enableHardSleeping()
{
	mParams->useHardSleeping = true;
}

void DestructibleActorImpl::disableHardSleeping(bool wake)
{
	if (!useHardSleeping())
	{
		return;	// Nothing to do
	}

	if (mStructure == NULL || mStructure->dscene == NULL || mStructure->dscene->mModule == NULL)
	{
		return;	// Can't do anything
	}

	mParams->useHardSleeping = false;

	Bank<DormantActorEntry, uint32_t>& dormantActors = mStructure->dscene->mDormantActors;
	for (uint32_t dormantActorRank = dormantActors.usedCount(); dormantActorRank--;)
	{
		const uint32_t dormantActorIndex = dormantActors.usedIndices()[dormantActorRank];
		DormantActorEntry& dormantActorEntry = dormantActors.direct(dormantActorIndex);
		// Look at every destructible actor which contributes to this physx actor, and see if any use hard sleeping
		bool keepDormant = false;
		PhysXObjectDescIntl* actorObjDesc = (PhysXObjectDescIntl*)mStructure->dscene->mModule->mSdk->getPhysXObjectInfo(dormantActorEntry.actor);
		if (actorObjDesc != NULL)
		{
			for (uint32_t i = 0; i < actorObjDesc->mApexActors.size(); ++i)
			{
				const DestructibleActor* dActor = static_cast<const DestructibleActor*>(actorObjDesc->mApexActors[i]);
				if (dActor != NULL)
				{
					if (actorObjDesc->mApexActors[i]->getOwner()->getObjTypeID() == DestructibleAssetImpl::getAssetTypeID())
					{
						const DestructibleActorImpl& destructibleActor = static_cast<const DestructibleActorProxy*>(dActor)->impl;
						if (destructibleActor.useHardSleeping())
						{
							keepDormant = true;
							break;
						}
					}
				}
			}
		}
		// If none use hard sleeping, we will remove the physx actor from the dormant list
		if (!keepDormant)
		{
			PxRigidDynamic* actor = dormantActorEntry.actor;
			dormantActorEntry.actor = NULL;
			dormantActors.free(dormantActorIndex);
			{
				SCOPED_PHYSX_LOCK_WRITE(actor->getScene());
				actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, false);
			}
			if (actorObjDesc != NULL)
			{
				actorObjDesc->userData = NULL;
				mStructure->dscene->addActor(*actorObjDesc, *actor, dormantActorEntry.unscaledMass,
					((dormantActorEntry.flags & ActorFIFOEntry::IsDebris) != 0));
			}
			// Wake if requested
			if (wake)
			{
				SCOPED_PHYSX_LOCK_WRITE(actor->getScene());
				actor->wakeUp();
			}
		}
	}
}

bool DestructibleActorImpl::setChunkPhysXActorAwakeState(uint32_t chunkIndex, bool awake)
{
	PxRigidDynamic* actor = getChunkActor(chunkIndex);
	if (actor == NULL)
	{
		return false;
	}

	PxScene* scene = actor->getScene();
	if (scene == NULL)
	{
		// defer
		if (mDestructibleScene != NULL)
		{
			mDestructibleScene->addForceToAddActorsMap(actor, ActorForceAtPosition(PxVec3(0.0f), PxVec3(0.0f), physx::PxForceMode::eFORCE, awake, false));
			return true;
		}
		return false;
	}

	// Actor has a scene, set sleep state now
	if (awake)
	{
		actor->wakeUp();
	}
	else
	{
		((PxRigidDynamic*)actor)->putToSleep();
	}

	return true;
}

bool DestructibleActorImpl::addForce(uint32_t chunkIndex, const PxVec3& force, physx::PxForceMode::Enum mode, const PxVec3* position, bool wakeup)
{
	PxRigidDynamic* actor = getChunkActor(chunkIndex);
	if (actor == NULL)
	{
		return false;
	}

	if (actor->getScene() == NULL)
	{
		// defer
		if (mDestructibleScene != NULL)
		{
			if (position)
			{
				mDestructibleScene->addForceToAddActorsMap(actor, ActorForceAtPosition(force, *position, mode, wakeup, true));
			}
			else
			{
				mDestructibleScene->addForceToAddActorsMap(actor, ActorForceAtPosition(force, PxVec3(0.0f), mode, wakeup, false));
			}
			return true;
		}
		return false;
	}

	// Actor has a scene, add force now
	PxRigidBody* rigidBody = actor->is<PxRigidBody>();
	if (rigidBody)
	{
		if (position)
		{
			PxRigidBodyExt::addForceAtPos(*rigidBody, force, *position, mode, wakeup);
		}
		else
		{
			rigidBody->addForce(force, mode, wakeup);
		}
	}

	return true;
}

void DestructibleActorImpl::getLodRange(float& min, float& max, bool& intOnly) const
{
	min = 0.0f;
	max = (float)(PxMax(mAsset->getDepthCount(), (uint32_t)1) - 1);
	intOnly = true;
}

float DestructibleActorImpl::getActiveLod() const
{
	return (float)getLOD();
}

void DestructibleActorImpl::forceLod(float lod)
{
	if (lod < 0.0f)
	{
		mState->lod = PxMax(mAsset->getDepthCount(), (uint32_t)1) - 1;
		mState->forceLod = false;
	}
	else
	{
		float min, max;
		bool intOnly;
		getLodRange(min, max, intOnly);
		mState->lod = (uint32_t)PxClamp(lod, min, max);
		mState->forceLod = true;
	}
}

void DestructibleActorImpl::setDynamic(int32_t chunkIndex, bool immediate)
{
	const uint16_t* indexPtr = NULL;
	const uint16_t* indexPtrStop = NULL;
	uint16_t index;
	if (chunkIndex == ModuleDestructibleConst::INVALID_CHUNK_INDEX)
	{
		indexPtr = getVisibleChunks();
		indexPtrStop = indexPtr + getNumVisibleChunks();
	}
	else
	if (chunkIndex >= 0 && chunkIndex < mAsset->mParams->chunks.arraySizes[0])
	{
		index = (uint16_t)chunkIndex;
		indexPtr = &index;
		indexPtrStop = indexPtr+1;
	}

	while (indexPtr < indexPtrStop)
	{
		const uint16_t index = *indexPtr++;
		DestructibleStructure::Chunk& chunk = mStructure->chunks[mFirstChunkIndex + index];
		if ((chunk.state & ChunkDynamic)==0)
		{
			FractureEvent stackEvent;
			FractureEvent& fractureEvent = immediate ? stackEvent : mStructure->dscene->mFractureBuffer.pushBack();
			fractureEvent.chunkIndexInAsset = chunk.indexInAsset;
			fractureEvent.destructibleID = mID;
			fractureEvent.impulse = PxVec3(0.0f);
			fractureEvent.position = PxVec3(0.0f);
			fractureEvent.flags = FractureEvent::Forced | FractureEvent::Silent;
			if (immediate)
			{
				getStructure()->fractureChunk(fractureEvent);
			}
		}
	}
}

void DestructibleActorImpl::getChunkVisibilities(uint8_t* visibilityArray, uint32_t visibilityArraySize) const
{
	if (visibilityArray == NULL || visibilityArraySize == 0)
	{
		return;
	}

	memset(visibilityArray, 0, visibilityArraySize);

	const uint32_t visiblePartCount = mVisibleChunks.usedCount();
	const uint16_t* visiblePartIndices = mVisibleChunks.usedIndices();
	for (uint32_t i = 0; i < visiblePartCount; ++i)
	{
		const uint32_t index = visiblePartIndices[i];
		if (index < visibilityArraySize)
		{
			visibilityArray[index] = 1;
		}
	}
}

bool DestructibleActorImpl::acquireChunkEventBuffer(const DestructibleChunkEvent*& buffer, uint32_t& bufferSize)
{
	mChunkEventBufferLock.lock();
		
	buffer = mChunkEventBuffer.begin();
	bufferSize = mChunkEventBuffer.size();
	return true;
}

bool DestructibleActorImpl::releaseChunkEventBuffer(bool clearBuffer /* = true */)
{
	if (clearBuffer)
	{
		mChunkEventBuffer.reset();
		// potentially O(n), but is O(1) (empty list) if chunk event callbacks aren't enabled.  The chunk event callbacks
		// and aquireChunkEventBuffer/releaseChunkEventBuffer mechanisms probably won't be used together.
		mDestructibleScene->mActorsWithChunkStateEvents.findAndReplaceWithLast(this);
	}

	mChunkEventBufferLock.unlock();
	return true;
}

// PhysX actor buffer API
bool DestructibleActorImpl::acquirePhysXActorBuffer(physx::PxRigidDynamic**& buffer, uint32_t& bufferSize, uint32_t flags)
{
	mPhysXActorBufferLock.lock();

	mPhysXActorBufferAcquired = true;
	// Clear buffer, just in case
	mPhysXActorBuffer.reset();

	const bool eliminateRedundantActors = (flags & DestructiblePhysXActorQueryFlags::AllowRedundancy) == 0;

	const bool onlyAllowActorsInScene = (flags & DestructiblePhysXActorQueryFlags::AllowActorsNotInScenes) == 0;

	SCOPED_PHYSX_LOCK_READ(mDestructibleScene->mPhysXScene);

	// Fill the actor buffer
	for (uint32_t actorNum = 0; actorNum < mReferencingActors.size(); ++actorNum)
	{
		PxRigidDynamic* actor = mReferencingActors[actorNum];
		if (actor == NULL || actor->getNbShapes() == 0
			// don't return actors that have not yet been added to the scene
			// to prevent errors in actor functions that require a scene
			|| ((actor->getScene() == NULL) && onlyAllowActorsInScene)
		)
		{
			continue;
		}

		PxShape* firstShape;
		actor->getShapes(&firstShape, 1, 0);
		PhysXObjectDescIntl* objDesc = mStructure->dscene->mModule->mSdk->getGenericPhysXObjectInfo(firstShape);
		if (objDesc == NULL)
		{
			continue;
		}
		const DestructibleStructure::Chunk& chunk = *(DestructibleStructure::Chunk*)objDesc->userData;
		const uint32_t actorState = !(actor->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC) ? DestructiblePhysXActorQueryFlags::Dynamic :
			((chunk.state & ChunkDynamic) == 0 ? DestructiblePhysXActorQueryFlags::Static : DestructiblePhysXActorQueryFlags::Dormant);
		if ((actorState & flags) == 0)
		{
			continue;	// Not a type of actor requested
		}
		if (eliminateRedundantActors && chunk.destructibleID != mID)
		{
			continue;	// If eliminateRedundantActors is set, only take actors whose first shape belongs to a chunk from this destructible
		}
		mPhysXActorBuffer.pushBack((physx::PxRigidDynamic*)actor);
	}

	// Return buffer
	buffer = mPhysXActorBuffer.begin();
	bufferSize = mPhysXActorBuffer.size();
	return true;
}

bool DestructibleActorImpl::releasePhysXActorBuffer()
{
	if(mPhysXActorBufferAcquired)
	{
		mPhysXActorBuffer.reset();
		mPhysXActorBufferAcquired = false;
		mPhysXActorBufferLock.unlock();
		return true;
	}
	else
	{	
		return false;
	}
}

bool DestructibleActorImpl::recreateApexEmitter(DestructibleEmitterType::Enum type)
{
	bool ret = false;

	switch (type)
	{
	case DestructibleEmitterType::Crumble:
		if (getCrumbleEmitterName())
		{
			ret = initCrumbleSystem(getCrumbleEmitterName());
		}
		break;
	case DestructibleEmitterType::Dust:
		if (getDustEmitterName())
		{
			ret = initDustSystem(getDustEmitterName());
		}
		break;
	default:
		break;
	}
	return ret;
}

bool DestructibleActorImpl::initDustSystem(const char* name)
{
#if APEX_USE_PARTICLES

	if (mDustEmitter)
	{
		return true;
	}

	// Set up dust system
	if (name)
	{
		setDustEmitterName(name);
		/* This destructible actor will hold a reference to its MeshParticleFactoryAsset */
		Asset* tmpAsset = mAsset->mDustAssetTracker.getAssetFromName(name);
		EmitterAsset* fasset = static_cast<EmitterAsset*>(tmpAsset);
		if (fasset)
		{
			NvParameterized::Interface* descParams = fasset->getDefaultActorDesc();
			PX_ASSERT(descParams);
			if (descParams)
			{
				mDustEmitter = static_cast<EmitterActor*>(fasset->createApexActor(*descParams, *mDestructibleScene->mApexScene));
				if (mDustEmitter)
				{
					ApexActor* aa = mAsset->module->mSdk->getApexActor(mDustEmitter);
					if (aa)
					{
						aa->addSelfToContext(*this);
					}
					if (!mDustEmitter->isExplicitGeom())
					{
						APEX_INTERNAL_ERROR("Destructible actors need EmitterGeomExplicit emitters.");
					}
					if (mDustRenderVolume)
					{
						mDustEmitter->setPreferredRenderVolume(mDustRenderVolume);
					}
					mDustEmitter->startEmit(true);
				}
			}
		}
	}
	else
	{
		mState->enableDustEmitter = false;
	}
#else
	PX_UNUSED(name);
#endif // APEX_USE_PARTICLES

	return mDustEmitter ? true : false;
}

bool DestructibleActorImpl::initCrumbleSystem(const char* name)
{
#if APEX_USE_PARTICLES
	// Set up crumble system
	if (mCrumbleEmitter)
	{
		return true;
	}
	if (name)
	{
		setCrumbleEmitterName(name);
		Asset* tmpAsset = mAsset->mCrumbleAssetTracker.getAssetFromName(name);
		EmitterAsset* fasset = static_cast<EmitterAsset*>(tmpAsset);
		if (fasset)
		{
			NvParameterized::Interface* descParams = fasset->getDefaultActorDesc();
			PX_ASSERT(descParams);
			if (descParams)
			{
				mCrumbleEmitter = static_cast<EmitterActor*>(fasset->createApexActor(*descParams, *mDestructibleScene->mApexScene));
				if (mCrumbleEmitter)
				{
					ApexActor* aa = mAsset->module->mSdk->getApexActor(mCrumbleEmitter);
					if (aa)
					{
						aa->addSelfToContext(*this);
					}
					if (!mCrumbleEmitter->isExplicitGeom())
					{
						APEX_INTERNAL_ERROR("Destructible actors need EmitterGeomExplicit emitters.");
					}
					if (mCrumbleRenderVolume)
					{
						mCrumbleEmitter->setPreferredRenderVolume(mCrumbleRenderVolume);
					}
					mCrumbleEmitter->startEmit(true);
				}
			}
		}
	}
	else
	{
		mState->enableCrumbleEmitter = false;
	}
#else
	PX_UNUSED(name);
#endif // APEX_USE_PARTICLES

	return mCrumbleEmitter ? true : false;
}

void DestructibleActorImpl::setCrumbleEmitterName(const char* name)
{
	// Avoid self-assignment
	if (name == getCrumbleEmitterName())
		return;
	NvParameterized::Handle handle(*mParams);
	mParams->getParameterHandle("crumbleEmitterName", handle);
	mParams->setParamString(handle, name ? name : "");
}

const char* DestructibleActorImpl::getCrumbleEmitterName() const
{
	const char* name = (const char*)mParams->crumbleEmitterName;
	return (name && *name) ? name : NULL;
}

void DestructibleActorImpl::setDustEmitterName(const char* name)
{
	// Avoid self-assignment
	if (name == getDustEmitterName())
		return;
	NvParameterized::Handle handle(*mParams);
	mParams->getParameterHandle("dustEmitterName", handle);
	mParams->setParamString(handle, name ? name : "");
}

const char* DestructibleActorImpl::getDustEmitterName() const
{
	const char* name = (const char*)mParams->dustEmitterName;
	return (name && *name) ? name : NULL;
}


void DestructibleActorImpl::setPreferredRenderVolume(RenderVolume* volume, DestructibleEmitterType::Enum type)
{
#if APEX_USE_PARTICLES
	switch (type)
	{
	case DestructibleEmitterType::Crumble:
		mCrumbleRenderVolume = volume;
		if (mCrumbleEmitter)
		{
			mCrumbleEmitter->setPreferredRenderVolume(volume);
		}
		break;
	case DestructibleEmitterType::Dust:
		mDustRenderVolume = volume;
		if (mDustEmitter)
		{
			mDustEmitter->setPreferredRenderVolume(volume);
		}
		break;
	default:
		break;
	}
#else
	PX_UNUSED(volume);
	PX_UNUSED(type);
#endif // APEX_USE_PARTICLES
}

EmitterActor* DestructibleActorImpl::getApexEmitter(DestructibleEmitterType::Enum type)
{
	EmitterActor* ret = NULL;

#if APEX_USE_PARTICLES
	switch (type)
	{
	case DestructibleEmitterType::Crumble:
		ret = mCrumbleEmitter;
		break;
	case DestructibleEmitterType::Dust:
		ret = mDustEmitter;
		break;
	default:
		break;
	}
#else
	PX_UNUSED(type);
#endif // APEX_USE_PARTICLES

	return ret;
}

void DestructibleActorImpl::preSerialize(void* nvParameterizedType)
{
	DestructibleParameterizedType::Enum nxType = (DestructibleParameterizedType::Enum)((intptr_t)nvParameterizedType);
	switch (nxType)
	{
	case DestructibleParameterizedType::State:
		{
			PX_ASSERT(mState->actorChunks);
			serialize(*this, &getStructure()->chunks[0] + mFirstChunkIndex, getDestructibleAsset()->getChunkCount(), *mChunks);
			break;
		}
	case DestructibleParameterizedType::Params:
		{
			PX_ASSERT(mParams);
			PxMat44 globalPoseForStaticChunks;
			if (!getGlobalPoseForStaticChunks(globalPoseForStaticChunks))
			{
				globalPoseForStaticChunks = getInitialGlobalPose();	// This will occur if there are no static chunks
			}
			PxTransform globalPose(globalPoseForStaticChunks);
			globalPose.q.normalize();
			mParams->globalPose = globalPose;
			PX_ASSERT(mState->actorParameters);
			serialize(mDestructibleParameters, *mState->actorParameters);
			break;
		}
	default:
		PX_ASSERT(0 && "Invalid destructible parameterized type");
		break;
	}
}

bool DestructibleActorImpl::applyDamageColoring(uint16_t indexInAsset, const PxVec3& position, float damage, float damageRadius)
{
	bool bRet = applyDamageColoringRecursive(indexInAsset, position, damage, damageRadius);

	RenderMeshAsset* rma = mAsset->getRenderMeshAsset();
	for (uint32_t submeshIndex = 0; submeshIndex < rma->getSubmeshCount(); ++submeshIndex)
	{
		physx::Array<ColorRGBA>& damageColorArray = mDamageColorArrays[submeshIndex];
		// fix asset on &damageColorArray[0]
		if (damageColorArray.size() != 0)
		{
			for (uint32_t typeN = 0; typeN < DestructibleActorMeshType::Count; ++typeN)
			{
				RenderMeshActorIntl* renderMeshActor = (RenderMeshActorIntl*)getRenderMeshActor((DestructibleActorMeshType::Enum)typeN);
				if (renderMeshActor != NULL)
				{
					renderMeshActor->setStaticColorReplacement(submeshIndex, &damageColorArray[0]);
				}
			}
		}
	}

	return bRet;
}

bool DestructibleActorImpl::applyDamageColoringRecursive(uint16_t indexInAsset, const PxVec3& position, float damage, float damageRadius)
{
	RenderMeshAsset* rma = mAsset->getRenderMeshAsset();
	if (mDamageColorArrays.size() != rma->getSubmeshCount())
	{
		return false;
	}

	// Get behavior group
	const DestructibleAssetParametersNS::Chunk_Type& source = getDestructibleAsset()->mParams->chunks.buf[indexInAsset];
	const DestructibleActorParamNS::BehaviorGroup_Type& behaviorGroup = getBehaviorGroupImp(source.behaviorGroupIndex);

	const float maxRadius = behaviorGroup.damageColorSpread.minimumRadius + damageRadius*behaviorGroup.damageColorSpread.radiusMultiplier;

	const DestructibleStructure::Chunk& chunk = mStructure->chunks[indexInAsset + mFirstChunkIndex];
	const PxVec3 disp = mStructure->getChunkWorldCentroid(chunk) - position;
	const float dist = disp.magnitude();
	if (dist > maxRadius + chunk.localSphereRadius)
	{
		return false;	// Outside of max radius
	}

	bool bRet = false;

//	const float recipRadiusRange = maxRadius > behaviorGroup.damageColorSpread.minimumRadius ? 1.0f/(maxRadius - behaviorGroup.damageColorSpread.minimumRadius) : 0.0f;
	const float recipRadiusRange = maxRadius > damageRadius ? 1.0f/(maxRadius - damageRadius) : 0.0f;

	// Color scale multiplier based upon damage
	const float colorScale = behaviorGroup.damageThreshold > 0.0f ? PxMin(1.0f, damage/behaviorGroup.damageThreshold) : 1.0f;

	const uint32_t partIndex = mAsset->getPartIndex(indexInAsset);

//	const float minRadiusSquared = behaviorGroup.damageColorSpread.minimumRadius*behaviorGroup.damageColorSpread.minimumRadius;
	const float minRadiusSquared = damageRadius*damageRadius;
	const float maxRadiusSquared = maxRadius*maxRadius;

	PxMat44 chunkGlobalPose(getChunkPose(indexInAsset));
	chunkGlobalPose.scale(PxVec4(getScale(), 1.0f));

	// Find all vertices and modify color
	for (uint32_t submeshIndex = 0; submeshIndex < rma->getSubmeshCount(); ++submeshIndex)
	{
		const RenderSubmesh& submesh = rma->getSubmesh(submeshIndex);
		physx::Array<ColorRGBA>& damageColorArray = mDamageColorArrays[submeshIndex];
		const VertexBuffer& vb = submesh.getVertexBuffer();
		PX_ASSERT(damageColorArray.size() == vb.getVertexCount());
		if (damageColorArray.size() != vb.getVertexCount())	// Make sure we have the correct size color array
		{
			continue;
		}
		const VertexFormat& vf = vb.getFormat();
		const int32_t positionBufferIndex = vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::POSITION));
		if (positionBufferIndex >= 0)	// Make sure we have a position buffer
		{
			RenderDataFormat::Enum positionBufferFormat = vf.getBufferFormat((uint32_t)positionBufferIndex);
			PX_ASSERT(positionBufferFormat == RenderDataFormat::FLOAT3);
			if (positionBufferFormat != RenderDataFormat::FLOAT3)
			{
				continue;
			}
			const PxVec3* positions = (const PxVec3*)vb.getBuffer((uint32_t)positionBufferIndex);
			// Get the vertex range for the part associated with this chunk (note: this will _not_ work with instancing)
			const uint32_t firstVertexIndex = submesh.getFirstVertexIndex(partIndex);
			const uint32_t stopVertexIndex = firstVertexIndex + submesh.getVertexCount(partIndex);
			for (uint32_t vertexIndex = firstVertexIndex; vertexIndex < stopVertexIndex; ++vertexIndex)
			{
				// Adjust the color scale based upon the envelope function
				float colorChangeMultiplier = colorScale;
				// Get the world vertex position
				const PxVec3 worldVertexPosition = chunkGlobalPose.transform(positions[vertexIndex]);
				const float radiusSquared = (position - worldVertexPosition).magnitudeSquared();
				if (radiusSquared > maxRadiusSquared)
				{
					continue;
				}
				if (radiusSquared > minRadiusSquared)
				{
					colorChangeMultiplier *= PxPow((maxRadius - PxSqrt(radiusSquared))*recipRadiusRange, behaviorGroup.damageColorSpread.falloffExponent);
				}
				// Get the color, add the scaled color values, clamp, and set the new color
				ColorRGBA& color = damageColorArray[vertexIndex];
				PxVec4 newColor = PxVec4((float)color.r, (float)color.g, (float)color.b, (float)color.a) + colorChangeMultiplier*behaviorGroup.damageColorChange;
				newColor = newColor.maximum(PxVec4(0.0f));
				newColor = newColor.minimum(PxVec4(255.0f));
				// save previous color
				ColorRGBA preColor = color;
				color.r = (uint8_t)(newColor[0] + 0.5f);
				color.g = (uint8_t)(newColor[1] + 0.5f);
				color.b = (uint8_t)(newColor[2] + 0.5f);
				color.a = (uint8_t)(newColor[3] + 0.5f);

				// Only save the static chunk now.
				if ((chunk.state & ChunkDynamic) == 0)
				{
					// compare the previous color with the new color
					bRet = preColor.r != color.r || preColor.g != color.g || preColor.b != color.b || preColor.a != color.a;
				}
			}
		}
	}

	// Recurse to children
	const uint32_t stopIndex = uint32_t(source.firstChildIndex + source.numChildren);
	for (uint32_t childIndex = source.firstChildIndex; childIndex < stopIndex; ++childIndex)
	{
		bRet |= applyDamageColoringRecursive((uint16_t)childIndex, position, damage, damageRadius);
	}

	return bRet;
}

void DestructibleActorImpl::fillBehaviorGroupDesc(DestructibleBehaviorGroupDesc& behaviorGroupDesc, const DestructibleActorParamNS::BehaviorGroup_Type behaviorGroup) const
{
	behaviorGroupDesc.name = behaviorGroup.name;
	behaviorGroupDesc.damageThreshold = behaviorGroup.damageThreshold;
	behaviorGroupDesc.damageToRadius = behaviorGroup.damageToRadius;
	behaviorGroupDesc.damageSpread.minimumRadius = behaviorGroup.damageSpread.minimumRadius;
	behaviorGroupDesc.damageSpread.radiusMultiplier = behaviorGroup.damageSpread.radiusMultiplier;
	behaviorGroupDesc.damageSpread.falloffExponent = behaviorGroup.damageSpread.falloffExponent;
	behaviorGroupDesc.damageColorSpread.minimumRadius = behaviorGroup.damageColorSpread.minimumRadius;
	behaviorGroupDesc.damageColorSpread.radiusMultiplier = behaviorGroup.damageColorSpread.radiusMultiplier;
	behaviorGroupDesc.damageColorSpread.falloffExponent = behaviorGroup.damageColorSpread.falloffExponent;
	behaviorGroupDesc.damageColorChange = behaviorGroup.damageColorChange;
	behaviorGroupDesc.materialStrength = behaviorGroup.materialStrength;
	behaviorGroupDesc.density = behaviorGroup.density;
	behaviorGroupDesc.fadeOut = behaviorGroup.fadeOut;
	behaviorGroupDesc.maxDepenetrationVelocity = behaviorGroup.maxDepenetrationVelocity;
	behaviorGroupDesc.userData = behaviorGroup.userData;
}

void DestructibleActorImpl::spawnParticles(EmitterActor* emitter, UserChunkParticleReport* report, DestructibleStructure::Chunk& chunk, physx::Array<PxVec3>& positions, bool deriveVelocitiesFromChunk, const PxVec3* overrideVelocity)
{
#if APEX_USE_PARTICLES
	uint32_t numParticles = positions.size();
	if (numParticles == 0)
	{
		return;
	}

	EmitterGeomExplicit* geom = emitter != NULL ? emitter->isExplicitGeom() : NULL;	// emitter may be NULL, since we may have a particle callback

	if (geom == NULL && report == NULL)
	{
		APEX_INTERNAL_ERROR("DestructibleActor::spawnParticles requires an EmitterGeomExplicit emitter or a UserChunkParticleReport, or both.");
		return;
	}

	if (overrideVelocity == NULL && deriveVelocitiesFromChunk)
	{
		physx::Array<PxVec3> volumeFillVel;
		volumeFillVel.resize(numParticles);

		// Use dynamic actor's current velocity
		PxVec3 angVel;
		PxVec3 linVel;
		PxVec3 COM;
		PxRigidDynamic* actor = mStructure->getChunkActor(chunk);
		{
			SCOPED_PHYSX_LOCK_READ(actor->getScene());
			angVel = actor->getAngularVelocity();
			linVel = actor->getLinearVelocity();
			COM = actor->getGlobalPose().p;
		}

		for (uint32_t i = 0; i < numParticles; ++i)
		{
			volumeFillVel[i] = linVel + angVel.cross(positions[i] - COM);
			PX_ASSERT(PxIsFinite(volumeFillVel[i].magnitude()));
		}

		if (geom != NULL)
		{
			geom->addParticleList(numParticles, &positions.front(), &volumeFillVel.front());
			//emitter->emit(false);
		}

		if (report != NULL)
		{
			ChunkParticleReportData reportData;
			reportData.positions = &positions.front();
			reportData.positionCount = numParticles;
			reportData.velocities = &volumeFillVel.front();
			reportData.velocityCount = numParticles;
			report->onParticleEmission(reportData);
		}
	}
	else
	{
		PX_ASSERT(overrideVelocity == NULL || PxIsFinite(overrideVelocity->magnitude()));

		if (geom != NULL)
		{
			const PxVec3 velocity = overrideVelocity != NULL ? *overrideVelocity : PxVec3(0.0f);
			emitter->setVelocityLow(velocity);
			emitter->setVelocityHigh(velocity);
			geom->addParticleList(numParticles, &positions.front());
		}

		if (report != NULL)
		{
			ChunkParticleReportData reportData;
			reportData.positions = &positions.front();
			reportData.positionCount = numParticles;
			reportData.velocities = overrideVelocity;
			reportData.velocityCount = overrideVelocity != NULL ? 1u : 0u;
			report->onParticleEmission(reportData);
		}
	}
#else
	PX_UNUSED(emitter);
	PX_UNUSED(report);
	PX_UNUSED(chunk);
	PX_UNUSED(positions);
	PX_UNUSED(deriveVelocitiesFromChunk);
	PX_UNUSED(overrideVelocity);
#endif // APEX_USE_PARTICLES
}

void DestructibleActorImpl::setDeleteFracturedChunks(bool inDeleteChunkMode)
{
	mInDeleteChunkMode = inDeleteChunkMode;
}

bool DestructibleActorImpl::setHitChunkTrackingParams(bool flushHistory, bool startTracking, uint32_t trackingDepth, bool trackAllChunks)
{
	bool validOperation = false;
	if(getDestructibleAsset()->getDepthCount() >= trackingDepth)
	{
		hitChunkParams.cacheChunkHits = startTracking;
		hitChunkParams.cacheAllChunks = trackAllChunks;
		if(flushHistory && !hitChunkParams.hitChunkContainer.empty())
		{
			hitChunkParams.hitChunkContainer.clear();
			damageColoringParams.damageEventCoreDataContainer.clear();
		}
		hitChunkParams.trackingDepth = trackingDepth;
		validOperation = true;
	}
	return validOperation;
}

bool DestructibleActorImpl::getHitChunkHistory(const DestructibleHitChunk *& hitChunkContainer, uint32_t & hitChunkCount) const
{
	bool validOperation = false;
	{
		hitChunkContainer = !hitChunkParams.hitChunkContainer.empty() ? static_cast<const DestructibleHitChunk*>(&hitChunkParams.hitChunkContainer[0]) : NULL;
		hitChunkCount = hitChunkParams.hitChunkContainer.size();
		validOperation = true;
	}
	return validOperation;
}

bool DestructibleActorImpl::forceChunkHits(const DestructibleHitChunk * hitChunkContainer, uint32_t hitChunkCount, bool removeChunks /*= true*/, bool deferredEvent /*= false*/, PxVec3 damagePosition /*= PxVec3(0.0f)*/, PxVec3 damageDirection /*= PxVec3(0.0f)*/)
{
	bool validOperation = false;
	PX_ASSERT(!((NULL == hitChunkContainer) ^ (0 == hitChunkCount)));
	if(NULL != hitChunkContainer && 0 != hitChunkCount)
	{
		uint32_t manualFractureCount = 0;
		for(uint32_t index = 0; index < hitChunkCount; ++index)
		{
			if(mStructure->chunks.size() > (hitChunkContainer[index].chunkIndex + mFirstChunkIndex))
			{
				hitChunkParams.manualFractureEventInstance.chunkIndexInAsset	= hitChunkContainer[index].chunkIndex;
				hitChunkParams.manualFractureEventInstance.destructibleID		= mID;
				hitChunkParams.manualFractureEventInstance.position				= damagePosition;
				hitChunkParams.manualFractureEventInstance.hitDirection			= damageDirection;
				hitChunkParams.manualFractureEventInstance.flags				= (hitChunkContainer[index].hitChunkFlags | static_cast<uint32_t>(FractureEvent::Manual) | (removeChunks ? static_cast<uint32_t>(FractureEvent::DeleteChunk) : 0));
				if(deferredEvent)
				{
					mDestructibleScene->mDeferredFractureBuffer.pushBack()		= hitChunkParams.manualFractureEventInstance;
				}
				else
				{
					mDestructibleScene->mDeprioritisedFractureBuffer.pushBack()		= hitChunkParams.manualFractureEventInstance;					
				}
				++manualFractureCount;
			}
		}
		validOperation = (manualFractureCount == hitChunkCount);
	}
	return validOperation;
}

void DestructibleActorImpl::evaluateForHitChunkList(const FractureEvent & fractureEvent)
{
	PX_ASSERT(NULL != &fractureEvent);

	// cache hit chunks for non-manual fractureEvents, if user set so
	if(hitChunkParams.cacheChunkHits && (0 == (fractureEvent.flags & FractureEvent::Manual)))
	{
		// walk up the depth until we reach the user-specified tracking depth
		int32_t chunkToUseIndexInAsset = static_cast<int32_t>(fractureEvent.chunkIndexInAsset);
		PX_ASSERT(chunkToUseIndexInAsset >= 0);
		while(ModuleDestructibleConst::INVALID_CHUNK_INDEX != chunkToUseIndexInAsset)
		{
			PX_ASSERT(chunkToUseIndexInAsset < static_cast<int32_t>(getDestructibleAsset()->getChunkCount()));
			const DestructibleAssetParametersNS::Chunk_Type & source = getDestructibleAsset()->mParams->chunks.buf[chunkToUseIndexInAsset];
			if(source.depth <= hitChunkParams.trackingDepth)
			{
				break;
			}
			chunkToUseIndexInAsset = static_cast<int32_t>(source.parentIndex);
		}

		// cache the chunk index at the user-specified tracking depth
		if(ModuleDestructibleConst::INVALID_CHUNK_INDEX != chunkToUseIndexInAsset)
		{
			PX_ASSERT(chunkToUseIndexInAsset + getFirstChunkIndex() < getStructure()->chunks.size());
			const DestructibleStructure::Chunk & chunkToUse = getStructure()->chunks[chunkToUseIndexInAsset + getFirstChunkIndex()];
			if((!chunkToUse.isDestroyed()) && (0 == (FractureEvent::DamageFromImpact & fractureEvent.flags)) && (!hitChunkParams.cacheAllChunks ? ((chunkToUse.state & ChunkDynamic) == 0) : true))
			{
				// these flags are only used internally for fracture events coming in from the sync buffer, so we should exclude them
				uint32_t hitChunkFlags = fractureEvent.flags;
				hitChunkFlags &= ~FractureEvent::SyncDirect;
				hitChunkFlags &= ~FractureEvent::SyncDerived;
				hitChunkFlags &= ~FractureEvent::Manual;

				// disallow crumbling and snapping as well
				hitChunkFlags &= ~FractureEvent::CrumbleChunk;
				hitChunkFlags &= ~FractureEvent::Snap;

				// cache the chunk index
				hitChunkParams.hitChunkContainer.pushBack(DestructibleActorImpl::CachedHitChunk(static_cast<uint32_t>(chunkToUseIndexInAsset), hitChunkFlags));
			}
		}
	}
}

bool DestructibleActorImpl::getDamageColoringHistory(const DamageEventCoreData *& damageEventCoreDataContainer, uint32_t & damageEventCoreDataCount) const
{
	bool validOperation = false;
	{
		damageEventCoreDataContainer = !damageColoringParams.damageEventCoreDataContainer.empty() ? static_cast<const DamageEventCoreData*>(&damageColoringParams.damageEventCoreDataContainer[0]) : NULL;
		damageEventCoreDataCount = damageColoringParams.damageEventCoreDataContainer.size();
		validOperation = true;
	}
	return validOperation;

}

bool DestructibleActorImpl::forceDamageColoring(const DamageEventCoreData * damageEventCoreDataContainer, uint32_t damageEventCoreDataCount)
{
	bool validOperation = false;
	PX_ASSERT(!((NULL == damageEventCoreDataContainer) ^ (0 == damageEventCoreDataCount)));
	if (NULL != damageEventCoreDataContainer && 0 != damageEventCoreDataCount)
	{
		uint32_t manualDamageColorCount = 0;
		for (uint32_t index = 0; index < damageEventCoreDataCount; ++index)
		{
			if (mStructure->chunks.size() > (damageEventCoreDataContainer[index].chunkIndexInAsset + mFirstChunkIndex))
			{
				damageColoringParams.damageEventCoreDataInstance.destructibleID = mID;
				damageColoringParams.damageEventCoreDataInstance.chunkIndexInAsset = damageEventCoreDataContainer[index].chunkIndexInAsset;
				damageColoringParams.damageEventCoreDataInstance.position = damageEventCoreDataContainer[index].position;
				damageColoringParams.damageEventCoreDataInstance.damage = damageEventCoreDataContainer[index].damage;
				damageColoringParams.damageEventCoreDataInstance.radius = damageEventCoreDataContainer[index].radius;

				mDestructibleScene->mSyncDamageEventCoreDataBuffer.pushBack() = damageColoringParams.damageEventCoreDataInstance;

				++manualDamageColorCount;
			}
		}
		validOperation = (manualDamageColorCount == damageEventCoreDataCount);
	}
	return validOperation;

}

void DestructibleActorImpl::collectDamageColoring(const int32_t indexInAsset, const PxVec3& position, const float damage, const float damageRadius)
{
	// only start cached the damage coloring when the flag set. see setHitChunkTrackingParams()
	if (hitChunkParams.cacheChunkHits)
	{
		damageColoringParams.damageEventCoreDataContainer.pushBack(DestructibleActorImpl::CachedDamageEventCoreData(indexInAsset, position, damage, damageRadius));
	}
}

void DestructibleActorImpl::applyDamageColoring_immediate(const int32_t indexInAsset, const PxVec3& position, const float damage, const float damageRadius)
{
	// Damage coloring:
	if (useDamageColoring())
	{
		if (indexInAsset != ModuleDestructibleConst::INVALID_CHUNK_INDEX)
		{
			// Normal Damage - apply damage coloring directly
			applyDamageColoring(static_cast<uint16_t>(indexInAsset), position, damage, damageRadius);
		}
		else
		{
			// Radius Damage - need to traverse all the visible chunks
			const uint16_t* chunkIndexPtr = mVisibleChunks.usedIndices();
			const uint16_t* chunkIndexPtrStop = chunkIndexPtr + mVisibleChunks.usedCount();

			while (chunkIndexPtr < chunkIndexPtrStop)
			{
				uint16_t chunkIndex = *chunkIndexPtr++;
				DestructibleStructure::Chunk& chunk = mStructure->chunks[chunkIndex + mFirstChunkIndex];

				applyDamageColoring(chunk.indexInAsset, position, damage, damageRadius);
			}
		}
	}
}

bool DestructibleActorImpl::getUseLegacyChunkBoundsTesting() const
{
	const int8_t setting = mParams->destructibleParameters.legacyChunkBoundsTestSetting;
	if (setting < 0)
	{
		return mDestructibleScene->getModuleDestructible()->getUseLegacyChunkBoundsTesting();
	}
	return setting > 0;
}

bool DestructibleActorImpl::getUseLegacyDamageRadiusSpread() const
{
	const int8_t setting = mParams->destructibleParameters.legacyDamageRadiusSpreadSetting;
	if (setting < 0)
	{
		return mDestructibleScene->getModuleDestructible()->getUseLegacyDamageRadiusSpread();
	}
	return setting > 0;
}

/*** DestructibleActor::SyncParams ***/
bool DestructibleActorImpl::setSyncParams(uint32_t userActorID, uint32_t actorSyncFlags, const DestructibleActorSyncState * actorSyncState, const DestructibleChunkSyncState * chunkSyncState)
{
	bool validEntry = false;
	PX_ASSERT(!mDestructibleScene->getSyncParams().lockSyncParams && "if this happens, theres more work to do!");
	if(!mDestructibleScene->getSyncParams().lockSyncParams)
	{
		const bool validActorSyncFlags = static_cast<uint32_t>(DestructibleActorSyncFlags::Last) > actorSyncFlags;
		const bool validActorSyncState = (NULL != actorSyncState) ? (mAsset->getDepthCount() >= actorSyncState->damageEventFilterDepth) && (mAsset->getDepthCount() >= actorSyncState->fractureEventFilterDepth) : true;
		const bool validChunkSyncState = (NULL != chunkSyncState) ? (mAsset->getDepthCount() >= chunkSyncState->chunkTransformCopyDepth) : true;
#if 0
		//const bool validDepth = (mAsset->getDepthCount() >= chunkPositionCopyDepth) ? (chunkPositionCopyDepth >= chunkTransformCopyDepth) : false;
#endif
		if(validActorSyncFlags && validActorSyncState && validChunkSyncState)
		{
			//determine type of operation
			const uint32_t presentUserActorID = mSyncParams.getUserActorID();
			const bool addEntry			= (0 != userActorID) && (0 == presentUserActorID);
			const bool removeEntry		= (0 == userActorID) && (0 != presentUserActorID);
			const bool relocateEntry	= (0 != userActorID) && (0 != presentUserActorID) && (userActorID != presentUserActorID);
			const bool editEntry		= (0 != userActorID) && (0 != presentUserActorID) && (userActorID == presentUserActorID);
			const bool invalidEntry		= (0 == userActorID) && (0 == presentUserActorID);
			PX_ASSERT(addEntry ? (!removeEntry && !relocateEntry && !editEntry && !invalidEntry) : removeEntry ? (!relocateEntry && !editEntry && !invalidEntry) : relocateEntry ? (!editEntry && !invalidEntry) : editEntry? (!invalidEntry) : invalidEntry);

			//attempt update scene and actor params
			if(addEntry || removeEntry || relocateEntry || editEntry)
			{
				bool validUserActorID = false;
				bool useUserArguments = false;
				DestructibleScene::SyncParams & sceneParams = mDestructibleScene->getSyncParamsMutable();
				DestructibleActorImpl * erasedEntry = NULL;
				if(addEntry)
				{
					validUserActorID = sceneParams.setSyncActor(userActorID, this, erasedEntry);
					PX_ASSERT(this != erasedEntry);
					if(validUserActorID)
					{
						useUserArguments = true;
						if(NULL != erasedEntry)
						{
							erasedEntry->getSyncParamsMutable().onReset();
							erasedEntry = NULL;
						}
					}
				}
				else if(removeEntry)
				{
					validUserActorID = sceneParams.setSyncActor(presentUserActorID, NULL, erasedEntry);
					PX_ASSERT((NULL != erasedEntry) && (this == erasedEntry));
					if(validUserActorID)
					{
						mSyncParams.onReset();
					}
				}
				else if(relocateEntry)
				{
					validUserActorID = sceneParams.setSyncActor(presentUserActorID, NULL, erasedEntry);
					PX_ASSERT((NULL != erasedEntry) && (this == erasedEntry));
					if(validUserActorID)
					{
						validUserActorID = sceneParams.setSyncActor(userActorID, this, erasedEntry);
						PX_ASSERT(this != erasedEntry);
						if(validUserActorID)
						{
							useUserArguments = true;
							if(NULL != erasedEntry)
							{
								erasedEntry->getSyncParamsMutable().onReset();
								erasedEntry = NULL;
							}
						}
					}
				}
				else if(editEntry)
				{
					validUserActorID = true;
					useUserArguments = true;
				}
				else
				{
					PX_ASSERT(!"!");
				}
				validEntry = validUserActorID && validActorSyncFlags && validActorSyncState && validChunkSyncState;
				if(useUserArguments)
				{
					PX_ASSERT(validEntry);
					mSyncParams.userActorID			= userActorID;
					mSyncParams.actorSyncFlags		= actorSyncFlags;
					if (actorSyncState)
					{
						mSyncParams.useActorSyncState = true;
						mSyncParams.actorSyncState	= *actorSyncState;
					}
					if (chunkSyncState)
					{
						mSyncParams.useChunkSyncState = true;
						mSyncParams.chunkSyncState	= *chunkSyncState;
					}
				}
			}
			else
			{
				PX_ASSERT(invalidEntry);
				PX_UNUSED(invalidEntry);
				PX_ASSERT(!"invalid use of function!");
			}
		}
	}
	return validEntry;
}

typedef DestructibleActorImpl::SyncParams SyncParams;

SyncParams::SyncParams()
	:userActorID(0)
	,actorSyncFlags(0)
	,useActorSyncState(false)
	,useChunkSyncState(false)
{
	PX_ASSERT(damageBufferIndices.empty());
	PX_ASSERT(fractureBufferIndices.empty());
    PX_ASSERT(cachedChunkTransforms.empty());
}

SyncParams::~SyncParams()
{
	PX_ASSERT(cachedChunkTransforms.empty());
	PX_ASSERT(fractureBufferIndices.empty());
	PX_ASSERT(damageBufferIndices.empty());
	PX_ASSERT(!useChunkSyncState);
	PX_ASSERT(!useActorSyncState);
	PX_ASSERT(0 == actorSyncFlags);
	PX_ASSERT(0 == userActorID);
}

uint32_t SyncParams::getUserActorID() const
{
	return userActorID;
}

bool SyncParams::isSyncFlagSet(DestructibleActorSyncFlags::Enum flag) const
{
	PX_ASSERT(0 != userActorID);
	return (0 != (actorSyncFlags & static_cast<uint32_t>(flag)));
}

const DestructibleActorSyncState * SyncParams::getActorSyncState() const
{
	PX_ASSERT(useActorSyncState ? (isSyncFlagSet(DestructibleActorSyncFlags::CopyDamageEvents) || isSyncFlagSet(DestructibleActorSyncFlags::CopyFractureEvents)) : true);
	return useActorSyncState ? &actorSyncState : NULL;
}

const DestructibleChunkSyncState * SyncParams::getChunkSyncState() const
{
	PX_ASSERT(useChunkSyncState ? isSyncFlagSet(DestructibleActorSyncFlags::CopyChunkTransform) : true);
	return useChunkSyncState ? &chunkSyncState : NULL;
}

void SyncParams::pushDamageBufferIndex(uint32_t index)
{
	PX_ASSERT(isSyncFlagSet(DestructibleActorSyncFlags::CopyDamageEvents));
	damageBufferIndices.pushBack(index);
}

void SyncParams::pushFractureBufferIndex(uint32_t index)
{
	PX_ASSERT(isSyncFlagSet(DestructibleActorSyncFlags::CopyFractureEvents));
	fractureBufferIndices.pushBack(index);
}

void SyncParams::pushCachedChunkTransform(const CachedChunk & cachedChunk)
{
	PX_ASSERT(isSyncFlagSet(DestructibleActorSyncFlags::CopyChunkTransform));
	cachedChunkTransforms.pushBack(cachedChunk);
}

const physx::Array<uint32_t> & SyncParams::getDamageBufferIndices() const
{
	return damageBufferIndices;
}

const physx::Array<uint32_t> & SyncParams::getFractureBufferIndices() const
{
	return fractureBufferIndices;
}

const physx::Array<CachedChunk> & SyncParams::getCachedChunkTransforms() const
{
	return cachedChunkTransforms;
}

template<> void SyncParams::clear<DamageEventUnit>()
{
	PX_ASSERT(!damageBufferIndices.empty() ? isSyncFlagSet(DestructibleActorSyncFlags::CopyDamageEvents) : true);
	if(!damageBufferIndices.empty()) damageBufferIndices.clear();
}

template<> void SyncParams::clear<FractureEventUnit>()
{
	PX_ASSERT(!fractureBufferIndices.empty() ? isSyncFlagSet(DestructibleActorSyncFlags::CopyFractureEvents) : true);
	if(!fractureBufferIndices.empty()) fractureBufferIndices.clear();
}

template<> void SyncParams::clear<ChunkTransformUnit>()
{
	PX_ASSERT(!cachedChunkTransforms.empty() ? isSyncFlagSet(DestructibleActorSyncFlags::CopyChunkTransform) : true);
	if(!cachedChunkTransforms.empty()) cachedChunkTransforms.clear();
}

template<> uint32_t SyncParams::getCount<DamageEventUnit>() const
{
	PX_ASSERT(!damageBufferIndices.empty() ? isSyncFlagSet(DestructibleActorSyncFlags::CopyDamageEvents) : true);
	return damageBufferIndices.size();
}

template<> uint32_t SyncParams::getCount<FractureEventUnit>() const
{
	PX_ASSERT(!fractureBufferIndices.empty() ? isSyncFlagSet(DestructibleActorSyncFlags::CopyFractureEvents) : true);
	return fractureBufferIndices.size();
}

template<> uint32_t SyncParams::getCount<ChunkTransformUnit>() const
{
	PX_ASSERT(!cachedChunkTransforms.empty() ? isSyncFlagSet(DestructibleActorSyncFlags::CopyChunkTransform) : true);
	return cachedChunkTransforms.size();
}

void SyncParams::onReset()
{
	userActorID = 0;
	actorSyncFlags = 0;
	useActorSyncState = false;
	useChunkSyncState = false;
	damageBufferIndices.clear();
	fractureBufferIndices.clear();
	cachedChunkTransforms.clear();
}

const SyncParams & DestructibleActorImpl::getSyncParams() const
{
    return mSyncParams;
}

SyncParams & DestructibleActorImpl::getSyncParamsMutable()
{
    return const_cast<SyncParams&>(getSyncParams());
}

// Renderable support:

DestructibleRenderable* DestructibleActorImpl::acquireRenderableReference()
{
	return mRenderable ? mRenderable->incrementReferenceCount() : NULL;
}

RenderMeshActor* DestructibleActorImpl::getRenderMeshActor(DestructibleActorMeshType::Enum type) const
{
	return mRenderable ? mRenderable->getRenderMeshActor(type) : NULL;
}

}
} // end namespace nvidia


