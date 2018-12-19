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

#ifndef PLATFORM_INPUT_STREAM_H_
#define PLATFORM_INPUT_STREAM_H_

// WARNING: before doing any changes to this file
// check comments at the head of BinSerializer.cpp

#include "PlatformStream.h"
#include "ApbDefinitions.h"

namespace NvParameterized
{

//ABI-aware input stream
class PlatformInputStream: public PlatformStream
{
	physx::PxFileBuf &mStream;
	physx::shdfnd::Array<uint32_t, Traits::Allocator> mPos;
	uint32_t mStartPos; //This is necessary to handle NvParameterized streams starting in the middle of other files

	PlatformInputStream(const PlatformInputStream &); //Don't
	void operator =(const PlatformInputStream &); //Don't

public:
	PlatformInputStream(physx::PxFileBuf &stream, const PlatformABI &targetParams, Traits *traits);

	Serializer::ErrorType skipBytes(uint32_t nbytes);

	Serializer::ErrorType pushPos(uint32_t newPos);

	void popPos();

	uint32_t getPos() const;

	//Read string stored at given offset
	//TODO: this could much faster if we loaded whole dictionary in the beginning
	Serializer::ErrorType readString(uint32_t off, const char *&s);

	//Deserialize header of NvParameterized object (see wiki for details)
	Serializer::ErrorType readObjHeader(ObjHeader &hdr);

	//Deserialize primitive type
	template<typename T> PX_INLINE Serializer::ErrorType read(T &x, bool doAlign = true);

	//Deserialize array of primitive type (slow path)
	template<typename T> PX_INLINE Serializer::ErrorType readSimpleArraySlow(Handle &handle);

	//Deserialize array of structs of primitive type
	Serializer::ErrorType readSimpleStructArray(Handle &handle);

	//Deserialize array of primitive type
	template<typename T> PX_INLINE Serializer::ErrorType readSimpleArray(Handle &handle);

	//Align current offset according to supplied alignment and padding
	void beginStruct(uint32_t align_, uint32_t pad_);

	//Align current offset according to supplied alignment (padding = alignment)
	void beginStruct(uint32_t align_);

	//Align current offset according to supplied DataType
	void beginStruct(const Definition *pd);

	//Insert tail padding
	void closeStruct();

	//beginStruct for DummyStringStruct
	void beginString();

	//closeStruct for DummyStringStruct
	void closeString();

	//beginStruct for arrays
	void beginArray(const Definition *pd);

	//closeStruct for arrays
	void closeArray();

	//Align offset to be n*border
	void align(uint32_t border);

	//Read value of pointer (offset from start of file)
	Serializer::ErrorType readPtr(uint32_t &val);
};

#include "PlatformInputStream.inl"

}

#endif
