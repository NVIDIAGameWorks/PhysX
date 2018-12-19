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
#include "DestructibleStructure.h"
#include "DestructibleScene.h"
#include "ModuleDestructibleImpl.h"
#include "ModulePerfScope.h"
#include "PsIOStream.h"
#include <PxPhysics.h>
#include <PxScene.h>
#include "PxConvexMeshGeometry.h"

#if APEX_USE_PARTICLES
#include "EmitterActor.h"
#include "EmitterGeoms.h"
#endif

#include "RenderDebugInterface.h"
#include "DestructibleActorProxy.h"

#include "DestructibleStructureStressSolver.h"

#if APEX_RUNTIME_FRACTURE
#include "SimScene.h"
#endif

#include "ApexMerge.h"

namespace nvidia
{
namespace destructible
{
using namespace physx;

// Local definitions

#define REDUCE_DAMAGE_TO_CHILD_CHUNKS				1

// DestructibleStructure methods

DestructibleStructure::DestructibleStructure(DestructibleScene* inScene, uint32_t inID) :
	dscene(inScene),
	ID(inID),
	supportDepthChunksNotExternallySupportedCount(0),
	supportInvalid(false),
	actorForStaticChunks(NULL),
	stressSolver(NULL)
{
}

DestructibleStructure::~DestructibleStructure()
{
	if(NULL != stressSolver)
	{
		PX_DELETE(stressSolver);
		stressSolver = NULL;
		dscene->setStressSolverTick(this, false);
	}
	
	for (uint32_t destructibleIndex = destructibles.size(); destructibleIndex--;)
	{
		DestructibleActorImpl* destructible = destructibles[destructibleIndex];
		if (destructible)
		{
			destructible->setStructure(NULL);
			dscene->mDestructibles.direct(destructible->getID()) = NULL;
			dscene->mDestructibles.free(destructible->getID());
		}
	}

	dscene->setStructureSupportRebuild(this, false);
	dscene->setStructureUpdate(this, false);

	for (uint32_t i = 0; i < chunks.size(); ++i)
	{
		Chunk& chunk = chunks[i];
		if ((chunk.state & ChunkVisible) != 0)
		{
			physx::Array<PxShape*>& shapeArray = getChunkShapes(chunk);
			for (uint32_t i = shapeArray.size(); i--;)
			{
				PxShape* shape = shapeArray[i];
				dscene->mModule->mSdk->releaseObjectDesc(shape);
				physx::PxRigidDynamic* actor = shape->getActor()->is<PxRigidDynamic>();
				actor->detachShape(*shape);
				if (dscene->mTotalChunkCount > 0)
				{
					--dscene->mTotalChunkCount;
				}
				SCOPED_PHYSX_LOCK_READ(actor->getScene());
				if (actor->getNbShapes() == 0)
				{
					dscene->releasePhysXActor(*actor);
				}
			}
		}
		chunk.clearShapes();
#if USE_CHUNK_RWLOCK
		if (chunk.lock)
		{
			chunk.lock->~shdfnd::ReadWriteLock();
			PX_FREE(chunk.lock);
		}
		chunk.lock = NULL;
#endif
	}

	actorToIsland.clear();
	islandToActor.clear();

	dscene = NULL;
	ID = (uint32_t)InvalidID;
}

bool DestructibleStructure::addActors(const physx::Array<DestructibleActorImpl*>& destructiblesToAdd)
{
	PX_PROFILE_ZONE("DestructibleAddActors", GetInternalApexSDK()->getContextId());

	// use the scene lock to protect chunk data for
	// multi-threaded obbSweep calls on destructible actors
	// (different lock would be good, but mixing locks can cause deadlocks)
	SCOPED_PHYSX_LOCK_WRITE(dscene->mApexScene);

	const uint32_t oldDestructibleCount = destructibles.size();

	const uint32_t destructibleCount = oldDestructibleCount + destructiblesToAdd.size();
	destructibles.resize(destructibleCount);
	uint32_t totalAddedChunkCount = 0;
	for (uint32_t destructibleIndex = oldDestructibleCount; destructibleIndex < destructibleCount; ++destructibleIndex)
	{
		DestructibleActorImpl* destructible = destructiblesToAdd[destructibleIndex - oldDestructibleCount];
		destructibles[destructibleIndex] = destructible;
		totalAddedChunkCount += destructible->getDestructibleAsset()->getChunkCount();
	}

	const uint32_t oldChunkCount = chunks.size();
	const uint32_t chunkCount = oldChunkCount + totalAddedChunkCount;
	chunks.resize(chunkCount);

	ApexSDKIntl* sdk = dscene->mModule->mSdk;

	// Fix chunk pointers in shapes that may have been moved because of the resize() operation
	for (uint32_t chunkIndex = 0; chunkIndex < oldChunkCount; ++chunkIndex)
	{
		Chunk& chunk = chunks[chunkIndex];
		if ((chunk.state & ChunkVisible) != 0)
		{
			physx::Array<PxShape*>& shapeArray = getChunkShapes(chunk);
			for (uint32_t i = shapeArray.size(); i--;)
			{
				PxShape* shape = shapeArray[i];
				if (shape != NULL)
				{
					PhysXObjectDescIntl* objDesc = sdk->getGenericPhysXObjectInfo(shape);
					if (objDesc != NULL)
					{
						objDesc->userData = &chunk;
					}
				}
			}
		}
	}

	uint32_t chunkIndex = oldChunkCount;
	for (uint32_t destructibleIndex = oldDestructibleCount; destructibleIndex < destructibleCount; ++destructibleIndex)
	{
		DestructibleActorImpl* destructible = destructiblesToAdd[destructibleIndex - oldDestructibleCount];

		const bool formsExtendedStructures = !destructible->isInitiallyDynamic() && destructible->formExtendedStructures();

		if (!formsExtendedStructures)
		{
			PX_ASSERT(destructibleCount == 1);	// All dynamic destructibles must be in their own structure
		}

		destructible->setStructure(this);
		PX_VERIFY(dscene->mDestructibles.useNextFree(destructible->getIDRef()));
		dscene->mDestructibles.direct(destructible->getID()) = destructible;

		//PxVec3 destructibleScale = destructible->getScale();

		destructible->setFirstChunkIndex(chunkIndex);
		for (uint32_t i = 0; i < destructible->getDestructibleAsset()->getChunkCount(); ++i)
		{
			destructible->initializeChunk(i, chunks[chunkIndex++]);
		}
		// This should be done after all chunks are initialized, since it will traverse all chunks
		Chunk& root = chunks[destructible->getFirstChunkIndex()];
		PX_ASSERT(root.isDestroyed());    // ensure shape pointer is still NULL

		// Create a new static actor if this is the first actor
		if (destructibleIndex == 0)
		{
			// Disable visibility before creation
			root.state &= ~(uint8_t)ChunkVisible;
			PxRigidDynamic* rootActor = dscene->createRoot(root, PxTransform(destructible->getInitialGlobalPose()), destructible->isInitiallyDynamic());
			if (!destructible->isInitiallyDynamic() || destructible->initializedFromState())
			{
				actorForStaticChunks = rootActor;
			}
		}
        // Otherwise, append the new destructible actor's root chunk shapes to the existing root chunk's actor
        //     If the new destructible is initialized from state, defer chunk loading to ChunkLoadState
		else if (!destructible->initializedFromState())
		{
			DestructibleActorImpl* firstDestructible = dscene->mDestructibles.direct(chunks[0].destructibleID);
			PxTransform relTM(firstDestructible->getInitialGlobalPose().inverseRT() * destructible->getInitialGlobalPose());
			if (actorForStaticChunks)
			{
				dscene->appendShapes(root, destructible->isInitiallyDynamic(), &relTM, actorForStaticChunks);
				PhysXObjectDescIntl* objDesc = sdk->getGenericPhysXObjectInfo(actorForStaticChunks);
				PX_ASSERT(objDesc->mApexActors.find(destructible->getAPI()) == objDesc->mApexActors.end());
				objDesc->mApexActors.pushBack(destructible->getAPI());
				destructible->referencedByActor(actorForStaticChunks);
			}
			else
			{
				// Disable visibility before creation
				root.state &= ~(uint8_t)ChunkVisible;
				PxRigidDynamic* rootActor = dscene->createRoot(root, PxTransform(destructible->getInitialGlobalPose()), destructible->isInitiallyDynamic());
				actorForStaticChunks = rootActor;
			}
		}

		destructible->setRelativeTMs();
		if (destructible->initializedFromState())
		{
			DestructibleScene::ChunkLoadState chunkLoadState(*dscene);
			dscene->forSubtree(root, chunkLoadState, false);
		}

		destructible->setChunkVisibility(0, destructible->getInitialChunkVisible(root.indexInAsset));
		// init bounds
		destructible->setRenderTMs();
	}

	dscene->setStructureSupportRebuild(this, true);

	return true;
}

struct Ptr_LT
{
	PX_INLINE bool operator()(void* a, void* b) const
	{
		return a < b;
	}
};

bool DestructibleStructure::removeActor(DestructibleActorImpl* destructibleToRemove)
{
	PX_PROFILE_ZONE("DestructibleRemoveActor", GetInternalApexSDK()->getContextId());

	// use the scene lock to protect chunk data for
	// multi-threaded obbSweep calls on destructible actors
	// (different lock would be good, but mixing locks can cause deadlocks)
	SCOPED_PHYSX_LOCK_WRITE(dscene->mApexScene);

	if (destructibleToRemove->getStructure() != this)
	{
		return false;	// Destructible not in structure
	}

	const uint32_t oldDestructibleCount = destructibles.size();

	uint32_t destructibleIndex = 0;
	for (; destructibleIndex < oldDestructibleCount; ++destructibleIndex)
	{
		if (destructibles[destructibleIndex] == destructibleToRemove)
		{
			break;
		}
	}

	if (destructibleIndex == oldDestructibleCount)
	{
		return false;	// Destructible not in structure
	}

	// Remove chunks (releases shapes) and ensure we dissociate from their actors.
	// Note: if we're not in a structure, releasing shapes will always release the actors, which releases the actorObjDesc, thus dissociating us.
	// But if we're in a structure, some other destructible might have shapes in the actor, and we won't get dissociated.  This is why we must
	// Go through the trouble of ensuring dissociation here.
	physx::Array<void*> dissociatedActors;
	uint32_t visibleChunkCount = destructibleToRemove->getNumVisibleChunks();	// Do it this way in case removeChunk doesn't work for some reason
	while (visibleChunkCount--)
	{
		const uint16_t chunkIndex = destructibleToRemove->getVisibleChunks()[destructibleToRemove->getNumVisibleChunks()-1];
		DestructibleStructure::Chunk& chunk = chunks[chunkIndex + destructibleToRemove->getFirstChunkIndex()];
		PxRigidDynamic* actor = getChunkActor(chunk);
		if (actor != NULL)
		{
			dissociatedActors.pushBack(actor);
		}
		removeChunk(chunk);
	}

	// Now sort the actor pointers so we only deal with each once
	const uint32_t dissociatedActorCount = dissociatedActors.size();
	if (dissociatedActorCount > 1)
	{
		nvidia::sort<void*, Ptr_LT>(&dissociatedActors[0], dissociatedActors.size(), Ptr_LT());
	}

	// Run through the sorted list
	void* lastActor = NULL;
	for (uint32_t actorNum = 0; actorNum < dissociatedActorCount; ++actorNum)
	{
		void* nextActor = dissociatedActors[actorNum];
		if (nextActor != lastActor)
		{
			PhysXObjectDescIntl* actorObjDesc = dscene->mModule->mSdk->getGenericPhysXObjectInfo(nextActor);
			if (actorObjDesc != NULL)
			{
				for (uint32_t i = actorObjDesc->mApexActors.size(); i--;)
				{
					if (actorObjDesc->mApexActors[i] == destructibleToRemove->getAPI())
					{
						actorObjDesc->mApexActors.replaceWithLast(i);
						destructibleToRemove->unreferencedByActor((PxRigidDynamic*)nextActor);
					}
				}
				if (actorObjDesc->mApexActors.size() == 0)
				{
					dscene->releasePhysXActor(*static_cast<PxRigidDynamic*>(nextActor));
				}
			}
			lastActor = nextActor;
		}
	}

	// Also check the actor for static chunks
	if (actorForStaticChunks != NULL)
	{
		PhysXObjectDescIntl* actorObjDesc = dscene->mModule->mSdk->getGenericPhysXObjectInfo(actorForStaticChunks);
		if (actorObjDesc != NULL)
		{
			for (uint32_t i = actorObjDesc->mApexActors.size(); i--;)
			{
				if (actorObjDesc->mApexActors[i] == destructibleToRemove->getAPI())
				{
					actorObjDesc->mApexActors.replaceWithLast(i);
					destructibleToRemove->unreferencedByActor(actorForStaticChunks);
				}
			}
			if (actorObjDesc->mApexActors.size() == 0)
			{
				dscene->releasePhysXActor(*static_cast<PxRigidDynamic*>(actorForStaticChunks));
			}
		}
	}

	const uint32_t destructibleToRemoveFirstChunkIndex = destructibleToRemove->getFirstChunkIndex();
	const uint32_t destructibleToRemoveChunkCount = destructibleToRemove->getDestructibleAsset()->getChunkCount();
	const uint32_t destructibleToRemoveChunkIndexStop = destructibleToRemoveFirstChunkIndex + destructibleToRemoveChunkCount;

	destructibleToRemove->setStructure(NULL);

	// Compact destructibles array
	for (; destructibleIndex < oldDestructibleCount-1; ++destructibleIndex)
	{
		destructibles[destructibleIndex] = destructibles[destructibleIndex+1];
		const uint32_t firstChunkIndex = destructibles[destructibleIndex]->getFirstChunkIndex();
		PX_ASSERT(firstChunkIndex >= destructibleToRemoveChunkCount);
		destructibles[destructibleIndex]->setFirstChunkIndex(firstChunkIndex - destructibleToRemoveChunkCount);
	}
	destructibles.resize(oldDestructibleCount-1);

	// Compact chunks array
	chunks.removeRange(destructibleToRemoveFirstChunkIndex, destructibleToRemoveChunkCount);
	const uint32_t chunkCount = chunks.size();

	ApexSDKIntl* sdk = dscene->mModule->mSdk;

	// Fix chunk pointers in shapes that may have been moved because of the removeRange() operation
	for (uint32_t chunkIndex = destructibleToRemoveFirstChunkIndex; chunkIndex < chunkCount; ++chunkIndex)
	{
		Chunk& chunk = chunks[chunkIndex];
		if (chunk.visibleAncestorIndex != (int32_t)InvalidChunkIndex)
		{
			chunk.visibleAncestorIndex -= destructibleToRemoveChunkCount;
		}
		for (uint32_t i = chunk.shapes.size(); i--;)
		{
			PxShape* shape = chunk.shapes[i];
			if (shape != NULL)
			{
				PhysXObjectDescIntl* objDesc = sdk->getGenericPhysXObjectInfo(shape);
				if (objDesc != NULL)
				{
					objDesc->userData = &chunk;
				}
			}
		}
	}

	// Remove obsolete support info
	uint32_t newSupportChunkNum = 0;
	PX_ASSERT(supportDepthChunksNotExternallySupportedCount <= supportDepthChunks.size());
	for (uint32_t supportChunkNum = 0; supportChunkNum < supportDepthChunksNotExternallySupportedCount; ++supportChunkNum)
	{
		const uint32_t supportChunkIndex = supportDepthChunks[supportChunkNum];
		if (supportChunkIndex < destructibleToRemoveFirstChunkIndex)
		{
			// This chunk index will not be affected
			supportDepthChunks[newSupportChunkNum++] = supportChunkIndex;
		}
		else
		if (supportChunkIndex >= destructibleToRemoveChunkIndexStop)
		{
			// This chunk index will be affected
			supportDepthChunks[newSupportChunkNum++] = supportChunkIndex - destructibleToRemoveChunkCount;
		}
	}

	const uint32_t newSupportDepthChunksNotExternallySupportedCount = newSupportChunkNum;

	for (uint32_t supportChunkNum = supportDepthChunksNotExternallySupportedCount; supportChunkNum < supportDepthChunks.size(); ++supportChunkNum)
	{
		const uint32_t supportChunkIndex = supportDepthChunks[supportChunkNum];
		if (supportChunkIndex < destructibleToRemoveFirstChunkIndex)
		{
			// This chunk index will not be affected
			supportDepthChunks[newSupportChunkNum++] = supportChunkIndex;
		}
		else
		if (supportChunkIndex >= destructibleToRemoveChunkIndexStop)
		{
			// This chunk index will be affected
			supportDepthChunks[newSupportChunkNum++] = supportChunkIndex - destructibleToRemoveChunkCount;
		}
	}

	supportDepthChunksNotExternallySupportedCount = newSupportDepthChunksNotExternallySupportedCount;
	supportDepthChunks.resize(newSupportChunkNum);

	dscene->setStructureSupportRebuild(this, true);

	return true;
}


void DestructibleStructure::updateIslands()
{
	PX_PROFILE_ZONE("DestructibleStructureUpdateIslands", GetInternalApexSDK()->getContextId());

	// islandToActor is used within a single frame to reconstruct deserialized islands.
	//     After that frame, there are no guarantees on island recreation within
	//     a single structure between multiple actors.  Thus we reset it each frame.
	if (islandToActor.size())
	{
		islandToActor.clear();
	}

	if (supportInvalid)
	{
		separateUnsupportedIslands();
	}
}


void DestructibleStructure::tickStressSolver(float deltaTime)
{
	PX_PROFILE_ZONE("DestructibleStructureTickStressSolver", GetInternalApexSDK()->getContextId());

	PX_ASSERT(stressSolver != NULL);
	if (stressSolver == NULL)
		return;

	if(stressSolver->isPhysxBasedSim)
	{
		if(stressSolver->isCustomEnablesSimulating)
		{
			stressSolver->physcsBasedStressSolverTick();
		}
	}
	else
	{
		stressSolver->onTick(deltaTime);
		if (supportInvalid)
		{
			stressSolver->onResolve();
		}
	}
}

PX_INLINE uint32_t colorFromUInt(uint32_t x)
{
	x = (x % 26) + 1;
	uint32_t rIndex = x / 9;
	x -= rIndex * 9;
	uint32_t gIndex = x / 3;
	uint32_t bIndex = x - gIndex * 3;
	uint32_t r = (rIndex << 7) - (uint32_t)(rIndex != 0);
	uint32_t g = (gIndex << 7) - (uint32_t)(gIndex != 0);
	uint32_t b = (bIndex << 7) - (uint32_t)(bIndex != 0);
	return 0xFF000000 | r << 16 | g << 8 | b;
}

void DestructibleStructure::visualizeSupport(RenderDebugInterface* debugRender)
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(debugRender);
#else

