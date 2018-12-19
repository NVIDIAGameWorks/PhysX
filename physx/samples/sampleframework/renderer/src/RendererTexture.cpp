// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
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
