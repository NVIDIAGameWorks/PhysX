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

#ifndef DY_SOLVER_CONSTRAINT_EXT_SHARED_H
#define DY_SOLVER_CONSTRAINT_EXT_SHARED_H

#include "foundation/PxPreprocessor.h"
#include "PsVecMath.h"
#include "DyArticulationContactPrep.h"
#include "DySolverConstraintDesc.h"
#include "DySolverConstraint1D.h"
#include "DySolverContact.h"
#include "DySolverContactPF.h"
#include "DyArticulationHelper.h"
#include "PxcNpWorkUnit.h"
#include "PxsMaterialManager.h"
#include "PxsMaterialCombiner.h"

namespace physx
{
namespace Dy
{
	PX_FORCE_INLINE void setupExtSolverContact(const SolverExtBody& b0, const SolverExtBody& b1, 
		const PxF32 d0, const PxF32 d1, const PxF32 angD0, const PxF32 angD1, const PxTransform& bodyFrame0, const PxTransform& bodyFrame1,
		const Vec3VArg normal, const FloatVArg invDt, const FloatVArg invDtp8, const FloatVArg restDistance, const FloatVArg maxPenBias,  const FloatVArg restitution,
		const FloatVArg bounceThreshold, const Gu::ContactPoint& contact, SolverContactPointExt& solverContact, const FloatVArg ccdMaxSeparation)
	{
		const FloatV zero = FZero();
		const FloatV separation = FLoad(contact.separation);

		const FloatV penetration = FSub(separation, restDistance);

		const PxVec3 ra = contact.point - bodyFrame0.p;
		const PxVec3 rb = contact.point - bodyFrame1.p;

		const PxVec3 raXn = ra.cross(contact.normal);
		const PxVec3 rbXn = rb.cross(contact.normal);

		Cm::SpatialVector deltaV0, deltaV1;

		const Cm::SpatialVector resp0 = createImpulseResponseVector(contact.normal, raXn, b0);
		const Cm::SpatialVector resp1 = createImpulseResponseVector(-contact.normal, -rbXn, b1);

		const FloatV unitResponse = FLoad(getImpulseResponse(b0, resp0, deltaV0, d0, angD0,
															 b1, resp1, deltaV1, d1, angD1));

		const FloatV vel0 = FLoad(b0.projectVelocity(contact.normal, raXn));
		const FloatV vel1 = FLoad(b1.projectVelocity(contact.normal, rbXn));
		const FloatV vrel = FSub(vel0, vel1);

		FloatV velMultiplier = FSel(FIsEq(unitResponse, zero), zero, FRecip(unitResponse));
		FloatV scaledBias = FMul(velMultiplier, FMax(maxPenBias, FMul(penetration, invDtp8)));
		const FloatV penetrationInvDt = FMul(penetration, invDt);

		const BoolV isGreater2 = BAnd(BAnd(FIsGrtr(restitution, zero), FIsGrtr(bounceThreshold, vrel)), FIsGrtr(FNeg(vrel), penetrationInvDt));

		const BoolV ccdSeparationCondition = FIsGrtrOrEq(ccdMaxSeparation, penetration);

		scaledBias = FSel(BAnd(ccdSeparationCondition, isGreater2), zero, scaledBias);

		FloatV targetVelocity = FSel(isGreater2, FMul(FNeg(vrel), restitution), zero);

		//Get the rigid body's current velocity and embed into the constraint target velocities
		if(b0.mLinkIndex == PxSolverConstraintDesc::NO_LINK)
			targetVelocity = FSub(targetVelocity, vel0);
		else if(b1.mLinkIndex == PxSolverConstraintDesc::NO_LINK)
			targetVelocity = FAdd(targetVelocity, vel1);

		targetVelocity = FAdd(targetVelocity, V3Dot(V3LoadA(contact.targetVel), normal));

		const FloatV biasedErr = FScaleAdd(targetVelocity, velMultiplier, FNeg(scaledBias));
		const FloatV unbiasedErr = FScaleAdd(targetVelocity, velMultiplier, FSel(isGreater2, zero, FNeg(FMax(scaledBias, zero))));


		FStore(velMultiplier, &solverContact.velMultiplier);
		FStore(biasedErr, &solverContact.biasedErr);
		FStore(unbiasedErr, &solverContact.unbiasedErr);
		solverContact.maxImpulse = contact.maxImpulse;

		solverContact.raXn = V3LoadA(resp0.angular);
		solverContact.rbXn = V3Neg(V3LoadA(resp1.angular));
		solverContact.linDeltaVA = V3LoadA(deltaV0.linear);
		solverContact.angDeltaVA = V3LoadA(deltaV0.angular);
		solverContact.linDeltaVB = V3LoadA(deltaV1.linear);
		solverContact.angDeltaVB = V3LoadA(deltaV1.angular);
	}
}
}

#endif //DY_SOLVER_CONSTRAINT_EXT_SHARED_H
