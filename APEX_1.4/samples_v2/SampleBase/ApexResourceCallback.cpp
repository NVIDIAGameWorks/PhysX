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


#include "ApexResourceCallback.h"

#include "DestructibleAsset.h"
#include "ClothingAsset.h"
#include "EffectPackageAsset.h"

#include "EmitterAsset.h"
#include "ImpactEmitterAsset.h"
#include "IofxAsset.h"
#include "BasicIosAsset.h"
#include "RenderMeshAsset.h"
#include "ParticleIosAsset.h"

#include <windows.h>
#include "Utils.h"

#include "UserOpaqueMesh.h"

#include "ApexRenderMaterial.h"

#include "PsString.h"

using namespace physx;
using namespace nvidia;

#define PATH_MAX_LEN 512


ApexResourceCallback::ApexResourceCallback()
{
	// search for root folder by default
	addSearchDir(".");
}

void* ApexResourceCallback::requestResource(const char* nameSpace, const char* name)
{
	Asset* asset = 0;

	if(!shdfnd::strcmp(nameSpace, DESTRUCTIBLE_AUTHORING_TYPE_NAME)
		|| !shdfnd::strcmp(nameSpace, CLOTHING_AUTHORING_TYPE_NAME)
		|| !shdfnd::strcmp(nameSpace, EMITTER_AUTHORING_TYPE_NAME)
		|| !shdfnd::strcmp(nameSpace, IMPACT_EMITTER_AUTHORING_TYPE_NAME)
		|| !shdfnd::strcmp(nameSpace, IOFX_AUTHORING_TYPE_NAME)
		|| !shdfnd::strcmp(nameSpace, BASIC_IOS_AUTHORING_TYPE_NAME)
		|| !shdfnd::strcmp(nameSpace, PARTICLE_IOS_AUTHORING_TYPE_NAME)
		|| !shdfnd::strcmp(nameSpace, RENDER_MESH_AUTHORING_TYPE_NAME))
	{
		NvParameterized::Interface* params = deserializeFromFile(name);
		if (params != NULL)
		{
			asset = mApexSDK->createAsset(params, name);
			PX_ASSERT(asset && "ERROR Creating NvParameterized Asset");
		}
	}
	else if (!shdfnd::strcmp(nameSpace, APEX_OPAQUE_MESH_NAME_SPACE))
	{
		NvParameterized::Interface* params = deserializeFromFile("woodChunkMesh");
		if (params != NULL)
		{
			asset = mApexSDK->createAsset(params, "woodChunkMesh");
			PX_ASSERT(asset && "ERROR Creating NvParameterized Asset");
		}
	}
	else if(!shdfnd::strcmp(nameSpace, APEX_MATERIALS_NAME_SPACE))
	{
		// try load as GraphicMaterial first
		if (mModuleParticles)
		{
			const NvParameterized::Interface *graphicMaterialData = mModuleParticles->locateGraphicsMaterialData(name);
			if (graphicMaterialData)
			{
				ApexRenderMaterial* material = new ApexRenderMaterial(this, graphicMaterialData);
				return material;
			}
		}

		// try load from xml file then
		char path[PATH_MAX_LEN];
		const char* exts[] = { "xml" };
		if (findFile(name, std::vector<const char*>(exts, exts + sizeof(exts) / sizeof(exts[0])), path))
		{
			ApexRenderMaterial* material = new ApexRenderMaterial(this, path);
			return material;
		}
	}
	else if (!shdfnd::strcmp(nameSpace, APEX_COLLISION_GROUP_128_NAME_SPACE))
	{
		return NULL;
	}
	else if (!shdfnd::strcmp(nameSpace, APEX_CUSTOM_VB_NAME_SPACE))
	{
		return NULL;
	}

	// try load from particles DB
	if (asset == NULL && mModuleParticles)
	{
		// mModuleParticles
		NvParameterized::Interface *iface = mModuleParticles->locateResource(name, nameSpace);
		if (iface)
		{
			NvParameterized::Interface *copyInterface = NULL;
			iface->clone(copyInterface);	// Create a copy of the parameterize data.
			PX_ASSERT(copyInterface);
			if (copyInterface)
			{
				asset = mApexSDK->createAsset(copyInterface, name);	// Create the asset using this NvParameterized::Inteface data
				PX_ASSERT(asset);
			}
		}
	}

	PX_ASSERT_WITH_MESSAGE(asset, name);
	return asset;
}

