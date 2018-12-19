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



#ifndef MODULE_CLOTHING_IMPL_H
#define MODULE_CLOTHING_IMPL_H

#include "ModuleClothing.h"
#include "ClothingAsset.h"
#include "ApexSDKIntl.h"
#include "ModuleIntl.h"
#include "ModuleBase.h"
#include "ApexSDKHelpers.h"
#include "ModuleClothingRegistration.h"
#include "ClothStructs.h"
#include "ApexRWLockable.h"
#include "ApexPvdClient.h"
// The clothing GPU source is in a separate DLL, we're going to load it and get a CuFactory create method
#if PX_WINDOWS_FAMILY
#include "ModuleUpdateLoader.h"
#endif

class ClothingPhysicalMeshParameters;

namespace physx
{
	class PxCudaContextManager;
}

namespace nvidia
{
#if APEX_CUDA_SUPPORT
	namespace cloth
	{
		class CuFactory;
	}
#endif

namespace apex
{
class ApexIsoMesh;
class RenderMeshAssetAuthoringIntl;
class ClothingAsset;
class ClothingAssetAuthoring;
}
namespace clothing
{
class ClothingScene;
class ClothingAssetImpl;
class ClothingAssetAuthoringImpl;
class ClothingPhysicalMeshImpl;
class CookingAbstract;
class DummyActor;
class DummyAsset;
class SimulationAbstract;


class BackendFactory
{
public:
	virtual bool				isMatch(const char* simulationBackend) = 0;
	virtual const char*			getName() = 0;
	virtual uint32_t			getCookingVersion() = 0;
	virtual uint32_t			getCookedDataVersion(const NvParameterized::Interface* cookedData) = 0;
	virtual CookingAbstract*	createCookingJob() = 0;
	virtual void				releaseCookedInstances(NvParameterized::Interface* cookedData) = 0;
	virtual SimulationAbstract* createSimulation(ClothingScene* clothingScene, bool useHW) = 0;
};



class ModuleClothingImpl : public ModuleClothing, public ModuleIntl, public ModuleBase, public ApexRWLockable
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	ModuleClothingImpl(ApexSDKIntl* sdk);

	// from ApexInterface
	PX_INLINE void				release()
	{
		ModuleBase::release();
	}

	// from Module
	void						init(NvParameterized::Interface& desc);
	NvParameterized::Interface* getDefaultModuleDesc();
	PX_INLINE const char*		getName() const
	{
		return ModuleBase::getName();
	}
	AuthObjTypeID				getModuleID() const;

	RenderableIterator*	createRenderableIterator(const Scene&);

	ClothingPhysicalMesh*		createEmptyPhysicalMesh();
	ClothingPhysicalMesh*		createSingleLayeredMesh(RenderMeshAssetAuthoring* asset, uint32_t subdivisionSize, bool mergeVertices, bool closeHoles, IProgressListener* progress);

	PX_INLINE NvParameterized::Interface* getApexClothingActorParams(void) const
	{
		return mApexClothingActorParams;
	}
	PX_INLINE NvParameterized::Interface* getApexClothingPreviewParams(void) const
	{
		return mApexClothingPreviewParams;
	}

	// from ModuleIntl
	virtual void				destroy();
	virtual ModuleSceneIntl* 	createInternalModuleScene(SceneIntl&, RenderDebugInterface*);
	virtual void				releaseModuleSceneIntl(ModuleSceneIntl&);
	virtual uint32_t			forceLoadAssets();

#ifndef WITHOUT_PVD
	virtual void				initPvdClasses(pvdsdk::PvdDataStream& pvdDataStream);
	virtual void				initPvdInstances(pvdsdk::PvdDataStream& pvdDataStream);
	virtual void				destroyPvdInstances();
#endif

	// own methods

	ClothingScene* 				getClothingScene(const Scene& scene);

	ClothingPhysicalMeshImpl*	createPhysicalMeshInternal(ClothingPhysicalMeshParameters* mesh);

	void						releasePhysicalMesh(ClothingPhysicalMeshImpl* physicalMesh);
	void						unregisterAssetWithScenes(ClothingAssetImpl* asset);
	void						notifyReleaseGraphicalData(ClothingAssetImpl* asset);

	Actor*						getDummyActor();

	// This pointer will not be released by this module!
	virtual void				registerBackendFactory(BackendFactory* factory);
	virtual void				unregisterBackendFactory(BackendFactory* factory);
	// this works both with the name of the simulation backend, or the cooking data type
	BackendFactory*				getBackendFactory(const char* simulationBackend);

	ClothFactory				createClothFactory(PxCudaContextManager* contextManager);
	void						releaseClothFactory(PxCudaContextManager* contextManager);

