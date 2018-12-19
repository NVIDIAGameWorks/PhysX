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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.


#include "GLES2RendererMaterial.h"

#if defined(RENDERER_ENABLE_GLES2)

#include <RendererMaterialDesc.h>
#include <RendererMemoryMacros.h>

#include "GLES2RendererTexture2D.h"

#include <stdio.h>

#include "mojoshader.h"

namespace SampleRenderer
{

class GLESVariable : public RendererMaterial::Variable
{
public:
    GLESVariable(const char *name, RendererMaterial::VariableType type, PxU32 offset)
    : Variable(name, type, offset)
    {		
    }

	virtual ~GLESVariable(void) {}

    GLint 					m_location[RendererMaterial::NUM_PASSES];
    GLint 					m_sampler[RendererMaterial::NUM_PASSES];
};

const std::string vertexUniformArrayName 	= "vs_uniforms_vec4";
const std::string pixelUniformArrayName 	= "ps_uniforms_vec4";

std::string load_file(const char* file)
{
    LOG_INFO("GLES2RendererMaterial", "Opening: \"%s\"", file);
    char file2[512];
    sprintf(file2, "/sdcard/media/SampleRenderer/4/shaders/%s", file);
    FILE *f = fopen(file2, "rb");
    assert(f);

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buffer = new char[size+1];
    fread(buffer, 1, size, f);
    fclose(f);
    buffer[size] = '\0';

	std::string result(buffer);
	delete[] buffer;
    return result;
}


static int glsl_log(GLuint obj, GLenum check_compile)
{
    if (check_compile == GL_COMPILE_STATUS)
    {
        int len = 0;
        glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &len);
        if(len > 0)
        {
            char *str = (char *) malloc(len * sizeof(char));
            if (str)
            {
                glGetShaderInfoLog(obj, len, NULL, str);
                LOG_INFO("nv_shader",  "shader_debug: %s\n", str);
                free(str);
            }
        }
    }
    else
    {
        int len = 0;
        glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &len);
        if(len > 0)
        {
            char *str = (char *)malloc(len * sizeof(char));
            if (str)
            {
                glGetProgramInfoLog(obj, len, NULL, str);
                LOG_INFO("nv_shader",  "shader_debug: %s\n", str);
                free(str);
            }
        }
    }
    return 0;
}

GLES2RendererMaterial::shaderProgram::shaderProgram() :
														modelMatrixUniform(-1),
														viewMatrixUniform(-1),
														projMatrixUniform(-1),
														modelViewMatrixUniform(-1),
														boneMatricesUniform(-1),
														modelViewProjMatrixUniform(-1),
														positionAttr(-1),
														colorAttr(-1),
														normalAttr(-1),
														tangentAttr(-1),
														boneIndexAttr(-1),
														boneWeightAttr(-1),
														vsUniformsTotal(0),
														psUniformsTotal(0),
														vsUniformsVec4(NULL),
														vsUniformsVec4Location(-1),
														vsUniformsVec4Size(0),
														psUniformsVec4(NULL),
														psUniformsVec4Location(-1),
														psUniformsVec4Size(0),
														vertexMojoResult(NULL),
														fragmentMojoResult(NULL)
{
}

GLES2RendererMaterial::shaderProgram::~shaderProgram()
{
	for(int i = 0; i < NUM_PASSES; ++i)
	    glDeleteProgram(program);
	DELETESINGLE(vertexMojoResult);
	DELETESINGLE(fragmentMojoResult);
	DELETEARRAY(vsUniformsVec4);
	DELETEARRAY(psUniformsVec4);
}


