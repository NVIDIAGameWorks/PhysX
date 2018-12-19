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



#ifndef __APEX_CUSTOM_BUFFER_ITERARTOR_H__
#define __APEX_CUSTOM_BUFFER_ITERARTOR_H__

#include "CustomBufferIterator.h"
#include <PsUserAllocated.h>
#include <PsArray.h>
#include <ApexUsingNamespace.h>

namespace nvidia
{
namespace apex
{

class ApexCustomBufferIterator : public CustomBufferIterator, public UserAllocated
{
public:
	ApexCustomBufferIterator();

	// CustomBufferIterator methods

	virtual void		setData(void* data, uint32_t elemSize, uint32_t maxTriangles);

	virtual void		addCustomBuffer(const char* name, RenderDataFormat::Enum format, uint32_t offset);

	virtual void*		getVertex(uint32_t triangleIndex, uint32_t vertexIndex) const;

	virtual int32_t		getAttributeIndex(const char* attributeName) const;

	virtual void*		getVertexAttribute(uint32_t triangleIndex, uint32_t vertexIndex, const char* attributeName, RenderDataFormat::Enum& outFormat) const;

	virtual void*		getVertexAttribute(uint32_t triangleIndex, uint32_t vertexIndex, uint32_t attributeIndex, RenderDataFormat::Enum& outFormat, const char*& outName) const;

private:
	uint8_t* mData;
	uint32_t mElemSize;
	uint32_t mMaxTriangles;
	struct CustomBuffer
	{
		const char* name;
		uint32_t offset;
		RenderDataFormat::Enum format;
	};
	physx::Array<CustomBuffer> mCustomBuffers;
};

}
} // end namespace nvidia::apex


#endif // __APEX_CUSTOM_BUFFER_ITERARTOR_H__
