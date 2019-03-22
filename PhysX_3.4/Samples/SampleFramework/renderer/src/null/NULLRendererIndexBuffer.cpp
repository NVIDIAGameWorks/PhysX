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

#include <RendererConfig.h>


#include "NULLRendererIndexBuffer.h"
#include <RendererIndexBufferDesc.h>

using namespace SampleRenderer;

NullRendererIndexBuffer::NullRendererIndexBuffer(const RendererIndexBufferDesc &desc) :
	RendererIndexBuffer(desc), m_Buffer(0)
{
	PxU32 indexSize  = getFormatByteSize(getFormat());
	
	RENDERER_ASSERT(desc.maxIndices > 0 && desc.maxIndices > 0, "Cannot create zero size Index Buffer.");
	if(indexSize && desc.maxIndices > 0)
	{
		m_Buffer = new PxU8[indexSize*desc.maxIndices];
		m_maxIndices = desc.maxIndices;
	}

}

NullRendererIndexBuffer::~NullRendererIndexBuffer(void)
{
	if(m_Buffer)
	{
		delete [] m_Buffer;
		m_Buffer = NULL;
	}
}

void *NullRendererIndexBuffer::lock(void)
{	
	return m_Buffer;
}

void NullRendererIndexBuffer::unlock(void)
{
}

void NullRendererIndexBuffer::bind(void) const
{
}

void NullRendererIndexBuffer::unbind(void) const
{
}
