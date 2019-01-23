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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#include "RenderPhysX3Debug.h"
#include "RendererColor.h"
#include "common/PxRenderBuffer.h"
#include "foundation/PxSimpleTypes.h"
#include "SampleCamera.h"

#include "geometry/PxConvexMesh.h"
#include "geometry/PxConvexMeshGeometry.h"
#include "geometry/PxCapsuleGeometry.h"
#include "geometry/PxSphereGeometry.h"
#include "geometry/PxBoxGeometry.h"
#include "PsUtilities.h"
#include "PsString.h"

using namespace physx;
using namespace SampleRenderer;
using namespace SampleFramework;

RenderPhysX3Debug::RenderPhysX3Debug(Renderer& renderer, SampleAssetManager& assetmanager) :
	SamplePointDebugRender		(renderer, assetmanager),
	SampleLineDebugRender		(renderer, assetmanager),
	SampleTriangleDebugRender	(renderer, assetmanager)
{
}

RenderPhysX3Debug::~RenderPhysX3Debug()
{
}

void RenderPhysX3Debug::update(const PxRenderBuffer& debugRenderable)
{
	// Points
	const PxU32 numPoints = debugRenderable.getNbPoints();
	if(numPoints)
	{
		const PxDebugPoint* PX_RESTRICT points = debugRenderable.getPoints();
		checkResizePoint(numPoints);
		for(PxU32 i=0; i<numPoints; i++)
		{
			const PxDebugPoint& point = points[i];
			addPoint(point.pos, RendererColor(point.color));
		}
	}

	// Lines
	const PxU32 numLines = debugRenderable.getNbLines();
	if(numLines)
	{
		const PxDebugLine* PX_RESTRICT lines = debugRenderable.getLines();
		checkResizeLine(numLines * 2);
		for(PxU32 i=0; i<numLines; i++)
		{
			const PxDebugLine& line = lines[i];
			addLine(line.pos0, line.pos1, RendererColor(line.color0));
		}
	}

	// Triangles
	const PxU32 numTriangles = debugRenderable.getNbTriangles();
	if(numTriangles)
	{
		const PxDebugTriangle* PX_RESTRICT triangles = debugRenderable.getTriangles();
		checkResizeTriangle(numTriangles * 3);
		for(PxU32 i=0; i<numTriangles; i++)
		{
			const PxDebugTriangle& triangle = triangles[i];
			addTriangle(triangle.pos0, triangle.pos1, triangle.pos2, RendererColor(triangle.color0));
		}
	}
}

void RenderPhysX3Debug::update(const PxRenderBuffer& debugRenderable, const Camera& camera)
{
	// Points
	const PxU32 numPoints = debugRenderable.getNbPoints();
	if(numPoints)
	{
		const PxDebugPoint* PX_RESTRICT points = debugRenderable.getPoints();
		checkResizePoint(numPoints);
		for(PxU32 i=0; i<numPoints; i++)
		{
			const PxDebugPoint& point = points[i];
			addPoint(point.pos, RendererColor(point.color));
		}
	}

	// Lines
	const PxU32 numLines = debugRenderable.getNbLines();
	if(numLines)
	{
		const PxDebugLine* PX_RESTRICT lines = debugRenderable.getLines();
		checkResizeLine(numLines * 2);
		PxU32 nbVisible = 0;
		for(PxU32 i=0; i<numLines; i++)
		{
			const PxDebugLine& line = lines[i];

			PxBounds3 b;
			b.minimum.x = PxMin(line.pos0.x, line.pos1.x);
			b.minimum.y = PxMin(line.pos0.y, line.pos1.y);
			b.minimum.z = PxMin(line.pos0.z, line.pos1.z);
			b.maximum.x = PxMax(line.pos0.x, line.pos1.x);
			b.maximum.y = PxMax(line.pos0.y, line.pos1.y);
			b.maximum.z = PxMax(line.pos0.z, line.pos1.z);
			if(camera.cull(b)==PLANEAABB_EXCLUSION)
				continue;

			addLine(line.pos0, line.pos1, RendererColor(line.color0));
			nbVisible++;
		}
		shdfnd::printFormatted("%f\n", float(nbVisible)/float(numLines));
	}

	// Triangles
	const PxU32 numTriangles = debugRenderable.getNbTriangles();
	if(numTriangles)
	{
		const PxDebugTriangle* PX_RESTRICT triangles = debugRenderable.getTriangles();
		checkResizeTriangle(numTriangles * 3);
		for(PxU32 i=0; i<numTriangles; i++)
		{
			const PxDebugTriangle& triangle = triangles[i];
			addTriangle(triangle.pos0, triangle.pos1, triangle.pos2, RendererColor(triangle.color0));
		}
	}
}

void RenderPhysX3Debug::queueForRender()
{
	queueForRenderPoint();
	queueForRenderLine();
	queueForRenderTriangle();
}

void RenderPhysX3Debug::clear()
{
	clearPoint();
	clearLine();
	clearTriangle();
}

