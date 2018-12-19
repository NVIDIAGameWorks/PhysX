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
#include "SceneIntl.h"
#include "ModuleDestructibleImpl.h"
#include "DestructibleScene.h"
#include "DestructibleAssetImpl.h"
#include "DestructibleActorImpl.h"
#include "DestructibleActorProxy.h"
#include "DestructibleActorJointProxy.h"
#include "DestructibleStructureStressSolver.h"
#if APEX_USE_PARTICLES
#include "EmitterActor.h"
#include "EmitterGeoms.h"
#endif

#include "PsArray.h"
#include "PxScene.h"
#include "PxConvexMeshDesc.h"
#include "PxConvexMeshGeometry.h"
#include "PxBoxGeometry.h"
#include "PxSphereGeometry.h"

#if APEX_RUNTIME_FRACTURE
#include "SimScene.h"
#include "Convex.h"
#include "Compound.h"
#include "Actor.h"
#endif

#include "ApexRand.h"
#include "ModulePerfScope.h"
#include "PsString.h"
#include "PsTime.h"

#include "RenderMeshAssetIntl.h"

#include "Lock.h"
#include "PxProfiler.h"

#define USE_ACTIVE_TRANSFORMS_FOR_AWAKE_LIST	1

namespace nvidia
{
namespace destructible
{
using namespace physx;

namespace
{
	void unfortunateCompilerWorkaround(uint32_t)
	{
	}
};

PX_INLINE PxVec3 transformToScreenSpace(const PxVec3& pos, const float* viewProjectionMatrix)
{
	const float v[4] = { pos.x, pos.y, pos.z, 1.0f };

	float u[4] = { 0, 0, 0, 0 };

	const float* row = viewProjectionMatrix;
	for (uint32_t i = 0; i < 4; ++i, ++row)
	{
		for (uint32_t j = 0; j < 4; ++j)
		{
			u[i] += row[j << 2] * v[j];
		}
	}

	const float recipW = 1.0f / u[3];

	return PxVec3(u[0] * recipW, u[1] * recipW, u[2] * recipW);
}

// Put an actor to sleep based upon input velocities.
PX_INLINE void handleSleeping(PxRigidDynamic* actor, PxVec3 linearVelocity, PxVec3 angularVelocity)
{
	// In PhysX3, we only use an energy threshold.
	if (actor == NULL)
	{
		return;
	}
	// Calculate kinetic energy
	const float mass = actor->getMass();
	const float linearKE = 0.5f*mass*linearVelocity.magnitudeSquared();
	const PxTransform globalToMassT = actor->getCMassLocalPose().transform(actor->getGlobalPose());
	const PxVec3 massSpaceAngularVelocity = globalToMassT.rotateInv(angularVelocity);
	const float rotationalKE = 0.5f*massSpaceAngularVelocity.dot(actor->getMassSpaceInertiaTensor().multiply(massSpaceAngularVelocity));
	const float totalKE = linearKE + rotationalKE;
	// Put to sleep if below threshold
	if (totalKE <= actor->getSleepThreshold()*mass)
	{
		actor->setLinearVelocity(PxVec3(0.0f));
		actor->setAngularVelocity(PxVec3(0.0f));
		actor->putToSleep();
	}
}

/****************************
* DestructibleUserNotify	*
*****************************/

DestructibleUserNotify::DestructibleUserNotify(ModuleDestructibleImpl& module, DestructibleScene* destructibleScene) :
mModule(module),
mDestructibleScene(destructibleScene)
{

}
void DestructibleUserNotify::onConstraintBreak(physx::PxConstraintInfo* constraints, uint32_t count)
{
	PX_UNUSED(constraints);
	PX_UNUSED(count);
}

void DestructibleUserNotify::onWake(PxActor** actors, uint32_t count)
{
	if (mDestructibleScene->mUsingActiveTransforms)	// The remaining code in this function only updates the destructible actor awake list when not using active transforms
	{
		return;
	}

	for (uint32_t i = 0; i < count; i++)
	{
		PxActor* actor = actors[i];
		PhysXObjectDescIntl* desc = (PhysXObjectDescIntl*)(mModule.mSdk->getPhysXObjectInfo(actor));
		if (desc != NULL)
		{
			if (desc->getUserDefinedFlag(PhysXActorFlags::CREATED_THIS_FRAME) || desc->getUserDefinedFlag(PhysXActorFlags::IS_SLEEPING))
			{
				// Only increase the counter in the first wake call, or
				// when the state has changed since the last callback.
				// IS_SLEEPING has to be checked, because with PhysX 3.3 we also
				// receive a callback when the user calls putToSleep on an awake actor
				// and the SDK wakes it up again.

				// increase wake count on each referenced destructible
				const uint32_t dActorCount = desc->mApexActors.size();
				for (uint32_t i = 0; i < dActorCount; ++i)
				{
					const DestructibleActor* dActor = static_cast<const DestructibleActor*>(desc->mApexActors[i]);
					if (dActor != NULL)
					{
						if (desc->mApexActors[i]->getOwner()->getObjTypeID() == DestructibleAssetImpl::getAssetTypeID())
						{
							DestructibleActorImpl& destructibleActor = const_cast<DestructibleActorImpl&>(static_cast<const DestructibleActorProxy*>(dActor)->impl);
							destructibleActor.incrementWakeCount();
						}
					}
				}

				// update externally stored state of this physx actor
				desc->setUserDefinedFlag(PhysXActorFlags::IS_SLEEPING, false);
			}

			desc->setUserDefinedFlag(PhysXActorFlags::CREATED_THIS_FRAME, false);
		}
	}
}

void DestructibleUserNotify::onSleep(PxActor** actors, uint32_t count)
{
	if (mDestructibleScene->mUsingActiveTransforms)	// The remaining code in this function only updates the destructible actor awake list when not using active transforms
	{
		return;
	}

	for (uint32_t i = 0; i < count; i++)
	{
		PxActor* actor = actors[i];
		PhysXObjectDescIntl* desc = (PhysXObjectDescIntl*)(mModule.mSdk->getPhysXObjectInfo(actor));
		if (desc != NULL)
		{
			if (desc->getUserDefinedFlag(PhysXActorFlags::CREATED_THIS_FRAME))
			{
				// first sleep callback must be ignored, as it's not a state change, but initialization
				desc->setUserDefinedFlag(PhysXActorFlags::CREATED_THIS_FRAME, false);
				desc->setUserDefinedFlag(PhysXActorFlags::IS_SLEEPING, true);
				continue;
			}

			if (!desc->getUserDefinedFlag(PhysXActorFlags::IS_SLEEPING))
			{
				// Only decrease the wake count, if the state has really changed to sleeping
				// since the last callback
				const uint32_t dActorCount = desc->mApexActors.size();
				for (uint32_t i = 0; i < dActorCount; ++i)
				{
					const DestructibleActor* dActor = static_cast<const DestructibleActor*>(desc->mApexActors[i]);
					if (dActor != NULL)
					{
						if (desc->mApexActors[i]->getOwner()->getObjTypeID() == DestructibleAssetImpl::getAssetTypeID())
						{
							DestructibleActorImpl& destructibleActor = const_cast<DestructibleActorImpl&>(static_cast<const DestructibleActorProxy*>(dActor)->impl);
							destructibleActor.decrementWakeCount();
						}
					}
				}

				desc->setUserDefinedFlag(PhysXActorFlags::IS_SLEEPING, true);
			}
		}
	}
}


void DestructibleUserNotify::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, uint32_t nbPairs)
{
	if (pairHeader.flags & physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_0 ||
		pairHeader.flags & physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_1 ||
		pairHeader.actors[0] == NULL ||
		pairHeader.actors[1] == NULL)
	{
		return;
	}

	ModuleDestructibleImpl* module = mDestructibleScene->mModule;
	int moduleOwnsActor[2] =
	{
		(int)module->owns(pairHeader.actors[0]->is<physx::PxRigidActor>()),
		(int)module->owns(pairHeader.actors[1]->is<physx::PxRigidActor>())
	};

	if (!(moduleOwnsActor[0] | moduleOwnsActor[1]))
	{
		return;	// Neither is owned by the destruction module
	}

	for (uint32_t pairIdx = 0; pairIdx < nbPairs; pairIdx++)
	{
		const PxContactPair& currentPair = pairs[pairIdx];

		if (currentPair.flags & physx::PxContactPairFlag::eREMOVED_SHAPE_0 ||
			currentPair.flags & physx::PxContactPairFlag::eREMOVED_SHAPE_1 ||
			currentPair.shapes[0] == NULL ||
			currentPair.shapes[1] == NULL)
		{
			continue;
		}

		DestructibleActorImpl*				destructibles[2] = {NULL, NULL};
		DestructibleStructure::Chunk*	chunks[2] = {NULL, NULL};
		bool							takesImpactDamage[2] = {false, false};
		float					minImpactVelocityThresholdsSq = PX_MAX_REAL;

		for (int i = 0; i < 2; ++i)
		{
			PxShape* shape = currentPair.shapes[i];
			if (moduleOwnsActor[i] && module->getDestructibleAndChunk(shape, NULL) == NULL)
			{
				chunks[i] = NULL;
			}
			else
			{
				chunks[i] = mDestructibleScene->getChunk(shape);
			}
			if (chunks[i] != NULL)
			{
				destructibles[i] = mDestructibleScene->mDestructibles.direct(chunks[i]->destructibleID);
				PX_ASSERT(destructibles[i] != NULL);
				if (destructibles[i] != NULL)
				{
					float ivts = destructibles[i]->getDestructibleParameters().impactVelocityThreshold;
					ivts *= ivts;
					if (ivts < minImpactVelocityThresholdsSq)
					{
						minImpactVelocityThresholdsSq = ivts;
					}

					int32_t depth = destructibles[i]->getDestructibleAsset()->getChunkDepth(chunks[i]->indexInAsset);
					takesImpactDamage[i] = destructibles[i]->takesImpactDamageAtDepth((uint32_t)depth);
				}
			}
		}
		if (destructibles[0] == destructibles[1])
		{
			return; // No self-collision.  To do: multiply by a self-collision factor instead?
		}

		if (!takesImpactDamage[0] && !takesImpactDamage[1])
		{
			return;
		}

		float masses[2];
		{
			for (int i = 0; i < 2; ++i)
			{
				masses[i] = 0;
				PxRigidDynamic* rigidDynamic = pairHeader.actors[i]->is<physx::PxRigidDynamic>();
				if (rigidDynamic)
				{
					SCOPED_PHYSX_LOCK_READ(rigidDynamic->getScene());
					if (!(rigidDynamic->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC))
					{
						masses[i] = rigidDynamic->getMass();
					}
				}
			}
		};

		float reducedMass;
		if (masses[0] == 0.0f)
		{
			reducedMass = masses[1];
		}
		else if (masses[1] == 0.0f)
		{
			reducedMass = masses[0];
		}
		else
		{
			reducedMass = masses[0] * masses[1] / (masses[0] + masses[1]);
		}


		PxVec3 destructibleForces[2] = {PxVec3(0.0f), PxVec3(0.0f)};
		PxVec3 avgContactPosition = PxVec3(0.0f);
		PxVec3 avgContactNormal = PxVec3(0.0f);
		uint32_t  numContacts = 0;

#if USE_EXTRACT_CONTACTS
#if PAIR_POINT_ALLOCS
		PxContactPairPoint* pairPointBuffer = (PxContactPairPoint*)PX_ALLOC(currentPair.contactCount * sizeof(PxContactPairPoint), PX_DEBUG_EXP("PxContactPairPoints"));
#else
		mPairPointBuffer.reserve(currentPair.contactCount * sizeof(PxContactPairPoint));

		// if this method isn't used, the operator[] method will fail because the actual size may be zero
		mPairPointBuffer.forceSize_Unsafe(currentPair.contactCount * sizeof(PxContactPairPoint));
		PxContactPairPoint* pairPointBuffer = currentPair.contactCount > 0 ? (PxContactPairPoint*)&(mPairPointBuffer[0]) : NULL;
#endif
		uint32_t numContactsInStream = pairPointBuffer != NULL ? currentPair.extractContacts(pairPointBuffer, currentPair.contactCount) : 0;
#else
		uint32_t numContactsInStream = currentPair.contactCount;
		const PxContactPoint* contacts = reinterpret_cast<const PxContactPoint*>(currentPair.contactStream);
#endif


		for (uint32_t contactIdx = 0; contactIdx < numContactsInStream; contactIdx++)
		{
#if USE_EXTRACT_CONTACTS
			PxContactPairPoint& currentPoint = pairPointBuffer[contactIdx];

			const PxVec3& patchNormal = currentPoint.normal;
			const PxVec3& position = currentPoint.position;
#else
			const PxContactPoint& cp = contacts[contactIdx];

			const PxVec3& patchNormal = cp.normal;
			const PxVec3& position = cp.point;
#endif
			PxVec3 velocities[2] = { PxVec3(0.0f), PxVec3(0.0f) };
			for (int i = 0; i < 2; ++i)
			{
				PxRigidBody* rigidBody = pairHeader.actors[i]->is<physx::PxRigidBody>();
				if (rigidBody)
				{
					SCOPED_PHYSX_LOCK_READ(rigidBody->getScene());
					velocities[i] = physx::PxRigidBodyExt::getVelocityAtPos(*rigidBody, position);
				}
			}

			const PxVec3 velocityDelta = velocities[0] - velocities[1];
			if (velocityDelta.magnitudeSquared() >= minImpactVelocityThresholdsSq || reducedMass == 0.0f)	// If reduced mass == 0, this is kineamtic vs. kinematic.  Generate damage.
			{
				for (int i = 0; i < 2; ++i)
				{
					DestructibleActorImpl* destructible = destructibles[i];
					if (destructible)
					{
						// this is not really physically correct, but at least its deterministic...
						destructibleForces[i] += (patchNormal * patchNormal.dot(velocityDelta)) * reducedMass * (i ? 1.0f : -1.0f);
					}
				}
				avgContactPosition += position;
				avgContactNormal += patchNormal;
				numContacts++;
			}
			if (numContacts)
			{
				avgContactPosition /= (float)numContacts;
				avgContactNormal.normalize();
				for (uint32_t i = 0; i < 2; i++)
				{
					if (!takesImpactDamage[i])
						continue;

					const PxVec3 force = destructibleForces[i] / (float)numContacts;
					DestructibleActorImpl* destructible = destructibles[i];
					if (destructible != NULL)
					{
						if (!force.isZero())
						{
							destructible->takeImpact(force, avgContactPosition, chunks[i]->indexInAsset, pairHeader.actors[i ^ 1]);
						}
						else if (reducedMass == 0.0f)	// Handle kineamtic vs. kinematic
						{
							const DestructibleActorParamNS::BehaviorGroup_Type& behaviorGroup = destructible->getBehaviorGroup(chunks[i]->indexInAsset);
							destructible->applyDamage(2.0f*behaviorGroup.damageThreshold, 0.0f, avgContactPosition, (avgContactNormal * (i ? 1.0f : -1.0f)), chunks[i]->indexInAsset);
						}
					}
				}
			}
		}

#if PAIR_POINT_ALLOCS
		if (pairPointBuffer != NULL)
		{
			PX_FREE(pairPointBuffer);
		}
#endif
	}
}


void DestructibleUserNotify::onTrigger(PxTriggerPair* pairs, uint32_t count)
{
	PX_UNUSED(pairs);
	PX_UNUSED(count);
}


void DestructibleUserNotify::onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count)
{
	PX_UNUSED(bodyBuffer);
	PX_UNUSED(poseBuffer);
	PX_UNUSED(count);
}


/****************************
* DestructibleContactModify *
*****************************/

void DestructibleContactModify::onContactModify(PxContactModifyPair* const pairs, uint32_t count)
{
	PX_PROFILE_ZONE("DestructibleOnContactConstraint", GetInternalApexSDK()->getContextId());

	for (uint32_t iPair = 0; iPair < count; iPair++)
	{
		PxContactModifyPair&	pair = pairs[iPair];

		ModuleDestructibleImpl* module = destructibleScene->mModule;

		int32_t chunkIndex0 = 0;
		DestructibleActorProxy* proxy0 = static_cast<DestructibleActorProxy*>(module->getDestructibleAndChunk((PxShape*)pair.shape[0], &chunkIndex0));
		int32_t chunkIndex1 = 0;
		DestructibleActorProxy* proxy1 = static_cast<DestructibleActorProxy*>(module->getDestructibleAndChunk((PxShape*)pair.shape[1], &chunkIndex1));

		const bool moduleOwnsActor[2] = {proxy0 != NULL, proxy1 != NULL};

		if (moduleOwnsActor[0] == moduleOwnsActor[1])
		{
			continue;	// Neither is owned by the destruction module, or both are
		}

		const int externalRBIndex = (int)(moduleOwnsActor[1] == 0);

		destructibleScene->mApexScene->getPhysXScene()->lockRead();
		const bool externalActorDynamic = pair.actor[externalRBIndex]->is<physx::PxRigidDynamic>() != NULL;
		destructibleScene->mApexScene->getPhysXScene()->unlockRead();

		if (!externalActorDynamic)
		{
			continue;
		}

		DestructibleActorProxy* proxy = externalRBIndex ? proxy0 : proxy1;
		const float materialStrength = proxy->impl.getBehaviorGroup(uint32_t(externalRBIndex ? chunkIndex0 : chunkIndex1)).materialStrength;
		if (materialStrength > 0.0f)
		{
			for (uint32_t contactIndex = 0; contactIndex < pair.contacts.size(); ++contactIndex)
			{
				pair.contacts.setMaxImpulse(contactIndex, materialStrength);
			}
		}
	}
}


/****************************
* ApexDamageEventReportDataImpl *
*****************************/

uint32_t
ApexDamageEventReportDataImpl::addFractureEvent(const DestructibleStructure::Chunk& chunk, uint32_t flags)
{
	PX_ASSERT(m_destructible != NULL);
	if (m_destructible == NULL)
	{
		return 0xFFFFFFFF;
	}

	// Find flags to see if we record this event
	PX_ASSERT(!chunk.isDestroyed());
	if (chunk.state & ChunkDynamic)
	{
		flags |= ApexChunkFlag::DYNAMIC;
	}
	if (chunk.flags & ChunkExternallySupported)
	{
		flags |= ApexChunkFlag::EXTERNALLY_SUPPORTED;
	}
	if (chunk.flags & ChunkWorldSupported)
	{
		flags |= ApexChunkFlag::WORLD_SUPPORTED;
	}
	if (chunk.flags & ChunkCrumbled)
	{
		flags |= ApexChunkFlag::DESTROYED_CRUMBLED;
	}

	// return invalid index if we don't record this event
	if ((m_chunkReportBitMask & flags) == 0)
	{
		return 0xFFFFFFFF;
	}

	PX_ASSERT(m_destructible->getID() == chunk.destructibleID);
	const DestructibleAssetParametersNS::Chunk_Type& source = m_destructible->getDestructibleAsset()->mParams->chunks.buf[chunk.indexInAsset];

	PxBounds3 chunkWorldBoundsOnStack;
	PxBounds3* chunkWorldBounds = &chunkWorldBoundsOnStack;

	uint32_t fractureEventIndex = 0xFFFFFFFF;

	if (source.depth <= m_chunkReportMaxFractureEventDepth)
	{
		fractureEventIndex = m_fractureEvents.size();
		ChunkData& fractureEvent = m_fractureEvents.insert();
		fractureEventList = &m_fractureEvents[0];
		fractureEventListSize = m_fractureEvents.size();
		fractureEvent.index = chunk.indexInAsset;
		fractureEvent.depth = source.depth;
		fractureEvent.damage = chunk.damage;
		chunkWorldBounds = &fractureEvent.worldBounds;	// Will be filled in below
		fractureEvent.flags = flags;
	}

	// Adjust bounds to world coordinates
	const PxVec3 scale = m_destructible->getScale();
	const PxBounds3& bounds = m_destructible->getDestructibleAsset()->getChunkShapeLocalBounds(chunk.indexInAsset);
	chunkWorldBounds->minimum = bounds.minimum.multiply(scale);
	chunkWorldBounds->maximum = bounds.maximum.multiply(scale);
	PxBounds3Transform(*chunkWorldBounds, PxMat44(m_destructible->getChunkPose(chunk.indexInAsset)));

	worldBounds.include(*chunkWorldBounds);
	minDepth = PxMin(minDepth, source.depth);
	maxDepth = PxMax(maxDepth, source.depth);
	++totalNumberOfFractureEvents;

	return fractureEventIndex;
}

