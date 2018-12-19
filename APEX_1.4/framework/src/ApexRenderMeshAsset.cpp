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



#include "ApexRenderMeshAsset.h"
#include "ApexRenderMeshActor.h"
#include "ApexSharedUtils.h"

#include "ApexSDKIntl.h"
#include "ResourceProviderIntl.h"

namespace nvidia
{
namespace apex
{


// ApexRenderMeshAsset functions

ApexRenderMeshAsset::ApexRenderMeshAsset(ResourceList& list, const char* name, AuthObjTypeID ownerModuleID) :
	mOwnerModuleID(ownerModuleID),
	mParams(NULL),
	mOpaqueMesh(NULL),
	mName(name)
{
	list.add(*this);
}



ApexRenderMeshAsset::~ApexRenderMeshAsset()
{
	// this should have been cleared in releaseActor()
	PX_ASSERT(mRuntimeSubmeshData.empty());

	// Release named resources
	ResourceProviderIntl* resourceProvider = GetInternalApexSDK()->getInternalResourceProvider();
	for (uint32_t i = 0 ; i < mMaterialIDs.size() ; i++)
	{
		resourceProvider->releaseResource(mMaterialIDs[i]);
	}

	setSubmeshCount(0);
}



void ApexRenderMeshAsset::destroy()
{
	for (uint32_t i = 0; i < mSubmeshes.size(); i++)
	{
		mSubmeshes[i]->setParams(NULL, NULL);
	}

	if (mParams != NULL)
	{
		if (!mParams->isReferenced)
		{
			mParams->destroy();
		}
		mParams = NULL;
	}

	// this is necessary so that all the actors will be destroyed before the destructor runs
	mActorList.clear();

	delete this;
}



bool ApexRenderMeshAsset::createFromParameters(RenderMeshAssetParameters* params)
{
	mParams = params;

	NvParameterized::Handle handle(*mParams);
	uint32_t size;

	// submeshes
	mParams->getParameterHandle("submeshes", handle);
	mParams->getArraySize(handle, (int32_t&)size);
	setSubmeshCount(size);
	for (uint32_t i = 0; i < size; ++i)
	{
		NvParameterized::Handle elementHandle(*mParams);
		handle.getChildHandle((int32_t)i, elementHandle);
		NvParameterized::Interface* submeshParams = NULL;
		mParams->getParamRef(elementHandle, submeshParams);

		mSubmeshes[i]->setParams(static_cast<SubmeshParameters*>(submeshParams), NULL);
	}

	createLocalData();

	return true;
}

// Load all of our named resources (that consists of materials) if they are
// not registered in the NRP
uint32_t ApexRenderMeshAsset::forceLoadAssets()
{
	uint32_t assetLoadedCount = 0;
	ResourceProviderIntl* nrp = GetInternalApexSDK()->getInternalResourceProvider();
	ResID materialNS = GetInternalApexSDK()->getMaterialNameSpace();

	for (uint32_t i = 0; i < mMaterialIDs.size(); i++)
	{

		if (!nrp->checkResource(materialNS, mParams->materialNames.buf[i]))
		{
			/* we know for SURE that createResource() has already been called, so just getResource() */
			nrp->getResource(mMaterialIDs[i]);
			assetLoadedCount++;
		}
	}

	return assetLoadedCount;
}


RenderMeshActor* ApexRenderMeshAsset::createActor(const RenderMeshActorDesc& desc)
{
	return PX_NEW(ApexRenderMeshActor)(desc, *this, mActorList);
}



void ApexRenderMeshAsset::releaseActor(RenderMeshActor& renderMeshActor)
{
	ApexRenderMeshActor* actor = DYNAMIC_CAST(ApexRenderMeshActor*)(&renderMeshActor);
	actor->destroy();

	// Last one out turns out the lights
	if (!mActorList.getSize())
	{
		UserRenderResourceManager* rrm = GetInternalApexSDK()->getUserRenderResourceManager();
		for (uint32_t i = 0 ; i < mRuntimeSubmeshData.size() ; i++)
		{
			if (mRuntimeSubmeshData[i].staticVertexBuffer != NULL)
			{
				rrm->releaseVertexBuffer(*mRuntimeSubmeshData[i].staticVertexBuffer);
				mRuntimeSubmeshData[i].staticVertexBuffer = NULL;
			}
			if (mRuntimeSubmeshData[i].skinningVertexBuffer != NULL)
			{
				rrm->releaseVertexBuffer(*mRuntimeSubmeshData[i].skinningVertexBuffer);
				mRuntimeSubmeshData[i].skinningVertexBuffer = NULL;
			}
			if (mRuntimeSubmeshData[i].dynamicVertexBuffer != NULL)
			{
				rrm->releaseVertexBuffer(*mRuntimeSubmeshData[i].dynamicVertexBuffer);
				mRuntimeSubmeshData[i].dynamicVertexBuffer = NULL;
			}
		}
		mRuntimeSubmeshData.clear();
	}
}



void ApexRenderMeshAsset::permuteBoneIndices(const physx::Array<int32_t>& old2new)
{
	int32_t maxBoneIndex = -1;
	for (uint32_t i = 0; i < mSubmeshes.size(); i++)
	{
		RenderDataFormat::Enum format;
		const VertexBuffer& vb = mSubmeshes[i]->getVertexBuffer();
		const VertexFormat& vf = vb.getFormat();
		uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(nvidia::RenderVertexSemantic::BONE_INDEX));
		uint16_t* boneIndices = (uint16_t*)vb.getBufferAndFormat(format, bufferIndex);
		if (boneIndices == NULL)
		{
			continue;
		}

		uint32_t numBonesPerVertex = 0;
		switch (format)
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
			continue;
		}

