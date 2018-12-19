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

