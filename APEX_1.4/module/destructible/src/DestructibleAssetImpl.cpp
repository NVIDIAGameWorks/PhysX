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
#include "DestructibleAssetImpl.h"
#include "DestructibleActorProxy.h"
#include "DestructiblePreviewProxy.h"
#include "ModuleDestructibleImpl.h"
#include "PxCooking.h"
#include "PxPhysics.h"
#include <PxScene.h>
#include "ModulePerfScope.h"
#include "ApexUsingNamespace.h"
#include "RenderMeshAssetIntl.h"
#include "Cof44.h"
#include "nvparameterized/NvParamUtils.h"
#include "PsMemoryBuffer.h"
#if APEX_USE_PARTICLES
#include "EmitterAsset.h"
#else
static const char* EMITTER_AUTHORING_TYPE_NAME = "ApexEmitterAsset";
#endif

#include "../../../framework/include/autogen/RenderMeshAssetParameters.h"
#include "../../../framework/include/ApexRenderMeshAsset.h"


#include "ApexSharedSerialization.h"
#include "ApexRand.h"

#include "ApexMerge.h"
#include "ApexFind.h"

namespace nvidia
{
namespace destructible
{
using namespace physx;

struct ChunkSortElement
{
	int32_t index;
	int32_t parentIndex;
	int32_t depth;
};

struct IndexSortedEdge
{
	IndexSortedEdge() {}
	IndexSortedEdge(uint32_t _i0, uint32_t _i1, uint32_t _submeshIndex, const PxVec3& _triangleNormal)
	{
		if (_i0 <= _i1)
		{
			i0 = _i0;
			i1 = _i1;
		}
		else
		{
			i0 = _i1;
			i1 = _i0;
		}
		submeshIndex = _submeshIndex;
		triangleNormal = _triangleNormal;
	}

	uint32_t i0;
	uint32_t i1;
	uint32_t submeshIndex;
	PxVec3 triangleNormal;
};

// We'll use this struct to store trim planes for each hull in each part
struct TrimPlane
{
	uint32_t partIndex;
	uint32_t hullIndex;
	PxPlane plane;

