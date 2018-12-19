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
#ifndef DELAUNAY_3D_BASE_H
#define DELAUNAY_3D_BASE_H

// Matthias Muller-Fischer

#include <PxVec3.h>
#include <PsArray.h>
#include <PsUserAllocated.h>

using namespace nvidia;

#include "CompoundGeometryBase.h"

namespace nvidia
{
namespace fracture
{
namespace base
{
	class SimScene;

// ---------------------------------------------------------------------------------------
class Delaunay3d : public UserAllocated {
	friend class SimScene;
public:
	// singleton pattern
	//static Delaunay3d* getInstance();
	//static void destroyInstance();

	// tetra mesh
	void tetrahedralize(const PxVec3 *vertices, int numVerts, int byteStride, bool removeExtraVerts = true);
	const nvidia::Array<int> getTetras() const { return mIndices; }

	// voronoi mesh, needs tetra mesh
	void computeVoronoiMesh();
	
	const CompoundGeometry &getGeometry() { return mGeom; }

protected:
	Delaunay3d(SimScene* scene): mScene(scene) {}
	virtual ~Delaunay3d() {}

	// ------------------------------------------------------
	struct Edge 
	{
		void init(int i0, int i1, int tetraNr, int neighborNr = -1) {
			this->tetraNr = tetraNr;
			this->neighborNr = neighborNr;
			if (i0 > i1) { this->i0 = i0; this->i1 = i1; }
			else { this->i0 = i1; this->i1 = i0; }
		}
		bool operator <(const Edge &e) const {
			if (i0 < e.i0) return true;
			if (i0 > e.i0) return false;
			if (i1 < e.i1) return true;
			if (i1 > e.i1) return false;
			return (neighborNr < e.neighborNr);
		}
		bool operator ==(Edge &e) const {
			return i0 == e.i0 && i1 == e.i1;
		}
		int i0, i1;
		int tetraNr;
		int neighborNr;
	};

	// ------------------------------------------------------
	struct Tetra 
	{
		void init(int i0, int i1, int i2, int i3) {
			ids[0] = i0; ids[1] = i1; ids[2] = i2; ids[3] = i3;
			neighborNrs[0] = neighborNrs[1] = neighborNrs[2] = neighborNrs[3] = -1;
			circumsphereDirty = true;
			center = PxVec3(0.0f, 0.0f, 0.0f);
			radiusSquared = 0.0f;
			deleted = false;
		}
		inline int& neighborOf(int i0, int i1, int i2) {
			if (ids[0] != i0 && ids[0] != i1 && ids[0] != i2) return neighborNrs[0]; 
			if (ids[1] != i0 && ids[1] != i1 && ids[1] != i2) return neighborNrs[1]; 
			if (ids[2] != i0 && ids[2] != i1 && ids[2] != i2) return neighborNrs[2]; 
			if (ids[3] != i0 && ids[3] != i1 && ids[3] != i2) return neighborNrs[3]; 
			return neighborNrs[0];
		}
		
		int ids[4];
		int neighborNrs[4];
		bool circumsphereDirty;
		PxVec3 center;
		float radiusSquared;
		bool deleted;

		static const int sideIndices[4][3];
	};

	// ------------------------------------------------------
	void clear();
	void delaunayTetrahedralization();

	int findSurroundingTetra(int startTetra, const PxVec3 &p) const;
	void updateCircumSphere(Tetra &tetra);
	bool pointInCircumSphere(Tetra &tetra, const PxVec3 &p);
	void retriangulate(int tetraNr, int vertNr);
	void compressTetrahedra(bool removeExtraVerts);

	SimScene* mScene;

	int mFirstFarVertex;
	int mLastFarVertex;
	nvidia::Array<PxVec3> mVertices;
	nvidia::Array<Tetra> mTetras;
	nvidia::Array<int> mIndices;

	CompoundGeometry mGeom;

	mutable nvidia::Array<int> mTetMarked;
	mutable int mTetMark;

};

}
}
}

#endif
#endif
