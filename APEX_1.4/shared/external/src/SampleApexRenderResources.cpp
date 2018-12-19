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



#include <SampleApexRenderResources.h>

#include <Renderer.h>

#include <RendererVertexBuffer.h>
#include <RendererVertexBufferDesc.h>

#include <RendererIndexBuffer.h>
#include <RendererIndexBufferDesc.h>

#include <RendererInstanceBuffer.h>
#include <RendererInstanceBufferDesc.h>

#include <RendererTexture.h>
#include <RendererTextureDesc.h>

#include <RendererMesh.h>
#include <RendererMeshDesc.h>

#include <RendererMaterial.h>
#include <RendererMaterialDesc.h>
#include <RendererMaterialInstance.h>

#if !USE_RENDERER_MATERIAL
#include <SampleMaterialAsset.h>
#endif

#include <RenderContext.h>
#include <UserRenderIndexBufferDesc.h>
#include <UserRenderInstanceBuffer.h>
#include <UserRenderResourceDesc.h>
#include <UserRenderSpriteBufferDesc.h>
#include <UserRenderSurfaceBufferDesc.h>
#include <UserRenderVertexBufferDesc.h>


static RendererVertexBuffer::Hint convertFromApexVB(nvidia::apex::RenderBufferHint::Enum apexHint)
{
	RendererVertexBuffer::Hint vbhint = RendererVertexBuffer::HINT_STATIC;
	if (apexHint == nvidia::apex::RenderBufferHint::DYNAMIC || apexHint == nvidia::apex::RenderBufferHint::STREAMING)
	{
		vbhint = RendererVertexBuffer::HINT_DYNAMIC;
	}
	return vbhint;
}

static RendererVertexBuffer::Semantic convertFromApexVB(nvidia::apex::RenderVertexSemantic::Enum apexSemantic)
{
	RendererVertexBuffer::Semantic semantic = RendererVertexBuffer::NUM_SEMANTICS;
	switch (apexSemantic)
	{
	case nvidia::apex::RenderVertexSemantic::POSITION:
		semantic = RendererVertexBuffer::SEMANTIC_POSITION;
		break;
	case nvidia::apex::RenderVertexSemantic::NORMAL:
		semantic = RendererVertexBuffer::SEMANTIC_NORMAL;
		break;
	case nvidia::apex::RenderVertexSemantic::TANGENT:
		semantic = RendererVertexBuffer::SEMANTIC_TANGENT;
		break;
	case nvidia::apex::RenderVertexSemantic::COLOR:
		semantic = RendererVertexBuffer::SEMANTIC_COLOR;
		break;
	case nvidia::apex::RenderVertexSemantic::TEXCOORD0:
		semantic = RendererVertexBuffer::SEMANTIC_TEXCOORD0;
		break;
	case nvidia::apex::RenderVertexSemantic::TEXCOORD1:
		semantic = RendererVertexBuffer::SEMANTIC_TEXCOORD1;
		break;
	case nvidia::apex::RenderVertexSemantic::TEXCOORD2:
		semantic = RendererVertexBuffer::SEMANTIC_TEXCOORD2;
		break;
	case nvidia::apex::RenderVertexSemantic::TEXCOORD3:
		semantic = RendererVertexBuffer::SEMANTIC_TEXCOORD3;
		break;
	case nvidia::apex::RenderVertexSemantic::BONE_INDEX:
		semantic = RendererVertexBuffer::SEMANTIC_BONEINDEX;
		break;
	case nvidia::apex::RenderVertexSemantic::BONE_WEIGHT:
		semantic = RendererVertexBuffer::SEMANTIC_BONEWEIGHT;
		break;
	case nvidia::apex::RenderVertexSemantic::DISPLACEMENT_TEXCOORD:
		semantic = RendererVertexBuffer::SEMANTIC_DISPLACEMENT_TEXCOORD;
		break;
	case nvidia::apex::RenderVertexSemantic::DISPLACEMENT_FLAGS:
		semantic = RendererVertexBuffer::SEMANTIC_DISPLACEMENT_FLAGS;
		break;
	default:
		//PX_ASSERT(semantic < RendererVertexBuffer::NUM_SEMANTICS);
		break;
	}
	return semantic;
}

static RendererVertexBuffer::Format convertFromApexVB(nvidia::apex::RenderDataFormat::Enum apexFormat)
{
	RendererVertexBuffer::Format format = RendererVertexBuffer::NUM_FORMATS;
	switch (apexFormat)
	{
	case nvidia::apex::RenderDataFormat::FLOAT1:
		format = RendererVertexBuffer::FORMAT_FLOAT1;
		break;
	case nvidia::apex::RenderDataFormat::FLOAT2:
		format = RendererVertexBuffer::FORMAT_FLOAT2;
		break;
	case nvidia::apex::RenderDataFormat::FLOAT3:
		format = RendererVertexBuffer::FORMAT_FLOAT3;
		break;
	case nvidia::apex::RenderDataFormat::FLOAT4:
		format = RendererVertexBuffer::FORMAT_FLOAT4;
		break;
	case nvidia::apex::RenderDataFormat::B8G8R8A8:
		format = RendererVertexBuffer::FORMAT_COLOR_BGRA;
		break;
	case nvidia::apex::RenderDataFormat::UINT1:
	case nvidia::apex::RenderDataFormat::UBYTE4:
	case nvidia::apex::RenderDataFormat::R8G8B8A8:
		format = RendererVertexBuffer::FORMAT_COLOR_RGBA;
		break;
	case nvidia::apex::RenderDataFormat::USHORT1:
	case nvidia::apex::RenderDataFormat::USHORT2:
	case nvidia::apex::RenderDataFormat::USHORT3:
	case nvidia::apex::RenderDataFormat::USHORT4:
		format = RendererVertexBuffer::FORMAT_USHORT4;
		break;
	default:
		//PX_ASSERT(format < RendererVertexBuffer::NUM_FORMATS || apexFormat==RenderDataFormat::UNSPECIFIED);
		break;
	}
	return format;
}

#if 0 // Unused
static RendererVertexBuffer::Semantic convertFromApexSB(nvidia::apex::RenderSpriteSemantic::Enum apexSemantic)
{
	RendererVertexBuffer::Semantic semantic = RendererVertexBuffer::NUM_SEMANTICS;
	switch (apexSemantic)
	{
	case nvidia::apex::RenderSpriteSemantic::POSITION:
		semantic = RendererVertexBuffer::SEMANTIC_POSITION;
		break;
	case nvidia::apex::RenderSpriteSemantic::COLOR:
		semantic = RendererVertexBuffer::SEMANTIC_COLOR;
		break;
	case nvidia::apex::RenderSpriteSemantic::VELOCITY:
		semantic = RendererVertexBuffer::SEMANTIC_NORMAL;
		break;
	case nvidia::apex::RenderSpriteSemantic::SCALE:
		semantic = RendererVertexBuffer::SEMANTIC_TANGENT;
		break;
	case nvidia::apex::RenderSpriteSemantic::LIFE_REMAIN:
		semantic = RendererVertexBuffer::SEMANTIC_TEXCOORD0;
		break;
	case nvidia::apex::RenderSpriteSemantic::DENSITY:
		semantic = RendererVertexBuffer::SEMANTIC_TEXCOORD1;
		break;
	case nvidia::apex::RenderSpriteSemantic::SUBTEXTURE:
		semantic = RendererVertexBuffer::SEMANTIC_TEXCOORD2;
		break;
	case nvidia::apex::RenderSpriteSemantic::ORIENTATION:
		semantic = RendererVertexBuffer::SEMANTIC_TEXCOORD3;
		break;
	default:
		//PX_ASSERT(semantic < RendererVertexBuffer::NUM_SEMANTICS);
		break;
	}
	return semantic;
}
#endif

static RendererVertexBuffer::Semantic convertFromApexLayoutSB(const nvidia::apex::RenderSpriteLayoutElement::Enum layout)
{
	RendererVertexBuffer::Semantic semantic = RendererVertexBuffer::NUM_SEMANTICS;
	switch (layout)
	{
	case nvidia::apex::RenderSpriteLayoutElement::POSITION_FLOAT3:
		semantic = RendererVertexBuffer::SEMANTIC_POSITION;
		break;
	case nvidia::apex::RenderSpriteLayoutElement::COLOR_BGRA8:
	case nvidia::apex::RenderSpriteLayoutElement::COLOR_RGBA8:
	case nvidia::apex::RenderSpriteLayoutElement::COLOR_FLOAT4:
		semantic = RendererVertexBuffer::SEMANTIC_COLOR;
		break;
	case nvidia::apex::RenderSpriteLayoutElement::VELOCITY_FLOAT3:
		semantic = RendererVertexBuffer::SEMANTIC_NORMAL;
		break;
	case nvidia::apex::RenderSpriteLayoutElement::SCALE_FLOAT2:
		semantic = RendererVertexBuffer::SEMANTIC_TANGENT;
		break;
	case nvidia::apex::RenderSpriteLayoutElement::LIFE_REMAIN_FLOAT1:
		semantic = RendererVertexBuffer::SEMANTIC_TEXCOORD0;
		break;
	case nvidia::apex::RenderSpriteLayoutElement::DENSITY_FLOAT1:
		semantic = RendererVertexBuffer::SEMANTIC_TEXCOORD1;
		break;
	case nvidia::apex::RenderSpriteLayoutElement::SUBTEXTURE_FLOAT1:
		semantic = RendererVertexBuffer::SEMANTIC_TEXCOORD2;
		break;
	case nvidia::apex::RenderSpriteLayoutElement::ORIENTATION_FLOAT1:
		semantic = RendererVertexBuffer::SEMANTIC_TEXCOORD3;
		break;
	default:
		//PX_ASSERT(semantic < RendererVertexBuffer::NUM_SEMANTICS);
		break;
	}
	return semantic;
}

