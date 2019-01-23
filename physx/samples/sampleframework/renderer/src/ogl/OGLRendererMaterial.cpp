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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.

#include "RendererConfig.h"

#if defined(RENDERER_ENABLE_OPENGL)

#include "OGLRendererMaterial.h"
#include "OGLRendererTexture2D.h"

#include <RendererMaterialDesc.h>

#include <SamplePlatform.h>

#include <stdio.h>

// for PsString.h
namespace physx
{
	namespace string
	{}
}
#include <PsString.h>
#include <PxTkFile.h>
namespace Ps = physx::shdfnd;

using namespace SampleRenderer;

#if defined(RENDERER_ENABLE_CG)
	
static RendererMaterial::VariableType getVariableType(CGtype cgt)
{
	RendererMaterial::VariableType vt = RendererMaterial::NUM_VARIABLE_TYPES;
	switch(cgt)
	{
		case CG_INT:       vt = RendererMaterial::VARIABLE_INT;       break;
		case CG_FLOAT:     vt = RendererMaterial::VARIABLE_FLOAT;     break;
		case CG_FLOAT2:    vt = RendererMaterial::VARIABLE_FLOAT2;    break;
		case CG_FLOAT3:    vt = RendererMaterial::VARIABLE_FLOAT3;    break;
		case CG_FLOAT4:    vt = RendererMaterial::VARIABLE_FLOAT4;    break;
		case CG_FLOAT4x4:  vt = RendererMaterial::VARIABLE_FLOAT4x4;  break;
		case CG_SAMPLER2D: vt = RendererMaterial::VARIABLE_SAMPLER2D; break;
		case CG_SAMPLER3D: vt = RendererMaterial::VARIABLE_SAMPLER3D; break;
		default: break;
	}
	RENDERER_ASSERT(vt < RendererMaterial::NUM_VARIABLE_TYPES, "Unable to convert shader parameter type.");
	return vt;
}

static GLuint getGLBlendFunc(RendererMaterial::BlendFunc func)
{
	GLuint glfunc = 0;
	switch(func)
	{
		case RendererMaterial::BLEND_ZERO:                glfunc = GL_ZERO;                break;
		case RendererMaterial::BLEND_ONE:                 glfunc = GL_ONE;                 break;
		case RendererMaterial::BLEND_SRC_COLOR:           glfunc = GL_SRC_COLOR;           break;
		case RendererMaterial::BLEND_ONE_MINUS_SRC_COLOR: glfunc = GL_ONE_MINUS_SRC_COLOR; break;
		case RendererMaterial::BLEND_SRC_ALPHA:           glfunc = GL_SRC_ALPHA;           break;
		case RendererMaterial::BLEND_ONE_MINUS_SRC_ALPHA: glfunc = GL_ONE_MINUS_SRC_ALPHA; break;
		case RendererMaterial::BLEND_DST_ALPHA:           glfunc = GL_DST_COLOR;           break;
		case RendererMaterial::BLEND_ONE_MINUS_DST_ALPHA: glfunc = GL_ONE_MINUS_DST_ALPHA; break;
		case RendererMaterial::BLEND_DST_COLOR:           glfunc = GL_DST_COLOR;           break;
		case RendererMaterial::BLEND_ONE_MINUS_DST_COLOR: glfunc = GL_ONE_MINUS_DST_COLOR; break;
		case RendererMaterial::BLEND_SRC_ALPHA_SATURATE:  glfunc = GL_SRC_ALPHA_SATURATE;  break;
		default: break;
	}
	RENDERER_ASSERT(glfunc, "Unable to convert Material Blend Func.");
	return glfunc;
}

static void connectParameters(CGparameter from, CGparameter to)
{
	if(from && to) cgConnectParameter(from, to);
}

