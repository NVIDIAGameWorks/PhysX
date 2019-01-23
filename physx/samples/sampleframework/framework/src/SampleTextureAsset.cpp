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
#include <SampleTextureAsset.h>

#include <Renderer.h>
#include <RendererTexture2D.h>
#include <RendererTexture2DDesc.h>
#include <RendererMemoryMacros.h>
#include "PxTkBmpLoader.h"

#include "nv_dds.h"
#include "PxTkFile.h"

#ifdef RENDERER_ENABLE_TGA_SUPPORT
# include <targa.h>
#endif

#ifdef RENDERER_ENABLE_PVR_SUPPORT
#include "pvrt.h"
#endif

using namespace SampleFramework;

SampleTextureAsset::SampleTextureAsset(SampleRenderer::Renderer &renderer, File &file, const char *path, Type texType) :
	SampleAsset(ASSET_TEXTURE, path)
{
	m_texture = 0;

	switch(texType)
	{
	case DDS: loadDDS(renderer, file); break;
	case TGA: loadTGA(renderer, file); break;
	case PVR: loadPVR(renderer, file); break;
	case BMP: loadBMP(renderer, file); break;
	default: PX_ASSERT(0 && "Invalid texture type requested"); break;
	}
}

SampleTextureAsset::~SampleTextureAsset(void)
{
	SAFE_RELEASE(m_texture);
}

void SampleTextureAsset::loadDDS(SampleRenderer::Renderer &renderer, File &file) 
{
	nv_dds::CDDSImage ddsimage;
	bool ok = ddsimage.load(&file, false);
	PX_ASSERT(ok);
	if(ok)
	{
		SampleRenderer::RendererTexture2DDesc tdesc;
		nv_dds::TextureFormat ddsformat = ddsimage.get_format();
		switch(ddsformat)
		{
		case nv_dds::TextureBGRA:      tdesc.format = SampleRenderer::RendererTexture2D::FORMAT_B8G8R8A8; break;
		case nv_dds::TextureDXT1:      tdesc.format = SampleRenderer::RendererTexture2D::FORMAT_DXT1;     break;
		case nv_dds::TextureDXT3:      tdesc.format = SampleRenderer::RendererTexture2D::FORMAT_DXT3;     break;
		case nv_dds::TextureDXT5:      tdesc.format = SampleRenderer::RendererTexture2D::FORMAT_DXT5;     break;
		default: break;
 		}
		tdesc.width     = ddsimage.get_width();
		tdesc.height    = ddsimage.get_height();
		// if there is 1 mipmap, nv_dds reports 0
		tdesc.numLevels = ddsimage.get_num_mipmaps()+1;
		PX_ASSERT(tdesc.isValid());
		m_texture = renderer.createTexture2D(tdesc);
		PX_ASSERT(m_texture);
		if(m_texture)
		{
			PxU32 pitch  = 0;
			void *buffer = m_texture->lockLevel(0, pitch);
			PX_ASSERT(buffer);
			if(buffer)
			{
				//PxU32 size = ddsimage.get_size();

				PxU8       *levelDst    = (PxU8*)buffer;
				const PxU8 *levelSrc    = (PxU8*)(unsigned char*)ddsimage;
				const PxU32 levelWidth  = m_texture->getWidthInBlocks();
				const PxU32 levelHeight = m_texture->getHeightInBlocks();
				const PxU32 rowSrcSize  = levelWidth * m_texture->getBlockSize();
				PX_ASSERT(rowSrcSize <= pitch); // the pitch can't be less than the source row size.
				for(PxU32 row=0; row<levelHeight; row++)
				{
					memcpy(levelDst, levelSrc, rowSrcSize);
					levelDst += pitch;
					levelSrc += rowSrcSize;
				}
			}
			m_texture->unlockLevel(0);

			for(PxU32 i=1; i<tdesc.numLevels; i++)
			{
				void *buffer = m_texture->lockLevel(i, pitch);
				PX_ASSERT(buffer);
				if(buffer && pitch )
				{
					const nv_dds::CSurface &surface = ddsimage.get_mipmap(i-1);
					//PxU32 size = surface.get_size();

					PxU8       *levelDst    = (PxU8*)buffer;
					const PxU8 *levelSrc    = (PxU8*)(unsigned char*)surface;
					const PxU32 levelWidth  = SampleRenderer::RendererTexture2D::getFormatNumBlocks(surface.get_width(),  m_texture->getFormat());
					const PxU32 levelHeight = SampleRenderer::RendererTexture2D::getFormatNumBlocks(surface.get_height(), m_texture->getFormat());
					const PxU32 rowSrcSize  = levelWidth * m_texture->getBlockSize();
					PX_ASSERT(rowSrcSize <= pitch); // the pitch can't be less than the source row size.
					for(PxU32 row=0; row<levelHeight; row++)
					{
						memcpy(levelDst, levelSrc, rowSrcSize);
						levelDst += pitch;
						levelSrc += rowSrcSize;
					}
				}
				m_texture->unlockLevel(i);
			}
		}
	}
}

