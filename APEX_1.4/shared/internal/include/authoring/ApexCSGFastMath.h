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


#ifndef APEX_CSG_FAST_MATH_H
#define APEX_CSG_FAST_MATH_H

#include "ApexUsingNamespace.h"
#include "PxMath.h"
#include "PxVec3.h"

#include "PsUtilities.h"

#include "PxIntrinsics.h"
#include <emmintrin.h>
#include <fvec.h>

#include <math.h>
#include <float.h>

#ifdef __SSE2__
#define APEX_CSG_SSE
#include <mmintrin.h>
#include <emmintrin.h>
#endif
#ifdef __SSE3__
#include <pmmintrin.h>
#endif
#ifdef __SSE4_1__
#include <smmintrin.h>
#endif

namespace ApexCSG
{

/* Utilities */

template<typename T>
T square(T t)
{
	return t * t;
}


/* Linear algebra */

#define ALL_i( _D, _exp )	for( int i = 0; i < _D; ++i ) { _exp; }

#ifndef APEX_CSG_LOOP_UNROLL
#define APEX_CSG_LOOP_UNROLL 1
#endif

#ifndef APEX_CSG_SSE
#define APEX_CSG_SSE 1
#endif

#ifndef APEX_CSG_INLINE
#define APEX_CSG_INLINE 1
#endif

#ifndef APEX_CSG_ALIGN
#define APEX_CSG_ALIGN 16
#endif

#if APEX_CSG_LOOP_UNROLL

#define VEC_SIZE() sizeof(*this)/sizeof(Real)

#define OP_VV(a,op,b,i)         a[i] op  b[i]
#define OP_SV(a,op,b,i)         a    op  b[i]
#define OP_VS(a,op,b,i)         a[i] op  b
#define OP_VVV(a,op1,b,op2,c,i) a[i] op1 b[i] op2 c[i]
#define OP_SVV(a,op1,b,op2,c,i) a    op1 b[i] op2 c[i]
#define OP_VVS(a,op1,b,op2,c,i) a[i] op1 b[i] op2 c
#define OP_D(_D) (_D == 0 ? 0 : (_D == 1 ? 1 : (_D == 2 ? 2 : (_D == 3 ? 3 : 3))))
#define OP_NAME(_T)   PX_CONCAT(OP_,_T)
#define OP_2_NAME(_D) ALL_2_##_D /*PX_CONCAT(ALL_2_,OP_D(_D))*/
#define OP_3_NAME(_D) ALL_3_##_D /*PX_CONCAT(ALL_3_,OP_D(_D))*/

#define ALL_2_1(  _T, a,op,b)                     OP_##_T (a,op,b,0)
#define ALL_2_2(  _T, a,op,b) ALL_2_1(_T,a,op,b); OP_##_T (a,op,b,1)
#define ALL_2_3(  _T, a,op,b) ALL_2_2(_T,a,op,b); OP_##_T (a,op,b,2)
#define ALL_2_4(  _T, a,op,b) ALL_2_3(_T,a,op,b); OP_##_T (a,op,b,3)
#define ALL_VV_i( _D, a,op,b) OP_2_NAME(_D)(VV,a,op,b)
#define ALL_SV_i( _D, a,op,b) OP_2_NAME(_D)(SV,a,op,b)
#define ALL_VS_i( _D, a,op,b) OP_2_NAME(_D)(VS,a,op,b)

#define ALL_3_1(   _T, a,op1,b,op2,c)                            OP_NAME(_T) (a,op1,b,op2,c,0)
#define ALL_3_2(   _T, a,op1,b,op2,c) ALL_3_1(_T,a,op1,b,op2,c); OP_NAME(_T) (a,op1,b,op2,c,1)
#define ALL_3_3(   _T, a,op1,b,op2,c) ALL_3_2(_T,a,op1,b,op2,c); OP_NAME(_T) (a,op1,b,op2,c,2)
#define ALL_3_4(   _T, a,op1,b,op2,c) ALL_3_3(_T,a,op1,b,op2,c); OP_NAME(_T) (a,op1,b,op2,c,3)
#define ALL_VVV_i( _D, a,op1,b,op2,c) OP_3_NAME(_D)(VVV,a,op1,b,op2,c)
#define ALL_SVV_i( _D, a,op1,b,op2,c) OP_3_NAME(_D)(SVV,a,op1,b,op2,c)
#define ALL_VVS_i( _D, a,op1,b,op2,c) OP_3_NAME(_D)(VVS,a,op1,b,op2,c)
#define ALL_VVV(       a,op1,b,op2,c) OP_3_NAME(VEC_SIZE())(VVV,a,op1,b,op2,c)
#define ALL_SVV(       a,op1,b,op2,c) OP_3_NAME(VEC_SIZE())(SVV,a,op1,b,op2,c)
#define ALL_VVS(       a,op1,b,op2,c) OP_3_NAME(VEC_SIZE())(VVS,a,op1,b,op2,c)

#else

#define ALL_VV_i( _D, a,op,b)         ALL_i( _D, a[i] op b[i] )
#define ALL_SV_i( _D, a,op,b)         ALL_i( _D, a    op b[i] )
#define ALL_VS_i( _D, a,op,b)         ALL_i( _D, a[i] op b    )
#define ALL_VVV_i( _D, a,op1,b,op2,c) ALL_i( _D, a[i] op1 b[i] op2 c[i] )
#define ALL_SVV_i( _D, a,op1,b,op2,c) ALL_i( _D, a    op1 b[i] op2 c[i] )
#define ALL_VVS_i( _D, a,op1,b,op2,c) ALL_i( _D, a[i] op1 b[i] op2 c    )

#endif

/* General vector */

__declspec(align(APEX_CSG_ALIGN)) class aligned { };

template<typename T, int D>
class Vec : public aligned
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

