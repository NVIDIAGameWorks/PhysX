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
#ifndef D3D11_RENDERER_MESH_H
#define D3D11_RENDERER_MESH_H

#include <RendererConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D11)

#include <RendererMesh.h>
#include "D3D11Renderer.h"

namespace SampleRenderer
{
class D3D11RendererMaterial;

class D3D11RendererMesh : public RendererMesh
{
	friend class D3D11Renderer;
	
public:
	D3D11RendererMesh(D3D11Renderer& renderer, const RendererMeshDesc& desc);
	virtual ~D3D11RendererMesh(void);

protected:
	virtual void renderIndices(PxU32 numVertices, PxU32 firstIndex, PxU32 numIndices, RendererIndexBuffer::Format indexFormat, RendererMaterial* material) const;
	virtual void renderVertices(PxU32 numVertices, RendererMaterial* material) const;

	virtual void renderIndicesInstanced(PxU32 numVertices, PxU32 firstIndex, PxU32 numIndices, RendererIndexBuffer::Format indexFormat, RendererMaterial* material) const;
	virtual void renderVerticesInstanced(PxU32 numVertices, RendererMaterial* material) const;

	D3D11RendererMesh& operator=(const D3D11RendererMesh&) { return *this; }

	Renderer& renderer() { return m_renderer; }

private:
	void         bind(void) const;
	void         render(RendererMaterial* material) const;

	void setTopology(const Primitive& primitive, bool bTessellationEnabled) const;
	void setLayout(const RendererMaterial*, bool bInstanced) const;
	void setSprites(bool bEnabled) const;
	void setNumVerticesAndIndices(PxU32 numIndices, PxU32 numVertices);

	ID3D11InputLayout* getInputLayoutForMaterial(const D3D11RendererMaterial*, bool bInstanced) const;

private:
	class ScopedMeshRender;
	friend class ScopedMeshRender;

	typedef std::vector<D3D11_INPUT_ELEMENT_DESC> LayoutVector;

	D3D11Renderer&           m_renderer;

	LayoutVector             m_inputDescriptions;
	LayoutVector             m_instancedInputDescriptions;
	PxU64                    m_inputHash;
	PxU64                    m_instancedInputHash;

	// This is merely for cleanup, and has no effect on externally visible state
	mutable bool             m_bPopStates;

	std::string              m_spriteShaderPath;
};

} // namespace SampleRenderer

#endif // #if defined(RENDERER_ENABLE_DIRECT3D11)
#endif
