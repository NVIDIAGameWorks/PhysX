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
#include "foundation/PxIO.h"
#include "foundation/PxStrideIterator.h"
#include "extensions/PxClothFabricCooker.h"
#include "extensions/PxClothTetherCooker.h"
#include "PxPhysics.h"
#include "PsFoundation.h"
#include "PsArray.h"
#include "PsHashMap.h"
#include "PsSort.h"

using namespace physx;

struct physx::PxFabricCookerImpl
{
	bool cook(const PxClothMeshDesc& desc, PxVec3 gravity, bool useGeodesicTether);

	PxClothFabricDesc getDescriptor() const;
	void save(PxOutputStream& stream, bool platformMismatch) const;

public:
	PxU32 mNumParticles;

	shdfnd::Array<PxClothFabricPhase> mPhases;
	shdfnd::Array<PxU32> mSets; // with 0 prefix
	shdfnd::Array<PxReal> mRestvalues;
	shdfnd::Array<PxU32> mIndices;

	shdfnd::Array<PxU32> mTetherAnchors;
	shdfnd::Array<PxReal> mTetherLengths;

	shdfnd::Array<PxU32> mTriangles;
};

PxClothFabricCooker::PxClothFabricCooker(const PxClothMeshDesc& desc, const PxVec3& gravity, bool useGeodesicTether)
: mImpl(new PxFabricCookerImpl())
{
	mImpl->cook(desc, gravity, useGeodesicTether);
}

PxClothFabricCooker::~PxClothFabricCooker()
{
	delete mImpl;
}

PxClothFabricDesc PxClothFabricCooker::getDescriptor() const
{
	return mImpl->getDescriptor();
}

void PxClothFabricCooker::save(PxOutputStream& stream, bool platformMismatch) const
{
	mImpl->save(stream, platformMismatch);
}


PxClothFabric* physx::PxClothFabricCreate( PxPhysics& physics, const PxClothMeshDesc& desc, const PxVec3& gravity, bool useGeodesicTether )
{
	PxFabricCookerImpl impl;

	if(!impl.cook(desc, gravity, useGeodesicTether))
		return 0;

	return physics.createClothFabric(impl.getDescriptor());
}

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
	void gatherTriangles(shdfnd::Array<PxU32>& indices, const PxBoundedData& triangles, const PxBoundedData& quads)
	{
		indices.reserve(triangles.count * 3 + quads.count * 6);

		PxStrideIterator<const T> tIt, qIt;
		tIt = PxMakeIterator(reinterpret_cast<const T*>(triangles.data), triangles.stride);
		for (PxU32 i = 0; i<triangles.count; ++i, ++tIt, ++qIt)
		{
			for (PxU32 j = 0; j<3; ++j)
				indices.pushBack(tIt.ptr()[j]);
		}
		qIt = PxMakeIterator(reinterpret_cast<const T*>(quads.data), quads.stride);
		for (PxU32 i = 0; i<quads.count; ++i, ++tIt, ++qIt)
		{
			indices.pushBack(qIt.ptr()[0]);
			indices.pushBack(qIt.ptr()[1]);
			indices.pushBack(qIt.ptr()[2]);
			indices.pushBack(qIt.ptr()[2]);
			indices.pushBack(qIt.ptr()[3]);
			indices.pushBack(qIt.ptr()[0]);
		}
	}

	
	struct Edge
	{
		Edge() : mStretching(0.0f), mBending(0.0f), mShearing(0.0f) {}

		void classify()
		{
			mStretching += 0.1f;
		}

		// classify v0-v2 edge based on alternative v0-v1-v2 path 
		void classify(const PxVec4& v0, const PxVec4& v1, const PxVec4& v2)
		{
			const PxVec3& p0 = reinterpret_cast<const PxVec3&>(v0);
			const PxVec3& p1 = reinterpret_cast<const PxVec3&>(v1);
			const PxVec3& p2 = reinterpret_cast<const PxVec3&>(v2);

			PxReal area = (p1-p0).cross(p2-p1).magnitude();
			// triangle height / base length
			// 1.0 = quad edge, 0.2 = quad diagonal + quad edge, 
			PxReal ratio = area / (p2-p0).magnitudeSquared();

			// 0.5 = quad diagonal
			mShearing += PxMax(0.0f, 0.15f - fabsf(0.45f - ratio));
			// 0.0 = collinear points
			mBending += PxMax(0.0f, 0.1f - ratio) * 3;
		}

		PxReal mStretching;
		PxReal mBending;
		PxReal mShearing;
	};

	typedef shdfnd::Pair<PxU32, PxU32> Pair;
	typedef shdfnd::Pair<Pair, PxClothFabricPhaseType::Enum> Entry;

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
	// Heap element to sort constraint based on graph color count
	struct ConstraintGraphColorCount
	{
		ConstraintGraphColorCount(PxU32 cid, PxU32 count) 
			: constraint(cid), colorCount(count) {}

		PxU32 constraint;
		PxU32 colorCount; 

		bool operator < (const ConstraintGraphColorCount& c) const
		{
			return colorCount < c.colorCount;
		}
	};

	struct ConstraintSorter
	{
	public:

		ConstraintSorter(PxU32* constraints_) : constraints(constraints_) {}

		bool operator()(PxU32 i, PxU32 j) const
		{
			PxU32 ci = i*2;
			PxU32 cj = j*2;

			if (constraints[ci] == constraints[cj])
				return constraints[ci+1] < constraints[cj+1];
			else
				return constraints[ci] < constraints[cj];
		}

		PxU32* constraints;
	};

} // anonymous namespace

