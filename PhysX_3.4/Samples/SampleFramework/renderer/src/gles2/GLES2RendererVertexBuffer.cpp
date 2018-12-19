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


#include "GLES2RendererVertexBuffer.h"
#include "GLES2RendererMaterial.h"

#if defined(RENDERER_ENABLE_GLES2)

#include <RendererVertexBufferDesc.h>

namespace SampleRenderer
{

extern GLES2RendererMaterial *g_hackCurrentMat;

GLES2RendererVertexBuffer::GLES2RendererVertexBuffer(const RendererVertexBufferDesc &desc) :
	RendererVertexBuffer(desc)
{
	m_vbo = 0;
	
	RENDERER_ASSERT(GLEW_ARB_vertex_buffer_object, "Vertex Buffer Objects not supported on this machine!");
	if(GLEW_ARB_vertex_buffer_object)
	{
		GLenum usage = GL_STATIC_DRAW_ARB;
		if(getHint() == HINT_DYNAMIC)
		{
			usage = GL_DYNAMIC_DRAW_ARB;
		}
		
		RENDERER_ASSERT(m_stride && desc.maxVertices, "Unable to create Vertex Buffer of zero size.");
		if(m_stride && desc.maxVertices)
		{
			glGenBuffersARB(1, &m_vbo);
			RENDERER_ASSERT(m_vbo, "Failed to create Vertex Buffer Object.");
		}
		if(m_vbo)
		{
			m_maxVertices = desc.maxVertices;
			PxU32 bufferSize = m_stride * m_maxVertices;
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vbo);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB, bufferSize, 0, usage);
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		}
	}
}

GLES2RendererVertexBuffer::~GLES2RendererVertexBuffer(void)
{
	if(m_vbo)
	{
		glDeleteBuffersARB(1, &m_vbo);
	}
}

void GLES2RendererVertexBuffer::swizzleColor(void *colors, PxU32 stride, PxU32 numColors, RendererVertexBuffer::Format inFormat)
{
	if (inFormat == RendererVertexBuffer::FORMAT_COLOR_RGBA)
	{
		const void *end = ((PxU8*)colors)+(stride*numColors);
		for(PxU8* iterator = (PxU8*)colors; iterator < end; iterator+=stride)
		{
			std::swap(((PxU8*)iterator)[0], ((PxU8*)iterator)[2]);
		}
	}
}

PxU32 GLES2RendererVertexBuffer::convertColor(const RendererColor& color)
{
	return color.a << 24 | color.b << 16 | color.g << 8 | color.r;
}

void *GLES2RendererVertexBuffer::lock(void)
{
	void *buffer = 0;
	if(m_vbo)
	{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vbo);
		buffer = glMapBufferOES(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	}
	return buffer;
}

void GLES2RendererVertexBuffer::unlock(void)
{
	if(m_vbo)
	{
		/* TODO: print out if format is USHORT4 */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vbo);
		glUnmapBufferOES(GL_ARRAY_BUFFER);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	}
}

static GLuint getGLFormatSize(RendererVertexBuffer::Format format)
{
	PxU32 size = 0;
	switch(format)
	{
		case RendererVertexBuffer::FORMAT_FLOAT1:		size = 1; break;
		case RendererVertexBuffer::FORMAT_FLOAT2:		size = 2; break;
		case RendererVertexBuffer::FORMAT_FLOAT3:		size = 3; break;
		case RendererVertexBuffer::FORMAT_FLOAT4:		size = 4; break;
		case RendererVertexBuffer::FORMAT_UBYTE4:		size = 4; break;
		case RendererVertexBuffer::FORMAT_USHORT4:		size = 4; break;
		case RendererVertexBuffer::FORMAT_COLOR_BGRA:	size = 4; break;
		case RendererVertexBuffer::FORMAT_COLOR_RGBA:	size = 4; break;
		case RendererVertexBuffer::FORMAT_COLOR_NATIVE:	size = 4; break;
		default:                                                  break;
	}
	RENDERER_ASSERT(size, "Unable to compute number of Vertex Buffer elements.");
	return size;
}

