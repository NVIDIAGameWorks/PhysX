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

#ifndef RENDERER_INSTANCEBUFFER_H
#define RENDERER_INSTANCEBUFFER_H

#include <RendererConfig.h>
#include "RendererInteropableBuffer.h"

namespace SampleRenderer
{

	class RendererInstanceBufferDesc;

	class RendererInstanceBuffer: public RendererInteropableBuffer
	{
		friend class RendererMesh;
	public:
		enum Semantic
		{
			SEMANTIC_POSITION = 0,
			SEMANTIC_NORMALX,
			SEMANTIC_NORMALY,
			SEMANTIC_NORMALZ,
			SEMANTIC_VELOCITY_LIFE,// life remain (0-1] is packed into w
			SEMANTIC_DENSITY,
			SEMANTIC_UV_OFFSET,
			SEMANTIC_LOCAL_OFFSET,

			NUM_SEMANTICS
		}_Semantic;

		enum Format
		{
			FORMAT_FLOAT1 = 0,
			FORMAT_FLOAT2,
			FORMAT_FLOAT3,
			FORMAT_FLOAT4,

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
		RendererInstanceBuffer(const RendererInstanceBufferDesc &desc);
		virtual ~RendererInstanceBuffer(void);

	public:
		void release(void) { delete this; }

		Hint   getHint(void) const;
		Format getFormatForSemantic(Semantic semantic) const;
		PxU32  getMaxInstances(void) const;
		PxU32  getOffsetForSemantic(Semantic semantic) const;

		void *lockSemantic(Semantic semantic, PxU32 &stride);
		void  unlockSemantic(Semantic semantic);

		virtual void *lock(void) = 0;
		virtual void  unlock(void) = 0;
		virtual PxU32 getStride(void) const { return m_stride; }
	private:

		virtual void  bind(PxU32 streamID, PxU32 firstInstance) const = 0;
		virtual void  unbind(PxU32 streamID) const = 0;

		RendererInstanceBuffer &operator=(const RendererInstanceBuffer &) { return *this; }

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
		PxU32        m_maxInstances;
		PxU32        m_stride;
		SemanticDesc m_semanticDescs[NUM_SEMANTICS];

	private:
		void        *m_lockedBuffer;
		PxU32        m_numSemanticLocks;
	};

} // namespace SampleRenderer

#endif
