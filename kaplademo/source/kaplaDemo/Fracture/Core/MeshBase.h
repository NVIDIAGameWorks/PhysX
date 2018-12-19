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
#ifndef MESH_BASE
#define MESH_BASE

#include <foundation/PxVec3.h>
#include <foundation/PxVec2.h>
#include <PsArray.h>
#include <PsUserAllocated.h>

namespace physx
{
namespace fracture
{
namespace base
{

// -----------------------------------------------------------------------------------
class Mesh : public ::physx::shdfnd::UserAllocated
{
public:
	Mesh() {};
	virtual ~Mesh() {};

	const shdfnd::Array<PxVec3> &getVertices() const { return mVertices; }
	const shdfnd::Array<PxVec3> &getNormals() const { return mNormals; }
	const shdfnd::Array<PxVec2> &getTexCoords() const { return mTexCoords; }
	const shdfnd::Array<PxU32> &getIndices() const { return mIndices; }
	const shdfnd::Array<int> &getNeighbors() const { return mNeighbors; }

	struct SubMesh {
		void init() { /*name = "";*/ firstIndex = -1; numIndices = 0; }
		//std::string name;
		int firstIndex;
		int numIndices;
	};

	const shdfnd::Array<SubMesh> &getSubMeshes() const { return mSubMeshes; }

	void normalize(const PxVec3 &center, float diagonal);
	void scale(float diagonal);
	void getBounds(PxBounds3 &bounds, int subMeshNr = -1) const;

	void flipV(); // flips v values to hand coordinate system change (Assumes normalized coordinates)

	bool computeNeighbors();
	bool computeWeldedNeighbors();

protected:
	void clear();
	void updateNormals();

	shdfnd::Array<PxVec3> mVertices;
	shdfnd::Array<PxVec3> mNormals;
	shdfnd::Array<PxVec2> mTexCoords;

	shdfnd::Array<SubMesh> mSubMeshes;
	shdfnd::Array<PxU32> mIndices;
	shdfnd::Array<int> mNeighbors;
};

}
}
}

#endif
#endif