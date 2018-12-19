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

#ifndef RENDERER_VERTEXBUFFER_H
#define RENDERER_VERTEXBUFFER_H

#include <RendererConfig.h>
#include "RendererInteropableBuffer.h"

namespace SampleRenderer
{

	class RendererVertexBufferDesc;

	class RendererVertexBuffer: public RendererInteropableBuffer
	{
		friend class RendererMesh;
	public:
		enum Semantic
		{
			SEMANTIC_POSITION = 0,
			SEMANTIC_COLOR,
			SEMANTIC_NORMAL,
			SEMANTIC_TANGENT,

			SEMANTIC_DISPLACEMENT_TEXCOORD,
			SEMANTIC_DISPLACEMENT_FLAGS,

			SEMANTIC_BONEINDEX,
			SEMANTIC_BONEWEIGHT,

			SEMANTIC_TEXCOORD0,
			SEMANTIC_TEXCOORD1,
			SEMANTIC_TEXCOORD2,
			SEMANTIC_TEXCOORD3,
			SEMANTIC_TEXCOORDMAX = SEMANTIC_TEXCOORD3,

			NUM_SEMANTICS,
			NUM_TEXCOORD_SEMANTICS = SEMANTIC_TEXCOORDMAX - SEMANTIC_TEXCOORD0,
		}_Semantic;

		enum Format
		{
			FORMAT_FLOAT1 = 0,
			FORMAT_FLOAT2,
			FORMAT_FLOAT3,
			FORMAT_FLOAT4,

			FORMAT_UBYTE4,
			FORMAT_USHORT4,

			FORMAT_COLOR_BGRA, // RendererColor
			FORMAT_COLOR_RGBA, // OGL format
			FORMAT_COLOR_NATIVE, // do not convert

			NUM_FORMATS,
		}_Format;

		enum Hint
		{
			HINT_STATIC = 0,
			HINT_DYNAMIC,
		}_Hint;

	public:
		static PxU32 getFormatByteSize(Format format);

	protected:
		RendererVertexBuffer(const RendererVertexBufferDesc &desc);
		virtual ~RendererVertexBuffer(void);

	public:
		void release(void) { delete this; }

		PxU32  getMaxVertices(void) const;

		Hint   getHint(void) const;
		Format getFormatForSemantic(Semantic semantic) const;

		void *lockSemantic(Semantic semantic, PxU32 &stride);
		void  unlockSemantic(Semantic semantic);

		//Checks buffer written state for vertex buffers in D3D
		virtual bool checkBufferWritten() { return true; }

		virtual void *lock(void) = 0;
		virtual void  unlock(void) = 0;
		PxU32 getStride(void) const { return m_stride; }

	private:
		virtual void  swizzleColor(void *colors, PxU32 stride, PxU32 numColors, RendererVertexBuffer::Format inFormat) = 0;

		virtual void  bind(PxU32 streamID, PxU32 firstVertex) = 0;
		virtual void  unbind(PxU32 streamID) = 0;

		RendererVertexBuffer &operator=(const RendererVertexBuffer &) { return *this; }

	protected:
		void prepareForRender(void);

	protected:
		class SemanticDesc
		{
		public:
			Format format;
			PxU32  offset;
			bool   locked;
		public:
			SemanticDesc(void)
			{
				format = NUM_FORMATS;
				offset = 0;
				locked = false;
			}
		};

	protected:
		const Hint   m_hint;
		PxU32        m_maxVertices;
		PxU32        m_stride;
		bool         m_deferredUnlock;
		SemanticDesc m_semanticDescs[NUM_SEMANTICS];

	private:
		void        *m_lockedBuffer;
		PxU32        m_numSemanticLocks;
	};

} // namespace SampleRenderer

#endif
