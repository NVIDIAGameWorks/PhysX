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



#include "ApexDefs.h"

#include "ClothingPhysicalMeshImpl.h"
#include "ApexMeshHash.h"
#include "ApexQuadricSimplifier.h"
#include "ClothingAssetAuthoringImpl.h"
#include "ModuleClothingImpl.h"

#include "ApexPermute.h"
#include "ApexSharedUtils.h"

#include "PxStrideIterator.h"
#include "PsMathUtils.h"
#include "PsSort.h"

namespace nvidia
{
namespace clothing
{

struct SortedEdge
{
	SortedEdge(uint32_t _i0, uint32_t _i1) : i0(PxMin(_i0, _i1)), i1(PxMax(_i0, _i1)) {}

	bool operator==(const SortedEdge& other) const
	{
		return i0 == other.i0 && i1 == other.i1;
	}
	bool operator()(const SortedEdge& s1, const SortedEdge& s2) const
	{
		if (s1.i0 != s2.i0)
		{
			return s1.i0 < s2.i0;
		}

		return s1.i1 < s2.i1;
	}

	uint32_t i0, i1;
};

ClothingPhysicalMeshImpl::ClothingPhysicalMeshImpl(ModuleClothingImpl* module, ClothingPhysicalMeshParameters* params, ResourceList* list) :
	mModule(module),
	mParams(NULL),
	ownsParams(false),
	mSimplifier(NULL),
	isDirty(false)
{
	if (params != NULL && nvidia::strcmp(params->className(), ClothingPhysicalMeshParameters::staticClassName()) != 0)
	{
		APEX_INTERNAL_ERROR(
		    "The parameterized interface is of type <%s> instead of <%s>.  "
		    "An empty ClothingPhhsicalMesh has been created instead.",
		    params->className(),
		    ClothingPhysicalMeshParameters::staticClassName());

		params = NULL;
	}

	if (params == NULL)
	{
		params = DYNAMIC_CAST(ClothingPhysicalMeshParameters*)(GetInternalApexSDK()->getParameterizedTraits()->createNvParameterized(ClothingPhysicalMeshParameters::staticClassName()));
		ownsParams = true;
	}
	PX_ASSERT(params != NULL);

	mParams = params;
	mVertices.init(mParams, "physicalMesh.vertices", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->physicalMesh.vertices));
	mNormals.init(mParams, "physicalMesh.normals", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->physicalMesh.normals));
	mSkinningNormals.init(mParams, "physicalMesh.skinningNormals", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->physicalMesh.skinningNormals));
	mConstrainCoefficients.init(mParams, "physicalMesh.constrainCoefficients", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->physicalMesh.constrainCoefficients));
	mBoneIndices.init(mParams, "physicalMesh.boneIndices", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->physicalMesh.boneIndices));
	mBoneWeights.init(mParams, "physicalMesh.boneWeights", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->physicalMesh.boneWeights));
	mIndices.init(mParams, "physicalMesh.indices", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->physicalMesh.indices));

	mNumSimulatedVertices = mParams->physicalMesh.numSimulatedVertices;
	mNumMaxDistanc0Vertices = mParams->physicalMesh.numMaxDistance0Vertices;
	mNumSimulatedIndices = mParams->physicalMesh.numSimulatedIndices;

	mParams->referenceCount++;

	mParams->setSerializationCallback(this);
#if 0
	// debugging only
	char buf[32];
	sprintf_s(buf, 32, "++ %p -> %d\n", mParams, mParams->referenceCount);
	OutputDebugString(buf);
#endif

	if (mParams->physicalMesh.shortestEdgeLength == 0.0f)
	{
		computeEdgeLengths();
	}

	list->add(*this);
}



void ClothingPhysicalMeshImpl::release()
{
	PX_ASSERT(mParams != NULL);
	if (mParams != NULL)
	{
		// make sure everything is set up correctly before we let the param object live on its own
		preSerialize(NULL);

		mParams->setSerializationCallback(NULL);
	}
	mModule->releasePhysicalMesh(this);
}



void ClothingPhysicalMeshImpl::destroy()
{
	if (mSimplifier != NULL)
	{
		delete mSimplifier;
		mSimplifier = NULL;
	}

	if (mParams != NULL)
	{
		mParams->referenceCount--;
#if 0
		// debugging only
		char buf[32];
		sprintf_s(buf, 32, "-- %p -> %d\n", mParams, mParams->referenceCount);
		OutputDebugString(buf);
#endif
	}

	if (ownsParams && mParams)
	{
		PX_ASSERT(mParams->referenceCount == 0);
		mParams->destroy();
	}

	delete this;
}



void ClothingPhysicalMeshImpl::makeCopy(ClothingPhysicalMeshParameters* params)
{
	PX_ASSERT(mParams != NULL);
	params->copy(*mParams);
}



void ClothingPhysicalMeshImpl::allocateNormalBuffer()
{
	mNormals.resize(mParams->physicalMesh.numVertices);
}



void ClothingPhysicalMeshImpl::allocateSkinningNormalsBuffer()
{
	mSkinningNormals.resize(mParams->physicalMesh.numVertices);
}


void ClothingPhysicalMeshImpl::allocateMasterFlagsBuffer()
{
	uint32_t numVertices = mVertices.size();
	mMasterFlags.resize(numVertices);

	for (uint32_t i = 0; i < numVertices; i++)
	{
		mMasterFlags[i] = 0xffffffff;
	}
}


void ClothingPhysicalMeshImpl::allocateConstrainCoefficientBuffer()
{
	WRITE_ZONE();
	const uint32_t numVertices = mParams->physicalMesh.numVertices;
	mConstrainCoefficients.resize(numVertices);

	for (uint32_t i = 0; i < numVertices; i++)
	{
		mConstrainCoefficients[i].maxDistance = PX_MAX_F32;
		mConstrainCoefficients[i].collisionSphereRadius = PX_MAX_F32;
		mConstrainCoefficients[i].collisionSphereDistance = PX_MAX_F32;
	}
}



