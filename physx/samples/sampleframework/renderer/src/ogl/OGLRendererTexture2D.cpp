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

#include "OGLRendererTexture2D.h"
#include <RendererTexture2DDesc.h>

using namespace SampleRenderer;

static GLuint getGLPixelFormat(RendererTexture2D::Format format)
{
	GLuint glformat = 0;
	switch(format)
	{
	case RendererTexture2D::FORMAT_B8G8R8A8: glformat = GL_BGRA;            break;
	case RendererTexture2D::FORMAT_R8G8B8A8: glformat = GL_RGBA;            break;
	case RendererTexture2D::FORMAT_A8:       glformat = GL_ALPHA;           break;
	case RendererTexture2D::FORMAT_R32F:     glformat = GL_LUMINANCE;       break;
	case RendererTexture2D::FORMAT_D16:      glformat = GL_DEPTH_COMPONENT; break;
	case RendererTexture2D::FORMAT_D24S8:    glformat = GL_DEPTH_COMPONENT; break;
	default:
		// The pixel type need not be defined (in the case of compressed formats)
		break;
	}
	return glformat;
}

static GLuint getGLPixelInternalFormat(RendererTexture2D::Format format)
{
	GLuint glinternalformat = 0;
	switch(format)
	{
	case RendererTexture2D::FORMAT_B8G8R8A8:
		glinternalformat = GL_RGBA8;
		break;
	case RendererTexture2D::FORMAT_R8G8B8A8:
		glinternalformat = GL_RGBA8;
		break;
	case RendererTexture2D::FORMAT_A8:
		glinternalformat = GL_ALPHA8;
		break;
	case RendererTexture2D::FORMAT_R32F:
		if(GLEW_ARB_texture_float)      glinternalformat = GL_LUMINANCE32F_ARB;
		else if(GLEW_ATI_texture_float) glinternalformat = GL_LUMINANCE_FLOAT32_ATI;
		break;
	case RendererTexture2D::FORMAT_DXT1:
		glinternalformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		break;
	case RendererTexture2D::FORMAT_DXT3:
		glinternalformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		break;
	case RendererTexture2D::FORMAT_DXT5:
		glinternalformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		break;

	case RendererTexture2D::FORMAT_D16:
		glinternalformat = GL_DEPTH_COMPONENT16_ARB;
		break;
	case RendererTexture2D::FORMAT_D24S8:
		glinternalformat = GL_DEPTH_COMPONENT24_ARB;
		break;
	default: RENDERER_ASSERT(0, "Unsupported GL pixel internal format."); break;
	}
	RENDERER_ASSERT(glinternalformat, "Unable to compute GL Internal Format.");
	return glinternalformat;
}

static GLuint getGLPixelType(RendererTexture2D::Format format)
{
	GLuint gltype = 0;
	switch(format)
	{
	case RendererTexture2D::FORMAT_B8G8R8A8:
	case RendererTexture2D::FORMAT_R8G8B8A8:
		gltype = GL_UNSIGNED_BYTE;
		break;
	case RendererTexture2D::FORMAT_A8:
		gltype = GL_UNSIGNED_BYTE;
		break;
	case RendererTexture2D::FORMAT_R32F:
		gltype = GL_FLOAT;
		break;
	case RendererTexture2D::FORMAT_D16:
		gltype = GL_UNSIGNED_SHORT;
		break;
	case RendererTexture2D::FORMAT_D24S8:
		gltype = GL_UNSIGNED_INT;
		break;
	default:
		// The pixel type need not be defined (in the case of compressed formats)
		break;
	}
	return gltype;
}

static GLint getGLTextureFilter(RendererTexture2D::Filter filter, bool mipmap)
{
	GLint glfilter = 0;
	switch(filter)
	{
	case RendererTexture2D::FILTER_NEAREST:     glfilter = mipmap ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST; break;
	case RendererTexture2D::FILTER_LINEAR:      glfilter = mipmap ? GL_LINEAR_MIPMAP_LINEAR   : GL_LINEAR;  break;
	case RendererTexture2D::FILTER_ANISOTROPIC: glfilter = mipmap ? GL_LINEAR_MIPMAP_LINEAR   : GL_LINEAR;  break;
	default: break;
	}
	RENDERER_ASSERT(glfilter, "Unable to convert to OpenGL Filter mode.");
	return glfilter;
}

static GLint getGLTextureAddressing(RendererTexture2D::Addressing addressing)
{
	GLint glwrap = 0;
	switch(addressing)
	{
	case RendererTexture2D::ADDRESSING_WRAP:   glwrap = GL_REPEAT;          break;
	case RendererTexture2D::ADDRESSING_CLAMP:  glwrap = GL_CLAMP;           break;
	case RendererTexture2D::ADDRESSING_MIRROR: glwrap = GL_MIRRORED_REPEAT; break;
	default: break;
	}
	RENDERER_ASSERT(glwrap, "Unable to convert to OpenGL Addressing mode.");
	return glwrap;
}

