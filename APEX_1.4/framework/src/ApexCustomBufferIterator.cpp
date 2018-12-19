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



#include "ApexCustomBufferIterator.h"
#include "PsString.h"

namespace nvidia
{
namespace apex
{

ApexCustomBufferIterator::ApexCustomBufferIterator() :
	mData(NULL),
	mElemSize(0),
	mMaxTriangles(0)
{
}

void ApexCustomBufferIterator::setData(void* data, uint32_t elemSize, uint32_t maxTriangles)
{
	mData = (uint8_t*)data;
	mElemSize = elemSize;
	mMaxTriangles = maxTriangles;
}

void ApexCustomBufferIterator::addCustomBuffer(const char* name, RenderDataFormat::Enum format, uint32_t offset)
{
	CustomBuffer buffer;
	buffer.name = name;
	buffer.offset = offset;
	buffer.format = format;

	mCustomBuffers.pushBack(buffer);
}
void* ApexCustomBufferIterator::getVertex(uint32_t triangleIndex, uint32_t vertexIndex) const
{
	if (mData == NULL || triangleIndex >= mMaxTriangles)
	{
		return NULL;
	}

	return mData + mElemSize * (triangleIndex * 3 + vertexIndex);
}
int32_t ApexCustomBufferIterator::getAttributeIndex(const char* attributeName) const
{
	if (attributeName == NULL || attributeName[0] == 0)
	{
		return -1;
	}

	for (uint32_t i = 0; i < mCustomBuffers.size(); i++)
	{
		if (nvidia::strcmp(mCustomBuffers[i].name, attributeName) == 0)
		{
			return (int32_t)i;
		}
	}
	return -1;
}
void* ApexCustomBufferIterator::getVertexAttribute(uint32_t triangleIndex, uint32_t vertexIndex, const char* attributeName, RenderDataFormat::Enum& outFormat) const
{
	outFormat = RenderDataFormat::UNSPECIFIED;

	uint8_t* elementData = (uint8_t*)getVertex(triangleIndex, vertexIndex);
	if (elementData == NULL)
	{
		return NULL;
	}


	for (uint32_t i = 0; i < mCustomBuffers.size(); i++)
	{
		if (nvidia::strcmp(mCustomBuffers[i].name, attributeName) == 0)
		{
			outFormat = mCustomBuffers[i].format;
			return elementData + mCustomBuffers[i].offset;
		}
	}
	return NULL;
}

void* ApexCustomBufferIterator::getVertexAttribute(uint32_t triangleIndex, uint32_t vertexIndex, uint32_t attributeIndex, RenderDataFormat::Enum& outFormat, const char*& outName) const
{
	outFormat = RenderDataFormat::UNSPECIFIED;
	outName = NULL;

	uint8_t* elementData = (uint8_t*)getVertex(triangleIndex, vertexIndex);
	if (elementData == NULL || attributeIndex >= mCustomBuffers.size())
	{
		return NULL;
	}

	outName = mCustomBuffers[attributeIndex].name;
	outFormat = mCustomBuffers[attributeIndex].format;
	return elementData + mCustomBuffers[attributeIndex].offset;
}

}
} // end namespace nvidia::apex
