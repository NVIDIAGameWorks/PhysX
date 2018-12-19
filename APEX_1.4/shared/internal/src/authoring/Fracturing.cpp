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


//#ifdef _MANAGED
//#pragma managed(push, off)
//#endif

#include <stdarg.h>
#include "Apex.h"
#include "ApexSharedUtils.h"

#include "authoring/Fracturing.h"
#include "authoring/ApexGSA.h"
#include "PsUserAllocated.h"
#include "ApexRand.h"
#include "PxErrorCallback.h"
#include "PsString.h"
#include "ApexUsingNamespace.h"
#include "PxPlane.h"
#include "PsMathUtils.h"
#include "PsAllocator.h"
#include "ConvexDecomposition.h"
#include "Noise.h"
#include "DestructibleAsset.h"
#include "Link.h"
#include "RenderDebugInterface.h"
#include "PsSort.h"
#include "PsInlineArray.h"
#include "PsBitUtils.h"

#define INCREMENTAL_GSA	0

/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#ifndef WITHOUT_APEX_AUTHORING
#include "ApexSharedSerialization.h"


using namespace nvidia;	// !?  Need to do this for PX_ALLOCA!?


#define MAX_ALLOWED_ESTIMATED_CHUNK_TOTAL	10000

#define CUTOUT_MAP_BOUNDS_TOLERANCE	0.0001f
#define MESH_INSTANACE_TOLERANCE	0.025f

static ApexCSG::BSPBuildParameters
gDefaultBuildParameters;

static bool gIslandGeneration = false;
static unsigned gMicrogridSize = 65536;
static BSPOpenMode::Enum gMeshMode = BSPOpenMode::Automatic;
static int	gVerbosity = 0;


static CollisionVolumeDesc getVolumeDesc(const CollisionDesc& collisionDesc, unsigned depth)
{
	return collisionDesc.mDepthCount > 0 ? collisionDesc.mVolumeDescs[PxMin(depth, collisionDesc.mDepthCount-1)] : CollisionVolumeDesc();
}

PX_INLINE float extentDistance(float min0, float max0, float min1, float max1)
{
	return PxMax(min0 - max1, min1 - max0);
}


static physx::PxMat44 randomRotationMatrix(physx::PxVec3 zAxis, nvidia::QDSRand& rnd)
{
	physx::PxMat44 rot;
	zAxis.normalize();
	uint32_t maxDir =  physx::PxAbs(zAxis.x) > physx::PxAbs(zAxis.y) ?
		(physx::PxAbs(zAxis.x) > physx::PxAbs(zAxis.z) ? 0u : 2u) :
		(physx::PxAbs(zAxis.y) > physx::PxAbs(zAxis.z) ? 1u : 2u);
	physx::PxVec3 xAxis = physx::PxMat33(physx::PxIdentity)[(maxDir + 1) % 3];
	physx::PxVec3 yAxis = zAxis.cross(xAxis);
	yAxis.normalize();
	xAxis = yAxis.cross(zAxis);

	const float angle = rnd.getScaled(-physx::PxPi, physx::PxPi);
	const float c = physx::PxCos(angle);
	const float s = physx::PxSin(angle);

	rot.column0 = physx::PxVec4(c*xAxis + s*yAxis, 0.0f);
	rot.column1 = physx::PxVec4(c*yAxis - s*xAxis, 0.0f);
	rot.column2 = physx::PxVec4(zAxis, 0.0f);
	rot.column3 = physx::PxVec4(0.0f, 0.0f, 0.0f, 1.0f);

	return rot;
}

PX_INLINE physx::PxVec3 randomPositionInTriangle(const physx::PxVec3& v1, const physx::PxVec3& v2, const physx::PxVec3& v3, nvidia::QDSRand& rnd)
{
	const physx::PxVec3 d1 = v2 - v1;
	const physx::PxVec3 d2 = v3 - v1;
	float c1 = rnd.getUnit();
	float c2 = rnd.getUnit();
	const float d = 1.0f - (c1+c2);
	if (d < 0.0f)
	{
		c1 += d;
		c2 += d;
	}
	return v1 + c1*d1 + c2*d2;
}


// Used by VoronoiCellPlaneIterator
class ReciprocalSitePairLink : public Link
{
public:
	ReciprocalSitePairLink() : Link(), m_recip(NULL)
	{
	}
	ReciprocalSitePairLink(const ReciprocalSitePairLink& other) : Link()
	{
		index0 = other.index0;
		index1 = other.index1;
		plane = other.plane;
		m_recip = NULL;
	}
	~ReciprocalSitePairLink()
	{
		remove();
	}

	void					setRecip(ReciprocalSitePairLink& recip)
	{
		PX_ASSERT(m_recip == NULL && recip.m_recip == NULL);
		m_recip = &recip;
		recip.m_recip = this;
	}

	ReciprocalSitePairLink* getRecip() const
	{
		return m_recip;
	}

	ReciprocalSitePairLink*	getAdj(uint32_t which) const
	{
		return static_cast<ReciprocalSitePairLink*>(Link::getAdj(which));
	}

	void					remove()
	{
		if (m_recip)
		{
			m_recip->m_recip = NULL;
			m_recip = NULL;
		}
		Link::remove();
	}

	uint32_t	index0, index1;
	physx::PxPlane	plane;

private:
	ReciprocalSitePairLink*	m_recip;
};


struct SiteMidPlaneIteratorInit
{
	SiteMidPlaneIteratorInit() : first(NULL), stop(NULL) {}

	ReciprocalSitePairLink*	first;
	ReciprocalSitePairLink* stop;
};

class SiteMidPlaneIterator
{
public:
	SiteMidPlaneIterator(const SiteMidPlaneIteratorInit& listBounds) : current(listBounds.first), stop(listBounds.stop) {}

	bool	valid() const
	{
		return current != stop;
	}

	void	inc()
	{
		current = current->getAdj(1);
	}

	ApexCSG::Plane	plane() const
	{
		const physx::PxPlane& midPlane = current->plane;
		ApexCSG::Plane plane(ApexCSG::Dir((ApexCSG::Real)midPlane.n.x, (ApexCSG::Real)midPlane.n.y,(ApexCSG::Real)midPlane.n.z), (ApexCSG::Real)midPlane.d);
		plane.normalize();
		return plane;
	}

private:
	ReciprocalSitePairLink* current;
	ReciprocalSitePairLink* stop;
};

class SiteMidPlaneIntersection : public ApexCSG::GSA::StaticConvexPolyhedron<SiteMidPlaneIterator, SiteMidPlaneIteratorInit>
{
public:
	void	setPlanes(ReciprocalSitePairLink* first, ReciprocalSitePairLink* stop)
	{
		m_initValues.first = first;
		m_initValues.stop = stop;
	}

	void	replacePlanes(ReciprocalSitePairLink* first, ReciprocalSitePairLink* stop, const physx::PxPlane& oldFlipPlane, const physx::PxPlane& newFlipPlane)
	{
		m_initValues.first = first;
		m_initValues.stop = stop;
#if INCREMENTAL_GSA
		const ApexCSG::Plane oldFlipGSAPlane = ApexCSG::Plane(ApexCSG::Dir((ApexCSG::Real)oldFlipPlane.n.x, (ApexCSG::Real)oldFlipPlane.n.y, (ApexCSG::Real)oldFlipPlane.n.z), (ApexCSG::Real)oldFlipPlane.d);
		const ApexCSG::Plane newFlipGSAPlane = ApexCSG::Plane(ApexCSG::Dir((ApexCSG::Real)newFlipPlane.n.x, (ApexCSG::Real)newFlipPlane.n.y, (ApexCSG::Real)newFlipPlane.n.z), (ApexCSG::Real)newFlipPlane.d);
		for (int i = 0; i < 4; ++i)
		{
			if (m_S(0,i) == oldFlipGSAPlane(0) && m_S(1,i) == oldFlipGSAPlane(1) && m_S(2,i) == oldFlipGSAPlane(2) && m_S(3,i) == oldFlipGSAPlane(3))
			{
				m_S.setCol(i, -oldFlipGSAPlane);
			}
			if (m_S(0,i) == newFlipGSAPlane(0) && m_S(1,i) == newFlipGSAPlane(1) && m_S(2,i) == newFlipGSAPlane(2) && m_S(3,i) == newFlipGSAPlane(3))
			{
				m_S.setCol(i, -newFlipGSAPlane);
			}
		}
#else
		(void)oldFlipPlane;
		(void)newFlipPlane;
#endif
	}

	void	resetPlanes()
	{
		m_initValues.first = NULL;
		m_initValues.stop = NULL;
	}
};

// Voronoi decomposition utility
class VoronoiCellPlaneIterator
{
public:
							VoronoiCellPlaneIterator(const physx::PxVec3* sites, uint32_t siteCount, const physx::PxPlane* boundPlanes = NULL, uint32_t boundPlaneCount = 0, uint32_t startSiteIndex = 0);

	bool					valid() const
							{
								return m_valid;
							}

	uint32_t			cellIndex() const
							{
								return m_valid ? m_cellIndex : 0xFFFFFFFF;
							}

	const physx::PxPlane*	cellPlanes() const
							{
								return m_valid ? m_cellPlanes.begin() : NULL;
							}

	uint32_t			cellPlaneCount() const
							{
								return m_valid ? m_cellPlanes.size() : 0;
							}

	void					inc()
							{
								if (m_valid)
								{
									if (m_startPair != &m_listRoot)
									{
										prepareOutput();
									}
									else
									{
										m_valid = false;
									}
								}
							}

private:
	void					prepareOutput();

	// Input
	const physx::PxVec3*					m_sites;
	uint32_t							m_siteCount;
	uint32_t							m_boundPlaneCount;

	// State and intermediate data
	physx::Array<ReciprocalSitePairLink>	m_sitePairs;	// A symmetric array of site pairs and their bisector planes, in order (i major, j minor), with the diagonal removed
	SiteMidPlaneIntersection				m_test;			// Used to see if a plane is necessary for a cell
	ReciprocalSitePairLink*					m_startPair;	// Current start site pair
	ReciprocalSitePairLink					m_listRoot;		// A stopping node

	// Output
	bool									m_valid;
	uint32_t							m_cellIndex;
	physx::Array<physx::PxPlane>			m_cellPlanes;
};

VoronoiCellPlaneIterator::VoronoiCellPlaneIterator(const physx::PxVec3* sites, uint32_t siteCount, const physx::PxPlane* boundPlanes, uint32_t boundPlaneCount, uint32_t startSiteIndex)
{
	m_valid = false;

	if (sites == NULL || startSiteIndex >= siteCount)
	{
		return;
	}

	m_valid = true;

	m_sites = sites;
	m_siteCount = siteCount;
	m_cellIndex = startSiteIndex;
	m_boundPlaneCount = boundPlanes != NULL ? boundPlaneCount : 0;

	// Add the bound planes
	m_cellPlanes.reserve(m_boundPlaneCount);
	for (uint32_t boundPlaneNum = 0; boundPlaneNum < m_boundPlaneCount; ++boundPlaneNum)
	{
		m_cellPlanes.pushBack(boundPlanes[boundPlaneNum]);
	}

	// This should mean m_siteCount = 1.  In this case, there are no planes (besides the bound planes)
	if (m_siteCount < 2)
	{
		m_startPair = &m_listRoot;	// Causes termination after one iteration
		return;
	}

	// Fill in the pairs
	m_sitePairs.resize(m_siteCount*(m_siteCount-1));
	uint32_t pairIndex = 0;
	for (uint32_t i = 0; i < m_siteCount; ++i)
	{
		for (uint32_t j = 0; j < m_siteCount; ++j)
		{
			if (j == i)
			{
				continue;
			}
			ReciprocalSitePairLink& pair = m_sitePairs[pairIndex];
			if (j > i)
			{
				pair.setRecip(m_sitePairs[pairIndex+(j-i)*(m_siteCount-2)+1]);
			}
			pair.index0 = i;
			pair.index1 = j;
			pair.plane = physx::PxPlane(0.5f*(m_sites[j] + m_sites[i]), (m_sites[j] - m_sites[i]).getNormalized());
			// Link together into a single loop
			if (pairIndex > 0)
			{
				m_sitePairs[pairIndex-1].setAdj(1, &pair);
			}

			++pairIndex;
		}
	}

	// Start with the first pair in the array
	m_startPair = &m_sitePairs[0];

	// Create a list root
	m_listRoot.setAdj(1, m_startPair);

	// Find first pair with the desired index0
	while (m_startPair->index0 != startSiteIndex && m_startPair != &m_listRoot)
	{
		m_startPair = m_startPair->getAdj(1);
	}

	prepareOutput();
}

void VoronoiCellPlaneIterator::prepareOutput()
{
	if (!m_valid)
	{
		return;
	}

	m_cellIndex = m_startPair->index0;

	// Find the first pair with a different first site index, which is our end-marker
	ReciprocalSitePairLink* stopPair = m_startPair->getAdj(1);
	while (stopPair != &m_listRoot && stopPair->index0 == m_cellIndex)
	{
		stopPair = stopPair->getAdj(1);
	}

	PX_ASSERT(stopPair == &m_listRoot || stopPair->index0 == m_startPair->index0+1);

	// Reset planes (keeping bound planes)
	m_cellPlanes.resize(m_boundPlaneCount);

	// Now iterate through this subset of the list, flipping one plane each time
	ReciprocalSitePairLink* testPlanePair = m_startPair;
	bool firstGSAUse = true;
	physx::PxPlane lastPlane;
	do
	{
		ReciprocalSitePairLink* nextPlanePair = testPlanePair->getAdj(1);
		testPlanePair->plane = physx::PxPlane(-testPlanePair->plane.n, -testPlanePair->plane.d);	// Flip
		if (firstGSAUse)
		{
			m_test.setPlanes(m_startPair, stopPair);
#if INCREMENTAL_GSA
			firstGSAUse = false;
#endif
		}
		else
		{
			m_test.replacePlanes(m_startPair, stopPair, lastPlane, testPlanePair->plane);
		}
		lastPlane = testPlanePair->plane;
		const bool keep = (1 == ApexCSG::GSA::vs3d_test(m_test));
		testPlanePair->plane = physx::PxPlane(-testPlanePair->plane.n, -testPlanePair->plane.d);	// Flip back
		if (keep)
		{
			// This is a bounding plane
			m_cellPlanes.pushBack(testPlanePair->plane);
		}
		else
		{
			// Flipping this plane results in an empty set intersection.  It is non-essential, so remove it
			// And its reciprocal
			if (testPlanePair->getRecip() == stopPair)
			{
				stopPair = stopPair->getAdj(1);
			}
			testPlanePair->getRecip()->remove();
			if (testPlanePair == m_startPair)
			{
				m_startPair = m_startPair->getAdj(1);
			}
			testPlanePair->remove();
		}
		testPlanePair = nextPlanePair;
	} while (testPlanePair != stopPair);

	m_startPair = stopPair;
}


PX_INLINE bool segmentOnBorder(const physx::PxVec3& v0, const physx::PxVec3& v1, float width, float height)
{
	return
	    (v0.x < -0.5f && v1.x < -0.5f) ||
	    (v0.y < -0.5f && v1.y < -0.5f) ||
	    (v0.x >= width - 0.5f && v1.x >= width - 0.5f) ||
	    (v0.y >= height - 0.5f && v1.y >= height - 0.5f);
}

class Random : public ApexCSG::UserRandom
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

	nvidia::QDSRand	m_rnd;
} userRnd;


typedef ApexCSG::Vec<float,2> Vec2Float;
typedef ApexCSG::Vec<float,3> Vec3Float;

// TODO: Provide configurable octave parameter
ApexCSG::PerlinNoise<float, 64, 2, Vec2Float > userPerlin2D(userRnd, 2, 1.5f, 2.5f);
ApexCSG::PerlinNoise<float, 64, 3, Vec3Float > userPerlin3D(userRnd, 2, 1.5f, 2.5f);

PX_INLINE bool edgesOverlap(const physx::PxVec3& pv00, const physx::PxVec3& pv01, const physx::PxVec3& pv10, const physx::PxVec3& pv11, float eps)
{
	physx::PxVec3 e0 = pv01 - pv00;
	physx::PxVec3 e1 = pv11 - pv10;

	if (e0.dot(e1) < 0)
	{
		return false;
	}

	float l0 = e0.normalize();
	e1.normalize();

	const physx::PxVec3 disp0 = pv10 - pv00;
	const physx::PxVec3 disp1 = pv11 - pv00;

	const float d10 = disp0.dot(e0);

	const float d11 = disp1.dot(e0);

	if (d11 < -eps)
	{
		return false;
	}

	if (d10 > l0 + eps)
	{
		return false;
	}

	const float disp02 = disp0.dot(disp0);
	if (disp02 - d10 * d10 > eps * eps)
	{
		return false;
	}

	const float disp12 = disp1.dot(disp1);
	if (disp12 - d11 * d11 > eps * eps)
	{
		return false;
	}

	return true;
}

PX_INLINE bool trianglesOverlap(const physx::PxVec3& pv00, const physx::PxVec3& pv01, const physx::PxVec3& pv02, const physx::PxVec3& pv10, const physx::PxVec3& pv11, const physx::PxVec3& pv12, float eps)
{
	return	edgesOverlap(pv00, pv02, pv10, pv11, eps) || edgesOverlap(pv00, pv02, pv11, pv12, eps) || edgesOverlap(pv00, pv02, pv12, pv10, eps) ||
	        edgesOverlap(pv01, pv00, pv10, pv11, eps) || edgesOverlap(pv01, pv00, pv11, pv12, eps) || edgesOverlap(pv01, pv00, pv12, pv10, eps) ||
	        edgesOverlap(pv02, pv01, pv10, pv11, eps) || edgesOverlap(pv02, pv01, pv11, pv12, eps) || edgesOverlap(pv02, pv01, pv12, pv10, eps);
}


// Returns a point uniformly distributed on the "polar cap" in +axisN direction, of azimuthal size range (in radians)
PX_INLINE physx::PxVec3	randomNormal(uint32_t axisN, float range)
{
	physx::PxVec3 result;
	const float cosTheta = 1.0f - (1.0f - physx::PxCos(range)) * userRnd.getReal(0.0f, 1.0f);
	const float sinTheta = physx::PxSqrt(1.0f - cosTheta * cosTheta);
	float cosPhi, sinPhi;
	physx::shdfnd::sincos(userRnd.getReal(-physx::PxPi, physx::PxPi), sinPhi, cosPhi);
	result[axisN % 3] = cosTheta;
	result[(axisN + 1) % 3] = cosPhi * sinTheta;
	result[(axisN + 2) % 3] = sinPhi * sinTheta;
	return result;
}

void calculatePartition(int partition[3], const unsigned requestedSplits[3], const physx::PxVec3& extent, const float* targetProportions)
{
	partition[0] = (int32_t)requestedSplits[0] + 1;
	partition[1] = (int32_t)requestedSplits[1] + 1;
	partition[2] = (int32_t)requestedSplits[2] + 1;

	if (targetProportions != NULL)
	{
		physx::PxVec3 n(extent[0] / targetProportions[0], extent[1] / targetProportions[1], extent[2] / targetProportions[2]);
		n *= physx::PxVec3((float)partition[0], (float)partition[1], (float)partition[2]).dot(n) / n.magnitudeSquared();
		// n now contains the # of partitions per axis closest to the desired # of partitions
		// which give the correct target proportions. However, the numbers will not (in general)
		// be integers, so round:
		partition[0] = PxMax(1, (int)(n[0] + 0.5f));
		partition[1] = PxMax(1, (int)(n[1] + 0.5f));
		partition[2] = PxMax(1, (int)(n[2] + 0.5f));
	}
}

static void outputMessage(const char* message, physx::PxErrorCode::Enum errorCode = physx::PxErrorCode::eNO_ERROR, int verbosity = 0)	// Lower # = higher priority
{
	if (verbosity > gVerbosity)
	{
		return;
	}

	physx::PxErrorCallback* outputStream = nvidia::GetApexSDK()->getErrorCallback();
	if (outputStream)
	{
		outputStream->reportError(errorCode, message, __FILE__, __LINE__);
	}
}

struct ChunkIndexer
{
	ExplicitHierarchicalMeshImpl::Chunk*	chunk;
	int32_t							parentIndex;
	int32_t							index;

	static int compareParentIndices(const void* A, const void* B)
	{
		const int diff = ((const ChunkIndexer*)A)->parentIndex - ((const ChunkIndexer*)B)->parentIndex;
		if (diff)
		{
			return diff;
		}
		return ((const ChunkIndexer*)A)->index - ((const ChunkIndexer*)B)->index;
	}
};

static physx::PxBounds3 boundTriangles(const physx::Array<nvidia::ExplicitRenderTriangle>& triangles, const PxMat44& interiorTM)
{
	physx::PxBounds3 bounds;
	bounds.setEmpty();
	for (uint32_t triangleN = 0; triangleN < triangles.size(); ++triangleN)
	{
		for (int v = 0; v < 3; ++v)
		{
			physx::PxVec3 localVert = interiorTM.inverseRT().transform(triangles[triangleN].vertices[v].position);
			bounds.include(localVert);
		}
	}
	return bounds;
}

PX_INLINE void generateSliceAxes(uint32_t sliceAxes[3], uint32_t sliceAxisNum)
{
	switch (sliceAxisNum)
	{
	case 0:
		sliceAxes[1] = 2;
		sliceAxes[0] = 1;
		break;
	case 1:
		sliceAxes[1] = 2;
		sliceAxes[0] = 0;
		break;
	default:
	case 2:
		sliceAxes[1] = 1;
		sliceAxes[0] = 0;
	}
	sliceAxes[2] = sliceAxisNum;
}

PX_INLINE physx::PxVec3 createAxis(uint32_t axisNum)
{
	return physx::PxMat33(physx::PxIdentity)[axisNum];
}

PX_INLINE void 	getCutoutSliceAxisAndSign(uint32_t& sliceAxisNum, uint32_t& sliceSignNum, uint32_t sliceDirIndex)
{
	sliceAxisNum  = sliceDirIndex >> 1;
	sliceSignNum  = sliceDirIndex & 1;
}

typedef float(*NoiseFn)(float x, float y, float z, float& xGrad, float& yGrad, float& zGrad);

static float planeWave(float x, float y, float, float& xGrad, float& yGrad, float& zGrad)
{
	float c, s;
	physx::shdfnd::sincos(x + y, s, c);
	xGrad = c;
	yGrad = 0.0f;
	zGrad = 0.0f;
	return s;
}

static float perlin2D(float x, float y, float, float& xGrad, float& yGrad, float& zGrad)
{
	const float xy[] = {x, y};
	float s = userPerlin2D.sample(Vec2Float(xy));
	// TODO: Implement
	xGrad = 0.0f;
	yGrad = 0.0f;
	zGrad = 0.0f;
	return s;
}

static float perlin3D(float x, float y, float z, float& xGrad, float& yGrad, float& zGrad)
{
	const float xyz[] = {x, y, z};
	float s = userPerlin3D.sample(Vec3Float(xyz));
	// TODO: Implement
	xGrad = 0.0f;
	yGrad = 0.0f;
	zGrad = 0.0f;
	return s;
}

static NoiseFn noiseFns[] =
{
	planeWave,
	perlin2D,
	perlin3D
};

static int noiseFnCount = sizeof(noiseFns) / sizeof(noiseFns[0]);

static void buildNoise(physx::Array<float>& f, physx::Array<physx::PxVec3>* n,
					   nvidia::IntersectMesh::GridPattern pattern, float cornerX, float cornerY, float xSpacing, float ySpacing, uint32_t numX, uint32_t numY,
					   float noiseAmplitude, float relativeFrequency, float xPeriod, float yPeriod,
					   int noiseType, int noiseDir)
{
	const uint32_t gridSize = (numX + 1) * (numY + 1);
	
	if( f.size() != gridSize) 
		f.resize(gridSize, 0.);
	
	if( n && n->size() != gridSize) 
		n->resize(gridSize, physx::PxVec3(0,0,0));

	noiseType = physx::PxClamp(noiseType, 0 , noiseFnCount - 1);
	NoiseFn noiseFn = noiseFns[noiseType];

	// This differentiation between wave planes and perlin is rather arbitrary, but works alright
	const uint32_t numModes = noiseType == FractureSliceDesc::NoiseWavePlane ? 20u : 4u;
	const float amplitude = noiseAmplitude / physx::PxSqrt((float)numModes);	// Scale by frequency?
	for (uint32_t i = 0; i < numModes; ++i)
	{
		float phase     = userRnd.getReal(-3.14159265f, 3.14159265f);
		float freqShift = userRnd.getReal(0.0f, 3.0f);
		float kx, ky;
		switch (noiseDir)
		{
		case 0:
			kx = physx::PxPow(2.0f, freqShift) * relativeFrequency / xSpacing;
			ky = 0.0f;
			break;
		case 1:
			kx = 0.0f;
			ky = physx::PxPow(2.0f, freqShift) * relativeFrequency / ySpacing;
			break;
		default:
			{
				const float f = physx::PxPow(2.0f, freqShift) * relativeFrequency;
				const float theta = userRnd.getReal(-3.14159265f, 3.14159265f);
				const float c = physx::PxCos(theta);
				const float s = physx::PxSin(theta);
				kx = c * f / xSpacing;
				ky = s * f / ySpacing;
			}
		}

		if (xPeriod != 0.0f)
		{
			const float cx = (2.0f * 3.14159265f) / xPeriod;
			const int nx = (int)physx::PxSign(kx) * (int)(physx::PxAbs(kx) / cx + 0.5f);	// round
			kx = nx * cx;
		}

		if (yPeriod != 0.0f)
		{
			// Make sure the wavenumbers are integers
			const float cy = (2.0f * 3.14159265f) / yPeriod;
			const int ny = (int)physx::PxSign(ky) * (int)(physx::PxAbs(ky) / cy + 0.5f);	// round
			ky = ny * cy;
		}

		uint32_t pointN = 0;
		float y = cornerY;
		for (uint32_t iy = 0; iy <= numY; ++iy, y += ySpacing)
		{
			float x = cornerX;
			for (uint32_t ix = 0; ix <= numX; ++ix, x += xSpacing, ++pointN)
			{
				if (pattern == nvidia::IntersectMesh::Equilateral && (((iy & 1) == 0 && ix == numX) || ((iy & 1) != 0 && ix == 1)))
				{
					x -= 0.5f * xSpacing;
				}
				float xGrad, yGrad, zGrad;
				// TODO: Find point in 3D space for use with NoisePerlin3D
				f[pointN] += amplitude * noiseFn(x * kx - phase, y * ky - phase, 0, xGrad, yGrad, zGrad);
				if (n) (*n)[pointN] += physx::PxVec3(-xGrad * kx * amplitude, -yGrad * ky * amplitude, 0.0f);
			}
		}
	}

}

// noiseDir = 0 => X
// noiseDir = 1 => Y
// noiseDir = -1 => userRnd
void nvidia::IntersectMesh::build(GridPattern pattern, const physx::PxPlane& plane,
                                 float cornerX, float cornerY, float xSpacing, float ySpacing, uint32_t numX, uint32_t numY,
                                 const PxMat44& tm, float noiseAmplitude, float relativeFrequency, float xPeriod, float yPeriod,
                                 int noiseType, int noiseDir, uint32_t submeshIndex, uint32_t frameIndex, const nvidia::TriangleFrame& triangleFrame, bool forceGrid)
{
	m_pattern = pattern;
	m_plane = plane;
	m_cornerX = cornerX;
	m_cornerY = cornerY;
	m_xSpacing = xSpacing;
	m_ySpacing = ySpacing;
	m_numX = numX;
	m_numY = numY;
	m_tm = tm;

	if (relativeFrequency == 0.0f)
	{
		// 0 frequency only provides a plane offset
		m_plane.d += userRnd.getReal(-noiseAmplitude, noiseAmplitude);
		noiseAmplitude = 0.0f;
	}

	if (!forceGrid && noiseAmplitude == 0.0f)
	{
		// Without noise, we only need one triangle
		m_pattern = Equilateral;
		m_vertices.resize(3);
		m_triangles.resize(1);

		const float rX = 0.5f * (xSpacing * numX);
		const float rY = 0.5f * (ySpacing * numY);
		const float centerX = cornerX + rX;
		const float centerY = cornerY + rY;

		// Circumscribe rectangle
		const float R = physx::PxSqrt(rX * rX + rY * rY);

		// Fit equilateral triangle around circle
		const float x = 1.73205081f * R;
		m_vertices[0].position = tm.transform(physx::PxVec3(centerX, centerY + 2 * R, 0));
		m_vertices[1].position = tm.transform(physx::PxVec3(centerX - x, centerY - R, 0));
		m_vertices[2].position = tm.transform(physx::PxVec3(centerX + x, centerY - R, 0));

		for (uint32_t i = 0; i < 3; ++i)
		{
			m_vertices[i].normal = m_plane.n;
			m_vertices[i].tangent = tm.column0.getXYZ();
			m_vertices[i].binormal = tm.column1.getXYZ();
			m_vertices[i].color = ColorRGBA(255, 255, 255, 255);
		}

		ExplicitRenderTriangle& triangle = m_triangles[0];
		for (uint32_t v = 0; v < 3; ++v)
		{
			Vertex& gridVertex = m_vertices[v];
			triangle.vertices[v] = gridVertex;
			triangleFrame.interpolateVertexData(triangle.vertices[v]);
			// Only really needed to interpolate u,v... replace normals and tangents with proper ones
			triangle.vertices[v].normal = gridVertex.normal;
			triangle.vertices[v].tangent = gridVertex.tangent;
			triangle.vertices[v].binormal = gridVertex.binormal;
		}
		triangle.extraDataIndex = frameIndex;
		triangle.smoothingMask = 0;
		triangle.submeshIndex = (int32_t)submeshIndex;

		return;
	}

	///////////////////////////////////////////////////////////////////////////

	physx::PxVec3 corner = m_tm.transform(physx::PxVec3(m_cornerX, m_cornerY, 0));
	const physx::PxVec3 localX = m_tm.column0.getXYZ() * m_xSpacing;
	const physx::PxVec3 localY = m_tm.column1.getXYZ() * m_ySpacing;
	const physx::PxVec3 localZ = m_tm.column2.getXYZ();

	// Vertices:
	m_vertices.resize((m_numX + 1) * (m_numY + 1));
	const physx::PxVec3 halfLocalX = 0.5f * localX;
	uint32_t pointN = 0;
	physx::PxVec3 side = corner;
	for (uint32_t iy = 0; iy <= m_numY; ++iy, side += localY)
	{
		physx::PxVec3 point = side;
		for (uint32_t ix = 0; ix <= m_numX; ++ix, point += localX)
		{
			if (m_pattern == nvidia::IntersectMesh::Equilateral && (((iy & 1) == 0 && ix == m_numX) || ((iy & 1) != 0 && ix == 1)))
			{
				point -= halfLocalX;
			}
			Vertex& vertex = m_vertices[pointN++];
			vertex.position = point;
			vertex.normal = physx::PxVec3(0.0f);
		}
	}

	// Build noise
	physx::Array<float>  f(m_vertices.size(), 0.);
	physx::Array<physx::PxVec3> n(m_vertices.size(), physx::PxVec3(0,0,0));
	buildNoise(f, &n, pattern, m_cornerX, m_cornerY, m_xSpacing, m_ySpacing, m_numX, m_numY, 
	           noiseAmplitude, relativeFrequency, xPeriod, yPeriod, noiseType, noiseDir);
	pointN = 0;
	for (uint32_t iy = 0; iy <= m_numY; ++iy)
	{
		for (uint32_t ix = 0; ix <= m_numX; ++ix, ++pointN)
		{
			Vertex& vertex = m_vertices[pointN];
			vertex.position += localZ * f[pointN];
			vertex.normal   += n[pointN];
		}
	}

	// Normalize normals and put in correct frame
	for (pointN = 0; pointN < m_vertices.size(); pointN++)
	{
		Vertex& vertex = m_vertices[pointN];
		vertex.normal.z = 1.0f;
		vertex.normal.normalize();
		vertex.normal = m_tm.rotate(vertex.normal);
		vertex.tangent = m_tm.column1.getXYZ().cross(vertex.normal);
		vertex.tangent.normalize();
		vertex.color = ColorRGBA(255, 255, 255, 255);
		vertex.binormal = vertex.normal.cross(vertex.tangent);
	}

	m_triangles.resize(2 * m_numX * m_numY);
	uint32_t triangleN = 0;
	uint32_t index = 0;
	const uint32_t tpattern[12] = { 0, m_numX + 2, m_numX + 1, 0, 1, m_numX + 2, 0, 1, m_numX + 1, 1, m_numX + 2, m_numX + 1 };
	for (uint32_t iy = 0; iy < m_numY; ++iy)
	{
		const uint32_t* yPattern = tpattern + (iy & 1) * 6;
		for (uint32_t ix = 0; ix < m_numX; ++ix, ++index)
		{
			const uint32_t* ytPattern = yPattern;
			for (uint32_t it = 0; it < 2; ++it, ytPattern += 3)
			{
				ExplicitRenderTriangle& triangle = m_triangles[triangleN++];
				for (uint32_t v = 0; v < 3; ++v)
				{
					Vertex& gridVertex = m_vertices[index + ytPattern[v]];
					triangle.vertices[v] = gridVertex;
					triangleFrame.interpolateVertexData(triangle.vertices[v]);
					// Only really needed to interpolate u,v... replace normals and tangents with proper ones
					triangle.vertices[v].normal = gridVertex.normal;
					triangle.vertices[v].tangent = gridVertex.tangent;
					triangle.vertices[v].binormal = gridVertex.binormal;
				}
				triangle.extraDataIndex = frameIndex;
				triangle.smoothingMask = 0;
				triangle.submeshIndex = (int32_t)submeshIndex;
			}
		}
		++index;
	}
}

static const int gSliceDirs[6][3] =
{
	{0, 1, 2},	// XYZ
	{1, 2, 0},	// YZX
	{2, 0, 1},	// ZXY
	{2, 1, 0},	// ZYX
	{1, 0, 2},	// YXZ
	{0, 2, 1}	// XZY
};

struct GridParameters
{
	GridParameters() :
		sizeScale(1.0f),
		xPeriod(0.0f),
		yPeriod(0.0f),
		interiorSubmeshIndex(0xFFFFFFFF),
		materialFrameIndex(0xFFFFFFFF),
		forceGrid(false)
	{
	}

	physx::Array< nvidia::ExplicitRenderTriangle >*	level0Mesh;
	float										sizeScale;
	nvidia::NoiseParameters								noise;
	float										xPeriod;
	float										yPeriod;
	uint32_t										interiorSubmeshIndex;
	uint32_t										materialFrameIndex;
	nvidia::TriangleFrame								triangleFrame;
	bool												forceGrid;
};


//////////////////////////////////////////////////////////////////////////////

static PX_INLINE uint32_t nearestPowerOf2(uint32_t v)
{
	v = v > 0 ? v - 1 : 0;

	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	++v;

	return v;
}

nvidia::DisplacementMapVolumeImpl::DisplacementMapVolumeImpl()
: width(0),
  height(0),
  depth(0)
{

}

void nvidia::DisplacementMapVolumeImpl::init(const FractureSliceDesc& desc)
{
	PX_UNUSED(desc);

	// Compute the number of slices for each plane
	uint32_t slices[3]; 
	slices[0] = slices[1] = slices[2] = 0;
	uint32_t maxGridSize = 0;
	for (uint32_t i = 0; i < 3; ++i)
	{
		for (uint32_t j = 1; j < desc.maxDepth; ++j)
		{
			if (slices[i] == 0) 
				slices[i]  = desc.sliceParameters[j].splitsPerPass[i];
			else
				slices[i] *= desc.sliceParameters[j].splitsPerPass[i];
		}
		for (uint32_t j = 0; j < desc.maxDepth; ++j)
		{
			if (desc.sliceParameters[j].noise[i].gridSize > (int)maxGridSize)
				maxGridSize = (uint32_t)desc.sliceParameters[j].noise[i].gridSize;
		}
	}

	width  = 4 * nearestPowerOf2(PxMax(maxGridSize, PxMax(slices[0], slices[1])));
	height = width;
	depth  = 4 * nearestPowerOf2(PxMax(maxGridSize, slices[2]));
}


void nvidia::DisplacementMapVolumeImpl::getData(uint32_t& w, uint32_t& h, uint32_t& d, uint32_t& size, unsigned char const** ppData) const
{
	PX_ASSERT(ppData);
	if(data.size() == 0) 
		buildData();

	w       = width;
	h       = height;
	d       = depth;
	size    = data.size();
	*ppData = data.begin();
}

template<typename T, typename U>
class Conversion
{
public:
	static PX_INLINE U convert(T x)
	{
		return (U)physx::PxClamp(x, (T)-1, (T)1);
	}
};

template<typename T>
class Conversion<T, unsigned char>
{
public:
	static PX_INLINE unsigned char convert(T x)
	{
		unsigned char value = (unsigned char)((physx::PxClamp(x, (T)-1, (T)1) + 1) * .5 * 255);
		return value;
	}
};

