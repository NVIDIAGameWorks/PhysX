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



#include "ApexDefs.h"

#include "Apex.h"
#include "ClothingScene.h"
#include "ClothingActorImpl.h"
#include "ClothingAssetImpl.h"
#include "ClothingCooking.h"
#include "SceneIntl.h"
#include "ClothingRenderProxyImpl.h"

#include "DebugRenderParams.h"
#include "ProfilerCallback.h"

#include "Simulation.h"

#include "PsThread.h"
#include "ApexUsingNamespace.h"
#include "PsAtomic.h"

#if APEX_CUDA_SUPPORT
#include "PxGpuDispatcher.h"
#endif

#include "ApexPvdClient.h"

namespace nvidia
{
namespace clothing
{

ClothingScene::ClothingScene(ModuleClothingImpl& _module, SceneIntl& scene, RenderDebugInterface* renderDebug, ResourceList& list)
	: mModule(&_module)
	, mApexScene(&scene)
#if PX_PHYSICS_VERSION_MAJOR == 3
	, mPhysXScene(NULL)
#endif
	, mSumBenefit(0)
	, mWaitForSolverTask(NULL)
	, mSimulationTask(NULL)
	, mSceneRunning(0)
	, mRenderDebug(renderDebug)
	, mDebugRenderParams(NULL)
	, mClothingDebugRenderParams(NULL)
	, mCurrentCookingTask(NULL)
	, mCurrentSimulationDelta(0)
	, mAverageSimulationFrequency(0.0f)
#ifndef _DEBUG
	, mFramesCount(0)
	, mSimulatedTime(0.f)
	, mTimestep(0.f)
#endif
	, mCpuFactory(NULL, NULL)
#if APEX_CUDA_SUPPORT
	, mGpuFactory(NULL, NULL)
	, mPhysXGpuIndicator(NULL)
#endif
{
	mClothingBeforeTickStartTask.setScene(this);
	list.add(*this);

	/* Initialize reference to ClothingDebugRenderParams */
	mDebugRenderParams = DYNAMIC_CAST(DebugRenderParams*)(mApexScene->getDebugRenderParams());
	PX_ASSERT(mDebugRenderParams);
	NvParameterized::Handle handle(*mDebugRenderParams), memberHandle(*mDebugRenderParams);
	int size;

	if (mDebugRenderParams->getParameterHandle("moduleName", handle) == NvParameterized::ERROR_NONE)
	{
		handle.getArraySize(size, 0);
		handle.resizeArray(size + 1);
		if (handle.getChildHandle(size, memberHandle) == NvParameterized::ERROR_NONE)
		{
			memberHandle.initParamRef(ClothingDebugRenderParams::staticClassName(), true);
		}
	}

	/* Load reference to ClothingDebugRenderParams */
	NvParameterized::Interface* refPtr = NULL;
	memberHandle.getParamRef(refPtr);
	mClothingDebugRenderParams = DYNAMIC_CAST(ClothingDebugRenderParams*)(refPtr);
	PX_ASSERT(mClothingDebugRenderParams);

	mLastSimulationDeltas.reserve(mModule->getAvgSimFrequencyWindowSize());

	mWaitForSolverTask = PX_NEW(WaitForSolverTask)(this);

	mSimulationTask = PX_NEW(ClothingSceneSimulateTask)(mApexScene, this, mModule, GetInternalApexSDK()->getProfileZoneManager());
	mSimulationTask->setWaitTask(mWaitForSolverTask);
}



ClothingScene::~ClothingScene()
{
}



void ClothingScene::simulate(float elapsedTime)
{
	for (uint32_t i = 0; i < mActorArray.size(); i++)
	{
		ClothingActorImpl* clothingActor = static_cast<ClothingActorImpl*>(mActorArray[i]);
		clothingActor->waitForFetchResults();
	}

	if (mLastSimulationDeltas.size() < mLastSimulationDeltas.capacity())
	{
		mCurrentSimulationDelta = mLastSimulationDeltas.size();
		mLastSimulationDeltas.pushBack(elapsedTime);
	}
	else if (mLastSimulationDeltas.size() > 0)
	{
		mCurrentSimulationDelta = (mCurrentSimulationDelta + 1) % mLastSimulationDeltas.size();
		mLastSimulationDeltas[mCurrentSimulationDelta] = elapsedTime;
	}

	float temp = 0.0f;
	for (uint32_t i = 0; i < mLastSimulationDeltas.size(); i++)
		temp += mLastSimulationDeltas[i];

	if (temp > 0.0f)
		mAverageSimulationFrequency = (float)(mLastSimulationDeltas.size()) / temp;
	else
		mAverageSimulationFrequency = 0.0f;

	tickRenderProxies();
}



bool ClothingScene::needsManualSubstepping() const
{
	// we could test if any of them is being simulated, but assuming some sane budget settings
	// there will always be >0 clothing actors simulated if there are any present

	if (!mModule->allowApexWorkBetweenSubsteps())
	{
		return false;
	}

	// PH: new rule. The actual simulation object needs to request this too!
	bool manualSubstepping = false;
	for (uint32_t i = 0; i < mActorArray.size(); i++)
	{
		ClothingActorImpl* clothingActor = static_cast<ClothingActorImpl*>(mActorArray[i]);
		manualSubstepping |= clothingActor->needsManualSubstepping();
	}
	return manualSubstepping;
}



void ClothingScene::interStep(uint32_t substepNumber, uint32_t maxSubSteps)
{
	for (uint32_t i = 0; i < mActorArray.size(); i++)
	{
		ClothingActorImpl* clothingActor = static_cast<ClothingActorImpl*>(mActorArray[i]);

		if (clothingActor->needsManualSubstepping())
		{
			clothingActor->tickSynchBeforeSimulate_LocksPhysX(0.0f, 0.0f, substepNumber, maxSubSteps);
			clothingActor->skinPhysicsMesh(maxSubSteps > 1, (float)(substepNumber + 1) / (float)maxSubSteps);
			clothingActor->updateConstrainPositions_LocksPhysX();
			clothingActor->applyCollision_LocksPhysX();
		}
	}
}



void ClothingScene::submitTasks(float elapsedTime, float substepSize, uint32_t numSubSteps)
{
	PxTaskManager* taskManager = mApexScene->getTaskManager();
	const bool isFinalStep = mApexScene->isFinalStep();

	for (uint32_t i = 0; i < mActorArray.size(); i++)
	{
		ClothingActorImpl* clothingActor = static_cast<ClothingActorImpl*>(mActorArray[i]);
#if APEX_UE4
		clothingActor->initBeforeTickTasks(elapsedTime, substepSize, numSubSteps, taskManager, 0, 0);
#else
		clothingActor->initBeforeTickTasks(elapsedTime, substepSize, numSubSteps);
#endif

		if (isFinalStep)
		{
			clothingActor->submitTasksDuring(taskManager);
		}
	}

	taskManager->submitUnnamedTask(mClothingBeforeTickStartTask);

	mSimulationTask->setDeltaTime(elapsedTime);
	if (elapsedTime > 0.0f)
	{
		taskManager->submitUnnamedTask(*mSimulationTask);
		taskManager->submitUnnamedTask(*mWaitForSolverTask);
	}
}



void ClothingScene::setTaskDependencies()
{
	PxTaskManager* taskManager = mApexScene->getTaskManager();
	const PxTaskID physxTick = taskManager->getNamedTask(AST_PHYSX_SIMULATE);
	PxTask* physxTickTask = taskManager->getTaskFromID(physxTick);

#if APEX_DURING_TICK_TIMING_FIX
	const PxTaskID duringFinishedId = taskManager->getNamedTask(AST_DURING_TICK_COMPLETE);
#else
	const PxTaskID duringFinishedId = taskManager->getNamedTask(AST_PHYSX_CHECK_RESULTS);
#endif

	bool startSimulateTask = mSimulationTask->getDeltaTime() > 0;

#if APEX_UE4
	PxTask* dependentTask = physxTickTask;
	PxTaskID duringStartId = physxTick;

	if (startSimulateTask)
	{
		dependentTask = mSimulationTask;
		duringStartId = mSimulationTask->getTaskID();
		mSimulationTask->startAfter(mClothingBeforeTickStartTask.getTaskID());
	}
#else
	PxTaskID duringStartId = startSimulateTask ? mSimulationTask->getTaskID() : physxTick;
#endif
	const bool isFinalStep = mApexScene->isFinalStep();

	for (uint32_t i = 0; i < mActorArray.size(); i++)
	{
		ApexActor* actor = mActorArray[i];
		ClothingActorImpl* clothingActor = static_cast<ClothingActorImpl*>(actor);
#if !APEX_UE4
		PxTask* dependentTask = physxTickTask;
		if (startSimulateTask)
		{
			dependentTask = mSimulationTask;
			mSimulationTask->startAfter(mClothingBeforeTickStartTask.getTaskID());
		}
#endif

		clothingActor->setTaskDependenciesBefore(dependentTask);

		if (isFinalStep)
		{
			// PH:	daisy chain the during tasks to not trash other (=PhysX) tasks' cache etc.
			// HL:	found a case where duringTick becomes the bottleneck because of the daisy chaining

			/*duringStartId = */clothingActor->setTaskDependenciesDuring(duringStartId, duringFinishedId);
		}
	}
#if APEX_UE4
	mSimulationTask->startAfter(mClothingBeforeTickStartTask.getTaskID());
#endif

	mClothingBeforeTickStartTask.finishBefore(physxTick);

	if (startSimulateTask)
	{
		mSimulationTask->finishBefore(physxTick);
		mWaitForSolverTask->startAfter(mSimulationTask->getTaskID());
		mWaitForSolverTask->startAfter(duringFinishedId);
		mWaitForSolverTask->finishBefore(taskManager->getNamedTask(AST_PHYSX_FETCH_RESULTS));
	}
}



void ClothingScene::fetchResults()
{
	if (!mApexScene->isFinalStep())
	{
		return;
	}

	PX_PROFILE_ZONE("ClothingScene::fetchResults", GetInternalApexSDK()->getContextId());

	// make sure to start cooking tasks if possible (and delete old ones)
	submitCookingTask(NULL);

	if (!mModule->allowAsyncFetchResults())
	{
		for (uint32_t i = 0; i < mActorArray.size(); i++)
		{
			ClothingActorImpl* clothingActor = static_cast<ClothingActorImpl*>(mActorArray[i]);
			clothingActor->waitForFetchResults();
		}
	}

	// TBD - if we need to send callbacks to the user, add them here
}


#if PX_PHYSICS_VERSION_MAJOR == 3

void ClothingScene::setModulePhysXScene(PxScene* newPhysXScene)
{
	if (mPhysXScene == newPhysXScene)
	{
		return;
	}

	PxScene* oldPhysXScene = mPhysXScene;

	mPhysXScene = newPhysXScene;
	for (uint32_t i = 0; i < mActorArray.size(); ++i)
	{
		// downcast
		ClothingActorImpl* actor = static_cast<ClothingActorImpl*>(mActorArray[i]);

		actor->setPhysXScene(newPhysXScene);
	}

	mClothingAssetsMutex.lock();
	mClothingAssets.clear();
	mClothingAssetsMutex.unlock();

#if APEX_CUDA_SUPPORT
	{
		if (mGpuFactory.factory != NULL && oldPhysXScene != NULL)
		{
			mSimulationTask->clearGpuSolver();
			PX_ASSERT(mApexScene->getTaskManager() == oldPhysXScene->getTaskManager());
			if (newPhysXScene != NULL)
			{
				mModule->releaseClothFactory(oldPhysXScene->getTaskManager()->getGpuDispatcher()->getCudaContextManager());
			}
			mGpuFactory.clear();
		}
	}
#else 
	{
		if (mCpuFactory.factory != NULL && oldPhysXScene != NULL)
		{
			if (newPhysXScene != NULL)
			{
				mModule->releaseClothFactory(NULL);
			}
			mCpuFactory.clear();
		}
	}
#endif
}

#endif

void ClothingScene::release()
{
	mModule->releaseModuleSceneIntl(*this);
}



void ClothingScene::visualize()
{
#ifdef WITHOUT_DEBUG_VISUALIZE
#else
	if (!mClothingDebugRenderParams->Actors)
	{
		return;
	}

	for (uint32_t i = 0; i < mActorArray.size(); i++)
	{
		// downcast
		ClothingActorImpl* actor = static_cast<ClothingActorImpl*>(mActorArray[i]);
		actor->visualize();
	}
#endif
}



Module* ClothingScene::getModule()
{
	return mModule;
}



bool ClothingScene::isSimulating() const
{
	if (mApexScene != NULL)
	{
		return mApexScene->isSimulating();
	}

	return false;
}



void ClothingScene::registerAsset(ClothingAssetImpl* asset)
{
	mClothingAssetsMutex.lock();
	for (uint32_t i = 0; i < mClothingAssets.size(); i++)
	{
		if (mClothingAssets[i] == asset)
		{
			mClothingAssetsMutex.unlock();
			return;
		}
	}
	mClothingAssets.pushBack(asset);
	mClothingAssetsMutex.unlock();
}



void ClothingScene::unregisterAsset(ClothingAssetImpl* asset)
{
	// remove assets from assets list
	mClothingAssetsMutex.lock();
	for (int32_t i = (int32_t)mClothingAssets.size() - 1; i >= 0; i--)
	{
		if (mClothingAssets[(uint32_t)i] == asset)
		{
			mClothingAssets.replaceWithLast((uint32_t)i);
		}
	}
	mClothingAssetsMutex.unlock();

	removeRenderProxies(asset);
}



void ClothingScene::removeRenderProxies(ClothingAssetImpl* asset)
{
	// delete all render proxies that have the RenderMeshAsset
	// of this ClothingAssetImpl
	mRenderProxiesLock.lock();
	uint32_t numGraphicalMeshes = asset->getNumGraphicalMeshes();
	for (uint32_t i = 0; i < numGraphicalMeshes; ++i)
	{
		RenderMeshAssetIntl* renderMeshAsset = asset->getGraphicalMesh(i);

		Array<ClothingRenderProxyImpl*>& renderProxies = mRenderProxies[renderMeshAsset];
		for (int32_t i = (int32_t)renderProxies.size()-1; i >= 0 ; --i)
		{
			ClothingRenderProxyImpl* renderProxy = renderProxies[(uint32_t)i];
			if (renderProxy->getTimeInPool() > 0)
			{
				PX_DELETE(renderProxies[(uint32_t)i]);
			}
			else
			{
				renderProxy->notifyAssetRelease();
			}
		}
		renderProxies.clear();
		mRenderProxies.erase(renderMeshAsset);
	}
	mRenderProxiesLock.unlock();
}



uint32_t ClothingScene::submitCookingTask(ClothingCookingTask* newTask)
{
	mCookingTaskMutex.lock();

	ClothingCookingTask** currPointer = &mCurrentCookingTask;
	ClothingCookingTask* lastTask = NULL;

	uint32_t numRunning = 0;
	uint32_t numReleased = 0;

	while (*currPointer != NULL)
	{
		PX_ASSERT(lastTask == NULL || currPointer == &lastTask->nextTask);
		if ((*currPointer)->readyForRelease())
		{
			ClothingCookingTask* releaseMe = *currPointer;
			*currPointer = releaseMe->nextTask;
			delete releaseMe;
			numReleased++;
		}
		else
		{
			lastTask = *currPointer;
			numRunning += lastTask->waitsForBeingScheduled() ? 0 : 1;
			currPointer = &(*currPointer)->nextTask;
		}
	}

	// set the linked list
	*currPointer = newTask;
	if (newTask != NULL)
	{
		PX_ASSERT(mApexScene->getTaskManager() != NULL);
		newTask->initCooking(*mApexScene->getTaskManager(), NULL);
	}

	if (numRunning == 0 && mCurrentCookingTask != NULL)
	{
		PX_ASSERT(mCurrentCookingTask->waitsForBeingScheduled());
		mCurrentCookingTask->removeReference();
	}

	mCookingTaskMutex.unlock();

	return numReleased;
}



void ClothingScene::destroy()
{
	for (uint32_t i = 0; i < mActorArray.size(); i++)
	{
		ClothingActorImpl* clothingActor = static_cast<ClothingActorImpl*>(mActorArray[i]);
		clothingActor->waitForFetchResults();
	}

	removeAllActors();

	mClothingAssetsMutex.lock();
	for (uint32_t i = 0 ; i < mClothingAssets.size(); i++)
	{
		// for PhysX3: making sure that fabrics (in assets) are released before the factories (in mSimulationTask)
		mClothingAssets[i]->releaseCookedInstances();
	}
	mClothingAssets.clear();
	mClothingAssetsMutex.unlock();

	// clear render list
	for (HashMap<RenderMeshAssetIntl*, Array<ClothingRenderProxyImpl*> >::Iterator iter = mRenderProxies.getIterator(); !iter.done(); ++iter)
	{
		Array<ClothingRenderProxyImpl*>& renderProxies = iter->second;

		for (int32_t i = (int32_t)renderProxies.size()-1; i >= 0 ; --i)
		{
			uint32_t timeInPool = renderProxies[(uint32_t)i]->getTimeInPool();
			if (timeInPool > 0)
			{
				PX_DELETE(renderProxies[(uint32_t)i]);
				renderProxies.replaceWithLast((uint32_t)i);
			}
			else
			{
				// actually the scene is released, but we just want to make sure
				// that the render proxy deletes itself when it's returned next time
				renderProxies[(uint32_t)i]->notifyAssetRelease();
			}
		}

		renderProxies.clear();
	}
	//mRenderProxies.clear();

	while (mCurrentCookingTask != NULL)
	{
		submitCookingTask(NULL);
		nvidia::Thread::sleep(0); // wait for remaining cooking tasks to finish
	}

	if (mSimulationTask != NULL)
	{
#if PX_PHYSICS_VERSION_MAJOR == 3
		setModulePhysXScene(NULL); // does some cleanup necessary here. Only needed when module gets deleted without the apex scene being deleted before!
#elif APEX_CUDA_SUPPORT
		if (mGpuFactory.factory != NULL)
		{
			mSimulationTask->clearGpuSolver();
			mGpuFactory.clear();
		}
#endif
		PX_DELETE(mSimulationTask);
		mSimulationTask = NULL;
	}

	if (mWaitForSolverTask != NULL)
	{
		PX_DELETE(mWaitForSolverTask);
		mWaitForSolverTask = NULL;
	}

	{
		if (mCpuFactory.factory != NULL)
		{
			mCpuFactory.clear();
		}

#if APEX_CUDA_SUPPORT
		PX_ASSERT(mGpuFactory.factory == NULL);

		ApexSDKIntl* apexSdk = GetInternalApexSDK();
		apexSdk->unregisterPhysXIndicatorGpuClient(mPhysXGpuIndicator);
		mPhysXGpuIndicator = NULL;
#endif
	}

	mApexScene->moduleReleased(*this);
	delete this;
}



void ClothingScene::ClothingBeforeTickStartTask::run()
{
#ifdef PROFILE
	PIXBeginNamedEvent(0, "ClothingBeforeTickStartTask");
#endif
	for (uint32_t i = 0; i < m_pScene->mActorArray.size(); ++i)
	{
		ClothingActorImpl* actor = static_cast<ClothingActorImpl*>(m_pScene->mActorArray[i]);

		actor->startBeforeTickTask();
	}
#ifdef PROFILE
	PIXEndNamedEvent();
#endif
}



const char* ClothingScene::ClothingBeforeTickStartTask::getName() const
{
#if APEX_UE4
	return CLOTHING_BEFORE_TICK_START_TASK_NAME;
#else
	return "ClothingScene::ClothingBeforeTickStartTask";
#endif
}



ClothFactory ClothingScene::getClothFactory(bool& useCuda)
{
#if APEX_CUDA_SUPPORT
	if (useCuda)
	{
		if (mGpuFactory.factory == NULL)
		{
			PxCudaContextManager* contextManager = NULL;
			PxGpuDispatcher* gpuDispatcher = mApexScene->getTaskManager()->getGpuDispatcher();
			if (gpuDispatcher != NULL)
			{
				contextManager = gpuDispatcher->getCudaContextManager();
			}

			if (contextManager != NULL)
			{
				mGpuFactory = mModule->createClothFactory(contextManager);
				if (mGpuFactory.factory != NULL)
				{
					ApexSDKIntl* apexSdk = GetInternalApexSDK();
					mPhysXGpuIndicator = apexSdk->registerPhysXIndicatorGpuClient();
				}
			}
		}

		//APEX_DEBUG_INFO("Gpu Factory %p", mGpuFactory);
		if (mGpuFactory.factory != NULL)
		{
			return mGpuFactory;
		}
		else
		{
			APEX_DEBUG_INFO("Gpu Factory could not be created");
			useCuda = false;
		}
	}
	
	if (!useCuda)
#else
	PX_UNUSED(useCuda);
#endif
	{
		if (mCpuFactory.factory == NULL)
		{
			mCpuFactory = mModule->createClothFactory(NULL);
		}

		//APEX_DEBUG_INFO("Cpu Factory %p", mCpuFactory.factory);
		return mCpuFactory;
	}

#if APEX_CUDA_SUPPORT
	PX_ALWAYS_ASSERT_MESSAGE("this code path is unreachable, at least it used to be.");
	return ClothFactory(NULL, NULL);
#endif
}



cloth::Solver* ClothingScene::getClothSolver(bool useCuda)
{
	ClothFactory factory(NULL, NULL);
#if APEX_CUDA_SUPPORT
	if (useCuda)
	{
		factory = mGpuFactory;
	}
	else
#else
	PX_UNUSED(useCuda);
#endif
	{
		factory = mCpuFactory;
	}

	PX_ASSERT(factory.factory != NULL);
	if (factory.factory != NULL)
	{
		return mSimulationTask->getSolver(factory);
	}

	return NULL;
}



void ClothingScene::lockScene()
{
	mSceneLock.lock();

	if (mSceneRunning == 1)
	{
		APEX_INVALID_OPERATION("The scene is running while the scene write lock is being acquired!");
		PX_ALWAYS_ASSERT();
	}
}



void ClothingScene::unlockScene()
{
	mSceneLock.unlock();
}



void ClothingScene::setSceneRunning(bool on)
{
#ifndef _DEBUG
	int32_t newValue;
	if (on)
	{
		APEX_CHECK_STAT_TIMER("--------- Start ClothingSimulationTime");
		mClothingSimulationTime.getElapsedSeconds();

		newValue = nvidia::atomicIncrement(&mSceneRunning);
	}
	else
	{
		StatValue dataVal;
		dataVal.Float = (float)(1000.0f * mClothingSimulationTime.getElapsedSeconds());
		APEX_CHECK_STAT_TIMER("--------- Stop ClothingSimulationTime");
		mApexScene->setApexStatValue(SceneIntl::ClothingSimulationTime, dataVal);

		// Warn if simulation time was bigger than timestep for 10 or more consecutive frames
		float simulatedTime = 1000.0f * mApexScene->getElapsedTime();
		if (simulatedTime < dataVal.Float)
		{
			mFramesCount++;
			mSimulatedTime	+= simulatedTime;
			mTimestep		+= dataVal.Float;
		}

		if (mFramesCount >= 10)
		{
			float averageSimulatedTime = mSimulatedTime / (float)mFramesCount;
			float averageTimestep = mTimestep / (float)mFramesCount;
			APEX_DEBUG_WARNING("Cloth complexity in scene is too high to be simulated in real time for 10 consecutive frames. (Average Delta Time: %f ms, Average Simulation Time: %f ms)", 
								averageSimulatedTime, averageTimestep);
			mFramesCount	= 0;
			mSimulatedTime	= 0.f;
			mTimestep		= 0.f;
		}

		newValue = nvidia::atomicDecrement(&mSceneRunning);
	}

	if (newValue != (on ? 1 : 0))
	{
		APEX_INTERNAL_ERROR("scene running state was not tracked properly!: on = %s, prevValue = %d", on ? "true" : "false", newValue);
	}
#else
	PX_UNUSED(on);
#endif
}



void ClothingScene::embeddedPostSim()
{
	for (uint32_t i = 0; i < mActorArray.size(); i++)
	{
		ClothingActorImpl* clothingActor = static_cast<ClothingActorImpl*>(mActorArray[i]);
		clothingActor->startFetchTasks();
	}
}



ClothingRenderProxyImpl* ClothingScene::getRenderProxy(RenderMeshAssetIntl* rma, bool useFallbackSkinning, bool useCustomVertexBuffer, const HashMap<uint32_t, ApexSimpleString>& overrideMaterials, const PxVec3* morphTargetNewPositions, const uint32_t* morphTargetVertexOffsets)
{
	if (rma == NULL)
	{
		return NULL;
	}

	ClothingRenderProxyImpl* renderProxy = NULL;


	mRenderProxiesLock.lock();
	Array<ClothingRenderProxyImpl*>& renderProxies = mRenderProxies[rma];
	for (uint32_t i = 0; i < renderProxies.size(); ++i)
	{
		ClothingRenderProxyImpl* proxyInPool = renderProxies[i];
		if (
			proxyInPool->getTimeInPool() > 0 && // proxy is available
			useFallbackSkinning == proxyInPool->usesFallbackSkinning() &&
			useCustomVertexBuffer == proxyInPool->usesCustomVertexBuffer() &&
			morphTargetNewPositions == proxyInPool->getMorphTargetBuffer() &&
			proxyInPool->overrideMaterialsEqual(overrideMaterials)
			)
		{
			renderProxy = proxyInPool;
			break;
		}
	}

	// no corresponding proxy in pool, so create one
	if (renderProxy == NULL)
	{
		renderProxy = PX_NEW(ClothingRenderProxyImpl)(rma, useFallbackSkinning, useCustomVertexBuffer, overrideMaterials, morphTargetNewPositions, morphTargetVertexOffsets, this);
		renderProxies.pushBack(renderProxy);
	}

	renderProxy->setTimeInPool(0);
	mRenderProxiesLock.unlock();

	return renderProxy;
}



void ClothingScene::tickRenderProxies()
{
	PX_PROFILE_ZONE("ClothingScene::tickRenderProxies", GetInternalApexSDK()->getContextId());
	mRenderProxiesLock.lock();

	for(HashMap<RenderMeshAssetIntl*, Array<ClothingRenderProxyImpl*> >::Iterator iter = mRenderProxies.getIterator(); !iter.done(); ++iter)
	{
		Array<ClothingRenderProxyImpl*>& renderProxies = iter->second;

		for (int32_t i = (int32_t)renderProxies.size()-1; i >= 0 ; --i)
		{
			uint32_t timeInPool = renderProxies[(uint32_t)i]->getTimeInPool();

			if (timeInPool > 0)
			{
				if (timeInPool > mModule->getMaxTimeRenderProxyInPool() + 1) // +1 because we add them with time 1
				{
					PX_DELETE(renderProxies[(uint32_t)i]);
					renderProxies.replaceWithLast((uint32_t)i);
				}
				else
				{
					renderProxies[(uint32_t)i]->setTimeInPool(timeInPool+1);
				}
			}
		}
	}

	mRenderProxiesLock.unlock();
}

}
} // namespace nvidia

