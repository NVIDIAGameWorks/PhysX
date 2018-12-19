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

#include "foundation/PxVec4.h"
#include "foundation/PxMemory.h"
#include "foundation/PxStrideIterator.h"
#include "extensions/PxClothTetherCooker.h"

// from shared foundation
#include <PsFoundation.h>
#include <PsSort.h>
#include <Ps.h>
#include <PsMathUtils.h>
#include "PsArray.h"

using namespace physx;


namespace
{
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

	template <typename T>
	void gatherAdjacencies(shdfnd::Array<PxU32>& valency, shdfnd::Array<PxU32>& adjacencies, 
		const PxBoundedData& triangles, const PxBoundedData& quads)
	{
		// count number of edges per vertex
		PxStrideIterator<const T> tIt, qIt;
		tIt = PxMakeIterator(reinterpret_cast<const T*>(triangles.data), triangles.stride);
		for(PxU32 i=0; i<triangles.count; ++i, ++tIt, ++qIt)
		{
			for(PxU32 j=0; j<3; ++j)
				valency[tIt.ptr()[j]] += 2;
		}
		qIt = PxMakeIterator(reinterpret_cast<const T*>(quads.data), quads.stride);
		for(PxU32 i=0; i<quads.count; ++i, ++tIt, ++qIt)
		{
			for(PxU32 j=0; j<4; ++j)
				valency[qIt.ptr()[j]] += 2;
		}

		prefixSum(valency.begin(), valency.end(), valency.begin());
		adjacencies.resize(valency.back());

		// gather adjacent vertices
		tIt = PxMakeIterator(reinterpret_cast<const T*>(triangles.data), triangles.stride);
		for(PxU32 i=0; i<triangles.count; ++i, ++tIt)
		{
			for(PxU32 j=0; j<3; ++j)
			{
				adjacencies[--valency[tIt.ptr()[j]]] = tIt.ptr()[(j+1)%3];
				adjacencies[--valency[tIt.ptr()[j]]] = tIt.ptr()[(j+2)%3];
			}
		}
		qIt = PxMakeIterator(reinterpret_cast<const T*>(quads.data), quads.stride);
		for(PxU32 i=0; i<quads.count; ++i, ++qIt)
		{
			for(PxU32 j=0; j<4; ++j)
			{
				adjacencies[--valency[qIt.ptr()[j]]] = qIt.ptr()[(j+1)%4];
				adjacencies[--valency[qIt.ptr()[j]]] = qIt.ptr()[(j+3)%4];
			}
		}
	}

	template <typename T>
	void gatherIndices(shdfnd::Array<PxU32>& indices, 
		const PxBoundedData& triangles, const PxBoundedData& quads)
	{
		PxStrideIterator<const T> tIt, qIt;

		indices.reserve(triangles.count * 3 + quads.count * 6);
	
		tIt = PxMakeIterator(reinterpret_cast<const T*>(triangles.data), triangles.stride);
		for(PxU32 i=0; i<triangles.count; ++i, ++tIt)
		{
			indices.pushBack(tIt.ptr()[0]);
			indices.pushBack(tIt.ptr()[1]);
			indices.pushBack(tIt.ptr()[2]);
		}
		qIt = PxMakeIterator(reinterpret_cast<const T*>(quads.data), quads.stride);
		for(PxU32 i=0; i<quads.count; ++i, ++qIt)
		{
			indices.pushBack(qIt.ptr()[0]);
			indices.pushBack(qIt.ptr()[1]);
			indices.pushBack(qIt.ptr()[2]);
			indices.pushBack(qIt.ptr()[0]);
			indices.pushBack(qIt.ptr()[2]);
			indices.pushBack(qIt.ptr()[3]);
		}
	}

	// maintain heap status after elements have been pushed (heapify)
	template<typename T>
	void pushHeap(shdfnd::Array<T> &heap, const T &value)
	{
		heap.pushBack(value);
		T* begin = heap.begin();
		T* end = heap.end();

		if (end <= begin)
			return;
	
		PxU32 current = PxU32(end - begin) - 1;
		while (current > 0)
		{
			const PxU32 parent = (current - 1) / 2;
			if (!(begin[parent] < begin[current]))
				break;

			shdfnd::swap(begin[parent], begin[current]);
			current = parent;
		}
	}