///////////////////////////////////////////////////////////////////////////////

#define NB_CIRCLE_PTS	20

void RenderPhysX3Debug::addBox(const PxVec3* pts, const RendererColor& color, PxU32 renderFlags)
{
	if(renderFlags & RENDER_DEBUG_WIREFRAME)
	{
		const PxU8 indices[] = {
		0, 1,	1, 2,	2, 3,	3, 0,
		7, 6,	6, 5,	5, 4,	4, 7,
		1, 5,	6, 2,
		3, 7,	4, 0
		};

		for(PxU32 i=0;i<12;i++)
			addLine(pts[indices[i*2]], pts[indices[i*2+1]], color);
	}

	if(renderFlags & RENDER_DEBUG_SOLID)
	{
		const PxU8 indices[] = {
		0,2,1,	0,3,2,
		1,6,5,	1,2,6,
		5,7,4,	5,6,7,
		4,3,0,	4,7,3,
		3,6,2,	3,7,6,
		5,0,1,	5,4,0
		};
		for(PxU32 i=0;i<12;i++)
			addTriangle(pts[indices[i*3+0]], pts[indices[i*3+1]], pts[indices[i*3+2]], color);
	}
}

void RenderPhysX3Debug::addCircle(PxU32 nbPts, const PxVec3* pts, const RendererColor& color, const PxVec3& offset)
{
	for(PxU32 i=0;i<nbPts;i++)
	{
		const PxU32 j = (i+1) % nbPts;
		addLine(pts[i]+offset, pts[j]+offset, color);
	}
}

void RenderPhysX3Debug::addAABB(const PxBounds3& box, const RendererColor& color, PxU32 renderFlags)
{
	const PxVec3& min = box.minimum;
	const PxVec3& max = box.maximum;

	//     7+------+6			0 = ---
	//     /|     /|			1 = +--
	//    / |    / |			2 = ++-
	//   / 4+---/--+5			3 = -+-
	// 3+------+2 /    y   z	4 = --+
	//  | /    | /     |  /		5 = +-+
	//  |/     |/      |/		6 = +++
	// 0+------+1      *---x	7 = -++

	// Generate 8 corners of the bbox
	PxVec3 pts[8];
	pts[0] = PxVec3(min.x, min.y, min.z);
	pts[1] = PxVec3(max.x, min.y, min.z);
	pts[2] = PxVec3(max.x, max.y, min.z);
	pts[3] = PxVec3(min.x, max.y, min.z);
	pts[4] = PxVec3(min.x, min.y, max.z);
	pts[5] = PxVec3(max.x, min.y, max.z);
	pts[6] = PxVec3(max.x, max.y, max.z);
	pts[7] = PxVec3(min.x, max.y, max.z);

	addBox(pts, color, renderFlags);
}

void RenderPhysX3Debug::addBox(const PxBoxGeometry& bg, const PxTransform& tr, const RendererColor& color, PxU32 renderFlags)
{
	addOBB(tr.p, bg.halfExtents, PxMat33(tr.q), color, renderFlags);
}

void RenderPhysX3Debug::addOBB(const PxVec3& boxCenter, const PxVec3& boxExtents, const PxMat33& boxRot, const RendererColor& color, PxU32 renderFlags)
{
	PxVec3 Axis0 = boxRot.column0;
	PxVec3 Axis1 = boxRot.column1;
	PxVec3 Axis2 = boxRot.column2;

	// "Rotated extents"
	Axis0 *= boxExtents.x;
	Axis1 *= boxExtents.y;
	Axis2 *= boxExtents.z;

	//     7+------+6			0 = ---
	//     /|     /|			1 = +--
	//    / |    / |			2 = ++-
	//   / 4+---/--+5			3 = -+-
	// 3+------+2 /    y   z	4 = --+
	//  | /    | /     |  /		5 = +-+
	//  |/     |/      |/		6 = +++
	// 0+------+1      *---x	7 = -++

	// Original code: 24 vector ops
/*	pts[0] = mCenter - Axis0 - Axis1 - Axis2;
	pts[1] = mCenter + Axis0 - Axis1 - Axis2;
	pts[2] = mCenter + Axis0 + Axis1 - Axis2;
	pts[3] = mCenter - Axis0 + Axis1 - Axis2;
	pts[4] = mCenter - Axis0 - Axis1 + Axis2;
	pts[5] = mCenter + Axis0 - Axis1 + Axis2;
	pts[6] = mCenter + Axis0 + Axis1 + Axis2;
	pts[7] = mCenter - Axis0 + Axis1 + Axis2;*/

	// Rewritten: 12 vector ops
	PxVec3 pts[8];
	pts[0] = pts[3] = pts[4] = pts[7] = boxCenter - Axis0;
	pts[1] = pts[2] = pts[5] = pts[6] = boxCenter + Axis0;

	PxVec3 Tmp = Axis1 + Axis2;
	pts[0] -= Tmp;
	pts[1] -= Tmp;
	pts[6] += Tmp;
	pts[7] += Tmp;

	Tmp = Axis1 - Axis2;
	pts[2] += Tmp;
	pts[3] += Tmp;
	pts[4] -= Tmp;
	pts[5] -= Tmp;

	addBox(pts, color, renderFlags);
}

	enum Orientation
	{
		ORIENTATION_XY,
		ORIENTATION_XZ,
		ORIENTATION_YZ,

		ORIENTATION_FORCE_DWORD = 0x7fffffff
	};

