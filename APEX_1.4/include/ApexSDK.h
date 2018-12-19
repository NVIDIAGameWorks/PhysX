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



#ifndef APEX_SDK_H
#define APEX_SDK_H

/*!
\file
\brief APEX SDK classes
*/

#include "ApexDefs.h"
#include "ApexUsingNamespace.h"
#include "ApexDesc.h"
#include "ApexInterface.h"

#include "nvparameterized/NvSerializer.h"
#include "nvparameterized/NvParameterized.h"
#include "nvparameterized/NvParameterizedTraits.h"
#include "Shape.h"


namespace RENDER_DEBUG
{
	class RenderDebugInterface;
};

namespace physx
{
	class PxFoundation;
	class PxCpuDispatcher;

	namespace profile
	{
		class PxProfileZoneManager;
	}
	class PxPvd;
}

//! \brief nvidia namespace
namespace nvidia
{
//! \brief apex namespace
namespace apex
{

class UserRenderResourceManager;
class Scene;
class SceneDesc;
class AssetPreviewScene;
class RenderMeshAsset;
class RenderMeshAssetAuthoring;
class Module;
class PhysXObjectDesc;
class ResourceProvider;
class ResourceCallback;
class RenderDebugInterface;
class Asset;
class AssetAuthoring;
class ApexSDKCachedData;
class RenderMeshActor;

PX_PUSH_PACK_DEFAULT

/**
\brief 64-bit mask used for collision filtering between IOSes and Field Samplers.
*/
class GroupsMask64
{
public:
	/**
	\brief Default constructor.
	*/
	PX_INLINE GroupsMask64() 
	{
		bits0 = bits1 = 0;
	}

	/**
	\brief Constructor to set filter data initially.
	*/
	PX_INLINE GroupsMask64(uint32_t b0, uint32_t b1) : bits0(b0), bits1(b1) {}

	/**
	\brief (re)sets the structure to the default.	
	*/
	PX_INLINE void setToDefault()
	{
		*this = GroupsMask64();
	}

	/**	\brief Mask bits. */
	uint32_t		bits0;
	/**	\brief Mask bits. */
	uint32_t		bits1;
};

/**
\brief Collision groups filtering operations.
*/
struct GroupsFilterOp
{
	/**
	\brief Enum of names for group filtering operations.
	*/
	enum Enum
	{
		AND,
		OR,
		XOR,
		NAND,
		NOR,
		NXOR,
		SWAP_AND
	};
};

/**
\brief Integer values, each representing a unique authorable object class/type
*/
typedef unsigned int AuthObjTypeID;

/**
\brief Descriptor class for ApexSDK
*/
class ApexSDKDesc : public ApexDesc
{
public:
	/**
	\brief The PxFoundation you are building with
	*/
	PxFoundation* foundation;

	/**
	\brief The PhysX SDK version you are building with

	A particular APEX build will be against a particular PhysX version except the case physXSDKVersion = 0.
	If physXSDKVersion = 0 APEX will be built without PhysX. 
	Versioning the PhysX API will require versioning of APEX.
	*/
	uint32_t physXSDKVersion;

#if PX_PHYSICS_VERSION_MAJOR == 3
	/**
	\brief Pointer to the physX sdk (PhysX SDK version specific structure)
	*/
	PxPhysics* physXSDK;

	/**
	\brief Pointer to the cooking interface (PhysX SDK version specific structure)
	*/
	PxCooking* cooking;
#endif

	/**
	\brief Pointer to PVD (optional)
	*/
	PxPvd*	pvd;

	/**
	\brief User defined interface for creating renderable resources.
	*/
	UserRenderResourceManager* renderResourceManager;

	/**
	\brief Pointer to a user-callback for unresolved named resources.

	This function will be called by APEX when
	an unresolved named resource is requested.  The function will be called at most once for each named
	resource.  The function must directly return the pointer to the resource or NULL.
	*/
	ResourceCallback* resourceCallback;

	/**
	\brief Path to APEX module DLLs (Windows Only)

	If specified, this string will be prepended to the APEX module names prior to calling LoadLibrary() to load them.  You can
	use this mechanism to ship the APEX module DLLS into their own subdirectory.  For example:  "APEX/"
	*/
	const char* dllLoadPath;

