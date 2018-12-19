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



#ifndef APEX_VERTEX_BUFFER_H
#define APEX_VERTEX_BUFFER_H

#include "RenderMeshAssetIntl.h"
#include "ApexVertexFormat.h"
#include "VertexBufferParameters.h"
#include <nvparameterized/NvParameterized.h>
#include "ApexSharedUtils.h"
#include "ApexInteropableBuffer.h"

namespace nvidia
{
namespace apex
{

class ApexVertexBuffer : public VertexBufferIntl, public ApexInteropableBuffer, public NvParameterized::SerializationCallback
{
public:
	ApexVertexBuffer();
	~ApexVertexBuffer();

	// from VertexBuffer
	const VertexFormat&	getFormat() const
	{
		return mFormat;
	}
	uint32_t			getVertexCount() const
	{
		return mParams->vertexCount;
	}
	void*					getBuffer(uint32_t bufferIndex);
	void*					getBufferAndFormatWritable(RenderDataFormat::Enum& format, uint32_t bufferIndex)
	{
		return getBufferAndFormat(format, bufferIndex);
	}

	void*					getBufferAndFormat(RenderDataFormat::Enum& format, uint32_t bufferIndex)
	{
		format = getFormat().getBufferFormat(bufferIndex);
		return getBuffer(bufferIndex);
	}
	bool					getBufferData(void* dstBuffer, nvidia::RenderDataFormat::Enum dstBufferFormat, uint32_t dstBufferStride, uint32_t bufferIndex,
	                                      uint32_t startVertexIndex, uint32_t elementCount) const;
	PX_INLINE const void*	getBuffer(uint32_t bufferIndex) const
	{
		return (const void*)((ApexVertexBuffer*)this)->getBuffer(bufferIndex);
	}
	PX_INLINE const void*	getBufferAndFormat(RenderDataFormat::Enum& format, uint32_t bufferIndex) const
	{
		return (const void*)((ApexVertexBuffer*)this)->getBufferAndFormat(format, bufferIndex);
	}

	// from VertexBufferIntl
	void					build(const VertexFormat& format, uint32_t vertexCount);

	VertexFormat&			getFormatWritable()
	{
		return mFormat;
	}
	void					applyTransformation(const PxMat44& transformation);
	void					applyScale(float scale);
	bool					mergeBinormalsIntoTangents();

	void					copy(uint32_t dstIndex, uint32_t srcIndex, ApexVertexBuffer* srcBufferPtr = NULL);
	void					resize(uint32_t vertexCount);

	// from NvParameterized::SerializationCallback

	void					preSerialize(void* userData_);

	void					setParams(VertexBufferParameters* param);
	VertexBufferParameters* getParams()
	{
		return mParams;
	}

	uint32_t			getAllocationSize() const;

	void					applyPermutation(const Array<uint32_t>& permutation);

protected:
	VertexBufferParameters*			mParams;

	ApexVertexFormat				mFormat;	// Wrapper class for mParams->vertexFormat
};


} // namespace apex
} // namespace nvidia


#endif // APEX_VERTEX_BUFFER_H
