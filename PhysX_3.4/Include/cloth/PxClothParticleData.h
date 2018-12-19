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


#ifndef PX_PHYSICS_NX_CLOTH_READ_DATA
#define PX_PHYSICS_NX_CLOTH_READ_DATA

#include "PxPhysXConfig.h"

#if PX_USE_CLOTH_API

/** \addtogroup cloth
  @{
*/

#include "PxLockedData.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

struct PxClothParticle;

/**
\brief Data layout descriptor for reading cloth data from the SDK.
\deprecated The PhysX cloth feature has been deprecated in PhysX version 3.4.1
@see PxCloth.lockParticleData()
*/
class PX_DEPRECATED PxClothParticleData : public PxLockedData
{
public:
	/**
	\brief Particle position and mass data.
	@see PxCloth.getNbParticles()
	*/
	PxClothParticle* particles;

	/**
	\brief Particle position and mass data from the second last iteration.
	\details Can be used together with #particles and #PxCloth.getPreviousTimeStep() to compute particle velocities.
	*/
	PxClothParticle* previousParticles;
};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */

#endif // PX_USE_CLOTH_API

#endif
