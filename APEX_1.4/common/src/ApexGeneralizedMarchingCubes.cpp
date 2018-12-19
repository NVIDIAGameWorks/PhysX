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



#include "ApexGeneralizedMarchingCubes.h"

#include "ApexGeneralizedCubeTemplates.h"

#include "PxMath.h"

#include "ApexSharedUtils.h"

#include "PsSort.h"

namespace nvidia
{
namespace apex
{

struct TriangleEdge
{
	void init(int32_t v0, int32_t v1, int32_t edgeNr, int32_t triNr)
	{
		this->v0 = PxMin(v0, v1);
		this->v1 = PxMax(v0, v1);
		this->edgeNr = edgeNr;
		this->triNr = triNr;
	}
	bool operator == (const TriangleEdge& e) const
	{
		if (v0 == e.v0 && v1 == e.v1)
		{
			return true;
		}
		if (v0 == e.v1 && v1 == e.v0)
		{
			return true;
		}
		return false;
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
		if (a.v1 < b.v1)
		{
			return true;
		}
		if (a.v1 > b.v1)
		{
			return false;
		}
		return a.triNr < b.triNr;
	}
	int32_t v0, v1;
	int32_t edgeNr;
	int32_t triNr;
};



struct BorderEdge
{
	void set(int vertNr, int prev = -1, int depth = 0)
	{
		this->vertNr = vertNr;
		this->prev = prev;
		this->depth = depth;
	}
	int32_t vertNr;
	int32_t prev;
	int32_t depth;
};




ApexGeneralizedMarchingCubes::ApexGeneralizedMarchingCubes(const PxBounds3& bound, uint32_t subdivision) :
	mTemplates(NULL)
{
	mCubes.clear();
	mBound = bound;
	PX_ASSERT(subdivision > 0);
	mSpacing = (bound.maximum - bound.minimum).magnitude() / (float)subdivision;
	mInvSpacing = 1.0f / mSpacing;

	memset(mFirstCube, -1, sizeof(int32_t) * HASH_INDEX_SIZE);

	mVertices.clear();
	mIndices.clear();
}


ApexGeneralizedMarchingCubes::~ApexGeneralizedMarchingCubes()
{
	if (mTemplates != NULL)
	{
		delete mTemplates;
		mTemplates = NULL;
	}
}





void ApexGeneralizedMarchingCubes::registerTriangle(const PxVec3& p0, const PxVec3& p1, const PxVec3& p2)
{
	PxBounds3 bounds;
	bounds.setEmpty();
	bounds.include(p0);
	bounds.include(p1);
	bounds.include(p2);
	PX_ASSERT(!bounds.isEmpty());
	bounds.fattenFast(0.001f * mSpacing);
	PxVec3 n, vertPos, q0, q1, qt;
	n = (p1 - p0).cross(p2 - p0);
	float d = n.dot(p0);

	int32_t min[3] = { (int32_t)PxFloor(bounds.minimum.x* mInvSpacing), (int32_t)PxFloor(bounds.minimum.y* mInvSpacing), (int32_t)PxFloor(bounds.minimum.z* mInvSpacing) };
	int32_t max[3] = { (int32_t)PxFloor(bounds.maximum.x* mInvSpacing) + 1, (int32_t)PxFloor(bounds.maximum.y* mInvSpacing) + 1, (int32_t)PxFloor(bounds.maximum.z* mInvSpacing) + 1 };

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
				//const float axis0 = coord[dim0] * mSpacing;
				const float axis1 = coord[dim1] * mSpacing;
				const float axis2 = coord[dim2] * mSpacing;

				// does the ray go through the triangle?
				bool intersection = true;
				if (n[dim0] > 0.0f)
				{
					if ((p1[dim1] - p0[dim1]) * (axis2 - p0[dim2]) - (axis1 - p0[dim1]) * (p1[dim2] - p0[dim2]) < 0.0f)
					{
						intersection = false;
					}
					if ((p2[dim1] - p1[dim1]) * (axis2 - p1[dim2]) - (axis1 - p1[dim1]) * (p2[dim2] - p1[dim2]) < 0.0f)
					{
						intersection = false;
					}
					if ((p0[dim1] - p2[dim1]) * (axis2 - p2[dim2]) - (axis1 - p2[dim1]) * (p0[dim2] - p2[dim2]) < 0.0f)
					{
						intersection = false;
					}
				}
				else
				{
					if ((p1[dim1] - p0[dim1]) * (axis2 - p0[dim2]) - (axis1 - p0[dim1]) * (p1[dim2] - p0[dim2]) > 0.0f)
					{
						intersection = false;
					}
					if ((p2[dim1] - p1[dim1]) * (axis2 - p1[dim2]) - (axis1 - p1[dim1]) * (p2[dim2] - p1[dim2]) > 0.0f)
					{
						intersection = false;
					}
					if ((p0[dim1] - p2[dim1]) * (axis2 - p2[dim2]) - (axis1 - p2[dim1]) * (p0[dim2] - p2[dim2]) > 0.0f)
					{
						intersection = false;
					}
				}

				if (intersection)
				{
					const float pos = (d - axis1 * n[dim1] - axis2 * n[dim2]) / n[dim0];
					coord[dim0] = (int32_t)PxFloor(pos * mInvSpacing);

					uint32_t nr = (uint32_t)createCube(coord[0], coord[1], coord[2]);
					GeneralizedCube& cube = mCubes[nr];

					if (cube.vertRefs[dim0].vertNr < 0)
					{
						vertPos = PxVec3(coord[0] * mSpacing, coord[1] * mSpacing, coord[2] * mSpacing);
						vertPos[dim0] = pos;
						cube.vertRefs[dim0].vertNr = (int32_t)mVertices.size();
						mVertices.pushBack(vertPos);
					}
				}
			}
		}
	}

	// does a triangle edge cut a cube face?
	PxBounds3 cellBounds;
	for (int32_t xi = min[0]; xi <= max[0]; xi++)
	{
		for (int32_t yi = min[1]; yi <= max[1]; yi++)
		{
			for (int32_t zi = min[2]; zi <= max[2]; zi++)
			{
				cellBounds.minimum = PxVec3(xi * mSpacing, yi * mSpacing, zi * mSpacing);
				cellBounds.maximum = PxVec3((xi + 1) * mSpacing, (yi + 1) * mSpacing, (zi + 1) * mSpacing);

				for (uint32_t i = 0; i < 3; i++)
				{
					switch (i)
					{
					case 0 :
						q0 = p0;
						q1 = p1;
						break;
					case 1 :
						q0 = p1;
						q1 = p2;
						break;
					case 2 :
						q0 = p2;
						q1 = p0;
						break;
					default: // Make compiler happy
						q0 = q1 = PxVec3(0.0f);
						break;
					}

					if (q0.x != q1.x)
					{
						const float t = (cellBounds.minimum.x - q0.x) / (q1.x - q0.x);
						if (0.0f <= t && t <= 1.0f)
						{
							qt = q0 + (q1 - q0) * t;
							if (cellBounds.minimum.y <= qt.y && qt.y <= cellBounds.maximum.y && cellBounds.minimum.z <= qt.z && qt.z <= cellBounds.maximum.z)
							{
								GeneralizedCube& cube = mCubes[(uint32_t)createCube(xi, yi, zi)];
								cube.sideBounds[0].include(qt);
							}
						}
					}
					if (q0.y != q1.y)
					{
						const float t = (cellBounds.minimum.y - q0.y) / (q1.y - q0.y);
						if (0.0f <= t && t <= 1.0f)
						{
							qt = q0 + (q1 - q0) * t;
							if (cellBounds.minimum.z <= qt.z && qt.z <= cellBounds.maximum.z && cellBounds.minimum.x <= qt.x && qt.x <= cellBounds.maximum.x)
							{
								GeneralizedCube& cube = mCubes[(uint32_t)createCube(xi, yi, zi)];
								cube.sideBounds[1].include(qt);
							}
						}
					}
					if (q0.z != q1.z)
					{
						const float t = (cellBounds.minimum.z - q0.z) / (q1.z - q0.z);
						if (0.0f <= t && t <= 1.0f)
						{
							qt = q0 + (q1 - q0) * t;
							if (cellBounds.minimum.x <= qt.x && qt.x <= cellBounds.maximum.x && cellBounds.minimum.y <= qt.y && qt.y <= cellBounds.maximum.y)
							{
								GeneralizedCube& cube = mCubes[(uint32_t)createCube(xi, yi, zi)];
								cube.sideBounds[2].include(qt);
							}
						}
					}
				}
			}
		}
	}
}



