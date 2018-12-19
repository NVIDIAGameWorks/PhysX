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


#ifndef APEX_CSG_FAST_MATH_2_H
#define APEX_CSG_FAST_MATH_2_H

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

#define ALL_VV_i( _D, a,op,b) ALL_i( _D, a[i] op b[i] )
#define ALL_SV_i( _D, a,op,b) ALL_i( _D, a    op b[i] )
#define ALL_VS_i( _D, a,op,b) ALL_i( _D, a[i] op b    )
#define ALL_VVV_i( _D, a,op1,b,op2,c) ALL_i( _D, a[i] op1 b[i] op2 c[i] )
#define ALL_SVV_i( _D, a,op1,b,op2,c) ALL_i( _D, a    op1 b[i] op2 c[i] )
#define ALL_VVS_i( _D, a,op1,b,op2,c) ALL_i( _D, a[i] op1 b[i] op2 c    )

#endif

#if 1 

#define DEFINE_VEC(_D)                                                                                         \
    public:                                                                                                    \
	typedef Vec<T,_D> VecD;                                                                                    \
	PX_INLINE VecD() {}                                                                                        \
	PX_INLINE T&       operator [](int i)       { return el[i]; }                                              \
	PX_INLINE const T& operator [](int i) const { return el[i]; }                                              \
	PX_INLINE VecD(const VecD& v) { set(v); }                                                                  \
	PX_INLINE VecD(const T&    v) { set(v); }                                                                  \
	PX_INLINE VecD(const T*    v) { set(v); }                                                                  \
	PX_INLINE VecD& operator=(const VecD& v) { ALL_VV_i(_D, el, =, v); return *this; }                         \
	PX_INLINE VecD operator/(T v) const { return *this * ((T)1 / v); }                                         \
	PX_INLINE VecD operator+(const VecD& v) const { VecD r; ALL_VVV_i(_D, r, =, el, +, v); return r; }         \
	PX_INLINE VecD operator-(const VecD& v) const { VecD r; ALL_VVV_i(_D, r, =, el, -, v); return r; }         \
	PX_INLINE VecD operator*(const VecD& v) const {	VecD r; ALL_VVV_i(_D, r, =, el, *, v); return r; }         \
	PX_INLINE VecD operator-(             ) const { VecD r; ALL_VV_i( _D, r, =, -el);      return r;}          \
	PX_INLINE VecD& operator+=(const VecD& v) {ALL_VV_i(_D, el, +=, v); return *this; }                        \
	PX_INLINE VecD& operator-=(const VecD& v) {ALL_VV_i(_D, el, -=, v); return *this; }                        \
	PX_INLINE VecD& operator*=(const VecD& v) {ALL_VV_i(_D, el, *=, v); return *this; }                        \
	PX_INLINE VecD& operator/=(const VecD& v) {ALL_VV_i(_D, el, /=, v); return *this; }                        \
	PX_INLINE void set(const VecD& v) { ALL_VV_i(_D, el, =, v); }                                              \
	PX_INLINE void set(const T* v)    { ALL_VV_i(_D, el, =, v); }                                              \
	PX_INLINE void set(const T& v)    { ALL_VS_i(_D, el, =, v); }                                              \
	PX_INLINE T lengthSquared() const { return *this | *this; }                                                \
	PX_INLINE T normalize() { const T l2 = *this | *this; if (l2 == (T)0) { return (T)0; } const T recipL = (T)1 / physx::PxSqrt(l2); *this *= recipL; return recipL * l2; } \
	protected:T el[_D];                                                                                        \

    //PX_FORCE_INLINE T operator|(const VecD& v) const { T r = (T)0; ALL_SVV_i(_D, r, +=, el, *, v); return r; }
#endif

/* General vector */

__declspec(align(APEX_CSG_ALIGN)) class aligned { };

template<typename T, int D>
class Vec : public aligned
{

};


template<typename T, int D>
PX_INLINE Vec<T, D>
operator * (T s, const Vec<T, D>& v)
{
	Vec<T, D> r;
	ALL_i(D, r[i] = s * v[i]);
	//ALL_VVS_i(D, r, =, v, *, s);
	return r;
}

template<typename T>
class Vec<T, 2> : public aligned {
	DEFINE_VEC(2);
public:
	typedef Vec<T,2> Vec2;

	PX_INLINE			Vec2(T x, T y)
	{
		set(x, y);
	}

	PX_INLINE void		set(T x, T y)
	{
		el[0] = x;
		el[1] = y;
	}

	PX_INLINE Real		operator ^ (const Vec2& v)	const
	{
		return el[0] * v.el[1] - el[1] * v.el[0];
	}

	PX_FORCE_INLINE	T	operator | (const Vec2& v)	const
	{
		T r = (T)0;
		ALL_SVV_i(2, r, +=, el, *, v);
		return r;
	}

};

template<typename T>
class Vec<T, 3> : public aligned {
	DEFINE_VEC(3);
public:
	typedef Vec<T,3> Vec3;

