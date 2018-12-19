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


#include "ExtSphericalJoint.h"
#include "ExtConstraintHelper.h"
#include "CmConeLimitHelper.h"
#include "CmRenderOutput.h"

namespace physx
{
namespace Ext
{
	PxU32 SphericalJointSolverPrep(Px1DConstraint* constraints,
		PxVec3& body0WorldOffset,
		PxU32 /*maxConstraints*/,
		PxConstraintInvMassScale &invMassScale,
		const void* constantBlock,							  
		const PxTransform& bA2w,
		const PxTransform& bB2w)
	{

		using namespace joint;
		const SphericalJointData& data = *reinterpret_cast<const SphericalJointData*>(constantBlock);
		invMassScale = data.invMassScale;


		PxTransform cA2w = bA2w * data.c2b[0];
		PxTransform cB2w = bB2w * data.c2b[1];

		if(cB2w.q.dot(cA2w.q)<0) 
			cB2w.q = -cB2w.q;

		body0WorldOffset = cB2w.p-bA2w.p;
		joint::ConstraintHelper ch(constraints, cA2w.p - bA2w.p, cB2w.p - bB2w.p);

		if(data.jointFlags & PxSphericalJointFlag::eLIMIT_ENABLED)
		{
			PxQuat swing, twist;
			Ps::separateSwingTwist(cA2w.q.getConjugate() * cB2w.q, swing, twist);
			PX_ASSERT(PxAbs(swing.x)<1e-6f);

			const PxReal pad = data.limit.isSoft() ? 0.0f : data.tanQPad;
			Cm::ConeLimitHelper coneHelper(data.tanQZLimit, data.tanQYLimit, pad);

			PxVec3 axis;
			PxReal error;
			if(coneHelper.getLimit(swing, axis, error))
				ch.angularLimit(cA2w.rotate(axis),error,data.limit);
		}

		ch.prepareLockedAxes(cA2w.q, cB2w.q, cA2w.transformInv(cB2w.p), 7, 0);

		return ch.getCount();
	}
}//namespace

}

//~PX_SERIALIZATION

