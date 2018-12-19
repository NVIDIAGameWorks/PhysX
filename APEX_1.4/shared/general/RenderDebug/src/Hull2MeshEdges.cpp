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


#include "Hull2MeshEdges.h"
#include "PsUserAllocated.h"
#include "PxVec4.h"
#include "PxVec3.h"
#include "PxPlane.h"
#include "PxMath.h"
#include "PxTransform.h"
#include "PsArray.h"


#define FLT_EPS 0.000000001f

PX_INLINE bool intersectPlanes(physx::PxVec3& pos, physx::PxVec3& dir, const physx::PxPlane& plane0, const physx::PxPlane& plane1)
{
	const physx::PxVec3 n0 = plane0.n;
	const physx::PxVec3 n1 = plane1.n;

	dir = n0.cross(n1);

	if(dir.normalize() < FLT_EPS)
	{
		return false;
	}

	pos = physx::PxVec3(0.0f);

	for (int iter = 3; iter--;)
	{
		// Project onto plane0:
		pos = plane0.project(pos);

		// Raycast to plane1:
		const physx::PxVec3 b = dir.cross(n0);
		//pos = pos - (pos.dot(plane1.n)/b.dot(plane1.n))*b;

		pos = pos - (plane1.distance(pos) / b.dot(plane1.n))*b;

	}

	return true;
}

void calculatePolygonEdges(physx::shdfnd::Array<HullEdge>& edges,uint32_t planeCount,const physx::PxPlane *planes)
{
	for (uint32_t i = 0; i <planeCount; ++i)
	{
		const physx::PxPlane& plane_i = planes[i];
		for (uint32_t j = i+1; j <planeCount; ++j)
		{
			const physx::PxPlane& plane_j = planes[j];
			// Find potential edge from intersection if plane_i and plane_j
			physx::PxVec3 orig;
			physx::PxVec3 edgeDir;
			if (intersectPlanes(orig, edgeDir, plane_i, plane_j))
			{
				float minS = -FLT_MAX;
				float maxS = FLT_MAX;
				bool intersectionFound = true;
				for (uint32_t k = 0; k < planeCount; ++k)
				{
					if (k == i || k == j)
					{
						continue;
					}
					const physx::PxPlane& plane_k = planes[k];
					const float num = -plane_k.distance(orig);
					const float den = plane_k.n.dot(edgeDir);
					if (physx::PxAbs(den) > FLT_EPS)
					{
						const float s = num/den;
						if (den > 0.0f)
						{
							maxS = physx::PxMin(maxS, s);
						}
						else
						{
							minS = physx::PxMax(minS, s);
						}
						if (maxS <= minS)
						{
							intersectionFound = false;
							break;
						}
					}
					else
						if (num < -10*FLT_EPS)
						{
							intersectionFound = false;
							break;
						}
				}
				if (intersectionFound && minS != -FLT_MAX && maxS != FLT_MIN)
				{
					HullEdge& s = edges.insert();
					s.e0 = orig + minS * edgeDir;
					s.e1 = orig + maxS * edgeDir;
				}
			}
		}
	}
}

struct ConvexMeshBuilder 
{
	ConvexMeshBuilder(void) : mVertices("ConvexMeshBuilder::mVertice"), mIndices("ConvexMeshBuilder::mIndices") {}
	void buildMesh(uint16_t numPlanes,const physx::PxVec4 *planes);
	physx::shdfnd::Array<physx::PxVec3> mVertices;
	physx::shdfnd::Array<uint16_t> mIndices;
};

static float det(physx::PxVec4 v0, physx::PxVec4 v1, physx::PxVec4 v2, physx::PxVec4 v3)
{
	const physx::PxVec3& d0 = reinterpret_cast<const physx::PxVec3&>(v0);
	const physx::PxVec3& d1 = reinterpret_cast<const physx::PxVec3&>(v1);
	const physx::PxVec3& d2 = reinterpret_cast<const physx::PxVec3&>(v2);
	const physx::PxVec3& d3 = reinterpret_cast<const physx::PxVec3&>(v3);

	return v0.w * d1.cross(d2).dot(d3)
		- v1.w * d0.cross(d2).dot(d3)
		+ v2.w * d0.cross(d1).dot(d3)
		- v3.w * d0.cross(d1).dot(d2);
}

