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

#ifndef __dgVector__
#define __dgVector__

#include "dgTypes.h"

#ifdef _DEBUG
#define dgCheckVector(x) (HACD_ISFINITE(x[0]) && HACD_ISFINITE(x[1]) && HACD_ISFINITE(x[2]) && HACD_ISFINITE(x[3]))
#else
#define dgCheckVector(x) true
#endif

template<class T>
class dgTemplateVector
{
	public:
	dgTemplateVector ();
	dgTemplateVector (const T *ptr);
	dgTemplateVector (T m_x, T m_y, T m_z, T m_w); 
	dgTemplateVector Scale (T s) const;
	dgTemplateVector Scale4 (T s) const;

	T& operator[] (int32_t i);
	const T& operator[] (int32_t i) const;

	dgTemplateVector operator+ (const dgTemplateVector &A) const; 
	dgTemplateVector operator- (const dgTemplateVector &A) const; 
	dgTemplateVector &operator+= (const dgTemplateVector &A);
	dgTemplateVector &operator-= (const dgTemplateVector &A); 

	// return dot product
	T operator% (const dgTemplateVector &A) const; 

	// return cross product
	dgTemplateVector operator* (const dgTemplateVector &B) const; 

	// return dot 4d dot product
	dgTemplateVector Add4 (const dgTemplateVector &A) const;
	dgTemplateVector Sub4 (const dgTemplateVector &A) const;
	T DotProduct4 (const dgTemplateVector &A) const; 
	dgTemplateVector CrossProduct4 (const dgTemplateVector &A, const dgTemplateVector &B) const; 

	// component wise multiplication
	dgTemplateVector CompProduct (const dgTemplateVector &A) const;

	// component wise multiplication
	dgTemplateVector CompProduct4 (const dgTemplateVector &A) const;

	// check validity of floats
#ifdef _DEBUG
	void Trace () const
	{
//		dgTrace (("%f %f %f %f\n", m_x, m_y, m_z, m_w));
	}
#endif

	T m_x;
	T m_y;
	T m_z;
	T m_w;
};

class dgBigVector;

class dgVector: public dgTemplateVector<float>
{
	public:
	dgVector();

	dgVector (const dgTemplateVector<float>& v);
	dgVector (const float *ptr);
	dgVector (float x, float y, float z, float w); 
         dgVector (const dgBigVector& copy); 

};

class dgBigVector: public dgTemplateVector<double>
{
	public:
	dgBigVector();
	dgBigVector (const dgVector& v);
	dgBigVector (const dgTemplateVector<double>& v);
	dgBigVector (const float *ptr);
#ifndef __USE_DOUBLE_PRECISION__
	dgBigVector (const double *ptr);
#endif
	dgBigVector (double x, double y, double z, double w); 
};





template<class T>
dgTemplateVector<T>::dgTemplateVector () {}

template<class T>
dgTemplateVector<T>::dgTemplateVector (const T *ptr)
	:m_x(ptr[0]), m_y(ptr[1]), m_z(ptr[2]), m_w (0.0f)
{
//	HACD_ASSERT (dgCheckVector ((*this)));
}

template<class T>
dgTemplateVector<T>::dgTemplateVector (T x, T y, T z, T w) 
	:m_x(x), m_y(y), m_z(z), m_w (w)
{
}


template<class T>
T& dgTemplateVector<T>::operator[] (int32_t i)
{
	HACD_ASSERT (i < 4);
	HACD_ASSERT (i >= 0);
	return (&m_x)[i];
}	

template<class T>
const T& dgTemplateVector<T>::operator[] (int32_t i) const
{
	HACD_ASSERT (i < 4);
	HACD_ASSERT (i >= 0);
	return (&m_x)[i];
}

template<class T>
dgTemplateVector<T> dgTemplateVector<T>::Scale (T scale) const
{
	return dgTemplateVector<T> (m_x * scale, m_y * scale, m_z * scale, m_w);
}

template<class T>
dgTemplateVector<T> dgTemplateVector<T>::Scale4 (T scale) const
{
	return dgTemplateVector<T> (m_x * scale, m_y * scale, m_z * scale, m_w * scale);
}


template<class T>
dgTemplateVector<T> dgTemplateVector<T>::operator+ (const dgTemplateVector<T> &B) const
{
	return dgTemplateVector<T> (m_x + B.m_x, m_y + B.m_y, m_z + B.m_z, m_w);
}

template<class T>
dgTemplateVector<T>& dgTemplateVector<T>::operator+= (const dgTemplateVector<T> &A) 
{
	m_x += A.m_x;
	m_y += A.m_y;
	m_z += A.m_z;
//	HACD_ASSERT (dgCheckVector ((*this)));
	return *this;
}

template<class T>
dgTemplateVector<T> dgTemplateVector<T>::operator- (const dgTemplateVector<T> &A) const
{
	return dgTemplateVector<T> (m_x - A.m_x, m_y - A.m_y, m_z - A.m_z, m_w);
}

template<class T>
dgTemplateVector<T>& dgTemplateVector<T>::operator-= (const dgTemplateVector<T> &A) 
{
	m_x -= A.m_x;
	m_y -= A.m_y;
	m_z -= A.m_z;
//	HACD_ASSERT (dgCheckVector ((*this)));
	return *this;
}


template<class T>
T dgTemplateVector<T>::operator% (const dgTemplateVector<T> &A) const
{
	return m_x * A.m_x + m_y * A.m_y + m_z * A.m_z;
}