	// apparently if this method is called before an actor is ever stepped bad things can happen
	if (firstOverlapIndices.size() == 0)
	{
		return;
	}

	for (uint32_t i = 0; i < supportDepthChunks.size(); ++i)
	{
		uint32_t chunkIndex = supportDepthChunks[i];
		Chunk& chunk = chunks[chunkIndex];
		PxRigidDynamic* chunkActor = dscene->chunkIntact(chunk);
		if (chunkActor != NULL)
		{
			SCOPED_PHYSX_LOCK_READ(chunkActor->getScene());
			PhysXObjectDesc* actorObjDesc = (PhysXObjectDesc*) dscene->mModule->mSdk->getPhysXObjectInfo(chunkActor);
			uint32_t color = colorFromUInt((uint32_t)(-(intptr_t)actorObjDesc->userData));
			RENDER_DEBUG_IFACE(debugRender)->setCurrentColor(color);
			PxVec3 actorCentroid = (chunkActor->getGlobalPose() * PxTransform(chunk.localSphereCenter)).p;
			const PxVec3 chunkCentroid = !chunk.isDestroyed() ? getChunkWorldCentroid(chunk) : actorCentroid;
			const uint32_t firstOverlapIndex = firstOverlapIndices[chunkIndex];
			const uint32_t stopOverlapIndex = firstOverlapIndices[chunkIndex + 1];
			for (uint32_t j = firstOverlapIndex; j < stopOverlapIndex; ++j)
			{
				uint32_t overlapChunkIndex = overlaps[j];
				if (overlapChunkIndex > chunkIndex)
				{
					Chunk& overlapChunk = chunks[overlapChunkIndex];
					PxRigidDynamic* overlapChunkActor = dscene->chunkIntact(overlapChunk);
					if (overlapChunkActor == chunkActor ||
					        (overlapChunkActor != NULL &&
							 chunkActor->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC &&
							 overlapChunkActor->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC))
					{
						PxVec3 actorCentroid = (overlapChunkActor->getGlobalPose() * PxTransform(overlapChunk.localSphereCenter)).p;
						const PxVec3 overlapChunkCentroid = !overlapChunk.isDestroyed() ? getChunkWorldCentroid(overlapChunk) : actorCentroid;
						// Draw line from chunkCentroid to overlapChunkCentroid
						RENDER_DEBUG_IFACE(debugRender)->debugLine(chunkCentroid, overlapChunkCentroid);
					}
				}
			}
			if (chunk.flags & ChunkExternallySupported)
			{
				// Draw mark at chunkCentroid
				DestructibleActorImpl* destructible = dscene->mDestructibles.direct(chunk.destructibleID);
				if (destructible)
				{
					PxBounds3 bounds = destructible->getDestructibleAsset()->getChunkShapeLocalBounds(chunk.indexInAsset);
					PxMat44 tm;
					if (chunk.isDestroyed())
					{
						tm = PxMat44(chunkActor->getGlobalPose());
					}
					else
					{
						tm = PxMat44(getChunkGlobalPose(chunk));
					}
					tm.scale(PxVec4(destructible->getScale(), 1.f));
					PxBounds3Transform(bounds, tm);
					RENDER_DEBUG_IFACE(debugRender)->debugBound(bounds);
				}
			}
		}
	}
#endif
}

uint32_t DestructibleStructure::damageChunk(Chunk& chunk, const PxVec3& position, const PxVec3& direction, bool fromImpact, float damage, float damageRadius,
												physx::Array<FractureEvent> outputs[], uint32_t& possibleDeleteChunks, float& totalDeleteChunkRelativeDamage,
												uint32_t& maxDepth, uint32_t depth, uint16_t stopDepth, float padding)
{
	if (depth > DamageEvent::MaxDepth || depth > (uint32_t)stopDepth)
	{
		return 0;
	}

	DestructibleActorImpl* destructible = dscene->mDestructibles.direct(chunk.destructibleID);

	if (depth > destructible->getLOD())
	{
		return 0;
	}

	physx::Array<FractureEvent>& output = outputs[depth];

	const DestructibleAssetParametersNS::Chunk_Type& source = destructible->getDestructibleAsset()->mParams->chunks.buf[chunk.indexInAsset];

	if (source.flags & DestructibleAssetImpl::UnfracturableChunk)
	{
		return 0;
	}

	if (chunk.isDestroyed())
	{
		return 0;
	}

	const DestructibleParameters& destructibleParameters = destructible->getDestructibleParameters();

	const uint32_t originalEventBufferSize = output.size();

	const bool hasChildren = source.numChildren != 0 && depth < destructible->getLOD();

	// Support legacy behavior
	const bool useLegacyChunkOverlap = destructible->getUseLegacyChunkBoundsTesting();
	const bool useLegacyDamageSpread = destructible->getUseLegacyDamageRadiusSpread();

	// Get behavior group
	const DestructibleActorParamNS::BehaviorGroup_Type& behaviorGroup = destructible->getBehaviorGroupImp(source.behaviorGroupIndex);

	// Get radial info from behavior group
	float minRadius = 0.0f;
	float maxRadius = 0.0f;
	float falloff = 1.0f;
	if (!useLegacyDamageSpread)
	{
		// New behavior
		minRadius = behaviorGroup.damageSpread.minimumRadius;
		falloff = behaviorGroup.damageSpread.falloffExponent;

		if (!fromImpact)
		{
			maxRadius = minRadius + damageRadius*behaviorGroup.damageSpread.radiusMultiplier;
		}
		else
		{
			maxRadius = behaviorGroup.damageToRadius;
			if (behaviorGroup.damageThreshold > 0)
			{
				maxRadius *= damage / behaviorGroup.damageThreshold;
			}
		}
	}
	else
	{
		// Legacy behavior
		if (damageRadius == 0.0f)
		{
			maxRadius = destructible->getLinearSize() * behaviorGroup.damageToRadius;
			if (behaviorGroup.damageThreshold > 0)
			{
				maxRadius *= damage / behaviorGroup.damageThreshold;
			}
		}
		else
		{
			maxRadius = damageRadius;
		}
	}

	float overlapDistance = PX_MAX_F32;
	if (!useLegacyChunkOverlap)
	{
		destructible->getDestructibleAsset()->chunkAndSphereInProximity(chunk.indexInAsset, destructible->getChunkPose(chunk.indexInAsset), destructible->getScale(), position, maxRadius, padding, &overlapDistance);
	}
	else
	{
	PxVec3 disp = getChunkWorldCentroid(chunk) - position;
	float dist = disp.magnitude();
		overlapDistance = dist - maxRadius;
		padding = 0.0f;	// Note we're overriding the function parameter here, as it didn't exist in the old (legacy) code
		if (!hasChildren)	// Test against bounding sphere for smallest chunks
		{
			overlapDistance -= chunk.localSphereRadius;
		}
	}

	uint32_t fractureCount = 0;

	const float recipRadiusRange = maxRadius > minRadius ? 1.0f/(maxRadius - minRadius) : 0.0f;

	const bool canFracture =
		source.depth >= (uint16_t)destructibleParameters.minimumFractureDepth &&	// At or deeper than minimum fracture depth
		(source.flags & DestructibleAssetImpl::DescendantUnfractureable) == 0 &&			// Some descendant cannot be fractured
		(source.flags & DestructibleAssetImpl::UndamageableChunk) == 0 &&					// This chunk is not to be fractured (though a descendant may be)
		(useLegacyChunkOverlap || source.depth >= destructible->getSupportDepth());		// If we're using the new chunk overlap tests, we will only fracture at or deeper than the support depth

	const bool canCrumble = (destructibleParameters.flags & DestructibleParametersFlag::CRUMBLE_SMALLEST_CHUNKS) != 0 && (source.flags & DestructibleAssetImpl::UncrumbleableChunk) == 0;

	const bool forceCrumbleOrRTFracture =
		source.depth < (uint16_t)destructibleParameters.minimumFractureDepth &&	// minimum fracture depth deeper than this chunk
		!hasChildren &&																// leaf chunk
		(canCrumble || (destructibleParameters.flags & DestructibleParametersFlag::CRUMBLE_VIA_RUNTIME_FRACTURE) != 0);	// set to crumble or RT fracture

	if (canFracture || forceCrumbleOrRTFracture)
	{
	uint32_t virtualEventFlag = 0;
	for (uint32_t it = 0; it < 2; ++it)	// Try once as a real event, then possibly as a virtual event
	{
		if (overlapDistance < padding)	// Overlap distance can change
		{
			float damageFraction = 1;
			if (!useLegacyDamageSpread)
			{
				const float radius = maxRadius + overlapDistance;
				if (radius > minRadius && recipRadiusRange > 0.0f)
				{
					if (radius < maxRadius)
					{
						damageFraction = PxPow( (maxRadius - radius)*recipRadiusRange, falloff);
					}
					else
					{
						damageFraction = 0.0f;
					}
				}
			}
			else
			{
				if (falloff > 0.0f && maxRadius > 0.0f)
				{
					damageFraction -= PxMax((maxRadius + overlapDistance) / maxRadius, 0.0f);
				}
			}
			const float oldChunkDamage = chunk.damage;
			float effectiveDamage = damageFraction * damage;
			chunk.damage += effectiveDamage;

			if ((chunk.damage >= behaviorGroup.damageThreshold) || (fromImpact && (behaviorGroup.materialStrength > 0)) || forceCrumbleOrRTFracture)
			{
					chunk.damage = behaviorGroup.damageThreshold;
				// Fracture
#if REDUCE_DAMAGE_TO_CHILD_CHUNKS
				if (it == 0)
				{
					damage -= chunk.damage - oldChunkDamage;
				}
#endif
				// this is an alternative deletion path (instead of using the delete flag on the fracture event)
				// this can allow us to skip some parts of damageChunk() and fractureChunk(), but it involves duplicating some code
				// we are still relying on the delete flag for now
				if(destructible->mInDeleteChunkMode && false)
				{
					removeChunk(chunks[chunk.indexInAsset]);
					uint16_t parentIndex = source.parentIndex;

					Chunk* rootChunk = getRootChunk(chunk);
					const bool dynamic = rootChunk != NULL ? (rootChunk->state & ChunkDynamic) != 0 : false;
					if(!dynamic)
					{
						Chunk* parent = (parentIndex != DestructibleAssetImpl::InvalidChunkIndex) ? &chunks[parentIndex + destructible->getFirstChunkIndex()] : NULL;
						const DestructibleAssetParametersNS::Chunk_Type* parentSource = parent ? (destructible->getDestructibleAsset()->mParams->chunks.buf + parent->indexInAsset) : NULL;
						const uint32_t startIndex = parentSource ? parentSource->firstChildIndex + destructible->getFirstChunkIndex() : chunk.indexInAsset /* + actor first index */;
						const uint32_t stopIndex = parentSource ? startIndex + parentSource->numChildren : startIndex + 1;
						// Walk children of current parent (siblings)
						for (uint32_t childIndex = startIndex; childIndex < stopIndex; ++childIndex)
						{
							Chunk& child = chunks[childIndex];
							if (child.isDestroyed())
							{
								continue;
							}
							bool belowSupportDepth = (child.flags & ChunkBelowSupportDepth) != 0;
							if (&child != &chunk && !belowSupportDepth)
							{
								// this is a sibling, which we can keep because we're at a supported depth or above
								if ((child.state & ChunkVisible) == 0)
								{
									// Make this chunk shape visible, attach to root bone
									if (dscene->appendShapes(child, dynamic))
									{
										destructible->setChunkVisibility(child.indexInAsset, true);
									}
								}
							}
						}
					}

					while(parentIndex != DestructibleAssetImpl::InvalidChunkIndex)
					{
						Chunk * parent = &chunks[parentIndex + destructible->getFirstChunkIndex()];
						if(parent->state & ChunkVisible)
						{
							removeChunk(*parent);
						}
						PX_ASSERT(parent->indexInAsset == parentIndex + destructible->getFirstChunkIndex());
						parentIndex = destructible->getDestructibleAsset()->mParams->chunks.buf[parent->indexInAsset].parentIndex;
					}
				}
				else
				{
					FractureEvent& fractureEvent = output.insert();
					fractureEvent.position = position;
					fractureEvent.impulse = PxVec3(0.0f);
					fractureEvent.chunkIndexInAsset = chunk.indexInAsset;
					fractureEvent.destructibleID = destructible->getID();
					fractureEvent.flags = virtualEventFlag;
					fractureEvent.damageFraction = damageFraction;
					const bool normalCrumbleCondition = canCrumble && !hasChildren && damageFraction*damage >= behaviorGroup.damageThreshold;
					if (it == 0 && (normalCrumbleCondition || forceCrumbleOrRTFracture))
					{
						// Crumble
						fractureEvent.flags |= FractureEvent::CrumbleChunk;
					}
						fractureEvent.deletionWeight = 0.0f;
						if (destructibleParameters.debrisDepth >= 0 && source.depth >= (uint16_t)destructibleParameters.debrisDepth)
						{
							++possibleDeleteChunks;
//							const float relativeDamage = behaviorGroup.damageThreshold > 0.0f ? effectiveDamage/behaviorGroup.damageThreshold : 1.0f;
							fractureEvent.deletionWeight = 1.0f;// - PxExp(-relativeDamage);	// Flat probability for now
							totalDeleteChunkRelativeDamage += fractureEvent.deletionWeight;
						}
				}
			}
			if (!(destructibleParameters.flags & DestructibleParametersFlag::ACCUMULATE_DAMAGE))
			{
				chunk.damage = 0;
			}
			if (it == 1)
			{
				chunk.damage = oldChunkDamage;
			}
		}

		if (it == 1)
		{
			break;
		}

		fractureCount = output.size() - originalEventBufferSize;
		if (!hasChildren || fractureCount > 0)
		{
			break;
		}

			if (useLegacyChunkOverlap)
			{
				// Try treating this as a childless chunk.  
				overlapDistance -= chunk.localSphereRadius;
			}

			// If we have bounding sphere overlap, mark it as a virtual fracture event
		virtualEventFlag = FractureEvent::Virtual;
	}
	}
	else
	if (useLegacyChunkOverlap)
	{
		// Legacy system relied on this being done in the loop above (in the other branch of the conditional)
		overlapDistance -= chunk.localSphereRadius;
	}

	if (fractureCount > 0)
	{
		maxDepth = PxMax(depth, maxDepth);
	}

	if (hasChildren && overlapDistance < padding && depth < (uint32_t)stopDepth)
	{
		// Recurse
		const uint32_t startIndex = source.firstChildIndex + destructible->getFirstChunkIndex();
		const uint32_t stopIndex = startIndex + source.numChildren;
		for (uint32_t childIndex = startIndex; childIndex < stopIndex; ++childIndex)
		{
			fractureCount += damageChunk(chunks[childIndex], position, direction, fromImpact, damage, damageRadius, outputs,
										 possibleDeleteChunks, totalDeleteChunkRelativeDamage, maxDepth, depth + 1, stopDepth, padding);
		}
	}

	return fractureCount;
}


/*
	fractureChunk causes the chunk referenced in fractureEvent to become a dynamic, standalone chunk.

	If the chunk was invisible because it was an ancestor of a visible chunk, then the ancestor will desroyed,
	and appropriate descendants (including the chunk in question) will be made visible.

	Any chunks which become visible that are at, or deeper than, the support depth will also become standalone
	dynamic chunks.
*/

void DestructibleStructure::fractureChunk(const FractureEvent& fractureEvent)
{
	DestructibleActorImpl* destructible = dscene->mDestructibles.direct(fractureEvent.destructibleID);
	if (!destructible)
	{
		return;
	}

	destructible->wakeForEvent();

	// ===SyncParams===
	if(0 != destructible->getSyncParams().getUserActorID())
	{
	destructible->evaluateForHitChunkList(fractureEvent);
	}

	Chunk& chunk = chunks[fractureEvent.chunkIndexInAsset + destructible->getFirstChunkIndex()];

	const PxVec3& position = fractureEvent.position;
	const PxVec3& impulse = fractureEvent.impulse;
	if(fractureEvent.flags & FractureEvent::CrumbleChunk)
	{
		chunk.flags |= ChunkCrumbled;
	}

	const DestructibleAssetParametersNS::Chunk_Type& source = destructible->getDestructibleAsset()->mParams->chunks.buf[chunk.indexInAsset];

	if ((fractureEvent.flags & FractureEvent::Forced) == 0 && (source.flags & DestructibleAssetImpl::UnfracturableChunk) != 0)
	{
		return;
	}

	if (chunk.isDestroyed())
	{
		return;
	}

#if APEX_RUNTIME_FRACTURE
	// The runtime fracture flag pre-empts the crumble flag
	if (source.flags & DestructibleAssetImpl::RuntimeFracturableChunk)
	{
		chunk.flags |= ChunkRuntime;
		chunk.flags &= ~(uint8_t)ChunkCrumbled;
	}
#endif

	bool loneDynamicShape = false;
	if ((chunk.state & ChunkVisible) != 0 && chunkIsSolitary(chunk) && (chunk.state & ChunkDynamic) != 0)
	{
		loneDynamicShape = true;
	}

	uint16_t depth = source.depth;

	PxRigidDynamic* actor = getChunkActor(chunk);
	PX_ASSERT(actor != NULL);

	SCOPED_PHYSX_LOCK_READ(actor->getScene());
	bool isKinematic = actor->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC;

	if (loneDynamicShape)
	{
		if(0 == (fractureEvent.flags & FractureEvent::Manual)) // if it is a manual fractureEvent, we do not consider giving it an impulse nor to crumble it
		{
			if (isKinematic)
			{
				PhysXObjectDescIntl* actorObjDesc = (PhysXObjectDescIntl*)dscene->mModule->mSdk->getPhysXObjectInfo(actor);
				if (actorObjDesc != NULL)
				{
					uintptr_t& cindex = (uintptr_t&)actorObjDesc->userData;
					if (cindex != 0)
					{
						// Dormant actor - turn dynamic
						const uint32_t dormantActorIndex = (uint32_t)~cindex; 
						DormantActorEntry& dormantActorEntry = dscene->mDormantActors.direct(dormantActorIndex);
						dormantActorEntry.actor = NULL;
						dscene->mDormantActors.free(dormantActorIndex);
						cindex = (uintptr_t)0;
						{
							SCOPED_PHYSX_LOCK_WRITE(actor->getScene());
							actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, false);
							dscene->addActor(*actorObjDesc, *actor, dormantActorEntry.unscaledMass,
								((dormantActorEntry.flags & ActorFIFOEntry::IsDebris) != 0));
							actor->wakeUp();
						}
						isKinematic = false;
					}
				}
			}

			if (!isKinematic && impulse.magnitudeSquared() > 0.0f && ((chunk.flags & ChunkCrumbled) != 0 || (fractureEvent.flags & FractureEvent::DamageFromImpact) == 0))
			{
				addChunkImpluseForceAtPos(chunk, impulse, position);
			}
			if ((chunk.flags & ChunkCrumbled))
			{
				crumbleChunk(fractureEvent, chunk);
			}
#if APEX_RUNTIME_FRACTURE
			else if ((chunk.flags & ChunkRuntime))
			{
				runtimeFractureChunk(fractureEvent, chunk);
			}
#endif
			return;
		}
	}

