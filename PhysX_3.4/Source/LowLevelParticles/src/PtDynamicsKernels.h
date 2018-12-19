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

#ifndef PT_DYNAMICS_KERNELS_H
#define PT_DYNAMICS_KERNELS_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "PsFPU.h"
#include "foundation/PxUnionCast.h"

#include "PtDynamicsParameters.h"

#define REFERENCE_KERNELS 0

#if !REFERENCE_KERNELS
#include "PsVecMath.h"
#endif

namespace physx
{
namespace Pt
{

using namespace Ps;
using namespace aos;

#define COMPILE_IN_SIMD_DENSITY 1
#define PX_FORCE_INLINE_KERNELS PX_FORCE_INLINE

#define MAX_INDEX_STREAM_SIZE 128
#define PRESSURE_ORIGIN 1

PX_FORCE_INLINE PxF32 calcDensity(const PxF32 distSqr, const DynamicsParameters& params)
{
	PxF32 dist2Std = distSqr * params.scaleSqToStd;
	PxF32 radius2MinusDist2Std = params.radiusSqStd - dist2Std;
	PxF32 densityStd = params.densityMultiplierStd * radius2MinusDist2Std * radius2MinusDist2Std * radius2MinusDist2Std;
	return densityStd;
}

PX_FORCE_INLINE void addDensity(Particle& particleDst, const PxF32 distSqr, const DynamicsParameters& params)
{
	PX_ASSERT(distSqr <= params.cellSizeSq);
	PxF32 densityStd = calcDensity(distSqr, params);
	particleDst.density += densityStd;
}

PX_FORCE_INLINE void addDensity_twoWay(Particle& particleA, Particle& particleB, const PxF32 distSqr,
                                       const DynamicsParameters& params)
{
	PX_ASSERT(distSqr <= params.cellSizeSq);
	PxF32 densityStd = calcDensity(distSqr, params);
	particleA.density += densityStd;
	particleB.density += densityStd;
}

PX_FORCE_INLINE PxVec3 calcForce(const Particle& particleA, const Particle& particleB, const PxF32 distSqr,
                                 const PxVec3& distVec, const DynamicsParameters& params)
{
	PxReal dist2Std = distSqr * params.scaleSqToStd;

	PxReal recipDistStd = physx::intrinsics::recipSqrtFast(dist2Std);
	PxReal distStd = dist2Std * recipDistStd;

	PxReal radiusMinusDistStd = params.radiusStd - distStd;

// pressure force
#if PRESSURE_ORIGIN
	PxF32 pressureA = PxMax(particleA.density - params.initialDensity, 0.0f);
	PxF32 pressureB = PxMax(particleB.density - params.initialDensity, 0.0f);
	PxF32 pressureSum = pressureA + pressureB;
#else
	PxF32 pressureSum = PxMax(particleA.density + particleB.density - 2 * params.initialDensity, 0.0f);
#endif

	PxReal multiplierPressStd = (params.radiusSqStd * recipDistStd - 2 * params.radiusStd + distStd) *
	                            params.stiffMulPressureMultiplierStd * pressureSum;

	PxVec3 force = distVec * multiplierPressStd * params.scaleToStd;

	// viscosity force
	PxReal multiplierViscStd = radiusMinusDistStd * params.viscosityMultiplierStd;

	PxVec3 vDiff = (particleB.velocity - particleA.velocity) * params.scaleToStd;
	force += (vDiff * multiplierViscStd);

	return force;
}

PX_FORCE_INLINE void addForce(PxVec3& particleForceDst, const Particle& particleDst, const Particle& particleSrc,
                              const PxF32 distSqr, const PxVec3& distVec, const DynamicsParameters& params)
{
	PX_ASSERT(distSqr <= params.cellSizeSq);
	PxVec3 force = calcForce(particleDst, particleSrc, distSqr, distVec, params);
	particleForceDst += (force * physx::intrinsics::recipFast(particleSrc.density));
}

PX_FORCE_INLINE void addForce_twoWay(PxVec3& particleAForce, PxVec3& particleBForce, const Particle& particleA,
                                     const Particle& particleB, const PxF32 distSqr, const PxVec3& distVec,
                                     const DynamicsParameters& params)
{
	PX_ASSERT(distSqr <= params.cellSizeSq);
	PxVec3 force = calcForce(particleA, particleB, distSqr, distVec, params);
	particleAForce += (force * physx::intrinsics::recipFast(particleB.density));
	particleBForce -= (force * physx::intrinsics::recipFast(particleA.density));
}

#if REFERENCE_KERNELS

PX_FORCE_INLINE void updateParticleGroupPair(PxVec3* __restrict forceBufA, PxVec3* __restrict forceBufB,
                                             Particle* __restrict particlesSpA, Particle* __restrict particlesSpB,
                                             const PxU32* __restrict particleIndicesSpA, const PxU32 numParticlesA,
                                             const PxU32* __restrict particleIndicesSpB, const PxU32 numParticlesB,
                                             const bool twoWayUpdate, const bool isDensityMode,
                                             const DynamicsParameters& params, PxU8* tempSimdPositionBuffer,
                                             PxU32* tempIndexStream)
{
	// Check given particle against particles of another cell.

	for(PxU32 pA = 0; pA < numParticlesA; pA++)
	{
		PxU32 idxA = particleIndicesSpA[pA];
		Particle& particleA = particlesSpA[idxA];
		PxVec3& forceA = forceBufA[idxA];

		for(PxU32 pB = 0; pB < numParticlesB; pB++)
		{
			PxU32 idxB = particleIndicesSpB[pB];
			Particle& particleB = particlesSpB[idxB];
			PxVec3& forceB = forceBufB[idxB];

			PxVec3 distVec = particleA.position - particleB.position;
			PxReal distSqr = distVec.magnitudeSquared();

			if(distSqr < params.cellSizeSq && distSqr > 0.0f)
			{
				if(isDensityMode)
				{
					if(!twoWayUpdate)
						addDensity(particleA, distSqr, params);
					else
						addDensity_twoWay(particleA, particleB, distSqr, params);
				}
				else
				{
					if(!twoWayUpdate)
						addForce(forceA, particleA, particleB, distSqr, distVec, params);
					else
						addForce_twoWay(forceA, forceB, particleA, particleB, distSqr, distVec, params);
				}
			}
		}
	}
}

#else // REFERENCE_KERNELS

class DensityPassType
{
};
class ForcePassType
{
};
class TwoWayUpdateType
{
};
class OneWayUpdateType
{
};

template <typename PassType, typename UpdateType>
struct Contribution
{
};

template <>
struct Contribution<DensityPassType, TwoWayUpdateType>
{
	static void add(PxVec3&, PxVec3&, PxReal distSqr, const PxVec3&, Particle& particleA, Particle& particleB,
	                const DynamicsParameters& params)
	{
		addDensity_twoWay(particleA, particleB, distSqr, params);
	}
};

template <>
struct Contribution<ForcePassType, TwoWayUpdateType>
{
	static void add(PxVec3& forceA, PxVec3& forceB, PxReal distSqr, const PxVec3& distVec, Particle& particleA,
	                Particle& particleB, const DynamicsParameters& params)
	{
		addForce_twoWay(forceA, forceB, particleA, particleB, distSqr, distVec, params);
	}
};

template <>
struct Contribution<DensityPassType, OneWayUpdateType>
{
	static void add(PxVec3&, PxVec3&, PxReal distSqr, const PxVec3&, Particle& particleA, Particle&,
	                const DynamicsParameters& params)
	{
		addDensity(particleA, distSqr, params);
	}
};

template <>
struct Contribution<ForcePassType, OneWayUpdateType>
{
	static void add(PxVec3& forceA, PxVec3&, PxReal distSqr, const PxVec3& distVec, Particle& particleA,
	                Particle& particleB, const DynamicsParameters& params)
	{
		addForce(forceA, particleA, particleB, distSqr, distVec, params);
	}
};

// Parameters for simd kernel execution
struct DynamicsParametersSIMD
{
	Ps::aos::Vec4V scaleToStd;
	Ps::aos::Vec4V scaleSqToStd;
	Ps::aos::Vec4V radiusStd;
	Ps::aos::Vec4V radiusSqStd;
	Ps::aos::Vec4V densityMultiplierStd;
	Ps::aos::Vec4V stiffMulPressureMultiplierStd;
	Ps::aos::Vec4V viscosityMultiplierStd;
	Ps::aos::Vec4V initialDensity;
	Ps::aos::Vec4V stiffnessStd;
};

#if COMPILE_IN_SIMD_DENSITY

PX_FORCE_INLINE void calcDensity4_onlyPtrs(Mat44V& posDensDstT, const Particle* __restrict pSrc0,
                                           const Particle* __restrict pSrc1, const Particle* __restrict pSrc2,
                                           const Particle* __restrict pSrc3, const DynamicsParametersSIMD& params)
{
	Ps::aos::Mat44V posDensSrc(V4LoadA(&pSrc0->position.x), V4LoadA(&pSrc1->position.x), V4LoadA(&pSrc2->position.x),
	                           V4LoadA(&pSrc3->position.x));

	Mat44V posDensSrcT = M44Trnsps(posDensSrc);

	Vec4V distVec_x = V4Sub(posDensDstT.col0, posDensSrcT.col0);
	Vec4V distVec_y = V4Sub(posDensDstT.col1, posDensSrcT.col1);
	Vec4V distVec_z = V4Sub(posDensDstT.col2, posDensSrcT.col2);

	Vec4V distSqr_x = V4Mul(distVec_x, distVec_x);
	Vec4V distSqr_xy = V4MulAdd(distVec_y, distVec_y, distSqr_x);
	Vec4V distSqr = V4MulAdd(distVec_z, distVec_z, distSqr_xy);

	Vec4V distSqrStd = V4Mul(distSqr, params.scaleSqToStd);

	Vec4V radius2MinusDist2Std = V4Sub(params.radiusSqStd, distSqrStd);
	Vec4V densityStd = V4Mul(params.densityMultiplierStd, radius2MinusDist2Std);
	densityStd = V4Mul(densityStd, radius2MinusDist2Std);
	densityStd = V4Mul(densityStd, radius2MinusDist2Std);

	posDensDstT.col3 = V4Add(posDensDstT.col3, densityStd);
}

PX_FORCE_INLINE void calcDensity4_twoWay_onlyPtrs(Mat44V& posDensDstT, Particle* __restrict pSrc0,
                                                  Particle* __restrict pSrc1, Particle* __restrict pSrc2,
                                                  Particle* __restrict pSrc3, const DynamicsParametersSIMD& params)
{
	Mat44V posDensSrc(V4LoadA(&pSrc0->position.x), V4LoadA(&pSrc1->position.x), V4LoadA(&pSrc2->position.x),
	                  V4LoadA(&pSrc3->position.x));

	Mat44V posDensSrcT = M44Trnsps(posDensSrc);

	Vec4V distVec_x = V4Sub(posDensDstT.col0, posDensSrcT.col0);
	Vec4V distVec_y = V4Sub(posDensDstT.col1, posDensSrcT.col1);
	Vec4V distVec_z = V4Sub(posDensDstT.col2, posDensSrcT.col2);

	Vec4V distSqr_x = V4Mul(distVec_x, distVec_x);
	Vec4V distSqr_xy = V4MulAdd(distVec_y, distVec_y, distSqr_x);
	Vec4V distSqr = V4MulAdd(distVec_z, distVec_z, distSqr_xy);

	Vec4V distSqrStd = V4Mul(distSqr, params.scaleSqToStd);

	Vec4V radius2MinusDist2Std = V4Sub(params.radiusSqStd, distSqrStd);
	Vec4V densityStd = V4Mul(params.densityMultiplierStd, radius2MinusDist2Std);
	densityStd = V4Mul(densityStd, radius2MinusDist2Std);
	densityStd = V4Mul(densityStd, radius2MinusDist2Std);

	// apply to srcParticles (sschirm TOTO rename)
	PX_ALIGN(16, PxVec4 density);
	V4StoreA(densityStd, &density[0]);
	pSrc0->density += density[0];
	pSrc1->density += density[1];
	pSrc2->density += density[2];
	pSrc3->density += density[3];

	// apply to dstParticle (sschirm TOTO rename)
	posDensDstT.col3 = V4Add(posDensDstT.col3, densityStd);
}

#endif // COMPILE_IN_SIMD_DENSITY

PX_FORCE_INLINE void calcForce4_onlyPtrs(Mat44V& forceDstT, const Particle* __restrict pSrc0,
                                         const Particle* __restrict pSrc1, const Particle* __restrict pSrc2,
                                         const Particle* __restrict pSrc3, const Mat44V& posDensDstT,
                                         const Mat44V& velPressDstT, const DynamicsParametersSIMD& params)
{
	Mat44V posDensSrc(V4LoadA(&pSrc0->position.x), V4LoadA(&pSrc1->position.x), V4LoadA(&pSrc2->position.x),
	                  V4LoadA(&pSrc3->position.x));

	Mat44V posDensSrcT = M44Trnsps(posDensSrc);

	Vec4V distVec_x = V4Sub(posDensDstT.col0, posDensSrcT.col0);
	Vec4V distVec_y = V4Sub(posDensDstT.col1, posDensSrcT.col1);
	Vec4V distVec_z = V4Sub(posDensDstT.col2, posDensSrcT.col2);

	Vec4V distSqr_x = V4Mul(distVec_x, distVec_x);
	Vec4V distSqr_xy = V4MulAdd(distVec_y, distVec_y, distSqr_x);
	Vec4V distSqr = V4MulAdd(distVec_z, distVec_z, distSqr_xy);

	Vec4V distSqrStd = V4Mul(distSqr, params.scaleSqToStd);

	Vec4V recipDistStd = V4RsqrtFast(distSqrStd);
	Vec4V distStd = V4Mul(distSqrStd, recipDistStd);
	Vec4V radiusMinusDistStd = V4Sub(params.radiusStd, distStd);

	// pressure force
	Mat44V velPressSrc(V4LoadA(&pSrc0->velocity.x), V4LoadA(&pSrc1->velocity.x), V4LoadA(&pSrc2->velocity.x),
	                   V4LoadA(&pSrc3->velocity.x));

	Mat44V velPressSrcT = M44Trnsps(velPressSrc);

	Vec4V pressureDst = V4Sub(posDensDstT.col3, params.initialDensity);
	Vec4V pressureSrc = V4Sub(posDensSrcT.col3, params.initialDensity);
#if PRESSURE_ORIGIN
	pressureDst = V4Max(pressureDst, V4Zero());
	pressureSrc = V4Max(pressureSrc, V4Zero());
	Vec4V pressureSum = V4Add(pressureDst, pressureSrc);
#else
	Vec4V pressureSum = V4Add(pressureDst, pressureSrc);
	pressureSum = V4Max(pressureSum, V4Zero());
#endif

	Vec4V radiusStd_x2 = V4Add(params.radiusStd, params.radiusStd);
	Vec4V multiplierPressStd = V4MulAdd(params.radiusSqStd, recipDistStd, distStd);
	multiplierPressStd = V4Sub(multiplierPressStd, radiusStd_x2);
	multiplierPressStd = V4Mul(multiplierPressStd, params.stiffMulPressureMultiplierStd);
	multiplierPressStd = V4Mul(multiplierPressStd, pressureSum);

	Vec4V pressureForceMult = V4Mul(multiplierPressStd, params.scaleToStd);
	Vec4V force_x = V4Mul(distVec_x, pressureForceMult);
	Vec4V force_y = V4Mul(distVec_y, pressureForceMult);
	Vec4V force_z = V4Mul(distVec_z, pressureForceMult);

	// viscosity force
	Vec4V multiplierViscStd = V4Mul(radiusMinusDistStd, params.viscosityMultiplierStd);

	Vec4V viscossityForceMult = V4Mul(params.scaleToStd, multiplierViscStd);

	Vec4V vDiff_x = V4Sub(velPressSrcT.col0, velPressDstT.col0);
	Vec4V vDiff_y = V4Sub(velPressSrcT.col1, velPressDstT.col1);
	Vec4V vDiff_z = V4Sub(velPressSrcT.col2, velPressDstT.col2);

	force_x = V4MulAdd(vDiff_x, viscossityForceMult, force_x);
	force_y = V4MulAdd(vDiff_y, viscossityForceMult, force_y);
	force_z = V4MulAdd(vDiff_z, viscossityForceMult, force_z);

	// application of force
	Vec4V invDensities = V4RecipFast(posDensSrcT.col3);
	force_x = V4Mul(force_x, invDensities);
	force_y = V4Mul(force_y, invDensities);
	force_z = V4Mul(force_z, invDensities);

	forceDstT.col0 = V4Add(forceDstT.col0, force_x);
	forceDstT.col1 = V4Add(forceDstT.col1, force_y);
	forceDstT.col2 = V4Add(forceDstT.col2, force_z);
}

PX_FORCE_INLINE void calcForce4_twoWay_onlyPtrs(Mat44V& forceDstT, Mat44V& forceSrcT, Particle* __restrict pSrc0,
                                                Particle* __restrict pSrc1, Particle* __restrict pSrc2,
                                                Particle* __restrict pSrc3, const Mat44V& posDensDstT,
                                                const Mat44V& velPressDstT, const Vec4V& invDensityDst,
                                                const DynamicsParametersSIMD& params)
{
	Mat44V posDensSrc(V4LoadA(&pSrc0->position.x), V4LoadA(&pSrc1->position.x), V4LoadA(&pSrc2->position.x),
	                  V4LoadA(&pSrc3->position.x));

	Mat44V posDensSrcT = M44Trnsps(posDensSrc);

	Vec4V distVec_x = V4Sub(posDensDstT.col0, posDensSrcT.col0);
	Vec4V distVec_y = V4Sub(posDensDstT.col1, posDensSrcT.col1);
	Vec4V distVec_z = V4Sub(posDensDstT.col2, posDensSrcT.col2);

	Vec4V distSqr_x = V4Mul(distVec_x, distVec_x);
	Vec4V distSqr_xy = V4MulAdd(distVec_y, distVec_y, distSqr_x);
	Vec4V distSqr = V4MulAdd(distVec_z, distVec_z, distSqr_xy);

	Vec4V distSqrStd = V4Mul(distSqr, params.scaleSqToStd);

	Vec4V recipDistStd = V4RsqrtFast(distSqrStd);
	Vec4V distStd = V4Mul(distSqrStd, recipDistStd);
	Vec4V radiusMinusDistStd = V4Sub(params.radiusStd, distStd);

	// pressure force
	Mat44V velPressSrc(V4LoadA(&pSrc0->velocity.x), V4LoadA(&pSrc1->velocity.x), V4LoadA(&pSrc2->velocity.x),
	                   V4LoadA(&pSrc3->velocity.x));

	Mat44V velPressSrcT = M44Trnsps(velPressSrc);

	Vec4V pressureDst = V4Sub(posDensDstT.col3, params.initialDensity);
	Vec4V pressureSrc = V4Sub(posDensSrcT.col3, params.initialDensity);
#if PRESSURE_ORIGIN
	pressureDst = V4Max(pressureDst, V4Zero());
	pressureSrc = V4Max(pressureSrc, V4Zero());
	Vec4V pressureSum = V4Add(pressureDst, pressureSrc);
#else
	Vec4V pressureSum = V4Add(pressureDst, pressureSrc);
	pressureSum = V4Max(pressureSum, V4Zero());
#endif

	Vec4V radiusStd_x2 = V4Add(params.radiusStd, params.radiusStd);
	Vec4V multiplierPressStd = V4MulAdd(params.radiusSqStd, recipDistStd, distStd);
	multiplierPressStd = V4Sub(multiplierPressStd, radiusStd_x2);
	multiplierPressStd = V4Mul(multiplierPressStd, params.stiffMulPressureMultiplierStd);
	multiplierPressStd = V4Mul(multiplierPressStd, pressureSum);

	Vec4V pressureForceMult = V4Mul(multiplierPressStd, params.scaleToStd);
	Vec4V force_x = V4Mul(distVec_x, pressureForceMult);
	Vec4V force_y = V4Mul(distVec_y, pressureForceMult);
	Vec4V force_z = V4Mul(distVec_z, pressureForceMult);

	// viscosity force
	Vec4V multiplierViscStd = V4Mul(radiusMinusDistStd, params.viscosityMultiplierStd);

	Vec4V viscossityForceMult = V4Mul(params.scaleToStd, multiplierViscStd);

	Vec4V vDiff_x = V4Sub(velPressSrcT.col0, velPressDstT.col0);
	Vec4V vDiff_y = V4Sub(velPressSrcT.col1, velPressDstT.col1);
	Vec4V vDiff_z = V4Sub(velPressSrcT.col2, velPressDstT.col2);

	force_x = V4MulAdd(vDiff_x, viscossityForceMult, force_x);
	force_y = V4MulAdd(vDiff_y, viscossityForceMult, force_y);
	force_z = V4MulAdd(vDiff_z, viscossityForceMult, force_z);

	// apply to src particles (sschirm TODO:rename)
	forceSrcT.col0 = V4NegMulSub(force_x, invDensityDst, forceSrcT.col0);
	forceSrcT.col1 = V4NegMulSub(force_y, invDensityDst, forceSrcT.col1);
	forceSrcT.col2 = V4NegMulSub(force_z, invDensityDst, forceSrcT.col2);

	// apply to dst particle (sschirm TODO:rename)
	Vec4V invDensities = V4RecipFast(posDensSrcT.col3);
	forceDstT.col0 = V4MulAdd(force_x, invDensities, forceDstT.col0);
	forceDstT.col1 = V4MulAdd(force_y, invDensities, forceDstT.col1);
	forceDstT.col2 = V4MulAdd(force_z, invDensities, forceDstT.col2);
}

#if !PX_IOS

static void updateStreamDensity(Particle* __restrict particlesA, const Particle* __restrict particlesB,
                                const PxU32* indexStream, const PxU32 indexStreamSize, const DynamicsParameters& params,
                                const DynamicsParametersSIMD& simdParams)
{
	PX_UNUSED(simdParams);
	PxU32 s = 0;
	while(s < indexStreamSize)
	{
		PxU32 dstIdx = indexStream[s++];
		PxU32 numInteractions = indexStream[s++];

		// the simd density code is currently disabled, since it's not a real win
		if(1)
		{
			for(PxU32 i = 0; i < numInteractions; ++i)
			{
				PxU32 srcIdx = indexStream[s++];
				PX_ALIGN(16, PxVec3 distVec) = particlesA[dstIdx].position - particlesB[srcIdx].position;
				PxF32 distSqr = distVec.magnitudeSquared();
				addDensity(particlesA[dstIdx], distSqr, params);
			}
		}
#if COMPILE_IN_SIMD_DENSITY
		else
		{
			Particle* __restrict dstParticle = particlesA + dstIdx;
			PxU32 blockCount = numInteractions / 4;

			if(blockCount > 0)
			{
				Vec4V tmp = V4LoadA(&dstParticle->position.x);
				Mat44V posDensDst(tmp, tmp, tmp, tmp);
				Mat44V posDensDstT = M44Trnsps(posDensDst);

				// set density to zero
				posDensDstT.col3 = V4Zero();

				for(PxU32 i = 0; i < blockCount; ++i)
				{
					PxU32 srcIdx0 = indexStream[s++];
					PxU32 srcIdx1 = indexStream[s++];
					PxU32 srcIdx2 = indexStream[s++];
					PxU32 srcIdx3 = indexStream[s++];

					calcDensity4_onlyPtrs(posDensDstT, particlesB + srcIdx0, particlesB + srcIdx1, particlesB + srcIdx2,
					                      particlesB + srcIdx3, simdParams);
				}

				// simd to scalar
				PX_ALIGN(16, PxVec4 density);
				V4StoreA(posDensDstT.col3, &density[0]);
				dstParticle->density += density[0] + density[1] + density[2] + density[3];
			}

			PxU32 numLeft = numInteractions - blockCount * 4;
			for(PxU32 i = 0; i < numLeft; ++i)
			{
				PxU32 srcIdx = indexStream[s++];

				PX_ALIGN(16, PxVec3) distVec = particlesA[dstIdx].position - particlesB[srcIdx].position;
				PxF32 distSqr = distVec.magnitudeSquared();
				addDensity(particlesA[dstIdx], distSqr, params);
			}
		}
#endif // COMPILE_IN_SIMD_DENSITY
	}
}

static void updateStreamDensityTwoWay(Particle* __restrict particlesA, Particle* __restrict particlesB,
                                      const PxU32* indexStream, const PxU32 indexStreamSize,
                                      const DynamicsParameters& params, const DynamicsParametersSIMD& simdParams)
{
	PX_UNUSED(simdParams);
	PxU32 s = 0;
	while(s < indexStreamSize)
	{
		PxU32 dstIdx = indexStream[s++];
		PxU32 numInteractions = indexStream[s++];

		// the simd density code is currently disabled, since it's not a real win
		if(1)
		{
			for(PxU32 i = 0; i < numInteractions; ++i)
			{
				PxU32 srcIdx = indexStream[s++];
				PX_ALIGN(16, PxVec3) distVec = particlesA[dstIdx].position - particlesB[srcIdx].position;
				PxF32 distSqr = distVec.magnitudeSquared();
				addDensity_twoWay(particlesA[dstIdx], particlesB[srcIdx], distSqr, params);
			}
		}
#if COMPILE_IN_SIMD_DENSITY
		else
		{
			Particle* __restrict dstParticle = particlesA + dstIdx;
			PxU32 blockCount = numInteractions / 4;

			if(blockCount > 0)
			{
				Vec4V tmp = V4LoadA(&dstParticle->position.x);
				Mat44V posDensDst(tmp, tmp, tmp, tmp);
				Mat44V posDensDstT = M44Trnsps(posDensDst);

				// set density to zero
				posDensDstT.col3 = V4Zero();

				for(PxU32 i = 0; i < blockCount; ++i)
				{
					PxU32 srcIdx0 = indexStream[s++];
					PxU32 srcIdx1 = indexStream[s++];
					PxU32 srcIdx2 = indexStream[s++];
					PxU32 srcIdx3 = indexStream[s++];

					calcDensity4_twoWay_onlyPtrs(posDensDstT, particlesB + srcIdx0, particlesB + srcIdx1,
					                             particlesB + srcIdx2, particlesB + srcIdx3, simdParams);
				}

				// simd to scalar
				PX_ALIGN(16, PxVec4 density);
				V4StoreA(posDensDstT.col3, &density[0]);
				dstParticle->density += density[0] + density[1] + density[2] + density[3];
			}

			PxU32 numLeft = numInteractions - blockCount * 4;
			for(PxU32 i = 0; i < numLeft; ++i)
			{
				PxU32 srcIdx = indexStream[s++];

				PX_ALIGN(16, PxVec3 distVec) = particlesA[dstIdx].position - particlesB[srcIdx].position;
				PxF32 distSqr = distVec.magnitudeSquared();
				addDensity_twoWay(particlesA[dstIdx], particlesB[srcIdx], distSqr, params);
			}
		}
#endif // COMPILE_IN_SIMD_DENSITY
	}
}

static void updateStreamForce(PxVec3* __restrict forceBufA, Particle* __restrict particlesA,
                              const Particle* __restrict particlesB, const PxU32* indexStream,
                              const PxU32 indexStreamSize, const DynamicsParameters& params,
                              const DynamicsParametersSIMD& simdParams)
{
	PxU32 s = 0;
	while(s < indexStreamSize)
	{
		PxU32 dstIdx = indexStream[s++];
		Particle* __restrict dstParticle = particlesA + dstIdx;

		PxU32 numInteractions = indexStream[s++];
		PxU32 blockCount = numInteractions / 4;

		if(blockCount > 0)
		{
			Vec4V tmp = V4LoadA(&dstParticle->position.x);
			Mat44V posDensDst(tmp, tmp, tmp, tmp);
			Mat44V posDensDstT = M44Trnsps(posDensDst);

			Mat44V forceDstT(V4Zero(), V4Zero(), V4Zero(), V4Zero());

			tmp = V4LoadA(&dstParticle->velocity.x);
			Mat44V velPressDst(tmp, tmp, tmp, tmp);
			Mat44V velPressDstT = M44Trnsps(velPressDst);

			for(PxU32 i = 0; i < blockCount; ++i)
			{
				PxU32 srcIdx0 = indexStream[s++];
				PxU32 srcIdx1 = indexStream[s++];
				PxU32 srcIdx2 = indexStream[s++];
				PxU32 srcIdx3 = indexStream[s++];

				calcForce4_onlyPtrs(forceDstT, particlesB + srcIdx0, particlesB + srcIdx1, particlesB + srcIdx2,
				                    particlesB + srcIdx3, posDensDstT, velPressDstT, simdParams);
			}

			// simd to scalar
			Mat44V forceDst = M44Trnsps(forceDstT);
			Vec4V forceTmp1 = V4Add(forceDst.col0, forceDst.col1);
			Vec4V forceTmp2 = V4Add(forceDst.col2, forceDst.col3);
			forceTmp1 = V4Add(forceTmp1, forceTmp2);
			forceBufA[dstIdx] += V4ReadXYZ(forceTmp1);
		}

		PxU32 numLeft = numInteractions - blockCount * 4;
		for(PxU32 i = 0; i < numLeft; ++i)
		{
			PxU32 srcIdx = indexStream[s++];

			PX_ALIGN(16, PxVec3 distVec) = particlesA[dstIdx].position - particlesB[srcIdx].position;
			PxF32 distSqr = distVec.magnitudeSquared();
			addForce(forceBufA[dstIdx], particlesA[dstIdx], particlesB[srcIdx], distSqr, distVec, params);
		}
	}
}

static void updateStreamForceTwoWay(PxVec3* __restrict forceBufA, PxVec3* __restrict forceBufB,
                                    Particle* __restrict particlesA, Particle* __restrict particlesB,
                                    const PxU32* indexStream, const PxU32 indexStreamSize,
                                    const DynamicsParameters& params, const DynamicsParametersSIMD& simdParams)
{
	PX_ASSERT(forceBufB);
	PxU32 s = 0;
	while(s < indexStreamSize)
	{
		PxU32 dstIdx = indexStream[s++];
		Particle* __restrict dstParticle = particlesA + dstIdx;

		PxU32 numInteractions = indexStream[s++];
		PxU32 blockCount = numInteractions / 4;

		if(blockCount > 0)
		{
			Vec4V tmp = V4LoadA(&dstParticle->position.x);
			Mat44V posDensDst(tmp, tmp, tmp, tmp);
			Mat44V posDensDstT = M44Trnsps(posDensDst);

			Mat44V forceDstT(V4Zero(), V4Zero(), V4Zero(), V4Zero());

			tmp = V4LoadA(&dstParticle->velocity.x);
			Mat44V velPressDst(tmp, tmp, tmp, tmp);
			Mat44V velPressDstT = M44Trnsps(velPressDst);

			tmp = V4Load(dstParticle->density);
			Vec4V invDensityA = V4RecipFast(tmp);

			for(PxU32 i = 0; i < blockCount; ++i)
			{
				PxU32 srcIdx0 = indexStream[s++];
				PxU32 srcIdx1 = indexStream[s++];
				PxU32 srcIdx2 = indexStream[s++];
				PxU32 srcIdx3 = indexStream[s++];

				Vec4V tmp0 = Vec4V_From_Vec3V(V3LoadU(&forceBufB[srcIdx0].x));
				Vec4V tmp1 = Vec4V_From_Vec3V(V3LoadU(&forceBufB[srcIdx1].x));
				Vec4V tmp2 = Vec4V_From_Vec3V(V3LoadU(&forceBufB[srcIdx2].x));
				Vec4V tmp3 = Vec4V_From_Vec3V(V3LoadU(&forceBufB[srcIdx3].x));
				Mat44V forceSrc(tmp0, tmp1, tmp2, tmp3);
				Mat44V forceSrcT = M44Trnsps(forceSrc);

				calcForce4_twoWay_onlyPtrs(forceDstT, forceSrcT, particlesB + srcIdx0, particlesB + srcIdx1,
				                           particlesB + srcIdx2, particlesB + srcIdx3, posDensDstT, velPressDstT,
				                           invDensityA, simdParams);

				forceSrc = M44Trnsps(forceSrcT);
				forceBufB[srcIdx0] = V4ReadXYZ(forceSrc.col0);
				forceBufB[srcIdx1] = V4ReadXYZ(forceSrc.col1);
				forceBufB[srcIdx2] = V4ReadXYZ(forceSrc.col2);
				forceBufB[srcIdx3] = V4ReadXYZ(forceSrc.col3);
			}

			// simd to scalar
			Mat44V forceDst = M44Trnsps(forceDstT);
			Vec4V forceTmp1 = V4Add(forceDst.col0, forceDst.col1);
			Vec4V forceTmp2 = V4Add(forceDst.col2, forceDst.col3);
			forceTmp1 = V4Add(forceTmp1, forceTmp2);
			forceBufA[dstIdx] += V4ReadXYZ(forceTmp1);
		}

		PxU32 numLeft = numInteractions - blockCount * 4;
		for(PxU32 i = 0; i < numLeft; ++i)
		{
			PxU32 srcIdx = indexStream[s++];

			PX_ALIGN(16, PxVec3 distVec) = particlesA[dstIdx].position - particlesB[srcIdx].position;
			PxF32 distSqr = distVec.magnitudeSquared();
			addForce_twoWay(forceBufA[dstIdx], forceBufB[srcIdx], particlesA[dstIdx], particlesB[srcIdx], distSqr,
			                distVec, params);
		}
	}
}

#endif // !PX_IOS

template <typename PassType, typename UpdateType>
PX_FORCE_INLINE_KERNELS static void updateParticleGroupPair_small_template(
    PxVec3* __restrict forceBufA, PxVec3* __restrict forceBufB, Particle* __restrict particlesA,
    Particle* __restrict particlesB, const PxU32* __restrict particleIndicesA, const PxU32 numParticlesA,
    const PxU32* __restrict particleIndicesB, const PxU32 numParticlesB, const DynamicsParameters& params)
{
	PxU32 num_loopB = 4 * (numParticlesB / 4);
	PxU32 u_cellSizeSq = PxUnionCast<PxU32, PxF32>(params.cellSizeSq);

	for(PxU32 pA = 0; pA < numParticlesA; pA++)
	{
		PxU32 idxA = particleIndicesA[pA];
		Particle& particleA = particlesA[idxA];
		PxVec3& forceA = forceBufA[idxA];

		for(PxU32 pB = 0; pB < num_loopB; pB += 4)
		{
			PxU32 idxB0 = particleIndicesB[pB];
			PxU32 idxB1 = particleIndicesB[pB + 1];
			PxU32 idxB2 = particleIndicesB[pB + 2];
			PxU32 idxB3 = particleIndicesB[pB + 3];

			Particle& particleB0 = particlesB[idxB0];
			Particle& particleB1 = particlesB[idxB1];
			Particle& particleB2 = particlesB[idxB2];
			Particle& particleB3 = particlesB[idxB3];

			PxVec3& forceB0 = forceBufB[idxB0];
			PxVec3& forceB1 = forceBufB[idxB1];
			PxVec3& forceB2 = forceBufB[idxB2];
			PxVec3& forceB3 = forceBufB[idxB3];

			PX_ALIGN(16, PxVec3 distVec0) = particleA.position - particleB0.position;
			PX_ALIGN(16, PxVec3 distVec1) = particleA.position - particleB1.position;
			PX_ALIGN(16, PxVec3 distVec2) = particleA.position - particleB2.position;
			PX_ALIGN(16, PxVec3 distVec3) = particleA.position - particleB3.position;

			PxReal distSqr0 = distVec0.magnitudeSquared();
			PxReal distSqr1 = distVec1.magnitudeSquared();
			PxReal distSqr2 = distVec2.magnitudeSquared();
			PxReal distSqr3 = distVec3.magnitudeSquared();

			// marginally faster to do that test (not as good as in brute force)
			PxF32 isec = physx::intrinsics::fsel(params.cellSizeSq - distSqr0, 1.0f, 0.0f);
			isec = physx::intrinsics::fsel(params.cellSizeSq - distSqr1, 1.0f, isec);
			isec = physx::intrinsics::fsel(params.cellSizeSq - distSqr2, 1.0f, isec);
			isec = physx::intrinsics::fsel(params.cellSizeSq - distSqr3, 1.0f, isec);

			if(isec == 0.0f)
				continue;

			PxU32 u_distSqr0 = PxUnionCast<PxU32, PxReal>(distSqr0);
			PxU32 u_distSqr1 = PxUnionCast<PxU32, PxReal>(distSqr1);
			PxU32 u_distSqr2 = PxUnionCast<PxU32, PxReal>(distSqr2);
			PxU32 u_distSqr3 = PxUnionCast<PxU32, PxReal>(distSqr3);

			if(u_distSqr0 < u_cellSizeSq && u_distSqr0 > 0)
			{
				Contribution<PassType, UpdateType>::add(forceA, forceB0, distSqr0, distVec0, particleA, particleB0,
				                                        params);
			}
			if(u_distSqr1 < u_cellSizeSq && u_distSqr1 > 0)
			{
				Contribution<PassType, UpdateType>::add(forceA, forceB1, distSqr1, distVec1, particleA, particleB1,
				                                        params);
			}
			if(u_distSqr2 < u_cellSizeSq && u_distSqr2 > 0)
			{
				Contribution<PassType, UpdateType>::add(forceA, forceB2, distSqr2, distVec2, particleA, particleB2,
				                                        params);
			}
			if(u_distSqr3 < u_cellSizeSq && u_distSqr3 > 0)
			{
				Contribution<PassType, UpdateType>::add(forceA, forceB3, distSqr3, distVec3, particleA, particleB3,
				                                        params);
			}
		}

		for(PxU32 pB = num_loopB; pB < numParticlesB; pB++)
		{
			PxU32 idxB = particleIndicesB[pB];
			Particle& particleB = particlesB[idxB];
			PxVec3& forceB = forceBufB[idxB];

			PX_ALIGN(16, PxVec3 distVec) = particleA.position - particleB.position;

			PxReal distSqr = distVec.magnitudeSquared();
			PxU32 u_distSqr = PxUnionCast<PxU32, PxReal>(distSqr);

			if(u_distSqr < u_cellSizeSq && u_distSqr > 0)
			{
				Contribution<PassType, UpdateType>::add(forceA, forceB, distSqr, distVec, particleA, particleB, params);
			}
		}
	}
}

#if !PX_IOS
/**
particlesA, particlesB, particleIndicesA, particleIndicesB are guaranteed to be non-overlapping
*/
static void updateParticleGroupPair_simd_template(PxVec3* forceBufA, PxVec3* forceBufB, Particle* particlesA,
                                                  Particle* particlesB, const PxU32* particleIndicesA,
                                                  const PxU32 numParticlesA, const PxU32* particleIndicesB,
                                                  const PxU32 numParticlesB, const DynamicsParameters& params,
                                                  const bool isDensityMode, const bool twoWayUpdate,
                                                  Vec4V* tempSimdPositionBuffer, PxU32* tempIndexStream)
{
	PxU32 numParticles4B = ((numParticlesB + 3) & ~0x3) + 4; // ceil up to multiple of four + 4 for save unrolling

	PX_ALIGN(16, Particle fakeParticle);
	fakeParticle.position = PxVec3(FLT_MAX, FLT_MAX, FLT_MAX);
	fakeParticle.density = FLT_MAX; // avoid uninitialized access by V4LoadA

	const PxU32* __restrict idxB = particleIndicesB;
	const PxU32* __restrict idxBEnd = particleIndicesB + numParticlesB;
	for(PxU32 q = 0, v = 0; q < numParticles4B; q += 4, idxB += 4, v += 3)
	{
		const Particle* prtB0 = (q < numParticlesB) ? particlesB + *(idxB) : &fakeParticle;
		const Particle* prtB1 = (q + 1 < numParticlesB) ? particlesB + *(idxB + 1) : &fakeParticle;
		const Particle* prtB2 = (q + 2 < numParticlesB) ? particlesB + *(idxB + 2) : &fakeParticle;
		const Particle* prtB3 = (q + 3 < numParticlesB) ? particlesB + *(idxB + 3) : &fakeParticle;

		Mat44V posDensB_N(V4LoadA(&prtB0->position.x), V4LoadA(&prtB1->position.x), V4LoadA(&prtB2->position.x),
		                  V4LoadA(&prtB3->position.x));
		Mat44V posDensTB_N = M44Trnsps(posDensB_N);

		tempSimdPositionBuffer[v] = posDensTB_N.col0;
		tempSimdPositionBuffer[v + 1] = posDensTB_N.col1;
		tempSimdPositionBuffer[v + 2] = posDensTB_N.col2;
	}

	DynamicsParametersSIMD simdParams;
	simdParams.scaleToStd = V4Load(params.scaleToStd);
	simdParams.scaleSqToStd = V4Load(params.scaleSqToStd);
	simdParams.radiusStd = V4Load(params.radiusStd);
	simdParams.radiusSqStd = V4Load(params.radiusSqStd);
	simdParams.densityMultiplierStd = V4Load(params.densityMultiplierStd);
	simdParams.stiffMulPressureMultiplierStd = V4Load(params.stiffMulPressureMultiplierStd);
	simdParams.viscosityMultiplierStd = V4Load(params.viscosityMultiplierStd);
	simdParams.initialDensity = V4Load(params.initialDensity);
	Vec4V simdCellSizeSq = V4Load(params.cellSizeSq);
	VecU32V simdIntOne = U4LoadXYZW(1, 1, 1, 1);
	VecU32V simdIntZero = U4LoadXYZW(0, 0, 0, 0);

	PxU32 indexStreamSize = 0;
	const PxU32* __restrict idxA = particleIndicesA;
	for(PxU32 p = 0; p < numParticlesA; p++, idxA++)
	{
		Particle* __restrict prtA = particlesA + *idxA;

		PX_ASSERT(MAX_INDEX_STREAM_SIZE - indexStreamSize >= 2);
		tempIndexStream[indexStreamSize++] = *idxA;

		PxU32* interactionCountPtr = tempIndexStream + indexStreamSize++;
		PxU32 indexStreamSizeOld = indexStreamSize;

		PX_ALIGN(16, PxU32 isecs[8]);
		idxB = particleIndicesB;

		Vec4V tmp = V4LoadA(&prtA->position.x);
		Mat44V posDensA(tmp, tmp, tmp, tmp);
		Mat44V posDensTA = M44Trnsps(posDensA);

		const Vec4V* prtB = tempSimdPositionBuffer;
		Vec4V posT0B = *prtB++;
		Vec4V posT1B = *prtB++;
		Vec4V posT2B = *prtB++;
		Vec4V distVec_x = V4Sub(posDensTA.col0, posT0B);
		Vec4V distVec_y = V4Sub(posDensTA.col1, posT1B);
		Vec4V distVec_z = V4Sub(posDensTA.col2, posT2B);
		Vec4V distSqr_x = V4Mul(distVec_x, distVec_x);
		Vec4V distSqr_xy = V4MulAdd(distVec_y, distVec_y, distSqr_x);
		Vec4V distSqr = V4MulAdd(distVec_z, distVec_z, distSqr_xy);
		BoolV isec_b = V4IsGrtr(simdCellSizeSq, distSqr);
		isec_b = BAnd(isec_b, V4IsGrtr(distSqr, V4Zero()));
		VecU32V isec = V4U32Sel(isec_b, simdIntOne, simdIntZero);

		U4StoreA(isec, isecs);

		for(PxU32 q = 0; q < numParticlesB; q += 4, idxB += 4)
		{
			Vec4V posT0B_N = *prtB++;
			Vec4V posT1B_N = *prtB++;
			Vec4V posT2B_N = *prtB++;
			Vec4V distVec_x_N = V4Sub(posDensTA.col0, posT0B_N);
			Vec4V distVec_y_N = V4Sub(posDensTA.col1, posT1B_N);
			Vec4V distVec_z_N = V4Sub(posDensTA.col2, posT2B_N);
			Vec4V distSqr_x_N = V4Mul(distVec_x_N, distVec_x_N);
			Vec4V distSqr_xy_N = V4MulAdd(distVec_y_N, distVec_y_N, distSqr_x_N);
			Vec4V distSqr_N = V4MulAdd(distVec_z_N, distVec_z_N, distSqr_xy_N);
			BoolV isec_b_N = V4IsGrtr(simdCellSizeSq, distSqr_N);
			isec_b_N = BAnd(isec_b_N, V4IsGrtr(distSqr_N, V4Zero()));
			VecU32V isec_N = V4U32Sel(isec_b_N, simdIntOne, simdIntZero);

			PxU32 base_write_index = (q + 4) & 7;
			U4StoreA(isec_N, isecs + base_write_index);

			PxU32 base_read_index = q & 7;
			PxU32 u_isec0 = isecs[base_read_index];
			PxU32 u_isec1 = isecs[base_read_index + 1];
			PxU32 u_isec2 = isecs[base_read_index + 2];
			PxU32 u_isec3 = isecs[base_read_index + 3];

			PX_ASSERT(MAX_INDEX_STREAM_SIZE - indexStreamSize >= 4);

			PX_ASSERT(indexStreamSize < MAX_INDEX_STREAM_SIZE);
			PX_ASSERT(idxB < idxBEnd);
			tempIndexStream[indexStreamSize] = *(idxB);
			indexStreamSize += u_isec0;

			PX_ASSERT(indexStreamSize < MAX_INDEX_STREAM_SIZE);
			tempIndexStream[indexStreamSize] = ((idxB + 1) < idxBEnd) ? *(idxB + 1) : 0;
			indexStreamSize += u_isec1;

			PX_ASSERT(indexStreamSize < MAX_INDEX_STREAM_SIZE);
			tempIndexStream[indexStreamSize] = ((idxB + 2) < idxBEnd) ? *(idxB + 2) : 0;
			indexStreamSize += u_isec2;

			PX_ASSERT(indexStreamSize < MAX_INDEX_STREAM_SIZE);
			tempIndexStream[indexStreamSize] = ((idxB + 3) < idxBEnd) ? *(idxB + 3) : 0;
			indexStreamSize += u_isec3;

			// flush interactions
			if(MAX_INDEX_STREAM_SIZE - indexStreamSize >= (4 + 2))
				;
			else // 4+2, since we potentially need to add the dst index + the src count as well.
			{
				*interactionCountPtr = indexStreamSize - indexStreamSizeOld;
				if(isDensityMode)
				{
					if(twoWayUpdate)
						updateStreamDensityTwoWay(particlesA, particlesB, tempIndexStream, indexStreamSize, params,
						                          simdParams);
					else
						updateStreamDensity(particlesA, particlesB, tempIndexStream, indexStreamSize, params, simdParams);
				}
				else
				{
					if(twoWayUpdate)
						updateStreamForceTwoWay(forceBufA, forceBufB, particlesA, particlesB, tempIndexStream,
						                        indexStreamSize, params, simdParams);
					else
						updateStreamForce(forceBufA, particlesA, particlesB, tempIndexStream, indexStreamSize, params,
						                  simdParams);
				}

				indexStreamSize = 0;
				tempIndexStream[indexStreamSize++] = *idxA;
				interactionCountPtr = tempIndexStream + indexStreamSize++;
				indexStreamSizeOld = indexStreamSize;
			}
		}

		*interactionCountPtr = indexStreamSize - indexStreamSizeOld;
	}

	if(indexStreamSize > 0)
	{
		if(isDensityMode)
		{
			if(twoWayUpdate)
				updateStreamDensityTwoWay(particlesA, particlesB, tempIndexStream, indexStreamSize, params, simdParams);
			else
				updateStreamDensity(particlesA, particlesB, tempIndexStream, indexStreamSize, params, simdParams);
		}
		else
		{
			if(twoWayUpdate)
				updateStreamForceTwoWay(forceBufA, forceBufB, particlesA, particlesB, tempIndexStream, indexStreamSize,
				                        params, simdParams);
			else
				updateStreamForce(forceBufA, particlesA, particlesB, tempIndexStream, indexStreamSize, params,
				                  simdParams);
		}
	}
}

#endif // !PX_IOS

#define SIMD_THRESH_SRC 8

/**
Computes and adds contributions of particle group B to particle group A. If twoWayUpdate is true,
group B is updated with contributions from group A as well.
*/
PX_FORCE_INLINE_KERNELS static void
updateParticleGroupPair(PxVec3* __restrict forceBufA, PxVec3* __restrict forceBufB, Particle* __restrict particlesA,
                        Particle* __restrict particlesB, const PxU32* __restrict particleIndicesA,
                        const PxU32 numParticlesA, const PxU32* __restrict particleIndicesB, const PxU32 numParticlesB,
                        const bool twoWayUpdate, const bool isDensityMode, const DynamicsParameters& params,
                        PxU8* tempSimdPositionBuffer, PxU32* tempIndexStream)
{
	PX_ASSERT(numParticlesA > 0);
	PX_ASSERT(numParticlesB > 0);

#if !PX_IOS
	if(numParticlesB < SIMD_THRESH_SRC)
#endif
	{
		if(isDensityMode)
		{
			if(twoWayUpdate)
			{
				PX_ASSERT(forceBufB);
				updateParticleGroupPair_small_template<DensityPassType, TwoWayUpdateType>(
				    forceBufA, forceBufB, particlesA, particlesB, particleIndicesA, numParticlesA, particleIndicesB,
				    numParticlesB, params);
			}
			else
			{
				updateParticleGroupPair_small_template<DensityPassType, OneWayUpdateType>(
				    forceBufA, forceBufB, particlesA, particlesB, particleIndicesA, numParticlesA, particleIndicesB,
				    numParticlesB, params);
			}
		}
		else
		{
			if(twoWayUpdate)
			{
				PX_ASSERT(forceBufB);
				updateParticleGroupPair_small_template<ForcePassType, TwoWayUpdateType>(
				    forceBufA, forceBufB, particlesA, particlesB, particleIndicesA, numParticlesA, particleIndicesB,
				    numParticlesB, params);
			}
			else
			{
				updateParticleGroupPair_small_template<ForcePassType, OneWayUpdateType>(
				    forceBufA, forceBufB, particlesA, particlesB, particleIndicesA, numParticlesA, particleIndicesB,
				    numParticlesB, params);
			}
		}
	}
#if !PX_IOS
	else
	{
		updateParticleGroupPair_simd_template(forceBufA, forceBufB, particlesA, particlesB, particleIndicesA,
		                                      numParticlesA, particleIndicesB, numParticlesB, params, isDensityMode,
		                                      twoWayUpdate, reinterpret_cast<Vec4V*>(tempSimdPositionBuffer),
		                                      tempIndexStream);
	}
#else
	PX_UNUSED(tempSimdPositionBuffer);
	PX_UNUSED(tempIndexStream);
#endif
}

#endif // REFERENCE_KERNELS

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_DYNAMICS_KERNELS_H
