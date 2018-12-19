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


#ifndef PX_PARTICLE_BASE_FLAG
#define PX_PARTICLE_BASE_FLAG
/** \addtogroup particles
  @{
*/

#include "foundation/PxFlags.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief ParticleBase flags (deprecated)

\deprecated The PhysX particle feature has been deprecated in PhysX version 3.4
*/
struct PX_DEPRECATED PxParticleBaseFlag
{
	enum Enum
	{
		/**
		\brief Enable/disable two way collision of particles with the rigid body scene.
		In either case, particles are influenced by colliding rigid bodies.
		If eCOLLISION_TWOWAY is not set, rigid bodies are not influenced by 
		colliding particles. Use particleMass to
		control the strength of the feedback force on rigid bodies.
		
		\note Switching this flag while the particle system is part of a scene might have a negative impact on performance.
		*/
		eCOLLISION_TWOWAY					= (1<<0),

		/**
		\brief Enable/disable collision of particles with dynamic actors.
		The flag can be turned off as a hint to the sdk to save memory space and 
		execution time. In principle any collisions can be turned off using filters
		but without or reduced memory and performance benefits.

		\note Switching this flag while the particle system is part of a scene might have a negative impact on performance.
		*/
		eCOLLISION_WITH_DYNAMIC_ACTORS		= (1<<1),

		/**
		\brief Enable/disable execution of particle simulation.
		*/
		eENABLED							= (1<<2),

		/**
		\brief Defines whether the particles of this particle system should be projected to a plane.
		This can be used to build 2D applications, for instance. The projection
		plane is defined by the parameter projectionPlaneNormal and projectionPlaneDistance.
		*/
		ePROJECT_TO_PLANE					= (1<<3),

		/**
		\brief Enable/disable per particle rest offsets.
		Per particle rest offsets can be used to support particles having different sizes with 
		respect to collision.
		
		\note This configuration cannot be changed after the particle system was created.
		*/
		ePER_PARTICLE_REST_OFFSET			= (1<<4),

		/**
		\brief Ename/disable per particle collision caches.
		Per particle collision caches improve collision detection performance at the cost of increased 
		memory usage.

		\note Switching this flag while the particle system is part of a scene might have a negative impact on performance.
		*/
		ePER_PARTICLE_COLLISION_CACHE_HINT	= (1<<5),

		/**
		\brief Enable/disable GPU acceleration. 
		Enabling GPU acceleration might fail. In this case the eGPU flag is switched off. 

		\note Switching this flag while the particle system is part of a scene might have a negative impact on performance.
		
		@see PxScene.removeActor() PxScene.addActor() PxParticleGpu
		*/
		eGPU								= (1<<6)
	};
};


/**
\brief collection of set bits defined in PxParticleBaseFlag. (deprecated)

\deprecated The PhysX particle feature has been deprecated in PhysX version 3.4

@see PxParticleBaseFlag
*/
typedef PX_DEPRECATED PxFlags<PxParticleBaseFlag::Enum, PxU16> PxParticleBaseFlags;
PX_FLAGS_OPERATORS(PxParticleBaseFlag::Enum,PxU16)

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
