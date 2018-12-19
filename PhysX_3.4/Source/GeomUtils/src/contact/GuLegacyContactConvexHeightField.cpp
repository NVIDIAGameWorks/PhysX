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

        
#include "GuLegacyTraceLineCallback.h"
#include "GuConvexMeshData.h"
#include "GuEdgeCache.h"
#include "GuConvexHelper.h"
#include "GuContactMethodImpl.h"
#include "GuContactBuffer.h"

using namespace physx;
using namespace Gu;
using namespace shdfnd::aos;

#define DISTANCE_BASED_TEST

// ptchernev TODO: make sure these are ok before shipping
static const bool gCompileConvexVertex		= true;
static const bool gCompileEdgeEdge          = true;
static const bool gCompileHeightFieldVertex = true;

/** 
\param point vertex tested for penetration (in local hull space)
\param safeNormal if none of the faces are good this becomes the normal
\param normal the direction to translate vertex to depenetrate
\param distance the distance along normal to translate vertex to depenetrate
*/
static bool GuDepenetrateConvex(	const PxVec3& point,
									const PxVec3& safeNormal,
									const Gu::ConvexHullData& hull,
									float contactDistance,
									PxVec3& normal,
									PxReal& distance,
									const Cm::FastVertex2ShapeScaling& scaling,
									bool isConvexScaleIdentity)
{
	PxVec3 faceNormal(PxReal(0));
	PxReal distance1 = -PX_MAX_REAL; // cant be more
	PxReal distance2 = -PX_MAX_REAL; // cant be more
	PxI32 poly1 = -1;
	PxI32 poly2 = -2;

//	const Cm::FastVertex2ShapeScaling& scaling = context.mVertex2ShapeSkew[0];

	for (PxU32 poly = 0; poly < hull.mNbPolygons; poly++)
	{
		PX_ALIGN(16, PxPlane) shapeSpacePlane;
		if(isConvexScaleIdentity)
		{
			V4StoreA(V4LoadU(&hull.mPolygons[poly].mPlane.n.x), &shapeSpacePlane.n.x);
		}
		else
		{
			const PxPlane& vertSpacePlane = hull.mPolygons[poly].mPlane;
			scaling.transformPlaneToShapeSpace(vertSpacePlane.n, vertSpacePlane.d, shapeSpacePlane.n, shapeSpacePlane.d);//transform plane into shape space
		}

#ifdef DISTANCE_BASED_TEST
		// PT: I'm not really sure about contactDistance here
		const PxReal d = shapeSpacePlane.distance(point) - contactDistance;
#else
		const PxReal d = shapeSpacePlane.distance(point);
#endif

		if (d >= 0)
		{
			// no penetration at all
			return false;
		}

		//const PxVec3& n = plane.normal;
		const PxReal proj = shapeSpacePlane.n.dot(safeNormal);
		if (proj > 0)
		{
			if (d > distance1) // less penetration
			{
				distance1 = d;
				faceNormal = shapeSpacePlane.n;
				poly1 = PxI32(poly);
			}

			// distance2 / d = 1 / proj 
			const PxReal tmp = d / proj;
			if (tmp > distance2)
			{
				distance2 = tmp;
				poly2 = PxI32(poly);
			}
		}
	}

	if (poly1 == poly2)
	{
		PX_ASSERT(faceNormal.magnitudeSquared() != 0.0f);
		normal = faceNormal;
		distance = -distance1;
	}
	else
	{
		normal = safeNormal;
		distance = -distance2;
	}

	return true;
}

//Box-Heightfield and Convex-Heightfield do not support positive values for contactDistance,
//and if in this case we would emit contacts normally, we'd cause things to jitter.
//as a workaround we add contactDistance to the distance values that we emit in contacts.
//this has the effect that the biasing will work exactly as if we had specified a legacy skinWidth of (contactDistance - restDistance)

#include "GuContactMethodImpl.h"

