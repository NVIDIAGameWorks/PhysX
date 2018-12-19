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



#ifndef MI_MATH_H

#define MI_MATH_H

#include <math.h>
#include <float.h>
#include "MeshImportTypes.h"
#include "MiPlatformConfig.h"

using namespace mimp;

#define MiSqrt sqrtf
#define MiIsFinite(x) _finite(x)
#define MiAbs(x) fabsf(x)
#define MiRecipSqrt(x) (1.0f/MiSqrt(x))
#define MiSin(x) sinf(x)
#define MiCos(x) cosf(x)
#define MiAtan2(x,y) atan2f(x,y)
#define MiAcos(x) acosf(x)


namespace mimp
{
	// constants
	static const MiF32 MiPi		=	3.141592653589793f;
	static const MiF32 MiHalfPi	=	1.57079632679489661923f;
	static const MiF32 MiTwoPi		=	6.28318530717958647692f;
	static const MiF32 MiInvPi		=	0.31830988618379067154f;




	MI_FORCE_INLINE MiF32 MiMax(MiF32 a,MiF32 b)
	{
		return a < b ? b : a;
	}

	MI_FORCE_INLINE MiF32 MiMin(MiF32 a,MiF32 b)
	{
		return a < b ? a : b;
	}
/**
\brief 2 Element vector class.

This is a vector class with public data members.
This is not nice but it has become such a standard that hiding the xy data members
makes it difficult to reuse external code that assumes that these are public in the library.
The vector class can be made to use float or double precision by appropriately defining MiF32.
This has been chosen as a cleaner alternative to a template class.
*/
class MiVec2
{
public:

	/**
	\brief default constructor leaves data uninitialized.
	*/
	 MI_FORCE_INLINE MiVec2() {}

	/**
	\brief Assigns scalar parameter to all elements.

	Useful to initialize to zero or one.

	\param[in] a Value to assign to elements.
	*/
	explicit  MI_FORCE_INLINE MiVec2(MiF32 a): x(a), y(a) {}

	/**
	\brief Initializes from 2 scalar parameters.

	\param[in] nx Value to initialize X component.
	\param[in] ny Value to initialize Y component.
	*/
	 MI_FORCE_INLINE MiVec2(MiF32 nx, MiF32 ny): x(nx), y(ny){}

	/**
	\brief Copy ctor.
	*/
	 MI_FORCE_INLINE MiVec2(const MiVec2& v): x(v.x), y(v.y) {}

	//Operators

	/**
	\brief Assignment operator
	*/
	 MI_FORCE_INLINE	MiVec2&	operator=(const MiVec2& p)			{ x = p.x; y = p.y;	return *this;		}

	/**
	\brief element access
	*/
	 MI_FORCE_INLINE MiF32& operator[](int index)					{ MI_ASSERT(index>=0 && index<=1); return (&x)[index]; }

	/**
	\brief element access
	*/
	 MI_FORCE_INLINE const MiF32& operator[](int index) const		{ MI_ASSERT(index>=0 && index<=1); return (&x)[index]; }

	/**
	\brief returns true if the two vectors are exactly equal.
	*/
	 MI_FORCE_INLINE bool operator==(const MiVec2&v) const	{ return x == v.x && y == v.y; }

	/**
	\brief returns true if the two vectors are not exactly equal.
	*/
	 MI_FORCE_INLINE bool operator!=(const MiVec2&v) const	{ return x != v.x || y != v.y; }

	/**
	\brief tests for exact zero vector
	*/
	 MI_FORCE_INLINE bool isZero()	const					{ return x==0.0f && y==0.0f;			}

	/**
	\brief returns true if all 2 elems of the vector are finite (not NAN or INF, etc.)
	*/
	 MI_INLINE bool isFinite() const
	{
		return MiIsFinite(x) && MiIsFinite(y);
	}

	/**
	\brief is normalized - used by API parameter validation
	*/
	 MI_FORCE_INLINE bool isNormalized() const
	{
		const float unitTolerance = MiF32(1e-4);
		return isFinite() && MiAbs(magnitude()-1)<unitTolerance;
	}

	/**
	\brief returns the squared magnitude

	Avoids calling MiSqrt()!
	*/
	 MI_FORCE_INLINE MiF32 magnitudeSquared() const		{	return x * x + y * y;					}

	/**
	\brief returns the magnitude
	*/
	 MI_FORCE_INLINE MiF32 magnitude() const				{	return MiSqrt(magnitudeSquared());		}

	/**
	\brief negation
	*/
	 MI_FORCE_INLINE MiVec2 operator -() const
	{
		return MiVec2(-x, -y);
	}

	/**
	\brief vector addition
	*/
	 MI_FORCE_INLINE MiVec2 operator +(const MiVec2& v) const		{	return MiVec2(x + v.x, y + v.y);	}

	/**
	\brief vector difference
	*/
	 MI_FORCE_INLINE MiVec2 operator -(const MiVec2& v) const		{	return MiVec2(x - v.x, y - v.y);	}

	/**
	\brief scalar post-multiplication
	*/
	 MI_FORCE_INLINE MiVec2 operator *(MiF32 f) const				{	return MiVec2(x * f, y * f);			}

	/**
	\brief scalar division
	*/
	 MI_FORCE_INLINE MiVec2 operator /(MiF32 f) const
	{
		f = MiF32(1) / f;	// PT: inconsistent notation with operator /=
		return MiVec2(x * f, y * f);
	}

	/**
	\brief vector addition
	*/
	 MI_FORCE_INLINE MiVec2& operator +=(const MiVec2& v)
	{
		x += v.x;
		y += v.y;
		return *this;
	}
	
	/**
	\brief vector difference
	*/
	 MI_FORCE_INLINE MiVec2& operator -=(const MiVec2& v)
	{
		x -= v.x;
		y -= v.y;
		return *this;
	}

	/**
	\brief scalar multiplication
	*/
	 MI_FORCE_INLINE MiVec2& operator *=(MiF32 f)
	{
		x *= f;
		y *= f;
		return *this;
	}
	/**
	\brief scalar division
	*/
	 MI_FORCE_INLINE MiVec2& operator /=(MiF32 f)
	{
		f = 1.0f/f;	// PT: inconsistent notation with operator /
		x *= f;
		y *= f;
		return *this;
	}

	/**
	\brief returns the scalar product of this and other.
	*/
	 MI_FORCE_INLINE MiF32 dot(const MiVec2& v) const		
	{	
		return x * v.x + y * v.y;				
	}

	/** return a unit vector */

	 MI_FORCE_INLINE MiVec2 getNormalized() const
	{
		const MiF32 m = magnitudeSquared();
		return m>0 ? *this * MiRecipSqrt(m) : MiVec2(0,0);
	}

	/**
	\brief normalizes the vector in place
	*/
	 MI_FORCE_INLINE MiF32 normalize()
	{
		const MiF32 m = magnitude();
		if (m>0) 
			*this /= m;
		return m;
	}

	/**
	\brief a[i] * b[i], for all i.
	*/
	 MI_FORCE_INLINE MiVec2 multiply(const MiVec2& a) const
	{
		return MiVec2(x*a.x, y*a.y);
	}

	/**
	\brief element-wise minimum
	*/
	 MI_FORCE_INLINE MiVec2 minimum(const MiVec2& v) const
	{ 
		return MiVec2(MiMin(x, v.x), MiMin(y,v.y));	
	}

	/**
	\brief returns MIN(x, y);
	*/
	 MI_FORCE_INLINE float minElement()	const
	{
		return MiMin(x, y);
	}
	
	/**
	\brief element-wise maximum
	*/
	 MI_FORCE_INLINE MiVec2 maximum(const MiVec2& v) const
	{ 
		return MiVec2(MiMax(x, v.x), MiMax(y,v.y));	
	} 

	/**
	\brief returns MAX(x, y);
	*/
	 MI_FORCE_INLINE float maxElement()	const
	{
		return MiMax(x, y);
	}

	MiF32 x,y;
};