bool PxFabricCookerImpl::cook(const PxClothMeshDesc& desc, PxVec3 gravity, bool useGeodesicTether)
{	
	if(!desc.isValid())
	{
		shdfnd::getFoundation().error(PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, 
			"PxFabricCookerImpl::cook: desc.isValid() failed!");
		return false;
	}

	gravity = gravity.getNormalized();

	mNumParticles = desc.points.count;

	// assemble points
	shdfnd::Array<PxVec4> particles;
	particles.reserve(mNumParticles);
	PxStrideIterator<const PxVec3> pIt(reinterpret_cast<const PxVec3*>(desc.points.data), desc.points.stride);
	PxStrideIterator<const PxReal> wIt(reinterpret_cast<const PxReal*>(desc.invMasses.data), desc.invMasses.stride);
	for(PxU32 i=0; i<mNumParticles; ++i)
		particles.pushBack(PxVec4(*pIt++, wIt.ptr() ? *wIt++ : 1.0f));

	// build adjacent vertex list
	shdfnd::Array<PxU32> valency(mNumParticles+1, 0);
	shdfnd::Array<PxU32> adjacencies;
	if (desc.flags & PxMeshFlag::e16_BIT_INDICES)
	{
		gatherTriangles<PxU16>(mTriangles, desc.triangles, desc.quads);
		gatherAdjacencies<PxU16>(valency, adjacencies, desc.triangles, desc.quads);
	}
	else
	{
		gatherTriangles<PxU32>(mTriangles, desc.triangles, desc.quads);
		gatherAdjacencies<PxU32>(valency, adjacencies, desc.triangles, desc.quads);
	}

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

	// build map of unique edges and classify
	shdfnd::HashMap<Pair, Edge> edges;
	for(PxU32 i=0; i<mNumParticles; ++i)
	{
		PxReal wi = particles[i].w;
		// iterate all neighbors
		PxU32 jlast = valency[i+1];
		for(PxU32 j=valency[i]; j<jlast; ++j)
		{
			// add 1-ring edge
			PxU32 m = neighbors[j];
			if(wi + particles[m].w > 0.0f)
				edges[Pair(PxMin(i, m), PxMax(i, m))].classify();

			// iterate all neighbors of neighbor
			PxU32 klast = valency[m+1];
			for(PxU32 k=valency[m]; k<klast; ++k)
			{
				PxU32 n = neighbors[k];
				if(n != i && wi + particles[n].w > 0.0f)
				{
					// add 2-ring edge
					edges[Pair(PxMin(i, n), PxMax(i, n))].classify(
						particles[i], particles[m], particles[n]);
				}
			}
		}
	}

	// copy classified edges to constraints array
	// build histogram of constraints per vertex
	shdfnd::Array<Entry> constraints; 	
	constraints.reserve(edges.size());
	valency.resize(0); valency.resize(mNumParticles+1, 0);

	const PxReal sqrtHalf = PxSqrt(0.4f);
	for(shdfnd::HashMap<Pair, Edge>::Iterator eIt = edges.getIterator(); !eIt.done(); ++eIt)
	{
		const Edge& edge = eIt->second;
		const Pair& pair = eIt->first;
		if((edge.mStretching + edge.mBending + edge.mShearing) > 0.0f)
		{	
			PxClothFabricPhaseType::Enum type = PxClothFabricPhaseType::eINVALID;
			if(edge.mBending > PxMax(edge.mStretching, edge.mShearing))
				type = PxClothFabricPhaseType::eBENDING;
			else if(edge.mShearing > PxMax(edge.mStretching, edge.mBending))
				type = PxClothFabricPhaseType::eSHEARING;
			else 
			{
				PxVec4 diff = particles[pair.first]-particles[pair.second];
				PxReal dot = gravity.dot(reinterpret_cast<const PxVec3&>(diff).getNormalized());
				type = fabsf(dot) < sqrtHalf ? PxClothFabricPhaseType::eHORIZONTAL : PxClothFabricPhaseType::eVERTICAL;
			}
			++valency[pair.first];
			++valency[pair.second];
			constraints.pushBack(Entry(pair, type));
		}
	} 

	prefixSum(valency.begin(), valency.end(), valency.begin());

	PxU32 numConstraints = constraints.size();

	// build adjacent constraint list
	adjacencies.resize(0); adjacencies.resize(valency.back(), 0);
	for(PxU32 i=0; i<numConstraints; ++i)
	{
		adjacencies[--valency[constraints[i].first.first]] = i;
		adjacencies[--valency[constraints[i].first.second]] = i;
	}
	
	shdfnd::Array<PxU32>::ConstIterator aFirst = adjacencies.begin();
	shdfnd::Array<PxU32> colors(numConstraints, numConstraints); // constraint -> color, initialily not colored
	mark.resize(0); mark.resize(numConstraints+1, PX_MAX_U32); // color -> constraint index
	shdfnd::Array<PxU32> adjColorCount(numConstraints, 0); // # of neighbors that are already colored

	shdfnd::Array<ConstraintGraphColorCount> constraintHeap; 
	constraintHeap.reserve(numConstraints); // set of constraints to color (added in edge distance order)

	// Do graph coloring based on edge distance.
	// For each constraint, we add its uncolored neighbors to the heap
	// ,and we pick the constraint with most colored neighbors from the heap.
	for(;;)
	{
		PxU32 constraint = 0;
		while ( (constraint < numConstraints) && (colors[constraint] != numConstraints))
			constraint++; // start with the first uncolored constraint
	
		if (constraint >= numConstraints)
			break;

		constraintHeap.clear();
		pushHeap(constraintHeap, ConstraintGraphColorCount(constraint, adjColorCount[constraint]));
		PxClothFabricPhaseType::Enum type = constraints[constraint].second;
		
		while (!constraintHeap.empty())
		{		
			ConstraintGraphColorCount heapItem = popHeap(constraintHeap);
			constraint = heapItem.constraint;
			if (colors[constraint] != numConstraints)
				continue; // skip if already colored 

			const Pair& pair = constraints[constraint].first;			
			for(PxU32 j=0; j<2; ++j)
			{
				PxU32 index = j ? pair.first : pair.second;
				if(particles[index].w == 0.0f)
					continue; // don't mark adjacent particles if attached

				for(shdfnd::Array<PxU32>::ConstIterator aIt = aFirst + valency[index], aEnd = aFirst + valency[index+1]; aIt != aEnd; ++aIt)
				{				
					PxU32 adjacentConstraint = *aIt;
					if ((constraints[adjacentConstraint].second != type) || (adjacentConstraint == constraint))
						continue;

					mark[colors[adjacentConstraint]] = constraint; 
					++adjColorCount[adjacentConstraint];
					pushHeap(constraintHeap, ConstraintGraphColorCount(adjacentConstraint, adjColorCount[adjacentConstraint]));
				}
			}

			// find smallest color with matching type
			PxU32 color = 0;
			while((color < mPhases.size() && mPhases[color].phaseType != type) || mark[color] == constraint)
				++color;

			// create a new color set
			if(color == mPhases.size())
			{
				PxClothFabricPhase phase(type, mPhases.size());
				mPhases.pushBack(phase);
				mSets.pushBack(0);
			}

			colors[constraint] = color;
			++mSets[color];
		} 
	}

#if 0 // PX_DEBUG
	printf("set[%u] = ", mSets.size());
	for(PxU32 i=0; i<mSets.size(); ++i)
		printf("%u ", mSets[i]);
#endif

	prefixSum(mSets.begin(), mSets.end(), mSets.begin());

#if 0 // PX_DEBUG
	printf(" = %u\n", mSets.back());
#endif

	// write indices and rest lengths
	// convert mSets to exclusive sum
	PxU32 back = mSets.back();
	mSets.pushBack(back);
	mIndices.resize(numConstraints*2);
	mRestvalues.resize(numConstraints);
	for(PxU32 i=0; i<numConstraints; ++i)
	{
		PxU32 first = constraints[i].first.first;
		PxU32 second = constraints[i].first.second;

		PxU32 index = --mSets[colors[i]];

		mIndices[2*index  ] = first;
		mIndices[2*index+1] = second;

		PxVec4 diff = particles[second] - particles[first];
		mRestvalues[index] = reinterpret_cast<
			const PxVec3&>(diff).magnitude();
	} 
	
	// reorder constraints and rest values for more efficient cache access (linear)
	shdfnd::Array<PxU32> newIndices(mIndices.size());
	shdfnd::Array<PxF32> newRestValues(mRestvalues.size());

	// sort each constraint set in vertex order
	for (PxU32 i=0; i < mSets.size()-1; ++i)
	{
		// create a re-ordering list
		shdfnd::Array<PxU32> reorder(mSets[i+1]-mSets[i]);

		for (PxU32 r=0; r < reorder.size(); ++r)
			reorder[r] = r;

		const PxU32 indicesOffset = mSets[i]*2;
		const PxU32 restOffset = mSets[i];

		ConstraintSorter predicate(&mIndices[indicesOffset]);
		shdfnd::sort(&reorder[0], reorder.size(), predicate);
		
		for (PxU32 r=0; r < reorder.size(); ++r)
		{
			newIndices[indicesOffset + r*2] = mIndices[indicesOffset + reorder[r]*2];
			newIndices[indicesOffset + r*2+1] = mIndices[indicesOffset + reorder[r]*2+1];
			newRestValues[restOffset + r] = mRestvalues[restOffset + reorder[r]];
		}
	}

	mIndices = newIndices;
	mRestvalues = newRestValues;

	PX_ASSERT(mIndices.size() == mRestvalues.size()*2);
	PX_ASSERT(mRestvalues.size() == mSets.back());

#if 0 // PX_DEBUG
	for (PxU32 i = 1; i < mSets.size(); i++)
	{
		PxClothFabricPhase phase = mPhases[i-1];
		printf("%d : type %d, size %d\n", 
			i-1, phase.phaseType, mSets[i] - mSets[i-1]);
	}
#endif

	if (useGeodesicTether)
	{
		PxClothGeodesicTetherCooker tetherCooker(desc);
		if (tetherCooker.getCookerStatus() == 0)
		{
			PxU32 numTethersPerParticle = tetherCooker.getNbTethersPerParticle();
			PxU32 tetherSize = mNumParticles * numTethersPerParticle;
			mTetherAnchors.resize(tetherSize);
			mTetherLengths.resize(tetherSize);
			tetherCooker.getTetherData(mTetherAnchors.begin(), mTetherLengths.begin());
		}
		else
			useGeodesicTether = false;
	}

	if (!useGeodesicTether)
	{
		PxClothSimpleTetherCooker tetherCooker(desc);
		mTetherAnchors.resize(mNumParticles);
		mTetherLengths.resize(mNumParticles);
		tetherCooker.getTetherData(mTetherAnchors.begin(), mTetherLengths.begin());
	}

	return true;
}

