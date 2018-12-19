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

template <typename T>
struct Simd4iFactory
{
	Simd4iFactory(T v_) : v(v_)
	{
	}
	inline operator Simd4i() const;
	inline operator Scalar4i() const;
	Simd4iFactory& operator=(const Simd4iFactory&); // not implemented
	T v;
};

template <>
struct Simd4iFactory<detail::FourTuple>
{
	Simd4iFactory(int x, int y, int z, int w)
	{
		v[0] = x, v[1] = y, v[2] = z, v[3] = w;
	}
	Simd4iFactory(const Simd4iFactory<const int&>& f)
	{
		v[3] = v[2] = v[1] = v[0] = f.v;
	}
	inline operator Simd4i() const;
	inline operator Scalar4i() const;
	Simd4iFactory& operator=(const Simd4iFactory&); // not implemented
	PX_ALIGN(16, int) v[4];
};

template <int i>
struct Simd4iFactory<detail::IntType<i> >
{
	inline operator Simd4i() const;
	inline operator Scalar4i() const;
};

// forward declaration
template <typename>
struct Simd4fFactory;

// map Simd4f/Scalar4f to Simd4i/Scalar4i
template <typename>
struct Simd4fToSimd4i;
template <>
struct Simd4fToSimd4i<Simd4f>
{
	typedef Simd4i Type;
};
template <>
struct Simd4fToSimd4i<Scalar4f>
{
	typedef Scalar4i Type;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// expression template
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if NVMATH_DISTINCT_TYPES
inline Simd4i operator&(const ComplementExpr<Simd4i>&, const Simd4i&);
inline Simd4i operator&(const Simd4i&, const ComplementExpr<Simd4i>&);
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// operators
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if NVMATH_DISTINCT_TYPES

/*! \brief Vector bit-wise NOT operator
* \return A vector holding the bit-negate of \a v.
* \relates Simd4i */
inline ComplementExpr<Simd4i> operator~(const Simd4i& v);

/*! \brief Vector bit-wise AND operator
* \return A vector holding the bit-wise AND of \a v0 and \a v1.
* \relates Simd4i */
inline Simd4i operator&(const Simd4i& v0, const Simd4i& v1);

/*! \brief Vector bit-wise OR operator
* \return A vector holding the bit-wise OR of \a v0 and \a v1.
* \relates Simd4i */
inline Simd4i operator|(const Simd4i& v0, const Simd4i& v1);

/*! \brief Vector bit-wise XOR operator
* \return A vector holding the bit-wise XOR of \a v0 and \a v1.
* \relates Simd4i */
inline Simd4i operator^(const Simd4i& v0, const Simd4i& v1);

/*! \brief Vector logical left shift.
* \return A vector with 4 elements of \a v0, each shifted left by \a shift bits.
* \relates Simd4i */
inline Simd4i operator<<(const Simd4i& v, int shift);

/*! \brief Vector logical right shift.
* \return A vector with 4 elements of \a v0, each shifted right by \a shift bits.
* \relates Simd4i */
inline Simd4i operator>>(const Simd4i& v, int shift);

#if NVMATH_SHIFT_BY_VECTOR

/*! \brief Vector logical left shift.
* \return A vector with 4 elements of \a v0, each shifted left by \a shift bits.
* \relates Simd4i */
inline Simd4i operator<<(const Simd4i& v, const Simd4i& shift);

/*! \brief Vector logical right shift.
* \return A vector with 4 elements of \a v0, each shifted right by \a shift bits.
* \relates Simd4i */
inline Simd4i operator>>(const Simd4i& v, const Simd4i& shift);

#endif // NVMATH_SHIFT_BY_VECTOR

#endif // NVMATH_DISTINCT_TYPES

namespace simdi // disambiguate for VMX
{
// note: operator?= missing because they don't have corresponding intrinsics.

/*! \brief Test for equality of two vectors.
* \return Vector of per element result mask (all bits set for 'true', none set for 'false').
* \relates Simd4i */
inline Simd4i operator==(const Simd4i& v0, const Simd4i& v1);

// no !=, <=, >= because VMX128/SSE don't support it, use ~equal etc.

/*! \brief Less-compare all elements of two *signed* vectors.
* \return Vector of per element result mask (all bits set for 'true', none set for 'false').
* \relates Simd4i */
inline Simd4i operator<(const Simd4i& v0, const Simd4i& v1);

/*! \brief Greater-compare all elements of two *signed* vectors.
* \return Vector of per element result mask (all bits set for 'true', none set for 'false').
* \relates Simd4i */
inline Simd4i operator>(const Simd4i& v0, const Simd4i& v1);

/*! \brief Vector addition operator
* \return A vector holding the component-wise sum of \a v0 and \a v1.
* \relates Simd4i */
inline Simd4i operator+(const Simd4i& v0, const Simd4i& v1);

/*! \brief Unary vector negation operator.
* \return A vector holding the component-wise negation of \a v.
* \relates Simd4i */
inline Simd4i operator-(const Simd4i& v);

/*! \brief Vector subtraction operator.
* \return A vector holding the component-wise difference of \a v0 and \a v1.
* \relates Simd4i */
inline Simd4i operator-(const Simd4i& v0, const Simd4i& v1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// functions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*! \brief Load int value into all vector components.
* \relates Simd4i */
inline Simd4iFactory<const int&> simd4i(const int& s)
{
	return Simd4iFactory<const int&>(s);
}

/*! \brief Load 4 int values into vector.
* \relates Simd4i */
inline Simd4iFactory<detail::FourTuple> simd4i(int x, int y, int z, int w)
{
	return Simd4iFactory<detail::FourTuple>(x, y, z, w);
}

/*! \brief Create vector from literal.
* \return Vector with all elements set to \c i.
* \relates Simd4i */
template <int i>
inline Simd4iFactory<detail::IntType<i> > simd4i(const detail::IntType<i>&)
{
	return Simd4iFactory<detail::IntType<i> >();
}

template <>
inline Simd4iFactory<detail::IntType<1> > simd4i(const detail::IntType<1>&)
{
	return Simd4iFactory<detail::IntType<1> >();
}

template <>
inline Simd4iFactory<detail::IntType<int(0x80000000)> > simd4i(const detail::IntType<int(0x80000000)>&)
{
	return Simd4iFactory<detail::IntType<int(0x80000000)> >();
}

template <>
inline Simd4iFactory<detail::IntType<-1> > simd4i(const detail::IntType<-1>&)
{
	return Simd4iFactory<detail::IntType<-1> >();
}

/*! \brief Reinterpret Simd4f as Simd4i.
* \return A copy of \a v, but cast as Simd4i.
* \relates Simd4i */
inline Simd4i simd4i(const Simd4f& v);

/*! \brief Reinterpret Simd4fFactory as Simd4iFactory.
* \relates Simd4i */
template <typename T>
inline Simd4iFactory<T> simd4i(const Simd4fFactory<T>& v)
{
	return reinterpret_cast<const Simd4iFactory<T>&>(v);
}

namespace simdi
{

/*! \brief return reference to contiguous array of vector elements
* \relates Simd4i */
inline int (&array(Simd4i& v))[4];

/*! \brief return constant reference to contiguous array of vector elements
* \relates Simd4i */
inline const int (&array(const Simd4i& v))[4];

} // namespace simdi

/*! \brief Create vector from int array.
* \relates Simd4i */
inline Simd4iFactory<const int*> load(const int* ptr)
{
	return ptr;
}

/*! \brief Create vector from aligned int array.
* \note \a ptr needs to be 16 byte aligned.
* \relates Simd4i */
inline Simd4iFactory<detail::AlignedPointer<int> > loadAligned(const int* ptr)
{
	return detail::AlignedPointer<int>(ptr);
}

/*! \brief Create vector from aligned float array.
* \param offset pointer offset in bytes.
* \note \a ptr+offset needs to be 16 byte aligned.
* \relates Simd4i */
inline Simd4iFactory<detail::OffsetPointer<int> > loadAligned(const int* ptr, unsigned int offset)
{
	return detail::OffsetPointer<int>(ptr, offset);
}

/*! \brief Store vector \a v to int array \a ptr.
* \relates Simd4i */
inline void store(int* ptr, const Simd4i& v);

/*! \brief Store vector \a v to aligned int array \a ptr.
* \note \a ptr needs to be 16 byte aligned.
* \relates Simd4i */
inline void storeAligned(int* ptr, const Simd4i& v);

/*! \brief Store vector \a v to aligned int array \a ptr.
* \param offset pointer offset in bytes.
* \note \a ptr+offset needs to be 16 byte aligned.
* \relates Simd4i */
inline void storeAligned(int* ptr, unsigned int offset, const Simd4i& v);

#if NVMATH_DISTINCT_TYPES

/*! \brief replicate i-th component into all vector components.
* \return Vector with all elements set to \a v[i].
* \relates Simd4i */
template <size_t i>
inline Simd4i splat(const Simd4i& v);

/*! \brief Select \a v0 or \a v1 based on \a mask.
* \return mask ? v0 : v1
* \relates Simd4i */
inline Simd4i select(const Simd4i& mask, const Simd4i& v0, const Simd4i& v1);

#endif // NVMATH_DISTINCT_TYPES

namespace simdi // disambiguate for VMX
{
/*! \brief returns non-zero if all elements or \a v0 and \a v1 are equal
* \relates Simd4i */
inline int allEqual(const Simd4i& v0, const Simd4i& v1);

/*! \brief returns non-zero if all elements or \a v0 and \a v1 are equal
* \param outMask holds the result of \a v0 == \a v1.
* \relates Simd4i */
inline int allEqual(const Simd4i& v0, const Simd4i& v1, Simd4i& outMask);

/*! \brief returns non-zero if any elements or \a v0 and \a v1 are equal
* \relates Simd4i */
inline int anyEqual(const Simd4i& v0, const Simd4i& v1);

/*! \brief returns non-zero if any elements or \a v0 and \a v1 are equal
* \param outMask holds the result of \a v0 == \a v1.
* \relates Simd4i */
inline int anyEqual(const Simd4i& v0, const Simd4i& v1, Simd4i& outMask);

/*! \brief returns non-zero if all *signed* elements or \a v0 and \a v1 are greater
* \relates Simd4i */
inline int allGreater(const Simd4i& v0, const Simd4i& v1);

/*! \brief returns non-zero if all *signed* elements or \a v0 and \a v1 are greater
* \param outMask holds the result of \a v0 == \a v1.
* \relates Simd4i */
inline int allGreater(const Simd4i& v0, const Simd4i& v1, Simd4i& outMask);

/*! \brief returns non-zero if any elements or \a v0 and \a v1 are greater
* \relates Simd4i */
inline int anyGreater(const Simd4i& v0, const Simd4i& v1);

/*! \brief returns non-zero if any elements or \a v0 and \a v1 are greater
* \param outMask holds the result of \a v0 == \a v1.
* \relates Simd4i */
inline int anyGreater(const Simd4i& v0, const Simd4i& v1, Simd4i& outMask);
}

#if NVMATH_DISTINCT_TYPES

/*! \brief returns non-zero if all elements are true
* \note undefined if parameter is not result of a comparison.
* \relates Simd4i */
inline int allTrue(const Simd4i& v);

/*! \brief returns non-zero if any element is true
* \note undefined if parameter is not result of a comparison.
* \relates Simd4i */
inline int anyTrue(const Simd4i& v);

#endif // NVMATH_DISTINCT_TYPES

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// platform specific includes
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if NVMATH_SSE2
#include "sse2/Simd4i.h"
#elif NVMATH_NEON
#include "neon/Simd4i.h"
#endif

#if NVMATH_SCALAR
#include "scalar/Simd4i.h"
#endif
