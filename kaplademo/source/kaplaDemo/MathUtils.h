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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.

#ifndef MATH_UTILS
#define MATH_UTILS

#include "foundation/PxVec3.h"
#include "foundation/PxQuat.h"
#include "foundation/PxMath.h"

using namespace physx;

// ------------------------------------------------------------------------
PX_FORCE_INLINE PxF32 randRange(const PxF32 a, const PxF32 b)	
{
	return a + (b-a)*::rand()/RAND_MAX;
}

// ------------------------------------------------------------------------
inline static float fastInvSqrt(float input)
{
	const float halfInput = 0.5f * input;
	int         i     = *(int*)&input;

	i = 0x5f375a86 - ( i >> 1 );
	input = *(float*) & i;
	input = input * ( 1.5f - halfInput * input * input);
	return input;
}

// ------------------------------------------------------------------------
inline static void fastNormalize(PxVec3 &v) {
	v *= fastInvSqrt(v.magnitudeSquared());
}

// ------------------------------------------------------------------------
inline static float fastLen(PxVec3 &v) {
	return 1.0f / fastInvSqrt(v.magnitudeSquared());
}

// ------------------------------------------------------------------------
inline static float fastNormalizeLen(PxVec3 &v) {
	float inv = fastInvSqrt(v.magnitudeSquared());
	v *= inv;
	return 1.0f / inv;
}

// ------------------------------------------------------------------------
inline static float fastAcos(float x) {
	// MacLaurin series
	if (x < -1.0f) x = -1.0f;
	if (x > 1.0f) x = 1.0f;
	float x2 = x*x;
	float angle = (35.0f/1152.0f)*x2;
	angle = (5.0f/112.0f) + angle*x2;
	angle = (3.0f/40.0f) + angle*x2;
	angle = (1.0f/6.0f) + angle*x2;
	angle =  1        + angle*x2;
	angle =	 1.5707963267948966f - angle * x;		
	return angle;	
}

// ------------------------------------------------------------------------
inline static void quatFromVectors(const PxVec3 &n0, const PxVec3 &n1, PxQuat &q)
{
	// fast approximation
	PxVec3 axis = n0.cross(n1) * 0.5f;
	q = PxQuat(axis.x, axis.y, axis.z, PxSqrt(1.0f - axis.magnitudeSquared()));

	// correct
	//PxVec3 axis = n0 ^ n1;
	//axis.normalize();
	//float s = PxMath::sqrt(0.5f * (1.0f - n0.dot(n1)));
	//q.x = axis.x * s;
	//q.y = axis.y * s;
	//q.z = axis.z * s;
	//q.w = PxMath::sqrt(1.0f - s*s);
}

// ------------------------------------------------------------------------
void polarDecomposition(const PxMat33 &A, PxMat33 &R);
void polarDecompositionStabilized(const PxMat33 &A, PxMat33 &R);

// ------------------------------------------------------------------------
void eigenDecomposition(PxMat33 &A, PxMat33 &R);

// ------------------------------------------------------------------------
void eigenDecomposition22(const PxMat33 &A, PxMat33 &R, PxMat33 &D);

// ------------------------------------------------------------------------
PxMat33 outerProduct(const PxVec3 &v0, const PxVec3 &v1);

// ------------------------------------------------------------------------
PxQuat align(const PxVec3& v1, const PxVec3& v2);

// ------------------------------------------------------------------------
void decomposeTwistTimesSwing(const PxQuat& q,  const PxVec3& v1,
		PxQuat& twist, PxQuat& swing);

// ------------------------------------------------------------------------
void decomposeSwingTimesTwist(const PxQuat& q, const PxVec3& v1,
		PxQuat& swing, PxQuat& twist);


#endif