	/**
	\brief Wireframe material name

	If specified, this string designates the material to use when rendering wireframe data.
	It not specified, the default value is "ApexWireframe"
	*/
	const char* wireframeMaterial;

	/**
	\brief Solid material name

	If specified, this string designates the material to use when rendering solid shaded (per vertex color) lit triangles.
	If not specified, the default value is "ApexSolidShaded"
	*/
	const char* solidShadedMaterial;

	/**
	\brief Delays material loading

	Any RenderMeshActor will issue callbacks to the ResourceCallback to create one or more materials. This happens either
	at actor creation or lazily during updateRenderResource upon necessity.
	Additionally it will call UserRenderResourceManager::getMaxBonesPerMaterial for the freshly created material right
	after a successful material creation.
	If this is set to true, the updateRenderResource calls should not run asynchronously.
	*/
	bool renderMeshActorLoadMaterialsLazily;

	/**
	\brief APEX module DLL name postfix (Windows Only)

	If specified, this string will be appended to the APEX module DLL names prior to calling LoadLibrary() to load them.  You can
	use this mechanism to load the APEX module DLLS for two different PhysX SDKs in the same process (DCC tools).

	For example, the APEX_Destructible_x86.dll is renamed to APEX_Destructible_x86_PhysX-2.8.4.dll.
	dllNamePostfix would be "_PhysX-2.8.4"
	*/
	const char* dllNamePostfix;

	/**
	\brief Application-specific GUID (Windows Only)

	Provide an optional appGuid if you have made local modifications to your APEX DLLs.
	Application GUIDs are available from NVIDIA. For non-Windows platforms the appGuid
	is ignored.
	*/
	const char* appGuid;


	/**
	\brief Sets the resource provider's case sensitive mode.

	\note This is a reference to the ResourceProvider string lookups
	*/
	bool resourceProviderIsCaseSensitive;

	/**
	\brief Sets the number of physX object descriptor table entries that are allocated when it gets full.

	*/
	uint32_t physXObjDescTableAllocationIncrement;

	/**
	\brief Enables warnings upon detection of concurrent access to the APEX SDK

	*/
	bool enableConcurrencyCheck;

	/**
	\brief constructor sets to default.
	*/
	PX_INLINE ApexSDKDesc() : ApexDesc()
	{
		init();
	}

	/**
	\brief (re)sets the structure to the default.
	*/
	PX_INLINE void setToDefault()
	{
		ApexDesc::setToDefault();
		init();
	}

	/**
	\brief Returns true if the descriptor is valid.
	\return true if the current settings are valid
	*/
	PX_INLINE bool isValid() const
	{
		bool retVal = ApexDesc::isValid();

		if (foundation == NULL)
		{
			return false;
		}
#if PX_PHYSICS_VERSION_MAJOR == 3
		if (physXSDK == NULL)
		{
			return false;
		}
		if (cooking == NULL)
		{
			return false;
		}
		if (physXSDKVersion != PX_PHYSICS_VERSION)
		{
			return false;
		}
#endif
		return retVal;
	}

private:
	PX_INLINE void init()
	{
		renderResourceManager = NULL;
		foundation = NULL;
#if PX_PHYSICS_VERSION_MAJOR == 3
		physXSDKVersion = PX_PHYSICS_VERSION;
		physXSDK = NULL;
		cooking = NULL;
#endif
		pvd = NULL;
		resourceCallback = NULL;
		dllLoadPath = NULL;
		solidShadedMaterial = "ApexSolidShaded";
		wireframeMaterial = "ApexWireframe";
		renderMeshActorLoadMaterialsLazily = true;
		dllNamePostfix = NULL;
		appGuid = NULL;
		resourceProviderIsCaseSensitive = false;
		physXObjDescTableAllocationIncrement = 128;
		enableConcurrencyCheck = false;
	}
};


/**
\brief These errors are returned by the CreateApexSDK() and ApexSDK::createModule() functions
*/
enum ApexCreateError
{
	/**
	\brief No errors occurred when creating the Physics SDK.
	*/
	APEX_CE_NO_ERROR = 0,

	/**
	\brief Unable to find the libraries.
	For statically linked APEX, a module specific instantiate function must be called
	prior to the createModule call.
	*/
	APEX_CE_NOT_FOUND = 1,

