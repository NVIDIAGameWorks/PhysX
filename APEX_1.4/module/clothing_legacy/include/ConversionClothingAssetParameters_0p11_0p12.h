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



#ifndef MODULE_CONVERSIONCLOTHINGASSETPARAMETERS_0P11_0P12H_H
#define MODULE_CONVERSIONCLOTHINGASSETPARAMETERS_0P11_0P12H_H

#include "NvParamConversionTemplate.h"
#include "ClothingAssetParameters_0p11.h"
#include "ClothingAssetParameters_0p12.h"

#include "ClothingGraphicalLodParameters_0p4.h"
#include "ClothingPhysicalMeshParameters_0p10.h"
#include "RenderMeshAssetParameters.h"
#include "SubmeshParameters.h"
#include "VertexBufferParameters.h"
#include "VertexFormatParameters.h"
#include "BufferF32x3.h"
#include "BufferF32x4.h"
#include "ModuleClothingHelpers.h"

namespace nvidia {
namespace apex {
namespace legacy {


typedef NvParameterized::ParamConversionTemplate<nvidia::parameterized::ClothingAssetParameters_0p11, 
						nvidia::parameterized::ClothingAssetParameters_0p12, 
						nvidia::parameterized::ClothingAssetParameters_0p11::ClassVersion, 
						nvidia::parameterized::ClothingAssetParameters_0p12::ClassVersion>
						ConversionClothingAssetParameters_0p11_0p12Parent;

class ConversionClothingAssetParameters_0p11_0p12: public ConversionClothingAssetParameters_0p11_0p12Parent
{
public:
	static NvParameterized::Conversion* Create(NvParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionClothingAssetParameters_0p11_0p12));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionClothingAssetParameters_0p11_0p12)(t) : 0;
	}

protected:
	ConversionClothingAssetParameters_0p11_0p12(NvParameterized::Traits* t) : ConversionClothingAssetParameters_0p11_0p12Parent(t) {}

