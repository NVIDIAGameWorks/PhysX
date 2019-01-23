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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
#include <RendererTexture.h>
#include <RendererTextureDesc.h>
#include "foundation/PxMath.h"

using namespace SampleRenderer;

static PxU32 computeCompressedDimension(PxU32 dimension)
{
	if(dimension < 4)
	{
		dimension = 4;
	}
	else
	{
		PxU32 mod = dimension % 4;
		if(mod) dimension += 4 - mod;
	}
	return dimension;
}

PxU32 RendererTexture::computeImageByteSize(PxU32 width, PxU32 height, PxU32 depth, RendererTexture::Format format)
{
	PxU32 size = 0;
	PxU32 numPixels = width * height * depth;
	switch(format)
	{
	case RendererTexture::FORMAT_B8G8R8A8:
	case RendererTexture::FORMAT_R8G8B8A8:
		size = numPixels * sizeof(PxU8) * 4;
		break;
	case RendererTexture::FORMAT_A8:
		size = numPixels * sizeof(PxU8) * 1;
		break;
	case RendererTexture::FORMAT_R32F:
		size = numPixels * sizeof(float);
		break;
	case RendererTexture::FORMAT_DXT1:
		width  = computeCompressedDimension(width);
		height = computeCompressedDimension(height);
		size   = computeImageByteSize(width, height, depth, RendererTexture::FORMAT_B8G8R8A8) / 8;
		break;
	case RendererTexture::FORMAT_DXT3:
	case RendererTexture::FORMAT_DXT5:
		width  = computeCompressedDimension(width);
		height = computeCompressedDimension(height);
		size   = computeImageByteSize(width, height, depth, RendererTexture::FORMAT_B8G8R8A8) / 4;
		break;
	case RendererTexture::FORMAT_PVR_2BPP:
		{
			PxU32 bytesPerBlock = 8 * 4 * 2; //8 by 4 pixels times bytes per pixel
			width = PxMax(PxI32(width / 8), 2);
			height = PxMax(PxI32(height / 4), 2);
			size = width*height*bytesPerBlock / 8;
		}
		break;
	case RendererTexture::FORMAT_PVR_4BPP:
		{
			PxU32 bytesPerBlock = 4 * 4 * 4; //4 by 4 pixels times bytes per pixel
			width = PxMax(PxI32(width / 4), 2);
			height = PxMax(PxI32(height / 4), 2);
			size = width*height*bytesPerBlock / 8;
		}
		break;
	default: break;
	}
	RENDERER_ASSERT(size, "Unable to compute Image Size.");
	return size;
}

PxU32 RendererTexture::getLevelDimension(PxU32 dimension, PxU32 level)
{
	dimension >>=level;
	if(!dimension) dimension=1;
	return dimension;
}

bool RendererTexture::isCompressedFormat(Format format)
{
	return (format >= FORMAT_DXT1 && format <= FORMAT_DXT5) || (format == FORMAT_PVR_2BPP) || (format == FORMAT_PVR_4BPP);
}

bool RendererTexture::isDepthStencilFormat(Format format)
{
	return (format >= FORMAT_D16 && format <= FORMAT_D24S8);
}

PxU32 RendererTexture::getFormatNumBlocks(PxU32 dimension, Format format)
{
	PxU32 blockDimension = 0;
	if(isCompressedFormat(format))
	{
		blockDimension = dimension / 4;
		if(dimension % 4) blockDimension++;
	}
	else
	{
		blockDimension = dimension;
	}
	return blockDimension;
}

PxU32 RendererTexture::getFormatBlockSize(Format format)
{
	return computeImageByteSize(1, 1, 1, format);
}

RendererTexture::RendererTexture(const RendererTextureDesc &desc)
{
	m_format      = desc.format;
	m_filter      = desc.filter;
	m_addressingU = desc.addressingU;
	m_addressingV = desc.addressingV;
	m_width       = desc.width;
	m_height      = desc.height;
	m_depth       = desc.depth;
	m_numLevels   = desc.numLevels;
}

RendererTexture::~RendererTexture(void)
{

}

PxU32 RendererTexture::getWidthInBlocks(void) const
{
	return getFormatNumBlocks(getWidth(), getFormat());
}

PxU32 RendererTexture::getHeightInBlocks(void) const
{
	return getFormatNumBlocks(getHeight(), getFormat());
}

PxU32 RendererTexture::getBlockSize(void) const
{
	return getFormatBlockSize(getFormat());
}