	PX_INLINE			Vec3(T x, T y, T z)
	{
		set(x, y, z);
	}

	PX_INLINE void		set(T x, T y, T z)
	{
		el[0] = x;
		el[1] = y;
		el[2] = z;
	}

	PX_FORCE_INLINE	T		operator |	(const Vec3& v)	const
	{
		T r = (T)0;
		ALL_SVV_i(3, r, +=, el, *, v);
		return r;
	}
};

template<typename T>
class Vec<T, 4> : public aligned {
	DEFINE_VEC(4);
public:
	typedef Vec<T,4> Vec4;

	PX_INLINE			Vec4(T x, T y, T z, T w)
	{
		set(x, y, z, w);
	}

	PX_INLINE void		set(T x, T y, T z, T w)
	{
		el[0] = x;
		el[1] = y;
		el[2] = z;
		el[3] = w;
	}

#if APEX_CSG_SSE

#if APEX_CSG_DBL
	PX_INLINE       __m128d& xy()         { return *reinterpret_cast<      __m128d*>(&el[0]); }
	PX_INLINE const __m128d& xy()   const { return *reinterpret_cast<const __m128d*>(&el[0]); }
	PX_INLINE       __m128d& zw()         { return *reinterpret_cast<      __m128d*>(&el[2]); }
	PX_INLINE const __m128d& zw()   const { return *reinterpret_cast<const __m128d*>(&el[2]); }
#else
	PX_INLINE       __m128& xyzw()       { return *reinterpret_cast<      __m128*>(&el[0]); }
	PX_INLINE const __m128& xyzw() const { return *reinterpret_cast<const __m128*>(&el[0]); }
#endif /* #if APEX_CSG_DBL */

	friend Real dot(const Vec4&, const Vec4&);
	PX_FORCE_INLINE Real operator | (const Vec4& v) const
	{
		return dot(*this, v);
	}

#endif /* #if APEX_CSG_SSE */
};

/* Popular real vectors */

typedef Vec<Real, 2> Vec2Real;
typedef Vec<Real, 3> Vec3Real;
typedef Vec<Real, 4> Vec4Real;

#if APEX_CSG_SSE 

#if APEX_CSG_DBL

PX_FORCE_INLINE Real dot(const Vec4Real& a, const Vec4Real& b)
{
	/*
	__m128d mr = _mm_add_sd ( _mm_mul_pd ( a.xy(), b.xy()),
	                          _mm_mul_sd ( a.zw(), b.zw()) ) ;
	mr =         _mm_add_sd ( _mm_unpackhi_pd ( mr , mr ), mr );
	double r;
	_mm_store_sd(&r, mr);
	return r;*/
	__declspec(align(16)) double r[2] = { 0., 0. };
	__m128d mresult;
	mresult = _mm_add_pd(_mm_mul_pd( a.xy(), b.xy() ),
	                     _mm_mul_pd( a.zw(), b.zw() ) );
	_mm_store_pd(r, mresult);
	return r[0] + r[1];
}

#else

PX_FORCE_INLINE Real dot(const Vec4Real& a, const Vec4Real& b)
{
	float r;
	_mm_store_ps(&s, _mm_dot_pos(a.xyzw(), b.xyzw()));
	return r;
}

#endif /* #if APEX_CSG_DBL */

#else

PX_FORCE_INLINE Real dot(const Vec4Real& a, const Vec4Real& b) 
{
	Real r = 0;
	ALL_SVV_i(sizeof(a)/sizeof(Real), r, +=, a, *, b);
	return r;
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
		//ALL_i(4, el[i] = p[i]);
		ALL_VV_i(4, el, =, p);
	}
	PX_INLINE	Plane&	operator = (const Plane& p)
	{
		ALL_VV_i(4, el, =, p);
		//ALL_i(4, el[i] = p[i]);
		return *this;
	}

	PX_INLINE	void	set(const Dir& n, Real d)
	{
		//ALL_i(3, el[i] = n[i]);
		ALL_VV_i(3, el, =, n);
		el[3] = d;
	}
	PX_INLINE	void	set(const Dir& n, const Pos& p)
	{
		//ALL_i(3, el[i] = n[i]);
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
		//ALL_i(4, r[i] = el[i] | v);
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
		//ALL_i(4, r[i] = el[i] * s);
		ALL_VVS_i(4, r, =, el, *, s);
		return r;
	}
	PX_INLINE	Mat4Real	operator /	(Real s)				const
	{
		return *this * ((Real)1 / s);
	}

	PX_INLINE	Mat4Real&	operator *=	(Real s)
	{
		//ALL_i(4, el[i] *= s);
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
	//ALL_i(4, r[i] = s * m[i]);
	ALL_VVS_i(4, r, =, m, *, s);
	return r;
}

}	// namespace ApexCSG

#endif // #define APEX_CSG_FAST_MATH_2_H
