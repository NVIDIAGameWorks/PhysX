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
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
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
// Copyright (c) 2008-2021 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


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
#include "DyCorrelationBuffer.h"
#include "DySolverConstraintExtShared.h"

using namespace physx;
using namespace Gu;

// constraint-gen only, since these use getVelocityFast methods
// which aren't valid during the solver phase

namespace physx
{

namespace Dy
{


bool setupFinalizeExtSolverContactsCoulomb(
						    const ContactBuffer& buffer,
							const CorrelationBuffer& c,
							const PxTransform& bodyFrame0,
							const PxTransform& bodyFrame1,
							PxU8* workspace,
							PxReal invDt,
							PxReal bounceThresholdF32,
							const SolverExtBody& b0,
							const SolverExtBody& b1,
							PxU32 frictionCountPerPoint,
							PxReal invMassScale0, PxReal invInertiaScale0, 
							PxReal invMassScale1, PxReal invInertiaScale1,
							PxReal restDist,
							PxReal ccdMaxDistance,
							Cm::SpatialVectorF* Z)
{
	// NOTE II: the friction patches are sparse (some of them have no contact patches, and
	// therefore did not get written back to the cache) but the patch addresses are dense,
	// corresponding to valid patches

	const FloatV ccdMaxSeparation = FLoad(ccdMaxDistance);

	PxU8* PX_RESTRICT ptr = workspace;

	const PxF32 maxPenBias0 = b0.mLinkIndex == PxSolverConstraintDesc::NO_LINK ? b0.mBodyData->penBiasClamp : b0.mArticulation->getLinkMaxPenBias(b0.mLinkIndex);
	const PxF32 maxPenBias1 = b1.mLinkIndex == PxSolverConstraintDesc::NO_LINK ? b1.mBodyData->penBiasClamp : b1.mArticulation->getLinkMaxPenBias(b1.mLinkIndex);

	const FloatV maxPenBias = FLoad(PxMax(maxPenBias0, maxPenBias1)/invDt);

	const FloatV restDistance = FLoad(restDist); 
	const FloatV bounceThreshold = FLoad(bounceThresholdF32);

	const Cm::SpatialVectorV vel0 = b0.getVelocity();
	const Cm::SpatialVectorV vel1 = b1.getVelocity();

	const FloatV invDtV = FLoad(invDt);
	const FloatV pt8 = FLoad(0.8f);

	const FloatV invDtp8 = FMul(invDtV, pt8);

	const Vec3V bodyFrame0p = V3LoadU(bodyFrame0.p);
	const Vec3V bodyFrame1p = V3LoadU(bodyFrame1.p);

	Ps::prefetchLine(c.contactID);
	Ps::prefetchLine(c.contactID, 128);

	const PxU32 frictionPatchCount = c.frictionPatchCount;

	const PxU32 pointStride = sizeof(SolverContactPointExt);
	const PxU32 frictionStride = sizeof(SolverContactFrictionExt);
	const PxU8 pointHeaderType = DY_SC_TYPE_EXT_CONTACT;
	const PxU8 frictionHeaderType = DY_SC_TYPE_EXT_FRICTION;

	const FloatV d0 = FLoad(invMassScale0);
	const FloatV d1 = FLoad(invMassScale1);
	const FloatV angD0 = FLoad(invInertiaScale0);
	const FloatV angD1 = FLoad(invInertiaScale1);

	PxU8 flags = 0;

	for(PxU32 i=0;i< frictionPatchCount;i++)
	{
		const PxU32 contactCount = c.frictionPatchContactCounts[i];
		if(contactCount == 0)
			continue;

		const Gu::ContactPoint* contactBase0 = buffer.contacts + c.contactPatches[c.correlationListHeads[i]].start;

		const Vec3V normalV = Ps::aos::V3LoadA(contactBase0->normal);
		const Vec3V normal = V3LoadA(contactBase0->normal);

		const PxReal combinedRestitution = contactBase0->restitution;
	
		
		SolverContactCoulombHeader* PX_RESTRICT header = reinterpret_cast<SolverContactCoulombHeader*>(ptr);
		ptr += sizeof(SolverContactCoulombHeader);

		Ps::prefetchLine(ptr, 128);
		Ps::prefetchLine(ptr, 256);
		Ps::prefetchLine(ptr, 384);

		const FloatV restitution = FLoad(combinedRestitution);


		header->numNormalConstr		= PxU8(contactCount);
		header->type				= pointHeaderType;
		//header->setRestitution(combinedRestitution);

		header->setDominance0(d0);
		header->setDominance1(d1);
		header->angDom0 = invInertiaScale0;
		header->angDom1 = invInertiaScale1;
		header->flags = flags;
		
		header->setNormal(normalV);
		
		for(PxU32 patch=c.correlationListHeads[i]; 
			patch!=CorrelationBuffer::LIST_END; 
			patch = c.contactPatches[patch].next)
		{
			const PxU32 count = c.contactPatches[patch].count;
			const Gu::ContactPoint* contactBase = buffer.contacts + c.contactPatches[patch].start;
				
			PxU8* p = ptr;
			for(PxU32 j=0;j<count;j++)
			{
				const Gu::ContactPoint& contact = contactBase[j];

				SolverContactPointExt* PX_RESTRICT solverContact = reinterpret_cast<SolverContactPointExt*>(p);
				p += pointStride;

				setupExtSolverContact(b0, b1, d0, d1, angD0, angD1, bodyFrame0p, bodyFrame1p, normal, invDtV, invDtp8, restDistance, maxPenBias, restitution,
					bounceThreshold, contact, *solverContact, ccdMaxSeparation, Z, vel0, vel1);
			}			
			ptr = p;
		}
	}

	//construct all the frictions

	PxU8* PX_RESTRICT ptr2 = workspace;

	const PxF32 orthoThreshold = 0.70710678f;
	const PxF32 eps = 0.00001f;
	bool hasFriction = false;

	for(PxU32 i=0;i< frictionPatchCount;i++)
	{
		const PxU32 contactCount = c.frictionPatchContactCounts[i];
		if(contactCount == 0)
			continue;

		SolverContactCoulombHeader* header = reinterpret_cast<SolverContactCoulombHeader*>(ptr2); 
		header->frictionOffset = PxU16(ptr - ptr2);
		ptr2 += sizeof(SolverContactCoulombHeader) + header->numNormalConstr * pointStride;

		const Gu::ContactPoint* contactBase0 = buffer.contacts + c.contactPatches[c.correlationListHeads[i]].start;

		PxVec3 normal = contactBase0->normal;

		const PxReal staticFriction = contactBase0->staticFriction;
		const bool disableStrongFriction = !!(contactBase0->materialFlags & PxMaterialFlag::eDISABLE_FRICTION);
		const bool haveFriction = (disableStrongFriction == 0);
	
		SolverFrictionHeader* frictionHeader = reinterpret_cast<SolverFrictionHeader*>(ptr);
		frictionHeader->numNormalConstr = Ps::to8(c.frictionPatchContactCounts[i]);
		frictionHeader->numFrictionConstr = Ps::to8(haveFriction ? c.frictionPatchContactCounts[i] * frictionCountPerPoint : 0);
		frictionHeader->flags = flags;
		ptr += sizeof(SolverFrictionHeader);
		PxF32* forceBuffer = reinterpret_cast<PxF32*>(ptr);
		ptr += frictionHeader->getAppliedForcePaddingSize(c.frictionPatchContactCounts[i]);
		PxMemZero(forceBuffer, sizeof(PxF32) * c.frictionPatchContactCounts[i]);
		Ps::prefetchLine(ptr, 128);
		Ps::prefetchLine(ptr, 256);
		Ps::prefetchLine(ptr, 384);


		const PxVec3 t0Fallback1(0.f, -normal.z, normal.y);
		const PxVec3 t0Fallback2(-normal.y, normal.x, 0.f) ;
		const PxVec3 tFallback1 = orthoThreshold > PxAbs(normal.x) ? t0Fallback1 : t0Fallback2;
		const PxVec3 vrel = b0.getLinVel() - b1.getLinVel();
		const PxVec3 t0_ = vrel - normal * (normal.dot(vrel));
		const PxReal sqDist = t0_.dot(t0_);
		const PxVec3 tDir0 = (sqDist > eps ? t0_: tFallback1).getNormalized();
		const PxVec3 tDir1 = tDir0.cross(normal);
		PxVec3 tFallback[2] = {tDir0, tDir1};

		PxU32 ind = 0;

		if(haveFriction)
		{
			hasFriction = true;
			frictionHeader->setStaticFriction(staticFriction);
			frictionHeader->invMass0D0 = invMassScale0;
			frictionHeader->invMass1D1 = invMassScale1;
			frictionHeader->angDom0 = invInertiaScale0;
			frictionHeader->angDom1 = invInertiaScale1;
			frictionHeader->type			= frictionHeaderType;
			
			PxU32 totalPatchContactCount = 0;
		
			for(PxU32 patch=c.correlationListHeads[i]; 
				patch!=CorrelationBuffer::LIST_END; 
				patch = c.contactPatches[patch].next)
			{
				const PxU32 count = c.contactPatches[patch].count;
				const PxU32 start = c.contactPatches[patch].start;
				const Gu::ContactPoint* contactBase = buffer.contacts + start;
					
				PxU8* p = ptr;

				for(PxU32 j =0; j < count; j++)
				{
					const Gu::ContactPoint& contact = contactBase[j];
					const Vec3V ra = V3Sub(V3LoadA(contact.point), bodyFrame0p);
					const Vec3V rb = V3Sub(V3LoadA(contact.point), bodyFrame1p);
						
					const Vec3V targetVel = V3LoadA(contact.targetVel);
					const Vec3V pVRa = V3Add(vel0.linear, V3Cross(vel0.angular, ra));
					const Vec3V pVRb = V3Add(vel1.linear, V3Cross(vel1.angular, rb));
					//const PxVec3 vrel = pVRa - pVRb;

					for(PxU32 k = 0; k < frictionCountPerPoint; ++k)
					{
						SolverContactFrictionExt* PX_RESTRICT f0 = reinterpret_cast<SolverContactFrictionExt*>(p);
						p += frictionStride;

						Vec3V t0 = V3LoadU(tFallback[ind]);
						ind = 1 - ind;
						const Vec3V raXn = V3Cross(ra, t0); 
						const Vec3V rbXn = V3Cross(rb, t0);
						Cm::SpatialVectorV deltaV0, deltaV1;

						const Cm::SpatialVectorV resp0 = createImpulseResponseVector(t0, raXn, b0);
						const Cm::SpatialVectorV resp1 = createImpulseResponseVector(V3Neg(t0), V3Neg(rbXn), b1);

						FloatV unitResponse = getImpulseResponse(b0, resp0, deltaV0, d0, angD0,
																 b1, resp1, deltaV1, d1, angD1, reinterpret_cast<Cm::SpatialVectorV*>(Z));

						FloatV tv = V3Dot(targetVel, t0);
						if(b0.mLinkIndex == PxSolverConstraintDesc::NO_LINK)
							tv = FAdd(tv, V3Dot(pVRa, t0));
						else if(b1.mLinkIndex == PxSolverConstraintDesc::NO_LINK)
							tv = FSub(tv, V3Dot(pVRb, t0));

						const FloatV recipResponse = FSel(FIsGrtr(unitResponse, FZero()), FRecip(unitResponse), FZero());

						f0->raXnXYZ_velMultiplierW = V4SetW(Vec4V_From_Vec3V(resp0.angular), recipResponse);
						f0->rbXnXYZ_biasW = V4SetW(V4Neg(Vec4V_From_Vec3V(resp1.angular)), FZero());
						f0->normalXYZ_appliedForceW = V4SetW(Vec4V_From_Vec3V(t0), FZero());
						FStore(tv, &f0->targetVel);
						f0->linDeltaVA = deltaV0.linear;
						f0->angDeltaVA = deltaV0.angular;
						f0->linDeltaVB = deltaV1.linear;
						f0->angDeltaVB = deltaV1.angular;
					}					
				}

				totalPatchContactCount += c.contactPatches[patch].count;
				
				ptr = p;	
			}
		}
	}
	//PX_ASSERT(ptr - workspace == n.solverConstraintSize);
	return hasFriction;
}


}

}
