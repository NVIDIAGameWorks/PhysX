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



#ifndef __DESTRUCTIBLE_SCENE_H__
#define __DESTRUCTIBLE_SCENE_H__

#include "Apex.h"
#include "SceneIntl.h"
#include "ModuleIntl.h"
#include "DestructibleActorImpl.h"
#include "DestructibleAssetImpl.h"
#include "DestructibleStructure.h"
#include "ModuleDestructibleImpl.h"
#include "PsHashMap.h"
#include "PsHashSet.h"

#include <PxSimulationEventCallback.h>
#include <PxContactModifyCallback.h>

#include "DestructibleActorJoint.h"

#include "RenderDebugInterface.h"

#include "DebugRenderParams.h"
#include "DestructibleDebugRenderParams.h"

#include "PsTime.h"
#include "PsMutex.h"
#include "ApexContext.h"

#include "PxTaskManager.h"
#include "PxTask.h"

#if APEX_RUNTIME_FRACTURE
#include "SimScene.h"
#endif

#include "ModulePerfScope.h"

// We currently have to copy into the contact buffer because the native contact
// stream is compressed within PhysX.  If we need to optimize we could use a
// contact iterator like what's found in PxContactPair::extractContacts()
#define USE_EXTRACT_CONTACTS						1
#define PAIR_POINT_ALLOCS							0

#ifndef PX_VERIFY
#ifndef NDEBUG
#define PX_VERIFY(f) PX_ASSERT(f)
#else   // NDEBUG
#define PX_VERIFY(f) ((void)(f))
#endif
#endif

namespace nvidia
{
namespace destructible
{

static const int32_t MAX_SHAPE_COUNT = 256;

class ModuleDestructibleImpl;
class DestructibleScene;
class DestructibleActorProxy;
class DestructibleBeforeTick;

struct IndexedReal
{
	float	value;
	uint32_t	index;

	PX_INLINE bool	operator > (IndexedReal& i)
	{
		return value > i.value;
	}
};


class DestructibleContactModify : public PxContactModifyCallback
{
public:
	DestructibleContactModify() : destructibleScene(NULL) {}

	void onContactModify(physx::PxContactModifyPair* const pairs, uint32_t count);

	DestructibleScene*		destructibleScene;
};

class DestructibleUserNotify : public PxSimulationEventCallback
{
public:
	DestructibleUserNotify(ModuleDestructibleImpl& module, DestructibleScene* destructibleScene);

private:
	virtual void onConstraintBreak(physx::PxConstraintInfo* constraints, uint32_t count);
	virtual void onWake(PxActor** actors, uint32_t count);
	virtual void onSleep(PxActor** actors, uint32_t count);
	virtual void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, uint32_t nbPairs);
	virtual void onTrigger(physx::PxTriggerPair* pairs, uint32_t count);
	virtual void onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count);
		
	void operator=(const DestructibleUserNotify&) {}

private:
	ModuleDestructibleImpl&			mModule;
	DestructibleScene*			mDestructibleScene;
#if USE_EXTRACT_CONTACTS
	physx::Array<uint8_t>	mPairPointBuffer;
#endif
};


class ApexDamageEventReportDataImpl : public DamageEventReportData
{
public:
	ApexDamageEventReportDataImpl()
	{
		clear();
	}

	ApexDamageEventReportDataImpl(const ApexDamageEventReportDataImpl& other)
	{
		*this = other;
	}

	ApexDamageEventReportDataImpl&	operator = (const ApexDamageEventReportDataImpl& other)
	{
		m_destructible = other.m_destructible;
		m_fractureEvents = other.m_fractureEvents;
		m_chunkReportBitMask = other.m_chunkReportBitMask;
		m_chunkReportMaxFractureEventDepth = other.m_chunkReportMaxFractureEventDepth;
		destructible = other.destructible;
		hitDirection = other.hitDirection;
		worldBounds = other.worldBounds;
		totalNumberOfFractureEvents = other.totalNumberOfFractureEvents;
		minDepth = other.minDepth;
		maxDepth = other.maxDepth;
		fractureEventList = m_fractureEvents.size() > 0 ? &m_fractureEvents[0] : NULL;
		fractureEventListSize = m_fractureEvents.size();
		impactDamageActor = other.impactDamageActor;
		appliedDamageUserData = other.appliedDamageUserData;
		hitPosition = other.hitPosition;
		return *this;
	}

	void						setDestructible(DestructibleActorImpl* inDestructible);

	uint32_t				addFractureEvent(const DestructibleStructure::Chunk& chunk, uint32_t flags);

	ChunkData&			getFractureEvent(uint32_t index)
	{
		return m_fractureEvents[index];
	}

	void						clearChunkReports();

private:
	void						clear()
	{
		m_destructible = NULL;
		m_fractureEvents.reset();
		m_chunkReportBitMask = 0xffffffff;
		m_chunkReportMaxFractureEventDepth = 0xffffffff;
		destructible = NULL;
		hitDirection = PxVec3(0.0f);
		worldBounds.setEmpty();
		totalNumberOfFractureEvents = 0;
		minDepth = 0xffff;
		maxDepth = 0;
		fractureEventList = NULL;
		fractureEventListSize = 0;
		impactDamageActor = NULL;
		appliedDamageUserData = NULL;
		hitPosition = PxVec3(0.0f);
	}

	DestructibleActorImpl*				m_destructible;
	physx::Array<ChunkData>	m_fractureEvents;
	uint32_t					m_chunkReportBitMask;
	uint32_t					m_chunkReportMaxFractureEventDepth;
};

struct ActorFIFOEntry
{
	enum Flag
	{
		IsDebris =			1 << 0,
		ForceLODRemove =	1 << 1,

		MassUpdateNeeded =	1 << 8,
	};

	ActorFIFOEntry() : actor(NULL), origin(0.0f), age(0.0f), benefitCache(PX_MAX_F32), maxSpeed(0.0f), averagedLinearVelocity(0.0f), averagedAngularVelocity(0.0f), unscaledMass(0.0f), flags(0) {}
	ActorFIFOEntry(PxRigidDynamic* inActor, float inUnscaledMass, uint32_t inFlags) : actor(inActor), age(0.0f), benefitCache(PX_MAX_F32), maxSpeed(0.0f),
		averagedLinearVelocity(0.0f), averagedAngularVelocity(0.0f), unscaledMass(inUnscaledMass), flags(inFlags)
	{
		SCOPED_PHYSX_LOCK_READ(actor->getScene());
		origin = (actor->getGlobalPose() * actor->getCMassLocalPose()).p;
	}