static RendererVertexBuffer::Format convertFromApexLayoutVB(const nvidia::apex::RenderSpriteLayoutElement::Enum layout)
{
	RendererVertexBuffer::Format format = RendererVertexBuffer::NUM_FORMATS;
	switch (layout)
	{
	case nvidia::apex::RenderSpriteLayoutElement::POSITION_FLOAT3:
		format = RendererVertexBuffer::FORMAT_FLOAT3;
		break;
	case nvidia::apex::RenderSpriteLayoutElement::COLOR_BGRA8:
		format = RendererVertexBuffer::FORMAT_COLOR_BGRA;
		break;
	case nvidia::apex::RenderSpriteLayoutElement::COLOR_RGBA8:
		format = RendererVertexBuffer::FORMAT_COLOR_RGBA;
		break;
	case nvidia::apex::RenderSpriteLayoutElement::COLOR_FLOAT4:
		format = RendererVertexBuffer::FORMAT_FLOAT4;
		break;
	case nvidia::apex::RenderSpriteLayoutElement::VELOCITY_FLOAT3:
		format = RendererVertexBuffer::FORMAT_FLOAT3;
		break;
	case nvidia::apex::RenderSpriteLayoutElement::SCALE_FLOAT2:
		format = RendererVertexBuffer::FORMAT_FLOAT2;
		break;
	case nvidia::apex::RenderSpriteLayoutElement::LIFE_REMAIN_FLOAT1:
		format = RendererVertexBuffer::FORMAT_FLOAT1;
		break;
	case nvidia::apex::RenderSpriteLayoutElement::DENSITY_FLOAT1:
		format = RendererVertexBuffer::FORMAT_FLOAT1;
		break;
	case nvidia::apex::RenderSpriteLayoutElement::SUBTEXTURE_FLOAT1:
		format = RendererVertexBuffer::FORMAT_FLOAT1;
		break;
	case nvidia::apex::RenderSpriteLayoutElement::ORIENTATION_FLOAT1:
		format = RendererVertexBuffer::FORMAT_FLOAT1;
		break;
	default:
		//PX_ASSERT(semantic < RendererVertexBuffer::NUM_SEMANTICS);
		break;
	}
	return format;
}

static RendererIndexBuffer::Hint convertFromApexIB(nvidia::apex::RenderBufferHint::Enum apexHint)
{
	RendererIndexBuffer::Hint ibhint = RendererIndexBuffer::HINT_STATIC;
	if (apexHint == nvidia::apex::RenderBufferHint::DYNAMIC || apexHint == nvidia::apex::RenderBufferHint::STREAMING)
	{
		ibhint = RendererIndexBuffer::HINT_DYNAMIC;
	}
	return ibhint;
}

static RendererIndexBuffer::Format convertFromApexIB(nvidia::apex::RenderDataFormat::Enum apexFormat)
{
	RendererIndexBuffer::Format format = RendererIndexBuffer::NUM_FORMATS;
	switch (apexFormat)
	{
	case nvidia::apex::RenderDataFormat::UBYTE1:
		PX_ASSERT(0); /* UINT8 Indices not in HW. */
		break;
	case nvidia::apex::RenderDataFormat::USHORT1:
		format = RendererIndexBuffer::FORMAT_UINT16;
		break;
	case nvidia::apex::RenderDataFormat::UINT1:
		format = RendererIndexBuffer::FORMAT_UINT32;
		break;
	default:
		PX_ASSERT(format < RendererIndexBuffer::NUM_FORMATS);
	}
	return format;
}

static RendererTexture::Format convertFromApexSB(nvidia::apex::RenderDataFormat::Enum apexFormat)
{
	RendererTexture::Format format = RendererTexture::NUM_FORMATS;
	switch (apexFormat)
	{
	case nvidia::apex::RenderDataFormat::FLOAT1:
		format = RendererTexture::FORMAT_R32F;
		break;
	case nvidia::apex::RenderDataFormat::R32G32B32A32_FLOAT:
	case nvidia::apex::RenderDataFormat::B32G32R32A32_FLOAT:
	case nvidia::apex::RenderDataFormat::FLOAT4:
		format = RendererTexture::FORMAT_R32F_G32F_B32G_A32F;
		break;
	case nvidia::apex::RenderDataFormat::HALF4:
		format = RendererTexture::FORMAT_R16F_G16F_B16G_A16F;
		break;
	default:
		PX_ASSERT(format < RendererTexture::NUM_FORMATS);
		break;
	}
	return format;
}

static RendererMesh::Primitive convertFromApex(nvidia::apex::RenderPrimitiveType::Enum apexPrimitive)
{
	RendererMesh::Primitive primitive = RendererMesh::NUM_PRIMITIVES;
	switch (apexPrimitive)
	{
	case nvidia::apex::RenderPrimitiveType::TRIANGLES:
		primitive = RendererMesh::PRIMITIVE_TRIANGLES;
		break;
	case nvidia::apex::RenderPrimitiveType::TRIANGLE_STRIP:
		primitive = RendererMesh::PRIMITIVE_TRIANGLE_STRIP;
		break;

	case nvidia::apex::RenderPrimitiveType::LINES:
		primitive = RendererMesh::PRIMITIVE_LINES;
		break;
	case nvidia::apex::RenderPrimitiveType::LINE_STRIP:
		primitive = RendererMesh::PRIMITIVE_LINE_STRIP;
		break;

	case nvidia::apex::RenderPrimitiveType::POINTS:
		primitive = RendererMesh::PRIMITIVE_POINTS;
		break;
	case nvidia::apex::RenderPrimitiveType::POINT_SPRITES:
		primitive = RendererMesh::PRIMITIVE_POINT_SPRITES;
		break;

	case nvidia::apex::RenderPrimitiveType::UNKNOWN: // Make compiler happy
		break;
	}
	PX_ASSERT(primitive < RendererMesh::NUM_PRIMITIVES);
	return primitive;
}

/*********************************
* SampleApexRendererVertexBuffer *
*********************************/

SampleApexRendererVertexBuffer::SampleApexRendererVertexBuffer(SampleRenderer::Renderer& renderer, const nvidia::apex::UserRenderVertexBufferDesc& desc) :
	m_renderer(renderer)
{
	m_vertexbuffer = 0;
	SampleRenderer::RendererVertexBufferDesc vbdesc;
	vbdesc.hint = convertFromApexVB(desc.hint);
	for (uint32_t i = 0; i < nvidia::apex::RenderVertexSemantic::NUM_SEMANTICS; i++)
	{
		nvidia::apex::RenderVertexSemantic::Enum apexSemantic = nvidia::apex::RenderVertexSemantic::Enum(i);
		nvidia::apex::RenderDataFormat::Enum apexFormat = desc.buffersRequest[i];
		if ((apexSemantic == nvidia::apex::RenderVertexSemantic::NORMAL || apexSemantic == nvidia::apex::RenderVertexSemantic::BINORMAL) && apexFormat != nvidia::apex::RenderDataFormat::UNSPECIFIED)
		{
			PX_ASSERT(apexFormat == nvidia::apex::RenderDataFormat::FLOAT3 || apexFormat == nvidia::apex::RenderDataFormat::BYTE_SNORM3);
			// always use FLOAT3 for normals and binormals
			apexFormat = nvidia::apex::RenderDataFormat::FLOAT3;
		}
		else if (apexSemantic == nvidia::apex::RenderVertexSemantic::TANGENT && apexFormat != nvidia::apex::RenderDataFormat::UNSPECIFIED)
		{
			PX_ASSERT(apexFormat == nvidia::apex::RenderDataFormat::FLOAT3 || apexFormat == nvidia::apex::RenderDataFormat::BYTE_SNORM3 ||
					  apexFormat == nvidia::apex::RenderDataFormat::FLOAT4 || apexFormat == nvidia::apex::RenderDataFormat::BYTE_SNORM4);
			// always use FLOAT4 for tangents!!!
			apexFormat = nvidia::apex::RenderDataFormat::FLOAT4;
		}
		else if (apexSemantic == nvidia::apex::RenderVertexSemantic::BONE_INDEX && apexFormat != nvidia::apex::RenderDataFormat::UNSPECIFIED)
		{
			PX_ASSERT(apexFormat == nvidia::apex::RenderDataFormat::USHORT1
				|| apexFormat == nvidia::apex::RenderDataFormat::USHORT2
				|| apexFormat == nvidia::apex::RenderDataFormat::USHORT3
				|| apexFormat == nvidia::apex::RenderDataFormat::USHORT4);

			// use either USHORT1 for destruction or USHORT4 for everything else. fill with 0 in writeBuffer
			if (apexFormat == nvidia::apex::RenderDataFormat::USHORT2 || apexFormat == nvidia::apex::RenderDataFormat::USHORT3)
			{
				apexFormat = nvidia::apex::RenderDataFormat::USHORT4;
			}
		}
		else if (apexSemantic == nvidia::apex::RenderVertexSemantic::BONE_WEIGHT && apexFormat != nvidia::apex::RenderDataFormat::UNSPECIFIED)
		{
			PX_ASSERT(apexFormat == nvidia::apex::RenderDataFormat::FLOAT1
				|| apexFormat == nvidia::apex::RenderDataFormat::FLOAT2
				|| apexFormat == nvidia::apex::RenderDataFormat::FLOAT3
				|| apexFormat == nvidia::apex::RenderDataFormat::FLOAT4);

			// use either FLOAT1 for destruction or FLOAT4 for everything else. fill with 0.0 in writeBuffer
			if (apexFormat == nvidia::apex::RenderDataFormat::FLOAT2 || apexFormat == nvidia::apex::RenderDataFormat::FLOAT3)
			{
				apexFormat = nvidia::apex::RenderDataFormat::FLOAT4;
			}
		}
		RendererVertexBuffer::Semantic semantic = convertFromApexVB(apexSemantic);
		RendererVertexBuffer::Format   format   = convertFromApexVB(apexFormat);
		if (semantic < RendererVertexBuffer::NUM_SEMANTICS && format < RendererVertexBuffer::NUM_FORMATS)
		{
			vbdesc.semanticFormats[semantic] = format;
		}
	}
#if APEX_CUDA_SUPPORT
	vbdesc.registerInCUDA = desc.registerInCUDA;
	vbdesc.interopContext = desc.interopContext;
#endif
	vbdesc.maxVertices = desc.maxVerts;
	m_vertexbuffer = m_renderer.createVertexBuffer(vbdesc);
	PX_ASSERT(m_vertexbuffer);
	m_uvOrigin = desc.uvOrigin;
}

SampleApexRendererVertexBuffer::~SampleApexRendererVertexBuffer(void)
{
	if (m_vertexbuffer)
	{
		m_vertexbuffer->release();
	}
}

bool SampleApexRendererVertexBuffer::getInteropResourceHandle(CUgraphicsResource& handle)
{
#if APEX_CUDA_SUPPORT
	if (m_vertexbuffer)
	{
		return	m_vertexbuffer->getInteropResourceHandle(handle);
	}
	else
	{
		return false;
	}
#else
	CUgraphicsResource* tmp = &handle;
	PX_UNUSED(tmp);

	return false;
#endif
}

