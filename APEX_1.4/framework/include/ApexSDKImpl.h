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



#ifndef APEX_SDK_IMPL_H
#define APEX_SDK_IMPL_H

#include "Apex.h"
#include "nvparameterized/NvParameterizedTraits.h"
#include "ApexInterface.h"
#include "ApexSDKHelpers.h"
#include "ApexContext.h"
#include "ApexPhysXObjectDesc.h"
#include "FrameworkPerfScope.h"
#include "ApexSDKIntl.h"
#include "AuthorableObjectIntl.h"
#include "SceneIntl.h"
#include "RenderDebugInterface.h"
#include "ApexRenderMeshAsset.h"
#include "ApexAuthorableObject.h"
#include "ModuleIntl.h"
#include "DebugColorParamsEx.h"

#include "ModuleCommonRegistration.h"
#include "ModuleFrameworkRegistration.h"

#include "ApexSDKCachedDataImpl.h"

namespace physx
{
	class PxCudaContextManager;
	class PxCudaContextManagerDesc;
	
	namespace pvdsdk
	{
		class PvdDataStream;
		class ApexPvdClient;
	}
}

#if APEX_CUDA_SUPPORT
namespace nvidia
{
	class PhysXGpuIndicator;
}
#endif

namespace nvidia
{
	namespace apex
	{

		class ApexResourceProvider;
		class ApexScene;
		class RenderMeshAsset;

		typedef PxFileBuf* (CreateStreamFunc)(const char* filename, PxFileBuf::OpenMode mode);


		// ApexAuthorableObject needs a "module", so we'll give it one that
		// describes the APEX framework with regards to render mesh stuff
		// needed: ModuleIntl, no methods implemented
		class ModuleFramework : public ModuleIntl
		{
			void destroy() {}

			ModuleSceneIntl* createInternalModuleScene(SceneIntl& apexScene, RenderDebugInterface* rd)
			{
				PX_UNUSED(apexScene);
				PX_UNUSED(rd);
				return NULL;
			}

			void           releaseModuleSceneIntl(ModuleSceneIntl& moduleScene)
			{
				PX_UNUSED(moduleScene);
			}

		public:

			void init(NvParameterized::Traits* t);
			void release(NvParameterized::Traits* t);
		};


		class ApexSDKImpl : public ApexSDKIntl, public ApexRWLockable, public UserAllocated
		{
		public:
			APEX_RW_LOCKABLE_BOILERPLATE

				ApexSDKImpl(ApexCreateError* errorCode, uint32_t APEXsdkVersion);
			void					init(const ApexSDKDesc& desc);

			/* ApexSDK */
			Scene* 			createScene(const SceneDesc&);
			void					releaseScene(Scene* scene);
			AssetPreviewScene* createAssetPreviewScene();
			void					releaseAssetPreviewScene(AssetPreviewScene* nxScene);

			Module* 				createModule(const char* name, ApexCreateError* err);

#if PX_PHYSICS_VERSION_MAJOR == 0
			PxCpuDispatcher*	createCpuDispatcher(uint32_t numThreads);
			PxCpuDispatcher*	getDefaultThreadPool();
			void					releaseCpuDispatcher(PxCpuDispatcher& cd);
#endif

			/**
			Creates/releases an ApexDebugRender interface
			*/
			virtual RenderDebugInterface* createApexRenderDebug(RENDER_DEBUG::RenderDebugInterface* iface, bool useRemoteDebugVisualization);
			virtual void                releaseApexRenderDebug(RenderDebugInterface& debug);


			/**
			Create/release ApexShape interfaces
			*/

			virtual SphereShape* createApexSphereShape();
			virtual CapsuleShape* createApexCapsuleShape();
			virtual BoxShape* createApexBoxShape();
			virtual HalfSpaceShape* createApexHalfSpaceShape();

			virtual void releaseApexShape(Shape& shape);

#if PX_PHYSICS_VERSION_MAJOR == 3
			PhysXObjectDescIntl* 	createObjectDesc(const Actor*, const PxActor*);
			PhysXObjectDescIntl* 	createObjectDesc(const Actor*, const PxShape*);
			PhysXObjectDescIntl* 	createObjectDesc(const Actor*, const PxJoint*);
			PhysXObjectDescIntl* 	createObjectDesc(const Actor*, const PxCloth*);
			PhysXObjectDescIntl* 	createObjectDesc(const Actor*, const PxParticleSystem*);
			PhysXObjectDescIntl* 	createObjectDesc(const Actor*, const PxParticleFluid*);
			const PhysXObjectDesc* getPhysXObjectInfo(const PxActor*) const;
			const PhysXObjectDesc* getPhysXObjectInfo(const PxShape*) const;
			const PhysXObjectDesc* getPhysXObjectInfo(const PxJoint*) const;
			const PhysXObjectDesc* getPhysXObjectInfo(const PxCloth*) const;
			const PhysXObjectDesc* getPhysXObjectInfo(const PxParticleSystem*) const;
			const PhysXObjectDesc* getPhysXObjectInfo(const PxParticleFluid*) const;
			PxCooking* 	            getCookingInterface();
			PxPhysics* 	    		getPhysXSDK();
#endif // PX_PHYSICS_VERSION_MAJOR == 3

