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

#include "D3D11RendererVariableManager.h"

#include "D3D11Renderer.h"
#include "D3D11RendererTexture2D.h"
#include "D3D11RendererTexture3D.h"
#include "D3D11RendererMemoryMacros.h"
#include "D3D11RendererUtils.h"

#include "RendererMemoryMacros.h"

#include <stdio.h>

using namespace SampleRenderer;
static const PxU32 getShaderTypeKey(D3DType shaderType, RendererMaterial::Pass pass = RendererMaterial::NUM_PASSES)
{
	RENDERER_ASSERT(shaderType < D3DTypes::NUM_SHADER_TYPES && 
		           ((shaderType > 0 && pass == RendererMaterial::NUM_PASSES) ||
				   (shaderType == 0 && pass < RendererMaterial::NUM_PASSES)), 
				   "Invalid shader index"); 
	// pass       = NUM_PASSES for any non-pixel shader
	// shaderType = 0 for any pixel shader
	// pass + shaderType = UNIQUE for any combination
	return (PxU32)shaderType + (PxU32)pass;
}

static D3DType getD3DType(PxU32 shaderTypeKey)
{
	return D3DType(shaderTypeKey < RendererMaterial::NUM_PASSES ? D3DTypes::SHADER_PIXEL : shaderTypeKey-RendererMaterial::NUM_PASSES);
}

class D3D11RendererVariableManager::D3D11ConstantBuffer
{
public:
	D3D11ConstantBuffer(ID3D11Buffer* pBuffer = NULL, PxU8* pData = NULL, PxU32 size = 0)
		: buffer(pBuffer),
		data(pData),
		dataSize(size),
		bDirty(false),
		refCount(1)
	{ 
	
	}

	~D3D11ConstantBuffer()
	{
		RENDERER_ASSERT(refCount == 0, "Constant buffer not released as often as it was created.");
		dxSafeRelease(buffer);
		DELETEARRAY(data);
	}

	void addref() 
	{
		++refCount; 
	}

	void release()
	{
		if(0 == --refCount)
		{
			delete this;
		}
	}

	ID3D11Buffer* buffer;
	PxU8*         data;
	PxU32         dataSize;
	mutable bool  bDirty;

protected:
	PxU32         refCount;
};

/****************************************************
* D3D11RendererVariableManager::D3D11SharedVariable *
****************************************************/

class D3D11RendererVariableManager::D3D11SharedVariable : public D3D11RendererMaterial::D3D11BaseVariable
{
	friend class D3D11RendererMaterial;
	friend class D3D11RendererVariableManager;
public:
	D3D11SharedVariable(const char* name, RendererMaterial::VariableType type, PxU32 offset, D3D11ConstantBuffer* pBuffer)
		: D3D11RendererMaterial::D3D11BaseVariable(name, type, offset), m_pBuffer(pBuffer) 
	{
	}
	D3D11SharedVariable& operator=(const D3D11SharedVariable&)
	{
		return *this;
	}
protected:
	D3D11ConstantBuffer* m_pBuffer;
};

/**************************************************
* D3D11RendererVariableManager::D3D11DataVariable *
**************************************************/

class D3D11RendererVariableManager::D3D11DataVariable : public D3D11RendererMaterial::D3D11Variable
{
	friend class D3D11RendererMaterial;
	friend class D3D11RendererVariableManager;
public:
	D3D11DataVariable(const char* name, RendererMaterial::VariableType type, PxU32 offset)
		: D3D11Variable(name, type, offset) 
	{
		for (int i = 0; i < NUM_SHADER_TYPES; ++i)
		{
			m_pData[i] = NULL;
			m_pDirtyFlags[i] = NULL;
		}
	}

	virtual void bind(RendererMaterial::Pass pass, const void* data)
	{
		for (PxU32 i = getShaderTypeKey(D3DTypes::SHADER_VERTEX); i < NUM_SHADER_TYPES; ++i)
		{
			internalBind(i, data);
		}
		internalBind(pass, data);
	}

private:

	void internalBind(PxU32 typeKey, const void* data)
	{
		if (m_pData[typeKey])
		{
			memcpy(m_pData[typeKey], data, getDataSize());
			if (m_pDirtyFlags[typeKey])
			{
				*(m_pDirtyFlags[typeKey]) = true;
			}
		}
	}

