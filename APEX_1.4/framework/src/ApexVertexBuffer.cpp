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



#include "ApexVertexBuffer.h"

#include "ApexSDKIntl.h"

#include "VertexFormatParameters.h"

#include <ParamArray.h>

#include "PsMemoryBuffer.h"
#include "Cof44.h"

#include "BufferU8x1.h"
#include "BufferU8x2.h"
#include "BufferU8x3.h"
#include "BufferU8x4.h"
#include "BufferU16x1.h"
#include "BufferU16x2.h"
#include "BufferU16x3.h"
#include "BufferU16x4.h"
#include "BufferU32x1.h"
#include "BufferU32x2.h"
#include "BufferU32x3.h"
#include "BufferU32x4.h"
#include "BufferF32x1.h"
#include "BufferF32x2.h"
#include "BufferF32x3.h"
#include "BufferF32x4.h"

#include "ApexPermute.h"

namespace nvidia
{
namespace apex
{

#ifdef _DEBUG
#define VERIFY_PARAM(_A) PX_ASSERT(_A == NvParameterized::ERROR_NONE)
#else
#define VERIFY_PARAM(_A) _A
#endif

// Transform Vec3 by PxMat44Legacy
PX_INLINE void transform_FLOAT3_by_PxMat44(FLOAT3_TYPE& dst, const FLOAT3_TYPE& src, const PxMat44& m)
{
	(PxVec3&)dst = m.transform((const PxVec3&)src);
}

// Transform Vec3 by PxMat33
PX_INLINE void transform_FLOAT3_by_PxMat33(FLOAT3_TYPE& dst, const FLOAT3_TYPE& src, const PxMat33& m)
{
	(PxVec3&)dst = m * (const PxVec3&)src;
}

// Transform Vec4 (tangent) by PxMat33, ignoring tangent.w
PX_INLINE void transform_FLOAT4_by_PxMat33(FLOAT4_TYPE& dst, const FLOAT4_TYPE& src, const PxMat33& m)
{
	const PxVec4 source = (const PxVec4&)src;
	(PxVec4&)dst = PxVec4(m * source.getXYZ(), PxSign(m.getDeterminant()) * source.w);
}

// Transform Quat by PxMat33
PX_INLINE void transform_FLOAT4_QUAT_by_PxMat33(FLOAT4_QUAT_TYPE& dst, const FLOAT4_QUAT_TYPE& src, const PxMat33& m)
{
	*((PxVec3*)&dst) = m * (*(const PxVec3*)&src.x);
}

// Multiply Vec3 by scalar
PX_INLINE void transform_FLOAT3_by_float(FLOAT3_TYPE& dst, const FLOAT3_TYPE& src, const float& s)
{
	(PxVec3&)dst = s * (const PxVec3&)src;
}

// Transform signed normalized byte 3-vector by PxMat44Legacy
PX_INLINE void transform_BYTE_SNORM3_by_PxMat44(BYTE_SNORM3_TYPE& dst, const BYTE_SNORM3_TYPE& src, const PxMat44& m)
{
	PxVec3 v;
	convert_FLOAT3_from_BYTE_SNORM3((FLOAT3_TYPE&)v, src);
	transform_FLOAT3_by_PxMat44((FLOAT3_TYPE&)v, (const FLOAT3_TYPE&)v, m);
	convert_BYTE_SNORM3_from_FLOAT3(dst, (const FLOAT3_TYPE&)v);
}

// Transform signed normalized byte 3-vector by PxMat33
PX_INLINE void transform_BYTE_SNORM3_by_PxMat33(BYTE_SNORM3_TYPE& dst, const BYTE_SNORM3_TYPE& src, const PxMat33& m)
{
	PxVec3 v;
	convert_FLOAT3_from_BYTE_SNORM3((FLOAT3_TYPE&)v, src);
	transform_FLOAT3_by_PxMat33((FLOAT3_TYPE&)v, (const FLOAT3_TYPE&)v, m);
	convert_BYTE_SNORM3_from_FLOAT3(dst, (const FLOAT3_TYPE&)v);
}

// Transform signed normalized byte 4-vector by PxMat33
PX_INLINE void transform_BYTE_SNORM4_by_PxMat33(BYTE_SNORM4_TYPE& dst, const BYTE_SNORM4_TYPE& src, const PxMat33& m)
{
	physx::PxVec4 v;
	convert_FLOAT4_from_BYTE_SNORM4((FLOAT4_TYPE&)v, src);
	transform_FLOAT4_by_PxMat33((FLOAT4_TYPE&)v, (const FLOAT4_TYPE&)v, m);
	convert_BYTE_SNORM4_from_FLOAT4(dst, (const FLOAT4_TYPE&)v);
}

// Multiply signed normalized byte 3-vector by scalar
PX_INLINE void transform_BYTE_SNORM3_by_float(BYTE_SNORM3_TYPE& dst, const BYTE_SNORM3_TYPE& src, const float& s)
{
	PxVec3 v;
	convert_FLOAT3_from_BYTE_SNORM3((FLOAT3_TYPE&)v, src);
	transform_FLOAT3_by_float((FLOAT3_TYPE&)v, (const FLOAT3_TYPE&)v, s);
	convert_BYTE_SNORM3_from_FLOAT3(dst, (const FLOAT3_TYPE&)v);
}

// Transform signed normalized byte quat by PxMat33
PX_INLINE void transform_BYTE_SNORM4_QUATXYZW_by_PxMat33(BYTE_SNORM4_QUATXYZW_TYPE& dst, const BYTE_SNORM4_QUATXYZW_TYPE& src, const PxMat33& m)
{
	transform_BYTE_SNORM3_by_PxMat33(*(BYTE_SNORM3_TYPE*)&dst, *(const BYTE_SNORM3_TYPE*)&src, m);
}

// Transform signed normalized short 3-vector by PxMat44
PX_INLINE void transform_SHORT_SNORM3_by_PxMat44(SHORT_SNORM3_TYPE& dst, const SHORT_SNORM3_TYPE& src, const PxMat44& m)
{
	PxVec3 v;
	convert_FLOAT3_from_SHORT_SNORM3((FLOAT3_TYPE&)v, src);
	transform_FLOAT3_by_PxMat44((FLOAT3_TYPE&)v, (const FLOAT3_TYPE&)v, m);
	convert_SHORT_SNORM3_from_FLOAT3(dst, (const FLOAT3_TYPE&)v);
}

// Transform signed normalized short 3-vector by PxMat33
PX_INLINE void transform_SHORT_SNORM3_by_PxMat33(SHORT_SNORM3_TYPE& dst, const SHORT_SNORM3_TYPE& src, const PxMat33& m)
{
	PxVec3 v;
	convert_FLOAT3_from_SHORT_SNORM3((FLOAT3_TYPE&)v, src);
	transform_FLOAT3_by_PxMat33((FLOAT3_TYPE&)v, (const FLOAT3_TYPE&)v, m);
	convert_SHORT_SNORM3_from_FLOAT3(dst, (const FLOAT3_TYPE&)v);
}

// Transform signed normalized short 4-vector by PxMat33
PX_INLINE void transform_SHORT_SNORM4_by_PxMat33(SHORT_SNORM4_TYPE& dst, const SHORT_SNORM4_TYPE& src, const PxMat33& m)
{
	physx::PxVec4 v;
	convert_FLOAT4_from_SHORT_SNORM4((FLOAT4_TYPE&)v, src);
	transform_FLOAT4_by_PxMat33((FLOAT4_TYPE&)v, (const FLOAT4_TYPE&)v, m);
	convert_SHORT_SNORM4_from_FLOAT4(dst, (const FLOAT4_TYPE&)v);
}

// Multiply signed normalized short 3-vector by scalar
PX_INLINE void transform_SHORT_SNORM3_by_float(SHORT_SNORM3_TYPE& dst, const SHORT_SNORM3_TYPE& src, const float& s)
{
	PxVec3 v;
	convert_FLOAT3_from_SHORT_SNORM3((FLOAT3_TYPE&)v, src);
	transform_FLOAT3_by_float((FLOAT3_TYPE&)v, (const FLOAT3_TYPE&)v, s);
	convert_SHORT_SNORM3_from_FLOAT3(dst, (const FLOAT3_TYPE&)v);
}

// Transform signed normalized short quat by PxMat33
PX_INLINE void transform_SHORT_SNORM4_QUATXYZW_by_PxMat33(SHORT_SNORM4_QUATXYZW_TYPE& dst, const SHORT_SNORM4_QUATXYZW_TYPE& src, const PxMat33& m)
{
	transform_SHORT_SNORM3_by_PxMat33(*(SHORT_SNORM3_TYPE*)&dst, *(const SHORT_SNORM3_TYPE*)&src, m);
}

#define SAME(x) x

#define HANDLE_TRANSFORM( _DataType, _OpType ) \
	case RenderDataFormat::SAME(_DataType): \
	while( numVertices-- ) \
	{ \
		transform_##_DataType##_by_##_OpType( *(_DataType##_TYPE*)dst, *(const _DataType##_TYPE*)src, op ); \
		((uint8_t*&)dst) += sizeof( _DataType##_TYPE ); \
		((const uint8_t*&)src) += sizeof( _DataType##_TYPE ); \
	} \
	return;

void transformRenderBuffer(void* dst, const void* src, RenderDataFormat::Enum format, uint32_t numVertices, const PxMat44& op)
{
	switch (format)
	{
		// Put transform handlers here
		HANDLE_TRANSFORM(FLOAT3, PxMat44)
		HANDLE_TRANSFORM(BYTE_SNORM3, PxMat44)
		HANDLE_TRANSFORM(SHORT_SNORM3, PxMat44)
	default:
		break;
	}

	PX_ALWAYS_ASSERT();	// Unhandled format
}

void transformRenderBuffer(void* dst, const void* src, RenderDataFormat::Enum format, uint32_t numVertices, const PxMat33& op)
{
	switch (format)
	{
		// Put transform handlers here
		HANDLE_TRANSFORM(FLOAT3, PxMat33)
		HANDLE_TRANSFORM(FLOAT4, PxMat33)
		HANDLE_TRANSFORM(FLOAT4_QUAT, PxMat33)
		HANDLE_TRANSFORM(BYTE_SNORM3, PxMat33)
		HANDLE_TRANSFORM(BYTE_SNORM4, PxMat33)
		HANDLE_TRANSFORM(BYTE_SNORM4_QUATXYZW, PxMat33)
		HANDLE_TRANSFORM(SHORT_SNORM3, PxMat33)
		HANDLE_TRANSFORM(SHORT_SNORM4, PxMat33)
		HANDLE_TRANSFORM(SHORT_SNORM4_QUATXYZW, PxMat33)
	default:
		break;
	}

	PX_ALWAYS_ASSERT();	// Unhandled format
}

void transformRenderBuffer(void* dst, const void* src, RenderDataFormat::Enum format, uint32_t numVertices, const float& op)
{
	switch (format)
	{
		// Put transform handlers here
		HANDLE_TRANSFORM(FLOAT3, float)
		HANDLE_TRANSFORM(BYTE_SNORM3, float)
	default:
		break;
	}

	PX_ALWAYS_ASSERT();	// Unhandled format
}


ApexVertexBuffer::ApexVertexBuffer() : mParams(NULL), mFormat(NULL)
{
}

ApexVertexBuffer::~ApexVertexBuffer()
{
	PX_ASSERT(mParams == NULL);
}

void ApexVertexBuffer::build(const VertexFormat& format, uint32_t vertexCount)
{
	const ApexVertexFormat* apexVertexFormat = DYNAMIC_CAST(const ApexVertexFormat*)(&format);
	if (apexVertexFormat)
	{
		mFormat.copy(*apexVertexFormat);
	}

	NvParameterized::Handle handle(*mParams);
	VERIFY_PARAM(mParams->getParameterHandle("buffers", handle));
	VERIFY_PARAM(mParams->resizeArray(handle, mFormat.mParams->bufferFormats.arraySizes[0]));

	resize(vertexCount);
}

void ApexVertexBuffer::applyTransformation(const PxMat44& transformation)
{
	RenderDataFormat::Enum format;
	void* buf;
	uint32_t index;

	// Positions
	index = (uint32_t)getFormat().getBufferIndexFromID(getFormat().getSemanticID(RenderVertexSemantic::POSITION));
	buf = getBuffer(index);
	if (buf)
	{
		format = getFormat().getBufferFormat(index);
		transformRenderBuffer(buf, buf, format, getVertexCount(), transformation);
	}

	// Normals
	index = (uint32_t)getFormat().getBufferIndexFromID(getFormat().getSemanticID(RenderVertexSemantic::NORMAL));
	buf = getBuffer(index);
	if (buf)
	{
		// PH: the Cofactor matrix now also handles negative determinants, so it does the same as multiplying with the inverse transpose of transformation.M.
		const Cof44 cof(transformation);
		format = getFormat().getBufferFormat(index);
		transformRenderBuffer(buf, buf, format, getVertexCount(), cof.getBlock33());
	}

	// Tangents
	index = (uint32_t)getFormat().getBufferIndexFromID(getFormat().getSemanticID(RenderVertexSemantic::TANGENT));
	buf = getBuffer(index);
	if (buf)
	{
		format = getFormat().getBufferFormat(index);
		const PxMat33 tm(transformation.column0.getXYZ(),
								transformation.column1.getXYZ(),
								transformation.column2.getXYZ());
		transformRenderBuffer(buf, buf, format, getVertexCount(), tm);
	}

	// Binormals
	index = (uint32_t)getFormat().getBufferIndexFromID(getFormat().getSemanticID(RenderVertexSemantic::BINORMAL));
	buf = getBuffer(index);
	if (buf)
	{
		format = getFormat().getBufferFormat(index);
		const PxMat33 tm(transformation.column0.getXYZ(),
								transformation.column1.getXYZ(),
								transformation.column2.getXYZ());
		transformRenderBuffer(buf, buf, format, getVertexCount(), tm);
	}
}



void ApexVertexBuffer::applyScale(float scale)
{
	uint32_t index = (uint32_t)getFormat().getBufferIndexFromID(getFormat().getSemanticID(RenderVertexSemantic::POSITION));
	void* buf = getBuffer(index);
	RenderDataFormat::Enum format = getFormat().getBufferFormat(index);
	transformRenderBuffer(buf, buf, format, getVertexCount(), scale);
}



bool ApexVertexBuffer::mergeBinormalsIntoTangents()
{
	const uint32_t numBuffers = mFormat.getBufferCount();

	int32_t normalBufferIndex = -1;
	int32_t tangentBufferIndex = -1;
	int32_t binormalBufferIndex = -1;
	for (uint32_t i = 0; i < numBuffers; i++)
	{
		const RenderVertexSemantic::Enum semantic  = mFormat.getBufferSemantic(i);
		const RenderDataFormat::Enum format = mFormat.getBufferFormat(i);
		if (semantic == RenderVertexSemantic::NORMAL && format == RenderDataFormat::FLOAT3)
		{
			normalBufferIndex = (int32_t)i;
		}
		else if (semantic == RenderVertexSemantic::TANGENT && format == RenderDataFormat::FLOAT3)
		{
			tangentBufferIndex = (int32_t)i;
		}
		else if (semantic == RenderVertexSemantic::BINORMAL && format == RenderDataFormat::FLOAT3)
		{
			binormalBufferIndex = (int32_t)i;
		}
	}

	if (normalBufferIndex != -1 && tangentBufferIndex != -1 && binormalBufferIndex != -1)
	{
		// PH: This gets dirty. modifying the parameterized object directly
		BufferF32x3* normalsBuffer = static_cast<BufferF32x3*>(mParams->buffers.buf[normalBufferIndex]);
		BufferF32x3* oldTangentsBuffer = static_cast<BufferF32x3*>(mParams->buffers.buf[tangentBufferIndex]);
		BufferF32x3* oldBinormalsBuffer = static_cast<BufferF32x3*>(mParams->buffers.buf[binormalBufferIndex]);
		BufferF32x4* newTangentsBuffer = static_cast<BufferF32x4*>(GetInternalApexSDK()->getParameterizedTraits()->createNvParameterized("BufferF32x4"));

		if (normalsBuffer != NULL && oldTangentsBuffer != NULL && oldBinormalsBuffer != NULL && newTangentsBuffer != NULL)
		{
			const uint32_t numElements = (uint32_t)oldTangentsBuffer->data.arraySizes[0];

			PX_ASSERT(oldTangentsBuffer->data.arraySizes[0] == oldBinormalsBuffer->data.arraySizes[0]);
			{
				// resize the array
				NvParameterized::Handle handle(*newTangentsBuffer, "data");
				PX_ASSERT(handle.isValid());
				handle.resizeArray((int32_t)numElements);
			}
			PX_ASSERT(oldTangentsBuffer->data.arraySizes[0] == newTangentsBuffer->data.arraySizes[0]);

			const PxVec3* normals = normalsBuffer->data.buf;
			const PxVec3* oldTangents = oldTangentsBuffer->data.buf;
			const PxVec3* oldBinormals = oldBinormalsBuffer->data.buf;
			PxVec4* newTangents = (PxVec4*)newTangentsBuffer->data.buf;

			for (uint32_t i = 0; i < numElements; i++)
			{
				const float binormal = PxSign(normals[i].cross(oldTangents[i]).dot(oldBinormals[i]));
				newTangents[i] = PxVec4(oldTangents[i], binormal);
			}

			// Ok, real dirty now
			mParams->buffers.buf[(uint32_t)tangentBufferIndex] = newTangentsBuffer;
			for (uint32_t i = (uint32_t)binormalBufferIndex + 1; i < numBuffers; i++)
			{
				mParams->buffers.buf[i - 1] = mParams->buffers.buf[i];
			}
			mParams->buffers.buf[numBuffers - 1] = NULL;
			{
				NvParameterized::Handle handle(*mParams, "buffers");
				PX_ASSERT(handle.isValid());
				handle.resizeArray((int32_t)numBuffers - 1);
			}
			oldTangentsBuffer->destroy();
			oldBinormalsBuffer->destroy();

			// and make same change to the format too
			VertexFormatParameters* format = static_cast<VertexFormatParameters*>(mParams->vertexFormat);
			PX_ASSERT(format->bufferFormats.buf[tangentBufferIndex].semantic == RenderVertexSemantic::TANGENT);
			PX_ASSERT(format->bufferFormats.buf[tangentBufferIndex].format == RenderDataFormat::FLOAT3);
			format->bufferFormats.buf[tangentBufferIndex].format = RenderDataFormat::FLOAT4;

			VertexFormatParametersNS::BufferFormat_Type binormalBuffer = format->bufferFormats.buf[binormalBufferIndex];
			for (uint32_t i = (uint32_t)binormalBufferIndex + 1; i < numBuffers; i++)
			{
				format->bufferFormats.buf[i - 1] = format->bufferFormats.buf[i];
			}

			// swap it to the last such that it gets released properly
			format->bufferFormats.buf[numBuffers - 1] = binormalBuffer;
			{
				NvParameterized::Handle handle(*format, "bufferFormats");
				PX_ASSERT(handle.isValid());
				handle.resizeArray((int32_t)numBuffers - 1);
			}

			return true;
		}
	}
	return false;
}



void ApexVertexBuffer::copy(uint32_t dstIndex, uint32_t srcIndex, ApexVertexBuffer* srcBufferPtr)
{
	ApexVertexBuffer& srcVB = srcBufferPtr != NULL ? *srcBufferPtr : *this;
	ApexVertexFormat& srcVF = srcVB.mFormat;

	if (mParams->buffers.arraySizes[0] != srcVB.mParams->buffers.arraySizes[0])
	{
		PX_ALWAYS_ASSERT();
		return;
	}

	for (uint32_t i = 0; i < (uint32_t)mParams->buffers.arraySizes[0]; i++)
	{
		RenderDataFormat::Enum dstFormat = mFormat.getBufferFormat(i);
		VertexFormat::BufferID id = mFormat.getBufferID(i);
		const int32_t srcBufferIndex = srcVF.getBufferIndexFromID(id);
		if (srcBufferIndex >= 0)
		{
			RenderDataFormat::Enum srcFormat = srcVF.getBufferFormat((uint32_t)srcBufferIndex);
			NvParameterized::Interface* dstInterface = mParams->buffers.buf[i];
			NvParameterized::Interface* srcInterface = srcVB.mParams->buffers.buf[(uint32_t)srcBufferIndex];
			// BRG: Using PH's reasoning: Technically all those CustomBuffer* classes should have the same struct, so I just use the first one
			BufferU8x1& srcBuffer = *static_cast<BufferU8x1*>(srcInterface);
			BufferU8x1& dstBuffer = *static_cast<BufferU8x1*>(dstInterface);
			PX_ASSERT(dstIndex < (uint32_t)dstBuffer.data.arraySizes[0]);
			PX_ASSERT(srcIndex < (uint32_t)srcBuffer.data.arraySizes[0]);
			copyRenderVertexData(dstBuffer.data.buf, dstFormat, dstIndex, srcBuffer.data.buf, srcFormat, srcIndex);
		}
	}
}

void ApexVertexBuffer::resize(uint32_t vertexCount)
{
	mParams->vertexCount = vertexCount;

	NvParameterized::Handle handle(*mParams);

	VERIFY_PARAM(mParams->getParameterHandle("buffers", handle));
	int32_t buffersSize = 0;
	VERIFY_PARAM(mParams->getArraySize(handle, buffersSize));

	for (int32_t i = 0; i < buffersSize; i++)
	{
		RenderDataFormat::Enum outFormat = mFormat.getBufferFormat((uint32_t)i);

		NvParameterized::Handle elementHandle(*mParams);
		VERIFY_PARAM(handle.getChildHandle(i, elementHandle));

		NvParameterized::Interface* currentReference = NULL;
		VERIFY_PARAM(mParams->getParamRef(elementHandle, currentReference));

		// BUFFER_FORMAT_ADD This is just a bookmark for places where to add buffer formats
		if (currentReference == NULL && vertexCount > 0)
		{
			const char* className = NULL;

			switch (outFormat)
			{
			case RenderDataFormat::UBYTE1:
			case RenderDataFormat::BYTE_UNORM1:
			case RenderDataFormat::BYTE_SNORM1:
				className = BufferU8x1::staticClassName();
				break;
			case RenderDataFormat::UBYTE2:
			case RenderDataFormat::BYTE_UNORM2:
			case RenderDataFormat::BYTE_SNORM2:
				className = BufferU8x2::staticClassName();
				break;
			case RenderDataFormat::UBYTE3:
			case RenderDataFormat::BYTE_UNORM3:
			case RenderDataFormat::BYTE_SNORM3:
				className = BufferU8x3::staticClassName();
				break;
			case RenderDataFormat::UBYTE4:
			case RenderDataFormat::BYTE_UNORM4:
			case RenderDataFormat::BYTE_SNORM4:
			case RenderDataFormat::R8G8B8A8:
			case RenderDataFormat::B8G8R8A8:
				className = BufferU8x4::staticClassName();
				break;
			case RenderDataFormat::SHORT1:
			case RenderDataFormat::USHORT1:
			case RenderDataFormat::SHORT_UNORM1:
			case RenderDataFormat::SHORT_SNORM1:
			case RenderDataFormat::HALF1:
				className = BufferU16x1::staticClassName();
				break;
			case RenderDataFormat::SHORT2:
			case RenderDataFormat::USHORT2:
			case RenderDataFormat::SHORT_UNORM2:
			case RenderDataFormat::SHORT_SNORM2:
			case RenderDataFormat::HALF2:
				className = BufferU16x2::staticClassName();
				break;
			case RenderDataFormat::SHORT3:
			case RenderDataFormat::USHORT3:
			case RenderDataFormat::SHORT_UNORM3:
			case RenderDataFormat::SHORT_SNORM3:
			case RenderDataFormat::HALF3:
				className = BufferU16x3::staticClassName();
				break;
			case RenderDataFormat::SHORT4:
			case RenderDataFormat::USHORT4:
			case RenderDataFormat::SHORT_UNORM4:
			case RenderDataFormat::SHORT_SNORM4:
			case RenderDataFormat::HALF4:
				className = BufferU16x4::staticClassName();
				break;
			case RenderDataFormat::UINT1:
				className = BufferU32x1::staticClassName();
				break;
			case RenderDataFormat::UINT2:
				className = BufferU32x2::staticClassName();
				break;
			case RenderDataFormat::UINT3:
				className = BufferU32x3::staticClassName();
				break;
			case RenderDataFormat::UINT4:
				className = BufferU32x4::staticClassName();
				break;
			case RenderDataFormat::FLOAT1:
				className = BufferF32x1::staticClassName();
				break;
			case RenderDataFormat::FLOAT2:
				className = BufferF32x2::staticClassName();
				break;
			case RenderDataFormat::FLOAT3:
				className = BufferF32x3::staticClassName();
				break;
			case RenderDataFormat::FLOAT4:
			case RenderDataFormat::R32G32B32A32_FLOAT:
			case RenderDataFormat::B32G32R32A32_FLOAT:
				className = BufferF32x4::staticClassName();
				break;
			default:
				PX_ALWAYS_ASSERT();
				break;
			}

			if (className != NULL)
			{
				currentReference = GetInternalApexSDK()->getParameterizedTraits()->createNvParameterized(className);
			}

			if (currentReference != NULL)
			{
				NvParameterized::Handle arrayHandle(*currentReference);
				VERIFY_PARAM(currentReference->getParameterHandle("data", arrayHandle));
				PX_ASSERT(arrayHandle.isValid());
				VERIFY_PARAM(arrayHandle.resizeArray((int32_t)vertexCount));

				mParams->setParamRef(elementHandle, currentReference);
			}
		}
		else if (vertexCount > 0)
		{
			NvParameterized::Interface* oldReference = currentReference;
			PX_ASSERT(oldReference != NULL);
			currentReference = GetInternalApexSDK()->getParameterizedTraits()->createNvParameterized(oldReference->className());
			if (currentReference != NULL)
			{
				VERIFY_PARAM(currentReference->copy(*oldReference));

				NvParameterized::Handle arrayHandle(*currentReference);
				VERIFY_PARAM(currentReference->getParameterHandle("data", arrayHandle));
				VERIFY_PARAM(arrayHandle.resizeArray((int32_t)vertexCount));
			}
			VERIFY_PARAM(mParams->setParamRef(elementHandle, currentReference));
			oldReference->destroy();
		}
		else if (vertexCount == 0)
		{
			VERIFY_PARAM(mParams->setParamRef(elementHandle, NULL));

			if (currentReference != NULL)
			{
				currentReference->destroy();
			}
		}
	}
}



void ApexVertexBuffer::preSerialize(void*)
{
	PX_ASSERT((int32_t)mFormat.getBufferCount() == mParams->buffers.arraySizes[0]);
	ParamArray<NvParameterized::Interface*> buffers(mParams, "buffers", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->buffers));
	for (uint32_t i = 0; i < mFormat.getBufferCount(); i++)
	{
		if (!mFormat.getBufferSerialize(i))
		{
			// [i] no longer needs to be destroyed because the resize will handle it
			buffers.replaceWithLast(i);
			mFormat.bufferReplaceWithLast(i);
			i--;
		}
	}

	PX_ASSERT((int32_t)mFormat.getBufferCount() == mParams->buffers.arraySizes[0]);
}

bool ApexVertexBuffer::getBufferData(void* dstBuffer, nvidia::RenderDataFormat::Enum dstBufferFormat, uint32_t dstBufferStride, uint32_t bufferIndex,
                                     uint32_t startIndex, uint32_t elementCount) const
{
	const void* data = getBuffer(bufferIndex);
	if (data == NULL)
	{
		return false;
	}
	nvidia::RenderDataFormat::Enum srcFormat = getFormat().getBufferFormat(bufferIndex);
	return copyRenderVertexBuffer(dstBuffer, dstBufferFormat, dstBufferStride, 0, data, srcFormat, RenderDataFormat::getFormatDataSize(srcFormat), startIndex, elementCount);
}

void* ApexVertexBuffer::getBuffer(uint32_t bufferIndex)
{
	if (bufferIndex < (uint32_t)mParams->buffers.arraySizes[0])
	{
		NvParameterized::Interface* buffer = mParams->buffers.buf[bufferIndex];
		if (buffer != NULL)
		{
			BufferU8x1* particularBuffer = DYNAMIC_CAST(BufferU8x1*)(buffer);
			return particularBuffer->data.buf;
		}
	}

	return NULL;
}

uint32_t ApexVertexBuffer::getAllocationSize() const
{
	uint32_t size = sizeof(ApexVertexBuffer);

	for (uint32_t index = 0; (int32_t)index < mParams->buffers.arraySizes[0]; ++index)
	{
		PX_ASSERT(index < getFormat().getBufferCount());
		if (index >= getFormat().getBufferCount())
		{
			break;
		}
		const uint32_t dataSize = RenderDataFormat::getFormatDataSize(getFormat().getBufferFormat(index));
		NvParameterized::Interface* buffer = mParams->buffers.buf[index];
		if (buffer != NULL)
		{
			BufferU8x1* particularBuffer = DYNAMIC_CAST(BufferU8x1*)(buffer);
			size += particularBuffer->data.arraySizes[0] * dataSize;
		}
	}

	return size;
}

void ApexVertexBuffer::setParams(VertexBufferParameters* param)
{
	if (mParams != param)
	{
		if (mParams != NULL)
		{
			mParams->setSerializationCallback(NULL);
		}

		mParams = param;

		if (mParams != NULL)
		{
			NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
			if (mParams->vertexFormat != NULL)
			{
				if (mFormat.mParams && mFormat.mParams != (VertexFormatParameters*)mParams->vertexFormat)
				{
					mFormat.mParams->destroy();
				}
			}
			else
			{
				mParams->vertexFormat = DYNAMIC_CAST(VertexFormatParameters*)(traits->createNvParameterized(VertexFormatParameters::staticClassName()));
			}
		}

		mFormat.mParams = mParams != NULL ? static_cast<VertexFormatParameters*>(mParams->vertexFormat) : NULL;
		mFormat.mOwnsParams = false;

		if (mParams != NULL)
		{
			mParams->setSerializationCallback(this);
		}
	}
}

namespace 
{
	class PxMat34Legacy
	{
		float f[12];
	};
}

void ApexVertexBuffer::applyPermutation(const Array<uint32_t>& permutation)
{
	const uint32_t numVertices = mParams->vertexCount;
	PX_ASSERT(numVertices == permutation.size());
	for (uint32_t i = 0; i < (uint32_t)mParams->buffers.arraySizes[0]; i++)
	{
		NvParameterized::Interface* bufferInterface = mParams->buffers.buf[i];
		RenderDataFormat::Enum format = getFormat().getBufferFormat(i);
		switch(format)
		{
			// all 1 byte
		case RenderDataFormat::UBYTE1:
		case RenderDataFormat::BYTE_UNORM1:
		case RenderDataFormat::BYTE_SNORM1:
			{
				BufferU8x1* byte1 = static_cast<BufferU8x1*>(bufferInterface);
				PX_ASSERT(numVertices == (uint32_t)byte1->data.arraySizes[0]);
				ApexPermute(byte1->data.buf, permutation.begin(), numVertices);
			}
			break;

			// all 2 byte
		case RenderDataFormat::UBYTE2:
		case RenderDataFormat::USHORT1:
		case RenderDataFormat::SHORT1:
		case RenderDataFormat::BYTE_UNORM2:
		case RenderDataFormat::SHORT_UNORM1:
		case RenderDataFormat::BYTE_SNORM2:
		case RenderDataFormat::SHORT_SNORM1:
		case RenderDataFormat::HALF1:
			{
				BufferU16x1* short1 = static_cast<BufferU16x1*>(bufferInterface);
				PX_ASSERT(numVertices == (uint32_t)short1->data.arraySizes[0]);
				ApexPermute(short1->data.buf, permutation.begin(), numVertices);
			}
			break;

			// all 3 byte
		case RenderDataFormat::UBYTE3:
		case RenderDataFormat::BYTE_UNORM3:
		case RenderDataFormat::BYTE_SNORM3:
			{
				BufferU8x3* byte3 = static_cast<BufferU8x3*>(bufferInterface);
				PX_ASSERT(numVertices == (uint32_t)byte3->data.arraySizes[0]);
				ApexPermute(byte3->data.buf, permutation.begin(), numVertices);
			}
			break;

			// all 4 byte
		case RenderDataFormat::UBYTE4:
		case RenderDataFormat::USHORT2:
		case RenderDataFormat::SHORT2:
		case RenderDataFormat::UINT1:
		case RenderDataFormat::R8G8B8A8:
		case RenderDataFormat::B8G8R8A8:
		case RenderDataFormat::BYTE_UNORM4:
		case RenderDataFormat::SHORT_UNORM2:
		case RenderDataFormat::BYTE_SNORM4:
		case RenderDataFormat::SHORT_SNORM2:
		case RenderDataFormat::HALF2:
		case RenderDataFormat::FLOAT1:
		case RenderDataFormat::BYTE_SNORM4_QUATXYZW:
		case RenderDataFormat::SHORT_SNORM4_QUATXYZW:
			{
				BufferU32x1* int1 = static_cast<BufferU32x1*>(bufferInterface);
				PX_ASSERT(numVertices == (uint32_t)int1->data.arraySizes[0]);
				ApexPermute(int1->data.buf, permutation.begin(), numVertices);
			}
			break;

			// all 6 byte
		case RenderDataFormat::USHORT3:
		case RenderDataFormat::SHORT3:
		case RenderDataFormat::SHORT_UNORM3:
		case RenderDataFormat::SHORT_SNORM3:
		case RenderDataFormat::HALF3:
			{
				BufferU16x3* short3 = static_cast<BufferU16x3*>(bufferInterface);
				PX_ASSERT(numVertices == (uint32_t)short3->data.arraySizes[0]);
				ApexPermute(short3->data.buf, permutation.begin(), numVertices);
			}
			break;

			// all 8 byte
		case RenderDataFormat::USHORT4:
		case RenderDataFormat::SHORT4:
		case RenderDataFormat::SHORT_UNORM4:
		case RenderDataFormat::SHORT_SNORM4:
		case RenderDataFormat::UINT2:
		case RenderDataFormat::HALF4:
		case RenderDataFormat::FLOAT2:
			{
				BufferU32x2* int2 = static_cast<BufferU32x2*>(bufferInterface);
				PX_ASSERT(numVertices == (uint32_t)int2->data.arraySizes[0]);
				ApexPermute(int2->data.buf, permutation.begin(), numVertices);
			}
			break;

			// all 12 byte
		case RenderDataFormat::UINT3:
		case RenderDataFormat::FLOAT3:
			{
				BufferU32x3* int3 = static_cast<BufferU32x3*>(bufferInterface);
				PX_ASSERT(numVertices == (uint32_t)int3->data.arraySizes[0]);
				ApexPermute(int3->data.buf, permutation.begin(), numVertices);
			}
			break;

			// all 16 byte
		case RenderDataFormat::UINT4:
		case RenderDataFormat::R32G32B32A32_FLOAT:
		case RenderDataFormat::B32G32R32A32_FLOAT:
		case RenderDataFormat::FLOAT4:
		case RenderDataFormat::FLOAT4_QUAT:
			{
				BufferU32x4* int4 = static_cast<BufferU32x4*>(bufferInterface);
				PX_ASSERT(numVertices == (uint32_t)int4->data.arraySizes[0]);
				ApexPermute(int4->data.buf, permutation.begin(), numVertices);
			}
			break;

			// all 36 byte
		case RenderDataFormat::FLOAT3x3:
			{
				BufferF32x1* float1 = static_cast<BufferF32x1*>(bufferInterface);
				PX_ASSERT(numVertices == (uint32_t)float1->data.arraySizes[0]);
				ApexPermute((PxMat33*)float1->data.buf, permutation.begin(), numVertices);
			}
			break;

			// all 48 byte
		case RenderDataFormat::FLOAT3x4:
			{
				BufferF32x1* float1 = static_cast<BufferF32x1*>(bufferInterface);
				PX_ASSERT(numVertices == (uint32_t)float1->data.arraySizes[0]);
				ApexPermute((PxMat34Legacy*)float1->data.buf, permutation.begin(), numVertices);
				
			}
			break;

			// all 64 byte
		case RenderDataFormat::FLOAT4x4:
			{
				BufferF32x1* float1 = static_cast<BufferF32x1*>(bufferInterface);
				PX_ASSERT(numVertices == (uint32_t)float1->data.arraySizes[0]);
				ApexPermute((PxMat44*)float1->data.buf, permutation.begin(), numVertices);
			}
			break;

		// fix gcc warnings
		case RenderDataFormat::UNSPECIFIED:
		case RenderDataFormat::NUM_FORMATS:
			break;
		}
	}
}



}
} // end namespace nvidia::apex
