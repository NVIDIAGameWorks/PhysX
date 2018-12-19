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

#include "FXAAHelper.h"
#include "foundation/PxMat44.h"
const char *computeLumaVS = STRINGIFY(
    void main(void)
	{
		gl_TexCoord[0] = gl_MultiTexCoord0;
		gl_Position = gl_Vertex * 2.0 - 1.0;
	}
);

const char *computeLumaFS = STRINGIFY(
	uniform sampler2D imgTex;

	void main (void)
	{
		vec4 col = texture2D(imgTex,gl_TexCoord[0].xy);
		gl_FragColor = vec4(col.xyz, sqrt(dot(col.rgb, vec3(0.299, 0.587, 0.114))));
	}
);

FXAAHelper::FXAAHelper( const char* resourcePath) {

	char fxaaVSF[5000];
	char fxaaFSF[5000];
	sprintf(fxaaVSF, "%s\\fxaa.vs", resourcePath);
	sprintf(fxaaFSF, "%s\\fxaa.fs", resourcePath);
	computeLuma.loadShaderCode(computeLumaVS,computeLumaFS);
	fxaa.loadShaders(fxaaVSF,fxaaFSF);


	glUseProgram(computeLuma);
	glUniform1i(glGetUniformLocation(computeLuma,"imgTex"),0);
	glUseProgram(0);

	glUseProgram(fxaa);
	glUniform1i(glGetUniformLocation(fxaa,"imgWithLumaTex"),0);
	glUseProgram(0);

	glGenTextures(1,&imgTex);
	glGenTextures(1,&imgWithLumaTex);
	glGenTextures(1,&depthTex); 
	glGenFramebuffers(1,&FBO);
}

void FXAAHelper::Resize(int w,int h) {

	Width = w;
	Height = h;
	
	glViewport(0,0,Width,Height);

	glBindTexture(GL_TEXTURE_2D,imgTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,Width,Height,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);

	glBindTexture(GL_TEXTURE_2D,imgWithLumaTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,Width,Height,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);

	glBindTexture(GL_TEXTURE_2D,depthTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT32,Width,Height,0,GL_DEPTH_COMPONENT,GL_FLOAT,NULL);

	
	glUseProgram(fxaa);
	glUniform2f(glGetUniformLocation(fxaa,"rcpFrame"), 1.0f / ((float)Width), 1.0f / ((float)Height));
	
	glUseProgram(0);

}
void FXAAHelper::Destroy() {

	computeLuma.deleteShaders();
	fxaa.deleteShaders();
	
	glDeleteTextures(1,&imgTex);
	glDeleteTextures(1,&imgWithLumaTex);
	glDeleteTextures(1,&depthTex);
	glDeleteFramebuffers(1,&FBO);
}

void FXAAHelper::StartFXAA() {
	glBindFramebuffer(GL_FRAMEBUFFER,FBO);

	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,imgTex,0);
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,depthTex,0);

	glViewport(0,0,Width,Height);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void FXAAHelper::EndFXAA(GLuint oldFBO) {


	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
//	if(Blur)
//	{
	glBindFramebuffer(GL_FRAMEBUFFER,FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,imgWithLumaTex,0);
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);

	glClearColor(0.0f,0.0f,0.0f,0.0f);  
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,imgTex);

	glUseProgram(computeLuma);
		glBegin(GL_QUADS);

			glTexCoord2f(0.0f,0.0f); glVertex2f(0.0f,0.0f);
			glTexCoord2f(1.0f,0.0f); glVertex2f(1.0f,0.0f);
			glTexCoord2f(1.0f,1.0f); glVertex2f(1.0f,1.0f);
			glTexCoord2f(0.0f,1.0f); glVertex2f(0.0f,1.0f);
		glEnd();

	glUseProgram(0);


	glBindFramebuffer(GL_FRAMEBUFFER,oldFBO);
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,imgWithLumaTex);
	glUseProgram(fxaa);

	glBegin(GL_QUADS);
		glTexCoord2f(0.0f,0.0f); glVertex2f(0.0f,0.0f);
		glTexCoord2f(1.0f,0.0f); glVertex2f(1.0f,0.0f);
		glTexCoord2f(1.0f,1.0f); glVertex2f(1.0f,1.0f);
		glTexCoord2f(0.0f,1.0f); glVertex2f(0.0f,1.0f);
	glEnd();

	glUseProgram(0);

	glBindTexture(GL_TEXTURE_2D,0);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

}