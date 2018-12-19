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
#include <RendererTargetDesc.h>
#include <RendererTexture2D.h>

using namespace SampleRenderer;

RendererTargetDesc::RendererTargetDesc(void)
{
	textures            = 0;
	numTextures         = 0;
	depthStencilSurface = 0;
}

bool RendererTargetDesc::isValid(void) const
{
	bool ok = true;
	PxU32 width  = 0;
	PxU32 height = 0;
	if(numTextures != 1) ok = false; // for now we only support single render targets at the moment.
	if(textures)
	{
		if(textures[0])
		{
			width  = textures[0]->getWidth();
			height = textures[0]->getHeight();
		}
		for(PxU32 i=0; i<numTextures; i++)
		{
			if(!textures[i]) ok = false;
			else
			{
				if(width  != textures[i]->getWidth())  ok = false;
				if(height != textures[i]->getHeight()) ok = false;
				const RendererTexture2D::Format format = textures[i]->getFormat();
				if(     format == RendererTexture2D::FORMAT_DXT1) ok = false;
				else if(format == RendererTexture2D::FORMAT_DXT3) ok = false;
				else if(format == RendererTexture2D::FORMAT_DXT5) ok = false;
			}
		}
	}
	else
	{
		ok = false;
	}
	if(depthStencilSurface)
	{
		if(width  != depthStencilSurface->getWidth())  ok = false;
		if(height != depthStencilSurface->getHeight()) ok = false;
		if(!RendererTexture2D::isDepthStencilFormat(depthStencilSurface->getFormat())) ok = false;
	}
	else
	{
		ok = false;
	}
	return ok;
}
