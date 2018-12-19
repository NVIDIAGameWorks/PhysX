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

#include "foundation/PxProfiler.h"
#include "SwSolverKernel.h"
#include "SwCloth.h"
#include "SwClothData.h"
#include "SwFabric.h"
#include "SwFactory.h"
#include "PointInterpolator.h"
#include "BoundingBox.h"

#define PX_AVX (NV_SIMD_SIMD&&(PX_WIN32 || PX_WIN64) && PX_VC >= 10)

#if PX_AVX
namespace avx
{
// defined in SwSolveConstraints.cpp

void initialize();

template <bool, uint32_t>
void solveConstraints(float* __restrict, const float* __restrict, const float* __restrict, const uint16_t* __restrict,
                      const __m128&);
}

namespace
{
uint32_t getAvxSupport()
{
// Checking for AVX requires 3 things:
// 1) CPUID indicates that the OS uses XSAVE and XRSTORE
// 2) CPUID indicates support for AVX
// 3) XGETBV indicates registers are saved and restored on context switch

#if _MSC_FULL_VER < 160040219 || !defined(_XCR_XFEATURE_ENABLED_MASK)
	// need at least VC10 SP1 and compile on at least Win7 SP1
	return 0;
#else
	int cpuInfo[4];
	__cpuid(cpuInfo, 1);
	int avxFlags = 3 << 27; // checking 1) and 2) above
	if((cpuInfo[2] & avxFlags) != avxFlags)
		return 0; // xgetbv not enabled or no AVX support

	if((_xgetbv(_XCR_XFEATURE_ENABLED_MASK) & 0x6) != 0x6)
		return 0; // OS does not save YMM registers

	avx::initialize();

#if _MSC_VER < 1700
	return 1;
#else
	int fmaFlags = 1 << 12;
	if((cpuInfo[2] & fmaFlags) != fmaFlags)
		return 1; // no FMA3 support

	/* only using fma at the moment, don't lock out AMD's piledriver by requiring avx2
	__cpuid(cpuInfo, 7);
	int avx2Flags = 1 << 5;
	if((cpuInfo[1] & avx2Flags) != avx2Flags)
	    return 1; // no AVX2 support
	*/

	return 2;
#endif // _MSC_VER
#endif // _MSC_FULL_VER
}

const uint32_t sAvxSupport = getAvxSupport(); // 0: no AVX, 1: AVX, 2: AVX+FMA
}
#endif

using namespace physx;

