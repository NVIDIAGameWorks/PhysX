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
#include <RenderMeshAsset.h>

#include "Mesh.h"

namespace nvidia
{
namespace fracture
{

void Mesh::gatherPartMesh(Array<PxVec3>& vertices,
					nvidia::Array<uint32_t>&  indices,
					nvidia::Array<PxVec3>& normals,
					nvidia::Array<PxVec2>& texcoords,
					nvidia::Array<SubMesh>& subMeshes,
					const RenderMeshAsset& renderMeshAsset,
					uint32_t partIndex)
					
{
	if (partIndex >= renderMeshAsset.getPartCount())
	{
		vertices.resize(0);
		indices.resize(0);
		normals.resize(0);
		texcoords.resize(0);
		subMeshes.resize(0);
		return;
	}

	subMeshes.resize(renderMeshAsset.getSubmeshCount());

	// Pre-count vertices and indices so we can allocate once
	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;
	for (uint32_t submeshIndex = 0; submeshIndex < renderMeshAsset.getSubmeshCount(); ++submeshIndex)
	{
		const RenderSubmesh& submesh = renderMeshAsset.getSubmesh(submeshIndex);
		vertexCount += submesh.getVertexCount(partIndex);
		indexCount += submesh.getIndexCount(partIndex);
	}

	vertices.resize(vertexCount);
	normals.resize(vertexCount);
	texcoords.resize(vertexCount);
	indices.resize(indexCount);

	vertexCount = 0;
	indexCount = 0;
	for (uint32_t submeshIndex = 0; submeshIndex < renderMeshAsset.getSubmeshCount(); ++submeshIndex)
	{
		const RenderSubmesh& submesh = renderMeshAsset.getSubmesh(submeshIndex);
		const uint32_t submeshVertexCount = submesh.getVertexCount(partIndex);
		if (submeshVertexCount > 0)
		{
			const VertexBuffer& vertexBuffer = submesh.getVertexBuffer();
			const VertexFormat& vertexFormat = vertexBuffer.getFormat();

			enum { MESH_SEMANTIC_COUNT = 3 };
			struct { 
				RenderVertexSemantic::Enum semantic; 
				RenderDataFormat::Enum format; 
				uint32_t sizeInBytes;
				void* dstBuffer;
			} semanticData[MESH_SEMANTIC_COUNT] = {
				{ RenderVertexSemantic::POSITION,  RenderDataFormat::FLOAT3, sizeof(PxVec3), &vertices[vertexCount]  },
				{ RenderVertexSemantic::NORMAL,    RenderDataFormat::FLOAT3, sizeof(PxVec3), &normals[vertexCount]   },
				{ RenderVertexSemantic::TEXCOORD0, RenderDataFormat::FLOAT2, sizeof(PxVec2), &texcoords[vertexCount] } 
			};

			for (uint32_t i = 0; i < MESH_SEMANTIC_COUNT; ++i)
			{
				const int32_t bufferIndex = vertexFormat.getBufferIndexFromID(vertexFormat.getSemanticID(semanticData[i].semantic));
				if (bufferIndex >= 0)
					vertexBuffer.getBufferData(semanticData[i].dstBuffer, 
											   semanticData[i].format, 
											   semanticData[i].sizeInBytes, 
											   (uint32_t)bufferIndex, 
											   submesh.getFirstVertexIndex(partIndex), 
											   submesh.getVertexCount(partIndex));
				else
					memset(semanticData[i].dstBuffer, 0, submesh.getVertexCount(partIndex)*semanticData[i].sizeInBytes);
			}

			/*
			const uint32_t firstVertexIndex       = submesh.getFirstVertexIndex(partIndex);
			fillBuffer<PxVec3>(vertexBuffer, vertexFormat, RenderVertexSemantic::POSITION, RenderDataFormat::FLOAT3,
			                   firstVertexIndex, submeshVertexCount, &vertices[vertexCount]);
			fillBuffer<PxVec3>(vertexBuffer, vertexFormat, RenderVertexSemantic::NORMAL, RenderDataFormat::FLOAT3,
			                   firstVertexIndex, submeshVertexCount, &normals[vertexCount]);
			fillBuffer<PxVec2>(vertexBuffer, vertexFormat, RenderVertexSemantic::TEXCOORD0, RenderDataFormat::FLOAT2,
			                   firstVertexIndex, submeshVertexCount, &texcoords[vertexCount]);*/

			const uint32_t* partIndexBuffer = submesh.getIndexBuffer(partIndex);
			const uint32_t partIndexCount = submesh.getIndexCount(partIndex);
			subMeshes[submeshIndex].firstIndex = (int32_t)partIndexCount;
			for (uint32_t indexNum = 0; indexNum < partIndexCount; ++indexNum)
			{
				indices[indexCount++] = partIndexBuffer[indexNum] + vertexCount - submesh.getFirstVertexIndex(partIndex);
			}
			vertexCount += submeshVertexCount;
		}
	}
}


void Mesh::loadFromRenderMesh(const RenderMeshAsset& mesh, uint32_t partIndex)
{
	gatherPartMesh(mVertices, mIndices, mNormals, mTexCoords, mSubMeshes, mesh, partIndex);
}

}
}
#endif