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

// PHYSX-CHANGES:
//   - Removed dependency on OpenGL. -jdolan
//   - MACOS does not always equate to BIG_ENDIAN... fixed it for all platforms. -jdolan
//   - 64 bit fix for unix based platforms (replaced unsigned long with unsigned int) -sschirm

#ifndef NV_DDS_H
#define NV_DDS_H

#include <string>
#include <deque>
#include <assert.h>
#include <stdio.h>

#include <RendererConfig.h>

#if defined(__APPLE__)
	#if BYTE_ORDER == BIG_ENDIAN
		#define NV_DDS_BIG_ENDIAN
	#endif
#endif

namespace nv_dds
{
    // surface description flags
    const unsigned int DDSF_CAPS           = 0x00000001l;
    const unsigned int DDSF_HEIGHT         = 0x00000002l;
    const unsigned int DDSF_WIDTH          = 0x00000004l;
    const unsigned int DDSF_PITCH          = 0x00000008l;
    const unsigned int DDSF_PIXELFORMAT    = 0x00001000l;
    const unsigned int DDSF_MIPMAPCOUNT    = 0x00020000l;
    const unsigned int DDSF_LINEARSIZE     = 0x00080000l;
    const unsigned int DDSF_DEPTH          = 0x00800000l;

    // pixel format flags
    const unsigned int DDSF_ALPHAPIXELS    = 0x00000001l;
    const unsigned int DDSF_FOURCC         = 0x00000004l;
    const unsigned int DDSF_RGB            = 0x00000040l;
    const unsigned int DDSF_RGBA           = 0x00000041l;

    // dwCaps1 flags
    const unsigned int DDSF_COMPLEX         = 0x00000008l;
    const unsigned int DDSF_TEXTURE         = 0x00001000l;
    const unsigned int DDSF_MIPMAP          = 0x00400000l;

    // dwCaps2 flags
    const unsigned int DDSF_CUBEMAP         = 0x00000200l;
    const unsigned int DDSF_CUBEMAP_POSITIVEX  = 0x00000400l;
    const unsigned int DDSF_CUBEMAP_NEGATIVEX  = 0x00000800l;
    const unsigned int DDSF_CUBEMAP_POSITIVEY  = 0x00001000l;
    const unsigned int DDSF_CUBEMAP_NEGATIVEY  = 0x00002000l;
    const unsigned int DDSF_CUBEMAP_POSITIVEZ  = 0x00004000l;
    const unsigned int DDSF_CUBEMAP_NEGATIVEZ  = 0x00008000l;
    const unsigned int DDSF_CUBEMAP_ALL_FACES  = 0x0000FC00l;
    const unsigned int DDSF_VOLUME          = 0x00200000l;

    // compressed texture types
    const unsigned int FOURCC_DXT1 = 0x31545844l; //(MAKEFOURCC('D','X','T','1'))
    const unsigned int FOURCC_DXT3 = 0x33545844l; //(MAKEFOURCC('D','X','T','3'))
    const unsigned int FOURCC_DXT5 = 0x35545844l; //(MAKEFOURCC('D','X','T','5'))

    struct DXTColBlock
    {
        unsigned short col0;
        unsigned short col1;

        unsigned char row[4];
    };

    struct DXT3AlphaBlock
    {
        unsigned short row[4];
    };

    struct DXT5AlphaBlock
    {
        unsigned char alpha0;
        unsigned char alpha1;
        
        unsigned char row[6];
    };

    struct DDS_PIXELFORMAT
    {
        unsigned int dwSize;
        unsigned int dwFlags;
        unsigned int dwFourCC;
        unsigned int dwRGBBitCount;
        unsigned int dwRBitMask;
        unsigned int dwGBitMask;
        unsigned int dwBBitMask;
        unsigned int dwABitMask;
    };

    struct DDS_HEADER
    {
        unsigned int dwSize;
        unsigned int dwFlags;
        unsigned int dwHeight;
        unsigned int dwWidth;
        unsigned int dwPitchOrLinearSize;
        unsigned int dwDepth;
        unsigned int dwMipMapCount;
        unsigned int dwReserved1[11];
        DDS_PIXELFORMAT ddspf;
        unsigned int dwCaps1;
        unsigned int dwCaps2;
        unsigned int dwReserved2[3];
    };

    enum TextureType
    {
        TextureNone,
        TextureFlat,    // 1D, 2D, and rectangle textures
        Texture3D,
        TextureCubemap
    };
    
    enum TextureFormat
    {
		TextureUnknown=0,
		TextureBGRA,
		TextureBGR,
		TextureLuminance,
		TextureDXT1,
		TextureDXT3,
		TextureDXT5,
    };

    class CSurface
    {
        public:
            CSurface();
            CSurface(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const unsigned char *pixels);
            CSurface(const CSurface &copy);
            CSurface &operator= (const CSurface &rhs);
            virtual ~CSurface();

            operator unsigned char*() const;

            virtual void create(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const unsigned char *pixels);
            virtual void clear();

            inline unsigned int get_width() const { return m_width; }
            inline unsigned int get_height() const { return m_height; }
            inline unsigned int get_depth() const { return m_depth; }
            inline unsigned int get_size() const { return m_size; }

        private:
            unsigned int m_width;
            unsigned int m_height;
            unsigned int m_depth;
            unsigned int m_size;

            unsigned char *m_pixels;   
    };

