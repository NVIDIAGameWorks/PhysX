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



#ifndef RWLOCKABLE_H
#define RWLOCKABLE_H

/**
\file
\brief The include containing the interface for any rw-lockable object in the APEX SDK
*/

#include "foundation/PxSimpleTypes.h"

#include "ApexUsingNamespace.h"

namespace nvidia
{
namespace apex
{

/**
 * \brief Base class for any rw-lockable object implemented by APEX SDK
 */
class RWLockable
{
public:
	/**
	\brief Acquire RW lock for read access.
	
	The APEX 1.3.3 SDK (and higher) provides a multiple-reader single writer mutex lock to coordinate	
	access to the APEX SDK API from multiple concurrent threads.  This method will in turn invoke the
	lockRead call on the APEX Scene.  The source code fileName and line number are provided for debugging
	purposes.
	*/
	virtual void acquireReadLock(const char *fileName, const uint32_t lineno) const = 0;

	/**
	\brief Acquire RW lock for write access.
	
	The APEX 1.3.3 SDK (and higher) provides a multiple-reader single writer mutex lock to coordinate	
	access to the APEX SDK API from multiple concurrent threads.  This method will in turn invoke the
	lockRead call on the APEX Scene.  The source code fileName and line number are provided for debugging
	purposes.
	*/
	virtual void acquireWriteLock(const char *fileName, const uint32_t lineno) const = 0;

	/**
	\brief Release the RW read lock
	*/
	virtual void releaseReadLock(void) const = 0;

	/**
	\brief Release the RW write lock
	*/
	virtual void releaseWriteLock(void) const = 0;
};

}
}
#endif // RWLOCKABLE_H