	struct LessThan
	{
		PX_INLINE bool operator()(const TrimPlane& x, const TrimPlane& y) const
		{
			return x.partIndex != y.partIndex ? (x.partIndex < y.partIndex) : (x.hullIndex < y.hullIndex);
		}
	};
};

#ifndef WITHOUT_APEX_AUTHORING
static int compareChunkParents(
    const void* A,
    const void* B)
{
	ChunkSortElement& eA = *(ChunkSortElement*)A;
	ChunkSortElement& eB = *(ChunkSortElement*)B;

	const int32_t depthDiff = eA.depth - eB.depth;
	if (depthDiff)
	{
		return depthDiff;
	}

	const int32_t parentDiff = eA.parentIndex - eB.parentIndex;
	if (parentDiff)
	{
		return parentDiff;
	}

	return eA.index - eB.index;	// Keeps sort stable
}
#endif

void DestructibleAssetImpl::setParameters(const DestructibleParameters& parameters)
{
	setParameters(parameters, mParams->destructibleParameters);

	NvParameterized::Handle handle(*mParams);
	mParams->getParameterHandle("depthParameters", handle);
	mParams->resizeArray(handle, (int32_t)parameters.depthParametersCount);
	for (uint32_t i = 0; i < parameters.depthParametersCount; ++i)
	{
		DestructibleAssetParametersNS::DestructibleDepthParameters_Type& d = mParams->depthParameters.buf[i];
		const DestructibleDepthParameters& dparm = parameters.depthParameters[i];
		d.OVERRIDE_IMPACT_DAMAGE		= (dparm.flags & DestructibleDepthParametersFlag::OVERRIDE_IMPACT_DAMAGE) ? true : false;
		d.OVERRIDE_IMPACT_DAMAGE_VALUE	= (dparm.flags & DestructibleDepthParametersFlag::OVERRIDE_IMPACT_DAMAGE_VALUE) ? true : false;
		d.IGNORE_POSE_UPDATES			= (dparm.flags & DestructibleDepthParametersFlag::IGNORE_POSE_UPDATES) ? true : false;
		d.IGNORE_RAYCAST_CALLBACKS		= (dparm.flags & DestructibleDepthParametersFlag::IGNORE_RAYCAST_CALLBACKS) ? true : false;
		d.IGNORE_CONTACT_CALLBACKS		= (dparm.flags & DestructibleDepthParametersFlag::IGNORE_CONTACT_CALLBACKS) ? true : false;
		d.USER_FLAG_0					= (dparm.flags & DestructibleDepthParametersFlag::USER_FLAG_0) ? true : false;
		d.USER_FLAG_1					= (dparm.flags & DestructibleDepthParametersFlag::USER_FLAG_1) ? true : false;
		d.USER_FLAG_2					= (dparm.flags & DestructibleDepthParametersFlag::USER_FLAG_2) ? true : false;
		d.USER_FLAG_3					= (dparm.flags & DestructibleDepthParametersFlag::USER_FLAG_3) ? true : false;
	}
}

void DestructibleAssetImpl::setParameters(const DestructibleParameters& parameters, DestructibleAssetParametersNS::DestructibleParameters_Type& destructibleParameters)
{
	destructibleParameters.damageCap                             = parameters.damageCap;
	destructibleParameters.debrisDepth                           = parameters.debrisDepth;
	destructibleParameters.debrisLifetimeMax                     = parameters.debrisLifetimeMax;
	destructibleParameters.debrisLifetimeMin                     = parameters.debrisLifetimeMin;
	destructibleParameters.debrisMaxSeparationMax                = parameters.debrisMaxSeparationMax;
	destructibleParameters.debrisMaxSeparationMin                = parameters.debrisMaxSeparationMin;
	destructibleParameters.debrisDestructionProbability          = parameters.debrisDestructionProbability;
	destructibleParameters.dynamicChunkDominanceGroup            = parameters.dynamicChunksDominanceGroup;
	destructibleParameters.dynamicChunksGroupsMask.useGroupsMask = parameters.useDynamicChunksGroupsMask;
	destructibleParameters.dynamicChunksGroupsMask.bits0         = parameters.dynamicChunksFilterData.word0;
	destructibleParameters.dynamicChunksGroupsMask.bits1         = parameters.dynamicChunksFilterData.word1;
	destructibleParameters.dynamicChunksGroupsMask.bits2         = parameters.dynamicChunksFilterData.word2;
	destructibleParameters.dynamicChunksGroupsMask.bits3         = parameters.dynamicChunksFilterData.word3;
	destructibleParameters.supportStrength						 = parameters.supportStrength;
	destructibleParameters.legacyChunkBoundsTestSetting			 = parameters.legacyChunkBoundsTestSetting;
	destructibleParameters.legacyDamageRadiusSpreadSetting		 = parameters.legacyDamageRadiusSpreadSetting;
    destructibleParameters.alwaysDrawScatterMesh	             = parameters.alwaysDrawScatterMesh; 

	destructibleParameters.essentialDepth = parameters.essentialDepth;
	destructibleParameters.flags.ACCUMULATE_DAMAGE       = (parameters.flags & DestructibleParametersFlag::ACCUMULATE_DAMAGE) ? true : false;
	destructibleParameters.flags.DEBRIS_TIMEOUT          = (parameters.flags & DestructibleParametersFlag::DEBRIS_TIMEOUT) ? true : false;
	destructibleParameters.flags.DEBRIS_MAX_SEPARATION   = (parameters.flags & DestructibleParametersFlag::DEBRIS_MAX_SEPARATION) ? true : false;
	destructibleParameters.flags.CRUMBLE_SMALLEST_CHUNKS = (parameters.flags & DestructibleParametersFlag::CRUMBLE_SMALLEST_CHUNKS) ? true : false;
	destructibleParameters.flags.ACCURATE_RAYCASTS       = (parameters.flags & DestructibleParametersFlag::ACCURATE_RAYCASTS) ? true : false;
	destructibleParameters.flags.USE_VALID_BOUNDS        = (parameters.flags & DestructibleParametersFlag::USE_VALID_BOUNDS) ? true : false;
	destructibleParameters.flags.CRUMBLE_VIA_RUNTIME_FRACTURE = (parameters.flags & DestructibleParametersFlag::CRUMBLE_VIA_RUNTIME_FRACTURE) ? true : false;
	destructibleParameters.forceToDamage                 = parameters.forceToDamage;
	destructibleParameters.fractureImpulseScale          = parameters.fractureImpulseScale;
	destructibleParameters.damageDepthLimit              = parameters.damageDepthLimit;
	destructibleParameters.impactVelocityThreshold       = parameters.impactVelocityThreshold;
	destructibleParameters.maxChunkSpeed                 = parameters.maxChunkSpeed;
	destructibleParameters.minimumFractureDepth          = parameters.minimumFractureDepth;
	destructibleParameters.impactDamageDefaultDepth      = parameters.impactDamageDefaultDepth;
	destructibleParameters.debrisDestructionProbability  = parameters.debrisDestructionProbability;
	destructibleParameters.validBounds                   = parameters.validBounds;

	// RT Fracture Parameters
	destructibleParameters.runtimeFracture.sheetFracture			= parameters.rtFractureParameters.sheetFracture;
	destructibleParameters.runtimeFracture.depthLimit				= parameters.rtFractureParameters.depthLimit;
	destructibleParameters.runtimeFracture.destroyIfAtDepthLimit	= parameters.rtFractureParameters.destroyIfAtDepthLimit;
	destructibleParameters.runtimeFracture.minConvexSize			= parameters.rtFractureParameters.minConvexSize;
	destructibleParameters.runtimeFracture.impulseScale				= parameters.rtFractureParameters.impulseScale;
	destructibleParameters.runtimeFracture.glass.numSectors			= parameters.rtFractureParameters.glass.numSectors;
	destructibleParameters.runtimeFracture.glass.sectorRand			= parameters.rtFractureParameters.glass.sectorRand;
	destructibleParameters.runtimeFracture.glass.firstSegmentSize	= parameters.rtFractureParameters.glass.firstSegmentSize;
	destructibleParameters.runtimeFracture.glass.segmentScale		= parameters.rtFractureParameters.glass.segmentScale;
	destructibleParameters.runtimeFracture.glass.segmentRand		= parameters.rtFractureParameters.glass.segmentRand;
	destructibleParameters.runtimeFracture.attachment.posX			= parameters.rtFractureParameters.attachment.posX;
	destructibleParameters.runtimeFracture.attachment.negX			= parameters.rtFractureParameters.attachment.negX;
	destructibleParameters.runtimeFracture.attachment.posY			= parameters.rtFractureParameters.attachment.posY;
	destructibleParameters.runtimeFracture.attachment.negY			= parameters.rtFractureParameters.attachment.negY;
	destructibleParameters.runtimeFracture.attachment.posZ			= parameters.rtFractureParameters.attachment.posZ;
	destructibleParameters.runtimeFracture.attachment.negZ			= parameters.rtFractureParameters.attachment.negZ;
}

void DestructibleAssetImpl::setInitParameters(const DestructibleInitParameters& parameters)
{
	mParams->supportDepth = parameters.supportDepth;
	mParams->formExtendedStructures      = (parameters.flags & DestructibleInitParametersFlag::FORM_EXTENDED_STRUCTURES) ? true : false;
	mParams->useAssetDefinedSupport		 = (parameters.flags & DestructibleInitParametersFlag::ASSET_DEFINED_SUPPORT) ? true : false;
	mParams->useWorldSupport			 = (parameters.flags & DestructibleInitParametersFlag::WORLD_SUPPORT) ? true : false;
}

DestructibleParameters DestructibleAssetImpl::getParameters() const
{
	return getParameters(mParams->destructibleParameters, &mParams->depthParameters);
}

DestructibleParameters DestructibleAssetImpl::getParameters(const DestructibleAssetParametersNS::DestructibleParameters_Type& destructibleParameters,
														  const DestructibleAssetParametersNS::DestructibleDepthParameters_DynamicArray1D_Type* destructibleDepthParameters)
{
	DestructibleParameters parameters;

	parameters.damageCap                  = destructibleParameters.damageCap;
	parameters.debrisDepth                = destructibleParameters.debrisDepth;
	parameters.debrisLifetimeMax          = destructibleParameters.debrisLifetimeMax;
	parameters.debrisLifetimeMin          = destructibleParameters.debrisLifetimeMin;
	parameters.debrisMaxSeparationMax     = destructibleParameters.debrisMaxSeparationMax;
	parameters.debrisMaxSeparationMin     = destructibleParameters.debrisMaxSeparationMin;
	parameters.dynamicChunksDominanceGroup   = (uint8_t)destructibleParameters.dynamicChunkDominanceGroup;
	parameters.useDynamicChunksGroupsMask = destructibleParameters.dynamicChunksGroupsMask.useGroupsMask;
	parameters.dynamicChunksFilterData.word0 = destructibleParameters.dynamicChunksGroupsMask.bits0;
	parameters.dynamicChunksFilterData.word1 = destructibleParameters.dynamicChunksGroupsMask.bits1;
	parameters.dynamicChunksFilterData.word2 = destructibleParameters.dynamicChunksGroupsMask.bits2;
	parameters.dynamicChunksFilterData.word3 = destructibleParameters.dynamicChunksGroupsMask.bits3;
	parameters.essentialDepth = destructibleParameters.essentialDepth;
	parameters.flags = 0;
	parameters.alwaysDrawScatterMesh = destructibleParameters.alwaysDrawScatterMesh;
	if (destructibleParameters.flags.ACCUMULATE_DAMAGE)
	{
		parameters.flags |= DestructibleParametersFlag::ACCUMULATE_DAMAGE;
	}
	if (destructibleParameters.flags.DEBRIS_TIMEOUT)
	{
		parameters.flags |= DestructibleParametersFlag::DEBRIS_TIMEOUT;
	}
	if (destructibleParameters.flags.DEBRIS_MAX_SEPARATION)
	{
		parameters.flags |= DestructibleParametersFlag::DEBRIS_MAX_SEPARATION;
	}
	if (destructibleParameters.flags.CRUMBLE_SMALLEST_CHUNKS)
	{
		parameters.flags |= DestructibleParametersFlag::CRUMBLE_SMALLEST_CHUNKS;
	}
	if (destructibleParameters.flags.ACCURATE_RAYCASTS)
	{
		parameters.flags |= DestructibleParametersFlag::ACCURATE_RAYCASTS;
	}
	if (destructibleParameters.flags.USE_VALID_BOUNDS)
	{
		parameters.flags |= DestructibleParametersFlag::USE_VALID_BOUNDS;
	}
	if (destructibleParameters.flags.CRUMBLE_VIA_RUNTIME_FRACTURE)
	{
		parameters.flags |= DestructibleParametersFlag::CRUMBLE_VIA_RUNTIME_FRACTURE;
	}
	parameters.forceToDamage            = destructibleParameters.forceToDamage;
	parameters.fractureImpulseScale     = destructibleParameters.fractureImpulseScale;
	parameters.damageDepthLimit         = destructibleParameters.damageDepthLimit;
	parameters.impactVelocityThreshold  = destructibleParameters.impactVelocityThreshold;
	parameters.maxChunkSpeed            = destructibleParameters.maxChunkSpeed;
	parameters.minimumFractureDepth     = destructibleParameters.minimumFractureDepth;
	parameters.impactDamageDefaultDepth = destructibleParameters.impactDamageDefaultDepth;
	parameters.debrisDestructionProbability = destructibleParameters.debrisDestructionProbability;
	parameters.validBounds              = destructibleParameters.validBounds;

	if (destructibleDepthParameters)
	{
		//NvParameterized::Handle handle(*mParams);
		//mParams->getParameterHandle("depthParameters", handle);
		parameters.depthParametersCount = PxMin((uint32_t)DestructibleParameters::kDepthParametersCountMax,
														(uint32_t)destructibleDepthParameters->arraySizes[0]);
	for (int i = 0; i < (int)parameters.depthParametersCount; ++i)
	{
			DestructibleAssetParametersNS::DestructibleDepthParameters_Type& d = destructibleDepthParameters->buf[i];
		DestructibleDepthParameters& dparm = parameters.depthParameters[i];
		dparm.flags = 0;
		if (d.OVERRIDE_IMPACT_DAMAGE)
		{
			dparm.flags |= DestructibleDepthParametersFlag::OVERRIDE_IMPACT_DAMAGE;
		}
		if (d.OVERRIDE_IMPACT_DAMAGE_VALUE)
		{
			dparm.flags |= DestructibleDepthParametersFlag::OVERRIDE_IMPACT_DAMAGE_VALUE;
		}
		if (d.IGNORE_POSE_UPDATES)
		{
			dparm.flags |= DestructibleDepthParametersFlag::IGNORE_POSE_UPDATES;
		}
		if (d.IGNORE_RAYCAST_CALLBACKS)
		{
			dparm.flags |= DestructibleDepthParametersFlag::IGNORE_RAYCAST_CALLBACKS;
		}
		if (d.IGNORE_CONTACT_CALLBACKS)
		{
			dparm.flags |= DestructibleDepthParametersFlag::IGNORE_CONTACT_CALLBACKS;
		}
		if (d.USER_FLAG_0)
		{
			dparm.flags |= DestructibleDepthParametersFlag::USER_FLAG_0;
		}
		if (d.USER_FLAG_1)
		{
			dparm.flags |= DestructibleDepthParametersFlag::USER_FLAG_1;
		}
		if (d.USER_FLAG_2)
		{
			dparm.flags |= DestructibleDepthParametersFlag::USER_FLAG_2;
		}
		if (d.USER_FLAG_3)
		{
			dparm.flags |= DestructibleDepthParametersFlag::USER_FLAG_3;
		}
	}
	}

	// RT Fracture Parameters
	parameters.rtFractureParameters.sheetFracture			= destructibleParameters.runtimeFracture.sheetFracture;
	parameters.rtFractureParameters.depthLimit				= destructibleParameters.runtimeFracture.depthLimit;
	parameters.rtFractureParameters.destroyIfAtDepthLimit	= destructibleParameters.runtimeFracture.destroyIfAtDepthLimit;
	parameters.rtFractureParameters.minConvexSize			= destructibleParameters.runtimeFracture.minConvexSize;
	parameters.rtFractureParameters.impulseScale			= destructibleParameters.runtimeFracture.impulseScale;
	parameters.rtFractureParameters.glass.numSectors		= destructibleParameters.runtimeFracture.glass.numSectors;
	parameters.rtFractureParameters.glass.sectorRand		= destructibleParameters.runtimeFracture.glass.sectorRand;
	parameters.rtFractureParameters.glass.firstSegmentSize	= destructibleParameters.runtimeFracture.glass.firstSegmentSize;
	parameters.rtFractureParameters.glass.segmentScale		= destructibleParameters.runtimeFracture.glass.segmentScale;
	parameters.rtFractureParameters.glass.segmentRand		= destructibleParameters.runtimeFracture.glass.segmentRand;
	parameters.rtFractureParameters.attachment.posX			= destructibleParameters.runtimeFracture.attachment.posX;
	parameters.rtFractureParameters.attachment.negX			= destructibleParameters.runtimeFracture.attachment.negX;
	parameters.rtFractureParameters.attachment.posY			= destructibleParameters.runtimeFracture.attachment.posY;
	parameters.rtFractureParameters.attachment.negY			= destructibleParameters.runtimeFracture.attachment.negY;
	parameters.rtFractureParameters.attachment.posZ			= destructibleParameters.runtimeFracture.attachment.posZ;
	parameters.rtFractureParameters.attachment.negZ			= destructibleParameters.runtimeFracture.attachment.negZ;
	parameters.supportStrength								= destructibleParameters.supportStrength;
	parameters.legacyChunkBoundsTestSetting					 = destructibleParameters.legacyChunkBoundsTestSetting;
	parameters.legacyDamageRadiusSpreadSetting				 = destructibleParameters.legacyDamageRadiusSpreadSetting;

	return parameters;
}

DestructibleInitParameters DestructibleAssetImpl::getInitParameters() const
{
	DestructibleInitParameters parameters;

	parameters.supportDepth = mParams->supportDepth;
	parameters.flags = 0;
	if (mParams->formExtendedStructures)
	{
		parameters.flags |= DestructibleInitParametersFlag::FORM_EXTENDED_STRUCTURES;
	}
	if (mParams->useAssetDefinedSupport)
	{
		parameters.flags |= DestructibleInitParametersFlag::ASSET_DEFINED_SUPPORT;
	}
	if (mParams->useWorldSupport)
	{
		parameters.flags |= DestructibleInitParametersFlag::WORLD_SUPPORT;
	}

	return parameters;
}

void DestructibleAssetImpl::setCrumbleEmitterName(const char* name)
{
	NvParameterized::Handle handle(*mParams);
	mParams->getParameterHandle("crumbleEmitterName", handle);
	mParams->setParamString(handle, name ? name : "");
}

const char* DestructibleAssetImpl::getCrumbleEmitterName() const
{
	const char* name = mParams->crumbleEmitterName;
	return (name && *name) ? name : NULL;
}

void DestructibleAssetImpl::setDustEmitterName(const char* name)
{
	NvParameterized::Handle handle(*mParams);
	mParams->getParameterHandle("dustEmitterName", handle);
	mParams->setParamString(handle, name ? name : "");
}

const char* DestructibleAssetImpl::getDustEmitterName() const
{
	const char* name = mParams->dustEmitterName;
	return (name && *name) ? name : NULL;
}

void DestructibleAssetImpl::setFracturePatternName(const char* name)
{
	NvParameterized::Handle handle(*mParams);
	mParams->getParameterHandle("fracturePatternName", handle);
	mParams->setParamString(handle, name ? name : "");
}

const char* DestructibleAssetImpl::getFracturePatternName() const
{
	// TODO: Add to asset params
	const char* name = "";//mParams->fracturePatternName;
	return (name && *name) ? name : NULL;
}

void DestructibleAssetImpl::setChunkOverlapsCacheDepth(int32_t depth)
{
	chunkOverlapCacheDepth = depth;
}

void DestructibleAssetImpl::calculateChunkDepthStarts()
{
	const uint32_t chunkCount = (uint32_t)mParams->chunks.arraySizes[0];

	NvParameterized::Handle handle(*mParams);
	mParams->getParameterHandle("firstChunkAtDepth", handle);
	mParams->resizeArray(handle, (int32_t)mParams->depthCount + 1);
	mParams->firstChunkAtDepth.buf[mParams->depthCount] = chunkCount;

	uint32_t stopIndex = 0;
	for (uint32_t depth = 0; depth < mParams->depthCount; ++depth)
	{
		mParams->firstChunkAtDepth.buf[depth] = stopIndex;
		while (stopIndex < chunkCount)
		{
			if (mParams->chunks.buf[stopIndex].depth != depth)
			{
				break;
			}
			++stopIndex;
		}
	}
}

CachedOverlapsNS::IntPair_DynamicArray1D_Type* DestructibleAssetImpl::getOverlapsAtDepth(uint32_t depth, bool create) const
{
	if (depth >= mParams->depthCount)
	{
		return NULL;
	}

	int size = mParams->overlapsAtDepth.arraySizes[0];
	if (size <= (int)depth && !create)
	{
		return NULL;
	}

	NvParameterized::Handle handle(*mParams);
	mParams->getParameterHandle("overlapsAtDepth", handle);

	if (create)
	{
		mParams->resizeArray(handle, (int32_t)mParams->depthCount);
		NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
		while (size < (int)mParams->depthCount)
		{
			CachedOverlaps* cachedOverlaps = DYNAMIC_CAST(CachedOverlaps*)(traits->createNvParameterized(CachedOverlaps::staticClassName()));
			mParams->overlapsAtDepth.buf[size++] = cachedOverlaps;
			cachedOverlaps->isCached = false;
		}
	}

	CachedOverlaps* cachedOverlapsAtDepth = DYNAMIC_CAST(CachedOverlaps*)(mParams->overlapsAtDepth.buf[depth]);
	if (!cachedOverlapsAtDepth->isCached)
	{
		if (!create)
		{
			return NULL;
		}
		physx::Array<IntPair> overlaps;
		calculateChunkOverlaps(overlaps, depth);
		NvParameterized::Handle overlapsHandle(*cachedOverlapsAtDepth);
		cachedOverlapsAtDepth->getParameterHandle("overlaps", overlapsHandle);
		overlapsHandle.resizeArray(2*(int32_t)overlaps.size());
		for (uint32_t i = 0; i < overlaps.size(); ++i)
		{
			IntPair& pair = overlaps[i];
			
			CachedOverlapsNS::IntPair_Type& ppair = cachedOverlapsAtDepth->overlaps.buf[2*i];
			ppair.i0 = pair.i0;
			ppair.i1 = pair.i1;

			CachedOverlapsNS::IntPair_Type& ppairSymmetric = cachedOverlapsAtDepth->overlaps.buf[2*i+1];
			ppairSymmetric.i0 = pair.i1;
			ppairSymmetric.i1 = pair.i0;
		}
		qsort(cachedOverlapsAtDepth->overlaps.buf, (uint32_t)cachedOverlapsAtDepth->overlaps.arraySizes[0], sizeof(IntPair), IntPair::compare);

		cachedOverlapsAtDepth->isCached = 1;
	}

	return &cachedOverlapsAtDepth->overlaps;
}

void DestructibleAssetImpl::calculateChunkOverlaps(physx::Array<IntPair>& overlaps, uint32_t depth) const
{
	const float padding = mParams->neighborPadding * (mParams->bounds.maximum - mParams->bounds.minimum).magnitude();

	const PxTransform identityTM(PxIdentity);
	const PxVec3 identityScale(1.0f);

	const uint32_t startIndex = mParams->firstChunkAtDepth.buf[depth];
	const uint32_t stopIndex = mParams->firstChunkAtDepth.buf[depth + 1];
	const uint32_t chunksAtDepth = stopIndex - startIndex;

	// Find AABB overlaps
	physx::Array<BoundsRep> chunkBoundsReps;
	chunkBoundsReps.reserve(chunksAtDepth);
	for (uint32_t chunkIndex = startIndex; chunkIndex < stopIndex; ++chunkIndex)
	{
		BoundsRep& chunkBoundsRep = chunkBoundsReps.insert();
		chunkBoundsRep.aabb = getChunkActorLocalBounds(chunkIndex);
		PX_ASSERT(!chunkBoundsRep.aabb.isEmpty());
		chunkBoundsRep.aabb.fattenFast(padding);
	}
	if (chunkBoundsReps.size() > 0)
	{
		boundsCalculateOverlaps(overlaps, Bounds3XYZ, &chunkBoundsReps[0], chunkBoundsReps.size(), sizeof(chunkBoundsReps[0]));
	}

	// Now do detailed overlap test
	uint32_t overlapCount = 0;
	for (uint32_t overlapIndex = 0; overlapIndex < overlaps.size(); ++overlapIndex)
	{
		IntPair& AABBOverlap = overlaps[overlapIndex];
		AABBOverlap.i0 += startIndex;
		AABBOverlap.i1 += startIndex;
		if (chunksInProximity(*this, (uint16_t)AABBOverlap.i0, identityTM, identityScale, *this, (uint16_t)AABBOverlap.i1, identityTM, identityScale, 2 * padding))
		{
			overlaps[overlapCount++] = AABBOverlap;
		}
	}

	overlaps.resize(overlapCount);
}

void DestructibleAssetImpl::cacheChunkOverlapsUpToDepth(int32_t depth)
{
	if (mParams->depthCount < 1)
	{
		return;
	}

	if (depth < 0)
	{
		depth = (int32_t)mParams->supportDepth;
	}

	depth = PxMin(depth, (int32_t)mParams->depthCount - 1);

	for (uint32_t d = 0; d <= (uint32_t)depth; ++d)
	{
		getOverlapsAtDepth(d);
	}

	for (uint32_t d = (uint32_t)depth + 1; d < (uint32_t)mParams->overlapsAtDepth.arraySizes[0]; ++d)
	{
		CachedOverlaps* cachedOverlaps = DYNAMIC_CAST(CachedOverlaps*)(mParams->overlapsAtDepth.buf[d]);
		NvParameterized::Handle handle(*cachedOverlaps);
		cachedOverlaps->getParameterHandle("overlaps", handle);
		cachedOverlaps->resizeArray(handle, 0);
	}
}


void DestructibleAssetImpl::clearChunkOverlaps(int32_t depth, bool keepCachedFlag)
{
	int32_t depthStart = (depth < 0) ? 0 : depth;
	int32_t depthEnd = (depth < 0) ? mParams->overlapsAtDepth.arraySizes[0] : PxMin(depth+1, mParams->overlapsAtDepth.arraySizes[0]);
	for (int32_t d = depthStart; d < depthEnd; ++d)
	{
		CachedOverlaps* cachedOverlaps = DYNAMIC_CAST(CachedOverlaps*)(mParams->overlapsAtDepth.buf[d]);
		NvParameterized::Handle handle(*cachedOverlaps);
		cachedOverlaps->getParameterHandle("overlaps", handle);
		cachedOverlaps->resizeArray(handle, 0);
		if (!keepCachedFlag)
		{
			cachedOverlaps->isCached = false;
		}
	}
}


void DestructibleAssetImpl::addChunkOverlaps(IntPair* supportGraphEdges, uint32_t supportGraphEdgeCount)
{
	if (supportGraphEdgeCount == 0)
		return;

	uint32_t numChunks = (uint32_t)mParams->chunks.arraySizes[0];

	Array< Array<IntPair> > overlapsAtDepth(mParams->depthCount);

	// store symmetric pairs at corresponding depth
	for (uint32_t i = 0; i < supportGraphEdgeCount; ++i)
	{
		uint32_t chunkIndex0 = (uint32_t)supportGraphEdges[i].i0;
		uint32_t chunkIndex1 = (uint32_t)supportGraphEdges[i].i1;

		if (chunkIndex0 >= numChunks || chunkIndex1 >= numChunks)
		{
			APEX_DEBUG_WARNING("Edge %i supportGraphEdges has indices (%i,%i), but only a total of %i chunks are provided. supportEdges will be ignored.", i, chunkIndex0, chunkIndex1, numChunks);
			overlapsAtDepth.clear();
			break;
		}

		const DestructibleAssetParametersNS::Chunk_Type& chunk0 = mParams->chunks.buf[chunkIndex0];
		const DestructibleAssetParametersNS::Chunk_Type& chunk1 = mParams->chunks.buf[chunkIndex1];

		if (chunk0.depth != chunk1.depth)
		{
			APEX_DEBUG_WARNING("Support graph can only have edges between sibling chunks. supportEdges will be ignored.");
			overlapsAtDepth.clear();
			break;
		}

		PX_ASSERT(chunk0.depth < mParams->depthCount);

		IntPair pair;
		pair.i0 = (int32_t)chunkIndex0;
		pair.i1 = (int32_t)chunkIndex1;
		overlapsAtDepth[chunk0.depth].pushBack(pair);

		pair.i0 = (int32_t)chunkIndex1;
		pair.i1 = (int32_t)chunkIndex0;
		overlapsAtDepth[chunk0.depth].pushBack(pair);
	}

	if (overlapsAtDepth.size() == 0)
		return;


	// for each depth
	for (uint32_t depth = 0; depth < overlapsAtDepth.size(); ++depth)
	{
		if (overlapsAtDepth[depth].size() > 0)
		{
			// sort overlaps pairs
			qsort(&overlapsAtDepth[depth][0], overlapsAtDepth[depth].size(), sizeof(IntPair), IntPair::compare);
		}

		if (overlapsAtDepth[depth].size() == 0)
			continue;

		// resize parameterized array
		uint32_t numCachedDepths = (uint32_t)mParams->overlapsAtDepth.arraySizes[0];
		if (depth >= numCachedDepths)
		{
			NvParameterized::Handle handle(*mParams);
			mParams->getParameterHandle("overlapsAtDepth", handle);
			mParams->resizeArray(handle, (int32_t)depth+1);

			NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
			for (uint32_t d = numCachedDepths; d < depth+1; ++d)
			{
				CachedOverlaps* cachedOverlaps = DYNAMIC_CAST(CachedOverlaps*)(traits->createNvParameterized(CachedOverlaps::staticClassName()));
				mParams->overlapsAtDepth.buf[d] = cachedOverlaps;
				cachedOverlaps->isCached = false;
			}
		}

		CachedOverlaps* cachedOverlapsAtDepth = DYNAMIC_CAST(CachedOverlaps*)(mParams->overlapsAtDepth.buf[depth]);
		NvParameterized::Handle overlapsHandle(*cachedOverlapsAtDepth);
		cachedOverlapsAtDepth->getParameterHandle("overlaps", overlapsHandle);
		int32_t oldSize = cachedOverlapsAtDepth->overlaps.arraySizes[0];
		overlapsHandle.resizeArray(cachedOverlapsAtDepth->overlaps.arraySizes[0] + (int32_t)overlapsAtDepth[depth].size());

		// merge new pairs into existing graph
		bool ok = ApexMerge<IntPair>(	(IntPair*)cachedOverlapsAtDepth->overlaps.buf, (uint32_t)oldSize,
										&overlapsAtDepth[depth][0], overlapsAtDepth[depth].size(),
										(IntPair*)cachedOverlapsAtDepth->overlaps.buf, (uint32_t)cachedOverlapsAtDepth->overlaps.arraySizes[0],
										IntPair::compare);

		PX_UNUSED(ok);
		PX_ASSERT(ok);

		// check for duplicates
		if (cachedOverlapsAtDepth->overlaps.arraySizes[0] > 1)
		{
			Array<uint32_t> toRemove;
			for (uint32_t j = 1; j < (uint32_t)cachedOverlapsAtDepth->overlaps.arraySizes[0]; ++j)
			{
				if (cachedOverlapsAtDepth->overlaps.buf[j].i1 == cachedOverlapsAtDepth->overlaps.buf[j-1].i1 &&
					cachedOverlapsAtDepth->overlaps.buf[j].i0 == cachedOverlapsAtDepth->overlaps.buf[j-1].i0)
				{
					toRemove.pushBack(j);
				}
			}

			// remove duplicates
			toRemove.pushBack((uint32_t)cachedOverlapsAtDepth->overlaps.arraySizes[0]); // add guard
			uint32_t shift = 0;
			for (uint32_t j = 0; j < toRemove.size()-1; ++j)
			{
				++shift;
				for (uint32_t index = toRemove[j]+1; index < toRemove[j+1]; ++index)
				{
					cachedOverlapsAtDepth->overlaps.buf[index - shift] = cachedOverlapsAtDepth->overlaps.buf[index];
				}
			}
			NvParameterized::Handle overlapsHandle(*cachedOverlapsAtDepth);
			cachedOverlapsAtDepth->getParameterHandle("overlaps", overlapsHandle);
			overlapsHandle.resizeArray(cachedOverlapsAtDepth->overlaps.arraySizes[0] - (int32_t)shift);
		}

		cachedOverlapsAtDepth->isCached = cachedOverlapsAtDepth->overlaps.arraySizes[0] > 0;
	}
}


void DestructibleAssetImpl::removeChunkOverlaps(IntPair* supportGraphEdges, uint32_t numSupportGraphEdges, bool keepCachedFlagIfEmpty)
{
	Array< Array<uint32_t> > toRemoveAtDepth(mParams->depthCount);
	for (uint32_t i = 0; i < numSupportGraphEdges; ++i)
	{
		CachedOverlapsNS::IntPair_Type& pair = (CachedOverlapsNS::IntPair_Type&)supportGraphEdges[i];
		
		if (pair.i0 >= mParams->chunks.arraySizes[0] || pair.i1 >= mParams->chunks.arraySizes[0])
			continue;

		const DestructibleAssetParametersNS::Chunk_Type& chunk0 = mParams->chunks.buf[pair.i0];
		const DestructibleAssetParametersNS::Chunk_Type& chunk1 = mParams->chunks.buf[pair.i1];

		if (chunk0.depth != chunk1.depth)
			continue;

		if (chunk0.depth >= mParams->overlapsAtDepth.arraySizes[0])
			continue;

		if (chunk0.depth >= mParams->depthCount)
			continue;

		CachedOverlaps* cachedOverlapsAtDepth = DYNAMIC_CAST(CachedOverlaps*)(mParams->overlapsAtDepth.buf[chunk0.depth]);

		// binary search for pair and add add to index to removal list
		int32_t index = ApexFind(cachedOverlapsAtDepth->overlaps.buf, (uint32_t)cachedOverlapsAtDepth->overlaps.arraySizes[0], pair, IntPair::compare);
		if (index != -1)
		{
			toRemoveAtDepth[chunk0.depth].pushBack((uint32_t)index);
		}

		CachedOverlapsNS::IntPair_Type symmetricPair;
		symmetricPair.i0 = pair.i1;
		symmetricPair.i1 = pair.i0;
		index = ApexFind(cachedOverlapsAtDepth->overlaps.buf, (uint32_t)cachedOverlapsAtDepth->overlaps.arraySizes[0], symmetricPair, IntPair::compare);
		if (index != -1)
		{
			toRemoveAtDepth[chunk0.depth].pushBack((uint32_t)index);
		}
	}

	// go through removal list of each depth and shift remaining entries to overwrite the removed ones
	for (uint32_t depth = 0; depth < toRemoveAtDepth.size(); ++depth)
	{
		CachedOverlaps* cachedOverlapsAtDepth = DYNAMIC_CAST(CachedOverlaps*)(mParams->overlapsAtDepth.buf[depth]);
		toRemoveAtDepth[depth].pushBack((uint32_t)cachedOverlapsAtDepth->overlaps.arraySizes[0]); // add guard
		uint32_t shift = 0;
		for (uint32_t j = 1; j < toRemoveAtDepth[depth].size(); ++j)
		{
			++shift;
			for (uint32_t index = toRemoveAtDepth[depth][j-1]+1; index < toRemoveAtDepth[depth][j]; ++index)
			{
				cachedOverlapsAtDepth->overlaps.buf[index - shift] = cachedOverlapsAtDepth->overlaps.buf[index];
			}
		}

		NvParameterized::Handle overlapsHandle(*cachedOverlapsAtDepth);
		cachedOverlapsAtDepth->getParameterHandle("overlaps", overlapsHandle);
		overlapsHandle.resizeArray(cachedOverlapsAtDepth->overlaps.arraySizes[0] - (int32_t)shift);

		if (!keepCachedFlagIfEmpty)
		{
			cachedOverlapsAtDepth->isCached = cachedOverlapsAtDepth->overlaps.arraySizes[0] > 0;
		}
	}
}


DestructibleAssetImpl::DestructibleAssetImpl(ModuleDestructibleImpl* inModule, DestructibleAsset* api, const char* name) :
	mCrumbleAssetTracker(inModule->mSdk, EMITTER_AUTHORING_TYPE_NAME),
	mDustAssetTracker(inModule->mSdk, EMITTER_AUTHORING_TYPE_NAME),
	m_instancedChunkMeshCount(0)
{
	mApexDestructibleActorParams = 0;
	init();

	module = inModule;
	mNxAssetApi  = api;
	mName = name;

	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
	mParams = DYNAMIC_CAST(DestructibleAssetParameters*)(traits->createNvParameterized(DestructibleAssetParameters::staticClassName()));
	mOwnsParams = mParams != NULL;
	PX_ASSERT(mOwnsParams);
}

DestructibleAssetImpl::DestructibleAssetImpl(ModuleDestructibleImpl* inModule, DestructibleAsset* api, NvParameterized::Interface* params, const char* name) 
	: mCrumbleAssetTracker(inModule->mSdk, EMITTER_AUTHORING_TYPE_NAME)
	, mDustAssetTracker(inModule->mSdk, EMITTER_AUTHORING_TYPE_NAME)
	, mRuntimeCookedConvexCount(0)
	, m_instancedChunkMeshCount(0)
{
	mApexDestructibleActorParams = 0;
	init();

	module = inModule;
	mNxAssetApi  = api;
	mName = name;

	mParams = DYNAMIC_CAST(DestructibleAssetParameters*)(params);

	// The pattern for NvParameterized assets is that the params pointer now belongs to the asset
	mOwnsParams = true;

	// there's no deserialize, so init the ARMs
	if (mParams->renderMeshAsset)
	{
		ApexSimpleString meshName = mName + ApexSimpleString("RenderMesh");
		module->mSdk->getInternalResourceProvider()->generateUniqueName(module->mSdk->getApexMeshNameSpace(), meshName);

		setRenderMeshAsset(static_cast<RenderMeshAsset*>(module->mSdk->createAsset(mParams->renderMeshAsset,  meshName.c_str())));
	}

	// scatter meshes
	bool scatterMeshAssetsValid = true;
	physx::Array<RenderMeshAsset*> scatterMeshAssetArray((uint32_t)mParams->scatterMeshAssets.arraySizes[0]);
	for (uint32_t i = 0; i < (uint32_t)mParams->scatterMeshAssets.arraySizes[0]; ++i)
	{
		if (i > 65535 || mParams->scatterMeshAssets.buf[i] == NULL)
		{
			scatterMeshAssetsValid = false;
			break;
		}
		char suffix[20];
		sprintf(suffix, "ScatterMesh%d", i);
		ApexSimpleString meshName = mName + ApexSimpleString(suffix);
		module->mSdk->getInternalResourceProvider()->generateUniqueName(module->mSdk->getApexMeshNameSpace(), meshName);
		scatterMeshAssetArray[i] = static_cast<RenderMeshAsset*>(module->mSdk->createAsset(mParams->scatterMeshAssets.buf[i], meshName.c_str()));
	}

	bool success = false;
	if (scatterMeshAssetsValid && mParams->scatterMeshAssets.arraySizes[0] > 0)
	{
		success = setScatterMeshAssets(&scatterMeshAssetArray[0], (uint32_t)mParams->scatterMeshAssets.arraySizes[0]);
	}
	if (!success)
	{
		for (uint32_t i = 0; i < scatterMeshAssetArray.size(); ++i)
		{
			if (scatterMeshAssetArray[i] != NULL)
			{
				scatterMeshAssetArray[i]->release();
			}
		}
	}

	bool hullWarningGiven = false;

	// Connect contained classes to referenced parameters
	chunkConvexHulls.resize((uint32_t)mParams->chunkConvexHulls.arraySizes[0]);
	for (uint32_t i = 0; i < chunkConvexHulls.size(); ++i)
	{
		chunkConvexHulls[i].init(mParams->chunkConvexHulls.buf[i]);

		// Fix convex hulls to account for adjacentFaces bug
		if (chunkConvexHulls[i].mParams->adjacentFaces.arraySizes[0] != chunkConvexHulls[i].mParams->edges.arraySizes[0])
		{
			chunkConvexHulls[i].buildFromPoints(chunkConvexHulls[i].mParams->vertices.buf, (uint32_t)chunkConvexHulls[i].mParams->vertices.arraySizes[0], 
				(uint32_t)chunkConvexHulls[i].mParams->vertices.elementSize);
			if (!hullWarningGiven)
			{
				GetInternalApexSDK()->reportError(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, __FUNCTION__,
					"Chunk convex hull data bad in asset %s, rebuilding.  Asset should be re-exported.", name);
				hullWarningGiven = true;
			}
		}
	}

	m_currentInstanceBufferActorAllowance = mParams->initialDestructibleActorAllowanceForInstancing;

	physx::Array<uint16_t> tempPartToActorMap;
	tempPartToActorMap.resize(renderMeshAsset->getPartCount(), 0xFFFF);

	m_instancedChunkMeshCount = 0;

	m_instancedChunkActorMap.resize((uint32_t)mParams->chunkInstanceInfo.arraySizes[0]);
	for (uint32_t i = 0; i < (uint32_t)mParams->chunkInstanceInfo.arraySizes[0]; ++i)
	{
		uint16_t partIndex = mParams->chunkInstanceInfo.buf[i].partIndex;
		if (tempPartToActorMap[partIndex] == 0xFFFF)
		{
			tempPartToActorMap[partIndex] = m_instancedChunkMeshCount++;
		}
		m_instancedChunkActorMap[i] = tempPartToActorMap[partIndex];
	}

	m_instancedChunkActorVisiblePart.resize(m_instancedChunkMeshCount);
	for (uint32_t i = 0; i < (uint32_t)mParams->chunks.arraySizes[0]; ++i)
	{
		DestructibleAssetParametersNS::Chunk_Type& chunk = mParams->chunks.buf[i];
		if ((chunk.flags & DestructibleAssetImpl::Instanced) != 0)
		{
			uint16_t partIndex = mParams->chunkInstanceInfo.buf[chunk.meshPartIndex].partIndex;
			m_instancedChunkActorVisiblePart[m_instancedChunkActorMap[chunk.meshPartIndex]] = partIndex;
		}
	}

	m_instancingRepresentativeActorIndex = -1;	// not set

	reduceAccordingToLOD();

	initializeAssetNameTable();

	mStaticMaterialIDs.resize((uint32_t)mParams->staticMaterialNames.arraySizes[0]);
	ResourceProviderIntl* resourceProvider = GetInternalApexSDK()->getInternalResourceProvider();
	ResID materialNS = GetInternalApexSDK()->getMaterialNameSpace();
	// Resolve material names using the NRP...
	for (uint32_t i = 0; i < (uint32_t)mParams->staticMaterialNames.arraySizes[0]; ++i)
	{
		if (resourceProvider)
		{
			mStaticMaterialIDs[i] = resourceProvider->createResource(materialNS, mParams->staticMaterialNames.buf[i]);
		}
		else
		{
			mStaticMaterialIDs[i] = INVALID_RESOURCE_ID;
		}
	}
}

DestructibleAssetImpl::~DestructibleAssetImpl()
{
	// Release named resources
	ResourceProviderIntl* resourceProvider = GetInternalApexSDK()->getInternalResourceProvider();
	for (uint32_t i = 0 ; i < mStaticMaterialIDs.size() ; i++)
	{
		resourceProvider->releaseResource(mStaticMaterialIDs[i]);
	}

	if (mParams != NULL && mOwnsParams)
	{
		mParams->destroy();
	}
	mParams = NULL;
	mOwnsParams = false;

	if (mApexDestructibleActorParams)
	{
		mApexDestructibleActorParams->destroy();
		mApexDestructibleActorParams = 0;
	}
	/* Assets that were forceloaded or loaded by actors will be automatically
	 * released by the ApexAssetTracker member destructors.
	 */
}

void DestructibleAssetImpl::init()
{
	module = NULL;
	chunkOverlapCacheDepth = -1;
	renderMeshAsset = NULL;
	runtimeRenderMeshAsset = NULL;
	mCollisionMeshes = NULL;
	m_currentInstanceBufferActorAllowance = 0;
	m_needsInstanceBufferDataResize = false;
	m_needsInstanceBufferResize = false;
	m_needsScatterMeshInstanceInfoCreation = false;
}

uint32_t DestructibleAssetImpl::forceLoadAssets()
{
	uint32_t assetLoadedCount = 0;

	assetLoadedCount += mCrumbleAssetTracker.forceLoadAssets();
	assetLoadedCount += mDustAssetTracker.forceLoadAssets();

	if (renderMeshAsset != NULL)
	{
		assetLoadedCount += renderMeshAsset->forceLoadAssets();
	}

	ResourceProviderIntl* nrp = GetInternalApexSDK()->getInternalResourceProvider();
	ResID materialNS = GetInternalApexSDK()->getMaterialNameSpace();
	for (uint32_t i = 0; i < mStaticMaterialIDs.size(); i++)
	{
		if (!nrp->checkResource(materialNS, mParams->staticMaterialNames.buf[i]))
		{
			/* we know for SURE that createResource() has already been called, so just getResource() */
			nrp->getResource(mStaticMaterialIDs[i]);
			assetLoadedCount++;
		}
	}

	return assetLoadedCount;
}

void DestructibleAssetImpl::initializeAssetNameTable()
{
	if (mParams->dustEmitterName && *mParams->dustEmitterName)
	{
		mDustAssetTracker.addAssetName(mParams->dustEmitterName, false);
	}

	if (mParams->crumbleEmitterName && *mParams->crumbleEmitterName)
	{
		mCrumbleAssetTracker.addAssetName(mParams->crumbleEmitterName, false);
	}
}

void DestructibleAssetImpl::cleanup()
{
	// Release internal RenderMesh, preview instances, and authoring instance

	while (m_previewList.getSize())
	{
		DestructiblePreviewProxy* proxy = DYNAMIC_CAST(DestructiblePreviewProxy*)(m_previewList.getResource(m_previewList.getSize() - 1));
		PX_ASSERT(proxy != NULL);
		if (proxy == NULL)
		{
			m_previewList.remove(m_previewList.getSize() - 1);	// To avoid an infinite loop
		}
		else
		{
			proxy->release();
		}
	}

	m_previewList.clear();
	m_destructibleList.clear();

	setRenderMeshAsset(NULL);

	// release chunk instance render resources
	m_chunkInstanceBufferDataLock.lock();
	m_needsInstanceBufferResize = false;
	m_chunkInstanceBufferData.clear();
	updateChunkInstanceRenderResources(false, NULL);
	m_chunkInstanceBufferDataLock.unlock();

	setScatterMeshAssets(NULL, 0);

	m_instancedChunkActorMap.resize(0);
	m_instancedChunkActorVisiblePart.resize(0);

	if (module->mCachedData != NULL)
	{
		module->mCachedData->clearAssetCollisionSet(*this);
	}
}

void DestructibleAssetImpl::prepareForNewInstance()
{
	if (m_currentInstanceBufferActorAllowance < m_destructibleList.getSize() + 1)	// Add 1 to predict new actor
	{
		// This loop should only be hit once
		do
		{
			m_currentInstanceBufferActorAllowance = m_currentInstanceBufferActorAllowance > 0 ? 2*m_currentInstanceBufferActorAllowance : 1;
			m_needsInstanceBufferDataResize = true;
		}
		while (m_currentInstanceBufferActorAllowance < m_destructibleList.getSize());	// Add 1 to predict new actor
	}
}



void DestructibleAssetImpl::resetInstanceData()
{
	PX_PROFILE_ZONE("DestructibleAsset::resetInstanceData", GetInternalApexSDK()->getContextId());

	m_chunkInstanceBufferDataLock.lock();
	m_chunkInstanceBufferData.resize(m_instancedChunkMeshCount);
	if (m_needsInstanceBufferDataResize)
	{
		//
		// reserve the right amount of memory in the per chunk mesh arrays
		// 
		for (uint32_t index = 0; index < m_instancedChunkMeshCount; ++index)
		{
			if (m_currentInstanceBufferActorAllowance > 0)
			{
				// Find out how many potential instances there are
				uint32_t maxInstanceCount = 0;
				for (int32_t i = 0; i < mParams->chunkInstanceInfo.arraySizes[0]; ++i)
				{
					if (mParams->chunkInstanceInfo.buf[i].partIndex == m_instancedChunkActorVisiblePart[index])
					{
						maxInstanceCount += m_currentInstanceBufferActorAllowance;
					}
				}

				// Instance buffer data
				m_chunkInstanceBufferData[index].reserve(maxInstanceCount);
			}
			
			m_chunkInstanceBufferData[index].resize(0);
		}

		m_needsInstanceBufferDataResize = false;
		m_needsInstanceBufferResize = true;
	}
	else
	{
		for (uint32_t j = 0; j < m_chunkInstanceBufferData.size(); ++j)
		{
			m_chunkInstanceBufferData[j].resize(0);
		}
	}
	m_chunkInstanceBufferDataLock.unlock();


	for (uint32_t j = 0; j < m_scatterMeshInstanceInfo.size(); ++j)
	{
		m_scatterMeshInstanceInfo[j].m_instanceBufferData.resize(0);
	}
	m_instancingRepresentativeActorIndex = -1;	// not set
}


template <class ParamType>
DestructibleActor* createDestructibleActorImpl(ParamType& params, 
												 DestructibleAssetImpl& destructibleAsset,
												 ResourceList& destructibleList,
												 DestructibleScene* destructibleScene)
{
	if (NULL == destructibleScene)
		return NULL;

	destructibleAsset.prepareForNewInstance();

	return PX_NEW(DestructibleActorProxy)(params, destructibleAsset, destructibleList, *destructibleScene);
}

DestructibleActor* DestructibleAssetImpl::createDestructibleActorFromDeserializedState(NvParameterized::Interface* params, Scene& scene)
{
	PX_PROFILE_ZONE("DestructibleCreateActor", GetInternalApexSDK()->getContextId());

	if (NULL == params || !isValidForActorCreation(*params, scene))
		return NULL;

	return createDestructibleActorImpl(params, *this, m_destructibleList, module->getDestructibleScene(scene));
}

DestructibleActor* DestructibleAssetImpl::createDestructibleActor(const NvParameterized::Interface& params, Scene& scene)
{
	PX_PROFILE_ZONE("DestructibleCreateActor", GetInternalApexSDK()->getContextId());

	return createDestructibleActorImpl(params, *this, m_destructibleList, module->getDestructibleScene(scene));
}

void DestructibleAssetImpl::releaseDestructibleActor(DestructibleActor& nxactor)
{
	DestructibleActorProxy* proxy = DYNAMIC_CAST(DestructibleActorProxy*)(&nxactor);
	proxy->destroy();
}

bool DestructibleAssetImpl::setRenderMeshAsset(RenderMeshAsset* newRenderMeshAsset)
{
	if (newRenderMeshAsset == renderMeshAsset)
	{
		return false;
	}

	for (uint32_t i = 0; i < m_instancedChunkRenderMeshActors.size(); ++i)
	{
		if (m_instancedChunkRenderMeshActors[i] != NULL)
		{
			m_instancedChunkRenderMeshActors[i]->release();
			m_instancedChunkRenderMeshActors[i] = NULL;
		}
	}

	if (renderMeshAsset != NULL)
	{
		if(mOwnsParams && mParams != NULL)
		{
			// set isReferenced to false, so that the parameterized object
			// for the render mesh asset is destroyed in renderMeshAsset->release
			NvParameterized::ErrorType e;
			if (mParams->renderMeshAsset != NULL)
			{
				NvParameterized::Handle h(*mParams->renderMeshAsset);
				e = mParams->renderMeshAsset->getParameterHandle("isReferenced", h);
				PX_ASSERT(e == NvParameterized::ERROR_NONE);
				if (e == NvParameterized::ERROR_NONE)
				{
					h.setParamBool(false);
				}
				mParams->renderMeshAsset = NULL;
			}
		}
		renderMeshAsset->release();
	}

	renderMeshAsset = newRenderMeshAsset;
	if (renderMeshAsset != NULL)
	{
		mParams->renderMeshAsset = (NvParameterized::Interface*)renderMeshAsset->getAssetNvParameterized();
		NvParameterized::ErrorType e;
		NvParameterized::Handle h(*mParams->renderMeshAsset);
		e = mParams->renderMeshAsset->getParameterHandle("isReferenced", h);
		PX_ASSERT(e == NvParameterized::ERROR_NONE);
		if (e == NvParameterized::ERROR_NONE)
		{
			h.setParamBool(true);
		}

		for (uint32_t i = 0; i < m_instancedChunkRenderMeshActors.size(); ++i)
		{
			// Create actor
			RenderMeshActorDesc renderableMeshDesc;
			renderableMeshDesc.maxInstanceCount = m_chunkInstanceBufferData[i].capacity();
			renderableMeshDesc.renderWithoutSkinning = true;
			renderableMeshDesc.visible = false;
			m_instancedChunkRenderMeshActors[i] = newRenderMeshAsset->createActor(renderableMeshDesc);
			m_instancedChunkRenderMeshActors[i]->setInstanceBuffer(m_chunkInstanceBuffers[i]);
			m_instancedChunkRenderMeshActors[i]->setVisibility(true, m_instancedChunkActorVisiblePart[i]);
			m_instancedChunkRenderMeshActors[i]->setReleaseResourcesIfNothingToRender(false);
		}
	}

	return true;
}

bool DestructibleAssetImpl::setScatterMeshAssets(RenderMeshAsset** scatterMeshAssetArray, uint32_t scatterMeshAssetArraySize)
{
	if (scatterMeshAssetArray == NULL && scatterMeshAssetArraySize > 0)
	{
		return false;
	}

	for (uint32_t i = 0; i < scatterMeshAssetArraySize; ++i)
	{
		if (scatterMeshAssetArray[i] == NULL)
		{
			return false;
		}
	}

	// First clear instance information
	m_scatterMeshInstanceInfo.resize(0);	// Ensure we delete all instanced actors
	m_scatterMeshInstanceInfo.resize(scatterMeshAssetArraySize);

	// Clear out scatter mesh assets, including parameterized data
	for (int32_t i = 0; mParams && (i < mParams->scatterMeshAssets.arraySizes[0]); ++i)
	{
		if (mParams->scatterMeshAssets.buf[i] != NULL)
		{
			NvParameterized::ErrorType e;
			NvParameterized::Handle h(*mParams->scatterMeshAssets.buf[i]);
			e = mParams->scatterMeshAssets.buf[i]->getParameterHandle("isReferenced", h);
			PX_ASSERT(e == NvParameterized::ERROR_NONE);
			if (e == NvParameterized::ERROR_NONE)
			{
				h.setParamBool(false);
			}
			mParams->scatterMeshAssets.buf[i] = NULL;
		}
	}

	for (uint32_t i = 0; i < scatterMeshAssets.size(); ++i)
	{
		if (scatterMeshAssets[i] != NULL)
		{
			scatterMeshAssets[i]->release();
			scatterMeshAssets[i] = NULL;
		}
	}

	if (mParams != NULL)
	{
	scatterMeshAssets.resize(scatterMeshAssetArraySize, NULL);
	NvParameterized::Handle h(*mParams, "scatterMeshAssets");
	h.resizeArray((int32_t)scatterMeshAssetArraySize);

	for (uint32_t i = 0; i < scatterMeshAssetArraySize; ++i)
	{
		// Create new asset
		scatterMeshAssets[i] = scatterMeshAssetArray[i];
		mParams->scatterMeshAssets.buf[i] = (NvParameterized::Interface*)scatterMeshAssets[i]->getAssetNvParameterized();
		NvParameterized::ErrorType e;
		NvParameterized::Handle h(*mParams->scatterMeshAssets.buf[i]);
		e = mParams->scatterMeshAssets.buf[i]->getParameterHandle("isReferenced", h);
		PX_ASSERT(e == NvParameterized::ERROR_NONE);
		if (e == NvParameterized::ERROR_NONE)
		{
			h.setParamBool(true);
		}
	}

	m_needsScatterMeshInstanceInfoCreation = true;
	}


	return true;
}

void DestructibleAssetImpl::createScatterMeshInstanceInfo()
{
	if (!m_needsScatterMeshInstanceInfoCreation)
		return;

	m_needsScatterMeshInstanceInfoCreation = false;

	UserRenderResourceManager* rrm = GetInternalApexSDK()->getUserRenderResourceManager();

	physx::Array<uint32_t> totalInstanceCounts(scatterMeshAssets.size(), 0);
	for (int32_t i = 0; i < mParams->scatterMeshIndices.arraySizes[0]; ++i)
	{
		const uint8_t scatterMeshIndex = mParams->scatterMeshIndices.buf[i];
		if (scatterMeshIndex < scatterMeshAssets.size())
		{
			++totalInstanceCounts[scatterMeshIndex];
		}
	}

	for (uint32_t i = 0; i < scatterMeshAssets.size(); ++i)
	{
		// Create instanced info
		ScatterMeshInstanceInfo& info = m_scatterMeshInstanceInfo[i];
		RenderMeshActorDesc renderableMeshDesc;
		renderableMeshDesc.maxInstanceCount = totalInstanceCounts[i];
		renderableMeshDesc.renderWithoutSkinning = true;
		renderableMeshDesc.visible = true;
		info.m_actor = scatterMeshAssets[i]->createActor(renderableMeshDesc);

		// Create instance buffer
		info.m_instanceBuffer = NULL;
		info.m_IBSize = totalInstanceCounts[i];
		if (totalInstanceCounts[i] > 0)
		{
			UserRenderInstanceBufferDesc instanceBufferDesc = getScatterMeshInstanceBufferDesc();
			instanceBufferDesc.maxInstances = totalInstanceCounts[i];
			info.m_instanceBuffer = rrm->createInstanceBuffer(instanceBufferDesc);
		}

		// Instance buffer data
		info.m_instanceBufferData.reset();
		info.m_instanceBufferData.reserve(totalInstanceCounts[i]);

		info.m_actor->setInstanceBuffer(info.m_instanceBuffer);
		info.m_actor->setReleaseResourcesIfNothingToRender(false);
	}
}

UserRenderInstanceBufferDesc DestructibleAssetImpl::getScatterMeshInstanceBufferDesc()
{
	UserRenderInstanceBufferDesc instanceBufferDesc;
	instanceBufferDesc.hint = RenderBufferHint::DYNAMIC;
	instanceBufferDesc.semanticOffsets[RenderInstanceLayoutElement::POSITION_FLOAT3] = ScatterInstanceBufferDataElement::translationOffset();
	instanceBufferDesc.semanticOffsets[RenderInstanceLayoutElement::ROTATION_SCALE_FLOAT3x3] = ScatterInstanceBufferDataElement::scaledRotationOffset();
	instanceBufferDesc.semanticOffsets[RenderInstanceLayoutElement::DENSITY_FLOAT1] = ScatterInstanceBufferDataElement::alphaOffset();
	instanceBufferDesc.stride = sizeof(ScatterInstanceBufferDataElement);

	return instanceBufferDesc;
}

void DestructibleAssetImpl::updateChunkInstanceRenderResources(bool rewriteBuffers, void* userRenderData)
{
	PX_PROFILE_ZONE("DestructibleAsset::updateChunkInstanceRenderResources", GetInternalApexSDK()->getContextId());

	UserRenderResourceManager* rrm = GetInternalApexSDK()->getUserRenderResourceManager();

	Mutex::ScopedLock scopeLock(m_chunkInstanceBufferDataLock);

	uint32_t oldCount = m_chunkInstanceBuffers.size();
	uint32_t count = m_chunkInstanceBufferData.size();

	//
	// release resources
	//
	// release all on resize for recreation lateron
	uint32_t startIndexForRelease = m_needsInstanceBufferResize ? 0 : count;
	for (uint32_t i = startIndexForRelease; i < oldCount; ++i)
	{
		if (m_instancedChunkRenderMeshActors[i] != NULL)
		{
			m_instancedChunkRenderMeshActors[i]->release();
			m_instancedChunkRenderMeshActors[i] = NULL;
		}
		if (m_chunkInstanceBuffers[i] != NULL)
		{
			rrm->releaseInstanceBuffer(*m_chunkInstanceBuffers[i]);
			m_chunkInstanceBuffers[i] = NULL;
		}
	}

	// resize and init arrays
	m_chunkInstanceBuffers.resize(count);
	m_instancedChunkRenderMeshActors.resize(count);
	for (uint32_t index = oldCount; index < count; ++index)
	{
		m_instancedChunkRenderMeshActors[index] = NULL;
		m_chunkInstanceBuffers[index] = NULL;
	}


	//
	// create resources when needed
	//

	for (uint32_t index = 0; index < count; ++index)
	{
		// if m_chunkInstanceBufferData[index] contains any data there's an instance to render
		if (m_chunkInstanceBuffers[index] == NULL && m_chunkInstanceBufferData[index].size() > 0)
		{
			// Find out how many potential instances there are
			uint32_t maxInstanceCount = 0;
			for (int32_t i = 0; i < mParams->chunkInstanceInfo.arraySizes[0]; ++i)
			{
				if (mParams->chunkInstanceInfo.buf[i].partIndex == m_instancedChunkActorVisiblePart[index])
				{
					maxInstanceCount += m_currentInstanceBufferActorAllowance;
				}
			}

			// Create instance buffer
			UserRenderInstanceBufferDesc instanceBufferDesc;
			instanceBufferDesc.maxInstances = maxInstanceCount;
			instanceBufferDesc.hint = RenderBufferHint::DYNAMIC;
			instanceBufferDesc.semanticOffsets[RenderInstanceLayoutElement::POSITION_FLOAT3] = ChunkInstanceBufferDataElement::translationOffset();
			instanceBufferDesc.semanticOffsets[RenderInstanceLayoutElement::ROTATION_SCALE_FLOAT3x3] = ChunkInstanceBufferDataElement::scaledRotationOffset();
			instanceBufferDesc.semanticOffsets[RenderInstanceLayoutElement::UV_OFFSET_FLOAT2] = ChunkInstanceBufferDataElement::uvOffsetOffset();
			instanceBufferDesc.semanticOffsets[RenderInstanceLayoutElement::LOCAL_OFFSET_FLOAT3] = ChunkInstanceBufferDataElement::localOffsetOffset();
			instanceBufferDesc.stride = sizeof(ChunkInstanceBufferDataElement);
			m_chunkInstanceBuffers[index] = rrm->createInstanceBuffer(instanceBufferDesc);

			// Create actor
			if (renderMeshAsset != NULL)
			{
				PX_ASSERT(m_instancedChunkRenderMeshActors[index] == NULL);

				RenderMeshActorDesc renderableMeshDesc;
				renderableMeshDesc.maxInstanceCount = maxInstanceCount;
				renderableMeshDesc.renderWithoutSkinning = true;
				renderableMeshDesc.visible = false;
				m_instancedChunkRenderMeshActors[index] = renderMeshAsset->createActor(renderableMeshDesc);
				m_instancedChunkRenderMeshActors[index]->setInstanceBuffer(m_chunkInstanceBuffers[index]);
				m_instancedChunkRenderMeshActors[index]->setVisibility(true, m_instancedChunkActorVisiblePart[index]);
				m_instancedChunkRenderMeshActors[index]->setReleaseResourcesIfNothingToRender(false);
			}
		}

		//
		// update with new data
		//
		PX_ASSERT(index < m_chunkInstanceBufferData.size());
		RenderMeshActorIntl* renderMeshActor = (RenderMeshActorIntl*)m_instancedChunkRenderMeshActors[index];
		if (renderMeshActor != NULL)
		{
			RenderInstanceBufferData data;
			const uint32_t instanceBufferSize = m_chunkInstanceBufferData[index].size();

			if (instanceBufferSize > 0)
			{
				m_chunkInstanceBuffers[index]->writeBuffer(&m_chunkInstanceBufferData[index][0], 0, instanceBufferSize);
			}

			renderMeshActor->setInstanceBufferRange(0, instanceBufferSize);
			renderMeshActor->updateRenderResources(false, rewriteBuffers, userRenderData);
		}
	}

	m_needsInstanceBufferResize = false;
}

bool DestructibleAssetImpl::setPlatformMaxDepth(PlatformTag platform, uint32_t maxDepth)
{
	bool isExistingPlatform = false;
	for (Array<PlatformKeyValuePair>::Iterator iter = m_platformFractureDepthMap.begin(); iter != m_platformFractureDepthMap.end(); ++iter)
	{
		if (nvidia::strcmp(iter->key, platform) == 0)
		{
			isExistingPlatform = true;
			iter->val = maxDepth; //overwrite if existing
			break;
		}
	}
	if (!isExistingPlatform)
	{
		m_platformFractureDepthMap.pushBack(PlatformKeyValuePair(platform, maxDepth));
	}
	return maxDepth < mParams->depthCount - 1; //depthCount == 1 => unfractured mesh
}

bool DestructibleAssetImpl::removePlatformMaxDepth(PlatformTag platform)
{
	bool isExistingPlatform = false;
	for (uint32_t index = 0; index < m_platformFractureDepthMap.size(); ++index)
	{
		if (nvidia::strcmp(m_platformFractureDepthMap[index].key, platform) == 0)
		{
			isExistingPlatform = true;
			m_platformFractureDepthMap.remove(index); //yikes! for lack of a map...
			break;
		}
	}
	return isExistingPlatform;
}

bool DestructibleAssetImpl::setDepthCount(uint32_t targetDepthCount) const
{
	if (mParams->depthCount <= targetDepthCount)
	{
		return false;
	}

	int newChunkCount = mParams->chunks.arraySizes[0];
	for (int i = newChunkCount; i--;)
	{
		if (mParams->chunks.buf[i].depth >= targetDepthCount)
		{
			newChunkCount = i;
		}
		else if (mParams->chunks.buf[i].depth == targetDepthCount - 1)
		{
			// These chunks must have no children
			DestructibleAssetParametersNS::Chunk_Type& chunk = mParams->chunks.buf[i];
			chunk.numChildren = 0;
			chunk.firstChildIndex = DestructibleAssetImpl::InvalidChunkIndex;
			chunk.flags &= ~(uint16_t)DescendantUnfractureable;
		}
		else
		{
			break;
		}
	}

	NvParameterized::Handle handle(*mParams);
	mParams->getParameterHandle("chunks", handle);
	mParams->resizeArray(handle, newChunkCount);

	mParams->getParameterHandle("overlapsAtDepth", handle);
	int size;
	mParams->getArraySize(handle, size);
	if ((int)targetDepthCount < size)
	{
		mParams->resizeArray(handle, (int32_t)targetDepthCount);
	}
	mParams->getParameterHandle("firstChunkAtDepth", handle);
	mParams->resizeArray(handle, (int32_t)targetDepthCount + 1);

	mParams->depthCount = targetDepthCount;

	return true;
}

bool DestructibleAssetImpl::prepareForPlatform(nvidia::apex::PlatformTag platform) const
{
	bool isExistingPlatform = false;
	bool isDepthLimitChanged = false;
	for (Array<PlatformKeyValuePair>::ConstIterator kIter = m_platformFractureDepthMap.begin(); kIter != m_platformFractureDepthMap.end(); ++kIter)
	{
		if (nvidia::strcmp(kIter->key, platform) == 0)
		{
			isExistingPlatform = true;
			isDepthLimitChanged = setDepthCount(kIter->val + 1); //targetDepthCount == 1 => unfractured mesh
			break;
		}
	}
	//if (!isExistingPlatform) {/*keep all depths, behaviour by default*/}
	return (isExistingPlatform & isDepthLimitChanged);
}

void DestructibleAssetImpl::reduceAccordingToLOD()
{
	if (module == NULL)
	{
		return;
	}

	const uint32_t targetDepthCount = mParams->originalDepthCount > module->m_maxChunkDepthOffset ? mParams->originalDepthCount - module->m_maxChunkDepthOffset : 1;

	setDepthCount(targetDepthCount);
}

void DestructibleAssetImpl::getStats(DestructibleAssetStats& stats) const
{
	memset(&stats, 0, sizeof(DestructibleAssetStats));

	if (renderMeshAsset)
	{
		renderMeshAsset->getStats(stats.renderMeshAssetStats);
	}

	// BRG - to do - need a way of getting the serialized size
	stats.totalBytes = 0;

	stats.chunkCount = (uint32_t)mParams->chunks.arraySizes[0];
	stats.chunkBytes = mParams->chunks.arraySizes[0] * sizeof(DestructibleAssetParametersNS::Chunk_Type);

	uint32_t maxEdgeCount = 0;

	for (uint16_t chunkIndex = 0; chunkIndex < getChunkCount(); ++chunkIndex)
	{
		for (uint32_t hullIndex = getChunkHullIndexStart(chunkIndex); hullIndex < getChunkHullIndexStop(chunkIndex); ++hullIndex)
		{
			const ConvexHullImpl& hullData = chunkConvexHulls[hullIndex];
			const uint32_t hullDataBytes = hullData.getVertexCount() * sizeof(PxVec3) +
											   hullData.getUniquePlaneNormalCount() * 4 * sizeof(float) +
											   hullData.getUniquePlaneNormalCount() * sizeof(float) +
											   hullData.getEdgeCount() * sizeof(uint32_t);
			stats.chunkHullDataBytes += hullDataBytes;
			stats.chunkBytes += hullDataBytes;

			// Get cooked convex mesh stats
			uint32_t numVerts = 0;
			uint32_t numPolys = 0;
			const uint32_t cookedHullDataBytes = hullData.calculateCookedSizes(numVerts, numPolys, true);
			stats.maxHullVertexCount = PxMax(stats.maxHullVertexCount, numVerts);
			stats.maxHullFaceCount = PxMax(stats.maxHullFaceCount, numPolys);
			const uint32_t numEdges = numVerts + numPolys - 2;
			if (numEdges > maxEdgeCount)
			{
				maxEdgeCount = numEdges;
				stats.chunkWithMaxEdgeCount = chunkIndex;
			}

			stats.chunkBytes += cookedHullDataBytes;
		}
	}

	stats.runtimeCookedConvexCount = mRuntimeCookedConvexCount;
	/* To do - count collision data in ApexScene! */
}

void DestructibleAssetImpl::applyTransformation(const PxMat44& transformation, float scale)
{
	/* For now, we'll just clear the current cached streams at scale. */
	/* transform and scale the PxConvexMesh(es) */
	if (mParams->collisionData)
	{
		APEX_DEBUG_WARNING("Cached collision data is already present, removing");
		mParams->collisionData->destroy();
		mParams->collisionData = NULL;
#if 0
		DestructibleAssetCollisionDataSet* cds =
		    DYNAMIC_CAST(DestructibleAssetCollisionDataSet*)(mParams->collisionData);

		for (int i = 0; i < cds->meshCookedCollisionStreamsAtScale.arraySizes[0]; ++i)
		{
			PX_ASSERT(cds->meshCookedCollisionStreamsAtScale.buf[i]);

			MeshCookedCollisionStreamsAtScale* meshStreamsAtScale =
			    DYNAMIC_CAST(MeshCookedCollisionStreamsAtScale*)(cds->meshCookedCollisionStreamsAtScale.buf[i]);
			for (int j = 0; j < meshStreamsAtScale->meshCookedCollisionStreams.arraySizes[0]; ++j)
			{
				PX_ASSERT(meshStreamsAtScale->meshCookedCollisionStreams.buf[i]);

				MeshCookedCollisionStream* meshStreamParams =
				    DYNAMIC_CAST(MeshCookedCollisionStream*)(meshStreamsAtScale->meshCookedCollisionStreams.buf[i]);

				/* stream it into the physx sdk */
				nvidia::PsMemoryBuffer memStream(meshStreamParams->bytes.buf, meshStreamParams->bytes.arraySizes[0]);
				memStream.setEndianMode(PxFileBuf::ENDIAN_NONE);
				PxStreamFromFileBuf nvs(memStream);
				PxConvexMesh* convexMesh = GetApexSDK()->getPhysXSDK()->createConvexMesh(nxs);

				PxConvexMeshDesc meshDesc;
				convexMesh->saveToDesc(meshDesc);
				meshDesc.

				uint32_t submeshCount = convexMesh->getSubmeshCount();
				(void)submeshCount;
			}
		}
#endif
	}

	/* chunk surface normal */
	for (int i = 0; i < mParams->chunks.arraySizes[0]; ++i)
	{
		PX_ASSERT(mParams->chunks.buf);

		DestructibleAssetParametersNS::Chunk_Type* chunk = &(mParams->chunks.buf[i]);

		chunk->surfaceNormal = transformation.rotate(chunk->surfaceNormal);
	}

	/* bounds */
	PX_ASSERT(!mParams->bounds.isEmpty());
	mParams->bounds.minimum *= scale;
	mParams->bounds.maximum *= scale;
	if (scale < 0.0f)
	{
		nvidia::swap(mParams->bounds.minimum, mParams->bounds.maximum);
	}
	mParams->bounds = PxBounds3::transformSafe(PxTransform(transformation), mParams->bounds);

	/* chunk convex hulls */
	for (int i = 0; i < mParams->chunkConvexHulls.arraySizes[0]; ++i)
	{
		PX_ASSERT(mParams->chunkConvexHulls.buf[i]);
		ConvexHullImpl chunkHull;
		chunkHull.mParams = DYNAMIC_CAST(ConvexHullParameters*)(mParams->chunkConvexHulls.buf[i]);
		chunkHull.mOwnsParams = false;
		chunkHull.applyTransformation(transformation, scale);
	}

	/* render mesh (just apply to both the asset and author if they both exist) */
	if (renderMeshAsset)
	{
		reinterpret_cast<RenderMeshAssetIntl*>(renderMeshAsset)->applyTransformation(transformation, scale);
	}

	/* scatter meshes */
	const PxMat33 m33(transformation.column0.getXYZ(), transformation.column1.getXYZ(), transformation.column2.getXYZ());
	for (int i = 0; i < mParams->scatterMeshTransforms.arraySizes[0]; ++i)
	{
		PxMat44& tm = mParams->scatterMeshTransforms.buf[i];
		PxMat33 tm33(tm.column0.getXYZ(), tm.column1.getXYZ(), tm.column2.getXYZ()); 

		tm33 = m33*tm33;
		tm33 *= scale;
		const PxVec3 pos = transformation.getPosition() + scale*(m33*tm.getPosition());
		tm.column0 = PxVec4(tm33.column0, 0);
		tm.column1 = PxVec4(tm33.column1, 0);
		tm.column2 = PxVec4(tm33.column2, 0);
		tm.setPosition(pos);
	}
	if (m33.getDeterminant() * scale < 0.0f)
	{
		for (uint32_t i = 0; i < scatterMeshAssets.size(); ++i)
		{
			RenderMeshAsset* scatterMesh = scatterMeshAssets[i];
			if (scatterMesh != NULL)
			{
				reinterpret_cast<RenderMeshAssetIntl*>(scatterMesh)->reverseWinding();
			}
		}
	}
}

void DestructibleAssetImpl::applyTransformation(const PxMat44& transformation)
{
	/* For now, we'll just clear the current cached streams at scale. */
	/* transform and scale the PxConvexMesh(es) */
	if (mParams->collisionData)
	{
		APEX_DEBUG_WARNING("Cached collision data is already present, removing");
		mParams->collisionData->destroy();
		mParams->collisionData = NULL;
	}

	Cof44 cofTM(transformation);

	/* chunk surface normal */
	for (int i = 0; i < mParams->chunks.arraySizes[0]; ++i)
	{
		PX_ASSERT(mParams->chunks.buf);

		DestructibleAssetParametersNS::Chunk_Type* chunk = &(mParams->chunks.buf[i]);

		chunk->surfaceNormal = cofTM.getBlock33().transform(chunk->surfaceNormal);
		chunk->surfaceNormal.normalize();
	}

	/* bounds */
	PX_ASSERT(!mParams->bounds.isEmpty());
	mParams->bounds = PxBounds3::transformSafe(PxTransform(transformation), mParams->bounds);

	/* chunk convex hulls */
	for (int i = 0; i < mParams->chunkConvexHulls.arraySizes[0]; ++i)
	{
		PX_ASSERT(mParams->chunkConvexHulls.buf[i]);
		ConvexHullImpl chunkHull;
		chunkHull.mParams = DYNAMIC_CAST(ConvexHullParameters*)(mParams->chunkConvexHulls.buf[i]);
		chunkHull.mOwnsParams = false;
		chunkHull.applyTransformation(transformation);
	}

	/* render mesh (just apply to both the asset and author if they both exist) */
	if (renderMeshAsset)
	{
		reinterpret_cast<RenderMeshAssetIntl*>(renderMeshAsset)->applyTransformation(transformation, 1.0f);	// This transformation function properly handles non-uniform scales
	}
}

void DestructibleAssetImpl::traceSurfaceBoundary(physx::Array<PxVec3>& outPoints, uint16_t chunkIndex, const PxTransform& localToWorldRT, const PxVec3& scale,
        float spacing, float jitter, float surfaceDistance, uint32_t maxPoints)
{
	outPoints.resize(0);

	// Removing this function's implementation for now

	PX_UNUSED(chunkIndex);
	PX_UNUSED(localToWorldRT);
	PX_UNUSED(scale);
	PX_UNUSED(spacing);
	PX_UNUSED(jitter);
	PX_UNUSED(surfaceDistance);
	PX_UNUSED(maxPoints);
}


bool DestructibleAssetImpl::rebuildCollisionGeometry(uint32_t partIndex, const DestructibleGeometryDesc& geometryDesc)
{
#ifdef WITHOUT_APEX_AUTHORING
	PX_UNUSED(partIndex);
	PX_UNUSED(geometryDesc);
	APEX_DEBUG_WARNING("DestructibleAsset::rebuildCollisionGeometry is not available in release builds.");
	return false;
#else
	NvParameterized::Handle handle(*mParams);
	mParams->getParameterHandle("chunkConvexHulls", handle);
	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();

	const uint32_t startHullIndex = mParams->chunkConvexHullStartIndices.buf[partIndex];
	uint32_t hullCount = geometryDesc.convexHullCount;
	const nvidia::ExplicitHierarchicalMesh::ConvexHull** convexHulls = geometryDesc.convexHulls;
	physx::Array<PartConvexHullProxy*> collision;
	if (hullCount == 0 && geometryDesc.collisionVolumeDesc != NULL)
	{
		physx::Array<PxVec3> vertices;
		physx::Array<uint32_t> indices;
		gatherPartMesh(vertices, indices, renderMeshAsset, partIndex);
		buildCollisionGeometry(collision, *geometryDesc.collisionVolumeDesc, vertices.begin(), vertices.size(), sizeof(PxVec3), indices.begin(), indices.size());
		convexHulls = (const nvidia::ExplicitHierarchicalMesh::ConvexHull**)collision.begin();
		hullCount = collision.size();
	}
	const uint32_t oldStopHullIndex = mParams->chunkConvexHullStartIndices.buf[partIndex + 1];
	const uint32_t oldHullCount = oldStopHullIndex - startHullIndex;
	const uint32_t stopHullIndex = startHullIndex + hullCount;
	const int32_t hullCountDelta = (int32_t)(hullCount - oldHullCount);

	// Adjust start indices
	for (uint32_t i = partIndex+1; i < (uint32_t)mParams->chunkConvexHullStartIndices.arraySizes[0]; ++i)
	{
		mParams->chunkConvexHullStartIndices.buf[i] += hullCountDelta;
	}
	const uint32_t totalHullCount = (uint32_t)mParams->chunkConvexHullStartIndices.buf[mParams->chunkConvexHullStartIndices.arraySizes[0]-1];

	// Eliminate hulls if the count has decreased
	if (hullCountDelta < 0)
	{
		for (uint32_t index = stopHullIndex; index < totalHullCount; ++index)
		{
			chunkConvexHulls[index].mParams->copy( *chunkConvexHulls[index-hullCountDelta].mParams );
		}
	}

	// Resize the hull arrays
	if (hullCountDelta != 0)
	{
		mParams->resizeArray(handle, (int32_t)totalHullCount);
		chunkConvexHulls.resize(totalHullCount);
	}

	// Insert hulls if the count has increased
	if (hullCountDelta > 0)
	{
		for (uint32_t index = totalHullCount; index-- > stopHullIndex;)
		{
			mParams->chunkConvexHulls.buf[index] = mParams->chunkConvexHulls.buf[index-hullCountDelta];
			chunkConvexHulls[index].init(mParams->chunkConvexHulls.buf[index]);
		}
		for (uint32_t index = oldStopHullIndex; index < stopHullIndex; ++index)
		{
			mParams->chunkConvexHulls.buf[index] = traits->createNvParameterized(ConvexHullParameters::staticClassName());
			chunkConvexHulls[index].init(mParams->chunkConvexHulls.buf[index]);
		}
	}

	if (hullCount > 0)
	{
		for (uint32_t hullNum = 0; hullNum < hullCount; ++hullNum)
		{
			ConvexHullImpl& chunkHullData = chunkConvexHulls[startHullIndex + hullNum];
			PartConvexHullProxy* convexHullProxy = (PartConvexHullProxy*)convexHulls[hullNum];
			chunkHullData.mParams->copy(*convexHullProxy->impl.mParams);
			if (chunkHullData.isEmpty())
			{
				chunkHullData.buildFromAABB(renderMeshAsset->getBounds(partIndex));	// \todo: need better way of simplifying
			}
		}
	}

	return hullCount > 0;
#endif
}


// DestructibleAssetAuthoring
#ifndef WITHOUT_APEX_AUTHORING



void gatherPartMesh(physx::Array<PxVec3>& vertices,
					physx::Array<uint32_t>& indices,
					const RenderMeshAsset* renderMeshAsset,
					uint32_t partIndex)
{
	if (renderMeshAsset == NULL || partIndex >= renderMeshAsset->getPartCount())
	{
		vertices.resize(0);
		indices.resize(0);
		return;
	}


	// Pre-count vertices and indices so we can allocate once
	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;
	for (uint32_t submeshIndex = 0; submeshIndex < renderMeshAsset->getSubmeshCount(); ++submeshIndex)
	{
		const RenderSubmesh& submesh = renderMeshAsset->getSubmesh(submeshIndex);
		vertexCount += submesh.getVertexCount(partIndex);
		indexCount += submesh.getIndexCount(partIndex);
	}

	vertices.resize(vertexCount);
	indices.resize(indexCount);

	vertexCount = 0;
	indexCount = 0;
	for (uint32_t submeshIndex = 0; submeshIndex < renderMeshAsset->getSubmeshCount(); ++submeshIndex)
	{
		const RenderSubmesh& submesh = renderMeshAsset->getSubmesh(submeshIndex);
		if (submesh.getVertexCount(partIndex) > 0)
		{
			const VertexBuffer& vertexBuffer = submesh.getVertexBuffer();
			const VertexFormat& vertexFormat = vertexBuffer.getFormat();
			const int32_t bufferIndex = vertexFormat.getBufferIndexFromID(vertexFormat.getSemanticID(nvidia::RenderVertexSemantic::POSITION));
			if (bufferIndex >= 0)
			{
				vertexBuffer.getBufferData(&vertices[vertexCount], nvidia::RenderDataFormat::FLOAT3, sizeof(PxVec3), (uint32_t)bufferIndex, 
					submesh.getFirstVertexIndex(partIndex), submesh.getVertexCount(partIndex));
			}
			else
			{
				memset(&vertices[vertexCount], 0, submesh.getVertexCount(partIndex)*sizeof(PxVec3));
			}
			const uint32_t* partIndexBuffer = submesh.getIndexBuffer(partIndex);
			const uint32_t partIndexCount = submesh.getIndexCount(partIndex);
			for (uint32_t indexNum = 0; indexNum < partIndexCount; ++indexNum)
			{
				indices[indexCount++] = partIndexBuffer[indexNum] + vertexCount - submesh.getFirstVertexIndex(partIndex);
			}
			vertexCount += submesh.getVertexCount(partIndex);
		}
	}
}


/**
	Private function - it's way too finicky and really only meant to serve the (public) trimCollisionVolumes function.
	The chunkIndices array is expected to contain every member of an instanced set.  That is, if a chunk indexed by some element of chunkIndices
	references partIndex, then *every* chunk that references partIndex should be in the chunkIndices array.
	Also, all chunks in chunkIndices are expected to be at the same depth.
*/
void DestructibleAssetAuthoringImpl::trimCollisionGeometryInternal(const uint32_t* chunkIndices, uint32_t chunkIndexCount, const physx::Array<IntPair>& parentDepthOverlaps, uint32_t depth, float maxTrimFraction)
{
	/*
		1) Find overlaps between each chunk (hull to hull).
		2) For each overlap:
			a) Create a trim plane for the overlapping hulls (one for each).  Adjust the trim planes to respect maxTrimFraction.
		3) For each chunk:
			a) For each hull in the chunk:
				i) Add additional trim planes from all parent chunks' neighbors' hulls, and sibling's hulls.  Adjust trim planes to respect maxTrimFraction.
		4) Intersect the trimmed hull(s) from (2) and (3) with the trim planes.  This may cause some hulls to disappear.
		5) Recurse to children
	*/

	// Create an epslion
	PxBounds3 bounds = renderMeshAsset->getBounds();
	const float sizeScale = (bounds.minimum - bounds.maximum).magnitude();
	const float eps = 0.0001f * sizeScale;	// \todo - expose?

	//	1) Find overlaps between each chunk (hull to hull).
	const PxMat44 identityTM(PxVec4(1.0f));
	const PxVec3 identityScale(1.0f);

	// Find AABB overlaps
	physx::Array<BoundsRep> chunkBoundsReps;
	chunkBoundsReps.reserve(chunkIndexCount);
	for (uint32_t chunkNum = 0; chunkNum < chunkIndexCount; ++chunkNum)
	{
		const uint32_t chunkIndex = chunkIndices[chunkNum];
		BoundsRep& chunkBoundsRep = chunkBoundsReps.insert();
		chunkBoundsRep.aabb = getChunkActorLocalBounds(chunkIndex);
	}
	physx::Array<IntPair> overlaps;
	if (chunkBoundsReps.size() > 0)
	{
		boundsCalculateOverlaps(overlaps, Bounds3XYZ, &chunkBoundsReps[0], chunkBoundsReps.size(), sizeof(chunkBoundsReps[0]));
	}

	// We'll store the trim planes here
	physx::Array<TrimPlane> trimPlanes;

	// Now do detailed overlap test
	for (uint32_t overlapIndex = 0; overlapIndex < overlaps.size(); ++overlapIndex)
	{
		IntPair& AABBOverlap = overlaps[overlapIndex];
		const uint32_t chunkIndex0 = chunkIndices[AABBOverlap.i0];
		const uint32_t chunkIndex1 = chunkIndices[AABBOverlap.i1];

		// Offset chunks (in case they are instanced)
		const PxTransform tm0(getChunkPositionOffset(chunkIndex0));
		const PxTransform tm1(getChunkPositionOffset(chunkIndex1));
		for (uint32_t hullIndex0 = getChunkHullIndexStart(chunkIndex0); hullIndex0 < getChunkHullIndexStop(chunkIndex0); ++hullIndex0)
		{
			TrimPlane trimPlane0;
			trimPlane0.hullIndex = hullIndex0;
			trimPlane0.partIndex = getPartIndex(chunkIndex0);
			for (uint32_t hullIndex1 = getChunkHullIndexStart(chunkIndex1); hullIndex1 < getChunkHullIndexStop(chunkIndex1); ++hullIndex1)
			{
				TrimPlane trimPlane1;
				trimPlane1.hullIndex = hullIndex1;
				trimPlane1.partIndex = getPartIndex(chunkIndex1);
				ConvexHullImpl::Separation separation;
				if (ConvexHullImpl::hullsInProximity(chunkConvexHulls[hullIndex0], tm0, identityScale, chunkConvexHulls[hullIndex1], tm1, identityScale, 0.0f, &separation))
				{
					//	2) For each overlap:
					//		a) Create a trim plane for the overlapping hulls (one for each).  Adjust the trim planes to respect maxTrimFraction.
					trimPlane0.plane = separation.plane;
					trimPlane0.plane.d = PxMin(trimPlane0.plane.d, maxTrimFraction*(separation.max0-separation.min0) - separation.max0);	// Bound clip distance
					trimPlane0.plane.d += trimPlane0.plane.n.dot(tm0.p);	// Transform back into part local space
					trimPlanes.pushBack(trimPlane0);
					trimPlane1.plane = PxPlane(-separation.plane.n, -separation.plane.d);
					trimPlane1.plane.d = PxMin(trimPlane1.plane.d, maxTrimFraction*(separation.max1-separation.min1) + separation.min1);	// Bound clip distance
					trimPlane1.plane.d += trimPlane1.plane.n.dot(tm1.p);	// Transform back into part local space
					trimPlanes.pushBack(trimPlane1);
				}
			}
		}
	}

	// Get overlaps for this depth
	physx::Array<IntPair> overlapsAtDepth;
	calculateChunkOverlaps(overlapsAtDepth, depth);

	//	3) For each chunk:
	for (uint32_t chunkNum = 0; chunkNum < chunkIndexCount; ++chunkNum)
	{
		const uint32_t chunkIndex = chunkIndices[chunkNum];
		const PxTransform tm0(getChunkPositionOffset(chunkIndex));
		const int32_t chunkParentIndex = mParams->chunks.buf[chunkIndex].parentIndex;

		for (int ancestorLevel = 0; ancestorLevel < 2; ++ancestorLevel)	// First time iterate through uncles, second time through siblings
		{
			// Optimization opportunity: symmetrize, sort and index the overlap list
			uint32_t overlapCount;
			const IntPair* overlapArray;
			if (ancestorLevel == 0)
			{
				overlapCount = parentDepthOverlaps.size();
				overlapArray = parentDepthOverlaps.begin();
			}
			else
			{
				overlapCount = overlapsAtDepth.size();
				overlapArray = overlapsAtDepth.begin();
			}
			for (uint32_t overlapNum = 0; overlapNum < overlapCount; ++overlapNum)
			{
				const IntPair& overlap = overlapArray[overlapNum];
				uint32_t otherChunkIndex;
				const int32_t ignoreChunkIndex = ancestorLevel == 0 ? chunkParentIndex : (int32_t)chunkIndex;
				if (overlap.i0 == ignoreChunkIndex)
				{
					otherChunkIndex = (uint32_t)overlap.i1;
				}
				else
				if (overlap.i1 == ignoreChunkIndex)
				{
					otherChunkIndex = (uint32_t)overlap.i0;
				}
				else
				{
					continue;
				}
				// Make sure we're not trimming from a chunk already in our chunk list (we've handled that already)
				bool alreadyConsidered = false;
				if (ancestorLevel == 1)
				{
					for (uint32_t checkNum = 0; checkNum < chunkIndexCount; ++checkNum)
					{
						if (chunkIndices[checkNum] == otherChunkIndex)
						{
							alreadyConsidered = true;
							break;
						}
					}
				}
				if (alreadyConsidered)
				{
					continue;
				}
				// Check other Chunk
				const PxTransform tm1(getChunkPositionOffset(otherChunkIndex));
				//	a) For each hull in the chunk:
				for (uint32_t hullIndex0 = getChunkHullIndexStart(chunkIndex); hullIndex0 < getChunkHullIndexStop(chunkIndex); ++hullIndex0)
				{
					TrimPlane trimPlane;
					trimPlane.hullIndex = hullIndex0;
					trimPlane.partIndex = getPartIndex(chunkIndex);
					for (uint32_t hullIndex1 = getChunkHullIndexStart(otherChunkIndex); hullIndex1 < getChunkHullIndexStop(otherChunkIndex); ++hullIndex1)
					{
						ConvexHullImpl::Separation separation;
						if (ConvexHullImpl::hullsInProximity(chunkConvexHulls[hullIndex0], tm0, identityScale, chunkConvexHulls[hullIndex1], tm1, identityScale, 0.0f, &separation))
						{
							if (PxMax(separation.min0 - separation.max1, separation.min1 - separation.max0) < -eps)
							{
								trimPlane.plane = separation.plane;
								trimPlane.plane.d = -separation.min1;	// Allow for other hull to intrude completely
								trimPlane.plane.d = PxMin(trimPlane.plane.d, maxTrimFraction*(separation.max0-separation.min0) - separation.max0);	// Bound clip distance
								trimPlane.plane.d += trimPlane.plane.n.dot(tm0.p);	// Transform back into part local space
								trimPlanes.pushBack(trimPlane);
							}
						}
					}
				}
			}
		}
	}

	//	4) Intersect the trimmed hull(s) from (2) and (3) with the trim planes.  This may cause some hulls to disappear.

	// Sort by part, then by hull.  We're going to get a little rough with these parts.
	nvidia::sort<TrimPlane, TrimPlane::LessThan>(trimPlanes.begin(), trimPlanes.size(), TrimPlane::LessThan());

	// Create a lookup into the array
	physx::Array<uint32_t> trimPlaneLookup;
	createIndexStartLookup(trimPlaneLookup, 0, renderMeshAsset->getPartCount(), trimPlanes.size() > 0 ? (int32_t*)&trimPlanes[0].partIndex : NULL, trimPlanes.size(), sizeof(TrimPlane));

	// Trim
	for (uint32_t partIndex = 0; partIndex < renderMeshAsset->getPartCount(); ++partIndex)
	{
		bool hullCountChanged = false;
		for (uint32_t partTrimPlaneIndex = trimPlaneLookup[partIndex]; partTrimPlaneIndex < trimPlaneLookup[partIndex+1]; ++partTrimPlaneIndex)
		{
			TrimPlane& trimPlane = trimPlanes[partTrimPlaneIndex];
			ConvexHullImpl& hull = chunkConvexHulls[trimPlane.hullIndex];
			hull.intersectPlaneSide(trimPlane.plane);
			if (hull.isEmpty())
			{
				hullCountChanged = true;
			}
		}
		if (hullCountChanged)
		{
			// Re-apply hulls that haven't disappeared
			physx::Array<PartConvexHullProxy*> newHulls;
			for (uint32_t hullIndex = mParams->chunkConvexHullStartIndices.buf[partIndex]; hullIndex < mParams->chunkConvexHullStartIndices.buf[partIndex+1]; ++hullIndex)
			{
				ConvexHullImpl& hull = chunkConvexHulls[hullIndex];
				if (!hull.isEmpty())
				{
					PartConvexHullProxy* newHull = PX_NEW(PartConvexHullProxy);
					newHulls.pushBack(newHull);
					newHull->impl.init(hull.mParams);
				}
			}
			DestructibleGeometryDesc geometryDesc;
			CollisionVolumeDesc collisionVolumeDesc;
			if (newHulls.size() > 0)
			{
				geometryDesc.convexHulls = (const nvidia::ExplicitHierarchicalMesh::ConvexHull**)newHulls.begin();
				geometryDesc.convexHullCount = newHulls.size();
			}
			else
			{
				// We've lost all of the collision volume!  Quite a shame... create a hull to replace it.
				geometryDesc.collisionVolumeDesc = &collisionVolumeDesc;
				collisionVolumeDesc.mHullMethod = ConvexHullMethod::WRAP_GRAPHICS_MESH;
			}
			rebuildCollisionGeometry(partIndex, geometryDesc);
			for (uint32_t hullN = 0; hullN < newHulls.size(); ++hullN)
			{
				PX_DELETE(newHulls[hullN]);
			}
		}
	}

	//	5) Recurse to children

	// Iterate through chunks and collect children
	physx::Array<uint32_t> childChunkIndices;
	for (uint32_t chunkNum = 0; chunkNum < chunkIndexCount; ++chunkNum)
	{
		const uint32_t chunkIndex = chunkIndices[chunkNum];
		const uint16_t firstChildIndex = mParams->chunks.buf[chunkIndex].firstChildIndex;
		for (uint16_t childNum = 0; childNum < mParams->chunks.buf[chunkIndex].numChildren; ++childNum)
		{
			childChunkIndices.pushBack((uint32_t)(firstChildIndex + childNum));
		}
	}

	// Recurse
	if (childChunkIndices.size())
	{
		trimCollisionGeometryInternal(childChunkIndices.begin(), childChunkIndices.size(), overlapsAtDepth, depth+1, maxTrimFraction);
	}
}

struct PartAndChunk
{
	uint32_t chunkIndex;
	int32_t partIndex;

