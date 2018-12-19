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



#ifndef APEX_SDK_INTL_H
#define APEX_SDK_INTL_H

/* Framework-internal interface class */

#include "ApexSDK.h"
#include "ResourceProviderIntl.h"
#include "PhysXObjectDescIntl.h"

#include "PsString.h"
#include "PxErrors.h"

#if APEX_CUDA_SUPPORT
namespace nvidia
{
	class PhysXGpuIndicator;
}
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace NvParameterized
{
class Traits;
};

namespace physx
{
namespace pvdsdk
{
	class ApexPvdClient;
}
namespace profile
{
	class PxProfileZone;
}
}

namespace nvidia
{
namespace apex
{

class ModuleIntl;
class Actor;
class ApexActor;
class AuthorableObjectIntl;
class UserOpaqueMesh;

/**
Internal interface to the ApexSDK, available to Modules and Scenes
*/
class ApexSDKIntl : public ApexSDK
{
public:
	/**
	Register an authorable object type with the SDK.  These IDs should be accessable
	from the Module* public interface so users can compare them against game objects
	in callbacks.
	*/
	virtual AuthObjTypeID			registerAuthObjType(const char*, ResID nsid) = 0;
	virtual AuthObjTypeID			registerAuthObjType(const char*, AuthorableObjectIntl* authObjPtr) = 0;
	virtual AuthObjTypeID			registerNvParamAuthType(const char*, AuthorableObjectIntl* authObjPtr) = 0;
	virtual void					unregisterAuthObjType(const char*) = 0;
	virtual void					unregisterNvParamAuthType(const char*) = 0;
	/**
	Query the ResID of an authorable object namespace.  This is useful if you have
	an authorable object class name, but not a module pointer.
	*/
	virtual AuthorableObjectIntl*	getAuthorableObject(const char*) = 0;
	virtual AuthorableObjectIntl*	getParamAuthObject(const char*) = 0;

	virtual AssetAuthoring*	createAssetAuthoring(const char* aoTypeName) = 0;
	virtual AssetAuthoring*	createAssetAuthoring(const char* aoTypeName, const char* name) = 0;
	virtual Asset*			createAsset(AssetAuthoring&, const char*) = 0;
	virtual Asset*			createAsset(NvParameterized::Interface*, const char*) = 0;
	virtual Asset*			createAsset(const char* opaqueMeshName, UserOpaqueMesh* om) = 0;
	virtual void					releaseAsset(Asset&) = 0;
	virtual void					releaseAssetAuthoring(AssetAuthoring&) = 0;
	/**
	Request an ApexActor pointer for an Actor
	*/
	virtual ApexActor*              getApexActor(Actor*) const = 0;

	/**
	When APEX creates a PhysX object and injects it into a PhysX scene, it must
	register that object with the ApexSDK so that the end user can differentiate between
	their own PhysX objects and those created by APEX.  The object descriptor will also
	provide a method for working their way back to the containing APEX data structure.
	*/
#if PX_PHYSICS_VERSION_MAJOR == 3
	virtual PhysXObjectDescIntl* 	createObjectDesc(const Actor*, const PxActor*) = 0;
	virtual PhysXObjectDescIntl* 	createObjectDesc(const Actor*, const PxShape*) = 0;
	virtual PhysXObjectDescIntl* 	createObjectDesc(const Actor*, const PxJoint*) = 0;
	virtual PhysXObjectDescIntl* 	createObjectDesc(const Actor*, const PxCloth*) = 0;
#endif // PX_PHYSICS_VERSION_MAJOR == 3

	/**
	Retrieve an object desc created by createObjectDesc.
	*/
	virtual PhysXObjectDescIntl* getGenericPhysXObjectInfo(const void*) const = 0;

	/**
	When APEX deletes a PhysX object or otherwise removes it from the PhysX scene, it must
	call this function to remove it from the ApexSDK object description cache.
	*/
	virtual void					releaseObjectDesc(void*) = 0;

	/* Utility functions intended to be used internally */
	virtual void reportError(PxErrorCode::Enum code, const char* file, int line, const char* functionName, const char* message, ...) = 0;

	virtual uint32_t getCookingVersion() const = 0;

	virtual void* getTempMemory(uint32_t size) = 0;

	virtual void releaseTempMemory(void* data) = 0;

	/**
	 * ApexScenes and Modules can query the ModuleIntl interfaces from the ApexSDK
	 */
	virtual ModuleIntl** getInternalModules() = 0;

	/**
	 * Returns the internal named resource provider.  The internal interface allows
	 * resource creation and deletion
	 */
	virtual ResourceProviderIntl* getInternalResourceProvider() = 0;

	/**
	 * Allow 3rd party modules distributed as static libraries to link
	 * into the ApexSDK such that the normal ApexSDK::createModule()
	 * will still work correctly.  The 3rd party lib must export a C
	 * function that instantiates their module and calls this function.
	 * The user must call that instantiation function before calling
	 * createModule() for it.
	 */
	virtual void registerExternalModule(Module* nx, ModuleIntl* ni) = 0;