GLES2RendererMaterial::GLES2RendererMaterial(GLES2Renderer &renderer, const RendererMaterialDesc &desc) :
	RendererMaterial(desc, false),
	m_renderer(renderer)
{
	m_glAlphaTestFunc = GL_ALWAYS;
	
	AlphaTestFunc alphaTestFunc = getAlphaTestFunc();
	switch(alphaTestFunc)
	{
		case ALPHA_TEST_ALWAYS:        m_glAlphaTestFunc = GL_ALWAYS;   break;
		case ALPHA_TEST_EQUAL:         m_glAlphaTestFunc = GL_EQUAL;    break;
		case ALPHA_TEST_NOT_EQUAL:     m_glAlphaTestFunc = GL_NOTEQUAL; break;
		case ALPHA_TEST_LESS:          m_glAlphaTestFunc = GL_LESS;     break;
		case ALPHA_TEST_LESS_EQUAL:    m_glAlphaTestFunc = GL_LEQUAL;   break;
		case ALPHA_TEST_GREATER:       m_glAlphaTestFunc = GL_GREATER;  break;
		case ALPHA_TEST_GREATER_EQUAL: m_glAlphaTestFunc = GL_GEQUAL;   break;
		default:
			RENDERER_ASSERT(0, "Unknown Alpha Test Func.");
	}
	
	std::string vertexShaderPath(desc.vertexShaderPath);
	std::string fragmentShaderPath(desc.fragmentShaderPath);
	std::string vertexSource, fragmentSource;
	
	for(int i = 0; i < NUM_PASSES; ++i)
	{
		if(vertexShaderPath.substr(vertexShaderPath.size() - 3, 3) == ".cg")
		{
			m_program[i].vertexMojoResult = static_cast<mojoResult*>(SampleFramework::SamplePlatform::platform()->
													compileProgram(NULL, m_renderer.getAssetDir(), desc.vertexShaderPath, 0, getPassName(static_cast<Pass>(i)), "vmain", NULL));
			vertexSource = m_program[i].vertexMojoResult->parseResult.source;
			LOGI("Vertex source is: \n %s \n", vertexSource.c_str());
		}
		else 
		{
			vertexSource = load_file(desc.vertexShaderPath);
		}
		if(fragmentShaderPath.substr(fragmentShaderPath.size() - 3, 3) == ".cg")
		{
			m_program[i].fragmentMojoResult = static_cast<mojoResult*>(SampleFramework::SamplePlatform::platform()->
														compileProgram(NULL, m_renderer.getAssetDir(), desc.fragmentShaderPath, 0, getPassName(static_cast<Pass>(i)), "fmain", NULL));
			fragmentSource = m_program[i].fragmentMojoResult->parseResult.source;
			LOGI("Fragment source is: \n %s \n", fragmentSource.c_str());
		}
		else 
		{
			fragmentSource = load_file(desc.fragmentShaderPath);
		}
		
		m_program[i].program		= glCreateProgram();
		if(!m_program[i].program) 
			LOGI("GLES2RenderMaterial", "Failed to create program");
		m_program[i].vertexShader 	= glCreateShader(GL_VERTEX_SHADER);
		if(!m_program[i].vertexShader) 
			LOGI("GLES2RenderMaterial", "Failed to create vertex shader");
		m_program[i].fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		if(!m_program[i].fragmentShader) 
			LOGI("GLES2RenderMaterial", "Failed to create vertex shader");

		glAttachShader(m_program[i].program, m_program[i].vertexShader);
		glAttachShader(m_program[i].program, m_program[i].fragmentShader);
		
		int size = (int)vertexSource.size();
		const char* vertexShaderCstr = vertexSource.c_str();
		glShaderSource(m_program[i].vertexShader, 1, &vertexShaderCstr, &size);
		glCompileShader(m_program[i].vertexShader);
		// __android_log_print(ANDROID_LOG_DEBUG, "GLES2RendererMaterial", "Compiling:\n_______________________________\n%s\n_______________________________\n", vertexSource.c_str());
		LOGI("Vertex shader compile status:");
		glsl_log(m_program[i].vertexShader, GL_COMPILE_STATUS);

		size = (int)fragmentSource.size();
		const char* fragmentShaderCstr = fragmentSource.c_str();
		glShaderSource(m_program[i].fragmentShader, 1, &fragmentShaderCstr, &size);
		glCompileShader(m_program[i].fragmentShader);
		// __android_log_print(ANDROID_LOG_DEBUG, "GLES2RendererMaterial", "Compiling:\n_______________________________\n%s\n_______________________________\n", fragmentSource.c_str());
		LOGI("Fragment shader compile status:");
		glsl_log(m_program[i].fragmentShader, GL_COMPILE_STATUS);

		glLinkProgram(m_program[i].program);
		GLint link_status;
		glsl_log(m_program[i].program, GL_LINK_STATUS);
		glGetProgramiv(m_program[i].program, GL_LINK_STATUS, &link_status);
		if(link_status == GL_FALSE) 
		{
			LOGI("Failed to link program"); 
			GLint error = glGetError();
			if(error == GL_INVALID_VALUE) 
				LOGI("GL_INVALID_VALUE");
			else if(error == GL_INVALID_OPERATION)  
				LOGI("GL_INVALID_OPERATION");
		}
		loadCustomConstants(static_cast<Pass>(i));
	}
	LOGI("");	
}

