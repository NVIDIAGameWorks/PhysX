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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "PxPhysXConfig.h"
#if PX_USE_CLOTH_API

#include "foundation/PxStrideIterator.h"
#include "extensions/PxClothMeshQuadifier.h"

// from shared foundation
#include <PsFoundation.h>
#include <PsSort.h>
#include <Ps.h>
#include <PsMathUtils.h>
#include <PsArray.h>

using namespace physx;

struct physx::PxClothMeshQuadifierImpl
{
	PxClothMeshQuadifierImpl(const PxClothMeshDesc& desc);
	PxClothMeshDesc getDescriptor() const;

public:
	PxClothMeshDesc mDesc;
	shdfnd::Array<PxU32> mQuads;
	shdfnd::Array<PxU32> mTriangles;
};

PxClothMeshQuadifier::PxClothMeshQuadifier(const PxClothMeshDesc& desc)
: mImpl(new PxClothMeshQuadifierImpl(desc))
{
}

PxClothMeshQuadifier::~PxClothMeshQuadifier()
{
	delete mImpl;
}

PxClothMeshDesc PxClothMeshQuadifier::getDescriptor() const
{
	return mImpl->getDescriptor();
}

namespace 
{
	struct UniqueEdge
	{
		PX_FORCE_INLINE bool operator()(const UniqueEdge& e1, const UniqueEdge& e2) const
		{
			return e1 < e2;
		}

		PX_FORCE_INLINE bool operator==(const UniqueEdge& other) const
		{
			return vertex0 == other.vertex0 && vertex1 == other.vertex1;
		}
		PX_FORCE_INLINE bool operator<(const UniqueEdge& other) const
		{
			if (vertex0 != other.vertex0)
			{
				return vertex0 < other.vertex0;
			}

			return vertex1 < other.vertex1;
		}

		///////////////////////////////////////////////////////////////////////////////
		UniqueEdge() 
			: vertex0(0), vertex1(0), vertex2(0), vertex3(0xffffffff),
			maxAngle(0.0f), isQuadDiagonal(false), isUsed(false) {}

		UniqueEdge(PxU32 v0, PxU32 v1, PxU32 v2) 
		    : vertex0(PxMin(v0, v1)), vertex1(PxMax(v0, v1)), vertex2(v2), vertex3(0xffffffff),
			maxAngle(0.0f), isQuadDiagonal(false), isUsed(false) {}

		
		PxU32 vertex0, vertex1;
		PxU32 vertex2, vertex3;
		PxF32 maxAngle;
		bool isQuadDiagonal;
		bool isUsed;
	};

	struct SortHiddenEdges
	{
		SortHiddenEdges(shdfnd::Array<UniqueEdge>& uniqueEdges) : mUniqueEdges(uniqueEdges) {}

		bool operator()(PxU32 a, PxU32 b) const
		{
			return mUniqueEdges[a].maxAngle < mUniqueEdges[b].maxAngle;
		}

