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

#ifndef DG_TYPES_H

#define DG_TYPES_H

#include "PlatformConfigHACD.h"

#include <math.h>
#include <float.h>


class dgTriplex
{
	public:
	float m_x;
	float m_y;
	float m_z;
};


typedef uint32_t (*OnGetPerformanceCountCallback) ();

#define DG_MEMORY_GRANULARITY 16

#define dgPI			 	float (3.14159f)
#define dgPI2			 	float (dgPI * 2.0f)
#define dgEXP			 	float (2.71828f)
#define dgEPSILON	  	 	float (1.0e-5f)
#define dgGRAVITY	  	 	float (9.8f)
#define dgDEG2RAD	  	 	float (dgPI / 180.0f)
#define dgRAD2DEG	  	 	float (180.0f / dgPI)
#define dgKMH2MPSEC		 	float (0.278f)


class dgVector;
class dgBigVector;

HACD_INLINE int32_t exp_2 (int32_t x)
{
	int32_t exp;
	for (exp = -1; x; x >>= 1) {
		exp ++;
	}
	return exp;
}

template <class T> HACD_INLINE T ClampValue(T val, T min, T max)
{
	if (val < min) {
		return min;
	}
	if (val > max) {
		return max;
	}
	return val;
}

template <class T> HACD_INLINE T GetMin(T A, T B)
{
	if (B < A) {
		A = B;
	}
	return A;
}

template <class T> HACD_INLINE T GetMax(T A, T B)
{
	if (B > A) {
		A = B;
	}
	return A;
}



template <class T> HACD_INLINE T GetMin(T A, T B, T C)
{
	return GetMin(GetMin (A, B), C);
}



template <class T> HACD_INLINE T GetMax(T A, T B, T C)
{
	return GetMax(GetMax (A, B), C);
}

template <class T> HACD_INLINE void Swap(T& A, T& B)
{
	T tmp (A);
	A = B;
	B = tmp;
}	


template <class T> HACD_INLINE T GetSign(T A)
{
	T sign (1.0f);
	if (A < T (0.0f)) {
		sign = T (-1.0f);
	}
	return sign;
}

template <class T> 
int32_t dgBinarySearch (T const* array, int32_t elements, int32_t entry)
{
	int32_t index0;
	int32_t index1;
	int32_t index2;
	int32_t entry0;
	int32_t entry1;
	int32_t entry2;

	index0 = 0;
	index2 = elements - 1;
	entry0 = array[index0].m_Key;
	entry2 = array[index2].m_Key;

   while ((index2 - index0) > 1) {
      index1 = (index0 + index2) >> 1;
		entry1 = array[index1].m_Key;
		if (entry1 == entry) {
			HACD_ASSERT (array[index1].m_Key <= entry);
			HACD_ASSERT (array[index1 + 1].m_Key >= entry);
			return index1;
		} else if (entry < entry1) {
			index2 = index1;
			entry2 = entry1;
		} else {
			index0 = index1;
			entry0 = entry1;
		}
	}

	if (array[index0].m_Key > index0) {
		index0 --;
	}

	HACD_ASSERT (array[index0].m_Key <= entry);
	HACD_ASSERT (array[index0 + 1].m_Key >= entry);
	return index0;
}




