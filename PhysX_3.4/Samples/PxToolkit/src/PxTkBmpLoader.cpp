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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "PxTkBmpLoader.h"
#include "PxTkFile.h"


using namespace PxToolkit;

#define MAKETWOCC(a,b) ( (static_cast<char>(a)) | ((static_cast<char>(b))<< 8) ) 

static bool isBigEndian() { int i = 1; return *(reinterpret_cast<char*>(&i))==0; }

static unsigned short endianSwap(unsigned short nValue)
{
   return (((nValue>> 8)) | (nValue << 8));

}

static unsigned int endianSwap(unsigned int i)
{
  unsigned char b1, b2, b3, b4;

  b1 = i & 255;
  b2 = ( i >> 8 ) & 255;
  b3 = ( i>>16 ) & 255;
  b4 = ( i>>24 ) & 255;

  return (static_cast<unsigned int>(b1) << 24) + (static_cast<unsigned int>(b2) << 16) + (static_cast<unsigned int>(b3) << 8) + b4;
}

// -------------------------------------------------------------------

#pragma pack(1)

struct BMPHEADER {
	unsigned short		Type;
	unsigned int		Size;
	unsigned short		Reserved1;
	unsigned short		Reserved2;
	unsigned int		OffBits;
};

// Only Win3.0 BMPINFO (see later for OS/2)
struct BMPINFO {
	unsigned int		Size;
	unsigned int		Width;
	unsigned int		Height;
	unsigned short		Planes;
	unsigned short		BitCount;
	unsigned int		Compression;
	unsigned int		SizeImage;
	unsigned int		XPelsPerMeter;
	unsigned int		YPelsPerMeter;
	unsigned int		ClrUsed;
	unsigned int		ClrImportant;
};

#pragma pack()

// Compression Type
#define BI_RGB      0L
#define BI_RLE8     1L
#define BI_RLE4     2L

// -------------------------------------------------------------------
BmpLoader::BmpLoader() :
	mWidth		(0),
	mHeight		(0),
	mRGB		(NULL),
	mHasAlpha	(false)
{
}

// -------------------------------------------------------------------
BmpLoader::~BmpLoader() 
{
	if (mRGB) free(mRGB);
}

// -------------------------------------------------------------------
bool BmpLoader::loadBmp(PxFileHandle f)
{
	if (!f) return false;

	if (mRGB) {
		free(mRGB);
		mRGB = NULL;
	}
	mWidth = 0;
	mHeight = 0;

	size_t num;
	BMPHEADER header;
	num = fread(&header, 1, sizeof(BMPHEADER), f);
	if(isBigEndian()) header.Type = endianSwap(header.Type);
	if (num != sizeof(BMPHEADER)) { fclose(f); return false; }
	if (header.Type != MAKETWOCC('B','M')) { fclose(f); return false; }

	BMPINFO info;
	num = fread(&info, 1, sizeof(BMPINFO), f);
	if (num != sizeof(BMPINFO)) { fclose(f); return false; }
	if(isBigEndian()) info.Size = endianSwap(info.Size);
	if(isBigEndian()) info.BitCount = endianSwap(info.BitCount);
	if(isBigEndian()) info.Compression = endianSwap(info.Compression);
	if(isBigEndian()) info.Width = endianSwap(info.Width);
	if(isBigEndian()) info.Height = endianSwap(info.Height);

	if (info.Size != sizeof(BMPINFO)) { fclose(f); return false; }
	if (info.BitCount != 24 && info.BitCount != 32) { fclose(f); return false; }
	if (info.Compression != BI_RGB) { fclose(f); return false; }

	mWidth	= info.Width;
	mHeight	= info.Height;

	int bytesPerPixel = 0;
	if(info.BitCount == 24)
	{
		mHasAlpha = false;
		bytesPerPixel = 3;
	}
	else if(info.BitCount == 32)
	{
		mHasAlpha = true;
		bytesPerPixel = 4;
	}
	else assert(0);

	mRGB = static_cast<unsigned char*>(malloc(mWidth * mHeight * bytesPerPixel));

	int lineLen = (((info.Width * (info.BitCount>>3)) + 3)>>2)<<2;
	unsigned char* line = static_cast<unsigned char*>(malloc(lineLen));

	for(int i = info.Height-1; i >= 0; i--)
	{
		num = fread(line, 1, static_cast<size_t>(lineLen), f);
		if (num != static_cast<size_t>(lineLen)) { fclose(f); return false; }
		unsigned char* src = line;
		unsigned char* dest = mRGB + i*info.Width*bytesPerPixel;
		for(unsigned int j = 0; j < info.Width; j++)
		{
			unsigned char b = *src++;
			unsigned char g = *src++;
			unsigned char r = *src++;
			unsigned char a = mHasAlpha ? *src++ : 0;
			*dest++ = r;
			*dest++ = g;
			*dest++ = b;
			if(mHasAlpha)
				*dest++ = a;
		}
	}

	free(line);
	return true;
}

// -------------------------------------------------------------------
bool BmpLoader::loadBmp(const char *filename)
{
	PxFileHandle f = NULL;
	PxToolkit::fopen_s(&f, filename, "rb");
	bool ret = loadBmp(f); 
	if(f) fclose(f);
	return ret;
}
