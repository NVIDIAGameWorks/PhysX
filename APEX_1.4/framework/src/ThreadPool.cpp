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
#if PX_PHYSICS_VERSION_MAJOR == 0

#include "ApexSDK.h"

#include "PsThread.h"
#include "PsSList.h"
#include "PsSync.h"
#include "PsString.h"
#include "PsUserAllocated.h"
#include "PsAllocator.h"

#include "ThreadPool.h"
#include "ProfilerCallback.h"

#if PX_WINDOWS_FAMILY
#define PROFILE_TASKS 1
#else
#define PROFILE_TASKS 1
#endif

namespace nvidia
{
namespace apex
{

PxCpuDispatcher* createDefaultThreadPool(unsigned int numThreads)
{
	if (numThreads == 0)
	{
#if PX_WINDOWS_FAMILY
		numThreads = 4;
#elif PX_APPLE_FAMILY
		numThreads = 2;
#endif
	}
	return PX_NEW(DefaultCpuDispatcher)(numThreads, 0);
}

DefaultCpuDispatcher::DefaultCpuDispatcher(uint32_t numThreads, uint32_t* affinityMasks)
	: mQueueEntryPool(TASK_QUEUE_ENTRY_POOL_SIZE), mNumThreads(numThreads), mShuttingDown(false)
{
	uint32_t defaultAffinityMask = 0;

	// initialize threads first, then start

	mWorkerThreads = reinterpret_cast<CpuWorkerThread*>(PX_ALLOC(numThreads * sizeof(CpuWorkerThread), PX_DEBUG_EXP("CpuWorkerThread")));
	if (mWorkerThreads)
	{
		for (uint32_t i = 0; i < numThreads; ++i)
		{
			PX_PLACEMENT_NEW(mWorkerThreads + i, CpuWorkerThread)();
			mWorkerThreads[i].initialize(this);
		}

		for (uint32_t i = 0; i < numThreads; ++i)
		{
			mWorkerThreads[i].start(shdfnd::Thread::getDefaultStackSize());
			if (affinityMasks)
			{
				mWorkerThreads[i].setAffinityMask(affinityMasks[i]);
			}
			else
			{
				mWorkerThreads[i].setAffinityMask(defaultAffinityMask);
			}

			char threadName[32];
			shdfnd::snprintf(threadName, 32, "PxWorker%02d", i);
			mWorkerThreads[i].setName(threadName);
		}
	}
	else
	{
		mNumThreads = 0;
	}
}


DefaultCpuDispatcher::~DefaultCpuDispatcher()
{
	for (uint32_t i = 0; i < mNumThreads; ++i)
	{
		mWorkerThreads[i].signalQuit();
	}

	mShuttingDown = true;
	mWorkReady.set();
	for (uint32_t i = 0; i < mNumThreads; ++i)
	{
		mWorkerThreads[i].waitForQuit();
	}

	for (uint32_t i = 0; i < mNumThreads; ++i)
	{
		mWorkerThreads[i].~CpuWorkerThread();
	}

	PX_FREE(mWorkerThreads);
}


void DefaultCpuDispatcher::submitTask(PxBaseTask& task)
{
	shdfnd::Thread::Id currentThread = shdfnd::Thread::getId();

	// TODO: Could use TLS to make this more efficient
	for (uint32_t i = 0; i < mNumThreads; ++i)
		if (mWorkerThreads[i].tryAcceptJobToLocalQueue(task, currentThread))
		{
			return mWorkReady.set();
		}

	SharedQueueEntry* entry = mQueueEntryPool.getEntry(&task);
	if (entry)
	{
		mJobList.push(*entry);
		mWorkReady.set();
	}
}

void DefaultCpuDispatcher::flush( PxBaseTask& task, int32_t targetRef)
{
	// TODO: implement
	PX_ALWAYS_ASSERT();
	PX_UNUSED(task);
	PX_UNUSED(targetRef);
}

uint32_t DefaultCpuDispatcher::getWorkerCount() const
{
	return mNumThreads;
}

void DefaultCpuDispatcher::release()
{
	GetApexSDK()->releaseCpuDispatcher(*this);
}


PxBaseTask* DefaultCpuDispatcher::getJob(void)
{
	return TaskQueueHelper::fetchTask(mJobList, mQueueEntryPool);
}


PxBaseTask* DefaultCpuDispatcher::stealJob()
{
	PxBaseTask* ret = NULL;

	for (uint32_t i = 0; i < mNumThreads; ++i)
	{
		ret = mWorkerThreads[i].giveUpJob();

		if (ret != NULL)
		{
			break;
		}
	}

	return ret;
}


void DefaultCpuDispatcher::resetWakeSignal()
{
	mWorkReady.reset();

	// The code below is necessary to avoid deadlocks on shut down.
	// A thread usually loops as follows:
	// while quit is not signaled
	// 1)  reset wake signal
	// 2)  fetch work
	// 3)  if work -> process
	// 4)  else -> wait for wake signal
	//
	// If a thread reaches 1) after the thread pool signaled wake up,
	// the wake up sync gets reset and all other threads which have not
	// passed 4) already will wait forever.
	// The code below makes sure that on shutdown, the wake up signal gets
	// sent again after it was reset
	//
	if (mShuttingDown)
	{
		mWorkReady.set();
	}
}


CpuWorkerThread::CpuWorkerThread()
	:	mQueueEntryPool(TASK_QUEUE_ENTRY_POOL_SIZE)
	,	mThreadId(0)
{
}


CpuWorkerThread::~CpuWorkerThread()
{
}


void CpuWorkerThread::initialize(DefaultCpuDispatcher* ownerDispatcher)
{
	mOwner = ownerDispatcher;
}


bool CpuWorkerThread::tryAcceptJobToLocalQueue(PxBaseTask& task, shdfnd::Thread::Id taskSubmitionThread)
{
	if (taskSubmitionThread == mThreadId)
	{
		SharedQueueEntry* entry = mQueueEntryPool.getEntry(&task);
		if (entry)
		{
			mLocalJobList.push(*entry);
			return true;
		}
		else
		{
			return false;
		}
	}

	return false;
}


PxBaseTask* CpuWorkerThread::giveUpJob()
{
	return TaskQueueHelper::fetchTask(mLocalJobList, mQueueEntryPool);
}


void CpuWorkerThread::execute()
{
	mThreadId = getId();

	while (!quitIsSignalled())
	{
		mOwner->resetWakeSignal();

		PxBaseTask* task = TaskQueueHelper::fetchTask(mLocalJobList, mQueueEntryPool);

		if (!task)
		{
			task = mOwner->getJob();
		}

		if (!task)
		{
			task = mOwner->stealJob();
		}

		if (task)
		{
#if PHYSX_PROFILE_SDK
			if (mApexPvdClient!=NULL)
			{
				PX_PROFILE_ZONE(task->getName(), task->getContextId());
				task->run();
			}
			else
			{
				task->run();
			}
#else
			task->run();
#endif
			task->release();
		}
		else
		{
			mOwner->waitForWork();
		}
	}

	quit();
};



} // end pxtask namespace
} // end physx namespace

#endif // PX_PHYSICS_VERSION_MAJOR == 0
