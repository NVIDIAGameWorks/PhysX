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


#ifndef APEX_CONTROLLER_H
#define APEX_CONTROLLER_H

#include "SampleManager.h"
#include <DirectXMath.h>

#include "ApexCudaContextManager.h"
#include "ModuleClothing.h"
#include "ClothingActor.h"
#include "ClothingAsset.h"
#include "ClothingRenderProxy.h"
#include "DestructibleAsset.h"
#include "DestructibleActor.h"
#include "DestructibleRenderable.h"
#include "ModuleParticles.h"
#include "ModuleIofx.h"
#include "IofxActor.h"
#include "RenderVolume.h"
#include "EffectPackageActor.h"
#include "EffectPackageAsset.h"
#include "nvparameterized/NvParameterized.h"
#include "nvparameterized/NvParamUtils.h"
#include "PxPhysicsAPI.h"
#include "Apex.h"
#include "PxMat44.h"

#pragma warning(push)
#pragma warning(disable : 4350)
#include <set>
#include <map>
#pragma warning(pop)

using namespace physx;
using namespace nvidia;
using namespace nvidia::apex;

class ApexRenderResourceManager;
class ApexResourceCallback;
class CFirstPersonCamera;
class PhysXPrimitive;
class ApexRenderer;

class ApexController : public ISampleController
{
  public:
	ApexController(PxSimulationFilterShader filterShader, CFirstPersonCamera* camera);
	virtual ~ApexController();

	virtual void onInitialize();
	virtual void onTerminate();

	virtual void Animate(double dt);

	void renderOpaque(ApexRenderer* renderer);
	void renderTransparency(ApexRenderer* renderer);

	void getEyePoseAndPickDir(float mouseX, float mouseY, PxVec3& eyePos, PxVec3& pickDir);

	DestructibleActor* spawnDestructibleActor(const char* assetPath);
	void removeActor(DestructibleActor* actor);

	ClothingActor* spawnClothingActor(const char* assetPath);
	void removeActor(ClothingActor* actor);

	EffectPackageActor* spawnEffectPackageActor(const char* assetPath);
	void removeActor(EffectPackageActor* actor);

	PhysXPrimitive* spawnPhysXPrimitiveBox(const nvidia::PxTransform& position, float density = 2000.0f);
	PhysXPrimitive* spawnPhysXPrimitivePlane(const nvidia::PxPlane& plane);
	void removePhysXPrimitive(PhysXPrimitive*);


	void setRenderDebugInterface(RENDER_DEBUG::RenderDebugInterface* iFace)
	{
		mRenderDebugInterface = iFace;
	}


	ApexResourceCallback* getResourceCallback()
	{
		return mApexResourceCallback;
	};

	ResourceProvider* getResourceProvider()
	{
		return mApexSDK->getNamedResourceProvider();
	}

	ModuleDestructible* getModuleDestructible()
	{
		return mModuleDestructible;
	}

	ModuleParticles* getModuleParticles()
	{
		return mModuleParticles;
	}

	ApexSDK* getApexSDK()
	{
		return mApexSDK;
	}

	Scene* getApexScene()
	{
		return mApexScene;
	}

	float getLastSimulationTime()
	{
		return mLastSimulationTime;
	}

	void togglePlayPause()
	{
		mPaused = !mPaused;
	}

	void toggleFixedTimestep()
	{
		mUseFixedTimestep = !mUseFixedTimestep;
		if (mUseFixedTimestep)
		{
			mTimeRemainder = 0.0;
		}
	}

	bool usingFixedTimestep() const
	{
		return mUseFixedTimestep;
	}

	void setFixedTimestep(double fixedTimestep)
	{
		if (fixedTimestep > 0.0)
		{
			mFixedTimestep = fixedTimestep;
		}
	}

	double getFixedTimestep()
	{
		return mFixedTimestep;
	}

private:
	void initPhysX();
	void releasePhysX();

	void initApex();
	void releaseApex();

	void initPhysXPrimitives();
	void releasePhysXPrimitives();

	void renderParticles(ApexRenderer* renderer, IofxRenderable::Type type);

	PhysXPrimitive* spawnPhysXPrimitive(PxRigidActor* position, PxVec3 scale);

	PxSimulationFilterShader mFilterShader;
#if APEX_CUDA_SUPPORT
	PxCudaContextManager* mCudaContext;
#endif
	ApexSDK* mApexSDK;
	ApexRenderResourceManager* mApexRenderResourceManager;
	ApexResourceCallback* mApexResourceCallback;

	ModuleDestructible* mModuleDestructible;
	ModuleParticles* mModuleParticles;
	ModuleIofx* mModuleIofx;
	Module* mModuleTurbulenceFS;
	Module* mModuleLegacy;
	ModuleClothing* mModuleClothing;
	RenderVolume* mRenderVolume;

	Scene* mApexScene;

	PxDefaultAllocator mAllocator;
	PxDefaultErrorCallback mErrorCallback;

	PxFoundation* mFoundation;
	PxPhysics* mPhysics;
	PxCooking* mCooking;

	PxDefaultCpuDispatcher* mDispatcher;
	PxScene* mPhysicsScene;

	PxMaterial* mDefaultMaterial;

	PxPvd* mPvd;

	RenderDebugInterface* mApexRenderDebug;
	RENDER_DEBUG::RenderDebugInterface* mRenderDebugInterface;

	std::set<Asset*> mAssets;
	std::map<DestructibleActor*, DestructibleRenderable*> mDestructibleActors;
	std::set<ClothingActor*> mClothingActors;
	std::set<EffectPackageActor*> mEffectPackageActors;
	std::set<PhysXPrimitive*> mPrimitives;

	CFirstPersonCamera* mCamera;

	float mLastSimulationTime;
	LARGE_INTEGER mPerformanceFreq;

	bool mPaused;

	bool mUseFixedTimestep;
	double mFixedTimestep;
	double mTimeRemainder;
};

#endif
