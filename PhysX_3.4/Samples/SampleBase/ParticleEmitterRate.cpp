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

#include "ParticleEmitterRate.h"
#include "PsHash.h"

//----------------------------------------------------------------------------//

/**
If the emitter has less emission sites than PARTICLE_EMITTER_SPARSE_FACTOR 
times the number of particles it needs to emit, "sparse mode" code path is applied.
*/
#define PARTICLE_EMITTER_SPARSE_FACTOR 4

/**
Defines how many random sites are choosen before giving up, avoiding spawning more than
one particle per site.
*/
#define PARTICLE_EMITTER_NUM_HASH_TRIALS 10

//----------------------------------------------------------------------------//

ParticleEmitterRate::ParticleEmitterRate(Shape::Enum shape, PxReal extentX, PxReal extentY, PxReal spacing) :
	ParticleEmitter(shape, extentX, extentY, spacing),
	mRate(1.0f),
	mParticlesToEmit(0)
{
}

//----------------------------------------------------------------------------//

ParticleEmitterRate::~ParticleEmitterRate()
{
}

//----------------------------------------------------------------------------//

void ParticleEmitterRate::stepInternal(ParticleData& particles, PxReal dt, const PxVec3& externalAcceleration, PxReal maxParticleVelocity)
{
	PX_ASSERT(mNumX > 0 && mNumY > 0);
	PxU32 numEmittedParticles = 0;

	//figure out how many particle have to be emitted with the given rate.
	mParticlesToEmit += mRate*dt;
	PxU32 numEmit = (PxU32)(mParticlesToEmit);
	if(numEmit == 0)
		return;
	
	PxU32 numLayers = (PxU32)(numEmit / (mNumX * mNumY)) + 1;
	PxReal layerDistance = dt * mVelocity / numLayers;

	PxU32 sparseMax = 0;

	//either shuffle or draw without repeat (approximation)
	bool denseEmission = (PxU32(PARTICLE_EMITTER_SPARSE_FACTOR*numEmit) > mNumSites);
	if(denseEmission)
	{
		initDenseSites();
	}
	else
	{
		sparseMax = PARTICLE_EMITTER_SPARSE_FACTOR*numEmit;
		mSites.resize(sparseMax);
	}

	// generate particles
	PxU32 l = 0;
	while(numEmit > 0)
	{
		PxVec3 layerVec = mAxisZ * (layerDistance * (PxReal)l);
		l++;

		if(denseEmission)
			shuffleDenseSites();
		else
			initSparseSiteHash(numEmit, sparseMax);

		for (PxU32 i = 0; i < mNumSites && numEmit > 0; i++)
		{
			PxU32 emissionSite;
			if (denseEmission)
				emissionSite = mSites[i];
			else
				emissionSite = pickSparseEmissionSite(sparseMax);

			PxU32 x = emissionSite / mNumY;
			PxU32 y = emissionSite % mNumY;

			PxReal offset = 0.0f;
			if (y%2) offset = mSpacingX * 0.5f;
				
			if (isOutsideShape(x,y,offset)) 
				continue;

			//position noise
			PxVec3 posNoise;
			posNoise.x = randInRange(-mRandomPos.x, mRandomPos.x);
			posNoise.y = randInRange(-mRandomPos.y, mRandomPos.y);	
			posNoise.z = randInRange(-mRandomPos.z, mRandomPos.z);	

			PxVec3 emissionPoint = mBasePos + layerVec + 
				mAxisX*(posNoise.x+offset+mSpacingX*x) + mAxisY*(posNoise.y+mSpacingY*y) + mAxisZ*posNoise.z;

			PxVec3 particleVelocity;
			computeSiteVelocity(particleVelocity, emissionPoint);

			bool isSpawned = spawnParticle(particles, numEmittedParticles, particles.maxParticles - particles.numParticles, emissionPoint, particleVelocity);
			if(isSpawned)
			{
				numEmit--;
				mParticlesToEmit -= 1.0f;
			}
			else
				return;
		}
	}
}

//----------------------------------------------------------------------------//

void ParticleEmitterRate::initDenseSites()
{
	mSites.resize(mNumSites);

	for(PxU32 i = 0; i < mNumSites; i++)
		mSites[i] = i;
}

//----------------------------------------------------------------------------//

void ParticleEmitterRate::shuffleDenseSites()
{
	PxU32 i,j;
	PX_ASSERT(mSites.size() == mNumSites);

	for (i = 0; i < mNumSites; i++) 
	{
		j = randInRange(mNumSites);
		PX_ASSERT(j<mNumSites);

		PxU32 k = mSites[i];
		mSites[i] = mSites[j]; 
		mSites[j] = k;
	}
}

//----------------------------------------------------------------------------//

void ParticleEmitterRate::initSparseSiteHash(PxU32 numEmit, PxU32 sparseMax)
{
	PX_ASSERT(PxU32(PARTICLE_EMITTER_SPARSE_FACTOR*numEmit) <= sparseMax);
	PX_ASSERT(mSites.size() == sparseMax);
	for(PxU32 i = 0; i < sparseMax; i++)
		mSites[i] = 0xffffffff;
}

//----------------------------------------------------------------------------//

PxU32 ParticleEmitterRate::pickSparseEmissionSite(PxU32 sparseMax)
{
	PxU32 emissionSite = randInRange(mNumSites);
	PxU32 hashKey = Ps::hash(emissionSite);
	PxU32 hashIndex = hashKey % sparseMax;
	PxU32 numTrials = 0;
	while(mSites[hashIndex] != 0xffffffff && numTrials < PARTICLE_EMITTER_NUM_HASH_TRIALS)
	{
		emissionSite = randInRange(mNumSites);
		hashKey = Ps::hash(emissionSite);
		hashIndex = hashKey % sparseMax;
		numTrials++;
	}
	//allow sites to be used multiple times if mSites[hashIndex] == 0xffffffff
	return mSites[hashIndex] = emissionSite;
}

//----------------------------------------------------------------------------//

