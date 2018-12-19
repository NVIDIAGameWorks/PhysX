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
#include "PxSimpleTypes.h"
#include "PxErrorCallback.h"

#include "authoring/ApexCSGHull.h"
#include "authoring/ApexCSGSerialization.h"
#include "ApexSharedSerialization.h"
#include <stdio.h>

#ifndef WITHOUT_APEX_AUTHORING

#define EPS			((Real)1.0e-6)

#define SOFT_ASSERT(x)	//PX_ASSERT(x)

using namespace nvidia;


/* Local utilities */

static int compareEdgeFirstFaceIndices(const void* a, const void* b)
{
	return (int)((const ApexCSG::Hull::Edge*)a)->m_indexF1 - (int)((const ApexCSG::Hull::Edge*)b)->m_indexF1;
}


/* Hull */

void ApexCSG::Hull::intersect(const ApexCSG::Plane& plane, ApexCSG::Real distanceTol)
{
	if (isEmptySet())
	{
		return;
	}

	if (isAllSpace())
	{
		Plane& newFace = faces.insert();
		newFace = plane;
		allSpace = false;
		return;
	}

	const Dir planeNormal = plane.normal();

	if (edges.size() == 0)
	{
		PX_ASSERT(vectors.size() == 0);
		PX_ASSERT(faces.size() == 1 || faces.size() == 2);

		bool addFace = true;
		const uint32_t newFaceIndex = faces.size();

		// Nothing but a plane or two.
		for (uint32_t i = 0; i < newFaceIndex; ++i)
		{
			Plane& faceI = faces[i];
			const Dir normalI = faceI.normal();
			Dir edgeDir = normalI ^ planeNormal;
			const Real e2 = edgeDir | edgeDir;	// = 1-cosAngle*cosAngle
			const Real cosAngle = normalI | planeNormal;
			if (e2 < EPS * EPS)
			{
				if (cosAngle > 0)
				{
					// Parallel case
					if (plane.d() <= faceI.d())
					{
						SOFT_ASSERT(testConsistency(100 * distanceTol, (Real)1.0e-8));
						return;	// Plane is outside of an existing plane
					}
					faceI = plane;	// Plane is inside an existing plane - replace
					addFace = false;
				}
				else
				{
					// Antiparallel case
					if (-plane.d() < faceI.d() + distanceTol)
					{
						setToEmptySet();	// Halfspaces are mutually exclusive
						return;
					}
				}
			}
			else
			{
				// Intersecting case - add an edge
				const Real recipE2 = (Real)1 / e2;
				edgeDir *= physx::PxSqrt(recipE2);
				const Pos orig = Pos((Real)0) + (recipE2 * (faceI.d() * cosAngle - plane.d())) * planeNormal + (recipE2 * (plane.d() * cosAngle - faceI.d())) * normalI;
				if (vectors.size() == 0)
				{
					vectors.resize(newFaceIndex + 1);
					vectors[newFaceIndex] = edgeDir;
				}
				vectors[i] = orig;
				Edge& newEdge = edges.insert();
				newEdge.m_indexV0 = i;
				newEdge.m_indexV1 = newFaceIndex;	// We will have as many edges as old faces.  This will be the edge direction vector index.
				if (i == 0)
				{
					newEdge.m_indexF1 = i;
					newEdge.m_indexF2 = newFaceIndex;
				}
				else
				{
					// Keep the orientation and edge directions the same
					newEdge.m_indexF1 = newFaceIndex;
					newEdge.m_indexF2 = i;
				}
			}
		}

		if (addFace)
		{
			Plane& newFace = faces.insert();
			newFace = plane;
		}

		SOFT_ASSERT(testConsistency(100 * distanceTol, (Real)1.0e-8));
		return;
	}

	// Hull has edges.  If they are lines, and parallel to the plane, this will continue to be the case.
	if (vertexCount == 0)
	{
		PX_ASSERT(vectors.size() >= 2);
		const Dir edgeDir = getDir(edges[0]);	// All line edges will be in the same direction
		PX_ASSERT(vectors[vectors.size() - 1][3] == (Real)0);	// The last vector should be a direction (the edge direction);
		const Real den = edgeDir | planeNormal;
		if (physx::PxAbs(den) <= EPS)
		{
			//	Lines are parallel to plane
			//	This is essentially a 2-d algorithm, edges->vertices, faces->edges
			bool negClassEmpty = true;
			bool posClassEmpty = true;
			int8_t* edgeClass = (int8_t*)PxAlloca((edges.size() + 2) * sizeof(int8_t));	// Adding 2 for "edges at infinity"
			struct FaceEdge
			{
				int32_t	e1, e2;
			};
			FaceEdge* faceEdges = (FaceEdge*)PxAlloca(sizeof(FaceEdge) * faces.size() + 1);// Adding 1 for possible new face
			uint32_t* faceIndices = (uint32_t*)PxAlloca(sizeof(uint32_t) * (faces.size() + 1));	// Adding 1 for possible new face
			uint32_t* faceRemap = (uint32_t*)PxAlloca(sizeof(uint32_t) * (faces.size() + 1));	// Adding 1 for possible new face
			for (uint32_t i = 0; i < faces.size(); ++i)
			{
				FaceEdge& faceEdge = faceEdges[i];
				faceEdge.e2 = faceEdge.e1 = 0x80000000;	// Indicates unset
				faceIndices[i] = i;
				faceRemap[i] = i;
			}

			// Classify the actual edges
			for (uint32_t i = 0; i < edges.size(); ++i)
			{
				// The inequalities are set up so that when distanceTol = 0, class 0 is empty and classes -1 and 1 are disjoint
				Edge& edge = edges[i];
				PX_ASSERT(faceEdges[edge.m_indexF1].e2 == INT32_MIN);
				faceEdges[edge.m_indexF1].e2 = (int32_t)i;
				PX_ASSERT(faceEdges[edge.m_indexF2].e1 == INT32_MIN);
				faceEdges[edge.m_indexF2].e1 = (int32_t)i;
				const Real dist = plane.distance(getV0(edge));
				if (dist < -distanceTol)
				{
					edgeClass[i] = -1;
					negClassEmpty = false;
				}
				else if (dist < distanceTol)
				{
					edgeClass[i] = 0;
				}
				else
				{
					edgeClass[i] = 1;
					posClassEmpty = false;
				}
			}

			// Also classify fictitious "edges at infinity" for a complete description
			uint32_t halfInfinitePlaneCount = 0;
			for (uint32_t i = 0; i < faces.size(); ++i)
			{
				Plane& face = faces[i];
				FaceEdge& faceEdge = faceEdges[i];
				PX_ASSERT(faceEdge.e2 != INT32_MIN || faceEdge.e1 != INT32_MIN);
				if (faceEdge.e2 == INT32_MIN || faceEdge.e1 == INT32_MIN)
				{
					PX_ASSERT(halfInfinitePlaneCount < 2);
					if (halfInfinitePlaneCount < 2)
					{
						const int32_t classIndex = (int32_t)(edges.size() + halfInfinitePlaneCount++);

						Real cosTheta = (edgeDir ^ face.normal()) | planeNormal;
						uint32_t realEdgeIndex;
						if (faceEdge.e2 == INT32_MIN)
						{
							faceEdge.e2 = -classIndex;	// Negating => fictitious edge.
							realEdgeIndex = (uint32_t)faceEdge.e1;
						}
						else
						{
							faceEdge.e1 = -classIndex;	// Negating => fictitious edge.
							cosTheta *= -1;
							realEdgeIndex = (uint32_t)faceEdge.e2;
						}

						if (cosTheta < -EPS)
						{
							edgeClass[classIndex] = -1;
							negClassEmpty = false;
						}
						else if (cosTheta < EPS)
						{
							const Real d = plane.distance(getV0(edges[realEdgeIndex]));
							if (d < -distanceTol)
							{
								edgeClass[classIndex] = -1;
								negClassEmpty = false;
							}
							else if (d < distanceTol)
							{
								edgeClass[classIndex] = 0;
							}
							else
							{
								edgeClass[classIndex] = 1;
								posClassEmpty = false;
							}
						}
						else
						{
							edgeClass[classIndex] = 1;
							posClassEmpty = false;
						}
					}
				}
			}

			if (negClassEmpty)
			{
				setToEmptySet();
				return;
			}
			if (posClassEmpty)
			{
				SOFT_ASSERT(testConsistency(100 * distanceTol, (Real)1.0e-8));
				return;
			}

			vectors.popBack();	// We will keep the line origins contiguous, and place the edge direction at the end of the array later

			bool addFace = false;
			FaceEdge newFaceEdge = { (int32_t)0x80000000, (int32_t)0x80000000 };
			const uint32_t newFaceIndex = faces.size();

			for (uint32_t i = faces.size(); i--;)
			{
				int32_t clippedEdgeIndex = (int32_t)edges.size();
				Plane& face = faces[i];
				FaceEdge& faceEdge = faceEdges[i];
				PX_ASSERT(faceEdge.e2 != INT32_MIN && faceEdge.e1 != INT32_MIN);
				uint32_t newEdgeIndex = 0xFFFFFFFF;
				int32_t orig1Index = faceEdge.e2;
				int32_t orig2Index = faceEdge.e1;
				switch (edgeClass[physx::PxAbs(faceEdge.e2)])
				{
				case 1:
					if (edgeClass[physx::PxAbs(faceEdge.e1)] == -1)
					{
						// Clip face
						PX_ASSERT(((face.normal() ^ planeNormal) | edgeDir) > 0);
						PX_ASSERT(newFaceEdge.e1 == INT32_MIN);
						newFaceEdge.e1 = clippedEdgeIndex;
						newEdgeIndex = edges.size();
						Edge& newEdge =	edges.insert();
						newEdge.m_indexF1 = i;
						newEdge.m_indexF2 = newFaceIndex;
						faceEdge.e2 = clippedEdgeIndex;
						addFace = true;
					}
					else
					{
						// Eliminate face
						faces.replaceWithLast(i);
						faceEdges[i] = faceEdges[faces.size()];
						faceIndices[i] = faceIndices[faces.size()];
						faceRemap[faceIndices[faces.size()]] = i;
#ifdef _DEBUG	// Should not be necessary, but helps with debugging
						faceIndices[faces.size()] = 0xFFFFFFFF;
						faceRemap[i] = 0xFFFFFFFF;
#endif
					}
					break;
				case 0:
					if (edgeClass[physx::PxAbs(faceEdge.e1)] == -1)
					{
						// Keep this face, and use this edge on the new face
						clippedEdgeIndex = faceEdge.e2;
						PX_ASSERT(newFaceEdge.e1 == INT32_MIN);
						newFaceEdge.e1 = faceEdge.e2;
						edges[(uint32_t)faceEdge.e2].m_indexF2 = newFaceIndex;
						addFace = true;
					}
					else
					{
						// Eliminate face
						faces.replaceWithLast(i);
						faceEdges[i] = faceEdges[faces.size()];
						faceIndices[i] = faceIndices[faces.size()];
						faceRemap[faceIndices[faces.size()]] = i;
#ifdef _DEBUG	// Should not be necessary, but helps with debugging
						faceIndices[faces.size()] = 0xFFFFFFFF;
						faceRemap[i] = 0xFFFFFFFF;
#endif
					}
					break;
				case -1:
					switch (edgeClass[physx::PxAbs(faceEdge.e1)])
					{
					case 1:		// Clip face
					{
						PX_ASSERT(((planeNormal ^ face.normal()) | edgeDir) > 0);
						PX_ASSERT(newFaceEdge.e2 == INT32_MIN);
						newFaceEdge.e2 = clippedEdgeIndex;
						newEdgeIndex = edges.size();
						Edge& newEdge =	edges.insert();
						newEdge.m_indexF1 = newFaceIndex;
						newEdge.m_indexF2 = i;
						faceEdge.e1 = clippedEdgeIndex;
						addFace = true;
					}
					break;
					case 0:		// Keep this face, and use this edge on the new face
						clippedEdgeIndex = faceEdge.e1;
						PX_ASSERT(newFaceEdge.e2 == INT32_MIN);
						newFaceEdge.e2 = faceEdge.e1;
						edges[(uint32_t)faceEdge.e1].m_indexF1 = newFaceIndex;
						addFace = true;
						break;
					}
				}

				if (newEdgeIndex < edges.size())
				{
					Edge& newEdge =	edges[newEdgeIndex];
					newEdge.m_indexV0 = vectors.size();
					newEdge.m_indexV1 = 0xFFFFFFFF;	// Will be replaced below
					Pos& newOrig = *(Pos*)&vectors.insert();
					if (orig1Index >= 0)
					{
						const Edge& e1 = edges[(uint32_t)orig1Index];
						const Pos& o1 = getV0(e1);
						const Real d1 = plane.distance(o1);
						if (orig2Index >= 0)
						{
							const Edge& e2 = edges[(uint32_t)orig2Index];
							const Pos& o2 = getV0(e2);
							const Real d2 = plane.distance(o2);
							newOrig = o1 + (d1 / (d1 - d2)) * (o2 - o1);

						}
						else
						{
							const Dir tangent = face.normal() ^ edgeDir;
							const Real cosTheta = tangent | planeNormal;
							PX_ASSERT(physx::PxAbs(cosTheta) > EPS);
							newOrig = o1 - tangent * (d1 / cosTheta);
						}
					}
					else
					{
						const Dir tangent = edgeDir ^ face.normal();
						const Real cosTheta = tangent | planeNormal;
						PX_ASSERT(physx::PxAbs(cosTheta) > EPS);
						if (orig2Index >= 0)
						{
							const Edge& e2 = edges[(uint32_t)orig2Index];
							const Pos& o2 = getV0(e2);
							const Real d2 = plane.distance(o2);
							newOrig = o2 - tangent * (d2 / cosTheta);
						}
						else
						{
							PX_ALWAYS_ASSERT();	// Should not have any full planes
						}
					}
				}
			}

			if (addFace)
			{
				faceRemap[newFaceIndex] = faces.size();
				faceIndices[faces.size()] = newFaceIndex;
				faceEdges[faces.size()] = newFaceEdge;
				Plane& newFace = faces.insert();
				newFace = plane;
			}

			if (faces.size() == 0)
			{
				setToEmptySet();
				SOFT_ASSERT(testConsistency(100 * distanceTol, (Real)1.0e-8));
				return;
			}

			// Replacing edge direction, and re-indexing
			const uint32_t edgeDirIndex = vectors.size();
			vectors.pushBack(edgeDir);
			for (uint32_t i = 0; i < edges.size(); ++i)
			{
				edges[i].m_indexV1 = edgeDirIndex;
			}

			// Eliminate unused edges (and remap face indices in remaining edges)
			uint8_t* edgeMarks = (uint8_t*)PxAlloca(sizeof(uint8_t) * edges.size());
			memset(edgeMarks, 0, sizeof(uint8_t)*edges.size());

			for (uint32_t i = 0; i < faces.size(); ++i)
			{
				FaceEdge& faceEdge = faceEdges[i];
				if (faceEdge.e2 >= 0)
				{
					edgeMarks[faceEdge.e2] = 1;
				}
				if (faceEdge.e1 >= 0)
				{
					edgeMarks[faceEdge.e1] = 1;
				}
			}

			for (uint32_t i = edges.size(); i--;)
			{
				if (edgeMarks[i] == 0)
				{
					edges.replaceWithLast(i);
				}
				else
				{
					Edge& edge = edges[i];
					PX_ASSERT(faceRemap[edge.m_indexF1] != 0xFFFFFFFF);
					if (faceRemap[edge.m_indexF1] != 0xFFFFFFFF)
					{
						edge.m_indexF1 = faceRemap[edge.m_indexF1];
					}
					PX_ASSERT(faceRemap[edge.m_indexF2] != 0xFFFFFFFF);
					if (faceRemap[edge.m_indexF2] != 0xFFFFFFFF)
					{
						edge.m_indexF2 = faceRemap[edge.m_indexF2];
					}
				}
			}

			// Eliminate unused vectors (and remap vertex indices in edges)
			int32_t* vectorOffsets = (int32_t*)PxAlloca(sizeof(int32_t) * vectors.size());
			memset(vectorOffsets, 0, sizeof(int32_t)*vectors.size());

			for (uint32_t i = 0; i < edges.size(); ++i)
			{
				Edge& edge = edges[i];
				++vectorOffsets[edge.m_indexV0];
				++vectorOffsets[edge.m_indexV1];
			}

			uint32_t vectorCount = 0;
			for (uint32_t i = 0; i < vectors.size(); ++i)
			{
				const bool copy = vectorOffsets[i] > 0;
				vectorOffsets[i] = (int32_t)vectorCount - (int32_t)i;
				if (copy)
				{
					vectors[vectorCount++] = vectors[i];
				}
			}
			vectors.resize(vectorCount);

			for (uint32_t i = 0; i < edges.size(); ++i)
			{
				Edge& edge = edges[i];
				edge.m_indexV0 += vectorOffsets[edge.m_indexV0];
				edge.m_indexV1 += vectorOffsets[edge.m_indexV1];
			}

			PX_ASSERT(vectors.size() == edges.size() + 1);

			SOFT_ASSERT(testConsistency(100 * distanceTol, (Real)1.0e-8));

			return;
		}
	}

	// The hull will have vertices

	// Compare vertex positions to input plane
	bool negClassEmpty = true;
	bool posClassEmpty = true;
	int8_t* vertexClass = (int8_t*)PxAlloca(vertexCount * sizeof(int8_t));
	for (uint32_t i = 0; i < vertexCount; ++i)
	{
		// The inequalities are set up so that when distanceTol = 0, class 0 is empty and classes -1 and 1 are disjoint
		const Real dist = plane.distance(getVertex(i));
		if (dist < -distanceTol)
		{
			vertexClass[i] = -1;
			negClassEmpty = false;
		}
		else if (dist < distanceTol)
		{
			vertexClass[i] = 0;
		}
		else
		{
			vertexClass[i] = 1;
			posClassEmpty = false;
		}
	}

	if (vertexCount != 0)
	{
		// Also test "points at infinity" for better culling
		for (uint32_t i = vertexCount; i < vectors.size(); ++i)
		{
			if (vectors[i][3] < (Real)0.5)
			{
				const Real cosTheta = plane | vectors[i];
				if (cosTheta < -EPS)
				{
					negClassEmpty = false;
				}
				else if (cosTheta >= EPS)
				{
					posClassEmpty = false;
				}
			}
		}

		if (negClassEmpty)
		{
			setToEmptySet();
			return;
		}
		if (posClassEmpty)
		{
			SOFT_ASSERT(testConsistency(100 * distanceTol, (Real)1.0e-8));
			return;
		}
	}

	// Intersect new plane against edges.
	const uint32_t newFaceIndex = faces.size();

	// Potential number of new edges is the number of faces
	Edge* newEdges = (Edge*)PxAlloca((newFaceIndex + 1) * sizeof(Edge));
	memset(newEdges, 0xFF, (newFaceIndex + 1)*sizeof(Edge));

	bool addFace = false;

	for (uint32_t i = edges.size(); i--;)
	{
		Edge& edge = edges[i];
		uint32_t clippedVertexIndex = vectors.size();
		bool createNewEdge = false;
		switch (getType(edge))
		{
		case EdgeType::LineSegment:
			switch (vertexClass[edge.m_indexV0])
			{
			case -1:
				switch (vertexClass[edge.m_indexV1])
				{
				case 1:
				{
					// Clip this edge.
					const Pos& v0 = getV0(edge);
					const Real d0 = plane.distance(v0);
					const Pos& v1 = getV1(edge);
					const Real d1 = plane.distance(v1);
					const Pos clipVertex = v0 + (d0 / (d0 - d1)) * (v1 - v0);
					edge.m_indexV1 = clippedVertexIndex;
					vectors.pushBack(clipVertex);
					createNewEdge = true;
					break;
				}
				case 0:
					createNewEdge = true;
					clippedVertexIndex = edge.m_indexV1;
					break;
				}
				break;
			case 0:
				if (vertexClass[edge.m_indexV1] == -1)
				{
					createNewEdge = true;
					clippedVertexIndex = edge.m_indexV0;
				}
				else
				{
					// Eliminate this edge
					edges.replaceWithLast(i);
				}
				break;
			case 1:
				if (vertexClass[edge.m_indexV1] == -1)
				{
					// Clip this edge.
					const Pos& v0 = getV0(edge);
					const Real d0 = plane.distance(v0);
					const Pos& v1 = getV1(edge);
					const Real d1 = plane.distance(v1);
					const Pos clipVertex = v1 + (d1 / (d1 - d0)) * (v0 - v1);
					edge.m_indexV0 = clippedVertexIndex;
					vectors.pushBack(clipVertex);
					createNewEdge = true;
				}
				else
				{
					// Eliminate this edge
					edges.replaceWithLast(i);
				}
				break;
			}
			break;
		case EdgeType::Ray:
		{
			const Dir& edgeDir = getDir(edge);
			const Real den = edgeDir | planeNormal;
			switch (vertexClass[edge.m_indexV0])
			{
			case -1:
				if (den > EPS)
				{
					// Clip this edge.
					const Pos& v0 = getV0(edge);
					const Real d0 = plane.distance(v0);
					const Pos clipVertex = v0 - edgeDir * (d0 / den);
					edge.m_indexV1 = clippedVertexIndex;
					vectors.pushBack(clipVertex);
					createNewEdge = true;
				}
				break;
			case 0:
				if (den < -EPS)
				{
					createNewEdge = true;
					clippedVertexIndex = edge.m_indexV0;
				}
				else
				{
					// Eliminate this edge
					edges.replaceWithLast(i);
				}
				break;
			case 1:
				if (den < -EPS)
				{
					// Clip this edge.
					const Pos& v0 = getV0(edge);
					const Real d0 = plane.distance(v0);
					const Pos clipVertex = v0 - edgeDir * (d0 / den);
					edge.m_indexV0 = clippedVertexIndex;
					vectors.pushBack(clipVertex);
					createNewEdge = true;
				}
				else
				{
					// Eliminate this edge
					edges.replaceWithLast(i);
				}
				break;
			}
		}
		break;
		case EdgeType::Line:
		{
			const Pos& point = getV0(edge);
			const Dir& edgeDir = getDir(edge);
			const Real h = plane.distance(point);
			const Real den = edgeDir | planeNormal;
			PX_ASSERT(physx::PxAbs(den) >= EPS);
			// Clip this edge
			clippedVertexIndex = edge.m_indexV0;	// Re-use this vector (will become a vertex)
			vectors[clippedVertexIndex] = point - edgeDir * (h / den);
			if (den > 0)	// Make sure the ray points in the correct direction
			{
				vectors[edge.m_indexV1] *= -(Real)1;
				nvidia::swap(edge.m_indexF1, edge.m_indexF2);
			}
			createNewEdge = true;
		}
		break;
		}

		if (createNewEdge)
		{
			if (newEdges[edge.m_indexF1].m_indexV0 == 0xFFFFFFFF)
			{
				newEdges[edge.m_indexF1].m_indexV0 = clippedVertexIndex;
				PX_ASSERT(newEdges[edge.m_indexF1].m_indexV1 == 0xFFFFFFFF);
				newEdges[edge.m_indexF1].m_indexV1 = vectors.size();
				Dir newDir = planeNormal ^ faces[edge.m_indexF1].normal();
				newDir.normalize();
				if ((newDir | faces[edge.m_indexF2].normal()) > 0)
				{
					newDir *= -(Real)1;
					newEdges[edge.m_indexF1].m_indexF1 = edge.m_indexF1;
					newEdges[edge.m_indexF1].m_indexF2 = newFaceIndex;
				}
				else
				{
					newEdges[edge.m_indexF1].m_indexF1 = newFaceIndex;
					newEdges[edge.m_indexF1].m_indexF2 = edge.m_indexF1;
				}
				vectors.pushBack(newDir);
				addFace = true;
			}
			else
			{
				PX_ASSERT(newEdges[edge.m_indexF1].m_indexV1 != 0xFFFFFFFF && vectors[newEdges[edge.m_indexF1].m_indexV1][3] == (Real)0);
				newEdges[edge.m_indexF1].m_indexV1 = clippedVertexIndex;
			}
			if (newEdges[edge.m_indexF2].m_indexV0 == 0xFFFFFFFF)
			{
				newEdges[edge.m_indexF2].m_indexV0 = clippedVertexIndex;
				PX_ASSERT(newEdges[edge.m_indexF2].m_indexV1 == 0xFFFFFFFF);
				newEdges[edge.m_indexF2].m_indexV1 = vectors.size();
				Dir newDir = faces[edge.m_indexF2].normal() ^ planeNormal;
				newDir.normalize();
				if ((newDir | faces[edge.m_indexF1].normal()) > 0)
				{
					newDir *= -(Real)1;
					newEdges[edge.m_indexF2].m_indexF1 = newFaceIndex;
					newEdges[edge.m_indexF2].m_indexF2 = edge.m_indexF2;
				}
				else
				{
					newEdges[edge.m_indexF2].m_indexF1 = edge.m_indexF2;
					newEdges[edge.m_indexF2].m_indexF2 = newFaceIndex;
				}
				vectors.pushBack(newDir);
				addFace = true;
			}
			else
			{
				PX_ASSERT(newEdges[edge.m_indexF2].m_indexV1 != 0xFFFFFFFF && vectors[newEdges[edge.m_indexF2].m_indexV1][3] == (Real)0);
				newEdges[edge.m_indexF2].m_indexV1 = clippedVertexIndex;
			}
		}
	}

	if (addFace)
	{
		Plane& newFace = faces.insert();
		newFace = plane;
	}

	for (uint32_t i = 0; i < faces.size(); ++i)
	{
		Edge& newEdge = newEdges[i];
		if (newEdge.m_indexV0 != 0xFFFFFFFF)
		{
			if (newEdge.m_indexV0 != newEdge.m_indexV1)	// Skip split vertices
			{
				edges.pushBack(newEdge);
			}
		}
	}

	// Now eliminate unused faces and vectors
	int32_t* vectorOffsets = (int32_t*)PxAlloca(sizeof(int32_t) * vectors.size());
	int32_t* faceOffsets = (int32_t*)PxAlloca(sizeof(int32_t) * faces.size());
	memset(vectorOffsets, 0, sizeof(int32_t)*vectors.size());
	memset(faceOffsets, 0, sizeof(int32_t)*faces.size());

	for (uint32_t i = 0; i < edges.size(); ++i)
	{
		Edge& edge = edges[i];
		++vectorOffsets[edge.m_indexV0];
		++vectorOffsets[edge.m_indexV1];
		++faceOffsets[edge.m_indexF1];
		++faceOffsets[edge.m_indexF2];
	}

	uint32_t vectorCount = 0;
	for (uint32_t i = 0; i < vectors.size(); ++i)
	{
		const bool copy = vectorOffsets[i] > 0;
		vectorOffsets[i] = (int32_t)vectorCount - (int32_t)i;
		if (copy)
		{
			vectors[vectorCount++] = vectors[i];
		}
	}
	vectors.resize(vectorCount);

	uint32_t faceCount = 0;
	for (uint32_t i = 0; i < faces.size(); ++i)
	{
		const bool copy = faceOffsets[i] > 0;
		faceOffsets[i] = (int32_t)faceCount - (int32_t)i;
		if (copy)
		{
			faces[faceCount++] = faces[i];
		}
	}
	faces.resize(faceCount);

	PX_ASSERT(faceCount != 1);	// Single faces would have been handled above.

	for (uint32_t i = 0; i < edges.size(); ++i)
	{
		Edge& edge = edges[i];
		edge.m_indexV0 += vectorOffsets[edge.m_indexV0];
		edge.m_indexV1 += vectorOffsets[edge.m_indexV1];
		edge.m_indexF1 += faceOffsets[edge.m_indexF1];
		edge.m_indexF2 += faceOffsets[edge.m_indexF2];
	}

	if (faces.size() == 0)
	{
		setToEmptySet();
	}

	// Now sort vectors, vertices before directions.  We will need to keep track of the mapping to re-index the edges.
	uint32_t* vectorIndices = (uint32_t*)PxAlloca(sizeof(uint32_t) * vectors.size());
	uint32_t* vectorRemap = (uint32_t*)PxAlloca(sizeof(uint32_t) * vectors.size());
	for (uint32_t i = 0; i < vectors.size(); ++i)
	{
		vectorIndices[i] = vectorRemap[i] = i;
	}
	int32_t firstDirIndex = -1;
	int32_t lastPosIndex = (int32_t)vectors.size();
	for (;;)
	{
		// Correct this
		while (++firstDirIndex < (int32_t)vectors.size())
		{
			if (vectors[(uint32_t)firstDirIndex][3] < (Real)0.5)	// Should be 0 or 1, but in case some f.p. inexactness creeps in...
			{
				break;
			}
		}
		while (--lastPosIndex >= 0)
		{
			if (vectors[(uint32_t)lastPosIndex][3] >= (Real)0.5)	// Should be 0 or 1, but in case some f.p. inexactness creeps in...
			{
				break;
			}
		}
		if (firstDirIndex > lastPosIndex)
		{
			break;	// All's good
		}
		// Fix this - swap vectors, and update map
		nvidia::swap(vectors[(uint32_t)firstDirIndex], vectors[(uint32_t)lastPosIndex]);
		nvidia::swap(vectorIndices[(uint32_t)firstDirIndex], vectorIndices[(uint32_t)lastPosIndex]);
		vectorRemap[vectorIndices[(uint32_t)firstDirIndex]] = (uint32_t)firstDirIndex;
		vectorRemap[vectorIndices[(uint32_t)lastPosIndex]] = (uint32_t)lastPosIndex;
	}
	vertexCount = (uint32_t)firstDirIndex;	// Correct vertex count

	// Correct edge indices
	for (uint32_t i = 0; i < edges.size(); ++i)
	{
		Edge& edge = edges[i];
		edge.m_indexV0 = vectorRemap[edge.m_indexV0];
		edge.m_indexV1 = vectorRemap[edge.m_indexV1];
	}

	SOFT_ASSERT(testConsistency(100 * distanceTol, (Real)1.0e-8));
}