namespace
{
/* simd constants */

const Simd4fTupleFactory sMaskW = simd4f(simd4i(0, 0, 0, ~0));
const Simd4fTupleFactory sMaskXY = simd4f(simd4i(~0, ~0, 0, 0));
const Simd4fTupleFactory sMaskXYZ = simd4f(simd4i(~0, ~0, ~0, 0));
const Simd4fTupleFactory sMaskYZW = simd4f(simd4i(0, ~0, ~0, ~0));
const Simd4fTupleFactory sMinusOneXYZOneW = simd4f(-1.0f, -1.0f, -1.0f, 1.0f);
const Simd4fTupleFactory sFloatMaxW = simd4f(0.0f, 0.0f, 0.0f, FLT_MAX);
const Simd4fTupleFactory sMinusFloatMaxXYZ = simd4f(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f);

/* static worker functions */

/**
   This function performs explicit Euler integration based on position, where
   x_next = x_cur + (x_cur - x_prev) * dt_cur/dt_prev * damping + g * dt * dt
   The g * dt * dt term is folded into accelIt.
 */

template <typename Simd4f, typename AccelerationIterator>
void integrateParticles(Simd4f* __restrict curIt, Simd4f* __restrict curEnd, Simd4f* __restrict prevIt,
                        const Simd4f& scale, const AccelerationIterator& aIt, const Simd4f& prevBias)
{
	// local copy to avoid LHS
	AccelerationIterator accelIt(aIt);

	for(; curIt != curEnd; ++curIt, ++prevIt, ++accelIt)
	{
		Simd4f current = *curIt;
		Simd4f previous = *prevIt;
		// if(current.w == 0) current.w = previous.w
		current = select(current > sMinusFloatMaxXYZ, current, previous);
		Simd4f finiteMass = splat<3>(previous) > sFloatMaxW;
		Simd4f delta = (current - previous) * scale + *accelIt;
		*curIt = current + (delta & finiteMass);
		*prevIt = select(sMaskW, previous, current) + (prevBias & finiteMass);
	}
}

template <typename Simd4f, typename AccelerationIterator>
void integrateParticles(Simd4f* __restrict curIt, Simd4f* __restrict curEnd, Simd4f* __restrict prevIt,
                        const Simd4f (&prevMatrix)[3], const Simd4f (&curMatrix)[3], const AccelerationIterator& aIt,
                        const Simd4f& prevBias)
{
	// local copy to avoid LHS
	AccelerationIterator accelIt(aIt);

	for(; curIt != curEnd; ++curIt, ++prevIt, ++accelIt)
	{
		Simd4f current = *curIt;
		Simd4f previous = *prevIt;
		// if(current.w == 0) current.w = previous.w
		current = select(current > sMinusFloatMaxXYZ, current, previous);
		Simd4f finiteMass = splat<3>(previous) > sFloatMaxW;
		// curMatrix*current + prevMatrix*previous + accel
		Simd4f delta = cloth::transform(curMatrix, cloth::transform(prevMatrix, *accelIt, previous), current);
		*curIt = current + (delta & finiteMass);
		*prevIt = select(sMaskW, previous, current) + (prevBias & finiteMass);
	}
}

template <typename Simd4f, typename ConstraintIterator>
void constrainMotion(Simd4f* __restrict curIt, const Simd4f* __restrict curEnd, const ConstraintIterator& spheres,
                     const Simd4f& scaleBiasStiffness)
{
	Simd4f scale = splat<0>(scaleBiasStiffness);
	Simd4f bias = splat<1>(scaleBiasStiffness);
	Simd4f stiffness = splat<3>(scaleBiasStiffness);

	// local copy of iterator to maintain alignment
	ConstraintIterator sphIt = spheres;

	for(; curIt < curEnd; curIt += 4)
	{
		// todo: use msub where available
		Simd4f curPos0 = curIt[0];
		Simd4f curPos1 = curIt[1];
		Simd4f curPos2 = curIt[2];
		Simd4f curPos3 = curIt[3];

		Simd4f delta0 = *sphIt - (sMaskXYZ & curPos0);
		++sphIt;
		Simd4f delta1 = *sphIt - (sMaskXYZ & curPos1);
		++sphIt;
		Simd4f delta2 = *sphIt - (sMaskXYZ & curPos2);
		++sphIt;
		Simd4f delta3 = *sphIt - (sMaskXYZ & curPos3);
		++sphIt;

		Simd4f deltaX = delta0, deltaY = delta1, deltaZ = delta2, deltaW = delta3;
		transpose(deltaX, deltaY, deltaZ, deltaW);

		Simd4f sqrLength = gSimd4fEpsilon + deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ;
		Simd4f radius = max(gSimd4fZero, deltaW * scale + bias);

		Simd4f slack = gSimd4fOne - radius * rsqrt(sqrLength);

		// if slack <= 0.0f then we don't want to affect particle
		// and can skip if all particles are unaffected
		Simd4f isPositive;
		if(anyGreater(slack, gSimd4fZero, isPositive))
		{
			// set invMass to zero if radius is zero
			curPos0 = curPos0 & (splat<0>(radius) > sMinusFloatMaxXYZ);
			curPos1 = curPos1 & (splat<1>(radius) > sMinusFloatMaxXYZ);
			curPos2 = curPos2 & (splat<2>(radius) > sMinusFloatMaxXYZ);
			curPos3 = curPos3 & ((radius) > sMinusFloatMaxXYZ);

			slack = slack * stiffness & isPositive;

			curIt[0] = curPos0 + (delta0 & sMaskXYZ) * splat<0>(slack);
			curIt[1] = curPos1 + (delta1 & sMaskXYZ) * splat<1>(slack);
			curIt[2] = curPos2 + (delta2 & sMaskXYZ) * splat<2>(slack);
			curIt[3] = curPos3 + (delta3 & sMaskXYZ) * splat<3>(slack);
		}
	}
}

template <typename Simd4f, typename ConstraintIterator>
void constrainSeparation(Simd4f* __restrict curIt, const Simd4f* __restrict curEnd, const ConstraintIterator& spheres)
{
	// local copy of iterator to maintain alignment
	ConstraintIterator sphIt = spheres;

	for(; curIt < curEnd; curIt += 4)
	{
		// todo: use msub where available
		Simd4f curPos0 = curIt[0];
		Simd4f curPos1 = curIt[1];
		Simd4f curPos2 = curIt[2];
		Simd4f curPos3 = curIt[3];

		Simd4f delta0 = *sphIt - (sMaskXYZ & curPos0);
		++sphIt;
		Simd4f delta1 = *sphIt - (sMaskXYZ & curPos1);
		++sphIt;
		Simd4f delta2 = *sphIt - (sMaskXYZ & curPos2);
		++sphIt;
		Simd4f delta3 = *sphIt - (sMaskXYZ & curPos3);
		++sphIt;

		Simd4f deltaX = delta0, deltaY = delta1, deltaZ = delta2, deltaW = delta3;
		transpose(deltaX, deltaY, deltaZ, deltaW);

		Simd4f sqrLength = gSimd4fEpsilon + deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ;

		Simd4f slack = gSimd4fOne - deltaW * rsqrt<1>(sqrLength);

		// if slack >= 0.0f then we don't want to affect particle
		// and can skip if all particles are unaffected
		Simd4f isNegative;
		if(anyGreater(gSimd4fZero, slack, isNegative))
		{
			slack = slack & isNegative;

			curIt[0] = curPos0 + (delta0 & sMaskXYZ) * splat<0>(slack);
			curIt[1] = curPos1 + (delta1 & sMaskXYZ) * splat<1>(slack);
			curIt[2] = curPos2 + (delta2 & sMaskXYZ) * splat<2>(slack);
			curIt[3] = curPos3 + (delta3 & sMaskXYZ) * splat<3>(slack);
		}
	}
}

/**
    traditional gauss-seidel internal constraint solver
 */
template <bool useMultiplier, typename Simd4f>
void solveConstraints(float* __restrict posIt, const float* __restrict rIt, const float* __restrict rEnd,
                      const uint16_t* __restrict iIt, const Simd4f& stiffnessEtc)
{
	Simd4f stretchLimit, compressionLimit, multiplier;
	if(useMultiplier)
	{
		stretchLimit = splat<3>(stiffnessEtc);
		compressionLimit = splat<2>(stiffnessEtc);
		multiplier = splat<1>(stiffnessEtc);
	}
	Simd4f stiffness = splat<0>(stiffnessEtc);

	for(; rIt != rEnd; rIt += 4, iIt += 8)
	{
		uint32_t p0i = iIt[0] * sizeof(PxVec4);
		uint32_t p0j = iIt[1] * sizeof(PxVec4);
		uint32_t p1i = iIt[2] * sizeof(PxVec4);
		uint32_t p1j = iIt[3] * sizeof(PxVec4);
		uint32_t p2i = iIt[4] * sizeof(PxVec4);
		uint32_t p2j = iIt[5] * sizeof(PxVec4);
		uint32_t p3i = iIt[6] * sizeof(PxVec4);
		uint32_t p3j = iIt[7] * sizeof(PxVec4);

		Simd4f v0i = loadAligned(posIt, p0i);
		Simd4f v0j = loadAligned(posIt, p0j);
		Simd4f v1i = loadAligned(posIt, p1i);
		Simd4f v1j = loadAligned(posIt, p1j);
		Simd4f v2i = loadAligned(posIt, p2i);
		Simd4f v2j = loadAligned(posIt, p2j);
		Simd4f v3i = loadAligned(posIt, p3i);
		Simd4f v3j = loadAligned(posIt, p3j);

		Simd4f h0ij = v0j + v0i * sMinusOneXYZOneW;
		Simd4f h1ij = v1j + v1i * sMinusOneXYZOneW;
		Simd4f h2ij = v2j + v2i * sMinusOneXYZOneW;
		Simd4f h3ij = v3j + v3i * sMinusOneXYZOneW;

		Simd4f hxij = h0ij, hyij = h1ij, hzij = h2ij, vwij = h3ij;
		transpose(hxij, hyij, hzij, vwij);

		Simd4f rij = loadAligned(rIt);
		Simd4f e2ij = gSimd4fEpsilon + hxij * hxij + hyij * hyij + hzij * hzij;
		Simd4f erij = (gSimd4fOne - rij * rsqrt(e2ij)) & (rij > gSimd4fEpsilon);

		if(useMultiplier)
		{
			erij = erij - multiplier * max(compressionLimit, min(erij, stretchLimit));
		}
		Simd4f exij = erij * stiffness * recip(gSimd4fEpsilon + vwij);

		h0ij = h0ij * splat<0>(exij) & sMaskXYZ;
		h1ij = h1ij * splat<1>(exij) & sMaskXYZ;
		h2ij = h2ij * splat<2>(exij) & sMaskXYZ;
		h3ij = h3ij * splat<3>(exij) & sMaskXYZ;

		storeAligned(posIt, p0i, v0i + h0ij * splat<3>(v0i));
		storeAligned(posIt, p0j, v0j - h0ij * splat<3>(v0j));
		storeAligned(posIt, p1i, v1i + h1ij * splat<3>(v1i));
		storeAligned(posIt, p1j, v1j - h1ij * splat<3>(v1j));
		storeAligned(posIt, p2i, v2i + h2ij * splat<3>(v2i));
		storeAligned(posIt, p2j, v2j - h2ij * splat<3>(v2j));
		storeAligned(posIt, p3i, v3i + h3ij * splat<3>(v3i));
		storeAligned(posIt, p3j, v3j - h3ij * splat<3>(v3j));
	}
}

#if PX_WINDOWS
#include "sse2/SwSolveConstraints.h"
#endif

// calculates upper bound of all position deltas
template <typename Simd4f>
Simd4f calculateMaxDelta(const Simd4f* prevIt, const Simd4f* curIt, const Simd4f* curEnd)
{
	Simd4f maxDelta = gSimd4fZero;
	for(; curIt < curEnd; ++curIt, ++prevIt)
		maxDelta = max(maxDelta, abs(*curIt - *prevIt));

	return maxDelta & sMaskXYZ;
}

template <bool IsTurning, typename Simd4f>
void applyWind(Simd4f* __restrict curIt, const Simd4f* __restrict prevIt, const uint16_t* __restrict tIt,
               const uint16_t* __restrict tEnd, Simd4f dragCoefficient, Simd4f liftCoefficient, Simd4f wind,
               const Simd4f (&rotation)[3])
{
	const Simd4f oneThird = simd4f(1 / 3.0f);

	for(; tIt < tEnd; tIt += 3)
	{
		uint16_t i0 = tIt[0];
		uint16_t i1 = tIt[1];
		uint16_t i2 = tIt[2];

		Simd4f c0 = curIt[i0];
		Simd4f c1 = curIt[i1];
		Simd4f c2 = curIt[i2];

		Simd4f p0 = prevIt[i0];
		Simd4f p1 = prevIt[i1];
		Simd4f p2 = prevIt[i2];

		// use particle weights instead?
		Simd4f current = (c0 + c1 + c2) * oneThird;
		Simd4f previous = (p0 + p1 + p2) * oneThird;

		Simd4f delta = current - previous + wind;

		if(IsTurning)
		{
			// add rotation of frame
			delta = cloth::transform(rotation, delta - current, current);
		}

		Simd4f normal = cross3(c2 - c0, c1 - c0);

		Simd4f invSqrScale = dot3(delta, delta) * dot3(normal, normal);
		Simd4f isZero = invSqrScale < gSimd4fEpsilon;
		Simd4f scale = rsqrt(invSqrScale);

		Simd4f cosTheta = dot3(normal, delta) * scale;
		Simd4f sinTheta = sqrt(max(gSimd4fZero, gSimd4fOne - cosTheta * cosTheta));

		// orthogonal to delta, in delta-normal plane, same length as delta
		Simd4f liftDir = cross3(cross3(delta, normal), delta * scale);

		// sin(theta) * cos(theta) = 0.5 * sin(2 * theta)
		Simd4f lift = liftCoefficient * cosTheta * sinTheta * liftDir;
		Simd4f drag = dragCoefficient * abs(cosTheta) * delta;

		Simd4f impulse = (lift + drag) & ~isZero;

		curIt[i0] = c0 - impulse * splat<3>(c0);
		curIt[i1] = c1 - impulse * splat<3>(c1);
		curIt[i2] = c2 - impulse * splat<3>(c2);
	}
}

} // anonymous namespace

