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
#include "PxFileBuf.h"

#include "authoring/ApexCSGDefs.h"
#include "authoring/ApexCSGSerialization.h"
#include "ApexSharedSerialization.h"
#include "RenderDebugInterface.h"

#include <stdio.h>

#include "PxErrorCallback.h"


#ifndef WITHOUT_APEX_AUTHORING

using namespace nvidia;
using namespace apex;
using namespace nvidia;

namespace ApexCSG
{

// Tolerances for geometric calculations

#define CSG_EPS	((Real)1.0e-9)

BSPTolerances
gDefaultTolerances;


// Set to 1 to Measure the stack
#define MEASURE_STACK_USAGE	0

#if MEASURE_STACK_USAGE
static size_t
gStackTop = (size_t)-1;
static size_t
gStackBottom = (size_t)-1;

#define RECORD_STACK_TOP() \
{ \
	int x; \
	gStackTop = (size_t)&x; \
	gStackBottom = (size_t)-1; \
}
#define RECORD_STACK_BOTTOM() \
{ \
	int x; \
	gStackBottom = PxMin(gStackBottom, (size_t)&x); \
}
#define OUTPUT_STACK_USAGE(fn_name) \
{ \
	char stackMsg[100]; \
	sprintf(stackMsg, "%s stack usage: %d bytes", #fn_name, gStackTop - gStackBottom); \
	debugWarn(stackMsg); \
}
#else
#define RECORD_STACK_TOP()
#define RECORD_STACK_BOTTOM()
#define OUTPUT_STACK_USAGE(fn_name)
#endif


/* Interpolator */

size_t
Interpolator::s_offsets[Interpolator::VertexFieldCount];

static InterpolatorBuilder
sInterpolatorBuilder;

void
Interpolator::serialize(physx::PxFileBuf& stream) const
{
	for (uint32_t i = 0; i < VertexFieldCount; ++i)
	{
		stream << m_frames[i];
	}
}

void
Interpolator::deserialize(physx::PxFileBuf& stream, uint32_t version)
{
	if (version < Version::SerializingTriangleFrames)
	{
		return;
	}

	for (uint32_t i = 0; i < VertexFieldCount; ++i)
	{
		stream >> m_frames[i];
	}
}


/* Utilities */

#define debugInfo(_msg)	GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_INFO, _msg, __FILE__, __LINE__)
#define debugWarn(_msg)	GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING, _msg, __FILE__, __LINE__)

static bool
transformsEqual(const physx::PxMat44& a, const physx::PxMat44& b, float eps)
{
	const float eps2 = eps*eps;
	const float scaledEps = eps*PxMax(a.getPosition().abs().maxElement(), b.getPosition().abs().maxElement());

	for (unsigned i = 0; i < 4; ++i)
	{
		for (unsigned j = 0; j < 4; ++j)
		{
			const float tol = i == j ? eps2 : (i == 4 ? scaledEps : eps);
			if (!physx::PxEquals(a(i,j), b(i,j), tol))
			{
				return false;
			}
		}
	}

	return true;
}

static bool
isZero(const physx::PxMat44& a)
{
	for (unsigned i = 0; i < 4; ++i)
	{
		if (!a[i].isZero())
		{
			return false;
		}
	}

	return true;
}

PX_INLINE Mat4Real
CSGFromPx(const physx::PxMat44& a)
{
	Mat4Real r;
	for (unsigned i = 0; i < 4; ++i)
	{
		for (unsigned j = 0; j < 4; ++j)
		{
			r[i][j] = (Real)a(i,j);
		}
	}
	return r;
}

PX_INLINE physx::PxMat44
PxFromCSG(const Mat4Real& a)
{
	physx::PxMat44 r;
	for (unsigned i = 0; i < 4; ++i)
	{
		for (unsigned j = 0; j < 4; ++j)
		{
			r(i,j) = (float)a[i][j];
		}
	}
	return r;
}

class DefaultRandom : public UserRandom
{
public:
	uint32_t	getInt()
	{
		return m_rnd.nextSeed();
	}
	float	getReal(float min, float max)
	{
		return m_rnd.getScaled(min, max);
	}

	QDSRand	m_rnd;
} defaultRnd;

PX_INLINE int	// Returns 0 if the point is on the plane (within tolerance), otherwise +/-1 if the point is above/below the plane
cmpPointToPlane(const Pos& pos, const Plane& plane, Real tol)
{
	const Real dist = plane.distance(pos);

	return dist < -tol ? -1 : (dist < tol ? 0 : 1);
}

template<typename T>
struct IndexedValue
{
	T			value;
	uint32_t	index;

	static	int	cmpIncreasing(const void* a, const void* b)
	{
		return ((IndexedValue*)a)->value == ((IndexedValue*)b)->value ? 0 : (((IndexedValue*)a)->value < ((IndexedValue*)b)->value ? -1 : 1);
	}
};


PX_INLINE LinkedVertex*
clipPolygonByPlane(LinkedVertex* poly, const Plane& plane, Pool<LinkedVertex>& pool, Real tol)
{
	LinkedVertex* prev = poly;
	int prevSide = cmpPointToPlane(prev->vertex, plane, tol);
	bool outsideFound = prevSide == 1;
	bool insideFound = prevSide == -1;
	LinkedVertex* clip0 = NULL;
	LinkedVertex* clip1 = NULL;
	LinkedVertex* next = prev->getAdj(1);
	if (next != poly)
	{
		do
		{
			int nextSide = cmpPointToPlane(next->vertex, plane, tol);
			switch (nextSide)
			{
			case -1:
				insideFound = true;
				if (prevSide == 1)
				{
					// Clip
					clip1 = pool.borrow();
					const Dir disp = next->vertex - prev->vertex;
					const Real dDisp = disp | plane.normal();
					PX_ASSERT(dDisp < 0);
					const Real dAbove = plane.distance(prev->vertex);
					clip1->vertex = prev->vertex - (dAbove / dDisp) * disp;
					next->setAdj(0, clip1);	// Insert clip1 between prev and next
				}
				else if (prevSide == 0)
				{
					clip1 = prev;
				}
				break;
			case 0:
				if (prevSide == -1)
				{
					clip0 = next;
				}
				else if (prevSide == 1)
				{
					clip1 = next;
				}
				break;
			case 1:
				outsideFound = true;
				if (prevSide == -1)
				{
					// Clip
					clip0 = pool.borrow();
					const Dir disp = next->vertex - prev->vertex;
					const Real dDisp = disp | plane.normal();
					PX_ASSERT(dDisp > 0);
					const Real dBelow = plane.distance(prev->vertex);
					clip0->vertex = prev->vertex - (dBelow / dDisp) * disp;
					next->setAdj(0, clip0);	// Insert clip0 between prev and next
				}
				else if (prevSide == 0)
				{
					clip0 = prev;
				}
				break;
			}
			prev = next;
			prevSide = nextSide;
			next = prev->getAdj(1);
		}
		while (prev != poly);
	}

	PX_ASSERT((clip0 != NULL) == (clip1 != NULL));

	if (clip0 != NULL && clip1 != NULL && clip0 != clip1)
	{
		// Get rid of vertices between clip0 and clip1
		LinkedVertex* v = clip0->getAdj(1);
		while (v != clip1)
		{
			LinkedVertex* w = v->getAdj(1);
			v->remove();
			pool.replace(v);
			v = w;
		}
		poly = clip1;
	}

	if (outsideFound && !insideFound)
	{
		// Completely outside.  Eliminate.
		LinkedVertex* v;
		do
		{
			v = poly->getAdj(0);
			v->remove();
			pool.replace(v);
		}
		while (v != poly);
		poly = NULL;
	}

	return poly;
}

// If clippedMesh is not NULL, it will be appended with clipped triangles from the leaf.
// Return value is the sum triangle area on the leaf.
PX_INLINE void
clipTriangleToLeaf(physx::Array<Triangle>* clippedMesh, Real& clippedTriangleArea, Real& clippedPyramidVolume, const Pos& origin,
				   const Triangle& tri, const BSP::Node* leaf, uint32_t edgeTraversalDir,
                   Pool<LinkedVertex>& vertexPool, Real distanceTol, const physx::Array<Plane>& planes, uint32_t skipPlaneIndex = 0xFFFFFFFF)
{
	clippedTriangleArea = (Real)0;
	clippedPyramidVolume = (Real)0;

	// Form a ring of vertices out of the triangle
	LinkedVertex* v0 = vertexPool.borrow();
	LinkedVertex* v1 = vertexPool.borrow();
	LinkedVertex* v2 = vertexPool.borrow();
	v0->vertex = tri.vertices[0];
	v1->vertex = tri.vertices[1];
	v2->vertex = tri.vertices[2];
	v0->setAdj(edgeTraversalDir, v1);
	v1->setAdj(edgeTraversalDir, v2);

	for (SurfaceIt it(leaf); it.valid(); it.inc())
	{
		if (it.surface()->planeIndex == skipPlaneIndex)
		{
			continue;
		}
		const Real sign = it.side() ? (Real)1 : -(Real)1;
		v0 = clipPolygonByPlane(v0, sign * planes[it.surface()->planeIndex], vertexPool, distanceTol);
		if (v0 == NULL)
		{
			break;	// Completely clipped away
		}
	}

	if (v0 != NULL)
	{
		// Something remains.  Add to clippedMesh if it's not NULL
		v1 = v0->getAdj(1);
		v2 = v1->getAdj(1);
		if (v1 != v0 && v2 != v0)
		{
			if (clippedMesh != NULL)
			{
				// Triangluate
				do
				{
					Triangle& newTri = clippedMesh->insert();
					newTri.vertices[0] = v0->vertex;
					newTri.vertices[1] = v1->vertex;
					newTri.vertices[2] = v2->vertex;
					newTri.submeshIndex = tri.submeshIndex;
					newTri.smoothingMask = tri.smoothingMask;
					newTri.extraDataIndex = tri.extraDataIndex;
					newTri.calculateQuantities();
					clippedTriangleArea += newTri.area;
					clippedPyramidVolume += newTri.area*((newTri.vertices[0]-origin)|newTri.normal);	// 3 * volume
					v1 = v2;
					v2 = v2->getAdj(1);
				}
				while (v2 != v0);
			}
			else
			{
				// Triangluate
				do
				{
					Dir normal = Dir(v1->vertex - v0->vertex)^Dir(v2->vertex - v0->vertex);
					const Real area = (Real)0.5 * normal.normalize();
					clippedTriangleArea += area;
					clippedPyramidVolume += area*((v0->vertex-origin)|normal);	// 3 * volume
					v1 = v2;
					v2 = v2->getAdj(1);
				}
				while (v2 != v0);
			}
		}
		// Return links to pool.
		LinkedVertex* v;
		do
		{
			v = v0->getAdj(0);
			v->remove();
			vertexPool.replace(v);
		}
		while (v != v0);
	}

	clippedPyramidVolume *= (Real)0.333333333333333333;
}

PX_INLINE bool
intersectPlanes(Pos& pos, Dir& dir, const Plane& plane0, const Plane& plane1)
{
	const Dir n0 = plane0.normal();
	const Dir n1 = plane1.normal();

	dir = n0^n1;
	const Real dir2 = dir.lengthSquared();
	if (dir2 < square(EPS_REAL))
	{
		return false;
	}

	const Real recipDir2 = (Real)1/dir2;	// DIVIDE

	// Normalize dir
	dir *= sqrt(recipDir2);

	// Calculate point in both planes
	const Real n0n0RecipDir2 = n0.lengthSquared()*recipDir2;
	const Real n1n1RecipDir2 = n1.lengthSquared()*recipDir2;
	const Real n0n1RecipDir2 = (n0|n1)*recipDir2;
	pos = Pos((Real)0) + (plane1.d()*n0n1RecipDir2 - plane0.d()*n1n1RecipDir2)*n0 + (plane0.d()*n0n1RecipDir2 - plane1.d()*n0n0RecipDir2)*n1;

	// Improve accuracy of solution
	const Real error0 = pos|plane0;
	const Real error1 = pos|plane1;
	pos += (error1*n0n1RecipDir2 - error0*n1n1RecipDir2)*n0 + (error0*n0n1RecipDir2 - error1*n0n0RecipDir2)*n1;

	return true;
}

PX_INLINE bool
intersectLineWithHalfspace(Real& minS, Real& maxS, const Pos& pos, const Dir& dir, const Plane& plane)
{
	const Real num = -(pos|plane);
	const Real den = dir|plane;
	if (den < -CSG_EPS)
	{
		const Real s = num/den;
		if (s > minS)
		{
			minS = s;
		}
	}
	else
	if (den > CSG_EPS)
	{
		const Real s = num/den;
		if (s < maxS)
		{
			maxS = s;
		}
	}
	else
	if (num < -CSG_EPS)
	{
		minS = CSG_EPS;
		maxS = -CSG_EPS;
	}

	return minS < maxS;
}

// Returns true if the leaf has finite area and volume, false otherwise
PX_INLINE bool
calculateLeafAreaAndVolume(Real& area, Real& volume, const Plane* planes, uint32_t planeCount, const Mat4Real& cofInternalTransform)
{
	if (planeCount <= 1)
	{
		area = MAX_REAL;
		volume = MAX_REAL;
		return false;
	}

	area = (Real)0;
	volume = (Real)0;

	bool originSet = false;
	Pos origin(0.0f);
	for (uint32_t i = 0; i < planeCount; ++i)
	{
		bool p0Set = false;
		Pos p0(0.0f);
		Real h = (Real)0;
		Real faceArea = (Real)0;
		for (uint32_t j = 0; j < planeCount; ++j)
		{
			if (j == i)
			{
				continue;
			}
			Pos pos;
			Dir dir;
			if (!intersectPlanes(pos, dir, planes[i], planes[j]))
			{
				continue;
			}
			Pos v1, v2;
			Real minS = -MAX_REAL;
			Real maxS = MAX_REAL;
			for (uint32_t k = 0; k < planeCount; ++k)
			{
				if (k == j || k == i)
				{
					continue;
				}
				if (!intersectLineWithHalfspace(minS, maxS, pos, dir, planes[k]))
				{
					break;
				}
			}

			if (minS >= maxS)
			{
				continue;
			}

			if (minS == -MAX_REAL || maxS == MAX_REAL)
			{
				area = MAX_REAL;
				volume = MAX_REAL;
				return false;
			}

			const Pos p1 = pos + minS*dir;
			if (!originSet)
			{
				origin = p1;
				originSet = true;
			}
			if (!p0Set)
			{
				p0 = p1;
				h = (p0-origin)|planes[i];
				p0Set = true;
				continue;	// The edge (p1,p2) won't contribute to the area or volume
			}
			const Pos p2 = pos + maxS*dir;
			faceArea += (Dir(p1-p0)^Dir(p2-p0))|planes[i];
		}
		area += faceArea*physx::PxSqrt((cofInternalTransform*planes[i].normal()).lengthSquared());
		volume += faceArea*h;
	}

	area *= (Real)0.5;
	volume *= (Real)0.16666666666666666667*cofInternalTransform[3][3];

	return true;
}


// GSA for a generic plane container
struct PlaneIteratorInit
{
	PlaneIteratorInit() : first(NULL), stop(NULL) {}

	Plane*	first;
	Plane*	stop;
};

class PlaneIterator
{
public:
	PlaneIterator(const PlaneIteratorInit& listBounds) : current(listBounds.first), stop(listBounds.stop) {}

	bool	valid() const
	{
		return current != stop;
	}

	void	inc()
	{
		++current;
	}

	Plane	plane() const
	{
		return *current;
	}

private:
	Plane* current;
	Plane* stop;
};

class HalfspaceIntersection : public ApexCSG::GSA::StaticConvexPolyhedron<PlaneIterator, PlaneIteratorInit>
{
public:
	void	setPlanes(Plane* first, uint32_t count)
	{
		m_initValues.first = first;
		m_initValues.stop = first + count;
	}
};


/* BSP */

BSP::BSP(IApexBSPMemCache* memCache, const physx::PxMat44& internalTransform) :
	m_root(NULL),
	m_meshSize(1),
	m_meshBounds(physx::PxBounds3::empty()),
	m_internalTransform(internalTransform),
	m_internalTransformInverse(CSGFromPx(internalTransform).inverse34()),
	m_incidentalMesh(false),
	m_combined(false),
	m_combiningMeshSize(1),
	m_combiningIncidentalMesh(false),
	m_memCache((BSPMemCache*)memCache),
	m_ownsMemCache(false)
{
	if (m_memCache == NULL)
	{
		m_memCache = (BSPMemCache*)createBSPMemCache();
		m_ownsMemCache = true;
	}

	// Always have a node.  The trivial (one-leaf) tree is considered "inside".
	m_root = m_memCache->m_nodePool.borrow();
}

BSP::~BSP()
{
	if (m_ownsMemCache)
	{
		m_memCache->release();
	}
}

void
BSP::setTolerances(const BSPTolerances& tolerances)
{
	m_tolerarnces = tolerances;
}

bool
BSP::fromMesh(const nvidia::ExplicitRenderTriangle* mesh, uint32_t triangleCount, const BSPBuildParameters& params, IProgressListener* progressListener, volatile bool* cancel)
{
	if (triangleCount == 0)
	{
		return false;
	}

	clear();

	// Shuffle triangle ordering
	physx::Array<uint32_t> triangleOrder(triangleCount);
	for (uint32_t i = 0; i < triangleCount; ++i)
	{
		triangleOrder[i] = i;
	}
	UserRandom* rnd = params.rnd != NULL ? params.rnd : &defaultRnd;
	for (uint32_t i = 0; i < triangleCount; ++i)
	{
		nvidia::swap(triangleOrder[i], triangleOrder[i + (uint32_t)(((uint64_t)rnd->getInt() * (uint64_t)(triangleCount - i)) >> 32)]);
	}

	// Collect mesh triangles and find mesh bounds
	m_mesh.resize(triangleCount);
	m_frames.resize(triangleCount);
	m_meshBounds.setEmpty();
	for (uint32_t i = 0; i < m_mesh.size(); ++i)
	{
		const ExplicitRenderTriangle& inTri = mesh[triangleOrder[i]];
		VertexData vertexData[3];
		m_mesh[i].fromExplicitRenderTriangle(vertexData, inTri);
		m_frames[i].setFromTriangle(m_mesh[i], vertexData);
		m_meshBounds.include(inTri.vertices[0].position);
		m_meshBounds.include(inTri.vertices[1].position);
		m_meshBounds.include(inTri.vertices[2].position);
	}

	// Size scales
	const Dir extents(m_meshBounds.getExtents());
	m_meshSize = PxMax(extents[0], PxMax(extents[1], extents[2]));

	// Scale to unit size and zero offset for BSP building
	const Vec4Real recipScale((extents[0] > m_tolerarnces.linear ? extents[0] : (Real)1), (extents[1] > m_tolerarnces.linear ? extents[1] : (Real)1), (extents[2] > m_tolerarnces.linear ? extents[2] : (Real)1), (Real)1);
	const Vec4Real scale((Real)1/recipScale[0], (Real)1/recipScale[1], (Real)1/recipScale[2], (Real)1);
	const Pos center = m_meshBounds.getCenter();
	const Real gridSize = (Real)params.snapGridSize;
	const Real recipGridSize = params.snapGridSize > 0 ? (Real)1/gridSize : (Real)0;
	// Rescale
	for (physx::PxU32 i = 0; i < m_mesh.size(); ++i)
	{
		Triangle& tri = m_mesh[i];
		for (physx::PxU32 j = 0; j < 3; ++j)
		{
			Pos& pos = tri.vertices[j];
			pos = (pos - center) * scale;
		}
	}

	// Align vertices
	if (params.snapGridSize > 0)
	{
		physx::Array< IndexedValue<Real> > snapValues[3];	// x, y, and z
		snapValues[0].resize(3*m_mesh.size());
		snapValues[1].resize(3*m_mesh.size());
		snapValues[2].resize(3*m_mesh.size());
		for (physx::PxU32 i = 0; i < m_mesh.size(); ++i)
		{
			Triangle& tri = m_mesh[i];
			for (physx::PxU32 j = 0; j < 3; ++j)
			{
				const Pos& pos = tri.vertices[j];
				for (int e = 0; e < 3; ++e)
				{
					const physx::PxU32 index = i*3+j;
					IndexedValue<Real>& v = snapValues[e][index];
					v.index = index;
					v.value = pos[e];
				}
			}
		}

		for (int e = 0; e < 3; ++e)
		{
			for (physx::PxU32 valueNum = 0; valueNum < snapValues[e].size(); ++valueNum)
			{
				const physx::PxU32 index = snapValues[e][valueNum].index;
				const physx::PxU32 i = index/3;
				const physx::PxU32 j = index-3*i;
				m_mesh[i].vertices[j][e] = recipGridSize*floor(gridSize * snapValues[e][valueNum].value + (Real)0.5);
			}
		}
	}

	// Cache triangle quantities
	for (physx::PxU32 i = 0; i < m_mesh.size(); ++i)
	{
		m_mesh[i].calculateQuantities();
	}

	// Initialize surface stack with surfaces formed from mesh triangles
	physx::Array<Surface> surfaceStack;

	// Crude estimate, hopefully will reduce re-allocations
	surfaceStack.reserve(m_mesh.size() * ((int)physx::PxLog((float)m_mesh.size()) + 1));

	// Track maximum and total surface triangle area
	float maxArea = 0;
	Real totalArea = 0;

	// Add mesh triangles
	uint32_t triangleIndex = 0;
	while (triangleIndex < m_mesh.size())
	{
		// Create a surface for the next triangle
		const Triangle& tri = m_mesh[triangleIndex];
		surfaceStack.pushBack(Surface());
		Surface* surface = &surfaceStack.back();
		surface->planeIndex = m_planes.size();
		surface->triangleIndexStart = triangleIndex++;
		Real surfaceTotalTriangleArea = tri.area;
		Plane& plane = m_planes.insert();
		plane.set(tri.normal, (tri.vertices[0] + tri.vertices[1] + tri.vertices[2])/(Real)3);
		plane.normalize();
		// See if any of the remaining triangles can fit on this surface.
		for (uint32_t testTriangleIndex = triangleIndex; testTriangleIndex < m_mesh.size(); ++testTriangleIndex)
		{
			Triangle& testTri = m_mesh[testTriangleIndex];
			if ((testTri.normal ^ plane.normal()).lengthSquared() < square(m_tolerarnces.angular) && (testTri.normal | plane.normal()) > 0 &&
			        0 == cmpPointToPlane(testTri.vertices[0], plane, m_tolerarnces.linear) &&
			        0 == cmpPointToPlane(testTri.vertices[1], plane, m_tolerarnces.linear) &&
			        0 == cmpPointToPlane(testTri.vertices[2], plane, m_tolerarnces.linear))
			{
				// This triangle fits.  Move it next to others in the surface.
				if (testTriangleIndex != triangleIndex)
				{
					nvidia::swap(m_mesh[triangleIndex], m_mesh[testTriangleIndex]);
					nvidia::swap(m_frames[triangleIndex], m_frames[testTriangleIndex]);
				}
				Triangle& newTri = m_mesh[triangleIndex];
				// Add in the new normal, properly weighted
				Dir averageNormal = surfaceTotalTriangleArea * plane.normal() + newTri.area * m_mesh[triangleIndex].normal;
				averageNormal.normalize();
				surfaceTotalTriangleArea += newTri.area;
				++triangleIndex;
				// Calculate the average projection
				Real averageProjection = 0;
				for (uint32_t i = surface->triangleIndexStart; i < triangleIndex; ++i)
				{
					for (uint32_t j = 0; j < 3; ++j)
					{
						averageProjection += averageNormal | m_mesh[i].vertices[j];
					}
				}
				averageProjection /= 3 * (triangleIndex - surface->triangleIndexStart);
				plane.set(averageNormal, -averageProjection);
			}
		}
		surface->triangleIndexStop = triangleIndex;
		surface->totalTriangleArea = (float)surfaceTotalTriangleArea;
		maxArea = PxMax(maxArea, surface->totalTriangleArea);
		totalArea += surfaceTotalTriangleArea;
		// Ensure triangles lie on or below surface
		Real maxProjection = -MAX_REAL;
		for (uint32_t i = surface->triangleIndexStart; i < surface->triangleIndexStop; ++i)
		{
			Triangle& tri = m_mesh[i];
			for (uint32_t j = 0; j < 3; ++j)
			{
				maxProjection = PxMax(maxProjection, plane.normal() | tri.vertices[j]);
			}
		}
		plane[3] = -maxProjection;
	}

	// Set build process constants
	BuildConstants buildConstants;
	buildConstants.m_params = params;
	buildConstants.m_recipMaxArea = maxArea > 0 ? 1.0f / maxArea : (float)0;

	// Build
	m_root = m_memCache->m_nodePool.borrow();
	PX_ASSERT(m_root != NULL);

	QuantityProgressListener quantityListener(totalArea, progressListener);
	bool ok = buildTree(m_root, surfaceStack, 0, surfaceStack.size(), buildConstants, &quantityListener, cancel);
	if (!ok)
	{
		return false;
	}

	// Bring the mesh back to actual size
	Mat4Real tm;
	tm.set((Real)1);
	for (uint32_t i = 0; i < 3; ++i)
	{
		tm[i][i] = recipScale[i];
		tm[i][3] = center[i];
	}

	// Currently the BSP is in "unit space", and tm will transform the BSP back to mesh space.
	// If params.internalTransform is valid, then the user is asking that:
	// (BSP space) = (params.internalTransform)(mesh space)
	// But (mesh space) = (tm)(unit space), so (BSP space) = (params.internalTransform)(tm)(unit space),
	// and therefore we apply (params.internalTransform)(tm).
	// If params.internalTransform is not valid, then the user is asking to keep the BSP in unit space,
	// so our effective internalTransform is the inverse of tm.
	if (!isZero(params.internalTransform))
	{
		m_internalTransform = params.internalTransform;
		const Mat4Real internalTransformCSG = CSGFromPx(m_internalTransform);
		m_internalTransformInverse = internalTransformCSG.inverse34();
		const Real meshSize = m_meshSize;	// Save off mesh size.  This gets garbled by scaled transforms.
		transform(internalTransformCSG*tm, false);
		const Real maxScale = PxMax(internalTransformCSG[0].lengthSquared(), PxMax(internalTransformCSG[1].lengthSquared(), internalTransformCSG[2].lengthSquared()));
		m_meshSize = meshSize*maxScale;
	}
	else
	{
		m_internalTransformInverse = tm;
		m_internalTransform = PxFromCSG(tm.inverse34());
		m_meshSize = (Real)1;
	}

	// Delete triangle info if requested.  This is done here in case any of the processing above needs this info.
	if (!params.keepTriangles)
	{
		deleteTriangles();
	}

//	performDiagnostics();

	return true;
}

bool
BSP::fromConvexPolyhedron(const physx::PxPlane* poly, uint32_t polySize, const physx::PxMat44& internalTransform, const nvidia::ExplicitRenderTriangle* mesh, uint32_t triangleCount)
{
	clear();

	// Default is all space.  If there are no planes, that is the result.
	m_root = m_memCache->m_nodePool.borrow();
	PX_ASSERT(m_root != NULL);

	if (polySize == 0)
	{
		return true;
	}

	// Put planes into our format
	m_planes.resize(polySize);
	for (uint32_t planeIndex = 0; planeIndex < polySize; ++planeIndex)
	{
		for (unsigned i = 0; i < 3; ++i)
		{
			m_planes[planeIndex][i] = (Real)poly[planeIndex].n[i];
		}
		m_planes[planeIndex][3] = (Real)poly[planeIndex].d;
		m_planes[planeIndex].normalize();
	}

	// Build the tree.
	Node* node = m_root;
	for (uint32_t planeIndex = 0; planeIndex < polySize; ++planeIndex)
	{
		ApexCSG::Region outside;
		outside.side = 0;
		Node* child0 = m_memCache->m_nodePool.borrow();
		child0->setLeafData(outside);
		Node* child1 = m_memCache->m_nodePool.borrow();	// No need to set inside leaf data, that is the default
		Surface surface;
		surface.planeIndex = planeIndex;
		surface.triangleIndexStart = 0;
		surface.triangleIndexStop = 0;
		surface.totalTriangleArea = 0.0f;
		node->setBranchData(surface);
		node->setChild(0, child0);
		node->setChild(1, child1);
		node = child1;
	}

	// See if the planes bound a non-empty set
	RegionShape regionShape(m_planes.begin());
	regionShape.set_leaf(node);
	regionShape.calculate();
	if (!regionShape.is_nonempty())
	{
		clear();
		Region leafData;
		leafData.side = 0;
		m_root = m_memCache->m_nodePool.borrow();
		m_root->setLeafData(leafData);	// Planes define a null intersection.  The result is the empty set.
		return true;
	}

	// Currently there is no internal transform, BSP space = poly space
	// With internalTransform is valid, then the user is asking that:
	// (BSP space) = (params.internalTransform)(poly space)
	// so we simply transform by params.internalTransform.
	if (!isZero(internalTransform))
	{
		m_internalTransform = internalTransform;
		const Mat4Real internalTransformCSG = CSGFromPx(m_internalTransform);
		m_internalTransformInverse = internalTransformCSG.inverse34();
		const Real meshSize = m_meshSize;	// Save off mesh size.  This gets garbled by scaled transforms.
		transform(internalTransformCSG, false);
		const Real maxScale = PxMax(internalTransformCSG[0].lengthSquared(), PxMax(internalTransformCSG[1].lengthSquared(), internalTransformCSG[2].lengthSquared()));
		m_meshSize = meshSize*maxScale;
	}

	if (triangleCount > 0)
	{
		// Collect mesh triangles and find mesh bounds
		m_mesh.resize(triangleCount);
		m_frames.resize(triangleCount);
		m_meshBounds.setEmpty();
		for (uint32_t i = 0; i < m_mesh.size(); ++i)
		{
			const ExplicitRenderTriangle& inTri = mesh[i];
			VertexData vertexData[3];
			m_mesh[i].fromExplicitRenderTriangle(vertexData, inTri);
			m_frames[i].setFromTriangle(m_mesh[i], vertexData);
			m_meshBounds.include(inTri.vertices[0].position);
			m_meshBounds.include(inTri.vertices[1].position);
			m_meshBounds.include(inTri.vertices[2].position);
		}

		// Size scales
		const Dir extents(m_meshBounds.getExtents());
		m_meshSize = PxMax(extents[0], PxMax(extents[1], extents[2]));

		// Scale to unit size and zero offset for BSP building
		const Vec4Real recipScale((extents[0] > m_tolerarnces.linear ? extents[0] : (Real)1), (extents[1] > m_tolerarnces.linear ? extents[1] : (Real)1), (extents[2] > m_tolerarnces.linear ? extents[2] : (Real)1), (Real)1);
		const Vec4Real scale((Real)1/recipScale[0], (Real)1/recipScale[1], (Real)1/recipScale[2], (Real)1);
		const Pos center = m_meshBounds.getCenter();
		for (uint32_t i = 0; i < m_mesh.size(); ++i)
		{
			Triangle& tri = m_mesh[i];
			for (uint32_t j = 0; j < 3; ++j)
			{
				Pos& pos = tri.vertices[j];
				pos = (pos - center) * scale;
			}
			tri.calculateQuantities();
		}

		m_incidentalMesh = true;
	}

	return true;
}

bool
BSP::combine(const IApexBSP& ibsp)
{
	const BSP& bsp = *(const BSP*)&ibsp;

	if (m_combined || bsp.m_combined)
	{
		GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eINVALID_OPERATION,
		        "BSP::combine: can only combine two uncombined BSPs.  Use op() to merge a combined BSP.", __FILE__, __LINE__);
		return false;
	}

	if (!transformsEqual(m_internalTransform, bsp.m_internalTransform, 0.001f))
	{
		GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING,
			"BSP::combine: Nontrivial BSPs being combined have different internal transformations.  Behavior is undefined.", __FILE__, __LINE__);
	}

