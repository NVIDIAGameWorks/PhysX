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


// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#ifndef PX_PHYSICS_NX_CLOTH_TYPES
#define PX_PHYSICS_NX_CLOTH_TYPES
/** \addtogroup cloth
  @{
*/

#include "PxPhysXConfig.h"
#include "PxFlags.h"

#include "PxVec3.h"

#if PX_DOXYGEN == 0
namespace nvidia
{
#endif

/**
   \brief flag for behaviors of the cloth solver
   \details Defines flags to turn on/off features of the cloth solver.
   The flag can be set during the cloth object construction (\see PxPhysics.createCloth() ),
   or individually after the cloth has been created (\see PxCloth.setClothFlag() ).
 */
struct PxClothFlag
{
	enum Enum
	{
		eGPU			 = (1<<0), //!< turn on/off gpu based solver
		eSWEPT_CONTACT	 = (1<<1), //!< use swept contact (continuous collision)
		eSCENE_COLLISION = (1<<2), //!< collide against rigid body shapes in scene
		eCOUNT			 = 3	   // internal use only
	};
};

typedef PxFlags<PxClothFlag::Enum,uint16_t> PxClothFlags;
PX_FLAGS_OPERATORS(PxClothFlag::Enum, uint16_t)

/**
   \brief Per particle data for cloth.
   \details Defines position of the cloth particle as well as inverse mass.
   When inverse mass is set to 0, the particle gets fully constrained
   to the position during simulation.
   \see PxPhysics.createCloth()
   \see PxCloth.setParticles()
*/
struct PxClothParticle
{
	PxVec3 pos;			//!< position of the particle (in cloth local space)
	float invWeight;	//!< inverse mass of the particle. If set to 0, the particle is fully constrained.

	/**
	\brief Default constructor, performs no initialization.
	*/
	PxClothParticle() {}
	PxClothParticle(const PxVec3& pos_, float invWeight_) 
		: pos(pos_), invWeight(invWeight_){}
};

/**
\brief Constraints for cloth particle motion.
\details Defines a spherical volume to which the motion of a particle should be constrained.
@see PxCloth.setMotionConstraints()
*/
struct PxClothParticleMotionConstraint
{
	PxVec3 pos;			//!< Center of the motion constraint sphere (in cloth local space)
	float radius;		//!< Maximum distance the particle can move away from the sphere center.

	/**
	\brief Default constructor, performs no initialization.
	*/
	PxClothParticleMotionConstraint() {}
	PxClothParticleMotionConstraint(const PxVec3& p, float r) 
		: pos(p), radius(r){}
};

/**
\brief Separation constraints for cloth particle movement
\details Defines a spherical volume such that corresponding particles should stay outside.
@see PxCloth.setSeparationConstraints()
*/
struct PxClothParticleSeparationConstraint
{
	PxVec3 pos;			//!< Center of the constraint sphere (in cloth local space)
	float radius;		//!< Radius of the constraint sphere such that the particle stay outside of this sphere.

	/**
	\brief Default constructor, performs no initialization.
	*/
	PxClothParticleSeparationConstraint() {}
	PxClothParticleSeparationConstraint(const PxVec3& p, float r) 
		: pos(p), radius(r){}
};

#if PX_DOXYGEN == 0
} // namespace nvidia
#endif

/** @} */
#endif
