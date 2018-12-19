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



#include "ApexIsoMesh.h"
#include "ApexCollision.h"
#include "ApexMarchingCubes.h"
#include "ApexSharedUtils.h"

#include "ApexSDKIntl.h"

#include "PsSort.h"

namespace nvidia
{
namespace apex
{

ApexIsoMesh::ApexIsoMesh(uint32_t isoGridSubdivision, uint32_t keepNBiggestMeshes, bool discardInnerMeshes) :
	mIsoGridSubdivision(isoGridSubdivision),
	mKeepNBiggestMeshes(keepNBiggestMeshes),
	mDiscardInnerMeshes(discardInnerMeshes),
	mCellSize(0.0f),
	mThickness(0.0f),
	mOrigin(0.0f, 0.0f, 0.0f),
	mNumX(0), mNumY(0), mNumZ(0),
	mIsoValue(0.5f)
{
	if (mIsoGridSubdivision == 0)
	{
		APEX_INVALID_PARAMETER("isoGridSubdivision must be bigger than 0, setting it to 10");
		mIsoGridSubdivision = 10;
	}
	mBound.setEmpty();
}



ApexIsoMesh::~ApexIsoMesh()
{
}



void ApexIsoMesh::setBound(const PxBounds3& bound)
{
	mBound = bound;
	mCellSize = (mBound.maximum - mBound.minimum).magnitude() / mIsoGridSubdivision;
	if (mCellSize == 0.0f)
	{
		mCellSize = 1.0f;
	}
	mThickness = mCellSize * 1.5f;
	PX_ASSERT(!mBound.isEmpty());
	mBound.fattenFast(2.0f * mThickness);

	mOrigin = mBound.minimum;
	float invH = 1.0f / mCellSize;
	mNumX = (int32_t)((mBound.maximum.x - mBound.minimum.x) * invH) + 1;
	mNumY = (int32_t)((mBound.maximum.y - mBound.minimum.y) * invH) + 1;
	mNumZ = (int32_t)((mBound.maximum.z - mBound.minimum.z) * invH) + 1;
}



void ApexIsoMesh::clear()
{
	mCellSize = 0.0f;
	mThickness = 0.0f;
	mOrigin = PxVec3(0.0f);
	mNumX = mNumY = mNumZ = 0;
	mBound.setEmpty();

	clearTemp();

	mIsoVertices.clear();
	mIsoTriangles.clear();
	mIsoEdges.clear();
}



void ApexIsoMesh::clearTemp()
{
	mGrid.clear();
	mGrid.reset();
}



void ApexIsoMesh::addTriangle(const PxVec3& v0, const PxVec3& v1, const PxVec3& v2)
{
	int32_t num = mNumX * mNumY * mNumZ;
	if (mGrid.size() != (uint32_t) num)
	{
		IsoCell cell;
		cell.init();
		mGrid.resize((uint32_t)num, cell);
	}

	PX_ASSERT(mThickness != 0.0f);

	PxBounds3 bounds;
	bounds.minimum = bounds.maximum = v0;
	bounds.include(v1);
	bounds.include(v2);
	PX_ASSERT(!bounds.isEmpty());
	bounds.fattenFast(mThickness);

	float h = mCellSize;
	float invH = 1.0f / h;
	float invT = 1.0f / mThickness;

	int32_t x0 = PxMax(0, (int32_t)((bounds.minimum.x - mOrigin.x) * invH));
	int32_t x1 = PxMin(mNumX - 1, (int32_t)((bounds.maximum.x - mOrigin.x) * invH));
	int32_t y0 = PxMax(0, (int32_t)((bounds.minimum.y - mOrigin.y) * invH));
	int32_t y1 = PxMin(mNumY - 1, (int32_t)((bounds.maximum.y - mOrigin.y) * invH));
	int32_t z0 = PxMax(0, (int32_t)((bounds.minimum.z - mOrigin.z) * invH));
	int32_t z1 = PxMin(mNumZ - 1, (int32_t)((bounds.maximum.z - mOrigin.z) * invH));

	for (int32_t xi = x0; xi <= x1; xi++)
	{
		PxVec3 pos;
		pos.x = mOrigin.x + h * xi;
		for (int32_t yi = y0; yi <= y1; yi++)
		{
			pos.y = mOrigin.y + h * yi;
			for (int32_t zi = z0; zi <= z1; zi++)
			{
				pos.z = mOrigin.z + h * zi;
				Triangle triangle(v0, v1, v2);
				float dist = PxSqrt(APEX_pointTriangleSqrDst(triangle, pos));
				if (dist > mThickness)
				{
					continue;
				}

				IsoCell& cell = cellAt(xi, yi, zi);
				float density = 1.0f - dist * invT;
				PX_ASSERT(density >= 0);
				cell.density = PxMax(cell.density, density); ///< \todo, is this right?
			}
		}
	}
}



bool ApexIsoMesh::update(IProgressListener* progressListener)
{
	HierarchicalProgressListener progress(100, progressListener);
	progress.setSubtaskWork(90, "Generating first mesh");

	if (generateMesh(&progress))
	{

		progress.completeSubtask();
		progress.setSubtaskWork(5, "Finding Neighbors");

		if (findNeighbors(&progress))
		{

			if (mKeepNBiggestMeshes > 0 || mDiscardInnerMeshes)
			{
				progress.completeSubtask();
				progress.setSubtaskWork(3, "Removing layers");
				removeLayers();
				//	removeSide(2);
			}
			progress.completeSubtask();
			progress.setSubtaskWork(2, "Removing Triangles and Vertices");

			removeTrisAndVerts();
		}
	}
	progress.completeSubtask();

	return true;
}



void ApexIsoMesh::getTriangle(uint32_t index, uint32_t& v0, uint32_t& v1, uint32_t& v2) const
{
	PX_ASSERT(index < mIsoTriangles.size());
	const IsoTriangle& t = mIsoTriangles[index];
	v0 = (uint32_t)t.vertexNr[0];
	v1 = (uint32_t)t.vertexNr[1];
	v2 = (uint32_t)t.vertexNr[2];
}



bool ApexIsoMesh::generateMesh(IProgressListener* progressListener)
{
	mIsoVertices.clear();

	int xi, yi, zi;
	float h = mCellSize;
	PxVec3 pos, vPos;

	uint32_t maximum = (uint32_t)(mNumX * mNumY * mNumZ);
	uint32_t progressCounter = 0;

	HierarchicalProgressListener progress(100, progressListener);
	progress.setSubtaskWork(80, "Generating Vertices");

	// generate vertices
	for (xi = 0; xi < mNumX; xi++)
	{
		pos.x = mOrigin.x + h * xi;
		for (yi = 0; yi < mNumY; yi++)
		{
			pos.y = mOrigin.y + h * yi;
			for (zi = 0; zi < mNumZ; zi++)
			{

				if ((progressCounter++ & 0xff) == 0)
				{
					const int32_t curr = ((xi * mNumY) + yi) * mNumZ + zi;
					const int32_t percent = 100 * curr / (int32_t)maximum;
					progress.setProgress(percent);
				}

				pos.z = mOrigin.z + h * zi;
				IsoCell& cell = cellAt(xi, yi, zi);
				float d  = cell.density;
				if (xi < mNumX - 1)
				{
					float dx = cellAt(xi + 1, yi, zi).density;
					if (interpolate(d, dx, pos, pos + PxVec3(h, 0.0f, 0.0f), vPos))
					{
						cell.vertNrX = (int32_t)mIsoVertices.size();
						mIsoVertices.pushBack(vPos);
					}
				}
				if (yi < mNumY - 1)
				{
					float dy = cellAt(xi, yi + 1, zi).density;
					if (interpolate(d, dy, pos, pos + PxVec3(0.0f, h, 0.0f), vPos))
					{
						cell.vertNrY = (int32_t)mIsoVertices.size();
						mIsoVertices.pushBack(vPos);
					}
				}
				if (zi < mNumZ - 1)
				{
					float dz = cellAt(xi, yi, zi + 1).density;
					if (interpolate(d, dz, pos, pos + PxVec3(0.0f, 0.0f, h), vPos))
					{
						cell.vertNrZ = (int32_t)mIsoVertices.size();
						mIsoVertices.pushBack(vPos);
					}
				}
			}
		}
	}

	progress.completeSubtask();
	progress.setSubtaskWork(20, "Generating Faces");
	progressCounter = 0;

	mIsoTriangles.clear();
	// generate triangles
	int edges[12];
	IsoTriangle triangle;

	for (xi = 0; xi < mNumX - 1; xi++)
	{
		pos.x = mOrigin.x + h * xi;
		for (yi = 0; yi < mNumY - 1; yi++)
		{
			pos.y = mOrigin.y + h * yi;
			for (zi = 0; zi < mNumZ - 1; zi++)
			{

				if ((progressCounter++ & 0xff) == 0)
				{
					const int32_t curr = ((xi * mNumY) + yi) * mNumZ + zi;
					const int32_t percent = 100 * curr / (int32_t)maximum;
					progress.setProgress(percent);
				}
				pos.z = mOrigin.z + h * zi;
				int code = 0;
				if (cellAt(xi,  yi,  zi).density > mIsoValue)
				{
					code |= 1;
				}
				if (cellAt(xi + 1, yi,  zi).density > mIsoValue)
				{
					code |= 2;
				}
				if (cellAt(xi + 1, yi + 1, zi).density > mIsoValue)
				{
					code |= 4;
				}
				if (cellAt(xi,  yi + 1, zi).density > mIsoValue)
				{
					code |= 8;
				}
				if (cellAt(xi,  yi,  zi + 1).density > mIsoValue)
				{
					code |= 16;
				}
				if (cellAt(xi + 1, yi,  zi + 1).density > mIsoValue)
				{
					code |= 32;
				}
				if (cellAt(xi + 1, yi + 1, zi + 1).density > mIsoValue)
				{
					code |= 64;
				}
				if (cellAt(xi,  yi + 1, zi + 1).density > mIsoValue)
				{
					code |= 128;
				}
				if (code == 0 || code == 255)
				{
					continue;
				}

				edges[ 0] = cellAt(xi,  yi,  zi).vertNrX;
				edges[ 1] = cellAt(xi + 1, yi,  zi).vertNrY;
				edges[ 2] = cellAt(xi,  yi + 1, zi).vertNrX;
				edges[ 3] = cellAt(xi,  yi,  zi).vertNrY;

				edges[ 4] = cellAt(xi,  yi,  zi + 1).vertNrX;
				edges[ 5] = cellAt(xi + 1, yi,  zi + 1).vertNrY;
				edges[ 6] = cellAt(xi,  yi + 1, zi + 1).vertNrX;
				edges[ 7] = cellAt(xi,  yi,  zi + 1).vertNrY;

				edges[ 8] = cellAt(xi,  yi,  zi).vertNrZ;
				edges[ 9] = cellAt(xi + 1, yi,  zi).vertNrZ;
				edges[10] = cellAt(xi + 1, yi + 1, zi).vertNrZ;
				edges[11] = cellAt(xi,  yi + 1, zi).vertNrZ;

				IsoCell& c = cellAt(xi, yi, zi);
				c.firstTriangle = (int32_t)mIsoTriangles.size();

				const int* v = MarchingCubes::triTable[code];
				while (*v >= 0)
				{
					int v0 = edges[*v++];
					PX_ASSERT(v0 >= 0);
					int v1 = edges[*v++];
					PX_ASSERT(v1 >= 0);
					int v2 = edges[*v++];
					PX_ASSERT(v2 >= 0);
					triangle.set(v0, v1, v2, xi, yi, zi);
					mIsoTriangles.pushBack(triangle);
					c.numTriangles++;
				}
			}
		}
	}
	progress.completeSubtask();

	return true;
}



bool ApexIsoMesh::interpolate(float d0, float d1, const PxVec3& pos0, const PxVec3& pos1, PxVec3& pos)
{
	if ((d0 < mIsoValue && d1 < mIsoValue) || (d0 > mIsoValue && d1 > mIsoValue))
	{
		return false;
	}

	float s = (mIsoValue - d0) / (d1 - d0);
	s = PxClamp(s, 0.01f, 0.99f);	// safety not to produce vertices at the same location
	pos = pos0 * (1.0f - s) + pos1 * s;
	return true;
}



bool ApexIsoMesh::findNeighbors(IProgressListener* progress)
{
	mIsoEdges.clear();
	IsoEdge edge;

	for (int32_t i = 0; i < (int32_t)mIsoTriangles.size(); i++)
	{
		IsoTriangle& t = mIsoTriangles[(uint32_t)i];
		edge.set(t.vertexNr[0], t.vertexNr[1], i);
		mIsoEdges.pushBack(edge);
		edge.set(t.vertexNr[1], t.vertexNr[2], i);
		mIsoEdges.pushBack(edge);
		edge.set(t.vertexNr[2], t.vertexNr[0], i);
		mIsoEdges.pushBack(edge);
	}

	progress->setProgress(30);

	shdfnd::sort(mIsoEdges.begin(), mIsoEdges.size(), IsoEdge());

	progress->setProgress(60);

	uint32_t i = 0;
	while (i < mIsoEdges.size())
	{
		uint32_t j = i + 1;
		while (j < mIsoEdges.size() && mIsoEdges[j] == mIsoEdges[i])
		{
			j++;
		}
		if (j > i + 1)
		{
			mIsoTriangles[(uint32_t)mIsoEdges[i  ].triangleNr].addNeighbor(mIsoEdges[j - 1].triangleNr);
			mIsoTriangles[(uint32_t)mIsoEdges[j - 1].triangleNr].addNeighbor(mIsoEdges[i  ].triangleNr);
		}
		i = j;
	}

	progress->setProgress(100);
	return true;
}

struct GroupAndSize
{
	int32_t group;
	uint32_t size;
	bool deleteMesh;

