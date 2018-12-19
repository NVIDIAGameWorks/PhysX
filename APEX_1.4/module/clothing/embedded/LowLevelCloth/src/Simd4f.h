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


// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#pragma once

#include "SimdTypes.h"

#if NVMATH_FUSE_MULTIPLY_ADD

/*! \brief Expression template to fuse multiply-adds.
 * \relates Simd4f */
struct ProductExpr
{
	inline ProductExpr(Simd4f const& v0_, Simd4f const& v1_) : v0(v0_), v1(v1_)
	{
	}
	inline operator Simd4f() const;
	const Simd4f v0, v1;

  private:
	ProductExpr& operator=(const ProductExpr&); // not implemented
};

inline Simd4f operator+(const ProductExpr&, const Simd4f&);
inline Simd4f operator+(const Simd4f& v, const ProductExpr&);
inline Simd4f operator+(const ProductExpr&, const ProductExpr&);
inline Simd4f operator-(const Simd4f& v, const ProductExpr&);
inline Simd4f operator-(const ProductExpr&, const ProductExpr&);

#else  // NVMATH_FUSE_MULTIPLY_ADD
typedef Simd4f ProductExpr;
#endif // NVMATH_FUSE_MULTIPLY_ADD

template <typename T>
struct Simd4fFactory
{
	Simd4fFactory(T v_) : v(v_)
	{
	}
	inline operator Simd4f() const;
	inline operator Scalar4f() const;
	Simd4fFactory& operator=(const Simd4fFactory&); // not implemented
	T v;
};

template <>
struct Simd4fFactory<detail::FourTuple>
{
	Simd4fFactory(float x, float y, float z, float w)
	{
		v[0] = x, v[1] = y, v[2] = z, v[3] = w;
	}
	Simd4fFactory(const Simd4fFactory<const float&>& f)
	{
		v[3] = v[2] = v[1] = v[0] = f.v;
	}
	inline operator Simd4f() const;
	inline operator Scalar4f() const;
	Simd4fFactory& operator=(const Simd4fFactory&); // not implemented
	PX_ALIGN(16, float) v[4];
};

template <int i>
struct Simd4fFactory<detail::IntType<i> >
{
	inline operator Simd4f() const;
	inline operator Scalar4f() const;
};

// forward declaration
template <typename>
struct Simd4iFactory;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// expression template
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if NVMATH_SIMD
inline Simd4f operator&(const ComplementExpr<Simd4f>&, const Simd4f&);
inline Simd4f operator&(const Simd4f&, const ComplementExpr<Simd4f>&);
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// operators
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// note: operator?= missing because they don't have corresponding intrinsics.

/*! \brief Test for equality of two vectors.
* \return Vector of per element result mask (all bits set for 'true', none set for 'false').
* \note QNaPs aren't handled on SPU: comparing two QNaPs will return true.
* \relates Simd4f */
inline Simd4f operator==(const Simd4f& v0, const Simd4f& v1);

// no operator!= because VMX128 does not support it, use ~operator== and handle QNaPs

/*! \brief Less-compare all elements of two vectors.
* \return Vector of per element result mask (all bits set for 'true', none set for 'false').
* \note QNaPs aren't handled on SPU: comparisons against QNaPs don't necessarily return false.
* \relates Simd4f */
inline Simd4f operator<(const Simd4f& v0, const Simd4f& v1);

/*! \brief Less-or-equal-compare all elements of two vectors.
* \return Vector of per element result mask (all bits set for 'true', none set for 'false').
* \note QNaPs aren't handled on SPU: comparisons against QNaPs don't necessarily return false.
* \relates Simd4f */
inline Simd4f operator<=(const Simd4f& v0, const Simd4f& v1);

/*! \brief Greater-compare all elements of two vectors.
* \return Vector of per element result mask (all bits set for 'true', none set for 'false').
* \note QNaPs aren't handled on SPU: comparisons against QNaPs don't necessarily return false.
* \relates Simd4f */
inline Simd4f operator>(const Simd4f& v0, const Simd4f& v1);

/*! \brief Greater-or-equal-compare all elements of two vectors.
* \return Vector of per element result mask (all bits set for 'true', none set for 'false').
* \note QNaPs aren't handled on SPU: comparisons against QNaPs don't necessarily return false.
* \relates Simd4f */
inline Simd4f operator>=(const Simd4f& v0, const Simd4f& v1);

/*! \brief Vector bit-wise NOT operator
* \return A vector holding the bit-negate of \a v.
* \relates Simd4f */
inline ComplementExpr<Simd4f> operator~(const Simd4f& v);

