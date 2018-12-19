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


#ifndef MI_ASCII_CONVERSION_H
#define MI_ASCII_CONVERSION_H

/*!
\file
\brief MiAsciiConversion namespace contains string/value helper functions
*/


#include "MiPlatformConfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <float.h>

namespace mimp
{

	namespace MiAsc
	{

const MiU32 MiF32StrLen = 24;
const MiU32 MiF64StrLen = 32;
const MiU32 IntStrLen = 32;

MI_INLINE bool isWhiteSpace(char c);
MI_INLINE const char * skipNonWhiteSpace(const char *scan);
MI_INLINE const char * skipWhiteSpace(const char *scan);

//////////////////////////
// str to value functions
//////////////////////////
MI_INLINE bool strToBool(const char *str, const char **endptr);
MI_INLINE MiI8  strToI8(const char *str, const char **endptr);
MI_INLINE MiI16 strToI16(const char *str, const char **endptr);
MI_INLINE MiI32 strToI32(const char *str, const char **endptr);
MI_INLINE MiI64 strToI64(const char *str, const char **endptr);
MI_INLINE MiU8  strToU8(const char *str, const char **endptr);
MI_INLINE MiU16 strToU16(const char *str, const char **endptr);
MI_INLINE MiU32 strToU32(const char *str, const char **endptr);
MI_INLINE MiU64 strToU64(const char *str, const char **endptr);
MI_INLINE MiF32 strToF32(const char *str, const char **endptr);
MI_INLINE MiF64 strToF64(const char *str, const char **endptr);
MI_INLINE void strToF32s(MiF32 *v,MiU32 count,const char *str, const char**endptr);


//////////////////////////
// value to str functions
//////////////////////////
MI_INLINE const char * valueToStr( bool val, char *buf, MiU32 n );
MI_INLINE const char * valueToStr( MiI8 val, char *buf, MiU32 n );
MI_INLINE const char * valueToStr( MiI16 val, char *buf, MiU32 n );
MI_INLINE const char * valueToStr( MiI32 val, char *buf, MiU32 n );
MI_INLINE const char * valueToStr( MiI64 val, char *buf, MiU32 n );
MI_INLINE const char * valueToStr( MiU8 val, char *buf, MiU32 n );
MI_INLINE const char * valueToStr( MiU16 val, char *buf, MiU32 n );
MI_INLINE const char * valueToStr( MiU32 val, char *buf, MiU32 n );
MI_INLINE const char * valueToStr( MiU64 val, char *buf, MiU32 n );
MI_INLINE const char * valueToStr( MiF32 val, char *buf, MiU32 n );
MI_INLINE const char * valueToStr( MiF64 val, char *buf, MiU32 n );

#include "MiAsciiConversion.inl"

	}; // end of MiAsc namespace

}; // end of namespace


#endif // MI_ASCII_CONVERSION_H
