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



#ifndef PLATFORM_CONFIG_MESH_IMPORT_H

#define PLATFORM_CONFIG_MESH_IMPORT_H

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <float.h>
#include <new>

#include "MeshImportTypes.h"

// This header file provides a brief compatibility layer between the PhysX and APEX SDK foundation header files.
// Modify this header file to your own data types and memory allocation routines and do a global find/replace if necessary

namespace mimp
{

class MiEmpty;

#define MI_SIGN_BITMASK		0x80000000

	// avoid unreferenced parameter warning (why not just disable it?)
	// PT: or why not just omit the parameter's name from the declaration????
#define MI_FORCE_PARAMETER_REFERENCE(_P) (void)(_P);
#define MI_UNUSED(_P) MI_FORCE_PARAMETER_REFERENCE(_P)

#define MI_ALLOC(x) ::malloc(x)
#define MI_FREE(x) ::free(x)
#define MI_REALLOC(x,y) ::realloc(x,y)

#define MI_ASSERT(x) assert(x)
#define MI_ALWAYS_ASSERT() assert(0)

#define MI_PLACEMENT_NEW(p, T)  new(p) T

class MeshImportAllocated
{
public:
	MI_INLINE void* operator new(size_t size,MeshImportAllocated *t)
	{
		MI_FORCE_PARAMETER_REFERENCE(size);
		return t;
	}

	MI_INLINE void* operator new(size_t size,const char *className,const char* fileName, int lineno,size_t classSize)
	{
		MI_FORCE_PARAMETER_REFERENCE(className);
		MI_FORCE_PARAMETER_REFERENCE(fileName);
		MI_FORCE_PARAMETER_REFERENCE(lineno);
		MI_FORCE_PARAMETER_REFERENCE(classSize);
		return MI_ALLOC(size);
	}

	inline void* operator new[](size_t size,const char *className,const char* fileName, int lineno,size_t classSize)
	{
		MI_FORCE_PARAMETER_REFERENCE(className);
		MI_FORCE_PARAMETER_REFERENCE(fileName);
		MI_FORCE_PARAMETER_REFERENCE(lineno);
		MI_FORCE_PARAMETER_REFERENCE(classSize);
		return MI_ALLOC(size);
	}

	inline void  operator delete(void* p,MeshImportAllocated *t)
	{
		MI_FORCE_PARAMETER_REFERENCE(p);
		MI_FORCE_PARAMETER_REFERENCE(t);
		MI_ALWAYS_ASSERT(); // should never be executed
	}

	inline void  operator delete(void* p)
	{
		MI_FREE(p);
	}

	inline void  operator delete[](void* p)
	{
		MI_FREE(p);
	}

	inline void  operator delete(void *p,const char *className,const char* fileName, int line,size_t classSize)
	{
		MI_FORCE_PARAMETER_REFERENCE(className);
		MI_FORCE_PARAMETER_REFERENCE(fileName);
		MI_FORCE_PARAMETER_REFERENCE(line);
		MI_FORCE_PARAMETER_REFERENCE(classSize);
		MI_FREE(p);
	}

	inline void  operator delete[](void *p,const char *className,const char* fileName, int line,size_t classSize)
	{
		MI_FORCE_PARAMETER_REFERENCE(className);
		MI_FORCE_PARAMETER_REFERENCE(fileName);
		MI_FORCE_PARAMETER_REFERENCE(line);
		MI_FORCE_PARAMETER_REFERENCE(classSize);
		MI_FREE(p);
	}

};
}; // end of mimp namespace

#define MI_NEW(T) new(#T,__FILE__,__LINE__,sizeof(T)) T

#pragma warning(push)
#pragma warning(disable:4996)

#ifdef PLUGINS_EMBEDDED

#include "PsString.h"
#include "PxIntrinsics.h"

#define MESH_IMPORT_STRING physx::shdfnd
#define MESH_IMPORT_INTRINSICS nvidia::intrinsics

#else

namespace mimp
{
namespace string
{