void
ApexDamageEventReportDataImpl::setDestructible(DestructibleActorImpl* inDestructible)
{
	m_destructible = inDestructible;
	if (m_destructible != NULL)
	{
		m_chunkReportBitMask = m_destructible->getStructure()->dscene->mModule->m_chunkReportBitMask;
		m_chunkReportMaxFractureEventDepth = m_destructible->getStructure()->dscene->mModule->m_chunkReportMaxFractureEventDepth;
		destructible = m_destructible->getAPI();
		minDepth = (uint16_t)(m_destructible->getDestructibleAsset()->mParams->depthCount > 0 ? m_destructible->getDestructibleAsset()->mParams->depthCount - 1 : 0);
		maxDepth = 0;
	}
	else
	{
		clear();
	}
}

void
ApexDamageEventReportDataImpl::clearChunkReports()
{
	PX_ASSERT(m_destructible != NULL);
	if (m_destructible == NULL)
	{
		return;
	}

	for (uint32_t i = 0; i < m_fractureEvents.size(); ++i)
	{
		ChunkData& fractureEvent = m_fractureEvents[i];
		DestructibleStructure::Chunk& chunk = m_destructible->getStructure()->chunks[fractureEvent.index + m_destructible->getFirstChunkIndex()];
		chunk.reportID = (uint32_t)DestructibleScene::InvalidReportID;
	}

	totalNumberOfFractureEvents = 0;
}

/********************
* DestructibleScene *
********************/

class DestructibleBeforeTick : public PxTask, public UserAllocated
{
public:
	DestructibleBeforeTick(DestructibleScene& scene) : mScene(&scene), mDeltaTime(0.0f) {}
	const char* getName() const
	{
		return "DestructibleScene::BeforeTick";
	}
	void setDeltaTime(float deltaTime)
	{
		mDeltaTime = deltaTime;
	}
	void run()
	{
		mScene->tasked_beforeTick(mDeltaTime);
	}
	DestructibleScene* mScene;
	float mDeltaTime;
};

static int comparePointers(const void* a, const void* b)
{
	return (uintptr_t)a == (uintptr_t)b ? 0 : ((uintptr_t)a < (uintptr_t)b ? -1 : 1);
}

static int compareOverlapHitShapePointers(const void* a, const void* b)
{
	const physx::PxOverlapHit* overlapA = (const physx::PxOverlapHit*)a;
	const physx::PxOverlapHit* overlapB = (const physx::PxOverlapHit*)b;

	return comparePointers(overlapA->shape, overlapB->shape);
}

DestructibleScene::DestructibleScene(ModuleDestructibleImpl& module, SceneIntl& scene, RenderDebugInterface* debugRender, ResourceList& list) :
	mUserNotify(module, this),
	mElapsedTime(0.0f),
	mMassScale(1.0f),
	mMassScaleInv(1.0f),
	mScaledMassExponent(0.5f),
	mScaledMassExponentInv(2.0f),
	mPreviousVisibleDestructibleChunkCount(0),
	mPreviousDynamicDestructibleChunkIslandCount(0),
	mDynamicActorFIFONum(0),
	mTotalChunkCount(0),
	mFractureEventCount(0),
	mDamageBufferWriteIndex(0),
	mUsingActiveTransforms(false),
	m_worldSupportPhysXScene(NULL),
	m_damageApplicationRaycastFlags(nvidia::DestructibleActorRaycastFlags::StaticChunks),
	mDebugRender(debugRender),
	mRenderLockMode(RenderLockMode::PER_ACTOR_RENDER_LOCK),
	mSyncParams(module.getSyncParams())
{
	list.add(*this);
	mModule = &module;
	mApexScene = &scene;
	mPhysXScene = NULL;
	mOverlapHits.resize(MAX_SHAPE_COUNT);
	mApexScene->addModuleUserContactModify(mContactModify);
	mContactModify.destructibleScene = this;
	mNumFracturesProcessedThisFrame = 0;
	mNumActorsCreatedThisFrame = 0;
	mApexScene->addModuleUserNotifier(mUserNotify);

#if APEX_RUNTIME_FRACTURE
	mRTScene = NULL;
#endif

	mBeforeTickTask = PX_NEW(DestructibleBeforeTick)(*this);

	/* Initialize reference to DestructibleDebugRenderParams */
	{
		READ_LOCK(*mApexScene);
		mDebugRenderParams = DYNAMIC_CAST(DebugRenderParams*)(mApexScene->getDebugRenderParams());
	}
	PX_ASSERT(mDebugRenderParams);
	NvParameterized::Handle handle(*mDebugRenderParams), memberHandle(*mDebugRenderParams);
	int size;

	if (mDebugRenderParams->getParameterHandle("moduleName", handle) == NvParameterized::ERROR_NONE)
	{
		handle.getArraySize(size, 0);
		handle.resizeArray(size + 1);
		if (handle.getChildHandle(size, memberHandle) == NvParameterized::ERROR_NONE)
		{
			memberHandle.initParamRef(DestructibleDebugRenderParams::staticClassName(), true);
		}
	}

	/* Load reference to DestructibleDebugRenderParams */
	NvParameterized::Interface* refPtr = NULL;
	memberHandle.getParamRef(refPtr);
	mDestructibleDebugRenderParams = DYNAMIC_CAST(DestructibleDebugRenderParams*)(refPtr);
	PX_ASSERT(mDestructibleDebugRenderParams);

	if (mModule->isInitialized())
	{
		// when scene is created after the module
		initModuleSettings();
	}
	setMassScaling(mModule->m_massScale, mModule->m_scaledMassExponent);

	createModuleStats();
}

void DestructibleScene::initModuleSettings()
{
	/* Initialize module defaults */
	setMassScaling(mModule->m_massScale, mModule->m_scaledMassExponent);
}

DestructibleScene::~DestructibleScene()
{
#if APEX_RUNTIME_FRACTURE
	PX_DELETE(mRTScene);
#endif

	delete(DestructibleBeforeTick*) mBeforeTickTask;

	destroyModuleStats();
}

void DestructibleScene::destroy()
{
	removeAllActors();
	reset();
	PX_ASSERT(mAwakeActors.usedCount() == 0); // if there are actors left in here... thats very bad indeed.
	mApexScene->removeModuleUserNotifier(mUserNotify);
	mApexScene->removeModuleUserContactModify(mContactModify);
	mApexScene->moduleReleased(*this);

	// The order of user callbacks being modified needed to change due to a crash (DE9025)
#if APEX_RUNTIME_FRACTURE
	if(mRTScene)
	{
		mRTScene->restoreUserCallbacks();
	}
#endif

	delete this;
}

void DestructibleScene::setModulePhysXScene(PxScene* pxScene)
{
	if (pxScene == mPhysXScene)
	{
		return;
	}

	mPhysXScene = pxScene;
	mSceneClientIDs.reset();

	if (pxScene)
	{
#if USE_ACTIVE_TRANSFORMS_FOR_AWAKE_LIST
		SCOPED_PHYSX_LOCK_READ(mApexScene);
		mUsingActiveTransforms = (mPhysXScene->getFlags() & PxSceneFlag::eENABLE_ACTIVETRANSFORMS);
#endif
		// Actors will make calls back to add themselves to structures
		for (uint32_t i = 0 ; i < mActorArray.size() ; i++)
		{
			mActorArray[i]->setPhysXScene(pxScene);
		}
	}
	else
	{
		mUsingActiveTransforms = false;

		/* Release all destructible structures.  TODO - this is not an optimal way to do this */
		for (uint32_t i = 0 ; i < mActorArray.size() ; i++)
		{
			mActorArray[i]->setPhysXScene(0);
		}

		tasked_beforeTick(0.0f);
	}
}

#if APEX_RUNTIME_FRACTURE
::nvidia::fracture::SimScene* DestructibleScene::getDestructibleRTScene(bool create)
{
	if(mRTScene == NULL && create)
	{
		READ_LOCK(*mApexScene);
		mRTScene = nvidia::fracture::SimScene::createSimScene(&mApexScene->getPhysXScene()->getPhysics(),GetApexSDK()->getCookingInterface(),mPhysXScene,0.02f,NULL,NULL);
		mRTScene->clear();
	}
	return mRTScene;
}
#endif

DestructibleActorJoint* DestructibleScene::createDestructibleActorJoint(const DestructibleActorJointDesc& destructibleActorJointDesc)
{
	return PX_NEW(DestructibleActorJointProxy)(destructibleActorJointDesc, *this, mDestructibleActorJointList);
}

PX_INLINE float square(float x)
{
	return x * x;
}

bool DestructibleScene::insertDestructibleActor(DestructibleActor* nxdestructible)
{
	DestructibleActorImpl* destructible = &((DestructibleActorProxy*)nxdestructible)->impl;
	if (destructible->getStructure())
	{
		return false;
	}

	const DestructibleActorParam* p = destructible->getParams();
	const physx::PxClientID clientID = p->p3ActorDescTemplate.ownerClient;
	if (mSceneClientIDs.find(clientID) == mSceneClientIDs.end())
	{
		mSceneClientIDs.pushBack(clientID);
	}

	const float paddingFactor = destructible->getDestructibleAsset()->mParams->neighborPadding;

	physx::Array<DestructibleStructure*> overlappedStructures;

	const float padding = (destructible->getOriginalBounds().maximum - destructible->getOriginalBounds().minimum).magnitude() * paddingFactor;

	const bool formsExtendedStructures =  !destructible->isInitiallyDynamic()     &&
	                                       destructible->formExtendedStructures();

	uint32_t destructibleCount = 0;
	DestructibleStructure* structureToUse = NULL;

	if (formsExtendedStructures)
	{
		// Static actor
		const PxBounds3& box = destructible->getOriginalBounds();

		// Find structures that this actor touches
		for (uint32_t structureNum = 0; structureNum < mStructures.usedCount(); ++structureNum)
		{
			DestructibleStructure* structure = mStructures.getUsed(structureNum);
			if (structure->destructibles.size() == 0)
			{
				continue;
			}
			DestructibleActorImpl* firstDestructible = structure->destructibles[0];
			if (firstDestructible->isInitiallyDynamic() || !firstDestructible->formExtendedStructures())
			{
				continue;
			}
			// TODO: Support structure selection by hashing structure when serializing actor
			// We can hook up with this structure
			for (uint32_t destructibleIndex = 0; destructibleIndex < structure->destructibles.size(); ++destructibleIndex)
			{
				DestructibleActorImpl* existingDestructible = structure->destructibles[destructibleIndex];
				const PxBounds3& existingDestructibleBox = existingDestructible->getOriginalBounds();
				if (box.intersects(existingDestructibleBox))
				{
					if (DestructibleAssetImpl::chunksInProximity(*destructible->getDestructibleAsset(), 0, PxTransform(destructible->getInitialGlobalPose()), destructible->getScale(),
					        *existingDestructible->getDestructibleAsset(), 0, PxTransform(existingDestructible->getInitialGlobalPose()), existingDestructible->getScale(), padding))
					{
						// Record individual actor touches for neighbor list
						if (structureToUse == NULL || structureToUse->destructibles.size() < structure->destructibles.size())
						{
							structureToUse = structure;
						}
						overlappedStructures.pushBack(structure);
						destructibleCount += structure->destructibles.size();
						break;
					}
				}
			}
		}
	}

	physx::Array<DestructibleActorImpl*> destructiblesToAdd;

	if (structureToUse == NULL)
	{
		// Need to make a structure for this destructible
		destructiblesToAdd.pushBack(destructible);
		uint32_t structureID = UINT32_MAX;
		if (!mStructures.useNextFree(structureID))
		{
			PX_ASSERT(!"Could not create a new structure ID.\n");
			return false;
		}
		structureToUse = PX_NEW(DestructibleStructure)(this, structureID);
		mStructures.direct(structureID) = structureToUse;
	}
	else
	{
		// We may re-use one of the overlappedStructures, and delete the rest
		destructiblesToAdd.resize(destructibleCount - structureToUse->destructibles.size() + 1);	// Exclude the ones in the structure already, include the one we're adding
		uint32_t destructibleIndexOffset = 0;
		for (uint32_t i = 0; i < overlappedStructures.size(); ++i)
		{
			DestructibleStructure* structure = overlappedStructures[i];
			if (structure == structureToUse)
			{
				continue;
			}
			memcpy(&destructiblesToAdd[destructibleIndexOffset], structure->destructibles.begin(), structure->destructibles.size()*sizeof(DestructibleActorImpl*));
			destructibleIndexOffset += structure->destructibles.size();
			mStructures.free(structure->ID);
			mStructures.direct(structure->ID) = NULL;
			delete structure;
		}
		PX_ASSERT(destructibleIndexOffset == destructiblesToAdd.size() - 1);
		// Finally, add our new destructible to the list
		destructiblesToAdd[destructibleIndexOffset] = destructible;
	}

	return structureToUse->addActors(destructiblesToAdd);
}

void DestructibleScene::reset()
{
	//===SyncParams===
	mDeprioritisedFractureBuffer.erase();

	// Fracture buffer
	mFractureBuffer.erase();

	// Damage buffer
	getDamageWriteBuffer().erase();
	getDamageReadBuffer().erase();

	// FIFO
	mDynamicActorFIFONum = 0;
	mTotalChunkCount = 0;
	mActorFIFO.reset();

	// Dormant list
	mDormantActors.clear();

	// Level-specific arrays
	mStructureKillList.reset();
	for (uint32_t actorKillIndex = 0; actorKillIndex < mActorKillList.size(); ++actorKillIndex)
	{
		PxRigidDynamic* actor = mActorKillList[actorKillIndex];
		PX_ASSERT(actor);
		if (actor)
		{
			releasePhysXActor(*actor);
		}
	}
	mActorKillList.reset();
	mDamageEventReportData.reset();	// DestructibleScene::reset should delete all DestructibleActors, too, so we shouldn't have bad DestructibleActor::mDamageEventReportIndex values floating around
	mChunkReportHandles.reset();

	// Structure container
	for (uint32_t i = mStructures.usedCount(); i--;)
	{
		uint32_t index = mStructures.usedIndices()[i];
		delete mStructures.direct(index);
		mStructures.free(index);
	}

	mStructures.~Bank<DestructibleStructure*, uint32_t>();
	PX_PLACEMENT_NEW(&mStructures, (Bank<DestructibleStructure*, uint32_t>))();

	mStructureUpdateList.~Bank<DestructibleStructure*, uint32_t>();
	PX_PLACEMENT_NEW(&mStructureUpdateList, (Bank<DestructibleStructure*, uint32_t>))();

	mStructureSupportRebuildList.~Bank<DestructibleStructure*, uint32_t>();
	PX_PLACEMENT_NEW(&mStructureSupportRebuildList, (Bank<DestructibleStructure*, uint32_t>))();

	uint32_t apexActorKillIndex = mApexActorKillList.getSize();
	while (apexActorKillIndex--)
	{
		DestructibleActorProxy* proxy = DYNAMIC_CAST(DestructibleActorProxy*)(mApexActorKillList.getResource(apexActorKillIndex));
		PX_ASSERT(proxy);
		delete proxy;
	}

	mChunkKillList.clear();

	m_damageApplicationRaycastFlags = nvidia::DestructibleActorRaycastFlags::StaticChunks;
}

void DestructibleScene::resetEmitterActors()
{
# if APEX_USE_PARTICLES
	for (uint32_t structureNum = 0; structureNum < mStructures.usedCount(); ++structureNum)
	{
		DestructibleStructure* structure = mStructures.getUsed(structureNum);
		if (structure)
		{
			for (uint32_t destructibleIndex = 0; destructibleIndex < structure->destructibles.size(); ++destructibleIndex)
			{
				DestructibleActorImpl* destructible = structure->destructibles[destructibleIndex];
				if (!destructible)
				{
					continue;
				}

				// stop all of the crumble and dust emitters in the destructible actors
				if (destructible->getCrumbleEmitter() && destructible->getCrumbleEmitter()->isExplicitGeom())
				{
					destructible->getCrumbleEmitter()->isExplicitGeom()->resetParticleList();
				}
				if (destructible->getDustEmitter() && destructible->getDustEmitter()->isExplicitGeom())
				{
					destructible->getDustEmitter()->isExplicitGeom()->resetParticleList();
				}
			}
		}
	}
#endif
}

void DestructibleScene::submitTasks(float elapsedTime, float /*substepSize*/, uint32_t /*numSubSteps*/)
{
	PX_PROFILE_ZONE("DestructibleScene/submitTasks", GetInternalApexSDK()->getContextId());
	mFractureEventCount = 0;
	PxTaskManager* tm;
	{
		tm = mApexScene->getTaskManager();
	}
	tm->submitNamedTask(mBeforeTickTask, mBeforeTickTask->getName());
	mBeforeTickTask->setDeltaTime(elapsedTime);
}

void DestructibleScene::setTaskDependencies()
{
	PxTaskManager* tm;
	{
		tm = mApexScene->getTaskManager();
	}
	const PxTaskID physxTick = tm->getNamedTask(AST_PHYSX_SIMULATE);
	mBeforeTickTask->finishBefore(physxTick);
}

class IRLess
{
public:
	bool operator()(IndexedReal& ir1, IndexedReal& ir2) const
	{
		return ir1.value < ir2.value;
	}
};

