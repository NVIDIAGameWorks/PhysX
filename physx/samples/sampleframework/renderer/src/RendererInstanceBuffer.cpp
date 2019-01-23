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
#include <RendererInstanceBuffer.h>
#include <RendererInstanceBufferDesc.h>

using namespace SampleRenderer;

PxU32 RendererInstanceBuffer::getFormatByteSize(Format format)
{
	PxU32 size = 0;
	switch(format)
	{
	case FORMAT_FLOAT1: size = sizeof(float)*1; break;
	case FORMAT_FLOAT2: size = sizeof(float)*2; break;
	case FORMAT_FLOAT3: size = sizeof(float)*3; break;
	case FORMAT_FLOAT4: size = sizeof(float)*4; break;
	default: break;
	}
	RENDERER_ASSERT(size, "Unable to determine size of Format.");
	return size;
}

RendererInstanceBuffer::RendererInstanceBuffer(const RendererInstanceBufferDesc &desc) :
RendererInteropableBuffer(desc.registerInCUDA, desc.interopContext),
	m_hint(desc.hint)
{
	m_maxInstances     = 0;
	m_stride           = 0;
	m_lockedBuffer     = 0;
	m_numSemanticLocks = 0;
	for(PxU32 i=0; i<NUM_SEMANTICS; i++)
	{
		Format format = desc.semanticFormats[i];
		if(format < NUM_FORMATS)
		{
			SemanticDesc &sm  = m_semanticDescs[i];
			sm.format         = format;
			sm.offset         = m_stride;
			m_stride         += getFormatByteSize(format);
		}
	}
}

RendererInstanceBuffer::~RendererInstanceBuffer(void)
{
	RENDERER_ASSERT(m_lockedBuffer==0 && m_numSemanticLocks==0, "InstanceBuffer had outstanding locks during destruction!");
}

RendererInstanceBuffer::Hint RendererInstanceBuffer::getHint(void) const
{
	return m_hint;
}

RendererInstanceBuffer::Format RendererInstanceBuffer::getFormatForSemantic(Semantic semantic) const
{
	return m_semanticDescs[semantic].format;
}

PxU32 SampleRenderer::RendererInstanceBuffer::getMaxInstances(void) const
{
	return m_maxInstances;
}

PxU32  RendererInstanceBuffer::getOffsetForSemantic(Semantic semantic) const
{
		return m_semanticDescs[semantic].offset;
}

void *RendererInstanceBuffer::lockSemantic(Semantic semantic, PxU32 &stride)
{
	void *semanticBuffer = 0;
	RENDERER_ASSERT(semantic < NUM_SEMANTICS, "Invalid InstanceBuffer Semantic!");
	if(semantic < NUM_SEMANTICS)
	{
		SemanticDesc &sm = m_semanticDescs[semantic];
		RENDERER_ASSERT(!sm.locked,              "InstanceBuffer Semantic already locked.");
		RENDERER_ASSERT(sm.format < NUM_FORMATS, "InstanceBuffer Semantic format not valid.");
		if(!sm.locked && sm.format < NUM_FORMATS)
		{
			if(!m_lockedBuffer && !m_numSemanticLocks)
			{
				m_lockedBuffer = lock();
			}
			RENDERER_ASSERT(m_lockedBuffer, "Unable to lock InstanceBuffer!");
			if(m_lockedBuffer)
			{
				m_numSemanticLocks++;
				sm.locked        = true;
				semanticBuffer   = ((PxU8*)m_lockedBuffer) + sm.offset;
				stride           = m_stride;
			}
		}
	}
	return semanticBuffer;
}

void RendererInstanceBuffer::unlockSemantic(Semantic semantic)
{
	RENDERER_ASSERT(semantic < NUM_SEMANTICS, "Invalid InstanceBuffer Semantic!");
	if(semantic < NUM_SEMANTICS)
	{
		SemanticDesc &sm = m_semanticDescs[semantic];
		RENDERER_ASSERT(m_lockedBuffer && m_numSemanticLocks && sm.locked, "Trying to unlock a semantic that was not locked.");
		if(m_lockedBuffer && m_numSemanticLocks && sm.locked)
		{
			sm.locked = false;
			m_numSemanticLocks--;
		}
		if(m_lockedBuffer && m_numSemanticLocks == 0)
		{
			unlock();
			m_lockedBuffer = 0;
		}
	}
}

