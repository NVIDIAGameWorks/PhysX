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
#include <SampleMaterialAsset.h>

#include <Renderer.h>
#include <RendererMaterial.h>
#include <RendererMaterialDesc.h>
#include <RendererMaterialInstance.h>

#include <SampleAssetManager.h>
#include <SampleTextureAsset.h>

#include "SampleXml.h"
#include <ctype.h>

using namespace SampleFramework;

static SampleRenderer::RendererMaterial::BlendFunc 
getBlendFunc(const char* nodeValue, 
			 SampleRenderer::RendererMaterial::BlendFunc defaultBlendFunc = SampleRenderer::RendererMaterial::BLEND_ONE)
{
	using SampleRenderer::RendererMaterial;
	RendererMaterial::BlendFunc blendFunc = defaultBlendFunc;
	if(     !strcmp(nodeValue, "ZERO"))                 blendFunc = RendererMaterial::BLEND_ZERO;
	else if(!strcmp(nodeValue, "ONE"))                  blendFunc = RendererMaterial::BLEND_ONE;
	else if(!strcmp(nodeValue, "SRC_COLOR"))            blendFunc = RendererMaterial::BLEND_SRC_COLOR;
	else if(!strcmp(nodeValue, "ONE_MINUS_SRC_COLOR"))  blendFunc = RendererMaterial::BLEND_ONE_MINUS_SRC_COLOR;
	else if(!strcmp(nodeValue, "SRC_ALPHA"))            blendFunc = RendererMaterial::BLEND_SRC_ALPHA;
	else if(!strcmp(nodeValue, "ONE_MINUS_SRC_ALPHA"))  blendFunc = RendererMaterial::BLEND_ONE_MINUS_SRC_ALPHA;
	else if(!strcmp(nodeValue, "DST_COLOR"))            blendFunc = RendererMaterial::BLEND_DST_COLOR;
	else if(!strcmp(nodeValue, "ONE_MINUS_DST_COLOR"))  blendFunc = RendererMaterial::BLEND_ONE_MINUS_DST_COLOR;
	else if(!strcmp(nodeValue, "DST_ALPHA"))            blendFunc = RendererMaterial::BLEND_DST_ALPHA;
	else if(!strcmp(nodeValue, "ONE_MINUS_DST_ALPHA"))  blendFunc = RendererMaterial::BLEND_ONE_MINUS_DST_ALPHA;
	else if(!strcmp(nodeValue, "SRC_ALPHA_SATURATE"))   blendFunc = RendererMaterial::BLEND_SRC_ALPHA_SATURATE;
	else PX_ASSERT(0); // Unknown blend func!

	return blendFunc;
}

static void readFloats(const char *str, float *floats, unsigned int numFloats)
{
	PxU32 fcount = 0;
	while(*str && !((*str>='0'&&*str<='9') || *str=='.')) str++;
	for(PxU32 i=0; i<numFloats; i++)
	{
		if(*str)
		{
			floats[i] = (float)atof(str);
			while(*str &&  ((*str>='0'&&*str<='9') || *str=='.')) str++;
			while(*str && !((*str>='0'&&*str<='9') || *str=='.')) str++;
			fcount++;
		}
	}
	PX_ASSERT(fcount==numFloats);
}

