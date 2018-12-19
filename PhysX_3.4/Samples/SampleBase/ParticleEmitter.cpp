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

//----------------------------------------------------------------------------//

#if PARTICLE_EMITTER_RANDOMIZE_EMISSION
PxToolkit::BasicRandom ParticleEmitter::mRandom(42);
#endif

ParticleEmitter::ParticleEmitter(Shape::Enum shape, PxReal extentX, PxReal extentY, PxReal spacing) :
	mRandomPos(0.0f),
	mRandomAngle(0.0f),
	mVelocity(1.0f),
	mParticleMass(0.0f),
	mNumSites(0),
	mSpacingX(spacing),
	mLocalPose(PxTransform()),
	mFrameBody(NULL),
	mExtentX(extentX),
	mExtentY(extentY),
	mShape(shape)
{
	PX_ASSERT(spacing > 0.0f);
	updateDerivedBase();
}

//----------------------------------------------------------------------------//

ParticleEmitter::~ParticleEmitter()
{

}

//----------------------------------------------------------------------------//

void ParticleEmitter::updateDerivedBase()
{
	PX_ASSERT(mSpacingX > 0.0f);
	mSpacingY = mSpacingX * PxSqrt(3.0f) * 0.5f;
	mSpacingZ = mSpacingX;

	mNumX = 2*(int)floor(mExtentX/mSpacingX);
	mNumY = 2*(int)floor(mExtentY/mSpacingY);
	
	//SDS: limit minimal dimension to 1
	if (mNumX == 0)
	{
		mNumX = 1;
		mSpacingX = 0.0f;
	}
	if (mNumY == 0)
	{
		mNumY = 1;
		mSpacingY = 0.0f;
	}

	mNumSites = mNumX * mNumY;

	if (mShape == Shape::eELLIPSE) 
	{
		if (mNumX > 1) 
			mEllipseRadius2 = 0.5f - 1.0f/(mNumX-1);
		else			
			mEllipseRadius2 = 0.5f; 
		mEllipseRadius2 *= mEllipseRadius2;

		mEllipseConstX0 = (mNumX-0.5f) * 0.5f;
		mEllipseConstX1 = 1.0f/mNumX;
		mEllipseConstY0 = (mNumY-1.0f) * 0.5f;
		mEllipseConstY1 = PxSqrt(3.0f) * 0.5f / mNumY;
	}
	else 
	{
		mEllipseRadius2 = 0;
		mEllipseConstX0 = 0;
		mEllipseConstX1 = 0;
		mEllipseConstY0 = 0;
		mEllipseConstY1 = 0;
	}
}

//----------------------------------------------------------------------------//

void ParticleEmitter::step(ParticleData& particles, PxReal dt, const PxVec3& externalAcceleration, PxReal maxParticleVelocity)
{
	initStep(particles, dt);
	stepInternal(particles, dt, externalAcceleration, maxParticleVelocity);
	finalizeStep();
}

//----------------------------------------------------------------------------//

void ParticleEmitter::initStep(ParticleData& particles, PxReal dt)
{
	mLinMomentum = PxVec3(0.0f);
	mAngMomentum = PxVec3(0.0f);

	// state of frameBody
	if (mFrameBody) 
	{
		mBodyAngVel = mFrameBody->getAngularVelocity();
		mBodyLinVel = mFrameBody->getLinearVelocity();
		mBodyCenter = mFrameBody->getGlobalPose().p;
		mGlobalPose = mFrameBody->getGlobalPose() * mLocalPose;
	}
	else
	{
		mBodyAngVel = PxVec3(0.0f);
		mBodyLinVel = PxVec3(0.0f);
		mBodyCenter = PxVec3(0.0f);
		mGlobalPose = mLocalPose;
	}

	mAxisX = mGlobalPose.q.getBasisVector0();
	mAxisY = mGlobalPose.q.getBasisVector1();
	mAxisZ = mGlobalPose.q.getBasisVector2();

	mBasePos = mGlobalPose.p 
		- mAxisX * (mNumX-0.5f)*0.5f*mSpacingX
		- mAxisY * (mNumY-1.0f)*0.5f*mSpacingY;
}

//----------------------------------------------------------------------------//

void ParticleEmitter::finalizeStep()
{
	// apply impulse on attached body
	if (mFrameBody && mParticleMass > 0.0f)
	{
		mLinMomentum *= mParticleMass;
		mAngMomentum *= mParticleMass;

		mFrameBody->addForce(mLinMomentum, PxForceMode::eIMPULSE);
		mFrameBody->addTorque(mAngMomentum, PxForceMode::eIMPULSE);
	}
}

//----------------------------------------------------------------------------//

void ParticleEmitter::computeSiteVelocity(PxVec3& siteVel, const PxVec3& sitePos)
{
	//velocity dir noise
	PxReal noiseXYAngle = randInRange(0.0f, PxTwoPi);
	PxReal noiseZAngle  = randInRange(0.0f, mRandomAngle);

	PxVec3 noiseDirVec = mAxisX * PxCos(noiseXYAngle) + mAxisY * PxSin(noiseXYAngle);
	noiseDirVec.normalize();
	noiseDirVec = mAxisZ * PxCos(noiseZAngle) + noiseDirVec * PxSin(noiseZAngle);

	siteVel = noiseDirVec * mVelocity;

	//add emitter repulsion
	if (mParticleMass > 0.0f)
	{
		mLinMomentum -= siteVel;
		mAngMomentum -= (sitePos - mBodyCenter).cross(siteVel);
	}

	if (mFrameBody)
		siteVel += mBodyLinVel + (mBodyAngVel.cross(sitePos - mBodyCenter)); 
}

//----------------------------------------------------------------------------//

bool ParticleEmitter::spawnParticle(ParticleData& data, 
									PxU32& num, 
									const PxU32 max, 
									const PxVec3& position, 
									const PxVec3& velocity)
{
	PX_ASSERT(PxI32(max) - PxI32(num) <= PxI32(data.maxParticles) - PxI32(data.numParticles));
	if(num >= max) 
		return false;

	data.positions[data.numParticles] = position;
	data.velocities[data.numParticles] = velocity;
	data.numParticles++;
	num++;
	return true;
}

//----------------------------------------------------------------------------//
