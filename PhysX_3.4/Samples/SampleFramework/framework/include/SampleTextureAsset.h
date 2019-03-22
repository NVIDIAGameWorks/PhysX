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


#ifndef SAMPLE_TEXTURE_ASSET_H
#define SAMPLE_TEXTURE_ASSET_H

#include <stdio.h>

#include <SampleAsset.h>

namespace SampleRenderer
{
	class Renderer;
	class RendererTexture;
}

namespace SampleFramework
{

	class SampleTextureAsset : public SampleAsset
	{
		friend class SampleAssetManager;

	public:
		enum Type
		{
			DDS,
			TGA,
			BMP,
			PVR,
			TEXTURE_FILE_TYPE_COUNT,
		};

	public:
		SampleTextureAsset(SampleRenderer::Renderer &renderer, File &file, const char *path, Type texType);
		virtual ~SampleTextureAsset(void);

	public:
		SampleRenderer::RendererTexture *getTexture(void);
		const SampleRenderer::RendererTexture *getTexture(void) const;

	public:
		virtual bool isOk(void) const;

	private:
		void loadDDS(SampleRenderer::Renderer &renderer, File &file);
		void loadTGA(SampleRenderer::Renderer &renderer, File &file);
		void loadPVR(SampleRenderer::Renderer &renderer, File &file);
		void loadBMP(SampleRenderer::Renderer &renderer, File &file);

		SampleRenderer::RendererTexture *m_texture;
	};

} // namespace SampleFramework

#endif
