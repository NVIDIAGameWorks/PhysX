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



#include "ApexRenderMeshActor.h"
#include "FrameworkPerfScope.h"
#include "PsAllocator.h"
#include "RenderDebugInterface.h"
#include "DebugRenderParams.h"

#include "RenderContext.h"
#include "RenderMeshActorDesc.h"
#include "UserRenderBoneBuffer.h"
#include "UserRenderBoneBufferDesc.h"
#include "UserRenderIndexBuffer.h"
#include "UserRenderIndexBufferDesc.h"
#include "UserRenderResource.h"
#include "UserRenderResourceDesc.h"
#include "UserRenderVertexBuffer.h"
#include "UserRenderer.h"

#include "PsIntrinsics.h"
#include "PxProfiler.h"

#define VERBOSE 0

namespace nvidia
{
namespace apex
{
// ApexRenderMeshActor methods

ApexRenderMeshActor::ApexRenderMeshActor(const RenderMeshActorDesc& desc, ApexRenderMeshAsset& asset, ResourceList& list) :
	mRenderMeshAsset(&asset),
	mIndexBufferHint(RenderBufferHint::STATIC),
	mMaxInstanceCount(0),
	mInstanceCount(0),
	mInstanceOffset(0),
	mInstanceBuffer(NULL),
	mRenderResource(NULL),
	mRenderWithoutSkinning(false),
	mForceBoneIndexChannel(false),
	mApiVisibilityChanged(false),
	mPartVisibilityChanged(false),
	mInstanceCountChanged(false),
	mKeepVisibleBonesPacked(false),
	mForceFallbackSkinning(false),
	mBonePosesDirty(false),
	mOneUserVertexBufferChanged(false),
	mBoneBufferInUse(false),
	mReleaseResourcesIfNothingToRender(true),
	mCreateRenderResourcesAfterInit(false),
	mBufferVisibility(false),
	mKeepPreviousFrameBoneBuffer(false),
	mPreviousFrameBoneBufferValid(false),
	mSkinningMode(RenderMeshActorSkinningMode::Default)
{
#if ENABLE_INSTANCED_MESH_CLEANUP_HACK
	mOrderedInstanceTemp     = 0;
	mOrderedInstanceTempSize = 0;
#endif
	list.add(*this);
	init(desc, (uint16_t) asset.getPartCount(), (uint16_t) asset.getBoneCount());
}

ApexRenderMeshActor::~ApexRenderMeshActor()
{
#if ENABLE_INSTANCED_MESH_CLEANUP_HACK
	if (mOrderedInstanceTemp)
	{
		PX_FREE(mOrderedInstanceTemp);
		mOrderedInstanceTemp = 0;
	}
#endif
}

void ApexRenderMeshActor::release()
{
	if (mInRelease)
	{
		return;
	}
	mInRelease = true;
	mRenderMeshAsset->releaseActor(*this);
}

void ApexRenderMeshActor::destroy()
{
	ApexActor::destroy();

	releaseRenderResources();

	// Release named resources
	ResourceProviderIntl* resourceProvider = GetInternalApexSDK()->getInternalResourceProvider();
	if (resourceProvider != NULL)
	{
		for (uint32_t i = 0; i < mSubmeshData.size(); i++)
		{
			if (mSubmeshData[i].materialID != mRenderMeshAsset->mMaterialIDs[i])
			{
				resourceProvider->releaseResource(mSubmeshData[i].materialID);
			}
			mSubmeshData[i].materialID = INVALID_RESOURCE_ID;
			mSubmeshData[i].material = NULL;
			mSubmeshData[i].isMaterialPointerValid = false;
		}
	}

	delete this;
}


void ApexRenderMeshActor::loadMaterial(SubmeshData& submeshData)
{
	ResourceProviderIntl* resourceProvider = GetInternalApexSDK()->getInternalResourceProvider();
	submeshData.material = resourceProvider->getResource(submeshData.materialID);
	submeshData.isMaterialPointerValid = true;

	if (submeshData.material != NULL)
	{
		UserRenderResourceManager* rrm = GetInternalApexSDK()->getUserRenderResourceManager();
		submeshData.maxBonesPerMaterial = rrm->getMaxBonesForMaterial(submeshData.material);

		if (submeshData.maxBonesPerMaterial == 0)
		{
			submeshData.maxBonesPerMaterial = mRenderMeshAsset->getBoneCount();
		}
	}
}


void ApexRenderMeshActor::init(const RenderMeshActorDesc& desc, uint16_t partCount, uint16_t boneCount)
{
	// TODO - LRR - This happened once, it shouldn't happen any more, let me know if it does
	//PX_ASSERT(boneCount != 0);
	//if (boneCount == 0)
	//	boneCount = partCount;

	mRenderWithoutSkinning = desc.renderWithoutSkinning;
	mForceBoneIndexChannel = desc.forceBoneIndexChannel;

	if (mRenderWithoutSkinning)
	{
		boneCount = 1;
	}

	const uint32_t oldBoneCount = mTransforms.size();
	mTransforms.resize(boneCount);
	for (uint32_t i = oldBoneCount; i < boneCount; ++i)
	{
		mTransforms[i] = PxMat44(PxIdentity);
	}

	mVisiblePartsForAPI.reserve(partCount);
	mVisiblePartsForAPI.lockCapacity(true);
	mVisiblePartsForRendering.reserve(partCount);
	mVisiblePartsForRendering.reset();	// size to 0, without reallocating

	mBufferVisibility = desc.bufferVisibility;

	mKeepPreviousFrameBoneBuffer = desc.keepPreviousFrameBoneBuffer;
	mPreviousFrameBoneBufferValid = false;

	mApiVisibilityChanged = true;
	mPartVisibilityChanged = !mBufferVisibility;

	for (uint32_t i = 0; i < partCount; ++i)
	{
		setVisibility(desc.visible, (uint16_t) i);
	}

	mIndexBufferHint  = desc.indexBufferHint;
	mMaxInstanceCount = desc.maxInstanceCount;
	mInstanceCount    = 0;
	mInstanceOffset   = 0;
	mInstanceBuffer   = NULL;

	if (desc.keepVisibleBonesPacked && !mRenderWithoutSkinning)
	{
		if (mRenderMeshAsset->getBoneCount() == mRenderMeshAsset->getPartCount())
		{
			mKeepVisibleBonesPacked = true;
		}
		else
		{
			APEX_INVALID_PARAMETER("RenderMeshActorDesc::keepVisibleBonesPacked is only allowed when the number of bones (%d) equals the number of parts (%d)\n",
			                       mRenderMeshAsset->getBoneCount(), mRenderMeshAsset->getPartCount());
		}
	}

	if (desc.forceFallbackSkinning)
	{
		if (mKeepVisibleBonesPacked)
		{
			APEX_INVALID_PARAMETER("RenderMeshActorDesc::forceFallbackSkinning can not be used when RenderMeshActorDesc::keepVisibleBonesPacked is also used!\n");
		}
		else
		{
			mForceFallbackSkinning = true;
		}
	}

	ResourceProviderIntl* resourceProvider = GetInternalApexSDK()->getInternalResourceProvider();
	ResID materialNS = GetInternalApexSDK()->getMaterialNameSpace();

	bool loadMaterialsOnActorCreation = !GetInternalApexSDK()->getRMALoadMaterialsLazily();

	if (mRenderWithoutSkinning)
	{
		// make sure that createRenderResources() is called in this special case!
		mCreateRenderResourcesAfterInit = true;
	}

	mSubmeshData.reserve(mRenderMeshAsset->getSubmeshCount());
	for (uint32_t submeshIndex = 0; submeshIndex < mRenderMeshAsset->getSubmeshCount(); submeshIndex++)
	{
		SubmeshData submeshData;

		// Resolve override material names using the NRP...
		if (submeshIndex < desc.overrideMaterialCount && resourceProvider != NULL)
		{
			submeshData.materialID = resourceProvider->createResource(materialNS, desc.overrideMaterials[submeshIndex]);;
		}
		else
		{
			submeshData.materialID = mRenderMeshAsset->mMaterialIDs[submeshIndex];
		}

		submeshData.maxBonesPerMaterial = 0;

		if (loadMaterialsOnActorCreation)
		{
			loadMaterial(submeshData);
		}

		mSubmeshData.pushBack(submeshData);
	}
}

bool ApexRenderMeshActor::setVisibility(bool visible, uint16_t partIndex)
{
	WRITE_ZONE();
	uint32_t oldRank = UINT32_MAX, newRank = UINT32_MAX;

	bool changed;

	if (visible)
	{
		changed = mVisiblePartsForAPI.useAndReturnRanks(partIndex, newRank, oldRank);
		if (changed)
		{
			mApiVisibilityChanged = true;
			if (!mBufferVisibility)
			{
				mPartVisibilityChanged = true;
			}
			if (mKeepVisibleBonesPacked && newRank != oldRank)
			{
				mTMSwapBuffer.pushBack(newRank << 16 | oldRank);
			}
		}
	}
	else
	{
		changed = mVisiblePartsForAPI.freeAndReturnRanks(partIndex, newRank, oldRank);
		if (changed)
		{
			mApiVisibilityChanged = true;
			if (!mBufferVisibility)
			{
				mPartVisibilityChanged = true;
			}
			if (mKeepVisibleBonesPacked && newRank != oldRank)
			{
				mTMSwapBuffer.pushBack(newRank << 16 | oldRank);
			}
		}
	}

	return changed;
}

bool ApexRenderMeshActor::getVisibilities(uint8_t* visibilityArray, uint32_t visibilityArraySize) const
{
	READ_ZONE();
	uint8_t changed = 0;
	const uint32_t numParts = PxMin(mRenderMeshAsset->getPartCount(), visibilityArraySize);
	for (uint32_t index = 0; index < numParts; ++index)
	{
		const uint8_t newVisibility = (uint8_t)isVisible((uint16_t) index);
		changed |= newVisibility ^(*visibilityArray);
		*visibilityArray++ = newVisibility;
	}
	return changed != 0;
}

void ApexRenderMeshActor::updateRenderResources(bool rewriteBuffers, void* userRenderData)
{
	updateRenderResources(!mRenderWithoutSkinning, rewriteBuffers, userRenderData);
}

void ApexRenderMeshActor::updateRenderResources(bool useBones, bool rewriteBuffers, void* userRenderData)
{
	URR_SCOPE;

#if VERBOSE > 1
	printf("updateRenderResources(useBones=%s, rewriteBuffers=%s, userRenderData=0x%p)\n", useBones ? "true" : "false", rewriteBuffers ? "true" : "false", userRenderData);
#endif

	UserRenderResourceManager* rrm = GetInternalApexSDK()->getUserRenderResourceManager();

	// fill out maxBonesPerMaterial (if it hasn't already been filled out). Also create fallback skinning if necessary.
	for (uint32_t submeshIndex = 0; submeshIndex < mSubmeshData.size(); submeshIndex++)
	{
		SubmeshData& submeshData = mSubmeshData[submeshIndex];

		if (submeshData.maxBonesPerMaterial == 0 && rrm != NULL)
		{
			if (!submeshData.isMaterialPointerValid)
			{
				// this should only be reached, when renderMeshActorLoadMaterialsLazily is true.
				// URR may not be called asynchronously in that case (for example in a render thread)
				ResourceProviderIntl* nrp = GetInternalApexSDK()->getInternalResourceProvider();
				submeshData.material = nrp->getResource(submeshData.materialID);
				submeshData.isMaterialPointerValid = true;
			}

			submeshData.maxBonesPerMaterial = rrm->getMaxBonesForMaterial(submeshData.material);

			if (submeshData.maxBonesPerMaterial == 0)
			{
				submeshData.maxBonesPerMaterial = mRenderMeshAsset->getBoneCount();
			}
		}

		bool needsFallbackSkinning = mForceFallbackSkinning || submeshData.maxBonesPerMaterial < mTransforms.size();
		if (needsFallbackSkinning && !mKeepVisibleBonesPacked && submeshData.fallbackSkinningMemory == NULL)
		{
			createFallbackSkinning(submeshIndex);
		}
	}

	PX_PROFILE_ZONE("ApexRenderMesh::updateRenderResources", GetInternalApexSDK()->getContextId());

	const bool invisible = visiblePartCount() == 0;
	const bool instanceless = mMaxInstanceCount > 0 && mInstanceCount == 0;
	if ((mReleaseResourcesIfNothingToRender && ((mPartVisibilityChanged && invisible) || (mInstanceCountChanged && instanceless))) || rewriteBuffers)
	{
		// First send out signals that the resource is no longer needed.
		for (uint32_t submeshIndex = 0; submeshIndex < mSubmeshData.size(); ++submeshIndex)
		{
			SubmeshData& submeshData = mSubmeshData[submeshIndex];
			for (uint32_t i = 0; i < submeshData.renderResources.size(); ++i)
			{
				UserRenderResource* renderResource = submeshData.renderResources[i].resource;
				if (renderResource != NULL)
				{
					if (renderResource->getBoneBuffer() != NULL)
					{
						renderResource->setBoneBufferRange(0, 0);
					}
					if (renderResource->getInstanceBuffer() != NULL)
					{
						renderResource->setInstanceBufferRange(0, 0);
					}
				}
			}
		}

		// Now release the resources
		releaseRenderResources();
		mPartVisibilityChanged = false;
		mBonePosesDirty = false;
		mInstanceCountChanged = false;

		// Rewrite buffers condition
		if (rewriteBuffers)
		{
			mCreateRenderResourcesAfterInit = true; // createRenderResources
			mPartVisibilityChanged = true; // writeBuffer for submesh data
		}

		return;
	}

	if (mCreateRenderResourcesAfterInit || mOneUserVertexBufferChanged || mPartVisibilityChanged || mBoneBufferInUse != useBones)
	{
		createRenderResources(useBones, userRenderData);
		mCreateRenderResourcesAfterInit = false;
	}
	if (mRenderResource)
	{
		mRenderResource->setInstanceBufferRange(mInstanceOffset, mInstanceCount);
	}

	PX_ASSERT(mSubmeshData.size() == mRenderMeshAsset->getSubmeshCount());

	for (uint32_t submeshIndex = 0; submeshIndex < mRenderMeshAsset->getSubmeshCount(); ++submeshIndex)
	{
		SubmeshData& submeshData = mSubmeshData[submeshIndex];

		if (submeshData.indexBuffer == NULL)
		{
			continue;
		}

		if (mPartVisibilityChanged || submeshData.staticColorReplacementDirty)
		{
			updatePartVisibility(submeshIndex, useBones, userRenderData);
			submeshData.staticColorReplacementDirty = false;
		}
		if (mBonePosesDirty)
		{
			if (submeshData.userDynamicVertexBuffer && !submeshData.userSpecifiedData)
			{
				updateFallbackSkinning(submeshIndex);
			}

			// Set up the previous bone buffer, if requested and available.  If we're packing bones, we need to do a remapping
			PX_ASSERT(!mPreviousFrameBoneBufferValid || mTransformsLastFrame.size() == mTransforms.size());
			const uint32_t tmBufferSize = mKeepVisibleBonesPacked ? getRenderVisiblePartCount() : mTransforms.size();
			if (mPreviousFrameBoneBufferValid && mTransformsLastFrame.size() == mTransforms.size() && mKeepVisibleBonesPacked)
			{
				mRemappedPreviousBoneTMs.resize(tmBufferSize);
				for (uint32_t tmNum = 0; tmNum < tmBufferSize; ++tmNum)
				{
					mRemappedPreviousBoneTMs[tmNum] = mTransformsLastFrame[mVisiblePartsForAPILastFrame.getRank(mVisiblePartsForAPI.usedIndices()[tmNum])];
				}
			}
			else
			{
				mRemappedPreviousBoneTMs.resize(0);
			}

			updateBonePoses(submeshIndex);

			// move this under the render lock because the fracture buffer processing accesses these arrays
			// this used to be at the end of dispatchRenderResources
			if (mKeepPreviousFrameBoneBuffer)
			{
				PX_ASSERT(mTransforms.size() != 0);
				mTransformsLastFrame = mTransforms;
				mVisiblePartsForAPILastFrame = mVisiblePartsForAPI;
				mPreviousFrameBoneBufferValid = true;
			}
		}

		if (submeshData.userDynamicVertexBuffer && (submeshData.fallbackSkinningDirty || submeshData.userVertexBufferAlwaysDirty))
		{
			writeUserBuffers(submeshIndex);
			submeshData.fallbackSkinningDirty = false;
		}

		if (mMaxInstanceCount)
		{
			updateInstances(submeshIndex);
		}

		if (!submeshData.isMaterialPointerValid)
		{
			// this should only be reached, when renderMeshActorLoadMaterialsLazily is true.
			// URR may not be called asynchronously in that case (for example in a render thread)
			ResourceProviderIntl* nrp = GetInternalApexSDK()->getInternalResourceProvider();
			submeshData.material = nrp->getResource(submeshData.materialID);
			submeshData.isMaterialPointerValid = true;
		}

		for (uint32_t i = 0; i < submeshData.renderResources.size(); ++i)
		{
			UserRenderResource* res = submeshData.renderResources[i].resource;
			if (res != NULL)
			{
				// LRR - poor workaround for http://nvbugs/534501, you'll crash here if you have more than 60 bones/material
				// and keepVisibleBonesPacked == false
				res->setMaterial(submeshData.material);
			}
		}
	}
	mBonePosesDirty = false;
	mPartVisibilityChanged = false;
}



void ApexRenderMeshActor::dispatchRenderResources(UserRenderer& renderer)
{
	dispatchRenderResources(renderer, PxMat44(PxIdentity));
}



void ApexRenderMeshActor::dispatchRenderResources(UserRenderer& renderer, const PxMat44& globalPose)
{
	PX_PROFILE_ZONE("ApexRenderMesh::dispatchRenderResources", GetInternalApexSDK()->getContextId());

	RenderContext context;

	// Assign the transform to the context when there is 1 part and no instancing

	// if there are no parts to render, return early
	// else if using instancing and there are not instances, return early
	// else if not using instancing and there is just 1 part (no bone buffer), save the transform to the context
	// else (using instancing and/or multiple parts), just assign identity to context
	if (mRenderMeshAsset->getPartCount() == 0 && !mRenderMeshAsset->getOpaqueMesh())
	{
		return;
	}
	else if (mInstanceCount == 0 && mMaxInstanceCount > 0)
	{
		return;
	}
	else if (mMaxInstanceCount == 0 && mTransforms.size() == 1)
	{
		context.local2world = globalPose * mTransforms[0]; // provide context for non-instanced ARMs with a single bone
		context.world2local = context.local2world.inverseRT();
	}
	else
	{
		context.local2world = globalPose;
		context.world2local = globalPose.inverseRT();
	}
	if (mRenderMeshAsset->getOpaqueMesh())
	{
		if (mRenderResource)
		{
			context.renderResource = mRenderResource;
			renderer.renderResource(context);
		}
	}
	else
	{
		for (uint32_t submeshIndex = 0; submeshIndex < mSubmeshData.size(); ++submeshIndex)
		{
			for (uint32_t i = 0; i < mSubmeshData[submeshIndex].renderResources.size(); ++i)
			{
				context.renderResource = mSubmeshData[submeshIndex].renderResources[i].resource;

				// no reason to render if we don't have any indices
				if ((mSubmeshData[submeshIndex].indexBuffer && (mSubmeshData[submeshIndex].visibleTriangleCount == 0)) || (mSubmeshData[submeshIndex].renderResources[i].vertexCount == 0))
				{
					continue;
				}

				if (context.renderResource)
				{
                    context.renderMeshName = mRenderMeshAsset->getName();
					renderer.renderResource(context);
				}
			}
		}
	}
}



void ApexRenderMeshActor::addVertexBuffer(uint32_t submeshIndex, bool alwaysDirty, PxVec3* position, PxVec3* normal, PxVec4* tangents)
{
#if VERBOSE
	GetInternalApexSDK()->getErrorCallback().reportError(PxErrorCode::eNO_ERROR, "addVertexBuffer\n", __FILE__, __LINE__);
	printf("addVertexBuffer(submeshIndex=%d)\n", submeshIndex);
#endif
	if (submeshIndex < mSubmeshData.size())
	{
		SubmeshData& submeshData = mSubmeshData[submeshIndex];

		if (submeshData.userSpecifiedData)
		{
			APEX_INVALID_PARAMETER("Cannot add user buffer to submesh %d, it's already assigned!", submeshIndex);
		}
		else
		{
			submeshData.userSpecifiedData = true;
			submeshData.userPositions = position;
			submeshData.userNormals = normal;
			submeshData.userTangents4 = tangents;

			submeshData.userVertexBufferAlwaysDirty = alwaysDirty;
			mOneUserVertexBufferChanged = true;
		}
	}
}



void ApexRenderMeshActor::removeVertexBuffer(uint32_t submeshIndex)
{
#if VERBOSE
	GetInternalApexSDK()->getErrorCallback().reportError(PxErrorCode::eNO_ERROR, "removeVertexBuffer\n", __FILE__, __LINE__);
	printf("removeVertexBuffer(submeshIndex=%d)\n", submeshIndex);
#endif
	if (submeshIndex < mSubmeshData.size())
	{
		SubmeshData& submeshData = mSubmeshData[submeshIndex];

		if (!submeshData.userSpecifiedData)
		{
			APEX_INVALID_PARAMETER("Cannot remove user buffer to submesh %d, it's not assigned!", submeshIndex);
		}
		else
		{
			submeshData.userSpecifiedData = false;
			submeshData.userPositions = NULL;
			submeshData.userNormals = NULL;
			submeshData.userTangents4 = NULL;

			submeshData.userVertexBufferAlwaysDirty = false;
			mOneUserVertexBufferChanged = true;

			if (submeshData.fallbackSkinningMemory != NULL)
			{
				distributeFallbackData(submeshIndex);
			}
		}
	}
}



void ApexRenderMeshActor::setStaticPositionReplacement(uint32_t submeshIndex, const PxVec3* staticPositions)
{
	PX_ASSERT(staticPositions != NULL);

	PX_ASSERT(submeshIndex < mSubmeshData.size());
	if (submeshIndex < mSubmeshData.size())
	{
		SubmeshData& submeshData = mSubmeshData[submeshIndex];

		PX_ASSERT(submeshData.staticPositionReplacement == NULL);
		submeshData.staticPositionReplacement = staticPositions;

		PX_ASSERT(submeshData.staticBufferReplacement == NULL);
		PX_ASSERT(submeshData.dynamicBufferReplacement == NULL);
	}
}

void ApexRenderMeshActor::setStaticColorReplacement(uint32_t submeshIndex, const ColorRGBA* staticColors)
{
	PX_ASSERT(staticColors != NULL);

	PX_ASSERT(submeshIndex < mSubmeshData.size());
	if (submeshIndex < mSubmeshData.size())
	{
		SubmeshData& submeshData = mSubmeshData[submeshIndex];

		submeshData.staticColorReplacement = staticColors;
		submeshData.staticColorReplacementDirty = true;
	}
}



void ApexRenderMeshActor::setInstanceBuffer(UserRenderInstanceBuffer* instBuf)
{
	WRITE_ZONE();
	mInstanceBuffer = instBuf;

	for (nvidia::Array<SubmeshData>::Iterator it = mSubmeshData.begin(), end = mSubmeshData.end(); it != end; ++it)
	{
		it->instanceBuffer = mInstanceBuffer;
		it->userIndexBufferChanged = true;
	}

	mOneUserVertexBufferChanged = true;
}

void ApexRenderMeshActor::setMaxInstanceCount(uint32_t count)
{
	WRITE_ZONE();
	mMaxInstanceCount = count;
}

void ApexRenderMeshActor::setInstanceBufferRange(uint32_t from, uint32_t count)
{
	WRITE_ZONE();
	mInstanceOffset = from;
	mInstanceCountChanged = count != mInstanceCount;
	mInstanceCount = count < mMaxInstanceCount ? count : mMaxInstanceCount;
}



void ApexRenderMeshActor::getLodRange(float& min, float& max, bool& intOnly) const
{
	READ_ZONE();
	PX_UNUSED(min);
	PX_UNUSED(max);
	PX_UNUSED(intOnly);
	APEX_INVALID_OPERATION("RenderMeshActor does not support this operation");
}



float ApexRenderMeshActor::getActiveLod() const
{
	READ_ZONE();
	APEX_INVALID_OPERATION("RenderMeshActor does not support this operation");
	return -1.0f;
}

void ApexRenderMeshActor::forceLod(float lod)
{
	WRITE_ZONE();
	PX_UNUSED(lod);
	APEX_INVALID_OPERATION("RenderMeshActor does not support this operation");
}



void ApexRenderMeshActor::createRenderResources(bool useBones, void* userRenderData)
{
#if VERBOSE
	printf("createRenderResources(useBones=%s, userRenderData=0x%p)\n", useBones ? "true" : "false", userRenderData);
#endif

	PX_PROFILE_ZONE("ApexRenderMesh::createRenderResources", GetInternalApexSDK()->getContextId());

	if (mRenderMeshAsset->getOpaqueMesh())
	{
		if (mRenderResource == NULL || mRenderResource->getInstanceBuffer() != mInstanceBuffer)
		{
			UserRenderResourceManager* rrm = GetInternalApexSDK()->getUserRenderResourceManager();
			if (mRenderResource != NULL)
			{
				rrm->releaseResource(*mRenderResource);
				mRenderResource = NULL;
			}

			UserRenderResourceDesc desc;
			desc.instanceBuffer = mInstanceBuffer;
			desc.opaqueMesh = mRenderMeshAsset->getOpaqueMesh();
			desc.userRenderData = userRenderData;
			mRenderResource = rrm->createResource(desc);
		}
	}

	PX_ASSERT(mSubmeshData.size() == mRenderMeshAsset->getSubmeshCount());

	UserRenderResourceManager* rrm = GetInternalApexSDK()->getUserRenderResourceManager();

	bool createAndFillSharedVertexBuffersAll = mOneUserVertexBufferChanged;
	if (mRenderMeshAsset->mRuntimeSubmeshData.empty())
	{
		mRenderMeshAsset->mRuntimeSubmeshData.resize(mRenderMeshAsset->getSubmeshCount());
		memset(mRenderMeshAsset->mRuntimeSubmeshData.begin(), 0, sizeof(ApexRenderMeshAsset::SubmeshData) * mRenderMeshAsset->mRuntimeSubmeshData.size());
		createAndFillSharedVertexBuffersAll = true;
	}

	bool fill2ndVertexBuffersAll = false;
	if (!mPerActorVertexBuffers.size())
	{
		// Create a separate (instanced) buffer for bone indices and/or colors
		fill2ndVertexBuffersAll = mKeepVisibleBonesPacked;
		for (uint32_t submeshIndex = 0; !fill2ndVertexBuffersAll && submeshIndex < mRenderMeshAsset->getSubmeshCount(); ++submeshIndex)
		{
			fill2ndVertexBuffersAll = (mSubmeshData[submeshIndex].staticColorReplacement != NULL);
		}
		if (fill2ndVertexBuffersAll)
		{
			mPerActorVertexBuffers.resize(mRenderMeshAsset->getSubmeshCount());
		}
	}

	PX_ASSERT(mRenderMeshAsset->mRuntimeSubmeshData.size() == mSubmeshData.size());
	PX_ASSERT(mRenderMeshAsset->getSubmeshCount() == mSubmeshData.size());
	for (uint32_t submeshIndex = 0; submeshIndex < mRenderMeshAsset->getSubmeshCount(); ++submeshIndex)
	{
		ApexRenderSubmesh& submesh = *mRenderMeshAsset->mSubmeshes[submeshIndex];
		SubmeshData& submeshData = mSubmeshData[submeshIndex];

		if (submesh.getVertexBuffer().getVertexCount() == 0 || !submeshHasVisibleTriangles(submeshIndex))
		{
			for (uint32_t i = 0; i < submeshData.renderResources.size(); ++i)
			{
				if (submeshData.renderResources[i].resource != NULL)
				{
					UserRenderBoneBuffer* boneBuffer = submeshData.renderResources[i].resource->getBoneBuffer();
					rrm->releaseResource(*submeshData.renderResources[i].resource);
					if (boneBuffer)
					{
						rrm->releaseBoneBuffer(*boneBuffer);
					}
					submeshData.renderResources[i].resource = NULL;
				}
			}
			continue;
		}

		bool fill2ndVertexBuffers = fill2ndVertexBuffersAll;
		// Handling color replacement through "2nd vertex buffer"
		if ((mKeepVisibleBonesPacked || submeshData.staticColorReplacement != NULL) && mPerActorVertexBuffers[submeshIndex] == NULL)
		{
			fill2ndVertexBuffers = true;
		}

		bool createAndFillSharedVertexBuffers = createAndFillSharedVertexBuffersAll;

		// create vertex buffers if some buffer replacements are present in the actor
		if (submeshData.staticPositionReplacement != NULL && submeshData.staticBufferReplacement == NULL && submeshData.dynamicBufferReplacement == NULL)
		{
			createAndFillSharedVertexBuffers = true;
		}

		{
			ApexRenderMeshAsset::SubmeshData& runtimeSubmeshData = mRenderMeshAsset->mRuntimeSubmeshData[submeshIndex];
			if (runtimeSubmeshData.staticVertexBuffer == NULL && runtimeSubmeshData.dynamicVertexBuffer == NULL && runtimeSubmeshData.skinningVertexBuffer == NULL)
			{
				createAndFillSharedVertexBuffers = true;
			}

			// create vertex buffers if not all static vertex buffers have been created by the previous actors that were doing this
			if (runtimeSubmeshData.needsStaticData && runtimeSubmeshData.staticVertexBuffer == NULL)
			{
				createAndFillSharedVertexBuffers = true;
			}

			if (runtimeSubmeshData.needsDynamicData && runtimeSubmeshData.dynamicVertexBuffer == NULL)
			{
				createAndFillSharedVertexBuffers = true;
			}
		}


		VertexBufferIntl& srcVB = submesh.getVertexBufferWritable();
		const VertexFormat& vf = srcVB.getFormat();

		bool fillStaticSharedVertexBuffer = false;
		bool fillDynamicSharedVertexBuffer = false;
		bool fillSkinningSharedVertexBuffer = false;

		if (createAndFillSharedVertexBuffers)
		{
			UserRenderVertexBufferDesc staticBufDesc, dynamicBufDesc, boneBufDesc;
			staticBufDesc.moduleIdentifier = mRenderMeshAsset->mOwnerModuleID;
			staticBufDesc.maxVerts = srcVB.getVertexCount();
			staticBufDesc.hint = RenderBufferHint::STATIC;
			staticBufDesc.uvOrigin = mRenderMeshAsset->getTextureUVOrigin();
			staticBufDesc.numCustomBuffers = 0;
			staticBufDesc.canBeShared = true;

			dynamicBufDesc = staticBufDesc;
			boneBufDesc = dynamicBufDesc;
			bool useDynamicBuffer = false;
			bool replaceStaticBuffer = false;
			bool replaceDynamicBuffer = false;

			// extract all the buffers into one of the three descs
			for (uint32_t i = 0; i < vf.getBufferCount(); ++i)
			{
				RenderVertexSemantic::Enum semantic = vf.getBufferSemantic(i);
				if (semantic >= RenderVertexSemantic::POSITION && semantic <= RenderVertexSemantic::COLOR)
				{
					if (vf.getBufferAccess(i) == RenderDataAccess::STATIC)
					{
						staticBufDesc.buffersRequest[semantic]	= vf.getBufferFormat(i);

						if (semantic == RenderVertexSemantic::POSITION && submeshData.staticPositionReplacement != NULL)
						{
							replaceStaticBuffer = true;
						}
					}
					else
					{
						dynamicBufDesc.buffersRequest[semantic]	= vf.getBufferFormat(i);
						useDynamicBuffer = true;

						if (semantic == RenderVertexSemantic::POSITION && submeshData.staticPositionReplacement != NULL)
						{
							replaceDynamicBuffer = true;
						}
					}
				}
				else if (semantic == RenderVertexSemantic::CUSTOM)
				{
					++staticBufDesc.numCustomBuffers;
				}
			}

			if (staticBufDesc.numCustomBuffers)
			{
				staticBufDesc.customBuffersIdents = &mRenderMeshAsset->mRuntimeCustomSubmeshData[submeshIndex].customBufferVoidPtrs[0];
				staticBufDesc.customBuffersRequest = &mRenderMeshAsset->mRuntimeCustomSubmeshData[submeshIndex].customBufferFormats[0];
			}

			// PH: only create bone indices/weights if more than one bone is present. one bone just needs local2world
			if (mTransforms.size() > 1)
			{
				UserRenderVertexBufferDesc* boneDesc = vf.hasSeparateBoneBuffer() ? &boneBufDesc : &staticBufDesc;

				if (!fill2ndVertexBuffers)
				{
					uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::BONE_INDEX));
					boneDesc->buffersRequest[RenderVertexSemantic::BONE_INDEX] = vf.getBufferFormat(bufferIndex);
					bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::BONE_WEIGHT));
					boneDesc->buffersRequest[RenderVertexSemantic::BONE_WEIGHT] = vf.getBufferFormat(bufferIndex);
				}
			}
			else
			if (mForceBoneIndexChannel)
			{
				// Note, it is assumed here that this means there's an actor which will handle dynamic parts, and will require a shared bone index buffer
				UserRenderVertexBufferDesc* boneDesc = vf.hasSeparateBoneBuffer() ? &boneBufDesc : &staticBufDesc;

				if (!fill2ndVertexBuffers)
				{
					uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::BONE_INDEX));
					boneDesc->buffersRequest[RenderVertexSemantic::BONE_INDEX] = vf.getBufferFormat(bufferIndex);
				}
			}

			for (uint32_t semantic = RenderVertexSemantic::TEXCOORD0; semantic <= RenderVertexSemantic::TEXCOORD3; ++semantic)
			{
				uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID((RenderVertexSemantic::Enum)semantic));
				staticBufDesc.buffersRequest[ semantic ]	= vf.getBufferFormat(bufferIndex);
			}

			{
				uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::DISPLACEMENT_TEXCOORD));
				staticBufDesc.buffersRequest[ RenderVertexSemantic::DISPLACEMENT_TEXCOORD ] = vf.getBufferFormat(bufferIndex);
				bufferIndex       = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::DISPLACEMENT_FLAGS));
				staticBufDesc.buffersRequest[ RenderVertexSemantic::DISPLACEMENT_FLAGS ] = vf.getBufferFormat(bufferIndex);
			}

			// empty static buffer?
			uint32_t numEntries = staticBufDesc.numCustomBuffers;
			for (uint32_t i = 0; i < RenderVertexSemantic::NUM_SEMANTICS; i++)
			{
				numEntries += (staticBufDesc.buffersRequest[i] == RenderDataFormat::UNSPECIFIED) ? 0 : 1;

				PX_ASSERT(staticBufDesc.buffersRequest[i] == RenderDataFormat::UNSPECIFIED || vertexSemanticFormatValid((RenderVertexSemantic::Enum)i, staticBufDesc.buffersRequest[i]));
				PX_ASSERT(dynamicBufDesc.buffersRequest[i] == RenderDataFormat::UNSPECIFIED || vertexSemanticFormatValid((RenderVertexSemantic::Enum)i, dynamicBufDesc.buffersRequest[i]));
				PX_ASSERT(boneBufDesc.buffersRequest[i] == RenderDataFormat::UNSPECIFIED || vertexSemanticFormatValid((RenderVertexSemantic::Enum)i, boneBufDesc.buffersRequest[i]));
			}

			if (numEntries > 0)
			{
				if (replaceStaticBuffer)
				{
					submeshData.staticBufferReplacement = rrm->createVertexBuffer(staticBufDesc);
					fillStaticSharedVertexBuffer = true;
				}
				else if (mRenderMeshAsset->mRuntimeSubmeshData[submeshIndex].staticVertexBuffer == NULL)
				{
					mRenderMeshAsset->mRuntimeSubmeshData[submeshIndex].staticVertexBuffer = rrm->createVertexBuffer(staticBufDesc);
					fillStaticSharedVertexBuffer = true;
				}
				mRenderMeshAsset->mRuntimeSubmeshData[submeshIndex].needsStaticData = true;
			}

			if (useDynamicBuffer)
			{
				// only create this if we don't create a per-actor dynamic buffer
				if (submeshData.fallbackSkinningMemory == NULL && !submeshData.userSpecifiedData)
				{
					if (replaceDynamicBuffer)
					{
						submeshData.dynamicBufferReplacement = rrm->createVertexBuffer(dynamicBufDesc);
						fillDynamicSharedVertexBuffer = true;
					}
					else if (mRenderMeshAsset->mRuntimeSubmeshData[submeshIndex].dynamicVertexBuffer == NULL)
					{
						mRenderMeshAsset->mRuntimeSubmeshData[submeshIndex].dynamicVertexBuffer = rrm->createVertexBuffer(dynamicBufDesc);
						fillDynamicSharedVertexBuffer = true;
					}
				}
				mRenderMeshAsset->mRuntimeSubmeshData[submeshIndex].needsDynamicData = true;
			}

			uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::BONE_INDEX));
			const uint32_t bonesPerVertex = vertexSemanticFormatElementCount(RenderVertexSemantic::BONE_INDEX, vf.getBufferFormat(bufferIndex));
			if (vf.hasSeparateBoneBuffer() && bonesPerVertex > 0 && mTransforms.size() > 1 && useBones && mRenderMeshAsset->mRuntimeSubmeshData[submeshIndex].skinningVertexBuffer == NULL)
			{
				mRenderMeshAsset->mRuntimeSubmeshData[submeshIndex].skinningVertexBuffer = rrm->createVertexBuffer(boneBufDesc);
				fillSkinningSharedVertexBuffer = true;
			}
		}

		if ((submeshData.fallbackSkinningMemory != NULL || submeshData.userSpecifiedData) && submeshData.userDynamicVertexBuffer == NULL)
		{
			UserRenderVertexBufferDesc perActorDynamicBufDesc;
			perActorDynamicBufDesc.moduleIdentifier = mRenderMeshAsset->mOwnerModuleID;
			perActorDynamicBufDesc.maxVerts = srcVB.getVertexCount();
			perActorDynamicBufDesc.uvOrigin = mRenderMeshAsset->getTextureUVOrigin();
			perActorDynamicBufDesc.hint = RenderBufferHint::DYNAMIC;
			perActorDynamicBufDesc.canBeShared = false;

			if (submeshData.userPositions != NULL)
			{
				perActorDynamicBufDesc.buffersRequest[RenderVertexSemantic::POSITION] = RenderDataFormat::FLOAT3;
			}

			if (submeshData.userNormals != NULL)
			{
				perActorDynamicBufDesc.buffersRequest[RenderVertexSemantic::NORMAL] = RenderDataFormat::FLOAT3;
			}

			if (submeshData.userTangents4 != NULL)
			{
				perActorDynamicBufDesc.buffersRequest[RenderVertexSemantic::TANGENT] = RenderDataFormat::FLOAT4;
			}

			submeshData.userDynamicVertexBuffer = rrm->createVertexBuffer(perActorDynamicBufDesc);
		}

		if (fill2ndVertexBuffers)
		{
			UserRenderVertexBufferDesc bufDesc;
			bufDesc.moduleIdentifier = mRenderMeshAsset->mOwnerModuleID;
			bufDesc.maxVerts = srcVB.getVertexCount();
			bufDesc.hint = RenderBufferHint::DYNAMIC;
			if (mKeepVisibleBonesPacked)
			{
				bufDesc.buffersRequest[ RenderVertexSemantic::BONE_INDEX ] = RenderDataFormat::USHORT1;
			}
			if (submeshData.staticColorReplacement)
			{
				bufDesc.buffersRequest[ RenderVertexSemantic::COLOR ] = RenderDataFormat::R8G8B8A8;
			}
			bufDesc.uvOrigin = mRenderMeshAsset->getTextureUVOrigin();
			bufDesc.canBeShared = false;
			for (uint32_t i = 0; i < RenderVertexSemantic::NUM_SEMANTICS; i++)
			{
				PX_ASSERT(bufDesc.buffersRequest[i] == RenderDataFormat::UNSPECIFIED || vertexSemanticFormatValid((RenderVertexSemantic::Enum)i, bufDesc.buffersRequest[i]));
			}
			mPerActorVertexBuffers[submeshIndex] = rrm->createVertexBuffer(bufDesc);
		}

		// creates and/or fills index buffers
		updatePartVisibility(submeshIndex, useBones, userRenderData);

		if (fillStaticSharedVertexBuffer || fillDynamicSharedVertexBuffer || fillSkinningSharedVertexBuffer)
		{
			const VertexFormat& vf = srcVB.getFormat();

			RenderVertexBufferData    dynamicWriteData;
			RenderVertexBufferData    staticWriteData;
			RenderVertexBufferData    skinningWriteData;
			RenderVertexBufferData&	skinningWriteDataRef = vf.hasSeparateBoneBuffer() ? skinningWriteData : staticWriteData;

			UserRenderVertexBuffer* staticVb = mRenderMeshAsset->mRuntimeSubmeshData[submeshIndex].staticVertexBuffer;
			UserRenderVertexBuffer* dynamicVb = mRenderMeshAsset->mRuntimeSubmeshData[submeshIndex].dynamicVertexBuffer;
			UserRenderVertexBuffer* skinningVb = mRenderMeshAsset->mRuntimeSubmeshData[submeshIndex].skinningVertexBuffer;

			if (submeshData.staticBufferReplacement != NULL)
			{
				staticVb = submeshData.staticBufferReplacement;
			}

			if (submeshData.dynamicBufferReplacement != NULL)
			{
				dynamicVb = submeshData.dynamicBufferReplacement;
			}

			for (uint32_t semantic = RenderVertexSemantic::POSITION; semantic <= RenderVertexSemantic::COLOR; ++semantic)
			{
				if (semantic == RenderVertexSemantic::COLOR && submeshData.staticColorReplacement != NULL)
				{
					// Gets done in updatePartVisibility if submeshData.staticColorReplacement is used
					continue;
				}

				RenderDataFormat::Enum format;
				int32_t bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID((RenderVertexSemantic::Enum)semantic));
				if (bufferIndex < 0)
				{
					continue;
				}
				const void* src = srcVB.getBufferAndFormat(format, (uint32_t)bufferIndex);

				if (semantic == RenderVertexSemantic::POSITION && submeshData.staticPositionReplacement != NULL)
				{
					src = submeshData.staticPositionReplacement;
				}

				if (format != RenderDataFormat::UNSPECIFIED)
				{
					if (srcVB.getFormat().getBufferAccess((uint32_t)bufferIndex) == RenderDataAccess::STATIC)
					{
						staticWriteData.setSemanticData((RenderVertexSemantic::Enum)semantic, src, RenderDataFormat::getFormatDataSize(format), format);
					}
					else
					{
						dynamicWriteData.setSemanticData((RenderVertexSemantic::Enum)semantic, src, RenderDataFormat::getFormatDataSize(format), format);
					}
				}
			}

			for (uint32_t semantic = RenderVertexSemantic::TEXCOORD0; semantic <= RenderVertexSemantic::TEXCOORD3; ++semantic)
			{
				RenderDataFormat::Enum format;
				int32_t bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID((RenderVertexSemantic::Enum)semantic));
				if (bufferIndex < 0)
				{
					continue;
				}
				const void* src = srcVB.getBufferAndFormat(format, (uint32_t)bufferIndex);
				if (format != RenderDataFormat::UNSPECIFIED)
				{
					staticWriteData.setSemanticData((RenderVertexSemantic::Enum)semantic, src, RenderDataFormat::getFormatDataSize(format), format);
				}
			}

			for (uint32_t semantic = RenderVertexSemantic::DISPLACEMENT_TEXCOORD; semantic <= RenderVertexSemantic::DISPLACEMENT_FLAGS; ++semantic)
			{
				int32_t bufferIndex = vf.getBufferIndexFromID(vf.getSemanticID((RenderVertexSemantic::Enum)semantic));
				if (bufferIndex >= 0)
				{
					RenderDataFormat::Enum format;
					const void* src = srcVB.getBufferAndFormat(format, (uint32_t)bufferIndex);
					if (format != RenderDataFormat::UNSPECIFIED)
					{
						staticWriteData.setSemanticData((RenderVertexSemantic::Enum)semantic, src, RenderDataFormat::getFormatDataSize(format), format);
					}
				}
			}

			nvidia::Array<RenderSemanticData> semanticData;

			const uint32_t numCustom = vf.getCustomBufferCount();
			if (numCustom)
			{
				// NvParameterized::Handle custom vertex buffer semantics
				semanticData.resize(numCustom);

				uint32_t writeIndex = 0;
				for (uint32_t i = 0; i < vf.getBufferCount(); i++)
				{
					// Fill in a RenderSemanticData for each custom semantic
					if (vf.getBufferSemantic(i) != RenderVertexSemantic::CUSTOM)
					{
						continue;
					}
					semanticData[writeIndex].data = srcVB.getBuffer(i);
					RenderDataFormat::Enum fmt = mRenderMeshAsset->mRuntimeCustomSubmeshData[submeshIndex].customBufferFormats[writeIndex];
					semanticData[writeIndex].stride = RenderDataFormat::getFormatDataSize(fmt);
					semanticData[writeIndex].format = fmt;
					semanticData[writeIndex].ident = mRenderMeshAsset->mRuntimeCustomSubmeshData[submeshIndex].customBufferVoidPtrs[writeIndex];

					writeIndex++;
				}
				PX_ASSERT(writeIndex == numCustom);
				staticWriteData.setCustomSemanticData(&semanticData[0], numCustom);
			}

			physx::Array<uint16_t> boneIndicesModuloMaxBoneCount;

			if (mTransforms.size() > 1 || mForceBoneIndexChannel)
			{
				uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::BONE_INDEX));
				const uint32_t numBonesPerVertex = vertexSemanticFormatElementCount(RenderVertexSemantic::BONE_INDEX, vf.getBufferFormat(bufferIndex));
				if (numBonesPerVertex == 1)
				{
					// Gets done in updatePartVisibility if keepVisibleBonesPacked is true
					if (!mKeepVisibleBonesPacked)
					{
						RenderDataFormat::Enum format;
						const VertexFormat& vf = srcVB.getFormat();
						uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::BONE_INDEX));
						const void* src = srcVB.getBufferAndFormat(format, bufferIndex);
						if (format != RenderDataFormat::UNSPECIFIED)
						{
							if (mForceBoneIndexChannel && format == RenderDataFormat::USHORT1 && submeshData.maxBonesPerMaterial > 0 && submeshData.maxBonesPerMaterial < mRenderMeshAsset->getBoneCount())
							{
								boneIndicesModuloMaxBoneCount.resize(srcVB.getVertexCount());
								uint16_t* srcBuf = (uint16_t*)src;
								for (uint32_t vertexNum = 0; vertexNum < srcVB.getVertexCount(); ++vertexNum)
								{
									boneIndicesModuloMaxBoneCount[vertexNum] = *(srcBuf++)%submeshData.maxBonesPerMaterial;
								}
								src = &boneIndicesModuloMaxBoneCount[0];
							}
							skinningWriteDataRef.setSemanticData(RenderVertexSemantic::BONE_INDEX, src, RenderDataFormat::getFormatDataSize(format), format);
						}
					}
				}
				else
				{
					RenderDataFormat::Enum format;
					const void* src;
					const VertexFormat& vf = srcVB.getFormat();
					uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(nvidia::RenderVertexSemantic::BONE_INDEX));
					src = srcVB.getBufferAndFormat(format, bufferIndex);
					if (format != RenderDataFormat::UNSPECIFIED)
					{
						skinningWriteDataRef.setSemanticData(RenderVertexSemantic::BONE_INDEX, src, RenderDataFormat::getFormatDataSize(format), format);
					}
					bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(nvidia::RenderVertexSemantic::BONE_WEIGHT));
					src = srcVB.getBufferAndFormat(format, bufferIndex);
					if (format != RenderDataFormat::UNSPECIFIED)
					{
						skinningWriteDataRef.setSemanticData(RenderVertexSemantic::BONE_WEIGHT, src, RenderDataFormat::getFormatDataSize(format), format);
					}
				}
			}

			if (staticVb != NULL && fillStaticSharedVertexBuffer)
			{
				staticVb->writeBuffer(staticWriteData,   0, srcVB.getVertexCount());
			}
			if (dynamicVb != NULL && fillDynamicSharedVertexBuffer)
			{
				dynamicVb->writeBuffer(dynamicWriteData,  0, srcVB.getVertexCount());
			}
			if (skinningVb != NULL && fillSkinningSharedVertexBuffer)
			{
				skinningVb->writeBuffer(skinningWriteData, 0, srcVB.getVertexCount());
			}

			// TODO - SJB - Beta2 - release submesh after updateRenderResources() returns. It requires acquiring the actor lock as game engine could delay these
			// writes so long as it holds the render lock.  Could be done in updateBounds(), which implictly has the lock.  Perhaps we need to catch lock release
			// so it happens immediately.
		}

		// Delete static vertex buffers after writing them
		if (mRenderMeshAsset->mParams->deleteStaticBuffersAfterUse)
		{
			ApexVertexFormat dynamicFormats;
			dynamicFormats.copy((const ApexVertexFormat&)vf);
			for (uint32_t semantic = RenderVertexSemantic::POSITION; semantic < RenderVertexSemantic::NUM_SEMANTICS; ++semantic)
			{
				uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID((RenderVertexSemantic::Enum)semantic));
				if (dynamicFormats.getBufferAccess(bufferIndex) == RenderDataAccess::STATIC)
				{
					dynamicFormats.setBufferFormat(bufferIndex, RenderDataFormat::UNSPECIFIED);
				}
			}
			srcVB.build(dynamicFormats, srcVB.getVertexCount());
		}
	}
	mOneUserVertexBufferChanged = false;
	mPartVisibilityChanged = false;
	mBoneBufferInUse = useBones;
}

