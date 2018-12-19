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
	return vdupq_n_u32(0);
}

Simd4fOneFactory::operator Simd4f() const
{
	return vdupq_n_f32(1.0f);
}

Simd4fScalarFactory::operator Simd4f() const
{
	return vdupq_n_f32(reinterpret_cast<const float32_t&>(value));
}

Simd4fTupleFactory::operator Simd4f() const
{
	return reinterpret_cast<const Simd4f&>(tuple);
}

Simd4fLoadFactory::operator Simd4f() const
{
	return vld1q_f32(static_cast<const float32_t*>(ptr));
}

Simd4fLoad3Factory::operator Simd4f() const
{
#if 0
	float32x2_t xy = vld1_f32(ptr);
	float32x2_t zz = vld1_dup_f32(ptr+2);
	return vcombine_f32(xy, zz);
#else
	float fltArray[] = { ptr[0], ptr[1], ptr[2], 0.0 };
	return vld1q_f32(static_cast<const float32_t*>(fltArray));
#endif
}

Simd4fLoad3SetWFactory::operator Simd4f() const
{
#if 0
	float32x2_t xy = vld1_f32(ptr);
	float32x2_t zz = vld1_dup_f32(ptr+2);
	return vcombine_f32(xy, zz);
#else
	float fltArray[] = { ptr[0], ptr[1], ptr[2], w };
	return vld1q_f32(static_cast<const float32_t*>(fltArray));
#endif
}

Simd4fAlignedLoadFactory::operator Simd4f() const
{
	return vld1q_f32(static_cast<const float32_t*>(ptr));
}