	PX_INLINE uint32_t			getMaxUnusedPhysXResources() const
	{
		return mInternalModuleParams.maxUnusedPhysXResources;
	}
	PX_INLINE uint32_t			getMaxNumCompartments() const
	{
		return mInternalModuleParams.maxNumCompartments;
	}
	PX_INLINE bool				allowAsyncFetchResults() const
	{
		return mInternalModuleParams.asyncFetchResults;
	}
	PX_INLINE bool				allowAsyncCooking() const
	{
		return mInternalModuleParams.allowAsyncCooking;
	}
	PX_INLINE uint32_t			getAvgSimFrequencyWindowSize() const
	{
		return mInternalModuleParams.avgSimFrequencyWindow;
	}
	PX_INLINE bool				allowApexWorkBetweenSubsteps() const
	{
		return mInternalModuleParams.allowApexWorkBetweenSubsteps;
	}
	PX_INLINE float				getInterCollisionDistance() const
	{
		return mInternalModuleParams.interCollisionDistance;
	}
	PX_INLINE float				getInterCollisionStiffness() const
	{
		return mInternalModuleParams.interCollisionStiffness;
	}
	PX_INLINE uint32_t			getInterCollisionIterations() const
	{
		return mInternalModuleParams.interCollisionIterations;
	}
	PX_INLINE void				setInterCollisionDistance(float distance)
	{
		mInternalModuleParams.interCollisionDistance = distance;
	}
	PX_INLINE void				setInterCollisionStiffness(float stiffness)
	{
		mInternalModuleParams.interCollisionStiffness = stiffness;
	}
	PX_INLINE void				setInterCollisionIterations(uint32_t iterations)
	{
		mInternalModuleParams.interCollisionIterations = iterations;
	}
	PX_INLINE bool				useSparseSelfCollision() const
	{
		return mInternalModuleParams.sparseSelfCollision;
	}
	PX_INLINE uint32_t			getMaxTimeRenderProxyInPool() const
	{
		return mInternalModuleParams.maxTimeRenderProxyInPool;
	}

private:
	ClothingPhysicalMesh*		createSingleLayeredMeshInternal(RenderMeshAssetAuthoringIntl* asset, uint32_t subdivisionSize, bool mergeVertices, bool closeHoles, IProgressListener* progress);

	ResourceList				mClothingSceneList;
	ResourceList				mPhysicalMeshes;
	ResourceList				mAssetAuthorableObjectFactories;

	DummyActor*					mDummyActor;
	DummyAsset*					mDummyAsset;
	nvidia::Mutex				mDummyProtector;

	class ClothingBackendFactory : public BackendFactory, public nvidia::UserAllocated
	{
	public:
		virtual bool				isMatch(const char* simulationBackend);
		virtual const char*			getName();
		virtual uint32_t			getCookingVersion();
		virtual uint32_t			getCookedDataVersion(const NvParameterized::Interface* cookedData);
		virtual CookingAbstract*	createCookingJob();
		virtual void				releaseCookedInstances(NvParameterized::Interface* cookedData);
		virtual SimulationAbstract* createSimulation(ClothingScene* clothingScene, bool useHW);
	};

	ClothingBackendFactory		mBackendFactory;

	Array<BackendFactory*>		mBackendFactories;

	ClothingModuleParameters*	mModuleParams;
	ClothingModuleParametersNS::ParametersStruct mInternalModuleParams;

	NvParameterized::Interface*	mApexClothingActorParams;
	NvParameterized::Interface*	mApexClothingPreviewParams;

	nvidia::Mutex				mFactoryMutex;
	cloth::Factory*				mCpuFactory;
	uint32_t					mCpuFactoryReferenceCount;

#if APEX_CUDA_SUPPORT
	void*						mGpuDllHandle;

	// this function is declared in CreateCuFactory.h and implemented in CreateCuFactory.cpp, which is in in APEX_ClothingGPU
	typedef nvidia::cloth::CuFactory* (PxCreateCuFactory_FUNC)(PxCudaContextManager* contextManager);
	PxCreateCuFactory_FUNC*		mPxCreateCuFactoryFunc;
	
	struct GpuFactoryEntry
	{
		GpuFactoryEntry(cloth::Factory* f, PxCudaContextManager* c) : factoryGpu(f), contextManager(c), referenceCount(0) {}

		cloth::Factory* factoryGpu;
		PxCudaContextManager* contextManager;
		uint32_t referenceCount;
	};

	nvidia::Array<GpuFactoryEntry> mGpuFactories;
#endif

	
	virtual ~ModuleClothingImpl() {}
};

}
} // namespace nvidia

#endif // MODULE_CLOTHING_IMPL_H
