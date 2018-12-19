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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "SqSceneQueryManager.h"
#include "SqAABBPruner.h"
#include "SqBucketPruner.h"
#include "SqBounds.h"
#include "NpBatchQuery.h"
#include "PxFiltering.h"
#include "NpRigidDynamic.h"
#include "NpRigidStatic.h"
#include "NpArticulationLink.h"
#include "CmTransformUtils.h"
#include "PsAllocator.h"
#include "PxSceneDesc.h"
#include "ScBodyCore.h"
#include "SqPruner.h"
#include "GuBounds.h"
#include "NpShape.h"

using namespace physx;
using namespace Sq;
using namespace Sc;

namespace physx
{
	namespace Sq
	{
		OffsetTable gOffsetTable;
	}
}

PrunerExt::PrunerExt() :
	mPruner		(NULL),
	mDirtyList	(PX_DEBUG_EXP("SQmDirtyList")),
	mPrunerType	(PxPruningStructureType::eLAST),
	mTimestamp	(0xffffffff)
{
}

PrunerExt::~PrunerExt()
{
	PX_DELETE_AND_RESET(mPruner);
}

void PrunerExt::init(PxPruningStructureType::Enum type, PxU64 contextID)
{
	mPrunerType = type;
	mTimestamp	= 0;
	Pruner* pruner = NULL;
	switch(type)
	{
		case PxPruningStructureType::eNONE:					{ pruner = PX_NEW(BucketPruner);					break;	}
		case PxPruningStructureType::eDYNAMIC_AABB_TREE:	{ pruner = PX_NEW(AABBPruner)(true, contextID);		break;	}
		case PxPruningStructureType::eSTATIC_AABB_TREE:		{ pruner = PX_NEW(AABBPruner)(false, contextID);	break;	}
		case PxPruningStructureType::eLAST:					break;
	}
	mPruner = pruner;
}

void PrunerExt::preallocate(PxU32 nbShapes)
{
	if(nbShapes > mDirtyMap.size())
		mDirtyMap.resize(nbShapes);

	if(mPruner)
		mPruner->preallocate(nbShapes);
}

void PrunerExt::flushMemory()
{
	if(!mDirtyList.size())
		mDirtyList.reset();

	// PT: TODO: flush bitmap here

	// PT: TODO: flush pruner here?
}

void PrunerExt::flushShapes(PxU32 index)
{
	const PxU32 numDirtyList = mDirtyList.size();
	if(!numDirtyList)
		return;
	const PrunerHandle* const prunerHandles = mDirtyList.begin();

	const ComputeBoundsFunc func = gComputeBoundsTable[index];

	for(PxU32 i=0; i<numDirtyList; i++)
	{
		const PrunerHandle handle = prunerHandles[i];
		mDirtyMap.reset(handle);

		// PT: we compute the new bounds and store them directly in the pruner structure to avoid copies. We delay the updateObjects() call
		// to take advantage of batching.
		PxBounds3* bounds;
		const PrunerPayload& pp = mPruner->getPayload(handle, bounds);
		(func)(*bounds, *(reinterpret_cast<Scb::Shape*>(pp.data[0])), *(reinterpret_cast<Scb::Actor*>(pp.data[1])));
	}
	// PT: batch update happens after the loop instead of once per loop iteration
	mPruner->updateObjectsAfterManualBoundsUpdates(prunerHandles, numDirtyList);
	mTimestamp += numDirtyList;
	mDirtyList.clear();
}

// PT: TODO: re-inline this
void PrunerExt::addToDirtyList(PrunerHandle handle)
{
	Cm::BitMap& dirtyMap = mDirtyMap;
	if(!dirtyMap.test(handle))
	{
		dirtyMap.set(handle);
		mDirtyList.pushBack(handle);
		mTimestamp++;
	}
}

// PT: TODO: re-inline this
Ps::IntBool PrunerExt::isDirty(PrunerHandle handle) const
{
	return mDirtyMap.test(handle);
}

// PT: TODO: re-inline this
void PrunerExt::removeFromDirtyList(PrunerHandle handle)
{
	Cm::BitMap& dirtyMap = mDirtyMap;
	if(dirtyMap.test(handle))
	{
		dirtyMap.reset(handle);
		mDirtyList.findAndReplaceWithLast(handle);
	}
}