void DestructibleScene::tasked_beforeTick(float elapsedTime)
{
	SCOPED_PHYSX_LOCK_WRITE(mApexScene);

	for (uint32_t i = 0; i < mStructureSupportRebuildList.usedCount(); ++i)
	{
		DestructibleStructure*& structure = mStructureSupportRebuildList.getUsed(i);
		structure->buildSupportGraph();
		structure = NULL;	// This is only OK because we are calling clearFast after this.  This allows setStructureSupportRebuild to operate without firing asserts.
	}
	mStructureSupportRebuildList.clearFast();

	if (m_invalidBounds.size())
	{
		for (uint32_t i = 0; i < mStructures.usedCount(); ++i)
		{
			DestructibleStructure* structure = mStructures.getUsed(i);
			if (structure != NULL)
			{
				structure->invalidateBounds(&m_invalidBounds[0], m_invalidBounds.size());
			}
		}
		m_invalidBounds.clear();
	}
	processEventBuffers();


	capDynamicActorCount();

	for (uint32_t actorKillIndex = 0; actorKillIndex < mActorKillList.size(); ++actorKillIndex)
	{
		PxRigidDynamic* actor = mActorKillList[actorKillIndex];
		PX_ASSERT(actor);
		PhysXObjectDescIntl* actorObjDesc = mModule->mSdk->getGenericPhysXObjectInfo(actor);
		if (actorObjDesc != NULL)
		{
			const uint32_t dActorCount = actorObjDesc->mApexActors.size();
			for (uint32_t i = 0; i < dActorCount; ++i)
			{
				const DestructibleActor* dActor = static_cast<const DestructibleActor*>(actorObjDesc->mApexActors[i]);
				if (dActor != NULL)
				{
					if (actorObjDesc->mApexActors[i]->getOwner()->getObjTypeID() == DestructibleAssetImpl::getAssetTypeID())
					{
						DestructibleActorImpl& destructibleActor = const_cast<DestructibleActorImpl&>(static_cast<const DestructibleActorProxy*>(dActor)->impl);
						if (destructibleActor.getStructure() != NULL)
						{
							if (actor == destructibleActor.getStructure()->actorForStaticChunks)
							{
								destructibleActor.getStructure()->actorForStaticChunks = NULL;
							}
						}
					}
				}
			}
			actorObjDesc->mApexActors.clear();
			releasePhysXActor(*actor);
		}
	}
	mActorKillList.reset();

	NvParameterized::Interface* iface;
	{
		iface = mApexScene->getDebugRenderParams();
	}

	mNumFracturesProcessedThisFrame = 0;	//reset this counter
	mNumActorsCreatedThisFrame = 0;			//reset this counter

	resetEmitterActors();

	{
		PX_PROFILE_ZONE("DestructibleRemoveChunksForBudget", GetInternalApexSDK()->getContextId());

		// Remove chunks which need eliminating to keep budget
		for (uint32_t i = 0; i < mChunkKillList.size(); ++i)
		{
			const IntPair& deadChunk = mChunkKillList[i];
			DestructibleStructure* structure = mStructures.direct((uint32_t)deadChunk.i0);
			if (structure != NULL)
			{
				DestructibleStructure::Chunk& chunk = structure->chunks[(uint32_t)deadChunk.i1];
				if (!chunk.isDestroyed())
				{
					structure->removeChunk(chunk);
				}
			}
		}
		mChunkKillList.clear();
	}

	//===SyncParams=== prepare user's fracture event buffer, if available
	UserFractureEventHandler * callback = NULL;
	callback = mModule->getSyncParams().getUserFractureEventHandler();
	const physx::Array<SyncParams::UserFractureEvent> * userSource = NULL;
	if(NULL != callback)
	{
		mSyncParams.onPreProcessReadData(*callback, userSource);
	}

	//===SyncParams=== give the user the fracture event buffer. fracture event buffer must be fully populated and locked during this call
	if(NULL != callback)
	{
		mSyncParams.onProcessWriteData(*callback, mFractureBuffer);
	}
	
	// Clear as much of the queue as we can
	processFractureBuffer();
	//===SyncParams=== process user's fracture events
	processFractureBuffer(userSource);

	// at this point all actors should have been created and added to the list for this tick
	addActorsToScene();

	//===SyncParams=== done with user's fracture event buffer, if available
	if(NULL != callback)
	{
		mSyncParams.onPostProcessReadData(*callback);
	}
	callback = NULL;

	// Process Damage coloring from forceDamageColoring()
	processDamageColoringBuffer();

	{
		PX_PROFILE_ZONE("DestructibleKillStructures", GetInternalApexSDK()->getContextId());

		for (uint32_t structureKillIndex = 0; structureKillIndex < mStructureKillList.size(); ++structureKillIndex)
		{
			DestructibleStructure* structure = mStructureKillList[structureKillIndex];
			if (structure)
			{
				for (uint32_t destructibleIndex = structure->destructibles.size(); destructibleIndex--;)
				{
					DestructibleActorImpl*& destructible = structure->destructibles[destructibleIndex];
					if (destructible)
					{
						destructible->setStructure(NULL);
						mDestructibles.direct(destructible->getID()) = NULL;
						mDestructibles.free(destructible->getID());
						destructible = NULL;
					}
				}
			}

			setStructureUpdate(structure, false);

			mStructures.free(structure->ID);
			mStructures.direct(structure->ID) = NULL;
			delete structure;
		}
		mStructureKillList.reset();
	}

	switch (getRenderLockMode())
	{
	case RenderLockMode::NO_RENDER_LOCK:
		break;
	case RenderLockMode::PER_MODULE_SCENE_RENDER_LOCK:
		lockModuleSceneRenderLock();
		break;
	case RenderLockMode::PER_ACTOR_RENDER_LOCK:
	default:
		{
			PX_PROFILE_ZONE("DestructibleBeforeTickLockRenderables", GetInternalApexSDK()->getContextId());
			for (uint32_t i = 0; i < mActorArray.size(); ++i)
			{
				mActorArray[i]->renderDataLock();
			}
		}
		break;
	}

	const uint32_t actorCount = mActorFIFO.size();

	if (mDynamicActorFIFONum > 0 && (((mModule->m_dynamicActorFIFOMax > 0 && mDynamicActorFIFONum > mModule->m_dynamicActorFIFOMax) ||
		(mModule->m_chunkFIFOMax > 0 && mTotalChunkCount > mModule->m_chunkFIFOMax)) && mModule->m_sortByBenefit))
	{
		if (mActorBenefitSortArray.size() < mActorFIFO.size())
		{
			mActorBenefitSortArray.resize(mActorFIFO.size());
		}
		for (uint32_t i = 0; i < mActorFIFO.size(); ++i)
		{
			IndexedReal& ir = mActorBenefitSortArray[i];
			ActorFIFOEntry& entry = mActorFIFO[i];
			ir.value = entry.benefitCache;
			ir.index = i;
		}
		if (mActorFIFO.size() > 1)
		{
			shdfnd::sort(&mActorBenefitSortArray[0], mActorFIFO.size(), IRLess());
		}
		uint32_t sortIndex = 0;
		if (mModule->m_dynamicActorFIFOMax > 0 && mDynamicActorFIFONum > mModule->m_dynamicActorFIFOMax)
		{
			while (sortIndex < mActorFIFO.size() && mDynamicActorFIFONum > mModule->m_dynamicActorFIFOMax)
			{
				IndexedReal& ir = mActorBenefitSortArray[sortIndex++];
				ActorFIFOEntry& entry = mActorFIFO[ir.index];
				if (entry.actor)
				{
					entry.flags |= ActorFIFOEntry::ForceLODRemove;
					--mDynamicActorFIFONum;
				}
			}
		}
		uint32_t estTotalChunkCount = mTotalChunkCount;	// This will get decremented again, in the FIFO loop below
		if (mModule->m_chunkFIFOMax > 0 && estTotalChunkCount > mModule->m_chunkFIFOMax)
		{
			while (sortIndex < mActorFIFO.size() && estTotalChunkCount > mModule->m_chunkFIFOMax)
			{
				IndexedReal& ir = mActorBenefitSortArray[sortIndex++];
				ActorFIFOEntry& entry = mActorFIFO[ir.index];
				if (entry.actor)
				{
					entry.flags |= ActorFIFOEntry::ForceLODRemove;
					const uint32_t chunkCount =  entry.actor->getNbShapes();
					estTotalChunkCount = estTotalChunkCount > chunkCount ? estTotalChunkCount - chunkCount : 0;
				}
			}
		}
	}

	mDynamicActorFIFONum = 0;
	for (uint32_t FIFOIndex = 0; FIFOIndex < actorCount; ++FIFOIndex)
	{
		ActorFIFOEntry& entry = mActorFIFO[FIFOIndex];
		if (!entry.actor)
		{
			continue;
		}
		PhysXObjectDescIntl* actorObjDesc = mModule->mSdk->getGenericPhysXObjectInfo(entry.actor);
		if (!actorObjDesc)
		{
			continue;
		}

		entry.benefitCache = 0.0f;
		entry.age += elapsedTime;
		uint32_t shapeCount;
		if (actorObjDesc->userData != NULL && (shapeCount = entry.actor->getNbShapes()) != 0)
		{
			uint32_t reasonToDestroy = 0;
			DestructibleActorImpl* destructible = NULL;
			entry.maxSpeed = PX_MAX_F32;
			float sleepVelocityFrameDecayConstant = 0.0f;
			bool useHardSleeping = false;
			for (uint32_t i = 0; i < actorObjDesc->mApexActors.size(); ++i)
			{
				DestructibleActorProxy* proxy = const_cast<DestructibleActorProxy*>(static_cast<const DestructibleActorProxy*>(actorObjDesc->mApexActors[i]));
				if (proxy == NULL)
				{
					continue;
				}
				destructible = &proxy->impl;
				const DestructibleParameters& parameters = destructible->getDestructibleParameters();
				PxVec3 islandPos;
				{
					SCOPED_PHYSX_LOCK_READ(entry.actor->getScene());
					islandPos = (entry.actor->getGlobalPose() * entry.actor->getCMassLocalPose()).p;
				}
				if (destructible->getParams()->deleteChunksLeavingUserDefinedBB)
				{
					const uint32_t bbc = mApexScene->getBoundingBoxCount();
					for(uint32_t i = 0; i < bbc; ++i)
					{
						if(mApexScene->getBoundingBoxFlags(i) & UserBoundingBoxFlags::LEAVE)
						{
							if(!mApexScene->getBoundingBox(i).contains(islandPos))
							{
								reasonToDestroy = ApexChunkFlag::DESTROYED_LEFT_USER_BOUNDS;
								break;
							}
						}
					}
				}
				if (destructible->getParams()->deleteChunksEnteringUserDefinedBB)
				{
					const uint32_t bbc = mApexScene->getBoundingBoxCount();
					for(uint32_t i = 0; i < bbc; ++i)
					{
						if(mApexScene->getBoundingBoxFlags(i) & UserBoundingBoxFlags::ENTER)
						{
							if(mApexScene->getBoundingBox(i).contains(islandPos))
							{
								reasonToDestroy = ApexChunkFlag::DESTROYED_ENTERED_USER_BOUNDS;
								break;
							}
						}
					}
				}
				if ((parameters.flags & DestructibleParametersFlag::USE_VALID_BOUNDS) != 0 &&
					!parameters.validBounds.contains(islandPos - destructible->getInitialGlobalPose().getPosition()))
				{
					reasonToDestroy = ApexChunkFlag::DESTROYED_LEFT_VALID_BOUNDS;
				}
				else if ((entry.flags & ActorFIFOEntry::ForceLODRemove) != 0)
				{
					reasonToDestroy = ApexChunkFlag::DESTROYED_FIFO_FULL;
				}
				else if ((entry.flags & ActorFIFOEntry::IsDebris) != 0)
				{
					// Check if too old or too far
					if ((parameters.flags & DestructibleParametersFlag::DEBRIS_TIMEOUT) != 0 &&
						entry.age > (parameters.debrisLifetimeMax - parameters.debrisLifetimeMin)*mModule->m_maxChunkSeparationLOD + parameters.debrisLifetimeMin)
					{
						reasonToDestroy = ApexChunkFlag::DESTROYED_TIMED_OUT;
					}
					else if ((parameters.flags & DestructibleParametersFlag::DEBRIS_MAX_SEPARATION) != 0 &&
						(entry.origin - islandPos).magnitudeSquared() >
						square((parameters.debrisMaxSeparationMax - parameters.debrisMaxSeparationMin)*mModule->m_maxChunkSeparationLOD + parameters.debrisMaxSeparationMin))
					{
						reasonToDestroy = ApexChunkFlag::DESTROYED_EXCEEDED_MAX_DISTANCE;
					}
				}
				if (reasonToDestroy)
				{
					destroyActorChunks(*entry.actor, reasonToDestroy);	// places the actor on the kill list
					entry.actor = NULL;
					destructible->wakeForEvent();
					break;
				}
				if (parameters.maxChunkSpeed > 0.0f)
				{
					entry.maxSpeed = PxMin(entry.maxSpeed, parameters.maxChunkSpeed);
				}
				sleepVelocityFrameDecayConstant = PxMax(sleepVelocityFrameDecayConstant, destructible->getSleepVelocityFrameDecayConstant());
				useHardSleeping = useHardSleeping || destructible->useHardSleeping();
			}
			if (reasonToDestroy)
			{
				continue;
			}
			// Smooth velocities and see if the actor should be put to sleep
			if (sleepVelocityFrameDecayConstant > 1.0f)
			{
				// Create smoothed velocities
				const float sleepVelocitySmoothingFactor = 1.0f-1.0f/sleepVelocityFrameDecayConstant;
				const float sleepVelocitySmoothingFactorComplement = 1.0f-sleepVelocitySmoothingFactor;
				const PxVec3 currentLinearVelocity = entry.actor->getLinearVelocity();
				const PxVec3 currentAngularVelocity = entry.actor->getAngularVelocity();
				if (!entry.actor->isSleeping())
				{
					entry.averagedLinearVelocity = sleepVelocitySmoothingFactor*entry.averagedLinearVelocity + sleepVelocitySmoothingFactorComplement*currentLinearVelocity;
					entry.averagedAngularVelocity = sleepVelocitySmoothingFactor*entry.averagedAngularVelocity + sleepVelocitySmoothingFactorComplement*currentAngularVelocity;
					handleSleeping(entry.actor, entry.averagedLinearVelocity, entry.averagedAngularVelocity);
				}
				else
				{
					// Initialize smoothed velocity so that the actor may wake up again
					entry.averagedLinearVelocity = mApexScene->getGravity();
					entry.averagedAngularVelocity = PxVec3(0.0f, 0.0f, PxTwoPi);
				}
			}
			// Cap the linear velocity here
			if (entry.maxSpeed < PX_MAX_F32)
			{
				const PxVec3 chunkVel = entry.actor->getLinearVelocity();
				const float chunkSpeed2 = chunkVel.magnitudeSquared();
				if (chunkSpeed2 > entry.maxSpeed * entry.maxSpeed)
				{
					entry.actor->setLinearVelocity((entry.maxSpeed * RecipSqrt(chunkSpeed2))*chunkVel);
				}
			}
			if (actorObjDesc->userData == NULL)	// Signals that shapes have changed, need to recalculate the mass properties
			{
				PX_ASSERT(destructible != NULL);
			}
			if (destructible != NULL)
			{
				entry.benefitCache += destructible->getBenefit() * (float)shapeCount / (float)PxMax<uint32_t>(destructible->getVisibleDynamicChunkShapeCount(), 1);
			}
			if ((entry.flags & ActorFIFOEntry::MassUpdateNeeded) != 0)
			{
				PX_ASSERT(entry.unscaledMass > 0.0f);
				if (entry.unscaledMass > 0.0f)
				{
					physx::PxRigidBodyExt::setMassAndUpdateInertia(*entry.actor, scaleMass(entry.unscaledMass), NULL, false);
				}
				entry.flags &= ~(uint32_t)ActorFIFOEntry::MassUpdateNeeded;
			}
			if (useHardSleeping && entry.actor->isSleeping())
			{
				bool isKinematic;
				{
					SCOPED_PHYSX_LOCK_READ(entry.actor->getScene());
					isKinematic = (entry.actor->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC);
				}
				if (!isKinematic)
				{
					{
						entry.actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
					}
					uint32_t dormantID = 0;
					if (mDormantActors.useNextFree(dormantID))
					{
						actorObjDesc->userData = (void*)~(uintptr_t)dormantID;
						mDormantActors.direct(dormantID) = DormantActorEntry(entry.actor, entry.unscaledMass, entry.flags);
					}
					else
					{
						actorObjDesc->userData = NULL;
					}
				}
				entry.actor = NULL;
				continue;
			}
			if (mDynamicActorFIFONum != FIFOIndex)
			{
				actorObjDesc->userData = (void*)~(uintptr_t)mDynamicActorFIFONum;
				mActorFIFO[mDynamicActorFIFONum] = entry;
			}
			++mDynamicActorFIFONum;
		}
		else
		{
			schedulePxActorForDelete(*actorObjDesc);
			entry.actor = NULL;
		}
	}
	mActorFIFO.resize(mDynamicActorFIFONum);

	mElapsedTime += elapsedTime;


	for (uint32_t structureNum = mStructureUpdateList.usedCount(); structureNum--;)
	{
		DestructibleStructure* structure = mStructureUpdateList.getUsed(structureNum);
		structure->updateIslands();
		setStructureUpdate(structure, false);
	}


	for (uint32_t structureNum = mStressSolverTickList.usedCount(); structureNum--;)
	{
		DestructibleStructure* structure = mStressSolverTickList.getUsed(structureNum);
		structure->tickStressSolver(elapsedTime);
	}


	switch (getRenderLockMode())
	{
	case RenderLockMode::NO_RENDER_LOCK:
		break;
	case RenderLockMode::PER_MODULE_SCENE_RENDER_LOCK:
		unlockModuleSceneRenderLock();
		break;
	case RenderLockMode::PER_ACTOR_RENDER_LOCK:
	default:
		{
			PX_PROFILE_ZONE("DestructibleBeforeTickLockRenderables", GetInternalApexSDK()->getContextId());
			for (uint32_t i = 0; i < mActorArray.size(); ++i)
			{
				mActorArray[i]->renderDataUnLock();
			}
		}
		break;
	}

	for (uint32_t jointIndex = mDestructibleActorJointList.getSize(); jointIndex--;)
	{
		DestructibleActorJointProxy* jointProxy =
		    DYNAMIC_CAST(DestructibleActorJointProxy*)(mDestructibleActorJointList.getResource(jointIndex));
		PX_ASSERT(jointProxy != NULL);
		if (jointProxy != NULL)
		{
			bool result = jointProxy->impl.updateJoint();
			if (!result)
			{
				jointProxy->release();
			}
		}
	}
#if APEX_RUNTIME_FRACTURE
	fracture::SimScene* simScene = getDestructibleRTScene(false);
	if (simScene != NULL)
	{
		simScene->preSim(elapsedTime);
	}
#endif

	if (mApexScene->isFinalStep())
	{
		if (mModule->m_chunkReport)
		{
			PX_PROFILE_ZONE("DestructibleChunkReport", GetInternalApexSDK()->getContextId());
			if (mModule->m_chunkStateEventCallbackSchedule == DestructibleCallbackSchedule::BeforeTick)
			{
				for (uint32_t actorWithChunkStateEventIndex = mActorsWithChunkStateEvents.size(); actorWithChunkStateEventIndex--;)
				{
					ChunkStateEventData data;
					DestructibleActorImpl* dactor = mActorsWithChunkStateEvents[actorWithChunkStateEventIndex];
					data.destructible = dactor->getAPI();
					if (dactor->acquireChunkEventBuffer(data.stateEventList, data.stateEventListSize))
					{
						mModule->m_chunkReport->onStateChangeNotify(data);
						dactor->releaseChunkEventBuffer();
					}
				}
				//mActorsWithChunkStateEvents.clear();
			}
		}
	}

	// Update mUsingActiveTransforms, in case the user has changed it (PhysX3 only)
#if USE_ACTIVE_TRANSFORMS_FOR_AWAKE_LIST
	SCOPED_PHYSX_LOCK_READ(mApexScene);
	mUsingActiveTransforms = mPhysXScene != NULL && (mPhysXScene->getFlags() & PxSceneFlag::eENABLE_ACTIVETRANSFORMS);
#endif
}

void DestructibleScene::setStructureSupportRebuild(DestructibleStructure* structure, bool rebuild)
{
	if (rebuild)
	{
		if (mStructureSupportRebuildList.use(structure->ID))
		{
			mStructureSupportRebuildList.direct(structure->ID) = structure;
		}
		else
		{
			PX_ASSERT(mStructureSupportRebuildList.direct(structure->ID) == structure);
		}
	}
	else
	{
		if (mStructureSupportRebuildList.free(structure->ID))
		{
			mStructureSupportRebuildList.direct(structure->ID) = NULL;
		}
		else
		{
			PX_ASSERT(mStructureSupportRebuildList.direct(structure->ID) == NULL);
		}
	}
}

