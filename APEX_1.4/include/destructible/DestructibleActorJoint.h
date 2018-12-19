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



#ifndef DESTRUCTIBLE_ACTOR_JOINT_H
#define DESTRUCTIBLE_ACTOR_JOINT_H

#include "ModuleDestructible.h"

#include "extensions/PxJoint.h"


namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
	Descriptor used to create the Destructible actor joint.
*/
class DestructibleActorJointDesc : public ApexDesc
{
public:

	/**
	\brief constructor sets to default.
	*/
	PX_INLINE				DestructibleActorJointDesc();

	/**
	\brief Resets descriptor to default settings.
	*/
	PX_INLINE void			setToDefault();

	/**
		Returns true iff an object can be created using this descriptor.
	*/
	PX_INLINE bool			isValid() const;

	/**
		PhysX SDK 3.X only.
	*/
	physx::PxJointConcreteType::Enum type;
	/**
		PhysX SDK 3.X only.
	*/
	PxRigidActor*			actor[2];
	/**
		PhysX SDK 3.X only.
	*/
	PxVec3					localAxis[2];
	/**
		PhysX SDK 3.X only.
	*/
	PxVec3					localAnchor[2];
	/**
		PhysX SDK 3.X only.
	*/
	PxVec3					localNormal[2];

	/**
		If destructible[i] is not NULL, it will effectively replace the actor[i] in jointDesc.
		At least one must be non-NULL.
	*/
	DestructibleActor*		destructible[2];

	/**
		If destructible[i] is not NULL and attachmentChunkIndex[i] is a valid chunk index within that
		destructible, then that chunk will be used for attachment.
		If destructible[i] is not NULL and attachmentChunkIndex[i] is NOT a valid chunk index (such as the
		default ModuleDestructibleConst::INVALID_CHUNK_INDEX), then the nearest chunk to globalAnchor[i] is used instead.
		Once a chunk is determined, the chunk's associated PxActor will be the PxJoint's attachment actor.
	*/
	int32_t					attachmentChunkIndex[2];


	/**
		Global attachment data.  Since destructibles come apart, there is no single referece frame
		associated with them.  Therefore it makes more sense to use a global reference frame
		when describing the attachment positions and axes.
	*/

	/**
	\brief global attachment positions
	*/
	PxVec3			globalAnchor[2];

	/**
	\brief global axes
	*/
	PxVec3			globalAxis[2];

	/**
	\brief global normals
	*/
	PxVec3			globalNormal[2];
};

// DestructibleActorJointDesc inline functions

PX_INLINE DestructibleActorJointDesc::DestructibleActorJointDesc() : ApexDesc()
{
	setToDefault();
}

PX_INLINE void DestructibleActorJointDesc::setToDefault()
{
	ApexDesc::setToDefault();
	for (int i=0; i<2; i++)
	{
		actor[i] = 0;
		localAxis[i]	= PxVec3(0,0,1);
		localNormal[i]	= PxVec3(1,0,0);
		localAnchor[i]	= PxVec3(0);
	}
	for (int i = 0; i < 2; ++i)
	{
		destructible[i] = NULL;
		attachmentChunkIndex[i] = ModuleDestructibleConst::INVALID_CHUNK_INDEX;
		globalAnchor[i] = PxVec3(0.0f);
		globalAxis[i] = PxVec3(0.0f, 0.0f, 1.0f);
		globalNormal[i] = PxVec3(1.0f, 0.0f, 0.0f);
	}
}

PX_INLINE bool DestructibleActorJointDesc::isValid() const
{
	if (destructible[0] == NULL && destructible[1] == NULL)
	{
		return false;
	}

	for (int i = 0; i < 2; ++i)
	{
		if (PxAbs(globalAxis[i].magnitudeSquared() - 1.0f) > 0.1f)
		{
			return false;
		}
		if (PxAbs(globalNormal[i].magnitudeSquared() - 1.0f) > 0.1f)
		{
			return false;
		}
		//check orthogonal pairs
		if (PxAbs(globalAxis[i].dot(globalNormal[i])) > 0.1f)
		{
			return false;
		}
	}

	return ApexDesc::isValid();
}


/**
	Destructible actor joint - a wrapper for an PxJoint.  This needs to be used because APEX may need to replace an PxJoint
	when fracturing occurs.  Always use the joint() method to get the currently valid joint.
*/
class DestructibleActorJoint : public ApexInterface
{
public:
	/**
		PhysX SDK 3.X
		Access to the {xJoint represented by the DestructibleActorJoint.  This is a temporary interface,
		and there are some shortcomings and restrictions:
		1) The user will have to upcast the result of joint() to the correct joint type in order
		to access the interface for derived joints.
		2) The user must never delete the joint using the PhysX SDK.  To release this joint, simply
		use the release() method of this object (defined in the ApexInterface base class).
	*/
	virtual	PxJoint*	joint()	= 0;

protected:
	virtual				~DestructibleActorJoint() {}
};

PX_POP_PACK

}
} // end namespace nvidia

#endif // DESTRUCTIBLE_ACTOR_JOINT_H
