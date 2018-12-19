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

#if PX_PHYSICS_VERSION_MAJOR == 3

#pragma warning(push)
#pragma warning(disable: 4324)

#include <ApexSceneUserNotify.h>
#include "PxPreprocessor.h"
#include <PxJoint.h>
#include <PxScene.h>

namespace nvidia
{
namespace apex
{
	using namespace physx;

ApexSceneUserNotify::~ApexSceneUserNotify(void)
{
	// All callbacks should have been removed by now... something is wrong.
	PX_ASSERT(mModuleNotifiers.size() == 0);
}

void ApexSceneUserNotify::addModuleNotifier(PxSimulationEventCallback& notify)
{
	mModuleNotifiers.pushBack(&notify);
}

void ApexSceneUserNotify::removeModuleNotifier(PxSimulationEventCallback& notify)
{
	const uint32_t numNotifiers = mModuleNotifiers.size();
	uint32_t found = numNotifiers;
	for (uint32_t i = 0; i < numNotifiers; i++)
	{
		if (mModuleNotifiers[i] == &notify)
		{
			found = i;
			break;
		}
	}
	PX_ASSERT(found < numNotifiers);
	if (found < numNotifiers)
	{
		mModuleNotifiers.replaceWithLast(found);
	}
}

void ApexSceneUserNotify::playBatchedNotifications()
{
#if TODO_HANDLE_NEW_CONTACT_STREAM
	// onConstraintBreak
	{
		for (uint32_t i = 0; i < mBatchedBreakNotifications.size(); i++)
		{
			physx::PxConstraintInfo& constraintInfo = mBatchedBreakNotifications[i];
			PX_ASSERT(mAppNotify != NULL);
			mAppNotify->onConstraintBreak(&constraintInfo, 1);

			/*
			// apan, shold we release joint? how?
			if (releaseJoint)
			{
			// scene isn't running anymore, guess we need to release the joint by hand.
			notify.breakingJoint->getScene().releaseJoint(*notify.breakingJoint);
			}
			*/
		}
		// release if the array is too big
		if (mBatchedBreakNotifications.size() * 4 < mBatchedBreakNotifications.capacity())
		{
			mBatchedBreakNotifications.shrink();
		}

		mBatchedBreakNotifications.clear();
	}


	// onContact
	{
		for (uint32_t i = 0; i < mBatchedContactNotifications.size(); i++)
		{
			BatchedContactNotification& contact = mBatchedContactNotifications[i];
			PX_ASSERT(contact.batchedStreamStart < mBatchedContactStreams.size());
			contact.batchedPair.stream = (PxConstContactStream)(mBatchedContactStreams.begin() + contact.batchedStreamStart);

			mAppNotify->onContact(contact.batchedPair, contact.batchedEvents);
		}
		mBatchedContactNotifications.clear();
		mBatchedContactStreams.clear();
	}

	// onSleep/onWake
	{
		for (uint32_t i = 0; i < mBatchedSleepWakeEventBorders.size(); i++)
		{
			const SleepWakeBorders border = mBatchedSleepWakeEventBorders[i];
			if (border.sleepEvents)
			{
				mAppNotify->onSleep(&mBatchedSleepEvents[border.start], border.count);
			}
			else
			{
				mAppNotify->onWake(&mBatchedWakeEvents[border.start], border.count);
			}
		}
		mBatchedSleepWakeEventBorders.clear();
		mBatchedSleepEvents.clear();
		mBatchedWakeEvents.clear();
	}

	// mBatchedTriggerReports
	{
		for (uint32_t i = 0; i < mBatchedTriggerReports.size(); i++)
		{
			PxTriggerPair& triggerPair = mBatchedTriggerReports[i];
			mAppNotify->onTrigger(&triggerPair, 1);
		}
		mBatchedTriggerReports.clear();
	}
#endif
}

void ApexSceneUserNotify::onConstraintBreak(physx::PxConstraintInfo* constraints, uint32_t count)
{
	for (Array<PxSimulationEventCallback*>::Iterator curr = mModuleNotifiers.begin(); curr != mModuleNotifiers.end(); ++curr)
	{
		(*curr)->onConstraintBreak(constraints, count);
	}

	if (mAppNotify != NULL)
	{
		if (mBatchAppNotify)
		{
			for (uint32_t i = 0 ; i < count; i++)
			{
				mBatchedBreakNotifications.pushBack(constraints[i]);
			}
		}
		else
		{
			mAppNotify->onConstraintBreak(constraints, count);
		}
	}
}

void ApexSceneUserNotify::onWake(PxActor** actors, uint32_t count)
{
	for (Array<PxSimulationEventCallback*>::Iterator curr = mModuleNotifiers.begin(); curr != mModuleNotifiers.end(); ++curr)
	{
		(*curr)->onWake(actors, count);
	}

	if (mAppNotify != NULL)
	{
		if (mBatchAppNotify)
		{
			SleepWakeBorders border(mBatchedWakeEvents.size(), count, false);
			mBatchedSleepWakeEventBorders.pushBack(border);
			mBatchedWakeEvents.resize(mBatchedWakeEvents.size() + count);
			for (uint32_t i = 0; i < count; i++)
			{
				mBatchedWakeEvents.pushBack(actors[i]);
			}
		}
		else
		{
			mAppNotify->onWake(actors, count);
		}
	}
}

void ApexSceneUserNotify::onSleep(PxActor** actors, uint32_t count)
{
	for (Array<PxSimulationEventCallback*>::Iterator curr = mModuleNotifiers.begin(); curr != mModuleNotifiers.end(); ++curr)
	{
		(*curr)->onSleep(actors, count);
	}
	if (mAppNotify)
	{
		if (mBatchAppNotify)
		{
			SleepWakeBorders border(mBatchedSleepEvents.size(), count, true);
			mBatchedSleepWakeEventBorders.pushBack(border);
			mBatchedSleepEvents.resize(mBatchedSleepEvents.size() + count);
			for (uint32_t i = 0; i < count; i++)
			{
				mBatchedSleepEvents.pushBack(actors[i]);
			}
		}
		else
		{
			mAppNotify->onSleep(actors, count);
		}
	}
}


void ApexSceneUserNotify::onContact(const physx::PxContactPairHeader& pairHeader, const PxContactPair* pairs, uint32_t nbPairs)
{
	for (Array<PxSimulationEventCallback*>::Iterator curr = mModuleNotifiers.begin(); curr != mModuleNotifiers.end(); ++curr)
	{
		(*curr)->onContact(pairHeader, pairs, nbPairs);
	}

	if (mAppNotify)
	{
		if (mBatchAppNotify)
		{
#if TODO_HANDLE_NEW_CONTACT_STREAM
			mBatchedContactNotifications.pushBack(BatchedContactNotification(pairHeader, pairs, nbPairs));
			const uint32_t length = pair.contactCount; //getContactStreamLength(pair.stream);
			for (uint32_t i = 0; i < length; i++)
			{
				mBatchedContactStreams.pushBack(pair.stream[i]);
			}
#endif
		}
		else
		{
			mAppNotify->onContact(pairHeader, pairs, nbPairs);
		}
	}
}


#if TODO_HANDLE_NEW_CONTACT_STREAM
class ApexContactStreamIterator : public PxContactStreamIterator
{
public:
	ApexContactStreamIterator( PxConstContactStream streamIt) : PxContactStreamIterator(streamIt) { }