			ApexActor*              getApexActor(Actor*) const;

			// deprecated, use getErrorCallback()
			PxErrorCallback* 		getOutputStream();
			PxFoundation*			getFoundation() const;
			PxErrorCallback* 		getErrorCallback() const;
			PxAllocatorCallback* 	getAllocator() const;
			ResourceProvider* 	getNamedResourceProvider();
			ResourceProviderIntl* 	getInternalResourceProvider();
			PxFileBuf* 		createStream(const char* filename, PxFileBuf::OpenMode mode);
			PxFileBuf* 		createMemoryReadStream(const void* mem, uint32_t len);
			PxFileBuf* 		createMemoryWriteStream(uint32_t alignment = 0);
			const void* 			getMemoryWriteBuffer(PxFileBuf& stream, uint32_t& len);
			void					releaseMemoryReadStream(PxFileBuf& stream);
			void					releaseMemoryWriteStream(PxFileBuf& stream);

			uint32_t			getNbModules();
			Module**				getModules();
			ModuleIntl**				getInternalModules();
			void					releaseModule(Module* module);
			ModuleIntl*				getInternalModuleByName(const char* name);

			uint32_t			forceLoadAssets();

			const char* 			checkAssetName(AuthorableObjectIntl& ao, const char* inName, ApexSimpleString& autoNameStorage);
			AssetAuthoring*	createAssetAuthoring(const char* authorTypeName);
			AssetAuthoring*	createAssetAuthoring(const char* authorTypeName, const char* name);
			AssetAuthoring*	createAssetAuthoring(NvParameterized::Interface* params, const char* name);
			Asset*			createAsset(AssetAuthoring&, const char*);
			Asset* 			createAsset(NvParameterized::Interface* params, const char* name);
			virtual Asset*	createAsset(const char* opaqueMeshName, UserOpaqueMesh* om);
			void					releaseAsset(Asset& nxasset);
			void					releaseAssetAuthoring(AssetAuthoring&);
			/* ApexSDKIntl */
			void					reportError(PxErrorCode::Enum code, const char* file, int line, const char* functionName, const char* message, ...);
			void*					getTempMemory(uint32_t size);
			void					releaseTempMemory(void* data);
			NvParameterized::Traits* getParameterizedTraits()
			{
				return mParameterizedTraits;
			}
			uint32_t			getCookingVersion() const
			{
				return cookingVersion;
			}
			void					registerExternalModule(Module* nx, ModuleIntl* ni)
			{
				registerModule(nx, ni);
			}

			AuthObjTypeID			registerAuthObjType(const char*, ResID nsid);
			AuthObjTypeID			registerAuthObjType(const char*, AuthorableObjectIntl* authObjPtr);
			AuthObjTypeID			registerNvParamAuthType(const char*, AuthorableObjectIntl* authObjPtr);
			void					unregisterAuthObjType(const char*);
			void					unregisterNvParamAuthType(const char*);
			AuthorableObjectIntl*	getAuthorableObject(const char*);
			AuthorableObjectIntl*	getParamAuthObject(const char*);
			bool					getAuthorableObjectNames(const char** authTypeNames, uint32_t& outCount, uint32_t inCount);
			ResID					getMaterialNameSpace() const
			{
				return mMaterialNS;
			}
			ResID					getOpaqueMeshNameSpace() const
			{
				return mOpaqueMeshNS;
			}
			ResID					getCustomVBNameSpace() const
			{
				return mCustomVBNS;
			}
			ResID					getApexMeshNameSpace();
			ResID					getCollisionGroupNameSpace() const
			{
				return mCollGroupNS;
			}
			ResID					getCollisionGroup128NameSpace() const
			{
				return mCollGroup128NS;
			}
			ResID					getCollisionGroup64NameSpace() const
			{
				return mCollGroup64NS;
			}
			ResID					getCollisionGroupMaskNameSpace() const
			{
				return mCollGroupMaskNS;
			}
			ResID					getPhysicalMaterialNameSpace() const
			{
				return mPhysMatNS;
			}
			ResID					getAuthorableTypesNameSpace() const
			{
				return mObjTypeNS;
			}

