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

#include "Types.h"

namespace physx
{

namespace cloth
{

template <typename T>
struct Vec4T
{
	Vec4T()
	{
	}

	Vec4T(T a, T b, T c, T d) : x(a), y(b), z(c), w(d)
	{
	}

	template <typename S>
	Vec4T(const Vec4T<S>& other)
	{
		x = T(other.x);
		y = T(other.y);
		z = T(other.z);
		w = T(other.w);
	}

	template <typename Index>
	T& operator[](Index i)
	{
		return reinterpret_cast<T*>(this)[i];
	}

	template <typename Index>
	const T& operator[](Index i) const
	{
		return reinterpret_cast<const T*>(this)[i];
	}

	T x, y, z, w;
};

template <typename T>
Vec4T<T> operator*(const Vec4T<T>& vec, T scalar)
{
	return Vec4T<T>(vec.x * scalar, vec.y * scalar, vec.z * scalar, vec.w * scalar);
}

template <typename T>
Vec4T<T> operator/(const Vec4T<T>& vec, T scalar)
{
	return Vec4T<T>(vec.x / scalar, vec.y / scalar, vec.z / scalar, vec.w / scalar);
}

template <typename T>
T (&array(Vec4T<T>& vec))[4]
{
	return reinterpret_cast<T(&)[4]>(vec);
}

template <typename T>
const T (&array(const Vec4T<T>& vec))[4]
{
	return reinterpret_cast<const T(&)[4]>(vec);
}

typedef Vec4T<uint32_t> Vec4u;
typedef Vec4T<uint16_t> Vec4us;

} // namespace cloth

} // namespace physx
