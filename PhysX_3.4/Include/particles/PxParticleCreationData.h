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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#ifndef PX_PARTICLESYSTEM_NXPARTICLECREATIONDATA
#define PX_PARTICLESYSTEM_NXPARTICLECREATIONDATA
/** \addtogroup particles
@{
*/

#include "PxPhysXConfig.h"
#include "foundation/PxVec3.h"
#include "foundation/PxStrideIterator.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief Descriptor-like user-side class describing buffers for particle creation. (deprecated)

PxParticleCreationData is used to create particles within the SDK. The SDK copies the particle data referenced by PxParticleCreationData, it
may therefore be deallocated right after the creation call returned.

\deprecated The PhysX particle feature has been deprecated in PhysX version 3.4

@see PxParticleBase::createParticles()
*/
class PX_DEPRECATED PxParticleCreationData
{
public:

	/**
	\brief The number of particles stored in the buffer. 
	*/
	PxU32									numParticles;

	/**
	\brief Particle index data.

	When creating particles, providing the particle indices is mandatory.
	*/
	PxStrideIterator<const PxU32>			indexBuffer;

	/**
	\brief Particle position data.

	When creating particles, providing the particle positions is mandatory.
	*/
	PxStrideIterator<const PxVec3>			positionBuffer;

	/**
	\brief Particle velocity data.

	Providing velocity data is optional.
	*/
	PxStrideIterator<const PxVec3>			velocityBuffer;

	/**
	\brief Particle rest offset data. 

	Values need to be in the range [0.0f, restOffset].
	If PxParticleBaseFlag.ePER_PARTICLE_REST_OFFSET is set, providing per particle rest offset data is mandatory.  
	@see PxParticleBaseFlag.ePER_PARTICLE_REST_OFFSET.
	*/
	PxStrideIterator<const PxF32>			restOffsetBuffer;

	/**
	\brief Particle flags.
	
	PxParticleFlag.eVALID, PxParticleFlag.eCOLLISION_WITH_STATIC, PxParticleFlag.eCOLLISION_WITH_DYNAMIC,
	PxParticleFlag.eCOLLISION_WITH_DRAIN, PxParticleFlag.eSPATIAL_DATA_STRUCTURE_OVERFLOW are all flags that 
	can't be set on particle creation. They are written by the SDK exclusively.
	
	Providing flag data is optional.
	@see PxParticleFlag
	*/
	PxStrideIterator<const PxU32>			flagBuffer;

	/**
	\brief (Re)sets the structure to the default.	
	*/
	PX_INLINE void setToDefault();
	
	/**
	\brief Returns true if the current settings are valid
	*/
	PX_INLINE bool isValid() const;

	/**
	\brief Constructor sets to default.
	*/
	PX_INLINE	PxParticleCreationData();
};

PX_INLINE PxParticleCreationData::PxParticleCreationData()
{
	indexBuffer					= PxStrideIterator<const PxU32>();
	positionBuffer				= PxStrideIterator<const PxVec3>();
	velocityBuffer				= PxStrideIterator<const PxVec3>();
	restOffsetBuffer			= PxStrideIterator<const PxF32>();
	flagBuffer					= PxStrideIterator<const PxU32>();
}

PX_INLINE void PxParticleCreationData::setToDefault()
{
	*this = PxParticleCreationData();
}

PX_INLINE bool PxParticleCreationData::isValid() const
{
	if (numParticles > 0 && !(indexBuffer.ptr() && positionBuffer.ptr())) return false;
	return true;
}

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
