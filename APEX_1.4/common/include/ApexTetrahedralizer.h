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



#ifndef APEX_TETRAHEDRALIZER_H
#define APEX_TETRAHEDRALIZER_H

#include "ApexDefs.h"
#include "PxBounds3.h"

#include "ApexUsingNamespace.h"
#include "PsArray.h"
#include "PsUserAllocated.h"

#define TETRAHEDRALIZER_DEBUG_RENDERING 0

namespace nvidia
{
namespace apex
{

class ApexMeshHash;
class IProgressListener;

class ApexTetrahedralizer : public UserAllocated
{
public:
	ApexTetrahedralizer(uint32_t subdivision);
	~ApexTetrahedralizer();


	void registerVertex(const PxVec3& v);
	void registerTriangle(uint32_t i0, uint32_t i1, uint32_t i2);
	void endRegistration(IProgressListener* progress);

	uint32_t getNumVertices() const
	{
		return mVertices.size();
	}
	void getVertices(PxVec3* data);
	uint32_t getNumIndices() const
	{
		return mTetras.size() * 4;
	}
	void getIndices(uint32_t* data);

private:
	struct FullTetrahedron;

	void weldVertices();
	void delaunayTetrahedralization(IProgressListener* progress);
	int32_t findSurroundingTetra(uint32_t startTetra, const PxVec3& p) const;
	float retriangulate(const uint32_t tetraNr, uint32_t vertexNr);
	uint32_t swapTetrahedra(uint32_t startTet, IProgressListener* progress);
	bool swapEdge(uint32_t v0, uint32_t v1);
	bool removeOuterTetrahedra(IProgressListener* progress);

	void updateCircumSphere(FullTetrahedron& tetra) const;
	bool pointInCircumSphere(FullTetrahedron& tetra, const PxVec3& p) const;
	bool pointInTetra(const FullTetrahedron& tetra, const PxVec3& p) const;

	float getTetraVolume(const FullTetrahedron& tetra) const;
	float getTetraVolume(int32_t v0, int32_t v1, int32_t v2, int32_t v3) const;
	float getTetraQuality(const FullTetrahedron& tetra) const;
	float getTetraLongestEdge(const FullTetrahedron& tetra) const;

	bool triangleContainsVertexNr(uint32_t* triangle, uint32_t* vertexNumber, uint32_t nbVertices);

	void compressTetrahedra(bool trashNeighbours);
	void compressVertices();

	inline bool isFarVertex(uint32_t vertNr) const
	{
		return mFirstFarVertex <= vertNr && vertNr <= mLastFarVertex;
	}

	ApexMeshHash* mMeshHash;

	uint32_t mSubdivision;

	PxBounds3 mBound;
	float mBoundDiagonal;

	struct TetraVertex
	{
		inline void init(const PxVec3& pos, uint32_t  flags)
		{
			this->pos = pos;
			this->flags = flags;
		}
		inline bool isDeleted() const
		{
			return flags == (uint32_t)0xdeadf00d;
		}
		inline void markDeleted()
		{
			flags = 0xdeadf00d;
		}

		PxVec3 pos;
		uint32_t flags;
	};
	class LessInOneAxis
	{
		uint32_t mAxis;
	public:
		LessInOneAxis(uint32_t axis) : mAxis(axis) {}
		bool operator()(const TetraVertex& v1, const TetraVertex& v2) const
		{
			return v1.pos[mAxis] < v2.pos[mAxis];
		}
	};

	struct TetraEdge
	{
		void init(int32_t v0, int32_t v1, int32_t tetra, int neighborNr = -1)
		{
			this->tetraNr = (uint32_t)tetra;
			this->neighborNr = neighborNr;
			PX_ASSERT(v0 != v1);
			vNr0 = (uint32_t)PxMin(v0, v1);
			vNr1 = (uint32_t)PxMax(v0, v1);
		}
		PX_INLINE bool operator <(const TetraEdge& e) const
		{
			if (vNr0 < e.vNr0)
			{
				return true;
			}
			if (vNr0 > e.vNr0)
			{
				return false;
			}
			if (vNr1 < e.vNr1)
			{
				return true;
			}
			if (vNr1 > e.vNr1)
			{
				return false;
			}
			return (neighborNr < e.neighborNr);
		}
		PX_INLINE bool operator()(const TetraEdge& e1, const TetraEdge& e2) const
		{
			return e1 < e2;
		}
		bool operator ==(TetraEdge& e) const
		{
			return vNr0 == e.vNr0 && vNr1 == e.vNr1;
		}

