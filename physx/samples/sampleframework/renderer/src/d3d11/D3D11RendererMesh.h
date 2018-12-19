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