GLES2RendererMaterial::~GLES2RendererMaterial()
{
}

GLES2RendererMaterial 		*g_hackCurrentMat = NULL;
RendererMaterialInstance 	*g_hackCurrentMatInstance = NULL;

void GLES2RendererMaterial::bind(RendererMaterial::Pass pass, RendererMaterialInstance *materialInstance, bool instanced) const
{
    g_hackCurrentMat = const_cast<GLES2RendererMaterial*>(this);
	g_hackCurrentMatInstance = materialInstance;
	m_currentPass = pass;
    glUseProgram(m_program[m_currentPass].program);
	m_renderer.setCommonRendererParameters();
    RendererMaterial::bind(m_currentPass, materialInstance, instanced);
}

void GLES2RendererMaterial::bindMeshState(bool instanced) const
{
}

void GLES2RendererMaterial::unbind(void) const
{
}

static void bindSampler(const RendererMaterial::Variable &variable, const void *data, RendererMaterial::Pass pass)
{
    GLESVariable &var = (GLESVariable&) variable;
	data = *(void**)data;
	RENDERER_ASSERT(data, "NULL Sampler.");
	if(data)
	{
		GLES2RendererTexture2D *tex = const_cast<GLES2RendererTexture2D*>(static_cast<const GLES2RendererTexture2D*>(data));
		glUniform1i(var.m_location[pass], var.m_sampler[pass]);
		tex->bind(var.m_sampler[pass]);
	}
}

void GLES2RendererMaterial::submitUniforms()
{
	/* When all variables were fetched, we send this uniform array and clear collected variables.
	At this point, if some variables will be binded after all variables already were supplied, 
	this will cause these variables not binded at all, until all other variables collected again. */
	Pass pass = g_hackCurrentMat->m_currentPass;
	if(m_program[pass].vsUniformsTotal > 0 && m_program[pass].vsUniformsCollected.size() == m_program[pass].vsUniformsTotal)
	{
		const GLfloat* ptr = const_cast<const GLfloat*>(reinterpret_cast<GLfloat*>(m_program[pass].vsUniformsVec4));
		//LOGI("VERTEX: Setting uniform at %d of size %d from %x", m_program[pass].vsUniformsVec4Location, m_program[pass].vsUniformsVec4Size, ptr);
		glUniform4fv(m_program[pass].vsUniformsVec4Location, (int)m_program[pass].vsUniformsVec4Size, ptr);
		m_program[pass].vsUniformsCollected.clear();
	}
	/* When all variables were fetched, we send this uniform array and clear collected variables.
	At this point, if some variables will be binded after all variables already were supplied, 
	this will cause these variables not binded at all, until all other variables collected again. */
	if(m_program[pass].psUniformsTotal > 0 && m_program[pass].psUniformsCollected.size() == m_program[pass].psUniformsTotal)
	{
		const GLfloat* ptr = const_cast<const GLfloat*>(reinterpret_cast<GLfloat*>(m_program[pass].psUniformsVec4));
		// LOGI("FRAGMENT: Setting uniform at %d of size %d from %x", m_program[pass].psUniformsVec4Location, m_program[pass].psUniformsVec4Size, ptr);
		glUniform4fv(m_program[pass].psUniformsVec4Location, (int)m_program[pass].psUniformsVec4Size, ptr);
		m_program[pass].psUniformsCollected.clear();
	}
}