void ApexRenderMeshActor::updatePartVisibility(uint32_t submeshIndex, bool useBones, void* userRenderData)
{
#if VERBOSE
	printf("updatePartVisibility(submeshIndex=%d, useBones=%s, userRenderData=0x%p)\n", submeshIndex, useBones ? "true" : "false", userRenderData);
	printf("  mPartVisibilityChanged=%s\n", mPartVisibilityChanged ? "true" : "false");
#endif

	const ApexRenderSubmesh& submesh = *mRenderMeshAsset->mSubmeshes[submeshIndex];
	SubmeshData& submeshData = mSubmeshData[ submeshIndex ];

	UserRenderResourceManager* rrm = GetInternalApexSDK()->getUserRenderResourceManager();
	PX_ASSERT(rrm != NULL);

	// we end up with a division by 0 otherwise :(
	PX_ASSERT(submeshData.maxBonesPerMaterial > 0);

	const uint32_t partCount = mKeepVisibleBonesPacked ? getRenderVisiblePartCount() : mRenderMeshAsset->getPartCount();

	// only use bones if there is no fallback skinning
	useBones &= submeshData.fallbackSkinningMemory == NULL;

	uint32_t resourceCount;
	if (!useBones)
	{
		// If we're not skinning, we only need one resource
		resourceCount = 1;
	}
	else
	{
		// LRR - poor workaround for http://nvbugs/534501
		if (mKeepVisibleBonesPacked)
		{
			resourceCount = partCount == 0 ? 0 : (partCount + submeshData.maxBonesPerMaterial - 1) / submeshData.maxBonesPerMaterial;
		}
		else
		{
			resourceCount = partCount == 0 ? 0 : (mRenderMeshAsset->getBoneCount() + submeshData.maxBonesPerMaterial - 1) / submeshData.maxBonesPerMaterial;
		}
	}

	// Eliminate unneeded resources:
	const uint32_t start = (submeshData.userIndexBufferChanged || useBones != mBoneBufferInUse) ? 0 : resourceCount;
	for (uint32_t i = start; i < submeshData.renderResources.size(); ++i)
	{
		if (submeshData.renderResources[i].resource != NULL)
		{
			UserRenderBoneBuffer* boneBuffer = submeshData.renderResources[i].resource->getBoneBuffer();
			rrm->releaseResource(*submeshData.renderResources[i].resource);

			if (boneBuffer)
			{
				rrm->releaseBoneBuffer(*boneBuffer);
			}

			submeshData.renderResources[i].resource = NULL;
		}
	}
	submeshData.userIndexBufferChanged = false;

	uint16_t resourceBoneCount = 0;
	uint32_t resourceNum = 0;
	uint32_t startIndex = 0;

	if (mKeepVisibleBonesPacked)
	{
		if (mBoneIndexTempBuffer.size() < submesh.getVertexBuffer().getVertexCount())
		{
			mBoneIndexTempBuffer.resize(submesh.getVertexBuffer().getVertexCount());	// A new index buffer to remap the bone indices to the smaller buffer
		}
	}

	uint32_t boneIndexStart = uint32_t(-1);
	uint32_t boneIndexEnd = 0;

	// Figure out how many indices we'll need
	uint32_t totalIndexCount = submesh.getTotalIndexCount();	// Worst case

	const uint32_t* visiblePartIndexPtr = getRenderVisibleParts();

	if (mKeepVisibleBonesPacked)
	{
		// We can do better
		totalIndexCount = 0;
		for (uint32_t partNum = 0; partNum < partCount; ++partNum)
		{
			totalIndexCount += submesh.getIndexCount(visiblePartIndexPtr[partNum]);
		}
	}

	uint32_t newIndexBufferRequestSize = submeshData.indexBufferRequestedSize;

	// If there has not already been an index buffer request, set to exact size
	if (newIndexBufferRequestSize == 0 || totalIndexCount >= 0x80000000)	// special handling of potential overflow
	{
		newIndexBufferRequestSize = totalIndexCount;
	}
	else
	{
		// If the buffer has already been requested, see if we need to grow or shrink it
		while (totalIndexCount > newIndexBufferRequestSize)
		{
			newIndexBufferRequestSize *= 2;
		}
		while (2*totalIndexCount < newIndexBufferRequestSize)
		{
			newIndexBufferRequestSize /= 2;
		}
	}

	// In case our doubling schedule gave it a larger size than we'll ever need
	if (newIndexBufferRequestSize > submesh.getTotalIndexCount())
	{
		newIndexBufferRequestSize = submesh.getTotalIndexCount();
	}

	if (submeshData.indexBuffer != NULL && newIndexBufferRequestSize != submeshData.indexBufferRequestedSize)
	{
		// Release the old buffer
		rrm->releaseIndexBuffer(*submeshData.indexBuffer);
		submeshData.indexBuffer = NULL;
		releaseSubmeshRenderResources(submeshIndex);
	}

	// Create the index buffer now if needed
	if (submeshData.indexBuffer == NULL && newIndexBufferRequestSize > 0)
	{
		UserRenderIndexBufferDesc indexDesc;
		indexDesc.maxIndices = newIndexBufferRequestSize;
		indexDesc.hint       = mIndexBufferHint;
		indexDesc.format     = RenderDataFormat::UINT1;
		submeshData.indexBuffer = rrm->createIndexBuffer(indexDesc);
		submeshData.indexBufferRequestedSize = newIndexBufferRequestSize;
	}

	submeshData.renderResources.resize(resourceCount, ResourceData());

	submeshData.visibleTriangleCount = 0;
	// KHA - batch writes to temporary buffer so that index buffer is only locked once per frame
	if(mPartIndexTempBuffer.size() < totalIndexCount)
	{
		mPartIndexTempBuffer.resize(totalIndexCount);
	}
	for (uint32_t partNum = 0; partNum < partCount;)
	{
		uint32_t partIndex;
		bool partIsVisible;
		if (mKeepVisibleBonesPacked)
		{
			partIndex = visiblePartIndexPtr[partNum++];
			partIsVisible = true;

			const uint32_t indexStart = submesh.getFirstVertexIndex(partIndex);
			const uint32_t vertexCount = submesh.getVertexCount(partIndex);

			uint16_t* boneIndex = mBoneIndexTempBuffer.begin() + indexStart;
			const uint16_t* boneIndexStop = boneIndex + vertexCount;

			boneIndexStart = PxMin(boneIndexStart, indexStart);
			boneIndexEnd = PxMax(boneIndexEnd, indexStart + vertexCount);

			while (boneIndex < boneIndexStop)
			{
				*boneIndex++ = resourceBoneCount;
			}
		}
		else
		{
			partIndex = partNum++;
			partIsVisible = isVisible((uint16_t)partIndex);
		}

		if (partIsVisible)
		{
			const uint32_t indexCount = submesh.getIndexCount(partIndex);
			const uint32_t* indices = submesh.getIndexBuffer(partIndex);
			const uint32_t currentIndexNum = submeshData.visibleTriangleCount * 3;
			if (indexCount > 0 && mPartVisibilityChanged)
			{
				memcpy(mPartIndexTempBuffer.begin() + currentIndexNum, indices, indexCount * sizeof(uint32_t));
			}
			submeshData.visibleTriangleCount += indexCount / 3;
		}

		// LRR - poor workaround for http://nvbugs/534501
		bool generateNewRenderResource = false;

		const bool oneBonePerPart = mSkinningMode != RenderMeshActorSkinningMode::AllBonesPerPart;

		const uint16_t bonesToAdd = oneBonePerPart ? 1u : (uint16_t)mRenderMeshAsset->getBoneCount();
		resourceBoneCount = PxMin<uint16_t>((uint16_t)(resourceBoneCount + bonesToAdd), (uint16_t)submeshData.maxBonesPerMaterial);

		// Check if we exceed max bones limit or if this is the last part
		if ((useBones && resourceBoneCount == submeshData.maxBonesPerMaterial) || partNum == partCount)
		{
			generateNewRenderResource = true;
		}
		if (generateNewRenderResource)
		{
			submeshData.renderResources[resourceNum].boneCount = resourceBoneCount;
			submeshData.renderResources[resourceNum].vertexCount = submeshData.visibleTriangleCount * 3 - startIndex;
			UserRenderResource*& renderResource = submeshData.renderResources[resourceNum].resource;	// Next resource
			++resourceNum;
			if (renderResource == NULL)	// Create if needed
			{
				UserRenderVertexBuffer* vertexBuffers[5] = { NULL };
				uint32_t numVertexBuffers = 0;

				if (submeshData.staticBufferReplacement != NULL)
				{
					vertexBuffers[numVertexBuffers++] = submeshData.staticBufferReplacement;
				}
				else if (mRenderMeshAsset->mRuntimeSubmeshData[submeshIndex].staticVertexBuffer != NULL)
				{
					vertexBuffers[numVertexBuffers++] = mRenderMeshAsset->mRuntimeSubmeshData[ submeshIndex ].staticVertexBuffer;
				}

				if (submeshData.userDynamicVertexBuffer != NULL && (submeshData.userSpecifiedData || submeshData.fallbackSkinningMemory != NULL))
				{
					vertexBuffers[numVertexBuffers++] = submeshData.userDynamicVertexBuffer;
				}
				else if (submeshData.dynamicBufferReplacement != NULL)
				{
					vertexBuffers[numVertexBuffers++] = submeshData.dynamicBufferReplacement;
				}
				else if (mRenderMeshAsset->mRuntimeSubmeshData[submeshIndex].dynamicVertexBuffer != NULL)
				{
					if (submeshData.userDynamicVertexBuffer != NULL && mReleaseResourcesIfNothingToRender)
					{
						rrm->releaseVertexBuffer(*submeshData.userDynamicVertexBuffer);
						submeshData.userDynamicVertexBuffer = NULL;
					}

					vertexBuffers[numVertexBuffers++] = mRenderMeshAsset->mRuntimeSubmeshData[submeshIndex].dynamicVertexBuffer;
				}

				if (useBones && mRenderMeshAsset->mRuntimeSubmeshData[submeshIndex].skinningVertexBuffer != NULL)
				{
					vertexBuffers[numVertexBuffers++] = mRenderMeshAsset->mRuntimeSubmeshData[submeshIndex].skinningVertexBuffer;
				}

				// Separate (instanced) buffer for bone indices
				if (mPerActorVertexBuffers.size())
				{
					vertexBuffers[numVertexBuffers++] = mPerActorVertexBuffers[ submeshIndex ];
				}

				PX_ASSERT(numVertexBuffers <= 5);

				UserRenderResourceDesc resourceDesc;
				resourceDesc.primitives = RenderPrimitiveType::TRIANGLES;

				resourceDesc.vertexBuffers = vertexBuffers;
				resourceDesc.numVertexBuffers = numVertexBuffers;

				resourceDesc.numVerts = submesh.getVertexBuffer().getVertexCount();

				resourceDesc.indexBuffer = submeshData.indexBuffer;
				resourceDesc.firstIndex = startIndex;
				resourceDesc.numIndices = submeshData.visibleTriangleCount * 3 - startIndex;

				// not assuming partcount == bonecount anymore
				//if (mRenderMeshAsset->getPartCount() > 1)
				const uint32_t numBones = mRenderMeshAsset->getBoneCount();
				if (numBones > 1 && useBones)
				{
					UserRenderBoneBufferDesc boneDesc;
					// we don't need to use the minimum of numBones and max bones because the 
					// bone buffer update contains the proper range
					boneDesc.maxBones = submeshData.maxBonesPerMaterial;
					boneDesc.hint = RenderBufferHint::DYNAMIC;
					boneDesc.buffersRequest[ RenderBoneSemantic::POSE ] = RenderDataFormat::FLOAT4x4;
					if (mKeepPreviousFrameBoneBuffer)
					{
						boneDesc.buffersRequest[ RenderBoneSemantic::PREVIOUS_POSE ] = RenderDataFormat::FLOAT4x4;
					}
					resourceDesc.boneBuffer = rrm->createBoneBuffer(boneDesc);
					PX_ASSERT(resourceDesc.boneBuffer);
					if (resourceDesc.boneBuffer)
					{
						resourceDesc.numBones = numBones;
					}
				}

				resourceDesc.instanceBuffer = submeshData.instanceBuffer;
				resourceDesc.numInstances = 0;

				if (!submeshData.isMaterialPointerValid)
				{
					// this should only be reached, when renderMeshActorLoadMaterialsLazily is true.
					// URR may not be called asynchronously in that case (for example in a render thread)
					ResourceProviderIntl* nrp = GetInternalApexSDK()->getInternalResourceProvider();
					if (nrp != NULL)
					{
						submeshData.material = nrp->getResource(submeshData.materialID);
						submeshData.isMaterialPointerValid = true;
					}
				}

				resourceDesc.material = submeshData.material;

				resourceDesc.submeshIndex = submeshIndex;

				resourceDesc.userRenderData = userRenderData;

				resourceDesc.cullMode = submesh.getVertexBuffer().getFormat().getWinding();

				if (resourceDesc.isValid()) // TODO: should probably make this an if-statement... -jgd // I did, -poh
				{
					renderResource = rrm->createResource(resourceDesc);
				}
			}

			if (renderResource != NULL)
			{
				renderResource->setIndexBufferRange(startIndex, submeshData.visibleTriangleCount * 3 - startIndex);
				startIndex = submeshData.visibleTriangleCount * 3;

				if (renderResource->getBoneBuffer() != NULL)
				{
					renderResource->setBoneBufferRange(0, resourceBoneCount);
					// TODO - LRR - make useBoneVisibilitySemantic work with >1 bone/part
					// if visible bone optimization enabled (as set in the actor desc)
					// {
					//		if (renderMesh->getBoneBuffer() != NULL)
					//		if we have a 1:1 bone:part mapping, as determined when asset is loaded (or authored)
					//			renderMesh->getBoneBuffer()->writeBuffer(RenderBoneSemantic::VISIBLE_INDEX, visibleParts.usedIndices(), sizeof(uint32_t), 0, visibleParts.usedCount());
					//		else
					//		{
					//			// run through index buffer, and find all bones referenced by visible verts and store in visibleBones
					//			renderMesh->getBoneBuffer()->writeBuffer(RenderBoneSemantic::VISIBLE_INDEX, visibleBones.usedIndices(), sizeof(uint32_t), 0, visibleBones.usedCount());
					//		}
					// }
				}
			}

			resourceBoneCount = 0;
		}
	}

	if (boneIndexStart == uint32_t(-1))
	{
		boneIndexStart = 0;
	}

	// KHA - Write temporary buffer to index buffer
	if(submeshData.indexBuffer != NULL && mPartVisibilityChanged)
	{
		submeshData.indexBuffer->writeBuffer(mPartIndexTempBuffer.begin(), sizeof(uint32_t), 0, submeshData.visibleTriangleCount*3);
	}

	// Write re-mapped bone indices
	if (mPerActorVertexBuffers.size())
	{
		if (mTransforms.size() > 1 && mKeepVisibleBonesPacked)
		{
			RenderVertexBufferData skinningWriteData;
			skinningWriteData.setSemanticData(RenderVertexSemantic::BONE_INDEX, mBoneIndexTempBuffer.begin() + boneIndexStart, sizeof(uint16_t), RenderDataFormat::USHORT1);
			if (submeshData.staticColorReplacement != NULL)
			{
				skinningWriteData.setSemanticData(RenderVertexSemantic::COLOR, submeshData.staticColorReplacement + boneIndexStart, sizeof(ColorRGBA), RenderDataFormat::R8G8B8A8);
			}
			mPerActorVertexBuffers[submeshIndex]->writeBuffer(skinningWriteData, boneIndexStart, boneIndexEnd - boneIndexStart);
		}
		else
		if (submeshData.staticColorReplacement != NULL)
		{
			RenderVertexBufferData skinningWriteData;
			skinningWriteData.setSemanticData(RenderVertexSemantic::COLOR, submeshData.staticColorReplacement, sizeof(ColorRGBA), RenderDataFormat::R8G8B8A8);
			mPerActorVertexBuffers[submeshIndex]->writeBuffer(skinningWriteData, 0, submesh.getVertexBuffer().getVertexCount());
		}
	}

	mBonePosesDirty = true;

#if VERBOSE
	printf("-updatePartVisibility(submeshIndex=%d, useBones=%s, userRenderData=0x%p)\n", submeshIndex, useBones ? "true" : "false", userRenderData);
#endif
}