namespace physx
{
namespace Gu
{
bool legacyContactConvexHeightfield(GU_CONTACT_METHOD_ARGS)
{
	PX_UNUSED(cache);
	PX_UNUSED(renderOutput);

#ifndef DISTANCE_BASED_TEST
	PX_WARN_ONCE(contactDistance > 0.0f, "PxcContactConvexHeightField: Convex-Heightfield does not support distance based contact generation! Ignoring contactOffset > 0!");
#endif

	// Get actual shape data
	const PxConvexMeshGeometryLL& shapeConvex = shape0.get<const PxConvexMeshGeometryLL>();
	const PxHeightFieldGeometryLL& hfGeom = shape1.get<const PxHeightFieldGeometryLL>();

	Cm::Matrix34 convexShapeAbsPose(transform0);
	Cm::Matrix34 hfShapeAbsPose(transform1);

	PX_ASSERT(contactBuffer.count==0);

	const Gu::HeightFieldTraceUtil hfUtil(hfGeom);
	const Gu::HeightField& hf = hfUtil.getHeightField();

	const bool isConvexScaleIdentity = shapeConvex.scale.isIdentity();
	Cm::FastVertex2ShapeScaling convexScaling;	// PT: TODO: remove default ctor
	if(!isConvexScaleIdentity)
		convexScaling.init(shapeConvex.scale);

	const PxMat33& left = hfShapeAbsPose.m;
	const PxMat33& right = convexShapeAbsPose.m;

	Cm::Matrix34 convexShape2HfShape(left.getInverse()* right, left.getInverse()*(convexShapeAbsPose.p - hfShapeAbsPose.p));
	Cm::Matrix34 convexVertex2World( right * convexScaling.getVertex2ShapeSkew(),convexShapeAbsPose.p );

	// Allocate space for transformed vertices.
	const Gu::ConvexHullData* PX_RESTRICT hull = shapeConvex.hullData;
	PxVec3* PX_RESTRICT convexVerticesInHfShape = reinterpret_cast<PxVec3*>(PxAlloca(hull->mNbHullVertices*sizeof(PxVec3)));

	// Transform vertices to height field shape
	PxMat33 convexShape2HfShape_rot(convexShape2HfShape[0],convexShape2HfShape[1],convexShape2HfShape[2]);
	Cm::Matrix34 convexVertices2HfShape(convexShape2HfShape_rot*convexScaling.getVertex2ShapeSkew(), convexShape2HfShape[3]);
	const PxVec3* const PX_RESTRICT hullVerts = hull->getHullVertices();
	for(PxU32 i = 0; i<hull->mNbHullVertices; i++)
		convexVerticesInHfShape[i] = convexVertices2HfShape.transform(hullVerts[i]);

	PxVec3 convexBoundsInHfShapeMin( PX_MAX_REAL,  PX_MAX_REAL,  PX_MAX_REAL);
	PxVec3 convexBoundsInHfShapeMax(-PX_MAX_REAL, -PX_MAX_REAL, -PX_MAX_REAL);

	// Compute bounds of convex in hf space
	for(PxU32 i = 0; i<hull->mNbHullVertices; i++)
	{
		const PxVec3& v = convexVerticesInHfShape[i];

		convexBoundsInHfShapeMin.x = PxMin(convexBoundsInHfShapeMin.x, v.x);
		convexBoundsInHfShapeMin.y = PxMin(convexBoundsInHfShapeMin.y, v.y);
		convexBoundsInHfShapeMin.z = PxMin(convexBoundsInHfShapeMin.z, v.z);

		convexBoundsInHfShapeMax.x = PxMax(convexBoundsInHfShapeMax.x, v.x);
		convexBoundsInHfShapeMax.y = PxMax(convexBoundsInHfShapeMax.y, v.y);
		convexBoundsInHfShapeMax.z = PxMax(convexBoundsInHfShapeMax.z, v.z);
	}

	const bool thicknessNegOrNull = (hf.getThicknessFast() <= 0.0f);	// PT: don't do this each time! FCMPs are slow.

	// Compute the height field extreme over the bounds area.
	const PxReal oneOverRowScale = hfUtil.getOneOverRowScale();
	const PxReal oneOverColumnScale = hfUtil.getOneOverColumnScale();

	PxU32 minRow;
	PxU32 maxRow;
	PxU32 minColumn;
	PxU32 maxColumn;

	minRow = hf.getMinRow(convexBoundsInHfShapeMin.x * oneOverRowScale);
	maxRow = hf.getMaxRow(convexBoundsInHfShapeMax.x * oneOverRowScale);

	minColumn = hf.getMinColumn(convexBoundsInHfShapeMin.z * oneOverColumnScale);
	maxColumn = hf.getMaxColumn(convexBoundsInHfShapeMax.z * oneOverColumnScale);

	PxReal hfExtreme = hf.computeExtreme(minRow, maxRow, minColumn, maxColumn);

	hfExtreme *= hfGeom.heightScale;


	// Return if convex is on the wrong side of the extreme.
	if (thicknessNegOrNull)
	{
		if (convexBoundsInHfShapeMin.y > hfExtreme) return false;
	}
	else
	{
		if (convexBoundsInHfShapeMax.y < hfExtreme) return false;
	}

	// Test convex vertices
	if (gCompileConvexVertex)
	{
		for(PxU32 i=0; i<hull->mNbHullVertices; i++)
		{
			const PxVec3& convexVertexInHfShape = convexVerticesInHfShape[i];

			//////// SAME CODE AS IN BOX-HF
			const bool insideExtreme = thicknessNegOrNull ? (convexVertexInHfShape.y < hfExtreme) : (convexVertexInHfShape.y > hfExtreme);

			if (insideExtreme && hfUtil.isShapePointOnHeightField(convexVertexInHfShape.x, convexVertexInHfShape.z))
			{
				// PT: compute this once, reuse results (3 times!)
				// PT: TODO: also reuse this in EE tests
				PxReal fracX, fracZ;
				const PxU32 vertexIndex = hfUtil.getHeightField().computeCellCoordinates(convexVertexInHfShape.x * oneOverRowScale, convexVertexInHfShape.z * oneOverColumnScale, fracX, fracZ);

				const PxReal y = hfUtil.getHeightAtShapePoint2(vertexIndex, fracX, fracZ);

				const PxReal dy = convexVertexInHfShape.y - y;
			#ifdef DISTANCE_BASED_TEST
				if (hf.isDeltaHeightInsideExtent(dy, params.mContactDistance/**2.0f*/))
			#else
				if (hf.isDeltaHeightInsideExtent(dy))
			#endif
				{
					const PxU32 faceIndex = hfUtil.getFaceIndexAtShapePointNoTest2(vertexIndex, fracX, fracZ);
					if (faceIndex != 0xffffffff)
					{
						PxVec3 n;
						n = hfUtil.getNormalAtShapePoint2(vertexIndex, fracX, fracZ);
						n = n.getNormalized();

						contactBuffer
						#ifdef DISTANCE_BASED_TEST
							.contact(convexVertex2World.transform(hullVerts[i]), hfShapeAbsPose.rotate(n), n.y*dy/* - contactDistance*/, faceIndex);
						#else
							.contact(convexVertex2World.transform(hullVerts[i]), hfShapeAbsPose.rotate(n), n.y*dy + contactDistance, faceIndex);//add contactDistance to compensate for fact that we don't support dist based contacts! See comment at start of funct.
						#endif
					}
				}
			}
			////////~SAME CODE AS IN BOX-HF
		} 
	}

	// Test convex edges
	if (gCompileEdgeEdge)
	{
		// create helper class for the trace segment
		GuContactHeightfieldTraceSegmentHelper traceSegmentHelper(hfUtil);

		if(1)
		{
			PxU32 numPolygons = hull->mNbPolygons;
			const Gu::HullPolygonData* polygons = hull->mPolygons;
			const PxU8* vertexData = hull->getVertexData8();

			ConvexEdge edges[512];
			PxU32 nbEdges = findUniqueConvexEdges(512, edges, numPolygons, polygons, vertexData);

			for(PxU32 i=0;i<nbEdges;i++)
			{
				const PxVec3 convexNormalInHfShape = convexVertices2HfShape.rotate(edges[i].normal);
				if(convexNormalInHfShape.y>0.0f)
					continue;

				const PxU8 vi0 = edges[i].vref0;
				const PxU8 vi1 = edges[i].vref1;

				const PxVec3& sv0 = convexVerticesInHfShape[vi0];
				const PxVec3& sv1 = convexVerticesInHfShape[vi1];

				if (thicknessNegOrNull) 
				{
					if ((sv0.y > hfExtreme) && (sv1.y > hfExtreme)) continue;
				}
				else
				{
					if ((sv0.y < hfExtreme) && (sv1.y < hfExtreme)) continue;
				}
				GuContactTraceSegmentCallback cb(sv1 - sv0, contactBuffer, hfShapeAbsPose, params.mContactDistance);
				traceSegmentHelper.traceSegment(sv0, sv1, &cb);
			}
		}
		else
		{
			Gu::EdgeCache edgeCache;
			PxU32 numPolygons = hull->mNbPolygons;
			const Gu::HullPolygonData* polygons = hull->mPolygons;
			const PxU8* vertexData = hull->getVertexData8();
			while (numPolygons--)
			{
				const Gu::HullPolygonData& polygon = *polygons++;
				const PxU8* vRefBase = vertexData + polygon.mVRef8;

				PxU32 numEdges = polygon.mNbVerts;

				PxU32 a = numEdges - 1;
				PxU32 b = 0;
				while(numEdges--)
				{
					PxU8 vi0 =  vRefBase[a];
					PxU8 vi1 =	vRefBase[b];


					if(vi1 < vi0)
					{
						PxU8 tmp = vi0;
						vi0 = vi1;
						vi1 = tmp;
					}

					a = b;
					b++;

					// avoid processing edges 2x if possible (this will typically have cache misses about 5% of the time
					// leading to 5% redundant work).
					if (edgeCache.isInCache(vi0, vi1))
						continue;

					const PxVec3& sv0 = convexVerticesInHfShape[vi0];
					const PxVec3& sv1 = convexVerticesInHfShape[vi1];

					if (thicknessNegOrNull) 
					{
						if ((sv0.y > hfExtreme) && (sv1.y > hfExtreme))
							continue;
					}
					else
					{
						if ((sv0.y < hfExtreme) && (sv1.y < hfExtreme))
							continue;
					}
					GuContactTraceSegmentCallback cb(sv1 - sv0, contactBuffer, hfShapeAbsPose, params.mContactDistance);
					traceSegmentHelper.traceSegment(sv0, sv1, &cb);
				}
			}
		}
	}

	// Test height field vertices
	if (gCompileHeightFieldVertex)
	{
		// Iterate over HeightField vertices inside the projected box bounds.
		for(PxU32 row = minRow; row <= maxRow; row++)
		{
			for(PxU32 column = minColumn; column <= maxColumn; column++)
			{
				const PxU32 vertexIndex = row * hf.getNbColumnsFast() + column;

				if (!hfUtil.isCollisionVertex(vertexIndex, row, column))
					continue;

				const PxVec3 hfVertex(hfGeom.rowScale * row, hfGeom.heightScale * hf.getHeight(vertexIndex), hfGeom.columnScale * column);

				const PxVec3 nrm = hfUtil.getVertexNormal(vertexIndex, row, column);
				PxVec3 hfVertexNormal = thicknessNegOrNull ? nrm : -nrm;
				hfVertexNormal = hfVertexNormal.getNormalized();
				const PxVec3 hfVertexNormalInConvexShape = convexShape2HfShape.rotateTranspose(hfVertexNormal);
				PxVec3 hfVertexInConvexShape = convexShape2HfShape.transformTranspose(hfVertex);
				PxReal depth;
				PxVec3 normal;
				if (!GuDepenetrateConvex(hfVertexInConvexShape, -hfVertexNormalInConvexShape, *hull, params.mContactDistance, normal, depth, 
					convexScaling,
					isConvexScaleIdentity))
				{
					continue;
				}
				PxVec3 normalInHfShape = convexShape2HfShape.rotate(-normal);
				hfUtil.clipShapeNormalToVertexVoronoi(normalInHfShape, vertexIndex, row, column);
				if (normalInHfShape.dot(hfVertexNormal) < PX_EPS_REAL) // AP scaffold: verify this is impossible
					continue;

				normalInHfShape = normalInHfShape.getNormalized();
				PxU32 faceIndex = hfUtil.getVertexFaceIndex(vertexIndex, row, column);
				contactBuffer
					.contact(hfShapeAbsPose.transform(hfVertex), hfShapeAbsPose.rotate(normalInHfShape),
					#ifdef DISTANCE_BASED_TEST
						-depth,
					#else
						//add contactDistance to compensate for fact that we don't support dist based contacts!
						// See comment at start of funct.
						-depth + contactDistance,
					#endif
						faceIndex);
			}
		}
	}

	return contactBuffer.count>0;
}
}//Gu
}//physx
