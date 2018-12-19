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


#include "SimdMath.h"
#include "PxPreprocessor.h"
#include "RenderDataFormat.h"
#include "ClothingAssetData.h"
#include "PsIntrinsics.h"
#include "PsVecMath.h"
#include "PxMat44.h"

#define NX_PARAMETERIZED_ONLY_LAYOUTS
#include "ClothingGraphicalLodParameters.h"

#pragma warning(disable : 4101 4127) // unreferenced local variable and conditional expression is constant


namespace nvidia
{
using namespace physx::shdfnd;
//using namespace physx::shdfnd::aos;

namespace clothing
{

ClothingAssetSubMesh::ClothingAssetSubMesh() :
	mPositions(NULL),
	mNormals(NULL),
	mTangents(NULL),
	mBoneWeights(NULL),
	mBoneIndices(NULL),
	mIndices(NULL),
	mUvs(NULL),

	mPositionOutFormat(RenderDataFormat::UNSPECIFIED),
	mNormalOutFormat(RenderDataFormat::UNSPECIFIED),
	mTangentOutFormat(RenderDataFormat::UNSPECIFIED),
	mBoneWeightOutFormat(RenderDataFormat::UNSPECIFIED),
	mUvFormat(RenderDataFormat::UNSPECIFIED),

	mVertexCount(0),
	mIndicesCount(0),
	mUvCount(0),
	mNumBonesPerVertex(0),

	mCurrentMaxVertexSimulation(0),
	mCurrentMaxVertexAdditionalSimulation(0),
	mCurrentMaxIndexSimulation(0)
{
}



ClothingMeshAssetData::ClothingMeshAssetData() :
	mImmediateClothMap(NULL),
	mSkinClothMap(NULL),
	mSkinClothMapB(NULL),
	mTetraMap(NULL),

	mImmediateClothMapCount(0),
	mSkinClothMapCount(0),
	mSkinClothMapBCount(0),
	mTetraMapCount(0),

	mSubmeshOffset(0),
	mSubMeshCount(0),

	mPhysicalMeshId(0),

	mSkinClothMapThickness(0.0f),
	mSkinClothMapOffset(0.0f),

	mBounds(PxBounds3::empty()),

	bActive(false)
{

}



ClothingPhysicalMeshData::ClothingPhysicalMeshData() :
	mVertices(NULL),
	mVertexCount(0),
	mSimulatedVertexCount(0),
	mMaxDistance0VerticesCount(0),
	mNormals(NULL),
	mSkinningNormals(NULL),
	mBoneIndices(NULL),
	mBoneWeights(NULL),
	mOptimizationData(NULL),
	mIndices(NULL),

	mSkinningNormalsCount(0),
	mBoneWeightsCount(0),
	mOptimizationDataCount(0),
	mIndicesCount(0),
	mSimulatedIndicesCount(0),

	mNumBonesPerVertex(0)
{

}



ClothingAssetData::ClothingAssetData() :
	mData(NULL),
	mCompressedNumBonesPerVertex(NULL),
	mCompressedTangentW(NULL),
	mExt2IntMorphMapping(NULL),
	mCompressedNumBonesPerVertexCount(0),
	mCompressedTangentWCount(0),
	mExt2IntMorphMappingCount(0),

