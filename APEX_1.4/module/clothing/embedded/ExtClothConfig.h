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


#ifndef EXT_CLOTH_CONFIG_NX
#define EXT_CLOTH_CONFIG_NX

/** \addtogroup common 
@{ */

#include "Px.h"

// Exposing of the Cloth API. Run API meta data generation in Tools/PhysXMetaDataGenerator when changing.
#define APEX_USE_CLOTH_API 1

// define API function declaration (public API only needed because of extensions)
#if defined EXT_CLOTH_STATIC_LIB
	#define EXT_CLOTH_CORE_API
#else
	#if PX_WINDOWS_FAMILY || PX_WINRT
		#if defined EXT_CLOTH_CORE_EXPORTS
			#define EXT_CLOTH_CORE_API PX_DLL_EXPORT
		#else
			#define EXT_CLOTH_CORE_API PX_DLL_IMPORT
		#endif
	#elif PX_UNIX_FAMILY
		#define EXT_CLOTH_CORE_API PX_UNIX_EXPORT
    #else
		#define EXT_CLOTH_CORE_API
    #endif
#endif

#if PX_WINDOWS_FAMILY || PX_WINRT && !defined(__CUDACC__)
	#if defined EXT_CLOTH_COMMON_EXPORTS
		#define EXT_CLOTH_COMMON_API __declspec(dllexport)
	#else
		#define EXT_CLOTH_COMMON_API __declspec(dllimport)
	#endif
#elif PX_UNIX_FAMILY
	#define EXT_CLOTH_COMMON_API PX_UNIX_EXPORT
#else
	#define EXT_CLOTH_COMMON_API
#endif

// Changing these parameters requires recompilation of the SDK

#ifndef PX_DOXYGEN
namespace physx
{
#endif
	class PxCollection;
	class PxBase;

	class PxHeightField;
	class PxHeightFieldDesc;

	class PxTriangleMesh;
	class PxConvexMesh;

	typedef uint32_t PxTriangleID;
	typedef uint16_t PxMaterialTableIndex;

#ifndef PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
