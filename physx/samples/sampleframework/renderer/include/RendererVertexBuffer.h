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