template <typename Simd4f>
cloth::SwSolverKernel<Simd4f>::SwSolverKernel(SwCloth const& cloth, SwClothData& clothData,
                                              SwKernelAllocator& allocator, IterationStateFactory& factory)
: mCloth(cloth)
, mClothData(clothData)
, mAllocator(allocator)
, mCollision(clothData, allocator)
, mSelfCollision(clothData, allocator)
, mState(factory.create<Simd4f>(cloth))
{
	mClothData.verify();
}

template <typename Simd4f>
void cloth::SwSolverKernel<Simd4f>::operator()()
{
	simulateCloth();
}

template <typename Simd4f>
size_t cloth::SwSolverKernel<Simd4f>::estimateTemporaryMemory(const SwCloth& cloth)
{
	size_t collisionTempMemory = SwCollision<Simd4f>::estimateTemporaryMemory(cloth);
	size_t selfCollisionTempMemory = SwSelfCollision<Simd4f>::estimateTemporaryMemory(cloth);

	size_t tempMemory = PxMax(collisionTempMemory, selfCollisionTempMemory);
	size_t persistentMemory = SwCollision<Simd4f>::estimatePersistentMemory(cloth);

	// account for any allocator overhead (this could be exposed in the allocator)
	size_t maxAllocs = 32;
	size_t maxPerAllocationOverhead = 32;
	size_t maxAllocatorOverhead = maxAllocs * maxPerAllocationOverhead;

	return maxAllocatorOverhead + persistentMemory + tempMemory;
}

