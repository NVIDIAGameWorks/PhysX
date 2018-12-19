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

#include "GuDistancePointSegment.h"
#include "GuDistanceSegmentSegment.h"
#include "GuGeometryUnion.h"
#include "GuHeightFieldData.h"
#include "GuHeightFieldUtil.h"
#include "GuContactMethodImpl.h"
#include "GuContactBuffer.h"
#include "GuContactMethodImpl.h"
#include "GuInternal.h"

#define DO_EDGE_EDGE 1
#define DEBUG_HFNORMAL 0
#define DEBUG_HFNORMALV 0
#define DEBUG_RENDER_HFCONTACTS 0

#if DEBUG_RENDER_HFCONTACTS
#include "PxPhysics.h"
#include "PxScene.h"
#endif

using namespace physx;
using namespace Gu;

bool GuContactSphereHeightFieldShared(GU_CONTACT_METHOD_ARGS, bool isCapsule);

namespace physx
{
namespace Gu
{
bool legacyContactCapsuleHeightfield(GU_CONTACT_METHOD_ARGS)
{
	// Get actual shape data
	const PxCapsuleGeometry& shapeCapsule = shape0.get<const PxCapsuleGeometry>();
	const PxHeightFieldGeometryLL& hfGeom = shape1.get<const PxHeightFieldGeometryLL>();

	const HeightFieldUtil hfUtil(hfGeom);
	const Gu::HeightField& hf = hfUtil.getHeightField();

	const PxReal radius = shapeCapsule.radius;
	const PxReal inflatedRadius = shapeCapsule.radius + params.mContactDistance;
	const PxReal radiusSquared = inflatedRadius * inflatedRadius;
	const PxReal halfHeight = shapeCapsule.halfHeight;
	const PxReal eps = PxReal(0.1)*radius;
	const PxReal epsSqr = eps*eps;
	const PxReal oneOverRowScale = hfUtil.getOneOverRowScale();
	const PxReal oneOverColumnScale = hfUtil.getOneOverColumnScale();
	const PxReal radiusOverRowScale = inflatedRadius * PxAbs(oneOverRowScale);
	const PxReal radiusOverColumnScale = inflatedRadius * PxAbs(oneOverColumnScale);
	const PxTransform capsuleShapeToHfShape = transform1.transformInv(transform0);

	PxVec3 verticesInHfShape[2];
	verticesInHfShape[0] = capsuleShapeToHfShape.transform(PxVec3(-halfHeight, 0, 0));
	verticesInHfShape[1] = capsuleShapeToHfShape.transform(PxVec3(halfHeight, 0, 0));

	PX_ASSERT(contactBuffer.count==0);
	Gu::GeometryUnion u;
	u.set(PxSphereGeometry(radius));
	PxTransform ts0(transform1.transform(verticesInHfShape[0])), ts1(transform1.transform(verticesInHfShape[1]));
	GuContactSphereHeightFieldShared(u, shape1, ts0, transform1, params, cache, contactBuffer, renderOutput, true);
	GuContactSphereHeightFieldShared(u, shape1, ts1, transform1, params, cache, contactBuffer, renderOutput, true);

	Segment worldCapsule;
	worldCapsule.p0 = -getCapsuleHalfHeightVector(transform0, shapeCapsule);
	worldCapsule.p1 = -worldCapsule.p0;
	worldCapsule.p0 += transform0.p;
	worldCapsule.p1 += transform0.p;

	const Segment capsuleSegmentInHfShape(verticesInHfShape[0], verticesInHfShape[1]);

	const PxU32 numCapsuleVertexContacts = contactBuffer.count; // remember how many contacts were stored as capsule vertex vs hf

	// test capsule edges vs HF
	PxVec3 v0h = hfUtil.shape2hfp(verticesInHfShape[0]), v1h = hfUtil.shape2hfp(verticesInHfShape[1]);
	const PxU32 absMinRow = hf.getMinRow(PxMin(v0h.x - radiusOverRowScale, v1h.x - radiusOverRowScale));
	const PxU32 absMaxRow = hf.getMaxRow(PxMax(v0h.x + radiusOverRowScale, v1h.x + radiusOverRowScale));
	const PxU32 absMinCol = hf.getMinColumn(PxMin(v0h.z - radiusOverColumnScale, v1h.z - radiusOverColumnScale));
	const PxU32 absMaxCol = hf.getMaxColumn(PxMax(v0h.z + radiusOverColumnScale, v1h.z + radiusOverColumnScale));
	if (DO_EDGE_EDGE)
	for(PxU32 row = absMinRow; row <= absMaxRow; row++)
	{
		for(PxU32 column = absMinCol; column <= absMaxCol; column++)
		{
			//PxU32 vertexIndex = row * hfShape.getNbColumnsFast() + column;
			const PxU32 vertexIndex = row * hf.getNbColumnsFast() + column;
			const PxU32 firstEdge = 3 * vertexIndex;

			// omg I am sorry about this code but I can't find a simpler way:
			//  last column will only test edge 2
			//  last row will only test edge 0
			//  and most importantly last row and column will not go inside the for
			const PxU32 minEi = PxU32((column == absMaxCol) ? 2 : 0); 
			const PxU32 maxEi = PxU32((row    == absMaxRow) ? 1 : 3); 
			// perform capsule edge vs HF edge collision 
			for (PxU32 ei = minEi; ei < maxEi; ei++)
			{
				const PxU32 edgeIndex = firstEdge + ei;

				PX_ASSERT(vertexIndex == edgeIndex / 3);
				PX_ASSERT(row == vertexIndex / hf.getNbColumnsFast());
				PX_ASSERT(column == vertexIndex % hf.getNbColumnsFast());

				// look up the face indices adjacent to the current edge
				PxU32 adjFaceIndices[2];
				const PxU32 adjFaceCount = hf.getEdgeTriangleIndices(edgeIndex, adjFaceIndices);
				bool doCollision = false;
				if(adjFaceCount == 2)
				{
					doCollision =		hf.getMaterialIndex0(adjFaceIndices[0] >> 1) != PxHeightFieldMaterial::eHOLE
									||	hf.getMaterialIndex1(adjFaceIndices[1] >> 1) != PxHeightFieldMaterial::eHOLE;
				}
				else if(adjFaceCount == 1)
				{
					doCollision = (hf.getMaterialIndex0(adjFaceIndices[0] >> 1) != PxHeightFieldMaterial::eHOLE);
				}

				if(doCollision)
				{
					PxVec3 origin;
					PxVec3 direction;
					hfUtil.getEdge(edgeIndex, vertexIndex, row, column, origin, direction);

					PxReal s, t;
					const PxReal ll = distanceSegmentSegmentSquared(
						capsuleSegmentInHfShape.p0, capsuleSegmentInHfShape.computeDirection(), origin, direction, &s, &t);
					if ((ll < radiusSquared) && (t >= 0) && (t <= 1))
					{

						// We only want to test the vertices for either rows or columns.
						// In this case we have chosen rows (ei == 0).
						if (ei != 0 && (t == 0 || t == 1)) 
							continue;

						const PxVec3 pointOnCapsuleInHfShape = capsuleSegmentInHfShape.getPointAt(s);
						const PxVec3 pointOnEdge = origin + t * direction;
						const PxVec3 d = pointOnCapsuleInHfShape - pointOnEdge;
						//if (hfShape.isDeltaHeightOppositeExtent(d.y))
						if (hf.isDeltaHeightOppositeExtent(d.y))
						{

							// Check if the current edge's normal is within any of it's 2 adjacent faces' Voronoi regions
							// If it is, force the normal to that region's face normal
							PxReal l;
							PxVec3 n = hfUtil.computePointNormal(hfGeom.heightFieldFlags, d, transform1, ll, pointOnEdge.x, pointOnEdge.z, epsSqr, l);
							PxVec3 localN = transform1.rotateInv(n);
							for (PxU32 j = 0; j < adjFaceCount; j++)
							{
								const PxVec3 adjNormal = hfUtil.hf2shapen(hf.getTriangleNormalInternal(adjFaceIndices[j])).getNormalized();
								PxU32 triCell = adjFaceIndices[j] >> 1;
								PxU32 triRow = triCell/hf.getNbColumnsFast();
								PxU32 triCol = triCell%hf.getNbColumnsFast();
								PxVec3 tv0, tv1, tv2, tvc;
								hf.getTriangleVertices(adjFaceIndices[j], triRow, triCol, tv0, tv1, tv2);
								tvc = hfUtil.hf2shapep((tv0+tv1+tv2)/3.0f); // compute adjacent triangle center
								PxVec3 perp = adjNormal.cross(direction).getNormalized(); // adj face normal cross edge dir
								if (perp.dot(tvc-origin) < 0.0f) // make sure perp is pointing toward the center of the triangle
									perp = -perp;
								// perp is now a vector sticking out of the edge of the triangle (also the test edge) pointing toward the center
								// perpendicular to the normal (in triangle plane)
								if (perp.dot(localN) > 0.0f) // if the normal is in perp halfspace, clamp it to Voronoi region
								{
									n = transform1.rotate(adjNormal);
									break;
								}
							}

							const PxVec3 worldPoint = worldCapsule.getPointAt(s);
							const PxVec3 p = worldPoint - n * radius;
							PxU32 adjTri = adjFaceIndices[0];
							if(adjFaceCount == 2)
							{
								const PxU16 m0 = hf.getMaterialIndex0(adjFaceIndices[0] >> 1);
								if(m0 == PxHeightFieldMaterial::eHOLE)
									adjTri = adjFaceIndices[1];
							}
							contactBuffer.contact(p, n, l-radius, adjTri);
							#if DEBUG_HFNORMAL
								printf("n=%.5f %.5f %.5f; d=%.5f\n", n.x, n.y, n.z, l-radius);
								#if DEBUG_RENDER_HFCONTACTS
								PxScene *s; PxGetPhysics().getScenes(&s, 1, 0);
								Cm::RenderOutput((Cm::RenderBuffer&)s->getRenderBuffer()) << Cm::RenderOutput::LINES << PxDebugColor::eARGB_BLUE // red
									<< p << (p + n * 10.0f);
								#endif
							#endif
						}
					}
				}
			}

			// also perform capsule edge vs HF vertex collision 
			if (hfUtil.isCollisionVertex(vertexIndex, row, column))
			{
				PxVec3 vertex(row * hfGeom.rowScale, hfGeom.heightScale * hf.getHeight(vertexIndex), column * hfGeom.columnScale);
				PxReal s;
				const PxReal ll = distancePointSegmentSquared(capsuleSegmentInHfShape, vertex, &s);
				if (ll < radiusSquared)
				{
					const PxVec3 pointOnCapsuleInHfShape = capsuleSegmentInHfShape.getPointAt(s);
					const PxVec3 d = pointOnCapsuleInHfShape - vertex;
					//if (hfShape.isDeltaHeightOppositeExtent(d.y))
					if (hf.isDeltaHeightOppositeExtent(d.y))
					{
						// we look through all prior capsule vertex vs HF face contacts and see
						// if any of those share a face with hf_edge for the currently considered capsule_edge/hf_vertex contact
						bool normalFromFace = false;
						PxVec3 n;
						PxReal l = 1.0f;
						for (PxU32 iVertexContact = 0; iVertexContact < numCapsuleVertexContacts; iVertexContact++)
						{
							const ContactPoint& cp = contactBuffer.contacts[iVertexContact];
							PxU32 vi0, vi1, vi2;
							hf.getTriangleVertexIndices(cp.internalFaceIndex1, vi0, vi1, vi2);

							const PxU32 vi = vertexIndex;
							if ((cp.forInternalUse == 0) // if this is a face contact
								&& (vi == vi0 || vi == vi1 || vi == vi2)) // with one of the face's vertices matching this one
							{
								n = cp.normal; // then copy the normal from this contact
								l = PxAbs(d.dot(n));
								normalFromFace = true;
								break;
							}
						}
						
						if (!normalFromFace)
							n = hfUtil.computePointNormal(hfGeom.heightFieldFlags, d, transform1, ll, vertex.x, vertex.z, epsSqr, l);

						const PxVec3 worldPoint = worldCapsule.getPointAt(s);

						const PxU32 faceIndex = hfUtil.getVertexFaceIndex(vertexIndex, row, column);

						const PxVec3 p = worldPoint - n * radius;
						contactBuffer.contact(p,  n, l-radius, faceIndex);
							#if DEBUG_HFNORMAL
								printf("n=%.5f %.5f %.5f; d=%.5f\n", n.x, n.y, n.z, l-radius);
								#if DEBUG_RENDER_HFCONTACTS
								PxScene *s; PxGetPhysics().getScenes(&s, 1, 0);
								Cm::RenderOutput((Cm::RenderBuffer&)s->getRenderBuffer()) << Cm::RenderOutput::LINES << PxDebugColor::eARGB_BLUE // red
									<< p << (p + n * 10.0f);
								#endif
							#endif
					}
				} // if ll < radiusSquared
			} // if isCollisionVertex
		} // forEach HF column intersecting with capsule edge AABB
	} // forEach HF row intersecting with capsule edge AABB

	return contactBuffer.count>0;
}
}//Gu
}//physx
