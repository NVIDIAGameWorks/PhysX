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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.


#include "GLES2RendererIndexBuffer.h"

#if defined(RENDERER_ENABLE_GLES2) 
#include <RendererIndexBufferDesc.h>

namespace SampleRenderer
{

GLES2RendererIndexBuffer::GLES2RendererIndexBuffer(const RendererIndexBufferDesc &desc) :
	RendererIndexBuffer(desc)
{
	m_indexSize  = getFormatByteSize(getFormat());
	RENDERER_ASSERT(GLEW_ARB_vertex_buffer_object, "Vertex Buffer Objects not supported on this machine!");
	if(GLEW_ARB_vertex_buffer_object)
	{
		RENDERER_ASSERT(desc.maxIndices > 0 && desc.maxIndices > 0, "Cannot create zero size Index Buffer.");
		if(desc.maxIndices > 0 && desc.maxIndices > 0)
		{
			GLenum usage = GL_STATIC_DRAW_ARB;
			if(getHint() == HINT_DYNAMIC)
			{
				usage = GL_DYNAMIC_DRAW_ARB;
			}
			
			glGenBuffersARB(1, &m_ibo);
			RENDERER_ASSERT(m_ibo, "Failed to create Index Buffer.");
			if(m_ibo)
			{
				m_maxIndices = desc.maxIndices;
				const PxU32 bufferSize = m_indexSize * m_maxIndices;
			
				glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_ibo);
				glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, bufferSize, 0, usage);
				glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
			}
		}
    }
}

GLES2RendererIndexBuffer::~GLES2RendererIndexBuffer(void)
{
	if(m_ibo)
	{
		glDeleteBuffersARB(1, &m_ibo);
	}
}

void *GLES2RendererIndexBuffer::lock(void)
{
	void *buffer = 0;
	if(m_ibo)
	{
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_ibo);
		buffer = glMapBufferOES(GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}
	return buffer;
}

void GLES2RendererIndexBuffer::unlock(void)
{
	if(m_ibo)
	{
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_ibo);
		glUnmapBufferOES(GL_ELEMENT_ARRAY_BUFFER_ARB);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}
}

void GLES2RendererIndexBuffer::bind(void) const
{
	if(m_ibo)
	{
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_ibo);
	}
}

void GLES2RendererIndexBuffer::unbind(void) const
{
	if(m_ibo)
	{
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}
	}
}

#endif // #if defined(RENDERER_ENABLE_GLES2)
