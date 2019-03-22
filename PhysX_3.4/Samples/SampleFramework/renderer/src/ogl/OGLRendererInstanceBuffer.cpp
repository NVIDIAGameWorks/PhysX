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

#if defined(RENDERER_ENABLE_OPENGL) 

#include "OGLRendererInstanceBuffer.h"
#include <RendererInstanceBufferDesc.h>

using namespace SampleRenderer;

OGLRendererInstanceBuffer::OGLRendererInstanceBuffer(const RendererInstanceBufferDesc &desc) :
	RendererInstanceBuffer(desc)
{
	m_bufferSize   = (PxU32)(desc.maxInstances * m_stride);
	m_buffer       = malloc(m_bufferSize);//PX_ALLOC(m_bufferSize);
	m_maxInstances = desc.maxInstances;
}

OGLRendererInstanceBuffer::~OGLRendererInstanceBuffer(void)
{
	if(m_buffer) free(m_buffer);//PX_FREE(m_buffer);
}

physx::PxMat44 OGLRendererInstanceBuffer::getModelMatrix(PxU32 index) const
{
	physx::PxMat44 model = PxMat44(PxIdentity);
	if(index < m_maxInstances)
	{
		const void *instance = ((PxU8*)m_buffer)+(m_stride*index);
		PxVec3 column0 = getInstanceColumn(instance, m_semanticDescs[SEMANTIC_NORMALX]);
		PxVec3 column1 = getInstanceColumn(instance, m_semanticDescs[SEMANTIC_NORMALY]);
		PxVec3 column2 = getInstanceColumn(instance, m_semanticDescs[SEMANTIC_NORMALZ]);
		PxVec3 column3 = getInstanceColumn(instance, m_semanticDescs[SEMANTIC_POSITION]);

		model = PxMat44(column0, column1, column2, column3);
	}
	return model;
}

PxVec3 OGLRendererInstanceBuffer::getInstanceColumn(const void *instance, const OGLRendererInstanceBuffer::SemanticDesc &sd) const
{
	PxVec3 col = *(PxVec3*)(((PxU8*)instance)+sd.offset);
	return col;
}

void *OGLRendererInstanceBuffer::lock(void)
{
	return m_buffer;
}

void OGLRendererInstanceBuffer::unlock(void)
{

}

void OGLRendererInstanceBuffer::bind(PxU32 streamID, PxU32 firstInstance) const
{

}

void OGLRendererInstanceBuffer::unbind(PxU32 streamID) const
{

}

#endif // #if defined(RENDERER_ENABLE_OPENGL) 
