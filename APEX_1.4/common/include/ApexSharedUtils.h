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



#ifndef APEXSHAREDUTILS_H
#define APEXSHAREDUTILS_H

#include "ApexUsingNamespace.h"
#include "ApexDefs.h"
#include "IProgressListener.h"
#include "RenderMeshAsset.h"

#include "PxStreamFromFileBuf.h"

#include "ApexString.h"
#include "PsArray.h"
#include "ConvexHullMethod.h"
#include "PxPlane.h"

#include "ConvexHullParameters.h"

#include "Cof44.h"

#if PX_PHYSICS_VERSION_MAJOR == 3
namespace physx
{
class PxConvexMesh;
}
typedef physx::PxConvexMesh	ConvexMesh;
#endif

namespace nvidia
{
namespace apex
{

PX_INLINE PxPlane toPxPlane(const ConvexHullParametersNS::Plane_Type& plane)
{
	return PxPlane(plane.normal.x, plane.normal.y, plane.normal.z, plane.d);
}

/*
File-local functions and definitions
*/


/*
Global utilities
*/

// Diagonalize a symmetric 3x3 matrix.  Returns the eigenvectors in the first parameter, eigenvalues as the return value.
PxVec3 diagonalizeSymmetric(PxMat33& eigenvectors, const PxMat33& m);



PX_INLINE bool worldToLocalRay(PxVec3& localorig, PxVec3& localdir,
                               const PxVec3& worldorig, const PxVec3& worlddir,
                               const physx::PxTransform& localToWorldRT, const PxVec3& scale)
{
	// Invert scales
	const float detS = scale.x * scale.y * scale.z;
	if (detS == 0.0f)
	{
		return false;	// Not handling singular TMs
	}
	const float recipDetS = 1.0f / detS;

	// Is it faster to do a bunch of multiplies, or a few divides?
	const PxVec3 invS(scale.y * scale.z * recipDetS, scale.z * scale.x * recipDetS, scale.x * scale.y * recipDetS);

	// Create hull-local ray
	localorig = localToWorldRT.transformInv(worldorig);
	localorig = invS.multiply(localorig);
	localdir = localToWorldRT.rotateInv(worlddir);
	localdir = invS.multiply(localdir);

	return true;
}

// barycentric utilities
/**
This function starts recording the number of cycles elapsed.
\param		pa		[in] first vertex of triangle
\param		pb		[in] second vertex of triangle
\param		pc		[in] third vertex of triangle
\param		p		[in] vertex to generate barycentric coordinates for
\param		s		[out] the first barycentric coordinate
\param		t		[out] the second barycentric coordinate
\note the third barycentric coordinate is defined as (1 - s - t)
\see		EndProfile
*/
void generateBarycentricCoordinatesTri(const PxVec3& pa, const PxVec3& pb, const PxVec3& pc, const PxVec3& p, float& s, float& t);
void generateBarycentricCoordinatesTet(const PxVec3& pa, const PxVec3& pb, const PxVec3& pc, const PxVec3& pd, const PxVec3& p, PxVec3& bary);

struct OverlapLineSegmentAABBCache
{
	PxVec3	sgnDir;
	PxVec3	invDir;
};

PX_INLINE void computeOverlapLineSegmentAABBCache(OverlapLineSegmentAABBCache& cache, const PxVec3& segmentDisp)
{
	cache.sgnDir = PxVec3((float)(1 - (((int)(segmentDisp.x < 0.0f)) << 1)), (float)(1 - (((int)(segmentDisp.y < 0.0f)) << 1)), (float)(1 - (((int)(segmentDisp.z < 0.0f)) << 1)));
	PxVec3 absDir = cache.sgnDir.multiply(segmentDisp);
	absDir += PxVec3(PX_EPS_F32);	// To avoid divide-by-zero
	cache.invDir = PxVec3(absDir.y * absDir.z, absDir.z * absDir.x, absDir.x * absDir.y);
	cache.invDir *= 1.0f / (absDir.x * cache.invDir.x);
}

PX_INLINE bool overlapLineSegmentAABBCached(const PxVec3& segmentOrig, const OverlapLineSegmentAABBCache& cache, const PxBounds3& aabb)
{
	const PxVec3 center = 0.5f * (aabb.maximum + aabb.minimum);
	const PxVec3 radii = 0.5f * (aabb.maximum - aabb.minimum);
	PxVec3 disp = (center - segmentOrig).multiply(cache.sgnDir);
	PxVec3 tMin = (disp - radii).multiply(cache.invDir);
	PxVec3 tMax = (disp + radii).multiply(cache.invDir);
	int maxMinIndex = tMin.y > tMin.x;
	const int maxMinIndexIs2 = tMin.z > tMin[(unsigned int)maxMinIndex];
	maxMinIndex = (maxMinIndex | maxMinIndexIs2) << maxMinIndexIs2;
	int minMaxIndex = tMax.y < tMax.x;
	const int minMaxIndexIs2 = tMax.z > tMax[(unsigned int)minMaxIndex];
	minMaxIndex = (minMaxIndex | minMaxIndexIs2) << minMaxIndexIs2;
	const float tIn = tMin[(unsigned int)maxMinIndex];
	const float tOut = tMax[(unsigned int)minMaxIndex];
	return tIn < tOut && tOut > 0.0f && tIn < 1.0f;
}

PX_INLINE bool overlapLineSegmentAABB(const PxVec3& segmentOrig, const PxVec3& segmentDisp, const PxBounds3& aabb)
{
	OverlapLineSegmentAABBCache cache;
	computeOverlapLineSegmentAABBCache(cache, segmentDisp);
	return overlapLineSegmentAABBCached(segmentOrig, cache, aabb);
}

struct IntPair
{
	void	set(int32_t _i0, int32_t _i1)
	{
		i0 = _i0;
		i1 = _i1;
	}

	int32_t	i0, i1;

	static	int	compare(const void* a, const void* b)
	{
		const int32_t diff0 = ((IntPair*)a)->i0 - ((IntPair*)b)->i0;
		return diff0 ? diff0 : (((IntPair*)a)->i1 - ((IntPair*)b)->i1);
	}
};

PX_INLINE PxFileBuf& operator >> (PxFileBuf& stream, IntPair& p)
{
	p.i0 = (int32_t)stream.readDword();
	p.i1 = (int32_t)stream.readDword();
	return stream;
}
PX_INLINE PxFileBuf& operator << (PxFileBuf& stream, const IntPair& p)
{
	stream.storeDword((uint32_t)p.i0);
	stream.storeDword((uint32_t)p.i1);
	return stream;
}

struct BoundsRep
{
	BoundsRep() : type(0)
	{
		aabb.setEmpty();
	}

	PxBounds3 aabb;
	uint32_t	 type;	// By default only reports if subtypes are the same, configurable.  Valid range {0...7}
};

struct BoundsInteractions
{
	BoundsInteractions() : bits(0x8040201008040201ULL) {}
	BoundsInteractions(bool setAll) : bits(setAll ? 0xFFFFFFFFFFFFFFFFULL : 0x0000000000000000ULL) {}

	bool	set(unsigned group1, unsigned group2, bool interacts)
	{
		if (group1 >= 8 || group2 >= 8)
		{
			return false;
		}
		const uint64_t mask = (uint64_t)1 << ((group1 << 3) + group2) | (uint64_t)1 << ((group2 << 3) + group1);
		if (interacts)
		{
			bits |= mask;
		}
		else
		{
			bits &= ~mask;
		}
		return true;
	}

	uint64_t bits;
};

enum Bounds3Axes
{
	Bounds3X =		1,
	Bounds3Y =		2,
	Bounds3Z =		4,

	Bounds3XY =		Bounds3X | Bounds3Y,
	Bounds3YZ =		Bounds3Y | Bounds3Z,
	Bounds3ZX =		Bounds3Z | Bounds3X,

	Bounds3XYZ =	Bounds3X | Bounds3Y | Bounds3Z
};

void boundsCalculateOverlaps(physx::Array<IntPair>& overlaps, Bounds3Axes axesToUse, const BoundsRep* bounds, uint32_t boundsCount, uint32_t boundsByteStride,
                             const BoundsInteractions& interactions = BoundsInteractions(), bool append = false);


/*
Descriptor for building a ConvexHullImpl, below
*/
class ConvexHullDesc
{
public:
	const void*		vertices;
	uint32_t	numVertices;
	uint32_t	vertexStrideBytes;
	uint32_t*	indices;
	uint32_t	numIndices;
	uint8_t*	faceIndexCounts;
	uint32_t	numFaces;

	ConvexHullDesc() :
		vertices(NULL),
		numVertices(0),
		vertexStrideBytes(0),
		indices(NULL),
		numIndices(0),
		faceIndexCounts(NULL),
		numFaces(0)
	{
	}

	bool isValid() const
	{
		return
			vertices != NULL &&
			numVertices != 0 && 
			vertexStrideBytes != 0 &&
			indices != NULL &&
			numIndices != 0 &&
			faceIndexCounts != NULL &&
			numFaces != 0;
	}
};

/*
ConvexHullImpl - precomputed (redundant) information about a convex hull: vertices, hull planes, etc.
*/
class ConvexHullImpl
{
public:
	struct Separation
	{
		PxPlane	plane;
		float	min0, max0, min1, max1;

		float getDistance()
		{
			return PxMax(min0 - max1, min1 - max0);
		}
	};

	ConvexHullImpl();
	ConvexHullImpl(const ConvexHullImpl& hull) : mParams(NULL), mOwnsParams(false)
	{
		*this = hull;
	}
	virtual ~ConvexHullImpl();

	// If params == NULL, this will build (and own) its own internal parameters
	void			init(NvParameterized::Interface* params = NULL);

	ConvexHullImpl&		operator = (const ConvexHullImpl& hull)
	{
		mParams = hull.mParams;
		mOwnsParams = false;
		return *this;
	}

	// Only returns non-NULL value if this object owns its parameters.
	NvParameterized::Interface*	giveOwnersipOfNvParameters();

	// Releases parameters if it owns them
	void			term();

	void			buildFromDesc(const ConvexHullDesc& desc);
	void			buildFromPoints(const void* points, uint32_t numPoints, uint32_t pointStrideBytes);
	void			buildFromPlanes(const PxPlane* planes, uint32_t numPlanes, float eps);
#if PX_PHYSICS_VERSION_MAJOR == 3
	void			buildFromConvexMesh(const ConvexMesh* mesh);
#endif
	void			buildFromAABB(const PxBounds3& aabb);
	void			buildKDOP(const void* points, uint32_t numPoints, uint32_t pointStrideBytes, const PxVec3* directions, uint32_t numDirections);

	void			intersectPlaneSide(const PxPlane& plane);
	void			intersectHull(const ConvexHullImpl& hull);

	// If the distance between the hulls exceeds maxDistance, false is returned.
	// Otherwise, true is returned.  In this case, if 'separation' is not NULL, then separation plane
	// and projected extents are returned in *separation.
	static	bool	hullsInProximity(const ConvexHullImpl& hull0, const physx::PxTransform& localToWorldRT0, const PxVec3& scale0,
	                                 const ConvexHullImpl& hull1, const physx::PxTransform& localToWorldRT1, const PxVec3& scale1,
	                                 float maxDistance, Separation* separation = NULL);

