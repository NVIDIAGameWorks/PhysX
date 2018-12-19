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



#ifndef APEX_DEFS_H
#define APEX_DEFS_H

/*!
\file
\brief Version identifiers and other macro definitions

	This file is intended to be usable without picking up the entire
	public APEX API, so it explicitly does not include Apex.h
*/

#include "PhysXSDKVersion.h"

/*!
 \def APEX_SDK_VERSION
 \brief APEX Framework API version

 Used for making sure you are linking to the same version of the SDK files
 that you have included.  Should be incremented with every API change.

 \def APEX_SDK_RELEASE
 \brief APEX SDK Release version

 Used for conditionally compiling user code based on the APEX SDK release version.

 \def DYNAMIC_CAST
 \brief Determines use of dynamic_cast<> by APEX modules

 \def APEX_USE_PARTICLES
 \brief Determines use of particle-related APEX modules

 \def APEX_DEFAULT_NO_INTEROP_IMPLEMENTATION
 \brief Provide API stubs with no CUDA interop support

 Use this to add default implementations of interop-related interfaces for UserRenderer.
*/

#include "foundation/PxPreprocessor.h"

#define APEX_SDK_VERSION 1
#define APEX_SDK_RELEASE 0x01040100

#if USE_RTTI
#define DYNAMIC_CAST(type) dynamic_cast<type>
#else
#define DYNAMIC_CAST(type) static_cast<type>
#endif

/// Enables CUDA code
#if defined(EXCLUDE_CUDA) && (EXCLUDE_CUDA > 0)
#define APEX_CUDA_SUPPORT 0
#else
#define APEX_CUDA_SUPPORT (PX_SUPPORT_GPU_PHYSX) && !(PX_LINUX)
#endif

/// Enables particles related code
#if !defined(EXCLUDE_PARTICLES) && PX_WINDOWS
#define APEX_USE_PARTICLES 1
#else
#define APEX_USE_PARTICLES 0
#endif

/// Enables code specific for UE4
#ifndef APEX_UE4
#define APEX_UE4 0
#endif

/// Enables Linux shared library
#ifndef APEX_LINUX_SHARED_LIBRARIES
#define APEX_LINUX_SHARED_LIBRARIES 0
#endif

#define APEX_DEFAULT_NO_INTEROP_IMPLEMENTATION 1


#endif // APEX_DEFS_H
