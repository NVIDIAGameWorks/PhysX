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


#include "PxPreprocessor.h"
#include "RenderDataFormat.h"
#include "ClothingActorData.h"
#include "AbstractMeshDescription.h"
#include "PsIntrinsics.h"
#include "PxMat44.h"
#include "ApexSDKIntl.h"

#include "ClothingGlobals.h"
#include "SimdMath.h"

#include "ProfilerCallback.h"

using namespace physx::shdfnd;

#pragma warning(disable : 4101 4127) // unreferenced local variable and conditional is constant

#define NX_PARAMETERIZED_ONLY_LAYOUTS
#include "ClothingGraphicalLodParameters.h"

#include "PsIntrinsics.h"
#include "PsVecMath.h"

namespace nvidia
{
namespace clothing
{


ClothingActorData::ClothingActorData() :
	mNewBounds(PxBounds3::empty()),

	mGlobalPose(PxVec4(1.0f)),
	mInternalGlobalPose(PxVec4(1.0f)),

	mInternalBoneMatricesCur(NULL),
	mInternalBoneMatricesPrev(NULL),
	mRenderingDataPosition(NULL),
	mRenderingDataNormal(NULL),
	mRenderingDataTangent(NULL),
	mMorphDisplacementBuffer(NULL),
	mSdkWritebackNormal(NULL),
	mSdkWritebackPositions(NULL),
	mSkinnedPhysicsPositions(NULL),
	mSkinnedPhysicsNormals(NULL),

	mInternalMatricesCount(0),
	mMorphDisplacementBufferCount(0),
	mSdkDeformableVerticesCount(0),
	mSdkDeformableIndicesCount(0),
	mCurrentGraphicalLodId(0),
	mCurrentPhysicsSubmesh(0),

	mActorScale(0.0f),