template <typename Simd4f>
template <typename AccelerationIterator>
void cloth::SwSolverKernel<Simd4f>::integrateParticles(AccelerationIterator& accelIt, const Simd4f& prevBias)
{
	Simd4f* curIt = reinterpret_cast<Simd4f*>(mClothData.mCurParticles);
	Simd4f* curEnd = curIt + mClothData.mNumParticles;
	Simd4f* prevIt = reinterpret_cast<Simd4f*>(mClothData.mPrevParticles);

	if(!mState.mIsTurning)
		::integrateParticles(curIt, curEnd, prevIt, mState.mPrevMatrix[0], accelIt, prevBias);
	else
		::integrateParticles(curIt, curEnd, prevIt, mState.mPrevMatrix, mState.mCurMatrix, accelIt, prevBias);
}

template <typename Simd4f>
void cloth::SwSolverKernel<Simd4f>::integrateParticles()
{
	PX_PROFILE_ZONE("cloth::SwSolverKernel::integrateParticles", 0);

	const Simd4f* startAccelIt = reinterpret_cast<const Simd4f*>(mClothData.mParticleAccelerations);

	// dt^2 (todo: should this be the smoothed dt used for gravity?)
	const Simd4f sqrIterDt = simd4f(sqr(mState.mIterDt)) & static_cast<Simd4f>(sMaskXYZ);

	if(!startAccelIt)
	{
		// no per-particle accelerations, use a constant
		ConstantIterator<Simd4f> accelIt(mState.mCurBias);
		integrateParticles(accelIt, mState.mPrevBias);
	}
	else
	{
		// iterator implicitly scales by dt^2 and adds gravity
		ScaleBiasIterator<Simd4f, const Simd4f*> accelIt(startAccelIt, sqrIterDt, mState.mCurBias);
		integrateParticles(accelIt, mState.mPrevBias);
	}
}