bool ApexGeneralizedMarchingCubes::endRegistration(uint32_t bubleSizeToRemove, IProgressListener* progressListener)
{
	HierarchicalProgressListener progress(100, progressListener);

	progress.setSubtaskWork(20, "Complete Cells");
	completeCells();
	progress.completeSubtask();

	progress.setSubtaskWork(20, "Create Triangles");
	for (int i = 0; i < (int)mCubes.size(); i++)
	{
		createTrianglesForCube(i);
	}
	progress.completeSubtask();

	progress.setSubtaskWork(20, "Create Neighbor Info");
	createNeighbourInfo();
	progress.completeSubtask();

	uint32_t numTris = mIndices.size() / 3;

	mTriangleDeleted.resize(numTris);
	for (uint32_t i = 0; i < numTris; i++)
	{
		mTriangleDeleted[i] = 0;
	}

	if (bubleSizeToRemove > 0)
	{
		progress.setSubtaskWork(10, "Remove Bubbles");
		determineGroups();
		removeBubbles((int32_t)bubleSizeToRemove);
		progress.completeSubtask();
	}
	progress.setSubtaskWork(20, "Fix Orientation");
	determineGroups();
	fixOrientations();
	progress.completeSubtask();

	progress.setSubtaskWork(-1, "Compress");
	compress();
	progress.completeSubtask();

	return true;
}