SampleMaterialAsset::SampleMaterialAsset(SampleAssetManager &assetManager, FAST_XML::xml_node &xmlroot, const char *path) :
SampleAsset(ASSET_MATERIAL, path),
	m_assetManager(assetManager)
{

	std::vector<const char*> mVertexShaderPaths;
	//m_material         = 0;
	//m_materialInstance = 0;

	SampleRenderer::Renderer &renderer = assetManager.getRenderer();

	SampleRenderer::RendererMaterialDesc matdesc;
	const char *materialTypeName = xmlroot.getXMLAttribute("type");
	if(materialTypeName && !strcmp(materialTypeName, "lit"))
	{
		matdesc.type = SampleRenderer::RendererMaterial::TYPE_LIT;
	}
	else if(materialTypeName && !strcmp(materialTypeName, "unlit"))
	{
		matdesc.type = SampleRenderer::RendererMaterial::TYPE_UNLIT;
	}
	for(FAST_XML::xml_node *child=xmlroot.first_node(); child; child=child->next_sibling())
	{
		const char *nodeName  = child->name();
		const char *nodeValue = child->value();
		const char *name      = child->getXMLAttribute("name");
		if(nodeName && nodeValue)
		{
			while(isspace(*nodeValue)) nodeValue++; // skip leading spaces.
			if(!strcmp(nodeName, "shader"))
			{
				if(name && !strcmp(name, "vertex"))
				{
					//matdesc.vertexShaderPath = nodeValue;
					mVertexShaderPaths.push_back(nodeValue);
				}
				else if(name && !strcmp(name, "fragment"))
				{
					matdesc.fragmentShaderPath = nodeValue;
				}
				else if(name && !strcmp(name, "geometry"))
				{
					matdesc.geometryShaderPath = nodeValue;
				}
				else if(name && !strcmp(name, "hull"))
				{
					matdesc.hullShaderPath = nodeValue;
				}
				else if(name && !strcmp(name, "domain"))
				{
					matdesc.domainShaderPath = nodeValue;
				}
			}
			else if(!strcmp(nodeName, "alphaTestFunc"))
			{
				if(     !strcmp(nodeValue, "EQUAL"))         matdesc.alphaTestFunc = SampleRenderer::RendererMaterial::ALPHA_TEST_EQUAL;
				else if(!strcmp(nodeValue, "NOT_EQUAL"))     matdesc.alphaTestFunc = SampleRenderer::RendererMaterial::ALPHA_TEST_NOT_EQUAL;
				else if(!strcmp(nodeValue, "LESS"))          matdesc.alphaTestFunc = SampleRenderer::RendererMaterial::ALPHA_TEST_LESS;
				else if(!strcmp(nodeValue, "LESS_EQUAL"))    matdesc.alphaTestFunc = SampleRenderer::RendererMaterial::ALPHA_TEST_LESS_EQUAL;
				else if(!strcmp(nodeValue, "GREATER"))       matdesc.alphaTestFunc = SampleRenderer::RendererMaterial::ALPHA_TEST_GREATER;
				else if(!strcmp(nodeValue, "GREATER_EQUAL")) matdesc.alphaTestFunc = SampleRenderer::RendererMaterial::ALPHA_TEST_GREATER_EQUAL;
				else PX_ASSERT(0); // Unknown alpha test func!
			}
			else if(!strcmp(nodeName, "alphaTestRef"))
			{
				matdesc.alphaTestRef = PxClamp((float)atof(nodeValue), 0.0f, 1.0f);
			}
			else if(!strcmp(nodeName, "blending") && strstr(nodeValue, "true"))
			{
				matdesc.blending = true;

				static const PxU32 numBlendFuncs = 2;
				static const char* blendFuncNames[numBlendFuncs] = 
				{
					"srcBlendFunc", 
					"dstBlendFunc"
				};
				static const SampleRenderer::RendererMaterial::BlendFunc blendFuncDefaults[numBlendFuncs] =
				{
					SampleRenderer::RendererMaterial::BLEND_SRC_ALPHA,
					SampleRenderer::RendererMaterial::BLEND_ONE_MINUS_SRC_ALPHA
				};
				SampleRenderer::RendererMaterial::BlendFunc* blendFuncs[numBlendFuncs] = 
				{
					&matdesc.srcBlendFunc, 
					&matdesc.dstBlendFunc
				};

				// Parse optional src/dst blend funcs
				for (PxU32 i = 0; i < numBlendFuncs; ++i)
				{
					FAST_XML::xml_node *blendNode = child->first_node(blendFuncNames[i]);
					if (blendNode && blendNode->value())
						*blendFuncs[i] = getBlendFunc(blendNode->value(), blendFuncDefaults[i]);
					else
						*blendFuncs[i] = blendFuncDefaults[i];
				}
			}
		}
	}

	for (size_t materialIndex = 0; materialIndex < mVertexShaderPaths.size(); materialIndex++)
	{
		MaterialStruct materialStruct;

		matdesc.vertexShaderPath = mVertexShaderPaths[materialIndex];
		materialStruct.m_material = NULL;
		materialStruct.m_materialInstance = NULL;
		materialStruct.m_maxBones = 0;
		if (strstr(mVertexShaderPaths[materialIndex], "skeletalmesh") != NULL)
			materialStruct.m_maxBones = RENDERER_MAX_BONES;

		materialStruct.m_material = renderer.createMaterial(matdesc);
		PX_ASSERT(materialStruct.m_material);
		if(materialStruct.m_material)
		{
			FAST_XML::xml_node *varsnode = xmlroot.first_node("variables");
			if(varsnode)
			{
				materialStruct.m_materialInstance = new SampleRenderer::RendererMaterialInstance(*materialStruct.m_material);
				for(FAST_XML::xml_node *child=varsnode->first_node(); child; child=child->next_sibling())
				{
					const char *nodename = child->name();
					const char *varname  = child->getXMLAttribute("name");
					const char *value    = child->value();

					if(!strcmp(nodename, "float"))
					{
						float f = (float)atof(value);
						const SampleRenderer::RendererMaterial::Variable *var = materialStruct.m_materialInstance->findVariable(varname, SampleRenderer::RendererMaterial::VARIABLE_FLOAT);
						//PX_ASSERT(var);
						if(var) materialStruct.m_materialInstance->writeData(*var, &f);
					}
					else if(!strcmp(nodename, "float2"))
					{
						float f[2];
						readFloats(value, f, 2);
						const SampleRenderer::RendererMaterial::Variable *var = materialStruct.m_materialInstance->findVariable(varname, SampleRenderer::RendererMaterial::VARIABLE_FLOAT2);
						//PX_ASSERT(var);
						if(var) materialStruct.m_materialInstance->writeData(*var, f);
					}
					else if(!strcmp(nodename, "float3"))
					{
						float f[3];
						readFloats(value, f, 3);
						const SampleRenderer::RendererMaterial::Variable *var = materialStruct.m_materialInstance->findVariable(varname, SampleRenderer::RendererMaterial::VARIABLE_FLOAT3);
						//PX_ASSERT(var);
						if(var) materialStruct.m_materialInstance->writeData(*var, f);
					}
					else if(!strcmp(nodename, "float4"))
					{
						float f[4];
						readFloats(value, f, 4);
						const SampleRenderer::RendererMaterial::Variable *var = materialStruct.m_materialInstance->findVariable(varname, SampleRenderer::RendererMaterial::VARIABLE_FLOAT4);
						//PX_ASSERT(var);
						if(var) materialStruct.m_materialInstance->writeData(*var, f);
					}
					else if(!strcmp(nodename, "sampler2D") || !strcmp(nodename, "sampler3D"))
					{
						SampleTextureAsset *textureAsset = static_cast<SampleTextureAsset*>(assetManager.getAsset(value, ASSET_TEXTURE));
						PX_ASSERT(textureAsset);
						if(textureAsset)
						{
							m_assets.push_back(textureAsset);
							SampleRenderer::RendererMaterial::VariableType vartype = (0 == strcmp(nodename, "sampler2D")) ? SampleRenderer::RendererMaterial::VARIABLE_SAMPLER2D : SampleRenderer::RendererMaterial::VARIABLE_SAMPLER3D;
							const SampleRenderer::RendererMaterial::Variable *var  = materialStruct.m_materialInstance->findVariable(varname, vartype);
							//PX_ASSERT(var);
							if(var)
							{
								SampleRenderer::RendererTexture *texture = textureAsset->getTexture();
								materialStruct.m_materialInstance->writeData(*var, &texture);
							}
						}
					}

				}
			}

			m_vertexShaders.push_back(materialStruct);
		}
	}
}

