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



#ifndef MODULELEGACY_H_
#define MODULELEGACY_H_

#include "Apex.h"
#include "ApexSDKIntl.h"
#include "ModuleIntl.h"
#include "ModuleBase.h"
#include "ModuleLoader.h"
#include "PsArray.h"
#include "ApexRWLockable.h"

namespace nvidia
{
namespace apex
{

class ModuleLoaderImpl : public ModuleLoader, public ModuleIntl, public ModuleBase, public ApexRWLockable
{
	physx::Array<Module*> mModules; // Loaded modules
	ModuleLoader::ModuleNameErrorMap mCreateModuleError; // Modules error during it creating

	uint32_t getIdx(const char* name) const;

	uint32_t getIdx(Module* module) const;

public:
	APEX_RW_LOCKABLE_BOILERPLATE

	ModuleLoaderImpl(ApexSDKIntl* inSdk)
	{
		mName = "Loader";
		mSdk = inSdk;
		mApiProxy = this;
	}

	Module* loadModule(const char* name);

	void loadModules(const char** names, uint32_t size, Module** modules);

	void loadAllModules();

	const ModuleNameErrorMap& getLoadedModulesErrors() const;

	void loadAllLegacyModules();

	uint32_t getLoadedModuleCount() const;

	Module* getLoadedModule(uint32_t idx) const;

	Module* getLoadedModule(const char* name) const;

	void releaseModule(uint32_t idx);

	void releaseModule(const char* name);

	void releaseModule(Module* module);

	void releaseModules(const char** modules, uint32_t size);

	void releaseModules(Module** modules, uint32_t size);

	void releaseLoadedModules();

protected:

	virtual ~ModuleLoaderImpl() {}

	// ModuleBase's stuff

	void						init(NvParameterized::Interface&) {}

	NvParameterized::Interface* getDefaultModuleDesc()
	{
		return 0;
	}

	void release()
	{
		ModuleBase::release();
	}
	void destroy()
	{
		ModuleBase::destroy();
		delete this;
	}

	const char*					getName() const
	{
		return ModuleBase::getName();
	}

	AuthObjTypeID				getModuleID() const
	{
		return UINT32_MAX;
	}
	RenderableIterator* 	createRenderableIterator(const Scene&)
	{
		return NULL;
	}

	// ModuleIntl's stuff

	ModuleSceneIntl* 				createInternalModuleScene(SceneIntl&, RenderDebugInterface*)
	{
		return NULL;
	}
	void						releaseModuleSceneIntl(ModuleSceneIntl&) {}
	uint32_t				forceLoadAssets()
	{
		return 0;
	}
};

}
} // physx::apex

#endif
