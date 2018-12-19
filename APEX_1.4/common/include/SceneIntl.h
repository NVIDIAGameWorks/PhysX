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



#ifndef SCENE_INTL_H
#define SCENE_INTL_H

#define APEX_CHECK_STAT_TIMER(name)// { PX_PROFILE_ZONE(name, GetInternalApexSDK()->getContextId()); }

#include "Scene.h"
#include "ApexUsingNamespace.h"

#if PX_PHYSICS_VERSION_MAJOR == 3
// PH prevent PxScene.h from including PxPhysX.h, it will include sooo many files that it will break the clothing embedded branch
#define PX_PHYSICS_NX_PHYSICS
#include "PxScene.h"
#undef PX_PHYSICS_NX_PHYSICS
#endif

namespace nvidia
{
namespace apex
{

class ModuleSceneIntl;
class ApexContext;
class RenderDebugInterface;
class PhysX3Interface;
class ApexCudaTestManager;

/**
 * Framework interface to ApexScenes for use by modules
 */
class SceneIntl : public Scene
{
public:
	/**
	When a module has been released by the end-user, the module must release
	its ModuleScenesIntl and notify those Scenes that their module
	scenes no longer exist
	*/
	virtual void moduleReleased(ModuleSceneIntl& moduleScene) = 0;

#if PX_PHYSICS_VERSION_MAJOR == 3
	virtual void lockRead(const char *fileName,uint32_t lineo) = 0;
	virtual void lockWrite(const char *fileName,uint32_t lineno) = 0;
	virtual void unlockRead() = 0;
	virtual void unlockWrite() = 0;

	virtual void addModuleUserNotifier(physx::PxSimulationEventCallback& notify) = 0;
	virtual void removeModuleUserNotifier(physx::PxSimulationEventCallback& notify) = 0;
	virtual void addModuleUserContactModify(physx::PxContactModifyCallback& notify) = 0;
	virtual void removeModuleUserContactModify(physx::PxContactModifyCallback& notify) = 0;
	virtual PhysX3Interface* getApexPhysX3Interface()	const = 0;
#endif

	virtual ApexContext* getApexContext() = 0;
	virtual float getElapsedTime() const = 0;

	/* Get total elapsed simulation time, in integer milliseconds */
	virtual uint32_t getTotalElapsedMS() const = 0;

	virtual bool isSimulating() const = 0;
	virtual bool physXElapsedTime(float& dt) const = 0;

	virtual float getPhysXSimulateTime() const = 0;

	virtual bool isFinalStep() const = 0;

	virtual uint32_t getSeed() = 0; // Not necessarily const

	enum ApexStatsDataEnum
	{
		NumberOfActors,
		NumberOfShapes,
		NumberOfAwakeShapes,
		NumberOfCpuShapePairs,
		ApexBeforeTickTime,
		ApexDuringTickTime,
		ApexPostTickTime,
		PhysXSimulationTime,
		ClothingSimulationTime,
		ParticleSimulationTime,
		TurbulenceSimulationTime,
		PhysXFetchResultTime,
		UserDelayedFetchTime,
		RbThroughput,
		SimulatedSpriteParticlesCount,
		SimulatedMeshParticlesCount,
		VisibleDestructibleChunkCount,
		DynamicDestructibleChunkIslandCount,

		// insert new items before this line
		NumberOfApexStats	// The number of stats
	};

	virtual void setApexStatValue(int32_t index, StatValue dataVal) = 0;

#if APEX_CUDA_SUPPORT
	virtual ApexCudaTestManager& getApexCudaTestManager() = 0;
	virtual bool isUsingCuda() const = 0;
#endif
	virtual ModuleSceneIntl* getInternalModuleScene(const char* moduleName) = 0;
};

/* ApexScene task names */
#define APEX_DURING_TICK_TIMING_FIX 1

#define AST_PHYSX_SIMULATE				"ApexScene::PhysXSimulate"
#define AST_PHYSX_BETWEEN_STEPS			"ApexScene::PhysXBetweenSteps"

#if APEX_DURING_TICK_TIMING_FIX
#	define AST_DURING_TICK_COMPLETE		"ApexScene::DuringTickComplete"
#endif

#define AST_PHYSX_CHECK_RESULTS			"ApexScene::CheckResults"
#define AST_PHYSX_FETCH_RESULTS			"ApexScene::FetchResults"




}
} // end namespace nvidia::apex


#endif // SCENE_INTL_H