int32_t ApexGeneralizedMarchingCubes::createCube(int32_t xi, int32_t yi, int32_t zi)
{
	int32_t nr = findCube(xi, yi, zi);
	if (nr >= 0)
	{
		return nr;
	}

	GeneralizedCube newCube;
	newCube.init(xi, yi, zi);
	int32_t h = hashFunction(xi, yi, zi);
	newCube.next = mFirstCube[h];
	mFirstCube[h] = (int32_t)mCubes.size();
	nr = (int32_t)mCubes.size();
	mCubes.pushBack(newCube);
	return nr;
}



int32_t ApexGeneralizedMarchingCubes::findCube(int32_t xi, int32_t yi, int32_t zi)
{
	int32_t h = hashFunction(xi, yi, zi);
	int32_t i = mFirstCube[h];

	while (i >= 0)
	{
		GeneralizedCube& c = mCubes[(uint32_t)i];
		if (!c.deleted && c.xi == xi && c.yi == yi && c.zi == zi)
		{
			return i;
		}
		i = mCubes[(uint32_t)i].next;
	}
	return -1;
}



void ApexGeneralizedMarchingCubes::completeCells()
{
	// make sure we have the boarder cells as well
	uint32_t numCubes = mCubes.size();
	for (uint32_t i = 0; i < numCubes; i++)
	{
		int32_t xi = mCubes[i].xi;
		int32_t yi = mCubes[i].yi;
		int32_t zi = mCubes[i].zi;
		//createCube(xi-1,yi,  zi);
		//createCube(xi,  yi-1,zi);
		//createCube(xi,  yi,  zi-1);

		createCube(xi - 1, yi,   zi);
		createCube(xi - 1, yi - 1, zi);
		createCube(xi,   yi - 1, zi);
		createCube(xi,   yi,   zi - 1);
		createCube(xi - 1, yi,   zi - 1);
		createCube(xi - 1, yi - 1, zi - 1);
		createCube(xi,   yi - 1, zi - 1);
	}
}