NvParameterized::Interface* ApexResourceCallback::deserializeFromFile(const char* name)
{
	NvParameterized::Interface* params = NULL;

	char path[PATH_MAX_LEN];
	const char* exts[] = { "apb", "apx" };
	if (findFile(name, std::vector<const char*>(exts, exts + sizeof(exts) / sizeof(exts[0])), path))
	{
		PxFileBuf* stream = mApexSDK->createStream(path, PxFileBuf::OPEN_READ_ONLY);

		if (stream)
		{
			NvParameterized::Serializer::SerializeType serType = mApexSDK->getSerializeType(*stream);
			NvParameterized::Serializer::ErrorType serError;
			NvParameterized::Serializer* ser = mApexSDK->createSerializer(serType);
			PX_ASSERT(ser);

			NvParameterized::Serializer::DeserializedData data;
			serError = ser->deserialize(*stream, data);

			if (serError == NvParameterized::Serializer::ERROR_NONE && data.size() == 1)
			{
				params = data[0];
			}
			else
			{
				PX_ASSERT(0 && "ERROR Deserializing NvParameterized Asset");
			}

			stream->release();
			ser->release();
		}
	}

	return params;
}


void ApexResourceCallback::releaseResource(const char* nameSpace, const char*, void* resource)
{
	if (!shdfnd::strcmp(nameSpace, DESTRUCTIBLE_AUTHORING_TYPE_NAME) ||
		!shdfnd::strcmp(nameSpace, CLOTHING_AUTHORING_TYPE_NAME))
	{
		Asset* asset = (Asset*)resource;
		mApexSDK->releaseAsset(*asset);
	}
	else if (!shdfnd::strcmp(nameSpace, APEX_MATERIALS_NAME_SPACE))
	{
		delete (ApexRenderMaterial*)resource;
	}
}

void* ApexResourceCallback::requestResourceCustom(CustomResourceNameSpace customNameSpace, const char* name)
{
	std::pair<CustomResourceNameSpace, std::string> key(customNameSpace, name);
	if (mCustomResources.find(key) == mCustomResources.end())
	{
		void *resource = NULL;
		if (customNameSpace == eSHADER_FILE_PATH)
		{
			char* path = new char[PATH_MAX_LEN];
			const char* exts[] = { "hlsl" };
			if (findFile(name, std::vector<const char*>(exts, exts + sizeof(exts) / sizeof(exts[0])), path))
			{
				resource = path;
			}
			else
			{
				PX_ALWAYS_ASSERT_MESSAGE(name);
				delete[] path;
			}
		}
		else if (customNameSpace == eTEXTURE_RESOURCE)
		{
			char path[PATH_MAX_LEN];
			const char* exts[] = { "dds", "tga" };
			if (findFile(name, std::vector<const char*>(exts, exts + sizeof(exts) / sizeof(exts[0])), path))
			{
				TextureResource* textureResource = new TextureResource();
				WCHAR wPath[MAX_PATH];
				MultiByteToWideChar(CP_ACP, 0, path, -1, wPath, MAX_PATH);
				wPath[MAX_PATH - 1] = 0;

				const char* ext = strext(path);
				if (::strcmp(ext, "dds") == 0)
				{
					V(DirectX::LoadFromDDSFile(wPath, DirectX::DDS_FLAGS_NONE, &textureResource->metaData,
						textureResource->image));
				}
				else if (::strcmp(ext, "tga") == 0)
				{
					V(DirectX::LoadFromTGAFile(wPath, &textureResource->metaData,
						textureResource->image));
				}
				else
				{
					PX_ALWAYS_ASSERT_MESSAGE("Unsupported texture extension");
				}
				resource = textureResource;
			}
		}

		if (resource != NULL)
		{
			mCustomResources.emplace(key, resource);
		}
		else
		{
			PX_ALWAYS_ASSERT_MESSAGE(name);
		}
	}
	return mCustomResources.at(key);
}