	// If the distance between this hull and the given sphere exceeds maxDistance, false is returned.
	// Otherwise, true is returned.  In this case, if 'separation' is not NULL, then separation plane
	// and projected extents are returned in *separation.  The '0' values will correspond to the hull,
	// and the '1' values to the sphere.
	bool			sphereInProximity(const physx::PxTransform& hullLocalToWorldRT, const PxVec3& hullScale,
									  const PxVec3& sphereWorldCenter, float sphereRadius,
									  float maxDistance, Separation* separation = NULL);

#if PX_PHYSICS_VERSION_MAJOR == 3
	bool			intersects(const PxShape& shape, const physx::PxTransform& localToWorldRT, const PxVec3& scale, float padding) const;
#endif

	// worldRay.dir need not be normalized.  in & out times are relative to worldRay.dir length
	// N.B.  in & out are both input and output variables:
	// input:	in = minimum possible ray intersect time
	//			out = maximum possible ray intersect time
	// output:	in = time ray enters hull
	//			out = time ray exits hull
	bool			rayCast(float& in, float& out, const PxVec3& orig, const PxVec3& dir,
	                        const physx::PxTransform& localToWorldRT, const PxVec3& scale, PxVec3* normal = NULL) const;

	// in & out times are relative to worldDisp length
	// N.B.  in & out are both input and output variables:
	// input:	in = minimum possible ray intersect time
	//			out = maximum possible ray intersect time
	// output:	in = time ray enters hull
	//			out = time ray exits hull
	bool			obbSweep(float& in, float& out, const PxVec3& worldBoxCenter, const PxVec3& worldBoxExtents, const PxVec3 worldBoxAxes[3],
	                         const PxVec3& worldDisp, const physx::PxTransform& localToWorldRT, const PxVec3& scale, PxVec3* normal = NULL) const;

	// Returns the min and max dot product of the vertices with the given normal
	void			extent(float& min, float& max, const PxVec3& normal) const;

	void			fill(physx::Array<PxVec3>& outPoints, const physx::PxTransform& localToWorldRT, const PxVec3& scale,
	                     float spacing, float jitter, uint32_t maxPoints, bool adjustSpacing) const;

	void			setEmpty()
	{
		NvParameterized::Handle handle(*mParams);
		mParams->getParameterHandle("vertices", handle);
		mParams->resizeArray(handle, 0);
		mParams->getParameterHandle("uniquePlanes", handle);
		mParams->resizeArray(handle, 0);
		mParams->getParameterHandle("widths", handle);
		mParams->resizeArray(handle, 0);
		mParams->getParameterHandle("edges", handle);
		mParams->resizeArray(handle, 0);
		mParams->bounds.setEmpty();
		mParams->volume = 0.0f;
		mParams->uniqueEdgeDirectionCount = 0;
		mParams->planeCount = 0;
	}

	bool			isEmpty() const
	{
		PX_ASSERT(mParams->bounds.isEmpty() == (mParams->vertices.arraySizes[0] == 0));
		PX_ASSERT(mParams->bounds.isEmpty() == (mParams->uniquePlanes.arraySizes[0] == 0));
		PX_ASSERT(mParams->bounds.isEmpty() == (mParams->widths.arraySizes[0] == 0));
		PX_ASSERT(mParams->bounds.isEmpty() == (mParams->edges.arraySizes[0] == 0));
		PX_ASSERT(mParams->bounds.isEmpty() == (mParams->volume == 0.0f));
		return mParams->bounds.isEmpty();
	}

	uint32_t	getVertexCount() const
	{
		return (uint32_t)mParams->vertices.arraySizes[0];
	}
	const PxVec3&	getVertex(uint32_t index) const
	{
		return mParams->vertices.buf[index];
	}

	uint32_t	getPlaneCount() const
	{
		return mParams->planeCount;
	}
	uint32_t	getUniquePlaneNormalCount() const
	{
		return (uint32_t)mParams->uniquePlanes.arraySizes[0];
	}
	PxPlane	getPlane(uint32_t index) const
	{
		PX_ASSERT(index < getPlaneCount());
		if (index < (uint32_t)mParams->uniquePlanes.arraySizes[0])
		{
			return toPxPlane(mParams->uniquePlanes.buf[index]);
		}
		index -= mParams->uniquePlanes.arraySizes[0];
		PxPlane plane = toPxPlane(mParams->uniquePlanes.buf[index]);
		plane.n = -plane.n;
		plane.d = -plane.d - mParams->widths.buf[index];
		return plane;
	}

	uint32_t	getWidthCount() const
	{
		return (uint32_t)mParams->widths.arraySizes[0];
	}
	float	getWidth(uint32_t index) const
	{
		return mParams->widths.buf[index];
	}

	uint32_t	getEdgeCount() const
	{
		return (uint32_t)mParams->edges.arraySizes[0];
	}
	uint32_t	getEdgeEndpointIndex(uint32_t edgeIndex, uint32_t endpointIndex) const	// endpointIndex = 0 or 1
	{
		PX_ASSERT(edgeIndex < getEdgeCount());
		PX_ASSERT((endpointIndex & 1) == endpointIndex);
		endpointIndex &= 1;
		const uint32_t edge = mParams->edges.buf[edgeIndex];
		return (endpointIndex & 1) ? (edge & 0x0000FFFF) : (edge >> 16);
	}
	uint32_t	getEdgeAdjacentFaceIndex(uint32_t edgeIndex, uint32_t adjacencyIndex) const	// adjacencyIndex = 0 or 1
	{
		PX_ASSERT(edgeIndex < getEdgeCount());
		PX_ASSERT((adjacencyIndex & 1) == adjacencyIndex);
		adjacencyIndex &= 1;
		const uint32_t adj = mParams->adjacentFaces.buf[edgeIndex];
		return (adjacencyIndex & 1) ? (adj & 0x0000FFFF) : (adj >> 16);
	}
	uint32_t	getUniqueEdgeDirectionCount() const
	{
		return mParams->uniqueEdgeDirectionCount;
	}
	PxVec3	getEdgeDirection(uint32_t index) const
	{
		PX_ASSERT(index < getEdgeCount());
		uint32_t edge = mParams->edges.buf[index];
		return mParams->vertices.buf[edge & 0xFFFF] - mParams->vertices.buf[edge >> 16];
	}

	const PxBounds3&	getBounds() const
	{
		return mParams->bounds;
	}
	float			getVolume() const
	{
		return (float)mParams->volume;
	}

	// transform may include an arbitrary 3x3 block and a translation
	void					applyTransformation(const PxMat44& tm)	
	{
		PX_ASSERT(mParams);

		const float det3 = PxMat33(tm.getBasis(0), tm.getBasis(1), tm.getBasis(2)).getDeterminant();
		PX_ASSERT(det3 > 0.0f); // mirroring or degeneracy won't work well here

		// planes and slab widths
		const Cof44 cof(tm);
		const uint32_t numPlanes = (uint32_t)mParams->uniquePlanes.arraySizes[0];
		ConvexHullParametersNS::Plane_Type* planes = mParams->uniquePlanes.buf;
		PX_ASSERT(planes);
		PX_ASSERT(numPlanes == (uint32_t)mParams->widths.arraySizes[0]);
		float* widths = mParams->widths.buf;
		PX_ASSERT(widths);
		for (uint32_t i = 0; i < numPlanes; i++)
		{
			PxPlane src(planes[i].normal, planes[i].d);
			PxPlane dst;
			cof.transform(src, dst);
			planes[i].normal = dst.n;
			planes[i].d = dst.d;
			const float n2 = dst.n.magnitudeSquared();
			if (n2 > 0.0f)
			{
				const float recipN = PxRecipSqrt(n2);
				planes[i].normal *= recipN;
				planes[i].d *= recipN;
				widths[i] *= det3*recipN;
			}
		}

		// vertices
		const uint32_t numVertices = (uint32_t)mParams->vertices.arraySizes[0];
		PxVec3* vertices = mParams->vertices.buf;
		PX_ASSERT(vertices);

		mParams->bounds.setEmpty();
		for (uint32_t i = 0; i < numVertices; i++)
		{
			vertices[i] = tm.transform(vertices[i]);
			mParams->bounds.include(vertices[i]);
		}

		// volume
		mParams->volume *= det3;
	}

	// Special case - transformation must be a pure rotation plus translation, and we only allow a positive, uniform scale
	// Note, we could implement this with applyTransformation(const PxMat44& tm), above, but we will keep this
	// old implementation to ensure that behavior doesn't change
	void					applyTransformation(const PxMat44& transformation, float scale)
	{
		PX_ASSERT(mParams);
		PX_ASSERT(scale > 0.0f); // negative scale won't work well here

		// planes
		const uint32_t numPlanes = (uint32_t)mParams->uniquePlanes.arraySizes[0];
		ConvexHullParametersNS::Plane_Type* planes = mParams->uniquePlanes.buf;
		PX_ASSERT(planes);
		for (uint32_t i = 0; i < numPlanes; i++)
		{
			planes[i].normal = transformation.rotate(planes[i].normal);
			planes[i].d *= scale;
		}

		// slab widths
		const uint32_t numWidths = (uint32_t)mParams->widths.arraySizes[0];
		float* widths = mParams->widths.buf;
		PX_ASSERT(widths);
		for (uint32_t i = 0; i < numWidths; i++)
		{
			widths[i] *= scale;
		}

		// vertices
		const uint32_t numVertices = (uint32_t)mParams->vertices.arraySizes[0];
		PxVec3* vertices = mParams->vertices.buf;
		PX_ASSERT(vertices);

		mParams->bounds.setEmpty();
		for (uint32_t i = 0; i < numVertices; i++)
		{
			vertices[i] = transformation.transform(vertices[i]) * scale;	// Works since scale is uniform
			mParams->bounds.include(vertices[i]);
		}

		// volume
		mParams->volume *= scale*scale*scale;
	}

#if PX_PHYSICS_VERSION_MAJOR == 3
	// Returns the number of vertices and faces of the cooked mesh.  If inflated = false,
	// these should be the same as the values returned by getVertexCount() and getPlaneCount().
	// However, the numerical properties of the cooker could result in different values.  If inflated = true,
	// then sharp edges will be beveled by the cooker, resulting in more vertices and faces.
	// Note: the number of edges E may be calculated from the number of vertices V and faces F using E = V + F - 2.
	// Return value = size in bytes of the cooked convex mesh
	uint32_t calculateCookedSizes(uint32_t& vertexCount, uint32_t& faceCount, bool inflated) const;

	// Removes vertices from the hull until the bounds given in the function's parameters are met.
	// If inflated = true, then the maximum counts given are compared with the cooked hull, which may have higher counts due to beveling.
	// Note: a value of zero indicates no limit, effectively infinite.
	// Return value: true if successful, i.e. the limits were met.  False otherwise.
	bool reduceHull(uint32_t maxVertexCount, uint32_t maxEdgeCount, uint32_t maxFaceCount, bool inflated);

	// Replaces vertices with cooked, un-inflated vertices, if the latter set is smaller.  Returns true if the number of vertices is reduced.
	bool reduceByCooking();
#endif