template <typename Simd4f>
void cloth::SwSolverKernel<Simd4f>::constrainTether()
{
	if(0.0f == mClothData.mTetherConstraintStiffness || !mClothData.mNumTethers)
		return;

	PX_PROFILE_ZONE("cloth::SwSolverKernel::solveTethers", 0);

	uint32_t numParticles = mClothData.mNumParticles;
	uint32_t numTethers = mClothData.mNumTethers;
	PX_ASSERT(0 == numTethers % numParticles);

	float* __restrict curIt = mClothData.mCurParticles;
	const float* __restrict curFirst = curIt;
	const float* __restrict curEnd = curIt + 4 * numParticles;

	typedef const SwTether* __restrict TetherIter;
	TetherIter tFirst = mClothData.mTethers;
	TetherIter tEnd = tFirst + numTethers;

	Simd4f stiffness =
	    static_cast<Simd4f>(sMaskXYZ) & simd4f(numParticles * mClothData.mTetherConstraintStiffness / numTethers);
	Simd4f scale = simd4f(mClothData.mTetherConstraintScale);

	for(; curIt != curEnd; curIt += 4, ++tFirst)
	{
		Simd4f position = loadAligned(curIt);
		Simd4f offset = gSimd4fZero;

		for(TetherIter tIt = tFirst; tIt < tEnd; tIt += numParticles)
		{
			PX_ASSERT(tIt->mAnchor < numParticles);
			Simd4f anchor = loadAligned(curFirst, tIt->mAnchor * sizeof(PxVec4));
			Simd4f delta = anchor - position;
			Simd4f sqrLength = gSimd4fEpsilon + dot3(delta, delta);

			Simd4f tetherLength = load(&tIt->mLength);
			tetherLength = splat<0>(tetherLength);

			Simd4f radius = tetherLength * scale;
			Simd4f slack = gSimd4fOne - radius * rsqrt(sqrLength);

			offset = offset + delta * max(slack, gSimd4fZero);
		}

		storeAligned(curIt, position + offset * stiffness);
	}
}