			void					releaseObjectDesc(void*);
			UserRenderResourceManager* getUserRenderResourceManager() const
			{
				return renderResourceManagerWrapper ? renderResourceManagerWrapper : renderResourceManager;
			}

			const char* 			getWireframeMaterial();
			const char* 			getSolidShadedMaterial();
			virtual pvdsdk::ApexPvdClient* getApexPvdClient();
			virtual profile::PxProfileZone * getProfileZone();
			virtual profile::PxProfileZoneManager * getProfileZoneManager();

			virtual void setEnableApexStats(bool enableApexStats)
			{
				mEnableApexStats = enableApexStats;
			}

			virtual void setEnableConcurrencyCheck(bool enableConcurrencyCheck)
			{
				mEnableConcurrencyCheck = enableConcurrencyCheck;
			}

			virtual bool isConcurrencyCheckEnabled()
			{
				return mEnableConcurrencyCheck;
			}

			bool isApexStatsEnabled() const
			{
				return mEnableApexStats;
			}

#if PX_WINDOWS_FAMILY
			virtual const char*		getAppGuid();
#endif

#if APEX_CUDA_SUPPORT
			virtual PhysXGpuIndicator* registerPhysXIndicatorGpuClient();
			virtual void unregisterPhysXIndicatorGpuClient(PhysXGpuIndicator* gpuIndicator);
#endif

			ApexSDKCachedData&	getCachedData() const
			{
				return *mCachedData;
			}

			NvParameterized::Serializer* createSerializer(NvParameterized::Serializer::SerializeType type);
			NvParameterized::Serializer* createSerializer(NvParameterized::Serializer::SerializeType type, NvParameterized::Traits* traits);

			NvParameterized::Serializer::SerializeType getSerializeType(const void* data, uint32_t dlen);
			NvParameterized::Serializer::SerializeType getSerializeType(PxFileBuf& stream);

			NvParameterized::Serializer::ErrorType getSerializePlatform(const void* data, uint32_t dlen, NvParameterized::SerializePlatform& platform);
			NvParameterized::Serializer::ErrorType getSerializePlatform(PxFileBuf& stream, NvParameterized::SerializePlatform& platform);
			void getCurrentPlatform(NvParameterized::SerializePlatform& platform) const;
			bool getPlatformFromString(const char* name, NvParameterized::SerializePlatform& platform) const;
			const char* getPlatformName(const NvParameterized::SerializePlatform& platform) const;

			NvParameterized::Interface* 	getDebugColorParams() const
			{
				return mDebugColorParams;
			}
			void					updateDebugColorParams(const char* color, uint32_t val);

			bool					getRMALoadMaterialsLazily();

			// applications can append strings to the APEX DLL filenames
			const char* getCustomDllNamePostfix() const
			{
				return mCustomDllNamePostfix.c_str();
			}

			bool isParticlesSupported() const
			{
#if APEX_USE_PARTICLES
				return true;
#else
				return false;
#endif
			}

			virtual ModuleIntl *getInternalModule(Module *module);
			virtual Module *getModule(ModuleIntl *module);

			virtual void enterURR();
			virtual void leaveURR();
			virtual void checkURR();

		protected:
			virtual ~ApexSDKImpl();
			void					registerModule(Module*, ModuleIntl*);
			void					release();

		private:

			void debugAsset(Asset* asset, const char* name);

			ApexSimpleString                mDllLoadPath;
			ApexSimpleString                mCustomDllNamePostfix;

			ApexSimpleString                mWireframeMaterial;
			ApexSimpleString                mSolidShadedMaterial;

#if PX_WINDOWS_FAMILY
			ApexSimpleString                mAppGuid;
#endif

			ResourceList* 				mAuthorableObjects;

			physx::Array<Module*>			modules;
			physx::Array<Module*>			moduleListForAPI;
			physx::Array<ModuleIntl*>			imodules;
			physx::Array<Scene*>		mScenes;

			PhysXObjectDescIntl* 			createObjectDesc(const Actor*, const void*);
			PhysXObjectDescIntl* 			getGenericPhysXObjectInfo(const void*) const;

			enum { DescHashSize = 1024U * 32U };

			uint32_t					mBatchSeedSize;

			mutable nvidia::Mutex			mPhysXObjDescsLock;

			physx::Array<ApexPhysXObjectDesc> mPhysXObjDescs;
			uint32_t					mPhysXObjDescHash[DescHashSize];
			uint32_t					mDescFreeList;
			ResID                         mObjTypeNS;
			ResID							mNxParamObjTypeNS;