	// Utility function
	static bool createKDOPDirections(physx::Array<PxVec3>& directions, ConvexHullMethod::Enum method);

//		DeclareArray(PxVec3)	vertices;
//		DeclareArray(PxPlane)	uniquePlanes;	// These are the unique face directions.  If there is an opposite face, the corresponding widths[i] will give its distance
//		physx::Array<float>	widths;			// Same size as uniquePlanes.  Gives width of hull in uniquePlane direction
//		physx::Array<uint32_t>	edges;			// Vertex indices stored in high/low words.  The first uniqueEdgeDirectionCount elements give the unique directions.
//		PxBounds3			bounds;
//		float				volume;
//		uint32_t				uniqueEdgeDirectionCount;
//		uint32_t				planeCount;		// Total number of faces.  Greater than or equal to size of uniquePlanes.

	ConvexHullParameters*		mParams;
	bool						mOwnsParams;
};


/*
ConvexMeshBuilder - creates triangles for a convex hull defined by a set of planes. Copied from physx samples (RenderClothActor)
*/
struct ConvexMeshBuilder
{
	ConvexMeshBuilder(const PxVec4* planes)
		: mPlanes(planes)
	{}

	void operator()(uint32_t mask, float scale=1.0f);

	const PxVec4* mPlanes;
	Array<PxVec3> mVertices;
	Array<uint16_t> mIndices;
};


// Fast implementation for sse
PX_INLINE float	RecipSqrt(float x)
{
#if defined( APEX_SUPPORT_SSE )
	const float three = 3.0f;
	const float oneHalf = 0.5f;
	float y;
	_asm
	{
		movss	xmm2, three;
		rsqrtss	xmm0, x
		movss	xmm1, xmm0
		mulss	xmm1, oneHalf
		mulss	xmm0, xmm0
		mulss	xmm0, x
		subss	xmm2, xmm0
		mulss	xmm1, xmm2
		movss	y, xmm1
	}
	return y;
#else
	return 1.0f / sqrtf(x);
#endif
}

/*
	Array find utility
 */

// If t is found in array, index is set to the array element and the function returns true
// If t is not found in the array, index is not modified and the function returns false
template<class T>
bool arrayFind(uint32_t& index, const T& t, const physx::Array<T>& array)
{
	const uint32_t size = array.size();
	for (uint32_t i = 0; i < size; ++i)
	{
		if (array[i] == t)
		{
			index = i;
			return true;
		}
	}
	return false;
}

#include "ApexUsingNamespace.h"
#if PX_X64
#pragma warning(push)
#pragma warning(disable: 4324) // 'IndexBank<IndexType>' : structure was padded due to __declspec(align())
#endif

/*
	Index bank - double-sided free list for O(1) borrow/return of unique IDs

	Type IndexType should be an unsigned integer type or something that can be cast to and from
	an integer
 */
template <class IndexType>
class IndexBank
{
	public:
	IndexBank<IndexType>(uint32_t capacity = 0) : indexCount(0), capacityLocked(false)
	{
		maxCapacity = calculateMaxCapacity();
		reserve_internal(capacity);
	}

	// Copy constructor
	IndexBank<IndexType>(const IndexBank<IndexType>& other)
	{
		*this = other;
	}

	virtual				~IndexBank<IndexType>() {}

	// Assignment operator
	IndexBank<IndexType>& operator = (const IndexBank<IndexType>& other)
	{
		indices = other.indices;
		ranks = other.ranks;
		maxCapacity = other.maxCapacity;
		indexCount = other.indexCount;
		capacityLocked = other.capacityLocked;
		return *this;
	}

	void				setIndicesAndRanks(uint16_t* indicesIn, uint16_t* ranksIn, uint32_t capacityIn, uint32_t usedCountIn)
	{
		indexCount = usedCountIn;
		reserve_internal(capacityIn);
		for (uint32_t i = 0; i < capacityIn; ++i)
		{
			indices[i] = indicesIn[i];
			ranks[i]   = ranksIn[i];
		}
	}

	void				clear(uint32_t capacity = 0, bool used = false)
	{
		capacityLocked = false;
		indices.reset();
		ranks.reset();
		reserve_internal(capacity);
		if (used)
		{
			indexCount = capacity;
			indices.resize(capacity);
			for (IndexType i = (IndexType)0; i < (IndexType)capacity; ++i)
			{
				indices[i] = i;
			}
		}
		else
		{
			indexCount = 0;
		}
	}

	// Equivalent to calling freeLastUsed() until the used list is empty.
	void				clearFast()
	{
		indexCount = 0;
	}

	// This is the reserve size.  The bank can only grow, due to shuffling of indices
	virtual void		reserve(uint32_t capacity)
	{
		reserve_internal(capacity);
	}

	// If lock = true, keeps bank from automatically resizing
	void				lockCapacity(bool lock)
	{
		capacityLocked = lock;
	}

	bool				isCapacityLocked() const
	{
		return capacityLocked;
	}

	void				setMaxCapacity(uint32_t inMaxCapacity)
	{
		// Cannot drop below current capacity, nor above max set by data types
		maxCapacity = PxClamp(inMaxCapacity, capacity(), calculateMaxCapacity());
	}

	uint32_t		capacity() const
	{
		return indices.size();
	}
	uint32_t		usedCount() const
	{
		return indexCount;
	}
	uint32_t		freeCount() const
	{
		return capacity() - usedCount();
	}

	// valid from [0] to [size()-1]
	const IndexType* 	usedIndices() const
	{
		return indices.begin();
	}

	// valid from [0] to [free()-1]
	const IndexType* 	freeIndices() const
	{
		return indices.begin() + usedCount();
	}

	bool				isValid(IndexType index) const
	{
		return index < (IndexType)capacity();
	}
	bool				isUsed(IndexType index) const
	{
		return isValid(index) && (ranks[index] < (IndexType)usedCount());
	}
	bool				isFree(IndexType index) const
	{
		return isValid(index) && !isUsed();
	}

	IndexType			getRank(IndexType index) const
	{
		return ranks[index];
	}

	// Gets the next available index, if any
	bool				useNextFree(IndexType& index)
	{
		if (freeCount() == 0)
		{
			if (capacityLocked)
			{
				return false;
			}
			if (capacity() >= maxCapacity)
			{
				return false;
			}
			reserve(PxClamp(capacity() * 2, (uint32_t)1, maxCapacity));
			PX_ASSERT(freeCount() > 0);
		}
		index = indices[indexCount++];
		return true;
	}

	// Frees the last used index, if any
	bool				freeLastUsed(IndexType& index)
	{
		if (usedCount() == 0)
		{
			return false;
		}
		index = indices[--indexCount];
		return true;
	}

	// Requests a particular index.  If that index is available, it is borrowed and the function
	// returns true.  Otherwise nothing happens and the function returns false.
	bool				use(IndexType index)
	{
		if (!indexIsValidForUse(index))
		{
			return false;
		}
		IndexType oldRank;
		placeIndexAtRank(index, (IndexType)indexCount++, oldRank);
		return true;
	}

	bool				free(IndexType index)
	{
		if (!indexIsValidForFreeing(index))
		{
			return false;
		}
		IndexType oldRank;
		placeIndexAtRank(index, (IndexType)--indexCount, oldRank);
		return true;
	}

	bool				useAndReturnRanks(IndexType index, IndexType& newRank, IndexType& oldRank)
	{
		if (!indexIsValidForUse(index))
		{
			return false;
		}
		newRank = (IndexType)indexCount++;
		placeIndexAtRank(index, newRank, oldRank);
		return true;
	}

	bool				freeAndReturnRanks(IndexType index, IndexType& newRank, IndexType& oldRank)
	{
		if (!indexIsValidForFreeing(index))
		{
			return false;
		}
		newRank = (IndexType)--indexCount;
		placeIndexAtRank(index, newRank, oldRank);
		return true;
	}

	protected:

	bool				indexIsValidForUse(IndexType index)
	{
		if (!isValid(index))
		{
			if (capacityLocked)
			{
				return false;
			}
			if (capacity() >= maxCapacity)
			{
				return false;
			}
			reserve(PxClamp(2*(uint32_t)index, (uint32_t)1, maxCapacity));
			PX_ASSERT(isValid(index));
		}
		return !isUsed(index);
	}

	bool				indexIsValidForFreeing(IndexType index)
	{
		if (!isValid(index))
		{
			// Invalid index
			return false;
		}
		return isUsed(index);
	}

	// This is the reserve size.  The bank can only grow, due to shuffling of indices
	void				reserve_internal(uint32_t capacity)
	{
		capacity = PxMin(capacity, maxCapacity);
		const uint32_t oldCapacity = indices.size();
		if (capacity > oldCapacity)
		{
			indices.resize(capacity);
			ranks.resize(capacity);
			for (IndexType i = (IndexType)oldCapacity; i < (IndexType)capacity; ++i)
			{
				indices[i] = i;
				ranks[i] = i;
			}
		}
	}

	private:

	void				placeIndexAtRank(IndexType index, IndexType newRank, IndexType& oldRank)	// returns old rank
	{
		const IndexType replacementIndex = indices[newRank];
		oldRank = ranks[index];
		indices[oldRank] = replacementIndex;
		indices[newRank] = index;
		ranks[replacementIndex] = oldRank;
		ranks[index] = newRank;
	}

	uint32_t				calculateMaxCapacity()
	{
#pragma warning(push)
#pragma warning(disable: 4127) // conditional expression is constant
		if (sizeof(IndexType) >= sizeof(uint32_t))
		{
			return 0xFFFFFFFF;	// Limited by data type we use to report capacity
		}
		else
		{
			return (1u << (8 * PxMin((uint32_t)sizeof(IndexType), 3u))) - 1;	// Limited by data type we use for indices
		}
#pragma warning(pop)
	}

	protected:

	Array<IndexType>		indices;
	Array<IndexType>		ranks;
	uint32_t					maxCapacity;
	uint32_t					indexCount;
	bool					capacityLocked;
};

#if PX_X64
#pragma warning(pop)
#endif

/*
	Bank - Index bank of type IndexType with an associated object array of type T
 */
template <class T, class IndexType>
class Bank : public IndexBank<IndexType>
{
	public:
	Bank<T, IndexType>(uint32_t capacity = 0) : IndexBank<IndexType>(capacity)
	{
		objects = (T*)PX_ALLOC(IndexBank<IndexType>::indices.size() * sizeof(T), PX_DEBUG_EXP("Bank"));
		if (objects != NULL)
		{
			PX_ASSERT(memset(objects, 0, IndexBank<IndexType>::indices.size() * sizeof(T)));
		}
	}
	Bank<T, IndexType>(const Bank<T, IndexType>& bank) : objects(NULL)
	{
		*this = bank;
	}

	~Bank<T, IndexType>()
	{
		clear();
	}

	Bank<T, IndexType>&	operator = (const Bank<T, IndexType>& bank)
	{
		if (&bank == this)
		{
			return *this;
		}

		this->clear();

		this->indices = bank.indices;
		this->ranks = bank.ranks;
		this->maxCapacity = bank.maxCapacity;
		this->indexCount = bank.indexCount;
		this->capacityLocked = bank.capacityLocked;

		if (this->indices.size())
		{
			objects = (T*)PX_ALLOC(IndexBank<IndexType>::indices.size() * sizeof(T), PX_DEBUG_EXP("Bank"));
			PX_ASSERT(memset(objects, 0, IndexBank<IndexType>::indices.size() * sizeof(T)));
			for (uint32_t i = 0; i < this->indexCount; ++i)
			{
				uint32_t index = this->indices[i];
				new(objects + index) T();
				objects[index] = bank.objects[index];
			}
		}
		return *this;
	}

