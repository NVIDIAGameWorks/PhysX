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


#ifdef _MANAGED
#pragma managed(push, off)
#endif

#include "Apex.h"
#include "PxMath.h"
#include "ApexUsingNamespace.h"
#include "ApexSharedUtils.h"
#include "FractureTools.h"

#include "authoring/Fracturing.h"

#ifndef WITHOUT_APEX_AUTHORING
#include "ApexSharedSerialization.h"

#define CUTOUT_DISTANCE_THRESHOLD	(0.7f)

#define CUTOUT_DISTANCE_EPS			(0.01f)

struct POINT2D
{
	POINT2D() {}
	POINT2D(int32_t _x, int32_t _y) : x(_x), y(_y) {}

	int32_t x;
	int32_t y;
};

// Unsigned modulus
PX_INLINE uint32_t mod(int32_t n, uint32_t modulus)
{
	const int32_t d = n/(int32_t)modulus;
	const int32_t m = n - d*(int32_t)modulus;
	return m >= 0 ? (uint32_t)m : (uint32_t)m + modulus;
}

PX_INLINE float square(float x)
{
	return x * x;
}

// 2D cross product
PX_INLINE float dotXY(const physx::PxVec3& v, const physx::PxVec3& w)
{
	return v.x * w.x + v.y * w.y;
}

// Z-component of cross product
PX_INLINE float crossZ(const physx::PxVec3& v, const physx::PxVec3& w)
{
	return v.x * w.y - v.y * w.x;
}

// z coordinates may be used to store extra info - only deal with x and y
PX_INLINE float perpendicularDistanceSquared(const physx::PxVec3& v0, const physx::PxVec3& v1, const physx::PxVec3& v2)
{
	const physx::PxVec3 base = v2 - v0;
	const physx::PxVec3 leg = v1 - v0;

	const float baseLen2 = dotXY(base, base);

	return baseLen2 > PX_EPS_F32 * dotXY(leg, leg) ? square(crossZ(base, leg)) / baseLen2 : 0.0f;
}

// z coordinates may be used to store extra info - only deal with x and y
PX_INLINE float perpendicularDistanceSquared(const physx::Array< physx::PxVec3 >& cutout, uint32_t index)
{
	const uint32_t size = cutout.size();
	return perpendicularDistanceSquared(cutout[(index + size - 1) % size], cutout[index], cutout[(index + 1) % size]);
}


struct CutoutVert
{
	int32_t	cutoutIndex;
	int32_t	vertIndex;

	void	set(int32_t _cutoutIndex, int32_t _vertIndex)
	{
		cutoutIndex = _cutoutIndex;
		vertIndex = _vertIndex;
	}
};

struct NewVertex
{
	CutoutVert	vertex;
	float		edgeProj;
};

static int compareNewVertices(const void* a, const void* b)
{
	const int32_t cutoutDiff = ((NewVertex*)a)->vertex.cutoutIndex - ((NewVertex*)b)->vertex.cutoutIndex;
	if (cutoutDiff)
	{
		return cutoutDiff;
	}
	const int32_t vertDiff = ((NewVertex*)a)->vertex.vertIndex - ((NewVertex*)b)->vertex.vertIndex;
	if (vertDiff)
	{
		return vertDiff;
	}
	const float projDiff = ((NewVertex*)a)->edgeProj - ((NewVertex*)b)->edgeProj;
	return projDiff ? (projDiff < 0.0f ? -1 : 1) : 0;
}

template<typename T>
class Map2d
{
public:
	Map2d() : mMem(NULL) {}
	Map2d(uint32_t width, uint32_t height) : mMem(NULL)
	{
		create_internal(width, height, NULL);
	}
	Map2d(uint32_t width, uint32_t height, T fillValue) : mMem(NULL)
	{
		create_internal(width, height, &fillValue);
	}
	Map2d(const Map2d& map)
	{
		*this = map;
	}
	~Map2d()
	{
		delete [] mMem;
	}

	Map2d&		operator = (const Map2d& map)
	{
		delete [] mMem;
		mMem = NULL;
		if (map.mMem)
		{
			create_internal(map.mWidth, map.mHeight, NULL);
			memcpy(mMem, map.mMem, mWidth * mHeight);
		}
		return *this;
	}

	void		create(uint32_t width, uint32_t height)
	{
		return create_internal(width, height, NULL);
	}
	void		create(uint32_t width, uint32_t height, T fillValue)
	{
		create_internal(width, height, &fillValue);
	}

	void		clear(const T value)
	{
		T* mem = mMem;
		T* stop = mMem + mWidth * mHeight;
		while (mem < stop)
		{
			*mem++ = value;
		}
	}

	void		setOrigin(uint32_t x, uint32_t y)
	{
		mOriginX = x;
		mOriginY = y;
	}

	const T&	operator()(int32_t x, int32_t y) const
	{
		x = (int32_t)mod(x+(int32_t)mOriginX, mWidth);
		y = (int32_t)mod(y+(int32_t)mOriginY, mHeight);
		return mMem[(uint32_t)(x + y * (int32_t)mWidth)];
	}
	T&			operator()(int32_t x, int32_t y)
	{
		x = (int32_t)mod(x+(int32_t)mOriginX, mWidth);
		y = (int32_t)mod(y+(int32_t)mOriginY, mHeight);
		return mMem[(uint32_t)(x + y * (int32_t)mWidth)];
	}

private:

	void	create_internal(uint32_t width, uint32_t height, T* val)
	{
		delete [] mMem;
		mWidth = width;
		mHeight = height;
		mMem = new T[mWidth * mHeight];
		mOriginX = 0;
		mOriginY = 0;
		if (val)
		{
			clear(*val);
		}
	}

	T*		mMem;
	uint32_t	mWidth;
	uint32_t	mHeight;
	uint32_t	mOriginX;
	uint32_t	mOriginY;
};

class BitMap
{
public:
	BitMap() : mMem(NULL) {}
	BitMap(uint32_t width, uint32_t height) : mMem(NULL)
	{
		create_internal(width, height, NULL);
	}
	BitMap(uint32_t width, uint32_t height, bool fillValue) : mMem(NULL)
	{
		create_internal(width, height, &fillValue);
	}
	BitMap(const BitMap& map)
	{
		*this = map;
	}
	~BitMap()
	{
		delete [] mMem;
	}

	BitMap&	operator = (const BitMap& map)
	{
		delete [] mMem;
		mMem = NULL;
		if (map.mMem)
		{
			create_internal(map.mWidth, map.mHeight, NULL);
			memcpy(mMem, map.mMem, mHeight * mRowBytes);
		}
		return *this;
	}

	void	create(uint32_t width, uint32_t height)
	{
		return create_internal(width, height, NULL);
	}
	void	create(uint32_t width, uint32_t height, bool fillValue)
	{
		create_internal(width, height, &fillValue);
	}

	void	clear(bool value)
	{
		memset(mMem, value ? 0xFF : 0x00, mRowBytes * mHeight);
	}

	void	setOrigin(uint32_t x, uint32_t y)
	{
		mOriginX = x;
		mOriginY = y;
	}

	bool	read(int32_t x, int32_t y) const
	{
		x = (int32_t)mod(x+(int32_t)mOriginX, mWidth);
		y = (int32_t)mod(y+(int32_t)mOriginY, mHeight);
		return ((mMem[(x >> 3) + y * mRowBytes] >> (x & 7)) & 1) != 0;
	}
	void	set(int32_t x, int32_t y)
	{
		x = (int32_t)mod(x+(int32_t)mOriginX, mWidth);
		y = (int32_t)mod(y+(int32_t)mOriginY, mHeight);
		mMem[(x >> 3) + y * mRowBytes] |= 1 << (x & 7);
	}
	void	reset(int32_t x, int32_t y)
	{
		x = (int32_t)mod(x+(int32_t)mOriginX, mWidth);
		y = (int32_t)mod(y+(int32_t)mOriginY, mHeight);
		mMem[(x >> 3) + y * mRowBytes] &= ~(1 << (x & 7));
	}

private:

