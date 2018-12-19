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

Simd4iZeroFactory::operator Scalar4i() const
{
	return Scalar4i(0, 0, 0, 0);
}

Simd4iScalarFactory::operator Scalar4i() const
{
	return Scalar4i(value, value, value, value);
}

Simd4iTupleFactory::operator Scalar4i() const
{
	return reinterpret_cast<const Scalar4i&>(tuple);
}

Simd4iLoadFactory::operator Scalar4i() const
{
	return Scalar4i(ptr[0], ptr[1], ptr[2], ptr[3]);
}

Simd4iAlignedLoadFactory::operator Scalar4i() const
{
	return Scalar4i(ptr[0], ptr[1], ptr[2], ptr[3]);
}

Simd4iOffsetLoadFactory::operator Scalar4i() const
{
	return Simd4iAlignedLoadFactory(reinterpret_cast<const int*>(reinterpret_cast<const char*>(ptr) + offset));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// expression template
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <>
inline ComplementExpr<Scalar4i>::operator Scalar4i() const
{
	return Scalar4i(~v.u4[0], ~v.u4[1], ~v.u4[2], ~v.u4[3]);
}

template <>
inline Scalar4i operator&(const ComplementExpr<Scalar4i>& complement, const Scalar4i& v)
{
	return Scalar4i(v.u4[0] & ~complement.v.u4[0], v.u4[1] & ~complement.v.u4[1], v.u4[2] & ~complement.v.u4[2],
	                v.u4[3] & ~complement.v.u4[3]);
}

template <>
inline Scalar4i operator&(const Scalar4i& v, const ComplementExpr<Scalar4i>& complement)
{
	return Scalar4i(v.u4[0] & ~complement.v.u4[0], v.u4[1] & ~complement.v.u4[1], v.u4[2] & ~complement.v.u4[2],
	                v.u4[3] & ~complement.v.u4[3]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// operator implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline ComplementExpr<Scalar4i> operator~(const Scalar4i& v)
{
	return ComplementExpr<Scalar4i>(v);
}

inline Scalar4i operator&(const Scalar4i& v0, const Scalar4i& v1)
{
	return Scalar4i(v0.u4[0] & v1.u4[0], v0.u4[1] & v1.u4[1], v0.u4[2] & v1.u4[2], v0.u4[3] & v1.u4[3]);
}

inline Scalar4i operator|(const Scalar4i& v0, const Scalar4i& v1)
{
	return Scalar4i(v0.u4[0] | v1.u4[0], v0.u4[1] | v1.u4[1], v0.u4[2] | v1.u4[2], v0.u4[3] | v1.u4[3]);
}

inline Scalar4i operator^(const Scalar4i& v0, const Scalar4i& v1)
{
	return Scalar4i(v0.u4[0] ^ v1.u4[0], v0.u4[1] ^ v1.u4[1], v0.u4[2] ^ v1.u4[2], v0.u4[3] ^ v1.u4[3]);
}

inline Scalar4i operator<<(const Scalar4i& v, int shift)
{
	return Scalar4i(v.u4[0] << shift, v.u4[1] << shift, v.u4[2] << shift, v.u4[3] << shift);
}

inline Scalar4i operator>>(const Scalar4i& v, int shift)
{
	return Scalar4i(v.u4[0] >> shift, v.u4[1] >> shift, v.u4[2] >> shift, v.u4[3] >> shift);
}

inline Scalar4i operator==(const Scalar4i& v0, const Scalar4i& v1)
{
	return Scalar4i(v0.i4[0] == v1.i4[0], v0.i4[1] == v1.i4[1], v0.i4[2] == v1.i4[2], v0.i4[3] == v1.i4[3]);
}

inline Scalar4i operator<(const Scalar4i& v0, const Scalar4i& v1)
{
	return Scalar4i(v0.i4[0] < v1.i4[0], v0.i4[1] < v1.i4[1], v0.i4[2] < v1.i4[2], v0.i4[3] < v1.i4[3]);
}

inline Scalar4i operator>(const Scalar4i& v0, const Scalar4i& v1)
{
	return Scalar4i(v0.i4[0] > v1.i4[0], v0.i4[1] > v1.i4[1], v0.i4[2] > v1.i4[2], v0.i4[3] > v1.i4[3]);
}

inline Scalar4i operator+(const Scalar4i& v)
{
	return v;
}

inline Scalar4i operator+(const Scalar4i& v0, const Scalar4i& v1)
{
	return Scalar4i(v0.i4[0] + v1.i4[0], v0.i4[1] + v1.i4[1], v0.i4[2] + v1.i4[2], v0.i4[3] + v1.i4[3]);
}

inline Scalar4i operator-(const Scalar4i& v)
{
	return Scalar4i(-v.i4[0], -v.i4[1], -v.i4[2], -v.i4[3]);
}

inline Scalar4i operator-(const Scalar4i& v0, const Scalar4i& v1)
{
	return Scalar4i(v0.i4[0] - v1.i4[0], v0.i4[1] - v1.i4[1], v0.i4[2] - v1.i4[2], v0.i4[3] - v1.i4[3]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// function implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline Scalar4i simd4i(const Scalar4f& v)
{
	return reinterpret_cast<const Scalar4i&>(v);
}

inline Scalar4i truncate(const Scalar4f& v)
{
	return Scalar4i(int(v.f4[0]), int(v.f4[1]), int(v.f4[2]), int(v.f4[3]));
}

inline int (&array(Scalar4i& v))[4]
{
	return v.i4;
}

inline const int (&array(const Scalar4i& v))[4]
{
	return v.i4;
}

inline void store(int* ptr, const Scalar4i& v)
{
	ptr[0] = v.i4[0];
	ptr[1] = v.i4[1];
	ptr[2] = v.i4[2];
	ptr[3] = v.i4[3];
}

inline void storeAligned(int* ptr, const Scalar4i& v)
{
	store(ptr, v);
}

inline void storeAligned(int* ptr, unsigned int offset, const Scalar4i& v)
{
	store(reinterpret_cast<int*>(reinterpret_cast<char*>(ptr) + offset), v);
}

template <size_t i>
inline Scalar4i splat(const Scalar4i& v)
{
	return Scalar4i(v.u4[i], v.u4[i], v.u4[i], v.u4[i]);
}

inline Scalar4i select(const Scalar4i& mask, const Scalar4i& v0, const Scalar4i& v1)
{
	return ((v0 ^ v1) & mask) ^ v1;
}

inline int allEqual(const Scalar4i& v0, const Scalar4i& v1)
{
	return v0.i4[0] == v1.i4[0] && v0.i4[1] == v1.i4[1] && v0.i4[2] == v1.i4[2] && v0.i4[3] == v1.i4[3];
}

inline int allEqual(const Scalar4i& v0, const Scalar4i& v1, Scalar4i& outMask)
{
	bool b0 = v0.i4[0] == v1.i4[0], b1 = v0.i4[1] == v1.i4[1], b2 = v0.i4[2] == v1.i4[2], b3 = v0.i4[3] == v1.i4[3];
	outMask = Scalar4i(b0, b1, b2, b3);
	return b0 && b1 && b2 && b3;
}

inline int anyEqual(const Scalar4i& v0, const Scalar4i& v1)
{
	return v0.i4[0] == v1.i4[0] || v0.i4[1] == v1.i4[1] || v0.i4[2] == v1.i4[2] || v0.i4[3] == v1.i4[3];
}

inline int anyEqual(const Scalar4i& v0, const Scalar4i& v1, Scalar4i& outMask)
{
	bool b0 = v0.i4[0] == v1.i4[0], b1 = v0.i4[1] == v1.i4[1], b2 = v0.i4[2] == v1.i4[2], b3 = v0.i4[3] == v1.i4[3];
	outMask = Scalar4i(b0, b1, b2, b3);
	return b0 || b1 || b2 || b3;
}

inline int allGreater(const Scalar4i& v0, const Scalar4i& v1)
{
	return v0.i4[0] > v1.i4[0] && v0.i4[1] > v1.i4[1] && v0.i4[2] > v1.i4[2] && v0.i4[3] > v1.i4[3];
}

inline int allGreater(const Scalar4i& v0, const Scalar4i& v1, Scalar4i& outMask)
{
	bool b0 = v0.i4[0] > v1.i4[0], b1 = v0.i4[1] > v1.i4[1], b2 = v0.i4[2] > v1.i4[2], b3 = v0.i4[3] > v1.i4[3];
	outMask = Scalar4i(b0, b1, b2, b3);
	return b0 && b1 && b2 && b3;
}

inline int anyGreater(const Scalar4i& v0, const Scalar4i& v1)
{
	return v0.i4[0] > v1.i4[0] || v0.i4[1] > v1.i4[1] || v0.i4[2] > v1.i4[2] || v0.i4[3] > v1.i4[3];
}

inline int anyGreater(const Scalar4i& v0, const Scalar4i& v1, Scalar4i& outMask)
{
	bool b0 = v0.i4[0] > v1.i4[0], b1 = v0.i4[1] > v1.i4[1], b2 = v0.i4[2] > v1.i4[2], b3 = v0.i4[3] > v1.i4[3];
	outMask = Scalar4i(b0, b1, b2, b3);
	return b0 || b1 || b2 || b3;
}

inline int allTrue(const Scalar4i& v)
{
	return v.i4[0] & v.i4[1] & v.i4[2] & v.i4[3];
}

inline int anyTrue(const Scalar4i& v)
{
	return v.i4[0] | v.i4[1] | v.i4[2] | v.i4[3];
}

NV_SIMD_NAMESPACE_END