ApexCSG::Real ApexCSG::Hull::calculateVolume() const
{
	if (isEmptySet())
	{
		return (Real)0;
	}

	if (edges.size() == 0 || vectors.size() > vertexCount)
	{
		return MAX_REAL;
	}

	// Volume is finite
	PX_ASSERT(vertexCount != 0);

	// Work relative to an internal point, for better accuracy
	Vec4Real centroid((Real)0);
	for (uint32_t i = 0; i < vertexCount; ++i)
	{
		centroid += vectors[i];
	}
	centroid /= (Real)vertexCount;

	// Create a doubled edge list
	const uint32_t edgeCount = edges.size();
	const uint32_t edgeListSize = 2 * edgeCount;
	Edge* edgeList = (Edge*)PxAlloca(edgeListSize * sizeof(Edge));
	for (uint32_t i = 0; i < edgeCount; ++i)
	{
		edgeList[i] = edges[i];
		edgeList[i + edgeCount] = edges[i];
		nvidia::swap(edgeList[i + edgeCount].m_indexF1, edgeList[i + edgeCount].m_indexF2);
		nvidia::swap(edgeList[i + edgeCount].m_indexV0, edgeList[i + edgeCount].m_indexV1);
	}

	// Sort edgeList by first face index
	qsort(edgeList, edgeListSize, sizeof(Edge), compareEdgeFirstFaceIndices);

	// A scratchpad for edge vertex locations
	uint32_t* vertex0Locations = (uint32_t*)PxAlloca(vertexCount * sizeof(uint32_t));
	memset(vertex0Locations, 0xFF, vertexCount * sizeof(uint32_t));

	Real volume = 0;

	// Edges are grouped by first face index - each group describes the polygonal face.
	uint32_t groupStop = 0;
	do
	{
		const uint32_t groupStart = groupStop;
		const uint32_t faceIndex = edgeList[groupStart].m_indexF1;
		while (++groupStop < edgeListSize && edgeList[groupStop].m_indexF1 == faceIndex) {}
		// Evaluate group
		if (groupStop - groupStart >= 3)
		{
			// Mark first vertex locations within group
			for (uint32_t i = groupStart; i < groupStop; ++i)
			{
				Edge& edge = edgeList[i];
				SOFT_ASSERT(vertex0Locations[edge.m_indexV0] == 0xFFFFFFFF);
				vertex0Locations[edge.m_indexV0] = i;
			}
			const Dir d0 = vectors[edgeList[groupStart].m_indexV0] - centroid;
			uint32_t i1 = edgeList[groupStart].m_indexV1;
			Dir d1 = vectors[i1] - centroid;
			const uint32_t tetCount = groupStop - groupStart - 2;
			for (uint32_t i = 0; i < tetCount; ++i)
			{
				const uint32_t nextEdgeIndex = vertex0Locations[i1];
				SOFT_ASSERT(nextEdgeIndex != 0xFFFFFFFF);
				if (nextEdgeIndex == 0xFFFFFFFF)
				{
					break;
				}
				const uint32_t i2 = edgeList[nextEdgeIndex].m_indexV1;
				const Dir d2 = vectors[i2] - centroid;
				const Real tripleProduct = d0 | (d1 ^ d2);
				SOFT_ASSERT(tripleProduct > -EPS_REAL);
				volume += tripleProduct;	// 6 times volume
				i1 = i2;
				d1 = d2;
			}
			// Unmark first vertex locations
			for (uint32_t i = groupStart; i < groupStop; ++i)
			{
				Edge& edge = edgeList[i];
				vertex0Locations[edge.m_indexV0] = 0xFFFFFFFF;
			}
		}
	}
	while (groupStop < edgeListSize);

	volume *= (Real)0.166666666666666667;

	return volume;
}