	// Add in other bsp's triangles.
	const uint32_t thisTriangleCount = m_mesh.size();
	const uint32_t totalTriangleCount = thisTriangleCount + bsp.m_mesh.size();
	m_mesh.resize(totalTriangleCount);
	m_frames.resize(totalTriangleCount);
	for (uint32_t i = thisTriangleCount; i < totalTriangleCount; ++i)
	{
		m_mesh[i] = bsp.m_mesh[i - thisTriangleCount];
		m_frames[i] = bsp.m_frames[i - thisTriangleCount];
	}

	// Add in other bsp's planes.
	const uint32_t thisPlaneCount = m_planes.size();
	const uint32_t totalPlaneCount = thisPlaneCount + bsp.m_planes.size();
	m_planes.resize(totalPlaneCount);
	for (uint32_t i = thisPlaneCount; i < totalPlaneCount; ++i)
	{
		m_planes[i] = bsp.m_planes[i - thisPlaneCount];
	}

	combineTrees(m_root, bsp.m_root, thisTriangleCount, thisPlaneCount);

	m_combiningMeshSize = bsp.m_meshSize;
	m_combiningIncidentalMesh = bsp.m_incidentalMesh;

	m_meshBounds.include(bsp.m_meshBounds);

	m_combined = true;

	clean();

