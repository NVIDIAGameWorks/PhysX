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

#ifndef PARTICLE_EMITTER_H
#define PARTICLE_EMITTER_H

#include "ParticleFactory.h"
#include "PxTkRandom.h"

using namespace PxToolkit;
//----------------------------------------------------------------------------//

#define PARTICLE_EMITTER_RANDOMIZE_EMISSION 1

//----------------------------------------------------------------------------//

class ParticleEmitter : public SampleAllocateable
{
public:

	struct Shape
	{
		enum Enum
		{
			eELLIPSE	= 0,
			eRECTANGLE	= 1,
		};
	};

						ParticleEmitter(Shape::Enum shape, PxReal extentX, PxReal extentY, PxReal spacing);
	virtual				~ParticleEmitter();

	// Relative to mFrameBody, or relative to global frame, if mFrameBody == NULL
	void				setLocalPose(const PxTransform& pose)		{ mLocalPose = pose; }
	PxTransform			getLocalPose()						const	{ return mLocalPose; } 

	void				setFrameRigidBody(PxRigidDynamic* rigidBody){ mFrameBody = rigidBody; }
	PxRigidDynamic*		getFrameRigidBody()					const	{ return mFrameBody; }

	void 				setRandomPos(PxVec3 t)						{ mRandomPos = t; }
	PxVec3 				getRandomPos()						const	{ return mRandomPos; }

	void 				setRandomAngle(PxReal t)					{ mRandomAngle = t; }
	PxReal 				getRandomAngle()					const	{ return mRandomAngle; }

	void 				setVelocity(PxReal t)						{ mVelocity = t; }
	PxReal 				getVelocity()						const	{ return mVelocity; }

	// Used for two way interaction, zero meaning, there is none
	void				setParticleMass(PxReal m)						{ mParticleMass = m; }
	PxReal 				getParticleMass()						const	{ return mParticleMass; }

	void				step(ParticleData& particles, PxReal dt, const PxVec3& externalAcceleration, PxReal maxParticleVelocity);

protected:

	virtual	void		stepInternal(ParticleData& particles, PxReal dt, const PxVec3& externalAcceleration, PxReal maxParticleVelocity) = 0;

	void				initStep(ParticleData& particles, PxReal dt);
	void				finalizeStep();

	PX_INLINE	void	computePositionNoise(PxVec3& posNoise);
				void	computeSiteVelocity(PxVec3& siteVel, const PxVec3& sitePos);
	PX_INLINE	bool	isOutsideShape(PxU32 x, PxU32 y, PxReal offset) { return mShape == Shape::eELLIPSE && outsideEllipse(x, y, offset); }

	static		bool	spawnParticle(
							ParticleData& data,
							PxU32& num, 
							const PxU32 max, 
							const PxVec3& position, 
							const PxVec3& velocity);

	static PX_INLINE PxReal randInRange(PxReal a,PxReal b);
	static PX_INLINE PxU32 randInRange(PxU32 range);
#if PARTICLE_EMITTER_RANDOMIZE_EMISSION
	static PxToolkit::BasicRandom mRandom;
#endif

protected:

	PxVec3				mRandomPos;
	PxReal				mRandomAngle;
	PxReal				mVelocity;
	PxReal				mParticleMass;

	//derived quantities
	PxU32				mNumSites;
	PxU32				mNumX;
	PxU32				mNumY;
	PxReal				mSpacingX;
	PxReal				mSpacingY;
	PxReal				mSpacingZ;

	//only needed during step computations.
	PxVec3				mAxisX;
	PxVec3				mAxisY;
	PxVec3				mAxisZ;	
	PxVec3				mBasePos;

private:

	PX_INLINE	bool	outsideEllipse(PxU32 x, PxU32 y, PxReal offset);
	void				updateDerivedBase();

private:

	PxTransform			mLocalPose;
	PxRigidDynamic*		mFrameBody;

	PxReal				mExtentX;
	PxReal				mExtentY;
	Shape::Enum			mShape;

	//state derived quantities
	PxReal				mEllipseRadius2;
	PxReal				mEllipseConstX0;
	PxReal				mEllipseConstX1;
	PxReal				mEllipseConstY0;
	PxReal				mEllipseConstY1;

	//only needed during step computations.
	PxVec3				mBodyAngVel;
	PxVec3				mBodyLinVel;
	PxVec3				mBodyCenter;
	PxTransform			mGlobalPose;
	PxVec3				mLinMomentum;
	PxVec3				mAngMomentum;
};

//----------------------------------------------------------------------------//

PX_INLINE bool ParticleEmitter::outsideEllipse(PxU32 x, PxU32 y, PxReal offset)
{
	PxReal cX = (x + offset - mEllipseConstX0)*mEllipseConstX1;
	PxReal cY = (y 		    - mEllipseConstY0)*mEllipseConstY1;
	return ( cX*cX + cY*cY >= mEllipseRadius2);
}

//----------------------------------------------------------------------------//

PX_INLINE void ParticleEmitter::computePositionNoise(PxVec3& posRand)
{
	posRand.x = randInRange(-mRandomPos.x, mRandomPos.x);
	posRand.y = randInRange(-mRandomPos.y, mRandomPos.y);
	posRand.z = randInRange(-mRandomPos.z, mRandomPos.z);
}

//----------------------------------------------------------------------------//

PX_INLINE PxReal ParticleEmitter::randInRange(PxReal a, PxReal b)
{
#if PARTICLE_EMITTER_RANDOMIZE_EMISSION
	return Rand(a, b);
#else
	return a + (b-a)/2.0f;
#endif
}

//----------------------------------------------------------------------------//

PX_INLINE PxU32 ParticleEmitter::randInRange(PxU32 range)
{
#if PARTICLE_EMITTER_RANDOMIZE_EMISSION
	PxU32 retval = Rand();
	if(range > 0x7fff)
	{
		retval = (retval << 15) | Rand();
		retval = (retval << 15) | Rand();
	}
	return retval % range;
#else
	static PxU32 noRandomVal = 0;
	return noRandomVal++;
#endif
}

//----------------------------------------------------------------------------//

#endif // PARTICLE_EMITTER_H
