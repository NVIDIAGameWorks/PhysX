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
#include <RendererInstanceBufferDesc.h>
#include "foundation/PxAssert.h"

using namespace SampleRenderer;

RendererInstanceBufferDesc::RendererInstanceBufferDesc(void)
{
	hint = RendererInstanceBuffer::HINT_STATIC;
	for(PxU32 i=0; i<RendererInstanceBuffer::NUM_SEMANTICS; i++)
	{
		semanticFormats[i] = RendererInstanceBuffer::NUM_FORMATS;
	}
	maxInstances = 0;
	canReadBack = false;
	registerInCUDA = false;
	interopContext = 0;
}

bool RendererInstanceBufferDesc::isValid(void) const
{
	bool ok = true;
	if(!maxInstances) ok = false;

	bool bValidTurbulence = ok;
	if(semanticFormats[RendererInstanceBuffer::SEMANTIC_POSITION] != RendererInstanceBuffer::FORMAT_FLOAT3) ok = false;
	if(semanticFormats[RendererInstanceBuffer::SEMANTIC_NORMALX]  != RendererInstanceBuffer::FORMAT_FLOAT3) ok = false;
	if(semanticFormats[RendererInstanceBuffer::SEMANTIC_NORMALY]  != RendererInstanceBuffer::FORMAT_FLOAT3) ok = false;
	if(semanticFormats[RendererInstanceBuffer::SEMANTIC_NORMALZ]  != RendererInstanceBuffer::FORMAT_FLOAT3) ok = false;

	if(semanticFormats[RendererInstanceBuffer::SEMANTIC_POSITION] != RendererInstanceBuffer::FORMAT_FLOAT3) bValidTurbulence = false;
	if(semanticFormats[RendererInstanceBuffer::SEMANTIC_VELOCITY_LIFE] != RendererInstanceBuffer::FORMAT_FLOAT4) bValidTurbulence = false;

	if((semanticFormats[RendererInstanceBuffer::SEMANTIC_UV_OFFSET]    != RendererInstanceBuffer::FORMAT_FLOAT2) &&
	   (semanticFormats[RendererInstanceBuffer::SEMANTIC_UV_OFFSET]    != RendererInstanceBuffer::NUM_FORMATS)) ok = false;
	if((semanticFormats[RendererInstanceBuffer::SEMANTIC_LOCAL_OFFSET] != RendererInstanceBuffer::FORMAT_FLOAT3) &&
	   (semanticFormats[RendererInstanceBuffer::SEMANTIC_LOCAL_OFFSET] != RendererInstanceBuffer::NUM_FORMATS)) ok = false;

	if(registerInCUDA && !interopContext)
		ok = false;

	PX_ASSERT(bValidTurbulence || ok);
	return bValidTurbulence || ok;
}