template <typename Simd4f>
void cloth::SwSolverKernel<Simd4f>::solveFabric()
{
	PX_PROFILE_ZONE("cloth::SwSolverKernel::solveFabric", 0);

	float* pIt = mClothData.mCurParticles;

	const PhaseConfig* cIt = mClothData.mConfigBegin;
	const PhaseConfig* cEnd = mClothData.mConfigEnd;

	const uint32_t* pBegin = mClothData.mPhases;
	const float* rBegin = mClothData.mRestvalues;

	const uint32_t* sBegin = mClothData.mSets;
	const uint16_t* iBegin = mClothData.mIndices;

	uint32_t totalConstraints = 0;

	Simd4f stiffnessExponent = simd4f(mCloth.mStiffnessFrequency * mState.mIterDt);

	for(; cIt != cEnd; ++cIt)
	{
		const uint32_t* sIt = sBegin + pBegin[cIt->mPhaseIndex];
		const float* rIt = rBegin + sIt[0];
		const float* rEnd = rBegin + sIt[1];
		const uint16_t* iIt = iBegin + sIt[0] * 2;

		totalConstraints += uint32_t(rEnd - rIt);

		// (stiffness, multiplier, compressionLimit, stretchLimit)
		Simd4f config = load(&cIt->mStiffness);
		// stiffness specified as fraction of constraint error per-millisecond
		Simd4f scaledConfig = gSimd4fOne - exp2(config * stiffnessExponent);
		Simd4f stiffness = select(sMaskXY, scaledConfig, config);

		int neutralMultiplier = allEqual(sMaskYZW & stiffness, gSimd4fZero);

#if PX_AVX
		switch(sAvxSupport)
		{
		case 2:
#if _MSC_VER >= 1700
			neutralMultiplier ? avx::solveConstraints<false, 2>(pIt, rIt, rEnd, iIt, stiffness)
			                  : avx::solveConstraints<true, 2>(pIt, rIt, rEnd, iIt, stiffness);
			break;
#endif
		case 1:
			neutralMultiplier ? avx::solveConstraints<false, 1>(pIt, rIt, rEnd, iIt, stiffness)
			                  : avx::solveConstraints<true, 1>(pIt, rIt, rEnd, iIt, stiffness);
			break;
		default:
#endif
			neutralMultiplier ? solveConstraints<false>(pIt, rIt, rEnd, iIt, stiffness)
			                  : solveConstraints<true>(pIt, rIt, rEnd, iIt, stiffness);
#if PX_AVX
			break;
		}
#endif
	}
}