void ApexGeneralizedMarchingCubes::createTrianglesForCube(int32_t cellNr)
{
	const int sideEdges[6][4] =
	{
		{3, 7, 8, 11}, {1, 5, 9, 10}, {0, 4, 8, 9}, {2, 6, 10, 11}, {0, 1, 2, 3}, {4, 5, 6, 7}
	};
	const int adjVerts[12][8] =
	{
		{16, 1, 2, 3, 14, 4, 8, 9}, {16, 0, 2, 3, 13, 5, 9, 10}, {16, 0, 1, 3, 15, 6, 10, 11}, {16, 0, 1, 2, 12, 7, 8, 11},
		{17, 5, 6, 7, 14, 0, 8, 9}, {17, 4, 6, 7, 13, 1, 9, 10}, {17, 4, 5, 7, 15, 2, 10, 11}, {17, 4, 5, 6, 12, 3, 8, 11},
		{12, 3, 7, 11, 14, 0, 4, 9}, {14, 0, 4, 8, 13, 1, 5, 10}, {13, 1, 5, 9, 15, 2, 6, 11}, {15, 2, 6, 10, 12, 3, 7, 8}
	};

	int32_t groups[8];
	int32_t vertNrs[19];

	memset(vertNrs, -1, sizeof(int32_t) * 19);

	// get edge vertices
	GeneralizedVertRef* vertRefs[12];
	getCubeEdgesAndGroups(cellNr, vertRefs, groups);

	uint32_t numCuts = 0;
	for (uint32_t i = 0; i < 12; i++)
	{
		if (vertRefs[i] != NULL)
		{
			vertNrs[i] = vertRefs[i]->vertNr;
			if (vertNrs[i] >= 0)
			{
				numCuts++;
			}
		}
	}

	if (numCuts == 0)
	{
		return;
	}

	GeneralizedCube& c = mCubes[(uint32_t)cellNr];

	int startFace = -1, endFace = -1;

	PX_UNUSED(endFace); // Make compiler happy

	// create side vertices if necessary
	for (uint32_t i = 0; i < 6; i++)
	{
		uint32_t faceNr = 12 + i;
		int32_t* faceVertNr = NULL;
		PxBounds3* b = NULL;
		switch (faceNr)
		{
		case 12:
			faceVertNr = &c.sideVertexNr[0];
			b = &c.sideBounds[0];
			break;
		case 13:
		{
			int32_t nr = findCube(c.xi + 1, c.yi, c.zi);
			if (nr >= 0)
			{
				faceVertNr = &mCubes[(uint32_t)nr].sideVertexNr[0];
				b = &mCubes[(uint32_t)nr].sideBounds[0];
			}
		}
		break;
		case 14:
			faceVertNr = &c.sideVertexNr[1];
			b = &c.sideBounds[1];
			break;
		case 15:
		{
			int32_t nr = findCube(c.xi, c.yi + 1, c.zi);
			if (nr >= 0)
			{
				faceVertNr = &mCubes[(uint32_t)nr].sideVertexNr[1];
				b = &mCubes[(uint32_t)nr].sideBounds[1];
			}
		}
		break;
		case 16:
			faceVertNr = &c.sideVertexNr[2];
			b = &c.sideBounds[2];
			break;
		case 17:
		{
			int32_t nr = findCube(c.xi, c.yi, c.zi + 1);
			if (nr >= 0)
			{
				faceVertNr = &mCubes[(uint32_t)nr].sideVertexNr[2];
				b = &mCubes[(uint32_t)nr].sideBounds[2];
			}
		}
		break;
		}

		PxVec3 pos(0.0f, 0.0f, 0.0f);
		uint32_t num = 0;

		for (uint32_t j = 0; j < 4; j++)
		{
			int32_t edgeVertNr = vertNrs[sideEdges[i][j]];
			if (edgeVertNr < 0)
			{
				continue;
			}

			pos += mVertices[(uint32_t)edgeVertNr];
			num++;
		}

		if (num == 0 || num == 2)
		{
			continue;
		}

		pos = pos / (float)num;

		if (num == 1)
		{
			if (startFace < 0)
			{
				startFace = (int32_t)faceNr;
			}
			else
			{
				endFace = (int32_t)faceNr;
			}

			if (!(b->isEmpty()))
			{
				if (PxAbs(pos.x - b->minimum.x) > PxAbs(pos.x - b->maximum.x))
				{
					pos.x = b->minimum.x;
				}
				else
				{
					pos.x = b->maximum.x;
				}
				if (PxAbs(pos.y - b->minimum.y) > PxAbs(pos.y - b->maximum.y))
				{
					pos.y = b->minimum.y;
				}
				else
				{
					pos.y = b->maximum.y;
				}
				if (PxAbs(pos.z - b->minimum.z) > PxAbs(pos.z - b->maximum.z))
				{
					pos.z = b->minimum.z;
				}
				else
				{
					pos.z = b->maximum.z;
				}
			}
			else
			{
				continue;
			}
		}

		if (*faceVertNr < 0)
		{
			*faceVertNr = (int32_t)mVertices.size();
			mVertices.pushBack(pos);
		}
		vertNrs[faceNr] = *faceVertNr;
	}

	int32_t maxGroup = groups[0];
	for (uint32_t i = 1; i < 8; i++)
	{
		if (groups[i] > maxGroup)
		{
			maxGroup = groups[i];
		}
	}

	// boundary cell
	physx::Array<BorderEdge> queue;
	if (startFace >= 0)
	{
		BorderEdge edge;
		queue.clear();
		if (queue.capacity() < 20)
		{
			queue.reserve(20);
		}

		int32_t prev[19];
		bool visited[19];
		for (uint32_t i = 0; i < 19; i++)
		{
			prev[i] = -1;
			visited[i] = false;
		}

		edge.set(startFace);
		queue.pushBack(edge);

		int32_t maxVert = -1;
		int32_t maxDepth = -1;

		while (!queue.empty())
		{
			edge = queue[queue.size() - 1];
			queue.popBack();

			int32_t v = edge.vertNr;
			if (visited[v])
			{
				continue;
			}

			visited[v] = true;
			prev[v] = edge.prev;
			edge.prev = v;
			edge.depth++;

			if (edge.depth > maxDepth)
			{
				maxDepth = edge.depth;
				maxVert = v;
			}
			//			if (v == endFace) break;

			if (v < 12)
			{
				for (uint32_t i = 0; i < 8; i += 4)
				{
					edge.vertNr = adjVerts[v][i];
					if (vertNrs[edge.vertNr] >= 0)
					{
						if (!visited[edge.vertNr])
						{
							queue.pushBack(edge);
						}
					}
					else
					{
						for (uint32_t j = i + 1; j < i + 4; j++)
						{
							edge.vertNr = adjVerts[v][j];
							if (vertNrs[edge.vertNr] >= 0 && !visited[edge.vertNr])
							{
								queue.pushBack(edge);
							}
						}
					}
				}
			}
			else
			{
				for (uint32_t i = 0; i < 4; i++)
				{
					edge.vertNr = sideEdges[v - 12][i];
					if (!visited[edge.vertNr] && vertNrs[edge.vertNr] >= 0)
					{
						queue.pushBack(edge);
					}
				}
			}
		}

		int32_t chain[14];
		uint32_t chainLen = 0;
		int32_t v = maxVert;

		if (vertNrs[v] >= 0)
		{
			chain[chainLen++] = v;
		}

		v = prev[v];
		while (v >= 0)
		{
			if (vertNrs[v] >= 0)
			{
				chain[chainLen++] = v;
			}
			v = prev[v];
		}

		int32_t numTris = (int32_t)chainLen - 2;

		c.firstTriangle = (int32_t)mIndices.size() / 3;
		for (int i = 0; i < numTris; i++)
		{
			mIndices.pushBack((uint32_t)vertNrs[(uint32_t)chain[0]]);
			mIndices.pushBack((uint32_t)vertNrs[(uint32_t)chain[i + 1]]);
			mIndices.pushBack((uint32_t)vertNrs[(uint32_t)chain[i + 2]]);
			c.numTriangles++;
		}
		return;
	}

	// inner cell
	if (mTemplates == NULL)
	{
		mTemplates = PX_NEW(ApexGeneralizedCubeTemplates)();
	}
	mTemplates->getTriangles(groups, mGeneralizedTriangles);

	// create face and inner vertices if necessary
	for (uint32_t i = 0; i < mGeneralizedTriangles.size(); i++)
	{
		int32_t localNr = mGeneralizedTriangles[i];

		if (vertNrs[localNr] < 0)
		{
			if (localNr < 12)
			{
				return;    // impossible to create triangles, border cube
			}

			if (localNr == 18 && vertNrs[18] < 0)
			{
				// center vertex, not shared with other cells
				vertNrs[18] = (int32_t)mVertices.size();
				PxVec3 pos(0.0f);
				float num = 0.0f;
				for (int j = 0; j < 12; j++)
				{
					if (vertNrs[j] >= 0)
					{
						pos += mVertices[(uint32_t)vertNrs[j]];
						num = num + 1.0f;
					}
				}
				mVertices.pushBack(pos / num);
			}
		}
	}

	c.firstTriangle = (int32_t)mIndices.size() / 3;
	const uint32_t numTris = mGeneralizedTriangles.size() / 3;
	for (uint32_t i = 0; i < numTris; i++)
	{
		PX_ASSERT(vertNrs[mGeneralizedTriangles[3 * i + 0]] != vertNrs[mGeneralizedTriangles[3 * i + 1]]);
		PX_ASSERT(vertNrs[mGeneralizedTriangles[3 * i + 0]] != vertNrs[mGeneralizedTriangles[3 * i + 2]]);
		PX_ASSERT(vertNrs[mGeneralizedTriangles[3 * i + 1]] != vertNrs[mGeneralizedTriangles[3 * i + 2]]);
		mIndices.pushBack((uint32_t)vertNrs[mGeneralizedTriangles[3 * i + 0]]);
		mIndices.pushBack((uint32_t)vertNrs[mGeneralizedTriangles[3 * i + 1]]);
		mIndices.pushBack((uint32_t)vertNrs[mGeneralizedTriangles[3 * i + 2]]);
		c.numTriangles++;
	}
}



