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

// SSE + SSE2 (don't include intrin.h!)
#include <emmintrin.h>

#if defined _MSC_VER && !(defined NV_SIMD_USE_NAMESPACE && NV_SIMD_USE_NAMESPACE)

// SIMD libarary lives in global namespace and Simd4f is
// typedef'd  to __m128 so it can be passed by value on MSVC.

typedef __m128 Simd4f;
typedef __m128i Simd4i;

#else

NV_SIMD_NAMESPACE_BEGIN

/** \brief SIMD type containing 4 floats */
struct Simd4f
{
	Simd4f()
	{
	}
	Simd4f(__m128 x) : m128(x)
	{
	}

	operator __m128&()
	{
		return m128;
	}
	operator const __m128&() const
	{
		return m128;
	}

  private:
	__m128 m128;
};

/** \brief SIMD type containing 4 integers */
struct Simd4i
{
	Simd4i()
	{
	}
	Simd4i(__m128i x) : m128i(x)
	{
	}

	operator __m128i&()
	{
		return m128i;
	}
	operator const __m128i&() const
	{
		return m128i;
	}

  private:
	__m128i m128i;
};

NV_SIMD_NAMESPACE_END

#endif