		const uint32_t numVertices = vb.getVertexCount();
		for (uint32_t j = 0; j < numVertices; j++)
		{
			for (uint32_t k = 0; k < numBonesPerVertex; k++)
			{
				uint16_t& index = boneIndices[j * numBonesPerVertex + k];
				PX_ASSERT(old2new[index] >= 0);
				PX_ASSERT(old2new[index] <= 0xffff);
				index = (uint16_t)old2new[index];
				maxBoneIndex = PxMax(maxBoneIndex, (int32_t)index);
			}
		}
	}
	mParams->boneCount = (uint32_t)maxBoneIndex + 1;
}


void ApexRenderMeshAsset::reverseWinding()
{
	for (uint32_t submeshId = 0; submeshId < mSubmeshes.size(); submeshId++)
	{
		uint32_t numIndices = mSubmeshes[submeshId]->getTotalIndexCount();
		// assume that all of the parts are contiguous
		uint32_t* indices = mSubmeshes[submeshId]->getIndexBufferWritable(0);
		for (uint32_t i = 0; i < numIndices; i += 3)
		{
			nvidia::swap<uint32_t>(indices[i + 1], indices[i + 2]);
		}
	}

	updatePartBounds();
}

void ApexRenderMeshAsset::applyTransformation(const PxMat44& transformation, float scale)
{
	for (uint32_t submeshId = 0; submeshId < mSubmeshes.size(); submeshId++)
	{
		VertexBufferIntl& vb = mSubmeshes[submeshId]->getVertexBufferWritable();
		vb.applyScale(scale);
		vb.applyTransformation(transformation);
	}

	// if the transform will mirror the mesh, change the triangle winding in the ib

	const PxMat33 tm(transformation.column0.getXYZ(),
						transformation.column1.getXYZ(),
						transformation.column2.getXYZ());

	if (tm.getDeterminant() * scale < 0.0f)
	{
		reverseWinding();
	}
	else
	{
		updatePartBounds();
	}
}