static void connectEnvParameters(const OGLRenderer::CGEnvironment &cgEnv, CGprogram program)
{
	connectParameters(cgEnv.modelMatrix,         cgGetNamedParameter(program, "g_" "modelMatrix"));
	connectParameters(cgEnv.viewMatrix,          cgGetNamedParameter(program, "g_" "viewMatrix"));
	connectParameters(cgEnv.projMatrix,          cgGetNamedParameter(program, "g_" "projMatrix"));
	connectParameters(cgEnv.modelViewMatrix,     cgGetNamedParameter(program, "g_" "modelViewMatrix"));
	connectParameters(cgEnv.modelViewProjMatrix, cgGetNamedParameter(program, "g_" "modelViewProjMatrix"));


	connectParameters(cgEnv.boneMatrices,         cgGetNamedParameter(program, "g_" "boneMatrices"));
	
	connectParameters(cgEnv.eyePosition,         cgGetNamedParameter(program, "g_" "eyePosition"));
	connectParameters(cgEnv.eyeDirection,        cgGetNamedParameter(program, "g_" "eyeDirection"));

	connectParameters(cgEnv.fogColorAndDistance, cgGetNamedParameter(program, "g_" "fogColorAndDistance"));

	connectParameters(cgEnv.ambientColor,        cgGetNamedParameter(program, "g_" "ambientColor"));
	
	connectParameters(cgEnv.lightColor,          cgGetNamedParameter(program, "g_" "lightColor"));
	connectParameters(cgEnv.lightIntensity,      cgGetNamedParameter(program, "g_" "lightIntensity"));
	connectParameters(cgEnv.lightDirection,      cgGetNamedParameter(program, "g_" "lightDirection"));
	connectParameters(cgEnv.lightPosition,       cgGetNamedParameter(program, "g_" "lightPosition"));
	connectParameters(cgEnv.lightInnerRadius,    cgGetNamedParameter(program, "g_" "lightInnerRadius"));
	connectParameters(cgEnv.lightOuterRadius,    cgGetNamedParameter(program, "g_" "lightOuterRadius"));
	connectParameters(cgEnv.lightInnerCone,      cgGetNamedParameter(program, "g_" "lightInnerCone"));
	connectParameters(cgEnv.lightOuterCone,      cgGetNamedParameter(program, "g_" "lightOuterCone"));
}

OGLRendererMaterial::CGVariable::CGVariable(const char *name, VariableType type, PxU32 offset) :
	Variable(name, type, offset)
{
	m_vertexHandle = 0;
	memset(m_fragmentHandles, 0, sizeof(m_fragmentHandles));
}

OGLRendererMaterial::CGVariable::~CGVariable(void)
{
	
}

void OGLRendererMaterial::CGVariable::addVertexHandle(CGparameter handle)
{
	m_vertexHandle = handle;
}

void OGLRendererMaterial::CGVariable::addFragmentHandle(CGparameter handle, Pass pass)
{
	m_fragmentHandles[pass] = handle;
}

#endif

OGLRendererMaterial::OGLRendererMaterial(OGLRenderer &renderer, const RendererMaterialDesc &desc) :
	RendererMaterial(desc, renderer.getEnableMaterialCaching()),
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
	
#if defined(RENDERER_ENABLE_CG)
	m_vertexProgram   = 0;
#define NO_SUPPORT_DDX_DDY
#if 0
	// JD: CG_PROFILE_GPU_VP FAILS SO BAD!
	m_vertexProfile   = CG_PROFILE_ARBVP1;
	m_fragmentProfile = CG_PROFILE_ARBVP1;
#else
	// PH: Seems to work fine nowadays
	m_vertexProfile   = cgGLGetLatestProfile(CG_GL_VERTEX);
	m_fragmentProfile = cgGLGetLatestProfile(CG_GL_FRAGMENT);
