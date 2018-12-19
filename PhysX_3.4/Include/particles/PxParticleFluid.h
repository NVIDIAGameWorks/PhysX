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


#ifndef PX_PHYSICS_NX_PARTICLEFLUID
#define PX_PHYSICS_NX_PARTICLEFLUID
/** \addtogroup particles
  @{
*/
#include "particles/PxParticleFluidReadData.h"
#include "particles/PxParticleBase.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief The particle fluid class represents the main module for particle based fluid simulation. (deprecated)
SPH (Smoothed Particle Hydrodynamics) is used to animate the particles.  This class inherits the properties 
of the PxParticleBase class and adds particle-particle interactions. 

There are two kinds of particle interaction forces which govern the behaviour of the fluid:
<ol>
<li>
	Pressure forces: These forces result from particle densities higher than the
	"rest density" of the fluid.  The rest density is given by specifying the inter-particle
	distance at which the fluid is in its relaxed state.  Particles which are closer than
	the rest spacing are pushed away from each other.
<li>
	Viscosity forces:  These forces act on neighboring particles depending on the difference
	of their velocities.  Particles drag other particles with them which is used to simulate the
	viscous behaviour of the fluid.
</ol>

For a good introduction to SPH fluid simulation,
see http://www.matthiasmueller.info/publications/sca03.pdf

\deprecated The PhysX particle feature has been deprecated in PhysX version 3.4

@see PxParticleBase, PxParticleFluidReadData, PxPhysics.createParticleFluid
*/
class PX_DEPRECATED PxParticleFluid : public PxParticleBase
{

public:

/************************************************************************************************/

/** @name Particle Access and Manipulation
*/
//@{
	
	/**
	\brief Locks the particle data and provides the data descriptor for accessing the particles including fluid particle densities.
	\note Only PxDataAccessFlag::eREADABLE and PxDataAccessFlag::eDEVICE are supported, PxDataAccessFlag::eWRITABLE will be ignored.
	@see PxParticleFluidReadData
	@see PxParticleBase::lockParticleReadData()
	*/
	virtual		PxParticleFluidReadData*		lockParticleFluidReadData(PxDataAccessFlags flags)	= 0;

	/**
	\brief Locks the particle data and provides the data descriptor for accessing the particles including fluid particle densities.
	\note This is the same as calling lockParticleFluidReadData(PxDataAccessFlag::eREADABLE).
	@see PxParticleFluidReadData
	@see PxParticleBase::lockParticleReadData()
	*/
	virtual		PxParticleFluidReadData*		lockParticleFluidReadData()	= 0;

//@}
/************************************************************************************************/


/** @name Particle Fluid Parameters
*/
//@{

	/**
	\brief Returns the fluid stiffness.

	\return The fluid stiffness.
	*/
	virtual		PxReal							getStiffness()											const	= 0;

	/**
	\brief Sets the fluid stiffness (must be positive).

	\param stiffness The new fluid stiffness.
	*/
	virtual		void 							setStiffness(PxReal stiffness)									= 0;

	/**
	\brief Returns the fluid viscosity.

	\return The viscosity  of the fluid.
	*/
	virtual		PxReal							getViscosity()											const	= 0;

	/**
	\brief Sets the fluid viscosity (must be positive).

	\param viscosity The new viscosity of the fluid.
	*/
	virtual		void 							setViscosity(PxReal viscosity)									= 0;

//@}
/************************************************************************************************/

/** @name Particle Fluid Property Read Back
*/
//@{

	/**
	\brief Returns the typical distance of particles in the relaxed state of the fluid.

	\return Rest particle distance.
	*/
	virtual		PxReal							getRestParticleDistance()								const	= 0;

//@}
/************************************************************************************************/

/** @name Particle Fluid Parameters
*/
//@{

	/**
	\brief Sets the typical distance of particles in the relaxed state of the fluid.

	\param restParticleDistance The new restParticleDistance of the fluid.
	*/
	virtual		void							setRestParticleDistance(PxReal restParticleDistance)			= 0;

//@}
/************************************************************************************************/

	virtual		const char*						getConcreteTypeName() const { return "PxParticleFluid"; }

protected:
	PX_INLINE									PxParticleFluid(PxType concreteType, PxBaseFlags baseFlags) : PxParticleBase(concreteType, baseFlags) {}
	PX_INLINE									PxParticleFluid(PxBaseFlags baseFlags) : PxParticleBase(baseFlags) {}
	virtual										~PxParticleFluid() {}
	virtual		bool							isKindOf(const char* name) const { return !::strcmp("PxParticleFluid", name) || PxParticleBase::isKindOf(name);  }
};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