void ApexCSG::Hull::serialize(physx::PxFileBuf& stream) const
{
	apex::serialize(stream, faces);
	apex::serialize(stream, edges);
	apex::serialize(stream, vectors);
	stream << vertexCount;
	stream << allSpace;
	stream << emptySet;
}

void ApexCSG::Hull::deserialize(physx::PxFileBuf& stream, uint32_t version)
{
	setToAllSpace();

	apex::deserialize(stream, version, faces);
	apex::deserialize(stream, version, edges);
	apex::deserialize(stream, version, vectors);
	stream >> vertexCount;
	stream >> allSpace;
	stream >> emptySet;
}

bool ApexCSG::Hull::testConsistency(ApexCSG::Real distanceTol, ApexCSG::Real angleTol) const
{
	bool halfInfiniteEdges = false;
	for (uint32_t j = 0; j < edges.size(); ++j)
	{
		if (edges[j].m_indexV0 < vertexCount && edges[j].m_indexV1 >= vertexCount)
		{
			halfInfiniteEdges = true;
			break;
		}
	}

	for (uint32_t i = 0; i < vertexCount; ++i)
	{
		uint32_t count = 0;
		for (uint32_t j = 0; j < edges.size(); ++j)
		{
			if (edges[j].m_indexV0 == i)
			{
				++count;
			}
			if (edges[j].m_indexV1 == i)
			{
				++count;
			}
			if (edges[j].m_indexV1 < vertexCount && edges[j].m_indexV0 == edges[j].m_indexV1)
			{
				GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING, "Hull::testConsistency: 0-length edge found.", __FILE__, __LINE__);
				PX_ALWAYS_ASSERT();
				return false;
			}
		}
		if (count < 3)
		{
			GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING, "Hull::testConsistency: vertex connected to fewer than 3 edges.", __FILE__, __LINE__);
			PX_ALWAYS_ASSERT();
			return false;
		}
	}

	if (edges.size() == 0)
	{
		if (faces.size() >= 3)
		{
			GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING,  "Hull::testConsistency: face count 3 or greater, but with no edges.", __FILE__, __LINE__);
			PX_ALWAYS_ASSERT();
			return false;
		}
		if (vertexCount > 0)
		{
			GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING,  "Hull::testConsistency: hull has vertices but no edges.", __FILE__, __LINE__);
			PX_ALWAYS_ASSERT();
			return false;
		}
		return true;
	}

	bool halfInfiniteFaces = false;

	for (uint32_t i = 0; i < faces.size(); ++i)
	{
		uint32_t count = 0;
		for (uint32_t j = 0; j < edges.size(); ++j)
		{
			if (edges[j].m_indexF1 == i)
			{
				++count;
			}
			if (edges[j].m_indexF2 == i)
			{
				++count;
			}
			if (edges[j].m_indexF1 == edges[j].m_indexF2)
			{
				GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING,  "Hull::testConsistency: edge connecting face to itself.", __FILE__, __LINE__);
				PX_ALWAYS_ASSERT();
				return false;
			}
		}
		if (count == 0)
		{
			GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING,  "Hull::testConsistency: face has no edges", __FILE__, __LINE__);
			PX_ALWAYS_ASSERT();
			return false;
		}
		if (count == 1)
		{
			halfInfiniteFaces = true;
		}
	}

	for (uint32_t i = 0; i < edges.size(); ++i)
	{
		Real d;
		const Edge& edge = edges[i];
		if (edge.m_indexV1 >= vertexCount)
		{
			// Edge is a line or ray
			d = faces[edge.m_indexF1].distance(getV0(edge));
			if (physx::PxAbs(d) > distanceTol)
			{
				GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING,  "Hull::testConsistency: edge line/ray origin not on face.", __FILE__, __LINE__);
				PX_ALWAYS_ASSERT();
				return false;
			}
			d = faces[edge.m_indexF1].normal() | getDir(edge);
			if (physx::PxAbs(d) > angleTol)
			{
				GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING,  "Hull::testConsistency: edge line/ray direction not perpendicular to face.", __FILE__, __LINE__);
				PX_ALWAYS_ASSERT();
				return false;
			}
			d = faces[edge.m_indexF2].distance(getV0(edge));
			if (physx::PxAbs(d) > distanceTol)
			{
				GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING,  "Hull::testConsistency: edge line/ray origin not on face.", __FILE__, __LINE__);
				PX_ALWAYS_ASSERT();
				return false;
			}
			d = faces[edge.m_indexF2].normal() | getDir(edge);
			if (physx::PxAbs(d) > angleTol)
			{
				GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING,  "Hull::testConsistency: edge line/ray direction not perpendicular to face.", __FILE__, __LINE__);
				PX_ALWAYS_ASSERT();
				return false;
			}
		}
		else
		{
			// Edge is a line segment
			if (edge.m_indexV0 >= vertexCount)
			{
				GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING,  "Hull::testConsistency: edge has a line/ray origin but a real destination point.", __FILE__, __LINE__);
				PX_ALWAYS_ASSERT();
				return false;
			}
			d = faces[edge.m_indexF1].distance(getV0(edge));
			if (physx::PxAbs(d) > distanceTol)
			{
				GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING,  "Hull::testConsistency: edge vertex 0 not on face.", __FILE__, __LINE__);
				PX_ALWAYS_ASSERT();
				return false;
			}
			d = faces[edge.m_indexF1].distance(getV1(edge));
			if (physx::PxAbs(d) > distanceTol)
			{
				GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING,  "Hull::testConsistency: edge vertex 1 not on face.", __FILE__, __LINE__);
				PX_ALWAYS_ASSERT();
				return false;
			}
			d = faces[edge.m_indexF2].distance(getV0(edge));
			if (physx::PxAbs(d) > distanceTol)
			{
				GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING,  "Hull::testConsistency: edge vertex 0 not on face.", __FILE__, __LINE__);
				PX_ALWAYS_ASSERT();
				return false;
			}
			d = faces[edge.m_indexF2].distance(getV1(edge));
			if (physx::PxAbs(d) > distanceTol)
			{
				GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING,  "Hull::testConsistency: edge vertex 1 not on face.", __FILE__, __LINE__);
				PX_ALWAYS_ASSERT();
				return false;
			}
		}
	}

	if (vertexCount == 0)
	{
		if (!halfInfiniteFaces)
		{
			if (faces.size() != edges.size())
			{
				GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING,  "Hull::testConsistency: hull has edges but no vertices, with no half-infinite faces.  Face count should equal edge count but does not.", __FILE__, __LINE__);
				PX_ALWAYS_ASSERT();
				return false;
			}
		}
		else
		{
			if (faces.size() != edges.size() + 1)
			{
				GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING,  "Hull::testConsistency: hull has edges but no vertices, with half-infinite faces.  Face count should be one more than edge count but is not.", __FILE__, __LINE__);
				PX_ALWAYS_ASSERT();
				return false;
			}
		}
		const Edge& edge0 = edges[0];
		for (uint32_t i = 1; i < edges.size(); ++i)
		{
			const Edge& edge = edges[i];
			Dir e = getDir(edge0) ^ getDir(edge);
			if ((e | e) > (Real)EPS * EPS)
			{
				GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING,  "Hull::testConsistency: prism edges not all facing the same direction.", __FILE__, __LINE__);
				PX_ALWAYS_ASSERT();
				return false;
			}
		}
	}
	else
	{
		const uint32_t c = faces.size() - edges.size() + vertexCount;
		if (!halfInfiniteEdges)
		{
			if (c != 2)
			{
				GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING,  "Hull::testConsistency: bounded hull with faces, vertices and edges does not obey Euler's formula.", __FILE__, __LINE__);
				PX_ALWAYS_ASSERT();
				return false;
			}
		}
		else
		{
			if (c != 1)
			{
				GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING,  "Hull::testConsistency: unbounded hull with faces, vertices and edges does not obey Euler's formula (with a vertex at infinity).", __FILE__, __LINE__);
				PX_ALWAYS_ASSERT();
				return false;
			}
		}
	}

	return true;
}
#endif