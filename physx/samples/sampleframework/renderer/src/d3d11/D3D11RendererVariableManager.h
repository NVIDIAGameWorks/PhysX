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
#ifndef D3D11_RENDERER_VARIABLE_MANAGER_H
#define D3D11_RENDERER_VARIABLE_MANAGER_H

#include <RendererConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D11)

#include <RendererMaterial.h>

#include "D3D11RendererMaterial.h"
#include "D3D11RendererTraits.h"
#include "D3D11RendererUtils.h"
#include "D3Dcompiler.h"

#include <set>
#include <string>
#include <limits>

// Enable to check that binding a shared variable by name actually
//   finds the specified shared variable 
#define RENDERER_ASSERT_SHARED_VARIABLE_EXISTS 0

namespace SampleRenderer
{

static const PxU32 NUM_SHADER_TYPES = D3DTypes::NUM_SHADER_TYPES + RendererMaterial::NUM_PASSES;

class D3D11RendererVariableManager
{
public:
	enum SharedVariableSize
	{
		USE_DEFAULT = 0,
	};

	enum BindMode
	{
		BIND_MAP = 0,
		BIND_SUBRESOURCE
	};

public:
	typedef std::set<std::string> StringSet;

	D3D11RendererVariableManager(D3D11Renderer& renderer, StringSet& cbNames, BindMode bindMode = BIND_SUBRESOURCE);
	virtual ~D3D11RendererVariableManager(void);

public:
	void bind(const void* pResource, D3DType shaderType, RendererMaterial::Pass pass = RendererMaterial::NUM_PASSES) const;
	void setSharedVariable(const char* sharedBufferName, const char* variableName, const void* data, UINT size = USE_DEFAULT, UINT offset = 0);
	void loadVariables(D3D11RendererMaterial* pMaterial, ID3DBlob* pShader, D3DType shaderType, RendererMaterial::Pass pass = RendererMaterial::NUM_PASSES);
	void loadSharedVariables(const void* pResource, ID3DBlob* pShader, D3DType shaderType, RendererMaterial::Pass pass = RendererMaterial::NUM_PASSES);
	void unloadVariables(const void* pResource);

	class D3D11ConstantBuffer;
	class D3D11DataVariable;
	class D3D11TextureVariable;
	class D3D11SharedVariable;

	typedef std::vector<D3D11ConstantBuffer*> ConstantBuffers;
	typedef std::vector<ID3D11Buffer*>        D3DBuffers;
	typedef std::vector<D3D11SharedVariable*> Variables;
	typedef std::vector<D3D11RendererMaterial::Variable*> MaterialVariables;

	typedef D3D11StringKey                          StringKey;
	typedef const void*                             ResourceKey;
	typedef PxU32                                   ShaderTypeKey;
	typedef std::pair<StringKey,     StringKey>     VariableKey;

	typedef std::map<StringKey,   D3D11ConstantBuffer*>  NameBuffersMap;
	typedef std::map<VariableKey, D3D11SharedVariable*>  NameVariablesMap;
	typedef std::map<ResourceKey, ConstantBuffers>       ResourceBuffersMap;

	typedef ConstantBuffers::const_iterator CBIterator;

private:
	D3D11RendererVariableManager& operator=(const D3D11RendererVariableManager&)
	{
		return *this;
	}

	D3D11ConstantBuffer* loadBuffer(MaterialVariables& variables,
	                                PxU32& variableBufferSize,
	                                ShaderTypeKey typeKey,
	                                ID3D11ShaderReflectionConstantBuffer* pReflectionBuffer,
	                                const D3D11_SHADER_BUFFER_DESC& sbDesc,
	                                const D3D11_BUFFER_DESC& cbDesc);
	D3D11ConstantBuffer* loadSharedBuffer(ShaderTypeKey typeKey,
	                                      ID3D11ShaderReflectionConstantBuffer* pReflectionBuffer,
	                                      const D3D11_SHADER_BUFFER_DESC& sbDesc,
	                                      const D3D11_BUFFER_DESC& cbDesc);

	void loadConstantVariables(const void* pResource,
	                           ID3DBlob* pShader,
	                           ShaderTypeKey typeKey,
	                           ID3D11ShaderReflection* pReflection,
	                           MaterialVariables* pVariables = NULL,
	                           PxU32* pVariableBufferSize    = NULL);
	void loadTextureVariables(D3D11RendererMaterial* pMaterial,
	                          ID3DBlob* pShader,
	                          ShaderTypeKey typeKey,
	                          ID3D11ShaderReflection* pReflection);

	void internalSetVariable(D3D11ConstantBuffer* pBuffer, PxU32 offset, const void* data, PxU32 size);
	void updateVariables(const ConstantBuffers*) const;
	void bindVariables(const ConstantBuffers*, bool bFragment) const;

private:

	D3D11Renderer&         mRenderer;
	StringSet              mSharedBufferNames;

	BindMode               mBindMode;

	Variables              mVariables;

	NameBuffersMap         mNameToSharedBuffer;
	NameVariablesMap       mNameToSharedVariables;

	ResourceBuffersMap     mResourceToBuffers[NUM_SHADER_TYPES];
};

} // namespace SampleRenderer

#endif // #if defined(RENDERER_ENABLE_DIRECT3D11)
#endif
