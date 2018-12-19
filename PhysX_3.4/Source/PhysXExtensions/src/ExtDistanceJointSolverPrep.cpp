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


#include "foundation/PxSimpleTypes.h"
#include "ExtDistanceJoint.h"

namespace physx
{
namespace Ext
{
	PxU32 DistanceJointSolverPrep(Px1DConstraint* constraints,
		PxVec3& body0WorldOffset,
		PxU32 maxConstraints,
		PxConstraintInvMassScale &invMassScale,
		const void* constantBlock,
		const PxTransform& bA2w,
		const PxTransform& bB2w)
	{
		PX_UNUSED(maxConstraints);

		const DistanceJointData& data = *reinterpret_cast<const DistanceJointData*>(constantBlock);
		invMassScale = data.invMassScale;

		PxTransform cA2w = bA2w.transform(data.c2b[0]);
		PxTransform cB2w = bB2w.transform(data.c2b[1]);

		body0WorldOffset = cB2w.p - bA2w.p;

		PxVec3 direction = cA2w.p - cB2w.p;
		PxReal distance = direction.normalize();

		bool enforceMax = (data.jointFlags & PxDistanceJointFlag::eMAX_DISTANCE_ENABLED);
		bool enforceMin = (data.jointFlags & PxDistanceJointFlag::eMIN_DISTANCE_ENABLED);

		if((!enforceMax || distance<=data.maxDistance) && (!enforceMin || distance>=data.minDistance))
			return 0;

#define EPS_REAL 1.192092896e-07F

		if (distance < EPS_REAL)
			direction = PxVec3(1.f,0,0);

		Px1DConstraint *c = constraints;

		// constraint is breakable, so we need to output forces

		c->flags = Px1DConstraintFlag::eOUTPUT_FORCE;

		c->linear0 = direction;		c->angular0 = (cA2w.p - bA2w.p).cross(c->linear0);		
		c->linear1 = direction;		c->angular1 = (cB2w.p - bB2w.p).cross(c->linear1);		

		if (data.jointFlags & PxDistanceJointFlag::eSPRING_ENABLED)
		{
			c->flags |= Px1DConstraintFlag::eSPRING;
			c->mods.spring.stiffness= data.stiffness;
			c->mods.spring.damping	= data.damping;
		}

		//add tolerance so we don't have contact-style jitter problem.

		if (data.minDistance == data.maxDistance && enforceMin && enforceMax)
		{
			PxReal error = distance - data.maxDistance;
			c->geometricError = error >  data.tolerance ? error - data.tolerance :
				error < -data.tolerance ? error + data.tolerance : 0;
		}
		else if (enforceMax && distance > data.maxDistance)
		{
			c->geometricError = distance - data.maxDistance - data.tolerance;
			c->maxImpulse = 0;
		}
		else if (enforceMin && distance < data.minDistance)
		{
			c->geometricError = distance - data.minDistance + data.tolerance;	
			c->minImpulse = 0;
		}

		return 1;
	}
}//namespace

}

//~PX_SERIALIZATION