	return true;
}

bool
BSP::op(const IApexBSP& icombinedBSP, Operation::Enum operation)
{
	const BSP& combinedBSP = *(const BSP*)&icombinedBSP;

	if (!combinedBSP.m_combined)
	{
		GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eINVALID_OPERATION,
		        "BSP::op: can only perform an operation upon a combined BSP.  Use combine() with another BSP.", __FILE__, __LINE__);
		return false;
	}

	if (operation == Operation::NOP)
	{
		GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eINVALID_OPERATION,
		        "BSP::op: NOP requested.  Mesh will remain combined.", __FILE__, __LINE__);
		return false;
	}

	copy(combinedBSP);	// No-ops if this = combinedBSP, so this is safe

	// Combine size tolerances - look at symmetry
	switch (operation >> 1)
	{
	case 1:	// From set A
	case 5:
		// Keep size scales
		break;
	case 2:	// From set B
	case 6:
		// Replace with other size tolerance
		m_meshSize = m_combiningMeshSize;
		break;
		// Symmetric cases
	case 0:	// Empty_Set or All_Space, set size scale to unitless value
		m_meshSize = 1;
		break;
	case 3:	// Symmetric combinations of sets, use the min scale
	case 4:
	case 7:
		m_meshSize = PxMin(m_meshSize, m_combiningMeshSize);
		break;
	}

	mergeLeaves(BoolOp(operation), m_root);

	m_incidentalMesh = m_incidentalMesh || m_combiningIncidentalMesh;

	m_combined = false;

	return true;
}

bool
BSP::complement()
{
	if (m_combined)
	{
		GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eINVALID_OPERATION,
		        "BSP::complement: can only complement an uncombined BSP.  Use op() to merge a combined BSP.", __FILE__, __LINE__);
		return false;
	}

	complementLeaves(m_root);

	return true;
}

BSPType::Enum
BSP::getType() const
{
	if (m_combined)
	{
		return BSPType::Combined;
	}

	if (m_root->getType() != Node::Leaf)
	{
		return BSPType::Nontrivial;
	}

	return m_root->getLeafData()->side == 1 ? BSPType::All_Space : BSPType::Empty_Set;
}

bool
BSP::getSurfaceAreaAndVolume(float& area, float& volume, bool inside, Operation::Enum operation) const
{
	if (m_combined && operation == Operation::NOP)
	{
		GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eINVALID_OPERATION,
		        "BSP::getSurfaceAreaAndVolume: an operation must be provided for combined BSPs.", __FILE__, __LINE__);
		return false;
	}

	if (!m_combined && operation != Operation::NOP)
	{
		GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING, "BSP::getSurfaceAreaAndVolume: warning, operation ignored for non-combined BSPs." , __FILE__, __LINE__);
	}

	Real realArea = (Real)0;
	Real realVolume = (Real)0;
	if (addLeafAreasAndVolumes(realArea, realVolume, m_root, inside, BoolOp(operation)))
	{
		area = (float)realArea;
		volume =  (float)realVolume;
		return true;
	}

	area = PX_MAX_F32;
	volume = PX_MAX_F32;
	return false;
}

bool
BSP::pointInside(const PxVec3& point, Operation::Enum operation) const
{
	if (m_combined && operation == Operation::NOP)
	{
		GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eINVALID_OPERATION,
			"BSP::pointInside: an operation must be provided for combined BSPs.", __FILE__, __LINE__);
		return 0;
	}

	if (!m_combined && operation != Operation::NOP)
	{
		GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_WARNING, "BSP::pointInside: warning, operation ignored for non-combined BSPs." , __FILE__, __LINE__);
	}

	Node* node = m_root;

	const PxVec3 BSPPoint = m_internalTransform.transform(point);

	while (node->getType() == Node::Branch)
	{
		const Surface* surface = node->getBranchData();
		node = node->getChild((uint32_t)((m_planes[surface->planeIndex].distance(BSPPoint)) <= 0.0f));
	}

	const Region* region = node->getLeafData();

	uint32_t side = region->side;
	if (m_combined)
	{
		side = BoolOp(operation)(side & 1, (side >> 1) & 1);
	}

	return side != 0;
}

