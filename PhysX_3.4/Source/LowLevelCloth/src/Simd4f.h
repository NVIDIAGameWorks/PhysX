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

#include "SimdTypes.h"
#include <float.h>
#include <math.h>

NV_SIMD_NAMESPACE_BEGIN

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// factories
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*! \brief Creates Simd4f with all components set to zero.
* \relates Simd4f */
struct Simd4fZeroFactory
{
	inline operator Simd4f() const;
	inline operator Scalar4f() const;
};

/*! \brief Creates Simd4f with all components set to one.
* \relates Simd4f */
struct Simd4fOneFactory
{
	inline operator Simd4f() const;
	inline operator Scalar4f() const;
};

/*! \brief Replicates float into all four Simd4f components.
* \relates Simd4f */
struct Simd4fScalarFactory
{
	explicit Simd4fScalarFactory(const float& s) : value(s)
	{
	}
	Simd4fScalarFactory& operator=(const Simd4fScalarFactory&); // not implemented
	inline operator Simd4f() const;
	inline operator Scalar4f() const;

	const float value;
};

/*! \brief Creates Simd4f from four floats.
* \relates Simd4f */
struct Simd4fTupleFactory
{
	Simd4fTupleFactory(float x, float y, float z, float w)
	// c++11: : tuple{ x, y, z, w }
	{
		tuple[0] = x;
		tuple[1] = y;
		tuple[2] = z;
		tuple[3] = w;
	}
	Simd4fTupleFactory(unsigned x, unsigned y, unsigned z, unsigned w)
	{
		unsigned* ptr = reinterpret_cast<unsigned*>(tuple);
		ptr[0] = x;
		ptr[1] = y;
		ptr[2] = z;
		ptr[3] = w;
	}
	Simd4fTupleFactory& operator=(const Simd4fTupleFactory&); // not implemented
	inline operator Simd4f() const;
	inline operator Scalar4f() const;
	NV_SIMD_ALIGN(16, float) tuple[4];
};

/*! \brief Loads Simd4f from (unaligned) pointer.
* \relates Simd4f */
struct Simd4fLoadFactory
{
	explicit Simd4fLoadFactory(const float* p) : ptr(p)
	{
	}
	Simd4fLoadFactory& operator=(const Simd4fLoadFactory&); // not implemented
	inline operator Simd4f() const;
	inline operator Scalar4f() const;
	const float* const ptr;
};

/*! \brief Loads Simd4f from (aligned) pointer.
* \relates Simd4f */
struct Simd4fAlignedLoadFactory
{
	explicit Simd4fAlignedLoadFactory(const float* p) : ptr(p)
	{
	}
	Simd4fAlignedLoadFactory& operator=(const Simd4fAlignedLoadFactory&); // not implemented
	inline operator Simd4f() const;
	inline operator Scalar4f() const;
	const float* const ptr;
};

/*! \brief Loads Simd4f from (unaligned) pointer.
* \relates Simd4f */
struct Simd4fLoad3Factory
{
	explicit Simd4fLoad3Factory(const float* p) : ptr(p)
	{
	}
	Simd4fLoad3Factory& operator=(const Simd4fLoad3Factory&); // not implemented
	inline operator Simd4f() const;
	inline operator Scalar4f() const;
	const float* const ptr;
};

/*! \brief Loads Simd4f from (unaligned) pointer, which point to 3 floats in memory. 4th component will be initialized
* with w
* \relates Simd4f */
struct Simd4fLoad3SetWFactory
{
	explicit Simd4fLoad3SetWFactory(const float* p, const float wComponent) : ptr(p), w(wComponent)
	{
	}
	Simd4fLoad3SetWFactory& operator=(const Simd4fLoad3SetWFactory&); // not implemented
	inline operator Simd4f() const;
	inline operator Scalar4f() const;
	const float* const ptr;
	const float w;
};

/*! \brief Loads Simd4f from (aligned) pointer with offset.
* \relates Simd4f */
struct Simd4fOffsetLoadFactory
{
	Simd4fOffsetLoadFactory(const float* p, unsigned int off) : ptr(p), offset(off)
	{
	}
	Simd4fOffsetLoadFactory& operator=(const Simd4fOffsetLoadFactory&); // not implemented
	inline operator Simd4f() const;
	inline operator Scalar4f() const;
	const float* const ptr;
	const unsigned int offset;
};

// forward declaration
struct Simd4iScalarFactory;
struct Simd4iTupleFactory;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// expression templates
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if NV_SIMD_FUSE_MULTIPLY_ADD
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
#else  // NV_SIMD_FUSE_MULTIPLY_ADD
typedef Simd4f ProductExpr;
#endif // NV_SIMD_FUSE_MULTIPLY_ADD

