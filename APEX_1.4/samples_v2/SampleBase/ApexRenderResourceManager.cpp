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


#include "ApexRenderResourceManager.h"
#include "ApexRenderResources.h"


bool ApexRenderResourceManager::getInstanceLayoutData(uint32_t particleCount,
	uint32_t particleSemanticsBitmap,
	nvidia::apex::UserRenderInstanceBufferDesc* bufferDesc)
{
	PX_UNUSED(particleSemanticsBitmap);
	using namespace nvidia::apex;
	RenderDataFormat::Enum positionFormat = RenderInstanceLayoutElement::getSemanticFormat(RenderInstanceLayoutElement::POSITION_FLOAT3);
	RenderDataFormat::Enum rotationFormat = RenderInstanceLayoutElement::getSemanticFormat(RenderInstanceLayoutElement::ROTATION_SCALE_FLOAT3x3);
	const uint32_t positionElementSize = RenderDataFormat::getFormatDataSize(positionFormat);
	const uint32_t rotationElementSize = RenderDataFormat::getFormatDataSize(rotationFormat);
	bufferDesc->semanticOffsets[RenderInstanceLayoutElement::POSITION_FLOAT3] = 0;
	bufferDesc->semanticOffsets[RenderInstanceLayoutElement::ROTATION_SCALE_FLOAT3x3] = positionElementSize;
	uint32_t strideInBytes = positionElementSize + rotationElementSize;;
	bufferDesc->stride = strideInBytes;
	bufferDesc->maxInstances = particleCount;
	return true;
}

bool ApexRenderResourceManager::getSpriteLayoutData(uint32_t spriteCount,
	uint32_t spriteSemanticsBitmap,
	nvidia::apex::UserRenderSpriteBufferDesc* bufferDesc)
{
	PX_UNUSED(spriteSemanticsBitmap);
	RenderDataFormat::Enum positionFormat = RenderSpriteLayoutElement::getSemanticFormat(RenderSpriteLayoutElement::POSITION_FLOAT3);
	RenderDataFormat::Enum colorFormat = RenderSpriteLayoutElement::getSemanticFormat(RenderSpriteLayoutElement::COLOR_BGRA8);
	RenderDataFormat::Enum scaleFormat = RenderSpriteLayoutElement::getSemanticFormat(RenderSpriteLayoutElement::SCALE_FLOAT2);
	const uint32_t positionElementSize = RenderDataFormat::getFormatDataSize(positionFormat);
	const uint32_t colorElementSize = RenderDataFormat::getFormatDataSize(colorFormat);
	const uint32_t scaleElementSize = RenderDataFormat::getFormatDataSize(scaleFormat);
	bufferDesc->semanticOffsets[RenderSpriteLayoutElement::POSITION_FLOAT3] = 0;
	bufferDesc->semanticOffsets[RenderSpriteLayoutElement::COLOR_BGRA8] = positionElementSize;
	bufferDesc->semanticOffsets[RenderSpriteLayoutElement::SCALE_FLOAT2] = positionElementSize + colorElementSize;
	uint32_t strideInBytes = positionElementSize + colorElementSize + scaleElementSize;
	bufferDesc->stride = strideInBytes;
	bufferDesc->maxSprites = spriteCount;
	bufferDesc->textureCount = 0;
	return true;
}