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