void SampleApexRendererVertexBuffer::fixUVOrigin(void* uvdata, uint32_t stride, uint32_t num)
{
#define ITERATE_UVS(_uop,_vop)								\
	for(uint32_t i=0; i<num; i++)						\
	{														\
	float &u = *(((float*)uvdata) + 0);	\
	float &v = *(((float*)uvdata) + 1);	\
	u=_uop;												\
	v=_vop;												\
	uvdata = ((uint8_t*)uvdata)+stride;				\
}
	switch (m_uvOrigin)
	{
	case nvidia::apex::TextureUVOrigin::ORIGIN_BOTTOM_LEFT:
		// nothing to do...
		break;
	case nvidia::apex::TextureUVOrigin::ORIGIN_BOTTOM_RIGHT:
		ITERATE_UVS(1 - u, v);
		break;
	case nvidia::apex::TextureUVOrigin::ORIGIN_TOP_LEFT:
		ITERATE_UVS(u, 1 - v);
		break;
	case nvidia::apex::TextureUVOrigin::ORIGIN_TOP_RIGHT:
		ITERATE_UVS(1 - u, 1 - v);
		break;
	default:
		PX_ASSERT(0); // UNKNOWN ORIGIN / TODO!
	}
#undef ITERATE_UVS
}

void SampleApexRendererVertexBuffer::flipColors(void* uvData, uint32_t stride, uint32_t num)
{
	for (uint32_t i = 0; i < num; i++)
	{
		uint8_t* color = ((uint8_t*)uvData) + (stride * i);
		std::swap(color[0], color[3]);
		std::swap(color[1], color[2]);
	}
}

// special fast path for interleaved vec3 position and normal and optional vec4 tangent (avoids painfully slow strided writes to locked vertex buffer)
bool SampleApexRendererVertexBuffer::writeBufferFastPath(const nvidia::apex::RenderVertexBufferData& data, uint32_t firstVertex, uint32_t numVerts)
{
	for (uint32_t i = 0; i < nvidia::apex::RenderVertexSemantic::NUM_SEMANTICS; i++)
	{
		nvidia::apex::RenderVertexSemantic::Enum apexSemantic = (nvidia::apex::RenderVertexSemantic::Enum)i;
		switch(apexSemantic)
		{
		case nvidia::apex::RenderVertexSemantic::POSITION:
		case nvidia::apex::RenderVertexSemantic::NORMAL:
		case nvidia::apex::RenderVertexSemantic::TANGENT:
			break;
		default:
			if(data.getSemanticData(apexSemantic).data)
				return false;
		}
	}

	const nvidia::apex::RenderSemanticData& positionSemantic = data.getSemanticData(nvidia::apex::RenderVertexSemantic::POSITION);
	const nvidia::apex::RenderSemanticData& normalSemantic = data.getSemanticData(nvidia::apex::RenderVertexSemantic::NORMAL);
	const nvidia::apex::RenderSemanticData& tangentSemantic = data.getSemanticData(nvidia::apex::RenderVertexSemantic::TANGENT);

	const physx::PxVec3* PX_RESTRICT positionSrc = (const physx::PxVec3*)positionSemantic.data;
	const physx::PxVec3* PX_RESTRICT normalSrc = (const physx::PxVec3*)normalSemantic.data;
	const physx::PxVec4* PX_RESTRICT tangentSrc = (const physx::PxVec4*)tangentSemantic.data;

	RendererVertexBuffer::Format positionFormat = m_vertexbuffer->getFormatForSemantic(RendererVertexBuffer::SEMANTIC_POSITION);
	RendererVertexBuffer::Format normalFormat = m_vertexbuffer->getFormatForSemantic(RendererVertexBuffer::SEMANTIC_NORMAL);
	RendererVertexBuffer::Format tangentFormat = m_vertexbuffer->getFormatForSemantic(RendererVertexBuffer::SEMANTIC_TANGENT);

	if(positionSrc == 0 || positionSemantic.stride != 12 || RendererVertexBuffer::getFormatByteSize(positionFormat) != 12)
		return false;

	if(normalSrc == 0 || normalSemantic.stride != 12 || RendererVertexBuffer::getFormatByteSize(normalFormat) != 12)
		return false;

	if(tangentSrc != 0 && (tangentSemantic.stride != 16 || RendererVertexBuffer::getFormatByteSize(tangentFormat) != 16))
		return false;

	uint32_t stride = 0;
	void* positionDst = m_vertexbuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_POSITION, stride);
	void* normalDst = m_vertexbuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_NORMAL, stride);
	void* tangentDst = tangentSrc ? m_vertexbuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_TANGENT, stride) : 0;

	bool useFastPath = stride == (uint32_t)(tangentSrc ? 40 : 24);
	useFastPath &= normalDst == (uint8_t*)positionDst + 12;
	useFastPath &= !tangentSrc || tangentDst == (uint8_t*)positionDst + 24;

	if(useFastPath)
	{
		uint8_t* dstIt = (uint8_t*)positionDst + stride * firstVertex;
		uint8_t* dstEnd = dstIt + stride * numVerts;

		for(; dstIt < dstEnd; dstIt += stride)
		{
			*(physx::PxVec3* PX_RESTRICT)(dstIt   ) = *positionSrc++;
			*(physx::PxVec3* PX_RESTRICT)(dstIt+12) = *normalSrc++;
			if(tangentSrc)
				*(physx::PxVec4* PX_RESTRICT)(dstIt+24) = -*tangentSrc++;
		}
	}

	m_vertexbuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_POSITION);
	m_vertexbuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_NORMAL);
	if(tangentSrc)
		m_vertexbuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_TANGENT);

	return useFastPath;
}