SampleFramework::SampleMaterialAsset::SampleMaterialAsset(SampleAssetManager &assetManager, Type type, const char *path)
: SampleAsset(type, path),
  m_assetManager(assetManager)
{

}

SampleMaterialAsset::~SampleMaterialAsset(void)
{
	PxU32 numAssets = (PxU32)m_assets.size();
	for(PxU32 i=0; i<numAssets; i++)
	{
		m_assetManager.returnAsset(*m_assets[i]);
	}
	for (size_t index = 0; index < m_vertexShaders.size(); index++)
	{
		if(m_vertexShaders[index].m_materialInstance) delete m_vertexShaders[index].m_materialInstance;
		if(m_vertexShaders[index].m_material)         m_vertexShaders[index].m_material->release();
	}
}

size_t SampleMaterialAsset::getNumVertexShaders() const
{
	return m_vertexShaders.size();
}

SampleRenderer::RendererMaterial *SampleMaterialAsset::getMaterial(size_t vertexShaderIndex)
{
	return m_vertexShaders[vertexShaderIndex].m_material;
}

SampleRenderer::RendererMaterialInstance *SampleMaterialAsset::getMaterialInstance(size_t vertexShaderIndex)
{
	return m_vertexShaders[vertexShaderIndex].m_materialInstance;
}

bool SampleMaterialAsset::isOk(void) const
{
	return !m_vertexShaders.empty();
}

unsigned int SampleMaterialAsset::getMaxBones(size_t vertexShaderIndex) const
{
	return m_vertexShaders[vertexShaderIndex].m_maxBones;
}