/*! \brief Vector bit-wise AND operator
* \return A vector holding the bit-wise AND of \a v0 and \a v1.
* \relates Simd4f */
inline Simd4f operator&(const Simd4f& v0, const Simd4f& v1);

/*! \brief Vector bit-wise OR operator
* \return A vector holding the bit-wise OR of \a v0 and \a v1.
* \relates Simd4f */
inline Simd4f operator|(const Simd4f& v0, const Simd4f& v1);

/*! \brief Vector bit-wise XOR operator
* \return A vector holding the bit-wise XOR of \a v0 and \a v1.
* \relates Simd4f */
inline Simd4f operator^(const Simd4f& v0, const Simd4f& v1);

/*! \brief Vector logical left shift.
* \return A vector with 4 elements of \a v0, each shifted left by \a shift bits.
* \relates Simd4f */
inline Simd4f operator<<(const Simd4f& v, int shift);

/*! \brief Vector logical right shift.
* \return A vector with 4 elements of \a v0, each shifted right by \a shift bits.
* \relates Simd4f */
inline Simd4f operator>>(const Simd4f& v, int shift);

#if NVMATH_SHIFT_BY_VECTOR
/*! \brief Vector logical left shift.
* \return A vector with 4 elements of \a v0, each shifted left by \a shift bits.
* \relates Simd4f */
inline Simd4f operator<<(const Simd4f& v, const Simd4f& shift);

/*! \brief Vector logical right shift.
* \return A vector with 4 elements of \a v0, each shifted right by \a shift bits.
* \relates Simd4f */
inline Simd4f operator>>(const Simd4f& v, const Simd4f& shift);
#endif

/*! \brief Unary vector addition operator.
* \return A vector holding the component-wise copy of \a v.
* \relates Simd4f */
inline Simd4f operator+(const Simd4f& v);

/*! \brief Vector addition operator
* \return A vector holding the component-wise sum of \a v0 and \a v1.
* \relates Simd4f */
inline Simd4f operator+(const Simd4f& v0, const Simd4f& v1);

/*! \brief Unary vector negation operator.
* \return A vector holding the component-wise negation of \a v.
* \relates Simd4f */
inline Simd4f operator-(const Simd4f& v);

/*! \brief Vector subtraction operator.
* \return A vector holding the component-wise difference of \a v0 and \a v1.
* \relates Simd4f */
inline Simd4f operator-(const Simd4f& v0, const Simd4f& v1);

/*! \brief Vector multiplication.
* \return Element-wise product of \a v0 and \a v1.
* \note For VMX, returns expression template to fuse multiply-add.
* \relates Simd4f */
inline ProductExpr operator*(const Simd4f& v0, const Simd4f& v1);

/*! \brief Vector division.
* \return Element-wise division of \a v0 and \a v1.
* \relates Simd4f */
inline Simd4f operator/(const Simd4f& v0, const Simd4f& v1);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// functions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*! \brief Load float value into all vector components.
* \relates Simd4f */
inline Simd4fFactory<const float&> simd4f(const float& s)
{
	return Simd4fFactory<const float&>(s);
}

/*! \brief Load 4 float values into vector.
* \relates Simd4f */
inline Simd4fFactory<detail::FourTuple> simd4f(float x, float y, float z, float w)
{
	return Simd4fFactory<detail::FourTuple>(x, y, z, w);
}

/*! \brief Create vector from literal.
* \return Vector with all elements set to i.
* \relates Simd4f */
template <int i>
inline Simd4fFactory<detail::IntType<i> > simd4f(detail::IntType<i> const&)
{
	return Simd4fFactory<detail::IntType<i> >();
}

/*! \brief Reinterpret Simd4i as Simd4f.
* \return A copy of \a v, but cast as Simd4f.
* \relates Simd4f */
inline Simd4f simd4f(const Simd4i& v);

/*! \brief Reinterpret Simd4iFactory as Simd4fFactory.
* \relates Simd4f */
template <typename T>
inline Simd4fFactory<T> simd4f(const Simd4iFactory<T>& v)
{
	return reinterpret_cast<const Simd4fFactory<T>&>(v);
}

/*! \brief return reference to contiguous array of vector elements
* \relates Simd4f */
inline float (&array(Simd4f& v))[4];

/*! \brief return constant reference to contiguous array of vector elements
* \relates Simd4f */
inline const float (&array(const Simd4f& v))[4];

