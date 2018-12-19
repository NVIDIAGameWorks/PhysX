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
#ifndef COMPOUND_GEOMETRY_BASE
#define COMPOUND_GEOMETRY_BASE

#include <foundation/PxVec3.h>
#include <foundation/PxPlane.h>
#include <PsArray.h>
#include <PsUserAllocated.h>

namespace physx
{
namespace fracture
{
namespace base
{

// -----------------------------------------------------------------------------------
class CompoundGeometry : public ::physx::shdfnd::UserAllocated
{
public:
	CompoundGeometry() {}
	virtual ~CompoundGeometry() {}
	bool loadFromFile(const char *filename);
	bool saveFromFile(const char *filename);
	void clear();
	void derivePlanes();

	virtual void debugDraw(int /*maxConvexes*/ = 0) const {}

	struct Convex {			// init using CompoundGeometry::initConvex()
		int firstVert;
		int numVerts;
		int firstNormal;	// one per face index!	If not provided (see face flags) face normal is used
		int firstIndex;
		int numFaces;
		int firstPlane;
		int firstNeighbor;
		int numNeighbors;
		float radius;
		bool isSphere;
	};

	void initConvex(Convex &c);

	shdfnd::Array<Convex> convexes;
	shdfnd::Array<PxVec3> vertices;
	shdfnd::Array<PxVec3> normals;	// one per face and vertex!
	// face size, face flags, id, id, .., face size, face flags, id ..
	shdfnd::Array<int> indices;
	shdfnd::Array<int> neighbors;

	shdfnd::Array<PxPlane> planes;  // derived for faster cuts

	enum FaceFlags {
		FF_OBJECT_SURFACE = 1,
		FF_HAS_NORMALS = 2,
		FF_INVISIBLE = 4,
		FF_NEW = 8,
	};
};

}
}
}

#endif
#endif