	void	create_internal(uint32_t width, uint32_t height, bool* val)
	{
		delete [] mMem;
		mRowBytes = (width + 7) >> 3;
		const uint32_t bytes = mRowBytes * height;
		if (bytes == 0)
		{
			mWidth = mHeight = 0;
			mMem = NULL;
			return;
		}
		mWidth = width;
		mHeight = height;
		mMem = new uint8_t[bytes];
		mOriginX = 0;
		mOriginY = 0;
		if (val)
		{
			clear(*val);
		}
	}

	uint8_t*	mMem;
	uint32_t	mWidth;
	uint32_t	mHeight;
	uint32_t	mRowBytes;
	uint32_t	mOriginX;
	uint32_t	mOriginY;
};


PX_INLINE int32_t taxicabSine(int32_t i)
{
	// 0 1 1 1 0 -1 -1 -1
	return (int32_t)((0x01A9 >> ((i & 7) << 1)) & 3) - 1;
}

// Only looks at x and y components
PX_INLINE bool directionsXYOrderedCCW(const physx::PxVec3& d0, const physx::PxVec3& d1, const physx::PxVec3& d2)
{
	const bool ccw02 = crossZ(d0, d2) > 0.0f;
	const bool ccw01 = crossZ(d0, d1) > 0.0f;
	const bool ccw21 = crossZ(d2, d1) > 0.0f;
	return ccw02 ? ccw01 && ccw21 : ccw01 || ccw21;
}

PX_INLINE float compareTraceSegmentToLineSegment(const physx::Array<POINT2D>& trace, int _start, int delta, float distThreshold, uint32_t width, uint32_t height, bool hasBorder)
{
	if (delta < 2)
	{
		return 0.0f;
	}

	const uint32_t size = trace.size();

	uint32_t start = (uint32_t)_start, end = (uint32_t)(_start + delta) % size;

	const bool startIsOnBorder = hasBorder && (trace[start].x == -1 || trace[start].x == (int)width || trace[start].y == -1 || trace[start].y == (int)height);
	const bool endIsOnBorder = hasBorder && (trace[end].x == -1 || trace[end].x == (int)width || trace[end].y == -1 || trace[end].y == (int)height);

	if (startIsOnBorder || endIsOnBorder)
	{
		if ((trace[start].x == -1 && trace[end].x == -1) ||
		        (trace[start].y == -1 && trace[end].y == -1) ||
		        (trace[start].x == (int)width && trace[end].x == (int)width) ||
		        (trace[start].y == (int)height && trace[end].y == (int)height))
		{
			return 0.0f;
		}
		return PX_MAX_F32;
	}

	physx::PxVec3 orig((float)trace[start].x, (float)trace[start].y, 0);
	physx::PxVec3 dest((float)trace[end].x, (float)trace[end].y, 0);
	physx::PxVec3 dir = dest - orig;

	dir.normalize();

	float aveError = 0.0f;

	for (;;)
	{
		if (++start >= size)
		{
			start = 0;
		}
		if (start == end)
		{
			break;
		}
		physx::PxVec3 testDisp((float)trace[start].x, (float)trace[start].y, 0);
		testDisp -= orig;
		aveError += (float)(physx::PxAbs(testDisp.x * dir.y - testDisp.y * dir.x) >= distThreshold);
	}

	aveError /= delta - 1;

	return aveError;
}

// Segment i starts at vi and ends at vi+ei
// Tests for overlap in segments' projection onto xy plane
// Returns distance between line segments.  (Negative value indicates overlap.)
PX_INLINE float segmentsIntersectXY(const physx::PxVec3& v0, const physx::PxVec3& e0, const physx::PxVec3& v1, const physx::PxVec3& e1)
{
	const physx::PxVec3 dv = v1 - v0;

	physx::PxVec3 d0 = e0;
	d0.normalize();
	physx::PxVec3 d1 = e1;
	d1.normalize();

	const float c10 = crossZ(dv, d0);
	const float d10 = crossZ(e1, d0);

	float a1 = physx::PxAbs(c10);
	float b1 = physx::PxAbs(c10 + d10);

	if (c10 * (c10 + d10) < 0.0f)
	{
		if (a1 < b1)
		{
			a1 = -a1;
		}
		else
		{
			b1 = -b1;
		}
	}

	const float c01 = crossZ(d1, dv);
	const float d01 = crossZ(e0, d1);

	float a2 = physx::PxAbs(c01);
	float b2 = physx::PxAbs(c01 + d01);

	if (c01 * (c01 + d01) < 0.0f)
	{
		if (a2 < b2)
		{
			a2 = -a2;
		}
		else
		{
			b2 = -b2;
		}
	}

	return physx::PxMax(physx::PxMin(a1, b1), physx::PxMin(a2, b2));
}

// If point projects onto segment, returns true and proj is set to a
// value in the range [0,1], indicating where along the segment (from v0 to v1)
// the projection lies, and dist2 is set to the distance squared from point to
// the line segment.  Otherwise, returns false.
// Note, if v1 = v0, then the function returns true with proj = 0.
PX_INLINE bool projectOntoSegmentXY(float& proj, float& dist2, const physx::PxVec3& point, const physx::PxVec3& v0, const physx::PxVec3& v1, float margin)
{
	const physx::PxVec3 seg = v1 - v0;
	const physx::PxVec3 x = point - v0;
	const float seg2 = dotXY(seg, seg);
	const float d = dotXY(x, seg);

	if (d < 0.0f || d > seg2)
	{
		return false;
	}

	const float margin2 = margin * margin;

	const float p = seg2 > 0.0f ? d / seg2 : 0.0f;
	const float lineDist2 = d * p;

	if (lineDist2 < margin2)
	{
		return false;
	}

	const float pPrime = 1.0f - p;
	const float dPrime = seg2 - d;
	const float lineDistPrime2 = dPrime * pPrime;

	if (lineDistPrime2 < margin2)
	{
		return false;
	}

	proj = p;
	dist2 = dotXY(x, x) - lineDist2;
	return true;
}

PX_INLINE bool isOnBorder(const physx::PxVec3& v, uint32_t width, uint32_t height)
{
	return v.x < -0.5f || v.x >= width - 0.5f || v.y < -0.5f || v.y >= height - 0.5f;
}

static void createCutout(nvidia::Cutout& cutout, const physx::Array<POINT2D>& trace, float snapThreshold, uint32_t width, uint32_t height, bool hasBorder)
{
	cutout.vertices.reset();

	const uint32_t traceSize = trace.size();

	if (traceSize == 0)
	{
		return;	// Nothing to do
	}

	uint32_t size = traceSize;

	physx::Array<int> vertexIndices;

	const float errorThreshold = 0.1f;

	const float pixelCenterOffset = hasBorder ? 0.5f : 0.0f;

	// Find best segment
	uint32_t start = 0;
	uint32_t delta = 0;
	for (uint32_t iStart = 0; iStart < size; ++iStart)
	{
		uint32_t iDelta = (size >> 1) + (size & 1);
		for (; iDelta > 1; --iDelta)
		{
			float fit = compareTraceSegmentToLineSegment(trace, (int32_t)iStart, (int32_t)iDelta, CUTOUT_DISTANCE_THRESHOLD, width, height, hasBorder);
			if (fit < errorThreshold)
			{
				break;
			}
		}
		if (iDelta > delta)
		{
			start = iStart;
			delta = iDelta;
		}
	}
	cutout.vertices.pushBack(physx::PxVec3((float)trace[start].x + pixelCenterOffset, (float)trace[start].y + pixelCenterOffset, 0));

	// Now complete the loop
	while ((size -= delta) > 0)
	{
		start = (start + delta) % traceSize;
		cutout.vertices.pushBack(physx::PxVec3((float)trace[start].x + pixelCenterOffset, (float)trace[start].y + pixelCenterOffset, 0));
		if (size == 1)
		{
			delta = 1;
			break;
		}
		for (delta = size - 1; delta > 1; --delta)
		{
			float fit = compareTraceSegmentToLineSegment(trace, (int32_t)start, (int32_t)delta, CUTOUT_DISTANCE_THRESHOLD, width, height, hasBorder);
			if (fit < errorThreshold)
			{
				break;
			}
		}
	}

	const float snapThresh2 = square(snapThreshold);

	// Use the snapThreshold to clean up
	while ((size = cutout.vertices.size()) >= 4)
	{
		bool reduced = false;
		for (uint32_t i = 0; i < size; ++i)
		{
			const uint32_t i1 = (i + 1) % size;
			const uint32_t i2 = (i + 2) % size;
			const uint32_t i3 = (i + 3) % size;
			physx::PxVec3& v0 = cutout.vertices[i];
			physx::PxVec3& v1 = cutout.vertices[i1];
			physx::PxVec3& v2 = cutout.vertices[i2];
			physx::PxVec3& v3 = cutout.vertices[i3];
			const physx::PxVec3 d0 = v1 - v0;
			const physx::PxVec3 d1 = v2 - v1;
			const physx::PxVec3 d2 = v3 - v2;
			const float den = crossZ(d0, d2);
			if (den != 0)
			{
				const float recipDen = 1.0f / den;
				const float s0 = crossZ(d1, d2) * recipDen;
				const float s2 = crossZ(d0, d1) * recipDen;
				if (s0 >= 0 || s2 >= 0)
				{
					if (d0.magnitudeSquared()*s0* s0 <= snapThresh2 && d2.magnitudeSquared()*s2* s2 <= snapThresh2)
					{
						v1 += d0 * s0;

						uint32_t index = (uint32_t)(&v2 - cutout.vertices.begin());

						cutout.vertices.remove(index);

						reduced = true;
						break;
					}
				}
			}
		}
		if (!reduced)
		{
			break;
		}
	}
}