void DestructibleScene::setStructureUpdate(DestructibleStructure* structure, bool update)
{
	if (update)
	{
		if (mStructureUpdateList.use(structure->ID))
		{
			mStructureUpdateList.direct(structure->ID) = structure;
		}
		else
		{
			PX_ASSERT(mStructureUpdateList.direct(structure->ID) == structure);
		}
	}
	else
	{
		if (mStructureUpdateList.free(structure->ID))
		{
			mStructureUpdateList.direct(structure->ID) = NULL;
		}
		else
		{
			PX_ASSERT(mStructureUpdateList.direct(structure->ID) == NULL);
		}
	}
}


void DestructibleScene::setStressSolverTick(DestructibleStructure* structure, bool update)
{
	if (update)
	{
		if (mStressSolverTickList.use(structure->ID))
		{
			mStressSolverTickList.direct(structure->ID) = structure;
		}
		else
		{
			PX_ASSERT(mStressSolverTickList.direct(structure->ID) == structure);
		}
	}
	else
	{
		if (mStressSolverTickList.free(structure->ID))
		{
			mStressSolverTickList.direct(structure->ID) = NULL;
		}
		else
		{
			PX_ASSERT(mStressSolverTickList.direct(structure->ID) == NULL);
		}
	}
}


void DestructibleScene::fetchResults()
{
	PX_PROFILE_ZONE("DestructibleFetchResults", GetInternalApexSDK()->getContextId());

	// beforeTick() should have deleted all of the PxActors.  It should be safe now to delete the Apex actors
	// since there should be no more PhysXObjectDesc which reference this actor.
	uint32_t apexActorKillIndex = mApexActorKillList.getSize();
	while (apexActorKillIndex--)
	{
		DestructibleActorProxy* proxy = DYNAMIC_CAST(DestructibleActorProxy*)(mApexActorKillList.getResource(apexActorKillIndex));
		PX_ASSERT(proxy);
		delete proxy;
	}

	{
		PX_PROFILE_ZONE("DestructibleUpdateRenderMeshBonePoses", GetInternalApexSDK()->getContextId());

		// Reset instanced mesh buffers
		for (uint32_t i = 0; i < mInstancedActors.size(); ++i)
		{
			DestructibleActorImpl* actor = mInstancedActors[i];
			if (actor->m_listIndex == 0)
			{
				actor->getDestructibleAsset()->resetInstanceData();
			}
		}

		//===SyncParams=== prepare user's chunk motion buffer, if available
		UserChunkMotionHandler * callback = NULL;
		callback = mModule->getSyncParams().getUserChunkMotionHandler();
		if(NULL != callback)
		{
			mSyncParams.onPreProcessReadData(*callback);
		}

		for (uint32_t i = 0; i < mInstancedActors.size(); ++i)
		{
			DestructibleActorImpl* actor = mInstancedActors[i];
			actor->fillInstanceBuffers();
		}

		if (mUsingActiveTransforms)
		{
			// in AT mode, mAwakeActors is only used for temporarily storing actors that need update
			// a frame history is kept to prevent wake/sleep event chatter
			// TODO: update only the actually moving chunks. [APEX-670]
			// with the current mechanism, all actor's chunks are updated regardless of active transforms

			SCOPED_PHYSX_LOCK_READ(mPhysXScene);
			for (uint32_t clientNum = 0; clientNum < mSceneClientIDs.size(); ++clientNum)
			{
				uint32_t transformCount = 0;
				const physx::PxActiveTransform* activeTransforms = mPhysXScene->getActiveTransforms(transformCount, mSceneClientIDs[clientNum]);
				for (uint32_t i = 0; i < transformCount; ++i)
				{
					PhysXObjectDescIntl* actorObjDesc = (PhysXObjectDescIntl*)mModule->mSdk->getPhysXObjectInfo(activeTransforms[i].actor);
					if (actorObjDesc != NULL)
					{
						for (uint32_t j = 0; j < actorObjDesc->mApexActors.size(); ++j)
						{
							DestructibleActorProxy* dActor = (DestructibleActorProxy*)actorObjDesc->mApexActors[j];
							if (dActor != NULL)
							{
								// ignore duplicate entries on purpose
								if (dActor->impl.mActiveFrames == 0)	// Hasn't been recorded as active yet
								{
									dActor->impl.incrementWakeCount();
								}
								dActor->impl.mActiveFrames |= 1;	// Record as active this frame
							}
						}
					}
#if APEX_RUNTIME_FRACTURE
					else
					{
						nvidia::fracture::SimScene* simScene = getDestructibleRTScene(false);
						if (simScene != NULL)
						{
							physx::PxActor* actor = activeTransforms[i].actor;
							if (actor != NULL)
							{
								physx::PxRigidDynamic* rigidDynamic = actor->is<physx::PxRigidDynamic>();
								if (rigidDynamic != NULL)
								{
									if (rigidDynamic->getNbShapes() > 0)
									{
										physx::PxShape* firstShape;
										if (rigidDynamic->getShapes(&firstShape, 1, 0) && firstShape != NULL)
										{
											nvidia::fracture::Convex* convex = (nvidia::fracture::Convex*)simScene->findConvexForShape(*firstShape);
											if (simScene->owns(*firstShape))
											{
												nvidia::fracture::Compound* compound = (nvidia::fracture::Compound*)convex->getParent();
												// ignore duplicate entries on purpose
												if (compound->getDestructibleActor()->mActiveFrames == 0)	// Hasn't been recorded as active yet
												{
													compound->getDestructibleActor()->incrementWakeCount();
												}
												compound->getDestructibleActor()->mActiveFrames |= 1;	// Record as active this frame
											}
										}
									}
								}
							}
						}
					}
#endif
				}
			}
		}

		// Iterate backwards through mAwakeActors, since an IndexBank used-list may have entries freed during iteration (as a result of actor->decrementWakeCount() or actor->resetWakeForEvent())
		for (uint32_t awakeActorNum = mAwakeActors.usedCount(); awakeActorNum--;)
		{
			DestructibleActorImpl* actor = mDestructibles.direct(mAwakeActors.usedIndices()[awakeActorNum]);
			if (mUsingActiveTransforms)
			{
				if (actor->mActiveFrames == 2)	// Active last frame, not active this frame
				{
					actor->decrementWakeCount();
					if (actor->mAwakeActorCount == 0)
					{
						actor->mActiveFrames = 0;	// The result of the skipped shift & mask, below
						continue;
					}
				}
				actor->mActiveFrames = (actor->mActiveFrames << 1) & 3;	// Shift up and erase history past last frame
			}

			updateActorPose(actor, callback);
			actor->resetWakeForEvent();

			if (actor->getNumVisibleChunks() == 0)
			{
				if (getModuleDestructible()->m_chunkReport != NULL)
				{
					if (getModuleDestructible()->m_chunkReport->releaseOnNoChunksVisible(actor->getAPI()))
					{
						destructibleActorKillList.pushBack(actor->getAPI());
					}
				}
			}
		}

		//===SyncParams=== give the user the chunk motion buffer. chunk motion buffer must be fully populated and locked during this call
		if(NULL != callback)
		{
			mSyncParams.onProcessWriteData(*callback);
		}

		//===SyncParams=== done with user's chunk event motion, if available
		if(NULL != callback)
		{
			mSyncParams.onPostProcessReadData(*callback);
		}
		callback = NULL;

		//===SyncParams=== allow user from changing sync params again. do this just after the last encounter of working with sync params in the program
		PX_ASSERT(mSyncParams.lockSyncParams);
		mSyncParams.lockSyncParams = false;

#if 0
		/* Update bone boses in the render mesh, update world bounds */
		for (uint32_t i = 0 ; i < mAwakeActors.size() ; i++)
		{
			DestructibleActor* actor = DYNAMIC_CAST(DestructibleActor*)(mAwakeActors[ i ]);
			if (actor->getStructure())
			{
				actor->setRenderTMs();
			}
		}
#endif
	}

	if (mApexScene->isFinalStep())
	{
		if (mModule->m_chunkReport)
		{
			PX_PROFILE_ZONE("DestructibleChunkReport", GetInternalApexSDK()->getContextId());

			// Chunk damage reports
			for (uint32_t reportNum = 0; reportNum < mDamageEventReportData.size(); ++reportNum)
			{
				ApexDamageEventReportDataImpl& data = mDamageEventReportData[reportNum];
				if (data.totalNumberOfFractureEvents > 0)
				{
					mModule->m_chunkReport->onDamageNotify(data);
				}
				data.clearChunkReports();
				((DestructibleActorProxy*)data.destructible)->impl.mDamageEventReportIndex = 0xFFFFFFFF;
			}

			// Chunk state notifies
			if (mModule->m_chunkStateEventCallbackSchedule == DestructibleCallbackSchedule::FetchResults)
			{
				for (uint32_t actorWithChunkStateEventIndex = mActorsWithChunkStateEvents.size(); actorWithChunkStateEventIndex--;)
				{
					ChunkStateEventData data;
					DestructibleActorImpl* dactor = mActorsWithChunkStateEvents[actorWithChunkStateEventIndex];
					data.destructible = dactor->getAPI();
					if (dactor->acquireChunkEventBuffer(data.stateEventList, data.stateEventListSize))
					{
						mModule->m_chunkReport->onStateChangeNotify(data);
						dactor->releaseChunkEventBuffer();
					}
				}
				//mActorsWithChunkStateEvents.clear();
			}

			// Destructible actor wake/sleep reports
			if (mOnWakeActors.size())
			{
				mModule->m_chunkReport->onDestructibleWake(&mOnWakeActors[0], mOnWakeActors.size());
			}
			if (mOnSleepActors.size())
			{
				mModule->m_chunkReport->onDestructibleSleep(&mOnSleepActors[0], mOnSleepActors.size());
			}
		}
		mDamageEventReportData.clear();
		mChunkReportHandles.clear();
		mOnWakeActors.clear();
		mOnSleepActors.clear();

		// Kill destructibles on death row
		for (uint32_t i = 0; i < destructibleActorKillList.size(); ++i)
		{
			if (destructibleActorKillList[i] != NULL)
			{
				destructibleActorKillList[i]->release();
			}
		}
		destructibleActorKillList.resize(0);
	}

	if (mModule->m_impactDamageReport)
	{
		if (mImpactDamageEventData.size() != 0)
		{
			mModule->m_impactDamageReport->onImpactDamageNotify(&mImpactDamageEventData[0], mImpactDamageEventData.size());
		}
	}
	mImpactDamageEventData.clear();

#if 0 //dead code
	// here all scenes have finished beforeTick for sure, so we know all the PxActors have been deleted
	// TODO: can that happen: beforeTick, releaseActor, releaseAsset, fetchResults ? that would still cause a problem.
	mModule->releaseBufferedConvexMeshes();
#endif

#if APEX_RUNTIME_FRACTURE
	fracture::SimScene* simScene = getDestructibleRTScene(false);
	if (simScene != NULL)
	{
		simScene->postSim(PX_MAX_F32);
	}
#endif

	swapDamageBuffers();
}


void DestructibleScene::visualize()
{
#ifdef WITHOUT_DEBUG_VISUALIZE
#else
	if (!mDestructibleDebugRenderParams->VISUALIZE_DESTRUCTIBLE_ACTOR)
	{
		return;
	}

	using RENDER_DEBUG::DebugColors;
	using RENDER_DEBUG::DebugRenderState;
	// save the rendering state
	const physx::PxMat44& savedPose = *RENDER_DEBUG_IFACE(mDebugRender)->getPoseTyped();
	RENDER_DEBUG_IFACE(mDebugRender)->setIdentityPose();
	RENDER_DEBUG_IFACE(mDebugRender)->pushRenderState();
	if (mDestructibleDebugRenderParams->VISUALIZE_DESTRUCTIBLE_BOUNDS > 0)
	{
		RENDER_DEBUG_IFACE(mDebugRender)->setCurrentColor(RENDER_DEBUG_IFACE(mDebugRender)->getDebugColor(DebugColors::Red));
		for (uint32_t i = 0 ; i < mActorArray.size() ; i++)
		{
			DestructibleActorImpl* actor = DYNAMIC_CAST(DestructibleActorImpl*)(mActorArray[ i ]);
			PxBounds3 bounds = actor->getBounds();
			RENDER_DEBUG_IFACE(mDebugRender)->debugBound(bounds);
		}
	}

	if (mDestructibleDebugRenderParams->VISUALIZE_DESTRUCTIBLE_SUPPORT > 0)
	{
		for (uint32_t structureNum = 0; structureNum < mStructures.usedCount(); ++structureNum)
		{
			DestructibleStructure* structure = mStructures.getUsed(structureNum);
			if (structure)
			{
				structure->visualizeSupport(mDebugRender);
			}
		}
	}

	// debug visualization
	for (uint32_t i = 0 ; i < mActorArray.size() ; i++)
	{
		DestructibleActorImpl* actor = DYNAMIC_CAST(DestructibleActorImpl*)(mActorArray[ i ]);
		const uint16_t*	visibleChunkIndexContainer	= actor->getVisibleChunks();
		const uint32_t visibleChunkIndexCount = actor->getNumVisibleChunks();
		const PxVec3&	eyePos						= mApexScene->getEyePosition();

		for (uint32_t chunkIndex = 0 ; chunkIndex < visibleChunkIndexCount; ++chunkIndex)
		{
			PxMat44	chunkPose(actor->getChunkPose(visibleChunkIndexContainer[chunkIndex]));
			float	disToEye	= (-eyePos + chunkPose.getPosition()).magnitude();

			if (visibleChunkIndexCount == 1 && visibleChunkIndexContainer[0] == 0)
			{
				// visualize actor pose
				if (mDestructibleDebugRenderParams->VISUALIZE_DESTRUCTIBLE_ACTOR_POSE &&
				        disToEye < mDestructibleDebugRenderParams->THRESHOLD_DISTANCE_DESTRUCTIBLE_ACTOR_POSE)
				{
					RENDER_DEBUG_IFACE(mDebugRender)->debugAxes(chunkPose, 1, 1);
				}

				// visualize actor name
				if (mDestructibleDebugRenderParams->VISUALIZE_DESTRUCTIBLE_ACTOR_NAME &&
				        disToEye < mDestructibleDebugRenderParams->THRESHOLD_DISTANCE_DESTRUCTIBLE_ACTOR_NAME)
				{
					float shiftDistance		= actor->getBounds().getExtents().magnitude();
					PxVec3 shiftDirection	= -1 * mApexScene->getViewMatrix(0).column1.getXYZ();
					PxVec3 textLocation		= chunkPose.getPosition() + (shiftDistance * shiftDirection);

					RENDER_DEBUG_IFACE(mDebugRender)->addToCurrentState(DebugRenderState::CameraFacing);
					RENDER_DEBUG_IFACE(mDebugRender)->debugText(textLocation, "Destructible");
					RENDER_DEBUG_IFACE(mDebugRender)->removeFromCurrentState(DebugRenderState::CameraFacing);
				}
			}
			else
			{
				// visualize actor pose (fragmented)
				if (mDestructibleDebugRenderParams->VISUALIZE_DESTRUCTIBLE_FRAGMENT_POSE &&
				        disToEye < mDestructibleDebugRenderParams->THRESHOLD_DISTANCE_DESTRUCTIBLE_FRAGMENT_POSE)
				{
#if 0
					RENDER_DEBUG_IFACE(mDebugRender)->debugAxes(chunkPose, 0.5, 0.7);
#endif //lionel: todo: chunk pose is incorrect
				}
			}
		}
	}

	// visualize render mesh actor
	for (uint32_t i = 0 ; i < mActorArray.size() ; i++)
	{
		DestructibleActorImpl* currentActor = NULL;
		currentActor = DYNAMIC_CAST(DestructibleActorImpl*)(mActorArray[i]);
		PX_ASSERT(currentActor != NULL);

		for (uint32_t meshTypeIndex = 0; meshTypeIndex < DestructibleActorMeshType::Count; ++meshTypeIndex)
		{
			const RenderMeshActorIntl* currentMeshActor = NULL;
			currentMeshActor = static_cast<RenderMeshActorIntl*>(const_cast<RenderMeshActor*>(currentActor->getRenderMeshActor(static_cast<DestructibleActorMeshType::Enum>(meshTypeIndex)))); //lionel: const_cast bad!
			if (NULL == currentMeshActor)
			{
				continue;
			}
			PX_ASSERT(mDebugRender != NULL);
			currentMeshActor->visualize(*mDebugRender, mDebugRenderParams);
		}

		// Instancing
		if (currentActor->m_listIndex == 0)
		{
			for (uint32_t j = 0; j < currentActor->getDestructibleAsset()->m_instancedChunkRenderMeshActors.size(); ++j)
			{
				PX_ASSERT(j < currentActor->getDestructibleAsset()->m_chunkInstanceBufferData.size());
				if (currentActor->getDestructibleAsset()->m_instancedChunkRenderMeshActors[j] != NULL && currentActor->getDestructibleAsset()->m_chunkInstanceBufferData[j].size() > 0)
				{
					uint32_t count = currentActor->getDestructibleAsset()->m_chunkInstanceBufferData[j].size();
					PxMat33* scaledRotations = &currentActor->getDestructibleAsset()->m_chunkInstanceBufferData[j][0].scaledRotation;
					PxVec3* translations = &currentActor->getDestructibleAsset()->m_chunkInstanceBufferData[j][0].translation;

					const RenderMeshActorIntl* currentMeshActor = NULL;
					currentMeshActor = static_cast<RenderMeshActorIntl*>(const_cast<RenderMeshActor*>(currentActor->getDestructibleAsset()->m_instancedChunkRenderMeshActors[j]));
					if (NULL == currentMeshActor)
					{
						continue;
					}
					PX_ASSERT(mDebugRender != NULL);
					currentMeshActor->visualize(*mDebugRender, mDebugRenderParams, scaledRotations, translations, sizeof(DestructibleAssetImpl::ChunkInstanceBufferDataElement), count);
				}
			}
		}
	}

	// restore the rendering state
	RENDER_DEBUG_IFACE(mDebugRender)->setPose(savedPose);
	RENDER_DEBUG_IFACE(mDebugRender)->popRenderState();
#endif
}

//	Private interface, used by Destructible* classes

DestructibleActor* DestructibleScene::getDestructibleAndChunk(const PxShape* shape, int32_t* chunkIndex) const
{
	if (chunkIndex)
	{
		*chunkIndex = ModuleDestructibleConst::INVALID_CHUNK_INDEX;
	}

	if (!mModule->owns(shape->getActor()))
	{
		return NULL;
	}

	DestructibleStructure::Chunk* chunk = getChunk((const PxShape*)shape);
	if (chunk == NULL)
	{
		return NULL;
	}

	DestructibleActorImpl* destructible = mDestructibles.direct(chunk->destructibleID);

	if (destructible != NULL)
	{
		if (chunkIndex)
		{
			*chunkIndex = (int32_t)chunk->indexInAsset;
		}
		return destructible->getAPI();
	}

	return NULL;
}

bool DestructibleScene::removeStructure(DestructibleStructure* structure, bool immediate)
{
	if (structure)
	{
		if (!immediate)
		{
			mStructureKillList.pushBack(structure);
		}
		else
		{
			for (uint32_t destructibleIndex = structure->destructibles.size(); destructibleIndex--;)
			{
				DestructibleActorImpl*& destructible = structure->destructibles[destructibleIndex];
				if (destructible)
				{
					destructible->setStructure(NULL);
					mDestructibles.direct(destructible->getID()) = NULL;
					mDestructibles.free(destructible->getID());
					destructible = NULL;
				}
			}

			setStructureUpdate(structure, false);

			mStructures.free(structure->ID);
			mStructures.direct(structure->ID) = NULL;
			delete structure;
		}
		return true;
	}

	return false;
}

