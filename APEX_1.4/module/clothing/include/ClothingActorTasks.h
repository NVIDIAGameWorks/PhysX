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



#ifndef CLOTHING_ACTOR_TASKS_H
#define CLOTHING_ACTOR_TASKS_H

#include "PxTask.h"

#if APEX_UE4
#include "PsSync.h"
#include "ApexInterface.h"
#endif

namespace nvidia
{
namespace clothing
{

class ClothingActorImpl;
class ClothingActorData;


class ClothingActorBeforeTickTask : public physx::PxLightCpuTask
{
public:
	ClothingActorBeforeTickTask(ClothingActorImpl* actor) : mActor(actor), mDeltaTime(0.0f), mSubstepSize(0.0f), mNumSubSteps(0) {}
#if APEX_UE4
	~ClothingActorBeforeTickTask() {}
#endif

	PX_INLINE void setDeltaTime(float simulationDelta, float substepSize, uint32_t numSubSteps)
	{
		mDeltaTime = simulationDelta;
		mSubstepSize = substepSize;
		mNumSubSteps = numSubSteps;
	}

	virtual void        run();
	virtual const char* getName() const;

private:
	ClothingActorImpl* mActor;
	float mDeltaTime;
	float mSubstepSize;
	uint32_t mNumSubSteps;
};



class ClothingActorDuringTickTask : public physx::PxTask
{
public:
	ClothingActorDuringTickTask(ClothingActorImpl* actor) : mActor(actor) {}

	virtual void		run();
	virtual const char*	getName() const;

private:
	ClothingActorImpl* mActor;
};



class ClothingActorFetchResultsTask : 
#if APEX_UE4
	public PxTask
#else
	public physx::PxLightCpuTask
#endif
{
public:
	ClothingActorFetchResultsTask(ClothingActorImpl* actor) : mActor(actor) {}

	virtual void		run();
	virtual const char*	getName() const;
#if APEX_UE4
	virtual void release();
#endif

private:
	ClothingActorImpl* mActor;
};


}
} // namespace nvidia

#endif