static void splitTJunctions(nvidia::CutoutSetImpl& cutoutSet, float threshold)
{
	// Set bounds reps
	physx::Array<nvidia::BoundsRep> bounds;
	physx::Array<CutoutVert> cutoutMap;	// maps bounds # -> ( cutout #, vertex # ).
	physx::Array<nvidia::IntPair> overlaps;

	const float distThreshold2 = threshold * threshold;

	// Split T-junctions
	uint32_t edgeCount = 0;
	for (uint32_t i = 0; i < cutoutSet.cutouts.size(); ++i)
	{
		edgeCount += cutoutSet.cutouts[i].vertices.size();
	}

	bounds.resize(edgeCount);
	cutoutMap.resize(edgeCount);

	edgeCount = 0;
	for (uint32_t i = 0; i < cutoutSet.cutouts.size(); ++i)
	{
		nvidia::Cutout& cutout = cutoutSet.cutouts[i];
		const uint32_t cutoutSize = cutout.vertices.size();
		for (uint32_t j = 0; j < cutoutSize; ++j)
		{
			bounds[edgeCount].aabb.include(cutout.vertices[j]);
			bounds[edgeCount].aabb.include(cutout.vertices[(j + 1) % cutoutSize]);
			PX_ASSERT(!bounds[edgeCount].aabb.isEmpty());
			bounds[edgeCount].aabb.fattenFast(threshold);
			cutoutMap[edgeCount].set((int32_t)i, (int32_t)j);
			++edgeCount;
		}
	}

	// Find bounds overlaps
	if (bounds.size() > 0)
	{
		boundsCalculateOverlaps(overlaps, nvidia::Bounds3XY, &bounds[0], bounds.size(), sizeof(bounds[0]));
	}

	physx::Array<NewVertex> newVertices;
	for (uint32_t overlapIndex = 0; overlapIndex < overlaps.size(); ++overlapIndex)
	{
		const nvidia::IntPair& mapPair = overlaps[overlapIndex];
		const CutoutVert& seg0Map = cutoutMap[(uint32_t)mapPair.i0];
		const CutoutVert& seg1Map = cutoutMap[(uint32_t)mapPair.i1];

		if (seg0Map.cutoutIndex == seg1Map.cutoutIndex)
		{
			// Only split based on vertex/segment junctions from different cutouts
			continue;
		}

		NewVertex newVertex;
		float dist2 = 0;

		const nvidia::Cutout& cutout0 = cutoutSet.cutouts[(uint32_t)seg0Map.cutoutIndex];
		const uint32_t cutoutSize0 = cutout0.vertices.size();
		const nvidia::Cutout& cutout1 = cutoutSet.cutouts[(uint32_t)seg1Map.cutoutIndex];
		const uint32_t cutoutSize1 = cutout1.vertices.size();

		if (projectOntoSegmentXY(newVertex.edgeProj, dist2, cutout0.vertices[(uint32_t)seg0Map.vertIndex], cutout1.vertices[(uint32_t)seg1Map.vertIndex], 
			cutout1.vertices[(uint32_t)(seg1Map.vertIndex + 1) % cutoutSize1], 0.25f))
		{
			if (dist2 <= distThreshold2)
			{
				newVertex.vertex = seg1Map;
				newVertices.pushBack(newVertex);
			}
		}

		if (projectOntoSegmentXY(newVertex.edgeProj, dist2, cutout1.vertices[(uint32_t)seg1Map.vertIndex], cutout0.vertices[(uint32_t)seg0Map.vertIndex], 
			cutout0.vertices[(uint32_t)(seg0Map.vertIndex + 1) % cutoutSize0], 0.25f))
		{
			if (dist2 <= distThreshold2)
			{
				newVertex.vertex = seg0Map;
				newVertices.pushBack(newVertex);
			}
		}
	}

	if (newVertices.size())
	{
		// Sort new vertices
		qsort(newVertices.begin(), newVertices.size(), sizeof(NewVertex), compareNewVertices);

		// Insert new vertices
		uint32_t lastCutoutIndex = 0xFFFFFFFF;
		uint32_t lastVertexIndex = 0xFFFFFFFF;
		float lastProj = 1.0f;
		for (uint32_t newVertexIndex = newVertices.size(); newVertexIndex--;)
		{
			const NewVertex& newVertex = newVertices[newVertexIndex];
			if (newVertex.vertex.cutoutIndex != (int32_t)lastCutoutIndex)
			{
				lastCutoutIndex = (uint32_t)newVertex.vertex.cutoutIndex;
				lastVertexIndex = 0xFFFFFFFF;
			}
			if (newVertex.vertex.vertIndex != (int32_t)lastVertexIndex)
			{
				lastVertexIndex = (uint32_t)newVertex.vertex.vertIndex;
				lastProj = 1.0f;
			}
			nvidia::Cutout& cutout = cutoutSet.cutouts[(uint32_t)newVertex.vertex.cutoutIndex];
			const float proj = lastProj > 0.0f ? newVertex.edgeProj / lastProj : 0.0f;
			const physx::PxVec3 pos = (1.0f - proj) * cutout.vertices[(uint32_t)newVertex.vertex.vertIndex] 
				+ proj * cutout.vertices[(uint32_t)(newVertex.vertex.vertIndex + 1) % cutout.vertices.size()];
			cutout.vertices.insert();
			for (uint32_t n = cutout.vertices.size(); --n > (uint32_t)newVertex.vertex.vertIndex + 1;)
			{
				cutout.vertices[n] = cutout.vertices[n - 1];
			}
			cutout.vertices[(uint32_t)newVertex.vertex.vertIndex + 1] = pos;
			lastProj = newVertex.edgeProj;
		}
	}
}

