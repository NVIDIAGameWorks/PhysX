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

#include "dgTypes.h"
#include "dgVector.h"
#include "dgStack.h"
#include <string.h>

void GetMinMax (dgVector &minOut, dgVector &maxOut, const float* const vertexArray, int32_t vCount, int32_t strideInBytes)
{
	int32_t stride = int32_t (strideInBytes / sizeof (float));
	const float* vArray = vertexArray + stride;

	HACD_ASSERT (stride >= 3);
 	minOut = dgVector (vertexArray[0], vertexArray[1], vertexArray[2], float (0.0f)); 
	maxOut = dgVector (vertexArray[0], vertexArray[1], vertexArray[2], float (0.0f)); 

	for (int32_t i = 1; i < vCount; i ++) {
		minOut.m_x = GetMin (minOut.m_x, vArray[0]);
		minOut.m_y = GetMin (minOut.m_y, vArray[1]);
		minOut.m_z = GetMin (minOut.m_z, vArray[2]);

		maxOut.m_x = GetMax (maxOut.m_x, vArray[0]);
		maxOut.m_y = GetMax (maxOut.m_y, vArray[1]);
		maxOut.m_z = GetMax (maxOut.m_z, vArray[2]);

		vArray += stride;
	}
}


void GetMinMax (dgBigVector &minOut, dgBigVector &maxOut, const double* const vertexArray, int32_t vCount, int32_t strideInBytes)
{
	int32_t stride = int32_t (strideInBytes / sizeof (double));
	const double* vArray = vertexArray + stride;

	HACD_ASSERT (stride >= 3);
	minOut = dgBigVector (vertexArray[0], vertexArray[1], vertexArray[2], double (0.0f)); 
	maxOut = dgBigVector (vertexArray[0], vertexArray[1], vertexArray[2], double (0.0f)); 

	for (int32_t i = 1; i < vCount; i ++) {
		minOut.m_x = GetMin (minOut.m_x, vArray[0]);
		minOut.m_y = GetMin (minOut.m_y, vArray[1]);
		minOut.m_z = GetMin (minOut.m_z, vArray[2]);

		maxOut.m_x = GetMax (maxOut.m_x, vArray[0]);
		maxOut.m_y = GetMax (maxOut.m_y, vArray[1]);
		maxOut.m_z = GetMax (maxOut.m_z, vArray[2]);

		vArray += stride;
	}
}



static inline int32_t cmp_vertex (const double* const v1, const double* const v2, int32_t firstSortAxis)
	{
		if (v1[firstSortAxis] < v2[firstSortAxis]) {
			return -1;
		}

		if (v1[firstSortAxis] > v2[firstSortAxis]){
			return 1;
		}

		return 0;
	}
	
