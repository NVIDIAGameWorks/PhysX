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



#ifndef __EMITTER_SCENE_H__
#define __EMITTER_SCENE_H__

#include "Apex.h"

#include "ModuleEmitterImpl.h"
#include "ApexSDKIntl.h"
#include "ModuleIntl.h"
#include "ApexContext.h"
#include "ApexSDKHelpers.h"
#include "ApexActor.h"

#include "DebugRenderParams.h"
#include "EmitterDebugRenderParams.h"

#include "PxTask.h"

namespace nvidia
{
namespace apex
{
class SceneIntl;
}
namespace emitter
{

class ModuleEmitterImpl;


/* Each Emitter Actor should derive this class, so the scene can deal with it */
class EmitterActorBase : public ApexActor
{
public:
	virtual bool		isValid()
	{
		return mValid;
	}
	virtual void		tick() = 0;
	virtual void					visualize(RenderDebugInterface& renderDebug) = 0;

	virtual void		submitTasks() = 0;
	virtual void		setTaskDependencies() = 0;
	virtual void		fetchResults() = 0;

protected:
	EmitterActorBase() : mValid(false) {}

	bool mValid;
};

class EmitterScene : public ModuleSceneIntl, public ApexContext, public ApexResourceInterface, public ApexResource
{
public:
	EmitterScene(ModuleEmitterImpl& module, SceneIntl& scene, RenderDebugInterface* debugRender, ResourceList& list);
	~EmitterScene();

	/* ModuleSceneIntl */
	void				visualize();
	void				setModulePhysXScene(PxScene* s);
	PxScene*			getModulePhysXScene() const
	{
		return mPhysXScene;
	}
	PxScene*			mPhysXScene;

	Module*			getModule()
	{
		return mModule;
	}

	void				fetchResults();

	virtual SceneStats* getStats()
	{
		return 0;
	}

	bool							lockRenderResources()
	{
		renderLockAllActors();	// Lock options not implemented yet
		return true;
	}

	bool							unlockRenderResources()
	{
		renderUnLockAllActors();	// Lock options not implemented yet
		return true;
	}

	/* ApexResourceInterface */
	uint32_t		getListIndex() const
	{
		return m_listIndex;
	}
	void				setListIndex(ResourceList& list, uint32_t index)
	{
		m_listIndex = index;
		m_list = &list;
	}
	void				release()
	{
		mModule->releaseModuleSceneIntl(*this);
	}

	void				submitTasks(float elapsedTime, float substepSize, uint32_t numSubSteps);
	void				setTaskDependencies();

protected:
	void                destroy();

	ModuleEmitterImpl*		mModule;
	SceneIntl*		mApexScene;

	float		mSumBenefit;
private:
	RenderDebugInterface* mDebugRender;

	DebugRenderParams*					mDebugRenderParams;
	EmitterDebugRenderParams*			mEmitterDebugRenderParams;

	friend class ModuleEmitterImpl;
	friend class EmitterActorImpl;
	friend class GroundEmitterActorImpl;
	friend class ImpactEmitterActorImpl;
};

}
} // end namespace nvidia

#endif