#if 0
static void mergeVertices(CutoutSetImpl& cutoutSet, float threshold, uint32_t width, uint32_t height)
{
	// Set bounds reps
	physx::Array<BoundsRep> bounds;
	physx::Array<CutoutVert> cutoutMap;	// maps bounds # -> ( cutout #, vertex # ).
	physx::Array<IntPair> overlaps;

	const float threshold2 = threshold * threshold;

	uint32_t vertexCount = 0;
	for (uint32_t i = 0; i < cutoutSet.cutouts.size(); ++i)
	{
		vertexCount += cutoutSet.cutouts[i].vertices.size();
	}

	bounds.resize(vertexCount);
	cutoutMap.resize(vertexCount);

	vertexCount = 0;
	for (uint32_t i = 0; i < cutoutSet.cutouts.size(); ++i)
	{
		Cutout& cutout = cutoutSet.cutouts[i];
		for (uint32_t j = 0; j < cutout.vertices.size(); ++j)
		{
			physx::PxVec3& vertex = cutout.vertices[j];
			physx::PxVec3 min(vertex.x - threshold, vertex.y - threshold, 0.0f);
			physx::PxVec3 max(vertex.x + threshold, vertex.y + threshold, 0.0f);
			bounds[vertexCount].aabb.set(min, max);
			cutoutMap[vertexCount].set(i, j);
			++vertexCount;
		}
	}

	// Find bounds overlaps
	overlaps.reset();
	if (bounds.size() > 0)
	{
		boundsCalculateOverlaps(overlaps, Bounds3XY, &bounds[0], bounds.size(), sizeof(bounds[0]));
	}

	const uint32_t overlapCount = overlaps.size();
	if (overlapCount)
	{
		// Sort overlaps by index0 and index1
		qsort(overlaps.begin(), overlaps.size(), sizeof(IntPair), IntPair::compare);

		// Process overlaps: merge vertices
		uint32_t groupStart = 0;
		uint32_t groupStop;
		do
		{
			const int32_t groupI0 = overlaps[groupStart].i0;
			groupStop = groupStart;
			while (++groupStop < overlapCount)
			{
				const int32_t i0 = overlaps[groupStop].i0;
				if (i0 != groupI0)
				{
					break;
				}
			}
			// Process group
			physx::PxVec3 straightV(0.0f);
			uint32_t straightCount = 0;
			physx::PxVec3 borderV(0.0f);
			uint32_t borderCount = 0;
			physx::PxVec3 v(0.0f);
			float weight = 0.0f;
			// Include i0
			const CutoutVert& vertexMap = cutoutMap[overlaps[groupStart].i0];
			Cutout& cutout = cutoutSet.cutouts[vertexMap.cutoutIndex];
			float dist2 = perpendicularDistanceSquared(cutout.vertices, vertexMap.vertIndex);
			if (isOnBorder(cutout.vertices[vertexMap.vertIndex], width, height))
			{
				borderV += cutout.vertices[vertexMap.vertIndex];
				++borderCount;
			}
			else if (dist2 < threshold2)
			{
				straightV += cutout.vertices[vertexMap.vertIndex];
				++straightCount;
			}
			else
			{
				const float recipDist2 = 1.0f / dist2;
				weight += recipDist2;
				v += cutout.vertices[vertexMap.vertIndex] * recipDist2;
			}
			for (uint32_t i = groupStart; i < groupStop; ++i)
			{
				const CutoutVert& vertexMap = cutoutMap[overlaps[i].i1];
				Cutout& cutout = cutoutSet.cutouts[vertexMap.cutoutIndex];
				dist2 = perpendicularDistanceSquared(cutout.vertices, vertexMap.vertIndex);
				if (isOnBorder(cutout.vertices[vertexMap.vertIndex], width, height))
				{
					borderV += cutout.vertices[vertexMap.vertIndex];
					++borderCount;
				}
				else if (dist2 < threshold2)
				{
					straightV += cutout.vertices[vertexMap.vertIndex];
					++straightCount;
				}
				else
				{
					const float recipDist2 = 1.0f / dist2;
					weight += recipDist2;
					v += cutout.vertices[vertexMap.vertIndex] * recipDist2;
				}
			}
			if (borderCount)
			{
				// If we have any borderVertices, these will be the only ones considered
				v = (1.0f / borderCount) * borderV;
			}
			else if (straightCount)
			{
				// Otherwise if we have any straight angles, these will be the only ones considered
				v = (1.0f / straightCount) * straightV;
			}
			else
			{
				v *= 1.0f / weight;
			}
			// Now replace all group vertices by v
			{
				const CutoutVert& vertexMap = cutoutMap[overlaps[groupStart].i0];
				cutoutSet.cutouts[vertexMap.cutoutIndex].vertices[vertexMap.vertIndex] = v;
				for (uint32_t i = groupStart; i < groupStop; ++i)
				{
					const CutoutVert& vertexMap = cutoutMap[overlaps[i].i1];
					cutoutSet.cutouts[vertexMap.cutoutIndex].vertices[vertexMap.vertIndex] = v;
				}
			}
		}
		while ((groupStart = groupStop) < overlapCount);
	}
}
#else
static void mergeVertices(nvidia::CutoutSetImpl& cutoutSet, float threshold, uint32_t width, uint32_t height)
{
	// Set bounds reps
	uint32_t vertexCount = 0;
	for (uint32_t i = 0; i < cutoutSet.cutouts.size(); ++i)
	{
		vertexCount += cutoutSet.cutouts[i].vertices.size();
	}

	physx::Array<nvidia::BoundsRep> bounds;
	physx::Array<CutoutVert> cutoutMap;	// maps bounds # -> ( cutout #, vertex # ).
	bounds.resize(vertexCount);
	cutoutMap.resize(vertexCount);

	vertexCount = 0;
	for (uint32_t i = 0; i < cutoutSet.cutouts.size(); ++i)
	{
		nvidia::Cutout& cutout = cutoutSet.cutouts[i];
		for (uint32_t j = 0; j < cutout.vertices.size(); ++j)
		{
			physx::PxVec3& vertex = cutout.vertices[j];
			physx::PxVec3 min(vertex.x - threshold, vertex.y - threshold, 0.0f);
			physx::PxVec3 max(vertex.x + threshold, vertex.y + threshold, 0.0f);
			bounds[vertexCount].aabb = physx::PxBounds3(min, max);
			cutoutMap[vertexCount].set((int32_t)i, (int32_t)j);
			++vertexCount;
		}
	}

	// Find bounds overlaps
	physx::Array<nvidia::IntPair> overlaps;
	if (bounds.size() > 0)
	{
		boundsCalculateOverlaps(overlaps, nvidia::Bounds3XY, &bounds[0], bounds.size(), sizeof(bounds[0]));
	}
	uint32_t overlapCount = overlaps.size();

	if (overlapCount == 0)
	{
		return;
	}

	// Sort by first index
	qsort(overlaps.begin(), overlapCount, sizeof(nvidia::IntPair), nvidia::IntPair::compare);

	const float threshold2 = threshold * threshold;

	physx::Array<nvidia::IntPair> pairs;

	// Group by first index
	physx::Array<uint32_t> lookup;
	nvidia::createIndexStartLookup(lookup, 0, vertexCount, &overlaps.begin()->i0, overlapCount, sizeof(nvidia::IntPair));
	for (uint32_t i = 0; i < vertexCount; ++i)
	{
		const uint32_t start = lookup[i];
		const uint32_t stop = lookup[i + 1];
		if (start == stop)
		{
			continue;
		}
		const CutoutVert& cutoutVert0 = cutoutMap[(uint32_t)overlaps[start].i0];
		const physx::PxVec3& vert0 = cutoutSet.cutouts[(uint32_t)cutoutVert0.cutoutIndex].vertices[(uint32_t)cutoutVert0.vertIndex];
		const bool isOnBorder0 = !cutoutSet.periodic && isOnBorder(vert0, width, height);
		for (uint32_t j = start; j < stop; ++j)
		{
			const CutoutVert& cutoutVert1 = cutoutMap[(uint32_t)overlaps[j].i1];
			if (cutoutVert0.cutoutIndex == cutoutVert1.cutoutIndex)
			{
				// No pairs from the same cutout
				continue;
			}
			const physx::PxVec3& vert1 = cutoutSet.cutouts[(uint32_t)cutoutVert1.cutoutIndex].vertices[(uint32_t)cutoutVert1.vertIndex];
			const bool isOnBorder1 = !cutoutSet.periodic && isOnBorder(vert1, width, height);
			if (isOnBorder0 != isOnBorder1)
			{
				// No border/non-border pairs
				continue;
			}
			if ((vert0 - vert1).magnitudeSquared() > threshold2)
			{
				// Distance outside threshold
				continue;
			}
			// A keeper.  Keep a symmetric list
			nvidia::IntPair overlap = overlaps[j];
			pairs.pushBack(overlap);
			const int32_t i0 = overlap.i0;
			overlap.i0 = overlap.i1;
			overlap.i1 = i0;
			pairs.pushBack(overlap);
		}
	}

	// Sort by first index
	qsort(pairs.begin(), pairs.size(), sizeof(nvidia::IntPair), nvidia::IntPair::compare);

	// For every vertex, only keep closest neighbor from each cutout
	nvidia::createIndexStartLookup(lookup, 0, vertexCount, &pairs.begin()->i0, pairs.size(), sizeof(nvidia::IntPair));
	for (uint32_t i = 0; i < vertexCount; ++i)
	{
		const uint32_t start = lookup[i];
		const uint32_t stop = lookup[i + 1];
		if (start == stop)
		{
			continue;
		}
		const CutoutVert& cutoutVert0 = cutoutMap[(uint32_t)pairs[start].i0];
		const physx::PxVec3& vert0 = cutoutSet.cutouts[(uint32_t)cutoutVert0.cutoutIndex].vertices[(uint32_t)cutoutVert0.vertIndex];
		uint32_t groupStart = start;
		while (groupStart < stop)
		{
			uint32_t next = groupStart;
			const CutoutVert& cutoutVert1 = cutoutMap[(uint32_t)pairs[next].i1];
			int32_t currentOtherCutoutIndex = cutoutVert1.cutoutIndex;
			const physx::PxVec3& vert1 = cutoutSet.cutouts[(uint32_t)currentOtherCutoutIndex].vertices[(uint32_t)cutoutVert1.vertIndex];
			uint32_t keep = groupStart;
			float minDist2 = (vert0 - vert1).magnitudeSquared();
			while (++next < stop)
			{
				const CutoutVert& cutoutVertNext = cutoutMap[(uint32_t)pairs[next].i1];
				if (currentOtherCutoutIndex != cutoutVertNext.cutoutIndex)
				{
					break;
				}
				const physx::PxVec3& vertNext = cutoutSet.cutouts[(uint32_t)cutoutVertNext.cutoutIndex].vertices[(uint32_t)cutoutVertNext.vertIndex];
				const float dist2 = (vert0 - vertNext).magnitudeSquared();
				if (dist2 < minDist2)
				{
					pairs[keep].set(-1, -1);	// Invalidate
					keep = next;
					minDist2 = dist2;
				}
				else
				{
					pairs[next].set(-1, -1);	// Invalidate
				}
			}
			groupStart = next;
		}
	}

	// Eliminate invalid pairs (compactify)
	uint32_t pairCount = 0;
	for (uint32_t i = 0; i < pairs.size(); ++i)
	{
		if (pairs[i].i0 >= 0 && pairs[i].i1 >= 0)
		{
			pairs[pairCount++] = pairs[i];
		}
	}
	pairs.resize(pairCount);

	// Snap points together
	physx::Array<bool> pinned;
	pinned.resize(vertexCount);
	memset(pinned.begin(), 0, pinned.size()*sizeof(bool));

	for (uint32_t i = 0; i < pairCount; ++i)
	{
		const uint32_t i0 = (uint32_t)pairs[i].i0;
		bool& pinned0 = pinned[i0];
		if (pinned0)
		{
			continue;
		}
		const CutoutVert& cutoutVert0 = cutoutMap[i0];
		physx::PxVec3& vert0 = cutoutSet.cutouts[(uint32_t)cutoutVert0.cutoutIndex].vertices[(uint32_t)cutoutVert0.vertIndex];
		const uint32_t i1 = (uint32_t)pairs[i].i1;
		bool& pinned1 = pinned[i1];
		const CutoutVert& cutoutVert1 = cutoutMap[i1];
		physx::PxVec3& vert1 = cutoutSet.cutouts[(uint32_t)cutoutVert1.cutoutIndex].vertices[(uint32_t)cutoutVert1.vertIndex];
		const physx::PxVec3 disp = vert1 - vert0;
		// Move and pin
		pinned0 = true;
		if (pinned1)
		{
			vert0 = vert1;
		}
		else
		{
			vert0 += 0.5f * disp;
			vert1 = vert0;
			pinned1 = true;
		}
	}
}
#endif

