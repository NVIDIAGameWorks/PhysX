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



#include "CookingAbstract.h"



namespace nvidia
{
namespace clothing
{

using namespace nvidia;

void CookingAbstract::PhysicalMesh::computeTriangleAreas()
{
	smallestTriangleArea = largestTriangleArea = 0.0f;

	if (indices == NULL || vertices == NULL)
	{
		return;
	}

	smallestTriangleArea = PX_MAX_F32;

	for (uint32_t i = 0; i < numIndices; i += 3)
	{
		const PxVec3 edge1 = vertices[indices[i + 1]] - vertices[indices[i]];
		const PxVec3 edge2 = vertices[indices[i + 2]] - vertices[indices[i]];
		const float triangleArea = edge1.cross(edge2).magnitude();

		largestTriangleArea = PxMax(largestTriangleArea, triangleArea);
		smallestTriangleArea = PxMin(smallestTriangleArea, triangleArea);
	}
}



void CookingAbstract::addPhysicalMesh(const PhysicalMesh& physicalMesh)
{
	PhysicalMesh physicalMeshCopy = physicalMesh;
	physicalMeshCopy.computeTriangleAreas();
	mPhysicalMeshes.pushBack(physicalMeshCopy);
}



void CookingAbstract::setConvexBones(const BoneActorEntry* boneActors, uint32_t numBoneActors, const BoneEntry* boneEntries,
                                     uint32_t numBoneEntries, const PxVec3* boneVertices, uint32_t maxConvexVertices)
{
	mBoneActors = boneActors;
	mNumBoneActors = numBoneActors;
	mBoneEntries = boneEntries;
	mNumBoneEntries = numBoneEntries;
	mBoneVertices = boneVertices;

	PX_ASSERT(maxConvexVertices <= 256);
	mMaxConvexVertices = maxConvexVertices;
}


bool CookingAbstract::isValid() const
{
	return mPhysicalMeshes.size() > 0;
}

}
}