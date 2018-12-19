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
#include "Delaunay3dBase.h"
#include "PsSort.h"
#include <PxAssert.h>

#include "PxBounds3.h"

namespace nvidia
{
namespace fracture
{
namespace base
{

using namespace nvidia;

// side orientation points outwards
const int Delaunay3d::Tetra::sideIndices[4][3] = {{1,2,3},{2,0,3},{0,1,3},{2,1,0}};

// ------ singleton pattern -----------------------------------------------------------
//
//static Delaunay3d *gDelaunay3d = NULL;
//
//Delaunay3d* Delaunay3d::getInstance() 
//{
//	if (gDelaunay3d == NULL) {
//		gDelaunay3d = PX_NEW(Delaunay3d)();
//	}
//	return gDelaunay3d;
//}
//
//void Delaunay3d::destroyInstance() 
//{
//	if (gDelaunay3d != NULL) {
//		PX_DELETE(gDelaunay3d);
//	}
//	gDelaunay3d = NULL;
//}

// -----------------------------------------------------------------------------
void Delaunay3d::tetrahedralize(const PxVec3 *vertices,  int numVerts, int byteStride, bool removeExtraVerts)
{
	clear();

	uint8_t *vp = (uint8_t*)vertices;
	for (int i = 0; i < numVerts; i++) {
		mVertices.pushBack(*(PxVec3*)vp);
		vp += byteStride;
	}

	delaunayTetrahedralization();

	compressTetrahedra(removeExtraVerts);
	if (removeExtraVerts) {
		mVertices.resize((uint32_t)mFirstFarVertex);
	}

	for (uint32_t i = 0; i < mTetras.size(); i++) {
		Tetra &t = mTetras[i];
		mIndices.pushBack(t.ids[0]);
		mIndices.pushBack(t.ids[1]);
		mIndices.pushBack(t.ids[2]);
		mIndices.pushBack(t.ids[3]);
	}
}

// -----------------------------------------------------------------------------
void Delaunay3d::clear()
{
	mVertices.clear();
	mIndices.clear();
	mTetras.clear();
	mFirstFarVertex = 0;
	mLastFarVertex = 0;

	mGeom.clear();

	mTetMarked.clear();
	mTetMark = 0;
}

// -----------------------------------------------------------------------------
void Delaunay3d::delaunayTetrahedralization()
{
	int i,j;

	nvidia::Array<Edge> edges;
	Edge edge;

	PxBounds3 bounds;
	bounds.setEmpty();
	for (uint32_t i = 0; i < mVertices.size(); i++) 
		bounds.include(mVertices[i]);

	// start with large tetrahedron
	mTetras.clear();
	float a = 3.0f * bounds.getDimensions().magnitude();
	float x  = 0.5f * a;
	float y0 = x / sqrtf(3.0f);
	float y1 = x * sqrtf(3.0f) - y0;
	float z0 = 0.25f * sqrtf(6.0f) * a;
	float z1 = a * sqrtf(6.0f) / 3.0f - z0;

	PxVec3 center = bounds.getCenter();

	mFirstFarVertex = (int32_t)mVertices.size();
	PxVec3 p0(-x,-y0,-z1); mVertices.pushBack(center + p0);
	PxVec3 p1( x,-y0,-z1); mVertices.pushBack(center + p1);
	PxVec3 p2( 0, y1,-z1); mVertices.pushBack(center + p2);
	PxVec3 p3( 0,  0, z0); mVertices.pushBack(center + p3);
	mLastFarVertex = (int32_t)mVertices.size()-1;

	Tetra tetra;
	tetra.init(mFirstFarVertex, mFirstFarVertex+1, mFirstFarVertex+2, mFirstFarVertex+3);
	mTetras.pushBack(tetra);

	// build Delaunay triangulation iteratively

	int lastVertex = (int)mVertices.size()-4;
	for (i = 0; i < lastVertex; i++) {
		PxVec3 &v = mVertices[(uint32_t)i];

		int tNr = findSurroundingTetra((int32_t)mTetras.size()-1, v);	// fast walk
		if (tNr < 0)
			continue;
		// assert(tNr >= 0);

		int newTetraNr = (int32_t)mTetras.size();
		retriangulate(tNr, i);
		edges.clear();
		for (j = newTetraNr; j < (int)mTetras.size(); j++) {
			Tetra &newTet = mTetras[(uint32_t)j];
			edge.init(newTet.ids[2], newTet.ids[3], j,1);
			edges.pushBack(edge);
			edge.init(newTet.ids[3], newTet.ids[1], j,2);
			edges.pushBack(edge);
			edge.init(newTet.ids[1], newTet.ids[2], j,3);
			edges.pushBack(edge);
		}
		shdfnd::sort(edges.begin(), edges.size());
		for (j = 0; j < (int)edges.size(); j += 2) {
			Edge &edge0 = edges[(uint32_t)j];
			Edge &edge1 = edges[(uint32_t)j+1];
			PX_ASSERT(edge0 == edge1);
			mTetras[(uint32_t)edge0.tetraNr].neighborNrs[(uint32_t)edge0.neighborNr] = edge1.tetraNr;
			mTetras[(uint32_t)edge1.tetraNr].neighborNrs[(uint32_t)edge1.neighborNr] = edge0.tetraNr;
		}
	}
}

// -----------------------------------------------------------------------------
int Delaunay3d::findSurroundingTetra(int startTetra, const PxVec3 &p) const
{
	// find the tetrahedra which contains the vertex
	// by walking through mesh O(n ^ (1/3))

	mTetMark++;

	if (mTetras.size() == 0) return -1;
	int tetNr = startTetra;
	PX_ASSERT(!mTetras[(uint32_t)startTetra].deleted);

	if (mTetMarked.size() < mTetras.size())
		mTetMarked.resize(mTetras.size(), -1);

	bool loop = false;

	while (tetNr >= 0) {
		if (mTetMarked[(uint32_t)tetNr] == mTetMark) {
			loop = true;
			break;
		}
		mTetMarked[(uint32_t)tetNr] = mTetMark;

		const Tetra &tetra = mTetras[(uint32_t)tetNr];
		const PxVec3 &p0 = mVertices[(uint32_t)tetra.ids[0]];
		PxVec3 q  = p-p0;
		PxVec3 q0 = mVertices[(uint32_t)tetra.ids[1]] - p0;
		PxVec3 q1 = mVertices[(uint32_t)tetra.ids[2]] - p0;
		PxVec3 q2 = mVertices[(uint32_t)tetra.ids[3]] - p0;
		PxMat33 m;
		m.column0 = q0;
		m.column1 = q1;
		m.column2 = q2;
		float det = m.getDeterminant();
		m.column0 = q;
		float x = m.getDeterminant();
		if (x < 0.0f && tetra.neighborNrs[1] >= 0) {
			tetNr = tetra.neighborNrs[1];
			continue;
		}
		m.column0 = q0; m.column1 = q;
		float y = m.getDeterminant();
		if (y < 0.0f && tetra.neighborNrs[2] >= 0) {
			tetNr = tetra.neighborNrs[2];
			continue;
		}
		m.column1 = q1; m.column2 = q;
		float z = m.getDeterminant();
		if (z < 0.0f && tetra.neighborNrs[3] >= 0) {
			tetNr = tetra.neighborNrs[3];
			continue;
		}
		if (x + y + z > det  && tetra.neighborNrs[0] >= 0) {
			tetNr = tetra.neighborNrs[0];
			continue;
		}
		return tetNr;
	}
	if (loop) {		// search failed, brute force:
		for (uint32_t i = 0; i < mTetras.size(); i++) {
			const Tetra &tetra = mTetras[i];
			if (tetra.deleted)
				continue;

			const PxVec3 &p0 = mVertices[(uint32_t)tetra.ids[0]];
			const PxVec3 &p1 = mVertices[(uint32_t)tetra.ids[1]];
			const PxVec3 &p2 = mVertices[(uint32_t)tetra.ids[2]];
			const PxVec3 &p3 = mVertices[(uint32_t)tetra.ids[3]];

			PxVec3 n;
			n = (p1-p0).cross(p2-p0);
			if (n.dot(p) < n.dot(p0)) continue;
			n = (p2-p0).cross(p3-p0);
			if (n.dot(p) < n.dot(p0)) continue;
			n = (p3-p0).cross(p1-p0);
			if (n.dot(p) < n.dot(p0)) continue;
			n = (p3-p1).cross(p2-p1);
			if (n.dot(p) < n.dot(p1)) continue;
			return (int32_t)i;
		}
		return -1;
	}
	else
		return -1;
}

// -----------------------------------------------------------------------------
void Delaunay3d::updateCircumSphere(Tetra &tetra)
{
	if (!tetra.circumsphereDirty)  
		return;
	PxVec3 p0 = mVertices[(uint32_t)tetra.ids[0]];
	PxVec3 b  = mVertices[(uint32_t)tetra.ids[1]] - p0;
	PxVec3 c  = mVertices[(uint32_t)tetra.ids[2]] - p0;
	PxVec3 d  = mVertices[(uint32_t)tetra.ids[3]] - p0;
	float det = b.x*(c.y*d.z - c.z*d.y) - b.y*(c.x*d.z - c.z*d.x) + b.z*(c.x*d.y-c.y*d.x);
	if (det == 0.0f) {
		tetra.center = p0; tetra.radiusSquared = 0.0f; 
		return; // singular case
	}
	det *= 2.0f;
	PxVec3 v = c.cross(d)*b.dot(b) + d.cross(b)*c.dot(c) +  b.cross(c)*d.dot(d);
	v /= det;
	tetra.radiusSquared = v.magnitudeSquared();
	tetra.center = p0 + v;
	tetra.circumsphereDirty = false;
}

// -----------------------------------------------------------------------------
bool Delaunay3d::pointInCircumSphere(Tetra &tetra, const PxVec3 &p)
{
	updateCircumSphere(tetra);
	return (tetra.center - p).magnitudeSquared() < tetra.radiusSquared;
}

// -----------------------------------------------------------------------------
void Delaunay3d::retriangulate(int tetraNr, int vertNr)
{
	Tetra &tetra = mTetras[(uint32_t)tetraNr];
	if (tetra.deleted) return;
	Tetra tNew;
	PxVec3 &v = mVertices[(uint32_t)vertNr];
	tetra.deleted = true;
	for (uint32_t i = 0; i < 4; i++) {
		int n = mTetras[(uint32_t)tetraNr].neighborNrs[i];
		if (n >= 0 && mTetras[(uint32_t)n].deleted)
			continue;
		if (n >= 0 && pointInCircumSphere(mTetras[(uint32_t)n],v))
			retriangulate(n, vertNr);
		else {
			Tetra &t = mTetras[(uint32_t)tetraNr];
			tNew.init(vertNr, 
				t.ids[(uint32_t)Tetra::sideIndices[i][0]], 
				t.ids[(uint32_t)Tetra::sideIndices[i][1]], 
				t.ids[(uint32_t)Tetra::sideIndices[i][2]]
			);
			tNew.neighborNrs[0] = n;
			if (n >= 0) {
				mTetras[(uint32_t)n].neighborOf(
					tNew.ids[1], tNew.ids[2], tNew.ids[3]) 
					= (int32_t)mTetras.size();
			}
			mTetras.pushBack(tNew);
		}
	}
}

// ----------------------------------------------------------------------
void Delaunay3d::compressTetrahedra(bool removeExtraVerts)
{
	nvidia::Array<int> oldToNew(mTetras.size(), -1);

	uint32_t num = 0;
	for (uint32_t i = 0; i < mTetras.size(); i++) {
		Tetra &t = mTetras[i];

		if (removeExtraVerts) {
			if (t.ids[0] >= mFirstFarVertex ||
				t.ids[1] >= mFirstFarVertex ||
				t.ids[2] >= mFirstFarVertex ||
				t.ids[3] >= mFirstFarVertex)
				continue;
		}

		if (t.deleted) 
			continue;

		oldToNew[i] = (int32_t)num;
		mTetras[num] = t;
		num++;
	}
	mTetras.resize(num);

	for (uint32_t i = 0; i < num; i++) {
		Tetra &t = mTetras[i];
		for (uint32_t j = 0; j < 4; j++) {
			if (t.neighborNrs[j] >= 0)
				t.neighborNrs[j] = oldToNew[(uint32_t)t.neighborNrs[j]];
		}
	}
}

// ----------------------------------------------------------------------
void Delaunay3d::computeVoronoiMesh()
{
	mGeom.clear();

	if (mTetras.empty())
		return;

	// vertex -> tetras links
	uint32_t numVerts = mVertices.size();

	nvidia::Array<int> firstVertTet(numVerts+1, 0);
	nvidia::Array<int> vertTets;

	nvidia::Array<int> tetMarks(mTetras.size(), -1);
	int tetMark = 0;

	for (uint32_t i = 0; i < mTetras.size(); i++) {
		Tetra &t = mTetras[i];
		firstVertTet[(uint32_t)t.ids[0]]++;
		firstVertTet[(uint32_t)t.ids[1]]++;
		firstVertTet[(uint32_t)t.ids[2]]++;
		firstVertTet[(uint32_t)t.ids[3]]++;
	}

	int numLinks = 0;
	for (uint32_t i = 0; i < numVerts; i++) {
		numLinks += firstVertTet[i];
		firstVertTet[i] = numLinks;
	}
	firstVertTet[numVerts] = numLinks;
	vertTets.resize((uint32_t)numLinks);

	for (uint32_t i = 0; i < mTetras.size(); i++) {
		Tetra &t = mTetras[i];
		uint32_t &i0 = (uint32_t&)firstVertTet[(uint32_t)t.ids[0]];
		uint32_t &i1 = (uint32_t&)firstVertTet[(uint32_t)t.ids[1]];
		uint32_t &i2 = (uint32_t&)firstVertTet[(uint32_t)t.ids[2]];
		uint32_t &i3 = (uint32_t&)firstVertTet[(uint32_t)t.ids[3]];
		i0--; vertTets[i0] = (int32_t)i;
		i1--; vertTets[i1] = (int32_t)i;
		i2--; vertTets[i2] = (int32_t)i;
		i3--; vertTets[i3] = (int32_t)i;
	}

	nvidia::Array<int> convexOfFace(numVerts, -1);
	nvidia::Array<int> vertOfTet(mTetras.size(), -1);
	nvidia::Array<int> convexOfVert(numVerts, -1);

	for (uint32_t i = 0; i < numVerts; i++) {
		int i0 = (int32_t)i;

		int firstTet = firstVertTet[i];
		int lastTet = firstVertTet[i+1];

		// debug
		//bool ok = true;
		//for (int j = firstTet; j < lastTet; j++) {
		//	int tetNr = vertTets[j];
		//	Tetra &t = mTetras[tetNr];
		//	int num = 0;
		//	for (int k = 0; k < 4; k++) {
		//		int n = t.neighborNrs[k];
		//		bool in = false;
		//		for (int l = firstTet; l < lastTet; l++) {
		//			if (vertTets[l] == n)
		//				in = true;
		//		}
		//		if (in)
		//			num++;
		//	}
		//	if (num != 3)
		//		ok = false;
		//}
		//if (ok)
		//	int foo = 0;


		// new convex
		int convexNr = (int32_t)i;
		CompoundGeometry::Convex c;
		mGeom.initConvex(c);
		c.numVerts = lastTet - firstTet;

		bool rollBack = false;

		// create vertices
		for (int j = firstTet; j < lastTet; j++) {
			int tetNr = vertTets[(uint32_t)j];
			vertOfTet[(uint32_t)tetNr] = j - firstTet;
			updateCircumSphere(mTetras[(uint32_t)tetNr]);
			mGeom.vertices.pushBack(mTetras[(uint32_t)tetNr].center);
		}

		// create indices
		for (int j = firstTet; j < lastTet; j++) {
			int tetNr = vertTets[(uint32_t)j];
			Tetra &t = mTetras[(uint32_t)tetNr];

			for (uint32_t k = 0; k < 4; k++) {
				int i1 = t.ids[k];
				if (i1 == i0)
					continue;
				if (convexOfFace[(uint32_t)i1] == convexNr)
					continue;
				convexOfFace[(uint32_t)i1] = convexNr;
				mGeom.neighbors.pushBack(i1);
				c.numNeighbors++;

				// new face
				c.numFaces++;
				int faceStart = (int32_t)mGeom.indices.size();
				mGeom.indices.pushBack(0);
				mGeom.indices.pushBack(0);		// face attrib

				int faceSize = 0;
				int currNr = tetNr;
				int prevNr = -1;
				tetMark++;

				do {
					mGeom.indices.pushBack(vertOfTet[(uint32_t)currNr]);
					faceSize++;
					
					if (tetMarks[(uint32_t)currNr] == tetMark) {	// safety
						rollBack = true;
						break;
					}
					tetMarks[(uint32_t)currNr] = tetMark;

					// find proper neighbor tet
					int nextTet = -1;
					for (uint32_t l = 0; l < 4; l++) {
						int nr = mTetras[(uint32_t)currNr].neighborNrs[l];
						if (nr < 0)
							continue;
						if (nr == prevNr)
							continue;

						Tetra &tn = mTetras[(uint32_t)nr];
						bool hasEdge = 
							(tn.ids[0] == i0 || tn.ids[1] == i0 || tn.ids[2] == i0 || tn.ids[3] == i0) &&
							(tn.ids[0] == i1 || tn.ids[1] == i1 || tn.ids[2] == i1 || tn.ids[3] == i1);
						if (!hasEdge)
							continue;

						//if (prevNr < 0) {	// correct winding
						//	if ((t.center - faceC).cross(tn.center - faceC).dot(faceN) < 0.0f)
						//		continue;
						//}
						nextTet = nr;
						break;
					}

					if (nextTet < 0) {		// not proper convex, roll back
						rollBack = true;
						break;
					}

					prevNr = currNr;
					currNr = nextTet;

				} while (currNr != tetNr);

				if (rollBack)
					break;

				mGeom.indices[(uint32_t)faceStart] = faceSize;
			}
			if (rollBack)
				break;
		}
		if (rollBack) {
			mGeom.indices.resize((uint32_t)c.firstIndex);
			mGeom.vertices.resize((uint32_t)c.firstVert);
		}
		else {
			convexOfVert[(uint32_t)i] = (int32_t)mGeom.convexes.size();
			mGeom.convexes.pushBack(c);
		}
	}

	// fix face orientations
	for (uint32_t i = 0; i < mGeom.convexes.size(); i++) {
		CompoundGeometry::Convex &c = mGeom.convexes[i];
		int *ids = &mGeom.indices[(uint32_t)c.firstIndex];
		PxVec3 *vertices = &mGeom.vertices[(uint32_t)c.firstVert];
		PxVec3 cc(0.0f, 0.0f, 0.0f);
		for (uint32_t j = 0; j < (uint32_t)c.numVerts; j++)
			cc += vertices[j];
		cc /= (float)c.numVerts;
		
		uint32_t *faceIds = (uint32_t*)ids;
		for (int j = 0; j < c.numFaces; j++) {
			uint32_t faceSize = *faceIds++;
			faceIds++; //int faceAttr = *faceIds++;
			PxVec3 fc(0.0f, 0.0f, 0.0f);
			for (uint32_t k = 0; k < faceSize; k++)
				fc += vertices[faceIds[k]];
			fc /= (float)faceSize;
			PxVec3 n = fc - cc;
			if ((vertices[faceIds[1]] - vertices[faceIds[0]]).cross(vertices[faceIds[2]] - vertices[faceIds[0]]).dot(n) < 0.0f) {
				for (uint32_t k = 0; k < faceSize/2; k++) {
					uint32_t d = faceIds[k]; faceIds[k] = faceIds[faceSize-1-k]; faceIds[faceSize-1-k] = d;
				}
			}
			faceIds += faceSize;
		}
	}

	// fix neighbors
	for (uint32_t i = 0; i < mGeom.neighbors.size(); i++) {
		if (mGeom.neighbors[i] >= 0)
			mGeom.neighbors[i] = convexOfVert[(uint32_t)mGeom.neighbors[i]];
	}
}

}
}
}
#endif