void SampleApexRendererVertexBuffer::writeBuffer(const nvidia::apex::RenderVertexBufferData& data, uint32_t firstVertex, uint32_t numVerts)
{
	if (!m_vertexbuffer || !numVerts)
	{
		return;
	}

	if(writeBufferFastPath(data, firstVertex, numVerts))
		return;

	for (uint32_t i = 0; i < nvidia::apex::RenderVertexSemantic::NUM_SEMANTICS; i++)
	{
		nvidia::apex::RenderVertexSemantic::Enum apexSemantic = (nvidia::apex::RenderVertexSemantic::Enum)i;
		const nvidia::apex::RenderSemanticData& semanticData = data.getSemanticData(apexSemantic);
		if (semanticData.data)
		{
			const void* srcData = semanticData.data;
			const uint32_t srcStride = semanticData.stride;

			RendererVertexBuffer::Semantic semantic = convertFromApexVB(apexSemantic);
			if (semantic < RendererVertexBuffer::NUM_SEMANTICS)
			{
				RendererVertexBuffer::Format format = m_vertexbuffer->getFormatForSemantic(semantic);
				uint32_t semanticStride = 0;
				void* dstData = m_vertexbuffer->lockSemantic(semantic, semanticStride);
				void* dstDataCopy = dstData;
				PX_ASSERT(dstData && semanticStride);
				if (dstData && semanticStride)
				{
#if defined(RENDERER_DEBUG) && 0 // enable to confirm that destruction bone indices are within valid range.
					// verify input data...
					if (apexSemantic == RenderVertexSemantic::BONE_INDEX)
					{
						uint32_t maxIndex = 0;
						for (uint32_t i = 0; i < numVerts; i++)
						{
							uint16_t index = *(uint16_t*)(((uint8_t*)srcData) + (srcStride * i));
							if (index > maxIndex)
							{
								maxIndex = index;
							}
						}
						PX_ASSERT(maxIndex < RENDERER_MAX_BONES);
					}
#endif
					dstData = ((uint8_t*)dstData) + firstVertex * semanticStride;
					uint32_t formatSize = RendererVertexBuffer::getFormatByteSize(format);

					if ((apexSemantic == nvidia::apex::RenderVertexSemantic::NORMAL || apexSemantic == nvidia::apex::RenderVertexSemantic::BINORMAL) && semanticData.format == nvidia::apex::RenderDataFormat::BYTE_SNORM3)
					{
						for (uint32_t j = 0; j < numVerts; j++)
						{
							uint8_t* vector = (uint8_t*)srcData;
							*(physx::PxVec3*)dstData = physx::PxVec3(vector[0], vector[1], vector[2])/127.0f;
							dstData = ((uint8_t*)dstData) + semanticStride;
							srcData = ((uint8_t*)srcData) + srcStride;
						}
					}
					else if (apexSemantic == nvidia::apex::RenderVertexSemantic::TANGENT && semanticData.format == nvidia::apex::RenderDataFormat::FLOAT4)
					{
						// invert entire tangent
						for (uint32_t j = 0; j < numVerts; j++)
						{
							physx::PxVec4 tangent = *(physx::PxVec4*)srcData;
							*(physx::PxVec4*)dstData = -tangent;
							dstData = ((uint8_t*)dstData) + semanticStride;
							srcData = ((uint8_t*)srcData) + srcStride;
						}
					}
					else if (apexSemantic == nvidia::apex::RenderVertexSemantic::TANGENT && semanticData.format == nvidia::apex::RenderDataFormat::BYTE_SNORM4)
					{
						// invert entire tangent
						for (uint32_t j = 0; j < numVerts; j++)
						{
							uint8_t* tangent = (uint8_t*)srcData;
							*(physx::PxVec4*)dstData = physx::PxVec4(tangent[0], tangent[1], tangent[2], tangent[3])/-127.0f;
							dstData = ((uint8_t*)dstData) + semanticStride;
							srcData = ((uint8_t*)srcData) + srcStride;
						}
					}
					else if (apexSemantic == nvidia::apex::RenderVertexSemantic::TANGENT && (semanticData.format == nvidia::apex::RenderDataFormat::FLOAT3 || semanticData.format == nvidia::apex::RenderDataFormat::BYTE_SNORM3))
					{
						// we need to increase the data from 3 components to 4
						const nvidia::apex::RenderSemanticData& bitangentData = data.getSemanticData(nvidia::apex::RenderVertexSemantic::BINORMAL);
						const nvidia::apex::RenderSemanticData& normalData = data.getSemanticData(nvidia::apex::RenderVertexSemantic::NORMAL);
						if (bitangentData.format != nvidia::apex::RenderDataFormat::UNSPECIFIED && normalData.format != nvidia::apex::RenderDataFormat::UNSPECIFIED)
						{
							PX_ASSERT(bitangentData.format == nvidia::apex::RenderDataFormat::FLOAT3);
							const void* srcDataBitangent = bitangentData.data;
							const uint32_t srcStrideBitangent = bitangentData.stride;

							PX_ASSERT(normalData.format == nvidia::apex::RenderDataFormat::FLOAT3);
							const void* srcDataNormal = normalData.data;
							const uint32_t srcStrideNormal = normalData.stride;

							for (uint32_t j = 0; j < numVerts; j++)
							{
								physx::PxVec3 normal = normalData.format == nvidia::RenderDataFormat::FLOAT3 ? *(physx::PxVec3*)srcDataNormal :
									physx::PxVec3(((uint8_t*)srcDataNormal)[0], ((uint8_t*)srcDataNormal)[1], ((uint8_t*)srcDataNormal)[2])/127.0f;
								physx::PxVec3 bitangent = bitangentData.format == nvidia::RenderDataFormat::FLOAT3 ? *(physx::PxVec3*)srcDataBitangent :
									physx::PxVec3(((uint8_t*)srcDataBitangent)[0], ((uint8_t*)srcDataBitangent)[1], ((uint8_t*)srcDataBitangent)[2])/127.0f;
								physx::PxVec3 tangent = semanticData.format == nvidia::RenderDataFormat::FLOAT3 ? *(physx::PxVec3*)srcData :
									physx::PxVec3(((uint8_t*)srcData)[0], ((uint8_t*)srcData)[1], ((uint8_t*)srcData)[2])/127.0f;
								float tangentw = physx::PxSign(normal.cross(tangent).dot(bitangent));
								*(physx::PxVec4*)dstData = physx::PxVec4(tangent, -tangentw);

								dstData = ((uint8_t*)dstData) + semanticStride;
								srcData = ((uint8_t*)srcData) + srcStride;
								srcDataBitangent = ((uint8_t*)srcDataBitangent) + srcStrideBitangent;
								srcDataNormal = ((uint8_t*)srcDataNormal) + srcStrideNormal;
							}
						}
						else
						{
							// just assume 1.0 as tangent.w if there is no bitangent to calculate this from
							if (semanticData.format == nvidia::apex::RenderDataFormat::FLOAT3)
							{
								for (uint32_t j = 0; j < numVerts; j++)
								{
									physx::PxVec3 tangent = *(physx::PxVec3*)srcData;
									*(physx::PxVec4*)dstData = physx::PxVec4(tangent, 1.0f);
									dstData = ((uint8_t*)dstData) + semanticStride;
									srcData = ((uint8_t*)srcData) + srcStride;
								}
							}
							else
							{
								for (uint32_t j = 0; j < numVerts; j++)
								{
									uint8_t* tangent = (uint8_t*)srcData;
									*(physx::PxVec4*)dstData = physx::PxVec4(tangent[0]/127.0f, tangent[1]/127.0f, tangent[2]/127.0f, 1.0f);
									dstData = ((uint8_t*)dstData) + semanticStride;
									srcData = ((uint8_t*)srcData) + srcStride;
								}
							}
						}
					}
					else if (apexSemantic == nvidia::apex::RenderVertexSemantic::BONE_INDEX && (semanticData.format == nvidia::apex::RenderDataFormat::USHORT2 || semanticData.format == nvidia::apex::RenderDataFormat::USHORT3))
					{
						unsigned int numIndices = 0;
						switch (semanticData.format)
						{
						case nvidia::apex::RenderDataFormat::USHORT1: numIndices = 1; break;
						case nvidia::apex::RenderDataFormat::USHORT2: numIndices = 2; break;
						case nvidia::apex::RenderDataFormat::USHORT3: numIndices = 3; break;
						default:
							PX_ALWAYS_ASSERT();
							break;
						}

						for (uint32_t j = 0; j < numVerts; j++)
						{
							unsigned short* boneIndices = (unsigned short*)srcData;
							unsigned short* dstBoneIndices = (unsigned short*)dstData;

							for (unsigned int i = 0; i < numIndices; i++)
							{
								dstBoneIndices[i] = boneIndices[i];
							}
							for (unsigned int i = numIndices; i < 4; i++)
							{
								dstBoneIndices[i] = 0;
							}

							dstData = ((uint8_t*)dstData) + semanticStride;
							srcData = ((uint8_t*)srcData) + srcStride;
						}
					}
					else if (apexSemantic == nvidia::apex::RenderVertexSemantic::BONE_WEIGHT && (semanticData.format == nvidia::apex::RenderDataFormat::FLOAT2 || semanticData.format == nvidia::apex::RenderDataFormat::FLOAT3))
					{
						unsigned int numWeights = 0;
						switch (semanticData.format)
						{
						case nvidia::apex::RenderDataFormat::FLOAT1: numWeights = 1; break;
						case nvidia::apex::RenderDataFormat::FLOAT2: numWeights = 2; break;
						case nvidia::apex::RenderDataFormat::FLOAT3: numWeights = 3; break;
						default:
							PX_ALWAYS_ASSERT();
							break;
						}

						for (uint32_t j = 0; j < numVerts; j++)
						{
							float* boneIndices = (float*)srcData;
							float* dstBoneIndices = (float*)dstData;

							for (unsigned int i = 0; i < numWeights; i++)
							{
								dstBoneIndices[i] = boneIndices[i];
							}
							for (unsigned int i = numWeights; i < 4; i++)
							{
								dstBoneIndices[i] = 0.0f;
							}

							dstData = ((uint8_t*)dstData) + semanticStride;
							srcData = ((uint8_t*)srcData) + srcStride;
						}
					}
					else if (formatSize == 4)
					{
						for (uint32_t j = 0; j < numVerts; j++)
						{
							*(uint32_t*)dstData = *(uint32_t*)srcData;
							dstData = ((uint8_t*)dstData) + semanticStride;
							srcData = ((uint8_t*)srcData) + srcStride;
						}
					}
					else if (formatSize == 12)
					{
						for (uint32_t j = 0; j < numVerts; j++)
						{
							*(physx::PxVec3*)dstData = *(physx::PxVec3*)srcData;
							dstData = ((uint8_t*)dstData) + semanticStride;
							srcData = ((uint8_t*)srcData) + srcStride;
						}
					}
					else
					{
						for (uint32_t j = 0; j < numVerts; j++)
						{
							memcpy(dstData, srcData, formatSize);
							dstData = ((uint8_t*)dstData) + semanticStride;
							srcData = ((uint8_t*)srcData) + srcStride;
						}
					}

					// fix-up the UVs...
					if ((semantic >= RendererVertexBuffer::SEMANTIC_TEXCOORD0  &&
						semantic <= RendererVertexBuffer::SEMANTIC_TEXCOORDMAX) ||
						semantic == RendererVertexBuffer::SEMANTIC_DISPLACEMENT_TEXCOORD)
					{
						fixUVOrigin(dstDataCopy, semanticStride, numVerts);
					}
				}
				m_vertexbuffer->unlockSemantic(semantic);
			}
		}
	}
}


/********************************
* SampleApexRendererIndexBuffer *
********************************/

SampleApexRendererIndexBuffer::SampleApexRendererIndexBuffer(SampleRenderer::Renderer& renderer, const nvidia::apex::UserRenderIndexBufferDesc& desc) :
	m_renderer(renderer)
{
	m_indexbuffer = 0;
	SampleRenderer::RendererIndexBufferDesc ibdesc;
	ibdesc.hint       = convertFromApexIB(desc.hint);
	ibdesc.format     = convertFromApexIB(desc.format);
	ibdesc.maxIndices = desc.maxIndices;
#if APEX_CUDA_SUPPORT
	ibdesc.registerInCUDA = desc.registerInCUDA;
	ibdesc.interopContext = desc.interopContext;
#endif
	m_indexbuffer = m_renderer.createIndexBuffer(ibdesc);
	PX_ASSERT(m_indexbuffer);

	m_primitives = desc.primitives;
}

bool SampleApexRendererIndexBuffer::getInteropResourceHandle(CUgraphicsResource& handle)
{
#if APEX_CUDA_SUPPORT
	if (m_indexbuffer)
	{
		return	m_indexbuffer->getInteropResourceHandle(handle);
	}
	else
	{
		return false;
	}
#else
	CUgraphicsResource* tmp = &handle;
	PX_UNUSED(tmp);

	return false;
#endif
}

SampleApexRendererIndexBuffer::~SampleApexRendererIndexBuffer(void)
{
	if (m_indexbuffer)
	{
		m_indexbuffer->release();
	}
}

void SampleApexRendererIndexBuffer::writeBuffer(const void* srcData, uint32_t srcStride, uint32_t firstDestElement, uint32_t numElements)
{
	if (m_indexbuffer && numElements)
	{
		void* dstData = m_indexbuffer->lock();
		PX_ASSERT(dstData);
		if (dstData)
		{
			RendererIndexBuffer::Format format = m_indexbuffer->getFormat();

			if (m_primitives == nvidia::apex::RenderPrimitiveType::TRIANGLES)
			{
				uint32_t numTriangles = numElements / 3;
				if (format == RendererIndexBuffer::FORMAT_UINT16)
				{
					uint16_t*       dst = ((uint16_t*)dstData) + firstDestElement;
					const uint16_t* src = (const uint16_t*)srcData;
					for (uint32_t i = 0; i < numTriangles; i++)
						for (uint32_t j = 0; j < 3; j++)
						{
							dst[i * 3 + j] = src[i * 3 + (2 - j)];
						}
				}
				else if (format == RendererIndexBuffer::FORMAT_UINT32)
				{
					uint32_t*       dst = ((uint32_t*)dstData) + firstDestElement;
					const uint32_t* src = (const uint32_t*)srcData;
					for (uint32_t i = 0; i < numTriangles; i++)
						for (uint32_t j = 0; j < 3; j++)
						{
							dst[i * 3 + j] = src[i * 3 + (2 - j)];
						}
				}
				else
				{
					PX_ASSERT(0);
				}
			}
			else
			{
				if (format == RendererIndexBuffer::FORMAT_UINT16)
				{
					uint16_t*		dst = ((uint16_t*)dstData) + firstDestElement;
					const uint8_t*	src = (const uint8_t*)srcData;
					for (uint32_t i = 0; i < numElements; i++, dst++, src += srcStride)
					{
						*dst = *((uint16_t*)src);
					}
				}
				else if (format == RendererIndexBuffer::FORMAT_UINT32)
				{
					uint32_t*		dst = ((uint32_t*)dstData) + firstDestElement;
					const uint8_t*	src = (const uint8_t*)srcData;
					for (uint32_t i = 0; i < numElements; i++, dst++, src += srcStride)
					{
						*dst = *((uint32_t*)src);
					}
				}
				else
				{
					PX_ASSERT(0);
				}
			}
		}
		m_indexbuffer->unlock();
	}
}