	void addHandle(D3D11RendererVariableManager::ShaderTypeKey typeKey, PxU8* pData, bool* pDirtyFlag)
	{
		m_pData[typeKey] = pData;
		m_pDirtyFlags[typeKey] = pDirtyFlag;
	}

private:
	PxU8* m_pData[NUM_SHADER_TYPES];
	bool* m_pDirtyFlags[NUM_SHADER_TYPES];
};


/*****************************************************
* D3D11RendererVariableManager::D3D11TextureVariable *
*****************************************************/

class D3D11RendererVariableManager::D3D11TextureVariable : public D3D11RendererMaterial::D3D11Variable
{
	friend class D3D11RendererMaterial;
	friend class D3D11RendererVariableManager;
public:
	D3D11TextureVariable(const char* name, RendererMaterial::VariableType type, PxU32 offset)
		: D3D11Variable(name, type, offset)
	{
		for (int i = 0; i < NUM_SHADER_TYPES; ++i)
		{
			m_bufferIndex[i] = -1;
		}
	}


	virtual void bind(RendererMaterial::Pass pass, const void* data)
	{
		data = *(void**)data;
		//RENDERER_ASSERT(data, "NULL Sampler.");
		if (data)
		{
			for (PxU32 i = getShaderTypeKey(D3DTypes::SHADER_VERTEX); i < NUM_SHADER_TYPES; ++i)
			{
				internalBind(i, data);
			}
			internalBind(pass, data);
		}
	}

private:

	void internalBind(PxU32 typeKey, const void* data)
	{
		if (m_bufferIndex[typeKey] != -1)
		{
			PxU32 bindFlags = getBindFlags(getD3DType(typeKey));
			if (RendererMaterial::VARIABLE_SAMPLER2D == getType())
				static_cast<D3D11RendererTexture2D*>((RendererTexture2D*)data)->bind(m_bufferIndex[typeKey], bindFlags);
			else if (RendererMaterial::VARIABLE_SAMPLER3D == getType())
				static_cast<D3D11RendererTexture3D*>((RendererTexture2D*)data)->bind(m_bufferIndex[typeKey], bindFlags);
		}
	}

	void addHandle(D3D11RendererVariableManager::ShaderTypeKey typeKey, int bufferIndex)
	{
		m_bufferIndex[typeKey] = bufferIndex;
	}

private:
	int  m_bufferIndex[NUM_SHADER_TYPES];
};

/*******************************
* D3D11RendererVariableManager *
*******************************/

D3D11RendererVariableManager::D3D11RendererVariableManager(D3D11Renderer& renderer, StringSet& cbNames, BindMode bindMode)
	: mRenderer(renderer), mSharedBufferNames(cbNames), mBindMode(bindMode)
{

}

D3D11RendererVariableManager::~D3D11RendererVariableManager(void)
{
	while (!mVariables.empty())
	{
		delete mVariables.back(), mVariables.pop_back();
	}

	for (PxU32 i = 0; i < NUM_SHADER_TYPES; ++i)
	{
		for (ResourceBuffersMap::iterator it  = mResourceToBuffers[i].begin();
			it != mResourceToBuffers[i].end();
			++it)
		{
			for(PxU32 j = 0; j < it->second.size(); ++j)
			{
				it->second[j]->release();
			}
		}
	}
}

class D3D11RendererResourceMapper
{
public:
	D3D11RendererResourceMapper( ID3D11DeviceContext *pContext, ID3D11Resource *pResource, UINT Subresource = 0, D3D11_MAP MapType = D3D11_MAP_WRITE_DISCARD )
		: mpContext(pContext), mpResource(pResource), mSubresource(Subresource)
	{ 
		pContext->Map(pResource, Subresource, MapType, NULL, &mMappedResource);
	}

	void load(const void* pSrc, size_t srcSize)
	{
		RENDERER_ASSERT(mMappedResource.pData, "Invalid D3D11 mapped pointer");
		memcpy(mMappedResource.pData, pSrc, srcSize);
	}

	~D3D11RendererResourceMapper()
	{
		mpContext->Unmap(mpResource, mSubresource);
	}

protected:
	ID3D11DeviceContext*     mpContext;
	ID3D11Resource*          mpResource;
	UINT                     mSubresource;
	D3D11_MAPPED_SUBRESOURCE mMappedResource;
};

class D3D11RendererVariableBinder
{
public:
	D3D11RendererVariableBinder(ID3D11DeviceContext* pContext)
		: mContext(pContext)
	{
		RENDERER_ASSERT(pContext, "Invalid D3D11 device context");
	}
	