    class CTexture : public CSurface
    {
        friend class CDDSImage;

        public:
            CTexture();
            CTexture(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const unsigned char *pixels);
            CTexture(const CTexture &copy);
            CTexture &operator= (const CTexture &rhs);
            ~CTexture();

            void create(unsigned int w, unsigned int h, unsigned int d, unsigned int imgsize, const unsigned char *pixels);
            void clear();

            inline const CSurface &get_mipmap(unsigned int index) const
            { 
                assert(!m_mipmaps.empty());
                assert(index < m_mipmaps.size());
                
                return m_mipmaps[index]; 
            }

            inline void add_mipmap(const CSurface &mipmap)
            {
                m_mipmaps.push_back(mipmap);
            }

            inline unsigned int get_num_mipmaps() const { return (unsigned int)m_mipmaps.size(); }

        protected:
            inline CSurface &get_mipmap(unsigned int index)
            { 
                assert(!m_mipmaps.empty());
                assert(index < m_mipmaps.size());

                return m_mipmaps[index]; 
            }

        private:
            std::deque<CSurface> m_mipmaps;
    };

    class CDDSImage
    {
        public:
            CDDSImage();
            ~CDDSImage();

            void create_textureFlat(TextureFormat format, unsigned int components, const CTexture &baseImage);
            void create_texture3D(TextureFormat format, unsigned int components, const CTexture &baseImage);
            void create_textureCubemap(TextureFormat format, unsigned int components,
                                       const CTexture &positiveX, const CTexture &negativeX, 
                                       const CTexture &positiveY, const CTexture &negativeY, 
                                       const CTexture &positiveZ, const CTexture &negativeZ);

            void clear();
            bool load(std::string filename, bool flipImage = true);
            bool load(SampleRenderer::File* fp, bool flipImage = true);
            bool save(std::string filename, bool flipImage = true);
            bool save(SampleRenderer::File* fp, bool flipImage = true);

            inline operator unsigned char*()
            { 
                assert(m_valid);
                assert(!m_images.empty());
                
                return m_images[0]; 
            }

            inline unsigned int get_width() 
            {
                assert(m_valid);
                assert(!m_images.empty());
                
                return m_images[0].get_width(); 
            }

            inline unsigned int get_height()
            {
                assert(m_valid);
                assert(!m_images.empty());
                
                return m_images[0].get_height(); 
            }

            inline unsigned int get_depth()
            {
                assert(m_valid);
                assert(!m_images.empty());
                
                return m_images[0].get_depth(); 
            }

            inline unsigned int get_size()
            {
                assert(m_valid);
                assert(!m_images.empty());

                return m_images[0].get_size();
            }

            inline unsigned int get_num_mipmaps() 
            { 
                assert(m_valid);
                assert(!m_images.empty());

                return m_images[0].get_num_mipmaps(); 
            }

            inline const CSurface &get_mipmap(unsigned int index) const
            { 
                assert(m_valid);
                assert(!m_images.empty());
                assert(index < m_images[0].get_num_mipmaps());

                return m_images[0].get_mipmap(index); 
            }

            inline const CTexture &get_cubemap_face(unsigned int face) const
            {
                assert(m_valid);
                assert(!m_images.empty());
                assert(m_images.size() == 6);
                assert(m_type == TextureCubemap);
                assert(face < 6);

                return m_images[face];
            }

            inline unsigned int get_components() { return m_components; }
            inline TextureFormat get_format() { return m_format; }
            inline TextureType get_type() { return m_type; }

            inline bool is_compressed() 
            { 
                if ((m_format == TextureDXT1) || 
                    (m_format == TextureDXT3) ||
                    (m_format == TextureDXT5))
                    return true; 
                else
                    return false;
            }
            
            inline bool is_cubemap() { return (m_type == TextureCubemap); }
            inline bool is_volume() { return (m_type == Texture3D); }
            inline bool is_valid() { return m_valid; }

            inline bool is_dword_aligned()
            {
                assert(m_valid);

                int dwordLineSize = get_dword_aligned_linesize(get_width(), m_components*8);
                int curLineSize = get_width() * m_components;
                
                return (dwordLineSize == curLineSize);
            }
            
        private:
            unsigned int clamp_size(unsigned int size);
            unsigned int size_dxtc(unsigned int width, unsigned int height);
            unsigned int size_rgb(unsigned int width, unsigned int height);
            inline void swap_endian(void *val);

            // calculates 4-byte aligned width of image
            inline unsigned int get_dword_aligned_linesize(unsigned int width, unsigned int bpp)
            {
                return ((width * bpp + 31) & (unsigned int)(-32)) >> 3;
            }

            void flip(CSurface &surface);
            void flip_texture(CTexture &texture);

            void swap(void *byte1, void *byte2, unsigned int size);
            void flip_blocks_dxtc1(DXTColBlock *line, unsigned int numBlocks);
            void flip_blocks_dxtc3(DXTColBlock *line, unsigned int numBlocks);
            void flip_blocks_dxtc5(DXTColBlock *line, unsigned int numBlocks);
            void flip_dxt5_alpha(DXT5AlphaBlock *block);

            void write_texture(const CTexture &texture, SampleRenderer::File *fp);
            
            TextureFormat m_format;
            unsigned int m_components;
            TextureType m_type;
            bool m_valid;

            std::deque<CTexture> m_images;
    };
}
#endif