void ApexRenderMeshActor::updateBonePoses(uint32_t submeshIndex)
{
// There can now be >1 bones per part
//	if (mRenderMeshAsset->getPartCount() > 1)
	if (mRenderMeshAsset->getBoneCount() > 1)
	{
		SubmeshData& submeshData = mSubmeshData[ submeshIndex ];
		PxMat44* boneTMs = mTransforms.begin();
		const uint32_t tmBufferSize = mKeepVisibleBonesPacked ? getRenderVisiblePartCount() : mTransforms.size();

		// Set up the previous bone buffer, if requested and available
		PxMat44* previousBoneTMs = NULL;
		if (!mPreviousFrameBoneBufferValid || mTransformsLastFrame.size() != mTransforms.size())
		{
			previousBoneTMs = boneTMs;
		}
		else
		if (!mKeepVisibleBonesPacked || mRemappedPreviousBoneTMs.size() == 0)
		{
			previousBoneTMs = mTransformsLastFrame.begin();
		}
		else
		{
			previousBoneTMs = mRemappedPreviousBoneTMs.begin();
		}

		uint32_t tmsRemaining = tmBufferSize;
		for (uint32_t i = 0; i < submeshData.renderResources.size(); ++i)
		{
			UserRenderResource* renderResource = submeshData.renderResources[i].resource;
			const uint32_t resourceBoneCount = submeshData.renderResources[i].boneCount;
			if (renderResource && renderResource->getBoneBuffer() != NULL)
			{
				RenderBoneBufferData boneWriteData;
				boneWriteData.setSemanticData(RenderBoneSemantic::POSE, boneTMs, sizeof(PxMat44), RenderDataFormat::FLOAT4x4);
				if (mKeepPreviousFrameBoneBuffer)
				{
					boneWriteData.setSemanticData(RenderBoneSemantic::PREVIOUS_POSE, previousBoneTMs, sizeof(PxMat44), RenderDataFormat::FLOAT4x4);
				}
				renderResource->getBoneBuffer()->writeBuffer(boneWriteData, 0, PxMin(tmsRemaining, resourceBoneCount));
				tmsRemaining -= resourceBoneCount;
				boneTMs += resourceBoneCount;
				previousBoneTMs += resourceBoneCount;
			}
		}
	}
}

