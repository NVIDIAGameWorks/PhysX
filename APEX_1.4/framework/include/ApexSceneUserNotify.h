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



#ifndef APEX_SCENE_USER_NOTIFY_H
#define APEX_SCENE_USER_NOTIFY_H

#include "ApexDefs.h"

#if PX_PHYSICS_VERSION_MAJOR == 3

#include <ApexUsingNamespace.h>

#include <PxSimulationEventCallback.h>
#include <PxContactModifyCallback.h>

#include "PxSimpleTypes.h"
#include <PsArray.h>
#include <PsAllocator.h>


namespace nvidia
{
namespace apex
{

class ApexSceneUserNotify : public physx::PxSimulationEventCallback
{
public:
	ApexSceneUserNotify() : mAppNotify(NULL), mBatchAppNotify(false) {}
	virtual ~ApexSceneUserNotify();

	void addModuleNotifier(physx::PxSimulationEventCallback& notify);
	void removeModuleNotifier(physx::PxSimulationEventCallback& notify);

	void setApplicationNotifier(physx::PxSimulationEventCallback* notify)
	{
		mAppNotify = notify;
	}
	PxSimulationEventCallback* getApplicationNotifier() const
	{
		return mAppNotify;
	}

	void setBatchAppNotify(bool enable)
	{
		mBatchAppNotify = enable;
	}
	void playBatchedNotifications();

private:
	// from PxSimulationEventCallback
	virtual void onConstraintBreak(physx::PxConstraintInfo* constraints, uint32_t count);
	virtual void onWake(PxActor** actors, uint32_t count);
	virtual void onSleep(PxActor** actors, uint32_t count);
	virtual void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, uint32_t nbPairs);
	virtual void onTrigger(physx::PxTriggerPair* pairs, uint32_t count);
	virtual void onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count);

private:
	Array<physx::PxSimulationEventCallback*>	mModuleNotifiers;
	PxSimulationEventCallback*			mAppNotify;


	// for batch notification
	bool								mBatchAppNotify;

	// onConstraintBreak
	Array<physx::PxConstraintInfo>		mBatchedBreakNotifications;

	// onContact
	struct BatchedContactNotification
	{
		BatchedContactNotification(const physx::PxContactPairHeader& _pairHeader, const physx::PxContactPair* _pairs, uint32_t _nbPairs)
		{
			pairHeader			= _pairHeader;
			nbPairs				= _nbPairs;

			pairs = (physx::PxContactPair *)PX_ALLOC(sizeof(physx::PxContactPair) * nbPairs, PX_DEBUG_EXP("BatchedContactNotifications"));
			PX_ASSERT(pairs != NULL);
			for (uint32_t i=0; i<nbPairs; i++)
			{
				pairs[i] = _pairs[i];
			}
		}

		~BatchedContactNotification()
		{
			if (pairs)
			{
				PX_FREE(pairs);
				pairs = NULL;
			}
		}

		physx::PxContactPairHeader pairHeader;
		physx::PxContactPair *		pairs;
		uint32_t				nbPairs;
	};
	Array<BatchedContactNotification>		mBatchedContactNotifications;
	Array<uint32_t>							mBatchedContactStreams;

	// onWake/onSleep
	struct SleepWakeBorders
	{
		SleepWakeBorders(uint32_t s, uint32_t c, bool sleep) : start(s), count(c), sleepEvents(sleep) {}
		uint32_t start;
		uint32_t count;
		bool sleepEvents;
	};
	Array<SleepWakeBorders>				mBatchedSleepWakeEventBorders;
	Array<PxActor*>						mBatchedSleepEvents;
	Array<PxActor*>						mBatchedWakeEvents;

	// onTrigger
	Array<physx::PxTriggerPair> mBatchedTriggerReports;
};


class ApexSceneUserContactModify : public PxContactModifyCallback
{
public:
	ApexSceneUserContactModify();
	virtual ~ApexSceneUserContactModify();

	void addModuleContactModify(physx::PxContactModifyCallback& contactModify);
	void removeModuleContactModify(physx::PxContactModifyCallback& contactModify);

	void setApplicationContactModify(physx::PxContactModifyCallback* contactModify);
	PxContactModifyCallback* getApplicationContactModify() const
	{
		return mAppContactModify;
	}

private:
	// from PxContactModifyCallback
	virtual void onContactModify(physx::PxContactModifyPair* const pairs, uint32_t count);

private:
	Array<physx::PxContactModifyCallback*>	mModuleContactModify;
	PxContactModifyCallback*	   	mAppContactModify;
};

}
} // namespace nvidia::apex

#endif // PX_PHYSICS_VERSION_MAJOR == 3

#endif