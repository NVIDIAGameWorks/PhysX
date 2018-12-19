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

#include "NpCast.h"
#include "NpVolumeCache.h"
#include "SqSceneQueryManager.h"
#include "GuBounds.h"
#include "PxRigidActor.h"
#include "GuRaycastTests.h"
#include "PxGeometryQuery.h"
#include "GuIntersectionRayBox.h"
#include "NpQueryShared.h"
#include "NpSceneQueries.h"
#include "PsFoundation.h"

namespace physx {

using namespace Sq;
using namespace Gu;

static PX_FORCE_INLINE NpScene* getNpScene(SceneQueryManager* sqm)
{
	return const_cast<NpScene*>(getNpScene(&sqm->getScene()));
}

//========================================================================================================================
NpVolumeCache::NpVolumeCache(Sq::SceneQueryManager* sqm, PxU32 maxStaticShapes, PxU32 maxDynamicShapes)
	: mSQManager(sqm)
{
	mCacheVolume.any() = InvalidGeometry();
	mMaxShapeCount[0] = maxStaticShapes;
	mMaxShapeCount[1] = maxDynamicShapes;
	mIsInvalid[0] = mIsInvalid[1] = true;
	mCache[0].reserve(maxStaticShapes);
	mCache[1].reserve(maxDynamicShapes);
}

//========================================================================================================================
NpVolumeCache::~NpVolumeCache()
{
}

//========================================================================================================================
void NpVolumeCache::invalidate()
{
	mCacheVolume.any() = InvalidGeometry();
	mCache[0].clear();
	mCache[1].clear();
	mIsInvalid[0] = mIsInvalid[1] = true;
}

//========================================================================================================================
void NpVolumeCache::release()
{
	getNpScene(mSQManager)->releaseVolumeCache(this);
}

//========================================================================================================================
PxI32 NpVolumeCache::getNbCachedShapes()
{
	if(!isValid())
		return -1;
	return PxI32(mCache[0].size()+mCache[1].size());
}

//========================================================================================================================
bool NpVolumeCache::isValid() const
{
	if(mIsInvalid[0] || mIsInvalid[1])
		return false;

	return mSQManager->get(PruningIndex::eSTATIC).timestamp() == mStaticTimestamp && mSQManager->get(PruningIndex::eDYNAMIC).timestamp() == mDynamicTimestamp;
}

//========================================================================================================================
bool NpVolumeCache::isValid(PxU32 isDynamic) const
{
	if(mIsInvalid[isDynamic])
		return false;

	return isDynamic ?
		(mSQManager->get(PruningIndex::eDYNAMIC).timestamp() == mDynamicTimestamp) : (mSQManager->get(PruningIndex::eSTATIC).timestamp() == mStaticTimestamp);
}

//========================================================================================================================
// PT: TODO: replace this with just holder.storeAny() when/if we can support convexes
static PX_INLINE bool geometryToHolder(const PxGeometry& geometry, PxGeometryHolder& holder)
{
	holder.any() = geometry; // this just copies the type, so that later holder.box() etc don't assert. Awkward but works.
	switch ( geometry.getType() )
	{
	case PxGeometryType::eBOX:
		holder.box() = static_cast<const PxBoxGeometry&>( geometry );
		break;
	case PxGeometryType::eSPHERE:
		holder.sphere() = static_cast<const PxSphereGeometry&>( geometry );
		break;
	case PxGeometryType::eCAPSULE:
		holder.capsule() = static_cast<const PxCapsuleGeometry&>( geometry );
		break;
	// convexes are not supported for now. The rationale is unknown utility
	// and we need to correctly handle refcounts on a convex mesh (and write unit tests for that)
	//case PxGeometryType::eCONVEXMESH:
	//	holder.convexMesh() = static_cast<const PxConvexMeshGeometry&>( geometry );
	//	break;
	case PxGeometryType::ePLANE:
	case PxGeometryType::eCONVEXMESH:
	case PxGeometryType::eTRIANGLEMESH:
	case PxGeometryType::eHEIGHTFIELD:
	case PxGeometryType::eGEOMETRY_COUNT:
	case PxGeometryType::eINVALID:
		holder.any() = InvalidGeometry();
		return false;
	}

	return true;
}

//========================================================================================================================
PxVolumeCache::FillStatus NpVolumeCache::fill(const PxGeometry& cacheVolume, const PxTransform& pose)
{
	PX_CHECK_AND_RETURN_VAL(pose.isSane(), "Invalid pose in PxVolumeCache::fill()", FILL_UNSUPPORTED_GEOMETRY_TYPE);

	// save the cache volume pose into a cache pose
	mCachePose = pose;

	// save the provided geometry arg in mCacheVolume
	// try to convert geometry to holder, if fails mark the cache as invalid and notify the user about unsupported geometry type
	if(!geometryToHolder(cacheVolume, mCacheVolume))
	{
		physx::shdfnd::getFoundation().error(physx::PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__,
			"PxVolumeCache::fill(): unsupported cache volume geometry type.");
		mIsInvalid[0] = mIsInvalid[1] = true;
		return FILL_UNSUPPORTED_GEOMETRY_TYPE;
	}

	mIsInvalid[0] = mIsInvalid[1] = true; // invalidate the cache due to volume change

	FillStatus status0 = fillInternal(0);
	FillStatus status1 = fillInternal(1);

	PX_COMPILE_TIME_ASSERT(FILL_OK < FILL_OVER_MAX_COUNT && FILL_OVER_MAX_COUNT < FILL_UNSUPPORTED_GEOMETRY_TYPE);
	PX_COMPILE_TIME_ASSERT(FILL_UNSUPPORTED_GEOMETRY_TYPE < FILL_OUT_OF_MEMORY);
	return PxMax(status0, status1);
}

//========================================================================================================================
PxVolumeCache::FillStatus NpVolumeCache::fillInternal(PxU32 isDynamic, const PxOverlapHit* prefilledBuffer, PxI32 prefilledCount)
{
	PX_ASSERT(isDynamic == 0 || isDynamic == 1);
	PX_CHECK_AND_RETURN_VAL(mCachePose.isValid(), "PxVolumeCache::fillInternal: pose is not valid.", FILL_UNSUPPORTED_GEOMETRY_TYPE);

	// allocate a buffer for temp results from SQ overlap call (or use prefilledBuffer)
	const PxU32 maxStackHits = 64;
	PxOverlapHit* hitBuffer = const_cast<PxOverlapHit*>(prefilledBuffer);
	bool hitBufferAlloca = false;
	if(prefilledBuffer == NULL)
	{
		if(mMaxShapeCount[isDynamic]+1 <= maxStackHits) // try allocating on the stack first if we can
		{
			hitBuffer = reinterpret_cast<PxOverlapHit*>(PxAlloca((mMaxShapeCount[isDynamic]+1) * sizeof(PxOverlapHit)));
			hitBufferAlloca = true;
		}
		else
		{
			hitBuffer = reinterpret_cast<PxOverlapHit*>(physx::shdfnd::TempAllocator().allocate(
				sizeof(PxOverlapHit) * (mMaxShapeCount[isDynamic]+1), __FILE__, __LINE__));
			if(hitBuffer == NULL)
			{
				mIsInvalid[isDynamic] = true;
				physx::shdfnd::getFoundation().error(physx::PxErrorCode::eOUT_OF_MEMORY, __FILE__, __LINE__,
					"PxVolumeCache::fill(): Fallback memory allocation failed, mMaxShapeCount = %d. Try reducing the cache size.",
					mMaxShapeCount[isDynamic]);
				return FILL_OUT_OF_MEMORY;
			}
		}
	}

	// clear current cache
	mCache[isDynamic].resize(0);

	// query the scene
	PxI32 resultCount = prefilledCount;
	PxQueryFilterData fd(isDynamic ? PxQueryFlag::eDYNAMIC : PxQueryFlag::eSTATIC);
	PxOverlapBuffer sqBuffer(hitBuffer, mMaxShapeCount[isDynamic]+1); // one extra element so we can detect overflow in a single callback
	if(!prefilledBuffer)
	{
		getNpScene(mSQManager)->overlap(mCacheVolume.any(), mCachePose, sqBuffer, fd, NULL);
		resultCount = PxI32(sqBuffer.getNbAnyHits());
	}

	if(resultCount > PxI32(mMaxShapeCount[isDynamic]))
	{
		// cache overflow - deallocate the temp buffer
		if(!hitBufferAlloca && hitBuffer != prefilledBuffer)
			physx::shdfnd::TempAllocator().deallocate(hitBuffer);
		mIsInvalid[isDynamic] = true;
		return FILL_OVER_MAX_COUNT;
	}

	// fill the cache
	PX_ASSERT(resultCount <= PxI32(mMaxShapeCount[isDynamic]));
	for (PxI32 iHit = 0; iHit < resultCount; iHit++)
	{
		PxActorShape as;
		as.actor = hitBuffer[iHit].actor;
		as.shape = hitBuffer[iHit].shape;
		PX_ASSERT(as.actor && as.shape);
		mCache[isDynamic].pushBack(as);
	}

	// timestamp the cache
	if(isDynamic)
		mDynamicTimestamp = mSQManager->get(PruningIndex::eDYNAMIC).timestamp();
	else
		mStaticTimestamp = mSQManager->get(PruningIndex::eSTATIC).timestamp();

	// clear the invalid flag
	mIsInvalid[isDynamic] = false;

	if(!hitBufferAlloca && hitBuffer != prefilledBuffer)
		physx::shdfnd::TempAllocator().deallocate(hitBuffer);

	return FILL_OK;
}

//========================================================================================================================
bool NpVolumeCache::getCacheVolume(PxGeometryHolder& resultVolume, PxTransform& resultPose)
{
	resultVolume = mCacheVolume;
	resultPose = mCachePose;

	return isValid();
}

//========================================================================================================================
struct NpVolumeCacheSqCallback : PxOverlapCallback
{
	NpVolumeCache*				cache;
	NpVolumeCache::Iterator&	iter;
	PxU32						isDynamic;
	PxActorShape*				reportBuf;
	bool						reportedOverMaxCount;