	bool operator()(const GroupAndSize& a, const GroupAndSize& b) const
	{
		return a.size > b.size;
	}
};

struct TimeAndDot
{
	float time;
	float dot;

	bool operator()(const TimeAndDot& a, const TimeAndDot& b) const
	{
		return a.time > b.time;
	}
};

void ApexIsoMesh::removeLayers()
{
	// mark regions
	nvidia::Array<GroupAndSize> triangleGroups;

	for (uint32_t i = 0; i < mIsoTriangles.size(); i++)
	{
		if (mIsoTriangles[i].groupNr >= 0)
		{
			continue;
		}

		GroupAndSize gas;
		gas.size = 0;
		gas.group = (int32_t)triangleGroups.size();
		gas.deleteMesh = false;

		gas.size = floodFill(i, (uint32_t)gas.group);

		triangleGroups.pushBack(gas);
	}

	if (triangleGroups.size() == 0)
	{
		return;
	}

	// clear regions
	if (mDiscardInnerMeshes)
	{
		nvidia::Array<TimeAndDot> raycastHits;
		for (uint32_t group = 0; group < triangleGroups.size(); group++)
		{
			// see if this group is an inner mesh
			raycastHits.clear();

			const int32_t groupNumber = triangleGroups[group].group;

			uint32_t start = 0;
			const uint32_t numTriangles = mIsoTriangles.size();
			for (uint32_t i = 0; i < numTriangles; i++)
			{
				if (mIsoTriangles[i].groupNr == groupNumber)
				{
					start = i;
					break;
				}
			}

			// raycast from the start triangle
			const PxVec3 rayOrig =   (mIsoVertices[(uint32_t)mIsoTriangles[start].vertexNr[0]] 
									+ mIsoVertices[(uint32_t)mIsoTriangles[start].vertexNr[1]] 
									+ mIsoVertices[(uint32_t)mIsoTriangles[start].vertexNr[2]]) / 3.0f;
			PxVec3 rayDir(1.0f, 1.0f, 1.0f);
			rayDir.normalize(); // can really be anything.

			// Find all ray hits
			for (uint32_t i = start; i < numTriangles; i++)
			{
				if (mIsoTriangles[i].groupNr != groupNumber)
				{
					continue;
				}

				float t, u, v;
				PxVec3 verts[3] =
				{
					mIsoVertices[(uint32_t)mIsoTriangles[i].vertexNr[0]],
					mIsoVertices[(uint32_t)mIsoTriangles[i].vertexNr[1]],
					mIsoVertices[(uint32_t)mIsoTriangles[i].vertexNr[2]],
				};

				if (APEX_RayTriangleIntersect(rayOrig, rayDir, verts[0], verts[1], verts[2], t, u, v))
				{
					TimeAndDot hit;
					hit.time = t;

					PxVec3 faceNormal = (verts[1] - verts[0]).cross(verts[2] - verts[0]);
					faceNormal.normalize();

					hit.dot = faceNormal.dot(rayDir);

					raycastHits.pushBack(hit);
				}
			}

			if (raycastHits.size() > 0)
			{
				shdfnd::sort(raycastHits.begin(), raycastHits.size(), TimeAndDot());

				triangleGroups[group].deleteMesh = raycastHits[0].dot > 0.0f;
			}
		}
	}

	if (mKeepNBiggestMeshes > 0)
	{
		shdfnd::sort(triangleGroups.begin(), triangleGroups.size(), GroupAndSize());

		for (uint32_t i = mKeepNBiggestMeshes; i < triangleGroups.size(); i++)
		{
			triangleGroups[i].deleteMesh = true;
		}
	}


	for (uint32_t i = 0; i < triangleGroups.size(); i++)
	{
		if (!triangleGroups[i].deleteMesh)
		{
			continue;
		}

		const int32_t group = triangleGroups[i].group;

		const uint32_t size = mIsoTriangles.size();
		for (uint32_t j = 0; j < size; j++)
		{
			if (mIsoTriangles[j].groupNr == group)
			{
				mIsoTriangles[j].deleted = true;
			}
		}
	}
}



uint32_t ApexIsoMesh::floodFill(uint32_t triangleNr, uint32_t groupNr)
{
	uint32_t numTriangles = 0;
	nvidia::Array<uint32_t> queue;
	queue.pushBack(triangleNr);

	while (!queue.empty())
	{
		IsoTriangle& t = mIsoTriangles[queue.back()];
		queue.popBack();
		if (t.groupNr == int32_t(groupNr))
		{
			continue;
		}

		t.groupNr = (int32_t)groupNr;
		numTriangles++;

		int32_t adj = t.adjTriangles[0];
		if (adj >= 0 && mIsoTriangles[(uint32_t)adj].groupNr < 0)
		{
			queue.pushBack((uint32_t)adj);
		}
		adj = t.adjTriangles[1];
		if (adj >= 0 && mIsoTriangles[(uint32_t)adj].groupNr < 0)
		{
			queue.pushBack((uint32_t)adj);
		}
		adj = t.adjTriangles[2];
		if (adj >= 0 && mIsoTriangles[(uint32_t)adj].groupNr < 0)
		{
			queue.pushBack((uint32_t)adj);
		}
	}

	return numTriangles;
}



void ApexIsoMesh::removeTrisAndVerts()
{
	for (int32_t i = (int32_t)mIsoTriangles.size() - 1; i >= 0; i--)
	{
		if (mIsoTriangles[(uint32_t)i].deleted)
		{
			mIsoTriangles.replaceWithLast((uint32_t)i);
		}
	}

	Array<int32_t> oldToNew;
	Array<PxVec3> newVertices;

	oldToNew.resize(mIsoVertices.size(), -1);

	for (uint32_t i = 0; i < mIsoTriangles.size(); i++)
	{
		IsoTriangle& t = mIsoTriangles[i];
		for (uint32_t j = 0; j < 3; j++)
		{
			uint32_t vNr = (uint32_t)t.vertexNr[j];
			if (oldToNew[vNr] < 0)
			{
				oldToNew[vNr] = (int32_t)newVertices.size();
				newVertices.pushBack(mIsoVertices[vNr]);
			}
			t.vertexNr[j] = oldToNew[vNr];
		}
	}

	mIsoVertices.clear();
	mIsoVertices.reserve(newVertices.size());

	for (uint32_t i = 0; i < newVertices.size(); i++)
	{
		mIsoVertices.pushBack(newVertices[i]);
	}
}

} // end namespace apex
} // end namespace nvidia
