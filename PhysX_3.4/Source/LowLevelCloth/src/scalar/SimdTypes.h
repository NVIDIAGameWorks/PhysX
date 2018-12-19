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

#include <algorithm>

#ifdef _MSC_VER
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
#endif

NV_SIMD_NAMESPACE_BEGIN

/** \brief Scalar fallback for SIMD containing 4 floats */
union Scalar4f
{
	Scalar4f()
	{
	}

	Scalar4f(float x, float y, float z, float w)
	{
		f4[0] = x;
		f4[1] = y;
		f4[2] = z;
		f4[3] = w;
	}

	Scalar4f(uint32_t x, uint32_t y, uint32_t z, uint32_t w)
	{
		u4[0] = x;
		u4[1] = y;
		u4[2] = z;
		u4[3] = w;
	}

	Scalar4f(bool x, bool y, bool z, bool w)
	{
		u4[0] = ~(uint32_t(x) - 1);
		u4[1] = ~(uint32_t(y) - 1);
		u4[2] = ~(uint32_t(z) - 1);
		u4[3] = ~(uint32_t(w) - 1);
	}

	Scalar4f(const Scalar4f& other)
	{
		f4[0] = other.f4[0];
		f4[1] = other.f4[1];
		f4[2] = other.f4[2];
		f4[3] = other.f4[3];
	}

	Scalar4f& operator=(const Scalar4f& other)
	{
		f4[0] = other.f4[0];
		f4[1] = other.f4[1];
		f4[2] = other.f4[2];
		f4[3] = other.f4[3];
		return *this;
	}

	float f4[4];
	int32_t i4[4];
	uint32_t u4[4];
};

/** \brief Scalar fallback for SIMD containing 4 integers */
union Scalar4i
{
	Scalar4i()
	{
	}

	Scalar4i(int32_t x, int32_t y, int32_t z, int32_t w)
	{
		i4[0] = x;
		i4[1] = y;
		i4[2] = z;
		i4[3] = w;
	}

	Scalar4i(uint32_t x, uint32_t y, uint32_t z, uint32_t w)
	{
		u4[0] = x;
		u4[1] = y;
		u4[2] = z;
		u4[3] = w;
	}

	Scalar4i(bool x, bool y, bool z, bool w)
	{
		u4[0] = ~(uint32_t(x) - 1);
		u4[1] = ~(uint32_t(y) - 1);
		u4[2] = ~(uint32_t(z) - 1);
		u4[3] = ~(uint32_t(w) - 1);
	}

	Scalar4i(const Scalar4i& other)
	{
		u4[0] = other.u4[0];
		u4[1] = other.u4[1];
		u4[2] = other.u4[2];
		u4[3] = other.u4[3];
	}

	Scalar4i& operator=(const Scalar4i& other)
	{
		u4[0] = other.u4[0];
		u4[1] = other.u4[1];
		u4[2] = other.u4[2];
		u4[3] = other.u4[3];
		return *this;
	}

	int32_t i4[4];
	uint32_t u4[4];
};

NV_SIMD_NAMESPACE_END
