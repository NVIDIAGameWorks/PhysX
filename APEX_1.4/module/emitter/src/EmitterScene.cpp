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



#include "Apex.h"
#include "ApexUsingNamespace.h"
#include "EmitterScene.h"
#include "SceneIntl.h"
#include "ModulePerfScope.h"

#include "Lock.h"

namespace nvidia
{
namespace emitter
{

EmitterScene::EmitterScene(ModuleEmitterImpl& module, SceneIntl& scene, RenderDebugInterface* debugRender, ResourceList& list) :
	mSumBenefit(0.0f),
	mDebugRender(debugRender)
{
	mModule = &module;
	mApexScene = &scene;
	list.add(*this);		// Add self to module's list of EmitterScenes

	/* Initialize reference to EmitterDebugRenderParams */
	{
		READ_LOCK(*mApexScene);
		mDebugRenderParams = DYNAMIC_CAST(DebugRenderParams*)(mApexScene->getDebugRenderParams());
	}
	PX_ASSERT(mDebugRenderParams);
	NvParameterized::Handle handle(*mDebugRenderParams), memberHandle(*mDebugRenderParams);
	int size;

	if (mDebugRenderParams->getParameterHandle("moduleName", handle) == NvParameterized::ERROR_NONE)
	{
		handle.getArraySize(size, 0);
		handle.resizeArray(size + 1);
		if (handle.getChildHandle(size, memberHandle) == NvParameterized::ERROR_NONE)
		{
			memberHandle.initParamRef(EmitterDebugRenderParams::staticClassName(), true);
		}
	}

	/* Load reference to EmitterDebugRenderParams */
	NvParameterized::Interface* refPtr = NULL;
	memberHandle.getParamRef(refPtr);
	mEmitterDebugRenderParams = DYNAMIC_CAST(EmitterDebugRenderParams*)(refPtr);
	PX_ASSERT(mEmitterDebugRenderParams);
}

EmitterScene::~EmitterScene()
{
}

void EmitterScene::visualize()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	for (uint32_t i = 0 ; i < mActorArray.size() ; i++)
	{
		EmitterActorBase* actor = DYNAMIC_CAST(EmitterActorBase*)(mActorArray[ i ]);
		actor->visualize(*mDebugRender);
	}
#endif
}

void EmitterScene::destroy()
{
	removeAllActors();
	mApexScene->moduleReleased(*this);
	delete this;
}

void EmitterScene::setModulePhysXScene(PxScene* scene)
{
	if (scene)
	{
		for (uint32_t i = 0 ; i < mActorArray.size() ; i++)
		{
			EmitterActorBase* actor = DYNAMIC_CAST(EmitterActorBase*)(mActorArray[ i ]);
			actor->setPhysXScene(scene);
		}
	}
	else
	{
		for (uint32_t i = 0 ; i < mActorArray.size() ; i++)
		{
			EmitterActorBase* actor = DYNAMIC_CAST(EmitterActorBase*)(mActorArray[ i ]);
			actor->setPhysXScene(NULL);
		}
	}

	mPhysXScene = scene;
}

void EmitterScene::submitTasks(float /*elapsedTime*/, float /*substepSize*/, uint32_t /*numSubSteps*/)
{
	for (uint32_t i = 0 ; i < mActorArray.size() ; i++)
	{
		EmitterActorBase* actor = DYNAMIC_CAST(EmitterActorBase*)(mActorArray[ i ]);
		actor->submitTasks();
	}
}

void EmitterScene::setTaskDependencies()
{
	for (uint32_t i = 0 ; i < mActorArray.size() ; i++)
	{
		EmitterActorBase* actor = DYNAMIC_CAST(EmitterActorBase*)(mActorArray[ i ]);
		actor->setTaskDependencies();
	}
}

// Called by ApexScene simulation thread after PhysX scene is stepped. All
// actors in the scene are render-locked.
void EmitterScene::fetchResults()
{
	PX_PROFILE_ZONE("EmitterSceneFetchResults", GetInternalApexSDK()->getContextId());

	for (uint32_t i = 0 ; i < mActorArray.size() ; i++)
	{
		EmitterActorBase* actor = DYNAMIC_CAST(EmitterActorBase*)(mActorArray[ i ]);
		actor->fetchResults();
	}
}

}
} // namespace nvidia::apex
