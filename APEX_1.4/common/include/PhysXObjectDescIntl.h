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



#ifndef PHYSX_OBJECT_DESC_INTL_H
#define PHYSX_OBJECT_DESC_INTL_H

#include "PhysXObjectDesc.h"

namespace nvidia
{
namespace apex
{

/**
 * Module/Asset interface to actor info structure.  This allows the asset to
 * set the various flags without knowing their implementation.
 */
class PhysXObjectDescIntl : public PhysXObjectDesc
{
public:
	void setIgnoreTransform(bool b)
	{
		if (b)
		{
			mFlags |= TRANSFORM;
		}
		else
		{
			mFlags &= ~(uint32_t)TRANSFORM;
		}
	};
	void setIgnoreRaycasts(bool b)
	{
		if (b)
		{
			mFlags |= RAYCASTS;
		}
		else
		{
			mFlags &= ~(uint32_t)RAYCASTS;
		}
	};
	void setIgnoreContacts(bool b)
	{
		if (b)
		{
			mFlags |= CONTACTS;
		}
		else
		{
			mFlags &= ~(uint32_t)CONTACTS;
		}
	};
	void setUserDefinedFlag(uint32_t index, bool b)
	{
		if (b)
		{
			mFlags |= (1 << index);
		}
		else
		{
			mFlags &= ~(1 << index);
		}
	}

	/**
	\brief Implementation of pure virtual functions in PhysXObjectDesc, used for external (read-only)
	access to the Actor list
	*/
	uint32_t				getApexActorCount() const
	{
		return mApexActors.size();
	}
	const Actor*	getApexActor(uint32_t i) const
	{
		return mApexActors[i];
	}


	void swap(PhysXObjectDescIntl& rhs)
	{
		mApexActors.swap(rhs.mApexActors);
		nvidia::swap(mPhysXObject, rhs.mPhysXObject);

		nvidia::swap(userData, rhs.userData);
		nvidia::swap(mFlags, rhs.mFlags);
	}

	/**
	\brief Array of pointers to APEX actors assiciated with this PhysX object

	Pointers may be NULL in cases where the APEX actor has been deleted
	but PhysX actor cleanup has been deferred
	*/
	physx::Array<const Actor*>	mApexActors;

	/**
	\brief the PhysX object which uses this descriptor
	*/
	const void* mPhysXObject;
protected:
	virtual ~PhysXObjectDescIntl(void) {}
};

}
} // end namespace nvidia::apex

#endif // PHYSX_OBJECT_DESC_INTL_H
