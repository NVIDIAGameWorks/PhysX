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
#ifndef POLYGON_TRIANGULATOR_BASE_H
#define POLYGON_TRIANGULATOR_BASE_H

#include <foundation/PxVec3.h>
#include <foundation/PxMath.h>
#include <PsArray.h>
#include <PsUserAllocated.h>

namespace physx
{
namespace fracture
{
namespace base
{

// ------------------------------------------------------------------------------

class PolygonTriangulator : public ::physx::shdfnd::UserAllocated {
	friend class SimScene;
public: 
	// singleton pattern
	//static PolygonTriangulator* getInstance();
	//static void destroyInstance();

	void triangulate(const PxVec3 *points, int numPoints, const int *indices = NULL, PxVec3 *planeNormal = NULL);
	const shdfnd::Array<int> &getIndices() const { return mIndices; }

protected:
	void importPoints(const PxVec3 *points, int numPoints, const int *indices, PxVec3 *planeNormal, bool &isConvex);
	void clipEars();

	inline float cross(const PxVec3 &p0, const PxVec3 &p1);
	bool inTriangle(const PxVec3 &p, const PxVec3 &p0, const PxVec3 &p1, const PxVec3 &p2);

	PolygonTriangulator(SimScene* scene);
	virtual ~PolygonTriangulator();

	SimScene* mScene;

	shdfnd::Array<PxVec3> mPoints;
	shdfnd::Array<int> mIndices;

	struct Corner {
		int prev;
		int next;
		bool isEar;
		float angle;
	};
	shdfnd::Array<Corner> mCorners;
};

}
}
}

#endif
#endif