	MI_INLINE MiI32 stricmp(const char *str, const char *str1) {return(::_stricmp(str, str1));}
	MI_INLINE MiI32 strnicmp(const char *str, const char *str1, size_t len) {return(::_strnicmp(str, str1, len));}
	MI_INLINE MiI32 strncat_s(char* a, MiI32 b, const char* c, size_t d) {return(::strncat_s(a, b, c, d));}
	MI_INLINE MiI32 strncpy_s( char *strDest, size_t sizeInBytes, const char *strSource, size_t count) {return(::strncpy_s( strDest,sizeInBytes,strSource, count));}
	MI_INLINE void strcpy_s(char* dest, size_t size, const char* src) {::strcpy_s(dest, size, src);}
	MI_INLINE void strcat_s(char* dest, size_t size, const char* src) {::strcat_s(dest, size, src);}
	MI_INLINE MiI32 _vsnprintf(char* dest, size_t size, const char* src, va_list arg) 
	{
		MiI32 r = ::_vsnprintf(dest, size, src, arg);

		return r;
	}
	MI_INLINE MiI32 vsprintf_s(char* dest, size_t size, const char* src, va_list arg)
	{
		MiI32 r = ::vsprintf_s(dest, size, src, arg);

		return r;
	}

	MI_INLINE MiI32 sprintf_s( char * _DstBuf, size_t _DstSize, const char * _Format, ...)
	{
		va_list arg;
		va_start( arg, _Format );
		MiI32 r = ::vsprintf_s(_DstBuf, _DstSize, _Format, arg);
		va_end(arg);

		return r;
	}
	MI_INLINE MiI32 sscanf_s( const char *buffer, const char *format,  ...)
	{
		va_list arg;
		va_start( arg, format );
		MiI32 r = ::sscanf_s(buffer, format, arg);
		va_end(arg);

		return r;
	};

	MI_INLINE void strlwr(char* str)
	{
		while ( *str )
		{
			if ( *str>='A' &&  *str<='Z' ) *str+=32;
			str++;
		}
	}

	MI_INLINE void strupr(char* str)
	{
		while ( *str )
		{
			if ( *str>='a' &&  *str<='z' ) *str-=32;
			str++;
		}
	}


}; // end of string namespace
}; // end of mimp namesapce


namespace mimp
{

	namespace intrinsics
	{
		static MI_FORCE_INLINE bool isFinite(float a)
		{
			return _finite(a) ? true : false;
		}
		static MI_FORCE_INLINE bool isFinite(double a)
		{
			return _finite(a) ? true : false;
		}

	}

};

#define MESH_IMPORT_STRING mimp::string
#define MESH_IMPORT_INTRINSICS mimp::intrinsics

#endif


#pragma warning(pop)


#define USE_STL 0 // set to 1 to use the standard template library for all code; if off it uses high performance custom containers which trap all memory allocations.

#if USE_STL

#include <map>
#include <set>
#include <vector>
#define STDNAME std

#else

#include "MiVector.h"
#include "MiMapSet.h"

#define STDNAME mimp

#endif



#define	MI_MAX_I8			127					//maximum possible sbyte value, 0x7f
#define	MI_MIN_I8			(-128)				//minimum possible sbyte value, 0x80
#define	MI_MAX_U8			255U				//maximum possible ubyte value, 0xff
#define	MI_MIN_U8			0					//minimum possible ubyte value, 0x00
#define	MI_MAX_I16			32767				//maximum possible sword value, 0x7fff
#define	MI_MIN_I16			(-32768)			//minimum possible sword value, 0x8000
#define	MI_MAX_U16			65535U				//maximum possible uword value, 0xffff
#define	MI_MIN_U16			0					//minimum possible uword value, 0x0000
#define	MI_MAX_I32			2147483647			//maximum possible sdword value, 0x7fffffff
#define	MI_MIN_I32			(-2147483647 - 1)	//minimum possible sdword value, 0x80000000
#define	MI_MAX_U32			4294967295U			//maximum possible udword value, 0xffffffff
#define	MI_MIN_U32			0					//minimum possible udword value, 0x00000000
#define	MI_MAX_F32			3.4028234663852885981170418348452e+38F	
//maximum possible float value
#define	MI_MAX_F64			DBL_MAX				//maximum possible double value

#define MI_EPS_F32			FLT_EPSILON			//maximum relative error of float rounding
#define MI_EPS_F64			DBL_EPSILON			//maximum relative error of double rounding

#define	MI_MAX_REAL		MI_MAX_F32
#define MI_EPS_REAL		MI_EPS_F32




#endif