template<class T>
dgTemplateVector<T> dgTemplateVector<T>::operator* (const dgTemplateVector<T> &B) const
{
	return dgTemplateVector<T> (m_y * B.m_z - m_z * B.m_y,
 							    m_z * B.m_x - m_x * B.m_z,
								m_x * B.m_y - m_y * B.m_x, m_w);
}

template<class T>
dgTemplateVector<T> dgTemplateVector<T>::Add4 (const dgTemplateVector &A) const
{
	return dgTemplateVector<T> (m_x + A.m_x, m_y + A.m_y, m_z + A.m_z, m_w + A.m_w);
}

template<class T>
dgTemplateVector<T> dgTemplateVector<T>::Sub4 (const dgTemplateVector &A) const
{
	return dgTemplateVector<T> (m_x - A.m_x, m_y - A.m_y, m_z - A.m_z, m_w - A.m_w);
}


// return dot 4d dot product
template<class T>
T dgTemplateVector<T>::DotProduct4 (const dgTemplateVector &A) const
{
	return m_x * A.m_x + m_y * A.m_y + m_z * A.m_z + m_w * A.m_w;
}

template<class T>
dgTemplateVector<T> dgTemplateVector<T>::CrossProduct4 (const dgTemplateVector &A, const dgTemplateVector &B) const
{
	T cofactor[3][3];
	T array[4][4];

	const dgTemplateVector<T>& me = *this;
	for (int32_t i = 0; i < 4; i ++) {
		array[0][i] = me[i];
		array[1][i] = A[i];
		array[2][i] = B[i];
		array[3][i] = T (1.0f);
	}
	
	dgTemplateVector<T> normal;
	T sign = T (-1.0f);
	for (int32_t i = 0; i < 4; i ++)  {
		
		for (int32_t j = 0; j < 3; j ++) {
			int32_t k0 = 0;
			for (int32_t k = 0; k < 4; k ++) {
				if (k != i) {
					cofactor[j][k0] = array[j][k];
					k0 ++;
				}
			}
		}
		T x = cofactor[0][0] * (cofactor[1][1] * cofactor[2][2] - cofactor[1][2] * cofactor[2][1]);
		T y = cofactor[0][1] * (cofactor[1][2] * cofactor[2][0] - cofactor[1][0] * cofactor[2][2]);
		T z = cofactor[0][2] * (cofactor[1][0] * cofactor[2][1] - cofactor[1][1] * cofactor[2][0]);
		T det = x + y + z;
		
		normal[i] = sign * det;
		sign *= T (-1.0f);
	}

	return normal;
}



template<class T>
dgTemplateVector<T> dgTemplateVector<T>::CompProduct (const dgTemplateVector<T> &A) const
{
	return dgTemplateVector<T> (m_x * A.m_x, m_y * A.m_y, m_z * A.m_z, A.m_w);
}

template<class T>
dgTemplateVector<T> dgTemplateVector<T>::CompProduct4 (const dgTemplateVector<T> &A) const
{
	return dgTemplateVector<T> (m_x * A.m_x, m_y * A.m_y, m_z * A.m_z, m_w * A.m_w);
}



HACD_FORCE_INLINE dgVector::dgVector()
	:dgTemplateVector<float>()
{
}

HACD_FORCE_INLINE dgVector::dgVector (const dgTemplateVector<float>& v)
	:dgTemplateVector<float>(v)
{
	HACD_ASSERT (dgCheckVector ((*this)));
}

HACD_FORCE_INLINE dgVector::dgVector (const float *ptr)
	:dgTemplateVector<float>(ptr)
{
	HACD_ASSERT (dgCheckVector ((*this)));
}

HACD_FORCE_INLINE dgVector::dgVector (const dgBigVector& copy)
	:dgTemplateVector<float>(float (copy.m_x), float (copy.m_y), float (copy.m_z), float (copy.m_w))
{
	HACD_ASSERT (dgCheckVector ((*this)));
}

HACD_FORCE_INLINE dgVector::dgVector (float x, float y, float z, float w) 
	:dgTemplateVector<float>(x, y, z, w)
{
	HACD_ASSERT (dgCheckVector ((*this)));
}

HACD_FORCE_INLINE dgBigVector::dgBigVector()
	:dgTemplateVector<double>()
{
}

HACD_FORCE_INLINE dgBigVector::dgBigVector (const dgVector& v)
	:dgTemplateVector<double>(v.m_x, v.m_y, v.m_z, v.m_w)
{
	HACD_ASSERT (dgCheckVector ((*this)));
}

HACD_FORCE_INLINE dgBigVector::dgBigVector (const dgTemplateVector<double>& v)
	:dgTemplateVector<double>(v)
{
	HACD_ASSERT (dgCheckVector ((*this)));
}

HACD_FORCE_INLINE dgBigVector::dgBigVector (const float *ptr)
	:dgTemplateVector<double>(ptr[0], ptr[1], ptr[2], double (0.0f))
{
	HACD_ASSERT (dgCheckVector ((*this)));
}

#ifndef __USE_DOUBLE_PRECISION__
HACD_FORCE_INLINE dgBigVector::dgBigVector (const double *ptr)
	:dgTemplateVector<double>(ptr)
{
	HACD_ASSERT (dgCheckVector ((*this)));
}
#endif

HACD_FORCE_INLINE dgBigVector::dgBigVector (double x, double y, double z, double w) 
	:dgTemplateVector<double>(x, y, z, w)
{
	HACD_ASSERT (dgCheckVector ((*this)));
}


#endif