void ApexRenderMeshActor::releaseSubmeshRenderResources(uint32_t submeshIndex)
{
#if VERBOSE
	printf("releaseSubmeshRenderResources()\n");
#endif

	if (submeshIndex >= mSubmeshData.size())
	{
		return;
	}

	UserRenderResourceManager* rrm = GetInternalApexSDK()->getUserRenderResourceManager();

	SubmeshData& submeshData = mSubmeshData[submeshIndex];
	for (uint32_t j = submeshData.renderResources.size(); j--;)
	{
		if (submeshData.renderResources[j].resource != NULL)
		{
			if (submeshData.renderResources[j].resource->getBoneBuffer() != NULL)
			{
				rrm->releaseBoneBuffer(*submeshData.renderResources[j].resource->getBoneBuffer());
			}
			rrm->releaseResource(*submeshData.renderResources[j].resource);
			submeshData.renderResources[j].resource = NULL;
		}
	}
	submeshData.renderResources.reset();
}


void ApexRenderMeshActor::releaseRenderResources()
{
#if VERBOSE
	printf("releaseRenderResources()\n");
#endif

	UserRenderResourceManager* rrm = GetInternalApexSDK()->getUserRenderResourceManager();

	for (uint32_t i = mSubmeshData.size(); i--;)
	{
		releaseSubmeshRenderResources(i);

		SubmeshData& submeshData = mSubmeshData[i];

		if (submeshData.indexBuffer != NULL)
		{
			rrm->releaseIndexBuffer(*submeshData.indexBuffer);
			submeshData.indexBuffer = NULL;
		}
		submeshData.instanceBuffer = NULL;

		if (submeshData.staticBufferReplacement != NULL)
		{
			rrm->releaseVertexBuffer(*submeshData.staticBufferReplacement);
			submeshData.staticBufferReplacement = NULL;
		}

		if (submeshData.dynamicBufferReplacement != NULL)
		{
			rrm->releaseVertexBuffer(*submeshData.dynamicBufferReplacement);
			submeshData.dynamicBufferReplacement = NULL;
		}

		if (submeshData.userDynamicVertexBuffer != NULL)
		{
			rrm->releaseVertexBuffer(*submeshData.userDynamicVertexBuffer);
			submeshData.userDynamicVertexBuffer = NULL;
		}
		submeshData.userIndexBufferChanged = false;
	}

	for (uint32_t i = mPerActorVertexBuffers.size(); i--;)
	{
		if (mPerActorVertexBuffers[i] != NULL)
		{
			rrm->releaseVertexBuffer(*mPerActorVertexBuffers[i]);
			mPerActorVertexBuffers[i] = NULL;
		}
	}
	mPerActorVertexBuffers.reset();

	if (mRenderResource)
	{
		rrm->releaseResource(*mRenderResource);
		mRenderResource = NULL;
	}

	mBoneBufferInUse = false;
}