/********************************
* SampleApexRendererSurfaceBuffer *
********************************/

SampleApexRendererSurfaceBuffer::SampleApexRendererSurfaceBuffer(SampleRenderer::Renderer& renderer, const nvidia::apex::UserRenderSurfaceBufferDesc& desc) :
	m_renderer(renderer)
{
	m_texture = 0;
	SampleRenderer::RendererTextureDesc tdesc;
	//tdesc.hint       = convertFromApexSB(desc.hint);
	tdesc.format		= convertFromApexSB(desc.format);
	tdesc.width			= desc.width;
	tdesc.height		= desc.height;
	tdesc.depth			= desc.depth;
	tdesc.filter		= SampleRenderer::RendererTexture::FILTER_NEAREST; // we don't need interpolation at all
	tdesc.addressingU	= SampleRenderer::RendererTexture::ADDRESSING_CLAMP;
	tdesc.addressingV	= SampleRenderer::RendererTexture::ADDRESSING_CLAMP;
	tdesc.addressingW	= SampleRenderer::RendererTexture::ADDRESSING_CLAMP;
	tdesc.numLevels		= 1;

#if APEX_CUDA_SUPPORT
	tdesc.registerInCUDA = desc.registerInCUDA;
	tdesc.interopContext = desc.interopContext;
#endif
	
	m_texture = m_renderer.createTexture(tdesc);
	PX_ASSERT(m_texture);
}

SampleApexRendererSurfaceBuffer::~SampleApexRendererSurfaceBuffer(void)
{
	if (m_texture)
	{
		m_texture->release();
	}
}

void SampleApexRendererSurfaceBuffer::writeBuffer(const void* srcData, uint32_t srcPitch, uint32_t srcHeight, uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t width, uint32_t height, uint32_t depth)
{
	if (m_texture && width && height && depth)
	{
		const RendererTexture::Format format = m_texture->getFormat();
		PX_ASSERT(format < RendererTexture::NUM_FORMATS);
		PX_ASSERT(RendererTexture::isCompressedFormat(format) == false);
		PX_ASSERT(RendererTexture::isDepthStencilFormat(format) == false);

		const uint32_t texelSize = RendererTexture::getFormatBlockSize(format);
		const uint32_t dstXInBytes = dstX * texelSize;
		const uint32_t widthInBytes = width * texelSize;

		const uint32_t dstHeight = m_texture->getHeight();
		uint32_t dstPitch;
		void* dstData = m_texture->lockLevel(0, dstPitch);
		PX_ASSERT(dstData);
		if (dstData)
		{
			uint8_t* dst = ((uint8_t*)dstData) + dstXInBytes + dstPitch * (dstY + dstHeight * dstZ);
			const uint8_t* src = (const uint8_t*)srcData;
			for (uint32_t z = 0; z < depth; z++)
			{
				uint8_t* dstLine = dst;
				const uint8_t* srcLine = src;
				for (uint32_t y = 0; y < height; y++)
				{
					memcpy(dstLine, srcLine, widthInBytes);
					dstLine += dstPitch;
					srcLine += srcPitch;
				}
				dst += dstPitch * dstHeight;
				src += srcPitch * srcHeight;
			}
		}
		m_texture->unlockLevel(0);
	}
}

bool SampleApexRendererSurfaceBuffer::getInteropResourceHandle(CUgraphicsResource& handle)
{
#if APEX_CUDA_SUPPORT
	return (m_texture != 0) && m_texture->getInteropResourceHandle(handle);
#else
	CUgraphicsResource* tmp = &handle;
	PX_UNUSED(tmp);

	return false;
#endif
}


/*******************************
* SampleApexRendererBoneBuffer *
*******************************/

SampleApexRendererBoneBuffer::SampleApexRendererBoneBuffer(SampleRenderer::Renderer& renderer, const nvidia::apex::UserRenderBoneBufferDesc& desc) :
	m_renderer(renderer)
{
	m_boneTexture = 0;
	m_maxBones = desc.maxBones;
	m_bones    = 0;

	if(m_renderer.isVTFEnabled())	// Create vertex texture to hold bone matrices
	{	
		SampleRenderer::RendererTextureDesc textureDesc;
		textureDesc.format = RendererTexture::FORMAT_R32F_G32F_B32G_A32F;
		textureDesc.filter = RendererTexture::FILTER_NEAREST;
		textureDesc.addressingU = RendererTexture::ADDRESSING_CLAMP;
		textureDesc.addressingV = RendererTexture::ADDRESSING_CLAMP;
		textureDesc.addressingU = RendererTexture::ADDRESSING_CLAMP;
		textureDesc.width = 4;	// 4x4 matrix per row
		textureDesc.height = m_maxBones;
		textureDesc.depth = 1;
		textureDesc.numLevels = 1;

		m_boneTexture = renderer.createTexture(textureDesc);
	}
	else
	{
		m_bones    = new physx::PxMat44[m_maxBones];
	}
}

SampleApexRendererBoneBuffer::~SampleApexRendererBoneBuffer(void)
{
	if(m_boneTexture)
		m_boneTexture->release();

	if (m_bones)
	{
		delete [] m_bones;
	}
}

void SampleApexRendererBoneBuffer::writeBuffer(const nvidia::apex::RenderBoneBufferData& data, uint32_t firstBone, uint32_t numBones)
{
	const nvidia::apex::RenderSemanticData& semanticData = data.getSemanticData(nvidia::apex::RenderBoneSemantic::POSE);
	if(!semanticData.data)
		return;

	if(m_boneTexture)	// Write bone matrices into vertex texture
	{
		const void* srcData = semanticData.data;
		const uint32_t srcStride = semanticData.stride;

		unsigned pitch = 0;
		unsigned char* textureData = static_cast<unsigned char*>(m_boneTexture->lockLevel(0, pitch));
		if(textureData)
		{
			for(uint32_t row = firstBone; row < firstBone + numBones; ++row)
			{
				physx::PxMat44* mat = (physx::PxMat44*)(textureData + row * pitch);

				*mat = static_cast<const physx::PxMat44*>(srcData)->getTranspose();

				srcData = ((uint8_t*)srcData) + srcStride;				
			}

			m_boneTexture->unlockLevel(0);
		}
	}
	else if(m_bones)
	{
		const void* srcData = semanticData.data;
		const uint32_t srcStride = semanticData.stride;
		for (uint32_t i = 0; i < numBones; i++)
		{
			// the bones are stored in physx::PxMat44
			m_bones[firstBone + i] = *(const physx::PxMat44*)srcData;
			srcData = ((uint8_t*)srcData) + srcStride;
		}
	}
}


/***********************************
* SampleApexRendererInstanceBuffer *
***********************************/
#if 0
float lifeRemain[0x10000]; //64k
#endif
SampleApexRendererInstanceBuffer::SampleApexRendererInstanceBuffer(SampleRenderer::Renderer& renderer, const nvidia::apex::UserRenderInstanceBufferDesc& desc)
{
	m_maxInstances   = desc.maxInstances;
	m_instanceBuffer = 0;

	SampleRenderer::RendererInstanceBufferDesc ibdesc;

	ibdesc.hint = RendererInstanceBuffer::HINT_DYNAMIC;
	ibdesc.maxInstances = desc.maxInstances;

	for (uint32_t i = 0; i < RendererInstanceBuffer::NUM_SEMANTICS; i++)
	{
		ibdesc.semanticFormats[i] = RendererInstanceBuffer::NUM_FORMATS;
	}

	for (uint32_t i = 0; i < nvidia::apex::RenderInstanceLayoutElement::NUM_SEMANTICS; i++)
	{
		// Skip unspecified, but if it IS specified, IGNORE the specification and make your own up!! yay!
		if (desc.semanticOffsets[i] == uint32_t(-1))
		{
			continue;
		}

		switch (i)
		{
		case nvidia::apex::RenderInstanceLayoutElement::POSITION_FLOAT3:
			ibdesc.semanticFormats[RendererInstanceBuffer::SEMANTIC_POSITION] = RendererInstanceBuffer::FORMAT_FLOAT3;
			break;
		case nvidia::apex::RenderInstanceLayoutElement::ROTATION_SCALE_FLOAT3x3:
			ibdesc.semanticFormats[RendererInstanceBuffer::SEMANTIC_NORMALX] = RendererInstanceBuffer::FORMAT_FLOAT3;
			ibdesc.semanticFormats[RendererInstanceBuffer::SEMANTIC_NORMALY] = RendererInstanceBuffer::FORMAT_FLOAT3;
			ibdesc.semanticFormats[RendererInstanceBuffer::SEMANTIC_NORMALZ] = RendererInstanceBuffer::FORMAT_FLOAT3;
			break;
		case nvidia::apex::RenderInstanceLayoutElement::VELOCITY_LIFE_FLOAT4:
			ibdesc.semanticFormats[RendererInstanceBuffer::SEMANTIC_VELOCITY_LIFE] = RendererInstanceBuffer::FORMAT_FLOAT4;
			break;
		case nvidia::apex::RenderInstanceLayoutElement::DENSITY_FLOAT1:
			ibdesc.semanticFormats[RendererInstanceBuffer::SEMANTIC_DENSITY] = RendererInstanceBuffer::FORMAT_FLOAT1;
			break;
		case nvidia::apex::RenderInstanceLayoutElement::UV_OFFSET_FLOAT2:
			ibdesc.semanticFormats[RendererInstanceBuffer::SEMANTIC_UV_OFFSET] = RendererInstanceBuffer::FORMAT_FLOAT2;
			break;
		case nvidia::apex::RenderInstanceLayoutElement::LOCAL_OFFSET_FLOAT3:
			ibdesc.semanticFormats[RendererInstanceBuffer::SEMANTIC_LOCAL_OFFSET] = RendererInstanceBuffer::FORMAT_FLOAT3;
			break;
		}
	}

#if APEX_CUDA_SUPPORT
	ibdesc.registerInCUDA = desc.registerInCUDA;
	ibdesc.interopContext = desc.interopContext;
#endif
	m_instanceBuffer = renderer.createInstanceBuffer(ibdesc);
}

