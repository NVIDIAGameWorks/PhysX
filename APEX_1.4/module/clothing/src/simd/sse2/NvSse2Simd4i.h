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

Simd4iZeroFactory::operator Simd4i() const
{
	return _mm_setzero_si128();
}

Simd4iScalarFactory::operator Simd4i() const
{
	return _mm_set1_epi32(value);
}

Simd4iTupleFactory::operator Simd4i() const
{
	return reinterpret_cast<const Simd4i&>(tuple);
}

Simd4iLoadFactory::operator Simd4i() const
{
	return _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
}

Simd4iAlignedLoadFactory::operator Simd4i() const
{
	return _mm_load_si128(reinterpret_cast<const __m128i*>(ptr));
}

Simd4iOffsetLoadFactory::operator Simd4i() const
{
	return _mm_load_si128(reinterpret_cast<const __m128i*>(reinterpret_cast<const char*>(ptr) + offset));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// expression template
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <>
inline ComplementExpr<Simd4i>::operator Simd4i() const
{
	return _mm_andnot_si128(v, _mm_set1_epi32(0xffffffff));
}

template <>
inline Simd4i operator&(const ComplementExpr<Simd4i>& complement, const Simd4i& v)
{
	return _mm_andnot_si128(complement.v, v);
}

template <>
inline Simd4i operator&(const Simd4i& v, const ComplementExpr<Simd4i>& complement)
{
	return _mm_andnot_si128(complement.v, v);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// operator implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Simd4i operator==(const Simd4i& v0, const Simd4i& v1)
{
	return _mm_cmpeq_epi32(v0, v1);
}

Simd4i operator<(const Simd4i& v0, const Simd4i& v1)
{
	return _mm_cmplt_epi32(v0, v1);
}

Simd4i operator>(const Simd4i& v0, const Simd4i& v1)
{
	return _mm_cmpgt_epi32(v0, v1);
}

ComplementExpr<Simd4i> operator~(const Simd4i& v)
{
	return ComplementExpr<Simd4i>(v);
}

Simd4i operator&(const Simd4i& v0, const Simd4i& v1)
{
	return _mm_and_si128(v0, v1);
}

Simd4i operator|(const Simd4i& v0, const Simd4i& v1)
{
	return _mm_or_si128(v0, v1);
}

Simd4i operator^(const Simd4i& v0, const Simd4i& v1)
{
	return _mm_xor_si128(v0, v1);
}

Simd4i operator<<(const Simd4i& v, int shift)
{
	return _mm_slli_epi32(v, shift);
}

Simd4i operator>>(const Simd4i& v, int shift)
{
	return _mm_srli_epi32(v, shift);
}

Simd4i operator+(const Simd4i& v)
{
	return v;
}

Simd4i operator+(const Simd4i& v0, const Simd4i& v1)
{
	return _mm_add_epi32(v0, v1);
}

Simd4i operator-(const Simd4i& v)
{
	return _mm_sub_epi32(_mm_setzero_si128(), v);
}

Simd4i operator-(const Simd4i& v0, const Simd4i& v1)
{
	return _mm_sub_epi32(v0, v1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// function implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Simd4i simd4i(const Simd4f& v)
{
	return _mm_castps_si128(v);
}

Simd4i truncate(const Simd4f& v)
{
	return _mm_cvttps_epi32(v);
}

int (&array(Simd4i& v))[4]
{
	return reinterpret_cast<int(&)[4]>(v);
}

const int (&array(const Simd4i& v))[4]
{
	return reinterpret_cast<const int(&)[4]>(v);
}

void store(int* ptr, const Simd4i& v)
{
	_mm_storeu_si128(reinterpret_cast<__m128i*>(ptr), v);
}

void storeAligned(int* ptr, const Simd4i& v)
{
	_mm_store_si128(reinterpret_cast<__m128i*>(ptr), v);
}

void storeAligned(int* ptr, unsigned int offset, const Simd4i& v)
{
	_mm_store_si128(reinterpret_cast<__m128i*>(reinterpret_cast<char*>(ptr) + offset), v);
}

template <size_t i>
Simd4i splat(const Simd4i& v)
{
	return _mm_shuffle_epi32(v, _MM_SHUFFLE(i, i, i, i));
}

Simd4i select(const Simd4i& mask, const Simd4i& v0, const Simd4i& v1)
{
	return _mm_xor_si128(v1, _mm_and_si128(mask, _mm_xor_si128(v1, v0)));
}

int allEqual(const Simd4i& v0, const Simd4i& v1)
{
	return allTrue(operator==(v0, v1));
}

int allEqual(const Simd4i& v0, const Simd4i& v1, Simd4i& outMask)
{
	return allTrue(outMask = operator==(v0, v1));
}

int anyEqual(const Simd4i& v0, const Simd4i& v1)
{
	return anyTrue(operator==(v0, v1));
}

int anyEqual(const Simd4i& v0, const Simd4i& v1, Simd4i& outMask)
{
	return anyTrue(outMask = operator==(v0, v1));
}

int allGreater(const Simd4i& v0, const Simd4i& v1)
{
	return allTrue(operator>(v0, v1));
}

int allGreater(const Simd4i& v0, const Simd4i& v1, Simd4i& outMask)
{
	return allTrue(outMask = operator>(v0, v1));
}

int anyGreater(const Simd4i& v0, const Simd4i& v1)
{
	return anyTrue(operator>(v0, v1));
}

int anyGreater(const Simd4i& v0, const Simd4i& v1, Simd4i& outMask)
{
	return anyTrue(outMask = operator>(v0, v1));
}

int allTrue(const Simd4i& v)
{
	return _mm_movemask_ps(_mm_castsi128_ps(v)) == 0xf;
}

int anyTrue(const Simd4i& v)
{
	return _mm_movemask_ps(_mm_castsi128_ps(v));
}

NV_SIMD_NAMESPACE_END