	Chunk* rootChunk = getRootChunk(chunk);
	const bool dynamic = rootChunk != NULL ? (rootChunk->state & ChunkDynamic) != 0 : false;

	uint16_t parentIndex = source.parentIndex;
	uint32_t chunkIndex = chunk.indexInAsset + destructible->getFirstChunkIndex();

	//Sync the shadow scene if using physics based stress solver
	if(stressSolver&&stressSolver->isPhysxBasedSim)
	{
		stressSolver->removeChunkFromShadowScene(false,chunkIndex);
	}
	for (;;)
	{
		Chunk* parent = (parentIndex != DestructibleAssetImpl::InvalidChunkIndex) ? &chunks[parentIndex + destructible->getFirstChunkIndex()] : NULL;
		const DestructibleAssetParametersNS::Chunk_Type* parentSource = parent ? (destructible->getDestructibleAsset()->mParams->chunks.buf + parent->indexInAsset) : NULL;
		const uint32_t startIndex = parentSource ? parentSource->firstChildIndex + destructible->getFirstChunkIndex() : chunkIndex;
		const uint32_t stopIndex = parentSource ? startIndex + parentSource->numChildren : startIndex + 1;
		// Walk children of current parent (siblings)
		for (uint32_t childIndex = startIndex; childIndex < stopIndex; ++childIndex)
		{
			Chunk& child = chunks[childIndex];
			if (child.isDestroyed())
			{
				continue;
			}

			bool belowSupportDepth = (child.flags & ChunkBelowSupportDepth) != 0;

			if (&child != &chunk && !belowSupportDepth)
			{
				// this is a sibling, which we can keep because we're at a supported depth or above
				if ((child.state & ChunkVisible) == 0)
				{
					// Make this chunk shape visible, attach to root bone
					dscene->appendShapes(child, dynamic);
					destructible->setChunkVisibility(child.indexInAsset, true);
				}
			}
			else if (child.flags & ChunkCrumbled)
			{
				// This chunk needs to be crumbled
				if (isKinematic)
				{
					crumbleChunk(fractureEvent, child, &impulse);
				}
				else
				{
					if (impulse.magnitudeSquared() > 0.0f)
					{
						addChunkImpluseForceAtPos(child, impulse, position);
					}
					crumbleChunk(fractureEvent, child);
				}
				if (parent)
				{
					parent->flags |= ChunkMissingChild;
				}
				setSupportInvalid(true);
			}
#if APEX_RUNTIME_FRACTURE
			else if (child.flags & ChunkRuntime)
			{
				runtimeFractureChunk(fractureEvent, child);
				if (parent)
				{
					parent->flags |= ChunkMissingChild;
				}
				setSupportInvalid(true);
			}
#endif
			else
			{
				// This is either the chunk or a sibling, which we are breaking off
				if ((fractureEvent.flags & FractureEvent::Silent) == 0 && dscene->mModule->m_chunkReport != NULL)
				{
					dscene->createChunkReportDataForFractureEvent(fractureEvent, destructible, child);
				}
				destructible->setChunkVisibility(child.indexInAsset, false);	// In case its mesh changes

				// if the fractureEvent is manually generated, we remove the chunk instead of simulating it
				if(0 != (fractureEvent.flags & FractureEvent::DeleteChunk))
				{
					removeChunk(child);
				}
				else
				{
					PX_ASSERT(0 == (fractureEvent.flags & FractureEvent::DeleteChunk));
					PxRigidDynamic* oldActor = getChunkActor(child);
					PxRigidDynamic* newActor = dscene->createRoot(child, getChunkGlobalPose(child), true);
					if (newActor != NULL && oldActor == actor)
					{
						physx::PxRigidDynamic* newRigidDynamic = newActor->is<physx::PxRigidDynamic>();
						PxVec3 childCenterOfMass = newRigidDynamic->getGlobalPose().transform(newRigidDynamic->getCMassLocalPose().p);
						{
							SCOPED_PHYSX_LOCK_WRITE(actor->getScene());
							newActor->setLinearVelocity(physx::PxRigidBodyExt::getVelocityAtPos(*actor, childCenterOfMass));
							newActor->setAngularVelocity(actor->getAngularVelocity());
						}
					}
				}

				PX_ASSERT(child.isDestroyed() == ((child.state & ChunkVisible) == 0));
				if (!child.isDestroyed())
				{
					destructible->setChunkVisibility(child.indexInAsset, true);
					if (&child == &chunk)
					{
						if (impulse.magnitudeSquared() > 0.0f)
						{
							// Bring position nearer to chunk for better effect
							PxVec3 newPosition = position;
							const PxVec3 c = getChunkWorldCentroid(child);
							const PxVec3 disp = newPosition - c;
							const float d2 = disp.magnitudeSquared();
							if (d2 > child.localSphereRadius * child.localSphereRadius)
							{
								newPosition = c + disp * (disp.magnitudeSquared() * PxRecipSqrt(d2));
							}
							addChunkImpluseForceAtPos(child, impulse, newPosition);
						}
					}
					addDust(child);
				}
				if (parent)
				{
					parent->flags |= ChunkMissingChild;
				}
				setSupportInvalid(true);
			}
		}
		if (!parent)
		{
			break;
		}
		if (parent->state & ChunkVisible)
		{
			removeChunk(*parent);
		}
		else if (parent->isDestroyed())
		{
			break;
		}
		parent->clearShapes();
		chunkIndex = parentIndex;
		parentIndex = parentSource->parentIndex;
		PX_ASSERT(depth > 0);
		if (depth > 0)
		{
			--depth;
		}
	}

