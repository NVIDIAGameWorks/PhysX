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

#include "D3D11RendererMesh.h"
#include "D3D11RendererVertexBuffer.h"
#include "D3D11RendererInstanceBuffer.h"
#include "D3D11RendererMemoryMacros.h"
#include "D3D11RendererMaterial.h"
#include "D3D11RendererUtils.h"
#include "D3D11RendererResourceManager.h"
#include "D3D11RendererVariableManager.h"

#include <RendererMeshDesc.h>

#include <SamplePlatform.h>

#pragma warning(disable:4702 4189)

using namespace SampleRenderer;

D3D11RendererMesh::D3D11RendererMesh(D3D11Renderer& renderer, const RendererMeshDesc& desc) :
	RendererMesh(desc),
	m_renderer(renderer),
	m_inputHash(0),
	m_instancedInputHash(0),
	m_bPopStates(false),
#ifdef PX_USE_DX11_PRECOMPILED_SHADERS
	m_spriteShaderPath(std::string(m_renderer.getAssetDir())+"compiledshaders/dx11feature11/geometry/pointsprite.cg.cso")
#else
	m_spriteShaderPath(std::string(m_renderer.getAssetDir())+"shaders/vertex/pointsprite.cg")
#endif
{
	ID3D11Device* d3dDevice = m_renderer.getD3DDevice();
	RENDERER_ASSERT(d3dDevice, "Renderer's D3D Device not found!");
	if (d3dDevice)
	{
		PxU32                             numVertexBuffers = getNumVertexBuffers();
		const RendererVertexBuffer* const* vertexBuffers   = getVertexBuffers();

		for (PxU32 i = 0; i < numVertexBuffers; i++)
		{
			const RendererVertexBuffer* vb = vertexBuffers[i];
			if (vb)
			{
				const D3D11RendererVertexBuffer& d3dVb = *static_cast<const D3D11RendererVertexBuffer*>(vb);
				d3dVb.addVertexElements(i, m_inputDescriptions);
			}
		}
		m_inputHash = D3DX11::getInputHash(m_inputDescriptions);

		m_instancedInputDescriptions = m_inputDescriptions;
		if (m_instanceBuffer)
		{
			static_cast<const D3D11RendererInstanceBuffer*>(m_instanceBuffer)->addVertexElements(numVertexBuffers, m_instancedInputDescriptions);
			m_instancedInputHash = D3DX11::getInputHash(m_instancedInputDescriptions);
		}
		else
		{
			m_instancedInputHash = m_inputHash;
		}
	}
}

D3D11RendererMesh::~D3D11RendererMesh(void)
{

}

static D3D_PRIMITIVE_TOPOLOGY getD3DPrimitive(const RendererMesh::Primitive& primitive, bool bTessellationEnabled)
{
	D3D_PRIMITIVE_TOPOLOGY d3dPrimitive = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	switch (primitive)
	{
	case RendererMesh::PRIMITIVE_POINTS:
		d3dPrimitive = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		break;
	case RendererMesh::PRIMITIVE_LINES:
		d3dPrimitive = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		break;
	case RendererMesh::PRIMITIVE_LINE_STRIP:
		d3dPrimitive = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
		break;
	case RendererMesh::PRIMITIVE_TRIANGLES:
		d3dPrimitive = bTessellationEnabled ? D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST : D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;
	case RendererMesh::PRIMITIVE_TRIANGLE_STRIP:
		d3dPrimitive = bTessellationEnabled ? D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST : D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		break;
	case RendererMesh::PRIMITIVE_POINT_SPRITES:
		d3dPrimitive = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		break;
	}
	RENDERER_ASSERT(d3dPrimitive != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED, "Unable to find DIRECT3D11 Primitive.");
	return d3dPrimitive;
}

class D3D11RendererMesh::ScopedMeshRender {
public:

