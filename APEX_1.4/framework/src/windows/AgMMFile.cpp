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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "AgMMFile.h"

using namespace nvidia;

AgMMFile::AgMMFile():
mAddr(0), mSize(0), mFileH(0)
{}

AgMMFile::AgMMFile(char *name, unsigned int size, bool &alreadyExists)
{
	this->create(name, size, alreadyExists);
}

void AgMMFile::create(char *name, unsigned int size, bool &alreadyExists)
{
	alreadyExists = false;
	mSize = size;

	mFileH = CreateFileMapping(INVALID_HANDLE_VALUE,	// use paging file
		NULL,											// default security
		PAGE_READWRITE,									// read/write access
		0,												// buffer size (upper 32bits)
		mSize,											// buffer size (lower 32bits)
		name);											// name of mapping object
	if (mFileH == NULL || mFileH == INVALID_HANDLE_VALUE)
	{
		mSize=0;
		mAddr=0;
		return;
	}

	if (ERROR_ALREADY_EXISTS == GetLastError())
	{
		alreadyExists = true;
	}

	mAddr = MapViewOfFile(mFileH,		// handle to map object
		FILE_MAP_READ|FILE_MAP_WRITE,	// read/write permission
		0,                   
		0,                   
		mSize);

	if (mFileH == NULL || mAddr == NULL)
	{
		mSize=0;
		mAddr=0;
		return;
	}
}

void AgMMFile::destroy()
{
	if (!mAddr || !mFileH || !mSize)
		return;

	UnmapViewOfFile(mAddr);
	CloseHandle(mFileH);

	mAddr = 0;
	mFileH = 0;
	mSize = 0;
}

AgMMFile::~AgMMFile()
{
	destroy();
}