void DestructibleScene::addActor(PhysXObjectDescIntl& desc, PxRigidDynamic& actor, float unscaledMass, bool isDebris)
{
	uintptr_t& cindex = (uintptr_t&)desc.userData;

	SCOPED_PHYSX_LOCK_READ(actor.getScene());

	if (actor.getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC && cindex != 0)
	{
		// Remove from dormant list
		const uint32_t index = (uint32_t)~cindex; 
		if (mDormantActors.free(index))
		{
			mDormantActors.direct(index).actor = NULL;
		}
		cindex = 0;
	}

	if (cindex != 0)
	{
		PX_ASSERT(!"Attempting to add an actor twice.\n");
		return;
	}

	// Add to FIFO
	cindex = ~(uintptr_t)mActorFIFO.size();
	uint32_t entryFlags = 0;
	if (isDebris)
	{
		entryFlags |= ActorFIFOEntry::IsDebris;
	}
	mActorFIFO.pushBack(ActorFIFOEntry(&actor, unscaledMass, entryFlags));

	// Initialize smoothed velocity so as not to get immediate sleeping
	ActorFIFOEntry& fifoEntry = mActorFIFO.back();
	fifoEntry.averagedLinearVelocity = mApexScene->getGravity();
	fifoEntry.averagedAngularVelocity = PxVec3(0.0f, 0.0f, PxTwoPi);

	++mDynamicActorFIFONum;
}

void DestructibleScene::capDynamicActorCount()
{
	if ((mModule->m_dynamicActorFIFOMax > 0 || mModule->m_chunkFIFOMax > 0) && !mModule->m_sortByBenefit)
	{
		while (mDynamicActorFIFONum > 0 && ((mModule->m_dynamicActorFIFOMax > 0 && mDynamicActorFIFONum > mModule->m_dynamicActorFIFOMax) ||
		        (mModule->m_chunkFIFOMax > 0 && mTotalChunkCount > mModule->m_chunkFIFOMax)))
		{
			ActorFIFOEntry& entry = mActorFIFO[mActorFIFO.size() - mDynamicActorFIFONum--];
			if (entry.actor != NULL)
			{
				destroyActorChunks(*entry.actor, ApexChunkFlag::DESTROYED_FIFO_FULL);
				entry.actor = NULL;
			}
		}
	}
}

void DestructibleScene::removeReferencesToActor(DestructibleActorImpl& destructible)
{
	if (destructible.getStructure() == NULL)
	{
		return;
	}

	mApexScene->lockRead(__FILE__, __LINE__);

	// Remove from FIFO or dormant list
	const uint16_t* chunkIndexPtr = destructible.getVisibleChunks();
	const uint16_t* chunkIndexPtrStop = chunkIndexPtr + destructible.getNumVisibleChunks();
	while (chunkIndexPtr < chunkIndexPtrStop)
	{
		uint16_t chunkIndex = *chunkIndexPtr++;
		DestructibleStructure::Chunk& chunk = destructible.getStructure()->chunks[chunkIndex + destructible.getFirstChunkIndex()];

		PxRigidDynamic* actor = destructible.getStructure()->getChunkActor(chunk);
		if (actor == NULL)
		{
			continue;
		}

		if ((chunk.state & ChunkDynamic) != 0)
		{
			const PhysXObjectDesc* desc = mModule->mSdk->getPhysXObjectInfo(actor);
			PX_ASSERT(desc != NULL);

			uint32_t index = ~(uint32_t)(uintptr_t)desc->userData;

			if (!(actor->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC))
			{
				// find the entry in the fifo
				if (index < mDynamicActorFIFONum)
				{
					mActorFIFO[index].actor = NULL;
				}
			}
			else
			{
				// find the actor in the dormant list
				if (mDormantActors.free(index))
				{
					mDormantActors.direct(index).actor = NULL;
				}
			}
		}
	}
	mApexScene->unlockRead();

	destructible.setPhysXScene(NULL);

	destructible.removeSelfFromStructure();

	uint32_t referencingActorCount;
	PxRigidDynamic** referencingActors = NULL;
	while((referencingActors = destructible.getReferencingActors(referencingActorCount)) != NULL)
	{
		PxRigidDynamic* actor = referencingActors[referencingActorCount - 1];
		PhysXObjectDescIntl* actorObjDesc = (PhysXObjectDescIntl*)mModule->mSdk->getPhysXObjectInfo(actor);
		if (actorObjDesc != NULL)
		{
			for (uint32_t i = actorObjDesc->mApexActors.size(); i--;)
			{
				if (actorObjDesc->mApexActors[i] == destructible.getAPI())
				{
					actorObjDesc->mApexActors.replaceWithLast(i);
				}
			}
		}
		destructible.unreferencedByActor(actor);
	}

	// Remove from kill list
	for (uint32_t killListIndex = mActorKillList.size(); killListIndex--;)
	{
		bool removeActorFromList = false;
		PxRigidDynamic* killListActor = mActorKillList[killListIndex];
		if (killListActor != NULL)
		{
			PhysXObjectDescIntl* actorObjDesc = (PhysXObjectDescIntl*)mModule->mSdk->getPhysXObjectInfo(killListActor);
			if (actorObjDesc != NULL)
			{
				for (uint32_t i = actorObjDesc->mApexActors.size(); i--;)
				{
					if (actorObjDesc->mApexActors[i] == destructible.getAPI())
					{
						actorObjDesc->mApexActors.replaceWithLast(i);
						destructible.unreferencedByActor(killListActor);
					}
				}
				if (actorObjDesc->mApexActors.size() == 0)
				{
					releasePhysXActor(*killListActor);
					removeActorFromList = true;
				}
			}
			else
			{
				removeActorFromList = true;
			}
		}
		else
		{
			removeActorFromList = true;
		}

		if (removeActorFromList)
		{
			mActorKillList.replaceWithLast(killListIndex);
		}
	}

	PX_ASSERT(mDestructibles.usedCount() == 0 ? mActorKillList.size() == 0 : true);

	// Remove from damage and fracture buffers
	for (RingBuffer<DamageEvent>::It i(getDamageReadBuffer()); i; ++i)
	{
		DamageEvent& e = *i;
		if (e.destructibleID == destructible.getID())
		{
			e.flags |= (uint32_t)DamageEvent::Invalid;
			//e.destructibleID = (uint32_t)DestructibleStructure::InvalidID;
		}
	}

	for (RingBuffer<DamageEvent>::It i(getDamageWriteBuffer()); i; ++i)
	{
		DamageEvent& e = *i;
		if (e.destructibleID == destructible.getID())
		{
			e.flags |= (uint32_t)DamageEvent::Invalid;
			//e.destructibleID = (uint32_t)DestructibleStructure::InvalidID;
		}
	}

	for (RingBuffer<FractureEvent>::It i(mFractureBuffer); i; ++i)
	{
		FractureEvent& e = *i;
		if (e.destructibleID == destructible.getID() && e.chunkIndexInAsset < destructible.getDestructibleAsset()->getChunkCount())
		{
			e.flags |= (uint32_t)FractureEvent::Invalid;
			//e.destructibleID = (uint32_t)DestructibleStructure::InvalidID;
		}
	}

	for (RingBuffer<FractureEvent>::It i(mDeprioritisedFractureBuffer); i; ++i)
	{
		FractureEvent& e = *i;
		if (e.destructibleID == destructible.getID() && e.chunkIndexInAsset < destructible.getDestructibleAsset()->getChunkCount())
		{
			e.flags |= (uint32_t)FractureEvent::Invalid;
			//e.destructibleID = (uint32_t)DestructibleStructure::InvalidID;
		}
	}

	// Remove from scene awake list
	mAwakeActors.free(destructible.getID());

	// Remove from instanced actors list, if it's in one
	if (destructible.getDestructibleAsset()->mParams->chunkInstanceInfo.arraySizes[0] || destructible.getDestructibleAsset()->mParams->scatterMeshAssets.arraySizes[0])
	{
		for (uint32_t i = 0; i < mInstancedActors.size(); ++i)
		{
			if (mInstancedActors[i] == &destructible)
			{
				mInstancedActors.replaceWithLast(i);
				break;
			}
		}
	}

	// Remove from damage report list
	for (uint32_t damageReportIndex = 0; damageReportIndex < mDamageEventReportData.size(); ++damageReportIndex)
	{
		ApexDamageEventReportDataImpl& damageReport = mDamageEventReportData[damageReportIndex];
		if (damageReport.destructible == destructible.getAPI())
		{
			damageReport.destructible = NULL;
			damageReport.clearChunkReports();
		}
	}

	// Remove from chunk state event list
	for (uint32_t actorWithChunkStateEventIndex = mActorsWithChunkStateEvents.size(); actorWithChunkStateEventIndex--;)
	{
		if (mActorsWithChunkStateEvents[actorWithChunkStateEventIndex] == &destructible)
		{
			mActorsWithChunkStateEvents.replaceWithLast(actorWithChunkStateEventIndex);
		}
	}
}

bool DestructibleScene::destroyActorChunks(PxRigidDynamic& actor, uint32_t chunkFlag)
{
	PhysXObjectDescIntl* actorObjDesc = (PhysXObjectDescIntl*) mModule->mSdk->getPhysXObjectInfo(&actor);
	if (!actorObjDesc)
	{
		return false;
	}
	uintptr_t& cindex = (uintptr_t&)actorObjDesc->userData;
	if (cindex == 0)
	{
		return false;
	}

	SCOPED_PHYSX_LOCK_READ(actor.getScene());

	// First collect a list of invariant destructible and chunk IDs
	physx::Array<IntPair> chunks;
	for (uint32_t i = actor.getNbShapes(); i--;)
	{
		PxShape* shape;
		actor.getShapes(&shape, 1, i);
		DestructibleStructure::Chunk* chunk = getChunk(shape);
		PX_ASSERT(chunk != NULL);
		if (chunk != NULL && chunk->isFirstShape(shape))	// BRG OPTIMIZE
		{
			getChunkReportData(*chunk, chunkFlag);
			if (chunk->state & ChunkVisible)
			{
				IntPair chunkIDs;
				chunkIDs.set((int32_t)chunk->destructibleID, (int32_t)chunk->indexInAsset);
				chunks.pushBack(chunkIDs);
			}
		}
	}

	// Now release the list of chunks
	for (uint32_t i = 0; i < chunks.size(); ++i)
	{
		const IntPair& chunkIDs = chunks[i];
		DestructibleActorImpl* dactor = mDestructibles.direct((uint32_t)chunkIDs.i0);
		if (dactor != NULL && dactor->getStructure() != NULL)
		{
			dactor->getStructure()->removeChunk(dactor->getStructure()->chunks[dactor->getFirstChunkIndex()+(uint16_t)chunkIDs.i1]);
			dactor->getStructure()->removePxActorIslandReferences(actor);
			if (dactor->getStructure()->actorForStaticChunks == &actor)
			{
				dactor->getStructure()->actorForStaticChunks = NULL;
			}
		}
	}

	for (uint32_t i = actorObjDesc->mApexActors.size(); i--;)
	{
		const DestructibleActor* dActor = static_cast<const DestructibleActor*>(actorObjDesc->mApexActors[i]);
		if (dActor != NULL)
		{
			((DestructibleActorProxy*)dActor)->impl.unreferencedByActor(&actor);
		}
	}

	if (cindex != 0)	// Destroying all associated chunks should have set this to 0, above
	{
		if (actor.getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC)
		{
			// Dormant actor
			const uint32_t index = (uint32_t)~cindex; 
			if (mDormantActors.free(index))
			{
				mDormantActors.direct(index).actor = NULL;
			}
		}
		else
		{
			mActorFIFO[(uint32_t)~cindex].actor = NULL;
			cindex = 0;
		}
	}

	schedulePxActorForDelete(*actorObjDesc);
	return true;
}

void DestructibleScene::releasePhysXActor(PxRigidDynamic& actor)
{
	PX_PROFILE_ZONE("DestructibleScene::releasePhysXActor", GetInternalApexSDK()->getContextId());

	const HashMap<physx::PxActor*, uint32_t>::Entry* entry = mActorsToAddIndexMap.find(&actor);
	if (entry != NULL)
	{
		uint32_t index = entry->second;
		mActorsToAdd.replaceWithLast(index);
		if (index < mActorsToAdd.size())
		{
			mActorsToAddIndexMap[mActorsToAdd[index]] = index;
		}
		mActorsToAddIndexMap.erase(entry->first);
	}

	// handle the case where an actor was given a force but then immediately released (not sure this is possible)
	mForcesToAddToActorsMap.erase(&actor);

	{
		SCOPED_PHYSX_LOCK_READ(actor.getScene());
		for (uint32_t shapeN = 0; shapeN < actor.getNbShapes(); ++shapeN)
		{
			PxShape* shape;
			actor.getShapes(&shape, 1, shapeN);
			PhysXObjectDescIntl* shapeObjDesc = (PhysXObjectDescIntl*)mModule->mSdk->getPhysXObjectInfo(shape);
			if (shapeObjDesc != NULL)
			{
				DestructibleStructure::Chunk* chunk = (DestructibleStructure::Chunk*)shapeObjDesc->userData;
				if (chunk != NULL)
				{
					// safety net if for any reason the chunk has not been disassociated from the shape
					PX_ALWAYS_ASSERT();
					chunk->clearShapes();
				}

				mModule->mSdk->releaseObjectDesc(shape);
			}
		}
	}

	for ( uint32_t damageReportIndex = 0; damageReportIndex < mDamageEventReportData.size(); ++damageReportIndex )
	{
		ApexDamageEventReportDataImpl& damageReport = mDamageEventReportData[damageReportIndex];

		if ( damageReport.impactDamageActor == &actor )
		{
			damageReport.impactDamageActor = NULL;
		}
	}

	if (getModuleDestructible()->m_destructiblePhysXActorReport != NULL)
	{
		getModuleDestructible()->m_destructiblePhysXActorReport->onPhysXActorRelease(actor);
	}


	// make sure the actor is not referenced by any destructible
	const PhysXObjectDescIntl* desc = static_cast<const PhysXObjectDescIntl*>(GetApexSDK()->getPhysXObjectInfo(&actor));
	if (desc != NULL)
	{
		const uint32_t dActorCount = desc->mApexActors.size();
		for (uint32_t i = 0; i < dActorCount; ++i)
		{
			const DestructibleActor* dActor = static_cast<const DestructibleActor*>(desc->mApexActors[i]);
			if (dActor != NULL)
			{
				if (desc->mApexActors[i]->getOwner()->getObjTypeID() == DestructibleAssetImpl::getAssetTypeID())
				{
					DestructibleActorImpl& destructibleActor = const_cast<DestructibleActorImpl&>(static_cast<const DestructibleActorProxy*>(dActor)->impl);
					destructibleActor.unreferencedByActor(&actor);
				}
			}
		}
	}


	mModule->mSdk->releaseObjectDesc(&actor);
	SCOPED_PHYSX_LOCK_WRITE(actor.getScene());
	actor.release();
}

bool DestructibleScene::scheduleChunkShapesForDelete(DestructibleStructure::Chunk& chunk)
{
	DestructibleActorImpl* destructible = mDestructibles.direct(chunk.destructibleID);

	if ((chunk.state & ChunkVisible) == 0)
	{
//		PX_ASSERT(!"Cannot schedule shape for release from invisible chunk.\n");
		chunk.clearShapes();
		ChunkClearDescendents chunkOp(int32_t(chunk.indexInAsset + destructible->getFirstChunkIndex()));
		forSubtree(chunk, chunkOp, true);
		return true;
	}

	if (chunk.isDestroyed())
	{
		return false;
	}

	PxRigidDynamic* actor = destructible->getStructure()->getChunkActor(chunk);
	SCOPED_PHYSX_LOCK_READ(actor->getScene());

	PhysXObjectDescIntl* actorObjDesc = (PhysXObjectDescIntl*)mModule->mSdk->getPhysXObjectInfo(actor);
	if (!actorObjDesc)
	{
		PX_ASSERT(!"Cannot schedule actor for release that is not owned by APEX.\n");
		return false;
	}
	bool owned = false;
	for (uint32_t i = 0; i < actorObjDesc->mApexActors.size(); ++i)
	{
		if (actorObjDesc->mApexActors[i] && actorObjDesc->mApexActors[i]->getOwner()->getObjTypeID() == DestructibleAssetImpl::mAssetTypeID)
		{
			owned = true;
			break;
		}
	}
	if (!owned && actorObjDesc->mApexActors.size())
	{
		PX_ASSERT(!"Cannot schedule actor for release that is not owned by this module.\n");
		return false;
	}

#if USE_DESTRUCTIBLE_RWLOCK
	DestructibleScopedWriteLock destructibleWriteLock(*destructible);
#endif

	if (getRenderLockMode() == RenderLockMode::PER_ACTOR_RENDER_LOCK)
	{
		destructible->renderDataLock();
	}

#if USE_CHUNK_RWLOCK
	DestructibleStructure::ChunkScopedWriteLock chunkWriteLock(chunk);
#endif

	if (mTotalChunkCount > 0)
	{
		--mTotalChunkCount;
	}

	physx::Array<PxShape*>&	shapes = destructible->getStructure()->getChunkShapes(chunk);
	for (uint32_t i = 0; i < shapes.size(); ++i)
	{
		PxShape* shape = shapes[i];

		const PhysXObjectDescIntl* shapeObjDesc = (const PhysXObjectDescIntl*)mModule->mSdk->getPhysXObjectInfo(shape);
		if (!shapeObjDesc)
		{
			PX_ASSERT(!"Cannot schedule object for release that is not owned by APEX.\n");
			continue;
		}

		mModule->mSdk->releaseObjectDesc(shape);
		SCOPED_PHYSX_LOCK_WRITE(mPhysXScene);
		((physx::PxRigidActor*)actor)->detachShape(*shape);
	}

	if (mTotalChunkCount > 0)
	{
		--mTotalChunkCount;
	}

	if (actor->getNbShapes() == 0)
	{
		PhysXObjectDescIntl* actorObjDesc = (PhysXObjectDescIntl*)mModule->mSdk->getPhysXObjectInfo(actor);
		if (actorObjDesc != NULL)
		{
			for (uint32_t i = actorObjDesc->mApexActors.size(); i--;)
			{
				const DestructibleActor* dActor = static_cast<const DestructibleActor*>(actorObjDesc->mApexActors[i]);
				actorObjDesc->mApexActors.replaceWithLast(i);
				((DestructibleActorProxy*)dActor)->impl.unreferencedByActor(actor);
			}
		}
		destructible->unreferencedByActor(actor);

		if (actorObjDesc != NULL)
		{
			uintptr_t& cindex = (uintptr_t&)actorObjDesc->userData;
			if (actor->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC)
			{
				if (cindex != 0)
				{
					// Dormant actor
					const uint32_t index = (uint32_t)~cindex;
					if (mDormantActors.free(index))
					{
						mDormantActors.direct(index).actor = NULL;
					}
				}
				destructible->getStructure()->removePxActorIslandReferences(*actor);
				if (destructible->getStructure()->actorForStaticChunks == actor)
				{
					destructible->getStructure()->actorForStaticChunks = NULL;
				}
				schedulePxActorForDelete(*actorObjDesc);
			}
			else// if(0 != (uintptr_t&)actorObjDesc->userData)
			{
				PX_ASSERT(cindex != 0);
				PX_ASSERT(mActorFIFO[(uint32_t)~cindex].actor == NULL || mActorFIFO[(uint32_t)~cindex].actor == actor);
				cindex = 0;
			}
		}
	}
	else if (!(actor->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC))
	{
		if (actor->getNbShapes() > 0)
		{
			const uintptr_t cindex = (uintptr_t)actorObjDesc->userData;
			if (cindex != 0)
			{
				PX_ASSERT(mActorFIFO[(uint32_t)~cindex].actor == NULL || mActorFIFO[(uint32_t)~cindex].actor == actor);
				ActorFIFOEntry& FIFOEntry = mActorFIFO[(uint32_t)~cindex];
				FIFOEntry.unscaledMass -= destructible->getChunkMass(chunk.indexInAsset);
				if (FIFOEntry.unscaledMass <= 0.0f)
				{
					FIFOEntry.unscaledMass = 1.0f;	// This should only occur if the last shape is deleted.  In this case, the mass won't matter.
				}
				FIFOEntry.flags |= ActorFIFOEntry::MassUpdateNeeded;
			}
		}
	}
	chunk.clearShapes();
	ChunkClearDescendents chunkOp(int32_t(chunk.indexInAsset + destructible->getFirstChunkIndex()));
	forSubtree(chunk, chunkOp, true);

	chunk.state &= ~(uint32_t)ChunkVisible;

	if (getRenderLockMode() == RenderLockMode::PER_ACTOR_RENDER_LOCK)
	{
		destructible->renderDataUnLock();
	}

	return true;
}

