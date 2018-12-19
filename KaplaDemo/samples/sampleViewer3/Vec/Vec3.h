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

#ifndef VEC3_H
#define VEC3_H

#include <math.h>

//	Singe / VecReal Precision Vec 3
//  Matthias Mueller
//  derived from NxVec3.h

namespace M
{

#define VEC3_DOUBLE 0

#if VEC3_DOUBLE

typedef double VecReal;
#define MAX_VEC_REAL DBL_MAX 
#define MIN_VEC_REAL -DBL_MAX 
#define VEC_PI 3.14159265358979323846

inline int vecFloor(VecReal f ) { return (int)::floor(f); }
inline VecReal vecSqrt(VecReal f) { return ::sqrt(f); }
inline VecReal vecSin(VecReal f) { return ::sin(f); }
inline VecReal vecCos(VecReal f) { return ::cos(f); }
inline VecReal vecAbs(VecReal f) { return ::abs(f); }
inline VecReal vecAtan2(VecReal y, VecReal x) { return ::atan2(y, x); }
inline VecReal vecASin(VecReal f) { return ::asin(f); }
inline VecReal vecACos(VecReal f) { return ::acos(f); }
inline VecReal vecATan(VecReal f) { return ::atan(f); }

#else

typedef float VecReal;
#define MAX_VEC_REAL FLT_MAX 
#define MIN_VEC_REAL -FLT_MAX 
#define VEC_PI 3.14159265358979323846f

inline int vecFloor(VecReal f ) { return (int)::floorf(f); }
inline VecReal vecSqrt(VecReal f) { return ::sqrtf(f); }
inline VecReal vecSin(VecReal f) { return ::sinf(f); }
inline VecReal vecCos(VecReal f) { return ::cosf(f); }
inline VecReal vecAbs(VecReal f) { return ::fabs(f); }
inline VecReal vecAtan2(VecReal y, VecReal x) { return ::atan2f(y, x); }
inline VecReal vecASin(VecReal f) { return ::asinf(f); }
inline VecReal vecACos(VecReal f) { return ::acosf(f); }
inline VecReal vecATan(VecReal f) { return ::atanf(f); }

#endif


/**
\brief Enum to classify an axis.
*/
	enum DAxisType
	{
		D_AXIS_PLUS_X,
		D_AXIS_MINUS_X,
		D_AXIS_PLUS_Y,
		D_AXIS_MINUS_Y,
		D_AXIS_PLUS_Z,
		D_AXIS_MINUS_Z,
		D_AXIS_ARBITRARY
	};

// -------------------------------------------------------------------------------------
class Vec3
{
public:
	VecReal x,y,z;

	Vec3() {};
	Vec3(VecReal _x, VecReal _y, VecReal _z) : x(_x), y(_y), z(_z) {}
	Vec3(const VecReal v[]) : x(v[0]), y(v[1]), z(v[2]) {}

	const VecReal* Vec3::get() const { return &x;}
	VecReal* get() { return &x; }
	void  get(VecReal * v) const
	{
		v[0] = x;
		v[1] = y;
		v[2] = z;
	}

	VecReal& operator[](int index)
	{
		return (&x)[index];
	}

	const VecReal  operator[](int index) const
	{
		return (&x)[index];
	}

	void setx(const VecReal & d) 
	{ 
		x = d; 
	}


	void sety(const VecReal & d) 
	{ 
		y = d; 
	}


	void setz(const VecReal & d) 
	{ 
		z = d; 
	}

	Vec3 getNormalized() const
	{
		const VecReal m = magnitudeSquared();
		return m>0 ? *this * 1.0 / vecSqrt(m) : Vec3(0,0,0);
	}

	//Operators

	bool operator< (const Vec3&v) const
	{
		return ((x < v.x)&&(y < v.y)&&(z < v.z));
	}


	bool operator==(const Vec3& v) const
	{
		return ((x == v.x)&&(y == v.y)&&(z == v.z));
	}


	bool operator!=(const Vec3& v) const
	{
		return ((x != v.x)||(y != v.y)||(z != v.z));
	}

	//Methods

	void  set(const Vec3 & v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
	}


	void  setNegative(const Vec3 & v)
	{
		x = -v.x;
		y = -v.y;
		z = -v.z;
	}


	void  setNegative()
	{
		x = -x;
		y = -y;
		z = -z;
	}



	void  set(const VecReal * v)
	{
		x = v[0];
		y = v[1];
		z = v[2];
	}


	void  set(VecReal _x, VecReal _y, VecReal _z)
	{
		this->x = _x;
		this->y = _y;
		this->z = _z;
	}


	void set(VecReal v)
	{
		x = v;
		y = v;
		z = v;
	}


	void  zero()
	{
		x = y = z = 0.0;
	}


	void  setPlusInfinity()
	{
		x = y = z = MAX_VEC_REAL; 
	}


	void  setMinusInfinity()
	{
		x = y = z = MIN_VEC_REAL; 
	}


	void max(const Vec3 & v)
	{
		x = x > v.x ? x : v.x;
		y = y > v.y ? y : v.y;
		z = z > v.z ? z : v.z;
	}

	void min(const Vec3 & v)
	{
		x = x < v.x ? x : v.x;
		y = y < v.y ? y : v.y;
		z = z < v.z ? z : v.z;
	}




	void  add(const Vec3 & a, const Vec3 & b)
	{
		x = a.x + b.x;
		y = a.y + b.y;
		z = a.z + b.z;
	}


	void  subtract(const Vec3 &a, const Vec3 &b)
	{
		x = a.x - b.x;
		y = a.y - b.y;
		z = a.z - b.z;
	}