void ApexResourceCallback::addSearchDir(const char* dir, bool recursive)
{
	uint32_t len = dir && *dir ? (uint32_t)strlen(dir) : 0;
	if(len)
	{
		len++;
		char* searchPath = new char[len];
		shdfnd::strlcpy(searchPath, len, dir);
		SearchDir searcDir = { searchPath, recursive };
		mSearchDirs.push_back(searcDir);
	}
}

ApexResourceCallback::~ApexResourceCallback()
{
	// release search dirs
	for(size_t i = 0; i < mSearchDirs.size(); i++)
	{
		delete[] mSearchDirs[i].path;
	}
	mSearchDirs.clear();

	// release custom resources map
	for (std::map<std::pair<CustomResourceNameSpace, std::string>, void*>::iterator it = mCustomResources.begin(); it != mCustomResources.end(); it++)
	{
		switch ((*it).first.first)
		{
		case eSHADER_FILE_PATH:
			delete (char*)((*it).second);
			break;
		case eTEXTURE_RESOURCE:
			delete (TextureResource*)((*it).second);
			break;
		default:
			PX_ALWAYS_ASSERT();
			break;
		}
	}
	mCustomResources.clear();
}

bool ApexResourceCallback::findFileInDir(const char* fileNameFull, const char* path, bool recursive, char* foundPath)
{
	WIN32_FIND_DATA ffd;
	char tmp[PATH_MAX_LEN];
	shdfnd::snprintf(tmp, sizeof(tmp), "%s\\*", path);
	HANDLE hFind = FindFirstFile(tmp, &ffd);

	if(INVALID_HANDLE_VALUE == hFind)
	{
		return NULL;
	}

	do
	{
		if(0 == nvidia::strcmp(".", ffd.cFileName) || 0 == nvidia::strcmp("..", ffd.cFileName))
			continue;

		if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			char tmp[PATH_MAX_LEN];
			shdfnd::snprintf(tmp, sizeof(tmp), "%s\\%s", path, ffd.cFileName);
			if(findFileInDir(fileNameFull, tmp, recursive, foundPath))
				return true;
		}
		else if(nvidia::stricmp(ffd.cFileName, fileNameFull) == 0)
		{
			shdfnd::snprintf(foundPath, PATH_MAX_LEN, "%s\\%s", path, ffd.cFileName);
			return true;
		}
	} while(FindNextFile(hFind, &ffd) != 0);

	return false;
}

bool ApexResourceCallback::findFile(const char* fileName, std::vector<const char*> exts, char* foundPath)
{
	std::string fileNameOnly = fileName;
	size_t ind = fileNameOnly.find_last_of('/');
	if (ind > 0)
		fileNameOnly = fileNameOnly.substr(ind + 1);

	for(size_t i = 0; i < mSearchDirs.size(); i++)
	{
		const SearchDir& searchDir = mSearchDirs[i];

		for(size_t j = 0; j < exts.size(); j++)
		{
			const char* ext = exts[j];
			const uint32_t fileMaxLen = 128;
			char fileNameFull[fileMaxLen] = { 0 };

			nvidia::shdfnd::snprintf(fileNameFull, fileMaxLen, "%s.%s", fileNameOnly.c_str(), ext);
			if(findFileInDir(fileNameFull, searchDir.path, searchDir.recursive, foundPath))
				return true;
		}

		if (findFileInDir(fileNameOnly.c_str(), searchDir.path, searchDir.recursive, foundPath))
			return true;
	}
	return false;
}