bool DestructibleScene::schedulePxActorForDelete(PhysXObjectDescIntl& actorDesc)
{
	bool inScene = false;
	bool isKinematic = false;
	if (mPhysXScene)
	{
		mPhysXScene->lockRead();
		inScene = ((PxRigidDynamic*)actorDesc.mPhysXObject)->getScene() != NULL;
		isKinematic = ((PxRigidDynamic*)actorDesc.mPhysXObject)->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC;
		mPhysXScene->unlockRead();
	}
	if (inScene && !isKinematic)
	{
		mPhysXScene->lockWrite();
		((PxRigidDynamic*)actorDesc.mPhysXObject)->wakeUp();
		mPhysXScene->unlockWrite();
	}
	PX_ASSERT(mActorKillList.find((PxRigidDynamic*)actorDesc.mPhysXObject) == mActorKillList.end());
	mActorKillList.pushBack((PxRigidDynamic*)actorDesc.mPhysXObject);
	return true;
}

void DestructibleScene::applyRadiusDamage(float damage, float momentum, const PxVec3& position, float radius, bool falloff)
{
	// Apply scene-based damage actor-based damage.  Those will split off islands.
	DamageEvent& damageEvent = getDamageWriteBuffer().pushFront();
	damageEvent.damage = damage;
	damageEvent.momentum = momentum;
	damageEvent.position = position;
	damageEvent.direction = PxVec3(0.0f); // not used
	damageEvent.radius = radius;
	damageEvent.chunkIndexInAsset = ModuleDestructibleConst::INVALID_CHUNK_INDEX;
	damageEvent.flags = DamageEvent::UseRadius;
	if (falloff)
	{
		damageEvent.flags |= DamageEvent::HasFalloff;
	}
}


bool DestructibleScene::isActorCreationRateExceeded()
{
	return mNumActorsCreatedThisFrame >= mModule->m_maxActorsCreateablePerFrame;
}

bool DestructibleScene::isFractureBufferProcessRateExceeded()
{
	return mNumFracturesProcessedThisFrame >= mModule->m_maxFracturesProcessedPerFrame;
}

void DestructibleScene::addToAwakeList(DestructibleActorImpl& actor)
{
	if (mAwakeActors.use(actor.getID()))
	{
		if (getModuleDestructible()->m_chunkReport != NULL)	// Only use the list if there's a report
		{
			mOnWakeActors.pushBack(actor.getAPI());
		}
		return;
	}

	APEX_INTERNAL_ERROR("Destructible actor already present in awake actors list");
}

void DestructibleScene::removeFromAwakeList(DestructibleActorImpl& actor)
{

	if (mAwakeActors.free(actor.getID()))
	{
		if (getModuleDestructible()->m_chunkReport != NULL)	// Only use the list if there's a report
		{
			mOnSleepActors.pushBack(actor.getAPI());
		}
		return;
	}

	APEX_INTERNAL_ERROR("Destructible actor not found in awake actors list, size %d", mAwakeActors.usedCount());
}

bool DestructibleScene::setMassScaling(float massScale, float scaledMassExponent)
{
	if (massScale > 0.0f && scaledMassExponent > 0.0f && scaledMassExponent <= 1.0f)
	{
		mMassScale = massScale;
		mMassScaleInv = 1.0f / mMassScale;
		mScaledMassExponent = scaledMassExponent;
		mScaledMassExponentInv = 1.0f / scaledMassExponent;
		return true;
	}

	return false;
}

void DestructibleScene::invalidateBounds(const PxBounds3* bounds, uint32_t boundsCount)
{
	PX_ASSERT(bounds);
	m_invalidBounds.reserve(m_invalidBounds.size() + boundsCount);
	for (uint32_t i = 0; i < boundsCount; ++i)
	{
		m_invalidBounds.pushBack(bounds[i]);
	}
}

void DestructibleScene::setDamageApplicationRaycastFlags(nvidia::DestructibleActorRaycastFlags::Enum flags)
{
	m_damageApplicationRaycastFlags = flags;
}


void DestructibleScene::processFractureBuffer()
{
	PX_PROFILE_ZONE("DestructibleProcessFractureBuffer", GetInternalApexSDK()->getContextId());

	// We need to use the render lock because the last frame's TMs are accessed in this method and in URR
	bool freeSceneRenderLock = false;
	if (mFractureBuffer.size() > 0 && getRenderLockMode() == RenderLockMode::PER_MODULE_SCENE_RENDER_LOCK)
	{
		lockModuleSceneRenderLock();
		freeSceneRenderLock = true;
	}

	DestructibleActorImpl* lastDestructible = NULL;
	for (;
		(mFractureBuffer.size() > 0) && (!isFractureBufferProcessRateExceeded() && !isActorCreationRateExceeded());
		mFractureBuffer.popFront())
	{
		FractureEvent& fractureEvent = mFractureBuffer.front();
		PX_ASSERT(0 == (FractureEvent::SyncDirect & fractureEvent.flags) && (0 == (FractureEvent::SyncDerived & fractureEvent.flags)) && (0 == (FractureEvent::Manual & fractureEvent.flags)));

		if (fractureEvent.flags & FractureEvent::Invalid)
		{
			continue;
		}

		PX_ASSERT(fractureEvent.destructibleID < mDestructibles.capacity());
		DestructibleActorImpl* destructible = mDestructibles.direct(fractureEvent.destructibleID);
		if (destructible && destructible->getStructure())
		{
			if (destructible->mDamageEventReportIndex == 0xffffffff && mModule->m_chunkReport != NULL)
			{
				destructible->mDamageEventReportIndex = mDamageEventReportData.size();
				ApexDamageEventReportDataImpl& damageEventReport = mDamageEventReportData.insert();
				damageEventReport.setDestructible(destructible);
				damageEventReport.hitDirection = fractureEvent.hitDirection;
				damageEventReport.impactDamageActor = fractureEvent.impactDamageActor;
				damageEventReport.hitPosition = fractureEvent.position;
				damageEventReport.appliedDamageUserData = fractureEvent.appliedDamageUserData; 
			}
			PX_ASSERT(mModule->m_chunkReport == NULL || destructible->mDamageEventReportIndex < mDamageEventReportData.size());

			// Avoid cycling lock/unlock render lock on the same actor
			if (getRenderLockMode() == RenderLockMode::PER_ACTOR_RENDER_LOCK &&
				destructible != lastDestructible)
			{
				if (lastDestructible != NULL)
				{
					lastDestructible->renderDataUnLock();
				}
				destructible->renderDataLock();
			}

			destructible->getStructure()->fractureChunk(fractureEvent);
			++mNumFracturesProcessedThisFrame;
			lastDestructible = destructible;
		}
	}

	if (freeSceneRenderLock &&
		getRenderLockMode() == RenderLockMode::PER_MODULE_SCENE_RENDER_LOCK)
	{
		unlockModuleSceneRenderLock();
	}
	else if(lastDestructible != NULL &&
			getRenderLockMode() == RenderLockMode::PER_ACTOR_RENDER_LOCK)
	{
		lastDestructible->renderDataUnLock();
	}
}


void DestructibleScene::processFractureBuffer(const physx::Array<SyncParams::UserFractureEvent> * userSource)
{
	// if we still have pending local fractures, we push any new incoming fractures into the deprioritised fracture buffer
	if(mFractureBuffer.size() > 0)
	{
		if(NULL != userSource)
		{
			for(physx::Array<SyncParams::UserFractureEvent>::ConstIterator iter = userSource->begin(); iter != userSource->end(); ++iter)
			{
				FractureEvent & fractureEvent = mSyncParams.interpret(*iter);
				PX_ASSERT(mSyncParams.assertUserFractureEventOk(fractureEvent, *this));
				PX_ASSERT(0 == (FractureEvent::SyncDirect & fractureEvent.flags));
				fractureEvent.flags |= FractureEvent::SyncDirect;
				mDeprioritisedFractureBuffer.pushBack() = fractureEvent;
			}
		}
	}
	// process deprioritised fractures only if local fractures have been processed, and any new incoming fractures only after clearing the backlog
	else
	{
		// process fractures from the sync buffer
#define DEPRIORITISED_CONDITION (mDeprioritisedFractureBuffer.size() > 0)
		PX_ASSERT(0 == mFractureBuffer.size());
		bool processingDeprioritised = true;
		const uint32_t userSourceCount = (NULL != userSource) ? userSource->size() : 0;
		uint32_t userSourceIndex = 0;
		for (;
			(DEPRIORITISED_CONDITION || (userSourceIndex < userSourceCount)) && (!isFractureBufferProcessRateExceeded() && !isActorCreationRateExceeded());
			processingDeprioritised ? mDeprioritisedFractureBuffer.popFront() : unfortunateCompilerWorkaround(++userSourceIndex))
		{
			processingDeprioritised = DEPRIORITISED_CONDITION;
#undef DEPRIORITISED_CONDITION
			FractureEvent & fractureEvent = processingDeprioritised ? mDeprioritisedFractureBuffer.front() : mSyncParams.interpret((*userSource)[userSourceIndex]);
			PX_ASSERT(!processingDeprioritised ? mSyncParams.assertUserFractureEventOk(fractureEvent, *this) :true);
			if(!processingDeprioritised)
			{
				PX_ASSERT(0 == (FractureEvent::SyncDirect & fractureEvent.flags));
				fractureEvent.flags |= FractureEvent::SyncDirect;
			}

			const bool usingEditFeature = false;
			if(usingEditFeature)
			{
				if(!processingDeprioritised)
				{
					const DestructibleActorImpl::SyncParams & actorParams = mDestructibles.direct(fractureEvent.destructibleID)->getSyncParams();
					mSyncParams.interceptEdit(fractureEvent, actorParams);
				}
			}

			if(0 != (FractureEvent::Invalid & fractureEvent.flags))
			{
				continue;
			}

			PX_ASSERT(fractureEvent.destructibleID < mDestructibles.capacity());
			DestructibleActorImpl * destructible = mDestructibles.direct(fractureEvent.destructibleID);
			if(NULL != destructible && NULL != destructible->getStructure())
			{
				// set damage event report data and fracture event chunk report data
				if(0xFFFFFFFF == destructible->mDamageEventReportIndex && NULL != mModule->m_chunkReport)
				{
					destructible->mDamageEventReportIndex = mDamageEventReportData.size();
					ApexDamageEventReportDataImpl & damageEventReport = mDamageEventReportData.insert();
					damageEventReport.setDestructible(destructible);
					damageEventReport.hitDirection = fractureEvent.hitDirection;
					damageEventReport.appliedDamageUserData = fractureEvent.appliedDamageUserData;
				}

				// perform the fracturing
				destructible->getStructure()->fractureChunk(fractureEvent);
				++mNumFracturesProcessedThisFrame;
			}
		}

		// if we still have any incoming fractures unprocessed, we push them into the deprioritised fracture buffer
		if(NULL != userSource ? userSourceIndex < userSourceCount : false)
		{
			for(; userSourceIndex < userSourceCount; ++userSourceIndex)
			{
				FractureEvent & fractureEvent = mSyncParams.interpret((*userSource)[userSourceIndex]);
				PX_ASSERT(mSyncParams.assertUserFractureEventOk(fractureEvent, *this));
				PX_ASSERT(0 == (FractureEvent::SyncDirect & fractureEvent.flags));
				fractureEvent.flags |= FractureEvent::SyncDirect;
				mDeprioritisedFractureBuffer.pushBack() = fractureEvent;
			}
		}
	}

	// Move fracture event from Deferred fracture buffer to Deprioritised fracture buffer
	if(mDeferredFractureBuffer.size() > 0)
	{
		mDeprioritisedFractureBuffer.reserve(mDeprioritisedFractureBuffer.size() + mDeferredFractureBuffer.size());
		do
		{
			mDeprioritisedFractureBuffer.pushBack() = mDeferredFractureBuffer.front();
			mDeferredFractureBuffer.popFront();
		}
		while(mDeferredFractureBuffer.size() > 0);
	}
}

void DestructibleScene::processDamageColoringBuffer()
{
	for (; (mSyncDamageEventCoreDataBuffer.size() > 0) && (!isFractureBufferProcessRateExceeded() && !isActorCreationRateExceeded()); mSyncDamageEventCoreDataBuffer.popFront())
	{
		SyncDamageEventCoreDataParams& syncDamageEventCoreDataParams = mSyncDamageEventCoreDataBuffer.front();

		PX_ASSERT(syncDamageEventCoreDataParams.destructibleID < mDestructibles.capacity());
		DestructibleActorImpl * destructible = mDestructibles.direct(syncDamageEventCoreDataParams.destructibleID);
		if (NULL != destructible && destructible->useDamageColoring())
		{
			destructible->applyDamageColoring_immediate(	syncDamageEventCoreDataParams.chunkIndexInAsset, 
														syncDamageEventCoreDataParams.position, 
														syncDamageEventCoreDataParams.damage, 
														syncDamageEventCoreDataParams.radius );
		}
	}
}


void DestructibleScene::calculateDamageBufferCostProfiles(nvidia::RingBuffer<DamageEvent> & subjectDamageBuffer)
{
	physx::Array<uint8_t*> trail;
	physx::Array<uint8_t> undo;

	for (RingBuffer<DamageEvent>::It i(subjectDamageBuffer); i; ++i)
	{
		DamageEvent& e = *i;

		if (e.flags & DamageEvent::Invalid)
		{
			continue;
		}

		// Calculate costs and benefits for this event
		float realCost = 0;
		float realBenefit = 0;
		for (uint32_t depth = 0; depth <= e.getMaxDepth(); ++depth)
		{
			float depthCost = realCost;	// Cost & benefit at depth include only real events at lower depths
			float depthBenefit = realBenefit;
			physx::Array<FractureEvent>& fractures = e.fractures[depth];
			for (uint32_t j = 0; j < fractures.size(); ++j)
			{
				FractureEvent& fractureEvent = fractures[j];
				float eventCost = 0;
				float eventBenefit = 0;
				uint32_t oldTrailSize = trail.size();
				calculatePotentialCostAndBenefit(eventCost, eventBenefit, trail, undo, fractureEvent);
				depthCost += eventCost;	// Cost & benefit at depth include virtual events at this depth
				depthBenefit += eventBenefit;
				if ((fractureEvent.flags & FractureEvent::Virtual) == 0)
				{
					// Add real cost
					realCost += eventCost;
					realBenefit += eventBenefit;
				}
				else
				{
					// Undo chunk temporary state changes from virtual event
					uint8_t** trailMark = trail.begin() + oldTrailSize;
					uint8_t** trailEnd = trail.end();
					uint8_t* undoEnd = undo.end();
					while (trailEnd-- > trailMark)
					{
						PX_ASSERT(undoEnd > undo.begin());
						*(*trailEnd) = (uint8_t)(((**trailEnd)&~(uint8_t)ChunkTempMask) | (*--undoEnd));
					}
					trail.resize((uint32_t)(trailMark - trail.begin()));
				}
				undo.reset();
			}
			e.cost[depth] = depthCost;
			e.benefit[depth] = depthBenefit;
		}

		e.processDepth = e.getMaxDepth();

		// Cover trail
		for (uint8_t** statePtr = trail.begin(); statePtr < trail.end(); ++statePtr)
		{
			*(*statePtr) &= ~(uint8_t)ChunkTempMask;
		}
		trail.reset();
	}
}


void DestructibleScene::processEventBuffers()
{
	//===SyncParams=== prevent user from changing sync params during this phase. do this just before the first encounter of working with sync params in the program
	//PX_ASSERT(!mSyncParams.lockSyncParams);
	mSyncParams.lockSyncParams = true;

	//===SyncParams=== prepare user's damage event buffer, if available
	UserDamageEventHandler * callback = NULL;
	callback = mModule->getSyncParams().getUserDamageEventHandler();
	const physx::Array<SyncParams::UserDamageEvent> * userSource = NULL;
	if(NULL != callback)
	{
		mSyncParams.onPreProcessReadData(*callback, userSource);
	}

	//process damage buffer's damageEvents
	nvidia::RingBuffer<DamageEvent> userDamageBuffer;
	const nvidia::RingBuffer<DamageEvent> * const prePopulateAddress = &userDamageBuffer;
	PX_ASSERT(0 == userDamageBuffer.size());
	generateFractureProfilesInDamageBuffer(userDamageBuffer, userSource);
	PX_ASSERT((NULL != userSource) ? (0 != userDamageBuffer.size()) : (0 == userDamageBuffer.size()));
	userSource = NULL;
	const nvidia::RingBuffer<DamageEvent> * const postPopulateAddress = &userDamageBuffer;
	PX_ASSERT(prePopulateAddress == postPopulateAddress);
	PX_UNUSED(prePopulateAddress);
	PX_UNUSED(postPopulateAddress);

	calculateDamageBufferCostProfiles(getDamageReadBuffer());

	//===SyncParams===
	if(0 != userDamageBuffer.size())
	{
		calculateDamageBufferCostProfiles(userDamageBuffer);
	}

	//===SyncParams=== give the user the damage event buffer. damage event buffer must be fully populated and locked during this call
	if(NULL != callback)
	{
		mSyncParams.onProcessWriteData(*callback, getDamageReadBuffer());
	}

	//pop damage buffer, push fracture buffer. note that local fracture buffer may be 'contaminated' with non-local fractureEvents, which are derivatives of the user's damageEvents
	fillFractureBufferFromDamage((0 != userDamageBuffer.size()) ? &userDamageBuffer : NULL);
	userDamageBuffer.clear();

	//===SyncParams=== done with user's damage event buffer, if available
	if(NULL != callback)
	{
		mSyncParams.onPostProcessReadData(*callback);
	}
	callback = NULL;
}