template <class T> 
void dgRadixSort (T* const array, T* const tmpArray, int32_t elements, int32_t radixPass, 
				  int32_t (*getRadixKey) (const T* const  A, void* const context), void* const context = NULL)
{
	int32_t scanCount[256]; 
	int32_t histogram[256][4];

	HACD_ASSERT (radixPass >= 1);
	HACD_ASSERT (radixPass <= 4);
	
	memset (histogram, 0, sizeof (histogram));
	for (int32_t i = 0; i < elements; i ++) {
		int32_t key = getRadixKey (&array[i], context);
		for (int32_t j = 0; j < radixPass; j ++) {
			int32_t radix = (key >> (j << 3)) & 0xff;
			histogram[radix][j] = histogram[radix][j] + 1;
		}
	}

	for (int32_t radix = 0; radix < radixPass; radix += 2) {
		scanCount[0] = 0;
		for (int32_t i = 1; i < 256; i ++) {
			scanCount[i] = scanCount[i - 1] + histogram[i - 1][radix];
		}
		int32_t radixShift = radix << 3;
		for (int32_t i = 0; i < elements; i ++) {
			int32_t key = (getRadixKey (&array[i], context) >> radixShift) & 0xff;
			int32_t index = scanCount[key];
			tmpArray[index] = array[i];
			scanCount[key] = index + 1;
		}

		if ((radix + 1) < radixPass) { 
			scanCount[0] = 0;
			for (int32_t i = 1; i < 256; i ++) {
				scanCount[i] = scanCount[i - 1] + histogram[i - 1][radix + 1];
			}
			
			int32_t radixShift = (radix + 1) << 3;
			for (int32_t i = 0; i < elements; i ++) {
				int32_t key = (getRadixKey (&array[i], context) >> radixShift) & 0xff;
				int32_t index = scanCount[key];
				array[index] = tmpArray[i];
				scanCount[key] = index + 1;
			}
		} else {
			memcpy (array, tmpArray, elements * sizeof (T)); 
		}
	}


#ifdef _DEBUG
	for (int32_t i = 0; i < (elements - 1); i ++) {
		HACD_ASSERT (getRadixKey (&array[i], context) <= getRadixKey (&array[i + 1], context));
	}
#endif

}


