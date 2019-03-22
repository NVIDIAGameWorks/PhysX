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

#include <RendererConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D11)

#include "D3D11RendererMaterial.h"

#include <RendererMaterialDesc.h>

#include "D3D11RendererVariableManager.h"
#include "D3D11RendererTexture2D.h"
#include "D3D11RendererMemoryMacros.h"
#include "D3D11RendererResourceManager.h"
#include "D3D11RendererUtils.h"
#include "D3D11RendererTraits.h"
#include "RendererMemoryMacros.h"

#include <stdio.h>
#include <sstream>

#include <PsUtilities.h>

#if !defined PX_USE_DX11_PRECOMPILED_SHADERS
#define RENDERER_ENABLE_LAYOUT_PRECACHE 1
#endif
static D3D_FEATURE_LEVEL	gFeatureLevel = D3D_FEATURE_LEVEL_9_1;

#if RENDERER_ENABLE_LAYOUT_PRECACHE

static bool gBonePrecached = false;
static bool gStaticPrecached = false;

/* Cache some sensible default input layouts */
static D3D11_INPUT_ELEMENT_DESC inputDescStaticDiffuse[4] = 
{{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}};

static D3D11_INPUT_ELEMENT_DESC inputDescStaticDiffuseInstanced[10] = 
{{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"TEXCOORD", 8, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 1},
 {"TEXCOORD", 9, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
 {"TEXCOORD", 10, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
 {"TEXCOORD", 11, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
 {"TEXCOORD", 12, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
 {"TEXCOORD", 13, DXGI_FORMAT_R32G32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1}};

static D3D11_INPUT_ELEMENT_DESC inputDescBoneDiffuse0[3] = 
{{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}};

static D3D11_INPUT_ELEMENT_DESC inputDescBoneDiffuse1[5] = 
{{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"TEXCOORD", 6, DXGI_FORMAT_R16G16B16A16_UINT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}};

static D3D11_INPUT_ELEMENT_DESC inputDescBoneDiffuse2[6] = 
{{"COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"TEXCOORD", 6, DXGI_FORMAT_R16G16B16A16_UINT, 2, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}};

static D3D11_INPUT_ELEMENT_DESC inputDescBoneDiffuse3[4] = 
{{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"TEXCOORD", 6, DXGI_FORMAT_R16G16B16A16_UINT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}};

static D3D11_INPUT_ELEMENT_DESC inputDescBoneSimple[5] = 
{{"COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
 {"TEXCOORD", 6, DXGI_FORMAT_R16G16B16A16_UINT, 2, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}};

#endif

using namespace SampleRenderer;

static D3D11_BLEND getD3DBlend(RendererMaterial::BlendFunc func)
{
	D3D11_BLEND d3dBlend = D3D11_BLEND_ONE;
	switch (func)
	{
	case RendererMaterial::BLEND_ZERO:
		d3dBlend = D3D11_BLEND_ZERO;
		break;
	case RendererMaterial::BLEND_ONE:
		d3dBlend = D3D11_BLEND_ONE;
		break;
	case RendererMaterial::BLEND_SRC_COLOR:
		d3dBlend = D3D11_BLEND_SRC_COLOR;
		break;
	case RendererMaterial::BLEND_ONE_MINUS_SRC_COLOR:
		d3dBlend = D3D11_BLEND_INV_SRC_COLOR;
		break;
	case RendererMaterial::BLEND_SRC_ALPHA:
		d3dBlend = D3D11_BLEND_SRC_ALPHA;
		break;
	case RendererMaterial::BLEND_ONE_MINUS_SRC_ALPHA:
		d3dBlend = D3D11_BLEND_INV_SRC_ALPHA;
		break;
	case RendererMaterial::BLEND_DST_ALPHA:
		d3dBlend = D3D11_BLEND_DEST_ALPHA;
		break;
	case RendererMaterial::BLEND_ONE_MINUS_DST_ALPHA:
		d3dBlend = D3D11_BLEND_INV_DEST_ALPHA;
		break;
	case RendererMaterial::BLEND_DST_COLOR:
		d3dBlend = D3D11_BLEND_DEST_COLOR;
		break;
	case RendererMaterial::BLEND_ONE_MINUS_DST_COLOR:
		d3dBlend = D3D11_BLEND_INV_DEST_COLOR;
		break;
	case RendererMaterial::BLEND_SRC_ALPHA_SATURATE:
		d3dBlend = D3D11_BLEND_SRC_ALPHA_SAT;
		break;
	}
	return d3dBlend;
}

static D3D_SHADER_MACRO getD3DDefine(const D3D11_INPUT_ELEMENT_DESC& inputDesc)
{
#define D3D_DEFINE_NAME(a)     d3dDefine.Name = PX_STRINGIZE(PX_CONCAT(USE_,a))
#define D3D_DEFINE_NAME_2(a,b) D3D_DEFINE_NAME(PX_CONCAT(a,b))
#define D3D_CASE_INDEX_DEFINE_NAME_WITH_INDEX(a,b) case b: D3D_DEFINE_NAME_2(a,b); break; 
#define D3D_DEFINE_NAME_WITH_INDEX(a,b) {       \
	switch(b) {                                 \
	D3D_CASE_INDEX_DEFINE_NAME_WITH_INDEX(a,0)  \
	D3D_CASE_INDEX_DEFINE_NAME_WITH_INDEX(a,1)  \
	D3D_CASE_INDEX_DEFINE_NAME_WITH_INDEX(a,2)  \
	D3D_CASE_INDEX_DEFINE_NAME_WITH_INDEX(a,3)  \
	D3D_CASE_INDEX_DEFINE_NAME_WITH_INDEX(a,4)  \
	D3D_CASE_INDEX_DEFINE_NAME_WITH_INDEX(a,5)  \
	D3D_CASE_INDEX_DEFINE_NAME_WITH_INDEX(a,6)  \
	D3D_CASE_INDEX_DEFINE_NAME_WITH_INDEX(a,7)  \
	D3D_CASE_INDEX_DEFINE_NAME_WITH_INDEX(a,8)  \
	D3D_CASE_INDEX_DEFINE_NAME_WITH_INDEX(a,9)  \
	D3D_CASE_INDEX_DEFINE_NAME_WITH_INDEX(a,10) \
	D3D_CASE_INDEX_DEFINE_NAME_WITH_INDEX(a,11) \
	D3D_CASE_INDEX_DEFINE_NAME_WITH_INDEX(a,12) \
	D3D_CASE_INDEX_DEFINE_NAME_WITH_INDEX(a,13) \
	D3D_CASE_INDEX_DEFINE_NAME_WITH_INDEX(a,14) \
	D3D_CASE_INDEX_DEFINE_NAME_WITH_INDEX(a,15) \
	D3D_CASE_INDEX_DEFINE_NAME_WITH_INDEX(a,16) \
	default: D3D_DEFINE_NAME(a);    break;      \
	};                                          \
}                                               \

	D3D_SHADER_MACRO d3dDefine = {"DEFAULT", "1"};
	if      (_stricmp(inputDesc.SemanticName, "TEXCOORD") == 0) D3D_DEFINE_NAME_WITH_INDEX(TEXCOORD, inputDesc.SemanticIndex)
	else if (_stricmp(inputDesc.SemanticName, "TANGENT")  == 0) D3D_DEFINE_NAME(TANGENT);
	else if (_stricmp(inputDesc.SemanticName, "POSITION") == 0) D3D_DEFINE_NAME(POSITION);
	else if (_stricmp(inputDesc.SemanticName, "NORMAL")   == 0) D3D_DEFINE_NAME(NORMAL);
	else if (_stricmp(inputDesc.SemanticName, "COLOR")    == 0) D3D_DEFINE_NAME(COLOR);
	else if (_stricmp(inputDesc.SemanticName, "BONE")     == 0) D3D_DEFINE_NAME(BONE);
	return d3dDefine;

#undef D3D_DEFINE_NAME
#undef D3D_DEFINE_NAME_2
#undef D3D_CASE_INDEX_DEFINE_NAME_WITH_INDEX
#undef D3D_DEFINE_NAME_WITH_INDEX
}

static void getD3DDefines(const char* passName, std::vector<D3D10_SHADER_MACRO>& outputDefines)
{
	if(gFeatureLevel <= D3D_FEATURE_LEVEL_9_3)
	{
		const D3D_SHADER_MACRO psDefines[] =
		{
			"RENDERER_FRAGMENT",  "1",
			"RENDERER_D3D11",     "1",
#if RENDERER_ENABLE_SINGLE_PASS_LIGHTING
			"PASS_ALL_LIGHTS",    "1",
			RendererMaterial::getPassName((RendererMaterial::Pass)1), RENDERER_TEXT2(1),
			RendererMaterial::getPassName((RendererMaterial::Pass)2), RENDERER_TEXT2(2),
			RendererMaterial::getPassName((RendererMaterial::Pass)3), RENDERER_TEXT2(3),
			RendererMaterial::getPassName((RendererMaterial::Pass)4), RENDERER_TEXT2(4),
			"MAX_LIGHTS",         RENDERER_TEXT2(RENDERER_MAX_LIGHTS),
#else
			passName,             "1",
#endif
			"NO_SUPPORT_DDX_DDY", "1",
			"ENABLE_VFACE",       "0",
			"ENABLE_VFACE_SCALE", "0",
			"PX_WINDOWS",         "1",
			"ENABLE_SHADOWS",     RENDERER_TEXT2(RENDERER_ENABLE_SHADOWS),
			NULL, NULL
		};

		outputDefines.resize(PX_ARRAY_SIZE(psDefines));
		//memcpy(&outputDefines[0], psDefines, sizeof(psDefines));
		for (PxU32 i = 0; i < outputDefines.size(); ++i)
		{
			outputDefines[i] = psDefines[i];
		}
	}
	else
	{
		const D3D_SHADER_MACRO psDefines[] =
		{
			"RENDERER_FRAGMENT",  "1",
			"RENDERER_D3D11",     "1",
#if RENDERER_ENABLE_SINGLE_PASS_LIGHTING
			"PASS_ALL_LIGHTS",    "1",
			RendererMaterial::getPassName((RendererMaterial::Pass)1), RENDERER_TEXT2(1),
			RendererMaterial::getPassName((RendererMaterial::Pass)2), RENDERER_TEXT2(2),
			RendererMaterial::getPassName((RendererMaterial::Pass)3), RENDERER_TEXT2(3),
			RendererMaterial::getPassName((RendererMaterial::Pass)4), RENDERER_TEXT2(4),
			"MAX_LIGHTS",         RENDERER_TEXT2(RENDERER_MAX_LIGHTS),
#else
			passName,             "1",
#endif
			"ENABLE_VFACE",       RENDERER_TEXT2(RENDERER_ENABLE_VFACE_SCALE),
			"ENABLE_VFACE_SCALE", RENDERER_TEXT2(RENDERER_ENABLE_VFACE_SCALE),
			"PX_WINDOWS",         "1",
			"ENABLE_SHADOWS",     RENDERER_TEXT2(RENDERER_ENABLE_SHADOWS),
			NULL, NULL
		};

		outputDefines.resize(PX_ARRAY_SIZE(psDefines));
		//memcpy(&outputDefines[0], psDefines, sizeof(psDefines));
		for (PxU32 i = 0; i < outputDefines.size(); ++i)
		{
			outputDefines[i] = psDefines[i];
		}
	}
}

static const char* boolToString(bool bTrue)
{
	return bTrue ? "1" : "0";
}

/*
static const bool hasDisplacementSemantic(const D3D11_INPUT_ELEMENT_DESC* inputDesc, PxU32 numInputDescs)
{
	// If no input semantics were specified, enable it by default
	if (NULL == inputDesc || numInputDescs == 0)
		return true;

	for (PxU32 i = 0; i < numInputDescs; ++i)
	{
		if (strcmp(inputDesc[i].SemanticName, "TEXCOORD") == 0 && 
			(inputDesc[i].SemanticIndex == RENDERER_DISPLACEMENT_CHANNEL ||
			 inputDesc[i].SemanticIndex == RENDERER_DISPLACEMENT_FLAGS_CHANNEL))
		return true;
	}
	return false;
}
*/

static void getD3DDefines(const D3D11_INPUT_ELEMENT_DESC* inputDesc, PxU32 numInputDescs, bool bTessellationEnabled, bool bInstanced, std::vector<D3D10_SHADER_MACRO>& outputDefines)
{
	PxU32 i = 0;
	static const D3D_SHADER_MACRO allVsDefine = { "USE_ALL", "1" };
	static const D3D_SHADER_MACRO nullVsDefine = { NULL,     NULL };

	if(gFeatureLevel <= D3D_FEATURE_LEVEL_9_3)
	{
		const D3D_SHADER_MACRO baseVsDefines[] =
		{
			"RENDERER_VERTEX",       "1",
			"RENDERER_D3D11",        "1",
			"PX_WINDOWS",            "1",
			"RENDERER_INSTANCED",    "0",
			"RENDERER_DISPLACED",    "0",
			"ENABLE_VFACE",          "0",
			"ENABLE_VFACE_SCALE",    "0",
			"ENABLE_TESSELLATION",   "0",
			"ADAPTIVE_TESSELLATION", "0",
			"SEMANTIC_TANGENT",      "TANGENT", // This will prevent mapping tangent to texcoord5 and instead to the proper TANGENT semantic
		};
		const int numBaseVsDefines = PX_ARRAY_SIZE(baseVsDefines);

		// Each input element description adds a define
		if (inputDesc && numInputDescs > 0) outputDefines.resize(numBaseVsDefines + numInputDescs + 1);
		// If there are no input element descriptions, we simply add a "USE_ALL" define
		else                                outputDefines.resize(numBaseVsDefines + 2);

		for (; i < numBaseVsDefines; ++i)
		{
			outputDefines[i] = baseVsDefines[i];
		}
	}
	else
	{
		const D3D_SHADER_MACRO baseVsDefines[] =
		{
			"RENDERER_VERTEX",       "1",
			"RENDERER_D3D11",        "1",
			"PX_WINDOWS",            "1",
			"RENDERER_INSTANCED",    boolToString(bInstanced),
			"RENDERER_DISPLACED",    boolToString(bTessellationEnabled),// && hasDisplacementSemantic(inputDesc, numInputDescs)),
			"ENABLE_VFACE",          RENDERER_TEXT2(RENDERER_ENABLE_VFACE_SCALE),
			"ENABLE_VFACE_SCALE",    RENDERER_TEXT2(RENDERER_ENABLE_VFACE_SCALE),
			"ENABLE_TESSELLATION",   boolToString(bTessellationEnabled),
			"ADAPTIVE_TESSELLATION", boolToString(bTessellationEnabled),
			"SEMANTIC_TANGENT",      "TANGENT", // This will prevent mapping tangent to texcoord5 and instead to the proper TANGENT semantic
		};
		const int numBaseVsDefines = PX_ARRAY_SIZE(baseVsDefines);

		// Each input element description adds a define
		if (inputDesc && numInputDescs > 0) outputDefines.resize(numBaseVsDefines + numInputDescs + 1);
		// If there are no input element descriptions, we simply add a "USE_ALL" define
		else                                outputDefines.resize(numBaseVsDefines + 2);
		
		for (; i < numBaseVsDefines; ++i)
		{
			outputDefines[i] = baseVsDefines[i];
		}
	}

	// If input element descriptions were provided, add the appropriate shader defines
	if (inputDesc && numInputDescs > 0)
	{
		for (PxU32 j = 0; j < numInputDescs; ++j)
		{
			if (inputDesc[j].SemanticName)
				outputDefines[i++] = getD3DDefine(inputDesc[j]);
		}
	}
	// Otherwise add the default USE_ALL define
	else
	{
		outputDefines[i++] = allVsDefine;
	}

	outputDefines[i] = nullVsDefine;
}

D3D11RendererMaterial::D3D11RendererMaterial(D3D11Renderer& renderer, const RendererMaterialDesc& desc) :
	RendererMaterial(desc, renderer.getEnableMaterialCaching()),
	m_renderer(renderer),
	m_blendState(NULL),
	m_vertexShader(NULL),
	m_instancedVertexShader(NULL),
	m_geometryShader(NULL),
	m_hullShader(NULL),
	m_domainShader(NULL)
{
	gFeatureLevel = m_renderer.getFeatureLevel();
	memset(m_fragmentPrograms, 0, sizeof(m_fragmentPrograms));

	if (m_renderer.getD3DDevice())
	{
		if (getBlending()) loadBlending(desc);
		loadShaders(desc);
	}	
}

D3D11RendererMaterial::~D3D11RendererMaterial(void)
{
	dxSafeRelease(m_blendState);
	m_renderer.getVariableManager()->unloadVariables(this);

}

void D3D11RendererMaterial::setModelMatrix(const float* matrix)
{
	m_renderer.getVariableManager()->setSharedVariable("cbMesh", "g_modelMatrix", matrix);
	bindMeshState(false);
}

void D3D11RendererMaterial::bind(RendererMaterial::Pass pass, RendererMaterialInstance* materialInstance, bool instanced) const
{
	RENDERER_ASSERT(pass < NUM_PASSES, "Invalid Material Pass.");
	if (m_renderer.getD3DDeviceContext() && pass < NUM_PASSES)
	{
		m_renderer.bind(materialInstance);
		RendererMaterial::bind(pass, materialInstance, instanced);
		setVariables(pass);
		setBlending(pass);
		setShaders(instanced, pass);
	}
}

void D3D11RendererMaterial::bindMeshState(bool instanced) const
{
	m_renderer.getVariableManager()->bind(this, D3DTypes::SHADER_VERTEX);
	if (m_geometryShader) m_renderer.getVariableManager()->bind(this, D3DTypes::SHADER_GEOMETRY);
	if (m_hullShader)     m_renderer.getVariableManager()->bind(this, D3DTypes::SHADER_HULL);
	if (m_domainShader)   m_renderer.getVariableManager()->bind(this, D3DTypes::SHADER_DOMAIN);
}

// This may not be the best place for this, but it works just fine
static ID3D11BlendState* gBlendState = NULL;
static FLOAT gBlendFactor[4]         = {0., 0., 0., 0.};
static UINT  gBlendMask              = 0;

void D3D11RendererMaterial::unbind(void) const
{
	ID3D11DeviceContext* d3dDeviceContext = m_renderer.getD3DDeviceContext();
	if (d3dDeviceContext)
	{
		// Only reset the blend state if it was changed
		if (getBlending() && m_blendState)
		{
			m_renderer.popState(D3D11Renderer::STATE_DEPTHSTENCIL);
			d3dDeviceContext->OMSetBlendState(gBlendState, gBlendFactor, gBlendMask);
			dxSafeRelease(gBlendState);
		}
		d3dDeviceContext->VSSetShader(NULL, NULL, 0);
		d3dDeviceContext->PSSetShader(NULL, NULL, 0);
		d3dDeviceContext->GSSetShader(NULL, NULL, 0);
		d3dDeviceContext->HSSetShader(NULL, NULL, 0);
		d3dDeviceContext->DSSetShader(NULL, NULL, 0);
	}
}

void D3D11RendererMaterial::bindVariable(Pass pass, const Variable& variable, const void* data) const
{
	D3D11Variable& var = *(D3D11Variable*)&variable;
	var.bind(pass, data);
}

void D3D11RendererMaterial::setBlending(RendererMaterial::Pass pass) const
{
	if (getBlending() && m_blendState)
	{
		m_renderer.pushState(D3D11Renderer::STATE_DEPTHSTENCIL);
		m_renderer.setDepthStencilState(D3D11Renderer::DEPTHSTENCIL_TRANSPARENT);

		m_renderer.getD3DDeviceContext()->OMGetBlendState(&gBlendState, gBlendFactor, &gBlendMask);
		m_renderer.getD3DDeviceContext()->OMSetBlendState(m_blendState, NULL, 0xffffffff);
	}
}

void D3D11RendererMaterial::setShaders(bool instanced, RendererMaterial::Pass pass) const
{
	m_renderer.getD3DDeviceContext()->VSSetShader(getVS(instanced), NULL, 0);
	m_renderer.getD3DDeviceContext()->PSSetShader(getPS(pass),      NULL, 0);
	m_renderer.getD3DDeviceContext()->GSSetShader(getGS(),          NULL, 0);
	m_renderer.getD3DDeviceContext()->HSSetShader(getHS(),          NULL, 0);
	m_renderer.getD3DDeviceContext()->DSSetShader(getDS(),          NULL, 0);
}

void D3D11RendererMaterial::setVariables(RendererMaterial::Pass pass) const
{
	m_renderer.getVariableManager()->bind(this, D3DTypes::SHADER_PIXEL, pass);
}

bool D3D11RendererMaterial::tessellationEnabled() const
{
	return  m_renderer.tessellationEnabled() && tessellationSupported();
}

bool SampleRenderer::D3D11RendererMaterial::tessellationInitialized() const
{
	return (NULL != m_hullShader) && (NULL != m_domainShader);
}

bool D3D11RendererMaterial::tessellationSupported() const
{
	return m_renderer.isTessellationSupported() &&
	       !m_shaderNames[D3DTypes::SHADER_DOMAIN].empty() && 
	       !m_shaderNames[D3DTypes::SHADER_HULL].empty();
}

bool D3D11RendererMaterial::geometryEnabled() const
{
	return  m_renderer.getFeatureLevel() >= D3D_FEATURE_LEVEL_10_0 &&
	       !m_shaderNames[D3DTypes::SHADER_GEOMETRY].empty();
}

bool D3D11RendererMaterial::geometryInitialized() const
{
	return NULL != m_geometryShader;
}

void D3D11RendererMaterial::loadBlending(const RendererMaterialDesc& desc)
{
	D3D11_BLEND_DESC blendDesc;
	memset(&blendDesc, 0, sizeof(blendDesc));
	blendDesc.AlphaToCoverageEnable                 = m_renderer.multisamplingEnabled();
	blendDesc.IndependentBlendEnable                = FALSE;
	blendDesc.RenderTarget[0].BlendEnable           = getBlending();
	blendDesc.RenderTarget[0].SrcBlend              = getD3DBlend(getSrcBlendFunc());
	blendDesc.RenderTarget[0].DestBlend             = getD3DBlend(getDstBlendFunc());
	blendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	m_renderer.getD3DDevice()->CreateBlendState(&blendDesc, &m_blendState);
	RENDERER_ASSERT(m_blendState, "Failed to create blend state.");
}

void D3D11RendererMaterial::loadShaders(const RendererMaterialDesc& desc)
{
	HRESULT result = S_OK;

#ifdef PX_USE_DX11_PRECOMPILED_SHADERS
	if(gFeatureLevel < D3D_FEATURE_LEVEL_11_0)
	{
		m_shaderPaths[D3DTypes::SHADER_VERTEX]       = std::string(m_renderer.getAssetDir()) + "compiledshaders/dx11feature9/" + desc.vertexShaderPath + ".cso";
		m_shaderNames[D3DTypes::SHADER_VERTEX]       = std::string(desc.vertexShaderPath) + ".cso";
		m_shaderPaths[D3DTypes::SHADER_PIXEL]        = std::string(m_renderer.getAssetDir()) + "compiledshaders/dx11feature9/" + desc.fragmentShaderPath;
		m_shaderNames[D3DTypes::SHADER_PIXEL]        = std::string(desc.fragmentShaderPath);
	}
	else
	{
		m_shaderPaths[D3DTypes::SHADER_VERTEX]       = std::string(m_renderer.getAssetDir()) + "compiledshaders/dx11feature11/" + desc.vertexShaderPath + ".cso";
		m_shaderNames[D3DTypes::SHADER_VERTEX]       = std::string(desc.vertexShaderPath) + ".cso";
		m_shaderPaths[D3DTypes::SHADER_PIXEL]        = std::string(m_renderer.getAssetDir()) + "compiledshaders/dx11feature11/" + desc.fragmentShaderPath;
		m_shaderNames[D3DTypes::SHADER_PIXEL]        = std::string(desc.fragmentShaderPath);
	
		if (desc.geometryShaderPath)
		{
			m_shaderPaths[D3DTypes::SHADER_GEOMETRY] = std::string(m_renderer.getAssetDir()) + "compiledshaders/dx11feature11/" + desc.geometryShaderPath;
			m_shaderNames[D3DTypes::SHADER_GEOMETRY] = desc.geometryShaderPath;
		}
		if (desc.hullShaderPath)
		{
			m_shaderPaths[D3DTypes::SHADER_HULL]     = std::string(m_renderer.getAssetDir()) + "compiledshaders/dx11feature11/" + desc.hullShaderPath;	
			m_shaderNames[D3DTypes::SHADER_HULL]     = desc.hullShaderPath;
		}
		if (desc.domainShaderPath)
		{
			m_shaderPaths[D3DTypes::SHADER_DOMAIN]   = std::string(m_renderer.getAssetDir()) + "compiledshaders/dx11feature11/" + desc.domainShaderPath;
			m_shaderNames[D3DTypes::SHADER_DOMAIN]   = desc.domainShaderPath;
		}

	}
#else
	m_shaderPaths[D3DTypes::SHADER_VERTEX]       = std::string(m_renderer.getAssetDir()) + "shaders/" + desc.vertexShaderPath;
	m_shaderNames[D3DTypes::SHADER_VERTEX]       = desc.vertexShaderPath;
	m_shaderPaths[D3DTypes::SHADER_PIXEL]        = std::string(m_renderer.getAssetDir()) + "shaders/" + desc.fragmentShaderPath;
	m_shaderNames[D3DTypes::SHADER_PIXEL]        = desc.fragmentShaderPath;
	if (desc.geometryShaderPath)
	{
		m_shaderPaths[D3DTypes::SHADER_GEOMETRY] = std::string(m_renderer.getAssetDir()) + "shaders/" + desc.geometryShaderPath;
		m_shaderNames[D3DTypes::SHADER_GEOMETRY] = desc.geometryShaderPath;
	}
	if (desc.hullShaderPath)
	{
		m_shaderPaths[D3DTypes::SHADER_HULL]     = std::string(m_renderer.getAssetDir()) + "shaders/" + desc.hullShaderPath;	
		m_shaderNames[D3DTypes::SHADER_HULL]     = desc.hullShaderPath;
	}
	if (desc.domainShaderPath)
	{
		m_shaderPaths[D3DTypes::SHADER_DOMAIN]   = std::string(m_renderer.getAssetDir()) + "shaders/" + desc.domainShaderPath;
		m_shaderNames[D3DTypes::SHADER_DOMAIN]   = desc.domainShaderPath;
	}
#endif

	ID3DBlob* pShaderBlob = NULL;

	D3D11ShaderLoader loader(m_renderer);
	std::vector<D3D_SHADER_MACRO> vsDefines;
	std::vector<D3D_SHADER_MACRO> psDefines;

	// Load vertex shader
	getD3DDefines(NULL, 0, tessellationSupported(), false, vsDefines);
	D3DTraits<ID3D11VertexShader>::key_type vsKey(0xffffff, m_shaderNames[D3DTypes::SHADER_VERTEX]);
	result = loader.load<ID3D11VertexShader>(vsKey, getPath(D3DTypes::SHADER_VERTEX), &vsDefines[0], &m_vertexShader, &pShaderBlob);
	if (SUCCEEDED(result)) m_renderer.getVariableManager()->loadVariables(this, pShaderBlob, D3DTypes::SHADER_VERTEX);

	// Load pixel shadders for each pass
	for (PxU32 i = 0; i < NUM_PASSES; i++)
	{
		if (SUCCEEDED(result))
		{
			getD3DDefines(getPassName(Pass(i)), psDefines);
			std::string shaderName = m_shaderNames[D3DTypes::SHADER_PIXEL];
			const char* pixelShaderPath = getPath(D3DTypes::SHADER_PIXEL);
#ifdef PX_USE_DX11_PRECOMPILED_SHADERS
			shaderName += std::string(".") + (getPassName(Pass(i))) + ".cso";
			char shaderPathCompiled[MAX_PATH];
			strcpy(shaderPathCompiled,pixelShaderPath);
			strcat(shaderPathCompiled,".");
			strcat(shaderPathCompiled,getPassName(Pass(i)));
			strcat(shaderPathCompiled,".cso");
			pixelShaderPath = shaderPathCompiled;
#endif
			D3DTraits<ID3D11PixelShader>::key_type psKey(i, shaderName);
			result = loader.load<ID3D11PixelShader>(psKey, pixelShaderPath, &psDefines[0], &m_fragmentPrograms[i], &pShaderBlob, true);
			if (SUCCEEDED(result)) m_renderer.getVariableManager()->loadVariables(this, pShaderBlob, D3DTypes::SHADER_PIXEL, (Pass)i);
		}
	}

	// Load geometry shader
	if (SUCCEEDED(result) && geometryEnabled())
	{
		D3DTraits<ID3D11GeometryShader>::key_type gsKey(m_shaderNames[D3DTypes::SHADER_GEOMETRY]);
		result = loader.load<ID3D11GeometryShader>(gsKey, getPath(D3DTypes::SHADER_GEOMETRY), &vsDefines[0], &m_geometryShader, &pShaderBlob);
		if (SUCCEEDED(result)) m_renderer.getVariableManager()->loadVariables(this, pShaderBlob, D3DTypes::SHADER_GEOMETRY);
	}

	// Load hull shader
	if (SUCCEEDED(result) && tessellationSupported())
	{
		D3DTraits<ID3D11HullShader>::key_type hsKey(0xffffff, m_shaderNames[D3DTypes::SHADER_HULL]);
		result = loader.load<ID3D11HullShader>(hsKey, getPath(D3DTypes::SHADER_HULL), &vsDefines[0], &m_hullShader, &pShaderBlob);
		if (SUCCEEDED(result)) m_renderer.getVariableManager()->loadVariables(this, pShaderBlob, D3DTypes::SHADER_HULL);
	}

	// Load domain shader
	if (SUCCEEDED(result) && tessellationSupported())
	{
		D3DTraits<ID3D11DomainShader>::key_type dsKey(0xffffff, m_shaderNames[D3DTypes::SHADER_DOMAIN]);
		result = loader.load<ID3D11DomainShader>(dsKey, getPath(D3DTypes::SHADER_DOMAIN), &vsDefines[0], &m_domainShader, &pShaderBlob);
		if (SUCCEEDED(result)) m_renderer.getVariableManager()->loadVariables(this, pShaderBlob, D3DTypes::SHADER_DOMAIN);
	}

#if RENDERER_ENABLE_LAYOUT_PRECACHE
#define CACHE_VS(desc, instanced) cacheVS(desc, ARRAYSIZE(desc), D3DX11::getInputHash(desc, ARRAYSIZE(desc)), instanced)

	if (!gStaticPrecached && !strcmp(desc.vertexShaderPath, "vertex/staticmesh.cg"))
	{
		gStaticPrecached = true;
		CACHE_VS(inputDescStaticDiffuse, false);
		CACHE_VS(inputDescStaticDiffuseInstanced, true);
	}
	if (!gBonePrecached && !strcmp(desc.vertexShaderPath, "vertex/skeletalmesh_1bone.cg"))
	{
		gBonePrecached = true;
		CACHE_VS(inputDescBoneDiffuse0, false);
		CACHE_VS(inputDescBoneDiffuse1, false);
		CACHE_VS(inputDescBoneDiffuse2, false);
		CACHE_VS(inputDescBoneDiffuse3, false);
		CACHE_VS(inputDescBoneSimple, false);
	}
#undef CACHE_VS
#endif // RENDERER_ENABLE_LAYOUT_PRECACHE
}

std::string D3D11RendererMaterial::getShaderNameFromInputLayout(const D3D11_INPUT_ELEMENT_DESC* inputDesc,PxU32 numInputDescs, const std::string& shaderName) const
{
	std::string outString = shaderName;

#ifdef PX_USE_DX11_PRECOMPILED_SHADERS
	// rename
	if(inputDesc && numInputDescs > 0)
	{
		PX_ASSERT(outString.size() > 4);
		outString.resize(outString.length()-4);
		for (PxU32 i = 0; i < numInputDescs; i++)
		{
			if (inputDesc[i].SemanticName)
			{
				outString += "_";
				outString += &getD3DDefine(inputDesc[i]).Name[4];
			}
		}
		outString += ".cso";
	}
#endif

	return outString;
}

ID3DBlob* D3D11RendererMaterial::getVSBlob(const D3D11_INPUT_ELEMENT_DESC* inputDesc, PxU32 numInputDescs, PxU64 inputDescHash, bool bInstanced) const
{
	D3D11ShaderCacher cacher(m_renderer.getResourceManager());
	D3DTraits<ID3D11VertexShader>::value_type vsValue;
	if (!cacher.check<ID3D11VertexShader>(std::make_pair(inputDescHash,getShaderNameFromInputLayout(inputDesc,numInputDescs,m_shaderNames[D3DTypes::SHADER_VERTEX])), vsValue.first, vsValue.second))
	{
		cacheVS(inputDesc, numInputDescs, inputDescHash, bInstanced, &vsValue.first, &vsValue.second);
	}

	// In the event we had to create a new shader for the given input layout,
	//        we'll need to assign it as the current shader
	ID3D11VertexShader*& currentShader = bInstanced ? m_instancedVertexShader : m_vertexShader;
	if (currentShader != vsValue.first)
	{
		currentShader = vsValue.first;
		m_renderer.getD3DDeviceContext()->VSSetShader(currentShader, NULL, 0);
	}

	return vsValue.second;
}

ID3DBlob* D3D11RendererMaterial::getVSBlob(const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputDesc, PxU64 inputDescHash, bool bInstanced) const
{
	return getVSBlob(&inputDesc[0], (UINT)inputDesc.size(), inputDescHash, bInstanced);
}

void D3D11RendererMaterial::cacheVS(const D3D11_INPUT_ELEMENT_DESC* inputDesc, PxU32 numInputDescs, PxU64 inputDescHash, bool bInstanced, ID3D11VertexShader** ppShader, ID3DBlob** ppBlob) const 
{
	std::vector<D3D10_SHADER_MACRO> defines;
	getD3DDefines(inputDesc, numInputDescs, tessellationSupported(), bInstanced, defines);

	D3DTraits<ID3D11VertexShader>::key_type vsKey(inputDescHash, getShaderNameFromInputLayout(inputDesc,numInputDescs, m_shaderNames[D3DTypes::SHADER_VERTEX]));

	D3D11ShaderLoader loader(m_renderer);
	if (FAILED(loader.load<ID3D11VertexShader>(vsKey, getShaderNameFromInputLayout(inputDesc,numInputDescs,getPath(D3DTypes::SHADER_VERTEX)).c_str(), &defines[0], ppShader, ppBlob, false)))
	{
		RENDERER_ASSERT(0, "Error loading D3D11 layout signature.");
	}
}

#endif

