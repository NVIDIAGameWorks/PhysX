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



#ifndef __DESTRUCTIBLEACTOR_IMPL_H__
#define __DESTRUCTIBLEACTOR_IMPL_H__

#include "Apex.h"
#include "ApexActor.h"
#include "DestructibleAssetProxy.h"
#include "DestructibleActorState.h"
#include "DestructibleActor.h"
#include "DestructibleStructure.h"
#include "RenderMeshAssetIntl.h"
#include "ResourceProviderIntl.h"
#include "DestructibleRenderableImpl.h"
#if APEX_RUNTIME_FRACTURE
#include "SimScene.h"
#include "../fracture/Actor.h"
#endif

namespace nvidia
{
namespace apex
{
class PhysXObjectDescIntl;
class EmitterActor;
}
namespace destructible
{

class DestructibleScene;
class DestructibleActorProxy;
class DestructibleActorJointImpl;

struct PhysXActorFlags
{
	enum Enum
	{
		DEPTH_PARAM_USER_FLAG_0 = 0,
		DEPTH_PARAM_USER_FLAG_1,
		DEPTH_PARAM_USER_FLAG_2,
		DEPTH_PARAM_USER_FLAG_3,
		CREATED_THIS_FRAME,
		IS_SLEEPING // external sleep state tracking to be in sync with mAwakeActorCount that is updated in onWake/onSleep callbacks
	};
};

PX_INLINE void PxBounds3Transform(PxBounds3& b, const PxMat44& tm)
{
	b = PxBounds3::basisExtent(tm.transform(b.getCenter()), PxMat33(tm.getBasis(0), tm.getBasis(1), tm.getBasis(2)), b.getExtents());
}

class DestructibleActorImpl : public ApexResource
	, public ApexActorSource
	, public ApexActor
	, public NvParameterized::SerializationCallback
{
public:

	friend class DestructibleActorProxy;
	friend class DestructibleRenderableImpl;
	friend class DestructibleScopedReadLock;
	friend class DestructibleScopedWriteLock;

	enum
	{
		InvalidID =	0xFFFFFFFF
	};

	enum Flag
	{
		Dynamic	=	(1 << 0)
	};

	enum InternalFlag
	{
		IslandMarker =	(1 << 15)
	};

public:
	
	void							setState(NvParameterized::Interface* state);
	const DestructibleActorState*	getState() const					{ return mState; }
	const DestructibleActorChunks*  getChunks() const					{ return mChunks; }
	      DestructibleActorChunks*  getChunks() 						{ return mChunks; }
	const DestructibleActorParam*	getParams() const					{ return mParams; }
	      DestructibleActorParam*	getParams()							{ return mParams; }
	const DestructibleParameters&	getDestructibleParameters() const	{ return mDestructibleParameters; }
	      DestructibleParameters&	getDestructibleParameters() 		{ return mDestructibleParameters; }
	void							setDestructibleParameters(const DestructibleParameters& destructibleParameters);
	const DestructibleScene*		getDestructibleScene() const		{ return mDestructibleScene; }
	      DestructibleScene*		getDestructibleScene()				{ return mDestructibleScene; }
	const DestructibleActor*		getAPI() const						{ return mAPI; }
	      DestructibleActor*		getAPI()							{ return mAPI; }
	
	void					incrementWakeCount(void);
	void					decrementWakeCount(void);
	void					removeActorAtIndex(uint32_t index);

	void					setPhysXScene(PxScene*);
	PxScene*				getPhysXScene() const;

	void					release();
	void					destroy();
	void					reset();
	
	virtual void			getLodRange(float& min, float& max, bool& intOnly) const;
	virtual float			getActiveLod() const;
	virtual void			forceLod(float lod);

	void					cacheModuleData() const;
	void					removeSelfFromStructure();
	void					removeSelfFromScene();
	bool					isInitiallyDynamic() const				{ return 0 != (getFlags() & (uint32_t)Dynamic); }
	uint16_t			getFlags() const						{ return mFlags; }
	uint16_t			getInternalFlags() const				{ return mInternalFlags; }
	uint16_t&			getInternalFlags()						{ return mInternalFlags; }
	DestructibleStructure*		getStructure()						{ return mStructure; }
	const DestructibleStructure*getStructure() const				{ return mStructure; }
	void 					setStructure(DestructibleStructure* s)	{ mStructure = s; }
	DestructibleAssetImpl*			getDestructibleAsset()							{ return mAsset; }
	const DestructibleAssetImpl*	getDestructibleAsset() const					{ return mAsset; }
	uint32_t				getID() const						{ return mID; }
	uint32_t&				getIDRef()							{ return mID; }
	const PxBounds3&		getOriginalBounds() const			{ return mOriginalBounds; }
	uint32_t				getFirstChunkIndex() const			{ return mFirstChunkIndex; }
	void						setFirstChunkIndex(uint32_t i)	{ mFirstChunkIndex = i; }
	const EmitterActor*	getCrumbleEmitter() const			{ return mCrumbleEmitter; }
	      EmitterActor*	getCrumbleEmitter()					{ return mCrumbleEmitter; }
		  float			getCrumbleParticleSpacing() const;
	const EmitterActor*	getDustEmitter() const				{ return mDustEmitter; }
	      EmitterActor*	getDustEmitter()					{ return mDustEmitter; }
		  float			getDustParticleSpacing() const;
	const PxMat44&	getInitialGlobalPose() const			{ return mTM; }
	void					setInitialGlobalPose(const PxMat44& pose);
	const PxVec3&	getScale() const						{ return mParams->scale; }
	void					setCrumbleEmitterEnabled(bool enabled)	{ mState->enableCrumbleEmitter = enabled; }
	bool					isCrumbleEmitterEnabled() const			{ return mState->enableCrumbleEmitter; }
	void					setDustEmitterEnabled(bool enabled)		{ mState->enableDustEmitter = enabled; }
	bool					isDustEmitterEnabled() const			{ return mState->enableDustEmitter; }
	void					setCrumbleEmitterName(const char*);
	const char*				getCrumbleEmitterName() const;
	void					setDustEmitterName(const char*);
	const char*				getDustEmitterName() const;

	uint32_t			getSupportDepth() const					{ return mParams->supportDepth; }
	bool					formExtendedStructures() const			{ return mParams->formExtendedStructures; }
	bool					performDetailedOverlapTestForExtendedStructures() const { return mParams->performDetailedOverlapTestForExtendedStructures; }
	bool					useAssetDefinedSupport() const			{ return mParams->useAssetDefinedSupport; }
	bool					useWorldSupport() const					{ return mParams->useWorldSupport; }
	bool					drawStaticChunksInSeparateMesh() const	{ return mParams->renderStaticChunksSeparately; }
	bool					keepVisibleBonesPacked() const			{ return mParams->keepVisibleBonesPacked; }
	bool					createChunkEvents() const				{ return mParams->createChunkEvents; }
	bool					keepPreviousFrameBoneBuffer() const		{ return mParams->keepPreviousFrameBoneBuffer; }
	float			getSleepVelocityFrameDecayConstant() const	{ return mParams->sleepVelocityFrameDecayConstant; }
	bool					useHardSleeping() const					{ return mParams->useHardSleeping; }
	bool					useStressSolver() const					{ return mParams->structureSettings.useStressSolver; }
	float			getStressSolverTimeDelay() const		{ return mParams->structureSettings.stressSolverTimeDelay; }
	float			getStressSolverMassThreshold() const	{ return mParams->structureSettings.stressSolverMassThreshold; }

	void					enableHardSleeping();
	void					disableHardSleeping(bool wake);
	bool					setChunkPhysXActorAwakeState(uint32_t chunkIndex, bool awake);
	bool					addForce(uint32_t chunkIndex, const PxVec3& force, physx::PxForceMode::Enum mode, const PxVec3* position = NULL, bool wakeup = true);

	uint32_t			getLOD() const { return mState->lod; }

	uint32_t			getRenderSubmeshCount() 
	{
		return mAsset->getRenderMeshAsset()->getSubmeshCount();
	}
	uint32_t			getAwakeActorCount() const { return mAwakeActorCount; }
#if APEX_RUNTIME_FRACTURE
	::nvidia::fracture::Actor*		getRTActor()
	{
		return mRTActor;
	}
#endif

	void					setGlobalPose(const PxMat44& pose);
	void					setGlobalPoseForStaticChunks(const PxMat44& pose);
	bool					getGlobalPoseForStaticChunks(PxMat44& pose) const;

	void					setChunkPose(uint32_t index, PxTransform worldPose);
	void					setLinearVelocity(const PxVec3& linearVelocity);
	void					setAngularVelocity(const PxVec3& angularVelocity);

	void					setDynamic(int32_t chunkIndex = ModuleDestructibleConst::INVALID_CHUNK_INDEX, bool immediate = false);
	bool					getDynamic(int32_t chunkIndex) const
	{
		PX_ASSERT(mAsset != NULL && mStructure != NULL && chunkIndex >= 0 && chunkIndex < (int32_t)mAsset->getChunkCount());
		DestructibleStructure::Chunk& chunk = mStructure->chunks[mFirstChunkIndex + chunkIndex];
		return (chunk.state & ChunkDynamic) != 0;
	}

	void					getChunkVisibilities(uint8_t* visibilityArray, uint32_t visibilityArraySize) const;
	void					setChunkVisibility(uint16_t index, bool visibility);

	// Chunk event buffer API
	bool					acquireChunkEventBuffer(const DestructibleChunkEvent*& buffer, uint32_t& bufferSize);
	bool					releaseChunkEventBuffer(bool clearBuffer = true);

	// PhysX actor buffer API
	bool					acquirePhysXActorBuffer(physx::PxRigidDynamic**& buffer, uint32_t& bufferSize, uint32_t flags);
	bool					releasePhysXActorBuffer();

	float			getContactReportThreshold(const DestructibleStructure::Chunk& chunk) const
	{
		const DestructibleActorParamNS::BehaviorGroup_Type& behaviorGroup = getBehaviorGroup(chunk.indexInAsset);
		return getContactReportThreshold(behaviorGroup);
	}

	float			getContactReportThreshold(const DestructibleActorParamNS::BehaviorGroup_Type& behaviorGroup) const
	{
		float contactReportThreshold = PX_MAX_F32;
		if (mDestructibleParameters.forceToDamage > 0)
		{
			const float maxEstimatedTimeStep = 0.1f;
			const float thresholdFraction = 0.5f;
			const float damageThreshold = behaviorGroup.damageThreshold;
			contactReportThreshold = thresholdFraction * maxEstimatedTimeStep * damageThreshold / mDestructibleParameters.forceToDamage;
		}

		return contactReportThreshold;
	}

	float		getAge(float elapsedTime) const		{ return elapsedTime - mStartTime; }

	bool				getUseLegacyChunkBoundsTesting() const;
	bool				getUseLegacyDamageRadiusSpread() const;

	const IndexBank<uint16_t>&	getStaticRoots() const			{ return mStaticRoots; }
	      IndexBank<uint16_t>&	getStaticRoots()				{ return mStaticRoots; }

	uint32_t			getNumVisibleChunks() const		{ return mVisibleChunks.usedCount(); }
	const uint16_t*		getVisibleChunks() const		{ return mVisibleChunks.usedIndices(); }
	bool					initializedFromState() const 	{ return mInitializedFromState; }

	uint32_t			getVisibleDynamicChunkShapeCount() const			{ return mVisibleDynamicChunkShapeCount; }
	uint32_t			getEssentialVisibleDynamicChunkShapeCount() const	{ return mEssentialVisibleDynamicChunkShapeCount; }

	void					increaseVisibleDynamicChunkShapeCount(const uint32_t number)			{ mVisibleDynamicChunkShapeCount += number; }
	void					increaseEssentialVisibleDynamicChunkShapeCount(const uint32_t number) 	{ mEssentialVisibleDynamicChunkShapeCount += number; }

	void					initializeChunk(uint32_t index, DestructibleStructure::Chunk& chunk) const;
	bool					getInitialChunkDynamic(uint32_t index) const;
	bool					getInitialChunkVisible(uint32_t index) const;
	bool					getInitialChunkDestroyed(uint32_t index) const;
	PxTransform		getInitialChunkGlobalPose(uint32_t index) const;
	PxTransform		getInitialChunkLocalPose(uint32_t index) const;
	PxVec3			getInitialChunkLinearVelocity(uint32_t index) const;
	PxVec3			getInitialChunkAngularVelocity(uint32_t index) const;
	PxTransform		getChunkPose(uint32_t index) const;
	PxTransform		getChunkTransform(uint32_t index) const;
	PxVec3			getChunkLinearVelocity(uint32_t index) const;
	PxVec3			getChunkAngularVelocity(uint32_t index) const;
	const PxRigidDynamic*		getChunkActor(uint32_t index) const;
	PxRigidDynamic*				getChunkActor(uint32_t index);
	uint32_t			getChunkPhysXShapes(physx::PxShape**& shapes, uint32_t chunkIndex) const;
	uint32_t			getChunkCount() const { return getDestructibleAsset()->getChunkCount(); }
	const DestructibleStructure::Chunk& getChunk(uint32_t index) const;

	void					setBenefit(float benefit)	{ mBenefit = benefit; }
	float			getBenefit() const					{ return mBenefit; }

	float			getChunkMass(uint32_t index) const
	{
		float volume = 0.0f;
		for (uint32_t hullIndex = mAsset->getChunkHullIndexStart(index); hullIndex < mAsset->getChunkHullIndexStop(index); ++hullIndex)
		{
			volume += PxAbs(mAsset->chunkConvexHulls[hullIndex].mParams->volume);
		}

		// override actor descriptor if (behaviorGroup.density > 0)
		const DestructibleActorParamNS::BehaviorGroup_Type& behaviorGroup = getBehaviorGroup(index);
		float density = behaviorGroup.density;
		if (density == 0)
		{
			density = physX3Template.data.density;
		}
		return volume * density * PxAbs(getScale().x * getScale().y * getScale().z);
	}

	const PxMat44	getChunkTM(uint32_t index) const
	{
		DestructibleActorMeshType::Enum typeN;
		if (getDynamic((int32_t)index) || !drawStaticChunksInSeparateMesh())
		{
			typeN = DestructibleActorMeshType::Skinned;
		}
		else
		{
			typeN = DestructibleActorMeshType::Static;
			index = 0;	// Static rendering only has one transform
		}
		RenderMeshActor* rma = getRenderMeshActor(typeN);
		if (rma != NULL)
		{
			return rma->getTM(mAsset->getPartIndex(index));
		}
		return PxMat44(PxIdentity);
	}

	int32_t			getBehaviorGroupIndex(uint32_t chunkIndex) const
	{
		PX_ASSERT(chunkIndex < (uint16_t)mAsset->mParams->chunks.arraySizes[0]);
		const DestructibleAssetParametersNS::Chunk_Type& sourceChunk = mAsset->mParams->chunks.buf[chunkIndex];

		return (int32_t)sourceChunk.behaviorGroupIndex;
	}

	uint32_t			getChunkActorFlags(uint32_t chunkIndex) const;

	const DestructibleActorParamNS::BehaviorGroup_Type& getBehaviorGroupImp(int8_t behaviorGroupIndex) const
	{
		if (behaviorGroupIndex == -1)
		{
			return mParams->defaultBehaviorGroup;
		}
		else
		{
			PX_ASSERT(behaviorGroupIndex < (uint16_t)mParams->behaviorGroups.arraySizes[0]);
			return mParams->behaviorGroups.buf[behaviorGroupIndex];
		}
	}

	const DestructibleActorParamNS::BehaviorGroup_Type& getBehaviorGroup(uint32_t chunkIndex) const
	{
		PX_ASSERT(chunkIndex < (uint16_t)mAsset->mParams->chunks.arraySizes[0]);
		const DestructibleAssetParametersNS::Chunk_Type& sourceChunk = mAsset->mParams->chunks.buf[chunkIndex];

		return getBehaviorGroupImp(sourceChunk.behaviorGroupIndex);
	}

	const DestructibleActorParamNS::BehaviorGroup_Type& getRTFractureBehaviorGroup() const
	{
		return getBehaviorGroupImp(mAsset->mParams->RTFractureBehaviorGroup);
	}

	void					setSkinnedOverrideMaterial(uint32_t index, const char* overrideMaterialName);
	void					setStaticOverrideMaterial(uint32_t index, const char* overrideMaterialName);
	void					setRuntimeFracturePattern(const char* fracturePatternName);

	void					updateRenderResources(bool rewriteBuffers, void* userRenderData);
	void					dispatchRenderResources(UserRenderer& renderer);

	void					applyDamage(float damage, float momentum, const PxVec3& position, const PxVec3& direction, int32_t chunkIndex = ModuleDestructibleConst::INVALID_CHUNK_INDEX, void* damageUserData = NULL);
	void					applyRadiusDamage(float damage, float momentum, const PxVec3& position, float radius, bool falloff, void* damageUserData = NULL);

	void					takeImpact(const PxVec3& force, const PxVec3& position, uint16_t chunkIndex, PxActor const* damageActor);
	bool					takesImpactDamageAtDepth(uint32_t depth)
	{
		if (depth < mDestructibleParameters.depthParametersCount)
		{
			if (mDestructibleParameters.depthParameters[depth].overrideImpactDamage())
			{
				return mDestructibleParameters.depthParameters[depth].overrideImpactDamageValue();
			}
		}
		return (int32_t)depth <= mDestructibleParameters.impactDamageDefaultDepth;
	}

	bool					isChunkSolitary(int32_t chunkIndex) const
	{
		PX_ASSERT(mAsset != NULL && mStructure != NULL && chunkIndex >= 0 && chunkIndex < (int32_t)mAsset->getChunkCount());
		DestructibleStructure::Chunk& chunk = mStructure->chunks[mFirstChunkIndex + chunkIndex];
		return mStructure->chunkIsSolitary( chunk );
	}

	bool					isChunkDestroyed(uint32_t chunkIndex) const
	{
		PX_ASSERT(mAsset != NULL && mStructure != NULL && chunkIndex < mAsset->getChunkCount());
		DestructibleStructure::Chunk& chunk = mStructure->chunks[mFirstChunkIndex + chunkIndex];
		return chunk.isDestroyed();
	}

	PxBounds3		getChunkLocalBounds(uint32_t chunkIndex) const
	{ 
		PX_ASSERT(mAsset != NULL && mStructure != NULL && chunkIndex < mAsset->getChunkCount());

		PxBounds3 localBounds = getDestructibleAsset()->getChunkShapeLocalBounds( chunkIndex );

		PxVec3 boundsExtents = localBounds.getExtents();
		PxVec3 scaledExtents( boundsExtents.x * getScale().x, boundsExtents.y * getScale().y, boundsExtents.z * getScale().z );

		PxBounds3 scaledLocalBounds = PxBounds3::centerExtents( localBounds.getCenter(), scaledExtents );

		return scaledLocalBounds;
	}

	PxBounds3		getChunkBounds(uint32_t chunkIndex) const
	{ 
		PX_ASSERT(mAsset != NULL && mStructure != NULL && chunkIndex < mAsset->getChunkCount());

		DestructibleStructure::Chunk& chunk = mStructure->chunks[mFirstChunkIndex + chunkIndex];
		PxMat44 chunkGlobalPose = mStructure->getChunkGlobalPose( chunk );

		PxBounds3 chunkBounds = getDestructibleAsset()->getChunkShapeLocalBounds( chunkIndex );

		// Apply scaling.
		chunkGlobalPose.scale( PxVec4(getScale(), 1.f) );

		// Transform into actor local space.
		PxBounds3Transform( chunkBounds, chunkGlobalPose );

		return chunkBounds;
	}

	uint32_t			getSupportDepthChunkIndices(uint32_t* const OutChunkIndices, uint32_t  MaxOutIndices) const
	{
		return mStructure->getSupportDepthChunkIndices( OutChunkIndices,  MaxOutIndices );
	}

	PxBounds3		getLocalBounds() const
	{
		PxBounds3 bounds = getDestructibleAsset()->getBounds();
		PxVec3 scale = getScale();

		bounds.minimum.x *= scale.x;
		bounds.minimum.y *= scale.y;
		bounds.minimum.z *= scale.z;

		bounds.maximum.x *= scale.x;
		bounds.maximum.y *= scale.y;
		bounds.maximum.z *= scale.z;

		return bounds;
	}

	float			getLinearSize() const { return mLinearSize; }

	void					applyDamage_immediate(struct DamageEvent& damageEvent);
	void					applyRadiusDamage_immediate(struct DamageEvent& damageEvent);

	int32_t			rayCast(float& time, PxVec3& normal, const PxVec3& worldRayOrig, const PxVec3& worldRayDir, const DestructibleActorRaycastFlags::Enum flags, int32_t parentChunkIndex = ModuleDestructibleConst::INVALID_CHUNK_INDEX) const
	{
		PX_ASSERT(worldRayOrig.isFinite() && worldRayDir.isFinite());

		PxVec3 worldBoxCenter = worldRayOrig;
		PxVec3 worldBoxExtents = PxVec3(0.0f, 0.0f, 0.0f);
		PxMat33 worldBoxRT = PxMat33(PxIdentity);
		int32_t chunk;
		if (flags & DestructibleActorRaycastFlags::DynamicChunks)
		{
			chunk = pointOrOBBSweep(time, normal, worldBoxCenter, worldBoxExtents, worldBoxRT, worldRayDir, flags, parentChunkIndex);
		}
		else
		{
			chunk = pointOrOBBSweepStatic(time, normal, worldBoxCenter, worldBoxExtents, worldBoxRT, worldRayDir, flags, parentChunkIndex);
		}
#if APEX_RUNTIME_FRACTURE
		if (mRTActor != NULL)
		{
			float rtTime = PX_MAX_F32;
			// TODO: Refactor the runtime actor raycasting to use a more sensible approach
			//  (Perhaps we can re-use some of the existing pointOrOBBSweep code
			if (mRTActor->rayCast(worldRayOrig, worldRayDir, rtTime) &&
				(chunk == ModuleDestructibleConst::INVALID_CHUNK_INDEX || rtTime < time))
			{
				time = rtTime;
				chunk = 0;
			}
		}
#endif
		return chunk;
	}

	int32_t			obbSweep(float& time, PxVec3& normal, const PxVec3& worldBoxCenter, const PxVec3& worldBoxExtents, const PxMat33& worldBoxRT, const PxVec3& worldDisplacement, DestructibleActorRaycastFlags::Enum flags) const
	{
		if (flags & DestructibleActorRaycastFlags::DynamicChunks)
		{
			return pointOrOBBSweep(time, normal, worldBoxCenter, worldBoxExtents, worldBoxRT, worldDisplacement, flags, ModuleDestructibleConst::INVALID_CHUNK_INDEX);
		}
		else
		{
			return pointOrOBBSweepStatic(time, normal, worldBoxCenter, worldBoxExtents, worldBoxRT, worldDisplacement, flags, ModuleDestructibleConst::INVALID_CHUNK_INDEX);
		}
	}

	void					setActorObjDescFlags(class PhysXObjectDescIntl* actorObjDesc, uint32_t depth) const;

	void					fillInstanceBuffers();
	void					setRenderTMs(bool processChunkPoseForSyncing = false);
	void					setRelativeTMs();

	virtual bool			recreateApexEmitter(DestructibleEmitterType::Enum type);
	bool					initCrumbleSystem(const char* crumbleEmitterName);
	bool					initDustSystem(const char* dustEmitterName);
	bool					initFracturePattern(const char* fracturePatternName);

	virtual void			preSerialize(void *userData = NULL);

	virtual void				setPreferredRenderVolume(RenderVolume* volume, DestructibleEmitterType::Enum type);
	virtual EmitterActor*	getApexEmitter(DestructibleEmitterType::Enum type);

	void					spawnParticles(class EmitterActor* emitter, UserChunkParticleReport* report, DestructibleStructure::Chunk& chunk, physx::Array<PxVec3>& positions, bool deriveVelocitiesFromChunk = false, const PxVec3* overrideVelocity = NULL);

	void					setDeleteFracturedChunks(bool inDeleteChunkMode);

	bool					useDamageColoring()
	{
		return mUseDamageColoring;
	}

	/**
		Return ture means these core data of Damage Event should be saved for the damage coloring
	*/
	bool					applyDamageColoring(uint16_t indexInAsset, const PxVec3& position, float damage, float damageRadius);
	bool					applyDamageColoringRecursive(uint16_t indexInAsset, const PxVec3& position, float damage, float damageRadius);

	void					fillBehaviorGroupDesc(DestructibleBehaviorGroupDesc& behaviorGroupDesc, const DestructibleActorParamNS::BehaviorGroup_Type behaviorGroup) const;

	/*** Public behavior group functions ***/
	uint32_t			getCustomBehaviorGroupCount() const
	{
		return (uint32_t)mParams->behaviorGroups.arraySizes[0];
	}

	bool					getBehaviorGroup(DestructibleBehaviorGroupDesc& behaviorGroupDesc, int32_t index) const
	{
		if (index == -1)
		{
			fillBehaviorGroupDesc(behaviorGroupDesc, mParams->defaultBehaviorGroup);
			return true;
		}
		if (index >= 0 && index < (int32_t)mParams->behaviorGroups.arraySizes[0])
		{
			fillBehaviorGroupDesc(behaviorGroupDesc, mParams->behaviorGroups.buf[index]);
			return true;
		}
		return false;
	}

	/*** DestructibleHitChunk operations ***/
	bool					setHitChunkTrackingParams(bool flushHistory, bool startTracking, uint32_t trackingDepth, bool trackAllChunks = true);
	bool					getHitChunkHistory(const DestructibleHitChunk *& hitChunkContainer, uint32_t & hitChunkCount) const;
	bool					forceChunkHits(const DestructibleHitChunk * hitChunkContainer, uint32_t hitChunkCount, bool removeChunks = true, bool deferredEvent = false, PxVec3 damagePosition = PxVec3(0.0f), PxVec3 damageDirection = PxVec3(0.0f));
	void					evaluateForHitChunkList(const FractureEvent & fractureEvent);

	struct CachedHitChunk : public DestructibleHitChunk
	{
		CachedHitChunk(uint32_t chunkIndex_, uint32_t hitChunkFlags_)
		{
#if defined WIN32
			extern char enforce[sizeof(*this) == sizeof(DestructibleHitChunk)?1:-1];
#endif // WIN32
			DestructibleHitChunk::chunkIndex = chunkIndex_;
			DestructibleHitChunk::hitChunkFlags = hitChunkFlags_;
		}
		~CachedHitChunk() {}
	private:
		CachedHitChunk();
	};

	/*** Damage Coloring ***/
	bool					getDamageColoringHistory(const DamageEventCoreData *& damageEventCoreDataContainer, uint32_t & damageEventCoreDataCount) const;
	bool					forceDamageColoring(const DamageEventCoreData * damageEventCoreDataContainer, uint32_t damageEventCoreDataCount);
	void					collectDamageColoring(const int32_t indexInAsset, const PxVec3& position, const float damage, const float damageRadius);
	void					applyDamageColoring_immediate(const int32_t indexInAsset, const PxVec3& position, const float damage, const float damageRadius);

	struct CachedDamageEventCoreData : public DamageEventCoreData
	{
		CachedDamageEventCoreData(int32_t chunkIndex_, PxVec3 position_, float damage_, float radius_)
		{
#if defined WIN32
			extern char enforce[sizeof(*this) == sizeof(DamageEventCoreData)?1:-1];
#endif // WIN32			
			DamageEventCoreData::chunkIndexInAsset = chunkIndex_;
			DamageEventCoreData::position = position_;
			DamageEventCoreData::damage = damage_;
			DamageEventCoreData::radius = radius_;
		}
		~CachedDamageEventCoreData() {}
	private:
		CachedDamageEventCoreData();
	};

private:
	struct HitChunkParams
	{
	public:
		HitChunkParams():cacheChunkHits(false),cacheAllChunks(false),trackingDepth(0)
		{
			manualFractureEventInstance.position			= PxVec3(0.0f);
			manualFractureEventInstance.impulse				= PxVec3(0.0f);
			manualFractureEventInstance.hitDirection		= PxVec3(0.0f);
		}
		~HitChunkParams() {}
		bool									cacheChunkHits;
		bool									cacheAllChunks;
		uint32_t							trackingDepth;
		physx::Array<CachedHitChunk>			hitChunkContainer;
		FractureEvent							manualFractureEventInstance;
	}											hitChunkParams;

	/*** structure for damage coloring ***/
	struct DamageColoringParams
	{
		physx::Array<CachedDamageEventCoreData>			damageEventCoreDataContainer;
		SyncDamageEventCoreDataParams					damageEventCoreDataInstance;
	} damageColoringParams;


    /*** DestructibleActor::SyncParams ***/
public:
    bool setSyncParams(uint32_t userActorID, uint32_t actorSyncFlags, const DestructibleActorSyncState * actorSyncState, const DestructibleChunkSyncState * chunkSyncState);
public:
    class SyncParams
    {
		friend bool DestructibleActorImpl::setSyncParams(uint32_t, uint32_t, const DestructibleActorSyncState *, const DestructibleChunkSyncState *);
    public:
        SyncParams();
        ~SyncParams();
		uint32_t							getUserActorID() const;
		bool									isSyncFlagSet(DestructibleActorSyncFlags::Enum flag) const;
		const DestructibleActorSyncState *	getActorSyncState() const;
		const DestructibleChunkSyncState *	getChunkSyncState() const;

		void									pushDamageBufferIndex(uint32_t index);
		void									pushFractureBufferIndex(uint32_t index);
		void									pushCachedChunkTransform(const CachedChunk & cachedChunk);

		const physx::Array<uint32_t> &		getDamageBufferIndices() const;
		const physx::Array<uint32_t> &		getFractureBufferIndices() const;
		const physx::Array<CachedChunk> &		getCachedChunkTransforms() const;

		template<typename Unit> void			clear();
		template<typename Unit> uint32_t	getCount() const;
	private:
		DECLARE_DISABLE_COPY_AND_ASSIGN(SyncParams);
		void									onReset();
		uint32_t							userActorID;
        uint32_t							actorSyncFlags;
		bool									useActorSyncState;
		DestructibleActorSyncState			actorSyncState;
		bool									useChunkSyncState;
		DestructibleChunkSyncState			chunkSyncState;

		physx::Array<uint32_t>				damageBufferIndices;
		physx::Array<uint32_t>				fractureBufferIndices;
		physx::Array<CachedChunk>				cachedChunkTransforms;
    };

	const DestructibleActorImpl::SyncParams &		getSyncParams() const;
	DestructibleActorImpl::SyncParams &				getSyncParamsMutable();
private:
	SyncParams									mSyncParams;

private:
	DestructibleActorImpl(DestructibleActor* _api, DestructibleAssetImpl& _asset, DestructibleScene& scene);
	virtual					~DestructibleActorImpl();

	void					initialize(NvParameterized::Interface* stateOrParams);
	void					initializeFromState(NvParameterized::Interface* state);
	void					initializeFromParams(NvParameterized::Interface* params);
	void					initializeCommon(void);
	void					createRenderable(void);
	void					initializeActor(void);
	void					initializeRTActor(void);
	void					initializeEmitters(void);
	void					deinitialize(void);

	void					setDestructibleParameters(const DestructibleActorParamNS::DestructibleParameters_Type& destructibleParameters,
	                                                  const DestructibleActorParamNS::DestructibleDepthParameters_DynamicArray1D_Type& destructibleDepthParameters);

	int32_t			pointOrOBBSweep(float& time, PxVec3& normal, const PxVec3& worldBoxCenter, const PxVec3& worldBoxExtents, const PxMat33& worldBoxRT, const PxVec3& worldDisp,
	                                        DestructibleActorRaycastFlags::Enum flags, int32_t parentChunkIndex) const;

	int32_t			pointOrOBBSweepStatic(float& time, PxVec3& normal, const PxVec3& worldBoxCenter, const PxVec3& worldBoxExtents, const PxMat33& worldBoxRT, const PxVec3& pxWorldDisp,
											DestructibleActorRaycastFlags::Enum flags, int32_t parentChunkIndex) const;

	void					wakeUp(void);
	void					putToSleep(void);

	// Renderable support:
public:
	Renderable*			getRenderable()
	{
		return static_cast<Renderable*>(mRenderable);
	}
	DestructibleRenderable*	acquireRenderableReference();
	RenderMeshActor*			getRenderMeshActor(DestructibleActorMeshType::Enum type = DestructibleActorMeshType::Skinned) const;

private:

	DestructibleActorState*		mState;					// Destructible asset state, contains data for serialization

	// Cached parameters
	DestructibleActorParam*		mParams;				// Destructible actor params, this is just a convenient reference to the params in mState
	DestructibleActorChunks*	mChunks;				// Destructible actor chunks, this is just a convenient reference to the chunks in mState

	// Read-write cached parameters (require writeback on preSerialization)
	DestructibleParameters	mDestructibleParameters;
	PxMat44				mTM;
	PxMat44				mRelTM;					// Relative transform between the actor 

	// Derived parameters
	IndexBank<uint16_t>	mStaticRoots;
	IndexBank<uint16_t>	mVisibleChunks;
	uint32_t				mVisibleDynamicChunkShapeCount;
	uint32_t				mEssentialVisibleDynamicChunkShapeCount;
	DestructibleScene*			mDestructibleScene;
	DestructibleActor*		mAPI;
	uint16_t				mFlags;
	uint16_t				mInternalFlags;			// Internal flags, currently indicates whether the actor has been marked as being part of an 'island' or not.
	DestructibleStructure*		mStructure;				// The destructible structure that this actor is a member of.  Multiple destructible actors can belong to a single destructible structure.
	DestructibleAssetImpl*			mAsset;					// The asset associated with this actor.
	uint32_t				mID;						// A unique 32 bit GUID given to this actor.
	float				mLinearSize;
	PxBounds3			mOriginalBounds;
	PxBounds3			mNonInstancedBounds;	// Recording this separately, since instanced buffers get updated separately
	PxBounds3			mInstancedBounds;		// Recording this separately, since instanced buffers get updated separately
	uint32_t				mFirstChunkIndex;		// This first chunk index used from the destructible structure
	EmitterActor*			mCrumbleEmitter;		// The crumble emitter associated with this actor.
	EmitterActor*			mDustEmitter;			// The dust emitter associated with this actor.
	RenderVolume*			mCrumbleRenderVolume;	// The render volume to use for crumble effects.
	RenderVolume*			mDustRenderVolume;		// The render volume to use for dust effects.
	float				mStartTime;				// Time the actor became "alive."
	float				mBenefit;
	bool						mInitializedFromState;			// Whether the actor was initialized from state
	physx::Array<DestructibleChunkEvent>	mChunkEventBuffer;
	nvidia::Mutex				mChunkEventBufferLock;

	physx::Array<physx::PxRigidDynamic*>	mPhysXActorBuffer;

	nvidia::Mutex				mPhysXActorBufferLock;

	DestructibleRenderableImpl*		mRenderable;

	physx::Array< physx::Array<ColorRGBA> >	mDamageColorArrays;
	bool						mUseDamageColoring;

	uint32_t				mDescOverrideSkinnedMaterialCount;
	const char**				mDescOverrideSkinnedMaterials;
	uint32_t				mDescOverrideStaticMaterialCount;
	const char** 				mDescOverrideStaticMaterials;
	bool						mPhysXActorBufferAcquired;
public:
	bool						mInDeleteChunkMode;

	uint32_t				mAwakeActorCount;	// number of awake PxActors.
	uint32_t				mActiveFrames;		// Bit representation of active frame history.  Only used when mDestructibleScene->mUsingActiveTransforms == true

	uint32_t				mDamageEventReportIndex;

#if USE_DESTRUCTIBLE_RWLOCK
	shdfnd::ReadWriteLock*		mLock;
#endif

#if APEX_RUNTIME_FRACTURE
	::nvidia::fracture::Actor*			mRTActor;
#endif

	bool						mWakeForEvent;

	physx::Array<PxRigidDynamic*>		mReferencingActors;

public:
	void						referencedByActor(PxRigidDynamic* actor);
	void						unreferencedByActor(PxRigidDynamic* actor);

	PxRigidDynamic**					getReferencingActors(uint32_t& count)
	{
		count = mReferencingActors.size();
		return count ? &mReferencingActors[0] : NULL;
	}

	void				wakeForEvent();
	void				resetWakeForEvent();

public:

	PxConvexMesh*				getConvexMesh(uint32_t hullIndex)
	{
		if (mAsset->mCollisionMeshes == NULL)
		{
			mAsset->mCollisionMeshes = mAsset->module->mCachedData->getConvexMeshesForScale(*mAsset, mAsset->module->getChunkCollisionHullCookingScale());
			PX_ASSERT(mAsset->mCollisionMeshes != NULL);
			if (mAsset->mCollisionMeshes == NULL)
			{
				return NULL;
			}
		}
		return (*mAsset->mCollisionMeshes)[hullIndex];
	}
};


struct DamageEvent : public DamageEventCoreData
{
	// Default constructor for a damage event.
	DamageEvent() :
		destructibleID((uint32_t)DestructibleActorImpl::InvalidID),
		momentum(0.0f), 
		direction(PxVec3(0.0f, 0.0f, 1.0f)),
		flags(0), 
		impactDamageActor(NULL),
		appliedDamageUserData(NULL),
		minDepth(0), 
		maxDepth(0), 
		processDepth(0)
	{
		DamageEventCoreData::chunkIndexInAsset = 0;
		DamageEventCoreData::damage = 0.0f;
		DamageEventCoreData::radius = 0.0f;
		DamageEventCoreData::position = PxVec3(0.0f);

		for (uint32_t i = 0; i <= MaxDepth; ++i)
		{
			cost[i] = benefit[i] = 0;
			new(fractures + i) physx::Array<FractureEvent>();
		}
	}

