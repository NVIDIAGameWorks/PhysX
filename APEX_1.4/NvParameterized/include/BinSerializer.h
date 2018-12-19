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

#ifndef PX_BIN_SERIALIZER_H
#define PX_BIN_SERIALIZER_H

// APB serializer

// WARNING: before doing any changes to this file
// check comments at the head of BinSerializer.cpp

#include "nvparameterized/NvParameterized.h"
#include "nvparameterized/NvSerializer.h"

#include "AbstractSerializer.h"

#include "ApbDefinitions.h"

namespace NvParameterized
{

class DefinitionImpl;
class PlatformInputStream;
class PlatformOutputStream;
struct PlatformABI;

class BinSerializer : public AbstractSerializer
{
	// Methods for updating legacy formats
	Serializer::ErrorType updateInitial2AllCounted(BinaryHeader &hdr, char *start);
	Serializer::ErrorType updateAllCounted2WithAlignment(BinaryHeader &hdr, char *start);

	Serializer::ErrorType readMetadataInfo(const BinaryHeader &hdr, PlatformInputStream &s, DefinitionImpl *def);

	// Read array of arbitrary type (slow version)
	Serializer::ErrorType readArraySlow(Handle &handle, PlatformInputStream &s);

	// Read NvParameterized object data
	Serializer::ErrorType readObject(NvParameterized::Interface *&obj, PlatformInputStream &data);

	// Read binary data of NvParameterized object addressed by handle
	Serializer::ErrorType readBinaryData(Handle &handle, PlatformInputStream &data);

#ifndef WITHOUT_APEX_SERIALIZATION
	Serializer::ErrorType storeMetadataInfo(const Definition *def, PlatformOutputStream &s);

	// Store array of arbitrary type (slow version)
	Serializer::ErrorType storeArraySlow(Handle &handle, PlatformOutputStream &s);

	// Print binary data for part of NvParameterized object addressed by handle
	Serializer::ErrorType storeBinaryData(const NvParameterized::Interface &obj, Handle &handle, PlatformOutputStream &res, bool isRootObject = true);
#endif

	BinSerializer(BinSerializer &); // Don't
	void operator=(BinSerializer &); // Don't

	Serializer::ErrorType verifyFileHeader(
		const BinaryHeader &hdr,
		const BinaryHeaderExt *ext,
		uint32_t dataLen ) const;

	Serializer::ErrorType getPlatformInfo(
		BinaryHeader &hdr,
		BinaryHeaderExt *ext,
		PlatformABI &abi ) const;

	Serializer::ErrorType verifyObjectHeader(const ObjHeader &hdr, const Interface *obj, Traits *traits) const;

protected:

	Serializer::ErrorType internalDeserialize(physx::PxFileBuf &stream, Serializer::DeserializedData &res, bool &doesNeedUpdate);
	Serializer::ErrorType internalDeserializeInplace(void *mdata, uint32_t dataLen, Serializer::DeserializedData &res, bool &doesNeedUpdate);

#ifndef WITHOUT_APEX_SERIALIZATION
	Serializer::ErrorType internalSerialize(physx::PxFileBuf &stream,const NvParameterized::Interface **objs, uint32_t nobjs, bool doMetadata);
#endif

public:
	BinSerializer(Traits *traits): AbstractSerializer(traits) {}

	void release()
	{
		Traits *t = mTraits;
		this->~BinSerializer();
		serializerMemFree(this, t);
	}

	static const uint32_t Magic = APB_MAGIC;
	static const uint32_t Version = BinVersions::WithExtendedHeader;

	Serializer::ErrorType peekNumObjects(physx::PxFileBuf &stream, uint32_t &numObjects);

	Serializer::ErrorType peekNumObjectsInplace(const void *data, uint32_t dataLen, uint32_t &numObjects);

	Serializer::ErrorType peekClassNames(physx::PxFileBuf &stream, char **classNames, uint32_t &numClassNames);

	Serializer::ErrorType peekInplaceAlignment(physx::PxFileBuf& stream, uint32_t& align);

	Serializer::ErrorType deserializeMetadata(physx::PxFileBuf &stream, DeserializedMetadata &desData);
};

bool isBinaryFormat(physx::PxFileBuf &stream);

Serializer::ErrorType peekBinaryPlatform(physx::PxFileBuf &stream, SerializePlatform &platform);

} // namespace NvParameterized

#endif
