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



#ifndef PHYS_XOBJECT_DESC_H
#define PHYS_XOBJECT_DESC_H

/*!
\file
\brief class PhysXObjectDesc
*/

#include "ApexUsingNamespace.h"

namespace nvidia
{
namespace apex
{

class Actor;

PX_PUSH_PACK_DEFAULT

/**
 * \brief PhysX object descriptor
 *
 * Class which describes how a PhysX object is being used by APEX.  Data access is
 * non virtual for performance reasons.
 */
class PhysXObjectDesc
{
protected:
	/**
	\brief Object interaction flags

	These flags determine how this PhysX object should interact with user callbacks.  For
	instance whether contact callbacks with the object should be ignored.
	*/
	uint32_t				mFlags;

	enum
	{
		TRANSFORM   = (1U << 31),	//!< If set, ignore this object's xform in active actor callbacks
		RAYCASTS    = (1U << 30), //!< If set, ignore this object in raycasts
		CONTACTS    = (1U << 29) //!< If set, ignore this object in contact callbacks
	};


public:
	/** \brief Returns the number of Actors associated with the PhysX object */
	virtual uint32_t				getApexActorCount() const = 0;
	/** \brief Returns the indexed Actor pointer */
	virtual const Actor*	getApexActor(uint32_t i) const = 0;

	/** \brief Returns whether this object's xform should be ignored */
	bool ignoreTransform() const
	{
		return (mFlags & (uint32_t)TRANSFORM) ? true : false;
	}
	/** \brief Returns whether this object should be ignored by raycasts */
	bool ignoreRaycasts() const
	{
		return (mFlags & (uint32_t)RAYCASTS) ? true : false;
	}
	/** \brief Returns whether this object should be ignored by contact report callbacks */
	bool ignoreContacts() const
	{
		return (mFlags & (uint32_t)CONTACTS) ? true : false;
	}
	/** \brief Returns a user defined status bit */
	bool getUserDefinedFlag(uint32_t index) const
	{
		return (mFlags & (uint32_t)(1 << index)) ? true : false;
	}

	/**
	\brief User data, for use by APEX

	For internal use by APEX.  Please do not modify this field.  You may use the PhysX object
	userData or Actor userData field.
	*/
	void* 				userData;
};

// To get owning Actor's authorable object type name:
//		getActor()->getOwner()->getAuthObjName();
// To get owning Actor's authorable object type ID:
//		getActor()->getOwner()->getAuthObjType();

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // PHYS_XOBJECT_DESC_H