static bool generatePolygon(PxU32 nbVerts, PxVec3* verts, Orientation orientation, float amplitude, float phase, const PxTransform* transform=NULL)
{
	if(!nbVerts || !verts)
		return false;

	const float step = PxTwoPi/float(nbVerts);

	for(PxU32 i=0;i<nbVerts;i++)
	{
		const float angle = phase + float(i) * step;
		const float y = sinf(angle) * amplitude;
		const float x = cosf(angle) * amplitude;

				if(orientation==ORIENTATION_XY)	{ verts[i] = PxVec3(x,		y,		0.0f);	}
		else	if(orientation==ORIENTATION_XZ)	{ verts[i] = PxVec3(x,		0.0f,	y);		}
		else	if(orientation==ORIENTATION_YZ)	{ verts[i] = PxVec3(0.0f,	x,		y); 	}

		if(transform)
			verts[i] = transform->transform(verts[i]);
	}
	return true;
}


// PT: this comes from RendererCapsuleShape.cpp. Maybe we could grab the data from there instead of duplicating. But it protects us from external changes.
static const PxVec3 gCapsuleVertices[] =
{
	PxVec3(0.0000f, -2.0000f, -0.0000f),
	PxVec3(0.3827f, -1.9239f, -0.0000f),
	PxVec3(0.2706f, -1.9239f, 0.2706f),
	PxVec3(-0.0000f, -1.9239f, 0.3827f),
	PxVec3(-0.2706f, -1.9239f, 0.2706f),
	PxVec3(-0.3827f, -1.9239f, -0.0000f),
	PxVec3(-0.2706f, -1.9239f, -0.2706f),
	PxVec3(0.0000f, -1.9239f, -0.3827f),
	PxVec3(0.2706f, -1.9239f, -0.2706f),
	PxVec3(0.7071f, -1.7071f, -0.0000f),
	PxVec3(0.5000f, -1.7071f, 0.5000f),
	PxVec3(-0.0000f, -1.7071f, 0.7071f),
	PxVec3(-0.5000f, -1.7071f, 0.5000f),
	PxVec3(-0.7071f, -1.7071f, -0.0000f),
	PxVec3(-0.5000f, -1.7071f, -0.5000f),
	PxVec3(0.0000f, -1.7071f, -0.7071f),
	PxVec3(0.5000f, -1.7071f, -0.5000f),
	PxVec3(0.9239f, -1.3827f, -0.0000f),
	PxVec3(0.6533f, -1.3827f, 0.6533f),
	PxVec3(-0.0000f, -1.3827f, 0.9239f),
	PxVec3(-0.6533f, -1.3827f, 0.6533f),
	PxVec3(-0.9239f, -1.3827f, -0.0000f),
	PxVec3(-0.6533f, -1.3827f, -0.6533f),
	PxVec3(0.0000f, -1.3827f, -0.9239f),
	PxVec3(0.6533f, -1.3827f, -0.6533f),
	PxVec3(1.0000f, -1.0000f, -0.0000f),
	PxVec3(0.7071f, -1.0000f, 0.7071f),
	PxVec3(-0.0000f, -1.0000f, 1.0000f),
	PxVec3(-0.7071f, -1.0000f, 0.7071f),
	PxVec3(-1.0000f, -1.0000f, -0.0000f),
	PxVec3(-0.7071f, -1.0000f, -0.7071f),
	PxVec3(0.0000f, -1.0000f, -1.0000f),
	PxVec3(0.7071f, -1.0000f, -0.7071f),
	PxVec3(1.0000f, 1.0000f, 0.0000f),
	PxVec3(0.7071f, 1.0000f, 0.7071f),
	PxVec3(-0.0000f, 1.0000f, 1.0000f),
	PxVec3(-0.7071f, 1.0000f, 0.7071f),
	PxVec3(-1.0000f, 1.0000f, -0.0000f),
	PxVec3(-0.7071f, 1.0000f, -0.7071f),
	PxVec3(0.0000f, 1.0000f, -1.0000f),
	PxVec3(0.7071f, 1.0000f, -0.7071f),
	PxVec3(0.9239f, 1.3827f, 0.0000f),
	PxVec3(0.6533f, 1.3827f, 0.6533f),
	PxVec3(-0.0000f, 1.3827f, 0.9239f),
	PxVec3(-0.6533f, 1.3827f, 0.6533f),
	PxVec3(-0.9239f, 1.3827f, -0.0000f),
	PxVec3(-0.6533f, 1.3827f, -0.6533f),
	PxVec3(0.0000f, 1.3827f, -0.9239f),
	PxVec3(0.6533f, 1.3827f, -0.6533f),
	PxVec3(0.7071f, 1.7071f, 0.0000f),
	PxVec3(0.5000f, 1.7071f, 0.5000f),
	PxVec3(-0.0000f, 1.7071f, 0.7071f),
	PxVec3(-0.5000f, 1.7071f, 0.5000f),
	PxVec3(-0.7071f, 1.7071f, 0.0000f),
	PxVec3(-0.5000f, 1.7071f, -0.5000f),
	PxVec3(0.0000f, 1.7071f, -0.7071f),
	PxVec3(0.5000f, 1.7071f, -0.5000f),
	PxVec3(0.3827f, 1.9239f, 0.0000f),
	PxVec3(0.2706f, 1.9239f, 0.2706f),
	PxVec3(-0.0000f, 1.9239f, 0.3827f),
	PxVec3(-0.2706f, 1.9239f, 0.2706f),
	PxVec3(-0.3827f, 1.9239f, 0.0000f),
	PxVec3(-0.2706f, 1.9239f, -0.2706f),
	PxVec3(0.0000f, 1.9239f, -0.3827f),
	PxVec3(0.2706f, 1.9239f, -0.2706f),
	PxVec3(0.0000f, 2.0000f, 0.0000f),
};

