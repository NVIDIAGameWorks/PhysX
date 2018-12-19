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


#include "../SnippetCommon/SnippetCommon.h"

#include "nvparameterized/NvParameterized.h"
#include "nvparameterized/NvParamUtils.h"

#include "particles/ModuleParticles.h"
#include "emitter/EmitterAsset.h"
#include "iofx/IofxAsset.h"
#include "basicios/BasicIosAsset.h"
#include "RenderMeshAsset.h"

#include <legacy/ModuleLegacy.h>

#include "PsString.h"

#include <sys/stat.h>
#include "Shlwapi.h"

using namespace nvidia::apex;

class MyResourceCallback;

ApexSDK*					gApexSDK = NULL;
DummyRenderResourceManager* gDummyRenderResourceManager = NULL;
MyResourceCallback*			gMyResourceCallback = NULL;

LPTSTR gMediaPath;

class MyMaterial
{
public:
	MyMaterial(const char*)
	{
	}
};

class MyResourceCallback : public ResourceCallback
{
public:
	virtual void* requestResource(const char* nameSpace, const char* name)
	{
		void* resource = NULL;

		if (
			!physx::shdfnd::strcmp(nameSpace, EMITTER_AUTHORING_TYPE_NAME)
			|| !physx::shdfnd::strcmp(nameSpace, IOFX_AUTHORING_TYPE_NAME)
			|| !physx::shdfnd::strcmp(nameSpace, BASIC_IOS_AUTHORING_TYPE_NAME)
			|| !physx::shdfnd::strcmp(nameSpace, RENDER_MESH_AUTHORING_TYPE_NAME)
			)
		{
			Asset* asset = 0;

			const char* path = name;

			// does file exists?
			if (isFileExist(path))
			{
				PxFileBuf* stream = gApexSDK->createStream(path, PxFileBuf::OPEN_READ_ONLY);

				if (stream)
				{
					NvParameterized::Serializer::SerializeType serType = gApexSDK->getSerializeType(*stream);
					NvParameterized::Serializer::ErrorType serError;
					NvParameterized::Serializer*  ser = gApexSDK->createSerializer(serType);
					PX_ASSERT(ser);

					NvParameterized::Serializer::DeserializedData data;
					serError = ser->deserialize(*stream, data);

					if (serError == NvParameterized::Serializer::ERROR_NONE && data.size() == 1)
					{
						NvParameterized::Interface* params = data[0];
						asset = gApexSDK->createAsset(params, name);
						PX_ASSERT(asset && "ERROR Creating NvParameterized Asset");
					}
					else
					{
						PX_ASSERT(0 && "ERROR Deserializing NvParameterized Asset");
					}

					stream->release();
					ser->release();
				}
			}
			else
			{
				shdfnd::printFormatted("Can't find file: %s", path);
				return NULL;
			}

			resource = asset;
		}
		else if (!shdfnd::strcmp(nameSpace, APEX_MATERIALS_NAME_SPACE))
		{
			MyMaterial* material = new MyMaterial(name);
			resource = material;
		}
		else
		{
			PX_ASSERT(0 && "Namespace not implemented.");
		}

		PX_ASSERT(resource);
		return resource;
	}

	virtual void  releaseResource(const char* nameSpace, const char*, void* resource)
	{
		if (!shdfnd::strcmp(nameSpace, APEX_MATERIALS_NAME_SPACE))
		{
			MyMaterial* material = (MyMaterial*)resource;
			delete material;
		}
		else
		{
			Asset* asset = (Asset*)resource;
			gApexSDK->releaseAsset(*asset);
		}
	}
};


void initApex()
{
	// Fill out the Apex SDKriptor
	ApexSDKDesc apexDesc;

	// Apex needs an allocator and error stream.  By default it uses those of the PhysX SDK.

	// Let Apex know about our PhysX SDK and cooking library
	apexDesc.physXSDK = gPhysics;
	apexDesc.cooking = gCooking;
	apexDesc.pvd = gPvd;

	// Our custom dummy render resource manager
	gDummyRenderResourceManager = new DummyRenderResourceManager();
	apexDesc.renderResourceManager = gDummyRenderResourceManager;

	// Our custom named resource handler
	gMyResourceCallback = new MyResourceCallback();
	apexDesc.resourceCallback = gMyResourceCallback;
	apexDesc.foundation = gFoundation;

	// Finally, create the Apex SDK
	ApexCreateError error;
	gApexSDK = CreateApexSDK(apexDesc, &error);
	PX_ASSERT(gApexSDK);

#if APEX_MODULES_STATIC_LINK
	nvidia::apex::instantiateModuleParticles();
	nvidia::apex::instantiateModuleLegacy();
#endif

	// Initialize particles module
	ModuleParticles* apexParticlesModule = static_cast<ModuleParticles*>(gApexSDK->createModule("Particles"));
	PX_ASSERT(apexParticlesModule);
	apexParticlesModule->init(*apexParticlesModule->getDefaultModuleDesc());
}

Asset* loadApexAsset(const char* nameSpace, const char* path)
{
	Asset* asset = static_cast<Asset*>(gApexSDK->getNamedResourceProvider()->getResource(nameSpace, path));
	return asset;
}

void releaseAPEX()
{
	gApexSDK->release();
	delete gDummyRenderResourceManager;
	delete gMyResourceCallback;
}

void snippetMain(const char* rootPath)
{
	initPhysX();
	initApex();

	Asset* asset;

	std::string path;
	path.append(rootPath);
	path.append("/snippets/SnippetCommon/");

	std::string file0 = path + "testMeshEmitter4BasicIos6.apx";
	asset = loadApexAsset(EMITTER_AUTHORING_TYPE_NAME, file0.c_str());
	asset->forceLoadAssets();
	gApexSDK->forceLoadAssets();
	asset->release();

	std::string file1 = path + "testMeshEmitter4BasicIos6.apx";
	asset = loadApexAsset(EMITTER_AUTHORING_TYPE_NAME, file1.c_str());
	asset->forceLoadAssets();
	gApexSDK->forceLoadAssets();
	asset->release();

	releaseAPEX();
	releasePhysX();
}
