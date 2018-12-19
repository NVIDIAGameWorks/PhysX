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


#ifndef SAMPLE_APEX_RENDERER_H
#define SAMPLE_APEX_RENDERER_H

#include <UserRenderer.h>
#include <UserRenderResourceManager.h>


#pragma warning(push)
#pragma warning(disable:4512)

class UserRenderSpriteTextureDesc;

namespace SampleRenderer
{
class Renderer;
}

namespace SampleFramework
{
class SampleMaterialAsset;
}


class SampleApexRenderResourceManager : public nvidia::apex::UserRenderResourceManager
{
public:
	/* We either store particle position/color/transform in the texture 
	   and fetch it in the vertex shader OR we store them in the VBO */
	enum ParticleRenderingMechanism {
		VERTEX_TEXTURE_FETCH,
		VERTEX_BUFFER_OBJECT
	};

	SampleApexRenderResourceManager(SampleRenderer::Renderer& renderer);
	virtual								~SampleApexRenderResourceManager(void);

public:
	virtual nvidia::apex::UserRenderVertexBuffer*		createVertexBuffer(const nvidia::apex::UserRenderVertexBufferDesc& desc);
	virtual void										releaseVertexBuffer(nvidia::apex::UserRenderVertexBuffer& buffer);

	virtual nvidia::apex::UserRenderIndexBuffer*		createIndexBuffer(const nvidia::apex::UserRenderIndexBufferDesc& desc);
	virtual void										releaseIndexBuffer(nvidia::apex::UserRenderIndexBuffer& buffer);

	virtual nvidia::apex::UserRenderSurfaceBuffer*		createSurfaceBuffer(const nvidia::apex::UserRenderSurfaceBufferDesc& desc);
	virtual void										releaseSurfaceBuffer(nvidia::apex::UserRenderSurfaceBuffer& buffer);

	virtual nvidia::apex::UserRenderBoneBuffer*		createBoneBuffer(const nvidia::apex::UserRenderBoneBufferDesc& desc);
	virtual void										releaseBoneBuffer(nvidia::apex::UserRenderBoneBuffer& buffer);

	virtual nvidia::apex::UserRenderInstanceBuffer*	createInstanceBuffer(const nvidia::apex::UserRenderInstanceBufferDesc& desc);
	virtual void										releaseInstanceBuffer(nvidia::apex::UserRenderInstanceBuffer& buffer);

	virtual nvidia::apex::UserRenderSpriteBuffer*		createSpriteBuffer(const nvidia::apex::UserRenderSpriteBufferDesc& desc);
	virtual void										releaseSpriteBuffer(nvidia::apex::UserRenderSpriteBuffer& buffer);

	virtual nvidia::apex::UserRenderResource*			createResource(const nvidia::apex::UserRenderResourceDesc& desc);
	virtual void										releaseResource(nvidia::apex::UserRenderResource& resource);

	virtual uint32_t								getMaxBonesForMaterial(void* material);

	virtual bool										getSpriteLayoutData(uint32_t spriteCount, 
																			uint32_t spriteSemanticsBitmap, 
																			nvidia::apex::UserRenderSpriteBufferDesc* vertexDescArray);

	virtual bool										getInstanceLayoutData(uint32_t spriteCount, 
																				uint32_t particleSemanticsBitmap, 
																				nvidia::apex::UserRenderInstanceBufferDesc* instanceDescArray);

	// change the material of a render resource
	void												setMaterial(nvidia::apex::UserRenderResource& resource, void* material);

	void												setParticleRenderingMechanism(ParticleRenderingMechanism m) { m_particleRenderingMechanism = m;	}
protected:
	SampleRenderer::Renderer&	m_renderer;
	ParticleRenderingMechanism  m_particleRenderingMechanism;
	uint32_t				m_numVertexBuffers;
	uint32_t				m_numIndexBuffers;
	uint32_t				m_numSurfaceBuffers; //?
	uint32_t				m_numBoneBuffers;
	uint32_t				m_numInstanceBuffers;
	uint32_t				m_numResources;
};

class SampleApexRenderer : public nvidia::apex::UserRenderer
{
public:
	SampleApexRenderer() : mForceWireframe(false), mOverrideMaterial(NULL) {}
	virtual void renderResource(const nvidia::apex::RenderContext& context);

	bool mForceWireframe;
	SampleFramework::SampleMaterialAsset* mOverrideMaterial;
};

#pragma warning(pop)

#endif