			char*							mErrorString;
			//temp memories:
			struct TempMemory
			{
				TempMemory() : memory(NULL), size(0), used(0) {}
				void* memory;
				uint32_t size;
				uint32_t used;
			};
			physx::Array<TempMemory>		mTempMemories;
			uint32_t						mNumTempMemoriesActive;	//temp memories are a LIFO, mNumTempMemoriesActive <= mTempMemories.size()
			nvidia::Mutex					mTempMemoryLock;
			nvidia::Mutex					mReportErrorLock;

			PxFoundation*					foundation;

#if PX_PHYSICS_VERSION_MAJOR == 0
			PxCpuDispatcher*			mApexThreadPool;
			physx::Array<PxCpuDispatcher*> mUserAllocThreadPools;
#else
			PxPhysics*	    				physXSDK;
			PxCooking*	     			    cooking;
#endif // PX_PHYSICS_VERSION_MAJOR == 0

			UserRenderResourceManager*	renderResourceManager;
			UserRenderResourceManager*	renderResourceManagerWrapper;
			ApexResourceProvider*			apexResourceProvider;
			uint32_t					physXsdkVersion;
			uint32_t					cookingVersion;

			NvParameterized::Traits*		mParameterizedTraits;

			ResID							mMaterialNS;
			ResID							mOpaqueMeshNS;
			ResID							mCustomVBNS;
			ResID							mCollGroupNS;
			ResID							mCollGroup128NS;
			ResID							mCollGroup64NS;
			ResID							mCollGroupMaskNS;
			ResID							mPhysMatNS;

			ModuleFramework					frameworkModule;

			ApexSDKCachedDataImpl*				mCachedData;

			DebugColorParamsEx*				mDebugColorParams;

			bool							mRMALoadMaterialsLazily;

#ifdef PHYSX_PROFILE_SDK
			pvdsdk::ApexPvdClient*				mApexPvdClient;
			profile::PxProfileZone			*mProfileZone;
#endif

			uint32_t							mURRdepthTLSslot;

			bool							mEnableApexStats;
			bool							mEnableConcurrencyCheck;
		};


		///////////////////////////////////////////////////////////////////////////////
		// ApexRenderMeshAssetAuthoring
		///////////////////////////////////////////////////////////////////////////////
		// Didn't use ApexAuthorableObject<> here because there were enough differences
		// from a "normal" asset to make it difficult.  I could probably bend the will
		// of the APEX render mesh to get it to comply, but it wasn't worth it at the time.
		// -LRR

		class RenderMeshAuthorableObject : public AuthorableObjectIntl
		{
		public:
			RenderMeshAuthorableObject(ModuleIntl* m, ResourceList& list, const char* parameterizedName) :
				AuthorableObjectIntl(m, list, ApexRenderMeshAsset::getClassName())
			{
				// Register the proper authorable object type in the NRP (override)
				mAOResID = GetInternalApexSDK()->getInternalResourceProvider()->createNameSpace(mAOTypeName.c_str());
				mAOPtrResID = GetInternalApexSDK()->registerAuthObjType(mAOTypeName.c_str(), this);

				mParameterizedName = parameterizedName;

				// Register the parameterized name in the NRP to point to this authorable object
				GetInternalApexSDK()->registerNvParamAuthType(mParameterizedName.c_str(), this);
			}

			Asset* 			createAsset(AssetAuthoring& author, const char* name);
			Asset* 			createAsset(NvParameterized::Interface* params, const char* name);

			void					releaseAsset(Asset& nxasset);

			AssetAuthoring* 	createAssetAuthoring();
			AssetAuthoring* 	createAssetAuthoring(const char* name);
			AssetAuthoring* 	createAssetAuthoring(NvParameterized::Interface* params, const char* name);
			void					releaseAssetAuthoring(AssetAuthoring& nxauthor);

			uint32_t					forceLoadAssets();
			virtual uint32_t			getAssetCount()
			{
				return mAssets.getSize();
			}
			virtual bool			getAssetList(Asset** outAssets, uint32_t& outAssetCount, uint32_t inAssetCount)
			{
				PX_ASSERT(outAssets);
				PX_ASSERT(inAssetCount >= mAssets.getSize());

				if (!outAssets || inAssetCount < mAssets.getSize())
				{
					outAssetCount = 0;
					return false;
				}

				outAssetCount = mAssets.getSize();
				for (uint32_t i = 0; i < mAssets.getSize(); i++)
				{
					ApexRenderMeshAsset* asset = static_cast<ApexRenderMeshAsset*>(mAssets.getResource(i));
					outAssets[i] = static_cast<Asset*>(asset);
				}

				return true;
			}


			ResID					getResID()
			{
				return mAOResID;
			}

			ApexSimpleString&		getName()
			{
				return mAOTypeName;
			}

			// Resource methods
			void					release();
			void					destroy();
		};

	}
} // end namespace nvidia::apex


#endif // APEX_SDK_IMPL_H
