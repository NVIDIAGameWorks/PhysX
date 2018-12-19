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


#ifndef PX_PARTICLE_FLAG
#define PX_PARTICLE_FLAG

#include "foundation/PxPreprocessor.h"

/** \addtogroup particles
  @{
*/

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
Particle flags are used for additional information on the particles. (deprecated)

\deprecated The PhysX particle feature has been deprecated in PhysX version 3.4
*/
struct PX_DEPRECATED PxParticleFlag
	{
	enum Enum
		{
			/**
			\brief Marks a valid particle. The particle data corresponding to these particle flags is valid, i.e. defines a particle, when set.
			Particles that are not marked with PxParticleFlag::eVALID are ignored during simulation.
			
			Application read only.
			*/
			eVALID								= (1<<0),

			/**
			\brief Marks a particle that has collided with a static actor shape.

			Application read only.
			*/
			eCOLLISION_WITH_STATIC				= (1<<1),	

			/**
			\brief Marks a particle that has collided with a dynamic actor shape.

			Application read only.
			*/
			eCOLLISION_WITH_DYNAMIC				= (1<<2),

			/**
			\brief Marks a particle that has collided with a shape that has been flagged as a drain (See PxShapeFlag.ePARTICLE_DRAIN).
			
			Application read only.
			@see PxShapeFlag.ePARTICLE_DRAIN
			*/
			eCOLLISION_WITH_DRAIN				= (1<<3),

			/**
			\brief Marks a particle that had to be ignored for simulation, because the spatial data structure ran out of resources.

			Application read only.
			*/
			eSPATIAL_DATA_STRUCTURE_OVERFLOW	= (1<<4)
		};
	};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