	PX_FORCE_INLINE	T			operator |	(const Vec& v)	const
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
	//ALL_VVS_i(D, r, =, v, *, s);
	return r;
}


/* Popular real vectors */
template<typename T>
class Vec2 : public Vec<T, 2>
{
public:
	PX_INLINE			Vec2()									{}
	PX_INLINE			Vec2(const Vec2& v)
	{
		ALL_VV_i(2, el, =, v);
	}
	PX_INLINE			Vec2(const Vec<T, 2>& v)
	{
		ALL_VV_i(2, el, =, v);
	}
	PX_INLINE			Vec2(T x, T y)
	{
		set(x, y);
	}
	PX_INLINE Vec2&	operator = (const Vec2& v)
	{
		ALL_VV_i(2, el, =, v);
		return *this;
	}

	PX_INLINE void		set(const T* v)
	{
		ALL_VV_i(2, el, =, v);
	}
	PX_INLINE void		set(T x, T y)
	{
		el[0] = x;
		el[1] = y;
	}

	PX_INLINE T			operator ^(const Vec2& v)	const
	{
		return el[0] * v.el[1] - el[1] * v.el[0];
	}
};
typedef Vec2<Real> Vec2Real;

template<typename T>
class Vec3 : public Vec<T, 3>
{
public:
	PX_INLINE			Vec3()									{}
	PX_INLINE			Vec3(const Vec3& v)
	{
		ALL_VV_i(3, el, =, v);
	}
	PX_INLINE			Vec3(const Vec<T, 3>& v)
	{
		ALL_VV_i(3, el, =, v);
	}
	PX_INLINE			Vec3(T x, T y, T z)
	{
		set(x, y, z);
	}
	PX_INLINE Vec3&	operator = (const Vec3& v)
	{
		ALL_VV_i(3, el, =, v);
		return *this;
	}

	PX_INLINE void		set(const T* v)
	{
		ALL_VV_i(3, el, =, v);
	}
	PX_INLINE void		set(T x, T y, T z)
	{
		el[0] = x;
		el[1] = y;
		el[2] = z;
	}
};
typedef Vec3<Real> Vec3Real;

template<typename T>
class Vec4 : public Vec<Real, 4>
{
public:
	PX_INLINE			Vec4()									{}
	PX_INLINE			Vec4(const Vec4& v)
	{
		ALL_VV_i(4, el, =, v);
	}
	PX_INLINE			Vec4(const Vec<T, 4>& v)
	{
		ALL_VV_i(4, el, =, v);
	}
	PX_INLINE			Vec4(T x, T y, T z, T w)
	{
		set(x, y, z, w);
	}
	PX_INLINE Vec4&		operator = (const Vec4& v)
	{
		ALL_VV_i(4, el, =, v);
		return *this;
	}

