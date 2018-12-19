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

#include "stdafx.h"
#include "CPPJPEGWrapper.h"

CPPJPEGWrapper::CPPJPEGWrapper() {
	data = 0;
	x = 0;
	y = 0;
}
CPPJPEGWrapper::~CPPJPEGWrapper() {
	Unload();
}
void CPPJPEGWrapper::Unload() {
	if (data) {
		delete [] data;
		data = 0;
	}
}

unsigned char* CPPJPEGWrapper::GetData() const {
	return data;
}
unsigned CPPJPEGWrapper::GetWidth() {
	return x;
}
unsigned CPPJPEGWrapper::GetHeight() {
	return y;
}
unsigned CPPJPEGWrapper::GetBPP() {
	return bpp;
}
unsigned CPPJPEGWrapper::GetChannels() {
	return channels;
}


bool CPPJPEGWrapper::LoadJPEG(const char* FileName, bool Fast)
{
	Unload();
	FILE* file = fopen(FileName, "rb");  //open the file
	if (!file) return false;
	struct jpeg_decompress_struct info;  //the jpeg decompress info
	struct jpeg_error_mgr err;           //the error handler

	info.err = jpeg_std_error(&err);     //tell the jpeg decompression handler to send the errors to err
	jpeg_create_decompress(&info);       //sets info to all the default stuff

	//if the jpeg file didnt load exit
	if(!file)
	{
		//fprintf(stderr, "Error reading JPEG file %s!!!", FileName);
		//LoadBlackWhiteBorder();
		return false;
	}

	jpeg_stdio_src(&info, file);    //tell the jpeg lib the file we'er reading

	jpeg_read_header(&info, TRUE);   //tell it to start reading it

	//if it wants to be read fast or not
	if(Fast)
	{
		info.do_fancy_upsampling = FALSE;
	}

	jpeg_start_decompress(&info);    //decompress the file

	//set the x and y
	x = info.output_width;
	y = info.output_height;
	channels = info.num_components;

	bpp = channels * 8;

	size = x * y * 3;

	//read turn the uncompressed data into something ogl can read
	data = new unsigned char[size];      //setup data for the data its going to be handling

	unsigned char* p1 = data;
	unsigned char** p2 = &p1;
	int numlines = 0;

	while(info.output_scanline < info.output_height)
	{
		numlines = jpeg_read_scanlines(&info, p2, 1);
		*p2 += numlines * 3 * info.output_width;
	}

	jpeg_finish_decompress(&info);   //finish decompressing this file

	fclose(file);                    //close the file

	return true;
}