void GLES2RendererMaterial::bindVariable(Pass pass, const Variable &variable, const void *data) const
{
    GLESVariable &var = (GLESVariable&) variable;
	if(m_program[pass].vertexMojoResult || m_program[pass].fragmentMojoResult)
	{
		mojoResult::relocationType::iterator it = m_program[pass].vertexMojoResult->relocation.find(variable.getName());
		if(it != m_program[pass].vertexMojoResult->relocation.end())
		{
			if(it->second.name == vertexUniformArrayName)
			{
				mojoResult::shaderUniformRelocation& relocation = it->second;
				// Mojo could truncate unused parts of the original variable, so in the D3D assembly it'll have type which is bigger then needed in the translated shader.
				// This is to prevent buffer overruns in such cases.
				//relocation.size = (relocation.index + relocation.size) > m_program[pass].vsUniformsVec4SizeInBytes ? (m_program[pass].vsUniformsVec4SizeInBytes - relocation.index) : relocation.size;
				//LOGI("Found variable '%s' for %s (%x)", variable.getName(), getPassName(pass), this);
				//LOGI("VERTEX: Copying %d bytes from %x to %x+%d for %s (%s) -- %p", relocation.size, data, m_program[pass].vsUniformsVec4, relocation.index, variable.getName(), getPassName(pass), this);
				memcpy(m_program[pass].vsUniformsVec4 + relocation.index, data, relocation.size);
				m_program[pass].vsUniformsCollected.insert(variable.getName());
			}
			else
			{
				bindSampler(variable, data, pass);
			}
		}
		else if((it = m_program[pass].fragmentMojoResult->relocation.find(variable.getName())) != m_program[pass].fragmentMojoResult->relocation.end())
		{
			if(it->second.name == pixelUniformArrayName)
			{
				mojoResult::shaderUniformRelocation& relocation = it->second;
				// Mojo could truncate unused parts of the original variable, so in the D3D assembly it'll have type which is bigger then needed in the translated shader.
				// This is to prevent buffer overruns in such cases.
				//relocation.size = (relocation.index + relocation.size) > m_program[pass].psUniformsVec4SizeInBytes ? (m_program[pass].psUniformsVec4SizeInBytes - relocation.index) : relocation.size;
				//LOGI("FRAGMENT: Copying %d bytes from %x to %x+%d for %s (%s) -- %p", relocation.size, data, m_program[pass].psUniformsVec4, relocation.index, variable.getName(), getPassName(pass), this);
				memcpy(m_program[pass].psUniformsVec4 + relocation.index, data, relocation.size);
				m_program[pass].psUniformsCollected.insert(variable.getName());
			}
			else
			{
				bindSampler(variable, data, pass);
			}
		}
	}
	else
	{
		switch(var.getType())
		{
			case VARIABLE_FLOAT:
			{
				float f = *(const float*)data;
				//LOGI("GLES2RendererMaterial: VARIABLE_FLOAT");
				glUniform1f(var.m_location[pass], f);
				break;
			}
			case VARIABLE_FLOAT2:
			{
				//LOGI("GLES2RendererMaterial: VARIABLE_FLOAT2");
				glUniform2fv(var.m_location[pass], 1, (const GLfloat*)data);
				break;
			}
			case VARIABLE_FLOAT3:
			{
				//LOGI("GLES2RendererMaterial: VARIABLE_FLOAT3");
				glUniform3fv(var.m_location[pass], 1, (const GLfloat*)data);
				break;
			}
			case VARIABLE_FLOAT4:
			{
				//LOGI("GLES2RendererMaterial: VARIABLE_FLOAT4");
				glUniform4fv(var.m_location[pass], 1, (const GLfloat*)data);
				break;
			}
			case VARIABLE_FLOAT4x4:
			{
				//LOGI("GLES2RendererMaterial: VARIABLE_FLOAT4x4");
				glUniformMatrix4fv(var.m_location[pass], 1, GL_FALSE, (const GLfloat*)data);
				break;
			}
			case VARIABLE_SAMPLER2D:
			{
				bindSampler(variable, data, pass);
				break;
			}
			default:
				break;
		}
	}
}

