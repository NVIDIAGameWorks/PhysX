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



#ifndef __APEX_GENERALIZED_MARCHING_CUBES_H__
#define __APEX_GENERALIZED_MARCHING_CUBES_H__

#include "ApexUsingNamespace.h"
#include "PsUserAllocated.h"
#include "PsArray.h"

#include "PxBounds3.h"

namespace nvidia
{
namespace apex
{

class IProgressListener;
class ApexGeneralizedCubeTemplates;

class ApexGeneralizedMarchingCubes : public UserAllocated
{
public:
	ApexGeneralizedMarchingCubes(const PxBounds3& bound, uint32_t subdivision);
	~ApexGeneralizedMarchingCubes();

	void release();

	void registerTriangle(const PxVec3& p0, const PxVec3& p1, const PxVec3& p2);
	bool endRegistration(uint32_t bubleSizeToRemove, IProgressListener* progress);

	uint32_t getNumVertices()
	{
		return mVertices.size();
	}
	uint32_t getNumIndices()
	{
		return mIndices.size();
	}
	const PxVec3* getVertices()
	{
		return mVertices.begin();
	}
	const uint32_t* getIndices()
	{
		return mIndices.begin();
	}
private:

	struct GeneralizedVertRef
	{
		void init()
		{
			vertNr = -1;
			dangling = false;
			deleted = false;
		}
		int32_t vertNr;
		bool dangling;
		bool deleted;
	};

	struct GeneralizedCube
	{
		void init(int32_t xi, int32_t yi, int32_t zi)
		{
			this->xi = xi;
			this->yi = yi;
			this->zi = zi;
			next = -1;
			vertRefs[0].init();
			vertRefs[1].init();
			vertRefs[2].init();
			sideVertexNr[0] = -1;
			sideVertexNr[1] = -1;
			sideVertexNr[2] = -1;
			sideBounds[0].setEmpty();
			sideBounds[1].setEmpty();
			sideBounds[2].setEmpty();
			firstTriangle = -1;
			numTriangles = 0;
			deleted = false;
		}
		int32_t xi, yi, zi;
		int32_t next;
		GeneralizedVertRef vertRefs[3];
		int32_t sideVertexNr[3];
		PxBounds3 sideBounds[3];
		int32_t firstTriangle;
		uint32_t numTriangles;
		bool deleted;
	};

	inline int hashFunction(int xi, int yi, int zi)
	{
		int h = (int)(unsigned int)((xi * 92837111) ^(yi * 689287499) ^(zi * 283923481));
		return h % HASH_INDEX_SIZE;
	}
	int32_t createCube(int32_t xi, int32_t yi, int32_t zi);
	int32_t findCube(int32_t xi, int32_t yi, int32_t zi);
	void completeCells();
	void createTrianglesForCube(int32_t cellNr);
	void createNeighbourInfo();
	void getCubeEdgesAndGroups(int32_t cellNr, GeneralizedVertRef* vertRefs[12], int32_t groups[8]);
	void determineGroups();
	void removeBubbles(int32_t minGroupSize);
	void fixOrientations();
	void compress();


	PxBounds3 mBound;

	float mSpacing;
	float mInvSpacing;

	physx::Array<GeneralizedCube> mCubes;

	enum { HASH_INDEX_SIZE = 170111 };

	int32_t mFirstCube[HASH_INDEX_SIZE];

	physx::Array<PxVec3> mVertices;
	physx::Array<uint32_t> mIndices;

	physx::Array<int32_t> mFirstNeighbour;
	physx::Array<int32_t> mNeighbours;
	physx::Array<uint8_t> mTriangleDeleted;
	physx::Array<int32_t> mGeneralizedTriangles;
	physx::Array<int32_t> mCubeQueue;

	physx::Array<int32_t> mTriangleGroup;
	physx::Array<int32_t> mGroupFirstTriangle;
	physx::Array<int32_t> mGroupTriangles;

	ApexGeneralizedCubeTemplates* mTemplates;

	// for debugging only
public:
	physx::Array<PxVec3> mDebugLines;
};

}
} // end namespace nvidia::apex

#endif
