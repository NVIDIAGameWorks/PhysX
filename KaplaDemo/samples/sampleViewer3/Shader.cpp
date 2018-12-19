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

#include "Shader.h"
#include <assert.h>
//#define NOMINMAX
#include <windows.h>

#include "foundation/PxMat44.h"

// --------------------------------------------------------------------------------------------
Shader::Shader()
{
	mVertexShaderSource = NULL;
	mVertexShaderSourceLength = 0;
	
	mGeometryShaderSource = NULL;
	mGeometryShaderSourceLength = 0;

	mFragmentShaderSource = NULL;
	mFragmentShaderSourceLength = 0;

	mShaderProgram  = 0;
	mVertexShader   = 0;
	mGeometryShader = 0;
	mFragmentShader = 0;

	mErrorMessage = "";
}

// --------------------------------------------------------------------------------------------
bool Shader::loadShaders(const char* vertexShaderFile, const char* fragmentShaderFile)
{
	mErrorMessage = "";

	if (vertexShaderFile == NULL || fragmentShaderFile == NULL)
		return false;

	if (!GLEW_VERSION_2_0)
		return false;

	if (!loadFile(vertexShaderFile, mVertexShaderSource, mVertexShaderSourceLength)) {
		printf("Can't load vertex shader\n");
		return false;
	}

	if (mGeometryShaderSource) {
		free(mGeometryShaderSource);
		mGeometryShaderSource = NULL;
		mGeometryShaderSourceLength = 0;
	}

	if (!loadFile(fragmentShaderFile, mFragmentShaderSource, mFragmentShaderSourceLength)) {
		printf("Can't load fragment shader\n");
		return false;
	}

	if (!compileShaders()) {
		printf("Can't compile shaders\n");
		return false;
	}

	findVariables();

	return true;
}

// --------------------------------------------------------------------------------------------
bool Shader::loadShaders(const char* vertexShaderFile, const char* geometryShaderFile, const char* fragmentShaderFile)
{
	mErrorMessage = "";

	if (vertexShaderFile == NULL  || geometryShaderFile == NULL || fragmentShaderFile == NULL)
		return false;

	if (!GLEW_VERSION_2_0)
		return false;

	if (!loadFile(vertexShaderFile, mVertexShaderSource, mVertexShaderSourceLength))
		return false;

	if (!loadFile(geometryShaderFile, mGeometryShaderSource, mGeometryShaderSourceLength))
		return false;

	if (!loadFile(fragmentShaderFile, mFragmentShaderSource, mFragmentShaderSourceLength))
		return false;

	if (!compileShaders())
		return false;

	findVariables();

	return true;
}




// --------------------------------------------------------------------------------------------
bool Shader::loadShaderCode(const char* vertexShaderCode, const char* fragmentShaderCode)
{
/*
	mErrorMessage = "";

	if (!GLEW_VERSION_2_0)
		return false;

	size_t size;

	if (mVertexShaderSource != NULL)
		free(mVertexShaderSource);

	size = strlen(vertexShaderCode)+1;
	mVertexShaderSourceLength = size-1;
	mVertexShaderSource = (char*)malloc(size * sizeof(char));
	strcpy_s(mVertexShaderSource, size, vertexShaderCode);

	if (mFragmentShaderSource != NULL)
		free(mFragmentShaderSource);

	size = strlen(fragmentShaderCode)+1;
	mFragmentShaderSourceLength = size-1;
	mFragmentShaderSource = (char*)malloc(size * sizeof(char));
	strcpy_s(mFragmentShaderSource, size, fragmentShaderCode);

	if (!compileShaders())
		return false;

	findVariables();

	return true;
	*/
	return loadShaderCode(vertexShaderCode, NULL, fragmentShaderCode);
}

