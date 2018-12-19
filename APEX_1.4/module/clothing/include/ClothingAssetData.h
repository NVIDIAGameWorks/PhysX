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



#ifndef CLOTHING_ASSET_DATA
#define CLOTHING_ASSET_DATA

#include "PxSimpleTypes.h"
#include "PxAssert.h"
#include "AbstractMeshDescription.h"
#include "RenderDataFormat.h"
#include "PxBounds3.h"
#include "PsAllocator.h"
#include "RenderMeshAsset.h"

namespace nvidia
{
namespace clothing
{

// forward declarations
namespace ClothingGraphicalLodParametersNS
{
struct SkinClothMapB_Type;
struct SkinClothMapD_Type;
struct TetraLink_Type;
}

}

//typedef ClothingGraphicalLodParametersNS::SkinClothMapB_Type SkinClothMapB_TypeLocal;
//typedef ClothingGraphicalLodParametersNS::SkinClothMapC_Type SkinClothMapC_TypeLocal;
//typedef ClothingGraphicalLodParametersNS::TetraLink_Type TetraLink_TypeLocal;

struct VertexUVLocal
{
	VertexUVLocal() {}
	VertexUVLocal(float _u, float _v)
	{
		set(_u, _v);
	}
	VertexUVLocal(const float uv[])
	{
		set(uv);
	}

	void			set(float _u, float _v)
	{
		u = _u;
		v = _v;
	}

	void			set(const float uv[])
	{
		u = uv[0];
		v = uv[1];
	}

	float&			operator [](int i)
	{
		PX_ASSERT(i >= 0 && i <= 1);
		return (&u)[i];
	}

	const float&	operator [](int i) const
	{
		PX_ASSERT(i >= 0 && i <= 1);
		return (&u)[i];
	}

	float	u, v;
};

PX_COMPILE_TIME_ASSERT(sizeof(VertexUVLocal) == sizeof(VertexUV));

struct TetraEncoding_Local
{
	float sign[4];
	uint32_t lastVtxIdx;
};

#define TETRA_LUT_SIZE_LOCAL 6
static const TetraEncoding_Local tetraTableLocal[TETRA_LUT_SIZE_LOCAL] =
{
	{ {0,  0, 0, 1}, 0},
	{ {1,  0, 0, 1}, 2},
	{ {1,  0, 1, 1}, 1},

	{ { -1, -1, -1, 0}, 0},
	{ {0, -1, -1, 0}, 2 },
	{ {0, -1,  0, 0}, 1 }
};


namespace clothing
{

//This seems to be the data we are interested in in each submesh...
class ClothingAssetSubMesh
{

public:
	ClothingAssetSubMesh();

	const PxVec3* PX_RESTRICT mPositions;
	const PxVec3* PX_RESTRICT mNormals;
	const PxVec4* PX_RESTRICT mTangents;

	const float* PX_RESTRICT mBoneWeights;
	const uint16_t* PX_RESTRICT mBoneIndices;

	const uint32_t* PX_RESTRICT mIndices;

	VertexUVLocal* mUvs;

	RenderDataFormat::Enum mPositionOutFormat;
	RenderDataFormat::Enum mNormalOutFormat;
	RenderDataFormat::Enum mTangentOutFormat;
	RenderDataFormat::Enum mBoneWeightOutFormat;
	RenderDataFormat::Enum mUvFormat;

	uint32_t mVertexCount;
	uint32_t mIndicesCount;
	uint32_t mUvCount;
	uint32_t mNumBonesPerVertex;

	uint32_t mCurrentMaxVertexSimulation;
	uint32_t mCurrentMaxVertexAdditionalSimulation;
	uint32_t mCurrentMaxIndexSimulation;
};

class ClothingMeshAssetData
{
public:

	ClothingMeshAssetData();

	const uint32_t* mImmediateClothMap;
	ClothingGraphicalLodParametersNS::SkinClothMapD_Type* mSkinClothMap;
	ClothingGraphicalLodParametersNS::SkinClothMapB_Type* mSkinClothMapB;
	ClothingGraphicalLodParametersNS::TetraLink_Type* mTetraMap;

	uint32_t mImmediateClothMapCount;
	uint32_t mSkinClothMapCount;
	uint32_t mSkinClothMapBCount;
	uint32_t mTetraMapCount;

	uint32_t mSubmeshOffset;
	uint32_t mSubMeshCount;

	//Index of the physics mesh this lod relates to
	uint32_t mPhysicalMeshId;

	float mSkinClothMapThickness;
	float mSkinClothMapOffset;

	PxBounds3 mBounds;

	bool bActive;
	bool bNeedsTangents;
};


//A physical clothing mesh
class ClothingPhysicalMeshData
{
public:
	ClothingPhysicalMeshData();

	PxVec3*		mVertices;
	uint32_t	mVertexCount;
	uint32_t	mSimulatedVertexCount;
	uint32_t	mMaxDistance0VerticesCount;

