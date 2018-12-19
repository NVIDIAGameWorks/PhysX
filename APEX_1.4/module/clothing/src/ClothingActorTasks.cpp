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
#include "ClothingActorTasks.h"
#include "ClothingActorImpl.h"
#include "ModulePerfScope.h"


namespace nvidia
{
namespace clothing
{

void ClothingActorBeforeTickTask::run()
{
#if APEX_UE4	// SHORTCUT_CLOTH_TASKS
	mActor->simulate(mDeltaTime);
#else
#ifdef PROFILE
	PIXBeginNamedEvent(0, "ClothingActorBeforeTickTask");
#endif
	//PX_ASSERT(mDeltaTime > 0.0f); // need to allow simulate(0) calls
	mActor->tickSynchBeforeSimulate_LocksPhysX(mDeltaTime, mSubstepSize, 0, mNumSubSteps);
#ifdef PROFILE
	PIXEndNamedEvent();
#endif
#endif // SHORTCUT_CLOTH_TASKS
}



const char* ClothingActorBeforeTickTask::getName() const
{
	return "ClothingActorImpl::BeforeTickTask";
}


// --------------------------------------------------------------------


void ClothingActorDuringTickTask::run()
{
	mActor->tickAsynch_NoPhysX();
}



const char* ClothingActorDuringTickTask::getName() const
{
	return "ClothingActorImpl::DuringTickTask";
}

// --------------------------------------------------------------------

void ClothingActorFetchResultsTask::run()
{
#ifdef PROFILE
	PIXBeginNamedEvent(0, "ClothingActorFetchResultsTask");
#endif
	mActor->fetchResults();
	ClothingActorData& actorData = mActor->getActorData();

	actorData.tickSynchAfterFetchResults_LocksPhysX();
#ifdef PROFILE
	PIXEndNamedEvent();
#endif
#if APEX_UE4
	mActor->setFetchResultsSync();
#endif
}


#if APEX_UE4
void ClothingActorFetchResultsTask::release()
{
	PxTask::release();
}
#endif


const char* ClothingActorFetchResultsTask::getName() const
{
	return "ClothingActorImpl::FetchResultsTask";
}


}
}


