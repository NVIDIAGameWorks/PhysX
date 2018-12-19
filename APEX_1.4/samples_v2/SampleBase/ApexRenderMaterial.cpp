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


#include "ApexRenderMaterial.h"
#include <DirectXMath.h>
#include "ApexResourceCallback.h"
#include "Utils.h"
#include "PsFastXml.h"

#include "PsFileBuffer.h"
#include "PxInputDataFromPxFileBuf.h"
#include "nvparameterized/NvParamUtils.h"


const char* DEFAULT_SPRITE_PARTICLE_SHADER = "pointsprite.hlsl";
const char* DEFAULT_MESH_PARTICLE_SHADER = "pointsprite.hlsl";

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											Material .xml files parser
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class MaterialXmlParser : public nvidia::shdfnd::FastXml::Callback
{
public:
	MaterialXmlParser() :
		blendMode(ApexRenderMaterial::BLEND_NONE)
	{
	}
	virtual ~MaterialXmlParser()
	{
	}

	string textureFile;
	vector<string> shaderFiles;
	ApexRenderMaterial::BlendMode blendMode;

protected:
	// encountered a comment in the XML
	virtual bool processComment(const char* /*comment*/)
	{
		return true;
	}

	virtual bool processClose(const char* /*element*/, unsigned int /*depth*/, bool& /*isError*/)
	{
		return true;
	}

	// return true to continue processing the XML document, false to skip.
	virtual bool processElement(const char* elementName, // name of the element
		const char* elementData, // element data, null if none
		const nvidia::shdfnd::FastXml::AttributePairs& attr,
		int /*lineno*/) // line number in the source XML file
	{
		PX_UNUSED(attr);
		if (::strcmp(elementName, "texture") == 0)
		{
			textureFile = elementData;
		}
		else if (::strcmp(elementName, "shader") == 0)
		{
			shaderFiles.push_back(elementData);
		}
		else if (::strcmp(elementName, "blending") == 0)
		{
			blendMode = (::stricmp(elementData, "additive") == 0) ? ApexRenderMaterial::BLEND_ADDITIVE :
				ApexRenderMaterial::BLEND_ALPHA_BLENDING;
		}

		return true;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												ApexRenderMaterial
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ApexRenderMaterial::ApexRenderMaterial(ApexResourceCallback* resourceProvider, const char* xmlFilePath)
{
	PsFileBuffer theBuffer(xmlFilePath, nvidia::general_PxIOStream2::PxFileBuf::OPEN_READ_ONLY);
	PxInputDataFromPxFileBuf id(theBuffer);
	MaterialXmlParser parser;
	nvidia::shdfnd::FastXml* xml = nvidia::shdfnd::createFastXml(&parser);
	xml->processXml(id, false);

	this->initialize(resourceProvider, parser.shaderFiles, parser.textureFile.c_str(), parser.blendMode);

	xml->release();
}

ApexRenderMaterial::ApexRenderMaterial(ApexResourceCallback* resourceCallback, const NvParameterized::Interface *graphicMaterialData)
{
	const char* textureFileName = NULL;
	NvParameterized::getParamString(*graphicMaterialData, "DiffuseTexture", textureFileName);

	BlendMode blendMode = BLEND_NONE;

	const char *technique = "SPRITE_ONE";
	NvParameterized::getParamEnum(*graphicMaterialData, "RenderTechnique", technique);
	if (technique)
	{
		if (::strcmp(technique, "SPRITE_ONE") == 0)
		{
			blendMode = BLEND_ADDITIVE;
		}
		else if (::strcmp(technique, "SPRITE_ALPHA") == 0)
		{
			blendMode = BLEND_ALPHA_BLENDING;
		}
	}

	this->initialize(resourceCallback, blendMode == BLEND_NONE ? DEFAULT_MESH_PARTICLE_SHADER : DEFAULT_SPRITE_PARTICLE_SHADER, textureFileName, blendMode);
}

ApexRenderMaterial::ApexRenderMaterial(ApexResourceCallback* resourceCallback, const char* shaderFileName,
	const char* textureFileName, BlendMode blendMode)
{
	this->initialize(resourceCallback, shaderFileName, textureFileName, blendMode);
}

void ApexRenderMaterial::initialize(ApexResourceCallback* resourceCallback, const char* shaderFileName, const char* textureFileName, BlendMode blendMode)
{
	vector<string> v;
	v.push_back(shaderFileName);
	initialize(resourceCallback, v, textureFileName, blendMode);
}

void ApexRenderMaterial::initialize(ApexResourceCallback* resourceCallback, vector<string> shaderFileNames, const char* textureFileName, BlendMode blendMode)
{
	mTextureSRV = nullptr;
	mTexture = nullptr;
	mBlendState = nullptr;
	mTextureFileName = textureFileName;

	for (uint32_t i = 0; i < shaderFileNames.size(); i++)
	{
		string shaderFilePath = ((char*)resourceCallback->requestResourceCustom(ApexResourceCallback::eSHADER_FILE_PATH, shaderFileNames[i].c_str()));
		mShaderFilePathes.push_back(shaderFilePath);
	}
	mShaderGroups.reserve(mShaderFilePathes.size());

	if (!mTextureFileName.empty())
	{
		mTexture =
			(TextureResource*)resourceCallback->requestResourceCustom(ApexResourceCallback::eTEXTURE_RESOURCE, mTextureFileName.c_str());
	}

	setBlending(blendMode);

	reload();
}

void ApexRenderMaterial::releaseReloadableResources()
{
	for (vector<ShaderGroup*>::iterator it = mShaderGroups.begin(); it != mShaderGroups.end(); it++)
	{
		delete *it;
	}
	mShaderGroups.clear();

	SAFE_RELEASE(mTextureSRV);
}

ApexRenderMaterial::~ApexRenderMaterial()
{
	releaseReloadableResources();
	SAFE_RELEASE(mBlendState);

	for (list<Instance*>::iterator it = mInstances.begin(); it != mInstances.end(); it++)
	{
		delete *it;
	}
	mInstances.clear();
}

void ApexRenderMaterial::setBlending(BlendMode blendMode)
{
	SAFE_RELEASE(mBlendState);

	D3D11_BLEND_DESC desc;
	ZeroMemory(&desc, sizeof(desc));

	switch (blendMode)
	{
	case BLEND_NONE:
		desc.RenderTarget[0].BlendEnable = FALSE;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		break;
	case BLEND_ALPHA_BLENDING:
		desc.AlphaToCoverageEnable = FALSE;
		desc.IndependentBlendEnable = TRUE;
		desc.RenderTarget[0].BlendEnable = TRUE;
		desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		break;
	case BLEND_ADDITIVE: // actually, is's additive by alpha
		desc.AlphaToCoverageEnable = FALSE;
		desc.IndependentBlendEnable = TRUE;
		desc.RenderTarget[0].BlendEnable = TRUE;
		desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		break;
	default:
		PX_ALWAYS_ASSERT_MESSAGE("Unknown blend mode");
	}

	ID3D11Device* device = GetDeviceManager()->GetDevice();
	V(device->CreateBlendState(&desc, &mBlendState));
}

void ApexRenderMaterial::reload()
{
	releaseReloadableResources();

	// load shaders
	ID3D11Device* device = GetDeviceManager()->GetDevice();

	for (vector<string>::iterator it = mShaderFilePathes.begin(); it != mShaderFilePathes.end(); it++)
	{
		const char* shaderFilePath = (*it).c_str();
		ShaderGroup* shaderGroup = new ShaderGroup();
		V(createShaderFromFile(device, shaderFilePath, "VS", &(shaderGroup->vs), shaderGroup->buffer));
		V(createShaderFromFile(device, shaderFilePath, "PS", &shaderGroup->ps));
		createShaderFromFile(device, shaderFilePath, "GS", &shaderGroup->gs);
		mShaderGroups.push_back(shaderGroup);
	}

	// load texture
	if (mTexture)
	{
		V(DirectX::CreateShaderResourceView(device, mTexture->image.GetImages(), mTexture->image.GetImageCount(),
		                                    mTexture->metaData, &mTextureSRV));
	}
}

ApexRenderMaterial::Instance* ApexRenderMaterial::getMaterialInstance(const D3D11_INPUT_ELEMENT_DESC* elementDescs, uint32_t numElements)
{
	ID3D11InputLayout* inputLayout = NULL;
	ID3D11Device* device = GetDeviceManager()->GetDevice();

	for (uint32_t i = 0; i < mShaderGroups.size(); i++)
	{
		if (mShaderGroups[i]->buffer == NULL)
			continue;

		device->CreateInputLayout(elementDescs, numElements, mShaderGroups[i]->buffer->GetBufferPointer(), mShaderGroups[i]->buffer->GetBufferSize(), &inputLayout);

		if (inputLayout)
		{
			Instance* materialInstance = new Instance(*this, inputLayout, i);
			mInstances.push_back(materialInstance);
			return materialInstance;
		}
	}
	PX_ALWAYS_ASSERT();
	return NULL;
}

void ApexRenderMaterial::Instance::bind(ID3D11DeviceContext& context, uint32_t slot)
{
	mMaterial.mShaderGroups[mShaderNum]->Set(&context);

	context.OMSetBlendState(mMaterial.mBlendState, nullptr, 0xFFFFFFFF);
	context.PSSetShaderResources(slot, 1, &(mMaterial.mTextureSRV));
	context.IASetInputLayout(mInputLayout);
}

bool ApexRenderMaterial::Instance::isValid()
{
	return mMaterial.mShaderGroups.size() > 0 && mMaterial.mShaderGroups[mShaderNum]->IsValid();
}