Simd4fOffsetLoadFactory::operator Simd4f() const
{
	return vld1q_f32(reinterpret_cast<const float32_t*>(reinterpret_cast<const char*>(ptr) + offset));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// expression templates
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <>
inline ComplementExpr<Simd4f>::operator Simd4f() const
{
	return vbicq_u32(vdupq_n_u32(0xffffffff), v.u4);
}

template <>
inline Simd4f operator&(const ComplementExpr<Simd4f>& complement, const Simd4f& v)
{
	return vbicq_u32(v.u4, complement.v.u4);
}

template <>
inline Simd4f operator&(const Simd4f& v, const ComplementExpr<Simd4f>& complement)
{
	return vbicq_u32(v.u4, complement.v.u4);
}

ProductExpr::operator Simd4f() const
{
	return vmulq_f32(v0.f4, v1.f4);
}

Simd4f operator+(const ProductExpr& p, const Simd4f& v)
{
	return vmlaq_f32(v.f4, p.v0.f4, p.v1.f4);
}

Simd4f operator+(const Simd4f& v, const ProductExpr& p)
{
	return vmlaq_f32(v.f4, p.v0.f4, p.v1.f4);
}

Simd4f operator+(const ProductExpr& p0, const ProductExpr& p1)
{
	// cast calls operator Simd4f() which evaluates the other ProductExpr
	return vmlaq_f32(static_cast<Simd4f>(p0).f4, p1.v0.f4, p1.v1.f4);
}

Simd4f operator-(const Simd4f& v, const ProductExpr& p)
{
	return vmlsq_f32(v.f4, p.v0.f4, p.v1.f4);
}

Simd4f operator-(const ProductExpr& p0, const ProductExpr& p1)
{
	// cast calls operator Simd4f() which evaluates the other ProductExpr
	return vmlsq_f32(static_cast<Simd4f>(p0).f4, p1.v0.f4, p1.v1.f4);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// operator implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Simd4f operator==(const Simd4f& v0, const Simd4f& v1)
{
	return vceqq_f32(v0.f4, v1.f4);
}

Simd4f operator<(const Simd4f& v0, const Simd4f& v1)
{
	return vcltq_f32(v0.f4, v1.f4);
}

Simd4f operator<=(const Simd4f& v0, const Simd4f& v1)
{
	return vcleq_f32(v0.f4, v1.f4);
}

Simd4f operator>(const Simd4f& v0, const Simd4f& v1)
{
	return vcgtq_f32(v0.f4, v1.f4);
}

Simd4f operator>=(const Simd4f& v0, const Simd4f& v1)
{
	return vcgeq_f32(v0.f4, v1.f4);
}

ComplementExpr<Simd4f> operator~(const Simd4f& v)
{
	return ComplementExpr<Simd4f>(v);
}

Simd4f operator&(const Simd4f& v0, const Simd4f& v1)
{
	return vandq_u32(v0.u4, v1.u4);
}

Simd4f operator|(const Simd4f& v0, const Simd4f& v1)
{
	return vorrq_u32(v0.u4, v1.u4);
}

Simd4f operator^(const Simd4f& v0, const Simd4f& v1)
{
	return veorq_u32(v0.u4, v1.u4);
}

Simd4f operator<<(const Simd4f& v, int shift)
{
	return vshlq_u32(v.u4, vdupq_n_s32(shift));
}

Simd4f operator>>(const Simd4f& v, int shift)
{
	return vshlq_u32(v.u4, vdupq_n_s32(-shift));
}

Simd4f operator<<(const Simd4f& v, const Simd4f& shift)
{
	return vshlq_u32(v.u4, shift.i4);
}

Simd4f operator>>(const Simd4f& v, const Simd4f& shift)
{
	return vshlq_u32(v.u4, vnegq_s32(shift.i4));
}

Simd4f operator+(const Simd4f& v)
{
	return v;
}

Simd4f operator+(const Simd4f& v0, const Simd4f& v1)
{
	return vaddq_f32(v0.f4, v1.f4);
}

Simd4f operator-(const Simd4f& v)
{
	return vnegq_f32(v.f4);
}

Simd4f operator-(const Simd4f& v0, const Simd4f& v1)
{
	return vsubq_f32(v0.f4, v1.f4);
}

ProductExpr operator*(const Simd4f& v0, const Simd4f& v1)
{
	return ProductExpr(v0, v1);
}

Simd4f operator/(const Simd4f& v0, const Simd4f& v1)
{
	return v0 * vrecpeq_f32(v1.f4); // reciprocal estimate
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// function implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Simd4f simd4f(const Simd4i& v)
{
	return v.u4;
}

Simd4f convert(const Simd4i& v)
{
	return vcvtq_f32_s32(v.i4);
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
	return vst1q_f32(reinterpret_cast<float32_t*>(ptr), v.f4);
}

void storeAligned(float* ptr, Simd4f const& v)
{
	return vst1q_f32(reinterpret_cast<float32_t*>(ptr), v.f4);
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
	return storeAligned(reinterpret_cast<float*>(reinterpret_cast<char*>(ptr) + offset), v);
}

template <size_t i>
Simd4f splat(Simd4f const& v)
{
	return vdupq_n_f32(array(v)[i]);
}

Simd4f select(Simd4f const& mask, Simd4f const& v0, Simd4f const& v1)
{
	return vbslq_f32(mask.u4, v0.f4, v1.f4);
}

Simd4f abs(const Simd4f& v)
{
	return vabsq_f32(v.f4);
}

Simd4f floor(const Simd4f& v)
{
	int32x4_t i = vcvtq_s32_f32(v.f4);
	int32x4_t s = vreinterpretq_s32_u32(vcgtq_f32(vcvtq_f32_s32(i), v.f4));
	return vcvtq_f32_s32(vsubq_s32(i, vshrq_n_s32(s, 31)));
}

#if !defined max
Simd4f max(const Simd4f& v0, const Simd4f& v1)
{
	return vmaxq_f32(v0.f4, v1.f4);
}
#endif

#if !defined min
Simd4f min(const Simd4f& v0, const Simd4f& v1)
{
	return vminq_f32(v0.f4, v1.f4);
}
#endif

Simd4f recip(const Simd4f& v)
{
	return recip<0>(v);
}

template <int n>
Simd4f recip(const Simd4f& v)
{
	Simd4f r = vrecpeq_f32(v.f4);
	// n+1 newton iterations because initial approximation is crude
	for(int i = 0; i <= n; ++i)
		r = vrecpsq_f32(v.f4, r.f4) * r;
	return r;
}

Simd4f sqrt(const Simd4f& v)
{
	return (v > gSimd4fZero) & (v * rsqrt(v));
}

Simd4f rsqrt(const Simd4f& v)
{
	return rsqrt<0>(v);
}

template <int n>
Simd4f rsqrt(const Simd4f& v)
{
	Simd4f r = vrsqrteq_f32(v.f4);
	// n+1 newton iterations because initial approximation is crude
	for(int i = 0; i <= n; ++i)
		r = vrsqrtsq_f32(vmulq_f32(v.f4, r.f4), r.f4) * r;
	return r;
}

Simd4f exp2(const Simd4f& v)
{
	// http://www.netlib.org/cephes/

	Simd4f limit = simd4f(127.4999f);
	Simd4f x = min(max(-limit, v), limit);

	// separate into integer and fractional part

	Simd4f fx = x + simd4f(0.5f);
	Simd4i ix = vsubq_s32(vcvtq_s32_f32(fx.f4), vreinterpretq_s32_u32(vshrq_n_u32(fx.u4, 31)));
	fx = x - vcvtq_f32_s32(ix.i4);

	// exp2(fx) ~ 1 + 2*P(fx) / (Q(fx) - P(fx))

	Simd4f fx2 = fx * fx;

	Simd4f px = fx * (simd4f(1.51390680115615096133e+3f) +
	                  fx2 * (simd4f(2.02020656693165307700e+1f) + fx2 * simd4f(2.30933477057345225087e-2f)));
	Simd4f qx = simd4f(4.36821166879210612817e+3f) + fx2 * (simd4f(2.33184211722314911771e+2f) + fx2);

	Simd4f exp2fx = px * recip(qx - px);
	exp2fx = gSimd4fOne + exp2fx + exp2fx;

	// exp2(ix)

	Simd4f exp2ix = vreinterpretq_f32_s32(vshlq_n_s32(vaddq_s32(ix.i4, vdupq_n_s32(0x7f)), 23));

	return exp2fx * exp2ix;
}

Simd4f log2(const Simd4f& v)
{
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
	float32x2_t x0_y0 = vget_low_f32(v0.f4);
	float32x2_t z0_w0 = vget_high_f32(v0.f4);
	float32x2_t x1_y1 = vget_low_f32(v1.f4);
	float32x2_t z1_w1 = vget_high_f32(v1.f4);

	float32x2_t y1_z1 = vext_f32(x1_y1, z1_w1, 1);
	float32x2_t y0_z0 = vext_f32(x0_y0, z0_w0, 1);

	float32x2_t z0x1_w0y1 = vmul_f32(z0_w0, x1_y1);
	float32x2_t x0y1_y0z1 = vmul_f32(x0_y0, y1_z1);

	float32x2_t y2_w2 = vmls_f32(z0x1_w0y1, x0_y0, z1_w1);
	float32x2_t z2_x2 = vmls_f32(x0y1_y0z1, y0_z0, x1_y1);
	float32x2_t x2_y2 = vext_f32(z2_x2, y2_w2, 1);

	return vcombine_f32(x2_y2, z2_x2);
}

void transpose(Simd4f& x, Simd4f& y, Simd4f& z, Simd4f& w)
{
#if NV_SIMD_INLINE_ASSEMBLER
	asm volatile("vzip.f32 %q0, %q2 \n\t"
	             "vzip.f32 %q1, %q3 \n\t"
	             "vzip.f32 %q0, %q1 \n\t"
	             "vzip.f32 %q2, %q3 \n\t"
	             : "+w"(x.f4), "+w"(y.f4), "+w"(z.f4), "+w"(w.f4));
#else
	float32x4x2_t v0v1 = vzipq_f32(x.f4, z.f4);
	float32x4x2_t v2v3 = vzipq_f32(y.f4, w.f4);
	float32x4x2_t zip0 = vzipq_f32(v0v1.val[0], v2v3.val[0]);
	float32x4x2_t zip1 = vzipq_f32(v0v1.val[1], v2v3.val[1]);

	x = zip0.val[0];
	y = zip0.val[1];
	z = zip1.val[0];
	w = zip1.val[1];
#endif
}

void zip(Simd4f& v0, Simd4f& v1)
{
#if NV_SIMD_INLINE_ASSEMBLER
	asm volatile("vzip.f32 %q0, %q1 \n\t" : "+w"(v0.f4), "+w"(v1.f4));
#else
	float32x4x2_t uzp = vzipq_f32(v0.f4, v1.f4);
	v0 = uzp.val[0];
	v1 = uzp.val[1];
#endif
}

void unzip(Simd4f& v0, Simd4f& v1)
{
#if NV_SIMD_INLINE_ASSEMBLER
	asm volatile("vuzp.f32 %q0, %q1 \n\t" : "+w"(v0.f4), "+w"(v1.f4));
#else
	float32x4x2_t uzp = vuzpq_f32(v0.f4, v1.f4);
	v0 = uzp.val[0];
	v1 = uzp.val[1];
#endif
}

Simd4f swaphilo(const Simd4f& v)
{
	return vcombine_f32(vget_high_f32(v.f4), vget_low_f32(v.f4));
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
#if NV_SIMD_INLINE_ASSEMBLER
	int result;
	asm volatile("vmovq q0, %q1 \n\t"
	             "vand.u32 d0, d0, d1 \n\t"
	             "vpmin.u32 d0, d0, d0 \n\t"
	             "vcmp.f32 s0, #0 \n\t"
	             "fmrx %0, fpscr"
	             : "=r"(result)
	             : "w"(v.f4)
	             : "q0");
	return result >> 28 & 0x1;
#else
	uint16x4_t hi = vget_high_u16(vreinterpretq_u16_u32(v.u4));
	uint16x4_t lo = vmovn_u32(v.u4);
	uint16x8_t combined = vcombine_u16(lo, hi);
	uint32x2_t reduced = vreinterpret_u32_u8(vmovn_u16(combined));
	return vget_lane_u32(reduced, 0) == 0xffffffff;
#endif
}

int anyTrue(const Simd4f& v)
{
#if NV_SIMD_INLINE_ASSEMBLER
	int result;
	asm volatile("vmovq q0, %q1 \n\t"
	             "vorr.u32 d0, d0, d1 \n\t"
	             "vpmax.u32 d0, d0, d0 \n\t"
	             "vcmp.f32 s0, #0 \n\t"
	             "fmrx %0, fpscr"
	             : "=r"(result)
	             : "w"(v.f4)
	             : "q0");
	return result >> 28 & 0x1;
#else
	uint16x4_t hi = vget_high_u16(vreinterpretq_u16_u32(v.u4));
	uint16x4_t lo = vmovn_u32(v.u4);
	uint16x8_t combined = vcombine_u16(lo, hi);
	uint32x2_t reduced = vreinterpret_u32_u8(vmovn_u16(combined));
	return vget_lane_u32(reduced, 0) != 0x0;
#endif
}

NV_SIMD_NAMESPACE_END
