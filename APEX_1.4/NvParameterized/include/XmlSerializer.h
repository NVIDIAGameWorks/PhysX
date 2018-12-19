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

#ifndef PX_XML_SERIALIZER_H
#define PX_XML_SERIALIZER_H

//XML serialization (by John Ratcliff)

#include "nvparameterized/NvSerializer.h"
#include "AbstractSerializer.h"

#include "PsIOStream.h"

namespace NvParameterized
{

struct traversalState;

bool isXmlFormat(physx::PxFileBuf &stream);

// XML serializer implementation
class XmlSerializer : public AbstractSerializer
{
	ErrorType peekNumObjects(char *data, uint32_t len, uint32_t &numObjects);

	Serializer::ErrorType peekClassNames(physx::PxFileBuf &stream, char **classNames, uint32_t &numClassNames);

#ifndef WITHOUT_APEX_SERIALIZATION
	Serializer::ErrorType traverseParamDefTree(
		const Interface &obj,
		physx::PsIOStream &stream,
		traversalState &state,
		Handle &handle,
		bool printValues = true);

	Serializer::ErrorType emitElementNxHints(
		physx::PsIOStream &stream,
		Handle &handle,
		traversalState &state,
		bool &includedRef);

	Serializer::ErrorType emitElement(
		const Interface &obj,
		physx::PsIOStream &stream,
		const char *elementName,
		Handle &handle,
		bool includedRef,
		bool printValues,
		bool isRoot = false);
#endif

protected:

#ifndef WITHOUT_APEX_SERIALIZATION
	Serializer::ErrorType internalSerialize(physx::PxFileBuf &fbuf,const Interface **objs, uint32_t n, bool doMetadata);
#endif

	Serializer::ErrorType internalDeserialize(physx::PxFileBuf &stream, DeserializedData &res, bool &doesNeedUpdate);

public:

	XmlSerializer(Traits *traits): AbstractSerializer(traits) {}

	~XmlSerializer() {}

	virtual void release(void)
	{
		this->~XmlSerializer();
		serializerMemFree(this,mTraits);
	}

	PX_INLINE static uint32_t version()
	{
		return 0x00010000;
	}

	ErrorType peekNumObjectsInplace(const void * data, uint32_t dataLen, uint32_t & numObjects);

	ErrorType peekNumObjects(physx::PxFileBuf &stream, uint32_t &numObjects);

	using Serializer::deserializeMetadata;
	ErrorType deserializeMetadata(physx::PxFileBuf & /*stream*/, Definition ** /*defs*/, uint32_t & /*ndefs*/)
	{
		return Serializer::ERROR_NOT_IMPLEMENTED;
	}
};

} // namespace NvParameterized

#endif