static GLenum getGLFormatType(RendererVertexBuffer::Format format)
{
	GLenum type = 0;
	switch(format)
	{
		case RendererVertexBuffer::FORMAT_FLOAT1:
		case RendererVertexBuffer::FORMAT_FLOAT2:
		case RendererVertexBuffer::FORMAT_FLOAT3:
		case RendererVertexBuffer::FORMAT_FLOAT4:
			type = GL_FLOAT;
			break;
		case RendererVertexBuffer::FORMAT_UBYTE4:
			type = GL_BYTE;
			break;
		case RendererVertexBuffer::FORMAT_USHORT4:
			type = GL_SHORT;
			break;
		case RendererVertexBuffer::FORMAT_COLOR_BGRA:
		case RendererVertexBuffer::FORMAT_COLOR_RGBA:
		case RendererVertexBuffer::FORMAT_COLOR_NATIVE:
			type = GL_UNSIGNED_BYTE;
			break;
		default:
			break;
	}
	RENDERER_ASSERT(type, "Unable to compute GLType for Vertex Buffer Format.");
	return type;
}

#if !defined(RENDERER_ANDROID) && !defined(RENDERER_IOS)
static GLenum getGLSemantic(RendererVertexBuffer::Semantic semantic)
{
	GLenum glsemantic = 0;
	switch(semantic)
	{
		case RendererVertexBuffer::SEMANTIC_POSITION: glsemantic = GL_VERTEX_ARRAY;        break;
		case RendererVertexBuffer::SEMANTIC_COLOR:    glsemantic = GL_COLOR_ARRAY;         break;
		case RendererVertexBuffer::SEMANTIC_NORMAL:   glsemantic = GL_NORMAL_ARRAY;        break;
		case RendererVertexBuffer::SEMANTIC_TANGENT:  glsemantic = GL_TEXTURE_COORD_ARRAY; break;

		case RendererVertexBuffer::SEMANTIC_TEXCOORD0:
		case RendererVertexBuffer::SEMANTIC_TEXCOORD1:
		case RendererVertexBuffer::SEMANTIC_TEXCOORD2:
		case RendererVertexBuffer::SEMANTIC_TEXCOORD3:
		case RendererVertexBuffer::SEMANTIC_TEXCOORD3:
		case RendererVertexBuffer::SEMANTIC_BONEINDEX:
		case RendererVertexBuffer::SEMANTIC_BONEWEIGHT:
			glsemantic = GL_TEXTURE_COORD_ARRAY;
			break;
	}
	RENDERER_ASSERT(glsemantic, "Unable to compute the GL Semantic for Vertex Buffer Semantic.");
	return glsemantic;
}
#endif

void GLES2RendererVertexBuffer::bindGL(GLint position, const SemanticDesc &sm, PxU8 *buffer)
{
    if (position == -1)
        return;

    glEnableVertexAttribArray(position);
	GLenum type = getGLFormatType(sm.format);
    glVertexAttribPointer(position, getGLFormatSize(sm.format), type, type == GL_UNSIGNED_BYTE, m_stride, buffer+sm.offset);    
}

void GLES2RendererVertexBuffer::unbindGL(GLint pos)
{
    if (pos != -1)
        glDisableVertexAttribArray(pos); 
}

