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



#ifndef VORTEX_FSACTOR_H
#define VORTEX_FSACTOR_H

#include "Apex.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

class BasicFSAsset;


/**
 \brief VortexFS Actor class
 */
class VortexFSActor : public Actor, public Renderable
{
protected:
	virtual ~VortexFSActor() {}

public:

	/**
	\brief Returns the asset the instance has been created from.
	*/
	virtual BasicFSAsset* 		getVortexFSAsset() const = 0;

	/**
	\brief Gets the current pose of the actor
	*/
	virtual PxMat44				getCurrentPose() const = 0;
	
	/**
	\brief Sets the current pose of the actor
	*/
	virtual void				setCurrentPose(const PxTransform& pose) = 0;
	
	/**
	\brief Gets the current position of the actor
	*/
	virtual PxVec3				getCurrentPosition() const = 0;
	
	/**
	\brief Sets the current position of the actor
	*/
	virtual void				setCurrentPosition(const PxVec3& pos) = 0;

	/**
	\brief Sets the axis of the capsule in local coordinate system
	*/
	virtual void				setAxis(const PxVec3&) = 0;
	
	/**
	\brief Sets the height of the capsule
	*/
	virtual void				setHeight(float) = 0;
	
	/**
	\brief Sets the bottom radius of the capsule
	*/
	virtual void				setBottomRadius(float) = 0;
	
	/**
	\brief Sets the top radius of the capsule
	*/
	virtual void				setTopRadius(float) = 0;

	/**
	\brief Sets the bottom spherical force of the capsule
	*/
	virtual void				setBottomSphericalForce(bool) = 0;
	
	/**
	\brief Sets the top spherical force of the capsule
	*/
	virtual void				setTopSphericalForce(bool) = 0;

	/**
	\brief Sets strength of the rotational part of vortex field
	*/
	virtual void				setRotationalStrength(float) = 0;
	
	/**
	\brief Sets strength of the radial part of vortex field
	*/
	virtual void				setRadialStrength(float) = 0;
	
	/**
	\brief Sets strength of the lifting part of vortex field
	*/
	virtual void				setLiftStrength(float) = 0;

	/**
	\brief Enable/Disable the field simulation
	*/
	virtual void				setEnabled(bool isEnabled) = 0;
	

	/**
	\brief Sets the uniform overall object scale
	*/
	virtual void				setCurrentScale(float scale) = 0;

	/**
	\brief Retrieves the uniform overall object scale
	*/
	virtual float				getCurrentScale(void) const = 0;

};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // VORTEX_FSACTOR_H