// PT: TODO: re-inline this
void PrunerExt::growDirtyList(PrunerHandle handle)
{
	// pruners must either provide indices in order or reuse existing indices, so this 'if' is enough to ensure we have space for the new handle
	// PT: TODO: fix this. There is just no need for any of it. The pruning pool itself could support the feature for free, similar to what we do
	// in MBP. There would be no need for the bitmap or the dirty list array. However doing this through the virtual interface would be clumsy,
	// adding the cost of virtual calls for very cheap & simple operations. It would be a lot easier to drop it and go back to what we had before.

	Cm::BitMap& dirtyMap = mDirtyMap;
	if(dirtyMap.size() <= handle)
		dirtyMap.resize(PxMax<PxU32>(dirtyMap.size() * 2, 1024));
	PX_ASSERT(handle<dirtyMap.size());
	dirtyMap.reset(handle);
}

///////////////////////////////////////////////////////////////////////////////

SceneQueryManager::SceneQueryManager(	Scb::Scene& scene, PxPruningStructureType::Enum staticStructure, 
										PxPruningStructureType::Enum dynamicStructure, PxU32 dynamicTreeRebuildRateHint,
										const PxSceneLimits& limits) :
	mScene		(scene)
{
	mPrunerExt[PruningIndex::eSTATIC].init(staticStructure, scene.getContextId());
	mPrunerExt[PruningIndex::eDYNAMIC].init(dynamicStructure, scene.getContextId());

	setDynamicTreeRebuildRateHint(dynamicTreeRebuildRateHint);

	preallocate(limits.maxNbStaticShapes, limits.maxNbDynamicShapes);

	mDynamicBoundsSync.mPruner = mPrunerExt[PruningIndex::eDYNAMIC].pruner();
	mDynamicBoundsSync.mTimestamp = &mPrunerExt[PruningIndex::eDYNAMIC].mTimestamp;

	mPrunerNeedsUpdating = false;
}

SceneQueryManager::~SceneQueryManager()
{
}

void SceneQueryManager::flushMemory()
{
	for(PxU32 i=0;i<PruningIndex::eCOUNT;i++)
		mPrunerExt[i].flushMemory();
}

void SceneQueryManager::markForUpdate(PrunerData data)
{ 
	mPrunerNeedsUpdating = true;
	const PxU32 index = getPrunerIndex(data);
	const PrunerHandle handle = getPrunerHandle(data);

	mPrunerExt[index].addToDirtyList(handle);
}

void SceneQueryManager::preallocate(PxU32 staticShapes, PxU32 dynamicShapes)
{
	mPrunerExt[PruningIndex::eSTATIC].preallocate(staticShapes);
	mPrunerExt[PruningIndex::eDYNAMIC].preallocate(dynamicShapes);
}

PrunerData SceneQueryManager::addPrunerShape(const NpShape& shape, const PxRigidActor& actor, bool dynamic, const PxBounds3* bounds, bool hasPrunerStructure)
{
	mPrunerNeedsUpdating = true;

	PrunerPayload pp;
	const Scb::Shape& scbShape = shape.getScbShape();
	const Scb::Actor& scbActor = gOffsetTable.convertPxActor2Scb(actor);
	pp.data[0] = size_t(&scbShape);
	pp.data[1] = size_t(&scbActor);

	PxBounds3 b;
	if(bounds)
		inflateBounds(b, *bounds);
	else
		(gComputeBoundsTable[dynamic])(b, scbShape, scbActor);

	const PxU32 index = PxU32(dynamic);
	PrunerHandle handle;
	PX_ASSERT(mPrunerExt[index].pruner());
	mPrunerExt[index].pruner()->addObjects(&handle, &b, &pp, 1, hasPrunerStructure);
	mPrunerExt[index].invalidateTimestamp();

	mPrunerExt[index].growDirtyList(handle);

	return createPrunerData(index, handle);
}

const PrunerPayload& SceneQueryManager::getPayload(PrunerData data) const
{
	const PxU32 index = getPrunerIndex(data);
	const PrunerHandle handle = getPrunerHandle(data);
	return mPrunerExt[index].pruner()->getPayload(handle);
}

void SceneQueryManager::removePrunerShape(PrunerData data)
{
	mPrunerNeedsUpdating = true;
	const PxU32 index = getPrunerIndex(data);
	const PrunerHandle handle = getPrunerHandle(data);

	PX_ASSERT(mPrunerExt[index].pruner());

	mPrunerExt[index].removeFromDirtyList(handle);

	mPrunerExt[index].invalidateTimestamp();
	mPrunerExt[index].pruner()->removeObjects(&handle, 1);
}

