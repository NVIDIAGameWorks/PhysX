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

#pragma once

#include "GFSDK_SSAO.h"
#include "Shader.h"

class HBAOHelper
{
public:
	HBAOHelper(float fov, float zNear, float zFar);
	~HBAOHelper();

	bool init();
	bool renderAO(void(*renderScene)(), GLuint oldFBO = 0, bool useNormalTexture = true);
	void resize(int wAA, int hAA, int realw, int realH);

private:
	GFSDK_SSAO_Parameters	mAoParams;
	GFSDK_SSAO_GLFunctions	mGLFunctions;
	GFSDK_SSAO_Context_GL*	mHbaoGlContext;
	float mNormalMapTransform[16];
	float mFov;
	float mZnear;
	float mZFar;
	GLint mWidthAA;
	GLint mHeightAA;
	GLint mWidthReal;
	GLint mHeightReal;
	GLuint mNormalTex;
	GLuint mDepthTex;
	GLuint mFBO;
	GLuint mDownScaledFBO;
	GLuint mColorTex;
};