	// pop one element from the heap
	template<typename T>
	T popHeap(shdfnd::Array<T> &heap)
	{
		T* begin = heap.begin();
		T* end = heap.end();

		shdfnd::swap(begin[0], end[-1]); // exchange elements

		// shift down
		end--;

		PxU32 current = 0;
		while (begin + (current * 2 + 1) < end)
		{
			PxU32 child = current * 2 + 1;
			if (begin + child + 1 < end && begin[child] < begin[child + 1])
				++child;

			if (!(begin[current] < begin[child]))
				break;

			shdfnd::swap(begin[current], begin[child]);
			current = child;
		}

		return heap.popBack();
	}

	// ---------------------------------------------------------------------------------------
	struct VertexDistanceCount
	{
		VertexDistanceCount(int vert, float dist, int count) 
			: vertNr(vert), distance(dist), edgeCount(count) {}

		int vertNr;
		float distance;
		int edgeCount;
		bool operator < (const VertexDistanceCount& v) const
		{
			return v.distance < distance;
		}
	};

	// ---------------------------------------------------------------------------------------
	struct PathIntersection
	{
		PxU32 vertOrTriangle;
		PxU32 index; // vertex id or triangle edge id
		float s; // only used for edge intersection
		float distance; // computed distance

	public:
		PathIntersection() {}

		PathIntersection(PxU32 vort, PxU32 in_index, float in_distance, float in_s = 0.0f) 
			: vertOrTriangle(vort), index(in_index), s(in_s), distance(in_distance)
		{
		}
	};

	//---------------------------------------------------------------------------------------
	struct VertTriangle
	{
		VertTriangle(int vert, int triangle)
			: mVertIndex(vert), mTriangleIndex(triangle)
		{
		}

		bool operator<(const VertTriangle &vt) const
		{
			return mVertIndex == vt.mVertIndex ?
				mTriangleIndex < vt.mTriangleIndex : mVertIndex < vt.mVertIndex;
		}

		int mVertIndex;
		int mTriangleIndex;
	};

	// ---------------------------------------------------------------------------------------
	struct MeshEdge
	{
		MeshEdge(int v0, int v1, int halfEdgeIndex)
			: mFromVertIndex(v0), mToVertIndex(v1), mHalfEdgeIndex(halfEdgeIndex)
		{
			if(mFromVertIndex > mToVertIndex)
				shdfnd::swap(mFromVertIndex, mToVertIndex);
		}

		bool operator<(const MeshEdge& e) const
		{
			return mFromVertIndex == e.mFromVertIndex ?
				mToVertIndex < e.mToVertIndex : mFromVertIndex < e.mFromVertIndex;
		}

		bool operator==(const MeshEdge& e) const
		{
			return mFromVertIndex == e.mFromVertIndex 
				&& mToVertIndex == e.mToVertIndex;
		}

		int mFromVertIndex, mToVertIndex; 
		int mHalfEdgeIndex; 
	};

	// check if the edge is following triangle order or not
	bool checkEdgeOrientation(const MeshEdge &e, const shdfnd::Array<PxU32> &indices)
	{		
		int offset0 = e.mHalfEdgeIndex % 3;
		int offset1 = (offset0 < 2) ? 1 : -2;

		int v0 = int(indices[PxU32(e.mHalfEdgeIndex)]);
		int v1 = int(indices[PxU32(e.mHalfEdgeIndex + offset1)]);

		if ((e.mFromVertIndex == v0) && (e.mToVertIndex == v1))
			return true;

		return false;
	}

	// check if two index pairs represent same edge regardless of order.
	inline bool checkEdge(int ei0, int ei1, int ej0, int ej1)
	{
		return ( (ei0 == ej0) && (ei1 == ej1) ) ||
			( (ei0 == ej1) && (ei1 == ej0) );
	}

	// compute ray edge intersection
	bool intersectRayEdge(const PxVec3 &O, const PxVec3 &D, const PxVec3 &A, const PxVec3 &B, float &s, float &t)
	{
		// point on edge P = A + s * AB
		// point on ray R = o + t * d
		// for this two points to intersect, we have
		// |AB -d| | s t | = o - A
		const float eps = 1e-4;

		PxVec3 OA = O - A;
		PxVec3 AB = B - A;

		float a = AB.dot(AB), b = -AB.dot(D);
		float c = b, d = D.dot(D);

		float e = AB.dot(OA);
		float f = -D.dot(OA);

		float det = a * d - b * c;
		if (PxAbs(det) < eps) // coplanar case
			return false;

		float inv_det = 1.0f / det;

		s = (d * inv_det) * e + (-b * inv_det) * f;
		t = (-c * inv_det) * e + (a * inv_det) * f;

		return true;
	}
}