void ApexGeneralizedMarchingCubes::createNeighbourInfo()
{
	physx::Array<TriangleEdge> edges;
	edges.reserve(mIndices.size());

	mFirstNeighbour.resize(mIndices.size());
	for (uint32_t i = 0; i < mFirstNeighbour.size(); i++)
	{
		mFirstNeighbour[i] = -1;
	}

	mNeighbours.clear();

	const uint32_t numTriangles = mIndices.size() / 3;
	for (uint32_t i = 0; i < numTriangles; i++)
	{
		TriangleEdge edge;
		int32_t i0 = (int32_t)mIndices[3 * i];
		int32_t i1 = (int32_t)mIndices[3 * i + 1];
		int32_t i2 = (int32_t)mIndices[3 * i + 2];
		PX_ASSERT(i0 != i1);
		PX_ASSERT(i0 != i2);
		PX_ASSERT(i1 != i2);
		edge.init(i0, i1, 0, (int32_t)i);
		edges.pushBack(edge);
		edge.init(i1, i2, 1, (int32_t)i);
		edges.pushBack(edge);
		edge.init(i2, i0, 2, (int32_t)i);
		edges.pushBack(edge);
	}
	shdfnd::sort(edges.begin(), edges.size(), TriangleEdge());

	uint32_t i = 0;
	const uint32_t numEdges = edges.size();
	while (i < numEdges)
	{
		const TriangleEdge& e0 = edges[i];
		const int32_t first = (int32_t)mNeighbours.size();
		PX_ASSERT(mFirstNeighbour[(uint32_t)(e0.triNr * 3 + e0.edgeNr)] == -1);
		mFirstNeighbour[(uint32_t)(e0.triNr * 3 + e0.edgeNr)] = first;
		mNeighbours.pushBack(e0.triNr);
		i++;

		while (i < numEdges && edges[i] == e0)
		{
			const TriangleEdge& e1 = edges[i];
			PX_ASSERT(mFirstNeighbour[(uint32_t)(e1.triNr * 3 + e1.edgeNr)] == -1);
			mFirstNeighbour[(uint32_t)(e1.triNr * 3 + e1.edgeNr)] = first;
			mNeighbours.pushBack(e1.triNr);
			i++;
		}
		mNeighbours.pushBack(-1);	// end marker
	}
}