static void eliminateStraightAngles(nvidia::CutoutSetImpl& cutoutSet)
{
	// Eliminate straight angles
	for (uint32_t i = 0; i < cutoutSet.cutouts.size(); ++i)
	{
		nvidia::Cutout& cutout = cutoutSet.cutouts[i];
		uint32_t oldSize;
		do
		{
			oldSize = cutout.vertices.size();
			for (uint32_t j = 0; j < cutout.vertices.size();)
			{
//				if( isOnBorder( cutout.vertices[j], width, height ) )
//				{	// Don't eliminate border vertices
//					++j;
//					continue;
//				}
				if (perpendicularDistanceSquared(cutout.vertices, j) < CUTOUT_DISTANCE_EPS * CUTOUT_DISTANCE_EPS)
				{
					cutout.vertices.remove(j);
				}
				else
				{
					++j;
				}
			}
		}
		while (cutout.vertices.size() != oldSize);
	}
}

static void simplifyCutoutSetImpl(nvidia::CutoutSetImpl& cutoutSet, float threshold, uint32_t width, uint32_t height)
{
	splitTJunctions(cutoutSet, 1.0f);
	mergeVertices(cutoutSet, threshold, width, height);
	eliminateStraightAngles(cutoutSet);
}

static void cleanCutout(nvidia::Cutout& cutout, uint32_t loopIndex, float tolerance)
{
	nvidia::ConvexLoop& loop = cutout.convexLoops[loopIndex];
	const float tolerance2 = tolerance * tolerance;
	uint32_t oldSize;
	do
	{
		oldSize = loop.polyVerts.size();
		uint32_t size = oldSize;
		for (uint32_t i = 0; i < size; ++i)
		{
			nvidia::PolyVert& v0 = loop.polyVerts[(i + size - 1) % size];
			nvidia::PolyVert& v1 = loop.polyVerts[i];
			nvidia::PolyVert& v2 = loop.polyVerts[(i + 1) % size];
			if (perpendicularDistanceSquared(cutout.vertices[v0.index], cutout.vertices[v1.index], cutout.vertices[v2.index]) <= tolerance2)
			{
				loop.polyVerts.remove(i);
				--size;
				--i;
			}
		}
	}
	while (loop.polyVerts.size() != oldSize);
}