SampleApexRendererInstanceBuffer::~SampleApexRendererInstanceBuffer(void)
{
	if (m_instanceBuffer)
	{
		m_instanceBuffer->release();
	}
}

void SampleApexRendererInstanceBuffer::writeBuffer(const void* srcData, uint32_t firstInstance, uint32_t numInstances)
{
	/* Find beginning of the destination data buffer */
	RendererInstanceBuffer::Semantic semantic = (RendererInstanceBuffer::Semantic)0;
	for(uint32_t i = 0; i <= RendererInstanceBuffer::NUM_SEMANTICS; i++)
	{
		if(i == RendererVertexBuffer::NUM_SEMANTICS)
		{
			PX_ASSERT(0 && "Couldn't find any semantic in the VBO having zero offset");
		}
		semantic = static_cast<RendererInstanceBuffer::Semantic>(i);;
		RendererInstanceBuffer::Format format = m_instanceBuffer->getFormatForSemantic(semantic);
		if(format != RendererInstanceBuffer::NUM_FORMATS && m_instanceBuffer->getOffsetForSemantic(semantic) == 0)
		{
			break;
		}
	}
	uint32_t dstStride = 0;
	void* dstData = m_instanceBuffer->lockSemantic(semantic, dstStride);
	::memcpy(static_cast<char*>(dstData) + firstInstance * dstStride, srcData, dstStride * numInstances);
	m_instanceBuffer->unlockSemantic(semantic);
}

bool SampleApexRendererInstanceBuffer::writeBufferFastPath(const nvidia::apex::RenderInstanceBufferData& data, uint32_t firstInstance, uint32_t numInstances)
{
	const void* srcData = reinterpret_cast<void*>(~0);
	uint32_t srcStride = 0;
	/* Find beginning of the source data buffer */
	for (uint32_t i = 0; i < nvidia::apex::RenderInstanceSemantic::NUM_SEMANTICS; i++)
	{
		nvidia::apex::RenderInstanceSemantic::Enum semantic = static_cast<nvidia::apex::RenderInstanceSemantic::Enum>(i);
		const nvidia::apex::RenderSemanticData& semanticData = data.getSemanticData(semantic);
		if(semanticData.data && semanticData.data < srcData) 
		{
			srcData = semanticData.data;
			srcStride = semanticData.stride;
		}
	}
	/* Find beginning of the destination data buffer */
	RendererInstanceBuffer::Semantic semantic = (RendererInstanceBuffer::Semantic)0;
	for(uint32_t i = 0; i <= RendererInstanceBuffer::NUM_SEMANTICS; i++)
	{
		if(i == RendererVertexBuffer::NUM_SEMANTICS)
		{
			PX_ASSERT(0 && "Couldn't find any semantic in the VBO having zero offset");
			return false;
		}
		semantic = static_cast<RendererInstanceBuffer::Semantic>(i);;
		RendererInstanceBuffer::Format format = m_instanceBuffer->getFormatForSemantic(semantic);
		if(format != RendererInstanceBuffer::NUM_FORMATS && m_instanceBuffer->getOffsetForSemantic(semantic) == 0)
		{
			break;
		}
	}
	uint32_t dstStride = 0;
	void* dstData = m_instanceBuffer->lockSemantic(semantic, dstStride);
	bool stridesEqual = false;
	if(dstStride == srcStride) 
	{
		stridesEqual = true;
		::memcpy(static_cast<char*>(dstData) + firstInstance * dstStride, srcData, dstStride * numInstances);
	}
	m_instanceBuffer->unlockSemantic(semantic);
	if(stridesEqual)
		return true;
	else
		return false;
}

void SampleApexRendererInstanceBuffer::internalWriteBuffer(nvidia::apex::RenderInstanceSemantic::Enum semantic, const void* srcData, uint32_t srcStride,
														   uint32_t firstDestElement, uint32_t numElements)
{
	if (semantic == nvidia::apex::RenderInstanceSemantic::POSITION)
	{
		internalWriteSemantic<physx::PxVec3>(RendererInstanceBuffer::SEMANTIC_POSITION, srcData, srcStride, firstDestElement, numElements);
	}
	else if (semantic == nvidia::apex::RenderInstanceSemantic::ROTATION_SCALE)
	{
		PX_ASSERT(m_instanceBuffer);

		uint32_t xnormalsStride = 0;
		uint8_t* xnormals = (uint8_t*)m_instanceBuffer->lockSemantic(RendererInstanceBuffer::SEMANTIC_NORMALX, xnormalsStride);
		uint32_t ynormalsStride = 0;
		uint8_t* ynormals = (uint8_t*)m_instanceBuffer->lockSemantic(RendererInstanceBuffer::SEMANTIC_NORMALY, ynormalsStride);
		uint32_t znormalsStride = 0;
		uint8_t* znormals = (uint8_t*)m_instanceBuffer->lockSemantic(RendererInstanceBuffer::SEMANTIC_NORMALZ, znormalsStride);

		PX_ASSERT(xnormals && ynormals && znormals);

		if (xnormals && ynormals && znormals)
		{
			xnormals += firstDestElement * xnormalsStride;
			ynormals += firstDestElement * ynormalsStride;
			znormals += firstDestElement * znormalsStride;

			for (uint32_t i = 0; i < numElements; i++)
			{
				physx::PxVec3* p = (physx::PxVec3*)(((uint8_t*)srcData) + srcStride * i);

				*((physx::PxVec3*)xnormals) = p[0];
				*((physx::PxVec3*)ynormals) = p[1];
				*((physx::PxVec3*)znormals) = p[2];

				xnormals += xnormalsStride;
				ynormals += ynormalsStride;
				znormals += znormalsStride;
			}
		}
		m_instanceBuffer->unlockSemantic(RendererInstanceBuffer::SEMANTIC_NORMALX);
		m_instanceBuffer->unlockSemantic(RendererInstanceBuffer::SEMANTIC_NORMALY);
		m_instanceBuffer->unlockSemantic(RendererInstanceBuffer::SEMANTIC_NORMALZ);
	}
	else if (semantic == nvidia::apex::RenderInstanceSemantic::VELOCITY_LIFE)
	{
		internalWriteSemantic<physx::PxVec4>(RendererInstanceBuffer::SEMANTIC_VELOCITY_LIFE, srcData, srcStride, firstDestElement, numElements);
	}
	else if (semantic == nvidia::apex::RenderInstanceSemantic::DENSITY)
	{
		internalWriteSemantic<float>(RendererInstanceBuffer::SEMANTIC_DENSITY, srcData, srcStride, firstDestElement, numElements);
	}
	else if (semantic == nvidia::apex::RenderInstanceSemantic::UV_OFFSET)
	{
		internalWriteSemantic<physx::PxVec2>(RendererInstanceBuffer::SEMANTIC_UV_OFFSET, srcData, srcStride, firstDestElement, numElements);
	}
	else if (semantic == nvidia::apex::RenderInstanceSemantic::LOCAL_OFFSET)
	{
		internalWriteSemantic<physx::PxVec3>(RendererInstanceBuffer::SEMANTIC_LOCAL_OFFSET, srcData, srcStride, firstDestElement, numElements);
	}
}

bool SampleApexRendererInstanceBuffer::getInteropResourceHandle(CUgraphicsResource& handle)
{
#if APEX_CUDA_SUPPORT
	if (m_instanceBuffer)
	{
		return	m_instanceBuffer->getInteropResourceHandle(handle);
	}
	else
	{
		return false;
	}
#else
	CUgraphicsResource* tmp = &handle;
	PX_UNUSED(tmp);

	return false;
#endif
}

#if USE_RENDER_SPRITE_BUFFER

/*********************************
* SampleApexRendererSpriteBuffer *
*********************************/

SampleApexRendererSpriteBuffer::SampleApexRendererSpriteBuffer(SampleRenderer::Renderer& renderer, const nvidia::apex::UserRenderSpriteBufferDesc& desc) :
	m_renderer(renderer)
{
	memset(m_textures, 0, sizeof(m_textures));
	memset(m_textureIndexFromLayoutType, -1, sizeof(m_textureIndexFromLayoutType));
	m_vertexbuffer = 0;
	m_texturesCount = desc.textureCount;
	SampleRenderer::RendererVertexBufferDesc vbdesc;
	vbdesc.hint = convertFromApexVB(desc.hint);
#if APEX_CUDA_SUPPORT
	vbdesc.registerInCUDA = desc.registerInCUDA;
	vbdesc.interopContext = desc.interopContext;
#endif
	/* TODO: there is no need to create VBO if desc.textureCount > 0. Document this.
	   Maybe it's better to change APEX so that desc will not require creating VBOs
	   in case desc.textureCount > 0 */
	if(m_texturesCount == 0) 
	{
		for (uint32_t i = 0; i < nvidia::apex::RenderSpriteLayoutElement::NUM_SEMANTICS; i++)
		{
			if(desc.semanticOffsets[i] == static_cast<uint32_t>(-1)) continue;
			RendererVertexBuffer::Semantic semantic = convertFromApexLayoutSB((nvidia::apex::RenderSpriteLayoutElement::Enum)i);
			RendererVertexBuffer::Format   format   = convertFromApexLayoutVB((nvidia::apex::RenderSpriteLayoutElement::Enum)i);
			if (semantic < RendererVertexBuffer::NUM_SEMANTICS && format < RendererVertexBuffer::NUM_FORMATS)
			{
				vbdesc.semanticFormats[semantic] = format;
			}
		}
	} 
	else
	{
		vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_POSITION] = RendererVertexBuffer::FORMAT_FLOAT1;
		vbdesc.hint = RendererVertexBuffer::HINT_STATIC;
		vbdesc.registerInCUDA = false;
		vbdesc.interopContext = NULL;
	}
	vbdesc.maxVertices = desc.maxSprites;
	m_vertexbuffer = m_renderer.createVertexBuffer(vbdesc);
	PX_ASSERT(m_vertexbuffer);

	if(desc.textureCount != 0) 
	{
		uint32_t semanticStride = 0;
		void* dstData = m_vertexbuffer->lockSemantic(SampleRenderer::RendererVertexBuffer::SEMANTIC_POSITION, semanticStride);
		PX_ASSERT(dstData && semanticStride);
		if (dstData && semanticStride)
		{
			uint32_t formatSize = SampleRenderer::RendererVertexBuffer::getFormatByteSize(SampleRenderer::RendererVertexBuffer::FORMAT_FLOAT1);
			for (uint32_t j = 0; j < desc.maxSprites; j++)
			{
				float index = j * 1.0f;
				memcpy(dstData, &index, formatSize);
				dstData = ((uint8_t*)dstData) + semanticStride;
			}
		}
		m_vertexbuffer->unlockSemantic(SampleRenderer::RendererVertexBuffer::SEMANTIC_POSITION);
	}
	/* Create textures for the user-defined layout */
	for (uint32_t i = 0; i < m_texturesCount; i++)
	{
		SampleRenderer::RendererTextureDesc texdesc;
		switch(desc.textureDescs[i].layout) {
			case nvidia::apex::RenderSpriteTextureLayout::POSITION_FLOAT4:
			case nvidia::apex::RenderSpriteTextureLayout::COLOR_FLOAT4:
			case nvidia::apex::RenderSpriteTextureLayout::SCALE_ORIENT_SUBTEX_FLOAT4:
				texdesc.format = SampleRenderer::RendererTexture::FORMAT_R32F_G32F_B32G_A32F;
				break;
			case nvidia::apex::RenderSpriteTextureLayout::COLOR_BGRA8:
				texdesc.format = SampleRenderer::RendererTexture::FORMAT_B8G8R8A8;
				break;
			default: PX_ASSERT("Unknown sprite texture layout");
		}
		texdesc.width = desc.textureDescs[i].width;
		texdesc.height = desc.textureDescs[i].height;
		texdesc.filter = SampleRenderer::RendererTexture::FILTER_NEAREST; // we don't need interpolation at all
		texdesc.addressingU = SampleRenderer::RendererTexture::ADDRESSING_CLAMP;
		texdesc.addressingV = SampleRenderer::RendererTexture::ADDRESSING_CLAMP;
		texdesc.addressingW = SampleRenderer::RendererTexture::ADDRESSING_CLAMP;
		texdesc.numLevels = 1;
		texdesc.registerInCUDA = desc.registerInCUDA;
		texdesc.interopContext = desc.interopContext;
		m_textureIndexFromLayoutType[desc.textureDescs[i].layout] = i;
		m_textures[i] = m_renderer.createTexture(texdesc);
		PX_ASSERT(m_textures[i]);
	}
}