static physx::PxVec3 intersect(physx::PxVec4 p0, physx::PxVec4 p1, physx::PxVec4 p2)
{
	const physx::PxVec3& d0 = reinterpret_cast<const physx::PxVec3&>(p0);
	const physx::PxVec3& d1 = reinterpret_cast<const physx::PxVec3&>(p1);
	const physx::PxVec3& d2 = reinterpret_cast<const physx::PxVec3&>(p2);

	return (p0.w * d1.cross(d2) 
			+ p1.w * d2.cross(d0) 
			+ p2.w * d0.cross(d1))
			/ d0.dot(d2.cross(d1));
}

static const uint16_t sInvalid = uint16_t(-1);

// restriction: only supports a single patch per vertex.
struct HalfedgeMesh
{
	struct Halfedge
	{
		Halfedge(uint16_t vertex = sInvalid, uint16_t face = sInvalid, 
			uint16_t next = sInvalid, uint16_t prev = sInvalid)
			: mVertex(vertex), mFace(face), mNext(next), mPrev(prev)
		{}

		uint16_t mVertex; // to
		uint16_t mFace; // left
		uint16_t mNext; // ccw
		uint16_t mPrev; // cw
	};

	HalfedgeMesh() : mNumTriangles(0), mHalfedges("mHalfedges"),mVertices("mVertices"),mFaces("mFaces"),mPoints("mPoints") {}

	uint16_t findHalfedge(uint16_t v0, uint16_t v1)
	{
		uint16_t h = mVertices[v0], start = h;
		while(h != sInvalid && mHalfedges[h].mVertex != v1)
		{
			h = mHalfedges[h ^ 1u].mNext;
			if(h == start) 
				return sInvalid;
		}
		return h;
	}

	void connect(uint16_t h0, uint16_t h1)
	{
		mHalfedges[h0].mNext = h1;
		mHalfedges[h1].mPrev = h0;
	}

	void addTriangle(uint16_t v0, uint16_t v1, uint16_t v2)
	{
		// add new vertices
		uint16_t n = physx::PxMax(v0, physx::PxMax(v1, v2))+1u;
		if(mVertices.size() < n)
			mVertices.resize(n, sInvalid);

		// collect halfedges, prev and next of triangle
		uint16_t verts[] = { v0, v1, v2 };
		uint16_t handles[3], prev[3], next[3];
		for(uint16_t i=0; i<3; ++i)
		{
			uint16_t j = (i+1u)%3;
			uint16_t h = findHalfedge(verts[i], verts[j]);
			if(h == sInvalid)
			{
				// add new edge
				h = uint16_t(mHalfedges.size());
				mHalfedges.pushBack(Halfedge(verts[j]));
				mHalfedges.pushBack(Halfedge(verts[i]));
			}
			handles[i] = h;
			prev[i] = mHalfedges[h].mPrev;
			next[i] = mHalfedges[h].mNext;
		}

		// patch connectivity
		for(uint16_t i=0; i<3; ++i)
		{
			uint16_t j = (i+1u)%3;

			mHalfedges[handles[i]].mFace = uint16_t(mFaces.size());

			// connect prev and next
			connect(handles[i], handles[j]);

			if(next[j] == sInvalid) // new next edge, connect opposite
				connect(handles[j]^1u, next[i]!=sInvalid ? next[i] : handles[i]^1u);

			if(prev[i] == sInvalid) // new prev edge, connect opposite
				connect(prev[j]!=sInvalid ? prev[j] : handles[j]^1u, handles[i]^1u);

			// prev is boundary, update middle vertex
			if(mHalfedges[handles[i]^1u].mFace == sInvalid)
				mVertices[verts[j]] = handles[i]^1u;
		}

		PX_ASSERT(mNumTriangles < 0xffff);
		mFaces.pushBack(handles[2]);
		++mNumTriangles;
	}