	/**
	\brief The application supplied a version number that does not match with the libraries.
	*/
	APEX_CE_WRONG_VERSION = 2,

	/**
	\brief The supplied descriptor is invalid.
	*/
	APEX_CE_DESCRIPTOR_INVALID = 3,

	/**
	\brief This module cannot be created by the application, it is created via a parent module use as APEX_Particles or APEX_Legacy
	*/
	APEX_CE_CREATE_NO_ALLOWED = 4,

};


/**
\brief The ApexSDK abstraction. Manages scenes and modules.
*/
class ApexSDK : public ApexInterface
{
public:
	/**
	\brief Create an APEX Scene
	*/
	virtual Scene* createScene(const SceneDesc&) = 0;

	/**
	\brief Release an APEX Scene
	*/
	virtual void releaseScene(Scene*) = 0;

	/**
	\brief Create an APEX Asset Preview Scene
	*/
	virtual AssetPreviewScene* createAssetPreviewScene() = 0;

	/**
	\brief Release an APEX Asset Preview Scene
	*/
	virtual void releaseAssetPreviewScene(AssetPreviewScene* nxScene) = 0;

	/**
	\brief Create/Load a module
	*/
	virtual Module* createModule(const char* name, ApexCreateError* err = NULL) = 0;

#if PX_PHYSICS_VERSION_MAJOR == 0
	/**
	 \brief Allocates a CpuDispatcher with a thread pool of the specified number of threads

	 If numThreads is zero, the thread pool will use the default number of threads for the
	 current platform.
	*/
	virtual physx::PxCpuDispatcher* createCpuDispatcher(uint32_t numThreads = 0) = 0;
	/**
	 \brief Releases a CpuDispatcher
	*/
	virtual void releaseCpuDispatcher(physx::PxCpuDispatcher& cd) = 0;
#endif

#if PX_PHYSICS_VERSION_MAJOR == 3
	/**
	\brief Return an object describing how APEX is using the PhysX Actor.
	\return NULL if PhysX Actor is not owned by APEX.
	*/
	virtual const PhysXObjectDesc* getPhysXObjectInfo(const PxActor* actor) const = 0;

	/**
	\brief Return an object describing how APEX is using the PhysX Shape.
	\return NULL if PhysX Shape is not owned by APEX.
	*/
	virtual const PhysXObjectDesc* getPhysXObjectInfo(const PxShape* shape) const = 0;

	/**
	\brief Return an object describing how APEX is using the PhysX Joint.
	\return NULL if PhysX Joint is not owned by APEX.
	*/
	virtual const PhysXObjectDesc* getPhysXObjectInfo(const PxJoint* joint) const = 0;

	/**
	\brief Return an object describing how APEX is using the PhysX Cloth.
	\return NULL if PhysX Cloth is not owned by APEX.
	*/
	virtual const PhysXObjectDesc* getPhysXObjectInfo(const PxCloth* cloth) const = 0;

	/**
	\brief Returns the cooking interface.
	*/
	virtual PxCooking* getCookingInterface() = 0;
#endif

	/**
	\brief Return the user error callback.
	\deprecated Use getErrorCallback() instead.
	*/
	PX_DEPRECATED virtual PxErrorCallback* getOutputStream() = 0;

	/**
	\brief Return the user error callback.
	*/
	virtual PxErrorCallback* getErrorCallback() const = 0;

	/**
	\brief Return the user allocator callback.
	*/
	virtual PxAllocatorCallback* getAllocator() const = 0;

	/**
	\brief Return the named resource provider.
	*/
	virtual ResourceProvider* getNamedResourceProvider() = 0;

	/**
	\brief Return an APEX PxFileBuf instance.  For use by APEX
	tools and samples, not by APEX internally. Internally,	APEX will use an
	PxFileBuf directly provided by the user.
	*/
	virtual PxFileBuf* createStream(const char* filename, PxFileBuf::OpenMode mode) = 0;

	/**
	\brief Return a PxFileBuf which reads from a buffer in memory.
	*/
	virtual PxFileBuf* createMemoryReadStream(const void* mem, uint32_t len) = 0;

	/**
	\brief Return a PxFileBuf which writes to memory.
	*/
	virtual PxFileBuf* createMemoryWriteStream(uint32_t alignment = 0) = 0;

