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

#include "RendererConfig.h"
#include <SamplePlatform.h>

#if defined(RENDERER_ENABLE_OPENGL)

#include "OGLRendererVertexBuffer.h"

#include <RendererVertexBufferDesc.h>

#if PX_WINDOWS
#include "cudamanager/PxCudaContextManager.h"
#endif

using namespace SampleRenderer;

OGLRendererVertexBuffer::OGLRendererVertexBuffer(const RendererVertexBufferDesc &desc) :
	RendererVertexBuffer(desc)
,	m_vbo(0)
,	m_access(0)
{
	RENDERER_ASSERT(GLEW_ARB_vertex_buffer_object, "Vertex Buffer Objects not supported on this machine!");
	if(GLEW_ARB_vertex_buffer_object)
	{
		GLenum usage = GL_STATIC_DRAW_ARB;
		m_access = GL_READ_WRITE;
		if(getHint() == HINT_DYNAMIC)
		{
			usage = GL_DYNAMIC_DRAW_ARB;
			m_access = GL_WRITE_ONLY;
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

#if PX_WINDOWS
			if(m_interopContext && m_mustBeRegisteredInCUDA)
			{
				RENDERER_ASSERT(m_deferredUnlock == false, "Deferred VB Unlock must be disabled when CUDA Interop is in use.")
					m_registeredInCUDA = m_interopContext->registerResourceInCudaGL(m_InteropHandle, (PxU32) m_vbo);
			}
#endif
		}
	}
}

OGLRendererVertexBuffer::~OGLRendererVertexBuffer(void)
{
	if(m_vbo)
	{
#if PX_WINDOWS
		if(m_registeredInCUDA)
		{
			m_interopContext->unregisterResourceInCuda(m_InteropHandle);
		}
#endif
		glDeleteBuffersARB(1, &m_vbo);
	}
}

PxU32 OGLRendererVertexBuffer::convertColor(const RendererColor& color)
{
#if defined(RENDERER_WINDOWS)
	return color.a << 24 | color.b << 16 | color.g << 8 | color.r;
#elif defined (RENDERER_MACOSX)
	return color.a << 24 | color.b << 16 | color.g << 8 | color.r; // TODO: JPB: Check if this is the right order
#elif defined(RENDERER_LINUX)
	return color.a << 24 | color.b << 16 | color.g << 8 | color.r;
#else
	PX_ASSERT(!"Platform not implemented");
	return (PxU32&)color;
#endif
}

void OGLRendererVertexBuffer::swizzleColor(void *colors, PxU32 stride, PxU32 numColors, RendererVertexBuffer::Format inFormat)
{
	if (inFormat == RendererVertexBuffer::FORMAT_COLOR_BGRA)
	{
		const void *end = ((PxU8*)colors)+(stride*numColors);
		for(PxU8* iterator = (PxU8*)colors; iterator < end; iterator+=stride)
		{
			// swizzle red and blue
			std::swap(((PxU8*)iterator)[0], ((PxU8*)iterator)[2]);
		}
	}
}

void *OGLRendererVertexBuffer::lock(void)
{
	PX_PROFILE_ZONE("OGLRendererVertexBufferLock",0);
	void *buffer = 0;
	if(m_vbo)
	{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vbo);
		buffer = glMapBuffer(GL_ARRAY_BUFFER, m_access);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	}
	return buffer;
}

void OGLRendererVertexBuffer::unlock(void)
{
	if(m_vbo)
	{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vbo);
		glUnmapBuffer(GL_ARRAY_BUFFER);
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
	default: break;
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
	default: break;
	}
	RENDERER_ASSERT(type, "Unable to compute GLType for Vertex Buffer Format.");
	return type;
}

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
	case RendererVertexBuffer::SEMANTIC_BONEINDEX:
	case RendererVertexBuffer::SEMANTIC_BONEWEIGHT:
		glsemantic = GL_TEXTURE_COORD_ARRAY;
		break;
	default:
		break;
	}
	// There is no reason why certain semantics can go unsupported by OpenGL
	//RENDERER_ASSERT(glsemantic, "Unable to compute the GL Semantic for Vertex Buffer Semantic.");
	return glsemantic;
}