void SampleTextureAsset::loadPVR(SampleRenderer::Renderer& renderer, File& file)
{
#ifdef RENDERER_ENABLE_PVR_SUPPORT	
	fseek(&file, 0, SEEK_END);
	size_t size = ftell(&file);
	fseek(&file, 0, SEEK_SET);
	char* fileBuffer = new char[size+1];
	fread(fileBuffer, 1, size, &file);
	fclose(&file);
	fileBuffer[size] = '\0';
	
	const PVRTextureInfo info(fileBuffer);
	PX_ASSERT(info.data);
	
	SampleRenderer::RendererTexture2DDesc tdesc;
	if (info.glFormat == GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG)
	{
		tdesc.format = SampleRenderer::RendererTexture2D::FORMAT_PVR_4BPP;
	}
	else 
	{
		tdesc.format = SampleRenderer::RendererTexture2D::FORMAT_PVR_2BPP;
	}

	tdesc.width     = info.width;
	tdesc.height    = info.height;	
	tdesc.numLevels = info.mipCount + 1;
	tdesc.data = NULL;
	
	PX_ASSERT(tdesc.isValid());
	m_texture = renderer.createTexture2D(tdesc);
	PX_ASSERT(m_texture);
	if (!info.data)
	{
		delete[] fileBuffer;
		return;
	}
	
	PxU32 pitch  = 0;
	PxU8* levelDst = (PxU8*)m_texture->lockLevel(0, pitch);
	const PxU8* levelSrc = (PxU8*)info.data;

	PX_ASSERT(levelDst && levelSrc);
	{
		PxU32 levelWidth = tdesc.width;
		PxU32 levelHeight = tdesc.height;
		const PxU32 levelSize = m_texture->computeImageByteSize(levelWidth, levelHeight, 1, tdesc.format);
		memcpy(levelDst, levelSrc, levelSize);
		levelSrc += levelSize;
	}
	m_texture->unlockLevel(0);
	
	for(PxU32 i=1; i<tdesc.numLevels; i++)
	{
		PxU8* levelDst = (PxU8*)m_texture->lockLevel(i, pitch);
		PX_ASSERT(levelDst);
		{
			const PxU32 levelWidth  = m_texture->getLevelDimension(tdesc.width, i);
			const PxU32 levelHeight = m_texture->getLevelDimension(tdesc.height, i);
			const PxU32 levelSize  = m_texture->computeImageByteSize(levelWidth, levelHeight, 1, tdesc.format);
			memcpy(levelDst, levelSrc, levelSize);
			levelSrc += levelSize;
		}
		m_texture->unlockLevel(i);
	}
	
	delete[] fileBuffer;
	
#endif
}

