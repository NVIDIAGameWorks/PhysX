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


#ifndef APEX_CSG_HULL_H
#define APEX_CSG_HULL_H

#include "ApexUsingNamespace.h"
#include "authoring/ApexCSGMath.h"
#include "PsArray.h"
#include "PxFileBuf.h"

#ifndef WITHOUT_APEX_AUTHORING

namespace ApexCSG
{

/* Convex hull that handles unbounded sets. */

class Hull
{
public:
	struct Edge
	{
		uint32_t	m_indexV0;
		uint32_t	m_indexV1;
		uint32_t	m_indexF1;
		uint32_t	m_indexF2;
	};

	struct EdgeType
	{
		enum Enum
		{
			LineSegment,
			Ray,
			Line
		};
	};

	PX_INLINE					Hull()
	{
		setToAllSpace();
	}
	PX_INLINE					Hull(const Hull& geom)
	{
		*this = geom;
	}

	PX_INLINE	void			setToAllSpace()
	{
		clear();
		allSpace = true;
	}
	PX_INLINE	void			setToEmptySet()
	{
		clear();
		emptySet = true;
	}

	void			intersect(const Plane& plane, Real distanceTol);

	PX_INLINE	void			transform(const Mat4Real& tm, const Mat4Real& cofTM);

	PX_INLINE	uint32_t	getFaceCount() const
	{
		return faces.size();
	}
	PX_INLINE	const Plane&	getFace(uint32_t faceIndex) const
	{
		return faces[faceIndex];
	}

	PX_INLINE	uint32_t	getEdgeCount() const
	{
		return edges.size();
	}
	PX_INLINE	const Edge&		getEdge(uint32_t edgeIndex) const
	{
		return edges[edgeIndex];
	}

	PX_INLINE	uint32_t	getVertexCount() const
	{
		return vertexCount;
	}
	PX_INLINE	const Pos&		getVertex(uint32_t vertexIndex) const
	{
		return *(const Pos*)(vectors.begin() + vertexIndex);
	}

	PX_INLINE	bool			isEmptySet() const
	{
		return emptySet;
	}
	PX_INLINE	bool			isAllSpace() const
	{
		return allSpace;
	}

	Real			calculateVolume() const;

	// Edge accessors
	PX_INLINE	EdgeType::Enum	getType(const Edge& edge) const
	{
		return (EdgeType::Enum)((uint32_t)(edge.m_indexV0 >= vertexCount) + (uint32_t)(edge.m_indexV1 >= vertexCount));
	}
	PX_INLINE	const Pos&		getV0(const Edge& edge)	const
	{
		return *(Pos*)(vectors.begin() + edge.m_indexV0);
	}
	PX_INLINE	const Pos&		getV1(const Edge& edge)	const
	{
		return *(Pos*)(vectors.begin() + edge.m_indexV1);
	}
	PX_INLINE	const Dir&		getDir(const Edge& edge)	const
	{
		PX_ASSERT(edge.m_indexV1 >= vertexCount);
		return *(Dir*)(vectors.begin() + edge.m_indexV1);
	}
	PX_INLINE	uint32_t	getF1(const Edge& edge)	const
	{
		return edge.m_indexF1;
	}
	PX_INLINE	uint32_t	getF2(const Edge& edge)	const
	{
		return edge.m_indexF2;
	}

	// Serialization
	void			serialize(physx::PxFileBuf& stream) const;
	void			deserialize(physx::PxFileBuf& stream, uint32_t version);

protected:
	PX_INLINE	void			clear();

	bool			testConsistency(Real distanceTol, Real angleTol) const;

	// Faces
	physx::Array<Plane>		faces;
	physx::Array<Edge>		edges;
	physx::Array<Vec4Real>	vectors;
	uint32_t						vertexCount;	// vectors[i], i >= vertexCount, are used to store vectors for ray and line edges
	bool						allSpace;
	bool						emptySet;
};

PX_INLINE void
Hull::transform(const Mat4Real& tm, const Mat4Real& cofTM)
{
	for (uint32_t i = 0; i < faces.size(); ++i)
	{
		Plane& face = faces[i];
		face = cofTM * face;
		face.normalize();
	}

	for (uint32_t i = 0; i < vectors.size(); ++i)
	{
		Vec4Real& vector = vectors[i];
		vector = tm * vector;
	}
}

PX_INLINE void
Hull::clear()
{
	vectors.reset();
	edges.reset();
	faces.reset();
	vertexCount = 0;
	allSpace = false;
	emptySet = false;
}


};	// namespace ApexCSG

#endif

#endif // #define APEX_CSG_HULL_H
