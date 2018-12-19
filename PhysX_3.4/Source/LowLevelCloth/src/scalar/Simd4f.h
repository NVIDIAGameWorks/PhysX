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

Simd4fZeroFactory::operator Scalar4f() const
{
	return Scalar4f(0.0f, 0.0f, 0.0f, 0.0f);
}

Simd4fOneFactory::operator Scalar4f() const
{
	return Scalar4f(1.0f, 1.0f, 1.0f, 1.0f);
}

Simd4fScalarFactory::operator Scalar4f() const
{
	return Scalar4f(value, value, value, value);
}

Simd4fTupleFactory::operator Scalar4f() const
{
	return reinterpret_cast<const Scalar4f&>(tuple);
}

Simd4fLoadFactory::operator Scalar4f() const
{
	return Scalar4f(ptr[0], ptr[1], ptr[2], ptr[3]);
}

Simd4fLoad3Factory::operator Scalar4f() const
{
	return Scalar4f(ptr[0], ptr[1], ptr[2], 0.0f);
}

Simd4fLoad3SetWFactory::operator Scalar4f() const
{
	return Scalar4f(ptr[0], ptr[1], ptr[2], w);
}

Simd4fAlignedLoadFactory::operator Scalar4f() const
{
	return Scalar4f(ptr[0], ptr[1], ptr[2], ptr[3]);
}