	NpVolumeCacheSqCallback(
		NpVolumeCache* cache_, NpVolumeCache::Iterator& iter_,
		PxU32 isDynamic_, PxOverlapHit* hits, PxActorShape* reportBuf_, PxU32 maxHits)
			: PxOverlapCallback(hits, maxHits), cache(cache_), iter(iter_), isDynamic(isDynamic_),
			reportBuf(reportBuf_), reportedOverMaxCount(false)
	{}

	virtual PxAgain processTouches(const PxOverlapHit* buffer, PxU32 nbHits)
	{
		// at this point we know that the callback's buffer capacity exceeds the cache max shape count by 1
		// so if nbHits is within shape count it means there will be no more callbacks.

		if(!reportedOverMaxCount && nbHits <= cache->mMaxShapeCount[isDynamic])
		{
			// if actual overlapCount is under cache capacity, fill the cache
			if(const_cast<NpVolumeCache*>(cache)->fillInternal(isDynamic, buffer, PxI32(nbHits)) != PxVolumeCache::FILL_OK)
				// we shouldn't really end up here because we already checked all possible bad conditions fill() could return
				// so the only way to end up here is if this is an error condition that should already be logged from inside fillInternal
				return false;
			// At this point the cache should be valid and within the user specified capacity
			PX_ASSERT(cache->isValid(isDynamic));
			// break out of the loop, this will fall through to return the results from cache via iterator

			return false;
		} else
		{
			// reroute to the user
			// copy into a temp buffer for iterator reporting
			for (PxU32 j = 0; j < PxU32(nbHits); j++)
			{
				reportBuf[j].actor = buffer[j].actor;
				reportBuf[j].shape = buffer[j].shape;
			}

			// invoke the iterator
			iter.processShapes(nbHits, reportBuf);
			reportedOverMaxCount = true;
			return true;
		}
	}

private:
	NpVolumeCacheSqCallback& operator=(const NpVolumeCacheSqCallback&);
};

void NpVolumeCache::forEach(Iterator& iter)
{
	if(mCacheVolume.getType() == PxGeometryType::eINVALID)
		return; // The volume wasn't set, do nothing. No results fed to the iterator.

	// keep track of whether we reported over max count shapes via callback to the user, per static/dynamic type
	bool reportedOverMaxCount[2] = { false, false };
	// will be set to true if cache was overflown inside of callback
	// if this flag stays at false, it means the cache was be filled and validated/timestamped

	for (PxU32 isDynamic = 0; isDynamic <= 1; isDynamic++)
	{
		if(isValid(isDynamic))
			continue;

		const PxU32 maxShapeCount = mMaxShapeCount[isDynamic];

		// retrieve all the shapes overlapping with the current cache volume from the scene
		// using currentTryShapeCount size temp buffer
		PxQueryFilterData fd(isDynamic ? PxQueryFlag::eDYNAMIC : PxQueryFlag::eSTATIC);

		// allocate a local hit buffer, either on the stack or from temp allocator, just big enough to hold #hits=max cached shapes
		PxOverlapHit* localBuffer;
		PxActorShape* reportBuffer;
		const PxU32 maxStackShapes = 65;
		if(maxShapeCount+1 <= maxStackShapes)
		{
			localBuffer = reinterpret_cast<PxOverlapHit*>(PxAlloca(maxStackShapes * (sizeof(PxOverlapHit) + sizeof(PxActorShape))));
			reportBuffer = static_cast<PxActorShape*>(localBuffer + maxStackShapes);
		} else
		{
			localBuffer = reinterpret_cast<PxOverlapHit*>(physx::shdfnd::TempAllocator().allocate(
						(sizeof(PxOverlapHit) + sizeof(PxActorShape)) * (maxShapeCount+1), __FILE__, __LINE__));
			reportBuffer = static_cast<PxActorShape*>(localBuffer + maxShapeCount+1);
		}

		// +1 shape so we can tell inside of single callback if we blew the buffer
		NpVolumeCacheSqCallback cacheSqCallback(this, iter, isDynamic, localBuffer, reportBuffer, maxShapeCount+1);

		// execute the overlap query to get all touching shapes. will be processed in cb.processTouches()
		getNpScene(mSQManager)->overlap(mCacheVolume.any(), mCachePose, cacheSqCallback, fd, NULL);
		reportedOverMaxCount[isDynamic] = cacheSqCallback.reportedOverMaxCount;

		if(maxShapeCount >= maxStackShapes) // release the local hit buffer if not on the stack
			physx::shdfnd::TempAllocator().deallocate(localBuffer);

	} // for (isDynamic)

	// report all cached shapes via the iterator callback, any shapes previously over capacity were already reported
	// if so reportedOverMaxCount is set to true at this point for statics and/or dynamics correspondingly
	if(!reportedOverMaxCount[0] && mCache[0].size() > 0)
		iter.processShapes(mCache[0].size(), mCache[0].begin());
	if(!reportedOverMaxCount[1] && mCache[1].size() > 0)
		iter.processShapes(mCache[1].size(), mCache[1].begin());
	iter.finalizeQuery();
}

//========================================================================================================================
void NpVolumeCache::setMaxNbStaticShapes(PxU32 maxCount)
{
	if(maxCount < mCache[0].size())
	{
		mIsInvalid[0] = true;
		mCache[0].clear();
	}
	mMaxShapeCount[0] = maxCount;
	mCache[0].reserve(maxCount);
}

//========================================================================================================================
PxU32 NpVolumeCache::getMaxNbStaticShapes()
{
	return mMaxShapeCount[0];
}

//========================================================================================================================
void NpVolumeCache::setMaxNbDynamicShapes(PxU32 maxCount)
{
	if(maxCount < mCache[1].size())
	{
		mIsInvalid[1] = true;
		mCache[1].clear();
	}
	mMaxShapeCount[1] = maxCount;
	mCache[1].reserve(maxCount);
}

//========================================================================================================================
PxU32 NpVolumeCache::getMaxNbDynamicShapes()
{
	return mMaxShapeCount[1];
}

//========================================================================================================================
 static PX_FORCE_INLINE PxActorShape* applyAllPreFiltersVC(
	PxActorShape* as, PxQueryHitType::Enum& hitType, const PxQueryFlags& inFilterFlags,
	const PxQueryFilterData& filterData, PxQueryFilterCallback* filterCall, const NpScene& scene, PxHitFlags& hitFlags)
{
	if(!applyClientFilter(as->actor, filterData, scene))
		return NULL;

	if(!applyFilterEquation(static_cast<NpShape*>(as->shape)->getScbShape(), filterData.data))
		return NULL;

	if(filterCall && (inFilterFlags & PxQueryFlag::ePREFILTER))
	{
		PxHitFlags outHitFlags = hitFlags;

		hitType = filterCall->preFilter(filterData.data, as->shape, as->actor, outHitFlags);

		hitFlags = (hitFlags & ~PxHitFlag::eMODIFIABLE_FLAGS) | (outHitFlags & PxHitFlag::eMODIFIABLE_FLAGS);
	}
	return as;
}

// performs a single geometry query for any HitType (PxSweepHit, PxOverlapHit, PxRaycastHit)
template<typename HitType>
struct GeomQueryAny
{
	static PX_INLINE PxU32 geomHit(
		const MultiQueryInput& input,
		const PxGeometry& geom, const PxTransform& pose, PxHitFlags outputFlags,
		PxU32 maxHits, HitType* hits, PxReal& shrunkMaxDistance)
	{
		if(HitTypeSupport<HitType>::IsRaycast)
		{
			// for meshes test against the mesh AABB
			if(0 && geom.getType() == PxGeometryType::eTRIANGLEMESH)
			{
				PxBounds3 bounds;
				computeBounds(bounds, geom, pose, 0.0f, NULL, 1.0f, false);
				PxF32 tnear, tfar;
				if(!intersectRayAABB2(
					bounds.minimum, bounds.maximum, *input.rayOrigin, *input.unitDir, shrunkMaxDistance, tnear, tfar))
						return 0;
			}
			// perform a raycast against the cached shape
			return PxGeometryQuery::raycast(
				input.getOrigin(), input.getDir(), geom, pose, shrunkMaxDistance, outputFlags,
				maxHits, reinterpret_cast<PxRaycastHit*>(hits));
		}
		else if(HitTypeSupport<HitType>::IsSweep)
		{
			PxU32 result = PxU32(PxGeometryQuery::sweep(
				input.getDir(), input.maxDistance, *input.geometry, *input.pose, geom, pose, reinterpret_cast<PxSweepHit&>(hits[0]), outputFlags));
			return result;
		}
		else if(HitTypeSupport<HitType>::IsOverlap)
		{
			PxU32 result = PxU32(PxGeometryQuery::overlap(*input.geometry, *input.pose, geom, pose));
			return result;
		}
		else
		{
			PX_ALWAYS_ASSERT_MESSAGE("Unexpected template expansion in GeomQueryAny::geomHit");
			return 0;
		}
	}
};

// performs a cache volume query for any HitType (PxSweepHit, PxOverlapHit, PxRaycastHit)
template<typename HitType>
struct SceneQueryAny
{
	static PX_INLINE bool doQuery(
		PxScene* scene, const MultiQueryInput& input, PxHitCallback<HitType>& hitCall, PxHitFlags hitFlags,
		const PxQueryFilterData& filterData, PxQueryFilterCallback* filterCall)
	{
		if(HitTypeSupport<HitType>::IsRaycast)
		{
			PX_ASSERT(sizeof(HitType) == sizeof(PxRaycastHit));
			scene->raycast(input.getOrigin(), input.getDir(), input.maxDistance,
				reinterpret_cast<PxRaycastCallback&>(hitCall), hitFlags, filterData, filterCall, NULL);
			return hitCall.hasAnyHits();
		}
		else if(HitTypeSupport<HitType>::IsSweep)
		{
			PX_ASSERT(sizeof(HitType) == sizeof(PxSweepHit));
			scene->sweep(*input.geometry, *input.pose, input.getDir(), input.maxDistance,
				reinterpret_cast<PxSweepCallback&>(hitCall), hitFlags, filterData, filterCall, NULL);
			return hitCall.hasAnyHits();
		}
		else if(HitTypeSupport<HitType>::IsOverlap)
		{
			PX_ASSERT(sizeof(HitType) == sizeof(PxOverlapHit));
			scene->overlap(*input.geometry, *input.pose, reinterpret_cast<PxOverlapCallback&>(hitCall), filterData, filterCall);
			return hitCall.hasAnyHits();
		}
		else
		{
			PX_ALWAYS_ASSERT_MESSAGE("Unexpected template expansion in SceneQueryAny::doQuery");
			return false;
		}
	}
};

//========================================================================================================================
template<typename HitType>
PX_FORCE_INLINE void makeHitSafe(HitType& hit) // hit can contain NaNs (due to uninitialized mem allocation) which cannot be copied
{
	if(HitTypeSupport<HitType>::IsRaycast == 1 || HitTypeSupport<HitType>::IsSweep == 1)
	{
#if PX_VC
#pragma warning(push)
#pragma warning(disable : 4946) // reinterpret_cast used between related classes
		// happens if the template param inherits from PxLocationHit
#endif
		PxLocationHit& hit1 = reinterpret_cast<PxLocationHit&>(hit);
#if PX_VC
#pragma warning(pop)
#endif
		if(!(hit1.flags & PxHitFlag::eDISTANCE))
			hit1.distance = 0.0f;
		if(!(hit1.flags & PxHitFlag::ePOSITION))
			hit1.position = PxVec3(0.0f);
		if(!(hit1.flags & PxHitFlag::eNORMAL))
			hit1.normal = PxVec3(0.0f);
	}
	if(HitTypeSupport<HitType>::IsRaycast == 1 && !(reinterpret_cast<PxRaycastHit&>(hit).flags & PxHitFlag::eUV) )
	{
		reinterpret_cast<PxRaycastHit&>(hit).u = 0.0f;
		reinterpret_cast<PxRaycastHit&>(hit).v = 0.0f;
	}
}

//========================================================================================================================
template<typename HitType>
bool NpVolumeCache::multiQuery(
	const MultiQueryInput& input, PxHitCallback<HitType>& hitCall, PxHitFlags hitFlags,
	const PxQueryFilterData& filterData, PxQueryFilterCallback* filterCall, PxF32 inflation) const
{

	if(HitTypeSupport<HitType>::IsRaycast == 0)
	{
		PX_CHECK_AND_RETURN_VAL(input.pose->isValid(), "sweepInputCheck: pose is not valid.", 0);
	}
	if(HitTypeSupport<HitType>::IsOverlap == 0)
	{
		PX_CHECK_AND_RETURN_VAL(input.getDir().isFinite(), "PxVolumeCache multiQuery input check: unitDir is not valid.", 0);
		PX_CHECK_AND_RETURN_VAL(input.getDir().isNormalized(), "PxVolumeCache multiQuery input check: direction must be normalized", 0);
	}
	if(HitTypeSupport<HitType>::IsRaycast)
	{
		PX_CHECK_AND_RETURN_VAL(input.maxDistance > 0.0f, "PxVolumeCache multiQuery input check: distance cannot be negative or zero", 0);
	}
	if(HitTypeSupport<HitType>::IsSweep)
	{
		PX_CHECK_AND_RETURN_VAL(input.maxDistance >= 0.0f, "NpSceneQueries multiQuery input check: distance cannot be negative", 0);
		PX_CHECK_AND_RETURN_VAL(input.maxDistance != 0.0f || !(hitFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP),
			"PxVolumeCache multiQuery input check: zero-length sweep only valid without the PxHitFlag::eASSUME_NO_INITIAL_OVERLAP flag", 0);
	}

	hitCall.hasBlock = false;
	hitCall.nbTouches = 0;

	const PxQueryFlags filterFlags = filterData.flags;

	// refill the cache if invalid
	for(PxU32 isDynamic = 0; isDynamic <= 1; isDynamic++)
	{
		if(!isValid(isDynamic) && ((isDynamic+1) & PxU32(filterFlags)) != 0) // isDynamic+1 = 1 for static, 2 for dynamic
			// check for overflow or unspecified cache volume&transform, fall back to scene query on overflow (or invalid voltype)
			if(const_cast<NpVolumeCache*>(this)->fillInternal(isDynamic) == FILL_OVER_MAX_COUNT
				|| mCacheVolume.getType() == PxGeometryType::eINVALID)
			{
				// fall back to full scene query with input flags if we blow the cache on either static or dynamic for now
				if(mCacheVolume.getType() == PxGeometryType::eINVALID)
					Ps::getFoundation().error(PxErrorCode::ePERF_WARNING, __FILE__, __LINE__,
						"PxVolumeCache: unspecified volume geometry. Reverting to uncached scene query.");
				return SceneQueryAny<HitType>::doQuery(
					getNpScene(mSQManager), input, hitCall, hitFlags, filterData, filterCall);
			}
	}

	// cache is now valid and there was no overflow
	PX_ASSERT_WITH_MESSAGE(isValid() ,"PxVolumeCache became invalid inside of a scene read call.");

	const PxU32 cacheSize[2] = { mCache[0].size(), mCache[1].size() };

	// early out if the cache is empty and valid
	if(cacheSize[0] == 0 && cacheSize[1] == 0)
		return 0;

	PxReal shrunkDistance = HitTypeSupport<HitType>::IsOverlap ? PX_MAX_REAL : input.maxDistance; // can be progressively shrunk as we go over the list of shapes
	const NpScene& scene = *getNpScene(mSQManager);
	HitType* subHits = NULL;

	// make sure to deallocate the temp buffer when we return
	struct FreeSubhits
	{
		HitType* toFree;
		PX_FORCE_INLINE FreeSubhits() { toFree = NULL; }
		PX_FORCE_INLINE ~FreeSubhits() { if (toFree) physx::shdfnd::TempAllocator().deallocate(toFree); }
	} ds;

	// allocate from temp storage rather than from the stack if we are over some shape count
	PxU32 maxMaxShapeCount = PxMax(mMaxShapeCount[0], mMaxShapeCount[1]); // max size buffer for statics and dynamics
	if(maxMaxShapeCount < 128) // somewhat arbitrary
		subHits = reinterpret_cast<HitType*>(PxAlloca(sizeof(HitType)*maxMaxShapeCount));
	else
		ds.toFree = subHits = reinterpret_cast<HitType*>(physx::shdfnd::TempAllocator().allocate(sizeof(HitType)*maxMaxShapeCount, __FILE__, __LINE__));

	const bool noBlock = (filterFlags & PxQueryFlag::eNO_BLOCK);

	// for statics & dynamics
	for(PxU32 isDynamic = 0; isDynamic <= 1; isDynamic++)
		// iterate over all the cached shapes
		for(PxU32 iCachedShape = 0; iCachedShape < cacheSize[isDynamic]; iCachedShape++)
	{
		PxActorShape* as = &mCache[isDynamic][iCachedShape];

		const PxU32 actorFlag = (PxU32(as->actor->is<PxRigidDynamic>() != NULL) + 1); // 1 for static, 2 for dynamic
		PX_COMPILE_TIME_ASSERT(PxQueryFlag::eSTATIC == 1);
		PX_COMPILE_TIME_ASSERT(PxQueryFlag::eDYNAMIC == 2);
		if((actorFlag & PxU32(filterFlags)) == 0) // filter the actor according to the input static/dynamic filter
			continue;

		// for no filter callback, default to eTOUCH for MULTIPLE, eBLOCK otherwise
		PxQueryHitType::Enum shapeHitType = hitCall.maxNbTouches ? PxQueryHitType::eTOUCH : PxQueryHitType::eBLOCK;

		// apply pre-filter
		PxHitFlags queryFlags = hitFlags;
		as = applyAllPreFiltersVC(as, shapeHitType, filterFlags, filterData, filterCall, scene, hitFlags);
		if(!as || shapeHitType == PxQueryHitType::eNONE)
			continue;
		PX_ASSERT(as->actor && as->shape);

		NpShape* shape = static_cast<NpShape*>(as->shape);

		// compute the global pose for the cached shape and actor
		PX_ALIGN(16, PxTransform) globalPose;
		NpActor::getGlobalPose(globalPose, *shape, *as->actor);

		const GeometryUnion& cachedShapeGeom = shape->getGeometryFast();

		// call the geometry specific intersection template
		PxU32 nbSubHits = GeomQueryAny<HitType>::geomHit(
			input, cachedShapeGeom.getGeometry(), globalPose, queryFlags,
			// limit number of hits to 1 for meshes if eMESH_MULTIPLE wasn't specified.
			//this tells geomQuery to only look for a closest hit
			(cachedShapeGeom.getType() == PxGeometryType::eTRIANGLEMESH && !(hitFlags & PxHitFlag::eMESH_MULTIPLE)) ? 1 : maxMaxShapeCount,
			subHits, shrunkDistance);

		// iterate over geometry subhits
		for(PxU32 iSubHit = 0; iSubHit < nbSubHits; iSubHit++)
		{
			HitType& hit = subHits[iSubHit];
			hit.actor = as->actor;
			hit.shape = as->shape;
			makeHitSafe<HitType>(hit);

			// some additional processing only for sweep hits with initial overlap
			if(HitTypeSupport<HitType>::IsSweep && HITDIST(hit) == 0.0f)
				// PT: necessary as some leaf routines are called with reversed params, thus writing +unitDir there.
				reinterpret_cast<PxSweepHit&>(hit).normal = -input.getDir();

			// start out with hitType for this cached shape set to a pre-filtered hit type
			PxQueryHitType::Enum hitType = shapeHitType;

			// run the post-filter if specified in filterFlags and filterCall is non-NULL
			if(filterCall && (filterFlags & PxQueryFlag::ePOSTFILTER))
				hitType = filterCall->postFilter(filterData.data, hit);

			// -------------------------- handle eANY_HIT hits ---------------------------------
			if(filterFlags & PxQueryFlag::eANY_HIT && hitType != PxQueryHitType::eNONE)
			{
				hitCall.block = hit;
				hitCall.finalizeQuery();
				return (hitCall.hasBlock = true);
			}

			if(noBlock)
				hitType = PxQueryHitType::eTOUCH;

			if(hitType == PxQueryHitType::eTOUCH)
			{
				// -------------------------- handle eTOUCH hits ---------------------------------
				// for MULTIPLE hits (hitCall.touches != NULL), store the hit. For other qTypes ignore it.
				if(hitCall.maxNbTouches && HITDIST(hit) <= shrunkDistance)
				{
					// Buffer full: need to find the closest blocking hit, clip touch hits and flush the buffer
					if(hitCall.nbTouches == hitCall.maxNbTouches)
					{
						// issue a second nested query just looking for the closest blocking hit
						// could do better perf-wise by saving traversal state (start looking for blocking from this point)
						// but this is not a perf critical case because users can provide a bigger buffer
						// that covers non-degenerate cases
						PxHitBuffer<HitType> buf1;
						if(multiQuery<HitType>(input, buf1, hitFlags, filterData, filterCall, inflation))
						{
							hitCall.block = buf1.block;
							hitCall.hasBlock = true;
							hitCall.nbTouches =
								clipHitsToNewMaxDist<HitType>(hitCall.touches, hitCall.nbTouches, HITDIST(buf1.block));
						}

						if(hitCall.nbTouches == hitCall.maxNbTouches)
						{
							PxAgain again = hitCall.processTouches(hitCall.touches, hitCall.maxNbTouches);
							if(!again) // early exit opportunity
							{
								hitCall.finalizeQuery();
								return hitCall.hasBlock;
							} else
								hitCall.nbTouches = 0; // reset nbTouches so we can continue accumulating again
						}

					} // if(hitCall.nbTouches == hitCall.maxNbTouches)

					hitCall.touches[hitCall.nbTouches++] = hit;
				} // if(hitCall.maxNbTouches && hit.dist <= shrunkDist)
			}
			else if(hitType == PxQueryHitType::eBLOCK)
			{
				// -------------------------- handle eBLOCK hits ---------------------------------
				// former SINGLE and MULTIPLE cases => update blocking hit distance
				// only eBLOCK qualifies as a closest hit candidate for "single" query
				// => compare against the best distance and store
				if(HITDIST(hit) <= shrunkDistance)
				{
					shrunkDistance = HITDIST(hit);
					hitCall.block = hit;
					hitCall.hasBlock = true;
				}
			} else
			{
				PX_ASSERT(hitType == PxQueryHitType::eNONE);
			}
		} // for iSubHit
	} // for isDynamic, for iCachedShape

	// clip any unreported touch hits to block.distance and report via callback
	if(hitCall.hasBlock && hitCall.nbTouches)
		hitCall.nbTouches = clipHitsToNewMaxDist(hitCall.touches, hitCall.nbTouches, HITDIST(hitCall.block));
	if(hitCall.nbTouches)
	{
		bool again = hitCall.processTouches(hitCall.touches, hitCall.nbTouches);
		if(again)
			hitCall.nbTouches = 0;
	}
	hitCall.finalizeQuery();

	return hitCall.hasBlock;
}

#undef HITDIST

//========================================================================================================================
bool NpVolumeCache::raycast(
	const PxVec3& origin, const PxVec3& unitDir, const PxReal distance, PxRaycastCallback& hitCall, PxHitFlags hitFlags,
	const PxQueryFilterData& filterData, PxQueryFilterCallback* filterCall) const
{
	PX_SIMD_GUARD;

	MultiQueryInput input(origin, unitDir, distance);
	bool result = multiQuery<PxRaycastHit>(input, hitCall, hitFlags, filterData, filterCall);
	return result;
}

//=======================================================================================================================
bool NpVolumeCache::sweep(
	const PxGeometry& geometry, const PxTransform& pose, const PxVec3& unitDir, const PxReal distance,
	PxSweepCallback& hitCall, PxHitFlags hitFlags,
	const PxQueryFilterData& filterData, PxQueryFilterCallback* filterCall, const PxReal inflation) const
{
	PX_SIMD_GUARD;

	MultiQueryInput input(&geometry, &pose, unitDir, distance, 0.0f);
	bool result = multiQuery<PxSweepHit>(input, hitCall, hitFlags, filterData, filterCall, inflation);
	return result;
}

//========================================================================================================================
bool NpVolumeCache::overlap(
	const PxGeometry& geometry, const PxTransform& transform,
	PxOverlapCallback& hitCall, const PxQueryFilterData& filterData, PxQueryFilterCallback* filterCall) const
{
	PX_SIMD_GUARD;

	MultiQueryInput input(&geometry, &transform);
	bool result = multiQuery<PxOverlapHit>(input, hitCall, PxHitFlags(), filterData, filterCall);
	return result;
}

//========================================================================================================================
void NpVolumeCache::onOriginShift(const PxVec3& shift)
{
	mCachePose.p -= shift;
}

} // namespace physx