bool
BSP::toMesh(physx::Array<ExplicitRenderTriangle>& mesh) const
{
	if (m_combined)
	{
		GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eINVALID_OPERATION,
		        "BSP::toMesh: can only generate a mesh from an uncombined BSP.  Use op() to merge a combined BSP.", __FILE__, __LINE__);
		return false;
	}

	// Clip triangles collected from leaves
	physx::Array<Triangle> clippedMesh;
	physx::Array<ClippedTriangleInfo> triangleInfo;
	clipMeshToLeaves(clippedMesh, triangleInfo, m_root, m_tolerarnces.clip);

	// Clean
	if (m_tolerarnces.cleaning > 0 && !m_incidentalMesh)
	{
		cleanMesh(mesh, clippedMesh, triangleInfo, m_planes, m_mesh, m_frames, (Real)m_tolerarnces.cleaning * m_meshSize, m_internalTransformInverse);
	}
	else
	{
		// Copy to render format
		mesh.resize(clippedMesh.size());
		for (uint32_t i = 0; i < clippedMesh.size(); ++i)
		{
			clippedMesh[i].transform(m_internalTransformInverse);
			VertexData vertexData[3];
			for (int v = 0; v < 3; ++v)
			{
				m_frames[triangleInfo[i].originalTriangleIndex].interpolateVertexData(vertexData[v], clippedMesh[i].vertices[v]);
				if (!triangleInfo[i].ccw)
				{
					vertexData[v].normal *= -1.0;
				}
			}
			clippedMesh[i].toExplicitRenderTriangle(mesh[i], vertexData);
		}
	}

	return true;
}

void
BSP::copy(const IApexBSP& ibsp, const physx::PxMat44& pxTM, const physx::PxMat44& internalTransform)
{
	const BSP& bsp = *(const BSP*)&ibsp;

	if (this != &bsp)
	{
		// Copy other bsp
		clear();

		if (bsp.m_root)
		{
			m_root = m_memCache->m_nodePool.borrow();
			clone(m_root, bsp.m_root);
		}
		m_tolerarnces = bsp.m_tolerarnces;
		m_mesh = bsp.m_mesh;
		m_frames = bsp.m_frames;
		m_planes = bsp.m_planes;
		m_meshSize = bsp.m_meshSize;
		m_incidentalMesh = bsp.m_incidentalMesh;
		m_internalTransform = bsp.m_internalTransform;
		m_internalTransformInverse = bsp.m_internalTransformInverse;
		m_combined = bsp.m_combined;
		m_combiningMeshSize = bsp.m_combiningMeshSize;
		m_combiningIncidentalMesh = bsp.m_combiningIncidentalMesh;
	}

	// Take new internal transform if it is valid
	if (!isZero(internalTransform))
	{
		m_internalTransform = internalTransform;
	}

	// Do not calculate new m_internalTransformInverse yet.  We need it to transform out of the BSP space.

	// Translate physx::PxMat44 to Mat4Real
	// We actually need to apply this transform *before* the internal transform, so we apply: m_internalTransform*pxTM*m_internalTransformInverse
	physx::PxMat44 pxTMITM = m_internalTransform*pxTM;
	Mat4Real tmITM;
	tmITM.setCol(0, Dir((physx::PxF32*)&pxTMITM[0]));
	tmITM.setCol(1, Dir((physx::PxF32*)&pxTMITM[1]));
	tmITM.setCol(2, Dir((physx::PxF32*)&pxTMITM[2]));
	tmITM.setCol(3, Pos((physx::PxF32*)&pxTMITM[3]));

	const Mat4Real netTM = tmITM*m_internalTransformInverse;

	// Do not transform if netTM is the identity
	bool isIdentity = true;
	for (uint32_t i = 0; i < 4 && isIdentity; ++i)
	{
		for (uint32_t j = 0; j < 4 && isIdentity; ++j)
		{
			isIdentity = physx::PxAbs(netTM[i][j] - (Real)(i == j)) < (Real)(10.0f*PX_EPS_F32);
		}
	}
	if (!isIdentity)
	{
		transform(netTM);
	}

	// Now calculate m_internalTransformInverse.
	m_internalTransformInverse = CSGFromPx(m_internalTransform).inverse34();
}

IApexBSP*
BSP::decomposeIntoIslands() const
{
	// Must be normal BSP
	if (m_combined)
	{
		return NULL;
	}

	// First enumerate all inside leaves
	uint32_t insideLeafCount = 0;
	indexInsideLeaves(insideLeafCount, m_root);
	if (insideLeafCount == 0)
	{
		return NULL;
	}

	// Find all leaf neighbors
	physx::Array<IntPair> neighbors;
	findInsideLeafNeighbors(neighbors, m_root);

	// Find leaf neighbor islands
	physx::Array< physx::Array<uint32_t> > islands;
	findIslands(islands, neighbors, insideLeafCount);

	// Return this if there is only one island
	if (islands.size() == 1)
	{
		return const_cast<BSP*>(this);
	}

	// Otherwise we make a BSP list
	physx::Array<Node*> insideLeaves;
	insideLeaves.reserve(insideLeafCount);
	BSPLink* listRoot = PX_NEW(BSPLink)();
	for (uint32_t islandNum = islands.size(); islandNum--;)
	{
		// Create new island
		BSP* islandBSP = static_cast<BSP*>(createBSP(m_memCache));
		if (islandBSP != NULL)
		{
			// Copy island BSP from this and add to list
			islandBSP->copy(*this);
			listRoot->setAdj(1, islandBSP);
			// Create a list of the inside leaf pointers
			insideLeaves.clear();
			listInsideLeaves(insideLeaves, islandBSP->m_root);
			// Set all the leaves' sides to 0
			for (uint32_t leafNum = 0; leafNum < insideLeaves.size(); ++leafNum)
			{
				insideLeaves[leafNum]->getLeafData()->side = 0;
			}
			// Set island leaves' sides to 1
			const physx::Array<uint32_t>& island = islands[islandNum];
			for (uint32_t islandLeafNum = 0; islandLeafNum < island.size(); ++islandLeafNum)
			{
				insideLeaves[island[islandLeafNum]]->getLeafData()->side = 1;
			}
			// Now merge leaves to consolidate new 0-0 siblings
			islandBSP->mergeLeaves(BoolOp(Operation::Set_A), islandBSP->m_root);
		}
	}

	// Return list head
	if (!listRoot->isSolitary())
	{
		delete this;
		return static_cast<BSP*>(listRoot->getAdj(1));
	}

	delete listRoot;
	return const_cast<BSP*>(this);
}

void
BSP::replaceInteriorSubmeshes(uint32_t frameCount, uint32_t* frameIndices, uint32_t submeshIndex)
{
	// Replace render mesh submesh indices
	for (uint32_t triangleIndex = 0; triangleIndex < m_mesh.size(); ++triangleIndex)
	{
		Triangle& triangle = m_mesh[triangleIndex];
		for (uint32_t frameNum = 0; frameNum < frameCount; ++frameNum)
		{
			if (triangle.extraDataIndex == frameIndices[frameNum])
			{
				triangle.submeshIndex = (int32_t)submeshIndex;
			}
		}
	}
}

void
BSP::deleteTriangles()
{
	m_mesh.reset();
	m_frames.reset();
	for (Node::It it(m_root); it.valid(); it.inc())
	{
		Node* node = it.node();
		if (node->getType() == BSP::Node::Branch)
		{
			Surface* surface = node->getBranchData();
			surface->triangleIndexStart = 0;
			surface->triangleIndexStop = 0;
			surface->totalTriangleArea = 0.0f;
		}
	}
}

void
BSP::serialize(physx::PxFileBuf& stream) const
{
	stream << (uint32_t)Version::Current;

	// Tree
	serializeNode(m_root, stream);

	// Internal mesh representation
	nvidia::serialize(stream, m_mesh);
	nvidia::serialize(stream, m_frames);
	stream << m_meshSize;
	stream << m_incidentalMesh;
	stream << m_meshBounds;
	stream << m_internalTransform;
	stream << m_internalTransformInverse;

	// Unique splitting planes
	nvidia::serialize(stream, m_planes);

	// Combination data
	stream << m_combined;
	stream << m_combiningMeshSize;
	stream << m_combiningIncidentalMesh;
}

void
BSP::deserialize(physx::PxFileBuf& stream)
{
	clear();

	uint32_t version;
	stream >> version;

	// Tree
	m_root = deserializeNode(version, stream);

	if (version < Version::RevisedMeshTolerances)
	{
		stream.readDouble();	// Swallow old m_linearTol
		stream.readDouble();	// Swallow old m_angularTol
	}

	// Internal mesh representation
	if (version >= Version::SerializingTriangleFrames)
	{
		apex::deserialize(stream, version, m_mesh);
		nvidia::deserialize(stream, version, m_frames);
	}
	else
	{
		const uint32_t triangleCount = stream.readDword();
		m_mesh.resize(triangleCount);
		m_frames.resize(triangleCount);
		for (uint32_t triN = 0; triN < triangleCount; ++triN)
		{
			Triangle& tri = m_mesh[triN];
			if (version < Version::UsingOnlyPositionDataInVertex)
			{
				VertexData vertexData[3];
				for (uint32_t v = 0; v < 3; ++v)
				{
					stream >> tri.vertices[v];
					stream >> vertexData[v].normal;
					stream >> vertexData[v].binormal;
					stream >> vertexData[v].binormal;
					for (uint32_t uvN = 0; uvN < VertexFormat::MAX_UV_COUNT; ++uvN)
					{
						stream >> vertexData[v].uv[uvN];
					}
					stream >> vertexData[v].color;
				}
				m_frames[triN].setFromTriangle(tri, vertexData);
			}
			else
			{
				for (uint32_t i = 0; i < 3; ++i)
				{
					nvidia::deserialize(stream, version, tri);
				}
			}
			stream >> tri.submeshIndex;
			stream >> tri.smoothingMask;
			stream >> tri.extraDataIndex;
			stream >> tri.normal;
			stream >> tri.area;
		}
	}
	stream >> m_meshSize;

	if (version >= Version::IncidentalMeshDistinction)
	{
		stream >> m_incidentalMesh;
	}

	if (version >= Version::SerializingMeshBounds)
	{
		stream >> m_meshBounds;
	}
	else
	{
		m_meshBounds.setEmpty();
		for (uint32_t triangleN = 0; triangleN < m_mesh.size(); ++triangleN)
		{
			Triangle& tri = m_mesh[triangleN];
			for (uint32_t v = 0; v < 3; ++v)
			{
				Pos& vertex = tri.vertices[v];
				m_meshBounds.include(physx::PxVec3((float)vertex[0], (float)vertex[1], (float)vertex[2]));
			}
		}
	}

	if (version < Version::RevisedMeshTolerances)
	{
		stream.readDouble();	// Swallow old m_distanceTol
	}

	if (version >= Version::AddedInternalTransform)
	{
		stream >> m_internalTransform;
		stream >> m_internalTransformInverse;
	}
	else
	{
		m_internalTransform = physx::PxMat44(physx::PxIdentity);
		m_internalTransformInverse.set((Real)1);
	}

	// Unique splitting planes
	apex::deserialize(stream, version, m_planes);

	// Combination data
	stream >> m_combined;
	stream >> m_combiningMeshSize;
	if (version >= Version::IncidentalMeshDistinction)
	{
		stream >> m_combiningIncidentalMesh;
	}

	if (m_root == NULL)
	{
		// Set to trivial tree
		clear();
		m_root = m_memCache->m_nodePool.borrow();
	}
}

void BSP::visualize(RenderDebugInterface& debugRender, uint32_t flags, uint32_t index) const
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(debugRender);
	PX_UNUSED(flags);
	PX_UNUSED(index);
#else
	const Node* node = m_root;
	if (flags & BSPVisualizationFlags::SingleRegion)
	{
		uint32_t count = 0;
		for (Node::It it(m_root); it.valid(); it.inc())
		{
			Node* current = it.node();
			if (current->getType() == BSP::Node::Leaf)
			{
				if (index == count++)
				{
					node = current;
					break;
				}
			}
		}
	}

	if (node != NULL)
	{
		visualizeNode(debugRender, flags, node);
	}
