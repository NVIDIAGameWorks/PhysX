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

/*! @file
\mainpage NVIDIA(R) SIMD Library
This library provides an abstraction to SSE2 and NEON SIMD instructions and provides
a scalar fallback for other architectures. The documentation of Simd4f and Simd4i contain
everything to get started.

The following design choices have been made:
- Use typedef for SSE2 data on MSVC (implies global namespace, see NV_SIMD_USE_NAMESPACE for options)
- Exposing SIMD types as float/integer values as well as bit patterns
- Free functions and overloaded operators for better code readability
- Expression templates for common use cases (and-not and multiply-add)
- Support for constants with same or individual values (see Scalar/TubleFactory)
- Documentation (!)
- Altivec/VMX128 support has been removed

The following areas could still use some work:
- generic shuffling instructions
- matrix and quaterion types

Here is a simple example of how to use the SIMD libarary:

\code
void foo(const float* ptr)
{
    assert(!(ptr & 0xf)); // make sure ptr is aligned
    using namespace nvidia::simd;
    Simd4f a = loadAligned(ptr);
    Simd4f b = simd4f(0.0f, 1.0f, 0.0f, 1.0f);
    Simd4f c = simd4f(3.0f);
    Simd4f d = a * b + gSimd4fOne; // maps to FMA on NEON
    Simd4f mask, e;
    // same result as e = max(c, d);
    if(anyGreater(c, d, mask))
        e = select(mask, c, d);
    Simd4f f = splat<2>(d) - rsqrt(e);
    printf("%f\n", array(f)[0]);
}
\endcode
*/

/*! \def NV_SIMD_SIMD
* Define Simd4f and Simd4i, which map to four 32bit float or integer tuples.
* */
// note: ps4 compiler defines _M_X64 without value
#if((defined _M_IX86) || (defined _M_X64) || (defined __i386__) || (defined __x86_64__) || (defined __EMSCRIPTEN__ && defined __SSE2__))
#define NV_SIMD_SSE2 1
#else
#define NV_SIMD_SSE2 0
#endif
#if defined (_M_ARM) || defined (__ARM_NEON__) || defined (__ARM_NEON)
#define NV_SIMD_NEON 1
#else
#define NV_SIMD_NEON 0
#endif
#define NV_SIMD_SIMD (NV_SIMD_SSE2 || NV_SIMD_NEON)

/*! \def NV_SIMD_SCALAR
* Define Scalar4f and Scalar4i (default: 0 if SIMD is supported, 1 otherwise).
* Scalar4f and Scalar4i can be typedef'd to Simd4f and Simd4i respectively to replace
* the SIMD classes, or they can be used in combination as template parameters to
* implement a scalar run-time fallback. */
#if !defined NV_SIMD_SCALAR
#define NV_SIMD_SCALAR !NV_SIMD_SIMD
#endif

// use template expression to fuse multiply-adds into a single instruction
#define NV_SIMD_FUSE_MULTIPLY_ADD (NV_SIMD_NEON)
// support shift by vector operarations
#define NV_SIMD_SHIFT_BY_VECTOR (NV_SIMD_NEON)
// support inline assembler
#if defined _M_ARM || defined SN_TARGET_PSP2 || defined __arm64__ || defined __aarch64__
#define NV_SIMD_INLINE_ASSEMBLER 0
#else
#define NV_SIMD_INLINE_ASSEMBLER 1
#endif

/*! \def NV_SIMD_USE_NAMESPACE
* \brief Set to 1 to define the SIMD library types and functions inside the nvidia::simd namespace.
* By default, the types and functions defined in this header live in the global namespace.
* This is because MSVC (prior to version 12, Visual Studio 2013) does an inferior job at optimizing
* SSE2 code when __m128 is wrapped in a struct (the cloth solver for example is more than 50% slower).
* Therefore, Simd4f is typedefe'd to __m128 on MSVC, and for name lookup to work all related functions
* live in the global namespace. This behavior can be overriden by defining NV_SIMD_USE_NAMESPACE to 1.
* The types and functions of the SIMD library are then defined inside the nvidia::simd namespace, but
* performance on MSVC version 11 and earlier is expected to be lower in this mode because __m128 and
* __m128i are wrapped into structs. Arguments need to be passed by reference in this mode.
* \see NV_SIMD_VECTORCALL, Simd4fArg */