static bool decomposeCutoutIntoConvexLoops(nvidia::Cutout& cutout, float cleanupTolerance = 0.0f)
{
	const uint32_t size = cutout.vertices.size();

	if (size < 3)
	{
		return false;
	}

	// Initialize to one loop, which may not be convex
	cutout.convexLoops.resize(1);
	cutout.convexLoops[0].polyVerts.resize(size);

	// See if the winding is ccw:

	// Scale to normalized size to avoid overflows
	physx::PxBounds3 bounds;
	bounds.setEmpty();
	for (uint32_t i = 0; i < size; ++i)
	{
		bounds.include(cutout.vertices[i]);
	}
	physx::PxVec3 center = bounds.getCenter();
	physx::PxVec3 extent = bounds.getExtents();
	if (extent[0] < PX_EPS_F32 || extent[1] < PX_EPS_F32)
	{
		return false;
	}
	const physx::PxVec3 scale(1.0f / extent[0], 1.0f / extent[1], 0.0f);

	// Find "area" (it will only be correct in sign!)
	physx::PxVec3 prevV = (cutout.vertices[size - 1] - center).multiply(scale);
	float area = 0.0f;
	for (uint32_t i = 0; i < size; ++i)
	{
		const physx::PxVec3 v = (cutout.vertices[i] - center).multiply(scale);
		area += crossZ(prevV, v);
		prevV = v;
	}

	if (physx::PxAbs(area) < PX_EPS_F32 * PX_EPS_F32)
	{
		return false;
	}

	const bool ccw = area > 0.0f;

	for (uint32_t i = 0; i < size; ++i)
	{
		nvidia::PolyVert& vert = cutout.convexLoops[0].polyVerts[i];
		vert.index = (uint16_t)(ccw ? i : size - i - 1);
		vert.flags = 0;
	}

	const float cleanupTolerance2 = square(cleanupTolerance);

	// Find reflex vertices
	for (uint32_t i = 0; i < cutout.convexLoops.size();)
	{
		nvidia::ConvexLoop& loop = cutout.convexLoops[i];
		const uint32_t loopSize = loop.polyVerts.size();
		if (loopSize <= 3)
		{
			++i;
			continue;
		}
		uint32_t j = 0;
		for (; j < loopSize; ++j)
		{
			const physx::PxVec3& v0 = cutout.vertices[loop.polyVerts[(j + loopSize - 1) % loopSize].index];
			const physx::PxVec3& v1 = cutout.vertices[loop.polyVerts[j].index];
			const physx::PxVec3& v2 = cutout.vertices[loop.polyVerts[(j + 1) % loopSize].index];
			const physx::PxVec3 e0 = v1 - v0;
			if (crossZ(e0, v2 - v1) < 0.0f)
			{
				// reflex
				break;
			}
		}
		if (j < loopSize)
		{
			// Find a vertex
			float minLen2 = PX_MAX_F32;
			float maxMinDist = -PX_MAX_F32;
			uint32_t kToUse = 0;
			uint32_t mToUse = 2;
			bool cleanSliceFound = false;	// A transversal is parallel with an edge
			for (uint32_t k = 0; k < loopSize; ++k)
			{
				const physx::PxVec3& vkPrev = cutout.vertices[loop.polyVerts[(k + loopSize - 1) % loopSize].index];
				const physx::PxVec3& vk = cutout.vertices[loop.polyVerts[k].index];
				const physx::PxVec3& vkNext = cutout.vertices[loop.polyVerts[(k + 1) % loopSize].index];
				const uint32_t mStop = k ? loopSize : loopSize - 1;
				for (uint32_t m = k + 2; m < mStop; ++m)
				{
					const physx::PxVec3& vmPrev = cutout.vertices[loop.polyVerts[(m + loopSize - 1) % loopSize].index];
					const physx::PxVec3& vm = cutout.vertices[loop.polyVerts[m].index];
					const physx::PxVec3& vmNext = cutout.vertices[loop.polyVerts[(m + 1) % loopSize].index];
					const physx::PxVec3 newEdge = vm - vk;
					if (!directionsXYOrderedCCW(vk - vkPrev, newEdge, vkNext - vk) ||
					        !directionsXYOrderedCCW(vm - vmPrev, -newEdge, vmNext - vm))
					{
						continue;
					}
					const float len2 = newEdge.magnitudeSquared();
					float minDist = PX_MAX_F32;
					for (uint32_t l = 0; l < loopSize; ++l)
					{
						const uint32_t l1 = (l + 1) % loopSize;
						if (l == k || l1 == k || l == m || l1 == m)
						{
							continue;
						}
						const physx::PxVec3& vl = cutout.vertices[loop.polyVerts[l].index];
						const physx::PxVec3& vl1 = cutout.vertices[loop.polyVerts[l1].index];
						const float dist = segmentsIntersectXY(vl, vl1 - vl, vk, newEdge);
						if (dist < minDist)
						{
							minDist = dist;
						}
					}
					if (minDist <= 0.0f)
					{
						if (minDist > maxMinDist)
						{
							maxMinDist = minDist;
							kToUse = k;
							mToUse = m;
						}
					}
					else
					{
						if (perpendicularDistanceSquared(vkPrev, vk, vm) <= cleanupTolerance2 ||
						        perpendicularDistanceSquared(vk, vm, vmNext) <= cleanupTolerance2)
						{
							if (!cleanSliceFound)
							{
								minLen2 = len2;
								kToUse = k;
								mToUse = m;
							}
							else
							{
								if (len2 < minLen2)
								{
									minLen2 = len2;
									kToUse = k;
									mToUse = m;
								}
							}
							cleanSliceFound = true;
						}
						else if (!cleanSliceFound && len2 < minLen2)
						{
							minLen2 = len2;
							kToUse = k;
							mToUse = m;
						}
					}
				}
			}
			nvidia::ConvexLoop& newLoop = cutout.convexLoops.insert();
			nvidia::ConvexLoop& oldLoop = cutout.convexLoops[i];
			newLoop.polyVerts.resize(mToUse - kToUse + 1);
			for (uint32_t n = 0; n <= mToUse - kToUse; ++n)
			{
				newLoop.polyVerts[n] = oldLoop.polyVerts[kToUse + n];
			}
			newLoop.polyVerts[mToUse - kToUse].flags = 1;	// Mark this vertex (and edge that follows) as a split edge
			oldLoop.polyVerts[kToUse].flags = 1;	// Mark this vertex (and edge that follows) as a split edge
			oldLoop.polyVerts.removeRange(kToUse + 1, (mToUse - (kToUse + 1)));
			if (cleanupTolerance > 0.0f)
			{
				cleanCutout(cutout, i, cleanupTolerance);
				cleanCutout(cutout, cutout.convexLoops.size() - 1, cleanupTolerance);
			}
		}
		else
		{
			if (cleanupTolerance > 0.0f)
			{
				cleanCutout(cutout, i, cleanupTolerance);
			}
			++i;
		}
	}

	return true;
}

static void traceRegion(physx::Array<POINT2D>& trace, Map2d<uint32_t>& regions, Map2d<uint8_t>& pathCounts, uint32_t regionIndex, const POINT2D& startPoint)
{
	POINT2D t = startPoint;
	trace.reset();
	trace.pushBack(t);
	++pathCounts(t.x, t.y);	// Increment path count
	// Find initial path direction
	int32_t dirN;
	for (dirN = 1; dirN < 8; ++dirN)
	{
		const POINT2D t1 = POINT2D(t.x + taxicabSine(dirN + 2), t.y + taxicabSine(dirN));
		if (regions(t1.x, t1.y) != regionIndex)
		{
			break;
		}
	}
	bool done = false;
	do
	{
		for (int32_t i = 1; i < 8; ++i)	// Skip direction we just came from
		{
			--dirN;
			const POINT2D t1 = POINT2D(t.x + taxicabSine(dirN + 2), t.y + taxicabSine(dirN));
			if (regions(t1.x, t1.y) != regionIndex)
			{
				if (t1.x == trace[0].x && t1.y == trace[0].y)
				{
					done = true;
					break;
				}
				trace.pushBack(t1);
				t = t1;
				++pathCounts(t.x, t.y);	// Increment path count
				dirN += 4;
				break;
			}
		}
	}
	while (!done);
}