#endif
}

void
BSP::release()
{
	clear();
	delete this;
}

void
BSP::clear()
{
	if (m_root != NULL)
	{
		releaseNode(m_root);
		m_root = NULL;
	}
	m_incidentalMesh = false;
	m_combiningIncidentalMesh = false;
	m_combiningMeshSize = (Real)1;
	m_combined = false;
	m_mesh.reset();
	m_frames.reset();
	m_planes.reset();
	removeBSPLink();
}

void
BSP::clipMeshToLeaf(Real& area, Real& volume, physx::Array<Triangle>* clippedMesh, physx::Array<ClippedTriangleInfo>* triangleInfo, const Node* leaf, float clipTolerance) const
{
	PX_ASSERT(leaf->getType() == BSP::Node::Leaf);

	area = (Real)0;
	volume = (Real)0;

	const Pos center(&m_meshBounds.getCenter()[0]);

	// Collect triangles on each surface and clip to other faces
	for (SurfaceIt it(leaf); it.valid(); it.inc())
	{
		for (uint32_t i = it.surface()->triangleIndexStart; i < it.surface()->triangleIndexStop; ++i)
		{
			const uint32_t oldClippedMeshSize = clippedMesh != NULL ? clippedMesh->size() : 0;
			Real clippedTriangleArea, clippedPyramidVolume;
			clipTriangleToLeaf(clippedMesh, clippedTriangleArea, clippedPyramidVolume, center, m_mesh[i], leaf, it.side(), m_memCache->m_linkedVertexPool,
							   clipTolerance * m_meshSize, m_planes, it.surface()->planeIndex);
			area += clippedTriangleArea;
			volume += clippedPyramidVolume;
			if (triangleInfo != NULL && clippedMesh != NULL)
			{
				// Fill triangleInfo corresponding to new clipped triangles
				const uint32_t newClippedMeshSize = clippedMesh->size();
				for (uint32_t j = oldClippedMeshSize; j < newClippedMeshSize; ++j)
				{
					ClippedTriangleInfo& info = triangleInfo->insert();
					info.planeIndex = it.surface()->planeIndex;
					info.originalTriangleIndex = i;
					info.clippedTriangleIndex = j;
					info.ccw = it.side();
				}
			}
		}
	}

	if (m_incidentalMesh)
	{
		for (uint32_t i = 0; i < m_mesh.size(); ++i)
		{
			const uint32_t oldClippedMeshSize = clippedMesh != NULL ? clippedMesh->size() : 0;
			Real clippedTriangleArea, clippedPyramidVolume;
			clipTriangleToLeaf(clippedMesh, clippedTriangleArea, clippedPyramidVolume, center, m_mesh[i], leaf, 1, m_memCache->m_linkedVertexPool, clipTolerance * m_meshSize, m_planes);
			if (triangleInfo != NULL && clippedMesh != NULL)
			{
				// Fill triangleInfo corresponding to new clipped triangles
				const uint32_t newClippedMeshSize = clippedMesh->size();
				for (uint32_t j = oldClippedMeshSize; j < newClippedMeshSize; ++j)
				{
					ClippedTriangleInfo& info = triangleInfo->insert();
					info.planeIndex = 0xFFFFFFFF;
					info.originalTriangleIndex = i;
					info.clippedTriangleIndex = j;
					info.ccw = 1;
				}
			}
		}
	}
}

void
BSP::transform(const Mat4Real& tm, bool transformFrames)
{
	// Build cofactor matrix for transformation of normals
	const Mat4Real cofTM = tm.cof34();
	const Mat4Real invTransposeTM = cofTM/cofTM[3][3];

	// Transform mesh
	for (uint32_t i = 0; i < m_mesh.size(); ++i)
	{
		for (int v = 0; v < 3; ++v)
		{
			m_mesh[i].vertices[v] = tm * m_mesh[i].vertices[v];
		}
		m_mesh[i].calculateQuantities();
		if (transformFrames)
		{
			m_frames[i].transform(m_frames[i], tm, invTransposeTM);
		}
	}

	// Transform planes
	for (uint32_t i = 0; i < m_planes.size(); ++i)
	{
		m_planes[i] = cofTM * m_planes[i];	// Don't normalize yet - surface areas will be calculated from plane normal lengths in "Transform tree" below
	}

	// Transform tree
	for (Node::It it(m_root); it.valid(); it.inc())
	{
		Node* node = it.node();
		if (node->getType() == BSP::Node::Branch)
		{
			// Transform surface quantities
			node->getBranchData()->totalTriangleArea *= (float)physx::PxSqrt(m_planes[node->getBranchData()->planeIndex].normal().lengthSquared());
		}
	}

	// Now normalize planes
	for (uint32_t i = 0; i < m_planes.size(); ++i)
	{
		m_planes[i].normalize();
	}

	// Adjust sizes
	const Real scale = physx::PxPow((float) tm.det3(), (float)0.33333333333333333);
	m_meshSize *= scale;
	m_combiningMeshSize *= scale;
}

void
BSP::clean()
{
	/*
		1) Mark planes and triangles that are used in the tree
		2) Remove those that aren't, creating index maps, bounds, and size
		3) Walk tree again and re-index
	*/

	physx::Array<uint32_t> planeMap(m_planes.size(), 0);
	physx::Array<uint32_t> triangleMap(m_mesh.size()+1, 0);

	//	1) Mark planes and triangles that are used in the tree
	for (Node::It it(m_root); it.valid(); it.inc())
	{
		Node* node = it.node();
		if (node->getType() == BSP::Node::Branch)
		{
			const Surface* surface = node->getBranchData();
			planeMap[surface->planeIndex] = 1;
			for (uint32_t triangleIndex = surface->triangleIndexStart; triangleIndex < surface->triangleIndexStop; ++triangleIndex)
			{
				triangleMap[triangleIndex] = 1;
			}
		}
	}

	if (m_incidentalMesh || (m_combined && m_combiningIncidentalMesh))
	{
		// All triangles used
		for (uint32_t triangleIndex = 0; triangleIndex < m_mesh.size(); ++triangleIndex)
		{
			triangleMap[triangleIndex] = 1;
		}
	}

	//	2) Remove those that aren't, creating index maps and bounds
	m_meshBounds.setEmpty();

	uint32_t newPlaneIndex = 0;
	for (uint32_t oldPlaneIndex = 0; oldPlaneIndex < planeMap.size(); ++oldPlaneIndex)
	{
		const bool planeUsed = planeMap[oldPlaneIndex] != 0;
		planeMap[oldPlaneIndex] = newPlaneIndex;
		if (planeUsed)
		{
			m_planes[newPlaneIndex++] = m_planes[oldPlaneIndex];
		}
	}
	m_planes.resize(newPlaneIndex);

	uint32_t newTriangleIndex = 0;
	for (uint32_t oldTriangleIndex = 0; oldTriangleIndex < triangleMap.size(); ++oldTriangleIndex)
	{
		const bool triangleUsed = triangleMap[oldTriangleIndex] != 0;
		triangleMap[oldTriangleIndex] = newTriangleIndex;
		if (triangleUsed)
		{
			Triangle& triangle = m_mesh[newTriangleIndex];
			triangle = m_mesh[oldTriangleIndex];
			for (int v = 0; v < 3; ++v)
			{
				const Pos& vertex = triangle.vertices[v];
				m_meshBounds.include(physx::PxVec3((float)vertex[0], (float)vertex[1], (float)vertex[3]));
			}
			m_frames[newTriangleIndex] = m_frames[oldTriangleIndex];
			++newTriangleIndex;
		}
	}
	m_mesh.resize(newTriangleIndex);
	m_frames.resize(newTriangleIndex);

	m_meshSize = (Real)m_meshBounds.getExtents().maxElement();

	//	3) Walk tree again and re-index
	for (Node::It it(m_root); it.valid(); it.inc())
	{
		Node* node = it.node();
		if (node->getType() == BSP::Node::Branch)
		{
			Surface* surface = const_cast<Surface*>(node->getBranchData());
			surface->planeIndex = planeMap[surface->planeIndex];
			const uint32_t surfaceTriangleCount = surface->triangleIndexStop - surface->triangleIndexStart;
			surface->triangleIndexStart = triangleMap[surface->triangleIndexStart];
			surface->triangleIndexStop = surface->triangleIndexStart + surfaceTriangleCount;	// Do it this way since the last triangleIndexStop is unmapped
		}
	}
}

void
BSP::performDiagnostics() const
{
	debugInfo("BSP diagnostics starting.");

	char msg[10240];

	debugInfo("Checking for holes...");

	// This is the "raw result" from toMesh().  It's in our internal (high-precision) format, not cleaned, etc.:
	physx::Array<Triangle> clippedMesh;
	physx::Array<ClippedTriangleInfo> triangleInfo;
	clipMeshToLeaves(clippedMesh, triangleInfo, m_root, m_tolerarnces.clip);

	for (uint32_t triangleIndex = 0; triangleIndex < m_mesh.size(); ++triangleIndex)
	{
		// Make sure triangle is in a branch somewhere
		physx::Array<BSP::Node*> foundInNodes;
		for (Node::It it(m_root); it.valid(); it.inc())
		{
			BSP::Node* node = static_cast<BSP::Node*>(it.node());
			if (node->getType() == BSP::Node::Branch)
			{
				const Surface* surface = node->getBranchData();
				if (triangleIndex >= surface->triangleIndexStart && triangleIndex < surface->triangleIndexStop)
				{
					foundInNodes.pushBack(node);
				}
			}
		}
		if (foundInNodes.empty())
		{
			sprintf(msg, "Triangle %d not found in any branch.", triangleIndex);
			debugWarn(msg);
		}

		// Make sure the triangle comes back with no holes
		const Triangle& triangle = m_mesh[triangleIndex];
		if (triangle.area > (Real)0)
		{
			Real area = (Real)0;
			for (uint32_t clippedTriangleIndex = 0; clippedTriangleIndex < clippedMesh.size(); ++clippedTriangleIndex)
			{
				ClippedTriangleInfo& info = triangleInfo[clippedTriangleIndex];
				if (info.originalTriangleIndex != triangleIndex)
				{
					continue;
				}
				Triangle& clippedMeshTriangle = clippedMesh[clippedTriangleIndex];
				area += clippedMeshTriangle.area;
			}

			const Real areaError = area/triangle.area - (Real)1;
			if (physx::PxAbs(areaError) > (Real)0.000001)
			{
				sprintf(msg, "Triangle %d is reconstructed with a different area: error = %7.4g%%.", triangleIndex, (Real)100*areaError);
				debugWarn(msg);

				sprintf(msg, "  Triangle %d is found in %d node(s):", triangleIndex, foundInNodes.size());
				debugInfo(msg);
				Real totalClippedArea = (Real)0;
				for (uint32_t nodeN = 0; nodeN < foundInNodes.size(); ++nodeN)
				{
					sprintf(msg, "  Node #%d:", nodeN);
					debugInfo(msg);
					Real nodeArea = (Real)0;
					for (Node::It it(foundInNodes[nodeN]); it.valid(); it.inc())
					{
						Node* subTreeNode = it.node();
						if (subTreeNode->getType() == BSP::Node::Leaf)
						{
							const uint32_t planeSide = subTreeNode->getIndex();
							if (subTreeNode->getLeafData()->side == 0)
							{
								continue;
							}
							Real clipArea, clipVolume;
							const Pos origin(&m_meshBounds.getCenter()[0]);
							clipTriangleToLeaf(NULL, clipArea, clipVolume, origin, triangle, subTreeNode, planeSide, m_memCache->m_linkedVertexPool, m_tolerarnces.clip*m_meshSize, m_planes, foundInNodes[nodeN]->getBranchData()->planeIndex);
							nodeArea += clipArea;
//							sprintf(msg, "    Subtree leaf area = %15.7f.", clipArea);
//							debugInfo(msg);
						}
					}
					totalClippedArea += nodeArea;
					sprintf(msg, "    Total node area = %15.7g.", nodeArea);
					debugInfo(msg);
				}
				sprintf(msg, "  Total clipped area = %15.7g.", totalClippedArea);
				debugInfo(msg);

				sprintf(msg, "  Attempting brute-force decoposition.");
				Real totalClippedArea2ndAttempt[2] = {(Real)0, (Real)0};
				uint32_t leafCount[2] = {0, 0};
				for (Node::It it(m_root); it.valid(); it.inc())
				{
					Node* n = it.node();
					if (n->getType() == BSP::Node::Leaf)
					{
						const uint32_t planeSide = n->getIndex();
						const uint32_t side = n->getLeafData()->side & 1;
						Real clipArea, clipVolume;
						const Pos origin(&m_meshBounds.getCenter()[0]);
						clipTriangleToLeaf(NULL, clipArea, clipVolume, origin, triangle, n, planeSide, m_memCache->m_linkedVertexPool, m_tolerarnces.clip*m_meshSize, m_planes);
						totalClippedArea2ndAttempt[side] += clipArea;
						if (clipArea != 0)
						{
							++leafCount[side];
							sprintf(msg, "    Non-zero area found in side(%d) leaf.  Parent planes:", side);
							const BSP::Node* nn = n;
							while((nn = (const BSP::Node*)nn->getParent()) != NULL)
							{
								char num[32];
								sprintf(num, " %d,", nn->getBranchData()->planeIndex);
								strcat(msg, num);
							}
							debugInfo(msg);
						}
					}
				}
				sprintf(msg, "  Total outside area from %d leaves = %15.7g.", leafCount[0], totalClippedArea2ndAttempt[0]);
				sprintf(msg, "  Total inside area from %d leaves = %15.7g.", leafCount[1], totalClippedArea2ndAttempt[1]);
				debugInfo(msg);
			}
		}
		else
		{
			sprintf(msg, "Triangle %d has non-positive area.", triangleIndex);
			debugWarn(msg);
		}
	}

	debugInfo("BSP diagnostics finished.");
}

