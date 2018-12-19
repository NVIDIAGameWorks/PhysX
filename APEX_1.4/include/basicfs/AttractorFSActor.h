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



#ifndef ATTRACTOR_FSACTOR_H
#define ATTRACTOR_FSACTOR_H

#include "Apex.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

class BasicFSAsset;


/**
 \brief AttractorFS Actor class
 */
class AttractorFSActor : public Actor, public Renderable
{
protected:
	virtual ~AttractorFSActor() {}

public:
	/**
	\brief Returns the asset the instance has been created from.
	*/
	virtual BasicFSAsset* 		getAttractorFSAsset() const = 0;

	/**
	\brief Gets the current position of the actor
	*/
	virtual PxVec3				getCurrentPosition() const = 0;
	
	/**
	\brief Sets the current position of the actor
	*/
	virtual void				setCurrentPosition(const PxVec3& pos) = 0;

	/**
	\brief Sets the attracting radius of the field
	*/
	virtual void				setFieldRadius(float) = 0;

	/**
	\brief Sets strength of the constant part of attracting field
	*/
	virtual void				setConstFieldStrength(float) = 0;

	/**
	\brief Sets strength coefficient for variable part of attracting field
	*/
	virtual void				setVariableFieldStrength(float) = 0;

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

#endif // ATTRACTOR_FSACTOR_H