	void bind(const D3DType d3dType,
			  const D3D11RendererVariableManager::ConstantBuffers* buffers,
			  D3D11RendererVariableManager::BindMode bindMode)
	{
		if (buffers && buffers->size() > 0)
		{
			D3D11RendererVariableManager::D3DBuffers pCBs(buffers->size());
			PxU32 cbIndex = 0;
			for (D3D11RendererVariableManager::CBIterator cbIt  = buffers->begin();
				cbIt != buffers->end();
				++cbIt)
			{
				if ((*cbIt)->bDirty)
				{
					if (bindMode == D3D11RendererVariableManager::BIND_MAP)
					{
						D3D11RendererResourceMapper mappedResource(mContext, (*cbIt)->buffer);
						mappedResource.load((*cbIt)->data, (*cbIt)->dataSize);
					}
					else
					{
						mContext->UpdateSubresource((*cbIt)->buffer, 0, NULL, (*cbIt)->data, 0, 0);
					}
					
					(*cbIt)->bDirty = false;
				}
				pCBs[cbIndex++] = (*cbIt)->buffer;
			}
			switch (d3dType)
			{
			case D3DTypes::SHADER_PIXEL:    D3DTraits<ID3D11PixelShader>::setConstants(mContext,    0, (UINT)pCBs.size(), &pCBs[0]);
				break;
			case D3DTypes::SHADER_VERTEX:   D3DTraits<ID3D11VertexShader>::setConstants(mContext,   0, (UINT)pCBs.size(), &pCBs[0]);
				break;
			case D3DTypes::SHADER_GEOMETRY: D3DTraits<ID3D11GeometryShader>::setConstants(mContext, 0, (UINT)pCBs.size(), &pCBs[0]);
				break;
			case D3DTypes::SHADER_HULL:     D3DTraits<ID3D11HullShader>::setConstants(mContext,     0, (UINT)pCBs.size(), &pCBs[0]);
				break;
			case D3DTypes::SHADER_DOMAIN:   D3DTraits<ID3D11DomainShader>::setConstants(mContext,   0, (UINT)pCBs.size(), &pCBs[0]);
				break;
			default:
				RENDERER_ASSERT(0, "Invalid D3D type");
				break;
			}
		}
		
	}

private:
	D3D11RendererVariableBinder();
	ID3D11DeviceContext* mContext;
};

void D3D11RendererVariableManager::bind(const void* pResource, D3DType type, RendererMaterial::Pass pass) const
{
	ID3D11DeviceContext* d3dDeviceContext = mRenderer.getD3DDeviceContext();
	ShaderTypeKey typeKey                 = getShaderTypeKey(type, pass);

	if (pResource && d3dDeviceContext && typeKey < NUM_SHADER_TYPES)
	{
		ResourceBuffersMap::const_iterator cbIterator = mResourceToBuffers[typeKey].find(pResource);
		const ConstantBuffers* cb = (cbIterator != mResourceToBuffers[typeKey].end()) ? &(cbIterator->second) : NULL;
		
		D3D11RendererVariableBinder binder(d3dDeviceContext);
		binder.bind(type, cb, mBindMode);
	}
}

void D3D11RendererVariableManager::setSharedVariable(const char* sharedBufferName, const char* variableName, const void* data, UINT size, UINT offset)
{
	NameVariablesMap::const_iterator svIt = mNameToSharedVariables.find(VariableKey(sharedBufferName, variableName));
	if (svIt != mNameToSharedVariables.end() && svIt->second)
	{
		D3D11SharedVariable* pSV = svIt->second;
		RENDERER_ASSERT(pSV, "Invalid shared variable");
		internalSetVariable(pSV->m_pBuffer, pSV->getDataOffset()+offset, data, size ? size : pSV->getDataSize());
	}
	else
	{
#if RENDERER_ASSERT_SHARED_VARIABLE_EXISTS
		RENDERER_ASSERT(0, "Shared variable has not been created!");
#endif
	}
}

void D3D11RendererVariableManager::internalSetVariable(D3D11ConstantBuffer* pBuffer, PxU32 offset, const void* data, PxU32 size)
{
	RENDERER_ASSERT(pBuffer, "Invalid constant buffer");
	memcpy(pBuffer->data + offset, data, size);
	pBuffer->bDirty = true;
}

