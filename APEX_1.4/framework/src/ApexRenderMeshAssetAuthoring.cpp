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


#include "PsArray.h"
#include "ApexRenderMeshAssetAuthoring.h"
#include "ApexRenderMeshActor.h"
#include "ApexSharedUtils.h"
#include "ApexCustomBufferIterator.h"
#include "ApexUsingNamespace.h"
#include "ApexSDKIntl.h"
#include "ResourceProviderIntl.h"

#include "PsSort.h"

#ifndef WITHOUT_APEX_AUTHORING

namespace nvidia
{
namespace apex
{


PX_INLINE bool PxVec3equals(const PxVec3& a, const PxVec3& v, float epsilon)
{
	return
		PxEquals(a.x, v.x, epsilon) &&
		PxEquals(a.y, v.y, epsilon) &&
		PxEquals(a.z, v.z, epsilon);
}

ApexRenderMeshAssetAuthoring::ApexRenderMeshAssetAuthoring(ResourceList& list, RenderMeshAssetParameters* params, const char* name)
{
	list.add(*this);

	createFromParameters(params);

	mName = name;
}

ApexRenderMeshAssetAuthoring::ApexRenderMeshAssetAuthoring(ResourceList& list)
{
	list.add(*this);
}

ApexRenderMeshAssetAuthoring::~ApexRenderMeshAssetAuthoring()
{
}

// We will create our vertex map here.  Remapping will be from sorting by part index


void ApexRenderMeshAssetAuthoring::createRenderMesh(const MeshDesc& meshDesc, bool createMappingInformation)
{
	if (!meshDesc.isValid())
	{
		APEX_INVALID_OPERATION("MeshDesc is not valid!");
		return;
	}

	if (mParams != NULL)
	{
		mParams->destroy();
	}

	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
	mParams = (RenderMeshAssetParameters*)traits->createNvParameterized(RenderMeshAssetParameters::staticClassName());
	NvParameterized::Handle rootHandle(*mParams);

	// Submeshes
	mParams->getParameterHandle("materialNames", rootHandle);
	rootHandle.resizeArray((int32_t)meshDesc.m_numSubmeshes);
	mParams->getParameterHandle("submeshes", rootHandle);
	rootHandle.resizeArray((int32_t)meshDesc.m_numSubmeshes);
	setSubmeshCount(0);
	setSubmeshCount(meshDesc.m_numSubmeshes);
	for (uint32_t submeshNum = 0; submeshNum < meshDesc.m_numSubmeshes; ++submeshNum)
	{
		ApexRenderSubmesh& submesh = *mSubmeshes[submeshNum];
		SubmeshParameters* submeshParams = (SubmeshParameters*)traits->createNvParameterized(SubmeshParameters::staticClassName());
		submesh.createFromParameters(submeshParams);
		mParams->submeshes.buf[submeshNum] = submeshParams;
		//NvParameterized::Handle submeshHandle( submeshParams );
		const SubmeshDesc& submeshDesc = meshDesc.m_submeshes[submeshNum];

		// Material name
		NvParameterized::Handle handle(*mParams);
		mParams->getParameterHandle("materialNames", handle);
		NvParameterized::Handle elementHandle(*mParams);
		handle.getChildHandle((int32_t)submeshNum, elementHandle);
		mParams->setParamString(elementHandle, submeshDesc.m_materialName);

		// Index buffer

		physx::Array<VertexPart> submeshMap;
		submeshMap.resize(submeshDesc.m_numVertices);
		const uint32_t invalidPart = PxMax(1u, submeshDesc.m_numParts);
		for (uint32_t i = 0; i < submeshDesc.m_numVertices; ++i)
		{
			submeshMap[i].part = invalidPart;
			submeshMap[i].vertexIndex = i;
		}
		bool success = false;
		switch (submeshDesc.m_indexType)
		{
		case IndexType::UINT:
			success = fillSubmeshMap<uint32_t>(submeshMap, submeshDesc.m_partIndices, submeshDesc.m_numParts, submeshDesc.m_vertexIndices, submeshDesc.m_numIndices, submeshDesc.m_numVertices);
			break;
		case IndexType::USHORT:
			success = fillSubmeshMap<uint16_t>(submeshMap, submeshDesc.m_partIndices, submeshDesc.m_numParts, submeshDesc.m_vertexIndices, submeshDesc.m_numIndices, submeshDesc.m_numVertices);
			break;
		default:
			PX_ALWAYS_ASSERT();
		}

		// error message?
		if (!success)
		{
			return;
		}

		if (submeshMap.size() > 1)
		{
			shdfnd::sort(submeshMap.begin(), submeshMap.size(), VertexPart());
		}

		uint32_t vertexCount = 0;
		for (; vertexCount < submeshDesc.m_numVertices; ++vertexCount)
		{
			if (submeshMap[vertexCount].part == invalidPart)
			{
				break;
			}
		}

		// Create inverse map for our internal remapping
		Array<int32_t> invMap;	// maps old indices to new indices
		invMap.resize(submeshDesc.m_numVertices);
		for (uint32_t i = 0; i < submeshDesc.m_numVertices; ++i)
		{
			const uint32_t vIndex = submeshMap[i].vertexIndex;
			if (i >= vertexCount)
			{
				invMap[vIndex] = -1;
			}
			else
			{
				invMap[vIndex] = (int32_t)i;
			}
		}

		// Copy index buffer (remapping)
		NvParameterized::Handle ibHandle(submeshParams);
		submeshParams->getParameterHandle("indexBuffer", ibHandle);
		ibHandle.resizeArray((int32_t)submeshDesc.m_numIndices);
		switch (submeshDesc.m_indexType)
		{
		case IndexType::UINT:
			for (uint32_t i = 0; i < submeshDesc.m_numIndices; ++i)
			{
				const uint32_t index = submeshDesc.m_vertexIndices != NULL ? ((uint32_t*)submeshDesc.m_vertexIndices)[i] : i;
				submeshParams->indexBuffer.buf[i] = (uint32_t)invMap[index];
				PX_ASSERT(submeshParams->indexBuffer.buf[i] != (uint32_t)-1);
			}
			break;
		case IndexType::USHORT:
			for (uint32_t i = 0; i < submeshDesc.m_numIndices; ++i)
			{
				const uint16_t index = submeshDesc.m_vertexIndices != NULL ? ((uint16_t*)submeshDesc.m_vertexIndices)[i] : (uint16_t)i;
				submeshParams->indexBuffer.buf[i] = (uint32_t)invMap[index];
				PX_ASSERT(submeshParams->indexBuffer.buf[i] != (uint32_t)-1);
			}
			break;
		default:
			PX_ALWAYS_ASSERT();
		}

		// Smoothing groups
		int32_t smoothingGroupArraySize = 0;
		if (submeshDesc.m_smoothingGroups != NULL)
		{
			switch (submeshDesc.m_primitive)
			{
			case Primitive::TRIANGLE_LIST:
				smoothingGroupArraySize = (int32_t)submeshDesc.m_numIndices/3;
				break;
			default:
				PX_ALWAYS_ASSERT();	// We only have one kind of primitive
			}
		}
		if (smoothingGroupArraySize != 0)
		{
			NvParameterized::Handle sgHandle(submeshParams);
			submeshParams->getParameterHandle("smoothingGroups", sgHandle);
			sgHandle.resizeArray(smoothingGroupArraySize);
			sgHandle.setParamU32Array(submeshDesc.m_smoothingGroups, smoothingGroupArraySize, 0);
		}

		// Index partition
		NvParameterized::Handle ipHandle(submeshParams);
		submeshParams->getParameterHandle("indexPartition", ipHandle);
		ipHandle.resizeArray(PxMax((int32_t)submeshDesc.m_numParts + 1, 2));

		if (submeshDesc.m_numParts == 0)
		{
			submeshParams->indexPartition.buf[0] = 0;
			submeshParams->indexPartition.buf[1] = submeshDesc.m_numIndices;
		}
		else
		{
			switch (submeshDesc.m_indexType)
			{
			case IndexType::UINT:
				for (uint32_t i = 0; i < submeshDesc.m_numParts; ++i)
				{
					submeshParams->indexPartition.buf[i] = ((uint32_t*)submeshDesc.m_partIndices)[i];
				}
				submeshParams->indexPartition.buf[submeshDesc.m_numParts] = submeshDesc.m_numIndices;
				break;
			case IndexType::USHORT:
				for (uint32_t i = 0; i < submeshDesc.m_numParts; ++i)
				{
					submeshParams->indexPartition.buf[i] = (uint32_t)((uint16_t*)submeshDesc.m_partIndices)[i];
				}
				submeshParams->indexPartition.buf[submeshDesc.m_numParts] = submeshDesc.m_numIndices;
				break;
			default:
				PX_ALWAYS_ASSERT();
			}
		}

		// Vertex partition
		Array<uint32_t> lookup;
		createIndexStartLookup(lookup, 0, submeshDesc.m_numParts, (int32_t*)submeshMap.begin(), vertexCount, sizeof(VertexPart));
		NvParameterized::Handle vpHandle(submeshParams);
		submeshParams->getParameterHandle("vertexPartition", vpHandle);
		vpHandle.resizeArray((int32_t)lookup.size());
		vpHandle.setParamU32Array(lookup.begin(), (int32_t)lookup.size());

		// Vertex buffer

		// Create format description
		ApexVertexFormat format;

		for (uint32_t i = 0; i < submeshDesc.m_numVertexBuffers; ++i)
		{
			const VertexBuffer& vb = submeshDesc.m_vertexBuffers[i];
			for (uint32_t semantic = 0; semantic < RenderVertexSemantic::NUM_SEMANTICS; ++semantic)
			{
				RenderVertexSemantic::Enum vertexSemantic = (RenderVertexSemantic::Enum)semantic;
				RenderDataFormat::Enum vertexFormat = vb.getSemanticData(vertexSemantic).format;

				if (vertexSemanticFormatValid(vertexSemantic, vertexFormat))
				{
					int32_t bufferIndex = format.addBuffer(format.getSemanticName(vertexSemantic));
					format.setBufferFormat((uint32_t)bufferIndex, vb.getSemanticData(vertexSemantic).format);
				}
				else if (vertexFormat != RenderDataFormat::UNSPECIFIED)
				{
					APEX_INVALID_PARAMETER("Format (%d) is not valid for Semantic (%s)", vertexFormat, format.getSemanticName(vertexSemantic));
				}
			}
		}

		format.setWinding(submeshDesc.m_cullMode);

		// Include custom buffers
		for (uint32_t i = 0; i < submeshDesc.m_numVertexBuffers; ++i)
		{
			const VertexBuffer& vb = submeshDesc.m_vertexBuffers[i];
			for (uint32_t index = 0; index < vb.getNumCustomSemantics(); ++index)
			{
				const RenderSemanticData& data = vb.getCustomSemanticData(index);
				// BRG - reusing data.ident as the custom channel name.  What to do with the serialize parameter?
				int32_t bufferIndex = format.addBuffer((char*)data.ident);
				format.setBufferFormat((uint32_t)bufferIndex, data.format);

				// PH: custom buffers are never serialized this way, we might need to change this!
				format.setBufferSerialize((uint32_t)bufferIndex, data.serialize);
			}
		}

		if (createMappingInformation)
		{
			int32_t bufferIndex = format.addBuffer("VERTEX_ORIGINAL_INDEX");
			format.setBufferFormat((uint32_t)bufferIndex, RenderDataFormat::UINT1);
		}

		// Create apex vertex buffer
		submesh.buildVertexBuffer(format, vertexCount);

		// Now fill in...
		for (uint32_t i = 0; i < submeshDesc.m_numVertexBuffers; ++i)
		{
			const VertexBuffer& vb = submeshDesc.m_vertexBuffers[i];
			const VertexFormat& vf = submesh.getVertexBuffer().getFormat();

			RenderSemanticData boneWeightData;
			RenderSemanticData boneIndexData;
			RenderDataFormat::Enum checkFormatBoneWeight = RenderDataFormat::UNSPECIFIED;
			RenderDataFormat::Enum checkFormatBoneIndex = RenderDataFormat::UNSPECIFIED;
			RenderDataFormat::Enum dstFormatBoneWeight = RenderDataFormat::UNSPECIFIED;
			RenderDataFormat::Enum dstFormatBoneIndex = RenderDataFormat::UNSPECIFIED;
			void* dstDataWeight = NULL;
			void* dstDataIndex = NULL;
			uint32_t numBoneWeights = 0;
			uint32_t numBoneIndices = 0;

			for (uint32_t semantic = 0; semantic < RenderVertexSemantic::NUM_SEMANTICS; ++semantic)
			{
				if (vertexSemanticFormatValid((RenderVertexSemantic::Enum)semantic, vb.getSemanticData((RenderVertexSemantic::Enum)semantic).format))
				{
					RenderDataFormat::Enum dstFormat;
					void* dst = submesh.getVertexBufferWritable().getBufferAndFormatWritable(dstFormat, (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID((RenderVertexSemantic::Enum)semantic)));
					const RenderSemanticData& data = vb.getSemanticData((RenderVertexSemantic::Enum)semantic);

					copyRenderVertexBuffer(dst, dstFormat, 0, 0, data.data, data.srcFormat, data.stride, 0, submeshDesc.m_numVertices, invMap.begin());

					if (semantic == RenderVertexSemantic::BONE_WEIGHT)
					{
						boneWeightData = data;
						// Verification code for bone weights.
						switch (data.srcFormat)
						{
						case RenderDataFormat::FLOAT1:
							checkFormatBoneWeight = RenderDataFormat::FLOAT1;
							numBoneWeights = 1;
							break;
						case RenderDataFormat::FLOAT2:
							checkFormatBoneWeight = RenderDataFormat::FLOAT2;
							numBoneWeights = 2;
							break;
						case RenderDataFormat::FLOAT3:
							checkFormatBoneWeight = RenderDataFormat::FLOAT3;
							numBoneWeights = 3;
							break;
						case RenderDataFormat::FLOAT4:
							checkFormatBoneWeight = RenderDataFormat::FLOAT4;
							numBoneWeights = 4;
							break;
						default:
							break;
						}

						dstDataWeight = dst;
						dstFormatBoneWeight = dstFormat;
					}
					else if (semantic == RenderVertexSemantic::BONE_INDEX)
					{
						boneIndexData = data;
						switch (data.srcFormat)
						{
						case RenderDataFormat::USHORT1:
							checkFormatBoneIndex = RenderDataFormat::USHORT1;
							numBoneIndices = 1;
							break;
						case RenderDataFormat::USHORT2:
							checkFormatBoneIndex = RenderDataFormat::USHORT2;
							numBoneIndices = 2;
							break;
						case RenderDataFormat::USHORT3:
							checkFormatBoneIndex = RenderDataFormat::USHORT3;
							numBoneIndices = 3;
							break;
						case RenderDataFormat::USHORT4:
							checkFormatBoneIndex = RenderDataFormat::USHORT4;
							numBoneIndices = 4;
							break;
						default:
							break;
						}
						dstDataIndex = dst;
						dstFormatBoneIndex = dstFormat;
					}
				}
			}

			// some verification code
			if (numBoneIndices > 1 && numBoneWeights == numBoneIndices)
			{
				float verifyWeights[4] = { 0.0f };
				uint16_t verifyIndices[4] = { 0 };
				for (uint32_t vi = 0; vi < submeshDesc.m_numVertices; vi++)
				{
					const int32_t dest = invMap[vi];
					if (dest >= 0)
					{

						copyRenderVertexBuffer(verifyWeights, checkFormatBoneWeight, 0, 0, boneWeightData.data, boneWeightData.srcFormat, boneWeightData.stride, vi, 1);
						copyRenderVertexBuffer(verifyIndices, checkFormatBoneIndex, 0, 0, boneIndexData.data, boneIndexData.srcFormat, boneIndexData.stride, vi, 1);

						float sum = 0.0f;
						for (uint32_t j = 0; j < numBoneWeights; j++)
						{
							sum += verifyWeights[j];
						}

						if (PxAbs(1 - sum) > 0.001)
						{
							if (sum > 0.0f)
							{
								for (uint32_t j = 0; j < numBoneWeights; j++)
								{
									verifyWeights[j] /= sum;
								}
							}

							APEX_INVALID_PARAMETER("Submesh %d Vertex %d has been normalized, bone weight was (%f)", i, vi, sum);
						}
						// PH: bubble sort, don't kill me for this
						for (uint32_t j = 1; j < numBoneWeights; j++)
						{
							for (uint32_t k = 1; k < numBoneWeights; k++)
							{
								if (verifyWeights[k - 1] < verifyWeights[k])
								{
									nvidia::swap(verifyWeights[k - 1], verifyWeights[k]);
									nvidia::swap(verifyIndices[k - 1], verifyIndices[k]);
								}
							}
						}

						for (uint32_t j = 0; j < numBoneWeights; j++)
						{
							if (verifyWeights[j] == 0.0f)
							{
								verifyIndices[j] = 0;
							}
						}

						copyRenderVertexBuffer(dstDataWeight, dstFormatBoneWeight, 0, (uint32_t)dest, verifyWeights, checkFormatBoneWeight, 0, 0, 1);
						copyRenderVertexBuffer(dstDataIndex, dstFormatBoneIndex, 0, (uint32_t)dest, verifyIndices, checkFormatBoneIndex, 0, 0, 1);
					}
				}
			}

			// Custom buffers
			for (uint32_t index = 0; index < vb.getNumCustomSemantics(); ++index)
			{
				const RenderSemanticData& data = vb.getCustomSemanticData(index);
				const int32_t bufferIndex = format.getBufferIndexFromID(format.getID((char*)data.ident));
				PX_ASSERT(bufferIndex >= 0);
				void* dst = const_cast<void*>(submesh.getVertexBuffer().getBuffer((uint32_t)bufferIndex));
				RenderDataFormat::Enum srcFormat = data.srcFormat != RenderDataFormat::UNSPECIFIED ? data.srcFormat : data.format;
				copyRenderVertexBuffer(dst, data.format, 0, 0, data.data, srcFormat, data.stride, 0, submeshDesc.m_numVertices, invMap.begin());
			}
		}

		if (createMappingInformation)
		{
			const VertexFormat::BufferID bufferID = format.getID("VERTEX_ORIGINAL_INDEX");
			const int32_t bufferIndex = format.getBufferIndexFromID(bufferID);
			RenderDataFormat::Enum bufferFormat = format.getBufferFormat((uint32_t)bufferIndex);
			PX_ASSERT(bufferIndex >= 0);
			const void* dst = submesh.getVertexBuffer().getBuffer((uint32_t)bufferIndex);
			copyRenderVertexBuffer(const_cast<void*>(dst), bufferFormat, 0, 0, &submeshMap[0].vertexIndex , RenderDataFormat::UINT1 , sizeof(VertexPart), 0, vertexCount, NULL);
		}
	}

	// Part bounds
	uint32_t partCount = 1;
	for (uint32_t submeshNum = 0; submeshNum < meshDesc.m_numSubmeshes; ++submeshNum)
	{
		partCount = PxMax(partCount, meshDesc.m_submeshes[submeshNum].m_numParts);
	}
	mParams->getParameterHandle("partBounds", rootHandle);
	mParams->resizeArray(rootHandle, (int32_t)partCount);
	for (uint32_t partNum = 0; partNum < partCount; ++partNum)
	{
		mParams->partBounds.buf[partNum].setEmpty();
		// Add part vertices
		for (uint32_t submeshNum = 0; submeshNum < meshDesc.m_numSubmeshes; ++submeshNum)
		{
			SubmeshParameters* submeshParams = DYNAMIC_CAST(SubmeshParameters*)(mParams->submeshes.buf[submeshNum]);
			ApexRenderSubmesh& submesh = *mSubmeshes[submeshNum];
			RenderDataFormat::Enum positionFormat;
			const VertexFormat& vf = submesh.getVertexBuffer().getFormat();
			const PxVec3* positions = (const PxVec3*)submesh.getVertexBuffer().getBufferAndFormat(positionFormat, 
				(uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::POSITION)));
			if (positions && positionFormat == RenderDataFormat::FLOAT3)
			{
				for (uint32_t vertexIndex = submeshParams->vertexPartition.buf[partNum]; vertexIndex < submeshParams->vertexPartition.buf[partNum + 1]; ++vertexIndex)
				{
					mParams->partBounds.buf[partNum].include(positions[vertexIndex]);
				}
			}
		}
	}

