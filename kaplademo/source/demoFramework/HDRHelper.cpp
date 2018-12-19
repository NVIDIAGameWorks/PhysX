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

#include "HDRHelper.h"
#include "foundation/PxMat44.h"

namespace
{
PxMat44 PerspectiveProjectionMatrix(float fovy,float x,float y,float n,float f)
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
}

const char *HDRToneMappingVS = STRINGIFY(
    void main(void)
	{
		gl_TexCoord[0] = gl_MultiTexCoord0;
		gl_Position = gl_Vertex * 2.0 - 1.0;
	}
);

const char *HDRBlurHFS = STRINGIFY(
	uniform sampler2D colorTex;
	uniform float sx;

	void main (void)
	{
		vec3 bloom = vec3(0.0, 0.0, 0.0);
		const float hdrScale = 1.5;
		const int kernelSize = 10;
		const float invScale = 1.0 / (hdrScale * float(kernelSize));

		for (int x = -kernelSize; x <= kernelSize; x++)
		{
			float s = gl_TexCoord[0].s + x * sx;
			float t = gl_TexCoord[0].t;
			vec3 color = texture2D(colorTex, vec2(s,t)).rgb;
			float luminance = dot(color, vec3(0.2125, 0.7154, 0.0721));
			if (luminance > 1.0)
			{
				bloom += color * ((kernelSize+1) - abs(float(x)));
			}
		}

		gl_FragColor = vec4(bloom * invScale, 1.0);
	}
);

const char *HDRBlurVFS = STRINGIFY(
	uniform sampler2D colorTex;
	uniform sampler2D blurTex;
	uniform float sy;

	void main (void)
	{
		const float hdrScale = 1.5;
		const int kernelSize = 10;
		const float invScale = 1.0 / (hdrScale * float(kernelSize) * 100.0);

		vec3 colorP = texture2D(colorTex, gl_TexCoord[0]).rgb;
		vec3 bloom = vec3(0.0, 0.0, 0.0);

		for (int y = -kernelSize; y <= kernelSize; y++)
		{
			float s = gl_TexCoord[0].s;
			float t = gl_TexCoord[0].t + y * sy;
			vec3 color = texture2D(blurTex, vec2(s,t)).rgb;
			float luminance = dot(color, vec3(0.2125, 0.7154, 0.0721));
			if (luminance > 1.0)
			{
				bloom += color  * ((kernelSize+1) - abs(float(y)));
			}
		}

		vec3 hdrColor = invScale * bloom + colorP;

		vec3 toneMappedColor = 2.0 * hdrColor / (hdrColor + vec3(1.0));
		gl_FragColor = vec4(toneMappedColor, 1.0);
	}
);

const char *HDRDepthOfFieldFS = STRINGIFY(
	uniform sampler2D colorTex;
	uniform sampler2D depthTex;
	uniform float sx;
	uniform float sy;

	void main(void)
	{
		const float depthEnd = 0.993;
		const float depthSize = 0.01;

		vec3 colorP = texture2D(colorTex, gl_TexCoord[0]).rgb;
		float depth = texture2D(depthTex, gl_TexCoord[0].st).r;

		if ((depth - depthEnd) < depthSize)
		{
			const int depthKernelSize = 5;
			vec3 colorSum = vec3(0.0);
			float cnt = 0.0;
			for (int x = -depthKernelSize; x <= depthKernelSize; x++)
				for (int y = -depthKernelSize; y <= depthKernelSize; y++)
				{
				float s = gl_TexCoord[0].s + x * sy;
				float t = gl_TexCoord[0].t + y * sy;
				float scalex = ((depthKernelSize + 1) - abs(float(x))) / depthKernelSize;
				float scaley = ((depthKernelSize + 1) - abs(float(y))) / depthKernelSize;
				float scale = scalex * scaley;
				vec3 color = texture2D(colorTex, vec2(s, t)).rgb;
				colorSum += scale * color;
				cnt += scale;
				}

			colorSum /= cnt;
			float depthScale = pow(max(0.0f, min(1.0, (abs(depth - depthEnd)) / depthSize)), 1.5);
			colorP = depthScale * colorSum + (1.0 - depthScale) * colorP;
		}

		gl_FragColor = vec4(colorP, 1.0);
	}
);