	ScopedMeshRender(const D3D11RendererMesh& mesh_, RendererMaterial* material, bool instanced) : mesh(mesh_) 
	{ 
		mesh.setTopology(mesh.getPrimitives(), static_cast<D3D11RendererMaterial*>(material)->tessellationEnabled());
		mesh.setLayout(material, instanced);
		if (mesh.getPrimitives() == RendererMesh::PRIMITIVE_POINT_SPRITES) 
			mesh.setSprites(true);
	}

	~ScopedMeshRender() 
	{
		if (mesh.getPrimitives() == RendererMesh::PRIMITIVE_POINT_SPRITES) 
			mesh.setSprites(false);
	}

	ScopedMeshRender& operator=(const ScopedMeshRender&) { return *this; }

	const D3D11RendererMesh& mesh;
};

void D3D11RendererMesh::renderIndices(PxU32 numVertices, PxU32 firstIndex, PxU32 numIndices, RendererIndexBuffer::Format indexFormat, RendererMaterial* material) const
{
	ID3D11DeviceContext* d3dDeviceContext = m_renderer.getD3DDeviceContext();
	RENDERER_ASSERT(material, "Invalid RendererMaterial");
	if (d3dDeviceContext && material)
	{
		ScopedMeshRender renderScope(*this, material, false);
		d3dDeviceContext->DrawIndexed(numIndices, firstIndex, 0);
	}
}

void D3D11RendererMesh::renderVertices(PxU32 numVertices, RendererMaterial* material) const
{
	ID3D11DeviceContext* d3dDeviceContext = m_renderer.getD3DDeviceContext();
	RENDERER_ASSERT(material, "Invalid RendererMaterial");
	if (d3dDeviceContext && material)
	{
		ScopedMeshRender renderScope(*this, material, false);
		d3dDeviceContext->Draw(numVertices, 0);
	}
}

void D3D11RendererMesh::renderIndicesInstanced(PxU32 numVertices, PxU32 firstIndex, PxU32 numIndices, RendererIndexBuffer::Format indexFormat, RendererMaterial* material) const
{
	ID3D11DeviceContext* d3dDeviceContext = m_renderer.getD3DDeviceContext();
	RENDERER_ASSERT(material, "Invalid RendererMaterial");
	if (d3dDeviceContext && material)
	{
		ScopedMeshRender renderScope(*this, material, true);
		d3dDeviceContext->DrawIndexedInstanced(numIndices, m_numInstances, firstIndex, 0, m_firstInstance);
	}
}

void D3D11RendererMesh::renderVerticesInstanced(PxU32 numVertices, RendererMaterial* material) const
{
	ID3D11DeviceContext* d3dDeviceContext = m_renderer.getD3DDeviceContext();
	RENDERER_ASSERT(material, "Invalid RendererMaterial");
	if (d3dDeviceContext && material)
	{
		ScopedMeshRender renderScope(*this, material, true);
		d3dDeviceContext->DrawInstanced(numVertices, m_numInstances, 0, m_firstInstance);
	}
}

void D3D11RendererMesh::bind(void) const
{
	RendererMesh::bind();
}

void D3D11RendererMesh::render(RendererMaterial* material) const
{
	RendererMesh::render(material);
}

void D3D11RendererMesh::setTopology(const Primitive& primitive, bool bTessellationEnabled) const
{
	ID3D11DeviceContext* d3dContext = m_renderer.getD3DDeviceContext();
	RENDERER_ASSERT(d3dContext, "Invalid D3D11 context");
	d3dContext->IASetPrimitiveTopology(getD3DPrimitive(primitive, bTessellationEnabled));
}