PX_INLINE uint32_t
BSP::removeRedundantSurfacesFromStack(physx::Array<Surface>& surfaceStack, uint32_t stackReadStart, uint32_t stackReadStop, Node* leaf)
{
	// Remove surfaces that don't have triangles intersecting this region
	const Pos center(&m_meshBounds.getCenter()[0]);

	for (uint32_t i = stackReadStop; i-- > stackReadStart;)
	{
		Surface* surface = surfaceStack.begin() + i;
		bool surfaceIntersectsThisRegion = false;
		for (uint32_t j = surface->triangleIndexStart; j < surface->triangleIndexStop; ++j)
		{
			Real clippedTriangleArea, clippedPyramidVolume;
			clipTriangleToLeaf(NULL, clippedTriangleArea, clippedPyramidVolume, center, m_mesh[j], leaf, 1, m_memCache->m_linkedVertexPool, m_tolerarnces.base, m_planes);
			if (0 < clippedTriangleArea)
			{
				surfaceIntersectsThisRegion = true;
				break;
			}
		}
		if (!surfaceIntersectsThisRegion)
		{
			surfaceStack[i] = surfaceStack[--stackReadStop];
		}
	}

	return stackReadStop;
}

PX_INLINE void
BSP::assignLeafSide(Node* leaf, QuantityProgressListener* quantityListener)
{
	const Pos center(&m_meshBounds.getCenter()[0]);

	// See if this leaf is inside or outside
	Real sumSignedArea = (Real)0;
	for (SurfaceIt it(leaf); it.valid(); it.inc())
	{
		const Real sign = it.side() ? (Real)1 : -(Real)1;
		for (uint32_t j = it.surface()->triangleIndexStart; j < it.surface()->triangleIndexStop; ++j)
		{
			Real clippedTriangleArea, clippedPyramidVolume;
			clipTriangleToLeaf(NULL, clippedTriangleArea, clippedPyramidVolume, center, m_mesh[j], leaf, it.side(), m_memCache->m_linkedVertexPool, m_tolerarnces.base, m_planes, it.surface()->planeIndex);
			sumSignedArea += sign * clippedTriangleArea;
		}
	}

	if (sumSignedArea != (Real)0)
	{
		leaf->getLeafData()->side = sumSignedArea > 0 ? 1u : 0u;
		quantityListener->add((Real)0.5*physx::PxAbs(sumSignedArea));
	}
}

PX_INLINE void
BSP::createBranchSurfaceAndSplitStack(uint32_t childReadStart[2], uint32_t childReadStop[2], Node* node, physx::Array<Surface>& surfaceStack, uint32_t stackReadStart,
									  uint32_t stackReadStop, const BuildConstants& buildConstants)
{
	const uint32_t surfaceListSize = stackReadStop - stackReadStart;
	Surface* surfaceList = surfaceStack.begin() + stackReadStart;

	if (m_memCache->m_surfaceFlags.size() < surfaceListSize)
	{
		m_memCache->m_surfaceFlags.resize(surfaceListSize);
		m_memCache->m_surfaceTestFlags.resize(surfaceListSize);
	}

	uint32_t branchSurfaceN = 0;	// Will be the winning surface - default to 1st surface

	Surface* branchSurface = surfaceList + branchSurfaceN;

	bool splittingCalculated = false;

	if (surfaceListSize > 1)
	{
		float maxLogArea = -PX_MAX_F32;
		float meanLogArea = 0.0f;
		float sigma2LogArea = 0.0f;
		if (buildConstants.m_params.logAreaSigmaThreshold > 0)
		{
			uint32_t positiveAreaCount = 0;
			for (uint32_t i = 0; i < surfaceListSize; ++i)
			{
				// Test surface
				Surface& testSurface = surfaceList[i];
				if (testSurface.totalTriangleArea <= 0.0f)
				{
					continue;
				}
				++positiveAreaCount;
				const float logArea = physx::PxLog(testSurface.totalTriangleArea);
				if (logArea > maxLogArea)
				{
					maxLogArea = logArea;
					branchSurfaceN = i;	// Candidate
				}
				meanLogArea += logArea;
			}
			if (positiveAreaCount > 1)
			{
				meanLogArea /= (float)positiveAreaCount;
				for (uint32_t i = 0; i < surfaceListSize; ++i)
				{
					// Test surface
					Surface& testSurface = surfaceList[i];
					if (testSurface.totalTriangleArea <= 0.0f)
					{
						continue;
					}
					const float logArea = physx::PxLog(testSurface.totalTriangleArea);
					sigma2LogArea += square<float>(logArea - meanLogArea);
				}
				sigma2LogArea /= (float)(positiveAreaCount-1);
			}

			// Possibly new branchSurfaceN
			branchSurface = surfaceList + branchSurfaceN;
		}
		if (maxLogArea > meanLogArea && square<float>(maxLogArea - meanLogArea) < square(buildConstants.m_params.logAreaSigmaThreshold)*sigma2LogArea)
		{
			// branchSurface chosen by max area does not have an area that is outside of one standard deviation from the mean surface area.  Use another method to determine branchSurface.

			// Pick buildConstants.m_testSetSize surfaces
			const uint32_t testSetSize = buildConstants.m_params.testSetSize > 0 ? PxMin(surfaceListSize, buildConstants.m_params.testSetSize) : surfaceListSize;

			// Low score wins
			float minScore = PX_MAX_F32;
			for (uint32_t i = 0; i < testSetSize; ++i)
			{
				// Test surface
				Surface* testSurface = surfaceList + i;
				int32_t counts[4] = {0, 0, 0, 0};	// on, above, below, split
				uint32_t triangleCount = 0;
				for (uint32_t j = 0; j < surfaceListSize; ++j)
				{
					uint8_t& flags = m_memCache->m_surfaceTestFlags[j];	// Whether this surface is above or below testSurface (or both)
					flags = 0;

					if (j == i)
					{
						continue;	// Don't score testSurface itself
					}

					// Surface to contribute to score
					Surface* surface = surfaceList + j;

					// Run through all triangles
					for (uint32_t k = surface->triangleIndexStart; k < surface->triangleIndexStop; ++k)
					{
						const Triangle& tri = m_mesh[k];
						uint8_t triFlags = 0;
						for (uint32_t v = 0; v < 3; ++v)
						{
							const int side = cmpPointToPlane(tri.vertices[v], m_planes[testSurface->planeIndex], m_tolerarnces.base);
							//							triFlags |= (side & 1) << ((1 - side) >> 1);	// 0 => 0, 1 => 1, -1 => 2
							triFlags |= (int)(side <= 0) << 1 | (int)(side >= 0);	// 0 => 3, 1 => 1, -1 => 2
						}
						++counts[triFlags];
						flags |= triFlags;
					}

					triangleCount += surface->triangleIndexStop - surface->triangleIndexStart;
				}

				// Compute score = (surface area)/(max area) + (split weight)*(# splits)/(# triangles) + (imbalance weight)*|(# above) - (# below)|/(# triangles)
				const float score = testSurface->totalTriangleArea * buildConstants.m_recipMaxArea +
					(buildConstants.m_params.splitWeight * counts[3] + buildConstants.m_params.imbalanceWeight * physx::PxAbs(counts[1] - counts[2])) / triangleCount;

				if (score < minScore)
				{
					// We have a winner
					branchSurfaceN = i;
					minScore = score;
					memcpy(m_memCache->m_surfaceFlags.begin(), m_memCache->m_surfaceTestFlags.begin(), surfaceListSize * sizeof(m_memCache->m_surfaceFlags[0]));
				}
			}

			// Possibly new branchSurfaceN
			branchSurface = surfaceList + branchSurfaceN;
			splittingCalculated = true;
		}
	}

	if (!splittingCalculated)
	{
		for (uint32_t i = 0; i < surfaceListSize; ++i)
		{
			uint8_t& flags = m_memCache->m_surfaceFlags[i];	// Whether this surface is above or below branchSurface (or both)
			flags = 0;

			if (i == branchSurfaceN)
			{
				continue;	// Don't score branchSurface itself
			}

			// Surface to contribute to score
			Surface& surface = surfaceList[i];

			// Run through all triangles
			for (uint32_t j = surface.triangleIndexStart; j < surface.triangleIndexStop; ++j)
			{
				const Triangle& tri = m_mesh[j];
				for (uint32_t v = 0; v < 3; ++v)
				{
					const int side = cmpPointToPlane(tri.vertices[v], m_planes[branchSurface->planeIndex], m_tolerarnces.base);
					//					flags |= (side & 1) << ((1 - side) >> 1);	// 0 => 0, 1 => 1, -1 => 2
					flags |= (int)(side <= 0) << 1 | (int)(side >= 0);	// 0 => 3, 1 => 1, -1 => 2
				}
			}
		}
	}

	// Run through the surface flags and create below/above arrays on the stack.
	// These arrays will be contiguous with child[0] surfaces first.
	childReadStart[0] = surfaceStack.size();
	childReadStop[0] = childReadStart[0];
	uint32_t targetStackSize = 2*(surfaceStack.size() + 2 * surfaceListSize);
	for (;;)
	{
		const uint32_t newTargetStackSize = targetStackSize&(targetStackSize-1);
		if (newTargetStackSize == 0)
		{
			break;
		}
		targetStackSize = newTargetStackSize;
	}

	surfaceStack.reserve(targetStackSize);
	for (uint32_t j = 0; j < surfaceListSize; ++j)
	{
		uint32_t surfaceJ = j + stackReadStart;
		if (j == branchSurfaceN)
		{
			continue;
		}	
		switch (m_memCache->m_surfaceFlags[j])
		{
		case 0:
			break;
		case 1:
			surfaceStack.insert();
			surfaceStack.back() = surfaceStack[childReadStop[0]];
			surfaceStack[childReadStop[0]++] = surfaceStack[surfaceJ];
			break;
		case 2:
			surfaceStack.pushBack(surfaceStack[surfaceJ]);
			break;
		case 3:
			surfaceStack.insert();
			surfaceStack.back() = surfaceStack[childReadStop[0]];
			surfaceStack[childReadStop[0]++] = surfaceStack[surfaceJ];
			surfaceStack.pushBack(surfaceStack[surfaceJ]);
			break;
		}
	}
	childReadStart[1] = childReadStop[0];
	childReadStop[1] = surfaceStack.size();

	// Set branch data to winning surface
	node->setBranchData(surfaceStack[branchSurfaceN + stackReadStart]);
}

/* Recursive functions */

// These can be implemented using a tree iterator or a simple node stack

void
BSP::releaseNode(Node* node)
{
	PX_ASSERT(node != NULL);

	Node* stop = node->getParent();
	do
	{
		Node* child0 = node->getChild(0);
		if (child0)
		{
			node = child0;
		}
		else
		{
			Node* child1 = node->getChild(1);
			if (child1)
			{
				node = child1;
			}
			else
			{
				Node* parent = node->getParent();
				node->detach();
				m_memCache->m_nodePool.replace(node);
				node = parent;
			}
		}
	} while (node != stop);
}

