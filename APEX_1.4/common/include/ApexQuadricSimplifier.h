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



#ifndef __APEX_QUADRIC_SIMPLIFIER_H__
#define __APEX_QUADRIC_SIMPLIFIER_H__

#include "ApexUsingNamespace.h"
#include "PsArray.h"
#include "PsUserAllocated.h"

namespace nvidia
{
namespace apex
{

class ApexQuadricSimplifier : public UserAllocated
{
public:
	ApexQuadricSimplifier();

	~ApexQuadricSimplifier();
	void clear();

	// registration
	void registerVertex(const PxVec3& pos);
	void registerTriangle(uint32_t v0, uint32_t v1, uint32_t v2);
	bool endRegistration(bool mergeCloseVertices, IProgressListener* progress);

	// manipulation
	uint32_t simplify(uint32_t subdivision, int32_t maxSteps, float maxError, IProgressListener* progress);

	// accessors
	uint32_t getNumVertices() const
	{
		return mVertices.size();
	}
	uint32_t getNumDeletedVertices() const
	{
		return mNumDeletedVertices;
	}

	bool getVertexPosition(uint32_t vertexNr, PxVec3& pos) const
	{
		PX_ASSERT(vertexNr < mVertices.size());
		if (mVertices[vertexNr]->bDeleted == 1)
		{
			return false;
		}

		pos = mVertices[vertexNr]->pos;
		return true;
	}
	int32_t getTriangleNr(uint32_t v0, uint32_t v1, uint32_t v2) const;
	uint32_t getNumTriangles() const
	{
		return mTriangles.size() - mNumDeletedTriangles;
	}
	bool getTriangle(uint32_t i, uint32_t& v0, uint32_t& v1, uint32_t& v2) const;

private:

	class Quadric
	{
	public:
		void zero()
		{
			a00 = 0.0f;
			a01 = 0.0f;
			a02 = 0.0f;
			a03 = 0.0f;
			a11 = 0.0f;
			a12 = 0.0f;
			a13 = 0.0f;
			a22 = 0.0f;
			a23 = 0.0f;
			a33 = 0.0f;
		}

		// generate quadric from plane
		void setFromPlane(const PxVec3& v0, const PxVec3& v1, const PxVec3& v2)
		{
			PxVec3 n = (v1 - v0).cross(v2 - v0);
			n.normalize();
			float d = -n.dot(v0);
			a00 = n.x * n.x;
			a01 = n.x * n.y;
			a02 = n.x * n.z;
			a03 = n.x * d;
			a11 = n.y * n.y;
			a12 = n.y * n.z;
			a13 = n.y * d;
			a22 = n.z * n.z;
			a23 = n.z * d;
			a33 = d * d;
		}

		Quadric operator +(const Quadric& q) const
		{
			Quadric sum;
			sum.a00 = a00 + q.a00;
			sum.a01 = a01 + q.a01;
			sum.a02 = a02 + q.a02;
			sum.a03 = a03 + q.a03;
			sum.a11 = a11 + q.a11;
			sum.a12 = a12 + q.a12;
			sum.a13 = a13 + q.a13;
			sum.a22 = a22 + q.a22;
			sum.a23 = a23 + q.a23;
			sum.a33 = a33 + q.a33;
			return sum;
		}

		void operator +=(const Quadric& q)
		{
			a00 += q.a00;
			a01 += q.a01;
			a02 += q.a02;
			a03 += q.a03;
			a11 += q.a11;
			a12 += q.a12;
			a13 += q.a13;
			a22 += q.a22;
			a23 += q.a23;
			a33 += q.a33;
		}

		float outerProduct(const PxVec3& v)
		{
			return a00 * v.x * v.x + 2.0f * a01 * v.x * v.y + 2.0f * a02 * v.x * v.z + 2.0f * a03 * v.x +
			       a11 * v.y * v.y + 2.0f * a12 * v.y * v.z + 2.0f * a13 * v.y +
			       a22 * v.z * v.z + 2.0f * a23 * v.z + a33;
		}
	private:
		float a00, a01, a02, a03;
		float      a11, a12, a13;
		float           a22, a23;
		float                a33;

	};

	struct QuadricVertex : public UserAllocated
	{
		QuadricVertex(const PxVec3& newPos)
		{
			pos = newPos;
			q.zero();
			bDeleted = 0;
			bReferenced = 0;
			bBorder = 0;
		}
		void removeEdge(int32_t edgeNr);
		void addTriangle(int32_t triangleNr);
		void removeTriangle(int32_t triangleNr);
		PxVec3 pos;
		Quadric  q;
		physx::Array<uint32_t> mEdges;
		physx::Array<uint32_t> mTriangles;
		uint32_t bDeleted : 1;
		uint32_t bReferenced : 1;
		uint32_t bBorder : 1;
	};