void ApexRenderMeshAsset::applyScale(float scale)
{
	for (uint32_t submeshId = 0; submeshId < mSubmeshes.size(); submeshId++)
	{
		VertexBufferIntl& vb = mSubmeshes[submeshId]->getVertexBufferWritable();
		vb.applyScale(scale);
	}

	for (int partId = 0; partId < mParams->partBounds.arraySizes[0]; partId++)
	{
		PX_ASSERT(!mParams->partBounds.buf[partId].isEmpty());
		mParams->partBounds.buf[partId].minimum *= scale;
		mParams->partBounds.buf[partId].maximum *= scale;
	}

	if (scale < 0.0f)
	{
		for (int partId = 0; partId < mParams->partBounds.arraySizes[0]; partId++)
		{
			PX_ASSERT(!mParams->partBounds.buf[partId].isEmpty());
			nvidia::swap(mParams->partBounds.buf[partId].minimum, mParams->partBounds.buf[partId].maximum);
		}
	}
}



bool ApexRenderMeshAsset::mergeBinormalsIntoTangents()
{
	bool changed = false;
	for (uint32_t submeshId = 0; submeshId < mSubmeshes.size(); submeshId++)
	{
		VertexBufferIntl& vb = mSubmeshes[submeshId]->getVertexBufferWritable();
		changed |= vb.mergeBinormalsIntoTangents();
	}
	return changed;
}



TextureUVOrigin::Enum ApexRenderMeshAsset::getTextureUVOrigin() const
{
	PX_ASSERT(mParams->textureUVOrigin < 4);
	return static_cast<TextureUVOrigin::Enum>(mParams->textureUVOrigin);
}



void ApexRenderMeshAsset::createLocalData()
{
	mMaterialIDs.resize((uint32_t)mParams->materialNames.arraySizes[0]);
	ResourceProviderIntl* resourceProvider = GetInternalApexSDK()->getInternalResourceProvider();
	ResID materialNS = GetInternalApexSDK()->getMaterialNameSpace();
	ResID customVBNS = GetInternalApexSDK()->getCustomVBNameSpace();


	// Resolve material names using the NRP...
	for (uint32_t i = 0; i < (uint32_t)mParams->materialNames.arraySizes[0]; ++i)
	{
		if (resourceProvider)
		{
			mMaterialIDs[i] = resourceProvider->createResource(materialNS, mParams->materialNames.buf[i]);
		}
		else
		{
			mMaterialIDs[i] = INVALID_RESOURCE_ID;
		}
	}

	// Resolve custom vertex buffer semantics using the NRP...
	mRuntimeCustomSubmeshData.resize(getSubmeshCount());
	//JPB memset(mRuntimeCustomSubmeshData.begin(), 0, sizeof(CustomSubmeshData) * mRuntimeCustomSubmeshData.size());

	for (uint32_t i = 0; i < getSubmeshCount(); ++i)
	{
		const VertexFormat& fmt = getSubmesh(i).getVertexBuffer().getFormat();

		mRuntimeCustomSubmeshData[i].customBufferFormats.resize(fmt.getCustomBufferCount());
		mRuntimeCustomSubmeshData[i].customBufferVoidPtrs.resize(fmt.getCustomBufferCount());

		uint32_t customBufferIndex = 0;
		for (uint32_t j = 0; j < fmt.getBufferCount(); ++j)
		{
			if (fmt.getBufferSemantic(j) != RenderVertexSemantic::CUSTOM)
			{
				continue;
			}
			RenderDataFormat::Enum f = fmt.getBufferFormat(j);
			const char* name = fmt.getBufferName(j);

			mRuntimeCustomSubmeshData[i].customBufferFormats[customBufferIndex] = f;
			mRuntimeCustomSubmeshData[i].customBufferVoidPtrs[customBufferIndex] = 0;

			if (resourceProvider)
			{
				ResID id = resourceProvider->createResource(customVBNS, name, true);
				mRuntimeCustomSubmeshData[i].customBufferVoidPtrs[customBufferIndex] = GetInternalApexSDK()->getInternalResourceProvider()->getResource(id);
			}

			++customBufferIndex;
		}
	}

	// find the bone count
	// LRR - required for new deserialize path
	// PH - mBoneCount is now serialized
	if (mParams->boneCount == 0)
	{
		for (uint32_t i = 0; i < getSubmeshCount(); i++)
		{

			RenderDataFormat::Enum format;
			const VertexBuffer& vb = mSubmeshes[i]->getVertexBuffer();
			const VertexFormat& vf = vb.getFormat();
			uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(nvidia::RenderVertexSemantic::BONE_INDEX));
			uint16_t* boneIndices = (uint16_t*)vb.getBufferAndFormat(format, bufferIndex);

			if (boneIndices == NULL)
			{
				continue;
			}

			if (!vertexSemanticFormatValid(RenderVertexSemantic::BONE_INDEX, format))
			{
				continue;
			}

			const uint32_t bonesPerVert = vertexSemanticFormatElementCount(RenderVertexSemantic::BONE_INDEX, format);

			PX_ASSERT(format == RenderDataFormat::USHORT1 || format == RenderDataFormat::USHORT2 || format == RenderDataFormat::USHORT3 || format == RenderDataFormat::USHORT4);

			const uint32_t numVertices = vb.getVertexCount();
			for (uint32_t v = 0; v < numVertices; v++)
			{
				for (uint32_t b = 0; b < bonesPerVert; b++)
				{
					mParams->boneCount = PxMax(mParams->boneCount, (uint32_t)(boneIndices[v * bonesPerVert + b] + 1));
				}
			}
		}
	}

	// PH - have one bone at all times, if it's just one, it is used as current pose (see ApexRenderMeshActor::dispatchRenderResources)
	if (mParams->boneCount == 0)
	{
		mParams->boneCount = 1;
	}
}