	dscene->capDynamicActorCount();
}

void DestructibleStructure::crumbleChunk(const FractureEvent& fractureEvent, Chunk& chunk, const PxVec3* impulse)
{
	if (chunk.isDestroyed())
	{
		return;
	}

	DestructibleActorImpl* destructible = dscene->mDestructibles.direct(chunk.destructibleID);

#if APEX_RUNTIME_FRACTURE
	if ((destructible->getDestructibleParameters().flags & DestructibleParametersFlag::CRUMBLE_VIA_RUNTIME_FRACTURE) != 0)
	{
		runtimeFractureChunk(fractureEvent, chunk);
		return;
	}
#else
	PX_UNUSED(fractureEvent);
#endif

	dscene->getChunkReportData(chunk, ApexChunkFlag::DESTROYED_CRUMBLED);

	PX_ASSERT(chunk.reportID < dscene->mChunkReportHandles.size() || chunk.reportID == DestructibleScene::InvalidReportID);
	if (chunk.reportID < dscene->mChunkReportHandles.size())
	{
		IntPair& handle = dscene->mChunkReportHandles[chunk.reportID];
		if ( (uint32_t)handle.i0 < dscene->mDamageEventReportData.size() )
		{
			ApexDamageEventReportDataImpl& DamageEventReport = dscene->mDamageEventReportData[(uint32_t)handle.i0];

			DamageEventReport.impactDamageActor = fractureEvent.impactDamageActor;
			DamageEventReport.appliedDamageUserData = fractureEvent.appliedDamageUserData;
			DamageEventReport.hitPosition = fractureEvent.position;
			DamageEventReport.hitDirection = fractureEvent.hitDirection;
		}
	}

	EmitterActor* emitterToUse = destructible->getCrumbleEmitter() && destructible->isCrumbleEmitterEnabled() ? destructible->getCrumbleEmitter() : NULL;

	const float crumbleParticleSpacing = destructible->getCrumbleParticleSpacing();

	if (crumbleParticleSpacing > 0.0f && (emitterToUse != NULL || dscene->mModule->m_chunkCrumbleReport != NULL))
	{
		physx::Array<PxVec3> volumeFillPos;
		/// \todo expose jitter
		const float jitter = 0.25f;
		for (uint32_t hullIndex = destructible->getDestructibleAsset()->getChunkHullIndexStart(chunk.indexInAsset); hullIndex < destructible->getDestructibleAsset()->getChunkHullIndexStop(chunk.indexInAsset); ++hullIndex)
		{
			destructible->getDestructibleAsset()->chunkConvexHulls[hullIndex].fill(volumeFillPos, getChunkGlobalPose(chunk), destructible->getScale(), destructible->getCrumbleParticleSpacing(), jitter, 250, true);
		}
		destructible->spawnParticles(emitterToUse, dscene->mModule->m_chunkCrumbleReport, chunk, volumeFillPos, true, impulse);
	}

	if (chunk.state & ChunkVisible)
	{
		removeChunk(chunk);
	}
}

#if APEX_RUNTIME_FRACTURE
void DestructibleStructure::runtimeFractureChunk(const FractureEvent& fractureEvent, Chunk& chunk)
{
	if (chunk.isDestroyed())
	{
		return;
	}

	DestructibleActorImpl* destructible = dscene->mDestructibles.direct(chunk.destructibleID);
	PX_UNUSED(destructible);

	if (destructible->getRTActor() != NULL)
	{
		destructible->getRTActor()->patternFracture(fractureEvent,true);
	}

	if (chunk.state & ChunkVisible)
	{
		removeChunk(chunk);
	}
}
#endif

void DestructibleStructure::addDust(Chunk& chunk)
{
	PX_UNUSED(chunk);
#if 0	// Not implementing dust in 1.2.0
	if (chunk.isDestroyed())
	{
		return;
	}

	DestructibleActor* destructible = dscene->mDestructibles.direct(chunk.destructibleID);

	EmitterActor* emitterToUse = destructible->getDustEmitter() && destructible->getDustEmitter() ? destructible->getDustEmitter() : NULL;

	const float dustParticleSpacing = destructible->getDustParticleSpacing();

	if (dustParticleSpacing > 0.0f && (emitterToUse != NULL || dscene->mModule->m_chunkDustReport != NULL))
	{
		physx::Array<PxVec3> tracePos;
		/// \todo expose jitter
		const float jitter = 0.25f;
		destructible->getAsset()->traceSurfaceBoundary(tracePos, chunk.indexInAsset, getChunkGlobalPose(chunk), destructible->getScale(), dustParticleSpacing, jitter, 0.5f * dustParticleSpacing, 250);
		destructible->spawnParticles(emitterToUse, dscene->mModule->m_chunkDustReport, chunk, tracePos);
	}
#endif
}

void DestructibleStructure::removeChunk(Chunk& chunk)
{
	dscene->scheduleChunkShapesForDelete(chunk);
	DestructibleActorImpl* destructible = dscene->mDestructibles.direct(chunk.destructibleID);

	//Sync the shadow scene if using physics based stress solver
	if(stressSolver&&stressSolver->isPhysxBasedSim)
	{
		uint32_t chunkIndex = chunk.indexInAsset + destructible->getFirstChunkIndex();
		stressSolver->removeChunkFromShadowScene(false,chunkIndex);
	}
	destructible->setChunkVisibility(chunk.indexInAsset, false);
	const DestructibleAssetParametersNS::Chunk_Type& source = destructible->getDestructibleAsset()->mParams->chunks.buf[chunk.indexInAsset];
	if (source.parentIndex != DestructibleAssetImpl::InvalidChunkIndex)
	{
		Chunk& parent = chunks[source.parentIndex + destructible->getFirstChunkIndex()];
		parent.flags |= ChunkMissingChild;
	}
	// Remove from static roots list (this no-ops if the chunk was not in the static roots list)
	destructible->getStaticRoots().free(chunk.indexInAsset);
	// No more static chunks, make sure this actor field is cleared
	if (actorForStaticChunks != NULL)
	{
		SCOPED_PHYSX_LOCK_READ(actorForStaticChunks->getScene());
		if (actorForStaticChunks->getNbShapes() == 0)
		{
			actorForStaticChunks = NULL;
		}
	}
}

struct ChunkIsland
{
	ChunkIsland() : actor(NULL), flags(0), actorIslandDataIndex(0) {}

	physx::Array<uint32_t>	indices;
	PxRigidDynamic*			actor;
	uint32_t				flags;
	uint32_t				actorIslandDataIndex;
};

struct ActorIslandData
{
	ActorIslandData() : islandCount(0), flags(0) {}

	enum Flag
	{
		SubmmittedForMassRecalc = 0x1
	};

	uint32_t	islandCount;
	uint32_t	flags;
};

