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



#include "ApexMeshContractor.h"
#include "ApexSDKIntl.h"
#include "PxBounds3.h"
#include "ApexBinaryHeap.h"
#include "ApexSharedUtils.h"

#include "PsSort.h"

namespace nvidia
{
namespace apex
{

#define EDGE_PRESERVATION 1

struct TriangleEdge
{
	void init(uint32_t v0, uint32_t v1, uint32_t edgeNr, uint32_t triNr)
	{
		PX_ASSERT(v0 != v1);
		this->v0 = PxMin(v0, v1);
		this->v1 = PxMax(v0, v1);
		this->edgeNr = edgeNr;
		this->triNr = triNr;
	}
	bool operator < (const TriangleEdge& e) const
	{
		if (v0 < e.v0)
		{
			return true;
		}
		if (v0 > e.v0)
		{
			return false;
		}
		return v1 < e.v1;
	}
	bool operator()(const TriangleEdge& a, const TriangleEdge& b) const
	{
		if (a.v0 < b.v0)
		{
			return true;
		}
		if (a.v0 > b.v0)
		{
			return false;
		}
		return a.v1 < b.v1;
	}
	bool operator == (const TriangleEdge& e) const
	{
		if (v0 == e.v0 && v1 == e.v1)
		{
			return true;
		}
		PX_ASSERT(!(v0 == e.v1 && v1 == e.v0));
		//if (v0 == e.v1 && v1 == e.v0) return true;
		return false;
	}
	uint32_t v0, v1;
	uint32_t edgeNr;
	uint32_t triNr;
};



struct CellCoord
{
	uint32_t xi, yi, zi;
	float value;
	bool operator < (CellCoord& c) const
	{
		return value < c.value;
	}
};



ApexMeshContractor::ApexMeshContractor() :
	mCellSize(1.0f),
	mOrigin(0.0f, 0.0f, 0.0f),
	mNumX(0), mNumY(0), mNumZ(0),
	mInitialVolume(0.0f),
	mCurrentVolume(0.0f)
{
	PX_COMPILE_TIME_ASSERT(sizeof(ContractorCell) == 12);
}



void ApexMeshContractor::registerVertex(const PxVec3& pos)
{
	mVertices.pushBack(pos);
}



void ApexMeshContractor::registerTriangle(uint32_t v0, uint32_t v1, uint32_t v2)
{
	mIndices.pushBack(v0);
	mIndices.pushBack(v1);
	mIndices.pushBack(v2);
}



bool ApexMeshContractor::endRegistration(uint32_t subdivision, IProgressListener* progressListener)
{
	HierarchicalProgressListener progress(100, progressListener);
	progress.setSubtaskWork(30, "Compute Neighbors");
	computeNeighbours();

	PxBounds3 bounds;
	bounds.setEmpty();
	for (unsigned i = 0; i < mVertices.size(); i++)
	{
		bounds.include(mVertices[i]);
	}

	mCellSize = (bounds.maximum - bounds.minimum).magnitude() / subdivision;
	if (mCellSize == 0.0f)
	{
		mCellSize = 1.0f;
	}
	PX_ASSERT(!bounds.isEmpty());
	bounds.fattenFast(2.0f * mCellSize);

	mOrigin = bounds.minimum;
	float invH = 1.0f / mCellSize;
	mNumX = (uint32_t)((bounds.maximum.x - bounds.minimum.x) * invH) + 1;
	mNumY = (uint32_t)((bounds.maximum.y - bounds.minimum.y) * invH) + 1;
	mNumZ = (uint32_t)((bounds.maximum.z - bounds.minimum.z) * invH) + 1;
	unsigned num = mNumX * mNumY * mNumZ;
	mGrid.resize(num, ContractorCell());

	progress.completeSubtask();

	progress.setSubtaskWork(60, "Compute Signed Distance Field");
	computeSignedDistanceField();
	progress.completeSubtask();

	progress.setSubtaskWork(10, "Computing Volume");
	float area = 0;
	computeAreaAndVolume(area, mInitialVolume);
	progress.completeSubtask();

	return true;
}



uint32_t ApexMeshContractor::contract(int32_t steps, float abortionRatio, float& volumeRatio, IProgressListener* progressListener)
{
	if (steps == -1 && (abortionRatio <= 0 || abortionRatio > 1))
	{
		APEX_INTERNAL_ERROR("Invalid abortionRatio when doing infinite steps (%f)", abortionRatio);
		return 0;
	}

	HierarchicalProgressListener progress(100, progressListener);
	progress.setSubtaskWork(100, "Contract");

	bool abort = false;
	int32_t currStep = 0;
	for (; (steps < 0 || currStep < steps) && !abort; currStep++)
	{
		{
			int32_t percent = 100 * currStep / steps;
			progress.setProgress(percent);
		}
		if (!abort)
		{
			contractionStep();
			subdivide(1.5f * mCellSize);
			collapse(0.3f * mCellSize);

			float area;
			computeAreaAndVolume(area, mCurrentVolume);

			volumeRatio = mCurrentVolume / mInitialVolume;
			abort = volumeRatio < abortionRatio;
		}
	}

	progress.completeSubtask();
	return (uint32_t)currStep;
}



void ApexMeshContractor::expandBorder()
{
	const float localRadius = 2.0f * mCellSize;
	//const float minDist = 1.0f * mCellSize;
	float maxDot = 0.0f;

	if (mNeighbours.size() != mIndices.size())
	{
		return;
	}

	const uint32_t numTris = mIndices.size() / 3;
	const uint32_t numVerts = mVertices.size();

	void* memory = PX_ALLOC(sizeof(uint32_t) * numTris + sizeof(PxVec3) * numTris, PX_DEBUG_EXP("ApexMeshContractor"));
	uint32_t* triMarks = (uint32_t*)memory;
	PxVec3* triComs = (PxVec3*)(triMarks + numTris);
	memset(triMarks, -1, sizeof(uint32_t) * numTris);

	physx::Array<PxVec3> dispField;
	dispField.resize(numVerts);

	physx::Array<float> dispWeights;
	dispWeights.resize(numVerts);

	for (uint32_t i = 0; i < numVerts; i++)
	{
		dispField[i] = PxVec3(0.0f);
		dispWeights[i] = 0.0f;
	}

	physx::Array<int32_t> localTris;
	physx::Array<float> localDists;

	for (uint32_t i = 0; i < numTris; i++)
	{
		triComs[i] = PxVec3(0.0f);

		// is there a border edge?
		uint32_t i0 = mIndices[3 * i];
		uint32_t i1 = mIndices[3 * i + 1];
		uint32_t i2 = mIndices[3 * i + 2];

		const PxVec3& p0 = mVertices[i0];
		const PxVec3& p1 = mVertices[i1];
		const PxVec3& p2 = mVertices[i2];

		const PxVec3 ci = (p0 + p1 + p2) / 3.0f;

		PxVec3 ni = (p1 - p0).cross(p2 - p0);
		ni.normalize();

		bool edgeFound = false;
		for (uint32_t j = 0; j < 3; j++)
		{
			int32_t adj = mNeighbours[3 * i + j];
			if (adj < 0)
			{
				continue;
			}

			PxVec3& q0 = mVertices[mIndices[3 * adj + j]];
			PxVec3& q1 = mVertices[mIndices[3 * adj + (j + 1) % 3]];
			PxVec3& q2 = mVertices[mIndices[3 * adj + (j + 2) % 3]];

			PxVec3 nadj = (q1 - q0).cross(q2 - q0);
			nadj.normalize();

			if (ni.dot(nadj) < maxDot)
			{
				edgeFound = true;
				break;
			}
		}

		if (!edgeFound)
		{
			continue;
		}

		collectNeighborhood((int32_t)i, localRadius, i, localTris, localDists, triMarks);
		uint32_t numLocals = localTris.size();

		// is the neighborhood double sided?
		float doubleArea = 0.0f;
		PxVec3 com(0.0f, 0.0f, 0.0f);
		float totalW = 0.0f;

		for (uint32_t j = 0; j < numLocals; j++)
		{
			uint32_t triJ = (uint32_t)localTris[j];
			const PxVec3& p0 = mVertices[mIndices[3 * triJ + 0]];
			const PxVec3& p1 = mVertices[mIndices[3 * triJ + 1]];
			const PxVec3& p2 = mVertices[mIndices[3 * triJ + 2]];

			PxVec3 nj = (p1 - p0).cross(p2 - p0);
			const float area = nj.normalize();
			const float w = area;
			totalW += w;
			com += (p0 + p1 + p2) / 3.0f * w;

			for (uint32_t k = 0; k < numLocals; k++)
			{
				uint32_t triK = (uint32_t)localTris[k];

				const PxVec3& q0 = mVertices[mIndices[3 * triK + 0]];
				const PxVec3& q1 = mVertices[mIndices[3 * triK + 1]];
				const PxVec3& q2 = mVertices[mIndices[3 * triK + 2]];

				PxVec3 nk = (q1 - p0).cross(q2 - q0);
				nk.normalize();

				if (nj.dot(nk) < -0.999f)
				{
					doubleArea += area;
					break;
				}
			}
		}

		float totalArea = PxPi * localRadius * localRadius;
		if (doubleArea < 0.5f * totalArea)
		{
			continue;
		}

		if (totalW > 0.0f)
		{
			com /= totalW;
		}

		triComs[i] = com;

		// update displacement field
		PxVec3 disp = ci - com;
		disp.normalize();
		disp *= 2.0f * mCellSize;

		float minT = findMin(ci, disp);
		disp *= minT;

		float maxDist = 0.0f;
		for (uint32_t j = 0; j < numLocals; j++)
		{
			maxDist = PxMax(maxDist, localDists[j]);
		}

		for (uint32_t j = 0; j < numLocals; j++)
		{
			int32_t triJ = localTris[j];

			float w = 1.0f;
			if (maxDist != 0.0f)
			{
				w = 1.0f - localDists[j] / maxDist;
			}

			for (uint32_t k = 0; k < 3; k++)
			{
				uint32_t v = mIndices[3 * triJ + k];
				dispField[v] += disp * w * w;
				dispWeights[v] += w;
			}
		}
	}

	for (uint32_t i = 0; i < mVertices.size(); i++)
	{
		const float w = dispWeights[i];
		if (w == 0.0f)
		{
			continue;
		}

		mVertices[i] += dispField[i] / w;
	}

	PX_FREE(memory);
}




void ApexMeshContractor::computeNeighbours()
{
	physx::Array<TriangleEdge> edges;
	edges.reserve(mIndices.size());

	mNeighbours.resize(mIndices.size());
	for (uint32_t i = 0; i < mNeighbours.size(); i++)
	{
		mNeighbours[i] = -1;
	}

	PX_ASSERT(mIndices.size() % 3 == 0);
	const uint32_t numTriangles = mIndices.size() / 3;
	for (uint32_t i = 0; i < numTriangles; i++)
	{
		uint32_t i0 = mIndices[3 * i  ];
		uint32_t i1 = mIndices[3 * i + 1];
		uint32_t i2 = mIndices[3 * i + 2];
		TriangleEdge edge;
		edge.init(i0, i1, 0, i);
		edges.pushBack(edge);
		edge.init(i1, i2, 1, i);
		edges.pushBack(edge);
		edge.init(i2, i0, 2, i);
		edges.pushBack(edge);
	}

	shdfnd::sort(edges.begin(), edges.size(), TriangleEdge());

	uint32_t i = 0;
	const uint32_t numEdges = edges.size();
	while (i < numEdges)
	{
		const TriangleEdge& e0 = edges[i];
		i++;
		while (i < numEdges && edges[i] == e0)
		{
			const TriangleEdge& e1 = edges[i];
			PX_ASSERT(mNeighbours[e0.triNr * 3 + e0.edgeNr] == -1);///?
			mNeighbours[e0.triNr * 3 + e0.edgeNr] = (int32_t)e1.triNr;
			PX_ASSERT(mNeighbours[e1.triNr * 3 + e1.edgeNr] == -1);///?
			mNeighbours[e1.triNr * 3 + e1.edgeNr] = (int32_t)e0.triNr;
			i++;
		}
	}
}



void ApexMeshContractor::computeSignedDistanceField()
{
	// init
	for (uint32_t i = 0; i < mGrid.size(); i++)
	{
		PX_ASSERT(mGrid[i].distance == PX_MAX_F32);
		PX_ASSERT(mGrid[i].inside == 0);
	}

	PX_ASSERT(mIndices.size() % 3 == 0);
	uint32_t numTris = mIndices.size() / 3;

	for (uint32_t i = 0; i < numTris; i++)
	{
		PxVec3 p0 = mVertices[mIndices[3 * i  ]] - mOrigin;
		PxVec3 p1 = mVertices[mIndices[3 * i + 1]] - mOrigin;
		PxVec3 p2 = mVertices[mIndices[3 * i + 2]] - mOrigin;
		addTriangle(p0, p1, p2);
	}


	// fast marching, see J.A. Sethian "Level Set Methods and Fast Marching Methods"
	// create front
	ApexBinaryHeap<CellCoord> heap;
	//CellCoord cc;

	//int xi,yi,zi;

	for (uint32_t xi = 0; xi < mNumX - 1; xi++)
	{
		CellCoord cc;
		cc.xi = xi;
		for (uint32_t yi = 0; yi < mNumY - 1; yi++)
		{
			cc.yi = yi;
			for (uint32_t zi = 0; zi < mNumZ - 1; zi++)
			{
				cc.zi = zi;
				const ContractorCell& c = cellAt((int32_t)xi, (int32_t)yi, (int32_t)zi);
				if (!c.marked)
				{
					continue;
				}
				cc.value = c.distance;
				heap.push(cc);
			}
		}
	}

	while (!heap.isEmpty())
	{
		CellCoord cc;

		// Make compiler happy
		cc.xi = cc.yi = cc.zi = 0;
		cc.value = 0.0f;

		heap.pop(cc);

		ContractorCell& c = cellAt((int32_t)cc.xi, (int32_t)cc.yi, (int32_t)cc.zi);
		c.marked = true;
		for (int i = 0; i < 6; i++)
		{
			CellCoord ncc = cc;
			switch (i)
			{
			case 0:
				if (ncc.xi < 1)
				{
					continue;
				}
				else
				{
					ncc.xi--;
				}
				break;
			case 1:
				if (ncc.xi >= mNumX - 1)
				{
					continue;
				}
				else
				{
					ncc.xi++;
				}
				break;
			case 2:
				if (ncc.yi < 1)
				{
					continue;
				}
				else
				{
					ncc.yi--;
				}
				break;
			case 3:
				if (ncc.yi >= mNumY - 1)
				{
					continue;
				}
				else
				{
					ncc.yi++;
				}
				break;
			case 4:
				if (ncc.zi < 1)
				{
					continue;
				}
				else
				{
					ncc.zi--;
				}
				break;
			case 5:
				if (ncc.zi >= mNumZ - 1)
				{
					continue;
				}
				else
				{
					ncc.zi++;
				}
				break;
			}

			ContractorCell& cn = cellAt((int32_t)ncc.xi, (int32_t)ncc.yi, (int32_t)ncc.zi);
			if (cn.marked)
			{
				continue;
			}
			if (!updateDistance(ncc.xi, ncc.yi, ncc.zi))
			{
				continue;
			}
			ncc.value = cn.distance;
			heap.push(ncc);
		}
	}

	setInsideOutside();
	for (unsigned i = 0; i < mGrid.size(); i++)
	{
		if (mGrid[i].inside < 3)	// majority vote
		{
			mGrid[i].distance = -mGrid[i].distance;
		}
	}
}



void ApexMeshContractor::contractionStep()
{
#if EDGE_PRESERVATION
	unsigned numTris = mIndices.size() / 3;
	mVertexCurvatures.resize(mVertices.size());
	for (unsigned i = 0; i < mVertexCurvatures.size(); i++)
	{
		mVertexCurvatures[i] = 0.0f;
	}

	for (unsigned i = 0; i < numTris; i++)
	{
		for (unsigned j = 0; j < 3; j++)
		{
			unsigned vertNr = mIndices[3 * i + j];
			if (mVertexCurvatures[vertNr] == 0.0f)
			{
				float curv = curvatureAt((int32_t)i, (int32_t)vertNr);
				mVertexCurvatures[vertNr] = curv;
			}
		}
	}
#endif

	float s = 0.1f * mCellSize;
	for (unsigned i = 0; i < mVertices.size(); i++)
	{
		PxVec3& p = mVertices[i];
		PxVec3 grad;
		interpolateGradientAt(p, grad);

#if EDGE_PRESERVATION
		p += grad * s  * (1.0f - mVertexCurvatures[i]);
#else
		p += grad * s;
#endif
	}
}



void ApexMeshContractor::computeAreaAndVolume(float& area, float& volume)
{
	area = volume = 0.0f;

	for (uint32_t i = 0; i < mIndices.size(); i += 3)
	{
		PxVec3& d0 = mVertices[mIndices[i]];
		PxVec3 d1 = mVertices[mIndices[i + 1]] - mVertices[mIndices[i]];
		PxVec3 d2 = mVertices[mIndices[i + 2]] - mVertices[mIndices[i]];
		PxVec3 n = d1.cross(d2);
		area += n.magnitude();
		volume += n.dot(d0);
	}

	area /= 2.0f;
	volume /= 6.0f;
}



void ApexMeshContractor::addTriangle(const PxVec3& p0, const PxVec3& p1, const PxVec3& p2)
{
	PxBounds3 bounds;
	bounds.setEmpty();
	bounds.include(p0);
	bounds.include(p1);
	bounds.include(p2);
	PX_ASSERT(!bounds.isEmpty());
	bounds.fattenFast(0.01f * mCellSize);

	PxVec3 n = (p1 - p0).cross(p2 - p0);

	float d_big = n.dot(p0);
	float invH = 1.0f / mCellSize;

	int32_t min[3];
	min[0] = (int32_t)PxFloor(bounds.minimum.x * invH) + 1;
	min[1] = (int32_t)PxFloor(bounds.minimum.y * invH) + 1;
	min[2] = (int32_t)PxFloor(bounds.minimum.z * invH) + 1;
	int32_t max[3];
	max[0] = (int32_t)PxFloor(bounds.maximum.x * invH);
	max[1] = (int32_t)PxFloor(bounds.maximum.y * invH);
	max[2] = (int32_t)PxFloor(bounds.maximum.z * invH);

	if (min[0] < 0)
	{
		min[0] = 0;
	}
	if (min[1] < 0)
	{
		min[1] = 0;
	}
	if (min[2] < 0)
	{
		min[2] = 0;
	}
	if (max[0] >= (int32_t)mNumX)
	{
		max[0] = (int32_t)mNumX - 1;
	}
	if (max[1] >= (int32_t)mNumY)
	{
		max[1] = (int32_t)mNumY - 1;
	}
	if (max[2] >= (int32_t)mNumZ)
	{
		max[2] = (int32_t)mNumZ - 1;
	}

	int32_t num[3] = { (int32_t)mNumX, (int32_t)mNumY, (int32_t)mNumZ };

	int32_t coord[3];

	for (uint32_t dim0 = 0; dim0 < 3; dim0++)
	{
		if (n[dim0] == 0.0f)
		{
			continue;    // singular case
		}

		const uint32_t dim1 = (dim0 + 1) % 3;
		const uint32_t dim2 = (dim1 + 1) % 3;
		for (coord[dim1] = min[dim1]; coord[dim1] <= max[dim1]; coord[dim1]++)
		{
			for (coord[dim2] = min[dim2]; coord[dim2] <= max[dim2]; coord[dim2]++)
			{
				float axis1 = coord[dim1] * mCellSize;
				float axis2 = coord[dim2] * mCellSize;
				/// \todo prevent singular cases when the same spacing was used in previous steps ????
				axis1 += 0.00017f * mCellSize;
				axis2 += 0.00017f * mCellSize;

				// does the ray go through the triangle?
				if (n[dim0] > 0.0f)
				{
					if ((p1[dim1] - p0[dim1]) * (axis2 - p0[dim2]) - (axis1 - p0[dim1]) * (p1[dim2] - p0[dim2]) < 0.0f)
					{
						continue;
					}
					if ((p2[dim1] - p1[dim1]) * (axis2 - p1[dim2]) - (axis1 - p1[dim1]) * (p2[dim2] - p1[dim2]) < 0.0f)
					{
						continue;
					}
					if ((p0[dim1] - p2[dim1]) * (axis2 - p2[dim2]) - (axis1 - p2[dim1]) * (p0[dim2] - p2[dim2]) < 0.0f)
					{
						continue;
					}
				}
				else
				{
					if ((p1[dim1] - p0[dim1]) * (axis2 - p0[dim2]) - (axis1 - p0[dim1]) * (p1[dim2] - p0[dim2]) > 0.0f)
					{
						continue;
					}
					if ((p2[dim1] - p1[dim1]) * (axis2 - p1[dim2]) - (axis1 - p1[dim1]) * (p2[dim2] - p1[dim2]) > 0.0f)
					{
						continue;
					}
					if ((p0[dim1] - p2[dim1]) * (axis2 - p2[dim2]) - (axis1 - p2[dim1]) * (p0[dim2] - p2[dim2]) > 0.0f)
					{
						continue;
					}
				}

				float pos = (d_big - axis1 * n[dim1] - axis2 * n[dim2]) / n[dim0];
				coord[dim0] = (int)PxFloor(pos * invH);
				if (coord[dim0] < 0 || coord[dim0] >= num[dim0])
				{
					continue;
				}

				ContractorCell& cell = cellAt(coord[0], coord[1], coord[2]);
				cell.numCuts[dim0]++;

				float d = PxAbs(pos - coord[dim0] * mCellSize) * invH;
				if (d < cell.distance)
				{
					cell.distance = d;
					cell.marked = true;
				}

				if (coord[dim0] < num[dim0] - 1)
				{
					int adj[3] = { coord[0], coord[1], coord[2] };
					adj[dim0]++;
					ContractorCell& ca = cellAt(adj[0], adj[1], adj[2]);
					d = 1.0f - d;
					if (d < ca.distance)
					{
						ca.distance = d;
						ca.marked = true;
					}
				}
			}
		}
	}
}



bool ApexMeshContractor::updateDistance(uint32_t xi, uint32_t yi, uint32_t zi)
{
	ContractorCell& ci = cellAt((int32_t)xi, (int32_t)yi, (int32_t)zi);
	if (ci.marked)
	{
		return false;
	}

	// create quadratic equation from the stencil
	float a = 0.0f;
	float b = 0.0f;
	float c = -1.0f;
	for (uint32_t i = 0; i < 3; i++)
	{
		float min = PX_MAX_F32;
		int32_t x = (int32_t)xi, y = (int32_t)yi, z = (int32_t)zi;
		switch (i)
		{
		case 0:
			x--;
			break;
		case 1:
			y--;
			break;
		case 2:
			z--;
			break;
		}

		if (x >= 0 && y >= 0 && z >= 0)
		{
			ContractorCell& c = cellAt(x, y, z);
			if (c.marked)
			{
				min = c.distance;
			}
		}

		switch (i)
		{
		case 0:
			x += 2;
			break;
		case 1:
			y += 2;
			break;
		case 2:
			z += 2;
			break;
		}

		if (x < (int32_t)mNumX && y < (int32_t)mNumY && z < (int32_t)mNumZ)
		{
			ContractorCell& c = cellAt(x, y, z);
			if (c.marked && c.distance < min)
			{
				min = c.distance;
			}
		}

		if (min != PX_MAX_F32)
		{
			a += 1.0f;
			b -= 2.0f * min;
			c += min * min;
		}
	}

	// solve the equation, the larger solution is the right one (distances grow as we proceed)
	if (a == 0.0f)
	{
		return false;
	}

	float d = b * b - 4.0f * a * c;

	if (d < 0.0f)
	{
		d = 0.0f;    // debug
	}

	float distance = (-b + PxSqrt(d)) / (2.0f * a);

	if (distance < ci.distance)
	{
		ci.distance = distance;
		return true;
	}

	return false;
}
void ApexMeshContractor::setInsideOutside()
{
	int32_t num[3] = { (int32_t)mNumX, (int32_t)mNumY, (int32_t)mNumZ };

	int32_t coord[3];
	for (uint32_t dim0 = 0; dim0 < 3; dim0++)
	{
		const uint32_t dim1 = (dim0 + 1) % 3;
		const uint32_t dim2 = (dim0 + 2) % 3;
		for (coord[dim1] = 0; coord[dim1] < num[dim1]; coord[dim1]++)
		{
			for (coord[dim2] = 0; coord[dim2] < num[dim2]; coord[dim2]++)
			{
				bool inside = false;
				for (coord[dim0] = 0; coord[dim0] < num[dim0]; coord[dim0]++)
				{
					ContractorCell& cell = cellAt(coord[0], coord[1], coord[2]);
					if (inside)
					{
						cell.inside++;
					}
					if ((cell.numCuts[dim0] % 2) == 1)
					{
						inside = !inside;
					}
				}

				inside = false;
				for (coord[dim0] = num[dim0] - 1; coord[dim0] >= 0; coord[dim0]--)
				{
					ContractorCell& cell = cellAt(coord[0], coord[1], coord[2]);
					if ((cell.numCuts[dim0] % 2) == 1)
					{
						inside = !inside;
					}
					if (inside)
					{
						cell.inside++;
					}
				}
			}
		}
	}
}



void ApexMeshContractor::interpolateGradientAt(const PxVec3& pos, PxVec3& grad)
{
	// the derivatives of the distances are defined on the center of the edges
	// from there they are interpolated trilinearly
	const PxVec3 p = pos - mOrigin;
	const float h = mCellSize;
	const float h1 = 1.0f / h;
	const float h2 = 0.5f * h;
	int32_t x0 = (int32_t)PxFloor(p.x * h1);
	const float dx0 = (p.x - h * x0) * h1;
	const float ex0 = 1.0f - dx0;
	int32_t y0 = (int32_t)PxFloor(p.y * h1);
	const float dy0 = (p.y - h * y0) * h1;
	const float ey0 = 1.0f - dy0;
	int32_t z0 = (int32_t)PxFloor(p.z * h1);
	const float dz0 = (p.z - h * z0) * h1;
	const float ez0 = 1.0f - dz0;

	if (x0 < 0)
	{
		x0 = 0;
	}
	if (x0 + 1 >= (int32_t)mNumX)
	{
		x0 = (int32_t)mNumX - 2;    // todo: handle boundary correctly
	}
	if (y0 < 0)
	{
		y0 = 0;
	}
	if (y0 + 1 >= (int32_t)mNumY)
	{
		y0 = (int32_t)mNumY - 2;
	}
	if (z0 < 0)
	{
		z0 = 0;
	}
	if (z0 + 1 >= (int32_t)mNumZ)
	{
		z0 = (int32_t)mNumZ - 2;
	}

	// the following coefficients are used on the axis of the component that is computed
	// everything is shifted there because the derivatives are defined at the center of edges
	//float dx2,dy2,dz2,ex2,ey2,ez2;
	int32_t x2 = (int32_t)PxFloor((p.x - h2) * h1);
	const float dx2 = (p.x - h2 - h * x2) * h1;
	const float ex2 = 1.0f - dx2;
	int32_t y2 = (int32_t)PxFloor((p.y - h2) * h1);
	const float dy2 = (p.y - h2 - h * y2) * h1;
	const float ey2 = 1.0f - dy2;
	int32_t z2 = (int32_t)PxFloor((p.z - h2) * h1);
	const float dz2 = (p.z - h2 - h * z2) * h1;
	const float ez2 = 1.0f - dz2;

	if (x2 < 0)
	{
		x2 = 0;
	}
	if (x2 + 2 >= (int32_t)mNumX)
	{
		x2 = (int32_t)mNumX - 3;    // todo: handle boundary correctly
	}
	if (y2 < 0)
	{
		y2 = 0;
	}
	if (y2 + 2 >= (int32_t)mNumY)
	{
		y2 = (int32_t)mNumY - 3;
	}
	if (z2 < 0)
	{
		z2 = 0;
	}
	if (z2 + 2 >= (int32_t)mNumZ)
	{
		z2 = (int32_t)mNumZ - 3;
	}

	float d, d0, d1, d2, d3, d4, d5, d6, d7;
	d = cellAt(x2 + 1, y0  , z0).distance;
	d0 = d - cellAt(x2, y0,  z0).distance;
	d4 = cellAt(x2 + 2, y0,  z0).distance - d;
	d = cellAt(x2 + 1, y0 + 1, z0).distance;
	d1 = d - cellAt(x2, y0 + 1, z0).distance;
	d5 = cellAt(x2 + 2, y0 + 1, z0).distance - d;
	d = cellAt(x2 + 1, y0 + 1, z0 + 1).distance;
	d2 = d - cellAt(x2, y0 + 1, z0 + 1).distance;
	d6 = cellAt(x2 + 2, y0 + 1, z0 + 1).distance - d;
	d = cellAt(x2 + 1, y0,  z0 + 1).distance;
	d3 = d - cellAt(x2, y0,  z0 + 1).distance;
	d7 = cellAt(x2 + 2, y0,  z0 + 1).distance - d;

	grad.x = ex2 * (d0 * ey0 * ez0 + d1 * dy0 * ez0 + d2 * dy0 * dz0 + d3 * ey0 * dz0)
	         + dx2 * (d4 * ey0 * ez0 + d5 * dy0 * ez0 + d6 * dy0 * dz0 + d7 * ey0 * dz0);

	d = cellAt(x0,  y2 + 1, z0).distance;
	d0 = d - cellAt(x0,  y2, z0).distance;
	d4 = cellAt(x0,  y2 + 2, z0).distance - d;
	d = cellAt(x0,  y2 + 1, z0 + 1).distance;
	d1 = d - cellAt(x0,  y2, z0 + 1).distance;
	d5 = cellAt(x0,  y2 + 2, z0 + 1).distance - d;
	d = cellAt(x0 + 1, y2 + 1, z0 + 1).distance;
	d2 = d - cellAt(x0 + 1, y2, z0 + 1).distance;
	d6 = cellAt(x0 + 1, y2 + 2, z0 + 1).distance - d;
	d = cellAt(x0 + 1, y2 + 1, z0).distance;
	d3 = d - cellAt(x0 + 1, y2, z0).distance;
	d7 = cellAt(x0 + 1, y2 + 2, z0).distance - d;

	grad.y = ey2 * (d0 * ez0 * ex0 + d1 * dz0 * ex0 + d2 * dz0 * dx0 + d3 * ez0 * dx0)
	         + dy2 * (d4 * ez0 * ex0 + d5 * dz0 * ex0 + d6 * dz0 * dx0 + d7 * ez0 * dx0);

	d = cellAt(x0,  y0  , z2 + 1).distance;
	d0 = d - cellAt(x0,  y0  , z2).distance;
	d4 = cellAt(x0,  y0  , z2 + 2).distance - d;
	d = cellAt(x0 + 1, y0,  z2 + 1).distance;
	d1 = d - cellAt(x0 + 1, y0,  z2).distance;
	d5 = cellAt(x0 + 1, y0,  z2 + 2).distance - d;
	d = cellAt(x0 + 1, y0 + 1, z2 + 1).distance;
	d2 = d - cellAt(x0 + 1, y0 + 1, z2).distance;
	d6 = cellAt(x0 + 1, y0 + 1, z2 + 2).distance - d;
	d = cellAt(x0,  y0 + 1, z2 + 1).distance;
	d3 = d - cellAt(x0,  y0 + 1, z2).distance;
	d7 = cellAt(x0,  y0 + 1, z2 + 2).distance - d;

	grad.z = ez2 * (d0 * ex0 * ey0 + d1 * dx0 * ey0 + d2 * dx0 * dy0 + d3 * ex0 * dy0)
	         + dz2 * (d4 * ex0 * ey0 + d5 * dx0 * ey0 + d6 * dx0 * dy0 + d7 * ex0 * dy0);
}



void ApexMeshContractor::subdivide(float spacing)
{
	const float min2 = spacing * spacing;
	const uint32_t numTris = mIndices.size() / 3;
	//const uint32_t numVerts = mVertices.size();

	for (uint32_t i = 0; i < numTris; i++)
	{
		const uint32_t triNr = i;
		for (uint32_t j = 0; j < 3; j++)
		{
			uint32_t k = (j + 1) % 3;
			uint32_t v0 = mIndices[3 * triNr + j];
			uint32_t v1 = mIndices[3 * triNr + k];

			PxVec3& p0 = mVertices[v0];
			PxVec3& p1 = mVertices[v1];
			if ((p1 - p0).magnitudeSquared() < min2)
			{
				continue;
			}

			uint32_t newVert = mVertices.size();
			mVertices.pushBack((p1 + p0) * 0.5f);

			int32_t adj;
			int32_t t0, t1, t2, t3;
			getButterfly(triNr, v0, v1, adj, t0, t1, t2, t3);

			uint32_t newTri  = mIndices.size() / 3;
			int newAdj  = -1;

			int v = getOppositeVertex((int32_t)triNr, v0, v1);
			PX_ASSERT(v >= 0);
			mIndices.pushBack(newVert);
			mIndices.pushBack(v1);
			mIndices.pushBack((uint32_t)v);
			if (adj >= 0)
			{
				v = getOppositeVertex(adj, v0, v1);
				PX_ASSERT(v >= 0);
				newAdj = (int32_t)mIndices.size() / 3;
				mIndices.pushBack(newVert);
				mIndices.pushBack((uint32_t)v);
				mIndices.pushBack(v1);
			}

			replaceVertex((int32_t)triNr, v1, newVert);
			replaceVertex(adj, v1, newVert);

			replaceNeighbor((int32_t)triNr, t1, newTri);
			replaceNeighbor(adj, t3, (uint32_t)newAdj);
			replaceNeighbor(t1, (int32_t)triNr, newTri);
			replaceNeighbor(t3, adj, (uint32_t)newAdj);

			mNeighbours.pushBack(newAdj);
			mNeighbours.pushBack(t1);
			mNeighbours.pushBack((int32_t)triNr);
			if (adj >= 0)
			{
				mNeighbours.pushBack(adj);
				mNeighbours.pushBack(t3);
				mNeighbours.pushBack((int32_t)newTri);
			}
		}
	}
}



void ApexMeshContractor::collapse(float spacing)
{
	const float min2 = spacing * spacing;
	const uint32_t numTris = mIndices.size() / 3;
	const uint32_t numVerts = mVertices.size();

	const uint32_t size = sizeof(int32_t) * numVerts + sizeof(int32_t) * numTris;
	void* memory = PX_ALLOC(size, PX_DEBUG_EXP("ApexMeshContractor"));
	memset(memory, 0, size);

	int32_t* old2newVerts = (int32_t*)memory;
	int32_t* old2newTris = old2newVerts + numVerts;

	// collapse edges
	for (uint32_t i = 0; i < numTris; i++)
	{
		const uint32_t triNr = i;

		if (old2newTris[triNr] < 0)
		{
			continue;
		}

		for (uint32_t j = 0; j < 3; j++)
		{
			const uint32_t k = (j + 1) % 3;
			const uint32_t v0 = mIndices[3 * triNr + j];
			const uint32_t v1 = mIndices[3 * triNr + k];

			PxVec3& p0 = mVertices[v0];
			PxVec3& p1 = mVertices[v1];
			if ((p1 - p0).magnitudeSquared() > min2)
			{
				continue;
			}

			//if (iterNr == 14 && triNr == 388 && j == 0) {
			//	int foo = 0;
			//	checkConsistency();
			//}

			if (!legalCollapse((int32_t)triNr, v0, v1))
			{
				continue;
			}

			int32_t adj, t0, t1, t2, t3;
			getButterfly(triNr, v0, v1, adj, t0, t1, t2, t3);

			mVertices[v0] = (p1 + p0) * 0.5f;

			int32_t prev = (int32_t)triNr;
			int32_t t = t1;
			while (t >= 0)
			{
				replaceVertex(t, v1, v0);
				advanceAdjTriangle(v1, t, prev);
				if (t == (int32_t)triNr)
				{
					break;
				}
			}

			for (uint32_t k = 0; k < 3; k++)
			{
				if (t0 >= 0 && mNeighbours[3 * (uint32_t)t0 + k] == (int32_t)triNr)
				{
					mNeighbours[3 * (uint32_t)t0 + k] = t1;
				}
				if (t1 >= 0 && mNeighbours[3 * (uint32_t)t1 + k] == (int32_t)triNr)
				{
					mNeighbours[3 * (uint32_t)t1 + k] = t0;
				}
				if (t2 >= 0 && mNeighbours[3 * (uint32_t)t2 + k] == adj)
				{
					mNeighbours[3 * t2 + k] = t3;
				}
				if (t3 >= 0 && mNeighbours[3 * (uint32_t)t3 + k] == adj)
				{
					mNeighbours[3 * (uint32_t)t3 + k] = t2;
				}
			}

			old2newVerts[v1] = -1;
			old2newTris[triNr] = -1;
			if (adj >= 0)
			{
				old2newTris[adj] = -1;
			}

			//checkConsistency();

			break;
		}
	}

	// compress mesh
	uint32_t vertNr = 0;
	for (uint32_t i = 0; i < numVerts; i++)
	{
		if (old2newVerts[i] != -1)
		{
			old2newVerts[i] = (int32_t)vertNr;
			mVertices[vertNr] = mVertices[i];
			vertNr++;
		}
	}
	mVertices.resize(vertNr);

	uint32_t triNr = 0;
	for (uint32_t i = 0; i < numTris; i++)
	{
		if (old2newTris[i] != -1)
		{
			old2newTris[i] = (int32_t)triNr;
			for (uint32_t j = 0; j < 3; j++)
			{
				mIndices[3 * triNr + j] = (uint32_t)old2newVerts[mIndices[3 * i + j]];
				mNeighbours[3 * triNr + j] = mNeighbours[3 * i + j];
			}
			triNr++;
		}
	}
	mIndices.resize(triNr * 3);
	mNeighbours.resize(triNr * 3);
	for (uint32_t i = 0; i < mNeighbours.size(); i++)
	{
		PX_ASSERT(mNeighbours[i] != -1);
		PX_ASSERT(old2newTris[mNeighbours[i]] != -1);
		mNeighbours[i] = old2newTris[mNeighbours[i]];
	}

	PX_FREE(memory);
}



void ApexMeshContractor::getButterfly(uint32_t triNr, uint32_t v0, uint32_t v1, int32_t& adj, int32_t& t0, int32_t& t1, int32_t& t2, int32_t& t3) const
{
	t0 = -1;
	t1 = -1;
	t2 = -1;
	t3 = -1;
	adj = -1;
	for (uint32_t i = 0; i < 3; i++)
	{
		int32_t n = mNeighbours[triNr * 3 + i];

		if (n < 0)
		{
			continue;
		}

		if (triangleContains(n, v0) && triangleContains(n, v1))
		{
			adj = n;
		}
		else if (triangleContains(n, v0))
		{
			t0 = n;
		}
		else if (triangleContains(n, v1))
		{
			t1 = n;
		}
	}
	if (adj >= 0)
	{
		for (uint32_t i = 0; i < 3; i++)
		{
			int32_t n = mNeighbours[adj * 3 + i];

			if (n < 0 || (uint32_t)n == triNr)
			{
				continue;
			}

			if (triangleContains(n, v0))
			{
				t2 = n;
			}
			else if (triangleContains(n, v1))
			{
				t3 = n;
			}
		}
	}
}



int32_t ApexMeshContractor::getOppositeVertex(int32_t t, uint32_t v0, uint32_t v1) const
{
	if (t < 0 || 3 * t + 2 >= (int32_t)mIndices.size())
	{
		return -1;
	}
	const uint32_t i = 3 * (uint32_t)t;
	if (mIndices[i + 0] != v0 && mIndices[i + 0] != v1)
	{
		return (int32_t)mIndices[i + 0];
	}
	if (mIndices[i + 1] != v0 && mIndices[i + 1] != v1)
	{
		return (int32_t)mIndices[i + 1];
	}
	if (mIndices[i + 2] != v0 && mIndices[i + 2] != v1)
	{
		return (int32_t)mIndices[i + 2];
	}
	return -1;
}



void ApexMeshContractor::replaceVertex(int32_t t, uint32_t vOld, uint32_t vNew)
{
	if (t < 0 || 3 * t + 2 >= (int32_t)mIndices.size())
	{
		return;
	}
	const uint32_t i = 3 * (uint32_t)t;
	if (mIndices[i + 0] == vOld)
	{
		mIndices[i + 0] = vNew;
	}
	if (mIndices[i + 1] == vOld)
	{
		mIndices[i + 1] = vNew;
	}
	if (mIndices[i + 2] == vOld)
	{
		mIndices[i + 2] = vNew;
	}
}



void ApexMeshContractor::replaceNeighbor(int32_t t, int32_t nOld, uint32_t nNew)
{
	if (t < 0 || 3 * t + 2 >= (int32_t)mNeighbours.size())
	{
		return;
	}
	const uint32_t i = 3 * (uint32_t)t;
	if (mNeighbours[i + 0] == nOld)
	{
		mNeighbours[i + 0] = (int32_t)nNew;
	}
	if (mNeighbours[i + 1] == nOld)
	{
		mNeighbours[i + 1] = (int32_t)nNew;
	}
	if (mNeighbours[i + 2] == nOld)
	{
		mNeighbours[i + 2] = (int32_t)nNew;
	}
}


bool ApexMeshContractor::triangleContains(int32_t t, uint32_t v) const
{
	if (t < 0 || 3 * t + 2 >= (int32_t)mIndices.size())
	{
		return false;
	}
	const uint32_t i = 3 * (uint32_t)t;
	return mIndices[i] == v || mIndices[i + 1] == v || mIndices[i + 2] == v;
}



bool ApexMeshContractor::legalCollapse(int32_t triNr, uint32_t v0, uint32_t v1) const
{
	int32_t adj, t0, t1, t2, t3;
	getButterfly((uint32_t)triNr, v0, v1, adj, t0, t1, t2, t3);

	// both of the end vertices must be completely surrounded by triangles
	int32_t prev = triNr;
	int32_t t = t0;
	while (t >= 0)
	{
		advanceAdjTriangle(v0, t, prev);
		if (t == triNr)
		{
			break;
		}
	}

	if (t != triNr)
	{
		return false;
	}

	prev = triNr;
	t = t1;
	while (t >= 0)
	{
		advanceAdjTriangle(v1, t, prev);
		if (t == triNr)
		{
			break;
		}
	}
	if (t != triNr)
	{
		return false;
	}

	// not legal if there exists v != v0,v1 with (v0,v) and (v1,v) edges
	// but (v,v0,v1) is not a triangle

	prev = triNr;
	t = t1;
	int adjV = getOppositeVertex(triNr, v0, v1);
	while (t >= 0)
	{
		if (t != t1)
		{
			int prev2 = prev;
			int t2 = t;
			while (t2 >= 0)
			{
				if (triangleContains(t2, v0))
				{
					return false;
				}
				advanceAdjTriangle((uint32_t)adjV, t2, prev2);
				if (t2 == t)
				{
					break;
				}
			}
		}
		adjV = getOppositeVertex(t, v1, (uint32_t)adjV);
		advanceAdjTriangle(v1, t, prev);
		if (t == triNr || t == adj)
		{
			break;
		}
	}

	if (areNeighbors(t0, t1))
	{
		return false;
	}
	if (areNeighbors(t2, t3))
	{
		return false;
	}
	return true;
}



void ApexMeshContractor::advanceAdjTriangle(uint32_t v, int32_t& t, int32_t& prev) const
{
	int32_t next = -1;
	for (uint32_t i = 0; i < 3; i++)
	{
		int32_t n = mNeighbours[3 * t + i];
		if (n >= 0 && n != prev && triangleContains(n, v))
		{
			next = n;
		}
	}
	prev = t;
	t = next;
}



bool ApexMeshContractor::areNeighbors(int32_t t0, int32_t t1) const
{
	if (t0 < 0 || 3 * t0 + 2 >= (int32_t)mNeighbours.size())
	{
		return false;
	}
	const uint32_t i = 3 * (uint32_t)t0;
	return mNeighbours[i] == t1 || mNeighbours[i + 1] == t1 || mNeighbours[i + 2] == t1;
}



float ApexMeshContractor::findMin(const PxVec3& p, const PxVec3& maxDisp) const
{
	const uint32_t numSteps = 20;
	const float dt = 1.0f / numSteps;

	float minDist = PxAbs(interpolateDistanceAt(p));
	float minT = 0.0f;

	for (uint32_t i = 1; i < numSteps; i++)
	{
		const float t = i * dt;
		float dist = PxAbs(interpolateDistanceAt(p + maxDisp * t));
		if (dist < minDist)
		{
			minT = t;
			// PH: isn't this missing?
			//minDist = dist;
		}
	}
	return minT;
}



float ApexMeshContractor::interpolateDistanceAt(const PxVec3& pos) const
{
	const PxVec3 p = pos - mOrigin;
	const float h = mCellSize;
	const float h1 = 1.0f / h;

	int32_t x = (int32_t)PxFloor(p.x * h1);
	const float dx = (p.x - h * x) * h1;
	const float ex = 1.0f - dx;
	int32_t y = (int32_t)PxFloor(p.y * h1);
	const float dy = (p.y - h * y) * h1;
	const float ey = 1.0f - dy;
	int32_t z = (int32_t)PxFloor(p.z * h1);
	const float dz = (p.z - h * z) * h1;
	const float ez = 1.0f - dz;

	if (x < 0)
	{
		x = 0;
	}
	if (x + 1 >= (int32_t)mNumX)
	{
		x = (int32_t)mNumX - 2;    // todo: handle boundary correctly
	}
	if (y < 0)
	{
		y = 0;
	}
	if (y + 1 >= (int32_t)mNumY)
	{
		y = (int32_t)mNumY - 2;
	}
	if (z < 0)
	{
		z = 0;
	}
	if (z + 1 >= (int32_t)mNumZ)
	{
		z = (int32_t)mNumZ - 2;
	}

	float d0, d1, d2, d3, d4, d5, d6, d7;
	d0 = constCellAt(x, y,  z).distance;
	d4 = constCellAt(x + 1, y,  z).distance;
	d1 = constCellAt(x, y + 1, z).distance;
	d5 = constCellAt(x + 1, y + 1, z).distance;
	d2 = constCellAt(x, y + 1, z + 1).distance;
	d6 = constCellAt(x + 1, y + 1, z + 1).distance;
	d3 = constCellAt(x, y,  z + 1).distance;
	d7 = constCellAt(x + 1, y,  z + 1).distance;

	float dist = ex * (d0 * ey * ez + d1 * dy * ez + d2 * dy * dz + d3 * ey * dz)
	                    + dx * (d4 * ey * ez + d5 * dy * ez + d6 * dy * dz + d7 * ey * dz);

	return dist;
}




struct DistTriangle
{
	int32_t triNr;
	float dist;
	bool operator < (const DistTriangle& t) const
	{
		return dist > t.dist;
	}
};

void ApexMeshContractor::collectNeighborhood(int32_t triNr, float radius, uint32_t newMark, physx::Array<int32_t> &tris, physx::Array<float> &dists, uint32_t* triMarks) const
{
	tris.clear();
	dists.clear();
	ApexBinaryHeap<DistTriangle> heap;

	DistTriangle dt;
	dt.triNr = triNr;
	dt.dist = 0.0f;
	heap.push(dt);

	while (!heap.isEmpty())
	{
		heap.pop(dt);

		if (triMarks[dt.triNr] == newMark)
		{
			continue;
		}
		triMarks[dt.triNr] = newMark;

		tris.pushBack(dt.triNr);
		dists.pushBack(-dt.dist);
		PxVec3 center;
		getTriangleCenter(dt.triNr, center);

		for (uint32_t i = 0; i < 3; i++)
		{
			int32_t adj = mNeighbours[3 * dt.triNr + i];
			if (adj < 0)
			{
				continue;
			}
			if (triMarks[adj] == newMark)
			{
				continue;
			}

			PxVec3 adjCenter;
			getTriangleCenter(adj, adjCenter);
			DistTriangle adjDt;
			adjDt.triNr = adj;
			adjDt.dist = dt.dist - (adjCenter - center).magnitude();	// it is a max heap, we need the smallest dist first
			//adjDt.dist = adjCenter.distance(center) - dt.dist;	// PH: inverting heap distance, now it's a min heap
			if (-adjDt.dist > radius)
			{
				continue;
			}

			heap.push(adjDt);
		}
	}
}



void ApexMeshContractor::getTriangleCenter(int32_t triNr, PxVec3& center) const
{
	const PxVec3& p0 = mVertices[mIndices[3 * (uint32_t)triNr + 0]];
	const PxVec3& p1 = mVertices[mIndices[3 * (uint32_t)triNr + 1]];
	const PxVec3& p2 = mVertices[mIndices[3 * (uint32_t)triNr + 2]];
	center = (p0 + p1 + p2) / 3.0f;
}

//------------------------------------------------------------------------------------
float ApexMeshContractor::curvatureAt(int triNr, int v)
{
	int prev = -1;
	int t = triNr;
	PxVec3 n0, n1;
	float minDot = 1.0f;
	while (t >= 0)
	{
		advanceAdjTriangle((uint32_t)v, t, prev);
		if (t < 0 || t == triNr)
		{
			break;
		}

		PxVec3& p0 = mVertices[mIndices[3 * (uint32_t)prev]];
		PxVec3& p1 = mVertices[mIndices[3 * (uint32_t)prev + 1]];
		PxVec3& p2 = mVertices[mIndices[3 * (uint32_t)prev + 2]];
		n0 = (p1 - p0).cross(p2 - p0);
		n0.normalize();

		PxVec3& q0 = mVertices[mIndices[3 * (uint32_t)t]];
		PxVec3& q1 = mVertices[mIndices[3 * (uint32_t)t + 1]];
		PxVec3& q2 = mVertices[mIndices[3 * (uint32_t)t + 2]];
		n1 = (q1 - q0).cross(q2 - q0);
		n1.normalize();
		float dot = n0.dot(n1);
		if (dot < minDot)
		{
			minDot = dot;
		}
	}
	return (1.0f - minDot) * 0.5f;
}

}
} // end namespace nvidia::apex
