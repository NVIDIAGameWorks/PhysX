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



#ifndef APEX_RW_LOCKABLE_H
#define APEX_RW_LOCKABLE_H

#include "RWLockable.h"
#include "PsThread.h"
#include "PsMutex.h"
#include "PsHashMap.h"

namespace nvidia
{
namespace apex
{

struct ThreadReadWriteCount 
{
	ThreadReadWriteCount() : value(0) {}
	union {
		struct {
			uint8_t readDepth;			// depth of re-entrant reads
			uint8_t writeDepth;		// depth of re-entrant writes 
			uint8_t readLockDepth;		// depth of read-locks
			uint8_t writeLockDepth;	// depth of write-locks
		} counters;
		uint32_t value;
	};
};

class ApexRWLockable : public RWLockable
{
public:
	ApexRWLockable();
	virtual ~ApexRWLockable();

	virtual void acquireReadLock(const char *fileName, const uint32_t lineno) const;
	virtual void acquireWriteLock(const char *fileName, const uint32_t lineno)const;
	virtual void releaseReadLock(void) const;
	virtual void releaseWriteLock(void) const;
	virtual uint32_t getReadWriteErrorCount() const;
	bool startWrite(bool allowReentry);
	void stopWrite(bool allowReentry);
	nvidia::Thread::Id getCurrentWriter() const;

	bool startRead() const;
	void stopRead() const;

	void setEnabled(bool);
	bool isEnabled() const;
private:
	bool											mEnabled;
	mutable volatile nvidia::Thread::Id		mCurrentWriter;
	mutable nvidia::ReadWriteLock			mRWLock;
	volatile int32_t									mConcurrentWriteCount;
	mutable volatile int32_t							mConcurrentReadCount;
	mutable volatile int32_t							mConcurrentErrorCount;
	nvidia::Mutex							mDataLock;
	typedef nvidia::HashMap<nvidia::ThreadImpl::Id, ThreadReadWriteCount> DepthsHashMap_t;
	mutable DepthsHashMap_t							mData;
};

#define APEX_RW_LOCKABLE_BOILERPLATE								\
	virtual void acquireReadLock(const char *fileName, const uint32_t lineno) const \
	{																\
		ApexRWLockable::acquireReadLock(fileName, lineno);					\
	}																\
	virtual void acquireWriteLock(const char *fileName, const uint32_t lineno) const\
	{																\
		ApexRWLockable::acquireWriteLock(fileName, lineno);			\
	}																\
	virtual void releaseReadLock(void)									const\
	{																\
		ApexRWLockable::releaseReadLock();								\
	}																\
	virtual void releaseWriteLock(void)									const\
	{																\
		ApexRWLockable::releaseWriteLock();								\
	}																\
	virtual uint32_t getReadWriteErrorCount() const				\
	{																\
		return ApexRWLockable::getReadWriteErrorCount();			\
	}																\
	bool startWrite(bool allowReentry)								\
	{																\
		return ApexRWLockable::startWrite(allowReentry);			\
	}																\
	void stopWrite(bool allowReentry)								\
	{																\
		ApexRWLockable::stopWrite(allowReentry);					\
	}																\
	bool startRead() const											\
	{																\
		return ApexRWLockable::startRead();							\
	}																\
	void stopRead() const											\
	{																\
		ApexRWLockable::stopRead();									\
	}																\

class ApexRWLockableScopedDisable
{
public:
	ApexRWLockableScopedDisable(RWLockable*);
	~ApexRWLockableScopedDisable();
private:
	ApexRWLockableScopedDisable(const ApexRWLockableScopedDisable&);
	ApexRWLockableScopedDisable& operator=(const ApexRWLockableScopedDisable&);

	ApexRWLockable* mLockable;
};

#define APEX_RW_LOCKABLE_SCOPED_DISABLE(lockable) ApexRWLockableScopedDisable __temporaryDisable(lockable);

}
}

#endif // APEX_RW_LOCKABLE_H
