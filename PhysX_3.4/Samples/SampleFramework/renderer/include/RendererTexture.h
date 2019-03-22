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

#ifndef RENDERER_TEXTURE_H
#define RENDERER_TEXTURE_H

#include <RendererConfig.h>

namespace SampleRenderer
{

	class RendererTextureDesc;

	class RendererTexture
	{
	public:
		enum Format
		{
			FORMAT_B8G8R8A8 = 0,
			FORMAT_R8G8B8A8,
			FORMAT_A8,
			FORMAT_R32F,

			FORMAT_DXT1,
			FORMAT_DXT3,
			FORMAT_DXT5,

			FORMAT_D16,
			FORMAT_D24S8,

			FORMAT_GXT,

			FORMAT_PVR_2BPP,
			FORMAT_PVR_4BPP,

			NUM_FORMATS
		}_Format;

		enum Filter
		{
			FILTER_NEAREST = 0,
			FILTER_LINEAR,
			FILTER_ANISOTROPIC,

			NUM_FILTERS
		}_Filter;

		enum Addressing
		{
			ADDRESSING_WRAP = 0,
			ADDRESSING_CLAMP,
			ADDRESSING_MIRROR,

			NUM_ADDRESSING
		}_Addressing;

	public:
		static PxU32 computeImageByteSize(PxU32 width, PxU32 height, PxU32 depth, Format format);
		static PxU32 getLevelDimension(PxU32 dimension, PxU32 level);
		static bool  isCompressedFormat(Format format);
		static bool  isDepthStencilFormat(Format format);
		static PxU32 getFormatNumBlocks(PxU32 dimension, Format format);
		static PxU32 getFormatBlockSize(Format format);

	protected:
		RendererTexture(const RendererTextureDesc &desc);
		virtual ~RendererTexture(void);

	public:
		void release(void) { delete this; }

		Format     getFormat(void)      const { return m_format; }
		Filter     getFilter(void)      const { return m_filter; }
		Addressing getAddressingU(void) const { return m_addressingU; }
		Addressing getAddressingV(void) const { return m_addressingV; }
		PxU32      getWidth(void)       const { return m_width; }
		PxU32      getHeight(void)      const { return m_height; }
		PxU32      getDepth(void)       const { return m_depth; }
		PxU32      getNumLevels(void)   const { return m_numLevels; }

		PxU32      getWidthInBlocks(void)  const;
		PxU32      getHeightInBlocks(void) const;
		PxU32      getBlockSize(void)      const;

	public:
		//! pitch is the number of bytes between the start of each row.
		virtual void *lockLevel(PxU32 level, PxU32 &pitch) = 0;
		virtual void  unlockLevel(PxU32 level)             = 0;
		virtual	void  select(PxU32 stageIndex)	           = 0;

	private:
		RendererTexture &operator=(const RendererTexture&) { return *this; }

	private:
		Format     m_format;
		Filter     m_filter;
		Addressing m_addressingU;
		Addressing m_addressingV;
		PxU32      m_width;
		PxU32      m_height;
		PxU32      m_depth;
		PxU32      m_numLevels;
	};

} // namespace SampleRenderer

#endif