		bool allEqual(TetraEdge& e) const
		{
			return (vNr0 == e.vNr0) && (vNr1 == e.vNr1) && (neighborNr == e.neighborNr);
		}
		uint32_t vNr0, vNr1;
		uint32_t tetraNr;
		int32_t neighborNr;
	};


	struct TetraEdgeList
	{
		void add(TetraEdge& edge)
		{
			mEdges.pushBack(edge);
		}
		void insert(uint32_t pos, TetraEdge& edge);
		uint32_t numEdges()
		{
			return mEdges.size();
		}
		void sort();
		int  findEdge(int v0, int v1);
		int  findEdgeTetra(int v0, int v1, int tetraNr);
		TetraEdge& operator[](unsigned i)
		{
			return mEdges[i];
		}
		const TetraEdge& operator[](unsigned i) const
		{
			return mEdges[i];
		}

		physx::Array<TetraEdge> mEdges;
	};


	struct FullTetrahedron
	{
		void init()
		{
			vertexNr[0] = vertexNr[1] = vertexNr[2] = vertexNr[3] = -1;
			neighborNr[0] = neighborNr[1] = neighborNr[2] = neighborNr[3] = -1;
			center = PxVec3(0.0f);
			radiusSquared = 0.0f;
			quality = 0;
			bCircumSphereDirty = 1;
			bDeleted = 0;
		}
		void set(int32_t v0, int32_t v1, int32_t v2, int32_t v3)
		{
			vertexNr[0] = v0;
			vertexNr[1] = v1;
			vertexNr[2] = v2;
			vertexNr[3] = v3;
			neighborNr[0] = neighborNr[1] = neighborNr[2] = neighborNr[3] = -1;
			center = PxVec3(0.0f);
			radiusSquared = 0.0f;
			quality = 0;
			bCircumSphereDirty = 1;
			bDeleted = 0;
		}
		bool operator==(const FullTetrahedron& t) const
		{
			return
			    (vertexNr[0] == t.vertexNr[0]) &&
			    (vertexNr[1] == t.vertexNr[1]) &&
			    (vertexNr[2] == t.vertexNr[2]) &&
			    (vertexNr[3] == t.vertexNr[3]);
		}