// multiply-add expression templates
inline Simd4f operator+(const ProductExpr&, const Simd4f&);
inline Simd4f operator+(const Simd4f&, const ProductExpr&);
inline Simd4f operator+(const ProductExpr&, const ProductExpr&);
inline Simd4f operator-(const Simd4f&, const ProductExpr&);
inline Simd4f operator-(const ProductExpr&, const ProductExpr&);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// operators
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// note: operator?= missing because they don't have corresponding intrinsics.

/*! \brief Test for equality of two vectors.
* \return Vector of per element result mask (all bits set for 'true', none set for 'false').
* \note QNaNs aren't handled on SPU: comparing two QNaNs will return true.
* \relates Simd4f */
inline Simd4f operator==(const Simd4f& v0, const Simd4f& v1);

// no operator!= because VMX128 does not support it, use ~operator== and handle QNaNs

/*! \brief Less-compare all elements of two vectors.
* \return Vector of per element result mask (all bits set for 'true', none set for 'false').
* \note QNaNs aren't handled on SPU: comparisons against QNaNs don't necessarily return false.
* \relates Simd4f */
inline Simd4f operator<(const Simd4f& v0, const Simd4f& v1);

/*! \brief Less-or-equal-compare all elements of two vectors.
* \return Vector of per element result mask (all bits set for 'true', none set for 'false').
* \note QNaNs aren't handled on SPU: comparisons against QNaNs don't necessarily return false.
* \relates Simd4f */
inline Simd4f operator<=(const Simd4f& v0, const Simd4f& v1);

/*! \brief Greater-compare all elements of two vectors.
* \return Vector of per element result mask (all bits set for 'true', none set for 'false').
* \note QNaNs aren't handled on SPU: comparisons against QNaNs don't necessarily return false.
* \relates Simd4f */
inline Simd4f operator>(const Simd4f& v0, const Simd4f& v1);

/*! \brief Greater-or-equal-compare all elements of two vectors.
* \return Vector of per element result mask (all bits set for 'true', none set for 'false').
* \note QNaNs aren't handled on SPU: comparisons against QNaNs don't necessarily return false.
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

#if NV_SIMD_SHIFT_BY_VECTOR
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
inline Simd4fScalarFactory simd4f(const float& s)
{
	return Simd4fScalarFactory(s);
}

/*! \brief Load 4 float values into vector.
* \relates Simd4f */
inline Simd4fTupleFactory simd4f(float x, float y, float z, float w)
{
	return Simd4fTupleFactory(x, y, z, w);
}

/*! \brief Reinterpret Simd4i as Simd4f.
* \return A copy of \a v, but reinterpreted as Simd4f.
* \relates Simd4f */
inline Simd4f simd4f(const Simd4i& v);

/*! \brief Reinterpret Simd4iScalarFactory as Simd4fScalarFactory.
* \relates Simd4f */
inline Simd4fScalarFactory simd4f(const Simd4iScalarFactory& v)
{
	return reinterpret_cast<const Simd4fScalarFactory&>(v);
}

/*! \brief Reinterpret Simd4iTupleFactory as Simd4fTupleFactory.
* \relates Simd4f */
inline Simd4fTupleFactory simd4f(const Simd4iTupleFactory& v)
{
	return reinterpret_cast<const Simd4fTupleFactory&>(v);
}

/*! \brief Convert Simd4i to Simd4f.
* \relates Simd4f */
inline Simd4f convert(const Simd4i& v);

/*! \brief Return reference to contiguous array of vector elements
* \relates Simd4f */
inline float (&array(Simd4f& v))[4];

/*! \brief Return constant reference to contiguous array of vector elements
* \relates Simd4f */
inline const float (&array(const Simd4f& v))[4];

/*! \brief Create vector from float array.
* \relates Simd4f */
inline Simd4fLoadFactory load(const float* ptr)
{
	return Simd4fLoadFactory(ptr);
}

/*! \brief Create vector from aligned float array.
* \note \a ptr needs to be 16 byte aligned.
* \relates Simd4f */
inline Simd4fAlignedLoadFactory loadAligned(const float* ptr)
{
	return Simd4fAlignedLoadFactory(ptr);
}

/*! \brief Create vector from float[3] \a ptr array. 4th component of simd4f will be equal to 0.0
* \relates Simd4f */
inline Simd4fLoad3Factory load3(const float* ptr)
{
	return Simd4fLoad3Factory(ptr);
}