void ApexGeneralizedMarchingCubes::getCubeEdgesAndGroups(int32_t cellNr, GeneralizedVertRef* vertRefs[12], int32_t groups[8])
{
	const int32_t adjCorners[8][3] =
	{
		{1, 3, 4}, {0, 2, 5}, {1, 3, 6},  {0, 2, 7},  {0, 5, 7}, {1, 4, 6}, {2, 5, 7},  {3, 4, 6}
	};

	const int32_t adjEdges[8][3] =
	{
		{0, 3, 8}, {0, 1, 9}, {1, 2, 10}, {3, 2, 11}, {8, 4, 7}, {9, 4, 5}, {10, 5, 6}, {11, 7, 6}
	};

	// collect edge vertices
	GeneralizedCube& c = mCubes[(uint32_t)cellNr];
	for (uint32_t i = 0; i < 12; i++)
	{
		vertRefs[i] = NULL;
	}

	vertRefs[0] = &c.vertRefs[0];
	vertRefs[3] = &c.vertRefs[1];
	vertRefs[8] = &c.vertRefs[2];

	int32_t nr = findCube(c.xi + 1, c.yi, c.zi);
	if (nr >= 0)
	{
		vertRefs[1] = &mCubes[(uint32_t)nr].vertRefs[1];
		vertRefs[9] = &mCubes[(uint32_t)nr].vertRefs[2];
	}
	nr = findCube(c.xi, c.yi + 1, c.zi);
	if (nr >= 0)
	{
		vertRefs[2] = &mCubes[(uint32_t)nr].vertRefs[0];
		vertRefs[11] = &mCubes[(uint32_t)nr].vertRefs[2];
	}
	nr = findCube(c.xi, c.yi, c.zi + 1);
	if (nr >= 0)
	{
		vertRefs[4] = &mCubes[(uint32_t)nr].vertRefs[0];
		vertRefs[7] = &mCubes[(uint32_t)nr].vertRefs[1];
	}
	nr = findCube(c.xi + 1, c.yi + 1, c.zi);
	if (nr >= 0)
	{
		vertRefs[10] = &mCubes[(uint32_t)nr].vertRefs[2];
	}
	nr = findCube(c.xi, c.yi + 1, c.zi + 1);
	if (nr >= 0)
	{
		vertRefs[6] = &mCubes[(uint32_t)nr].vertRefs[0];
	}
	nr = findCube(c.xi + 1, c.yi, c.zi + 1);
	if (nr >= 0)
	{
		vertRefs[5] = &mCubes[(uint32_t)nr].vertRefs[1];
	}

	// assign groups using flood fill on the cube edges
	for (uint32_t i = 0; i < 8; i++)
	{
		groups[i] = -1;
	}

	int groupNr = -1;
	for (uint32_t i = 0; i < 8; i++)
	{
		if (groups[i] >= 0)
		{
			continue;
		}

		groupNr++;
		mCubeQueue.clear();
		mCubeQueue.pushBack((int32_t)i);
		while (!mCubeQueue.empty())
		{
			int32_t cornerNr = mCubeQueue[mCubeQueue.size() - 1];
			mCubeQueue.popBack();

			if (groups[cornerNr] >= 0)
			{
				continue;
			}

			groups[cornerNr] = groupNr;

			for (uint32_t j = 0; j < 3; j++)
			{
				int32_t adjCorner = adjCorners[cornerNr][j];
				int32_t adjEdge   = adjEdges[cornerNr][j];
				if (vertRefs[adjEdge] != NULL && vertRefs[adjEdge]->vertNr >= 0)	// edge blocked by vertex
				{
					continue;
				}

				if (groups[adjCorner] < 0)
				{
					mCubeQueue.pushBack(adjCorner);
				}
			}
		}
	}
}



