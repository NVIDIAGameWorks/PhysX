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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.

#include <stdio.h>
#include <stdlib.h>
#include "BmpFile.h"

static bool isBigEndian() { int i = 1; return *((char*)&i)==0; }

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

  return ((unsigned int)b1 << 24) + ((unsigned int)b2 << 16) + ((unsigned int)b3 << 8) + b4;
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
BmpLoaderBuffer::BmpLoaderBuffer() 
{
	mWidth = 0;
	mHeight = 0;
	mRGB = NULL;
}


// -------------------------------------------------------------------
BmpLoaderBuffer::~BmpLoaderBuffer() 
{
	if (mRGB) free(mRGB);
}

// -------------------------------------------------------------------
bool BmpLoaderBuffer::loadFile(const char *filename)
{
	if (mRGB) {
		free(mRGB);
		mRGB = NULL;
	}
	mWidth = 0;
	mHeight = 0;

	FILE *f = fopen(filename, "rb");
	if (!f) 
		return false;

	size_t num;
	BMPHEADER header;
	num = fread(&header, sizeof(BMPHEADER), 1, f);
	if(isBigEndian()) header.Type = endianSwap(header.Type);
	if (num != 1) { fclose(f); return false; }
	if (header.Type != 'MB') { fclose(f); return false; }

	BMPINFO info;
	num = fread(&info, sizeof(BMPINFO), 1, f);
	if (num != 1) { fclose(f); return false; }
	if(isBigEndian()) info.Size = endianSwap(info.Size);
	if(isBigEndian()) info.BitCount = endianSwap(info.BitCount);
	if(isBigEndian()) info.Compression = endianSwap(info.Compression);
	if(isBigEndian()) info.Width = endianSwap(info.Width);
	if(isBigEndian()) info.Height = endianSwap(info.Height);

	if (info.Size != sizeof(BMPINFO)) { fclose(f); return false; }
	if (info.BitCount != 24) { fclose(f); return false; }
	if (info.Compression != BI_RGB) { fclose(f); return false; }

	mWidth = info.Width;
	mHeight = info.Height;
	mRGB = (unsigned char*)malloc(mWidth * mHeight * 3);

	int lineLen = (((info.Width * (info.BitCount>>3)) + 3)>>2)<<2;
	unsigned char *line = (unsigned char *)malloc(lineLen);

	for(int i = info.Height-1; i >= 0; i--) {
		num = fread(line, lineLen, 1, f);
		if (num != 1) { fclose(f); return false; }
		unsigned char *src = line;
		unsigned char *dest = mRGB + i*info.Width*3;
		for(unsigned int j = 0; j < info.Width; j++) {
			unsigned char r,g,b;
			b = *src++; g = *src++; r = *src++;
			*dest++ = r; *dest++ = g; *dest++ = b;
		}
	}

	free(line);
	fclose(f);

	return true;
}

// -------------------------------------------------------------------
bool saveBmpRBG(const char *filename, int width, int height, void *data)
{
	FILE *f = fopen(filename, "wb");
	if (!f) return false;

	// todo : works on pcs only, swap correctly if big endian 
	BMPHEADER header;
	header.Type = 'MB';
	header.Size = sizeof(BMPINFO);
	header.Reserved1 = 0;
	header.Reserved2 = 0;
	header.OffBits = sizeof(BMPHEADER) + sizeof(BMPINFO);
	fwrite(&header, sizeof(BMPHEADER), 1, f);

	BMPINFO info;
	info.Size = sizeof(BMPINFO);
	info.Width = width;
	info.Height = height;
	info.Planes = 1;
	info.BitCount = 24;
	info.Compression = BI_RGB;
	info.XPelsPerMeter = 4000;
	info.YPelsPerMeter = 4000;
	info.ClrUsed = 0;
	info.ClrImportant = 0;
	fwrite(&info, sizeof(info), 1, f);

	// padded to multiple of 4
	int lineLen = (((info.Width * (info.BitCount>>3)) + 3)>>2)<<2;
	info.SizeImage = lineLen * height;

	unsigned char *line = (unsigned char *)malloc(lineLen);

	for(int i = 0; i < height; i++) {
		unsigned char *src = (unsigned char*)data + i*width*3;
		unsigned char *dest = line;
		for(int j = 0; j < width; j++) {
			unsigned char r,g,b;
			r = *src++; g = *src++; b = *src++;
			*dest++ = b; *dest++ = g; *dest++ = r;
		}
		for (int j = 3*width; j < lineLen; j++)
			*dest++ = 0;
		fwrite(line, lineLen, 1, f);
	}

	free(line);
	fclose(f);

	return true;
}