	uint16_t removeTriangle(uint16_t f)
	{
		uint16_t result = sInvalid;

		for(uint16_t i=0, h = mFaces[f]; i<3; ++i)
		{
			uint16_t v0 = mHalfedges[h^1u].mVertex;
			uint16_t v1 = mHalfedges[h].mVertex;

			mHalfedges[h].mFace = sInvalid;

			if(mHalfedges[h^1u].mFace == sInvalid) // was boundary edge, remove
			{
				uint16_t v0Prev = mHalfedges[h  ].mPrev;
				uint16_t v0Next = mHalfedges[h^1u].mNext;
				uint16_t v1Prev = mHalfedges[h^1u].mPrev;
				uint16_t v1Next = mHalfedges[h  ].mNext;

				// update halfedge connectivity
				connect(v0Prev, v0Next);
				connect(v1Prev, v1Next);

				// update vertex boundary or delete
				mVertices[v0] = (v0Prev^1) == v0Next ? sInvalid : v0Next;
				mVertices[v1] = (v1Prev^1) == v1Next ? sInvalid : v1Next;
			} 
			else 
			{
				mVertices[v0] = h; // update vertex boundary
				result = v1;
			}

			h = mHalfedges[h].mNext;
		}

		mFaces[f] = sInvalid;
		--mNumTriangles;

		return result;
	}

	// true if vertex v is in front of face f
	bool visible(uint16_t v, uint16_t f)
	{
		uint16_t h = mFaces[f];
		if(h == sInvalid)
			return false;

		uint16_t v0 = mHalfedges[h].mVertex;
		h = mHalfedges[h].mNext;
		uint16_t v1 = mHalfedges[h].mVertex;
		h = mHalfedges[h].mNext;
		uint16_t v2 = mHalfedges[h].mVertex;
		h = mHalfedges[h].mNext;

		return det(mPoints[v], mPoints[v0], mPoints[v1], mPoints[v2]) < -1e-5f;
	}

	uint16_t mNumTriangles;
	physx::shdfnd::Array<Halfedge> mHalfedges;
	physx::shdfnd::Array<uint16_t> mVertices; // vertex -> (boundary) halfedge
	physx::shdfnd::Array<uint16_t> mFaces; // face -> halfedge
	physx::shdfnd::Array<physx::PxVec4> mPoints;
};

