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



#ifndef CLOTHING_VELOCITY_CALLBACK_H
#define CLOTHING_VELOCITY_CALLBACK_H

#include "ApexUsingNamespace.h"

namespace nvidia
{
namespace apex
{

/**
\brief container class for the velocity shader callback.
*/
class ClothingVelocityCallback
{
public:
	/**
	\brief This callback will be fired in Apex threads. It must not address any user data, just operate on the data.
	\param [in,out] velocities  The velocities of the cloth. These can be modified if necessary, but then the method needs to return true!
	\param [in] positions       The positions of the cloth. Must not be modified, only read.
	\param [in] numParticles    Size of the velocities and positions array.

	\return return true if the velocities have been altered, false if they just have been read
	*/
	virtual bool velocityShader(PxVec3* velocities, const PxVec3* positions, uint32_t numParticles) = 0;
};

} // namespace nvidia
} // namespace nvidia

#endif // CLOTHING_VELOCITY_CALLBACK_H
