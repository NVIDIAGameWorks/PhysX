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



#include "ApexUsingNamespace.h"
#include "Apex.h"
#include "ApexLegacyModule.h"
#include "ApexRWLockable.h"

#define SAFE_MODULE_RELEASE(x) if ( x ) { ModuleIntl *m = mSdk->getInternalModule(x); PX_ASSERT(m); m->setParent(NULL); m->setCreateOk(false); x->release(); x = NULL; }

namespace nvidia
{
namespace apex
{

#if defined(_USRDLL) || PX_OSX || (PX_LINUX && APEX_LINUX_SHARED_LIBRARIES)
ApexSDKIntl* gApexSdk = 0;
ApexSDK* GetApexSDK()
{
	return gApexSdk;
}
ApexSDKIntl* GetInternalApexSDK()
{
	return gApexSdk;
}
#endif

namespace legacy
{

#define MODULE_CHECK(x) if ( x == module ) { x = NULL; }
#define SAFE_MODULE_NULL(x) if ( x ) { ModuleIntl *m = mSdk->getInternalModule(x); PX_ASSERT(m); m->setParent(NULL); x = NULL;}

class ModuleLegacy : public ApexLegacyModule, public ApexRWLockable
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	ModuleLegacy(ApexSDKIntl* sdk);

	/**
	Notification from ApexSDK when a module has been released
	*/
	virtual void notifyChildGone(ModuleIntl* imodule)
	{
		Module* module = mSdk->getModule(imodule);
		PX_ASSERT(module);
		PX_UNUSED(module);
		// notifyChildGoneMarker
#if PX_PHYSICS_VERSION_MAJOR == 3
#if APEX_USE_PARTICLES
		MODULE_CHECK(mModuleParticlesLegacy);
		MODULE_CHECK(mModuleParticleIOSLegacy);
		MODULE_CHECK(mModuleForceFieldLegacy);
		MODULE_CHECK(mModuleBasicFSLegacy);
		MODULE_CHECK(mModuleTurbulenceFSLegacy);
		MODULE_CHECK(mModuleBasicIOSLegacy);
		MODULE_CHECK(mModuleEmitterLegacy);
		MODULE_CHECK(mModuleIOFXLegacy);
#endif
		MODULE_CHECK(mModuleDestructibleLegacy);
#endif
		MODULE_CHECK(mModuleClothingLegacy);
		MODULE_CHECK(mModuleCommonLegacy);
		MODULE_CHECK(mModuleFrameworkLegacy);
	};