 static MI_FORCE_INLINE MiVec2 operator *(MiF32 f, const MiVec2& v)
{
	return MiVec2(f * v.x, f * v.y);
}

/**
\brief 3 Element vector class.

This is a vector class with public data members.
This is not nice but it has become such a standard that hiding the xyz data members
makes it difficult to reuse external code that assumes that these are public in the library.
The vector class can be made to use float or double precision by appropriately defining MiF32.
This has been chosen as a cleaner alternative to a template class.
*/
class MiVec3
{
public:

	/**
	\brief default constructor leaves data uninitialized.
	*/
	 MI_FORCE_INLINE MiVec3() {}

	/**
	\brief Assigns scalar parameter to all elements.

	Useful to initialize to zero or one.

	\param[in] a Value to assign to elements.
	*/
	explicit  MI_FORCE_INLINE MiVec3(MiF32 a): x(a), y(a), z(a) {}

	/**
	\brief Initializes from 3 scalar parameters.

	\param[in] nx Value to initialize X component.
	\param[in] ny Value to initialize Y component.
	\param[in] nz Value to initialize Z component.
	*/
	 MI_FORCE_INLINE MiVec3(MiF32 nx, MiF32 ny, MiF32 nz): x(nx), y(ny), z(nz) {}

	/**
	\brief Copy ctor.
	*/
	 MI_FORCE_INLINE MiVec3(const MiVec3& v): x(v.x), y(v.y), z(v.z) {}

	//Operators

	/**
	\brief Assignment operator
	*/
	 MI_FORCE_INLINE	MiVec3&	operator=(const MiVec3& p)			{ x = p.x; y = p.y; z = p.z;	return *this;		}

	/**
	\brief element access
	*/
	 MI_FORCE_INLINE MiF32& operator[](int index)					{ MI_ASSERT(index>=0 && index<=2); return (&x)[index]; }

	/**
	\brief element access
	*/
	 MI_FORCE_INLINE const MiF32& operator[](int index) const		{ MI_ASSERT(index>=0 && index<=2); return (&x)[index]; }

	/**
	\brief returns true if the two vectors are exactly equal.
	*/
	 MI_FORCE_INLINE bool operator==(const MiVec3&v) const	{ return x == v.x && y == v.y && z == v.z; }

	/**
	\brief returns true if the two vectors are not exactly equal.
	*/
	 MI_FORCE_INLINE bool operator!=(const MiVec3&v) const	{ return x != v.x || y != v.y || z != v.z; }

	/**
	\brief tests for exact zero vector
	*/
	 MI_FORCE_INLINE bool isZero()	const					{ return x==0.0f && y==0.0f && z == 0.0f;			}

	/**
	\brief returns true if all 3 elems of the vector are finite (not NAN or INF, etc.)
	*/
	 MI_INLINE bool isFinite() const
	{
		return MiIsFinite(x) && MiIsFinite(y) && MiIsFinite(z);
	}

	/**
	\brief is normalized - used by API parameter validation
	*/
	 MI_FORCE_INLINE bool isNormalized() const
	{
		const float unitTolerance = MiF32(1e-4);
		return isFinite() && MiAbs(magnitude()-1)<unitTolerance;
	}

	/**
	\brief returns the squared magnitude

	Avoids calling MiSqrt()!
	*/
	 MI_FORCE_INLINE MiF32 magnitudeSquared() const		{	return x * x + y * y + z * z;					}

	/**
	\brief returns the magnitude
	*/
	 MI_FORCE_INLINE MiF32 magnitude() const				{	return MiSqrt(magnitudeSquared());		}

	/**
	\brief negation
	*/
	 MI_FORCE_INLINE MiVec3 operator -() const
	{
		return MiVec3(-x, -y, -z);
	}

	/**
	\brief vector addition
	*/
	 MI_FORCE_INLINE MiVec3 operator +(const MiVec3& v) const		{	return MiVec3(x + v.x, y + v.y, z + v.z);	}

	/**
	\brief vector difference
	*/
	 MI_FORCE_INLINE MiVec3 operator -(const MiVec3& v) const		{	return MiVec3(x - v.x, y - v.y, z - v.z);	}

	/**
	\brief scalar post-multiplication
	*/
	 MI_FORCE_INLINE MiVec3 operator *(MiF32 f) const				{	return MiVec3(x * f, y * f, z * f);			}

	/**
	\brief scalar division
	*/
	 MI_FORCE_INLINE MiVec3 operator /(MiF32 f) const
	{
		f = MiF32(1) / f;	// PT: inconsistent notation with operator /=
		return MiVec3(x * f, y * f, z * f);
	}

	/**
	\brief vector addition
	*/
	 MI_FORCE_INLINE MiVec3& operator +=(const MiVec3& v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}
	
	/**
	\brief vector difference
	*/
	 MI_FORCE_INLINE MiVec3& operator -=(const MiVec3& v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	/**
	\brief scalar multiplication
	*/
	 MI_FORCE_INLINE MiVec3& operator *=(MiF32 f)
	{
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}
	/**
	\brief scalar division
	*/
	 MI_FORCE_INLINE MiVec3& operator /=(MiF32 f)
	{
		f = 1.0f/f;	// PT: inconsistent notation with operator /
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}

	/**
	\brief returns the scalar product of this and other.
	*/
	 MI_FORCE_INLINE MiF32 dot(const MiVec3& v) const		
	{	
		return x * v.x + y * v.y + z * v.z;				
	}

	/**
	\brief cross product
	*/
	 MI_FORCE_INLINE MiVec3 cross(const MiVec3& v) const
	{
		return MiVec3(y * v.z - z * v.y, 
					  z * v.x - x * v.z, 
					  x * v.y - y * v.x);
	}

	/** return a unit vector */

	 MI_FORCE_INLINE MiVec3 getNormalized() const
	{
		const MiF32 m = magnitudeSquared();
		return m>0 ? *this * MiRecipSqrt(m) : MiVec3(0,0,0);
	}

	/**
	\brief normalizes the vector in place
	*/
	 MI_FORCE_INLINE MiF32 normalize()
	{
		const MiF32 m = magnitude();
		if (m>0) 
			*this /= m;
		return m;
	}

	/**
	\brief a[i] * b[i], for all i.
	*/
	 MI_FORCE_INLINE MiVec3 multiply(const MiVec3& a) const
	{
		return MiVec3(x*a.x, y*a.y, z*a.z);
	}

	/**
	\brief element-wise minimum
	*/
	 MI_FORCE_INLINE MiVec3 minimum(const MiVec3& v) const
	{ 
		return MiVec3(MiMin(x, v.x), MiMin(y,v.y), MiMin(z,v.z));	
	}

	/**
	\brief returns MIN(x, y, z);
	*/
	 MI_FORCE_INLINE float minElement()	const
	{
		return MiMin(x, MiMin(y, z));
	}
	
	/**
	\brief element-wise maximum
	*/
	 MI_FORCE_INLINE MiVec3 maximum(const MiVec3& v) const
	{ 
		return MiVec3(MiMax(x, v.x), MiMax(y,v.y), MiMax(z,v.z));	
	} 

	/**
	\brief returns MAX(x, y, z);
	*/
	 MI_FORCE_INLINE float maxElement()	const
	{
		return MiMax(x, MiMax(y, z));
	}

	MiF32 x,y,z;
};

 static MI_FORCE_INLINE MiVec3 operator *(MiF32 f, const MiVec3& v)
{
	return MiVec3(f * v.x, f * v.y, f * v.z);
}


class MiVec4
{
public:

	/**
	\brief default constructor leaves data uninitialized.
	*/
	 MI_INLINE MiVec4() {}

	/**
	\brief Assigns scalar parameter to all elements.

	Useful to initialize to zero or one.

	\param[in] a Value to assign to elements.
	*/
	explicit  MI_INLINE MiVec4(MiF32 a): x(a), y(a), z(a), w(a) {}