#if defined NV_SIMD_USE_NAMESPACE&& NV_SIMD_USE_NAMESPACE
#define NV_SIMD_NAMESPACE_BEGIN                                                                                        \
	namespace nvidia                                                                                                   \
	{                                                                                                                  \
	namespace simd                                                                                                     \
	{
#define NV_SIMD_NAMESPACE_END                                                                                          \
	}                                                                                                                  \
	}
#else
#define NV_SIMD_NAMESPACE_BEGIN
#define NV_SIMD_NAMESPACE_END
#endif

// alignment struct to \c alignment byte
#ifdef _MSC_VER
#define NV_SIMD_ALIGN(alignment, decl) __declspec(align(alignment)) decl
#else
#define NV_SIMD_ALIGN(alignment, decl) decl __attribute__((aligned(alignment)))
#endif

// define a global constant
#ifdef _MSC_VER
#define NV_SIMD_GLOBAL_CONSTANT extern const __declspec(selectany)
#else
#define NV_SIMD_GLOBAL_CONSTANT extern const __attribute__((weak))
#endif

// suppress warning of unused identifiers
#if defined(__GNUC__)
#define NV_SIMD_UNUSED __attribute__((unused))
#else
#define NV_SIMD_UNUSED
#endif

// disable warning
#if defined _MSC_VER
#if _MSC_VER < 1700
#pragma warning(disable : 4347) // behavior change: 'function template' is called instead of 'function'
#endif
#pragma warning(disable : 4350) // behavior change: 'member1' called instead of 'member2'
#endif

NV_SIMD_NAMESPACE_BEGIN

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// expression templates
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*! \brief Expression template to fuse and-not. */
template <typename T>
struct ComplementExpr
{
	inline explicit ComplementExpr(T const& v_) : v(v_)
	{
	}
	ComplementExpr& operator=(const ComplementExpr&); // not implemented
	inline operator T() const;
	const T v;
};

template <typename T>
inline T operator&(const ComplementExpr<T>&, const T&);
template <typename T>
inline T operator&(const T&, const ComplementExpr<T>&);

NV_SIMD_NAMESPACE_END

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// platform specific includes
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if NV_SIMD_SSE2
#include "sse2/SimdTypes.h"
#elif NV_SIMD_NEON
#include "neon/SimdTypes.h"
#elif NV_SIMD_SIMD
#error unknown SIMD architecture
#else
struct Simd4f;
struct Simd4i;
#endif

#if NV_SIMD_SCALAR
#include "scalar/SimdTypes.h"
#else
struct Scalar4f;
struct Scalar4i;
#endif

NV_SIMD_NAMESPACE_BEGIN

/*! \typedef Simd4fArg
* Maps to Simd4f value or reference, whichever is faster. */

/*! \def NV_SIMD_VECTORCALL
* MSVC passes aligned arguments by pointer, unless the vector calling convention
* introduced in Visual Studio 2013 is being used. For the last bit of performance
* of non-inlined functions, use the following pattern:
* Simd4f NV_SIMD_VECTORCALL foo(Simd4fArg x);
* This will pass the argument in register where possible (instead of by pointer).
* For inlined functions, the compiler will remove the store/load (except for MSVC
* when NV_SIMD_USE_NAMESPACE is set to 1).
* Non-inlined functions are rarely perf-critical, so it might be simpler
* to always pass by reference instead: Simd4f foo(const Simd4f&); */

#if defined _MSC_VER
#if _MSC_VER >= 1800 // Visual Studio 2013
typedef Simd4f Simd4fArg;
#define NV_SIMD_VECTORCALL __vectorcall
#else
typedef const Simd4f& Simd4fArg;
#define NV_SIMD_VECTORCALL
#endif
#else
typedef Simd4f Simd4fArg;
#define NV_SIMD_VECTORCALL
#endif

NV_SIMD_NAMESPACE_END