void DestructibleScene::generateFractureProfilesInDamageBuffer(nvidia::RingBuffer<DamageEvent> & userDamageBuffer, const physx::Array<SyncParams::UserDamageEvent> * userSource /*= NULL*/)
{
	
#define LOCAL_CONDITION (eventN < getDamageReadBuffer().size())
	bool processingLocal = true;
	for (uint32_t eventN = 0 , userEventN = 0, userEventCount = (NULL != userSource) ? userSource->size() : 0;
		 LOCAL_CONDITION || (userEventN < userEventCount);
		 processingLocal ? ++eventN : ++userEventN)
	{
		processingLocal = LOCAL_CONDITION;
#undef LOCAL_CONDITION
		//===SyncParams===
		DamageEvent& damageEvent = processingLocal ? getDamageReadBuffer()[eventN] : mSyncParams.interpret((*userSource)[userEventN]);
		PX_ASSERT(!processingLocal ? mSyncParams.assertUserDamageEventOk(damageEvent, *this) : true);
		const bool usingEditFeature = false;
		if(usingEditFeature)
		{
			if(!processingLocal)
			{
				const DestructibleActorImpl::SyncParams & actorParams = mDestructibles.direct(damageEvent.destructibleID)->getSyncParams();
				mSyncParams.interceptEdit(damageEvent, actorParams);
			}
		}

		for (uint32_t i = DamageEvent::MaxDepth + 1; i--;)
		{
			damageEvent.fractures[i].reset();
		}

		if (damageEvent.flags & DamageEvent::Invalid)
		{
			continue;
		}

#if APEX_RUNTIME_FRACTURE
		if ((damageEvent.flags & DamageEvent::IsFromImpact) == 0)
		{
			// TODO: Remove this, and properly process the damage events
			for (uint32_t i = 0; i < mDestructibles.usedCount(); ++i)
			{
				nvidia::fracture::Actor* rtActor = mDestructibles.getUsed(i)->getRTActor();
				if (rtActor != NULL && rtActor->patternFracture(damageEvent))
				{
					break;
				}
			}
		}
#endif

		if (damageEvent.destructibleID == (uint32_t)DestructibleActorImpl::InvalidID)
		{
			// Scene-based damage.  Must find destructibles.
			PxSphereGeometry sphere(damageEvent.radius);

			PxOverlapBuffer ovBuffer(&mOverlapHits[0], MAX_SHAPE_COUNT);
			mPhysXScene->lockRead();
			mPhysXScene->overlap(sphere, PxTransform(damageEvent.position), ovBuffer);
			uint32_t	nbHits	= ovBuffer.getNbAnyHits();
			//nbHits	= nbHits >= 0 ? nbHits : MAX_SHAPE_COUNT; //Ivan: it is always true and should be removed

			if (nbHits)
			{
				qsort(&mOverlapHits[0], nbHits, sizeof(PxOverlapHit), compareOverlapHitShapePointers);
				DestructibleActorImpl* lastDestructible = NULL;
				for (uint32_t i = 0; i < nbHits; ++i)
				{
					DestructibleActorImpl* overlapDestructible = NULL;
					DestructibleStructure::Chunk*	chunk	= getChunk((const PxShape*)mOverlapHits[i].shape);
					if (chunk != NULL)
					{
						overlapDestructible	= mDestructibles.direct(chunk->destructibleID);
					}

					if (overlapDestructible != lastDestructible)
					{
						if (overlapDestructible->getStructure() != NULL)
						{
							// Expand damage buffer
							DamageEvent& newEvent = processingLocal ? getDamageReadBuffer().pushBack() : userDamageBuffer.pushBack();
							newEvent = processingLocal ? getDamageReadBuffer()[eventN] : userDamageBuffer[userEventN];	// Need to use indexed access again; damageEvent may now be invalid if the buffer resized
							newEvent.destructibleID = overlapDestructible->getID();
							const uint32_t maxLOD = overlapDestructible->getDestructibleAsset()->getDepthCount() > 0 ? overlapDestructible->getDestructibleAsset()->getDepthCount() - 1 : 0;
							newEvent.minDepth = PxMin(overlapDestructible->getLOD(), maxLOD);
							newEvent.maxDepth = damageEvent.minDepth;
						}
						lastDestructible = overlapDestructible;
					}
				}
			}

			mPhysXScene->unlockRead();
		}
		else
		{
			// Actor-based damage
			DestructibleActorImpl* destructible = mDestructibles.direct(damageEvent.destructibleID);
			if (destructible == NULL)
			{
				continue;
			}

			if (getRenderLockMode() == RenderLockMode::PER_ACTOR_RENDER_LOCK)
			{
				destructible->renderDataLock();
			}

			if(!processingLocal)
			{
				PX_ASSERT(0 == (DamageEvent::SyncDirect & damageEvent.flags));
				damageEvent.flags |= DamageEvent::SyncDirect;
			}
			const uint32_t maxLOD = destructible->getDestructibleAsset()->getDepthCount() > 0 ? destructible->getDestructibleAsset()->getDepthCount() - 1 : 0;
			damageEvent.minDepth = PxMin(destructible->getLOD(), maxLOD);
			damageEvent.maxDepth = damageEvent.minDepth;
			if ((damageEvent.flags & DamageEvent::UseRadius) == 0)
			{
                destructible->applyDamage_immediate(damageEvent);
			}
			else
			{
				destructible->applyRadiusDamage_immediate(damageEvent);
			}
			if (getRenderLockMode() == RenderLockMode::PER_ACTOR_RENDER_LOCK)
			{
				destructible->renderDataUnLock();
			}
		}

		//===SyncParams===
		if(!processingLocal)
		{
			PX_ASSERT(0 != (DamageEvent::SyncDirect & damageEvent.flags));
			userDamageBuffer.pushBack() = damageEvent;
		}
	}
}

void DestructibleScene::fillFractureBufferFromDamage(nvidia::RingBuffer<DamageEvent> * userDamageBuffer /*= NULL*/)
{
#define LOCAL_CONDITION (getDamageReadBuffer().size() > 0)
	bool processingLocal = true;
	for (uint32_t userEventN = 0, userEventCount = (NULL != userDamageBuffer) ? userDamageBuffer->size() : 0;
		 LOCAL_CONDITION || (userEventN < userEventCount);
		 processingLocal ? getDamageReadBuffer().popFront() : unfortunateCompilerWorkaround(++userEventN))
	{
        processingLocal = LOCAL_CONDITION;
#undef LOCAL_CONDITION
		//===SyncParams===
		DamageEvent& damageEvent = processingLocal ? getDamageReadBuffer().front() : (*userDamageBuffer)[userEventN];

		if (damageEvent.flags & DamageEvent::Invalid)
		{
			continue;
		}
		for (uint32_t depth = 0; depth <= damageEvent.getProcessDepth(); ++depth)
		{
			const bool atProcessDepth = depth == damageEvent.getProcessDepth();
			physx::Array<FractureEvent>& buffer = damageEvent.fractures[depth];
			for (uint32_t i = 0; i < buffer.size(); ++i)
			{
				FractureEvent& fractureEvent = buffer[i];
				if (atProcessDepth || (fractureEvent.flags & FractureEvent::Virtual) == 0)
				{
					fractureEvent.hitDirection = damageEvent.direction;
					fractureEvent.impactDamageActor = damageEvent.impactDamageActor;
					fractureEvent.position = damageEvent.position;
					fractureEvent.appliedDamageUserData = damageEvent.appliedDamageUserData;

					//===SyncParams===
					PX_ASSERT(0 == (FractureEvent::DeleteChunk & fractureEvent.flags));
					if(mDestructibles.direct(damageEvent.destructibleID)->mInDeleteChunkMode)
					{
						fractureEvent.flags |= FractureEvent::DeleteChunk;
					}

					if(!processingLocal)
					{
						PX_ASSERT(0 != (FractureEvent::SyncDerived & fractureEvent.flags));
						mDeprioritisedFractureBuffer.pushBack() = fractureEvent;
					}
					else
					{
						PX_ASSERT(0 == (FractureEvent::SyncDirect & fractureEvent.flags) && (0 == (FractureEvent::SyncDerived & fractureEvent.flags)) && (0 == (FractureEvent::Manual & fractureEvent.flags)));
						mFractureBuffer.pushBack() = fractureEvent;
					}
				}
			}
		}
    }
}

static StatsInfo DestructionStatsData[] =
{
	{"VisibleDestructibleChunkCount",		StatDataType::INT, {{0}} },
	{"DynamicDestructibleChunkIslandCount",	StatDataType::INT, {{0}} },
	{"NumberOfShapes",						StatDataType::INT, {{0}} },
	{"NumberOfAwakeShapes",					StatDataType::INT, {{0}} },
	{"RbThroughput(Mpair/sec)",				StatDataType::FLOAT, {{0}} },
};

void DestructibleScene::createModuleStats(void)
{
	mModuleSceneStats.numApexStats		= NumberOfStats;
	mModuleSceneStats.ApexStatsInfoPtr	= (StatsInfo*)PX_ALLOC(sizeof(StatsInfo) * NumberOfStats, PX_DEBUG_EXP("StatsInfo"));

	for (uint32_t i = 0; i < NumberOfStats; i++)
	{
		mModuleSceneStats.ApexStatsInfoPtr[i] = DestructionStatsData[i];
	}
}

void DestructibleScene::destroyModuleStats(void)
{
	mModuleSceneStats.numApexStats = 0;
	if (mModuleSceneStats.ApexStatsInfoPtr)
	{
		PX_FREE_AND_RESET(mModuleSceneStats.ApexStatsInfoPtr);
	}
}

void DestructibleScene::setStatValue(int32_t index, StatValue dataVal)
{
	if (mModuleSceneStats.ApexStatsInfoPtr)
	{
		mModuleSceneStats.ApexStatsInfoPtr[index].StatCurrentValue = dataVal;
	}
}

SceneStats* DestructibleScene::getStats()
{
	PX_PROFILE_ZONE("DestructibleScene::getStats", GetInternalApexSDK()->getContextId());
	{	// CPU/GPU agnositic stats
		unsigned totalVisibleChunkCount = 0;
		unsigned totalDynamicIslandCount = 0;
		for (unsigned i = 0; i < mStructures.usedCount(); ++i)
		{
			DestructibleStructure* s = mStructures.getUsed(i);
			for (unsigned j = 0; j < s->destructibles.size(); ++j)
			{
				DestructibleActorImpl* a = s->destructibles[j];
				const unsigned visibleChunkCount = a->getNumVisibleChunks();
				totalVisibleChunkCount += visibleChunkCount;
				physx::PxRigidDynamic** buffer;
				unsigned bufferSize;
				if (a->acquirePhysXActorBuffer(buffer, bufferSize, DestructiblePhysXActorQueryFlags::Dynamic))
				{
					totalDynamicIslandCount += bufferSize;
					a->releasePhysXActorBuffer();
				}
			}
		}

#if 0	// Issue warnings if these don't align with updated values	
		if (mTotalChunkCount != totalVisibleChunkCount)
		{
			APEX_DEBUG_WARNING("mTotalChunkCount = %d, actual total visible chunk count = %d.\n", mTotalChunkCount, totalVisibleChunkCount);
		}
		if (mDynamicActorFIFONum != totalDynamicIslandCount)
		{
			APEX_DEBUG_WARNING("mDynamicActorFIFONum = %d, actual total dynamic chunk island count = %d.\n", mDynamicActorFIFONum, totalDynamicIslandCount);
		}
		if (mModule->m_chunkFIFOMax > 0 && mTotalChunkCount > mModule->m_chunkFIFOMax)
		{
			APEX_DEBUG_WARNING("mTotalChunkCount = %d, mModule->m_chunkFIFOMax = %d.\n", mTotalChunkCount, mModule->m_chunkFIFOMax);
		}
		if (mModule->m_dynamicActorFIFOMax > 0 && mDynamicActorFIFONum > mModule->m_dynamicActorFIFOMax)
		{
			APEX_DEBUG_WARNING("mDynamicActorFIFONum = %d, mModule->m_dynamicActorFIFOMax = %d.\n", mDynamicActorFIFONum, mModule->m_dynamicActorFIFOMax);
		}
#endif

		StatValue dataVal;

		dataVal.Int = (int)totalVisibleChunkCount - mPreviousVisibleDestructibleChunkCount;
		mPreviousVisibleDestructibleChunkCount = (int)totalVisibleChunkCount;
		setStatValue(VisibleDestructibleChunkCount, dataVal);

		dataVal.Int = (int)totalDynamicIslandCount - mPreviousDynamicDestructibleChunkIslandCount;
		mPreviousDynamicDestructibleChunkIslandCount = (int)totalDynamicIslandCount;
		setStatValue(DynamicDestructibleChunkIslandCount, dataVal);
	}

	{
		StatValue dataVal;
		dataVal.Int = 0;
		setStatValue(NumberOfShapes, dataVal);
		setStatValue(NumberOfAwakeShapes, dataVal);
		setStatValue(RbThroughput, dataVal);
	}

	return &mModuleSceneStats;
}

///////////////////////////////////////////////////////////////////////////

bool DestructibleScene::ChunkLoadState::execute(DestructibleStructure* structure, DestructibleStructure::Chunk& chunk)
{
	DestructibleScene& scene        = *structure->dscene;
	DestructibleActorImpl& destructible = *scene.mDestructibles.direct(chunk.destructibleID);

	const uint16_t chunkIndex  = chunk.indexInAsset;
	const uint16_t parentIndex = destructible.getDestructibleAsset()->getChunkParentIndex(chunkIndex);
	const bool chunkDynamic        = destructible.getInitialChunkDynamic(chunkIndex);
	const bool chunkVisible        = destructible.getInitialChunkVisible(chunkIndex);
	const bool chunkDestroyed      = destructible.getInitialChunkDestroyed(chunkIndex);
	bool chunkRecurse              = true;

	// Destroy the chunk if necessary
	if (chunkDestroyed && (!chunk.isDestroyed() || chunkVisible))
	{
		structure->removeChunk(chunk);
		structure->setSupportInvalid(true);
		chunkRecurse = true;
	}
	// Create the chunk actor or append the chunk shapes to the island's actor
	else if (chunkVisible && chunk.getShapeCount() == 0)
	{
		PxRigidDynamic* chunkActor = NULL;
		bool     chunkActorCreated = false;

		// Visible chunks not in an island require their own PxActor
		if (chunk.islandID == (uint32_t)DestructibleStructure::InvalidID)
		{
			chunk.state &= ~(uint8_t)ChunkVisible;
			chunkActor = scene.createRoot(chunk, destructible.getInitialChunkGlobalPose(chunkIndex), chunkDynamic, NULL, false);
			chunkActorCreated = true;
		}
		else
		{
			// Check if the island actor has already been created
			PxRigidDynamic* actor = NULL;
			if (!chunkDynamic && structure->actorForStaticChunks != NULL)
			{
				actor = structure->actorForStaticChunks;
				chunk.islandID = structure->actorToIsland[actor];
			}
			else
			{
				actor = structure->islandToActor[chunk.islandID];
			}
			if (NULL == actor)
			{
				// Create the island's actor
				chunk.state &= ~(uint8_t)ChunkVisible;
				actor = scene.createRoot(chunk, destructible.getInitialChunkGlobalPose(chunkIndex), chunkDynamic, NULL, false);
				structure->islandToActor[chunk.islandID] = actor;
				scene.setStructureUpdate(structure, true);	// islandToActor needs to be cleared in DestructibleStructure::tick
				if (!chunkDynamic)
				{
					structure->actorForStaticChunks = actor;
				}
				chunkActorCreated = true;
			}
			else
			{
				// Append the chunk shapes to the existing island actor
				SCOPED_PHYSX_LOCK_READ(actor->getScene());
				PxTransform relTM = actor->getGlobalPose().transformInv(destructible.getInitialChunkGlobalPose(chunkIndex));

				chunk.state &= ~(uint8_t)ChunkVisible;
				if (scene.appendShapes(chunk, chunkDynamic, &relTM, actor))
				{
					PhysXObjectDescIntl* objDesc = scene.mModule->mSdk->getGenericPhysXObjectInfo(actor);
					if(objDesc->mApexActors.find(destructible.getAPI()) == objDesc->mApexActors.end())
					{
						objDesc->mApexActors.pushBack(destructible.getAPI());
						destructible.referencedByActor(actor);
					}
				}
			}
			chunkActor = actor;
		}

		PX_ASSERT(!chunk.isDestroyed() && NULL != chunkActor);
		if (!chunk.isDestroyed() && NULL != chunkActor)
		{
			// BRG - must decouple this now since we have "dormant" kinematic chunks
			// which are chunks that have been freed but turned kinematic for "hard" sleeping
//			PX_ASSERT(chunkDynamic != chunkActor->readBodyFlag(BF_KINEMATIC));

			SCOPED_PHYSX_LOCK_WRITE(scene.mApexScene);
			SCOPED_PHYSX_LOCK_READ(scene.mApexScene);

			destructible.setChunkVisibility(chunkIndex, true);

			// Only set shape local poses on newly created actors (appendShapes properly positions otherwise)
			if (chunkActorCreated)
			{
				for (uint32_t i = 0; i < chunk.getShapeCount(); ++i)
				{
					chunk.getShape(i)->setLocalPose(destructible.getInitialChunkLocalPose(chunkIndex));
				}
			}

			// Initialize velocities for newly created dynamic actors
			if (chunkActorCreated && chunkDynamic)
			{
				chunkActor->setLinearVelocity( destructible.getInitialChunkLinearVelocity( chunkIndex));
				chunkActor->setAngularVelocity(destructible.getInitialChunkAngularVelocity(chunkIndex));
			}

			// Don't recurse visible chunks
			chunkRecurse = false;
			if (parentIndex != DestructibleAssetImpl::InvalidChunkIndex)
			{
				structure->chunks[parentIndex].flags |= ChunkMissingChild;
			}
		}
		structure->setSupportInvalid(true);
	}

	return chunkRecurse;
}


////////////////////////////////////////////////////////////////
// PhysX3 dependent methods
////////////////////////////////////////////////////////////////