	/**
	\brief Initializes from 3 scalar parameters.

	\param[in] nx Value to initialize X component.
	\param[in] ny Value to initialize Y component.
	\param[in] nz Value to initialize Z component.
	\param[in] nw Value to initialize W component.
	*/
	 MI_INLINE MiVec4(MiF32 nx, MiF32 ny, MiF32 nz, MiF32 nw): x(nx), y(ny), z(nz), w(nw) {}


	/**
	\brief Initializes from 3 scalar parameters.

	\param[in] v Value to initialize the X, Y, and Z components.
	\param[in] nw Value to initialize W component.
	*/
	 MI_INLINE MiVec4(const MiVec3& v, MiF32 nw): x(v.x), y(v.y), z(v.z), w(nw) {}


	/**
	\brief Initializes from an array of scalar parameters.

	\param[in] v Value to initialize with.
	*/
	explicit  MI_INLINE MiVec4(const MiF32 v[]): x(v[0]), y(v[1]), z(v[2]), w(v[3]) {}

	/**
	\brief Copy ctor.
	*/
	 MI_INLINE MiVec4(const MiVec4& v): x(v.x), y(v.y), z(v.z), w(v.w) {}

	//Operators

	/**
	\brief Assignment operator
	*/
	 MI_INLINE	MiVec4&	operator=(const MiVec4& p)			{ x = p.x; y = p.y; z = p.z; w = p.w;	return *this;		}

	/**
	\brief element access
	*/
	 MI_INLINE MiF32& operator[](int index)				{ MI_ASSERT(index>=0 && index<=3); return (&x)[index]; }

	/**
	\brief element access
	*/
	 MI_INLINE const MiF32& operator[](int index) const	{ MI_ASSERT(index>=0 && index<=3); return (&x)[index]; }

	/**
	\brief returns true if the two vectors are exactly equal.
	*/
	 MI_INLINE bool operator==(const MiVec4&v) const	{ return x == v.x && y == v.y && z == v.z && w == v.w; }

	/**
	\brief returns true if the two vectors are not exactly equal.
	*/
	 MI_INLINE bool operator!=(const MiVec4&v) const	{ return x != v.x || y != v.y || z != v.z || w!= v.w; }

	/**
	\brief tests for exact zero vector
	*/
	 MI_INLINE bool isZero()	const					{ return x==0 && y==0 && z == 0 && w == 0;			}

	/**
	\brief returns true if all 3 elems of the vector are finite (not NAN or INF, etc.)
	*/
	 MI_INLINE bool isFinite() const
	{
		return MiIsFinite(x) && MiIsFinite(y) && MiIsFinite(z) && MiIsFinite(w);
	}

	/**
	\brief is normalized - used by API parameter validation
	*/
	 MI_INLINE bool isNormalized() const
	{
		const float unitTolerance = MiF32(1e-4);
		return isFinite() && MiAbs(magnitude()-1)<unitTolerance;
	}


	/**
	\brief returns the squared magnitude

	Avoids calling MiSqrt()!
	*/
	 MI_INLINE MiF32 magnitudeSquared() const		{	return x * x + y * y + z * z + w * w;		}

	/**
	\brief returns the magnitude
	*/
	 MI_INLINE MiF32 magnitude() const				{	return MiSqrt(magnitudeSquared());		}

	/**
	\brief negation
	*/
	 MI_INLINE MiVec4 operator -() const
	{
		return MiVec4(-x, -y, -z, -w);
	}

	/**
	\brief vector addition
	*/
	 MI_INLINE MiVec4 operator +(const MiVec4& v) const		{	return MiVec4(x + v.x, y + v.y, z + v.z, w + v.w);	}

	/**
	\brief vector difference
	*/
	 MI_INLINE MiVec4 operator -(const MiVec4& v) const		{	return MiVec4(x - v.x, y - v.y, z - v.z, w - v.w);	}

	/**
	\brief scalar post-multiplication
	*/

	 MI_INLINE MiVec4 operator *(MiF32 f) const				{	return MiVec4(x * f, y * f, z * f, w * f);		}

	/**
	\brief scalar division
	*/
	 MI_INLINE MiVec4 operator /(MiF32 f) const
	{
		f = MiF32(1) / f; 
		return MiVec4(x * f, y * f, z * f, w * f);
	}

	/**
	\brief vector addition
	*/
	 MI_INLINE MiVec4& operator +=(const MiVec4& v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		w += v.w;
		return *this;
	}
	
	/**
	\brief vector difference
	*/
	 MI_INLINE MiVec4& operator -=(const MiVec4& v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		w -= v.w;
		return *this;
	}

	/**
	\brief scalar multiplication
	*/
	 MI_INLINE MiVec4& operator *=(MiF32 f)
	{
		x *= f;
		y *= f;
		z *= f;
		w *= f;
		return *this;
	}
	/**
	\brief scalar division
	*/
	 MI_INLINE MiVec4& operator /=(MiF32 f)
	{
		f = 1.0f/f;
		x *= f;
		y *= f;
		z *= f;
		w *= f;
		return *this;
	}

	/**
	\brief returns the scalar product of this and other.
	*/
	 MI_INLINE MiF32 dot(const MiVec4& v) const		
	{	
		return x * v.x + y * v.y + z * v.z + w * v.w;				
	}

	/** return a unit vector */

	 MI_INLINE MiVec4 getNormalized() const
	{
		MiF32 m = magnitudeSquared();
		return m>0 ? *this * MiRecipSqrt(m) : MiVec4(0,0,0,0);
	}


	/**
	\brief normalizes the vector in place
	*/
	 MI_INLINE MiF32 normalize()
	{
		MiF32 m = magnitude();
		if (m>0) 
			*this /= m;
		return m;
	}

	/**
	\brief a[i] * b[i], for all i.
	*/
	 MI_INLINE MiVec4 multiply(const MiVec4& a) const
	{
		return MiVec4(x*a.x, y*a.y, z*a.z, w*a.w);
	}

	/**
	\brief element-wise minimum
	*/
	 MI_INLINE MiVec4 minimum(const MiVec4& v) const
	{ 
		return MiVec4(MiMin(x, v.x), MiMin(y,v.y), MiMin(z,v.z), MiMin(w,v.w));	
	}

	/**
	\brief element-wise maximum
	*/
	 MI_INLINE MiVec4 maximum(const MiVec4& v) const
	{ 
		return MiVec4(MiMax(x, v.x), MiMax(y,v.y), MiMax(z,v.z), MiMax(w,v.w));	
	} 

	 MI_INLINE MiVec3 getXYZ() const
	{
		return MiVec3(x,y,z);
	}

	/**
	\brief set vector elements to zero
	*/
	 MI_INLINE void setZero() {	x = y = z = w = MiF32(0); }

	MiF32 x,y,z,w;
};


 static MI_INLINE MiVec4 operator *(MiF32 f, const MiVec4& v)
{
	return MiVec4(f * v.x, f * v.y, f * v.z, f * v.w);
}

/**
\brief This is a quaternion class. For more information on quaternion mathematics
consult a mathematics source on complex numbers.

*/

class MiMat33;

class MiQuat
{
public:
	/**
	\brief Default constructor, does not do any initialization.
	*/
	 MI_FORCE_INLINE MiQuat()	{	}


	/**
	\brief Constructor.  Take note of the order of the elements!
	*/
	 MI_FORCE_INLINE MiQuat(MiF32 nx, MiF32 ny, MiF32 nz, MiF32 nw) : x(nx),y(ny),z(nz),w(nw) {}

	/**
	\brief Creates from angle-axis representation.

	Axis must be normalized!

	Angle is in radians!

	<b>Unit:</b> Radians
	*/
	 MI_INLINE MiQuat(MiF32 angleRadians, const MiVec3& unitAxis)
	{
		MI_ASSERT(MiAbs(1.0f-unitAxis.magnitude())<1e-3f);
		const MiF32 a = angleRadians * 0.5f;
		const MiF32 s = MiSin(a);
		w = MiCos(a);
		x = unitAxis.x * s;
		y = unitAxis.y * s;
		z = unitAxis.z * s;
	}

