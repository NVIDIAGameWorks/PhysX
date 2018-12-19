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



#ifndef APEX_SUBDIVIDER_H
#define APEX_SUBDIVIDER_H

#include "ApexDefs.h"
#include "PsUserAllocated.h"
#include "ApexRand.h"
#include "PsArray.h"
#include "ApexUsingNamespace.h"

#include "PxBounds3.h"

namespace nvidia
{
namespace apex
{

class IProgressListener;

class ApexSubdivider : public UserAllocated
{
public:
	ApexSubdivider();

	void clear();

	void registerVertex(const PxVec3& v, uint32_t bitFlagPayload);
	void registerTriangle(uint32_t i0, uint32_t i1, uint32_t i2);

	void endRegistration();

	void mergeVertices(IProgressListener* progress);
	void closeMesh(IProgressListener* progress);
	void subdivide(uint32_t subdivisionGridSize, IProgressListener* progress);

	uint32_t getNumVertices() const;
	uint32_t getNumTriangles() const;

	void getVertex(uint32_t i, PxVec3& v, uint32_t& bitFlagPayload) const;
	void getTriangle(uint32_t i, uint32_t& i0, uint32_t& i1, uint32_t& i2) const;

private:
	void compress();
	void closeHole(uint32_t* indices, uint32_t numIndices);
	float qualityOfTriangle(uint32_t v0, uint32_t v1, uint32_t v2) const;
	int32_t getTriangleNr(const uint32_t v0, const uint32_t v1, const uint32_t v2) const;

	PxBounds3 mBound;

	struct SubdividerVertex
	{
		SubdividerVertex() : pos(0.0f, 0.0f, 0.0f), firstTriangle(-1), payload(0), marked(false) {}
		SubdividerVertex(const PxVec3& newPos, uint32_t bitFlagPayload) : pos(newPos), firstTriangle(-1), payload(bitFlagPayload), marked(false) {}

		PxVec3 pos;
		int32_t firstTriangle;
		uint32_t payload;
		bool marked;
	};

	struct SubdividerVertexRef
	{
		SubdividerVertexRef() : pos(0.0f, 0.0f, 0.0f), vertexNr(0) {}
		SubdividerVertexRef(const PxVec3& p, uint32_t vNr) : pos(p), vertexNr(vNr) {}
		PX_INLINE bool operator < (const SubdividerVertexRef& vr) const
		{
			return pos.x < vr.pos.x;
		}

		PX_INLINE bool operator()(const SubdividerVertexRef& v1, const SubdividerVertexRef& v2) const
		{
			return v1 < v2;
		}

		PxVec3 pos;
		uint32_t vertexNr;
	};



	struct SubdividerEdge
	{
		void init(uint32_t newV0, uint32_t newV1, uint32_t newTriangleNr)
		{
			v0 = PxMax(newV0, newV1);
			v1 = PxMin(newV0, newV1);
			triangleNr = newTriangleNr;
		}
		PX_INLINE bool operator < (const SubdividerEdge& e) const
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
		PX_INLINE bool operator()(const SubdividerEdge& e1, const SubdividerEdge& e2) const
		{
			return e1 < e2;
		}
		PX_INLINE bool operator == (const SubdividerEdge& e) const
		{
			return v0 == e.v0 && v1 == e.v1;
		}
		uint32_t v0, v1;
		uint32_t triangleNr;
	};

	int32_t binarySearchEdges(const Array<SubdividerEdge>& edges, uint32_t v0, uint32_t v1, uint32_t triangleNr) const;

	struct SubdividerTriangle
	{
		void init(uint32_t v0, uint32_t v1, uint32_t v2)
		{
			vertexNr[0] = v0;
			vertexNr[1] = v1;
			vertexNr[2] = v2;
		}

		bool containsVertex(uint32_t vNr) const
		{
			return vertexNr[0] == vNr || vertexNr[1] == vNr || vertexNr[2] == vNr;
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
				PX_ASSERT(0 && "replaceVertex failed");
			}
		}

		bool operator == (SubdividerTriangle& t) const
		{
			return
			    t.containsVertex(vertexNr[0]) &&
			    t.containsVertex(vertexNr[1]) &&
			    t.containsVertex(vertexNr[2]);
		}
		bool isValid() const
		{
			return (vertexNr[0] != vertexNr[1] && vertexNr[0] != vertexNr[2] && vertexNr[1] != vertexNr[2]);
		}

		uint32_t vertexNr[3];
	};

	struct TriangleList
	{
		TriangleList() : triangleNumber(0), nextTriangle(-1) {}
		TriangleList(uint32_t tNr) : triangleNumber(tNr), nextTriangle(-1) {}

		uint32_t triangleNumber;
		int32_t nextTriangle;
	};

	Array<SubdividerVertex> mVertices;
	Array<SubdividerTriangle> mTriangles;
	uint32_t mMarkedVertices;

	QDSRand mRand;

	Array<TriangleList> mTriangleList;
	int32_t mTriangleListEmptyElement;
	void addTriangleToVertex(uint32_t vertexNumber, uint32_t triangleNumber);
	void removeTriangleFromVertex(uint32_t vertexNumber, uint32_t triangleNumber);
	TriangleList& allocateTriangleElement();
	void freeTriangleElement(uint32_t index);
};

}
} // end namespace nvidia::apex

#endif
