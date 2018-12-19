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



// This define will use RendererMaterial instead of SampleMaterialAsset
#ifndef USE_RENDERER_MATERIAL
#define USE_RENDERER_MATERIAL 0
#endif

#include <UserRenderer.h>
#include <UserRenderResourceManager.h>

#include <RenderContext.h>
#include <UserRenderBoneBuffer.h>
#include <UserRenderIndexBuffer.h>
#include <UserRenderInstanceBuffer.h>
#include <UserRenderResource.h>
#include <UserRenderSpriteBuffer.h>
#include <UserRenderSurfaceBuffer.h>
#include <UserRenderVertexBuffer.h>
#include <UserRenderSpriteBufferDesc.h>

#include <RendererInstanceBuffer.h>
#include <RendererMeshContext.h>

#define	USE_RENDER_SPRITE_BUFFER 1
#if USE_RENDER_SPRITE_BUFFER
#include <RendererMaterial.h>
#endif

#pragma warning(push)
#pragma warning(disable:4512)

namespace SampleRenderer
{
	class Renderer;
	class RendererVertexBuffer;
	class RendererIndexBuffer;
	class RendererMesh;
	class RendererMeshContext;
	class RendererTexture;
}

namespace SampleFramework
{
	class SampleMaterialAsset;
}

using SampleRenderer::RendererVertexBuffer;
using SampleRenderer::RendererIndexBuffer;
using SampleRenderer::RendererInstanceBuffer;
using SampleRenderer::RendererMesh;
using SampleRenderer::RendererTexture;


/*********************************
* SampleApexRendererVertexBuffer *
*********************************/

class SampleApexRendererVertexBuffer : public nvidia::apex::UserRenderVertexBuffer
{
	friend class SampleApexRendererMesh;
public:
	SampleApexRendererVertexBuffer(SampleRenderer::Renderer& renderer, const nvidia::apex::UserRenderVertexBufferDesc& desc);
	virtual ~SampleApexRendererVertexBuffer(void);

	virtual bool getInteropResourceHandle(CUgraphicsResource& handle);

protected:
	void fixUVOrigin(void* uvdata, uint32_t stride, uint32_t num);
	void flipColors(void* uvData, uint32_t stride, uint32_t num);

	virtual void writeBuffer(const nvidia::apex::RenderVertexBufferData& data, uint32_t firstVertex, uint32_t numVerts);

	bool writeBufferFastPath(const nvidia::apex::RenderVertexBufferData& data, uint32_t firstVertex, uint32_t numVerts);

protected:
	SampleRenderer::Renderer&				m_renderer;
	SampleRenderer::RendererVertexBuffer*	m_vertexbuffer;
	nvidia::apex::TextureUVOrigin::Enum	m_uvOrigin;
};


/********************************
* SampleApexRendererIndexBuffer *
********************************/

class SampleApexRendererIndexBuffer : public nvidia::apex::UserRenderIndexBuffer
{
	friend class SampleApexRendererMesh;
public:
	SampleApexRendererIndexBuffer(SampleRenderer::Renderer& renderer, const nvidia::apex::UserRenderIndexBufferDesc& desc);
	virtual ~SampleApexRendererIndexBuffer(void);

	virtual bool getInteropResourceHandle(CUgraphicsResource& handle);

private:
	virtual void writeBuffer(const void* srcData, uint32_t srcStride, uint32_t firstDestElement, uint32_t numElements);

private:
	SampleRenderer::Renderer&					m_renderer;
	SampleRenderer::RendererIndexBuffer*		m_indexbuffer;
	nvidia::apex::RenderPrimitiveType::Enum	m_primitives;
};


/********************************
* SampleApexRendererSurfaceBuffer *
********************************/

class SampleApexRendererSurfaceBuffer : public nvidia::apex::UserRenderSurfaceBuffer
{
public:
	SampleApexRendererSurfaceBuffer(SampleRenderer::Renderer& renderer, const nvidia::apex::UserRenderSurfaceBufferDesc& desc);
	virtual ~SampleApexRendererSurfaceBuffer(void);

	virtual bool getInteropResourceHandle(CUgraphicsResource& handle);

private:
	virtual void writeBuffer(const void* srcData, uint32_t srcPitch, uint32_t srcHeight, uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t width, uint32_t height, uint32_t depth = 1);

private:
	SampleRenderer::Renderer&					m_renderer;
	SampleRenderer::RendererTexture*			m_texture;
};


/*******************************
* SampleApexRendererBoneBuffer *
*******************************/