HDRHelper::HDRHelper(float fov,float padding,float zNear,float zFar, const char* resourcePath, float scale) : fov(fov),padding(padding),zNear(zNear),zFar(zFar), scale(scale) 
{
	mShaderBloomH.loadShaderCode(HDRToneMappingVS, HDRBlurHFS);
	mShaderBloomV.loadShaderCode(HDRToneMappingVS, HDRBlurVFS);
	mShaderDOF.loadShaderCode(HDRToneMappingVS, HDRDepthOfFieldFS);

	glGenTextures(1,&mHDRColorTex);
	glGenTextures(1,&mHDRDepthTex);
	glGenTextures(1,&mHDRBlurTex);
	glGenTextures(1,&mHDRBloomTex);	

	glGenFramebuffers(1,&mHDRFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, mHDRFbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,mHDRColorTex, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	glGenFramebuffers(1,&mHDRBlurFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, mHDRBlurFbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,mHDRBlurTex, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
		
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	//printf("Frame buffer status %d\n\n\n",
	//	(status == GL_FRAMEBUFFER_COMPLETE) ? 1 : 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void HDRHelper::Resize(int w,int h) {

	realWidth = w;
	realHeight = h;
	Width = w*scale;
	Height = h*scale;
	
	fovPad = 2.0f*atan(tan(fov*0.5f*physx::PxPi/180.0f)*(1.0f+padding))*180.0f/physx::PxPi;
	
	glViewport(0,0,w,h);

	// allocate HDR color buffer
	glBindFramebuffer(GL_FRAMEBUFFER, mHDRFbo);

	glBindTexture(GL_TEXTURE_2D,mHDRColorTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,w,h,0,GL_RGBA,GL_FLOAT,NULL);

	glBindTexture(GL_TEXTURE_2D,mHDRDepthTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,w,h,0,GL_DEPTH_COMPONENT,GL_FLOAT,NULL);

	// allocate HDR color buffer for blur operations
	glBindFramebuffer(GL_FRAMEBUFFER, mHDRBlurFbo);
	
	glBindTexture(GL_TEXTURE_2D,mHDRBlurTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,w,h,0,GL_RGBA,GL_FLOAT,NULL);

	glBindTexture(GL_TEXTURE_2D,mHDRBloomTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,w,h,0,GL_RGBA,GL_FLOAT,NULL);

	// set program values
	glUseProgram(mShaderBloomH);
	glUniform1f(glGetUniformLocation(mShaderBloomH,"sx"),1.0f / (float)w);

	glUseProgram(mShaderBloomV);
	glUniform1f(glGetUniformLocation(mShaderBloomV,"sy"),1.0f / (float)h);

	glUseProgram(mShaderDOF);
	glUniform1f(glGetUniformLocation(mShaderDOF,"sx"),1.0f / (float)w);
	glUniform1f(glGetUniformLocation(mShaderDOF,"sy"),1.0f / (float)h);

	glUseProgram(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void HDRHelper::Destroy() {

	mShaderBloomH.deleteShaders();
	mShaderBloomV.deleteShaders();

	glDeleteTextures(1,&mHDRColorTex);
	glDeleteTextures(1,&mHDRDepthTex);
	glDeleteTextures(1,&mHDRBlurTex);
	glDeleteTextures(1,&mHDRBloomTex);

	glDeleteFramebuffers(1,&mHDRFbo);
	glDeleteFramebuffers(1,&mHDRBlurFbo);
}

void HDRHelper::beginHDR(bool useOwnFbo)
{
	if (useOwnFbo)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, mHDRFbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,mHDRColorTex, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,mHDRDepthTex,0);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
}

void HDRHelper::endHDR(bool useOwnFbo)
{
	if (useOwnFbo)
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void
drawQuads(float s = 1.0f)
{
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f,0.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(s,0.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(s,s);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f,s);
	glEnd();
}

void HDRHelper::DoHDR(GLuint oldFBO, bool useDOF) {

	PxMat44 Projection = PerspectiveProjectionMatrix(fov,Width,Height,zNear,zFar);

	// render stored HDR fbo onto blur fbo, first with horizontal blur
	glBindFramebuffer(GL_FRAMEBUFFER, mHDRBlurFbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,mHDRBlurTex,0);
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);

	glViewport(0,0,realWidth,realHeight);	
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(&Projection.column0.x);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mHDRColorTex);

	glUseProgram(mShaderBloomH);
	glUniform1f(glGetUniformLocation(mShaderBloomH, "sx"), 1.0f / (float)realWidth);
	glUniform1i(glGetUniformLocation(mShaderBloomH,"colorTex"),0);
	
	drawQuads();

	glUseProgram(0);
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,0);

	// now apply vertical blur for the bloom
	if (useDOF)
	{	
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,mHDRBloomTex,0);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);
	}
	else
		glBindFramebuffer(GL_FRAMEBUFFER,oldFBO);

	glViewport(0,0,realWidth,realHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(&Projection.column0.x);

	glActiveTexture(GL_TEXTURE0); 	glBindTexture(GL_TEXTURE_2D,mHDRColorTex);
	glActiveTexture(GL_TEXTURE1); 	glBindTexture(GL_TEXTURE_2D,mHDRBlurTex);

	glUseProgram(mShaderBloomV);
	glUniform1f(glGetUniformLocation(mShaderBloomV, "sy"), 1.0f / (float)realHeight);
	glUniform1i(glGetUniformLocation(mShaderBloomV,"colorTex"),0);
	glUniform1i(glGetUniformLocation(mShaderBloomV,"blurTex"),1);
	
	drawQuads();

	glUseProgram(0);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D,0);
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,0);

	// now render the final image onto supplied fbo, apply DOF
	if (!useDOF) return;

	glBindFramebuffer(GL_FRAMEBUFFER,oldFBO);

	glViewport(0,0,realWidth,realHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(&Projection.column0.x);

	glActiveTexture(GL_TEXTURE0); 	glBindTexture(GL_TEXTURE_2D,mHDRBloomTex);
	glActiveTexture(GL_TEXTURE1); 	glBindTexture(GL_TEXTURE_2D,mHDRDepthTex);

	glUseProgram(mShaderDOF);
	glUniform1f(glGetUniformLocation(mShaderDOF, "sx"), 1.0f / (float)realWidth);
	glUniform1f(glGetUniformLocation(mShaderDOF, "sy"), 1.0f / (float)realHeight);
	glUniform1i(glGetUniformLocation(mShaderDOF,"colorTex"),0);
	glUniform1i(glGetUniformLocation(mShaderDOF,"depthTex"),1);
	
	drawQuads();

	glUseProgram(0);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D,0);
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,0);
}