static RendererMaterial::VariableType getVariableType(const D3D11_SHADER_TYPE_DESC& desc)
{
	RendererMaterial::VariableType vt = RendererMaterial::NUM_VARIABLE_TYPES;
	switch (desc.Type)
	{
	case D3D_SVT_INT:
		if (desc.Rows == 1 && desc.Columns == 1)
		{
			vt = RendererMaterial::VARIABLE_INT;
		}
		break;
	case D3D_SVT_FLOAT:
		if (desc.Rows == 4 && desc.Columns == 4)
		{
			vt = RendererMaterial::VARIABLE_FLOAT4x4;
		}
		else if (desc.Rows == 1 && desc.Columns == 1)
		{
			vt = RendererMaterial::VARIABLE_FLOAT;
		}
		else if (desc.Rows == 1 && desc.Columns == 2)
		{
			vt = RendererMaterial::VARIABLE_FLOAT2;
		}
		else if (desc.Rows == 1 && desc.Columns == 3)
		{
			vt = RendererMaterial::VARIABLE_FLOAT3;
		}
		else if (desc.Rows == 1 && desc.Columns == 4)
		{
			vt = RendererMaterial::VARIABLE_FLOAT4;
		}
		break;
	case D3D_SVT_SAMPLER2D:
		vt = RendererMaterial::VARIABLE_SAMPLER2D;
		break;
	case D3D_SVT_SAMPLER3D:
		vt = RendererMaterial::VARIABLE_SAMPLER3D;
		break;
	}
	RENDERER_ASSERT(vt < RendererMaterial::NUM_VARIABLE_TYPES, "Unable to convert shader variable type.");
	return vt;
}

D3D11RendererVariableManager::D3D11ConstantBuffer* 
D3D11RendererVariableManager::loadBuffer(std::vector<D3D11RendererMaterial::Variable*>& variables,
                                         PxU32& variableBufferSize,
                                         ShaderTypeKey typeKey,
                                         ID3D11ShaderReflectionConstantBuffer* pReflectionBuffer,
                                         const D3D11_SHADER_BUFFER_DESC& sbDesc,
                                         const D3D11_BUFFER_DESC& cbDesc)
{
	ID3D11Buffer* pBuffer = NULL;
	D3D11ConstantBuffer* pCB = NULL;
	HRESULT result = mRenderer.getD3DDevice()->CreateBuffer(&cbDesc, NULL, &pBuffer);
	RENDERER_ASSERT(SUCCEEDED(result), "Error creating D3D11 constant buffer.");
	if (SUCCEEDED(result))
	{
		pCB = new D3D11ConstantBuffer(pBuffer, new PxU8[sbDesc.Size], sbDesc.Size);

		try 
		{
			for (PxU32 i = 0; i < sbDesc.Variables; ++i)
			{
				ID3D11ShaderReflectionVariable* pVariable = pReflectionBuffer->GetVariableByIndex(i);
				D3D11_SHADER_VARIABLE_DESC vDesc;
				pVariable->GetDesc(&vDesc);

				ID3D11ShaderReflectionType* pType = pVariable->GetType();
				D3D11_SHADER_TYPE_DESC tDesc;
				pType->GetDesc(&tDesc);

				RendererMaterial::VariableType type = getVariableType(tDesc);

				D3D11DataVariable* var = 0;

				// Search to see if the variable already exists...
				PxU32 numVariables = (PxU32)variables.size();
				for (PxU32 j = 0; j < numVariables; ++j)
				{
					if (!strcmp(variables[j]->getName(), vDesc.Name))
					{
						var = static_cast<D3D11DataVariable*>(variables[j]);
						break;
					}
				}

				// Check to see if the variable is of the same type.
				if (var)
				{
					RENDERER_ASSERT(var->getType() == type, "Variable changes type!");
				}

				// If we couldn't find the variable... create a new variable...
				if (!var)
				{
					var = new D3D11DataVariable(vDesc.Name, type, variableBufferSize);
					variables.push_back(var);
					variableBufferSize += var->getDataSize();
				}

				PxU8* varData = pCB->data + vDesc.StartOffset;
				var->addHandle(typeKey, varData, &(pCB->bDirty));
			}
		}
		catch(...)
		{
			RENDERER_ASSERT(0, "Exception in processing D3D shader variables");
			delete pCB;
		}
		
	}
	return pCB;
}