static const PxU8 gCapsuleIndices[] =
{
	1, 0, 2,  2, 0, 3,  3, 0, 4,  4, 0, 5,  5, 0, 6,  6, 0, 7,  7, 0, 8,
	8, 0, 1,  9, 1, 10,  10, 1, 2,  10, 2, 11,  11, 2, 3,  11, 3, 12,
	12, 3, 4,  12, 4, 13,  13, 4, 5,  13, 5, 14,  14, 5, 6,  14, 6, 15,
	15, 6, 7,  15, 7, 16,  16, 7, 8,  16, 8, 9,  9, 8, 1,  17, 9, 18,
	18, 9, 10,  18, 10, 19,  19, 10, 11,  19, 11, 20,  20, 11, 12,  20, 12, 21,
	21, 12, 13,  21, 13, 22,  22, 13, 14,  22, 14, 23,  23, 14, 15,  23, 15, 24,
	24, 15, 16,  24, 16, 17,  17, 16, 9,  25, 17, 26,  26, 17, 18,  26, 18, 27,
	27, 18, 19,  27, 19, 28,  28, 19, 20,  28, 20, 29,  29, 20, 21,  29, 21, 30,
	30, 21, 22,  30, 22, 31,  31, 22, 23,  31, 23, 32,  32, 23, 24,  32, 24, 25,
	25, 24, 17,  33, 25, 34,  34, 25, 26,  34, 26, 35,  35, 26, 27,  35, 27, 36,
	36, 27, 28,  36, 28, 37,  37, 28, 29,  37, 29, 38,  38, 29, 30,  38, 30, 39,
	39, 30, 31,  39, 31, 40,  40, 31, 32,  40, 32, 33,  33, 32, 25,  41, 33, 42,
	42, 33, 34,  42, 34, 43,  43, 34, 35,  43, 35, 44,  44, 35, 36,  44, 36, 45,
	45, 36, 37,  45, 37, 46,  46, 37, 38,  46, 38, 47,  47, 38, 39,  47, 39, 48,
	48, 39, 40,  48, 40, 41,  41, 40, 33,  49, 41, 50,  50, 41, 42,  50, 42, 51,
	51, 42, 43,  51, 43, 52,  52, 43, 44,  52, 44, 53,  53, 44, 45,  53, 45, 54,
	54, 45, 46,  54, 46, 55,  55, 46, 47,  55, 47, 56,  56, 47, 48,  56, 48, 49,
	49, 48, 41,  57, 49, 58,  58, 49, 50,  58, 50, 59,  59, 50, 51,  59, 51, 60,
	60, 51, 52,  60, 52, 61,  61, 52, 53,  61, 53, 62,  62, 53, 54,  62, 54, 63,
	63, 54, 55,  63, 55, 64,  64, 55, 56,  64, 56, 57,  57, 56, 49,  65, 57, 58,
	65, 58, 59,  65, 59, 60,  65, 60, 61,  65, 61, 62,  65, 62, 63,  65, 63, 64,
	65, 64, 57,
};
static const PxU32 gNumCapsuleIndices = PX_ARRAY_SIZE(gCapsuleIndices);

static PX_FORCE_INLINE void fixCapsuleVertex(PxVec3& p, PxF32 radius, PxF32 halfHeight)
{
	const PxF32 sign = p.y > 0 ? 1.0f : -1.0f;
	p.y -= sign;
	p   *= radius;
	p.y += halfHeight*sign;
}

