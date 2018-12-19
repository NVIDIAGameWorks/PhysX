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



#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include "PxTask.h"
#include "PxTaskManager.h"
#include "PxCpuDispatcher.h"
#include "PsSList.h"
#include "PsSync.h"
#include "PsThread.h"
#include "ApexUsingNamespace.h"

namespace physx
{
	namespace pvdsdk
	{
		class ApexPvdClient;
	}
}

namespace nvidia
{
namespace apex
{

PxCpuDispatcher* createDefaultThreadPool(unsigned int numThreads);

class SharedQueueEntry : public shdfnd::SListEntry
{
public:
	SharedQueueEntry(void* objectRef) : mObjectRef(objectRef), mPooledEntry(false) {}
	SharedQueueEntry() : mObjectRef(NULL), mPooledEntry(true) {}

public:
	void* mObjectRef;
	bool mPooledEntry; // True if the entry was preallocated in a pool
};

template<class Alloc = typename shdfnd::AllocatorTraits<SharedQueueEntry>::Type >
class SharedQueueEntryPool : private Alloc
{
public:
	SharedQueueEntryPool(uint32_t poolSize, const Alloc& alloc = Alloc());
	~SharedQueueEntryPool();

	SharedQueueEntry* getEntry(void* objectRef);
	void putEntry(SharedQueueEntry& entry);

private:
	SharedQueueEntry*					mTaskEntryPool;
	shdfnd::SList						mTaskEntryPtrPool;
};

template <class Alloc>
SharedQueueEntryPool<Alloc>::SharedQueueEntryPool(uint32_t poolSize, const Alloc& alloc)
	: Alloc(alloc)
{
	shdfnd::AlignedAllocator<PX_SLIST_ALIGNMENT, Alloc> alignedAlloc;

	mTaskEntryPool = poolSize ? (SharedQueueEntry*)alignedAlloc.allocate(sizeof(SharedQueueEntry) * poolSize, __FILE__, __LINE__) : NULL;

	if (mTaskEntryPool)
	{
		for (uint32_t i = 0; i < poolSize; i++)
		{
			PX_ASSERT((size_t(&mTaskEntryPool[i]) & (PX_SLIST_ALIGNMENT - 1)) == 0); // The SList entry must be aligned according to PX_SLIST_ALIGNMENT

			PX_PLACEMENT_NEW(&mTaskEntryPool[i], SharedQueueEntry)();
			PX_ASSERT(mTaskEntryPool[i].mPooledEntry == true);
			mTaskEntryPtrPool.push(mTaskEntryPool[i]);
		}
	}
}


template <class Alloc>
SharedQueueEntryPool<Alloc>::~SharedQueueEntryPool()
{
	if (mTaskEntryPool)
	{
		shdfnd::AlignedAllocator<PX_SLIST_ALIGNMENT, Alloc> alignedAlloc;
		alignedAlloc.deallocate(mTaskEntryPool);
	}
}


template <class Alloc>
SharedQueueEntry* SharedQueueEntryPool<Alloc>::getEntry(void* objectRef)
{
	SharedQueueEntry* e = static_cast<SharedQueueEntry*>(mTaskEntryPtrPool.pop());
	if (e)
	{
		PX_ASSERT(e->mPooledEntry == true);
		e->mObjectRef = objectRef;
		return e;
	}
	else
	{
		shdfnd::AlignedAllocator<PX_SLIST_ALIGNMENT, Alloc> alignedAlloc;
		e = (SharedQueueEntry*)alignedAlloc.allocate(sizeof(SharedQueueEntry), __FILE__, __LINE__);
		if (e)
		{
			PX_PLACEMENT_NEW(e, SharedQueueEntry)(objectRef);
			PX_ASSERT(e->mPooledEntry == false);
		}

		return e;
	}
}


template <class Alloc>
void SharedQueueEntryPool<Alloc>::putEntry(SharedQueueEntry& entry)
{
	if (entry.mPooledEntry)
	{
		entry.mObjectRef = NULL;
		mTaskEntryPtrPool.push(entry);
	}
	else
	{
		shdfnd::AlignedAllocator<PX_SLIST_ALIGNMENT, Alloc> alignedAlloc;
		alignedAlloc.deallocate(&entry);
	}
}

#define TASK_QUEUE_ENTRY_POOL_SIZE 128

class TaskQueueHelper
{
public:
	static PxBaseTask* fetchTask(shdfnd::SList& taskQueue, SharedQueueEntryPool<>& entryPool)
	{
		SharedQueueEntry* entry = static_cast<SharedQueueEntry*>(taskQueue.pop());
		if (entry)
		{
			PxBaseTask* task = reinterpret_cast<PxBaseTask*>(entry->mObjectRef);
			entryPool.putEntry(*entry);
			return task;
		}
		else
		{
			return NULL;
		}
	}
};


class DefaultCpuDispatcher;

class CpuWorkerThread : public shdfnd::Thread
{
public:
	CpuWorkerThread();
	~CpuWorkerThread();

	void					initialize(DefaultCpuDispatcher* ownerDispatcher);
	void					execute();
	bool					tryAcceptJobToLocalQueue(PxBaseTask& task, shdfnd::Thread::Id taskSubmitionThread);
	PxBaseTask*		giveUpJob();

protected:
	SharedQueueEntryPool<>			mQueueEntryPool;
	DefaultCpuDispatcher*			mOwner;
	shdfnd::SList      				mLocalJobList;
	shdfnd::Thread::Id				mThreadId;
	pvdsdk::ApexPvdClient			*mApexPvdClient;
};

/*
 * Default CpuDispatcher implementation, if none is provided
 */
class DefaultCpuDispatcher : public PxCpuDispatcher, public shdfnd::UserAllocated
{
	friend class TaskQueueHelper;

private:
	DefaultCpuDispatcher() : mQueueEntryPool(0) {}
	~DefaultCpuDispatcher();

public:
	DefaultCpuDispatcher(uint32_t numThreads, uint32_t* affinityMasks);
	void					submitTask(PxBaseTask& task);
	void					flush( PxBaseTask& task, int32_t targetRef);
	uint32_t					getWorkerCount() const;
	void					release();

	PxBaseTask*				getJob();
	PxBaseTask*				stealJob();
	void					waitForWork()
	{
		mWorkReady.wait();
	}
	void					resetWakeSignal();
	static uint32_t			getAffinityMask(uint32_t affinityMask);

protected:
	CpuWorkerThread*				mWorkerThreads;
	SharedQueueEntryPool<>			mQueueEntryPool;
	shdfnd::SList					mJobList;
	shdfnd::Sync					mWorkReady;
	uint32_t						mNumThreads;
	bool							mShuttingDown;
	pvdsdk::ApexPvdClient			*mApexPvdClient;
};

} // end pxtask namespace
} // end physx namespace

#endif // _THREAD_POOL_H_