	/**
	\brief Return the address and length of the contents of a memory write buffer stream.
	*/
	virtual const void* getMemoryWriteBuffer(PxFileBuf& stream, uint32_t& len) = 0;

	/**
	\brief Release a previously created PxFileBuf used as a read stream
	*/
	virtual void releaseMemoryReadStream(PxFileBuf& stream) = 0;

	/**
	\brief Release a previously created PxFileBuf used as a write stream
	*/
	virtual void releaseMemoryWriteStream(PxFileBuf& stream) = 0;

#if PX_PHYSICS_VERSION_MAJOR == 3
	/**
	\brief Return the PhysX SDK.
	*/
	virtual PxPhysics* getPhysXSDK() = 0;
#endif

	/**
	\brief Return the number of modules.
	*/
	virtual uint32_t getNbModules() = 0;

	/**
	\brief Return an array of module pointers.
	*/
	virtual Module** getModules() = 0;

	/**
	\brief Release a previously loaded module
	*/
	virtual void releaseModule(Module* module) = 0;

	/**
	\brief Creates an ApexDebugRender interface
	*/
	virtual RenderDebugInterface* createApexRenderDebug(RENDER_DEBUG::RenderDebugInterface* iface, bool useRemoteDebugVisualization = false) = 0;

	/**
	\brief Releases an ApexDebugRender interface
	*/
	virtual void releaseApexRenderDebug(RenderDebugInterface& debug) = 0;

	/**
	\brief Creates an ApexSphereShape interface
	*/
	virtual SphereShape* createApexSphereShape() = 0;

	/**
	\brief Creates an ApexCapsuleShape interface
	*/
	virtual CapsuleShape* createApexCapsuleShape() = 0;

	/**
	\brief Creates an ApexBoxShape interface
	*/
	virtual BoxShape* createApexBoxShape() = 0;

	/**
	\brief Creates an ApexHalfSpaceShape interface
	*/
	virtual HalfSpaceShape* createApexHalfSpaceShape() = 0;

	/**
	\brief Release an Shape interface
	*/
	virtual void releaseApexShape(Shape& shape) = 0;

	/**
	\brief Return the number of assets force loaded by all of the existing APEX modules
	*/
	virtual uint32_t forceLoadAssets() = 0;

	/**
	\brief Get a list of the APEX authorable types.

	The memory for the strings returned is owned by the APEX SDK and should only be read, not written or freed.
	Returns false if 'inCount' is not large enough to contain all of the names.
	*/
	virtual bool getAuthorableObjectNames(const char** authTypeNames, uint32_t& outCount, uint32_t inCount) = 0;

	/**
	\brief Get the a pointer to APEX's instance of the ::NvParameterized::Traits class
	*/
	virtual ::NvParameterized::Traits* getParameterizedTraits() = 0;

	/**
	\brief Creates an APEX asset from an AssetAuthoring object.
	*/
	virtual Asset* createAsset(AssetAuthoring&, const char* name) = 0;

	/**
	\brief Creates an APEX asset from a parameterized object.
	\note The NvParameterized::Interface object's ownership passes to the asset, if a copy is needed, use the NvParameterized::Interface::copy() method
	*/
	virtual Asset* createAsset(::NvParameterized::Interface*, const char* name) = 0;

	/**
	\brief Release an APEX asset
	*/
	virtual void releaseAsset(Asset&) = 0;

	/**
	\brief Creates an APEX asset authoring object
	*/
	virtual AssetAuthoring* createAssetAuthoring(const char* authorTypeName) = 0;

	/**
	\brief Create an APEX asset authoring object with a name
	*/
	virtual AssetAuthoring* createAssetAuthoring(const char* authorTypeName, const char* name) = 0;

	/**
	\brief Create an APEX asset authoring object from a parameterized object.
	*/
	virtual AssetAuthoring* createAssetAuthoring(::NvParameterized::Interface*, const char* name) = 0;

	/**
	\brief Releases an APEX asset authoring object
	*/
	virtual void releaseAssetAuthoring(AssetAuthoring&) = 0;

	/**
	\brief Returns the scene data cache for actors in the scene
	*/
	virtual ApexSDKCachedData& getCachedData() const = 0;

	/**
	\brief Create a Serialization object
	*/
	virtual ::NvParameterized::Serializer* createSerializer(::NvParameterized::Serializer::SerializeType type) = 0;

