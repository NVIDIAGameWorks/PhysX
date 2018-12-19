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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.



#include "ApexVertexFormat.h"
#include "ApexSDKIntl.h"

#include <ParamArray.h>

namespace nvidia
{
namespace apex
{

// Local functions and definitions

PX_INLINE char* apex_strdup(const char* input)
{
	if (input == NULL)
	{
		return NULL;
	}

	size_t len = strlen(input);

	char* result = (char*)PX_ALLOC(sizeof(char) * (len + 1), PX_DEBUG_EXP("apex_strdup"));
#ifdef WIN32
	strncpy_s(result, len + 1, input, len);
#else
	strncpy(result, input, len);
#endif

	return result;
}

PX_INLINE uint32_t hash(const char* string)
{
	// "DJB" string hash
	uint32_t h = 5381;
	char c;
	while ((c = *string++) != '\0')
	{
		h = ((h << 5) + h) ^ c;
	}
	return h;
}

struct SemanticNameAndID
{
	SemanticNameAndID(const char* name, VertexFormat::BufferID id) : m_name(name), m_id(id)
	{
		PX_ASSERT(m_id != 0 || nvidia::strcmp(m_name, "SEMANTIC_INVALID") == 0);
	}
	const char*					m_name;
	VertexFormat::BufferID	m_id;
};

#define SEMANTIC_NAME_AND_ID( name )	SemanticNameAndID( name, (VertexFormat::BufferID)hash( name ) )

static const SemanticNameAndID sSemanticNamesAndIDs[] =
{
	SEMANTIC_NAME_AND_ID("SEMANTIC_POSITION"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_NORMAL"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_TANGENT"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_BINORMAL"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_COLOR"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_TEXCOORD0"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_TEXCOORD1"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_TEXCOORD2"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_TEXCOORD3"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_BONE_INDEX"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_BONE_WEIGHT"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_DISPLACEMENT_TEXCOORD"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_DISPLACEMENT_FLAGS"),

	SemanticNameAndID("SEMANTIC_INVALID", (VertexFormat::BufferID)0)
};


// VertexFormat implementation
void ApexVertexFormat::reset()
{
	if (mParams != NULL)
	{
		mParams->winding = 0;
		mParams->hasSeparateBoneBuffer = 0;
	}
	clearBuffers();
}

void ApexVertexFormat::setWinding(RenderCullMode::Enum winding)
{
	mParams->winding = winding;
}

void ApexVertexFormat::setHasSeparateBoneBuffer(bool hasSeparateBoneBuffer)
{
	mParams->hasSeparateBoneBuffer = hasSeparateBoneBuffer;
}

RenderCullMode::Enum ApexVertexFormat::getWinding() const
{
	return (RenderCullMode::Enum)mParams->winding;
}

bool ApexVertexFormat::hasSeparateBoneBuffer() const
{
	return mParams->hasSeparateBoneBuffer;
}

const char* ApexVertexFormat::getSemanticName(RenderVertexSemantic::Enum semantic) const
{
	PX_ASSERT((uint32_t)semantic < RenderVertexSemantic::NUM_SEMANTICS);
	return (uint32_t)semantic < RenderVertexSemantic::NUM_SEMANTICS ? sSemanticNamesAndIDs[semantic].m_name : NULL;
}

VertexFormat::BufferID ApexVertexFormat::getSemanticID(RenderVertexSemantic::Enum semantic) const
{
	PX_ASSERT((uint32_t)semantic < RenderVertexSemantic::NUM_SEMANTICS);
	return (uint32_t)semantic < RenderVertexSemantic::NUM_SEMANTICS ? sSemanticNamesAndIDs[semantic].m_id : (BufferID)0;
}

VertexFormat::BufferID ApexVertexFormat::getID(const char* name) const
{
	if (name == NULL)
	{
		return (BufferID)0;
	}
	const BufferID id = hash(name);
	return id ? id : (BufferID)1;	// We reserve 0 for an invalid ID
}

int32_t ApexVertexFormat::addBuffer(const char* name)
{
	if (name == NULL)
	{
		return -1;
	}

	const BufferID id = getID(name);

	int32_t index = getBufferIndexFromID(id);
	if (index >= 0)
	{
		return index;
	}

	int32_t semantic = 0;
	for (; semantic < RenderVertexSemantic::NUM_SEMANTICS; ++semantic)
	{
		if (getSemanticID((RenderVertexSemantic::Enum)semantic) == id)
		{
			break;
		}
	}
	if (semantic == RenderVertexSemantic::NUM_SEMANTICS)
	{
		semantic = RenderVertexSemantic::CUSTOM;
	}

	NvParameterized::Handle handle(*mParams);
	mParams->getParameterHandle("bufferFormats", handle);

	mParams->getArraySize(handle, index);

	mParams->resizeArray(handle, index + 1);

	NvParameterized::Handle elementHandle(*mParams);
	handle.getChildHandle(index, elementHandle);
	NvParameterized::Handle subElementHandle(*mParams);
	elementHandle.getChildHandle(mParams, "name", subElementHandle);
	mParams->setParamString(subElementHandle, name);
	elementHandle.getChildHandle(mParams, "semantic", subElementHandle);
	mParams->setParamI32(subElementHandle, semantic);
	elementHandle.getChildHandle(mParams, "id", subElementHandle);
	mParams->setParamU32(subElementHandle, (uint32_t)id);
	elementHandle.getChildHandle(mParams, "format", subElementHandle);
	mParams->setParamU32(subElementHandle, (uint32_t)RenderDataFormat::UNSPECIFIED);
	elementHandle.getChildHandle(mParams, "access", subElementHandle);
	mParams->setParamU32(subElementHandle, (uint32_t)RenderDataAccess::STATIC);
	elementHandle.getChildHandle(mParams, "serialize", subElementHandle);
	mParams->setParamBool(subElementHandle, true);

	return index;
}

bool ApexVertexFormat::bufferReplaceWithLast(uint32_t index)
{
	PX_ASSERT((int32_t)index < mParams->bufferFormats.arraySizes[0]);
	if ((int32_t)index < mParams->bufferFormats.arraySizes[0])
	{
		ParamArray<VertexFormatParametersNS::BufferFormat_Type> bufferFormats(mParams, "bufferFormats", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->bufferFormats));
		bufferFormats.replaceWithLast(index);
		return true;
	}

