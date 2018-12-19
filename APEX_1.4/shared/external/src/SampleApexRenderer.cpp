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


#include <SampleApexRenderer.h>

#include <SampleApexRenderResources.h>

#if !USE_RENDERER_MATERIAL
#include <SampleMaterialAsset.h>
#endif

#include <RenderContext.h>
#include <UserRenderIndexBufferDesc.h>
#include <UserRenderInstanceBuffer.h>
#include <UserRenderResourceDesc.h>
#include <UserRenderBoneBufferDesc.h>
#include <UserRenderSpriteBufferDesc.h>
#include <UserRenderSurfaceBufferDesc.h>
#include <UserRenderVertexBufferDesc.h>
#include <UserRenderSpriteBufferDesc.h>
#include <algorithm>	// for std::min

/**********************************
* SampleApexRenderResourceManager *
**********************************/

SampleApexRenderResourceManager::SampleApexRenderResourceManager(SampleRenderer::Renderer& renderer) :
	m_renderer(renderer), m_particleRenderingMechanism(VERTEX_BUFFER_OBJECT)
{
	m_numVertexBuffers   = 0;
	m_numIndexBuffers    = 0;
	m_numSurfaceBuffers  = 0;
	m_numBoneBuffers     = 0;
	m_numInstanceBuffers = 0;
	m_numResources       = 0;
}

SampleApexRenderResourceManager::~SampleApexRenderResourceManager(void)
{
	RENDERER_ASSERT(m_numVertexBuffers   == 0, "Not all Vertex Buffers were released prior to Render Resource Manager destruction!");
	RENDERER_ASSERT(m_numIndexBuffers    == 0, "Not all Index Buffers were released prior to Render Resource Manager destruction!");
	RENDERER_ASSERT(m_numSurfaceBuffers  == 0, "Not all Surface Buffers were released prior to Render Resource Manager destruction!");
	RENDERER_ASSERT(m_numBoneBuffers     == 0, "Not all Bone Buffers were released prior to Render Resource Manager destruction!");
	RENDERER_ASSERT(m_numInstanceBuffers == 0, "Not all Instance Buffers were released prior to Render Resource Manager destruction!");
	RENDERER_ASSERT(m_numResources       == 0, "Not all Resources were released prior to Render Resource Manager destruction!");
}

nvidia::apex::UserRenderVertexBuffer* SampleApexRenderResourceManager::createVertexBuffer(const nvidia::apex::UserRenderVertexBufferDesc& desc)
{
	SampleApexRendererVertexBuffer* vb = 0;

	unsigned int numSemantics = 0;
	for (unsigned int i = 0; i < nvidia::apex::RenderVertexSemantic::NUM_SEMANTICS; i++)
	{
		numSemantics += desc.buffersRequest[i] != nvidia::apex::RenderDataFormat::UNSPECIFIED ? 1 : 0;
	}
	PX_ASSERT(desc.isValid());
	if (desc.isValid() && numSemantics > 0)
	{
		vb = new SampleApexRendererVertexBuffer(m_renderer, desc);
		m_numVertexBuffers++;
	}
	return vb;
}

void SampleApexRenderResourceManager::releaseVertexBuffer(nvidia::apex::UserRenderVertexBuffer& buffer)
{
	PX_ASSERT(m_numVertexBuffers > 0);
	m_numVertexBuffers--;
	delete &buffer;
}

nvidia::apex::UserRenderIndexBuffer* SampleApexRenderResourceManager::createIndexBuffer(const nvidia::apex::UserRenderIndexBufferDesc& desc)
{
	SampleApexRendererIndexBuffer* ib = 0;
	PX_ASSERT(desc.isValid());
	if (desc.isValid())
	{
		ib = new SampleApexRendererIndexBuffer(m_renderer, desc);
		m_numIndexBuffers++;
	}
	return ib;
}

void SampleApexRenderResourceManager::releaseIndexBuffer(nvidia::apex::UserRenderIndexBuffer& buffer)
{
	PX_ASSERT(m_numIndexBuffers > 0);
	m_numIndexBuffers--;
	delete &buffer;
}

nvidia::apex::UserRenderSurfaceBuffer* SampleApexRenderResourceManager::createSurfaceBuffer(const nvidia::apex::UserRenderSurfaceBufferDesc& desc)
{
	SampleApexRendererSurfaceBuffer* sb = 0;
	PX_ASSERT(desc.isValid());
	if (desc.isValid())
	{
		sb = new SampleApexRendererSurfaceBuffer(m_renderer, desc);
		m_numSurfaceBuffers++;
	}
	return sb;
}

void SampleApexRenderResourceManager::releaseSurfaceBuffer(nvidia::apex::UserRenderSurfaceBuffer& buffer)
{
	PX_ASSERT(m_numSurfaceBuffers > 0);
	m_numSurfaceBuffers--;
	delete &buffer;
}

