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

#include "SSAOHelper.h"
#include "foundation/PxMat44.h"
const char *ssaoFilterVS = STRINGIFY(
    void main(void)
	{
		gl_TexCoord[0] = gl_MultiTexCoord0;
		gl_Position = gl_Vertex * 2.0 - 1.0;
	}
);

const char *ssaoFilterHFS = STRINGIFY(
	uniform sampler2D ssaoTex;
	uniform float sx;

	void main (void)
	{
		float SSAO = 0.0;

		for(int x = -2; x <= 2; x++)
		{
			SSAO += texture2D(ssaoTex,vec2(x * sx + gl_TexCoord[0].s,gl_TexCoord[0].t)).r * (3.0 - abs(float(x)));
		}

		gl_FragColor = vec4(vec3(SSAO / 9.0),1.0);
		gl_FragColor.w = gl_FragColor.x;
	}
);

const char *ssaoFilterVFS = STRINGIFY(
	uniform sampler2D ssaoTex;
	uniform float sy;

	void main (void)
	{
		
		float SSAO = 0.0;

		for(int y = -2; y <= 2; y++)
		{
			SSAO += texture2D(ssaoTex,vec2(gl_TexCoord[0].s,y * sy + gl_TexCoord[0].t)).r * (3.0 - abs(float(y)));
		}

		gl_FragColor = vec4(vec3(pow((SSAO / 9.0),1.5)),1.0);
		gl_FragColor.w = gl_FragColor.x;
		//gl_FragColor = vec4(1,1,1,1);
	}
);
SSAOHelper::SSAOHelper(float fov,float padding,float zNear,float zFar, const char* resourcePath, float scale) : fov(fov),padding(padding),zNear(zNear),zFar(zFar), scale(scale) {

	char ssaoVSF[5000];
	char ssaoFSF[5000];
	sprintf(ssaoVSF, "%s\\ssao.vs", resourcePath);
	sprintf(ssaoFSF, "%s\\ssao.fs", resourcePath);
	SSAOFilterH.loadShaderCode(ssaoFilterVS,ssaoFilterHFS);
	SSAOFilterV.loadShaderCode(ssaoFilterVS,ssaoFilterVFS);
	SSAO.loadShaders(ssaoVSF,ssaoFSF);

	glUseProgram(SSAO);
	glUniform1i(glGetUniformLocation(SSAO,"depthTex"),0);
	glUniform1i(glGetUniformLocation(SSAO,"normalTex"),1);
	glUniform1i(glGetUniformLocation(SSAO,"unitVecTex"),2);
	glUseProgram(0);
//	srand(GetTickCount());

	glGenTextures(1,&unitVecTex);

	PxVec3 *RandomUnitVectorsTextureData = new PxVec3[64 * 64];

	for(int i = 0; i < 64 * 64; i++)
	{
		RandomUnitVectorsTextureData[i].x = (float)rand() / (float)RAND_MAX * 2.0f - 1.0f;
		RandomUnitVectorsTextureData[i].y = (float)rand() / (float)RAND_MAX * 2.0f - 1.0f;
		RandomUnitVectorsTextureData[i].z = (float)rand() / (float)RAND_MAX * 2.0f - 1.0f;


		RandomUnitVectorsTextureData[i].normalize();

		RandomUnitVectorsTextureData[i] = RandomUnitVectorsTextureData[i] * 0.5f + PxVec3(0.5f,0.5f,0.5f);
	}

	glBindTexture(GL_TEXTURE_2D,unitVecTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,64,64,0,GL_RGB,GL_FLOAT,RandomUnitVectorsTextureData);

	delete [] RandomUnitVectorsTextureData;	

	
	PxVec3 *samples = new PxVec3[NUMSAMPLES];
	
	float alfa = physx::PxPi / SAMPLEDIV,beta = physx::PxPiDivTwo / (SAMPLEDIV * 2.0f),radius = 0.5f;

	for(int i = 0; i < NUMSAMPLES; i++)
	{
		samples[i] = PxVec3(cos(alfa),sin(alfa),sin(beta));
		samples[i].normalize();
		samples[i] *= ((float)NUMSAMPLES - i) / ((float)NUMSAMPLES);
		samples[i] *= radius;

		alfa += physx::PxPi / (SAMPLEDIV / 2);

		if(((i + 1) % SAMPLEDIV) == 0)
		{
			alfa += physx::PxPi / SAMPLEDIV;
			beta += physx::PxPiDivTwo / SAMPLEDIV;
		}
	}

	glUseProgram(SSAO);
	glUniform3fv(glGetUniformLocation(SSAO,"samples"),NUMSAMPLES,(GLfloat*)samples);
	glUseProgram(0);

	delete [] samples;

	glGenTextures(1,&normalTex);
	glGenTextures(1,&depthTex);
	glGenTextures(1,&ssaoTex);
	glGenTextures(1,&blurTex);

	glGenFramebuffers(1,&FBO);
}

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

