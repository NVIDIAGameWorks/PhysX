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

#include "PxSimpleTypes.h"
#include "SerializerCommon.h"
#include "NvTraitsInternal.h"

namespace NvParameterized
{

#define CHECK(x) NV_BOOL_ERR_CHECK_RETURN(x, 0)

bool UpgradeLegacyObjects(Serializer::DeserializedData &data, bool &isUpdated, Traits *t)
{
	isUpdated = false;

	for(uint32_t i = 0; i < data.size(); ++i)
	{
		Interface *obj = data[i];
		if( !obj )
			continue;

		Interface *newObj = UpgradeObject(*obj, isUpdated, t);
		if( !newObj )
		{
			NV_PARAM_TRAITS_WARNING(
				t,
				"Failed to upgrade object of class %s and version %u",
				obj->className(),
				(unsigned)obj->version());

			DEBUG_ALWAYS_ASSERT();

			for(uint32_t j = 0; j < data.size(); ++j)
				data[i]->destroy();

			data.init(0, 0);

			return false;
		}

		if( newObj != obj )
		{
			// Need to retain the name of the old object into the new object
			const char *name = obj->name();
			newObj->setName(name);
			obj->destroy();
			data[i] = newObj;
		}
	}

	return true;
}

bool UpgradeIncludedRefs(Handle &h, bool &isUpdated, Traits *t)
{
	const Definition *pd = h.parameterDefinition();

	switch( pd->type() )
	{
	case TYPE_ARRAY:
		{
			if( pd->child(0)->isSimpleType() )
				break;

			int32_t size;
			CHECK( NvParameterized::ERROR_NONE == h.getArraySize(size) );

			for(int32_t i = 0; i < size; ++i)
			{
				h.set(i);
				CHECK( UpgradeIncludedRefs(h, isUpdated, t) );
				h.popIndex();
			}

			break;
		}

	case TYPE_STRUCT:
		{
			if( pd->isSimpleType() )
				break;

			for(int32_t i = 0; i < pd->numChildren(); ++i)
			{
				h.set(i);
				CHECK( UpgradeIncludedRefs(h, isUpdated, t) );
				h.popIndex();
			}

			break;
		}

	case TYPE_REF:
		{
			if( !pd->isIncludedRef() )
				break;

			Interface *refObj = 0;
			h.getParamRef(refObj);

			if( !refObj ) // No reference there?
				break;

			Interface *newRefObj = UpgradeObject(*refObj, isUpdated, t);
			CHECK( newRefObj );

			if( newRefObj == refObj ) // No update?
				break;

			refObj->destroy();

			if( NvParameterized::ERROR_NONE != h.setParamRef(newRefObj) )
			{
				DEBUG_ALWAYS_ASSERT();
				newRefObj->destroy();
				return false;
			}

			break;
		}
	NV_PARAMETRIZED_NO_AGGREGATE_AND_REF_DATATYPE_LABELS 
	default:
		{
			break;
		}

	}

	return true;
}

bool UpgradeIncludedRefs(Interface &obj, bool &isUpdated, Traits *t)
{
	Handle h(obj, "");
	CHECK( h.isValid() );

	return UpgradeIncludedRefs(h, isUpdated, t);
}

Interface *UpgradeObject(Interface &obj, bool &isUpdated, Traits *t)
{
	const char *className = obj.className();

	Interface *newObj = &obj;

	if( obj.version() != t->getCurrentVersion(className) )
	{
		isUpdated = true;

		newObj = t->createNvParameterized(className);

		if( !newObj )
		{
			NV_PARAM_TRAITS_WARNING(t, "Failed to create object of class %s", className);
			DEBUG_ALWAYS_ASSERT();
			return 0;
		}

		if( !t->updateLegacyNvParameterized(obj, *newObj) )
		{
			NV_PARAM_TRAITS_WARNING(t, "Failed to upgrade object of class %s and version %u",
				className,
				(unsigned)obj.version() );
			newObj->destroy();
			return 0;
		}
	}

	if( !UpgradeIncludedRefs(*newObj, isUpdated, t) )
	{
		newObj->destroy();
		return 0;
	}

	return newObj;
}

void *serializerMemAlloc(uint32_t size, Traits *t)
{
	if( t )
		return t->alloc(size);
	else
	{
		DEBUG_ALWAYS_ASSERT(); // indicates a memory leak
		return ::malloc(size);
	}
}

void serializerMemFree(void *data, Traits *t)
{
	if( t )
		t->free(data);
	else
	{
		DEBUG_ALWAYS_ASSERT();
		::free(data);
	}
}

bool DoIgnoreChecksum(const NvParameterized::Interface &obj)
{
	// Most of our classes initially do not have classVersion field.
	// When it is finally added (e.g. after adding new version)
	// schema checksum changes and we get invalid "checksum not equal" warnings;
	// because of that we ignore checksum differences for all 0.0 classes.
	return 0 == obj.version();
}

} // namespace NvParameterized
