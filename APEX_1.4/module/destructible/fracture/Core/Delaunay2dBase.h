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
#ifndef DELAUNAY_2D_BASE_H
#define DELAUNAY_2D_BASE_H

#include <PxVec3.h>
#include <PsArray.h>
#include <PsUserAllocated.h>

namespace nvidia
{
namespace fracture
{
namespace base
{

// ------------------------------------------------------------------------------

struct Point;
struct Edge;
struct Triangle;
class SimScene;

// ------------------------------------------------------------------------------

class Delaunay2d : public UserAllocated {
	friend class SimScene;
public: 
	// singleton pattern
	//static Delaunay2d* getInstance();
	//static void destroyInstance();

	void triangulate(const PxVec3 *vertices, int numVerts, int byteStride, bool removeFarVertices = true);
	const nvidia::Array<int> getIndices() const { return mIndices; }

	// voronoi mesh, needs triangle mesh
	void computeVoronoiMesh();
	
	struct Convex {
		int firstVert;
		int numVerts;
		int firstNeighbor;
		int numNeighbors;
	};

	const nvidia::Array<Convex> getConvexes() const { return mConvexes; }
	const nvidia::Array<PxVec3> getConvexVerts() const { return mConvexVerts; }
	const nvidia::Array<int> getConvexNeighbors() const { return mConvexNeighbors; }

protected:
	Delaunay2d(SimScene* scene);
	virtual ~Delaunay2d();

	void clear();
	void delaunayTriangulation();

	struct Edge {
		Edge() {}
		Edge(int np0, int np1) { p0 = np0; p1 = np1; }
		bool operator == (const Edge &e) {
			return (p0 == e.p0 && p1 == e.p1) || (p0 == e.p1 && p1 == e.p0);
		}
		int p0, p1;
	};

	struct Triangle {
		int p0, p1, p2;
		PxVec3 center;
		float circumRadiusSquared;
	};

	void addTriangle(int p0, int p1, int p2);
	void getCircumSphere(const PxVec3 &p0, const PxVec3 &p1, const PxVec3 &p2,
		PxVec3 &center, float &radiusSquared);

	SimScene* mScene;

	nvidia::Array<PxVec3> mVertices;
	nvidia::Array<int> mIndices;

	nvidia::Array<Triangle> mTriangles;
	nvidia::Array<Edge> mEdges;

	nvidia::Array<Convex> mConvexes;
	nvidia::Array<PxVec3> mConvexVerts;
	nvidia::Array<int> mConvexNeighbors;

	int mFirstFarVertex;
};

}
}
}

// ------------------------------------------------------------------------------


#endif
#endif
