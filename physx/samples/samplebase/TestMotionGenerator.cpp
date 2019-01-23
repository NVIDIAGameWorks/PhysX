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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "TestMotionGenerator.h"

//----------------------------------------------------------------------------//
MotionGenerator::MotionGenerator() 
:	mLinear(0.0f),
	mAngular(0.0f),
	mTransform(PxTransform(PxIdentity))
{
}

//----------------------------------------------------------------------------//
MotionGenerator::MotionGenerator(const PxTransform &pose, const PxVec3& linear, const PxVec3& angular) 
:	mLinear(linear),
	mAngular(angular),
	mTransform(pose)
{
}

//----------------------------------------------------------------------------//
PxVec3 MotionGenerator::getLinearVelocity( float time )
{
	float t = time - (int(time) & ~0xf);
	const float scale = 0.25f * PxPi;

	if(t > 0 && t < 2)
		return -scale * sinf(t * 0.5f * PxPi) * mLinear;

	if(t > 8 && t < 10)
		return +scale * sinf(t * 0.5f * PxPi) * mLinear;

	return PxVec3(0.0f);
}

//----------------------------------------------------------------------------//
PxVec3 MotionGenerator::getAngularVelocity( float time )
{
	float t = time - (int(time) & ~0xf);

	if(t > 4 && t < 6)
		return +PxPi * mAngular;

	if(t > 12 && t < 14)
		return -PxPi * mAngular;

	return PxVec3(0.0f);
}

static PxQuat computeQuatFromAngularVelocity(const PxVec3 &omega)
{
	PxReal angle = omega.magnitude();

	if (angle < 1e-5f) 
	{
		return PxQuat(PxIdentity);
	} else {
		PxReal s = sin( 0.5f * angle ) / angle;
		PxReal x = omega[0] * s;
		PxReal y = omega[1] * s;
		PxReal z = omega[2] * s;
		PxReal w = cos( 0.5f * angle );
		return PxQuat(x,y,z,w);
	}
}
//----------------------------------------------------------------------------//
const PxTransform& MotionGenerator::update(float time, float dt)
{
	PxVec3 dw = dt * getAngularVelocity(time);
	PxQuat dq = computeQuatFromAngularVelocity(dw);

	mTransform.q = (dq * mTransform.q).getNormalized();
	mTransform.p += dt * getLinearVelocity(time);
	
	return mTransform;
}





