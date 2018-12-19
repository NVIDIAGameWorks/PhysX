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

#ifndef PX_ABSTRACT_SERIALIZER_H
#define PX_ABSTRACT_SERIALIZER_H

#include "nvparameterized/NvParameterized.h"
#include "nvparameterized/NvParameterizedTraits.h"
#include "nvparameterized/NvSerializer.h"

#include "NvSerializerInternal.h"
#include "NvTraitsInternal.h"

#include "SerializerCommon.h"

namespace NvParameterized
{

// Base for other serializers which takes care of common stuff

class AbstractSerializer : public Serializer
{
public:
	AbstractSerializer(Traits *traits):
		mDoUpdate(true),
		mPlatform(GetCurrentPlatform()),
		mTraits(traits) {}

	virtual ~AbstractSerializer() {}

	Traits *getTraits() const { return mTraits; }

	//This is used in static Serializer::deserializer
	void setTraits(Traits *traits) { mTraits = traits; }

	void setAutoUpdate(bool doUpdate)
	{
		mDoUpdate = doUpdate;
	}

	Serializer::ErrorType peekInplaceAlignment(physx::PxFileBuf& /*stream*/, uint32_t& /*align*/)
	{
		return Serializer::ERROR_NOT_IMPLEMENTED;
	}

	Serializer::ErrorType setTargetPlatform(const SerializePlatform &platform)
	{
		mPlatform = platform;
		return Serializer::ERROR_NONE; //Only pdb cares about platforms
	}

	Serializer::ErrorType serialize(physx::PxFileBuf &stream,const NvParameterized::Interface **objs, uint32_t nobjs, bool doMetadata)
	{
#ifdef WITHOUT_APEX_SERIALIZATION
		PX_UNUSED(stream);
		PX_UNUSED(objs);
		PX_UNUSED(nobjs);
		PX_UNUSED(doMetadata);

		return Serializer::ERROR_NOT_IMPLEMENTED;
#else

		NV_BOOL_ERR_CHECK_WARN_RETURN(
			stream.isOpen(),
			Serializer::ERROR_STREAM_ERROR,
			"Stream not opened" );

		for(uint32_t i = 0; i < nobjs; ++i)
		{
			NV_BOOL_ERR_CHECK_WARN_RETURN(
				objs[i]->callPreSerializeCallback() == 0,
				Serializer::ERROR_PRESERIALIZE_FAILED,
				"Preserialize callback failed" );
		}

		return internalSerialize(stream, objs, nobjs, doMetadata);
#endif
	}
	
	using Serializer::deserialize;
	virtual Serializer::ErrorType deserialize(physx::PxFileBuf &stream, Serializer::DeserializedData &res, bool &isUpdated)
	{
		NV_BOOL_ERR_CHECK_WARN_RETURN(
			stream.isOpen(),
			Serializer::ERROR_STREAM_ERROR,
			"Stream not opened" );

		isUpdated = false;
		bool doesNeedUpdate = true;
		NV_ERR_CHECK_RETURN( internalDeserialize(stream, res, doesNeedUpdate) );
		return doesNeedUpdate && mDoUpdate ? upgrade(res, isUpdated) : Serializer::ERROR_NONE;
	}

	using Serializer::deserializeInplace;
	Serializer::ErrorType deserializeInplace(void *data, uint32_t dataLen, Serializer::DeserializedData &res, bool &isUpdated)
	{
		isUpdated = false;
		bool doesNeedUpdate = true;
		NV_ERR_CHECK_RETURN( internalDeserializeInplace(data, dataLen, res, doesNeedUpdate) );
		return doesNeedUpdate && mDoUpdate ? upgrade(res, isUpdated) : Serializer::ERROR_NONE;
	}

protected:

	bool mDoUpdate;
	SerializePlatform mPlatform;
	Traits *mTraits;

#ifndef WITHOUT_APEX_SERIALIZATION
	virtual Serializer::ErrorType internalSerialize(
		physx::PxFileBuf &stream,
		const NvParameterized::Interface **objs,
		uint32_t n,
		bool doMetadata) = 0;
#endif

	// doesNeedUpdate allows serializer to avoid costly depth-first scanning of included refs
	virtual Serializer::ErrorType internalDeserialize(
		physx::PxFileBuf &stream,
		Serializer::DeserializedData &res,
		bool &doesNeedUpdate) = 0;

	// See note for internalDeserialize
	virtual Serializer::ErrorType internalDeserializeInplace(
		void * /*data*/,
		uint32_t /*dataLen*/,
		Serializer::DeserializedData & /*res*/,
		bool & /*doesNeedUpdate*/)
	{
		DEBUG_ALWAYS_ASSERT();
		return Serializer::ERROR_NOT_IMPLEMENTED;
	}

private:
	Serializer::ErrorType upgrade(Serializer::DeserializedData &res, bool &isUpdated)
	{
		//Upgrade legacy objects
		NV_BOOL_ERR_CHECK_WARN_RETURN(
			UpgradeLegacyObjects(res, isUpdated, mTraits),
			Serializer::ERROR_CONVERSION_FAILED,
			"Upgrading legacy objects failed" );

		return Serializer::ERROR_NONE;
	}
};

} // namespace NvParameterized

#endif