void RenderPhysX3Debug::addSphere(const PxSphereGeometry& sg, const PxTransform& tr,  const RendererColor& color, PxU32 renderFlags)
{
	addSphere(tr.p, sg.radius, color, renderFlags);
}

void RenderPhysX3Debug::addSphere(const PxVec3& sphereCenter, float sphereRadius, const RendererColor& color, PxU32 renderFlags)
{
	const PxU32 nbVerts = NB_CIRCLE_PTS;
	PxVec3 pts[NB_CIRCLE_PTS];

	if(renderFlags & RENDER_DEBUG_WIREFRAME)
	{
		generatePolygon(nbVerts, pts, ORIENTATION_XY, sphereRadius, 0.0f);
		addCircle(nbVerts, pts, color, sphereCenter);

		generatePolygon(nbVerts, pts, ORIENTATION_XZ, sphereRadius, 0.0f);
		addCircle(nbVerts, pts, color, sphereCenter);

		generatePolygon(nbVerts, pts, ORIENTATION_YZ, sphereRadius, 0.0f);
		addCircle(nbVerts, pts, color, sphereCenter);
	}

	if(renderFlags & RENDER_DEBUG_SOLID)
	{
		const PxF32 halfHeight = 0.0f;
		for(PxU32 i=0;i<gNumCapsuleIndices/3;i++)
		{
			const PxU32 i0 = gCapsuleIndices[i*3+0];
			const PxU32 i1 = gCapsuleIndices[i*3+1];
			const PxU32 i2 = gCapsuleIndices[i*3+2];
			PxVec3 v0 = gCapsuleVertices[i0];
			PxVec3 v1 = gCapsuleVertices[i1];
			PxVec3 v2 = gCapsuleVertices[i2];

			fixCapsuleVertex(v0, sphereRadius, halfHeight);
			fixCapsuleVertex(v1, sphereRadius, halfHeight);
			fixCapsuleVertex(v2, sphereRadius, halfHeight);

			addTriangle(v0+sphereCenter, v1+sphereCenter, v2+sphereCenter, color);
		}
	}
}

#define MAX_TEMP_VERTEX_BUFFER 400

// creaet triangle strip of spheres
static bool generateSphere(PxU32 nbSeg, PxU32& nbVerts, PxVec3* verts, PxVec3* normals)
{
	PxVec3 tempVertexBuffer[MAX_TEMP_VERTEX_BUFFER];
	PxVec3 tempNormalBuffer[MAX_TEMP_VERTEX_BUFFER];

	int halfSeg = nbSeg / 2;
	int nSeg = halfSeg * 2;

	if (((nSeg+1) * (nSeg+1)) > MAX_TEMP_VERTEX_BUFFER)
		return false;

	const float stepTheta = PxTwoPi / float(nSeg);
	const float stepPhi = PxPi / float(nSeg);

	// compute sphere vertices on the temporary buffer
	nbVerts = 0;
	for (int i = 0; i <= nSeg; i++)
	{
		const float theta = float(i) * stepTheta;
		const float cosi = cos(theta);
		const float sini = sin(theta);

		for (int j = -halfSeg; j <= halfSeg; j++)
		{
			const float phi = float(j) * stepPhi;
			const float sinj = sin( phi);
			const float cosj = cos( phi);

			const float y = cosj * cosi;
			const float x = sinj;
			const float z = cosj * sini;

			tempVertexBuffer[nbVerts] = PxVec3(x,y,z);
			tempNormalBuffer[nbVerts] = PxVec3(x,y,z).getNormalized();
			nbVerts++;
		}
	}

	nbVerts = 0;
	// now create triangle soup data
	for (int i = 0; i < nSeg; i++)
	{
		for (int j = 0; j < nSeg; j++)
		{
			// add one triangle
			verts[nbVerts] = tempVertexBuffer[ (nSeg+1) * i + j];
			normals[nbVerts] = tempNormalBuffer[ (nSeg+1) * i + j];
			nbVerts++;

			verts[nbVerts] = tempVertexBuffer[ (nSeg+1) * i + j+1];
			normals[nbVerts] = tempNormalBuffer[ (nSeg+1) * i + j+1];
			nbVerts++;

			verts[nbVerts] = tempVertexBuffer[ (nSeg+1) * (i+1) + j+1];
			normals[nbVerts] = tempNormalBuffer[ (nSeg+1) * (i+1) + j+1];
			nbVerts++;

			// add another triangle
			verts[nbVerts] = tempVertexBuffer[ (nSeg+1) * i + j];
			normals[nbVerts] = tempNormalBuffer[ (nSeg+1) * i + j];
			nbVerts++;

			verts[nbVerts] = tempVertexBuffer[ (nSeg+1) * (i+1) + j+1];
			normals[nbVerts] = tempNormalBuffer[ (nSeg+1) * (i+1) + j+1];
			nbVerts++;

			verts[nbVerts] = tempVertexBuffer[ (nSeg+1) * (i+1) + j];
			normals[nbVerts] = tempNormalBuffer[ (nSeg+1) * (i+1) + j];
			nbVerts++;

		}
	}

	return true;
}

