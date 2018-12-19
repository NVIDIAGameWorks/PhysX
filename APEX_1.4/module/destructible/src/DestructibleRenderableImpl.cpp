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



#include "DestructibleRenderable.h"
#include "RenderMeshActor.h"
#include "DestructibleActorImpl.h"
#include "ModulePerfScope.h"

namespace nvidia
{
namespace destructible
{

DestructibleRenderableImpl::DestructibleRenderableImpl(RenderMeshActor* renderMeshActors[DestructibleActorMeshType::Count], DestructibleAssetImpl* asset, int32_t listIndex)
: mAsset(asset)
, mListIndex(listIndex)
, mRefCount(1)	// Ref count initialized to 1, assuming that whoever calls this constructor will store a reference
{
	for (uint32_t i = 0; i < DestructibleActorMeshType::Count; ++i)
	{
		mRenderMeshActors[i] = renderMeshActors[i];
	}
}

DestructibleRenderableImpl::~DestructibleRenderableImpl()
{
	WRITE_ZONE();
	for (uint32_t i = 0; i < DestructibleActorMeshType::Count; ++i)
	{
		if (mRenderMeshActors[i])
		{
			mRenderMeshActors[i]->release();
			mRenderMeshActors[i] = NULL;
		}
	}
}

void DestructibleRenderableImpl::release()
{
	bool triggerDelete = false;
	lockRenderResources();
	if (mRefCount > 0)
	{
		triggerDelete = !(--mRefCount);
	}
	unlockRenderResources();
	if (triggerDelete)
	{
		delete this;
	}
}

void DestructibleRenderableImpl::updateRenderResources(bool rewriteBuffers, void* userRenderData)
{
	PX_PROFILE_ZONE("DestructibleRenderableUpdateRenderResources", GetInternalApexSDK()->getContextId());

	URR_SCOPE;

	for (uint32_t typeN = 0; typeN < DestructibleActorMeshType::Count; ++typeN)
	{
		RenderMeshActorIntl* renderMeshActor = (RenderMeshActorIntl*)mRenderMeshActors[typeN];
		if (renderMeshActor != NULL)
		{
			renderMeshActor->updateRenderResources((typeN == DestructibleActorMeshType::Skinned), rewriteBuffers, userRenderData);
		}
	}

	// Render instanced meshes
	if (mAsset->m_instancingRepresentativeActorIndex == -1)
	{
		mAsset->m_instancingRepresentativeActorIndex = (int32_t)mListIndex;	// using this actor as our representative
	}
	if ((int32_t)mListIndex == mAsset->m_instancingRepresentativeActorIndex)	// doing it this way, in case (for some reason) someone wants to call this fn twice per frame
	{
		mAsset->updateChunkInstanceRenderResources(rewriteBuffers, userRenderData);
		
		mAsset->createScatterMeshInstanceInfo();

		for (uint32_t i = 0; i < mAsset->m_scatterMeshInstanceInfo.size(); ++i)
		{
			DestructibleAssetImpl::ScatterMeshInstanceInfo& info = mAsset->m_scatterMeshInstanceInfo[i];
			RenderMeshActorIntl* renderMeshActor = (RenderMeshActorIntl*)info.m_actor;
			if (renderMeshActor != NULL)
			{
				RenderInstanceBufferData data;
				physx::Array<DestructibleAssetImpl::ScatterInstanceBufferDataElement>& instanceBufferData = info.m_instanceBufferData;
				const uint32_t instanceBufferSize = instanceBufferData.size();

				if (info.m_instanceBuffer != NULL && instanceBufferSize > 0)
				{
					// If a new actor has added instances for this scatter mesh, recreate a larger instance buffer
					if (info.m_IBSize < instanceBufferSize)
					{
						UserRenderResourceManager* rrm = GetInternalApexSDK()->getUserRenderResourceManager();
						rrm->releaseInstanceBuffer(*info.m_instanceBuffer);

						UserRenderInstanceBufferDesc instanceBufferDesc = mAsset->getScatterMeshInstanceBufferDesc();
						instanceBufferDesc.maxInstances = instanceBufferSize;
						
						info.m_instanceBuffer = rrm->createInstanceBuffer(instanceBufferDesc);
						info.m_IBSize = instanceBufferSize;
						
						renderMeshActor->setInstanceBuffer(info.m_instanceBuffer);
						renderMeshActor->setMaxInstanceCount(instanceBufferSize);
					}

					info.m_instanceBuffer->writeBuffer(&instanceBufferData[0], 0, instanceBufferSize);
				}
				renderMeshActor->setInstanceBufferRange(0, instanceBufferSize);
				renderMeshActor->updateRenderResources(false, rewriteBuffers, userRenderData);
			}
		}
	}

#if APEX_RUNTIME_FRACTURE
	mRTrenderable.updateRenderResources(rewriteBuffers,userData);
#endif
}

void DestructibleRenderableImpl::dispatchRenderResources(UserRenderer& renderer)
{
	PX_PROFILE_ZONE("DestructibleRenderableDispatchRenderResources", GetInternalApexSDK()->getContextId());

	for (uint32_t typeN = 0; typeN < DestructibleActorMeshType::Count; ++typeN)
	{
		RenderMeshActor* renderMeshActor = mRenderMeshActors[typeN];
		if (renderMeshActor != NULL)
		{
			renderMeshActor->dispatchRenderResources(renderer);
		}
	}

	// Render instanced meshes
	if ((int32_t)mListIndex == mAsset->m_instancingRepresentativeActorIndex)
	{
		for (uint32_t i = 0; i < mAsset->m_instancedChunkRenderMeshActors.size(); ++i)
		{
			if (mAsset->m_instancedChunkRenderMeshActors[i] != NULL)
			{
				mAsset->m_instancedChunkRenderMeshActors[i]->dispatchRenderResources(renderer);
			}
		}

		for (uint32_t i = 0; i < mAsset->m_scatterMeshInstanceInfo.size(); ++i)
		{
			DestructibleAssetImpl::ScatterMeshInstanceInfo& scatterMeshInstanceInfo = mAsset->m_scatterMeshInstanceInfo[i];
			if (scatterMeshInstanceInfo.m_actor != NULL)
			{
				scatterMeshInstanceInfo.m_actor->dispatchRenderResources(renderer);
			}
		}
	}
#if APEX_RUNTIME_FRACTURE
	mRTrenderable.dispatchRenderResources(renderer);
#endif
}

DestructibleRenderableImpl* DestructibleRenderableImpl::incrementReferenceCount()
{
	DestructibleRenderableImpl* returnValue = NULL;
	lockRenderResources();
	if (mRefCount > 0)
	{
		++mRefCount;
		returnValue = this;
	}
	unlockRenderResources();
	return returnValue;
}

}
} // end namespace nvidia