	PxRigidDynamic*		actor;
	PxVec3	origin;
	float	age;
	float	benefitCache;	//for LOD
	float	maxSpeed;
	PxVec3	averagedLinearVelocity;
	PxVec3	averagedAngularVelocity;
	float	unscaledMass;
	uint32_t	flags;
};

struct DormantActorEntry
{
	DormantActorEntry() : actor(NULL), unscaledMass(0.0f), flags(0) {}
	DormantActorEntry(PxRigidDynamic* inActor, float inUnscaledMass, uint32_t inFlags) : actor(inActor), unscaledMass(inUnscaledMass), flags(inFlags) {}

	PxRigidDynamic*		actor;
	float	unscaledMass;
	uint32_t	flags;
};

struct ChunkBenefitCoefs
{
	ChunkBenefitCoefs() : perChunkInitialBenefit(1.0f), benefitDecayTimeConstant(1.0f), benefitPerPercentScreenArea(1.0f) {}

	float	perChunkInitialBenefit;
	float	benefitDecayTimeConstant;
	float	benefitPerPercentScreenArea;
};

struct ChunkCostCoefs
{
	ChunkCostCoefs() : perChunkCost(1.0f) {}

	float	perChunkCost;
};


/* Class which manages DestructibleActors and DestructibleStructures
 * associate with a single Scene
 */

class DestructibleScene : public ModuleSceneIntl, public ApexContext, public ApexResourceInterface, public ApexResource
{
public:
	DestructibleScene(ModuleDestructibleImpl& module, SceneIntl& scene, RenderDebugInterface* debugRender, ResourceList& list);
	~DestructibleScene();

	/* ModuleSceneIntl */
	virtual void				submitTasks(float elapsedTime, float substepSize, uint32_t numSubSteps);
	virtual void				setTaskDependencies();
	virtual void				tasked_beforeTick(float elapsedTime);
	virtual void				fetchResults();

	virtual void				setModulePhysXScene(PxScene* s);
	virtual PxScene*			getModulePhysXScene() const
	{
		return mPhysXScene;
	}
	virtual void				release()
	{
		mModule->releaseModuleSceneIntl(*this);
	}
	virtual void				visualize();

	virtual Module*			getModule()
	{
		return mModule;
	}

	ModuleDestructibleImpl*			getModuleDestructible() const
	{
		return mModule;
	}

	SceneIntl*				getApexScene()
	{
		return mApexScene;
	}

	void						initModuleSettings();

#if APEX_RUNTIME_FRACTURE
	::nvidia::fracture::SimScene*		getDestructibleRTScene(bool create = true);
#endif

	enum StatsDataEnum
	{
		VisibleDestructibleChunkCount,
		DynamicDestructibleChunkIslandCount,
		NumberOfShapes,
		NumberOfAwakeShapes,
		RbThroughput,

		// insert new items before this line
		NumberOfStats			// The number of stats
	};

	virtual SceneStats* getStats();

	/* ApexResourceInterface */
	uint32_t				getListIndex() const
	{
		return m_listIndex;
	}
	void						setListIndex(ResourceList& list, uint32_t index)
	{
		m_listIndex = index;
		m_list = &list;
	}

	/* DestructibleScene methods */

	void						applyRadiusDamage(float damage, float momentum, const PxVec3& position, float radius, bool falloff);

	bool						isActorCreationRateExceeded();
	bool						isFractureBufferProcessRateExceeded();

	void						addToAwakeList(DestructibleActorImpl& actor);
	void						removeFromAwakeList(DestructibleActorImpl& actor);

	enum
	{
		InvalidReportID =		0xFFFFFFFF
	};

private:

	void						addActorsToScene();
	physx::Array<physx::PxActor*>	mActorsToAdd;
	HashMap<physx::PxActor*, uint32_t>	mActorsToAddIndexMap;

	void addForceToAddActorsMap(physx::PxActor* actor, const ActorForceAtPosition& force);
	HashMap<physx::PxActor*, ActorForceAtPosition> mForcesToAddToActorsMap;

	ModuleDestructibleImpl*			mModule;
	SceneIntl*				mApexScene;
	PxScene*					mPhysXScene;
	DestructibleContactModify	mContactModify;

	ResourceList				mDestructibleActorJointList;
	DestructibleBeforeTick*		mBeforeTickTask;

	DestructibleUserNotify      mUserNotify;
	SceneStats			mModuleSceneStats;
	float						mElapsedTime;

	physx::Array<DestructibleActorImpl*> mInstancedActors;

#if APEX_RUNTIME_FRACTURE
	::nvidia::fracture::SimScene*	mRTScene;
#endif

	// These values are set per-scene by the destruction module
	float				mMassScale;
	float				mMassScaleInv;
	float				mScaledMassExponent;
	float				mScaledMassExponentInv;

	int							mPreviousVisibleDestructibleChunkCount;			// Must keep previous value because of our goofy cumulative-only stats system
	int							mPreviousDynamicDestructibleChunkIslandCount;	// Must keep previous value because of our goofy cumulative-only stats system

public:
	void						destroy();
	void						reset();
	DestructibleActorJoint*	createDestructibleActorJoint(const DestructibleActorJointDesc& destructibleActorJointDesc);
	DestructibleActor*		getDestructibleAndChunk(const PxShape* shape, int32_t* chunkIndex) const;

	bool						insertDestructibleActor(DestructibleActor* destructible);

	bool						removeStructure(DestructibleStructure* structure, bool immediate = true);
	void						addActor(PhysXObjectDescIntl& desc, PxRigidDynamic& actor, float unscaledMass, bool isDebris);
	bool						destroyActorChunks(PxRigidDynamic& actor, uint32_t chunkFlag);
	void						capDynamicActorCount();
	void						removeReferencesToActor(DestructibleActorImpl& actor);

