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


#ifndef Hull2MESHEDGES_H

#define Hull2MESHEDGES_H

// This is a small code snippet which will take a convex hull described as an array of plane equations and, from that,
// produce either an edge list (for wireframe debug visualization) or a triangle mesh.

#include "PxVec3.h"
#include "PxPlane.h"

struct HullEdge
{
	physx::PxVec3	e0,e1;
};

struct HullMesh
{
	uint32_t		mVertexCount;
	uint32_t		mTriCount;
	const physx::PxVec3 *mVertices;
	const uint16_t	*mIndices;
};

class Hull2MeshEdges
{
public:
	// Convert convex hull into a list of edges.
	virtual const HullEdge *getHullEdges(uint32_t planeCount,		// Number of source planes
										const physx::PxPlane *planes,	// The array of plane equations
										uint32_t &edgeCount) = 0;	// Contains the number of edges built

	virtual bool getHullMesh(uint32_t planeCount,				// Number of source planes
							 const physx::PxPlane *planes,			// The array of plane equations
							 HullMesh &m) = 0;						// The triangle mesh produced

	virtual void release(void) = 0;
protected:
	virtual ~Hull2MeshEdges(void)
	{

	}
};

Hull2MeshEdges *createHull2MeshEdges(void);

#endif
