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


#ifndef APEX_CSG_MATH_H
#define APEX_CSG_MATH_H

#include "ApexUsingNamespace.h"
#include "PxMath.h"
#include "PxVec3.h"

#include <math.h>
#include <float.h>

#ifndef WITHOUT_APEX_AUTHORING


// APEX_CSG_DBL may be defined externally
#ifndef APEX_CSG_DBL
#define APEX_CSG_DBL	1
#endif


namespace ApexCSG
{
#if !(APEX_CSG_DBL)
typedef	float	Real;
#define	MAX_REAL		FLT_MAX
#define	EPS_REAL		FLT_EPSILON
#else
typedef	double	Real;
#define	MAX_REAL		DBL_MAX
#define	EPS_REAL		DBL_EPSILON
#endif


/* Utilities */

template<typename T>
T square(T t)
{
	return t * t;
}


/* Linear algebra */

#define ALL_i( _D, _exp )	for( int i = 0; i < _D; ++i ) { _exp; }


/* General vector */

template<typename T, int D>
class Vec
{
public:

	PX_INLINE				Vec()									{}
	PX_INLINE	 			Vec(const T& v)
	{
		set(v);
	}
	PX_INLINE	 			Vec(const T* v)
	{
		set(v);
	}

	PX_INLINE	void		set(const T& v)
	{
		ALL_i(D, el[i] = v);
	}
	PX_INLINE	void		set(const T* v)
	{
		ALL_i(D, el[i] = v[i]);
	}

	PX_INLINE	T&			operator [](int i)
	{
		return el[i];
	}
	PX_INLINE	const T&	operator [](int i)			const
	{
		return el[i];
	}

	PX_INLINE	T&			operator [](unsigned i)
	{
		return el[i];
	}
	PX_INLINE	const T&	operator [](unsigned i)			const
	{
		return el[i];
	}

	PX_INLINE	Vec			operator -	()					const
	{
		Vec r;
		ALL_i(D, r[i] = -el[i]);
		return r;
	}

	PX_INLINE	Vec			operator +	(const Vec& v)	const
	{
		Vec r;
		ALL_i(D, r[i] = el[i] + v[i]);
		return r;
	}
	PX_INLINE	Vec			operator -	(const Vec& v)	const
	{
		Vec r;
		ALL_i(D, r[i] = el[i] - v[i]);
		return r;
	}
	PX_INLINE	Vec			operator *	(const Vec& v)	const
	{
		Vec r;
		ALL_i(D, r[i] = el[i] * v[i]);
		return r;
	}
	PX_INLINE	Vec			operator /	(const Vec& v)	const
	{
		Vec r;
		ALL_i(D, r[i] = el[i] / v[i]);
		return r;
	}

	PX_INLINE	Vec			operator *	(T v)				const
	{
		Vec r;
		ALL_i(D, r[i] = el[i] * v);
		return r;
	}
	PX_INLINE	Vec			operator /	(T v)				const
	{
		return *this * ((T)1 / v);
	}

	PX_INLINE	Vec&		operator +=	(const Vec& v)
	{
		ALL_i(D, el[i] += v[i]);
		return *this;
	}
	PX_INLINE	Vec&		operator -=	(const Vec& v)
	{
		ALL_i(D, el[i] -= v[i]);
		return *this;
	}
	PX_INLINE	Vec&		operator *=	(const Vec& v)
	{
		ALL_i(D, el[i] *= v[i]);
		return *this;
	}
	PX_INLINE	Vec&		operator /=	(const Vec& v)
	{
		ALL_i(D, el[i] /= v[i]);
		return *this;
	}

	PX_INLINE	T			operator |	(const Vec& v)	const
	{
		T r = (T)0;
		ALL_i(D, r += el[i] * v[i]);
		return r;
	}

	PX_INLINE	T			normalize();