	// This is the reserve size.  The bank can only grow, due to shuffling of indices
	virtual	void	reserve(uint32_t capacity)
	{
		const uint32_t oldSize = IndexBank<IndexType>::indices.size();
		IndexBank<IndexType>::reserve_internal(capacity);
		if (IndexBank<IndexType>::indices.size() > oldSize)
		{
			T* nb = (T*)PX_ALLOC(IndexBank<IndexType>::indices.size() * sizeof(T), PX_DEBUG_EXP("Bank"));
			if (nb)
			{
				PX_ASSERT(memset(nb, 0, IndexBank<IndexType>::indices.size() * sizeof(T)));

				const IndexType* usedIndices = IndexBank<IndexType>::usedIndices();
				uint32_t numIndices = IndexBank<IndexType>::usedCount();

				// this copy needs to be correct for nonPOD type T's
				for (int32_t i = (int32_t)numIndices - 1; i >= 0; i--)
				{
					IndexType index = usedIndices[i];
					new(nb + index) T(objects[index]);
					objects[index].~T();
				}
				//memcpy( nb, objects, IndexBank<IndexType>::indices.size()*sizeof(T) );
			}
			PX_FREE(objects);
			objects = nb;
		}
	}

	// Indirect array accessors: rank in [0,usedCount()-1] returns all "used" indexed objects
	const T& 		getUsed(IndexType rank) const
	{
		return objects[ IndexBank<IndexType>::indices[rank] ];
	}
	T& 				getUsed(IndexType rank)
	{
		return objects[ IndexBank<IndexType>::indices[rank] ];
	}

	// Direct array accessors
	const T& 		direct(IndexType index) const
	{
		return objects[index];
	}
	T& 				direct(IndexType index)
	{
		return objects[index];
	}

	// Wrappers for base class, which call appropriate constructors and destructors of objects
	bool			useNextFree(IndexType& index)
	{
		if (IndexBank<IndexType>::useNextFree(index))
		{
			new(objects + index) T();
			return true;
		}
		return false;
	}

	bool			freeLastUsed(IndexType& index)
	{
		if (IndexBank<IndexType>::freeLastUsed(index))
		{
			objects[index].~T();
			return true;
		}
		return false;
	}

	bool			use(IndexType index)
	{
		if (IndexBank<IndexType>::use(index))
		{
			new(objects + index) T();
			return true;
		}
		return false;
	}

	bool			free(IndexType index)
	{
		if (IndexBank<IndexType>::free(index))
		{
			objects[index].~T();
			return true;
		}
		return false;
	}

	bool			useAndReturnRanks(IndexType index, IndexType& newRank, IndexType& oldRank)
	{
		if (IndexBank<IndexType>::useAndReturnRanks(index,  newRank,  oldRank))
		{
			new(objects + index) T();
			return true;
		}
		return false;
	}

	bool			freeAndReturnRanks(IndexType index, IndexType& newRank, IndexType& oldRank)
	{
		if (IndexBank<IndexType>::freeAndReturnRanks(index,  newRank,  oldRank))
		{
			objects[index].~T();
			return true;
		}
		return false;
	}

	// Erases all object, index, and rank arrays (complete deallocation)
	void			clear()
	{
		const IndexType* usedIndices = IndexBank<IndexType>::usedIndices();
		uint32_t numIndices = IndexBank<IndexType>::usedCount();

		for (int32_t i = (int32_t)numIndices - 1; i >= 0; i--)
		{
			bool test = free(usedIndices[i]);
			PX_UNUSED(test);
			PX_ASSERT(test);
		}

		IndexBank<IndexType>::clear();
		PX_FREE(objects);
		objects = NULL;
	}

	// Re-arranges objects internally into rank-order, afterwards rank = index
	void			clean()
	{
		for (IndexType i = 0; i < IndexBank<IndexType>::capacity(); ++i)
		{
			const IndexType index = IndexBank<IndexType>::indices[i];
			if (index != i)
			{
				nvidia::swap(objects[i], objects[index]);
				const IndexType displacedRank = IndexBank<IndexType>::ranks[i];
				IndexBank<IndexType>::indices[i] = i;
				IndexBank<IndexType>::ranks[i] = i;
				IndexBank<IndexType>::indices[displacedRank] = index;
				IndexBank<IndexType>::ranks[index] = displacedRank;
			}
		}
	}

	protected:
	T*	objects;
};


/*
	Ring buffer
*/
template <class T>
class RingBuffer
{
	public:
	RingBuffer() : frontIndex(0), backIndex(0xFFFFFFFF), usedSize(0), bufferSize(0), buffer(NULL) {}
	~RingBuffer()
	{
		erase();
	}

	uint32_t	size()	const
	{
		return usedSize;
	}

	T&		operator [](uint32_t i)
	{
		PX_ASSERT(i < usedSize);
		i += frontIndex;
		return buffer[ i < bufferSize ? i : i - bufferSize ];
	}

	const T&	operator [](uint32_t i) const
	{
		return (const T&)(const_cast<RingBuffer<T>*>(this)->operator[](i));
	}

	T&		back()	const
	{
		return buffer[backIndex];
	}
	T&		front()	const
	{
		return buffer[frontIndex];
	}

	T&		pushBack()
	{
		if (bufferSize == usedSize)
		{
			reserve(2 * (bufferSize + 1));
		}
		++usedSize;
		if (++backIndex == bufferSize)
		{
			backIndex = 0;
		}
		T& back = buffer[backIndex];
		PX_PLACEMENT_NEW(&back, T)();
		return back;
	}

	void	popBack()
	{
		PX_ASSERT(size() != 0);
		if (size() == 0)
		{
			return;
		}
		buffer[backIndex].~T();
		--usedSize;
		if (backIndex-- == 0)
		{
			backIndex += bufferSize;
		}
	}

	T&		pushFront()
	{
		if (bufferSize == usedSize)
		{
			reserve(2 * (bufferSize + 1));
		}
		++usedSize;
		if (frontIndex-- == 0)
		{
			frontIndex += bufferSize;
		}
		T& front = buffer[frontIndex];
		PX_PLACEMENT_NEW(&front, T)();
		return front;
	}

	void	popFront()
	{
		PX_ASSERT(size() != 0);
		if (size() == 0)
		{
			return;
		}
		buffer[frontIndex].~T();
		--usedSize;
		if (++frontIndex == bufferSize)
		{
			frontIndex = 0;
		}
	}

	void	clear()
	{
		while (size() != 0)
		{
			popBack();
		}
		frontIndex = 0;
		backIndex = 0xFFFFFFFF;
	}

	void	erase()
	{
		clear();
		if (buffer != NULL)
		{
			PX_FREE(buffer);
			buffer = NULL;
		}
		bufferSize = 0;
	}

	void	reserve(uint32_t newBufferSize)
	{
		if (newBufferSize <= bufferSize)
		{
			return;
		}
		T* newBuffer = (T*)PX_ALLOC(newBufferSize * sizeof(T), PX_DEBUG_EXP("RingBuffer"));
		const uint32_t lastIndex = frontIndex + usedSize;
		if (lastIndex <= bufferSize)
		{
			for (uint32_t i = 0; i < usedSize; i++)
			{
				PX_PLACEMENT_NEW(newBuffer + i, T)(buffer[i]);
				buffer[i].~T();
			}
			//memcpy( newBuffer, buffer+frontIndex, usedSize*sizeof( T ) );
		}
		else
		{
			for (uint32_t i = 0; i < (bufferSize - frontIndex); i++)
			{
				PX_PLACEMENT_NEW(newBuffer + i, T)(buffer[i + frontIndex]);
				buffer[i + frontIndex].~T();
			}
			//memcpy( newBuffer, buffer+frontIndex, (bufferSize-frontIndex)*sizeof( T ) );

			for (uint32_t i = 0; i < (lastIndex - bufferSize); i++)
			{
				PX_PLACEMENT_NEW(newBuffer + i + (bufferSize - frontIndex), T)(buffer[i]);
				buffer[i].~T();
			}
			//memcpy( newBuffer + (bufferSize-frontIndex), buffer, (lastIndex-bufferSize)*sizeof( T ) );
		}
		bufferSize = newBufferSize;
		frontIndex = 0;
		backIndex = frontIndex + usedSize - 1;
		if (buffer)
		{
			PX_FREE(buffer);
		}
		buffer = newBuffer;
	}

	class It
	{
		public:
		It(const RingBuffer<T>& buffer) :
		m_bufferStart(buffer.buffer), m_bufferStop(buffer.buffer + buffer.bufferSize),
		m_current(buffer.usedSize > 0 ? buffer.buffer + buffer.frontIndex : NULL), m_remaining(buffer.usedSize) {}

		operator T* ()	const
		{
			return m_current;
		}
		T*		operator ++ ()
		{
			inc();
			return m_current;
		}
		T*		operator ++ (int)
		{
			T* prev = m_current;
			inc();
			return prev;
		}

		private:
		void	inc()
		{
			if (m_remaining > 1)
			{
				--m_remaining;
				if (++m_current == m_bufferStop)
				{
					m_current = m_bufferStart;
				}
			}
			else
			{
				m_remaining = 0;
				m_current = NULL;
			}
		}

		T*		m_bufferStart;
		T*		m_bufferStop;
		T*		m_current;
		uint32_t	m_remaining;
	};

	friend class It;

	protected:
	uint32_t	frontIndex;
	uint32_t	backIndex;
	uint32_t	usedSize;
	uint32_t	bufferSize;
	T*		buffer;
};


template<class T>
class Pool
{
	enum { DefaultBlockSizeInBytes = 1024 };	// This must be positive

	public:
	Pool(uint32_t objectsPerBlock = 0) : m_head(NULL), m_inUse(0)
	{
		PX_ASSERT(sizeof(T) >= sizeof(void*));
		setBlockSize(objectsPerBlock);
	}

	~Pool()
	{
		empty();
	}

	void	setBlockSize(uint32_t objectsPerBlock)
	{
		m_objectsPerBlock = objectsPerBlock > 0 ? objectsPerBlock : ((uint32_t)DefaultBlockSizeInBytes + sizeof(T) - 1) / sizeof(T);
	}

	/* Gives a single object, allocating if necessary */
	T*		borrow()
	{
		if (m_head == NULL)
		{
			allocateBlock();
		}
		T* ptr = (T*)m_head;
		m_head = *(void**)m_head;
		new(ptr) T();
		++m_inUse;
		return ptr;
	}

	/* Return a single object */
	void	replace(T* ptr)
	{
		if (ptr != NULL)
		{
			ptr->~T();
			*(void**)ptr = m_head;
			m_head = (void*)ptr;
			--m_inUse;
		}
	}

	void	allocateBlock()
	{
		T* block = (T*)PX_ALLOC(sizeof(T) * m_objectsPerBlock, PX_DEBUG_EXP("ApexSharedUtils::Pool"));
		m_blocks.pushBack(block);
		for (T* ptr = block + m_objectsPerBlock; ptr-- != block;)
		{
			*(void**)ptr = m_head;
			m_head = (void*)ptr;
		}
	}