	PX_INLINE void		set(const T* v)
	{
		ALL_VV_i(4, el, =, v);
	}
	PX_INLINE void		set(T x, T y, T z, T w)
	{
		el[0] = x;
		el[1] = y;
		el[2] = z;
		el[3] = w;
	}

#if APEX_CSG_INLINE
	PX_INLINE	Vec			operator -	()					const
	{
		Vec4Real r;
		ALL_VV_i(4, r, =, -el);
		return r;
	}

	PX_INLINE	Vec4			operator +	(const Vec4& v)	const
	{
		Vec r;
		ALL_VVV_i(4, r, =, el, +, v);
		return r;
	}
	PX_INLINE	Vec4			operator -	(const Vec4& v)	const
	{
		Vec r;
		ALL_VVV_i(4, r, =, el, -, v);
		return r;
	}
	PX_INLINE	Vec4			operator *	(const Vec4& v)	const
	{
		Vec r;
		ALL_VVV_i(4, r, =, el, *, v);
		return r;
	}
	PX_INLINE	Vec4			operator /	(const Vec4& v)	const
	{
		Vec4 r;
		ALL_VVV_i(4, r, =, el, /, v);
		return r;
	}

	PX_INLINE	Vec4			operator *	(Real v)				const
	{
		Vec4 r;
		ALL_VVS_i(4, r, = , el, *, v);
		return r;
	}
	PX_INLINE	Vec4			operator /	(Real v)				const
	{
		return *this * (1. / v);
	}

	PX_INLINE	Vec4&			operator +=	(const Vec4& v)
	{
		ALL_VV_i(4, el, +=, v);
		return *this;
	}
	PX_INLINE	Vec4&			operator -=	(const Vec4& v)
	{
		ALL_VV_i(4, el, -=, v);
		return *this;
	}
	PX_INLINE	Vec4&			operator *=	(const Vec4& v)
	{
		ALL_VV_i(4, el, *=, v);
		return *this;
	}
	PX_INLINE	Vec4&			operator *=	(Real v)
	{
		ALL_VS_i(4, el, *=, v);
		return *this;
	}
	PX_INLINE	Vec4&			operator /=	(Real v)
	{
		Real vInv = 1. / v;
		return operator*=(vInv);
	}
	PX_INLINE	Vec4&			operator /=	(const Vec4& v)
	{
		ALL_VV_i(4, el, /=, v);
		return *this;
	}
#endif /* #if APEX_CGS_INLINE */

	template<typename U> friend U dot(const Vec4<U>&, const Vec4<U>&);
	PX_FORCE_INLINE Real operator | (const Vec4& v) const
	{
		return dot(*this, v);
	}
};
typedef Vec4<Real> Vec4Real;

template<typename T>
PX_FORCE_INLINE T dot(const Vec4<T>& a, const Vec4<T>& b) 
{
	Real r = 0;
	ALL_SVV_i(sizeof(a)/sizeof(T), r, +=, a, *, b);
	return r;
}

#if APEX_CSG_SSE 

template<typename T> PX_INLINE       __m128d&  xy(Vec4<T>& v)       { return *reinterpret_cast<      __m128d*>(&v[0]); }
template<typename T> PX_INLINE const __m128d&  xy(const Vec4<T>& v) { return *reinterpret_cast<const __m128d*>(&v[0]); }
template<typename T> PX_INLINE       __m128d&  zw(Vec4<T>& v)       { return *reinterpret_cast<      __m128d*>(&v[2]); }
template<typename T> PX_INLINE const __m128d&  zw(const Vec4<T>& v) { return *reinterpret_cast<const __m128d*>(&v[2]); }
template<typename T> PX_INLINE       __m128& xyzw(Vec4<T>& v)       { return *reinterpret_cast<      __m128*>(&v[0]); }
template<typename T> PX_INLINE const __m128& xyzw(const Vec4<T>& v) { return *reinterpret_cast<const __m128*>(&v[0]); }

