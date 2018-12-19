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

#include "GuGeometryUnion.h"
#include "GuHeightFieldData.h"
#include "GuHeightFieldUtil.h"
#include "GuContactMethodImpl.h"
#include "GuContactBuffer.h"

#define DEBUG_RENDER_HFCONTACTS 0
#if DEBUG_RENDER_HFCONTACTS
#include "PxPhysics.h"
#include "PxScene.h"
#endif

using namespace physx;
using namespace Gu;

// Sphere-heightfield contact generation

// this code is shared between capsule vertices and sphere
bool GuContactSphereHeightFieldShared(GU_CONTACT_METHOD_ARGS, bool isCapsule)
{
#if 1
	PX_UNUSED(cache);
	PX_UNUSED(renderOutput);

	// Get actual shape data
	const PxSphereGeometry& shapeSphere = shape0.get<const PxSphereGeometry>();
	const PxHeightFieldGeometryLL& hfGeom = shape1.get<const PxHeightFieldGeometryLL>();

	const Gu::HeightFieldUtil hfUtil(hfGeom);
	const Gu::HeightField& hf = hfUtil.getHeightField();

	const PxReal radius = shapeSphere.radius;
	const PxReal eps = PxReal(0.1) * radius;

	const PxVec3 sphereInHfShape = transform1.transformInv(transform0.p);

	PX_ASSERT(isCapsule || contactBuffer.count==0);

	const PxReal oneOverRowScale = hfUtil.getOneOverRowScale();
	const PxReal oneOverColumnScale = hfUtil.getOneOverColumnScale();

	// check if the sphere is below the HF surface
	if (hfUtil.isShapePointOnHeightField(sphereInHfShape.x, sphereInHfShape.z))
	{

		PxReal fracX, fracZ;
		const PxU32 vertexIndex = hfUtil.getHeightField().computeCellCoordinates(sphereInHfShape.x * oneOverRowScale, sphereInHfShape.z * oneOverColumnScale, fracX, fracZ);

		// The sphere origin projects within the bounds of the heightfield in the X-Z plane
//		const PxReal sampleHeight = hfShape.getHeightAtShapePoint(sphereInHfShape.x, sphereInHfShape.z);
		const PxReal sampleHeight = hfUtil.getHeightAtShapePoint2(vertexIndex, fracX, fracZ);
		
		const PxReal deltaHeight = sphereInHfShape.y - sampleHeight;
		//if (hfShape.isDeltaHeightInsideExtent(deltaHeight, eps))
		if (hf.isDeltaHeightInsideExtent(deltaHeight, eps))
		{
			// The sphere origin is 'below' the heightfield surface
			// Actually there is an epsilon involved to make sure the
			// 'above' surface calculations can deliver a good normal
			const PxU32 faceIndex = hfUtil.getFaceIndexAtShapePointNoTest2(vertexIndex, fracX, fracZ);
			if (faceIndex != 0xffffffff)
			{

				//hfShape.getAbsPoseFast().M.getColumn(1, hfShapeUp);
				const PxVec3 hfShapeUp = transform1.q.getBasisVector1();

				if (hf.getThicknessFast() <= 0)
					contactBuffer.contact(transform0.p,  hfShapeUp,  deltaHeight-radius, faceIndex);
				else
					contactBuffer.contact(transform0.p, -hfShapeUp, -deltaHeight-radius, faceIndex);

				return true;
			}

			return false;
		}

	}

	const PxReal epsSqr = eps * eps;

	const PxReal inflatedRadius = radius + params.mContactDistance;
	const PxReal inflatedRadiusSquared = inflatedRadius * inflatedRadius;

	const PxVec3 sphereInHF = hfUtil.shape2hfp(sphereInHfShape);

	const PxReal radiusOverRowScale = inflatedRadius * PxAbs(oneOverRowScale);
	const PxReal radiusOverColumnScale = inflatedRadius * PxAbs(oneOverColumnScale);

	const PxU32 minRow = hf.getMinRow(sphereInHF.x - radiusOverRowScale);
	const PxU32 maxRow = hf.getMaxRow(sphereInHF.x + radiusOverRowScale);
	const PxU32 minColumn = hf.getMinColumn(sphereInHF.z - radiusOverColumnScale);
	const PxU32 maxColumn = hf.getMaxColumn(sphereInHF.z + radiusOverColumnScale);

	// this assert is here because the following code depends on it for reasonable performance for high-count situations
	PX_COMPILE_TIME_ASSERT(ContactBuffer::MAX_CONTACTS == 64);

	const PxU32 nbColumns = hf.getNbColumnsFast();

#define HFU Gu::HeightFieldUtil
	PxU32 numFaceContacts = 0;
	for (PxU32 i = 0; i<2; i++)
	{
		const bool facesOnly = (i == 0);
		// first we go over faces-only meaning only contacts directly in Voronoi regions of faces
		// at second pass we consider edges and vertices and clamp the normals to adjacent feature's normal
		// if there was a prior contact. it is equivalent to clipping the normal to it's feature's Voronoi region

		for (PxU32 r = minRow; r < maxRow; r++) 
		{
			for (PxU32 c = minColumn; c < maxColumn; c++) 
			{

				// x--x--x
				// | x   |
				// x  x  x
				// |   x |
				// x--x--x
				PxVec3 closestPoints[11];
				PxU32 closestFeatures[11];
				PxU32 npcp = hfUtil.findClosestPointsOnCell(
					r, c, sphereInHfShape, closestPoints, closestFeatures, facesOnly, !facesOnly, true);

				for(PxU32 pi = 0; pi < npcp; pi++)
				{
					PX_ASSERT(closestFeatures[pi] != 0xffffffff);
					const PxVec3 d = sphereInHfShape - closestPoints[pi];

					if (hf.isDeltaHeightOppositeExtent(d.y)) // See if we are 'above' the heightfield
					{
						const PxReal dMagSq = d.magnitudeSquared();

						if (dMagSq > inflatedRadiusSquared) 
							// Too far above
							continue;

						PxReal dMag = -1.0f; // dMag is sqrt(sMadSq) and comes up as a byproduct of other calculations in computePointNormal
						PxVec3 n; // n is in world space, rotated by transform1
						PxU32 featureType = HFU::getFeatureType(closestFeatures[pi]);
						if (featureType == HFU::eEDGE)
						{
							PxU32 edgeIndex = HFU::getFeatureIndex(closestFeatures[pi]);
							PxU32 adjFaceIndices[2];
							const PxU32 adjFaceCount = hf.getEdgeTriangleIndices(edgeIndex, adjFaceIndices);
							PxVec3 origin;
							PxVec3 direction;
							const PxU32 vertexIndex = edgeIndex / 3;
							const PxU32 row			= vertexIndex / nbColumns;
							const PxU32 col			= vertexIndex % nbColumns;
							hfUtil.getEdge(edgeIndex, vertexIndex, row, col, origin, direction);
							n = hfUtil.computePointNormal(
									hfGeom.heightFieldFlags, d, transform1, dMagSq,
									closestPoints[pi].x, closestPoints[pi].z, epsSqr, dMag);
							PxVec3 localN = transform1.rotateInv(n);
							// clamp the edge's normal to its Voronoi region
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
						} else if(featureType == HFU::eVERTEX)
						{
							// AP: these contacts are rare so hopefully it's ok
							const PxU32 bufferCount = contactBuffer.count;
							const PxU32 vertIndex = HFU::getFeatureIndex(closestFeatures[pi]);
							EdgeData adjEdges[8];
							const PxU32 row = vertIndex / nbColumns;
							const PxU32 col = vertIndex % nbColumns;
							const PxU32 numAdjEdges = ::getVertexEdgeIndices(hf, vertIndex, row, col, adjEdges);
							for (PxU32 iPrevEdgeContact = numFaceContacts; iPrevEdgeContact < bufferCount; iPrevEdgeContact++)
							{
								if (contactBuffer.contacts[iPrevEdgeContact].forInternalUse != HFU::eEDGE)
									continue; // skip non-edge contacts (can be other vertex contacts)

								for (PxU32 iAdjEdge = 0; iAdjEdge < numAdjEdges; iAdjEdge++)
									// does adjacent edge index for this vertex match a previously encountered edge index?
									if (adjEdges[iAdjEdge].edgeIndex == contactBuffer.contacts[iPrevEdgeContact].internalFaceIndex1)
									{
										// if so, clamp the normal for this vertex to that edge's normal
										n = contactBuffer.contacts[iPrevEdgeContact].normal;
										dMag = PxSqrt(dMagSq);
										break;
									}
							}
						}

						if (dMag == -1.0f)
							n = hfUtil.computePointNormal(hfGeom.heightFieldFlags, d, transform1,
								dMagSq, closestPoints[pi].x, closestPoints[pi].z, epsSqr, dMag);

						PxVec3 p = transform0.p - n * radius;
							#if DEBUG_RENDER_HFCONTACTS
							printf("n=%.5f %.5f %.5f;  ", n.x, n.y, n.z);
							if (n.y < 0.8f)
								int a = 1;
							PxScene *s; PxGetPhysics().getScenes(&s, 1, 0);
							Cm::RenderOutput((Cm::RenderBuffer&)s->getRenderBuffer()) << Cm::RenderOutput::LINES << PxDebugColor::eARGB_BLUE // red
								<< p << (p + n * 10.0f);
							#endif

						// temporarily use the internalFaceIndex0 slot in the contact buffer for featureType
	 					contactBuffer.contact(
							p, n, dMag - radius, PxU16(featureType), HFU::getFeatureIndex(closestFeatures[pi]));	
					}
				}
			}
		}
		if (facesOnly)
			numFaceContacts = contactBuffer.count;
	} // for facesOnly = true to false

	if (!isCapsule) // keep the face labels as internalFaceIndex0 if this function is called from inside capsule/HF function
		for (PxU32 k = 0; k < numFaceContacts; k++)
			contactBuffer.contacts[k].forInternalUse = 0xFFFF;

	for (PxU32 k = numFaceContacts; k < contactBuffer.count; k++)
	{
		PX_ASSERT(contactBuffer.contacts[k].forInternalUse != HFU::eFACE);
		PX_ASSERT(contactBuffer.contacts[k].forInternalUse <= HFU::eVERTEX);
		PxU32 featureIndex = contactBuffer.contacts[k].internalFaceIndex1;
		if (contactBuffer.contacts[k].forInternalUse == HFU::eEDGE)
			contactBuffer.contacts[k].internalFaceIndex1 = hfUtil.getEdgeFaceIndex(featureIndex);
		else if (contactBuffer.contacts[k].forInternalUse == HFU::eVERTEX)
		{
			PxU32 row = featureIndex/hf.getNbColumnsFast();
			PxU32 col = featureIndex%hf.getNbColumnsFast();
			contactBuffer.contacts[k].internalFaceIndex1 = hfUtil.getVertexFaceIndex(featureIndex, row, col);
		}
	}
#undef HFU

	return contactBuffer.count>0;
#endif
}

namespace physx
{
namespace Gu
{
bool legacyContactSphereHeightfield(GU_CONTACT_METHOD_ARGS)
{
	return GuContactSphereHeightFieldShared(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput, false);
}

}//Gu
}//physx