static int32_t SortVertices (double* const vertexList,  int32_t stride, int32_t compareCount, int32_t vertexCount, double tolerance)
	{
		double xc = 0;
		double yc = 0;
		double zc = 0;
		double x2c = 0;
		double y2c = 0;
		double z2c = 0;

		dgBigVector minP (1e10, 1e10, 1e10, 0);
		dgBigVector maxP (-1e10, -1e10, -1e10, 0);
		int32_t k = 0;
		for (int32_t i = 0; i < vertexCount; i ++) {
			double x  = vertexList[k + 2];
			double y  = vertexList[k + 3];
			double z  = vertexList[k + 4];
			k += stride;

			xc += x;
			yc += y;
			zc += z;
			x2c += x * x;
			y2c += y * y; 
			z2c += z * z;
	
			if (x < minP.m_x) {
				minP.m_x = x; 
			}
			if (y < minP.m_y) {
				minP.m_y = y; 
			}
	
			if (z < minP.m_z) {
				minP.m_z = z; 
			}
	
			if (x > maxP.m_x) {
				maxP.m_x = x; 
			}
			if (y > maxP.m_y) {
				maxP.m_y = y; 
			}
	
			if (z > maxP.m_z) {
				maxP.m_z = z; 
			}
		}
	
		dgBigVector del (maxP - minP);
		double minDist = GetMin (del.m_x, del.m_y, del.m_z);
		if (minDist < 1.0e-3) {
			minDist = 1.0e-3;
		}
	
	double tol = tolerance * minDist + 1.0e-12f;
		double sweptWindow = 2.0 * tol;
		sweptWindow += 1.0e-4;
	
		x2c = vertexCount * x2c - xc * xc;
		y2c = vertexCount * y2c - yc * yc;
		z2c = vertexCount * z2c - zc * zc;

		int32_t firstSortAxis = 2;
		if ((y2c >= x2c) && (y2c >= z2c)) {
			firstSortAxis = 3;
		} else if ((z2c >= x2c) && (z2c >= y2c)) {
			firstSortAxis = 4;
		}


		int32_t stack[1024][2];
		stack[0][0] = 0;
		stack[0][1] = vertexCount - 1;
		int32_t stackIndex = 1;
		while (stackIndex) {
			stackIndex --;
			int32_t lo = stack[stackIndex][0];
			int32_t hi = stack[stackIndex][1];
			if ((hi - lo) > 8) {
				int32_t i = lo;
				int32_t j = hi;
			double val[64]; 
			memcpy (val, &vertexList[((lo + hi) >> 1) * stride], stride * sizeof (double));
				do {    
					while (cmp_vertex (&vertexList[i * stride], val, firstSortAxis) < 0) i ++;
					while (cmp_vertex (&vertexList[j * stride], val, firstSortAxis) > 0) j --;

					if (i <= j)	{
					double tmp[64]; 
					memcpy (tmp, &vertexList[i * stride], stride * sizeof (double));
					memcpy (&vertexList[i * stride], &vertexList[j * stride], stride * sizeof (double)); 
					memcpy (&vertexList[j * stride], tmp, stride * sizeof (double)); 
						i++; 
						j--;
					}
				} while (i <= j);

				if (i < hi) {
					stack[stackIndex][0] = i;
					stack[stackIndex][1] = hi;
					stackIndex ++;
				}
				if (lo < j) {
					stack[stackIndex][0] = lo;
					stack[stackIndex][1] = j;
					stackIndex ++;
				}
				HACD_ASSERT (stackIndex < int32_t (sizeof (stack) / (2 * sizeof (stack[0][0]))));
			} else {
				for (int32_t i = lo + 1; i <= hi ; i++) {
				double tmp[64]; 
				memcpy (tmp, &vertexList[i * stride], stride * sizeof (double));

				int32_t j = i;
				for (; j && (cmp_vertex (&vertexList[(j - 1) * stride], tmp, firstSortAxis) > 0); j --) {
					memcpy (&vertexList[j * stride], &vertexList[(j - 1)* stride], stride * sizeof (double));
					}
				memcpy (&vertexList[j * stride], tmp, stride * sizeof (double)); 
				}
			}
		}


#ifdef _DEBUG
		for (int32_t i = 0; i < (vertexCount - 1); i ++) {
			HACD_ASSERT (cmp_vertex (&vertexList[i * stride], &vertexList[(i + 1) * stride], firstSortAxis) <= 0);
		}
#endif

		int32_t count = 0;
		for (int32_t i = 0; i < vertexCount; i ++) {
			int32_t m = i * stride;
		int32_t index = int32_t (vertexList[m + 0]);
			if (index == int32_t (0xffffffff)) {
				double swept = vertexList[m + firstSortAxis] + sweptWindow;
				int32_t k = i * stride + stride;
				for (int32_t i1 = i + 1; i1 < vertexCount; i1 ++) {

				index = int32_t (vertexList[k + 0]);
					if (index == int32_t (0xffffffff)) {
						double val = vertexList[k + firstSortAxis];
						if (val >= swept) {
							break;
						}
						bool test = true;
					for (int32_t t = 0; test && (t < compareCount); t ++) {
						double val = fabs (vertexList[m + t + 2] - vertexList[k + t + 2]);
							test = test && (val <= tol);
						}
						if (test) {
						vertexList[k + 0] = double (count);
						}
					}
					k += stride;
				}

			memcpy (&vertexList[count * stride + 2], &vertexList[m + 2], (stride - 2) * sizeof (double));
			vertexList[m + 0] = double (count);
				count ++;
			}
		}
				
		return count;
	}