	/**
	\brief Copy ctor.
	*/
	 MI_FORCE_INLINE MiQuat(const MiQuat& v): x(v.x), y(v.y), z(v.z), w(v.w) {}

	/**
	\brief Creates from orientation matrix.

	\param[in] m Rotation matrix to extract quaternion from.
	*/
	 MI_INLINE explicit MiQuat(const MiMat33& m); /* defined in MiMat33.h */

	/**
	\brief returns true if all elements are finite (not NAN or INF, etc.)
	*/
	 bool isFinite() const
	{
		return MiIsFinite(x) 
			&& MiIsFinite(y) 
			&& MiIsFinite(z)
			&& MiIsFinite(w);
	}

	/**
	\brief returns true if finite and magnitude is close to unit
	*/

	 bool isValid() const
	{
		const MiF32 unitTolerance = MiF32(1e-4);
		return isFinite() && MiAbs(magnitude()-1)<unitTolerance;
	}

	/**
	\brief returns true if finite and magnitude is reasonably close to unit to allow for some accumulation of error vs isValid
	*/

	 bool isSane() const
	{
	      const MiF32 unitTolerance = MiF32(1e-2);
	      return isFinite() && MiAbs(magnitude()-1)<unitTolerance;
	}

	/**
	\brief converts this quaternion to angle-axis representation
	*/

	 MI_INLINE void toRadiansAndUnitAxis(MiF32& angle, MiVec3& axis) const
	{
		const MiF32 quatEpsilon = (MiF32(1.0e-8f));
		const MiF32 s2 = x*x+y*y+z*z;
		if(s2<quatEpsilon*quatEpsilon)  // can't extract a sensible axis
		{
			angle = 0;
			axis = MiVec3(1,0,0);
		}
		else
		{
			const MiF32 s = MiRecipSqrt(s2);
			axis = MiVec3(x,y,z) * s; 
			angle = w<quatEpsilon ? MiPi : MiAtan2(s2*s, w) * 2;
		}

	}

	/**
	\brief Gets the angle between this quat and the identity quaternion.

	<b>Unit:</b> Radians
	*/
	 MI_INLINE MiF32 getAngle() const
	{
		return MiAcos(w) * MiF32(2);
	}


	/**
	\brief Gets the angle between this quat and the argument

	<b>Unit:</b> Radians
	*/
	 MI_INLINE MiF32 getAngle(const MiQuat& q) const
	{
		return MiAcos(dot(q)) * MiF32(2);
	}


	/**
	\brief This is the squared 4D vector length, should be 1 for unit quaternions.
	*/
	 MI_FORCE_INLINE MiF32 magnitudeSquared() const
	{
		return x*x + y*y + z*z + w*w;
	}

	/**
	\brief returns the scalar product of this and other.
	*/
	 MI_FORCE_INLINE MiF32 dot(const MiQuat& v) const
	{
		return x * v.x + y * v.y + z * v.z  + w * v.w;
	}

	 MI_INLINE MiQuat getNormalized() const
	{
		const MiF32 s = 1.0f/magnitude();
		return MiQuat(x*s, y*s, z*s, w*s);
	}


	 MI_INLINE float magnitude() const
	{
		return MiSqrt(magnitudeSquared());
	}

	//modifiers:
	/**
	\brief maps to the closest unit quaternion.
	*/
	 MI_INLINE MiF32 normalize()											// convert this MiQuat to a unit quaternion
	{
		const MiF32 mag = magnitude();
		if (mag)
		{
			const MiF32 imag = MiF32(1) / mag;

			x *= imag;
			y *= imag;
			z *= imag;
			w *= imag;
		}
		return mag;
	}

	/*
	\brief returns the conjugate.

	\note for unit quaternions, this is the inverse.
	*/
	 MI_INLINE MiQuat getConjugate() const
	{
		return MiQuat(-x,-y,-z,w);
	}

	/*
	\brief returns imaginary part.
	*/
	 MI_INLINE MiVec3& getImaginaryPart()
	{
		return reinterpret_cast<MiVec3&>(x);
	}

	/*
	\brief returns imaginary part.
	*/
	 MI_INLINE const MiVec3& getImaginaryPart() const
	{
		return reinterpret_cast<const MiVec3&>(x);
	}


	/** brief computes rotation of x-axis */
	 MI_FORCE_INLINE MiVec3 getBasisVector0()	const
	{	
//		return rotate(MiVec3(1,0,0));
		const MiF32 x2 = x*2.0f;
		const MiF32 w2 = w*2.0f;
		return MiVec3(	(w * w2) - 1.0f + x*x2,
						(z * w2)        + y*x2,
						(-y * w2)       + z*x2);
	}
	
	/** brief computes rotation of y-axis */
	 MI_FORCE_INLINE MiVec3 getBasisVector1()	const 
	{	
//		return rotate(MiVec3(0,1,0));
		const MiF32 y2 = y*2.0f;
		const MiF32 w2 = w*2.0f;
		return MiVec3(	(-z * w2)       + x*y2,
						(w * w2) - 1.0f + y*y2,
						(x * w2)        + z*y2);
	}


	/** brief computes rotation of z-axis */
	 MI_FORCE_INLINE MiVec3 getBasisVector2() const	
	{	
//		return rotate(MiVec3(0,0,1));
		const MiF32 z2 = z*2.0f;
		const MiF32 w2 = w*2.0f;
		return MiVec3(	(y * w2)        + x*z2,
						(-x * w2)       + y*z2,
						(w * w2) - 1.0f + z*z2);
	}

	/**
	rotates passed vec by this (assumed unitary)
	*/
	 MI_FORCE_INLINE const MiVec3 rotate(const MiVec3& v) const
//	 MI_INLINE const MiVec3 rotate(const MiVec3& v) const
	{
		const MiF32 vx = 2.0f*v.x;
		const MiF32 vy = 2.0f*v.y;
		const MiF32 vz = 2.0f*v.z;
		const MiF32 w2 = w*w-0.5f;
		const MiF32 dot2 = (x*vx + y*vy +z*vz);
		return MiVec3
		(
			(vx*w2 + (y * vz - z * vy)*w + x*dot2), 
			(vy*w2 + (z * vx - x * vz)*w + y*dot2), 
			(vz*w2 + (x * vy - y * vx)*w + z*dot2)
		);
		/*
		const MiVec3 qv(x,y,z);
		return (v*(w*w-0.5f) + (qv.cross(v))*w + qv*(qv.dot(v)))*2;
		*/
	}

	/**
	inverse rotates passed vec by this (assumed unitary)
	*/
	 MI_FORCE_INLINE const MiVec3 rotateInv(const MiVec3& v) const
//	 MI_INLINE const MiVec3 rotateInv(const MiVec3& v) const
	{
		const MiF32 vx = 2.0f*v.x;
		const MiF32 vy = 2.0f*v.y;
		const MiF32 vz = 2.0f*v.z;
		const MiF32 w2 = w*w-0.5f;
		const MiF32 dot2 = (x*vx + y*vy +z*vz);
		return MiVec3
		(
			(vx*w2 - (y * vz - z * vy)*w + x*dot2), 
			(vy*w2 - (z * vx - x * vz)*w + y*dot2), 
			(vz*w2 - (x * vy - y * vx)*w + z*dot2)
		);
//		const MiVec3 qv(x,y,z);
//		return (v*(w*w-0.5f) - (qv.cross(v))*w + qv*(qv.dot(v)))*2;
	}

	/**
	\brief Assignment operator
	*/
	 MI_FORCE_INLINE	MiQuat&	operator=(const MiQuat& p)			{ x = p.x; y = p.y; z = p.z; w = p.w;	return *this;		}

	 MI_FORCE_INLINE MiQuat& operator*= (const MiQuat& q)
	{
		const MiF32 tx = w*q.x + q.w*x + y*q.z - q.y*z;
		const MiF32 ty = w*q.y + q.w*y + z*q.x - q.z*x;
		const MiF32 tz = w*q.z + q.w*z + x*q.y - q.x*y;

		w = w*q.w - q.x*x - y*q.y - q.z*z;
		x = tx;
		y = ty;
		z = tz;

		return *this;
	}