void ConvexMeshBuilder::buildMesh(uint16_t numPlanes,const physx::PxVec4 *planes)
{
	if(numPlanes < 4)
		return; // todo: handle degenerate cases

	HalfedgeMesh mesh;

	// gather points (planes, that is)
	mesh.mPoints.reserve(numPlanes);
	for (uint32_t i=0; i<numPlanes; i++)
	{
		mesh.mPoints.pushBack(planes[i]);
	}

	// initialize to tetrahedron
	mesh.addTriangle(0, 1, 2);
	mesh.addTriangle(0, 3, 1);
	mesh.addTriangle(1, 3, 2);
	mesh.addTriangle(2, 3, 0);

	// flip if inside-out
	if(mesh.visible(3, 0))
		physx::shdfnd::swap(mesh.mPoints[0], mesh.mPoints[1]);

	// iterate through remaining points
	for(uint16_t i=4; i<mesh.mPoints.size(); ++i)
	{
		// remove any visible triangle
		uint16_t v0 = sInvalid;
		for(uint16_t j=0; j<mesh.mFaces.size(); ++j)
		{
			if(mesh.visible(i, j))
				v0 = physx::PxMin(v0, mesh.removeTriangle(j));
		}

		if(v0 == sInvalid)
			continue; // no triangle removed

		if(!mesh.mNumTriangles)
			return; // empty mesh

		// find non-deleted boundary vertex
		for(uint16_t h=0; mesh.mVertices[v0] == sInvalid; h+=2)
		{
			if ((mesh.mHalfedges[h  ].mFace == sInvalid) ^ 
				(mesh.mHalfedges[h+1u].mFace == sInvalid))
			{
				v0 = mesh.mHalfedges[h].mVertex;
			}
		}

		// tesselate hole
		uint16_t start = v0;
		do {
			uint16_t h = mesh.mVertices[v0];
			uint16_t v1 = mesh.mHalfedges[h].mVertex;
			mesh.addTriangle(v0, v1, i);
			v0 = v1;
		} while(v0 != start);
	}

	// convert triangles to vertices (intersection of 3 planes)
	physx::shdfnd::Array<uint32_t> face2Vertex(mesh.mFaces.size(), sInvalid);
	for(uint32_t i=0; i<mesh.mFaces.size(); ++i)
	{
		uint16_t h = mesh.mFaces[i];
		if(h == sInvalid)
			continue;

		uint16_t v0 = mesh.mHalfedges[h].mVertex;
		h = mesh.mHalfedges[h].mNext;
		uint16_t v1 = mesh.mHalfedges[h].mVertex;
		h = mesh.mHalfedges[h].mNext;
		uint16_t v2 = mesh.mHalfedges[h].mVertex;

		face2Vertex[i] = mVertices.size();
		mVertices.pushBack(intersect(mesh.mPoints[v0], mesh.mPoints[v1], mesh.mPoints[v2]));
	}

	// convert vertices to polygons (face one-ring)
	for(uint32_t i=0; i<mesh.mVertices.size(); ++i)
	{
		uint16_t h = mesh.mVertices[i];
		if(h == sInvalid)
			continue;

		uint16_t v0 = uint16_t(face2Vertex[mesh.mHalfedges[h].mFace]);
		h = mesh.mHalfedges[h].mPrev^1u;
		uint16_t v1 = uint16_t(face2Vertex[mesh.mHalfedges[h].mFace]);
		bool state = true;
		while( state )
		{
			h = mesh.mHalfedges[h].mPrev^1u;
			uint16_t v2 = uint16_t(face2Vertex[mesh.mHalfedges[h].mFace]);

			if(v0 == v2) 
				break;

			physx::PxVec3 e0 =  mVertices[v0] -  mVertices[v2];
			physx::PxVec3 e1 =  mVertices[v1] -  mVertices[v2];
			if(e0.cross(e1).magnitudeSquared() < 1e-10f)
				continue;

			mIndices.pushBack(v0);
			mIndices.pushBack(v2);
			mIndices.pushBack(v1);

			v1 = v2; 
		}

	}
}

class Hull2MeshEdgesImpl : public Hull2MeshEdges, public physx::shdfnd::UserAllocated
{
public:
	Hull2MeshEdgesImpl(void) : mEdges("HullEdges")
	{
		
	}
	virtual ~Hull2MeshEdgesImpl(void)
	{
	}

	virtual bool getHullMesh(uint32_t planeCount,const physx::PxPlane *planes,HullMesh &m) 
	{
		bool ret = false;

		mConvexMeshBuilder.buildMesh(uint16_t(planeCount),reinterpret_cast<const physx::PxVec4 *>(planes));

		m.mVertexCount = mConvexMeshBuilder.mVertices.size();
		if ( m.mVertexCount )
		{
			ret = true;
			m.mTriCount = mConvexMeshBuilder.mIndices.size()/3;
			m.mVertices = &mConvexMeshBuilder.mVertices[0];
			m.mIndices = &mConvexMeshBuilder.mIndices[0];
		}
		return ret;
	}

	virtual const HullEdge *getHullEdges(uint32_t planeCount,const physx::PxPlane *planes,uint32_t &edgeCount) 
	{
		const HullEdge *ret = NULL;

		mEdges.clear();
		calculatePolygonEdges(mEdges,planeCount,planes);
		if ( !mEdges.empty() )
		{
			ret = &mEdges[0];
			edgeCount = mEdges.size();
		}

		return ret;
	}

	virtual void release(void)
	{
		delete this;
	}
	physx::shdfnd::Array<HullEdge> mEdges;
	ConvexMeshBuilder	mConvexMeshBuilder;
};

Hull2MeshEdges *createHull2MeshEdges(void)
{
	Hull2MeshEdgesImpl *ret = PX_NEW(Hull2MeshEdgesImpl);
	return static_cast< Hull2MeshEdges *>(ret);
}
