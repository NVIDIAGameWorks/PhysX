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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "SampleStepper.h"
#include "PhysXSample.h"
#include "PxScene.h"


bool DebugStepper::advance(PxScene* scene, PxReal dt, void* scratchBlock, PxU32 scratchBlockSize)
{
	mTimer.getElapsedSeconds();
	
	{
		PxSceneWriteLock writeLock(*scene);
		scene->simulate(mStepSize, NULL, scratchBlock, scratchBlockSize);
	}

	return true;
}

void DebugStepper::wait(PxScene* scene)
{
	mSample->onSubstepPreFetchResult();
	{
		PxSceneWriteLock writeLock(*scene);
		scene->fetchResults(true, NULL);
	}
	mSimulationTime = (PxReal)mTimer.getElapsedSeconds();
	mSample->onSubstep(mStepSize);
}

void StepperTask::run()
{
	mStepper->substepDone(this);
	release();
}

void StepperTaskSimulate::run()
{
	mStepper->simulate(mCont);
	mStepper->getSample().onSubstepStart(mStepper->getSubStepSize());
}


void MultiThreadStepper::simulate(physx::PxBaseTask* ownerTask)
{
	PxSceneWriteLock writeLock(*mScene);

	mScene->simulate(mSubStepSize, ownerTask, mScratchBlock, mScratchBlockSize);
}

void MultiThreadStepper::renderDone()
{
	if(mFirstCompletionPending)
	{
		mCompletion0.removeReference();
		mFirstCompletionPending = false;
	}
}



bool MultiThreadStepper::advance(PxScene* scene, PxReal dt, void* scratchBlock, PxU32 scratchBlockSize)
{
	mScratchBlock = scratchBlock;
	mScratchBlockSize = scratchBlockSize;

	if(!mSync)
		mSync = SAMPLE_NEW(PsSyncAlloc);

	substepStrategy(dt, mNbSubSteps, mSubStepSize);
	
	if(mNbSubSteps == 0) return false;

	mScene = scene;

	mSync->reset();

	mCurrentSubStep = 1;

	mCompletion0.setContinuation(*mScene->getTaskManager(), NULL);

	mSimulationTime = 0.0f;
	mTimer.getElapsedSeconds();

	// take first substep
	substep(mCompletion0);	
	mFirstCompletionPending = true;

	return true;
}

void MultiThreadStepper::substepDone(StepperTask* ownerTask)
{
	mSample->onSubstepPreFetchResult();

	{
#if !PX_PROFILE
		PxSceneWriteLock writeLock(*mScene);
#endif
		mScene->fetchResults(true);
	}

	PxReal delta = (PxReal)mTimer.getElapsedSeconds();
	mSimulationTime += delta;

	mSample->onSubstep(mSubStepSize);

	if(mCurrentSubStep>=mNbSubSteps)
	{
		mSync->set();
	}
	else
	{
		StepperTask &s = ownerTask == &mCompletion0 ? mCompletion1 : mCompletion0;
		s.setContinuation(*mScene->getTaskManager(), NULL);
		mCurrentSubStep++;

		mTimer.getElapsedSeconds();

		substep(s);

		// after the first substep, completions run freely
		s.removeReference();
	}
}


void MultiThreadStepper::substep(StepperTask& completionTask)
{
	// setup any tasks that should run in parallel to simulate()
	mSample->onSubstepSetup(mSubStepSize, &completionTask);

	// step
	{
		mSimulateTask.setContinuation(&completionTask);
		mSimulateTask.removeReference();
	}
	// parallel sample tasks are started in mSolveTask (after solve was called which acquires a write lock).
}

void FixedStepper::substepStrategy(const PxReal stepSize, PxU32& substepCount, PxReal& substepSize)
{
	if(mAccumulator > mFixedSubStepSize)
		mAccumulator = 0.0f;

	// don't step less than the step size, just accumulate
	mAccumulator  += stepSize;
	if(mAccumulator < mFixedSubStepSize)
	{
		substepCount = 0;
		return;
	}

	substepSize = mFixedSubStepSize;
	substepCount = PxMin(PxU32(mAccumulator/mFixedSubStepSize), mMaxSubSteps);

	mAccumulator -= PxReal(substepCount)*substepSize;
}

void VariableStepper::substepStrategy(const PxReal stepSize, PxU32& substepCount, PxReal& substepSize)
{
	if(mAccumulator > mMaxSubStepSize)
		mAccumulator = 0.0f;

	// don't step less than the min step size, just accumulate
	mAccumulator  += stepSize;
	if(mAccumulator < mMinSubStepSize)
	{
		substepCount = 0;
		return;
	}

	substepCount = PxMin(PxU32(PxCeil(mAccumulator/mMaxSubStepSize)), mMaxSubSteps);
	substepSize = PxMin(mAccumulator/substepCount, mMaxSubStepSize);

	mAccumulator -= PxReal(substepCount)*substepSize;
}