void OGLRendererVertexBuffer::bind(PxU32 streamID, PxU32 firstVertex)
{
	PX_PROFILE_ZONE("OGLRendererVertexBufferBind",0);
	prepareForRender();
	const PxU8 *buffer = ((PxU8*)0) + firstVertex*m_stride;
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
						glVertexPointer(getGLFormatSize(sm.format), getGLFormatType(sm.format), m_stride, buffer+sm.offset);
					}
					break;
				case SEMANTIC_COLOR:
					// swizzling was already handled in unlock()
					RENDERER_ASSERT((sm.format == FORMAT_COLOR_BGRA || sm.format == FORMAT_COLOR_RGBA || sm.format == FORMAT_COLOR_NATIVE), "Unsupported Vertex Buffer Format for COLOR semantic.");
					if(sm.format == FORMAT_COLOR_BGRA || sm.format == FORMAT_COLOR_RGBA || sm.format == FORMAT_COLOR_NATIVE)
					{
						glColorPointer(getGLFormatSize(sm.format), getGLFormatType(sm.format), m_stride, buffer+sm.offset);
					}
					break;
				case SEMANTIC_NORMAL:
					RENDERER_ASSERT((sm.format == FORMAT_FLOAT3 || sm.format == FORMAT_FLOAT4), "Unsupported Vertex Buffer Format for NORMAL semantic.");
					if(sm.format == FORMAT_FLOAT3 || sm.format == FORMAT_FLOAT4)
					{
						glNormalPointer(getGLFormatType(sm.format), m_stride, buffer+sm.offset);
					}
					break;
				case SEMANTIC_TANGENT:
					RENDERER_ASSERT((sm.format == FORMAT_FLOAT3 || sm.format == FORMAT_FLOAT4), "Unsupported Vertex Buffer Format for TANGENT semantic.");
					if(sm.format == FORMAT_FLOAT3 || sm.format == FORMAT_FLOAT4)
					{
						const PxU32 channel = RENDERER_TANGENT_CHANNEL;
						glActiveTextureARB((GLenum)(GL_TEXTURE0_ARB + channel));
						glClientActiveTextureARB((GLenum)(GL_TEXTURE0_ARB + channel));
						glTexCoordPointer(getGLFormatSize(sm.format), getGLFormatType(sm.format), m_stride, buffer+sm.offset);
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
						glTexCoordPointer(getGLFormatSize(sm.format), getGLFormatType(sm.format), m_stride, buffer+sm.offset);
						break;
					}
				case SEMANTIC_BONEINDEX:
					{
						const PxU32 channel = RENDERER_BONEINDEX_CHANNEL;
						glActiveTextureARB((GLenum)(GL_TEXTURE0_ARB + channel));
						glClientActiveTextureARB((GLenum)(GL_TEXTURE0_ARB + channel));
						glTexCoordPointer(getGLFormatSize(sm.format), getGLFormatType(sm.format), m_stride, buffer+sm.offset);
						break;
					}
				case SEMANTIC_BONEWEIGHT:
					{
						const PxU32 channel = RENDERER_BONEWEIGHT_CHANNEL;
						glActiveTextureARB((GLenum)(GL_TEXTURE0_ARB + channel));
						glClientActiveTextureARB((GLenum)(GL_TEXTURE0_ARB + channel));
						glTexCoordPointer(getGLFormatSize(sm.format), getGLFormatType(sm.format), m_stride, buffer+sm.offset);
						break;
					}
				case SEMANTIC_DISPLACEMENT_TEXCOORD:
				case SEMANTIC_DISPLACEMENT_FLAGS:
					break;
				default:
					RENDERER_ASSERT(0, "Unable to bind Vertex Buffer Semantic.");
				}

				GLenum glsemantic = getGLSemantic(semantic);
				if(glsemantic)
				{
					glEnableClientState(getGLSemantic(semantic));
				}
			}
		}
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	}
}

void OGLRendererVertexBuffer::unbind(PxU32 streamID)
{
	for(PxU32 i=0; i<NUM_SEMANTICS; i++)
	{
		Semantic semantic = (Semantic)i;
		const SemanticDesc &sm = m_semanticDescs[semantic];
		if(sm.format < NUM_FORMATS)
		{
			GLenum glsemantic = getGLSemantic(semantic);
			if(glsemantic != GL_TEXTURE_COORD_ARRAY)
			{
				glDisableClientState(glsemantic);
			}
		}
	}
	for(PxU32 i=0; i<8; i++)
	{
		glActiveTextureARB((GLenum)(GL_TEXTURE0_ARB + i));
		glClientActiveTextureARB((GLenum)(GL_TEXTURE0_ARB + i));
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
}

#endif // #if defined(RENDERER_ENABLE_OPENGL)