D3D11RendererVariableManager::D3D11ConstantBuffer*
D3D11RendererVariableManager::loadSharedBuffer(ShaderTypeKey typeKey,
                                               ID3D11ShaderReflectionConstantBuffer* pReflectionBuffer,
                                               const D3D11_SHADER_BUFFER_DESC& sbDesc,
                                               const D3D11_BUFFER_DESC& cbDesc)
{
	D3D11ConstantBuffer* pCB = NULL;
	// Check to see if the specified shared constant buffer has already been created
	NameBuffersMap::iterator cbIt = mNameToSharedBuffer.find(sbDesc.Name);
	if (cbIt != mNameToSharedBuffer.end())
	{
		pCB = cbIt->second;
		if( pCB )
		{
			pCB->addref();
		}
	}
	else
	{
		ID3D11Buffer* pBuffer = NULL;
		HRESULT result = mRenderer.getD3DDevice()->CreateBuffer(&cbDesc, NULL, &pBuffer);
		RENDERER_ASSERT(SUCCEEDED(result), "Error creating D3D11 constant buffer.");
		if (SUCCEEDED(result))
		{
			pCB = new D3D11ConstantBuffer(pBuffer, new PxU8[sbDesc.Size], sbDesc.Size);
			for (PxU32 i = 0; i < sbDesc.Variables; ++i)
			{
				ID3D11ShaderReflectionVariable* pVariable = pReflectionBuffer->GetVariableByIndex(i);
				D3D11_SHADER_VARIABLE_DESC vDesc;
				pVariable->GetDesc(&vDesc);

				ID3D11ShaderReflectionType* pType = pVariable->GetType();
				D3D11_SHADER_TYPE_DESC tDesc;
				pType->GetDesc(&tDesc);

				mVariables.push_back(new D3D11SharedVariable(vDesc.Name, getVariableType(tDesc), vDesc.StartOffset, pCB));
				mNameToSharedVariables[VariableKey(sbDesc.Name, vDesc.Name)] = mVariables.back();
			}
			mNameToSharedBuffer[sbDesc.Name] = pCB;
		}
	}

	return pCB;
}

void D3D11RendererVariableManager::loadVariables(D3D11RendererMaterial* pMaterial,
                                                 ID3DBlob* pShader,
                                                 D3DType type,
                                                 RendererMaterial::Pass pass)
{
	D3DX11* pD3DX = mRenderer.getD3DX11();
	RENDERER_ASSERT(pD3DX, "Invalid D3D11 shader compiler");

	ID3D11ShaderReflection* pReflection = NULL;
	HRESULT result = pD3DX->reflect(pShader->GetBufferPointer(),
	                                pShader->GetBufferSize(),
	                                IID_ID3D11ShaderReflection,
	                                (void**) &pReflection);

	RENDERER_ASSERT(SUCCEEDED(result) && pReflection, "Failure in processing D3D11 shader reflection")
	if (SUCCEEDED(result) && pReflection)
	{
		loadConstantVariables(pMaterial, pShader, getShaderTypeKey(type, pass), pReflection, &pMaterial->m_variables, &pMaterial->m_variableBufferSize);
		loadTextureVariables(pMaterial,  pShader, getShaderTypeKey(type, pass), pReflection);
	}
}

void D3D11RendererVariableManager::loadSharedVariables(const void* pResource,
                                                       ID3DBlob* pShader,
                                                       D3DType type,
                                                       RendererMaterial::Pass pass)
{
	D3DX11* pD3DX = mRenderer.getD3DX11();
	RENDERER_ASSERT(pD3DX, "Invalid D3D11 shader compiler");

	ID3D11ShaderReflection* pReflection = NULL;
	HRESULT result = pD3DX->reflect(pShader->GetBufferPointer(),
                                    pShader->GetBufferSize(),
                                    IID_ID3D11ShaderReflection,
                                    (void**) &pReflection);

	RENDERER_ASSERT(SUCCEEDED(result) && pReflection, "Failure in processing D3D11 shader reflection")
	if (SUCCEEDED(result) && pReflection)
	{
		loadConstantVariables(pResource, pShader, getShaderTypeKey(type, pass), pReflection);
	}
}