// --------------------------------------------------------------------------------------------
bool Shader::loadShaderCode(const char* vertexShaderCode, const char* geometryShaderCode,  const char* fragmentShaderCode)
{
	mErrorMessage = "";

	if (!GLEW_VERSION_2_0)
		return false;

	size_t size;

	if (mVertexShaderSource != NULL)
		free(mVertexShaderSource);

	size = strlen(vertexShaderCode)+1;
	mVertexShaderSourceLength = size-1;
	mVertexShaderSource = (char*)malloc(size * sizeof(char));
	strcpy_s(mVertexShaderSource, size, vertexShaderCode);

	if (mFragmentShaderSource != NULL)
		free(mFragmentShaderSource);

	if (mGeometryShaderSource != NULL) {
		free(mGeometryShaderSource);
		mGeometryShaderSource = NULL;
	}

	if (geometryShaderCode) {
		size = strlen(geometryShaderCode)+1;
		mGeometryShaderSourceLength = size-1;
		mGeometryShaderSource = (char*)malloc(size * sizeof(char));
		strcpy_s(mGeometryShaderSource, size, geometryShaderCode);

	}

	if (mFragmentShaderSource != NULL)
		free(mFragmentShaderSource);

	size = strlen(fragmentShaderCode)+1;
	mFragmentShaderSourceLength = size-1;
	mFragmentShaderSource = (char*)malloc(size * sizeof(char));
	strcpy_s(mFragmentShaderSource, size, fragmentShaderCode);

	if (!compileShaders())
		return false;

	findVariables();

	return true;
}


// --------------------------------------------------------------------------------------------
Shader::~Shader()
{
	deleteShaders();
}

// --------------------------------------------------------------------------------------------
void Shader::deleteShaders()
{
	glDetachShader(mShaderProgram, mVertexShader);
	glDeleteShader(mVertexShader);
	
	glDetachShader(mShaderProgram, mFragmentShader);
	glDeleteShader(mFragmentShader);


	glDeleteProgram(mShaderProgram);

	mVertexShader = 0;
	mFragmentShader = 0;
	mShaderProgram = 0;

	free(mVertexShaderSource);
	free(mFragmentShaderSource);

	mVertexShaderSource = NULL;
	mFragmentShaderSource = NULL;
}

// --------------------------------------------------------------------------------------------
bool Shader::compileShaders()
{
	if (mShaderProgram != 0) {
		glDetachShader(mShaderProgram, mVertexShader);
		glDeleteShader(mVertexShader);


		glDetachShader(mShaderProgram, mGeometryShader);
		glDeleteShader(mGeometryShader);

		glDetachShader(mShaderProgram, mFragmentShader);
		glDeleteShader(mFragmentShader);

		glDeleteProgram(mShaderProgram);

		mVertexShader = 0;
		mGeometryShader = 0;
		mFragmentShader = 0;
		mShaderProgram = 0;
	}

	bool ok = true;

	mShaderProgram  = glCreateProgram();
	mVertexShader   = glCreateShader(GL_VERTEX_SHADER);
	if (mGeometryShaderSource) {
		mGeometryShader   = glCreateShader(GL_GEOMETRY_SHADER_ARB);
	}
	mFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	mGlShaderAttributes = 0;

	const GLchar* sources[3];
	GLint   lengths[3];

	char header[128];
	sprintf_s(header, 128, "\n");
	sources[0] = header;
	lengths[0] = strlen(header);

	char footer[2] = "\n";
	sources[2] = footer;
	lengths[2] = strlen(footer);

	sources[1] = mVertexShaderSource;
	lengths[1] = mVertexShaderSourceLength;

	glShaderSource(mVertexShader, 3, sources, lengths);
	glCompileShader(mVertexShader);

	sources[1] = mFragmentShaderSource;
	lengths[1] = mFragmentShaderSourceLength;

	glShaderSource(mFragmentShader, 3, sources, lengths);
	glCompileShader(mFragmentShader);


	if (mGeometryShaderSource) {
		sources[1] = mGeometryShaderSource;
		lengths[1] = mGeometryShaderSourceLength;

		glShaderSource(mGeometryShader, 3, sources, lengths);
		glCompileShader(mGeometryShader);

	}

	// error checking
	int param;
	
	glGetShaderiv(mVertexShader, GL_COMPILE_STATUS, &param);
	if (param != GL_TRUE) {
		getCompileError(mVertexShader);
		deleteShaders();
		return false;
	}

	if (mGeometryShader) {
		glGetShaderiv(mGeometryShader, GL_COMPILE_STATUS, &param);
		if (param != GL_TRUE) {
			getCompileError(mGeometryShader);
			deleteShaders();
			return false;
		}
	}

	glGetShaderiv(mFragmentShader, GL_COMPILE_STATUS, &param);
	if (param != GL_TRUE) {
		getCompileError(mFragmentShader);
		deleteShaders();
		return false;
	}
	

	// link
	glAttachShader(mShaderProgram, mVertexShader);
	if (mGeometryShader) {
		GLenum gsInput = GL_POINTS;
		GLenum gsOutput = GL_TRIANGLE_STRIP;
		GLuint maxGSVOut = 4;
		glAttachShader(mShaderProgram, mGeometryShader);

		glProgramParameteriEXT(mShaderProgram, GL_GEOMETRY_INPUT_TYPE_EXT, gsInput);
		glProgramParameteriEXT(mShaderProgram, GL_GEOMETRY_OUTPUT_TYPE_EXT, gsOutput); 
		glProgramParameteriEXT(mShaderProgram, GL_GEOMETRY_VERTICES_OUT_EXT, maxGSVOut); 
	}
	glAttachShader(mShaderProgram, mFragmentShader);


	glLinkProgram(mShaderProgram);
	glGetProgramiv(mShaderProgram, GL_LINK_STATUS, &param);
		
	// check if program linked

	if (!param) {
		char temp[4096];
		glGetProgramInfoLog(mShaderProgram, 4096, 0, temp);
		fprintf(stderr, "Failed to link program:\n%s\n", temp);	
		//printf("..\n");
		getLinkError(mShaderProgram);
		printf(mErrorMessage.c_str());
		return false;
	}

	return true;
}

