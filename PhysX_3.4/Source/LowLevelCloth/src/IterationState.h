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

#pragma once

#include "foundation/PxTransform.h"
#include "foundation/PxMat44.h"
#include "Types.h"
#include "Array.h"
#include "Simd.h"
#include "PsMathUtils.h"

namespace physx
{

/* function object to perform solver iterations on one cloth */

// todo: performance optimization: cache this object and test if velocity/iterDt has changed
// c'tor takes about 5% of the iteration time of a 20x20 cloth

namespace cloth
{

/*  helper functions */

template <typename T>
T sqr(const T& x)
{
	return x * x;
}

inline PxVec3 log(const PxQuat& q)
{
	float theta = q.getImaginaryPart().magnitude();
	float scale = theta > PX_EPS_REAL ? PxAsin(theta) / theta : 1.0f;
	scale = intrinsics::fsel(q.w, scale, -scale);
	return PxVec3(q.x * scale, q.y * scale, q.z * scale);
}

inline PxQuat exp(const PxVec3& v)
{
	float theta = v.magnitude();
	float scale = theta > PX_EPS_REAL ? PxSin(theta) / theta : 1.0f;
	return PxQuat(v.x * scale, v.y * scale, v.z * scale, PxCos(theta));
}

template <typename Simd4f, uint32_t N>
inline void assign(Simd4f (&columns)[N], const PxMat44& matrix)
{
	for(uint32_t i = 0; i < N; ++i)
		columns[i] = load(array(matrix[i]));
}

template <typename Simd4f>
inline Simd4f transform(const Simd4f (&columns)[3], const Simd4f& vec)
{
	return splat<0>(vec) * columns[0] + splat<1>(vec) * columns[1] + splat<2>(vec) * columns[2];
}

template <typename Simd4f>
inline Simd4f transform(const Simd4f (&columns)[3], const Simd4f& translate, const Simd4f& vec)
{
	return translate + splat<0>(vec) * columns[0] + splat<1>(vec) * columns[1] + splat<2>(vec) * columns[2];
}

template <typename>
struct IterationState; // forward declaration

struct IterationStateFactory
{
	template <typename MyCloth>
	IterationStateFactory(MyCloth& cloth, float frameDt);

	template <typename Simd4f, typename MyCloth>
	IterationState<Simd4f> create(MyCloth const& cloth) const;

	template <typename Simd4f>
	static Simd4f lengthSqr(Simd4f const& v)
	{
		return dot3(v, v);
	}

	template <typename Simd4f>
	static PxVec3 castToPxVec3(const Simd4f& v)
	{
		return *reinterpret_cast<const PxVec3*>(reinterpret_cast<const char*>(&v));
	}

	int mNumIterations;
	float mInvNumIterations;
	float mIterDt, mIterDtRatio, mIterDtAverage;
	PxQuat mCurrentRotation;
	PxVec3 mPrevLinearVelocity;
	PxVec3 mPrevAngularVelocity;
};

/* solver iterations helper functor */
template <typename Simd4f>
struct IterationState
{
	// call after each iteration
	void update();

	inline float getCurrentAlpha() const;
	inline float getPreviousAlpha() const;

  public:
	Simd4f mRotationMatrix[3]; // should rename to 'mRotation'

	Simd4f mCurBias;  // in local space
	Simd4f mPrevBias; // in local space
	Simd4f mWind;     // delta position per iteration

	Simd4f mPrevMatrix[3];
	Simd4f mCurMatrix[3];
	Simd4f mDampScaleUpdate;

	// iteration counter
	uint32_t mRemainingIterations;

	// reciprocal total number of iterations
	float mInvNumIterations;

	// time step size per iteration
	float mIterDt;