	private:
		SortHiddenEdges& operator=(const SortHiddenEdges&);
		shdfnd::Array<UniqueEdge>& mUniqueEdges;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	void copyIndices(const PxClothMeshDesc &desc, shdfnd::Array<PxU32> &triangles, shdfnd::Array<PxU32> &quads)
	{
		triangles.resize(desc.triangles.count*3);
		PxStrideIterator<const T> tIt = PxMakeIterator(reinterpret_cast<const T*>(desc.triangles.data), desc.triangles.stride);
		for(PxU32 i=0; i<desc.triangles.count; ++i, ++tIt)
			for(PxU32 j=0; j<3; ++j)
				triangles[i*3+j] = tIt.ptr()[j];

		quads.resize(desc.quads.count*4);
		PxStrideIterator<const T> qIt = PxMakeIterator(reinterpret_cast<const T*>(desc.quads.data), desc.quads.stride);
		for(PxU32 i=0; i<desc.quads.count; ++i, ++qIt)
			for(PxU32 j=0; j<4; ++j)
				quads[i*4+j] = qIt.ptr()[j];
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void computeUniqueEdges(shdfnd::Array<UniqueEdge> &uniqueEdges, const PxVec3* positions, const shdfnd::Array<PxU32>& triangles)
	{		
		uniqueEdges.resize(0);
		uniqueEdges.reserve(triangles.size());

		PxU32 indexMap[3][3] = { { 0, 1, 2 }, { 1, 2, 0 }, { 0, 2, 1 } };

		const PxF32 rightAngle = PxCos(shdfnd::degToRad(85.0f));

		for(PxU32 i=0; i<triangles.size(); i+=3)
		{
			UniqueEdge edges[3];
			PxF32 edgeLengths[3];
			PxF32 edgeAngles[3];

			for (PxU32 j = 0; j < 3; j++)
			{
				edges[j] = UniqueEdge(triangles[i+indexMap[j][0]], triangles[i+indexMap[j][1]], triangles[i+indexMap[j][2]]);
				edgeLengths[j] = (positions[edges[j].vertex0] - positions[edges[j].vertex1]).magnitude();
				const PxVec3 v1 = positions[edges[j].vertex2] - positions[edges[j].vertex0];
				const PxVec3 v2 = positions[edges[j].vertex2] - positions[edges[j].vertex1];
				edgeAngles[j] = PxAbs(v1.dot(v2)) / (v1.magnitude() * v2.magnitude());
			}

			// find the longest edge
			PxU32  longest = 0;
			for (PxU32 j = 1; j < 3; j++)
			{
				if (edgeLengths[j] > edgeLengths[longest])
					longest = j;
			}

			// check it's angle
			if (edgeAngles[longest] < rightAngle)
				edges[longest].isQuadDiagonal = true;
		
			for (PxU32 j = 0; j < 3; j++)
				uniqueEdges.pushBack(edges[j]);
		}

		shdfnd::sort(uniqueEdges.begin(), uniqueEdges.size(), UniqueEdge(0, 0, 0));

		PxU32 writeIndex = 0, readStart = 0, readEnd = 0;
		PxU32 numQuadEdges = 0;
		while (readEnd < uniqueEdges.size())
		{
			while (readEnd < uniqueEdges.size() && uniqueEdges[readStart] == uniqueEdges[readEnd])
				readEnd++;

			const PxU32 count = readEnd - readStart;

			UniqueEdge uniqueEdge = uniqueEdges[readStart];

			if (count == 2)
				// know the other diagonal
				uniqueEdge.vertex3 = uniqueEdges[readStart + 1].vertex2;
			else
				uniqueEdge.isQuadDiagonal = false;

			for (PxU32 i = 1; i < count; i++)
				uniqueEdge.isQuadDiagonal &= uniqueEdges[readStart + i].isQuadDiagonal;

			numQuadEdges += uniqueEdge.isQuadDiagonal ? 1 : 0;

			uniqueEdges[writeIndex] = uniqueEdge;

			writeIndex++;
			readStart = readEnd;
		}

		uniqueEdges.resize(writeIndex, UniqueEdge(0, 0, 0));
	}

	///////////////////////////////////////////////////////////////////////////////
	PxU32 findUniqueEdge(const shdfnd::Array<UniqueEdge> &uniqueEdges, PxU32 index1, PxU32 index2)
	{
		UniqueEdge searchFor(index1, index2, 0);

		PxU32 curMin = 0;
		PxU32 curMax = uniqueEdges.size();
		while (curMax > curMin)
		{
			PxU32 middle = (curMin + curMax) >> 1;

			const UniqueEdge& probe = uniqueEdges[middle];
			if (probe < searchFor)
				curMin = middle + 1;
			else
				curMax = middle;		
		}

		return curMin;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void refineUniqueEdges(shdfnd::Array<UniqueEdge> &uniqueEdges, const PxVec3* positions)
	{
		shdfnd::Array<PxU32> hideEdges;
		hideEdges.reserve(uniqueEdges.size());

		for (PxU32 i = 0; i < uniqueEdges.size(); i++)
		{
			UniqueEdge& uniqueEdge = uniqueEdges[i];
			uniqueEdge.maxAngle = 0.0f;
			uniqueEdge.isQuadDiagonal = false; // just to be sure

			if (uniqueEdge.vertex3 != 0xffffffff)
			{
				PxU32 indices[4] = { uniqueEdge.vertex0, uniqueEdge.vertex2, uniqueEdge.vertex1, uniqueEdge.vertex3 };

				// compute max angle of the quad
				for (PxU32 j = 0; j < 4; j++)
				{
					PxVec3 e0 = positions[indices[ j + 0   ]] - positions[indices[(j + 1) % 4]];
					PxVec3 e1 = positions[indices[(j + 1) % 4]] - positions[indices[(j + 2) % 4]];

					PxF32 cosAngle = PxAbs(e0.dot(e1)) / (e0.magnitude() * e1.magnitude());
					uniqueEdge.maxAngle = PxMax(uniqueEdge.maxAngle, cosAngle);
				}

				hideEdges.pushBack(i);
			}
		}

		shdfnd::sort(hideEdges.begin(), hideEdges.size(), SortHiddenEdges(uniqueEdges));

		const PxF32 maxAngle = PxSin(shdfnd::degToRad(60.0f));

		PxU32 numHiddenEdges = 0;

		for (PxU32 i = 0; i < hideEdges.size(); i++)
		{
			UniqueEdge& uniqueEdge = uniqueEdges[hideEdges[i]];

			// find some stop criterion
			if (uniqueEdge.maxAngle > maxAngle)
				break;
		
			// check if all four adjacent edges are still visible?
			PxU32 indices[5] = { uniqueEdge.vertex0, uniqueEdge.vertex2, uniqueEdge.vertex1, uniqueEdge.vertex3, uniqueEdge.vertex0 };

			PxU32 numVisible = 0;
			for (PxU32 j = 0; j < 4; j++)
			{
				const PxU32 edgeIndex = findUniqueEdge(uniqueEdges, indices[j], indices[j + 1]);
				PX_ASSERT(edgeIndex < uniqueEdges.size());
	
				numVisible += uniqueEdges[edgeIndex].isQuadDiagonal ? 0 : 1;
			}

			if (numVisible == 4)
			{
				uniqueEdge.isQuadDiagonal = true;
				numHiddenEdges++;
			}
		}
	}


	// calculate the inclusive prefix sum, equivalent of std::partial_sum
	template <typename T>
	void prefixSum(const T* first, const T* last, T* dest)
	{
		if (first != last)
		{	
			*(dest++) = *(first++);
			for (; first != last; ++first, ++dest)
				*dest = *(dest-1) + *first;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void quadifyTriangles(const shdfnd::Array<UniqueEdge> &uniqueEdges, shdfnd::Array<PxU32>& triangles, shdfnd::Array<PxU32> &quads)
	{
		shdfnd::Array<PxU32> valency(uniqueEdges.size()+1, 0); // edge valency
		shdfnd::Array<PxU32> adjacencies; // adjacency from unique edge to triangles
		PxU32 numTriangles = triangles.size() / 3;

		// compute edge valency w.r.t triangles
		for(PxU32 i=0; i<numTriangles; ++i)
		{
			for (PxU32 j=0; j < 3; j++)
			{
				PxU32 uniqueEdgeIndex = findUniqueEdge(uniqueEdges, triangles[i*3+j], triangles[i*3+(j+1)%3]);
				++valency[uniqueEdgeIndex];
			}
		}

		// compute adjacency from each edge to triangle, the value also encodes which side of the triangle this edge belongs to
		prefixSum(valency.begin(), valency.end(), valency.begin());
		adjacencies.resize(valency.back());
		for(PxU32 i=0; i<numTriangles; ++i)
		{
			for (PxU32 j=0; j < 3; j++)
			{
				PxU32 uniqueEdgeIndex = findUniqueEdge(uniqueEdges, triangles[i*3+j], triangles[i*3+(j+1)%3]);
				adjacencies[--valency[uniqueEdgeIndex]] = i*3+j;
			}
		}

		// now go through unique edges that are identified as diagonal, and build a quad out of two adjacent triangles
		shdfnd::Array<PxU32> mark(numTriangles, 0);
		for (PxU32 i = 0; i < uniqueEdges.size(); i++)
		{
			const UniqueEdge& edge = uniqueEdges[i];
			if (edge.isQuadDiagonal)
			{
				PxU32 vi = valency[i];
				if ((valency[i+1]-vi) != 2)
					continue; // we do not quadify around non-manifold edges

				PxU32 adj0 = adjacencies[vi], adj1 = adjacencies[vi+1];
				PxU32 tid0 = adj0 / 3, tid1 = adj1 / 3;
				PxU32 eid0 = adj0 % 3, eid1 = adj1 % 3;

				quads.pushBack(triangles[tid0 * 3 + eid0]);
				quads.pushBack(triangles[tid1 * 3 + (eid1+2)%3]);
				quads.pushBack(triangles[tid0 * 3 + (eid0+1)%3]);
				quads.pushBack(triangles[tid0 * 3 + (eid0+2)%3]);

				mark[tid0] = 1;
				mark[tid1] = 1;
#if 0 // PX_DEBUG
				printf("Deleting %d, %d, %d - %d, %d, %d, creating %d, %d, %d, %d\n",
					triangles[tid0*3],triangles[tid0*3+1],triangles[tid0*3+2],
					triangles[tid1*3],triangles[tid1*3+1],triangles[tid1*3+2],
					v0,v3,v1,v2);
#endif
			}
		}

		// add remaining triangles that are not marked as already quadified
		shdfnd::Array<PxU32> oldTriangles = triangles;
		triangles.resize(0);
		for (PxU32 i = 0; i < numTriangles; i++)
		{
			if (mark[i]) continue;

			triangles.pushBack(oldTriangles[i*3]);
			triangles.pushBack(oldTriangles[i*3+1]);
			triangles.pushBack(oldTriangles[i*3+2]);
		}
	}

} // namespace 


///////////////////////////////////////////////////////////////////////////////
PxClothMeshQuadifierImpl::PxClothMeshQuadifierImpl(const PxClothMeshDesc &desc)
	:mDesc(desc)
{
	shdfnd::Array<PxVec3> particles(desc.points.count);
	PxStrideIterator<const PxVec3> pIt(reinterpret_cast<const PxVec3*>(desc.points.data), desc.points.stride);
	for(PxU32 i=0; i<desc.points.count; ++i)
		particles[i] = *pIt++;

	// copy triangle indices
	if(desc.flags & PxMeshFlag::e16_BIT_INDICES)
		copyIndices<PxU16>(desc, mTriangles, mQuads);
	else
		copyIndices<PxU32>(desc, mTriangles, mQuads);

	shdfnd::Array<UniqueEdge> uniqueEdges;

	computeUniqueEdges(uniqueEdges, particles.begin(), mTriangles);

	refineUniqueEdges(uniqueEdges, particles.begin());

//	printf("before %d triangles, %d quads\n", mTriangles.size()/3, mQuads.size()/4);
	quadifyTriangles(uniqueEdges, mTriangles, mQuads);

//	printf("after %d triangles, %d quads\n", mTriangles.size()/3, mQuads.size()/4);
}

///////////////////////////////////////////////////////////////////////////////
PxClothMeshDesc 
PxClothMeshQuadifierImpl::getDescriptor() const
{
	// copy points and other data
	PxClothMeshDesc desc = mDesc;

	// for now use only 32 bit for temporary indices out of quadifier
	desc.flags &= ~PxMeshFlag::e16_BIT_INDICES;

	desc.triangles.count = mTriangles.size() / 3;
	desc.triangles.data = mTriangles.begin();
	desc.triangles.stride = 3 * sizeof(PxU32);

	desc.quads.count = mQuads.size() / 4;
	desc.quads.data = mQuads.begin();
	desc.quads.stride = 4 * sizeof(PxU32);

	PX_ASSERT(desc.isValid());

	return desc;
}
#endif //PX_USE_CLOTH_API


