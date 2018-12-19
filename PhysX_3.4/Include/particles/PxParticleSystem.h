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


#ifndef PX_PHYSICS_PX_PARTICLESYSTEM
#define PX_PHYSICS_PX_PARTICLESYSTEM
/** \addtogroup particles
  @{
*/

#include "particles/PxParticleBase.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief The particle system class represents the main module for particle based simulation. (deprecated)

This class inherits the properties of the PxParticleBase class.

The particle system class manages a set of particles.  Particles can be created, released and updated directly through the API.
When a particle is created the user gets an id for it which can be used to address the particle until it is released again.

Particles collide with static and dynamic shapes.  They are also affected by the scene gravity and a user force, 
as well as global velocity damping.  When a particle collides, a particle flag is raised corresponding to the type of 
actor, static or dynamic, it collided with.  Additionally a shape can be flagged as a drain (See PxShapeFlag), in order to get a corresponding 
particle flag raised when a collision occurs.  This information can be used to delete particles.

The particles of a particle system don't collide with each other.  In order to simulate particle-particle interactions use the 
subclass PxParticleFluid.

\deprecated The PhysX particle feature has been deprecated in PhysX version 3.4

@see PxParticleBase, PxParticleReadData, PxPhysics.createParticleSystem
*/
class PX_DEPRECATED PxParticleSystem : public PxParticleBase
{
public:

		virtual		const char*					getConcreteTypeName() const { return "PxParticleSystem"; }

/************************************************************************************************/

protected:
	PX_INLINE									PxParticleSystem(PxType concreteType, PxBaseFlags baseFlags) : PxParticleBase(concreteType, baseFlags) {}
	PX_INLINE									PxParticleSystem(PxBaseFlags baseFlags) : PxParticleBase(baseFlags) {}
	virtual										~PxParticleSystem() {}
	virtual			bool						isKindOf(const char* name) const { return !::strcmp("PxParticleSystem", name) || PxParticleBase::isKindOf(name);  }

};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