	PxConstContactStream getStreamIt()	{	return streamIt; }
};
#endif

void ApexSceneUserNotify::onTrigger(PxTriggerPair* pairs, uint32_t count)
{
	if (mAppNotify != NULL)
	{
		if (mBatchAppNotify)
		{
			for (uint32_t i = 0; i < count; i++)
			{
				mBatchedTriggerReports.pushBack(pairs[count]);
			}
		}
		else
		{
			mAppNotify->onTrigger(pairs, count);
		}
	}
}


void ApexSceneUserNotify::onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count)
{
	PX_UNUSED(bodyBuffer);
	PX_UNUSED(poseBuffer);
	PX_UNUSED(count);
}




ApexSceneUserContactModify::ApexSceneUserContactModify(void)
{
	mAppContactModify = 0;
}

ApexSceneUserContactModify::~ApexSceneUserContactModify(void)
{
	// All callbacks should have been removed by now... something is wrong.
	PX_ASSERT(mModuleContactModify.size() == 0);
}

void ApexSceneUserContactModify::addModuleContactModify(PxContactModifyCallback& contactModify)
{
	mModuleContactModify.pushBack(&contactModify);
}

void ApexSceneUserContactModify::removeModuleContactModify(PxContactModifyCallback& contactModify)
{
	const uint32_t numContactModifies = mModuleContactModify.size();
	uint32_t       found      = numContactModifies;
	for (uint32_t i = 0; i < numContactModifies; i++)
	{
		if (mModuleContactModify[i] == &contactModify)
		{
			found = i;
			break;
		}
	}
	PX_ASSERT(found < numContactModifies);
	if (found < numContactModifies)
	{
		mModuleContactModify.replaceWithLast(found);
	}
}

void ApexSceneUserContactModify::setApplicationContactModify(PxContactModifyCallback* contactModify)
{
	mAppContactModify = contactModify;
}

void ApexSceneUserContactModify::onContactModify(PxContactModifyPair* const pairs, uint32_t count)
{
	for (Array<PxContactModifyCallback*>::Iterator curr = mModuleContactModify.begin(); curr != mModuleContactModify.end(); curr++)
	{
		(*curr)->onContactModify(pairs, count);
	}
	if (mAppContactModify)
	{
		mAppContactModify->onContactModify(pairs, count);
	}
}



}
} // namespace nvidia::apex
#pragma warning(pop)

#endif // PX_PHYSICS_VERSION_MAJOR == 3