nvidia::apex::UserRenderBoneBuffer* SampleApexRenderResourceManager::createBoneBuffer(const nvidia::apex::UserRenderBoneBufferDesc& desc)
{
	SampleApexRendererBoneBuffer* bb = 0;
	PX_ASSERT(desc.isValid());
	if (desc.isValid())
	{
		bb = new SampleApexRendererBoneBuffer(m_renderer, desc);
		m_numBoneBuffers++;
	}
	return bb;
}

void SampleApexRenderResourceManager::releaseBoneBuffer(nvidia::apex::UserRenderBoneBuffer& buffer)
{
	PX_ASSERT(m_numBoneBuffers > 0);
	m_numBoneBuffers--;
	delete &buffer;
}

nvidia::apex::UserRenderInstanceBuffer* SampleApexRenderResourceManager::createInstanceBuffer(const nvidia::apex::UserRenderInstanceBufferDesc& desc)
{
	SampleApexRendererInstanceBuffer* ib = 0;
	PX_ASSERT(desc.isValid());
	if (desc.isValid())
	{
		ib = new SampleApexRendererInstanceBuffer(m_renderer, desc);
		m_numInstanceBuffers++;
	}
	return ib;
}

void SampleApexRenderResourceManager::releaseInstanceBuffer(nvidia::apex::UserRenderInstanceBuffer& buffer)
{
	PX_ASSERT(m_numInstanceBuffers > 0);
	m_numInstanceBuffers--;
	delete &buffer;
}

nvidia::apex::UserRenderSpriteBuffer* SampleApexRenderResourceManager::createSpriteBuffer(const nvidia::apex::UserRenderSpriteBufferDesc& desc)
{
#if USE_RENDER_SPRITE_BUFFER
	SampleApexRendererSpriteBuffer* sb = 0;
	PX_ASSERT(desc.isValid());
	if (desc.isValid())
	{
		// convert SB to VB
		sb = new SampleApexRendererSpriteBuffer(m_renderer, desc);
		m_numVertexBuffers++;
	}
	return sb;
#else
	return NULL;
#endif
}

void  SampleApexRenderResourceManager::releaseSpriteBuffer(nvidia::apex::UserRenderSpriteBuffer& buffer)
{
#if USE_RENDER_SPRITE_BUFFER
	// LRR: for now, just use a VB
	PX_ASSERT(m_numVertexBuffers > 0);
	m_numVertexBuffers--;
	delete &buffer;
#endif
}

nvidia::apex::UserRenderResource* SampleApexRenderResourceManager::createResource(const nvidia::apex::UserRenderResourceDesc& desc)
{
	SampleApexRendererMesh* mesh = 0;
	PX_ASSERT(desc.isValid());
	if (desc.isValid())
	{
		mesh = new SampleApexRendererMesh(m_renderer, desc);
		m_numResources++;
	}
	return mesh;
}

void SampleApexRenderResourceManager::releaseResource(nvidia::apex::UserRenderResource& resource)
{
	PX_ASSERT(m_numResources > 0);
	m_numResources--;
	delete &resource;
}

uint32_t SampleApexRenderResourceManager::getMaxBonesForMaterial(void* material)
{
	if (material != NULL)
	{
		unsigned int maxBones = 0xffffffff;
#if USE_RENDERER_MATERIAL
		// don't yet know if this material even supports bones, but this would be the max...
		maxBones = RENDERER_MAX_BONES;
#else
		SampleFramework::SampleMaterialAsset* materialAsset = static_cast<SampleFramework::SampleMaterialAsset*>(material);
		for (size_t i = 0; i < materialAsset->getNumVertexShaders(); i++)
		{
			unsigned int maxBonesMat = materialAsset->getMaxBones(i);
			if (maxBonesMat > 0)
			{
				maxBones = std::min(maxBones, maxBonesMat);
			}
		}
#endif

		return maxBones != 0xffffffff ? maxBones : 0;
	}
	else
	{
		return 0;
	}
}

