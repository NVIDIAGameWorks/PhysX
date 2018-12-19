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

#ifndef SERIALIZER_COMMON_H
#define SERIALIZER_COMMON_H

#include <stdio.h> // FILE

#include "PxAssert.h"

#include "PsArray.h"
#include "PsHashMap.h"

#include "nvparameterized/NvParameterized.h"
#include "nvparameterized/NvParameterizedTraits.h"
#include "nvparameterized/NvSerializer.h"

#define ENABLE_DEBUG_ASSERTS 1

#if ENABLE_DEBUG_ASSERTS
#	define DEBUG_ASSERT(x) PX_ASSERT(x)
#else
#	define DEBUG_ASSERT(x)
#endif
#define DEBUG_ALWAYS_ASSERT() DEBUG_ASSERT(0)

#define NV_ERR_CHECK_RETURN(x) { Serializer::ErrorType err = x; if( Serializer::ERROR_NONE != err ) { DEBUG_ALWAYS_ASSERT(); return err; } }
#define NV_BOOL_ERR_CHECK_RETURN(x, err) { if( !(x) ) { DEBUG_ALWAYS_ASSERT(); return err; } }
#define NV_PARAM_ERR_CHECK_RETURN(x, err) { if( NvParameterized::ERROR_NONE != (NvParameterized::ErrorType)(x) ) { DEBUG_ALWAYS_ASSERT(); return err; } }

#define NV_ERR_CHECK_WARN_RETURN(x, ...) { \
	Serializer::ErrorType err = x; \
	if( Serializer::ERROR_NONE != err ) { \
		NV_PARAM_TRAITS_WARNING(mTraits, ##__VA_ARGS__); \
		DEBUG_ALWAYS_ASSERT(); \
		return err; \
	} \
}

#define NV_BOOL_ERR_CHECK_WARN_RETURN(x, err, ...) { \
	if( !(x) ) { \
		NV_PARAM_TRAITS_WARNING(mTraits, ##__VA_ARGS__); \
		DEBUG_ALWAYS_ASSERT(); \
		return err; \
	} \
}

#define NV_PARAM_ERR_CHECK_WARN_RETURN(x, err, ...) { \
	if( NvParameterized::ERROR_NONE != (NvParameterized::ErrorType)(x) ) \
	{ \
		NV_PARAM_TRAITS_WARNING(mTraits, ##__VA_ARGS__); \
		DEBUG_ALWAYS_ASSERT(); \
		return err; \
	} \
}

namespace NvParameterized
{

bool UpgradeLegacyObjects(Serializer::DeserializedData &data, bool &isUpdated, Traits *t);
Interface *UpgradeObject(Interface &obj, bool &isUpdated, Traits *t);

//This is used for releasing resources (I wish we had generic smart ptrs...)
class Releaser
{
	FILE *mFile;

	void *mBuf;
	Traits *mTraits;

	Interface *mObj;

	Releaser(const Releaser &) {}

public:

	Releaser()
	{
		reset();
	}

	Releaser(void *buf, Traits *traits)
	{
		reset(buf, traits);
	}

	Releaser(FILE *file)
	{
		reset(file);
	}

	Releaser(Interface *obj)
	{
		reset(obj);
	}

	void reset()
	{
		mFile = 0;
		mBuf = 0;
		mTraits = 0;
		mObj = 0;
	}

	void reset(Interface *obj)
	{
		reset();

		mObj = obj;
	}

	void reset(FILE *file)
	{
		reset();

		mFile = file;
	}

	void reset(void *buf, Traits *traits)
	{
		reset();

		mBuf = buf;
		mTraits = traits;
	}

	~Releaser()
	{
		if( mBuf )
			mTraits->free(mBuf);

		if( mFile )
			fclose(mFile);

		if( mObj )
			mObj->destroy();
	}
};

void *serializerMemAlloc(uint32_t size, Traits *traits);
void serializerMemFree(void *mem, Traits *traits);

// Checksum for some classes is invalid
bool DoIgnoreChecksum(const Interface &obj);

} // namespace NvParameterized

#endif