template <typename Simd4f>
void cloth::SwSolverKernel<Simd4f>::applyWind()
{
	if(mClothData.mDragCoefficient == 0.0f && mClothData.mLiftCoefficient == 0.0f)
		return;

	PX_PROFILE_ZONE("cloth::SwSolverKernel::applyWind", 0);

	Simd4f* curIt = reinterpret_cast<Simd4f*>(mClothData.mCurParticles);
	Simd4f* prevIt = reinterpret_cast<Simd4f*>(mClothData.mPrevParticles);

	const uint16_t* tIt = mClothData.mTriangles;
	const uint16_t* tEnd = tIt + 3 * mClothData.mNumTriangles;

	Simd4f dragCoefficient = simd4f(mClothData.mDragCoefficient);
	Simd4f liftCoefficient = simd4f(mClothData.mLiftCoefficient);

	if(mState.mIsTurning)
	{
		::applyWind<true>(curIt, prevIt, tIt, tEnd, dragCoefficient, liftCoefficient, mState.mWind,
		                  mState.mRotationMatrix);
	}
	else
	{
		::applyWind<false>(curIt, prevIt, tIt, tEnd, dragCoefficient, liftCoefficient, mState.mWind,
		                   mState.mRotationMatrix);
	}
}

template <typename Simd4f>
void cloth::SwSolverKernel<Simd4f>::constrainMotion()
{
	if(!mClothData.mStartMotionConstraints)
		return;

	PX_PROFILE_ZONE("cloth::SwSolverKernel::constrainMotion", 0);

	Simd4f* curIt = reinterpret_cast<Simd4f*>(mClothData.mCurParticles);
	Simd4f* curEnd = curIt + mClothData.mNumParticles;

	const Simd4f* startIt = reinterpret_cast<const Simd4f*>(mClothData.mStartMotionConstraints);
	const Simd4f* targetIt = reinterpret_cast<const Simd4f*>(mClothData.mTargetMotionConstraints);

	Simd4f scaleBias = load(&mCloth.mMotionConstraintScale);
	Simd4f stiffness = simd4f(mClothData.mMotionConstraintStiffness);
	Simd4f scaleBiasStiffness = select(sMaskXYZ, scaleBias, stiffness);

	if(!mClothData.mTargetMotionConstraints)
		// no interpolation, use the start positions
		return ::constrainMotion(curIt, curEnd, startIt, scaleBiasStiffness);

	if(mState.mRemainingIterations == 1)
		// use the target positions on last iteration
		return ::constrainMotion(curIt, curEnd, targetIt, scaleBiasStiffness);

	// otherwise use an interpolating iterator
	LerpIterator<Simd4f, const Simd4f*> interpolator(startIt, targetIt, mState.getCurrentAlpha());
	::constrainMotion(curIt, curEnd, interpolator, scaleBiasStiffness);
}

