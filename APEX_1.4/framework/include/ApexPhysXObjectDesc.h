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



#ifndef __APEX_PHYSX_OBJECT_DESC_H__
#define __APEX_PHYSX_OBJECT_DESC_H__

#include "Apex.h"
#include "PhysXObjectDescIntl.h"

namespace nvidia
{
namespace apex
{

class ApexPhysXObjectDesc : public PhysXObjectDescIntl
{
public:
	typedef PhysXObjectDescIntl Parent;
	ApexPhysXObjectDesc() : mNext(0), mPrev(0)
	{
		mFlags = 0;
		userData = NULL;
		mPhysXObject = NULL;
	}

	// Need a copy constructor because we contain an array, and we are in arrays
	ApexPhysXObjectDesc(const ApexPhysXObjectDesc& desc) : PhysXObjectDescIntl(desc)
	{
		*this = desc;
	}

	ApexPhysXObjectDesc& operator = (const ApexPhysXObjectDesc& desc)
	{
		mFlags = desc.mFlags;
		userData = desc.userData;
		mApexActors = desc.mApexActors;
		mPhysXObject = desc.mPhysXObject;
		mNext = desc.mNext;
		mPrev = desc.mPrev;
		return *this;
	}

	void swap(ApexPhysXObjectDesc& rhs)
	{
		Parent::swap(rhs);
		shdfnd::swap(mNext, rhs.mNext);
		shdfnd::swap(mPrev, rhs.mPrev);
	}

	static uint16_t makeHash(size_t hashable);

	uint32_t		mNext, mPrev;

	friend class ApexSDKImpl;
	virtual ~ApexPhysXObjectDesc(void)
	{

	}
};

}
} // end namespace nvidia::apex

#endif // __APEX_PHYSX_OBJECT_DESC_H__
