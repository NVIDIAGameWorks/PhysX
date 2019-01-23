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

#ifndef RENDERER_INDEXBUFFER_H
#define RENDERER_INDEXBUFFER_H

#include <RendererConfig.h>
#include "RendererInteropableBuffer.h"

namespace SampleRenderer
{

	class RendererIndexBufferDesc;

	class RendererIndexBuffer: public RendererInteropableBuffer
	{
		friend class RendererMesh;
	public:
		enum Format
		{
			FORMAT_UINT16 = 0,
			FORMAT_UINT32,

			NUM_FORMATS
		}_Format;

		enum Hint
		{
			HINT_STATIC = 0,
			HINT_DYNAMIC,
		}_Hint;

	public:
		static PxU32 getFormatByteSize(Format format);

	protected:
		RendererIndexBuffer(const RendererIndexBufferDesc &desc);
		virtual ~RendererIndexBuffer(void);

	public:
		void release(void) { delete this; }

		Hint   getHint(void) const;
		Format getFormat(void) const;
		PxU32  getMaxIndices(void) const;

	public:
		virtual void *lock(void) = 0;
		virtual void  unlock(void) = 0;

	private:
		virtual void bind(void) const = 0;
		virtual void unbind(void) const = 0;

		RendererIndexBuffer &operator=(const RendererIndexBuffer &) { return *this; }

	protected:
		const Hint   m_hint;
		const Format m_format;
		PxU32        m_maxIndices;
	};

} // namespace SampleRenderer

#endif