	struct QuadricEdge
	{
		void init(int32_t v0, int32_t v1)
		{
			vertexNr[0] = (uint32_t)PxMin(v0, v1);
			vertexNr[1] = (uint32_t)PxMax(v0, v1);
			cost = -1.0f;
			lengthSquared = -1.0f;
			ratio = -1.0f;
			heapPos = -1;
			border = false;
			deleted = false;
		}
		bool operator < (QuadricEdge& e) const
		{
			if (vertexNr[0] < e.vertexNr[0])
			{
				return true;
			}
			if (vertexNr[0] > e.vertexNr[0])
			{
				return false;
			}
			return vertexNr[1] < e.vertexNr[1];
		}
		bool operator == (QuadricEdge& e) const
		{
			return vertexNr[0] == e.vertexNr[0] && vertexNr[1] == e.vertexNr[1];
		}
		uint32_t otherVertex(uint32_t vNr) const
		{
			if (vertexNr[0] == vNr)
			{
				return vertexNr[1];
			}
			else
			{
				PX_ASSERT(vertexNr[1] == vNr);
				return vertexNr[0];
			}
		}
		void replaceVertex(uint32_t vOld, uint32_t vNew)
		{
			if (vertexNr[0] == vOld)
			{
				vertexNr[0] = vNew;
			}
			else if (vertexNr[1] == vOld)
			{
				vertexNr[1] = vNew;
			}
			else
			{
				PX_ASSERT(0);
			}
			if (vertexNr[0] > vertexNr[1])
			{
				unsigned v = vertexNr[0];
				vertexNr[0] = vertexNr[1];
				vertexNr[1] = v;
			}
		}
		uint32_t vertexNr[2];
		float cost;
		float lengthSquared;
		float ratio;
		int32_t heapPos;
		bool border;
		bool deleted;
	};

	struct QuadricTriangle
	{
		void init(uint32_t v0, uint32_t v1, uint32_t v2)
		{
			vertexNr[0] = v0;
			vertexNr[1] = v1;
			vertexNr[2] = v2;
			deleted = false;
		}
		bool containsVertex(uint32_t vNr) const
		{
			return vertexNr[0] == vNr || vertexNr[1] == vNr || vertexNr[2] == vNr;
		}
		uint32_t otherVertex(uint32_t v0, uint32_t v1)
		{
			if (vertexNr[0] != v0 && vertexNr[0] != v1)
			{
				PX_ASSERT(v0 == vertexNr[1] || v0 == vertexNr[2]);
				PX_ASSERT(v1 == vertexNr[1] || v1 == vertexNr[2]);
				return vertexNr[0];
			}
			else if (vertexNr[1] != v0 && vertexNr[1] != v1)
			{
				PX_ASSERT(v0 == vertexNr[0] || v0 == vertexNr[2]);
				PX_ASSERT(v1 == vertexNr[0] || v1 == vertexNr[2]);
				return vertexNr[1];
			}
			else
			{
				PX_ASSERT(vertexNr[2] != v0 && vertexNr[2] != v1);
				PX_ASSERT(v0 == vertexNr[0] || v0 == vertexNr[1]);
				PX_ASSERT(v1 == vertexNr[0] || v1 == vertexNr[1]);
				return vertexNr[2];
			}
		}
		void replaceVertex(uint32_t vOld, uint32_t vNew)
		{
			if (vertexNr[0] == vOld)
			{
				vertexNr[0] = vNew;
			}
			else if (vertexNr[1] == vOld)
			{
				vertexNr[1] = vNew;
			}
			else if (vertexNr[2] == vOld)
			{
				vertexNr[2] = vNew;
			}
			else
			{
				PX_ASSERT(0);
			}
		}
		bool operator == (QuadricTriangle& t) const
		{
			return t.containsVertex(vertexNr[0]) &&
			       t.containsVertex(vertexNr[1]) &&
			       t.containsVertex(vertexNr[2]);
		}
		uint32_t vertexNr[3];
		bool deleted;
	};

	struct QuadricVertexRef
	{
		void init(const PxVec3& p, int32_t vNr)
		{
			pos = p;
			vertexNr = (uint32_t)vNr;
		}
		bool operator < (const QuadricVertexRef& vr)
		{
			return pos.x < vr.pos.x;
		}
		PxVec3 pos;
		uint32_t vertexNr;
	};


	void computeCost(QuadricEdge& edge);
	bool legalCollapse(QuadricEdge& edge, float maxLength);
	void collapseEdge(QuadricEdge& edge);
	void quickSortEdges(int32_t l, int32_t r);
	void quickSortVertexRefs(int32_t l, int32_t r);
	void mergeVertices();

	bool heapElementSmaller(QuadricEdge* e0, QuadricEdge* e1);
	void heapUpdate(uint32_t i);
	void heapSift(uint32_t i);
	void heapRemove(uint32_t i, bool append);
	void testHeap();
	void testMesh();

	PxBounds3 mBounds;

	physx::Array<QuadricVertex*>  mVertices;
	physx::Array<QuadricEdge>      mEdges;
	physx::Array<QuadricTriangle>  mTriangles;
	physx::Array<QuadricEdge*>    mHeap;
	physx::Array<QuadricVertexRef> mVertexRefs;

	uint32_t mNumDeletedTriangles;
	uint32_t mNumDeletedVertices;
	uint32_t mNumDeletedHeapElements;
};

}
} // end namespace nvidia::apex

#endif