void DestructibleStructure::separateUnsupportedIslands()
{
	// Find instances where an island broke away from an existing
	// island, but instead of two new islands there should actually
	// have been three (or more) because of discontinuities.  Fix it.

	PX_PROFILE_ZONE("DestructibleSeparateUnsupportedIslands", GetInternalApexSDK()->getContextId());
	physx::Array<uint32_t> chunksRemaining = supportDepthChunks;
	physx::Array<ChunkIsland> islands;

	// Instead of sorting to find islands that share actors, will merely count.
	// Since we expect the number of new islands to be small, we should be able
	// to eliminate most of the work this way.
	physx::Array<ActorIslandData> islandData(dscene->mActorFIFO.size() + dscene->mDormantActors.usedCount() + 1);	// Max size needed

	while (chunksRemaining.size())
	{
		uint32_t chunkIndex = chunksRemaining.back();
		chunksRemaining.popBack();

		Chunk& chunk = chunks[chunkIndex];

		if ((chunk.state & ChunkTemp0) != 0)
		{
			continue;
		}
		PxRigidDynamic* actor = dscene->chunkIntact(chunk);
		if (actor == NULL)
		{
			// we conveniently update and find recently broken links here since this loop is executed here
			if(NULL != stressSolver)
			{
				if(!stressSolver->isPhysxBasedSim)
				{
					stressSolver->onUpdate(chunkIndex);
				}
			}
			
			continue;
		}
		SCOPED_PHYSX_LOCK_READ(actor->getScene());
		ChunkIsland& island = islands.insert();
		island.indices.pushBack(chunkIndex);
		island.actor = actor;
		const bool actorStatic = (chunk.state & ChunkDynamic) == 0;
		PhysXObjectDesc* actorObjDesc = (PhysXObjectDesc*) dscene->mModule->mSdk->getPhysXObjectInfo(island.actor);
		int32_t index = -(int32_t)(intptr_t)actorObjDesc->userData;
		if (index > 0 && actor->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC)
		{
			index = int32_t(dscene->mDormantActors.getRank((uint32_t)index-1) + dscene->mActorFIFO.size());	// Get to the dormant portion of the array
		}
		PX_ASSERT(index >= 0 && index < (int32_t)islandData.size());

		island.actorIslandDataIndex = (uint32_t)index;

		++islandData[(uint32_t)index].islandCount;
		chunk.state |= ChunkTemp0;
		for (uint32_t i = 0; i < island.indices.size(); ++i)
		{
			const uint32_t chunkIndex = island.indices[i];
			Chunk& chunk = chunks[chunkIndex];
			island.flags |= chunk.flags & ChunkExternallySupported;
			DestructibleActorImpl* dactor = dscene->mDestructibles.direct(chunk.destructibleID);
			const uint32_t visibleAssetIndex = chunk.visibleAncestorIndex >= 0 ? (uint32_t)chunk.visibleAncestorIndex - dactor->getFirstChunkIndex() : chunk.indexInAsset;
			DestructibleAssetParametersNS::Chunk_Type& source = dactor->getDestructibleAsset()->mParams->chunks.buf[visibleAssetIndex];
			if (source.flags & (DestructibleAssetImpl::UnfracturableChunk | DestructibleAssetImpl::DescendantUnfractureable))
			{
				island.flags |= ChunkExternallySupported;
			}
			const uint32_t firstOverlapIndex = firstOverlapIndices[chunkIndex];
			const uint32_t stopOverlapIndex = firstOverlapIndices[chunkIndex + 1];
			for (uint32_t j = firstOverlapIndex; j < stopOverlapIndex; ++j)
			{
				const uint32_t overlapChunkIndex = overlaps[j];
				Chunk& overlapChunk = chunks[overlapChunkIndex];
				if ((overlapChunk.state & ChunkTemp0) == 0)
				{
					PxRigidDynamic* overlapActor = dscene->chunkIntact(overlapChunk);
					if (overlapActor != NULL)
					{
						if (overlapActor == actor ||
						        ((island.flags & ChunkExternallySupported) != 0 &&
						         actorStatic && (overlapChunk.state & ChunkDynamic) == 0))
						{
							island.indices.pushBack(overlapChunkIndex);
							overlapChunk.state |= (uint32_t)ChunkTemp0;
						}
					}
				}
			}
		}
	}

	physx::Array<PxRigidDynamic*> dirtyActors;	// will need mass properties recalculated

	physx::Array<DestructibleActorImpl*> destructiblesInIsland;

	for (uint32_t islandNum = 0; islandNum < islands.size(); ++islandNum)
	{
		ChunkIsland& island = islands[islandNum];

		SCOPED_PHYSX_LOCK_READ(island.actor->getScene());
		if (island.actor->getNbShapes() == 0)
		{
			// This can happen if this island's chunks have all been destroyed
			continue;
		}
		PhysXObjectDescIntl* actorObjDesc = dscene->mModule->mSdk->getGenericPhysXObjectInfo(island.actor);
		ActorIslandData& data = islandData[island.actorIslandDataIndex];
		PX_ASSERT(data.islandCount > 0);
		const bool dynamic = !(island.actor->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC);
		if (!(island.flags & ChunkExternallySupported) || dynamic)
		{
			if (data.islandCount > 1)
			{
				--data.islandCount;
				createDynamicIsland(island.indices);
				actorObjDesc = dscene->mModule->mSdk->getGenericPhysXObjectInfo(island.actor);	// Need to get this again, since the pointer can change from createDynamicIsland()!
				if ((data.flags & ActorIslandData::SubmmittedForMassRecalc) == 0)
				{
					dirtyActors.pushBack(island.actor);
					data.flags |= ActorIslandData::SubmmittedForMassRecalc;
				}
				if (island.actorIslandDataIndex > 0 && island.actorIslandDataIndex <= (uint32_t)dscene->mActorFIFO.size())
				{
					dscene->mActorFIFO[island.actorIslandDataIndex-1].age = 0.0f;	// Reset the age of the original actor.  It will have a different set of chunks after this
				}
			}
			else
			{
				if (!dynamic)
				{
					createDynamicIsland(island.indices);
					actorObjDesc = dscene->mModule->mSdk->getGenericPhysXObjectInfo(island.actor);	// Need to get this again, since the pointer can change from createDynamicIsland()!
					// If we ever are able to simply clear the BF_KINEMATIC flag, remember to recalculate the mass properties here.
				}
				else
				{
					// Recalculate list of destructibles on this dynamic PxRigidDynamic island
					destructiblesInIsland.reset();
					for (uint32_t i = 0; i < actorObjDesc->mApexActors.size(); ++i)
					{
						const DestructibleActor* dActor = static_cast<const DestructibleActor*>(actorObjDesc->mApexActors[i]);
						((DestructibleActorProxy*)dActor)->impl.unreferencedByActor(island.actor);
					}
					actorObjDesc->mApexActors.reset();
					bool isDebris = true;	// Until proven otherwise
					for (uint32_t i = 0; i < island.indices.size(); ++i)
					{
						const uint32_t chunkIndex = island.indices[i];
						Chunk& chunk = chunks[chunkIndex];
						DestructibleActorImpl* destructible = dscene->mDestructibles.direct(chunk.destructibleID);
						const DestructibleParameters& parameters = destructible->getDestructibleParameters();
						DestructibleAssetParametersNS::Chunk_Type& assetChunk = destructible->getDestructibleAsset()->mParams->chunks.buf[chunk.indexInAsset];
						if (parameters.debrisDepth >= 0)
						{
							if (assetChunk.depth < (island.indices.size() == 1 ? parameters.debrisDepth : (parameters.debrisDepth + 1)))	// If there is more than one chunk in an island, insist that all chunks must lie strictly _below_ the debris depth to be considered debris
							{
								isDebris = false;
							}
						}
						else
						{
							isDebris = false;
						}
						if ((destructible->getInternalFlags() & DestructibleActorImpl::IslandMarker) == 0)
						{
							destructiblesInIsland.pushBack(destructible);
							PX_ASSERT(actorObjDesc->mApexActors.find(destructible->getAPI()) == actorObjDesc->mApexActors.end());
							actorObjDesc->mApexActors.pushBack(destructible->getAPI());
							destructible->referencedByActor(island.actor);
							destructible->getInternalFlags() |= DestructibleActorImpl::IslandMarker;
						}
					}
					PX_ASSERT(island.actorIslandDataIndex > 0);
					if (isDebris && island.actorIslandDataIndex > 0 && island.actorIslandDataIndex <= (uint32_t)dscene->mActorFIFO.size())
					{
						dscene->mActorFIFO[island.actorIslandDataIndex-1].flags |= ActorFIFOEntry::IsDebris;
					}
					for (uint32_t i = 0; i < actorObjDesc->mApexActors.size(); ++i)
					{
						destructiblesInIsland[i]->getInternalFlags() &= ~(uint16_t)DestructibleActorImpl::IslandMarker;
					}
				}
			}
		}

		if (actorObjDesc->userData == 0) // initially static chunk
		{
			// Recalculate list of destructibles on the kinematic PxRigidDynamic island
			for (uint32_t i = actorObjDesc->mApexActors.size(); i--;)
			{
				DestructibleActorProxy* proxy = const_cast<DestructibleActorProxy*>(static_cast<const DestructibleActorProxy*>(actorObjDesc->mApexActors[i]));
				if (proxy->impl.getStaticRoots().usedCount() == 0)
				{
					proxy->impl.unreferencedByActor(island.actor);
					actorObjDesc->mApexActors.replaceWithLast(i);
				}
			}
		}

		if (!dynamic)
		{
			if (actorObjDesc->mApexActors.size() == 0)
			{
				// if all destructible references have been removed
				if (actorForStaticChunks == island.actor)
				{
					actorForStaticChunks = NULL;
				}
			}
			else
			{
				dirtyActors.pushBack(island.actor);
			}
		}
	}

	for (uint32_t dirtyActorNum = 0; dirtyActorNum < dirtyActors.size(); ++dirtyActorNum)
	{
		PxRigidDynamic* actor = dirtyActors[dirtyActorNum];
		SCOPED_PHYSX_LOCK_READ(actor->getScene());

		const uint32_t nShapes = actor->getNbShapes();
		if (nShapes > 0)
		{
			float mass = 0.0f;
			for (uint32_t i = 0; i < nShapes; ++i)
			{
				PxShape* shape;
				actor->getShapes(&shape, 1, i);
				DestructibleStructure::Chunk* chunk = dscene->getChunk(shape);
				if (chunk == NULL || !chunk->isFirstShape(shape))	// BRG OPTIMIZE
				{
					continue;
				}
				DestructibleActorImpl* dactor = dscene->mDestructibles.direct(chunk->destructibleID);
				mass += dactor->getChunkMass(chunk->indexInAsset);
			}
			if (mass > 0.0f)
			{
				// Updating mass here, since it won't cause extra work for MOI calculator
				SCOPED_PHYSX_LOCK_WRITE(actor->getScene());
				physx::PxRigidBodyExt::setMassAndUpdateInertia(*actor, dscene->scaleMass(mass), NULL, true); // Ensures that we count shapes that have been flagged as non-simulation shapes
			}
		}
	}

	for (uint32_t supportChunkNum = 0; supportChunkNum < supportDepthChunks.size(); ++supportChunkNum)
	{
		chunks[supportDepthChunks[supportChunkNum]].state &= ~(uint32_t)ChunkTemp0;
	}

	setSupportInvalid(false);
}

