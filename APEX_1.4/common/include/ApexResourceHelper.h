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



#ifndef __APEX_RESOURCE_HELPER_H__
#define __APEX_RESOURCE_HELPER_H__

#include "Apex.h"
#include "ResourceProviderIntl.h"

#if PX_PHYSICS_VERSION_MAJOR == 3
#include <PxFiltering.h>
#endif

namespace nvidia
{
namespace apex
{

class ApexResourceHelper
{
	ApexResourceHelper() {}
public:


#if PX_PHYSICS_VERSION_MAJOR == 3
	static PX_INLINE PxFilterData resolveCollisionGroup128(const char* collisionGroup128Name)
	{
		PxFilterData result; //default constructor sets all words to 0

		if (collisionGroup128Name)
		{
			/* create namespace for Collision Group (if it has not already been created) */
			ResourceProviderIntl* nrp = GetInternalApexSDK()->getInternalResourceProvider();
			ResID collisionGroup128NS = GetInternalApexSDK()->getCollisionGroup128NameSpace();
			ResID id = nrp->createResource(collisionGroup128NS, collisionGroup128Name);
			const uint32_t* resourcePtr = static_cast<const uint32_t*>(nrp->getResource(id));
			if (resourcePtr)
			{
				result.word0 = resourcePtr[0];
				result.word1 = resourcePtr[1];
				result.word2 = resourcePtr[2];
				result.word3 = resourcePtr[3];
			}
		}
		return result;
	}
#endif

	static PX_INLINE GroupsMask64 resolveCollisionGroup64(const char* collisionGroup64Name)
	{
		GroupsMask64 result(0, 0);

		if (collisionGroup64Name)
		{
			/* create namespace for Collision Group (if it has not already been created) */
			ResourceProviderIntl* nrp = GetInternalApexSDK()->getInternalResourceProvider();
			ResID collisionGroup64NS = GetInternalApexSDK()->getCollisionGroup64NameSpace();

			ResID id = nrp->createResource(collisionGroup64NS, collisionGroup64Name);
			const uint32_t* resourcePtr = static_cast<const uint32_t*>(nrp->getResource(id));
			if (resourcePtr)
			{
				result.bits0 = resourcePtr[0];
				result.bits1 = resourcePtr[1];
			}
		}
		return result;
	}

	static PX_INLINE uint32_t resolveCollisionGroupMask(const char* collisionGroupMaskName, uint32_t defGroupMask = 0xFFFFFFFFu)
	{
		uint32_t groupMask = defGroupMask;
		if (collisionGroupMaskName)
		{
			ResourceProviderIntl* nrp = GetInternalApexSDK()->getInternalResourceProvider();
			ResID collisionGroupMaskNS = GetInternalApexSDK()->getCollisionGroupMaskNameSpace();
			ResID id = nrp->createResource(collisionGroupMaskNS, collisionGroupMaskName);
			groupMask = (uint32_t)(size_t)(nrp->getResource(id));
		}
		return groupMask;
	}
};

}
} // end namespace nvidia::apex

#endif	// __APEX_RESOURCE_HELPER_H__