	bool mIsTurning; // if false, mPositionScale = mPrevMatrix[0]
};

} // namespace cloth

template <typename Simd4f>
inline float cloth::IterationState<Simd4f>::getCurrentAlpha() const
{
	return getPreviousAlpha() + mInvNumIterations;
}

template <typename Simd4f>
inline float cloth::IterationState<Simd4f>::getPreviousAlpha() const
{
	return 1.0f - mRemainingIterations * mInvNumIterations;
}

template <typename MyCloth>
cloth::IterationStateFactory::IterationStateFactory(MyCloth& cloth, float frameDt)
{
	mNumIterations = PxMax(1, int(frameDt * cloth.mSolverFrequency + 0.5f));
	mInvNumIterations = 1.0f / mNumIterations;
	mIterDt = frameDt * mInvNumIterations;

	mIterDtRatio = cloth.mPrevIterDt ? mIterDt / cloth.mPrevIterDt : 1.0f;
	mIterDtAverage = cloth.mIterDtAvg.empty() ? mIterDt : cloth.mIterDtAvg.average();

	mCurrentRotation = cloth.mCurrentMotion.q;
	mPrevLinearVelocity = cloth.mLinearVelocity;
	mPrevAngularVelocity = cloth.mAngularVelocity;

	// update cloth
	float invFrameDt = 1.0f / frameDt;
	cloth.mLinearVelocity = invFrameDt * (cloth.mTargetMotion.p - cloth.mCurrentMotion.p);
	PxQuat dq = cloth.mTargetMotion.q * cloth.mCurrentMotion.q.getConjugate();
	cloth.mAngularVelocity = log(dq) * invFrameDt;

	cloth.mPrevIterDt = mIterDt;
	cloth.mIterDtAvg.push(static_cast<uint32_t>(mNumIterations), mIterDt);
	cloth.mCurrentMotion = cloth.mTargetMotion;
}

/*
momentum conservation:
m2*x2 - m1*x1 = m1*x1 - m0*x0 + g*dt2, m = r+t
r2*x2+t2 = 2(r1*x1+t1) - (r0*x0+t0) + g*dt2
r2*x2 = r1*x1 + r1*x1 - r0*x0 - (t2-2t1+t0) + g*dt2
substitue       r1*x1 - r0*x0 = r1*(x1-x0) + (r1-r0)*x0
and     r1*x1 = r2*x1 - (r2-r1)*x1

x2 = x1 + r2'*g*dt2
   + r2'r1*(x1-x0) //< damp
   + (r2'r1-r2'r0)*x0 - (1-r2'r1)*x1 - r2'*(t2-2t1+t0) //< inertia
   + (1-r2'r1)x1 + t2-t1 //< drag (not momentum conserving)

x2 = x0 + a0*x0 + a1*x1 + b with
a0 = (inertia-damp)*r2'r1 - inertia*r2'r0 - eye
a1 = (1-inertia-drag)*eye + (damp+inertia+drag)*r2'r1
b = r2'*(g*dt2 - (inertia+drag)*(t2-t1) + inertia*(t1-t0))

Velocities are used to deal with multiple iterations and varying dt. Only b needs
to updated from one iteration to the next. Specifically, it is multiplied
by (r2'r1)^1/numIterations. a0 and a1 are unaffected by that multiplication.

The centrifugal and coriolis forces of non-inertial (turning) reference frame are
not generally captured in these formulas. The 'inertia' term above contains radial
acceleration plus centrifugal and coriolis force for a single iteration.
For multiple iterations, or when the centrifugal forces are scaled differently
than angular inertia, we need to add explicit centrifugal and coriolis forces.
We only use them to correct the above formula because their discretization is
not accurate.

Possible improvements: multiply coriolis and centrifugal matrix by curInvRotation
from the left. Do the alpha trick of linearInertia also for angularInertia, write
prevParticle after multiplying it with matrix.

If you change anything in this function, make sure that ClothCustomFloating and
ClothInertia haven't regressed for any choice of solver frequency.
*/

template <typename Simd4f, typename MyCloth>
cloth::IterationState<Simd4f> cloth::IterationStateFactory::create(MyCloth const& cloth) const
{
	IterationState<Simd4f> result;

	result.mRemainingIterations = static_cast<uint32_t>(mNumIterations);
	result.mInvNumIterations = mInvNumIterations;
	result.mIterDt = mIterDt;

	Simd4f curLinearVelocity = load(array(cloth.mLinearVelocity));
	Simd4f prevLinearVelocity = load(array(mPrevLinearVelocity));

	Simd4f iterDt = simd4f(mIterDt);
	Simd4f dampExponent = simd4f(cloth.mStiffnessFrequency) * iterDt;

	Simd4f translation = iterDt * curLinearVelocity;

	// gravity delta per iteration
	Simd4f gravity = load(array(cloth.mGravity)) * static_cast<Simd4f>(simd4f(sqr(mIterDtAverage)));

	// scale of local particle velocity per iteration
	Simd4f dampScale = exp2(load(array(cloth.mLogDamping)) * dampExponent);
	// adjust for the change in time step during the first iteration
	Simd4f firstDampScale = dampScale * simd4f(mIterDtRatio);

	// portion of negative frame velocity to transfer to particle
	Simd4f linearDrag = (gSimd4fOne - exp2(load(array(cloth.mLinearLogDrag)) * dampExponent)) * translation;

	// portion of frame acceleration to transfer to particle
	Simd4f linearInertia = load(array(cloth.mLinearInertia)) * iterDt * (prevLinearVelocity - curLinearVelocity);

	// for inertia, we want to violate newton physics to
	// match velocity and position as given by the user, which means:
	// vt = v0 + a*t and xt = x0 + v0*t + (!) a*t^2
	// this is achieved by applying a different portion to cur and prev
	// position, compared to the normal +0.5 and -0.5 for '... 1/2 a*t^2'.
	// specifically, the portion is alpha=(n+1)/2n and 1-alpha.

	float linearAlpha = (mNumIterations + 1) * 0.5f * mInvNumIterations;
	Simd4f curLinearInertia = linearInertia * simd4f(linearAlpha);

	// rotate to local space (use mRotationMatrix temporarily to hold matrix)
	PxMat44 invRotation(mCurrentRotation.getConjugate());
	assign(result.mRotationMatrix, invRotation);

	Simd4f maskXYZ = simd4f(simd4i(~0, ~0, ~0, 0));

	// Previously, we split the bias between previous and current position to
	// get correct disretized position and velocity. However, this made a
	// hanging cloth experience a downward velocity, which is problematic
	// when scaled by the iterDt ratio and results in jitter under variable
	// timesteps. Instead, we now apply the entire bias to current position
	// and accept a less noticeable error for a free falling cloth.

	Simd4f bias = gravity - linearDrag;
	result.mCurBias = transform(result.mRotationMatrix, curLinearInertia + bias) & maskXYZ;
	result.mPrevBias = transform(result.mRotationMatrix, linearInertia - curLinearInertia) & maskXYZ;

	Simd4f wind = load(array(cloth.mWind)) * iterDt;
	result.mWind = transform(result.mRotationMatrix, translation - wind) & maskXYZ;

	result.mIsTurning = mPrevAngularVelocity.magnitudeSquared() + cloth.mAngularVelocity.magnitudeSquared() > 0.0f;

	if(result.mIsTurning)
	{
		Simd4f curAngularVelocity = load(array(invRotation.rotate(cloth.mAngularVelocity)));
		Simd4f prevAngularVelocity = load(array(invRotation.rotate(mPrevAngularVelocity)));

		// rotation for one iteration in local space
		Simd4f curInvAngle = -iterDt * curAngularVelocity;
		Simd4f prevInvAngle = -iterDt * prevAngularVelocity;

		PxQuat curInvRotation = exp(castToPxVec3(curInvAngle));
		PxQuat prevInvRotation = exp(castToPxVec3(prevInvAngle));

		PxMat44 curMatrix(curInvRotation);
		PxMat44 prevMatrix(prevInvRotation * curInvRotation);

		assign(result.mRotationMatrix, curMatrix);

		Simd4f angularDrag = gSimd4fOne - exp2(load(array(cloth.mAngularLogDrag)) * dampExponent);
		Simd4f centrifugalInertia = load(array(cloth.mCentrifugalInertia));
		Simd4f angularInertia = load(array(cloth.mAngularInertia));
		Simd4f angularAcceleration = curAngularVelocity - prevAngularVelocity;

		Simd4f epsilon = simd4f(PxSqrt(FLT_MIN)); // requirement: sqr(epsilon) > 0
		Simd4f velocityLengthSqr = lengthSqr(curAngularVelocity) + epsilon;
		Simd4f dragLengthSqr = lengthSqr(Simd4f(curAngularVelocity * angularDrag)) + epsilon;
		Simd4f centrifugalLengthSqr = lengthSqr(Simd4f(curAngularVelocity * centrifugalInertia)) + epsilon;
		Simd4f accelerationLengthSqr = lengthSqr(angularAcceleration) + epsilon;
		Simd4f inertiaLengthSqr = lengthSqr(Simd4f(angularAcceleration * angularInertia)) + epsilon;

		float dragScale = array(rsqrt(velocityLengthSqr * dragLengthSqr) * dragLengthSqr)[0];
		float inertiaScale =
		    mInvNumIterations * array(rsqrt(accelerationLengthSqr * inertiaLengthSqr) * inertiaLengthSqr)[0];

		// magic factor found by comparing to global space simulation:
		// some centrifugal force is in inertia part, remainder is 2*(n-1)/n
		// after scaling the inertia part, we get for centrifugal:
		float centrifugalAlpha = (2 * mNumIterations - 1) * mInvNumIterations;
		float centrifugalScale =
		    centrifugalAlpha * array(rsqrt(velocityLengthSqr * centrifugalLengthSqr) * centrifugalLengthSqr)[0] -
		    inertiaScale;

		// slightly better in ClothCustomFloating than curInvAngle alone
		Simd4f centrifugalVelocity = (prevInvAngle + curInvAngle) * simd4f(0.5f);
		const Simd4f data = lengthSqr(centrifugalVelocity);
		float centrifugalSqrLength = array(data)[0] * centrifugalScale;

		Simd4f coriolisVelocity = centrifugalVelocity * simd4f(centrifugalScale);
		PxMat33 coriolisMatrix = shdfnd::star(castToPxVec3(coriolisVelocity));

		const float* dampScalePtr = array(firstDampScale);
		const float* centrifugalPtr = array(centrifugalVelocity);

		for(unsigned int j = 0; j < 3; ++j)
		{
			float centrifugalJ = -centrifugalPtr[j] * centrifugalScale;
			for(unsigned int i = 0; i < 3; ++i)
			{
				float damping = dampScalePtr[j];
				float coriolis = coriolisMatrix(i, j);
				float centrifugal = centrifugalPtr[i] * centrifugalJ;

				prevMatrix(i, j) = centrifugal - coriolis + curMatrix(i, j) * (inertiaScale - damping) -
				                   prevMatrix(i, j) * inertiaScale;
				curMatrix(i, j) = centrifugal + coriolis + curMatrix(i, j) * (inertiaScale + damping + dragScale);
			}
			curMatrix(j, j) += centrifugalSqrLength - inertiaScale - dragScale;
			prevMatrix(j, j) += centrifugalSqrLength;
		}

		assign(result.mPrevMatrix, prevMatrix);
		assign(result.mCurMatrix, curMatrix);
	}
	else
	{
		Simd4f minusOne = -static_cast<Simd4f>(gSimd4fOne);
		result.mRotationMatrix[0] = minusOne;
		result.mPrevMatrix[0] = select(maskXYZ, firstDampScale, minusOne);
	}

	// difference of damp scale between first and other iterations
	result.mDampScaleUpdate = (dampScale - firstDampScale) & maskXYZ;

	return result;
}

template <typename Simd4f>
void cloth::IterationState<Simd4f>::update()
{
	if(mIsTurning)
	{
		// only need to turn bias, matrix is unaffected (todo: verify)
		mCurBias = transform(mRotationMatrix, mCurBias);
		mPrevBias = transform(mRotationMatrix, mPrevBias);
		mWind = transform(mRotationMatrix, mWind);
	}

	// remove time step ratio in damp scale after first iteration
	for(uint32_t i = 0; i < 3; ++i)
	{
		mPrevMatrix[i] = mPrevMatrix[i] - mRotationMatrix[i] * mDampScaleUpdate;
		mCurMatrix[i] = mCurMatrix[i] + mRotationMatrix[i] * mDampScaleUpdate;
	}
	mDampScaleUpdate = gSimd4fZero; // only once

	--mRemainingIterations;
}

} // namespace physx