void ApexGeneralizedMarchingCubes::determineGroups()
{
	const uint32_t numTris = mIndices.size() / 3;

	mTriangleGroup.resize(numTris);
	for (uint32_t i = 0; i < numTris; i++)
	{
		mTriangleGroup[i] = -1;
	}

	mGroupFirstTriangle.clear();
	mGroupTriangles.clear();

	for (uint32_t i = 0; i < numTris; i++)
	{
		if (mTriangleDeleted[i])
		{
			continue;
		}
		if (mTriangleGroup[i] >= 0)
		{
			continue;
		}

		int32_t group = (int32_t)mGroupFirstTriangle.size();
		mGroupFirstTriangle.pushBack((int32_t)mGroupTriangles.size());

		mCubeQueue.clear();
		mCubeQueue.pushBack((int32_t)i);
		while (!mCubeQueue.empty())
		{
			const uint32_t t = (uint32_t)mCubeQueue[mCubeQueue.size() - 1];
			mCubeQueue.popBack();

			if (mTriangleGroup[t] >= 0)
			{
				continue;
			}

			mTriangleGroup[t] = group;
			mGroupTriangles.pushBack((int32_t)t);

			for (uint32_t j = 0; j < 3; j++)
			{
				uint32_t first = (uint32_t)mFirstNeighbour[3 * t + j];
				uint32_t num = 0;
				int32_t n = -1;
				while (mNeighbours[first] >= 0)
				{
					if (mNeighbours[first] != (int32_t)t)
					{
						n = mNeighbours[first];
					}

					first++;
					num++;
				}
				if (num == 2 && n >= 0 && mTriangleGroup[(uint32_t)n] < 0)
				{
					if (!mTriangleDeleted[(uint32_t)n])
					{
						mCubeQueue.pushBack(n);
					}
				}
			}
		}
	}
}



void ApexGeneralizedMarchingCubes::removeBubbles(int32_t minGroupSize)
{
	const uint32_t numGroups = mGroupFirstTriangle.size();
	for (uint32_t i = 0; i < numGroups; i++)
	{
		int32_t firstTri = mGroupFirstTriangle[i];
		int32_t lastTri  = i < numGroups - 1 ? mGroupFirstTriangle[i + 1] : (int32_t)mGroupTriangles.size();

		if (lastTri - firstTri > minGroupSize)
		{
			continue;
		}

		bool OK = true;

		for (int32_t j = firstTri; j < lastTri; j++)
		{
			int32_t t = mGroupTriangles[(uint32_t)j];
			for (uint32_t k = 0; k < 3; k++)
			{
				uint32_t first = (uint32_t)mFirstNeighbour[3 * t + k];
				uint32_t num = 0;
				int32_t n = -1;
				while (mNeighbours[first] >= 0)
				{
					if (mNeighbours[first] != t)
					{
						n = mNeighbours[first];
					}

					num++;
					first++;
				}
				if (num == 2 && mTriangleGroup[(uint32_t)n] != (int32_t)i)
				{
					OK = false;
					break;
				}
			}
			if (!OK)
			{
				break;
			}
		}


		if (!OK)
		{
			continue;
		}

		for (int32_t j = firstTri; j < lastTri; j++)
		{
			uint32_t t = (uint32_t)mGroupTriangles[(uint32_t)j];
			// remove neighbor info
			for (int k = 0; k < 3; k++)
			{
				uint32_t first = (uint32_t)mFirstNeighbour[3 * t + k];
				int32_t pos = -1;
				while (mNeighbours[first] >= 0)
				{
					if (mNeighbours[first] == (int32_t)t)
					{
						PX_ASSERT(pos == -1);
						pos = (int32_t)first;
					}

					first++;
				}
				if (pos >= 0)
				{
					mNeighbours[(uint32_t)pos] = mNeighbours[first - 1];
					mNeighbours[first - 1] = -1;
				}
			}
			// remove triangle
			mTriangleDeleted[t] = true;

			/*
			// debugging only
			mDebugLines.pushBack(mVertices[mIndices[t * 3 + 0]]);
			mDebugLines.pushBack(mVertices[mIndices[t * 3 + 1]]);
			mDebugLines.pushBack(mVertices[mIndices[t * 3 + 1]]);
			mDebugLines.pushBack(mVertices[mIndices[t * 3 + 2]]);
			mDebugLines.pushBack(mVertices[mIndices[t * 3 + 2]]);
			mDebugLines.pushBack(mVertices[mIndices[t * 3 + 0]]);
			*/
		}
	}
}



