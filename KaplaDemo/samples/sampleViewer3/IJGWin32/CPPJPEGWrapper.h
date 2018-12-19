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

#pragma once
#include <cstdio>
#include <cstdlib>
#include "jpeglib.h"
#include "jerror.h"

class CPPJPEGWrapper{
public:
	CPPJPEGWrapper();
	~CPPJPEGWrapper();
	void Unload();

	unsigned char* GetData() const;
	unsigned GetWidth();
	unsigned GetHeight();
	unsigned GetBPP();
	unsigned GetChannels();


	bool LoadJPEG(const char* FileName, bool Fast = true);
protected:
	//if the jpeglib stuff isnt after I think stdlib then it wont work just put it at the end
	unsigned long x;
	unsigned long y;
	unsigned short int bpp; //bits per pixels   unsigned short int
	unsigned char* data;             //the data of the image
	unsigned int ID;                //the id ogl gives it
	unsigned long size;     //length of the file
	int channels;      //the channels of the image 3 = RGA 4 = RGBA


};