bool SampleApexRenderResourceManager::getInstanceLayoutData(uint32_t particleCount, 
																	uint32_t particleSemanticsBitmap, 
																	nvidia::apex::UserRenderInstanceBufferDesc* bufferDesc)
{
	using namespace nvidia::apex;
	RenderDataFormat::Enum positionFormat = RenderInstanceLayoutElement::getSemanticFormat(RenderInstanceLayoutElement::POSITION_FLOAT3);
	RenderDataFormat::Enum rotationFormat = RenderInstanceLayoutElement::getSemanticFormat(RenderInstanceLayoutElement::ROTATION_SCALE_FLOAT3x3);
	RenderDataFormat::Enum velocityFormat = RenderInstanceLayoutElement::getSemanticFormat(RenderInstanceLayoutElement::VELOCITY_LIFE_FLOAT4);
	const uint32_t positionElementSize = RenderDataFormat::getFormatDataSize(positionFormat);
	const uint32_t rotationElementSize = RenderDataFormat::getFormatDataSize(rotationFormat);
	const uint32_t velocityElementSize = RenderDataFormat::getFormatDataSize(velocityFormat);
	bufferDesc->semanticOffsets[RenderInstanceLayoutElement::POSITION_FLOAT3]			= 0;
	bufferDesc->semanticOffsets[RenderInstanceLayoutElement::ROTATION_SCALE_FLOAT3x3] = positionElementSize;
	bufferDesc->semanticOffsets[RenderInstanceLayoutElement::VELOCITY_LIFE_FLOAT4] = positionElementSize + rotationElementSize;
	uint32_t strideInBytes = positionElementSize + rotationElementSize + velocityElementSize;
	bufferDesc->stride = strideInBytes;
	bufferDesc->maxInstances = particleCount;
	return true;
}

bool SampleApexRenderResourceManager::getSpriteLayoutData(uint32_t spriteCount, 
																	uint32_t spriteSemanticsBitmap, 
																	nvidia::apex::UserRenderSpriteBufferDesc* bufferDesc)
{
	using namespace nvidia::apex;
	if(m_particleRenderingMechanism == VERTEX_TEXTURE_FETCH)
	{
		const uint32_t TextureCount = 3;

		uint32_t width = (uint32_t)physx::PxCeil(physx::PxSqrt((float)spriteCount));
		//make sizeX >= 32 [32 is WARP_SIZE in CUDA]
		width = physx::PxMax(width, 32U);
		//compute the next highest power of 2
		width--;
		width |= width >> 1;
		width |= width >> 2;
		width |= width >> 4;
		width |= width >> 8;
		width |= width >> 16;
		width++;

		uint32_t height = (spriteCount + width - 1) / width;
		bufferDesc->textureCount = TextureCount;
		bufferDesc->textureDescs[0].layout = RenderSpriteTextureLayout::POSITION_FLOAT4;
		bufferDesc->textureDescs[1].layout = RenderSpriteTextureLayout::SCALE_ORIENT_SUBTEX_FLOAT4;
		bufferDesc->textureDescs[2].layout = RenderSpriteTextureLayout::COLOR_FLOAT4;

		for (uint32_t i = 0; i < TextureCount; ++i)
		{
			bufferDesc->textureDescs[i].width = width;
			bufferDesc->textureDescs[i].height = height;

			const uint32_t ElemSize = RenderDataFormat::getFormatDataSize( RenderSpriteTextureLayout::getLayoutFormat(bufferDesc->textureDescs[i].layout) );
			bufferDesc->textureDescs[i].pitchBytes = ElemSize * bufferDesc->textureDescs[i].width;

			bufferDesc->textureDescs[i].arrayIndex = 0;
			bufferDesc->textureDescs[i].mipLevel = 0;
		}

		bufferDesc->maxSprites = spriteCount;
		return true;
	}
	else if(m_particleRenderingMechanism == VERTEX_BUFFER_OBJECT)
	{
		RenderDataFormat::Enum positionFormat = RenderSpriteLayoutElement::getSemanticFormat(RenderSpriteLayoutElement::POSITION_FLOAT3);
		RenderDataFormat::Enum colorFormat = RenderSpriteLayoutElement::getSemanticFormat(RenderSpriteLayoutElement::COLOR_BGRA8);
		const uint32_t positionElementSize = RenderDataFormat::getFormatDataSize(positionFormat);
		const uint32_t colorElementSize = RenderDataFormat::getFormatDataSize(colorFormat);
		bufferDesc->semanticOffsets[RenderSpriteLayoutElement::POSITION_FLOAT3] = 0;
		bufferDesc->semanticOffsets[RenderSpriteLayoutElement::COLOR_BGRA8] = positionElementSize;
		uint32_t strideInBytes = positionElementSize + colorElementSize;
		bufferDesc->stride = strideInBytes;
		bufferDesc->maxSprites = spriteCount;
		bufferDesc->textureCount = 0;
		return true;
	}
	else
	{
		PX_ASSERT(0 && "Select a method to update particle render buffer.");
	}
	return true;
}

void SampleApexRenderResourceManager::setMaterial(nvidia::apex::UserRenderResource& resource, void* material)
{
	static_cast<SampleApexRendererMesh&>(resource).setMaterial(material);
}


/*********************
* SampleApexRenderer *
*********************/

void SampleApexRenderer::renderResource(const nvidia::apex::RenderContext& context)
{
	if (context.renderResource)
	{
		static_cast<SampleApexRendererMesh*>(context.renderResource)->render(context, mForceWireframe, mOverrideMaterial);
	}
}
