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
#include <RendererMaterialDesc.h>

using namespace SampleRenderer;

RendererMaterialDesc::RendererMaterialDesc(void)
{
	type               = RendererMaterial::TYPE_UNLIT;
	alphaTestFunc      = RendererMaterial::ALPHA_TEST_ALWAYS;
	alphaTestRef       = 0;
	blending           = false;
	instanced		   = false;
	srcBlendFunc       = RendererMaterial::BLEND_ZERO;
	dstBlendFunc       = RendererMaterial::BLEND_ZERO;
	geometryShaderPath = 0;
	hullShaderPath     = 0;
	domainShaderPath   = 0;
	vertexShaderPath   = 0;
	fragmentShaderPath = 0;
}

bool RendererMaterialDesc::isValid(void) const
{
	bool ok = true;
	if(type >= RendererMaterial::NUM_TYPES) ok = false;
	if(alphaTestRef < 0 || alphaTestRef > 1) ok = false;
	// Note: Lighting and blending may not be properly supported, but that 
	//    shouldn't crash the system, so this check is disabled for now
	// if(blending && type != RendererMaterial::TYPE_UNLIT) ok = false;
	if(geometryShaderPath || domainShaderPath || fragmentShaderPath)
	{
		// Note: These should be completely optional! Disabling for now.
		//RENDERER_ASSERT(0, "Not implemented!");
		//ok = false;
	}
	if(!vertexShaderPath)   ok = false;
	if(!fragmentShaderPath) ok = false;
	return ok;
}