	PX_INLINE	T			lengthSquared()					const
	{
		return *this | *this;
	}

protected:
	T el[D];
};

template<typename T, int D>
PX_INLINE T
Vec<T, D>::normalize()
{
	const T l2 = *this | *this;
	if (l2 == (T)0)
	{
		return (T)0;
	}
	const T recipL = (T)1 / physx::PxSqrt(l2);
	*this *= recipL;
	return recipL * l2;
}

template<typename T, int D>
PX_INLINE Vec<T, D>
operator * (T s, const Vec<T, D>& v)
{
	Vec<T, D> r;
	ALL_i(D, r[i] = s * v[i]);
	return r;
}


/* Popular real vectors */

class Vec2Real : public Vec<Real, 2>
{
public:
	PX_INLINE			Vec2Real()	 : Vec<Real, 2>()
	{
	}
	PX_INLINE			Vec2Real(const Vec2Real& v) : Vec<Real, 2>()
	{
		ALL_i(2, el[i] = v[i]);
	}
	PX_INLINE			Vec2Real(const Vec<Real, 2>& v) : Vec<Real, 2>()
	{
		ALL_i(2, el[i] = v[i]);
	}
	PX_INLINE			Vec2Real(Real x, Real y) : Vec<Real, 2>()
	{
		set(x, y);
	}
	PX_INLINE Vec2Real&	operator = (const Vec2Real& v)
	{
		ALL_i(2, el[i] = v[i]);
		return *this;
	}

	PX_INLINE void		set(const Real* v)
	{
		ALL_i(2, el[i] = v[i]);
	}
	PX_INLINE void		set(Real x, Real y)
	{
		el[0] = x;
		el[1] = y;
	}

	PX_INLINE Real		operator ^(const Vec2Real& v)	const
	{
		return el[0] * v.el[1] - el[1] * v.el[0];
	}

	PX_INLINE Vec2Real	perp() const
	{
		Vec2Real result;
		result.el[0] = el[1];
		result.el[1] = -el[0];
		return result;
	}
};

class Vec4Real : public Vec<Real, 4>
{
public:
	PX_INLINE			Vec4Real()	 : Vec<Real, 4>()						
	{
	}
	PX_INLINE			Vec4Real(const Vec4Real& v) : Vec<Real, 4>()
	{
		ALL_i(4, el[i] = v[i]);
	}
	PX_INLINE			Vec4Real(const Vec<Real, 4>& v) : Vec<Real, 4>()
	{
		ALL_i(4, el[i] = v[i]);
	}
	PX_INLINE			Vec4Real(Real x, Real y, Real z, Real w) : Vec<Real, 4>()
	{
		set(x, y, z, w);
	}
	PX_INLINE Vec4Real&	operator = (const Vec4Real& v)
	{
		ALL_i(4, el[i] = v[i]);
		return *this;
	}

	PX_INLINE void		set(const Real* v)
	{
		ALL_i(4, el[i] = v[i]);
	}
	PX_INLINE void		set(Real x, Real y, Real z, Real w)
	{
		el[0] = x;
		el[1] = y;
		el[2] = z;
		el[3] = w;
	}
};


/* Position */

class Pos : public Vec4Real
{
public:

	PX_INLINE		Pos() : Vec4Real()
	{
		el[3] = 1;
	}
	PX_INLINE		Pos(Real x, Real y, Real z) : Vec4Real()
	{
		set(x, y, z);
	}
	PX_INLINE		Pos(Real c) : Vec4Real()
	{
		set(c, c, c);
	}
	PX_INLINE       Pos(physx::PxVec3 p) : Vec4Real()
	{
		set(p.x, p.y, p.z);
	}
	PX_INLINE		Pos(const Vec<Real, 4>& v) : Vec4Real()
	{
		set(v[0], v[1], v[2]);
	}
	PX_INLINE		Pos(const float* v) : Vec4Real()
	{
		set((Real)v[0], (Real)v[1], (Real)v[2]);
	}
	PX_INLINE		Pos(const double* v) : Vec4Real()
	{
		set((Real)v[0], (Real)v[1], (Real)v[2]);
	}
	PX_INLINE		Pos(const Pos& p) : Vec4Real()
	{
		set(p[0], p[1], p[2]);
	}
	PX_INLINE Pos&	operator = (const Pos& p)
	{
		set(p[0], p[1], p[2]);
		return *this;
	}

	PX_INLINE void	set(Real x, Real y, Real z)
	{
		Vec4Real::set(x, y, z, (Real)1);
	}
};


/* Direction */

class Dir : public Vec4Real
{
public:

	PX_INLINE		Dir() : Vec4Real()
	{
		el[3] = 0;
	}
	PX_INLINE		Dir(Real x, Real y, Real z) : Vec4Real()
	{
		set(x, y, z);
	}
	PX_INLINE		Dir(Real c) : Vec4Real()
	{
		set(c, c, c);
	}
	PX_INLINE       Dir(physx::PxVec3 p) : Vec4Real()
	{
		set(p.x, p.y, p.z);
	}
	PX_INLINE		Dir(const Vec<Real, 4>& v) : Vec4Real()
	{
		set(v[0], v[1], v[2]);
	}
	PX_INLINE		Dir(const float* v) : Vec4Real()
	{
		set((Real)v[0], (Real)v[1], (Real)v[2]);
	}
	PX_INLINE		Dir(const double* v) : Vec4Real()
	{
		set((Real)v[0], (Real)v[1], (Real)v[2]);
	}
	PX_INLINE		Dir(const Dir& d) : Vec4Real()
	{
		set(d[0], d[1], d[2]);
	}
	PX_INLINE Dir&	operator = (const Dir& d)
	{
		set(d[0], d[1], d[2]);
		return *this;
	}

	PX_INLINE void	set(Real x, Real y, Real z)
	{
		Vec4Real::set(x, y, z, (Real)0);
	}

	PX_INLINE Dir	cross(const Dir& d)	const	// Simple cross-product
	{
		return Dir(el[1] * d[2] - el[2] * d[1], el[2] * d[0] - el[0] * d[2], el[0] * d[1] - el[1] * d[0]);
	}

	PX_INLINE Real	dot(const Dir& d)	const	// Simple dot-product
	{
		return el[0]*d[0] + el[1]*d[1] + el[2]*d[2];
	}

	PX_INLINE Dir	operator ^(const Dir& d)	const	// Uses an improvement step for more accuracy
	{
		const Dir c = cross(d);	// Cross-product gives perpendicular
		const Real c2 = c|c;
		if (c2 != 0.0f) 
		{
			return c + ((dot(c))*(c.cross(d)) + (d|c)*(cross(c)))/c2;
		}
		return c;	// Improvement to (*this d)^T(c) = (0)
	}
};


/* Plane */

class Plane : public Vec4Real
{
public:

	PX_INLINE			Plane()	: Vec4Real()							{}
	PX_INLINE			Plane(const Dir& n, Real d)	: Vec4Real()
	{
		set(n, d);
	}
	PX_INLINE			Plane(const Dir& n, const Pos& p)	: Vec4Real()
	{
		set(n, p);
	}
	PX_INLINE			Plane(const Vec<Real, 4>& v)	: Vec4Real()
	{
		Vec4Real::set(v[0], v[1], v[2], v[3]);
	}
	PX_INLINE			Plane(const Plane& p)	: Vec4Real()
	{
		ALL_i(4, el[i] = p[i]);
	}
	PX_INLINE	Plane&	operator = (const Plane& p)
	{
		ALL_i(4, el[i] = p[i]);
		return *this;
	}

	PX_INLINE	void	set(const Dir& n, Real d)
	{
		ALL_i(3, el[i] = n[i]);
		el[3] = d;
	}
	PX_INLINE	void	set(const Dir& n, const Pos& p)
	{
		ALL_i(3, el[i] = n[i]);
		el[3] = -(n | p);
	}

	PX_INLINE	Dir		normal()					const
	{
		return Dir(el[0], el[1], el[2]);
	}
	PX_INLINE	Real	d()							const
	{
		return el[3];
	}
	PX_INLINE	Real	distance(const Pos& p)	const
	{
		return p | *this;
	}
	PX_INLINE	Pos		project(const Pos& p)		const
	{
		return p - normal() * distance(p);
	}

	PX_INLINE	Real	normalize();
};

PX_INLINE Real
Plane::normalize()
{
	const Real oldD = el[3];
	el[3] = 0;
	const Real l2 = *this | *this;
	if (l2 == 0)
	{
		return 0;
	}
	Real recipL = 1 / physx::PxSqrt(l2);
	recipL *= (Real)1.5 - (Real)0.5*l2*recipL*recipL;
	recipL *= (Real)1.5 - (Real)0.5*l2*recipL*recipL;
	el[3] = oldD;
	*this *= recipL;
	return recipL * l2;
}


/* Matrix */

class Mat4Real : public Vec<Vec4Real, 4>
{
public:

	PX_INLINE				Mat4Real()									{}
	PX_INLINE	 			Mat4Real(const Real v)
	{
		set(v);
	}
	PX_INLINE	 			Mat4Real(const Real* v)
	{
		set(v);
	}