RendererMaterial::VariableType GLES2RendererMaterial::GLTypetoVariableType(GLenum type)
{
    switch (type)
    {
        case GL_SAMPLER_2D: return VARIABLE_SAMPLER2D;
        case GL_FLOAT: 		return VARIABLE_FLOAT;
		case GL_FLOAT_VEC2: return VARIABLE_FLOAT2;
		case GL_FLOAT_VEC3: return VARIABLE_FLOAT3;
		case GL_FLOAT_VEC4: return VARIABLE_FLOAT4;
		case GL_FLOAT_MAT4: return VARIABLE_FLOAT4x4;
        default: 			return NUM_VARIABLE_TYPES;
    }
}

static std::string variableTypeToString(RendererMaterial::VariableType type)
{
   switch (type)
    {
        case RendererMaterial::VARIABLE_SAMPLER2D: 		return "VARIABLE_SAMPLER2D";
        case RendererMaterial::VARIABLE_FLOAT: 			return "VARIABLE_FLOAT";
		case RendererMaterial::VARIABLE_FLOAT2: 		return "VARIABLE_FLOAT2";
		case RendererMaterial::VARIABLE_FLOAT3: 		return "VARIABLE_FLOAT3";
		case RendererMaterial::VARIABLE_FLOAT4: 		return "VARIABLE_FLOAT4";
		case RendererMaterial::VARIABLE_FLOAT4x4: 		return "VARIABLE_FLOAT4x4";
        default: 										return "VARIABLE_UNKNOWN";
    }
}

GLint GLES2RendererMaterial::getAttribLocation(size_t usage, size_t index, Pass pass)
{
	GLint result = -1;
	if(m_program[pass].vertexMojoResult->parseResult.attributes.find(usage) != m_program[pass].vertexMojoResult->parseResult.attributes.end() && 
		m_program[pass].vertexMojoResult->parseResult.attributes[usage].size() > index)
		return glGetAttribLocation(m_program[pass].program, m_program[pass].vertexMojoResult->parseResult.attributes[usage][index].c_str());
	return result;
}

std::string GLES2RendererMaterial::mojoSampleNameToOriginal(Pass pass, const std::string& name)
{
	for(mojoResult::relocationType::const_iterator it = m_program[pass].vertexMojoResult->relocation.begin();
		it != m_program[pass].vertexMojoResult->relocation.end(); ++it)
		{
			if(it->second.name == name) return it->first;
		}
	for(mojoResult::relocationType::const_iterator it = m_program[pass].fragmentMojoResult->relocation.begin();
		it != m_program[pass].fragmentMojoResult->relocation.end(); ++it)
		{
			if(it->second.name == name) return it->first;
		}
	return std::string();
}