/*! \brief Create vector from float[3] \a ptr array and extra \a wComponent
* \relates Simd4f */
inline Simd4fLoad3SetWFactory load3(const float* ptr, const float wComponent)
{
	return Simd4fLoad3SetWFactory(ptr, wComponent);
}

/*! \brief Create vector from aligned float array.
* \param offset pointer offset in bytes.
* \note \a ptr+offset needs to be 16 byte aligned.
* \relates Simd4f */
inline Simd4fOffsetLoadFactory loadAligned(const float* ptr, unsigned int offset)
{
	return Simd4fOffsetLoadFactory(ptr, offset);
}

/*! \brief Store vector \a v to float array \a ptr.
* \relates Simd4f */
inline void store(float* ptr, Simd4f const& v);

/*! \brief Store vector \a v to float[3] array \a ptr.
* \relates Simd4f */
inline void store3(float* ptr, Simd4f const& v);

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
* \note Result undefined for values outside of the integer range.
* \note Translates to 6 instructions on SSE and NEON.
* \relates Simd4f */
inline Simd4f floor(const Simd4f& v);

#if !defined max
/*! \brief Per-component minimum of two vectors
* \note Result undefined for QNaN elements.
* \relates Simd4f */
inline Simd4f max(const Simd4f& v0, const Simd4f& v1);
#endif

#if !defined min
/*! \brief Per-component minimum of two vectors
* \note Result undefined for QNaN elements.
* \relates Simd4f */
inline Simd4f min(const Simd4f& v0, const Simd4f& v1);
#endif

/*! \brief Return reciprocal estimate of a vector.
* \return Vector of per-element reciprocal estimate.
* \relates Simd4f */
inline Simd4f recip(const Simd4f& v);

/*! \brief Return reciprocal of a vector.
* \return Vector of per-element reciprocal.
* \note Performs \a n Newton-Raphson iterations on initial estimate.
* \relates Simd4f */
template <int n>
inline Simd4f recip(const Simd4f& v);

/*! \brief Return square root of a vector.
* \return Vector of per-element square root.
* \note Result undefined for negative elements.
* \relates Simd4f */
inline Simd4f sqrt(const Simd4f& v);

/*! \brief Return inverse square root estimate of a vector.
* \return Vector of per-element inverse square root estimate.
* \note Result undefined for negative, zero, and infinity elements.
* \relates Simd4f */
inline Simd4f rsqrt(const Simd4f& v);

/*! \brief Return inverse square root of a vector.
* \return Vector of per-element inverse square root.
* \note Performs \a n Newton-Raphson iterations on initial estimate.
* \note The result is undefined for negative and infinity elements.
* \relates Simd4f */
template <int n>
inline Simd4f rsqrt(const Simd4f& v);

/*! \brief Return 2 raised to the power of v.
* \note Result only defined for finite elements.
* \relates Simd4f */
inline Simd4f exp2(const Simd4f& v);

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
* \note Result only defined for finite x, y, and z values.
* \relates Simd4f */
inline Simd4f cross3(const Simd4f& v0, const Simd4f& v1);

/*! \brief Transposes 4x4 matrix represented by \a x, \a y, \a z, and \a w.
* \relates Simd4f */
inline void transpose(Simd4f& x, Simd4f& y, Simd4f& z, Simd4f& w);

/*! \brief Interleave elements.
* \a v0 becomes {x0, x1, y0, y1}, v1 becomes {z0, z1, w0, w1}.
* \relates Simd4f */
inline void zip(Simd4f& v0, Simd4f& v1);

/*! \brief De-interleave elements.
* \a v0 becomes {x0, z0, x1, z1}, v1 becomes {y0, w0, y1, w1}.
* \relates Simd4f */
inline void unzip(Simd4f& v0, Simd4f& v1);

/*! \brief Swaps quad words.
* Returns {z0, w0, x0, y0}
* \relates Simd4f */
inline Simd4f swaphilo(const Simd4f& v);

/*! \brief returns non-zero if all elements or \a v0 and \a v1 are equal
* \note QNaNs aren't handled on SPU: comparing two QNaNs will return true.
* \relates Simd4f */
inline int allEqual(const Simd4f& v0, const Simd4f& v1);

/*! \brief returns non-zero if all elements or \a v0 and \a v1 are equal
* \param outMask holds the result of \a v0 == \a v1.
* \note QNaNs aren't handled on SPU: comparing two QNaNs will return true.
* \relates Simd4f */
inline int allEqual(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask);

/*! \brief returns non-zero if any elements or \a v0 and \a v1 are equal
* \note QNaNs aren't handled on SPU: comparing two QNaNs will return true.
* \relates Simd4f */
inline int anyEqual(const Simd4f& v0, const Simd4f& v1);

