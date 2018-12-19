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


#ifndef WRITE_CHECK_H
#define WRITE_CHECK_H

#include "PxSimpleTypes.h"
#include "ApexUsingNamespace.h"

namespace nvidia
{
namespace apex
{

class ApexRWLockable;

// RAII wrapper around the Scene::startWrite() method, note that this
// object does not acquire any scene locks, it is an error checking only mechanism
class WriteCheck
{
public: 
	WriteCheck(ApexRWLockable* scene, const char* functionName, bool allowReentry=true);
	~WriteCheck();

private:
	ApexRWLockable* mLockable;
	const char* mName;
	bool mAllowReentry;
	uint32_t mErrorCount;
};

#if PX_DEBUG || PX_CHECKED
	// Creates a scoped write check object that detects whether appropriate scene locks
	// have been acquired and checks if reads/writes overlap, this macro should typically
	// be placed at the beginning of any non-const API methods that are not multi-thread safe. 
	// By default re-entrant  write calls by the same thread are allowed, the error conditions 
	// checked can be summarized as:
	
	// 1. Other threads were already reading, or began reading during the object lifetime
	// 2. Other threads were already writing, or began writing during the object lifetime
	#define WRITE_ZONE() WriteCheck __writeCheck(static_cast<ApexRWLockable*>(this), __FUNCTION__);

	// Creates a scoped write check object that disallows re-entrant writes, this is used by
	// the Scene::simulate method to detect when callbacks make write calls to the API
	#define WRITE_ZONE_NOREENTRY() WriteCheck __writeCheck(static_cast<ApexRWLockable*>(this), __FUNCTION__, false);
#else
	#define WRITE_ZONE()
	#define WRITE_ZONE_NOREENTRY()
#endif

}
}

#endif // WRITE_CHECK_H
