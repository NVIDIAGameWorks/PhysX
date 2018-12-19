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



#ifndef NOISE_UTILS_H
#define NOISE_UTILS_H

#include "authoring/ApexCSGMath.h"

#ifndef WITHOUT_APEX_AUTHORING

namespace ApexCSG 
{

template<typename T>
PX_INLINE T fade(T t) { return t * t * t * (t * (t  * (T)6.0 - (T)15.0) + (T)10.0); }

template<typename T>
PX_INLINE T lerp(T t, T a, T b) { return a + t * (b - a); }

template<typename T, class VecT, int SampleSize>
PX_INLINE void setup(int i, VecT& point, T& t, int& b0, int& b1, T& r0, T& r1)
{
	t  = point[i] + (0x1000);
	b0 = ((int)t) & (SampleSize-1);
	b1 = (b0+1)   & (SampleSize-1);
	r0 = t - (int)t;
	r1 = r0 - 1.0f;
}

template<typename T, class VecT>
PX_INLINE T at2(const T& rx, const T& ry, const VecT& q)
{
	return rx * q[0] + ry * q[1];
}

template<typename T, class VecT>
PX_INLINE T at3(const T& rx, const T& ry, const T& rz, const VecT& q)
{
	return rx * q[0] + ry * q[1] + rz * q[2];
}

///////////////////////////////////////////////////////////////////////////

template<typename T, int SampleSize>
T noiseSample(ApexCSG::Vec<T, 1> point, int* p, ApexCSG::Vec<T,1>* g)
{
	int bx0, bx1;
	T rx0, rx1, sx, t, u, v;

	setup<T,Vec<T,1>,SampleSize>(0, point,t, bx0,bx1, rx0,rx1);

	sx = fade(rx0);

	u = rx0 * g[ p[ bx0 ] ];
	v = rx1 * g[ p[ bx1 ] ];

	return lerp(sx, u, v);
}

template<typename T, int SampleSize>
T noiseSample(Vec<T,2> point, int* p, Vec<T,2>* g)
{
	int bx0, bx1, by0, by1, b00, b10, b01, b11;
	T rx0, rx1, ry0, ry1, sx, sy, a, b, t, u, v;
	Vec<T,2> q;
	int i, j;

	setup<T,Vec<T,2>,SampleSize>(0, point,t, bx0,bx1, rx0,rx1);
	setup<T,Vec<T,2>,SampleSize>(1, point,t, by0,by1, ry0,ry1);

	i = p[bx0];
	j = p[bx1];

	b00 = p[i + by0];
	b10 = p[j + by0];
	b01 = p[i + by1];
	b11 = p[j + by1];

	sx = fade(rx0);
	sy = fade(ry0);

	q = g[b00];
	u = at2(rx0,ry0,q);
	q = g[b10];
	v = at2(rx1,ry0,q);
	a = lerp(sx, u, v);

	q = g[b01];
	u = at2(rx0,ry1,q);
	q = g[b11];
	v = at2(rx1,ry1,q);
	b = lerp(sx, u, v);

	return lerp(sy, a, b);
}

template<typename T, int SampleSize>
T noiseSample(Vec<T,3> point, int* p, Vec<T,3>* g)
{
	int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
	T rx0, rx1, ry0, ry1, rz0, rz1,  sy, sz, a, b, c, d, t, u, v;
	Vec<T,3> q;
	int i, j;

	setup<T,Vec<T,3>,SampleSize>(0, point,t, bx0,bx1, rx0,rx1);
	setup<T,Vec<T,3>,SampleSize>(1, point,t, by0,by1, ry0,ry1);
	setup<T,Vec<T,3>,SampleSize>(2, point,t, bz0,bz1, rz0,rz1);

	i = p[ bx0 ];
	j = p[ bx1 ];

	b00 = p[ i + by0 ];
	b10 = p[ j + by0 ];
	b01 = p[ i + by1 ];
	b11 = p[ j + by1 ];

	t  = fade(rx0);
	sy = fade(ry0);
	sz = fade(rz0);

	q = g[ b00 + bz0 ] ; u = at3(rx0,ry0,rz0,q);
	q = g[ b10 + bz0 ] ; v = at3(rx1,ry0,rz0,q);
	a = lerp(t, u, v);

	q = g[ b01 + bz0 ] ; u = at3(rx0,ry1,rz0,q);
	q = g[ b11 + bz0 ] ; v = at3(rx1,ry1,rz0,q);
	b = lerp(t, u, v);

	c = lerp(sy, a, b);

	q = g[ b00 + bz1 ] ; u = at3(rx0,ry0,rz1,q);
	q = g[ b10 + bz1 ] ; v = at3(rx1,ry0,rz1,q);
	a = lerp(t, u, v);

	q = g[ b01 + bz1 ] ; u = at3(rx0,ry1,rz1,q);
	q = g[ b11 + bz1 ] ; v = at3(rx1,ry1,rz1,q);
	b = lerp(t, u, v);

	d = lerp(sy, a, b);

	return lerp(sz, c, d);
}

}

#endif /* #ifndef WITHOUT_APEX_AUTHORING */

#endif /* #ifndef NOISE_UTILS_H */