void RenderPhysX3Debug::addSphereExt(const PxVec3& sphereCenter, float sphereRadius, const RendererColor& color, PxU32 renderFlags)
{
	if(renderFlags & RENDER_DEBUG_WIREFRAME)
	{
		const PxU32 nbVerts = NB_CIRCLE_PTS;
		PxVec3 pts[NB_CIRCLE_PTS];

		generatePolygon(nbVerts, pts, ORIENTATION_XY, sphereRadius, 0.0f);
		addCircle(nbVerts, pts, color, sphereCenter);

		generatePolygon(nbVerts, pts, ORIENTATION_XZ, sphereRadius, 0.0f);
		addCircle(nbVerts, pts, color, sphereCenter);

		generatePolygon(nbVerts, pts, ORIENTATION_YZ, sphereRadius, 0.0f);
		addCircle(nbVerts, pts, color, sphereCenter);
	}

	if(renderFlags & RENDER_DEBUG_SOLID)
	{
		static bool initDone = false;
		static PxU32 nbVerts;
		static PxVec3 verts[MAX_TEMP_VERTEX_BUFFER*6];
		static PxVec3 normals[MAX_TEMP_VERTEX_BUFFER*6];

		if (!initDone)
		{
			generateSphere(16, nbVerts, verts, normals);
			initDone = true;
		}

		PxU32 i = 0;
		while ( i < nbVerts )
		{
			addTriangle( sphereCenter + sphereRadius * verts[i], sphereCenter + sphereRadius * verts[i+1], sphereCenter + sphereRadius * verts[i+2],
				normals[i], normals[i+1], normals[i+2], color);
			i += 3;
		}
	}
}
#undef MAX_TEMP_VERTEX_BUFFFER

static inline PxU32 minArgument(const PxVec3 &v)
{
	PxU32 j = 0;
	if ( v[j] > v[1]) j = 1;
	if ( v[j] > v[2]) j = 2;
	return j;
}
static inline PxVec3 abs(const PxVec3 &v)
{
	return PxVec3( PxAbs(v.x), PxAbs(v.y), PxAbs(v.z));
}

void RenderPhysX3Debug::addConeExt(float r0, float r1, const PxVec3& p0, const PxVec3& p1 , const RendererColor& color, PxU32 renderFlags)
{
	PxVec3 axis = p1 - p0;
	PxReal length = axis.magnitude();
	PxReal rdiff = r0 - r1;
	PxReal sinAngle = rdiff / length;
	PxReal x0 = r0 * sinAngle;
	PxReal x1 = r1 * sinAngle;
	PxVec3 center = 0.5f * (p0 + p1);

	if (length < fabs(rdiff))
		return;

	PxReal r0p = sqrt(r0 * r0 - x0 * x0);
	PxReal r1p = sqrt(r1 * r1 - x1 * x1);

	if (length == 0.0f)
		axis = PxVec3(1,0,0);
	else
		axis.normalize();

	PxVec3 axis1(0.0f);
	axis1[minArgument(abs(axis))] = 1.0f;
	axis1 = axis1.cross(axis);
	axis1.normalize();

	PxVec3 axis2 = axis.cross(axis1);
	axis2.normalize();

	PxMat44 m;
	m.column0 = PxVec4(axis, 0.0f);
	m.column1 = PxVec4(axis1, 0.0f);
	m.column2 = PxVec4(axis2, 0.0f);
	m.column3 = PxVec4(center, 1.0f);

	PxTransform tr(m);

#define NUM_CONE_VERTS 72
	const PxU32 nbVerts = NUM_CONE_VERTS;

	PxVec3 pts0[NUM_CONE_VERTS] ;
	PxVec3 pts1[NUM_CONE_VERTS];
	PxVec3 normals[NUM_CONE_VERTS] ;

	const float step = PxTwoPi / float(nbVerts);
	for (PxU32 i = 0; i < nbVerts; i++)
	{
		const float angle = float(i) * step;
		const float x = cosf(angle);
		const float y = sinf(angle);

		PxVec3 p = PxVec3(0.0f, x, y);

		pts0[i] = tr.transform(r0p * p + PxVec3(-0.5f * length + x0,0,0));
		pts1[i] = tr.transform(r1p * p + PxVec3(0.5f * length + x1, 0, 0));

		normals[i] = tr.q.rotate(p.getNormalized());
		normals[i] = x0 * axis + r0p * normals[i];
		normals[i].normalize();
	}
#undef NUM_CONE_VERTS

	if(renderFlags & RENDER_DEBUG_WIREFRAME)
	{
		for(PxU32 i=0;i<nbVerts;i++)
		{
			addLine(pts1[i], pts0[i], color);
		}
	}

	if(renderFlags & RENDER_DEBUG_SOLID)
	{
		for(PxU32 i=0;i<nbVerts;i++)
		{
			const PxU32 j = (i+1) % nbVerts;
			addTriangle(pts0[i], pts1[j], pts0[j], normals[i], normals[j], normals[j], color);
			addTriangle(pts0[i], pts1[i], pts1[j], normals[i], normals[i], normals[j], color);
		}
	}

}

