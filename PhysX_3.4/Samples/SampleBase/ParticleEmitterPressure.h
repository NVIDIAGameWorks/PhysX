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

#ifndef PARTICLE_EMITTER_PRESSURE_H
#define PARTICLE_EMITTER_PRESSURE_H

//----------------------------------------------------------------------------//

#include "ParticleEmitter.h"

//----------------------------------------------------------------------------//

class ParticleEmitterPressure : public ParticleEmitter
{

public:
						ParticleEmitterPressure(ParticleEmitter::Shape::Enum shape, PxReal	extentX, PxReal extentY, PxReal spacing);
	virtual				~ParticleEmitterPressure();

	virtual		void	stepInternal(ParticleData& particles, PxReal dt, const PxVec3& externalAcceleration, PxReal maxParticleVelocity);

				void	setMaxRate(PxReal v)									{ mMaxRate = v; }
				PxReal	getMaxRate() const										{ return mMaxRate; }


private:

	struct SiteData
	{
		PxVec3 position;
		PxVec3 velocity;
		bool predecessor;
		PxReal noiseZ;
	};

private:

	PxVec3 mSimulationAcceleration;
	PxReal mSimulationMaxVelocity;

	void clearPredecessors();

	bool stepEmissionSite(
		SiteData& siteData,
		ParticleData& spawnData,
		PxU32& spawnNum, 
		const PxU32 spawnMax,
		const PxVec3 &sitePos, 
		const PxVec3 &siteVel,
		const PxReal dt);

	void predictPredecessorPos(SiteData& siteData, PxReal dt);
	void updatePredecessor(SiteData& siteData, const PxVec3& position, const PxVec3& velocity);

private:

	std::vector<SiteData>	mSites;
	PxReal					mMaxZNoiseOffset;
	PxReal					mMaxRate;
};

//----------------------------------------------------------------------------//

#endif // PARTICLE_EMITTER_PRESSURE_H