bool ApexRenderMeshActor::submeshHasVisibleTriangles(uint32_t submeshIndex) const
{
	const ApexRenderSubmesh& submesh = *mRenderMeshAsset->mSubmeshes[submeshIndex];

	const uint32_t partCount = getRenderVisiblePartCount();
	const uint32_t* visiblePartIndexPtr = getRenderVisibleParts();

	for (uint32_t partNum = 0; partNum < partCount;)
	{
		const uint32_t partIndex = visiblePartIndexPtr[partNum++];
		const uint32_t indexCount = submesh.getIndexCount(partIndex);

		if (indexCount > 0)
		{
			return true;
		}
	}

	return false;
}



void ApexRenderMeshActor::createFallbackSkinning(uint32_t submeshIndex)
{
	if (mTransforms.size() == 1)
	{
		return;
	}

#if VERBOSE
	printf("createFallbackSkinning(submeshIndex=%d)\n", submeshIndex);
#endif
	const VertexBuffer& vertexBuffer = mRenderMeshAsset->getSubmesh(submeshIndex).getVertexBuffer();
	const VertexFormat& format = vertexBuffer.getFormat();

	const uint32_t bufferCount = format.getBufferCount();

	uint32_t bufferSize = 0;
	for (uint32_t bufferIndex = 0; bufferIndex < bufferCount; bufferIndex++)
	{
		if (format.getBufferAccess(bufferIndex) == RenderDataAccess::DYNAMIC)
		{
			RenderDataFormat::Enum bufferFormat = format.getBufferFormat(bufferIndex);
			RenderVertexSemantic::Enum bufferSemantic = format.getBufferSemantic(bufferIndex);

			if (bufferSemantic == RenderVertexSemantic::POSITION ||
			        bufferSemantic == RenderVertexSemantic::NORMAL ||
			        bufferSemantic == RenderVertexSemantic::TANGENT)
			{
				if (bufferFormat == RenderDataFormat::FLOAT3)
				{
					bufferSize += sizeof(PxVec3);
				}
				else if (bufferFormat == RenderDataFormat::FLOAT4)
				{
					bufferSize += sizeof(PxVec4);
				}
			}
		}
	}

	if (bufferSize > 0)
	{
		PX_ASSERT(mSubmeshData[submeshIndex].fallbackSkinningMemory == NULL);
		mSubmeshData[submeshIndex].fallbackSkinningMemorySize = bufferSize * vertexBuffer.getVertexCount();
		mSubmeshData[submeshIndex].fallbackSkinningMemory = PX_ALLOC(mSubmeshData[submeshIndex].fallbackSkinningMemorySize, "fallbackSkinnningMemory");

		PX_ASSERT(mSubmeshData[submeshIndex].fallbackSkinningDirty == false);

		if (!mSubmeshData[submeshIndex].userSpecifiedData)
		{
			distributeFallbackData(submeshIndex);
			mOneUserVertexBufferChanged = true;
		}
	}
}



