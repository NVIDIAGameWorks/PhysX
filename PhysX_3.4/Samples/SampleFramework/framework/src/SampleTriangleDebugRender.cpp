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
#include <SampleTriangleDebugRender.h>
#include <Renderer.h>
#include <RendererVertexBuffer.h>
#include <RendererVertexBufferDesc.h>
#include <RendererMesh.h>
#include <RendererMeshDesc.h>
#include <SampleAssetManager.h>
#include <SampleMaterialAsset.h>
#include <RendererMemoryMacros.h>

using namespace SampleFramework;

SampleTriangleDebugRender::SampleTriangleDebugRender(SampleRenderer::Renderer &renderer, SampleAssetManager &assetmanager) :
	m_renderer(renderer),
	m_assetmanager(assetmanager)
{
	m_material = static_cast<SampleMaterialAsset*>(m_assetmanager.getAsset("materials/simple_lit_color.xml", SampleAsset::ASSET_MATERIAL));
	PX_ASSERT(m_material);
	PX_ASSERT(m_material->getNumVertexShaders() == 1);
	m_meshContext.material = m_material ? m_material->getMaterial(0) : 0;

	m_maxVerts        = 0;
	m_numVerts        = 0;
	m_vertexbuffer    = 0;
	m_mesh            = 0;
	m_lockedPositions = 0;
	m_positionStride  = 0;
	m_lockedNormals   = 0;
	m_normalStride    = 0;
	m_lockedColors    = 0;
	m_colorStride     = 0;
}

SampleTriangleDebugRender::~SampleTriangleDebugRender(void)
{
	checkUnlock();
	SAFE_RELEASE(m_vertexbuffer);
	SAFE_RELEASE(m_mesh);
	if(m_material)
	{
		m_assetmanager.returnAsset(*m_material);
	}
}

void SampleTriangleDebugRender::addTriangle(const PxVec3 &p0, const PxVec3 &p1, const PxVec3 &p2, const SampleRenderer::RendererColor &color)
{
	checkResizeTriangle(m_numVerts+3);
	PxVec3 normal = (p1-p0).cross(p2-p0);
	normal.normalize();
	addVert(p0, normal, color);
	addVert(p1, normal, color);
	addVert(p2, normal, color);
}

void SampleTriangleDebugRender::addTriangle(const PxVec3 &p0, const PxVec3 &p1, const PxVec3 &p2, const PxVec3& n0, const PxVec3& n1, const PxVec3& n2, const SampleRenderer::RendererColor &color)
{
	checkResizeTriangle(m_numVerts+3);
	addVert(p0, n0, color);
	addVert(p1, n1, color);
	addVert(p2, n2, color);
}

void SampleTriangleDebugRender::queueForRenderTriangle(void)
{
	if(m_meshContext.mesh)
	{
		checkUnlock();
		m_mesh->setVertexBufferRange(0, m_numVerts);
		m_renderer.queueMeshForRender(m_meshContext);
	}
}

void SampleTriangleDebugRender::clearTriangle(void)
{
	m_numVerts = 0;
}