	// Copy constructor for a damage event.
	DamageEvent(const DamageEvent& that)
	{
		destructibleID = that.destructibleID;
		damage = that.damage;
		momentum = that.momentum;
		radius = that.radius;
		position = that.position;
		direction = that.direction;
		chunkIndexInAsset = that.chunkIndexInAsset;
		flags = that.flags;

		minDepth = that.minDepth;
		maxDepth = that.maxDepth;
		processDepth = that.processDepth;

		impactDamageActor = that.impactDamageActor;
		appliedDamageUserData = that.appliedDamageUserData;

		for (uint32_t i = 0; i < MaxDepth + 1; i++)
		{
			cost[i] = that.cost[i];
			benefit[i] = that.benefit[i];
			fractures[i] = that.fractures[i];
		}
	}

	~DamageEvent()
	{
		for (uint32_t i = MaxDepth + 1; i--;)
		{
			fractures[i].reset();
		}
	}

	// Deep copy of one damage event to another.
	DamageEvent&	operator=(const DamageEvent& that)
	{
		if (this != &that)
		{
			destructibleID = that.destructibleID;
			damage = that.damage;
			momentum = that.momentum;
			radius = that.radius;
			position = that.position;
			direction = that.direction;
			chunkIndexInAsset = that.chunkIndexInAsset;
			flags = that.flags;

			minDepth = that.minDepth;
			maxDepth = that.maxDepth;
			processDepth = that.processDepth;

			impactDamageActor = that.impactDamageActor;
			appliedDamageUserData = that.appliedDamageUserData;

			for (uint32_t i = 0; i < MaxDepth + 1; i++)
			{
				cost[i] = that.cost[i];
				benefit[i] = that.benefit[i];
				fractures[i] = that.fractures[i];
			}
		}
		return *this;

	}