	const NvParameterized::PrefVer* getPreferredVersions() const
	{
		static NvParameterized::PrefVer prefVers[] =
		{
			//TODO:
			//	Add your preferred versions for included references here.
			//	Entry format is
			//		{ (const char*)longName, (uint32_t)preferredVersion }
			{ "graphicalLods[]", 4 },
			{ "physicalMeshes[]", 10 },
			{ 0, 0 } // Terminator (do not remove!)
		};

		return prefVers;
	}

bool convert()
	{
		//TODO:
		//	Write custom conversion code here using mNewData and mLegacyData members.
		//
		//	Note that
		//		- mNewData has already been initialized with default values
		//		- same-named/same-typed members have already been copied
		//			from mLegacyData to mNewData
		//		- included references were moved to mNewData
		//			(and updated to preferred versions according to getPreferredVersions)
		//
		//	For more info see the versioning wiki.

		// look up the vertex indices for the skinClothMap update
		for (int32_t i = 0; i < mNewData->graphicalLods.arraySizes[0]; ++i)
		{
			parameterized::ClothingGraphicalLodParameters_0p4* graphicalLod = (parameterized::ClothingGraphicalLodParameters_0p4*)mNewData->graphicalLods.buf[i];

			uint32_t physicalMeshIndex = graphicalLod->physicalMeshId;
			float meshThickness = graphicalLod->skinClothMapOffset;

			// use already updated physical mesh for lookup
			PX_ASSERT(physicalMeshIndex < (uint32_t)mNewData->physicalMeshes.arraySizes[0]);
			parameterized::ClothingPhysicalMeshParameters_0p10NS::PhysicalMesh_Type& physicalMesh = ((parameterized::ClothingPhysicalMeshParameters_0p10*)mNewData->physicalMeshes.buf[physicalMeshIndex])->physicalMesh;

			RenderMeshAssetParameters* rma = DYNAMIC_CAST(RenderMeshAssetParameters*)(graphicalLod->renderMeshAsset);

			int32_t numSubmeshes = rma->submeshes.arraySizes[0];
			Array<physx::PxVec3*> submeshPositionBuffer((uint32_t)numSubmeshes, NULL);
			Array<PxVec4*> submeshTangentBuffer((uint32_t)numSubmeshes, NULL);
			Array<uint32_t> submeshVertexOffsets((uint32_t)numSubmeshes);
			uint32_t submeshVertexOffset = 0;
			for (int32_t submeshIdx = 0; submeshIdx < numSubmeshes; ++submeshIdx)
			{
				SubmeshParameters* submesh = DYNAMIC_CAST(SubmeshParameters*)(rma->submeshes.buf[submeshIdx]);

				VertexBufferParameters* vb = DYNAMIC_CAST(VertexBufferParameters*)(submesh->vertexBuffer);
				VertexFormatParameters* vertexFormat = DYNAMIC_CAST(VertexFormatParameters*)(vb->vertexFormat);

				uint32_t numBuffers = (uint32_t)vb->buffers.arraySizes[0];
				for (uint32_t bufferIdx = 0; bufferIdx < numBuffers; ++bufferIdx)
				{
					VertexFormatParametersNS::BufferFormat_Type bufferFormat = vertexFormat->bufferFormats.buf[bufferIdx];

					if (bufferFormat.semantic == RenderVertexSemantic::POSITION)
					{
						if (bufferFormat.format == RenderDataFormat::FLOAT3)
						{
							BufferF32x3* posBuffer = DYNAMIC_CAST(BufferF32x3*)(vb->buffers.buf[bufferIdx]);
							submeshPositionBuffer[(uint32_t)submeshIdx] = posBuffer->data.buf;
						}
					}

					if (bufferFormat.semantic == RenderVertexSemantic::TANGENT)
					{
						if (bufferFormat.format == RenderDataFormat::FLOAT4)
						{
							BufferF32x4* tangentBuffer = DYNAMIC_CAST(BufferF32x4*)(vb->buffers.buf[bufferIdx]);
							submeshTangentBuffer[(uint32_t)submeshIdx] = (physx::PxVec4*)tangentBuffer->data.buf;
						}
					}
				}

				submeshVertexOffsets[(uint32_t)submeshIdx] = submeshVertexOffset;
				submeshVertexOffset += vb->vertexCount;
			}

			PxVec3* normals = physicalMesh.skinningNormals.buf;
			if (normals == NULL)
			{
				normals = physicalMesh.normals.buf;
			}
			for (int32_t j = 0; j < graphicalLod->skinClothMap.arraySizes[0]; ++j)
			{
				parameterized::ClothingGraphicalLodParameters_0p4NS::SkinClothMapD_Type& mapD = graphicalLod->skinClothMap.buf[j];

				const uint32_t faceIndex = mapD.vertexIndex0; // this was temporarily stored in the GraphicalLod update
				mapD.vertexIndex0 = physicalMesh.indices.buf[faceIndex + 0];
				mapD.vertexIndex1 = physicalMesh.indices.buf[faceIndex + 1];
				mapD.vertexIndex2 = physicalMesh.indices.buf[faceIndex + 2];

				clothing::TriangleWithNormals triangle;
				triangle.faceIndex0 = faceIndex;
				// store vertex information in triangle
				for (uint32_t k = 0; k < 3; k++)
				{
					uint32_t triVertIndex = physicalMesh.indices.buf[triangle.faceIndex0 + k];
					triangle.vertices[k] = physicalMesh.vertices.buf[triVertIndex];
					triangle.normals[k] = normals[triVertIndex];
				}
				triangle.init();

				// find index of the gaphics vertex in the submesh
				uint32_t graphVertIdx = 0;
				int32_t submeshIdx = 0;
				for (int32_t k = (int32_t)submeshVertexOffsets.size()-1; k >= 0; --k)
				{
					if (submeshVertexOffsets[(uint32_t)k] <= mapD.vertexIndexPlusOffset)
					{
						graphVertIdx = mapD.vertexIndexPlusOffset - submeshVertexOffsets[(uint32_t)k];
						submeshIdx = k;
						break;
					}
				}

				if (submeshPositionBuffer[(uint32_t)submeshIdx] != NULL && submeshTangentBuffer[(uint32_t)submeshIdx] != NULL)
				{
					PxVec3 graphicalPos = submeshPositionBuffer[(uint32_t)submeshIdx][graphVertIdx];
					PxVec3 graphicalTangent = submeshTangentBuffer[(uint32_t)submeshIdx][graphVertIdx].getXYZ();
					PxVec3 tangentRelative = graphicalTangent.isZero() ? physx::PxVec3(0) : graphicalPos + graphicalTangent;

					PxVec3 dummy(0.0f);
					clothing::ModuleClothingHelpers::computeTriangleBarys(triangle, dummy, dummy, tangentRelative, meshThickness, 0, true);

					mapD.tangentBary = triangle.tempBaryTangent;
					if (mapD.vertexBary == physx::PxVec3(PX_MAX_F32) || !mapD.vertexBary.isFinite())
					{
						APEX_DEBUG_WARNING("Barycentric coordinates for position is not valid. SubmeshIndex: %i, VertexIndex: %i. MapIndex: %i", submeshIdx, graphVertIdx, j);
					}
					if (mapD.normalBary == physx::PxVec3(PX_MAX_F32) || !mapD.normalBary.isFinite())
					{
						APEX_DEBUG_WARNING("Barycentric coordinates for normal is not valid. SubmeshIndex: %i, VertexIndex: %i. MapIndex: %i", submeshIdx, graphVertIdx, j);
					}
					if (mapD.tangentBary == physx::PxVec3(PX_MAX_F32) || !mapD.tangentBary.isFinite())
					{
						APEX_DEBUG_WARNING("Barycentric coordinates for tangent is not valid. SubmeshIndex: %i, VertexIndex: %i. MapIndex: %i", submeshIdx, graphVertIdx, j);
					}
				}
			}
		}


		// look up the vertex indices for the lod transition maps
		for (int32_t i = 0; i < mNewData->physicalMeshes.arraySizes[0]; ++i)
		{
			parameterized::ClothingPhysicalMeshParameters_0p10** physicalMeshes = (parameterized::ClothingPhysicalMeshParameters_0p10**)mNewData->physicalMeshes.buf;
			parameterized::ClothingPhysicalMeshParameters_0p10* currentMesh = (parameterized::ClothingPhysicalMeshParameters_0p10*)physicalMeshes[i];

			const parameterized::ClothingPhysicalMeshParameters_0p10* nextMesh = (i+1) < mNewData->physicalMeshes.arraySizes[0] ? (parameterized::ClothingPhysicalMeshParameters_0p10*)physicalMeshes[i+1] : NULL;
			for (int32_t j = 0; j < currentMesh->transitionUp.arraySizes[0]; ++j)
			{
				const uint32_t faceIndex0 = currentMesh->transitionUp.buf[j].vertexIndex0; // this was temporarily stored in the PhysicalMesh update
				PX_ASSERT(nextMesh != NULL);
				PX_ASSERT(faceIndex0 + 2 < (uint32_t)nextMesh->physicalMesh.indices.arraySizes[0]);
				currentMesh->transitionUp.buf[j].vertexIndex0 = nextMesh->physicalMesh.indices.buf[faceIndex0 + 0];
				currentMesh->transitionUp.buf[j].vertexIndex1 = nextMesh->physicalMesh.indices.buf[faceIndex0 + 1];
				currentMesh->transitionUp.buf[j].vertexIndex2 = nextMesh->physicalMesh.indices.buf[faceIndex0 + 2];
			}

			const parameterized::ClothingPhysicalMeshParameters_0p10* previousMesh = (i-1) >= 0 ? (parameterized::ClothingPhysicalMeshParameters_0p10*)physicalMeshes[i-1] : NULL;
			for (int32_t j = 0; j < currentMesh->transitionDown.arraySizes[0]; ++j)
			{
				const uint32_t faceIndex0 = currentMesh->transitionDown.buf[j].vertexIndex0; // this was temporarily stored in the PhysicalMesh update
				PX_ASSERT(previousMesh != NULL);
				PX_ASSERT(faceIndex0 + 2 < (uint32_t)previousMesh->physicalMesh.indices.arraySizes[0]);
				currentMesh->transitionDown.buf[j].vertexIndex0 = previousMesh->physicalMesh.indices.buf[faceIndex0 + 0];
				currentMesh->transitionDown.buf[j].vertexIndex1 = previousMesh->physicalMesh.indices.buf[faceIndex0 + 1];
				currentMesh->transitionDown.buf[j].vertexIndex2 = previousMesh->physicalMesh.indices.buf[faceIndex0 + 2];
			}
		}

		return true;
	}
};


}
}
} //nvidia::apex::legacy

#endif
