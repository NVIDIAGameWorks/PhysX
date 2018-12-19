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


#ifndef APEX_RESOURCE_CALLBACK_H
#define APEX_RESOURCE_CALLBACK_H

#include "Apex.h"
#include "ModuleParticles.h"

#pragma warning(push)
#pragma warning(disable : 4350)
#include <vector>
#include <string>
#include <map>
#pragma warning(pop)

using namespace nvidia::apex;

class physx::PxFileBuf;

class ApexResourceCallback : public ResourceCallback
{
  public:
	ApexResourceCallback();
	~ApexResourceCallback();

	void setApexSDK(ApexSDK* apexSDK)
	{
		mApexSDK = apexSDK;
	}

	void setModuleParticles(ModuleParticles* moduleParticles)
	{
		mModuleParticles = moduleParticles;
	}

	// ResourceCallback API:
	virtual void* requestResource(const char* nameSpace, const char* name);
	virtual void releaseResource(const char* nameSpace, const char*, void* resource);

	// Internal samples API:
	void addSearchDir(const char* dir, bool recursive = true);
	NvParameterized::Interface* deserializeFromFile(const char* path);

	enum CustomResourceNameSpace
	{
		eSHADER_FILE_PATH,
		eTEXTURE_RESOURCE
	};

	void* requestResourceCustom(CustomResourceNameSpace customNameSpace, const char* name);

  private:
	ApexSDK* mApexSDK;
	ModuleParticles* mModuleParticles;

	bool findFile(const char* fileName, std::vector<const char*> exts, char* foundPath);
	bool findFileInDir(const char* fileNameFull, const char* path, bool recursive, char* foundPath);

	struct SearchDir
	{
		char* path;
		bool recursive;
	};

	std::vector<SearchDir> mSearchDirs;
	std::map<std::pair<CustomResourceNameSpace, std::string>, void*> mCustomResources;
};

#endif