	// This is a notification that the ApexSDK is being released.  During the shutdown process
	// the APEX SDK will automatically release all currently registered modules; therefore we are no longer
	// responsible for releasing these modules ourselves.
	virtual void notifyReleaseSDK()
	{
#if PX_PHYSICS_VERSION_MAJOR == 3
#if APEX_USE_PARTICLES
		// notifyReleaseSDKMarker
		SAFE_MODULE_NULL(mModuleBasicIOSLegacy);
		SAFE_MODULE_NULL(mModuleEmitterLegacy);
		SAFE_MODULE_NULL(mModuleIOFXLegacy);
		SAFE_MODULE_NULL(mModuleParticlesLegacy);
		SAFE_MODULE_NULL(mModuleParticleIOSLegacy);
		SAFE_MODULE_NULL(mModuleForceFieldLegacy);
		SAFE_MODULE_NULL(mModuleBasicFSLegacy);
		SAFE_MODULE_NULL(mModuleTurbulenceFSLegacy);
#endif
		SAFE_MODULE_NULL(mModuleDestructibleLegacy);
#endif
		SAFE_MODULE_NULL(mModuleClothingLegacy);
		SAFE_MODULE_NULL(mModuleCommonLegacy);
		SAFE_MODULE_NULL(mModuleFrameworkLegacy);
	}

protected:
	void releaseLegacyObjects();

private:
	// Add custom conversions here
	// ModulePointerDefinitionMarker
#if PX_PHYSICS_VERSION_MAJOR == 3
#if APEX_USE_PARTICLES
	nvidia::apex::Module*	mModuleBasicIOSLegacy;
	nvidia::apex::Module*	mModuleEmitterLegacy;
	nvidia::apex::Module*	mModuleIOFXLegacy;
	nvidia::apex::Module*	mModuleParticlesLegacy;
	nvidia::apex::Module*	mModuleParticleIOSLegacy;
	nvidia::apex::Module*	mModuleForceFieldLegacy;
	nvidia::apex::Module*	mModuleBasicFSLegacy;
	nvidia::apex::Module*	mModuleTurbulenceFSLegacy;
#endif
	nvidia::apex::Module*	mModuleDestructibleLegacy;
#endif
	nvidia::apex::Module*	mModuleClothingLegacy;
	nvidia::apex::Module*	mModuleCommonLegacy;
	nvidia::apex::Module*	mModuleFrameworkLegacy;
};

}
#if defined(_USRDLL) || PX_OSX || (PX_LINUX && APEX_LINUX_SHARED_LIBRARIES)

APEX_API Module*  CALL_CONV createModule(
    ApexSDKIntl* inSdk,
    ModuleIntl** niRef,
    uint32_t APEXsdkVersion,
    uint32_t PhysXsdkVersion,
    ApexCreateError* errorCode)
{
	if (APEXsdkVersion != APEX_SDK_VERSION)
	{
		if (errorCode)
		{
			*errorCode = APEX_CE_WRONG_VERSION;
		}
		return NULL;
	}

	if (PhysXsdkVersion != PX_PHYSICS_VERSION)
	{
		if (errorCode)
		{
			*errorCode = APEX_CE_WRONG_VERSION;
		}
		return NULL;
	}

	gApexSdk = inSdk;

	legacy::ModuleLegacy* impl = PX_NEW(legacy::ModuleLegacy)(inSdk);
	*niRef  = (ModuleIntl*) impl;
	return (Module*) impl;
}
#else
void instantiateModuleLegacy()
{
	ApexSDKIntl* sdk = GetInternalApexSDK();
	legacy::ModuleLegacy* impl = PX_NEW(legacy::ModuleLegacy)(sdk);
	sdk->registerExternalModule((Module*) impl, (ModuleIntl*) impl);
}
#endif

namespace legacy
{
#if PX_PHYSICS_VERSION_MAJOR == 3

#if APEX_USE_PARTICLES
// instantiateDefinitionMarker
void instantiateModuleBasicIOSLegacy();
void instantiateModuleEmitterLegacy();
void instantiateModuleIOFXLegacy();
void instantiateModuleParticlesLegacy();
void instantiateModuleParticleIOSLegacy();
void instantiateModuleForceFieldLegacy();
void instantiateModuleBasicFSLegacy();
void instantiateModuleTurbulenceFSLegacy();
#endif

void instantiateModuleDestructibleLegacy();
#endif

void instantiateModuleCommonLegacy();
void instantiateModuleFrameworkLegacy();
void instantiateModuleClothingLegacy();

#define MODULE_PARENT(x) if ( x ) { ModuleIntl *m = mSdk->getInternalModule(x); PX_ASSERT(m); m->setParent(this); }

ModuleLegacy::ModuleLegacy(ApexSDKIntl* inSdk)
{
	mName = "Legacy";
	mSdk = inSdk;
	mApiProxy = this;

	// Register legacy stuff

	NvParameterized::Traits* t = mSdk->getParameterizedTraits();
	if (!t)
	{
		return;
	}
#if PX_PHYSICS_VERSION_MAJOR == 3

#if APEX_USE_PARTICLES
	// instantiateCallMarker	
	
	instantiateModuleBasicIOSLegacy();
	mModuleBasicIOSLegacy = mSdk->createModule("BasicIOS_Legacy", NULL);
	PX_ASSERT(mModuleBasicIOSLegacy);
	MODULE_PARENT(mModuleBasicIOSLegacy);

	instantiateModuleEmitterLegacy();
	mModuleEmitterLegacy = mSdk->createModule("Emitter_Legacy", NULL);
	PX_ASSERT(mModuleEmitterLegacy);
	MODULE_PARENT(mModuleEmitterLegacy);

	instantiateModuleIOFXLegacy();
	mModuleIOFXLegacy = mSdk->createModule("IOFX_Legacy", NULL);
	PX_ASSERT(mModuleIOFXLegacy);
	MODULE_PARENT(mModuleIOFXLegacy);

	instantiateModuleParticlesLegacy();
	mModuleParticlesLegacy = mSdk->createModule("Particles_Legacy", NULL);
	PX_ASSERT(mModuleParticlesLegacy);
	MODULE_PARENT(mModuleParticlesLegacy);	
	
	instantiateModuleParticleIOSLegacy();
	mModuleParticleIOSLegacy = mSdk->createModule("ParticleIOS_Legacy", NULL);
	PX_ASSERT(mModuleParticleIOSLegacy);
	MODULE_PARENT(mModuleParticleIOSLegacy);

	instantiateModuleForceFieldLegacy();
	mModuleForceFieldLegacy = mSdk->createModule("ForceField_Legacy", NULL);
	PX_ASSERT(mModuleForceFieldLegacy);
	MODULE_PARENT(mModuleForceFieldLegacy);	

	instantiateModuleBasicFSLegacy();
	mModuleBasicFSLegacy = mSdk->createModule("BasicFS_Legacy", NULL);
	PX_ASSERT(mModuleBasicFSLegacy);
	MODULE_PARENT(mModuleBasicFSLegacy);	

	instantiateModuleTurbulenceFSLegacy();
	mModuleTurbulenceFSLegacy = mSdk->createModule("TurbulenceFS_Legacy", NULL);
	PX_ASSERT(mModuleTurbulenceFSLegacy);
	MODULE_PARENT(mModuleTurbulenceFSLegacy);
#endif
	
	instantiateModuleDestructibleLegacy();
	mModuleDestructibleLegacy = mSdk->createModule("Destructible_Legacy", NULL);
	PX_ASSERT(mModuleDestructibleLegacy);
	MODULE_PARENT(mModuleDestructibleLegacy);

#endif

	instantiateModuleCommonLegacy();
	mModuleCommonLegacy = mSdk->createModule("Common_Legacy", NULL);
	PX_ASSERT(mModuleCommonLegacy);
	MODULE_PARENT(mModuleCommonLegacy);
	
	instantiateModuleFrameworkLegacy();
	mModuleFrameworkLegacy = mSdk->createModule("Framework_Legacy", NULL);
	PX_ASSERT(mModuleFrameworkLegacy);
	MODULE_PARENT(mModuleFrameworkLegacy);

	instantiateModuleClothingLegacy();
	mModuleClothingLegacy = mSdk->createModule("Clothing_Legacy", NULL);
	PX_ASSERT(mModuleClothingLegacy);
	MODULE_PARENT(mModuleClothingLegacy);
}

void ModuleLegacy::releaseLegacyObjects()
{
	//Release legacy stuff

	NvParameterized::Traits* t = mSdk->getParameterizedTraits();
	if (!t)
	{
		return;
	}

#if PX_PHYSICS_VERSION_MAJOR == 3
#if APEX_USE_PARTICLES
		// releaseLegacyObjectsMarker
	SAFE_MODULE_RELEASE(mModuleBasicIOSLegacy);
	SAFE_MODULE_RELEASE(mModuleEmitterLegacy);
	SAFE_MODULE_RELEASE(mModuleIOFXLegacy);
	SAFE_MODULE_RELEASE(mModuleParticlesLegacy);
	SAFE_MODULE_RELEASE(mModuleParticleIOSLegacy);
	SAFE_MODULE_RELEASE(mModuleForceFieldLegacy);
	SAFE_MODULE_RELEASE(mModuleBasicFSLegacy);
	SAFE_MODULE_RELEASE(mModuleTurbulenceFSLegacy);
#endif

	SAFE_MODULE_RELEASE(mModuleDestructibleLegacy);
#endif
	SAFE_MODULE_RELEASE(mModuleCommonLegacy);
	SAFE_MODULE_RELEASE(mModuleFrameworkLegacy);
	SAFE_MODULE_RELEASE(mModuleClothingLegacy);
}

}
}
} // end namespace nvidia::apex