#endif

	memset(m_fragmentPrograms, 0, sizeof(m_fragmentPrograms));
	
	CGcontext                         cgContext = m_renderer.getCGContext();
	const OGLRenderer::CGEnvironment &cgEnv     = m_renderer.getCGEnvironment();
	if(cgContext && m_vertexProfile && m_fragmentProfile)
	{
		char fullPath[1024] = "-I";
		Ps::strlcat(fullPath, 1024, m_renderer.getAssetDir());
		Ps::strlcat(fullPath, 1024, "shaders/include");
		const char *vertexEntry = "vmain";
		const char *vertexArgs[] =
		{
			fullPath,
			"-DRENDERER_VERTEX",
			0, 0,
		};		

		cgGLSetOptimalOptions(m_vertexProfile);
		if(1)
		{
			PX_PROFILE_ZONE("OGLRendererMaterial_compile_vertexProgram",0);
			m_vertexProgram = static_cast<CGprogram>(SampleFramework::SamplePlatform::platform()->compileProgram(cgContext, m_renderer.getAssetDir(), desc.vertexShaderPath, m_vertexProfile, 0, vertexEntry, vertexArgs));
		}
		RENDERER_ASSERT(m_vertexProgram, "Failed to compile Vertex Shader.");
		if(m_vertexProgram)
		{
			PX_PROFILE_ZONE("OGLRendererMaterial_load_vertexProgram",0);
			connectEnvParameters(cgEnv, m_vertexProgram);
			cgGLLoadProgram(m_vertexProgram);
			loadCustomConstants(m_vertexProgram, NUM_PASSES);
		}
		else
		{
			char msg[1024];
			Ps::snprintf(msg, sizeof(msg), "Could not find shader file: <%s> in path: <%sshaders/>", desc.vertexShaderPath, m_renderer.getAssetDir());
			RENDERER_OUTPUT_MESSAGE(&m_renderer, msg);
		}
		
		const char *fragmentEntry = "fmain";
		for(PxU32 i=0; i<NUM_PASSES; i++)
		{
			char passDefine[64] = {0};
			Ps::snprintf(passDefine, 63, "-D%s", getPassName((Pass)i));

			char fvaceDefine[20] = "-DENABLE_VFACE=0";
#if PX_WINDOWS
			// Aparently the FACE semantic is only supported with fp40
			if (cgGLIsProfileSupported(CG_PROFILE_FP40))
			{
				fvaceDefine[15] = '1';
			}
#endif
			const char *fragmentArgs[]  =
			{
				fullPath,
				"-DRENDERER_FRAGMENT",
				fvaceDefine,			// used for double sided rendering (as done in D3D anyways)
				"-DVFACE=FACE",			// rename VFACE to FACE semantic, the first is only known to HLSL shaders...
#ifdef NO_SUPPORT_DDX_DDY
				"-DNO_SUPPORT_DDX_DDY",
#endif
				passDefine,
				0, 0,
			};
			cgGLSetOptimalOptions(m_fragmentProfile);
			CGprogram fp = 0;
			if(1)
			{
				PX_PROFILE_ZONE("OGLRendererMaterial_compile_fragmentProgram",0);
				fp = static_cast<CGprogram>(SampleFramework::SamplePlatform::platform()->compileProgram(cgContext, m_renderer.getAssetDir(), desc.fragmentShaderPath, m_fragmentProfile, getPassName((Pass)i), fragmentEntry, fragmentArgs));
			}
			RENDERER_ASSERT(fp, "Failed to compile Fragment Shader.");
			if(fp)
			{
				PX_PROFILE_ZONE("OGLRendererMaterial_load_fragmentProgram",0);
				connectEnvParameters(cgEnv, fp);
				cgGLLoadProgram(fp);
				m_fragmentPrograms[i] = fp;
				loadCustomConstants(fp, (Pass)i);
			}
		}
	}
#endif
}