void ApexGeneralizedMarchingCubes::fixOrientations()
{
	if (mIndices.size() == 0)
	{
		return;
	}

	const uint32_t numTris = mIndices.size() / 3;

	physx::Array<uint8_t> marks;
	marks.resize(numTris);

	// 0 = non visited, 1 = visited, 2 = visited, to be flipped
	for (uint32_t i = 0; i < numTris; i++)
	{
		marks[i] = 0;
	}

	physx::Array<int32_t> queue;
	for (uint32_t i = 0; i < numTris; i++)
	{
		if (mTriangleDeleted[i])
		{
			continue;
		}

		if (marks[i] != 0)
		{
			continue;
		}

		queue.clear();

		marks[i] = 1;
		queue.pushBack((int32_t)i);
		while (!queue.empty())
		{
			uint32_t triNr = (uint32_t)queue[queue.size() - 1];
			queue.popBack();

			for (uint32_t j = 0; j < 3; j++)
			{
				uint32_t first = (uint32_t)mFirstNeighbour[3 * triNr + j];
				uint32_t num = 0;
				int32_t adjNr = -1;

				while (mNeighbours[first] >= 0)
				{
					if (mNeighbours[first] != (int32_t)triNr)
					{
						adjNr = mNeighbours[first];
					}

					first++;
					num++;
				}

				if (num != 2)
				{
					continue;
				}
				if (marks[(uint32_t)adjNr] != 0)
				{
					continue;
				}

				queue.pushBack(adjNr);

				uint32_t i0, i1;
				if (marks[(uint32_t)triNr] == 1)
				{
					i0 = mIndices[3 * triNr + j];
					i1 = mIndices[3 * triNr + ((j + 1) % 3)];
				}
				else
				{
					i1 = mIndices[3 * triNr + j];
					i0 = mIndices[3 * triNr + ((j + 1) % 3)];
				}

				// don't swap here because this would corrupt the neighbor information
				marks[(uint32_t)adjNr] = 1;
				if (mIndices[3 * (uint32_t)adjNr + 0] == i0 && mIndices[3 * (uint32_t)adjNr + 1] == i1)
				{
					marks[(uint32_t)adjNr] = 2;
				}
				if (mIndices[3 * (uint32_t)adjNr + 1] == i0 && mIndices[3 * (uint32_t)adjNr + 2] == i1)
				{
					marks[(uint32_t)adjNr] = 2;
				}
				if (mIndices[3 * (uint32_t)adjNr + 2] == i0 && mIndices[3 * (uint32_t)adjNr + 0] == i1)
				{
					marks[(uint32_t)adjNr] = 2;
				}
			}
		}
	}

	for (uint32_t i = 0; i < numTris; i++)
	{
		if (marks[i] == 2)
		{
			uint32_t i0 = mIndices[3 * i];
			mIndices[3 * i] = mIndices[3 * i + 1];
			mIndices[3 * i + 1] = i0;
		}
	}
}



void ApexGeneralizedMarchingCubes::compress()
{
	PX_ASSERT(mTriangleDeleted.size() * 3 == mIndices.size());

	uint32_t writePos = 0;

	for (uint32_t i = 0; i < mTriangleDeleted.size(); i++)
	{
		if (mTriangleDeleted[i] == 1)
		{
			continue;
		}
		for (uint32_t j = 0; j < 3; j++)
		{
			mIndices[3 * writePos + j] = mIndices[3 * i + j];
		}
		writePos++;
	}

	mIndices.resize(3 * writePos);

	mTriangleDeleted.clear();
	mTriangleDeleted.reset();

	mNeighbours.clear();
	mNeighbours.reset();

	mFirstNeighbour.clear();
	mFirstNeighbour.reset();
}

}
} // end namespace nvidia::apex