//static int32_t QuickSortVertices (float* const vertList, int32_t stride, int32_t floatSize, int32_t unsignedSize, int32_t vertexCount, float tolerance)
static int32_t QuickSortVertices (double* const vertList, int32_t stride, int32_t compareCount, int32_t vertexCount, double tolerance)
	{
		int32_t count = 0;
		if (vertexCount > (3 * 1024 * 32)) {
		double x = float (0.0f);
		double y = float (0.0f);
		double z = float (0.0f);
		double xd = float (0.0f);
		double yd = float (0.0f);
		double zd = float (0.0f);
			
			for (int32_t i = 0; i < vertexCount; i ++) {
			double x0 = vertList[i * stride + 2];
			double y0 = vertList[i * stride + 3];
			double z0 = vertList[i * stride + 4];
				x += x0;
				y += y0;
				z += z0;
				xd += x0 * x0;
				yd += y0 * y0;
				zd += z0 * z0;
			}

			xd = vertexCount * xd - x * x;
			yd = vertexCount * yd - y * y;
			zd = vertexCount * zd - z * z;

			int32_t axis = 2;
		        double axisVal = x / vertexCount;
			if ((yd > xd) && (yd > zd)) {
				axis = 3;
				axisVal = y / vertexCount;
			}
			if ((zd > xd) && (zd > yd)) {
				axis = 4;
				axisVal = z / vertexCount;
			}

			int32_t i0 = 0;
			int32_t i1 = vertexCount - 1;
			do {    
				for ( ;vertList[i0 * stride + axis] < axisVal; i0 ++); 
				for ( ;vertList[i1 * stride + axis] > axisVal; i1 --);
				if (i0 <= i1) {
					for (int32_t i = 0; i < stride; i ++) {
						Swap (vertList[i0 * stride + i], vertList[i1 * stride + i]);
					}
					i0 ++; 
					i1 --;
				}
			} while (i0 <= i1);
			HACD_ASSERT (i0 < vertexCount);

		int32_t count0 = QuickSortVertices (&vertList[ 0 * stride], stride, compareCount, i0, tolerance);
		int32_t count1 = QuickSortVertices (&vertList[i0 * stride], stride, compareCount, vertexCount - i0, tolerance);
			
			count = count0 + count1;

			for (int32_t i = 0; i < count1; i ++) {
			memcpy (&vertList[(count0 + i) * stride + 2], &vertList[(i0 + i) * stride + 2], (stride - 2) * sizeof (double));
			}


//		double* const indexPtr = (int64_t*)vertList;
			for (int32_t i = i0; i < vertexCount; i ++) {
//			indexPtr[i * stride] += count0;
			vertList[i * stride] += double (count0);
			}

		} else {
		count = SortVertices (vertList, stride, compareCount, vertexCount, tolerance);
		}

		return count;
	}





int32_t dgVertexListToIndexList (double* const vertList, int32_t strideInBytes, int32_t compareCount, int32_t vertexCount, int32_t* const indexListOut, double tolerance)
{
	if (strideInBytes < 3 * int32_t (sizeof (double))) {
		return 0;
	}
	if (compareCount < 3) {
		return 0;
	}
	HACD_ASSERT (compareCount <= int32_t (strideInBytes / sizeof (double)));
	HACD_ASSERT (strideInBytes == int32_t (sizeof (double) * (strideInBytes / sizeof (double))));

	int32_t stride = strideInBytes / int32_t (sizeof (double));
	int32_t stride2 = stride + 2;

	dgStack<double>pool (stride2  * vertexCount);
	double* const tmpVertexList = &pool[0];

//	int64_t* const indexPtr = (int64_t*)tmpVertexList;

	int32_t k = 0;
	int32_t m = 0;
	for (int32_t i = 0; i < vertexCount; i ++) {
		memcpy (&tmpVertexList[m + 2], &vertList[k], stride * sizeof (double));
		tmpVertexList[m + 0] = double (- 1.0f);
		tmpVertexList[m + 1] = double (i);
		k += stride;
		m += stride2;
	}
	
	int32_t count = QuickSortVertices (tmpVertexList, stride2, compareCount, vertexCount, tolerance);

	k = 0;
	m = 0;
	for (int32_t i = 0; i < count; i ++) {
		k = i * stride;
		m = i * stride2;
		memcpy (&vertList[k], &tmpVertexList[m + 2], stride * sizeof (double));
		k += stride;
		m += stride2;
	}

	m = 0;
	for (int32_t i = 0; i < vertexCount; i ++) {
		int32_t i1 = int32_t (tmpVertexList [m + 1]);
		int32_t index = int32_t (tmpVertexList [m + 0]);
		indexListOut[i1] = index;
		m += stride2;
	}
	return count;
}




int32_t dgVertexListToIndexList (float* const vertList, int32_t strideInBytes, int32_t floatSizeInBytes, int32_t unsignedSizeInBytes, int32_t vertexCount, int32_t* const indexList, float tolerance)
{
	HACD_FORCE_PARAMETER_REFERENCE(unsignedSizeInBytes);
	uint32_t stride = (uint32_t)strideInBytes / sizeof (float);

	HACD_ASSERT (!unsignedSizeInBytes);
	dgStack<double> pool(vertexCount * (int32_t)stride);

	int32_t floatCount = floatSizeInBytes / (int32_t)sizeof (float);

	double* const data = &pool[0];
	for (uint32_t i = 0; i < (uint32_t)vertexCount; i ++) {

		double* const dst = &data[i * stride];
		float* const src = &vertList[i * stride];
		for (uint32_t j = 0; j < stride; j ++) {
			dst[j] = src[j];
		}
	}

	int32_t count = dgVertexListToIndexList (data, (int32_t)(stride * sizeof (double)), floatCount, vertexCount, indexList, double (tolerance));
	for (uint32_t i = 0; i < (uint32_t)count; i ++) {
		double* const src = &data[i * stride];
		float* const dst = &vertList[i * stride];
		for (uint32_t j = 0; j < stride; j ++) {
			dst[j] = float (src[j]);
		}
	}
	
	return count;
}


namespace hacd
{
size_t gAllocCount=0;
size_t gAllocSize=0;
};