	void						addInstancedActor(DestructibleActorImpl* actor)
	{
		mInstancedActors.pushBack(actor);
	}

	void						releasePhysXActor(PxRigidDynamic& actor);

	void						resetEmitterActors();

	PxRigidDynamic*				createRoot(DestructibleStructure::Chunk& chunk, const PxTransform& pose, bool dynamic, PxTransform* relTM = NULL, bool fromInitialData = false);
	bool						appendShapes(DestructibleStructure::Chunk& chunk, bool dynamic, PxTransform* relTM = NULL, PxRigidDynamic* actor = NULL);

	void						setWorldSupportPhysXScene(PxScene* physxScene)
	{
		m_worldSupportPhysXScene = physxScene;
	}

	bool						testWorldOverlap(const ConvexHullImpl& convexHull, const PxTransform& tm, const PxVec3& scale, float padding, const physx::PxFilterData* groupsMask = NULL);

	DestructibleStructure*			getChunkStructure(DestructibleStructure::Chunk& chunk) const
	{
		return mDestructibles.direct(chunk.destructibleID)->getStructure();
	}

	DestructibleStructure::Chunk*	getChunk(const PxShape* shape) const
	{
		// TODO: this fires all the time with PhysX3
		//PX_ASSERT(mModule->owns((PxActor*)&shape->getActor()));
		const PxRigidActor* actor = shape->getActor();

		if (!mModule->owns(actor))
		{
			return NULL;
		}

		const PhysXObjectDesc* shapeDesc = mModule->mSdk->getPhysXObjectInfo(shape);
		if (shapeDesc == NULL)
		{
			return NULL;
		}

		return (DestructibleStructure::Chunk*)shapeDesc->userData;
	}

	const DestructibleActorImpl*		getChunkActor(const DestructibleStructure::Chunk& chunk) const
	{
		return mDestructibles.direct(chunk.destructibleID);
	}

	PX_INLINE void					createChunkReportDataForFractureEvent(const FractureEvent& fractureEvent,  DestructibleActorImpl* destructible, DestructibleStructure::Chunk& chunk)
	{
		PX_UNUSED(fractureEvent);	// Keeping this parameter assuming we'll merge in a later change which requires fractureEvent
		PX_ASSERT(destructible != NULL && destructible->mDamageEventReportIndex < mDamageEventReportData.size());
		if (destructible != NULL && destructible->mDamageEventReportIndex < mDamageEventReportData.size())
		{
			ApexDamageEventReportDataImpl& damageReportData = mDamageEventReportData[destructible->mDamageEventReportIndex];
			const uint32_t fractureEventIndex = damageReportData.addFractureEvent(chunk, ApexChunkFlag::FRACTURED);
			mFractureEventCount++;
			chunk.reportID = mChunkReportHandles.size();
			IntPair& handle = mChunkReportHandles.insert();
			handle.set((int32_t)destructible->mDamageEventReportIndex, (int32_t)fractureEventIndex);
		}
	}

	PX_INLINE ChunkData*		getChunkReportData(DestructibleStructure::Chunk& chunk, uint32_t flags);

	PX_INLINE PxRigidDynamic*				chunkIntact(DestructibleStructure::Chunk& chunk);

	bool							scheduleChunkShapesForDelete(DestructibleStructure::Chunk& chunk);
	bool							schedulePxActorForDelete(PhysXObjectDescIntl& actorDesc);

	PX_INLINE void					calculatePotentialCostAndBenefit(float& cost, float& benefit, physx::Array<uint8_t*>& trail, physx::Array<uint8_t>& undo, const FractureEvent& fractureEvent) const;

	template <class ChunkOpClass>
	void							forSubtree(DestructibleStructure::Chunk& chunk, ChunkOpClass chunkOp, bool skipRoot = false);

	bool							setMassScaling(float massScale, float scaledMassExponent);

	void							invalidateBounds(const PxBounds3* bounds, uint32_t boundsCount);

	void							setDamageApplicationRaycastFlags(nvidia::DestructibleActorRaycastFlags::Enum flags);

	nvidia::DestructibleActorRaycastFlags::Enum	getDamageApplicationRaycastFlags() const { return (nvidia::DestructibleActorRaycastFlags::Enum)m_damageApplicationRaycastFlags; }

	PX_INLINE float			scaleMass(float mass)
	{
		if (PxAbs(mScaledMassExponent - 0.5f) < 1e-7)
		{
			return mMassScale * PxSqrt(mass * mMassScaleInv);
		}
		return mMassScale * PxPow(mass * mMassScaleInv, mScaledMassExponent);
	}

	PX_INLINE float			unscaleMass(float scaledMass)
	{
		if (PxAbs(mScaledMassExponentInv - 2.f) < 1e-7)
		{
			float tmp = scaledMass * mMassScaleInv;
			return mMassScale * tmp * tmp;
		}
		return mMassScale * PxPow(scaledMass * mMassScaleInv, mScaledMassExponentInv);
	}

	void							updateActorPose(DestructibleActorImpl* actor, UserChunkMotionHandler* callback)
	{
		const bool processChunkPoseForSyncing = ((NULL != callback) && (	actor->getSyncParams().isSyncFlagSet(DestructibleActorSyncFlags::CopyChunkTransform) ||
																			actor->getSyncParams().isSyncFlagSet(DestructibleActorSyncFlags::ReadChunkTransform)));
        actor->setRenderTMs(processChunkPoseForSyncing);
	}

	bool							setRenderLockMode(RenderLockMode::Enum renderLockMode)
	{
		if ((int)renderLockMode < 0 || (int)renderLockMode > RenderLockMode::PER_MODULE_SCENE_RENDER_LOCK)
		{
			return false;
		}

		mRenderLockMode = renderLockMode;

		return true;
	}

	RenderLockMode::Enum		getRenderLockMode() const
	{
		return mRenderLockMode;
	}

	bool							lockModuleSceneRenderLock()
	{
		return mRenderDataLock.lock();
	}

	bool							unlockModuleSceneRenderLock()
	{
		return mRenderDataLock.unlock();
	}