	struct LessThan
	{
		PX_INLINE bool operator()(const PartAndChunk& x, const PartAndChunk& y) const
		{
			return x.partIndex != y.partIndex ? (x.partIndex < y.partIndex) : (x.chunkIndex < y.chunkIndex);
		}
	};
};

// Here's our chunk list element, with depth (for sorting)
struct ChunkAndDepth
{
	uint32_t chunkIndex;
	int32_t depth;

	struct LessThan
	{
		PX_INLINE bool operator()(const ChunkAndDepth& x, const ChunkAndDepth& y) const
		{
			return x.depth != y.depth ? (x.depth < y.depth) : (x.chunkIndex < y.chunkIndex);
		}
	};
};

void DestructibleAssetAuthoringImpl::trimCollisionGeometry(const uint32_t* partIndices, uint32_t partIndexCount, float maxTrimFraction)
{
	/*
		1) Create a list of chunks which reference each partIndex. (If there is instancing, there may be more than one chunk per part.)
		2) Sort by depth (stable sort)
		3) For each depth:
			a) Collect a list of chunks at that depth, and call trimCollisionVolumesInternal.
	*/

	//	1) Create a list of chunks which reference each partIndex. (If there is instancing, there may be more than one chunk per part.)

	// Fill array and sort
	physx::Array<PartAndChunk> partAndChunkList;
	partAndChunkList.resize(getChunkCount());
	for (uint32_t chunkIndex = 0; chunkIndex < getChunkCount(); ++chunkIndex)
	{
		partAndChunkList[chunkIndex].chunkIndex = chunkIndex;
		partAndChunkList[chunkIndex].partIndex = (int32_t)getPartIndex(chunkIndex);
	}
	nvidia::sort<PartAndChunk, PartAndChunk::LessThan>(partAndChunkList.begin(), partAndChunkList.size(), PartAndChunk::LessThan());

	// Create a lookup into the array
	physx::Array<uint32_t> partAndChunkLookup;
	createIndexStartLookup(partAndChunkLookup, 0, renderMeshAsset->getPartCount(), &partAndChunkList[0].partIndex, getChunkCount(), sizeof(PartAndChunk));

	//	2) Sort by depth (stable sort)

	// Count how many chunks there will be
	uint32_t chunkListSize = 0;
	for (uint32_t partNum = 0; partNum < partIndexCount; ++partNum)
	{
		const uint32_t partIndex = partIndices[partNum];
		chunkListSize += partAndChunkLookup[partIndex+1] - partAndChunkLookup[partIndex];
	}

	// Fill and sort
	physx::Array<ChunkAndDepth> chunkAndDepthList;
	chunkAndDepthList.resize(chunkListSize);
	uint32_t chunkNum = 0;
	for (uint32_t partNum = 0; partNum < partIndexCount; ++partNum)
	{
		const uint32_t partIndex = partIndices[partNum];
		for (uint32_t partChunkNum = partAndChunkLookup[partIndex]; partChunkNum < partAndChunkLookup[partIndex+1]; ++partChunkNum, ++chunkNum)
		{
			const uint32_t chunkIndex = partAndChunkList[partChunkNum].chunkIndex;
			chunkAndDepthList[chunkNum].chunkIndex = chunkIndex;
			chunkAndDepthList[chunkNum].depth = mParams->chunks.buf[chunkIndex].depth;
		}
	}
	nvidia::sort<ChunkAndDepth, ChunkAndDepth::LessThan>(chunkAndDepthList.begin(), chunkAndDepthList.size(), ChunkAndDepth::LessThan());

	// And create a lookup into the array
	physx::Array<uint32_t> chunkAndDepthLookup;
	createIndexStartLookup(chunkAndDepthLookup, 0, mParams->depthCount, &chunkAndDepthList[0].depth, chunkListSize, sizeof(ChunkAndDepth));

	// 3) For each depth:
	for (uint32_t depth = 0; depth < mParams->depthCount; ++depth)
	{
		physx::Array<uint32_t> chunkIndexList;
		chunkIndexList.resize(chunkAndDepthLookup[depth+1] - chunkAndDepthLookup[depth]);
		if (chunkIndexList.size() == 0)
		{
			continue;
		}
		uint32_t chunkIndexListSize = 0;
		for (uint32_t depthChunkNum = chunkAndDepthLookup[depth]; depthChunkNum < chunkAndDepthLookup[depth+1]; ++depthChunkNum)
		{
			chunkIndexList[chunkIndexListSize++] = chunkAndDepthList[depthChunkNum].chunkIndex;
		}
		PX_ASSERT(chunkIndexListSize == chunkIndexList.size());
		physx::Array<IntPair> overlaps;
		if (depth > 0)
		{
			calculateChunkOverlaps(overlaps, depth-1);
		}
		trimCollisionGeometryInternal(chunkIndexList.begin(), chunkIndexList.size(), overlaps, depth, maxTrimFraction);
	}
}

void DestructibleAssetAuthoringImpl::setToolString(const char* toolString)
{
	if (mParams != NULL)
	{
		NvParameterized::Handle handle(*mParams, "comments");
		PX_ASSERT(handle.isValid());
		if (handle.isValid())
		{
			PX_ASSERT(handle.parameterDefinition()->type() == NvParameterized::TYPE_STRING);
			handle.setParamString(toolString);
		}
	}
}

void DestructibleAssetAuthoringImpl::cookChunks(const DestructibleAssetCookingDesc& cookingDesc, bool cacheOverlaps, uint32_t* chunkIndexMapUser2Apex, uint32_t* chunkIndexMapApex2User, uint32_t chunkIndexMapCount)
{
	if (!cookingDesc.isValid())
	{
		APEX_INVALID_PARAMETER("DestructibleAssetAuthoring::cookChunks: cookingDesc invalid.");
		return;
	}

	const uint32_t numChunks = cookingDesc.chunkDescCount;
	const uint32_t numBehaviorGroups = cookingDesc.behaviorGroupDescCount;
	const uint32_t numParts = renderMeshAsset->getPartCount();

	if ((chunkIndexMapUser2Apex != NULL || chunkIndexMapApex2User != NULL) && chunkIndexMapCount < numChunks)
	{
		APEX_INVALID_PARAMETER("DestructibleAssetAuthoring::cookChunks: chunkIndexMap is not big enough.");
		return;
	}

	NvParameterized::Handle handle(*mParams);

	mParams->getParameterHandle("chunks", handle);
	mParams->resizeArray(handle, (int32_t)numChunks);
	mParams->depthCount = 0;

	// Create convex hulls
	mParams->getParameterHandle("chunkConvexHulls", handle);
	mParams->resizeArray(handle, 0);
	chunkConvexHulls.reset();

	mParams->getParameterHandle("chunkConvexHullStartIndices", handle);
	mParams->resizeArray(handle, (int32_t)numParts + 1);
	for (uint32_t partIndex = 0; partIndex <= numParts; ++partIndex)
	{
		mParams->chunkConvexHullStartIndices.buf[partIndex] = 0;	// Initialize all to zero, so that the random-access rebuildCollisionGeometry does the right thing (below)
	}
	for (uint32_t partIndex = 0; partIndex < numParts; ++partIndex)
	{
		if (!rebuildCollisionGeometry(partIndex, cookingDesc.geometryDescs[partIndex]))
		{
			APEX_INVALID_PARAMETER("DestructibleAssetAuthoring::cookChunks: Could not find or generate collision hull for part.");
		}
	}

	// Sort - chunks must be in parent-sorted order
	Array<ChunkSortElement> sortElements;
	sortElements.resize(numChunks);
	for (uint32_t i = 0; i < numChunks; ++i)
	{
		const DestructibleChunkDesc& chunkDesc = cookingDesc.chunkDescs[i];
		sortElements[i].index = (int32_t)i;
		sortElements[i].parentIndex = chunkDesc.parentIndex;
		sortElements[i].depth = 0;
		int32_t parent = (int32_t)i;
		uint32_t counter = 0;
		while ((parent = cookingDesc.chunkDescs[parent].parentIndex) >= 0)
		{
			++sortElements[i].depth;
			if (++counter > numChunks)
			{
				APEX_INVALID_PARAMETER("DestructibleAssetAuthoring::cookChunks: loop found in cookingDesc parent indices.  Cannot build an DestructibleAsset.");
				return;
			}
		}
	}
	qsort(sortElements.begin(), numChunks, sizeof(ChunkSortElement), compareChunkParents);

	Array<uint32_t> ranks;
	if (chunkIndexMapUser2Apex == NULL && numChunks > 0)
	{
		ranks.resize(numChunks);
		chunkIndexMapUser2Apex = &ranks[0];
	}

	for (uint32_t i = 0; i < numChunks; ++i)
	{
		chunkIndexMapUser2Apex[sortElements[i].index] = i;
		if (chunkIndexMapApex2User != NULL)
		{
			chunkIndexMapApex2User[i] = (uint32_t)sortElements[i].index;
		}
	}

	// Count instanced chunks and allocate instanced info array
	uint32_t instancedChunkCount = 0;
	for (uint32_t i = 0; i < numChunks; ++i)
	{
		const DestructibleChunkDesc& chunkDesc = cookingDesc.chunkDescs[sortElements[i].index];
		if (chunkDesc.useInstancedRendering)
		{
			++instancedChunkCount;
		}
	}

	mParams->getParameterHandle("chunkInstanceInfo", handle);
	mParams->resizeArray(handle, (int32_t)instancedChunkCount);

	mParams->getParameterHandle("scatterMeshIndices", handle);
	mParams->resizeArray(handle, 0);
	mParams->getParameterHandle("scatterMeshTransforms", handle);
	mParams->resizeArray(handle, 0);

	const DestructibleChunkDesc defaultChunkDesc;

	instancedChunkCount = 0;	// reset and use as cursor

	for (uint32_t i = 0; i < numChunks; ++i)
	{
		DestructibleAssetParametersNS::Chunk_Type& chunk = mParams->chunks.buf[i];
		const DestructibleChunkDesc& chunkDesc = cookingDesc.chunkDescs[sortElements[i].index];
		chunk.flags = 0;
		if (chunkDesc.isSupportChunk)
		{
			chunk.flags |= SupportChunk;
		}
		if (chunkDesc.doNotFracture)
		{
			chunk.flags |= UnfracturableChunk;
		}
		if (chunkDesc.doNotDamage)
		{
			chunk.flags |= UndamageableChunk;
		}
		if (chunkDesc.doNotCrumble)
		{
			chunk.flags |= UncrumbleableChunk;
		}
#if APEX_RUNTIME_FRACTURE
		if (chunkDesc.runtimeFracture)
		{
			chunk.flags |= RuntimeFracturableChunk;
		}
#endif
		if (!chunkDesc.useInstancedRendering)
		{
			// Not instanced, meshPartIndex will be used to directly access the mesh part in the "normal" mesh
			chunk.meshPartIndex = chunkDesc.meshIndex;
		}
		else
		{
			// Instanced, meshPartIndex will be used to access instance info
			chunk.flags |= Instanced;
			chunk.meshPartIndex = (uint16_t)instancedChunkCount++;
			DestructibleAssetParametersNS::InstanceInfo_Type& instanceInfo = mParams->chunkInstanceInfo.buf[chunk.meshPartIndex];
			instanceInfo.partIndex = chunkDesc.meshIndex;
			instanceInfo.chunkPositionOffset = chunkDesc.instancePositionOffset;
			instanceInfo.chunkUVOffset = chunkDesc.instanceUVOffset;
		}
		if (sortElements[i].index == 0)
		{
			chunk.depth = 0;
			chunk.parentIndex = DestructibleAssetImpl::InvalidChunkIndex;
		}
		else
		{
			chunk.parentIndex = (uint16_t)chunkIndexMapUser2Apex[(int16_t)chunkDesc.parentIndex];
			DestructibleAssetParametersNS::Chunk_Type& parent = mParams->chunks.buf[chunk.parentIndex];
			PX_ASSERT(chunk.parentIndex >= mParams->chunks.buf[i - 1].parentIndex ||
			          mParams->chunks.buf[i - 1].parentIndex == DestructibleAssetImpl::InvalidChunkIndex);	// Parent-sorted order
			if (chunk.parentIndex != mParams->chunks.buf[i - 1].parentIndex)
			{
				parent.firstChildIndex = (uint16_t)i;
			}
			++parent.numChildren;
			chunk.depth = uint16_t(parent.depth + 1);
			if ((parent.flags & SupportChunk) != 0)
			{
				chunk.flags |= SupportChunk;	// All children of support chunks can be support chunks
			}
			if ((parent.flags & UnfracturableChunk) != 0)
			{
				chunk.flags |= UnfracturableChunk;	// All children of unfracturable chunks are unfracturable
			}
#if APEX_RUNTIME_FRACTURE
			if ((parent.flags & RuntimeFracturableChunk) != 0) // Runtime fracturable chunks cannot have any children
			{
				PX_ALWAYS_ASSERT();
			}
#endif
		}
		if ((chunk.flags & UnfracturableChunk) != 0)
		{
			// All ancestors of unfracturable chunks must be unfracturable or note that they have an unfracturable descendant
			uint16_t parentIndex = chunk.parentIndex;
			while (parentIndex != DestructibleAssetImpl::InvalidChunkIndex)
			{
				DestructibleAssetParametersNS::Chunk_Type& parent = mParams->chunks.buf[parentIndex];
				if ((parent.flags & UnfracturableChunk) == 0)
				{
					parent.flags |= DescendantUnfractureable;
				}
				parentIndex = parent.parentIndex;
			}
		}
		chunk.numChildren = 0;
		chunk.firstChildIndex = DestructibleAssetImpl::InvalidChunkIndex;
		mParams->depthCount = PxMax((uint16_t)mParams->depthCount, (uint16_t)(chunk.depth + 1));
		chunk.surfaceNormal = chunkDesc.surfaceNormal;
		chunk.behaviorGroupIndex = chunkDesc.behaviorGroupIndex;

		// Default behavior is to take on the parent's behavior group
		if (chunk.parentIndex != DestructibleAssetImpl::InvalidChunkIndex)
		{
			if (chunk.behaviorGroupIndex < 0)
			{
				chunk.behaviorGroupIndex = mParams->chunks.buf[chunk.parentIndex].behaviorGroupIndex;
			}
		}

		// Scatter mesh
		const int32_t oldScatterMeshCount = mParams->scatterMeshIndices.arraySizes[0];
		chunk.firstScatterMesh = (uint16_t)oldScatterMeshCount;
		chunk.scatterMeshCount = (uint16_t)chunkDesc.scatterMeshCount;
		if (chunk.scatterMeshCount > 0)
		{
			mParams->getParameterHandle("scatterMeshIndices", handle);
			mParams->resizeArray(handle, oldScatterMeshCount + (int32_t)chunk.scatterMeshCount);
			handle.setParamU8Array(chunkDesc.scatterMeshIndices, (int32_t)chunk.scatterMeshCount, oldScatterMeshCount);
			mParams->getParameterHandle("scatterMeshTransforms", handle);
			mParams->resizeArray(handle, oldScatterMeshCount + (int32_t)chunk.scatterMeshCount);
			for (uint16_t tmNum = 0; tmNum < chunk.scatterMeshCount; ++tmNum)
			{
				mParams->scatterMeshTransforms.buf[oldScatterMeshCount + tmNum] = chunkDesc.scatterMeshTransforms[tmNum];
			}
		}
	}
	
	mParams->getParameterHandle("behaviorGroups", handle);
	mParams->resizeArray(handle, (int32_t)numBehaviorGroups);

	struct {
		void operator() (const DestructibleBehaviorGroupDesc& behaviorGroupDesc, DestructibleAssetParameters* params, NvParameterized::Handle& elementHandle)
		{
			NvParameterized::Handle subElementHandle(*params);

			elementHandle.getChildHandle(params, "name", subElementHandle);
			params->setParamString(subElementHandle, behaviorGroupDesc.name);

			elementHandle.getChildHandle(params, "damageThreshold", subElementHandle);
			params->setParamF32(subElementHandle, behaviorGroupDesc.damageThreshold);
			
			elementHandle.getChildHandle(params, "damageToRadius", subElementHandle);
			params->setParamF32(subElementHandle, behaviorGroupDesc.damageToRadius);

			elementHandle.getChildHandle(params, "damageSpread.minimumRadius", subElementHandle);
			params->setParamF32(subElementHandle, behaviorGroupDesc.damageSpread.minimumRadius);

			elementHandle.getChildHandle(params, "damageSpread.radiusMultiplier", subElementHandle);
			params->setParamF32(subElementHandle, behaviorGroupDesc.damageSpread.radiusMultiplier);

			elementHandle.getChildHandle(params, "damageSpread.falloffExponent", subElementHandle);
			params->setParamF32(subElementHandle, behaviorGroupDesc.damageSpread.falloffExponent);

			elementHandle.getChildHandle(params, "damageColorSpread.minimumRadius", subElementHandle);
			params->setParamF32(subElementHandle, behaviorGroupDesc.damageColorSpread.minimumRadius);

			elementHandle.getChildHandle(params, "damageColorSpread.radiusMultiplier", subElementHandle);
			params->setParamF32(subElementHandle, behaviorGroupDesc.damageColorSpread.radiusMultiplier);

			elementHandle.getChildHandle(params, "damageColorSpread.falloffExponent", subElementHandle);
			params->setParamF32(subElementHandle, behaviorGroupDesc.damageColorSpread.falloffExponent);

			elementHandle.getChildHandle(params, "damageColorChange", subElementHandle);
			params->setParamVec4(subElementHandle, behaviorGroupDesc.damageColorChange);

			elementHandle.getChildHandle(params, "materialStrength", subElementHandle);
			params->setParamF32(subElementHandle, behaviorGroupDesc.materialStrength);

			elementHandle.getChildHandle(params, "density", subElementHandle);
			params->setParamF32(subElementHandle, behaviorGroupDesc.density);

			elementHandle.getChildHandle(params, "fadeOut", subElementHandle);
			params->setParamF32(subElementHandle, behaviorGroupDesc.fadeOut);

			elementHandle.getChildHandle(params, "maxDepenetrationVelocity", subElementHandle);
			params->setParamF32(subElementHandle, behaviorGroupDesc.maxDepenetrationVelocity);

			elementHandle.getChildHandle(params, "userData", subElementHandle);
			params->setParamU64(subElementHandle, behaviorGroupDesc.userData);
		}
	} ConvertBehaviorGroupDesc; // local function definitions illegal, eh?

	const uint32_t bufferCount = 128;
	char buffer[bufferCount] = {0};

	for (uint32_t i = 0; i < numBehaviorGroups; ++i)
	{
		const DestructibleBehaviorGroupDesc& chunkDesc = cookingDesc.behaviorGroupDescs[i];
		NvParameterized::Handle elementHandle(*mParams);

		shdfnd::snprintf(buffer, bufferCount, "behaviorGroups[%d]", i);
		mParams->getParameterHandle(buffer, elementHandle);

		ConvertBehaviorGroupDesc(chunkDesc, mParams, elementHandle);
	}

	mParams->getParameterHandle("defaultBehaviorGroup", handle);
	ConvertBehaviorGroupDesc(cookingDesc.defaultBehaviorGroupDesc, mParams, handle);

	mParams->RTFractureBehaviorGroup = cookingDesc.RTFractureBehaviorGroup;


	// Fill in default destructible parameters up to depth
	int oldDepthParametersSize = mParams->depthParameters.arraySizes[0];
	if (oldDepthParametersSize < (int)mParams->depthCount)
	{
		NvParameterized::Handle depthParametersHandle(*mParams);
		mParams->getParameterHandle("depthParameters", depthParametersHandle);
		mParams->resizeArray(depthParametersHandle, (int32_t)mParams->depthCount);
		for (int i = oldDepthParametersSize; i < (int)mParams->depthCount; ++i)
		{
			DestructibleAssetParametersNS::DestructibleDepthParameters_Type& depthParameters = mParams->depthParameters.buf[i];
			depthParameters.OVERRIDE_IMPACT_DAMAGE = false;
			depthParameters.OVERRIDE_IMPACT_DAMAGE_VALUE = false;
			depthParameters.IGNORE_POSE_UPDATES = false;
			depthParameters.IGNORE_RAYCAST_CALLBACKS = false;
			depthParameters.IGNORE_CONTACT_CALLBACKS = false;
			depthParameters.USER_FLAG_0 = false;
			depthParameters.USER_FLAG_1 = false;
			depthParameters.USER_FLAG_2 = false;
			depthParameters.USER_FLAG_3 = false;
		}
	}

	// Build collision data and bounds
	float skinWidth = 0.0025f;	// Default value
	if (GetApexSDK()->getCookingInterface())
	{
		const PxCookingParams& cookingParams = GetApexSDK()->getCookingInterface()->getParams();
		skinWidth = cookingParams.skinWidth;
	}

	mParams->bounds.setEmpty();
	for (uint32_t partIndex = 0; partIndex < numParts; ++partIndex)
	{
		const DestructibleGeometryDesc& geometryDesc = cookingDesc.geometryDescs[partIndex];
		const uint32_t startHullIndex = mParams->chunkConvexHullStartIndices.buf[partIndex];
		for (uint32_t hullIndex = 0; hullIndex < geometryDesc.convexHullCount; ++hullIndex)
		{
			ConvexHullImpl& chunkHullData = chunkConvexHulls[startHullIndex + hullIndex];
			PartConvexHullProxy* convexHullProxy = (PartConvexHullProxy*)geometryDesc.convexHulls[hullIndex];
			chunkHullData.mParams->copy(*convexHullProxy->impl.mParams);
			if (chunkHullData.isEmpty())
			{
				chunkHullData.buildFromAABB(renderMeshAsset->getBounds(partIndex));	// \todo: need better way of simplifying
			}
		}
	}

	for (uint32_t chunkIndex = 0; chunkIndex < numChunks; ++chunkIndex)
	{
		DestructibleAssetParametersNS::Chunk_Type& chunk = mParams->chunks.buf[chunkIndex];
		PxBounds3 bounds = getChunkActorLocalBounds(chunkIndex);
		mParams->bounds.include(bounds);
		chunk.surfaceNormal = PxVec3(0.0f);
	}
	PX_ASSERT(!mParams->bounds.isEmpty());
	mParams->bounds.fattenFast(skinWidth);

	mParams->originalDepthCount = mParams->depthCount;

	calculateChunkDepthStarts();

	if (cacheOverlaps)
	{
		cacheChunkOverlapsUpToDepth(chunkOverlapCacheDepth);
	}

	Array<IntPair> supportGraphEdgesInternal(cookingDesc.supportGraphEdgeCount);
	if (cookingDesc.supportGraphEdgeCount > 0)
	{
		for (uint32_t i = 0; i < cookingDesc.supportGraphEdgeCount; ++i)
		{
			supportGraphEdgesInternal[i].i0 = (int32_t)chunkIndexMapUser2Apex[(uint32_t)cookingDesc.supportGraphEdges[i].i0];
			supportGraphEdgesInternal[i].i1 = (int32_t)chunkIndexMapUser2Apex[(uint32_t)cookingDesc.supportGraphEdges[i].i1];
		}

		addChunkOverlaps(&supportGraphEdgesInternal[0], supportGraphEdgesInternal.size());
	}

	m_needsScatterMeshInstanceInfoCreation = true;
}

void DestructibleAssetAuthoringImpl::serializeFractureToolState(PxFileBuf& stream, nvidia::ExplicitHierarchicalMesh::Embedding& embedding) const
{
	stream.storeDword((uint32_t)ApexStreamVersion::Current);
	hMesh.serialize(stream, embedding);
	hMeshCore.serialize(stream, embedding);
	cutoutSet.serialize(stream);
}

void DestructibleAssetAuthoringImpl::deserializeFractureToolState(PxFileBuf& stream, nvidia::ExplicitHierarchicalMesh::Embedding& embedding)
{
	const uint32_t version = stream.readDword();
	PX_UNUSED(version);	// Initial version

	hMesh.deserialize(stream, embedding);
	hMeshCore.deserialize(stream, embedding);
	cutoutSet.deserialize(stream);
}
#endif

NvParameterized::Interface* DestructibleAssetImpl::getDefaultActorDesc()
{
	NvParameterized::Interface* ret = 0;

	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();

	// non-optimal.  Should use a copy-constructor so this only gets built once.
	if (!mApexDestructibleActorParams && traits)
	{
		mApexDestructibleActorParams = traits->createNvParameterized(DestructibleActorParam::staticClassName());
	}

	if (traits)
	{
		if (mApexDestructibleActorParams)
		{
			ret = mApexDestructibleActorParams;
			DestructibleActorParam* p = static_cast<DestructibleActorParam*>(ret);
			DestructibleActorParamNS::ParametersStruct* ps = static_cast<DestructibleActorParamNS::ParametersStruct*>(p);

			{
				NvParameterized::Handle handle(*p);
				if (p->getParameterHandle("crumbleEmitterName", handle) == NvParameterized::ERROR_NONE)
				{
					p->setParamString(handle, getCrumbleEmitterName());
				}
			}

			{
				NvParameterized::Handle handle(*p);
				if (p->getParameterHandle("dustEmitterName", handle) == NvParameterized::ERROR_NONE)
				{
					p->setParamString(handle, getDustEmitterName());
				}
			}

			DestructibleParameters destructibleParameters = getParameters();

			ps->destructibleParameters.damageCap								= destructibleParameters.damageCap;
			ps->destructibleParameters.forceToDamage							= destructibleParameters.forceToDamage;
			ps->destructibleParameters.impactVelocityThreshold					= destructibleParameters.impactVelocityThreshold;
			ps->destructibleParameters.minimumFractureDepth						= destructibleParameters.minimumFractureDepth;
			ps->destructibleParameters.damageDepthLimit							= destructibleParameters.damageDepthLimit;
			ps->destructibleParameters.impactDamageDefaultDepth					= destructibleParameters.impactDamageDefaultDepth;
			ps->destructibleParameters.debrisDepth								= destructibleParameters.debrisDepth;
			ps->destructibleParameters.essentialDepth							= destructibleParameters.essentialDepth;
			ps->destructibleParameters.debrisLifetimeMin						= destructibleParameters.debrisLifetimeMin;
			ps->destructibleParameters.debrisLifetimeMax						= destructibleParameters.debrisLifetimeMax;
			ps->destructibleParameters.debrisMaxSeparationMin					= destructibleParameters.debrisMaxSeparationMin;
			ps->destructibleParameters.debrisMaxSeparationMax					= destructibleParameters.debrisMaxSeparationMax;
			ps->destructibleParameters.dynamicChunksGroupsMask.useGroupsMask	= destructibleParameters.useDynamicChunksGroupsMask;
			ps->destructibleParameters.debrisDestructionProbability             = destructibleParameters.debrisDestructionProbability;
			ps->destructibleParameters.dynamicChunkDominanceGroup				= destructibleParameters.dynamicChunksDominanceGroup;
			ps->destructibleParameters.dynamicChunksGroupsMask.bits0			= destructibleParameters.dynamicChunksFilterData.word0;
			ps->destructibleParameters.dynamicChunksGroupsMask.bits1			= destructibleParameters.dynamicChunksFilterData.word1;
			ps->destructibleParameters.dynamicChunksGroupsMask.bits2			= destructibleParameters.dynamicChunksFilterData.word2;
			ps->destructibleParameters.dynamicChunksGroupsMask.bits3			= destructibleParameters.dynamicChunksFilterData.word3;
			ps->destructibleParameters.supportStrength							= destructibleParameters.supportStrength;
			ps->destructibleParameters.legacyChunkBoundsTestSetting				= destructibleParameters.legacyChunkBoundsTestSetting;
			ps->destructibleParameters.legacyDamageRadiusSpreadSetting			= destructibleParameters.legacyDamageRadiusSpreadSetting;
			ps->destructibleParameters.validBounds								= destructibleParameters.validBounds;
			ps->destructibleParameters.maxChunkSpeed							= destructibleParameters.maxChunkSpeed;
			ps->destructibleParameters.flags.ACCUMULATE_DAMAGE					= (destructibleParameters.flags & DestructibleParametersFlag::ACCUMULATE_DAMAGE) ? true : false;
			ps->destructibleParameters.flags.DEBRIS_TIMEOUT						= (destructibleParameters.flags & DestructibleParametersFlag::DEBRIS_TIMEOUT) ? true : false;
			ps->destructibleParameters.flags.DEBRIS_MAX_SEPARATION				= (destructibleParameters.flags & DestructibleParametersFlag::DEBRIS_MAX_SEPARATION) ? true : false;
			ps->destructibleParameters.flags.CRUMBLE_SMALLEST_CHUNKS			= (destructibleParameters.flags & DestructibleParametersFlag::CRUMBLE_SMALLEST_CHUNKS) ? true : false;
			ps->destructibleParameters.flags.ACCURATE_RAYCASTS					= (destructibleParameters.flags & DestructibleParametersFlag::ACCURATE_RAYCASTS) ? true : false;
			ps->destructibleParameters.flags.USE_VALID_BOUNDS					= (destructibleParameters.flags & DestructibleParametersFlag::USE_VALID_BOUNDS) ? true : false;
			ps->destructibleParameters.flags.CRUMBLE_VIA_RUNTIME_FRACTURE			= (destructibleParameters.flags & DestructibleParametersFlag::CRUMBLE_VIA_RUNTIME_FRACTURE) ? true : false;

			ps->supportDepth			= mParams->supportDepth;
			ps->formExtendedStructures	= mParams->formExtendedStructures;
			ps->useAssetDefinedSupport	= mParams->useAssetDefinedSupport;
			ps->useWorldSupport			= mParams->useWorldSupport;

			// RT Fracture Parameters
			ps->destructibleParameters.runtimeFracture.sheetFracture			= destructibleParameters.rtFractureParameters.sheetFracture;
			ps->destructibleParameters.runtimeFracture.depthLimit				= destructibleParameters.rtFractureParameters.depthLimit;
			ps->destructibleParameters.runtimeFracture.destroyIfAtDepthLimit	= destructibleParameters.rtFractureParameters.destroyIfAtDepthLimit;
			ps->destructibleParameters.runtimeFracture.minConvexSize			= destructibleParameters.rtFractureParameters.minConvexSize;
			ps->destructibleParameters.runtimeFracture.impulseScale				= destructibleParameters.rtFractureParameters.impulseScale;
			ps->destructibleParameters.runtimeFracture.glass.numSectors			= destructibleParameters.rtFractureParameters.glass.numSectors;
			ps->destructibleParameters.runtimeFracture.glass.sectorRand			= destructibleParameters.rtFractureParameters.glass.sectorRand;
			ps->destructibleParameters.runtimeFracture.glass.firstSegmentSize	= destructibleParameters.rtFractureParameters.glass.firstSegmentSize;
			ps->destructibleParameters.runtimeFracture.glass.segmentScale		= destructibleParameters.rtFractureParameters.glass.segmentScale;
			ps->destructibleParameters.runtimeFracture.glass.segmentRand		= destructibleParameters.rtFractureParameters.glass.segmentRand;
			ps->destructibleParameters.runtimeFracture.attachment.posX			= destructibleParameters.rtFractureParameters.attachment.posX;
			ps->destructibleParameters.runtimeFracture.attachment.negX			= destructibleParameters.rtFractureParameters.attachment.negX;
			ps->destructibleParameters.runtimeFracture.attachment.posY			= destructibleParameters.rtFractureParameters.attachment.posY;
			ps->destructibleParameters.runtimeFracture.attachment.negY			= destructibleParameters.rtFractureParameters.attachment.negY;
			ps->destructibleParameters.runtimeFracture.attachment.posZ			= destructibleParameters.rtFractureParameters.attachment.posZ;
			ps->destructibleParameters.runtimeFracture.attachment.negZ			= destructibleParameters.rtFractureParameters.attachment.negZ;

			uint32_t depth = destructibleParameters.depthParametersCount;
			if (depth > 0)
			{
				NvParameterized::Handle handle(*p);
				if (p->getParameterHandle("depthParameters", handle) == NvParameterized::ERROR_NONE)
				{
					p->resizeArray(handle, (int32_t)depth);
					for (uint32_t i = 0; i < depth; i++)
					{
						const DestructibleDepthParameters& dparm = destructibleParameters.depthParameters[i];
						ps->depthParameters.buf[i].OVERRIDE_IMPACT_DAMAGE		= (dparm.flags & DestructibleDepthParametersFlag::OVERRIDE_IMPACT_DAMAGE) ? true : false;
						ps->depthParameters.buf[i].OVERRIDE_IMPACT_DAMAGE_VALUE	= (dparm.flags & DestructibleDepthParametersFlag::OVERRIDE_IMPACT_DAMAGE_VALUE) ? true : false;
						ps->depthParameters.buf[i].IGNORE_POSE_UPDATES			= (dparm.flags & DestructibleDepthParametersFlag::IGNORE_POSE_UPDATES) ? true : false;
						ps->depthParameters.buf[i].IGNORE_RAYCAST_CALLBACKS		= (dparm.flags & DestructibleDepthParametersFlag::IGNORE_RAYCAST_CALLBACKS) ? true : false;
						ps->depthParameters.buf[i].IGNORE_CONTACT_CALLBACKS		= (dparm.flags & DestructibleDepthParametersFlag::IGNORE_CONTACT_CALLBACKS) ? true : false;
						ps->depthParameters.buf[i].USER_FLAG_0					= (dparm.flags & DestructibleDepthParametersFlag::USER_FLAG_0) ? true : false;
						ps->depthParameters.buf[i].USER_FLAG_1					= (dparm.flags & DestructibleDepthParametersFlag::USER_FLAG_1) ? true : false;
						ps->depthParameters.buf[i].USER_FLAG_2					= (dparm.flags & DestructibleDepthParametersFlag::USER_FLAG_2) ? true : false;
						ps->depthParameters.buf[i].USER_FLAG_3					= (dparm.flags & DestructibleDepthParametersFlag::USER_FLAG_3) ? true : false;
					}
				}
			}

			{
				NvParameterized::Handle handle(*p);
				if (p->getParameterHandle("behaviorGroups", handle) == NvParameterized::ERROR_NONE)
				{
					uint32_t behaviorGroupArraySize = (uint32_t)mParams->behaviorGroups.arraySizes[0];
					p->resizeArray(handle, (int32_t)behaviorGroupArraySize);

					struct {
						void operator() (const DestructibleAssetParametersNS::BehaviorGroup_Type& src,
										 DestructibleActorParamNS::BehaviorGroup_Type& dest)
						{
							dest.damageThreshold = src.damageThreshold;
							dest.damageToRadius = src.damageToRadius;
							dest.damageSpread.minimumRadius = src.damageSpread.minimumRadius;
							dest.damageSpread.radiusMultiplier = src.damageSpread.radiusMultiplier;
							dest.damageSpread.falloffExponent = src.damageSpread.falloffExponent;
							dest.materialStrength = src.materialStrength;
							dest.density = src.density;
							dest.fadeOut = src.fadeOut;
							dest.maxDepenetrationVelocity = src.maxDepenetrationVelocity;
						}						
					} ConvertBehaviorGroup;

					ConvertBehaviorGroup(mParams->defaultBehaviorGroup, ps->defaultBehaviorGroup);
					for (uint32_t i = 0; i < behaviorGroupArraySize; i++)
					{
						ConvertBehaviorGroup(mParams->behaviorGroups.buf[i],
											 ps->behaviorGroups.buf[i]);
					}
				}
			}
		}
	}
	return ret;
}

NvParameterized::Interface* DestructibleAssetImpl::getDefaultAssetPreviewDesc()
{
	NvParameterized::Interface* ret = NULL;

	if (module != NULL)
	{
		ret = module->getApexDestructiblePreviewParams();

		if (ret != NULL)
		{
			ret->initDefaults();
		}
	}

	return ret;
}

bool DestructibleAssetImpl::isValidForActorCreation(const ::NvParameterized::Interface& params, Scene& /*apexScene*/) const
{
	return nvidia::strcmp(params.className(), DestructibleActorParam::staticClassName()) == 0 ||
	       nvidia::strcmp(params.className(), DestructibleActorState::staticClassName()) == 0;
}

Actor* DestructibleAssetImpl::createApexActor(const NvParameterized::Interface& params, Scene& apexScene)
{
	if (!isValidForActorCreation(params, apexScene))
	{
		return NULL;
	}

	return createDestructibleActor(params, apexScene);
}

AssetPreview* DestructibleAssetImpl::createApexAssetPreview(const NvParameterized::Interface& params, AssetPreviewScene* /*previewScene*/)
{
	DestructiblePreviewProxy* proxy = NULL;

	proxy = PX_NEW(DestructiblePreviewProxy)(*this, m_previewList, &params);
	PX_ASSERT(proxy != NULL);

	return proxy;
}

void DestructibleAssetImpl::appendActorTransforms(const PxMat44* transforms, uint32_t transformCount)
		{
	NvParameterized::Handle handle(*mParams);
	mParams->getParameterHandle("actorTransforms", handle);
	const int32_t oldSize = mParams->actorTransforms.arraySizes[0];
	mParams->resizeArray(handle, (int32_t)(oldSize + transformCount));
	mParams->setParamMat44Array(handle, transforms, (int32_t)transformCount, oldSize);
		}

void DestructibleAssetImpl::clearActorTransforms()
{
	NvParameterized::Handle handle(*mParams);
	mParams->getParameterHandle("actorTransforms", handle);
	mParams->resizeArray(handle, 0);
}

bool DestructibleAssetImpl::chunksInProximity(const DestructibleAssetImpl& asset0, uint16_t chunkIndex0, const PxTransform& tm0, const PxVec3& scale0,
        const DestructibleAssetImpl& asset1, uint16_t chunkIndex1, const PxTransform& tm1, const PxVec3& scale1,
        float padding)
{
	PX_ASSERT(asset0.mParams != NULL);
	PX_ASSERT(asset1.mParams != NULL);

	// Offset chunks (in case they are instanced)
	PxTransform effectiveTM0(tm0);
	effectiveTM0.p = tm0.p + tm0.rotate(scale0.multiply(asset0.getChunkPositionOffset(chunkIndex0)));

	PxTransform effectiveTM1(tm1);
	effectiveTM1.p = tm1.p + tm1.rotate(scale1.multiply(asset1.getChunkPositionOffset(chunkIndex1)));

	for (uint32_t hullIndex0 = asset0.getChunkHullIndexStart(chunkIndex0); hullIndex0 < asset0.getChunkHullIndexStop(chunkIndex0); ++hullIndex0)
	{
		for (uint32_t hullIndex1 = asset1.getChunkHullIndexStart(chunkIndex1); hullIndex1 < asset1.getChunkHullIndexStop(chunkIndex1); ++hullIndex1)
		{
			if (ConvexHullImpl::hullsInProximity(asset0.chunkConvexHulls[hullIndex0], effectiveTM0, scale0, asset1.chunkConvexHulls[hullIndex1], effectiveTM1, scale1, padding))
			{
				return true;
			}
		}
	}
	return false;
}

bool DestructibleAssetImpl::chunkAndSphereInProximity(uint16_t chunkIndex, const PxTransform& chunkTM, const PxVec3& chunkScale, 
												  const PxVec3& sphereWorldCenter, float sphereRadius, float padding, float* distance)
{
	// Offset chunk (in case it is instanced)
	PxTransform effectiveTM = chunkTM;
	effectiveTM.p = chunkTM.p + chunkTM.rotate(chunkScale.multiply(getChunkPositionOffset(chunkIndex)));

	ConvexHullImpl::Separation testSeparation;
	ConvexHullImpl::Separation* testSeparationPtr = distance != NULL? &testSeparation : NULL;
	bool result = false;
	for (uint32_t hullIndex = getChunkHullIndexStart(chunkIndex); hullIndex < getChunkHullIndexStop(chunkIndex); ++hullIndex)
	{
		if (chunkConvexHulls[hullIndex].sphereInProximity(effectiveTM, chunkScale, sphereWorldCenter, sphereRadius, padding, testSeparationPtr))
		{
			result = true;
			if (distance != NULL)
			{
				const float testDistance = testSeparation.getDistance();
				if (testDistance < *distance)
				{
					*distance = testDistance;
				}
			}
		}
	}
	return result;
}


/*
	DestructibleAssetCollision
*/


DestructibleAssetCollision::DestructibleAssetCollision() :
	mAsset(NULL)
{
	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
	mParams = DYNAMIC_CAST(DestructibleAssetCollisionDataSet*)(traits->createNvParameterized(DestructibleAssetCollisionDataSet::staticClassName()));
	mOwnsParams = mParams != NULL;
	PX_ASSERT(mOwnsParams);
}

DestructibleAssetCollision::DestructibleAssetCollision(NvParameterized::Interface* params) :
	mAsset(NULL)
{
	mParams = DYNAMIC_CAST(DestructibleAssetCollisionDataSet*)(params);
	mOwnsParams = false;
}

DestructibleAssetCollision::~DestructibleAssetCollision()
{
	resize(0);

	if (mParams != NULL && mOwnsParams)
	{
		mParams->destroy();
	}
	mParams = NULL;
	mOwnsParams = false;
}

void DestructibleAssetCollision::setDestructibleAssetToCook(DestructibleAssetImpl* asset)
{
	if (asset == NULL || getAssetName() == NULL || nvidia::strcmp(asset->getName(), getAssetName()))
	{
		resize(0);
	}

	mAsset = asset;

	NvParameterized::Handle handle(*mParams);
	mParams->getParameterHandle("assetName", handle);
	mParams->setParamString(handle, mAsset != NULL ? mAsset->getName() : "");
}

void DestructibleAssetCollision::resize(uint32_t hullCount)
{
	for (uint32_t i = 0; i < mConvexMeshContainer.size(); ++i)
	{
		physx::Array< PxConvexMesh* >& convexMeshSet = mConvexMeshContainer[i];
		for (uint32_t j = hullCount; j < convexMeshSet.size(); ++j)
		{
			if (convexMeshSet[j] != NULL)
			{
				convexMeshSet[j]->release();
			}
		}
		mConvexMeshContainer[i].resize(hullCount);
	}

	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
	for (int i = 0; i < mParams->meshCookedCollisionStreamsAtScale.arraySizes[0]; ++i)
	{
		MeshCookedCollisionStreamsAtScale* streamsAtScale = DYNAMIC_CAST(MeshCookedCollisionStreamsAtScale*)(mParams->meshCookedCollisionStreamsAtScale.buf[i]);
		if (streamsAtScale == NULL)
		{
			streamsAtScale = DYNAMIC_CAST(MeshCookedCollisionStreamsAtScale*)(traits->createNvParameterized(MeshCookedCollisionStreamsAtScale::staticClassName()));
			mParams->meshCookedCollisionStreamsAtScale.buf[i] = streamsAtScale;
		}
		NvParameterized::Handle handle(*streamsAtScale);
		streamsAtScale->getParameterHandle("meshCookedCollisionStreams", handle);

		// resizing PxParam ref arrays doesn't call destroy(), do this here
		int32_t currentArraySize = 0;
		streamsAtScale->getArraySize(handle, currentArraySize);
		for (int j = currentArraySize - 1; j >= (int32_t)hullCount; j--)
		{
			NvParameterized::Interface*& hullStream = streamsAtScale->meshCookedCollisionStreams.buf[j];
			if (hullStream != NULL)
			{
				hullStream->destroy();
			}
			hullStream = NULL;
		}
		streamsAtScale->resizeArray(handle, (int32_t)hullCount);
	}
}

bool DestructibleAssetCollision::addScale(const PxVec3& scale)
{
	if (getScaleIndex(scale, kDefaultDestructibleAssetCollisionScaleTolerance) >= 0)
	{
		return false;	// Scale already exists
	}

	int scaleIndex = mParams->scales.arraySizes[0];

	NvParameterized::Handle handle(*mParams);
	mParams->getParameterHandle("scales", handle);
	mParams->resizeArray(handle, scaleIndex + 1);
	mParams->getParameterHandle("meshCookedCollisionStreamsAtScale", handle);
	mParams->resizeArray(handle, scaleIndex + 1);

	mConvexMeshContainer.resize((uint32_t)scaleIndex + 1);

	mParams->scales.buf[(uint32_t)scaleIndex] = scale;
	mParams->meshCookedCollisionStreamsAtScale.buf[(uint32_t)scaleIndex] = NULL;
	mConvexMeshContainer[(uint32_t)scaleIndex].reset();

	return true;
}

bool DestructibleAssetCollision::cookAll()
{
	bool result = true;
	for (int i = 0; i < mParams->scales.arraySizes[0]; ++i)
	{
		if (!cookScale(mParams->scales.buf[i]))
		{
			result = false;
		}
	}

	return result;
}

bool DestructibleAssetCollision::cookScale(const PxVec3& scale)
{
	if (mAsset == NULL)
	{
		return false;
	}

	const int32_t partCount = mAsset->mParams->chunkConvexHullStartIndices.arraySizes[0];
	if (partCount <= 0)
	{
		return false;
	}

	const uint32_t hullCount = mAsset->mParams->chunkConvexHullStartIndices.buf[partCount-1];

	bool result = true;
	for (uint16_t i = 0; i < hullCount; ++i)
	{
		PxConvexMesh* convexMesh = getConvexMesh(i, scale);
		if (convexMesh == NULL)
		{
			result = false;
		}
	}

	return result;
}

PxConvexMesh* DestructibleAssetCollision::getConvexMesh(uint32_t hullIndex, const PxVec3& scale)
{
	int32_t scaleIndex = getScaleIndex(scale, kDefaultDestructibleAssetCollisionScaleTolerance);

	if (scaleIndex >= 0)
	{
		if (scaleIndex < (int32_t)mConvexMeshContainer.size())
		{
			physx::Array<PxConvexMesh*>& convexMeshSet = mConvexMeshContainer[(uint32_t)scaleIndex];
			if (hullIndex < convexMeshSet.size())
			{
				PxConvexMesh* convexMesh = convexMeshSet[hullIndex];
				if (convexMesh != NULL)
				{
					return convexMesh;
				}
			}
		}
	}

	NvParameterized::Handle handle(*mParams);
	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();

	MeshCookedCollisionStreamsAtScale* streamsAtScale;
	if (scaleIndex < 0)
	{
		scaleIndex = mParams->scales.arraySizes[0];

		mParams->getParameterHandle("scales", handle);
		mParams->resizeArray(handle, scaleIndex + 1);
		mParams->getParameterHandle("meshCookedCollisionStreamsAtScale", handle);
		mParams->resizeArray(handle, scaleIndex + 1);

		mConvexMeshContainer.resize((uint32_t)scaleIndex + 1);

		mParams->scales.buf[(uint32_t)scaleIndex] = scale;
	}

	if (mParams->meshCookedCollisionStreamsAtScale.buf[scaleIndex] != NULL)
	{
		streamsAtScale = DYNAMIC_CAST(MeshCookedCollisionStreamsAtScale*)(mParams->meshCookedCollisionStreamsAtScale.buf[scaleIndex]);
	}
	else
	{
		streamsAtScale = DYNAMIC_CAST(MeshCookedCollisionStreamsAtScale*)(traits->createNvParameterized(MeshCookedCollisionStreamsAtScale::staticClassName()));
		mParams->meshCookedCollisionStreamsAtScale.buf[scaleIndex] = streamsAtScale;
	}

	if (mAsset != NULL && (int)mAsset->chunkConvexHulls.size() != streamsAtScale->meshCookedCollisionStreams.arraySizes[0])
	{
		NvParameterized::Handle streamsHandle(*streamsAtScale);
		streamsAtScale->getParameterHandle("meshCookedCollisionStreams", streamsHandle);
		streamsHandle.resizeArray((int32_t)mAsset->chunkConvexHulls.size());
		mConvexMeshContainer[(uint32_t)scaleIndex].resize(mAsset->chunkConvexHulls.size());
	}

	if ((int)hullIndex >= streamsAtScale->meshCookedCollisionStreams.arraySizes[0])
	{
		return NULL;
	}

	PxConvexMesh* convexMesh = NULL;

	MeshCookedCollisionStream* stream = DYNAMIC_CAST(MeshCookedCollisionStream*)(streamsAtScale->meshCookedCollisionStreams.buf[hullIndex]);
	if (stream == NULL)
	{
		if (mAsset == NULL)
		{
			return NULL;
		}

		stream = DYNAMIC_CAST(MeshCookedCollisionStream*)(traits->createNvParameterized(MeshCookedCollisionStream::staticClassName()));
		streamsAtScale->meshCookedCollisionStreams.buf[hullIndex] = stream;

		PX_PROFILE_ZONE("DestructibleCookChunkCollisionMeshes", GetInternalApexSDK()->getContextId());

		// Update the asset's stats with the number of cooked collision convex meshes
		if (mAsset != NULL)
		{
			++mAsset->mRuntimeCookedConvexCount;
		}

		const ConvexHullImpl& hullData = mAsset->chunkConvexHulls[hullIndex];

		if (hullData.getVertexCount() == 0)
		{
			return NULL;
		}

		Array<PxVec3> scaledPoints;
		scaledPoints.resize(hullData.getVertexCount());
		PxVec3 centroid(0.0f);
		for (uint32_t i = 0; i < scaledPoints.size(); ++i)
		{
			scaledPoints[i] = hullData.getVertex(i);	// Cook at unit scale first
			centroid += scaledPoints[i];
		}
		centroid *= 1.0f/(float)scaledPoints.size();
		for (uint32_t i = 0; i < scaledPoints.size(); ++i)
		{
			scaledPoints[i] -= centroid;
		}

		nvidia::PsMemoryBuffer memStream;
		memStream.setEndianMode(PxFileBuf::ENDIAN_NONE);
		PxStreamFromFileBuf nvs(memStream);
		physx::PxConvexMeshDesc meshDesc;
		meshDesc.points.count = scaledPoints.size();
		meshDesc.points.data = scaledPoints.begin();
		meshDesc.points.stride = sizeof(PxVec3);
		meshDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;
		const float skinWidth = GetApexSDK()->getCookingInterface() != NULL ? GetApexSDK()->getCookingInterface()->getParams().skinWidth : 0.0f;
		if (skinWidth > 0.0f)
		{
			meshDesc.flags |= physx::PxConvexFlag::eINFLATE_CONVEX;
		}
		bool success = GetApexSDK()->getCookingInterface()->cookConvexMesh(meshDesc, nvs);

		// Now scale all the points, in case this array is used as-is (failure cases)
		for (uint32_t i = 0; i < scaledPoints.size(); ++i)
		{
			scaledPoints[i] += centroid;
			scaledPoints[i] = scaledPoints[i].multiply(scale);
		}

		if (success)
		{
			PxStreamFromFileBuf nvs(memStream);
			convexMesh = GetApexSDK()->getPhysXSDK()->createConvexMesh(nvs);

			// Examine the mass properties to make sure they're reasonable.
			if (convexMesh != NULL)
			{
				float mass;
				PxMat33 localInertia;
				PxVec3 localCenterOfMass;
				convexMesh->getMassInformation(mass, localInertia, localCenterOfMass);
				PxMat33 massFrame;
				const PxVec3 massLocalInertia = diagonalizeSymmetric(massFrame, localInertia);
				success = (mass > 0 && massLocalInertia.x > 0 && massLocalInertia.y > 0 && massLocalInertia.z > 0);
				if (success && massLocalInertia.maxElement() > 4000*massLocalInertia.minElement())
				{
					convexMesh->release();
					convexMesh = NULL;
					success = false;
				}
			}
			else
			{
				success = false;
			}

			if (success)
			{
				// Now scale the convex hull
				memStream.reset();
				memStream.setEndianMode(PxFileBuf::ENDIAN_NONE);
				PxStreamFromFileBuf nvs(memStream);

				const uint32_t numVerts = convexMesh->getNbVertices();
				const PxVec3* verts = convexMesh->getVertices();

				scaledPoints.resize(numVerts);
				for (uint32_t i = 0; i < numVerts; ++i)
				{
					scaledPoints[i] = (verts[i] + centroid).multiply(scale);
				}

				// Unfortunately, we must build our own triangle buffer from the polygon buffer
				uint32_t triangleCount = 0;
				for (uint32_t i = 0; i < convexMesh->getNbPolygons(); ++i)
				{
					physx::PxHullPolygon polygon;
					convexMesh->getPolygonData(i, polygon);
					triangleCount += polygon.mNbVerts - 2;
				}
				const uint8_t* indexBuffer = (const uint8_t*)convexMesh->getIndexBuffer();
				Array<uint32_t> indices;
				indices.reserve(triangleCount*3);
				for (uint32_t i = 0; i < convexMesh->getNbPolygons(); ++i)
				{
					physx::PxHullPolygon polygon;
					convexMesh->getPolygonData(i, polygon);
					for (uint16_t j = 1; j < polygon.mNbVerts-1; ++j)
					{
						indices.pushBack((uint32_t)indexBuffer[polygon.mIndexBase]);
						indices.pushBack((uint32_t)indexBuffer[polygon.mIndexBase+j]);
						indices.pushBack((uint32_t)indexBuffer[polygon.mIndexBase+j+1]);
					}
				}

				physx::PxConvexMeshDesc meshDesc;
				meshDesc.points.count = scaledPoints.size();
				meshDesc.points.data = scaledPoints.begin();
				meshDesc.points.stride = sizeof(PxVec3);
				meshDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;
				success = GetApexSDK()->getCookingInterface()->cookConvexMesh(meshDesc, nvs);

				convexMesh->release();
				convexMesh = NULL;

				if (success)
				{
					convexMesh = GetApexSDK()->getPhysXSDK()->createConvexMesh(nvs);
				}

				if (convexMesh == NULL)
				{
					success = false;
				}
			}
		}

		if (!success)
		{
			convexMesh = NULL;
			memStream.reset();
			memStream.setEndianMode(PxFileBuf::ENDIAN_NONE);
			PxStreamFromFileBuf nvs(memStream);
			// Just form bbox
			PxBounds3 bounds;
			bounds.setEmpty();
			for (uint32_t i = 0; i < scaledPoints.size(); ++i)
			{
				bounds.include(scaledPoints[i]);
			}
			PX_ASSERT(!bounds.isEmpty());
			bounds.fattenFast(PxMax(0.00001f, bounds.getExtents().magnitude()*0.001f));
			scaledPoints.resize(8);
			for (uint32_t i = 0; i < 8; ++i)
			{
				scaledPoints[i] = PxVec3((i & 1) ? bounds.maximum.x : bounds.minimum.x,
				                                (i & 2) ? bounds.maximum.y : bounds.minimum.y,
				                                (i & 4) ? bounds.maximum.z : bounds.minimum.z);
			}
			meshDesc.points.data = scaledPoints.begin();
			meshDesc.points.count = 8;
			if (!GetApexSDK()->getCookingInterface()->cookConvexMesh(meshDesc, nvs))
			{
				memStream.reset();
			}
		}

		{
			NvParameterized::Handle bytesHandle(stream);
			stream->getParameterHandle("bytes", bytesHandle);
			stream->resizeArray(bytesHandle, (int32_t)memStream.getWriteBufferSize());
			stream->setParamU8Array(bytesHandle, memStream.getWriteBuffer(), (int32_t)memStream.getWriteBufferSize());
		}
	}

	if (convexMesh == NULL)
	{
		nvidia::PsMemoryBuffer memStream(stream->bytes.buf, (uint32_t)stream->bytes.arraySizes[0]);
		memStream.setEndianMode(PxFileBuf::ENDIAN_NONE);
		PxStreamFromFileBuf nvs(memStream);
		convexMesh = GetApexSDK()->getPhysXSDK()->createConvexMesh(nvs);
	}

	// These resizes should not be required, fix it and remove them
	if (mConvexMeshContainer.size() <= (uint32_t)scaleIndex)
	{
		APEX_DEBUG_WARNING("The asset's (%s) convex mesh container needed resizing, debug this", mAsset->getName());
		mConvexMeshContainer.resize((uint32_t)scaleIndex+1);
	}
	if (mConvexMeshContainer[(uint32_t)scaleIndex].size() <= (uint32_t)hullIndex)
	{
		APEX_DEBUG_WARNING("The asset's (%s) convex mesh container at scale index %d needed resizing, debug this", mAsset->getName(), scaleIndex);
		mConvexMeshContainer[(uint32_t)scaleIndex].resize(hullIndex+1);
	}
	
	mConvexMeshContainer[(uint32_t)scaleIndex][hullIndex] = convexMesh;

	return convexMesh;
}

MeshCookedCollisionStreamsAtScale* DestructibleAssetCollision::getCollisionAtScale(const PxVec3& scale)
{
	cookScale(scale);

	const int32_t scaleIndex = getScaleIndex(scale, kDefaultDestructibleAssetCollisionScaleTolerance);
	if (scaleIndex < 0)
	{
		return NULL;
	}

	return DYNAMIC_CAST(MeshCookedCollisionStreamsAtScale*)(mParams->meshCookedCollisionStreamsAtScale.buf[scaleIndex]);
}

physx::Array<PxConvexMesh*>* DestructibleAssetCollision::getConvexMeshesAtScale(const PxVec3& scale)
{
	cookScale(scale);

	const int32_t scaleIndex = getScaleIndex(scale, kDefaultDestructibleAssetCollisionScaleTolerance);
	if (scaleIndex < 0)
	{
		return NULL;
	}

	return &mConvexMeshContainer[(uint32_t)scaleIndex];
}

PxFileBuf& DestructibleAssetCollision::deserialize(PxFileBuf& stream, const char* assetName)
{
	// If there are any referenced meshes in ANY scales we're going to revoke this operation as not supported
	for (uint32_t i=0; i<mConvexMeshContainer.size(); i++)
	{
		if (mConvexMeshContainer.getReferenceCount(i) > 0)
		{
			APEX_DEBUG_INFO("Cannot deserialize the cooked collision data cache for asset <%s> scaleIdx <%d> because it is in use by actors", getAssetName(), i);
			return stream;
		}
	}

	mAsset = NULL;
	mConvexMeshContainer.reset();

	/*uint32_t version =*/
	stream.readDword();	// Eat version #, not used since this is the initial version

	ApexSimpleString name;
	name.deserialize(stream);
	NvParameterized::Handle handle(*mParams);
	mParams->getParameterHandle("assetName", handle);
	mParams->setParamString(handle, name.c_str());

	if (assetName != NULL)
		mParams->setParamString(handle, assetName);

	stream >> mParams->cookingPlatform;
	stream >> mParams->cookingVersionNum;

	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();

	int scaleCount = (int)stream.readDword();
	mParams->getParameterHandle("scales", handle);
	mParams->resizeArray(handle, scaleCount);
	for (int i = 0; i < scaleCount; ++i)
	{
		stream >> mParams->scales.buf[i];
	}

	int meshScaleCount = (int)stream.readDword();
	mParams->getParameterHandle("meshCookedCollisionStreamsAtScale", handle);
	mParams->resizeArray(handle, meshScaleCount);
	mConvexMeshContainer.resize((uint32_t)meshScaleCount);
	for (uint32_t i = 0; i < (uint32_t)meshScaleCount; ++i)
	{
		MeshCookedCollisionStreamsAtScale* streamsAtScale = DYNAMIC_CAST(MeshCookedCollisionStreamsAtScale*)(mParams->meshCookedCollisionStreamsAtScale.buf[i]);
		if (streamsAtScale == NULL)
		{
			streamsAtScale = DYNAMIC_CAST(MeshCookedCollisionStreamsAtScale*)(traits->createNvParameterized(MeshCookedCollisionStreamsAtScale::staticClassName()));
			mParams->meshCookedCollisionStreamsAtScale.buf[i] = streamsAtScale;
		}

		handle.setInterface(streamsAtScale);
		physx::Array<PxConvexMesh*>& meshSet = mConvexMeshContainer[i];

		uint32_t meshCount = stream.readDword();
		streamsAtScale->getParameterHandle("meshCookedCollisionStreams", handle);
		streamsAtScale->resizeArray(handle, (int32_t)meshCount);
		meshSet.resize(meshCount);
		for (uint32_t j = 0; j < meshCount; ++j)
		{
			MeshCookedCollisionStream* collisionStream = DYNAMIC_CAST(MeshCookedCollisionStream*)(streamsAtScale->meshCookedCollisionStreams.buf[j]);
			if (collisionStream == NULL)
			{
				collisionStream = DYNAMIC_CAST(MeshCookedCollisionStream*)(traits->createNvParameterized(MeshCookedCollisionStream::staticClassName()));
				streamsAtScale->meshCookedCollisionStreams.buf[j] = collisionStream;
			}

			handle.setInterface(collisionStream);

			int bufferSize = (int)stream.readDword();
			collisionStream->getParameterHandle("bytes", handle);
			collisionStream->resizeArray(handle, bufferSize);
			stream.read(collisionStream->bytes.buf, (uint32_t)bufferSize);

			nvidia::PsMemoryBuffer memStream(collisionStream->bytes.buf, (uint32_t)collisionStream->bytes.arraySizes[0]);
			memStream.setEndianMode(PxFileBuf::ENDIAN_NONE);
			PxStreamFromFileBuf nvs(memStream);
			meshSet[j] = GetApexSDK()->getPhysXSDK()->createConvexMesh(nvs);
		}
	}
	return stream;
}

PxFileBuf& DestructibleAssetCollision::serialize(PxFileBuf& stream) const
{
#ifndef WITHOUT_APEX_AUTHORING
	stream << (uint32_t)Version::Current;

	ApexSimpleString name(mParams->assetName);
	name.serialize(stream);

	stream << mParams->cookingPlatform;
	stream << mParams->cookingVersionNum;

	stream.storeDword((uint32_t)mParams->scales.arraySizes[0]);
	for (uint32_t i = 0; i < (uint32_t)mParams->scales.arraySizes[0]; ++i)
	{
		stream << mParams->scales.buf[i];
	}

	stream.storeDword((uint32_t)mParams->meshCookedCollisionStreamsAtScale.arraySizes[0]);
	for (uint32_t i = 0; i < (uint32_t)mParams->meshCookedCollisionStreamsAtScale.arraySizes[0]; ++i)
	{
		MeshCookedCollisionStreamsAtScale* streamsAtScale = DYNAMIC_CAST(MeshCookedCollisionStreamsAtScale*)(mParams->meshCookedCollisionStreamsAtScale.buf[i]);
		if (streamsAtScale == NULL)
		{
			stream.storeDword(0);
		}
		else
		{
			stream.storeDword((uint32_t)streamsAtScale->meshCookedCollisionStreams.arraySizes[0]);
			for (uint32_t j = 0; j < (uint32_t)streamsAtScale->meshCookedCollisionStreams.arraySizes[0]; ++j)
			{
				MeshCookedCollisionStream* collisionStream = DYNAMIC_CAST(MeshCookedCollisionStream*)(streamsAtScale->meshCookedCollisionStreams.buf[j]);
				if (collisionStream == NULL)
				{
					stream.storeDword(0);
				}
				else
				{
					stream.storeDword((uint32_t)collisionStream->bytes.arraySizes[0]);
					stream.write(collisionStream->bytes.buf, (uint32_t)collisionStream->bytes.arraySizes[0]);
				}
			}
		}
	}
#endif // #ifndef WITHOUT_APEX_AUTHORING
	return stream;
}

bool DestructibleAssetCollision::platformAndVersionMatch() const
{
	const PxCookingParams& cookingParams = GetInternalApexSDK()->getCookingInterface()->getParams();
	const uint32_t presentCookingVersionNum = GetInternalApexSDK()->getCookingVersion();

	return ((uint32_t) cookingParams.targetPlatform == mParams->cookingPlatform) &&
	       ((presentCookingVersionNum & 0xFFFF0000) == (mParams->cookingVersionNum & 0xFFFF0000));
}

void DestructibleAssetCollision::setPlatformAndVersion()
{
	mParams->cookingPlatform = GetInternalApexSDK()->getCookingInterface()->getParams().targetPlatform;
	mParams->cookingVersionNum = GetInternalApexSDK()->getCookingVersion();
}

uint32_t DestructibleAssetCollision::memorySize() const
{
	uint32_t size = 0;

	for (int i = 0; i < mParams->meshCookedCollisionStreamsAtScale.arraySizes[0]; ++i)
	{
		MeshCookedCollisionStreamsAtScale* streamsAtScale = DYNAMIC_CAST(MeshCookedCollisionStreamsAtScale*)(mParams->meshCookedCollisionStreamsAtScale.buf[i]);
		if (streamsAtScale == NULL)
		{
			continue;
		}
		for (int j = 0; j < streamsAtScale->meshCookedCollisionStreams.arraySizes[0]; ++j)
		{
			MeshCookedCollisionStream* stream = DYNAMIC_CAST(MeshCookedCollisionStream*)(streamsAtScale->meshCookedCollisionStreams.buf[j]);
			if (stream == NULL)
			{
				continue;
			}
			size += (uint32_t)stream->bytes.arraySizes[0];
		}
	}

	return size;
}

void DestructibleAssetCollision::clearUnreferencedSets()
{
	for (uint32_t i = 0; i < mConvexMeshContainer.size(); ++i)
	{
		if (mConvexMeshContainer.getReferenceCount(i) == 0)
		{
			MeshCookedCollisionStreamsAtScale* streamsAtScale = DYNAMIC_CAST(MeshCookedCollisionStreamsAtScale*)(mParams->meshCookedCollisionStreamsAtScale.buf[i]);
			if (streamsAtScale)
			{
				NvParameterized::Handle handle(*streamsAtScale);
				streamsAtScale->getParameterHandle("meshCookedCollisionStreams", handle);
				streamsAtScale->resizeArray(handle, 0);
			}

			// We need to NULL this pointer, otherwise we'll be accessing old data as a result of the reset below
			DestructibleAssetImpl* asset = mAsset;
			if (asset)
			{
				asset->mCollisionMeshes = NULL;
			}
		}
	}
	mConvexMeshContainer.reset(false);
}

// Spit out warnings to the error stream for any referenced sets
void DestructibleAssetCollision::reportReferencedSets()
{
	for (uint32_t i = 0; i < mConvexMeshContainer.size(); ++i)
	{
		if (mConvexMeshContainer.getReferenceCount(i))
		{
			APEX_DEBUG_WARNING("Clearing a referenced convex mesh container for asset: %s", mAsset);
		}
	}
}

bool DestructibleAssetCollision::incReferenceCount(int scaleIndex)
{
	if (scaleIndex < 0 || scaleIndex >= (int)mConvexMeshContainer.size())
	{
		return false;
	}

	mConvexMeshContainer.incReferenceCount((uint32_t)scaleIndex);

	return true;
}

bool DestructibleAssetCollision::decReferenceCount(int scaleIndex)
{
	if (scaleIndex < 0 || scaleIndex >= (int)mConvexMeshContainer.size())
	{
		return false;
	}

	return mConvexMeshContainer.decReferenceCount((uint32_t)scaleIndex);
}

// The source 'collisionSet' is not const because it's list of PxConvexMesh pointers
// in 'mConvexMeshContainer' needs to be cleared so they aren't released in 
// DestructibleAssetCollision::resize(0)
void DestructibleAssetCollision::merge(DestructibleAssetCollision& collisionSet)
{
	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();

	// Prepare the convexMesh container for the collisionSet's meshes
	if (mConvexMeshContainer.size() < collisionSet.mConvexMeshContainer.size())
	{
		mConvexMeshContainer.resize(collisionSet.mConvexMeshContainer.size());
	}

	// Loop through scales contained in collisionSet
	for (uint32_t i = 0; i < (uint32_t)collisionSet.mParams->scales.arraySizes[0]; ++i)
	{
		const PxVec3& scale = collisionSet.mParams->scales.buf[i];
		int scaleIndex = getScaleIndex(scale, kDefaultDestructibleAssetCollisionScaleTolerance);
		if (scaleIndex < 0)
		{
			// Scale not found, add it to this set
			addScale(scale);
			scaleIndex = getScaleIndex(scale, kDefaultDestructibleAssetCollisionScaleTolerance);
			if (scaleIndex < 0)
			{
				continue;	// Failed to add scale
			}
			if (mParams->meshCookedCollisionStreamsAtScale.buf[scaleIndex] == NULL)	// Create streams if we need them
			{
				mParams->meshCookedCollisionStreamsAtScale.buf[scaleIndex] = DYNAMIC_CAST(MeshCookedCollisionStreamsAtScale*)(traits->createNvParameterized(MeshCookedCollisionStreamsAtScale::staticClassName()));
			}
			mParams->meshCookedCollisionStreamsAtScale.buf[scaleIndex]->copy(*collisionSet.mParams->meshCookedCollisionStreamsAtScale.buf[i]);
		}
		else
		{
			MeshCookedCollisionStreamsAtScale* streamsAtScale = DYNAMIC_CAST(MeshCookedCollisionStreamsAtScale*)(mParams->meshCookedCollisionStreamsAtScale.buf[scaleIndex]);
			PX_ASSERT(streamsAtScale != NULL);
			if (streamsAtScale->meshCookedCollisionStreams.arraySizes[0] == 0)	// Only merge if this scale is empty; we won't stomp any existing data
			{
				mParams->meshCookedCollisionStreamsAtScale.buf[scaleIndex]->copy(*collisionSet.mParams->meshCookedCollisionStreamsAtScale.buf[i]);
			}

			// also copy the PxConvexMesh pointers (because the source collisionSet has them already at this point)
			physx::Array<PxConvexMesh*>& srcConvexMeshSet = collisionSet.mConvexMeshContainer[i];

			// make sure the destination list is present
			bool convexMeshSetResized = false;
			if (mConvexMeshContainer[i].size() < srcConvexMeshSet.size())
			{
				convexMeshSetResized = true;
				mConvexMeshContainer[i].resize(srcConvexMeshSet.size());
			}

			for (uint32_t j=0; j<srcConvexMeshSet.size(); j++)
			{
				// Only merge if we need PxConvexMesh pointers, otherwise just release these 
				// newly created PxConvexMesh after exiting this method
				if(mConvexMeshContainer[i][j] == NULL || convexMeshSetResized)
				{
					mConvexMeshContainer[i][j] = srcConvexMeshSet[j];
					// This prevents DestructibleAssetCollision::resize() from releasing the convex mesh
					srcConvexMeshSet[j] = NULL;
				}
			}
		}
	}
}

int32_t DestructibleAssetCollision::getScaleIndex(const PxVec3& scale, float tolerance) const
{
	for (int i = 0; i < mParams->scales.arraySizes[0]; ++i)
	{
		const PxVec3& error = scale - mParams->scales.buf[i];
		if (PxAbs(error.x) <= tolerance && PxAbs(error.y) <= tolerance && PxAbs(error.z) <= tolerance)
		{
			return i;
		}
	}

	return -1;
}

/** DestructibleAsset::ScatterMeshInstanceInfo **/

DestructibleAssetImpl::ScatterMeshInstanceInfo::~ScatterMeshInstanceInfo()
{
	if (m_actor != NULL)
	{
		m_actor->release();
		m_actor = NULL;
	}

	if (m_instanceBuffer != NULL)
	{
		UserRenderResourceManager* rrm = GetInternalApexSDK()->getUserRenderResourceManager();
		rrm->releaseInstanceBuffer(*m_instanceBuffer);
		m_instanceBuffer = NULL;
	}
}

}
} // end namespace nvidia


