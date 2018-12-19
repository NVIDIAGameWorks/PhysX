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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#pragma once

NV_SIMD_NAMESPACE_BEGIN

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// factory implementation
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Simd4fZeroFactory::operator Simd4f() const
{
	return _mm_setzero_ps();
}

Simd4fOneFactory::operator Simd4f() const
{
	return _mm_set1_ps(1.0f);
}

Simd4fScalarFactory::operator Simd4f() const
{
	return _mm_set1_ps(value);
}

Simd4fTupleFactory::operator Simd4f() const
{
	return reinterpret_cast<const Simd4f&>(tuple);
}

Simd4fLoadFactory::operator Simd4f() const
{
	return _mm_loadu_ps(ptr);
}

Simd4fLoad3Factory::operator Simd4f() const
{
	/* [f0 f1 f2 f3] = [ptr[0] ptr[1] 0 0] */
	__m128i xy = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(ptr));
	__m128 z = _mm_load_ss(ptr + 2);
	return _mm_movelh_ps(_mm_castsi128_ps(xy), z);
}

Simd4fLoad3SetWFactory::operator Simd4f() const
{
	__m128 z = _mm_load_ss(ptr + 2);
	__m128 wTmp = _mm_load_ss(&w);

	__m128i xy = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(ptr));
	__m128 zw = _mm_movelh_ps(z, wTmp);

	return _mm_shuffle_ps(_mm_castsi128_ps(xy), zw, _MM_SHUFFLE(2, 0, 1, 0));
}

Simd4fAlignedLoadFactory::operator Simd4f() const
{
	return _mm_load_ps(ptr);
}