	bInternalFrozen(false),
	bShouldComputeRenderData(false),
	bIsInitialized(false),
	bIsSimulationMeshDirty(false),
	bRecomputeNormals(false),
	bRecomputeTangents(false),
	bCorrectSimulationNormals(false),
	bParallelCpuSkinning(false),
	bIsClothingSimulationNull(false)
{
}



ClothingActorData::~ClothingActorData()
{
	PX_ASSERT(mInternalBoneMatricesCur == NULL); // properly deallocated
}



void ClothingActorData::renderDataLock()
{
	mRenderLock.lock();
}



void ClothingActorData::renderDataUnLock()
{
	//TODO - release a mutex here
	mRenderLock.unlock();
}



void ClothingActorData::skinPhysicsMaxDist0Normals_NoPhysx()
{
	if (mSdkWritebackNormal == NULL /*|| bInternalFrozen == 1*/)
	{
		return;
	}

	//ClothingPhysicalMeshParametersNS::PhysicalMesh_Type* physicalMesh = mAsset->getPhysicalMeshFromLod(mCurrentGraphicalLodId);

	ClothingPhysicalMeshData* physicalMesh = mAsset.GetPhysicalMeshFromLod(mCurrentGraphicalLodId);
	const PxVec3* PX_RESTRICT _normals = physicalMesh->mSkinningNormals;

	if (_normals == NULL)
	{
		return;
	}

	if (physicalMesh->mMaxDistance0VerticesCount == 0)
	{
		return;
	}

	const uint32_t startVertex = physicalMesh->mSimulatedVertexCount - physicalMesh->mMaxDistance0VerticesCount;
	const uint32_t numVertices = physicalMesh->mSimulatedVertexCount;
	const uint32_t numBoneIndicesPerVertex = physicalMesh->mNumBonesPerVertex;

	// offset the normals array as well
	_normals += startVertex;

	const uint32_t UnrollSize = 160;
	const uint32_t vertCount = numVertices - startVertex;
	const uint32_t numIterations = (vertCount + UnrollSize - 1) / UnrollSize;

	PxVec3* PX_RESTRICT targetNormals = mSdkWritebackNormal + startVertex;

	//uint32_t tags[2] = {10, 11};
	//const uint32_t prefetchRange = (startVertex & 0xfffffff0); //A multiple of 16 before this prefetch, with the assumption that normals is 16-byte aligned!
	//C_Prefetcher<2, sizeof(PxVec3) * UnrollSize> normPrefetcher(tags, (void*)(normals + prefetchRange), (void*)(normals + numVertices));

	if (mInternalBoneMatricesCur == NULL || numBoneIndicesPerVertex == 0)
	{
		if (mActorScale == 1.0f)
		{
			for (uint32_t a = 0; a < numIterations; ++a)
			{
				const uint32_t numToProcess = PxMin(UnrollSize, (vertCount - (UnrollSize * a)));
				const PxVec3* PX_RESTRICT localNormals = (const PxVec3 * PX_RESTRICT)(void*)_normals;
				for (uint32_t i = 0; i < numToProcess; i++)
				{
					targetNormals[i] = mInternalGlobalPose.rotate(localNormals[i]);
				}
				targetNormals += UnrollSize;
				_normals += UnrollSize;
			}
		}
		else
		{
			const float recipActorScale = 1.f / mActorScale;
			for (uint32_t a = 0; a < numIterations; ++a)
			{
				const uint32_t numToProcess = PxMin(UnrollSize, (vertCount - (UnrollSize * a)));
				const PxVec3* PX_RESTRICT localNormals = (const PxVec3 * PX_RESTRICT)(void*)_normals;
				for (uint32_t i = 0; i < numToProcess; i++)
				{
					targetNormals[i] = mInternalGlobalPose.rotate(localNormals[i]) * recipActorScale;
				}
				targetNormals += UnrollSize;
				_normals += UnrollSize;
			}
		}
	}
	else
	{
		//OK a slight refactor is required here - we don't want to fetch in everything only to
		const uint32_t startBoneIndex = startVertex * numBoneIndicesPerVertex;
		//Another problem - this is an arbitrarily large amount of data that has to be fetched here!!!! Consider revising

		const uint16_t* PX_RESTRICT eaSimBoneIndices = &physicalMesh->mBoneIndices[startBoneIndex];
		const float* PX_RESTRICT eaSimBoneWeights = &physicalMesh->mBoneWeights[startBoneIndex];

		const PxMat44* const PX_RESTRICT matrices = (const PxMat44*)mInternalBoneMatricesCur;

		for (uint32_t a = 0; a < numIterations; ++a)
		{
			const uint32_t numToProcess = PxMin(UnrollSize, (vertCount - (UnrollSize * a)));
			const PxVec3* PX_RESTRICT localNormals = (const PxVec3 * PX_RESTRICT)(void*)_normals;

			const uint16_t* const PX_RESTRICT simBoneIndices = (const uint16_t * const PX_RESTRICT)(void*)eaSimBoneIndices;
			const float* const PX_RESTRICT simBoneWeights = (const float * const PX_RESTRICT)(void*)eaSimBoneWeights;

			eaSimBoneIndices += numBoneIndicesPerVertex * numToProcess;
			eaSimBoneWeights += numBoneIndicesPerVertex * numToProcess;

			for (uint32_t i = 0; i < numToProcess; i++)
			{
				PxVec3 normal(0.0f, 0.0f, 0.0f);
				for (uint32_t j = 0; j < numBoneIndicesPerVertex; j++)
				{
					const float weight = simBoneWeights[i * numBoneIndicesPerVertex + j];

					if (weight > 0.f)
					{
						PX_ASSERT(weight <= 1.0f);
						const uint32_t index = simBoneIndices[i * numBoneIndicesPerVertex + j];

						const PxMat44& bone = matrices[index];

						normal += bone.rotate(localNormals[i]) * weight; // 12% here
					}
					else
					{
						// PH: Assuming sorted weights is faster
						break;
					}
				}

				normal.normalize();
				targetNormals[i] = normal;
			}
			targetNormals += UnrollSize;
			_normals += UnrollSize;
		}

	}

}


void ClothingActorData::skinToAnimation_NoPhysX(bool fromFetchResults)
{
	// This optimization only works if the render data from last frame is still there.
	// So this can only be used if we're using the same ClothingRenderProxy again.
	//if (!bIsSimulationMeshDirty)
	//{
	//	return;
	//}

	PX_PROFILE_ZONE("ClothingActorImpl::skinToAnimation", GetInternalApexSDK()->getContextId());

	//const bool recomputeNormals = bRecomputeNormals;

	// PH: If fromFetchResults is true, renderLock does not need to be aquired as it is already aquired by ApexScene::fetchResults()
	if (!fromFetchResults)
	{
		renderDataLock();
	}

	for (uint32_t graphicalLod = 0; graphicalLod < mAsset.mGraphicalLodsCount; graphicalLod++)
	{
		ClothingMeshAssetData& meshAsset = *mAsset.GetLod(graphicalLod);
		if (!meshAsset.bActive)
		{
			continue;
		}

		uint32_t submeshVertexOffset = 0;

		for (uint32_t submeshIndex = 0; submeshIndex < meshAsset.mSubMeshCount; submeshIndex++)
		{
			AbstractMeshDescription renderData;
			ClothingAssetSubMesh* pSubMesh = mAsset.GetSubmesh(&meshAsset, submeshIndex);
			renderData.numVertices = pSubMesh->mVertexCount;

			renderData.pPosition = mRenderingDataPosition + submeshVertexOffset;

			renderData.pNormal = mRenderingDataNormal + submeshVertexOffset;

			if (mRenderingDataTangent != NULL)
			{
				renderData.pTangent4 = mRenderingDataTangent + submeshVertexOffset;
			}

			PxMat44* matrices = NULL;
			PX_ALIGN(16, PxMat44 alignedGlobalPose); // matrices must be 16 byte aligned!
			if (mInternalBoneMatricesCur == NULL)
			{
				matrices = &alignedGlobalPose;
				alignedGlobalPose = mInternalGlobalPose;
			}
			else
			{
				matrices = (PxMat44*)mInternalBoneMatricesCur;
				PX_ASSERT(matrices != NULL);
			}

			mAsset.skinToBones(renderData, submeshIndex, graphicalLod, pSubMesh->mCurrentMaxVertexSimulation, matrices, mMorphDisplacementBuffer);

			submeshVertexOffset += pSubMesh->mVertexCount;
		}
	}

	if (!fromFetchResults)
	{
		renderDataUnLock();
	}
}

template<bool computeNormals>
uint32_t ClothingAssetData::skinClothMap(PxVec3* dstPositions, PxVec3* dstNormals, PxVec4* dstTangents, uint32_t numVertices,
									const AbstractMeshDescription& srcPM, ClothingGraphicalLodParametersNS::SkinClothMapD_Type* map,
									uint32_t numVerticesInMap, float offsetAlongNormal, float actorScale) const
{
	PX_ASSERT(srcPM.numIndices % 3 == 0);

	const ClothingGraphicalLodParametersNS::SkinClothMapD_Type* PX_RESTRICT pTCM = map;
	nvidia::prefetchLine(pTCM);

	const float invOffsetAlongNormal = 1.0f / offsetAlongNormal;

	uint32_t numVerticesWritten = 0;
	uint32_t numTangentsWritten = 0;
	const uint32_t numVerticesTotal = numVertices;

	uint32_t firstMiss = numVerticesInMap;

	const uint32_t unrollCount = 256;

	const uint32_t numIterations = (numVerticesInMap + unrollCount - 1) / unrollCount;

	//uint32_t vertexIndex = 0;
	for (uint32_t a = 0; a < numIterations; ++a)
	{
		const uint32_t numToProcess = PxMin(numVerticesInMap - (a * unrollCount), unrollCount);
		const ClothingGraphicalLodParametersNS::SkinClothMapD_Type* PX_RESTRICT pTCMLocal =
			(const ClothingGraphicalLodParametersNS::SkinClothMapD_Type * PX_RESTRICT)(void*)pTCM;

		for (uint32_t j = 0; j < numToProcess; ++j)
		{
			nvidia::prefetchLine(pTCMLocal + 1);

			//PX_ASSERT(vertexIndex == pTCMLocal->vertexIndexPlusOffset);
			uint32_t vertexIndex = pTCMLocal->vertexIndexPlusOffset;
			const uint32_t physVertIndex0 = pTCMLocal->vertexIndex0;
			const uint32_t physVertIndex1 = pTCMLocal->vertexIndex1;
			const uint32_t physVertIndex2 = pTCMLocal->vertexIndex2;

			if (vertexIndex >= numVerticesTotal)
			{
				pTCM++;
				pTCMLocal++;
				//vertexIndex++;
				continue;
			}

			// TODO do only 1 test, make sure physVertIndex0 is the smallest index
			if (physVertIndex0 >= srcPM.numVertices || physVertIndex1 >= srcPM.numVertices || physVertIndex2 >= srcPM.numVertices)
			{
				firstMiss = PxMin(firstMiss, vertexIndex);
				pTCM++;
				pTCMLocal++;
				//vertexIndex++;
				continue;
			}

			numVerticesWritten++;

			//PX_ASSERT(!vertexWriteCache.IsStomped());

			const PxVec3 vtx[3] =
			{
				*(PxVec3*)&srcPM.pPosition[physVertIndex0],
				*(PxVec3*)&srcPM.pPosition[physVertIndex1],
				*(PxVec3*)&srcPM.pPosition[physVertIndex2],
			};

			//PX_ASSERT(!vertexWriteCache.IsStomped());

			const PxVec3 nrm[3] =
			{
				*(PxVec3*)&srcPM.pNormal[physVertIndex0],
				*(PxVec3*)&srcPM.pNormal[physVertIndex1],
				*(PxVec3*)&srcPM.pNormal[physVertIndex2],
			};

			//PX_ASSERT(!vertexWriteCache.IsStomped());

			PxVec3 bary = pTCMLocal->vertexBary;
			const float vHeight = bary.z * actorScale;
			bary.z = 1.0f - bary.x - bary.y;

			const PxVec3 positionVertex = bary.x * vtx[0] + bary.y * vtx[1] + bary.z * vtx[2];
			const PxVec3 positionNormal = (bary.x * nrm[0] + bary.y * nrm[1] + bary.z * nrm[2]) * vHeight;

			const PxVec3 resultPosition = positionVertex + positionNormal;
			//Write back - to use a DMA list

			PxVec3* dstPosition = (PxVec3*)&dstPositions[vertexIndex];

			*dstPosition = resultPosition;

			PX_ASSERT(resultPosition.isFinite());

			if (computeNormals)
			{
				bary = pTCMLocal->normalBary;
				const float nHeight = bary.z * actorScale;
				bary.z = 1.0f - bary.x - bary.y;

				const PxVec3 normalVertex = bary.x * vtx[0] + bary.y * vtx[1] + bary.z * vtx[2];
				const PxVec3 normalNormal = (bary.x * nrm[0] + bary.y * nrm[1] + bary.z * nrm[2]) * nHeight;

				PxVec3* dstNormal = (PxVec3*)&dstNormals[vertexIndex];

				// we multiply in invOffsetAlongNormal in order to get a newNormal that is closer to size 1,
				// so the normalize approximation will be better
				PxVec3 newNormal = ((normalVertex + normalNormal) - (resultPosition)) * invOffsetAlongNormal;
#if 1
				// PH: Normally this is accurate enough. For testing we can also use the second
				const PxVec3 resultNormal = newNormal * nvidia::recipSqrtFast(newNormal.magnitudeSquared());
				*dstNormal = resultNormal;
#else
				newNormal.normalize();
				*dstNormal = newNormal;
#endif
			}
			if (dstTangents != NULL)
			{
				bary = pTCMLocal->tangentBary;
				const float nHeight = bary.z * actorScale;
				bary.z = 1.0f - bary.x - bary.y;

				const PxVec3 tangentVertex = bary.x * vtx[0] + bary.y * vtx[1] + bary.z * vtx[2];
				const PxVec3 tangentTangent = (bary.x * nrm[0] + bary.y * nrm[1] + bary.z * nrm[2]) * nHeight;

				PxVec4* dstTangent = (PxVec4*)&dstTangents[vertexIndex];

				// we multiply in invOffsetAlongNormal in order to get a newNormal that is closer to size 1,
				// so the normalize approximation will be better
				PxVec3 newTangent = ((tangentVertex + tangentTangent) - (resultPosition)) * invOffsetAlongNormal;
#if 1
				// PH: Normally this is accurate enough. For testing we can also use the second
				const PxVec3 resultTangent = newTangent * nvidia::recipSqrtFast(newTangent.magnitudeSquared());

				uint32_t arrayIndex	= numTangentsWritten / 4;
				uint32_t offset		= numTangentsWritten % 4;
				float w = ((mCompressedTangentW[arrayIndex] >> offset) & 1) ? 1.f : -1.f;

				*dstTangent = PxVec4(resultTangent, w);
#else
				newTangent.normalize();
				*dstTangent = newTangent;
#endif
			}

			pTCM++;
			pTCMLocal++;
			//vertexIndex++;
		}
	}

	return firstMiss;
}


#if PX_ANDROID || PX_LINUX
template uint32_t ClothingAssetData::skinClothMap<true>(PxVec3* dstPositions, PxVec3* dstNormals, PxVec4* dstTangents, uint32_t numVertices,
                                       const AbstractMeshDescription& srcPM, ClothingGraphicalLodParametersNS::SkinClothMapD_Type* map,
                                       uint32_t numVerticesInMap, float offsetAlongNormal, float actorScale) const;

template uint32_t ClothingAssetData::skinClothMap<false>(PxVec3* dstPositions, PxVec3* dstNormals, PxVec4* dstTangents, uint32_t numVertices,
                                       const AbstractMeshDescription& srcPM, ClothingGraphicalLodParametersNS::SkinClothMapD_Type* map,
                                       uint32_t numVerticesInMap, float offsetAlongNormal, float actorScale) const;
#endif

void ClothingActorData::skinToImmediateMap(const uint32_t* immediateClothMap_, uint32_t numGraphicalVertices_, uint32_t numSrcVertices_,
        const PxVec3* srcPositions_)
{
	const uint32_t* PX_RESTRICT immediateClothMap = immediateClothMap_;

	const PxVec3* PX_RESTRICT srcPositions = srcPositions_;
	PxVec3* PX_RESTRICT destPositions = mRenderingDataPosition;

	const uint32_t numGraphicalVertices = numGraphicalVertices_;
	const uint32_t numSrcVertices = numSrcVertices_;

	const uint32_t WorkSize = 512;

	const uint32_t numIterations = (numGraphicalVertices + WorkSize - 1) / WorkSize;

	for (uint32_t a = 0; a < numIterations; ++a)
	{
		const uint32_t numToProcess = PxMin(numGraphicalVertices - (a * WorkSize), WorkSize);

		const uint32_t* PX_RESTRICT immediateClothMapLocal = (const uint32_t * PX_RESTRICT)(void*)&immediateClothMap[a * WorkSize];
		PxVec3* PX_RESTRICT destPositionsLocal = (PxVec3 * PX_RESTRICT)(void*)&destPositions[a * WorkSize];

		for (uint32_t j = 0; j < numToProcess; ++j)
		{
			const uint32_t mapEntry = immediateClothMapLocal[j];
			const uint32_t index = mapEntry & ClothingConstants::ImmediateClothingReadMask;
			const uint32_t flags = mapEntry & ~ClothingConstants::ImmediateClothingReadMask;

			if (index < numSrcVertices && ((flags & ClothingConstants::ImmediateClothingInSkinFlag)) == 0)
			{
				destPositionsLocal[j] = *((PxVec3*)(void*)&srcPositions[index]);
				PX_ASSERT(destPositionsLocal[j].isFinite());
			}
		}
	}
}



void ClothingActorData::skinToImmediateMap(const uint32_t* immediateClothMap_, uint32_t numGraphicalVertices_, uint32_t numSrcVertices_,
        const PxVec3* srcPositions_, const PxVec3* srcNormals_)
{
	const uint32_t* PX_RESTRICT immediateClothMap = immediateClothMap_;

	const PxVec3* PX_RESTRICT srcPositions = srcPositions_;
	const PxVec3* PX_RESTRICT srcNormals = srcNormals_;

	PxVec3* PX_RESTRICT destPositions = mRenderingDataPosition;
	PxVec3* PX_RESTRICT destNormals = mRenderingDataNormal;

	const uint32_t numGraphicalVertices = numGraphicalVertices_;
	const uint32_t numSrcVertices = numSrcVertices_;

	const uint32_t WorkSize = 160;

	//__builtin_snpause();

	const uint32_t numIterations = (numGraphicalVertices + WorkSize - 1) / WorkSize;
	
	for (uint32_t a = 0; a < numIterations; ++a)
	{
		const uint32_t numToProcess = PxMin(numGraphicalVertices - (a * WorkSize), WorkSize);

		const uint32_t* PX_RESTRICT immediateClothMapLocal = (const uint32_t * PX_RESTRICT)(void*)&immediateClothMap[a * WorkSize];
		PxVec3* PX_RESTRICT destPositionsLocal = (PxVec3 * PX_RESTRICT)(void*)&destPositions[a * WorkSize];
		PxVec3* PX_RESTRICT destNormalsLocal = (PxVec3 * PX_RESTRICT)(void*)&destNormals[a * WorkSize];

		for (uint32_t j = 0; j < numToProcess; ++j)
		{
			const uint32_t mapEntry = immediateClothMapLocal[j];
			const uint32_t index = mapEntry & ClothingConstants::ImmediateClothingReadMask;
			const uint32_t flags = mapEntry & ~ClothingConstants::ImmediateClothingReadMask;

			if (index < numSrcVertices && ((flags & ClothingConstants::ImmediateClothingInSkinFlag)) == 0)
			{
				destPositionsLocal[j] = *((PxVec3*)(void*)&srcPositions[index]);
				PX_ASSERT(destPositionsLocal[j].isFinite());

				const PxVec3 destNormal = *((PxVec3*)(void*)&srcNormals[index]);
				destNormalsLocal[j] = (flags & ClothingConstants::ImmediateClothingInvertNormal) ? -destNormal : destNormal;
				PX_ASSERT(destNormalsLocal[j].isFinite());
			}
		}
	}
}



void ClothingActorData::skinToPhysicalMesh_NoPhysX(bool fromFetchResults)
{
	// This optimization only works if the render data from last frame is still there.
	// So this can only be used if we're using the same ClothingRenderProxy again.
	//if (!bIsSimulationMeshDirty)
	//{
	//	return;
	//}

	PX_PROFILE_ZONE("ClothingActorImpl::meshMesh-Skinning", GetInternalApexSDK()->getContextId());

	const ClothingMeshAssetData& graphicalLod = *mAsset.GetLod(mCurrentGraphicalLodId);

	const ClothingPhysicalMeshData* physicalMesh = mAsset.GetPhysicalMeshFromLod(mCurrentGraphicalLodId);

	AbstractMeshDescription pcm;
	pcm.numVertices = mSdkDeformableVerticesCount;
	pcm.numIndices = mSdkDeformableIndicesCount;
	pcm.pPosition = mSdkWritebackPositions;
	pcm.pNormal = mSdkWritebackNormal;
	pcm.pIndices = physicalMesh->mIndices;
	pcm.avgEdgeLength = graphicalLod.mSkinClothMapThickness;

	const bool skinNormals = !bRecomputeNormals;

	if (!fromFetchResults)
	{
		renderDataLock();
	}

	uint32_t activeCount = 0;

	for (uint32_t i = 0; i < mAsset.mGraphicalLodsCount; i++)
	{
		const ClothingMeshAssetData& lod = *mAsset.GetLod(i);
		if (!lod.bActive)
		{
			continue;
		}
		activeCount++;

		bool skinTangents = !bRecomputeTangents;

		uint32_t graphicalVerticesCount = 0;
		for (uint32_t j = 0; j < lod.mSubMeshCount; j++)
		{
			ClothingAssetSubMesh* subMesh = mAsset.GetSubmesh(&lod, j);
			graphicalVerticesCount += subMesh->mVertexCount; // only 1 part is supported

			if (subMesh->mTangents == NULL)
			{
				skinTangents = false;
			}
		}

		//__builtin_snpause();
		//RenderMeshAssetIntl* renderMeshAsset = mAsset->getGraphicalMesh(i);
		//PX_ASSERT(renderMeshAsset != NULL);

		// Do mesh-to-mesh skinning here
		if (graphicalLod.mSkinClothMapB != NULL)
		{
			mAsset.skinClothMapB(mRenderingDataPosition, mRenderingDataNormal, graphicalVerticesCount, pcm,
			                     graphicalLod.mSkinClothMapB, graphicalLod.mSkinClothMapBCount, skinNormals);
		}
		else if (graphicalLod.mSkinClothMap != NULL)
		{
			PxVec4* tangents = skinTangents ? mRenderingDataTangent : NULL;
			if (skinNormals)
				mAsset.skinClothMap<true>(mRenderingDataPosition, mRenderingDataNormal, tangents, graphicalVerticesCount, pcm,
											graphicalLod.mSkinClothMap, graphicalLod.mSkinClothMapCount, graphicalLod.mSkinClothMapOffset, mActorScale);
			else
				mAsset.skinClothMap<false>(mRenderingDataPosition, mRenderingDataNormal, tangents, graphicalVerticesCount, pcm,
											graphicalLod.mSkinClothMap, graphicalLod.mSkinClothMapCount, graphicalLod.mSkinClothMapOffset, mActorScale);

		}
		else if (graphicalLod.mTetraMap != NULL)
		{
			AbstractMeshDescription destMesh;
			destMesh.pPosition = mRenderingDataPosition;
			if (skinNormals)
			{
				destMesh.pNormal = mRenderingDataNormal;
			}
			destMesh.numVertices = graphicalVerticesCount;
			mAsset.skinToTetraMesh(destMesh, pcm, graphicalLod);
		}

		if (graphicalLod.mImmediateClothMap != NULL)
		{
			if (skinNormals)
			{
				skinToImmediateMap(graphicalLod.mImmediateClothMap, graphicalVerticesCount, pcm.numVertices, pcm.pPosition, pcm.pNormal);
			}
			else
			{
				skinToImmediateMap(graphicalLod.mImmediateClothMap, graphicalVerticesCount, pcm.numVertices, pcm.pPosition);
			}
		}
	}

	PX_ASSERT(activeCount < 2);

	if (!fromFetchResults)
	{
		renderDataUnLock();
	}
}






void ClothingActorData::finalizeSkinning_NoPhysX(bool fromFetchResults)
{
	// PH: If fromFetchResults is true, renderLock does not need to be aquired as it is already aquired by ApexScene::fetchResults()
	if (!fromFetchResults)
	{
		renderDataLock();
	}

	mNewBounds.setEmpty();

	for (uint32_t graphicalLod = 0; graphicalLod < mAsset.mGraphicalLodsCount; graphicalLod++)
	{
		ClothingMeshAssetData& renderMeshAsset = *mAsset.GetLod(graphicalLod);
		if (!renderMeshAsset.bActive)
		{
			continue;
		}

		const uint32_t submeshCount = renderMeshAsset.mSubMeshCount;

		uint32_t submeshVertexOffset = 0;
		for (uint32_t submeshIndex = 0; submeshIndex < submeshCount; submeshIndex++)
		{
			AbstractMeshDescription renderData;

			ClothingAssetSubMesh* pSubmesh = mAsset.GetSubmesh(&renderMeshAsset, submeshIndex);

			renderData.numVertices = pSubmesh->mVertexCount;

			renderData.pPosition = mRenderingDataPosition + submeshVertexOffset;

			bool recomputeTangents = bRecomputeTangents && renderMeshAsset.bNeedsTangents;
			if (bRecomputeNormals || recomputeTangents)
			{
				renderData.pNormal = mRenderingDataNormal + submeshVertexOffset;

				const uint32_t* compressedTangentW = NULL;

				if (recomputeTangents)
				{
					renderData.pTangent4 = mRenderingDataTangent + submeshVertexOffset;
					uint32_t mapSize = 0;
					compressedTangentW = mAsset.getCompressedTangentW(graphicalLod, submeshIndex, mapSize);
				}
				if (bRecomputeNormals && recomputeTangents)
				{
					PX_PROFILE_ZONE("ClothingActorImpl::recomupteNormalAndTangent", GetInternalApexSDK()->getContextId());
					computeTangentSpaceUpdate<true, true>(renderData, renderMeshAsset, submeshIndex, compressedTangentW);
				}
				else if (bRecomputeNormals)
				{
					PX_PROFILE_ZONE("ClothingActorImpl::recomupteNormal", GetInternalApexSDK()->getContextId());
					computeTangentSpaceUpdate<true, false>(renderData, renderMeshAsset, submeshIndex, compressedTangentW);
				}
				else
				{
					PX_PROFILE_ZONE("ClothingActorImpl::recomupteTangent", GetInternalApexSDK()->getContextId());
					computeTangentSpaceUpdate<false, true>(renderData, renderMeshAsset, submeshIndex, compressedTangentW);
				}
			}

			const uint32_t unrollCount = 1024;
			const uint32_t numIterations = (renderData.numVertices + unrollCount - 1) / unrollCount;

			for (uint32_t a = 0; a < numIterations; ++a)
			{
				const uint32_t numToProcess = PxMin(unrollCount, renderData.numVertices - (a * unrollCount));
				const PxVec3* PX_RESTRICT positions = (const PxVec3 * PX_RESTRICT)(renderData.pPosition + (a * unrollCount));
				for (uint32_t b = 0; b < numToProcess; ++b)
				{
					mNewBounds.include(positions[b]);
				}
			}

			submeshVertexOffset += renderData.numVertices;
		}
	}

	if (!fromFetchResults)
	{
		renderDataUnLock();
	}
}

#define FLOAT_TANGENT_UPDATE 0


template <bool withNormals, bool withTangents>
void ClothingActorData::computeTangentSpaceUpdate(AbstractMeshDescription& destMesh,
        const ClothingMeshAssetData& rendermesh, uint32_t submeshIndex, const uint32_t* compressedTangentW)
{
	//__builtin_snpause();
	ClothingAssetSubMesh* pSubMesh = mAsset.GetSubmesh(&rendermesh, submeshIndex);

	if (withNormals && withTangents)
	{
		computeTangentSpaceUpdate<true, false>(destMesh, rendermesh, submeshIndex, compressedTangentW);
		computeTangentSpaceUpdate<false, true>(destMesh, rendermesh, submeshIndex, compressedTangentW);
	}
	else
	{
		const RenderDataFormat::Enum uvFormat = pSubMesh->mUvFormat;

		if (uvFormat != RenderDataFormat::FLOAT2)
		{
			if (withNormals)
			{
				computeTangentSpaceUpdate<true, false>(destMesh, rendermesh, submeshIndex, compressedTangentW);
			}

			return;
		}

		PX_ASSERT(pSubMesh->mCurrentMaxIndexSimulation <= pSubMesh->mIndicesCount);
		const uint32_t numGraphicalVertexIndices =	pSubMesh->mCurrentMaxIndexSimulation;
		const uint32_t* indices =					pSubMesh->mIndices;

		const VertexUVLocal* PX_RESTRICT uvs = pSubMesh->mUvs;
		PX_ASSERT(uvs != NULL);

		const uint32_t numVertices = pSubMesh->mCurrentMaxVertexAdditionalSimulation;
		const uint32_t numZeroVertices = pSubMesh->mCurrentMaxVertexSimulation;
		PX_ASSERT(numVertices <= destMesh.numVertices);

		PX_ASSERT(pSubMesh->mVertexCount == destMesh.numVertices);
		PX_ASSERT(destMesh.pPosition != NULL);
		PX_ASSERT(destMesh.pNormal != NULL);
		PX_ASSERT(destMesh.pTangent4 != NULL || !withTangents);
		PX_ASSERT(destMesh.pTangent == NULL);
		PX_ASSERT(destMesh.pBitangent == NULL);

		const Simd4f vZero = gSimd4fZero;

		//All indices read in in blocks of 3...hence need to fetch in an exact multiple of 3...

		const uint32_t UnrollSize = 192; //exactly divisible by 16 AND 3 :-)

		const uint32_t numIterations = (numGraphicalVertexIndices + UnrollSize - 1) / UnrollSize;

		const PxVec3* PX_RESTRICT destPositions = (const PxVec3 * PX_RESTRICT)(destMesh.pPosition);

		if (withNormals)
		{
			//__builtin_snpause();
			PxVec3* PX_RESTRICT destNormals = destMesh.pNormal;
			for (uint32_t a = 0; a < numZeroVertices; ++a)
			{
				destNormals[a] = PxVec3(0.0f);
			}

			for (uint32_t a = 0; a < numIterations; ++a)
			{
				//__builtin_snpause();
				const uint32_t numToProcess = PxMin(numGraphicalVertexIndices - (a * UnrollSize), UnrollSize);
				const uint32_t* localIndices = (const uint32_t*)((void*)(indices + (a * UnrollSize)));

				for (uint32_t i = 0; i < numToProcess; i += 3)
				{
					const uint32_t i0 = localIndices[i + 0];
					const uint32_t i1 = localIndices[i + 1];
					const uint32_t i2 = localIndices[i + 2];

					const Simd4f P0 = createSimd3f(destPositions[i0]);
					const Simd4f P1 = createSimd3f(destPositions[i1]);
					const Simd4f P2 = createSimd3f(destPositions[i2]);

					const Simd4f X1 = P1 - P0;
					const Simd4f X2 = P2 - P0;

					Simd4f FACENORMAL = cross3(X1, X2);

					PxVec3* PX_RESTRICT nor1 = &destNormals[i0];
					Simd4f n1 = createSimd3f(*nor1);
					n1 = n1 + FACENORMAL;
					store3(&nor1->x, n1);

					PxVec3* PX_RESTRICT nor2 = &destNormals[i1];
					Simd4f n2 = createSimd3f(*nor2);
					n2 = n2 + FACENORMAL;
					store3(&nor2->x, n2);

					PxVec3* PX_RESTRICT nor3 = &destNormals[i2];
					Simd4f n3 = createSimd3f(*nor3);
					n3 = n3 + FACENORMAL;
					store3(&nor3->x, n3);

				}
			}
		}
		if (withTangents)
		{
			const VertexUVLocal* PX_RESTRICT uvLocal = (const VertexUVLocal * PX_RESTRICT)(void*)uvs;

			PxVec4* PX_RESTRICT tangents = destMesh.pTangent4;
			for (uint32_t a = 0; a < numZeroVertices; ++a)
			{
				tangents[a] = PxVec4(0.f);
			}


			for (uint32_t a = 0; a < numIterations; ++a)
			{
				//__builtin_snpause();
				const uint32_t numToProcess = PxMin(numGraphicalVertexIndices - (a * UnrollSize), UnrollSize);
				const uint32_t* localIndices = (const uint32_t*)(void*)(indices + (a * UnrollSize));

				for (uint32_t i = 0; i < numToProcess; i += 3)
				{
					const uint32_t i0 = localIndices[i + 0];
					const uint32_t i1 = localIndices[i + 1];
					const uint32_t i2 = localIndices[i + 2];

					const Simd4f P0 = createSimd3f(destPositions[i0]);
					const Simd4f P1 = createSimd3f(destPositions[i1]);
					const Simd4f P2 = createSimd3f(destPositions[i2]);

					const Simd4f X1 = P1 - P0;
					const Simd4f X2 = P2 - P0;

					const VertexUVLocal& w0 = uvLocal[i0];
					const VertexUVLocal& w1 = uvLocal[i1];
					const VertexUVLocal& w2 = uvLocal[i2];

					const Simd4f W0U = Simd4fScalarFactory(w0.u);
					const Simd4f W1U = Simd4fScalarFactory(w1.u);
					const Simd4f W2U = Simd4fScalarFactory(w2.u);
					const Simd4f W0V = Simd4fScalarFactory(w0.v);
					const Simd4f W1V = Simd4fScalarFactory(w1.v);
					const Simd4f W2V = Simd4fScalarFactory(w2.v);

					//This could be just 1 sub...

					const Simd4f S1 = W1U - W0U;
					const Simd4f S2 = W2U - W0U;
					const Simd4f T1 = W1V - W0V;
					const Simd4f T2 = W2V - W0V;

					// invH = (s1 * t2 - s2 * t1);			
					const Simd4f S1T2 = S1 * T2;
					const Simd4f invHR = S1T2 - S2 * T1;
					const Simd4f HR = recip(invHR);
					const Simd4f T2X1 = X1 * T2;
					//const Vec3V S1X2 = V3Scale(X2, S1);
					const Simd4f invHREqZero = (invHR == vZero);

					const Simd4f T1X2MT2X1 = T2X1 - X2 * T1;
					//const Simd4f S2X1MS1X2 = S1X2 - X1 * X2;

					const Simd4f scale = select(invHREqZero, vZero, HR);

					const Simd4f SDIR = T1X2MT2X1 * scale; // .w gets overwritten later on
					//const Simd4f TDIR = S2X1MS1X2 * scale;

					PxVec4* PX_RESTRICT tangent0 = tangents + i0;
					PxVec4* PX_RESTRICT tangent1 = tangents + i1;
					PxVec4* PX_RESTRICT tangent2 = tangents + i2;
					Simd4f t0 = Simd4fAlignedLoadFactory((float*)tangent0);
					Simd4f t1 = Simd4fAlignedLoadFactory((float*)tangent1);
					Simd4f t2 = Simd4fAlignedLoadFactory((float*)tangent2);

					t0 = t0 + SDIR;
					t1 = t1 + SDIR;
					t2 = t2 + SDIR;

					storeAligned((float*)tangent0, t0);
					storeAligned((float*)tangent1, t1);
					storeAligned((float*)tangent2, t2);
				}
			}

			uint32_t tangentW = 0;

			int32_t j = 0;
#if 1
			// This makes it quite a bit faster, but it also works without it.
			for (; j < (int32_t)numVertices - 4; j += 4)
			{
				if ((j & 0x1f) == 0)
				{
					tangentW = compressedTangentW[j >> 5];
				}

				tangents[j].w = (tangentW & 0x1) ? 1.0f : -1.0f;
				tangents[j + 1].w = (tangentW & 0x2) ? 1.0f : -1.0f;
				tangents[j + 2].w = (tangentW & 0x4) ? 1.0f : -1.0f;
				tangents[j + 3].w = (tangentW & 0x8) ? 1.0f : -1.0f;
				tangentW >>= 4;
			}
#endif

			// We need this loop to handle last vertices in tangents[], it shares the same j as previous loop
			for (; j < (int32_t)numVertices; j++)
			{
				if ((j & 0x1f) == 0)
				{
					tangentW = compressedTangentW[j >> 5];
				}

				tangents[j].w = (tangentW & 0x1) ? 1.0f : -1.0f;
				tangentW >>= 1;
			}
		}
	}
}


PxBounds3 ClothingActorData::getRenderMeshAssetBoundsTransformed()
{
	PxBounds3 newBounds = mAsset.GetLod(mCurrentGraphicalLodId)->mBounds;

	PxMat44 transformation;
	if (mInternalBoneMatricesCur != NULL)
	{
		transformation = mInternalBoneMatricesCur[mAsset.mRootBoneIndex];
	}
	else
	{
		//transformation = mActorDesc->globalPose;
		transformation = mGlobalPose;
	}

	if (!newBounds.isEmpty())
	{
		PxVec3 center = transformation.transform(newBounds.getCenter());
		PxVec3 extent = newBounds.getExtents();

		// extended basis vectors
		PxVec3 c0 = transformation.column0.getXYZ() * extent.x;
		PxVec3 c1 = transformation.column1.getXYZ() * extent.y;
		PxVec3 c2 = transformation.column2.getXYZ() * extent.z;

		// find combination of base vectors that produces max. distance for each component = sum of PxAbs()
		extent.x = PxAbs(c0.x) + PxAbs(c1.x) + PxAbs(c2.x);
		extent.y = PxAbs(c0.y) + PxAbs(c1.y) + PxAbs(c2.y);
		extent.z = PxAbs(c0.z) + PxAbs(c1.z) + PxAbs(c2.z);

		return PxBounds3::centerExtents(center, extent);
	}
	else
	{
		return newBounds;
	}
}


void ClothingActorData::tickSynchAfterFetchResults_LocksPhysX()
{
	if (bIsInitialized && !bIsClothingSimulationNull && bShouldComputeRenderData /*&& !bInternalFrozen*/)
	{
		// overwrite a few writeback normals!

		if (bCorrectSimulationNormals)
		{
			skinPhysicsMaxDist0Normals_NoPhysx();
		}

		//// perform mesh-to-mesh skinning if using skin cloth

		if (!bParallelCpuSkinning)
		{
			skinToAnimation_NoPhysX(true);
		}

		skinToPhysicalMesh_NoPhysX(true);

		finalizeSkinning_NoPhysX(true);

		PX_ASSERT(!mNewBounds.isEmpty());
		PX_ASSERT(mNewBounds.isFinite());
	}
}


bool ClothingActorData::calcIfSimplePhysicsMesh() const
{
	// this number is the blocksize in SPU_ClothSkinPhysicsSimple.spu.cpp
	return skinPhysicsSimpleMem() < BLOCK_SIZE_SKIN_PHYSICS;

	// with
	// BLOCK_SIZE_SKIN_PHYSICS (32768*6)
	// 100 bones
	// 4 bone indices per vertex
	// => simple mesh is vertexCount < 3336
}


uint32_t ClothingActorData::skinPhysicsSimpleMem() const
{
	PX_ASSERT(bIsInitialized);

	const ClothingPhysicalMeshData* physicalMesh = mAsset.GetPhysicalMeshFromLod(mCurrentGraphicalLodId);
	PX_ASSERT(physicalMesh != NULL);

	const uint32_t numVertices = physicalMesh->mSimulatedVertexCount;
	const uint32_t numBoneIndicesPerVertex = physicalMesh->mNumBonesPerVertex;

	uint32_t srcPositionMem = numVertices * sizeof(PxVec3);
	uint32_t srcNormalMem = numVertices * sizeof(PxVec3);

	uint32_t simBoneIndicesMem = numBoneIndicesPerVertex * numVertices * sizeof(uint16_t);
	uint32_t simBoneWeightsMem = numBoneIndicesPerVertex * numVertices * sizeof(float);

	uint32_t matricesMem = mInternalMatricesCount * sizeof(PxMat44);

	uint32_t optimizationDataMem = physicalMesh->mOptimizationDataCount * sizeof(uint8_t); // mOptimizationDataCount ~ numVertices

	uint32_t mem = srcPositionMem + srcNormalMem + simBoneIndicesMem + simBoneWeightsMem + matricesMem + optimizationDataMem;
	// numVertices * (33 + (6*numBonesPerVert)) + 64*numBones

	return mem;
}


void ClothingActorData::skinPhysicsMeshSimple()
{
	if (!bIsInitialized)
	{
		return;
	}

	// with bones, no interpolated matrices, no backstop?

	// data
	const ClothingPhysicalMeshData* physicalMesh = mAsset.GetPhysicalMeshFromLod(mCurrentGraphicalLodId);
	PX_ASSERT(physicalMesh != NULL);

	const uint32_t numVertices = physicalMesh->mSimulatedVertexCount;
	const uint32_t numBoneIndicesPerVertex = physicalMesh->mNumBonesPerVertex;

	PxVec3* const PX_RESTRICT eaPositions = physicalMesh->mVertices;
	PxVec3* const PX_RESTRICT positions = (PxVec3*)eaPositions;

	PxVec3* const PX_RESTRICT eaNormals = physicalMesh->mNormals;
	PxVec3* const PX_RESTRICT normals = (PxVec3*)eaNormals;

	PxVec3* const PX_RESTRICT targetPositions = mSkinnedPhysicsPositions;
	PxVec3* const PX_RESTRICT targetNormals = mSkinnedPhysicsNormals;

	uint16_t* const PX_RESTRICT eaSimBoneIndices = physicalMesh->mBoneIndices;
	const uint16_t* const PX_RESTRICT simBoneIndices = (uint16_t*)eaSimBoneIndices;

	float* const PX_RESTRICT eaSimBoneWeights = physicalMesh->mBoneWeights;
	const float* const PX_RESTRICT simBoneWeights = (float*)eaSimBoneWeights;

	PxMat44* eaMatrices = mInternalBoneMatricesCur; // TODO interpolated matrices?
	const PxMat44* matrices = (PxMat44*)eaMatrices;

	uint8_t* const PX_RESTRICT eaOptimizationData = physicalMesh->mOptimizationData;
	const uint8_t* const PX_RESTRICT optimizationData = (uint8_t*)eaOptimizationData;

	PX_ASSERT(optimizationData != NULL);

	for (uint32_t vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex)
	{
		Simd4f positionV = gSimd4fZero;
		Simd4f normalV = gSimd4fZero;

		const uint8_t shift = 4 * (vertexIndex % 2);
		const uint8_t numBones = uint8_t((optimizationData[vertexIndex / 2] >> shift) & 0x7);
		for (uint32_t k = 0; k < numBones; k++)
		{
			const float weight = simBoneWeights[vertexIndex * numBoneIndicesPerVertex + k];

			PX_ASSERT(weight <= 1.0f);

			//sumWeights += weight;
			Simd4f weightV = Simd4fScalarFactory(weight);

			const uint32_t index = simBoneIndices[vertexIndex * numBoneIndicesPerVertex + k];
			PX_ASSERT(index < mInternalMatricesCount);

			/// PH: This might be faster without the reference, but on PC I can't tell
			/// HL: Now with SIMD it's significantly faster as reference
			const PxMat44& bone = (PxMat44&)matrices[index];

			Simd4f pV = applyAffineTransform(bone, createSimd3f(positions[vertexIndex]));
			pV = pV * weightV;		
			positionV = positionV + pV;

			///todo There are probably cases where we don't need the normal on the physics mesh
			Simd4f nV = applyLinearTransform(bone, createSimd3f(normals[vertexIndex]));
			nV = nV * weightV;
			normalV = normalV + nV;
		}

		normalV = normalizeSimd3f(normalV);
		store3(&targetNormals[vertexIndex].x, normalV);
		store3(&targetPositions[vertexIndex].x, positionV);
	}
}


}
} // namespace nvidia
