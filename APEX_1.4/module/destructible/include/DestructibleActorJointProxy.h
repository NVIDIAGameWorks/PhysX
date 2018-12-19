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



#ifndef __DESTRUCTIBLEACTORJOINT_PROXY_H__
#define __DESTRUCTIBLEACTORJOINT_PROXY_H__

#include "Apex.h"
#include "DestructibleActorJoint.h"
#include "DestructibleActorJointImpl.h"
#include "PsUserAllocated.h"
#include "ApexRWLockable.h"
#include "ReadCheck.h"
#include "WriteCheck.h"

namespace nvidia
{
namespace destructible
{

class DestructibleScene;	// Forward declaration

class DestructibleActorJointProxy : public DestructibleActorJoint, public ApexResourceInterface, public UserAllocated, public ApexRWLockable
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	DestructibleActorJointImpl impl;

#pragma warning(disable : 4355) // disable warning about this pointer in argument list
	DestructibleActorJointProxy(const DestructibleActorJointDesc& destructibleActorJointDesc, DestructibleScene& dscene, ResourceList& list) :
		impl(destructibleActorJointDesc, dscene)
	{
		list.add(*this);
	};

	~DestructibleActorJointProxy()
	{
	};

	// DestructibleActorJoint methods
	virtual PxJoint* joint()
	{
		WRITE_ZONE();
		return impl.getJoint();
	}

	virtual void release()
	{
		// impl.release();
		delete this;
	};

	// ApexResourceInterface methods
	virtual void	setListIndex(ResourceList& list, uint32_t index)
	{
		impl.m_listIndex = index;
		impl.m_list = &list;
	}

	virtual uint32_t	getListIndex() const
	{
		return impl.m_listIndex;
	}
};

}
} // end namespace nvidia

#endif // __DESTRUCTIBLEACTORJOINT_PROXY_H__
