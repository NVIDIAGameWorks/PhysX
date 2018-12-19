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
#include "IslandDetectorBase.h"
#include "ConvexBase.h"
#include "CompoundGeometryBase.h"
#include "PsSort.h"

namespace nvidia
{
namespace fracture
{
namespace base
{

using namespace nvidia;

// -----------------------------------------------------------------------------
IslandDetector::IslandDetector(SimScene* scene):
	mScene(scene)
{
	mNeigborPairsDirty = true;
}

// -----------------------------------------------------------------------------
void IslandDetector::detect(const nvidia::Array<Convex*> &convexes, bool computeFaceCoverage)
{
	createConnectivity(convexes, computeFaceCoverage);
	addOverlapConnectivity(convexes);
	createIslands();
	mNeigborPairsDirty = true;
}

// -----------------------------------------------------------------------------
bool IslandDetector::touching(const Convex *c0, int faceNr, const Convex *c1, float eps)
{
	const Convex::Face &face = c0->getFaces()[(uint32_t)faceNr];
	const nvidia::Array<int> &indices = c0->getIndices();
	const nvidia::Array<PxVec3> &verts = c0->getVertices();
	const nvidia::Array<PxPlane> &planes = c1->getPlanes();

	for (int i = 0; i < face.numIndices; i++) {
		PxVec3 v = verts[(uint32_t)indices[uint32_t(face.firstIndex + i)]];
		v = v + c0->getMaterialOffset() - c1->getMaterialOffset();	// transform to c1
		bool inside = true;
		for (uint32_t j = 0; j < planes.size(); j++) {
			const PxPlane &p = planes[j];
			if (p.n.dot(v) - p.d > eps) {
				inside = false;
				break;
			}
		}
		if (inside)
			return true;
	}
	return false;
}

// -----------------------------------------------------------------------------
void IslandDetector::createConnectivity(const nvidia::Array<Convex*> &convexes, bool computeFaceCoverage)
{
	mFaces.clear();
	Face face;
	int globalNr = 0;

	for (uint32_t i = 0; i < convexes.size(); i++) {
		const Convex *c = convexes[i];
		face.convexNr = (int32_t)i;
		const nvidia::Array<PxPlane> &planes = c->getPlanes();
		for (uint32_t j = 0; j < planes.size(); j++) {
			globalNr++;
			if (c->getFaces()[j].flags & CompoundGeometry::FF_OBJECT_SURFACE)
				continue;
			face.faceNr = (int32_t)j;
			face.globalNr = globalNr-1;
			float d = planes[j].d;
			d = d + c->getMaterialOffset().dot(planes[j].n);	// transform to global
			face.orderVal = fabsf(d);
			mFaces.pushBack(face);
		}
	}
	mFaceCoverage.clear();

	if (computeFaceCoverage) {
		mFaceCoverage.resize((uint32_t)globalNr, 0.0f);
	}

	shdfnd::sort(mFaces.begin(), mFaces.size());

	float eps = 1e-3f;
	mFirstNeighbor.clear();
	mFirstNeighbor.resize(convexes.size(), -1);
	mNeighbors.clear();
	Neighbor n;

	for (uint32_t i = 0; i < mFaces.size(); i++) {
		Face &fi = mFaces[i];
		uint32_t j = i+1;
		while (j < mFaces.size() && mFaces[j].orderVal - fi.orderVal < eps) {
			Face &fj = mFaces[j];
			j++;

			if (fi.convexNr == fj.convexNr)
				continue;

			PxPlane pi = convexes[(uint32_t)fi.convexNr]->getPlanes()[(uint32_t)fi.faceNr];
			if (convexes[(uint32_t)fi.convexNr]->isGhostConvex()) 
				pi.n = -pi.n; 

			PxPlane pj = convexes[(uint32_t)fj.convexNr]->getPlanes()[(uint32_t)fj.faceNr];
			if (convexes[(uint32_t)fj.convexNr]->isGhostConvex()) 
				pj.n = -pj.n; 

			if (pi.n.dot(pj.n) > -1.0f + eps)
				continue;	// need to be opposite

			if (
				touching(convexes[(uint32_t)fi.convexNr], fi.faceNr, convexes[(uint32_t)fj.convexNr], eps) ||
				touching(convexes[(uint32_t)fj.convexNr], fj.faceNr, convexes[(uint32_t)fi.convexNr], eps))
			{
				n.area = 0.5f*( fabsf(faceArea(convexes[(uint32_t)fi.convexNr], fi.faceNr)) + fabsf(faceArea(convexes[(uint32_t)fj.convexNr], fj.faceNr)));
				if (computeFaceCoverage) {
					float area = faceIntersectionArea(convexes[(uint32_t)fi.convexNr], fi.faceNr, convexes[(uint32_t)fj.convexNr], fj.faceNr);
					mFaceCoverage[(uint32_t)fi.globalNr] += area;
					mFaceCoverage[(uint32_t)fj.globalNr] += area;
				}

				n.convexNr = fj.convexNr;
				n.next = mFirstNeighbor[(uint32_t)fi.convexNr];
				mFirstNeighbor[(uint32_t)fi.convexNr] = (int32_t)mNeighbors.size();
				mNeighbors.pushBack(n);

				n.convexNr = fi.convexNr;
				n.next = mFirstNeighbor[(uint32_t)fj.convexNr];
				mFirstNeighbor[(uint32_t)fj.convexNr] = (int32_t)mNeighbors.size();
				mNeighbors.pushBack(n);
			}
		}
	}

	if (computeFaceCoverage) {
		uint32_t globalNr = 0;

		for (uint32_t i = 0; i < convexes.size(); i++) {
			const Convex *c = convexes[i];
			int numFaces = (int32_t)c->getPlanes().size();
			for (int j = 0; j < numFaces; j++) {
				float total = faceArea(c, j);
				if (total != 0.0f)
					mFaceCoverage[globalNr] /= total;
				globalNr++;
			}
		}
	}

	mNeigborPairsDirty = true;
}

// -----------------------------------------------------------------------------
bool IslandDetector::axisOverlap(const Convex* c0, const Convex* c1, PxVec3 &dir)
{
	float off0 = c0->getMaterialOffset().dot(dir);
	float off1 = c1->getMaterialOffset().dot(dir);
	const nvidia::Array<PxVec3> &verts0 = c0->getVertices();
	const nvidia::Array<PxVec3> &verts1 = c1->getVertices();
	if (verts0.empty() || verts1.empty())
		return false;

	float d0 = dir.dot(verts0[0]) + off0;
	float d1 = dir.dot(verts1[0]) + off1;
	if (d0 < d1) {
		float max0 = d0;
		for (uint32_t i = 1; i < verts0.size(); i++) {
			float d = verts0[i].dot(dir) + off0;
			if (d > max0) max0 = d;
		}
		for (uint32_t i = 0; i < verts1.size(); i++) {
			float d = verts1[i].dot(dir) + off1;
			if (d < max0) 
				return true;
		}
	}
	else {
		float max1 = d1;
		for (uint32_t i = 1; i < verts1.size(); i++) {
			float d = verts1[i].dot(dir) + off1;
			if (d > max1) max1 = d;
		}
		for (uint32_t i = 0; i < verts0.size(); i++) {
			float d = verts0[i].dot(dir) + off0;
			if (d < max1)
				return true;
		}
	}
	return false;
}

// -----------------------------------------------------------------------------
bool IslandDetector::overlap(const Convex* c0, const Convex* c1)
{
	PxVec3 off0 = c0->getMaterialOffset();
	PxVec3 off1 = c1->getMaterialOffset();
	PxVec3 center0 = c0->getCenter() + off0;
	PxVec3 center1 = c1->getCenter() + off1;
	PxVec3 dir = center1 - center0;

	if (!axisOverlap(c0, c1, dir))
		return false;

	// todo: test other axes

	return true;
}

// -----------------------------------------------------------------------------
void IslandDetector::addOverlapConnectivity(const nvidia::Array<Convex*> &convexes)
{
	mBounds.resize(convexes.size());
	PxVec3 off;

	for (uint32_t i = 0; i < convexes.size(); i++) {
		const Convex *c = convexes[i];
		const PxBounds3 &cb = c->getBounds();
		off = c->getMaterialOffset();
		
		PxBounds3 &b = mBounds[i];
		b.minimum = cb.minimum + off;
		b.maximum = cb.maximum + off;
	}

	// broad phase with AABBs
	Axes axes;
	axes.set(0,2,1);
	mBoxPruning.completeBoxPruning(mBounds, mPairs, axes);

	for (uint32_t i = 0; i < mPairs.size(); i += 2) {
		uint32_t i0 = mPairs[i];
		uint32_t i1 = mPairs[i+1];

		if (convexes[i0]->getModelIslandNr() == convexes[i1]->getModelIslandNr())
			continue;

		// already detected?
		bool found = false;
		int nr = mFirstNeighbor[i0];
		while (nr >= 0) {
			Neighbor &n = mNeighbors[(uint32_t)nr];
			nr = n.next;
			if (n.convexNr == (int32_t)i1) {
				found = true;
				break;
			}
		}
		if (found)
			continue;

		if (overlap(convexes[i0], convexes[i1])) {
			Neighbor n;

			// add link
			n.convexNr = (int32_t)i0;
			n.next = mFirstNeighbor[i1];
			mFirstNeighbor[i1] = (int32_t)mNeighbors.size();
			mNeighbors.pushBack(n);

			n.convexNr = (int32_t)i1;
			n.next = mFirstNeighbor[i0];
			mFirstNeighbor[i0] = (int32_t)mNeighbors.size();
			mNeighbors.pushBack(n);
		}
		
	}
}

// -----------------------------------------------------------------------------
void IslandDetector::createIslands()
{
	uint32_t numConvexes = mFirstNeighbor.size();
	mIslands.clear();
	mIslandConvexes.clear();

	Island island;

	// color convexes
	mColors.clear();
	mColors.resize(numConvexes,-1);
	int color = -1;

	for (uint32_t i = 0; i < numConvexes; i++) {
		if (mColors[i] >= 0)
			continue;
		mQueue.clear();
		mQueue.pushBack((int32_t)i);
		color++;
		island.firstNr = (int32_t)mIslandConvexes.size();
		island.size = 0;

		while (mQueue.size() > 0) {
			int convexNr = mQueue[mQueue.size()-1];
			mQueue.popBack();
			if (mColors[(uint32_t)convexNr] >= 0)
				continue;
			mColors[(uint32_t)convexNr] = color;
			mIslandConvexes.pushBack(convexNr);
			island.size++;

			int nr = mFirstNeighbor[(uint32_t)convexNr];
			while (nr >= 0) {
				int adj = mNeighbors[(uint32_t)nr].convexNr;
				nr = mNeighbors[(uint32_t)nr].next;
				if (mColors[(uint32_t)adj] < 0)
					mQueue.pushBack(adj);
			}
		}
		mIslands.pushBack(island);
	}
}

// -----------------------------------------------------------------------------
float IslandDetector::faceArea(const Convex *c, int faceNr)
{
	const Convex::Face &face = c->getFaces()[(uint32_t)faceNr];
	const nvidia::Array<PxVec3> &verts = c->getVertices();
	const nvidia::Array<int> &indices = c->getIndices();

	float area = 0.0f;
	PxVec3 p0 = verts[(uint32_t)indices[(uint32_t)face.firstIndex]];
	for (int i = 1; i < face.numIndices-1; i++) {
		PxVec3 p1 = verts[(uint32_t)indices[uint32_t(face.firstIndex+i)]];
		PxVec3 p2 = verts[(uint32_t)indices[uint32_t(face.firstIndex+i+1)]];
		area += (p1-p0).cross(p2-p0).magnitude();
	}
	return 0.5f * area;
}

// -----------------------------------------------------------------------------
float IslandDetector::faceIntersectionArea(const Convex *c0, int faceNr0, const Convex *c1, int faceNr1)
{
	// vertices of the first face
	mCutPolys[0].clear();
	mCutPolys[1].clear();

	const Convex::Face &face0 = c0->getFaces()[(uint32_t)faceNr0];
	const nvidia::Array<PxVec3> &verts0 = c0->getVertices();
	const nvidia::Array<int> &indices0 = c0->getIndices();

	const Convex::Face &face1 = c1->getFaces()[(uint32_t)faceNr1];
	const nvidia::Array<PxVec3> &verts1 = c1->getVertices();
	const nvidia::Array<int> &indices1 = c1->getIndices();

	for (int i = 0; i < face0.numIndices; i++) 
		mCutPolys[0].pushBack(verts0[(uint32_t)indices0[uint32_t(face0.firstIndex + i)]]);

	const PxVec3 &n0 = c0->getPlanes()[(uint32_t)faceNr0].n;

	// cut face 0 polygon with face 1 polygon
	for (int i = 0; i < face1.numIndices; i++) {
		const PxVec3 &p0 = verts1[(uint32_t)indices1[uint32_t(face1.firstIndex + (i+1)%face1.numIndices)]];
		const PxVec3 &p1 = verts1[(uint32_t)indices1[uint32_t(face1.firstIndex + i)]];
		PxVec3 n = (p1-p0).cross(n0);
		float d = n.dot(p0);

		nvidia::Array<PxVec3> &currPoly = mCutPolys[uint32_t(i%2)];
		nvidia::Array<PxVec3> &newPoly = mCutPolys[uint32_t(1 - (i%2))];
		newPoly.clear();

		uint32_t num = currPoly.size();
		for (uint32_t j = 0; j < num; j++) {
			PxVec3 &q0 = currPoly[j];
			PxVec3 &q1 = currPoly[(j+1) % num];

			bool inside0 = q0.dot(n) < d;
			bool inside1 = q1.dot(n) < d;

			if (inside0) 
				newPoly.pushBack(currPoly[j]);	// point inside -> keep

			if (inside0 != inside1) {		// intersection
				float s = (d - q0.dot(n)) / (q1-q0).dot(n);
				PxVec3 ps = q0 + s*(q1-q0);
				newPoly.pushBack(ps);
			}
		}
	}

	// measure area
	float area = 0.0f;
	nvidia::Array<PxVec3> &currPoly = mCutPolys[face1.numIndices%2];
	if (currPoly.size() < 3)
		return 0.0f;

	PxVec3 &p0 = currPoly[0];
	for (uint32_t i = 1; i < currPoly.size()-1; i++) {
		PxVec3 &p1 = currPoly[i];
		PxVec3 &p2 = currPoly[i+1];
		area += (p1-p0).cross(p2-p0).magnitude();
	}
	return 0.5f * area;
}

// -----------------------------------------------------------------------------
const nvidia::Array<IslandDetector::Edge> &IslandDetector::getNeighborEdges()
{
	if (mNeigborPairsDirty) {
		Edge e;
		mNeighborPairs.clear();
		for (uint32_t i = 0; i < mFirstNeighbor.size(); i++) {
			int nr = mFirstNeighbor[i];
			e.n0 = (int32_t)i;
			while (nr >= 0) {
				Neighbor &n = mNeighbors[(uint32_t)nr];
				nr = n.next;
				if (n.convexNr > (int32_t)i) {
					e.n1 = n.convexNr;
					e.area = n.area;
					mNeighborPairs.pushBack(e);
					//mNeighborPairs.pushBack(i);
					//mNeighborPairs.pushBack(n.convexNr);
				}
			}
		}
		mNeigborPairsDirty = false;
	}
	return mNeighborPairs;
}

}
}
}
#endif