static void createCutoutSet(nvidia::CutoutSetImpl& cutoutSet, const uint8_t* pixelBuffer, uint32_t bufferWidth, uint32_t bufferHeight, float snapThreshold, bool periodic)
{
	cutoutSet.cutouts.reset();
	cutoutSet.periodic = periodic;
	cutoutSet.dimensions = physx::PxVec2((float)bufferWidth, (float)bufferHeight);

	if (!periodic)
	{
		cutoutSet.dimensions[0] += 1.0f;
		cutoutSet.dimensions[1] += 1.0f;
	}

	if (pixelBuffer == NULL || bufferWidth == 0 || bufferHeight == 0)
	{
		return;
	}

	const int borderPad = periodic ? 0 : 2;	// Padded for borders if not periodic
	const int originCoord = periodic ? 0 : 1;

	BitMap map(bufferWidth + borderPad, bufferHeight + borderPad, 0);
	map.setOrigin((uint32_t)originCoord, (uint32_t)originCoord);

	for (uint32_t y = 0; y < bufferHeight; ++y)
	{
		for (uint32_t x = 0; x < bufferWidth; ++x)
		{
			const uint32_t pix = 5033165 * (uint32_t)pixelBuffer[0] + 9898557 * (uint32_t)pixelBuffer[1] + 1845494 * (uint32_t)pixelBuffer[2];
			pixelBuffer += 3;
			if ((pix >> 28) != 0)
			{
				map.set((int32_t)x, (int32_t)y);
			}
		}
	}

	// Add borders if not tiling
	if (!periodic)
	{
		for (int32_t x = -1; x <= (int32_t)bufferWidth; ++x)
		{
			map.set(x, -1);
			map.set(x, (int32_t)bufferHeight);
		}
		for (int32_t y = -1; y <= (int32_t)bufferHeight; ++y)
		{
			map.set(-1, y);
			map.set((int32_t)bufferWidth, y);
		}
	}

	// Now search for regions

	// Create a region map
	Map2d<uint32_t> regions(bufferWidth + borderPad, bufferHeight + borderPad, 0xFFFFFFFF);	// Initially an invalid value
	regions.setOrigin((uint32_t)originCoord, (uint32_t)originCoord);

	// Create a path counting map
	Map2d<uint8_t> pathCounts(bufferWidth + borderPad, bufferHeight + borderPad, 0);
	pathCounts.setOrigin((uint32_t)originCoord, (uint32_t)originCoord);

	// Bump path counts on borders
	if (!periodic)
	{
		for (int32_t x = -1; x <= (int32_t)bufferWidth; ++x)
		{
			pathCounts(x, -1) = 1;
			pathCounts(x, (int32_t)bufferHeight) = 1;
		}
		for (int32_t y = -1; y <= (int32_t)bufferHeight; ++y)
		{
			pathCounts(-1, y) = 1;
			pathCounts((int32_t)bufferWidth, y) = 1;
		}
	}

	physx::Array<POINT2D> stack;
	physx::Array<POINT2D> traceStarts;
	physx::Array< physx::Array<POINT2D>* > traces;

	// Initial fill of region maps and path maps
	for (int32_t y = 0; y < (int32_t)bufferHeight; ++y)
	{
		for (int32_t x = 0; x < (int32_t)bufferWidth; ++x)
		{
			if (map.read(x-1, y) && !map.read(x, y))
			{
				// Found an empty spot next to a filled spot
				POINT2D t(x - 1, y);
				const uint32_t regionIndex = traceStarts.size();
				traceStarts.pushBack(t);	// Save off initial point
				traces.insert();	// This must be the same size as traceStarts
				traces.back() = (physx::Array<POINT2D>*)PX_ALLOC(sizeof(physx::Array<POINT2D>), PX_DEBUG_EXP("CutoutPoint2DSet"));
				new(traces.back()) physx::Array<POINT2D>;
				// Flood fill region map
				stack.pushBack(POINT2D(x, y));
				do
				{
					const POINT2D s = stack.back();
					stack.popBack();
					map.set(s.x, s.y);
					regions(s.x, s.y) = regionIndex;
					POINT2D n;
					for (int32_t i = 0; i < 4; ++i)
					{
						const int32_t i0 = i & 1;
						const int32_t i1 = (i >> 1) & 1;
						n.x = s.x + i0 - i1;
						n.y = s.y + i0 + i1 - 1;
						if (!map.read(n.x, n.y))
						{
							stack.pushBack(n);
						}
					}
				}
				while (stack.size());
				// Trace region
				PX_ASSERT(map.read(t.x, t.y));
				physx::Array<POINT2D>& trace = *traces[regionIndex];
				traceRegion(trace, regions, pathCounts, regionIndex, t);
			}
		}
	}

	uint32_t cutoutCount = traces.size();

	// Now expand regions until the paths completely overlap
	bool somePathChanged;
	int sanityCounter = 1000;
	bool abort = false;
	do
	{
		somePathChanged = false;
		for (uint32_t i = 0; i < cutoutCount; ++i)
		{
			bool pathChanged = false;
			physx::Array<POINT2D>& trace = *traces[i];
			for (uint32_t j = 0; j < trace.size(); ++j)
			{
				const POINT2D& t = trace[j];
				if (pathCounts(t.x, t.y) == 1)
				{
					regions(t.x, t.y) = i;
					pathChanged = true;
				}
			}
			if (pathChanged)
			{
				// Recalculate cutout
				// Decrement pathCounts
				for (uint32_t j = 0; j < trace.size(); ++j)
				{
					const POINT2D& t = trace[j];
					--pathCounts(t.x, t.y);
				}
				// Erase trace
				// Calculate new start point
				POINT2D& t = traceStarts[i];
				int stop = (int)cutoutSet.dimensions.x;
				while (regions(t.x, t.y) == i)
				{
					--t.x;
					if(--stop < 0)
					{
						// There is an error; abort
						break;
					}
				}
				if(stop < 0)
				{
					// Release traces and abort
					abort = true;
					somePathChanged = false;
					break;
				}
				traceRegion(trace, regions, pathCounts, i, t);
				somePathChanged = true;
			}
		}
		if (--sanityCounter <= 0)
		{
			abort = true;
			break;
		}
	}
	while (somePathChanged);

	if (abort)
	{
		for (uint32_t i = 0; i < cutoutCount; ++i)
		{
			traces[i]->~Array<POINT2D>();
			PX_FREE(traces[i]);
		}
		cutoutCount = 0;
	}

	// Create cutouts
	cutoutSet.cutouts.resize(cutoutCount);
	for (uint32_t i = 0; i < cutoutCount; ++i)
	{
		createCutout(cutoutSet.cutouts[i], *traces[i], snapThreshold, bufferWidth, bufferHeight, !cutoutSet.periodic);
	}

	simplifyCutoutSetImpl(cutoutSet, snapThreshold, bufferWidth, bufferHeight);

	// Release traces
	for (uint32_t i = 0; i < cutoutCount; ++i)
	{
		traces[i]->~Array<POINT2D>();
		PX_FREE(traces[i]);
	}

	// Decompose each cutout in the set into convex loops
	uint32_t cutoutSetSize = 0;
	for (uint32_t i = 0; i < cutoutSet.cutouts.size(); ++i)
	{
		bool success = decomposeCutoutIntoConvexLoops(cutoutSet.cutouts[i]);
		if (success)
		{
			if (cutoutSetSize != i)
			{
				cutoutSet.cutouts[cutoutSetSize] = cutoutSet.cutouts[i];
			}
			++cutoutSetSize;
		}
	}
	cutoutSet.cutouts.resize(cutoutSetSize);
}

class Matrix22
{
public:
	//! Default constructor
	Matrix22()
	{}

	//! Construct from two base vectors
	Matrix22(const physx::PxVec2& col0, const physx::PxVec2& col1)
		: column0(col0), column1(col1)
	{}

	//! Construct from float[4]
	explicit Matrix22(float values[]):
		column0(values[0],values[1]),
		column1(values[2],values[3])
	{
	}

	//! Copy constructor
	Matrix22(const Matrix22& other)
		: column0(other.column0), column1(other.column1)
	{}

	//! Assignment operator
	Matrix22& operator=(const Matrix22& other)
	{
		column0 = other.column0;
		column1 = other.column1;
		return *this;
	}

	//! Set to identity matrix
	static Matrix22 createIdentity()
	{
		return Matrix22(physx::PxVec2(1,0), physx::PxVec2(0,1));
	}

	//! Set to zero matrix
	static Matrix22 createZero()
	{
		return Matrix22(physx::PxVec2(0.0f), physx::PxVec2(0.0f));
	}

	//! Construct from diagonal, off-diagonals are zero.
	static Matrix22 createDiagonal(const physx::PxVec2& d)
	{
		return Matrix22(physx::PxVec2(d.x,0.0f), physx::PxVec2(0.0f,d.y));
	}


	//! Get transposed matrix
	Matrix22 getTranspose() const
	{
		const physx::PxVec2 v0(column0.x, column1.x);
		const physx::PxVec2 v1(column0.y, column1.y);

		return Matrix22(v0,v1);   
	}