Simd4fOffsetLoadFactory::operator Simd4f() const
{
	return _mm_load_ps(reinterpret_cast<const float*>(reinterpret_cast<const char*>(ptr) + offset));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// expression template
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <>
inline ComplementExpr<Simd4f>::operator Simd4f() const
{
	return _mm_andnot_ps(v, _mm_castsi128_ps(_mm_set1_epi32(-1)));
}

template <>
inline Simd4f operator&(const ComplementExpr<Simd4f>& complement, const Simd4f& v)
{
	return _mm_andnot_ps(complement.v, v);
}

template <>
inline Simd4f operator&(const Simd4f& v, const ComplementExpr<Simd4f>& complement)
{
	return _mm_andnot_ps(complement.v, v);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// operator implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Simd4f operator==(const Simd4f& v0, const Simd4f& v1)
{
	return _mm_cmpeq_ps(v0, v1);
}

Simd4f operator<(const Simd4f& v0, const Simd4f& v1)
{
	return _mm_cmplt_ps(v0, v1);
}

Simd4f operator<=(const Simd4f& v0, const Simd4f& v1)
{
	return _mm_cmple_ps(v0, v1);
}

Simd4f operator>(const Simd4f& v0, const Simd4f& v1)
{
	return _mm_cmpgt_ps(v0, v1);
}

Simd4f operator>=(const Simd4f& v0, const Simd4f& v1)
{
	return _mm_cmpge_ps(v0, v1);
}

ComplementExpr<Simd4f> operator~(const Simd4f& v)
{
	return ComplementExpr<Simd4f>(v);
}

Simd4f operator&(const Simd4f& v0, const Simd4f& v1)
{
	return _mm_and_ps(v0, v1);
}

Simd4f operator|(const Simd4f& v0, const Simd4f& v1)
{
	return _mm_or_ps(v0, v1);
}

Simd4f operator^(const Simd4f& v0, const Simd4f& v1)
{
	return _mm_xor_ps(v0, v1);
}

Simd4f operator<<(const Simd4f& v, int shift)
{
	return _mm_castsi128_ps(_mm_slli_epi32(_mm_castps_si128(v), shift));
}

Simd4f operator>>(const Simd4f& v, int shift)
{
	return _mm_castsi128_ps(_mm_srli_epi32(_mm_castps_si128(v), shift));
}

Simd4f operator+(const Simd4f& v)
{
	return v;
}

Simd4f operator+(const Simd4f& v0, const Simd4f& v1)
{
	return _mm_add_ps(v0, v1);
}

Simd4f operator-(const Simd4f& v)
{
	return _mm_xor_ps(_mm_castsi128_ps(_mm_set1_epi32(0x80000000)), v);
}

Simd4f operator-(const Simd4f& v0, const Simd4f& v1)
{
	return _mm_sub_ps(v0, v1);
}

Simd4f operator*(const Simd4f& v0, const Simd4f& v1)
{
	return _mm_mul_ps(v0, v1);
}

Simd4f operator/(const Simd4f& v0, const Simd4f& v1)
{
	return _mm_div_ps(v0, v1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// function implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Simd4f simd4f(const Simd4i& v)
{
	return _mm_castsi128_ps(v);
}

Simd4f convert(const Simd4i& v)
{
	return _mm_cvtepi32_ps(v);
}

float (&array(Simd4f& v))[4]
{
	return reinterpret_cast<float(&)[4]>(v);
}

const float (&array(const Simd4f& v))[4]
{
	return reinterpret_cast<const float(&)[4]>(v);
}

void store(float* ptr, Simd4f const& v)
{
	_mm_storeu_ps(ptr, v);
}

void storeAligned(float* ptr, Simd4f const& v)
{
	_mm_store_ps(ptr, v);
}

void store3(float* dst, const Simd4f& v)
{
	const float* __restrict src = array(v);
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
}

void storeAligned(float* ptr, unsigned int offset, Simd4f const& v)
{
	_mm_store_ps(reinterpret_cast<float*>(reinterpret_cast<char*>(ptr) + offset), v);
}

template <size_t i>
Simd4f splat(Simd4f const& v)
{
	return _mm_shuffle_ps(v, v, _MM_SHUFFLE(i, i, i, i));
}

Simd4f select(Simd4f const& mask, Simd4f const& v0, Simd4f const& v1)
{
	return _mm_xor_ps(v1, _mm_and_ps(mask, _mm_xor_ps(v1, v0)));
}

Simd4f abs(const Simd4f& v)
{
	return _mm_andnot_ps(_mm_castsi128_ps(_mm_set1_epi32(0x80000000)), v);
}

Simd4f floor(const Simd4f& v)
{
	// SSE 4.1: return _mm_floor_ps(v);
	Simd4i i = _mm_cvttps_epi32(v);
	Simd4i s = _mm_castps_si128(_mm_cmpgt_ps(_mm_cvtepi32_ps(i), v));
	return _mm_cvtepi32_ps(_mm_sub_epi32(i, _mm_srli_epi32(s, 31)));
}

#if !defined max
Simd4f max(const Simd4f& v0, const Simd4f& v1)
{
	return _mm_max_ps(v0, v1);
}
#endif

#if !defined min
Simd4f min(const Simd4f& v0, const Simd4f& v1)
{
	return _mm_min_ps(v0, v1);
}
#endif

Simd4f recip(const Simd4f& v)
{
	return _mm_rcp_ps(v);
}

template <int n>
Simd4f recip(const Simd4f& v)
{
	Simd4f two = simd4f(2.0f);
	Simd4f r = recip(v);
	for(int i = 0; i < n; ++i)
		r = r * (two - v * r);
	return r;
}

Simd4f sqrt(const Simd4f& v)
{
	return _mm_sqrt_ps(v);
}

Simd4f rsqrt(const Simd4f& v)
{
	return _mm_rsqrt_ps(v);
}

template <int n>
Simd4f rsqrt(const Simd4f& v)
{
	Simd4f halfV = v * simd4f(0.5f);
	Simd4f threeHalf = simd4f(1.5f);
	Simd4f r = rsqrt(v);
	for(int i = 0; i < n; ++i)
		r = r * (threeHalf - halfV * r * r);
	return r;
}

Simd4f exp2(const Simd4f& v)
{
	// http://www.netlib.org/cephes/

	Simd4f limit = simd4f(127.4999f);
	Simd4f x = min(max(-limit, v), limit);

	// separate into integer and fractional part

	Simd4f fx = x + simd4f(0.5f);
	Simd4i ix = _mm_sub_epi32(_mm_cvttps_epi32(fx), _mm_srli_epi32(_mm_castps_si128(fx), 31));
	fx = x - Simd4f(_mm_cvtepi32_ps(ix));

	// exp2(fx) ~ 1 + 2*P(fx) / (Q(fx) - P(fx))

	Simd4f fx2 = fx * fx;

	Simd4f px = fx * (simd4f(1.51390680115615096133e+3f) +
	                  fx2 * (simd4f(2.02020656693165307700e+1f) + fx2 * simd4f(2.30933477057345225087e-2f)));
	Simd4f qx = simd4f(4.36821166879210612817e+3f) + fx2 * (simd4f(2.33184211722314911771e+2f) + fx2);

	Simd4f exp2fx = px * recip(qx - px);
	exp2fx = gSimd4fOne + exp2fx + exp2fx;

	// exp2(ix)

	Simd4f exp2ix = _mm_castsi128_ps(_mm_slli_epi32(_mm_add_epi32(ix, _mm_set1_epi32(0x7f)), 23));

	return exp2fx * exp2ix;
}

Simd4f log2(const Simd4f& v)
{
	// todo: fast approximate implementation like exp2
	Simd4f scale = simd4f(1.44269504088896341f); // 1/ln(2)
	const float* ptr = array(v);
	return simd4f(::logf(ptr[0]), ::logf(ptr[1]), ::logf(ptr[2]), ::logf(ptr[3])) * scale;
}

Simd4f dot3(const Simd4f& v0, const Simd4f& v1)
{
	Simd4f tmp = v0 * v1;
	return splat<0>(tmp) + splat<1>(tmp) + splat<2>(tmp);
}

Simd4f cross3(const Simd4f& v0, const Simd4f& v1)
{
	Simd4f t0 = _mm_shuffle_ps(v0, v0, 0xc9); // w z y x -> w x z y
	Simd4f t1 = _mm_shuffle_ps(v1, v1, 0xc9);
	Simd4f tmp = v0 * t1 - t0 * v1;
	return _mm_shuffle_ps(tmp, tmp, 0xc9);
}

void transpose(Simd4f& x, Simd4f& y, Simd4f& z, Simd4f& w)
{
	_MM_TRANSPOSE4_PS(x, y, z, w);
}

void zip(Simd4f& v0, Simd4f& v1)
{
	Simd4f t0 = v0;
	v0 = _mm_unpacklo_ps(v0, v1);
	v1 = _mm_unpackhi_ps(t0, v1);
}

void unzip(Simd4f& v0, Simd4f& v1)
{
	Simd4f t0 = v0;
	v0 = _mm_shuffle_ps(v0, v1, _MM_SHUFFLE(2, 0, 2, 0));
	v1 = _mm_shuffle_ps(t0, v1, _MM_SHUFFLE(3, 1, 3, 1));
}

Simd4f swaphilo(const Simd4f& v)
{
	return _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 0, 3, 2));
}

int allEqual(const Simd4f& v0, const Simd4f& v1)
{
	return allTrue(v0 == v1);
}

int allEqual(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask)
{
	return allTrue(outMask = v0 == v1);
}

int anyEqual(const Simd4f& v0, const Simd4f& v1)
{
	return anyTrue(v0 == v1);
}

int anyEqual(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask)
{
	return anyTrue(outMask = v0 == v1);
}

int allGreater(const Simd4f& v0, const Simd4f& v1)
{
	return allTrue(v0 > v1);
}

int allGreater(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask)
{
	return allTrue(outMask = v0 > v1);
}

int anyGreater(const Simd4f& v0, const Simd4f& v1)
{
	return anyTrue(v0 > v1);
}

int anyGreater(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask)
{
	return anyTrue(outMask = v0 > v1);
}

int allGreaterEqual(const Simd4f& v0, const Simd4f& v1)
{
	return allTrue(v0 >= v1);
}

int allGreaterEqual(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask)
{
	return allTrue(outMask = v0 >= v1);
}

int anyGreaterEqual(const Simd4f& v0, const Simd4f& v1)
{
	return anyTrue(v0 >= v1);
}

int anyGreaterEqual(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask)
{
	return anyTrue(outMask = v0 >= v1);
}

int allTrue(const Simd4f& v)
{
	return _mm_movemask_ps(v) == 0xf;
}

int anyTrue(const Simd4f& v)
{
	return _mm_movemask_ps(v);
}

NV_SIMD_NAMESPACE_END
