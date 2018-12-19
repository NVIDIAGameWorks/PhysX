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



#include "RTdef.h"
#if RT_COMPILE
#include "Delaunay2dBase.h"
#include "PxBounds3.h"
#include <PxMath.h>

namespace nvidia
{
namespace fracture
{
namespace base
{

using namespace nvidia;
// -------------------------------------------------------------------------------------
Delaunay2d::Delaunay2d(SimScene* scene):
	mScene(scene)
{
}

// -------------------------------------------------------------------------------------
Delaunay2d::~Delaunay2d() 
{
}

// -------------------------------------------------------------------------------------
void Delaunay2d::clear()
{
	mVertices.clear();
	mIndices.clear();
	mTriangles.clear();
	mFirstFarVertex = 0;

	mConvexVerts.clear();
	mConvexes.clear();
	mConvexNeighbors.clear();
}

// -------------------------------------------------------------------------------------
void Delaunay2d::triangulate(const PxVec3 *vertices, int numVerts, int byteStride, bool removeFarVertices)
{
	clear();

	uint8_t *vp = (uint8_t*)vertices;
	mVertices.resize((uint32_t)numVerts);
	for (uint32_t i = 0; i < (uint32_t)numVerts; i++) {
		mVertices[i] = (*(PxVec3*)vp);
		mVertices[i].z = 0.0f;
		vp += byteStride;
	}

	delaunayTriangulation();

	for (uint32_t i = 0; i < mTriangles.size(); i++) {
		Triangle &t = mTriangles[i];
		if (!removeFarVertices || (t.p0 < mFirstFarVertex && t.p1 < mFirstFarVertex && t.p2 < mFirstFarVertex)) {
			mIndices.pushBack(t.p0);
			mIndices.pushBack(t.p1);
			mIndices.pushBack(t.p2);
		}
	}

	if (removeFarVertices)
		mVertices.resize((uint32_t)mFirstFarVertex);
}


// -------------------------------------------------------------------------------------
void Delaunay2d::delaunayTriangulation()
{
	PxBounds3 bounds;
	bounds.setEmpty();

	for (uint32_t i = 0; i < mVertices.size(); i++) 
		bounds.include(mVertices[i]);

	bounds.fattenSafe(bounds.getDimensions().magnitude());

	// start with two triangles
	//float scale = 10.0f;
	PxVec3 p0(bounds.minimum.x, bounds.minimum.y, 0.0f);
	PxVec3 p1(bounds.maximum.x, bounds.minimum.y, 0.0f);
	PxVec3 p2(bounds.maximum.x, bounds.maximum.y, 0.0f);
	PxVec3 p3(bounds.minimum.x, bounds.maximum.y, 0.0f);

	mFirstFarVertex = (int32_t)mVertices.size();
	mVertices.pushBack(p0);
	mVertices.pushBack(p1);
	mVertices.pushBack(p2);
	mVertices.pushBack(p3);

	mTriangles.clear();
	addTriangle(mFirstFarVertex, mFirstFarVertex+1, mFirstFarVertex+2);
	addTriangle(mFirstFarVertex, mFirstFarVertex+2, mFirstFarVertex+3);

	// insert other points
	for (uint32_t i = 0; i < (uint32_t)mFirstFarVertex; i++) {
		mEdges.clear();
		uint32_t j = 0;
		while (j < mTriangles.size()) {
			Triangle &t = mTriangles[j];
			if ((t.center - mVertices[i]).magnitudeSquared() > t.circumRadiusSquared) {
				j++;
				continue;
			}
			Edge e0(t.p0, t.p1);
			Edge e1(t.p1, t.p2);
			Edge e2(t.p2, t.p0);
			bool found0 = false;
			bool found1 = false;
			bool found2 = false;

			uint32_t k = 0;
			while (k < mEdges.size()) {
				Edge &e = mEdges[k];
				bool found = false;
				if (e == e0) { found0 = true; found = true; }
				if (e == e1) { found1 = true; found = true; }
				if (e == e2) { found2 = true; found = true; }
				if (found) {
					mEdges[k] = mEdges[mEdges.size()-1];
					mEdges.popBack();
				}
				else k++;
			}
			if (!found0) mEdges.pushBack(e0);
			if (!found1) mEdges.pushBack(e1);
			if (!found2) mEdges.pushBack(e2);
			mTriangles[j] = mTriangles[mTriangles.size()-1];
			mTriangles.popBack();
		}
		for (j = 0; j < mEdges.size(); j++) {
			Edge &e = mEdges[j];
			addTriangle(e.p0, e.p1, (int32_t)i);
		}
	}
}

// -------------------------------------------------------------------------------------
void Delaunay2d::addTriangle(int p0, int p1, int p2)
{
	Triangle triangle;
	triangle.p0 = p0;
	triangle.p1 = p1;
	triangle.p2 = p2;
	getCircumSphere(mVertices[(uint32_t)p0], mVertices[(uint32_t)p1], mVertices[(uint32_t)p2], triangle.center, triangle.circumRadiusSquared);
	mTriangles.pushBack(triangle);
}

// -------------------------------------------------------------------------------------
void Delaunay2d::getCircumSphere(const PxVec3 &p0, const PxVec3 &p1, const PxVec3 &p2,
		PxVec3 &center, float &radiusSquared)
{
	float x1 = p1.x - p0.x;
	float y1 = p1.y - p0.y;
	float x2 = p2.x - p0.x;
	float y2 = p2.y - p0.y;

	float det = x1 * y2 - x2 * y1;
	if (det == 0.0f) {
		center = p0; radiusSquared = 0.0f;
		return;
	}
	det = 0.5f / det;
	float len1 = x1*x1 + y1*y1;
	float len2 = x2*x2 + y2*y2;
	float cx = (len1 * y2 - len2 * y1) * det;
	float cy = (len2 * x1 - len1 * x2) * det;
	center.x = p0.x + cx;
	center.y = p0.y + cy;
	center.z = 0.0f;
	radiusSquared = cx * cx + cy * cy;
}

// -------------------------------------------------------------------------------------
void Delaunay2d::computeVoronoiMesh()
{
	mConvexes.clear(); 
	mConvexVerts.clear(); 
	mConvexNeighbors.clear(); 

	uint32_t numVerts = mVertices.size();
	uint32_t numTris = mIndices.size() / 3;

	// center positions
	nvidia::Array<PxVec3> centers(numTris);
	for (uint32_t i = 0; i < numTris; i++) {
		PxVec3 &p0 = mVertices[(uint32_t)mIndices[3*i]];
		PxVec3 &p1 = mVertices[(uint32_t)mIndices[3*i+1]];
		PxVec3 &p2 = mVertices[(uint32_t)mIndices[3*i+2]];
		float r2;
		getCircumSphere(p0,p1,p2, centers[i], r2);
	}

	// vertex -> triangles links
	nvidia::Array<int> firstVertTri(numVerts+1, 0);
	nvidia::Array<int> vertTris;

	for (uint32_t i = 0; i < numTris; i++) {
		firstVertTri[(uint32_t)mIndices[3*i]]++;
		firstVertTri[(uint32_t)mIndices[3*i+1]]++;
		firstVertTri[(uint32_t)mIndices[3*i+2]]++;
	}

	int numLinks = 0;
	for (uint32_t i = 0; i < numVerts; i++) {
		numLinks += firstVertTri[i];
		firstVertTri[i] = numLinks;
	}
	firstVertTri[numVerts] = numLinks;
	vertTris.resize((uint32_t)numLinks);

	for (uint32_t i = 0; i < numTris; i++) {
		uint32_t &i0 = (uint32_t&)firstVertTri[(uint32_t)mIndices[3*i]];
		uint32_t &i1 = (uint32_t&)firstVertTri[(uint32_t)mIndices[3*i+1]];
		uint32_t &i2 = (uint32_t&)firstVertTri[(uint32_t)mIndices[3*i+2]];
		i0--; vertTris[i0] = (int32_t)i;
		i1--; vertTris[i1] = (int32_t)i;
		i2--; vertTris[i2] = (int32_t)i;
	}

	// convexes
	Convex c;
	nvidia::Array<int> nextVert(numVerts, -1);
	nvidia::Array<int> vertVisited(numVerts, -1);
	nvidia::Array<int> triOfVert(numVerts, -1);
	nvidia::Array<int> convexOfVert(numVerts, -1);

	for (uint32_t i = 0; i < numVerts; i++) {
		int first = firstVertTri[i];
		int last = firstVertTri[i+1];
		int num = last - first;
		if (num < 3)
			continue;

		int start = -1;

		for (int j = first; j < last; j++) {
			int triNr = vertTris[(uint32_t)j];

			int k = 0;
			while (k < 3 && mIndices[uint32_t(3*triNr+k)] != (int32_t)i)
				k++;

			int j0 = mIndices[uint32_t(3*triNr + (k+1)%3)];
			int j1 = mIndices[uint32_t(3*triNr + (k+2)%3)];

			if (j == first)
				start = j0;

			nextVert[(uint32_t)j0] = j1;
			vertVisited[(uint32_t)j0] = 2*(int32_t)i;
			triOfVert[(uint32_t)j0] = triNr;
		}

		c.firstNeighbor = (int32_t)mConvexNeighbors.size();
		c.firstVert = (int32_t)mConvexVerts.size();
		c.numVerts = num;
		bool rollback = false;
		int id = start;
		do {
			if (vertVisited[(uint32_t)id] != 2*(int32_t)i) {
				rollback = true;
				break;
			}
			vertVisited[(uint32_t)id] = 2*(int32_t)i+1;

			mConvexVerts.pushBack(centers[(uint32_t)triOfVert[(uint32_t)id]]);
			mConvexNeighbors.pushBack(id);
			id = nextVert[(uint32_t)id];
		} while (id != start);

		if (rollback) {
			mConvexVerts.resize((uint32_t)c.firstVert);
			mConvexNeighbors.resize((uint32_t)c.firstNeighbor);
			continue;
		}

		c.numVerts = (int32_t)mConvexVerts.size() - c.firstVert;
		c.numNeighbors = (int32_t)mConvexNeighbors.size() - c.firstNeighbor;
		convexOfVert[i] = (int32_t)mConvexes.size();
		mConvexes.pushBack(c);
	}

	// compute neighbors
	uint32_t newPos = 0;
	for (uint32_t i = 0; i < mConvexes.size(); i++) {
		Convex &c = mConvexes[i];
		uint32_t pos = (uint32_t)c.firstNeighbor;
		uint32_t num = (uint32_t)c.numNeighbors;
		c.firstNeighbor = (int32_t)newPos;
		c.numNeighbors = 0;
		for (uint32_t j = 0; j < num; j++) {
			int n = convexOfVert[(uint32_t)mConvexNeighbors[pos+j]];
			if (n >= 0) {
				mConvexNeighbors[newPos] = n;
				newPos++;
				c.numNeighbors++;
			}
		}
	}
	mConvexNeighbors.resize(newPos);
}

}
}
}
#endif