	void  arrayMultiply(const Vec3 &a, const Vec3 &b)
	{
		x = a.x * b.x;
		y = a.y * b.y;
		z = a.z * b.z;
	}


	void  multiply(VecReal s,  const Vec3 & a)
	{
		x = a.x * s;
		y = a.y * s;
		z = a.z * s;
	}


	void  multiplyAdd(VecReal s, const Vec3 & a, const Vec3 & b)
	{
		x = s * a.x + b.x;
		y = s * a.y + b.y;
		z = s * a.z + b.z;
	}


	VecReal normalize()
	{
		VecReal m = magnitude();
		if (m)
		{
			const VecReal il =  VecReal(1.0) / m;
			x *= il;
			y *= il;
			z *= il;
		}
		return m;
	}


	void setMagnitude(VecReal length)
	{
		VecReal m = magnitude();
		if(m)
		{
			VecReal newLength = length / m;
			x *= newLength;
			y *= newLength;
			z *= newLength;
		}
	}


	DAxisType snapToClosestAxis()
	{
		const VecReal almostOne = 0.999999f;
		if(x >=  almostOne) { set( 1.0f,  0.0f,  0.0f);	return D_AXIS_PLUS_X ; }
		else	if(x <= -almostOne) { set(-1.0f,  0.0f,  0.0f);	return D_AXIS_MINUS_X; }
		else	if(y >=  almostOne) { set( 0.0f,  1.0f,  0.0f);	return D_AXIS_PLUS_Y ; }
		else	if(y <= -almostOne) { set( 0.0f, -1.0f,  0.0f);	return D_AXIS_MINUS_Y; }
		else	if(z >=  almostOne) { set( 0.0f,  0.0f,  1.0f);	return D_AXIS_PLUS_Z ; }
		else	if(z <= -almostOne) { set( 0.0f,  0.0f, -1.0f);	return D_AXIS_MINUS_Z; }
		else													return D_AXIS_ARBITRARY;
	}


	unsigned int closestAxis() const
	{
		const VecReal* vals = &x;
		unsigned int  m = 0;
		if(abs(vals[1]) > abs(vals[m])) m = 1;
		if(abs(vals[2]) > abs(vals[m])) m = 2;
		return m;
	}


	//const methods

	//bool isFinite() const
	//{
	//	return NxMath::isFinite(x) && NxMath::isFinite(y) && NxMath::isFinite(z);
	//}


	VecReal dot(const Vec3 &v) const
	{
		return x * v.x + y * v.y + z * v.z;
	}


	bool sameDirection(const Vec3 &v) const
	{
		return x*v.x + y*v.y + z*v.z >= 0.0f;
	}


	VecReal magnitude() const
	{
		return sqrt(x * x + y * y + z * z);
	}


	VecReal magnitudeSquared() const
	{
		return x * x + y * y + z * z;
	}


	VecReal distance(const Vec3 & v) const
	{
		VecReal dx = x - v.x;
		VecReal dy = y - v.y;
		VecReal dz = z - v.z;
		return sqrt(dx * dx + dy * dy + dz * dz);
	}


	VecReal distanceSquared(const Vec3 &v) const
	{
		VecReal dx = x - v.x;
		VecReal dy = y - v.y;
		VecReal dz = z - v.z;
		return dx * dx + dy * dy + dz * dz;
	}


	void cross(const Vec3 &left, const Vec3 & right)	//prefered version, w/o temp object.
	{
		// temps needed in case left or right is this.
		VecReal a = (left.y * right.z) - (left.z * right.y);
		VecReal b = (left.z * right.x) - (left.x * right.z);
		VecReal c = (left.x * right.y) - (left.y * right.x);

		x = a;
		y = b;
		z = c;
	}


	bool equals(const Vec3 & v, VecReal epsilon) const
	{
		return 
			abs(x - v.x) < epsilon &&
			abs(y - v.y) < epsilon &&
			abs(z - v.z) < epsilon;
	}



	Vec3 operator -() const
	{
		return Vec3(-x, -y, -z);
	}


	Vec3 operator +(const Vec3 & v) const
	{
		return Vec3(x + v.x, y + v.y, z + v.z);	// RVO version
	}


	Vec3 operator -(const Vec3 & v) const
	{
		return Vec3(x - v.x, y - v.y, z - v.z);	// RVO version
	}



	Vec3 operator *(VecReal f) const
	{
		return Vec3(x * f, y * f, z * f);	// RVO version
	}


	Vec3 operator /(VecReal f) const
	{
		f = VecReal(1.0) / f; return Vec3(x * f, y * f, z * f);
	}


	Vec3& operator +=(const Vec3& v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}


	Vec3& operator -=(const Vec3& v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}


	Vec3& operator *=(VecReal f)
	{
		x *= f;
		y *= f;
		z *= f;

		return *this;
	}


	Vec3& operator /=(VecReal f)
	{
		f = 1.0f/f;
		x *= f;
		y *= f;
		z *= f;

		return *this;
	}


	Vec3 cross(const Vec3& v) const
	{
		Vec3 temp;
		temp.cross(*this,v);
		return temp;
	}


	Vec3 operator^(const Vec3& v) const
	{
		Vec3 temp;
		temp.cross(*this,v);
		return temp;
	}


	VecReal operator|(const Vec3& v) const
	{
		return x * v.x + y * v.y + z * v.z;
	}
};

	/**
	scalar pre-multiplication
	*/

inline Vec3 operator *(VecReal f, const Vec3& v)
	{
		return Vec3(f * v.x, f * v.y, f * v.z);
	}


}
	/** @} */
#endif
