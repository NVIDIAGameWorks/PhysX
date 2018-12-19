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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.

#include "RenderTarget.h"


// -------------------------------------------------------------------------------------------
RenderTarget::RenderTarget(int width, int height)
{
	mColorTexId = 0;
	mDepthTexId = 0;
	mFBO = NULL;

	resize(width, height);
}

// -------------------------------------------------------------------------------------------
void RenderTarget::clear()
{
	if (mColorTexId > 0) {
		glDeleteTextures(1, &mColorTexId);
		mColorTexId = 0;
	}
	if (mDepthTexId > 0) {
		glDeleteTextures(1, &mDepthTexId);
		mDepthTexId = 0;
	}
	if (mFBO)
		delete mFBO;

	mFBO = NULL;
}

// -------------------------------------------------------------------------------------------
RenderTarget::~RenderTarget()
{
	clear();
}

// -------------------------------------------------------------------------------------------
GLuint RenderTarget::createTexture(GLenum target, int width, int height, GLint internalFormat, GLenum format)
{
	GLuint texid;
    glGenTextures(1, &texid);
    glBindTexture(target, texid);

    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(target, 0, internalFormat, width, height, 0, format, GL_FLOAT, 0);
    return texid;
}

// -------------------------------------------------------------------------------------------
void RenderTarget::resize(int width, int height)
{
	clear();
	mFBO = new FrameBufferObject();
	mColorTexId = createTexture(GL_TEXTURE_RECTANGLE_ARB, width, height, GL_RGBA8, GL_RGBA);
    mDepthTexId = createTexture(GL_TEXTURE_RECTANGLE_ARB, width, height, GL_DEPTH_COMPONENT32_ARB, GL_DEPTH_COMPONENT);
}

// -------------------------------------------------------------------------------------------
void RenderTarget::beginCapture()
{
	mFBO->Bind();
    mFBO->AttachTexture(GL_TEXTURE_RECTANGLE_ARB, mColorTexId, GL_COLOR_ATTACHMENT0_EXT);
    mFBO->AttachTexture(GL_TEXTURE_RECTANGLE_ARB, mDepthTexId, GL_DEPTH_ATTACHMENT_EXT);
    mFBO->IsValid();
}

// -------------------------------------------------------------------------------------------
void RenderTarget::endCapture()
{
    mFBO->AttachTexture(GL_TEXTURE_RECTANGLE_ARB, 0, GL_COLOR_ATTACHMENT0_EXT);
    mFBO->AttachTexture(GL_TEXTURE_RECTANGLE_ARB, 0, GL_DEPTH_ATTACHMENT_EXT);
    mFBO->Disable();
}