PxMat44 PerspectiveProjectionMatrixInverse(PxMat44 &PPM)
{
	float PPMI[16];

	PPMI[0] = 1.0f / PPM.column0.x;
	PPMI[1] = 0.0f;
	PPMI[2] = 0.0f;
	PPMI[3] = 0.0f;

	PPMI[4] = 0.0f;
	PPMI[5] = 1.0f / PPM.column1.y;
	PPMI[6] = 0.0f;
	PPMI[7] = 0.0f;

	PPMI[8] = 0.0f;
	PPMI[9] = 0.0f;
	PPMI[10] = 0.0f;
	PPMI[11] = 1.0f / PPM.column3.z;

	PPMI[12] = 0.0f;
	PPMI[13] = 0.0f;
	PPMI[14] = 1.0f / PPM.column2.w;
	PPMI[15] = - PPM.column2.z / (PPM.column2.w * PPM.column3.z);

	return PxMat44(PPMI);
}
PxMat44 BiasMatrix()
{
	float BM[16];

	BM[0] = 0.5f; BM[4] = 0.0f; BM[8] = 0.0f; BM[12] = 0.5f;
	BM[1] = 0.0f; BM[5] = 0.5f; BM[9] = 0.0f; BM[13] = 0.5f;
	BM[2] = 0.0f; BM[6] = 0.0f; BM[10] = 0.5f; BM[14] = 0.5f;
	BM[3] = 0.0f; BM[7] = 0.0f; BM[11] = 0.0f; BM[15] = 1.0f;

	return PxMat44(BM);
}

PxMat44 BiasMatrixInverse()
{
	float BMI[16];

	BMI[0] = 2.0f; BMI[4] = 0.0f; BMI[8] = 0.0f; BMI[12] = -1.0f;
	BMI[1] = 0.0f; BMI[5] = 2.0f; BMI[9] = 0.0f; BMI[13] = -1.0f;
	BMI[2] = 0.0f; BMI[6] = 0.0f; BMI[10] = 2.0f; BMI[14] = -1.0f;
	BMI[3] = 0.0f; BMI[7] = 0.0f; BMI[11] = 0.0f; BMI[15] = 1.0f;

	return PxMat44(BMI);
}
void SSAOHelper::Resize(int w,int h) {
	realWidth = w;
	realHeight = h;
	Width = w*scale;
	Height = h*scale;
	
	fovPad = 2.0f*atan(tan(fov*0.5f*physx::PxPi/180.0f)*(1.0f+padding))*180.0f/physx::PxPi;
	
	SWidth = Width*(1.0f+padding);
	SHeight = Height*(1.0f+padding);

	glViewport(0,0,SWidth,SHeight);

	PxMat44 mat();
	PxMat44 Projection = PerspectiveProjectionMatrix(fovPad,SWidth,SHeight,zNear,zFar);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(&Projection.column0.x);

	glBindTexture(GL_TEXTURE_2D,normalTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,SWidth,SHeight,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);

	glBindTexture(GL_TEXTURE_2D,depthTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT32,SWidth,SHeight,0,GL_DEPTH_COMPONENT,GL_FLOAT,NULL);

	glBindTexture(GL_TEXTURE_2D,ssaoTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,SWidth,SHeight,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);

	glBindTexture(GL_TEXTURE_2D,blurTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,SWidth,SHeight,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);

	PxMat44 biasProjMat = BiasMatrix() * Projection;
	PxMat44 biasProjMatInv = PerspectiveProjectionMatrixInverse(Projection) * BiasMatrixInverse();

	glUseProgram(SSAO);
	glUniformMatrix4fv(glGetUniformLocation(SSAO,"biasProjMat"),1,GL_FALSE,&biasProjMat.column0.x);
	glUniformMatrix4fv(glGetUniformLocation(SSAO,"biasProjMatInv"),1,GL_FALSE,&biasProjMatInv.column0.x);
	glUniform2f(glGetUniformLocation(SSAO,"scaleXY"),(float)SWidth / 64.0f,(float)SHeight / 64.0f);

	glUseProgram(SSAOFilterH);
	glUniform1f(glGetUniformLocation(SSAOFilterH,"sx"),1.0f / (float)SWidth);

	glUseProgram(SSAOFilterV);
	glUniform1f(glGetUniformLocation(SSAOFilterV,"sy"),1.0f / (float)SHeight);

	glUseProgram(0);

}
void SSAOHelper::Destroy() {

	SSAO.deleteShaders();
	SSAOFilterH.deleteShaders();
	SSAOFilterV.deleteShaders();

	glDeleteTextures(1,&unitVecTex);
	glDeleteTextures(1,&normalTex);
	glDeleteTextures(1,&depthTex);
	glDeleteTextures(1,&ssaoTex);
	glDeleteTextures(1,&blurTex);
	glDeleteFramebuffers(1,&FBO);
}

