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

#include "ClothingCooking.h"
#include "PsMemoryBuffer.h"
#include "ParamArray.h"

#include "CookingAbstract.h"

#include "ClothingScene.h"

#include "PsAtomic.h"

namespace nvidia
{
namespace clothing
{

void ClothingCookingLock::lockCooking()
{
	atomicIncrement(&mNumCookingDependencies);
}


void ClothingCookingLock::unlockCooking()
{
	atomicDecrement(&mNumCookingDependencies);
}


ClothingCookingTask::ClothingCookingTask(ClothingScene* clothingScene, CookingAbstract& _job) : job(&_job), nextTask(NULL),
	mState(Uninit), mClothingScene(clothingScene), mResult(NULL), mLockedObject(NULL)
{
	PX_ASSERT(((size_t)(void*)(&mState) & 0x00000003) == 0); // check alignment, mState must be aligned for the atomic exchange operations

	PX_ASSERT(job != NULL);
}


ClothingCookingTask::~ClothingCookingTask()
{
	if (job != NULL)
	{
		PX_DELETE_AND_RESET(job);
	}
}


void ClothingCookingTask::initCooking(PxTaskManager& tm, PxBaseTask* c)
{
	PxLightCpuTask::setContinuation(tm, c);

	int32_t oldState = atomicCompareExchange((int32_t*)(&mState), WaitForRun, Uninit);
	PX_ASSERT(oldState == Uninit);
	PX_UNUSED(oldState);
}


void ClothingCookingTask::run()
{
	// run
	NvParameterized::Interface* result = NULL;
	int32_t oldState = atomicCompareExchange((int32_t*)(&mState), Running, WaitForRun);
	PX_ASSERT(oldState != ReadyForRelease && oldState != Aborting && oldState != WaitForFetch);
	if (oldState == WaitForRun) // the change was successful
	{
		result = job->execute();
	}

	unlockObject();

	// try to run the next task. Must be called before in a state where it will be deleted
	mClothingScene->submitCookingTask(NULL);

	// finished
	oldState = atomicCompareExchange((int32_t*)(&mState), WaitForFetch, Running);
	if (oldState == Running)
	{
		mResult = result;
	}
	else
	{
		if (result != NULL)
		{
			result->destroy();
			result = NULL;
		}
		atomicExchange((int32_t*)(&mState), ReadyForRelease);
		PX_ASSERT(mResult == NULL);
	}
}

NvParameterized::Interface* ClothingCookingTask::getResult()
{
	if (mResult != NULL)
	{
		int32_t oldState = atomicCompareExchange((int32_t*)(&mState), ReadyForRelease, WaitForFetch);
		PX_ASSERT(oldState == WaitForFetch);
		PX_UNUSED(oldState);
		return mResult;
	}
	return NULL;
}

void ClothingCookingTask::lockObject(ClothingCookingLock* lockedObject)
{
	if (mLockedObject != NULL)
	{
		mLockedObject->unlockCooking();
	}

	mLockedObject = lockedObject;
	if (mLockedObject != NULL)
	{
		mLockedObject->lockCooking();
	}
}

void ClothingCookingTask::unlockObject()
{
	if (mLockedObject != NULL)
	{
		mLockedObject->unlockCooking();
		mLockedObject = NULL;
	}
}

void ClothingCookingTask::abort()
{
	int32_t oldState = atomicExchange((int32_t*)(&mState), Aborting);
	PX_ASSERT(oldState >= WaitForRun);
	if (oldState == WaitForFetch)
	{
		atomicExchange((int32_t*)(&mState), ReadyForRelease);
		if (mResult != NULL)
		{
			NvParameterized::Interface* oldParam = mResult;
			mResult = NULL;
			oldParam->destroy();
		}
		unlockObject();
	}
	else if (oldState != Running)
	{
		PX_ASSERT(mResult == NULL);
		atomicExchange((int32_t*)(&mState), ReadyForRelease);
		unlockObject();
	}
}

}
}

