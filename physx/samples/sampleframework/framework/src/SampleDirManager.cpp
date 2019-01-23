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
#include "SampleDirManager.h"
#include "SampleAssetManager.h"
#include "SamplePlatform.h"
#include "PsUtilities.h"
#include "PxTkFile.h"
#include "PsString.h"

using namespace SampleFramework;

static void FixPathSeparator(char* pathBuffer)
{
	char separator = SampleFramework::SamplePlatform::platform()->getPathSeparator()[0];

	const size_t length = strlen(pathBuffer);
	for(size_t i=0; i<length ; ++i)
	{
		if ('/' == pathBuffer[i] || '\\' == pathBuffer[i])
			pathBuffer[i] = separator;
	}
}

SampleDirManager::SampleDirManager(const char* relativePathRoot, bool isReadOnly, int maxRecursion) : mIsReadOnly(isReadOnly)
{
	if(!relativePathRoot || !SampleFramework::searchForPath(relativePathRoot, mPathRoot, PX_ARRAY_SIZE(mPathRoot), isReadOnly, maxRecursion))
	{
		shdfnd::printFormatted("path \"%s\" not found\n", relativePathRoot);
		mPathRoot[0] = '\0';
	}
}

#define MAX_PATH_LENGTH 256

const char* SampleDirManager::getFilePath(const char* relativeFilePath, char* pathBuffer, bool testFileValidity)
{
	PX_ASSERT(pathBuffer);

	strcpy(pathBuffer, getPathRoot());
	strcat(pathBuffer, "/");
	
	strcat(pathBuffer, relativeFilePath);
	if (!mIsReadOnly)
	{
		FixPathSeparator(pathBuffer);
		//strip file from path and make sure the output directory exists
		const char* ptr = strrchr(pathBuffer, '/');
		if (!ptr)
			ptr = strrchr(pathBuffer, '\\');
		
		if (ptr)
		{
			char dir[MAX_PATH_LENGTH];
			assert(MAX_PATH_LENGTH >= strlen(pathBuffer));
			strcpy(dir, pathBuffer);
			dir[ptr - pathBuffer] = '\0';
			FixPathSeparator(dir);
			SampleFramework::SamplePlatform::platform()->makeSureDirectoryPathExists(dir);
		}
	}

	FixPathSeparator(pathBuffer);

	if(testFileValidity)
	{
		File* fp = NULL;
		PxToolkit::fopen_s(&fp, pathBuffer, "rb");
		if(!fp)
		{
			// Kai: user can still get full path in the path buffer (side effect)
			return NULL;
		}
		fclose(fp);
	}

	return pathBuffer;
}
