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

#include <GL/glew.h>
#include "HBAOHelper.h"
#include "foundation/PxMat44.h"
#include "stdio.h"

using namespace physx;

static PxMat44 PerspectiveProjectionMatrix(float fovy, float x, float y, float n, float f)
{
	float PPM[16];

	float coty = 1.0f / tan(fovy * physx::PxPi / 360.0f);
	float aspect = x / (y > 0.0f ? y : 1.0f);

	PPM[0] = coty / aspect;
	PPM[1] = 0.0f;
	PPM[2] = 0.0f;
	PPM[3] = 0.0f;

	PPM[4] = 0.0f;
	PPM[5] = coty;
	PPM[6] = 0.0f;
	PPM[7] = 0.0f;

	PPM[8] = 0.0f;
	PPM[9] = 0.0f;
	PPM[10] = (n + f) / (n - f);
	PPM[11] = -1.0f;

	PPM[12] = 0.0f;
	PPM[13] = 0.0f;
	PPM[14] = 2.0f * n * f / (n - f);
	PPM[15] = 0.0f;

	return PxMat44(PPM);
}

#ifndef CHECK_GL_ERROR
#define CHECK_GL_ERROR() checkGLError(__FILE__, __LINE__)
#endif

void checkGLError(const char* file, int32_t line)
{
#if defined(_DEBUG)
	GLint error = glGetError();
	if (error)
	{
		const char* errorString = 0;
		switch (error)
		{
		case GL_INVALID_ENUM: errorString = "GL_INVALID_ENUM"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: errorString = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
		case GL_INVALID_VALUE: errorString = "GL_INVALID_VALUE"; break;
		case GL_INVALID_OPERATION: errorString = "GL_INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY: errorString = "GL_OUT_OF_MEMORY"; break;
		default: errorString = "unknown error"; break;
		}
		printf("GL error: %s, line %d: %s\n", file, line, errorString);
		error = 0; // nice place to hang a breakpoint in compiler... :)
	}
#endif
}

HBAOHelper::HBAOHelper(float fov, float zNear, float zFar)
	: mHbaoGlContext(NULL)
	, mFov(fov)
	, mZnear(zNear)
	, mZFar(zFar)
	, mWidthAA(0)
	, mHeightAA(0)
	, mWidthReal(0)
	, mHeightReal(0)
	, mNormalTex(-1)
	, mDepthTex(-1)
	, mFBO(0)
	, mDownScaledFBO(0)
	, mColorTex(-1)
{
	init();
}

HBAOHelper::~HBAOHelper()
{
	if (mHbaoGlContext)
		mHbaoGlContext->Release();

	glDeleteTextures(1, &mNormalTex);
	glDeleteTextures(1, &mDepthTex);
	glDeleteTextures(1, &mColorTex);
	glDeleteFramebuffers(1, &mFBO);
	glDeleteFramebuffers(1, &mDownScaledFBO);

	CHECK_GL_ERROR();
}

bool HBAOHelper::init()
{
	memset(mNormalMapTransform, 0, sizeof(float)*16);
	mNormalMapTransform[0] = -1.0f;
	mNormalMapTransform[5] = 1.0f;
	mNormalMapTransform[10] = 1.0f;
	mNormalMapTransform[15] = 1.0f;

	glGenTextures(1, &mNormalTex);
	glGenTextures(1, &mDepthTex);
	glGenTextures(1, &mColorTex);
	glGenFramebuffers(1, &mFBO);
	glGenFramebuffers(1, &mDownScaledFBO);
	CHECK_GL_ERROR();

	GFSDK_SSAO_CustomHeap CustomHeap;
	CustomHeap.new_ = ::operator new;
	CustomHeap.delete_ = ::operator delete;

	GFSDK_SSAO_INIT_GL_FUNCTIONS(mGLFunctions);

	GFSDK_SSAO_Status status = GFSDK_SSAO_CreateContext_GL(&mHbaoGlContext, &mGLFunctions, &CustomHeap);
	if (status != GFSDK_SSAO_OK)
		return false;

	GFSDK_SSAO_Version Version;
	status = GFSDK_SSAO_GetVersion(&Version);

	mAoParams.Radius = 1.0f;
	mAoParams.Bias = 0.5f;
	mAoParams.NearAO = 4.0f;
	mAoParams.FarAO = 1.5f;

	mAoParams.BackgroundAO.Enable = false;
	mAoParams.BackgroundAO.BackgroundViewDepth = 1.f;

	mAoParams.ForegroundAO.Enable = false;
	mAoParams.ForegroundAO.ForegroundViewDepth = 1.0f;

	mAoParams.DepthStorage = true ? GFSDK_SSAO_FP16_VIEW_DEPTHS : GFSDK_SSAO_FP32_VIEW_DEPTHS;
	mAoParams.PowerExponent = 2.0f;
	mAoParams.DepthClampMode = false ? GFSDK_SSAO_CLAMP_TO_BORDER : GFSDK_SSAO_CLAMP_TO_EDGE;
	mAoParams.Blur.Enable = true;
	mAoParams.Blur.Sharpness = 16.0f;
	mAoParams.Blur.Radius = GFSDK_SSAO_BLUR_RADIUS_4;

	return status == GFSDK_SSAO_OK;
}