struct physx::PxClothGeodesicTetherCookerImpl
{

	PxClothGeodesicTetherCookerImpl(const PxClothMeshDesc& desc);

	PxU32	getCookerStatus() const;
	PxU32	getNbTethersPerParticle() const;
	void	getTetherData(PxU32* userTetherAnchors, PxReal* userTetherLengths) const;

public:
	// input
	const PxClothMeshDesc&  mDesc;

	// internal variables
	PxU32					mNumParticles;
	shdfnd::Array<PxVec3>   mVertices;
	shdfnd::Array<PxU32>	mIndices;
	shdfnd::Array<PxU8>     mAttached;
	shdfnd::Array<PxU32>	mFirstVertTriAdj;
	shdfnd::Array<PxU32>	mVertTriAdjs;
	shdfnd::Array<PxU32>	mTriNeighbors; // needs changing for non-manifold support

	// error status
	PxU32					mCookerStatus;

	// output
	shdfnd::Array<PxU32>	mTetherAnchors;
	shdfnd::Array<PxReal>	mTetherLengths;

protected:
	void	createTetherData(const PxClothMeshDesc &desc);
	int		computeVertexIntersection(PxU32 parent, PxU32 src, PathIntersection &path);
	int		computeEdgeIntersection(PxU32 parent, PxU32 edge, float in_s, PathIntersection &path);
	float	computeGeodesicDistance(PxU32 i, PxU32 parent, int &errorCode);
	PxU32	findTriNeighbors();
	void	findVertTriNeighbors();

private:
	PxClothGeodesicTetherCookerImpl& operator=(const PxClothGeodesicTetherCookerImpl&);
};

PxClothGeodesicTetherCooker::PxClothGeodesicTetherCooker(const PxClothMeshDesc& desc)
: mImpl(new PxClothGeodesicTetherCookerImpl(desc))
{
}

PxClothGeodesicTetherCooker::~PxClothGeodesicTetherCooker()
{
	delete mImpl;
}

PxU32 PxClothGeodesicTetherCooker::getCookerStatus() const
{
	return mImpl->getCookerStatus();
}

PxU32 PxClothGeodesicTetherCooker::getNbTethersPerParticle() const
{
	return mImpl->getNbTethersPerParticle();
}

void PxClothGeodesicTetherCooker::getTetherData(PxU32* userTetherAnchors, PxReal* userTetherLengths) const
{
	mImpl->getTetherData(userTetherAnchors, userTetherLengths);
}

///////////////////////////////////////////////////////////////////////////////
PxClothGeodesicTetherCookerImpl::PxClothGeodesicTetherCookerImpl(const PxClothMeshDesc &desc)
	:mDesc(desc),
	mCookerStatus(0)
{
	createTetherData(desc);
}