void nvidia::DisplacementMapVolumeImpl::buildData(physx::PxVec3 scale) const
{
	// For now, we forgo use of the scaling parameter
	PX_UNUSED(scale);

	const uint32_t numChannels = 4;  // ZYX -> BGRA
	const uint32_t channelSize = sizeof(unsigned char);
	const uint32_t stride      = numChannels * channelSize;
	const uint32_t size        = width * depth * height * stride;
	data.resize(size);

	const float dX = width  > 1 ? 1.0f/(width - 1) : 0.0f;
	const float dY = height > 1 ? 1.0f/(height- 1) : 0.0f;
	const float dZ = depth  > 1 ? 1.0f/(depth - 1) : 0.0f;

	uint32_t index  = 0;
	float z      = 0.0f;
	for (uint32_t i = 0; i < depth; ++i, z+=dZ)
	{
		float y = 0.0f;
		for (uint32_t j = 0; j < height; ++j, y+=dY)
		{
			float x = 0.0f;
			for (uint32_t k = 0; k < width; ++k, x+=dX, index+=stride)
			{
				const float xyz[] = {x, y ,z};
				const float yzx[] = {y, z, x};

				// Random offsets in x and y, with the z offset as a combination of the two
				//   As long as we're consistent here in our ordering, noise will be a smooth vector function of position
				float xOffset = userPerlin3D.sample(Vec3Float(xyz));
				float yOffset = userPerlin3D.sample(Vec3Float(yzx));
				float zOffset = (xOffset + yOffset) * 0.5f;

				// ZXY -> RGB
				data[index]   = Conversion<float, unsigned char>::convert(zOffset);
				data[index+1] = Conversion<float, unsigned char>::convert(yOffset);
				data[index+2] = Conversion<float, unsigned char>::convert(xOffset);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////

static void buildIntersectMesh(nvidia::IntersectMesh& mesh,
							   const physx::PxPlane& plane,
							   const nvidia::MaterialFrame& materialFrame,
							   int noiseType = FractureSliceDesc::NoiseWavePlane,
							   const GridParameters* gridParameters = NULL)
{
	if (!gridParameters)
	{
		mesh.build(plane);
		return;
	}

	PxMat44 tm = materialFrame.mCoordinateSystem;

	physx::PxBounds3 localPlaneBounds = boundTriangles(*gridParameters->level0Mesh, tm);

	const physx::PxVec3 diameter = localPlaneBounds.maximum - localPlaneBounds.minimum;
	const float planeDiameter = PxMax(diameter.x, diameter.y);
	// No longer fattening - the BSP does not have side boundaries, so we will not shave off any of the mesh.
	//	localPlaneBounds.fatten( 0.005f*planeDiameter );	// To ensure we get the whole mesh
	const float gridSpacing = planeDiameter / gridParameters->noise.gridSize;

	physx::PxVec3 center = localPlaneBounds.getCenter();
	physx::PxVec3 extent = localPlaneBounds.getExtents();

#if 0	// Equilateral
	const float offset = 0.5f;
	const float yRatio = 0.866025404f;
	const nvidia::IntersectMesh::GridPattern pattern = nvidia::IntersectMesh::Equilateral;
	const float xSpacing = gridSpacing;
	const float numX = physx::PxCeil(2 * extent.x / xSpacing + offset);
	const float cornerX = center.x - 0.5f * (numX - offset) * xSpacing;
	const float ySpacing = yRatio * gridSpacing;
	const float numY = physx::PxCeil(2 * extent.y / ySpacing);
	const float cornerY = center.y - 0.5f * numY * ySpacing;
#else	// Right
	const nvidia::IntersectMesh::GridPattern pattern = nvidia::IntersectMesh::Right;
	const float numX = gridParameters->xPeriod != 0.0f ? gridParameters->noise.gridSize : physx::PxCeil(2 * extent.x / gridSpacing);
	const float xSpacing = 2 * extent.x / numX;
	const float cornerX = center.x - extent.x;
	const float numY = gridParameters->yPeriod != 0.0f ? gridParameters->noise.gridSize : physx::PxCeil(2 * extent.y / gridSpacing);
	const float ySpacing = 2 * extent.y / numY;
	const float cornerY = center.y - extent.y;
#endif

	const float noiseAmplitude = gridParameters->sizeScale * gridParameters->noise.amplitude;

	mesh.build(pattern, plane, cornerX, cornerY, xSpacing, ySpacing, (uint32_t)numX, (uint32_t)numY, tm,
	           noiseAmplitude, gridParameters->noise.frequency, gridParameters->xPeriod, gridParameters->yPeriod, noiseType, -1,
	           gridParameters->interiorSubmeshIndex, gridParameters->materialFrameIndex, gridParameters->triangleFrame, gridParameters->forceGrid);
}

PX_INLINE physx::PxPlane createSlicePlane(const physx::PxVec3& center, const physx::PxVec3& extent, int sliceDir, int sliceDirNum,
                                        const float sliceWidths[3], const float linearNoise[3], const float angularNoise[3])
{
	// Orient the plane (+apply the angular noise) and compute the d parameter (+apply the linear noise)
	physx::PxVec3 slicePoint = center;
	slicePoint[(unsigned)sliceDir] += (sliceDirNum + 1) * sliceWidths[(unsigned)sliceDir] - extent[(unsigned)sliceDir];
	const physx::PxVec3 normal = randomNormal((uint32_t)sliceDir, angularNoise[(unsigned)sliceDir]);
	return physx::PxPlane(normal, -normal.dot(slicePoint) + sliceWidths[(unsigned)sliceDir] * linearNoise[(unsigned)sliceDir] * userRnd.getReal(-0.5f, 0.5f));
}

static void buildSliceBSP(ApexCSG::IApexBSP& sliceBSP, ExplicitHierarchicalMeshImpl& hMesh, const nvidia::NoiseParameters& noise,
                          const physx::PxVec3& extent, int sliceDir, int sliceDepth, const physx::PxPlane planes[3], 
						  const nvidia::FractureMaterialDesc& materialDesc, int noiseType = FractureSliceDesc::NoiseWavePlane, bool useDisplacementMaps = false)
{
	// Build grid and slice BSP
	nvidia::IntersectMesh grid;
	GridParameters gridParameters;
	gridParameters.interiorSubmeshIndex = materialDesc.interiorSubmeshIndex;
	// Defer noise generation if we're using displacement maps
	gridParameters.noise = useDisplacementMaps ? nvidia::NoiseParameters() : noise;
	gridParameters.level0Mesh = &hMesh.mParts[0]->mMesh;
	gridParameters.sizeScale = extent[(unsigned)sliceDir];
	gridParameters.materialFrameIndex = hMesh.addMaterialFrame();
	nvidia::MaterialFrame materialFrame = hMesh.getMaterialFrame(gridParameters.materialFrameIndex);
	materialFrame.buildCoordinateSystemFromMaterialDesc(materialDesc, planes[(unsigned)sliceDir]);
	materialFrame.mFractureMethod = nvidia::FractureMethod::Slice;
	materialFrame.mFractureIndex = sliceDir;
	materialFrame.mSliceDepth = (uint32_t)sliceDepth;
	hMesh.setMaterialFrame(gridParameters.materialFrameIndex, materialFrame);
	gridParameters.triangleFrame.setFlat(materialFrame.mCoordinateSystem, materialDesc.uvScale, materialDesc.uvOffset);
	buildIntersectMesh(grid, planes[(unsigned)sliceDir], materialFrame, noiseType, &gridParameters);
	
	ApexCSG::BSPTolerances bspTolerances = ApexCSG::gDefaultTolerances;
	bspTolerances.linear = 1.e-9f;
	bspTolerances.angular = 0.00001f;
	sliceBSP.setTolerances(bspTolerances);

	ApexCSG::BSPBuildParameters bspBuildParams = gDefaultBuildParameters;
	bspBuildParams.rnd = &userRnd;
	bspBuildParams.internalTransform = sliceBSP.getInternalTransform();

	if(useDisplacementMaps)
	{
		// Displacement map generation is deferred until the end of fracturing
		// This used to be where a slice would populate a displacement map with 
		//  offsets along the plane, but no longer
	}

	sliceBSP.fromMesh(&grid.m_triangles[0], grid.m_triangles.size(), bspBuildParams);
}

PX_INLINE ApexCSG::IApexBSP* createFractureBSP(physx::PxPlane slicePlanes[3], ApexCSG::IApexBSP*& sliceBSP, ApexCSG::IApexBSP& sourceBSP,
        ExplicitHierarchicalMeshImpl& hMesh, float& childVolume, float minVolume,
        const physx::PxVec3& center, const physx::PxVec3& extent, int sliceDir, int sliceDirNum, int sliceDepth,
        const float sliceWidths[3], const float linearNoise[3], const float angularNoise[3],
		const nvidia::NoiseParameters& noise, const nvidia::FractureMaterialDesc& materialDesc, int noiseType, bool useDisplacementMaps)
{
	const physx::PxPlane oldSlicePlane = slicePlanes[sliceDir];
	slicePlanes[sliceDir] = createSlicePlane(center, extent, sliceDir, sliceDirNum, sliceWidths, linearNoise, angularNoise);
	if (sliceBSP == NULL)
	{
		sliceBSP = createBSP(hMesh.mBSPMemCache, sourceBSP.getInternalTransform());
		buildSliceBSP(*sliceBSP, hMesh, noise, extent, sliceDir, sliceDepth, slicePlanes, materialDesc, noiseType, useDisplacementMaps);
	}
	sourceBSP.combine(*sliceBSP);
	ApexCSG::IApexBSP* bsp = createBSP(hMesh.mBSPMemCache, sourceBSP.getInternalTransform());
	bsp->op(sourceBSP, ApexCSG::Operation::Intersection);
#if 1	// Eliminating volume calculation here, for performance.  May introduce it later once the mesh is calculated.
	sourceBSP.op(sourceBSP, ApexCSG::Operation::A_Minus_B);
	if (minVolume <= 0 || (bsp->getType() != ApexCSG::BSPType::Empty_Set && sourceBSP.getType() != ApexCSG::BSPType::Empty_Set))
	{
		childVolume = 1.0f;
	}
	else
	{
		// We will ignore this slice
		if (sourceBSP.getType() != ApexCSG::BSPType::Empty_Set)
		{
			// chunk bsp volume too small
			slicePlanes[sliceDir] = oldSlicePlane;
			bsp->release();
			bsp = NULL;
			childVolume = 0.0f;
		}
		else
		{
			// remainder is too small.  Terminate slicing along this direction
			childVolume = 1.0f;
		}
	}
#else
	float bspArea, bspVolume;
	bsp->getSurfaceAreaAndVolume(bspArea, bspVolume, true);
	float remainingBSPArea, remainingBSPVolume;
	sourceBSP.getSurfaceAreaAndVolume(remainingBSPArea, remainingBSPVolume, true, ApexCSG::Operation::A_Minus_B);
	if (minVolume <= 0 || (bspVolume >= minVolume && remainingBSPVolume >= minVolume))
	{
		sourceBSP.op(sourceBSP, ApexCSG::Operation::A_Minus_B);
		childVolume = bspVolume;
	}
	else
	{
		// We will ignore this slice
		if (remainingBSPVolume >= minVolume)
		{
			// chunk bsp volume too small
			slicePlanes[sliceDir] = oldSlicePlane;
			bsp->release();
			bsp = NULL;
			sourceBSP.op(sourceBSP, ApexCSG::Operation::Set_A);
			childVolume = 0.0f;
		}
		else
		{
			// remainder is too small.  Terminate slicing along this direction
			bsp->op(sourceBSP, ApexCSG::Operation::Set_A);
			sourceBSP.op(sourceBSP, ApexCSG::Operation::Empty_Set);
			childVolume = bspVolume + remainingBSPVolume;
		}
	}
#endif
	return bsp;
}

static bool hierarchicallySplitChunkInternal
(
 ExplicitHierarchicalMeshImpl& hMesh,
 uint32_t chunkIndex,
 uint32_t relativeSliceDepth,
 physx::PxPlane chunkTrailingPlanes[3],
 physx::PxPlane chunkLeadingPlanes[3],
 const ApexCSG::IApexBSP& chunkBSP,
 float chunkVolume,
 const nvidia::FractureSliceDesc& desc,
 const CollisionDesc& collisionDesc,
 nvidia::IProgressListener& progressListener,
 volatile bool* cancel
 );

static bool createChunk
(
	ExplicitHierarchicalMeshImpl& hMesh,
	uint32_t chunkIndex,
	uint32_t relativeSliceDepth,
	physx::PxPlane trailingPlanes[3],
	physx::PxPlane leadingPlanes[3],
	float chunkVolume,
	const nvidia::FractureSliceDesc& desc,
	const ApexCSG::IApexBSP& parentBSP,
	const ApexCSG::IApexBSP& fractureBSP,
	const nvidia::SliceParameters& sliceParameters,
	const CollisionDesc& collisionDesc,
	nvidia::IProgressListener& progressListener,
	volatile bool* cancel
)
{
	bool canceled = false;
	ApexCSG::IApexBSP* chunkBSP = createBSP(hMesh.mBSPMemCache);
#if 0
	chunkBSP->copy(parentBSP);
	chunkBSP->combine(fractureBSP);
#else
	chunkBSP->copy(fractureBSP);
	chunkBSP->combine(parentBSP);
#endif
	chunkBSP->op(*chunkBSP, ApexCSG::Operation::Intersection);

	if (chunkBSP->getType() == ApexCSG::BSPType::Empty_Set)
	{
		return true;
	}

	if (gIslandGeneration)
	{
		chunkBSP = chunkBSP->decomposeIntoIslands();
	}

	CollisionVolumeDesc volumeDesc = getVolumeDesc(collisionDesc, hMesh.depth(chunkIndex)+1);

	const physx::PxVec3 minimumExtents = hMesh.chunkBounds(0).getExtents().multiply(physx::PxVec3(desc.minimumChunkSize[0], desc.minimumChunkSize[1], desc.minimumChunkSize[2]));

	while (chunkBSP != NULL)
	{
		if (!canceled)
		{
			// Create a mesh with chunkBSP (or its islands)
			const uint32_t newPartIndex = hMesh.addPart();
			const uint32_t newChunkIndex = hMesh.addChunk();
			chunkBSP->toMesh(hMesh.mParts[newPartIndex]->mMesh);
			hMesh.buildMeshBounds(newPartIndex);
			hMesh.buildCollisionGeometryForPart(newPartIndex, volumeDesc);
			hMesh.mChunks[newChunkIndex]->mParentIndex = (int32_t)chunkIndex;
			hMesh.mChunks[newChunkIndex]->mPartIndex = (int32_t)newPartIndex;
			if (hMesh.mParts[(uint32_t)hMesh.mChunks[chunkIndex]->mPartIndex]->mFlags & ExplicitHierarchicalMeshImpl::Part::MeshOpen)
			{
				hMesh.mParts[newPartIndex]->mFlags |= ExplicitHierarchicalMeshImpl::Part::MeshOpen;
			}
			// Trim hull in directions where splitting is noisy
			for (uint32_t i = 0; i < 3; ++i)
			{
				if ((sliceParameters.noise[i].amplitude != 0.0f || volumeDesc.mHullMethod != nvidia::ConvexHullMethod::WRAP_GRAPHICS_MESH) &&
					volumeDesc.mHullMethod != nvidia::ConvexHullMethod::CONVEX_DECOMPOSITION)
				{
					for (uint32_t hullIndex = 0; hullIndex < hMesh.mParts[newPartIndex]->mCollision.size(); ++hullIndex)
					{
						PartConvexHullProxy& hull = *hMesh.mParts[newPartIndex]->mCollision[hullIndex];
						float min, max;
						hull.impl.extent(min, max, trailingPlanes[i].n);
						if (max > min)
						{
							physx::PxPlane clipPlane = trailingPlanes[i];
							clipPlane.d = PxMin(clipPlane.d, -(0.8f * (max - min) + min));	// 20% clip bound
							hull.impl.intersectPlaneSide(clipPlane);
						}
						hull.impl.extent(min, max, leadingPlanes[i].n);
						if (max > min)
						{
							physx::PxPlane clipPlane = leadingPlanes[i];
							clipPlane.d = PxMin(clipPlane.d, -(0.8f * (max - min) + min));	// 20% clip bound
							hull.impl.intersectPlaneSide(clipPlane);
						}
					}
				}
			}
			if (hMesh.mParts[newPartIndex]->mMesh.size() > 0 && hMesh.mParts[newPartIndex]->mCollision.size() > 0 && // We have a mesh and collision hulls
				(hMesh.chunkBounds(newChunkIndex).getExtents() - minimumExtents).minElement() >= 0.0f)	// Chunk is large enough
			{
				// Proper chunk
				hMesh.mParts[newPartIndex]->mMeshBSP->copy(*chunkBSP);
				if (relativeSliceDepth < desc.maxDepth)
				{
					// Recurse
					canceled = !hierarchicallySplitChunkInternal(hMesh, newChunkIndex, relativeSliceDepth, trailingPlanes, leadingPlanes, *chunkBSP, chunkVolume, desc, collisionDesc, progressListener, cancel);
				}
			}
			else
			{
				// No mesh, no colision, or too small.  Eliminate.
				hMesh.removeChunk(newChunkIndex);
				hMesh.removePart(newPartIndex);
			}
		}
		if (chunkBSP == &parentBSP)
		{
			// No islands were generated; break from loop
			break;
		}
		// Get next bsp in island decomposition
		ApexCSG::IApexBSP* nextBSP = chunkBSP->getNext();
		chunkBSP->release();
		chunkBSP = nextBSP;
	}

	return !canceled;
}

static bool hierarchicallySplitChunkInternal
(
    ExplicitHierarchicalMeshImpl& hMesh,
    uint32_t chunkIndex,
    uint32_t relativeSliceDepth,
    physx::PxPlane chunkTrailingPlanes[3],
    physx::PxPlane chunkLeadingPlanes[3],
    const ApexCSG::IApexBSP& chunkBSP,
    float chunkVolume,
    const nvidia::FractureSliceDesc& desc,
	const CollisionDesc& collisionDesc,
    nvidia::IProgressListener& progressListener,
    volatile bool* cancel
)
{
	if (relativeSliceDepth >= desc.maxDepth)
	{
		return true;	// No slice parameters at this depth
	}

	const physx::PxBounds3 bounds = hMesh.chunkBounds(chunkIndex);

	if (chunkIndex >= hMesh.chunkCount() || bounds.isEmpty())
	{
		return true;	// Done, nothing in chunk
	}

	bool canceled = false;	// our own copy of *cancel

	physx::PxVec3 center = bounds.getCenter();
	physx::PxVec3 extent = bounds.getExtents();

	if (relativeSliceDepth == 0)
	{
		chunkTrailingPlanes[0] = physx::PxPlane(-1, 0, 0, bounds.minimum[0]);
		chunkTrailingPlanes[1] = physx::PxPlane(0, -1, 0, bounds.minimum[1]);
		chunkTrailingPlanes[2] = physx::PxPlane(0, 0, -1, bounds.minimum[2]);
		chunkLeadingPlanes[0] = physx::PxPlane(1, 0, 0, -bounds.maximum[0]);
		chunkLeadingPlanes[1] = physx::PxPlane(0, 1, 0, -bounds.maximum[1]);
		chunkLeadingPlanes[2] = physx::PxPlane(0, 0, 1, -bounds.maximum[2]);
	}

	// Get parameters for this depth
	const nvidia::SliceParameters& sliceParameters = desc.sliceParameters[relativeSliceDepth++];

	// Determine slicing at this level
	int partition[3];
	calculatePartition(partition, sliceParameters.splitsPerPass, extent, desc.useTargetProportions ? desc.targetProportions : NULL);

	// Slice volume rejection ratio, perhaps should be exposed
	const float volumeRejectionRatio = 0.1f;
	// Resulting slices must have at least this volume
	const float minChildVolume = volumeRejectionRatio * chunkVolume / (partition[0] * partition[1] * partition[2]);

	const bool slicingThrough = sliceParameters.order >= 6;

	const uint32_t sliceDirOrder = slicingThrough ? 0u : (uint32_t)sliceParameters.order;
	const uint32_t sliceDir0 = (uint32_t)gSliceDirs[sliceDirOrder][0];
	const uint32_t sliceDir1 = (uint32_t)gSliceDirs[sliceDirOrder][1];
	const uint32_t sliceDir2 = (uint32_t)gSliceDirs[sliceDirOrder][2];
	const float sliceWidths[3] = { 2.0f * extent[0] / partition[0], 2.0f * extent[1] / partition[1], 2.0f * extent[2] / partition[2] };

	nvidia::HierarchicalProgressListener localProgressListener(PxMax(partition[0]*partition[1]*partition[2], 1), &progressListener);

	// If we are slicing through, then we need to cache the slice BSPs in the 2nd and 3rd directions
	physx::Array<ApexCSG::IApexBSP*> sliceBSPs1;
	physx::Array<ApexCSG::IApexBSP*> sliceBSPs2;
	if (slicingThrough)
	{
		sliceBSPs1.resize((uint32_t)partition[(uint32_t)gSliceDirs[sliceDirOrder][1]] - 1, NULL);
		sliceBSPs2.resize((uint32_t)partition[(uint32_t)gSliceDirs[sliceDirOrder][2]] - 1, NULL);
	}

	// If we are not slicingb through, we can re-use this sliceBSP
	ApexCSG::IApexBSP* reusedSliceBSP = NULL;

	physx::PxPlane trailingPlanes[3];
	physx::PxPlane leadingPlanes[3];

	float childVolume = 0.0f;

	ApexCSG::IApexBSP* fractureBSP0 = createBSP(hMesh.mBSPMemCache, chunkBSP.getInternalTransform());

	const int sliceDepth = (int)hMesh.depth(chunkIndex) + 1;

	trailingPlanes[sliceDir0] = chunkTrailingPlanes[sliceDir0];
	leadingPlanes[sliceDir0] = physx::PxPlane(-trailingPlanes[sliceDir0].n, -trailingPlanes[sliceDir0].d);
	for (int sliceDir0Num = 0; sliceDir0Num < partition[sliceDir0] && !canceled; ++sliceDir0Num)
	{
		ApexCSG::IApexBSP* fractureBSP1 = fractureBSP0;	// This is the default; if there is a need to slice it will be replaced below.
		if (sliceDir0Num + 1 < partition[sliceDir0])
		{
			// Slice off piece in the 0 direction
			fractureBSP1 = createFractureBSP(leadingPlanes, reusedSliceBSP, *fractureBSP0, hMesh, childVolume, 0, center, extent, (int32_t)sliceDir0, sliceDir0Num, sliceDepth, sliceWidths,
			                                 sliceParameters.linearVariation, sliceParameters.angularVariation, sliceParameters.noise[sliceDir0],
											 desc.materialDesc[sliceDir0], (int32_t)desc.noiseMode, desc.useDisplacementMaps);
			reusedSliceBSP->release();
			reusedSliceBSP = NULL;
		}
		else
		{
			leadingPlanes[sliceDir0] = chunkLeadingPlanes[sliceDir0];
		}
		trailingPlanes[sliceDir1] = chunkTrailingPlanes[sliceDir1];
		leadingPlanes[sliceDir1] = physx::PxPlane(-trailingPlanes[sliceDir1].n, -trailingPlanes[sliceDir1].d);
		for (int sliceDir1Num = 0; sliceDir1Num < partition[sliceDir1] && !canceled; ++sliceDir1Num)
		{
			ApexCSG::IApexBSP* fractureBSP2 = fractureBSP1;	// This is the default; if there is a need to slice it will be replaced below.
			if (sliceDir1Num + 1 < partition[sliceDir1])
			{
				// Slice off piece in the 1 direction
				ApexCSG::IApexBSP*& sliceBSP = !slicingThrough ? reusedSliceBSP : sliceBSPs1[(uint32_t)sliceDir1Num];
				fractureBSP2 = createFractureBSP(leadingPlanes, sliceBSP, *fractureBSP1, hMesh, childVolume, 0, center, extent, (int32_t)sliceDir1, sliceDir1Num, sliceDepth,
				                                 sliceWidths, sliceParameters.linearVariation, sliceParameters.angularVariation, sliceParameters.noise[sliceDir1],
												 desc.materialDesc[sliceDir1], (int32_t)desc.noiseMode, desc.useDisplacementMaps);
				if (sliceBSP == reusedSliceBSP)
				{
					reusedSliceBSP->release();
					reusedSliceBSP = NULL;
				}
			}
			else
			{
				leadingPlanes[sliceDir1] = chunkLeadingPlanes[sliceDir1];
			}
			trailingPlanes[sliceDir2] = chunkTrailingPlanes[sliceDir2];
			leadingPlanes[sliceDir2] = physx::PxPlane(-trailingPlanes[sliceDir2].n, -trailingPlanes[sliceDir2].d);
			for (int sliceDir2Num = 0; sliceDir2Num < partition[sliceDir2] && !canceled; ++sliceDir2Num)
			{
				ApexCSG::IApexBSP* fractureBSP3 = fractureBSP2;	// This is the default; if there is a need to slice it will be replaced below.
				if (sliceDir2Num + 1 < partition[sliceDir2])
				{
					// Slice off piece in the 2 direction
					ApexCSG::IApexBSP*& sliceBSP = !slicingThrough ? reusedSliceBSP : sliceBSPs2[(uint32_t)sliceDir2Num];
					fractureBSP3 = createFractureBSP(leadingPlanes, sliceBSP, *fractureBSP2, hMesh, childVolume, minChildVolume, center, extent, (int32_t)sliceDir2, sliceDir2Num, sliceDepth,
					                                 sliceWidths, sliceParameters.linearVariation, sliceParameters.angularVariation, sliceParameters.noise[sliceDir2],
													 desc.materialDesc[sliceDir2], (int32_t)desc.noiseMode, desc.useDisplacementMaps);
					if (sliceBSP == reusedSliceBSP)
					{
						reusedSliceBSP->release();
						reusedSliceBSP = NULL;
					}
				}
				else
				{
					leadingPlanes[sliceDir2] = chunkLeadingPlanes[sliceDir2];
				}
				if (fractureBSP3 != NULL)
				{
					if (hMesh.mParts[(uint32_t)hMesh.mChunks[chunkIndex]->mPartIndex]->mFlags & ExplicitHierarchicalMeshImpl::Part::MeshOpen)
					{
						fractureBSP3->deleteTriangles();	// Don't use interior triangles on an open mesh
					}
					canceled = !createChunk(hMesh, chunkIndex, relativeSliceDepth, trailingPlanes, leadingPlanes, childVolume, desc, chunkBSP, *fractureBSP3, sliceParameters, collisionDesc, localProgressListener, cancel);
				}
				localProgressListener.completeSubtask();
				// We no longer need fractureBSP3
				if (fractureBSP3 != NULL && fractureBSP3 != fractureBSP2)
				{
					fractureBSP3->release();
					fractureBSP3 = NULL;
				}
				trailingPlanes[sliceDir2] = physx::PxPlane(-leadingPlanes[sliceDir2].n, -leadingPlanes[sliceDir2].d);
				// Check for cancellation
				if (cancel != NULL && *cancel)
				{
					canceled = true;
				}
			}
			// We no longer need fractureBSP2
			if (fractureBSP2 != NULL && fractureBSP2 != fractureBSP1)
			{
				fractureBSP2->release();
				fractureBSP2 = NULL;
			}
			trailingPlanes[sliceDir1] = physx::PxPlane(-leadingPlanes[sliceDir1].n, -leadingPlanes[sliceDir1].d);
			// Check for cancellation
			if (cancel != NULL && *cancel)
			{
				canceled = true;
			}
		}
		// We no longer need fractureBSP1
		if (fractureBSP1 != NULL && fractureBSP1 != fractureBSP0)
		{
			fractureBSP1->release();
			fractureBSP1 = NULL;
		}
		trailingPlanes[sliceDir0] = physx::PxPlane(-leadingPlanes[sliceDir0].n, -leadingPlanes[sliceDir0].d);
		// Check for cancellation
		if (cancel != NULL && *cancel)
		{
			canceled = true;
		}
	}
	fractureBSP0->release();

	while (sliceBSPs2.size())
	{
		if (sliceBSPs2.back() != NULL)
		{
			sliceBSPs2.back()->release();
		}
		sliceBSPs2.popBack();
	}
	while (sliceBSPs1.size())
	{
		if (sliceBSPs1.back() != NULL)
		{
			sliceBSPs1.back()->release();
		}
		sliceBSPs1.popBack();
	}

	return !canceled;
}


struct TriangleLockInfo
{
	TriangleLockInfo() : lockedVertices(0), lockedEdges(0), originalTriangleIndex(0xFFFFFFFF) {}

	uint16_t lockedVertices;	// (lockedVertices>>N)&1 => vertex N is locked
	uint16_t lockedEdges;		// (lockedEdges>>M)&1 => edge M is locked
	uint32_t originalTriangleIndex;
};

PX_INLINE float square(float x)
{
	return x*x;
}

// Returns edge of triangle, and position on edge (in pointOnEdge) if an edge is split, otherwise returns -1
// If a valid edge index is returned, also returns distance squared from the point to the edge in perp2
PX_INLINE int32_t pointOnAnEdge(physx::PxVec3& pointOnEdge, float& perp2, const physx::PxVec3& point, const nvidia::ExplicitRenderTriangle& triangle, float paraTol2, float perpTol2)
{
	int32_t edgeIndex = -1;
	float closestPerp2E2 = 1.0f;
	float closestE2 = 0.0f;

	for (uint32_t i = 0; i < 3; ++i)
	{
		const physx::PxVec3& v0 = triangle.vertices[i].position;
		const physx::PxVec3& v1 = triangle.vertices[(i+1)%3].position;
		const physx::PxVec3 e = v1 - v0;
		const float e2 = e.magnitudeSquared();
		const float perpTol2e2 = perpTol2*e2;
		const physx::PxVec3 d0 = point - v0;
		const float d02 = d0.magnitudeSquared();
		if (d02 < paraTol2)
		{
			return -1;
		}
		if (e2 <= 4.0f*paraTol2)
		{
			continue;
		}
		const float d0e = d0.dot(e);
		if (d0e < 0.0f || d0e > e2)
		{
			continue;	// point does not project down onto the edge
		}
		const float perp2e2 = d0.cross(e).magnitudeSquared();
		if (perp2e2 > perpTol2e2)
		{
			continue;	// Point too far from edge
		}
		// Point is close to an edge.  Consider it if it's the closest.
		if (perp2e2*closestE2 < closestPerp2E2*e2)
		{
			closestPerp2E2 = perp2e2;
			closestE2 = e2;
			edgeIndex = (int32_t)i;
		}
	}

	if (edgeIndex < 0 || closestE2 == 0.0f)
	{
		return -1;
	}

	const physx::PxVec3& v0 = triangle.vertices[edgeIndex].position;
	const physx::PxVec3& v1 = triangle.vertices[(edgeIndex+1)%3].position;

	if ((point-v0).magnitudeSquared() < paraTol2 || (point-v1).magnitudeSquared() < paraTol2)
	{
		return -1;
	}

	pointOnEdge = point;
	perp2 = closestE2 > 0.0f ? closestPerp2E2/closestE2 : 0.0f;

	return edgeIndex;
}

// Returns shared edge of triangleA if an edge is shared, otherwise returns -1
PX_INLINE int32_t trianglesShareEdge(const nvidia::ExplicitRenderTriangle& triangleA, const nvidia::ExplicitRenderTriangle& triangleB, float tol2)
{
	for (uint32_t i = 0; i < 3; ++i)
	{
		const physx::PxVec3 eA = triangleA.vertices[(i+1)%3].position - triangleA.vertices[i].position;
		const float eA2 = eA.magnitudeSquared();
		const float tol2eA2 = tol2*eA2;
		// We will search for edges pointing in the opposite direction only
		for (uint32_t j = 0; j < 3; ++j)
		{
			const physx::PxVec3 d0 = triangleB.vertices[j].position - triangleA.vertices[i].position;
			const float d0A = d0.dot(eA);
			if (d0A <= 0.0f)
			{
				continue;	// edge on B starts before edge on A
			}
			const physx::PxVec3 d1 = triangleB.vertices[(j+1)%3].position - triangleA.vertices[i].position;
			const float d1A = d1.dot(eA);
			if (d1A >= eA2)
			{
				continue;	// edge on B ends after edge on A
			}
			if (d0A <= d1A)
			{
				continue;	// edges don't point in opposite directions
			}
			if (d0.cross(eA).magnitudeSquared() > tol2eA2)
			{
				continue;	// one vertex on B is not close enough to the edge of A
			}
			if (d1.cross(eA).magnitudeSquared() > tol2eA2)
			{
				continue;	// other vertex on B is not close enough to the edge of A
			}
			// These edges appear to have an overlap, to within tolerance
			return (int32_t)i;
		}
	}

	return -1;
}

// Positive tol means that interference will be registered even if the triangles are a small distance apart.
// Negative tol means that interference will not be registered even if the triangles have a small overlap.
PX_INLINE bool trianglesInterfere(const nvidia::ExplicitRenderTriangle& triangleA, const nvidia::ExplicitRenderTriangle& triangleB, float tol)
{
	// Check extent of B relative to plane of A
	const physx::PxPlane planeA(0.333333333f*(triangleA.vertices[0].position + triangleA.vertices[1].position + triangleA.vertices[2].position), triangleA.calculateNormal().getNormalized());
	physx::PxVec3 dispB(planeA.distance(triangleB.vertices[0].position), planeA.distance(triangleB.vertices[1].position), planeA.distance(triangleB.vertices[2].position));
	if (extentDistance(dispB.minElement(), dispB.maxElement(), 0.0f, 0.0f) > tol)
	{
		return false;
	}

	// Check extent of A relative to plane of B
	const physx::PxPlane planeB(0.333333333f*(triangleB.vertices[0].position + triangleB.vertices[1].position + triangleB.vertices[2].position), triangleB.calculateNormal().getNormalized());
	physx::PxVec3 dispA(planeB.distance(triangleA.vertices[0].position), planeB.distance(triangleA.vertices[1].position), planeB.distance(triangleA.vertices[2].position));
	if (extentDistance(dispA.minElement(), dispA.maxElement(), 0.0f, 0.0f) > tol)
	{
		return false;
	}

	for (uint32_t i = 0; i < 3; ++i)
	{
		physx::PxVec3 eA = triangleA.vertices[(i+1)%3].position - triangleA.vertices[i].position;
		eA.normalize();
		for (uint32_t j = 0; j < 3; ++j)
		{
			physx::PxVec3 eB = triangleB.vertices[(j+1)%3].position - triangleB.vertices[j].position;
			eB.normalize();
			physx::PxVec3 n = eA.cross(eB);
			if (n.normalize() > 0.00001f)
			{
				dispA = physx::PxVec3(n.dot(triangleA.vertices[0].position), n.dot(triangleA.vertices[1].position), n.dot(triangleA.vertices[2].position));
				dispB = physx::PxVec3(n.dot(triangleB.vertices[0].position), n.dot(triangleB.vertices[1].position), n.dot(triangleB.vertices[2].position));
				if (extentDistance(dispA.minElement(), dispA.maxElement(), dispB.minElement(), dispB.maxElement()) > tol)
				{
					return false;
				}
			}
		}
	}

	return true;
}

PX_INLINE bool segmentIntersectsTriangle(const physx::PxVec3 orig, const physx::PxVec3 dest, const nvidia::ExplicitRenderTriangle& triangle, float tol)
{
	// Check extent of segment relative to plane of triangle
	const physx::PxPlane plane(0.333333333f*(triangle.vertices[0].position + triangle.vertices[1].position + triangle.vertices[2].position), triangle.calculateNormal().getNormalized());
	const float dist0 = plane.distance(orig);
	const float dist1 = plane.distance(dest);
	if (extentDistance(PxMin(dist0, dist1), PxMax(dist0, dist1), 0.0f, 0.0f) > tol)
	{
		return false;
	}

	// Test to see if the segment goes through the triangle
	const float signDist0 = physx::PxSign(dist0);
	const physx::PxVec3 disp = dest-orig;
	const physx::PxVec3 relV[3] = {triangle.vertices[0].position - orig, triangle.vertices[1].position - orig, triangle.vertices[2].position - orig};
	for (uint32_t v = 0; v < 3; ++v)
	{
		if (physx::PxSign(relV[v].cross(relV[(v+1)%3]).dot(disp)) == signDist0)
		{
			return false;
		}
	}

	return true;
}

struct VertexRep
{
	BoundsRep		bounds;
	physx::PxVec3	position;	// Not necessarily the center of bounds, after we snap vertices
	physx::PxVec3	normal;
};

class MeshProcessor
{
public:
	class FreeVertexIt
	{
	public:
		FreeVertexIt(MeshProcessor& meshProcessor) : mMeshProcessor(meshProcessor), mVertexRep(NULL)
		{
			mTriangleIndex = meshProcessor.mTrianglePartition;
			mVertexIndex = 0;
			advanceToNextValidState();
		}

		PX_INLINE bool valid() const
		{
			return mTriangleIndex < mMeshProcessor.mMesh->size();
		}

		PX_INLINE void inc()
		{
			if (valid())
			{
				++mVertexIndex;
				advanceToNextValidState();
			}
		}

		PX_INLINE uint32_t triangleIndex() const
		{
			return mTriangleIndex;
		}

		PX_INLINE uint32_t vertexIndex() const
		{
			return mVertexIndex;
		}

		PX_INLINE VertexRep& vertexRep() const
		{
			return *mVertexRep;
		}

	private:
		FreeVertexIt& operator=(const FreeVertexIt&);

		PX_INLINE void advanceToNextValidState()
		{
			for (; valid(); ++mTriangleIndex, mVertexIndex = 0)
			{
				for (; mVertexIndex < 3; ++mVertexIndex)
				{
					const uint32_t relativeTriangleIndex = mTriangleIndex-mMeshProcessor.mTrianglePartition;
					if (!((mMeshProcessor.mTriangleInfo[relativeTriangleIndex].lockedVertices >> mVertexIndex)&1))
					{
						mVertexRep = &mMeshProcessor.mVertexBounds[3*relativeTriangleIndex + mVertexIndex];
						return;
					}
				}
			}
		}

		MeshProcessor&	mMeshProcessor;
		uint32_t	mTriangleIndex;
		uint32_t	mVertexIndex;
		VertexRep*		mVertexRep;
	};

	class FreeNeighborVertexIt
	{
	public:
		FreeNeighborVertexIt(MeshProcessor& meshProcessor, uint32_t triangleIndex, uint32_t vertexIndex) 
			: mMeshProcessor(meshProcessor)
			, mTriangleIndex(triangleIndex)
			, mVertexIndex(vertexIndex)
			, mVertexRep(NULL)
		{
			const uint32_t vertexRepIndex = 3*(triangleIndex - mMeshProcessor.mTrianglePartition) + vertexIndex;
			mNeighbors = meshProcessor.mVertexNeighborhoods.getNeighbors(vertexRepIndex);
			mNeighborStop = mNeighbors + meshProcessor.mVertexNeighborhoods.getNeighborCount(vertexRepIndex);
			advanceToNextValidState();
		}

		PX_INLINE bool valid() const
		{
			return mNeighbors < mNeighborStop;
		}

		PX_INLINE void inc()
		{
			if (valid())
			{
				++mNeighbors;
				advanceToNextValidState();
			}
		}

		PX_INLINE uint32_t triangleIndex() const
		{
			return mTriangleIndex;
		}

		PX_INLINE uint32_t vertexIndex() const
		{
			return mVertexIndex;
		}

		PX_INLINE VertexRep& vertexRep() const
		{
			return *mVertexRep;
		}

	private:
		FreeNeighborVertexIt& operator=(const FreeNeighborVertexIt&);

		PX_INLINE void advanceToNextValidState()
		{
			for (; valid(); ++mNeighbors)
			{
				const uint32_t neighbor = *mNeighbors;
				const uint32_t relativeTriangleIndex = neighbor/3;
				mTriangleIndex = mMeshProcessor.mTrianglePartition + relativeTriangleIndex;
				mVertexIndex = neighbor - 3*relativeTriangleIndex;
				mVertexRep = &mMeshProcessor.mVertexBounds[neighbor];
				if (!((mMeshProcessor.mTriangleInfo[relativeTriangleIndex].lockedVertices >> mVertexIndex)&1))
				{
					return;
				}
			}
		}

		MeshProcessor&		mMeshProcessor;
		const uint32_t*	mNeighbors;
		const uint32_t*	mNeighborStop;
		uint32_t		mTriangleIndex;
		uint32_t		mVertexIndex;
		VertexRep*			mVertexRep;
	};

	MeshProcessor()
	{
		reset();
	}

	// Removes any triangles with a width less than minWidth
	static void removeSlivers(physx::Array<nvidia::ExplicitRenderTriangle>& mesh, float minWidth)
	{
		const float minWidth2 = minWidth*minWidth;
		for (uint32_t i = mesh.size(); i--;)
		{
			nvidia::ExplicitRenderTriangle& triangle = mesh[i];
			for (uint32_t j = 0; j < 3; ++j)
			{
				const physx::PxVec3 edge = (triangle.vertices[(j+1)%3].position - triangle.vertices[j].position).getNormalized();
				if ((triangle.vertices[(j+2)%3].position - triangle.vertices[j].position).cross(edge).magnitudeSquared() < minWidth2)
				{
					mesh.replaceWithLast(i);
					break;
				}
			}
		}
	}

	// trianglePartition is a point in the part mesh where we want to start processing.  We will assume that triangles
	// before this index are locked, and vertices in triangles after this index will be locked if they coincide with
	// locked triangles
	void setMesh(physx::Array<nvidia::ExplicitRenderTriangle>& mesh, physx::Array<nvidia::ExplicitRenderTriangle>* parentMesh, uint32_t trianglePartition, float tol)
	{
		reset();

		if (mesh.size() == 0)
		{
			return;
		}

		mMesh = &mesh;
		mTrianglePartition = PxMin(trianglePartition, mesh.size());
		mTol = physx::PxAbs(tol);
		mPadding = 2*mTol;

		mParentMesh = parentMesh;

		// Find part triangle neighborhoods, expanding triangle bounds by some padding factor
		mOriginalTriangleBounds.resize(mesh.size());
		for (uint32_t i = 0; i < mesh.size(); ++i)
		{
			nvidia::ExplicitRenderTriangle& triangle = mesh[i];
			mOriginalTriangleBounds[i].aabb.setEmpty();
			mOriginalTriangleBounds[i].aabb.include(triangle.vertices[0].position);
			mOriginalTriangleBounds[i].aabb.include(triangle.vertices[1].position);
			mOriginalTriangleBounds[i].aabb.include(triangle.vertices[2].position);
			mOriginalTriangleBounds[i].aabb.fattenFast(mPadding);
		}
		mOriginalTriangleNeighborhoods.setBounds(&mOriginalTriangleBounds[0], mOriginalTriangleBounds.size(), sizeof(mOriginalTriangleBounds[0]));

		// Create additional triangle info.  This will parallel the mesh after the partition
		mTriangleInfo.resize(mesh.size()-mTrianglePartition);
		// Also create triangle interpolators from the original triangles
		mTriangleFrames.resize(mesh.size()-mTrianglePartition);
		// As well as child triangle info
		mTriangleChildLists.resize(mesh.size()-mTrianglePartition);
		for (uint32_t i = mTrianglePartition; i < mesh.size(); ++i)
		{
			mTriangleInfo[i-mTrianglePartition].originalTriangleIndex = i;	// Use the original triangles' neighborhood info
			mTriangleFrames[i-mTrianglePartition].setFromTriangle(mesh[i]);
			mTriangleChildLists[i-mTrianglePartition].pushBack(i);	// These are all of the triangles that use the corresponding triangle frame, and will be represented by the corresponding bounds for spatial lookup
		}
	}

	void lockBorderVertices()
	{
		physx::Array<nvidia::ExplicitRenderTriangle>& mesh = *mMesh;

		const float tol2 = mTol*mTol;

		for (uint32_t i = mTrianglePartition; i < mesh.size(); ++i)
		{
			// Use neighbor info to find out if we should lock any of the vertices
			nvidia::ExplicitRenderTriangle& triangle = mesh[i];
			const uint32_t neighborCount = mOriginalTriangleNeighborhoods.getNeighborCount(i);
			const uint32_t* neighbors = mOriginalTriangleNeighborhoods.getNeighbors(i);
			for (uint32_t j = 0; j < neighborCount; ++j)
			{
				const uint32_t neighbor = *neighbors++;
				if (neighbor < mTrianglePartition)
				{	
					// Neighbor is not a process triangle - if it shares an edge then lock vertices
					const int32_t sharedEdge = trianglesShareEdge(triangle, mesh[neighbor], tol2);
					if (sharedEdge >= 0)
					{
						mTriangleInfo[i-mTrianglePartition].lockedVertices |= (1<<sharedEdge) | (1<<((sharedEdge+1)%3));
						mTriangleInfo[i-mTrianglePartition].lockedEdges |= 1<<sharedEdge;
					}
					// Check triangle vertices against neighbor's edges as well
					for (uint32_t v = 0; v < 3; ++v)
					{
						const physx::PxVec3& point = triangle.vertices[v].position;
						physx::PxVec3 pointOnEdge;
						float perp2;
						if (0 <= pointOnAnEdge(pointOnEdge, perp2, point, mesh[neighbor], 0.0f, tol2))
						{
							mTriangleInfo[i-mTrianglePartition].lockedVertices |= 1<<v;
						}
					}
				}
			}
		}
	}

	void removeTJunctions()
	{
		physx::Array<nvidia::ExplicitRenderTriangle>& mesh = *mMesh;

		const float tol2 = mTol*mTol;

		const uint32_t originalMeshSize = mesh.size();
		for (uint32_t i = mTrianglePartition; i < originalMeshSize; ++i)
		{
			const uint32_t neighborCount = mOriginalTriangleNeighborhoods.getNeighborCount(i);
			for (uint32_t v = 0; v < 3; ++v)
			{
				const physx::PxVec3& point = mesh[i].vertices[v].position;
				int32_t neighborToSplit = -1;
				int32_t edgeToSplit = -1;
				float leastPerp2 = PX_MAX_F32;
				const uint32_t* neighbors = mOriginalTriangleNeighborhoods.getNeighbors(i);
				for (uint32_t j = 0; j < neighborCount; ++j)
				{
					const uint32_t originalNeighbor = *neighbors++;
					if (originalNeighbor >= mTrianglePartition)
					{
						physx::Array<uint32_t>& triangleChildList = mTriangleChildLists[originalNeighbor - mTrianglePartition];
						const uint32_t triangleChildListSize = triangleChildList.size();
						for (uint32_t k = 0; k < triangleChildListSize; ++k)
						{
							const uint32_t neighbor = triangleChildList[k];
							// Neighbor is a process triangle - split it at this triangle's vertices
							const physx::PxVec3& point = mesh[i].vertices[v].position;
							physx::PxVec3 pointOnEdge;
							float perp2 = 0.0f;
							const int32_t edge = pointOnAnEdge(pointOnEdge, perp2, point, mesh[neighbor], tol2, tol2);
							if (edge >= 0 && !((mTriangleInfo[neighbor - mTrianglePartition].lockedEdges >> edge)&1))
							{
								if (perp2 < leastPerp2)
								{
									neighborToSplit = (int32_t)neighbor;
									edgeToSplit = edge;
									leastPerp2 = perp2;
								}
							}
						}
					}
				}
				if (neighborToSplit >= 0 && edgeToSplit >= 0)
				{
					splitTriangle((uint32_t)neighborToSplit, (uint32_t)edgeToSplit, point);
				}
			}
		}
	}

	void subdivide(float maxEdgeLength)
	{
		physx::Array<nvidia::ExplicitRenderTriangle>& mesh = *mMesh;
		const float maxEdgeLength2 = maxEdgeLength*maxEdgeLength;

		const float tol2 = mTol*mTol;

		// Pass through list and split edges that are too long
		bool splitDone;
		do
		{
			splitDone = false;
			for (uint32_t i = mTrianglePartition; i < mesh.size(); ++i)	// this array (as well as info) might grow during the loop
			{
				nvidia::ExplicitRenderTriangle& triangle = mesh[i];
				// Find the longest edge of this triangle
				const float edgeLengthSquared[3] =
				{
					(triangle.vertices[1].position - triangle.vertices[0].position).magnitudeSquared(),
					(triangle.vertices[2].position - triangle.vertices[1].position).magnitudeSquared(),
					(triangle.vertices[0].position - triangle.vertices[2].position).magnitudeSquared()
				};
				const int longestEdge = edgeLengthSquared[1] > edgeLengthSquared[0] ? (edgeLengthSquared[2] > edgeLengthSquared[1] ? 2 : 1) : (edgeLengthSquared[2] > edgeLengthSquared[0] ? 2 : 0);
				if (edgeLengthSquared[longestEdge] > maxEdgeLength2)
				{
					// Split this edge
					const nvidia::ExplicitRenderTriangle oldTriangle = triangle;	// Save off old triangle for neighbor edge check
					const physx::PxVec3 newVertexPosition = 0.5f*(triangle.vertices[longestEdge].position + triangle.vertices[(longestEdge + 1)%3].position);
					splitTriangle(i, (uint32_t)longestEdge, newVertexPosition);
					// Now split neighbor edges
					const uint32_t neighborCount = mOriginalTriangleNeighborhoods.getNeighborCount(i);
					const uint32_t* neighbors = mOriginalTriangleNeighborhoods.getNeighbors(i);
					for (uint32_t j = 0; j < neighborCount; ++j)
					{
						const uint32_t originalNeighbor = *neighbors++;
						if (originalNeighbor >= mTrianglePartition)
						{
							physx::Array<uint32_t>& triangleChildList = mTriangleChildLists[originalNeighbor - mTrianglePartition];
							for (uint32_t k = 0; k < triangleChildList.size(); ++k)
							{
								const uint32_t neighbor = triangleChildList[k];
								if (neighbor >= mTrianglePartition)
								{	
									// Neighbor is a process triangle - split it too, if the neighbor shares an edge, and the split point is on the shared edge
									const int32_t sharedEdge = trianglesShareEdge(oldTriangle, mesh[neighbor], tol2);
									if (sharedEdge >= 0)
									{
										physx::PxVec3 pointOnEdge;
										float perp2;
										const int32_t edgeToSplit = pointOnAnEdge(pointOnEdge, perp2, newVertexPosition, mesh[neighbor], tol2, tol2);
										if (edgeToSplit == sharedEdge && !((mTriangleInfo[neighbor - mTrianglePartition].lockedEdges >> edgeToSplit)&1))
										{
											splitTriangle(neighbor, (uint32_t)edgeToSplit, pointOnEdge);
										}
									}
								}
							}
						}
					}
					splitDone = true;
				}
			}
		} while (splitDone);
	}

	void snapVertices(float snapTol)
	{
		physx::Array<nvidia::ExplicitRenderTriangle>& mesh = *mMesh;

		// Create a small bounding cube for each vertex
		const uint32_t vertexCount = 3*(mesh.size()-mTrianglePartition);
		mVertexBounds.resize(vertexCount);

		if (mVertexBounds.size() == 0)
		{
			return;
		}

		VertexRep* vertexRep = &mVertexBounds[0];
		for (uint32_t i = mTrianglePartition; i < mesh.size(); ++i)
		{
			const physx::PxVec3 normal = mesh[i].calculateNormal();
			for (uint32_t j = 0; j < 3; ++j, ++vertexRep)
			{
				vertexRep->bounds.aabb.include(mesh[i].vertices[j].position);
				vertexRep->bounds.aabb.fattenFast(snapTol);
				vertexRep->position = mesh[i].vertices[j].position;
				vertexRep->normal = normal;
			}
		}

		// Generate neighbor info
		mVertexNeighborhoods.setBounds(&mVertexBounds[0].bounds, vertexCount, sizeof(VertexRep));

		const float snapTol2 = snapTol*snapTol;

		// Run through all free vertices, look for neighbors that are free, and snap them together
		for (MeshProcessor::FreeVertexIt it(*this); it.valid(); it.inc())
		{
			Vertex& vertex = mesh[it.triangleIndex()].vertices[it.vertexIndex()];
			VertexRep& vertexRep = it.vertexRep();
			uint32_t N = 1;
			const physx::PxVec3 oldVertexPosition = vertex.position;
			for (MeshProcessor::FreeNeighborVertexIt nIt(*this, it.triangleIndex(), it.vertexIndex()); nIt.valid(); nIt.inc())
			{
				Vertex& neighborVertex = mesh[nIt.triangleIndex()].vertices[nIt.vertexIndex()];
				if ((neighborVertex.position-oldVertexPosition).magnitudeSquared() < snapTol2)
				{
					vertex.position += neighborVertex.position;
					vertexRep.normal += nIt.vertexRep().normal;
					++N;
				}
			}
			vertex.position *= 1.0f/N;
			vertexRep.position = vertex.position;
			vertexRep.normal.normalize();
			for (MeshProcessor::FreeNeighborVertexIt nIt(*this, it.triangleIndex(), it.vertexIndex()); nIt.valid(); nIt.inc())
			{
				Vertex& neighborVertex = mesh[nIt.triangleIndex()].vertices[nIt.vertexIndex()];
				if ((neighborVertex.position-oldVertexPosition).magnitudeSquared() < snapTol2)
				{
					neighborVertex.position = vertex.position;
					nIt.vertexRep().position = vertex.position;
					nIt.vertexRep().normal = vertexRep.normal;
				}
			}
		}
	}

	void resolveIntersections(float relaxationFactor = 0.5f, uint32_t maxIterations = 10)
	{
		physx::Array<nvidia::ExplicitRenderTriangle>& mesh = *mMesh;

		if (mesh.size() == 0 || mParentMesh == NULL)
		{
			return;
		}

		physx::Array<nvidia::ExplicitRenderTriangle>& parentMesh = *mParentMesh;

		// Find neighborhoods for the new active triangles, the inactive triangles, and the face mesh triangles
		const uint32_t parentMeshSize = parentMesh.size();
		physx::Array<BoundsRep> triangleBounds;
		triangleBounds.resize(parentMeshSize + mesh.size());
		// Triangles from the face mesh
		for (uint32_t i = 0; i < parentMeshSize; ++i)
		{
			nvidia::ExplicitRenderTriangle& triangle = parentMesh[i];
			BoundsRep& boundsRep = triangleBounds[i];
			boundsRep.aabb.setEmpty();
			for (uint32_t v = 0; v < 3; ++v)
			{
				boundsRep.aabb.include(triangle.vertices[v].position);
			}
			boundsRep.aabb.fattenFast(mPadding);
		}
		// Triangles from the part mesh
		for (uint32_t i = 0; i < mesh.size(); ++i)
		{
			nvidia::ExplicitRenderTriangle& triangle = mesh[i];
			BoundsRep& boundsRep = triangleBounds[i+parentMeshSize];
			boundsRep.aabb.setEmpty();
			for (uint32_t v = 0; v < 3; ++v)
			{
				boundsRep.aabb.include(triangle.vertices[v].position);
				// Also include the triangle's original vertices if it's a process triangle, so we can check for tunneling
				if (i >= mTrianglePartition)
				{
					VertexRep& vertexRep = mVertexBounds[3*(i-mTrianglePartition) + v];
					boundsRep.aabb.include(vertexRep.position);
				}
			}
			boundsRep.aabb.fattenFast(mPadding);
		}

		NeighborLookup triangleNeighborhoods;
		triangleNeighborhoods.setBounds(&triangleBounds[0], triangleBounds.size(), sizeof(triangleBounds[0]));

		const nvidia::ExplicitRenderTriangle* parentMeshStart = parentMeshSize ? &parentMesh[0] : NULL;

		// Find interfering pairs of triangles
		physx::Array<IntPair> interferingPairs;
		for (uint32_t repIndex = mTrianglePartition+parentMeshSize; repIndex < mesh.size()+parentMeshSize; ++repIndex)
		{
			const uint32_t neighborCount = triangleNeighborhoods.getNeighborCount(repIndex);
			const uint32_t* neighborRepIndices = triangleNeighborhoods.getNeighbors(repIndex);
			for (uint32_t j = 0; j < neighborCount; ++j)
			{
				const uint32_t neighborRepIndex = *neighborRepIndices++;
				if (repIndex > neighborRepIndex)	// Only count each pair once
				{
					if (trianglesInterfere(repIndex, neighborRepIndex, parentMeshStart, parentMeshSize, -mTol))
					{
						IntPair& pair = interferingPairs.insert();
						pair.set((int32_t)repIndex, (int32_t)neighborRepIndex);
					}
				}
			}
		}

		// Now run through the interference list, pulling the vertices in the offending triangles back to
		// their original positions.  Iterate until there are no more interfering triangles, or the maximum
		// number of iterations is reached.
		physx::Array<bool> handled;
		handled.resize(mesh.size() - mTrianglePartition, false);
		for (uint32_t iterN = 0; iterN < maxIterations && interferingPairs.size(); ++iterN)
		{
			for (uint32_t pairN = 0; pairN < interferingPairs.size(); ++pairN)
			{
				const IntPair& pair = interferingPairs[pairN];
				const uint32_t i0 = (uint32_t)pair.i0;
				const uint32_t i1 = (uint32_t)pair.i1;
				if (i0 >= mTrianglePartition + parentMeshSize && !handled[i0 - mTrianglePartition - parentMeshSize])
				{
					relaxTriangleFreeVertices(i0 - parentMeshSize, relaxationFactor);
					handled[i0 - mTrianglePartition - parentMeshSize] = true;
				}
				if (i1 >= mTrianglePartition + parentMeshSize && !handled[i1 - mTrianglePartition - parentMeshSize])
				{
					relaxTriangleFreeVertices(i1 - parentMeshSize, relaxationFactor);
					handled[i1 - mTrianglePartition - parentMeshSize] = true;
				}
			}
			// We've given the vertices a relaxation pass.  Reset the handled list, and remove pairs that no longer interfere
			for (uint32_t pairN = interferingPairs.size(); pairN--;)
			{
				const IntPair& pair = interferingPairs[pairN];
				const uint32_t i0 = (uint32_t)pair.i0;
				const uint32_t i1 = (uint32_t)pair.i1;
				if (i0 >= mTrianglePartition + parentMeshSize)
				{
					handled[i0 - mTrianglePartition - parentMeshSize] = false;
				}
				if (i1 >= mTrianglePartition + parentMeshSize)
				{
					handled[i1 - mTrianglePartition - parentMeshSize] = false;
				}
				if (!trianglesInterfere(i0, i1, parentMeshStart, parentMeshSize, -mTol))
				{
					interferingPairs.replaceWithLast(pairN);
				}
			}
		}
	}

private:
	void reset()
	{
		mMesh = NULL;
		mParentMesh = NULL;
		mTrianglePartition = 0;
		mOriginalTriangleBounds.resize(0);
		mTol = 0.0f;
		mPadding = 0.0f;
		mOriginalTriangleNeighborhoods.setBounds(NULL, 0, 0);
		mTriangleInfo.resize(0);
		mTriangleFrames.resize(0);
		mTriangleChildLists.resize(0);
		mVertexBounds.resize(0);
		mVertexNeighborhoods.setBounds(NULL, 0, 0);
	}

	PX_INLINE void splitTriangle(uint32_t triangleIndex, uint32_t edgeIndex, const physx::PxVec3& newVertexPosition)
	{
		physx::Array<nvidia::ExplicitRenderTriangle>& mesh = *mMesh;
		nvidia::ExplicitRenderTriangle& triangle = mesh[triangleIndex];
		const unsigned nextEdge = (edgeIndex + 1)%3;
		nvidia::ExplicitRenderTriangle newTriangle = triangle;
		TriangleLockInfo& info = mTriangleInfo[triangleIndex-mTrianglePartition];
		TriangleLockInfo newInfo = info;
		const bool splitEdgeIsLocked = ((info.lockedEdges>>edgeIndex)&1) != 0;
		info.lockedEdges &= ~(uint16_t)(1<<((edgeIndex + 2)%3));	// New edge is not locked
		if (!splitEdgeIsLocked)
		{
			info.lockedVertices &= ~(uint16_t)(1<<edgeIndex);	// New vertex is not locked if split edge is not locked
		}
		newInfo.lockedEdges &= ~(uint16_t)(1<<nextEdge);	// New edge is not locked
		if (!splitEdgeIsLocked)
		{
			newInfo.lockedVertices &= ~(uint16_t)(1<<nextEdge);	// New vertex is not locked if split edge is not locked
		}
		const nvidia::TriangleFrame& triangleFrame = mTriangleFrames[newInfo.originalTriangleIndex-mTrianglePartition];
		triangle.vertices[edgeIndex].position = newVertexPosition;
		triangleFrame.interpolateVertexData(triangle.vertices[edgeIndex]);
		newTriangle.vertices[nextEdge]= triangle.vertices[edgeIndex];
		const uint32_t newTriangleIndex = mesh.size();
		mesh.pushBack(newTriangle);
		mTriangleInfo.pushBack(newInfo);
		mTriangleChildLists[newInfo.originalTriangleIndex-mTrianglePartition].pushBack(newTriangleIndex);
	}

	PX_INLINE void relaxTriangleFreeVertices(uint32_t triangleIndex, float relaxationFactor)
	{
		const float tol2 = mTol*mTol;
		physx::Array<nvidia::ExplicitRenderTriangle>& mesh = *mMesh;
		const uint32_t relativeIndex = triangleIndex - mTrianglePartition;
		TriangleLockInfo& info = mTriangleInfo[relativeIndex];
		for (uint32_t v = 0; v < 3; ++v)
		{
			if (!((info.lockedVertices >> v)&1))
			{
				Vertex& vertex = mesh[triangleIndex].vertices[v];
				VertexRep& vertexRep = mVertexBounds[3*relativeIndex + v];
				const physx::PxVec3 oldVertexPosition = vertex.position;
				vertex.position = (1.0f - relaxationFactor)*vertex.position + relaxationFactor*vertexRep.position;
				for (MeshProcessor::FreeNeighborVertexIt nIt(*this, triangleIndex, v); nIt.valid(); nIt.inc())
				{
					Vertex& neighborVertex = mesh[nIt.triangleIndex()].vertices[nIt.vertexIndex()];
					if ((neighborVertex.position-oldVertexPosition).magnitudeSquared() < tol2)
					{
						neighborVertex.position = vertex.position;
					}
				}
			}
		}
	}

	PX_INLINE bool trianglesInterfere(uint32_t indexA, uint32_t indexB, const nvidia::ExplicitRenderTriangle* parentMeshStart, uint32_t parentMeshSize, float tol)
	{
		physx::Array<nvidia::ExplicitRenderTriangle>& mesh = *mMesh;
		const nvidia::ExplicitRenderTriangle& triangleA = indexA >= parentMeshSize ? mesh[indexA - parentMeshSize] : parentMeshStart[indexA];
		const nvidia::ExplicitRenderTriangle& triangleB = indexB >= parentMeshSize ? mesh[indexB - parentMeshSize] : parentMeshStart[indexB];

		// Check for static interference
		if (::trianglesInterfere(triangleA, triangleB, tol))
		{
			return true;
		}

		// See if one of the vertices of A swept through B
		if (indexA >= mTrianglePartition + parentMeshSize)
		{
			for (uint32_t v = 0; v < 3; ++v)
			{
				VertexRep& vertexRep = mVertexBounds[3*(indexA-mTrianglePartition-parentMeshSize) + v];
				if (segmentIntersectsTriangle(vertexRep.position, triangleA.vertices[v].position, triangleB, tol))
				{
					return true;
				}
			}
		}

		// See if one of the vertices of B swept through A
		if (indexB >= mTrianglePartition + parentMeshSize)
		{
			for (uint32_t v = 0; v < 3; ++v)
			{
				VertexRep& vertexRep = mVertexBounds[3*(indexB-mTrianglePartition-parentMeshSize) + v];
				if (segmentIntersectsTriangle(vertexRep.position, triangleB.vertices[v].position, triangleA, tol))
				{
					return true;
				}
			}
		}

		// No interference found
		return false;
	}


	physx::Array<nvidia::ExplicitRenderTriangle>*	mMesh;
	physx::Array<nvidia::ExplicitRenderTriangle>*	mParentMesh;
	uint32_t									mTrianglePartition;
	physx::Array<BoundsRep>							mOriginalTriangleBounds;
	float									mTol;
	float									mPadding;
	NeighborLookup									mOriginalTriangleNeighborhoods;
	physx::Array<TriangleLockInfo>					mTriangleInfo;
	physx::Array<nvidia::TriangleFrame>				mTriangleFrames;
	physx::Array< physx::Array<uint32_t> >		mTriangleChildLists;
	physx::Array<VertexRep>							mVertexBounds;
	NeighborLookup									mVertexNeighborhoods;

	friend class FreeVertexIt;
	friend class FreeNeighborVertexIt;
};


PX_INLINE bool triangleIsPartOfActiveSet(const nvidia::ExplicitRenderTriangle& triangle, const ExplicitHierarchicalMeshImpl& hMesh)
{
	if (triangle.extraDataIndex >= hMesh.mMaterialFrames.size())
	{
		return false;
	}

	const nvidia::MaterialFrame& materialFrame = hMesh.mMaterialFrames[triangle.extraDataIndex];

	return materialFrame.mFractureIndex == -1;
}


static void applyNoiseToChunk
(
 ExplicitHierarchicalMeshImpl& hMesh,
 uint32_t parentPartIndex,
 uint32_t partIndex,
 const nvidia::NoiseParameters& noise,
 float gridScale
 )
{
	if (partIndex >= hMesh.mParts.size() || parentPartIndex >= hMesh.mParts.size())
	{
		return;
	}

	// Mesh and mesh size
	float level0Size = hMesh.mParts[(uint32_t)hMesh.mChunks[0]->mPartIndex]->mBounds.getExtents().magnitude();
	physx::Array<nvidia::ExplicitRenderTriangle>& partMesh = hMesh.mParts[partIndex]->mMesh;
	physx::Array<nvidia::ExplicitRenderTriangle>& parentPartMesh = hMesh.mParts[parentPartIndex]->mMesh;

	// Grid parameters
	const float gridSize = physx::PxAbs(gridScale) / PxMax(2, noise.gridSize);
	if (gridSize == 0.0f)
	{
		return;
	}

	const float tol = 0.0001f*gridSize;

	//	MeshProcessor::removeSlivers(partMesh, 0.5f*tol);

	// Sort triangles based upon whether or not they are part of the active group.
	// Put the active triangles last in the list, so we only need traverse them when splitting
	uint32_t inactiveTriangleCount = 0;
	uint32_t highMark = partMesh.size();
	while (inactiveTriangleCount < highMark)
	{
		if (!triangleIsPartOfActiveSet(partMesh[inactiveTriangleCount], hMesh))
		{
			++inactiveTriangleCount;
		}
		else
			if (triangleIsPartOfActiveSet(partMesh[highMark-1], hMesh))
			{
				--highMark;
			}
			else
			{
				nvidia::swap(partMesh[inactiveTriangleCount++], partMesh[--highMark]);
			}
	}
	PX_ASSERT(inactiveTriangleCount == highMark);

	MeshProcessor chunkMeshProcessor;
	chunkMeshProcessor.setMesh(partMesh, &parentPartMesh, inactiveTriangleCount, tol);
	chunkMeshProcessor.lockBorderVertices();
	chunkMeshProcessor.removeTJunctions();
	chunkMeshProcessor.subdivide(gridSize);
	chunkMeshProcessor.snapVertices(4*tol);

	// Now create and apply noise field
	const uint32_t rndSeedSave = userRnd.m_rnd.seed();
	userRnd.m_rnd.setSeed(0);
	const float scaledAmplitude = noise.amplitude*level0Size;
	const uint32_t numModes = 10;
	const float amplitude = scaledAmplitude / physx::PxSqrt((float)numModes);	// Scale by frequency?
	const float spatialFrequency = noise.frequency*(physx::PxTwoPi/gridSize);
	float phase[numModes][3];
	physx::PxVec3 k[numModes][3];
	for (uint32_t n = 0; n < numModes; ++n)
	{
		for (uint32_t i = 0; i < 3; ++i)
		{
			phase[n][i] = userRnd.getReal(-physx::PxPi, physx::PxPi);
			k[n][i] = physx::PxVec3(userRnd.getReal(-1.0f, 1.0f), userRnd.getReal(-1.0f, 1.0f), userRnd.getReal(-1.0f, 1.0f));
			k[n][i].normalize();	// Not a uniform spherical distribution, but it's ok
			k[n][i] *= spatialFrequency;
		}
	}
	userRnd.m_rnd.setSeed(rndSeedSave);

	for (MeshProcessor::FreeVertexIt it(chunkMeshProcessor); it.valid(); it.inc())
	{
		physx::PxVec3& r = partMesh[it.triangleIndex()].vertices[it.vertexIndex()].position;
		physx::PxVec3 field(0.0f);
		physx::PxMat33 gradient(physx::PxVec3(0.0f), physx::PxVec3(0.0f), physx::PxVec3(0.0f));
		for (uint32_t n = 0; n < numModes; ++n)
		{
			for (uint32_t i = 0; i < 3; ++i)
			{
				const float phi = k[n][i].dot(r) + phase[n][i];
				field[i] += amplitude*physx::PxSin(phi);
				for (uint32_t j = 0; j < 3; ++j)
				{
					gradient(i,j) += amplitude*k[n][i][j]*physx::PxCos(phi);
				}
			}
		}
		r += field.dot(it.vertexRep().normal)*it.vertexRep().normal;
		physx::PxVec3 g = gradient.transformTranspose(it.vertexRep().normal);
		physx::PxVec3& n = partMesh[it.triangleIndex()].vertices[it.vertexIndex()].normal;
		n += g.dot(n)*n - g;
		n.normalize();
		physx::PxVec3& t = partMesh[it.triangleIndex()].vertices[it.vertexIndex()].tangent;
		t -= t.dot(n)*n;
		t.normalize();
		partMesh[it.triangleIndex()].vertices[it.vertexIndex()].binormal = n.cross(t);
	}

	// Fix up any mesh intersections that may have resulted from the application of noise
	chunkMeshProcessor.resolveIntersections();
}


static bool voronoiSplitChunkInternal
(
	ExplicitHierarchicalMeshImpl& hMesh,
	uint32_t chunkIndex,
	const ApexCSG::IApexBSP& chunkBSP,
	const nvidia::FractureVoronoiDesc& desc,
	const CollisionDesc& collisionDesc,
	nvidia::IProgressListener& progressListener,
	volatile bool* cancel
)
{
	bool canceled = false;

	physx::Array<physx::PxVec3> sitesForChunk;
	const physx::PxVec3* sites = desc.sites;
	uint32_t siteCount = desc.siteCount;
	if (desc.chunkIndices != NULL)
	{
		for (uint32_t siteN = 0; siteN < desc.siteCount; ++siteN)
		{
			if (desc.chunkIndices[siteN] == chunkIndex)
			{
				sitesForChunk.pushBack(desc.sites[siteN]);
			}
		}
		siteCount = sitesForChunk.size();
		sites = siteCount > 0 ? &sitesForChunk[0] : NULL;
	}

	if (siteCount < 2)
	{
		return !canceled;	// Don't want to generate a single child which is a duplicate of the parent, when siteCount == 1
	}

	HierarchicalProgressListener progress((int32_t)PxMax(siteCount, 1u), &progressListener);

	const float minimumRadius2 = hMesh.chunkBounds(0).getExtents().magnitudeSquared()*desc.minimumChunkSize*desc.minimumChunkSize;

	for (VoronoiCellPlaneIterator i(sites, siteCount); i.valid(); i.inc())
	{
		// Create a voronoi cell for this site
		ApexCSG::IApexBSP* cellBSP = createBSP(hMesh.mBSPMemCache, chunkBSP.getInternalTransform());	// BSPs start off representing all space
		ApexCSG::IApexBSP* planeBSP = createBSP(hMesh.mBSPMemCache, chunkBSP.getInternalTransform());
		const physx::PxPlane* planes = i.cellPlanes();
		for (uint32_t planeN = 0; planeN < i.cellPlaneCount(); ++planeN)
		{
			const physx::PxPlane& plane = planes[planeN];

			// Create single-plane slice BSP
			nvidia::IntersectMesh grid;
			GridParameters gridParameters;
			gridParameters.interiorSubmeshIndex = desc.materialDesc.interiorSubmeshIndex;
			// Defer noise generation if we're using displacement maps
			gridParameters.noise = nvidia::NoiseParameters();
			gridParameters.level0Mesh = &hMesh.mParts[0]->mMesh;
			gridParameters.materialFrameIndex = hMesh.addMaterialFrame();
			nvidia::MaterialFrame materialFrame = hMesh.getMaterialFrame(gridParameters.materialFrameIndex);
			materialFrame.buildCoordinateSystemFromMaterialDesc(desc.materialDesc, plane);
			materialFrame.mFractureMethod = nvidia::FractureMethod::Voronoi;
			materialFrame.mSliceDepth = hMesh.depth(chunkIndex) + 1;
			// Leaving the materialFrame.mFractureMethod at the default of -1, since voronoi cutout faces are not be associated with a direction index
			hMesh.setMaterialFrame(gridParameters.materialFrameIndex, materialFrame);
			gridParameters.triangleFrame.setFlat(materialFrame.mCoordinateSystem, desc.materialDesc.uvScale, desc.materialDesc.uvOffset);
			buildIntersectMesh(grid, plane, materialFrame, (int32_t)desc.noiseMode, &gridParameters);

			ApexCSG::BSPBuildParameters bspBuildParams = gDefaultBuildParameters;
			bspBuildParams.internalTransform = chunkBSP.getInternalTransform();
			bspBuildParams.rnd = &userRnd;

			if(desc.useDisplacementMaps)
			{
				// Displacement map generation is deferred until the end of fracturing
				// This used to be where a slice would populate a displacement map with 
				//  offsets along the plane, but no longer
			}

			planeBSP->fromMesh(&grid.m_triangles[0], grid.m_triangles.size(), bspBuildParams);
			cellBSP->combine(*planeBSP);
			cellBSP->op(*cellBSP, ApexCSG::Operation::Intersection);
		}
		planeBSP->release();

		if (hMesh.mParts[(uint32_t)hMesh.mChunks[chunkIndex]->mPartIndex]->mFlags & ExplicitHierarchicalMeshImpl::Part::MeshOpen)
		{
			cellBSP->deleteTriangles();	// Don't use interior triangles on an open mesh
		}

		ApexCSG::IApexBSP* bsp = createBSP(hMesh.mBSPMemCache);
		bsp->copy(*cellBSP);
		bsp->combine(chunkBSP);
		bsp->op(*bsp, ApexCSG::Operation::Intersection);
		cellBSP->release();

		if (gIslandGeneration)
		{
			bsp = bsp->decomposeIntoIslands();
		}

		while (bsp != NULL)
		{
			if (cancel != NULL && *cancel)
			{
				canceled = true;
			}

			if (!canceled)
			{
				// Create a mesh with chunkBSP (or its islands)
				const uint32_t newPartIndex = hMesh.addPart();
				const uint32_t newChunkIndex = hMesh.addChunk();
				bsp->toMesh(hMesh.mParts[newPartIndex]->mMesh);
				hMesh.mParts[newPartIndex]->mMeshBSP->copy(*bsp);
				hMesh.buildMeshBounds(newPartIndex);
				hMesh.buildCollisionGeometryForPart(newPartIndex, getVolumeDesc(collisionDesc, hMesh.depth(chunkIndex)+1));
				hMesh.mChunks[newChunkIndex]->mParentIndex = (int32_t)chunkIndex;
				hMesh.mChunks[newChunkIndex]->mPartIndex = (int32_t)newPartIndex;
				if (hMesh.mParts[(uint32_t)hMesh.mChunks[chunkIndex]->mPartIndex]->mFlags & ExplicitHierarchicalMeshImpl::Part::MeshOpen)
				{
					hMesh.mParts[newPartIndex]->mFlags |= ExplicitHierarchicalMeshImpl::Part::MeshOpen;
				}
				if (hMesh.mParts[newPartIndex]->mMesh.size() == 0 || hMesh.mParts[newPartIndex]->mCollision.size() == 0 ||
					hMesh.chunkBounds(newChunkIndex).getExtents().magnitudeSquared() < minimumRadius2)
				{
					// Either no mesh, no collision, or too small.  Eliminate.
					hMesh.removeChunk(newChunkIndex);
					hMesh.removePart(newPartIndex);
				}
				else
				{
					// Apply graphical noise to new part, if requested
					if (desc.faceNoise.amplitude > 0.0f){
						const uint32_t parentPartIndex = (uint32_t)*hMesh.partIndex(chunkIndex);
						applyNoiseToChunk(hMesh, parentPartIndex, newPartIndex, desc.faceNoise, hMesh.meshBounds(newPartIndex).getExtents().magnitude());
					}
				}
			}
			// Get next bsp in island decomposition
			ApexCSG::IApexBSP* nextBSP = bsp->getNext();
			bsp->release();
			bsp = nextBSP;
		}

		progress.completeSubtask();
	}

	return !canceled;
}

namespace FractureTools
{

void setBSPTolerances
(
	float linearTolerance,
	float angularTolerance,
	float baseTolerance,
	float clipTolerance,
	float cleaningTolerance
)
{
	ApexCSG::gDefaultTolerances.linear = linearTolerance;
	ApexCSG::gDefaultTolerances.angular = angularTolerance;
	ApexCSG::gDefaultTolerances.base = baseTolerance;
	ApexCSG::gDefaultTolerances.clip = clipTolerance;
	ApexCSG::gDefaultTolerances.cleaning = cleaningTolerance;
}

void setBSPBuildParameters
(
	float logAreaSigmaThreshold,
	uint32_t testSetSize,
	float splitWeight,
	float imbalanceWeight
)
{
	gDefaultBuildParameters.logAreaSigmaThreshold = logAreaSigmaThreshold;
	gDefaultBuildParameters.testSetSize = testSetSize;
	gDefaultBuildParameters.splitWeight = splitWeight;
	gDefaultBuildParameters.imbalanceWeight = imbalanceWeight;
}

static void trimChunkHulls(ExplicitHierarchicalMeshImpl& hMesh, uint32_t* chunkIndexArray, uint32_t chunkIndexArraySize, float maxTrimFraction)
{
	// Outer array is indexed by chunk #, and is of size chunkIndexArraySize
	// Middle array is indexed by hull # for chunkIndexArray[chunk #], is of the same size as the part mCollision array associated with the chunk
	// Inner array is a list of trim planes to be applied to each hull
	physx::Array< physx::Array< physx::Array<physx::PxPlane> > > chunkHullTrimPlanes;

	// Initialize arrays
	chunkHullTrimPlanes.resize(chunkIndexArraySize);
	for (uint32_t chunkNum = 0; chunkNum < chunkIndexArraySize; ++chunkNum)
	{
		physx::Array< physx::Array<physx::PxPlane> >& hullTrimPlanes = chunkHullTrimPlanes[chunkNum];
		const uint32_t chunkIndex = chunkIndexArray[chunkNum];
		const uint32_t partIndex = (uint32_t)hMesh.mChunks[chunkIndex]->mPartIndex;
		const uint32_t hullCount = hMesh.mParts[partIndex]->mCollision.size();
		hullTrimPlanes.resize(hullCount);
	}

	const physx::PxVec3 identityScale(1.0f);

	// Compare each chunk's hulls against other chunk hulls, building up list of trim planes.  O(N^2), but so far this is only used for multi-fbx level 1 chunks, so N shouldn't be too large.
	for (uint32_t chunkNum0 = 0; chunkNum0 < chunkIndexArraySize; ++chunkNum0)
	{
		const uint32_t chunkIndex0 = chunkIndexArray[chunkNum0];
		const uint32_t partIndex0 = (uint32_t)hMesh.mChunks[chunkIndex0]->mPartIndex;
		const physx::PxTransform tm0(hMesh.mChunks[chunkIndex0]->mInstancedPositionOffset);
		const uint32_t hullCount0 = hMesh.mParts[partIndex0]->mCollision.size();
		physx::Array< physx::Array<physx::PxPlane> >& hullTrimPlanes0 = chunkHullTrimPlanes[chunkNum0];
		for (uint32_t hullIndex0 = 0; hullIndex0 < hullCount0; ++hullIndex0)
		{
			PartConvexHullProxy* hull0 = hMesh.mParts[partIndex0]->mCollision[hullIndex0];
			physx::Array<physx::PxPlane>& trimPlanes0 = hullTrimPlanes0[hullIndex0];

			// Inner set of loops for other chunks
			for (uint32_t chunkNum1 = chunkNum0+1; chunkNum1 < chunkIndexArraySize; ++chunkNum1)
			{
				const uint32_t chunkIndex1 = chunkIndexArray[chunkNum1];
				const uint32_t partIndex1 = (uint32_t)hMesh.mChunks[chunkIndex1]->mPartIndex;
				const physx::PxTransform tm1(hMesh.mChunks[chunkIndex1]->mInstancedPositionOffset);
				const uint32_t hullCount1 = hMesh.mParts[partIndex1]->mCollision.size();
				physx::Array< physx::Array<physx::PxPlane> >& hullTrimPlanes1 = chunkHullTrimPlanes[chunkNum1];
				for (uint32_t hullIndex1 = 0; hullIndex1 < hullCount1; ++hullIndex1)
				{
					PartConvexHullProxy* hull1 = hMesh.mParts[partIndex1]->mCollision[hullIndex1];
					physx::Array<physx::PxPlane>& trimPlanes1 = hullTrimPlanes1[hullIndex1];

					// Test overlap
					ConvexHullImpl::Separation separation;
					if (ConvexHullImpl::hullsInProximity(hull0->impl, tm0, identityScale, hull1->impl, tm1, identityScale, 0.0f, &separation))
					{
						// Add trim planes if there's an overlap
						physx::PxPlane& trimPlane0 = trimPlanes0.insert();
						trimPlane0 = separation.plane;
						trimPlane0.d = PxMin(trimPlane0.d, maxTrimFraction*(separation.max0-separation.min0) - separation.max0);	// Bound clip distance
						trimPlane0.d += trimPlane0.n.dot(tm0.p);	// Transform back into part local space
						physx::PxPlane& trimPlane1 = trimPlanes1.insert();
						trimPlane1 = physx::PxPlane(-separation.plane.n, -separation.plane.d);
						trimPlane1.d = PxMin(trimPlane1.d, maxTrimFraction*(separation.max1-separation.min1) + separation.min1);	// Bound clip distance
						trimPlane1.d += trimPlane1.n.dot(tm1.p);	// Transform back into part local space
					}
				}
			}
		}
	}

	// Now traverse trim plane list and apply it to the chunks's hulls
	for (uint32_t chunkNum = 0; chunkNum < chunkIndexArraySize; ++chunkNum)
	{
		const uint32_t chunkIndex = chunkIndexArray[chunkNum];
		const uint32_t partIndex = (uint32_t)hMesh.mChunks[chunkIndex]->mPartIndex;
		const uint32_t hullCount = hMesh.mParts[partIndex]->mCollision.size();
		physx::Array< physx::Array<physx::PxPlane> >& hullTrimPlanes = chunkHullTrimPlanes[chunkNum];
		for (uint32_t hullIndex = hullCount; hullIndex--;)	// Traverse backwards, in case we need to delete empty hulls
		{
			PartConvexHullProxy* hull = hMesh.mParts[partIndex]->mCollision[hullIndex];
			physx::Array<physx::PxPlane>& trimPlanes = hullTrimPlanes[hullIndex];
			for (uint32_t planeIndex = 0; planeIndex < trimPlanes.size(); ++planeIndex)
			{
				hull->impl.intersectPlaneSide(trimPlanes[planeIndex]);
				if (hull->impl.isEmpty())
				{
					PX_DELETE(hMesh.mParts[partIndex]->mCollision[hullIndex]);
					hMesh.mParts[partIndex]->mCollision.replaceWithLast(hullIndex);
					break;
				}
			}
		}
		// Make sure we haven't deleted every collision hull
		if (hMesh.mParts[partIndex]->mCollision.size() == 0)
		{
			CollisionVolumeDesc collisionVolumeDesc;
			collisionVolumeDesc.mHullMethod = ConvexHullMethod::WRAP_GRAPHICS_MESH;	// Should we use something simpler, like a box?
			hMesh.buildCollisionGeometryForPart(partIndex, collisionVolumeDesc);
		}
	}
}

bool buildExplicitHierarchicalMesh
(
    ExplicitHierarchicalMesh& iHMesh,
    const nvidia::ExplicitRenderTriangle* meshTriangles,
    uint32_t meshTriangleCount,
    const nvidia::ExplicitSubmeshData* submeshData,
    uint32_t submeshCount,
    uint32_t* meshPartition,
    uint32_t meshPartitionCount,
	int32_t* parentIndices,
	uint32_t parentIndexCount
)
{
	bool flatDepthOne = parentIndexCount == 0;

	const bool havePartition = meshPartition != NULL && meshPartitionCount > 1;

	if (!havePartition)
	{
		flatDepthOne = true;	// This only makes sense if we have a partition
	}

	if (parentIndices == NULL)
	{
		parentIndexCount = 0;
	}

	ExplicitHierarchicalMeshImpl& hMesh = *(ExplicitHierarchicalMeshImpl*)&iHMesh;
	hMesh.clear();
	hMesh.addPart();
	hMesh.mParts[0]->mMesh.reset();
	const uint32_t part0Size = !flatDepthOne ? meshPartition[0] : meshTriangleCount;	// Build level 0 part out of all of the triangles if flatDepthOne = true
	hMesh.mParts[0]->mMesh.reserve(part0Size);
	uint32_t nextTriangle = 0;
	for (uint32_t i = 0; i < part0Size; ++i)
	{
		hMesh.mParts[0]->mMesh.pushBack(meshTriangles[nextTriangle++]);
	}
	hMesh.buildMeshBounds(0);
	hMesh.addChunk();
	hMesh.mChunks[0]->mParentIndex = -1;
	hMesh.mChunks[0]->mPartIndex = 0;

	if (flatDepthOne)
	{
		nextTriangle = 0;	// reset
	}

	physx::Array<bool> hasChildren(meshPartitionCount+1, false);

	if (havePartition)
	{
		// We have a partition - build hierarchy
		uint32_t partIndex = 1;
		const uint32_t firstLevel1Part = !flatDepthOne ? 1u : 0u;
		for (uint32_t i = firstLevel1Part; i < meshPartitionCount; ++i, ++partIndex)
		{
			hMesh.addPart();
			hMesh.mParts[partIndex]->mMesh.reset();
			hMesh.mParts[partIndex]->mMesh.reserve(meshPartition[i] - nextTriangle);
			while (nextTriangle < meshPartition[i])
			{
				hMesh.mParts[partIndex]->mMesh.pushBack(meshTriangles[nextTriangle++]);
			}
			hMesh.buildMeshBounds(partIndex);
			hMesh.addChunk();
			hMesh.mChunks[partIndex]->mParentIndex = partIndex < parentIndexCount ? parentIndices[partIndex] : 0;	// partIndex = chunkIndex here
			if (hMesh.mChunks[partIndex]->mParentIndex >= 0)
			{
				hasChildren[(uint32_t)hMesh.mChunks[partIndex]->mParentIndex] = true;
			}
			hMesh.mChunks[partIndex]->mPartIndex = (int32_t)partIndex;	// partIndex = chunkIndex here
		}
	}

	// Submesh data
	hMesh.mSubmeshData.reset();
	hMesh.mSubmeshData.reserve(submeshCount);
	for (uint32_t i = 0; i < submeshCount; ++i)
	{
		hMesh.mSubmeshData.pushBack(submeshData[i]);
	}

	for (uint32_t i = 0; i < hMesh.mChunks.size(); ++i)
	{
		hMesh.mChunks[i]->mPrivateFlags |= ExplicitHierarchicalMeshImpl::Chunk::Root;
		if (!hasChildren[i])
		{
			hMesh.mChunks[i]->mPrivateFlags |= ExplicitHierarchicalMeshImpl::Chunk::RootLeaf;
		}
	}

	hMesh.mRootSubmeshCount = submeshCount;

	hMesh.sortChunks();

	return true;
}

// If destructibleAsset == NULL, no hierarchy is assumed and we must have only one part in the render mesh.
static bool buildExplicitHierarchicalMeshFromApexAssetsInternal(ExplicitHierarchicalMeshImpl& hMesh, const nvidia::apex::RenderMeshAsset& renderMeshAsset,
																const nvidia::apex::DestructibleAsset* destructibleAsset, uint32_t maxRootDepth = UINT32_MAX)
{
	if (renderMeshAsset.getPartCount() == 0)
	{
		return false;
	}

	if (destructibleAsset == NULL && renderMeshAsset.getPartCount() != 1)
	{
		return false;
	}

	hMesh.clear();

	// Create parts
	for (uint32_t partIndex = 0; partIndex < renderMeshAsset.getPartCount(); ++partIndex)
	{
		const uint32_t newPartIndex = hMesh.addPart();
		PX_ASSERT(newPartIndex == partIndex);
		ExplicitHierarchicalMeshImpl::Part* part = hMesh.mParts[newPartIndex];
		// Fill in fields except for mesh (will be done in submesh loop below)
		// part->mMeshBSP is NULL, that's OK
		part->mBounds = renderMeshAsset.getBounds(partIndex);
		if (destructibleAsset != NULL)
		{
			// Get collision data from destructible asset
			part->mCollision.reserve(destructibleAsset->getPartConvexHullCount(partIndex));
			for (uint32_t hullIndex = 0; hullIndex < destructibleAsset->getPartConvexHullCount(partIndex); ++hullIndex)
			{
				NvParameterized::Interface* hullParams = destructibleAsset->getPartConvexHullArray(partIndex)[hullIndex];
				if (hullParams != NULL)
				{
					PartConvexHullProxy* newHull = PX_NEW(PartConvexHullProxy)();
					part->mCollision.pushBack(newHull);
					newHull->impl.mParams->copy(*hullParams);
				}
			}
		}
	}

	// Deduce root and interior submesh info
	hMesh.mRootSubmeshCount = 0;	// Incremented below

	// Fill in mesh and get submesh data
	hMesh.mSubmeshData.reset();
	hMesh.mSubmeshData.reserve(renderMeshAsset.getSubmeshCount());
	for (uint32_t submeshIndex = 0; submeshIndex < renderMeshAsset.getSubmeshCount(); ++submeshIndex)
	{
		const nvidia::RenderSubmesh& submesh = renderMeshAsset.getSubmesh(submeshIndex);

		// Submesh data
		nvidia::ExplicitSubmeshData& submeshData = hMesh.mSubmeshData.pushBack(nvidia::ExplicitSubmeshData());
		nvidia::strlcpy(submeshData.mMaterialName, nvidia::ExplicitSubmeshData::MaterialNameBufferSize, renderMeshAsset.getMaterialName(submeshIndex));
		submeshData.mVertexFormat.mBonesPerVertex = 1;

		// Mesh
		const nvidia::VertexBuffer& vb = submesh.getVertexBuffer();
		const nvidia::VertexFormat& vbFormat = vb.getFormat();
		const uint32_t submeshVertexCount = vb.getVertexCount();
		if (submeshVertexCount == 0)
		{
			continue;
		}

		// Get vb data:
		physx::Array<physx::PxVec3> positions;
		physx::Array<physx::PxVec3> normals;
		physx::Array<physx::PxVec4> tangents;	// Handle vec4 tangents
		physx::Array<physx::PxVec3> binormals;
		physx::Array<nvidia::ColorRGBA> colors;
		physx::Array<nvidia::VertexUV> uvs[VertexFormat::MAX_UV_COUNT];

		// Positions
		const int32_t positionBufferIndex = vbFormat.getBufferIndexFromID(vbFormat.getSemanticID(RenderVertexSemantic::POSITION));
		positions.resize(submeshVertexCount);
		submeshData.mVertexFormat.mHasStaticPositions = vb.getBufferData(&positions[0], nvidia::RenderDataFormat::FLOAT3, sizeof(physx::PxVec3), 
																		(uint32_t)positionBufferIndex, 0, submeshVertexCount);
		if (!submeshData.mVertexFormat.mHasStaticPositions)
		{
			return false;	// Need a position buffer!
		}

		// Normals
		const int32_t normalBufferIndex = vbFormat.getBufferIndexFromID(vbFormat.getSemanticID(RenderVertexSemantic::NORMAL));
		normals.resize(submeshVertexCount);
		submeshData.mVertexFormat.mHasStaticNormals = vb.getBufferData(&normals[0], nvidia::RenderDataFormat::FLOAT3, sizeof(physx::PxVec3), 
																		(uint32_t)normalBufferIndex, 0, submeshVertexCount);
		if (!submeshData.mVertexFormat.mHasStaticNormals)
		{
			::memset(&normals[0], 0, submeshVertexCount*sizeof(physx::PxVec3));	// Fill with zeros
		}

		// Tangents
		const int32_t tangentBufferIndex = vbFormat.getBufferIndexFromID(vbFormat.getSemanticID(RenderVertexSemantic::TANGENT));
		tangents.resize(submeshVertexCount, physx::PxVec4(physx::PxVec3(0.0f), 1.0f));	// Fill with (0,0,0,1), in case we read 3-component tangents
		switch (vbFormat.getBufferFormat((uint32_t)tangentBufferIndex))
		{
		case nvidia::RenderDataFormat::BYTE_SNORM3:
		case nvidia::RenderDataFormat::SHORT_SNORM3:
		case nvidia::RenderDataFormat::FLOAT3:
			submeshData.mVertexFormat.mHasStaticTangents = vb.getBufferData(&tangents[0], nvidia::RenderDataFormat::FLOAT3, sizeof(physx::PxVec4), (uint32_t)tangentBufferIndex, 0, submeshVertexCount);
			break;
		case nvidia::RenderDataFormat::BYTE_SNORM4:
		case nvidia::RenderDataFormat::SHORT_SNORM4:
		case nvidia::RenderDataFormat::FLOAT4:
			submeshData.mVertexFormat.mHasStaticTangents = vb.getBufferData(&tangents[0], nvidia::RenderDataFormat::FLOAT4, sizeof(physx::PxVec4), (uint32_t)tangentBufferIndex, 0, submeshVertexCount);
			break;
		default:
			submeshData.mVertexFormat.mHasStaticTangents = false;
			break;
		}

		// Binormals
		const int32_t binormalBufferIndex = vbFormat.getBufferIndexFromID(vbFormat.getSemanticID(RenderVertexSemantic::BINORMAL));
		binormals.resize(submeshVertexCount);
		submeshData.mVertexFormat.mHasStaticBinormals = vb.getBufferData(&binormals[0], nvidia::RenderDataFormat::FLOAT3, sizeof(physx::PxVec3), 
																		(uint32_t)binormalBufferIndex, 0, submeshVertexCount);
		if (!submeshData.mVertexFormat.mHasStaticBinormals)
		{
			submeshData.mVertexFormat.mHasStaticBinormals = submeshData.mVertexFormat.mHasStaticNormals && submeshData.mVertexFormat.mHasStaticTangents;
			for (uint32_t i = 0; i < submeshVertexCount; ++i)
			{
				binormals[i] = physx::PxSign(tangents[i][3])*normals[i].cross(tangents[i].getXYZ());	// Build from normals and tangents.  If one of these doesn't exist we'll get (0,0,0)'s
			}
		}

		// Colors
		const int32_t colorBufferIndex = vbFormat.getBufferIndexFromID(vbFormat.getSemanticID(RenderVertexSemantic::COLOR));
		colors.resize(submeshVertexCount);
		submeshData.mVertexFormat.mHasStaticColors = vb.getBufferData(&colors[0], nvidia::RenderDataFormat::B8G8R8A8, sizeof(nvidia::ColorRGBA), 
																		(uint32_t)colorBufferIndex, 0, submeshVertexCount);
		if (!submeshData.mVertexFormat.mHasStaticColors)
		{
			::memset(&colors[0], 0xFF, submeshVertexCount*sizeof(nvidia::ColorRGBA));	// Fill with 0xFF
		}

		// UVs
		submeshData.mVertexFormat.mUVCount = 0;
		uint32_t uvNum = 0;
		for (; uvNum < VertexFormat::MAX_UV_COUNT; ++uvNum)
		{
			uvs[uvNum].resize(submeshVertexCount);
			const int32_t uvBufferIndex = vbFormat.getBufferIndexFromID(vbFormat.getSemanticID((RenderVertexSemantic::Enum)(RenderVertexSemantic::TEXCOORD0 + uvNum)));
			if (!vb.getBufferData(&uvs[uvNum][0], nvidia::RenderDataFormat::FLOAT2, sizeof(nvidia::VertexUV), 
																		(uint32_t)uvBufferIndex, 0, submeshVertexCount))
			{
				break;
			}
		}
		submeshData.mVertexFormat.mUVCount = uvNum;
		for (; uvNum < VertexFormat::MAX_UV_COUNT; ++uvNum)
		{
			uvs[uvNum].resize(submeshVertexCount);
			::memset(&uvs[uvNum][0], 0, submeshVertexCount*sizeof(nvidia::VertexUV));	// Fill with zeros
		}

		// Now create triangles
		bool rootChunkHasTrianglesWithThisSubmesh = false;
		for (uint32_t partIndex = 0; partIndex < renderMeshAsset.getPartCount(); ++partIndex)
		{
			ExplicitHierarchicalMeshImpl::Part* part = hMesh.mParts[partIndex];
			physx::Array<nvidia::ExplicitRenderTriangle>& triangles = part->mMesh;
			const uint32_t* indexBuffer = submesh.getIndexBuffer(partIndex);
			const uint32_t* smoothingGroups = submesh.getSmoothingGroups(partIndex);
			const uint32_t indexCount = submesh.getIndexCount(partIndex);
			PX_ASSERT((indexCount%3) == 0);
			const uint32_t triangleCount = indexCount/3;
			triangles.reserve(triangles.size() + triangleCount);
			if (triangleCount > 0 && destructibleAsset != NULL)
			{
				for (uint32_t chunkIndex = 0; chunkIndex < destructibleAsset->getChunkCount(); ++chunkIndex)
				{
					if (destructibleAsset->getPartIndex(chunkIndex) == partIndex)
					{
						// This part is in a root chunk.  Make sure we've accounted for all of its submeshes
						rootChunkHasTrianglesWithThisSubmesh = true;
						break;
					}
				}
			}
			for (uint32_t triangleNum = 0; triangleNum < triangleCount; ++triangleNum)
			{
				nvidia::ExplicitRenderTriangle& triangle = triangles.pushBack(nvidia::ExplicitRenderTriangle());
				triangle.extraDataIndex = 0xFFFFFFFF;
				triangle.smoothingMask = smoothingGroups != NULL ? smoothingGroups[triangleNum] : 0;
				triangle.submeshIndex = (int32_t)submeshIndex;
				for (unsigned v = 0; v < 3; ++v)
				{
					const uint32_t index = *indexBuffer++;
					nvidia::Vertex& vertex = triangle.vertices[v];
					vertex.position = positions[index];
					vertex.normal = normals[index];
					vertex.tangent = tangents[index].getXYZ();
					vertex.binormal = binormals[index];
					vertex.color = VertexColor(ColorRGBA(colors[index]));
					for (uint32_t uvNum = 0; uvNum < VertexFormat::MAX_UV_COUNT; ++uvNum)
					{
						vertex.uv[uvNum] = uvs[uvNum][index];
					}
					vertex.boneIndices[0] = (uint16_t)partIndex;
				}
			}
		}

		if (rootChunkHasTrianglesWithThisSubmesh)
		{
			hMesh.mRootSubmeshCount = submeshIndex+1;
		}
	}

	// Create chunks
	if (destructibleAsset != NULL)
	{
		physx::Array<bool> hasRootChildren(destructibleAsset->getChunkCount(), false);
		for (uint32_t chunkIndex = 0; chunkIndex < destructibleAsset->getChunkCount(); ++chunkIndex)
		{
			const uint32_t newChunkIndex = hMesh.addChunk();
			PX_ASSERT(newChunkIndex == chunkIndex);
			ExplicitHierarchicalMeshImpl::Chunk* chunk = hMesh.mChunks[newChunkIndex];
			// Fill in fields of chunk
			chunk->mParentIndex = destructibleAsset->getChunkParentIndex(chunkIndex);
			chunk->mFlags = destructibleAsset->getChunkFlags(chunkIndex);
			chunk->mPartIndex = (int32_t)destructibleAsset->getPartIndex(chunkIndex);
			chunk->mInstancedPositionOffset = destructibleAsset->getChunkPositionOffset(chunkIndex);
			chunk->mInstancedUVOffset = destructibleAsset->getChunkUVOffset(chunkIndex);
			if (destructibleAsset->getChunkDepth(chunkIndex) <= maxRootDepth)
			{
				chunk->mPrivateFlags |= ExplicitHierarchicalMeshImpl::Chunk::Root;	// We will assume every chunk is a root chunk
				if (chunk->mParentIndex >= 0 && chunk->mParentIndex < (physx::PxI32)destructibleAsset->getChunkCount())
				{
					hasRootChildren[(physx::PxU32)chunk->mParentIndex] = true;
				}
			}
		}

		// See which root chunks have no children; these are root leaves
		for (uint32_t chunkIndex = 0; chunkIndex < destructibleAsset->getChunkCount(); ++chunkIndex)
		{
			ExplicitHierarchicalMeshImpl::Chunk* chunk = hMesh.mChunks[chunkIndex];
			if (chunk->isRootChunk() && !hasRootChildren[chunkIndex])
			{
				chunk->mPrivateFlags |= ExplicitHierarchicalMeshImpl::Chunk::RootLeaf;
			}
		}
	}
	else
	{
		// No destructible asset, there's just one chunk
		const uint32_t newChunkIndex = hMesh.addChunk();
		PX_ASSERT(newChunkIndex == 0);
		ExplicitHierarchicalMeshImpl::Chunk* chunk = hMesh.mChunks[newChunkIndex];
		// Fill in fields of chunk
		chunk->mParentIndex = -1;
		chunk->mFlags = 0;	// Can't retrieve this
		chunk->mPartIndex = 0;
		chunk->mInstancedPositionOffset = physx::PxVec3(0.0f);
		chunk->mInstancedUVOffset = physx::PxVec2(0.0f);
		chunk->mPrivateFlags |= (ExplicitHierarchicalMeshImpl::Chunk::Root | ExplicitHierarchicalMeshImpl::Chunk::RootLeaf);
	}

	return true;
}

PX_INLINE bool trianglesTouch(const nvidia::ExplicitRenderTriangle& t1, const nvidia::ExplicitRenderTriangle& t2)
{
	PX_UNUSED(t1);
	PX_UNUSED(t2);
	return true;	// For now, just keep AABB test.  May want to do better.
}

static void partitionMesh(physx::Array<uint32_t>& partition, nvidia::ExplicitRenderTriangle* mesh, uint32_t meshTriangleCount, float padding)
{
	// Find triangle neighbors
	physx::Array<nvidia::BoundsRep> triangleBounds;
	triangleBounds.reserve(meshTriangleCount);
	for (uint32_t i = 0; i < meshTriangleCount; ++i)
	{
		nvidia::ExplicitRenderTriangle& triangle = mesh[i];
		nvidia::BoundsRep& rep = triangleBounds.insert();
		for (int j = 0; j < 3; ++j)
		{
			rep.aabb.include(triangle.vertices[j].position);
		}
		rep.aabb.fattenFast(padding);
	}

	NeighborLookup triangleNeighborhoods;
	triangleNeighborhoods.setBounds(&triangleBounds[0], triangleBounds.size(), sizeof(triangleBounds[0]));

	// Re-ordering the mesh in-place will make the neighborhoods invalid, so we re-map
	physx::Array<uint32_t> triangleRemap(meshTriangleCount);
	physx::Array<uint32_t> triangleRemapInv(meshTriangleCount);
	for (uint32_t i = 0; i < meshTriangleCount; ++i)
	{
		triangleRemap[i] = i;
		triangleRemapInv[i] = i;
	}

	partition.resize(0);
	uint32_t nextTriangle = 0;
	while (nextTriangle < meshTriangleCount)
	{
		uint32_t partitionStop = nextTriangle+1;
		do
		{
			const uint32_t r = triangleRemap[nextTriangle];
			const uint32_t neighborCount = triangleNeighborhoods.getNeighborCount(r);
			const uint32_t* neighbors = triangleNeighborhoods.getNeighbors(r);
			for (uint32_t n = 0; n < neighborCount; ++n)
			{
				const uint32_t s = triangleRemapInv[neighbors[n]];
				if (s <= partitionStop || !trianglesTouch(mesh[nextTriangle], mesh[s]))
				{
					continue;
				}
				nvidia::swap(triangleRemapInv[triangleRemap[partitionStop]], triangleRemapInv[triangleRemap[s]]);
				nvidia::swap(triangleRemap[partitionStop], triangleRemap[s]);
				nvidia::swap(mesh[partitionStop], mesh[s]);
				++partitionStop;
			}
		} while(nextTriangle++ < partitionStop);
		partition.pushBack(nextTriangle);
	}
}

uint32_t partitionMeshByIslands
(
	nvidia::ExplicitRenderTriangle* mesh,
	uint32_t meshTriangleCount,
    uint32_t* meshPartition,
    uint32_t meshPartitionMaxCount,
	float padding
)
{
	// Adjust padding for mesh size
	physx::PxBounds3 bounds;
	bounds.setEmpty();
	for (uint32_t i = 0; i < meshTriangleCount; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			bounds.include(mesh[i].vertices[j].position);
		}
	}
	padding *= bounds.getExtents().magnitude();

	physx::Array<uint32_t> partition;
	partitionMesh(partition, mesh, meshTriangleCount, padding);

	for (uint32_t i = 0; i < meshPartitionMaxCount && i < partition.size(); ++i)
	{
		meshPartition[i] = partition[i];
	}

	return partition.size();
}

bool buildExplicitHierarchicalMeshFromRenderMeshAsset(ExplicitHierarchicalMesh& iHMesh, const nvidia::apex::RenderMeshAsset& renderMeshAsset, uint32_t maxRootDepth)
{
	return buildExplicitHierarchicalMeshFromApexAssetsInternal(*(ExplicitHierarchicalMeshImpl*)&iHMesh, renderMeshAsset, NULL, maxRootDepth);
}

bool buildExplicitHierarchicalMeshFromDestructibleAsset(ExplicitHierarchicalMesh& iHMesh, const nvidia::apex::DestructibleAsset& destructibleAsset, uint32_t maxRootDepth)
{
	if (destructibleAsset.getChunkCount() == 0)
	{
		return false;
	}

	const nvidia::RenderMeshAsset* renderMeshAsset = destructibleAsset.getRenderMeshAsset();
	if (renderMeshAsset == NULL)
	{
		return false;
	}

	return buildExplicitHierarchicalMeshFromApexAssetsInternal(*(ExplicitHierarchicalMeshImpl*)&iHMesh, *renderMeshAsset, &destructibleAsset, maxRootDepth);
}


class MeshSplitter
{
public:
	virtual bool validate(ExplicitHierarchicalMeshImpl& hMesh) = 0;

	virtual void initialize(ExplicitHierarchicalMeshImpl& hMesh) = 0;

	virtual bool process
	(
		ExplicitHierarchicalMeshImpl& hMesh,
		uint32_t chunkIndex,
		const ApexCSG::IApexBSP& chunkBSP,
		const CollisionDesc& collisionDesc,
		nvidia::IProgressListener& progressListener,
		volatile bool* cancel
	) = 0;

	virtual bool finalize(ExplicitHierarchicalMeshImpl& hMesh) = 0;
};

static bool splitMeshInternal
(
	ExplicitHierarchicalMesh& iHMesh,
	ExplicitHierarchicalMesh& iHMeshCore,
	bool exportCoreMesh,
	int32_t coreMeshImprintSubmeshIndex,
	const MeshProcessingParameters& meshProcessingParams,
	MeshSplitter& splitter,
	const CollisionDesc& collisionDesc,
	uint32_t randomSeed,
	nvidia::IProgressListener& progressListener,
	volatile bool* cancel
)
{
	ExplicitHierarchicalMeshImpl& hMesh = *(ExplicitHierarchicalMeshImpl*)&iHMesh;
	ExplicitHierarchicalMeshImpl& hMeshCore = *(ExplicitHierarchicalMeshImpl*)&iHMeshCore;

	if (hMesh.partCount() == 0)
	{
		return false;
	}

	bool rootDepthIsZero = hMesh.mChunks[0]->isRootChunk();	// Until proven otherwise
	for (uint32_t chunkIndex = 1; rootDepthIsZero && chunkIndex < hMesh.chunkCount(); ++chunkIndex)
	{
		rootDepthIsZero = !hMesh.mChunks[chunkIndex]->isRootChunk();
	}

	if (!rootDepthIsZero && hMeshCore.partCount() > 0 && exportCoreMesh)
	{
		char message[1000];
		sprintf(message, "Warning: cannot export core mesh with multiple-mesh root mesh.  Will not export core.");
		outputMessage(message, physx::PxErrorCode::eDEBUG_WARNING);
		exportCoreMesh = false;
	}

	if (!splitter.validate(hMesh))
	{
		return false;
	}

	// Save state if cancel != NULL
	physx::PxFileBuf* save = NULL;
	class NullEmbedding : public ExplicitHierarchicalMesh::Embedding
	{
		void	serialize(physx::PxFileBuf& stream, Embedding::DataType type) const
		{
			(void)stream;
			(void)type;
		}
		void	deserialize(physx::PxFileBuf& stream, Embedding::DataType type, uint32_t version)
		{
			(void)stream;
			(void)type;
			(void)version;
		}
	} embedding;
	if (cancel != NULL)
	{
		save = nvidia::GetApexSDK()->createMemoryWriteStream();
		if (save != NULL)
		{
			hMesh.serialize(*save, embedding);			
		}
	}
	bool canceled = false;

	hMesh.buildCollisionGeometryForPart(0, getVolumeDesc(collisionDesc, 0));

	userRnd.m_rnd.setSeed(randomSeed);

	// Call initialization callback
	splitter.initialize(hMesh);

	// Make sure we've got BSPs at root depth
	for (uint32_t i = 0; i < hMesh.chunkCount(); ++i)
	{
		if (!hMesh.mChunks[i]->isRootLeafChunk())
		{
			continue;
		}
		uint32_t partIndex = (uint32_t)*hMesh.partIndex(i);
		if (hMesh.mParts[partIndex]->mMeshBSP->getType() != ApexCSG::BSPType::Nontrivial)
		{
			outputMessage("Building mesh BSP...");
			progressListener.setProgress(0);
			if (hMesh.calculatePartBSP(partIndex, randomSeed, meshProcessingParams.microgridSize, meshProcessingParams.meshMode, &progressListener, cancel))
			{
				outputMessage("Mesh BSP completed.");
			}
			else
			{
				outputMessage("Mesh BSP failed.");
				canceled = true;
			}
			userRnd.m_rnd.setSeed(randomSeed);
		}
	}

#if 0	// Debugging aid - uses BSP mesh generation to replace level 0 mesh
	hMesh.mParts[*hMesh.partIndex(0)]->mMeshBSP->toMesh(hMesh.mParts[0]->mMesh);
#endif

	hMesh.clear(true);

	ExplicitHierarchicalMeshImpl tempCoreMesh;

	uint32_t coreChunkIndex = 0xFFFFFFFF;
	uint32_t corePartIndex = 0xFFFFFFFF;
	if (hMeshCore.partCount() > 0 && !canceled)
	{
		// We have a core mesh.
		tempCoreMesh.set(iHMeshCore);

		if (exportCoreMesh)
		{
			// Use it as our first split
			// Core starts as original mesh
			coreChunkIndex = hMesh.addChunk();
			corePartIndex = hMesh.addPart();
			hMesh.mChunks[coreChunkIndex]->mPartIndex = (int32_t)corePartIndex;
			hMesh.mParts[corePartIndex]->mMesh = hMeshCore.mParts[0]->mMesh;
			hMesh.buildMeshBounds(corePartIndex);
			hMesh.buildCollisionGeometryForPart(corePartIndex, getVolumeDesc(collisionDesc, 1));
			hMesh.mChunks[coreChunkIndex]->mParentIndex = 0;
		}

		// Add necessary submesh data to hMesh from core.
		physx::Array<uint32_t> submeshMap(tempCoreMesh.mSubmeshData.size());
		if (exportCoreMesh || coreMeshImprintSubmeshIndex < 0)
		{
			for (uint32_t i = 0; i < tempCoreMesh.mSubmeshData.size(); ++i)
			{
				nvidia::ExplicitSubmeshData& coreSubmeshData = tempCoreMesh.mSubmeshData[i];
				submeshMap[i] = hMesh.mSubmeshData.size();
				for (uint32_t j = 0; j < hMesh.mSubmeshData.size(); ++j)
				{
					nvidia::ExplicitSubmeshData& submeshData = hMesh.mSubmeshData[j];
					if (0 == nvidia::strcmp(submeshData.mMaterialName, coreSubmeshData.mMaterialName))
					{
						submeshMap[i] = j;
						break;
					}
				}
				if (submeshMap[i] == hMesh.mSubmeshData.size())
				{
					hMesh.mSubmeshData.pushBack(coreSubmeshData);
				}
			}
		}

		if (coreMeshImprintSubmeshIndex >= (int32_t)hMesh.mSubmeshData.size())
		{
			coreMeshImprintSubmeshIndex = 0;
		}

		for (uint32_t i = 0; i < tempCoreMesh.chunkCount(); ++i)
		{
			if (!tempCoreMesh.mChunks[i]->isRootChunk())
			{
				continue;
			}

			// Remap materials
			uint32_t partIndex = (uint32_t)*tempCoreMesh.partIndex(i);
			for (uint32_t j = 0; j < tempCoreMesh.mParts[partIndex]->mMesh.size(); ++j)
			{
				nvidia::ExplicitRenderTriangle& tri = tempCoreMesh.mParts[partIndex]->mMesh[j];
				if (tri.submeshIndex >= 0 && tri.submeshIndex < (int32_t)submeshMap.size())
				{
					tri.submeshIndex = coreMeshImprintSubmeshIndex < 0 ? (int32_t)submeshMap[(uint32_t)tri.submeshIndex] : coreMeshImprintSubmeshIndex;
					if (exportCoreMesh && i == 0)
					{
						hMesh.mParts[corePartIndex]->mMesh[j].submeshIndex = (int32_t)submeshMap[(uint32_t)hMesh.mParts[corePartIndex]->mMesh[j].submeshIndex];
					}
				}
				else
				{
					tri.submeshIndex = coreMeshImprintSubmeshIndex;
				}
			}

			// Make sure we've got BSPs up to hMesh.mRootDepth
			if (tempCoreMesh.mParts[partIndex]->mMeshBSP->getType() != ApexCSG::BSPType::Nontrivial)
			{
				outputMessage("Building core mesh BSP...");
				progressListener.setProgress(0);
				if(tempCoreMesh.calculatePartBSP(partIndex, randomSeed, meshProcessingParams.microgridSize, meshProcessingParams.meshMode, &progressListener, cancel)) 
				{
					outputMessage("Core mesh BSP completed.");
				}
				else
				{
					outputMessage("Core mesh BSP calculation failed.");
					canceled = true;
				}
				userRnd.m_rnd.setSeed(randomSeed);
			}
		}
	}

	gIslandGeneration = meshProcessingParams.islandGeneration;
	gMicrogridSize = meshProcessingParams.microgridSize;
	gVerbosity = meshProcessingParams.verbosity;

	for (uint32_t chunkIndex = 0; chunkIndex < hMesh.mChunks.size() && !canceled; ++chunkIndex)
	{
		const uint32_t depth = hMesh.depth(chunkIndex);

		if (!hMesh.mChunks[chunkIndex]->isRootLeafChunk())
		{
			continue;	// Only process core leaf chunk
		}

		if (chunkIndex == coreChunkIndex)
		{
			continue;	// Ignore core chunk
		}

		uint32_t partIndex = (uint32_t)*hMesh.partIndex(chunkIndex);

		ApexCSG::IApexBSP* seedBSP = createBSP(hMesh.mBSPMemCache);
		seedBSP->copy(*hMesh.mParts[partIndex]->mMeshBSP);

		// Subtract out core
		bool partModified = false;
		for (uint32_t i = 0; i < tempCoreMesh.chunkCount(); ++i)
		{
			if (!tempCoreMesh.mChunks[i]->isRootLeafChunk())
			{
				continue;
			}
			uint32_t corePartIndex = (uint32_t)*tempCoreMesh.partIndex(i);
			if (tempCoreMesh.mParts[corePartIndex]->mMeshBSP != NULL)
			{
				ApexCSG::IApexBSP* rescaledCoreMeshBSP = createBSP(hMesh.mBSPMemCache);
				rescaledCoreMeshBSP->copy(*tempCoreMesh.mParts[corePartIndex]->mMeshBSP, physx::PxMat44(physx::PxIdentity), seedBSP->getInternalTransform());
				seedBSP->combine(*rescaledCoreMeshBSP);
				rescaledCoreMeshBSP->release();
				seedBSP->op(*seedBSP, ApexCSG::Operation::A_Minus_B);
				partModified = true;
			}
		}

		if (partModified && depth > 0)
		{
			// Create part from modified seedBSP (unless it's at level 0)
			seedBSP->toMesh(hMesh.mParts[partIndex]->mMesh);
			if (hMesh.mParts[partIndex]->mMesh.size() != 0)
			{
				hMesh.mParts[partIndex]->mMeshBSP->copy(*seedBSP);
				hMesh.buildCollisionGeometryForPart(partIndex, getVolumeDesc(collisionDesc, depth));
			}
		}

#if 0	// Should always have been true
		if (depth == hMesh.mRootDepth)
#endif
		{
			// At hMesh.mRootDepth - split
			outputMessage("Splitting...");
			progressListener.setProgress(0);
			canceled = !splitter.process(hMesh, chunkIndex, *seedBSP, collisionDesc, progressListener, cancel);
			outputMessage("splitting completed.");
		}

		seedBSP->release();
	}

	// Restore if canceled
	if (canceled && save != NULL)
	{
		uint32_t len;
		const void* mem = nvidia::GetApexSDK()->getMemoryWriteBuffer(*save, len);
		physx::PxFileBuf* load = nvidia::GetApexSDK()->createMemoryReadStream(mem, len);
		if (load != NULL)
		{
			hMesh.deserialize(*load, embedding);
			nvidia::GetApexSDK()->releaseMemoryReadStream(*load);
		}
	}

	if (save != NULL)
	{
		nvidia::GetApexSDK()->releaseMemoryReadStream(*save);
	}

	if (canceled)
	{
		return false;
	}

	if (meshProcessingParams.removeTJunctions && hMesh.mParts.size())
	{
		MeshProcessor meshProcessor;
		const float size = hMesh.mParts[0]->mBounds.getExtents().magnitude();
		for (uint32_t i = 0; i < hMesh.partCount(); ++i)
		{
			meshProcessor.setMesh(hMesh.mParts[i]->mMesh, NULL, 0, 0.0001f*size);
			meshProcessor.removeTJunctions();
		}
	}

	physx::Array<uint32_t> remap;
	hMesh.sortChunks(&remap);

	hMesh.createPartSurfaceNormals();

	if (corePartIndex < hMesh.partCount())
	{
		// Create reasonable collision hulls when there is a core mesh
		coreChunkIndex = remap[coreChunkIndex];
		const PxTransform idTM(PxIdentity);
		const physx::PxVec3 idScale(1.0f);
		for (uint32_t coreHullIndex = 0; coreHullIndex < hMesh.mParts[corePartIndex]->mCollision.size(); ++coreHullIndex)
		{
			const PartConvexHullProxy& coreHull = *hMesh.mParts[corePartIndex]->mCollision[coreHullIndex];
			for (uint32_t i = 1; i < hMesh.partCount(); ++i)
			{
				if (i == coreChunkIndex)
				{
					continue;
				}
				for (uint32_t hullIndex = 0; hullIndex < hMesh.mParts[i]->mCollision.size(); ++hullIndex)
				{
					PartConvexHullProxy& hull = *hMesh.mParts[i]->mCollision[hullIndex];
					ConvexHullImpl::Separation separation;
					if (ConvexHullImpl::hullsInProximity(coreHull.impl, idTM, idScale, hull.impl, idTM, idScale, 0.0f, &separation))
					{
						const float hullWidth = separation.max1 - separation.min1;
						const float overlap = separation.max0 - separation.min1;
						if (overlap < 0.25f * hullWidth)
						{
							// Trim the hull
							hull.impl.intersectPlaneSide(physx::PxPlane(-separation.plane.n, -separation.max0));
						}
					}
				}
			}
		}
	}

	return splitter.finalize(hMesh);
}

// Note: chunks must be in breadth-first order
static void deleteChunkChildren
(
 ExplicitHierarchicalMeshImpl& hMesh,
 uint32_t chunkIndex,
 bool deleteChunk = false
 )
{
	for (uint32_t index = hMesh.chunkCount(); index-- > chunkIndex+1;)
	{
		if (hMesh.mChunks[index]->mParentIndex == (int32_t)chunkIndex)
		{
			deleteChunkChildren(hMesh, index, true);
		}
	}

	if (deleteChunk)
	{
		const int32_t partIndex = hMesh.mChunks[chunkIndex]->mPartIndex;
		hMesh.removeChunk(chunkIndex);
		bool partIndexUsed = false;
		for (uint32_t index = 0; index < hMesh.chunkCount(); ++index)
		{
			if (hMesh.mChunks[index]->mPartIndex == partIndex)
			{
				partIndexUsed = true;
				break;
			}
		}
		if (!partIndexUsed)
		{
			hMesh.removePart((uint32_t)partIndex);
		}
	}
}

static bool splitChunkInternal
(
	ExplicitHierarchicalMesh& iHMesh,
	uint32_t chunkIndex,
	const FractureTools::MeshProcessingParameters& meshProcessingParams,
	MeshSplitter& splitter,
	const CollisionDesc& collisionDesc,
	uint32_t* randomSeed,
	IProgressListener& progressListener,
	volatile bool* cancel
)
{
	const int32_t* partIndexPtr = iHMesh.partIndex(chunkIndex);
	if (partIndexPtr == NULL)
	{
		return true;
	}
	const uint32_t partIndex = (uint32_t)*partIndexPtr;

	gIslandGeneration = meshProcessingParams.islandGeneration;
	gMicrogridSize = meshProcessingParams.microgridSize;
	gVerbosity = meshProcessingParams.verbosity;

	outputMessage("Splitting...");

	// Save state if cancel != NULL
	physx::PxFileBuf* save = NULL;
	class NullEmbedding : public ExplicitHierarchicalMesh::Embedding
	{
		void	serialize(physx::PxFileBuf& stream, Embedding::DataType type) const
		{
			(void)stream;
			(void)type;
		}
		void	deserialize(physx::PxFileBuf& stream, Embedding::DataType type, uint32_t version)
		{
			(void)stream;
			(void)type;
			(void)version;
		}
	} embedding;

	ExplicitHierarchicalMeshImpl& hMesh = *(ExplicitHierarchicalMeshImpl*)&iHMesh;
	
	if (cancel != NULL)
	{
		save = nvidia::GetApexSDK()->createMemoryWriteStream();
		if (save != NULL)
		{
			hMesh.serialize(*save, embedding);			
		}
	}
	bool canceled = false;

	progressListener.setProgress(0);

	// Delete chunk children
	deleteChunkChildren(hMesh, chunkIndex);

	// Reseed if requested
	if (randomSeed != NULL)
	{
		userRnd.m_rnd.setSeed(*randomSeed);
	}
	const uint32_t seed = userRnd.m_rnd.seed();

	// Fracture chunk
	ApexCSG::IApexBSP* chunkMeshBSP = hMesh.mParts[partIndex]->mMeshBSP;

	// Make sure we've got a BSP.  If this is a root chunk, it may not have been created yet.
	if (chunkMeshBSP->getType() != ApexCSG::BSPType::Nontrivial)
	{
		if (!hMesh.mChunks[chunkIndex]->isRootChunk())
		{
			outputMessage("Warning: Building a BSP for a non-root mesh.  This should have been created by a splitting process.");
		}
		outputMessage("Building mesh BSP...");
		progressListener.setProgress(0);
		hMesh.calculatePartBSP(partIndex, seed, meshProcessingParams.microgridSize, meshProcessingParams.meshMode, &progressListener);
		outputMessage("Mesh BSP completed.");
		userRnd.m_rnd.setSeed(seed);
	}

	const uint32_t oldPartCount = hMesh.mParts.size();

	canceled = !splitter.process(hMesh, chunkIndex, *chunkMeshBSP, collisionDesc, progressListener, cancel);

	// Restore if canceled
	if (canceled && save != NULL)
	{
		uint32_t len;
		const void* mem = nvidia::GetApexSDK()->getMemoryWriteBuffer(*save, len);
		physx::PxFileBuf* load = nvidia::GetApexSDK()->createMemoryReadStream(mem, len);
		if (load != NULL)
		{
			hMesh.deserialize(*load, embedding);
			nvidia::GetApexSDK()->releaseMemoryReadStream(*load);
		}
	}

	if (save != NULL)
	{
		nvidia::GetApexSDK()->releaseMemoryReadStream(*save);
	}

	if (canceled)
	{
		return false;
	}

	if (meshProcessingParams.removeTJunctions)
	{
		MeshProcessor meshProcessor;
		const float size = hMesh.mParts[partIndex]->mBounds.getExtents().magnitude();
		for (uint32_t i = oldPartCount; i < hMesh.partCount(); ++i)
		{
			meshProcessor.setMesh(hMesh.mParts[i]->mMesh, NULL, 0, 0.0001f*size);
			meshProcessor.removeTJunctions();
		}
	}

	hMesh.sortChunks();

	hMesh.createPartSurfaceNormals();

	return true;
}


static uint32_t createVoronoiSitesInsideMeshInternal
(
	ExplicitHierarchicalMeshImpl& hMesh,
	const uint32_t* chunkIndices,
	uint32_t chunkCount,
	physx::PxVec3* siteBuffer,
	uint32_t* siteChunkIndices,
	uint32_t siteCount,
	uint32_t* randomSeed,
	uint32_t* microgridSize,
	BSPOpenMode::Enum meshMode,
	nvidia::IProgressListener& progressListener
)
{
	if (randomSeed != NULL)
	{
		userRnd.m_rnd.setSeed(*randomSeed);
	}

	const uint32_t microgridSizeToUse = microgridSize != NULL ? *microgridSize : gMicrogridSize;

	// Make sure we've got BSPs for all chunks
	for (uint32_t chunkNum = 0; chunkNum < chunkCount; ++chunkNum)
	{
		const uint32_t chunkIndex = chunkIndices[chunkNum];
		uint32_t partIndex = (uint32_t)*hMesh.partIndex(chunkIndex);
		if (hMesh.mParts[partIndex]->mMeshBSP->getType() != ApexCSG::BSPType::Nontrivial)
		{
			outputMessage("Building mesh BSP...");
			progressListener.setProgress(0);
			if (randomSeed == NULL)
			{
				outputMessage("Warning: no random seed given in createVoronoiSitesInsideMeshInternal but BSP must be built.  Using seed = 0.", physx::PxErrorCode::eDEBUG_WARNING);
			}
			hMesh.calculatePartBSP(partIndex, (randomSeed != NULL ? *randomSeed : 0), microgridSizeToUse, meshMode, &progressListener);
			outputMessage("Mesh BSP completed.");
			if (randomSeed != NULL)
			{
				userRnd.m_rnd.setSeed(*randomSeed);
			}
		}
	}

	// Come up with distribution that is weighted by chunk volume, but also tries to ensure each chunk gets at least one site.
	float totalVolume = 0.0f;
	physx::Array<float> volumes(chunkCount);
	physx::Array<uint32_t> siteCounts(chunkCount);
	for (uint32_t chunkNum = 0; chunkNum < chunkCount; ++chunkNum)
	{
		const uint32_t chunkIndex = chunkIndices[chunkNum];
		const physx::PxBounds3 bounds = hMesh.chunkBounds(chunkIndex);
		const physx::PxVec3 extents = bounds.getExtents();
		volumes[chunkNum] = extents.x*extents.y*extents.z;
		totalVolume += volumes[chunkNum];
		siteCounts[chunkNum] = 0;
	}

	// Now fill in site counts
	if (totalVolume <= 0.0f)
	{
		totalVolume = 1.0f;	// To avoid divide-by-zero
	}

	// Make site count proportional to volume, within error due to quantization.  Ensure at least one site per chunk, even if "zero volume"
	// is recorded (it might be an open-meshed chunk).  We distinguish between zero and one sites per chunk, even though they have the same
	// effect on a chunk, since using one site per chunk will reduce the number of sites available for other chunks.  The aim is to have
	// control over the number of chunks generated, so we will avoid using zero sites per chunk.
	uint32_t totalSiteCount = 0;
	for (uint32_t chunkNum = 0; chunkNum < chunkCount; ++chunkNum)
	{
		siteCounts[chunkNum] = PxMax(1U, (uint32_t)(siteCount*volumes[chunkNum]/totalVolume));
		totalSiteCount += siteCounts[chunkNum];
	}

	// Add sites if we need to.  This can happen due to the rounding.
	while (totalSiteCount < siteCount)
	{
		uint32_t chunkToAddSite = 0;
		float greatestDeficit = -PX_MAX_F32;
		for (uint32_t chunkNum = 0; chunkNum < chunkCount; ++chunkNum)
		{
			const float defecit = siteCount*volumes[chunkNum]/totalVolume - (float)siteCounts[chunkNum];
			if (defecit > greatestDeficit)
			{
				greatestDeficit = defecit;
				chunkToAddSite = chunkNum;
			}
		}
		++siteCounts[chunkToAddSite];
		++totalSiteCount;
	}

	// Remove sites if necessary.  This is much more likely.
	while (totalSiteCount > siteCount)
	{
		uint32_t chunkToRemoveSite = 0;
		float greatestSurplus = -PX_MAX_F32;
		for (uint32_t chunkNum = 0; chunkNum < chunkCount; ++chunkNum)
		{
			const float surplus = (float)siteCounts[chunkNum] - siteCount*volumes[chunkNum]/totalVolume;
			if (surplus > greatestSurplus)
			{
				greatestSurplus = surplus;
				chunkToRemoveSite = chunkNum;
			}
		}
		--siteCounts[chunkToRemoveSite];
		--totalSiteCount;
	}

	// Now generate the actual sites
	uint32_t totalSitesGenerated = 0;
	for (uint32_t chunkNum = 0; chunkNum < chunkCount; ++chunkNum)
	{
		const uint32_t chunkIndex = chunkIndices[chunkNum];
		uint32_t partIndex = (uint32_t)*hMesh.partIndex(chunkIndex);
		ApexCSG::IApexBSP* meshBSP = hMesh.mParts[partIndex]->mMeshBSP;
		const physx::PxBounds3 bounds = hMesh.chunkBounds(chunkIndex);
		uint32_t sitesGenerated = 0;
		uint32_t attemptsLeft = 100000;
		while (	sitesGenerated < siteCounts[chunkNum])
		{
			const physx::PxVec3 site(userRnd.getReal(bounds.minimum.x, bounds.maximum.x), userRnd.getReal(bounds.minimum.y, bounds.maximum.y), userRnd.getReal(bounds.minimum.z, bounds.maximum.z));
			if (!attemptsLeft || meshBSP->pointInside(site - *hMesh.instancedPositionOffset(chunkIndex)))
			{
				siteBuffer[totalSitesGenerated] = site;
				if (siteChunkIndices != NULL)
				{
					siteChunkIndices[totalSitesGenerated] = chunkIndex;
				}
				++sitesGenerated;
				++totalSitesGenerated;
			}
			if (attemptsLeft)
			{
				--attemptsLeft;
			}
		}
	}

	return totalSitesGenerated;
}

class HierarchicalMeshSplitter : public MeshSplitter
{
private:
	HierarchicalMeshSplitter& operator=(const HierarchicalMeshSplitter&);

public:
	HierarchicalMeshSplitter(const FractureSliceDesc& desc) : mDesc(desc)
	{
	}

	bool validate(ExplicitHierarchicalMeshImpl& hMesh)
	{
		// Try to see if we're going to generate too many chunks
		uint32_t estimatedTotalChunkCount = 0;
		for (uint32_t chunkIndex = 0; chunkIndex < hMesh.chunkCount(); ++chunkIndex)
		{
			if (!hMesh.mChunks[chunkIndex]->isRootLeafChunk())
			{
				continue;
			}
			uint32_t partIndex = (uint32_t)*hMesh.partIndex(chunkIndex);
			uint32_t estimatedLevelChunkCount = 1;
			physx::PxVec3 estimatedExtent = hMesh.mParts[partIndex]->mBounds.getExtents();
			for (uint32_t chunkDepth = 0; chunkDepth < mDesc.maxDepth; ++chunkDepth)
			{
				// Get parameters for this depth
				const nvidia::SliceParameters& sliceParameters = mDesc.sliceParameters[chunkDepth];
				int partition[3];
				calculatePartition(partition, sliceParameters.splitsPerPass, estimatedExtent, mDesc.useTargetProportions ? mDesc.targetProportions : NULL);
				estimatedLevelChunkCount *= partition[0] * partition[1] * partition[2];
				estimatedTotalChunkCount += estimatedLevelChunkCount;
				if (estimatedTotalChunkCount > MAX_ALLOWED_ESTIMATED_CHUNK_TOTAL)
				{
					char message[1000];
					shdfnd::snprintf(message,1000, "Slicing chunk count is estimated to be %d chunks, exceeding the maximum allowed estimated total of %d chunks.  Aborting.  Try using fewer slices, or a smaller fracture depth.",
						estimatedTotalChunkCount, (int)MAX_ALLOWED_ESTIMATED_CHUNK_TOTAL);
					outputMessage(message, physx::PxErrorCode::eINTERNAL_ERROR);
					return false;
				}
				estimatedExtent[0] /= partition[0];
				estimatedExtent[1] /= partition[1];
				estimatedExtent[2] /= partition[2];
			}
		}

		return true;
	}

	void initialize(ExplicitHierarchicalMeshImpl& hMesh)
	{
		if (mDesc.useDisplacementMaps)
		{
			hMesh.initializeDisplacementMapVolume(mDesc);
		}

		for (int i = 0; i < 3; ++i)
		{
			hMesh.mSubmeshData.resize(PxMax(hMesh.mRootSubmeshCount, mDesc.materialDesc[i].interiorSubmeshIndex + 1));
		}
	}

	bool process
	(
		ExplicitHierarchicalMeshImpl& hMesh,
		uint32_t chunkIndex,
		const ApexCSG::IApexBSP& chunkBSP,
		const CollisionDesc& collisionDesc,
		nvidia::IProgressListener& progressListener,
		volatile bool* cancel
	)
	{
		physx::PxPlane trailingPlanes[3];	// passing in depth = 0 will initialize these
		physx::PxPlane leadingPlanes[3];
#if 1	// Eliminating volume calculation here, for performance.  May introduce it later once the mesh is calculated.
		const float chunkVolume = 1.0f;
#else
		float chunkArea, chunkVolume;
		chunkBSP.getSurfaceAreaAndVolume(chunkArea, chunkVolume, true);
#endif
		return hierarchicallySplitChunkInternal(hMesh, chunkIndex, 0, trailingPlanes, leadingPlanes, chunkBSP, chunkVolume, mDesc, collisionDesc, progressListener, cancel);
	}

	bool finalize(ExplicitHierarchicalMeshImpl& hMesh)
	{
		if (mDesc.instanceChunks)
		{
			for (uint32_t i = 0; i < hMesh.partCount(); ++i)
			{
				hMesh.mChunks[i]->mFlags |= nvidia::apex::DestructibleAsset::ChunkIsInstanced;
			}
		}

		return true;
	}

protected:
	const FractureSliceDesc& mDesc;
};

bool createHierarchicallySplitMesh
(
    ExplicitHierarchicalMesh& iHMesh,
    ExplicitHierarchicalMesh& iHMeshCore,
    bool exportCoreMesh,
	int32_t coreMeshImprintSubmeshIndex,	// If this is < 0, use the core mesh materials (was applyCoreMeshMaterialToNeighborChunks).  Otherwise, use the given submesh.
    const MeshProcessingParameters& meshProcessingParams,
    const FractureSliceDesc& desc,
    const CollisionDesc& collisionDesc,
    uint32_t randomSeed,
    nvidia::IProgressListener& progressListener,
    volatile bool* cancel
)
{
	HierarchicalMeshSplitter splitter(desc);

	return splitMeshInternal(
		iHMesh,
		iHMeshCore,
		exportCoreMesh,
		coreMeshImprintSubmeshIndex,
		meshProcessingParams,
		splitter,
		collisionDesc,
		randomSeed,
		progressListener,
		cancel);
}

bool hierarchicallySplitChunk
(
	ExplicitHierarchicalMesh& iHMesh,
	uint32_t chunkIndex,
	const FractureTools::MeshProcessingParameters& meshProcessingParams,
	const FractureTools::FractureSliceDesc& desc,
	const CollisionDesc& collisionDesc,
	uint32_t* randomSeed,
	IProgressListener& progressListener,
	volatile bool* cancel
)
{
	HierarchicalMeshSplitter splitter(desc);

	return splitChunkInternal(iHMesh, chunkIndex, meshProcessingParams, splitter, collisionDesc, randomSeed, progressListener, cancel);
}

PX_INLINE PxMat44 createCutoutFrame(const physx::PxVec3& center, const physx::PxVec3& extents, uint32_t sliceAxes[3], uint32_t sliceSignNum, const FractureCutoutDesc& desc, bool& invertX)
{
	const uint32_t sliceDirIndex = sliceAxes[2] * 2 + sliceSignNum;
	const float sliceSign = sliceSignNum ? -1.0f : 1.0f;
	physx::PxVec3 n = createAxis(sliceAxes[2]) * sliceSign;
	PxMat44 cutoutTM;
	cutoutTM.column2 = PxVec4(n, 0.f);
	float applySign;
	switch (sliceAxes[2])
	{
	case 0:
		applySign = 1.0f;
		break;
	case 1:
		applySign = -1.0f;
		break;
	default:
	case 2:
		applySign = 1.0f;
	}
	const physx::PxVec3 p = createAxis(sliceAxes[1]);
	const float cutoutPadding = desc.tileFractureMap ? 0.0f : 0.0001f;
	cutoutTM.column1 = PxVec4(p, 0.f);
	cutoutTM.column0 = PxVec4(p.cross(n), 0.f);
	float cutoutWidth = 2 * extents[sliceAxes[0]] * (1.0f + cutoutPadding);
	float cutoutHeight = 2 * extents[sliceAxes[1]] * (1.0f + cutoutPadding);
	cutoutWidth *= (desc.cutoutWidthInvert[sliceDirIndex] ? -1.0f : 1.0f) * desc.cutoutWidthScale[sliceDirIndex];
	cutoutHeight *= (desc.cutoutHeightInvert[sliceDirIndex] ? -1.0f : 1.0f) * desc.cutoutHeightScale[sliceDirIndex];
	cutoutTM.scale(physx::PxVec4(cutoutWidth / desc.cutoutSizeX, cutoutHeight / desc.cutoutSizeY, 1.0f, 1.0f));
	cutoutTM.setPosition(physx::PxVec3(0.0f));
	float sign = applySign * sliceSign;
	invertX = sign < 0.0f;

	PxVec3 cutoutPosition(0.0, 0.0, 0.0);

	cutoutPosition[sliceAxes[0]] = center[sliceAxes[0]] - sign * (0.5f * cutoutWidth + desc.cutoutWidthOffset[sliceDirIndex] * extents[sliceAxes[0]]);
	cutoutPosition[sliceAxes[1]] = center[sliceAxes[1]] - 0.5f * cutoutHeight + desc.cutoutHeightOffset[sliceDirIndex] * extents[sliceAxes[1]];

	cutoutTM.setPosition(cutoutPosition);
	return cutoutTM;
}

static bool createCutoutChunk(ExplicitHierarchicalMeshImpl& hMesh, ApexCSG::IApexBSP& cutoutBSP, /*IApexBSP& remainderBSP,*/
							  const ApexCSG::IApexBSP& sourceBSP, uint32_t sourceIndex,
                              const CollisionVolumeDesc& volumeDesc, volatile bool* cancel)
{
//	remainderBSP.combine( cutoutBSP );
//	remainderBSP.op( remainderBSP, Operation::A_Minus_B );
	cutoutBSP.combine(sourceBSP);
	cutoutBSP.op(cutoutBSP, ApexCSG::Operation::Intersection);
	// BRG - should apply island generation here
//	if( gIslandGeneration )
//	{
//	}
	const uint32_t newPartIndex = hMesh.addPart();
	hMesh.mParts[newPartIndex]->mMeshBSP->release();
	hMesh.mParts[newPartIndex]->mMeshBSP = &cutoutBSP;	// Save off and own this
	cutoutBSP.toMesh(hMesh.mParts[newPartIndex]->mMesh);
	if (hMesh.mParts[newPartIndex]->mMesh.size() > 0)
	{
		hMesh.mParts[newPartIndex]->mMeshBSP->copy(cutoutBSP);
		hMesh.buildMeshBounds(newPartIndex);
		hMesh.buildCollisionGeometryForPart(newPartIndex, volumeDesc);
		const uint32_t newChunkIndex = hMesh.addChunk();
		hMesh.mChunks[newChunkIndex]->mParentIndex = (int32_t)sourceIndex;
		hMesh.mChunks[newChunkIndex]->mPartIndex = (int32_t)newPartIndex;
	}
	else
	{
		hMesh.removePart(newPartIndex);
	}

	return cancel == NULL || !(*cancel);
}

static void addQuad(ExplicitHierarchicalMeshImpl& hMesh, physx::Array<nvidia::ExplicitRenderTriangle>& mesh, uint32_t sliceDepth, uint32_t submeshIndex,
					const physx::PxVec2& interiorUVScale, const physx::PxVec3& v0, const physx::PxVec3& v1, const physx::PxVec3& v2, const physx::PxVec3& v3)
{
	// Create material frame TM
	const uint32_t materialIndex = hMesh.addMaterialFrame();
	nvidia::MaterialFrame materialFrame = hMesh.getMaterialFrame(materialIndex);

	nvidia::FractureMaterialDesc materialDesc;

	/* BRG: these should be obtained from an alternative set of material descs (one for each cutout direction), which describe the UV layout around the chunk cutout. */
	materialDesc.uAngle = 0.0f;
	materialDesc.uvOffset = physx::PxVec2(0.0f);
	materialDesc.uvScale = interiorUVScale;

	materialDesc.tangent = v1 - v0;
	materialDesc.tangent.normalize();
	physx::PxVec3 normal = materialDesc.tangent.cross(v3 - v0);
	normal.normalize();
	const physx::PxPlane plane(v0, normal);

	materialFrame.buildCoordinateSystemFromMaterialDesc(materialDesc, plane);
	materialFrame.mFractureMethod = nvidia::FractureMethod::Cutout;
	materialFrame.mFractureIndex = -1;	// Signifying that it's a cutout around the chunk, so we shouldn't make assumptions about the face direction
	materialFrame.mSliceDepth = sliceDepth;

	hMesh.setMaterialFrame(materialIndex, materialFrame);

	// Create interpolator for triangle quantities

	nvidia::TriangleFrame triangleFrame(materialFrame.mCoordinateSystem, interiorUVScale, physx::PxVec2(0.0f));

	// Fill one triangle
	nvidia::ExplicitRenderTriangle& tri0 = mesh.insert();
	memset(&tri0, 0, sizeof(nvidia::ExplicitRenderTriangle));
	tri0.submeshIndex = (int32_t)submeshIndex;
	tri0.extraDataIndex = materialIndex;
	tri0.smoothingMask = 0;
	tri0.vertices[0].position = v0;
	tri0.vertices[1].position = v1;
	tri0.vertices[2].position = v2;
	for (int i = 0; i < 3; ++i)
	{
		triangleFrame.interpolateVertexData(tri0.vertices[i]);
	}

	// ... and another
	nvidia::ExplicitRenderTriangle& tri1 = mesh.insert();
	memset(&tri1, 0, sizeof(nvidia::ExplicitRenderTriangle));
	tri1.submeshIndex = (int32_t)submeshIndex;
	tri1.extraDataIndex = materialIndex;
	tri1.smoothingMask = 0;
	tri1.vertices[0].position = v2;
	tri1.vertices[1].position = v3;
	tri1.vertices[2].position = v0;
	for (int i = 0; i < 3; ++i)
	{
		triangleFrame.interpolateVertexData(tri1.vertices[i]);
	}
}

float getDeterminant(const physx::PxMat44& mt)
{
	return mt.column0.getXYZ().dot(mt.column1.getXYZ().cross(mt.column2.getXYZ()));
}

static bool createCutout(
	ExplicitHierarchicalMeshImpl& hMesh,
	uint32_t faceChunkIndex,
	ApexCSG::IApexBSP& faceBSP,	// May be modified
	const nvidia::Cutout& cutout,
	const PxMat44& cutoutTM,
	const nvidia::FractureCutoutDesc& desc,
	const nvidia::NoiseParameters& edgeNoise,
	float cutoutDepth,
	const nvidia::FractureMaterialDesc& materialDesc,
	const CollisionVolumeDesc& volumeDesc,
	const physx::PxPlane& minPlane,
	const physx::PxPlane& maxPlane,
	nvidia::HierarchicalProgressListener& localProgressListener,
	volatile bool* cancel)
{
	bool canceled = false;

	const float cosSmoothingThresholdAngle = physx::PxCos(desc.facetNormalMergeThresholdAngle * physx::PxPi / 180.0f);

	physx::Array<physx::PxPlane> trimPlanes;
	const uint32_t oldPartCount = hMesh.partCount();
	localProgressListener.setSubtaskWork(1);
	const uint32_t loopCount = desc.splitNonconvexRegions ? cutout.convexLoops.size() : 1;

	const bool ccw = getDeterminant(cutoutTM) > (float)0;

	const uint32_t facePartIndex = (uint32_t)*hMesh.partIndex(faceChunkIndex);

	const uint32_t sliceDepth = hMesh.depth(faceChunkIndex) + 1;

	for (uint32_t j = 0; j < loopCount && !canceled; ++j)
	{
		const uint32_t loopSize = desc.splitNonconvexRegions ? cutout.convexLoops[j].polyVerts.size() : cutout.vertices.size();
		if (desc.splitNonconvexRegions)
		{
			trimPlanes.reset();
		}
		// Build mesh which surrounds the cutout
		physx::Array<nvidia::ExplicitRenderTriangle> loopMesh;
		for (uint32_t k = 0; k < loopSize; ++k)
		{
			uint32_t kPrime = ccw ? k : loopSize - 1 - k;
			uint32_t kPrimeNext = ccw ? ((kPrime + 1) % loopSize) : (kPrime == 0 ? (loopSize - 1) : (kPrime-1));
			const uint32_t vertexIndex0 = desc.splitNonconvexRegions ? cutout.convexLoops[j].polyVerts[kPrime].index : kPrime;
			const uint32_t vertexIndex1 = desc.splitNonconvexRegions ? cutout.convexLoops[j].polyVerts[kPrimeNext].index : kPrimeNext;
			const physx::PxVec3& v0 = cutout.vertices[vertexIndex0];
			const physx::PxVec3& v1 = cutout.vertices[vertexIndex1];
			const physx::PxVec3 v0World = cutoutTM.transform(v0);
			const physx::PxVec3 v1World = cutoutTM.transform(v1);
			const physx::PxVec3 quad0 = minPlane.project(v0World);
			const physx::PxVec3 quad1 = minPlane.project(v1World);
			const physx::PxVec3 quad2 = maxPlane.project(v1World);
			const physx::PxVec3 quad3 = maxPlane.project(v0World);
			addQuad(hMesh, loopMesh, sliceDepth, materialDesc.interiorSubmeshIndex, materialDesc.uvScale, quad0, quad1, quad2, quad3);
			if (cutout.convexLoops.size() == 1 || desc.splitNonconvexRegions)
			{
				physx::PxVec3 planeNormal = (quad2 - quad0).cross(quad3 - quad1);
				planeNormal.normalize();
				trimPlanes.pushBack(physx::PxPlane(0.25f * (quad0 + quad1 + quad2 + quad3), planeNormal));
			}
		}
		// Smooth the mesh's normals and tangents
		PX_ASSERT(loopMesh.size() == 2 * loopSize);
		if (loopMesh.size() == 2 * loopSize)
		{
			for (uint32_t k = 0; k < loopSize; ++k)
			{
				const uint32_t triIndex0 = 2 * k;
				const uint32_t frameIndex = loopMesh[triIndex0].extraDataIndex;
				PX_ASSERT(frameIndex == loopMesh[triIndex0 + 1].extraDataIndex);
				physx::PxMat44& frame = hMesh.mMaterialFrames[frameIndex].mCoordinateSystem;
				const uint32_t triIndex2 = 2 * ((k + 1) % loopSize);
				const uint32_t nextFrameIndex = loopMesh[triIndex2].extraDataIndex;
				PX_ASSERT(nextFrameIndex == loopMesh[triIndex2 + 1].extraDataIndex);
				physx::PxMat44& nextFrame = hMesh.mMaterialFrames[nextFrameIndex].mCoordinateSystem;
				const physx::PxVec3 normalK = frame.column2.getXYZ();
				const physx::PxVec3 normalK1 = nextFrame.column2.getXYZ();
				if (normalK.dot(normalK1) < cosSmoothingThresholdAngle)
				{
					continue;
				}
				physx::PxVec3 normal = normalK + normalK1;
				normal.normalize();
				loopMesh[triIndex0].vertices[1].normal = normal;
				loopMesh[triIndex0].vertices[2].normal = normal;
				loopMesh[triIndex0 + 1].vertices[0].normal = normal;
				loopMesh[triIndex2].vertices[0].normal = normal;
				loopMesh[triIndex2 + 1].vertices[1].normal = normal;
				loopMesh[triIndex2 + 1].vertices[2].normal = normal;
			}
			for (uint32_t k = 0; k < loopMesh.size(); ++k)
			{
				nvidia::ExplicitRenderTriangle& tri = loopMesh[k];
				for (uint32_t v = 0; v < 3; ++v)
				{
					nvidia::Vertex& vert = tri.vertices[v];
					vert.tangent = vert.binormal.cross(vert.normal);
				}
			}
		}
		// Create loop cutout BSP
		ApexCSG::IApexBSP* loopBSP = createBSP(hMesh.mBSPMemCache);
		ApexCSG::BSPBuildParameters bspBuildParams = gDefaultBuildParameters;
		bspBuildParams.rnd = &userRnd;
		bspBuildParams.internalTransform = faceBSP.getInternalTransform();
		loopBSP->fromMesh(&loopMesh[0], loopMesh.size(), bspBuildParams);
		const uint32_t oldSize = hMesh.partCount();
		// loopBSP will be modified and owned by the new chunk.
		canceled = !createCutoutChunk(hMesh, *loopBSP, faceBSP, /**cutoutSource,*/ faceChunkIndex, volumeDesc, cancel);
		for (uint32_t partN = oldSize; partN < hMesh.partCount(); ++partN)
		{
			// Apply graphical noise to new parts, if requested
			if (edgeNoise.amplitude > 0.0f)
			{
				applyNoiseToChunk(hMesh, facePartIndex, partN, edgeNoise, cutoutDepth);
			}
			// Trim new part collision hulls
			for (uint32_t trimN = 0; trimN < trimPlanes.size(); ++trimN)
			{
				for (uint32_t hullIndex = 0; hullIndex < hMesh.mParts[partN]->mCollision.size(); ++hullIndex)
				{
					hMesh.mParts[partN]->mCollision[hullIndex]->impl.intersectPlaneSide(trimPlanes[trimN]);
				}
			}
		}
		localProgressListener.completeSubtask();
	}
	// Trim hulls
	if (!canceled)
	{
		for (uint32_t partN = oldPartCount; partN < hMesh.partCount(); ++partN)
		{
			for (uint32_t hullIndex = 0; hullIndex < hMesh.mParts[partN]->mCollision.size(); ++hullIndex)
			{
				ConvexHullImpl& hull = hMesh.mParts[partN]->mCollision[hullIndex]->impl;
				if (!desc.splitNonconvexRegions)
				{
					for (uint32_t trimN = 0; trimN < trimPlanes.size(); ++trimN)
					{
						hull.intersectPlaneSide(trimPlanes[trimN]);
					}
				}
//				hull.intersectHull(hMesh.mParts[faceChunkIndex]->mCollision.impl);	// Do we need this?
			}
		}
	}

	return !canceled;
}

static void instanceChildren(ExplicitHierarchicalMeshImpl& hMesh, uint32_t instancingChunkIndex, uint32_t instancedChunkIndex)
{
	for (uint32_t chunkIndex = 0; chunkIndex < hMesh.chunkCount(); ++chunkIndex)
	{
		if (hMesh.mChunks[chunkIndex]->mParentIndex == (int32_t)instancingChunkIndex)
		{
			// Found a child.  Instance.
			const uint32_t instancedChildIndex = hMesh.addChunk();
			hMesh.mChunks[instancedChildIndex]->mFlags |= nvidia::apex::DestructibleAsset::ChunkIsInstanced;
			hMesh.mChunks[instancedChildIndex]->mParentIndex = (int32_t)instancedChunkIndex;
			hMesh.mChunks[instancedChildIndex]->mPartIndex = hMesh.mChunks[chunkIndex]->mPartIndex;	// Same part as instancing child
			hMesh.mChunks[instancedChildIndex]->mInstancedPositionOffset = hMesh.mChunks[instancedChunkIndex]->mInstancedPositionOffset;	// Same offset as parent
			hMesh.mChunks[instancedChildIndex]->mInstancedUVOffset = hMesh.mChunks[instancedChunkIndex]->mInstancedUVOffset;	// Same offset as parent
			instanceChildren(hMesh, chunkIndex, instancedChildIndex);	// Recurse
		}
	}
}

static bool createFaceCutouts
(
    ExplicitHierarchicalMeshImpl& hMesh,
    uint32_t faceChunkIndex,
    ApexCSG::IApexBSP& faceBSP,	// May be modified
    const nvidia::FractureCutoutDesc& desc,
	const nvidia::NoiseParameters& edgeNoise,
	float cutoutDepth,
	const nvidia::FractureMaterialDesc& materialDesc,
    const nvidia::CutoutSetImpl& cutoutSet,
	const PxMat44& cutoutTM,
	const float mapXLow,
	const float mapYLow,
	const CollisionDesc& collisionDesc,
    const nvidia::FractureSliceDesc& sliceDesc,
	const nvidia::FractureVoronoiDesc& voronoiDesc,
	const physx::PxPlane& facePlane,
    const physx::PxVec3& localCenter,
    const physx::PxVec3& localExtents,
    nvidia::IProgressListener& progressListener,
    volatile bool* cancel
)
{
	nvidia::FractureVoronoiDesc cutoutVoronoiDesc;
	physx::Array<physx::PxVec3> perChunkSites;

	switch (desc.chunkFracturingMethod)
	{
	case FractureCutoutDesc::VoronoiFractureCutoutChunks:
		{
			cutoutVoronoiDesc = voronoiDesc;
			perChunkSites.resize(voronoiDesc.siteCount);
			cutoutVoronoiDesc.sites = voronoiDesc.siteCount > 0 ? &perChunkSites[0] : NULL;
			cutoutVoronoiDesc.siteCount = voronoiDesc.siteCount;
		}
		break;
	}

	//	IApexBSP* cutoutSource = createBSP( hMesh.mBSPMemCache );
	//	cutoutSource->copy( faceBSP );

	const physx::PxVec3 faceNormal = facePlane.n;

	// "Sandwich" planes
	const float centerDisp = -facePlane.d - localExtents[2];
	const float paddedExtent = 1.01f * localExtents[2];
	const physx::PxPlane maxPlane(faceNormal, -centerDisp - paddedExtent);
	const physx::PxPlane minPlane(faceNormal, -centerDisp + paddedExtent);

	bool canceled = false;

	// Tiling bounds
	const float xTol = CUTOUT_MAP_BOUNDS_TOLERANCE*localExtents[0];
	const float yTol = CUTOUT_MAP_BOUNDS_TOLERANCE*localExtents[1];
	const float mapWidth = cutoutTM.column0.magnitude()*cutoutSet.getDimensions()[0];
	const float mapHeight = cutoutTM.column1.magnitude()*cutoutSet.getDimensions()[1];
	const float boundsXLow = localCenter[0] - localExtents[0];
	const float boundsWidth = 2*localExtents[0];
	const float boundsYLow = localCenter[1] - localExtents[1];
	const float boundsHeight = 2*localExtents[1];
	int32_t ixStart = desc.tileFractureMap ? (int32_t)physx::PxFloor((boundsXLow - mapXLow)/mapWidth + xTol) : 0;
	int32_t ixStop = desc.tileFractureMap ? (int32_t)physx::PxCeil((boundsXLow - mapXLow + boundsWidth)/mapWidth - xTol) : 1;
	int32_t iyStart = desc.tileFractureMap ? (int32_t)physx::PxFloor((boundsYLow - mapYLow)/mapHeight + yTol) : 0;
	int32_t iyStop = desc.tileFractureMap ? (int32_t)physx::PxCeil((boundsYLow - mapYLow + boundsHeight)/mapHeight - yTol) : 1;

	// Find UV map

	const physx::PxVec3 xDir = cutoutTM.column0.getXYZ().getNormalized();
	const physx::PxVec3 yDir = cutoutTM.column1.getXYZ().getNormalized();

	// First find a good representative face triangle
	const float faceDiffTolerance = 0.001f;
	uint32_t uvMapTriangleIndex = 0;
	float uvMapTriangleIndexFaceDiff = PX_MAX_F32;
	float uvMapTriangleIndexArea = 0.0f;
	const uint32_t facePartIndex = (uint32_t)*hMesh.partIndex(faceChunkIndex);
	const uint32_t facePartTriangleCount = hMesh.meshTriangleCount(facePartIndex);
	const nvidia::ExplicitRenderTriangle* facePartTriangles = hMesh.meshTriangles(facePartIndex);
	for (uint32_t triN = 0; triN < facePartTriangleCount; ++triN)
	{
		const nvidia::ExplicitRenderTriangle& tri = facePartTriangles[triN];
		physx::PxVec3 triNormal = (tri.vertices[1].position - tri.vertices[0].position).cross(tri.vertices[2].position - tri.vertices[0].position);
		const float triArea = triNormal.normalize();	// Actually twice the area, but it's OK
		const float triFaceDiff = (faceNormal-triNormal).magnitude();
		if (triFaceDiff < uvMapTriangleIndexFaceDiff - faceDiffTolerance || (triFaceDiff < uvMapTriangleIndexFaceDiff + faceDiffTolerance && triArea > uvMapTriangleIndexArea))
		{	// Significantly better normal, or normal is close and the area is bigger
			uvMapTriangleIndex = triN;
			uvMapTriangleIndexFaceDiff = triFaceDiff;
			uvMapTriangleIndexArea = triArea;
		}
	}

	// Set up interpolation for UV channel 0
	nvidia::TriangleFrame uvMapTriangleFrame(facePartTriangles[uvMapTriangleIndex], (uint64_t)1<<nvidia::TriangleFrame::UV0_u | (uint64_t)1<<nvidia::TriangleFrame::UV0_v);

	if (cutoutSet.isPeriodic())
	{
		--ixStart;
		++ixStop;
		--iyStart;
		++iyStop;
	}

#define FORCE_INSTANCING 0
#if !FORCE_INSTANCING
	const float volumeTol = PxMax(localExtents[0]*localExtents[1]*localExtents[2]*MESH_INSTANACE_TOLERANCE*MESH_INSTANACE_TOLERANCE*MESH_INSTANACE_TOLERANCE, (float)1.0e-15);
//	const float areaTol = PxMax((localExtents[0]*localExtents[1]+localExtents[1]*localExtents[2]+localExtents[2]*localExtents[0])*MESH_INSTANACE_TOLERANCE*MESH_INSTANACE_TOLERANCE, (float)1.0e-10);
#endif

	const bool instanceCongruentChunks = desc.instancingMode == FractureCutoutDesc::InstanceCongruentChunks || desc.instancingMode == FractureCutoutDesc::InstanceAllChunks;

	// Estimate total work for progress
	uint32_t totalWork = 0;
	for (uint32_t i = 0; i < cutoutSet.cutouts.size(); ++i)
	{
		totalWork += cutoutSet.cutouts[i].convexLoops.size();
	}
	totalWork *= (ixStop-ixStart)*(iyStop-iyStart);
	nvidia::HierarchicalProgressListener localProgressListener(PxMax((int)totalWork, 1), &progressListener);

	physx::Array<uint32_t> unhandledChunks;

	if (cutoutDepth == 0.0f)
	{
		cutoutDepth = 2*localExtents[2];	// handle special case of all-the-way-through cutout.  cutoutDepth is only used for noise grid calculations
	}

	const unsigned cutoutChunkDepth = hMesh.depth(faceChunkIndex) + 1;

	// Loop over cutouts on the outside loop.  For each cutout, create all tiled (potential) clones
	for (uint32_t i = 0; i < cutoutSet.cutouts.size() && !canceled; ++i)
	{
		// Keep track of starting chunk count.  We will process the newly created chunks below.
		const uint32_t oldChunkCount = hMesh.chunkCount();

		for (int32_t iy = iyStart; iy < iyStop && !canceled; ++iy)
		{
			for (int32_t ix = ixStart; ix < ixStop && !canceled; ++ix)
			{
				physx::PxVec3 offset = (float)ix*mapWidth*xDir + (float)iy*mapHeight*yDir;
				nvidia::Vertex interpolation;
				interpolation.position = offset;
				uvMapTriangleFrame.interpolateVertexData(interpolation);
				// BRG - note bizarre need to flip v...
				physx::PxVec2 uvOffset(interpolation.uv[0].u, -interpolation.uv[0].v);
				PxMat44 offsetCutoutTM(cutoutTM);
				offsetCutoutTM.setPosition(offsetCutoutTM.getPosition() + offset);
				const uint32_t newChunkIndex = hMesh.chunkCount();
				canceled = !createCutout(hMesh, faceChunkIndex, faceBSP, cutoutSet.cutouts[i], offsetCutoutTM, desc, edgeNoise, cutoutDepth, materialDesc, getVolumeDesc(collisionDesc, cutoutChunkDepth), minPlane, maxPlane, localProgressListener, cancel);
				if (!canceled && instanceCongruentChunks && newChunkIndex < hMesh.chunkCount())
				{
					PX_ASSERT(newChunkIndex + 1 == hMesh.chunkCount());
					if (newChunkIndex + 1 == hMesh.chunkCount())
					{
						hMesh.mChunks[newChunkIndex]->mInstancedPositionOffset = offset;
						hMesh.mChunks[newChunkIndex]->mInstancedUVOffset = uvOffset;
					}
				}
			}
		}

		// Keep track of which chunks we've checked for congruence
		const uint32_t possibleCongruentChunkCount = hMesh.chunkCount()-oldChunkCount;

		unhandledChunks.resize(possibleCongruentChunkCount);
		uint32_t index = hMesh.chunkCount();
		for (uint32_t i = 0; i < possibleCongruentChunkCount; ++i)
		{
			unhandledChunks[i] = --index;
		}

		uint32_t unhandledChunkCount = possibleCongruentChunkCount;
		while (unhandledChunkCount > 0)
		{
			// Have a fresh chunk to test for instancing.
			const uint32_t chunkIndex = unhandledChunks[--unhandledChunkCount];
			ExplicitHierarchicalMeshImpl::Chunk* chunk = hMesh.mChunks[chunkIndex];

			// Record its offset and rebase
			const physx::PxVec3 instancingBaseOffset = chunk->mInstancedPositionOffset;
			const physx::PxVec2 instancingBaseUVOffset = chunk->mInstancedUVOffset;
			chunk->mInstancedPositionOffset = physx::PxVec3(0.0f);
			chunk->mInstancedUVOffset = physx::PxVec2(0.0f);

			// If this option is selected, slice regions further
			switch (desc.chunkFracturingMethod)
			{
			case FractureCutoutDesc::SliceFractureCutoutChunks:
				{
					// Split hierarchically
					physx::PxPlane trailingPlanes[3];	// passing in depth = 0 will initialize these
					physx::PxPlane leadingPlanes[3];
#if 1	// Eliminating volume calculation here, for performance.  May introduce it later once the mesh is calculated.
					const float bspVolume = 1.0f;
#else
					float bspArea, bspVolume;
					chunkBSP->getSurfaceAreaAndVolume(bspArea, bspVolume, true);
#endif
					canceled = !hierarchicallySplitChunkInternal(hMesh, chunkIndex, 0, trailingPlanes, leadingPlanes, *hMesh.mParts[(uint32_t)chunk->mPartIndex]->mMeshBSP, bspVolume, sliceDesc, collisionDesc, localProgressListener, cancel);
				}
				break;
			case FractureCutoutDesc::VoronoiFractureCutoutChunks:
				{
					// Voronoi split
					cutoutVoronoiDesc.siteCount = createVoronoiSitesInsideMeshInternal(hMesh, &chunkIndex, 1, voronoiDesc.siteCount > 0 ? &perChunkSites[0] : NULL, NULL, voronoiDesc.siteCount, NULL, &gMicrogridSize, gMeshMode, progressListener );
					canceled = !voronoiSplitChunkInternal(hMesh, chunkIndex, *hMesh.mParts[(uint32_t)chunk->mPartIndex]->mMeshBSP, cutoutVoronoiDesc, collisionDesc, localProgressListener, cancel);
				}
				break;
			}

			// Now see if we can instance this chunk
			if (unhandledChunkCount > 0)
			{
				bool congruentChunkFound = false;
				uint32_t testChunkCount = unhandledChunkCount;
				while (testChunkCount > 0)
				{
					const uint32_t testChunkIndex = unhandledChunks[--testChunkCount];
					ExplicitHierarchicalMeshImpl::Chunk* testChunk = hMesh.mChunks[testChunkIndex];
					const uint32_t testPartIndex = (uint32_t)testChunk->mPartIndex;
					ExplicitHierarchicalMeshImpl::Part* testPart = hMesh.mParts[testPartIndex];

					// Create a shifted BSP of the test chunk
					ApexCSG::IApexBSP* combinedBSP = createBSP(hMesh.mBSPMemCache);
					const physx::PxMat44 tm(physx::PxMat33(physx::PxIdentity), instancingBaseOffset-testChunk->mInstancedPositionOffset);
					combinedBSP->copy(*testPart->mMeshBSP, tm);
					combinedBSP->combine(*hMesh.mParts[(uint32_t)chunk->mPartIndex]->mMeshBSP);
					float xorArea, xorVolume;
					combinedBSP->getSurfaceAreaAndVolume(xorArea, xorVolume, true, ApexCSG::Operation::Exclusive_Or);
					combinedBSP->release();
					if (xorVolume <= volumeTol)
					{
						// XOR of the two volumes is nearly zero.  Consider these chunks to be congruent, and instance.
						congruentChunkFound = true;
						testChunk->mInstancedPositionOffset -= instancingBaseOffset;	// Correct offset
						testChunk->mInstancedUVOffset -= instancingBaseUVOffset;		// Correct offset
						testChunk->mFlags |= nvidia::apex::DestructibleAsset::ChunkIsInstanced;		// Set instance flag
						hMesh.removePart((uint32_t)testChunk->mPartIndex);	// Remove part for this chunk, since we'll be instancing another part
						testChunk->mPartIndex = chunk->mPartIndex;
						instanceChildren(hMesh, chunkIndex, testChunkIndex);	// Recursive
						--unhandledChunkCount;	// This chunk is handled now
						nvidia::swap(unhandledChunks[unhandledChunkCount],unhandledChunks[testChunkCount]);	// Keep the unhandled chunk array packed
					}
				}
				
				// If the chunk is instanced, then mark it so
				if (congruentChunkFound)
				{
					chunk->mFlags |= nvidia::apex::DestructibleAsset::ChunkIsInstanced;
				}
			}
		}

		// Second pass at cutout chunks
		for (uint32_t j = 0; j < possibleCongruentChunkCount && !canceled; ++j)
		{
			ExplicitHierarchicalMeshImpl::Chunk* chunk = hMesh.mChunks[oldChunkCount+j];
			if ((chunk->mFlags & nvidia::apex::DestructibleAsset::ChunkIsInstanced) == 0)
			{
				// This chunk will not be instanced.  Zero its offsets.
				chunk->mInstancedPositionOffset = physx::PxVec3(0.0f);
				chunk->mInstancedUVOffset = physx::PxVec2(0.0f);
			}
		}
	}

//	cutoutSource->release();

	return !canceled;
}

static bool cutoutFace
(
	ExplicitHierarchicalMeshImpl& hMesh,
	physx::Array<physx::PxPlane>& faceTrimPlanes,
	ApexCSG::IApexBSP* coreBSP,
	uint32_t& coreChunkIndex,	// This may be changed if the original core chunk is sliced away completely
	const nvidia::FractureCutoutDesc& desc,
	const nvidia::NoiseParameters& backfaceNoise,
	const nvidia::NoiseParameters& edgeNoise,
	const nvidia::FractureMaterialDesc& materialDesc,
	const int32_t fractureIndex,
	const physx::PxPlane& facePlane,
	const nvidia::CutoutSet& iCutoutSetImpl,
	const PxMat44& cutoutTM,
	const float mapXLow,
	const float mapYLow,
	const physx::PxBounds3& localBounds,
	const float cutoutDepth,
	const nvidia::FractureSliceDesc& sliceDesc,
	const nvidia::FractureVoronoiDesc& voronoiDesc,
	const CollisionDesc& collisionDesc,
	nvidia::IProgressListener& progressListener,
	bool& stop,
	volatile bool* cancel
)
{
	nvidia::HierarchicalProgressListener localProgressListener(PxMax((int32_t)iCutoutSetImpl.getCutoutCount(), 1), &progressListener);

	const physx::PxVec3 localExtents = localBounds.getExtents();
	const physx::PxVec3 localCenter = localBounds.getCenter();

	const float sizeScale = PxMax(PxMax(localExtents.x, localExtents.y), localExtents.z);

	uint32_t corePartIndex = (uint32_t)hMesh.mChunks[coreChunkIndex]->mPartIndex;

	const uint32_t oldSize = hMesh.chunkCount();
	ApexCSG::IApexBSP* faceBSP = createBSP(hMesh.mBSPMemCache);	// face BSP defaults to all space
	uint32_t faceChunkIndex = 0xFFFFFFFF;
	if (cutoutDepth > 0.0f)	// (depth = 0) => slice all the way through
	{
		nvidia::IntersectMesh grid;

		const float mapWidth = cutoutTM.column0.magnitude()*iCutoutSetImpl.getDimensions()[0];
		const float mapHeight = cutoutTM.column1.magnitude()*iCutoutSetImpl.getDimensions()[1];

		// Create faceBSP from grid
		GridParameters gridParameters;
		gridParameters.interiorSubmeshIndex = materialDesc.interiorSubmeshIndex;
		gridParameters.noise = backfaceNoise;
		gridParameters.level0Mesh = &hMesh.mParts[0]->mMesh;	// must be set each time, since this can move with array resizing
		gridParameters.sizeScale = sizeScale;
		if (desc.instancingMode != FractureCutoutDesc::DoNotInstance)
		{
			gridParameters.xPeriod = mapWidth;
			gridParameters.yPeriod = mapHeight;
		}
		// Create the slicing plane
		physx::PxPlane slicePlane = facePlane;
		slicePlane.d += cutoutDepth;
		gridParameters.materialFrameIndex = hMesh.addMaterialFrame();
		nvidia::MaterialFrame materialFrame = hMesh.getMaterialFrame(gridParameters.materialFrameIndex);
		materialFrame.buildCoordinateSystemFromMaterialDesc(materialDesc, slicePlane);
		materialFrame.mFractureMethod = nvidia::FractureMethod::Cutout;
		materialFrame.mFractureIndex = fractureIndex;
		materialFrame.mSliceDepth = hMesh.depth(coreChunkIndex) + 1;
		hMesh.setMaterialFrame(gridParameters.materialFrameIndex, materialFrame);
		gridParameters.triangleFrame.setFlat(materialFrame.mCoordinateSystem, materialDesc.uvScale, materialDesc.uvOffset);
		buildIntersectMesh(grid, slicePlane, materialFrame, (int32_t)sliceDesc.noiseMode, &gridParameters);
		ApexCSG::BSPTolerances bspTolerances = ApexCSG::gDefaultTolerances;
		bspTolerances.linear = 0.00001f;
		bspTolerances.angular = 0.00001f;
		faceBSP->setTolerances(bspTolerances);
		ApexCSG::BSPBuildParameters bspBuildParams = gDefaultBuildParameters;
		bspBuildParams.rnd = &userRnd;
		bspBuildParams.internalTransform = coreBSP->getInternalTransform();
		faceBSP->fromMesh(&grid.m_triangles[0], grid.m_triangles.size(), bspBuildParams);
		coreBSP->combine(*faceBSP);
		faceBSP->op(*coreBSP, ApexCSG::Operation::A_Minus_B);
		coreBSP->op(*coreBSP, ApexCSG::Operation::Intersection);
		uint32_t facePartIndex = hMesh.addPart();
		faceChunkIndex = hMesh.addChunk();
		hMesh.mChunks[faceChunkIndex]->mPartIndex = (int32_t)facePartIndex;
		faceBSP->toMesh(hMesh.mParts[facePartIndex]->mMesh);
		CollisionVolumeDesc volumeDesc = getVolumeDesc(collisionDesc, hMesh.depth(coreChunkIndex) + 1);
		if (hMesh.mParts[facePartIndex]->mMesh.size() != 0)
		{
			hMesh.mParts[facePartIndex]->mMeshBSP->copy(*faceBSP);
			hMesh.buildMeshBounds(facePartIndex);
			hMesh.buildCollisionGeometryForPart(facePartIndex, volumeDesc);
			if (desc.trimFaceCollisionHulls && (gridParameters.noise.amplitude != 0.0f || volumeDesc.mHullMethod != nvidia::ConvexHullMethod::WRAP_GRAPHICS_MESH))
			{
				// Trim backface
				for (uint32_t hullIndex = 0; hullIndex < hMesh.mParts[facePartIndex]->mCollision.size(); ++hullIndex)
				{
					ConvexHullImpl& hull = hMesh.mParts[facePartIndex]->mCollision[hullIndex]->impl;
					hull.intersectPlaneSide(physx::PxPlane(-slicePlane.n, -slicePlane.d));
					faceTrimPlanes.pushBack(slicePlane);
				}
			}
			hMesh.mChunks[faceChunkIndex]->mParentIndex = 0;
		}
		else
		{
			hMesh.removePart(facePartIndex);
			hMesh.removeChunk(faceChunkIndex);
			faceChunkIndex = 0xFFFFFFFF;
			facePartIndex = 0xFFFFFFFF;
		}
	}
	else
	{
		// Slicing goes all the way through
		faceBSP->copy(*coreBSP);
		if (oldSize == coreChunkIndex + 1)
		{
			// Core hasn't been split yet.  We don't want a copy of the original mesh at level 1, so remove it.
			hMesh.removePart(corePartIndex--);
			hMesh.removeChunk(coreChunkIndex--);
		}
		faceChunkIndex = coreChunkIndex;
		// This will break us out of both loops (only want to slice all the way through once):
		stop = true;
	}

	localProgressListener.setSubtaskWork(1);

	bool canceled = false;

	if (faceChunkIndex < hMesh.chunkCount())
	{
		// We have a face chunk.  Create cutouts
		canceled = !createFaceCutouts(hMesh, faceChunkIndex, *faceBSP, desc, edgeNoise, cutoutDepth, materialDesc, *(const nvidia::CutoutSetImpl*)&iCutoutSetImpl, cutoutTM, mapXLow, mapYLow, collisionDesc,
									  sliceDesc, voronoiDesc, facePlane, localCenter, localExtents, localProgressListener, cancel);
		// If there is anything left in the face, attach it as unfracturable
		// Volume rejection ratio, perhaps should be exposed
#if 0	// BRG - to do : better treatment of face leftover
		const float volumeRejectionRatio = 0.0001f;
		if (faceBSP->getVolume() >= volumeRejectionRatio * faceVolumeEstimate)
		{
			const uint32_t newPartIndex = hMesh.addPart();
			faceBSP->toMesh(hMesh.mParts[newPartIndex]->mMesh);
			if (hMesh.mParts[newPartIndex]->mMesh.size() != 0)
			{
				hMesh.mParts[newPartIndex]->mMeshBSP->copy(*faceBSP);
				hMesh.buildMeshBounds(newPartIndex);
				hMesh.mParts[newPartIndex]->mCollision.setEmpty();	// BRG - to do : better treatment of face leftover
				hMesh.mParts[newPartIndex]->mParentIndex = faceChunkIndex;
				chunkFlags.resize(hMesh.partCount(), 0);
			}
			else
			{
				hMesh.removePart(newPartIndex);
			}
		}
#endif

		localProgressListener.completeSubtask();
	}
	faceBSP->release();

	return !canceled;
}

bool createChippedMesh
(
    ExplicitHierarchicalMesh& iHMesh,
    const nvidia::MeshProcessingParameters& meshProcessingParams,
    const nvidia::FractureCutoutDesc& desc,
    const nvidia::CutoutSet& iCutoutSetImpl,
    const nvidia::FractureSliceDesc& sliceDesc,
	const nvidia::FractureVoronoiDesc& voronoiDesc,
    const CollisionDesc& collisionDesc,
    uint32_t randomSeed,
    nvidia::IProgressListener& progressListener,
    volatile bool* cancel
)
{
	ExplicitHierarchicalMeshImpl& hMesh = *(ExplicitHierarchicalMeshImpl*)&iHMesh;

	if (hMesh.partCount() == 0)
	{
		return false;
	}

	outputMessage("Chipping...");
	progressListener.setProgress(0);

	// Save state if cancel != NULL
	physx::PxFileBuf* save = NULL;
	class NullEmbedding : public ExplicitHierarchicalMesh::Embedding
	{
		void	serialize(physx::PxFileBuf& stream, Embedding::DataType type) const
		{
			(void)stream;
			(void)type;
		}
		void	deserialize(physx::PxFileBuf& stream, Embedding::DataType type, uint32_t version)
		{
			(void)stream;
			(void)type;
			(void)version;
		}
	} embedding;
	if (cancel != NULL)
	{
		save = nvidia::GetApexSDK()->createMemoryWriteStream();
		if (save != NULL)
		{
			hMesh.serialize(*save, embedding);
		}
	}

	hMesh.buildCollisionGeometryForPart(0, getVolumeDesc(collisionDesc, 0));

	userRnd.m_rnd.setSeed(randomSeed);

	if (hMesh.mParts[0]->mMeshBSP->getType() != ApexCSG::BSPType::Nontrivial)
	{
		outputMessage("Building mesh BSP...");
		progressListener.setProgress(0);
		hMesh.calculateMeshBSP(randomSeed, &progressListener, &meshProcessingParams.microgridSize, meshProcessingParams.meshMode);
		outputMessage("Mesh BSP completed.");
		userRnd.m_rnd.setSeed(randomSeed);
	}

	gIslandGeneration = meshProcessingParams.islandGeneration;
	gMicrogridSize = meshProcessingParams.microgridSize;
	gVerbosity = meshProcessingParams.verbosity;

	if (hMesh.mParts[0]->mBounds.isEmpty())
	{
		return false;	// Done, nothing in mesh
	}

	hMesh.clear(true);

	for (int i = 0; i < FractureCutoutDesc::DirectionCount; ++i)
	{
		if ((desc.directions >> i) & 1)
		{
			hMesh.mSubmeshData.resize(PxMax(hMesh.mRootSubmeshCount, desc.cutoutParameters[i].materialDesc.interiorSubmeshIndex + 1));
		}
	}
	switch (desc.chunkFracturingMethod)
	{
	case FractureCutoutDesc::SliceFractureCutoutChunks:
		for (int i = 0; i < 3; ++i)
		{
			hMesh.mSubmeshData.resize(PxMax(hMesh.mRootSubmeshCount, sliceDesc.materialDesc[i].interiorSubmeshIndex + 1));
		}
		break;
	case FractureCutoutDesc::VoronoiFractureCutoutChunks:
		hMesh.mSubmeshData.resize(PxMax(hMesh.mRootSubmeshCount, voronoiDesc.materialDesc.interiorSubmeshIndex + 1));
		break;
	}

	// Count directions
	uint32_t directionCount = 0;
	uint32_t directions = desc.directions;
	while (directions)
	{
		directions = (directions - 1)&directions;
		++directionCount;
	}

	if (directionCount == 0 && desc.userDefinedDirection.isZero())	// directions = 0 is the way we invoke the user-supplied normal "UV-based" cutout fracturing
	{
		return true; // Done, no split directions
	}

	// Validate direction ordering
	bool dirUsed[FractureCutoutDesc::DirectionCount];
	memset(dirUsed, 0, sizeof(dirUsed) / sizeof(dirUsed[0]));
	for (uint32_t dirIndex = 0; dirIndex < FractureCutoutDesc::DirectionCount; ++dirIndex)
	{
		// The direction must be one found in FractureCutoutDesc::Directions
		//    and must not be used twice, if it is enabled
		if ((directions  & desc.directionOrder[dirIndex]) &&
			(!shdfnd::isPowerOfTwo(desc.directionOrder[dirIndex])                  ||
			 desc.directionOrder[dirIndex] <= 0                            ||
			 desc.directionOrder[dirIndex] > FractureCutoutDesc::PositiveZ ||
			 dirUsed[lowestSetBit(desc.directionOrder[dirIndex])]))
		{
			outputMessage("Invalid direction ordering, each direction may be used just once, "
				          "and must correspond to a direction defined in FractureCutoutDesc::Directions.",
				physx::PxErrorCode::eINTERNAL_ERROR);
			return false;
		}
		dirUsed[dirIndex] = true;
	}

	nvidia::HierarchicalProgressListener localProgressListener(PxMax((int32_t)directionCount, 1), &progressListener);

	// Core starts as original mesh
	uint32_t corePartIndex = hMesh.addPart();
	uint32_t coreChunkIndex = hMesh.addChunk();
	hMesh.mParts[corePartIndex]->mMesh = hMesh.mParts[0]->mMesh;
	hMesh.buildMeshBounds(0);
	hMesh.mChunks[coreChunkIndex]->mParentIndex = 0;
	hMesh.mChunks[coreChunkIndex]->mPartIndex = (int32_t)corePartIndex;

	ApexCSG::IApexBSP* coreBSP = createBSP(hMesh.mBSPMemCache);
	coreBSP->copy(*hMesh.mParts[0]->mMeshBSP);

	physx::Array<physx::PxPlane> faceTrimPlanes;

	const physx::PxBounds3& worldBounds = hMesh.mParts[0]->mBounds;
	const physx::PxVec3& extents = worldBounds.getExtents();
	const physx::PxVec3& center = worldBounds.getCenter();

	SliceParameters* sliceParametersAtDepth = (SliceParameters*)PxAlloca(sizeof(SliceParameters) * sliceDesc.maxDepth);

	bool canceled = false;
	bool stop = false;
	for (uint32_t dirNum = 0; dirNum < FractureCutoutDesc::DirectionCount && !stop && !canceled; ++dirNum)
	{
		const uint32_t sliceDirIndex = lowestSetBit(desc.directionOrder[dirNum]);
		uint32_t sliceAxisNum, sliceSignNum;
		getCutoutSliceAxisAndSign(sliceAxisNum, sliceSignNum, sliceDirIndex);
		{
			if ((desc.directions >> sliceDirIndex) & 1)
			{
				uint32_t sliceAxes[3];
				generateSliceAxes(sliceAxes, sliceAxisNum);

				localProgressListener.setSubtaskWork(1);

				physx::PxPlane facePlane;
				facePlane.n = physx::PxVec3(0, 0, 0);
				facePlane.n[sliceAxisNum] = sliceSignNum ? -1.0f : 1.0f;
				facePlane.d = -(facePlane.n[sliceAxisNum] * center[sliceAxisNum] + extents[sliceAxisNum]);	// coincides with depth = 0

				bool invertX;
				const PxMat44 cutoutTM = createCutoutFrame(center, extents, sliceAxes, sliceSignNum, desc, invertX);

				// Tiling bounds
				const float mapWidth = cutoutTM.column0.magnitude()*iCutoutSetImpl.getDimensions()[0];
				const float mapXLow = cutoutTM.getPosition()[sliceAxes[0]] - ((invertX != desc.cutoutWidthInvert[sliceDirIndex])? mapWidth : 0.0f);
				const float mapHeight = cutoutTM.column1.magnitude()*iCutoutSetImpl.getDimensions()[1];
				const float mapYLow = cutoutTM.getPosition()[sliceAxes[1]] - (desc.cutoutHeightInvert[sliceDirIndex] ? mapHeight : 0.0f);

				physx::PxBounds3 localBounds;
				for (unsigned i = 0; i < 3; ++i)
				{
					localBounds.minimum[i] = worldBounds.minimum[sliceAxes[i]];
					localBounds.maximum[i] = worldBounds.maximum[sliceAxes[i]];
				}

				// Slice desc, if needed
				nvidia::FractureSliceDesc cutoutSliceDesc;
				// Create a sliceDesc based off of the GUI slice desc's X and Y components, applied to the
				// two axes appropriate for this cutout direction.
				cutoutSliceDesc = sliceDesc;
				cutoutSliceDesc.sliceParameters = sliceParametersAtDepth;
				for (unsigned depth = 0; depth < sliceDesc.maxDepth; ++depth)
				{
					cutoutSliceDesc.sliceParameters[depth] = sliceDesc.sliceParameters[depth];
				}
				for (uint32_t axisN = 0; axisN < 3; ++axisN)
				{
					cutoutSliceDesc.targetProportions[sliceAxes[axisN]] = sliceDesc.targetProportions[axisN];
					for (uint32_t depth = 0; depth < sliceDesc.maxDepth; ++depth)
					{
						cutoutSliceDesc.sliceParameters[depth].splitsPerPass[sliceAxes[axisN]] = sliceDesc.sliceParameters[depth].splitsPerPass[axisN];
						cutoutSliceDesc.sliceParameters[depth].linearVariation[sliceAxes[axisN]] = sliceDesc.sliceParameters[depth].linearVariation[axisN];
						cutoutSliceDesc.sliceParameters[depth].angularVariation[sliceAxes[axisN]] = sliceDesc.sliceParameters[depth].angularVariation[axisN];
						cutoutSliceDesc.sliceParameters[depth].noise[sliceAxes[axisN]] = sliceDesc.sliceParameters[depth].noise[axisN];
					}
				}

				canceled = !cutoutFace(hMesh, faceTrimPlanes, coreBSP, coreChunkIndex, desc, desc.cutoutParameters[sliceDirIndex].backfaceNoise, desc.cutoutParameters[sliceDirIndex].edgeNoise,
									   desc.cutoutParameters[sliceDirIndex].materialDesc, (int32_t)sliceDirIndex, facePlane, iCutoutSetImpl, cutoutTM, mapXLow, mapYLow, localBounds,
									   desc.cutoutParameters[sliceDirIndex].depth, cutoutSliceDesc, voronoiDesc, collisionDesc, localProgressListener, stop, cancel);

				localProgressListener.completeSubtask();
			}
		}
	}

	if (desc.directions == 0) // user-supplied normal "UV-based" cutout fracturing
	{
		localProgressListener.setSubtaskWork(1);

		// Create cutout transform from user's supplied mapping and direction
		const physx::PxVec3 userNormal = desc.userDefinedDirection.getNormalized();

		PxMat44 cutoutTM;
		cutoutTM.column0 = PxVec4(desc.userUVMapping.column0/(float)desc.cutoutSizeX, 0.f);
		cutoutTM.column1 = PxVec4(desc.userUVMapping.column1/(float)desc.cutoutSizeY, 0.f);
		cutoutTM.column2 = PxVec4(userNormal, 0.f);
		cutoutTM.setPosition(desc.userUVMapping.column2);

		// Also create a local frame to get the local bounds for the mesh
		const physx::PxMat33 globalToLocal = physx::PxMat33(desc.userUVMapping.column0.getNormalized(), desc.userUVMapping.column1.getNormalized(), userNormal).getTranspose();

		physx::Array<nvidia::ExplicitRenderTriangle>& mesh = hMesh.mParts[0]->mMesh;
		physx::PxBounds3 localBounds;
		localBounds.setEmpty();
		for (uint32_t i = 0; i < mesh.size(); ++i)
		{
			nvidia::ExplicitRenderTriangle& tri = mesh[i];
			for (int v = 0; v < 3; ++v)
			{
				localBounds.include(globalToLocal*tri.vertices[v].position);
			}
		}

		physx::PxPlane facePlane;
		facePlane.n = userNormal;
		facePlane.d = -localBounds.maximum[2];	// coincides with depth = 0

		// Tiling bounds
		const physx::PxVec3 localOrigin = globalToLocal*cutoutTM.getPosition();
		const float mapXLow = localOrigin[0];
		const float mapYLow = localOrigin[1];

		canceled = !cutoutFace(hMesh, faceTrimPlanes, coreBSP, coreChunkIndex, desc, desc.userDefinedCutoutParameters.backfaceNoise, desc.userDefinedCutoutParameters.edgeNoise,
							   desc.userDefinedCutoutParameters.materialDesc, 6, facePlane, iCutoutSetImpl, cutoutTM, mapXLow, mapYLow, localBounds, desc.userDefinedCutoutParameters.depth,
							   sliceDesc, voronoiDesc, collisionDesc, localProgressListener, stop, cancel);

		localProgressListener.completeSubtask();
	}

	if (!canceled && coreChunkIndex != 0)
	{
		coreBSP->toMesh(hMesh.mParts[corePartIndex]->mMesh);
		if (hMesh.mParts[corePartIndex]->mMesh.size() != 0)
		{
			hMesh.mParts[corePartIndex]->mMeshBSP->copy(*coreBSP);
			hMesh.buildCollisionGeometryForPart(coreChunkIndex, getVolumeDesc(collisionDesc, hMesh.depth(coreChunkIndex)));
			for (uint32_t i = 0; i < faceTrimPlanes.size(); ++i)
			{
				for (uint32_t hullIndex = 0; hullIndex < hMesh.mParts[corePartIndex]->mCollision.size(); ++hullIndex)
				{
					ConvexHullImpl& hull = hMesh.mParts[corePartIndex]->mCollision[hullIndex]->impl;
					hull.intersectPlaneSide(faceTrimPlanes[i]);
				}
			}
		}
		else
		{
			// Remove core mesh and chunk
			if (corePartIndex < hMesh.mParts.size())
			{
				hMesh.removePart(corePartIndex);
			}
			if (coreChunkIndex < hMesh.mChunks.size())
			{
				hMesh.removeChunk(coreChunkIndex);
			}
			coreChunkIndex = 0xFFFFFFFF;
			corePartIndex = 0xFFFFFFFF;
		}
	}

	coreBSP->release();

	// Restore if canceled
	if (canceled && save != NULL)
	{
		uint32_t len;
		const void* mem = nvidia::GetApexSDK()->getMemoryWriteBuffer(*save, len);
		physx::PxFileBuf* load = nvidia::GetApexSDK()->createMemoryReadStream(mem, len);
		if (load != NULL)
		{
			hMesh.deserialize(*load, embedding);
			nvidia::GetApexSDK()->releaseMemoryReadStream(*load);
		}
	}

	if (save != NULL)
	{
		nvidia::GetApexSDK()->releaseMemoryReadStream(*save);
	}

	if (canceled)
	{
		return false;
	}

	if (meshProcessingParams.removeTJunctions)
	{
		MeshProcessor meshProcessor;
		for (uint32_t i = 0; i < hMesh.partCount(); ++i)
		{
			meshProcessor.setMesh(hMesh.mParts[i]->mMesh, NULL, 0, 0.0001f*extents.magnitude());
			meshProcessor.removeTJunctions();
		}
	}

	hMesh.sortChunks();

	hMesh.createPartSurfaceNormals();

	if (desc.instancingMode == FractureCutoutDesc::InstanceAllChunks)
	{
		for (uint32_t i = 0; i < hMesh.chunkCount(); ++i)
		{
			hMesh.mChunks[i]->mFlags |= nvidia::apex::DestructibleAsset::ChunkIsInstanced;
		}
	}

	outputMessage("chipping completed.");

	return true;
}

class VoronoiMeshSplitter : public MeshSplitter
{
private:
	VoronoiMeshSplitter& operator=(const VoronoiMeshSplitter&);

public:
	VoronoiMeshSplitter(const FractureVoronoiDesc& desc) : mDesc(desc)
	{
	}

	bool validate(ExplicitHierarchicalMeshImpl& hMesh)
	{
		if (hMesh.chunkCount() == 0)
		{
			return false;
		}

		if (mDesc.siteCount == 0)
		{
			return false;
		}

		return true;
	}

	void initialize(ExplicitHierarchicalMeshImpl& hMesh)
	{
		hMesh.mSubmeshData.resize(PxMax(hMesh.mRootSubmeshCount, mDesc.materialDesc.interiorSubmeshIndex + 1));

		// Need to split out DM parameters
//		if (mDesc.useDisplacementMaps)
//		{
//			hMesh.initializeDisplacementMapVolume(mDesc);
//		}
	}

	bool process
	(
		ExplicitHierarchicalMeshImpl& hMesh,
		uint32_t chunkIndex,
		const ApexCSG::IApexBSP& chunkBSP,
		const CollisionDesc& collisionDesc,
		nvidia::IProgressListener& progressListener,
		volatile bool* cancel
	)
	{
		return voronoiSplitChunkInternal(hMesh, chunkIndex, chunkBSP, mDesc, collisionDesc, progressListener, cancel);
	}

	bool finalize(ExplicitHierarchicalMeshImpl& hMesh)
	{
		if (mDesc.instanceChunks)
		{
			for (uint32_t i = 0; i < hMesh.partCount(); ++i)
			{
				hMesh.mChunks[i]->mFlags |= nvidia::apex::DestructibleAsset::ChunkIsInstanced;
			}
		}

		return true;
	}

protected:
	const FractureVoronoiDesc& mDesc;
};

bool createVoronoiSplitMesh
(
	ExplicitHierarchicalMesh& iHMesh,
	ExplicitHierarchicalMesh& iHMeshCore,
	bool exportCoreMesh,
	int32_t coreMeshImprintSubmeshIndex,	// If this is < 0, use the core mesh materials (was applyCoreMeshMaterialToNeighborChunks).  Otherwise, use the given submesh.
	const MeshProcessingParameters& meshProcessingParams,
	const FractureVoronoiDesc& desc,
	const CollisionDesc& collisionDesc,
	uint32_t randomSeed,
	nvidia::IProgressListener& progressListener,
	volatile bool* cancel
)
{
	VoronoiMeshSplitter splitter(desc);

	return splitMeshInternal(
		iHMesh,
		iHMeshCore,
		exportCoreMesh,
		coreMeshImprintSubmeshIndex,
		meshProcessingParams,
		splitter,
		collisionDesc,
		randomSeed,
		progressListener,
		cancel);
}

bool voronoiSplitChunk
(
	ExplicitHierarchicalMesh& iHMesh,
	uint32_t chunkIndex,
	const FractureTools::MeshProcessingParameters& meshProcessingParams,
	const FractureTools::FractureVoronoiDesc& desc,
	const CollisionDesc& collisionDesc,
	uint32_t* randomSeed,
	IProgressListener& progressListener,
	volatile bool* cancel
)
{
	VoronoiMeshSplitter splitter(desc);

	return splitChunkInternal(iHMesh, chunkIndex, meshProcessingParams, splitter, collisionDesc, randomSeed, progressListener, cancel);
}

uint32_t createVoronoiSitesInsideMesh
(
	ExplicitHierarchicalMesh& iHMesh,
	physx::PxVec3* siteBuffer,
	uint32_t* siteChunkIndices,
	uint32_t siteCount,
	uint32_t* randomSeed,
	uint32_t* microgridSize,
	BSPOpenMode::Enum meshMode,
	IProgressListener& progressListener,
	uint32_t chunkIndex
)
{
	ExplicitHierarchicalMeshImpl& hMesh = *(ExplicitHierarchicalMeshImpl*)&iHMesh;

	physx::Array<uint32_t> chunkList;

	if (hMesh.mChunks.size() == 0)
	{
		return 0;
	}

	if (chunkIndex >= hMesh.chunkCount())
	{
		// Find root-depth chunks
		for (uint32_t chunkIndex = 0; chunkIndex < hMesh.chunkCount(); ++chunkIndex)
		{
			if (hMesh.mChunks[chunkIndex]->isRootLeafChunk())
			{
				chunkList.pushBack(chunkIndex);
			}
		}

		if (chunkList.size() > 0)
		{
			return createVoronoiSitesInsideMeshInternal(hMesh, &chunkList[0], chunkList.size(), siteBuffer, siteChunkIndices, siteCount, randomSeed, microgridSize, meshMode, progressListener);
		}

		return 0;	// This means we didn't find a root leaf chunk
	}

	return createVoronoiSitesInsideMeshInternal(hMesh, &chunkIndex, 1, siteBuffer, siteChunkIndices, siteCount, randomSeed, microgridSize, meshMode, progressListener);
}

// Defining these structs here, so as not to offend gnu's sensibilities
struct TriangleData
{
	uint16_t							chunkIndex;
	physx::PxVec3							triangleNormal;
	float							summedAreaWeight;
	const nvidia::ExplicitRenderTriangle*	triangle;
};

struct InstanceInfo
{
	uint8_t		meshIndex;
	int32_t	chunkIndex;	// Using a int32_t so that the createIndexStartLookup can do its thing
	physx::PxMat44	relativeTransform;

	struct ChunkIndexLessThan
	{
		PX_INLINE bool operator()(const InstanceInfo& x, const InstanceInfo& y) const
		{
			return x.chunkIndex < y.chunkIndex;
		}
	};
};

uint32_t	createScatterMeshSites
(
	uint8_t*						meshIndices,
	physx::PxMat44*						relativeTransforms,
	uint32_t*						chunkMeshStarts,
	uint32_t						scatterMeshInstancesBufferSize,
	ExplicitHierarchicalMesh&	iHMesh,
	uint32_t						targetChunkCount,
	const uint16_t*					targetChunkIndices,
	uint32_t*						randomSeed,
	uint32_t						scatterMeshAssetCount,
	nvidia::RenderMeshAsset**			scatterMeshAssets,
	const uint32_t*					minCount,
	const uint32_t*					maxCount,
	const float*					minScales,
	const float*					maxScales,
	const float*					maxAngles
)
{
	ExplicitHierarchicalMeshImpl& hMesh = *(ExplicitHierarchicalMeshImpl*)&iHMesh;

	// Cap asset count to 1-byte range
	if (scatterMeshAssetCount > 255)
	{
		scatterMeshAssetCount = 255;
	}

	// Set random seed if requested
	if (randomSeed != NULL)
	{
		userRnd.m_rnd.setSeed(*randomSeed);
	}

	// Counts for each scatter mesh asset
	physx::Array<uint32_t> counts(scatterMeshAssetCount, 0);

	// Create convex hulls for each scatter mesh and add up valid weights
	physx::Array<physx::PxVec3> vertices;	// Reusing this array for convex hull building
	physx::Array<PartConvexHullProxy> hulls(scatterMeshAssetCount);
	uint32_t scatterMeshInstancesRequested = 0;
	for (uint32_t scatterMeshAssetIndex = 0; scatterMeshAssetIndex < scatterMeshAssetCount; ++scatterMeshAssetIndex)
	{
		hulls[scatterMeshAssetIndex].impl.setEmpty();
		const nvidia::RenderMeshAsset* rma = scatterMeshAssets[scatterMeshAssetIndex];
		if (rma != NULL)
		{
			vertices.resize(0);
			for (uint32_t submeshIndex = 0; submeshIndex < rma->getSubmeshCount(); ++submeshIndex)
			{
				const nvidia::RenderSubmesh& submesh = rma->getSubmesh(submeshIndex);
				const nvidia::VertexBuffer& vertexBuffer = submesh.getVertexBuffer();
				if (vertexBuffer.getVertexCount() > 0)
				{
					const nvidia::VertexFormat& vertexFormat = vertexBuffer.getFormat();
					const int32_t posBufferIndex = vertexFormat.getBufferIndexFromID(vertexFormat.getSemanticID(nvidia::RenderVertexSemantic::POSITION));
					const uint32_t oldVertexCount = vertices.size();
					vertices.resize(oldVertexCount + vertexBuffer.getVertexCount());
					if (!vertexBuffer.getBufferData(&vertices[oldVertexCount], nvidia::RenderDataFormat::FLOAT3, sizeof(physx::PxVec3), (uint32_t)posBufferIndex, 0, vertexBuffer.getVertexCount()))
					{
						vertices.resize(oldVertexCount);	// Operation failed, revert vertex array size
					}
				}
			}
			if (vertices.size() > 0)
			{
				physx::Array<physx::PxVec3> directions;
				ConvexHullImpl::createKDOPDirections(directions, nvidia::ConvexHullMethod::USE_6_DOP);
				hulls[scatterMeshAssetIndex].impl.buildKDOP(&vertices[0], vertices.size(), sizeof(vertices[0]), &directions[0], directions.size());
				if (!hulls[scatterMeshAssetIndex].impl.isEmpty())
				{
					counts[scatterMeshAssetIndex] = (uint32_t)userRnd.m_rnd.getScaled((float)minCount[scatterMeshAssetIndex], (float)maxCount[scatterMeshAssetIndex] + 1.0f);
					scatterMeshInstancesRequested += counts[scatterMeshAssetIndex];
				}
			}
		}
	}

	// Cap at buffer size
	if (scatterMeshInstancesRequested > scatterMeshInstancesBufferSize)
	{
		scatterMeshInstancesRequested = scatterMeshInstancesBufferSize;
	}

	// Return if no instances requested
	if (scatterMeshInstancesRequested == 0)
	{
		return 0;
	}

	// Count the interior triangles in all of the target chunks, and add up their areas
	// Build an area-weighted lookup table for the various triangles (also reference the chunks)
	physx::Array<TriangleData> triangleTable;
	float summedAreaWeight = 0.0f;
	for (uint32_t chunkNum = 0; chunkNum < targetChunkCount; ++chunkNum)
	{
		const uint16_t chunkIndex = targetChunkIndices[chunkNum];
		if (chunkIndex >= hMesh.chunkCount())
		{
			continue;
		}
		const uint32_t partIndex = (uint32_t)*hMesh.partIndex(chunkIndex);
		const nvidia::ExplicitRenderTriangle* triangles = hMesh.meshTriangles(partIndex);
		const uint32_t triangleCount = hMesh.meshTriangleCount(partIndex);
		for (uint32_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
		{
			const ExplicitRenderTriangle& triangle = triangles[triangleIndex];
			if (triangle.extraDataIndex != 0xFFFFFFFF)	// See if this is an interior triangle
			{

				TriangleData& triangleData = triangleTable.insert();
				triangleData.chunkIndex = chunkIndex;
				triangleData.triangleNormal = triangle.calculateNormal();
				summedAreaWeight += triangleData.triangleNormal.normalize();
				triangleData.summedAreaWeight = summedAreaWeight;
				triangleData.triangle = &triangle;
			}
		}
	}

	// Normalize summed area table
	if (summedAreaWeight <= 0.0f)
	{
		return 0;	// Non-normalizable
	}
	const float recipSummedAreaWeight = 1.0f/summedAreaWeight;
	for (uint32_t triangleNum = 0; triangleNum < triangleTable.size()-1; ++triangleNum)
	{
		triangleTable[triangleNum].summedAreaWeight *= recipSummedAreaWeight;
	}
	triangleTable[triangleTable.size()-1].summedAreaWeight = 1.0f;	// Just to be sure

	// Reserve instance info
	physx::Array<InstanceInfo> instanceInfo;
	instanceInfo.reserve(scatterMeshInstancesRequested);

	// Add scatter meshes
	ApexCSG::IApexBSP* hullBSP = createBSP(hMesh.mBSPMemCache);
	if (hullBSP == NULL)
	{
		return 0;
	}

	physx::Array<physx::PxPlane> planes;	// Reusing this array for bsp building

	for (uint32_t scatterMeshAssetIndex = 0; scatterMeshAssetIndex < scatterMeshAssetCount && instanceInfo.size() < scatterMeshInstancesRequested; ++scatterMeshAssetIndex)
	{
		bool success = true;
		for (uint32_t count = 0; success && count < counts[scatterMeshAssetIndex] && instanceInfo.size() < scatterMeshInstancesRequested; ++count)
		{
			success = false;
			for (uint32_t trial = 0; !success && trial < 1000; ++trial)
			{
				// Pick triangle
				const TriangleData* triangleData = NULL;
				const float unitRndForTriangle = userRnd.m_rnd.getUnit();
				for (uint32_t triangleNum = 0; triangleNum < triangleTable.size(); ++triangleNum)
				{
					if (triangleTable[triangleNum].summedAreaWeight > unitRndForTriangle)
					{
						triangleData = &triangleTable[triangleNum];
						break;
					}
				}
				if (triangleData == NULL)
				{
					continue;
				}

				// pick scale, angle, and position and build transform
				const float scale = physx::PxExp(userRnd.m_rnd.getScaled(physx::PxLog(minScales[scatterMeshAssetIndex]), physx::PxLog(maxScales[scatterMeshAssetIndex])));
				const float angle = (physx::PxPi/180.0f)*userRnd.m_rnd.getScaled(0.0f, maxAngles[scatterMeshAssetIndex]);
				// random position in triangle
				const Vertex* vertices = triangleData->triangle->vertices;
				const physx::PxVec3 position = randomPositionInTriangle(vertices[0].position, vertices[1].position, vertices[2].position, userRnd.m_rnd);
				physx::PxVec3 zAxis = triangleData->triangleNormal;
				// Rotate z axis into arbitrary vector in triangle plane
				physx::PxVec3 para = vertices[1].position - vertices[0].position;
				if (para.normalize() > 0.0f)
				{
					float cosPhi, sinPhi;
					physx::shdfnd::sincos(angle, sinPhi, cosPhi);
					zAxis = cosPhi*zAxis + sinPhi*para;
				}
				physx::PxMat44 tm = randomRotationMatrix(zAxis, userRnd.m_rnd);
				tm.setPosition(position);
				tm.scale(physx::PxVec4(physx::PxVec3(scale), 1.0f));

				const int32_t parentIndex = *hMesh.parentIndex(triangleData->chunkIndex);
				if (parentIndex >= 0)
				{
					const uint32_t parentPartIndex = (uint32_t)*hMesh.partIndex((uint32_t)parentIndex);
					ApexCSG::IApexBSP* parentPartBSP = hMesh.mParts[parentPartIndex]->mMeshBSP;
					if (parentPartBSP != NULL)
					{
						// Create BSP from hull and transform
						PartConvexHullProxy& hull = hulls[scatterMeshAssetIndex];
						planes.resize(hull.impl.getPlaneCount());
						for (uint32_t planeIndex = 0; planeIndex < hull.impl.getPlaneCount(); ++planeIndex)
						{
							planes[planeIndex] = hull.impl.getPlane(planeIndex);
						}
						hullBSP->fromConvexPolyhedron(&planes[0], planes.size(), parentPartBSP->getInternalTransform());
						hullBSP->copy(*hullBSP, tm);

						// Now combine with chunk parent bsp, and see if the mesh hull bsp lies within the parent bsp
						hullBSP->combine(*hMesh.mParts[parentPartIndex]->mMeshBSP);
						hullBSP->op(*hullBSP, ApexCSG::Operation::A_Minus_B);
						if (hullBSP->getType() == ApexCSG::BSPType::Empty_Set)	// True if the hull lies entirely within the parent chunk
						{
							success = true;
							InstanceInfo& info = instanceInfo.insert();
							info.meshIndex = (uint8_t)scatterMeshAssetIndex;
							info.chunkIndex = (int32_t)triangleData->chunkIndex;
							info.relativeTransform = tm;
						}
					}
				}
			}
		}
	}

	hullBSP->release();

	// Now sort the instance info by chunk index
	if (instanceInfo.size() > 1)
	{
		nvidia::sort<InstanceInfo, InstanceInfo::ChunkIndexLessThan>(instanceInfo.begin(), instanceInfo.size(), InstanceInfo::ChunkIndexLessThan());
	}

	// Write the info to the output arrays
	for (uint32_t instanceNum = 0; instanceNum < instanceInfo.size() && instanceNum < scatterMeshInstancesBufferSize; ++instanceNum)	// Second condition instanceNum < scatterMeshInstancesBufferSize should not be necessary
	{
		const InstanceInfo& info = instanceInfo[instanceNum];
		meshIndices[instanceNum] = info.meshIndex;
		relativeTransforms[instanceNum] = info.relativeTransform;
	}

	// Finally create an indexed lookup
	if (instanceInfo.size() > 0)
	{
		physx::Array<uint32_t> lookup;
		createIndexStartLookup(lookup, 0, hMesh.chunkCount(), &instanceInfo[0].chunkIndex, instanceInfo.size(), sizeof(InstanceInfo));

		// .. and copy it into the output lookup table
		for (uint32_t chunkLookup = 0; chunkLookup <= hMesh.chunkCount(); ++chunkLookup)	// <= is intentional
		{
			chunkMeshStarts[chunkLookup] = lookup[chunkLookup];
		}
	}

	return instanceInfo.size();
}

PX_INLINE bool intersectPlanes(physx::PxVec3& pos, physx::PxVec3& dir, const physx::PxPlane& plane0, const physx::PxPlane& plane1)
{
	dir = plane0.n.cross(plane1.n);

	if(dir.normalize() < PX_EPS_F32)
	{
		return false;
	}

	pos = physx::PxVec3(0.0f);

	for (int iter = 3; iter--;)
	{
		// Project onto plane0:
		pos = plane0.project(pos);

		// Raycast to plane1:
		const physx::PxVec3 b = dir.cross(plane0.n);
		pos -= (plane1.distance(pos)/(b.dot(plane1.n)))*b;
	}

	return true;
}

PX_INLINE void renderConvex(nvidia::RenderDebugInterface& debugRender, const physx::PxPlane* planes, uint32_t planeCount, uint32_t color, float tolerance)
{
	RENDER_DEBUG_IFACE(&debugRender)->setCurrentColor(color);

	physx::Array<physx::PxVec3> endpoints;

	const float tol2 = tolerance*tolerance;

	for (uint32_t i = 0; i < planeCount; ++i)
	{
		// We'll be drawing polygons in this plane
		const physx::PxPlane& plane_i = planes[i];
		endpoints.resize(0);
		for (uint32_t j = 0; j < planeCount; ++j)
		{
			if (j == i)
			{
				continue;
			}
			const physx::PxPlane& plane_j = planes[j];
			// Find potential edge from intersection if plane_i and plane_j
			physx::PxVec3 orig;
			physx::PxVec3 edgeDir;
			if (!intersectPlanes(orig, edgeDir, plane_i, plane_j))
			{
				continue;
			}
			float minS = -PX_MAX_F32;
			float maxS = PX_MAX_F32;
			bool intersectionFound = true;
			// Clip to planes
			for (uint32_t k = 0; k < planeCount; ++k)
			{
				if (k == i || i == j)
				{
					continue;
				}
				const physx::PxPlane& plane_k = planes[k];
				const float num = -plane_k.distance(orig);
				const float den = edgeDir.dot(plane_k.n);
				if (physx::PxAbs(den) > 10*PX_EPS_F32)
				{
					const float s = num/den;
					if (den > 0.0f)
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
				if (num < -tolerance)
				{
					intersectionFound = false;
					break;
				}
			}
			if (intersectionFound)
			{
				endpoints.pushBack(orig + minS * edgeDir);
				endpoints.pushBack(orig + maxS * edgeDir);
			}
		}
		if (endpoints.size() > 2)
		{
			physx::Array<physx::PxVec3> verts;
			verts.pushBack(endpoints[endpoints.size()-2]);
			verts.pushBack(endpoints[endpoints.size()-1]);
			endpoints.popBack();
			endpoints.popBack();
			while (endpoints.size())
			{
				uint32_t closestN = 0;
				float closestDist2 = PX_MAX_F32;
				for (uint32_t n = 0; n < endpoints.size(); ++n)
				{
					const float dist2 = (endpoints[n] - verts[verts.size()-1]).magnitudeSquared();
					if (dist2 < closestDist2)
					{
						closestDist2 = dist2;
						closestN = n;
					}
				}
				if ((endpoints[closestN^1] - verts[0]).magnitudeSquared() < tol2)
				{
					break;
				}
				verts.pushBack(endpoints[closestN^1]);
				endpoints.replaceWithLast(closestN^1);
				endpoints.replaceWithLast(closestN);
			}
			if (verts.size() > 2)
			{
				if (((verts[1]-verts[0]).cross(verts[2]-verts[0])).dot(plane_i.n) < 0.0f)
				{
					for (uint32_t n = verts.size()/2; n--;)
					{
						nvidia::swap(verts[n], verts[verts.size()-1-n]);
					}
				}
				RENDER_DEBUG_IFACE(&debugRender)->debugPolygon(verts.size(), &verts[0]);
			}
		}
	}
}

void visualizeVoronoiCells
(
	nvidia::RenderDebugInterface& debugRender,
	const physx::PxVec3* sites,
	uint32_t siteCount,
	const uint32_t* cellColors,
	uint32_t cellColorCount,
	const physx::PxBounds3& bounds,
	uint32_t cellIndex /* = 0xFFFFFFFF */
)
{
	// Rendering tolerance
	const float tolerance = 1.0e-5f*bounds.getDimensions().magnitude();

	// Whether or not to use cellColors
	const bool useCellColors = cellColors != NULL && cellColorCount > 0;

	// Whether to draw a single cell or all cells
	const bool drawSingleCell = cellIndex < siteCount;

	// Create bound planes
	physx::Array<physx::PxPlane> boundPlanes;
	boundPlanes.reserve(6);
	boundPlanes.pushBack(physx::PxPlane(-1.0f, 0.0f, 0.0f, bounds.minimum.x));
	boundPlanes.pushBack(physx::PxPlane(1.0f, 0.0f, 0.0f, -bounds.maximum.x));
	boundPlanes.pushBack(physx::PxPlane(0.0f, -1.0f, 0.0f, bounds.minimum.y));
	boundPlanes.pushBack(physx::PxPlane(0.0f, 1.0f, 0.0f, -bounds.maximum.y));
	boundPlanes.pushBack(physx::PxPlane(0.0f, 0.0f, -1.0f, bounds.minimum.z));
	boundPlanes.pushBack(physx::PxPlane(0.0f, 0.0f, 1.0f, -bounds.maximum.z));

	// Iterate over cells
	for (VoronoiCellPlaneIterator i(sites, siteCount, boundPlanes.begin(), boundPlanes.size(), drawSingleCell ? cellIndex : 0); i.valid(); i.inc())
	{
		const uint32_t cellColor = useCellColors ? cellColors[i.cellIndex()%cellColorCount] : 0xFFFFFFFF;
		renderConvex(debugRender, i.cellPlanes(), i.cellPlaneCount(), cellColor, tolerance);
		if (drawSingleCell)
		{
			break;
		}
	}
}

bool buildSliceMesh
(
	nvidia::IntersectMesh& intersectMesh,
	ExplicitHierarchicalMesh& referenceMesh,
	const physx::PxPlane& slicePlane,
	const FractureTools::NoiseParameters& noiseParameters,
	uint32_t randomSeed
)
{
	if( referenceMesh.chunkCount() == 0 )
	{
		return false;
	}

	ExplicitHierarchicalMeshImpl& hMesh = *(ExplicitHierarchicalMeshImpl*)&referenceMesh;

	GridParameters gridParameters;
	gridParameters.interiorSubmeshIndex = 0;
	gridParameters.noise = noiseParameters;
	const uint32_t partIndex = (uint32_t)hMesh.mChunks[0]->mPartIndex;
	gridParameters.level0Mesh = &hMesh.mParts[partIndex]->mMesh;
	physx::PxVec3 extents = hMesh.mParts[partIndex]->mBounds.getExtents();
	gridParameters.sizeScale = physx::PxAbs(extents.x*slicePlane.n.x) + physx::PxAbs(extents.y*slicePlane.n.y) + physx::PxAbs(extents.z*slicePlane.n.z);
	gridParameters.materialFrameIndex = hMesh.addMaterialFrame();
	nvidia::MaterialFrame materialFrame = hMesh.getMaterialFrame(gridParameters.materialFrameIndex );
	nvidia::FractureMaterialDesc materialDesc;
	materialFrame.buildCoordinateSystemFromMaterialDesc(materialDesc, slicePlane);
	materialFrame.mFractureMethod = nvidia::FractureMethod::Unknown;	// This is only a slice preview
	hMesh.setMaterialFrame(gridParameters.materialFrameIndex, materialFrame);
	gridParameters.triangleFrame.setFlat(materialFrame.mCoordinateSystem, physx::PxVec2(1.0f), physx::PxVec2(0.0f));
	gridParameters.forceGrid = true;
	userRnd.m_rnd.setSeed(randomSeed);
	buildIntersectMesh(intersectMesh, slicePlane, materialFrame, 0, &gridParameters);

	return true;
}

} // namespace FractureTools

namespace nvidia
{
namespace apex
{

void buildCollisionGeometry(physx::Array<PartConvexHullProxy*>& volumes, const CollisionVolumeDesc& desc,
	const physx::PxVec3* vertices, uint32_t vertexCount, uint32_t vertexByteStride,
	const uint32_t* indices, uint32_t indexCount)
{
	ConvexHullMethod::Enum hullMethod = desc.mHullMethod;

	do 
	{
		if (hullMethod == nvidia::ConvexHullMethod::CONVEX_DECOMPOSITION)
		{
			resizeCollision(volumes, 0);

			CONVEX_DECOMPOSITION::ConvexDecomposition* decomposer = CONVEX_DECOMPOSITION::createConvexDecomposition();
			if (decomposer != NULL)
			{
				CONVEX_DECOMPOSITION::DecompDesc decompDesc;
				decompDesc.mCpercent = desc.mConcavityPercent;
	//TODO:JWR			decompDesc.mPpercent = desc.mMergeThreshold;
				decompDesc.mDepth = desc.mRecursionDepth;

				decompDesc.mVcount = vertexCount;
				decompDesc.mVertices = (float*)vertices;
				decompDesc.mTcount = indexCount / 3;
				decompDesc.mIndices = indices;

				uint32_t hullCount = decomposer->performConvexDecomposition(decompDesc);
				resizeCollision(volumes, hullCount);
				for (uint32_t hullIndex = 0; hullIndex < hullCount; ++hullIndex)
				{
					CONVEX_DECOMPOSITION::ConvexResult* result = decomposer->getConvexResult(hullIndex,false);
					volumes[hullIndex]->buildFromPoints(result->mHullVertices, result->mHullVcount, 3 * sizeof(float));
					if (volumes[hullIndex]->impl.isEmpty())
					{
						// fallback
						physx::Array<physx::PxVec3> directions;
						ConvexHullImpl::createKDOPDirections(directions, nvidia::ConvexHullMethod::USE_26_DOP);
						volumes[hullIndex]->impl.buildKDOP(result->mHullVertices, result->mHullVcount, 3 * sizeof(float), directions.begin(), directions.size());
					}
				}
				decomposer->release();
			}

			if(volumes.size() > 0)
			{
				break;
			}

			// fallback
			hullMethod = nvidia::ConvexHullMethod::WRAP_GRAPHICS_MESH;
		}

		resizeCollision(volumes, 1);

		if (hullMethod == nvidia::ConvexHullMethod::WRAP_GRAPHICS_MESH)
		{
			volumes[0]->buildFromPoints(vertices, vertexCount, vertexByteStride);
			if (!volumes[0]->impl.isEmpty())
			{
				break;
			}

			// fallback
			hullMethod = nvidia::ConvexHullMethod::USE_26_DOP;
		}

		physx::Array<physx::PxVec3> directions;
		ConvexHullImpl::createKDOPDirections(directions, hullMethod);
		volumes[0]->impl.buildKDOP(vertices, vertexCount, vertexByteStride, directions.begin(), directions.size());
	} while(0);

	// Reduce hulls
	for (uint32_t hullIndex = 0; hullIndex < volumes.size(); ++hullIndex)
	{
		// First try uninflated, then try with inflation.  This may find a better reduction
		volumes[hullIndex]->reduceHull(desc.mMaxVertexCount, desc.mMaxEdgeCount, desc.mMaxFaceCount, false);
		volumes[hullIndex]->reduceHull(desc.mMaxVertexCount, desc.mMaxEdgeCount, desc.mMaxFaceCount, true);
	}
}


// Serialization of ExplicitSubmeshData


void serialize(physx::PxFileBuf& stream, const ExplicitSubmeshData& d)
{
	ApexSimpleString materialName(d.mMaterialName);
	apex::serialize(stream, materialName);
	stream << d.mVertexFormat.mWinding;
	stream << d.mVertexFormat.mHasStaticPositions;
	stream << d.mVertexFormat.mHasStaticNormals;
	stream << d.mVertexFormat.mHasStaticTangents;
	stream << d.mVertexFormat.mHasStaticBinormals;
	stream << d.mVertexFormat.mHasStaticColors;
	stream << d.mVertexFormat.mHasStaticSeparateBoneBuffer;
	stream << d.mVertexFormat.mHasStaticDisplacements;
	stream << d.mVertexFormat.mHasDynamicPositions;
	stream << d.mVertexFormat.mHasDynamicNormals;
	stream << d.mVertexFormat.mHasDynamicTangents;
	stream << d.mVertexFormat.mHasDynamicBinormals;
	stream << d.mVertexFormat.mHasDynamicColors;
	stream << d.mVertexFormat.mHasDynamicSeparateBoneBuffer;
	stream << d.mVertexFormat.mHasDynamicDisplacements;
	stream << d.mVertexFormat.mUVCount;
	stream << d.mVertexFormat.mBonesPerVertex;
}

void deserialize(physx::PxFileBuf& stream, uint32_t apexVersion, uint32_t meshVersion, ExplicitSubmeshData& d)
{
	ApexSimpleString materialName;
	apex::deserialize(stream, apexVersion, materialName);
	nvidia::strlcpy(d.mMaterialName, ExplicitSubmeshData::MaterialNameBufferSize, materialName.c_str());

	if (apexVersion >= ApexStreamVersion::CleanupOfApexRenderMesh)
	{
		stream >> d.mVertexFormat.mWinding;
		stream >> d.mVertexFormat.mHasStaticPositions;
		stream >> d.mVertexFormat.mHasStaticNormals;
		stream >> d.mVertexFormat.mHasStaticTangents;
		stream >> d.mVertexFormat.mHasStaticBinormals;
		stream >> d.mVertexFormat.mHasStaticColors;
		stream >> d.mVertexFormat.mHasStaticSeparateBoneBuffer;
		if (meshVersion >= ExplicitHierarchicalMeshImpl::DisplacementData)
			stream >> d.mVertexFormat.mHasStaticDisplacements;
		stream >> d.mVertexFormat.mHasDynamicPositions;
		stream >> d.mVertexFormat.mHasDynamicNormals;
		stream >> d.mVertexFormat.mHasDynamicTangents;
		stream >> d.mVertexFormat.mHasDynamicBinormals;
		stream >> d.mVertexFormat.mHasDynamicColors;
		stream >> d.mVertexFormat.mHasDynamicSeparateBoneBuffer;
		if (meshVersion >= ExplicitHierarchicalMeshImpl::DisplacementData)
			stream >> d.mVertexFormat.mHasDynamicDisplacements;
		stream >> d.mVertexFormat.mUVCount;
		if (apexVersion < ApexStreamVersion::RemovedTextureTypeInformationFromVertexFormat)
		{
			// Dead data
			uint32_t textureTypes[VertexFormat::MAX_UV_COUNT];
			for (uint32_t i = 0; i < VertexFormat::MAX_UV_COUNT; ++i)
			{
				stream >> textureTypes[i];
			}
		}
		stream >> d.mVertexFormat.mBonesPerVertex;
	}
	else
	{
#if 0	// BRG - to do, implement conversion
		bool	hasPosition;
		bool	hasNormal;
		bool	hasTangent;
		bool	hasBinormal;
		bool	hasColor;
		uint32_t	numBonesPerVertex;
		uint32_t	uvCount;
		RenderCullMode::Enum winding = RenderCullMode::CLOCKWISE;

		// PH: assuming position and normal as the default dynamic flags
		uint32_t dynamicFlags = VertexFormatFlag::POSITION | VertexFormatFlag::NORMAL;

		if (version >= ApexStreamVersion::AddedRenderCullModeToRenderMeshAsset)
		{
			//stream.readBuffer( &winding, sizeof(winding) );
			stream >> winding;
		}
		if (version >= ApexStreamVersion::AddedDynamicVertexBufferField)
		{
			stream >> dynamicFlags;
		}
		if (version >= ApexStreamVersion::AddingTextureTypeInformationToVertexFormat)
		{
			stream >> hasPosition;
			stream >> hasNormal;
			stream >> hasTangent;
			stream >> hasBinormal;
			stream >> hasColor;
			if (version >= ApexStreamVersion::RenderMeshAssetRedesign)
			{
				stream >> numBonesPerVertex;
			}
			else
			{
				bool hasBoneIndex;
				stream >> hasBoneIndex;
				numBonesPerVertex = hasBoneIndex ? 1 : 0;
			}
			stream >> uvCount;
			if (version < ApexStreamVersion::RemovedTextureTypeInformationFromVertexFormat)
			{
				// Dead data
				uint32_t textureTypes[VertexFormat::MAX_UV_COUNT];
				for (uint32_t i = 0; i < VertexFormat::MAX_UV_COUNT; ++i)
				{
					stream >> textureTypes[i];
				}
			}
		}
		else
		{
			uint32_t data;
			stream >> data;
			hasPosition = (data & (1 << 8)) != 0;
			hasNormal = (data & (1 << 9)) != 0;
			hasTangent = (data & (1 << 10)) != 0;
			hasBinormal = (data & (1 << 11)) != 0;
			hasColor = (data & (1 << 12)) != 0;
			numBonesPerVertex = (data & (1 << 13)) != 0 ? 1 : 0;
			uvCount = data & 0xFF;
		}

		d.mVertexFormat.mWinding = winding;
		d.mVertexFormat.mHasStaticPositions = hasPosition;
		d.mVertexFormat.mHasStaticNormals = hasNormal;
		d.mVertexFormat.mHasStaticTangents = hasTangent;
		d.mVertexFormat.mHasStaticBinormals = hasBinormal;
		d.mVertexFormat.mHasStaticColors = hasColor;
		d.mVertexFormat.mHasStaticSeparateBoneBuffer = false;
		d.mVertexFormat.mHasDynamicPositions = (dynamicFlags & VertexFormatFlag::POSITION) != 0;
		d.mVertexFormat.mHasDynamicNormals = (dynamicFlags & VertexFormatFlag::NORMAL) != 0;
		d.mVertexFormat.mHasDynamicTangents = (dynamicFlags & VertexFormatFlag::TANGENT) != 0;
		d.mVertexFormat.mHasDynamicBinormals = (dynamicFlags & VertexFormatFlag::BINORMAL) != 0;
		d.mVertexFormat.mHasDynamicColors = (dynamicFlags & VertexFormatFlag::COLOR) != 0;
		d.mVertexFormat.mHasDynamicSeparateBoneBuffer = (dynamicFlags & VertexFormatFlag::SEPARATE_BONE_BUFFER) != 0;
		d.mVertexFormat.mUVCount = uvCount;
		d.mVertexFormat.mBonesPerVertex = numBonesPerVertex;

		if (version >= ApexStreamVersion::RenderMeshAssetRedesign)
		{
			uint32_t customBufferCount;
			stream >> customBufferCount;
			for (uint32_t i = 0; i < customBufferCount; i++)
			{
				uint32_t stringLength;
				stream >> stringLength;
				PX_ASSERT(stringLength < 254);
				char buf[256];
				stream.read(buf, stringLength);
				buf[stringLength] = 0;
				uint32_t format;
				stream >> format;
			}
		}
#endif
	}
}


// Serialization of nvidia::MaterialFrame


void serialize(physx::PxFileBuf& stream, const nvidia::MaterialFrame& f)
{
	// f.mCoordinateSystem
	const PxMat44& m44 = f.mCoordinateSystem;
	stream << m44(0, 0) << m44(0, 1) << m44(0, 2)
		<< m44(1, 0) << m44(1, 1) << m44(1, 2)
		<< m44(2, 0) << m44(2, 1) << m44(2, 2) << m44.getPosition();

	// Other fields of f
	stream << f.mUVPlane << f.mUVScale << f.mUVOffset << f.mFractureMethod << f.mFractureIndex << f.mSliceDepth;
}

void deserialize(physx::PxFileBuf& stream, uint32_t apexVersion, uint32_t meshVersion, nvidia::MaterialFrame& f)
{
	PX_UNUSED(apexVersion);

	f.mSliceDepth = 0;

	if (meshVersion >= ExplicitHierarchicalMeshImpl::ChangedMaterialFrameToIncludeFracturingMethodContext)	// First version in which this struct exists
	{
		// f.mCoordinateSystem
		PxMat44 &m44 = f.mCoordinateSystem;
		stream >> m44(0, 0) >> m44(0, 1) >> m44(0, 2)
			>> m44(1, 0) >> m44(1, 1) >> m44(1, 2)
			>> m44(2, 0) >> m44(2, 1) >> m44(2, 2) >> *reinterpret_cast<PxVec3*>(&m44.column3);

		// Other fields of f
		stream >> f.mUVPlane >> f.mUVScale >> f.mUVOffset >> f.mFractureMethod >> f.mFractureIndex;

		if (meshVersion >= ExplicitHierarchicalMeshImpl::AddedSliceDepthToMaterialFrame)
		{
			stream >> f.mSliceDepth;
		}
	}
}


// ExplicitHierarchicalMeshImpl

ExplicitHierarchicalMeshImpl::ExplicitHierarchicalMeshImpl()
{
	mBSPMemCache = ApexCSG::createBSPMemCache();
	mRootSubmeshCount = 0;
}

ExplicitHierarchicalMeshImpl::~ExplicitHierarchicalMeshImpl()
{
	clear();
	mBSPMemCache->release();
}

uint32_t ExplicitHierarchicalMeshImpl::addPart()
{
	const uint32_t index = mParts.size();
	mParts.insert();
	Part* part = PX_NEW(Part);
	part->mMeshBSP = createBSP(mBSPMemCache);
	mParts.back() = part;
	return index;
}

bool ExplicitHierarchicalMeshImpl::removePart(uint32_t index)
{
	if (index >= partCount())
	{
		return false;
	}

	for (uint32_t i = 0; i < mChunks.size(); ++i)
	{
		if (mChunks[i]->mPartIndex == (int32_t)index)
		{
			mChunks[i]->mPartIndex = -1;
		}
		else if (mChunks[i]->mPartIndex > (int32_t)index)
		{
			--mChunks[i]->mPartIndex;
		}
	}

	delete mParts[index];
	mParts.remove(index);

	return true;
}

uint32_t ExplicitHierarchicalMeshImpl::addChunk()
{
	const uint32_t index = mChunks.size();
	mChunks.insert();
	mChunks.back() = PX_NEW(Chunk);
	return index;
}

bool ExplicitHierarchicalMeshImpl::removeChunk(uint32_t index)
{
	if (index >= chunkCount())
	{
		return false;
	}

	for (uint32_t i = 0; i < mChunks.size(); ++i)
	{
		if (mChunks[i]->mParentIndex == (int32_t)index)
		{
			mChunks[i]->mParentIndex = -1;
		}
		else if (mChunks[i]->mParentIndex > (int32_t)index)
		{
			--mChunks[i]->mParentIndex;
		}
	}

	delete mChunks[index];
	mChunks.remove(index);

	return true;
}

void ExplicitHierarchicalMeshImpl::serialize(physx::PxFileBuf& stream, Embedding& embedding) const
{
	stream << (uint32_t)ExplicitHierarchicalMeshImpl::Current;
	stream << (uint32_t)ApexStreamVersion::Current;
	stream << mParts.size();
	for (uint32_t i = 0; i < mParts.size(); ++i)
	{
		stream << mParts[i]->mBounds;
		apex::serialize(stream, mParts[i]->mMesh);
		stream.storeDword(mParts[i]->mCollision.size());
		for (uint32_t j = 0; j < mParts[i]->mCollision.size(); ++j)
		{
			apex::serialize(stream, mParts[i]->mCollision[j]->impl);
		}
		if (mParts[i]->mMeshBSP != NULL)
		{
			stream << (uint32_t)1;
			mParts[i]->mMeshBSP->serialize(stream);
		}
		else
		{
			stream << (uint32_t)0;
		}
		stream << mParts[i]->mFlags;
	}
	stream << mChunks.size();
	for (uint32_t i = 0; i < mChunks.size(); ++i)
	{
		stream << mChunks[i]->mParentIndex;
		stream << mChunks[i]->mFlags;
		stream << mChunks[i]->mPartIndex;
		stream << mChunks[i]->mInstancedPositionOffset;
		stream << mChunks[i]->mInstancedUVOffset;
		stream << mChunks[i]->mPrivateFlags;
	}
	apex::serialize(stream, mSubmeshData);
	apex::serialize(stream, mMaterialFrames);
	embedding.serialize(stream, Embedding::MaterialLibrary);
	stream << mRootSubmeshCount;
}

void ExplicitHierarchicalMeshImpl::deserialize(physx::PxFileBuf& stream, Embedding& embedding)
{
	clear();

	uint32_t meshStreamVersion;
	stream >> meshStreamVersion;
	uint32_t apexStreamVersion;
	stream >> apexStreamVersion;

	if (meshStreamVersion < ExplicitHierarchicalMeshImpl::RemovedExplicitHMesh_mMaxDepth)
	{
		int32_t maxDepth;
		stream >> maxDepth;
	}

	if (meshStreamVersion >= ExplicitHierarchicalMeshImpl::InstancingData)
	{
		uint32_t partCount;
		stream >> partCount;
		mParts.resize(partCount);
		for (uint32_t i = 0; i < partCount; ++i)
		{
			mParts[i] = PX_NEW(Part);
			stream >> mParts[i]->mBounds;
			apex::deserialize(stream, apexStreamVersion, mParts[i]->mMesh);
			resizeCollision(mParts[i]->mCollision, stream.readDword());
			for (uint32_t hullNum = 0; hullNum < mParts[i]->mCollision.size(); ++hullNum)
			{
				apex::deserialize(stream, apexStreamVersion, mParts[i]->mCollision[hullNum]->impl);
			}
			mParts[i]->mMeshBSP = createBSP(mBSPMemCache);
			uint32_t createMeshBSP;
			stream >> createMeshBSP;
			if (createMeshBSP)
			{
				mParts[i]->mMeshBSP->deserialize(stream);
			}
			else
			{
				ApexCSG::BSPBuildParameters bspBuildParameters = gDefaultBuildParameters;
				bspBuildParameters.internalTransform = physx::PxMat44(physx::PxIdentity);
				bspBuildParameters.rnd = &userRnd;
				userRnd.m_rnd.setSeed(0);
				mParts[i]->mMeshBSP->fromMesh(&mParts[i]->mMesh[0], mParts[i]->mMesh.size(), bspBuildParameters);
			}
			if (meshStreamVersion >= ExplicitHierarchicalMeshImpl::ReaddedFlagsToPart)
			{
				stream >> mParts[i]->mFlags;
			}
		}

		uint32_t chunkCount;
		stream >> chunkCount;
		mChunks.resize(chunkCount);
		for (uint32_t i = 0; i < chunkCount; ++i)
		{
			mChunks[i] = PX_NEW(Chunk);
			stream >> mChunks[i]->mParentIndex;
			stream >> mChunks[i]->mFlags;
			stream >> mChunks[i]->mPartIndex;
			stream >> mChunks[i]->mInstancedPositionOffset;
			if (meshStreamVersion >= ExplicitHierarchicalMeshImpl::UVInstancingData)
			{
				stream >> mChunks[i]->mInstancedUVOffset;
			}
			if (meshStreamVersion >= ExplicitHierarchicalMeshImpl::IntroducingChunkPrivateFlags)
			{
				stream >> mChunks[i]->mPrivateFlags;
			}
		}
	}
	else
	{
		if (meshStreamVersion >= ExplicitHierarchicalMeshImpl::UsingExplicitPartContainers)
		{
			uint32_t partCount;
			stream >> partCount;
			mParts.resize(partCount);
			mChunks.resize(partCount);
			for (uint32_t i = 0; i < partCount; ++i)
			{
				mParts[i] = PX_NEW(Part);
				mChunks[i] = PX_NEW(Chunk);
				stream >> mChunks[i]->mParentIndex;
				if (meshStreamVersion >= ExplicitHierarchicalMeshImpl::SerializingMeshBounds)
				{
					stream >> mParts[i]->mBounds;
				}
				apex::deserialize(stream, apexStreamVersion, mParts[i]->mMesh);
				if (meshStreamVersion < ExplicitHierarchicalMeshImpl::SerializingMeshBounds)
				{
					buildMeshBounds(i);
				}
				if (meshStreamVersion >= ExplicitHierarchicalMeshImpl::MultipleConvexHullsPerChunk)
				{
					resizeCollision(mParts[i]->mCollision, stream.readDword());
				}
				else
				{
					resizeCollision(mParts[i]->mCollision, 1);
				}
				for (uint32_t hullNum = 0; hullNum < mParts[i]->mCollision.size(); ++hullNum)
				{
					apex::deserialize(stream, apexStreamVersion, mParts[i]->mCollision[hullNum]->impl);
				}
				if (meshStreamVersion >= ExplicitHierarchicalMeshImpl::PerPartMeshBSPs)
				{
					mParts[i]->mMeshBSP = createBSP(mBSPMemCache);
					uint32_t createMeshBSP;
					stream >> createMeshBSP;
					if (createMeshBSP)
					{
						mParts[i]->mMeshBSP->deserialize(stream);
					}
					else
					{
						ApexCSG::BSPBuildParameters bspBuildParameters = gDefaultBuildParameters;
						bspBuildParameters.internalTransform = physx::PxMat44(physx::PxIdentity);
						mParts[i]->mMeshBSP->fromMesh(&mParts[i]->mMesh[0], mParts[i]->mMesh.size(), bspBuildParameters);
					}
				}
				if (meshStreamVersion >= ExplicitHierarchicalMeshImpl::AddedFlagsFieldToPart)
				{
					stream >> mChunks[i]->mFlags;
				}
			}
		}
		else
		{
			physx::Array<int32_t> parentIndices;
			physx::Array< physx::Array< ExplicitRenderTriangle > > meshes;
			physx::Array< ConvexHullImpl > meshHulls;
			apex::deserialize(stream, apexStreamVersion, parentIndices);
			apex::deserialize(stream, apexStreamVersion, meshes);
			apex::deserialize(stream, apexStreamVersion, meshHulls);
			PX_ASSERT(parentIndices.size() == meshes.size() && meshes.size() == meshHulls.size());
			uint32_t partCount = PxMin(parentIndices.size(), PxMin(meshes.size(), meshHulls.size()));
			mParts.resize(partCount);
			mChunks.resize(partCount);
			for (uint32_t i = 0; i < partCount; ++i)
			{
				mParts[i] = PX_NEW(Part);
				mChunks[i] = PX_NEW(Chunk);
				mChunks[i]->mParentIndex = parentIndices[i];
				mParts[i]->mMesh = meshes[i];
				resizeCollision(mParts[i]->mCollision, 1);
				mParts[i]->mCollision[0]->impl = meshHulls[i];
				buildMeshBounds(i);
			}
		}
		for (uint32_t i = 0; i < mChunks.size(); ++i)
		{
			mChunks[i]->mPartIndex = (int32_t)i;
		}
	}

	if (meshStreamVersion >= ExplicitHierarchicalMeshImpl::SerializingMeshBSP && meshStreamVersion < ExplicitHierarchicalMeshImpl::PerPartMeshBSPs)
	{
		mParts[0]->mMeshBSP = createBSP(mBSPMemCache);
		uint32_t createMeshBSP;
		stream >> createMeshBSP;
		if (createMeshBSP)
		{
			mParts[0]->mMeshBSP->deserialize(stream);
		}
		else
		{
			ApexCSG::BSPBuildParameters bspBuildParameters = gDefaultBuildParameters;
			bspBuildParameters.internalTransform = physx::PxMat44(physx::PxIdentity);
			mParts[0]->mMeshBSP->fromMesh(&mParts[0]->mMesh[0], mParts[0]->mMesh.size(), bspBuildParameters);
		}
	}

	if (meshStreamVersion >= ExplicitHierarchicalMeshImpl::IncludingVertexFormatInSubmeshData)
	{
		apex::deserialize(stream, apexStreamVersion, meshStreamVersion, mSubmeshData);
	}
	else
	{
		physx::Array<ApexSimpleString> materialNames;
		apex::deserialize(stream, apexStreamVersion, materialNames);
		mSubmeshData.resize(0);	// Make sure the next resize calls constructors
		mSubmeshData.resize(materialNames.size());
		for (uint32_t i = 0; i < materialNames.size(); ++i)
		{
			nvidia::strlcpy(mSubmeshData[i].mMaterialName, ExplicitSubmeshData::MaterialNameBufferSize, materialNames[i].c_str());
			mSubmeshData[i].mVertexFormat.mHasStaticPositions = true;
			mSubmeshData[i].mVertexFormat.mHasStaticNormals = true;
			mSubmeshData[i].mVertexFormat.mHasStaticTangents = true;
			mSubmeshData[i].mVertexFormat.mHasStaticBinormals = true;
			mSubmeshData[i].mVertexFormat.mHasStaticColors = true;
			mSubmeshData[i].mVertexFormat.mHasStaticDisplacements = false;
			mSubmeshData[i].mVertexFormat.mUVCount = 1;
			mSubmeshData[i].mVertexFormat.mBonesPerVertex = 1;
		}
	}

	if (meshStreamVersion >= ExplicitHierarchicalMeshImpl::AddedMaterialFramesToHMesh_and_NoiseType_and_GridSize_to_Cleavage)
	{
		if (meshStreamVersion >= ExplicitHierarchicalMeshImpl::ChangedMaterialFrameToIncludeFracturingMethodContext)
		{
			apex::deserialize(stream, apexStreamVersion, meshStreamVersion, mMaterialFrames);
		}
		else
		{
			const uint32_t size = stream.readDword();
			mMaterialFrames.resize(size);
			for (uint32_t i = 0; i < size; ++i)
			{
				PxMat44 &m44 = mMaterialFrames[i].mCoordinateSystem;
				stream >> m44(0, 0) >> m44(0, 1) >> m44(0, 2)
					>> m44(1, 0) >> m44(1, 1) >> m44(1, 2)
					>> m44(2, 0) >> m44(2, 1) >> m44(2, 2) >> *reinterpret_cast<PxVec3*>(&m44.column3);
				mMaterialFrames[i].mUVPlane = physx::PxPlane(m44.getPosition(), m44.column2.getXYZ());
				mMaterialFrames[i].mUVScale = physx::PxVec2(1.0f);
				mMaterialFrames[i].mUVOffset = physx::PxVec2(0.0f);
				mMaterialFrames[i].mFractureMethod = nvidia::FractureMethod::Unknown;
				mMaterialFrames[i].mFractureIndex = -1;
			}
		}
	}
	else
	{
		mMaterialFrames.resize(0);
	}

	if (meshStreamVersion >= ExplicitHierarchicalMeshImpl::AddedMaterialLibraryToMesh)
	{
		embedding.deserialize(stream, Embedding::MaterialLibrary, meshStreamVersion);
	}

	if (meshStreamVersion >= ExplicitHierarchicalMeshImpl::AddedCacheChunkSurfaceTracesAndInteriorSubmeshIndex && meshStreamVersion < ExplicitHierarchicalMeshImpl::RemovedInteriorSubmeshIndex)
	{
		int32_t interiorSubmeshIndex;
		stream >> interiorSubmeshIndex;
	}


	if (meshStreamVersion < ExplicitHierarchicalMeshImpl::IntroducingChunkPrivateFlags)
	{
		uint32_t rootDepth = 0;
		if (meshStreamVersion >= ExplicitHierarchicalMeshImpl::PerPartMeshBSPs)
		{
			stream >> rootDepth;
		}

		for (uint32_t i = 0; i < mChunks.size(); ++i)
		{
			mChunks[i]->mPrivateFlags = 0;
			const uint32_t chunkDepth = depth(i);
			if (chunkDepth <= rootDepth)
			{
				mChunks[i]->mPrivateFlags |= Chunk::Root;
				if (chunkDepth == rootDepth)
				{
					mChunks[i]->mPrivateFlags |= Chunk::RootLeaf;
				}
			}
		}
	}

	if (meshStreamVersion >= ExplicitHierarchicalMeshImpl::StoringRootSubmeshCount)
	{
		stream >> mRootSubmeshCount;
	}
	else
	{
		mRootSubmeshCount = mSubmeshData.size();
	}

	if (meshStreamVersion < ExplicitHierarchicalMeshImpl::RemovedNxChunkAuthoringFlag)
	{
		/* Need to translate flags:
			IsCutoutFaceSplit =	(1U << 0),
			IsCutoutLeftover =	(1U << 1),
			Instance =			(1U << 31)
		*/
		for (uint32_t chunkIndex = 0; chunkIndex < mChunks.size(); ++chunkIndex)
		{
			// IsCutoutFaceSplit and IsCutoutLeftover are no longer used.
			// Translate Instance flag:
			if (mChunks[chunkIndex]->mFlags & (1U << 31))
			{
				mChunks[chunkIndex]->mFlags = nvidia::apex::DestructibleAsset::ChunkIsInstanced;
			}
		}
	}
}

int32_t ExplicitHierarchicalMeshImpl::maxDepth() const
{
	int32_t max = -1;
	int32_t index = (int32_t)chunkCount()-1;
	while (index >= 0)
	{
		index = mChunks[(uint32_t)index]->mParentIndex;
		++max;
	}
	return max;
}

uint32_t ExplicitHierarchicalMeshImpl::partCount() const
{
	return mParts.size();
}

uint32_t ExplicitHierarchicalMeshImpl::chunkCount() const
{
	return mChunks.size();
}

int32_t* ExplicitHierarchicalMeshImpl::parentIndex(uint32_t chunkIndex)
{
	return chunkIndex < chunkCount() ? &mChunks[chunkIndex]->mParentIndex : NULL;
}

uint64_t ExplicitHierarchicalMeshImpl::chunkUniqueID(uint32_t chunkIndex)
{
	return chunkIndex < chunkCount() ? mChunks[chunkIndex]->getEUID() : (uint64_t)0;
}

int32_t* ExplicitHierarchicalMeshImpl::partIndex(uint32_t chunkIndex)
{
	return chunkIndex < chunkCount() ? &mChunks[chunkIndex]->mPartIndex : NULL;
}

physx::PxVec3* ExplicitHierarchicalMeshImpl::instancedPositionOffset(uint32_t chunkIndex)
{
	return chunkIndex < chunkCount() ? &mChunks[chunkIndex]->mInstancedPositionOffset : NULL;
}

physx::PxVec2* ExplicitHierarchicalMeshImpl::instancedUVOffset(uint32_t chunkIndex)
{
	return chunkIndex < chunkCount() ? &mChunks[chunkIndex]->mInstancedUVOffset : NULL;
}

uint32_t ExplicitHierarchicalMeshImpl::depth(uint32_t chunkIndex) const
{
	if (chunkIndex >= mChunks.size())
	{
		return 0;
	}

	uint32_t depth = 0;
	int32_t index = (int32_t)chunkIndex;
	while ((index = mChunks[(uint32_t)index]->mParentIndex) >= 0)
	{
		++depth;
	}

	return depth;
}

uint32_t ExplicitHierarchicalMeshImpl::meshTriangleCount(uint32_t partIndex) const
{
	return partIndex < partCount() ? mParts[partIndex]->mMesh.size() : 0;
}

ExplicitRenderTriangle* ExplicitHierarchicalMeshImpl::meshTriangles(uint32_t partIndex)
{
	return partIndex < partCount() ? mParts[partIndex]->mMesh.begin() : NULL;
}

physx::PxBounds3 ExplicitHierarchicalMeshImpl::meshBounds(uint32_t partIndex) const
{
	physx::PxBounds3 bounds;
	bounds.setEmpty();
	if (partIndex < partCount())
	{
		bounds = mParts[partIndex]->mBounds;
	}
	return bounds;
}

physx::PxBounds3 ExplicitHierarchicalMeshImpl::chunkBounds(uint32_t chunkIndex) const
{
	physx::PxBounds3 bounds;
	bounds.setEmpty();

	if (chunkIndex < chunkCount())
	{
		bounds = mParts[(uint32_t)mChunks[chunkIndex]->mPartIndex]->mBounds;
		bounds.minimum += mChunks[chunkIndex]->mInstancedPositionOffset;
		bounds.maximum += mChunks[chunkIndex]->mInstancedPositionOffset;
	}
	return bounds;
}

uint32_t* ExplicitHierarchicalMeshImpl::chunkFlags(uint32_t chunkIndex) const
{
	if (chunkIndex < chunkCount())
	{
		return &mChunks[chunkIndex]->mFlags;
	}
	return NULL;
}

uint32_t ExplicitHierarchicalMeshImpl::convexHullCount(uint32_t partIndex) const
{
	if (partIndex < partCount())
	{
		return mParts[partIndex]->mCollision.size();
	}
	return 0;
}

const ExplicitHierarchicalMeshImpl::ConvexHull** ExplicitHierarchicalMeshImpl::convexHulls(uint32_t partIndex) const
{
	if (partIndex < partCount())
	{
		Part* part = mParts[partIndex];
		return part->mCollision.size() > 0 ? (const ExplicitHierarchicalMeshImpl::ConvexHull**)&part->mCollision[0] : NULL;
	}
	return NULL;
}

physx::PxVec3* ExplicitHierarchicalMeshImpl::surfaceNormal(uint32_t partIndex)
{
	if (partIndex < partCount())
	{
		Part* part = mParts[partIndex];
		return &part->mSurfaceNormal;
	}
	return NULL;
}

const DisplacementMapVolume& ExplicitHierarchicalMeshImpl::displacementMapVolume() const
{
	return mDisplacementMapVolume;
}

uint32_t ExplicitHierarchicalMeshImpl::submeshCount() const
{
	return mSubmeshData.size();
}

ExplicitSubmeshData* ExplicitHierarchicalMeshImpl::submeshData(uint32_t submeshIndex)
{
	return submeshIndex < mSubmeshData.size() ? mSubmeshData.begin() + submeshIndex : NULL;
}

uint32_t ExplicitHierarchicalMeshImpl::addSubmesh(const ExplicitSubmeshData& submeshData)
{
	const uint32_t index = mSubmeshData.size();
	mSubmeshData.pushBack(submeshData);
	return index;
}

uint32_t ExplicitHierarchicalMeshImpl::getMaterialFrameCount() const
{
	return mMaterialFrames.size();
}

nvidia::MaterialFrame ExplicitHierarchicalMeshImpl::getMaterialFrame(uint32_t index) const
{
	return mMaterialFrames[index];
}

void ExplicitHierarchicalMeshImpl::setMaterialFrame(uint32_t index, const nvidia::MaterialFrame& materialFrame)
{
	mMaterialFrames[index] = materialFrame;
}

uint32_t ExplicitHierarchicalMeshImpl::addMaterialFrame()
{
	mMaterialFrames.insert();
	return mMaterialFrames.size()-1;
}

void ExplicitHierarchicalMeshImpl::clear(bool keepRoot)
{
	uint32_t newPartCount = 0;
	uint32_t index = chunkCount();
	while (index-- > 0)
	{
		if (!keepRoot || !mChunks[index]->isRootChunk())
		{
			removeChunk(index);
		}
		else
		{
			newPartCount = PxMax(newPartCount, (uint32_t)(mChunks[index]->mPartIndex+1));
		}
	}

	while (newPartCount < partCount())
	{
		removePart(partCount()-1);
	}

	mMaterialFrames.resize(0);

	if (!keepRoot)
	{
		mSubmeshData.reset();
		mBSPMemCache->clearAll();
		mRootSubmeshCount = 0;
	}
}

void ExplicitHierarchicalMeshImpl::sortChunks(physx::Array<uint32_t>* indexRemap)
{
	if (mChunks.size() <= 1)
	{
		return;
	}

	// Sort by original parent index
	physx::Array<ChunkIndexer> chunkIndices(mChunks.size());
	for (uint32_t i = 0; i < mChunks.size(); ++i)
	{
		chunkIndices[i].chunk = mChunks[i];
		chunkIndices[i].parentIndex = mChunks[i]->mParentIndex;
		chunkIndices[i].index = (int32_t)i;
	}
	qsort(chunkIndices.begin(), chunkIndices.size(), sizeof(ChunkIndexer), ChunkIndexer::compareParentIndices);

	// Now arrange in depth order
	physx::Array<uint32_t> parentStarts;
	createIndexStartLookup(parentStarts, -1, chunkIndices.size() + 1, &chunkIndices[0].parentIndex, chunkIndices.size(), sizeof(ChunkIndexer));

	physx::Array<ChunkIndexer> newChunkIndices;
	newChunkIndices.reserve(mChunks.size());
	int32_t parentIndex = -1;
	uint32_t nextPart = 0;
	while (newChunkIndices.size() < mChunks.size())
	{
		const uint32_t start = parentStarts[(uint32_t)parentIndex + 1];
		const uint32_t stop = parentStarts[(uint32_t)parentIndex + 2];
		for (uint32_t index = start; index < stop; ++index)
		{
			newChunkIndices.pushBack(chunkIndices[index]);
		}
		parentIndex = newChunkIndices[nextPart++].index;
	}

	// Remap the parts and parent indices
	physx::Array<uint32_t> internalRemap;
	physx::Array<uint32_t>& remap = indexRemap != NULL ? *indexRemap : internalRemap;
	remap.resize(newChunkIndices.size());
	for (uint32_t i = 0; i < newChunkIndices.size(); ++i)
	{
		mChunks[i] = newChunkIndices[i].chunk;
		remap[(uint32_t)newChunkIndices[i].index] = i;
	}
	for (uint32_t i = 0; i < mChunks.size(); ++i)
	{
		if (mChunks[i]->mParentIndex >= 0)
		{
			mChunks[i]->mParentIndex = (int32_t)remap[(uint32_t)mChunks[i]->mParentIndex];
		}
	}
}

void ExplicitHierarchicalMeshImpl::createPartSurfaceNormals()
{
	for (uint32_t partIndex = 0; partIndex < mParts.size(); ++partIndex)
	{
		Part* part = mParts[partIndex];
		physx::Array<ExplicitRenderTriangle>& mesh = part->mMesh;
		physx::PxVec3 normal(0.0f);
		for (uint32_t triangleIndex = 0; triangleIndex < mesh.size(); ++triangleIndex)
		{
			ExplicitRenderTriangle& triangle = mesh[triangleIndex];
			if (triangle.extraDataIndex == 0xFFFFFFFF)
			{
				normal += (triangle.vertices[1].position - triangle.vertices[0].position).cross(triangle.vertices[2].position - triangle.vertices[0].position);
			}
		}
		part->mSurfaceNormal = normal.getNormalized();
	}
}

void ExplicitHierarchicalMeshImpl::set(const ExplicitHierarchicalMesh& mesh)
{
	const ExplicitHierarchicalMeshImpl& m = (const ExplicitHierarchicalMeshImpl&)mesh;
	clear();
	mParts.resize(0);
	mParts.reserve(m.mParts.size());
	for (uint32_t i = 0; i < m.mParts.size(); ++i)
	{
		const uint32_t newPartIndex = addPart();
		PX_ASSERT(newPartIndex == i);
		mParts[newPartIndex]->mBounds = m.mParts[i]->mBounds;
		mParts[newPartIndex]->mMesh = m.mParts[i]->mMesh;
		PX_ASSERT(m.mParts[i]->mMeshBSP != NULL);
		mParts[newPartIndex]->mMeshBSP->copy(*m.mParts[i]->mMeshBSP);
		resizeCollision(mParts[newPartIndex]->mCollision, m.mParts[i]->mCollision.size());
		for (uint32_t j = 0; j < mParts[newPartIndex]->mCollision.size(); ++j)
		{
			mParts[newPartIndex]->mCollision[j]->impl = m.mParts[i]->mCollision[j]->impl;
		}
		mParts[newPartIndex]->mFlags = m.mParts[i]->mFlags;
	}
	mChunks.resize(m.mChunks.size());
	for (uint32_t i = 0; i < mChunks.size(); ++i)
	{
		mChunks[i] = PX_NEW(Chunk);
		mChunks[i]->mParentIndex = m.mChunks[i]->mParentIndex;
		mChunks[i]->mFlags = m.mChunks[i]->mFlags;
		mChunks[i]->mPartIndex = m.mChunks[i]->mPartIndex;
		mChunks[i]->mInstancedPositionOffset = m.mChunks[i]->mInstancedPositionOffset;
		mChunks[i]->mInstancedUVOffset = m.mChunks[i]->mInstancedUVOffset;
		mChunks[i]->mPrivateFlags = m.mChunks[i]->mPrivateFlags;
	}
	mSubmeshData = m.mSubmeshData;
	mMaterialFrames = m.mMaterialFrames;
	mRootSubmeshCount = m.mRootSubmeshCount;
}

static void buildCollisionGeometryForPartInternal(physx::Array<PartConvexHullProxy*>& volumes, ExplicitHierarchicalMeshImpl::Part* part, const CollisionVolumeDesc& desc, float inflation = 0.0f)
{
	uint32_t vertexCount = part->mMesh.size() * 3;
	if (inflation > 0.0f)
	{
		vertexCount *= 7;	// Will add vertices
	}
	physx::Array<physx::PxVec3> vertices(vertexCount);
	uint32_t vertexN = 0;
	for (uint32_t i = 0; i < part->mMesh.size(); ++i)
	{
		nvidia::ExplicitRenderTriangle& triangle = part->mMesh[i];
		for (uint32_t v = 0; v < 3; ++v)
		{
			const physx::PxVec3& position = triangle.vertices[v].position;
			vertices[vertexN++] = position;
			if (inflation > 0.0f)
			{
				for (uint32_t j = 0; j < 3; ++j)
				{
					physx::PxVec3 offset(0.0f);
					offset[j] = inflation;
					for (uint32_t k = 0; k < 2; ++k)
					{
						vertices[vertexN++] = position + offset;
						offset[j] *= -1.0f;
					}
				}
			}
		}
	}

	// Identity index buffer
	PX_ALLOCA(indices, uint32_t, vertices.size());
	for (uint32_t i = 0; i < vertices.size(); ++i)
	{
		indices[i] = i;
	}

	buildCollisionGeometry(volumes, desc, vertices.begin(), vertices.size(), sizeof(physx::PxVec3), indices, vertices.size());
}

bool ExplicitHierarchicalMeshImpl::calculatePartBSP(uint32_t partIndex, uint32_t randomSeed, uint32_t microgridSize, BSPOpenMode::Enum meshMode, IProgressListener* progressListener, volatile bool* cancel)
{
	if (partIndex >= mParts.size())
	{
		return false;
	}

	PX_ASSERT(mParts[partIndex]->mMeshBSP != NULL);

	ApexCSG::BSPBuildParameters bspBuildParameters = gDefaultBuildParameters;
	bspBuildParameters.snapGridSize = microgridSize;
	bspBuildParameters.internalTransform = physx::PxMat44(physx::PxZero);
	bspBuildParameters.rnd = &userRnd;
	userRnd.m_rnd.setSeed(randomSeed);
	bool ok = mParts[partIndex]->mMeshBSP->fromMesh(&mParts[partIndex]->mMesh[0], mParts[partIndex]->mMesh.size(), bspBuildParameters, progressListener, cancel);
	if (!ok)
	{
		return false;
	}

	// Check for open mesh
	if (meshMode == BSPOpenMode::Closed)
	{
		return true;
	}

	for (uint32_t chunkIndex = 0; chunkIndex < chunkCount(); ++chunkIndex)
	{
		// Find a chunk which uses this part
		if ((uint32_t)mChunks[chunkIndex]->mPartIndex == partIndex)
		{
			// If the chunk is a root chunk, test for openness
			if (mChunks[chunkIndex]->isRootChunk())
			{
				float area, volume;
				if (meshMode == BSPOpenMode::Open || !mParts[partIndex]->mMeshBSP->getSurfaceAreaAndVolume(area, volume, true))
				{
					// Mark the mesh as open
					mParts[partIndex]->mFlags |= Part::MeshOpen;
					// Instead of using this mesh's BSP, use the convex hull
					physx::Array<PartConvexHullProxy*> volumes;
					CollisionVolumeDesc collisionDesc;
					collisionDesc.mHullMethod = nvidia::ConvexHullMethod::WRAP_GRAPHICS_MESH;
					buildCollisionGeometryForPartInternal(volumes, mParts[partIndex], collisionDesc, mParts[partIndex]->mBounds.getExtents().magnitude()*0.01f);
					PX_ASSERT(volumes.size() == 1);
					if (volumes.size() > 0)
					{
						PartConvexHullProxy& hull = *volumes[0];
						physx::Array<physx::PxPlane> planes;
						planes.resize(hull.impl.getPlaneCount());
						const physx::PxVec3 extents = hull.impl.getBounds().getExtents();
						const float padding = 0.001f*extents.magnitude();
						for (uint32_t planeIndex = 0; planeIndex < hull.impl.getPlaneCount(); ++planeIndex)
						{
							planes[planeIndex] = hull.impl.getPlane(planeIndex);
							planes[planeIndex].d -= padding;
						}
						physx::PxMat44 internalTransform = physx::PxMat44(physx::PxIdentity);
						const physx::PxVec3 scale(1.0f/extents[0], 1.0f/extents[1], 1.0f/extents[2]);
						internalTransform.scale(physx::PxVec4(scale, 1.0f));
						internalTransform.setPosition(-scale.multiply(hull.impl.getBounds().getCenter()));
						mParts[partIndex]->mMeshBSP->fromConvexPolyhedron(&planes[0], planes.size(), internalTransform, &mParts[partIndex]->mMesh[0], mParts[partIndex]->mMesh.size());
					}
				}
			}
			break;
		}
	}

	return true;
}

void ExplicitHierarchicalMeshImpl::replaceInteriorSubmeshes(uint32_t partIndex, uint32_t frameCount, uint32_t* frameIndices, uint32_t submeshIndex)
{
	if (partIndex >= mParts.size())
	{
		return;
	}

	Part* part = mParts[partIndex];

	// Replace render mesh submesh indices
	for (uint32_t triangleIndex = 0; triangleIndex < part->mMesh.size(); ++triangleIndex)
	{
		ExplicitRenderTriangle& triangle = part->mMesh[triangleIndex];
		for (uint32_t frameNum = 0; frameNum < frameCount; ++frameNum)
		{
			if (triangle.extraDataIndex == frameIndices[frameNum])
			{
				triangle.submeshIndex = (int32_t)submeshIndex;
			}
		}
	}

	// Replace BSP mesh submesh indices
	part->mMeshBSP->replaceInteriorSubmeshes(frameCount, frameIndices, submeshIndex);
}

void ExplicitHierarchicalMeshImpl::calculateMeshBSP(uint32_t randomSeed, IProgressListener* progressListener, const uint32_t* microgridSize, BSPOpenMode::Enum meshMode)
{
	if (partCount() == 0)
	{
		outputMessage("No mesh, cannot calculate BSP.", physx::PxErrorCode::eDEBUG_WARNING);
		return;
	}

	uint32_t bspCount = 0;
	for (uint32_t chunkIndex = 0; chunkIndex < mChunks.size(); ++chunkIndex)
	{
		if (mChunks[chunkIndex]->isRootLeafChunk())
		{
			++bspCount;
		}
	}

	if (bspCount == 0)
	{
		outputMessage("No parts at root depth, no BSPs to calculate", physx::PxErrorCode::eDEBUG_WARNING);
		return;
	}

	HierarchicalProgressListener progress(PxMax((int32_t)bspCount, 1), progressListener);

	const uint32_t microgridSizeToUse = microgridSize != NULL ? *microgridSize : gMicrogridSize;

	for (uint32_t chunkIndex = 0; chunkIndex < mChunks.size(); ++chunkIndex)
	{
		if (mChunks[chunkIndex]->isRootLeafChunk())
		{
			uint32_t chunkPartIndex = (uint32_t)*partIndex(chunkIndex);
			calculatePartBSP(chunkPartIndex, randomSeed, microgridSizeToUse, meshMode, &progress);
			progress.completeSubtask();
		}
	}
}

void ExplicitHierarchicalMeshImpl::visualize(RenderDebugInterface& debugRender, uint32_t flags, uint32_t index) const
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(debugRender);
	PX_UNUSED(flags);
	PX_UNUSED(index);
#else
	uint32_t bspMeshFlags = 0;
	if (flags & VisualizeMeshBSPInsideRegions)
	{
		bspMeshFlags |= ApexCSG::BSPVisualizationFlags::InsideRegions;
	}
	if (flags & VisualizeMeshBSPOutsideRegions)
	{
		bspMeshFlags |= ApexCSG::BSPVisualizationFlags::OutsideRegions;
	}
	if (flags & VisualizeMeshBSPSingleRegion)
	{
		bspMeshFlags |= ApexCSG::BSPVisualizationFlags::SingleRegion;
	}
	for (uint32_t partIndex = 0; partIndex < mParts.size(); ++partIndex)
	{
		if (mParts[partIndex]->mMeshBSP != NULL)
		{
			mParts[partIndex]->mMeshBSP->visualize(debugRender, bspMeshFlags, index);
		}
	}
#endif
}

void ExplicitHierarchicalMeshImpl::release()
{
	delete this;
}

void ExplicitHierarchicalMeshImpl::buildMeshBounds(uint32_t partIndex)
{
	if (partIndex < partCount())
	{
		physx::PxBounds3& bounds = mParts[partIndex]->mBounds;
		bounds.setEmpty();
		const physx::Array<ExplicitRenderTriangle>& mesh = mParts[partIndex]->mMesh;
		for (uint32_t i = 0; i < mesh.size(); ++i)
		{
			bounds.include(mesh[i].vertices[0].position);
			bounds.include(mesh[i].vertices[1].position);
			bounds.include(mesh[i].vertices[2].position);
		}
	}
}

void ExplicitHierarchicalMeshImpl::buildCollisionGeometryForPart(uint32_t partIndex, const CollisionVolumeDesc& desc)
{
	if (partIndex < partCount())
	{
		Part* part = mParts[partIndex];
		buildCollisionGeometryForPartInternal(part->mCollision, part, desc);
	}
}

void ExplicitHierarchicalMeshImpl::aggregateCollisionHullsFromRootChildren(uint32_t chunkIndex)
{
	nvidia::InlineArray<uint32_t,16> rootChildren;
	for (uint32_t i = 0; i < mChunks.size(); ++i)
	{
		if (mChunks[i]->mParentIndex == (int32_t)chunkIndex && mChunks[i]->isRootChunk())
		{
			rootChildren.pushBack(i);
		}
	}

	if (rootChildren.size() != 0)
	{
		uint32_t newHullCount = 0;
		for (uint32_t rootChildNum = 0; rootChildNum < rootChildren.size(); ++rootChildNum)
		{
			const uint32_t rootChild = rootChildren[rootChildNum];
			aggregateCollisionHullsFromRootChildren(rootChild);
			const uint32_t childPartIndex = (uint32_t)mChunks[rootChild]->mPartIndex;
			newHullCount += mParts[childPartIndex]->mCollision.size();
		}
		const uint32_t partIndex = (uint32_t)mChunks[chunkIndex]->mPartIndex;
		resizeCollision(mParts[partIndex]->mCollision, newHullCount);
		newHullCount = 0;
		for (uint32_t rootChildNum = 0; rootChildNum < rootChildren.size(); ++rootChildNum)
		{
			const uint32_t rootChild = rootChildren[rootChildNum];
			const uint32_t childPartIndex = (uint32_t)mChunks[rootChild]->mPartIndex;
			for (uint32_t hullN = 0; hullN < mParts[childPartIndex]->mCollision.size(); ++hullN)
			{
				*mParts[partIndex]->mCollision[newHullCount++] = *mParts[childPartIndex]->mCollision[hullN];
			}
		}
		PX_ASSERT(newHullCount == mParts[partIndex]->mCollision.size());
	}
}

void ExplicitHierarchicalMeshImpl::buildCollisionGeometryForRootChunkParts(const CollisionDesc& desc, bool aggregateRootChunkParentCollision)
{
	// This helps keep the loops small if there are a lot of child chunks
	uint32_t rootChunkStop = 0;

	for (uint32_t chunkIndex = 0; chunkIndex < chunkCount(); ++chunkIndex)
	{
		if (mChunks[chunkIndex]->isRootChunk())
		{
			rootChunkStop = chunkIndex+1;
			const uint32_t partIndex = (uint32_t)mChunks[chunkIndex]->mPartIndex;
			if (partIndex < mParts.size())
			{
				resizeCollision(mParts[partIndex]->mCollision, 0);
			}
		}
	}

	for (uint32_t chunkIndex = 0; chunkIndex < rootChunkStop; ++chunkIndex)
	{
		if (mChunks[chunkIndex]->isRootLeafChunk() || (mChunks[chunkIndex]->isRootChunk() && !aggregateRootChunkParentCollision))
		{
			const uint32_t partIndex = (uint32_t)mChunks[chunkIndex]->mPartIndex;
			if (partIndex < mParts.size() && mParts[partIndex]->mCollision.size() == 0)
			{
				CollisionVolumeDesc volumeDesc = getVolumeDesc(desc, depth(chunkIndex));
				volumeDesc.mMaxVertexCount = volumeDesc.mMaxEdgeCount = volumeDesc.mMaxFaceCount = 0;	// Don't reduce hulls until the very end
				buildCollisionGeometryForPart(partIndex, volumeDesc);
			}
		}
	}

	if (aggregateRootChunkParentCollision)
	{
		// Aggregate collision volumes from root depth chunks to their parents, recursing to depth 0
		aggregateCollisionHullsFromRootChildren(0);
	}

	if (desc.mMaximumTrimming > 0.0f)
	{
		// Trim hulls up to root depth
		for (uint32_t processDepth = 1; (int32_t)processDepth <= maxDepth(); ++processDepth)
		{
			physx::Array<uint32_t> chunkIndexArray;
			for (uint32_t chunkIndex = 0; chunkIndex < rootChunkStop; ++chunkIndex)
			{
				if (mChunks[chunkIndex]->isRootChunk() && depth(chunkIndex) == processDepth)
				{
					chunkIndexArray.pushBack(chunkIndex);
				}
			}
			if (chunkIndexArray.size() > 0)
			{
				trimChunkHulls(*this, &chunkIndexArray[0], chunkIndexArray.size(), desc.mMaximumTrimming);
			}
		}
	}

	// Finally reduce the hulls
	reduceHulls(desc, true);
}

void ExplicitHierarchicalMeshImpl::reduceHulls(const CollisionDesc& desc, bool inflated)
{
	physx::Array<bool> partReduced(mParts.size(), false);

	for (uint32_t chunkIndex = 0; chunkIndex < mChunks.size(); ++chunkIndex)
	{
		uint32_t partIndex = (uint32_t)mChunks[chunkIndex]->mPartIndex;
		if (partReduced[partIndex])
		{
			continue;
		}
		Part* part = mParts[partIndex];
		CollisionVolumeDesc volumeDesc = getVolumeDesc(desc, depth(chunkIndex));
		for (uint32_t hullIndex = 0; hullIndex < part->mCollision.size(); ++hullIndex)
		{
			// First try uninflated, then try with inflation (if requested).  This may find a better reduction
			part->mCollision[hullIndex]->reduceHull(volumeDesc.mMaxVertexCount, volumeDesc.mMaxEdgeCount, volumeDesc.mMaxFaceCount, false);
			if (inflated)
			{
				part->mCollision[hullIndex]->reduceHull(volumeDesc.mMaxVertexCount, volumeDesc.mMaxEdgeCount, volumeDesc.mMaxFaceCount, true);
			}
		}
		partReduced[partIndex] = true;
	}
}

void ExplicitHierarchicalMeshImpl::initializeDisplacementMapVolume(const nvidia::FractureSliceDesc& desc)
{
	mDisplacementMapVolume.init(desc);
}

}
} // namespace nvidia::apex

namespace FractureTools
{
ExplicitHierarchicalMesh* createExplicitHierarchicalMesh()
{
	return PX_NEW(ExplicitHierarchicalMeshImpl)();
}

ExplicitHierarchicalMeshImpl::ConvexHull*	createExplicitHierarchicalMeshConvexHull()
{
	return PX_NEW(PartConvexHullProxy)();
}
} // namespace FractureTools

#endif // !defined(WITHOUT_APEX_AUTHORING)

//#ifdef _MANAGED
//#pragma managed(pop)
//#endif