	mAssetSize(0),
	mGraphicalLodsCount(0),
	mPhysicalMeshesCount(0),
	mPhysicalMeshOffset(0),
	mBoneCount(0),
	mRootBoneIndex(0)
{

}




ClothingAssetData::~ClothingAssetData()
{
	PX_FREE(mData);
	mData = NULL;
}



void ClothingAssetData::skinToBones(AbstractMeshDescription& destMesh, uint32_t submeshIndex, uint32_t graphicalMeshIndex, uint32_t startVertex,
                                    PxMat44* compositeMatrices, PxVec3* morphDisplacements)
{
	// This is no nice code, but it allows to have 8 different code paths through skinToBones that have all the if's figured out at compile time (hopefully)
	PX_ASSERT(destMesh.pTangent == NULL);
	PX_ASSERT(destMesh.pBitangent == NULL);

	if (destMesh.pNormal != NULL)
	{
		if (mBoneCount != 0)
		{
			if (morphDisplacements != NULL)
			{
				if (destMesh.pTangent4 != NULL)
				{
					skinToBonesInternal<true, true, true, true>(destMesh, submeshIndex, graphicalMeshIndex, startVertex, compositeMatrices, morphDisplacements);
				}
				else
				{
					skinToBonesInternal<true, true, true, false>(destMesh, submeshIndex, graphicalMeshIndex, startVertex, compositeMatrices, morphDisplacements);
				}
			}
			else
			{
				if (destMesh.pTangent4 != NULL)
				{
					skinToBonesInternal<true, true, false, true>(destMesh, submeshIndex, graphicalMeshIndex, startVertex, compositeMatrices, morphDisplacements);
				}
				else
				{
					skinToBonesInternal<true, true, false, false>(destMesh, submeshIndex, graphicalMeshIndex, startVertex, compositeMatrices, morphDisplacements);
				}
			}
		}
		else
		{
			if (morphDisplacements != NULL)
			{
				if (destMesh.pTangent4 != NULL)
				{
					skinToBonesInternal<true, false, true, true>(destMesh, submeshIndex, graphicalMeshIndex, startVertex, compositeMatrices, morphDisplacements);
				}
				else
				{
					skinToBonesInternal<true, false, true, false>(destMesh, submeshIndex, graphicalMeshIndex, startVertex, compositeMatrices, morphDisplacements);
				}
			}
			else
			{
				if (destMesh.pTangent4 != NULL)
				{
					skinToBonesInternal<true, false, false, true>(destMesh, submeshIndex, graphicalMeshIndex, startVertex, compositeMatrices, morphDisplacements);
				}
				else
				{
					skinToBonesInternal<true, false, false, false>(destMesh, submeshIndex, graphicalMeshIndex, startVertex, compositeMatrices, morphDisplacements);
				}
			}
		}
	}
	else
	{
		if (mBoneCount != 0)
		{
			if (morphDisplacements != NULL)
			{
				if (destMesh.pTangent4 != NULL)
				{
					skinToBonesInternal<false, true, true, true>(destMesh, submeshIndex, graphicalMeshIndex, startVertex, compositeMatrices, morphDisplacements);
				}
				else
				{
					skinToBonesInternal<false, true, true, false>(destMesh, submeshIndex, graphicalMeshIndex, startVertex, compositeMatrices, morphDisplacements);
				}
			}
			else
			{
				if (destMesh.pTangent4 != NULL)
				{
					skinToBonesInternal<false, true, false, true>(destMesh, submeshIndex, graphicalMeshIndex, startVertex, compositeMatrices, morphDisplacements);
				}
				else
				{
					skinToBonesInternal<false, true, false, false>(destMesh, submeshIndex, graphicalMeshIndex, startVertex, compositeMatrices, morphDisplacements);
				}
			}
		}
		else
		{
			if (morphDisplacements != NULL)
			{
				if (destMesh.pTangent4 != NULL)
				{
					skinToBonesInternal<false, false, true, true>(destMesh, submeshIndex, graphicalMeshIndex, startVertex, compositeMatrices, morphDisplacements);
				}
				else
				{
					skinToBonesInternal<false, false, true, false>(destMesh, submeshIndex, graphicalMeshIndex, startVertex, compositeMatrices, morphDisplacements);
				}
			}
			else
			{
				if (destMesh.pTangent4 != NULL)
				{
					skinToBonesInternal<false, false, false, true>(destMesh, submeshIndex, graphicalMeshIndex, startVertex, compositeMatrices, morphDisplacements);
				}
				else
				{
					skinToBonesInternal<false, false, false, false>(destMesh, submeshIndex, graphicalMeshIndex, startVertex, compositeMatrices, morphDisplacements);
				}
			}
		}
	}
}


template<bool withNormals, bool withBones, bool withMorph, bool withTangents>
void ClothingAssetData::skinToBonesInternal(AbstractMeshDescription& destMesh, uint32_t submeshIndex, uint32_t graphicalMeshIndex,
        uint32_t startVertex, PxMat44* compositeMatrices, PxVec3* morphDisplacements)
{
	PX_ASSERT(withNormals == (destMesh.pNormal != NULL));
	PX_ASSERT(withMorph == (morphDisplacements != NULL));

	PX_ASSERT(withTangents == (destMesh.pTangent4 != NULL));
	PX_ASSERT(destMesh.pTangent == NULL);
	PX_ASSERT(destMesh.pBitangent == NULL);

	// so we need to start further behind, but to make vec3* still aligned it must be a multiple of 4, and to make
	// compressedNumBones aligned, it needs to be a multiple of 16 (hence 16)
	// The start offset is reduced by up to 16, and instead of skipping the first [0, 16) vertices and adding additional logic
	// they just get computed as well, assuming much less overhead than an additional if in the innermost loop
	// Note: This only works if the mesh-mesh skinning is executed afterwards!
	const uint32_t startVertex16 = startVertex - (startVertex % 16);

	PX_ASSERT(((size_t)(compositeMatrices) & 0xf) == 0); // make sure the pointer is 16 byte aligned!!

	ClothingMeshAssetData* meshAsset = GetLod(graphicalMeshIndex);
	PX_ASSERT(submeshIndex < meshAsset->mSubMeshCount);
	ClothingAssetSubMesh* pSubmesh = GetSubmesh(meshAsset, submeshIndex);
	const uint32_t numVertices = pSubmesh->mVertexCount - startVertex16;
	if(numVertices == 0)
	{
		return;
	}

	// use the per-asset setting
	const uint32_t startVertexDiv16 = startVertex16 / 16;
	uint32_t mapSize = 0;
	const uint32_t* compressedNumBonesPerVertexEa = getCompressedNumBonesPerVertex(graphicalMeshIndex, submeshIndex, mapSize);
	PX_ASSERT((mapSize - startVertexDiv16) * 16 >= numVertices);

	const uint32_t* compressedNumBonesPerVertex = (const uint32_t*)((uint32_t*)compressedNumBonesPerVertexEa + startVertexDiv16);

	const PxVec3* PX_RESTRICT positionsEa = pSubmesh->mPositions + startVertex16;
	PX_ASSERT(pSubmesh->mPositionOutFormat == RenderDataFormat::FLOAT3);
	const PxVec3* PX_RESTRICT normalsEa = pSubmesh->mNormals + startVertex16;
	PX_ASSERT(pSubmesh->mNormalOutFormat == RenderDataFormat::FLOAT3);

	const PxVec4* PX_RESTRICT tangentsEa = pSubmesh->mTangents + (pSubmesh->mTangents != NULL ? startVertex16 : 0);
	PX_ASSERT(tangentsEa == NULL || pSubmesh->mTangentOutFormat == RenderDataFormat::FLOAT4);

	//These are effective addresses (ea)
	PxVec3* PX_RESTRICT destPositionsEa = destMesh.pPosition + startVertex16;
	PxVec3* PX_RESTRICT destNormalsEa = destMesh.pNormal + startVertex16;
	PxVec4* PX_RESTRICT destTangentsEa = (destMesh.pTangent4 != NULL) ? destMesh.pTangent4 + startVertex16 : NULL;

	uint32_t* morphReorderingEa = morphDisplacements != NULL ? getMorphMapping(graphicalMeshIndex) : NULL;
	if (morphReorderingEa != NULL)
	{
		morphReorderingEa += startVertex16;
	}
	else if (withMorph)
	{
		// actually we don't have a morph map, so let's take one step back
		skinToBonesInternal<withNormals, withBones, false, withTangents>(destMesh, submeshIndex, graphicalMeshIndex,
			startVertex, compositeMatrices, NULL);
		return;
	}
	PX_ASSERT((morphDisplacements != NULL) == (morphReorderingEa != NULL));

	const uint16_t* PX_RESTRICT boneIndicesEa = NULL;
	const float* PX_RESTRICT boneWeightsEa = NULL;

	const uint32_t maxNumBonesPerVertex = pSubmesh->mNumBonesPerVertex;
	if (withBones)
	{
		boneIndicesEa = (const uint16_t*)pSubmesh->mBoneIndices + (startVertex16 * maxNumBonesPerVertex);
		boneWeightsEa = (const float*)pSubmesh->mBoneWeights + (startVertex16 * maxNumBonesPerVertex);
	}


	//bones per vertex is LS address!!!!
	const uint32_t* PX_RESTRICT bonesPerVertex = compressedNumBonesPerVertex;

	//OK. Need to split this up into an even larger subset of work!!!! Perhaps sets of 1024 vertices/normals/bones....

	const uint32_t WorkSize = 160;

	const uint32_t countWork = (numVertices + (WorkSize - 1)) / WorkSize;

	for (uint32_t a = 0; a < countWork; ++a)
	{
		const uint32_t workCount = PxMin(numVertices - (a * WorkSize), WorkSize);

		const PxVec3* PX_RESTRICT positions = (const PxVec3 * PX_RESTRICT)(void*)positionsEa;
		const PxVec3* PX_RESTRICT normals = NULL;
		const float* PX_RESTRICT tangents4 = NULL;
		const uint16_t* PX_RESTRICT boneIndices = NULL;
		const float* PX_RESTRICT boneWeights = NULL;

		PxVec3* PX_RESTRICT destPositions = (PxVec3 * PX_RESTRICT)(void*)destPositionsEa;
		PxVec3* PX_RESTRICT destNormals = NULL;
		float* PX_RESTRICT destTangents4 = NULL;

		uint32_t* morphReordering = NULL;
		if (withMorph)
		{
			morphReordering = (uint32_t*)morphReorderingEa;
			morphReorderingEa += workCount;
		}

		if (withBones)
		{
			boneIndices = (const uint16_t * PX_RESTRICT)(void*)boneIndicesEa;
			boneWeights = (const float * PX_RESTRICT)(void*)boneWeightsEa;
		}

		if (withNormals)
		{
			destNormals = (PxVec3 * PX_RESTRICT)(void*)destNormalsEa;
			normals = (const PxVec3 * PX_RESTRICT)(void*)normalsEa;
		}

		if (withTangents)
		{
			destTangents4 = (float*)(void*)destTangentsEa;
			tangents4 = (const float*)(void*)tangentsEa;
		}


		const uint32_t count16 = (workCount + 15) / 16;
		for (uint32_t i = 0; i < count16; ++i)
		{
			//Fetch in the next block of stuff...
			const uint32_t maxVerts = PxMin(numVertices - (a * WorkSize) - (i * 16u), 16u);
			uint32_t numBoneWeight = *bonesPerVertex++;

			prefetchLine(positions + 16);

			if (withNormals)
			{
				prefetchLine(normals + 16);
			}

			if (withBones)
			{
				prefetchLine(boneIndices + (4 * 16));
				prefetchLine(boneWeights + (2 * 16));
				prefetchLine(boneWeights + (4 * 16));
			}

			if (withTangents)
			{
				prefetchLine(tangents4 + (4 * 16));
			}

			for (uint32_t j = 0; j < maxVerts; j++)
			{
				const uint32_t vertexIndex = i * 16 + j;
				const uint32_t numBones = (numBoneWeight & 0x3) + 1;
				numBoneWeight >>= 2;

				{
					PxVec3 untransformedPosition = *positions;
					if (withMorph)
					{
						const PxVec3* morphDisplacement = (PxVec3*)&morphDisplacements[morphReordering[vertexIndex]];
						untransformedPosition += *morphDisplacement;
					}
					Simd4f positionV = gSimd4fZero;
					Simd4f normalV = gSimd4fZero;
					Simd4f tangentV = gSimd4fZero;

					if (withBones)
					{
						for (uint32_t k = 0; k < numBones; k++)
						{
							float weight = boneWeights[k];

							PX_ASSERT(PxIsFinite(weight));

							const uint16_t boneNr = boneIndices[k];

							PxMat44& skinningMatrixV = (PxMat44&)compositeMatrices[boneNr];

							const Simd4f weightV = Simd4fScalarFactory(weight);

							Simd4f transformedPositionV = applyAffineTransform(skinningMatrixV, createSimd3f(untransformedPosition));
							transformedPositionV = transformedPositionV * weightV;
							positionV = positionV + transformedPositionV;

							if (withNormals)
							{
								Simd4f transformedNormalV = applyLinearTransform(skinningMatrixV, createSimd3f(*normals));
								transformedNormalV = transformedNormalV * weightV;
								normalV = normalV + transformedNormalV;
							}

							if (withTangents)
							{
								const Simd4f inTangent = Simd4fAlignedLoadFactory(tangents4);
								const Simd4f transformedTangentV = applyLinearTransform(skinningMatrixV, inTangent);
								tangentV = tangentV + transformedTangentV * weightV;
							}
						}
					}
					else
					{
						PxMat44& skinningMatrixV = (PxMat44&)compositeMatrices[0];
						positionV = applyAffineTransform(skinningMatrixV, createSimd3f(untransformedPosition));
						if (withNormals)
						{
							normalV = applyLinearTransform(skinningMatrixV, createSimd3f(*normals));
						}
						if (withTangents)
						{
							const Simd4f inTangent = Simd4fAlignedLoadFactory(tangents4);
							tangentV = applyLinearTransform(skinningMatrixV, inTangent);
						}

					}

					if (withNormals)
					{
						normalV = normalizeSimd3f(normalV);
						store3(&(destNormals->x), normalV);
					}


					if (withTangents)
					{
						tangentV = normalizeSimd3f(tangentV);
						const Simd4f bitangent = Simd4fScalarFactory(tangents4[3]);						
						const Simd4f outTangent = select(gSimd4fMaskXYZ, tangentV, bitangent);
						storeAligned(destTangents4, outTangent);
					}

					// if all weights are 0, we use the bind pose
					// we don't really need that and it just slows us down..
					//*destPositions = (numBones == 0) ? *positions : ps;
					//*destPositions = ps;
					store3(&destPositions->x, positionV);
				}

				positions++;
				positionsEa++;
				normals++;
				normalsEa++;
				if (withBones)
				{
					boneWeights += maxNumBonesPerVertex;
					boneWeightsEa += maxNumBonesPerVertex;
					boneIndices += maxNumBonesPerVertex;
					boneIndicesEa += maxNumBonesPerVertex;
				}
				if (withTangents)
				{
					tangents4 += 4;
					tangentsEa++;
					destTangents4 += 4;
					destTangentsEa++;
				}
				destPositions++;
				destPositionsEa++;
				destNormals++;
				destNormalsEa++;
			}
		}
	}

}



uint32_t* ClothingAssetData::getMorphMapping(uint32_t graphicalLod)
{
	if (mExt2IntMorphMappingCount == 0)
	{
		return NULL;
	}

	if (graphicalLod == (uint32_t) - 1 || graphicalLod > mGraphicalLodsCount)
	{
		graphicalLod = mGraphicalLodsCount;
	}

	uint32_t offset = 0;
	for (uint32_t i = 0; i < graphicalLod; i++)
	{
		const ClothingMeshAssetData* meshAsset = GetLod(i);
		for (uint32_t s = 0; s < meshAsset->mSubMeshCount; s++)
		{
			offset += GetSubmesh(meshAsset, s)->mVertexCount;
		}
	}

	PX_ASSERT(offset < mExt2IntMorphMappingCount);
	return mExt2IntMorphMapping + offset;
}



const uint32_t* ClothingAssetData::getCompressedNumBonesPerVertex(uint32_t graphicalLod, uint32_t submeshIndex, uint32_t& mapSize)
{
	mapSize = 0;

	uint32_t offset = 0;
	for (uint32_t lodIndex = 0; lodIndex < mGraphicalLodsCount; lodIndex++)
	{
		const ClothingMeshAssetData* meshAsset = GetLod(lodIndex);

		for (uint32_t s = 0; s < meshAsset->mSubMeshCount; s++)
		{
			const uint32_t numVertices = GetSubmesh(meshAsset, s)->mVertexCount;

			uint32_t numEntries = (numVertices + 15) / 16;
			while ((numEntries & 0x3) != 0) // this is a numEntries % 4
			{
				numEntries++;
			}

			if (lodIndex == graphicalLod && submeshIndex == s)
			{
				mapSize = numEntries;
				PX_ASSERT((mapSize % 4) == 0);
				return &mCompressedNumBonesPerVertex[offset];
			}

			offset += numEntries;
			PX_ASSERT(mCompressedNumBonesPerVertexCount >= offset);
		}
	}
	return NULL;
}



const uint32_t* ClothingAssetData::getCompressedTangentW(uint32_t graphicalLod, uint32_t submeshIndex, uint32_t& mapSize)
{
	mapSize = 0;

	uint32_t offset = 0;
	for (uint32_t lodIndex = 0; lodIndex < mGraphicalLodsCount; lodIndex++)
	{
		const ClothingMeshAssetData* meshAsset = GetLod(lodIndex);

		for (uint32_t s = 0; s < meshAsset->mSubMeshCount; s++)
		{
			const uint32_t numVertices = GetSubmesh(meshAsset, s)->mVertexCount;

			uint32_t numEntries = (numVertices + 31) / 32;
			while ((numEntries & 0x3) != 0) // this is a numEntries % 4
			{
				numEntries++;
			}

			if (lodIndex == graphicalLod && submeshIndex == s)
			{
				mapSize = numEntries;
				PX_ASSERT((mapSize % 4) == 0);
				return (uint32_t*)&mCompressedTangentW[offset];
			}

			offset += numEntries;
			PX_ASSERT(mCompressedTangentWCount >= offset);
		}
	}
	return NULL;
}



void ClothingAssetData::getNormalsAndVerticesForFace(PxVec3* vtx, PxVec3* nrm, uint32_t i,
        const AbstractMeshDescription& srcPM) const
{
	// copy indices for convenience
	PX_ASSERT(i < srcPM.numIndices);
	uint32_t di[3];
	for (uint32_t j = 0; j < 3; j++)
	{
		di[j] = srcPM.pIndices[i + j];
	}

	// To guarantee consistency in our implicit tetrahedral mesh definition we must always order vertices
	// idx[0,1,2] = min, max and mid
	uint32_t idx[3];
	idx[0] = PxMin(di[0], PxMin(di[1], di[2]));
	idx[1] = PxMax(di[0], PxMax(di[1], di[2]));
	idx[2] = idx[0];
	for (uint32_t j = 0; j < 3; j++)
	{
		if ((idx[0] != di[j]) && (idx[1] != di[j]))
		{
			idx[2] = di[j];
		}
	}

	for (uint32_t j = 0; j < 3; j++)
	{
		vtx[j] = srcPM.pPosition[idx[j]];
		nrm[j] = srcPM.pNormal[idx[j]];
#ifdef _DEBUG
		// sanity
		// PH: These normals 'should' always be normalized, maybe we can get rid of the normalize completely!
		const float length = nrm[j].magnitudeSquared();
		if (!(length >= 0.99f && length <= 1.01f))
		{
			static bool first = true;
			if (first)
			{
				PX_ALWAYS_ASSERT();
				first = false;
			}
		}
#else
		// PH: let's try and disable it in release mode...
		//nrm[j].normalize();
#endif
	};
}


uint32_t ClothingAssetData::skinClothMapBSkinVertex(PxVec3& dstPos, PxVec3* dstNormal, uint32_t vIndex,
        ClothingGraphicalLodParametersNS::SkinClothMapB_Type* pTCMB, const ClothingGraphicalLodParametersNS::SkinClothMapB_Type* pTCMBEnd,
        const AbstractMeshDescription& srcPM) const
{
	uint32_t numIterations(0);
	//uint32_t numSkipped(0);

	//PX_ASSERT(dstPos.isZero());
	//PX_ASSERT(dstNormal == NULL || dstNormal->isZero());

	PX_ASSERT(srcPM.avgEdgeLength != 0.0f);
	const float pmThickness = srcPM.avgEdgeLength;

	while ((pTCMB->vertexIndexPlusOffset == vIndex) && (pTCMB < pTCMBEnd))
	{
		// skip vertices that we couldn't find a binding for
		if (pTCMB->faceIndex0 == 0xFFFFFFFF || pTCMB->faceIndex0 >= srcPM.numIndices)
		{
			pTCMB++;
			//numSkipped++;
			//numIterations++;
			continue;
		}

		PxVec3 vtx[3], nrm[3];
		getNormalsAndVerticesForFace(vtx, nrm, pTCMB->faceIndex0, srcPM);

		const TetraEncoding_Local& tetEnc = tetraTableLocal[pTCMB->tetraIndex];
		const PxVec3 tv0 = vtx[0] + (tetEnc.sign[0] * pmThickness) * nrm[0];
		const PxVec3 tv1 = vtx[1] + (tetEnc.sign[1] * pmThickness) * nrm[1];
		const PxVec3 tv2 = vtx[2] + (tetEnc.sign[2] * pmThickness) * nrm[2];
		const PxVec3 tv3 = vtx[tetEnc.lastVtxIdx] + (tetEnc.sign[3] * pmThickness) * nrm[tetEnc.lastVtxIdx];

		const PxVec3& vtb = pTCMB->vtxTetraBary;
		const float vtbw = 1.0f - vtb.x - vtb.y - vtb.z;
		dstPos = (vtb.x * tv0) + (vtb.y * tv1) + (vtb.z * tv2) + (vtbw * tv3);
		PX_ASSERT(dstPos.isFinite());

		if (dstNormal != NULL)
		{
			const PxVec3& ntb = pTCMB->nrmTetraBary;
			const float ntbw = 1.0f - ntb.x - ntb.y - ntb.z;
			*dstNormal = (ntb.x * tv0) + (ntb.y * tv1) + (ntb.z * tv2) + (ntbw * tv3);
			PX_ASSERT(dstNormal->isFinite());
		}

		pTCMB++;
		numIterations++;
	}

	if (dstNormal != NULL && numIterations > 0)
	{
		// PH: this code certainly does not work if numIterations is bigger than 1 (it would need to average by dividing through numIterations)
		PX_ASSERT(numIterations == 1);
		*dstNormal -= dstPos;
		*dstNormal *= physx::shdfnd::recipSqrtFast(dstNormal->magnitudeSquared());
	}

	return numIterations;
}


uint32_t ClothingAssetData::skinClothMapB(PxVec3* dstPositions, PxVec3* dstNormals, uint32_t numVertices,
                                       const AbstractMeshDescription& srcPM, ClothingGraphicalLodParametersNS::SkinClothMapB_Type* map,
                                       uint32_t numVerticesInMap, bool computeNormals) const
{
	PX_ASSERT(srcPM.numIndices % 3 == 0);

	PX_ASSERT(srcPM.avgEdgeLength != 0.0f);

	ClothingGraphicalLodParametersNS::SkinClothMapB_Type* pTCMB = map;
	ClothingGraphicalLodParametersNS::SkinClothMapB_Type* pTCMBEnd = map + numVerticesInMap;

	for (uint32_t j = 0; j < numVerticesInMap; j++)
	{
		uint32_t vertexIndex = pTCMB->vertexIndexPlusOffset;

		if (vertexIndex >= numVertices)
		{
			pTCMB++;
			continue;
		}

		PxVec3& p = dstPositions[vertexIndex];

		PxVec3* n = NULL;
		if (computeNormals)
		{
			n = dstNormals + vertexIndex;
		}

		uint32_t numIters = skinClothMapBSkinVertex(p, n, vertexIndex, pTCMB, pTCMBEnd, srcPM);
		if (numIters == 0)
		{
			// PH: find next submesh
			const uint32_t currentIterations = (uint32_t)(size_t)(pTCMB - map);
			for (uint32_t i = currentIterations; i < numVerticesInMap; i++)
			{
				if (map[i].submeshIndex > pTCMB->submeshIndex)
				{
					numIters = i - currentIterations;
					break;
				}
			}

			// only return if it's still 0
			if (numIters == 0)
			{
				return currentIterations;
			}
		}

		pTCMB += numIters;
	}

	return numVertices;
}

bool ClothingAssetData::skinToTetraMesh(AbstractMeshDescription& destMesh,
                                        const AbstractMeshDescription& srcPM,
                                        const ClothingMeshAssetData& graphicalLod)
{
	if (graphicalLod.mTetraMap == NULL)
	{
		return false;
	}

	PX_ASSERT(srcPM.numIndices % 4 == 0);

	PX_ASSERT(destMesh.pIndices == NULL);
	PX_ASSERT(destMesh.pTangent == NULL);
	PX_ASSERT(destMesh.pBitangent == NULL);
	const bool computeNormals = destMesh.pNormal != NULL;

	PxVec3 dummyNormal;

	const uint32_t numGraphicalVertices = destMesh.numVertices;

	const uint32_t* mainIndices = GetPhysicalMesh(graphicalLod.mPhysicalMeshId)->mIndices;

	for (uint32_t i = 0; i < numGraphicalVertices; i++)
	{
		const ClothingGraphicalLodParametersNS::TetraLink_Type& currentLink = graphicalLod.mTetraMap[i];
		if (currentLink.tetraIndex0 >= srcPM.numIndices)
		{
			continue;
		}

		PxVec3& position = destMesh.pPosition[i];
		position = PxVec3(0.0f);

		float vertexBary[4];
		vertexBary[0] = currentLink.vertexBary.x;
		vertexBary[1] = currentLink.vertexBary.y;
		vertexBary[2] = currentLink.vertexBary.z;
		vertexBary[3] = 1 - vertexBary[0] - vertexBary[1] - vertexBary[2];

		const uint32_t* indices = mainIndices + currentLink.tetraIndex0;

		if (computeNormals)
		{
			PxVec3& normal = computeNormals ? destMesh.pNormal[i] : dummyNormal;
			normal = PxVec3(0.0f);

			float normalBary[4];
			normalBary[0] = currentLink.normalBary.x;
			normalBary[1] = currentLink.normalBary.y;
			normalBary[2] = currentLink.normalBary.z;
			normalBary[3] = 1 - normalBary[0] - normalBary[1] - normalBary[2];

			// compute skinned positions and normals
			for (uint32_t j = 0; j < 4; j++)
			{
				const PxVec3& pos = srcPM.pPosition[indices[j]];
				position += pos * vertexBary[j];
				normal += pos * normalBary[j];
			}

			normal = normal - position;
			normal *= nvidia::recipSqrtFast(normal.magnitudeSquared());
		}
		else
		{
			// only compute skinned positions, not normals
			for (uint32_t j = 0; j < 4; j++)
			{
				const PxVec3& pos = srcPM.pPosition[indices[j]];
				position += pos * vertexBary[j];
			}
		}
		PX_ASSERT(position.isFinite());
	}
	return true;
}



}
}