void D3D11RendererVariableManager::loadConstantVariables(const void* pResource,
                                                         ID3DBlob* pShader,
                                                         ShaderTypeKey typeKey,
                                                         ID3D11ShaderReflection* pReflection,
                                                         MaterialVariables* pVariables,
                                                         PxU32* pVariableBufferSize)
{
	D3D11_SHADER_DESC desc;
	pReflection->GetDesc(&desc);

	for (PxU32 i = 0; i < desc.ConstantBuffers; ++i)
	{
		D3D11_BUFFER_DESC cbDesc;
		cbDesc.CPUAccessFlags = (mBindMode == BIND_MAP) ? D3D11_CPU_ACCESS_WRITE : 0;
		cbDesc.Usage          = (mBindMode == BIND_MAP) ? D3D11_USAGE_DYNAMIC    : D3D11_USAGE_DEFAULT;
		cbDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.MiscFlags      = 0;

		ID3D11ShaderReflectionConstantBuffer* pConstBuffer = pReflection->GetConstantBufferByIndex(i);

		D3D11_SHADER_BUFFER_DESC sbDesc;
		pConstBuffer->GetDesc(&sbDesc);
		cbDesc.ByteWidth = sbDesc.Size;

		D3D11ConstantBuffer* pCB = NULL;
		// Check to see if the constant buffer's name has been specified as s shared buffer
		if (mSharedBufferNames.find(sbDesc.Name) != mSharedBufferNames.end())
		{
			pCB = loadSharedBuffer(typeKey, pConstBuffer, sbDesc, cbDesc);
		}
		else if (pVariables && pVariableBufferSize)
		{
			pCB = loadBuffer(*pVariables, *pVariableBufferSize, typeKey, pConstBuffer, sbDesc, cbDesc);
		}
		else
		{
			RENDERER_ASSERT(0, "Only materials support non-shared variables.");
		}

		RENDERER_ASSERT(pCB, "Error creating shared buffer");
		if (pCB)
		{
			mResourceToBuffers[typeKey][pResource].push_back(pCB);
		}
	}
}

RendererMaterial::VariableType getTextureVariableType(const D3D11_SHADER_INPUT_BIND_DESC& resourceDesc)
{
	switch (resourceDesc.Dimension)
	{
		case D3D_SRV_DIMENSION_TEXTURE1D:
		case D3D_SRV_DIMENSION_TEXTURE2D:
			return RendererMaterial::VARIABLE_SAMPLER2D;
		case D3D_SRV_DIMENSION_TEXTURE3D:
			return RendererMaterial::VARIABLE_SAMPLER3D;
		default:
			RENDERER_ASSERT(0, "Invalid texture type.");
			return RendererMaterial::NUM_VARIABLE_TYPES;
	}
}

void D3D11RendererVariableManager::loadTextureVariables(D3D11RendererMaterial* pMaterial,
                                                        ID3DBlob* pShader,
                                                        ShaderTypeKey typeKey,
                                                        ID3D11ShaderReflection* pReflection)
{
	D3D11_SHADER_DESC desc;
	pReflection->GetDesc(&desc);

	// Load texture variables for the specified shader
	for (PxU32 i = 0; i < desc.BoundResources; ++i)
	{
		D3D11_SHADER_INPUT_BIND_DESC resourceDesc;
		HRESULT result = pReflection->GetResourceBindingDesc(i, &resourceDesc);
		if (SUCCEEDED(result) && resourceDesc.Type == D3D10_SIT_TEXTURE)
		{
			D3D11TextureVariable* var = NULL;
			// Search to see if the variable already exists...
			PxU32 numVariables = (PxU32)pMaterial->m_variables.size();
			for (PxU32 j = 0; j < numVariables; ++j)
			{
				if (!strcmp(pMaterial->m_variables[j]->getName(), resourceDesc.Name))
				{
					var = static_cast<D3D11TextureVariable*>(pMaterial->m_variables[j]);
					break;
				}
			}

			RendererMaterial::VariableType varType = getTextureVariableType(resourceDesc);

			// Check to see if the variable is of the same type.
			if (var)
			{
				RENDERER_ASSERT(var->getType() == varType, "Variable changes type!");
			}

			// If we couldn't find the variable... create a new variable...
			if (!var && (varType == RendererMaterial::VARIABLE_SAMPLER2D || varType == RendererMaterial::VARIABLE_SAMPLER3D) )
			{
				var = new D3D11TextureVariable(resourceDesc.Name, varType, pMaterial->m_variableBufferSize);
				pMaterial->m_variables.push_back(var);
				pMaterial->m_variableBufferSize += var->getDataSize();
			}

			var->addHandle(typeKey, resourceDesc.BindPoint);
		}
	}
}

void D3D11RendererVariableManager::unloadVariables( const void* pResource )
{
	for (PxU32 i = 0; i < NUM_SHADER_TYPES; ++i)
	{
		ResourceBuffersMap::iterator it = mResourceToBuffers[i].find(pResource);
		if (it != mResourceToBuffers[i].end())
		{
			for (PxU32 j = 0; j < it->second.size(); ++j)
			{
				it->second[j]->release();
			}
			mResourceToBuffers[i].erase(it);
		}
	}
}
#endif