void
BSP::indexInsideLeaves(uint32_t& index, Node* root) const
{
	for (Node::It it(root); it.valid(); it.inc())
	{
		Node* node = it.node();
		if (node->getType() == BSP::Node::Leaf)
		{
			if (node->getLeafData()->side == 1)
			{
				node->getLeafData()->tempIndex1 = index++;
			}
		}
	}
}

void
BSP::listInsideLeaves(physx::Array<Node*>& insideLeaves, Node* root) const
{
	for (Node::It it(root); it.valid(); it.inc())
	{
		Node* node = it.node();
		if (node->getType() == BSP::Node::Leaf)
		{
			if (node->getLeafData()->side == 1)
			{
				insideLeaves.pushBack(node);
			}
		}
	}
}

void
BSP::complementLeaves(Node* root) const
{
	for (Node::It it(root); it.valid(); it.inc())
	{
		Node* node = it.node();
		if (node->getType() == BSP::Node::Leaf)
		{
			node->getLeafData()->side = node->getLeafData()->side ^ 1;
		}
	}
}

void
BSP::clipMeshToLeaves(physx::Array<Triangle>& clippedMesh, physx::Array<ClippedTriangleInfo>& triangleInfo, Node* root, float clipTolerance) const
{
	for (Node::It it(root); it.valid(); it.inc())
	{
		Node* node = it.node();
		if (node->getType() == BSP::Node::Leaf)
		{
			if (node->getLeafData()->side == 1)
			{
				Real area, volume;
				clipMeshToLeaf(area, volume, &clippedMesh, &triangleInfo, node, clipTolerance);
			}
		}
	}
}

void BSP::visualizeNode(RenderDebugInterface& debugRender, uint32_t flags, const Node* root) const
{
#if defined(WITHOUT_DEBUG_VISUALIZE)
	PX_UNUSED(debugRender);
	PX_UNUSED(flags);
	PX_UNUSED(root);
#else

	const physx::PxMat44 BSPToMeshTM = PxFromCSG(m_internalTransformInverse);

	for (Node::It it(root); it.valid(); it.inc())
	{
		Node* node = it.node();
		if (node->getType() == BSP::Node::Leaf)
		{
			bool showLeaf = false;
			uint32_t color = 0;
			if (node->getLeafData()->side == 0)
			{
				if (flags & (BSPVisualizationFlags::OutsideRegions | BSPVisualizationFlags::SingleRegion))
				{
					showLeaf = true;
					color = 0xFF0000; // JWR: TODO
				}
			}
			else
			{
				if (flags & (BSPVisualizationFlags::InsideRegions | BSPVisualizationFlags::SingleRegion))
				{
					showLeaf = true;
					color = 0x00FF00; // JWR: TODO
				}
			}

			if (showLeaf)
			{
				RENDER_DEBUG_IFACE(&debugRender)->setCurrentColor(color);
				const Real clampSize = m_meshSize * 10;
				for (SurfaceIt i(node); i.valid(); i.inc())
				{
					const uint32_t planeIndex_i = i.surface()->planeIndex;
					const Plane& plane_i = m_planes[planeIndex_i];
					SurfaceIt j = i;
					j.inc();
					for (; j.valid(); j.inc())
					{
						const uint32_t planeIndex_j = j.surface()->planeIndex;
						const Plane& plane_j = m_planes[planeIndex_j];
						// Find potential edge from intersection if plane_i and plane_j
						Pos orig;
						Dir edgeDir;
						if (intersectPlanes(orig, edgeDir, plane_i, plane_j))
						{
							Real minS = -clampSize;
							Real maxS = clampSize;
							bool intersectionFound = true;
							for (SurfaceIt k(node); k.valid(); k.inc())
							{
								const uint32_t planeIndex_k = k.surface()->planeIndex;
								if (planeIndex_k == planeIndex_i || planeIndex_k == planeIndex_j)
								{
									continue;
								}
								const Plane& plane_k = (k.side() ? (Real)1 : -(Real)1)*m_planes[planeIndex_k];
								const Real num = -(orig|plane_k);
								const Real den = edgeDir|plane_k;
								if (physx::PxAbs(den) > 10*EPS_REAL)
								{
									const Real s = num/den;
									if (den > (Real)0)
									{
										maxS = PxMin(maxS, s);
									}
									else
									{
										minS = PxMax(minS, s);
									}
									if (maxS <= minS)
									{
										intersectionFound = false;
										break;
									}
								}
								else
								if (num < -10*EPS_REAL)
								{
									intersectionFound = false;
									break;
								}
							}
							if (intersectionFound)
							{
								const Pos e0 = orig + minS * edgeDir;
								const Pos e1 = orig + maxS * edgeDir;
								physx::PxVec3 p0, p1;
								p0 = physx::PxVec3(static_cast<float>(e0[0]),static_cast<float>(e0[1]),static_cast<float>(e0[2]));
								p1 = physx::PxVec3(static_cast<float>(e1[0]),static_cast<float>(e1[1]),static_cast<float>(e1[2]));
								RENDER_DEBUG_IFACE(&debugRender)->debugLine(BSPToMeshTM.transform(p0), BSPToMeshTM.transform(p1));
							}
						}
					}
				}
			}
		}
	}
#endif
}

void
BSP::serializeNode(const Node* root, physx::PxFileBuf& stream) const
{
	for (Node::It it(root); it.valid(); it.inc())
	{
		Node* node = it.node();

		if (node != NULL)
		{
			stream << (uint32_t)1;
			stream << (uint32_t)node->getType();

			if (node->getType() == Node::Branch)
			{
				nvidia::serialize(stream, *node->getBranchData());
			}
			else
			{
				nvidia::serialize(stream, *node->getLeafData());
			}
		}
		else
		{
			stream << (uint32_t)0;
		}
	}
}

void
BSP::mergeLeaves(const BoolOp& op, Node* node)
{
	PX_ASSERT(node != NULL);

	// Stackless walk of tree
	bool up = false;
	Node* stop = node->getParent();
	for (;;)
	{
		if (up)
		{
			up = (node->getIndex() == 1);
			node = node->getParent();
			if (node == stop)
			{
				break;
			}
			if (!up)
			{
				node = node->getChild(1);
			}
			else
			{
				// Climbing hierarchy, at a branch
				Node* child0 = node->getChild(0);
				Node* child1 = node->getChild(1);

				// Can consolidate if the children are both leaves on the same side.
				PX_ASSERT(child0 != NULL && child1 != NULL);
				if (child0 != NULL && child1 != NULL && child0->getType() == Node::Leaf && child1->getType() == Node::Leaf)
				{
					Region* region0 = child0->getLeafData();
					Region* region1 = child1->getLeafData();

					PX_ASSERT(region0 != NULL && region1 != NULL);
					PX_ASSERT((region0->side & 1) == region0->side && (region1->side & 1) == region1->side);
					if (region0->side == region1->side)
					{
						// Consolidate

						// Delete children
						child0->detach();
						child1->detach();

						// Turn this node into a leaf
						node->setLeafData(*region0);
						m_memCache->m_nodePool.replace(child0);
						m_memCache->m_nodePool.replace(child1);
					}
				}
			}
		}
		else
		{
			up = (node->getType() == Node::Leaf);
			if (!up)
			{
				// Descend to first child
				node = node->getChild(0);
			}
			else
			{
				// Leaf found
				// Perform boolean operation
				const uint32_t side = node->getLeafData()->side;
				node->getLeafData()->side = op(side & 1, (side >> 1) & 1);
			}
		}
	}
}

// The following functions take a more complex set of arguments, or call recursively from points within the function

BSP::Node*
BSP::deserializeNode(uint32_t version, physx::PxFileBuf& stream)
{
	Node* root = NULL;

	physx::Array<Node*> stack;

	Node* parent = NULL;

	uint32_t readChildIndex = 0xFFFFFFFF;

	for (;;)
	{
		uint32_t createNode;
		stream >> createNode;

		if (createNode)
		{
			Node* node = m_memCache->m_nodePool.borrow();

			if (parent == NULL)
			{
				root = node;
			}
			else
			{
				parent->setChild(readChildIndex, node);
			}

			uint32_t nodeType;
			stream >> nodeType;

			if (nodeType != Node::Leaf)
			{
				Surface surface;
				nvidia::deserialize(stream, version, surface);
				node->setBranchData(surface);

				// Push child 1
				stack.pushBack(node);

				// Process child 0
				parent = node;
				readChildIndex = 0;
			}
			else
			{
				Region region;

				// Make compiler happy
				region.tempIndex1 = region.tempIndex2 = region.tempIndex3 = 0;

				nvidia::deserialize(stream, version, region);

				node->setLeafData(region);

				if (stack.size() == 0)
				{
					break;
				}

				parent = stack.popBack();
				readChildIndex = 1;
			}
		}
	}

	return root;
}

struct CombineTreesFrame
{
	BSP::Node*			node;
	const BSP::Node*	combineNode;
};

void
BSP::combineTrees(Node* root, const Node* combineRoot, uint32_t triangleIndexOffset, uint32_t planeIndexOffset)
{
	physx::Array<CombineTreesFrame> stack;
	stack.reserve(m_planes.size());	// To avoid reallocations

	RegionShape regionShape((const Plane*)m_planes.begin(), (Real)0.0001*m_meshSize);

	CombineTreesFrame localFrame;
	localFrame.node = root;
	localFrame.combineNode = combineRoot;

	for (;;)
	{
		if (localFrame.node->getType() != BSP::Node::Leaf)
		{
			// Push child 1
			CombineTreesFrame& callFrame = stack.insert();
			callFrame.node = localFrame.node->getChild(1);
			callFrame.combineNode = localFrame.combineNode;

			// Process child 0
			localFrame.node = localFrame.node->getChild(0);
			continue;
		}
		else
		{
			if (localFrame.combineNode->getType() != Node::Leaf)
			{
				// Branch node

				// Copy branch data, and offset the triangle indices
				Surface branchSurface;
				const Surface* combineBranchSurface = localFrame.combineNode->getBranchData();
				branchSurface.planeIndex = combineBranchSurface->planeIndex + planeIndexOffset;
				branchSurface.triangleIndexStart = combineBranchSurface->triangleIndexStart + triangleIndexOffset;
				branchSurface.triangleIndexStop = combineBranchSurface->triangleIndexStop + triangleIndexOffset;
				branchSurface.totalTriangleArea = combineBranchSurface->totalTriangleArea;

				// Store off old leaf data
				Region oldRegion = *localFrame.node->getLeafData();

				// Turn this leaf into a branch, see which sides are non-empty
				localFrame.node->setBranchData(branchSurface);
				bool intersects[2];
				for (uint32_t index = 0; index < 2; ++index)
				{
					Node* child = m_memCache->m_nodePool.borrow();
					child->setLeafData(oldRegion);
					localFrame.node->setChild(index, child);
					regionShape.set_leaf(child);
					regionShape.calculate();
					intersects[index] = regionShape.is_nonempty();
				}

				if (intersects[0] && intersects[1])
				{
					// We need both branches
					// Push child 1
					CombineTreesFrame& callFrame = stack.insert();
					callFrame.node = localFrame.node->getChild(1);
					callFrame.combineNode = localFrame.combineNode->getChild(1);

					// Process child 0
					localFrame.node = localFrame.node->getChild(0);
					localFrame.combineNode = localFrame.combineNode->getChild(0);
					continue;
				}
				else
				{
					// Leaf not split by the combining branch.  Return the new branch surface.
					for (uint32_t index = 0; index < 2; ++index)
					{
						Node* child = localFrame.node->getChild(index);
						localFrame.node->setChild(index, NULL);
						m_memCache->m_nodePool.replace(child);
					}
					// Turn this branch back into a leaf
					localFrame.node->setLeafData(oldRegion);
					// Collapse tree by following one branch.
					if (intersects[0])
					{
						localFrame.combineNode = localFrame.combineNode->getChild(0);
						continue;
					}
					else
					if (intersects[1])
					{
						localFrame.combineNode = localFrame.combineNode->getChild(1);
						continue;
					}
					// else we drop down into pop stack, below
				}
			}
			else
			{
				// Leaf node
				localFrame.node->getLeafData()->side = localFrame.node->getLeafData()->side | localFrame.combineNode->getLeafData()->side << 1;
			}
		}
		if (stack.size() == 0)
		{
			break;
		}
		localFrame = stack.popBack();
	}
}