physx::PxClothFabricDesc physx::PxFabricCookerImpl::getDescriptor() const
{
	PxClothFabricDesc result;

	result.nbParticles = mNumParticles;
	result.nbPhases = mPhases.size();
	result.phases = mPhases.begin();
	result.nbSets = mSets.size()-1;
	result.sets = mSets.begin()+1;
	result.restvalues = mRestvalues.begin();
	result.indices = mIndices.begin();
	result.nbTethers = mTetherAnchors.size();
	result.tetherAnchors = mTetherAnchors.begin();
	result.tetherLengths = mTetherLengths.begin();
	result.nbTriangles = mTriangles.size() / 3;
	result.triangles = mTriangles.begin();

	return result;
}

void physx::PxFabricCookerImpl::save( PxOutputStream& stream, bool /*platformMismatch*/ ) const
{
	// version 1 is equivalent to 0x030300 and 0x030301 (PX_PHYSICS_VERSION of 3.3.0 and 3.3.1).
	// If the stream format changes, the loader code in ScClothFabricCore.cpp 
	// and the version number need to change too. 
	PxU32 version = 1;
	stream.write(&version, sizeof(PxU32));

	PxClothFabricDesc desc = getDescriptor();

	// write explicit sizes, others are implicit
	stream.write(&mNumParticles, sizeof(PxU32));
	stream.write(&desc.nbPhases, sizeof(PxU32));
	stream.write(&desc.nbSets, sizeof(PxU32));
	stream.write(&desc.nbTethers, sizeof(PxU32));

	PxU32 nbConstraints = desc.sets[desc.nbSets-1];

	// write actual data
	PX_COMPILE_TIME_ASSERT(sizeof(PxClothFabricPhaseType::Enum) == sizeof(PxU32));
	stream.write(desc.phases, desc.nbPhases*sizeof(PxClothFabricPhase));
	stream.write(desc.sets, desc.nbSets*sizeof(PxU32));

	stream.write(desc.restvalues, nbConstraints*sizeof(PxReal));
	stream.write(desc.indices, nbConstraints*2*sizeof(PxU32));

	stream.write(desc.tetherAnchors, desc.nbTethers*sizeof(PxU32));
	stream.write(desc.tetherLengths, desc.nbTethers*sizeof(PxReal));
}

#endif //PX_USE_CLOTH_API