void RenderPhysX3Debug::addCone(float radius, float height, const PxTransform& tr, const RendererColor& color, PxU32 renderFlags)
{
	const PxU32 nbVerts = NB_CIRCLE_PTS;
	PxVec3 pts[NB_CIRCLE_PTS];
	generatePolygon(nbVerts, pts, ORIENTATION_XZ, radius, 0.0f, &tr);

	const PxVec3 tip = tr.transform(PxVec3(0.0f, height, 0.0f));

	if(renderFlags & RENDER_DEBUG_WIREFRAME)
	{
		addCircle(nbVerts, pts, color, PxVec3(0));
		for(PxU32 i=0;i<nbVerts;i++)
		{
			addLine(tip, pts[i], color);	// side of the cone
			addLine(tr.p, pts[i], color);	// base disk of the cone
		}
	}

	if(renderFlags & RENDER_DEBUG_SOLID)
	{
		for(PxU32 i=0;i<nbVerts;i++)
		{
			const PxU32 j = (i+1) % nbVerts;
			addTriangle(tip, pts[i], pts[j], color);
			addTriangle(tr.p, pts[i], pts[j], color);
		}
	}
}

void RenderPhysX3Debug::addCylinder(float radius, float height, const PxTransform& tr, const RendererColor& color, PxU32 renderFlags)
{
	const PxU32 nbVerts = NB_CIRCLE_PTS;
	PxVec3 pts[NB_CIRCLE_PTS];
	generatePolygon(nbVerts, pts, ORIENTATION_XZ, radius, 0.0f, &tr);

	PxTransform tr2 = tr;
	tr2.p = tr.transform(PxVec3(0.0f, height, 0.0f));
	PxVec3 pts2[NB_CIRCLE_PTS];
	generatePolygon(nbVerts, pts2, ORIENTATION_XZ, radius, 0.0f, &tr2);

	if(renderFlags & RENDER_DEBUG_WIREFRAME)
	{
		for(PxU32 i=0;i<nbVerts;i++)
		{
			const PxU32 j = (i+1) % nbVerts;
			addLine(pts[i], pts[j], color);		// circle
			addLine(pts2[i], pts2[j], color);	// circle
		}

		for(PxU32 i=0;i<nbVerts;i++)
		{
			addLine(pts[i], pts2[i], color);	// side
			addLine(tr.p, pts[i], color);		// disk
			addLine(tr2.p, pts2[i], color);		// disk
		}
	}

	if(renderFlags & RENDER_DEBUG_SOLID)
	{
		for(PxU32 i=0;i<nbVerts;i++)
		{
			const PxU32 j = (i+1) % nbVerts;
			addTriangle(tr.p, pts[i], pts[j], color);
			addTriangle(tr2.p, pts2[i], pts2[j], color);
			addTriangle(pts[i], pts[j], pts2[j], color);
			addTriangle(pts[i], pts2[j], pts2[i], color);
		}
	}
}


void RenderPhysX3Debug::addStar(const PxVec3& p, const float size, const RendererColor& color )
{
	const PxVec3 up(0.f, size, 0.f);
	const PxVec3 right(size, 0.f, 0.f);
	const PxVec3 forwards(0.f, 0.f, size);
	addLine(p + up, p - up, color);
	addLine(p + right, p - right, color);
	addLine(p + forwards, p - forwards, color);
}

void RenderPhysX3Debug::addCapsule(const PxCapsuleGeometry& cg, const PxTransform& tr, const RendererColor& color, PxU32 renderFlags)
{
	PxTransform pose = PxTransform(PxVec3(0.f), PxQuat(PxPi/2, PxVec3(0,0,1)));
	pose = tr * pose;

	PxVec3 p0(0, -cg.halfHeight, 0);
	PxVec3 p1(0, cg.halfHeight, 0);

	p0 = pose.transform(p0);
	p1 = pose.transform(p1);

	pose.p = p0;
	/*PxTransform pose = PxTransform(PxVec3(0.f), PxQuat(PxPi/2, PxVec3(0,0,1)));
	pose = tr * pose;*/

	//const PxReal height = cg.halfHeight;
	//const PxVec3 p0 = tr.p - PxVec3(0, height, 0);
	//const PxVec3 p1 = tr.p + PxVec3(0, height, 0);
	addCapsule(p0, p1, cg.radius, 2*cg.halfHeight, pose, color, renderFlags);
}

void RenderPhysX3Debug::addCapsule(const PxVec3& p0, const PxVec3& p1, const float radius, const float height, const PxTransform& tr, const RendererColor& color, PxU32 renderFlags)
{
	addSphere(p0, radius, color, renderFlags);
	addSphere(p1, radius, color, renderFlags);
	addCylinder(radius, height, tr, color, renderFlags);
}