void SceneQueryManager::setDynamicTreeRebuildRateHint(PxU32 rebuildRateHint)
{
	mRebuildRateHint = rebuildRateHint;

	for(PxU32 i=0;i<PruningIndex::eCOUNT;i++)
	{
		if(mPrunerExt[i].pruner() && mPrunerExt[i].type() == PxPruningStructureType::eDYNAMIC_AABB_TREE)
			static_cast<AABBPruner*>(mPrunerExt[i].pruner())->setRebuildRateHint(rebuildRateHint);
	}
}


static PxBounds3 computeWorldAABB(const Scb::Shape& scbShape, const Sc::BodyCore& bodyCore)
{
	const Gu::GeometryUnion& geom = scbShape.getGeometryUnion();
	const PxTransform& shape2Actor = scbShape.getShape2Actor();

	PX_ALIGN(16, PxTransform) globalPose;

	PX_ALIGN(16, PxTransform) kinematicTarget;
	PxU16 sqktFlags = PxRigidBodyFlag::eKINEMATIC | PxRigidBodyFlag::eUSE_KINEMATIC_TARGET_FOR_SCENE_QUERIES;
	bool useTarget = (PxU16(bodyCore.getFlags()) & sqktFlags) == sqktFlags;

	const PxTransform& body2World = (useTarget && bodyCore.getKinematicTarget(kinematicTarget)) ? kinematicTarget : bodyCore.getBody2World();
	Cm::getDynamicGlobalPoseAligned(body2World, shape2Actor, bodyCore.getBody2Actor(), globalPose);

	PxBounds3 tmp;
	inflateBounds(tmp, Gu::computeBounds(geom.getGeometry(), globalPose, false));
	return tmp;
}


void SceneQueryManager::validateSimUpdates()
{
	if (mPrunerExt[1].type() != PxPruningStructureType::eDYNAMIC_AABB_TREE)
		return;

	
	Sc::BodyCore*const* activeBodies = mScene.getActiveBodiesArray();
	const PxU32 nbActiveBodies = mScene.getNumActiveBodies();

	for (PxU32 i = 0; i < nbActiveBodies; ++i)
	{
		const Sc::BodyCore* bCore = activeBodies[i];

		if (bCore->isFrozen())
			continue;

		PxRigidBody* pxBody = static_cast<PxRigidBody*>(bCore->getPxActor());

		PX_ASSERT(pxBody->getConcreteType() == PxConcreteType::eRIGID_DYNAMIC || pxBody->getConcreteType() == PxConcreteType::eARTICULATION_LINK);

		NpShapeManager& shapeManager = *NpActor::getShapeManager(*pxBody);
		const PxU32 nbShapes = shapeManager.getNbShapes();
		NpShape* const* shape = shapeManager.getShapes();
		

		for (PxU32 j = 0; j<nbShapes; j++)
		{
			PrunerData prunerData = shapeManager.getPrunerData(j);
			if (prunerData != INVALID_PRUNERHANDLE)
			{
				const PrunerHandle handle = getPrunerHandle(prunerData);
				const PxBounds3 worldAABB = computeWorldAABB(shape[j]->getScbShape(), *bCore);
				PxBounds3 prunerAABB = static_cast<AABBPruner*>(mPrunerExt[1].pruner())->getAABB(handle);
				PX_ASSERT((worldAABB.minimum - prunerAABB.minimum).magnitudeSquared() < 0.005f*mScene.getPxScene()->getPhysics().getTolerancesScale().length);
				PX_ASSERT((worldAABB.maximum - prunerAABB.maximum).magnitudeSquared() < 0.005f*mScene.getPxScene()->getPhysics().getTolerancesScale().length);
				PX_UNUSED(worldAABB);
				PX_UNUSED(prunerAABB);
			}
		}
	}
}

void SceneQueryManager::afterSync(PxSceneQueryUpdateMode::Enum updateMode)
{
	PX_PROFILE_ZONE("Sim.sceneQueryBuildStep", mScene.getContextId());

	if(updateMode == PxSceneQueryUpdateMode::eBUILD_DISABLED_COMMIT_DISABLED)
	{
		mPrunerNeedsUpdating = true;
		return;
	}

	// flush user modified objects
	flushShapes();

	bool commit = updateMode == PxSceneQueryUpdateMode::eBUILD_ENABLED_COMMIT_ENABLED;

	for(PxU32 i = 0; i<2; i++)
	{
		if(mPrunerExt[i].pruner() && mPrunerExt[i].type() == PxPruningStructureType::eDYNAMIC_AABB_TREE)
			static_cast<AABBPruner*>(mPrunerExt[i].pruner())->buildStep(true);

		if(commit)
			mPrunerExt[i].pruner()->commit();
	}

	//If we didn't commit changes, then the next query must perform that operation so this bool must be set to true, otherwise there should be 
	//no outstanding work required by queries at this time
	mPrunerNeedsUpdating = !commit;
}