	int32_t	empty()
	{
		while (m_blocks.size())
		{
			PX_FREE(m_blocks.back());
			m_blocks.popBack();
		}
		m_blocks.reset();
		m_head = NULL;
		const int32_t inUse = m_inUse;
		m_inUse = 0;
		return inUse;
	}

	protected:

	void*			m_head;
	uint32_t	m_objectsPerBlock;
	physx::Array<T*>m_blocks;
	int32_t	m_inUse;
};


// Progress listener implementation for hierarchical progress reporting
class HierarchicalProgressListener : public IProgressListener
{
	public:
	HierarchicalProgressListener(int totalWork, IProgressListener* parent) :
	m_work(0), m_subtaskWork(1), m_totalWork(PxMax(totalWork, 1)), m_taskName(NULL), m_parent(parent) {}

	void	setSubtaskWork(int subtaskWork, const char* taskName = NULL)
	{
		if (subtaskWork < 0)
		{
			subtaskWork = m_totalWork - m_work;
		}

		m_subtaskWork = subtaskWork;
		PX_ASSERT(m_work + m_subtaskWork <= m_totalWork);
		m_taskName = taskName;
		setProgress(0, m_taskName);
	}

	void	completeSubtask()
	{
		setProgress(100, m_taskName);
		m_work += m_subtaskWork;
	}

	void	setProgress(int progress, const char* taskName = NULL)
	{
		PX_ASSERT(progress >= 0);
		PX_ASSERT(progress <= 100);

		if (taskName == NULL)
		{
			taskName = m_taskName;
		}

		if (m_parent != NULL)
		{
			const int parentProgress = m_totalWork > 0 ? (m_work * 100 + m_subtaskWork * progress) / m_totalWork : 100;
			m_parent->setProgress(PxClamp(parentProgress, 0, 100), taskName);
		}
	}

	protected:
	int m_work;
	int m_subtaskWork;
	int m_totalWork;
	const char* m_taskName;
	IProgressListener* m_parent;
};

void createIndexStartLookup(physx::Array<uint32_t>& lookup, int32_t indexBase, uint32_t indexRange, int32_t* indexSource, uint32_t indexCount, uint32_t indexByteStride);

void findIslands(physx::Array< physx::Array<uint32_t> >& islands, const physx::Array<IntPair>& overlaps, uint32_t indexRange);

// Neighbor-finding utility class
class NeighborLookup
{
public:
	void				setBounds(const BoundsRep* bounds, uint32_t boundsCount, uint32_t boundsByteStride);

	uint32_t		getNeighborCount(const uint32_t index) const;
	const uint32_t*	getNeighbors(const uint32_t index) const;

protected:
	physx::Array<uint32_t>	m_neighbors;
	physx::Array<uint32_t>	m_firstNeighbor;
};


// TriangleFrame - calculates interpolation data for triangle quantities
class TriangleFrame
{
	public:

	enum VertexField
	{
//		Position_x,	Position_y, Position_z,	// Not interpolating positions
		Normal_x, Normal_y, Normal_z,
		Tangent_x, Tangent_y, Tangent_z,
		Binormal_x, Binormal_y, Binormal_z,
		UV0_u, UV0_v, UV1_u, UV1_v, UV2_u, UV2_v, UV3_u, UV3_v,
		Color_r, Color_g, Color_b, Color_a,

		VertexFieldCount
	};

	TriangleFrame() : m_fieldMask(0)																							{}
	TriangleFrame(const ExplicitRenderTriangle& tri, uint64_t fieldMask = 0xFFFFFFFFFFFFFFFFULL)
	{
		setFromTriangle(tri, fieldMask);
	}
	TriangleFrame(const PxMat44& tm, const PxVec2& uvScale, const PxVec2& uvOffset, uint64_t fieldMask = 0xFFFFFFFFFFFFFFFFULL)
	{
		setFlat(tm, uvScale, uvOffset, fieldMask);
	}

	PX_INLINE void	setFromTriangle(const ExplicitRenderTriangle& tri, uint64_t fieldMask = 0xFFFFFFFFFFFFFFFFULL);
	PX_INLINE void	setFlat(const PxMat44& tm, const PxVec2& uvScale, const PxVec2& uvOffset, uint64_t fieldMask = 0xFFFFFFFFFFFFFFFFULL);

	PX_INLINE void	interpolateVertexData(Vertex& vertex) const;

	private:

	static size_t	s_offsets[VertexFieldCount];
	PxPlane	m_frames[VertexFieldCount];
	uint64_t	m_fieldMask;