	/**
	 * Allow modules to fetch these ApexSDK global name spaces
	 */
	virtual ResID getMaterialNameSpace() const = 0;
	virtual ResID getOpaqueMeshNameSpace() const = 0;
	virtual ResID getCustomVBNameSpace() const = 0;
	virtual ResID getApexMeshNameSpace() = 0;
	virtual ResID getCollisionGroupNameSpace() const = 0;
	virtual ResID getCollisionGroup128NameSpace() const = 0;
	virtual ResID getCollisionGroup64NameSpace() const = 0;
	virtual ResID getCollisionGroupMaskNameSpace() const = 0;
	virtual ResID getPhysicalMaterialNameSpace() const = 0;
	virtual ResID getAuthorableTypesNameSpace() const = 0;

	/**
	 * Retrieve the user provided render resource manager
	 */
	virtual UserRenderResourceManager* getUserRenderResourceManager() const = 0;

	virtual NvParameterized::Traits* 	getParameterizedTraits() = 0;

	virtual ModuleIntl* getInternalModuleByName(const char* name) = 0;

	/**
	 * Update debug renderer color tables in each apex scene with NvParameterized color table.
	 */
	virtual void updateDebugColorParams(const char* color, uint32_t val) = 0;

	virtual bool getRMALoadMaterialsLazily() = 0;

	virtual pvdsdk::ApexPvdClient* getApexPvdClient() = 0;
	virtual profile::PxProfileZoneManager * getProfileZoneManager() = 0;
	virtual profile::PxProfileZone * getProfileZone() = 0;

	// applications can append strings to the APEX DLL filenames
	virtual const char* getCustomDllNamePostfix() const = 0;
#if PX_WINDOWS_FAMILY
	/**
	 * Return the user-provided appGuid, or the default appGuid if the user didn't provide one.
	 */
	virtual const char* getAppGuid() = 0;
#endif

#if APEX_CUDA_SUPPORT
	virtual PhysXGpuIndicator* registerPhysXIndicatorGpuClient() = 0;
	virtual void unregisterPhysXIndicatorGpuClient(PhysXGpuIndicator* gpuIndicator) = 0;
#endif

	virtual bool isParticlesSupported() const = 0;

	virtual ModuleIntl *getInternalModule(Module *module) = 0;
	virtual Module *getModule(ModuleIntl *module) = 0;

	virtual void enterURR() = 0;
	virtual void leaveURR() = 0;
	virtual void checkURR() = 0;

	PX_FORCE_INLINE	PxU64 getContextId() const { return PxU64(reinterpret_cast<size_t>(this)); }

protected:
	virtual ~ApexSDKIntl() {}

};

/**
Returns global SDK pointer.  Not sure if we can do this.
*/
APEX_API ApexSDKIntl*   CALL_CONV GetInternalApexSDK();

}
} // end namespace nvidia::apex

#define APEX_SPRINTF_S(dest_buf, n, str_fmt, ...) \
	shdfnd::snprintf((char *) dest_buf, n, str_fmt, ##__VA_ARGS__);

// gcc uses names ...s
#define APEX_INVALID_PARAMETER(_A, ...) \
	nvidia::GetInternalApexSDK()->reportError(PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, __FUNCTION__, _A, ##__VA_ARGS__)
#define APEX_INVALID_OPERATION(_A, ...) \
	nvidia::GetInternalApexSDK()->reportError(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, __FUNCTION__, _A, ##__VA_ARGS__)
#define APEX_INTERNAL_ERROR(_A, ...) \
	nvidia::GetInternalApexSDK()->reportError(PxErrorCode::eINTERNAL_ERROR    , __FILE__, __LINE__, __FUNCTION__, _A, ##__VA_ARGS__)
#define APEX_DEBUG_INFO(_A, ...) \
	nvidia::GetInternalApexSDK()->reportError(PxErrorCode::eDEBUG_INFO          , __FILE__, __LINE__, __FUNCTION__, _A, ##__VA_ARGS__)
#define APEX_DEBUG_WARNING(_A, ...) \
	nvidia::GetInternalApexSDK()->reportError(PxErrorCode::eDEBUG_WARNING       , __FILE__, __LINE__, __FUNCTION__, _A, ##__VA_ARGS__)
#define APEX_DEPRECATED() \
	nvidia::GetInternalApexSDK()->reportError(PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, __FUNCTION__, "This method is deprecated")
#define APEX_DEPRECATED_ONCE() \
	{ static bool firstTime = true; if (firstTime) { firstTime = false; APEX_DEPRECATED(); } }


#if PX_DEBUG || PX_CHECKED

namespace nvidia
{
	namespace apex
	{
		class UpdateRenderResourcesScope
		{
		public:
			PX_INLINE	UpdateRenderResourcesScope() { GetInternalApexSDK()->enterURR(); }
			PX_INLINE	~UpdateRenderResourcesScope() { GetInternalApexSDK()->leaveURR(); }
		};
	}
}

# define URR_SCOPE UpdateRenderResourcesScope updateRenderResourcesScope
# define URR_CHECK GetInternalApexSDK()->checkURR()
#else
# define URR_SCOPE
# define URR_CHECK
#endif


#endif // APEX_SDK_INTL_H