void RenderPhysX3Debug::addRectangle(float width, float length, const PxTransform& tr, const RendererColor& color)
{
	PxMat33 m33 = PxMat33(tr.q);
	PxVec3 Axis1 = m33.column1;
	PxVec3 Axis2 = m33.column2;

	Axis1 *= length;
	Axis2 *= width;

	PxVec3 pts[4];
	pts[0] = tr.p + Axis1 + Axis2 ;
	pts[1] = tr.p - Axis1 + Axis2 ;
	pts[2] = tr.p - Axis1 - Axis2 ;
	pts[3] = tr.p + Axis1 - Axis2 ;

	addTriangle(pts[0], pts[1], pts[2], color);
	addTriangle(pts[0], pts[2], pts[3], color);
}

void RenderPhysX3Debug::addGeometry(const PxGeometry& geom, const PxTransform& tr,  const RendererColor& color, PxU32 renderFlags)
{
	switch(geom.getType())
    {
		case PxGeometryType::eBOX:
		{
			addBox(static_cast<const PxBoxGeometry&>(geom), tr, color, renderFlags);
		}
		break;
		case PxGeometryType::eSPHERE:
		{
			addSphere(static_cast<const PxSphereGeometry&>(geom),  tr, color, renderFlags);
		}
		break;
		case PxGeometryType::eCAPSULE:
		{
			addCapsule(static_cast<const PxCapsuleGeometry&>(geom), tr, color, renderFlags);
		}
		break;
		case PxGeometryType::eCONVEXMESH:
		{
			addConvex(static_cast<const PxConvexMeshGeometry&>(geom), tr, color, renderFlags);
		}
		break;
		case PxGeometryType::ePLANE:
		case PxGeometryType::eTRIANGLEMESH:
		case PxGeometryType::eHEIGHTFIELD:
		default:
		{
			PX_ASSERT(!"Not supported!");
			break;
		}
	}
}

void RenderPhysX3Debug::addConvex(const PxConvexMeshGeometry& cg, const PxTransform& tr,  const RendererColor& color, PxU32 renderFlags)
{
	const PxConvexMesh& mesh = *cg.convexMesh;

	const PxMat33 rot = PxMat33(tr.q) * cg.scale.toMat33();

	// PT: you can't use PxTransform with a non-uniform scaling
	const PxMat44 globalPose(rot, tr.p);
	const PxU32 polygonCount = mesh.getNbPolygons();
	const PxU8* indexBuffer = mesh.getIndexBuffer();
	const PxVec3* vertexBuffer = mesh.getVertices();

	if(renderFlags & RENDER_DEBUG_WIREFRAME)
	{
		for(PxU32 i=0; i<polygonCount; i++)
		{
			PxHullPolygon data;
			mesh.getPolygonData(i, data);

			const PxU32 vertexCount = data.mNbVerts;
			PxU32 i0 = indexBuffer[vertexCount-1];
			PxU32 i1 = *indexBuffer++;
			addLine(globalPose.transform(vertexBuffer[i0]), globalPose.transform(vertexBuffer[i1]), color);
			for(PxU32 j=1; j<vertexCount; j++)
			{
				i0 = indexBuffer[-1];
				i1 = *indexBuffer++;
				addLine(globalPose.transform(vertexBuffer[i0]), globalPose.transform(vertexBuffer[i1]), color);
			}
		}
	}

	if(renderFlags & RENDER_DEBUG_SOLID)
	{
		for(PxU32 i=0; i<polygonCount; i++)
		{
			PxHullPolygon data;
			mesh.getPolygonData(i, data);

			const PxU32 vertexCount = data.mNbVerts;

			const PxVec3& v0 = vertexBuffer[indexBuffer[0]];
			for(PxU32 j=0; j<vertexCount-2; j++)
			{
				const PxVec3& v1 = vertexBuffer[indexBuffer[j+1]];
				const PxVec3& v2 = vertexBuffer[indexBuffer[j+2]];

				addTriangle(globalPose.transform(v0), globalPose.transform(v1), globalPose.transform(v2), color);
			}
			indexBuffer += vertexCount;
		}
	}
}

void RenderPhysX3Debug::addArrow(const PxVec3& posA, const PxVec3& posB, const RendererColor& color)
{
	const PxVec3 t0 = (posB - posA).getNormalized();
	const PxVec3 a = PxAbs(t0.x)<0.707f ? PxVec3(1,0,0): PxVec3(0,1,0);
	const PxVec3 t1 = t0.cross(a).getNormalized();
	const PxVec3 t2 = t0.cross(t1).getNormalized();

	addLine(posA, posB, color);
	addLine(posB, posB - t0*0.15f + t1 * 0.15f, color);
	addLine(posB, posB - t0*0.15f - t1 * 0.15f, color);
	addLine(posB, posB - t0*0.15f + t2 * 0.15f, color);
	addLine(posB, posB - t0*0.15f - t2 * 0.15f, color);
}