void ClothingPhysicalMeshImpl::allocateBoneIndexAndWeightBuffers()
{
	const uint32_t numBonesPerVertex = mParams->physicalMesh.numBonesPerVertex;
	if (numBonesPerVertex == 0)
	{
		APEX_DEBUG_WARNING("Number of bones per vertex is set to 0. Not allocating memory.");
		return;
	}
	const uint32_t numVertices = mParams->physicalMesh.numVertices;

	mBoneIndices.resize(numBonesPerVertex * numVertices);

	// PH: At one point we can start trying to safe this buffer
	//if (numBonesPerVertex > 1)
	{
		mBoneWeights.resize(numBonesPerVertex * numVertices);
	}
}



void ClothingPhysicalMeshImpl::freeAdditionalBuffers()
{

	mNormals.resize(0);
	mSkinningNormals.resize(0);
	mConstrainCoefficients.resize(0);
	mBoneIndices.resize(0);
	mBoneWeights.resize(0);
	mParams->physicalMesh.numBonesPerVertex = 0;
}



uint32_t ClothingPhysicalMeshImpl::getNumVertices() const
{
	READ_ZONE();
	if (mSimplifier != NULL)
	{
		return mSimplifier->getNumVertices() - mSimplifier->getNumDeletedVertices();
	}

	return mParams->physicalMesh.numVertices;
}



uint32_t ClothingPhysicalMeshImpl::getNumSimulatedVertices() const
{
	READ_ZONE();
	return mParams->physicalMesh.numSimulatedVertices;
}



uint32_t ClothingPhysicalMeshImpl::getNumMaxDistance0Vertices() const
{
	READ_ZONE();
	return mParams->physicalMesh.numMaxDistance0Vertices;
}



uint32_t ClothingPhysicalMeshImpl::getNumIndices() const
{
	READ_ZONE();
	if (mSimplifier != NULL)
	{
		return mSimplifier->getNumTriangles() * 3;
	}

	return mParams->physicalMesh.numIndices;
}



uint32_t ClothingPhysicalMeshImpl::getNumSimulatedIndices() const
{
	READ_ZONE();
	if (mSimplifier != NULL)
	{
		return mSimplifier->getNumTriangles() * 3;
	}

	return mParams->physicalMesh.numSimulatedIndices;
}



void ClothingPhysicalMeshImpl::getIndices(void* indexDestination, uint32_t byteStride, uint32_t numIndices) const
{
	READ_ZONE();
	numIndices = PxMin(numIndices, mParams->physicalMesh.numIndices);

	if (byteStride == 0)
	{
		byteStride = sizeof(uint32_t);
	}

	if (byteStride < sizeof(uint32_t))
	{
		APEX_INTERNAL_ERROR("byte stride is too small (%d)", byteStride);
		return;
	}

	const_cast<ClothingPhysicalMeshImpl*>(this)->writeBackData();

	uint8_t* destPtr = (uint8_t*)indexDestination;
	for (uint32_t i = 0; i < numIndices; i++)
	{
		(uint32_t&)(*(destPtr + byteStride * i)) = mIndices[i];
	}
}



void ClothingPhysicalMeshImpl::simplify(uint32_t subdivisions, int32_t maxSteps, float maxError, IProgressListener* progressListener)
{
	WRITE_ZONE();
	if (mParams->physicalMesh.isTetrahedralMesh)
	{
		APEX_INVALID_OPERATION("Cannot simplify a tetrahedral mesh");
		return;
	}

	if (mParams->physicalMesh.boneIndices.buf != NULL || mParams->physicalMesh.boneWeights.buf != NULL)
	{
		APEX_INVALID_OPERATION("Cannot simplif a triangle mesh with additional bone data");
		return;
	}


	const uint32_t numVertices = mParams->physicalMesh.numVertices;
	const uint32_t numIndices = mParams->physicalMesh.numIndices;

	if (numVertices == 0 || numIndices == 0)
	{
		return;
	}

	HierarchicalProgressListener progress(100, progressListener);

	if (mSimplifier == NULL)
	{
		progress.setSubtaskWork(80, "Init simplificator");
		mSimplifier = PX_NEW(ApexQuadricSimplifier);

		for (uint32_t i = 0; i < numVertices; i++)
		{
			mSimplifier->registerVertex(mVertices[i]);
		}

		for (uint32_t i = 0; i < numIndices; i += 3)
		{
			mSimplifier->registerTriangle(mIndices[i + 0], mIndices[i + 1], mIndices[i + 2]);
		}

		mSimplifier->endRegistration(false, &progress);
		progress.completeSubtask();
	}

	progress.setSubtaskWork(-1, "Simplification steps");

	uint32_t steps = mSimplifier->simplify(subdivisions, maxSteps, maxError, &progress);

	if (!isDirty)
	{
		isDirty = steps > 0;
	}

	progress.completeSubtask();
}



void ClothingPhysicalMeshImpl::setGeometry(bool tetraMesh, uint32_t numVertices, uint32_t vertexByteStride, const void* vertices, 
										   const uint32_t* masterFlags, uint32_t numIndices, uint32_t indexByteStride, const void* indices)
{
	WRITE_ZONE();
	if (vertexByteStride < sizeof(PxVec3))
	{
		APEX_INTERNAL_ERROR("vertexByteStride is too small (%d)", vertexByteStride);
		return;
	}

	if (indexByteStride < sizeof(uint32_t))
	{
		APEX_INTERNAL_ERROR("indexByteStride is too small (%d)", indexByteStride);
		return;
	}

	if (numVertices > 0 && vertices == NULL)
	{
		APEX_INTERNAL_ERROR("vertex pointer is NULL");
		return;
	}

	if (numIndices > 0 && indices == NULL)
	{
		APEX_INTERNAL_ERROR("index pointer is NULL");
		return;
	}

	if (tetraMesh && (numIndices % 4 != 0))
	{
		APEX_INTERNAL_ERROR("Indices must be a multiple of 4 for physical tetrahedral meshes");
		return;
	}

	if (!tetraMesh && (numIndices % 3 != 0))
	{
		APEX_INTERNAL_ERROR("Indices must be a multiple of 3 for physical meshes");
		return;
	}

	mParams->physicalMesh.isTetrahedralMesh = tetraMesh;

	mParams->physicalMesh.numVertices = numVertices;
	mParams->physicalMesh.numSimulatedVertices = numVertices;
	mParams->physicalMesh.numMaxDistance0Vertices = 0;

	mVertices.resize(numVertices);
	mMasterFlags.resize(numVertices);

	mNormals.resize(0);
	mSkinningNormals.resize(0);

	const uint8_t* srcVertices = (const uint8_t*)vertices;
	for (uint32_t i = 0; i < numVertices; i++)
	{
		const PxVec3& currVec = *(const PxVec3*)(srcVertices + vertexByteStride * i);
		mVertices[i] = currVec;

		mMasterFlags[i] = masterFlags != NULL ? masterFlags[i] : 0xffffffff;
	}


	if (tetraMesh || !removeDuplicatedTriangles(numIndices, indexByteStride, indices))
	{
		mParams->physicalMesh.numIndices = numIndices;
		mParams->physicalMesh.numSimulatedIndices = numIndices;
		mIndices.resize(numIndices);

		const uint8_t* srcIndices = (const uint8_t*)indices;
		for (uint32_t i = 0; i < numIndices; i++)
		{
			const uint32_t currIndex = *(const uint32_t*)(srcIndices + indexByteStride * i);
			mIndices[i] = currIndex;
		}
	}

	if (mSimplifier != NULL)
	{
		delete mSimplifier;
		mSimplifier = NULL;
	}

	clearMiscBuffers();
	computeEdgeLengths();

	if (!mParams->physicalMesh.isTetrahedralMesh)
	{
		mSkinningNormals.resize(numVertices);
		updateSkinningNormals();
	}
}