// --------------------------------------------------------------------------------------------
void Shader::findVariables() 
{
	// Find uniforms
	GLint numUniforms, maxLength;
	mUniforms.clear();
	mAttributes.clear();
	glGetProgramiv(mShaderProgram, GL_ACTIVE_UNIFORMS, &numUniforms);
	glGetProgramiv(mShaderProgram, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);
	assert(maxLength < 256);
	char buf[256];

	for (GLint i = 0; i < numUniforms; i++)
	{
		GLint length;
		UniformVariable u;
		glGetActiveUniform(mShaderProgram, i, 256, &length, &u.size, &u.type, buf);
		assert(length < 256);
		if (strncmp("gl_", buf, 3) != 0)
		{
			u.index = glGetUniformLocation(mShaderProgram, buf);
			mUniforms[buf] = u;
		}
	}

	// Find attribs
	GLint numAttribs;
	glGetProgramiv(mShaderProgram, GL_ACTIVE_ATTRIBUTES, &numAttribs);
	glGetProgramiv(mShaderProgram, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLength);
	assert(maxLength < 256);

	for (GLint i = 0; i < numAttribs; i++)
	{
		GLint length;
		AttributeVariable a;
		glGetActiveAttrib(mShaderProgram, i, 256, &length, &a.size, &a.type, buf);
		if (strncmp("gl_", buf, 3) != 0)
		{
			a.index = glGetAttribLocation(mShaderProgram, buf);
			mAttributes[buf] = a;
		}
		else
		{
			if (strncmp("gl_Vertex", buf, 9) == 0)
				mGlShaderAttributes |= gl_VERTEX;
			else if (strncmp("gl_Normal", buf, 9) == 0)
				mGlShaderAttributes |= gl_NORMAL;
			else if (strncmp("gl_Color", buf, 8) == 0)
				mGlShaderAttributes |= gl_COLOR;
			else if (strncmp("gl_MultiTexCoord0", buf, 17) == 0)
				mGlShaderAttributes |= gl_TEXTURE;
			else if (strncmp("gl_MultiTexCoord1", buf, 17) == 0)
				mGlShaderAttributes |= gl_TEXTURE;
			else if (strncmp("gl_MultiTexCoord2", buf, 17) == 0)
				mGlShaderAttributes |= gl_TEXTURE;
			else if (strncmp("gl_MultiTexCoord3", buf, 17) == 0)
				mGlShaderAttributes |= gl_TEXTURE;
			else if (strncmp("gl_MultiTexCoord4", buf, 17) == 0)
				mGlShaderAttributes |= gl_TEXTURE;
			else if (strncmp("gl_MultiTexCoord5", buf, 17) == 0)
				mGlShaderAttributes |= gl_TEXTURE;
			else
				printf("Unknown gl attribute: %s\n", buf);
		}
	}
}

// --------------------------------------------------------------------------------------------
void Shader::getCompileError(GLuint shader)
{
	int infoLogLength;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (infoLogLength > 1)
	{
		char* log = (char*)malloc(sizeof(char) * infoLogLength);
		int slen;
		glGetShaderInfoLog(shader, infoLogLength, &slen, log);

		const GLubyte* vendor = glGetString(GL_VENDOR);
		const GLubyte* renderer = glGetString(GL_RENDERER);
		const GLubyte* version = glGetString(GL_VERSION);
	
		printf("Compiler error\nVendor: %s\nRenderer: %s\nVersion: %s\nError: %s\n",
				vendor, renderer, version, log);

		mErrorMessage = "compile error: ";
		mErrorMessage += std::string(log);
		free(log);
	}
}

