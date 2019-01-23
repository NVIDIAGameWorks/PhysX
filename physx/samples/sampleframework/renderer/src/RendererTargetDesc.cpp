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