void
BSP::findInsideLeafNeighbors(physx::Array<IntPair>& neighbors, Node* root) const
{
	if (root == NULL)
	{
		return;
	}

	const Real tol = m_meshSize*(Real)0.0001;

	physx::Array<Plane> planes;

	HalfspaceIntersection test;

	for (Node::It it(root); it.valid(); it.inc())
	{
		Node* node = it.node();
		if (node->getType() == BSP::Node::Leaf)
		{
			// Found a leaf.
			if (node->getLeafData()->side == 0)
			{
				continue;	// Only want inside leaves
			}

			// Iterate up to root and collect planes
			planes.resize(0);
			for (SurfaceIt it(node); it.valid(); it.inc())
			{
				const Real sign = it.side() ? (Real)1 : -(Real)1;
				planes.pushBack(sign * m_planes[it.surface()->planeIndex]);
				planes.back()[3] -= tol;
			}

#ifdef CULL_PLANES_LIST
#undef CULL_PLANES_LIST
#endif
#define CULL_PLANES_LIST	1
#if CULL_PLANES_LIST
			// Now flip each plane to see if it's necessary
			for (uint32_t planeIndex = planes.size(); planeIndex--;)
			{
				planes[planeIndex] *= -(Real)1;	// Invert
				test.setPlanes(planes.begin(), planes.size());
				const int result = GSA::vs3d_test(test);
				const bool necessary = 1 == result;
				const bool testError = result < 0;
				planes[planeIndex] *= -(Real)1;	// Restore
				if (!necessary && !testError)
				{
					planes.replaceWithLast(planeIndex);	// Unnecessary; remove
				}
			}
#endif

			if (planes.size() > 0)
			{
				// First half of pair will always be node's index.
				IntPair pair;
				pair.i0 = (int32_t)node->getLeafData()->tempIndex1;

				const uint32_t currentLeafPlaneCount = planes.size();

				// Stackless walk of remainder of tree
				bool up = true;
				while (node != root)
				{
					if (up)
					{
						up = (node->getIndex() == 1);
						node = node->getParent();
						if (planes.size() > currentLeafPlaneCount)
						{
							planes.popBack();
						}
						if (!up)
						{
							planes.pushBack(m_planes[node->getBranchData()->planeIndex]);
							planes.back()[3] -= tol;
							test.setPlanes(planes.begin(), planes.size());
							up = 0 == GSA::vs3d_test(test);	// Skip subtree if there is no intersection at this branch
							node = node->getChild(1);
						}
					}
					else
					{
						up = (node->getType() == Node::Leaf);
						if (!up)
						{
							planes.pushBack(-m_planes[node->getBranchData()->planeIndex]);
							planes.back()[3] -= tol;
							up = 0 == GSA::vs3d_test(test);	// Skip subtree if there is no intersection at this branch
							node = node->getChild(0);
						}
						else
						{
							Region& region = *node->getLeafData();
							if (region.side == 1)
							{
								// We have found another inside leaf which intersects (at boundary)
								pair.i1 = (int32_t)region.tempIndex1;
								neighbors.pushBack(pair);
							}
						}
					}
				}
			}
		}
	}
}

PX_INLINE bool
planeIsNotRedundant(const physx::Array<Plane>& planes, const Plane& plane)
{
	for (uint32_t i = 0; i < planes.size(); ++i)
	{
		if ((planes[i] - plane).lengthSquared() < CSG_EPS)
		{
			return false;
		}
	}
	return true;
}

bool
BSP::addLeafAreasAndVolumes(Real& totalArea, Real& totalVolume, const Node* root, bool inside, const BoolOp& op) const
{
	if (root == NULL)
	{
		return false;
	}

	// Build a list of essential planes
	physx::Array<Plane> planes;

#ifdef CULL_PLANES_LIST
#undef CULL_PLANES_LIST
#endif
#define CULL_PLANES_LIST	0

#if CULL_PLANES_LIST
	HalfspaceIntersection test;
#endif

	const Mat4Real cofInternalTransform = CSGFromPx(m_internalTransform).cof34();

	for (Node::It it(root); it.valid(); it.inc())
	{
		Node* node = it.node();
		if (node->getType() == BSP::Node::Leaf)
		{
			// Found a leaf.

			// See if it's on the correct side (possibly after combining)
			uint32_t side = node->getLeafData()->side;
			if (m_combined)
			{
				side = op(side & 1, (side >> 1) & 1);
			}
			if ((side != 0) != inside)
			{
				continue;
			}

			// Iterate up to root and collect planes
			planes.resize(0);
			for (SurfaceIt it(node); it.valid(); it.inc())
			{
				const Real sign = it.side() ? (Real)1 : -(Real)1;
				planes.pushBack(sign * m_planes[it.surface()->planeIndex]);
			}

#if CULL_PLANES_LIST
			// Now flip each plane to see if it's necessary
			for (uint32_t planeIndex = planes.size(); planeIndex--;)
			{
				planes[planeIndex] *= -(Real)1;	// Invert
				test.setPlanes(planes.begin(), planes.size());
				const bool necessary = test.intersect();
				const bool testError = (test.state() & ApexCSG::GSA::GSA_State::Error_Flag) != 0;
				planes[planeIndex] *= -(Real)1;	// Restore
				if (!necessary && !error)
				{
					planes.replaceWithLast(planeIndex);	// Unnecessary; remove
				}
			}
#endif

			// Now use this culled plane list to find areas and volumes
			if (planes.size() > 0)
			{
				Real area, volume;
				if (!calculateLeafAreaAndVolume(area, volume, planes.begin(), planes.size(), cofInternalTransform))
				{
					totalArea = MAX_REAL;
					totalVolume = MAX_REAL;
					return false;	// No need to add anymore
				}
				totalArea += area;
				totalVolume += volume;
			}
		}
	}

	return true;
}

struct CloneFrame
{
	BSP::Node*			node;
	const BSP::Node*	original;
};

void
BSP::clone(Node* root, const Node* originalRoot)
{
	physx::Array<CloneFrame> stack;

	CloneFrame localFrame;
	localFrame.node = root;
	localFrame.original = originalRoot;

	for (;;)
	{
		switch (localFrame.original->getType())
		{
		case Node::Leaf:
			localFrame.node->setLeafData(*localFrame.original->getLeafData());
			break;
		case Node::Branch:
			localFrame.node->setBranchData(*localFrame.original->getBranchData());
			break;
		}

		const Node* originalChild;
		Node* child;

		// Push child 1
		originalChild = localFrame.original->getChild(1);
		if (originalChild != NULL)
		{
			child = m_memCache->m_nodePool.borrow();
			localFrame.node->setChild(1, child);
			CloneFrame& callFrame = stack.insert();
			callFrame.node = child;
			callFrame.original = originalChild;
		}

		// Process child 0
		originalChild = localFrame.original->getChild(0);
		if (originalChild != NULL)
		{
			child = m_memCache->m_nodePool.borrow();
			localFrame.node->setChild(0, child);
			localFrame.node = child;
			localFrame.original = originalChild;
		}
		else
		{
			if (stack.size() == 0)
			{
				break;
			}
			localFrame = stack.popBack();
		}
	}
}

struct BuildTreeFrame
{
	BSP::Node*		node;
	uint32_t	surfaceStackReadStart;
	uint32_t	surfaceStackReadStop;
	uint32_t	inputSurfaceStackSize;
};

bool
BSP::buildTree(Node* node, physx::Array<Surface>& surfaceStack, uint32_t stackReadStart, uint32_t stackReadStop,
			   const BuildConstants& buildConstants, QuantityProgressListener* quantityListener, volatile bool* cancel)
{
	physx::Array<BuildTreeFrame> stack;
	stack.reserve(surfaceStack.size());	// To avoid reallocations

	BuildTreeFrame localFrame;
	localFrame.node = node;
	localFrame.surfaceStackReadStart = stackReadStart;
	localFrame.surfaceStackReadStop = stackReadStop;
	localFrame.inputSurfaceStackSize = surfaceStack.size();

	for (;;)
	{
		if (cancel && *cancel)
		{
			return false;
		}

		localFrame.surfaceStackReadStop = removeRedundantSurfacesFromStack(surfaceStack, localFrame.surfaceStackReadStart, localFrame.surfaceStackReadStop, localFrame.node);
		if (localFrame.surfaceStackReadStop == localFrame.surfaceStackReadStart)
		{
			assignLeafSide(localFrame.node, quantityListener);
			if (stack.size() == 0)
			{
				break;
			}
			localFrame = stack.popBack();
			surfaceStack.resize(localFrame.inputSurfaceStackSize);
		}
		else
		{
			uint32_t childReadStart[2];
			uint32_t childReadStop[2];
			createBranchSurfaceAndSplitStack(childReadStart, childReadStop, localFrame.node, surfaceStack, localFrame.surfaceStackReadStart, localFrame.surfaceStackReadStop, buildConstants);

			Node* child;

			// Push child 1
			child = m_memCache->m_nodePool.borrow();
			child->getLeafData()->side = 1;
			localFrame.node->setChild(1, child);
			BuildTreeFrame& callFrame = stack.insert();
			callFrame.node = child;
			callFrame.surfaceStackReadStart = childReadStart[1];
			callFrame.surfaceStackReadStop = childReadStop[1];
			callFrame.inputSurfaceStackSize = surfaceStack.size();

			// Process child 0
			child = m_memCache->m_nodePool.borrow();
			child->getLeafData()->side = 0;
			localFrame.node->setChild(0, child);
			localFrame.node = child;
			localFrame.surfaceStackReadStart = childReadStart[0];
			localFrame.surfaceStackReadStop = childReadStop[0];
			localFrame.inputSurfaceStackSize = surfaceStack.size();
		}
	}

	return true;
}


/* For GSA */

GSA::real
BSP::RegionShape::farthest_halfspace(GSA::real plane[4], const GSA::real point[4])
{
	plane[0] = plane[1] = plane[2] = 0.0f;
	plane[3] = 1.0f;
	Real greatest_s = -MAX_REAL;

	if (m_leaf && m_planes)
	{
		for (SurfaceIt it(m_leaf); it.valid(); it.inc())
		{
			const Real sign = it.side() ? (Real)1 : -(Real)1;
			Plane test = sign * m_planes[it.surface()->planeIndex];
			test[3] -= m_skinWidth;
			const Real s = point[0]*test[0] + point[1]*test[1] + point[2]*test[2] + point[3]*test[3];
			if (s > greatest_s)
			{
				greatest_s = s;
				for (int i = 0; i < 4; ++i)
				{
					plane[i] = (GSA::real)test[i];
				}
			}
		}
	}

	// Return results
	return (GSA::real)greatest_s;
}


/* BSPMemCache */

BSPMemCache::BSPMemCache() :
	m_nodePool(10000)
{
}

void
BSPMemCache::clearAll()
{
	const int32_t nodesRemaining = m_nodePool.empty();

	char message[1000];
	if (nodesRemaining != 0)
	{
		physx::shdfnd::snprintf(message, 1000, "BSPMemCache: %d nodes %sfreed ***", physx::PxAbs(nodesRemaining), nodesRemaining > 0 ? "un" : "over");
		GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_INFO, message, __FILE__, __LINE__);
	}

	clearTemp();
}

void
BSPMemCache::clearTemp()
{
	m_surfaceFlags.reset();
	m_surfaceTestFlags.reset();
	const int32_t linkedVerticesRemaining = m_linkedVertexPool.empty();

	char message[1000];
	if (linkedVerticesRemaining != 0)
	{
		physx::shdfnd::snprintf(message, 1000, "BSPMemCache: %d linked vertices %sfreed ***", physx::PxAbs(linkedVerticesRemaining), linkedVerticesRemaining > 0 ? "un" : "over");
		GetApexSDK()->getErrorCallback()->reportError(physx::PxErrorCode::eDEBUG_INFO, message, __FILE__, __LINE__);
	}
}

void
BSPMemCache::release()
{
	clearAll();
	delete this;
}


/* CSG Tools API */

IApexBSPMemCache*
createBSPMemCache()
{
	return PX_NEW(BSPMemCache)();
}

IApexBSP*
createBSP(IApexBSPMemCache* memCache, const physx::PxMat44& internalTransform)
{
	IApexBSP* bsp = PX_NEW(BSP)(memCache, internalTransform);

	bsp->setTolerances(gDefaultTolerances);

	return bsp;
}

}
#endif