	PX_INLINE	void		set(const Real v)
	{
		el[0].set(v, 0, 0, 0);
		el[1].set(0, v, 0, 0);
		el[2].set(0, 0, v, 0);
		el[3].set(0, 0, 0, v);
	}
	PX_INLINE	void		set(const Real* v)
	{
		ALL_i(4, el[i].set(v + 4 * i));
	}
	PX_INLINE	void		setCol(int colN, const Vec4Real& col)
	{
		ALL_i(4, el[i][colN] = col[i]);
	}

	PX_INLINE	Vec4Real	operator *	(const Vec4Real& v)	const
	{
		Vec4Real r;
		ALL_i(4, r[i] = el[i] | v);
		return r;
	}
	PX_INLINE	Mat4Real	operator +	(const Mat4Real& m)	const
	{
		Mat4Real r;
		for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
			{
				r[i][j] = el[i][j] + m[i][j];
			}
		return r;
	}
	PX_INLINE	Mat4Real	operator -	(const Mat4Real& m)	const
	{
		Mat4Real r;
		for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
			{
				r[i][j] = el[i][j] - m[i][j];
			}
		return r;
	}
	PX_INLINE	Mat4Real	operator *	(const Mat4Real& m)	const
	{
		Mat4Real r((Real)0);
		for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k)
				{
					r[i][j] += el[i][k] * m[k][j];
				}
		return r;
	}
	PX_INLINE	Mat4Real	operator *	(Real s)				const
	{
		Mat4Real r;
		ALL_i(4, r[i] = el[i] * s);
		return r;
	}
	PX_INLINE	Mat4Real	operator /	(Real s)				const
	{
		return *this * ((Real)1 / s);
	}

	PX_INLINE	Mat4Real&	operator *=	(Real s)
	{
		ALL_i(4, el[i] *= s);
		return *this;
	}
	PX_INLINE	Mat4Real&	operator /=	(Real s)
	{
		*this *= ((Real)1 / s);
		return *this;
	}

	PX_INLINE	Vec4Real	getCol(int colN)					const
	{
		Vec4Real col;
		ALL_i(4, col[i] = el[i][colN]);
		return col;
	}
	PX_INLINE	Real		det3()								const
	{
		return el[0] | (Dir(el[1]) ^ Dir(el[2]));    // Determinant of upper-left 3x3 block (same as full determinant if last row = (0,0,0,1))
	}
	PX_INLINE	Mat4Real	cof34()								const;	// Assumes last row = (0,0,0,1)
	PX_INLINE	Mat4Real	inverse34()							const;	// Assumes last row = (0,0,0,1)
	PX_INLINE	Mat4Real	transpose()							const
	{
		Mat4Real r;
		for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
			{
				r[i][j] = el[j][i];
			}
		return r;
	}
};

PX_INLINE Mat4Real
Mat4Real::cof34() const
{
	Mat4Real r;
	r[0].set(el[1][1]*el[2][2] - el[1][2]*el[2][1], el[1][2]*el[2][0] - el[1][0]*el[2][2], el[1][0]*el[2][1] - el[1][1]*el[2][0], 0);
	r[1].set(el[2][1]*el[0][2] - el[2][2]*el[0][1], el[2][2]*el[0][0] - el[2][0]*el[0][2], el[2][0]*el[0][1] - el[2][1]*el[0][0], 0);
	r[2].set(el[0][1]*el[1][2] - el[0][2]*el[1][1], el[0][2]*el[1][0] - el[0][0]*el[1][2], el[0][0]*el[1][1] - el[0][1]*el[1][0], 0);
	r[3] = -el[0][3] * r[0] - el[1][3] * r[1] - el[2][3] * r[2];
	r[3][3] = r[0][0] * el[0][0] + r[0][1] * el[0][1] + r[0][2] * el[0][2];
	return r;
}

PX_INLINE Mat4Real
Mat4Real::inverse34() const
{
	const Mat4Real cof = cof34();
	Mat4Real inv;
	const Real recipDet = physx::PxAbs(cof[3][3]) > EPS_REAL * EPS_REAL * EPS_REAL ? 1 / cof[3][3] : (Real)0;
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			inv[i][j] = cof[j][i] * recipDet;
		}
	}
	inv[3].set(0, 0, 0, 1);
	return inv;
}

PX_INLINE Mat4Real
operator * (Real s, const Mat4Real& m)
{
	Mat4Real r;
	ALL_i(4, r[i] = s * m[i]);
	return r;
}

}	// namespace ApexCSG

#endif // #define !WITHOUT_APEX_AUTHORING

#endif // #define APEX_CSG_MATH_H
