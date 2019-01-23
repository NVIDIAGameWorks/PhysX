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
#include <RendererIndexBuffer.h>
#include <RendererIndexBufferDesc.h>

using namespace SampleRenderer;

PxU32 RendererIndexBuffer::getFormatByteSize(Format format)
{
	PxU32 size = 0;
	switch(format)
	{
	case FORMAT_UINT16: size = sizeof(PxU16); break;
	case FORMAT_UINT32: size = sizeof(PxU32); break;
	default: break;
	}
	RENDERER_ASSERT(size, "Unable to determine size of format.");
	return size;
}

RendererIndexBuffer::RendererIndexBuffer(const RendererIndexBufferDesc &desc) :
RendererInteropableBuffer(desc.registerInCUDA, desc.interopContext),
	m_hint(desc.hint),
	m_format(desc.format)
{
	m_maxIndices = 0;
}

RendererIndexBuffer::~RendererIndexBuffer(void)
{

}

RendererIndexBuffer::Hint RendererIndexBuffer::getHint(void) const
{
	return m_hint;
}

RendererIndexBuffer::Format RendererIndexBuffer::getFormat(void) const
{
	return m_format;
}

PxU32 RendererIndexBuffer::getMaxIndices(void) const
{
	return m_maxIndices;
}