bool ClothingPhysicalMeshImpl::getIndices(uint32_t* indices, uint32_t byteStride) const
{
	READ_ZONE();
	if (mIndices.size() == 0)
	{
		return false;
	}

	if (byteStride == 0)
	{
		byteStride = sizeof(uint32_t);
	}

	if (byteStride < sizeof(uint32_t))
	{
		APEX_INTERNAL_ERROR("bytestride too small (%d)", byteStride);
		return false;
	}

	const_cast<ClothingPhysicalMeshImpl*>(this)->writeBackData();

	const uint32_t numIndices = mParams->physicalMesh.numIndices;

	PxStrideIterator<uint32_t> iterator(indices, byteStride);
	for (uint32_t i = 0; i < numIndices; i++, ++iterator)
	{
		*iterator = mIndices[i];
	}

	return true;
}



bool ClothingPhysicalMeshImpl::getVertices(PxVec3* vertices, uint32_t byteStride) const
{
	READ_ZONE();
	if (mVertices.size() == 0)
	{
		return false;
	}

	if (byteStride == 0)
	{
		byteStride = sizeof(PxVec3);
	}

	if (byteStride < sizeof(PxVec3))
	{
		APEX_INTERNAL_ERROR("bytestride too small (%d)", byteStride);
		return false;
	}

	const_cast<ClothingPhysicalMeshImpl*>(this)->writeBackData();

	const uint32_t numVertices = mParams->physicalMesh.numVertices;

	PxStrideIterator<PxVec3> iterator(vertices, byteStride);
	for (uint32_t i = 0; i < numVertices; i++, ++iterator)
	{
		*iterator = mVertices[i];
	}

	return true;
}



bool ClothingPhysicalMeshImpl::getNormals(PxVec3* normals, uint32_t byteStride) const
{
	READ_ZONE();
	if (mNormals.size() == 0)
	{
		return false;
	}

	if (byteStride == 0)
	{
		byteStride = sizeof(PxVec3);
	}

	if (byteStride < sizeof(PxVec3))
	{
		APEX_INTERNAL_ERROR("bytestride too small (%d)", byteStride);
		return false;
	}

	const_cast<ClothingPhysicalMeshImpl*>(this)->writeBackData();

	const uint32_t numVertices = mParams->physicalMesh.numVertices;

	PxStrideIterator<PxVec3> iterator(normals, byteStride);
	for (uint32_t i = 0; i < numVertices; i++, ++iterator)
	{
		*iterator = mNormals[i];
	}

	return true;
}



bool ClothingPhysicalMeshImpl::getBoneIndices(uint16_t* boneIndices, uint32_t byteStride) const
{
	READ_ZONE();
	if (mBoneIndices.size() == 0)
	{
		return false;
	}

	const uint32_t numBonesPerVertex = mParams->physicalMesh.numBonesPerVertex;
	if (byteStride == 0)
	{
		byteStride = numBonesPerVertex * sizeof(uint16_t);
	}

	if (byteStride < numBonesPerVertex * sizeof(uint16_t))
	{
		APEX_INTERNAL_ERROR("bytestride too small (%d)", byteStride);
		return false;
	}

	const uint32_t numElements = mParams->physicalMesh.numVertices * numBonesPerVertex;

	PxStrideIterator<uint16_t> iterator(boneIndices, byteStride);
	for (uint32_t i = 0; i < numElements; i += numBonesPerVertex, ++iterator)
	{
		for (uint32_t j = 0; j < numBonesPerVertex; j++)
		{
			uint16_t* current = iterator.ptr();
			current[j] = mBoneIndices[i + j];
		}
	}

	return true;
}



bool ClothingPhysicalMeshImpl::getBoneWeights(float* boneWeights, uint32_t byteStride) const
{
	READ_ZONE();
	if (mBoneWeights.size() == 0)
	{
		return false;
	}

	const uint32_t numBonesPerVertex = mParams->physicalMesh.numBonesPerVertex;
	if (byteStride == 0)
	{
		byteStride = numBonesPerVertex * sizeof(float);
	}

	if (byteStride < numBonesPerVertex * sizeof(float))
	{
		APEX_INTERNAL_ERROR("bytestride too small (%d)", byteStride);
		return false;
	}

	const_cast<ClothingPhysicalMeshImpl*>(this)->writeBackData();

	const uint32_t numElements = mParams->physicalMesh.numVertices * numBonesPerVertex;

	PxStrideIterator<float> iterator(boneWeights, byteStride);
	for (uint32_t i = 0; i < numElements; i += numBonesPerVertex, ++iterator)
	{
		for (uint32_t j = 0; j < numBonesPerVertex; j++)
		{
			float* current = iterator.ptr();
			current[j] = mBoneWeights[i + j];
		}
	}


	return true;
}