void DestructibleStructure::createDynamicIsland(const physx::Array<uint32_t>& indices)
{
	PX_PROFILE_ZONE("DestructibleCreateDynamicIsland", GetInternalApexSDK()->getContextId());

	if (indices.size() == 0)
	{
		return;
	}

	PxRigidDynamic* actor = dscene->chunkIntact(chunks[indices[0]]);
	PX_ASSERT(actor != NULL);

	SCOPED_PHYSX_LOCK_READ(actor->getScene());
	SCOPED_PHYSX_LOCK_WRITE(actor->getScene());

	physx::Array<Chunk*> removedChunks;
	for (uint32_t i = 0; i < indices.size(); ++i)
	{
		Chunk& chunk = chunks[indices[i]];
		if (!chunk.isDestroyed())
		{
			Chunk* rootChunk = getRootChunk(chunk);
			if (rootChunk && (rootChunk->state & ChunkTemp1) == 0)
			{
				rootChunk->state |= (uint32_t)ChunkTemp1;
				removedChunks.pushBack(rootChunk);
				if(stressSolver && stressSolver->isPhysxBasedSim)
				{
					stressSolver->removeChunkFromIsland(indices[i]);
				}
				PxRigidDynamic* actor = getChunkActor(*rootChunk);
				if (!(actor->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC))
				{
					actor->wakeUp();
				}
			}
		}
		else
		{
			// This won't mark the found chunks with ChunkTemp1, but we should not
			// have any duplicates from this search
			DestructibleScene::CollectVisibleChunks chunkOp(removedChunks);
			dscene->forSubtree(chunk, chunkOp);
		}
	}
	if (removedChunks.size() == 0)
	{
		return;
	}

	// Eliminate chunks that are unfractureable.
	for (uint32_t i = 0; i < removedChunks.size(); ++i)
	{
		Chunk& chunk = *removedChunks[i];
		DestructibleActorImpl* destructible = dscene->mDestructibles.direct(chunk.destructibleID);
		destructible->getStaticRoots().free(chunk.indexInAsset);
		DestructibleAssetParametersNS::Chunk_Type& source = destructible->getDestructibleAsset()->mParams->chunks.buf[chunk.indexInAsset];
		if ((source.flags & DestructibleAssetImpl::DescendantUnfractureable) != 0)
		{
			chunk.state &= ~(uint32_t)ChunkTemp1;
			removedChunks[i] = NULL;
			for (uint32_t j = 0; j < source.numChildren; ++j)
			{
				const uint32_t childIndex = destructible->getFirstChunkIndex() + source.firstChildIndex + j;
				Chunk& child = chunks[childIndex];
				removedChunks.pushBack(&child);
				destructible->setChunkVisibility(child.indexInAsset, false);	// In case its mesh changes
				dscene->appendShapes(child, true);
				destructible->setChunkVisibility(child.indexInAsset, true);
			}
			removeChunk(chunk);
		}
		if ((source.flags & DestructibleAssetImpl::UnfracturableChunk) != 0)
		{
			if (actor->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC)
			{
				chunk.state &= ~(uint32_t)ChunkTemp1;
				removedChunks[i] = NULL;
			}
		}
	}

	uint32_t convexShapeCount = 0;
	uint32_t chunkCount = 0;
	for (uint32_t i = 0; i < removedChunks.size(); ++i)
	{
		Chunk* chunk = removedChunks[i];
		if (chunk == NULL)
		{
			continue;
		}
		DestructibleActorImpl* destructible = dscene->mDestructibles.direct(chunk->destructibleID);
		removedChunks[chunkCount++] = chunk;

		convexShapeCount += destructible->getDestructibleAsset()->getChunkHullCount(chunk->indexInAsset);
	}
	for (uint32_t i = chunkCount; i < removedChunks.size(); ++i)
	{
		removedChunks[i] = NULL;
	}

	PxRigidDynamic* newActor = NULL;

	physx::Array<DestructibleActorImpl*> destructiblesInIsland;

	float mass = 0.0f;

	uint32_t* convexShapeCounts = (uint32_t*)PxAlloca(sizeof(uint32_t) * chunkCount);
	PxConvexMeshGeometry* convexShapeDescs = (PxConvexMeshGeometry*)PxAlloca(sizeof(PxConvexMeshGeometry) * convexShapeCount);
	uint32_t createdShapeCount = 0;
	bool isDebris = true;	// until proven otherwise
	uint32_t minDepth = 0xFFFFFFFF;
	bool takesImpactDamage = false;
	float minContactReportThreshold = PX_MAX_F32;
	PhysX3DescTemplateImpl physX3Template;

	for (uint32_t i = 0; i < chunkCount; ++i)
	{
		Chunk* chunk = removedChunks[i];

		DestructibleActorImpl* destructible = dscene->mDestructibles.direct(chunk->destructibleID);
		destructible->getPhysX3Template(physX3Template);
		if ((destructible->getInternalFlags() & DestructibleActorImpl::IslandMarker) == 0)
		{
			destructiblesInIsland.pushBack(destructible);
			destructible->getInternalFlags() |= DestructibleActorImpl::IslandMarker;

			if (!newActor)
			{
				newActor = dscene->mPhysXScene->getPhysics().createRigidDynamic(actor->getGlobalPose());

				PX_ASSERT(newActor);
				physX3Template.apply(newActor);

				if (destructible->getDestructibleParameters().dynamicChunksDominanceGroup < 32)
				{
					newActor->setDominanceGroup(destructible->getDestructibleParameters().dynamicChunksDominanceGroup);
				}

				newActor->setActorFlag(PxActorFlag::eSEND_SLEEP_NOTIFIES, true);
				const DestructibleActorParamNS::BehaviorGroup_Type& behaviorGroup = destructible->getBehaviorGroup(chunk->indexInAsset);
				newActor->setMaxDepenetrationVelocity(behaviorGroup.maxDepenetrationVelocity);
			}
		}
		DestructibleParameters& parameters = destructible->getDestructibleParameters();
		DestructibleAssetParametersNS::Chunk_Type& assetChunk = destructible->getDestructibleAsset()->mParams->chunks.buf[chunk->indexInAsset];
		if (parameters.debrisDepth >= 0)
		{
			if (assetChunk.depth < (chunkCount == 1 ? parameters.debrisDepth : (parameters.debrisDepth + 1)))	// If there is more than one chunk in an island, insist that all chunks must lie strictly _below_ the debris depth to be considered debris
			{
				isDebris = false;
			}
		}
		else
		{
			isDebris = false;
		}
		if (assetChunk.depth < minDepth)
		{
			minDepth = assetChunk.depth;
		}
		if (destructible->takesImpactDamageAtDepth(assetChunk.depth))
		{
			takesImpactDamage = true;
			minContactReportThreshold = PxMin(minContactReportThreshold,
											  destructible->getContactReportThreshold(*chunk));
		}

		chunk->state &= ~(uint32_t)ChunkTemp1;
		mass += destructible->getChunkMass(chunk->indexInAsset);

		const uint32_t oldShapeCount = newActor->getNbShapes();
		for (uint32_t hullIndex = destructible->getDestructibleAsset()->getChunkHullIndexStart(chunk->indexInAsset); hullIndex < destructible->getDestructibleAsset()->getChunkHullIndexStop(chunk->indexInAsset); ++hullIndex)
		{
			PxConvexMeshGeometry& convexShapeDesc = convexShapeDescs[createdShapeCount];
			PX_PLACEMENT_NEW(&convexShapeDesc, PxConvexMeshGeometry);

			// Need to get PxConvexMesh
			// Shape(s):
			PxTransform	localPose	= getChunkLocalPose(*chunk);

			convexShapeDesc.convexMesh = destructible->getConvexMesh(hullIndex);
			// Make sure we can get a collision mesh
			if (!convexShapeDesc.convexMesh)
			{
				PX_ALWAYS_ASSERT();
				return;
			}
			convexShapeDesc.scale.scale = destructible->getScale();
			// Divide out the cooking scale to get the proper scaling
			const PxVec3 cookingScale = dscene->getModuleDestructible()->getChunkCollisionHullCookingScale();
			convexShapeDesc.scale.scale.x /= cookingScale.x;
			convexShapeDesc.scale.scale.y /= cookingScale.y;
			convexShapeDesc.scale.scale.z /= cookingScale.z;
			if ((convexShapeDesc.scale.scale - PxVec3(1.0f)).abs().maxElement() < 10*PX_EPS_F32)
			{
				convexShapeDesc.scale.scale = PxVec3(1.0f);
			}

			PxShape*	shape;

			shape = newActor->createShape(convexShapeDesc, 
				physX3Template.materials.begin(), 
				static_cast<uint16_t>(physX3Template.materials.size()),
				static_cast<physx::PxShapeFlags>(physX3Template.shapeFlags));
			shape->setLocalPose(localPose);
			physX3Template.apply(shape);

			physx::PxPairFlags	pairFlag	= (physx::PxPairFlags)physX3Template.contactReportFlags;
			if (takesImpactDamage)
			{
				pairFlag	/* |= PxPairFlag::eNOTIFY_CONTACT_FORCES */
							|=  PxPairFlag::eNOTIFY_THRESHOLD_FORCE_PERSISTS
							|  PxPairFlag::eNOTIFY_THRESHOLD_FORCE_FOUND
							|  PxPairFlag::eNOTIFY_CONTACT_POINTS;
			}

			if (destructible->getBehaviorGroup(chunk->indexInAsset).materialStrength > 0.0f)
			{
				pairFlag |= PxPairFlag::eMODIFY_CONTACTS;
			}

			if (dscene->mApexScene->getApexPhysX3Interface())
				dscene->mApexScene->getApexPhysX3Interface()->setContactReportFlags(shape, pairFlag, destructible->getAPI(), chunk->indexInAsset);


			++createdShapeCount;
		}
		dscene->scheduleChunkShapesForDelete(*chunk);
		convexShapeCounts[i] = newActor->getNbShapes() - oldShapeCount;
	}

	if (destructiblesInIsland.size() == 0)
	{
		return;
	}

	/*
		What if the different destructibles have different properties?  Which to choose for the island?  For now we'll just choose one.
	*/
	DestructibleActorImpl* templateDestructible = destructiblesInIsland[0];

	newActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
	if (takesImpactDamage)
	{
		// Set contact report threshold if the actor can take impact damage
		newActor->setContactReportThreshold(minContactReportThreshold);
	}
	PxRigidBodyExt::setMassAndUpdateInertia(*newActor, dscene->scaleMass(mass));

	{
		dscene->mActorsToAddIndexMap[newActor] = dscene->mActorsToAdd.size();
		dscene->mActorsToAdd.pushBack(newActor);

		float initWakeCounter = dscene->mPhysXScene->getWakeCounterResetValue();
		newActor->setWakeCounter(initWakeCounter);
	}

	// Use the templateDestructible to start the actorDesc, then add the other destructibles in the island
	PhysXObjectDescIntl* actorObjDesc = dscene->mModule->mSdk->createObjectDesc(templateDestructible->getAPI(), newActor);
	actorObjDesc->userData = 0;
	templateDestructible->setActorObjDescFlags(actorObjDesc, minDepth);
	templateDestructible->referencedByActor(newActor); // needs to be called after setActorObjDescFlags
	// While we're at it, clear the marker flags here
	templateDestructible->getInternalFlags() &= ~(uint16_t)DestructibleActorImpl::IslandMarker;
	for (uint32_t i = 1; i < destructiblesInIsland.size(); ++i)
	{
		DestructibleActorImpl* destructibleInIsland = destructiblesInIsland[i];
		PX_ASSERT(actorObjDesc->mApexActors.find(destructibleInIsland->getAPI()) == actorObjDesc->mApexActors.end());
		actorObjDesc->mApexActors.pushBack(destructibleInIsland->getAPI());
		destructibleInIsland->referencedByActor(newActor);
		destructibleInIsland->getInternalFlags() &= ~(uint16_t)DestructibleActorImpl::IslandMarker;
	}

	dscene->addActor(*actorObjDesc, *newActor, mass, isDebris);

	uint32_t newIslandID = newPxActorIslandReference(*removedChunks[0], *newActor);
	uint32_t shapeStart = 0;
	const uint32_t actorShapeCount = newActor->getNbShapes();
	PX_ALLOCA(shapeArray, physx::PxShape*, actorShapeCount);
	PxRigidActor* rigidActor = newActor->is<physx::PxRigidActor>();
	rigidActor->getShapes(shapeArray, actorShapeCount);
	for (uint32_t i = 0; i < chunkCount; ++i)
	{
		Chunk& chunk = *removedChunks[i];
		chunk.islandID = newIslandID;
		uint32_t shapeCount = convexShapeCounts[i];
		PxShape* const* newShapes = shapeArray + shapeStart;
		shapeStart += shapeCount;
		DestructibleActorImpl* destructible = dscene->mDestructibles.direct(chunk.destructibleID);
		PX_ASSERT(this == destructible->getStructure());
		destructible->setChunkVisibility(chunk.indexInAsset, false);
		chunk.state |= (uint32_t)ChunkVisible | (uint32_t)ChunkDynamic;
		destructible->setChunkVisibility(chunk.indexInAsset, true);
		chunk.setShapes(newShapes, shapeCount);
		DestructibleScene::VisibleChunkSetDescendents chunkOp(chunk.indexInAsset+destructible->getFirstChunkIndex(), true);
		dscene->forSubtree(chunk, chunkOp, true);
		for (uint32_t j = 0; j < shapeCount; ++j)
		{
			PhysXObjectDescIntl* shapeObjDesc = dscene->mModule->mSdk->createObjectDesc(destructible->getAPI(), newShapes[j]);
			shapeObjDesc->userData = &chunk;
		}
	}

	if (newActor && dscene->getModuleDestructible()->m_destructiblePhysXActorReport != NULL)
	{
		dscene->getModuleDestructible()->m_destructiblePhysXActorReport->onPhysXActorCreate(*newActor);
	}

	//===SyncParams===
	evaluateForHitChunkList(indices);
}