template <typename Simd4f>
void cloth::SwSolverKernel<Simd4f>::constrainSeparation()
{
	if(!mClothData.mStartSeparationConstraints)
		return;

	PX_PROFILE_ZONE("cloth::SwSolverKernel::constrainSeparation", 0);

	Simd4f* curIt = reinterpret_cast<Simd4f*>(mClothData.mCurParticles);
	Simd4f* curEnd = curIt + mClothData.mNumParticles;

	const Simd4f* startIt = reinterpret_cast<const Simd4f*>(mClothData.mStartSeparationConstraints);
	const Simd4f* targetIt = reinterpret_cast<const Simd4f*>(mClothData.mTargetSeparationConstraints);

	if(!mClothData.mTargetSeparationConstraints)
		// no interpolation, use the start positions
		return ::constrainSeparation(curIt, curEnd, startIt);

	if(mState.mRemainingIterations == 1)
		// use the target positions on last iteration
		return ::constrainSeparation(curIt, curEnd, targetIt);

	// otherwise use an interpolating iterator
	LerpIterator<Simd4f, const Simd4f*> interpolator(startIt, targetIt, mState.getCurrentAlpha());
	::constrainSeparation(curIt, curEnd, interpolator);
}

template <typename Simd4f>
void cloth::SwSolverKernel<Simd4f>::collideParticles()
{
	PX_PROFILE_ZONE("cloth::SwSolverKernel::collideParticles", 0);

	mCollision(mState);
}

template <typename Simd4f>
void cloth::SwSolverKernel<Simd4f>::selfCollideParticles()
{
	PX_PROFILE_ZONE("cloth::SwSolverKernel::selfCollideParticles", 0);

	mSelfCollision();
}

template <typename Simd4f>
void cloth::SwSolverKernel<Simd4f>::updateSleepState()
{
	PX_PROFILE_ZONE("cloth::SwSolverKernel::updateSleepState", 0);

	mClothData.mSleepTestCounter += PxMax(1u, uint32_t(mState.mIterDt * 1000));
	if(mClothData.mSleepTestCounter >= mCloth.mSleepTestInterval)
	{
		const Simd4f* prevIt = reinterpret_cast<Simd4f*>(mClothData.mPrevParticles);
		const Simd4f* curIt = reinterpret_cast<Simd4f*>(mClothData.mCurParticles);
		const Simd4f* curEnd = curIt + mClothData.mNumParticles;

		// calculate max particle delta since last iteration
		Simd4f maxDelta = calculateMaxDelta(prevIt, curIt, curEnd);

		++mClothData.mSleepPassCounter;
		Simd4f threshold = simd4f(mCloth.mSleepThreshold * mState.mIterDt);
		if(anyGreaterEqual(maxDelta, threshold))
			mClothData.mSleepPassCounter = 0;

		mClothData.mSleepTestCounter -= mCloth.mSleepTestInterval;
	}
}

template <typename Simd4f>
void cloth::SwSolverKernel<Simd4f>::iterateCloth()
{
	// note on invMass (stored in current/previous positions.w):
	// integrateParticles()
	//   - if(current.w == 0) current.w = previous.w
	// constraintMotion()
	//   - if(constraint.radius <= 0) current.w = 0
	// computeBounds()
	//   - if(current.w > 0) current.w = previous.w
	// collideParticles()
	//   - if(collides) current.w *= 1/massScale
	// after simulate()
	//   - previous.w: original invMass as set by user
	//   - current.w: zeroed by motion constraints and mass-scaled by collision

	// integrate positions
	integrateParticles();

	// apply drag and lift
	applyWind();

	// motion constraints
	constrainMotion();

	// solve tether constraints
	constrainTether();

	// solve edge constraints
	solveFabric();

	// separation constraints
	constrainSeparation();

	// perform character collision
	collideParticles();

	// perform self collision
	selfCollideParticles();

	// test wake / sleep conditions
	updateSleepState();
}

template <typename Simd4f>
void cloth::SwSolverKernel<Simd4f>::simulateCloth()
{
	while(mState.mRemainingIterations)
	{
		iterateCloth();
		mState.update();
	}
}

// explicit template instantiation
#if NV_SIMD_SIMD
template class cloth::SwSolverKernel<Simd4f>;
#endif
#if NV_SIMD_SCALAR
template class cloth::SwSolverKernel<Scalar4f>;
#endif