	 MI_FORCE_INLINE MiQuat& operator+= (const MiQuat& q)
	{
		x+=q.x;
		y+=q.y;
		z+=q.z;
		w+=q.w;
		return *this;
	}

	 MI_FORCE_INLINE MiQuat& operator-= (const MiQuat& q)
	{
		x-=q.x;
		y-=q.y;
		z-=q.z;
		w-=q.w;
		return *this;
	}

	 MI_FORCE_INLINE MiQuat& operator*= (const MiF32 s)
	{
		x*=s;
		y*=s;
		z*=s;
		w*=s;
		return *this;
	}

	/** quaternion multiplication */
	 MI_INLINE MiQuat operator*(const MiQuat& q) const
	{
		return MiQuat(w*q.x + q.w*x + y*q.z - q.y*z,
					  w*q.y + q.w*y + z*q.x - q.z*x,
					  w*q.z + q.w*z + x*q.y - q.x*y,
					  w*q.w - x*q.x - y*q.y - z*q.z);
	}

	/** quaternion addition */
	 MI_FORCE_INLINE MiQuat operator+(const MiQuat& q) const
	{
		return MiQuat(x+q.x,y+q.y,z+q.z,w+q.w);
	}

	/** quaternion subtraction */
	 MI_FORCE_INLINE MiQuat operator-() const
	{
		return MiQuat(-x,-y,-z,-w);
	}


	 MI_FORCE_INLINE MiQuat operator-(const MiQuat& q) const
	{
		return MiQuat(x-q.x,y-q.y,z-q.z,w-q.w);
	}


	 MI_FORCE_INLINE MiQuat operator*(MiF32 r) const
	{
		return MiQuat(x*r,y*r,z*r,w*r);
	}

	static  MI_INLINE MiQuat createIdentity() { return MiQuat(0,0,0,1); }

	/** the quaternion elements */
	MiF32 x,y,z,w;
};

/*!
Basic mathematical 3x3 matrix

Some clarifications, as there have been much confusion about matrix formats etc in the past.

Short:
- Matrix have base vectors in columns (vectors are column matrices, 3x1 matrices).
- Matrix is physically stored in column major format
- Matrices are concaternated from left

Long:
Given three base vectors a, b and c the matrix is stored as
         
|a.x b.x c.x|
|a.y b.y c.y|
|a.z b.z c.z|

Vectors are treated as columns, so the vector v is 

|x|
|y|
|z|

And matrices are applied _before_ the vector (pre-multiplication)
v' = M*v

|x'|   |a.x b.x c.x|   |x|   |a.x*x + b.x*y + c.x*z|
|y'| = |a.y b.y c.y| * |y| = |a.y*x + b.y*y + c.y*z|
|z'|   |a.z b.z c.z|   |z|   |a.z*x + b.z*y + c.z*z|


Physical storage and indexing:
To be compatible with popular 3d rendering APIs (read D3d and OpenGL)
the physical indexing is

|0 3 6|
|1 4 7|
|2 5 8|

index = column*3 + row

which in C++ translates to M[column][row]

The mathematical indexing is M_row,column and this is what is used for _-notation 
so _12 is 1st row, second column and operator(row, column)!

*/
class MiMat33
{
public:
	//! Default constructor
	 MI_INLINE MiMat33()
	{}

	//! Construct from three base vectors
	 MiMat33(const MiVec3& col0, const MiVec3& col1, const MiVec3& col2)
		: column0(col0), column1(col1), column2(col2)
	{}

	//! Construct from float[9]
	 explicit MI_INLINE MiMat33(MiF32 values[]):
		column0(values[0],values[1],values[2]),
		column1(values[3],values[4],values[5]),
		column2(values[6],values[7],values[8])
	{
	}

	//! Construct from a quaternion
	 explicit MI_FORCE_INLINE MiMat33(const MiQuat& q)
	{
		const MiF32 x = q.x;
		const MiF32 y = q.y;
		const MiF32 z = q.z;
		const MiF32 w = q.w;

		const MiF32 x2 = x + x;
		const MiF32 y2 = y + y;
		const MiF32 z2 = z + z;

		const MiF32 xx = x2*x;
		const MiF32 yy = y2*y;
		const MiF32 zz = z2*z;

		const MiF32 xy = x2*y;
		const MiF32 xz = x2*z;
		const MiF32 xw = x2*w;

		const MiF32 yz = y2*z;
		const MiF32 yw = y2*w;
		const MiF32 zw = z2*w;

		column0 = MiVec3(1.0f - yy - zz, xy + zw, xz - yw);
		column1 = MiVec3(xy - zw, 1.0f - xx - zz, yz + xw);
		column2 = MiVec3(xz + yw, yz - xw, 1.0f - xx - yy);
	}

	//! Copy constructor
	 MI_INLINE MiMat33(const MiMat33& other)
		: column0(other.column0), column1(other.column1), column2(other.column2)
	{}

	//! Assignment operator
	 MI_FORCE_INLINE MiMat33& operator=(const MiMat33& other)
	{
		column0 = other.column0;
		column1 = other.column1;
		column2 = other.column2;
		return *this;
	}

	//! Set to identity matrix
	 MI_INLINE static MiMat33 createIdentity()
	{
		return MiMat33(MiVec3(1,0,0), MiVec3(0,1,0), MiVec3(0,0,1));
	}

	//! Set to zero matrix
	 MI_INLINE static MiMat33 createZero()
	{
		return MiMat33(MiVec3(0.0f), MiVec3(0.0f), MiVec3(0.0f));
	}

	//! Construct from diagonal, off-diagonals are zero.
	 MI_INLINE static MiMat33 createDiagonal(const MiVec3& d)
	{
		return MiMat33(MiVec3(d.x,0.0f,0.0f), MiVec3(0.0f,d.y,0.0f), MiVec3(0.0f,0.0f,d.z));
	}


	//! Get transposed matrix
	 MI_FORCE_INLINE MiMat33 getTranspose() const
	{
		const MiVec3 v0(column0.x, column1.x, column2.x);
		const MiVec3 v1(column0.y, column1.y, column2.y);
		const MiVec3 v2(column0.z, column1.z, column2.z);

		return MiMat33(v0,v1,v2);   
	}

	//! Get the real inverse
	 MI_INLINE MiMat33 getInverse() const
	{
		const MiF32 det = getDeterminant();
		MiMat33 inverse;

		if(det != 0)
		{
			const MiF32 invDet = 1.0f/det;

			inverse.column0[0] = invDet * (column1[1]*column2[2] - column2[1]*column1[2]);							
			inverse.column0[1] = invDet *-(column0[1]*column2[2] - column2[1]*column0[2]);
			inverse.column0[2] = invDet * (column0[1]*column1[2] - column0[2]*column1[1]);

			inverse.column1[0] = invDet *-(column1[0]*column2[2] - column1[2]*column2[0]);
			inverse.column1[1] = invDet * (column0[0]*column2[2] - column0[2]*column2[0]);
			inverse.column1[2] = invDet *-(column0[0]*column1[2] - column0[2]*column1[0]);

			inverse.column2[0] = invDet * (column1[0]*column2[1] - column1[1]*column2[0]);
			inverse.column2[1] = invDet *-(column0[0]*column2[1] - column0[1]*column2[0]);
			inverse.column2[2] = invDet * (column0[0]*column1[1] - column1[0]*column0[1]);

			return inverse;
		}
		else
		{
			return createIdentity();
		}
	}

	//! Get determinant
	 MI_INLINE MiF32 getDeterminant() const
	{
		return column0.dot(column1.cross(column2));
	}

	//! Unary minus
	 MI_INLINE MiMat33 operator-() const
	{
		return MiMat33(-column0, -column1, -column2);
	}

	//! Add
	 MI_INLINE MiMat33 operator+(const MiMat33& other) const
	{
		return MiMat33( column0+other.column0,
					  column1+other.column1,
					  column2+other.column2);
	}

