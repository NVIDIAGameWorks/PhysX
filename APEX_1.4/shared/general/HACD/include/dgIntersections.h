/* Copyright (c) <2003-2011> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __dgIntersections__
#define __dgIntersections__

#include "dgTypes.h"
#include "dgVector.h"


class dgPlane;
class dgObject;
class dgPolyhedra;

class dgFastRayTest
{
	public:
	dgFastRayTest(const dgVector& l0, const dgVector& l1);

	int32_t BoxTest (const dgVector& minBox, const dgVector& maxBox) const;

	float PolygonIntersect (const dgVector& normal, const float* const polygon, int32_t strideInBytes, const int32_t* const indexArray, int32_t indexCount) const;

	void Reset (float t) 
	{
		m_dpInv = m_dpBaseInv.Scale (float (1.0f) / t);
	}

	dgVector m_p0;
	dgVector m_p1;
	dgVector m_diff;
	dgVector m_dpInv;
	dgVector m_dpBaseInv;
	dgVector m_minT;
	dgVector m_maxT;

	dgVector m_tolerance;
	dgVector m_zero;
	int32_t m_isParallel[4];

	float m_dirError;
};



enum dgIntersectStatus
{
	t_StopSearh,
	t_ContinueSearh
};

typedef dgIntersectStatus (*dgAABBIntersectCallback) (void *context, 
													  const float* const polygon, int32_t strideInBytes,
											          const int32_t* const indexArray, int32_t indexCount);

typedef float (*dgRayIntersectCallback) (void *context, 
											 const float* const polygon, int32_t strideInBytes,
											 const int32_t* const indexArray, int32_t indexCount);


class dgBeamHitStruct
{
	public:
	dgVector m_Origin; 
	dgVector m_NormalOut;
	dgObject* m_HitObjectOut; 
	float m_ParametricIntersctionOut;
};


bool  dgRayBoxClip(dgVector& ray_p0, dgVector& ray_p1, const dgVector& boxP0, const dgVector& boxP1); 

dgVector  dgPointToRayDistance(const dgVector& point, const dgVector& ray_p0, const dgVector& ray_p1); 

void  dgRayToRayDistance (const dgVector& ray_p0, 
							   const dgVector& ray_p1,
							   const dgVector& ray_q0, 
							   const dgVector& ray_q1,
							   dgVector& p0Out, 
							   dgVector& p1Out); 

dgVector    dgPointToTriangleDistance (const dgVector& point, const dgVector& p0, const dgVector& p1, const dgVector& p2);
dgBigVector dgPointToTriangleDistance (const dgBigVector& point, const dgBigVector& p0, const dgBigVector& p1, const dgBigVector& p2);

bool   dgPointToPolygonDistance (const dgVector& point, const float* const polygon,  int32_t strideInInBytes,   
									  const int32_t* const indexArray, int32_t indexCount, float bailOutDistance, dgVector& pointOut);   


/*
int32_t  dgConvexPolyhedraToPlaneIntersection (
							const dgPolyhedra& convexPolyhedra,
							const float polyhedraVertexArray[],
							int polyhedraVertexStrideInBytes,
							const dgPlane& plane,
							dgVector* intersectionOut,
							int32_t maxSize);


int32_t  ConvecxPolygonToConvexPolygonIntersection (
							int32_t polygonIndexCount_1,
							const int32_t polygonIndexArray_1[],
						   int32_t convexPolygonStrideInBytes_1,   
							const float convexPolygonVertex_1[], 
							int32_t polygonIndexCount_2,
							const int32_t polygonIndexArray_2[],
						   int32_t convexPolygonStrideInBytes_2,   
							const float convexPolygonVertex_2[], 
							dgVector* intersectionOut,
							int32_t maxSize);


int32_t  dgConvexIntersection (const dgVector convexPolyA[], int32_t countA,
																 const dgVector convexPolyB[], int32_t countB,
																 dgVector output[]);

*/


HACD_FORCE_INLINE int32_t dgOverlapTest (const dgVector& p0, const dgVector& p1, const dgVector& q0, const dgVector& q1)
{
	return ((p0.m_x < q1.m_x) && (p1.m_x > q0.m_x) && (p0.m_z < q1.m_z) && (p1.m_z > q0.m_z) && (p0.m_y < q1.m_y) && (p1.m_y > q0.m_y)); 
}


HACD_FORCE_INLINE int32_t dgBoxInclusionTest (const dgVector& p0, const dgVector& p1, const dgVector& q0, const dgVector& q1)
{
	return (p0.m_x >= q0.m_x) && (p0.m_y >= q0.m_y) && (p0.m_z >= q0.m_z) && (p1.m_x <= q1.m_x) && (p1.m_y <= q1.m_y) && (p1.m_z <= q1.m_z);
}


HACD_FORCE_INLINE int32_t dgCompareBox (const dgVector& p0, const dgVector& p1, const dgVector& q0, const dgVector& q1)
{
	return (p0.m_x != q0.m_x) || (p0.m_y != q0.m_y) || (p0.m_z != q0.m_z) || (p1.m_x != q1.m_x) || (p1.m_y != q1.m_y) || (p1.m_z != q1.m_z);
}



dgBigVector LineTriangleIntersection (const dgBigVector& l0, const dgBigVector& l1, const dgBigVector& A, const dgBigVector& B, const dgBigVector& C);

#endif