void ApexRenderMeshActor::distributeFallbackData(uint32_t submeshIndex)
{
	const VertexBuffer& vertexBuffer = mRenderMeshAsset->getSubmesh(submeshIndex).getVertexBuffer();
	const VertexFormat& format = vertexBuffer.getFormat();
	const uint32_t bufferCount = format.getBufferCount();
	const uint32_t vertexCount = vertexBuffer.getVertexCount();

	unsigned char* memoryIterator = (unsigned char*)mSubmeshData[submeshIndex].fallbackSkinningMemory;

	uint32_t sizeUsed = 0;
	for (uint32_t bufferIndex = 0; bufferIndex < bufferCount; bufferIndex++)
	{
		if (format.getBufferAccess(bufferIndex) == RenderDataAccess::DYNAMIC)
		{
			RenderDataFormat::Enum bufferFormat = format.getBufferFormat(bufferIndex);
			RenderVertexSemantic::Enum bufferSemantic = format.getBufferSemantic(bufferIndex);

			if (bufferSemantic == RenderVertexSemantic::POSITION && bufferFormat == RenderDataFormat::FLOAT3)
			{
				mSubmeshData[submeshIndex].userPositions = (PxVec3*)memoryIterator;
				memoryIterator += sizeof(PxVec3) * vertexCount;
				sizeUsed += sizeof(PxVec3);
			}
			else if (bufferSemantic == RenderVertexSemantic::NORMAL && bufferFormat == RenderDataFormat::FLOAT3)
			{
				mSubmeshData[submeshIndex].userNormals = (PxVec3*)memoryIterator;
				memoryIterator += sizeof(PxVec3) * vertexCount;
				sizeUsed += sizeof(PxVec3);
			}
			else if (bufferSemantic == RenderVertexSemantic::TANGENT && bufferFormat == RenderDataFormat::FLOAT4)
			{
				mSubmeshData[submeshIndex].userTangents4 = (PxVec4*)memoryIterator;
				memoryIterator += sizeof(PxVec4) * vertexCount;
				sizeUsed += sizeof(PxVec4);
			}
		}
	}

	PX_ASSERT(sizeUsed * vertexCount == mSubmeshData[submeshIndex].fallbackSkinningMemorySize);
}



void ApexRenderMeshActor::updateFallbackSkinning(uint32_t submeshIndex)
{
	if (mSubmeshData[submeshIndex].fallbackSkinningMemory == NULL || mSubmeshData[submeshIndex].userSpecifiedData)
	{
		return;
	}

#if VERBOSE
	printf("updateFallbackSkinning(submeshIndex=%d)\n", submeshIndex);
#endif
	PX_PROFILE_ZONE("ApexRenderMesh::updateFallbackSkinning", GetInternalApexSDK()->getContextId());

	const VertexBuffer& vertexBuffer = mRenderMeshAsset->getSubmesh(submeshIndex).getVertexBuffer();
	const VertexFormat& format = vertexBuffer.getFormat();

	PxVec3* outPositions = mSubmeshData[submeshIndex].userPositions;
	PxVec3* outNormals = mSubmeshData[submeshIndex].userNormals;
	PxVec4* outTangents = mSubmeshData[submeshIndex].userTangents4;

	if (outPositions == NULL && outNormals == NULL && outTangents == NULL)
	{
		return;
	}

	RenderDataFormat::Enum inFormat;
	const uint32_t positionIndex = (uint32_t)format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::POSITION));
	const PxVec3* inPositions = (const PxVec3*)vertexBuffer.getBufferAndFormat(inFormat, positionIndex);
	PX_ASSERT(inPositions == NULL || inFormat == RenderDataFormat::FLOAT3);

	if (inPositions != NULL && mSubmeshData[submeshIndex].staticPositionReplacement != NULL)
	{
		inPositions = mSubmeshData[submeshIndex].staticPositionReplacement;
	}

	const uint32_t normalIndex = (uint32_t)format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::NORMAL));
	const PxVec3* inNormals = (const PxVec3*)vertexBuffer.getBufferAndFormat(inFormat, normalIndex);
	PX_ASSERT(inNormals == NULL || inFormat == RenderDataFormat::FLOAT3);

	const uint32_t tangentIndex = (uint32_t)format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::TANGENT));
	const PxVec4* inTangents = (const PxVec4*)vertexBuffer.getBufferAndFormat(inFormat, tangentIndex);
	PX_ASSERT(inTangents == NULL || inFormat == RenderDataFormat::FLOAT4);

	const uint32_t boneIndexIndex = (uint32_t)format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::BONE_INDEX));
	const uint16_t* inBoneIndices = (const uint16_t*)vertexBuffer.getBufferAndFormat(inFormat, boneIndexIndex);
	PX_ASSERT(inBoneIndices == NULL || inFormat == RenderDataFormat::USHORT1 || inFormat == RenderDataFormat::USHORT2 || inFormat == RenderDataFormat::USHORT3 || inFormat == RenderDataFormat::USHORT4);

	const uint32_t boneWeightIndex = (uint32_t)format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::BONE_WEIGHT));
	const float* inBoneWeights = (const float*)vertexBuffer.getBufferAndFormat(inFormat, boneWeightIndex);
	PX_ASSERT(inBoneWeights == NULL || inFormat == RenderDataFormat::FLOAT1 || inFormat == RenderDataFormat::FLOAT2 || inFormat == RenderDataFormat::FLOAT3 || inFormat == RenderDataFormat::FLOAT4);

	uint32_t numBonesPerVertex = 0;
	switch (inFormat)
	{
	case RenderDataFormat::FLOAT1:
		numBonesPerVertex = 1;
		break;
	case RenderDataFormat::FLOAT2:
		numBonesPerVertex = 2;
		break;
	case RenderDataFormat::FLOAT3:
		numBonesPerVertex = 3;
		break;
	case RenderDataFormat::FLOAT4:
		numBonesPerVertex = 4;
		break;
	default:
		break;
	}

	PX_ASSERT((inPositions != NULL) == (outPositions != NULL));
	PX_ASSERT((inNormals != NULL) == (outNormals != NULL));
	PX_ASSERT((inTangents != NULL) == (outTangents != NULL));

	if (inBoneWeights == NULL || inBoneIndices == NULL || numBonesPerVertex == 0)
	{
		return;
	}

	// clear all data
	nvidia::intrinsics::memSet(mSubmeshData[submeshIndex].fallbackSkinningMemory, 0, mSubmeshData[submeshIndex].fallbackSkinningMemorySize);

	const uint32_t vertexCount = vertexBuffer.getVertexCount();
	for (uint32_t i = 0; i < vertexCount; i++)
	{
		for (uint32_t k = 0; k < numBonesPerVertex; k++)
		{
			const uint32_t boneIndex = inBoneIndices[i * numBonesPerVertex + k];
			PX_ASSERT(boneIndex < mTransforms.size());
			PxMat44& transform = mTransforms[boneIndex];

			const float boneWeight = inBoneWeights[i * numBonesPerVertex + k];
			if (boneWeight > 0.0f)
			{
				if (outPositions != NULL)
				{
					outPositions[i] += transform.transform(inPositions[i]) * boneWeight;
				}

				if (outNormals != NULL)
				{
					outNormals[i] += transform.rotate(inNormals[i]) * boneWeight;
				}

				if (outTangents != NULL)
				{
					outTangents[i] += PxVec4(transform.rotate(inTangents[i].getXYZ()) * boneWeight, 0.0f);
				}
			}
		}
		if (outTangents != NULL)
		{
			outTangents[i].w = inTangents[i].w;
		}
	}

	mSubmeshData[submeshIndex].fallbackSkinningDirty = true;
}



void ApexRenderMeshActor::writeUserBuffers(uint32_t submeshIndex)
{
	PxVec3* outPositions = mSubmeshData[submeshIndex].userPositions;
	PxVec3* outNormals = mSubmeshData[submeshIndex].userNormals;
	PxVec4* outTangents4 = mSubmeshData[submeshIndex].userTangents4;

	if (outPositions == NULL && outNormals == NULL && outTangents4 == NULL)
	{
		return;
	}

	RenderVertexBufferData dynamicWriteData;
	if (outPositions != NULL)
	{
		dynamicWriteData.setSemanticData(RenderVertexSemantic::POSITION, outPositions, sizeof(PxVec3), RenderDataFormat::FLOAT3);
	}

	if (outNormals != NULL)
	{
		dynamicWriteData.setSemanticData(RenderVertexSemantic::NORMAL, outNormals, sizeof(PxVec3), RenderDataFormat::FLOAT3);
	}

	if (outTangents4)
	{
		dynamicWriteData.setSemanticData(RenderVertexSemantic::TANGENT, outTangents4, sizeof(PxVec4), RenderDataFormat::FLOAT4);
	}

	const uint32_t vertexCount = mRenderMeshAsset->mSubmeshes[submeshIndex]->getVertexBuffer().getVertexCount();

	mSubmeshData[submeshIndex].userDynamicVertexBuffer->writeBuffer(dynamicWriteData, 0, vertexCount);
}



void ApexRenderMeshActor::visualizeTangentSpace(RenderDebugInterface& batcher, float normalScale, float tangentScale, float bitangentScale, PxMat33* scaledRotations, PxVec3* translations, uint32_t stride, uint32_t numberOfTransforms) const
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(batcher);
	PX_UNUSED(normalScale);
	PX_UNUSED(tangentScale);
	PX_UNUSED(bitangentScale);
	PX_UNUSED(scaledRotations);
	PX_UNUSED(translations);
	PX_UNUSED(stride);
	PX_UNUSED(numberOfTransforms);