///////////////////////////////////////////////////////////////////////////////
void PxClothGeodesicTetherCookerImpl::createTetherData(const PxClothMeshDesc &desc)
{
	mNumParticles = desc.points.count;
	
	if (!desc.invMasses.data)
		return;

	// assemble points
	mVertices.resize(mNumParticles);
	mAttached.resize(mNumParticles);
	PxStrideIterator<const PxVec3> pIt(reinterpret_cast<const PxVec3*>(desc.points.data), desc.points.stride);
	PxStrideIterator<const PxReal> wIt(reinterpret_cast<const PxReal*>(desc.invMasses.data), desc.invMasses.stride);
	for(PxU32 i=0; i<mNumParticles; ++i)
	{
		mVertices[i] = *pIt++;
		mAttached[i] = PxU8(wIt.ptr() ? (*wIt++ == 0.0f) : 0);
	}

	// build triangle indices
	if(desc.flags & PxMeshFlag::e16_BIT_INDICES)
		gatherIndices<PxU16>(mIndices, desc.triangles, desc.quads);
	else
		gatherIndices<PxU32>(mIndices, desc.triangles, desc.quads);

	// build vertex-triangle adjacencies
	findVertTriNeighbors();

	// build triangle-triangle adjacencies
	mCookerStatus = findTriNeighbors();
	if (mCookerStatus != 0)
		return;

	// build adjacent vertex list
	shdfnd::Array<PxU32> valency(mNumParticles+1, 0);
	shdfnd::Array<PxU32> adjacencies;
	if(desc.flags & PxMeshFlag::e16_BIT_INDICES)
		gatherAdjacencies<PxU16>(valency, adjacencies, desc.triangles, desc.quads);
	else
		gatherAdjacencies<PxU32>(valency, adjacencies, desc.triangles, desc.quads);

	// build unique neighbors from adjacencies
	shdfnd::Array<PxU32> mark(valency.size(), 0);
	shdfnd::Array<PxU32> neighbors; neighbors.reserve(adjacencies.size());
	for(PxU32 i=1, j=0; i<valency.size(); ++i)
	{
		for(; j<valency[i]; ++j)
		{
			PxU32 k = adjacencies[j];
			if(mark[k] != i)
			{
				mark[k] = i;
				neighbors.pushBack(k);
			}
		}
		valency[i] = neighbors.size();
	}

	// create islands of attachment points
	shdfnd::Array<PxU32> vertexIsland(mNumParticles);
	shdfnd::Array<VertexDistanceCount> vertexIslandHeap;

	// put all the attachments in heap
	for (PxU32 i = 0; i < mNumParticles; ++i)
	{
		// we put each attached point with large distance so that 
		// we can prioritize things that are added during mesh traversal.
		vertexIsland[i] = PxU32(-1);
		if (mAttached[i])
			vertexIslandHeap.pushBack(VertexDistanceCount(int(i), FLT_MAX, 0));
	}
	PxU32 attachedCnt = vertexIslandHeap.size();

	// no attached vertices
	if (vertexIslandHeap.empty())
		return;

	// identify islands of attached vertices
	shdfnd::Array<PxU32> islandIndices;
	shdfnd::Array<PxU32> islandFirst;
	PxU32 islandCnt = 0;
	PxU32 islandIndexCnt = 0;

	islandIndices.reserve(attachedCnt);
	islandFirst.reserve(attachedCnt+1);

	// while the island heap is not empty
	while (!vertexIslandHeap.empty())
	{
		// pop vi from heap
		VertexDistanceCount vi = popHeap(vertexIslandHeap);

		// new cluster
		if (vertexIsland[PxU32(vi.vertNr)] == PxU32(-1))
		{
			islandFirst.pushBack(islandIndexCnt++);
			vertexIsland[PxU32(vi.vertNr)] = islandCnt++;
			vi.distance = 0;
			islandIndices.pushBack(PxU32(vi.vertNr));
		}
		
		// for each adjacent vj that's not visited
		const PxU32 begin = PxU32(valency[PxU32(vi.vertNr)]);
		const PxU32 end = PxU32(valency[PxU32(vi.vertNr + 1)]);
		for (PxU32 j = begin; j < end; ++j)
		{
			const PxU32 vj = neighbors[j];

			// do not expand unattached vertices
			if (!mAttached[vj])
				continue; 

			// already visited
			if (vertexIsland[vj] != PxU32(-1))
				continue;

			islandIndices.pushBack(vj);
			islandIndexCnt++;
			vertexIsland[vj] = vertexIsland[PxU32(vi.vertNr)];
			pushHeap(vertexIslandHeap, VertexDistanceCount(int(vj), vi.distance + 1.0f, 0));
		}
	}

	islandFirst.pushBack(islandIndexCnt);

	PX_ASSERT(islandCnt == (islandFirst.size() - 1));

	/////////////////////////////////////////////////////////
	PxU32 bufferSize = mNumParticles * islandCnt;
	PX_ASSERT(bufferSize > 0);

	shdfnd::Array<float> vertexDistanceBuffer(bufferSize, PX_MAX_F32);
	shdfnd::Array<PxU32> vertexParentBuffer(bufferSize, 0);
	shdfnd::Array<VertexDistanceCount> vertexHeap;

	// now process each island 
	for (PxU32 i = 0; i < islandCnt; i++)
	{
		vertexHeap.clear();
		float* vertexDistance = &vertexDistanceBuffer[0] + (i * mNumParticles);
		PxU32* vertexParent = &vertexParentBuffer[0] + (i * mNumParticles);

		// initialize parent and distance
		for (PxU32 j = 0; j < mNumParticles; ++j)
		{
			vertexParent[j] = j;
			vertexDistance[j] = PX_MAX_F32;
		}

		// put all the attached vertices in this island to heap
		const PxU32 beginIsland = islandFirst[i];
		const PxU32 endIsland = islandFirst[i+1];
		for (PxU32 j = beginIsland; j < endIsland; j++)
		{
			PxU32 vj = islandIndices[j];
			vertexDistance[vj] = 0.0f;
			vertexHeap.pushBack(VertexDistanceCount(int(vj), 0.0f, 0));
		}

		// no attached vertices in this island (error?)
		PX_ASSERT(vertexHeap.empty() == false);
		if (vertexHeap.empty())
			continue;

		// while heap is not empty
		while (!vertexHeap.empty())
		{
			// pop vi from heap
			VertexDistanceCount vi = popHeap(vertexHeap);

			// obsolete entry ( we already found better distance)
			if (vi.distance > vertexDistance[vi.vertNr])
				continue;

			// for each adjacent vj that's not visited
			const PxI32 begin = PxI32(valency[PxU32(vi.vertNr)]);
			const PxI32 end = PxI32(valency[PxU32(vi.vertNr + 1)]);
			for (PxI32 j = begin; j < end; ++j)
			{
				const PxI32 vj = PxI32(neighbors[PxU32(j)]);
				PxVec3 edge = mVertices[PxU32(vj)] - mVertices[PxU32(vi.vertNr)];
				const PxF32 edgeLength = edge.magnitude();
				float newDistance = vi.distance + edgeLength;

				if (newDistance < vertexDistance[vj])
				{
					vertexDistance[vj] = newDistance;
					vertexParent[vj] = vertexParent[vi.vertNr];
	
					pushHeap(vertexHeap, VertexDistanceCount(vj, newDistance, 0));
				}
			}
		}
	}

	const PxU32 maxTethersPerParticle = 4; // max tethers
	const PxU32 nbTethersPerParticle = (islandCnt > maxTethersPerParticle) ? maxTethersPerParticle : islandCnt;

	PxU32 nbTethers = nbTethersPerParticle * mNumParticles;
	mTetherAnchors.resize(nbTethers);
	mTetherLengths.resize(nbTethers);

	// now process the parent and distance and add to fibers
	for (PxU32 i = 0; i < mNumParticles; i++)
	{
		// we use the heap to sort out N-closest island
		vertexHeap.clear();
		for (PxU32 j = 0; j < islandCnt; j++)
		{
			int parent = int(vertexParentBuffer[j * mNumParticles + i]);
			float edgeDistance = vertexDistanceBuffer[j * mNumParticles + i];
			pushHeap(vertexHeap, VertexDistanceCount(parent, edgeDistance, 0));
		}

		// take out N-closest island from the heap
		for (PxU32 j = 0; j < nbTethersPerParticle; j++)
		{
			VertexDistanceCount vi = popHeap(vertexHeap);
			PxU32 parent = PxU32(vi.vertNr);
			float distance = 0.0f;
		
			if (parent != i) 
			{
				float euclideanDistance = (mVertices[i] - mVertices[parent]).magnitude();
				float dijkstraDistance = vi.distance;
				int errorCode = 0;
				float geodesicDistance = computeGeodesicDistance(i,parent, errorCode);
				if (errorCode < 0)
					geodesicDistance = dijkstraDistance;
				distance = PxMax(euclideanDistance, geodesicDistance);
			}

			PxU32 tetherLoc = j * mNumParticles + i;
			mTetherAnchors[ tetherLoc ] = parent;
			mTetherLengths[ tetherLoc ] = distance;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
PxU32 PxClothGeodesicTetherCookerImpl::getCookerStatus() const
{
	return mCookerStatus;
}

///////////////////////////////////////////////////////////////////////////////
PxU32 PxClothGeodesicTetherCookerImpl::getNbTethersPerParticle() const
{
	return mTetherAnchors.size() / mNumParticles;
}

///////////////////////////////////////////////////////////////////////////////
void  
PxClothGeodesicTetherCookerImpl::getTetherData(PxU32* userTetherAnchors, PxReal* userTetherLengths) const
{
	PxMemCopy(userTetherAnchors, mTetherAnchors.begin(), mTetherAnchors.size() * sizeof(PxU32));
	PxMemCopy(userTetherLengths, mTetherLengths.begin(), mTetherLengths.size() * sizeof(PxReal));
}

///////////////////////////////////////////////////////////////////////////////
// find triangle-triangle adjacency (return non-zero if there is an error)
PxU32 PxClothGeodesicTetherCookerImpl::findTriNeighbors()
{
	shdfnd::Array<MeshEdge> edges;

	mTriNeighbors.resize(mIndices.size(), PxU32(-1));

	// assemble all edges
	PxU32 numTriangles = mIndices.size() / 3;
	for (PxU32 i = 0; i < numTriangles; ++i)
	{
		PxU32 i0 = mIndices[3 * i];
		PxU32 i1 = mIndices[3 * i + 1];
		PxU32 i2 = mIndices[3 * i + 2];
		edges.pushBack(MeshEdge(int(i0), int(i1), int(3*i)));
		edges.pushBack(MeshEdge(int(i1), int(i2), int(3*i+1)));
		edges.pushBack(MeshEdge(int(i2), int(i0), int(3*i+2)));
	}

	shdfnd::sort(edges.begin(), edges.size());

	int numEdges = int(edges.size());
	for(int i=0; i < numEdges; )
	{
		const MeshEdge& e0 = edges[PxU32(i)];
		bool orientation0 = checkEdgeOrientation(e0, mIndices);

		int j = i;
		while(++i < numEdges && edges[PxU32(i)] == e0)
			;

		if(i - j > 2)
			return 1; // non-manifold
	
		while(++j < i)
		{
			const MeshEdge& e1 = edges[PxU32(j)];
			bool orientation1 = checkEdgeOrientation(e1, mIndices);
			mTriNeighbors[PxU32(e0.mHalfEdgeIndex)] = PxU32(e1.mHalfEdgeIndex/3);
			mTriNeighbors[PxU32(e1.mHalfEdgeIndex)] = PxU32(e0.mHalfEdgeIndex/3);

			if (orientation0 == orientation1)
				return 2; // bad winding
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// find vertex triangle adjacency information
void PxClothGeodesicTetherCookerImpl::findVertTriNeighbors()
{
	shdfnd::Array<VertTriangle> vertTriangles;
	vertTriangles.reserve(mIndices.size());

	int numTriangles = int(mIndices.size() / 3);
	for (int i = 0; i < numTriangles; ++i)
	{
		vertTriangles.pushBack(VertTriangle(int(mIndices[PxU32(3*i)]), i));
		vertTriangles.pushBack(VertTriangle(int(mIndices[PxU32(3*i+1)]), i));
		vertTriangles.pushBack(VertTriangle(int(mIndices[PxU32(3*i+2)]), i));
	}

	shdfnd::sort(vertTriangles.begin(), vertTriangles.size(), shdfnd::Less<VertTriangle>());
	mFirstVertTriAdj.resize(mNumParticles);
	mVertTriAdjs.reserve(mIndices.size());

	for (PxU32 i = 0; i < PxU32(vertTriangles.size()); )
	{
		int v = vertTriangles[i].mVertIndex;

		mFirstVertTriAdj[PxU32(v)] = i;

		while ((i < mIndices.size()) && (vertTriangles[i].mVertIndex == v))
		{
			int t = vertTriangles[i].mTriangleIndex;
			mVertTriAdjs.pushBack(PxU32(t));
			i++;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// compute intersection of a ray from a source vertex in direction toward parent
int PxClothGeodesicTetherCookerImpl::computeVertexIntersection(PxU32 parent, PxU32 src, PathIntersection &path)
{
	if (src == parent)
	{
		path = PathIntersection(true, src, 0.0);
		return 0;
	}

	float maxdot = -1.0f;
	int closestVert = -1;

	// gradient is toward the parent vertex
	PxVec3 g = (mVertices[parent] - mVertices[src]).getNormalized();

	// for every triangle incident on this vertex, we intersect against opposite edge of the triangle
	PxU32 sfirst = mFirstVertTriAdj[src];
	PxU32 slast = (src < (PxU32(mNumParticles-1))) ? mFirstVertTriAdj[src+1] : PxU32(mVertTriAdjs.size());
	for (PxU32 adj = sfirst; adj < slast; adj++)
	{
		PxU32 tid = mVertTriAdjs[adj];
		
		PxU32 i0 = mIndices[tid*3];
		PxU32 i1 = mIndices[tid*3+1];
		PxU32 i2 = mIndices[tid*3+2];

		int eid = 0;
		if (i0 == src) eid = 1;
		else if (i1 == src) eid = 2;
		else if (i2 == src) eid = 0;
		else continue; // error

		// reshuffle so that src is located at i2
		i0 = mIndices[tid*3 + eid];
		i1 = mIndices[tid*3 + (eid+1)%3];
		i2 = src;

		PxVec3 p0 = mVertices[i0];
		PxVec3 p1 = mVertices[i1];
		PxVec3 p2 = mVertices[i2];

		// check if we hit source immediately from this triangle
		if (i0 == parent)
		{
			path = PathIntersection(true, parent, (p0 - p2).magnitude());
			return 1;
		}

		if (i1 == parent)
		{
			path = PathIntersection(true, parent, (p1 - p2).magnitude());
			return 1;
		}

		// ray direction is the gradient projected on the plane of this triangle
		PxVec3 n = ((p0 - p2).cross(p1 - p2)).getNormalized();
		PxVec3 d = (g - g.dot(n) * n).getNormalized();

		// find intersection of ray (p2, d) against the edge (p0,p1)
		float s, t;
		bool result = intersectRayEdge(p2, d, p0, p1, s, t);
		if (result == false)
			continue;

		// t should be positive, otherwise we just hit the triangle in opposite direction, so ignore
		const float eps = 1e-5;
		if (t > -eps)
		{		
			PxVec3 ip; // intersection point
			if (( s > -eps ) && (s < (1.0f + eps)))
			{				
				// if intersection point is too close to each vertex, we record a vertex intersection
				if ( ( s < eps) || (s > (1.0f-eps)))
				{
					path.vertOrTriangle = true;
					path.index = (s < eps) ? i0 : i1;
					path.distance = (p2 - mVertices[path.index]).magnitude();				
				}
				else // found an edge instersection
				{
					ip = p0 + s * (p1 - p0);
					path = PathIntersection(false, tid*3 + eid, (p2 - ip).magnitude(), s);					
				}
				return 1;
			}
		}

		// for fall back (see below)
		PxVec3 d0 = (p0 - p2).getNormalized();
		PxVec3 d1 = (p1 - p2).getNormalized();
		float d0dotg = d0.dot(d);
		float d1dotg = d1.dot(d);

		if (d0dotg > maxdot)
		{
			closestVert = int(i0);
			maxdot = d0dotg;
		}
		if (d1dotg > maxdot)
		{
			closestVert = int(i1);
			maxdot = d1dotg;
		}
	} // end for (PxU32 adj = sfirst...

	// Fall back to use greedy (Dijkstra-like) path selection. 
	// This happens as triangles are curved and we may not find intersection on any triangle.
	// In this case, we choose a vertex closest to the gradient direction.
	if (closestVert > 0)
	{
		path = PathIntersection(true, PxU32(closestVert), (mVertices[src] - mVertices[PxU32(closestVert)]).magnitude());
		return 1;
	}

	// Error, (possibly dangling vertex)
	return -1;
}

///////////////////////////////////////////////////////////////////////////////
// compute intersection of a ray from a source vertex in direction toward parent
int PxClothGeodesicTetherCookerImpl::computeEdgeIntersection(PxU32 parent, PxU32 edge, float in_s, PathIntersection &path)
{
	int tid = int(edge / 3);
	int eid = int(edge % 3);

	PxU32 e0 = mIndices[PxU32(tid*3 + eid)];
	PxU32 e1 = mIndices[PxU32(tid*3 + (eid+1)%3)];

	PxVec3 v0 = mVertices[e0];
	PxVec3 v1 = mVertices[e1];

	PxVec3 v = v0 + in_s * (v1 - v0);
	PxVec3 g = mVertices[parent] - v;

	PxU32 triNbr = mTriNeighbors[edge];

	if (triNbr == PxU32(-1)) // boundary edge
	{
		float dir = g.dot(v1-v0);
		PxU32 vid = (dir > 0) ? e1 : e0;
		path = PathIntersection(true, vid, (mVertices[vid] - v).magnitude());
		return 1;
	}

	PxU32 i0 = mIndices[triNbr*3];
	PxU32 i1 = mIndices[triNbr*3+1];
	PxU32 i2 = mIndices[triNbr*3+2];

	// vertex is sorted s.t i0,i1 contains the edge point
	if ( checkEdge(int(i0), int(i1), int(e0), int(e1))) {
		eid = 0;
	}
	else if ( checkEdge(int(i1), int(i2), int(e0), int(e1))) {
		eid = 1;
		PxU32 tmp = i2;
		i2 = i0;
		i0 = i1;
		i1 = tmp;
	}
	else if ( checkEdge(int(i2), int(i0), int(e0), int(e1))) 
	{
		eid = 2;
		PxU32 tmp = i0;
		i0 = i2;
		i2 = i1;
		i1 = tmp;
	}

	// we hit the parent
	if (i2 == parent)
	{
		path = PathIntersection(true, i2, (mVertices[i2] - v).magnitude());
		return 1;
	}

	PxVec3 p0 = mVertices[i0];
	PxVec3 p1 = mVertices[i1];
	PxVec3 p2 = mVertices[i2];

	// project gradient vector on the plane of the triangle
	PxVec3 n = ((p0 - p2).cross(p1 - p2)).getNormalized();
	g = (g - g.dot(n) * n).getNormalized();

	float s = 0.0f, t = 0.0f;
	const float eps = 1e-5;
	PxVec3 ip;

	// intersect against edge form p2 to p0
	if (intersectRayEdge(v, g, p2, p0, s, t) && ( s >= -eps) && ( s <= (1.0f+eps) ) && (t > -eps))
	{
		if ( ( s < eps) || (s > (1.0f-eps)))
		{
			path.vertOrTriangle = true;
			path.index = (s < eps) ? i2 : i0;
			path.distance = (mVertices[path.index] - v).magnitude();
		}
		else
		{
			ip = p2 + s * (p0 - p2);
			path = PathIntersection(false, triNbr*3 + (eid + 2) % 3, (ip - v).magnitude(), s);
			
		}

		return 1;
	}

	// intersect against edge form p1 to p2
	if (intersectRayEdge(v, g, p1, p2, s, t) && ( s >= -eps) && ( s <= (1.0f+eps) ) && (t > -eps))
	{
		if ( ( s < eps) || (s > (1.0f-eps)))
		{
			path.vertOrTriangle = true;
			path.index = (s < eps) ? i1 : i2;
			path.distance = (mVertices[path.index] - v).magnitude();
		}
		else
		{
			ip = p1 + s * (p2 - p1);
			path = PathIntersection(false, triNbr*3 + (eid + 1) % 3, (ip - v).magnitude(), s);
		}
		
		return 1;
	}

	// fallback to pick closer vertex when no edges intersect
	float dir = g.dot(v1-v0);
	path.vertOrTriangle = true;
	path.index = (dir > 0) ? e1 : e0;
	path.distance = (mVertices[path.index] - v).magnitude();

	return 1;
}


///////////////////////////////////////////////////////////////////////////////
// compute geodesic distance and path from vertex i to its parent
float PxClothGeodesicTetherCookerImpl::computeGeodesicDistance(PxU32 i, PxU32 parent, int &errorCode)
{
	if (i == parent)
		return 0.0f;
		
	PathIntersection path;
	
	errorCode = 0;

	// find intial intersection
	int status = computeVertexIntersection(parent, i, path);
	if (status < 0)
	{
		errorCode = -1;
		return 0;
	}

	int pathcnt = 0;
	float geodesicDistance = 0;

	while (status > 0)
	{	
		geodesicDistance += path.distance;

		if (path.vertOrTriangle)
			status = computeVertexIntersection(parent, path.index, path);
		else 
			status = computeEdgeIntersection(parent, path.index, path.s, path);

		// cannot find valid path
		if (status < 0) 
		{
			errorCode = -2;
			return 0.0f;
		}

		// possibly cycles, too many path
		if (pathcnt > 1000) 
		{		
			errorCode = -3;
			return 0.0f;
		}

		pathcnt++;
	}

	return geodesicDistance;
}




#endif //PX_USE_CLOTH_API


