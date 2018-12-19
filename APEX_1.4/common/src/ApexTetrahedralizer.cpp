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



#include "ApexTetrahedralizer.h"

#include "ApexSDKIntl.h"

#include "ApexCollision.h"
#include "ApexMeshHash.h"
#include "ApexSharedUtils.h"

#include "PsSort.h"

namespace nvidia
{
namespace apex
{

// check for sanity in between algorithms
#define TETRALIZER_SANITY_CHECKS 0

const uint32_t ApexTetrahedralizer::FullTetrahedron::sideIndices[4][3] = {{1, 2, 3}, {2, 0, 3}, {0, 1, 3}, {2, 1, 0}};

void ApexTetrahedralizer::TetraEdgeList::sort()
{
	shdfnd::sort(mEdges.begin(), mEdges.size(), TetraEdge());
}


ApexTetrahedralizer::ApexTetrahedralizer(uint32_t subdivision) :
	mMeshHash(NULL),
	mSubdivision(subdivision),
	mBoundDiagonal(0),
	mFirstFarVertex(0),
	mLastFarVertex(0)
{
	mBound.setEmpty();
}



ApexTetrahedralizer::~ApexTetrahedralizer()
{
	if (mMeshHash != NULL)
	{
		delete mMeshHash;
		mMeshHash = NULL;
	}
}



void ApexTetrahedralizer::registerVertex(const PxVec3& pos)
{
	TetraVertex v;
	v.init(pos, 0);
	mVertices.pushBack(v);
	mBound.include(pos);
}



void ApexTetrahedralizer::registerTriangle(uint32_t i0, uint32_t i1, uint32_t i2)
{
	mIndices.pushBack(i0);
	mIndices.pushBack(i1);
	mIndices.pushBack(i2);
}



void ApexTetrahedralizer::endRegistration(IProgressListener* progressListener)
{

	// hash mesh triangles for faster access
	mBoundDiagonal = (mBound.minimum - mBound.maximum).magnitude();

	HierarchicalProgressListener progress(100, progressListener);

	if (mMeshHash == NULL)
	{
		mMeshHash = PX_NEW(ApexMeshHash)();
	}

	// clears the hash if it's not empty
	mMeshHash->setGridSpacing(0.1f * mBoundDiagonal);

	weldVertices();

	for (uint32_t i = 0; i < mIndices.size(); i += 3)
	{
		PxBounds3 triangleBound;
		triangleBound.minimum = triangleBound.maximum = mVertices[mIndices[i]].pos;
		triangleBound.include(mVertices[mIndices[i + 1]].pos);
		triangleBound.include(mVertices[mIndices[i + 2]].pos);
		mMeshHash->add(triangleBound, i);
	}


	progress.setSubtaskWork(10, "Delaunay Tetrahedralization");
	delaunayTetrahedralization(&progress);
	progress.completeSubtask();

	// neighbor info is gone from this point on!
	compressTetrahedra(true);

#if 1 // do we really need this?
	uint32_t tetStart = 0;
	uint32_t oldNumTets = mTetras.size();
	uint32_t numSwapped = 0;
	uint32_t lastNumSwapped = mTetras.size() * 10; // just a number that is way too big
	uint32_t iterations = 0; // this one is pure safety
	int32_t fraction = 50;
	do
	{
		fraction /= 2;
		progress.setSubtaskWork(fraction, "Swapping Tetrahedra");
		lastNumSwapped = numSwapped;
		numSwapped = swapTetrahedra(tetStart, &progress);
		tetStart = oldNumTets;
		oldNumTets = mTetras.size();
		progress.completeSubtask();
	}
	while (numSwapped > 0 && (numSwapped < lastNumSwapped) && (iterations++ < 20));


	// just to fill the entire fraction up
	progress.setSubtaskWork(fraction, "Swapping Tetrahedra");
	progress.completeSubtask();
#endif



	progress.setSubtaskWork(-1, "Removing outer tetrahedra");
	removeOuterTetrahedra(&progress);
	progress.completeSubtask();

	compressTetrahedra(true);
	compressVertices();

	// release the indices
	mIndices.clear();
}



void ApexTetrahedralizer::getVertices(PxVec3* data)
{
	for (uint32_t i = 0; i < mVertices.size(); i++)
	{
		data[i] = mVertices[i].pos;
	}
}



void ApexTetrahedralizer::getIndices(uint32_t* data)
{
	for (uint32_t i = 0; i < mTetras.size(); i++)
	{
		for (uint32_t j = 0; j < 4; j++)
		{
			data[i * 4 + j] = (uint32_t)mTetras[i].vertexNr[j];
		}
	}
}



void ApexTetrahedralizer::weldVertices()
{
	if (mSubdivision < 1)
	{
		return;
	}

	const float relativeThreshold = 0.3f;

	const float d = mBoundDiagonal / mSubdivision * relativeThreshold;
	const float d2 = d * d;

	uint32_t readIndex = 0;
	uint32_t writeIndex = 0;
	physx::Array<int32_t> old2new(mVertices.size(), -1);
	while (readIndex < mVertices.size())
	{
		for (uint32_t i = 0; i < writeIndex; i++)
		{
			if ((mVertices[i].pos - mVertices[readIndex].pos).magnitudeSquared() < d2)
			{
				old2new[readIndex] = (int32_t)i;
				break;
			}
		}

		if (old2new[readIndex] == -1)
		{
			old2new[readIndex] = (int32_t)writeIndex;
			mVertices[writeIndex] = mVertices[readIndex];
			writeIndex++;
		}
		readIndex++;
	}

	if (readIndex == writeIndex)
	{
		return;
	}

	for (uint32_t i = 0; i < mIndices.size(); i++)
	{
		PX_ASSERT(old2new[mIndices[i]] != -1);
		mIndices[i] = (uint32_t)old2new[mIndices[i]];
	}
}


void ApexTetrahedralizer::delaunayTetrahedralization(IProgressListener* progress)
{
	physx::Array<TetraEdge> edges;
	TetraEdge edge;

	// start with huge tetrahedron
	mTetras.clear();

	const float a = 3.0f * mBoundDiagonal;
	const float x  = 0.5f * a;
	const float y0 = x / PxSqrt(3.0f);
	const float y1 = x * PxSqrt(3.0f) - y0;
	const float z0 = 0.25f * PxSqrt(6.0f) * a;
	const float z1 = a * PxSqrt(6.0f) / 3.0f - z0;

	PxVec3 center = mBound.getCenter();

	mFirstFarVertex = mVertices.size();
	PxVec3 p0(-x, -y0, -z1);
	registerVertex(center + p0);
	PxVec3 p1(x, -y0, -z1);
	registerVertex(center + p1);
	PxVec3 p2(0, y1, -z1);
	registerVertex(center + p2);
	PxVec3 p3(0,  0, z0);
	registerVertex(center + p3);
	mLastFarVertex = mVertices.size() - 1;

	// do not update it as these vertices are irrelevant
	//mBoundDiagonal = mBound.minimum.distance(mBound.maximum);

	FullTetrahedron tetra;
	tetra.set((int32_t)mFirstFarVertex, (int32_t)mFirstFarVertex + 1, (int32_t)mFirstFarVertex + 2, (int32_t)mFirstFarVertex + 3);
	mTetras.pushBack(tetra);

	// build Delaunay triangulation iteratively

	for (uint32_t i = 0; i < mFirstFarVertex; i++)
	{

		if (progress != NULL && (i & 0x7f) == 0)
		{
			const int32_t percent = (int32_t)(100 * i / mFirstFarVertex);
			progress->setProgress(percent);
		}

		PxVec3& v = mVertices[i].pos;
		/// \todo vertex reordering could speed up this search via warm start
		int tNr = findSurroundingTetra(mTetras.size() - 1, v);	// fast walk
		if (tNr == -1)
		{
			continue;
		}
		PX_ASSERT(tNr >= 0);

		const uint32_t newTetraNr = mTetras.size();
		const float volumeDiff = retriangulate((uint32_t)tNr, i);

#if TETRALIZER_SANITY_CHECKS
		if (PxAbs(volumeDiff) > 0.001f)
		{
			APEX_INTERNAL_ERROR("Volume added: %f", volumeDiff);
		}
#else
		PX_UNUSED(volumeDiff);
#endif

		edges.clear();
		for (uint32_t j = newTetraNr; j < mTetras.size(); j++)
		{
			FullTetrahedron& newTet = mTetras[j];
			edge.init(newTet.vertexNr[2], newTet.vertexNr[3], (int32_t)j, 1);
			edges.pushBack(edge);
			edge.init(newTet.vertexNr[3], newTet.vertexNr[1], (int32_t)j, 2);
			edges.pushBack(edge);
			edge.init(newTet.vertexNr[1], newTet.vertexNr[2], (int32_t)j, 3);
			edges.pushBack(edge);
		}

		shdfnd::sort(edges.begin(), edges.size(), TetraEdge());

		for (uint32_t j = 0; j < edges.size(); j += 2)
		{
			TetraEdge& edge0 = edges[j];
			TetraEdge& edge1 = edges[j + 1];
			PX_ASSERT(edge0 == edge1);
			mTetras[edge0.tetraNr].neighborNr[(uint32_t)edge0.neighborNr] = (int32_t)edge1.tetraNr;
			mTetras[edge1.tetraNr].neighborNr[(uint32_t)edge1.neighborNr] = (int32_t)edge0.tetraNr;
		}
	}
}





int32_t ApexTetrahedralizer::findSurroundingTetra(uint32_t startTetra, const PxVec3& p) const
{
	// find the tetrahedra which contains the vertex
	// by walking through mesh O(n ^ (1/3))

	if (mTetras.empty())
	{
		return -1;
	}

	PX_ASSERT(mTetras[startTetra].bDeleted == 0);


	uint32_t iterationCounter = 0;
	const uint32_t iterationCounterMax = PxMin(1000u, mTetras.size());

	int32_t tetNr = (int32_t)startTetra;
	while (tetNr >= 0 && iterationCounter++ < iterationCounterMax)
	{
		const FullTetrahedron& tetra = mTetras[(uint32_t)tetNr];
		PX_ASSERT(tetra.bDeleted == 0);

		const PxVec3& p0 = mVertices[(uint32_t)tetra.vertexNr[0]].pos;
		PxVec3 q  = p - p0;
		PxVec3 q0 = mVertices[(uint32_t)tetra.vertexNr[1]].pos - p0;
		PxVec3 q1 = mVertices[(uint32_t)tetra.vertexNr[2]].pos - p0;
		PxVec3 q2 = mVertices[(uint32_t)tetra.vertexNr[3]].pos - p0;
		PxMat33 m(q0,q1,q2);
		const float det = m.getDeterminant();
		m.column0 = q;
		const float x = m.getDeterminant();
		if (x < 0.0f && tetra.neighborNr[1] >= 0)
		{
			tetNr = tetra.neighborNr[1];
			continue;
		}

		m.column0 = q0;
		m.column1 = q;
		const float y = m.getDeterminant();
		if (y < 0.0f && tetra.neighborNr[2] >= 0)
		{
			tetNr = tetra.neighborNr[2];
			continue;
		}

		m.column1 = q1;
		m.column2 = q;
		const float z = m.getDeterminant();

		if (z < 0.0f && tetra.neighborNr[3] >= 0)
		{
			tetNr = tetra.neighborNr[3];
			continue;
		}

		if (x + y + z > det  && tetra.neighborNr[0] >= 0)
		{
			tetNr = tetra.neighborNr[0];
			continue;
		}

#if TETRALIZER_SANITY_CHECKS
		static uint32_t maxIterationCounter = 0;
		maxIterationCounter = PxMax(maxIterationCounter, iterationCounter);
#endif
		return tetNr;
	}
	if (iterationCounter == iterationCounterMax + 1)
	{
		// search all vertices
		for (uint32_t i = 0; i < mVertices.size(); i++)
		{
			if (mVertices[i].pos == p)
			{
				PX_ASSERT(!mVertices[i].isDeleted());
				return -1;
			}
		}

		uint32_t nbDeleted = 0;
		for (uint32_t i = 0; i < mTetras.size(); i++)
		{
			const FullTetrahedron& tetra = mTetras[i];
			if (tetra.bDeleted == 0)
			{
				if (pointInTetra(tetra, p))
				{
					if (tetra.bDeleted == 0)
					{
						return (int32_t)i;
					}
					nbDeleted++;
				}
			}
		}
#if defined(_DEBUG) || TETRALIZER_SANITY_CHECKS
		// PH: Do not give out this warning/error when running in release mode
		APEX_INTERNAL_ERROR("Failed to find tetra for hull vertex");
		//debugPoints.pushBack(p);
#endif
	}
	return -1;
}



float ApexTetrahedralizer::retriangulate(const uint32_t tetraNr, uint32_t vertexNr)
{
	PX_ASSERT(tetraNr < mTetras.size());
	PX_ASSERT(vertexNr < mVertices.size());

	if (mTetras[tetraNr].bDeleted == 1)
	{
		return 0;
	}

#if TETRALIZER_SANITY_CHECKS
	const float volumeDeleted = getTetraVolume(mTetras[tetraNr].vertexNr[0], mTetras[tetraNr].vertexNr[1], mTetras[tetraNr].vertexNr[2], mTetras[tetraNr].vertexNr[3]);;
#endif
	float volumeCreated = 0;

	mTetras[tetraNr].bDeleted = 1;

	PxVec3& v = mVertices[vertexNr].pos;
	for (uint32_t i = 0; i < 4; i++)
	{
		int n = mTetras[tetraNr].neighborNr[i];
		if (n >= 0 && mTetras[(uint32_t)n].bDeleted == 1)
		{
			continue;
		}

		if (n >= 0 && pointInCircumSphere(mTetras[(uint32_t)n], v))
		{
			volumeCreated += retriangulate((uint32_t)n, vertexNr);
		}
		else
		{
			FullTetrahedron& tetra = mTetras[tetraNr];
			FullTetrahedron tNew;
			tNew.set((int32_t)vertexNr,
			         tetra.vertexNr[FullTetrahedron::sideIndices[i][0]],
			         tetra.vertexNr[FullTetrahedron::sideIndices[i][1]],
			         tetra.vertexNr[FullTetrahedron::sideIndices[i][2]]
			        );

#if TETRALIZER_SANITY_CHECKS
			volumeCreated += getTetraVolume(tNew.vertexNr[0], tNew.vertexNr[1], tNew.vertexNr[2], tNew.vertexNr[3]);
#endif

			tNew.neighborNr[0] = n;

			if (n >= 0)
			{
				PX_ASSERT(mTetras[(uint32_t)n].neighborOf(tNew.vertexNr[1], tNew.vertexNr[2], tNew.vertexNr[3]) == (int32_t)tetraNr);
				mTetras[(uint32_t)n].neighborOf(tNew.vertexNr[1], tNew.vertexNr[2], tNew.vertexNr[3]) = (int32_t)mTetras.size();
			}
			mTetras.pushBack(tNew);
		}
	}
#if TETRALIZER_SANITY_CHECKS
	return volumeCreated - volumeDeleted;
#else
	return 0;
#endif
}



uint32_t ApexTetrahedralizer::swapTetrahedra(uint32_t startTet, IProgressListener* progress)
{
	PX_ASSERT(mMeshHash != NULL);

	const float threshold = 0.05f * mBoundDiagonal / mSubdivision;

	TetraEdgeList edges;
	edges.mEdges.reserve(mTetras.size() * 6);
	for (uint32_t i = startTet; i < mTetras.size(); i++)
	{
		FullTetrahedron& t = mTetras[i];
		if (t.bDeleted == 1)
		{
			continue;
		}

		TetraEdge edge;
		edge.init(t.vertexNr[0], t.vertexNr[1], (int32_t)i);
		edges.add(edge);
		edge.init(t.vertexNr[1], t.vertexNr[2], (int32_t)i);
		edges.add(edge);
		edge.init(t.vertexNr[2], t.vertexNr[0], (int32_t)i);
		edges.add(edge);
		edge.init(t.vertexNr[0], t.vertexNr[3], (int32_t)i);
		edges.add(edge);
		edge.init(t.vertexNr[1], t.vertexNr[3], (int32_t)i);
		edges.add(edge);
		edge.init(t.vertexNr[2], t.vertexNr[3], (int32_t)i);
		edges.add(edge);
	}
	edges.sort();


	uint32_t index = 0;
	uint32_t progressCounter = 0;
	uint32_t numEdgesSwapped = 0;
	while (index < edges.numEdges())
	{
		if (progress != NULL && ((progressCounter++ & 0xf) == 0))
		{
			const int32_t percent = (int32_t)(100 * index / edges.numEdges());
			progress->setProgress(percent);
		}
		TetraEdge edge = edges[index];
		uint32_t vNr[2] = { edge.vNr0, edge.vNr1 };

		index++;
		while (index < edges.numEdges() && edges[index] == edge)
		{
			index++;
		}

		if (isFarVertex(vNr[0]) || isFarVertex(vNr[1]))
		{
			continue;
		}

		const PxVec3 v0 = mVertices[vNr[0]].pos;
		const PxVec3 v1 = mVertices[vNr[1]].pos;
		PxBounds3 edgeBounds;
		edgeBounds.setEmpty();
		edgeBounds.include(v0);
		edgeBounds.include(v1);
		mMeshHash->queryUnique(edgeBounds, mTempItemIndices);
		if (mTempItemIndices.size() == 0)
		{
			continue;
		}

		bool cut = false;
		for (uint32_t k = 0; k < mTempItemIndices.size(); k++)
		{
			uint32_t triNr = mTempItemIndices[k];
			PX_ASSERT(triNr % 3 == 0);
			uint32_t* triangle = &mIndices[triNr];

			// if one vertex is part of the iso surface
			if (triangleContainsVertexNr(triangle, vNr, 2))
			{
				continue;
			}

			const PxVec3 p0 = mVertices[triangle[0]].pos;
			const PxVec3 p1 = mVertices[triangle[1]].pos;
			const PxVec3 p2 = mVertices[triangle[2]].pos;

			PxBounds3 triBounds;
			triBounds.minimum = triBounds.maximum = p0;
			triBounds.include(p1);
			triBounds.include(p2);

			if (!triBounds.intersects(edgeBounds))
			{
				continue;
			}

			PxVec3 normal = (p1 - p0).cross(p2 - p0);
			normal.normalize();

			if (PxAbs(normal.dot(v0 - p0)) < threshold)
			{
				continue;
			}
			if (PxAbs(normal.dot(v1 - p0)) < threshold)
			{
				continue;
			}

			float t, u, v;
			if (!APEX_RayTriangleIntersect(v0, v1 - v0, p0, p1, p2, t, u, v))
			{
				continue;
			}

			// PH: I guess we don't need to cut when edge does not intersect triangle
			if (t < 0 || t > 1)
			{
				continue;
			}

			cut = true;
			break;
		}
		if (cut)
		{
#if TETRAHEDRALIZER_DEBUG_RENDERING
			debugLines.pushBack(mVertices[vNr[0]].pos);
			debugLines.pushBack(mVertices[vNr[1]].pos);
#endif
			numEdgesSwapped += swapEdge(vNr[0], vNr[1]) ? 1 : 0;
		}
	}

	return numEdgesSwapped;
}



bool ApexTetrahedralizer::swapEdge(uint32_t v0, uint32_t v1)
{
	physx::Array<int32_t> borderEdges;
	physx::Array<FullTetrahedron> newTetras;

	uint32_t nbDeleted = 0;

	mTempItemIndices.clear();

	for (uint32_t i = 0; i < mTetras.size(); i++)
	{
		FullTetrahedron& t = mTetras[i];

		if (t.bDeleted == 1)
		{
			continue;
		}

		if (!t.containsVertex((int32_t)v0) || !t.containsVertex((int32_t)v1))
		{
			continue;
		}

		//t.bDeleted = 1;
		mTempItemIndices.pushBack(i);
		nbDeleted++;
		int32_t v2, v3;
		t.get2OppositeVertices((int32_t)v0, (int32_t)v1, v2, v3);
		if (getTetraVolume((int32_t)v0, (int32_t)v1, v2, v3) >= 0.0f)
		{
			borderEdges.pushBack(v2);
			borderEdges.pushBack(v3);
		}
		else
		{
			borderEdges.pushBack(v3);
			borderEdges.pushBack(v2);
		}
	}

	if (borderEdges.size() < 6)
	{
		return false;
	}

	// start with first edge
	physx::Array<int32_t> borderVertices;
	physx::Array<float> borderQuality;
	borderVertices.pushBack(borderEdges[borderEdges.size() - 2]);
	borderVertices.pushBack(borderEdges[borderEdges.size() - 1]);
	borderEdges.popBack();
	borderEdges.popBack();

	// construct border
	int vEnd = borderVertices[1];
	while (borderEdges.size() > 0)
	{
		uint32_t i = 0;
		uint32_t num = borderEdges.size();
		for (; i < num - 1; i += 2)
		{
			if (borderEdges[i] == vEnd)
			{
				vEnd = borderEdges[i + 1];
				break;
			}
		}
		// not connected
		if (i >= num)
		{
			return false;
		}

		borderVertices.pushBack(vEnd);
		borderEdges[i] = borderEdges[num - 2];
		borderEdges[i + 1] = borderEdges[num - 1];
		borderEdges.popBack();
		borderEdges.popBack();
	}

	// not circular
	if (borderVertices[0] != borderVertices[borderVertices.size() - 1])
	{
		return false;
	}

	borderVertices.popBack();

	if (borderVertices.size() < 3)
	{
		return false;
	}

	// generate tetrahedra
	FullTetrahedron tetra0, tetra1;
	borderQuality.resize(borderVertices.size());
	while (borderVertices.size() > 3)
	{
		uint32_t num = borderVertices.size();
		uint32_t i0, i1, i2;
		for (i0 = 0; i0 < num; i0++)
		{
			i1 = (i0 + 1) % num;
			i2 = (i1 + 1) % num;
			tetra0.set(borderVertices[i0], borderVertices[i1], borderVertices[i2], (int32_t)v1);
			borderQuality[i1] = getTetraQuality(tetra0);
		}

		float maxQ = 0.0f;
		int32_t maxI0 = -1;
		for (i0 = 0; i0 < num; i0++)
		{
			i1 = (i0 + 1) % num;
			i2 = (i1 + 1) % num;
			tetra0.set(borderVertices[i0], borderVertices[i1], borderVertices[i2], (int32_t)v1);
			tetra1.set(borderVertices[i2], borderVertices[i1], borderVertices[i0], (int32_t)v0);
			if (getTetraVolume(tetra0) < 0.0f || getTetraVolume(tetra1) < 0.0f)
			{
				continue;
			}

			bool ear = true;
			for (uint32_t i = 0; i < num; i++)
			{
				if (i == i0 || i == i1 || i == i2)
				{
					continue;
				}

				PxVec3& pos = mVertices[(uint32_t)borderVertices[i]].pos;
				if (pointInTetra(tetra0, pos) || pointInTetra(tetra1, pos))
				{
					ear = false;
				}
			}

			if (!ear)
			{
				continue;
			}

			float q = (1.0f - borderQuality[i0]) + borderQuality[i1] + (1.0f - borderQuality[i2]);

			if (maxI0 < 0 || q > maxQ)
			{
				maxQ = q;
				maxI0 = (int32_t)i0;
			}
		}
		if (maxI0 < 0)
		{
			return false;
		}

		i0 = (uint32_t)maxI0;
		i1 = (i0 + 1) % num;
		i2 = (i1 + 1) % num;
		tetra0.set(borderVertices[i0], borderVertices[i1], borderVertices[i2], (int32_t)v1);
		tetra1.set(borderVertices[i2], borderVertices[i1], borderVertices[i0], (int32_t)v0);

		// add tetras, remove vertex;
		newTetras.pushBack(tetra0);
		newTetras.pushBack(tetra1);
		for (uint32_t i = i1; i < num - 1; i++)
		{
			borderVertices[i] = borderVertices[i + 1];
		}
		borderVertices.popBack();
	}
	tetra0.set(borderVertices[0], borderVertices[1], borderVertices[2], (int32_t)v1);
	tetra1.set(borderVertices[2], borderVertices[1], borderVertices[0], (int32_t)v0);

	if (getTetraVolume(tetra0) < 0.0f || getTetraVolume(tetra1) < 0.0f)
	{
		return false;
	}

	newTetras.pushBack(tetra0);
	newTetras.pushBack(tetra1);

	PX_ASSERT(nbDeleted <= newTetras.size() + 1);

	for (uint32_t i = 0; i < mTempItemIndices.size(); i++)
	{
		mTetras[mTempItemIndices[i]].bDeleted = 1;
	}

	// add new tetras;
	for (uint32_t i = 0; i < newTetras.size(); i++)
	{
		mTetras.pushBack(newTetras[i]);
	}

	return true;
}


class F32Less
{
public:
	PX_INLINE bool operator()(float v1, float v2) const
	{
		return v1 < v2;
	}
};



bool ApexTetrahedralizer::removeOuterTetrahedra(IProgressListener* progress)
{
	const float EPSILON = 1e-5f;
	PX_ASSERT((float)(mBoundDiagonal + EPSILON) > mBoundDiagonal);

#if TETRAHEDRALIZER_DEBUG_RENDERING && TETRALIZER_SANITY_CHECKS
	static uint32_t interesting = 0xffffffff;
	debugBounds.clear();
#endif

	physx::Array<float> raycastHitTimes;

	for (uint32_t i = 0; i < mTetras.size(); i++)
	{
		if (progress != NULL && (i & 0x7) == 0)
		{
			const int32_t percent = (int32_t)(100 * i / mTetras.size());
			progress->setProgress(percent);
		}
		FullTetrahedron& tetra = mTetras[i];

		if (isFarVertex((uint32_t)tetra.vertexNr[0]) || isFarVertex((uint32_t)tetra.vertexNr[1]) 
			|| isFarVertex((uint32_t)tetra.vertexNr[2]) || isFarVertex((uint32_t)tetra.vertexNr[3]))
		{
			tetra.bDeleted = 1;
		}
		else if (tetra.bDeleted == 0)
		{
			PxVec3 orig, dir;
			orig = mVertices[(uint32_t)tetra.vertexNr[0]].pos;
			orig += mVertices[(uint32_t)tetra.vertexNr[1]].pos;
			orig += mVertices[(uint32_t)tetra.vertexNr[2]].pos;
			orig += mVertices[(uint32_t)tetra.vertexNr[3]].pos;
			orig *= 0.25f;

			uint32_t numInside = 0;	// test 6 standard rays, majority vote
			for (uint32_t j = 0; j < 3; j++)
			{
				PxBounds3 rayBounds(orig, orig);
				switch (j)
				{
				case 0:
				{
					rayBounds.maximum.x = mBound.maximum.x + EPSILON;
					rayBounds.minimum.x = mBound.minimum.x - EPSILON;
					dir = PxVec3(1.0f, 0.0f, 0.0f);
				}
				break;
				case 1:
				{
					rayBounds.maximum.y = mBound.maximum.y + EPSILON;
					rayBounds.minimum.y = mBound.minimum.y - EPSILON;
					dir = PxVec3(0.0f, 1.0f, 0.0f);
				}
				break;
				case 2:
				{
					rayBounds.maximum.z = mBound.maximum.z + EPSILON;
					rayBounds.minimum.z = mBound.minimum.z - EPSILON;
					dir = PxVec3(0.0f, 0.0f, 1.0f);
				}
				break;
				}
				mMeshHash->queryUnique(rayBounds, mTempItemIndices);

				raycastHitTimes.clear();

#if TETRAHEDRALIZER_DEBUG_RENDERING && TETRALIZER_SANITY_CHECKS
				if (i == interesting)
				{
					debugBounds.pushBack(rayBounds.minimum);
					debugBounds.pushBack(rayBounds.maximum);
				}
#endif

				for (uint32_t k = 0; k < mTempItemIndices.size(); k++)
				{
					uint32_t indexNr = mTempItemIndices[k];
					PX_ASSERT(indexNr % 3 == 0);

					const PxVec3 p0 = mVertices[mIndices[indexNr + 0]].pos;
					const PxVec3 p1 = mVertices[mIndices[indexNr + 1]].pos;
					const PxVec3 p2 = mVertices[mIndices[indexNr + 2]].pos;
					PxBounds3 triBounds;
					triBounds.minimum = triBounds.maximum = p0;
					triBounds.include(p1);
					triBounds.include(p2);

					if (!rayBounds.intersects(triBounds))
					{
						continue;
					}

#if TETRAHEDRALIZER_DEBUG_RENDERING && TETRALIZER_SANITY_CHECKS
					if (i == interesting)
					{
						//debugTriangles.pushBack(p0);
						//debugTriangles.pushBack(p1);
						//debugTriangles.pushBack(p2);
						debugBounds.pushBack(triBounds.minimum);
						debugBounds.pushBack(triBounds.maximum);
					}
#endif


					float t, u, v;
					if (!APEX_RayTriangleIntersect(orig, dir, p0, p1, p2, t, u, v))
					{
						continue;
					}

					raycastHitTimes.pushBack(t);
				}

				if (!raycastHitTimes.empty())
				{
					shdfnd::sort(raycastHitTimes.begin(), raycastHitTimes.size(), F32Less());

					uint32_t negative = 0, positive = 0;
#if TETRALIZER_SANITY_CHECKS
					bool report = false;//0 <= i && i < 1033;
#endif
					for (uint32_t k = 0; k < raycastHitTimes.size(); k++)
					{
						if (k > 0 && PxAbs(raycastHitTimes[k] - raycastHitTimes[k - 1]) < EPSILON)
						{
#if TETRALIZER_SANITY_CHECKS
							report = true;
#endif
							// PH: This proved to not be working right, or not making any difference
							//continue;
						}
						if (raycastHitTimes[k] < 0)
						{
							negative++;
						}
						else
						{
							positive++;
						}
					}
					numInside += (positive & 0x1) + (negative & 0x1);
#if TETRAHEDRALIZER_DEBUG_RENDERING && TETRALIZER_SANITY_CHECKS
					if (report)
					{
						float scale = 1.01f;
						debugLines.pushBack(worldRay.orig - worldRay.dir * scale);
						debugLines.pushBack(worldRay.orig + worldRay.dir * scale);
					}
#endif
				}
			}
			if (numInside < 3)
			{
				tetra.bDeleted = 1;
			}
#if TETRAHEDRALIZER_DEBUG_RENDERING && TETRALIZER_SANITY_CHECKS
			if (numInside > 0 && numInside < 6)
			{
				debugTetras.pushBack(mVertices[mTetras[i].vertexNr[0]].pos);
				debugTetras.pushBack(mVertices[mTetras[i].vertexNr[1]].pos);
				debugTetras.pushBack(mVertices[mTetras[i].vertexNr[2]].pos);
				debugTetras.pushBack(mVertices[mTetras[i].vertexNr[3]].pos);
			}
#endif

			// remove degenerated tetrahedra (slivers)
			float quality = getTetraQuality(tetra);
			PX_ASSERT(quality >= 0);
			PX_ASSERT(quality <= 1);
			tetra.quality = (uint32_t)(quality * 1023.0f);
			if (quality < 0.001f)
			{
				tetra.bDeleted = 1;
			}
		}
	}

	return true;
}



void ApexTetrahedralizer::updateCircumSphere(FullTetrahedron& tetra) const
{
	if (tetra.bCircumSphereDirty == 0)
	{
		return;
	}

	const PxVec3 p0 = mVertices[(uint32_t)tetra.vertexNr[0]].pos;
	const PxVec3 b  = mVertices[(uint32_t)tetra.vertexNr[1]].pos - p0;
	const PxVec3 c  = mVertices[(uint32_t)tetra.vertexNr[2]].pos - p0;
	const PxVec3 d  = mVertices[(uint32_t)tetra.vertexNr[3]].pos - p0;

	float det = b.x * (c.y * d.z - c.z * d.y) - b.y * (c.x * d.z - c.z * d.x) + b.z * (c.x * d.y - c.y * d.x);

	if (det == 0.0f)
	{
		tetra.center = p0;
		tetra.radiusSquared = 0.0f;
		return; // singular case
	}

	det *= 2.0f;
	PxVec3 v = c.cross(d) * b.dot(b) + d.cross(b) * c.dot(c) + b.cross(c) * d.dot(d);
	v /= det;

	tetra.radiusSquared = v.magnitudeSquared();
	tetra.center = p0 + v;
	tetra.bCircumSphereDirty = 0;
}



bool ApexTetrahedralizer::pointInCircumSphere(FullTetrahedron& tetra, const PxVec3& p) const
{
	updateCircumSphere(tetra);
	return (tetra.center - p).magnitudeSquared() < tetra.radiusSquared;
}



bool ApexTetrahedralizer::pointInTetra(const FullTetrahedron& tetra, const PxVec3& p) const
{
	const PxVec3 q  = p - mVertices[(uint32_t)tetra.vertexNr[0]].pos;
	const PxVec3 q0 = mVertices[(uint32_t)tetra.vertexNr[1]].pos - mVertices[(uint32_t)tetra.vertexNr[0]].pos;
	const PxVec3 q1 = mVertices[(uint32_t)tetra.vertexNr[2]].pos - mVertices[(uint32_t)tetra.vertexNr[0]].pos;
	const PxVec3 q2 = mVertices[(uint32_t)tetra.vertexNr[3]].pos - mVertices[(uint32_t)tetra.vertexNr[0]].pos;

	PxMat33 m(q0,q1,q2);
	float det = m.getDeterminant();
	m.column0 = q;
	float x = m.getDeterminant();
	m.column0 = q0;
	m.column1 = q;
	float y = m.getDeterminant();
	m.column1 = q1;
	m.column2 = q;
	float z = m.getDeterminant();
	if (det < 0.0f)
	{
		x = -x;
		y = -y;
		z = -z;
		det = -det;
	}

	if (x < 0.0f || y < 0.0f || z < 0.0f)
	{
		return false;
	}

	return (x + y + z < det);
}



float ApexTetrahedralizer::getTetraVolume(const FullTetrahedron& tetra) const
{
	const PxVec3 v0 = mVertices[(uint32_t)tetra.vertexNr[0]].pos;
	const PxVec3 v1 = mVertices[(uint32_t)tetra.vertexNr[1]].pos - v0;
	const PxVec3 v2 = mVertices[(uint32_t)tetra.vertexNr[2]].pos - v0;
	const PxVec3 v3 = mVertices[(uint32_t)tetra.vertexNr[3]].pos - v0;
	return v3.dot(v1.cross(v2)) / 6.0f;
}



float ApexTetrahedralizer::getTetraVolume(int32_t v0, int32_t v1, int32_t v2, int32_t v3) const
{
	FullTetrahedron t;
	t.set(v0, v1, v2, v3);
	return getTetraVolume(t);
}



float ApexTetrahedralizer::getTetraQuality(const FullTetrahedron& tetra) const
{
	const float sqrt2 = 1.4142135623f;

	const float e = getTetraLongestEdge(tetra);
	if (e == 0.0f)
	{
		return 0.0f;
	}

	// for regular tetrahedron vol * 6 * sqrt(2) = s^3 -> quality = 1.0
	return PxAbs(getTetraVolume(tetra)) * 6.0f * sqrt2 / (e * e * e);
}



float ApexTetrahedralizer::getTetraLongestEdge(const FullTetrahedron& tetra) const
{
	const PxVec3& v0 = mVertices[(uint32_t)tetra.vertexNr[0]].pos;
	const PxVec3& v1 = mVertices[(uint32_t)tetra.vertexNr[1]].pos;
	const PxVec3& v2 = mVertices[(uint32_t)tetra.vertexNr[2]].pos;
	const PxVec3& v3 = mVertices[(uint32_t)tetra.vertexNr[3]].pos;
	float max = (v0 - v1).magnitudeSquared();;
	max = PxMax(max, (v0 - v2).magnitudeSquared());
	max = PxMax(max, (v0 - v3).magnitudeSquared());
	max = PxMax(max, (v1 - v2).magnitudeSquared());
	max = PxMax(max, (v1 - v3).magnitudeSquared());
	max = PxMax(max, (v2 - v3).magnitudeSquared());
	return PxSqrt(max);
}



bool ApexTetrahedralizer::triangleContainsVertexNr(uint32_t* triangle, uint32_t* vertexNumber, uint32_t nbVertices)
{
	if (triangle != NULL && vertexNumber != NULL && nbVertices > 0)
	{
		for (uint32_t i = 0; i < nbVertices; i++)
		{
			if (triangle[0] == vertexNumber[i] || triangle[1] == vertexNumber[i] || triangle[2] == vertexNumber[i])
			{
				return true;
			}
		}
	}
	return false;
}



void ApexTetrahedralizer::compressTetrahedra(bool trashNeighbours)
{
	if (trashNeighbours)
	{
		uint32_t i = 0;
		while (i < mTetras.size())
		{
			if (mTetras[i].bDeleted == 1)
			{
				mTetras.replaceWithLast(i);
			}
			else
			{
				mTetras[i].neighborNr[0] = mTetras[i].neighborNr[1] = mTetras[i].neighborNr[2] = mTetras[i].neighborNr[3] = -1;
				i++;
			}
		}
	}
	else
	{
		physx::Array<int32_t> oldToNew(mTetras.size());

		uint32_t i = 0;
		while (i < mTetras.size())
		{
			if (mTetras[i].bDeleted == 1)
			{
				oldToNew[i] = -1;
				if (mTetras[mTetras.size() - 1].bDeleted == 1)
				{
					oldToNew[mTetras.size() - 1] = -1;
				}
				else
				{
					oldToNew[mTetras.size() - 1] = (int32_t)i;
					mTetras[i] = mTetras[mTetras.size() - 1];
					i++;
				}
				mTetras.popBack();
			}
			else
			{
				oldToNew[i] = (int32_t)i;
				i++;
			}
		}

		for (i = 0; i < mTetras.size(); i++)
		{
			FullTetrahedron& t = mTetras[i];
			for (uint32_t j = 0; j < 4; j++)
			{
				if (t.neighborNr[j] >= 0)
				{
					PX_ASSERT((uint32_t)t.neighborNr[j] < oldToNew.size());
					t.neighborNr[j] = oldToNew[(uint32_t)t.neighborNr[j]];
				}
			}
		}
	}
}



void ApexTetrahedralizer::compressVertices()
{
	// remove vertices that are not referenced by any tetrahedra
	physx::Array<int32_t> oldToNew;
	physx::Array<TetraVertex> newVertices;

	mBound.setEmpty();

	oldToNew.resize(mVertices.size(), -1);

	for (uint32_t i = 0; i < mTetras.size(); i++)
	{
		FullTetrahedron& t = mTetras[i];
		for (uint32_t j = 0; j < 4; j++)
		{
			uint32_t vNr = (uint32_t)t.vertexNr[j];
			if (oldToNew[vNr] < 0)
			{
				oldToNew[vNr] = (int32_t)newVertices.size();
				newVertices.pushBack(mVertices[vNr]);
				mBound.include(mVertices[vNr].pos);
			}
			t.vertexNr[j] = oldToNew[vNr];
		}
	}

	mVertices.clear();
	mVertices.resize(newVertices.size());
	for (uint32_t i = 0; i < newVertices.size(); i++)
	{
		mVertices[i] = newVertices[i];
	}
}

}
} // end namespace nvidia::apex