SampleRenderer::RendererTexture* SampleApexRendererSpriteBuffer::getTexture(const nvidia::apex::RenderSpriteTextureLayout::Enum e) const
{
	if(e > nvidia::apex::RenderSpriteTextureLayout::NONE && 
		e < nvidia::apex::RenderSpriteTextureLayout::NUM_LAYOUTS) 
	{
		return m_textures[m_textureIndexFromLayoutType[e]];
	}
	return NULL;
}

uint32_t SampleApexRendererSpriteBuffer::getTexturesCount() const 
{
	return m_texturesCount;
}

bool SampleApexRendererSpriteBuffer::getInteropResourceHandle(CUgraphicsResource& handle)
{
#if APEX_CUDA_SUPPORT
	return (m_vertexbuffer != 0) && m_vertexbuffer->getInteropResourceHandle(handle);
#else
	CUgraphicsResource* tmp = &handle;
	PX_UNUSED(tmp);

	return false;
#endif
}

bool SampleApexRendererSpriteBuffer::getInteropTextureHandleList(CUgraphicsResource* handleList)
{
#if APEX_CUDA_SUPPORT
	for(uint32_t i = 0; i < m_texturesCount; ++i)
	{
		bool result = (m_textures[i] != 0) && m_textures[i]->getInteropResourceHandle(handleList[i]);
		if (!result)
		{
			return false;
		}
	}
	return true;
#else
	PX_UNUSED(handleList);

	return false;
#endif
}

SampleApexRendererSpriteBuffer::~SampleApexRendererSpriteBuffer(void)
{
	if (m_vertexbuffer)
	{
		m_vertexbuffer->release();
	}
	for(uint32_t i = 0; i < m_texturesCount; ++i)
	{
		if(m_textures[i]) 
		{
			m_textures[i]->release();
		}
	}
}

void SampleApexRendererSpriteBuffer::flipColors(void* uvData, uint32_t stride, uint32_t num)
{
	for (uint32_t i = 0; i < num; i++)
	{
		uint8_t* color = ((uint8_t*)uvData) + (stride * i);
		std::swap(color[0], color[3]);
		std::swap(color[1], color[2]);
	}
}

void SampleApexRendererSpriteBuffer::writeBuffer(const void* srcData, uint32_t firstSprite, uint32_t numSprites)
{
	/* Find beginning of the destination data buffer */
	RendererVertexBuffer::Semantic semantic = (RendererVertexBuffer::Semantic)0;
	for(uint32_t i = 0; i <= RendererVertexBuffer::NUM_SEMANTICS; i++)
	{
		if(i == RendererVertexBuffer::NUM_SEMANTICS)
		{
			PX_ASSERT(0 && "Couldn't find any semantic in the VBO having zero offset");
		}
		semantic = static_cast<RendererVertexBuffer::Semantic>(i);;
		RendererVertexBuffer::Format format = m_vertexbuffer->getFormatForSemantic(semantic);
		if(format != RendererVertexBuffer::NUM_FORMATS && m_vertexbuffer->getOffsetForSemantic(semantic) == 0)
		{
			break;
		}
	}
	uint32_t dstStride = 0;
	void* dstData = m_vertexbuffer->lockSemantic(semantic, dstStride);
	::memcpy(static_cast<char*>(dstData) + firstSprite * dstStride, srcData, dstStride * numSprites);
	m_vertexbuffer->unlockSemantic(semantic);
}

void SampleApexRendererSpriteBuffer::writeTexture(uint32_t textureId, uint32_t numSprites, const void* srcData, size_t srcSize)
{
	PX_ASSERT((textureId < m_texturesCount) && "Invalid sprite texture id!");
	if(textureId < m_texturesCount) 
	{
		PX_ASSERT(m_textures[textureId] && "Sprite texture is not initialized");
		if(m_textures[textureId])
		{
			uint32_t pitch;
			void* dstData = m_textures[textureId]->lockLevel(0, pitch);
			PX_ASSERT(dstData);
			uint32_t size = numSprites * (pitch / m_textures[textureId]->getWidth());
			memcpy(dstData, srcData, size);
			m_textures[textureId]->unlockLevel(0);
		}
	}
}

#endif /* USE_RENDER_SPRITE_BUFFER */


/*************************
* SampleApexRendererMesh *
*************************/
SampleApexRendererMesh::SampleApexRendererMesh(SampleRenderer::Renderer& renderer, const nvidia::apex::UserRenderResourceDesc& desc) :
	m_renderer(renderer)
{
	m_vertexBuffers     = 0;
	m_numVertexBuffers  = 0;
	m_indexBuffer       = 0;

	m_boneBuffer        = 0;
	m_firstBone         = 0;
	m_numBones          = 0;

	m_instanceBuffer    = 0;

	m_mesh              = 0;

	m_meshTransform = physx::PxMat44(physx::PxIdentity);

	m_numVertexBuffers = desc.numVertexBuffers;
	if (m_numVertexBuffers > 0)
	{
		m_vertexBuffers = new SampleApexRendererVertexBuffer*[m_numVertexBuffers];
		for (uint32_t i = 0; i < m_numVertexBuffers; i++)
		{
			m_vertexBuffers[i] = static_cast<SampleApexRendererVertexBuffer*>(desc.vertexBuffers[i]);
		}
	}

	uint32_t numVertexBuffers = m_numVertexBuffers;
#if USE_RENDER_SPRITE_BUFFER
	m_spriteBuffer = static_cast<SampleApexRendererSpriteBuffer*>(desc.spriteBuffer);
	if (m_spriteBuffer)
	{
		numVertexBuffers = 1;
	}
#endif

	RendererVertexBuffer** internalsvbs = 0;
	if (numVertexBuffers > 0)
	{
		internalsvbs    = new RendererVertexBuffer*[numVertexBuffers];

#if USE_RENDER_SPRITE_BUFFER
		if (m_spriteBuffer)
		{
			internalsvbs[0] = m_spriteBuffer->m_vertexbuffer;
		}
		else
#endif
		{
			for (uint32_t i = 0; i < m_numVertexBuffers; i++)
			{
				internalsvbs[i] = m_vertexBuffers[i]->m_vertexbuffer;
			}
		}
	}

	m_indexBuffer    = static_cast<SampleApexRendererIndexBuffer*>(desc.indexBuffer);
	m_boneBuffer     = static_cast<SampleApexRendererBoneBuffer*>(desc.boneBuffer);
	m_instanceBuffer = static_cast<SampleApexRendererInstanceBuffer*>(desc.instanceBuffer);

	m_cullMode       = desc.cullMode;

	SampleRenderer::RendererMeshDesc meshdesc;
	meshdesc.primitives       = convertFromApex(desc.primitives);

	meshdesc.vertexBuffers    = internalsvbs;
	meshdesc.numVertexBuffers = numVertexBuffers;

#if USE_RENDER_SPRITE_BUFFER
	// the sprite buffer currently uses a vb
	if (m_spriteBuffer)
	{
		meshdesc.firstVertex      = desc.firstSprite;
		meshdesc.numVertices      = desc.numSprites;
	}
	else
#endif
	{
		meshdesc.firstVertex      = desc.firstVertex;
		meshdesc.numVertices      = desc.numVerts;
	}

	{
		if (m_indexBuffer != 0)
		{
			meshdesc.indexBuffer      = m_indexBuffer->m_indexbuffer;
			meshdesc.firstIndex       = desc.firstIndex;
			meshdesc.numIndices       = desc.numIndices;
		}
	}

	meshdesc.instanceBuffer   = m_instanceBuffer ? m_instanceBuffer->m_instanceBuffer : 0;
	meshdesc.firstInstance    = desc.firstInstance;
	meshdesc.numInstances     = desc.numInstances;
	m_mesh = m_renderer.createMesh(meshdesc);
	PX_ASSERT(m_mesh);
	if (m_mesh)
	{
		m_meshContext.mesh      = m_mesh;
		m_meshContext.transform = &m_meshTransform;
	}

#if USE_RENDER_SPRITE_BUFFER
	// the sprite buffer currently uses a vb
	if (m_spriteBuffer)
	{
		setVertexBufferRange(desc.firstSprite, desc.numSprites);
	}
	else
#endif
	{
		setVertexBufferRange(desc.firstVertex, desc.numVerts);
	}
	setIndexBufferRange(desc.firstIndex, desc.numIndices);
	setBoneBufferRange(desc.firstBone, desc.numBones);
	setInstanceBufferRange(desc.firstInstance, desc.numInstances);

	setMaterial(desc.material);

	if (internalsvbs)
	{
		delete [] internalsvbs;
	}
}