	//! Subtract
	 MI_INLINE MiMat33 operator-(const MiMat33& other) const
	{
		return MiMat33( column0-other.column0,
					  column1-other.column1,
					  column2-other.column2);
	}

	//! Scalar multiplication
	 MI_INLINE MiMat33 operator*(MiF32 scalar) const
	{
		return MiMat33(column0*scalar, column1*scalar, column2*scalar);
	}

	friend MiMat33 operator*(MiF32, const MiMat33&);

	//! Matrix vector multiplication (returns 'this->transform(vec)')
	 MI_INLINE MiVec3 operator*(const MiVec3& vec) const
	{
		return transform(vec);
	}

	//! Matrix multiplication
	 MI_FORCE_INLINE MiMat33 operator*(const MiMat33& other) const
	{
		//Rows from this <dot> columns from other
		//column0 = transform(other.column0) etc
		return MiMat33(transform(other.column0), transform(other.column1), transform(other.column2));
	}

	// a <op>= b operators

	//! Equals-add
	 MI_INLINE MiMat33& operator+=(const MiMat33& other)
	{
		column0 += other.column0;
		column1 += other.column1;
		column2 += other.column2;
		return *this;
	}

	//! Equals-sub
	 MI_INLINE MiMat33& operator-=(const MiMat33& other)
	{
		column0 -= other.column0;
		column1 -= other.column1;
		column2 -= other.column2;
		return *this;
	}

	//! Equals scalar multiplication
	 MI_INLINE MiMat33& operator*=(MiF32 scalar)
	{
		column0 *= scalar;
		column1 *= scalar;
		column2 *= scalar;
		return *this;
	}

	//! Element access, mathematical way!
	 MI_FORCE_INLINE MiF32 operator()(unsigned int row, unsigned int col) const
	{
		return (*this)[col][row];
	}

	//! Element access, mathematical way!
	 MI_FORCE_INLINE MiF32& operator()(unsigned int row, unsigned int col)
	{
		return (*this)[col][row];
	}

	// Transform etc
	
	//! Transform vector by matrix, equal to v' = M*v
	 MI_FORCE_INLINE MiVec3 transform(const MiVec3& other) const
	{
		return column0*other.x + column1*other.y + column2*other.z;
	}

	//! Transform vector by matrix transpose, v' = M^t*v
	 MI_INLINE MiVec3 transformTranspose(const MiVec3& other) const
	{
		return MiVec3(	column0.dot(other),
						column1.dot(other),
						column2.dot(other));
	}

	 MI_FORCE_INLINE const MiF32* front() const
	{
		return &column0.x;
	}

	 MI_FORCE_INLINE			MiVec3& operator[](int num)			{return (&column0)[num];}
	 MI_FORCE_INLINE const	MiVec3& operator[](int num) const	{return (&column0)[num];}

	//Data, see above for format!

	MiVec3 column0, column1, column2; //the three base vectors
};

// implementation from MiQuat.h
 MI_INLINE MiQuat::MiQuat(const MiMat33& m)
{
	MiF32 tr = m(0,0) + m(1,1) + m(2,2), h;
	if(tr >= 0)
	{
		h = MiSqrt(tr +1);
		w = MiF32(0.5) * h;
		h = MiF32(0.5) / h;

		x = (m(2,1) - m(1,2)) * h;
		y = (m(0,2) - m(2,0)) * h;
		z = (m(1,0) - m(0,1)) * h;
	}
	else
	{
		int i = 0; 
		if (m(1,1) > m(0,0))
			i = 1; 
		if(m(2,2) > m(i,i))
			i=2; 
		switch (i)
		{
		case 0:
			h = MiSqrt((m(0,0) - (m(1,1) + m(2,2))) + 1);
			x = MiF32(0.5) * h;
			h = MiF32(0.5) / h;

			y = (m(0,1) + m(1,0)) * h; 
			z = (m(2,0) + m(0,2)) * h;
			w = (m(2,1) - m(1,2)) * h;
			break;
		case 1:
			h = MiSqrt((m(1,1) - (m(2,2) + m(0,0))) + 1);
			y = MiF32(0.5) * h;
			h = MiF32(0.5) / h;

			z = (m(1,2) + m(2,1)) * h;
			x = (m(0,1) + m(1,0)) * h;
			w = (m(0,2) - m(2,0)) * h;
			break;
		case 2:
			h = MiSqrt((m(2,2) - (m(0,0) + m(1,1))) + 1);
			z = MiF32(0.5) * h;
			h = MiF32(0.5) / h;

			x = (m(2,0) + m(0,2)) * h;
			y = (m(1,2) + m(2,1)) * h;
			w = (m(1,0) - m(0,1)) * h;
		}
	}
}

class MiMat44
{
public:
	//! Default constructor
	 MI_INLINE MiMat44()
	{}

	//! Construct from four 4-vectors
	 MiMat44(const MiVec4& col0, const MiVec4& col1, const MiVec4& col2, const MiVec4 &col3)
		: column0(col0), column1(col1), column2(col2), column3(col3)
	{}

	//! Construct from three base vectors and a translation
	 MiMat44(const MiVec3& column0, const MiVec3& column1, const MiVec3& column2, const MiVec3& column3)
		: column0(column0,0), column1(column1,0), column2(column2,0), column3(column3,1)
	{}

	//! Construct from float[16]
	explicit  MI_INLINE MiMat44(MiF32 values[]):
	column0(values[0],values[1],values[2], values[3]),
		column1(values[4],values[5],values[6], values[7]),
		column2(values[8],values[9],values[10], values[11]),
		column3(values[12], values[13], values[14], values[15])
	{
	}

