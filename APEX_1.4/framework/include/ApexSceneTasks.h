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



#ifndef APEX_SCENE_TASKS_H
#define APEX_SCENE_TASKS_H

#include "ApexScene.h"

#include "PsAllocator.h"

namespace nvidia
{
namespace apex
{

class PhysXSimulateTask : public PxTask, public UserAllocated
{
public:
	PhysXSimulateTask(ApexScene& scene, CheckResultsTask& checkResultsTask); 
	~PhysXSimulateTask();
	
	const char* getName() const;
	void run();
	void setElapsedTime(float elapsedTime);
	void setFollowingTask(PxBaseTask* following);

#if PX_PHYSICS_VERSION_MAJOR == 3
	void setScratchBlock(void* scratchBlock, uint32_t size)
	{
		mScratchBlock = scratchBlock;
		mScratchBlockSize = size;
	}
#endif

protected:
	ApexScene* mScene;
	float mElapsedTime;

	PxBaseTask* mFollowingTask;
	CheckResultsTask& mCheckResultsTask;

#if PX_PHYSICS_VERSION_MAJOR == 3
	void*			mScratchBlock;
	uint32_t			mScratchBlockSize;
#endif

private:
	PhysXSimulateTask& operator=(const PhysXSimulateTask&);
};



class CheckResultsTask : public PxTask, public UserAllocated
{
public:
	CheckResultsTask(ApexScene& scene) : mScene(&scene) {}

	const char* getName() const;
	void run();

protected:
	ApexScene* mScene;
};



class FetchResultsTask : public PxTask, public UserAllocated
{
public:
	FetchResultsTask(ApexScene& scene) 
	:	mScene(&scene)
	,	mFollowingTask(NULL)
	{}

	const char* getName() const;
	void run();

	/**
	* \brief Called by dispatcher after Task has been run.
	*
	* If you re-implement this method, you must call this base class
	* version before returning.
	*/
	void release();

	void setFollowingTask(PxBaseTask* following);

protected:
	ApexScene*					mScene;
	PxBaseTask*	mFollowingTask;
};


/**  
*	This task is solely meant to record the duration of APEX's "during tick" tasks.
*	It could be removed and replaced with only the check results task if it is found
*	to be a performance issue.
*/
#if APEX_DURING_TICK_TIMING_FIX
class DuringTickCompleteTask : public PxTask, public UserAllocated
{
public:
	DuringTickCompleteTask(ApexScene& scene) : mScene(&scene) {}

	const char* getName() const;
	void run();

protected:
	ApexScene* mScene;
};
#endif

/* This tasks loops all intermediate steps until the final fetchResults can be called */
class PhysXBetweenStepsTask : public PxLightCpuTask, public UserAllocated
{
public:
	PhysXBetweenStepsTask(ApexScene& scene) : mScene(scene), mSubStepSize(0.0f),
		mNumSubSteps(0), mSubStepNumber(0), mLast(NULL) {}

	const char* getName() const;
	void run();
	void setSubstepSize(float substepSize, uint32_t numSubSteps);
	void setFollower(uint32_t substepNumber, PxTask* last);

protected:
	ApexScene& mScene;
	float mSubStepSize;
	uint32_t mNumSubSteps;

	uint32_t mSubStepNumber;
	PxTask* mLast;

private:
	PhysXBetweenStepsTask& operator=(const PhysXBetweenStepsTask&);
};

}
}

#endif
