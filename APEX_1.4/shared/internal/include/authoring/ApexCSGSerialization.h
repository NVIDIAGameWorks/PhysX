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


#ifndef APEX_CSG_SERIALIZATION_H
#define APEX_CSG_SERIALIZATION_H

#include "ApexUsingNamespace.h"
#include "ApexSharedUtils.h"
#include "ApexStream.h"
#include "authoring/ApexCSGDefs.h"
#include "authoring/ApexCSGHull.h"

#ifndef WITHOUT_APEX_AUTHORING

namespace nvidia
{
namespace apex
{

/* Version for serialization */
struct Version
{
	enum Enum
	{
		Initial = 0,
		RevisedMeshTolerances,
		UsingOnlyPositionDataInVertex,
		SerializingTriangleFrames,
		UsingGSA,
		SerializingMeshBounds,
		AddedInternalTransform,
		IncidentalMeshDistinction,

		Count,
		Current = Count - 1
	};
};


// Vec<T,D>
template<typename T, int D>
PX_INLINE physx::PxFileBuf&
operator << (physx::PxFileBuf& stream, const ApexCSG::Vec<T, D>& v)
{
	for (uint32_t i = 0; i < D; ++i)
	{
		stream << v[(int32_t)i];
	}
	return stream;
}

template<typename T, int D>
PX_INLINE physx::PxFileBuf&
operator >> (physx::PxFileBuf& stream, ApexCSG::Vec<T, D>& v)
{
	for (uint32_t i = 0; i < D; ++i)
	{
		stream >> v[(int32_t)i];
	}

	return stream;
}


// Edge
PX_INLINE void
serialize(physx::PxFileBuf& stream, const ApexCSG::Hull::Edge& e)
{
	stream << e.m_indexV0 << e.m_indexV1 << e.m_indexF1 << e.m_indexF2;
}

PX_INLINE void
deserialize(physx::PxFileBuf& stream, uint32_t version, ApexCSG::Hull::Edge& e)
{
	PX_UNUSED(version);	// Initial

	stream >> e.m_indexV0 >> e.m_indexV1 >> e.m_indexF1 >> e.m_indexF2;
}


// Region
PX_INLINE void
serialize(physx::PxFileBuf& stream, const ApexCSG::Region& r)
{
	stream << r.side;
}

PX_INLINE void
deserialize(physx::PxFileBuf& stream, uint32_t version, ApexCSG::Region& r)
{
	if (version < Version::UsingGSA)
	{
		ApexCSG::Hull hull;
		hull.deserialize(stream, version);
	}

	stream >> r.side;
}


// Surface
PX_INLINE void
serialize(physx::PxFileBuf& stream, const ApexCSG::Surface& s)
{
	stream << s.planeIndex;
	stream << s.triangleIndexStart;
	stream << s.triangleIndexStop;
	stream << s.totalTriangleArea;
}

PX_INLINE void
deserialize(physx::PxFileBuf& stream, uint32_t version, ApexCSG::Surface& s)
{
	PX_UNUSED(version);	// Initial

	stream >> s.planeIndex;
	stream >> s.triangleIndexStart;
	stream >> s.triangleIndexStop;
	stream >> s.totalTriangleArea;
}


// Triangle
PX_INLINE void
serialize(physx::PxFileBuf& stream, const ApexCSG::Triangle& t)
{
	for (uint32_t i = 0; i < 3; ++i)
	{
		stream << t.vertices[i];
	}
	stream << t.submeshIndex;
	stream << t.smoothingMask;
	stream << t.extraDataIndex;
	stream << t.normal;
	stream << t.area;
}

PX_INLINE void
deserialize(physx::PxFileBuf& stream, uint32_t version, ApexCSG::Triangle& t)
{
	for (uint32_t i = 0; i < 3; ++i)
	{
		stream >> t.vertices[i];
		if (version < Version::UsingOnlyPositionDataInVertex)
		{
			ApexCSG::Dir v;
			stream >> v;	// normal
			stream >> v;	// tangent
			stream >> v;	// binormal
			ApexCSG::UV uv;
			for (uint32_t uvN = 0; uvN < VertexFormat::MAX_UV_COUNT; ++uvN)
			{
				stream >> uv;	// UVs
			}
			ApexCSG::Color c;
			stream >> c;	// color
		}
	}
	stream >> t.submeshIndex;
	stream >> t.smoothingMask;
	stream >> t.extraDataIndex;
	stream >> t.normal;
	stream >> t.area;
}

// Interpolator
PX_INLINE void
serialize(physx::PxFileBuf& stream, const ApexCSG::Interpolator& t)
{
	t.serialize(stream);
}

PX_INLINE void
deserialize(physx::PxFileBuf& stream, uint32_t version, ApexCSG::Interpolator& t)
{
	t.deserialize(stream, version);
}

}
};	// namespace nvidia::apex

#endif

#endif // #define APEX_CSG_SERIALIZATION_H
