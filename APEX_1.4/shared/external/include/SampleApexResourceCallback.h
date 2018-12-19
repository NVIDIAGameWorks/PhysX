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


#ifndef SAMPLE_APEX_RESOURCE_CALLBACK_H
#define SAMPLE_APEX_RESOURCE_CALLBACK_H

#include <ApexDefs.h>
#include <ApexSDK.h>
#include <PxFiltering.h>
#include <ResourceCallback.h>
#include <PxFileBuf.h>
#include "Find.h"
#include <SampleAsset.h>
#include <vector>

#pragma warning(push)
#pragma warning(disable:4512)

class FilterBits; // forward reference the filter bits class

namespace nvidia
{
namespace apex
{
class ApexSDK;
#if APEX_USE_PARTICLES
class ModuleParticles;
#endif
}
}

namespace SampleRenderer
{
class Renderer;
}

namespace SampleFramework
{
class SampleAssetManager;
}

// TODO: DISABLE ME!!!
#define WORK_AROUND_BROKEN_ASSET_PATHS 1

enum SampleAssetFileType
{
	XML_ASSET,
	BIN_ASSET,
	ANY_ASSET,
};

class SampleApexResourceCallback : public nvidia::apex::ResourceCallback
{
public:
	SampleApexResourceCallback(SampleRenderer::Renderer& renderer, SampleFramework::SampleAssetManager& assetManager);
	virtual				   ~SampleApexResourceCallback(void);

	void					addResourceSearchPath(const char* path);
	void					removeResourceSearchPath(const char* path);
	void					clearResourceSearchPaths();

	void					registerSimulationFilterData(const char* name, const physx::PxFilterData& simulationFilterData);
	void					registerPhysicalMaterial(const char* name, physx::PxMaterialTableIndex physicalMaterial);

	void					registerGroupsMask64(const char* name, nvidia::apex::GroupsMask64& groupsMask);

	void					setApexSupport(nvidia::apex::ApexSDK& apexSDK);

	physx::PxFileBuf*   	findApexAsset(const char* assetName);
	void					findFiles(const char* dir, nvidia::apex::FileHandler& handler);

	void					setAssetPreference(SampleAssetFileType pref)
	{
		m_assetPreference = pref;
	}

	static bool				xmlFileExtension(const char* assetName);
	static const char*		getFileExtension(const char* assetName);

private:
	SampleFramework::SampleAsset*	findSampleAsset(const char* assetName, SampleFramework::SampleAsset::Type type);

#if WORK_AROUND_BROKEN_ASSET_PATHS
	const char*				mapHackyPath(const char* path);
#endif

public:
	virtual void*			requestResource(const char* nameSpace, const char* name);
	virtual void			releaseResource(const char* nameSpace, const char* name, void* resource);

	bool					doesFileExist(const char* filename, const char* ext);
	bool					doesFileExist(const char* filename);
	bool					isFileReadable(const char* fullPath);

protected:
	SampleRenderer::Renderer&				m_renderer;
	SampleFramework::SampleAssetManager&	m_assetManager;
	std::vector<char*>						m_searchPaths;
	std::vector<physx::PxFilterData>		m_FilterDatas;
	FilterBits								*m_FilterBits;

	std::vector<nvidia::apex::GroupsMask64>	m_nxGroupsMask64s;
#if APEX_USE_PARTICLES
	nvidia::apex::ModuleParticles*			mModuleParticles;
#endif
	nvidia::apex::ApexSDK*					m_apexSDK;
	uint32_t							m_numGets;
	SampleAssetFileType						m_assetPreference;
};

#pragma warning(pop)

#endif // SAMPLE_APEX_RESOURCE_CALLBACK_H
