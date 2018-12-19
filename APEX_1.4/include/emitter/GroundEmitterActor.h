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



#ifndef GROUND_EMITTER_ACTOR_H
#define GROUND_EMITTER_ACTOR_H

#include "Apex.h"

namespace nvidia
{
namespace apex
{


PX_PUSH_PACK_DEFAULT

class GroundEmitterAsset;
class RenderVolume;

/**
 \brief a user calback interface used to map raycast hitpoints to material ID
 * If an instance of this class is registered with a ground emitter actor, the actor will call
 * requestMaterialLookups() in lieu of doing raycasts.  The call will occur from within the scope
 * of the ApexScene thread, so the callback must be thread safe.
 */
class MaterialLookupCallback
{
public:
	/// Material request structure.
	struct MaterialRequest
	{
		PxVec3		samplePosition; ///< test point position. This is filled by the emitter
		uint32_t	outMaterialID; ///< ID of the material at the hitpoint. This must be filled by the user
	};

	/**
	\brief Submit the meterial lookup request. This method is called by the emitter and implemented by the user
	\param requestCount [IN] number of requests
	\param reqList [IN/OUT] the pointer to the requests. samplePosition is read and materialID is written to by the user.
	*/
	virtual void requestMaterialLookups(uint32_t requestCount, MaterialRequest* reqList) = 0;

	virtual ~MaterialLookupCallback() {}
};

///Ground Emitter actor. Uses raycasts against ground to spawn particles
class GroundEmitterActor : public Actor
{
protected:
	virtual ~GroundEmitterActor() {}

public:
	/**
	\brief Returns the asset the instance has been created from.
	*/
	virtual GroundEmitterAsset*   	getEmitterAsset() const = 0;

	/**
	\brief Returns the pose of the emitter
	*/
	virtual const PxMat44			getPose() const = 0;
	/**
	\brief Sets the pose of the emitter
	*/
	virtual void				    setPose(const PxMat44& pos) = 0;

	/** \brief Set the material lookup callback method that replaces raycasts */
	virtual void					setMaterialLookupCallback(MaterialLookupCallback*) = 0;
	
	/** \brief Get the material lookup callback method that replaces raycasts */
	virtual MaterialLookupCallback* getMaterialLookupCallback() const = 0;

	/**
	\brief Attaches the emitter to an actor
	PxActor pointer can be NULL to detach existing actor
	*/
	virtual void					setAttachActor(PxActor*) = 0;

	/**
	\brief sets the relative position of the emitter in the space of the actor to which it is attached
	*/
	virtual void					setAttachRelativePosition(const PxVec3& pos) = 0;

	/**\brief PhysX SDK 3.X.  Retrieves the actor, to which the emitter is attached. NULL is returned for an unattached emitter. */
	virtual const PxActor* 			getAttachActor() const = 0;

	/** \brief Retrieves the relative position of the emitter in the space of the actor to which it is attached. */
	virtual const PxVec3& 			getAttachRelativePosition() const = 0;

	/* Override some asset settings at run time */

	///Sets the range from which the density of particles within the volume is randomly chosen
	virtual void					setDensity(const float&) = 0;
	
	///Sets the radius. The ground emitter actor will create objects within a circle of size 'radius'.
	virtual void                    setRadius(float) = 0;
	
	///Sets The maximum raycasts number per frame.
	virtual void                    setMaxRaycastsPerFrame(uint32_t) = 0;
	
	///Sets the height from which the ground emitter will cast rays at terrain/objects opposite of the 'upDirection'.
	virtual void                    setRaycastHeight(float) = 0;
	
	/**
	\brief Sets the height above the ground to emit particles.
	 If greater than 0, the ground emitter will refresh a disc above the player's position rather than
	 refreshing a circle around the player's position.

	*/
	virtual void                    setSpawnHeight(float) = 0;

	///Gets the range from which the density of particles within the volume is randomly chosen
	virtual const float & 			getDensity() const = 0;
	
	///Gets the radius. The ground emitter actor will create objects within a circle of size 'radius'.
	virtual float					getRadius() const = 0;
	
	///Gets The maximum raycasts number per frame.
	virtual uint32_t				getMaxRaycastsPerFrame() const = 0;
	
	///Gets the height from which the ground emitter will cast rays at terrain/objects opposite of the 'upDirection'.
	virtual float					getRaycastHeight() const = 0;
	
	/**
	\brief Gets the height above the ground to emit particles.
	 If greater than 0, the ground emitter will refresh a disc above the player's position rather than
	 refreshing a circle around the player's position.
	*/
	virtual float					getSpawnHeight() const = 0;

	///Sets collision groups used to cast rays
	virtual void					setRaycastCollisionGroups(uint32_t) = 0;

	///PHYSX SDK 3.X.  Sets collision groups mask.
	virtual void					setRaycastCollisionGroupsMask(physx::PxFilterData*) = 0;


	///Gets collision groups used to cast rays
	virtual uint32_t				getRaycastCollisionGroups() const = 0;

	///Emitted particles are injected to specified render volume on initial frame.
	///Set to NULL to clear the preferred volume.
	virtual void					setPreferredRenderVolume(RenderVolume* volume) = 0;
};


PX_POP_PACK

}
} // end namespace nvidia

#endif // GROUND_EMITTER_ACTOR_H
