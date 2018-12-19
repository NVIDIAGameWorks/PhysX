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



#ifndef APEX_LEGACY_MODULE
#define APEX_LEGACY_MODULE

#include "nvparameterized/NvParameterizedTraits.h"

#include "ModuleIntl.h"
#include "ModuleBase.h"

namespace nvidia
{
namespace apex
{

struct LegacyClassEntry
{
	uint32_t version;
	uint32_t nextVersion;
	NvParameterized::Factory* factory;
	void (*freeParameterDefinitionTable)(NvParameterized::Traits* t);
	NvParameterized::Conversion* (*createConv)(NvParameterized::Traits*);
	NvParameterized::Conversion* conv;
};

template<typename IFaceT>
class TApexLegacyModule : public IFaceT, public ModuleIntl, public ModuleBase
{
public:
	virtual ~TApexLegacyModule() {}

	// base class methods
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
		releaseLegacyObjects();
		ModuleBase::destroy();
		delete this;
	}

	const char*					getName() const
	{
		return ModuleBase::getName();
	}

	ModuleSceneIntl* 				createInternalModuleScene(SceneIntl&, RenderDebugInterface*)
	{
		return NULL;
	}
	void						releaseModuleSceneIntl(ModuleSceneIntl&) {}
	uint32_t				forceLoadAssets()
	{
		return 0;
	}
	AuthObjTypeID				getModuleID() const
	{
		return UINT32_MAX;
	}
	RenderableIterator* 	createRenderableIterator(const Scene&)
	{
		return NULL;
	}

protected:
	virtual void releaseLegacyObjects() = 0;

	void registerLegacyObjects(LegacyClassEntry* e)
	{
		NvParameterized::Traits* t = mSdk->getParameterizedTraits();
		if (!t)
		{
			return;
		}

		for (; e->factory; ++e)
		{
			t->registerFactory(*e->factory);

			e->conv = e->createConv(t);
			t->registerConversion(e->factory->getClassName(), e->version, e->nextVersion, *e->conv);
		}
	}

	void unregisterLegacyObjects(LegacyClassEntry* e)
	{
		NvParameterized::Traits* t = mSdk->getParameterizedTraits();
		if (!t)
		{
			return;
		}

		for (; e->factory; ++e)
		{
			t->removeConversion(
			    e->factory->getClassName(),
			    e->version,
			    e->nextVersion
			);
			e->conv->release();

			t->removeFactory(e->factory->getClassName(), e->factory->getVersion());

			e->freeParameterDefinitionTable(t);
		}
	}
};

typedef TApexLegacyModule<Module> ApexLegacyModule;

} // namespace apex
} // namespace nvidia

#define DEFINE_CREATE_MODULE(ModuleBase) \
	ApexSDKIntl* gApexSdk = 0; \
	ApexSDK* GetApexSDK() { return gApexSdk; } \
	ApexSDKIntl* GetInternalApexSDK() { return gApexSdk; } \
	APEX_API Module*  CALL_CONV createModule( \
	        ApexSDKIntl* inSdk, \
	        ModuleIntl** niRef, \
	        uint32_t APEXsdkVersion, \
	        uint32_t PhysXsdkVersion, \
	        ApexCreateError* errorCode) \
	{ \
		if (APEXsdkVersion != APEX_SDK_VERSION) \
		{ \
			if (errorCode) *errorCode = APEX_CE_WRONG_VERSION; \
			return NULL; \
		} \
		\
		if (PhysXsdkVersion != PHYSICS_SDK_VERSION) \
		{ \
			if (errorCode) *errorCode = APEX_CE_WRONG_VERSION; \
			return NULL; \
		} \
		\
		gApexSdk = inSdk; \
		\
		ModuleBase *impl = PX_NEW(ModuleBase)(inSdk); \
		*niRef  = (ModuleIntl *) impl; \
		return (Module *) impl; \
	}

#define DEFINE_INSTANTIATE_MODULE(ModuleBase) \
	void instantiate##ModuleBase() \
	{ \
		ApexSDKIntl *sdk = GetInternalApexSDK(); \
		ModuleBase *impl = PX_NEW(ModuleBase)(sdk); \
		sdk->registerExternalModule((Module *) impl, (ModuleIntl *) impl); \
	}

#endif // __APEX_LEGACY_MODULE__