bool ClothingPhysicalMeshImpl::getConstrainCoefficients(ClothingConstrainCoefficients* coeffs, uint32_t byteStride) const
{
	READ_ZONE();
	if (mConstrainCoefficients.size() == 0)
	{
		return false;
	}

	if (byteStride == 0)
	{
		byteStride = sizeof(ClothingConstrainCoefficients);
	}

	if (byteStride < sizeof(ClothingConstrainCoefficients))
	{
		APEX_INTERNAL_ERROR("bytestride too small (%d)", byteStride);
		return false;
	}

	const_cast<ClothingPhysicalMeshImpl*>(this)->writeBackData();

	const uint32_t numVertices = mParams->physicalMesh.numVertices;

	PxStrideIterator<ClothingConstrainCoefficients> iterator(coeffs, byteStride);
	for (uint32_t i = 0; i < numVertices; i++, ++iterator)
	{
		iterator->maxDistance = mConstrainCoefficients[i].maxDistance;
		iterator->collisionSphereDistance = mConstrainCoefficients[i].collisionSphereDistance;
		iterator->collisionSphereRadius = mConstrainCoefficients[i].collisionSphereRadius;
	}

	return true;
}



void ClothingPhysicalMeshImpl::getStats(ClothingPhysicalMeshStats& stats) const
{
	READ_ZONE();
	memset(&stats, 0, sizeof(ClothingPhysicalMeshStats));

	stats.totalBytes = sizeof(ClothingPhysicalMeshImpl);

	/*

	stats.numVertices = mNumVertices;
	stats.numIndices = mNumIndices;

	stats.totalBytes += (mVertices != NULL ? mNumVertices : 0) * sizeof(PxVec3);
	stats.totalBytes += (mNormals != NULL ? mNumVertices : 0) * sizeof(PxVec3);
	stats.totalBytes += (mSkinningNormals != NULL ? mNumVertices : 0) * sizeof(PxVec3);
	stats.totalBytes += (mVertexFlags != NULL ? mNumVertices : 0) * sizeof(uint32_t);
	stats.totalBytes += (mConstrainCoefficients != NULL ? mNumVertices : 0) * sizeof(ClothConstrainCoefficients);

	stats.totalBytes += (mBoneIndices != NULL ? mNumVertices * mNumBonesPerVertex : 0) * sizeof(uint16_t);
	stats.totalBytes += (mBoneWeights != NULL ? mNumVertices * mNumBonesPerVertex : 0) * sizeof(float);

	stats.totalBytes += (mIndices != NULL ? mNumIndices : 0) * sizeof(uint32_t);
	*/
}



void ClothingPhysicalMeshImpl::writeBackData()
{
	if (!isDirty || mSimplifier == NULL)
	{
		return;
	}

	isDirty = false;

	Array<int32_t> old2new(mSimplifier->getNumVertices());

	PX_ASSERT(mSimplifier->getNumVertices() - mSimplifier->getNumDeletedVertices() < mParams->physicalMesh.numVertices);

	uint32_t verticesWritten = 0;
	for (uint32_t i = 0; i < mSimplifier->getNumVertices(); i++)
	{
		PxVec3 pos;
		if (mSimplifier->getVertexPosition(i, pos))
		{
			old2new[i] = (int32_t)verticesWritten;
			mVertices[verticesWritten++] = pos;
		}
		else
		{
			old2new[i] = -1;
		}
	}
	PX_ASSERT(verticesWritten == (mSimplifier->getNumVertices() - mSimplifier->getNumDeletedVertices()));
	mParams->physicalMesh.numVertices = verticesWritten;

	PX_ASSERT(mSimplifier->getNumTriangles() * 3 < mParams->physicalMesh.numIndices);

	uint32_t indicesWritten = 0;
	uint32_t trianglesRead = 0;
	while (indicesWritten < mSimplifier->getNumTriangles() * 3)
	{
		uint32_t v0, v1, v2;
		if (mSimplifier->getTriangle(trianglesRead++, v0, v1, v2))
		{
			PX_ASSERT(old2new[v0] != -1);
			PX_ASSERT(old2new[v1] != -1);
			PX_ASSERT(old2new[v2] != -1);
			mIndices[indicesWritten++] = (uint32_t)old2new[v0];
			mIndices[indicesWritten++] = (uint32_t)old2new[v1];
			mIndices[indicesWritten++] = (uint32_t)old2new[v2];
		}
	}
	mParams->physicalMesh.numIndices = indicesWritten;

	updateSkinningNormals();
}


void ClothingPhysicalMeshImpl::clearMiscBuffers()
{
	mConstrainCoefficients.resize(0);
	mBoneIndices.resize(0);
	mBoneWeights.resize(0);
}



void ClothingPhysicalMeshImpl::computeEdgeLengths() const
{
	const uint32_t numIndices = mParams->physicalMesh.numIndices;

	float average = 0;
	float shortest = PX_MAX_F32;

	if (mParams->physicalMesh.isTetrahedralMesh)
	{
		for (uint32_t i = 0; i < numIndices; i += 4)
		{
			const float edge0 = (mVertices[mIndices[i + 0]] - mVertices[mIndices[i + 1]]).magnitudeSquared();
			const float edge1 = (mVertices[mIndices[i + 0]] - mVertices[mIndices[i + 2]]).magnitudeSquared();
			const float edge2 = (mVertices[mIndices[i + 0]] - mVertices[mIndices[i + 3]]).magnitudeSquared();
			const float edge3 = (mVertices[mIndices[i + 1]] - mVertices[mIndices[i + 2]]).magnitudeSquared();
			const float edge4 = (mVertices[mIndices[i + 1]] - mVertices[mIndices[i + 3]]).magnitudeSquared();
			const float edge5 = (mVertices[mIndices[i + 2]] - mVertices[mIndices[i + 3]]).magnitudeSquared();
			shortest = PxMin(shortest, edge0);
			shortest = PxMin(shortest, edge1);
			shortest = PxMin(shortest, edge2);
			shortest = PxMin(shortest, edge3);
			shortest = PxMin(shortest, edge4);
			shortest = PxMin(shortest, edge5);

			average += PxSqrt(edge0) + PxSqrt(edge1) + PxSqrt(edge2) + PxSqrt(edge3) + PxSqrt(edge4) + PxSqrt(edge5);
		}
		mParams->physicalMesh.isClosed = false;
	}
	else
	{
		// also check if the mesh is closed
		nvidia::Array<SortedEdge> edges;

		for (uint32_t i = 0; i < numIndices; i += 3)
		{
			const float edge0 = (mVertices[mIndices[i + 0]] - mVertices[mIndices[i + 1]]).magnitudeSquared();
			const float edge1 = (mVertices[mIndices[i + 0]] - mVertices[mIndices[i + 2]]).magnitudeSquared();
			const float edge2 = (mVertices[mIndices[i + 1]] - mVertices[mIndices[i + 2]]).magnitudeSquared();
			shortest = PxMin(shortest, edge0);
			shortest = PxMin(shortest, edge1);
			shortest = PxMin(shortest, edge2);

			average += PxSqrt(edge0) + PxSqrt(edge1) + PxSqrt(edge2);

			edges.pushBack(SortedEdge(mIndices[i + 0], mIndices[i + 1]));
			edges.pushBack(SortedEdge(mIndices[i + 1], mIndices[i + 2]));
			edges.pushBack(SortedEdge(mIndices[i + 2], mIndices[i + 0]));
		}

		nvidia::sort(edges.begin(), edges.size(), SortedEdge(0, 0));

		bool meshClosed = false;
		if ((edges.size() & 0x1) == 0) // only works for even number of indices
		{
			meshClosed = true;
			for (uint32_t i = 0; i < edges.size(); i += 2)
			{
				meshClosed &= edges[i] == edges[i + 1];

				if (i > 0)
				{
					meshClosed &= !(edges[i - 1] == edges[i]);
				}
			}
		}
		mParams->physicalMesh.isClosed = meshClosed;
	}

	mParams->physicalMesh.shortestEdgeLength = PxSqrt(shortest);
	if (numIndices > 0)
	{
		mParams->physicalMesh.averageEdgeLength = average / (float)numIndices;
	}
	else
	{
		mParams->physicalMesh.averageEdgeLength = 0.0f;
	}
}