	bool							lockRenderResources()
	{
		switch (getRenderLockMode())
		{
		case RenderLockMode::PER_ACTOR_RENDER_LOCK:
			renderLockAllActors();
			return true;
		case RenderLockMode::PER_MODULE_SCENE_RENDER_LOCK:
			return lockModuleSceneRenderLock();
		case RenderLockMode::NO_RENDER_LOCK:
			break;
		}
		return true;
	}

	bool							unlockRenderResources()
	{
		switch (getRenderLockMode())
		{
		case RenderLockMode::PER_ACTOR_RENDER_LOCK:
			renderUnLockAllActors();
			return true;
		case RenderLockMode::PER_MODULE_SCENE_RENDER_LOCK:
			return unlockModuleSceneRenderLock();
		case RenderLockMode::NO_RENDER_LOCK:
			break;
		}
		return true;
	}

	physx::Array<ApexDamageEventReportDataImpl>	mDamageEventReportData;
	physx::Array<IntPair>					mChunkReportHandles;	// i0 = m_damageEventReportData index, i1 = ApexDamageEventReportDataImpl::m_fractureEvents

	physx::Array<ImpactDamageEventData>	mImpactDamageEventData;	// Impact damage notify

	physx::Array<DestructibleActorImpl*>		mActorsWithChunkStateEvents;	// Filled when module->m_chunkStateEventCallbackSchedule is not DestructibleCallbackSchedule::Disabled

	void setStructureSupportRebuild(DestructibleStructure* structure, bool rebuild);
	void setStructureUpdate(DestructibleStructure* structure, bool update);
	void setStressSolverTick(DestructibleStructure* structure, bool update);

	// FIFO
	physx::Array<ActorFIFOEntry>	mActorFIFO;
	physx::Array<IndexedReal>		mActorBenefitSortArray;
	uint32_t					mDynamicActorFIFONum;	// Tracks the valid entries in actorFIFO;
	uint32_t					mTotalChunkCount;		// Tracks the chunks;

	uint32_t					mNumFracturesProcessedThisFrame;
	uint32_t					mNumActorsCreatedThisFrame;

	uint32_t					mFractureEventCount;

	// DestructibleActors with scene-global IDs
	Bank<DestructibleActorImpl*, uint32_t>	mDestructibles;

	// Structure container
	Bank<DestructibleStructure*, uint32_t>	mStructures;
	physx::Array<DestructibleStructure*>			mStructureKillList;
	Bank<DestructibleStructure*, uint32_t>	mStructureUpdateList;
	Bank<DestructibleStructure*, uint32_t>	mStructureSupportRebuildList;
	Bank<DestructibleStructure*, uint32_t>	mStressSolverTickList;

	// For delayed release of PxRigidDynamics
	physx::Array<PxRigidDynamic*>			mActorKillList;
	ResourceList					mApexActorKillList;

	// Damage queue
	RingBuffer<DamageEvent>		mDamageBuffer[2];	// Double-buffering
	uint32_t						mDamageBufferWriteIndex;

	// Fracture queue
	RingBuffer<FractureEvent>		mFractureBuffer;

	// Destroy queue
	physx::Array<IntPair>			mChunkKillList;

	// Destructibles on death row
	physx::Array<DestructibleActor*>	destructibleActorKillList;

	// Bank of dormant (kinematic dynamic) actors
	Bank<DormantActorEntry, uint32_t>	mDormantActors;

	// list of awake destructible actor IDs (mDestructibles bank)
	IndexBank<uint32_t> mAwakeActors; 
	bool							mUsingActiveTransforms;

	// Wake/sleep event actors
	physx::Array<DestructibleActor*>	mOnWakeActors;
	physx::Array<DestructibleActor*>	mOnSleepActors;

	physx::Array<physx::PxClientID>		mSceneClientIDs;

	// Scene to use for world support calculations if NULL, mPhysXScene is used
	physx::PxScene*					m_worldSupportPhysXScene;

	physx::Array<PxBounds3>	m_invalidBounds;

	uint32_t					m_damageApplicationRaycastFlags;

	RenderDebugInterface*				mDebugRender;	//debug renderer created and owned by the owning ApexScene and passed down.  Not owned.

	DebugRenderParams*				mDebugRenderParams;
	DestructibleDebugRenderParams*	mDestructibleDebugRenderParams;

	physx::Array<physx::PxOverlapHit>	mOverlapHits;

	RenderLockMode::Enum		mRenderLockMode;

	AtomicLock				mRenderDataLock;

	// Access to the double-buffered damage events
	RingBuffer<DamageEvent>&		getDamageWriteBuffer()
	{
		return mDamageBuffer[mDamageBufferWriteIndex];
	}
	RingBuffer<DamageEvent>&		getDamageReadBuffer()
	{
		return mDamageBuffer[mDamageBufferWriteIndex^1];
	}
	void							swapDamageBuffers()
	{
		mDamageBufferWriteIndex ^= 1;
	}

	class CollectVisibleChunks
	{
		physx::Array<DestructibleStructure::Chunk*>& chunkArray;

	public:

		CollectVisibleChunks(physx::Array<DestructibleStructure::Chunk*>& inChunkArray) :
			chunkArray(inChunkArray) {}

		bool execute(DestructibleStructure* structure, DestructibleStructure::Chunk& chunk)
		{
			PX_UNUSED(structure);
			if ((chunk.state & ChunkVisible) != 0)
			{
				chunkArray.pushBack(&chunk);
				return false;
			}
			return true;
		}
	private:
		CollectVisibleChunks& operator=(const CollectVisibleChunks&);
	};

	class VisibleChunkSetDescendents
	{
		uint32_t visibleChunkIndex;
		uint8_t raiseStateFlags;

	public:
		VisibleChunkSetDescendents(uint32_t chunkIndex, bool makeDynamic) 
			: visibleChunkIndex(chunkIndex)
			, raiseStateFlags(makeDynamic ? (uint8_t)ChunkDynamic : (uint8_t)0)
		{
		}
		bool execute(DestructibleStructure* structure, DestructibleStructure::Chunk& chunk)
		{
			PX_UNUSED(structure);
			chunk.clearShapes();
			chunk.state |= raiseStateFlags;	// setting dynamic flag, but these chunks are descendents of a visible chunk and therefore invisible.  No need to update visible dynamic chunk shape counts
			chunk.visibleAncestorIndex = (int32_t)visibleChunkIndex;
			return true;
		}
	};