class SampleApexRendererBoneBuffer : public nvidia::apex::UserRenderBoneBuffer
{
	friend class SampleApexRendererMesh;
public:
	SampleApexRendererBoneBuffer(SampleRenderer::Renderer& renderer, const nvidia::apex::UserRenderBoneBufferDesc& desc);
	virtual ~SampleApexRendererBoneBuffer(void);

	const physx::PxMat44* getBones() const { return m_bones; }

public:
	virtual void writeBuffer(const nvidia::apex::RenderBoneBufferData& data, uint32_t firstBone, uint32_t numBones);

private:
	SampleRenderer::Renderer& m_renderer;

	SampleRenderer::RendererTexture* m_boneTexture;	// Vertex texture to hold bone matrices
	uint32_t m_maxBones;
	physx::PxMat44* m_bones;
};


/***********************************
* SampleApexRendererInstanceBuffer *
***********************************/

class SampleApexRendererInstanceBuffer : public nvidia::apex::UserRenderInstanceBuffer
{
	friend class SampleApexRendererMesh;
public:

	SampleApexRendererInstanceBuffer(SampleRenderer::Renderer& renderer, const nvidia::apex::UserRenderInstanceBufferDesc& desc);
	virtual ~SampleApexRendererInstanceBuffer(void);

	uint32_t getMaxInstances(void) const
	{
		return m_maxInstances;
	}

public:
	virtual void writeBuffer(const void* data, uint32_t firstInstance, uint32_t numInstances);
	bool writeBufferFastPath(const nvidia::apex::RenderInstanceBufferData& data, uint32_t firstInstance, uint32_t numInstances);

	virtual bool getInteropResourceHandle(CUgraphicsResource& handle);
protected:
	template<typename ElemType>
	void internalWriteSemantic(SampleRenderer::RendererInstanceBuffer::Semantic semantic, const void* srcData, uint32_t srcStride, uint32_t firstDestElement, uint32_t numElements)
	{
		uint32_t destStride = 0;
		uint8_t* destData = (uint8_t*)m_instanceBuffer->lockSemantic(semantic, destStride);
		if (destData)
		{
			destData += firstDestElement * destStride;
			for (uint32_t i = 0; i < numElements; i++)
			{
				ElemType* srcElemPtr = (ElemType*)(((uint8_t*)srcData) + srcStride * i);

				*((ElemType*)destData) = *srcElemPtr;

				destData += destStride;
			}
			m_instanceBuffer->unlockSemantic(semantic);
		}
	}
private:
	void internalWriteBuffer(nvidia::apex::RenderInstanceSemantic::Enum semantic, 
							const void* srcData, uint32_t srcStride,
							uint32_t firstDestElement, uint32_t numElements);

	uint32_t							m_maxInstances;
	SampleRenderer::RendererInstanceBuffer*	m_instanceBuffer;
};


#if USE_RENDER_SPRITE_BUFFER

/*********************************
* SampleApexRendererSpriteBuffer *
*********************************/

/*
 *	This class is just a wrapper around the vertex buffer class because there is already
 *	a point sprite implementation using vertex buffers.  It takes the sprite buffer semantics
 *  and just converts them to vertex buffer semantics and ignores everything but position and color.
 *  Well, not really, it takes the lifetime and translates it to color.
 */
class SampleApexRendererSpriteBuffer : public nvidia::apex::UserRenderSpriteBuffer
{
public:
	SampleApexRendererSpriteBuffer(SampleRenderer::Renderer& renderer, const nvidia::apex::UserRenderSpriteBufferDesc& desc);
	virtual ~SampleApexRendererSpriteBuffer(void);

	SampleRenderer::RendererTexture* getTexture(const nvidia::apex::RenderSpriteTextureLayout::Enum e) const;
	uint32_t getTexturesCount() const;

	virtual bool getInteropResourceHandle(CUgraphicsResource& handle);
	virtual bool getInteropTextureHandleList(CUgraphicsResource* handleList);
	virtual void writeBuffer(const void* data, uint32_t firstSprite, uint32_t numSprites);
	virtual void writeTexture(uint32_t textureId, uint32_t numSprites, const void* srcData, size_t srcSize);

private:
	void flipColors(void* uvData, uint32_t stride, uint32_t num);
public:
	SampleRenderer::Renderer&				m_renderer;
	SampleRenderer::RendererVertexBuffer*	m_vertexbuffer;
	SampleRenderer::RendererTexture*		m_textures[nvidia::apex::UserRenderSpriteBufferDesc::MAX_SPRITE_TEXTURES];
	uint32_t							m_texturesCount;
	uint32_t							m_textureIndexFromLayoutType[nvidia::apex::RenderSpriteTextureLayout::NUM_LAYOUTS];
};