void SSAOHelper::DoSSAO(void (*renderScene)(), GLuint oldFBO) {
	glBindFramebuffer(GL_FRAMEBUFFER,FBO);

	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,normalTex,0);
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,depthTex,0);

	glViewport(0,0,SWidth,SHeight);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	PxMat44 Projection = PerspectiveProjectionMatrix(fovPad,SWidth,SHeight,zNear,zFar);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(&Projection.column0.x);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	renderScene();
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
//	if(Blur)
//	{
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,ssaoTex,0);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);

		glClearColor(0.0f,0.0f,0.0f,1.0f);  
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//	}
//	else
//	{
//		glBindFramebuffer(GL_FRAMEBUFFER,0);
//	}

	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,depthTex);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D,normalTex);
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D,unitVecTex);

	glUseProgram(SSAO);

	glBegin(GL_QUADS);
		glVertex2f(0.0f,0.0f);
		glVertex2f(1.0f,0.0f);
		glVertex2f(1.0f,1.0f);
		glVertex2f(0.0f,1.0f);
	glEnd();

	glUseProgram(0);

	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D,0);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D,0);
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,0);

//	if(Blur)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,blurTex,0);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);

		glBindTexture(GL_TEXTURE_2D,ssaoTex);

		glUseProgram(SSAOFilterH);
		glUniform1f(glGetUniformLocation(SSAOFilterH, "sx"), 1.0f / (float)SWidth);



		glBegin(GL_QUADS);
/*
			glTexCoord2f(0.0f+fx,0.0f+fy); glVertex2f(0.0f,0.0f);
			glTexCoord2f(1.0f-fx,0.0f+fy); glVertex2f(1.0f,0.0f);
			glTexCoord2f(1.0f-fx,1.0f-fy); glVertex2f(1.0f,1.0f);
			glTexCoord2f(0.0f+fx,1.0f-fy); glVertex2f(0.0f,1.0f);
*/
			glTexCoord2f(0.0f,0.0f); glVertex2f(0.0f,0.0f);
			glTexCoord2f(1.0f,0.0f); glVertex2f(1.0f,0.0f);
			glTexCoord2f(1.0f,1.0f); glVertex2f(1.0f,1.0f);
			glTexCoord2f(0.0f,1.0f); glVertex2f(0.0f,1.0f);
		glEnd();

		glUseProgram(0);

		glBindFramebuffer(GL_FRAMEBUFFER,oldFBO);
		//glClearColor(0.0f,0.0f,0.0f,1.0f);  
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ZERO, GL_SRC_ALPHA);

		glViewport(0,0,realWidth,realHeight);

		Projection = PerspectiveProjectionMatrix(fov,Width,Height,zNear,zFar);

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(&Projection.column0.x);


		glBindTexture(GL_TEXTURE_2D,blurTex);

		glUseProgram(SSAOFilterV);
		glUniform1f(glGetUniformLocation(SSAOFilterV, "sy"), 1.0f / (float)SHeight);

		//float fx = 0.5f*(1.0f-(((float)(SWidth-Width))/((float)Width)));
		//float fy = 0.5f*(1.0f-(((float)(SHeight-Height))/((float)Height)));
		float fx = 0.5f*((float)(SWidth-Width)) / ((float)SWidth);
		float fy = 0.5f*((float)(SHeight-Height)) / ((float)SHeight);

		glBegin(GL_QUADS);
			glTexCoord2f(0.0f+fx,0.0f+fy); glVertex2f(0.0f,0.0f);
			glTexCoord2f(1.0f-fx,0.0f+fy); glVertex2f(1.0f,0.0f);
			glTexCoord2f(1.0f-fx,1.0f-fy); glVertex2f(1.0f,1.0f);
			glTexCoord2f(0.0f+fx,1.0f-fy); glVertex2f(0.0f,1.0f);
		glEnd();

		glUseProgram(0);

		glBindTexture(GL_TEXTURE_2D,0);
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
	}
}