	class ChunkClearDescendents
	{
		int32_t rootChunkIndex;

	public:
		ChunkClearDescendents(int32_t chunkIndex) : rootChunkIndex(chunkIndex) {}

		bool execute(DestructibleStructure* structure, DestructibleStructure::Chunk& chunk)
		{
			PX_UNUSED(structure);
			if (chunk.visibleAncestorIndex == rootChunkIndex)
			{
				chunk.clearShapes();
				chunk.visibleAncestorIndex = DestructibleStructure::InvalidChunkIndex;
			}
			return true;
		}
	};

	class ChunkLoadState
	{
	public:
		ChunkLoadState(DestructibleScene& scene) : mScene(scene) { }
		bool execute(DestructibleStructure* structure, DestructibleStructure::Chunk& chunk);
	private:
		ChunkLoadState& operator=(const ChunkLoadState&);
		DestructibleScene& mScene;
	};

private:
	void createModuleStats();
	void destroyModuleStats();
	void setStatValue(int32_t index, StatValue dataVal);
	static PX_INLINE float ticksToMilliseconds(uint64_t t0, uint64_t t1)
	{
		// this is a copy of ApexScene::tickToMilliseconds. Unfortunately can't easily use the other
		static const CounterFrequencyToTensOfNanos freq = nvidia::Time::getBootCounterFrequency();
		static const double freqMultiplier = (double)freq.mNumerator/(double)freq.mDenominator * 0.00001; // convert from 10nanoseconds to milliseconds

		float ret = (float)((double)(t1 - t0) * freqMultiplier);
		return ret;
	}

	friend class DestructibleActorImpl;
	friend class DestructibleActorProxy;
	friend class DestructibleActorJointImpl;
	friend class DestructibleStructure;
	friend class DestructibleContactReport;
	friend class DestructibleContactModify;
	friend class OverlapSphereShapesReport;
	friend class ModuleDestructibleImpl;
	friend class ApexDamageEventReportDataImpl;
	friend class DestructibleUserNotify;

	/*
	<--- Destructible Sync-ables Memory Layout --->

    all actor data => buffer (consists of)
        per actor data => segment (consists of)
            1) header
                1.1) userActorID
                1.2) _X_unit count, pointer to _X_buffer start
                1.3) _Y_unit count, pointer to _Y_buffer start
                1.4) pointer to next header. NULL if last
            2) _X_buffer
                2.1) made up of '_X_count' of '_X_unit'
            3) _Y_buffer (if used)
                3.1) made up of '_Y_count' of '_Y_unit'

    |-------------|----------|---------------|----------|---------------|------|-----------|-----------|-----------------
    | userActorID | _X_count | _X_buffer_ptr | _Y_count | _Y_buffer_ptr | next | _X_buffer | _Y_buffer | userActorID  ...
    |-------------|----------|---------------|----------|---------------|------|-----------|-----------|-----------------

	<------------------------------------------------------ buffer ------------------------------------------------------...
    <-------------------------------------------- segment  -------------------------------------------->
	<--------------------------------- header --------------------------------->
																			   <-----------> uniform buffer
																						   <-----------> uniform buffer


	<--- Destructible Sync-ables Function Call Flow --->

	1) write
		onProcessWriteData()
			loadDataForWrite()
				interceptLoad()
			processWriteBuffer()
				getBufferSizeRequired()
					getSegmentSizeRequired()
				callback.onWriteBegin()
				writeUserBuffer()
					writeUserSegment()
						writeUserUniformBuffer()
							interpret()
							interceptEdit()
				callback.onWriteDone()
			unloadDataForWrite()

		___buffer full___
		a)	after generateFractureProfilesInDamageBuffer()
			before fillFractureBufferFromDamage()
		b)	before processFractureBuffer()
		c)	after fetchResults()
				actor->setRenderTMs()

	2) read
		onPreProcessReadData()
			processReadBuffer()
				callback.onPreProcessReadBegin()
				writeUserBufferPointers()
				callback.onPreProcessReadDone()
				callback.onReadBegin()
			loadDataForRead()

		___begin read___
		a)	generateFractureProfilesInDamageBuffer()
				interpret()
				interceptEdit()
			fillFractureBufferFromDamage()
		b)	processFractureBuffer()
				interpret()
				interceptEdit()
		c)	fetchResults()
				actor->setRenderTMs()
		___end read___

		onPostProcessReadData()
			unloadDataForRead()
			callback.onReadDone()
	*/

    /*** DestructibleScene::SyncParams ***/
public:
    class SyncParams
    {
    public:
		struct UserDamageEvent;
		struct UserFractureEvent;
        explicit SyncParams(const ModuleDestructibleImpl::SyncParams & moduleParams);
        ~SyncParams();
	public:
		bool					setSyncActor(uint32_t entryIndex, DestructibleActorImpl * entry, DestructibleActorImpl *& erasedEntry);
	private:
		DestructibleActorImpl *		getSyncActor(uint32_t entryIndex) const;
		uint32_t			getUserActorID(uint32_t destructibleID) const;

	public: //read and write - public APIs
		void					onPreProcessReadData(UserDamageEventHandler & callback, const physx::Array<UserDamageEvent> *& userSource);
		void					onPreProcessReadData(UserFractureEventHandler & callback, const physx::Array<UserFractureEvent> *& userSource);
		void					onPreProcessReadData(UserChunkMotionHandler & callback);
		
		DamageEvent &			interpret(const UserDamageEvent & userDamageEvent);
		FractureEvent &			interpret(const UserFractureEvent & userFractureEvent);
		void					interceptEdit(DamageEvent & damageEvent, const DestructibleActorImpl::SyncParams & actorParams) const;
		void					interceptEdit(FractureEvent & fractureEvent, const DestructibleActorImpl::SyncParams & actorParams) const;

		void					onPostProcessReadData(UserDamageEventHandler & callback);
		void					onPostProcessReadData(UserFractureEventHandler & callback);
		void					onPostProcessReadData(UserChunkMotionHandler & callback);