void DestructibleStructure::calculateExternalSupportChunks()
{
	const uint32_t chunkCount = chunks.size();
	PX_UNUSED(chunkCount);
	PX_PROFILE_ZONE("DestructibleStructureCalculateExternalSupportChunks", GetInternalApexSDK()->getContextId());

	supportDepthChunks.resize(0);
	supportDepthChunksNotExternallySupportedCount = 0;

	// iterate all chunks, one destructible after the other
	for (uint32_t i = 0; i < destructibles.size(); ++i)
	{
		DestructibleActorImpl* destructible = destructibles[i];
		if (destructible == NULL)
			continue;

		DestructibleAssetImpl* destructibleAsset = destructible->getDestructibleAsset();
		const float paddingFactor = destructibleAsset->mParams->neighborPadding;
		const float padding = paddingFactor * (destructible->getOriginalBounds().maximum - destructible->getOriginalBounds().minimum).magnitude();
		const DestructibleAssetParametersNS::Chunk_Type* destructibleAssetChunks = destructibleAsset->mParams->chunks.buf;

		const uint32_t supportDepth = destructible->getSupportDepth();
		const bool useAssetDefinedSupport = destructible->useAssetDefinedSupport();
		const bool useWorldSupport = destructible->useWorldSupport();

		const uint32_t firstChunkIndex = destructible->getFirstChunkIndex();
		const uint32_t destructibleChunkCount = destructible->getChunkCount();
		for (uint32_t chunkIndex = firstChunkIndex; chunkIndex < firstChunkIndex + destructibleChunkCount; ++chunkIndex)
		{
			Chunk& chunk = chunks[chunkIndex];
			const DestructibleAssetParametersNS::Chunk_Type& source = destructibleAssetChunks[chunk.indexInAsset];

			// init chunks flags (this is why all chunks need to be iterated)
			chunk.flags &= ~((uint8_t)ChunkBelowSupportDepth | (uint8_t)ChunkExternallySupported | (uint8_t)ChunkWorldSupported);

			if (source.depth > supportDepth)
			{
				chunk.flags |= ChunkBelowSupportDepth;
			}
			else if (source.depth == supportDepth)
			{	
				// keep list of all support depth chunks
				supportDepthChunks.pushBack(chunkIndex);

				if (!destructible->isInitiallyDynamic())
				{
					// Only static destructibles can be externally supported
					if (useAssetDefinedSupport)
					{
						if ((source.flags & DestructibleAssetImpl::SupportChunk) != 0)
						{
							chunk.flags |= ChunkExternallySupported;
						}
					}
					if (useWorldSupport)	// Test even if ChunkExternallySupported flag is already set, since we want to set the ChunkWorldSupported flag correctly
					{
						PhysX3DescTemplateImpl physX3Template;
						destructible->getPhysX3Template(physX3Template);
						for (uint32_t hullIndex = destructibleAsset->getChunkHullIndexStart(chunk.indexInAsset); hullIndex < destructibleAsset->getChunkHullIndexStop(chunk.indexInAsset); ++hullIndex)
						{
							PxTransform globalPose(destructible->getInitialGlobalPose());
							globalPose.p = globalPose.p + globalPose.rotate(destructible->getScale().multiply(destructibleAsset->getChunkPositionOffset(chunk.indexInAsset)));
							if (dscene->testWorldOverlap(destructibleAsset->chunkConvexHulls[hullIndex], globalPose, destructible->getScale(), padding, &physX3Template.queryFilterData))
							{
								chunk.flags |= ChunkExternallySupported | ChunkWorldSupported;
								break;
							}
						}
					}

					if (	((chunk.flags & ChunkExternallySupported) == 0) &&
						supportDepthChunks.size() > supportDepthChunksNotExternallySupportedCount + 1)
					{
						// Will sort the supportDepthChunks so that externally supported ones are all at the end of the array
						nvidia::swap(supportDepthChunks.back(), supportDepthChunks[supportDepthChunksNotExternallySupportedCount]);
						++supportDepthChunksNotExternallySupportedCount;
					}
				}
			}
		}
	}
}

void DestructibleStructure::invalidateBounds(const PxBounds3* bounds, uint32_t boundsCount)
{
	for (uint32_t chunkNum = supportDepthChunks.size(); chunkNum-- > supportDepthChunksNotExternallySupportedCount;)
	{
		const uint32_t chunkIndex = supportDepthChunks[chunkNum];
		Chunk& chunk = chunks[chunkIndex];
		if (chunk.state & ChunkDynamic)
		{
			continue;	// Only affect still-static chunks
		}
		DestructibleActorImpl* destructible = dscene->mDestructibles.direct(chunk.destructibleID);
		const float paddingFactor = destructible->getDestructibleAsset()->mParams->neighborPadding;
		const float padding = paddingFactor * (destructible->getOriginalBounds().maximum - destructible->getOriginalBounds().minimum).magnitude();
		const DestructibleAssetParametersNS::Chunk_Type& source = destructible->getDestructibleAsset()->mParams->chunks.buf[chunk.indexInAsset];
		if (destructible->useAssetDefinedSupport() && (source.flags & DestructibleAssetImpl::SupportChunk) != 0)
		{
			continue;	// Asset-defined support.  This is not affected by external actors.
		}
		PxBounds3 chunkBounds = destructible->getChunkBounds(chunk.indexInAsset);
		chunkBounds.fattenFast(2.0f*padding);
		bool affectedByInvalidBounds = false;
		for (uint32_t boundsNum = 0; boundsNum < boundsCount; ++boundsNum)
		{
			if (bounds[boundsNum].intersects(chunkBounds))
			{
				affectedByInvalidBounds = true;
				break;
			}
		}
		if (!affectedByInvalidBounds)
		{
			continue;
		}
		bool isWorldSupported = false;	// until proven otherwise
		PhysX3DescTemplateImpl physX3Template;
		destructible->getPhysX3Template(physX3Template);
		for (uint32_t hullIndex = destructible->getDestructibleAsset()->getChunkHullIndexStart(chunk.indexInAsset); hullIndex < destructible->getDestructibleAsset()->getChunkHullIndexStop(chunk.indexInAsset); ++hullIndex)
		{
			PxTransform globalPose(destructible->getInitialGlobalPose());
			globalPose.p = globalPose.p + globalPose.rotate(destructible->getScale().multiply(destructible->getDestructibleAsset()->getChunkPositionOffset(chunk.indexInAsset)));
			if (dscene->testWorldOverlap(destructible->getDestructibleAsset()->chunkConvexHulls[hullIndex], globalPose, destructible->getScale(), padding, &physX3Template.queryFilterData))
			{
				isWorldSupported = true;
				break;
			}
		}
		if (!isWorldSupported)
		{
			chunk.flags &= ~(uint8_t)ChunkExternallySupported;
			swap(supportDepthChunks[supportDepthChunksNotExternallySupportedCount++], supportDepthChunks[chunkNum++]);
			supportInvalid = true;
		}
	}
}

void DestructibleStructure::buildSupportGraph()
{
	PX_PROFILE_ZONE("DestructibleStructureBuildSupportGraph", GetInternalApexSDK()->getContextId());

	// First ensure external support data is up-to-date
	calculateExternalSupportChunks();

	// Now create graph

	if (destructibles.size() > 1)
	{
		// First record cached overlaps within each destructible
		physx::Array<IntPair> cachedOverlapPairs;
		for (uint32_t i = 0; i < destructibles.size(); ++i)
		{
			DestructibleActorImpl* destructible = destructibles[i];
			if (destructible != NULL)
			{
				DestructibleAssetImpl* asset = destructibles[i]->getDestructibleAsset();
				PX_ASSERT(asset);
				CachedOverlapsNS::IntPair_DynamicArray1D_Type* internalOverlaps = asset->getOverlapsAtDepth(destructible->getSupportDepth());
				cachedOverlapPairs.reserve(cachedOverlapPairs.size() + internalOverlaps->arraySizes[0]);
				for (int j = 0; j < internalOverlaps->arraySizes[0]; ++j)
				{
					CachedOverlapsNS::IntPair_Type& internalPair = internalOverlaps->buf[j];
					IntPair& pair = cachedOverlapPairs.insert();
					pair.i0 = internalPair.i0 + (int32_t)destructible->getFirstChunkIndex();
					pair.i1 = internalPair.i1 + (int32_t)destructible->getFirstChunkIndex();
				}
			}
		}

		physx::Array<IntPair> overlapPairs;
		uint32_t overlapCount = overlapPairs.size();

		// Now find overlaps between destructibles.  Start by creating a destructible overlap list
		physx::Array<BoundsRep> destructibleBoundsReps;
		destructibleBoundsReps.reserve(destructibles.size());
		for (uint32_t i = 0; i < destructibles.size(); ++i)
		{
			DestructibleActorImpl* destructible = destructibles[i];
			BoundsRep& boundsRep = destructibleBoundsReps.insert();
			if (destructible != NULL)
			{
				boundsRep.aabb = destructible->getOriginalBounds();
			}
			else
			{
				boundsRep.type = 1;	// Will not test 1-0 or 1-1 overlaps
			}
			const float paddingFactor = destructible->getDestructibleAsset()->mParams->neighborPadding;
			const float padding = paddingFactor * (destructible->getOriginalBounds().maximum - destructible->getOriginalBounds().minimum).magnitude();
			PX_ASSERT(!boundsRep.aabb.isEmpty());
			boundsRep.aabb.fattenFast(padding);
		}
		physx::Array<IntPair> destructiblePairs;
		{
			PX_PROFILE_ZONE("DestructibleStructureBoundsCalculateOverlaps", GetInternalApexSDK()->getContextId());
			BoundsInteractions interactions(false);
			interactions.set(0, 0, true);	// Only test 0-0 overlaps
			PX_ASSERT(destructibleBoundsReps.size() > 1);
			boundsCalculateOverlaps(destructiblePairs, Bounds3XYZ, &destructibleBoundsReps[0], destructibleBoundsReps.size(), sizeof(destructibleBoundsReps[0]), interactions);
		}

		// Test for chunk overlaps between overlapping destructibles
		for (uint32_t overlapIndex = 0; overlapIndex < destructiblePairs.size(); ++overlapIndex)
		{
			const IntPair& destructibleOverlap = destructiblePairs[overlapIndex];

			DestructibleActorImpl* destructible0 = destructibles[(uint32_t)destructibleOverlap.i0];
			const uint32_t startChunkAssetIndex0 = destructible0->getDestructibleAsset()->mParams->firstChunkAtDepth.buf[destructible0->getSupportDepth()];
			const uint32_t stopChunkAssetIndex0 = destructible0->getDestructibleAsset()->mParams->firstChunkAtDepth.buf[destructible0->getSupportDepth() + 1];
			const float paddingFactor0 = destructible0->getDestructibleAsset()->mParams->neighborPadding;
			const float padding0 = paddingFactor0 * (destructible0->getOriginalBounds().maximum - destructible0->getOriginalBounds().minimum).magnitude();

			DestructibleActorImpl* destructible1 = destructibles[(uint32_t)destructibleOverlap.i1];
			const uint32_t startChunkAssetIndex1 = destructible1->getDestructibleAsset()->mParams->firstChunkAtDepth.buf[destructible1->getSupportDepth()];
			const uint32_t stopChunkAssetIndex1 = destructible1->getDestructibleAsset()->mParams->firstChunkAtDepth.buf[destructible1->getSupportDepth() + 1];
			const float paddingFactor1 = destructible1->getDestructibleAsset()->mParams->neighborPadding;
			const float padding1 = paddingFactor1 * (destructible1->getOriginalBounds().maximum - destructible1->getOriginalBounds().minimum).magnitude();

			// Find AABB overlaps

			// chunkBoundsReps contains bounds of all support chunks for both destructibles (first part destructible0, second part destructible1)
			physx::Array<BoundsRep> chunkBoundsReps;
			chunkBoundsReps.reserve((stopChunkAssetIndex0 - startChunkAssetIndex0) + (stopChunkAssetIndex1 - startChunkAssetIndex1));

			// Destructible 0 bounds
			for (uint32_t chunkAssetIndex = startChunkAssetIndex0; chunkAssetIndex < stopChunkAssetIndex0; ++chunkAssetIndex)
			{
				BoundsRep& bounds = chunkBoundsReps.insert();
				PxMat44 scaledTM = destructible0->getInitialGlobalPose();
				scaledTM.scale(PxVec4(destructible0->getScale(), 1.f));
				bounds.aabb = destructible0->getDestructibleAsset()->getChunkActorLocalBounds(chunkAssetIndex);
				PxBounds3Transform(bounds.aabb, scaledTM);
				PX_ASSERT(!bounds.aabb.isEmpty());
				bounds.aabb.fattenFast(padding0);
				bounds.type = 0;
			}

			// Destructible 1 bounds
			for (uint32_t chunkAssetIndex = startChunkAssetIndex1; chunkAssetIndex < stopChunkAssetIndex1; ++chunkAssetIndex)
			{
				BoundsRep& bounds = chunkBoundsReps.insert();
				PxMat44 scaledTM = destructible1->getInitialGlobalPose();
				scaledTM.scale(PxVec4(destructible1->getScale(), 1.f));
				bounds.aabb = destructible1->getDestructibleAsset()->getChunkActorLocalBounds(chunkAssetIndex);
				PxBounds3Transform(bounds.aabb, scaledTM);
				PX_ASSERT(!bounds.aabb.isEmpty());
				bounds.aabb.fattenFast(padding1);
				bounds.type = 1;
			}

			{
				PX_PROFILE_ZONE("DestructibleStructureBoundsCalculateOverlaps", GetInternalApexSDK()->getContextId());
				BoundsInteractions interactions(false);
				interactions.set(0, 1, true);	// Only test 0-1 overlaps
				if (chunkBoundsReps.size() > 0)
				{
					// this adds overlaps into overlapPairs with indices into the chunkBoundsReps array
					boundsCalculateOverlaps(overlapPairs, Bounds3XYZ, &chunkBoundsReps[0], chunkBoundsReps.size(), sizeof(chunkBoundsReps[0]), interactions, true);
				}
			}

			// Now do detailed overlap test
			const bool performDetailedOverlapTest = destructible0->performDetailedOverlapTestForExtendedStructures() || destructible1->performDetailedOverlapTestForExtendedStructures();
			{
				const float padding = padding0 + padding1;

				// upperOffset transforms from chunkBoundsReps indices to asset chunk indices of the second destructible
				const int32_t upperOffset = int32_t(startChunkAssetIndex1 - (stopChunkAssetIndex0 - startChunkAssetIndex0));

				PX_PROFILE_ZONE("DestructibleStructureDetailedOverlapTest", GetInternalApexSDK()->getContextId());
				const uint32_t oldSize = overlapCount;
				for (uint32_t overlapIndex = oldSize; overlapIndex < overlapPairs.size(); ++overlapIndex)
				{
					IntPair& AABBOverlap = overlapPairs[overlapIndex];
					// The new overlaps will be indexed incorrectly.  Fix the indexing.
					if (AABBOverlap.i0 > AABBOverlap.i1)
					{
						// Ensures i0 corresponds to destructible0, i1 to destructible 1
						// because lower chunkBoundsReps indices belong to destructible0
						nvidia::swap(AABBOverlap.i0, AABBOverlap.i1);
					}

					// transform chunkBoundsReps indices to asset chunk indices
					AABBOverlap.i0 += startChunkAssetIndex0;
					AABBOverlap.i1 += upperOffset;
					if (!performDetailedOverlapTest || DestructibleAssetImpl::chunksInProximity(*destructible0->getDestructibleAsset(), (uint16_t)AABBOverlap.i0, 
						PxTransform(destructible0->getInitialGlobalPose()), destructible0->getScale(), *destructible1->getDestructibleAsset(), (uint16_t)AABBOverlap.i1, 
						PxTransform(destructible1->getInitialGlobalPose()), destructible1->getScale(), padding))
					{
						// Record overlap with global indexing
						IntPair& overlap = overlapPairs[overlapCount++];
						overlap.i0 = AABBOverlap.i0 + (int32_t)destructible0->getFirstChunkIndex();
						overlap.i1 = AABBOverlap.i1 + (int32_t)destructible1->getFirstChunkIndex();
					}
				}
			}
			overlapPairs.resize(overlapCount);
		}

		// We now have all chunk overlaps in the structure.
		// Make 'overlapPairs' symmetric - i.e. if (A,B) is in the array, then so is (B,A).
		overlapPairs.resize(2 * overlapCount);
		for (uint32_t i = 0; i < overlapCount; ++i)
		{
			IntPair& overlap = overlapPairs[i];
			IntPair& recipOverlap = overlapPairs[i + overlapCount];
			recipOverlap.i0 = overlap.i1;
			recipOverlap.i1 = overlap.i0;
		}
		overlapCount *= 2;

		// Now sort overlapPairs by index0 and index1 (symmetric, so it doesn't matter which we sort by first)
		qsort(overlapPairs.begin(), overlapCount, sizeof(IntPair), IntPair::compare);


		// merge new overlaps with cached ones
		physx::Array<IntPair> allOverlapPairs(overlapPairs.size() + cachedOverlapPairs.size());
		ApexMerge(overlapPairs.begin(), overlapPairs.size(), cachedOverlapPairs.begin(), cachedOverlapPairs.size(), allOverlapPairs.begin(), allOverlapPairs.size(), IntPair::compare);
		overlapCount = allOverlapPairs.size();

		// Create the structure's chunk overlap list
		createIndexStartLookup(firstOverlapIndices, 0, chunks.size(), &allOverlapPairs.begin()->i0, overlapCount, sizeof(IntPair));
		overlaps.resize(overlapCount);
		for (uint32_t overlapIndex = 0; overlapIndex < overlapCount; ++overlapIndex)
		{
			overlaps[overlapIndex] = (uint32_t)allOverlapPairs[overlapIndex].i1;
		}
	}
	else if (destructibles.size() == 1 && destructibles[0] != NULL)
	{
		// fast path for case of structures with only 1 destructible

		DestructibleAssetImpl* asset = destructibles[0]->getDestructibleAsset();
		CachedOverlapsNS::IntPair_DynamicArray1D_Type* internalOverlaps = asset->getOverlapsAtDepth(destructibles[0]->getSupportDepth());

		uint32_t numOverlaps = (uint32_t)internalOverlaps->arraySizes[0];

		// Create the structure's chunk overlap list
		createIndexStartLookup(firstOverlapIndices, 0, chunks.size(), &internalOverlaps->buf[0].i0, numOverlaps, sizeof(IntPair));

		overlaps.resize(numOverlaps);
		for (uint32_t overlapIndex = 0; overlapIndex < numOverlaps; ++overlapIndex)
		{
			overlaps[overlapIndex] = (uint32_t)internalOverlaps->buf[overlapIndex].i1;
		}
	}

	// embed this call within DestructibleStructure, rather than exposing the StressSolver to other classes
	postBuildSupportGraph();
}

