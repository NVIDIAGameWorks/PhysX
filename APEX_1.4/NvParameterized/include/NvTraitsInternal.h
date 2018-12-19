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

#ifndef PX_TRAITS_INTERNAL_H
#define PX_TRAITS_INTERNAL_H

#include "nvparameterized/NvParameterizedTraits.h"
#include "PsString.h"

namespace NvParameterized
{

PX_PUSH_PACK_DEFAULT

#ifndef WITHOUT_APEX_SERIALIZATION
#ifdef _MSC_VER
#pragma warning(disable:4127) // conditional expression is constant
#endif
#define NV_PARAM_TRAITS_WARNING(_traits, _format, ...) do { \
		char _tmp[256]; \
		physx::shdfnd::snprintf(_tmp, sizeof(_tmp), _format, ##__VA_ARGS__); \
		_traits->traitsWarn(_tmp); \
	} while(0)

#else
#define NV_PARAM_TRAITS_WARNING(...)
#endif

// Determines how default converter handles included references
struct RefConversionMode
{
	enum Enum
	{
		// Simply move it to new object (and remove from old)
		REF_CONVERT_COPY = 0,

		// Same as REF_CONVERT_COPY but also update legacy references
		REF_CONVERT_UPDATE,

		// Skip references
		REF_CONVERT_SKIP,

		REF_CONVERT_LAST
	};
};

// Specifies preferred version for element at position specified in longName
struct PrefVer
{
	const char *longName;
	uint32_t ver;
};

// A factory function to create an instance of default conversion.
Conversion *internalCreateDefaultConversion(Traits *traits, const PrefVer *prefVers = 0, RefConversionMode::Enum refMode = RefConversionMode::REF_CONVERT_COPY);

PX_POP_PACK

}

#endif