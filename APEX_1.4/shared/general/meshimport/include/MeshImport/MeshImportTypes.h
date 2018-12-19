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



#ifndef MESH_IMPORT_TYPES_H

#define MESH_IMPORT_TYPES_H

// If the mesh import plugin source code is embedded directly into the APEX samples then we create a remapping between
// PhysX data types and MeshImport data types for compatbility between all platforms.
#ifdef PLUGINS_EMBEDDED

#include "PxSimpleTypes.h"
#include "ApexUsingNamespace.h"

namespace mimp
{

typedef int64_t	MiI64;
typedef int32_t	MiI32;
typedef int16_t	MiI16;
typedef int8_t	MiI8;

typedef uint64_t	MiU64;
typedef uint32_t	MiU32;
typedef uint16_t	MiU16;
typedef uint8_t	MiU8;

typedef float	MiF32;
typedef double	MiF64;

#if PX_X86
#define MI_X86
#endif

#if PX_WINDOWS_FAMILY
#define MI_WINDOWS
#endif

#if PX_X64
#define MI_X64
#endif

#define MI_PUSH_PACK_DEFAULT PX_PUSH_PACK_DEFAULT
#define MI_POP_PACK	 PX_POP_PACK

#define MI_INLINE PX_INLINE
#define MI_NOINLINE PX_NOINLINE
#define MI_FORCE_INLINE PX_FORCE_INLINE

};


#else

namespace mimp
{

	typedef signed __int64		MiI64;
	typedef signed int			MiI32;
	typedef signed short		MiI16;
	typedef signed char			MiI8;

	typedef unsigned __int64	MiU64;
	typedef unsigned int		MiU32;
	typedef unsigned short		MiU16;
	typedef unsigned char		MiU8;

	typedef float				MiF32;
	typedef double				MiF64;

/**
Platform define
*/
#if defined(_M_IX86) || PX_EMSCRIPTEN
#define MI_X86
#define MI_WINDOWS
#elif defined(_M_X64)
#define MI_X64
#define MI_WINDOWS
#endif

#define MI_PUSH_PACK_DEFAULT	__pragma( pack(push, 8) )
#define MI_POP_PACK			__pragma( pack(pop) )

#define MI_INLINE inline
#define MI_NOINLINE __declspec(noinline)
#define MI_FORCE_INLINE __forceinline


};

#endif

#endif
