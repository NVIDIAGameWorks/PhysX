// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
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
