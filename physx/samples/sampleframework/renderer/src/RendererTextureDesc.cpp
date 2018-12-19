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
#include <RendererTextureDesc.h>

using namespace SampleRenderer;

RendererTextureDesc::RendererTextureDesc(void)
{
	format       = RendererTexture::NUM_FORMATS;
	filter       = RendererTexture::FILTER_LINEAR;
	addressingU  = RendererTexture::ADDRESSING_WRAP;
	addressingV  = RendererTexture::ADDRESSING_WRAP;
	addressingW  = RendererTexture::ADDRESSING_WRAP;
	width        = 0;
	height       = 0;
	depth        = 1;
	numLevels    = 0;
	renderTarget = false;
	data         = NULL;
}


bool RendererTextureDesc::isValid(void) const
{
	bool ok = true;
	if(format      >= RendererTexture2D::NUM_FORMATS)    ok = false;
	if(filter      >= RendererTexture2D::NUM_FILTERS)    ok = false;
	if(addressingU >= RendererTexture2D::NUM_ADDRESSING) ok = false;
	if(addressingV >= RendererTexture2D::NUM_ADDRESSING) ok = false;
	if(width <= 0 || height <= 0 || depth <= 0) ok = false; // TODO: check for power of two.
	if(numLevels <= 0) ok = false;
	if(renderTarget)
	{
		if(depth > 1) ok = false;
		if(format == RendererTexture2D::FORMAT_DXT1) ok = false;
		if(format == RendererTexture2D::FORMAT_DXT3) ok = false;
		if(format == RendererTexture2D::FORMAT_DXT5) ok = false;
	}
	return ok;
}