OGLRendererMaterial::~OGLRendererMaterial(void)
{
#if defined(RENDERER_ENABLE_CG)
	if(m_vertexProgram)
	{
		cgDestroyProgram(m_vertexProgram);
	}
	for(PxU32 i=0; i<NUM_PASSES; i++)
	{
		CGprogram fp = m_fragmentPrograms[i];
		if(fp)
		{
			cgDestroyProgram(fp);
		}
	}
#endif
}

void OGLRendererMaterial::bind(RendererMaterial::Pass pass, RendererMaterialInstance *materialInstance, bool instanced) const
{
	m_renderer.setCurrentMaterial(this);
	
	if(m_glAlphaTestFunc == GL_ALWAYS)
	{
		glDisable(GL_ALPHA_TEST);
	}
	else
	{
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(m_glAlphaTestFunc, PxClamp(getAlphaTestRef(), 0.0f, 1.0f));
	}
	
	if(getBlending())
	{
		glBlendFunc(getGLBlendFunc(getSrcBlendFunc()), getGLBlendFunc(getDstBlendFunc()));
		glEnable(GL_BLEND);
		glDepthMask(0);
	}

#if defined(RENDERER_ENABLE_CG)
	if(m_vertexProgram)
	{
		cgGLEnableProfile(m_vertexProfile);
		cgGLBindProgram(m_vertexProgram);
		cgUpdateProgramParameters(m_vertexProgram);
	}
	if(pass < NUM_PASSES && m_fragmentPrograms[pass])
	{
		cgGLEnableProfile(m_fragmentProfile);
		cgGLBindProgram(m_fragmentPrograms[pass]);
		
		RendererMaterial::bind(pass, materialInstance, instanced);
		//this is a kludge to make the particles work. I see no way to set env's through this interface
		glEnable(GL_POINT_SPRITE_ARB);
		glTexEnvf(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
		glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);

		cgUpdateProgramParameters(m_fragmentPrograms[pass]);
	}
#endif
}

void OGLRendererMaterial::bindMeshState(bool instanced) const
{
#if defined(RENDERER_ENABLE_CG)
	if(m_vertexProgram)
	{
		cgUpdateProgramParameters(m_vertexProgram);
	}
#endif
}

void OGLRendererMaterial::unbind(void) const
{
#if defined(RENDERER_ENABLE_CG)
	glTexEnvf(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_FALSE);
	glDisable(GL_POINT_SPRITE_ARB);
	glDisable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);

	if(m_vertexProfile)
	{
		cgGLUnbindProgram(m_vertexProfile);
		cgGLDisableProfile(m_vertexProfile);
	}
	if(m_fragmentProfile)
	{
		cgGLUnbindProgram(m_fragmentProfile);
		cgGLDisableProfile(m_fragmentProfile);
	}
#endif

	if(getBlending())
	{
		glDisable(GL_BLEND);
		glDepthMask(1);
	}

	m_renderer.setCurrentMaterial(0);
}

#if defined(RENDERER_ENABLE_CG)
template<class TextureType>
static void bindSamplerVariable(CGparameter param, RendererTexture2D &texture)
{
	TextureType &tex = *static_cast<TextureType*>(&texture);
	if(param)
	{
		CGresource resource = cgGetParameterResource(param);
		RENDERER_ASSERT(resource >= CG_TEXUNIT0 && resource <= CG_TEXUNIT15, "Invalid Texture Resource Location.");
		if(resource >= CG_TEXUNIT0 && resource <= CG_TEXUNIT15)
		{
			tex.bind(resource-CG_TEXUNIT0);
		}
	}
}
#endif

