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

#ifndef SHADER_H
#define SHADER_H

#include "foundation/PxVec3.h"
#include "foundation/PxMat33.h"
#include "foundation/PxTransform.h"

#include <GL/glew.h>
#include <map>
#include <vector>
#include <string>

using namespace physx;

#define STRINGIFY(A) #A

// ----------------------------------------------------------------------------------
struct ShaderMaterial 
{
	void init(unsigned int texId = 0) {
		this->texId = texId;
		ambientCoeff = 0.0f;
		diffuseCoeff = 1.0f;
		specularCoeff = 0.0f;
		reflectionCoeff = 0.0f;
		refractionCoeff = 0.0f;
		shadowCoeff = 0.0f;
		color[0] = 1.0f; color[1] = 1.0f; color[2] = 1.0f; color[3] = 1.0f;
	}
	void setColor(float r, float g, float b, float a = 1.0f) {
		color[0] = r; color[1] = g; color[2] = b; color[3] = a;
	}
	bool operator == (const ShaderMaterial &m) const {
		if (texId != m.texId) return false;
		if (color[0] != m.color[0] || color[1] != m.color[1] || color[2] != m.color[2] || color[3] != m.color[3]) return false;
		return true;
	}
	unsigned int texId;
	float color[4];
	float ambientCoeff;
	float diffuseCoeff;
	float specularCoeff;
	float reflectionCoeff;
	float refractionCoeff;
	float shadowCoeff;

};

// ----------------------------------------------------------------------------------
class Shader
{
public:
	Shader();
	virtual ~Shader();

	bool isValid();
	virtual void activate();
	virtual void activate(const ShaderMaterial &mat);
	virtual void deactivate();
	operator GLuint ()
	{
		return mShaderProgram;
	}

	const std::string &getErrorMessage() { return mErrorMessage; }
	GLuint getShaderProgram() {return mShaderProgram;}

//protected:
	bool loadShaders(const char* vertexShaderFile, const char* fragmentShaderFile);
	bool loadShaders(const char* vertexShaderFile, const char* geometryShaderFile, const char* fragmentShaderFile);
	bool loadShaderCode(const char* vertexShaderCode, const char* fragmentShaderCode);
	bool loadShaderCode(const char* vertexShaderCode, const char* geometryShaderCode,  const char* fragmentShaderCode);
	void deleteShaders();

	bool compileShaders();
	void getCompileError(GLuint shader);
	void getLinkError(GLuint shader);
	void findVariables();
	void bindTexture(const char *name, GLuint tex, GLenum target, GLint unit);

	GLuint mShaderProgram;
	GLuint mVertexShader;
	GLuint mGeometryShader;
	GLuint mFragmentShader;
	GLuint mGlShaderAttributes;

	char* mVertexShaderSource;
	int   mVertexShaderSourceLength;

	char* mGeometryShaderSource;
	int   mGeometryShaderSourceLength;

	char* mFragmentShaderSource;
	int   mFragmentShaderSourceLength;

	struct UniformVariable
	{
		GLint size;
		GLenum type;
		GLint index;
	};

	struct AttributeVariable
	{
		GLint size;
		GLenum type;
		GLint index;
	};

		enum glShaderAttribute
	{
		gl_VERTEX	= (1 << 0),
		gl_NORMAL	= (1 << 1),
		gl_COLOR	= (1 << 2),
		gl_TEXTURE	= (1 << 3),
	};

	typedef std::map<std::string, UniformVariable> tUniforms;
	tUniforms mUniforms;
	typedef std::map<std::string, AttributeVariable> tAttributes;
	tAttributes mAttributes;

	std::string mErrorMessage;

public:
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
	bool loadFile(const char* filename, char*& destination, int& destinationLength);
};

#endif