	return false;
}

bool ApexVertexFormat::setBufferFormat(uint32_t index, RenderDataFormat::Enum format)
{
	if (index < getBufferCount())
	{
		mParams->bufferFormats.buf[index].format = format;
		return true;
	}

	return false;
}

bool ApexVertexFormat::setBufferAccess(uint32_t index, RenderDataAccess::Enum access)
{
	if (index < getBufferCount())
	{
		mParams->bufferFormats.buf[index].access = access;
		return true;
	}

	return false;
}

bool ApexVertexFormat::setBufferSerialize(uint32_t index, bool serialize)
{
	if (index < getBufferCount())
	{
		mParams->bufferFormats.buf[index].serialize = serialize;
		return true;
	}

	return false;
}

const char* ApexVertexFormat::getBufferName(uint32_t index) const
{
	return index < getBufferCount() ? (const char*)mParams->bufferFormats.buf[index].name : NULL;
}

RenderVertexSemantic::Enum ApexVertexFormat::getBufferSemantic(uint32_t index) const
{
	return index < getBufferCount() ? (RenderVertexSemantic::Enum)mParams->bufferFormats.buf[index].semantic : RenderVertexSemantic::NUM_SEMANTICS;
}

VertexFormat::BufferID ApexVertexFormat::getBufferID(uint32_t index) const
{
	return index < getBufferCount() ? (BufferID)mParams->bufferFormats.buf[index].id : (BufferID)0;
}

RenderDataFormat::Enum ApexVertexFormat::getBufferFormat(uint32_t index) const
{
	return index < getBufferCount() ? (RenderDataFormat::Enum)mParams->bufferFormats.buf[index].format : RenderDataFormat::UNSPECIFIED;
}

RenderDataAccess::Enum ApexVertexFormat::getBufferAccess(uint32_t index) const
{
	return index < getBufferCount() ? (RenderDataAccess::Enum)mParams->bufferFormats.buf[index].access : RenderDataAccess::ACCESS_TYPE_COUNT;
}

bool ApexVertexFormat::getBufferSerialize(uint32_t index) const
{
	return index < getBufferCount() ? mParams->bufferFormats.buf[index].serialize : false;
}

uint32_t ApexVertexFormat::getBufferCount() const
{
	return (uint32_t)mParams->bufferFormats.arraySizes[0];
}

uint32_t ApexVertexFormat::getCustomBufferCount() const
{
	PX_ASSERT(mParams != NULL);
	uint32_t customBufferCount = 0;
	for (int32_t i = 0; i < mParams->bufferFormats.arraySizes[0]; ++i)
	{
		if (mParams->bufferFormats.buf[i].semantic == RenderVertexSemantic::CUSTOM)
		{
			++customBufferCount;
		}
	}
	return customBufferCount;
}

int32_t ApexVertexFormat::getBufferIndexFromID(BufferID id) const
{
	for (int32_t i = 0; i < mParams->bufferFormats.arraySizes[0]; ++i)
	{
		if (mParams->bufferFormats.buf[i].id == (uint32_t)id)
		{
			return i;
		}
	}

	return -1;
}



// ApexVertexFormat functions

ApexVertexFormat::ApexVertexFormat()
{
	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
	mParams = DYNAMIC_CAST(VertexFormatParameters*)(traits->createNvParameterized(VertexFormatParameters::staticClassName()));
	mOwnsParams = mParams != NULL;
}

ApexVertexFormat::ApexVertexFormat(VertexFormatParameters* params) : mParams(params), mOwnsParams(false)
{
}

ApexVertexFormat::ApexVertexFormat(const ApexVertexFormat& f) : VertexFormat(f)
{
	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
	mParams = DYNAMIC_CAST(VertexFormatParameters*)(traits->createNvParameterized(VertexFormatParameters::staticClassName()));
	mOwnsParams = mParams != NULL;
	if (mParams)
	{
		copy(f);
	}
}

ApexVertexFormat::~ApexVertexFormat()
{
	if (mOwnsParams && mParams != NULL)
	{
		mParams->destroy();
	}
}

bool ApexVertexFormat::operator == (const VertexFormat& format) const
{
	if (getWinding() != format.getWinding())
	{
		return false;
	}

	if (hasSeparateBoneBuffer() != format.hasSeparateBoneBuffer())
	{
		return false;
	}

	if (getBufferCount() != format.getBufferCount())
	{
		return false;
	}

	for (uint32_t thisIndex = 0; thisIndex < getBufferCount(); ++thisIndex)
	{
		BufferID id = getBufferID(thisIndex);
		const int32_t thatIndex = format.getBufferIndexFromID(id);
		if (thatIndex < 0)
		{
			return false;
		}
		if (getBufferFormat(thisIndex) != format.getBufferFormat((uint32_t)thatIndex))
		{
			return false;
		}
		if (getBufferAccess(thisIndex) != format.getBufferAccess((uint32_t)thatIndex))
		{
			return false;
		}
	}

	return true;
}

void ApexVertexFormat::copy(const ApexVertexFormat& other)
{
	reset();

	setWinding(other.getWinding());
	setHasSeparateBoneBuffer(other.hasSeparateBoneBuffer());

	for (uint32_t i = 0; i < other.getBufferCount(); ++i)
	{
		const char* name = other.getBufferName(i);
		const uint32_t index = (uint32_t)addBuffer(name);
		setBufferFormat(index, other.getBufferFormat(i));
		setBufferAccess(index, other.getBufferAccess(i));
		setBufferSerialize(index, other.getBufferSerialize(i));
	}
}

void ApexVertexFormat::clearBuffers()
{
	if (mParams)
	{
		NvParameterized::Handle handle(*mParams);

		mParams->getParameterHandle("bufferFormats", handle);
		handle.resizeArray(0);
	}
}


}
} // end namespace nvidia::apex
