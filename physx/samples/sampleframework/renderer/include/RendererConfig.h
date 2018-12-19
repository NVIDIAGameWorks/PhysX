// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.

#ifndef RENDERER_CONFIG_H
#define RENDERER_CONFIG_H

#include <RendererFoundation.h>
#include <assert.h>

#define RENDERER_TEXT(_foo)   #_foo
#define RENDERER_TEXT2(_foo)  RENDERER_TEXT(_foo)

// number of lights required before it switches from forward rendering to deferred rendering.
#define RENDERER_DEFERRED_THRESHOLD  0x7FFFFFFF // set to a big number just to disable it for now...

// Enables/Disables support for dresscode in the renderer...
#define RENDERER_ENABLE_DRESSCODE 0

// If turned on, asserts get compiled in as print statements in release mode.
#define RENDERER_ENABLE_CHECKED_RELEASE 0

// If enabled, all lights will be bound in a single pass. Requires appropriate shader support.
#define RENDERER_ENABLE_SINGLE_PASS_LIGHTING 0

// maximum number of bones per-drawcall allowed.
#define RENDERER_MAX_BONES 60

#define RENDERER_TANGENT_CHANNEL            5
#define RENDERER_BONEINDEX_CHANNEL          6
#define RENDERER_BONEWEIGHT_CHANNEL         7
#define RENDERER_INSTANCE_POSITION_CHANNEL  8
#define RENDERER_INSTANCE_NORMALX_CHANNEL   9
#define RENDERER_INSTANCE_NORMALY_CHANNEL  10
#define RENDERER_INSTANCE_NORMALZ_CHANNEL  11
#define RENDERER_INSTANCE_VEL_LIFE_CHANNEL 12
#define RENDERER_INSTANCE_DENSITY_CHANNEL  13

#define RENDERER_INSTANCE_UV_CHANNEL       12
#define RENDERER_INSTANCE_LOCAL_CHANNEL    13

#define RENDERER_DISPLACEMENT_CHANNEL      14
#define RENDERER_X_DISPLACEMENT_CHANNEL    13
#define RENDERER_Y_DISPLACEMENT_CHANNEL    14
#define RENDERER_Z_DISPLACEMENT_CHANNEL    15
#define RENDERER_DISPLACEMENT_FLAGS_CHANNEL 15

// Compiler specific configuration...
#if defined(_MSC_VER)
#define RENDERER_VISUALSTUDIO
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4100) // unreferenced formal parameter

#elif defined(__GNUC__)
	#define RENDERER_GCC
#else
#error Unknown Compiler!

#endif

// Platform specific configuration...
#if defined(WIN32) || defined(WIN64)
	#define RENDERER_WINDOWS
	#define RENDERER_ENABLE_DIRECT3D9
	#define RENDERER_ENABLE_DIRECT3D11
	#define RENDERER_ENABLE_NVPERFHUD
	#define RENDERER_ENABLE_TGA_SUPPORT
	#if !defined(RENDERER_PVD) && PX_SUPPORT_GPU_PHYSX
		//Removed this to get PhysX distro working without shipping CUDA.
		//#define RENDERER_ENABLE_CUDA_INTEROP 
	#endif
	#define	DIRECT3D9_SUPPORT_D3DUSAGE_DYNAMIC
	#if defined(WIN64)
		#define RENDERER_64BIT
	#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define NOMINMAX
#include <windows.h>
#elif defined(__APPLE__)
#define RENDERER_MACOSX
#define RENDERER_ENABLE_OPENGL
#define RENDERER_ENABLE_CG
#if !PX_PPC
#define RENDERER_ENABLE_TGA_SUPPORT
#endif

#elif defined(__CYGWIN__)
#define RENDERER_LINUX
#define RENDERER_ENABLE_OPENGL
#define RENDERER_ENABLE_CG
#define RENDERER_ENABLE_TGA_SUPPORT

#elif defined(__linux__)
	#define RENDERER_LINUX
	#define RENDERER_ENABLE_OPENGL
	#define RENDERER_ENABLE_CG
	#define RENDERER_ENABLE_TGA_SUPPORT

#else
#error "Unknown Platform!"

#endif

#if PX_DEBUG
#define RENDERER_DEBUG
#endif


#if defined(RENDERER_DEBUG)
	#if defined(RENDERER_WINDOWS)
		#define RENDERER_ASSERT(_exp, _msg)                     \
		    if(!(_exp))                                         \
		    {                                                   \
		        MessageBoxA(0, _msg, "Renderer Assert", MB_OK); \
		        __debugbreak();                                 \
		    }
	#else
		#define RENDERER_ASSERT(_exp, _msg) assert(_exp && (_msg));
	#endif
#elif RENDERER_ENABLE_CHECKED_RELEASE
#if defined(RENDERER_VISUALSTUDIO)
#define RENDERER_ASSERT(_exp, _msg)                                          \
	if(!(_exp))                                                              \
	{                                                                        \
		OutputDebugStringA("*** (" __FILE__":"RENDERER_TEXT2(__LINE__)") "); \
		OutputDebugStringA(_msg);                                            \
		OutputDebugStringA(" ***\n");                                        \
	}
#else
#define RENDERER_ASSERT(_exp, _msg) if(!(_exp)) shdfnd::printFormatted("*** (" __FILE__ ":" RENDERER_TEXT2(__LINE__)") %s ***\n", _msg);
#endif
#else
#define RENDERER_ASSERT(_exp, _msg)
#endif

#define RENDERER_OUTPUT_MESSAGE(_rendererPtr, _msg) \
	if((_rendererPtr) && (_rendererPtr)->getErrorCallback()) \
	{ \
		(_rendererPtr)->getErrorCallback()->reportError(PxErrorCode::eDEBUG_INFO, (_msg), __FILE__, __LINE__); \
	}


#if 0
	#include <stdio.h>
	#include <stdarg.h>

	static void printInfo(const char* title, const char* message, ...)
	{
		char buff[4096];
		
		shdfnd::printFormatted("%s ", title);
		va_list va;
		va_start(va, message);
		vsprintf(buff, message, va);
		va_end(va);
		if (strlen(buff)>=4096)
			assert(!"buffer overflow!!");
		
		shdfnd::printFormatted("%s\n", buff);
	}

	#define LOG_INFO(title, ...) (printInfo(title, __VA_ARGS__))
#else
	#define LOG_INFO(title, ...)
#endif

#define RENDERER_INSTANCING 1

namespace SampleRenderer
{
	// 2D and 3D Textures have identical external interfaces
	//    Using a typedef provides compatibility with legacy code that used only 2D textures
	class RendererTexture;
	class RendererTextureDesc;
	typedef RendererTexture     RendererTexture2D;
	typedef RendererTextureDesc RendererTexture2DDesc;
	typedef RendererTexture     RendererTexture3D;
	typedef RendererTextureDesc RendererTexture3DDesc;
}
#define LOGI(...) LOG_INFO("LOGI: ", __VA_ARGS__)

#endif
