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


#ifndef PX_PHYSICS_COMMON
#define PX_PHYSICS_COMMON

//! \file Top level internal include file for PhysX SDK

#include "Ps.h"
#include "PxFoundation.h"

#ifndef CACHE_LOCAL_CONTACTS_XP
#define CACHE_LOCAL_CONTACTS_XP 1
#endif

#if PX_CHECKED
	#define PX_CHECK_MSG(exp, msg)				(!!(exp) || (PxGetFoundation().getErrorCallback().reportError(PxErrorCode::eINVALID_PARAMETER, msg, __FILE__, __LINE__), 0) )
	#define PX_CHECK(exp)						PX_CHECK_MSG(exp, #exp)
	#define PX_CHECK_AND_RETURN(exp,msg)		{ if(!(exp)) { PX_CHECK_MSG(exp, msg); return; } }
	#define PX_CHECK_AND_RETURN_NULL(exp,msg)	{ if(!(exp)) { PX_CHECK_MSG(exp, msg); return 0; } }
	#define PX_CHECK_AND_RETURN_VAL(exp,msg,r)	{ if(!(exp)) { PX_CHECK_MSG(exp, msg); return r; } }
#else
	#define PX_CHECK_MSG(exp, msg)
	#define PX_CHECK(exp)
	#define PX_CHECK_AND_RETURN(exp,msg)
	#define PX_CHECK_AND_RETURN_NULL(exp,msg)
	#define PX_CHECK_AND_RETURN_VAL(exp,msg,r)
#endif

#if PX_VC
	// VC compiler defines __FUNCTION__ as a string literal so it is possible to concatenate it with another string
	// Example: #define PX_CHECK_VALID(x)	PX_CHECK_MSG(shdfnd::checkValid(x), __FUNCTION__ ": parameter invalid!")
	#define PX_CHECK_VALID(x)				PX_CHECK_MSG(shdfnd::checkValid(x), __FUNCTION__)
#elif PX_GCC_FAMILY || PX_GHS
	// GCC compiler defines __FUNCTION__ as a variable, hence, it is NOT possible concatenate an additional string to it
	// In GCC, __FUNCTION__ only returns the function name, using __PRETTY_FUNCTION__ will return the full function definition
	#define PX_CHECK_VALID(x)				PX_CHECK_MSG(shdfnd::checkValid(x), __PRETTY_FUNCTION__)
#else
	// Generic macro for other compilers
	#define PX_CHECK_VALID(x)				PX_CHECK_MSG(shdfnd::checkValid(x), __FUNCTION__)
#endif


#endif