	friend class TriangleFrameBuilder;
};

PX_INLINE void
TriangleFrame::setFromTriangle(const ExplicitRenderTriangle& tri, uint64_t fieldMask)
{
	m_fieldMask = fieldMask;

	PxVec3 p0, p1, p2;
	p0 = tri.vertices[0].position;
	p1 = tri.vertices[1].position;
	p2 = tri.vertices[2].position;
	const PxVec3 p1xp2 = p1.cross(p2);
	const PxVec3 p2xp0 = p2.cross(p0);
	const PxVec3 p0xp1 = p0.cross(p1);
	const PxVec3 n = p1xp2 + p2xp0 + p0xp1;
	const float n2 = n.dot(n);
	if (n2 < PX_EPS_F32 * PX_EPS_F32)
	{
		for (uint32_t i = 0; fieldMask != 0 && i < VertexFieldCount; ++i, (fieldMask >>= 1))
		{
			if (fieldMask & 1)
			{
				m_frames[i] = PxPlane(0, 0, 0, 0);
			}
		}
		return;
	}

	// Calculate inverse 4x4 matrix (only need first three columns):
	const PxVec3 nP = n / n2;	// determinant is -n2
	const PxVec3 Q0(nP.z * (p1.y - p2.y) - nP.y * (p1.z - p2.z), nP.z * (p2.y - p0.y) - nP.y * (p2.z - p0.z), nP.z * (p0.y - p1.y) - nP.y * (p0.z - p1.z));
	const PxVec3 Q1(nP.x * (p1.z - p2.z) - nP.z * (p1.x - p2.x), nP.x * (p2.z - p0.z) - nP.z * (p2.x - p0.x), nP.x * (p0.z - p1.z) - nP.z * (p0.x - p1.x));
	const PxVec3 Q2(nP.y * (p1.x - p2.x) - nP.x * (p1.y - p2.y), nP.y * (p2.x - p0.x) - nP.x * (p2.y - p0.y), nP.y * (p0.x - p1.x) - nP.x * (p0.y - p1.y));
	const PxVec3 r(nP.dot(p1xp2), nP.dot(p2xp0), nP.dot(p0xp1));

	for (uint32_t i = 0; fieldMask != 0 && i < VertexFieldCount; ++i, (fieldMask >>= 1))
	{
		if (fieldMask & 1)
		{
			const size_t offset = s_offsets[i];
			const PxVec3 vi(*(float*)(((uint8_t*)&tri.vertices[0]) + offset), *(float*)(((uint8_t*)&tri.vertices[1]) + offset), *(float*)(((uint8_t*)&tri.vertices[2]) + offset));
			m_frames[i] = PxPlane(Q0.dot(vi), Q1.dot(vi), Q2.dot(vi), r.dot(vi));
		}
	}
}

PX_INLINE void
TriangleFrame::setFlat(const PxMat44& tm, const PxVec2& uvScale, const PxVec2& uvOffset, uint64_t fieldMask)
{
	m_fieldMask = fieldMask;

	// Local z ~ normal = tangents[2], x ~ u and tangent = tangents[0], y ~ v and binormal = tangents[1]
	if ((fieldMask >> Normal_x) & 1)
	{
		m_frames[Normal_x] = PxPlane(PxVec3((float)0), tm(0, 2));
	}
	if ((fieldMask >> Normal_y) & 1)
	{
		m_frames[Normal_y] = PxPlane(PxVec3((float)0), tm(1, 2));
	}
	if ((fieldMask >> Normal_z) & 1)
	{
		m_frames[Normal_z] = PxPlane(PxVec3((float)0), tm(2, 2));
	}
	if ((fieldMask >> Tangent_x) & 1)
	{
		m_frames[Tangent_x] = PxPlane(PxVec3((float)0), tm(0, 0));
	}
	if ((fieldMask >> Tangent_y) & 1)
	{
		m_frames[Tangent_y] = PxPlane(PxVec3((float)0), tm(1, 0));
	}
	if ((fieldMask >> Tangent_z) & 1)
	{
		m_frames[Tangent_z] = PxPlane(PxVec3((float)0), tm(2, 0));
	}
	if ((fieldMask >> Binormal_x) & 1)
	{
		m_frames[Binormal_x] = PxPlane(PxVec3((float)0), tm(0, 1));
	}
	if ((fieldMask >> Binormal_y) & 1)
	{
		m_frames[Binormal_y] = PxPlane(PxVec3((float)0), tm(1, 1));
	}
	if ((fieldMask >> Binormal_z) & 1)
	{
		m_frames[Binormal_z] = PxPlane(PxVec3((float)0), tm(2, 1));
	}
	const PxVec3 psu = (uvScale[0] ? 1 / uvScale[0] : (float)0) * tm.column0.getXYZ();
	const PxVec3 psv = (uvScale[1] ? 1 / uvScale[1] : (float)0) * tm.column1.getXYZ();
	if ((fieldMask >> UV0_u) & 1)
	{
		m_frames[UV0_u] = PxPlane(psu, uvOffset[0]);
	}
	if ((fieldMask >> UV0_v) & 1)
	{
		m_frames[UV0_v] = PxPlane(psv, uvOffset[1]);
	}
	if ((fieldMask >> UV1_u) & 1)
	{
		m_frames[UV1_u] = PxPlane(psu, uvOffset[0]);
	}
	if ((fieldMask >> UV1_v) & 1)
	{
		m_frames[UV1_v] = PxPlane(psv, uvOffset[1]);
	}
	if ((fieldMask >> UV2_u) & 1)
	{
		m_frames[UV2_u] = PxPlane(psu, uvOffset[0]);
	}
	if ((fieldMask >> UV2_v) & 1)
	{
		m_frames[UV2_v] = PxPlane(psv, uvOffset[1]);
	}
	if ((fieldMask >> UV3_u) & 1)
	{
		m_frames[UV3_u] = PxPlane(psu, uvOffset[0]);
	}
	if ((fieldMask >> UV3_v) & 1)
	{
		m_frames[UV3_v] = PxPlane(psv, uvOffset[1]);
	}
	if ((fieldMask >> Color_r) & 1)
	{
		m_frames[Color_r] = PxPlane(PxVec3((float)0), (float)1);
	}
	if ((fieldMask >> Color_g) & 1)
	{
		m_frames[Color_g] = PxPlane(PxVec3((float)0), (float)1);
	}
	if ((fieldMask >> Color_b) & 1)
	{
		m_frames[Color_b] = PxPlane(PxVec3((float)0), (float)1);
	}
	if ((fieldMask >> Color_a) & 1)
	{
		m_frames[Color_a] = PxPlane(PxVec3((float)0), (float)1);
	}
}

PX_INLINE void
TriangleFrame::interpolateVertexData(Vertex& vertex) const
{
	uint64_t fieldMask = m_fieldMask;
	for (uint32_t i = 0; fieldMask != 0 && i < VertexFieldCount; ++i, (fieldMask >>= 1))
	{
		if (fieldMask & 1)
		{
			float& value = *(float*)(((uint8_t*)&vertex) + s_offsets[i]);
			value = m_frames[i].distance(vertex.position);
		}
	}
}

class TriangleFrameBuilder
{
	public:
	TriangleFrameBuilder()
	{
#define CREATE_TF_OFFSET( field, element )				(size_t)((uintptr_t)&vertex.field.element-(uintptr_t)&vertex)
#define CREATE_TF_OFFSET_IDX( field, element, index )	(size_t)((uintptr_t)&vertex.field[index].element-(uintptr_t)&vertex)

		Vertex vertex;
//		TriangleFrame::s_offsets[TriangleFrame::Position_x] =	CREATE_TF_OFFSET( position, x );
//		TriangleFrame::s_offsets[TriangleFrame::Position_y] =	CREATE_TF_OFFSET( position, y );
//		TriangleFrame::s_offsets[TriangleFrame::Position_z] =	CREATE_TF_OFFSET( position, z );
		TriangleFrame::s_offsets[TriangleFrame::Normal_x] =		CREATE_TF_OFFSET(normal, x);
		TriangleFrame::s_offsets[TriangleFrame::Normal_y] =		CREATE_TF_OFFSET(normal, y);
		TriangleFrame::s_offsets[TriangleFrame::Normal_z] =		CREATE_TF_OFFSET(normal, z);
		TriangleFrame::s_offsets[TriangleFrame::Tangent_x] =	CREATE_TF_OFFSET(tangent, x);
		TriangleFrame::s_offsets[TriangleFrame::Tangent_y] =	CREATE_TF_OFFSET(tangent, y);
		TriangleFrame::s_offsets[TriangleFrame::Tangent_z] =	CREATE_TF_OFFSET(tangent, z);
		TriangleFrame::s_offsets[TriangleFrame::Binormal_x] =	CREATE_TF_OFFSET(binormal, x);
		TriangleFrame::s_offsets[TriangleFrame::Binormal_y] =	CREATE_TF_OFFSET(binormal, y);
		TriangleFrame::s_offsets[TriangleFrame::Binormal_z] =	CREATE_TF_OFFSET(binormal, z);
		TriangleFrame::s_offsets[TriangleFrame::UV0_u] =		CREATE_TF_OFFSET_IDX(uv, u, 0);
		TriangleFrame::s_offsets[TriangleFrame::UV0_v] =		CREATE_TF_OFFSET_IDX(uv, v, 0);
		TriangleFrame::s_offsets[TriangleFrame::UV1_u] =		CREATE_TF_OFFSET_IDX(uv, u, 1);
		TriangleFrame::s_offsets[TriangleFrame::UV1_v] =		CREATE_TF_OFFSET_IDX(uv, v, 1);
		TriangleFrame::s_offsets[TriangleFrame::UV2_u] =		CREATE_TF_OFFSET_IDX(uv, u, 2);
		TriangleFrame::s_offsets[TriangleFrame::UV2_v] =		CREATE_TF_OFFSET_IDX(uv, v, 2);
		TriangleFrame::s_offsets[TriangleFrame::UV3_u] =		CREATE_TF_OFFSET_IDX(uv, u, 3);
		TriangleFrame::s_offsets[TriangleFrame::UV3_v] =		CREATE_TF_OFFSET_IDX(uv, v, 3);
		TriangleFrame::s_offsets[TriangleFrame::Color_r] =		CREATE_TF_OFFSET(color, r);
		TriangleFrame::s_offsets[TriangleFrame::Color_g] =		CREATE_TF_OFFSET(color, g);
		TriangleFrame::s_offsets[TriangleFrame::Color_b] =		CREATE_TF_OFFSET(color, b);
		TriangleFrame::s_offsets[TriangleFrame::Color_a] =		CREATE_TF_OFFSET(color, a);
	}
};


// Format conversion utilities

// Explicit data layouts, used for data conversion

typedef uint8_t UBYTE1_TYPE;
typedef uint8_t UBYTE2_TYPE[2];
typedef uint8_t UBYTE3_TYPE[3];
typedef uint8_t UBYTE4_TYPE[4];

typedef uint16_t USHORT1_TYPE;
typedef uint16_t USHORT2_TYPE[2];
typedef uint16_t USHORT3_TYPE[3];
typedef uint16_t USHORT4_TYPE[4];

typedef int16_t SHORT1_TYPE;
typedef int16_t SHORT2_TYPE[2];
typedef int16_t SHORT3_TYPE[3];
typedef int16_t SHORT4_TYPE[4];

typedef uint32_t UINT1_TYPE;
typedef uint32_t UINT2_TYPE[2];
typedef uint32_t UINT3_TYPE[3];
typedef uint32_t UINT4_TYPE[4];

struct R8G8B8A8_TYPE
{
	uint8_t r, g, b, a;
};
struct B8G8R8A8_TYPE
{
	uint8_t b, g, r, a;
};
struct R32G32B32A32_FLOAT_TYPE
{
	float r, g, b, a;
};
struct B32G32R32A32_FLOAT_TYPE
{
	float b, g, r, a;
};

typedef uint8_t BYTE_UNORM1_TYPE;
typedef uint8_t BYTE_UNORM2_TYPE[2];
typedef uint8_t BYTE_UNORM3_TYPE[3];
typedef uint8_t BYTE_UNORM4_TYPE[4];

typedef uint16_t SHORT_UNORM1_TYPE;
typedef uint16_t SHORT_UNORM2_TYPE[2];
typedef uint16_t SHORT_UNORM3_TYPE[3];
typedef uint16_t SHORT_UNORM4_TYPE[4];

typedef int8_t BYTE_SNORM1_TYPE;
typedef int8_t BYTE_SNORM2_TYPE[2];
typedef int8_t BYTE_SNORM3_TYPE[3];
typedef int8_t BYTE_SNORM4_TYPE[4];

typedef int16_t SHORT_SNORM1_TYPE;
typedef int16_t SHORT_SNORM2_TYPE[2];
typedef int16_t SHORT_SNORM3_TYPE[3];
typedef int16_t SHORT_SNORM4_TYPE[4];

typedef uint16_t HALF1_TYPE;
typedef uint16_t HALF2_TYPE[2];
typedef uint16_t HALF3_TYPE[3];
typedef uint16_t HALF4_TYPE[4];

typedef float FLOAT1_TYPE;
typedef float FLOAT2_TYPE[2];
typedef float FLOAT3_TYPE[3];
typedef float FLOAT4_TYPE[4];

typedef PxMat44			FLOAT4x4_TYPE;
typedef PxMat33			FLOAT3x3_TYPE;

typedef PxQuat	FLOAT4_QUAT_TYPE;
typedef int8_t		BYTE_SNORM4_QUATXYZW_TYPE[4];
typedef int16_t	SHORT_SNORM4_QUATXYZW_TYPE[4];


// Data converters

// USHORT1_TYPE -> UINT1_TYPE
PX_INLINE void convert_UINT1_from_USHORT1(UINT1_TYPE& dst, const USHORT1_TYPE& src)
{
	dst = (uint32_t)src;
}

// USHORT2_TYPE -> UINT2_TYPE
PX_INLINE void convert_UINT2_from_USHORT2(UINT2_TYPE& dst, const USHORT2_TYPE& src)
{
	convert_UINT1_from_USHORT1(dst[0], src[0]);
	convert_UINT1_from_USHORT1(dst[1], src[1]);
}

// USHORT3_TYPE -> UINT3_TYPE
PX_INLINE void convert_UINT3_from_USHORT3(UINT3_TYPE& dst, const USHORT3_TYPE& src)
{
	convert_UINT1_from_USHORT1(dst[0], src[0]);
	convert_UINT1_from_USHORT1(dst[1], src[1]);
	convert_UINT1_from_USHORT1(dst[2], src[2]);
}

// USHORT4_TYPE -> UINT4_TYPE
PX_INLINE void convert_UINT4_from_USHORT4(UINT4_TYPE& dst, const USHORT4_TYPE& src)
{
	convert_UINT1_from_USHORT1(dst[0], src[0]);
	convert_UINT1_from_USHORT1(dst[1], src[1]);
	convert_UINT1_from_USHORT1(dst[2], src[2]);
	convert_UINT1_from_USHORT1(dst[3], src[3]);
}

// UINT1_TYPE -> USHORT1_TYPE
PX_INLINE void convert_USHORT1_from_UINT1(USHORT1_TYPE& dst, const UINT1_TYPE& src)
{
	dst = (uint16_t)src;
}

// UINT2_TYPE -> USHORT2_TYPE
PX_INLINE void convert_USHORT2_from_UINT2(USHORT2_TYPE& dst, const UINT2_TYPE& src)
{
	convert_USHORT1_from_UINT1(dst[0], src[0]);
	convert_USHORT1_from_UINT1(dst[1], src[1]);
}

// UINT3_TYPE -> USHORT3_TYPE
PX_INLINE void convert_USHORT3_from_UINT3(USHORT3_TYPE& dst, const UINT3_TYPE& src)
{
	convert_USHORT1_from_UINT1(dst[0], src[0]);
	convert_USHORT1_from_UINT1(dst[1], src[1]);
	convert_USHORT1_from_UINT1(dst[2], src[2]);
}

// UINT4_TYPE -> USHORT4_TYPE
PX_INLINE void convert_USHORT4_from_UINT4(USHORT4_TYPE& dst, const UINT4_TYPE& src)
{
	convert_USHORT1_from_UINT1(dst[0], src[0]);
	convert_USHORT1_from_UINT1(dst[1], src[1]);
	convert_USHORT1_from_UINT1(dst[2], src[2]);
	convert_USHORT1_from_UINT1(dst[3], src[3]);
}

// BYTE_SNORM1_TYPE -> FLOAT1_TYPE
PX_INLINE void convert_FLOAT1_from_BYTE_SNORM1(FLOAT1_TYPE& dst, const BYTE_SNORM1_TYPE& src)
{
	dst = (float)src / 127.0f;
}

// BYTE_SNORM2_TYPE -> FLOAT2_TYPE
PX_INLINE void convert_FLOAT2_from_BYTE_SNORM2(FLOAT2_TYPE& dst, const BYTE_SNORM2_TYPE& src)
{
	convert_FLOAT1_from_BYTE_SNORM1(dst[0], src[0]);
	convert_FLOAT1_from_BYTE_SNORM1(dst[1], src[1]);
}

// BYTE_SNORM3_TYPE -> FLOAT3_TYPE
PX_INLINE void convert_FLOAT3_from_BYTE_SNORM3(FLOAT3_TYPE& dst, const BYTE_SNORM3_TYPE& src)
{
	convert_FLOAT1_from_BYTE_SNORM1(dst[0], src[0]);
	convert_FLOAT1_from_BYTE_SNORM1(dst[1], src[1]);
	convert_FLOAT1_from_BYTE_SNORM1(dst[2], src[2]);
}

// BYTE_SNORM4_TYPE -> FLOAT4_TYPE
PX_INLINE void convert_FLOAT4_from_BYTE_SNORM4(FLOAT4_TYPE& dst, const BYTE_SNORM4_TYPE& src)
{
	convert_FLOAT1_from_BYTE_SNORM1(dst[0], src[0]);
	convert_FLOAT1_from_BYTE_SNORM1(dst[1], src[1]);
	convert_FLOAT1_from_BYTE_SNORM1(dst[2], src[2]);
	convert_FLOAT1_from_BYTE_SNORM1(dst[3], src[3]);
}

// BYTE_SNORM4_QUATXYZW_TYPE -> FLOAT4_QUAT_TYPE
PX_INLINE void convert_FLOAT4_QUAT_from_BYTE_SNORM4_QUATXYZW(FLOAT4_QUAT_TYPE& dst, const BYTE_SNORM4_QUATXYZW_TYPE& src)
{
	convert_FLOAT1_from_BYTE_SNORM1(dst.x, src[0]);
	convert_FLOAT1_from_BYTE_SNORM1(dst.y, src[1]);
	convert_FLOAT1_from_BYTE_SNORM1(dst.z, src[2]);
	convert_FLOAT1_from_BYTE_SNORM1(dst.w, src[3]);
}

// SHORT_SNORM1_TYPE -> FLOAT1_TYPE
PX_INLINE void convert_FLOAT1_from_SHORT_SNORM1(FLOAT1_TYPE& dst, const SHORT_SNORM1_TYPE& src)
{
	dst = (float)src / 32767.0f;
}

// SHORT_SNORM2_TYPE -> FLOAT2_TYPE
PX_INLINE void convert_FLOAT2_from_SHORT_SNORM2(FLOAT2_TYPE& dst, const SHORT_SNORM2_TYPE& src)
{
	convert_FLOAT1_from_SHORT_SNORM1(dst[0], src[0]);
	convert_FLOAT1_from_SHORT_SNORM1(dst[1], src[1]);
}

// SHORT_SNORM3_TYPE -> FLOAT3_TYPE
PX_INLINE void convert_FLOAT3_from_SHORT_SNORM3(FLOAT3_TYPE& dst, const SHORT_SNORM3_TYPE& src)
{
	convert_FLOAT1_from_SHORT_SNORM1(dst[0], src[0]);
	convert_FLOAT1_from_SHORT_SNORM1(dst[1], src[1]);
	convert_FLOAT1_from_SHORT_SNORM1(dst[2], src[2]);
}

// SHORT_SNORM4_TYPE -> FLOAT4_TYPE
PX_INLINE void convert_FLOAT4_from_SHORT_SNORM4(FLOAT4_TYPE& dst, const SHORT_SNORM4_TYPE& src)
{
	convert_FLOAT1_from_SHORT_SNORM1(dst[0], src[0]);
	convert_FLOAT1_from_SHORT_SNORM1(dst[1], src[1]);
	convert_FLOAT1_from_SHORT_SNORM1(dst[2], src[2]);
	convert_FLOAT1_from_SHORT_SNORM1(dst[3], src[3]);
}

// SHORT_SNORM4_QUATXYZW_TYPE -> FLOAT4_QUAT_TYPE
PX_INLINE void convert_FLOAT4_QUAT_from_SHORT_SNORM4_QUATXYZW(FLOAT4_QUAT_TYPE& dst, const SHORT_SNORM4_QUATXYZW_TYPE& src)
{
	convert_FLOAT1_from_SHORT_SNORM1(dst.x, src[0]);
	convert_FLOAT1_from_SHORT_SNORM1(dst.y, src[1]);
	convert_FLOAT1_from_SHORT_SNORM1(dst.z, src[2]);
	convert_FLOAT1_from_SHORT_SNORM1(dst.w, src[3]);
}

// FLOAT1_TYPE -> BYTE_SNORM1_TYPE
PX_INLINE void convert_BYTE_SNORM1_from_FLOAT1(BYTE_SNORM1_TYPE& dst, const FLOAT1_TYPE& src)
{
	dst = (int8_t)((int16_t)(src * 127.0f + 127.5f) - 127);	// Doing it this way to avoid nonuniform mapping near zero
}

// FLOAT2_TYPE -> BYTE_SNORM2_TYPE
PX_INLINE void convert_BYTE_SNORM2_from_FLOAT2(BYTE_SNORM2_TYPE& dst, const FLOAT2_TYPE& src)
{
	convert_BYTE_SNORM1_from_FLOAT1(dst[0], src[0]);
	convert_BYTE_SNORM1_from_FLOAT1(dst[1], src[1]);
}

// FLOAT3_TYPE -> BYTE_SNORM3_TYPE
PX_INLINE void convert_BYTE_SNORM3_from_FLOAT3(BYTE_SNORM3_TYPE& dst, const FLOAT3_TYPE& src)
{
	convert_BYTE_SNORM1_from_FLOAT1(dst[0], src[0]);
	convert_BYTE_SNORM1_from_FLOAT1(dst[1], src[1]);
	convert_BYTE_SNORM1_from_FLOAT1(dst[2], src[2]);
}

// FLOAT4_TYPE -> BYTE_SNORM4_TYPE
PX_INLINE void convert_BYTE_SNORM4_from_FLOAT4(BYTE_SNORM4_TYPE& dst, const FLOAT4_TYPE& src)
{
	convert_BYTE_SNORM1_from_FLOAT1(dst[0], src[0]);
	convert_BYTE_SNORM1_from_FLOAT1(dst[1], src[1]);
	convert_BYTE_SNORM1_from_FLOAT1(dst[2], src[2]);
	convert_BYTE_SNORM1_from_FLOAT1(dst[3], src[3]);
}

// FLOAT4_QUAT_TYPE -> BYTE_SNORM4_QUATXYZW_TYPE
PX_INLINE void convert_BYTE_SNORM4_QUATXYZW_from_FLOAT4_QUAT(BYTE_SNORM4_QUATXYZW_TYPE& dst, const FLOAT4_QUAT_TYPE& src)
{
	convert_BYTE_SNORM1_from_FLOAT1(dst[0], src.x);
	convert_BYTE_SNORM1_from_FLOAT1(dst[1], src.y);
	convert_BYTE_SNORM1_from_FLOAT1(dst[2], src.z);
	convert_BYTE_SNORM1_from_FLOAT1(dst[3], src.w);
}

// FLOAT1_TYPE -> SHORT_SNORM1_TYPE
PX_INLINE void convert_SHORT_SNORM1_from_FLOAT1(SHORT_SNORM1_TYPE& dst, const FLOAT1_TYPE& src)
{
	dst = (int16_t)((int32_t)(src * 32767.0f + 32767.5f) - 32767);	// Doing it this way to avoid nonuniform mapping near zero
}

// FLOAT2_TYPE -> SHORT_SNORM2_TYPE
PX_INLINE void convert_SHORT_SNORM2_from_FLOAT2(SHORT_SNORM2_TYPE& dst, const FLOAT2_TYPE& src)
{
	convert_SHORT_SNORM1_from_FLOAT1(dst[0], src[0]);
	convert_SHORT_SNORM1_from_FLOAT1(dst[1], src[1]);
}

// FLOAT3_TYPE -> SHORT_SNORM3_TYPE
PX_INLINE void convert_SHORT_SNORM3_from_FLOAT3(SHORT_SNORM3_TYPE& dst, const FLOAT3_TYPE& src)
{
	convert_SHORT_SNORM1_from_FLOAT1(dst[0], src[0]);
	convert_SHORT_SNORM1_from_FLOAT1(dst[1], src[1]);
	convert_SHORT_SNORM1_from_FLOAT1(dst[2], src[2]);
}

// FLOAT4_TYPE -> SHORT_SNORM4_TYPE
PX_INLINE void convert_SHORT_SNORM4_from_FLOAT4(SHORT_SNORM4_TYPE& dst, const FLOAT4_TYPE& src)
{
	convert_SHORT_SNORM1_from_FLOAT1(dst[0], src[0]);
	convert_SHORT_SNORM1_from_FLOAT1(dst[1], src[1]);
	convert_SHORT_SNORM1_from_FLOAT1(dst[2], src[2]);
	convert_SHORT_SNORM1_from_FLOAT1(dst[3], src[3]);
}

// FLOAT4_QUAT_TYPE -> SHORT_SNORM4_QUATXYZW_TYPE
PX_INLINE void convert_SHORT_SNORM4_QUATXYZW_from_FLOAT4_QUAT(SHORT_SNORM4_QUATXYZW_TYPE& dst, const FLOAT4_QUAT_TYPE& src)
{
	convert_SHORT_SNORM1_from_FLOAT1(dst[0], src.x);
	convert_SHORT_SNORM1_from_FLOAT1(dst[1], src.y);
	convert_SHORT_SNORM1_from_FLOAT1(dst[2], src.z);
	convert_SHORT_SNORM1_from_FLOAT1(dst[3], src.w);
}

// Color format conversions
PX_INLINE void convert_B8G8R8A8_from_R8G8B8A8(B8G8R8A8_TYPE& dst, const R8G8B8A8_TYPE& src)
{
	dst.r = src.r;
	dst.g = src.g;
	dst.b = src.b;
	dst.a = src.a;
}

PX_INLINE void convert_R8G8B8A8_from_B8G8R8A8(R8G8B8A8_TYPE& dst, const B8G8R8A8_TYPE& src)
{
	dst.r = src.r;
	dst.g = src.g;
	dst.b = src.b;
	dst.a = src.a;
}

PX_INLINE void convert_R32G32B32A32_FLOAT_from_R8G8B8A8(R32G32B32A32_FLOAT_TYPE& dst, const R8G8B8A8_TYPE& src)
{
	(VertexColor&)dst = VertexColor((const ColorRGBA&)src);
}

PX_INLINE void convert_R8G8B8A8_from_R32G32B32A32_FLOAT(R8G8B8A8_TYPE& dst, const R32G32B32A32_FLOAT_TYPE& src)
{
	(ColorRGBA&)dst = ((const VertexColor&)src).toColorRGBA();
}

PX_INLINE void convert_B32G32R32A32_FLOAT_from_R8G8B8A8(B32G32R32A32_FLOAT_TYPE& dst, const R8G8B8A8_TYPE& src)
{
	(VertexColor&)dst = VertexColor((const ColorRGBA&)src);
	float t = dst.r;
	dst.r = dst.b;
	dst.b = t;
}

PX_INLINE void convert_R8G8B8A8_from_B32G32R32A32_FLOAT(R8G8B8A8_TYPE& dst, const B32G32R32A32_FLOAT_TYPE& src)
{
	(ColorRGBA&)dst = ((const VertexColor&)src).toColorRGBA();
	uint8_t t = dst.r;
	dst.r = dst.b;
	dst.b = t;
}

PX_INLINE void convert_R32G32B32A32_FLOAT_from_B8G8R8A8(R32G32B32A32_FLOAT_TYPE& dst, const B8G8R8A8_TYPE& src)
{
	(VertexColor&)dst = VertexColor((const ColorRGBA&)src);
	float t = dst.r;
	dst.r = dst.b;
	dst.b = t;
}

PX_INLINE void convert_B8G8R8A8_from_R32G32B32A32_FLOAT(B8G8R8A8_TYPE& dst, const R32G32B32A32_FLOAT_TYPE& src)
{
	(ColorRGBA&)dst = ((const VertexColor&)src).toColorRGBA();
	uint8_t t = dst.r;
	dst.r = dst.b;
	dst.b = t;
}

PX_INLINE void convert_B32G32R32A32_FLOAT_from_B8G8R8A8(B32G32R32A32_FLOAT_TYPE& dst, const B8G8R8A8_TYPE& src)
{
	(VertexColor&)dst = VertexColor((const ColorRGBA&)src);
}

PX_INLINE void convert_B8G8R8A8_from_B32G32R32A32_FLOAT(B8G8R8A8_TYPE& dst, const B32G32R32A32_FLOAT_TYPE& src)
{
	(ColorRGBA&)dst = ((const VertexColor&)src).toColorRGBA();
}

PX_INLINE void convert_B32G32R32A32_FLOAT_from_R32G32B32A32_FLOAT(B32G32R32A32_FLOAT_TYPE& dst, const R32G32B32A32_FLOAT_TYPE& src)
{
	dst.r = src.r;
	dst.g = src.g;
	dst.b = src.b;
	dst.a = src.a;
}

PX_INLINE void convert_R32G32B32A32_FLOAT_from_B32G32R32A32_FLOAT(R32G32B32A32_FLOAT_TYPE& dst, const B32G32R32A32_FLOAT_TYPE& src)
{
	dst.r = src.r;
	dst.g = src.g;
	dst.b = src.b;
	dst.a = src.a;
}

// Data conversion macros
#define HANDLE_CONVERT1( _DstFormat, _SrcFormat ) \
	case RenderDataFormat::_DstFormat : \
		if( srcFormat == RenderDataFormat::_SrcFormat ) \
		{ \
			convert_##_DstFormat##_from_##_SrcFormat( ((_DstFormat##_TYPE*)dst)[dstIndex], ((const _SrcFormat##_TYPE*)src)[srcIndex] ); \
		} \
		break;

#define HANDLE_CONVERT2( _DstFormat, _SrcFormat1, _SrcFormat2 ) \
	case RenderDataFormat::_DstFormat : \
		if( srcFormat == RenderDataFormat::_SrcFormat1 ) \
		{ \
			convert_##_DstFormat##_from_##_SrcFormat1( ((_DstFormat##_TYPE*)dst)[dstIndex], ((const _SrcFormat1##_TYPE*)src)[srcIndex] ); \
		} \
		else if( srcFormat == RenderDataFormat::_SrcFormat2 ) \
		{ \
			convert_##_DstFormat##_from_##_SrcFormat2( ((_DstFormat##_TYPE*)dst)[dstIndex], ((const _SrcFormat2##_TYPE*)src)[srcIndex] ); \
		} \
		break;

#define HANDLE_CONVERT3( _DstFormat, _SrcFormat1, _SrcFormat2, _SrcFormat3 ) \
	case RenderDataFormat::_DstFormat : \
		if( srcFormat == RenderDataFormat::_SrcFormat1 ) \
		{ \
			convert_##_DstFormat##_from_##_SrcFormat1( ((_DstFormat##_TYPE*)dst)[dstIndex], ((const _SrcFormat1##_TYPE*)src)[srcIndex] ); \
		} \
		else if( srcFormat == RenderDataFormat::_SrcFormat2 ) \
		{ \
			convert_##_DstFormat##_from_##_SrcFormat2( ((_DstFormat##_TYPE*)dst)[dstIndex], ((const _SrcFormat2##_TYPE*)src)[srcIndex] ); \
		} \
		else if( srcFormat == RenderDataFormat::_SrcFormat3 ) \
		{ \
			convert_##_DstFormat##_from_##_SrcFormat3( ((_DstFormat##_TYPE*)dst)[dstIndex], ((const _SrcFormat3##_TYPE*)src)[srcIndex] ); \
		} \
		break;

// ... etc.

PX_INLINE bool copyRenderVertexData(void* dst, RenderDataFormat::Enum dstFormat, uint32_t dstIndex, const void* src, RenderDataFormat::Enum srcFormat, uint32_t srcIndex)
{
	if (dstFormat == srcFormat)
	{
		// Direct data copy
		if (dstFormat != RenderDataFormat::UNSPECIFIED)
		{
			uint8_t* srcPtr = (uint8_t*)src;
			uint8_t* dstPtr = (uint8_t*)dst;

			const uint32_t size = RenderDataFormat::getFormatDataSize(dstFormat);
			memcpy(dstPtr + (dstIndex * size), srcPtr + (srcIndex * size), size);
		}
		return true;
	}

	switch (dstFormat)
	{
	case RenderDataFormat::UNSPECIFIED:
			break; // The simplest case, do nothing

		// Put format converters here

		HANDLE_CONVERT1(USHORT1, UINT1)
		HANDLE_CONVERT1(USHORT2, UINT2)
		HANDLE_CONVERT1(USHORT3, UINT3)
		HANDLE_CONVERT1(USHORT4, UINT4)

		HANDLE_CONVERT1(UINT1, USHORT1)
		HANDLE_CONVERT1(UINT2, USHORT2)
		HANDLE_CONVERT1(UINT3, USHORT3)
		HANDLE_CONVERT1(UINT4, USHORT4)

		HANDLE_CONVERT1(BYTE_SNORM1, FLOAT1)
		HANDLE_CONVERT1(BYTE_SNORM2, FLOAT2)
		HANDLE_CONVERT1(BYTE_SNORM3, FLOAT3)
		HANDLE_CONVERT1(BYTE_SNORM4, FLOAT4)
		HANDLE_CONVERT1(BYTE_SNORM4_QUATXYZW, FLOAT4_QUAT)
		HANDLE_CONVERT1(SHORT_SNORM1, FLOAT1)
		HANDLE_CONVERT1(SHORT_SNORM2, FLOAT2)
		HANDLE_CONVERT1(SHORT_SNORM3, FLOAT3)
		HANDLE_CONVERT1(SHORT_SNORM4, FLOAT4)
		HANDLE_CONVERT1(SHORT_SNORM4_QUATXYZW, FLOAT4_QUAT)

		HANDLE_CONVERT2(FLOAT1, BYTE_SNORM1, SHORT_SNORM1)
		HANDLE_CONVERT2(FLOAT2, BYTE_SNORM2, SHORT_SNORM2)
		HANDLE_CONVERT2(FLOAT3, BYTE_SNORM3, SHORT_SNORM3)
		HANDLE_CONVERT2(FLOAT4, BYTE_SNORM4, SHORT_SNORM4)
		HANDLE_CONVERT2(FLOAT4_QUAT, BYTE_SNORM4_QUATXYZW, SHORT_SNORM4_QUATXYZW)

		HANDLE_CONVERT3(R8G8B8A8, B8G8R8A8, R32G32B32A32_FLOAT, B32G32R32A32_FLOAT)
		HANDLE_CONVERT3(B8G8R8A8, R8G8B8A8, R32G32B32A32_FLOAT, B32G32R32A32_FLOAT)
		HANDLE_CONVERT3(R32G32B32A32_FLOAT, R8G8B8A8, B8G8R8A8, B32G32R32A32_FLOAT)
		HANDLE_CONVERT3(B32G32R32A32_FLOAT, R8G8B8A8, B8G8R8A8, R32G32B32A32_FLOAT)

	default:
		{
		    PX_ALWAYS_ASSERT();	// Format conversion not handled
		    return false;
		}
		}

	return true;
}

bool copyRenderVertexBuffer(void* dst, RenderDataFormat::Enum dstFormat, uint32_t dstStride, uint32_t dstStart,
                            const void* src, RenderDataFormat::Enum srcFormat, uint32_t srcStride, uint32_t srcStart,
                            uint32_t numVertices, int32_t* invMap = NULL);

/*
	Local utilities
 */
PX_INLINE bool vertexSemanticFormatValid(RenderVertexSemantic::Enum semantic, RenderDataFormat::Enum format)
{
	switch (semantic)
	{
	case RenderVertexSemantic::POSITION:
			return format == RenderDataFormat::FLOAT3;
	case RenderVertexSemantic::NORMAL:
			case RenderVertexSemantic::BINORMAL:
					return format == RenderDataFormat::FLOAT3 || format == RenderDataFormat::BYTE_SNORM3;
	case RenderVertexSemantic::TANGENT:
		return	format == RenderDataFormat::FLOAT3 || format == RenderDataFormat::BYTE_SNORM3 ||
				format == RenderDataFormat::FLOAT4 || format == RenderDataFormat::BYTE_SNORM4;
	case RenderVertexSemantic::COLOR:
			return format == RenderDataFormat::R8G8B8A8 || format == RenderDataFormat::B8G8R8A8;
	case RenderVertexSemantic::TEXCOORD0:
		case RenderVertexSemantic::TEXCOORD1:
			case RenderVertexSemantic::TEXCOORD2:
				case RenderVertexSemantic::TEXCOORD3:
						return format == RenderDataFormat::FLOAT2;	// Not supporting other formats yet
	case RenderVertexSemantic::DISPLACEMENT_TEXCOORD:
		return format == RenderDataFormat::FLOAT2 || format == RenderDataFormat::FLOAT3;
	case RenderVertexSemantic::DISPLACEMENT_FLAGS:
		return format == RenderDataFormat::UINT1 || format == RenderDataFormat::USHORT1;
	case RenderVertexSemantic::BONE_INDEX:
			return	format == RenderDataFormat::USHORT1 ||
			        format == RenderDataFormat::USHORT2 ||
			        format == RenderDataFormat::USHORT3 ||
			        format == RenderDataFormat::USHORT4;	// Not supporting other formats yet
	case RenderVertexSemantic::BONE_WEIGHT:
			return	format == RenderDataFormat::FLOAT1 ||
			        format == RenderDataFormat::FLOAT2 ||
			        format == RenderDataFormat::FLOAT3 ||
			        format == RenderDataFormat::FLOAT4;	// Not supporting other formats yet
	default:
		return false;
	}
}

PX_INLINE uint32_t vertexSemanticFormatElementCount(RenderVertexSemantic::Enum semantic, RenderDataFormat::Enum format)
{
	switch (semantic)
	{
	case RenderVertexSemantic::CUSTOM:
		case RenderVertexSemantic::POSITION:
			case RenderVertexSemantic::NORMAL:
				case RenderVertexSemantic::TANGENT:
					case RenderVertexSemantic::BINORMAL:
						case RenderVertexSemantic::COLOR:
							case RenderVertexSemantic::TEXCOORD0:
								case RenderVertexSemantic::TEXCOORD1:
									case RenderVertexSemantic::TEXCOORD2:
										case RenderVertexSemantic::TEXCOORD3:
											case RenderVertexSemantic::DISPLACEMENT_TEXCOORD:
												case RenderVertexSemantic::DISPLACEMENT_FLAGS:
													return 1;
	case RenderVertexSemantic::BONE_INDEX:
			switch (format)
			{
			case RenderDataFormat::USHORT1:
					return 1;
			case RenderDataFormat::USHORT2:
					return 2;
			case RenderDataFormat::USHORT3:
					return 3;
			case RenderDataFormat::USHORT4:
					return 4;
			default:
				break;
			}
			return 0;
	case RenderVertexSemantic::BONE_WEIGHT:
			switch (format)
			{
			case RenderDataFormat::FLOAT1:
					return 1;
			case RenderDataFormat::FLOAT2:
					return 2;
			case RenderDataFormat::FLOAT3:
					return 3;
			case RenderDataFormat::FLOAT4:
					return 4;
			default:
				break;
			}
			return 0;
	default:
		return 0;
	}
}


}
} // end namespace apex


#endif	// __APEXSHAREDUTILS_H__