OGLRendererTexture2D::OGLRendererTexture2D(const RendererTexture2DDesc &desc) :
RendererTexture2D(desc)
{
	RENDERER_ASSERT(desc.depth == 1, "Invalid depth for 3D Texture!");

	m_textureid        = 0;
	m_glformat         = 0;
	m_glinternalformat = 0;
	m_gltype           = 0;
	m_numLevels        = 0;
	m_data             = 0;

	m_glformat         = getGLPixelFormat(desc.format);
	m_glinternalformat = getGLPixelInternalFormat(desc.format);
	m_gltype           = getGLPixelType(desc.format);

	glGenTextures(1, &m_textureid);
	RENDERER_ASSERT(m_textureid, "Failed to create OpenGL Texture.");
	if(m_textureid)
	{
		m_width     = desc.width;
		m_height    = desc.height;
		m_numLevels = desc.numLevels;
		m_data = new PxU8*[m_numLevels];
		memset(m_data, 0, sizeof(PxU8*)*m_numLevels);

		glBindTexture(GL_TEXTURE_2D, m_textureid);
		if(isDepthStencilFormat(desc.format))
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB,   GL_INTENSITY);
		}
		if(isCompressedFormat(desc.format))
		{
			for(PxU32 i=0; i<desc.numLevels; i++)
			{
				PxU32 w = getLevelDimension(m_width,  i);
				PxU32 h = getLevelDimension(m_height, i);
				PxU32 levelSize = computeImageByteSize(w, h, 1, desc.format);
				m_data[i]       = new PxU8[levelSize];
				memset(m_data[i], 0, levelSize);

				// limit the maximum level used for mip mapping (we don't update levels < 4 pixels wide)
				if (w >= 4 && h >= 4)
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, float(i));

				glCompressedTexImage2D(GL_TEXTURE_2D, (GLint)i, m_glinternalformat, w, h, 0, levelSize, m_data[i]);
			}
		}
		else
		{
			RENDERER_ASSERT(m_glformat, "Unknown OpenGL Format!");
			for(PxU32 i=0; i<desc.numLevels; i++)
			{
				PxU32 w = getLevelDimension(m_width,  i);
				PxU32 h = getLevelDimension(m_height, i);
				PxU32 levelSize = computeImageByteSize(w, h, 1, desc.format);
				m_data[i]       = new PxU8[levelSize];
				memset(m_data[i], 0, levelSize);
				glTexImage2D(GL_TEXTURE_2D, (GLint)i, m_glinternalformat, w, h, 0, m_glformat, m_gltype, m_data[i]);
			}
		}


		// set filtering mode...
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, getGLTextureFilter(desc.filter, m_numLevels > 1));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, getGLTextureFilter(desc.filter, false));
		if(desc.filter == FILTER_ANISOTROPIC)
		{
			GLfloat maxAnisotropy = 1.0f;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
		}

		// set addressing modes...
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, getGLTextureAddressing(desc.addressingU));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, getGLTextureAddressing(desc.addressingV));

		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

OGLRendererTexture2D::~OGLRendererTexture2D(void)
{
	if(m_textureid)
	{
		glDeleteTextures(1, &m_textureid);
	}
	if(m_data)
	{
		for(PxU32 i=0; i<m_numLevels; i++)
		{
			delete [] m_data[i];
		}
		delete [] m_data;
	}
}

void *OGLRendererTexture2D::lockLevel(PxU32 level, PxU32 &pitch)
{
	void *buffer = 0;
	RENDERER_ASSERT(level < m_numLevels, "Level out of range!");
	if(level < m_numLevels)
	{
		buffer = m_data[level];
		pitch  = getFormatNumBlocks(getWidth()>>level, getFormat()) * getBlockSize();
	}
	return buffer;
}

void OGLRendererTexture2D::unlockLevel(PxU32 level)
{
	RENDERER_ASSERT(level < m_numLevels, "Level out of range!");
	if(m_textureid && level < m_numLevels)
	{
		PxU32 w = getLevelDimension(getWidth(),  level);
		PxU32 h = getLevelDimension(getHeight(), level);
		glBindTexture(GL_TEXTURE_2D, m_textureid);
		if(isCompressedFormat(getFormat()))
		{
			PxU32 levelSize = computeImageByteSize(w, h, 1, getFormat());

			// Some platforms don't like smaller than 4x4 compressed textures
			if (w >= 4 && h >= 4)
				glCompressedTexSubImage2D(GL_TEXTURE_2D, (GLint)level, 0, 0, w, h, m_glinternalformat, levelSize, m_data[level]);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, (GLint)level, m_glinternalformat, w, h, 0, m_glformat, m_gltype, m_data[level]);
		}
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void OGLRendererTexture2D::bind(PxU32 textureUnit)
{
	if(m_textureid)
	{
		glActiveTextureARB(GL_TEXTURE0_ARB + textureUnit);
		glClientActiveTextureARB(GL_TEXTURE0_ARB + textureUnit);
		glBindTexture(GL_TEXTURE_2D, m_textureid);
	}
}

#endif // #if defined(RENDERER_ENABLE_OPENGL)
