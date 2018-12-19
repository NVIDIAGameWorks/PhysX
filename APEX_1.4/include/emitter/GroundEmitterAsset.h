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



#ifndef GROUND_EMITTER_ASSET_H
#define GROUND_EMITTER_ASSET_H

#include "Apex.h"

namespace nvidia
{
namespace apex
{


PX_PUSH_PACK_DEFAULT

#define GROUND_EMITTER_AUTHORING_TYPE_NAME "GroundEmitterAsset"

///Ground emitter asset. Used to create Ground emitter actors with specific properties.
class GroundEmitterAsset : public Asset
{
protected:
	PX_INLINE GroundEmitterAsset() {}
	virtual ~GroundEmitterAsset() {}

public:

	///Gets the velocity range.	The ground emitter actor will create objects with a random velocity within the velocity range.
	virtual const PxVec3 & 			getVelocityLow() const = 0;
	
	///Gets the velocity range.	The ground emitter actor will create objects with a random velocity within the velocity range.
	virtual const PxVec3 & 			getVelocityHigh() const = 0;
	
	///Gets the lifetime range. The ground emitter actor will create objects with a random lifetime (in seconds) within the lifetime range.
	virtual const float & 			getLifetimeLow() const = 0;
	
	///Gets the lifetime range. The ground emitter actor will create objects with a random lifetime (in seconds) within the lifetime range.
	virtual const float & 			getLifetimeHigh() const = 0;

	///Gets the radius.  The ground emitter actor will create objects within a circle of size 'radius'.
	virtual float                  	getRadius() const = 0;
	
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
	
	/// Gets collision groups name used to cast rays
	virtual const char* 			getRaycastCollisionGroupMaskName() const = 0;
};

///Ground emitter authoring class. Used to create Ground emitter assets.
class GroundEmitterAssetAuthoring : public AssetAuthoring
{
protected:
	virtual ~GroundEmitterAssetAuthoring() {}
};


PX_POP_PACK

}
} // end namespace nvidia

#endif // GROUND_EMITTER_ASSET_H
