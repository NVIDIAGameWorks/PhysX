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


#ifndef PX_PARTICLESYSTEM_NXPARTICLEFLUIDREADDATA
#define PX_PARTICLESYSTEM_NXPARTICLEFLUIDREADDATA
/** \addtogroup particles
  @{
*/

#include "particles/PxParticleReadData.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief Data layout descriptor for reading fluid particle data from the SDK. (deprecated)

Additionally to PxParticleReadData, the density can be read from the SDK.

\deprecated The PhysX particle feature has been deprecated in PhysX version 3.4

@see PxParticleReadData PxParticleFluid.lockParticleFluidReadData()
*/
class PX_DEPRECATED PxParticleFluidReadData : public PxParticleReadData
	{
	public:

	/**
	\brief Particle density data.
	
	The density depends on how close particles are to each other. The density values are normalized such that:
	<ol>
	<li>
	Particles which have no neighbors (no particles closer than restParticleDistance * 2) 
	will have a density of zero.
	<li>
	Particles which are at rest density (distances corresponding to restParticleDistance in the mean) 
	will have a density of one.
	</ol>
	 
	The density buffer is only guaranteed to be valid after the particle 
	fluid has been simulated. Otherwise densityBuffer.ptr() is NULL. This also 
	applies to particle fluids that are not assigned to a scene.
	*/
	PxStrideIterator<const PxF32> densityBuffer;

	/**
	\brief virtual destructor
	*/
	virtual ~PxParticleFluidReadData() {}

	};

#if !PX_DOXYGEN
} // namespace physx
#endif

/** @} */
#endif