void ClothingPhysicalMeshImpl::addBoneToVertex(uint32_t vertexNumber, uint16_t boneIndex, float boneWeight)
{
	if (mBoneIndices.size() == 0 || mBoneWeights.size() == 0)
	{
		return;
	}

	const uint32_t numBonesPerVertex = mParams->physicalMesh.numBonesPerVertex;
	for (uint32_t i = 0; i < numBonesPerVertex; i++)
	{
		if (mBoneIndices[vertexNumber * numBonesPerVertex + i] == boneIndex ||
		        mBoneWeights[vertexNumber * numBonesPerVertex + i] == 0.0f)
		{
			mBoneIndices[vertexNumber * numBonesPerVertex + i] = boneIndex;
			mBoneWeights[vertexNumber * numBonesPerVertex + i] =
			    PxMax(mBoneWeights[vertexNumber * numBonesPerVertex + i], boneWeight);
			sortBonesOfVertex(vertexNumber);
			return;
		}
	}
}



void ClothingPhysicalMeshImpl::sortBonesOfVertex(uint32_t vertexNumber)
{
	const uint32_t numBonesPerVertex = mParams->physicalMesh.numBonesPerVertex;
	if (mBoneIndices.size() == 0 || mBoneWeights.size() == 0 || numBonesPerVertex <= 1)
	{
		return;
	}

	// bubble sort
	bool changed = true;
	while (changed)
	{
		changed = false;
		for (uint32_t i = 0; i < numBonesPerVertex - 1; i++)
		{
			const uint32_t index = vertexNumber * numBonesPerVertex + i;
			if (mBoneWeights[index] < mBoneWeights[index + 1])
			{
				// swap
				float tempF = mBoneWeights[index];
				mBoneWeights[index] = mBoneWeights[index + 1];
				mBoneWeights[index + 1] = tempF;

				uint16_t tempI = mBoneIndices[index];
				mBoneIndices[index] = mBoneIndices[index + 1];
				mBoneIndices[index + 1] = tempI;

				changed = true;
			}
		}
	}
}



void ClothingPhysicalMeshImpl::normalizeBonesOfVertex(uint32_t vertexNumber)
{
	if (mBoneIndices.size() == 0 || mBoneWeights.size() == 0)
	{
		return;
	}

	const uint32_t numBonesPerVertex = mParams->physicalMesh.numBonesPerVertex;

	float sum = 0;
	float last = FLT_MAX;
	for (uint32_t i = 0; i < numBonesPerVertex; i++)
	{
		sum += mBoneWeights[vertexNumber * numBonesPerVertex + i];

		// make sure it is sorted!
		PX_ASSERT(mBoneWeights[vertexNumber * numBonesPerVertex + i] <= last);
		last = mBoneWeights[vertexNumber * numBonesPerVertex + i];
	}

	PX_UNUSED(last);

	if (sum > 0)
	{
		float invSum = 1.0f / sum;
		for (uint32_t i = 0; i < numBonesPerVertex; i++)
		{
			float& weight = mBoneWeights[vertexNumber * numBonesPerVertex + i];
			if (weight > 0)
			{
				weight *= invSum;
			}
			else
			{
				mBoneIndices[vertexNumber * numBonesPerVertex + i] = 0;
			}
		}
	}
	else
	{
		for (uint32_t i = 0; i < numBonesPerVertex; i++)
		{
			mBoneIndices[vertexNumber * numBonesPerVertex + i] = 0;
			mBoneWeights[vertexNumber * numBonesPerVertex + i] = 0.0f;
		}
	}
}



void ClothingPhysicalMeshImpl::updateSkinningNormals()
{
	// only for non-softbodies
	if (isTetrahedralMesh())
	{
		return;
	}

	const uint32_t numVertices = mParams->physicalMesh.numVertices;
	const uint32_t numIndices = mParams->physicalMesh.numIndices;

	PX_ASSERT(mSkinningNormals.size() == mVertices.size());
	memset(mSkinningNormals.begin(), 0, sizeof(PxVec3) * numVertices);

	for (uint32_t i = 0; i < numIndices; i += 3)
	{
		PxVec3 normal;
		normal = mVertices[mIndices[i + 1]] - mVertices[mIndices[i]];
		normal = normal.cross(mVertices[mIndices[i + 2]] - mVertices[mIndices[i]]);
		mSkinningNormals[mIndices[i]] += normal;
		mSkinningNormals[mIndices[i + 1]] += normal;
		mSkinningNormals[mIndices[i + 2]] += normal;
	}

	for (uint32_t i = 0; i < numVertices; i++)
	{
		mSkinningNormals[i].normalize();
	}
}