/*! \brief returns non-zero if any elements or \a v0 and \a v1 are equal
* \param outMask holds the result of \a v0 == \a v1.
* \note QNaNs aren't handled on SPU: comparing two QNaNs will return true.
* \relates Simd4f */
inline int anyEqual(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask);

/*! \brief returns non-zero if all elements or \a v0 and \a v1 are greater
* \note QNaNs aren't handled on SPU: comparisons against QNaNs don't necessarily return false.
* \relates Simd4f */
inline int allGreater(const Simd4f& v0, const Simd4f& v1);

/*! \brief returns non-zero if all elements or \a v0 and \a v1 are greater
* \param outMask holds the result of \a v0 == \a v1.
* \note QNaNs aren't handled on SPU: comparisons against QNaNs don't necessarily return false.
* \relates Simd4f */
inline int allGreater(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask);

/*! \brief returns non-zero if any elements or \a v0 and \a v1 are greater
* \note QNaNs aren't handled on SPU: comparisons against QNaNs don't necessarily return false.
* \relates Simd4f */
inline int anyGreater(const Simd4f& v0, const Simd4f& v1);

/*! \brief returns non-zero if any elements or \a v0 and \a v1 are greater
* \param outMask holds the result of \a v0 == \a v1.
* \note QNaNs aren't handled on SPU: comparisons against QNaNs don't necessarily return false.
* \relates Simd4f */
inline int anyGreater(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask);

/*! \brief returns non-zero if all elements or \a v0 and \a v1 are greater or equal
* \note QNaNs aren't handled on SPU: comparisons against QNaNs don't necessarily return false.
* \relates Simd4f */
inline int allGreaterEqual(const Simd4f& v0, const Simd4f& v1);

/*! \brief returns non-zero if all elements or \a v0 and \a v1 are greater or equal
* \param outMask holds the result of \a v0 == \a v1.
* \note QNaNs aren't handled on SPU: comparisons against QNaNs don't necessarily return false.
* \relates Simd4f */
inline int allGreaterEqual(const Simd4f& v0, const Simd4f& v1, Simd4f& outMask);

/*! \brief returns non-zero if any elements or \a v0 and \a v1 are greater or equal
* \note QNaNs aren't handled on SPU: comparisons against QNaNs don't necessarily return false.
* \relates Simd4f */
inline int anyGreaterEqual(const Simd4f& v0, const Simd4f& v1);

/*! \brief returns non-zero if any elements or \a v0 and \a v1 are greater or equal
* \param outMask holds the result of \a v0 == \a v1.
* \note QNaNs aren't handled on SPU: comparisons against QNaNs don't necessarily return false.
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
// constants
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

NV_SIMD_GLOBAL_CONSTANT Simd4fZeroFactory gSimd4fZero = Simd4fZeroFactory();
NV_SIMD_GLOBAL_CONSTANT Simd4fOneFactory gSimd4fOne = Simd4fOneFactory();
NV_SIMD_GLOBAL_CONSTANT Simd4fScalarFactory gSimd4fMinusOne = simd4f(-1.0f);
NV_SIMD_GLOBAL_CONSTANT Simd4fScalarFactory gSimd4fHalf = simd4f(0.5f);
NV_SIMD_GLOBAL_CONSTANT Simd4fScalarFactory gSimd4fTwo = simd4f(2.0f);
NV_SIMD_GLOBAL_CONSTANT Simd4fScalarFactory gSimd4fPi = simd4f(3.14159265358979323846f);
NV_SIMD_GLOBAL_CONSTANT Simd4fScalarFactory gSimd4fEpsilon = simd4f(FLT_EPSILON);
NV_SIMD_GLOBAL_CONSTANT Simd4fScalarFactory gSimd4fFloatMax = simd4f(FLT_MAX);
NV_SIMD_GLOBAL_CONSTANT Simd4fTupleFactory gSimd4fMaskX = Simd4fTupleFactory(~0u, 0u, 0u, 0u);
NV_SIMD_GLOBAL_CONSTANT Simd4fTupleFactory gSimd4fMaskXYZ = Simd4fTupleFactory(~0u, ~0u, ~0u, 0u);

NV_SIMD_NAMESPACE_END

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// platform specific includes
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if NV_SIMD_SSE2
#include "sse2/Simd4f.h"
#elif NV_SIMD_NEON
#include "neon/Simd4f.h"
#endif

#if NV_SIMD_SCALAR
#include "scalar/Simd4f.h"
#endif
