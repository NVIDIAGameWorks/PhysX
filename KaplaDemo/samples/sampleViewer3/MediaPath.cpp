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

#if defined(WIN32) || defined(WIN64)
#define NOMINMAX
#include <windows.h>
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "MediaPath.h"

#if defined(_XBOX)

#include <xtl.h>
#include <Xbdm.h>

static bool MediaFileExists(const char *fname)
{
	FILE *fp=fopen(fname,"rb");
	if(fp == NULL)
	{
		return false;
	}
	else
	{
		fclose(fp);
		return true;
	}
}

char* FindMediaFile(const char *fname,char *buff)
{
	strcpy(buff,"d:\\");
	strcat(buff,fname);

	if(MediaFileExists(buff))
		return buff;

	DmMapDevkitDrive();

	strcpy(buff,"DEVKIT:\\");
	strcat(buff,fname);

	if(MediaFileExists(buff))
		return buff;

	char msg[1024];
	sprintf(msg, "Can't find media file: %s\n", fname);
	OutputDebugString(msg);

	return buff;
}

char* GetTempFilePath(char *buff)
{
	strcpy(buff,"d:\\");
	return buff;
}

#elif defined(__CELLOS_LV2__)

#include <sys/paths.h>

char* FindMediaFile(const char *fname,char *buff)
{
	strcpy(buff,SYS_APP_HOME "/");
	strcat(buff,fname);
	return buff;
}

char* GetTempFilePath(char *buff)
{
	strcpy(buff,SYS_APP_HOME "/");
	return buff;
}

#elif defined(WIN32) || defined(WIN64)

static bool MediaFileExists(const char *fname)
{
	FILE *fp=fopen(fname,"rb");
	if(fp == NULL)
	{
		return false;
	}
	else
	{
		fclose(fp);
		return true;
	}
}

char* FindMediaFile(const char* fname, const char* path)
{
	static char fullPath[512];
	strcpy(fullPath, path);
	strcat(fullPath, "/");
	strcat(fullPath, fname);
	return fullPath;
}

char* FindMediaFile(const char *fname,char *buff)
{
	char curPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, curPath);
	strcat(curPath, "\\");
	strcpy(buff, curPath);
	strcat(buff,fname);
	if(MediaFileExists(buff))
		return buff;

	char basePath[MAX_PATH];
	GetModuleFileName(NULL, basePath, MAX_PATH);
	char* pTmp = strrchr(basePath, '\\');
	if(pTmp != NULL){
		*pTmp = 0;
		SetCurrentDirectory(basePath);
		pTmp = strrchr(basePath, '\\');
		if(pTmp != NULL){
			*pTmp = 0;
		}
		pTmp = strrchr(basePath, '\\');
		if(pTmp != NULL){
			*(pTmp + 1) = 0;
		}
	}

	strcpy(buff, basePath);
	strcat(buff,"..\\..\\..\\externals\\GRB\\1.0\\");
	strcat(buff,fname);
	if(MediaFileExists(buff))
		return buff;

	// printf("base path is: %s\n", basePath);
	strcpy(buff, basePath);
	strcat(buff,"Samples\\Media\\");
	strcat(buff,fname);
	if(MediaFileExists(buff))
		return buff;

	strcpy(buff, basePath);
	strcat(buff,"Samples\\compiler\\vc8win32\\");
	strcat(buff,fname);
	if(MediaFileExists(buff))
		return buff;

	strcpy(buff, basePath);
	strcat(buff,"Bin\\win32\\");
	strcat(buff,fname);
	if(MediaFileExists(buff))
		return buff;

	strcpy(buff,fname);
	if(MediaFileExists(buff))
		return buff;

	printf("Can't find media file: %s\n", fname);

	strcpy(buff,fname);
	return buff;
}

char* GetTempFilePath(char *buff)
{
	strcpy(buff,"");
	return buff;
}

#elif defined(LINUX)

static bool MediaFileExists(const char *fname)
{
	FILE *fp=fopen(fname,"rb");
	if(fp == NULL)
	{
		return false;
	}
	else
	{
		fclose(fp);
		return true;
	}
}

char* FindMediaFile(const char *fname,char *buff)
{
	strcpy(buff,fname);
	if(MediaFileExists(buff))
		return buff;

	strcpy(buff,"../../Samples/Media/");
	strcat(buff,fname);
	if(MediaFileExists(buff))
		return buff;

	strcpy(buff,"../../../Media/");
	strcat(buff,fname);
	if(MediaFileExists(buff))
		return buff;

	strcpy(buff,"../../Media/");
	strcat(buff,fname);
	if(MediaFileExists(buff))
		return buff;

	strcpy(buff,"Samples/Media/");
	strcat(buff,fname);
	if(MediaFileExists(buff))
		return buff;

	printf("Can't find media file: %s\n", fname);

	strcpy(buff,fname);
	return buff;
}

char* GetTempFilePath(char *buff)
{
	strcpy(buff,"");
	return buff;
}
#elif defined(__PPCGEKKO__)

#include <revolution/dvd.h>

static bool MediaFileExists(const char *fname)
{
	DVDFileInfo fileInfo;
	
	if(DVDOpen(fname,&fileInfo))
	{
		DVDClose(&fileInfo);
		return true;
	}

	return false;

}

char* FindMediaFile(const char *fname,char *buff)
{
	strcpy(buff,fname);
	if(MediaFileExists(buff))
		return buff;
	
	strcpy(buff,"/Media/");
	strcat(buff,fname);
	if(MediaFileExists(buff))
		return buff;
	
	strcpy(buff,"../../Samples/Media/");
	strcat(buff,fname);
	if(MediaFileExists(buff))
		return buff;

	strcpy(buff,"../../../Media/");
	strcat(buff,fname);
	if(MediaFileExists(buff))
		return buff;

	strcpy(buff,"../../Media/");
	strcat(buff,fname);
	if(MediaFileExists(buff))
		return buff;

	strcpy(buff,"Samples/Media/");
	strcat(buff,fname);
	if(MediaFileExists(buff))
		return buff;

	printf("Can't find media file: %s\n", fname);

	strcpy(buff,fname);
	return buff;
}

char* GetTempFilePath(char *buff)
{
	strcpy(buff,"");
	return buff;
}

#endif