// --------------------------------------------------------------------------------------------
void Shader::getLinkError(GLuint shader)
{
	int infoLogLength;
	glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (infoLogLength > 1)
	{
		char* log = (char*)malloc(sizeof(char) * infoLogLength);
		int slen;
		glGetProgramInfoLog(shader, infoLogLength, &slen, log);

		//const GLubyte* vendor = glGetString(GL_VENDOR);
		//const GLubyte* renderer = glGetString(GL_RENDERER);
		//const GLubyte* version = glGetString(GL_VERSION);

		//printf("Link Error:\nVendor: %s\nRenderer: %s\nVersion: %s\nError:\n%s\n",
		//	vendor, renderer, version, log);

		mErrorMessage = "link error: ";
		mErrorMessage += std::string(log);

		free(log);
	}
}

// --------------------------------------------------------------------------------------------
void Shader::activate(const ShaderMaterial &mat)
{
	if (mShaderProgram == 0)
		return;

	glUseProgram(mShaderProgram);
}


// --------------------------------------------------------------------------------------------
void Shader::activate()
{
	if (mShaderProgram == 0)
		return;

	glUseProgram(mShaderProgram);
}

// --------------------------------------------------------------------------------------------
void Shader::deactivate()
{
	if (mShaderProgram == 0)
		return;

	GLint curProg;
	glGetIntegerv(GL_CURRENT_PROGRAM, &curProg);
	assert(curProg == mShaderProgram);

	//for (tAttributes::iterator it = mAttributes.begin(); it != mAttributes.end(); ++it)
	//{
	//	glDisableVertexAttribArray(it->second.index);
	//}
	//if (mGlShaderAttributes & gl_VERTEX)
	//	glDisableClientState(GL_VERTEX_ARRAY);

	//if (mGlShaderAttributes & gl_NORMAL)
	//	glDisableClientState(GL_NORMAL_ARRAY);

	//if (mGlShaderAttributes & gl_COLOR)
	//	glDisableClientState(GL_COLOR_ARRAY);

	//if (mGlShaderAttributes & gl_TEXTURE)
	//	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glUseProgram(0);
}

// --------------------------------------------------------------------------------------------
bool Shader::isValid()
{
	return mShaderProgram != 0;
}

// --------------------------------------------------------------------------------------------
bool Shader::setUniform(const char *name, const PxMat33& value)
{
	GLint uniform = getUniformCommon(name);
	if (uniform != -1)
	{
		float v[9];
		v[0] = value.column0.x; v[3] = value.column0.y; v[6] = value.column0.z; 
		v[1] = value.column1.x; v[4] = value.column1.y; v[7] = value.column1.z; 
		v[2] = value.column2.x; v[5] = value.column2.y; v[8] = value.column2.z; 
		glUniformMatrix3fv(uniform, 1, false, v);

		return true;
	}
	return false;
}

// --------------------------------------------------------------------------------------------
bool Shader::setUniform(const char *name, const PxTransform& value)
{
	GLint uniform = getUniformCommon(name);
	if (uniform != -1)
	{
		float v[16];
		PxMat44 v44 = value;
		v[0] = v44.column0.x; v[4] = v44.column0.y; v[8]  = v44.column0.z; v[12] = v44.column0.w; 
		v[1] = v44.column1.x; v[5] = v44.column1.y; v[9]  = v44.column1.z; v[13] = v44.column1.w; 
		v[2] = v44.column2.x; v[6] = v44.column2.y; v[10] = v44.column2.z; v[14] = v44.column2.w; 
		v[3] = v44.column3.x; v[7] = v44.column3.y; v[11] = v44.column3.z; v[15] = v44.column3.w; 
		glUniformMatrix4fv(uniform, 1, false, v);

		return true;
	}
	return false;
}

// --------------------------------------------------------------------------------------------
bool Shader::setUniform(const char *name, PxU32 size, const PxVec3* value)
{
	GLint uniform = getUniformCommon(name);
	if (uniform != -1)
	{
		glUniform3fv(uniform, size, (const GLfloat*)value);
		return true;
	}
	return false;
}

// --------------------------------------------------------------------------------------------
bool Shader::setUniform1(const char* name, const float value)
{
    GLint loc = glGetUniformLocation(mShaderProgram, name);
	if (loc >= 0) {
        glUniform1f(loc, value);
		return true;
	}
	return false;
}

