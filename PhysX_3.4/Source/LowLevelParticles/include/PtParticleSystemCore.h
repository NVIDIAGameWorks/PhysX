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

#ifndef PT_PARTICLE_SYSTEM_CORE_H
#define PT_PARTICLE_SYSTEM_CORE_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "foundation/PxBounds3.h"
#include "foundation/PxStrideIterator.h"

#include "CmPhysXCommon.h"
#include "PtParticleSystemFlags.h"
#include "particles/PxParticleReadData.h"
#include "CmBitMap.h"

namespace physx
{

class PxParticleCreationData;
class PxSerializationContext;

namespace Pt
{

typedef size_t ShapeHandle;
typedef size_t BodyHandle;

/*!
Particle system / particle fluid parameter. API + internal.
*/
struct ParticleSystemParameter
{
	//= ATTENTION! =====================================================================================
	// Changing the data layout of this class breaks the binary serialization format.  See comments for
	// PX_BINARY_SERIAL_VERSION.  If a modification is required, please adjust the getBinaryMetaData
	// function.  If the modification is made on a custom branch, please change PX_BINARY_SERIAL_VERSION
	// accordingly.
	//==================================================================================================

	ParticleSystemParameter(const PxEMPTY) : particleReadDataFlags(PxEmpty)
	{
	}
	ParticleSystemParameter()
	{
	}

	PxReal restParticleDistance;
	PxReal kernelRadiusMultiplier;
	PxReal viscosity;
	PxReal surfaceTension;
	PxReal fadeInTime;
	PxU32 flags;
	PxU32 packetSizeMultiplierLog2;
	PxReal restitution;
	PxReal dynamicFriction;
	PxReal staticFriction;
	PxReal restDensity;
	PxReal damping;
	PxReal stiffness;
	PxReal maxMotionDistance;
	PxReal restOffset;
	PxReal contactOffset;
	PxPlane projectionPlane;
	PxParticleReadDataFlags particleReadDataFlags;
	PxU32 noiseCounter; // Needed for deterministic temporal noise
};

/*!
Descriptor for particle retrieval
*/
struct ParticleSystemStateDataDesc
{
	PxU32 maxParticles;
	PxU32 numParticles;
	PxU32 validParticleRange;
	const Cm::BitMap* bitMap;
	PxStrideIterator<const PxVec3> positions;
	PxStrideIterator<const PxVec3> velocities;
	PxStrideIterator<const ParticleFlags> flags;
	PxStrideIterator<const PxF32> restOffsets;
};

/*!
Descriptor for particle retrieval: TODO
*/
struct ParticleSystemSimDataDesc
{
	PxStrideIterator<const PxF32> densities;            //! Particle densities
	PxStrideIterator<const PxVec3> collisionNormals;    //! Particle collision normals
	PxStrideIterator<const PxVec3> collisionVelocities; //! Particle collision velocities
	PxStrideIterator<const PxVec3> twoWayImpluses;      //! collision impulses(for two way interaction)
	PxStrideIterator<BodyHandle> twoWayBodies;          //! Colliding rigid bodies each particle (zero if no collision)
};

class ParticleSystemState
{
  public:
	virtual ~ParticleSystemState()
	{
	}
	virtual bool addParticlesV(const PxParticleCreationData& creationData) = 0;
	virtual void removeParticlesV(PxU32 count, const PxStrideIterator<const PxU32>& indices) = 0;
	virtual void removeParticlesV() = 0;

	/**
	If fullState is set, the entire particle state is read, ignoring PxParticleReadDataFlags
	*/
	virtual void getParticlesV(ParticleSystemStateDataDesc& particles, bool fullState, bool devicePtr) const = 0;
	virtual void setPositionsV(PxU32 numParticles, const PxStrideIterator<const PxU32>& indices,
	                           const PxStrideIterator<const PxVec3>& positions) = 0;
	virtual void setVelocitiesV(PxU32 numParticles, const PxStrideIterator<const PxU32>& indices,
	                            const PxStrideIterator<const PxVec3>& velocities) = 0;
	virtual void setRestOffsetsV(PxU32 numParticles, const PxStrideIterator<const PxU32>& indices,
	                             const PxStrideIterator<const PxF32>& restOffsets) = 0;
	virtual void addDeltaVelocitiesV(const Cm::BitMap& bufferMap, const PxVec3* buffer, PxReal multiplier) = 0;

	virtual PxBounds3 getWorldBoundsV() const = 0;
	virtual PxU32 getMaxParticlesV() const = 0;
	virtual PxU32 getParticleCountV() const = 0;
};

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_PARTICLE_SYSTEM_CORE_H
