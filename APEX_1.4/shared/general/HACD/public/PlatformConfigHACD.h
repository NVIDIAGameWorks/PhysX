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

#ifndef PLATFORM_CONFIG_H

#define PLATFORM_CONFIG_H

// Modify this header file to make the HACD data types be compatible with your
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <new>

#include "PxSimpleTypes.h"
#include "PxMath.h"
#include "PsAllocator.h"
#include "PsUserAllocated.h"
#include "PsString.h"

#if PX_VC == 9
typedef uint8_t uint8_t;
typedef uint16_t uint16_t;
typedef uint32_t uint32_t;
typedef uint64_t uint64_t;
typedef int8_t int8_t;
typedef int16_t int16_t;
typedef int32_t int32_t;
typedef int64_t int64_t;
#else
#include <stdint.h>
#endif

// This header file provides a brief compatibility layer between the PhysX and APEX SDK foundation header files.
// Modify this header file to your own data types and memory allocation routines and do a global find/replace if necessary

namespace hacd
{

class PxEmpty;

#define HACD_SIGN_BITMASK		0x80000000

	// avoid unreferenced parameter warning (why not just disable it?)
	// PT: or why not just omit the parameter's name from the declaration????
#define HACD_FORCE_PARAMETER_REFERENCE(_P) (void)(_P);
#define HACD_UNUSED(_P) HACD_FORCE_PARAMETER_REFERENCE(_P)

#define HACD_ALLOC_ALIGNED(x,y) PX_ALLOC(x,#x)
#define HACD_ALLOC(x) PX_ALLOC(x,#x)
#define HACD_FREE PX_FREE

#define HACD_ASSERT PX_ASSERT
#define HACD_ALWAYS_ASSERT PX_ALWAYS_ASSERT

#define HACD_INLINE PX_INLINE
#define HACD_NOINLINE PX_NOINLINE
#define HACD_FORCE_INLINE PX_FORCE_INLINE
#define HACD_PLACEMENT_NEW PX_PLACEMENT_NEW

#define HACD_SPRINTF_S physx::shdfnd::snprintf

#define HACD_ISFINITE(x) physx::PxIsFinite(x)

#define HACD_NEW PX_NEW

#if PX_WINDOWS_FAMILY
#define HACD_WINDOWS
#endif

#if PX_VC
#define HACD_VC
#endif

#if PX_VC == 9
#define HACD_VC9
#endif

#if PX_VC == 8
#define HACD_VC8
#endif

#if PX_VC == 7
#define HACD_VC7
#endif

#if PX_VC == 6
#define HACD_VC6
#endif

#if PX_GCC_FAMILY
#define HACD_GNUC
#endif

#if PX_CW
#define HACD_CW
#endif

#if PX_X86
#define HACD_X86
#endif

#if PX_X64
#define HACD_X64
#endif

#if PX_A64
#define HACD_A64
#endif

#if PX_PPC
#define HACD_PPC
#endif

#if PX_PPC64
#define HACD_PPC64
#endif

#if PX_VMX
#define HACD_VMX
#endif

#ifdef  PX_ARM
#define HACD_ARM
#endif

#if PX_WINDOWS_FAMILY
#define HACD_WINDOWS
#endif

#if PX_LINUX_FAMILY
#define HACD_LINUX
#endif

#if PX_ANDROID
#define HACD_ANDROID
#endif

#if PX_APPLE_FAMILY
#define HACD_APPLE
#endif

#if PX_CYGWIN
#define HACD_CYGWIN
#endif

#if PX_WII
#define HACD_WII
#endif

#if PX_GC
#define HACD_GC
#endif


#define HACD_C_EXPORT PX_C_EXPORT
#define HACD_CALL_CONV PX_CALL_CONV
#define HACD_PUSH_PACK_DEFAULT PX_PUSH_PACK_DEFAULT
#define HACD_POP_PACK PX_POP_PACK
#define HACD_INLINE PX_INLINE
#define HACD_FORCE_INLINE PX_FORCE_INLINE
#define HACD_NOINLINE PX_NOINLINE
#define HACD_RESTRICT PX_RESTRICT
#define HACD_NOALIAS PX_NOALIAS
#define HACD_ALIGN PX_ALIGN
#define HACD_ALIGN_PREFIX PX_ALIGN_PREFIX
#define HACD_ALIGN_SUFFIX PX_ALIGN_SUFFIX
#define HACD_DEPRECATED PX_DEPRECATED
#define HACD_JOIN_HELPER PX_JOIN_HELPER
#define HACD_JOIN PX_JOIN
#define HACD_COMPILE_TIME_ASSERT PX_COMPILE_TIME_ASSERT
#define HACD_OFFSET_OF PX_OFFSET_OF
#define HACD_CHECKED PX_CHECKED
#define HACD_CUDA_CALLABLE PX_CUDA_CALLABLE


class ICallback
{
public:
	virtual void ReportProgress(const char *, float progress) = 0;
	virtual bool Cancelled() = 0;
};

#if defined(HACD_X64) || defined(HACD_A64)
typedef uint64_t HaSizeT;
#else
typedef uint32_t HaSizeT;
#endif


}; // end HACD namespace

#define UANS physx::shdfnd	// the user allocator namespace

#define HACD_SQRT(x) float (physx::PxSqrt(x))	
#define HACD_RECIP_SQRT(x) float (physx::PxRecipSqrt(x))	

#include "PxVector.h"



#endif
