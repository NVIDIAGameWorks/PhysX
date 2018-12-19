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

#ifndef XML_DESERIALIZER_H_
#define XML_DESERIALIZER_H_

//XML deserialization (by John Ratcliff)

#include "PsFastXml.h"

#include "nvparameterized/NvSerializer.h"
#include "nvparameterized/NvParameterized.h"
#include "nvparameterized/NvParameterizedTraits.h"

#include "SerializerCommon.h"

namespace NvParameterized
{

typedef enum
{
	ARRAY,
	STRUCT,
	VALUE,
	SKIP
} FieldType;

struct FieldInfo
{
	PX_INLINE void init(const char *name_, FieldType type_)
	{
		name = name_;
		type = type_;
		idx = 0;
	}

	const char *name;
	FieldType type;
	uint32_t idx; //For arrays
};

class ObjectInfo
{
	static const uint32_t MAX_STRUCT_FIELD_STACK = 256;

	Interface *mObj;

	//Field stack
	uint32_t mIndex;
	FieldInfo mFields[MAX_STRUCT_FIELD_STACK];

public:

	PX_INLINE ObjectInfo(): mObj(0), mIndex(0) {}

	PX_INLINE void init(Interface *obj)
	{
		mObj = obj;
	}

	PX_INLINE Interface *getObject()
	{
		return mObj;
	}

	PX_INLINE bool popField(const char *&name, FieldType &type)
	{
		if( !mIndex )
		{
			DEBUG_ALWAYS_ASSERT();
			return false;
		}

		FieldInfo &field = mFields[--mIndex];
		name = field.name;
		type = field.type;

#		ifndef NDEBUG
		field.name = 0;
#		endif

		if( mIndex )
		{
			FieldInfo &lastField = mFields[mIndex-1];
			if( ARRAY == lastField.type )
				++lastField.idx;
		}

		return true;
	}

	PX_INLINE void pushField(const char *name, FieldType type)
	{
		PX_ASSERT( mIndex < MAX_STRUCT_FIELD_STACK );

		if( mIndex < MAX_STRUCT_FIELD_STACK )
			mFields[mIndex++].init(name, type);
	}

	PX_INLINE uint32_t getIndex() const { return mIndex; };

	PX_INLINE FieldInfo &getFieldInfo(uint32_t i)
	{
		PX_ASSERT( i < mIndex );
		return mFields[i];
	}
};

class XmlDeserializer: public physx::shdfnd::FastXml::Callback
{
	static const uint32_t MAX_REF_STACK = 8,
		MAX_ROOT_OBJ = 64;

	Serializer::ErrorType mError;

	Traits *mTraits;

	//Object stack
	uint32_t mObjIndex;
	ObjectInfo mObjects[MAX_REF_STACK];

	//Array of root objects
	uint32_t mRootIndex;
	Interface *mRootObjs[MAX_ROOT_OBJ];

	//Check errors in <NvParameters>
	uint32_t mRootTags;
	bool mInRootElement;

	//Check DOCTYPE
	bool mHasDoctype;

	uint32_t mVer;

	// read simple structs in array
	int32_t* mSimpleStructRedirect;
	uint32_t mSimpleStructRedirectSize;

	//Top Of Stack
	PX_INLINE ObjectInfo &tos()
	{
		PX_ASSERT( mObjIndex >= 1 && mObjIndex <= MAX_REF_STACK );
		return mObjIndex > 0 ? mObjects[mObjIndex - 1] : mObjects[0];
	}

	PX_INLINE void pushObj(Interface *obj)
	{
		if( mObjIndex >= MAX_REF_STACK )
		{
			PX_ALWAYS_ASSERT(); //included references nested too deeply
			return;
		}

		++mObjIndex;
		tos().init(obj);
	}

	PX_INLINE bool popObj()
	{
		if( mObjIndex <= 0 )
			return false;

		--mObjIndex;

		return true;
	}

	PX_INLINE void pushField(const char *name, FieldType type)
	{
		tos().pushField(mTraits->strdup(name), type);
	}

	PX_INLINE bool popField()
	{
		const char *name = 0;
		FieldType type;
		if( !tos().popField(name, type) )
			return false;

		mTraits->strfree(const_cast<char *>(name));

		return true;
	}

	bool verifyObject(Interface *obj, const physx::shdfnd::FastXml::AttributePairs& attr);

	bool initAddressString(char *dest, uint32_t len, const char *name);

public:

	PX_INLINE XmlDeserializer(Traits *traits, uint32_t ver):
		mError(Serializer::ERROR_NONE),
		mTraits(traits),
		mObjIndex(0),
		mRootIndex(0),
		mRootTags(0),
		mInRootElement(false),
		mHasDoctype(false),
		mVer(ver),
		mSimpleStructRedirect(NULL),
		mSimpleStructRedirectSize(0) {}

	PX_INLINE virtual ~XmlDeserializer()
	{
		if (mSimpleStructRedirect != NULL)
		{
			mTraits->free(mSimpleStructRedirect);
		}
		mSimpleStructRedirect = NULL;
		mSimpleStructRedirectSize = 0;
	}

	static PX_INLINE XmlDeserializer *Create(Traits *traits, uint32_t ver)
	{
		char *buf = (char *)serializerMemAlloc(sizeof(XmlDeserializer), traits);
		return PX_PLACEMENT_NEW(buf, XmlDeserializer)(traits, ver);		
	}

	PX_INLINE void destroy()
	{
		Traits *traits = mTraits;
		this->~XmlDeserializer();
		serializerMemFree(this, traits);
	}

	PX_INLINE Serializer::ErrorType getLastError() const
	{
		return mError;
	}

	PX_INLINE Interface **getObjs()
	{
		return mRootObjs;
	}

	PX_INLINE uint32_t getNobjs() const
	{
		return physx::PxMin(mRootIndex, MAX_ROOT_OBJ);
	}

	//Release all created objects (in case of error)
	PX_INLINE void releaseAll()
	{
		for(uint32_t i = 0; i < getNobjs(); ++i)
			mRootObjs[i]->destroy();
	}

	virtual bool processComment(const char *)
	{
		return true;
	}

	virtual bool processDoctype(const char * rootElement, const char *, const char *, const char *)
	{
		mHasDoctype = true;
		return 0 == ::strcmp(rootElement, "NvParameters") || 0 == ::strcmp(rootElement, "NxParameters");
	}

	virtual void *allocate(uint32_t size)
	{
		return mTraits->alloc(size);
	}

	virtual void deallocate(void *ptr) 
	{
		mTraits->free(ptr);
	}

	virtual bool processClose(const char *tag,uint32_t depth,bool &isError);

	virtual bool processElement(
		const char *elementName,
		const char *elementData,
		const physx::shdfnd::FastXml::AttributePairs& attr,
		int32_t /*lineno*/
	);

	int32_t* getSimpleStructRedirect(uint32_t size);
};

}

#endif
