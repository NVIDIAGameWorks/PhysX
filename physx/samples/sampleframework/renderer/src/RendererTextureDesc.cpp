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