SampleApexRendererMesh::~SampleApexRendererMesh(void)
{
	if (m_mesh)
	{
		m_mesh->release();
	}
	if (m_vertexBuffers)
	{
		delete [] m_vertexBuffers;
	}
}

void SampleApexRendererMesh::setVertexBufferRange(uint32_t firstVertex, uint32_t numVerts)
{
	if (m_mesh)
	{
		m_mesh->setVertexBufferRange(firstVertex, numVerts);
	}
}

void SampleApexRendererMesh::setIndexBufferRange(uint32_t firstIndex, uint32_t numIndices)
{
	if (m_mesh)
	{
		m_mesh->setIndexBufferRange(firstIndex, numIndices);
	}
}

void SampleApexRendererMesh::setBoneBufferRange(uint32_t firstBone, uint32_t numBones)
{
	m_firstBone = firstBone;
	m_numBones  = numBones;
}

void SampleApexRendererMesh::setInstanceBufferRange(uint32_t firstInstance, uint32_t numInstances)
{
	if (m_mesh)
	{
		m_mesh->setInstanceBufferRange(firstInstance, numInstances);
	}
}

#if !USE_RENDERER_MATERIAL
void SampleApexRendererMesh::pickMaterial(SampleRenderer::RendererMeshContext& context, bool hasBones, SampleFramework::SampleMaterialAsset& material, BlendType hasBlending)
{
	// use this if it can't find another
	context.material         = material.getMaterial(0);
	context.materialInstance = material.getMaterialInstance(0);

	// try to find a better one
	for (size_t i = 0; i < material.getNumVertexShaders(); i++)
	{
		if ((material.getMaxBones(i) > 0) == hasBones &&
			(BLENDING_ANY == hasBlending || material.getMaterial(i)->getBlending() == (BLENDING_ENABLED == hasBlending)))
		{
			context.material         = material.getMaterial(i);
			context.materialInstance = material.getMaterialInstance(i);
			break;
		}
	}
	RENDERER_ASSERT(context.material, "Material has wrong vertex buffers!")
}
#endif

void SampleApexRendererMesh::setMaterial(void* material, BlendType hasBlending)
{
	if (material)
	{
#if USE_RENDERER_MATERIAL
		m_meshContext.materialInstance = static_cast<SampleRenderer::RendererMaterialInstance*>(material);
		m_meshContext.material = &m_meshContext.materialInstance->getMaterial();
#else
		SampleFramework::SampleMaterialAsset& materialAsset = *static_cast<SampleFramework::SampleMaterialAsset*>(material);

		pickMaterial(m_meshContext, m_boneBuffer != NULL, materialAsset, hasBlending);
#endif

#if USE_RENDER_SPRITE_BUFFER
		// get sprite shader variables
		if (m_spriteBuffer)
		{
			m_spriteShaderVariables[0] = m_meshContext.materialInstance->findVariable("windowWidth", SampleRenderer::RendererMaterial::VARIABLE_FLOAT);
			m_spriteShaderVariables[1] = m_meshContext.materialInstance->findVariable("particleSize", SampleRenderer::RendererMaterial::VARIABLE_FLOAT);
			m_spriteShaderVariables[2] = m_meshContext.materialInstance->findVariable("positionTexture", SampleRenderer::RendererMaterial::VARIABLE_SAMPLER2D);
			m_spriteShaderVariables[3] = m_meshContext.materialInstance->findVariable("colorTexture", SampleRenderer::RendererMaterial::VARIABLE_SAMPLER2D);
			m_spriteShaderVariables[4] = m_meshContext.materialInstance->findVariable("transformTexture", SampleRenderer::RendererMaterial::VARIABLE_SAMPLER2D);
			m_spriteShaderVariables[5] = m_meshContext.materialInstance->findVariable("vertexTextureWidth", SampleRenderer::RendererMaterial::VARIABLE_FLOAT);			
			m_spriteShaderVariables[6] = m_meshContext.materialInstance->findVariable("vertexTextureHeight", SampleRenderer::RendererMaterial::VARIABLE_FLOAT);			
		}
#endif
	}
	else
	{
		m_meshContext.material         = 0;
		m_meshContext.materialInstance = 0;
	}
}

void SampleApexRendererMesh::setScreenSpace(bool ss)
{
	m_meshContext.screenSpace = ss;
}

void SampleApexRendererMesh::render(const nvidia::apex::RenderContext& context, bool forceWireframe, SampleFramework::SampleMaterialAsset* overrideMaterial)
{
	if (m_mesh && m_meshContext.mesh && m_mesh->getNumVertices() > 0)
	{
#if USE_RENDER_SPRITE_BUFFER
		// set default sprite shader variables
		if (m_spriteBuffer)
		{
			// windowWidth
			if (m_spriteShaderVariables[0] && m_meshContext.materialInstance)
			{
				uint32_t width, height;
				m_renderer.getWindowSize(width, height);
				float fwidth = (float)width;
				m_meshContext.materialInstance->writeData(*m_spriteShaderVariables[0], &fwidth);
			}

			// position texture
			if (m_spriteShaderVariables[2] && m_meshContext.materialInstance)
			{
				SampleRenderer::RendererTexture* tex = m_spriteBuffer->getTexture(nvidia::apex::RenderSpriteTextureLayout::POSITION_FLOAT4);
				if(tex)	m_meshContext.materialInstance->writeData(*m_spriteShaderVariables[2], &tex);
			}
			// color texture
			if (m_spriteShaderVariables[3] && m_meshContext.materialInstance)
			{
				SampleRenderer::RendererTexture* tex = m_spriteBuffer->getTexture(nvidia::apex::RenderSpriteTextureLayout::COLOR_FLOAT4);
				if(tex)	
				{
					m_meshContext.materialInstance->writeData(*m_spriteShaderVariables[3], &tex);
				}
				else 
				{
					tex = m_spriteBuffer->getTexture(nvidia::apex::RenderSpriteTextureLayout::COLOR_BGRA8);
					if(tex)
					{
						m_meshContext.materialInstance->writeData(*m_spriteShaderVariables[3], &tex);
					}
					else
					{
						/* Couldn't find a texture for color */
						PX_ALWAYS_ASSERT();
					}
				}
			}
			// transform texture
			if (m_spriteShaderVariables[4] && m_meshContext.materialInstance)
			{
				SampleRenderer::RendererTexture* tex = m_spriteBuffer->getTexture(nvidia::apex::RenderSpriteTextureLayout::SCALE_ORIENT_SUBTEX_FLOAT4);
				if(tex)	m_meshContext.materialInstance->writeData(*m_spriteShaderVariables[4], &tex);
			}
			// vertexTextureWidth
			if (m_spriteShaderVariables[5] && m_meshContext.materialInstance)
			{
				SampleRenderer::RendererTexture* tex = m_spriteBuffer->getTexture(nvidia::apex::RenderSpriteTextureLayout::POSITION_FLOAT4);
				if(!tex) tex = m_spriteBuffer->getTexture(nvidia::apex::RenderSpriteTextureLayout::COLOR_FLOAT4);
				if(!tex) tex = m_spriteBuffer->getTexture(nvidia::apex::RenderSpriteTextureLayout::COLOR_BGRA8);
				if(!tex) tex = m_spriteBuffer->getTexture(nvidia::apex::RenderSpriteTextureLayout::SCALE_ORIENT_SUBTEX_FLOAT4);
				if(tex)	
				{
					const float width = tex->getWidth() * 1.0f;
					m_meshContext.materialInstance->writeData(*m_spriteShaderVariables[5], &width);
				}
			}
			// vertexTextureHeight
			if (m_spriteShaderVariables[6] && m_meshContext.materialInstance)
			{
				SampleRenderer::RendererTexture* tex = m_spriteBuffer->getTexture(nvidia::apex::RenderSpriteTextureLayout::POSITION_FLOAT4);
				if(!tex) tex = m_spriteBuffer->getTexture(nvidia::apex::RenderSpriteTextureLayout::COLOR_FLOAT4);
				if(!tex) tex = m_spriteBuffer->getTexture(nvidia::apex::RenderSpriteTextureLayout::COLOR_BGRA8);
				if(!tex) tex = m_spriteBuffer->getTexture(nvidia::apex::RenderSpriteTextureLayout::SCALE_ORIENT_SUBTEX_FLOAT4);
				if(tex)	
				{
					const float height = tex->getHeight() * 1.0f;
					m_meshContext.materialInstance->writeData(*m_spriteShaderVariables[6], &height);
				}
			}
		}
#endif /* #if USE_RENDER_SPRITE_BUFFER */

		m_meshTransform = context.local2world;
		if (m_boneBuffer)
		{
			// Pass bone texture information
			m_meshContext.boneTexture = m_boneBuffer->m_boneTexture;
			m_meshContext.boneTextureHeight = m_boneBuffer->m_maxBones;

			m_meshContext.boneMatrices = m_boneBuffer->m_bones + m_firstBone;
			m_meshContext.numBones     = m_numBones;
		}
		switch (m_cullMode)
		{
		case nvidia::apex::RenderCullMode::CLOCKWISE:
			m_meshContext.cullMode = SampleRenderer::RendererMeshContext::CLOCKWISE;
			break;
		case nvidia::apex::RenderCullMode::COUNTER_CLOCKWISE:
			m_meshContext.cullMode = SampleRenderer::RendererMeshContext::COUNTER_CLOCKWISE;
			break;
		case nvidia::apex::RenderCullMode::NONE:
			m_meshContext.cullMode = SampleRenderer::RendererMeshContext::NONE;
			break;
		default:
			PX_ASSERT(0 && "Invalid Cull Mode");
		}
		m_meshContext.screenSpace = context.isScreenSpace;

		SampleRenderer::RendererMeshContext tmpContext = m_meshContext;
		if (forceWireframe)
		{
			tmpContext.fillMode = SampleRenderer::RendererMeshContext::LINE;
		}
#if !USE_RENDERER_MATERIAL
		if (overrideMaterial != NULL)
		{
			pickMaterial(tmpContext, m_boneBuffer != NULL, *overrideMaterial);
		}
#endif
		m_renderer.queueMeshForRender(tmpContext);
	}
}