	enum Flag
	{
		UseRadius =				(1U << 0),			// Indicates whether or not to process the damage event as a radius based effect
		HasFalloff =			(1U << 1),			// Indicates whether or not the damage amount (for radius damage) falls off with distance from the impact.
		IsFromImpact =			(1U << 2),			// Indicates that this is an impact event, where damage is applied to specific chunk rather than radius based damage.
		SyncDirect =			(1U << 3),			// Indicates whether this is a sync-ed damage event
		DeleteChunkModeUnused =	(1U << 30),			// Indicates whether or not to delete the chunk instead of breaking it off. Propagates down to FractureEvent.

		Invalid =				(1U << 31)			// Indicates that this event is invalid (can occur when a destructible is removed)
	};

	uint32_t	destructibleID;			// The ID of the destructible actor that is being damaged.
	float	momentum;				// The inherited momentum of the damage event.
	PxVec3	direction;				// The direction of the damage event.
	uint32_t	flags;					// Flags which indicate whether this damage event is radius based and if we use a fall off computation for the amount of damage.
	physx::PxActor const* impactDamageActor;// Other PhysX actor that caused damage to ApexDamageEventReportData.

	void*			appliedDamageUserData;	// User data from applyDamage or applyRadiusDamage.

	enum
	{
		MaxDepth	= 5
	};