PxRigidDynamic* DestructibleScene::createRoot(DestructibleStructure::Chunk& chunk, const PxTransform& pose, bool dynamic, PxTransform* relTM, bool fromInitialData)
{
	// apan2 need verify 3.0 (createRoot)
	PX_PROFILE_ZONE("DestructibleCreateRoot", GetInternalApexSDK()->getContextId());

	DestructibleActorImpl* destructible = mDestructibles.direct(chunk.destructibleID);

	PxRigidDynamic* actor = NULL;
	if (!chunk.isDestroyed())
	{
		actor = destructible->getStructure()->getChunkActor(chunk);
	}

	if ((chunk.state & ChunkVisible) != 0)
	{
		SCOPED_PHYSX_LOCK_READ(actor->getScene());
		PhysXObjectDesc* actorObjDesc = (PhysXObjectDesc*)mModule->mSdk->getPhysXObjectInfo(actor);
		PX_ASSERT(actor && actorObjDesc && actor->getNbShapes() >= 1);
		if (destructible->getStructure()->chunkIsSolitary(chunk))
		{
			bool actorIsDynamic = (chunk.state & ChunkDynamic) != 0;
			if (dynamic == actorIsDynamic)
			{
				return actor;	// nothing to do
			}
			if (!dynamic)
			{
				SCOPED_PHYSX_LOCK_WRITE(actor->getScene());
				actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
				actorObjDesc->userData = NULL;
				return actor;
			}
#if 0	// XXX \todo: why doesn't this work?
			else
			{
				actor->clearBodyFlag(BF_KINEMATIC);
				addActor(*actorDesc, *actor);
				return;
			}
#endif
		}
		scheduleChunkShapesForDelete(chunk);
	}

	const uint32_t firstHullIndex = destructible->getDestructibleAsset()->getChunkHullIndexStart(chunk.indexInAsset);
	const uint32_t hullCount = destructible->getDestructibleAsset()->getChunkHullIndexStop(chunk.indexInAsset) - firstHullIndex;
	PxConvexMeshGeometry* convexShapeDescs = (PxConvexMeshGeometry*)PxAlloca(sizeof(PxConvexMeshGeometry) * hullCount);

	DestructibleAssetParametersNS::Chunk_Type* source = destructible->getDestructibleAsset()->mParams->chunks.buf + chunk.indexInAsset;

	// Whether or not this chunk can be damaged by an impact
	const bool takesImpactDamage = destructible->takesImpactDamageAtDepth(source->depth);

	// Get user-defined descriptors, modify as needed

	// Create a new actor
	PhysX3DescTemplateImpl physX3Template;
	destructible->getPhysX3Template(physX3Template);

	const DestructibleActorParamNS::BehaviorGroup_Type& behaviorGroup = destructible->getBehaviorGroup(chunk.indexInAsset);

	PxRigidDynamic* newActor = NULL;
	PxTransform	poseActor(pose);
	if (!fromInitialData)
		poseActor.p -= poseActor.rotate(destructible->getDestructibleAsset()->getChunkPositionOffset(chunk.indexInAsset));

	poseActor.q.normalize();

	newActor = mPhysXScene->getPhysics().createRigidDynamic(poseActor);

	PX_ASSERT(newActor && "creating actor failed");
	if (!newActor)
		return NULL;

	physX3Template.apply(newActor);

	if (dynamic && destructible->getDestructibleParameters().dynamicChunksDominanceGroup < 32)
	{
		newActor->setDominanceGroup(destructible->getDestructibleParameters().dynamicChunksDominanceGroup);
	}

	newActor->setActorFlag(PxActorFlag::eSEND_SLEEP_NOTIFIES, true);
	newActor->setMaxDepenetrationVelocity(behaviorGroup.maxDepenetrationVelocity);

	// Shape(s):
	for (uint32_t hullIndex = 0; hullIndex < hullCount; ++hullIndex)
	{
		PxConvexMeshGeometry& convexShapeDesc = convexShapeDescs[hullIndex];
		PX_PLACEMENT_NEW(&convexShapeDesc, PxConvexMeshGeometry);
		convexShapeDesc.convexMesh = destructible->getConvexMesh(hullIndex + firstHullIndex);
		PX_ASSERT(convexShapeDesc.convexMesh != NULL);
		convexShapeDesc.scale.scale = destructible->getScale();
		// Divide out the cooking scale to get the proper scaling
		const PxVec3 cookingScale = getModuleDestructible()->getChunkCollisionHullCookingScale();
		convexShapeDesc.scale.scale.x /= cookingScale.x;
		convexShapeDesc.scale.scale.y /= cookingScale.y;
		convexShapeDesc.scale.scale.z /= cookingScale.z;
		if ((convexShapeDesc.scale.scale - PxVec3(1.0f)).abs().maxElement() < 10 * PX_EPS_F32)
		{
			convexShapeDesc.scale.scale = PxVec3(1.0f);
		}

		PxShape*	shape;

		PxTransform	localPose;
		if (relTM != NULL)
		{
			localPose = *relTM;
			localPose.p += destructible->getScale().multiply(destructible->getDestructibleAsset()->getChunkPositionOffset(chunk.indexInAsset));
		}
		else
		{
			localPose = PxTransform(destructible->getDestructibleAsset()->getChunkPositionOffset(chunk.indexInAsset));
		}

		shape = newActor->createShape(convexShapeDesc,
			physX3Template.materials.begin(),
			static_cast<uint16_t>(physX3Template.materials.size()),
			static_cast<physx::PxShapeFlags>(physX3Template.shapeFlags));
		PX_ASSERT(shape);
		if (!shape)
		{
			APEX_INTERNAL_ERROR("Failed to create the PhysX shape.");
			return NULL;
		}
		shape->setLocalPose(localPose);
		physX3Template.apply(shape);

		if (behaviorGroup.groupsMask.useGroupsMask)
		{
			physx::PxFilterData filterData(behaviorGroup.groupsMask.bits0,
				behaviorGroup.groupsMask.bits1,
				behaviorGroup.groupsMask.bits2,
				behaviorGroup.groupsMask.bits3);
			shape->setSimulationFilterData(filterData);
			shape->setQueryFilterData(filterData);
		}
		else if (dynamic && destructible->getDestructibleParameters().useDynamicChunksGroupsMask)
		{
			// Override the filter data
			shape->setSimulationFilterData(destructible->getDestructibleParameters().dynamicChunksFilterData);
			shape->setQueryFilterData(destructible->getDestructibleParameters().dynamicChunksFilterData);
		}

		// apan2 
		physx::PxPairFlags	pairFlag = (physx::PxPairFlags)physX3Template.contactReportFlags;
		if (takesImpactDamage)
		{
			pairFlag	/* |= PxPairFlag::eNOTIFY_CONTACT_FORCES */
				|= PxPairFlag::eNOTIFY_THRESHOLD_FORCE_PERSISTS
				| PxPairFlag::eNOTIFY_THRESHOLD_FORCE_FOUND
				| PxPairFlag::eNOTIFY_CONTACT_POINTS;

		}

		if (behaviorGroup.materialStrength > 0.0f)
		{
			pairFlag |= PxPairFlag::eMODIFY_CONTACTS;
		}

		if (mApexScene->getApexPhysX3Interface())
			mApexScene->getApexPhysX3Interface()->setContactReportFlags(shape, pairFlag, destructible->getAPI(), chunk.indexInAsset);

	}

	// Calculate mass
	const float actorMass = destructible->getChunkMass(chunk.indexInAsset);
	const float scaledMass = scaleMass(actorMass);

	if (!dynamic)
	{
		// Make kinematic if the chunk is not dynamic
		newActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
	}
	else
	{
		newActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
	}

	if (takesImpactDamage)
	{
		// Set contact report threshold if the actor can take impact damage
		newActor->setContactReportThreshold(destructible->getContactReportThreshold(behaviorGroup));
	}

	// Actor:
	if (takesImpactDamage && behaviorGroup.materialStrength > 0.0f)
	{
		newActor->setContactReportThreshold(behaviorGroup.materialStrength);
	}

	if (newActor)
	{
		// apan2
		// bodyDesc.apply(newActor);
		PxRigidBodyExt::setMassAndUpdateInertia(*newActor, scaledMass);
		newActor->setAngularDamping(0.05f);

		++mNumActorsCreatedThisFrame;
		++mTotalChunkCount;

		PhysXObjectDescIntl* actorObjDesc = mModule->mSdk->createObjectDesc(destructible->getAPI(), newActor);
		actorObjDesc->userData = 0;
		destructible->setActorObjDescFlags(actorObjDesc, source->depth);

		if (!(newActor->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC))
		{
			bool isDebris = destructible->getDestructibleParameters().debrisDepth >= 0
				&& source->depth >= destructible->getDestructibleParameters().debrisDepth;
			addActor(*actorObjDesc, *newActor, actorMass, isDebris);
		}

		// Set the PxShape for the chunk and all descendants
		const uint32_t shapeCount = newActor->getNbShapes();
		PX_ALLOCA(shapeArray, physx::PxShape*, shapeCount);
		newActor->getShapes(shapeArray, shapeCount);

		chunk.setShapes(shapeArray, shapeCount);
		chunk.state |= (uint32_t)ChunkVisible;
		if (dynamic)
		{
			chunk.state |= (uint32_t)ChunkDynamic;
			// New visible dynamic chunk, add to visible dynamic chunk shape counts
			destructible->increaseVisibleDynamicChunkShapeCount(hullCount);
			if (source->depth <= destructible->getDestructibleParameters().essentialDepth)
			{
				destructible->increaseEssentialVisibleDynamicChunkShapeCount(hullCount);
			}
		}

		// Create and store an island ID for the root actor
		if (actor != NULL || chunk.islandID == DestructibleStructure::InvalidID)
		{
			chunk.islandID = destructible->getStructure()->newPxActorIslandReference(chunk, *newActor);
		}

		VisibleChunkSetDescendents chunkOp(chunk.indexInAsset + destructible->getFirstChunkIndex(), dynamic);
		forSubtree(chunk, chunkOp, true);

		physx::Array<PxShape*>& shapes = destructible->getStructure()->getChunkShapes(chunk);
		for (uint32_t i = shapes.size(); i--;)
		{
			PxShape* shape = shapes[i];
			PhysXObjectDescIntl* shapeObjDesc = mModule->mSdk->createObjectDesc(destructible->getAPI(), shape);
			shapeObjDesc->userData = &chunk;
		}

		{
			mActorsToAddIndexMap[newActor] = mActorsToAdd.size();
			mActorsToAdd.pushBack(newActor);

			// TODO do we really need this?
			SCOPED_PHYSX_LOCK_WRITE(mPhysXScene);
			if (!(newActor->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC)) // cannot call 'wake-up' on kinematic bodies
			{
				float initWakeCounter = mPhysXScene->getWakeCounterResetValue();
				newActor->setWakeCounter(initWakeCounter);
			}
		}

		destructible->referencedByActor(newActor);	// Must be done after adding to scene, since sleeping is checked
	}

	// Add to static root list if static
	if (!dynamic)
	{
		destructible->getStaticRoots().use(chunk.indexInAsset);
	}

	if (newActor && getModuleDestructible()->m_destructiblePhysXActorReport != NULL)
	{
		getModuleDestructible()->m_destructiblePhysXActorReport->onPhysXActorCreate(*newActor);
	}

	return newActor;
}

void DestructibleScene::addForceToAddActorsMap(physx::PxActor* actor, const ActorForceAtPosition& force)
{
	mForcesToAddToActorsMap[actor] = force;
}

void DestructibleScene::addActorsToScene()
{
	PX_PROFILE_ZONE("DestructibleAddActorsToScene", GetInternalApexSDK()->getContextId());

	if (mActorsToAdd.empty() || mPhysXScene == NULL)
		return;

	{
		SCOPED_PHYSX_LOCK_WRITE(mPhysXScene);
		mPhysXScene->addActors(&mActorsToAdd[0], mActorsToAdd.size());

		{
			PX_PROFILE_ZONE("DestructibleAddForcesToNewSceneActors", GetInternalApexSDK()->getContextId());
			for (HashMap<physx::PxActor*, ActorForceAtPosition>::Iterator iter = mForcesToAddToActorsMap.getIterator(); !iter.done(); iter++)
			{
				PxActor* const actor = iter->first;
				ActorForceAtPosition& forceToAdd = iter->second;
				if (actor->getScene())
				{
					PxRigidDynamic* rigidDynamic = actor->is<physx::PxRigidDynamic>();
					if (rigidDynamic && !(rigidDynamic->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC))
					{
						if (!forceToAdd.force.isZero())
						{
							PxRigidBody* rigidBody = actor->is<physx::PxRigidBody>();
							if (rigidBody)
							{
								if (forceToAdd.usePosition)
								{
									PxRigidBodyExt::addForceAtPos(*rigidBody, forceToAdd.force, forceToAdd.pos, forceToAdd.mode, forceToAdd.wakeup);
								}
								else
								{
									rigidBody->addForce(forceToAdd.force, forceToAdd.mode, forceToAdd.wakeup);
								}
							}
						}
						else
						{
							// No force, but we will apply the wakeup flag
							if (forceToAdd.wakeup)
							{
								rigidDynamic->wakeUp();
							}
							else
							{
								rigidDynamic->putToSleep();
							}
						}
					}
				}
			}
			mForcesToAddToActorsMap.clear();
		}
	}

	mActorsToAdd.clear();
	mActorsToAddIndexMap.clear();
}


bool DestructibleScene::appendShapes(DestructibleStructure::Chunk& chunk, bool dynamic, PxTransform* relTM, PxRigidDynamic* actor)
{
	// PX_PROFILE_ZONE(DestructibleAppendShape, GetInternalApexSDK()->getContextId());

	if (chunk.state & ChunkVisible)
	{
		return true;
	}

	if (chunk.isDestroyed() && actor == NULL)
	{
		return false;
	}

	DestructibleActorImpl* destructible = mDestructibles.direct(chunk.destructibleID);

	if (actor == NULL)
	{
		actor = destructible->getStructure()->getChunkActor(chunk);
		relTM = NULL;
	}

	SCOPED_PHYSX_LOCK_READ(actor->getScene());
	const uint32_t oldActorShapeCount = actor->getNbShapes();

	PhysX3DescTemplateImpl physX3Template;

	destructible->getPhysX3Template(physX3Template);

	DestructibleAssetParametersNS::Chunk_Type* source = destructible->getDestructibleAsset()->mParams->chunks.buf + chunk.indexInAsset;
	const bool takesImpactDamage = destructible->takesImpactDamageAtDepth(source->depth);

	// Shape(s):
	for (uint32_t hullIndex = destructible->getDestructibleAsset()->getChunkHullIndexStart(chunk.indexInAsset); hullIndex < destructible->getDestructibleAsset()->getChunkHullIndexStop(chunk.indexInAsset); ++hullIndex)
	{
		PxConvexMeshGeometry convexShapeDesc;
		convexShapeDesc.convexMesh = destructible->getConvexMesh(hullIndex);
		// Make sure we can get a collision mesh
		if (!convexShapeDesc.convexMesh)
		{
			return false;
		}
		convexShapeDesc.scale.scale = destructible->getScale();
		// Divide out the cooking scale to get the proper scaling
		const PxVec3 cookingScale = getModuleDestructible()->getChunkCollisionHullCookingScale();
		convexShapeDesc.scale.scale.x /= cookingScale.x;
		convexShapeDesc.scale.scale.y /= cookingScale.y;
		convexShapeDesc.scale.scale.z /= cookingScale.z;
		if ((convexShapeDesc.scale.scale - PxVec3(1.0f)).abs().maxElement() < 10 * PX_EPS_F32)
		{
			convexShapeDesc.scale.scale = PxVec3(1.0f);
		}

		PxTransform	localPose;
		if (relTM != NULL)
		{
			localPose = *relTM;
			localPose.p += destructible->getScale().multiply(destructible->getDestructibleAsset()->getChunkPositionOffset(chunk.indexInAsset));
		}
		else
		{
			localPose = destructible->getStructure()->getChunkLocalPose(chunk);
		}

		PxShape* newShape = NULL;

		{
			{
				SCOPED_PHYSX_LOCK_WRITE(actor->getScene());
				newShape = actor->createShape(convexShapeDesc,
					physX3Template.materials.begin(),
					static_cast<uint16_t>(physX3Template.materials.size()),
					static_cast<physx::PxShapeFlags>(physX3Template.shapeFlags));
				newShape->setLocalPose(localPose);

				PX_ASSERT(newShape);
				if (!newShape)
				{
					APEX_INTERNAL_ERROR("Failed to create the PhysX shape.");
					return false;
				}
				physX3Template.apply(newShape);
			}

			const DestructibleActorParamNS::BehaviorGroup_Type& behaviorGroup = destructible->getBehaviorGroup(chunk.indexInAsset);
			if (behaviorGroup.groupsMask.useGroupsMask)
			{
				physx::PxFilterData filterData(behaviorGroup.groupsMask.bits0,
					behaviorGroup.groupsMask.bits1,
					behaviorGroup.groupsMask.bits2,
					behaviorGroup.groupsMask.bits3);
				newShape->setSimulationFilterData(filterData);
				newShape->setQueryFilterData(filterData);
			}
			else if (dynamic && destructible->getDestructibleParameters().useDynamicChunksGroupsMask)
			{
				// Override the filter data
				newShape->setSimulationFilterData(destructible->getDestructibleParameters().dynamicChunksFilterData);
				newShape->setQueryFilterData(destructible->getDestructibleParameters().dynamicChunksFilterData);
			}

			physx::PxPairFlags	pairFlag = (physx::PxPairFlags)physX3Template.contactReportFlags;

			if (takesImpactDamage)
			{
				pairFlag	/* |= PxPairFlag::eNOTIFY_CONTACT_FORCES */
					|= PxPairFlag::eNOTIFY_THRESHOLD_FORCE_PERSISTS
					| PxPairFlag::eNOTIFY_THRESHOLD_FORCE_FOUND
					| PxPairFlag::eNOTIFY_CONTACT_POINTS;
			}

			if (behaviorGroup.materialStrength > 0.0f)
			{
				pairFlag |= PxPairFlag::eMODIFY_CONTACTS;
			}

			if (mApexScene->getApexPhysX3Interface())
				mApexScene->getApexPhysX3Interface()->setContactReportFlags(newShape, pairFlag, destructible->getAPI(), chunk.indexInAsset);
		}

		PhysXObjectDescIntl* shapeObjDesc = mModule->mSdk->createObjectDesc(destructible->getAPI(), newShape);
		shapeObjDesc->userData = &chunk;
	}

	++mTotalChunkCount;
	chunk.state |= (uint8_t)ChunkVisible;
	if (dynamic)
	{
		chunk.state |= (uint8_t)ChunkDynamic;
		// New visible dynamic chunk, add to visible dynamic chunk shape counts
		const uint32_t hullCount = destructible->getDestructibleAsset()->getChunkHullCount(chunk.indexInAsset);
		destructible->increaseVisibleDynamicChunkShapeCount(hullCount);
		if (destructible->getDestructibleAsset()->mParams->chunks.buf[chunk.indexInAsset].depth <= destructible->getDestructibleParameters().essentialDepth)
		{
			destructible->increaseEssentialVisibleDynamicChunkShapeCount(hullCount);
		}
	}

	// Retrieve the island ID associated with the parent PxActor
	chunk.islandID = destructible->getStructure()->actorToIsland[actor];

	const uint32_t shapeCount = actor->getNbShapes();
	PX_ALLOCA(shapeArray, physx::PxShape*, shapeCount);
	{
		actor->getShapes(shapeArray, shapeCount);
	}

	chunk.setShapes(shapeArray + oldActorShapeCount, shapeCount - oldActorShapeCount);
	VisibleChunkSetDescendents chunkOp(chunk.indexInAsset + destructible->getFirstChunkIndex(), dynamic);
	forSubtree(chunk, chunkOp, true);

	// Update the mass
	{
		PhysXObjectDescIntl* actorObjDesc = (PhysXObjectDescIntl*)mModule->mSdk->getPhysXObjectInfo(actor);
		const uintptr_t cindex = (uintptr_t)actorObjDesc->userData;
		if (cindex != 0)
		{
			// In the FIFO, trigger mass update
			PX_ASSERT(mActorFIFO[(uint32_t)~cindex].actor == actor);
			ActorFIFOEntry& FIFOEntry = mActorFIFO[(uint32_t)~cindex];
			FIFOEntry.unscaledMass += destructible->getChunkMass(chunk.indexInAsset);
			bool isKinematic = false;
			{
				isKinematic = actor->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC;
			}
			if (!isKinematic)
			{
				FIFOEntry.flags |= ActorFIFOEntry::MassUpdateNeeded;
			}
		}
	}

	if ((chunk.state & ChunkDynamic) == 0)
	{
		destructible->getStaticRoots().use(chunk.indexInAsset);
	}

	return true;
}

bool DestructibleScene::testWorldOverlap(const ConvexHullImpl& convexHull, const PxTransform& tm, const PxVec3& scale, float padding, const PxFilterData* groupsMask)
{
	physx::PxScene* physxSceneToUse = m_worldSupportPhysXScene != NULL ? m_worldSupportPhysXScene : mPhysXScene;
	if (physxSceneToUse == NULL)
	{
		return false;
	}

	PxMat44 scaledTM(tm);
	scaledTM.scale(PxVec4(scale, 1.f));
	PxBounds3 worldBounds = convexHull.getBounds();
	PxBounds3Transform(worldBounds, scaledTM);
	PX_ASSERT(!worldBounds.isEmpty());
	worldBounds.fattenFast(padding);

	PxBoxGeometry			box(worldBounds.getExtents());
	PxQueryFilterData	filterData;
	filterData.flags = PxQueryFlag::eSTATIC;
	if (groupsMask)
		filterData.data = *groupsMask;
	SCOPED_PHYSX_LOCK_READ(physxSceneToUse);
	PxOverlapBuffer ovBuffer(&mOverlapHits[0], MAX_SHAPE_COUNT);
	physxSceneToUse->overlap(box, PxTransform(worldBounds.getCenter()), ovBuffer, filterData, NULL);
	uint32_t	nbHit = ovBuffer.getNbAnyHits();
	//nbHit	= nbHit >= 0 ? nbHit : MAX_SHAPE_COUNT; //Ivan: it is always true and should be removed
	for (uint32_t i = 0; i < nbHit; i++)
	{
		if (convexHull.intersects(*mOverlapHits[i].shape, tm, scale, padding))
		{
			return true;
		}
	}
	return false;
}


}
} // end namespace nvidia