void GLES2RendererMaterial::loadCustomConstants(Pass pass)
{
	/* Find samplers locations */
	std::vector<std::string> uniformArrayName;
	uniformArrayName.push_back("vs_uniforms_vec4");
	uniformArrayName.push_back("ps_uniforms_vec4");
	int sampler = 0;
	GLint count;
	glGetProgramiv(m_program[pass].program, GL_ACTIVE_UNIFORMS, &count);
	
	if (count == 0)
	{
		LOGI("No active uniforms!");
	}

	for (int i = 0; i < count; i++)
	{
		char name[512];
		GLsizei length;
		GLint size;
		GLenum type;
		glGetActiveUniform(m_program[pass].program, i, 512, &length, &size, &type, name);
		
		assert(m_program[pass].vertexMojoResult && m_program[pass].fragmentMojoResult);
		
		RendererMaterial::VariableType vtype = GLTypetoVariableType(type);
		if (strncmp(name, "g_", 2) && (vtype != NUM_VARIABLE_TYPES))
		{
			if(m_program[pass].vertexMojoResult || m_program[pass].fragmentMojoResult)
			{
				if (vtype == VARIABLE_SAMPLER2D)
				{
					// find existing variable...
					GLESVariable *var = NULL;
					bool varFound = false;
					const char* varName = mojoSampleNameToOriginal(pass, name).c_str();
					PxU32 numVariables = (PxU32)m_variables.size();
					for(PxU32 j=0; j<numVariables; j++)
					{
						if(!strcmp(m_variables[j]->getName(), varName))
						{
							var = static_cast<GLESVariable*>(m_variables[j]);
							varFound = true;
							break;
						}
					}
					
					if(!varFound)
						var = new GLESVariable(varName, vtype, m_variableBufferSize);
						
					var->m_location[pass] = glGetUniformLocation(m_program[pass].program, name);
					var->m_sampler[pass] = sampler++;
					if(!varFound)
					{
						m_variables.push_back(var);
						m_variableBufferSize += var->getDataSize();
						LOGI("Uniform '%s' -> location %d, size %d", var->getName(), var->m_location[pass], var->getDataSize());
					} 
					else
					{
						LOGI("Uniform '%s' was updated -> location %d, m_sampler %d", var->getName(), var->m_location[pass], var->m_sampler[pass]);
					}
				}
				else if(uniformArrayName[0].compare(0, uniformArrayName[0].size(), name, uniformArrayName[0].size()) == 0)
				{
					m_program[pass].vsUniformsVec4Size = size;
					m_program[pass].vsUniformsVec4SizeInBytes = m_program[pass].vsUniformsVec4Size * sizeof(float) * 4;
					m_program[pass].vsUniformsVec4 = new char[m_program[pass].vsUniformsVec4SizeInBytes];
					LOGI("Allocated %d bytes at %x for vsUniformsVec4", m_program[pass].vsUniformsVec4SizeInBytes, size_t(m_program[pass].vsUniformsVec4));
					m_program[pass].vsUniformsVec4Location = glGetUniformLocation(m_program[pass].program, name);
					LOGI("Hidden uniform '%s' -> location %d, size %d x vec4", name, m_program[pass].vsUniformsVec4Location, m_program[pass].vsUniformsVec4Size);
				}
				else if(uniformArrayName[1].compare(0, uniformArrayName[1].size(), name, uniformArrayName[1].size()) == 0)
				{
					m_program[pass].psUniformsVec4Size = size;
					m_program[pass].psUniformsVec4SizeInBytes = m_program[pass].psUniformsVec4Size * sizeof(float) * 4;
					m_program[pass].psUniformsVec4 = new char[m_program[pass].psUniformsVec4SizeInBytes];
					LOGI("Allocated %d bytes at %x for psUniformsVec4", m_program[pass].psUniformsVec4SizeInBytes, size_t(m_program[pass].psUniformsVec4));
					m_program[pass].psUniformsVec4Location = glGetUniformLocation(m_program[pass].program, name);
					LOGI("Hidden uniform '%s' -> location %d, size %d x vec4", name, m_program[pass].psUniformsVec4Location, m_program[pass].psUniformsVec4Size);
				}
				else 
				{
					LOGI("Couldn't find uniform!");
				}
			}
			else
			{
				GLESVariable *var = new GLESVariable(name, vtype, m_variableBufferSize);
				var->m_location[pass] = glGetUniformLocation(m_program[pass].program, name);
				if (vtype == VARIABLE_SAMPLER2D)
					var->m_sampler[pass] = sampler++;
				LOGI("Uniform '%s' -> location %d, size %d", name, var->m_location[pass], var->getDataSize());
				m_variables.push_back(var);
				m_variableBufferSize += var->getDataSize();
			}
		}
	}
	/* Use attributes info received from the AndroidSamplePlatform layer */
	if(m_program[pass].vertexMojoResult || m_program[pass].fragmentMojoResult) 
	{
		/* Find attributes locations */
		m_program[pass].positionAttr   = getAttribLocation(MOJOSHADER_USAGE_POSITION, 0, pass);
		m_program[pass].colorAttr      = getAttribLocation(MOJOSHADER_USAGE_COLOR, 0, pass);
		m_program[pass].normalAttr     = getAttribLocation(MOJOSHADER_USAGE_NORMAL, 0, pass);
		m_program[pass].tangentAttr    = getAttribLocation(MOJOSHADER_USAGE_TANGENT, 0, pass);
		m_program[pass].boneIndexAttr  = -1;
		m_program[pass].boneWeightAttr = -1;
		memset(m_program[pass].texcoordAttr, -1, sizeof(m_program[pass].texcoordAttr));
		LOGI("Attributes:");
		if(m_program[pass].vertexMojoResult->parseResult.attributes.find(MOJOSHADER_USAGE_TEXCOORD) != m_program[pass].vertexMojoResult->parseResult.attributes.end())
			{
				for(int i = 0; i < m_program[pass].vertexMojoResult->parseResult.attributes[MOJOSHADER_USAGE_TEXCOORD].size(); ++i)
				{
					m_program[pass].texcoordAttr[i] = glGetAttribLocation(m_program[pass].program, m_program[pass].vertexMojoResult->parseResult.attributes[MOJOSHADER_USAGE_TEXCOORD][i].c_str());
					LOGI("\t TEXCOORD%d: %d", i, m_program[pass].texcoordAttr[i]);
				}
				/* If shader contains more than 4 texture coords inputs, then next one/two should be: bone indices input, bone weights input */
				if(m_program[pass].vertexMojoResult->parseResult.attributes[MOJOSHADER_USAGE_TEXCOORD].size() > 4) 
				{
					m_program[pass].boneIndexAttr = glGetAttribLocation(m_program[pass].program, m_program[pass].vertexMojoResult->parseResult.attributes[MOJOSHADER_USAGE_TEXCOORD][4].c_str());
				}
				if(m_program[pass].vertexMojoResult->parseResult.attributes[MOJOSHADER_USAGE_TEXCOORD].size() > 5) 
				{
					m_program[pass].boneWeightAttr = glGetAttribLocation(m_program[pass].program, m_program[pass].vertexMojoResult->parseResult.attributes[MOJOSHADER_USAGE_TEXCOORD][5].c_str());
				}				
			}

		LOGI("\t POSITION: %d", m_program[pass].positionAttr);
		LOGI("\t COLOR: %d", m_program[pass].colorAttr);
		LOGI("\t NORMAL: %d", m_program[pass].normalAttr);
		LOGI("\t TANGENT: %d", m_program[pass].tangentAttr);
		LOGI("\t BONE INDEX: %d", m_program[pass].boneIndexAttr);
		LOGI("\t BONE WEIGHT: %d", m_program[pass].boneWeightAttr);

		/* Expose variables that were in the original shader, but are now inside one vec4 array */
		std::vector<mojoResult*> mojoResults;
		mojoResults.push_back(m_program[pass].vertexMojoResult);
		mojoResults.push_back(m_program[pass].fragmentMojoResult);
		for(int i = 0; i < mojoResults.size(); ++i) 
		{
			for(mojoResult::relocationType::const_iterator it = mojoResults[i]->relocation.begin();
				it != mojoResults[i]->relocation.end(); ++it)
				{
						if(it->second.name == uniformArrayName[i]) {
							// find existing variable...
							GLESVariable *var = NULL;
							bool varFound = false;
							PxU32 numVariables = (PxU32)m_variables.size();
							for(PxU32 j=0; j<numVariables; j++)
							{
								if(!strcmp(m_variables[j]->getName(), it->first.c_str()))
								{
									var = static_cast<GLESVariable*>(m_variables[j]);
									varFound = true;
									break;
								}
							}
							if(!varFound)
								var = new GLESVariable(it->first.c_str(), GLTypetoVariableType(it->second.type), m_variableBufferSize);
							
							var->setSize(it->second.size);
							var->m_location[pass] = glGetAttribLocation(m_program[pass].program, it->second.name.c_str());
							
							/* Count total variables inside an array */
							/* Inside vertex uniform array ... */
							if(i == 0) m_program[pass].vsUniformsTotal++;
							/* or fragment uniform array */
							else m_program[pass].psUniformsTotal++;

							if(!varFound)
							{
								m_variables.push_back(var);
								m_variableBufferSize += it->second.size;
							}
							 if(varFound)
								LOGI("Variable is found and updated with m_location for %s", getPassName(static_cast<Pass>(pass)));
							LOGI("Uniform '%s' -> location %d, size %d of type %s", var->getName(), var->m_location[pass], it->second.size, variableTypeToString(var->getType()).c_str());
						}
				}
		}
		LOGI("Variables inside vertex uniform array - %d", m_program[pass].vsUniformsTotal);
		LOGI("Variables inside fragment uniform array - %d", m_program[pass].psUniformsTotal);		
	}
	else 
	{		
		m_program[pass].modelViewProjMatrixUniform = glGetUniformLocation(m_program[pass].program, "g_MVP");
		m_program[pass].modelMatrixUniform = glGetUniformLocation(m_program[pass].program, "g_modelMatrix");
		m_program[pass].viewMatrixUniform = glGetUniformLocation(m_program[pass].program, "g_viewMatrix");
		m_program[pass].projMatrixUniform = glGetUniformLocation(m_program[pass].program, "g_projMatrix");
		m_program[pass].modelViewMatrixUniform = glGetUniformLocation(m_program[pass].program, "g_modelViewMatrix");
		m_program[pass].boneMatricesUniform = glGetUniformLocation(m_program[pass].program, "g_boneMatrices");

		m_program[pass].positionAttr   = glGetAttribLocation(m_program[pass].program, "position_attr");
		m_program[pass].colorAttr      = glGetAttribLocation(m_program[pass].program, "color_attr");
		m_program[pass].normalAttr     = glGetAttribLocation(m_program[pass].program, "normal_attr");
		m_program[pass].tangentAttr    = glGetAttribLocation(m_program[pass].program, "tangent_attr");
		m_program[pass].boneIndexAttr  = glGetAttribLocation(m_program[pass].program, "boneIndex_attr");
		m_program[pass].boneWeightAttr = glGetAttribLocation(m_program[pass].program, "boneWeight_attr");

		for(PxU32 i=0; i<8; i++)
		{
			char attr[32];
			sprintf(attr, "texcoord%d_attr", i);
			m_program[pass].texcoordAttr[i] = glGetAttribLocation(m_program[pass].program, attr);
		}
	}
	
	LOGI("That was %p", this);
}

}
#endif // #if defined(RENDERER_ENABLE_GLES2)
