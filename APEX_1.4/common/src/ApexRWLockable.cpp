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


// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "ApexRWLockable.h"
#include "PsThread.h"
#include "PsAtomic.h"
#include "ApexSDKIntl.h"

namespace nvidia
{
namespace apex
{

ApexRWLockable::ApexRWLockable()
	: 
	mEnabled (true)
	, mCurrentWriter(0)
	, mRWLock()
	, mConcurrentWriteCount	(0)
	, mConcurrentReadCount	(0)
	, mConcurrentErrorCount	(0)
{
}

ApexRWLockable::~ApexRWLockable()
{
}

void ApexRWLockable::setEnabled(bool e)
{
	mEnabled = e;
}

bool ApexRWLockable::isEnabled() const
{
	return mEnabled;
}

void ApexRWLockable::acquireReadLock(const char*, uint32_t) const
{
	mRWLock.lockReader();
}

void ApexRWLockable::acquireWriteLock(const char*, uint32_t) const
{
	mRWLock.lockWriter();
	mCurrentWriter = Thread::getId();
}

void ApexRWLockable::releaseReadLock(void) const
{
	mRWLock.unlockReader();
}

void ApexRWLockable::releaseWriteLock(void) const
{
	mCurrentWriter = 0;
	mRWLock.unlockWriter();
}

bool ApexRWLockable::startWrite(bool allowReentry) 
{
	PX_COMPILE_TIME_ASSERT(sizeof(ThreadReadWriteCount) == 4);
	mDataLock.lock();
	bool error = false;

	ThreadReadWriteCount& rwc = mData[Thread::getId()];
	// check that we are the only thread reading (this allows read->write order on a single thread)
	error |= mConcurrentReadCount != rwc.counters.readDepth;

	// check no other threads are writing 
	error |= mConcurrentWriteCount != rwc.counters.writeDepth;

	// increment shared write counter
	shdfnd::atomicIncrement(&mConcurrentWriteCount);

	// in the normal case (re-entry is allowed) then we simply increment
	// the writeDepth by 1, otherwise (re-entry is not allowed) increment
	// by 2 to force subsequent writes to fail by creating a mismatch between
	// the concurrent write counter and the local counter, any value > 1 will do
	if (allowReentry)
		rwc.counters.writeDepth++;
	else
		rwc.counters.writeDepth += 2;
	mDataLock.unlock();

	if (error)
		shdfnd::atomicIncrement(&mConcurrentErrorCount);

	return !error;
}

void ApexRWLockable::stopWrite(bool allowReentry)
{
	shdfnd::atomicDecrement(&mConcurrentWriteCount);

	// decrement depth of writes for this thread
	mDataLock.lock();
	nvidia::ThreadImpl::Id id = nvidia::Thread::getId();

	// see comment in startWrite()
	if (allowReentry)
		mData[id].counters.writeDepth--;
	else
		mData[id].counters.writeDepth -= 2;

	mDataLock.unlock();
}

nvidia::Thread::Id ApexRWLockable::getCurrentWriter() const
{
	return mCurrentWriter;
}

bool ApexRWLockable::startRead() const
{
	shdfnd::atomicIncrement(&mConcurrentReadCount);

	mDataLock.lock();
	nvidia::ThreadImpl::Id id = nvidia::Thread::getId();
	ThreadReadWriteCount& rwc = mData[id];
	rwc.counters.readDepth++;
	bool success = (rwc.counters.writeDepth > 0 || mConcurrentWriteCount == 0); 
	mDataLock.unlock();

	if (!success)
		shdfnd::atomicIncrement(&mConcurrentErrorCount);

	return success;
}

void ApexRWLockable::stopRead() const
{
	shdfnd::atomicDecrement(&mConcurrentReadCount);
	mDataLock.lock();
	nvidia::ThreadImpl::Id id = nvidia::Thread::getId();
	mData[id].counters.readDepth--;
	mDataLock.unlock();
}

uint32_t ApexRWLockable::getReadWriteErrorCount() const
{
	return static_cast<uint32_t>(mConcurrentErrorCount);
}

ApexRWLockableScopedDisable::ApexRWLockableScopedDisable(RWLockable* rw) : mLockable(static_cast<ApexRWLockable*>(rw))
{
	mLockable->setEnabled(false);
}

ApexRWLockableScopedDisable::~ApexRWLockableScopedDisable()
{
	mLockable->setEnabled(true);
}

ApexRWLockableScopedDisable::ApexRWLockableScopedDisable(const ApexRWLockableScopedDisable& o) : mLockable(o.mLockable)
{
}

ApexRWLockableScopedDisable& ApexRWLockableScopedDisable::operator=(const ApexRWLockableScopedDisable&)
{
	return *this;
}

}
}