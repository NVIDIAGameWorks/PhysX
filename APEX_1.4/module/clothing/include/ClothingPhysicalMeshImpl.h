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



#ifndef CLOTHING_PHYSICAL_MESH_IMPL_H
#define CLOTHING_PHYSICAL_MESH_IMPL_H

#include "ClothingPhysicalMesh.h"
#include "PsUserAllocated.h"
#include "ApexResource.h"
#include "PsArray.h"
#include "ParamArray.h"
#include "ClothingPhysicalMeshParameters.h"
#include "ModuleClothingImpl.h"
#include "ApexRWLockable.h"
#include "nvparameterized/NvParameterized.h"
#include "ReadCheck.h"

namespace nvidia
{
namespace apex
{
class ApexQuadricSimplifier;
}
namespace clothing
{
class ModuleClothingImpl;


class ClothingPhysicalMeshImpl : public ClothingPhysicalMesh, public ApexResourceInterface, public ApexResource, public NvParameterized::SerializationCallback, public ApexRWLockable
{
private:
	ClothingPhysicalMeshImpl(ModuleClothingImpl* module, ClothingPhysicalMeshParameters* params, ResourceList* list);
	friend class ModuleClothingImpl;

public:
	APEX_RW_LOCKABLE_BOILERPLATE

	virtual void release();
	void destroy();

	virtual uint32_t getNumVertices() const;
	virtual uint32_t getNumSimulatedVertices() const;
	virtual uint32_t getNumMaxDistance0Vertices() const;

	virtual uint32_t getNumIndices() const;
	virtual uint32_t getNumSimulatedIndices() const;

	virtual uint32_t getNumBonesPerVertex() const
	{
		READ_ZONE();
		return mParams->physicalMesh.numBonesPerVertex;
	}

	virtual void getIndices(void* indexDestination, uint32_t byteStride, uint32_t numIndices) const;

	virtual bool isTetrahedralMesh() const
	{
		READ_ZONE();
		return mParams->physicalMesh.isTetrahedralMesh;
	}

	virtual void simplify(uint32_t subdivisions, int32_t maxSteps, float maxError, IProgressListener* progress);

	// user overwrites geometry
	virtual void setGeometry(bool tetraMesh, uint32_t numVertices, uint32_t vertexByteStride, const void* vertices, const uint32_t* masterFlags, 
							uint32_t numIndices, uint32_t indexByteStride, const void* indices);

	// direct access to specific buffers
	virtual bool getIndices(uint32_t* indices, uint32_t byteStride) const;
	virtual bool getVertices(PxVec3* vertices, uint32_t byteStride) const ;
	virtual bool getNormals(PxVec3* vertices, uint32_t byteStride) const ;
	virtual bool getBoneIndices(uint16_t* boneIndices, uint32_t byteStride) const;
	virtual bool getBoneWeights(float* boneWeights, uint32_t byteStride) const;
	virtual bool getConstrainCoefficients(ClothingConstrainCoefficients* values, uint32_t byteStride) const;
	virtual void getStats(ClothingPhysicalMeshStats& stats) const;

	virtual void allocateMasterFlagsBuffer();
	virtual void allocateConstrainCoefficientBuffer();
	virtual ClothingConstrainCoefficients* getConstrainCoefficientBuffer() const
	{
		READ_ZONE();
		PX_COMPILE_TIME_ASSERT(sizeof(ClothingConstrainCoefficients) == sizeof(ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type));
		return (ClothingConstrainCoefficients*)mParams->physicalMesh.constrainCoefficients.buf;
	}

	// from NvParameterized::SerializationCallback
	void preSerialize(void* userData_ = NULL);

	void permuteBoneIndices(Array<int32_t>& old2newBoneIndices);
	void applyTransformation(const PxMat44& transformation, float scale);
	void applyPermutation(const Array<uint32_t>& permutation);

	void makeCopy(ClothingPhysicalMeshParameters* params);
	void allocateNormalBuffer();
	void allocateSkinningNormalsBuffer();
	void allocateBoneIndexAndWeightBuffers();
	void freeAdditionalBuffers();
	PX_INLINE void setNumBonesPerVertex(uint32_t numBonesPerVertex)
	{
		mParams->physicalMesh.numBonesPerVertex = numBonesPerVertex;
	}

	PX_INLINE uint16_t* getBoneIndicesBuffer()
	{
		return mParams->physicalMesh.boneIndices.buf;
	}
	PX_INLINE float* getBoneWeightsBuffer()
	{
		return mParams->physicalMesh.boneWeights.buf;
	}
	PX_INLINE PxVec3* getPositionBuffer()
	{
		return mParams->physicalMesh.vertices.buf;
	}
	PX_INLINE PxVec3* getNormalBuffer()
	{
		return mParams->physicalMesh.normals.buf;
	}
	PX_INLINE PxVec3* getSkinningNormalBuffer()
	{
		return mParams->physicalMesh.skinningNormals.buf;
	}
	PX_INLINE uint32_t* getMasterFlagsBuffer()
	{
		return mMasterFlags.begin();
	}

	PX_INLINE uint32_t* getIndicesBuffer()
	{
		return mParams->physicalMesh.indices.buf;
	}

	PX_INLINE float getMaxMaxDistance()
	{
		return mParams->physicalMesh.maximumMaxDistance;
	}
	void updateMaxMaxDistance();

	void addBoneToVertex(uint32_t vertexNumber, uint16_t boneIndex, float boneWeight);
	void sortBonesOfVertex(uint32_t vertexNumber);
	void normalizeBonesOfVertex(uint32_t vertexNumber);

	void updateSkinningNormals();
	void smoothNormals(uint32_t numIterations);

	void updateOptimizationData();

	ClothingPhysicalMeshParameters* getNvParameterized() const
	{
		return mParams;
	}

	// from ApexResource
	uint32_t	getListIndex() const
	{
		return m_listIndex;
	}
	void	setListIndex(class ResourceList& list, uint32_t index)
	{
		m_list = &list;
		m_listIndex = index;
	}

private:
	void writeBackData();
	void clearMiscBuffers();
	void computeEdgeLengths() const;

	bool removeDuplicatedTriangles(uint32_t numIndices, uint32_t indexByteStride, const void* indices);
	void computeNeighborInformation(Array<int32_t> &neighbors);
	void fixTriangleOrientations();

	ModuleClothingImpl* mModule;

	ClothingPhysicalMeshParameters* mParams;
	bool ownsParams;

	ParamArray<PxVec3> mVertices;
	ParamArray<PxVec3> mNormals;
	ParamArray<PxVec3> mSkinningNormals;
	ParamArray<ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type> mConstrainCoefficients;
	ParamArray<uint16_t> mBoneIndices;
	ParamArray<float> mBoneWeights;
	ParamArray<uint32_t> mIndices;

	uint32_t mNumSimulatedVertices;
	uint32_t mNumMaxDistanc0Vertices;
	uint32_t mNumSimulatedIndices;

	Array<uint32_t> mMasterFlags;

	ApexQuadricSimplifier* mSimplifier;
	bool isDirty;
};

}
} // namespace nvidia

#endif // CLOTHING_PHYSICAL_MESH_IMPL_H
