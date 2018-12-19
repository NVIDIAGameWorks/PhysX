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


#include "ApexUsingNamespace.h"
#include "authoring/ApexCSGDefs.h"
#include "PxErrorCallback.h"

#ifndef WITHOUT_APEX_AUTHORING

#pragma warning(disable:4505)

namespace ApexCSG
{

PX_INLINE bool
pointOnRay2D(const Vec2Real& point, const Vec2Real& lineOrig, const Vec2Real& lineDisp, Real distanceTol2)
{
	const Vec2Real disp = point - lineOrig;
	const Real lineDisp2 = lineDisp | lineDisp;
	const Real dispDotLineDisp = disp | lineDisp;
	const Real distanceTol2LineDisp2 = distanceTol2 * lineDisp2;
	if (dispDotLineDisp < distanceTol2LineDisp2)
	{
		return false;
	}
	if ((disp | disp) * lineDisp2 - square(dispDotLineDisp) > distanceTol2LineDisp2)
	{
		return false;
	}
	return true;
}

PX_INLINE bool
diagonalIsValidInLoop(const LinkedEdge2D* edge, const LinkedEdge2D* otherEdge, const LinkedEdge2D* loop)
{
	const Vec2Real& diagonalStart = edge->v[0];
	const Vec2Real& diagonalEnd = otherEdge->v[0];
	const Vec2Real diagonalDisp = diagonalEnd - diagonalStart;

	const LinkedEdge2D* loopEdge = loop;
	LinkedEdge2D* loopEdgeNext = loopEdge->getAdj(1);
	do
	{
		loopEdgeNext = loopEdge->getAdj(1);
		if (loopEdge == edge || loopEdgeNext == edge || loopEdge == otherEdge || loopEdgeNext == otherEdge)
		{
			continue;
		}
		const Vec2Real& loopEdgeStart = loopEdge->v[0];
		const Vec2Real& loopEdgeEnd = loopEdge->v[1];
		const Vec2Real& loopEdgeDisp = loopEdgeEnd - loopEdgeStart;
		if ((loopEdgeDisp ^(diagonalStart - loopEdgeStart)) * (loopEdgeDisp ^(diagonalEnd - loopEdgeStart)) <= 0 &&
		        (diagonalDisp ^(loopEdgeStart - diagonalStart)) * (diagonalDisp ^(loopEdgeEnd - diagonalStart)) <= 0)
		{
			// Edge intersection
			return false;
		}
	}
	while ((loopEdge = loopEdgeNext) != loop);

	return true;
}

PX_INLINE bool
diagonalIsValid(const LinkedEdge2D* edge, const LinkedEdge2D* otherEdge, const physx::Array<LinkedEdge2D*>& loops)
{
	for (uint32_t i = 0; i < loops.size(); ++i)
	{
		if (!diagonalIsValidInLoop(edge, otherEdge, loops[i]))
		{
			return false;
		}
	}

	return true;
}

PX_INLINE uint32_t
pointCondition(const Vec2Real& point, const Vec2Real& n0, Real d0, const Vec2Real& n1, Real d1, const Vec2Real& n2, Real d2)
{
	return (uint32_t)((point|n0) + d0 >= (Real)0) | ((uint32_t)((point|n1) + d1 >= (Real)0)) << 1 | ((uint32_t)((point|n2) + d2 >= (Real)0)) << 2;
}

PX_INLINE bool
triangulate(LinkedEdge2D* loop, physx::Array<Vec2Real>& planeVertices, nvidia::Pool<LinkedEdge2D>& linkedEdgePool)
{
	// Trivial cases
	LinkedEdge2D* edge0 = loop;
	LinkedEdge2D* edge1 = edge0->getAdj(1);
	if (edge1 == edge0)
	{
		// Single vertex !?
		linkedEdgePool.replace(edge0);
		return true;
	}

	LinkedEdge2D* edge2 = edge1->getAdj(1);
	if (edge2 == edge0)
	{
		// Degenerate
		linkedEdgePool.replace(edge0);
		linkedEdgePool.replace(edge1);
		return true;
	}

	for(;;)
	{
		LinkedEdge2D* edge3 = edge2->getAdj(1);
		if (edge3 == edge0)
		{
			// Single triangle - we're done
			planeVertices.pushBack(edge0->v[0]);
			planeVertices.pushBack(edge1->v[0]);
			planeVertices.pushBack(edge2->v[0]);
			linkedEdgePool.replace(edge0);
			linkedEdgePool.replace(edge1);
			linkedEdgePool.replace(edge2);
			break;
		}

		// Polygon has more than three vertices.  Find an ear
		bool earFound = false;
		do
		{
			Vec2Real e01 = edge1->v[0] - edge0->v[0];
			Vec2Real e12 = edge2->v[0] - edge1->v[0];
			if ((e01 ^ e12) > (Real)0)
			{
				// Convex, see if this is an ear
				const Vec2Real n01 = e01.perp();
				const Real d01 = -(n01 | edge0->v[0]);
				const Vec2Real n12 = e12.perp();
				const Real d12 = -(n12 | edge1->v[0]);
				const Vec2Real n20 = Vec2Real(edge0->v[0] - edge2->v[0]).perp();
				const Real d20 = -(n20 | edge2->v[0]);
				LinkedEdge2D* eTest = edge3;
				bool edgeIntersectsTriangle = false;	// Until proven otherwise
				do
				{
					if (pointCondition(eTest->v[0], n01, d01, n12, d12, n20, d20))
					{
						// Point is inside the triangle
						edgeIntersectsTriangle = true;
						break;
					}
				} while ((eTest = eTest->getAdj(1)) != edge0);
				if (!edgeIntersectsTriangle)
				{
					// Ear found
					planeVertices.pushBack(edge0->v[0]);
					planeVertices.pushBack(edge1->v[0]);
					planeVertices.pushBack(edge2->v[0]);
					edge0->v[1] = edge0->v[0];	// So that the next and previous edges will join up to the correct location after remove() is called()
					edge0->remove();
					linkedEdgePool.replace(edge0);
					if (edge0 == loop)
					{
						loop = edge1;
					}
					edge0 = edge1;
					edge1 = edge2;
					edge2 = edge3;
					earFound = true;
					break;
				}
			}

			edge0 = edge1;
			edge1 = edge2;
			edge2 = edge3;
			edge3 = edge3->getAdj(1);
		} while (edge0 != loop);

		if (!earFound)
		{
			// Something went wrong
			return false;
		}
	}

	return true;
}

static bool
mergeTriangles2D(physx::Array<Vec2Real>& planeVertices, nvidia::Pool<LinkedEdge2D>& linkedEdgePool, Real distanceTol)
{
	// Create a set of linked edges for each triangle. The initial set will consist of nothing but three-edge loops (the triangles).
	physx::Array<LinkedEdge2D*> edges;
	edges.reserve(planeVertices.size());
	PX_ASSERT((planeVertices.size() % 3) == 0);
	for (uint32_t i = 0; i < planeVertices.size(); i += 3)
	{
		LinkedEdge2D* v0 = linkedEdgePool.borrow();
		edges.pushBack(v0);
		LinkedEdge2D* v1 = linkedEdgePool.borrow();
		edges.pushBack(v1);
		LinkedEdge2D* v2 = linkedEdgePool.borrow();
		edges.pushBack(v2);
		v0->setAdj(1, v1);
		v1->setAdj(1, v2);
		v0->v[0] = planeVertices[i];
		v0->v[1] = planeVertices[i + 1];
		v1->v[0] = planeVertices[i + 1];
		v1->v[1] = planeVertices[i + 2];
		v2->v[0] = planeVertices[i + 2];
		v2->v[1] = planeVertices[i];
	}

	const Real distanceTol2 = distanceTol * distanceTol;

	// Find all edge overlaps and merge loops
	for (uint32_t i = 0; i < edges.size(); ++i)
	{
		LinkedEdge2D* edge = edges[i];
		const Vec2Real edgeDisp = edge->v[1] - edge->v[0];
		for (uint32_t j = i + 1; j < edges.size(); ++j)
		{
			LinkedEdge2D* otherEdge = edges[j];
			const Vec2Real otherEdgeDisp = otherEdge->v[1] - otherEdge->v[0];
			if ((otherEdgeDisp | edgeDisp) < 0)
			{
				if (pointOnRay2D(otherEdge->v[0], edge->v[0], edgeDisp, distanceTol2) && pointOnRay2D(otherEdge->v[1], edge->v[1], -edgeDisp, distanceTol2))
				{
					edge->setAdj(0, otherEdge->getAdj(0));
				}
				else
				if (pointOnRay2D(edge->v[0], otherEdge->v[0], otherEdgeDisp, distanceTol2) && pointOnRay2D(edge->v[1], otherEdge->v[1], -otherEdgeDisp, distanceTol2))
				{
					otherEdge->setAdj(0, edge->getAdj(0));
				}
			}
		}
	}

	// Clean further by removing adjacent collinear edges.  Also label loops.
	physx::Array<LinkedEdge2D*> loops;
	int32_t loopID = 0;
	for (uint32_t i = edges.size(); i--;)
	{
		LinkedEdge2D* edge = edges[i];
		if (edge == NULL || edge->isSolitary())
		{
			if (edge != NULL)
			{
				linkedEdgePool.replace(edge);
			}
			edges.replaceWithLast(i);
			continue;
		}
		if (edge->loopID < 0)
		{
			do
			{
				edge->loopID = loopID;
				LinkedEdge2D* prev = edge->getAdj(0);
				LinkedEdge2D* next = edge->getAdj(1);
				const Vec2Real edgeDisp = edge->v[1] - edge->v[0];
				const Vec2Real prevDisp = prev->v[1] - prev->v[0];
				const Real sumDisp2 = (prevDisp + edgeDisp).lengthSquared();
				if (sumDisp2 < distanceTol2)
				{
					// These two edges cancel.  Eliminate both.
					next = edge->getAdj(1);
					prev->v[1] = prev->v[0];
					edge->v[0] = prev->v[1];
					prev->remove();
					edge->remove();
					if (next == prev)
					{
						// Loops is empty
						edge = NULL;
						break;	// Done
					}
				}
				else
				{
					const Real h2 = square(prevDisp ^ edgeDisp) / sumDisp2;
					if (h2 < distanceTol2)
					{
						// Collinear, remove
						prev->v[1] = edge->v[0] = edge->v[1];
						edge->remove();
					}
				}
				edge = next;
			}
			while (edge->loopID < 0);
			if (edge != NULL)
			{
				++loopID;
				loops.pushBack(edge);
			}
		}
	}

	if (loops.size() == 0)
	{
		// No loops, done
		planeVertices.reset();
		return true;
	}

	// The methods employed below are not optimal in time.  But the majority of cases will be simple polygons
	// with no holes and a small number of vertices.  So an optimal algorithm probably isn't worth implementing.

	// Merge all loops into one by finding diagonals to join them
	while (loops.size() > 1)
	{
		LinkedEdge2D* loop = loops.back();
		LinkedEdge2D* bestEdge = NULL;
		LinkedEdge2D* bestOtherEdge = NULL;
		uint32_t bestOtherLoopIndex = 0xFFFFFFFF;
		Real minDist2 = MAX_REAL;
		for (uint32_t i = loops.size() - 1; i--;)
		{
			LinkedEdge2D* otherLoop = loops[i];
			LinkedEdge2D* otherEdge = otherLoop;
			do
			{
				LinkedEdge2D* edge = loop;
				do
				{
					if (diagonalIsValid(edge, otherEdge, loops))
					{
						const Real dist2 = (edge->v[0] - otherEdge->v[0]).lengthSquared();
						if (dist2 < minDist2)
						{
							bestEdge = edge;
							bestOtherEdge = otherEdge;
							bestOtherLoopIndex = i;
							minDist2 = dist2;
						}
					}
				}
				while ((edge = edge->getAdj(1)) != loop);
			}
			while ((otherEdge = otherEdge->getAdj(1)) != otherLoop);
		}

		if (bestOtherLoopIndex == 0xFFFFFFFF)
		{
			// Clean up loops
			for (uint32_t i = 0; i < loops.size(); ++i)
			{
				LinkedEdge2D* edge = loops[i];
				bool done = false;
				do
				{
					done = edge->isSolitary();
					LinkedEdge2D* next = edge->getAdj(1);
					linkedEdgePool.replace(edge);	// This also removes the link from the loop
					edge = next;
				}
				while (!done);
			}
			return false;
		}

		// Create diagonal loop with correct endpoints
		LinkedEdge2D* diagonal = linkedEdgePool.borrow();
		LinkedEdge2D* reciprocal = linkedEdgePool.borrow();
		diagonal->setAdj(1, reciprocal);
		diagonal->v[1] = reciprocal->v[0] = bestEdge->v[0];
		diagonal->v[0] = reciprocal->v[1] = bestOtherEdge->v[0];

		// Insert diagonal loop, merging loops.back() with loops[i]
		diagonal->setAdj(1, bestEdge);
		reciprocal->setAdj(1, bestOtherEdge);
		loops.popBack();
	}

	// Erase planeVertices, will reuse.
	planeVertices.reset();

	// We have one loop.  Triangulate.
	return triangulate(loops[0], planeVertices, linkedEdgePool);
}

static void
mergeTriangles(physx::Array<nvidia::ExplicitRenderTriangle>& cleanedMesh, const Triangle* triangles, const Interpolator* frames, ClippedTriangleInfo* info, uint32_t triangleCount,
               const physx::Array<Triangle>& originalTriangles, const Plane& plane, nvidia::Pool<LinkedEdge2D>& linkedEdgePool, Real distanceTol, const Mat4Real& BSPToMeshTM)
{
	if (triangleCount == 0)
	{
		return;
	}

	const uint32_t originalTriangleIndexStart = info[0].originalTriangleIndex;
	const uint32_t originalTriangleIndexCount = info[triangleCount - 1].originalTriangleIndex - originalTriangleIndexStart + 1;

	physx::Array<uint32_t> originalTriangleGroupStarts;
	nvidia::createIndexStartLookup(originalTriangleGroupStarts, (int32_t)originalTriangleIndexStart, originalTriangleIndexCount,
	                              (int32_t*)&info->originalTriangleIndex, triangleCount, sizeof(ClippedTriangleInfo));

	// Now group equal reference frames, and transform into 2D
	physx::Array<Vec2Real> planeVertices;
	nvidia::IndexBank<uint32_t> frameIndices;
	frameIndices.reserve(originalTriangleIndexCount);
	frameIndices.lockCapacity(true);
	while (frameIndices.freeCount())
	{
		planeVertices.reset();	// Erase, we'll reuse this array
		uint32_t seedFrameIndex = 0;
		frameIndices.useNextFree(seedFrameIndex);
		const Interpolator& seedFrame = frames[seedFrameIndex + originalTriangleIndexStart];
		const Triangle& seedOriginalTri = originalTriangles[originalTriangleIndexStart + seedFrameIndex];
//		const Plane plane(seedOriginalTri.normal, (Real)0.333333333333333333333 * (seedOriginalTri.vertices[0] + seedOriginalTri.vertices[1] + seedOriginalTri.vertices[2]));
		const Dir& zAxis = plane.normal();
		const uint32_t maxDir = physx::PxAbs(zAxis[0]) > physx::PxAbs(zAxis[1]) ?
		                            (physx::PxAbs(zAxis[0]) > physx::PxAbs(zAxis[2]) ? 0u : 2u) :
			                        (physx::PxAbs(zAxis[1]) > physx::PxAbs(zAxis[2]) ? 1u : 2u);
		Dir xAxis((Real)0);
		xAxis[(int32_t)(maxDir + 1) % 3] = (Real)1;
		Dir yAxis = zAxis ^ xAxis;
		yAxis.normalize();
		xAxis = yAxis ^ zAxis;
		Real signedArea = 0;
		physx::Array<uint32_t> mergedFrameIndices;
		mergedFrameIndices.pushBack(seedFrameIndex);
		for (uint32_t i = originalTriangleGroupStarts[seedFrameIndex]; i < originalTriangleGroupStarts[seedFrameIndex + 1]; ++i)
		{
			const Triangle& triangle = triangles[info[i].clippedTriangleIndex];
			const uint32_t ccw = (uint32_t)((triangle.normal | zAxis) > 0);
			const Real sign = ccw ? (Real)1 : -(Real)1;
			signedArea += sign * physx::PxAbs(triangle.area);
			const uint32_t i1 = 2 - ccw;
			const uint32_t i2 = 1 + ccw;
			planeVertices.pushBack(Vec2Real(xAxis | triangle.vertices[0], yAxis | triangle.vertices[0]));
			planeVertices.pushBack(Vec2Real(xAxis | triangle.vertices[i1], yAxis | triangle.vertices[i1]));
			planeVertices.pushBack(Vec2Real(xAxis | triangle.vertices[i2], yAxis | triangle.vertices[i2]));
		}
#if 1
		const uint32_t* freeIndexPtrStop = frameIndices.usedIndices() + frameIndices.capacity();
		for (const uint32_t* nextFreeIndexPtr = frameIndices.freeIndices(); nextFreeIndexPtr < freeIndexPtrStop; ++nextFreeIndexPtr)
		{
			const uint32_t nextFreeIndex = *nextFreeIndexPtr;
			const Triangle& nextOriginalTri = originalTriangles[originalTriangleIndexStart + nextFreeIndex];
			if (nextOriginalTri.submeshIndex != seedOriginalTri.submeshIndex)
			{
				continue;	// Different submesh, don't use
			}
			if (plane.distance(nextOriginalTri.vertices[0]) > distanceTol ||
			    plane.distance(nextOriginalTri.vertices[1]) > distanceTol ||
			    plane.distance(nextOriginalTri.vertices[2]) > distanceTol)
			{
				continue;	// Not coplanar
			}
			const Interpolator& nextFreeFrame = frames[nextFreeIndex + originalTriangleIndexStart];
			// BRG - Ouch, any way to set these tolerances in a little less of an ad hoc fashion?
			if (!nextFreeFrame.equals(seedFrame, (Real)0.001, (Real)0.001, (Real)0.001, (Real)0.01, (Real)0.001))
			{
				continue;	// Frames different, don't use
			}
			// We can use this frame
			frameIndices.use(nextFreeIndex);
			mergedFrameIndices.pushBack(nextFreeIndex);
			for (uint32_t i = originalTriangleGroupStarts[nextFreeIndex]; i < originalTriangleGroupStarts[nextFreeIndex + 1]; ++i)
			{
				const Triangle& triangle = triangles[info[i].clippedTriangleIndex];
				const uint32_t ccw = (uint32_t)((triangle.normal | zAxis) > 0);
				const Real sign = ccw ? (Real)1 : -(Real)1;
				signedArea += sign * physx::PxAbs(triangle.area);
				const uint32_t i1 = 2 - ccw;
				const uint32_t i2 = 1 + ccw;
				planeVertices.pushBack(Vec2Real(xAxis | triangle.vertices[0], yAxis | triangle.vertices[0]));
				planeVertices.pushBack(Vec2Real(xAxis | triangle.vertices[i1], yAxis | triangle.vertices[i1]));
				planeVertices.pushBack(Vec2Real(xAxis | triangle.vertices[i2], yAxis | triangle.vertices[i2]));
			}
		}
#endif

		// We've collected all of the clipped triangles that fit within a single reference frame, and transformed them into the x,y plane.
		// Now process this collection.
		const uint32_t oldPlaneVertexCount = planeVertices.size();
		const bool success = mergeTriangles2D(planeVertices, linkedEdgePool, distanceTol);
		if (success && planeVertices.size() < oldPlaneVertexCount)
		{
			// Transform back into 3 space and append to cleanedMesh
			const uint32_t ccw = (uint32_t)(signedArea >= 0);
			const Real sign = ccw ? (Real)1 : (Real) - 1;
			const uint32_t vMap[3] = { 0, 2 - ccw, 1 + ccw };
			const Pos planeOffset = Pos((Real)0) + (-plane.d()) * zAxis;
			for (uint32_t i = 0; i < planeVertices.size(); i += 3)
			{
				nvidia::ExplicitRenderTriangle& cleanedTri = cleanedMesh.insert();
				VertexData vertexData[3];
				Triangle tri;
				for (uint32_t v = 0; v < 3; ++v)
				{
					const Vec2Real& planeVertex = planeVertices[i + vMap[v]];
					tri.vertices[v] = planeOffset + planeVertex[0] * xAxis + planeVertex[1] * yAxis;
				}
				tri.transform(BSPToMeshTM);
				for (uint32_t v = 0; v < 3; ++v)
				{
					seedFrame.interpolateVertexData(vertexData[v], tri.vertices[v]);
					vertexData[v].normal *= sign;
				}
				tri.toExplicitRenderTriangle(cleanedTri, vertexData);
				cleanedTri.submeshIndex = seedOriginalTri.submeshIndex;
				cleanedTri.smoothingMask = seedOriginalTri.smoothingMask;
				cleanedTri.extraDataIndex = seedOriginalTri.extraDataIndex;
			}
		}
		else
		{
			// An error occurred, or we increased the triangle count.  Just use original triangles
			for (uint32_t i = 0; i < mergedFrameIndices.size(); ++i)
			{
				for (uint32_t j = originalTriangleGroupStarts[mergedFrameIndices[i]]; j < originalTriangleGroupStarts[mergedFrameIndices[i] + 1]; ++j)
				{
					Triangle tri = triangles[info[j].clippedTriangleIndex];
					tri.transform(BSPToMeshTM);
					nvidia::ExplicitRenderTriangle& cleanedMeshTri = cleanedMesh.insert();
					VertexData vertexData[3];
					for (int v = 0; v < 3; ++v)
					{
						frames[info[j].originalTriangleIndex].interpolateVertexData(vertexData[v], tri.vertices[v]);
						if (!info[j].ccw)
						{
							vertexData[v].normal *= -1.0;
						}
					}
					tri.toExplicitRenderTriangle(cleanedMeshTri, vertexData);
				}
			}
		}
	}

	return;
}

void
cleanMesh(physx::Array<nvidia::ExplicitRenderTriangle>& cleanedMesh, const physx::Array<Triangle>& mesh, physx::Array<ClippedTriangleInfo>& triangleInfo, const physx::Array<Plane>& planes, const physx::Array<Triangle>& originalTriangles, const physx::Array<Interpolator>& frames, Real distanceTol, const Mat4Real& BSPToMeshTM)
{
	cleanedMesh.clear();

	// Sort triangles into splitting plane groups, then original triangle groups
	qsort(triangleInfo.begin(), triangleInfo.size(), sizeof(ClippedTriangleInfo), ClippedTriangleInfo::cmp);

	physx::Array<uint32_t> planeGroupStarts;
	nvidia::createIndexStartLookup(planeGroupStarts, 0, planes.size(), (int32_t*)&triangleInfo.begin()->planeIndex, triangleInfo.size(), sizeof(ClippedTriangleInfo));

	nvidia::Pool<LinkedEdge2D> linkedEdgePool;
	for (uint32_t i = 0; i < planes.size(); ++i)
	{
		mergeTriangles(cleanedMesh, mesh.begin(), frames.begin(), triangleInfo.begin() + planeGroupStarts[i], planeGroupStarts[i + 1] - planeGroupStarts[i], originalTriangles, planes[i], linkedEdgePool, distanceTol, BSPToMeshTM);
	}
}

}
#endif
