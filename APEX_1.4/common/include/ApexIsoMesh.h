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



#ifndef APEX_ISO_MESH_H
#define APEX_ISO_MESH_H

#include "ApexUsingNamespace.h"
#include "ApexUsingNamespace.h"
#include "PsUserAllocated.h"
#include "PsArray.h"
#include "PxBounds3.h"

namespace nvidia
{
namespace apex
{

class IProgressListener;

class ApexIsoMesh : public UserAllocated
{
public:
	ApexIsoMesh(uint32_t isoGridSubdivision, uint32_t keepNBiggestMeshes, bool discardInnerMeshes);
	~ApexIsoMesh();

	void setBound(const PxBounds3& bound);
	void clear();
	void clearTemp();
	void addTriangle(const PxVec3& v0, const PxVec3& v1, const PxVec3& v2);
	bool update(IProgressListener* progress);


	uint32_t getNumVertices() const
	{
		return mIsoVertices.size();
	}
	const PxVec3& getVertex(uint32_t index) const
	{
		PX_ASSERT(index < mIsoVertices.size());
		return mIsoVertices[index];
	}

	uint32_t getNumTriangles() const
	{
		return mIsoTriangles.size();
	}
	void getTriangle(uint32_t index, uint32_t& v0, uint32_t& v1, uint32_t& v2) const;
private:
	// settable parameters
	uint32_t mIsoGridSubdivision;
	uint32_t mKeepNBiggestMeshes;
	bool  mDiscardInnerMeshes;
	PxBounds3 mBound;

	bool generateMesh(IProgressListener* progress);
	bool interpolate(float d0, float d1, const PxVec3& pos0, const PxVec3& pos1, PxVec3& pos);
	bool findNeighbors(IProgressListener* progress);
	void removeLayers();
	uint32_t floodFill(uint32_t triangleNr, uint32_t groupNr);

	void removeTrisAndVerts();

	// non-settable parameters (deducted from the ones you can set)
	float mCellSize;
	float mThickness;
	PxVec3 mOrigin;
	int32_t mNumX, mNumY, mNumZ;
	const float mIsoValue;


	struct IsoCell
	{
		void init()
		{
			density = 0.0f;
			vertNrX = -1;
			vertNrY = -1;
			vertNrZ = -1;
			firstTriangle = -1;
			numTriangles = 0;
		}
		float density;
		int32_t vertNrX;
		int32_t vertNrY;
		int32_t vertNrZ;
		int32_t firstTriangle;
		int32_t numTriangles;
	};
	physx::Array<IsoCell> mGrid;
	inline IsoCell& cellAt(int xi, int yi, int zi)
	{
		uint32_t index = (uint32_t)(((xi * mNumY) + yi) * mNumZ + zi);
		PX_ASSERT(index < mGrid.size());
		return mGrid[index];
	}


	struct IsoTriangle
	{
		void init()
		{
			vertexNr[0] = -1;
			vertexNr[1] = -1;
			vertexNr[2] = -1;
			adjTriangles[0] = -1;
			adjTriangles[1] = -1;
			adjTriangles[2] = -1;
			groupNr = -1;
			deleted = false;
		}
		void set(int32_t v0, int32_t v1, int32_t v2, int32_t cubeX, int32_t cubeY, int32_t cubeZ)
		{
			init();
			vertexNr[0] = v0;
			vertexNr[1] = v1;
			vertexNr[2] = v2;
			this->cubeX = cubeX;
			this->cubeY = cubeY;
			this->cubeZ = cubeZ;
		}
		void addNeighbor(int32_t triangleNr)
		{
			if (adjTriangles[0] == -1)
			{
				adjTriangles[0] = triangleNr;
			}
			else if (adjTriangles[1] == -1)
			{
				adjTriangles[1] = triangleNr;
			}
			else if (adjTriangles[2] == -1)
			{
				adjTriangles[2] = triangleNr;
			}
		}

		int32_t vertexNr[3];
		int32_t cubeX, cubeY, cubeZ;
		int32_t adjTriangles[3];
		int32_t groupNr;
		bool deleted;
	};

	struct IsoEdge
	{
		void set(int newV0, int newV1, int newTriangle)
		{
			if (newV0 < newV1)
			{
				v0 = newV0;
				v1 = newV1;
			}
			else
			{
				v0 = newV1;
				v1 = newV0;
			}
			triangleNr = newTriangle;
		}
		PX_INLINE bool operator < (const IsoEdge& e) const
		{
			if (v0 < e.v0)
			{
				return true;
			}
			if (v0 > e.v0)
			{
				return false;
			}
			return (v1 < e.v1);
		}

		PX_INLINE bool operator()(const IsoEdge& e1, const IsoEdge& e2) const
		{
			return e1 < e2;
		}

		PX_INLINE bool operator == (const IsoEdge& e) const
		{
			return v0 == e.v0 && v1 == e.v1;
		}

		int v0, v1;
		int triangleNr;
	};

	physx::Array<PxVec3> mIsoVertices;
	physx::Array<IsoTriangle> mIsoTriangles;
	physx::Array<IsoEdge> mIsoEdges;

	// evil, should not be used
	ApexIsoMesh& operator=(const ApexIsoMesh&);
};

}
} // end namespace nvidia::apex

#endif // APEX_ISO_MESH_H