	mParams->textureUVOrigin = meshDesc.m_uvOrigin;

	createLocalData();
}

uint32_t ApexRenderMeshAssetAuthoring::createReductionMap(uint32_t* map, const Vertex* vertices, const uint32_t* smoothingGroups, uint32_t vertexCount,
        const PxVec3& positionTolerance, float normalTolerance, float UVTolerance)
{
	physx::Array<BoundsRep> vertexNeighborhoods;
	vertexNeighborhoods.resize(vertexCount);
	const PxVec3 neighborhoodExtent = 0.5f * positionTolerance;
	for (uint32_t vertexNum = 0; vertexNum < vertexCount; ++vertexNum)
	{
		vertexNeighborhoods[vertexNum].aabb = PxBounds3(vertices[vertexNum].position - neighborhoodExtent, vertices[vertexNum].position + neighborhoodExtent);
	}

	physx::Array<IntPair> vertexNeighbors;
	if (vertexNeighborhoods.size() > 0)
	{
		boundsCalculateOverlaps(vertexNeighbors, Bounds3XYZ, &vertexNeighborhoods[0], vertexNeighborhoods.size(), sizeof(vertexNeighborhoods[0]));
	}

	for (uint32_t i = 0; i < vertexCount; ++i)
	{
		map[i] = i;
	}

	for (uint32_t pairNum = 0; pairNum < vertexNeighbors.size(); ++pairNum)
	{
		const IntPair& pair = vertexNeighbors[pairNum];
		const uint32_t map0 = map[pair.i0];
		const uint32_t map1 = map[pair.i1];
		if (smoothingGroups != NULL && smoothingGroups[map0] != smoothingGroups[map1])
		{
			continue;
		}
		const Vertex& vertex0 = vertices[map0];
		const Vertex& vertex1 = vertices[map1];
		if (PxAbs(vertex0.position.x - vertex1.position.x) > positionTolerance.x ||
		        PxAbs(vertex0.position.y - vertex1.position.y) > positionTolerance.y ||
		        PxAbs(vertex0.position.z - vertex1.position.z) > positionTolerance.z)
		{
			continue;
		}
		if (!PxVec3equals(vertex0.normal, vertex1.normal, normalTolerance) ||
		        !PxVec3equals(vertex0.tangent, vertex1.tangent, normalTolerance) ||
		        !PxVec3equals(vertex0.binormal, vertex1.binormal, normalTolerance))
		{
			continue;
		}
		uint32_t uvNum = 0;
		for (; uvNum < VertexFormat::MAX_UV_COUNT; ++uvNum)
		{
			const VertexUV& uv0 = vertex0.uv[uvNum];
			const VertexUV& uv1 = vertex1.uv[uvNum];
			if (PxAbs(uv0[0] - uv1[0]) > UVTolerance || PxAbs(uv0[1] - uv1[1]) > UVTolerance)
			{
				break;
			}
		}
		if (uvNum < VertexFormat::MAX_UV_COUNT)
		{
			continue;
		}
		map[pair.i1] = map0;
	}

	physx::Array<int32_t> offsets(vertexCount, -1);
	for (uint32_t i = 0; i < vertexCount; ++i)
	{
		offsets[map[i]] = 0;
	}
	int32_t delta = 0;
	for (uint32_t i = 0; i < vertexCount; ++i)
	{
		delta += offsets[i];
		offsets[i] = delta;
	}
	for (uint32_t i = 0; i < vertexCount; ++i)
	{
		map[i] += offsets[map[i]];
	}
	return vertexCount + delta;
}



void ApexRenderMeshAssetAuthoring::setMaterialName(uint32_t submeshIndex, const char* name)
{
	size_t maxMaterials = (uint32_t)mParams->materialNames.arraySizes[0];
	PX_ASSERT(submeshIndex < maxMaterials);
	if (submeshIndex < maxMaterials)
	{
		NvParameterized::Handle handle(*mParams);
		mParams->getParameterHandle("materialNames", handle);
		NvParameterized::Handle elementHandle(*mParams);
		handle.getChildHandle((int32_t)submeshIndex, elementHandle);
		mParams->setParamString(elementHandle, name ? name : "");
	}
}

void						ApexRenderMeshAssetAuthoring::setWindingOrder(uint32_t submeshIndex, RenderCullMode::Enum winding)
{
	ApexRenderSubmesh& subMesh = *ApexRenderMeshAsset::mSubmeshes[submeshIndex];
	VertexBufferIntl& vb = subMesh.getVertexBufferWritable();
	vb.getFormatWritable().setWinding(winding);
}

RenderCullMode::Enum		ApexRenderMeshAssetAuthoring::getWindingOrder(uint32_t submeshIndex) const
{
	const RenderSubmesh& subMesh = getSubmesh(submeshIndex);
	const nvidia::apex::VertexBuffer& vb = subMesh.getVertexBuffer();
	const VertexFormat& format = vb.getFormat();
	return format.getWinding();
}



template <typename PxU>
bool ApexRenderMeshAssetAuthoring::fillSubmeshMap(physx::Array<VertexPart>& submeshMap, const void* const partIndicesVoid,
        uint32_t numParts, const void* const vertexIndicesVoid,
        uint32_t numSubmeshIndices, uint32_t numSubmeshVertices)
{
	PxU partIndexStart = 0;
	if (numParts == 0)
	{
		numParts = 1;
	}

	const PxU* const partIndices = partIndicesVoid != NULL ? reinterpret_cast<const PxU * const>(partIndicesVoid) : &partIndexStart;
	const PxU* const vertexIndices = reinterpret_cast<const PxU * const>(vertexIndicesVoid);

	for (uint32_t i = 0; i < numParts; ++i)
	{
		const uint32_t stop = i + 1 < numParts ? partIndices[i + 1] : numSubmeshIndices;
		for (uint32_t j = partIndices[i]; j < stop; ++j)
		{
			const uint32_t vertexIndex = vertexIndices != NULL ? vertexIndices[j] : j;
			if (vertexIndex >= numSubmeshVertices)
			{
				return false;	// to do: issue error - index out of range
			}
			if (submeshMap[vertexIndex].part != numParts && submeshMap[vertexIndex].part != i)
			{
				return false;	// to do: issue error - vertex in more than one part
			}
			submeshMap[vertexIndex].part = i;
		}
	}
	return true;
}

}
} // end namespace nvidia::apex
#endif