	//! Construct from a quaternion
	explicit  MI_INLINE MiMat44(const MiQuat& q)
	{
		const MiF32 x = q.x;
		const MiF32 y = q.y;
		const MiF32 z = q.z;
		const MiF32 w = q.w;

		const MiF32 x2 = x + x;
		const MiF32 y2 = y + y;
		const MiF32 z2 = z + z;

		const MiF32 xx = x2*x;
		const MiF32 yy = y2*y;
		const MiF32 zz = z2*z;

		const MiF32 xy = x2*y;
		const MiF32 xz = x2*z;
		const MiF32 xw = x2*w;

		const MiF32 yz = y2*z;
		const MiF32 yw = y2*w;
		const MiF32 zw = z2*w;

		column0 = MiVec4(1.0f - yy - zz, xy + zw, xz - yw, 0.0f);
		column1 = MiVec4(xy - zw, 1.0f - xx - zz, yz + xw, 0.0f);
		column2 = MiVec4(xz + yw, yz - xw, 1.0f - xx - yy, 0.0f);
		column3 = MiVec4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	//! Construct from a diagonal vector
	explicit  MI_INLINE MiMat44(const MiVec4& diagonal):
	column0(diagonal.x,0.0f,0.0f,0.0f),
		column1(0.0f,diagonal.y,0.0f,0.0f),
		column2(0.0f,0.0f,diagonal.z,0.0f),
		column3(0.0f,0.0f,0.0f,diagonal.w)
	{
	}

	 MiMat44(const MiMat33& orientation, const MiVec3& position):
	column0(orientation.column0,0.0f),
		column1(orientation.column1,0.0f),
		column2(orientation.column2,0.0f),
		column3(position,1)
	{
	}

	//! Copy constructor
	 MI_INLINE MiMat44(const MiMat44& other)
		: column0(other.column0), column1(other.column1), column2(other.column2), column3(other.column3)
	{}

	//! Assignment operator
	 MI_INLINE const MiMat44& operator=(const MiMat44& other)
	{
		column0 = other.column0;
		column1 = other.column1;
		column2 = other.column2;
		column3 = other.column3;
		return *this;
	}

	 MI_INLINE static MiMat44 createIdentity()
	{
		return MiMat44(
			MiVec4(1.0f,0.0f,0.0f,0.0f),
			MiVec4(0.0f,1.0f,0.0f,0.0f),
			MiVec4(0.0f,0.0f,1.0f,0.0f),
			MiVec4(0.0f,0.0f,0.0f,1.0f));
	}


	 MI_INLINE static MiMat44 createZero()
	{
		return MiMat44(MiVec4(0.0f), MiVec4(0.0f), MiVec4(0.0f), MiVec4(0.0f));
	}

	//! Get transposed matrix
	 MI_INLINE MiMat44 getTranspose() const
	{
		return MiMat44(MiVec4(column0.x, column1.x, column2.x, column3.x),
			MiVec4(column0.y, column1.y, column2.y, column3.y),
			MiVec4(column0.z, column1.z, column2.z, column3.z),
			MiVec4(column0.w, column1.w, column2.w, column3.w));
	}

	//! Unary minus
	 MI_INLINE MiMat44 operator-() const
	{
		return MiMat44(-column0, -column1, -column2, -column3);
	}

	//! Add
	 MI_INLINE MiMat44 operator+(const MiMat44& other) const
	{
		return MiMat44( column0+other.column0,
			column1+other.column1,
			column2+other.column2,
			column3+other.column3);
	}

	//! Subtract
	 MI_INLINE MiMat44 operator-(const MiMat44& other) const
	{
		return MiMat44( column0-other.column0,
			column1-other.column1,
			column2-other.column2,
			column3-other.column3);
	}

	//! Scalar multiplication
	 MI_INLINE MiMat44 operator*(MiF32 scalar) const
	{
		return MiMat44(column0*scalar, column1*scalar, column2*scalar, column3*scalar);
	}

	friend MiMat44 operator*(MiF32, const MiMat44&);

	//! Matrix multiplication
	 MI_INLINE MiMat44 operator*(const MiMat44& other) const
	{
		//Rows from this <dot> columns from other
		//column0 = transform(other.column0) etc
		return MiMat44(transform(other.column0), transform(other.column1), transform(other.column2), transform(other.column3));
	}

	// a <op>= b operators

	//! Equals-add
	 MI_INLINE MiMat44& operator+=(const MiMat44& other)
	{
		column0 += other.column0;
		column1 += other.column1;
		column2 += other.column2;
		column3 += other.column3;
		return *this;
	}

	//! Equals-sub
	 MI_INLINE MiMat44& operator-=(const MiMat44& other)
	{
		column0 -= other.column0;
		column1 -= other.column1;
		column2 -= other.column2;
		column3 -= other.column3;
		return *this;
	}

	//! Equals scalar multiplication
	 MI_INLINE MiMat44& operator*=(MiF32 scalar)
	{
		column0 *= scalar;
		column1 *= scalar;
		column2 *= scalar;
		column3 *= scalar;
		return *this;
	}

	//! Element access, mathematical way!
	 MI_FORCE_INLINE MiF32 operator()(unsigned int row, unsigned int col) const
	{
		return (*this)[col][row];
	}

	//! Element access, mathematical way!
	 MI_FORCE_INLINE MiF32& operator()(unsigned int row, unsigned int col)
	{
		return (*this)[col][row];
	}

	//! Transform vector by matrix, equal to v' = M*v
	 MI_INLINE MiVec4 transform(const MiVec4& other) const
	{
		return column0*other.x + column1*other.y + column2*other.z + column3*other.w;
	}

	//! Transform vector by matrix, equal to v' = M*v
	 MI_INLINE MiVec3 transform(const MiVec3& other) const
	{
		return transform(MiVec4(other,1)).getXYZ();
	}

	//! Rotate vector by matrix, equal to v' = M*v
	 MI_INLINE MiVec4 rotate(const MiVec4& other) const
	{
		return column0*other.x + column1*other.y + column2*other.z;// + column3*0;
	}

	//! Rotate vector by matrix, equal to v' = M*v
	 MI_INLINE MiVec3 rotate(const MiVec3& other) const
	{
		return rotate(MiVec4(other,1)).getXYZ();
	}


	 MI_INLINE MiVec3 getBasis(int num) const
	{
		MI_ASSERT(num>=0 && num<3);
		return (&column0)[num].getXYZ();
	}

	 MI_INLINE MiVec3 getPosition() const
	{
		return column3.getXYZ();
	}

	 MI_INLINE void setPosition(const MiVec3& position)
	{
		column3.x = position.x;
		column3.y = position.y;
		column3.z = position.z;
	}

	 MI_FORCE_INLINE const MiF32* front() const
	{
		return &column0.x;
	}

	 MI_FORCE_INLINE			MiVec4& operator[](int num)			{return (&column0)[num];}
	 MI_FORCE_INLINE const	MiVec4& operator[](int num)	const	{return (&column0)[num];}

	 MI_INLINE void	scale(const MiVec4& p)
	{
		column0 *= p.x;
		column1 *= p.y;
		column2 *= p.z;
		column3 *= p.w;
	}

	 MI_INLINE MiMat44 inverseRT(void) const
	{
		MiVec3 r0(column0.x, column1.x, column2.x),
			r1(column0.y, column1.y, column2.y),
			r2(column0.z, column1.z, column2.z);

		return MiMat44(r0, r1, r2, -(r0 * column3.x + r1 * column3.y + r2 * column3.z));
	}

	 MI_INLINE bool isFinite() const
	{
		return column0.isFinite() && column1.isFinite() && column2.isFinite() && column3.isFinite();
	}


	//Data, see above for format!

	MiVec4 column0, column1, column2, column3; //the four base vectors
};


/**
\brief Class representing 3D range or axis aligned bounding box.

Stored as minimum and maximum extent corners. Alternate representation
would be center and dimensions.
May be empty or nonempty. If not empty, minimum <= maximum has to hold.
*/
class MiBounds3
{
public:

	/**
	\brief Default constructor, not performing any initialization for performance reason.
	\remark Use empty() function below to construct empty bounds.
	*/
	 MI_FORCE_INLINE MiBounds3()	{}

	/**
	\brief Construct from two bounding points
	*/
	 MI_FORCE_INLINE MiBounds3(const MiVec3& minimum, const MiVec3& maximum);

	/**
	\brief Return empty bounds. 
	*/
	static  MI_FORCE_INLINE MiBounds3 empty();

	/**
	\brief returns the AABB containing v0 and v1.
	\param v0 first point included in the AABB.
	\param v1 second point included in the AABB.
	*/
	static  MI_FORCE_INLINE MiBounds3 boundsOfPoints(const MiVec3& v0, const MiVec3& v1);

	/**
	\brief returns the AABB from center and extents vectors.
	\param center Center vector
	\param extent Extents vector
	*/
	static  MI_FORCE_INLINE MiBounds3 centerExtents(const MiVec3& center, const MiVec3& extent);

	/**
	\brief Construct from center, extent, and (not necessarily orthogonal) basis
	*/
	static  MI_INLINE MiBounds3 basisExtent(const MiVec3& center, const MiMat33& basis, const MiVec3& extent);

	/**
	\brief gets the transformed bounds of the passed AABB (resulting in a bigger AABB).
	\param[in] matrix Transform to apply, can contain scaling as well
	\param[in] bounds The bounds to transform.
	*/
	static  MI_INLINE MiBounds3 transform(const MiMat33& matrix, const MiBounds3& bounds);

	/**
	\brief Sets empty to true
	*/
	 MI_FORCE_INLINE void setEmpty();

	/**
	\brief Sets infinite bounds
	*/
	 MI_FORCE_INLINE void setInfinite();

	/**
	\brief expands the volume to include v
	\param v Point to expand to.
	*/
	 MI_FORCE_INLINE void include(const MiVec3& v);

	/**
	\brief expands the volume to include b.
	\param b Bounds to perform union with.
	*/
	 MI_FORCE_INLINE void include(const MiBounds3& b);

	 MI_FORCE_INLINE bool isEmpty() const;

	/**
	\brief indicates whether the intersection of this and b is empty or not.
	\param b Bounds to test for intersection.
	*/
	 MI_FORCE_INLINE bool intersects(const MiBounds3& b) const;

