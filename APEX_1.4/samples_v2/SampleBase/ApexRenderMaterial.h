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


#ifndef APEX_RENDER_MATERIAL_H
#define APEX_RENDER_MATERIAL_H

#include "Apex.h"
#include "Utils.h"
#include "DirectXTex.h"

#pragma warning(push)
#pragma warning(disable : 4350)
#include <string>
#include <vector>
#include <list>
#pragma warning(pop)

#pragma warning(disable : 4512)

using namespace nvidia;
using namespace nvidia::apex;
using namespace std;

class ApexResourceCallback;

struct TextureResource
{
	DirectX::TexMetadata metaData;
	DirectX::ScratchImage image;
};

class ApexRenderMaterial
{
  public:

	enum BlendMode
	{
		BLEND_NONE,
		BLEND_ALPHA_BLENDING,
		BLEND_ADDITIVE
	};

    ApexRenderMaterial(ApexResourceCallback* resourceProvider, const char* xmlFilePath);
	ApexRenderMaterial(ApexResourceCallback* resourceProvider, const NvParameterized::Interface *graphicMaterialData);
	ApexRenderMaterial(ApexResourceCallback* resourceProvider, const char* shaderFileName, const char* textureFileName, BlendMode blendMode = BLEND_NONE);
	~ApexRenderMaterial();

	void setBlending(BlendMode blendMode);
	void reload();

	class Instance
	{
	public:
		Instance(ApexRenderMaterial& material, ID3D11InputLayout* inputLayout, uint32_t shaderNum = 0) : mMaterial(material), mInputLayout(inputLayout), mShaderNum(shaderNum) {}
		~Instance() { SAFE_RELEASE(mInputLayout); }

		bool isValid();
		void bind(ID3D11DeviceContext& context, uint32_t slot);
	private:
		ApexRenderMaterial& mMaterial;
		ID3D11InputLayout* mInputLayout;
		uint32_t mShaderNum;
	};

	Instance* getMaterialInstance(const D3D11_INPUT_ELEMENT_DESC* elementDescs, uint32_t numElements);

  private:
	void initialize(ApexResourceCallback* resourceCallback, const char* shaderFileName, const char* textureFileName, BlendMode blendMode);
	void initialize(ApexResourceCallback* resourceProvider, vector<string> shaderFileNames, const char* textureFileName, BlendMode blendMode);

	void releaseReloadableResources();

	string mShaderFileName;
	string mTextureFileName;

	struct ShaderGroup
	{
		ShaderGroup() : vs(NULL), gs(NULL), ps(NULL)
		{
		}
		~ShaderGroup()
		{
			Release();
		}
		void Release()
		{
			SAFE_RELEASE(vs);
			SAFE_RELEASE(gs);
			SAFE_RELEASE(ps);
			SAFE_RELEASE(buffer);
		}
		void Set(ID3D11DeviceContext* c)
		{
			c->VSSetShader(vs, nullptr, 0);
			c->GSSetShader(gs, nullptr, 0);
			c->PSSetShader(ps, nullptr, 0);
		}
		bool IsValid()
		{
			return vs != NULL && ps != NULL;
		}
		ID3D11VertexShader* vs;
		ID3D11GeometryShader* gs;
		ID3D11PixelShader* ps;
		ID3DBlob* buffer;
	};

	list<Instance*> mInstances;
	TextureResource* mTexture;
	ID3D11ShaderResourceView* mTextureSRV;
	ID3D11BlendState* mBlendState;
	vector<string> mShaderFilePathes;
	vector<ShaderGroup*> mShaderGroups;
};

#endif