// --------------------------------------------------------------------------------------------
bool Shader::setUniform2(const char* name, float val0, float val1)
{
    GLint loc = glGetUniformLocation(mShaderProgram, name);
	if (loc >= 0) {
        glUniform2f(loc, val0, val1);
		return true;
	}
	return false;
}

// --------------------------------------------------------------------------------------------
bool Shader::setUniform3(const char* name, float val0, float val1, float val2)
{
    GLint loc = glGetUniformLocation(mShaderProgram, name);
	if (loc >= 0) {
        glUniform3f(loc, val0, val1, val2);
		return true;
	}
	return false;
}

// --------------------------------------------------------------------------------------------
bool Shader::setUniform4(const char* name, float val0, float val1, float val2, float val3)
{
    GLint loc = glGetUniformLocation(mShaderProgram, name);
	if (loc >= 0) {
        glUniform4f(loc, val0, val1, val2, val3);
		return true;
	}
	return false;
}


// --------------------------------------------------------------------------------------------
bool Shader::setUniform(const char* name, float value)
{
	GLint uniform = getUniformCommon(name);
	if (uniform != -1)
	{
		glUniform1f(uniform, value);
		return true;
	}
	return false;
}

// --------------------------------------------------------------------------------------------
bool Shader::setUniform(const char* name, int value)
{
	GLint uniform = getUniformCommon(name);
	if (uniform != -1)
	{
		glUniform1i(uniform, value);
		return true;
	}
	return false;
}


bool
Shader::setUniformfv(const GLchar *name, GLfloat *v, int elementSize, int count)
{
	GLint loc = glGetUniformLocation(mShaderProgram, name);//getUniformCommon( name);
	if (loc == -1) {
		return false;
	}
	switch (elementSize) {
		case 1:
			glUniform1fv(loc, count, v);
			break;
		case 2:
			glUniform2fv(loc, count, v);
			break;
		case 3:
			glUniform3fv(loc, count, v);
			break;
		case 4:
			glUniform4fv(loc, count, v);
			break;
	}
	return true;
}

// --------------------------------------------------------------------------------------------
bool Shader::setAttribute(const char* name, PxU32 size, PxU32 stride, GLenum type, void* data)
{
	GLint activeProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &activeProgram);
	tAttributes::iterator it = mAttributes.find(name);
	if (it != mAttributes.end())
	{
		PxI32 index = it->second.index;
		glEnableVertexAttribArray(index);
		glVertexAttribPointer(index, size, type, GL_FALSE, stride, data);

		return true;
	}
	return false;
}

// --------------------------------------------------------------------------------------------
GLint Shader::getUniformCommon(const char* name)
{
	GLint activeProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &activeProgram);
	if (activeProgram == 0) return -1;
	tUniforms::iterator it = mUniforms.find(name);
	if (it != mUniforms.end())
	{
		assert(it->second.index != -1);
		return it->second.index;
	}
	return -1;
}

// --------------------------------------------------------------------------------------------
bool Shader::loadFile(const char* filename, char*& destination, int& destinationLength)
{
	if (destination != NULL)
	{
		assert(destinationLength != 0);
		free(destination);
		destination = NULL;
		destinationLength = 0;
	}
	assert(destinationLength == 0);

	FILE* file = NULL;
	file = fopen(filename, "rb");
	if (file == NULL)
	{
		mErrorMessage = "Shader file " + std::string(filename) + "not found\n";
		return false;
	}
	// find length
	fseek(file, 0, SEEK_END);
	destinationLength = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (destinationLength > 0)
	{
		destination = (char*)malloc(destinationLength * sizeof(char) + 1);

		fread(destination, 1, destinationLength, file);
		destination[destinationLength] = 0;
	}
	fclose(file);
	return true;
}

void
Shader::bindTexture(const char *name, GLuint tex, GLenum target, GLint unit)
{
	GLint loc = getUniformCommon(name);
	if (loc >= 0) {
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(target, tex);
		glUseProgram(mShaderProgram);
		glUniform1i(loc, unit);
		glActiveTexture(GL_TEXTURE0);
	} else {
#if _DEBUG
		fprintf(stderr, "Error binding texture '%s'\n", name);
#endif
	}
}

bool
Shader::setUniformMatrix4fv(const GLchar *name, const GLfloat *m, bool transpose)
{
	GLint loc = getUniformCommon(name);
	if (loc >= 0) {
		glUniformMatrix4fv(loc, 1, transpose, m);
	} else {
		return false;
	}

	return true;
}