	/**
	 \brief computes the 1D-intersection between two AABBs, on a given axis.
	 \param	a		the other AABB
	 \param	axis	the axis (0, 1, 2)
	 */
	 MI_FORCE_INLINE bool intersects1D(const MiBounds3& a, MiU32 axis)	const;

	/**
	\brief indicates if these bounds contain v.
	\param v Point to test against bounds.
	*/
	 MI_FORCE_INLINE bool contains(const MiVec3& v) const;

	/**
	 \brief	checks a box is inside another box.
	 \param	box		the other AABB
	 */
	 MI_FORCE_INLINE bool isInside(const MiBounds3& box) const;

	/**
	\brief returns the center of this axis aligned box.
	*/
	 MI_FORCE_INLINE MiVec3 getCenter() const;

	/**
	\brief get component of the box's center along a given axis
	*/
	 MI_FORCE_INLINE float	getCenter(MiU32 axis)	const;

	/**
	\brief get component of the box's extents along a given axis
	*/
	 MI_FORCE_INLINE float	getExtents(MiU32 axis)	const;

	/**
	\brief returns the dimensions (width/height/depth) of this axis aligned box.
	*/
	 MI_FORCE_INLINE MiVec3 getDimensions() const;

	/**
	\brief returns the extents, which are half of the width/height/depth.
	*/
	 MI_FORCE_INLINE MiVec3 getExtents() const;

	/**
	\brief scales the AABB.
	\param scale Factor to scale AABB by.
	*/
	 MI_FORCE_INLINE void scale(MiF32 scale);

	/** 
	fattens the AABB in all 3 dimensions by the given distance. 
	*/
	 MI_FORCE_INLINE void fatten(MiF32 distance);

	/** 
	checks that the AABB values are not NaN
	*/

	 MI_FORCE_INLINE bool isFinite() const;

	MiVec3 minimum, maximum;
};


 MI_FORCE_INLINE MiBounds3::MiBounds3(const MiVec3& minimum, const MiVec3& maximum)
: minimum(minimum), maximum(maximum)
{
}

 MI_FORCE_INLINE MiBounds3 MiBounds3::empty()
{
	return MiBounds3(MiVec3(MI_MAX_REAL), MiVec3(-MI_MAX_REAL));
}

 MI_FORCE_INLINE bool MiBounds3::isFinite() const
	{
	return minimum.isFinite() && maximum.isFinite();
}

 MI_FORCE_INLINE MiBounds3 MiBounds3::boundsOfPoints(const MiVec3& v0, const MiVec3& v1)
{
	return MiBounds3(v0.minimum(v1), v0.maximum(v1));
}

 MI_FORCE_INLINE MiBounds3 MiBounds3::centerExtents(const MiVec3& center, const MiVec3& extent)
{
	return MiBounds3(center - extent, center + extent);
}

 MI_INLINE MiBounds3 MiBounds3::basisExtent(const MiVec3& center, const MiMat33& basis, const MiVec3& extent)
{
	// extended basis vectors
	MiVec3 c0 = basis.column0 * extent.x;
	MiVec3 c1 = basis.column1 * extent.y;
	MiVec3 c2 = basis.column2 * extent.z;

	MiVec3 w;
	// find combination of base vectors that produces max. distance for each component = sum of abs()
	w.x = MiAbs(c0.x) + MiAbs(c1.x) + MiAbs(c2.x);
	w.y = MiAbs(c0.y) + MiAbs(c1.y) + MiAbs(c2.y);
	w.z = MiAbs(c0.z) + MiAbs(c1.z) + MiAbs(c2.z);

	return MiBounds3(center - w, center + w);
}

 MI_FORCE_INLINE void MiBounds3::setEmpty()
{
	minimum = MiVec3(MI_MAX_REAL);
	maximum = MiVec3(-MI_MAX_REAL);
}

 MI_FORCE_INLINE void MiBounds3::setInfinite()
{
	minimum = MiVec3(-MI_MAX_REAL);
	maximum = MiVec3(MI_MAX_REAL);
}

 MI_FORCE_INLINE void MiBounds3::include(const MiVec3& v)
{
	MI_ASSERT(isFinite());
	minimum = minimum.minimum(v);
	maximum = maximum.maximum(v);
}

 MI_FORCE_INLINE void MiBounds3::include(const MiBounds3& b)
{
	MI_ASSERT(isFinite());
	minimum = minimum.minimum(b.minimum);
	maximum = maximum.maximum(b.maximum);
}

 MI_FORCE_INLINE bool MiBounds3::isEmpty() const
{
	MI_ASSERT(isFinite());
	// Consistency condition for (Min, Max) boxes: minimum < maximum
	return minimum.x > maximum.x || minimum.y > maximum.y || minimum.z > maximum.z;
}

 MI_FORCE_INLINE bool MiBounds3::intersects(const MiBounds3& b) const
{
	MI_ASSERT(isFinite() && b.isFinite());
	return !(b.minimum.x > maximum.x || minimum.x > b.maximum.x ||
			 b.minimum.y > maximum.y || minimum.y > b.maximum.y ||
			 b.minimum.z > maximum.z || minimum.z > b.maximum.z);
}

 MI_FORCE_INLINE bool MiBounds3::intersects1D(const MiBounds3& a, MiU32 axis)	const
{
	MI_ASSERT(isFinite() && a.isFinite());
	return maximum[axis] >= a.minimum[axis] && a.maximum[axis] >= minimum[axis];
}

 MI_FORCE_INLINE bool MiBounds3::contains(const MiVec3& v) const
{
	MI_ASSERT(isFinite());

	return !(v.x < minimum.x || v.x > maximum.x ||
		v.y < minimum.y || v.y > maximum.y ||
		v.z < minimum.z || v.z > maximum.z);
}

 MI_FORCE_INLINE bool MiBounds3::isInside(const MiBounds3& box) const
{
	MI_ASSERT(isFinite() && box.isFinite());
	if(box.minimum.x>minimum.x)	return false;
	if(box.minimum.y>minimum.y)	return false;
	if(box.minimum.z>minimum.z)	return false;
	if(box.maximum.x<maximum.x)	return false;
	if(box.maximum.y<maximum.y)	return false;
	if(box.maximum.z<maximum.z)	return false;
	return true;
}

 MI_FORCE_INLINE MiVec3 MiBounds3::getCenter() const
{
	MI_ASSERT(isFinite());
	return (minimum+maximum) * MiF32(0.5);
}

 MI_FORCE_INLINE float	MiBounds3::getCenter(MiU32 axis)	const
{
	MI_ASSERT(isFinite());
	return (minimum[axis] + maximum[axis]) * MiF32(0.5);
}

 MI_FORCE_INLINE float	MiBounds3::getExtents(MiU32 axis)	const
{
	MI_ASSERT(isFinite());
	return (maximum[axis] - minimum[axis]) * MiF32(0.5);
}

 MI_FORCE_INLINE MiVec3 MiBounds3::getDimensions() const
{
	MI_ASSERT(isFinite());
	return maximum - minimum;
}

 MI_FORCE_INLINE MiVec3 MiBounds3::getExtents() const
{
	MI_ASSERT(isFinite());
	return getDimensions() * MiF32(0.5);
}

 MI_FORCE_INLINE void MiBounds3::scale(MiF32 scale)
{
	MI_ASSERT(isFinite());
	*this = centerExtents(getCenter(), getExtents() * scale);
}

 MI_FORCE_INLINE void MiBounds3::fatten(MiF32 distance)
{
	MI_ASSERT(isFinite());
	minimum.x -= distance;
	minimum.y -= distance;
	minimum.z -= distance;

	maximum.x += distance;
	maximum.y += distance;
	maximum.z += distance;
}

 MI_INLINE MiBounds3 MiBounds3::transform(const MiMat33& matrix, const MiBounds3& bounds)
{
	MI_ASSERT(bounds.isFinite());
	return bounds.isEmpty() ? bounds :
		MiBounds3::basisExtent(matrix * bounds.getCenter(), matrix, bounds.getExtents());
}


};

#endif