ID3D11InputLayout* D3D11RendererMesh::getInputLayoutForMaterial(const D3D11RendererMaterial* pMaterial, bool bInstanced) const
{
	ID3D11InputLayout* pLayout = NULL;

	RENDERER_ASSERT(pMaterial, "Invalid D3D11 Material");
	const LayoutVector& inputDescriptions = bInstanced ? m_instancedInputDescriptions : m_inputDescriptions;
	const PxU64&        inputHash         = bInstanced ? m_instancedInputHash : m_inputHash;
	ID3DBlob* pVSBlob = pMaterial->getVSBlob(inputDescriptions, inputHash, bInstanced);
	RENDERER_ASSERT(pVSBlob, "Invalid D3D11 Shader Blob");

	D3DTraits<ID3D11InputLayout>::key_type ilKey(inputHash, pVSBlob);
	pLayout = m_renderer.getResourceManager()->hasResource<ID3D11InputLayout>(ilKey);
	if (!pLayout)
	{
		ID3D11Device* d3dDevice = m_renderer.getD3DDevice();
		RENDERER_ASSERT(d3dDevice, "Invalid D3D11 device");
		HRESULT result = d3dDevice->CreateInputLayout(&inputDescriptions[0],
		                 (UINT)inputDescriptions.size(),
		                 pVSBlob->GetBufferPointer(),
		                 pVSBlob->GetBufferSize(),
		                 &pLayout);
		RENDERER_ASSERT(SUCCEEDED(result) && pLayout, "Failed to create DIRECT3D11 Input Layout.");
		if (SUCCEEDED(result) && pLayout)
		{
			m_renderer.getResourceManager()->registerResource<ID3D11InputLayout>(ilKey, pLayout);
		}
	}

	RENDERER_ASSERT(pLayout, "Failed to find DIRECT3D11 Input Layout.");
	return pLayout;
}

void D3D11RendererMesh::setLayout(const RendererMaterial* pMaterial, bool bInstanced) const
{
	ID3D11DeviceContext* d3dContext = m_renderer.getD3DDeviceContext();
	RENDERER_ASSERT(d3dContext, "Invalid D3D11 context");
	ID3D11InputLayout* pLayout = getInputLayoutForMaterial(static_cast<const D3D11RendererMaterial*>(pMaterial), bInstanced);
	d3dContext->IASetInputLayout(pLayout);
}

void D3D11RendererMesh::setSprites(bool bEnabled) const
{
	if (bEnabled && m_renderer.getFeatureLevel() > D3D_FEATURE_LEVEL_9_3)
	{
		static const D3D_SHADER_MACRO geometryDefines[] =
		{
			"RENDERER_GEOMETRY", "1",
			"RENDERER_D3D11",    "1",
			"PX_WINDOWS",        "1",
			"USE_ALL",           "1",
			"SEMANTIC_TANGENT",  "TANGENT",
			NULL,                NULL
		};

		ID3D11GeometryShader* pShader = NULL;
		ID3DBlob*               pBlob = NULL;
		bool         bLoadedFromCache = false;

		D3D11ShaderLoader gsLoader(m_renderer);
		if (SUCCEEDED(gsLoader.load("pointsprite", m_spriteShaderPath.c_str(), geometryDefines, &pShader, &pBlob, true, &bLoadedFromCache)))
		{
			// If the shader was just compiled we need to load the proper variables for it
			if (!bLoadedFromCache) 
			{
				m_renderer.getVariableManager()->loadSharedVariables(this, pBlob, D3DTypes::SHADER_GEOMETRY);
			}
			m_renderer.getVariableManager()->bind(this, D3DTypes::SHADER_GEOMETRY);
			m_renderer.getD3DDeviceContext()->GSSetShader(pShader, NULL, 0);
		}
	}
	else
	{
		m_renderer.getD3DDeviceContext()->GSSetShader(NULL, NULL, 0);
	}
}

void D3D11RendererMesh::setNumVerticesAndIndices(PxU32 nbVerts, PxU32 nbIndices)
{
	m_numVertices = nbVerts;
	m_numIndices =  nbIndices;
}


#endif // #if defined(RENDERER_ENABLE_DIRECT3D11)