void ClothingPhysicalMeshImpl::smoothNormals(uint32_t numIterations)
{
	const uint32_t increment = mParams->physicalMesh.isTetrahedralMesh ? 4u : 3u;
	const uint32_t numIndices = mParams->physicalMesh.numIndices;

	for (uint32_t iters = 0; iters < numIterations; iters++)
	{
		for (uint32_t i = 0; i < numIndices; i += increment)
		{
			PxVec3& n0 = mNormals[mIndices[i + 0]];
			PxVec3& n1 = mNormals[mIndices[i + 1]];
			PxVec3& n2 = mNormals[mIndices[i + 2]];
			PxVec3 n = n0 + n1 + n2;
			n.normalize();
			n0 = n;
			n1 = n;
			n2 = n;
		}
	}
}



void ClothingPhysicalMeshImpl::updateOptimizationData()
{
	PX_ASSERT(mParams != NULL);

	const int32_t numVertices = mParams->physicalMesh.vertices.arraySizes[0];
	const uint32_t numBonesPerVertex = mParams->physicalMesh.numBonesPerVertex;

	const float* boneWeights = mParams->physicalMesh.boneWeights.buf;
	PX_ASSERT(boneWeights == NULL || mParams->physicalMesh.boneWeights.arraySizes[0] == numVertices * (int32_t)numBonesPerVertex);

	const ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type* constrainCoeffs = mParams->physicalMesh.constrainCoefficients.buf;
	PX_ASSERT(constrainCoeffs == NULL || mParams->physicalMesh.constrainCoefficients.arraySizes[0] == numVertices);

	if (boneWeights == NULL && constrainCoeffs == NULL)
	{
		return;
	}

	uint32_t allocNumVertices = ((uint32_t)physx::shdfnd::ceil((float)numVertices / NUM_VERTICES_PER_CACHE_BLOCK)) * NUM_VERTICES_PER_CACHE_BLOCK; // allocate more to have a multiple of numVerticesPerCachBlock

	NvParameterized::Handle optimizationDataHandle(*mParams, "physicalMesh.optimizationData");
	PX_ASSERT(optimizationDataHandle.isValid());
	optimizationDataHandle.resizeArray((int32_t)(allocNumVertices + 1) / 2);
	uint8_t* optimizationData = mParams->physicalMesh.optimizationData.buf;
	memset(optimizationData, 0, sizeof(uint8_t) * mParams->physicalMesh.optimizationData.arraySizes[0]);

	for (int32_t i = 0; i < numVertices; ++i)
	{
		uint8_t numBones = 0;
		if (boneWeights != NULL)
		{
			for (; numBones < numBonesPerVertex; numBones++)
			{
				if (boneWeights[i * numBonesPerVertex + numBones] == 0.0f)
				{
					break;
				}
			}
		}

		uint8_t& data = optimizationData[i / 2];
		PX_ASSERT(numBones < 8); // we use 3 bits

		if (constrainCoeffs != NULL)
		{
			uint8_t bitShift = 0;
			if (i % 2 == 0)
			{
				data = 0;
			}
			else
			{
				bitShift = 4;
			}
			data |= numBones << bitShift;

			// store for each vertex if collisionSphereDistance is < 0
			if (constrainCoeffs[i].collisionSphereDistance < 0.0f)
			{
				data |= 8 << bitShift;
				mParams->physicalMesh.hasNegativeBackstop = true;
			}
			else
			{
				data &= ~(8 << bitShift);
			}
		}
	}
}



void ClothingPhysicalMeshImpl::updateMaxMaxDistance()
{
	const uint32_t numVertices = mParams->physicalMesh.numVertices;

	float maxMaxDistance = 0.0f;
	for (uint32_t i = 0; i < numVertices; i++)
	{
		maxMaxDistance = PxMax(maxMaxDistance, mConstrainCoefficients[i].maxDistance);
	}

	mParams->physicalMesh.maximumMaxDistance = maxMaxDistance;
}



void ClothingPhysicalMeshImpl::preSerialize(void* userData_)
{
	PX_UNUSED(userData_);

	writeBackData();

	// shrink the buffers

	if (!mVertices.isEmpty() && mVertices.size() != mParams->physicalMesh.numVertices)
	{
		mVertices.resize(mParams->physicalMesh.numVertices);
	}

	if (!mNormals.isEmpty() && mNormals.size() != mParams->physicalMesh.numVertices)
	{
		mNormals.resize(mParams->physicalMesh.numVertices);
	}

	if (!mSkinningNormals.isEmpty() && mSkinningNormals.size() != mParams->physicalMesh.numVertices)
	{
		mSkinningNormals.resize(mParams->physicalMesh.numVertices);
	}

	if (!mConstrainCoefficients.isEmpty() && mConstrainCoefficients.size() != mParams->physicalMesh.numVertices)
	{
		mConstrainCoefficients.resize(mParams->physicalMesh.numVertices);
	}

	if (!mBoneIndices.isEmpty() && mBoneIndices.size() != mParams->physicalMesh.numVertices * mParams->physicalMesh.numBonesPerVertex)
	{
		mBoneIndices.resize(mParams->physicalMesh.numVertices * mParams->physicalMesh.numBonesPerVertex);
	}

	if (!mBoneWeights.isEmpty() && mBoneWeights.size() != mParams->physicalMesh.numVertices * mParams->physicalMesh.numBonesPerVertex)
	{
		mBoneWeights.resize(mParams->physicalMesh.numVertices * mParams->physicalMesh.numBonesPerVertex);
	}

	if (!mIndices.isEmpty() && mIndices.size() != mParams->physicalMesh.numIndices)
	{
		mIndices.resize(mParams->physicalMesh.numIndices);
	}

	updateOptimizationData();
}



void ClothingPhysicalMeshImpl::permuteBoneIndices(Array<int32_t>& old2newBoneIndices)
{
	if (mBoneIndices.size() == 0)
	{
		return;
	}

	const uint32_t numVertices = mParams->physicalMesh.numVertices;
	const uint32_t numBonesPerVertex = mParams->physicalMesh.numBonesPerVertex;

	for (uint32_t j = 0; j < numVertices; j++)
	{
		for (uint32_t k = 0; k < numBonesPerVertex; k++)
		{
			uint16_t& index = mBoneIndices[j * numBonesPerVertex + k];
			PX_ASSERT(old2newBoneIndices[index] >= 0);
			PX_ASSERT(old2newBoneIndices[index] <= 0xffff);
			index = (uint16_t)old2newBoneIndices[index];
		}
	}
}



