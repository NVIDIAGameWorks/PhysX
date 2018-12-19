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

#ifndef PLATFORM_ABI_H_
#define PLATFORM_ABI_H_

// WARNING: before doing any changes to this file
// check comments at the head of BinSerializer.cpp

#include "BinaryHelper.h"
#include "nvparameterized/NvSerializer.h"

namespace NvParameterized
{

//Describes platform ABI (endian, alignment, etc.)
struct PlatformABI
{
	enum Endian
	{
		LITTLE,
		BIG
	};
	
	Endian endian;

	//Sizes of basic types
	struct
	{
		uint32_t Char,
			Bool,
			pointer,
			real;
	} sizes;

	//Alignments of basic types
	struct
	{
		uint32_t Char,
			Bool,
			pointer,
			real,
			i8,
			i16,
			i32,
			i64,
			f32,
			f64;
	} aligns;

	//Does child class reuse tail padding of parent? (google for "overlaying tail padding")
	bool doReuseParentPadding;

	//Are empty base classes eliminated? (google for "empty base class optimization")
	//We may need this in future
	bool doEbo;

	//Get ABI of platform
	static Serializer::ErrorType GetPredefinedABI(const SerializePlatform &platform, PlatformABI &params);

	//Get alignment of (complex) NvParameterized data type
	uint32_t getAlignment(const Definition *pd) const;

	//Get padding of (complex) NvParameterized data type
	uint32_t getPadding(const Definition *pd) const;

	//Get size of (complex) NvParameterized data type
	uint32_t getSize(const Definition *pd) const;

	//Helper function which calculates aligned value
	PX_INLINE static uint32_t align(uint32_t len, uint32_t border);

	//Verifying that platforms are going to work with our serializer
	PX_INLINE bool isNormal() const;
	static bool VerifyCurrentPlatform();

	//Alignment of metadata table entry in metadata section
	//TODO: find better place for this
	PX_INLINE uint32_t getMetaEntryAlignment() const;

	//Alignment of metadata info in metadata section
	//TODO: find better place for this
	PX_INLINE uint32_t getMetaInfoAlignment() const;

	//Alignment of hint in metadata section
	//TODO: find better place for this
	PX_INLINE uint32_t getHintAlignment() const;

	//Alignment of hint value (union of uint32_t, uint64_t, const char *) in metadata section
	//TODO: find better place for this
	PX_INLINE uint32_t getHintValueAlignment() const;

	//Size of hint value (union of uint32_t, uint64_t, const char *) in metadata section
	//TODO: find better place for this
	PX_INLINE uint32_t getHintValueSize() const;

	//Template for getting target alignment of T
	template <typename T> PX_INLINE uint32_t getAlignment() const;

	//Template for getting target size of T
	template <typename T> PX_INLINE uint32_t getSize() const;

private:
	uint32_t getNatAlignment(const Definition *pd) const;
};

#include "PlatformABI.inl"

}

#endif
