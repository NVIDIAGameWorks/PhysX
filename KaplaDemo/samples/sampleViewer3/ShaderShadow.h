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

#ifndef SHADER_SHADOW_DIFFUSE_H
#define SHADER_SHADOW_DIFFUSE_H

//#define USE_OPTIX 1
#include "Shader.h"
#include <string>
class ShadowMap;

class ShaderShadow;
class ShaderShadow : public Shader
{
public:
	// RENDER_DEPTH is for shadow map computation
	// RENDER_COLOR is for the usual rendering
	// RENDER_DEPTH_NORMAL is for SSAO
	enum RenderMode{RENDER_DEPTH, RENDER_COLOR, RENDER_DEPTH_NORMAL};
	static RenderMode renderMode;
	static float hdrScale;
	static GLuint skyBoxTex;

	enum VS_MODE{VS_DEFAULT, VS_4BONES, VS_8BONES, VS_INSTANCE, VS_TEXINSTANCE, VS_DEFAULT_FOR_3D_TEX, VS_FILE,VS_CROSSHAIR};
	enum PS_MODE{PS_WHITE, PS_SHADE, PS_SHADE3D, PS_FILE, PS_NORMAL, PS_CROSSHAIR};
	enum SHADER3D_CHOICES{SAND_STONE, WHITE_MARBLE, GRAYVEIN_MARBLE, BLUEVEIN_MARBLE, GREEN_MARBLE, GRAY_MARBLE, RED_GRANITE, YELLOW_GRANITE, WOOD, COMBINE};

	ShaderShadow(VS_MODE vsMode = VS_DEFAULT, PS_MODE psMode = PS_SHADE, SHADER3D_CHOICES shade3D = GRAY_MARBLE);
	~ShaderShadow() {};

	bool init();

	void setSpotLight(int nr, const PxVec3 &pos, PxVec3 &dir, float decayBegin = 0.98f, float decayEnd = 0.997f);
	void setBackLightDir(const PxVec3 &dir) { mBackLightDir = dir; }

	void updateCamera(float* modelView, float* proj);

	void setShadowMap(int nr, ShadowMap *shadowMap) { mShadowMaps[nr] = shadowMap; }
	void setNumShadows(int num) { mNumShadows = num; }
	void setShowReflection(bool show) { mShowReflection = show; }
	bool getShowReflection() const {return mShowReflection;}

	void setReflectionTexId(unsigned int texId) { mReflectionTexId = texId; }

	virtual void activate(const ShaderMaterial &mat);
	virtual void deactivate();

	void setShadowAmbient(float sa);
	
	// For fast shader debugging
	bool forceLoadShaderFromFile(VS_MODE vsm, PS_MODE psm, const char* vsName = NULL, const char* psName = NULL);

	SHADER3D_CHOICES getMyShade3DChoice() {return myShade3DChoice;}

	PxVec3 getDustColor() { return  dustColor; }

	virtual bool setUniform(const char* name, const PxMat33& value);
	virtual bool setUniform(const char* name, const PxTransform& value);
	virtual bool setUniform(const char *name, PxU32 size, const PxVec3* value);

	virtual bool setUniform1(const char* name, float val);
	virtual bool setUniform2(const char* name, float val0, float val1);
	virtual bool setUniform3(const char* name, float val0, float val1, float val2);
	virtual bool setUniform4(const char* name, float val0, float val1, float val2, float val3);
	virtual bool setUniformfv(const GLchar *name, GLfloat *v, int elementSize, int count=1);

	virtual bool setUniform(const char* name, float value);
	virtual bool setUniform(const char* name, int value);
	virtual bool setUniformMatrix4fv(const GLchar *name, const GLfloat *m, bool transpose);

	virtual GLint getUniformCommon(const char* name);

	virtual	bool setAttribute(const char* name, PxU32 size, PxU32 stride, GLenum type, void* data);

private:
	static const int mNumLights = 1;

	ShadowMap *mShadowMaps[mNumLights];

	PxVec3 mSpotLightPos[mNumLights];
	PxVec3 mSpotLightDir[mNumLights];

	float mSpotLightCosineDecayBegin[mNumLights];
	float mSpotLightCosineDecayEnd[mNumLights];

	GLuint mReflectionTexId;

	PxVec3 mBackLightDir;
	int mNumShadows;
	bool mShowReflection;

	float mCamModelView[16];
	float mCamProj[16];
	float shadowAmbient;
	PxVec3 ambientColor;
	VS_MODE vsMode;
	PS_MODE psMode;

	std::string fragmentShader3DComposite;
	SHADER3D_CHOICES myShade3DChoice;
	PxVec3 dustColor;

	// For shadow map and SSAO
	ShaderShadow* whiteShader;
	ShaderShadow* normalShader;
};

#endif