		void					onProcessWriteData(UserDamageEventHandler & callback, const RingBuffer<DamageEvent> & localSource);
		void					onProcessWriteData(UserFractureEventHandler & callback, const RingBuffer<FractureEvent> & localSource);
		void					onProcessWriteData(UserChunkMotionHandler & callback);

	private: //read - helper functions
		void					loadDataForRead(const DamageEventHeader * bufferStart, const physx::Array<UserDamageEvent> *& userSource);
		void					loadDataForRead(const FractureEventHeader * bufferStart, const physx::Array<UserFractureEvent> *& userSource);
		void					loadDataForRead(const ChunkTransformHeader * bufferStart);

		bool					unloadDataForRead(const UserDamageEventHandler & dummy);
		bool					unloadDataForRead(const UserFractureEventHandler & dummy);
		bool					unloadDataForRead(const UserChunkMotionHandler & dummy);

		template<typename Callback, typename Header, typename Unit>	const Header *	processReadBuffer(Callback & callback) const;
		template<typename Header, typename Unit>					int32_t	writeUserBufferPointers(Header * bufferStart, uint32_t bufferSize) const;
		template<typename Callback>									void			postProcessReadData(Callback & callback);

	private: //write - helper functions
		uint32_t			loadDataForWrite(const RingBuffer<DamageEvent> & localSource);
		uint32_t			loadDataForWrite(const RingBuffer<FractureEvent> & localSource);
		uint32_t			loadDataForWrite(const UserChunkMotionHandler & dummy);

		void					unloadDataForWrite(const UserDamageEventHandler & dummy);
		void					unloadDataForWrite(const UserFractureEventHandler & dummy);
		void					unloadDataForWrite(const UserChunkMotionHandler & dummy);

        template<typename Callback, typename Header, typename Unit>	uint32_t	processWriteBuffer(Callback & callback);
		template<typename Header, typename Unit>					uint32_t	getBufferSizeRequired() const;
		template<typename Header, typename Unit>					uint32_t	getSegmentSizeRequired(const DestructibleActorImpl::SyncParams & actorParams) const;
		template<typename Header, typename Unit>					uint32_t	writeUserBuffer(Header * bufferStart, uint32_t & headerCount);
		template<typename Header, typename Unit>					uint32_t	writeUserSegment(Header * header, bool isLast, const DestructibleActorImpl::SyncParams & actorParams);
		template<typename Unit>										uint32_t	writeUserUniformBuffer(Unit * bufferStart, const DestructibleActorImpl::SyncParams & actorParams);
		
		bool								interceptLoad(const DamageEvent & damageEvent, const DestructibleActorImpl & syncActor) const;
		bool								interceptLoad(const FractureEvent & fractureEvent, const DestructibleActorImpl & syncActor) const;
		DamageEventUnit *				interpret(const DamageEvent & damageEvent);
		FractureEventUnit *			interpret(const FractureEvent & fractureEvent);
		void								interceptEdit(DamageEventUnit & nxApexDamageEventUnit, const DestructibleActorImpl::SyncParams & actorParams) const;
		void								interceptEdit(FractureEventUnit & nxApexFractureEventUnit, const DestructibleActorImpl::SyncParams & actorParams) const;

	private: //read and write - template helper functions
		template<typename Item>		uint32_t	getUniformBufferCount(const DamageEventHeader &		header) const;
		template<typename Item>		uint32_t	getUniformBufferCount(const FractureEventHeader &		header) const;
		template<typename Item>		uint32_t	getUniformBufferCount(const ChunkTransformHeader &	header) const;

		template<typename Item>		uint32_t &	getUniformBufferCountMutable(DamageEventHeader &		header) const;
		template<typename Item>		uint32_t &	getUniformBufferCountMutable(FractureEventHeader &	header) const;
		template<typename Item>		uint32_t &	getUniformBufferCountMutable(ChunkTransformHeader &	header) const;

		template<typename Item>		Item *			getUniformBufferStart(const DamageEventHeader &		header) const;
		template<typename Item>		Item *			getUniformBufferStart(const FractureEventHeader &		header) const;
		template<typename Item>		Item *			getUniformBufferStart(const ChunkTransformHeader &	header) const;

		template<typename Item>		Item *&			getUniformBufferStartMutable(DamageEventHeader &		header) const;
		template<typename Item>		Item *&			getUniformBufferStartMutable(FractureEventHeader &	header) const;
		template<typename Item>		Item *&			getUniformBufferStartMutable(ChunkTransformHeader &	header) const;

#if 1
    public:
        bool ditchStaleBuffers;
		bool lockSyncParams;
#endif // lionel: TODO...if necessary

    private:
		SyncParams();
        DECLARE_DISABLE_COPY_AND_ASSIGN(SyncParams);
		const ModuleDestructibleImpl::SyncParams &			moduleParams;
		mutable physx::Array<uint32_t>				segmentSizeChecker;
		const RingBuffer<DamageEvent>	*				damageEventWriteSource;
		DamageEventUnit							damageEventUserInstance;
		physx::Array<UserDamageEvent>					damageEventReadSource;
		DamageEvent										damageEventImplInstance;
		const RingBuffer<FractureEvent> *				fractureEventWriteSource;
        FractureEventUnit							fractureEventUserInstance;
		physx::Array<UserFractureEvent>					fractureEventReadSource;
        FractureEvent									fractureEventImplInstance;
		physx::Array<DestructibleStructure::Chunk*>		chunksWithUserControlledChunk;

		class SyncActorRecord
		{
			friend bool DestructibleScene::SyncParams::setSyncActor(uint32_t, DestructibleActorImpl *, DestructibleActorImpl *&);
		public:
			SyncActorRecord();
			~SyncActorRecord();
			const physx::Array<uint32_t> &			getIndexContainer() const;
			const physx::Array<DestructibleActorImpl*> &	getActorContainer() const;
		private:
			DECLARE_DISABLE_COPY_AND_ASSIGN(SyncActorRecord);
			void										onRebuild(const uint32_t & newCount);
			physx::Array<uint32_t>					indexContainer;
			physx::Array<DestructibleActorImpl*>			actorContainer;
		private:
			bool										assertActorContainerOk() const;
		}												syncActorRecord;

