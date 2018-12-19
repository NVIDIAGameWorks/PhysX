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


#include "ExtRevoluteJoint.h"
#include "PsUtilities.h"
#include "ExtConstraintHelper.h"
#include "CmRenderOutput.h"
#include "PsMathUtils.h"

namespace physx
{
namespace Ext
{
	PxU32 RevoluteJointSolverPrep(Px1DConstraint* constraints,
		PxVec3& body0WorldOffset,
		PxU32 /*maxConstraints*/,
		PxConstraintInvMassScale &invMassScale,
		const void* constantBlock,
		const PxTransform& bA2w,
		const PxTransform& bB2w)
	{
		const RevoluteJointData& data = *reinterpret_cast<const RevoluteJointData*>(constantBlock);
		invMassScale = data.invMassScale;

		const PxJointAngularLimitPair& limit = data.limit;

		bool limitEnabled = data.jointFlags & PxRevoluteJointFlag::eLIMIT_ENABLED;
		bool limitIsLocked = limitEnabled && limit.lower >= limit.upper;

		PxTransform cA2w = bA2w * data.c2b[0];
		PxTransform cB2w = bB2w * data.c2b[1];

		if(cB2w.q.dot(cA2w.q)<0.f)
			cB2w.q = -cB2w.q;

		body0WorldOffset = cB2w.p-bA2w.p;
		Ext::joint::ConstraintHelper ch(constraints, cB2w.p - bA2w.p, cB2w.p - bB2w.p);

		ch.prepareLockedAxes(cA2w.q, cB2w.q, cA2w.transformInv(cB2w.p), 7, PxU32(limitIsLocked ? 7 : 6));

		if(limitIsLocked)
			return ch.getCount();

		PxVec3 axis = cA2w.rotate(PxVec3(1.f,0,0));

		if(data.jointFlags & PxRevoluteJointFlag::eDRIVE_ENABLED)
		{
			Px1DConstraint *c = ch.getConstraintRow();

			c->solveHint = PxConstraintSolveHint::eNONE;

			c->linear0			= PxVec3(0);
			c->angular0			= -axis;
			c->linear1			= PxVec3(0);
			c->angular1			= -axis * data.driveGearRatio;

			c->velocityTarget	= data.driveVelocity;

			c->minImpulse = -data.driveForceLimit;
			c->maxImpulse = data.driveForceLimit;
			if(data.jointFlags & PxRevoluteJointFlag::eDRIVE_FREESPIN)
			{
				if(data.driveVelocity > 0)
					c->minImpulse = 0;
				if(data.driveVelocity < 0)
					c->maxImpulse = 0;
			}
			c->flags |= Px1DConstraintFlag::eHAS_DRIVE_LIMIT;
		}


		if(limitEnabled)
		{
			// PT: rotation part of "const PxTransform cB2cA = cA2w.transformInv(cB2w);"
			const PxQuat cB2cAq = cA2w.q.getConjugate() * cB2w.q;

			// PT: twist part of "Ps::separateSwingTwist(cB2cAq,swing,twist)" (more or less)
			PxQuat twist(cB2cAq.x,0,0,cB2cAq.w);
			PxReal magnitude = twist.normalize();

			PxReal tqPhi = physx::intrinsics::fsel(magnitude - 1e-6f, twist.x / (1.0f + twist.w), 0.f);

			ch.quarterAnglePair(tqPhi, data.tqLow, data.tqHigh, data.tqPad, axis, limit);
		}

		return ch.getCount();
	}
}//namespace

}

//~PX_SERIALIZATION

