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

#ifndef PARTICLE_FACTORY_H
#define PARTICLE_FACTORY_H

//----------------------------------------------------------------------------//
#include "PxPhysicsAPI.h"
#include "PhysXSampleApplication.h"

//----------------------------------------------------------------------------//

struct ParticleData : public SampleAllocateable
{
	ParticleData() : maxParticles(0), numParticles(0) {}

	ParticleData(PxU32 _maxParticles)
	{
		initialize(_maxParticles);
	}

	void initialize(PxU32 _maxParticles)
	{
		maxParticles = _maxParticles;
		numParticles = 0;
		positions.resize(maxParticles);
		velocities.resize(maxParticles);
		restOffsets.resize(0);
	}

	SampleArray<PxVec3> positions;
	SampleArray<PxVec3> velocities;
	SampleArray<PxF32> restOffsets;
	PxU32	maxParticles;
	PxU32	numParticles;
};

//----------------------------------------------------------------------------//

void CreateParticleAABB(ParticleData& particleData, const PxBounds3& aabb, const PxVec3& vel, float distance);
void CreateParticleSphere(ParticleData& particleData, const PxVec3& pos, const PxVec3& vel, float distance, unsigned sideNum);
void CreateParticleRand(ParticleData& particleData, const PxVec3& center, const PxVec3& range,const PxVec3& vel);
void SetParticleRestOffsetVariance(ParticleData& particleData, PxF32 maxRestOffset, PxF32 restOffsetVariance);

//----------------------------------------------------------------------------//

#endif // PARTICLE_FACTORY_H