void ClothingPhysicalMeshImpl::applyTransformation(const PxMat44& transformation, float scale)
{
	const uint32_t numVertices = mParams->physicalMesh.numVertices;

	PX_ASSERT(scale > 0.0f); // PH: negative scale won't work well here

	for (uint32_t i = 0; i < numVertices; i++)
	{
		if (!mVertices.isEmpty())
		{
			mVertices[i] = (transformation.transform(mVertices[i])) * scale;
		}
		if (!mNormals.isEmpty())
		{
			mNormals[i] = transformation.transform(mNormals[i]);
		}
		if (!mSkinningNormals.isEmpty())
		{
			mSkinningNormals[i] = transformation.transform(mSkinningNormals[i]);
		}
		if (!mConstrainCoefficients.isEmpty())
		{
			mConstrainCoefficients[i].maxDistance *= scale;
			mConstrainCoefficients[i].collisionSphereDistance *= scale;
			mConstrainCoefficients[i].collisionSphereRadius *= scale;
		}
	}

	PxMat33 t33(PxVec3(transformation.column0.x, transformation.column0.y, transformation.column0.z),
				PxVec3(transformation.column1.x, transformation.column1.y, transformation.column1.z),
				PxVec3(transformation.column2.x, transformation.column2.y, transformation.column2.z));

	if (t33.getDeterminant() * scale < 0.0f)
	{
		const uint32_t numIndices = mParams->physicalMesh.numIndices;

		if (mParams->physicalMesh.isTetrahedralMesh)
		{
			PX_ASSERT(numIndices % 4 == 0);
			for (uint32_t i = 0; i < numIndices; i += 4)
			{
				nvidia::swap(mIndices[i + 2], mIndices[i + 3]);
			}
		}
		else
		{
			// Flip the triangle indices to change winding (and thus normal generation in the PhysX SDK
			PX_ASSERT(numIndices % 3 == 0);
			for (uint32_t i = 0; i < numIndices; i += 3)
			{
				nvidia::swap(mIndices[i + 1], mIndices[i + 2]);
			}
		}

		mParams->physicalMesh.flipNormals ^= true;

		if (mParams->transitionDownB.buf != NULL || mParams->transitionUpB.buf != NULL)
		{
			APEX_DEBUG_WARNING("applyTransformation will not work with old assets, re-export from DCC tools");
		}

		const uint32_t numTransDown = (uint32_t)mParams->transitionDown.arraySizes[0];
		for (uint32_t i = 0; i < numTransDown; i++)
		{
			mParams->transitionDown.buf[i].vertexBary.z *= scale;
			mParams->transitionDown.buf[i].normalBary.z *= scale;
		}

		const uint32_t numTransUp = (uint32_t)mParams->transitionUp.arraySizes[0];
		for (uint32_t i = 0; i < numTransUp; i++)
		{
			mParams->transitionUp.buf[i].vertexBary.z *= scale;
			mParams->transitionUp.buf[i].normalBary.z *= scale;
		}
	}

	mParams->physicalMesh.maximumMaxDistance *= scale;
	mParams->physicalMesh.shortestEdgeLength *= scale;
	mParams->physicalMesh.averageEdgeLength *= scale;
}



void ClothingPhysicalMeshImpl::applyPermutation(const Array<uint32_t>& permutation)
{
	const uint32_t numVertices = mParams->physicalMesh.numVertices;
	const uint32_t numBonesPerVertex = mParams->physicalMesh.numBonesPerVertex;

	if (!mVertices.isEmpty())
	{
		ApexPermute<PxVec3>(mVertices.begin(), &permutation[0], numVertices);
	}

	if (!mNormals.isEmpty())
	{
		ApexPermute<PxVec3>(mNormals.begin(), &permutation[0], numVertices);
	}

	if (!mSkinningNormals.isEmpty())
	{
		ApexPermute<PxVec3>(mSkinningNormals.begin(), &permutation[0], numVertices);
	}

	if (!mConstrainCoefficients.isEmpty())
	{
		ApexPermute<ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type>(mConstrainCoefficients.begin(), &permutation[0], numVertices);
	}

	if (!mBoneIndices.isEmpty())
	{
		ApexPermute<uint16_t>(mBoneIndices.begin(), &permutation[0], numVertices, numBonesPerVertex);
	}

	if (!mBoneWeights.isEmpty())
	{
		ApexPermute<float>(mBoneWeights.begin(), &permutation[0], numVertices, numBonesPerVertex);
	}
}



struct OrderedTriangle
{
	void init(uint32_t _triNr, uint32_t _v0, uint32_t _v1, uint32_t _v2)
	{
		triNr = _triNr;
		v0 = _v0;
		v1 = _v1;
		v2 = _v2;
		// bubble sort
		if (v0 > v1)
		{
			uint32_t v = v0;
			v0 = v1;
			v1 = v;
		}
		if (v1 > v2)
		{
			uint32_t v = v1;
			v1 = v2;
			v2 = v;
		}
		if (v0 > v1)
		{
			uint32_t v = v0;
			v0 = v1;
			v1 = v;
		}
	}
	bool operator()(const OrderedTriangle& a, const OrderedTriangle& b) const
	{
		if (a.v0 < b.v0)
		{
			return true;
		}
		if (a.v0 > b.v0)
		{
			return false;
		}
		if (a.v1 < b.v1)
		{
			return true;
		}
		if (a.v1 > b.v1)
		{
			return false;
		}
		return (a.v2 < b.v2);
	}
	bool operator == (const OrderedTriangle& t) const
	{
		return v0 == t.v0 && v1 == t.v1 && v2 == t.v2;
	}
	uint32_t v0, v1, v2;
	uint32_t triNr;
};

