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
#include "ModuleDestructibleImpl.h"
#include "DestructiblePreviewImpl.h"
#include "DestructibleAssetImpl.h"
#include "DestructiblePreviewParam.h"
#include "RenderMeshAssetIntl.h"

namespace nvidia
{
namespace destructible
{

DestructiblePreviewImpl::DestructiblePreviewImpl(DestructiblePreview* _api, DestructibleAssetImpl& _asset, const NvParameterized::Interface* params) :
	ApexPreview(),
	m_asset(&_asset),
	m_api(_api),
	m_instancedChunkCount(0)
{
	for (int meshN = 0; meshN < DestructibleActorMeshType::Count; ++meshN)
	{
		m_renderMeshActors[meshN] = NULL;
	}

	const DestructiblePreviewParam* destructiblePreviewParams = DYNAMIC_CAST(const DestructiblePreviewParam*)(params);

	PX_ALLOCA(skinnedMaterialNames, const char*, destructiblePreviewParams->overrideSkinnedMaterialNames.arraySizes[0] > 0 ? destructiblePreviewParams->overrideSkinnedMaterialNames.arraySizes[0] : 1);
	for (int i = 0; i < destructiblePreviewParams->overrideSkinnedMaterialNames.arraySizes[0]; ++i)
	{
		skinnedMaterialNames[i] = destructiblePreviewParams->overrideSkinnedMaterialNames.buf[i].buf;
	}

	PX_ASSERT(m_asset->getRenderMeshAsset());
	RenderMeshActorDesc renderableMeshDesc;
	renderableMeshDesc.visible = false;
	renderableMeshDesc.indexBufferHint = RenderBufferHint::DYNAMIC;
	renderableMeshDesc.keepVisibleBonesPacked = true;
	renderableMeshDesc.overrideMaterials		= destructiblePreviewParams->overrideSkinnedMaterialNames.arraySizes[0] > 0 ? (const char**)skinnedMaterialNames : NULL;
	renderableMeshDesc.overrideMaterialCount	= (uint32_t)destructiblePreviewParams->overrideSkinnedMaterialNames.arraySizes[0];
	m_renderMeshActors[DestructibleActorMeshType::Skinned] = m_asset->getRenderMeshAsset()->createActor(renderableMeshDesc);
	const uint32_t numParts = m_asset->getRenderMeshAsset()->getPartCount();
	for (uint32_t i = 0; i < numParts; ++i)
	{
		m_renderMeshActors[DestructibleActorMeshType::Skinned]->setVisibility(false, (uint16_t)i);
	}
	m_renderMeshActors[DestructibleActorMeshType::Skinned]->updateBounds();

	m_drawUnexpandedChunksStatically = destructiblePreviewParams->renderUnexplodedChunksStatically;

	if (m_drawUnexpandedChunksStatically)
	{
		PX_ALLOCA(staticMaterialNames, const char*, destructiblePreviewParams->overrideStaticMaterialNames.arraySizes[0] > 0 ? destructiblePreviewParams->overrideStaticMaterialNames.arraySizes[0] : 1);
		for (int i = 0; i < destructiblePreviewParams->overrideStaticMaterialNames.arraySizes[0]; ++i)
		{
			staticMaterialNames[i] = destructiblePreviewParams->overrideStaticMaterialNames.buf[i].buf;
		}

		// Create static render mesh
		renderableMeshDesc.renderWithoutSkinning = true;
		renderableMeshDesc.overrideMaterials		= destructiblePreviewParams->overrideStaticMaterialNames.arraySizes[0] > 0 ? (const char**)staticMaterialNames : NULL;
		renderableMeshDesc.overrideMaterialCount	= (uint32_t)destructiblePreviewParams->overrideStaticMaterialNames.arraySizes[0];
		m_renderMeshActors[DestructibleActorMeshType::Static] = m_asset->getRenderMeshAsset()->createActor(renderableMeshDesc);
		for (uint32_t i = 0; i < numParts; ++i)
		{
			m_renderMeshActors[DestructibleActorMeshType::Static]->setVisibility(false, (uint16_t)i);
		}
		m_renderMeshActors[DestructibleActorMeshType::Static]->updateBounds();
	}

	// Instanced actors
	physx::Array<uint16_t> tempPartToActorMap;
	tempPartToActorMap.resize((uint32_t)m_asset->mParams->chunks.arraySizes[0], 0xFFFF);

	m_instancedChunkActorMap.resize((uint32_t)m_asset->mParams->chunkInstanceInfo.arraySizes[0]);
	for (int32_t i = 0; i < m_asset->mParams->chunkInstanceInfo.arraySizes[0]; ++i)
	{
		uint16_t partIndex = m_asset->mParams->chunkInstanceInfo.buf[i].partIndex;
		if (tempPartToActorMap[partIndex] == 0xFFFF)
		{
			tempPartToActorMap[partIndex] = m_instancedChunkCount++;
		}
		m_instancedChunkActorMap[(uint32_t)i] = tempPartToActorMap[partIndex];
	}

	m_instancedActorVisiblePart.resize(m_instancedChunkCount);
	for (int32_t i = 0; i < m_asset->mParams->chunks.arraySizes[0]; ++i)
	{
		DestructibleAssetParametersNS::Chunk_Type& chunk = m_asset->mParams->chunks.buf[i];
		if ((chunk.flags & DestructibleAssetImpl::Instanced) != 0)
		{
			uint16_t partIndex = m_asset->mParams->chunkInstanceInfo.buf[chunk.meshPartIndex].partIndex;
			m_instancedActorVisiblePart[m_instancedChunkActorMap[chunk.meshPartIndex]] = partIndex;
		}
	}

	setChunkVisibility(0, true);

	m_chunkDepth = destructiblePreviewParams->chunkDepth;
	m_explodeAmount = destructiblePreviewParams->explodeAmount;
	setPose(destructiblePreviewParams->globalPose);
	m_userData = reinterpret_cast<void*>(destructiblePreviewParams->userData);
}

DestructiblePreviewImpl::~DestructiblePreviewImpl()
{
}

// called at the end of 'setPose' which allows for any module specific updates.
void DestructiblePreviewImpl::setPose(const PxMat44& pose)
{
	ApexPreview::setPose(pose);
	setExplodeView(m_chunkDepth, m_explodeAmount);
}

void DestructiblePreviewImpl::setExplodeView(uint32_t depth, float explode)
{
	m_chunkDepth = PxClamp(depth, (uint32_t)0, m_asset->getDepthCount() - 1);
	const float newExplodeAmount = PxMax(explode, 0.0f);

	if (m_drawUnexpandedChunksStatically)
	{
		// Using a static mesh for unexploded chunks
		if (m_explodeAmount == 0.0f && newExplodeAmount != 0.0f)
		{
			// Going from unexploded to exploded.  Need to make the static mesh invisible.
			if (m_renderMeshActors[DestructibleActorMeshType::Static] != NULL)
			{
				for (uint32_t i = 0; i < m_asset->getChunkCount(); ++i)
				{
					m_renderMeshActors[DestructibleActorMeshType::Static]->setVisibility(false, (uint16_t)i);
				}
			}
		}
		else if (m_explodeAmount != 0.0f && newExplodeAmount == 0.0f)
		{
			// Going from exploded to unexploded.  Need to make the skinned mesh invisible.
			if (m_renderMeshActors[DestructibleActorMeshType::Skinned] != NULL)
			{
				for (uint32_t i = 0; i < m_asset->getChunkCount(); ++i)
				{
					m_renderMeshActors[DestructibleActorMeshType::Skinned]->setVisibility(false, (uint16_t)i);
				}
			}
		}
	}

	m_explodeAmount = newExplodeAmount;

	for (uint32_t i = 0; i < m_instancedChunkRenderMeshActors.size(); ++i)
	{
		PX_ASSERT(i < m_chunkInstanceBufferData.size());
		m_chunkInstanceBufferData[i].resize(0);
	}

	if (m_asset->getRenderMeshAsset() != NULL)
	{
		PxBounds3 bounds = m_asset->getRenderMeshAsset()->getBounds();
		PxVec3 c = bounds.getCenter();

		for (uint16_t i = 0; i < (uint16_t)m_asset->getChunkCount(); ++i)
		{
			setChunkVisibility(i, m_asset->mParams->chunks.buf[i].depth == m_chunkDepth);
		}

		DestructibleAssetParametersNS::Chunk_Type* sourceChunks = m_asset->mParams->chunks.buf;

		mRenderBounds.setEmpty();

		// Iterate over all visible chunks
		const uint16_t* indexPtr = mVisibleChunks.usedIndices();
		const uint16_t* indexPtrStop = indexPtr + mVisibleChunks.usedCount();
		DestructibleAssetParametersNS::InstanceInfo_Type* instanceDataArray = m_asset->mParams->chunkInstanceInfo.buf;	
		while (indexPtr < indexPtrStop)
		{
			const uint16_t index = *indexPtr++;
			if (index < m_asset->getChunkCount())
			{
				DestructibleAssetParametersNS::Chunk_Type& sourceChunk = sourceChunks[index];
				PxMat44 pose = mPose;
				if ((sourceChunk.flags & DestructibleAssetImpl::Instanced) == 0)
				{
					// Not instanced - only need to set skinned tms from chunks
					if (!m_drawUnexpandedChunksStatically || m_explodeAmount != 0.0f)
					{
						PxBounds3 partBounds = m_asset->getRenderMeshAsset()->getBounds(sourceChunk.meshPartIndex);
						PxVec3 partC = partBounds.getCenter();
						pose.setPosition(pose.getPosition() + m_explodeAmount*pose.rotate(partC-c));
						m_renderMeshActors[DestructibleActorMeshType::Skinned]->setTM(pose, sourceChunk.meshPartIndex);
					}
				}
				else
				{
					// Instanced
					PX_ASSERT(sourceChunk.meshPartIndex < m_asset->mParams->chunkInstanceInfo.arraySizes[0]);
					DestructibleAssetImpl::ChunkInstanceBufferDataElement instanceDataElement;
					DestructibleAssetParametersNS::InstanceInfo_Type& instanceData = instanceDataArray[sourceChunk.meshPartIndex];
					const uint16_t instancedActorIndex = m_instancedChunkActorMap[sourceChunk.meshPartIndex];
					physx::Array<DestructibleAssetImpl::ChunkInstanceBufferDataElement>& instanceBufferData = m_chunkInstanceBufferData[instancedActorIndex];
					instanceDataElement.scaledRotation = PxMat33(pose.getBasis(0), pose.getBasis(1), pose.getBasis(2));
					const PxVec3 globalOffset = instanceDataElement.scaledRotation*instanceData.chunkPositionOffset;
					PxBounds3 partBounds = m_asset->getRenderMeshAsset()->getBounds(instanceData.partIndex);
					partBounds.minimum += globalOffset;
					partBounds.maximum += globalOffset;
					PxVec3 partC = partBounds.getCenter();
					instanceDataElement.translation = pose.getPosition() + globalOffset + m_explodeAmount*pose.rotate(partC-c);
					instanceDataElement.uvOffset = instanceData.chunkUVOffset;
					instanceDataElement.localOffset = instanceData.chunkPositionOffset;
					instanceBufferData.pushBack(instanceDataElement);
					// Transform bounds
					PxVec3 center, extents;
					center = partBounds.getCenter();
					extents = partBounds.getExtents();
					center = instanceDataElement.scaledRotation.transform(center) + instanceDataElement.translation;
					extents = PxVec3(PxAbs(instanceDataElement.scaledRotation(0, 0) * extents.x) + PxAbs(instanceDataElement.scaledRotation(0, 1) * extents.y) + PxAbs(instanceDataElement.scaledRotation(0, 2) * extents.z),
											PxAbs(instanceDataElement.scaledRotation(1, 0) * extents.x) + PxAbs(instanceDataElement.scaledRotation(1, 1) * extents.y) + PxAbs(instanceDataElement.scaledRotation(1, 2) * extents.z),
											PxAbs(instanceDataElement.scaledRotation(2, 0) * extents.x) + PxAbs(instanceDataElement.scaledRotation(2, 1) * extents.y) + PxAbs(instanceDataElement.scaledRotation(2, 2) * extents.z));
					mRenderBounds.include(PxBounds3::centerExtents(center, extents));
				}
			}
		}

		m_renderMeshActors[DestructibleActorMeshType::Skinned]->updateBounds();
		mRenderBounds.include(m_renderMeshActors[DestructibleActorMeshType::Skinned]->getBounds());

		// If a static mesh exists, set its (single) tm from the destructible's tm
		if (m_renderMeshActors[DestructibleActorMeshType::Static] != NULL)
		{
			m_renderMeshActors[DestructibleActorMeshType::Static]->syncVisibility();
			m_renderMeshActors[DestructibleActorMeshType::Static]->setTM(mPose);
			m_renderMeshActors[DestructibleActorMeshType::Static]->updateBounds();
			mRenderBounds.include(m_renderMeshActors[DestructibleActorMeshType::Static]->getBounds());
		}
	}
}

void DestructiblePreviewImpl::updateRenderResources(bool rewriteBuffers, void* userRenderData)
{
	for (uint32_t i = 0; i < DestructibleActorMeshType::Count; ++i)
	{
		if (m_renderMeshActors[i] != NULL)
		{
			RenderMeshActorIntl* renderMeshActor = (RenderMeshActorIntl*)m_renderMeshActors[i];
			renderMeshActor->updateRenderResources((i == DestructibleActorMeshType::Skinned), rewriteBuffers, userRenderData);
		}
	}

	setInstancedChunkCount(m_instancedChunkCount);

	for (uint32_t i = 0; i < m_instancedChunkRenderMeshActors.size(); ++i)
	{
		PX_ASSERT(i < m_chunkInstanceBufferData.size());
		RenderMeshActorIntl* renderMeshActor = (RenderMeshActorIntl*)m_instancedChunkRenderMeshActors[i];
		if (renderMeshActor != NULL)
		{
			physx::Array<DestructibleAssetImpl::ChunkInstanceBufferDataElement>& instanceBufferData = m_chunkInstanceBufferData[i];
			const uint32_t instanceBufferSize = instanceBufferData.size();
			if (instanceBufferSize > 0)
			{
				m_chunkInstanceBuffers[i]->writeBuffer(&instanceBufferData[0], 0, instanceBufferSize);
			}
			renderMeshActor->setInstanceBufferRange(0, instanceBufferSize);
			renderMeshActor->updateRenderResources(false, rewriteBuffers, userRenderData);
		}
	}
}

void DestructiblePreviewImpl::dispatchRenderResources(UserRenderer& renderer)
{
	for (uint32_t i = 0; i < DestructibleActorMeshType::Count; ++i)
	{
		if (m_renderMeshActors[i] != NULL)
		{
			m_renderMeshActors[i]->dispatchRenderResources(renderer);
		}
	}

	for (uint32_t i = 0; i < m_instancedChunkRenderMeshActors.size(); ++i)
	{
		PX_ASSERT(i < m_chunkInstanceBufferData.size());
		if (m_instancedChunkRenderMeshActors[i] != NULL)
		{
			if (m_chunkInstanceBufferData[i].size() > 0)
			{
				m_instancedChunkRenderMeshActors[i]->dispatchRenderResources(renderer);
			}
		}
	}
}

void DestructiblePreviewImpl::release()
{
	if (mInRelease)
	{
		return;
	}
	mInRelease = true;
	destroy();
}

void DestructiblePreviewImpl::destroy()
{
	ApexPreview::destroy();

	for (uint32_t i = 0; i < DestructibleActorMeshType::Count; ++i)
	{
		if (m_renderMeshActors[i] != NULL)
		{
			m_renderMeshActors[i]->release();
		}
	}

	setInstancedChunkCount(0);
}

void DestructiblePreviewImpl::setInstancedChunkCount(uint32_t count)
{
	UserRenderResourceManager* rrm = GetInternalApexSDK()->getUserRenderResourceManager();

	const uint32_t oldCount = m_instancedChunkRenderMeshActors.size();
	for (uint32_t i = count; i < oldCount; ++i)
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
	m_instancedChunkRenderMeshActors.resize(count);
	m_chunkInstanceBuffers.resize(count);
	m_chunkInstanceBufferData.resize(count);
	for (uint32_t index = oldCount; index < count; ++index)
	{
		m_instancedChunkRenderMeshActors[index] = NULL;
		m_chunkInstanceBuffers[index] = NULL;
		m_chunkInstanceBufferData[index].reset();

		// Find out how many potential instances there are
		uint32_t maxInstanceCount = 0;
		for (int32_t i = 0; i < m_asset->mParams->chunkInstanceInfo.arraySizes[0]; ++i)
		{
			if (m_asset->mParams->chunkInstanceInfo.buf[i].partIndex == m_instancedActorVisiblePart[index])
			{
				++maxInstanceCount;
			}
		}

		// Create instance buffer
		UserRenderInstanceBufferDesc instanceBufferDesc;
		instanceBufferDesc.maxInstances = maxInstanceCount;
		instanceBufferDesc.hint = RenderBufferHint::DYNAMIC;
		instanceBufferDesc.semanticOffsets[RenderInstanceLayoutElement::POSITION_FLOAT3] = DestructibleAssetImpl::ChunkInstanceBufferDataElement::translationOffset();
		instanceBufferDesc.semanticOffsets[RenderInstanceLayoutElement::ROTATION_SCALE_FLOAT3x3] = DestructibleAssetImpl::ChunkInstanceBufferDataElement::scaledRotationOffset();
		instanceBufferDesc.semanticOffsets[RenderInstanceLayoutElement::UV_OFFSET_FLOAT2] = DestructibleAssetImpl::ChunkInstanceBufferDataElement::uvOffsetOffset();
		instanceBufferDesc.semanticOffsets[RenderInstanceLayoutElement::LOCAL_OFFSET_FLOAT3] = DestructibleAssetImpl::ChunkInstanceBufferDataElement::localOffsetOffset();
		instanceBufferDesc.stride = sizeof(DestructibleAssetImpl::ChunkInstanceBufferDataElement);
		m_chunkInstanceBuffers[index] = rrm->createInstanceBuffer(instanceBufferDesc);

		// Instance buffer data
		m_chunkInstanceBufferData[index].reserve(maxInstanceCount);
		m_chunkInstanceBufferData[index].resize(0);

		// Create actor
		if (m_asset->renderMeshAsset != NULL)
		{
			RenderMeshActorDesc renderableMeshDesc;
			renderableMeshDesc.maxInstanceCount = maxInstanceCount;
			renderableMeshDesc.renderWithoutSkinning = true;
			renderableMeshDesc.visible = false;
			m_instancedChunkRenderMeshActors[index] = m_asset->renderMeshAsset->createActor(renderableMeshDesc);
			m_instancedChunkRenderMeshActors[index]->setInstanceBuffer(m_chunkInstanceBuffers[index]);
			m_instancedChunkRenderMeshActors[index]->setVisibility(true, m_instancedActorVisiblePart[index]);
		}
	}
}

}
} // end namespace nvidia