#endif /* USE_RENDER_SPRITE_BUFFER */


/*************************
* SampleApexRendererMesh *
*************************/

/*
 *	There is some sprite hackery in here now.  Basically, if a sprite buffer is used
 *	we just treat is as a vertex buffer (because it really is a vertex buffer).
 */
class SampleApexRendererMesh : public nvidia::apex::UserRenderResource
{
	friend class SampleApexRenderer;
public:
	SampleApexRendererMesh(SampleRenderer::Renderer& renderer, const nvidia::apex::UserRenderResourceDesc& desc);
	virtual ~SampleApexRendererMesh();

	enum BlendType
	{
		BLENDING_ENABLED = 0,
		BLENDING_DISABLED,
		BLENDING_ANY,
		BLENDING_DEFAULT = BLENDING_ANY
	};

public:
	void setVertexBufferRange(uint32_t firstVertex, uint32_t numVerts);
	void setIndexBufferRange(uint32_t firstIndex, uint32_t numIndices);
	void setBoneBufferRange(uint32_t firstBone, uint32_t numBones);
	void setInstanceBufferRange(uint32_t firstInstance, uint32_t numInstances);

#if USE_RENDER_SPRITE_BUFFER
	void setSpriteBufferRange(uint32_t firstSprite, uint32_t numSprites)
	{
		setVertexBufferRange(firstSprite, numSprites);
	}
#endif

#if !USE_RENDERER_MATERIAL
	static void pickMaterial(SampleRenderer::RendererMeshContext& context, bool hasBones, SampleFramework::SampleMaterialAsset& material, BlendType hasBlending = BLENDING_DEFAULT);
#endif

	virtual void setMaterial(void* material) { setMaterial(material, BLENDING_DEFAULT); }
	void setMaterial(void* material, BlendType hasBlending);
	void setScreenSpace(bool ss);

	uint32_t getNbVertexBuffers() const
	{
		return m_numVertexBuffers;
	}

	nvidia::apex::UserRenderVertexBuffer* getVertexBuffer(uint32_t index) const
	{
		nvidia::apex::UserRenderVertexBuffer* buffer = 0;
		PX_ASSERT(index < m_numVertexBuffers);
		if (index < m_numVertexBuffers)
		{
			buffer = m_vertexBuffers[index];
		}
		return buffer;
	}

	nvidia::apex::UserRenderIndexBuffer* getIndexBuffer() const
	{
		return m_indexBuffer;
	}

	nvidia::apex::UserRenderBoneBuffer* getBoneBuffer()	const
	{
		return m_boneBuffer;
	}

	nvidia::apex::UserRenderInstanceBuffer* getInstanceBuffer()	const
	{
		return m_instanceBuffer;
	}

#if USE_RENDER_SPRITE_BUFFER
	nvidia::apex::UserRenderSpriteBuffer* getSpriteBuffer()	const
	{
		return m_spriteBuffer;
	}
#endif

protected:
	void render(const nvidia::apex::RenderContext& context, bool forceWireframe = false, SampleFramework::SampleMaterialAsset* overrideMaterial = NULL);

protected:
	SampleRenderer::Renderer&				m_renderer;

#if USE_RENDER_SPRITE_BUFFER
	SampleApexRendererSpriteBuffer*			m_spriteBuffer;

	// currently this renderer's sprite shaders take 5 variables:
	// particleSize 
	// windowWidth
	// positionTexture
	// colorTexture
	// transformTexture
	// vertexTextureWidth
	// vertexTextureHeight
	const SampleRenderer::RendererMaterial::Variable*  m_spriteShaderVariables[7];
#endif

	SampleApexRendererVertexBuffer**		m_vertexBuffers;
	uint32_t							m_numVertexBuffers;

	SampleApexRendererIndexBuffer*			m_indexBuffer;

	SampleApexRendererBoneBuffer*			m_boneBuffer;
	uint32_t							m_firstBone;
	uint32_t							m_numBones;

	SampleApexRendererInstanceBuffer*		m_instanceBuffer;

	SampleRenderer::RendererMesh*			m_mesh;
	SampleRenderer::RendererMeshContext		m_meshContext;
	physx::PxMat44							m_meshTransform;
	nvidia::apex::RenderCullMode::Enum		m_cullMode;
};

#pragma warning(pop)