void GLES2RendererVertexBuffer::bind(PxU32 streamID, PxU32 firstVertex)
{
	prepareForRender();
	GLES2RendererMaterial *mat = g_hackCurrentMat;
	PxU8 *buffer = ((PxU8*)0) + firstVertex*m_stride;
	if(m_vbo)
	{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vbo);
		for(PxU32 i=0; i<NUM_SEMANTICS; i++)
		{
			Semantic semantic = (Semantic)i;
			const SemanticDesc &sm = m_semanticDescs[semantic];
			if(sm.format < NUM_FORMATS)
			{
				switch(semantic)
				{
					case SEMANTIC_POSITION:
						RENDERER_ASSERT(sm.format >= FORMAT_FLOAT1 && sm.format <= FORMAT_FLOAT4, "Unsupported Vertex Buffer Format for POSITION semantic.");
						if(sm.format >= FORMAT_FLOAT1 && sm.format <= FORMAT_FLOAT4)
						{
                            bindGL(mat->m_program[mat->m_currentPass].positionAttr, sm, buffer);
						}
						break;
					case SEMANTIC_COLOR:
						RENDERER_ASSERT(sm.format == FORMAT_COLOR_BGRA || sm.format == FORMAT_COLOR_RGBA || sm.format == FORMAT_COLOR_NATIVE || sm.format == FORMAT_FLOAT4, "Unsupported Vertex Buffer Format for COLOR semantic.");
						if(sm.format == FORMAT_COLOR_BGRA || sm.format == FORMAT_COLOR_RGBA || sm.format == FORMAT_COLOR_NATIVE || sm.format == FORMAT_FLOAT4)
						{
                            bindGL(mat->m_program[mat->m_currentPass].colorAttr, sm, buffer);
						}
						break;
					case SEMANTIC_NORMAL:
						RENDERER_ASSERT(sm.format == FORMAT_FLOAT3 || sm.format == FORMAT_FLOAT4, "Unsupported Vertex Buffer Format for NORMAL semantic.");
						if(sm.format == FORMAT_FLOAT3 || sm.format == FORMAT_FLOAT4)
						{
                            bindGL(mat->m_program[mat->m_currentPass].normalAttr, sm, buffer);
						}
						break;
					case SEMANTIC_TANGENT:
						RENDERER_ASSERT(sm.format == FORMAT_FLOAT3 || sm.format == FORMAT_FLOAT4, "Unsupported Vertex Buffer Format for TANGENT semantic.");
						if(sm.format == FORMAT_FLOAT3 || sm.format == FORMAT_FLOAT4)
						{
                            bindGL(mat->m_program[mat->m_currentPass].tangentAttr, sm, buffer);
						}
						break;
					case SEMANTIC_TEXCOORD0:
					case SEMANTIC_TEXCOORD1:
					case SEMANTIC_TEXCOORD2:
					case SEMANTIC_TEXCOORD3:
					{
						const PxU32 channel = semantic - SEMANTIC_TEXCOORD0;
						glActiveTextureARB((GLenum)(GL_TEXTURE0_ARB + channel));
						glClientActiveTextureARB((GLenum)(GL_TEXTURE0_ARB + channel));

                        bindGL(mat->m_program[mat->m_currentPass].texcoordAttr[channel], sm, buffer);

						break;
					}
					case SEMANTIC_BONEINDEX:
					{
						glActiveTextureARB((GLenum)(GL_TEXTURE0_ARB + RENDERER_BONEINDEX_CHANNEL));
						glClientActiveTextureARB((GLenum)(GL_TEXTURE0_ARB + RENDERER_BONEINDEX_CHANNEL));
                        bindGL(mat->m_program[mat->m_currentPass].boneIndexAttr, sm, buffer);
                        break;
					}
					case SEMANTIC_BONEWEIGHT:
					{
						glActiveTextureARB((GLenum)(GL_TEXTURE0_ARB + RENDERER_BONEWEIGHT_CHANNEL));
						glClientActiveTextureARB((GLenum)(GL_TEXTURE0_ARB + RENDERER_BONEWEIGHT_CHANNEL));
                        bindGL(mat->m_program[mat->m_currentPass].boneWeightAttr, sm, buffer);
						break;
					}
					default:
						/*
                        __android_log_print(ANDROID_LOG_DEBUG, "GLES2RendererVertexBuffer", "semantic: %d", i);
						RENDERER_ASSERT(0, "Unable to bind Vertex Buffer Semantic.");
						*/
						break;
				}
			}
		}
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	}
}

void GLES2RendererVertexBuffer::unbind(PxU32 streamID)
{
    GLES2RendererMaterial *mat = g_hackCurrentMat;

	for(PxU32 i=0; i<NUM_SEMANTICS; i++)
	{
		Semantic semantic = (Semantic)i;
		const SemanticDesc &sm = m_semanticDescs[semantic];
		if(sm.format < NUM_FORMATS)
		{
            switch (semantic)
            {
                case SEMANTIC_POSITION: unbindGL(mat->m_program[mat->m_currentPass].positionAttr); break;
                case SEMANTIC_COLOR: unbindGL(mat->m_program[mat->m_currentPass].positionAttr);  break;
                case SEMANTIC_NORMAL: unbindGL(mat->m_program[mat->m_currentPass].normalAttr); break;
                case SEMANTIC_TANGENT: unbindGL(mat->m_program[mat->m_currentPass].tangentAttr); break;
                case SEMANTIC_BONEINDEX: unbindGL(mat->m_program[mat->m_currentPass].boneIndexAttr); break;
                case SEMANTIC_BONEWEIGHT: unbindGL(mat->m_program[mat->m_currentPass].boneWeightAttr); break;
                default:
                	/*
                    __android_log_print(ANDROID_LOG_DEBUG, "GLES2RendererVertexBuffer", "semantic: %d", i);
                    RENDERER_ASSERT(0, "Unable to unbind Vertex Buffer Semantic.");
                    */
                	break;
            }
		}
	}
	for(PxU32 i=0; i<8; i++)
	{
		glActiveTextureARB((GLenum)(GL_TEXTURE0_ARB + i));
		glClientActiveTextureARB((GLenum)(GL_TEXTURE0_ARB + i));

        unbindGL(mat->m_program[mat->m_currentPass].texcoordAttr[i]);
	}
}

}
#endif // #if defined(RENDERER_ENABLE_GLES2)
