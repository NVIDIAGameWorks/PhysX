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

#ifndef SAMPLE_STEPPER_H
#define SAMPLE_STEPPER_H

#include "SampleAllocator.h"
#include "SampleAllocatorSDKClasses.h"
#include "RendererMemoryMacros.h"
#include "task/PxTask.h"
#include "PxPhysicsAPI.h"
#include "PsTime.h"

class PhysXSample;

class Stepper: public SampleAllocateable
{
	public:
							Stepper() : mSample(NULL) {}
	virtual					~Stepper() {}

	virtual	bool			advance(PxScene* scene, PxReal dt, void* scratchBlock, PxU32 scratchBlockSize)	= 0;
	virtual	void			wait(PxScene* scene)				= 0;
	virtual void			substepStrategy(const PxReal stepSize, PxU32& substepCount, PxReal& substepSize) = 0;
	virtual void			postRender(const PxReal stepSize) = 0;
	
	virtual void			setSubStepper(const PxReal stepSize, const PxU32 maxSteps)	{}
	virtual	void			renderDone()							{}
	virtual	void			shutdown()								{}

	PxReal					getSimulationTime()	const				{ return mSimulationTime; }

	PhysXSample&			getSample()								{ return *mSample; }
	const PhysXSample&		getSample()	const						{ return *mSample; }
	void					setSample(PhysXSample* sample)			{ mSample = sample; }

protected:
	PhysXSample*			mSample;
	Ps::Time				mTimer;
	PxReal					mSimulationTime;

};

class MultiThreadStepper;
class StepperTask : public physx::PxLightCpuTask
{
public:
	void						setStepper(MultiThreadStepper* stepper) { mStepper = stepper; }
	MultiThreadStepper*			getStepper()							{ return mStepper; }
	const MultiThreadStepper*	getStepper() const 						{ return mStepper; }
	const char*					getName() const							{ return "Stepper Task"; }
	void						run();
protected:
	MultiThreadStepper*	mStepper;
};

class StepperTaskSimulate : public StepperTask
{
	
public:
	StepperTaskSimulate(){}
	void run();
};

class MultiThreadStepper : public Stepper
{
public:
	MultiThreadStepper()
		: mFirstCompletionPending(false)
		, mScene(NULL)
		, mSync(NULL)
		, mCurrentSubStep(0)
		, mNbSubSteps(0)
	{
		mCompletion0.setStepper(this);
		mCompletion1.setStepper(this);
		mSimulateTask.setStepper(this);
	}

	~MultiThreadStepper()	{}

	virtual bool			advance(PxScene* scene, PxReal dt, void* scratchBlock, PxU32 scratchBlockSize);
	virtual void			substepDone(StepperTask* ownerTask);
	virtual void			renderDone();
	virtual void			postRender(const PxReal stepSize){}
	
	// if mNbSubSteps is 0 then the sync will never 
	// be set so waiting would cause a deadlock
	virtual void			wait(PxScene* scene)	{	if(mNbSubSteps && mSync)mSync->wait();	}
	virtual void			shutdown()				{	DELETESINGLE(mSync);	}
	virtual void			reset() = 0;
	virtual void			substepStrategy(const PxReal stepSize, PxU32& substepCount, PxReal& substepSize) = 0;
	virtual void			simulate(physx::PxBaseTask* ownerTask);
	PxReal					getSubStepSize() const	{ return mSubStepSize; }

protected:
	void					substep(StepperTask& completionTask);

	// we need two completion tasks because when multistepping we can't submit completion0 from the
	// substepDone function which is running inside completion0
	bool				mFirstCompletionPending;
	StepperTaskSimulate	mSimulateTask;
	StepperTask			mCompletion0, mCompletion1;
	PxScene*			mScene;
	PsSyncAlloc*		mSync;

	PxU32				mCurrentSubStep;
	PxU32				mNbSubSteps;
	PxReal				mSubStepSize;
	void*				mScratchBlock;
	PxU32				mScratchBlockSize;
};

class DebugStepper : public Stepper
{
public:
	DebugStepper(const PxReal stepSize) : mStepSize(stepSize) {}
	
	virtual void substepStrategy(const PxReal stepSize, PxU32& substepCount, PxReal& substepSize)
	{
		substepCount = 1;
		substepSize = mStepSize;
	}

	virtual bool advance(PxScene* scene, PxReal dt, void* scratchBlock, PxU32 scratchBlockSize);

	virtual void			postRender(const PxReal stepSize)
	{
	}

	virtual void setSubStepper(const PxReal stepSize, const PxU32 maxSteps)
	{
		mStepSize = stepSize;
	}

	virtual void wait(PxScene* scene);

	PxReal mStepSize;
};

// The way this should be called is:
// bool stepped = advance(dt)
//
// ... reads from the scene graph for rendering
//
// if(stepped) renderDone()
//
// ... anything that doesn't need access to the physics scene
//
// if(stepped) sFixedStepper.wait()
//
// Note that per-substep callbacks to the sample need to be issued out of here, 
// between fetchResults and simulate

class FixedStepper : public MultiThreadStepper
{
public:
	FixedStepper(const PxReal subStepSize, const PxU32 maxSubSteps)
		: MultiThreadStepper()
		, mAccumulator(0)
		, mFixedSubStepSize(subStepSize)
		, mMaxSubSteps(maxSubSteps)
	{
	}

	virtual void	substepStrategy(const PxReal stepSize, PxU32& substepCount, PxReal& substepSize);
	virtual void	reset() { mAccumulator = 0.0f; }
	
	virtual void	setSubStepper(const PxReal stepSize, const PxU32 maxSteps) { mFixedSubStepSize = stepSize; mMaxSubSteps = maxSteps;}

	virtual void			postRender(const PxReal stepSize)
	{
	}

	PxReal	mAccumulator;
	PxReal	mFixedSubStepSize;
	PxU32	mMaxSubSteps;
};


class VariableStepper : public MultiThreadStepper
{
public:
	VariableStepper(const PxReal minSubStepSize, const PxReal maxSubStepSize, const PxU32 maxSubSteps)
		: MultiThreadStepper()
		, mAccumulator(0)
		, mMinSubStepSize(minSubStepSize)
		, mMaxSubStepSize(maxSubStepSize)
		, mMaxSubSteps(maxSubSteps)
	{
	}

	virtual void	substepStrategy(const PxReal stepSize, PxU32& substepCount, PxReal& substepSize);
	virtual void	reset() { mAccumulator = 0.0f; }

private:
	VariableStepper& operator=(const VariableStepper&);
			PxReal	mAccumulator;
	const	PxReal	mMinSubStepSize;
	const	PxReal	mMaxSubStepSize;
	const	PxU32	mMaxSubSteps;
};
#endif
