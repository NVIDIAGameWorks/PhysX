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

#include <cmath>

// ps4 compiler defines _M_X64 without value
#if !defined(PX_SIMD_DISABLED)
#if((defined _M_IX86) || (defined _M_X64) || (defined __i386__) || (defined __x86_64__) || (defined __EMSCRIPTEN__ && defined __SSE2__))
#define NVMATH_SSE2 1
#else
#define NVMATH_SSE2 0
#endif
#if((defined _M_ARM) || (defined __ARM_NEON__) || (defined __ARM_NEON_FP))
#define NVMATH_NEON 1
#else
#define NVMATH_NEON 0
#endif
#else
#define NVMATH_SSE2 0
#define NVMATH_NEON 0
#endif

// which simd types are implemented (one or both are all valid options)
#define NVMATH_SIMD (NVMATH_SSE2 || NVMATH_NEON)
#define NVMATH_SCALAR !NVMATH_SIMD
// #define NVMATH_SCALAR 1

// use template expression to fuse multiply-adds into a single instruction
#define NVMATH_FUSE_MULTIPLY_ADD (NVMATH_NEON)
// support shift by vector operarations
#define NVMATH_SHIFT_BY_VECTOR (NVMATH_NEON)
// Simd4f and Simd4i map to different types
#define NVMATH_DISTINCT_TYPES (NVMATH_SSE2 || NVMATH_NEON)
// support inline assembler
#if !((defined _M_ARM) || (defined SN_TARGET_PSP2) || (defined __arm64__) || (defined __aarch64__))
#define NVMATH_INLINE_ASSEMBLER 1
#else
#define NVMATH_INLINE_ASSEMBLER 0
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// expression template
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*! \brief Expression template to fuse and-not. */
template <typename T>
struct ComplementExpr
{
	inline ComplementExpr(T const& v_) : v(v_)
	{
	}
	inline operator T() const;
	const T v;

  private:
	ComplementExpr& operator=(const ComplementExpr&); // not implemented
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// helper functions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <typename T>
T sqr(const T& x)
{
	return x * x;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// details
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

namespace detail
{
template <typename T>
struct AlignedPointer
{
	AlignedPointer(const T* p) : ptr(p)
	{
	}
	const T* ptr;
};

template <typename T>
struct OffsetPointer
{
	OffsetPointer(const T* p, unsigned int off) : ptr(p), offset(off)
	{
	}
	const T* ptr;
	unsigned int offset;
};

struct FourTuple
{
};

// zero and one literals
template <int i>
struct IntType
{
};
}

// Supress warnings
#if defined(__GNUC__) || defined(__SNC__)
#define NVMATH_UNUSED __attribute__((unused))
#else
#define NVMATH_UNUSED
#endif

static detail::IntType<0> _0 NVMATH_UNUSED;
static detail::IntType<1> _1 NVMATH_UNUSED;
static detail::IntType<int(0x80000000)> _sign NVMATH_UNUSED;
static detail::IntType<-1> _true NVMATH_UNUSED;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// platform specific includes
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if NVMATH_SSE2
#include "sse2/SimdTypes.h"
#elif NVMATH_NEON
#include "neon/SimdTypes.h"
#else
struct Simd4f;
struct Simd4i;
#endif

#if NVMATH_SCALAR
#include "scalar/SimdTypes.h"
#else
struct Scalar4f;
struct Scalar4i;
#endif
