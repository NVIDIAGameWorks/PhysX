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
#ifndef ISLAND_DETECTOR_BASE_H
#define ISLAND_DETECTOR_BASE_H

// Matthias Muller-Fischer

#include <PxVec3.h>
#include <PsArray.h>
#include "IceBoxPruningBase.h"
#include <PsUserAllocated.h>

namespace nvidia
{
namespace fracture
{
namespace base
{

class SimScene;
class Convex;

// ---------------------------------------------------------------------------------------
class IslandDetector : public UserAllocated {
	friend class SimScene;
public:
	// singleton pattern
	//static IslandDetector* getInstance();
	//static void destroyInstance();

	void detect(const nvidia::Array<Convex*> &convexes, bool computeFaceCoverage);

	struct Island {
		int size;
		int firstNr;
	};
	const nvidia::Array<Island> &getIslands() { return mIslands; }
	const nvidia::Array<int> &getIslandConvexes() { return mIslandConvexes; }

	const nvidia::Array<float> &getFaceCoverage() { return mFaceCoverage; }

	class Edge{
	public:
		int n0, n1;
		float area;
	};
	const nvidia::Array<Edge> &getNeighborEdges();

protected:
	IslandDetector(SimScene* scene);
	virtual ~IslandDetector(){}

	void createConnectivity(const nvidia::Array<Convex*> &convexes, bool computeFaceCoverage);
	static bool touching(const Convex *c0, int faceNr, const Convex *c1, float eps);
	void createIslands();

	float faceArea(const Convex *c, int faceNr);
	float faceIntersectionArea(const Convex *c0, int faceNr0, const Convex *c1, int faceNr1);

	bool axisOverlap(const Convex* c0, const Convex* c1, PxVec3 &dir);
	bool overlap(const Convex* c0, const Convex* c1);

	void addOverlapConnectivity(const nvidia::Array<Convex*> &convexes);

	SimScene* mScene;

	// auxiliary
	nvidia::Array<int> mFirstNeighbor;
	struct Neighbor {
		int convexNr;
		int next;
		float area;
	};
	nvidia::Array<Neighbor> mNeighbors;
	struct Face {
		int convexNr;
		int faceNr;
		float orderVal;
		int globalNr;
		bool operator < (const Face &f) const { return orderVal < f.orderVal; }
	};
	bool mNeigborPairsDirty;
	nvidia::Array<Edge> mNeighborPairs;
	nvidia::Array<float> mNeighborAreaPairs;
	nvidia::Array<Face> mFaces;
	nvidia::Array<int> mColors;
	nvidia::Array<int> mQueue;

	nvidia::Array<PxVec3> mCutPolys[2];

	// overlaps
	BoxPruning mBoxPruning;
	nvidia::Array<PxBounds3> mBounds;
	nvidia::Array<uint32_t> mPairs;

	// output
	nvidia::Array<Island> mIslands;
	nvidia::Array<int> mIslandConvexes;
	nvidia::Array<float> mFaceCoverage;
};

}
}
}

#endif
#endif