void HBAOHelper::resize(int wAA, int hAA, int realw, int realH)
{
	mWidthAA = wAA;
	mHeightAA = hAA;

	mWidthReal = realw;
	mHeightReal = realH;

	glViewport(0, 0, mWidthReal, mHeightReal);
	PxMat44 Projection = PerspectiveProjectionMatrix(mFov, float(mWidthReal), float(mHeightReal), mZnear, mZFar);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(&Projection.column0.x);

	glBindTexture(GL_TEXTURE_2D, mNormalTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mWidthReal, mHeightReal, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, mDepthTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, mWidthReal, mHeightReal, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);

	GFSDK_SSAO_Status status = mHbaoGlContext->PreCreateFBOs(mAoParams, mWidthReal, mHeightReal);

	glBindTexture(GL_TEXTURE_2D, mColorTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mWidthReal, mHeightReal, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glBindFramebuffer(GL_FRAMEBUFFER, mDownScaledFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorTex, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	CHECK_GL_ERROR();
}

bool HBAOHelper::renderAO(void(*renderScene)(), GLuint oldFBO, bool useNormalTexture)
{
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mNormalTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mDepthTex, 0);

	glViewport(0, 0, mWidthReal, mHeightReal);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	PxMat44 Projection = PerspectiveProjectionMatrix(mFov, float(mWidthReal), float(mHeightReal), mZnear, mZFar);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(&Projection.column0.x);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	renderScene();
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	CHECK_GL_ERROR();

	//
	// render AO
	//

	GFSDK_SSAO_InputData_GL Input;
	Input.DepthData.DepthTextureType = GFSDK_SSAO_HARDWARE_DEPTHS;
	Input.DepthData.FullResDepthTexture = GFSDK_SSAO_Texture_GL(GL_TEXTURE_2D, mDepthTex);
	Input.DepthData.ProjectionMatrix.Data = GFSDK_SSAO_Float4x4(&Projection.column0.x);
	Input.DepthData.ProjectionMatrix.Layout = GFSDK_SSAO_ROW_MAJOR_ORDER;
	Input.DepthData.MetersToViewSpaceUnits = 1.0f;

	if (useNormalTexture)
	{	
		Input.NormalData.Enable = true;
		Input.NormalData.FullResNormalTexture = GFSDK_SSAO_Texture_GL(GL_TEXTURE_2D, mNormalTex);
		Input.NormalData.WorldToViewMatrix.Data = GFSDK_SSAO_Float4x4(mNormalMapTransform);
		Input.NormalData.WorldToViewMatrix.Layout = GFSDK_SSAO_ROW_MAJOR_ORDER;
		Input.NormalData.DecodeScale = -2.f;
		Input.NormalData.DecodeBias = 1.0f;
	}

	bool renderDirectlyToOldFbo = ((mHeightReal == mHeightAA) && (mWidthReal == mWidthAA));
	bool showDebugNormals = false;
	bool showHBAO = false;

	GFSDK_SSAO_RenderMask RenderMask = showDebugNormals ? GFSDK_SSAO_RENDER_DEBUG_NORMAL : GFSDK_SSAO_RENDER_AO;
	GFSDK_SSAO_Output_GL Output;
	Output.OutputFBO = renderDirectlyToOldFbo ? oldFBO : mDownScaledFBO;
	Output.Blend.Mode = !renderDirectlyToOldFbo || showHBAO ? GFSDK_SSAO_OVERWRITE_RGB : GFSDK_SSAO_MULTIPLY_RGB;

	GFSDK_SSAO_Status status;
	status = mHbaoGlContext->RenderAO(Input, mAoParams, Output, RenderMask);

	if (!renderDirectlyToOldFbo)
	{
		// upscale to oldFbo
		glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);
		
		glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
		glEnable(GL_BLEND);
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glBlendFuncSeparate(GL_ZERO, GL_SRC_COLOR, GL_ZERO, GL_ONE);
	
		glViewport(0, 0, mWidthAA, mHeightAA);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, mWidthAA, 0, mHeightAA, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		glUseProgram(0);

		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mColorTex);
		
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(float(mWidthAA), 0.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(float(mWidthAA), float(mHeightAA));
		glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, float(mHeightAA));
		glEnd();
		
		glEnable(GL_LIGHTING);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_BLEND);

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
	
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	CHECK_GL_ERROR();
	return true;
}