#else

	if (normalScale <= 0.0f && tangentScale <= 0.0f && bitangentScale <= 0.0f)
	{
		return;
	}

	using RENDER_DEBUG::DebugColors;
	uint32_t debugColorRed = RENDER_DEBUG_IFACE(&batcher)->getDebugColor(DebugColors::Red);
	uint32_t debugColorGreen = RENDER_DEBUG_IFACE(&batcher)->getDebugColor(DebugColors::Green);
	uint32_t debugColorBlue = RENDER_DEBUG_IFACE(&batcher)->getDebugColor(DebugColors::Blue);

	RENDER_DEBUG_IFACE(&batcher)->pushRenderState();

	const uint32_t submeshCount = mRenderMeshAsset->getSubmeshCount();
	PX_ASSERT(mSubmeshData.size() == submeshCount);
	for (uint32_t submeshIndex = 0; submeshIndex < submeshCount; ++submeshIndex)
	{
		const PxVec3* positions = NULL;
		const PxVec3* normals = NULL;
		const PxVec3* tangents = NULL;
		const PxVec4* tangents4 = NULL;

		const uint16_t* boneIndices = NULL;
		const float* boneWeights = NULL;

		uint32_t numBonesPerVertex = 0;

		if (mSubmeshData[submeshIndex].userSpecifiedData || mSubmeshData[submeshIndex].fallbackSkinningMemory != NULL)
		{
			positions = mSubmeshData[submeshIndex].userPositions;
			normals = mSubmeshData[submeshIndex].userNormals;
			tangents4 = mSubmeshData[submeshIndex].userTangents4;
		}
		else
		{
			const VertexBuffer& vertexBuffer = mRenderMeshAsset->getSubmesh(submeshIndex).getVertexBuffer();
			const VertexFormat& format = vertexBuffer.getFormat();

			RenderDataFormat::Enum inFormat;
			const uint32_t positionIndex = (uint32_t)format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::POSITION));
			positions = (const PxVec3*)vertexBuffer.getBufferAndFormat(inFormat, positionIndex);
			PX_ASSERT(positions == NULL || inFormat == RenderDataFormat::FLOAT3);

			if (positions != NULL && mSubmeshData[submeshIndex].staticPositionReplacement != NULL)
			{
				positions = mSubmeshData[submeshIndex].staticPositionReplacement;
			}

			const uint32_t normalIndex = (uint32_t)format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::NORMAL));
			normals = (const PxVec3*)vertexBuffer.getBufferAndFormat(inFormat, normalIndex);
			PX_ASSERT(normals == NULL || inFormat == RenderDataFormat::FLOAT3);

			const uint32_t tangentIndex = (uint32_t)format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::TANGENT));
			tangents = (const PxVec3*)vertexBuffer.getBufferAndFormat(inFormat, tangentIndex);
			PX_ASSERT(tangents == NULL || inFormat == RenderDataFormat::FLOAT3 || inFormat == RenderDataFormat::FLOAT4);
			if (inFormat == RenderDataFormat::FLOAT4)
			{
				tangents4 = (const PxVec4*)tangents;
				tangents = NULL;
			}

			const uint32_t boneIndexIndex = (uint32_t)format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::BONE_INDEX));
			boneIndices = (const uint16_t*)vertexBuffer.getBufferAndFormat(inFormat, boneIndexIndex);
			PX_ASSERT(boneIndices == NULL || inFormat == RenderDataFormat::USHORT1 || inFormat == RenderDataFormat::USHORT2 || inFormat == RenderDataFormat::USHORT3 || inFormat == RenderDataFormat::USHORT4);

			switch (inFormat)
			{
			case RenderDataFormat::USHORT1:
				numBonesPerVertex = 1;
				break;
			case RenderDataFormat::USHORT2:
				numBonesPerVertex = 2;
				break;
			case RenderDataFormat::USHORT3:
				numBonesPerVertex = 3;
				break;
			case RenderDataFormat::USHORT4:
				numBonesPerVertex = 4;
				break;
			default:
				break;
			}

			const uint32_t boneWeightIndex = (uint32_t)format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::BONE_WEIGHT));
			boneWeights = (const float*)vertexBuffer.getBufferAndFormat(inFormat, boneWeightIndex);
			PX_ASSERT(boneWeights == NULL || inFormat == RenderDataFormat::FLOAT1 || inFormat == RenderDataFormat::FLOAT2 || inFormat == RenderDataFormat::FLOAT3 || inFormat == RenderDataFormat::FLOAT4);
		}

		const uint32_t partCount = visiblePartCount();
		const uint32_t* visibleParts = getVisibleParts();
		for (uint32_t visiblePartIndex = 0; visiblePartIndex < partCount; visiblePartIndex++)
		{
			const uint32_t partIndex = visibleParts[visiblePartIndex];

			const RenderSubmesh& submesh = mRenderMeshAsset->getSubmesh(submeshIndex);
			const uint32_t vertexStart = submesh.getFirstVertexIndex(partIndex);
			const uint32_t vertexEnd = vertexStart + submesh.getVertexCount(partIndex);

			for (uint32_t i = vertexStart; i < vertexEnd; i++)
			{
				PxVec3 position(0.0f), tangent(0.0f), bitangent(0.0f), normal(0.0f);
				if (numBonesPerVertex == 0)
				{
					position = positions[i];
					if (normals != NULL)
					{
						normal = normals[i].getNormalized();
					}
					if (tangents4 != NULL)
					{
						tangent = tangents4[i].getXYZ().getNormalized();
						bitangent = normal.cross(tangent) * tangents4[i].w;
					}
					else if (tangents != NULL)
					{
						tangent = tangents[i].getNormalized();
						bitangent = normal.cross(tangent);
					}

				}
				else if (numBonesPerVertex == 1)
				{
					PX_ASSERT(boneIndices != NULL);
					uint32_t boneIndex = 0;
					if (mRenderWithoutSkinning)
					{
						boneIndex = 0;
					}
					else if (mKeepVisibleBonesPacked)
					{
						boneIndex = visiblePartIndex;
					}
					else
					{
						boneIndex = boneIndices[i];
					}

					const PxMat44& tm = mTransforms[boneIndex];
					position = tm.transform(positions[i]);
					if (normals != NULL)
					{
						normal = tm.rotate(normals[i].getNormalized());
					}
					if (tangents4 != NULL)
					{
						tangent = tm.rotate(tangents4[i].getXYZ().getNormalized());
						bitangent = normal.cross(tangent) * tangents4[i].w;
					}
					else if (tangents != NULL)
					{
						tangent = tm.rotate(tangents[i].getNormalized());
						bitangent = normal.cross(tangent);
					}
				}
				else
				{
					position = tangent = bitangent = normal = PxVec3(0.0f);
					for (uint32_t k = 0; k < numBonesPerVertex; k++)
					{
						const float weight = boneWeights[i * numBonesPerVertex + k];
						if (weight > 0.0f)
						{
							const PxMat44& tm = mTransforms[boneIndices[i * numBonesPerVertex + k]];
							position += tm.transform(positions[i]) * weight;
							if (normals != NULL)
							{
								normal += tm.rotate(normals[i]) * weight;
							}
							if (tangents4 != NULL)
							{
								tangent += tm.rotate(tangents4[i].getXYZ()) * weight;
							}
							else if (tangents != NULL)
							{
								tangent += tm.rotate(tangents[i]) * weight;
							}
						}
					}
					normal.normalize();
					tangent.normalize();
					if (tangents4 != NULL)
					{
						bitangent = normal.cross(tangent) * tangents4[i].w;
					}
					else if (tangents != NULL)
					{
						bitangent = normal.cross(tangent);
					}
				}

				if (numberOfTransforms == 0 || scaledRotations == NULL || translations == NULL)
				{
					if (!tangent.isZero() && tangentScale > 0.0f)
					{
						RENDER_DEBUG_IFACE(&batcher)->setCurrentColor(debugColorRed);
						RENDER_DEBUG_IFACE(&batcher)->debugLine(position, position + tangent * tangentScale);
					}

					if (!bitangent.isZero() && bitangentScale > 0.0f)
					{
						RENDER_DEBUG_IFACE(&batcher)->setCurrentColor(debugColorGreen);
						RENDER_DEBUG_IFACE(&batcher)->debugLine(position, position + bitangent * bitangentScale);
					}

					if (!normal.isZero() && normalScale > 0.0f)
					{
						RENDER_DEBUG_IFACE(&batcher)->setCurrentColor(debugColorBlue);
						RENDER_DEBUG_IFACE(&batcher)->debugLine(position, position + normal * normalScale);
					}
				}
				else	//instancing
				{
					for (uint32_t k = 0; k < numberOfTransforms; k++)
					{
						PxMat33& scaledRotation = *(PxMat33*)((uint8_t*)scaledRotations + k*stride);
						PxVec3& translation = *(PxVec3*)((uint8_t*)translations + k*stride);

						PxVec3 newPos = scaledRotation.transform(position) + translation;	//full transform

						PxVec3 newTangent = scaledRotation.transform(tangent);	//without translation
						PxVec3 newBitangent = scaledRotation.transform(bitangent);

						PxVec3 newNormal = (scaledRotation.getInverse()).getTranspose().transform(normal);

						if (!tangent.isZero() && tangentScale > 0.0f)
						{
							RENDER_DEBUG_IFACE(&batcher)->setCurrentColor(debugColorRed);
							RENDER_DEBUG_IFACE(&batcher)->debugLine(newPos, newPos + newTangent * tangentScale);
						}

						if (!bitangent.isZero() && bitangentScale > 0.0f)
						{
							RENDER_DEBUG_IFACE(&batcher)->setCurrentColor(debugColorGreen);
							RENDER_DEBUG_IFACE(&batcher)->debugLine(newPos, newPos + newBitangent * bitangentScale);
						}

						if (!normal.isZero() && normalScale > 0.0f)
						{
							RENDER_DEBUG_IFACE(&batcher)->setCurrentColor(debugColorBlue);
							RENDER_DEBUG_IFACE(&batcher)->debugLine(newPos, newPos + newNormal * normalScale);
						}
					}
				}
			}
		}

	}

	RENDER_DEBUG_IFACE(&batcher)->popRenderState();
#endif
}




ApexRenderMeshActor::SubmeshData::SubmeshData() :
	indexBuffer(NULL),
	fallbackSkinningMemory(NULL),
	userDynamicVertexBuffer(NULL),
	instanceBuffer(NULL),
	userPositions(NULL),
	userNormals(NULL),
	userTangents4(NULL),
	staticColorReplacement(NULL),
	staticColorReplacementDirty(false),
	staticPositionReplacement(NULL),
	staticBufferReplacement(NULL),
	dynamicBufferReplacement(NULL),
	fallbackSkinningMemorySize(0),
	visibleTriangleCount(0),
	materialID(INVALID_RESOURCE_ID),
	material(NULL),
	isMaterialPointerValid(false),
	maxBonesPerMaterial(0),
	indexBufferRequestedSize(0),
	userSpecifiedData(false),
	userVertexBufferAlwaysDirty(false),
	userIndexBufferChanged(false),
	fallbackSkinningDirty(false)
{
}



ApexRenderMeshActor::SubmeshData::~SubmeshData()
{
	if (fallbackSkinningMemory != NULL)
	{
		PX_FREE(fallbackSkinningMemory);
		fallbackSkinningMemory = NULL;
	}
	fallbackSkinningMemorySize = 0;
}



void ApexRenderMeshActor::setTM(const PxMat44& tm, uint32_t boneIndex /* = 0 */)
{
	WRITE_ZONE();
	PX_ASSERT(boneIndex < mRenderMeshAsset->getBoneCount());
	mBonePosesDirty = true;
	PxMat44& boneTM = accessTM(boneIndex);
	boneTM.column0 = tm.column0;
	boneTM.column1 = tm.column1;
	boneTM.column2 = tm.column2;
	boneTM.column3 = tm.column3;
}



void ApexRenderMeshActor::setTM(const PxMat44& tm, const PxVec3& scale, uint32_t boneIndex /* = 0 */)
{
	// Assumes tm is pure rotation.  This can allow some optimization.
	WRITE_ZONE();
	PX_ASSERT(boneIndex < mRenderMeshAsset->getBoneCount());
	mBonePosesDirty = true;
	PxMat44& boneTM = accessTM(boneIndex);
	boneTM.column0 = tm.column0;
	boneTM.column1 = tm.column1;
	boneTM.column2 = tm.column2;
	boneTM.column3 = tm.column3;
	boneTM.scale(PxVec4(scale, 1.f));
}


void ApexRenderMeshActor::setLastFrameTM(const PxMat44& tm, uint32_t boneIndex /* = 0 */)
{
	if (!mPreviousFrameBoneBufferValid)
	{
		return;
	}

	PX_ASSERT(boneIndex < mRenderMeshAsset->getBoneCount());
	PxMat44& boneTM = accessLastFrameTM(boneIndex);
	boneTM.column0 = tm.column0;
	boneTM.column1 = tm.column1;
	boneTM.column2 = tm.column2;
	boneTM.column3 = tm.column3;
}



void ApexRenderMeshActor::setLastFrameTM(const PxMat44& tm, const PxVec3& scale, uint32_t boneIndex /* = 0 */)
{
	if (!mPreviousFrameBoneBufferValid)
	{
		return;
	}

	// Assumes tm is pure rotation.  This can allow some optimization.

	PX_ASSERT(boneIndex < mRenderMeshAsset->getBoneCount());
	PxMat44& boneTM = accessLastFrameTM(boneIndex);
	boneTM.column0 = tm.column0;
	boneTM.column1 = tm.column1;
	boneTM.column2 = tm.column2;
	boneTM.column3 = tm.column3;
	boneTM.scale(PxVec4(scale, 1.f));
}


void ApexRenderMeshActor::setSkinningMode(RenderMeshActorSkinningMode::Enum mode)
{
	if (mode >= RenderMeshActorSkinningMode::Default && mode < RenderMeshActorSkinningMode::Count)
	{
		mSkinningMode = mode;
	}
}

RenderMeshActorSkinningMode::Enum ApexRenderMeshActor::getSkinningMode() const
{
	return mSkinningMode;
}


void ApexRenderMeshActor::syncVisibility(bool useLock)
{
	WRITE_ZONE();
	if (mApiVisibilityChanged && mBufferVisibility)
	{
		if (useLock)
		{
			lockRenderResources();
		}
		mVisiblePartsForRendering.resize(mVisiblePartsForAPI.usedCount());
		memcpy(mVisiblePartsForRendering.begin(), mVisiblePartsForAPI.usedIndices(), mVisiblePartsForAPI.usedCount()*sizeof(uint32_t));
		const uint32_t swapBufferSize = mTMSwapBuffer.size();
		for (uint32_t i = 0; i < swapBufferSize; ++i)
		{
			const uint32_t swapIndices = mTMSwapBuffer[i];
			nvidia::swap(mTransforms[swapIndices >> 16], mTransforms[swapIndices & 0xFFFF]);
		}
		mTMSwapBuffer.reset();
		mPartVisibilityChanged = true;
		mApiVisibilityChanged = false;
		if (useLock)
		{
			unlockRenderResources();
		}
	}
}

// TODO - LRR - update part bounds actor bounds to work with >1 bones per part
void ApexRenderMeshActor::updateBounds()
{
	mRenderBounds.setEmpty();
	const uint32_t* visiblePartIndexPtr = mVisiblePartsForAPI.usedIndices();
	const uint32_t* visiblePartIndexPtrStop = visiblePartIndexPtr + mVisiblePartsForAPI.usedCount();
	if (mTransforms.size() < mRenderMeshAsset->getPartCount())
	{
		// BRG - for static meshes.  We should create a mapping for more generality.
		PX_ASSERT(mTransforms.size() == 1);
		PxMat44& tm = accessTM();
		while (visiblePartIndexPtr < visiblePartIndexPtrStop)
		{
			const uint32_t partIndex = *visiblePartIndexPtr++;
			PxBounds3 partBounds = mRenderMeshAsset->getBounds(partIndex);
			partBounds = PxBounds3::basisExtent(tm.transform(partBounds.getCenter()), PxMat33(tm.getBasis(0), tm.getBasis(1), tm.getBasis(2)), partBounds.getExtents());
			mRenderBounds.include(partBounds);
		}
	}
	else
	{
		while (visiblePartIndexPtr < visiblePartIndexPtrStop)
		{
			const uint32_t partIndex = *visiblePartIndexPtr++;
			PxBounds3 partBounds = mRenderMeshAsset->getBounds(partIndex);
			PxMat44& tm = accessTM(partIndex);
			partBounds = PxBounds3::basisExtent(tm.transform(partBounds.getCenter()), PxMat33(tm.getBasis(0), tm.getBasis(1), tm.getBasis(2)), partBounds.getExtents());
			mRenderBounds.include(partBounds);
		}
	}
}

void ApexRenderMeshActor::updateInstances(uint32_t submeshIndex)
{
	PX_PROFILE_ZONE("ApexRenderMesh::updateInstances", GetInternalApexSDK()->getContextId());

	for (uint32_t i = 0; i < mSubmeshData[submeshIndex].renderResources.size(); ++i)
	{
		UserRenderResource* renderResource = mSubmeshData[submeshIndex].renderResources[i].resource;
		renderResource->setInstanceBufferRange(mInstanceOffset, mInstanceCount);
	}
}