void SceneQueryManager::flushShapes()
{
	PX_PROFILE_ZONE("SceneQuery.flushShapes", mScene.getContextId());

	// must already have acquired writer lock here

	for(PxU32 i=0; i<PruningIndex::eCOUNT; i++)
		mPrunerExt[i].flushShapes(i);
}

void SceneQueryManager::flushUpdates()
{
	PX_PROFILE_ZONE("SceneQuery.flushUpdates", mScene.getContextId());

	if (mPrunerNeedsUpdating)
	{
	// no need to take lock if manual sq update is enabled
	// as flushUpdates will only be called from NpScene::flushQueryUpdates()
		mSceneQueryLock.lock();

		//Check again because another thread may have already performed all pending updated (this secondary check might be unnecessary)
		if (mPrunerNeedsUpdating)
		{

			flushShapes();

			for (PxU32 i = 0; i < PruningIndex::eCOUNT; i++)
				if (mPrunerExt[i].pruner())
					mPrunerExt[i].pruner()->commit();

			//KS - force memory writes to have completed before updating the volatile mPrunerNeedsUpdating member. This should ensure that, if another thread
			//reads this value and finds it is false, that all the modifications we made to the pruner are visible in memory.
			physx::shdfnd::memoryBarrier();

			mPrunerNeedsUpdating = false;
		}
		mSceneQueryLock.unlock();
	}
}

void SceneQueryManager::forceDynamicTreeRebuild(bool rebuildStaticStructure, bool rebuildDynamicStructure)
{
	PX_PROFILE_ZONE("SceneQuery.forceDynamicTreeRebuild", mScene.getContextId());

	const bool rebuild[PruningIndex::eCOUNT] = { rebuildStaticStructure, rebuildDynamicStructure };

	Ps::Mutex::ScopedLock lock(mSceneQueryLock);
	for(PxU32 i=0; i<PruningIndex::eCOUNT; i++)
	{
		if(rebuild[i] && mPrunerExt[i].pruner() && mPrunerExt[i].type() == PxPruningStructureType::eDYNAMIC_AABB_TREE)
		{
			static_cast<AABBPruner*>(mPrunerExt[i].pruner())->purge();
			static_cast<AABBPruner*>(mPrunerExt[i].pruner())->commit();
		}
	}
}

void SceneQueryManager::sceneQueryBuildStep(PruningIndex::Enum index)
{
	PX_PROFILE_ZONE("SceneQuery.sceneQueryBuildStep", mScene.getContextId());

	if (mPrunerExt[index].pruner() && mPrunerExt[index].type() == PxPruningStructureType::eDYNAMIC_AABB_TREE)
	{
		const bool buildFinished = static_cast<AABBPruner*>(mPrunerExt[index].pruner())->buildStep(false);
		if(buildFinished)
		{
			mPrunerNeedsUpdating = true;
		}
	}
}

bool SceneQueryManager::prepareSceneQueriesUpdate(PruningIndex::Enum index)
{
	bool retVal = false;
	if (mPrunerExt[index].pruner() && mPrunerExt[index].type() == PxPruningStructureType::eDYNAMIC_AABB_TREE)
	{
		retVal = static_cast<AABBPruner*>(mPrunerExt[index].pruner())->prepareBuild();
	}
	return retVal;
}

void SceneQueryManager::shiftOrigin(const PxVec3& shift)
{
	for(PxU32 i=0; i<PruningIndex::eCOUNT; i++)
		mPrunerExt[i].pruner()->shiftOrigin(shift);
}

void DynamicBoundsSync::sync(const PrunerHandle* handles, const PxU32* indices, const PxBounds3* bounds, PxU32 count, const Cm::BitMap& dirtyShapeSimMap)
{
	if(!count)
		return;

	PxU32 startIndex = 0;
	PxU32 numIndices = count;

	// if shape sim map is not empty, parse the indices and skip update for the dirty one
	if(dirtyShapeSimMap.count())
	{
		numIndices = 0;

		for(PxU32 i=0; i<count; i++)
		{
			if(dirtyShapeSimMap.test(indices[i]))
			{
				mPruner->updateObjectsAndInflateBounds(handles + startIndex, indices + startIndex, bounds, numIndices);
				numIndices = 0;
				startIndex = i + 1;
			}
			else
				numIndices++;
		}
		// PT: we fallback to the next line on purpose - no "else"
	}

	mPruner->updateObjectsAndInflateBounds(handles + startIndex, indices + startIndex, bounds, numIndices);

	(*mTimestamp)++;
}