void OGLRendererMaterial::bindVariable(Pass pass, const Variable &variable, const void *data) const
{
#if defined(RENDERER_ENABLE_CG)
	CGVariable &var = *(CGVariable*)&variable;
	switch(var.getType())
	{
		case VARIABLE_FLOAT:
		{
			float f = *(const float*)data;
			if(var.m_vertexHandle)          cgGLSetParameter1f(var.m_vertexHandle,          f);
			if(var.m_fragmentHandles[pass]) cgGLSetParameter1f(var.m_fragmentHandles[pass], f);
			break;
		}
		case VARIABLE_FLOAT2:
		{
			const float *f = (const float*)data;
			if(var.m_vertexHandle)          cgGLSetParameter2fv(var.m_vertexHandle,          f);
			if(var.m_fragmentHandles[pass]) cgGLSetParameter2fv(var.m_fragmentHandles[pass], f);
			break;
		}
		case VARIABLE_FLOAT3:
		{
			const float *f = (const float*)data;
			if(var.m_vertexHandle)          cgGLSetParameter3fv(var.m_vertexHandle,          f);
			if(var.m_fragmentHandles[pass]) cgGLSetParameter3fv(var.m_fragmentHandles[pass], f);
			break;
		}
		case VARIABLE_FLOAT4:
		{
			const float *f = (const float*)data;
			if(var.m_vertexHandle)          cgGLSetParameter4fv(var.m_vertexHandle,          f);
			if(var.m_fragmentHandles[pass]) cgGLSetParameter4fv(var.m_fragmentHandles[pass], f);
			break;
		}
		case VARIABLE_SAMPLER2D:
			data = *(void**)data;
			RENDERER_ASSERT(data, "NULL Sampler.");
			if(data)
			{
				bindSamplerVariable<OGLRendererTexture2D>(var.m_vertexHandle,          *(RendererTexture2D*)data);
				bindSamplerVariable<OGLRendererTexture2D>(var.m_fragmentHandles[pass], *(RendererTexture2D*)data);
			}
			break;
		case VARIABLE_SAMPLER3D:
			RENDERER_ASSERT(0, "3D GL Textures Not Implemented.");
			/*
			data = *(void**)data;
			RENDERER_ASSERT(data, "NULL Sampler.");
			if(data)
			{
				bindSamplerVariable<OGLRendererTexture3D>(var.m_vertexHandle,          *(RendererTexture2D*)data);
				bindSamplerVariable<OGLRendererTexture3D>(var.m_fragmentHandles[pass], *(RendererTexture2D*)data);
			}
			*/
			break;
		default: 
			RENDERER_ASSERT(0, "Cannot bind variable of this type.");
			break;
	}
#endif
}

#if defined(RENDERER_ENABLE_CG)
void OGLRendererMaterial::loadCustomConstants(CGprogram program, Pass pass)
{
	for(CGparameter param = cgGetFirstParameter(program, CG_GLOBAL); param; param=cgGetNextParameter(param))
	{
		const char  *name   = cgGetParameterName(param);
		CGtype       cgtype = cgGetParameterType(param);
		VariableType type   = cgtype != CG_STRUCT && cgtype != CG_ARRAY ? getVariableType(cgtype) : NUM_VARIABLE_TYPES;
		if(type < NUM_VARIABLE_TYPES && cgIsParameter(param) && cgIsParameterReferenced(param) && strncmp(name, "g_", 2))
		{
			CGVariable *var = 0;
			// find existing variable...
			PxU32 numVariables = (PxU32)m_variables.size();
			for(PxU32 i=0; i<numVariables; i++)
			{
				if(!strcmp(m_variables[i]->getName(), name))
				{
					var = static_cast<CGVariable*>(m_variables[i]);
					break;
				}
			}
			// check to see if the variable is of the same type.
			if(var)
			{
				RENDERER_ASSERT(var->getType() == type, "Variable changes type!");
			}
			// if the variable was not found, add it.
			if(!var)
			{
				var = new CGVariable(name, type, m_variableBufferSize);
				m_variables.push_back(var);
				m_variableBufferSize += var->getDataSize();
			}
			if(pass < NUM_PASSES)
			{
				var->addFragmentHandle(param, pass);
			}
			else
			{
				var->addVertexHandle(param);
			}
		}
	}
}
#endif

#endif // #if defined(RENDERER_ENABLE_OPENGL)