void SampleTriangleDebugRender::checkResizeTriangle(PxU32 maxVerts)
{
	if(maxVerts > m_maxVerts)
	{
		m_maxVerts = maxVerts + (PxU32)(maxVerts*0.2f);

		SampleRenderer::RendererVertexBufferDesc vbdesc;
		vbdesc.hint = SampleRenderer::RendererVertexBuffer::HINT_DYNAMIC;
		vbdesc.semanticFormats[SampleRenderer::RendererVertexBuffer::SEMANTIC_POSITION] = SampleRenderer::RendererVertexBuffer::FORMAT_FLOAT3;
		vbdesc.semanticFormats[SampleRenderer::RendererVertexBuffer::SEMANTIC_NORMAL]   = SampleRenderer::RendererVertexBuffer::FORMAT_FLOAT3;
		vbdesc.semanticFormats[SampleRenderer::RendererVertexBuffer::SEMANTIC_COLOR]    = SampleRenderer::RendererVertexBuffer::FORMAT_COLOR_NATIVE; // Same format as RendererColor
		vbdesc.maxVertices = m_maxVerts;
		SampleRenderer::RendererVertexBuffer *vertexbuffer = m_renderer.createVertexBuffer(vbdesc);
		PX_ASSERT(vertexbuffer);

		if(vertexbuffer)
		{
			PxU32 positionStride = 0;
			void *positions = vertexbuffer->lockSemantic(SampleRenderer::RendererVertexBuffer::SEMANTIC_POSITION, positionStride);

			PxU32 normalStride = 0;
			void *normals = vertexbuffer->lockSemantic(SampleRenderer::RendererVertexBuffer::SEMANTIC_NORMAL, normalStride);

			PxU32 colorStride = 0;
			void *colors = vertexbuffer->lockSemantic(SampleRenderer::RendererVertexBuffer::SEMANTIC_COLOR, colorStride);

			PX_ASSERT(positions && colors);
			if(positions && colors)
			{
				if(m_numVerts > 0)
				{
					// if we have existing verts, copy them to the new VB...
					PX_ASSERT(m_lockedPositions);
					PX_ASSERT(m_lockedNormals);
					PX_ASSERT(m_lockedColors);
					if(m_lockedPositions && m_lockedNormals && m_lockedColors)
					{
						for(PxU32 i=0; i<m_numVerts; i++)
						{
							memcpy(((PxU8*)positions) + (positionStride*i), ((PxU8*)m_lockedPositions) + (m_positionStride*i), sizeof(PxVec3));
							memcpy(((PxU8*)normals)   + (normalStride*i),   ((PxU8*)m_lockedNormals)   + (m_normalStride*i),   sizeof(PxVec3));
							memcpy(((PxU8*)colors)    + (colorStride*i),    ((PxU8*)m_lockedColors)    + (m_colorStride*i),    sizeof(SampleRenderer::RendererColor));
						}
					}
					m_vertexbuffer->unlockSemantic(SampleRenderer::RendererVertexBuffer::SEMANTIC_COLOR);
					m_vertexbuffer->unlockSemantic(SampleRenderer::RendererVertexBuffer::SEMANTIC_NORMAL);
					m_vertexbuffer->unlockSemantic(SampleRenderer::RendererVertexBuffer::SEMANTIC_POSITION);
				}
			}
			vertexbuffer->unlockSemantic(SampleRenderer::RendererVertexBuffer::SEMANTIC_COLOR);
			vertexbuffer->unlockSemantic(SampleRenderer::RendererVertexBuffer::SEMANTIC_NORMAL);
			vertexbuffer->unlockSemantic(SampleRenderer::RendererVertexBuffer::SEMANTIC_POSITION);
		}
		if(m_vertexbuffer)
		{
			m_vertexbuffer->release();
			m_vertexbuffer    = 0;
			m_lockedPositions = 0;
			m_positionStride  = 0;
			m_lockedNormals   = 0;
			m_normalStride    = 0;
			m_lockedColors    = 0;
			m_colorStride     = 0;
		}
		SAFE_RELEASE(m_mesh);
		if(vertexbuffer)
		{
			m_vertexbuffer = vertexbuffer;
			SampleRenderer::RendererMeshDesc meshdesc;
			meshdesc.primitives       = SampleRenderer::RendererMesh::PRIMITIVE_TRIANGLES;
			meshdesc.vertexBuffers    = &m_vertexbuffer;
			meshdesc.numVertexBuffers = 1;
			meshdesc.firstVertex      = 0;
			meshdesc.numVertices      = m_numVerts;
			m_mesh = m_renderer.createMesh(meshdesc);
			PX_ASSERT(m_mesh);
		}
		m_meshContext.mesh = m_mesh;
		m_meshContext.cullMode = SampleRenderer::RendererMeshContext::NONE;
	}
}

void SampleTriangleDebugRender::checkLock(void)
{
	if(m_vertexbuffer)
	{
		if(!m_lockedPositions)
		{
			m_lockedPositions = m_vertexbuffer->lockSemantic(SampleRenderer::RendererVertexBuffer::SEMANTIC_POSITION, m_positionStride);
			PX_ASSERT(m_lockedPositions);
		}
		if(!m_lockedNormals)
		{
			m_lockedNormals = m_vertexbuffer->lockSemantic(SampleRenderer::RendererVertexBuffer::SEMANTIC_NORMAL, m_normalStride);
			PX_ASSERT(m_lockedNormals);
		}
		if(!m_lockedColors)
		{
			m_lockedColors = m_vertexbuffer->lockSemantic(SampleRenderer::RendererVertexBuffer::SEMANTIC_COLOR, m_colorStride);
			PX_ASSERT(m_lockedColors);
		}
	}
}

void SampleTriangleDebugRender::checkUnlock(void)
{
	if(m_vertexbuffer)
	{
		if(m_lockedPositions)
		{
			m_vertexbuffer->unlockSemantic(SampleRenderer::RendererVertexBuffer::SEMANTIC_POSITION);
			m_lockedPositions = 0;
		}
		if (m_lockedNormals)
		{
			m_vertexbuffer->unlockSemantic(SampleRenderer::RendererVertexBuffer::SEMANTIC_NORMAL);
			m_lockedNormals = 0;
		}
		if(m_lockedColors)
		{
			m_vertexbuffer->unlockSemantic(SampleRenderer::RendererVertexBuffer::SEMANTIC_COLOR);
			m_lockedColors = 0;
		}
	}
}

void SampleTriangleDebugRender::addVert(const PxVec3 &p, const PxVec3 &n, const SampleRenderer::RendererColor &color)
{
	PX_ASSERT(m_maxVerts > m_numVerts);
	{
		checkLock();
		if(m_lockedPositions && m_lockedNormals && m_lockedColors)
		{
			*(PxVec3*)(((PxU8*)m_lockedPositions) + (m_positionStride*m_numVerts)) = p;
			*(PxVec3*)(((PxU8*)m_lockedNormals)	+ (m_normalStride*m_numVerts)) = n;
			*(SampleRenderer::RendererColor*)(((PxU8*)m_lockedColors) + (m_colorStride*m_numVerts)) = color;
			
			m_numVerts++;
		}
	}
}
