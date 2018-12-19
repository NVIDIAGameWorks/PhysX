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

#ifndef APB_DEFINITIONS_H_
#define APB_DEFINITIONS_H_

// This file contains definitions of various parts of APB file format

// WARNING: before doing any changes to this file
// check comments at the head of BinSerializer.cpp

#include "SerializerCommon.h"
#include "BinaryHelper.h"

namespace NvParameterized
{

#define APB_MAGIC 0x5A5B5C5D

namespace BinVersions
{
	static const uint32_t Initial = 0x00010000,
		AllRefsCounted = 0x00010001,
		WithAlignment = 0x00010002,
		WithExtendedHeader = 0x00010003;
}

// Type of relocation in binary file
enum RelocType
{
	//Raw bytes
	RELOC_ABS_RAW = 0,

	//NvParameterized (will be initialized on deserialization)
	RELOC_ABS_REF,

	RELOC_LAST
};

// Relocation record in binary file
struct BinaryReloc
{
	uint32_t type;
	uint32_t off;
};

// Format of data in binary file; only BINARY_TYPE_PLAIN is used for now
enum BinaryType
{
	BINARY_TYPE_PLAIN = 0,
	BINARY_TYPE_XML_GZ,
	BINARY_TYPE_LAST
};

// Some dummy version control systems insert '\r' before '\n'.
// This short string in header is used to catch this.
// We also use 0xff byte to guarantee that we have non-printable chars
// (s.t. VCS can detect that file is binary).
#define VCS_SAFETY_FLAGS "ab\n\xff"

// File header
#pragma pack(push,1) // For cross-platform compatibility!

// Convert to platform-independent format
static void CanonizeArrayOfU32s(char *data, uint32_t len)
{
	PX_ASSERT(len % 4U == 0);

	if( IsBigEndian() )
		return;

	for(uint32_t i = 0; i < len; i += 4U)
		SwapBytes(data + i, 4U, TYPE_U32);
}

// Main binary header
struct BinaryHeader
{
	uint32_t magic;
	uint32_t type;
	uint32_t version;
	int32_t numObjects;

	uint32_t fileLength;
	uint32_t dictOffset;
	uint32_t dataOffset;
	uint32_t relocOffset;

	uint32_t metadataOffset;
	uint32_t archType;
	uint32_t compilerType;
	uint32_t compilerVer;

	uint32_t osType;
	uint32_t osVer;
	uint32_t numMetadata;
	uint32_t alignment;

	static bool CheckAlignment()
	{
		bool isPushPackOk = 4 != offsetof(BinaryHeader, type);
		if( isPushPackOk )
		{
			DEBUG_ASSERT( 0 && "PX_PUSH_PACK failed!" );
			return false;
		}
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127)
#endif
		if( sizeof(BinaryHeader) % 16 != 0 )
		{
			return false;
		}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

		return true;
	}

	Serializer::ErrorType getPlatform(SerializePlatform &platform) const
	{
		if( archType >= SerializePlatform::ARCH_LAST
				|| compilerType >= SerializePlatform::COMP_LAST
				|| osType >= SerializePlatform::OS_LAST )
		{
			DEBUG_ALWAYS_ASSERT();
			return Serializer::ERROR_INVALID_PLATFORM;
		}
		
		platform = SerializePlatform(
			static_cast<SerializePlatform::ArchType>(archType),
			static_cast<SerializePlatform::CompilerType>(compilerType),
			compilerVer,
			static_cast<SerializePlatform::OsType>(osType),
			osVer
		);

		return Serializer::ERROR_NONE;
	}

	void canonize()
	{
		CanonizeArrayOfU32s((char*)this, sizeof(BinaryHeader));
	}

	void decanonize() { canonize(); }
};

// Extended header (only in new versions)
struct BinaryHeaderExt
{
	uint32_t vcsSafetyFlags;
	uint32_t res[3 + 8]; // Pad to multiple of 16 byte

	void canonize()
	{
		// vcsSafetyFlags should be stored as-is
	}

	void decanonize()
	{
		// vcsSafetyFlags should be stored as-is
	}
};

#pragma pack(pop)

// NvParameterized object header
struct ObjHeader
{
	uint32_t dataOffset;
	const char *className;
	const char *name;
	bool isIncluded;
	uint32_t version;
	uint32_t checksumSize;
	const uint32_t *checksum;
};

// Element of root references table
struct ObjectTableEntry
{
	Interface *obj;
	const char *className;
	const char *name;
	const char *filename;
};

} // namespace NvParameterized

#endif