void SampleTextureAsset::loadTGA(SampleRenderer::Renderer &renderer, File &file)
{
#ifdef RENDERER_ENABLE_TGA_SUPPORT
	tga_image* image = new tga_image();
	bool ok = (TGA_NOERR == tga_read_from_FILE( image, &file ));

	// flip it to make it look correct in the SampleFramework's renderer
	tga_flip_vert( image );

	PX_ASSERT(ok);
	if( ok )
	{
		SampleRenderer::RendererTexture2DDesc tdesc;
		int componentCount = image->pixel_depth/8;
		if( componentCount == 3 || componentCount == 4 )
		{
			tdesc.format = SampleRenderer::RendererTexture2D::FORMAT_B8G8R8A8;

			tdesc.width     = image->width;
			tdesc.height    = image->height;

			tdesc.numLevels = 1;
			PX_ASSERT(tdesc.isValid());
			m_texture = renderer.createTexture2D(tdesc);
			PX_ASSERT(m_texture);

			if(m_texture)
			{
				PxU32 pitch  = 0;
				void *buffer = m_texture->lockLevel(0, pitch);
				PX_ASSERT(buffer);
				if(buffer)
				{
					PxU8       *levelDst    = (PxU8*)buffer;
					const PxU8 *levelSrc    = (PxU8*)image->image_data;
					const PxU32 levelWidth  = m_texture->getWidthInBlocks();
					const PxU32 levelHeight = m_texture->getHeightInBlocks();
					const PxU32 rowSrcSize  = levelWidth * m_texture->getBlockSize();
					PX_ASSERT(rowSrcSize <= pitch); // the pitch can't be less than the source row size.
					for(PxU32 row=0; row<levelHeight; row++)
					{ 
						if( componentCount == 3 )
						{
							// copy per pixel to handle RBG case, based on component count
							for(PxU32 col=0; col<levelWidth; col++)
							{
								*levelDst++ = levelSrc[0];
								*levelDst++ = levelSrc[1];
								*levelDst++ = levelSrc[2];
								*levelDst++ = 0xFF; //alpha
								levelSrc += componentCount;
							}
						}
						else
						{
							memcpy(levelDst, levelSrc, rowSrcSize);
							levelDst += pitch == 0xFFFFFFFF ?  rowSrcSize : pitch;
							levelSrc += rowSrcSize;
						}
					}
				}
				m_texture->unlockLevel(0);
			}
		}
	}
	tga_free_buffers(image);
	delete image;

#endif /* RENDERER_ENABLE_TGA_SUPPORT */
}

void SampleTextureAsset::loadBMP(SampleRenderer::Renderer &renderer, File &file)
{
	PxToolkit::BmpLoader loader;
	bool ok = loader.loadBmp(&file);
	PX_ASSERT(ok);
	if(ok)
	{
		SampleRenderer::RendererTexture2DDesc tdesc;	
		tdesc.format	= SampleRenderer::RendererTexture2D::FORMAT_B8G8R8A8;
		tdesc.width     = loader.mWidth;
		tdesc.height    = loader.mHeight;
		tdesc.numLevels = 1;
		PX_ASSERT(tdesc.isValid());
		m_texture = renderer.createTexture2D(tdesc);
		PX_ASSERT(m_texture);
		if(m_texture)
		{
			PxU32 pitch  = 0;
			void *buffer = m_texture->lockLevel(0, pitch);
			PX_ASSERT(buffer);
			if(buffer)
			{
				const PxU32						levelWidth  = m_texture->getWidthInBlocks();
				const PxU32						levelHeight = m_texture->getHeightInBlocks();
				SampleRenderer::RendererColor*	levelDst    = (SampleRenderer::RendererColor*)buffer + levelWidth * levelHeight;
				const PxU8*						levelSrc    = (PxU8*)loader.mRGB;
				for(PxU32 row=0; row<levelHeight; row++)
				{
					levelDst -= levelWidth;
					// copy per pixel to handle RBG case
					for(PxU32 col=0; col<levelWidth; col++)
					{
						levelDst[col].r = *levelSrc++;
						levelDst[col].g = *levelSrc++;
						levelDst[col].b = *levelSrc++;
						levelDst[col].a = loader.mHasAlpha ? *levelSrc++ : 0xff;
					}
				}
			}
			m_texture->unlockLevel(0);
		}
	}
}

SampleRenderer::RendererTexture2D *SampleTextureAsset::getTexture(void)
{
	return m_texture;
}

bool SampleTextureAsset::isOk(void) const
{
	return m_texture ? true : false;
}
