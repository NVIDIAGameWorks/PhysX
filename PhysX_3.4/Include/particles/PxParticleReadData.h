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


#ifndef PX_PARTICLESYSTEM_NXPARTICLEREADDATA
#define PX_PARTICLESYSTEM_NXPARTICLEREADDATA
/** \addtogroup particles
  @{
*/

#include "foundation/PxStrideIterator.h"
#include "foundation/PxFlags.h"
#include "PxLockedData.h"
#include "PxParticleFlag.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
Flags to configure PxParticleBase simulation output that can be read by the application. (deprecated)
Disabling unneeded buffers saves memory and improves performance.

\deprecated The PhysX particle feature has been deprecated in PhysX version 3.4
*/
struct PX_DEPRECATED PxParticleReadDataFlag
	{
	enum Enum
		{
			/**
			Enables reading particle positions from the SDK.
			@see PxParticleReadData.positionBuffer
			*/
			ePOSITION_BUFFER			= (1<<0),

			/**
			Enables reading particle velocities from the SDK.
			@see PxParticleReadData.velocityBuffer
			*/
			eVELOCITY_BUFFER			= (1<<1),

			/**
			Enables reading per particle rest offsets from the SDK.
			Per particle rest offsets are never changed by the simulation.
			This option may only be used on particle systems that have 
			PxParticleBaseFlag.ePER_PARTICLE_REST_OFFSET enabled.
			@see PxParticleBaseFlag.ePER_PARTICLE_REST_OFFSET
			*/
			eREST_OFFSET_BUFFER			= (1<<2),

			/**
			Enables reading particle flags from the SDK.
			@see PxParticleReadData.flagsBuffer
			*/
			eFLAGS_BUFFER				= (1<<3),

			/**
			Enables reading particle collision normals from the SDK.
			@see PxParticleReadData.collisionNormalBuffer
			*/
			eCOLLISION_NORMAL_BUFFER	= (1<<4),

			/**
			Enables reading particle collision velocities from the SDK.
			@see PxParticleReadData.collisionVelocity
			*/
			eCOLLISION_VELOCITY_BUFFER	= (1<<5),

			/**
			Enables reading particle densities from the SDK. (PxParticleFluid only)
			@see PxParticleFluidReadData.densityBuffer
			*/
			eDENSITY_BUFFER				= (1<<6)
		};
	};

/**
\brief collection of set bits defined in PxParticleReadDataFlag. (deprecated)

\deprecated The PhysX particle feature has been deprecated in PhysX version 3.4

@see PxParticleReadDataFlag
*/
typedef PX_DEPRECATED PxFlags<PxParticleReadDataFlag::Enum, PxU16> PxParticleReadDataFlags;
PX_FLAGS_OPERATORS(PxParticleReadDataFlag::Enum,PxU16)

/**
\brief collection of set bits defined in PxParticleFlag. (deprecated)

\deprecated The PhysX particle feature has been deprecated in PhysX version 3.4

@see PxParticleFlag
*/
typedef PX_DEPRECATED PxFlags<PxParticleFlag::Enum, PxU16> PxParticleFlags;
PX_FLAGS_OPERATORS(PxParticleFlag::Enum,PxU16)

/**
\brief Data layout descriptor for reading particle data from the SDK. (deprecated)

PxParticleReadData is used to retrieve information about simulated particles. It can be accessed by calling PxParticleBase.lockParticleReadData().

Each particle is described by its position, velocity, a set of (PxParticleFlag) flags and information on collisions (collision normal).
The particle buffers are sparse, i.e. occupied particle indices will have PxParticleFlag.eVALID set in the corresponding entry of 
PxParticleReadData.flagsBuffer. Alternatively valid particles can be identified with the bitmap PxParticleReadData.validParticleBitmap.
If (and only if) the index range of valid particles PxParticleReadData.validParticleRange is greater 0, i.e. any particles are present, 
data can be read from the particle buffers. Additionally individual particle buffers can only be read if the corresponding 
PxParticleReadDataFlag in particleReadDataFlags is set.

The particle data buffer are defined in the range [0, PxParticleReadData.validParticleRange-1].
The bitmap words are defined in the range [0, (PxParticleReadData.validParticleRange-1) >> 5].

\deprecated The PhysX particle feature has been deprecated in PhysX version 3.4

@see PxParticleBase::lockParticleReadData()
*/
class PX_DEPRECATED PxParticleReadData : public PxLockedData
	{
	public:

	/**
	\brief Number of particles (only including particles with PxParticleFlag.eVALID set). 
	*/
	PxU32										nbValidParticles;

	/**
	\brief Index after the last valid particle (PxParticleFlag.eVALID set). Its 0 if there are no valid particles. 
	*/
	PxU32										validParticleRange;

	/**
	\brief Bitmap marking valid particle indices. The bitmap is defined between [0, (PxParticleReadData.validParticleRange-1) >> 5].
	\note Might be NULL if PxParticleReadData.validParticleRange == 0. 
	*/
	const PxU32*								validParticleBitmap;

	/**
	\brief Particle position data.
	*/
	PxStrideIterator<const PxVec3>				positionBuffer;
	
	/**
	\brief Particle velocity data.
	*/
	PxStrideIterator<const PxVec3>				velocityBuffer;

	/**
	\brief Particle rest offset data.
	*/
	PxStrideIterator<const PxF32>				restOffsetBuffer;

	/**
	\brief Particle flags.
	*/
	PxStrideIterator<const PxParticleFlags>		flagsBuffer;

	/**
	\brief Collision normals of colliding particles.
	The collision normal buffer is only guaranteed to be valid after the particle 
	system has been simulated. Otherwise collisionNormalBuffer.ptr() is NULL. This also 
	applies to particle systems that are not assigned to a scene.
	*/
	PxStrideIterator<const PxVec3>				collisionNormalBuffer;
	
	/**
	\brief Velocities of particles relative to shapes they collide with.
	The collision velocity buffer is only guaranteed to be valid after the particle 
	system has been simulated. Otherwise collisionVelocityBuffer.ptr() is NULL. This also 
	applies to particle systems that are not assigned to a scene.
	The collision velocity is identical to the particle velocity if the particle is not colliding.
	*/
	PxStrideIterator<const PxVec3>				collisionVelocityBuffer;

	/**
	\brief Returns PxDataAccessFlag::eREADABLE, since PxParticleReadData is read only data
	@see PxLockedData
	*/
    virtual PxDataAccessFlags getDataAccessFlags() = 0;

	/**
	\brief Unlocks the data.
	@see PxLockedData
	*/
    virtual void unlock() = 0;

	/**
	\brief virtual destructor
	*/
	virtual ~PxParticleReadData() {}

	};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