		bool containsVertex(int32_t nr) const
		{
			return (vertexNr[0] == nr || vertexNr[1] == nr || vertexNr[2] == nr || vertexNr[3] == nr);
		}
		void replaceVertex(int nr, int newNr)
		{
			if (vertexNr[0] == nr)
			{
				vertexNr[0] = newNr;
			}
			else if (vertexNr[1] == nr)
			{
				vertexNr[1] = newNr;
			}
			else if (vertexNr[2] == nr)
			{
				vertexNr[2] = newNr;
			}
			else if (vertexNr[3] == nr)
			{
				vertexNr[3] = newNr;
			}
			else
			{
				PX_ASSERT(0);
			}
		}
		void get2OppositeVertices(int vNr0, int vNr1, int& vNr2, int& vNr3)
		{
			int v[4], p = 0;
			if (vertexNr[0] != vNr0 && vertexNr[0] != vNr1)
			{
				v[p++] = vertexNr[0];
			}
			if (vertexNr[1] != vNr0 && vertexNr[1] != vNr1)
			{
				v[p++] = vertexNr[1];
			}
			if (vertexNr[2] != vNr0 && vertexNr[2] != vNr1)
			{
				v[p++] = vertexNr[2];
			}
			if (vertexNr[3] != vNr0 && vertexNr[3] != vNr1)
			{
				v[p++] = vertexNr[3];
			}
			PX_ASSERT(p == 2);
			vNr2 = v[0];
			vNr3 = v[1];
		}
		void get3OppositeVertices(int vNr, int& vNr0, int& vNr1, int& vNr2)
		{
			if (vNr == vertexNr[0])
			{
				vNr0 = vertexNr[1];
				vNr1 = vertexNr[2];
				vNr2 = vertexNr[3];
			}
			else if (vNr == vertexNr[1])
			{
				vNr0 = vertexNr[2];
				vNr1 = vertexNr[0];
				vNr2 = vertexNr[3];
			}
			else if (vNr == vertexNr[2])
			{
				vNr0 = vertexNr[0];
				vNr1 = vertexNr[1];
				vNr2 = vertexNr[3];
			}
			else if (vNr == vertexNr[3])
			{
				vNr0 = vertexNr[2];
				vNr1 = vertexNr[1];
				vNr2 = vertexNr[0];
			}
			else
			{
				PX_ASSERT(0);
			}
		}
		int getOppositeVertex(int vNr0, int vNr1, int vNr2)
		{
			if (vertexNr[0] != vNr0 && vertexNr[0] != vNr1 && vertexNr[0] != vNr2)
			{
				return vertexNr[0];
			}
			if (vertexNr[1] != vNr0 && vertexNr[1] != vNr1 && vertexNr[1] != vNr2)
			{
				return vertexNr[1];
			}
			if (vertexNr[2] != vNr0 && vertexNr[2] != vNr1 && vertexNr[2] != vNr2)
			{
				return vertexNr[2];
			}
			if (vertexNr[3] != vNr0 && vertexNr[3] != vNr1 && vertexNr[3] != vNr2)
			{
				return vertexNr[3];
			}
			PX_ASSERT(0);
			return -1;
		}
		int sideOf(int vNr0, int vNr1, int vNr2)
		{
			if (vertexNr[0] != vNr0 && vertexNr[0] != vNr1 && vertexNr[0] != vNr2)
			{
				return 0;
			}
			if (vertexNr[1] != vNr0 && vertexNr[1] != vNr1 && vertexNr[1] != vNr2)
			{
				return 1;
			}
			if (vertexNr[2] != vNr0 && vertexNr[2] != vNr1 && vertexNr[2] != vNr2)
			{
				return 2;
			}
			if (vertexNr[3] != vNr0 && vertexNr[3] != vNr1 && vertexNr[3] != vNr2)
			{
				return 3;
			}
			PX_ASSERT(0);
			return -1;
		}
		inline int neighborNrOf(int vNr0, int vNr1, int vNr2)
		{
			PX_ASSERT(containsVertex(vNr0));
			PX_ASSERT(containsVertex(vNr1));
			PX_ASSERT(containsVertex(vNr2));
			if (vertexNr[0] != vNr0 && vertexNr[0] != vNr1 && vertexNr[0] != vNr2)
			{
				return 0;
			}
			if (vertexNr[1] != vNr0 && vertexNr[1] != vNr1 && vertexNr[1] != vNr2)
			{
				return 1;
			}
			if (vertexNr[2] != vNr0 && vertexNr[2] != vNr1 && vertexNr[2] != vNr2)
			{
				return 2;
			}
			if (vertexNr[3] != vNr0 && vertexNr[3] != vNr1 && vertexNr[3] != vNr2)
			{
				return 3;
			}
			PX_ASSERT(0);
			return 0;
		}
		inline int& neighborOf(int vNr0, int vNr1, int vNr2)
		{
			return neighborNr[neighborNrOf(vNr0, vNr1, vNr2)];
		}

		inline bool onSurface()
		{
			return neighborNr[0] < 0 || neighborNr[1] < 0 || neighborNr[2] < 0 || neighborNr[3] < 0;
		}
		// representation
		PxVec3 center;
		int32_t	vertexNr[4];
		int32_t	neighborNr[4];
		float	radiusSquared;
		uint32_t	quality : 10;
		uint32_t	bDeleted : 1;
		uint32_t	bCircumSphereDirty : 1;

		// static
		static const uint32_t sideIndices[4][3];
	};

	physx::Array<FullTetrahedron> mTetras;

	uint32_t mFirstFarVertex;
	uint32_t mLastFarVertex;

	physx::Array<TetraVertex> mVertices;
	physx::Array<uint32_t> mIndices;

	// temporary indices, that way we don't allocate this buffer all the time
	physx::Array<uint32_t> mTempItemIndices;

#if TETRAHEDRALIZER_DEBUG_RENDERING
public:
	physx::Array<PxVec3> debugLines;
	physx::Array<PxVec3> debugBounds;
	physx::Array<PxVec3> debugTetras;
#endif
};

}
} // end namespace nvidia::apex

#endif // APEX_TETRAHEDRALIZER_H
