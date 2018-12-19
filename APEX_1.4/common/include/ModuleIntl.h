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



#ifndef MODULE_INTL_H
#define MODULE_INTL_H

#include "ApexSDK.h"

typedef struct CUgraphicsResource_st* CUgraphicsResource;

// REMOVE OLD REGISTERS/UNREGISTER FACTORY PREPROCESSOR APPROACHES
// #define PARAM_CLASS_DECLARE_FACTORY(clas) clas ## Factory m ## clas ## Factory;
// #define PARAM_CLASS_REGISTER_FACTORY(t, clas) t->registerFactory(m ## clas ## Factory);
// #define PARAM_CLASS_REMOVE_FACTORY(t, clas) t->removeFactory(clas::staticClassName()); clas::freeParameterDefinitionTable(t);

namespace physx
{
	namespace pvdsdk
	{
		class PvdDataStream;
	}
}

namespace nvidia
{
namespace apex
{

class SceneIntl;
class ModuleSceneIntl;
class RenderDebugInterface;
class ApexActor;
class Actor;
class ModuleCachedDataIntl;
struct SceneStats;

/**
Framework interface to modules for use by ApexScenes and the ApexSDK
*/
class ModuleIntl
{
public:
	ModuleIntl(void)
	{
		mParent = NULL;
		mCreateOk = true;
	}
	virtual ~ModuleIntl(void)
	{
		if ( mParent )
		{
			mParent->notifyChildGone(this);
		}
	}
	/**
	Cause a module to free all of its resources.  Only callable from ApexSDK::releaseModule()
	*/
	virtual void destroy() = 0;

	/**
	Notification from ApexSDK when it is being destructed and will, therefore, be releasing all modules
	*/
	virtual void notifyReleaseSDK(void) 
	{

	}

	/**
	Inits Classes sent to Pvd from this module
	*/
	virtual void initPvdClasses(pvdsdk::PvdDataStream& /*pvdDataStream*/)
	{
	}

	/**
	Inits Instances when Pvd connects
	*/
	virtual void initPvdInstances(pvdsdk::PvdDataStream& /*pvdDataStream*/)
	{
	}

	/**
	Called by a newly created Scene to instantiate an ModuleSceneIntl.  Can also be
	called when modules are created after scenes.  If your module does
	not create ApexActors, this function can return NULL.

	The debug render that the scene is to use is also passed.
	*/
	virtual ModuleSceneIntl* createInternalModuleScene(SceneIntl& apexScene, RenderDebugInterface*) = 0;

	/**
	Release an ModuleSceneIntl.  Only called when an ApexScene has been released.
	All actors and other resources in the context should be released.
	*/
	virtual void           releaseModuleSceneIntl(ModuleSceneIntl& moduleScene) = 0;

	/**
	Module can provide a data cache for its objects.  It is valid to return NULL.
	*/
	virtual ModuleCachedDataIntl*	getModuleDataCache()
	{
		return NULL;
	}

	/**
	Returns the number of assets force loaded by all of the module's loaded assets
	Default impl returns 0, maybe this should be something really bad
	*/
	virtual uint32_t          forceLoadAssets()
	{
		return 0;
	}

	virtual ApexActor*     getApexActor(Actor*, AuthObjTypeID) const
	{
		return NULL;
	}

	virtual void setParent(ModuleIntl *parent)
	{
		mParent = parent;
	}

	virtual void notifyChildGone(ModuleIntl *child)
	{
		PX_UNUSED(child);
	}

	void setCreateOk(bool state)
	{
		mCreateOk = state;
	}

	bool isCreateOk(void) const
	{
		return mCreateOk;
	}

	bool			mCreateOk;
	ModuleIntl	*mParent;
};

class ModuleSceneIntl
{
public:

	/**
	ModuleSceneIntl::simulate() is called by ApexScene::simulate() from the context of the
	APEX API call (typically the main game thread).  Context sensitive code should run here.
	Note that the task manager will be executing tasks while simulate() is running, so it must
	be thread safe.
	\param elapsedTime The time passed to the Scene::simulate call
	*/
	virtual void		simulate(float elapsedTime)
	{
		PX_UNUSED(elapsedTime);
	}

	/**
	\brief If the PhysX scene runs with multiple substeps, modules can request manual substepping
	*/
	virtual bool		needsManualSubstepping() const
	{
		return false;
	}

	virtual void		interStep(uint32_t substepNumber, uint32_t maxSubSteps)
	{
		PX_UNUSED(substepNumber);
		PX_UNUSED(maxSubSteps);
	}

	/**
	ModuleSceneIntl::submitTasks() is called by ApexScene::simulate() at the start of every
	simulation step.  Each module should submit tasks within this function call, though
	they are not restricted from submitting tasks later if they require.
	\param elapsedTime The time passed to the Scene::simulate call
	\param numSubSteps Will be >1 if manual sub stepping is turned on, 1 otherwise
	*/
	virtual void		submitTasks(float elapsedTime, float substepSize, uint32_t numSubSteps) = 0;

	/**
	ModuleSceneIntl::setTaskDependencies() is called by ApexScene::simulate() after every
	module has had the opportunity to submit their tasks to the task manager.  Therefore it
	is safe to set dependencies in this function based on cross-module TaskID APIs.
	*/
	virtual void		setTaskDependencies() {}

	/**
	ModuleSceneIntl::fetchResults() is called by ApexScene::fetchResults() from the context of
	the APEX API call (typically the main game thread).  All renderable actors are locked by
	the scene for the length of this function call.
	*/
	virtual void		fetchResults() = 0;

	virtual void		fetchResultsPreRenderLock() {}
	virtual void		fetchResultsPostRenderUnlock() {}

#if PX_PHYSICS_VERSION_MAJOR == 3
	/**
	Called by ApexScene when its PxScene reference has changed.  Provided pointer can be NULL.
	*/
	virtual void		setModulePhysXScene(PxScene* s) = 0;
	virtual PxScene*	getModulePhysXScene() const = 0;
#endif // PX_PHYSICS_VERSION_MAJOR == 3

	/**
	Called by ApexScene when it has been released.  The ModuleSceneIntl must call its
	module's releaseModuleSceneIntl() method.
	*/
	virtual void		release() = 0;

	/**
	\brief Visualize the module's contents, using the new debug rendering facilities.

	This gets called from Scene::updateRenderResources
	*/
	virtual void		visualize() = 0;

	/**
	\brief Returns the corresponding Module.

	This allows to get to information like the module name.
	*/
	virtual Module*	getModule() = 0;

	/**
	\brief Lock render resources according to module scene-defined behavior.

	Returns true iff successful.
	*/
	virtual	bool		lockRenderResources() { return false; }

	/**
	\brief Unlock render resources according to module scene-defined behavior.

	Returns true iff successful.
	*/
	virtual	bool		unlockRenderResources() { return false; }

	virtual SceneStats* 	getStats() = 0;

	/**
	\brief return ApexCudaObj from CudaModuleScene or NULL for non CUDA scenes
	Should be implemented only for scenes that inherited from CudaModuleScene
	*/
	virtual void*				getHeadCudaObj()
	{		
		return NULL;
	}
};

}
} // end namespace nvidia::apex

#endif // MODULE_INTL_H