void ApexRenderMeshActor::setReleaseResourcesIfNothingToRender(bool value)
{
	WRITE_ZONE();
	mReleaseResourcesIfNothingToRender = value;
}

void ApexRenderMeshActor::setBufferVisibility(bool bufferVisibility)
{
	WRITE_ZONE();
	mBufferVisibility = bufferVisibility;
	mPartVisibilityChanged = true;
}

void ApexRenderMeshActor::setOverrideMaterial(uint32_t index, const char* overrideMaterialName)
{
	WRITE_ZONE();
	ResourceProviderIntl* nrp = GetInternalApexSDK()->getInternalResourceProvider();
	if (nrp != NULL && index < mSubmeshData.size())
	{
		// do create before release, so we don't release the resource if the newID is the same as the old
		ResID materialNS = GetInternalApexSDK()->getMaterialNameSpace();

		ResID newID = nrp->createResource(materialNS, overrideMaterialName);
		nrp->releaseResource(mSubmeshData[index].materialID);

		mSubmeshData[index].materialID = newID;
		mSubmeshData[index].material = NULL;
		mSubmeshData[index].isMaterialPointerValid = false;
		mSubmeshData[index].maxBonesPerMaterial = 0;

		if (!GetInternalApexSDK()->getRMALoadMaterialsLazily())
		{
			loadMaterial(mSubmeshData[index]);
		}
	}
}

// Need an inverse
PX_INLINE PxMat44 inverse(const PxMat44& m)
{
	const PxMat33 invM33 = PxMat33(m.getBasis(0), m.getBasis(1), m.getBasis(2)).getInverse();
	return PxMat44(invM33, -(invM33.transform(m.getPosition())));
}

bool ApexRenderMeshActor::rayCast(RenderMeshActorRaycastHitData& hitData,
                                  const PxVec3& worldOrig, const PxVec3& worldDisp,
                                  RenderMeshActorRaycastFlags::Enum flags,
                                  RenderCullMode::Enum winding,
                                  int32_t partIndex) const
{
	READ_ZONE();
	PX_ASSERT(mRenderMeshAsset != NULL);
	PX_ASSERT(worldOrig.isFinite() && worldDisp.isFinite()  && !worldDisp.isZero());

	// Come up with a part range which matches the flags, and if partIndex > 0, ensure it lies within the part range
	uint32_t rankStart = (flags & RenderMeshActorRaycastFlags::VISIBLE_PARTS) != 0 ? 0 : mVisiblePartsForAPI.usedCount();
	uint32_t rankStop = (flags & RenderMeshActorRaycastFlags::INVISIBLE_PARTS) != 0 ? mRenderMeshAsset->getPartCount() : mVisiblePartsForAPI.usedCount();
	// We use the visibility index bank, since it holds visible and invisible parts contiguously
	if (rankStart >= rankStop)
	{
		return false;	// No parts selected for raycast
	}
	if (partIndex >= 0)
	{
		const uint32_t partRank = mVisiblePartsForAPI.getRank((uint32_t)partIndex);
		if (partRank < rankStart || partRank >= rankStop)
		{
			return false;
		}
		rankStart = partRank;
		rankStop = partRank + 1;
	}
	const uint32_t* partIndices = mVisiblePartsForAPI.usedIndices();

	// Allocate an inverse transform and local ray for each part and calculate them
	const uint32_t tmCount = mRenderWithoutSkinning ? 1 : rankStop - rankStart;	// Only need one transform if not skinning

	PX_ALLOCA(invTMs, PxMat44, tmCount);
	PX_ALLOCA(localOrigs, PxVec3, tmCount);
	PX_ALLOCA(localDisps, PxVec3, tmCount);

	if (mRenderWithoutSkinning)
	{
		invTMs[0] = inverse(mTransforms[0]);
		localOrigs[0] = invTMs[0].transform(worldOrig);
		localDisps[0] = invTMs[0].rotate(worldDisp);
	}
	else
	{
		for (uint32_t partRank = rankStart; partRank < rankStop; ++partRank)
		{
			invTMs[partRank - rankStart] = inverse(mTransforms[partRank - rankStart]);
			localOrigs[partRank - rankStart] = invTMs[partRank - rankStart].transform(worldOrig);
			localDisps[partRank - rankStart] = invTMs[partRank - rankStart].rotate(worldDisp);
		}
	}

	// Side "discriminant" - used to reduce branches in inner loops
	const float disc = winding == RenderCullMode::CLOCKWISE ? 1.0f : (winding == RenderCullMode::COUNTER_CLOCKWISE ? -1.0f : 0.0f);

	// Keeping hit time as a fraction
	float tNum = -1.0f;
	float tDen = 0.0f;

	// To do: handle multiple-weighted vertices, and other cases where the number of parts does not equal the number of bones (besides non-skinned, which we do handle)
//	if (single-weighted vertices)
	{
		// Traverse the selected parts:
		const uint32_t submeshCount = mRenderMeshAsset->getSubmeshCount();
		for (uint32_t submeshIndex = 0; submeshIndex < submeshCount; ++submeshIndex)
		{
			const RenderSubmesh& submesh = mRenderMeshAsset->getSubmesh(submeshIndex);
			const VertexBuffer& vertexBuffer = submesh.getVertexBuffer();
			const VertexFormat& vertexFormat = vertexBuffer.getFormat();
			RenderDataFormat::Enum positionFormat;
			const PxVec3* vertexPositions = (const PxVec3*)vertexBuffer.getBufferAndFormat(positionFormat, (uint32_t)vertexFormat.getBufferIndexFromID(vertexFormat.getSemanticID(nvidia::RenderVertexSemantic::POSITION)));
			if (positionFormat != RenderDataFormat::FLOAT3)
			{
				continue;	// Not handling any position format other than FLOAT3
			}
			for (uint32_t partRank = rankStart; partRank < rankStop; ++partRank)
			{
				const uint32_t cachedLocalIndex = mRenderWithoutSkinning ? 0 : partRank - rankStart;
				const PxVec3& localOrig = localOrigs[cachedLocalIndex];
				const PxVec3& localDisp = localDisps[cachedLocalIndex];
				const uint32_t partIndex = partIndices[partRank];
				const uint32_t* ib = submesh.getIndexBuffer(partIndex);
				const uint32_t* ibStop = ib + submesh.getIndexCount(partIndex);
				PX_ASSERT(submesh.getIndexCount(partIndex) % 3 == 0);
				for (; ib < ibStop; ib += 3)
				{
					const PxVec3 offsetVertices[3] = { vertexPositions[ib[0]] - localOrig, vertexPositions[ib[1]] - localOrig, vertexPositions[ib[2]] - localOrig };
					const PxVec3 triangleNormal = (offsetVertices[1] - offsetVertices[0]).cross(offsetVertices[2] - offsetVertices[0]);
					const float den = triangleNormal.dot(localDisp);
					if (den > -PX_EPS_F32 * PX_EPS_F32)
					{
						// Ray misses plane (or is too near parallel)
						continue;
					}
					const float sides[3] = { (offsetVertices[0].cross(offsetVertices[1])).dot(localDisp), (offsetVertices[1].cross(offsetVertices[2])).dot(localDisp), (offsetVertices[2].cross(offsetVertices[0])).dot(localDisp) };
					if ((int)(sides[0]*disc > 0.0f) | (int)(sides[1]*disc > 0.0f) | (int)(sides[2]*disc > 0.0f))
					{
						// Ray misses triangle
						continue;
					}
					// Ray has hit the triangle; calculate time of intersection
					const float num = offsetVertices[0].dot(triangleNormal);
					// Since den and tDen both have the same (negative) sign, this is equivalent to : if (num/den < tNum/tDen)
					if (num * tDen < tNum * den)
					{
						// This intersection is earliest
						tNum = num;
						tDen = den;
						hitData.partIndex = partIndex;
						hitData.submeshIndex = submeshIndex;
						hitData.vertexIndices[0] = ib[0];
						hitData.vertexIndices[1] = ib[1];
						hitData.vertexIndices[2] = ib[2];
					}
				}
			}
		}

		if (tDen == 0.0f)
		{
			// No intersection found
			return false;
		}

		// Found a triangle.  Fill in hit data
		hitData.time = tNum / tDen;

		// See if normal, tangent, or binormal can be found
		const RenderSubmesh& submesh = mRenderMeshAsset->getSubmesh(hitData.submeshIndex);
		const VertexBuffer& vertexBuffer = submesh.getVertexBuffer();
		const VertexFormat& vertexFormat = vertexBuffer.getFormat();

		const int32_t normalBufferIndex = vertexFormat.getBufferIndexFromID(vertexFormat.getSemanticID(nvidia::RenderVertexSemantic::NORMAL));
		const int32_t tangentBufferIndex = vertexFormat.getBufferIndexFromID(vertexFormat.getSemanticID(nvidia::RenderVertexSemantic::TANGENT));
		const int32_t binormalBufferIndex = vertexFormat.getBufferIndexFromID(vertexFormat.getSemanticID(nvidia::RenderVertexSemantic::BINORMAL));

		ExplicitRenderTriangle triangle;
		const bool haveNormal = vertexBuffer.getBufferData(&triangle.vertices[0].normal, nvidia::RenderDataFormat::FLOAT3, 0, (uint32_t)normalBufferIndex, hitData.vertexIndices[0], 1);
		const bool haveTangent = vertexBuffer.getBufferData(&triangle.vertices[0].tangent, nvidia::RenderDataFormat::FLOAT3, 0, (uint32_t)tangentBufferIndex, hitData.vertexIndices[0], 1);
		const bool haveBinormal = vertexBuffer.getBufferData(&triangle.vertices[0].binormal, nvidia::RenderDataFormat::FLOAT3, 0, (uint32_t)binormalBufferIndex, hitData.vertexIndices[0], 1);

		uint32_t fieldMask = 0;

		if (haveNormal)
		{
			vertexBuffer.getBufferData(&triangle.vertices[1].normal, nvidia::RenderDataFormat::FLOAT3, 0, (uint32_t)normalBufferIndex, hitData.vertexIndices[1], 1);
			vertexBuffer.getBufferData(&triangle.vertices[2].normal, nvidia::RenderDataFormat::FLOAT3, 0, (uint32_t)normalBufferIndex, hitData.vertexIndices[2], 1);
			fieldMask |= 1 << TriangleFrame::Normal_x | 1 << TriangleFrame::Normal_y | 1 << TriangleFrame::Normal_z;
		}
		else
		{
			hitData.normal = PxVec3(0.0f);
		}

		if (haveTangent)
		{
			vertexBuffer.getBufferData(&triangle.vertices[1].tangent, nvidia::RenderDataFormat::FLOAT3, 0, (uint32_t)tangentBufferIndex, hitData.vertexIndices[1], 1);
			vertexBuffer.getBufferData(&triangle.vertices[2].tangent, nvidia::RenderDataFormat::FLOAT3, 0, (uint32_t)tangentBufferIndex, hitData.vertexIndices[2], 1);
			fieldMask |= 1 << TriangleFrame::Tangent_x | 1 << TriangleFrame::Tangent_y | 1 << TriangleFrame::Tangent_z;
		}
		else
		{
			hitData.tangent = PxVec3(0.0f);
		}

		if (haveBinormal)
		{
			vertexBuffer.getBufferData(&triangle.vertices[1].binormal, nvidia::RenderDataFormat::FLOAT3, 0, (uint32_t)binormalBufferIndex, hitData.vertexIndices[1], 1);
			vertexBuffer.getBufferData(&triangle.vertices[2].binormal, nvidia::RenderDataFormat::FLOAT3, 0, (uint32_t)binormalBufferIndex, hitData.vertexIndices[2], 1);
			fieldMask |= 1 << TriangleFrame::Binormal_x | 1 << TriangleFrame::Binormal_y | 1 << TriangleFrame::Binormal_z;
		}
		else
		{
			hitData.binormal = PxVec3(0.0f);
		}

		if (fieldMask != 0)
		{
			// We know the positions are in the correct format from the check in the raycast
			const PxVec3* vertexPositions = (const PxVec3*)vertexBuffer.getBuffer(
				(uint32_t)vertexFormat.getBufferIndexFromID(vertexFormat.getSemanticID(nvidia::RenderVertexSemantic::POSITION)));
			triangle.vertices[0].position = vertexPositions[hitData.vertexIndices[0]];
			triangle.vertices[1].position = vertexPositions[hitData.vertexIndices[1]];
			triangle.vertices[2].position = vertexPositions[hitData.vertexIndices[2]];
			TriangleFrame frame(triangle, fieldMask);

			// Find the local hit position
			const uint32_t partRank = mVisiblePartsForAPI.getRank(hitData.partIndex);
			const uint32_t cachedLocalIndex = mRenderWithoutSkinning ? 0 : partRank - rankStart;
			const PxMat44& tm = mTransforms[mRenderWithoutSkinning ? 0 : hitData.partIndex];

			Vertex v;
			v.position = localOrigs[cachedLocalIndex] + hitData.time * localDisps[cachedLocalIndex];
			frame.interpolateVertexData(v);
			if (haveNormal)
			{
				hitData.normal = invTMs[cachedLocalIndex].getTranspose().rotate(v.normal);
				hitData.normal.normalize();
			}

			if (haveTangent)
			{
				hitData.tangent = tm.rotate(v.tangent);
				hitData.tangent.normalize();
			}

			if (haveBinormal)
			{
				hitData.binormal = tm.rotate(v.binormal);
				hitData.binormal.normalize();
			}
			else
			{
				if (haveNormal && haveTangent)
				{
					hitData.binormal = hitData.normal.cross(hitData.tangent);
					hitData.binormal.normalize();
				}
			}
		}

		return true;
	}
}

void ApexRenderMeshActor::visualize(RenderDebugInterface& batcher, nvidia::apex::DebugRenderParams* debugParams, PxMat33* scaledRotations, PxVec3* translations, uint32_t stride, uint32_t numberOfTransforms) const
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(batcher);
	PX_UNUSED(debugParams);
	PX_UNUSED(scaledRotations);
	PX_UNUSED(translations);
	PX_UNUSED(stride);
	PX_UNUSED(numberOfTransforms);
#else
	PX_ASSERT(&batcher != NULL);
	if ( !mEnableDebugVisualization ) return;

	// This implementation seems to work for destruction and clothing!
	const float scale = debugParams->Scale;
	visualizeTangentSpace(batcher, debugParams->RenderNormals * scale, debugParams->RenderTangents * scale, debugParams->RenderBitangents * scale, scaledRotations, translations, stride, numberOfTransforms);
#endif
}

} // namespace apex
} // namespace nvidia