template<>
PX_FORCE_INLINE double dot<double>(const Vec4<double>& a, const Vec4<double>& b)
{
	__declspec(align(16)) double r[2] = { 0., 0. };
	__m128d mresult;
	mresult = _mm_add_pd(_mm_mul_pd( xy(a), xy(b) ),
	                     _mm_mul_pd( zw(a), zw(b) ) );
	_mm_store_pd(r, mresult);
	return r[0] + r[1];
}

#endif /* #if APEX_CSG_SSE */

/* Position */

class Pos : public Vec4Real
{
public:

	PX_INLINE		Pos()
	{
		el[3] = 1;
	}
	PX_INLINE		Pos(Real x, Real y, Real z)
	{
		set(x, y, z);
	}
	PX_INLINE		Pos(Real c)
	{
		set(c, c, c);
	}
	PX_INLINE       Pos(physx::PxVec3 p)
	{
		set(p.x, p.y, p.z);
	}
	PX_INLINE		Pos(const Vec<Real, 4>& v)
	{
		set(v[0], v[1], v[2]);
	}
	PX_INLINE		Pos(const float* v)
	{
		set((Real)v[0], (Real)v[1], (Real)v[2]);
	}
	PX_INLINE		Pos(const double* v)
	{
		set((Real)v[0], (Real)v[1], (Real)v[2]);
	}
	PX_INLINE		Pos(const Pos& p)
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

	PX_INLINE		Dir()
	{
		el[3] = 0;
	}
	PX_INLINE		Dir(Real x, Real y, Real z)
	{
		set(x, y, z);
	}
	PX_INLINE		Dir(Real c)
	{
		set(c, c, c);
	}
	PX_INLINE       Dir(physx::PxVec3 p)
	{
		set(p.x, p.y, p.z);
	}
	PX_INLINE		Dir(const Vec<Real, 4>& v)
	{
		set(v[0], v[1], v[2]);
	}
	PX_INLINE		Dir(const float* v)
	{
		set((Real)v[0], (Real)v[1], (Real)v[2]);
	}
	PX_INLINE		Dir(const double* v)
	{
		set((Real)v[0], (Real)v[1], (Real)v[2]);
	}
	PX_INLINE		Dir(const Dir& d)
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

	PX_INLINE Dir	operator ^(const Dir& d)	const
	{
		return Dir(el[1] * d[2] - el[2] * d[1], el[2] * d[0] - el[0] * d[2], el[0] * d[1] - el[1] * d[0]);
	}

};


/* Plane */

class Plane : public Vec4Real
{
public:

	PX_INLINE			Plane()								{}
	PX_INLINE			Plane(const Dir& n, Real d)
	{
		set(n, d);
	}
	PX_INLINE			Plane(const Dir& n, const Pos& p)
	{
		set(n, p);
	}
	PX_INLINE			Plane(const Vec<Real, 4>& v)
	{
		Vec4Real::set(v[0], v[1], v[2], v[3]);
	}
	PX_INLINE			Plane(const Plane& p)
	{
		ALL_VV_i(4, el, =, p);
	}
	PX_INLINE	Plane&	operator = (const Plane& p)
	{
		ALL_VV_i(4, el, =, p);
		return *this;
	}

	PX_INLINE	void	set(const Dir& n, Real d)
	{
		ALL_VV_i(3, el, =, n);
		el[3] = d;
	}
	PX_INLINE	void	set(const Dir& n, const Pos& p)
	{
		ALL_VV_i(3, el, =, n);
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
	const Real recipL = 1. / physx::PxSqrt(l2);
	el[3] = oldD;
	*this *= recipL;
	return recipL * l2;
}


/* Matrix */

__declspec(align(16)) class Mat4Real : public Vec<Vec4Real, 4>
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
		ALL_VVS_i(4, r, =, el, |, v);
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
		ALL_VVS_i(4, r, =, el, *, s);
		return r;
	}
	PX_INLINE	Mat4Real	operator /	(Real s)				const
	{
		return *this * ((Real)1 / s);
	}

	PX_INLINE	Mat4Real&	operator *=	(Real s)
	{
		ALL_VS_i(4, el, *=, s);
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
	ALL_VVS_i(4, r, =, m, *, s);
	return r;
}

}	// namespace ApexCSG

#endif // #define APEX_CSG_FAST_MATH_H