		struct ActorEventPair
		{
		public:
			const DestructibleActorImpl *			actorAlias;
		protected:
			union
			{
			public:
				const DamageEventUnit *	userDamageEvent;
				const FractureEventUnit *	userFractureEvent;
			};
			ActorEventPair(const DestructibleActorImpl * actorAlias, const DamageEventUnit * userDamageEvent);
			ActorEventPair(const DestructibleActorImpl * actorAlias, const FractureEventUnit * userFractureEvent);
			~ActorEventPair();
		private:
			ActorEventPair();
		};

	public:
		struct UserDamageEvent : public ActorEventPair
		{
		public:
			UserDamageEvent(const DestructibleActorImpl * actorAlias, const DamageEventUnit * userDamageEvent);
			~UserDamageEvent();
			const DamageEventUnit *		get() const;
		private:
			UserDamageEvent();
		};

		struct UserFractureEvent : public ActorEventPair
		{
		public:
			UserFractureEvent(const DestructibleActorImpl * actorAlias, const FractureEventUnit * userFractureEvent);
			~UserFractureEvent();
			const FractureEventUnit *		get() const;
		private:
			UserFractureEvent();
		};

	public:
		bool				assertUserDamageEventOk(const DamageEvent & damageEvent, const DestructibleScene & scene) const;
		bool				assertUserFractureEventOk(const FractureEvent & fractureEvent, const DestructibleScene & scene) const;
	private:
		bool				assertCachedChunkContainerOk(const DestructibleActorImpl::SyncParams & actorParams) const;
		bool				assertControlledChunkContainerOk() const;
	private:
		static const char *	errorMissingUserActorID;
		static const char *	errorMissingFlagReadDamageEvents;
		static const char *	errorMissingFlagReadFractureEvents;
		static const char *	errorMissingFlagReadChunkMotion;
		static const char *	errorOutOfBoundsActorChunkIndex;
		static const char *	errorOutOfBoundsStructureChunkIndex;
		static const char *	errorOverrunBuffer;
    };

	const DestructibleScene::SyncParams &		getSyncParams() const;
	DestructibleScene::SyncParams &				getSyncParamsMutable();
private:
	SyncParams									mSyncParams;
	
	//these methods participate in the sync process
private:
	void								processEventBuffers();
	void										generateFractureProfilesInDamageBuffer(nvidia::RingBuffer<DamageEvent> & userDamageBuffer, const physx::Array<SyncParams::UserDamageEvent> * userSource = NULL);
	void										calculateDamageBufferCostProfiles(nvidia::RingBuffer<DamageEvent> & subjectDamageBuffer);
	void										fillFractureBufferFromDamage(nvidia::RingBuffer<DamageEvent> * userDamageBuffer = NULL);
	void										processFractureBuffer();
	void										processFractureBuffer(const physx::Array<SyncParams::UserFractureEvent> * userSource);
	void										processDamageColoringBuffer();

private:
	RingBuffer<FractureEvent>					mDeprioritisedFractureBuffer;
	RingBuffer<FractureEvent>					mDeferredFractureBuffer;
	RingBuffer<SyncDamageEventCoreDataParams>	mSyncDamageEventCoreDataBuffer;
};

template <class ChunkOpClass>
PX_INLINE void DestructibleScene::forSubtree(DestructibleStructure::Chunk& chunk, ChunkOpClass chunkOp, bool skipRoot)
{
	uint16_t indexInAsset = chunk.indexInAsset;
	DestructibleStructure::Chunk* pProcessChunk = &chunk;
	physx::Array<uint16_t> stack;

	DestructibleActorImpl* destructible = mDestructibles.direct(chunk.destructibleID);
	const DestructibleAssetParametersNS::Chunk_Type* sourceChunks = destructible->getDestructibleAsset()->mParams->chunks.buf;
	const uint32_t firstChunkIndex = destructible->getFirstChunkIndex();

	for (;;)
	{
		bool recurse = skipRoot ? true : chunkOp.execute(destructible->getStructure(), *pProcessChunk);
		skipRoot = false;
		const DestructibleAssetParametersNS::Chunk_Type& source = sourceChunks[indexInAsset];
		if (recurse && source.numChildren)
		{
			for (indexInAsset = uint16_t(source.firstChildIndex + source.numChildren); --indexInAsset > source.firstChildIndex;)
			{
				stack.pushBack(indexInAsset);
			}
		}
		else
		{
			if (stack.size() == 0)
			{
				return;
			}
			indexInAsset = stack.back();
			stack.popBack();
		}
		pProcessChunk = destructible->getStructure()->chunks.begin() + (firstChunkIndex + indexInAsset);
	}
}

PX_INLINE ChunkData* DestructibleScene::getChunkReportData(DestructibleStructure::Chunk& chunk, uint32_t flags)
{
	if (!mModule->m_chunkReport)
	{
		return NULL;	// don't give out data if we won't be reporting it
	}

	if (chunk.reportID == InvalidReportID)
	{
		// No chunk report data.  Create.
		const uint32_t damageEventIndex = mDamageEventReportData.size();
		ApexDamageEventReportDataImpl& damageReportData = mDamageEventReportData.insert();
		damageReportData.setDestructible(mDestructibles.direct(chunk.destructibleID));
		const uint32_t fractureEventIndex = damageReportData.addFractureEvent(chunk, flags);
		chunk.reportID = mChunkReportHandles.size();
		IntPair& handle = mChunkReportHandles.insert();
		handle.set((int32_t)damageEventIndex, (int32_t)fractureEventIndex);
	}

	PX_ASSERT(chunk.reportID < mChunkReportHandles.size());
	if (chunk.reportID < mChunkReportHandles.size())
	{
		IntPair& handle = mChunkReportHandles[chunk.reportID];
		PX_ASSERT((uint32_t)handle.i0 < mDamageEventReportData.size());
		if ((uint32_t)handle.i0 < mDamageEventReportData.size() && (uint32_t)handle.i1 != 0xFFFFFFFF)
		{
			return &mDamageEventReportData[(uint32_t)handle.i0].getFractureEvent((uint32_t)handle.i1);
		}
	}

	return NULL;
}