	PX_INLINE	uint32_t	getMinDepth() const
	{
		return minDepth;
	}
	PX_INLINE	uint32_t	getMaxDepth() const
	{
		return maxDepth;
	}
	PX_INLINE	uint32_t	getProcessDepth() const
	{
		return processDepth;
	}
	PX_INLINE	float	getCost(uint32_t depth = 0xFFFFFFFF) const
	{
		return cost[depth <= MaxDepth ? depth : processDepth];
	}
	PX_INLINE	float	getBenefit(uint32_t depth = 0xFFFFFFFF) const
	{
		return benefit[depth <= MaxDepth ? depth : processDepth];
	}
	PX_INLINE bool isFromImpact(void) const
	{
		return flags & IsFromImpact ? true : false;
	};

	void resetFracturesInternal()
	{
		for(uint32_t index = DamageEvent::MaxDepth + 1; index--; )
		{
			fractures[index].reset();
		}
	}
private:
	uint32_t				minDepth;					// The minimum structure depth that this damage event can apply to.
	uint32_t				maxDepth;					// The maximum structure depth that this damage event can apply to.
	uint32_t				processDepth;				// The exact depth that the damage event applies to.
	float				cost[MaxDepth + 1];			// The LOD 'cost' at each depth for this damage event.
	float				benefit[MaxDepth + 1];		// The LOD 'benefit' at each depth for this damage event.
	physx::Array<FractureEvent>	fractures[MaxDepth + 1];	// The array of fracture events associated with this damage event.

	friend class DestructibleScene;
	friend class DestructibleActorImpl;
};

#if USE_DESTRUCTIBLE_RWLOCK
class DestructibleScopedReadLock : public physx::ScopedReadLock
{
public:
	DestructibleScopedReadLock(DestructibleActorImpl& destructible) : physx::ScopedReadLock(*destructible.mLock) {}
};

class DestructibleScopedWriteLock : public physx::ScopedWriteLock
{
public:
	DestructibleScopedWriteLock(DestructibleActorImpl& destructible) : physx::ScopedWriteLock(*destructible.mLock) {}
};
#endif

}
} // end namespace nvidia

#endif // __DESTRUCTIBLEACTOR_IMPL_H__
