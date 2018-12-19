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

#include "WriteCheck.h"
#include "ApexRWLockable.h"
#include "PsThread.h"
#include "ApexSDKIntl.h"

namespace nvidia
{
namespace apex
{

WriteCheck::WriteCheck(ApexRWLockable* scene, const char* functionName, bool allowReentry) 
: mLockable(scene), mName(functionName), mAllowReentry(allowReentry), mErrorCount(0)
{
	if (GetApexSDK()->isConcurrencyCheckEnabled() && mLockable)
	{
		if (!mLockable->startWrite(mAllowReentry) && !mLockable->isEnabled())
		{
			APEX_INTERNAL_ERROR("An API write call (%s) was made from thread %d but acquireWriteLock() was not called first, note that "
				"when ApexSDKDesc::enableConcurrencyCheck is enabled all API reads and writes must be "
				"wrapped in the appropriate locks.", mName, uint32_t(nvidia::Thread::getId()));
		}

		// Record the Scene read/write error counter which is
		// incremented any time a Scene::apexStartWrite/apexStartRead fails
		// (see destructor for additional error checking based on this count)
		mErrorCount = mLockable->getReadWriteErrorCount();
	}
}


WriteCheck::~WriteCheck()
{
	if (GetApexSDK()->isConcurrencyCheckEnabled() && mLockable)
	{
		// By checking if the NpScene::mConcurrentErrorCount has been incremented
		// we can detect if an erroneous read/write was performed during 
		// this objects lifetime. In this case we also print this function's
		// details so that the user can see which two API calls overlapped
		if (mLockable->getReadWriteErrorCount() != mErrorCount && !mLockable->isEnabled())
		{
			APEX_INTERNAL_ERROR("Leaving %s on thread %d, an overlapping API read or write by another thread was detected.", mName, uint32_t(nvidia::Thread::getId()));
		}

		mLockable->stopWrite(mAllowReentry);
	}
}

}
}