PX_INLINE PxRigidDynamic* DestructibleScene::chunkIntact(DestructibleStructure::Chunk& chunk)
{
	DestructibleStructure::Chunk* child = &chunk;
	DestructibleActorImpl* destructible = mDestructibles.direct(chunk.destructibleID);

	if (!chunk.isDestroyed())
	{
		return destructible->getStructure()->getChunkActor(chunk);
	}

	if ((chunk.flags & ChunkMissingChild) != 0)
	{
		return NULL;
	}

	DestructibleAssetImpl* asset = destructible->getDestructibleAsset();
	do
	{
		DestructibleAssetParametersNS::Chunk_Type& source = asset->mParams->chunks.buf[child->indexInAsset];
		PX_ASSERT(source.numChildren > 0);
		child = &destructible->getStructure()->chunks[destructible->getFirstChunkIndex() + source.firstChildIndex];

		if (!child->isDestroyed())
		{
			return destructible->getStructure()->getChunkActor(chunk);
		}
	}
	while ((child->flags & ChunkMissingChild) == 0);

	return NULL;
}

PX_INLINE void cacheChunkTempState(DestructibleStructure::Chunk& chunk, physx::Array<uint8_t*>& trail, physx::Array<uint8_t>& undo)
{
	// ChunkTemp0 = chunk state has been cached
	// ChunkTemp1 = chunk exists
	// ChunkTemp2 = chunk is visible
	// ChunkTemp3 = chunk is dynamic

	undo.pushBack((uint8_t)(chunk.state & (uint8_t)ChunkTempMask));

	if ((chunk.state & ChunkTemp0) == 0)
	{
		chunk.state |= ChunkTemp0;
		if (!chunk.isDestroyed())
		{
			chunk.state |= ChunkTemp1;
			if (chunk.state & ChunkVisible)
			{
				chunk.state |= ChunkTemp2;
			}
			if (chunk.state & ChunkDynamic)
			{
				chunk.state |= ChunkTemp3;
			}
		}
		trail.pushBack(&chunk.state);
	}
}

PX_INLINE void DestructibleScene::calculatePotentialCostAndBenefit(float& cost, float& benefit, physx::Array<uint8_t*>& trail, physx::Array<uint8_t>& undo, const FractureEvent& fractureEvent) const
{
	cost = 0.0f;
	benefit = 0.0f;

	DestructibleActorImpl* destructible = mDestructibles.direct(fractureEvent.destructibleID);
	if (destructible == NULL)
	{
		return;
	}

	DestructibleStructure* structure = destructible->getStructure();
	if (structure == NULL)
	{
		return;
	}

	DestructibleStructure::Chunk* chunk = structure->chunks.begin() + fractureEvent.chunkIndexInAsset + destructible->getFirstChunkIndex();

	// ChunkTemp0 = chunk state has been cached
	// ChunkTemp1 = chunk exists
	// ChunkTemp2 = chunk is visible
	// ChunkTemp3 = chunk is dynamic

	cacheChunkTempState(*chunk, trail, undo);

	if ((chunk->state & ChunkTemp1) == 0)
	{
		return;
	}

	if ((chunk->state & ChunkTemp3) != 0)	// If the chunk is dynamic, we're done... no extra cost or benefit, and it's disconnected from its parent
	{
		return;
	}

	const uint32_t chunkIndexInAsset = fractureEvent.chunkIndexInAsset;
	const uint32_t chunkShapeCount = destructible->getDestructibleAsset()->getChunkHullCount(chunkIndexInAsset);

	cost += chunkShapeCount;
	benefit += destructible->getBenefit()*(float)chunkShapeCount/(float)PxMax<uint32_t>(destructible->getVisibleDynamicChunkShapeCount(), 1);

	chunk->state |= ChunkTemp3;	// Mark it as dynamic

	if ((chunk->state & ChunkTemp2) != 0)
	{
		return;	// It was visible already, so we just made it dynamic.  It will have no parent and iits siblings will already be accounted for, if they exist.
	}

	chunk->state |= ChunkTemp2;	// Mark it as visible

	for (;;)
	{
		// Climb the hierarchy
		DestructibleAssetParametersNS::Chunk_Type& source = destructible->getDestructibleAsset()->mParams->chunks.buf[chunk->indexInAsset];
		if (source.parentIndex == DestructibleAssetImpl::InvalidChunkIndex)
		{
			break;
		}
		chunk = structure->chunks.begin() + (source.parentIndex + destructible->getFirstChunkIndex());
		cacheChunkTempState(*chunk, trail, undo);
		if ((chunk->state & ChunkTemp1) == 0)
		{
			break;	// Parent doesn't exist
		}

		// Make children visible
		DestructibleAssetParametersNS::Chunk_Type& parentSource = destructible->getDestructibleAsset()->mParams->chunks.buf[chunk->indexInAsset];
		const uint32_t firstChildIndex = parentSource.firstChildIndex + destructible->getFirstChunkIndex();
		const uint32_t endChildIndex = firstChildIndex + parentSource.numChildren;
		for (uint32_t childIndex = firstChildIndex; childIndex < endChildIndex; ++childIndex)
		{
			DestructibleStructure::Chunk& child = structure->chunks[childIndex];
			cacheChunkTempState(child, trail, undo);
			if ((child.state & ChunkTemp1) == 0)
			{
				continue; // Child doesn't exist, skip
			}
			child.state |= ChunkTemp2;	// Make it visible
			if ((chunk->state & ChunkTemp3) != 0 || (child.flags & ChunkBelowSupportDepth) != 0)
			{
				// Add cost and benefit for children
				const uint32_t childndexInAsset = childIndex - destructible->getFirstChunkIndex();
				const uint32_t childShapeCount = destructible->getDestructibleAsset()->getChunkHullCount(childndexInAsset);

				cost += childShapeCount;
				benefit += destructible->getBenefit()*(float)childShapeCount/(float)PxMax<uint32_t>(destructible->getVisibleDynamicChunkShapeCount(), 1);

				// Parent is dynamic (so its children will be too), or the child is below the support depth, so make it dynamic
				child.state |= ChunkTemp3;
			}
		}

		chunk->state &= ~(uint8_t)(ChunkTemp1 | ChunkTemp2 | ChunkTemp3);	// The parent will cease to exist
	}
}

}
} // end namespace nvidia

#endif // __DESTRUCTIBLE_SCENE_H__