template <class T> 
void dgSort (T* const array, int32_t elements, int32_t (*compare) (const T* const  A, const T* const B, void* const context), void* const context = NULL)
{
	int32_t stride = 8;
	int32_t stack[1024][2];

	stack[0][0] = 0;
	stack[0][1] = elements - 1;
	int32_t stackIndex = 1;
	while (stackIndex) {
		stackIndex --;
		int32_t lo = stack[stackIndex][0];
		int32_t hi = stack[stackIndex][1];
		if ((hi - lo) > stride) {
			int32_t i = lo;
			int32_t j = hi;
			T val (array[(lo + hi) >> 1]);
			do {    
				while (compare (&array[i], &val, context) < 0) i ++;
				while (compare (&array[j], &val, context) > 0) j --;

				if (i <= j)	{
					T tmp (array[i]);
					array[i] = array[j]; 
					array[j] = tmp;
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
		}
	}

	stride = stride * 2;
	if (elements < stride) {
		stride = elements;
	}
	for (int32_t i = 1; i < stride; i ++) {
		if (compare (&array[0], &array[i], context) > 0) {
			T tmp (array[0]);
			array[0] = array[i];
			array[i] = tmp;
		}
	}

	for (int32_t i = 1; i < elements; i ++) {
		int32_t j = i;
		T tmp (array[i]);
		//for (; j && (compare (&array[j - 1], &tmp, context) > 0); j --) {
		for (; compare (&array[j - 1], &tmp, context) > 0; j --) {
			HACD_ASSERT (j > 0);
			array[j] = array[j - 1];
		}
		array[j] = tmp;
	}

#ifdef _DEBUG
	for (int32_t i = 0; i < (elements - 1); i ++) {
		HACD_ASSERT (compare (&array[i], &array[i + 1], context) <= 0);
	}
#endif
}


template <class T> 
void dgSortIndirect (T** const array, int32_t elements, int32_t (*compare) (const T* const  A, const T* const B, void* const context), void* const context = NULL)
{
	int32_t stride = 8;
	int32_t stack[1024][2];

	stack[0][0] = 0;
	stack[0][1] = elements - 1;
	int32_t stackIndex = 1;
	while (stackIndex) {
		stackIndex --;
		int32_t lo = stack[stackIndex][0];
		int32_t hi = stack[stackIndex][1];
		if ((hi - lo) > stride) {
			int32_t i = lo;
			int32_t j = hi;
			T* val (array[(lo + hi) >> 1]);
			do {    
				while (compare (array[i], val, context) < 0) i ++;
				while (compare (array[j], val, context) > 0) j --;

				if (i <= j)	{
					T* tmp (array[i]);
					array[i] = array[j]; 
					array[j] = tmp;
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
		}
	}

	stride = stride * 2;
	if (elements < stride) {
		stride = elements;
	}
	for (int32_t i = 1; i < stride; i ++) {
		if (compare (&array[0], &array[i], context) > 0) {
			T tmp (array[0]);
			array[0] = array[i];
			array[i] = tmp;
		}
	}

	for (int32_t i = 1; i < elements; i ++) {
		int32_t j = i;
		T* tmp (array[i]);
		//for (; j && (compare (array[j - 1], tmp, context) > 0); j --) {
		for (; compare (&array[j - 1], &tmp, context) > 0; j --) {
			HACD_ASSERT (j > 0);
			array[j] = array[j - 1];
		}
		array[j] = tmp;
	}

#ifdef _DEBUG
	for (int32_t i = 0; i < (elements - 1); i ++) {
		HACD_ASSERT (compare (array[i], array[i + 1], context) <= 0);
	}
#endif
}



#ifdef __USE_DOUBLE_PRECISION__
	union dgFloatSign
	{
		struct {
			int32_t m_dommy;
			int32_t m_iVal;
		} m_integer;
		double m_fVal;
	};
#else
	union dgFloatSign
	{
		struct {
			int32_t m_iVal;
		} m_integer;
		float m_fVal;
	};
#endif

union dgDoubleInt
{
	struct {
		int32_t m_intL;
		int32_t m_intH;
	} _dgDoubleInt;
	int64_t m_int;
	double m_float;
};



void GetMinMax (dgVector &Min, dgVector &Max, const float* const vArray, int32_t vCount, int32_t StrideInBytes);
void GetMinMax (dgBigVector &Min, dgBigVector &Max, const double* const vArray, int32_t vCount, int32_t strideInBytes);

int32_t dgVertexListToIndexList (float* const vertexList, int32_t strideInBytes, int32_t floatSizeInBytes,
				       int32_t unsignedSizeInBytes, int32_t vertexCount, int32_t* const indexListOut, float tolerance = dgEPSILON);

int32_t dgVertexListToIndexList (double* const vertexList, int32_t strideInBytes, int32_t compareCount, int32_t vertexCount, int32_t* const indexListOut, double tolerance = dgEPSILON);



#define PointerToInt(x) ((size_t)x)
#define IntToPointer(x) ((void*)(size_t(x)))

HACD_INLINE float dgAbsf(float x)
{
	// according to Intel this is better because is doe not read after write
	return (x >= float (0.0f)) ? x : -x;
}


HACD_INLINE int32_t dgFastInt (double x)
{
	int32_t i = int32_t (x);
	if (double (i) > x) {
		i --;
	}
	return i;
}

HACD_INLINE int32_t dgFastInt (float x)
{
	int32_t i = int32_t (x);
	if (float (i) > x) {
		i --;
	}
	return i;
}

HACD_INLINE float dgFloor(float x)
{
#ifdef _MSC_VER
	float ret = float (dgFastInt (x));
	HACD_ASSERT (ret == ::floor (x));
	return  ret;
#else 
	return floor (x);
#endif
}

HACD_INLINE float dgCeil(float x)
{
#ifdef _MSC_VER
	float ret = dgFloor(x);
	if (ret < x) {
		ret += float (1.0f);
	}
	HACD_ASSERT (ret == ::ceil (x));
	return  ret;
#else 
	return ceil (x);
#endif
}

#define dgSqrt HACD_SQRT
#define dgRsqrt HACD_RECIP_SQRT
#define dgSin(x) float (sin(x))
#define dgCos(x) float (cos(x))
#define dgAsin(x) float (asin(x))
#define dgAcos(x) float (acos(x))
#define dgAtan2(x,y) float (atan2(x,y))
#define dgLog(x) float (log(x))
#define dgPow(x,y) float (pow(x,y))
#define dgFmod(x,y) float (fmod(x,y))

#endif