/*! \brief Create vector from float array.
* \relates Simd4f */
inline Simd4fFactory<const float*> load(const float* ptr)
{
	return ptr;
}

/*! \brief Create vector from aligned float array.
* \note \a ptr needs to be 16 byte aligned.
* \relates Simd4f */
inline Simd4fFactory<detail::AlignedPointer<float> > loadAligned(const float* ptr)
{
	return detail::AlignedPointer<float>(ptr);
}

/*! \brief Create vector from aligned float array.
* \param offset pointer offset in bytes.
* \note \a ptr+offset needs to be 16 byte aligned.
* \relates Simd4f */
inline Simd4fFactory<detail::OffsetPointer<float> > loadAligned(const float* ptr, unsigned int offset)
{
	return detail::OffsetPointer<float>(ptr, offset);
}

/*! \brief Store vector \a v to float array \a ptr.
* \relates Simd4f */
inline void store(float* ptr, Simd4f const& v);

/*! \brief Store vector \a v to aligned float array \a ptr.
* \note \a ptr needs to be 16 byte aligned.
* \relates Simd4f */
inline void storeAligned(float* ptr, Simd4f const& v);

/*! \brief Store vector \a v to aligned float array \a ptr.
* \param offset pointer offset in bytes.
* \note \a ptr+offset needs to be 16 byte aligned.
* \relates Simd4f */
inline void storeAligned(float* ptr, unsigned int offset, Simd4f const& v);

/*! \brief replicate i-th component into all vector components.
* \return Vector with all elements set to \a v[i].
* \relates Simd4f */
template <size_t i>
inline Simd4f splat(Simd4f const& v);

/*! \brief Select \a v0 or \a v1 based on \a mask.
* \return mask ? v0 : v1
* \relates Simd4f */
inline Simd4f select(Simd4f const& mask, Simd4f const& v0, Simd4f const& v1);

/*! \brief Per element absolute value.
* \return Vector with absolute values of \a v.
* \relates Simd4f */
inline Simd4f abs(const Simd4f& v);

/*! \brief Per element floor value.
* \note Result undefined for QNaN elements.
* \note On SSE and NEON, returns v-1 if v is negative integer value
* \relates Simd4f */
inline Simd4f floor(const Simd4f& v);

/*! \brief Per-component minimum of two vectors
* \note Result undefined for QNaN elements.
* \relates Simd4f */
inline Simd4f max(const Simd4f& v0, const Simd4f& v1);

/*! \brief Per-component minimum of two vectors
* \note Result undefined for QNaN elements.
* \relates Simd4f */
inline Simd4f min(const Simd4f& v0, const Simd4f& v1);

/*! \brief Return reciprocal estimate of a vector.
* \return Vector of per-element reciprocal estimate.
* \relates Simd4f */
inline Simd4f recip(const Simd4f& v);

/*! \brief Return reciprocal of a vector.
* \return Vector of per-element reciprocal.
* \note Performs \a n Newton-Raphson iterations on initial estimate.
* \relates Simd4f */
template <int n>
inline Simd4f recipT(const Simd4f& v);

/*! \brief Return square root of a vector.
* \return Vector of per-element square root.
* \note The behavior is undefined for negative elements.
* \relates Simd4f */
inline Simd4f sqrt(const Simd4f& v);

/*! \brief Return inverse square root estimate of a vector.
* \return Vector of per-element inverse square root estimate.
* \note The behavior is undefined for negative, zero, and infinity elements.
* \relates Simd4f */
inline Simd4f rsqrt(const Simd4f& v);

/*! \brief Return inverse square root of a vector.
* \return Vector of per-element inverse square root.
* \note Performs \a n Newton-Raphson iterations on initial estimate.
* \note The behavior is undefined for negative and infinity elements.
* \relates Simd4f */
template <int n>
inline Simd4f rsqrtT(const Simd4f& v);

/*! \brief Return 2 raised to the power of v.
* \note Result undefined for QNaN elements.
* \relates Simd4f */
inline Simd4f exp2(const Simd4f& v);

#if NVMATH_SIMD
namespace simdf
{
// PSP2 is confused resolving about exp2, forwarding works
inline Simd4f exp2(const Simd4f& v)
{
	return ::exp2(v);
}
}
#endif

/*! \brief Return logarithm of v to base 2.
* \note Result undefined for QNaN elements.
* \relates Simd4f */
inline Simd4f log2(const Simd4f& v);

/*! \brief Return dot product of two 3-vectors.
* \note The result is replicated across all 4 components.
* \relates Simd4f */
inline Simd4f dot3(const Simd4f& v0, const Simd4f& v1);

