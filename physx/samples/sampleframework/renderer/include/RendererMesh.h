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

#ifndef RENDERER_MESH_H
#define RENDERER_MESH_H

#include <RendererConfig.h>
#include <RendererIndexBuffer.h>

namespace SampleRenderer
{
	class Renderer;
	class RendererMeshDesc;
	class RendererVertexBuffer;
	class RendererIndexBuffer;
	class RendererInstanceBuffer;
	class RendererMaterial;

	class RendererMesh
	{
		friend class Renderer;
	public:
		enum Primitive
		{
			PRIMITIVE_POINTS = 0,
			PRIMITIVE_LINES,
			PRIMITIVE_LINE_STRIP,
			PRIMITIVE_TRIANGLES,
			PRIMITIVE_TRIANGLE_STRIP,
			PRIMITIVE_POINT_SPRITES,

			NUM_PRIMITIVES
		}_Primitive;

	protected:
		RendererMesh(const RendererMeshDesc &desc);
		virtual ~RendererMesh(void);

	public:
		void release(void);

		Primitive getPrimitives(void) const;

		PxU32 getNumVertices(void) const;
		PxU32 getNumIndices(void) const;
		PxU32 getNumInstances(void) const;

		void setVertexBufferRange(PxU32 firstVertex, PxU32 numVertices);
		void setIndexBufferRange(PxU32 firstIndex, PxU32 numIndices);
		void setInstanceBufferRange(PxU32 firstInstance, PxU32 numInstances);

		PxU32                             getNumVertexBuffers(void) const;
		const RendererVertexBuffer *const*getVertexBuffers(void) const;

		const RendererIndexBuffer        *getIndexBuffer(void) const;

		const RendererInstanceBuffer     *getInstanceBuffer(void) const;

		virtual bool willRender() const { return true; }

	protected:
		void         bind(void) const;
		void         render(RendererMaterial *material) const;
		void         unbind(void) const;

		virtual void renderIndices(PxU32 numVertices, PxU32 firstIndex, PxU32 numIndices, RendererIndexBuffer::Format indexFormat, RendererMaterial *material) const = 0;
		virtual void renderVertices(PxU32 numVertices, RendererMaterial *material) const = 0;

		virtual void renderIndicesInstanced(PxU32 numVertices, PxU32 firstIndex, PxU32 numIndices, RendererIndexBuffer::Format indexFormat,RendererMaterial *material) const = 0;
		virtual void renderVerticesInstanced(PxU32 numVertices,RendererMaterial *material) const = 0;

		virtual Renderer& renderer() = 0;

	protected:
		Primitive               m_primitives;

		RendererVertexBuffer  **m_vertexBuffers;
		PxU32                   m_numVertexBuffers;
		PxU32                   m_firstVertex;
		PxU32                   m_numVertices;

		RendererIndexBuffer    *m_indexBuffer;
		PxU32                   m_firstIndex;
		PxU32                   m_numIndices;

		RendererInstanceBuffer *m_instanceBuffer;
		PxU32                   m_firstInstance;
		PxU32                   m_numInstances;
	};

} // namespace SampleRenderer

#endif