	//! Get the real inverse
	Matrix22 getInverse() const
	{
		const float det = getDeterminant();
		Matrix22 inverse;

		if(det != 0)
		{
			const float invDet = 1.0f/det;

			inverse.column0[0] = invDet * column1[1];						
			inverse.column0[1] = invDet * (-column0[1]);

			inverse.column1[0] = invDet * (-column1[0]);
			inverse.column1[1] = invDet * column0[0];

			return inverse;
		}
		else
		{
			return createIdentity();
		}
	}

	//! Get determinant
	float getDeterminant() const
	{
		return column0[0] * column1[1] - column0[1] * column1[0];
	}

	//! Unary minus
	Matrix22 operator-() const
	{
		return Matrix22(-column0, -column1);
	}

	//! Add
	Matrix22 operator+(const Matrix22& other) const
	{
		return Matrix22( column0+other.column0,
					  column1+other.column1);
	}

	//! Subtract
	Matrix22 operator-(const Matrix22& other) const
	{
		return Matrix22( column0-other.column0,
					  column1-other.column1);
	}

	//! Scalar multiplication
	Matrix22 operator*(float scalar) const
	{
		return Matrix22(column0*scalar, column1*scalar);
	}
	
	//! Matrix vector multiplication (returns 'this->transform(vec)')
	physx::PxVec2 operator*(const physx::PxVec2& vec) const
	{
		return transform(vec);
	}

	//! Matrix multiplication
	Matrix22 operator*(const Matrix22& other) const
	{
		//Rows from this <dot> columns from other
		//column0 = transform(other.column0) etc
		return Matrix22(transform(other.column0), transform(other.column1));
	}

	// a <op>= b operators

	//! Equals-add
	Matrix22& operator+=(const Matrix22& other)
	{
		column0 += other.column0;
		column1 += other.column1;
		return *this;
	}

	//! Equals-sub
	Matrix22& operator-=(const Matrix22& other)
	{
		column0 -= other.column0;
		column1 -= other.column1;
		return *this;
	}

	//! Equals scalar multiplication
	Matrix22& operator*=(float scalar)
	{
		column0 *= scalar;
		column1 *= scalar;
		return *this;
	}

	//! Element access, mathematical way!
	float operator()(unsigned int row, unsigned int col) const
	{
		return (*this)[col][(int)row];
	}

	//! Element access, mathematical way!
	float& operator()(unsigned int row, unsigned int col)
	{
		return (*this)[col][(int)row];
	}

	// Transform etc
	
	//! Transform vector by matrix, equal to v' = M*v
	physx::PxVec2 transform(const physx::PxVec2& other) const
	{
		return column0*other.x + column1*other.y;
	}

	physx::PxVec2& operator[](unsigned int num)			{return (&column0)[num];}
	const	physx::PxVec2& operator[](unsigned int num) const	{return (&column0)[num];}

	//Data, see above for format!

	physx::PxVec2 column0, column1; //the two base vectors
};

PX_INLINE bool calculateUVMapping(const nvidia::ExplicitRenderTriangle& triangle, physx::PxMat33& theResultMapping)
{
	physx::PxMat33 rMat;
	physx::PxMat33 uvMat;
	for (unsigned col = 0; col < 3; ++col)
	{
		rMat[col] = triangle.vertices[col].position;
		uvMat[col] = physx::PxVec3(triangle.vertices[col].uv[0][0], triangle.vertices[col].uv[0][1], 1.0f);
	}

	if (uvMat.getDeterminant() == 0.0f)
	{
		return false;
	}

	theResultMapping = rMat*uvMat.getInverse();

	return true;
}

static bool calculateUVMapping(nvidia::ExplicitHierarchicalMesh& theHMesh, const physx::PxVec3& theDir, physx::PxMat33& theResultMapping)
{	
	physx::PxVec3 cutoutDir( theDir );
	cutoutDir.normalize( );

	const float cosineThreshold = physx::PxCos(3.141593f / 180);	// 1 degree

	nvidia::ExplicitRenderTriangle* triangleToUse = NULL;
	float greatestCosine = -PX_MAX_F32;
	float greatestArea = 0.0f;	// for normals within the threshold
	for ( uint32_t partIndex = 0; partIndex < theHMesh.partCount(); ++partIndex )
	{
		nvidia::ExplicitRenderTriangle* theTriangles = theHMesh.meshTriangles( partIndex );
		uint32_t triangleCount = theHMesh.meshTriangleCount( partIndex );
		for ( uint32_t tIndex = 0; tIndex < triangleCount; ++tIndex )
		{			
			nvidia::ExplicitRenderTriangle& theTriangle = theTriangles[tIndex];
			physx::PxVec3 theEdge1 = theTriangle.vertices[1].position - theTriangle.vertices[0].position;
			physx::PxVec3 theEdge2 = theTriangle.vertices[2].position - theTriangle.vertices[0].position;
			physx::PxVec3 theNormal = theEdge1.cross( theEdge2 );
			float theArea = theNormal.normalize();	// twice the area, but that's ok

			if (theArea == 0.0f)
			{
				continue;
			}

			const float cosine = cutoutDir.dot(theNormal);

			if (cosine < cosineThreshold)
			{
				if (cosine > greatestCosine && greatestArea == 0.0f)
				{
					greatestCosine = cosine;
					triangleToUse = &theTriangle;
				}
			}
			else
			{
				if (theArea > greatestArea)
				{
					greatestArea = theArea;
					triangleToUse = &theTriangle;
				}
			}
		}
	}

	if (triangleToUse == NULL)
	{
		return false;
	}

	return calculateUVMapping(*triangleToUse, theResultMapping);
}

namespace nvidia
{
namespace apex
{

PX_INLINE void serialize(physx::PxFileBuf& stream, const PolyVert& p)
{
	stream << p.index << p.flags;
}

PX_INLINE void deserialize(physx::PxFileBuf& stream, uint32_t version, PolyVert& p)
{
	// original version
	PX_UNUSED(version);
	stream >> p.index >> p.flags;
}

PX_INLINE void serialize(physx::PxFileBuf& stream, const ConvexLoop& l)
{
	apex::serialize(stream, l.polyVerts);
}

PX_INLINE void deserialize(physx::PxFileBuf& stream, uint32_t version, ConvexLoop& l)
{
	// original version
	apex::deserialize(stream, version, l.polyVerts);
}

PX_INLINE void serialize(physx::PxFileBuf& stream, const Cutout& c)
{
	apex::serialize(stream, c.vertices);
	apex::serialize(stream, c.convexLoops);
}

PX_INLINE void deserialize(physx::PxFileBuf& stream, uint32_t version, Cutout& c)
{
	// original version
	apex::deserialize(stream, version, c.vertices);
	apex::deserialize(stream, version, c.convexLoops);
}

void CutoutSetImpl::serialize(physx::PxFileBuf& stream) const
{
	stream << (uint32_t)Current;

	apex::serialize(stream, cutouts);
}

void CutoutSetImpl::deserialize(physx::PxFileBuf& stream)
{
	const uint32_t version = stream.readDword();

	apex::deserialize(stream, version, cutouts);
}

}
} // end namespace nvidia::apex

namespace FractureTools
{
CutoutSet* createCutoutSet()
{
	return new nvidia::CutoutSetImpl();
}

void buildCutoutSet(CutoutSet& cutoutSet, const uint8_t* pixelBuffer, uint32_t bufferWidth, uint32_t bufferHeight, float snapThreshold, bool periodic)
{
	::createCutoutSet(*(nvidia::CutoutSetImpl*)&cutoutSet, pixelBuffer, bufferWidth, bufferHeight, snapThreshold, periodic);
}

bool calculateCutoutUVMapping(nvidia::ExplicitHierarchicalMesh& hMesh, const physx::PxVec3& targetDirection, physx::PxMat33& theMapping)
{
	return ::calculateUVMapping(hMesh, targetDirection, theMapping);
}

bool calculateCutoutUVMapping(const nvidia::ExplicitRenderTriangle& targetDirection, physx::PxMat33& theMapping)
{
	return ::calculateUVMapping(targetDirection, theMapping);
}
} // namespace FractureTools

#endif

#ifdef _MANAGED
#pragma managed(pop)
#endif
