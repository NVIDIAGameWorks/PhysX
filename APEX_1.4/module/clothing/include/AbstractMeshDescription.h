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



#ifndef ABSTRACT_MESH_DESCRIPTION_H
#define ABSTRACT_MESH_DESCRIPTION_H

#include <ApexUsingNamespace.h>
#include "PxVec3.h"

class ClothConstrainCoefficients;


namespace nvidia
{
namespace apex
{

class RenderDebugInterface;
/**
\brief a simplified, temporal container for a mesh with non-interleaved vertex buffers
*/

struct AbstractMeshDescription
{
	AbstractMeshDescription() : numVertices(0), numIndices(0), numBonesPerVertex(0),
		pPosition(NULL), pNormal(NULL), pTangent(NULL), pTangent4(NULL), pBitangent(NULL),
		pBoneIndices(NULL), pBoneWeights(NULL), pConstraints(NULL), pVertexFlags(NULL), pIndices(NULL),
		avgEdgeLength(0.0f), avgTriangleArea(0.0f), pMin(0.0f), pMax(0.0f), centroid(0.0f), radius(0.0f) {}

	/// the number of vertices in the mesh
	uint32_t	numVertices;
	/// the number of indices in the mesh
	uint32_t	numIndices;
	/// the number of bones per vertex in the boneIndex and boneWeights buffer. Can be 0
	uint32_t	numBonesPerVertex;

	/// pointer to the positions array
	PxVec3*	PX_RESTRICT pPosition;
	/// pointer to the normals array
	PxVec3*	PX_RESTRICT pNormal;
	/// pointer to the tangents array
	PxVec3*	PX_RESTRICT pTangent;
	/// alternative pointer to the tangents array, with float4
	PxVec4*	PX_RESTRICT pTangent4;
	/// pointer to the bitangents/binormal array
	PxVec3*	PX_RESTRICT pBitangent;
	/// pointer to the bone indices array
	uint16_t*	PX_RESTRICT pBoneIndices;
	/// pointer to the bone weights array
	float*	PX_RESTRICT pBoneWeights;
	/// pointer to the cloth constraints array
	ClothConstrainCoefficients* PX_RESTRICT pConstraints;
	/// pointer to per-vertex flags
	uint32_t*	PX_RESTRICT pVertexFlags;
	/// pointer to the indices array
	uint32_t*	PX_RESTRICT pIndices;

	/// updates the derived data
	void UpdateDerivedInformation(RenderDebugInterface* renderDebug);

	/// Derived Data, average Edge Length
	float	avgEdgeLength;
	/// Derived Data, average Triangle Area
	float	avgTriangleArea;
	/// Derived Data, Bounding Box min value
	PxVec3	pMin;
	/// Derived Data, Bounding Box max value
	PxVec3	pMax;
	/// Derived Data, Average of pMin and pMax
	PxVec3	centroid;
	/// Derived Data, Half the distance between pMin and pMax
	float	radius;
};

}
}


#endif // ABSTRACT_MESH_DESCRIPTION_H
