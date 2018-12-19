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



#ifndef CLOTHING_COOKING_H
#define CLOTHING_COOKING_H

#include "ApexUsingNamespace.h"
#include "PsUserAllocated.h"

#include "PxTask.h"

namespace NvParameterized
{
class Interface;
}


namespace nvidia
{
namespace clothing
{

class ClothingScene;
class CookingAbstract;


class ClothingCookingLock
{
public:
	ClothingCookingLock() : mNumCookingDependencies(0) {}
	~ClothingCookingLock()
	{
		PX_ASSERT(mNumCookingDependencies == 0);
	}

	int32_t numCookingDependencies()
	{
		return mNumCookingDependencies;
	}
	void lockCooking();
	void unlockCooking();
private:
	int32_t mNumCookingDependencies;
};



/* These tasks contain a cooking job. They have no dependencies on each other, but the ClothingScene
 * make sure there's only ever one of them running (per ClothingScene that is).
 * ClothingScene::submitCookingTasks() will launch a new one only if no other cooking task is running.
 * At ClothingScene::fetchResults() this will be checked again.
 */
class ClothingCookingTask : public UserAllocated, public PxLightCpuTask
{
public:
	ClothingCookingTask(ClothingScene* clothingScene, CookingAbstract& job);
	~ClothingCookingTask();


	CookingAbstract* job;
	ClothingCookingTask* nextTask;

	// from LightCpuTask
	void				initCooking(PxTaskManager& tm, PxBaseTask* c);
	virtual const char* getName() const
	{
		return "ClothingCookingTask";
	}
	virtual void        run();

	NvParameterized::Interface* getResult();

	void lockObject(ClothingCookingLock* lockedObject);
	void unlockObject();

	bool waitsForBeingScheduled()
	{
		return mRefCount > 0;
	}
	bool readyForRelease()
	{
		return mState == ReadyForRelease;
	}
	void abort();

private:
	enum State
	{
		Uninit,
		WaitForRun,
		Running,
		Aborting,
		WaitForFetch,
		ReadyForRelease,
	};
	State mState;
	ClothingScene* mClothingScene;
	NvParameterized::Interface* mResult;
	ClothingCookingLock* mLockedObject;
};

}
} // namespace nvidia


#endif // CLOTHING_COOKING_H