Simd4fOffsetLoadFactory::operator Scalar4f() const
{
	return Simd4fAlignedLoadFactory(reinterpret_cast<const float*>(reinterpret_cast<const char*>(ptr) + offset));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// expression template
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <>
inline ComplementExpr<Scalar4f>::operator Scalar4f() const
{
	return Scalar4f(~v.u4[0], ~v.u4[1], ~v.u4[2], ~v.u4[3]);
}

template <>
inline Scalar4f operator&<Scalar4f>(const ComplementExpr<Scalar4f>& complement, const Scalar4f& v)
{
	return Scalar4f(v.u4[0] & ~complement.v.u4[0], v.u4[1] & ~complement.v.u4[1], v.u4[2] & ~complement.v.u4[2],
	                v.u4[3] & ~complement.v.u4[3]);
}

template <>
inline Scalar4f operator&<Scalar4f>(const Scalar4f& v, const ComplementExpr<Scalar4f>& complement)
{
	return Scalar4f(v.u4[0] & ~complement.v.u4[0], v.u4[1] & ~complement.v.u4[1], v.u4[2] & ~complement.v.u4[2],
	                v.u4[3] & ~complement.v.u4[3]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// operator implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline Scalar4f operator==(const Scalar4f& v0, const Scalar4f& v1)
{
	return Scalar4f(v0.f4[0] == v1.f4[0], v0.f4[1] == v1.f4[1], v0.f4[2] == v1.f4[2], v0.f4[3] == v1.f4[3]);
}

inline Scalar4f operator<(const Scalar4f& v0, const Scalar4f& v1)
{
	return Scalar4f(v0.f4[0] < v1.f4[0], v0.f4[1] < v1.f4[1], v0.f4[2] < v1.f4[2], v0.f4[3] < v1.f4[3]);
}

inline Scalar4f operator<=(const Scalar4f& v0, const Scalar4f& v1)
{
	return Scalar4f(v0.f4[0] <= v1.f4[0], v0.f4[1] <= v1.f4[1], v0.f4[2] <= v1.f4[2], v0.f4[3] <= v1.f4[3]);
}

inline Scalar4f operator>(const Scalar4f& v0, const Scalar4f& v1)
{
	return Scalar4f(v0.f4[0] > v1.f4[0], v0.f4[1] > v1.f4[1], v0.f4[2] > v1.f4[2], v0.f4[3] > v1.f4[3]);
}

inline Scalar4f operator>=(const Scalar4f& v0, const Scalar4f& v1)
{
	return Scalar4f(v0.f4[0] >= v1.f4[0], v0.f4[1] >= v1.f4[1], v0.f4[2] >= v1.f4[2], v0.f4[3] >= v1.f4[3]);
}

inline ComplementExpr<Scalar4f> operator~(const Scalar4f& v)
{
	return ComplementExpr<Scalar4f>(v);
}

inline Scalar4f operator&(const Scalar4f& v0, const Scalar4f& v1)
{
	return Scalar4f(v0.u4[0] & v1.u4[0], v0.u4[1] & v1.u4[1], v0.u4[2] & v1.u4[2], v0.u4[3] & v1.u4[3]);
}

inline Scalar4f operator|(const Scalar4f& v0, const Scalar4f& v1)
{
	return Scalar4f(v0.u4[0] | v1.u4[0], v0.u4[1] | v1.u4[1], v0.u4[2] | v1.u4[2], v0.u4[3] | v1.u4[3]);
}

inline Scalar4f operator^(const Scalar4f& v0, const Scalar4f& v1)
{
	return Scalar4f(v0.u4[0] ^ v1.u4[0], v0.u4[1] ^ v1.u4[1], v0.u4[2] ^ v1.u4[2], v0.u4[3] ^ v1.u4[3]);
}

inline Scalar4f operator<<(const Scalar4f& v, int shift)
{
	return Scalar4f(v.u4[0] << shift, v.u4[1] << shift, v.u4[2] << shift, v.u4[3] << shift);
}

inline Scalar4f operator>>(const Scalar4f& v, int shift)
{
	return Scalar4f(v.u4[0] >> shift, v.u4[1] >> shift, v.u4[2] >> shift, v.u4[3] >> shift);
}

inline Scalar4f operator+(const Scalar4f& v)
{
	return v;
}

inline Scalar4f operator+(const Scalar4f& v0, const Scalar4f& v1)
{
	return Scalar4f(v0.f4[0] + v1.f4[0], v0.f4[1] + v1.f4[1], v0.f4[2] + v1.f4[2], v0.f4[3] + v1.f4[3]);
}

inline Scalar4f operator-(const Scalar4f& v)
{
	return Scalar4f(-v.f4[0], -v.f4[1], -v.f4[2], -v.f4[3]);
}

inline Scalar4f operator-(const Scalar4f& v0, const Scalar4f& v1)
{
	return Scalar4f(v0.f4[0] - v1.f4[0], v0.f4[1] - v1.f4[1], v0.f4[2] - v1.f4[2], v0.f4[3] - v1.f4[3]);
}

inline Scalar4f operator*(const Scalar4f& v0, const Scalar4f& v1)
{
	return Scalar4f(v0.f4[0] * v1.f4[0], v0.f4[1] * v1.f4[1], v0.f4[2] * v1.f4[2], v0.f4[3] * v1.f4[3]);
}

inline Scalar4f operator/(const Scalar4f& v0, const Scalar4f& v1)
{
	return Scalar4f(v0.f4[0] / v1.f4[0], v0.f4[1] / v1.f4[1], v0.f4[2] / v1.f4[2], v0.f4[3] / v1.f4[3]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// function implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline Scalar4f simd4f(const Scalar4i& v)
{
	return reinterpret_cast<const Scalar4f&>(v);
}

inline Scalar4f convert(const Scalar4i& v)
{
	return Scalar4f(float(v.i4[0]), float(v.i4[1]), float(v.i4[2]), float(v.i4[3]));
}

inline float (&array(Scalar4f& v))[4]
{
	return v.f4;
}

inline const float (&array(const Scalar4f& v))[4]
{
	return v.f4;
}

inline void store(float* ptr, const Scalar4f& v)
{
	ptr[0] = v.f4[0];
	ptr[1] = v.f4[1];
	ptr[2] = v.f4[2];
	ptr[3] = v.f4[3];
}

inline void storeAligned(float* ptr, const Scalar4f& v)
{
	store(ptr, v);
}

inline void store3(float* ptr, const Scalar4f& v)
{
	ptr[0] = v.f4[0];
	ptr[1] = v.f4[1];
	ptr[2] = v.f4[2];
}

inline void storeAligned(float* ptr, unsigned int offset, const Scalar4f& v)
{
	storeAligned(reinterpret_cast<float*>(reinterpret_cast<char*>(ptr) + offset), v);
}

template <size_t i>
inline Scalar4f splat(const Scalar4f& v)
{
	return Scalar4f(v.f4[i], v.f4[i], v.f4[i], v.f4[i]);
}

inline Scalar4f select(const Scalar4f& mask, const Scalar4f& v0, const Scalar4f& v1)
{
	return ((v0 ^ v1) & mask) ^ v1;
}

inline Scalar4f abs(const Scalar4f& v)
{
	return Scalar4f(::fabsf(v.f4[0]), ::fabsf(v.f4[1]), ::fabsf(v.f4[2]), ::fabsf(v.f4[3]));
}

inline Scalar4f floor(const Scalar4f& v)
{
	return Scalar4f(::floorf(v.f4[0]), ::floorf(v.f4[1]), ::floorf(v.f4[2]), ::floorf(v.f4[3]));
}

#if !defined max
inline Scalar4f max(const Scalar4f& v0, const Scalar4f& v1)
{
	return Scalar4f(std::max(v0.f4[0], v1.f4[0]), std::max(v0.f4[1], v1.f4[1]), std::max(v0.f4[2], v1.f4[2]),
	                std::max(v0.f4[3], v1.f4[3]));
}
#endif

#if !defined min
inline Scalar4f min(const Scalar4f& v0, const Scalar4f& v1)
{
	return Scalar4f(std::min(v0.f4[0], v1.f4[0]), std::min(v0.f4[1], v1.f4[1]), std::min(v0.f4[2], v1.f4[2]),
	                std::min(v0.f4[3], v1.f4[3]));
}
#endif

inline Scalar4f recip(const Scalar4f& v)
{
	return Scalar4f(1 / v.f4[0], 1 / v.f4[1], 1 / v.f4[2], 1 / v.f4[3]);
}

template <int n>
inline Scalar4f recip(const Scalar4f& v)
{
	return recip(v);
}

inline Scalar4f sqrt(const Scalar4f& v)
{
	return Scalar4f(::sqrtf(v.f4[0]), ::sqrtf(v.f4[1]), ::sqrtf(v.f4[2]), ::sqrtf(v.f4[3]));
}

inline Scalar4f rsqrt(const Scalar4f& v)
{
	return recip(sqrt(v));
}

template <int n>
inline Scalar4f rsqrt(const Scalar4f& v)
{
	return rsqrt(v);
}

inline Scalar4f exp2(const Scalar4f& v)
{
	float scale = 0.69314718055994531f; // ::logf(2.0f);
	return Scalar4f(::expf(v.f4[0] * scale), ::expf(v.f4[1] * scale), ::expf(v.f4[2] * scale), ::expf(v.f4[3] * scale));
}

inline Scalar4f log2(const Scalar4f& v)
{
	float scale = 1.44269504088896341f; // 1/ln(2)
	return Scalar4f(::logf(v.f4[0]) * scale, ::logf(v.f4[1]) * scale, ::logf(v.f4[2]) * scale, ::logf(v.f4[3]) * scale);
}

inline Scalar4f dot3(const Scalar4f& v0, const Scalar4f& v1)
{
	return simd4f(v0.f4[0] * v1.f4[0] + v0.f4[1] * v1.f4[1] + v0.f4[2] * v1.f4[2]);
}

inline Scalar4f cross3(const Scalar4f& v0, const Scalar4f& v1)
{
	return simd4f(v0.f4[1] * v1.f4[2] - v0.f4[2] * v1.f4[1], v0.f4[2] * v1.f4[0] - v0.f4[0] * v1.f4[2],
	              v0.f4[0] * v1.f4[1] - v0.f4[1] * v1.f4[0], 0.0f);
}

inline void transpose(Scalar4f& x, Scalar4f& y, Scalar4f& z, Scalar4f& w)
{
	float x1 = x.f4[1], x2 = x.f4[2], x3 = x.f4[3];
	float y2 = y.f4[2], y3 = y.f4[3], z3 = z.f4[3];

	x.f4[1] = y.f4[0];
	x.f4[2] = z.f4[0];
	x.f4[3] = w.f4[0];
	y.f4[0] = x1;
	y.f4[2] = z.f4[1];
	y.f4[3] = w.f4[1];
	z.f4[0] = x2;
	z.f4[1] = y2;
	z.f4[3] = w.f4[2];
	w.f4[0] = x3;
	w.f4[1] = y3;
	w.f4[2] = z3;
}

inline void zip(Scalar4f& v0, Scalar4f& v1)
{
	float z0 = v0.f4[2];
	v0.f4[2] = v0.f4[1];
	v0.f4[1] = v1.f4[0];
	v1.f4[0] = z0;

	float z1 = v1.f4[2];
	v1.f4[2] = v0.f4[3];
	v0.f4[3] = v1.f4[1];
	v1.f4[1] = z1;
}

inline void unzip(Scalar4f& v0, Scalar4f& v1)
{
	float z0 = v0.f4[2];
	v0.f4[2] = v1.f4[0];
	v1.f4[0] = v0.f4[1];
	v0.f4[1] = z0;

	float z1 = v1.f4[2];
	v1.f4[2] = v1.f4[1];
	v1.f4[1] = v0.f4[3];
	v0.f4[3] = z1;
}

inline Scalar4f swaphilo(const Scalar4f& v)
{
	return Scalar4f(v.f4[2], v.f4[3], v.f4[0], v.f4[1]);
}

inline int allEqual(const Scalar4f& v0, const Scalar4f& v1)
{
	return v0.f4[0] == v1.f4[0] && v0.f4[1] == v1.f4[1] && v0.f4[2] == v1.f4[2] && v0.f4[3] == v1.f4[3];
}

inline int allEqual(const Scalar4f& v0, const Scalar4f& v1, Scalar4f& outMask)
{
	bool b0 = v0.f4[0] == v1.f4[0], b1 = v0.f4[1] == v1.f4[1], b2 = v0.f4[2] == v1.f4[2], b3 = v0.f4[3] == v1.f4[3];
	outMask = Scalar4f(b0, b1, b2, b3);
	return b0 && b1 && b2 && b3;
}

inline int anyEqual(const Scalar4f& v0, const Scalar4f& v1)
{
	return v0.f4[0] == v1.f4[0] || v0.f4[1] == v1.f4[1] || v0.f4[2] == v1.f4[2] || v0.f4[3] == v1.f4[3];
}

inline int anyEqual(const Scalar4f& v0, const Scalar4f& v1, Scalar4f& outMask)
{
	bool b0 = v0.f4[0] == v1.f4[0], b1 = v0.f4[1] == v1.f4[1], b2 = v0.f4[2] == v1.f4[2], b3 = v0.f4[3] == v1.f4[3];
	outMask = Scalar4f(b0, b1, b2, b3);
	return b0 || b1 || b2 || b3;
}

inline int allGreater(const Scalar4f& v0, const Scalar4f& v1)
{
	return v0.f4[0] > v1.f4[0] && v0.f4[1] > v1.f4[1] && v0.f4[2] > v1.f4[2] && v0.f4[3] > v1.f4[3];
}

inline int allGreater(const Scalar4f& v0, const Scalar4f& v1, Scalar4f& outMask)
{
	bool b0 = v0.f4[0] > v1.f4[0], b1 = v0.f4[1] > v1.f4[1], b2 = v0.f4[2] > v1.f4[2], b3 = v0.f4[3] > v1.f4[3];
	outMask = Scalar4f(b0, b1, b2, b3);
	return b0 && b1 && b2 && b3;
}

inline int anyGreater(const Scalar4f& v0, const Scalar4f& v1)
{
	return v0.f4[0] > v1.f4[0] || v0.f4[1] > v1.f4[1] || v0.f4[2] > v1.f4[2] || v0.f4[3] > v1.f4[3];
}

inline int anyGreater(const Scalar4f& v0, const Scalar4f& v1, Scalar4f& outMask)
{
	bool b0 = v0.f4[0] > v1.f4[0], b1 = v0.f4[1] > v1.f4[1], b2 = v0.f4[2] > v1.f4[2], b3 = v0.f4[3] > v1.f4[3];
	outMask = Scalar4f(b0, b1, b2, b3);
	return b0 || b1 || b2 || b3;
}

inline int allGreaterEqual(const Scalar4f& v0, const Scalar4f& v1)
{
	return v0.f4[0] >= v1.f4[0] && v0.f4[1] >= v1.f4[1] && v0.f4[2] >= v1.f4[2] && v0.f4[3] >= v1.f4[3];
}

inline int allGreaterEqual(const Scalar4f& v0, const Scalar4f& v1, Scalar4f& outMask)
{
	bool b0 = v0.f4[0] >= v1.f4[0], b1 = v0.f4[1] >= v1.f4[1], b2 = v0.f4[2] >= v1.f4[2], b3 = v0.f4[3] >= v1.f4[3];
	outMask = Scalar4f(b0, b1, b2, b3);
	return b0 && b1 && b2 && b3;
}

inline int anyGreaterEqual(const Scalar4f& v0, const Scalar4f& v1)
{
	return v0.f4[0] >= v1.f4[0] || v0.f4[1] >= v1.f4[1] || v0.f4[2] >= v1.f4[2] || v0.f4[3] >= v1.f4[3];
}

inline int anyGreaterEqual(const Scalar4f& v0, const Scalar4f& v1, Scalar4f& outMask)
{
	bool b0 = v0.f4[0] >= v1.f4[0], b1 = v0.f4[1] >= v1.f4[1], b2 = v0.f4[2] >= v1.f4[2], b3 = v0.f4[3] >= v1.f4[3];
	outMask = Scalar4f(b0, b1, b2, b3);
	return b0 || b1 || b2 || b3;
}

inline int allTrue(const Scalar4f& v)
{
	return v.i4[0] & v.i4[1] & v.i4[2] & v.i4[3];
}

inline int anyTrue(const Scalar4f& v)
{
	return v.i4[0] | v.i4[1] | v.i4[2] | v.i4[3];
}

NV_SIMD_NAMESPACE_END
