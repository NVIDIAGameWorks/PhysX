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

#ifndef __CUDACC__
#include "ApexUsingNamespace.h"
#include "Px.h"
#include "PxVec3.h"
#include "PxVec4.h"
#include "PxQuat.h"
#endif

// Factory.cpp gets included in both PhysXGPU and LowLevelCloth projects
// CuFactory can only be created in PhysXGPU project
// DxFactory can only be created in PhysXGPU (win) or LowLevelCloth (xbox1)
#if defined(PX_PHYSX_GPU_EXPORTS) || PX_XBOXONE
#define ENABLE_CUFACTORY ((PX_WINDOWS_FAMILY && (PX_WINRT==0)) || PX_LINUX)

//TEMPORARY DISABLE DXFACTORY
#define ENABLE_DXFACTORY 0
//#define ENABLE_DXFACTORY ((PX_WINDOWS_FAMILY && (PX_WINRT==0)) || PX_XBOXONE)
#else
#define ENABLE_CUFACTORY 0
#define ENABLE_DXFACTORY 0
#endif

#ifndef _MSC_VER
#include <stdint.h>
#else
// typedef standard integer types
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
#if _MSC_VER < 1600
#define nullptr NULL
#endif
#endif
