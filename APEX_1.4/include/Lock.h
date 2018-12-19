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


#ifndef LOCK_H
#define LOCK_H

/*!
\file
\brief classes PxSceneReadLock, PxSceneWriteLock
*/

#include "ApexInterface.h"

namespace nvidia
{
namespace apex
{

/**
\brief RAII wrapper for the Scene read lock.

Use this class as follows to lock the scene for reading by the current thread 
for the duration of the enclosing scope:

	ReadLock lock(sceneRef);

\see Scene::apexacquireReadLock(), Scene::apexUnacquireReadLock(), SceneDesc::useRWLock
*/
class ReadLock
{
	ReadLock(const ReadLock&);
	ReadLock& operator=(const ReadLock&);

public:
	
	/**
	\brief Constructor
	\param lockable The object to lock for reading
	\param file Optional string for debugging purposes
	\param line Optional line number for debugging purposes
	*/
	ReadLock(const ApexInterface& lockable, const char* file=NULL, uint32_t line=0)
		: mLockable(lockable)
	{
		mLockable.acquireReadLock(file, line);
	}

	~ReadLock()
	{
		mLockable.releaseReadLock();
	}

private:

	const ApexInterface& mLockable;
};

/**
\brief RAII wrapper for the Scene write lock.

Use this class as follows to lock the scene for writing by the current thread 
for the duration of the enclosing scope:

	WriteLock lock(sceneRef);

\see Scene::apexacquireWriteLock(), Scene::apexUnacquireWriteLock(), SceneDesc::useRWLock
*/
class WriteLock
{
	WriteLock(const WriteLock&);
	WriteLock& operator=(const WriteLock&);

public:

	/**
	\brief Constructor
	\param lockable The object to lock for writing
	\param file Optional string for debugging purposes
	\param line Optional line number for debugging purposes
	*/
	WriteLock(const ApexInterface& lockable, const char* file=NULL, uint32_t line=0)
		: mLockable(lockable)
	{
		mLockable.acquireWriteLock(file, line);
	}

	~WriteLock()
	{
		mLockable.releaseWriteLock();
	}

private:
	const ApexInterface& mLockable;
};


} // namespace apex
} // namespace nvidia

/**
\brief Lock an object for writing by the current thread for the duration of the enclosing scope.
*/
#define WRITE_LOCK(LOCKABLE) nvidia::apex::WriteLock __WriteLock(LOCKABLE, __FILE__, __LINE__);
/**
\brief Lock an object for reading by the current thread for the duration of the enclosing scope.
*/
#define READ_LOCK(LOCKABLE) nvidia::apex::ReadLock __ReadLock(LOCKABLE, __FILE__, __LINE__);

/** @} */
#endif
