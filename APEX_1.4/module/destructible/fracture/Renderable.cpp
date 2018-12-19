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



#include "RTdef.h"
#if RT_COMPILE
#include "DestructibleActorImpl.h"

#include "Actor.h"
#include "Compound.h"
#include "Convex.h"

#include "../fracture/Renderable.h"

namespace nvidia
{
namespace fracture
{

Renderable::Renderable():
	mVertexBuffer(NULL),
	mIndexBuffer(NULL),
	mBoneBuffer(NULL),
	mVertexBufferSize(0),
	mIndexBufferSize(0),
	mBoneBufferSize(0),
	mVertexBufferSizeLast(0),
	mIndexBufferSizeLast(0),
	mBoneBufferSizeLast(0),
	mFullBufferDirty(true),
	valid(false)
{

}

Renderable::~Renderable()
{
	UserRenderResourceManager* rrm = GetInternalApexSDK()->getUserRenderResourceManager();
	if (mVertexBuffer != NULL )
	{
		rrm->releaseVertexBuffer(*mVertexBuffer);
	}
	if (mIndexBuffer != NULL )
	{
		rrm->releaseIndexBuffer(*mIndexBuffer);
	}
	if (mBoneBuffer != NULL)
	{
		rrm->releaseBoneBuffer(*mBoneBuffer);
	}
	for (uint32_t k = 0; k < mConvexGroups.size(); k++)
	{
		ConvexGroup& g = mConvexGroups[k];
		for (uint32_t j = 0; j < g.mSubMeshes.size(); j++)
		{
			SubMesh& s = g.mSubMeshes[j];
			if(s.renderResource != NULL)
			{
				rrm->releaseResource(*s.renderResource);
				s.renderResource = NULL;
			}
		}
	}
}

void Renderable::updateRenderResources(bool rewriteBuffers, void* userRenderData)
{
	UserRenderResourceManager* rrm = GetInternalApexSDK()->getUserRenderResourceManager();
	ResourceProviderIntl* nrp = GetInternalApexSDK()->getInternalResourceProvider();

	PX_ASSERT(rrm != NULL && nrp != NULL);
	if (rrm == NULL || nrp == NULL || mConvexGroups.empty())
	{
		valid = false;
		return;
	}

	if (rewriteBuffers)
	{
		mFullBufferDirty = true;
	}

	// Resize buffers if necessary: TODO: intelligently oversize
	// vertex buffer
	if (mVertexBufferSize > mVertexBufferSizeLast)
	{
		if (mVertexBuffer != NULL )
		{
			rrm->releaseVertexBuffer(*mVertexBuffer);
		}
		{
			UserRenderVertexBufferDesc desc;
			desc.uvOrigin = nvidia::apex::TextureUVOrigin::ORIGIN_BOTTOM_LEFT;
			desc.hint = RenderBufferHint::DYNAMIC;
			desc.maxVerts = mVertexBufferSize;
			desc.buffersRequest[RenderVertexSemantic::POSITION]   = RenderDataFormat::FLOAT3;
			desc.buffersRequest[RenderVertexSemantic::NORMAL]     = RenderDataFormat::FLOAT3;
			desc.buffersRequest[RenderVertexSemantic::TEXCOORD0]  = RenderDataFormat::FLOAT2;
			desc.buffersRequest[RenderVertexSemantic::BONE_INDEX] = RenderDataFormat::USHORT1;
			mVertexBuffer = rrm->createVertexBuffer(desc);
			PX_ASSERT(mVertexBuffer);
		}
		mFullBufferDirty = true;
	}
	// index buffer
	if (mIndexBufferSize > mIndexBufferSizeLast)
	{
		if (mIndexBuffer != NULL )
		{
			rrm->releaseIndexBuffer(*mIndexBuffer);
		}
		UserRenderIndexBufferDesc desc;
		desc.hint = RenderBufferHint::DYNAMIC;
		desc.maxIndices = mIndexBufferSize;
		desc.format = RenderDataFormat::UINT1;
		mIndexBuffer = rrm->createIndexBuffer(desc);
		PX_ASSERT(mIndexBuffer);
		mFullBufferDirty = true;
	}
	// bone buffer
	if (mBoneBufferSize > mBoneBufferSizeLast)
	{
		if (mBoneBuffer != NULL)
		{
			rrm->releaseBoneBuffer(*mBoneBuffer);
		}
		UserRenderBoneBufferDesc desc;
		desc.hint = RenderBufferHint::DYNAMIC;
		desc.maxBones = mBoneBufferSize;
		desc.buffersRequest[RenderBoneSemantic::POSE] = RenderDataFormat::FLOAT3x4;
		mBoneBuffer = rrm->createBoneBuffer(desc);
		PX_ASSERT(mBoneBuffer);
		mFullBufferDirty = true;
	}
	// Fill buffers
	if (mFullBufferDirty)
	{
		uint32_t vertexIdx = 0;
		uint32_t indexIdx = 0;
		uint32_t boneIdx = 0;
		for (uint32_t k = 0; k < mConvexGroups.size(); k++)
		{
			ConvexGroup& g = mConvexGroups[k];
			for (uint32_t j = 0; j < g.mSubMeshes.size(); j++)
			{
				SubMesh& s = g.mSubMeshes[j];
				if(s.renderResource != NULL)
				{
					rrm->releaseResource(*s.renderResource);
					s.renderResource = NULL;
				}
				UserRenderResourceDesc desc;
				desc.primitives = RenderPrimitiveType::TRIANGLES;
				// configure vertices
				desc.vertexBuffers = &mVertexBuffer;
				desc.numVertexBuffers = 1;
				desc.numVerts = g.mVertexCache.size();
				desc.firstVertex = vertexIdx;
				// configure indices;
				desc.indexBuffer = mIndexBuffer;
				desc.firstIndex = indexIdx;
				desc.numIndices = s.mIndexCache.size();
				// configure bones;
				desc.boneBuffer = mBoneBuffer;
				desc.numBones = g.mBoneCache.size();
				desc.firstBone = boneIdx;
				// configure other info
				desc.material = nrp->getResource(mMaterialInfo[j].mMaterialID);
				desc.submeshIndex = j;
				desc.userRenderData = userRenderData;
				// create 
				s.renderResource = rrm->createResource(desc);
				PX_ASSERT(s.renderResource);
				// copy indices into buffer
				PX_ASSERT(indexIdx+s.mIndexCache.size() <= mIndexBufferSize);
				mIndexBuffer->writeBuffer(s.mIndexCache.begin(),sizeof(*s.mIndexCache.begin()),indexIdx,s.mIndexCache.size());
				indexIdx += s.mIndexCache.size();
			}
			// copy vertices and bones
			{
				RenderVertexBufferData data;
				data.setSemanticData(RenderVertexSemantic::POSITION,  g.mVertexCache.begin(),   sizeof(*g.mVertexCache.begin()),   RenderDataFormat::FLOAT3);
				data.setSemanticData(RenderVertexSemantic::NORMAL,    g.mNormalCache.begin(),   sizeof(*g.mNormalCache.begin()),   RenderDataFormat::FLOAT3);
				data.setSemanticData(RenderVertexSemantic::TEXCOORD0, g.mTexcoordCache.begin(), sizeof(*g.mTexcoordCache.begin()), RenderDataFormat::FLOAT2);
				data.setSemanticData(RenderVertexSemantic::BONE_INDEX,g.mBoneIndexCache.begin(),sizeof(*g.mBoneIndexCache.begin()),RenderDataFormat::USHORT1);
				PX_ASSERT(vertexIdx + g.mVertexCache.size() <= mVertexBufferSize);
				mVertexBuffer->writeBuffer(data,vertexIdx,g.mVertexCache.size());
			}
			{
				RenderBoneBufferData data;
				data.setSemanticData(RenderBoneSemantic::POSE,g.mBoneCache.begin(),sizeof(*g.mBoneCache.begin()),RenderDataFormat::FLOAT4x4);
				PX_ASSERT(boneIdx + g.mBoneCache.size() <= mBoneBufferSize);
				mBoneBuffer->writeBuffer(data,boneIdx,g.mBoneCache.size());
			}
			vertexIdx += g.mVertexCache.size();
			boneIdx += g.mBoneCache.size();
		}
		mFullBufferDirty = false;
	}
	else // Bones only
	{
		uint32_t boneIdx = 0;
		for (uint32_t k = 0; k < mConvexGroups.size(); k++)
		{
			ConvexGroup& g = mConvexGroups[k];
			{
				RenderBoneBufferData data;
				data.setSemanticData(RenderBoneSemantic::POSE,g.mBoneCache.begin(),sizeof(*g.mBoneCache.begin()),RenderDataFormat::FLOAT4x4);
				mBoneBuffer->writeBuffer(data,boneIdx,g.mBoneCache.size());
			}
			boneIdx += g.mBoneCache.size();
		}
	}
	mVertexBufferSizeLast = mVertexBufferSize;
	mIndexBufferSizeLast = mIndexBufferSize;
	mBoneBufferSizeLast = mBoneBufferSize;
	valid = true;
}

void Renderable::dispatchRenderResources(UserRenderer& api)
{
	if (!valid)
	{
		return;
	}
	RenderContext ctx;
	ctx.local2world = PxMat44(PxIdentity);
	ctx.world2local = PxMat44(PxIdentity);
	for (uint32_t k = 0; k < mConvexGroups.size(); k++)
	{
		ConvexGroup& g = mConvexGroups[k];
		for (uint32_t j = 0; j < g.mSubMeshes.size(); j++)
		{
			SubMesh& s = g.mSubMeshes[j];
			if(s.renderResource && !s.mIndexCache.empty())
			{
				ctx.renderResource = s.renderResource;
				api.renderResource(ctx);
			}
		}
	}
}

// -----------------------Cache Update-----------------------------------
void Renderable::updateRenderCacheFull(Actor* actor)
{
	mVertexBufferSize = 0;
	mIndexBufferSize = 0;
	mBoneBufferSize = 0;
	// Resize SubMeshes if necessary
	const uint32_t numSubMeshes = actor->mActor->getRenderSubmeshCount();
	if( numSubMeshes == 0 )
	{
		return;
	}
	if( numSubMeshes != mMaterialInfo.size() )
	{
		mMaterialInfo.resize(numSubMeshes);
	}
	// grab material information
	if (ResourceProviderIntl* nrp = GetInternalApexSDK()->getInternalResourceProvider())
	{
		if (UserRenderResourceManager* rrm = GetInternalApexSDK()->getUserRenderResourceManager())
		{
			ResID materialNS = GetInternalApexSDK()->getMaterialNameSpace();
			for(uint32_t i = 0; i < numSubMeshes; i++)
			{
				if(mMaterialInfo[i].mMaxBones == 0)
				{
					mMaterialInfo[i].mMaterialID = nrp->createResource(materialNS,actor->mActor->getDestructibleAsset()->getRenderMeshAsset()->getMaterialName(i),false);
					mMaterialInfo[i].mMaxBones = rrm->getMaxBonesForMaterial(nrp->getResource(mMaterialInfo[i].mMaterialID));
				}
			}
		}
	}
	// Find bone limit
	uint32_t maxBones = mMaterialInfo[0].mMaxBones;
	for (uint32_t i = 1; i < mMaterialInfo.size(); i++)
	{
		if (mMaterialInfo[i].mMaxBones < maxBones)
		{
			maxBones = mMaterialInfo[i].mMaxBones;
		}
	}
	//maxBones = 1; // TEMPORARY: FIXES TEXTURE MAPPING PROBLEM
	//maxBones = rand(1,maxBones-1);
	// Count Convexes
	uint32_t numConvexes = 0;
	const Array<base::Compound*>& compounds = actor->getCompounds();
	for (uint32_t k = 0; k < compounds.size(); k++)
	{
		const Array<base::Convex*>& convexes = compounds[k]->getConvexes();
		numConvexes += convexes.size();
	}
	mBoneBufferSize += numConvexes;
	
	//maxBones is 0 when VTF rendering method is used for destructible(only)
	if(maxBones == 0)
		maxBones = mBoneBufferSize;

	// Create more groups if necessary
	uint32_t numGroups = numConvexes/maxBones + ((numConvexes%maxBones)?1:0);
	if (numGroups != mConvexGroups.size())
	{
		mConvexGroups.resize(numGroups);
	}
	// Resize convex caches and subMeshes if necessary
	for (uint32_t k = 0; k < mConvexGroups.size(); k++)
	{
		ConvexGroup& g = mConvexGroups[k];
		g.mConvexCache.clear();
		if( g.mConvexCache.capacity() <= maxBones )
		{
			g.mConvexCache.reserve(maxBones);
		}
		if( g.mSubMeshes.size() != mMaterialInfo.size())
		{
			g.mSubMeshes.resize(mMaterialInfo.size());
		}
	}
	// Populate convex cache
	{
		uint32_t idx = 0;
		const Array<base::Compound*>& compounds = actor->getCompounds();
		for (uint32_t k = 0; k < compounds.size(); k++)
		{
			const Array<base::Convex*>& convexes = compounds[k]->getConvexes();
			for (uint32_t j = 0; j < convexes.size(); j++)
			{
				mConvexGroups[idx/maxBones].mConvexCache.pushBack((Convex*)convexes[j]);
				idx++;
			}
		}
	}
	// Fill other caches
	for (uint32_t k = 0; k < mConvexGroups.size(); k++)
	{
		ConvexGroup& g = mConvexGroups[k];
		// Calculate number of vertices
		uint32_t numVertices = 0;
		for (uint32_t j = 0; j < g.mConvexCache.size(); j++)
		{
			numVertices += g.mConvexCache[j]->getVisVertices().size();
		}
		mVertexBufferSize += numVertices;
		// Resize if necessary
		g.mVertexCache.clear();
		g.mNormalCache.clear();
		g.mTexcoordCache.clear();
		g.mBoneIndexCache.clear();
		if (numVertices >= g.mVertexCache.capacity())
		{
			g.mVertexCache.reserve(numVertices);
			g.mNormalCache.reserve(numVertices);
			g.mBoneIndexCache.reserve(numVertices);
			g.mTexcoordCache.reserve(numVertices);
		}
		g.mBoneCache.clear();
		if (maxBones >= g.mBoneCache.capacity())
		{
			g.mBoneCache.reserve(maxBones);
		}
		// Calculate index buffer sizes
		for (uint32_t i = 0; i < g.mSubMeshes.size(); i++)
		{
			uint32_t numIndices = 0;
			for (uint32_t j = 0; j < g.mConvexCache.size(); j++)
			{
				numIndices += g.mConvexCache[j]->getVisTriIndices().size();			
			}	
			g.mSubMeshes[i].mIndexCache.clear();
			if (numIndices >= g.mSubMeshes[i].mIndexCache.capacity())
			{
				g.mSubMeshes[i].mIndexCache.reserve(numIndices);
			}
			mIndexBufferSize += numIndices;
		}
		// Fill for each convex
		for (uint32_t j = 0; j < g.mConvexCache.size(); j++)
		{
			Convex* c = g.mConvexCache[j];
			uint32_t off = g.mVertexCache.size();
			// fill vertices
			const Array<PxVec3>& verts = c->getVisVertices();
			const Array<PxVec3>& norms = c->getVisNormals();
			const Array<float>& texcs = c->getVisTexCoords();
			PX_ASSERT(verts.size() == norms.size() && verts.size() == (texcs.size()/2));
			for (uint32_t i = 0; i < verts.size(); i++)
			{
				g.mVertexCache.pushBack(verts[i]);
				g.mNormalCache.pushBack(norms[i]);
				g.mTexcoordCache.pushBack(PxVec2(texcs[2*i],texcs[2*i+1]));
				g.mBoneIndexCache.pushBack((uint16_t)j);
			}
			// fill indicies for each submesh
			for (uint32_t i = 0; i < g.mSubMeshes.size(); i++)
			{
				const Array<int32_t>& indices = c->getVisTriIndices();
				PX_ASSERT(indices.size()%3 == 0);
				for (uint32_t a = 0; a < indices.size()/3; a++)
				{
					uint32_t subMeshID = 0;														// <<<--- TODO: acquire subMeshID for triangle
					if (subMeshID == i)
					{
						g.mSubMeshes[i].mIndexCache.pushBack(indices[3*a+0]+off);
						g.mSubMeshes[i].mIndexCache.pushBack(indices[3*a+1]+off);
						g.mSubMeshes[i].mIndexCache.pushBack(indices[3*a+2]+off);
					}					
				}
			}
			g.mBoneCache.pushBack(c->getGlobalPose());
		}
	}
}

void Renderable::updateRenderCache(Actor* actor)
{
	if( actor == NULL ) return;
	actor->mScene->getScene()->lockRead();
	//actor->mRenderResourcesDirty = true;
	if( actor->mRenderResourcesDirty)
	{
		updateRenderCacheFull(actor);
		mFullBufferDirty = true;
		actor->mRenderResourcesDirty = false;
	}
	// Fill other caches
	for (uint32_t k = 0; k < mConvexGroups.size(); k++)
	{
		ConvexGroup& g = mConvexGroups[k];
		g.mBoneCache.clear();
		// Fill bones for each convex
		for (uint32_t j = 0; j < g.mConvexCache.size(); j++)
		{
			Convex* c = g.mConvexCache[j];
			g.mBoneCache.pushBack(c->getGlobalPose());
		}
	}
	actor->mScene->getScene()->unlockRead();
}

PxBounds3 Renderable::getBounds() const
{
	PxBounds3 bounds;
	bounds.setEmpty();
	for (uint32_t k = 0; k < mConvexGroups.size(); k++)
	{
		const ConvexGroup& g = mConvexGroups[k];
		for (uint32_t j = 0; j < g.mConvexCache.size(); j++)
		{
			const Convex* c = g.mConvexCache[j];
			bounds.include(c->getBounds());
		}
	}
	return bounds;
}

}
}
#endif