bool ClothingPhysicalMeshImpl::removeDuplicatedTriangles(uint32_t numIndices, uint32_t indexByteStride, const void* indices)
{
	uint32_t numTriangles = numIndices / 3;
	Array<OrderedTriangle> triangles;
	triangles.resize(numTriangles);

	{
		const uint8_t* srcIndices = (const uint8_t*)indices;
		for (uint32_t i = 0; i < numTriangles; i++)
		{
			uint32_t i0 = *(const uint32_t*)(srcIndices);
			srcIndices += indexByteStride;
			uint32_t i1 = *(const uint32_t*)(srcIndices);
			srcIndices += indexByteStride;
			uint32_t i2 = *(const uint32_t*)(srcIndices);
			srcIndices += indexByteStride;

			triangles[i].init(i, i0, i1, i2);
		}
	}

	nvidia::sort(triangles.begin(), triangles.size(), OrderedTriangle());

	uint32_t fromPos = 0;
	uint32_t toPos = 0;
	while (fromPos < numTriangles)
	{
		OrderedTriangle& t = triangles[fromPos];
		triangles[toPos] = t;
		fromPos++;
		toPos++;
		while (fromPos < numTriangles && triangles[fromPos] == t)
		{
			fromPos++;
		}
	}
	if (fromPos == toPos)
	{
		return false;
	}

	mParams->physicalMesh.numIndices = 3 * toPos;

	mIndices.resize(mParams->physicalMesh.numIndices);
	if (mParams->physicalMesh.numIndices > 0)
	{
		for (uint32_t i = 0; i < toPos; i++)
		{
			OrderedTriangle& t = triangles[i];
			const uint8_t* srcIndices = (const uint8_t*)indices + 3 * t.triNr * indexByteStride;

			mIndices[3 * i]   = *(uint32_t*)srcIndices;
			srcIndices += indexByteStride;
			mIndices[3 * i + 1] = *(uint32_t*)srcIndices;
			srcIndices += indexByteStride;
			mIndices[3 * i + 2] = *(uint32_t*)srcIndices;
			srcIndices += indexByteStride;
		}
	}

	fixTriangleOrientations();
	return true;
}


struct OrderedTriangleEdge
{
	void init(uint32_t _v0, uint32_t _v1, uint32_t _triNr, uint32_t _edgeNr)
	{
		if (_v0 < _v1)
		{
			v0 = _v0;
			v1 = _v1;
		}
		else
		{
			v0 = _v1;
			v1 = _v0;
		}
		triNr = _triNr;
		edgeNr = _edgeNr;
	}
	bool operator()(const OrderedTriangleEdge& a, const OrderedTriangleEdge& b) const
	{
		if (a.v0 < b.v0)
		{
			return true;
		}
		if (a.v0 > b.v0)
		{
			return false;
		}
		return (a.v1 < b.v1);
	}
	bool operator == (const OrderedTriangleEdge& e) const
	{
		return v0 == e.v0 && v1 == e.v1;
	}
	uint32_t v0, v1;
	uint32_t triNr, edgeNr;
};

void ClothingPhysicalMeshImpl::computeNeighborInformation(Array<int32_t> &neighbors)
{
	// compute neighbor information
	const uint32_t numTriangles = mParams->physicalMesh.numIndices / 3;

	Array<OrderedTriangleEdge> edges;
	edges.resize(3 * numTriangles);

	for (uint32_t i = 0; i < numTriangles; i++)
	{
		uint32_t i0 = mIndices[3 * i];
		uint32_t i1 = mIndices[3 * i + 1];
		uint32_t i2 = mIndices[3 * i + 2];
		edges[3 * i  ].init(i0, i1, i, 0);
		edges[3 * i + 1].init(i1, i2, i, 1);
		edges[3 * i + 2].init(i2, i0, i, 2);
	}

	nvidia::sort(edges.begin(), edges.size(), OrderedTriangleEdge());

	neighbors.resize(3 * numTriangles, -1);

	uint32_t i = 0;
	while (i < edges.size())
	{
		OrderedTriangleEdge& e0 = edges[i];
		i++;
		while (i < edges.size() && edges[i] == e0)
		{
			OrderedTriangleEdge& e1 = edges[i];
			neighbors[3 * e0.triNr + e0.edgeNr] = (int32_t)e1.triNr;
			neighbors[3 * e1.triNr + e1.edgeNr] = (int32_t)e0.triNr;
			i++;
		}
	}
}


void ClothingPhysicalMeshImpl::fixTriangleOrientations()
{
	PX_ASSERT(!mParams->physicalMesh.isTetrahedralMesh);
	Array<int32_t> neighbors;
	computeNeighborInformation(neighbors);

	const uint32_t numTriangles = mParams->physicalMesh.numIndices / 3;

	// 0 = non visited, 1 = visited, 2 = visited, to be flipped
	Array<uint8_t> marks;
	marks.resize(numTriangles, 0);

	Array<uint32_t> queue;

	for (uint32_t i = 0; i < numTriangles; i++)
	{
		if (marks[i] != 0)
		{
			continue;
		}
		queue.clear();
		marks[i] = 1;
		queue.pushBack(i);
		while (!queue.empty())
		{
			uint32_t triNr = queue[queue.size() - 1];
			queue.popBack();
			for (uint32_t j = 0; j < 3; j++)
			{
				int adjNr = neighbors[3 * triNr + j];
				if (adjNr < 0 || marks[(uint32_t)adjNr] != 0)
				{
					continue;
				}
				queue.pushBack((uint32_t)adjNr);
				uint32_t i0, i1;
				if (marks[triNr] == 1)
				{
					i0 = mIndices[3 * triNr + j];
					i1 = mIndices[3 * triNr + ((j + 1) % 3)];
				}
				else
				{
					i1 = mIndices[3 * triNr + j];
					i0 = mIndices[3 * triNr + ((j + 1) % 3)];
				}
				// don't swap here because this would corrupt the neighbor information
				marks[(uint32_t)adjNr] = 1;
				if (mIndices[3 * (uint32_t)adjNr + 0] == i0 && mIndices[3 * (uint32_t)adjNr + 1] == i1)
				{
					marks[(uint32_t)adjNr] = 2;
				}
				if (mIndices[3 * (uint32_t)adjNr + 1] == i0 && mIndices[3 * (uint32_t)adjNr + 2] == i1)
				{
					marks[(uint32_t)adjNr] = 2;
				}
				if (mIndices[3 * (uint32_t)adjNr + 2] == i0 && mIndices[3 * (uint32_t)adjNr + 0] == i1)
				{
					marks[(uint32_t)adjNr] = 2;
				}
			}
		}
	}
	for (uint32_t i = 0; i < numTriangles; i++)
	{
		if (marks[i] == 2)
		{
			uint32_t i0 = mIndices[3 * i];
			mIndices[3 * i] = mIndices[3 * i + 1];
			mIndices[3 * i + 1] = i0;
		}
	}
}

}
} // namespace nvidia


