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
#ifndef D3D11_RENDERER_MATERIAL_H
#define D3D11_RENDERER_MATERIAL_H

#include <RendererConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D11)

#include <RendererMaterial.h>

#include "D3D11Renderer.h"
#include "D3D11RendererTraits.h"
#include "D3Dcompiler.h"

namespace SampleRenderer
{

class D3D11Renderer;

class D3D11RendererMaterial : public RendererMaterial
{
	friend class D3D11Renderer;
	friend class D3D11RendererMesh;
	friend class D3D11RendererVariableManager;

	typedef RendererMaterial::Variable Variable;

public:
	D3D11RendererMaterial(D3D11Renderer& renderer, const RendererMaterialDesc& desc);
	virtual ~D3D11RendererMaterial(void);
	virtual void setModelMatrix(const float* matrix);

	bool tessellationEnabled() const;
	bool tessellationInitialized() const;
	bool tessellationSupported() const;

	bool geometryEnabled() const;
	bool geometryInitialized() const;

private:
	virtual const Renderer& getRenderer() const { return m_renderer; }
	virtual void bind(RendererMaterial::Pass pass, RendererMaterialInstance* materialInstance, bool instanced) const;
	virtual void bindMeshState(bool instanced) const;
	virtual void unbind(void) const;
	virtual void bindVariable(Pass pass, const Variable& variable, const void* data) const;

private:
	typedef RendererMaterial::Variable D3D11BaseVariable;
	class D3D11Variable : public D3D11BaseVariable
	{
	public:
		D3D11Variable(const char* name, RendererMaterial::VariableType type, PxU32 offset)
			: Variable(name, type, offset) { }
		virtual void bind(RendererMaterial::Pass pass, const void* data) = 0;
	};

private:
	D3D11RendererMaterial& operator=(const D3D11RendererMaterial&) { return *this;}

	ID3D11VertexShader*   getVS(bool bInstanced) const { return bInstanced ? m_instancedVertexShader : m_vertexShader; }
	ID3D11PixelShader*    getPS(RendererMaterial::Pass pass) const { return m_fragmentPrograms[pass]; }
	ID3D11GeometryShader* getGS() const { return geometryEnabled()     ? m_geometryShader : NULL; }
	ID3D11HullShader*     getHS() const { return tessellationEnabled() ? m_hullShader     : NULL; }
	ID3D11DomainShader*   getDS() const { return tessellationEnabled() ? m_domainShader   : NULL; }

	ID3DBlob* getVSBlob(const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputDesc, PxU64 inputDescHash, bool bInstanced) const;
	ID3DBlob* getVSBlob(const D3D11_INPUT_ELEMENT_DESC* inputDesc, PxU32 numInputDescs, PxU64 inputDescHash, bool bInstanced) const;
	
	const char* getPath(const D3DType type) const { return m_shaderPaths[type].c_str(); }

	void cacheVS(const D3D11_INPUT_ELEMENT_DESC* inputDesc, PxU32 numInputDescs, PxU64 inputDescHash, bool bInstanced, ID3D11VertexShader** ppShader = NULL, ID3DBlob** ppBlob = NULL) const;
	std::string getShaderNameFromInputLayout(const D3D11_INPUT_ELEMENT_DESC* inputDesc, PxU32 numInputDescs,const std::string& shaderName) const;

	void setVariables(RendererMaterial::Pass pass) const;
	void setBlending(RendererMaterial::Pass pass) const;
	void setShaders(bool bInstanced, RendererMaterial::Pass pass) const;

	void loadBlending(const RendererMaterialDesc& desc);
	void loadShaders(const RendererMaterialDesc& desc);

private:
	D3D11Renderer&               m_renderer;

	ID3D11BlendState*            m_blendState;

	mutable ID3D11VertexShader*  m_vertexShader;
	mutable ID3D11VertexShader*  m_instancedVertexShader;
	ID3D11GeometryShader*        m_geometryShader;
	ID3D11HullShader*            m_hullShader;
	ID3D11DomainShader*          m_domainShader;
	ID3D11PixelShader*           m_fragmentPrograms[NUM_PASSES];

	std::string                  m_shaderNames[D3DTypes::NUM_SHADER_TYPES];
	std::string                  m_shaderPaths[D3DTypes::NUM_SHADER_TYPES];
};

} // namespace SampleRenderer

#endif // #if defined(RENDERER_ENABLE_DIRECT3D11)
#endif