void ApexRenderMeshAsset::getStats(RenderMeshAssetStats& stats) const
{
	stats.totalBytes = sizeof(ApexRenderMeshAsset);

	for (int i = 0; i < mParams->materialNames.arraySizes[0]; ++i)
	{
		stats.totalBytes += (uint32_t) strlen(mParams->materialNames.buf[i]) + 1;
	}

	stats.totalBytes += mParams->partBounds.arraySizes[0] * sizeof(PxBounds3);
	stats.totalBytes += mName.len() + 1;

	stats.submeshCount = mSubmeshes.size();
	stats.partCount = (uint32_t)mParams->partBounds.arraySizes[0];
	stats.vertexCount = 0;
	stats.indexCount = 0;
	stats.vertexBufferBytes = 0;
	stats.indexBufferBytes = 0;

	for (uint32_t i = 0; i < mSubmeshes.size(); ++i)
	{
		const ApexRenderSubmesh& submesh =	*mSubmeshes[i];

		submesh.addStats(stats);
	}
}


void ApexRenderMeshAsset::updatePartBounds()
{
	for (int i = 0; i < mParams->partBounds.arraySizes[0]; i++)
	{
		mParams->partBounds.buf[i].setEmpty();
	}

	for (uint32_t i = 0; i < mSubmeshes.size(); i++)
	{
		const uint32_t* part = mSubmeshes[i]->mParams->vertexPartition.buf;

		RenderDataFormat::Enum format;
		const VertexBuffer& vb = mSubmeshes[i]->getVertexBuffer();
		const VertexFormat& vf = vb.getFormat();
		uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(nvidia::RenderVertexSemantic::POSITION));
		PxVec3* positions = (PxVec3*)vb.getBufferAndFormat(format, bufferIndex);
		if (positions == NULL)
		{
			continue;
		}
		if (format != RenderDataFormat::FLOAT3)
		{
			continue;
		}

		for (int p = 0; p < mParams->partBounds.arraySizes[0]; p++)
		{
			const uint32_t start = part[p];
			const uint32_t end = part[p + 1];
			for (uint32_t v = start; v < end; v++)
			{
				mParams->partBounds.buf[p].include(positions[v]);
			}
		}
	}
}

void ApexRenderMeshAsset::setSubmeshCount(uint32_t submeshCount)
{
	const uint32_t oldSize = mSubmeshes.size();

	for (uint32_t i = oldSize; i-- > submeshCount;)
	{
		PX_DELETE(mSubmeshes[i]);
	}

	mSubmeshes.resize(submeshCount);

	for (uint32_t i = oldSize; i < submeshCount; ++i)
	{
		mSubmeshes[i] = PX_NEW(ApexRenderSubmesh);
	}
}


}
} // end namespace nvidia::apex