	PxVec3*		mNormals;
	PxVec3*		mSkinningNormals;
	uint16_t*	mBoneIndices;
	float*		mBoneWeights;
	uint8_t*	mOptimizationData;
	uint32_t*	mIndices;

	uint32_t	mSkinningNormalsCount;
	uint32_t	mBoneWeightsCount;
	uint32_t	mOptimizationDataCount;
	uint32_t	mIndicesCount;
	uint32_t	mSimulatedIndicesCount;

	uint32_t	mNumBonesPerVertex;
};

//A clothing asset contains a set of submeshes + some other data that we might be interested in...
class ClothingAssetData
{
public:
	ClothingAssetData();
	~ClothingAssetData();

	uint8_t* mData;
	uint32_t* mCompressedNumBonesPerVertex;
	uint32_t* mCompressedTangentW;
	uint32_t* mExt2IntMorphMapping;
	uint32_t mCompressedNumBonesPerVertexCount;
	uint32_t mCompressedTangentWCount;
	uint32_t mExt2IntMorphMappingCount;

	uint32_t mAssetSize;
	uint32_t mGraphicalLodsCount;
	uint32_t mPhysicalMeshesCount;
	uint32_t mPhysicalMeshOffset;


	uint32_t mBoneCount;

	uint32_t mRootBoneIndex;

	ClothingMeshAssetData* GetLod(const uint32_t lod) const
	{
		return ((ClothingMeshAssetData*)mData) + lod;
	}

	ClothingAssetSubMesh* GetSubmesh(const uint32_t lod, const uint32_t submeshIndex) const
	{
		return GetSubmesh(GetLod(lod), submeshIndex);
	}

	ClothingAssetSubMesh* GetSubmesh(const ClothingMeshAssetData* asset, const uint32_t submeshIndex) const
	{
		return ((ClothingAssetSubMesh*)(mData + asset->mSubmeshOffset)) + submeshIndex;
	}

	ClothingPhysicalMeshData* GetPhysicalMesh(const uint32_t index) const
	{
		return ((ClothingPhysicalMeshData*)(mData + mPhysicalMeshOffset)) + index;
	}

	const uint32_t* getCompressedNumBonesPerVertex(uint32_t graphicalLod, uint32_t submeshIndex, uint32_t& mapSize);
	const uint32_t* getCompressedTangentW(uint32_t graphicalLod, uint32_t submeshIndex, uint32_t& mapSize);

	uint32_t* getMorphMapping(uint32_t graphicalLod);

	template<bool withNormals, bool withBones, bool withMorph, bool withTangents>
	void skinToBonesInternal(AbstractMeshDescription& destMesh, uint32_t submeshIndex, uint32_t graphicalMeshIndex, uint32_t startVertex,
	                         PxMat44* compositeMatrices, PxVec3* morphDisplacements);

	void skinToBones(AbstractMeshDescription& destMesh, uint32_t submeshIndex, uint32_t graphicalMeshIndex, uint32_t startVertex,
	                 PxMat44* compositeMatrices, PxVec3* morphDisplacements);

	template<bool computeNormals>
	uint32_t skinClothMap(PxVec3* dstPositions, PxVec3* dstNormals, PxVec4* dstTangents, uint32_t numVertices,
	                    const AbstractMeshDescription& srcPM, ClothingGraphicalLodParametersNS::SkinClothMapD_Type* map,
	                    uint32_t numVerticesInMap, float offsetAlongNormal, float actorScale) const;

	void getNormalsAndVerticesForFace(PxVec3* vtx, PxVec3* nrm, uint32_t i, const AbstractMeshDescription& srcPM) const;

	uint32_t skinClothMapBSkinVertex(PxVec3& dstPos, PxVec3* dstNormal, uint32_t vIndex,
	                              ClothingGraphicalLodParametersNS::SkinClothMapB_Type* pTCMB, const ClothingGraphicalLodParametersNS::SkinClothMapB_Type* pTCMBEnd,
	                              const AbstractMeshDescription& srcPM) const;

	uint32_t skinClothMapB(PxVec3* dstPositions, PxVec3* dstNormals, uint32_t numVertices,
	                    const AbstractMeshDescription& srcPM, ClothingGraphicalLodParametersNS::SkinClothMapB_Type* map,
	                    uint32_t numVerticesInMap, bool computeNormals) const;

	bool skinToTetraMesh(AbstractMeshDescription& destMesh,
	                     const AbstractMeshDescription& srcPM,
	                     const ClothingMeshAssetData& graphicalLod);

	ClothingPhysicalMeshData* GetPhysicalMeshFromLod(const uint32_t graphicalLod) const
	{
		PX_ASSERT(graphicalLod < mGraphicalLodsCount);
		const uint32_t physicalMeshId = GetLod(graphicalLod)->mPhysicalMeshId;
		PX_ASSERT(physicalMeshId < mPhysicalMeshesCount);
		return GetPhysicalMesh(physicalMeshId);
	}

};

}
} // namespace nvidia

#endif // CLOTHING_ASSET_DATA
