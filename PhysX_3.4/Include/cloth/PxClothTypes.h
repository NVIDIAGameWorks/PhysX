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


#ifndef PX_PHYSICS_NX_CLOTH_TYPES
#define PX_PHYSICS_NX_CLOTH_TYPES

/** \defgroup cloth cloth (deprecated)*/

/** \addtogroup cloth
  @{
*/

#include "PxPhysXConfig.h"
#include "foundation/PxFlags.h"

#include "foundation/PxVec3.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
   \brief flag for behaviors of the cloth solver
   \details Defines flags to turn on/off features of the cloth solver.
   The flag can be set during the cloth object construction (\see PxPhysics.createCloth() ),
   or individually after the cloth has been created (\see PxCloth.setClothFlag() ).
   
   \deprecated The PhysX cloth feature has been deprecated in PhysX version 3.4.1
 */
struct PX_DEPRECATED PxClothFlag
{
	enum Enum
	{
		eDEFAULT           = 0,      //!< default value
		eCUDA			   = (1<<0), //!< use CUDA to simulate cloth
		eGPU PX_DEPRECATED = eCUDA,  //!< \deprecated Deprecated, use eCUDA instead
		eSWEPT_CONTACT	   = (1<<2), //!< use swept contact (continuous collision)
		eSCENE_COLLISION   = (1<<3), //!< collide against rigid body shapes in scene
		eCOUNT			   = 4	     // internal use only
	};
};

typedef PxFlags<PxClothFlag::Enum,PxU16> PxClothFlags;
PX_FLAGS_OPERATORS(PxClothFlag::Enum, PxU16)

/**
   \brief Per particle data for cloth.
   \details Defines position of the cloth particle as well as inverse mass.
   When inverse mass is set to 0, the particle gets fully constrained
   to the position during simulation.
   \see PxPhysics.createCloth()
   \see PxCloth.setParticles()
   \deprecated The PhysX cloth feature has been deprecated in PhysX version 3.4.1
*/
struct PX_DEPRECATED PxClothParticle
{
	PxVec3 pos;			//!< position of the particle (in cloth local space)
	PxReal invWeight;	//!< inverse mass of the particle. If set to 0, the particle is fully constrained.

	/**
	\brief Default constructor, performs no initialization.
	*/
	PxClothParticle() {}
	PxClothParticle(const PxVec3& pos_, PxReal invWeight_) 
		: pos(pos_), invWeight(invWeight_){}
};

/**
\brief Constraints for cloth particle motion.
\details Defines a spherical volume to which the motion of a particle should be constrained.
\deprecated The PhysX cloth feature has been deprecated in PhysX version 3.4.1
@see PxCloth.setMotionConstraints()
*/
struct PX_DEPRECATED PxClothParticleMotionConstraint
{
	PxVec3 pos;			//!< Center of the motion constraint sphere (in cloth local space)
	PxReal radius;		//!< Maximum distance the particle can move away from the sphere center.

	/**
	\brief Default constructor, performs no initialization.
	*/
	PxClothParticleMotionConstraint() {}
	PxClothParticleMotionConstraint(const PxVec3& p, PxReal r) 
		: pos(p), radius(r){}
};

/**
\brief Separation constraints for cloth particle movement
\details Defines a spherical volume such that corresponding particles should stay outside.
\deprecated The PhysX cloth feature has been deprecated in PhysX version 3.4.1
@see PxCloth.setSeparationConstraints()
*/
struct PX_DEPRECATED PxClothParticleSeparationConstraint
{
	PxVec3 pos;			//!< Center of the constraint sphere (in cloth local space)
	PxReal radius;		//!< Radius of the constraint sphere such that the particle stay outside of this sphere.

	/**
	\brief Default constructor, performs no initialization.
	*/
	PxClothParticleSeparationConstraint() {}
	PxClothParticleSeparationConstraint(const PxVec3& p, PxReal r) 
		: pos(p), radius(r){}
};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