	/**
	\brief Create a Serialization object from custom traits
	*/
	virtual ::NvParameterized::Serializer* createSerializer(::NvParameterized::Serializer::SerializeType type, ::NvParameterized::Traits* traits) = 0;

	/**
	\brief Figure out whether a given chunk of data was xml or binary serialized.
	To properly detect XML formats, 32 bytes of data are scanned, so dlen should be at least 32 bytes
	*/
	virtual ::NvParameterized::Serializer::SerializeType getSerializeType(const void* data, uint32_t dlen) = 0;

	/**
	\brief Figure out whether a given chunk of data was xml or binary serialized.
	*/
	virtual ::NvParameterized::Serializer::SerializeType getSerializeType(PxFileBuf& stream) = 0;

	/**
	\brief Find native platform for a given chunk of data.
	*/
	virtual NvParameterized::Serializer::ErrorType getSerializePlatform(PxFileBuf& stream, NvParameterized::SerializePlatform& platform) = 0;

	/**
	\brief Find native platform for a given chunk of data.
	\note To properly detect platform 64 bytes of data are scanned, so dlen should be at least 64 bytes
	*/
	virtual NvParameterized::Serializer::ErrorType getSerializePlatform(const void* data, uint32_t dlen, NvParameterized::SerializePlatform& platform) = 0;

	/**
	\brief Get current (native) platform
	*/
	virtual void getCurrentPlatform(NvParameterized::SerializePlatform& platform) const = 0;

	/**
	\brief Get platform from human-readable name
	\param [in]		name		platform name
	\param [out]	platform	Platform corresponding to name
	\return Success

	Name format: compiler + compiler version (if needed) + architecture.
	Supported names: VcWin32, VcWin64, VcXboxOne, GccPs4, AndroidARM, GccLinux32, GccLinux64, GccOsX32, Pib.
	*/
	virtual bool getPlatformFromString(const char* name, NvParameterized::SerializePlatform& platform) const = 0;

	/**
	\brief Get canonical name for platform
	*/
	virtual const char* getPlatformName(const NvParameterized::SerializePlatform& platform) const = 0;

	/**
	\brief Gets NvParameterized debug rendering color parameters.
	Modiying NvParameterized debug colors automatically updates color table in debug renderer
	*/
	virtual ::NvParameterized::Interface* getDebugColorParams() const = 0;

	/**
	\brief Gets a string for the material to use when rendering wireframe data.
	*/
	virtual const char* getWireframeMaterial() = 0;

	/**
	\brief Gets a string for the material to use when rendering solid shaded (per vertex color) lit triangles.
	*/
	virtual const char* getSolidShadedMaterial() = 0;

	/**
	\brief Enable or disable the APEX module-specific stat collection (some modules can be time consuming)
	*/
	virtual void setEnableApexStats(bool enableApexStats) = 0;

	/**
	\brief Enable or disable the APEX concurrent access check
	*/
	virtual void setEnableConcurrencyCheck(bool enableConcurrencyChecks) = 0;

	/**
	\brief Returns current setting for APEX concurrent access check
	*/
	virtual bool isConcurrencyCheckEnabled() = 0;

protected:
	virtual ~ApexSDK() {}

};

/**
\def APEX_API
\brief Export the function declaration from its DLL
*/

/**
\def CALL_CONV
\brief Use C calling convention, required for exported functions
*/

PX_POP_PACK

#ifdef CALL_CONV
#undef CALL_CONV
#endif

#if PX_WINDOWS_FAMILY

#define APEX_API extern "C" __declspec(dllexport)
#define CALL_CONV __cdecl
#else
#define APEX_API extern "C"
#define CALL_CONV /* void */
#endif

/**
\brief Global function to create the SDK object.  APEX_SDK_VERSION must be passed for the APEXsdkVersion.
*/
APEX_API ApexSDK*	CALL_CONV CreateApexSDK(	const ApexSDKDesc& desc, 
														ApexCreateError* errorCode = NULL, 
														uint32_t APEXsdkVersion = APEX_SDK_VERSION, 
														PxAllocatorCallback* alloc = 0);

/**
\brief Returns global SDK pointer.
*/
APEX_API ApexSDK*	CALL_CONV GetApexSDK();

}
} // end namespace nvidia::apex

#endif // APEX_SDK_H