void DestructibleStructure::postBuildSupportGraph()
{
	if(NULL != stressSolver)
	{
		PX_DELETE(stressSolver);
		stressSolver = NULL;
		dscene->setStressSolverTick(this, false);
	}

	{
		// instantiate a stress solver if the user wants to use it
		for(physx::Array<DestructibleActorImpl*>::ConstIterator kIter = destructibles.begin(); kIter != destructibles.end(); ++kIter)
		{
			const DestructibleActorImpl & currentDestructibleActor = *(*kIter);
			PX_ASSERT(NULL != &currentDestructibleActor);
			if(currentDestructibleActor.useStressSolver())
			{
				stressSolver = PX_NEW(DestructibleStructureStressSolver)(*this,currentDestructibleActor.getDestructibleParameters().supportStrength);
				if (!stressSolver->isPhysxBasedSim || stressSolver->isCustomEnablesSimulating)
				{
					dscene->setStressSolverTick(this, true);
				}
				break;
			}
		}
	}
}

void DestructibleStructure::evaluateForHitChunkList(const physx::Array<uint32_t> & chunkIndices) const
{
	PX_ASSERT(NULL != &chunkIndices);
	FractureEvent fractureEvent;
	::memset(static_cast<void *>(&fractureEvent), 0x00, 1 * sizeof(FractureEvent));
	for(physx::Array<uint32_t>::ConstIterator iter = chunkIndices.begin(); iter != chunkIndices.end(); ++iter)
	{
		DestructibleActorImpl & destructibleActor = *(dscene->mDestructibles.direct(chunks[*iter].destructibleID));
		PX_ASSERT(NULL != &destructibleActor);
		if(0 != destructibleActor.getSyncParams().getUserActorID())
		{
			fractureEvent.chunkIndexInAsset = *iter - destructibleActor.getFirstChunkIndex();
			PX_ASSERT(fractureEvent.chunkIndexInAsset < destructibleActor.getChunkCount());
			destructibleActor.evaluateForHitChunkList(fractureEvent);
		}
	}
}

void DestructibleStructure::addChunkImpluseForceAtPos(Chunk& chunk, const PxVec3& impulse, const PxVec3& position, bool wakeup)
{
	physx::Array<PxShape*>& shapes = getChunkShapes(chunk);
	PX_ASSERT(!shapes.empty());
	if (!shapes.empty())
	{
		// All shapes should have the same actor
		PxRigidActor* actor = shapes[0]->getActor();
		if (actor->getScene())
		{
			PxRigidDynamic* rigidDynamic = actor->is<physx::PxRigidDynamic>();
			if (rigidDynamic && !(rigidDynamic->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC))
			{
                PxRigidBodyExt::addForceAtPos(*actor->is<physx::PxRigidBody>(), impulse, position, PxForceMode::eIMPULSE, wakeup);
			}
		}
		else // the actor hasn't been added to the scene yet, so store the forces to apply later.
		{
			ActorForceAtPosition forceToAdd(impulse, position, PxForceMode::eIMPULSE, wakeup, true);
			dscene->addForceToAddActorsMap(actor, forceToAdd);
		}
	}
}

// http://en.wikipedia.org/wiki/Jenkins_hash_function
//   Fast and convenient hash for arbitrary multi-byte keys
uint32_t jenkinsHash(const void *data, size_t sizeInBytes)
{
	uint32_t hash, i;
	const char* key = reinterpret_cast<const char*>(data);
	for (hash = i = 0; i < sizeInBytes; ++i)
	{
		hash += key[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return hash;
}

uint32_t DestructibleStructure::newPxActorIslandReference(Chunk& chunk, PxRigidDynamic& nxActor)
{
	uint32_t islandID = jenkinsHash(&chunk, sizeof(Chunk));

	// Prevent hash collisions
	while (NULL != islandToActor.find(islandID))
	{
		++islandID;
	}

	PX_ASSERT(islandID != InvalidID);

	islandToActor[islandID] = &nxActor;
	actorToIsland[&nxActor] = islandID;

	dscene->setStructureUpdate(this, true);	// islandToActor needs to be cleared in DestructibleStructure::tick

	return islandID;
}

void DestructibleStructure::removePxActorIslandReferences(PxRigidDynamic& nxActor) const
{
	islandToActor.erase(actorToIsland[&nxActor]);
	actorToIsland.erase(&nxActor);
}

PxRigidDynamic* DestructibleStructure::getChunkActor(Chunk& chunk)
{
	physx::Array<PxShape*>& shapes = getChunkShapes(chunk);
	PxRigidDynamic* actor;
	SCOPED_PHYSX_LOCK_READ(dscene->mApexScene);
	actor = !shapes.empty() ? shapes[0]->getActor()->is<PxRigidDynamic>() : NULL;
	return actor;
}

const PxRigidDynamic* DestructibleStructure::getChunkActor(const Chunk& chunk) const
{
	const physx::Array<PxShape*>& shapes = getChunkShapes(chunk);
	PxRigidDynamic* actor;
	SCOPED_PHYSX_LOCK_READ(dscene->mApexScene);
	actor = !shapes.empty() ? shapes[0]->getActor()->is<PxRigidDynamic>() : NULL;
	return actor;
}

PxTransform DestructibleStructure::getChunkLocalPose(const Chunk& chunk) const
{
	const physx::Array<PxShape*>* shapes;
	PxVec3 offset;
	if (chunk.visibleAncestorIndex == (int32_t)InvalidChunkIndex)
	{
		shapes = &chunk.shapes;
		offset = PxVec3(0.0f);
	}
	else
	{
		shapes = &chunks[(uint32_t)chunk.visibleAncestorIndex].shapes;
		offset = chunk.localOffset - chunks[(uint32_t)chunk.visibleAncestorIndex].localOffset;
	}

	PxTransform pose(PxIdentity);
	if (!shapes->empty() && NULL != (*shapes)[0])
	{
		SCOPED_PHYSX_LOCK_READ(dscene->getModulePhysXScene());
		// All shapes should have the same global pose
		pose = (*shapes)[0]->getLocalPose();
	}
	pose.p += offset;
	return pose;
}

PxTransform DestructibleStructure::getChunkActorPose(const Chunk& chunk) const
{
	const physx::Array<PxShape*>& shapes = getChunkShapes(chunk);
	PxTransform pose(PxIdentity);
	if (!shapes.empty() && NULL != shapes[0])
	{
		// All shapes should have the same actor
		SCOPED_PHYSX_LOCK_READ(dscene->getModulePhysXScene());
		pose = shapes[0]->getActor()->getGlobalPose();
	}
	return pose;
}

void DestructibleStructure::setSupportInvalid(bool supportIsInvalid)
{
	supportInvalid = supportIsInvalid;
	dscene->setStructureUpdate(this, supportIsInvalid);
}

}
} // end namespace nvidia