/*! \brief Return cross product of two 3-vectors.
* \note The 4th component is undefined.
* \relates Simd4f */
inline Simd4f cross3(const Simd4f& v0, const Simd4f& v1);

/*! \brief Transposes 4x4 matrix represented by \a x, \a y, \a z, and \a w.
* \relates Simd4f */
inline void transpose(Simd4f& x, Simd4f& y, Simd4f& z, Simd4f& w);

/*! \brief returns non-zero if all elements or \a v0 and \a v1 are equal
* \note QNaPs aren't handled on SPU: comparing two QNaPs will return true.
* \relates Simd4f */
inline int allEqual(const Simd4f& v0, const Simd4f& v1);

/*! \brief returns non-zero if all elements or \a v0 and \a v1 are equal
* \param outMask holds the result of \a v0 == \a v1.
* \note QNaPs aren't handled on SPU: comparing two QNaPs will return true.
* \relates Simd4f */
inline int allEqual(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask);

/*! \brief returns non-zero if any elements or \a v0 and \a v1 are equal
* \note QNaPs aren't handled on SPU: comparing two QNaPs will return true.
* \relates Simd4f */
inline int anyEqual(const Simd4f& v0, const Simd4f& v1);

/*! \brief returns non-zero if any elements or \a v0 and \a v1 are equal
* \param outMask holds the result of \a v0 == \a v1.
* \note QNaPs aren't handled on SPU: comparing two QNaPs will return true.
* \relates Simd4f */
inline int anyEqual(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask);

/*! \brief returns non-zero if all elements or \a v0 and \a v1 are greater
* \note QNaPs aren't handled on SPU: comparisons against QNaPs don't necessarily return false.
* \relates Simd4f */
inline int allGreater(const Simd4f& v0, const Simd4f& v1);

/*! \brief returns non-zero if all elements or \a v0 and \a v1 are greater
* \param outMask holds the result of \a v0 == \a v1.
* \note QNaPs aren't handled on SPU: comparisons against QNaPs don't necessarily return false.
* \relates Simd4f */
inline int allGreater(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask);

/*! \brief returns non-zero if any elements or \a v0 and \a v1 are greater
* \note QNaPs aren't handled on SPU: comparisons against QNaPs don't necessarily return false.
* \relates Simd4f */
inline int anyGreater(const Simd4f& v0, const Simd4f& v1);

/*! \brief returns non-zero if any elements or \a v0 and \a v1 are greater
* \param outMask holds the result of \a v0 == \a v1.
* \note QNaPs aren't handled on SPU: comparisons against QNaPs don't necessarily return false.
* \relates Simd4f */
inline int anyGreater(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask);

/*! \brief returns non-zero if all elements or \a v0 and \a v1 are greater or equal
* \note QNaPs aren't handled on SPU: comparisons against QNaPs don't necessarily return false.
* \relates Simd4f */
inline int allGreaterEqual(const Simd4f& v0, const Simd4f& v1);

/*! \brief returns non-zero if all elements or \a v0 and \a v1 are greater or equal
* \param outMask holds the result of \a v0 == \a v1.
* \note QNaPs aren't handled on SPU: comparisons against QNaPs don't necessarily return false.
* \relates Simd4f */
inline int allGreaterEqual(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask);

/*! \brief returns non-zero if any elements or \a v0 and \a v1 are greater or equal
* \note QNaPs aren't handled on SPU: comparisons against QNaPs don't necessarily return false.
* \relates Simd4f */
inline int anyGreaterEqual(const Simd4f& v0, const Simd4f& v1);

/*! \brief returns non-zero if any elements or \a v0 and \a v1 are greater or equal
* \param outMask holds the result of \a v0 == \a v1.
* \note QNaPs aren't handled on SPU: comparisons against QNaPs don't necessarily return false.
* \relates Simd4f */
inline int anyGreaterEqual(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask);

/*! \brief returns non-zero if all elements are true
* \note Undefined if parameter is not result of a comparison.
* \relates Simd4f */
inline int allTrue(const Simd4f& v);

/*! \brief returns non-zero if any element is true
* \note Undefined if parameter is not result of a comparison.
* \relates Simd4f */
inline int anyTrue(const Simd4f& v);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// platform specific includes
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if NVMATH_SSE2
#include "sse2/Simd4f.h"
#elif NVMATH_NEON
#include "neon/Simd4f.h"
#endif

#if NVMATH_SCALAR
#include "scalar/Simd4f.h"
#endif
