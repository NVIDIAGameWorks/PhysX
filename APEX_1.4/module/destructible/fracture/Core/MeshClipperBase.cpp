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
#include "MeshClipperBase.h"
#include "PsSort.h"
#include "ConvexBase.h"
#include <PxAssert.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#else
#define printf(...)
#define fwrite(...)
#define fread(...)
#define fopen(...)
#define fclose(...)
#endif

#include "SimSceneBase.h"

#pragma warning(disable:4127)

namespace nvidia
{
namespace fracture
{
namespace base
{

using namespace nvidia;

int vecFloor(float f ) { return (int)::floorf(f); }

#define DUMP_PATH "c:\\meshClipDump.bin"

// -----------------------------------------------------------------------------
MeshClipper::MeshClipper(SimScene* scene)
{
	mScene = scene;
	mMeshConvex = NULL;
	mClipConvex = NULL;
	mDumpOnError = false;
}

// -----------------------------------------------------------------------------
void MeshClipper::Mesh::clear()
{
	vertices.clear();
	normals.clear();
	tangents.clear();
	texCoords.clear();
	polyStarts.resize(1, 0);
	polyIndices.clear();
	polyNeighbors.clear();
}

// -----------------------------------------------------------------------------
//PxVec3 MeshClipper::conv(const Vec3 &v)
//{
//	return PxVec3((float)v.x, (float)v.y, (float)v.z);
//}
//
//Vec3 MeshClipper::conv(const PxVec3 &v)
//{
//	return Vec3((VecReal)v.x, (VecReal)v.y, (VecReal)v.z);
//}
//
//Plane MeshClipper::conv(const PxPlane &p)
//{
//	return Plane(conv(p.n), VecReal(p.d));
//}
//
//Transform MeshClipper::conv(const PxTransform &t)
//{
//	return Transform(conv(t.p), Quat(t.q.x, t.q.y, t.q.z, t.q.w));
//}

// -----------------------------------------------------------------------------
int MeshClipper::getNumMeshes()
{
	if (mOutMeshes.size() == 1)		// no islands
		return 1;
	else
		return (int32_t)mOutMeshes.size()-1;		
}

// -----------------------------------------------------------------------------
const MeshClipper::Mesh& MeshClipper::getMesh(int meshNr)
{
	if (mOutMeshes.size() == 1)		// no islands
		return mOutMeshes[0];
	else
		return mOutMeshes[1 + (uint32_t)meshNr];
}

// -----------------------------------------------------------------------------
void MeshClipper::init(const Convex *meshConvex)
{
	if (!meshConvex->hasExplicitVisMesh()) {
		mMeshConvex = NULL;
		return;
	}

	mMeshConvex = meshConvex;
	const nvidia::Array<PxVec3> &verts = mMeshConvex->getVisVertices();
	mMeshVertices.resize(verts.size());
	for (uint32_t i = 0; i < verts.size(); i++)
		mMeshVertices[i] = verts[i];

	PxVec3 center(0.0, 0.0, 0.0);
	for (uint32_t i = 0; i < mMeshVertices.size(); i++) 
		center += mMeshVertices[i];
	center /= (float)mMeshVertices.size();

	for (uint32_t i = 0; i < mMeshVertices.size(); i++) {
		mMeshVertices[i] = center + (mMeshVertices[i] - center) * 1.001f;
	}

	const nvidia::Array<int> &meshPolyStarts = mMeshConvex->getVisPolyStarts();
	const nvidia::Array<int> &meshPolyIndices = mMeshConvex->getVisPolyIndices();
	uint32_t numPolys = meshPolyStarts.size()-1;

	mPolyBounds.resize(numPolys);
	for (uint32_t i = 0; i < numPolys; i++) {
		int first = meshPolyStarts[i];
		int last = meshPolyStarts[i+1];
		mPolyBounds[i].setEmpty();
		for (int k = first; k < last; k++) 
			mPolyBounds[i].include(mMeshVertices[(uint32_t)meshPolyIndices[(uint32_t)k]]);
	}

	createGrid();
}

// --------------------------------------------------------------------------------------------
void MeshClipper::createGrid()
{
	const uint32_t minGridPolys = 1000;

	uint32_t numPolys = mPolyBounds.size();
	mGridFirstPoly.clear();

	if (numPolys < minGridPolys)
		return;

	// determine grid dimensions
	PxBounds3 meshBounds;
	meshBounds.setEmpty();
	for (uint32_t i = 0; i < mMeshVertices.size(); i++)
		meshBounds.include(mMeshVertices[i]);
	PxVec3 meshExtent = meshBounds.maximum - meshBounds.minimum;

	// number of cells in the order of num polygons
//	int targetNumCells = numPolys;		
	float targetNumCells = 10000;		
	float s3 = meshExtent.x * meshExtent.y * meshExtent.z / (float)targetNumCells;
	mGridSpacing = PxPow((float)s3, 1.0f/3.0f);

	mGridBounds.minimum = meshBounds.minimum;
	mGridNumX = vecFloor(meshExtent.x / mGridSpacing) + 1;
	mGridNumY = vecFloor(meshExtent.y / mGridSpacing) + 1;
	mGridNumZ = vecFloor(meshExtent.z / mGridSpacing) + 1;
	uint32_t numCells = uint32_t(mGridNumX * mGridNumY * mGridNumZ);
	mGridBounds.maximum = mGridBounds.minimum + 
		PxVec3(mGridNumX * mGridSpacing, mGridNumY * mGridSpacing, mGridNumZ * mGridSpacing);

	mGridFirstPoly.resize(numCells+1, -0);
	mGridPolys.clear();

	// histogram
	//float h = mGridSpacing;
	float h1 = 1.0f / mGridSpacing;
	PxBounds3 polyBounds;
	for (uint32_t i = 0; i < numPolys; i++) {
		PxBounds3 &polyBounds = mPolyBounds[i];
		int x0 = (int)((polyBounds.minimum.x - mGridBounds.minimum.x) * h1);
		int y0 = (int)((polyBounds.minimum.y - mGridBounds.minimum.y) * h1);
		int z0 = (int)((polyBounds.minimum.z - mGridBounds.minimum.z) * h1);
		int x1 = (int)((polyBounds.maximum.x - mGridBounds.minimum.x) * h1);
		int y1 = (int)((polyBounds.maximum.y - mGridBounds.minimum.y) * h1);
		int z1 = (int)((polyBounds.maximum.z - mGridBounds.minimum.z) * h1);
		for (int xi = x0; xi <= x1; xi++) {
			for (int yi = y0; yi <= y1; yi++) {
				for (int zi = z0; zi <= z1; zi++) {
					int nr = (xi * mGridNumY + yi) * mGridNumZ + zi;
					mGridFirstPoly[(uint32_t)nr]++;
				}
			}
		}
	}
	int first = 0;
	for (uint32_t i = 0; i < numCells; i++) {
		first += mGridFirstPoly[i];
		mGridFirstPoly[i] = first;
	}
	mGridFirstPoly[numCells] = first;
	mGridPolys.resize((uint32_t)first);

	// fill in
	for (uint32_t i = 0; i < numPolys; i++) {
		PxBounds3 &polyBounds = mPolyBounds[i];
		int x0 = (int)((polyBounds.minimum.x - mGridBounds.minimum.x) * h1);
		int y0 = (int)((polyBounds.minimum.y - mGridBounds.minimum.y) * h1);
		int z0 = (int)((polyBounds.minimum.z - mGridBounds.minimum.z) * h1);
		int x1 = (int)((polyBounds.maximum.x - mGridBounds.minimum.x) * h1);
		int y1 = (int)((polyBounds.maximum.y - mGridBounds.minimum.y) * h1);
		int z1 = (int)((polyBounds.maximum.z - mGridBounds.minimum.z) * h1);
		for (int xi = x0; xi <= x1; xi++) {
			for (int yi = y0; yi <= y1; yi++) {
				for (int zi = z0; zi <= z1; zi++) {
					int nr = (xi * mGridNumY + yi) * mGridNumZ + zi;
					mGridFirstPoly[(uint32_t)nr]--;
					mGridPolys[(uint32_t)mGridFirstPoly[(uint32_t)nr]] = (int32_t)i;
				}
			}
		}
	}
	mGridPolyMarks.clear();
	mGridPolyMarks.resize(numPolys, 0);
	mGridCurrentMark = 1;
}

// --------------------------------------------------------------------------------------------
void MeshClipper::computeClipFaceNeighbors()
{
	const nvidia::Array<int> &indices = mClipConvex->getIndices();
	const nvidia::Array<Convex::Face> &faces = mClipConvex->getFaces();

	mClipFaceNeighbors.clear();
	mClipFaceNeighbors.resize(indices.size(), -1);

	mEdges.resize(indices.size());
	//int edgeNr = 0;
	for (uint32_t i = 0; i < faces.size(); i++) {
		const Convex::Face &f = faces[i];
		for (int j = 0; j < f.numIndices; j++) {
			int edgeNr = f.firstIndex + j;
			mEdges[(uint32_t)edgeNr].init(indices[uint32_t(f.firstIndex + j)], indices[uint32_t(f.firstIndex + (j+1)%f.numIndices)], edgeNr, (int32_t)i);
		}
	}
	shdfnd::sort(mEdges.begin(), mEdges.size());
	uint32_t i = 0;
	while (i < mEdges.size()) {
		Edge &e0 = mEdges[i];
		i++;
		if (i < mEdges.size() && mEdges[i] == e0) {
			Edge &e1 = mEdges[i];
			mClipFaceNeighbors[(uint32_t)e0.edgeNr] = e1.faceNr;
			mClipFaceNeighbors[(uint32_t)e1.edgeNr] = e0.faceNr;
			i++;
		}
		while (i < mEdges.size() && mEdges[i] == e0)
			i++;
	}
}

// --------------------------------------------------------------------------------------------
bool MeshClipper::getGridCandidates(const PxBounds3 &bounds)
{
	if (mGridFirstPoly.empty())
		return false;			// too few mesh polygons, no grid created

	float h1 = 1.0f / mGridSpacing;
	mGridCurrentMark++;		// make sure we return any poligon only once

	int x0 = vecFloor((bounds.minimum.x - mGridBounds.minimum.x) * h1);
	int y0 = vecFloor((bounds.minimum.y - mGridBounds.minimum.y) * h1);
	int z0 = vecFloor((bounds.minimum.z - mGridBounds.minimum.z) * h1);
	int x1 = vecFloor((bounds.maximum.x - mGridBounds.minimum.x) * h1);
	int y1 = vecFloor((bounds.maximum.y - mGridBounds.minimum.y) * h1);
	int z1 = vecFloor((bounds.maximum.z - mGridBounds.minimum.z) * h1);

	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (z0 < 0) z0 = 0;
	if (x1 >= mGridNumX) x1 = mGridNumX-1;
	if (y1 >= mGridNumY) y1 = mGridNumY-1;
	if (z1 >= mGridNumZ) z1 = mGridNumZ-1;


	mPolyCandidates.clear();

	for (int xi = x0; xi <= x1; xi++) {
		for (int yi = y0; yi <= y1; yi++) {
			for (int zi = z0; zi <= z1; zi++) {
				int nr = (xi * mGridNumY + yi) * mGridNumZ + zi;
				int first = mGridFirstPoly[(uint32_t)nr];
				int last = mGridFirstPoly[(uint32_t)nr+1];
				for (int i = first; i < last; i++) {
					int polyNr = mGridPolys[(uint32_t)i];
					if (mGridPolyMarks[(uint32_t)polyNr] != mGridCurrentMark) {
						mGridPolyMarks[(uint32_t)polyNr] = mGridCurrentMark;
						mPolyCandidates.pushBack(polyNr);
					}
				}
			}
		}
	}
	return true;
}

// --------------------------------------------------------------------------------------------
void MeshClipper::generateCandidates(const PxTransform &pxTrans)
{
	// cull polys that are completely outside of at least one convex plane
	// determine polys that are completely inside of all convex planes

	const nvidia::Array<int> &meshPolyStarts = mMeshConvex->getVisPolyStarts();
	const nvidia::Array<int> &meshPolyIndices = mMeshConvex->getVisPolyIndices();

	const nvidia::Array<PxPlane> &clipPlanes = mClipConvex->getPlanes();
	const nvidia::Array<PxVec3> &clipConvexVertices = mClipConvex->getVertices();

	PxTransform trans = (pxTrans);
	PxTransform invTrans = trans.getInverse();

	PxBounds3 clipBounds;
	clipBounds.setEmpty();
	for (uint32_t i = 0; i < clipConvexVertices.size(); i++)
		clipBounds.include(invTrans.transform((clipConvexVertices[i])));

	uint32_t numPolys = meshPolyStarts.size()-1;

	// use grid for initial culling
	if (!getGridCandidates(clipBounds)) {
		mPolyCandidates.resize(numPolys);		// no grid, start with all polys
		for (uint32_t i = 0; i < numPolys; i++)
			mPolyCandidates[i] = (int32_t)i;			
	}

	// use bounding boxes for further culling
	PxBounds3 polyBounds;
	uint32_t numCandidates = 0;
	for (uint32_t i = 0; i < mPolyCandidates.size(); i++) {
		int polyNr = mPolyCandidates[i];
		PxBounds3 &polyBounds = mPolyBounds[(uint32_t)polyNr];
		if (polyBounds.intersects(clipBounds)) {
			mPolyCandidates[numCandidates] = polyNr;
			numCandidates++;
		}
	}
	mPolyCandidates.resize(numCandidates);

	// use clip planes for further culling
	mPolyInside.resize(0);
	mPolyInside.resize(numPolys, true);

	for (uint32_t i = 0; i < clipPlanes.size(); i++) {
		PxPlane plane = (clipPlanes[i]);
		plane.n = invTrans.rotate(plane.n);
		plane.d = plane.d + invTrans.p.dot(plane.n);

		uint32_t numCandidates = 0;
		for (uint32_t j = 0; j < mPolyCandidates.size(); j++) {
			uint32_t polyNr = (uint32_t)mPolyCandidates[j];
			int first = meshPolyStarts[polyNr];
			int last = meshPolyStarts[polyNr+1];
			bool outside = true;
			for (int k = first; k < last; k++) {
				PxVec3 &p = mMeshVertices[(uint32_t)meshPolyIndices[(uint32_t)k]];
				float dot = plane.n.dot(p);
				if (dot > plane.d)
					mPolyInside[polyNr] = false;
				if (dot < plane.d)
					outside = false;
				if (!mPolyInside[polyNr] && !outside)
					break;
			}
			if (!outside) {
				mPolyCandidates[numCandidates] = (int32_t)polyNr;
				numCandidates++;
			}
		}
		mPolyCandidates.resize(numCandidates);
	}
}

// --------------------------------------------------------------------------------------------
bool MeshClipper::createInternalPolygons(const PxTransform &pxTrans)
{
#if VEC3_DOUBLE
	const float eps = 1e-8f;
#else
	const float eps = 1e-4f;
#endif

	PxTransform trans = (pxTrans);

	const nvidia::Array<PxVec3> &meshNormals = mMeshConvex->getVisNormals();
	const nvidia::Array<PxVec3> &meshTangents = mMeshConvex->getVisTangents();
	const nvidia::Array<float> &meshTexCoords = mMeshConvex->getVisTexCoords();

	const nvidia::Array<int> &meshPolyStarts = mMeshConvex->getVisPolyStarts();
	const nvidia::Array<int> &meshPolyIndices = mMeshConvex->getVisPolyIndices();
	const nvidia::Array<int> &meshPolyNeighbors = mMeshConvex->getVisPolyNeighbors();

	const nvidia::Array<PxPlane> &clipPlanes = mClipConvex->getPlanes();
	//const nvidia::Array<Convex::Face> &clipConvexFaces = mClipConvex->getFaces();

	uint32_t current = 0;
	Mesh &out = mOutMeshes[0];

	for (uint32_t i = 0; i < mPolyCandidates.size(); i++) {
		uint32_t polyNr = (uint32_t)mPolyCandidates[i];

		int first = meshPolyStarts[polyNr];
		int last = meshPolyStarts[polyNr+1];
		int polySize = last - first;
		PX_ASSERT(polySize >= 3);

		// initialize working poly with original poly
		// todo: fast poly for insides

		mClipPolyStarts[current].resize(2, 0);
		mClipPolyStarts[current][1] = polySize;

		mClipPolyVerts[current].resize((uint32_t)polySize);

		if (mPolyInside[polyNr]) {		// fast path for polygons completely inside convex

			mFirstOutPoly[polyNr] = (int32_t)out.polyStarts.size()-1;
			mNumOutPolys[polyNr] = 1;	

			for (int j = 0; j < polySize; j++) {
				uint32_t id = (uint32_t)meshPolyIndices[uint32_t(first + j)];
				if (mOutVertNr[id] < 0) {
					mOutVertNr[id] = (int32_t)out.vertices.size();
					out.vertices.pushBack((trans.transform(mMeshVertices[id])));
					out.normals.pushBack(pxTrans.rotate(meshNormals[id]));
					out.tangents.pushBack(pxTrans.rotate(meshTangents[id]));
					out.texCoords.pushBack(meshTexCoords[2*id]);
					out.texCoords.pushBack(meshTexCoords[2*id+1]);
				}
				out.polyIndices.pushBack(mOutVertNr[id]);
				out.polyNeighbors.pushBack(meshPolyNeighbors[uint32_t(first + j)]);
			}
			out.polyStarts.pushBack((int32_t)out.polyIndices.size());
			continue;
		}

		for (int j = 0; j < polySize; j++) {
			PolyVert &pv = mClipPolyVerts[current][(uint32_t)j];
			pv.init();
			pv.id = meshPolyIndices[uint32_t(first + j)];
			pv.pos = trans.transform(mMeshVertices[(uint32_t)pv.id]);
			pv.normal = trans.rotate((meshNormals[(uint32_t)pv.id]));
			pv.tangent = trans.rotate((meshTangents[(uint32_t)pv.id]));
			pv.u = meshTexCoords[2 * (uint32_t)pv.id];
			pv.v = meshTexCoords[2 * (uint32_t)pv.id + 1];
			pv.edgeNeighbor = meshPolyNeighbors[uint32_t(first + j)];
			pv.edgeOnFace = false;
		}
			
		// determine face normal
		PxVec3 faceNormal(0.0, 0.0, 0.0);
		PxVec3 &p0 = mClipPolyVerts[current][0].pos;
		for (int j = 1; j < polySize-1; j++) {
			PxVec3 &p1 = mClipPolyVerts[current][(uint32_t)j].pos;
			PxVec3 &p2 = mClipPolyVerts[current][(uint32_t)j+1].pos;
			faceNormal += (p1-p0).cross(p2-p0);
		}
		faceNormal.normalize();

		// clip working poly against all planes
		for (uint32_t j = 0; j < clipPlanes.size(); j++) {

			const PxPlane plane = (clipPlanes[j]);
			PxVec3 cutNormal = faceNormal.cross(plane.n);
			if (cutNormal.magnitudeSquared() < eps * eps)
				continue;

			uint32_t prev = current;
			current = 1-current;

			mClipPolyStarts[current].clear();
			mClipPolyStarts[current].pushBack(0);
			mClipPolyVerts[current].clear();

			uint32_t numPrevPolys = mClipPolyStarts[prev].size()-1;
			if (numPrevPolys == 0)
				break;

			for (uint32_t k = 0; k < numPrevPolys; k++) {
				int prevFirst = mClipPolyStarts[prev][k];
				int prevLast  = mClipPolyStarts[prev][k+1];
				int prevNum = prevLast - prevFirst;

				// determine cuts
				mCuts.clear();
				bool anyIn = false;

				for (int l = 0; l < prevNum; l++) {
					PolyVert &pv0 = mClipPolyVerts[prev][uint32_t(prevFirst + l)];
					PolyVert &pv1 = mClipPolyVerts[prev][uint32_t(prevFirst + (l+1)%prevNum)];
					pv0.nextVert = -1;

					bool sign0 = plane.n.dot(pv0.pos) > plane.d;
					bool sign1 = plane.n.dot(pv1.pos) > plane.d; 

					if (!sign0 || !sign1)
						anyIn = true;

					if (sign0 == sign1)
						continue;

					PxVec3 p0 = pv0.pos;
					PxVec3 p1 = pv1.pos;

					// compute in unique order such that cut positions are identical on shared edges
					bool smaller = 
						(p0.x < p1.x) || 
						(p0.x == p1.x && p0.y < p1.y) || 
						(p0.x == p1.x && p0.y == p1.y && p0.z < p1.z);
					if (smaller) {
						p0 = pv1.pos;
						p1 = pv0.pos;
					}

					float t = (p1 - p0).dot(plane.n);
					if (t == 0.0)
						continue;
					t = (plane.d - p0.dot(plane.n)) / t;

					PolyVert cut;
					cut.init();
					cut.pos = p0 + (p1 - p0) * t;
					if (smaller) {
						cut.normal = pv1.normal + (pv0.normal - pv1.normal) * t;
						cut.tangent = pv1.tangent + (pv0.tangent - pv1.tangent) * t;
						cut.u = pv1.u + (pv0.u - pv1.u) * t;
						cut.v = pv1.v + (pv0.v - pv1.v) * t;
					}
					else {
						cut.normal = pv0.normal + (pv1.normal - pv0.normal) * t;
						cut.tangent = pv0.tangent + (pv1.tangent - pv0.tangent) * t;
						cut.u = pv0.u + (pv1.u - pv0.u) * t;
						cut.v = pv0.v + (pv1.v - pv0.v) * t;
					}
					cut.normal.normalize();
					cut.tangent.normalize();
					cut.id = -1; 

					// vertex following this cut
					if (sign1) {
						cut.edgeOnFace = true;
						cut.edgeNeighbor = (int32_t)j;		// planeNr
						cut.nextVert = -1;	// exit convex
					}
					else {
						cut.edgeOnFace = pv0.edgeOnFace;
						cut.edgeNeighbor = pv0.edgeNeighbor;
						cut.nextVert = (l+1)%prevNum;	// enter convex
					}

					pv0.nextVert = (int32_t)mCuts.size();
					mCuts.pushBack(cut);
				}
				if (!anyIn)
					continue;

				// poly not cut, copy 1:1
				if (mCuts.empty()) {		
					for (int l = 0; l < prevNum; l++) {
						PolyVert &pv = mClipPolyVerts[prev][uint32_t(prevFirst + l)];
						mClipPolyVerts[current].pushBack(pv);
					}
					mClipPolyStarts[current].pushBack((int32_t)mClipPolyVerts[current].size());
					continue;
				}

				// construct new polygon(s)
				for (uint32_t l = 0; l < mCuts.size(); l++) 
				{
					if (mCuts[l].nextVert < 0) // exit polygon
						continue;		

					// start a polygon from this cut
					int nr = mCuts[l].nextVert;
					mCuts[l].nextVert = -1;		// visited
					mClipPolyVerts[current].pushBack(mCuts[l]);

					int safetyCnt = 0;

					while ( true )
					{
						if (!check(safetyCnt < prevNum + (int)mCuts.size(), "construct new polygons infinite loop")) 
							return false;
						safetyCnt++;

						if (!check(nr >= 0, "construct new polygons loop not closed")) 
							return false;

						if(nr >= (int)mClipPolyVerts[prev].size())		// prevents a crash
							return false;

						mClipPolyVerts[current].pushBack(mClipPolyVerts[prev][(uint32_t)nr]);
						int cutNr = mClipPolyVerts[prev][(uint32_t)nr].nextVert;
						if (cutNr < 0) {
							nr = (nr+1) % prevNum;
							continue;
						}
						// we reached the next cut
						mClipPolyVerts[current].pushBack(mCuts[(uint32_t)cutNr]);

						// find the closest cut to the left
						float s0 = mCuts[(uint32_t)cutNr].pos.dot(cutNormal);
						float sMax = -FLT_MAX;
						float sMin = FLT_MAX;
						int minNr = -1;
						int maxNr = -1;
						for (int m = 0; m < (int)mCuts.size(); m++) {
							float s = mCuts[(uint32_t)m].pos.dot(cutNormal);
							if (s > s0 && s < sMin) {
								sMin = s;
								minNr = m;
							}
							if (s > sMax) {
								sMax = s;
								maxNr = m;
							}
						}
						uint32_t nextCutNr = uint32_t((minNr >= 0) ? minNr : maxNr);

						if (nextCutNr == l)
							break;		// loop closed

						PolyVert &nextCut = mCuts[nextCutNr];
						nr = nextCut.nextVert;
						nextCut.nextVert = -1;		// visited
						mClipPolyVerts[current].pushBack(nextCut);
					}
					// close poly
					mClipPolyStarts[current].pushBack((int32_t)mClipPolyVerts[current].size());
				}
			}
		}	// next plane

		// map from old to new poly numbers
		mFirstOutPoly[polyNr] = (int32_t)out.polyStarts.size()-1;
		mNumOutPolys[polyNr] = (int32_t)mClipPolyStarts[current].size()-1;	
		// separate num needed because we do not process the polys with candidate list, not in order

		// clipped poly empty?
		if (mNumOutPolys[polyNr] <= 0)
			continue;

		// create visual polygon(s)
		for (uint32_t j = 0; j < mClipPolyStarts[current].size()-1; j++) {
			int first = mClipPolyStarts[current][j];
			int last  = mClipPolyStarts[current][j+1];
			int num = last - first;
			for (int k = 0; k < num; k++) {
				PolyVert &pv0 = mClipPolyVerts[current][uint32_t(first + k)];
				PolyVert &pv1 = mClipPolyVerts[current][uint32_t(first + (k+1)%num)];

				int id = -1;
				int faceId = -1;

				if (!pv0.edgeOnFace && !pv1.edgeOnFace) {		// existing vertex, duplicate
					if (!check(pv1.id >= 0, "internal vertex with id < 0")) 
						return false;

					if (mOutVertNr[(uint32_t)pv1.id] < 0) {
						mOutVertNr[(uint32_t)pv1.id] = (int32_t)out.vertices.size();
						out.vertices.pushBack((pv1.pos));
						out.normals.pushBack((pv1.normal));
						out.tangents.pushBack((pv1.tangent));
						out.texCoords.pushBack((float)pv1.u);
						out.texCoords.pushBack((float)pv1.v);
					}
					id = mOutVertNr[(uint32_t)pv1.id];
					faceId = -1;
				}
				else if (pv0.edgeOnFace && pv1.edgeOnFace) {	// new, non shared
					id = (int32_t)out.vertices.size();
					out.vertices.pushBack((pv1.pos));
					out.normals.pushBack((pv1.normal));
					out.tangents.pushBack((pv1.tangent));
					out.texCoords.pushBack((float)pv1.u);
					out.texCoords.pushBack((float)pv1.v);
					faceId = id;
				}
				else {		// new, shared
					int edgeNeighbor = pv0.edgeOnFace ? pv1.edgeNeighbor : pv0.edgeNeighbor;
					int sharedId = -1;
					int numPolys = mNumOutPolys[(uint32_t)edgeNeighbor];

					// new neighbor exists, search shared vertex
					if (numPolys > 0) {		
						int firstPoly = mFirstOutPoly[(uint32_t)edgeNeighbor];
						for (int l = 0; l < numPolys; l++) {
							int newPolyNr = firstPoly + l;
							int first = out.polyStarts[(uint32_t)newPolyNr];
							int last  = out.polyStarts[(uint32_t)newPolyNr+1];
							for (int m = first; m < last; m++) {
								int testId = out.polyIndices[(uint32_t)m];
								if (out.vertices[(uint32_t)testId] == (pv1.pos)) {
									sharedId = testId;
									break;
								}
							}
							if (sharedId >= 0)
								break;
						}
					}
					if (sharedId >= 0 && out.normals[(uint32_t)sharedId] == (pv1.normal)) 		// share it
						id = sharedId;
					else {
						id = (int32_t)out.vertices.size();		// create new
						out.vertices.pushBack((pv1.pos));
						out.normals.pushBack((pv1.normal));
						out.tangents.pushBack((pv1.tangent));
						out.texCoords.pushBack((float)pv1.u);
						out.texCoords.pushBack((float)pv1.v);
					}
					if (sharedId >= 0)
						faceId = sharedId;		// on the convex surface we need the shared id to make the surface poly connected
					else
						faceId = id;
				}
				out.polyIndices.pushBack(id);
				out.polyNeighbors.pushBack(pv1.edgeOnFace ? -1 : pv1.edgeNeighbor);
				pv1.id = faceId;
			}
			out.polyStarts.pushBack((int32_t)out.polyIndices.size());	// close poly

			// create face vertices
			for (int k = 0; k < num; k++) {
				PolyVert &pv0 = mClipPolyVerts[current][uint32_t(first + k)];
				PolyVert &pv1 = mClipPolyVerts[current][uint32_t(first + (k+1)%num)];
				if (!pv0.edgeOnFace && !pv1.edgeOnFace) 
					continue;

				if (pv1.id >= (int)mFaceVertNrs.size())
					mFaceVertNrs.resize((uint32_t)pv1.id + 1, -1);
				
				int &vertNr = mFaceVertNrs[(uint32_t)pv1.id];
				if (vertNr < 0) {
					FaceVert newSv;
					newSv.init();
					vertNr = (int32_t)mFaceVerts.size();
					mFaceVerts.pushBack(newSv);
				}
				FaceVert &sv = mFaceVerts[(uint32_t)vertNr];
				sv.id = pv1.id;
			}

			// create face polyline
			for (int k = 0; k < num; k++) {
				PolyVert &pv0 = mClipPolyVerts[current][uint32_t(first + k)];
				PolyVert &pv1 = mClipPolyVerts[current][uint32_t(first + (k+1)%num)];

				if (pv1.id < 0)		// not on surface
					continue;

				FaceVert &sv = mFaceVerts[(uint32_t)mFaceVertNrs[(uint32_t)pv1.id]];
				if (pv0.edgeOnFace) {
					sv.nextFaceNr = pv0.edgeNeighbor;
					sv.neighborPos = out.polyStarts[out.polyStarts.size()-2] + (k+num-1)%num;
					sv.neighborPolyNr = (int32_t)out.polyStarts.size()-2;
					sv.nextVert = mFaceVertNrs[(uint32_t)pv0.id];
				}
				if (pv1.edgeOnFace) {
					sv.prevFaceNr = pv1.edgeNeighbor;
				}
			}
		}
	}	// next polygon candidate

	// fix neighbors
	for (int i = 0; i < (int)out.polyStarts.size()-1; i++) {
		int first = out.polyStarts[(uint32_t)i];
		int num = out.polyStarts[(uint32_t)i+1] - first;
		for (int j = 0; j < num; j++) {
			int oldNr = out.polyNeighbors[uint32_t(first + j)];
			if (oldNr < 0)
				continue;

			int numNew = mNumOutPolys[(uint32_t)oldNr];
			if (!check(numNew > 0, "fix internal neighbors, no new polygond created")) 
				return false;

			if (numNew == 1) {		// simple mapping
				out.polyNeighbors[uint32_t(first + j)] = mFirstOutPoly[(uint32_t)oldNr];
			}
			else {		// find correct new poly
				PxVec3 v1 = out.vertices[(uint32_t)out.polyIndices[uint32_t(first + (j+1)%num)]];
				float minD2 = PX_MAX_F32;
				int minNr = -1;

				for (int k = 0; k < numNew; k++) {
					int newNr = mFirstOutPoly[(uint32_t)oldNr] + k;
					int firstNew = out.polyStarts[(uint32_t)newNr];
					int lastNew = out.polyStarts[(uint32_t)newNr+1];
					for (int l = firstNew; l < lastNew; l++) {
						PxVec3 v0 = out.vertices[(uint32_t)out.polyIndices[(uint32_t)l]];
						float d2 = (v1-v0).magnitudeSquared();
						if (d2 < minD2) {
							minD2 = d2;
							minNr = newNr;
						}
					}
				}
				out.polyNeighbors[uint32_t(first + j)] = minNr;
			}
		}
	}

	return true;
}
	
// --------------------------------------------------------------------------------------------
bool MeshClipper::createFacePolys()
{
	const nvidia::Array<PxPlane> &clipPlanes = mClipConvex->getPlanes();
	const nvidia::Array<Convex::Face> &clipConvexFaces = mClipConvex->getFaces();
	const nvidia::Array<PxVec3> &clipConvexVertices = mClipConvex->getVertices();
	const nvidia::Array<int> &clipConvexIndices = mClipConvex->getIndices();

	mFaceTypes.clear();
	mFaceTypes.resize(clipConvexFaces.size(), 0);
	mFaceOfPoly.clear();

	Mesh &out = mOutMeshes[0];
	int firstFacePoly = (int32_t)out.polyStarts.size()-1;


	// create face polys
	for (uint32_t i = 0; i < mFaceVerts.size(); i++) {
		FaceVert &startSv = mFaceVerts[i];
		if (startSv.visited)
			continue;	

		int faceNr = startSv.nextFaceNr;
		if (!check(faceNr >= 0, "create face polygons, loop interrupted")) 
			return false;

		mFaceTypes[(uint32_t)faceNr] = 1;	// has visual mesh

		PxVec3 faceNormal = clipPlanes[(uint32_t)faceNr].n;
		PxVec3 t0,t1;
		// tangents
		if (fabs(faceNormal.x) < fabs(faceNormal.y) && fabs(faceNormal.x) < fabs(faceNormal.z))
			t0 = PxVec3(1.0f, 0.0f, 0.0f);
		else if (fabs(faceNormal.y) < fabs(faceNormal.z))
			t0 = PxVec3(0.0f, 1.0f, 0.0);
		else
			t0 = PxVec3(0.0f, 0.0f, 1.0f);
		t1 = faceNormal.cross(t0);
		t1.normalize();
		t0 = t1.cross(faceNormal);

		const Convex::Face &face = clipConvexFaces[(uint32_t)faceNr];
		int thisPolyNr = (int32_t)out.polyStarts.size()-1;

		int vertNr = (int32_t)i; 
		int safetyCnt = 0;

		while (true) {
			if (!check(safetyCnt <= (int)mFaceVerts.size() + (int)clipConvexVertices.size(), "create face polygons infinite loop")) 
				return false;
			safetyCnt++;

			if (!check(vertNr >= 0, "create face polygons loop interrupted"))
				return false;

			FaceVert &sv = mFaceVerts[(uint32_t)vertNr];

			out.polyIndices.pushBack((int32_t)out.vertices.size());
			PxVec3 temp = out.vertices[(uint32_t)sv.id];
			out.vertices.pushBack(temp);
			out.normals.pushBack(faceNormal);
			out.tangents.pushBack(t0);
			out.texCoords.pushBack(out.vertices[(uint32_t)sv.id].dot(t0) * mTexScale);
			out.texCoords.pushBack(out.vertices[(uint32_t)sv.id].dot(t1) * mTexScale);

			if (sv.nextFaceNr == faceNr) {		// continues on this face
				sv.visited = true;
				out.polyNeighbors.pushBack(sv.neighborPolyNr);
				out.polyNeighbors[(uint32_t)sv.neighborPos] = thisPolyNr;
				vertNr = sv.nextVert;
				if (vertNr == (int32_t)i)
					break;			// start reached
				else
					continue;
			}

			// on convex edge, find next entry vert
			int edgeNr = 0;
			while (edgeNr < face.numIndices && mClipFaceNeighbors[uint32_t(face.firstIndex + edgeNr)] != sv.nextFaceNr)
				edgeNr++;
			if (!check(edgeNr < face.numIndices, "create face polys, find next entry vert not successfull"))
				return false;

			vertNr = -1;
			int numCorners = 0;

			for (int j = 0; j <= face.numIndices; j++) {
				int adjFaceNr = mClipFaceNeighbors[uint32_t(face.firstIndex + edgeNr)];
				out.polyNeighbors.pushBack(-1-adjFaceNr);
		
				PxVec3 edgeDir = clipPlanes[(uint32_t)faceNr].n.cross(clipPlanes[(uint32_t)adjFaceNr].n);
				float s0 = out.vertices[out.vertices.size()-1].dot(edgeDir);
				float minS = PX_MAX_F32;
				int minK = -1;
				for (uint32_t k = 0; k < mFaceVerts.size(); k++) {		// todo: create edge face vert list to speed up
					FaceVert &vk = mFaceVerts[k];
//					if (vk.id < 0) continue;
					if (vk.nextFaceNr != faceNr || vk.prevFaceNr != adjFaceNr)
						continue;
					float si = out.vertices[(uint32_t)vk.id].dot(edgeDir);
					if (si > s0 && si < minS) {
						minS = si;
						minK = (int32_t)k;
					}
				}
				
				if (minK >= 0) {
					vertNr = minK;
					break;
				}

				// no entry found, add corner of convex
				PxVec3 pos = clipConvexVertices[(uint32_t)clipConvexIndices[uint32_t(face.firstIndex + (edgeNr+1)%face.numIndices)]];
				out.vertices.pushBack(pos);
				out.normals.pushBack(faceNormal);
				out.tangents.pushBack(t0);
				out.texCoords.pushBack(pos.dot(t0) * mTexScale);
				out.texCoords.pushBack(pos.dot(t1) * mTexScale);

				out.polyIndices.pushBack((int32_t)out.vertices.size()-1);
				if (numCorners > 0) {
					if (mFaceTypes[(uint32_t)mClipFaceNeighbors[uint32_t(face.firstIndex + edgeNr)]] == 0)
						mFaceTypes[(uint32_t)mClipFaceNeighbors[uint32_t(face.firstIndex + edgeNr)]] = 2;	// inside visual mesh
				}

				numCorners++;
				edgeNr = (edgeNr+1) % face.numIndices;
			}
			if (vertNr == (int32_t)i)
				break;			// loop closed
		}

		mFaceOfPoly.pushBack(faceNr);
		out.polyStarts.pushBack((int32_t)out.polyIndices.size());	// close face poly
	}

	// determine faces completely inside the visual mesh via flood fill
	// start at faces of type 2 and fill all faces of type 0
	// reagion is bounded by faces of type 1 containing the visual mesh
	for (uint32_t i = 0; i < mFaceTypes.size(); i++) {
		if (mFaceTypes[i] != 2)
			continue;
		mFaceTypes[i] = 0;
		mFaceQueue.resize(1, (int32_t)i);
		while (mFaceQueue.size() > 0) {
			uint32_t faceNr = (uint32_t)mFaceQueue[mFaceQueue.size()-1];
			mFaceQueue.popBack();
			if (mFaceTypes[faceNr] != 0)
				continue;
			mFaceTypes[faceNr] = 2;
			const Convex::Face &face = clipConvexFaces[faceNr];
			for (int j = 0; j < face.numIndices; j++) {
				int adjFaceNr = mClipFaceNeighbors[uint32_t(face.firstIndex + j)];
				if (mFaceTypes[(uint32_t)adjFaceNr] == 0)
					mFaceQueue.pushBack(adjFaceNr);
			}
		}
	}

	// create a polygon for each complete face inside the mesh
	for (uint32_t i = 0; i < clipConvexFaces.size(); i++) {
		if (mFaceTypes[i] != 2)
			continue;
		const Convex::Face &face = clipConvexFaces[i];
		PxVec3 faceNormal = clipPlanes[i].n;

		// tangents
		PxVec3 t0,t1;
		if (fabs(faceNormal.x) < fabs(faceNormal.y) && fabs(faceNormal.x) < fabs(faceNormal.z))
			t0 = PxVec3(1.0f, 0.0f, 0.0f);
		else if (fabs(faceNormal.y) < fabs(faceNormal.z))
			t0 = PxVec3(0.0f, 1.0f, 0.0);
		else
			t0 = PxVec3(0.0f, 0.0f, 1.0f);
		t1 = faceNormal.cross(t0);
		t1.normalize();
		t0 = t1.cross(faceNormal);

		for (int j = 0; j < (int)face.numIndices; j++) {
			PxVec3 pos = clipConvexVertices[(uint32_t)clipConvexIndices[uint32_t(face.firstIndex + j)]];
			out.vertices.pushBack(pos);
			out.normals.pushBack(faceNormal);
			out.tangents.pushBack(t0);
			out.texCoords.pushBack(pos.dot(t0) * mTexScale);
			out.texCoords.pushBack(pos.dot(t1) * mTexScale);

			out.polyIndices.pushBack((int32_t)out.vertices.size()-1);
			out.polyNeighbors.pushBack(-1-mClipFaceNeighbors[uint32_t(face.firstIndex+j)]);
		}
		mFaceOfPoly.pushBack((int32_t)i);
		out.polyStarts.pushBack((int32_t)out.polyIndices.size());
	}

	// fix face polygon neighbors
	for (uint32_t i = (uint32_t)firstFacePoly; i < out.polyStarts.size()-1; i++) {
		int first = out.polyStarts[i];
		int num = out.polyStarts[i+1] - first;
		for (int j = 0; j < num; j++) {
			int adjFaceNr = out.polyNeighbors[uint32_t(first + j)];
			if (adjFaceNr >= 0)
				continue;
			adjFaceNr = -1 - adjFaceNr;
			PxVec3 &p0 = out.vertices[(uint32_t)out.polyIndices[uint32_t(first + (j+1)%num)]];
			float minD2 = PX_MAX_F32;
			int minAdjPolyNr = -1;

			for (uint32_t k = 0; k < mFaceOfPoly.size(); k++) {
				if (mFaceOfPoly[k] != adjFaceNr)
					continue;
				int adjPolyNr = firstFacePoly + (int32_t)k;
				int adjFirst = out.polyStarts[(uint32_t)adjPolyNr];
				int adjLast = out.polyStarts[(uint32_t)adjPolyNr+1];
				for (int l = adjFirst; l < adjLast; l++) {
					PxVec3 &p1 = out.vertices[(uint32_t)out.polyIndices[(uint32_t)l]];
					float d2 = (p0 - p1).magnitudeSquared();
					if (d2 < minD2) {
						minD2 = d2;
						minAdjPolyNr = adjPolyNr;
					}
				}
			}
			if (!check(minAdjPolyNr >= 0, "create face polys, fix neighbors"))
				return false;

			out.polyNeighbors[uint32_t(first + j)] = minAdjPolyNr;
		}
	}
	return true;
}

// --------------------------------------------------------------------------------------------
bool MeshClipper::clip(const Convex *clipConvex, const PxTransform &trans, float texScale, bool splitIslands,
					   bool &empty, bool &completelyInside)
{
	if (mMeshConvex == NULL)
		return false;

	mTexScale = texScale;

	mClipConvex = clipConvex;
	computeClipFaceNeighbors();

	empty = true;
	completelyInside = false;

	mOutMeshes.resize(1);
	Mesh &out = mOutMeshes[0];
	mOutMeshes[0].clear();

	mFaceVertNrs.clear();	// necessary? 
	mFaceVerts.clear();

	const nvidia::Array<int> &meshPolyStarts = mMeshConvex->getVisPolyStarts();
	//const nvidia::Array<PxPlane> &clipPlanes = mClipConvex->getPlanes();
	const nvidia::Array<Convex::Face> &clipConvexFaces = mClipConvex->getFaces();
	const nvidia::Array<PxVec3> &clipConvexVertices = mClipConvex->getVertices();

	int numPolys = (int32_t)meshPolyStarts.size()-1;

	mScene->profileBegin("generate poly candidates");
	generateCandidates(trans);
	mScene->profileEnd("generate poly candidates");

	mFirstOutPoly.clear();					// speed up, use only local mesh part
	mFirstOutPoly.resize((uint32_t)numPolys, -1);
	mNumOutPolys.clear();
	mNumOutPolys.resize((uint32_t)numPolys, 0);

	mOutVertNr.clear();
	mOutVertNr.resize(mMeshVertices.size(), -1);
	mFirstOutFacePoly.clear();
	mFirstOutFacePoly.resize(clipConvexFaces.size(), -1);
	mNumOutFacePolys.clear();
	mNumOutFacePolys.resize(clipConvexFaces.size(), 0);

	mScene->profileBegin("create internal polygons");
	if (!createInternalPolygons(trans)) {
		empty = true;
		mScene->profileEnd("create internal polygons");
		return false;
	}
	mScene->profileEnd("create internal polygons");

	if (out.polyStarts.size() <= 1) {		// cut empty

		// inside visual mesh? 

		PxVec3 center(0.0f, 0.0f, 0.0f);
		for (uint32_t i = 0; i < clipConvexVertices.size(); i++) 
			center += clipConvexVertices[i];
		center /= (float)clipConvexVertices.size();
		if (mMeshConvex->insideVisualMesh(trans.transformInv(center))) {
			empty = false;
			completelyInside = true;
			return true;
		}
		else {
			empty = true;
			completelyInside = false;
			return true;
		}
	}

	empty = true;

	mScene->profileBegin("create face polys");
	bool OK = createFacePolys();
	mScene->profileEnd("create face polys");
	if (!OK)
		return false;

	//if (!testNeighbors()) 
	//	return false;

	if (splitIslands)
		this->splitIslands();

	empty = false;
	return true;
}



// --------------------------------------------------------------------------------------------
bool MeshClipper::splitIslands()
{
	if (mOutMeshes.size() < 1)
		return false;

	uint32_t numPolys = mOutMeshes[0].polyStarts.size()-1;
	mColors.clear();
	mColors.resize(numPolys, -1);
	mQueue.clear();

	int numIslands = 0;

	// color islands
	for (uint32_t i = 0; i < numPolys; i++) {
		if (mColors[i] >= 0)
			continue;
		numIslands++;
		mQueue.clear();
		mQueue.pushBack((int32_t)i);
		while (!mQueue.empty()) {
			uint32_t polyNr = (uint32_t)mQueue[mQueue.size()-1];
			mQueue.popBack();
			if (mColors[polyNr] >= 0)
				continue;
			mColors[polyNr] = numIslands-1;
			int first = mOutMeshes[0].polyStarts[polyNr];
			int last = mOutMeshes[0].polyStarts[polyNr+1];
			for (int j = first; j < last; j++) {
				int neighborNr = mOutMeshes[0].polyNeighbors[(uint32_t)j];
				if (mColors[(uint32_t)neighborNr] >= 0)
					continue;
				mQueue.pushBack(neighborNr);
			}
		}
	}

	// only one island?
	if (numIslands <= 1)
		return false;

	// create child meshes
	mOutMeshes.resize((uint32_t)numIslands+1);
	for (uint32_t i = 0; i < (uint32_t)numIslands; i++) {
		mOutMeshes[1+i].clear();
	}

	// distribute visual mesh
	mNewVertNr.clear();
	mNewVertNr.resize(mOutMeshes[0].vertices.size(), -1);
	mNewPolyNr.clear();
	mNewPolyNr.resize(numPolys, -1);

	for (uint32_t i = 0; i < numPolys; i++) {
		int meshNr = mColors[i];
		Mesh &parent = mOutMeshes[0];
		Mesh &child = mOutMeshes[(uint32_t)meshNr+1];
		int first = parent.polyStarts[i];
		int last = parent.polyStarts[i+1];
		for (int j = first; j < last; j++) {
			uint32_t id = (uint32_t)parent.polyIndices[(uint32_t)j];
			if (mNewVertNr[id] < 0) {
				mNewVertNr[id] = (int32_t)child.vertices.size();
				child.vertices.pushBack(parent.vertices[id]);
				child.normals.pushBack(parent.normals[id]);
				child.tangents.pushBack(parent.tangents[id]);
				child.texCoords.pushBack(parent.texCoords[2*id]);
				child.texCoords.pushBack(parent.texCoords[2*id+1]);
			}
			child.polyIndices.pushBack(mNewVertNr[id]);
			child.polyNeighbors.pushBack(parent.polyNeighbors[(uint32_t)j]);	// cannot adjust neighbor number yet
		}
		mNewPolyNr[i] = (int32_t)child.polyStarts.size()-1;
		child.polyStarts.pushBack((int32_t)child.polyIndices.size());
	}

	// adjust neighbor numbers
	//int num = 0;
	for (int i = 0; i < numIslands; i++) {
		Mesh &child = mOutMeshes[1+(uint32_t)i];
		for (int j = 0; j < (int)child.polyStarts.size()-1; j++) {
			int first = child.polyStarts[(uint32_t)j];
			int last = child.polyStarts[(uint32_t)j+1];
			for (int k = first; k < last; k++) {
				child.polyNeighbors[(uint32_t)k] = mNewPolyNr[(uint32_t)child.polyNeighbors[(uint32_t)k]];
			}
		}
	}
	return true;
}

// --------------------------------------------------------------------------------------------
bool MeshClipper::insideVisualMesh(const PxVec3 & /*pos*/) const
{
	//if (mMeshConvex == NULL)
	//	return false;

	//const nvidia::Array<PxVec3> &meshNormals = mMeshConvex->getVisNormals();
	//const nvidia::Array<int> &meshPolyStarts = mMeshConvex->getVisPolyStarts();
	//const nvidia::Array<int> &meshPolyIndices = mMeshConvex->getVisPolyIndices();
	//const nvidia::Array<int> &meshPolyNeighbors = mMeshConvex->getVisPolyNeighbors();

	//int num[6] = {0,0,0,0,0,0};
	//const int numAxes = 2;		// max 3: the nigher the more robust but slower

	//for (int i = 0; i < (int)mVisTriIndices.size(); i += 3) {
	//	const PxVec3 &p0 = mVisVertices[mVisTriIndices[i]];
	//	const PxVec3 &p1 = mVisVertices[mVisTriIndices[i+1]];
	//	const PxVec3 &p2 = mVisVertices[mVisTriIndices[i+2]];
	//	PxVec3 n = (p1-p0).cross(p2-p0);
	//	float d = n.dot(p0);
	//	float ds = d - pos.dot(n);

	//	// for all axes, cound the hits of the ray starting from pos with the mesh
	//	for (int j = 0; j < numAxes; j++) {
	//		if (n[j] == 0.0f) 
	//			continue;

	//		int j0 = (j+1)%3;
	//		int j1 = (j+2)%3;
	//		float x0 = p0[j0];
	//		float y0 = p0[j1];
	//		float x1 = p1[j0];
	//		float y1 = p1[j1];
	//		float x2 = p2[j0];
	//		float y2 = p2[j1];

	//		float px = pos[j0];
	//		float py = pos[j1];

	//		// inside triangle?
	//		float d0 = (x1-x0) * (py-y0) - (y1-y0) * (px-x0);
	//		float d1 = (x2-x1) * (py-y1) - (y2-y1) * (px-x1);
	//		float d2 = (x0-x2) * (py-y2) - (y0-y2) * (px-x2);
	//		bool inside = 
	//			(d0 <= 0.0f && d1 <= 0.0f && d2 <= 0.0f) ||
	//			(d0 >= 0.0f && d1 >= 0.0f && d2 >= 0.0f);
	//		if (!inside)
	//			continue;

	//		float s = ds / n[j];
	//		if (s > 0.0f) 
	//			num[2*j] += (n[j] > 0.0f) ? +1 : -1;
	//		else 
	//			num[2*j+1] += (n[j] < 0.0f) ? +1 : -1;
	//	}
	//}
	//int numVotes = 0;
	//for (int i = 0; i < 6; i++) {
	//	if (num[i] > 0)
	//		numVotes++;
	//}
	//return numVotes > numAxes;
	return false;
}

// --------------------------------------------------------------------------------------------
bool MeshClipper::testNeighbors(int meshNr)
{
	if (meshNr >= (int)mOutMeshes.size())
		return false;
	Mesh &mesh = mOutMeshes[(uint32_t)meshNr];

	for (int i = 0; i < (int)mesh.polyStarts.size()-1; i++) {
		int first = mesh.polyStarts[(uint32_t)i];
		int num = mesh.polyStarts[(uint32_t)i+1] - first;
		for (int j = 0; j < num; j++) {
			uint32_t i0 = (uint32_t)mesh.polyIndices[uint32_t(first+j)];
			uint32_t i1 = (uint32_t)mesh.polyIndices[uint32_t(first + (j+1)%num)];
			PxVec3 p0 = mesh.vertices[i0];
			PxVec3 p1 = mesh.vertices[i1];

			int adj = mesh.polyNeighbors[uint32_t(first + j)];
			if (adj < 0) 
				return false;

			int adjFirst = mesh.polyStarts[(uint32_t)adj];
			int adjNum = mesh.polyStarts[(uint32_t)adj+1] - adjFirst;
			bool found = false;
			for (int k = 0; k < adjNum; k++) {
				if (mesh.polyNeighbors[uint32_t(adjFirst+k)] == i) {
					uint32_t j0 = (uint32_t)mesh.polyIndices[uint32_t(adjFirst+k)];
					uint32_t j1 = (uint32_t)mesh.polyIndices[uint32_t(adjFirst + (k+1)%adjNum)];
					PxVec3 q0 = mesh.vertices[j0];
					PxVec3 q1 = mesh.vertices[j1];
					if (p0 == q1 && p1 == q0)
						found = true;
				}
			}
			if (!found)
				return false;
		}
	}
	return true;
}

// -----------------------------------------------------------------------------
bool MeshClipper::check(bool test, const char* /*message*/)
{
#ifndef _DEBUG
	return test;
#else

	if (test)
		return true;

	printf("\nerror in MeshClipper.cpp: %s\n", message);

	if (!mDumpOnError)
		return test;
#endif

#if DEBUG
	FILE *f = fopen(DUMP_PATH, "wb");
	if (f == NULL)
		return test;
#endif

#ifdef _DEBUG

	printf("dump file saved: %s\n", DUMP_PATH);

	const nvidia::Array<PxVec3> &meshVertices = mMeshConvex->getVisVertices();
	//const nvidia::Array<PxVec3> &meshNormals = mMeshConvex->getVisNormals();
	const nvidia::Array<int> &meshPolyStarts = mMeshConvex->getVisPolyStarts();
	const nvidia::Array<int> &meshPolyIndices = mMeshConvex->getVisPolyIndices();
	//const nvidia::Array<int> &meshPolyNeighbors = mMeshConvex->getVisPolyNeighbors();

	int cnt = (int)meshVertices.size();
	fwrite(&cnt, sizeof(int), 1, f);
	fwrite(&meshVertices[0], sizeof(PxVec3), meshVertices.size(), f);
	fwrite(&meshNormals[0], sizeof(PxVec3), meshNormals.size(), f);

	cnt = (int)meshPolyStarts.size();
	fwrite(&cnt, sizeof(int), 1, f);
	fwrite(&meshPolyStarts[0], sizeof(int), meshPolyStarts.size(), f);

	cnt = (int)meshPolyIndices.size();
	fwrite(&cnt, sizeof(int), 1, f);
	fwrite(&meshPolyIndices[0], sizeof(int), meshPolyIndices.size(), f);

	fclose(f);
	return false;
#endif
}

// -----------------------------------------------------------------------------
bool MeshClipper::loadDump()
{
#if DEBUG
	FILE *f = fopen(DUMP_PATH, "rb");
	if (f == NULL)
		return false;
#endif

	nvidia::Array<PxVec3> vertices;
	nvidia::Array<PxVec3> normals;
	nvidia::Array<int> polyStarts;
	nvidia::Array<int> polyIndices;
	nvidia::Array<int> polyNeighbors;

	uint32_t cnt = 0;
	fread(&cnt, sizeof(int), 1, f);
	vertices.resize(cnt);
	fread(&vertices[0], sizeof(PxVec3), vertices.size(), f);
	normals.resize(cnt);
	fread(&normals[0], sizeof(PxVec3), normals.size(), f);

	fread(&cnt, sizeof(int), 1, f);
	polyStarts.resize(cnt);
	fread(&polyStarts[0], sizeof(int), polyStarts.size(), f);

	fread(&cnt, sizeof(int), 1, f);
	polyIndices.resize(cnt);
	fread(&polyIndices[0], sizeof(int), polyIndices.size(), f);

	fclose(f);

	Convex *c = mScene->createConvex();

	bool isManifold = c->setExplicitVisMeshFromPolygons(
		(int32_t)vertices.size(), &vertices[0], &normals[0], NULL, NULL,	// todo, save as well
		(int32_t)polyStarts.size()-1, &polyStarts[0],
		(int32_t)polyIndices.size(), &polyIndices[0]);

	mMeshConvex = c;

	return isManifold;
}

}
}
}
#endif