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


#include "PsMathUtils.h"
#include "CmConeLimitHelper.h"
#include "DySolverConstraint1D.h"
#include "DyFeatherstoneArticulation.h"
#include "PxsRigidBody.h"
#include "PxcConstraintBlockStream.h"
#include "DyArticulationContactPrep.h"
#include "DyDynamics.h"
#include "DyArticulationReference.h"
#include "DyArticulationPImpl.h"
#include "DyArticulationFnsSimd.h"
#include "DyFeatherstoneArticulationLink.h"
#include "DyFeatherstoneArticulationJointData.h"
#include "PsFoundation.h"
#include "PxsIslandSim.h"
#include "common/PxProfileZone.h"
#include <stdio.h>


#ifdef _MSC_VER
#pragma warning(disable:4505)
#endif

namespace physx
{
namespace Dy
{
	void PxcFsFlushVelocity(FeatherstoneArticulation& articulation, Cm::SpatialVectorF* deltaV);

	//initialize spatial articualted matrix and coriolis spatial force
	void FeatherstoneArticulation::initLinks(ArticulationData& data,
		const PxVec3& gravity, ScratchData& scratchData, Cm::SpatialVectorF* Z,
		Cm::SpatialVectorF* DeltaV)
	{
		PX_UNUSED(Z);
		PX_UNUSED(DeltaV);
		//compute individual link's spatial inertia tensor
		//[0, M]
		//[I, 0]
		computeSpatialInertia(data);

		//compute inidividual zero acceleration force
		computeZ(data, gravity, scratchData);

		if (data.mLinkCount > 1)
		{
			Cm::SpatialVectorF* za = mArticulationData.getTransmittedForces();
			//copy individual zero acceleration force to mTempData zaForce buffer
			PxMemCopy(za, mArticulationData.getSpatialZAVectors(), sizeof(Cm::SpatialVectorF) * mArticulationData.getLinkCount());
		}

		computeArticulatedSpatialInertia(data);

		computeArticulatedResponseMatrix(data);
	
		computeD(data, scratchData, Z, DeltaV);

		//compute corolis and centrifugal force
		computeC(data, scratchData);

		computeArticulatedSpatialZ(mArticulationData, scratchData);
	}

#if (FEATHERSTONE_DEBUG && (PX_DEBUG || PX_CHECKED))
	static bool isSpatialVectorEqual(Cm::SpatialVectorF& t0, Cm::SpatialVectorF& t1)
	{
		float eps = 0.0001f;
		bool e0 = PxAbs(t0.top.x - t1.top.x) < eps &&
			PxAbs(t0.top.y - t1.top.y) < eps &&
			PxAbs(t0.top.z - t1.top.z) < eps;

		bool e1 = PxAbs(t0.bottom.x - t1.bottom.x) < eps &&
			PxAbs(t0.bottom.y - t1.bottom.y) < eps &&
			PxAbs(t0.bottom.z - t1.bottom.z) < eps;

		return e0 && e1;
	}

	static bool isSpatialVectorZero(Cm::SpatialVectorF& t0)
	{
		float eps = 0.000001f;

		const bool c0 = PxAbs(t0.top.x) < eps && PxAbs(t0.top.y) < eps && PxAbs(t0.top.z) < eps;
		const bool c1 = PxAbs(t0.bottom.x) < eps && PxAbs(t0.bottom.y) < eps && PxAbs(t0.bottom.z) < eps;

		return c0 && c1;
	}
#endif

	//calculate Is
	void FeatherstoneArticulation::computeIs(ArticulationLinkData& linkDatum, ArticulationJointCoreData& jointDatum,
		const PxU32 linkID)
	{
		for (PxU32 ind = 0; ind < jointDatum.dof; ++ind)
		{
			const Cm::UnAlignedSpatialVector tmp = mArticulationData.mWorldSpatialArticulatedInertia[linkID] * mArticulationData.mWorldMotionMatrix[linkID][ind];
			linkDatum.IsW[ind].top = tmp.top;
			linkDatum.IsW[ind].bottom = tmp.bottom;
		}
	}

	//compute inertia contribution part
	SpatialMatrix FeatherstoneArticulation::computePropagateSpatialInertia(const PxU8 jointType, ArticulationJointCoreData& jointDatum,
		const SpatialMatrix& articulatedInertia, const Cm::SpatialVectorF* linkIs, InvStIs& invStIs, IsInvD& isInvD, const SpatialSubspaceMatrix& motionMatrix)
	{
		SpatialMatrix spatialInertia;

		switch (jointType)
		{
		case PxArticulationJointType::ePRISMATIC:
		case PxArticulationJointType::eREVOLUTE:
		{
			const Cm::UnAlignedSpatialVector& sa = motionMatrix[0];

			const Cm::SpatialVectorF& Is = linkIs[0];

			const PxReal stIs = sa.innerProduct(Is);

			const PxReal iStIs = (stIs > /*PX_EPS_REAL*/1e-5f) ? (1.f / stIs) : 0.f;

			invStIs.invStIs[0][0] = iStIs;

			Cm::SpatialVectorF isID = Is * iStIs;

			isInvD.isInvD[0] = isID;

			//(6x1)Is = [v0, v1]; (1x6)stI = [v1, v0]
			//Cm::SpatialVector stI1(Is1.angular, Is1.linear);
			Cm::SpatialVectorF stI(Is.bottom, Is.top);

			spatialInertia = SpatialMatrix::constructSpatialMatrix(isID, stI);

			break;
		}
		case PxArticulationJointType::eSPHERICAL:
		{
#if FEATHERSTONE_DEBUG
			//This is for debugging
			Temp6x6Matrix bigInertia(articulatedInertia);
			Temp6x3Matrix bigS(motionMatrix.getColumns());

			Temp6x3Matrix bigIs = bigInertia * bigS;

			for (PxU32 ind = 0; ind < jointDatum.dof; ++ind)
			{
				Cm::SpatialVectorF tempIs = bigInertia * motionMatrix[ind];

				PX_ASSERT(isSpatialVectorEqual(tempIs, linkIs[ind]));

				PX_ASSERT(bigIs.isColumnEqual(ind, tempIs));

			}
#endif
			PxMat33 D(PxIdentity);
			for (PxU32 ind = 0; ind < jointDatum.dof; ++ind)
			{
				for (PxU32 ind2 = 0; ind2 < jointDatum.dof; ++ind2)
				{
					const Cm::UnAlignedSpatialVector& sa = motionMatrix[ind2];
					D[ind][ind2] = sa.innerProduct(linkIs[ind]);
				}
			}

			PxMat33 invD = SpatialMatrix::invertSym33(D);
			for (PxU32 ind = 0; ind < jointDatum.dof; ++ind)
			{
				for (PxU32 ind2 = 0; ind2 < jointDatum.dof; ++ind2)
				{
					invStIs.invStIs[ind][ind2] = invD[ind][ind2];

				}
			}

#if FEATHERSTONE_DEBUG
			//debugging
			Temp6x3Matrix bigIsInvD = bigIs * invD;
#endif

			for (PxU32 ind = 0; ind < jointDatum.dof; ++ind)
			{
				Cm::SpatialVectorF isID(PxVec3(0.f), PxVec3(0.f));

				for (PxU32 ind2 = 0; ind2 < jointDatum.dof; ++ind2)
				{
					const Cm::SpatialVectorF& Is = linkIs[ind2];
					isID += Is * invStIs.invStIs[ind][ind2];
				}
				isInvD.isInvD[ind] = isID;

#if FEATHERSTONE_DEBUG
				const bool equal = bigIsInvD.isColumnEqual(ind, isInvD.isInvD[ind]);
				PX_ASSERT(equal);
#endif
			}

#if FEATHERSTONE_DEBUG
			Temp6x6Matrix transpose6x6 = bigInertia.getTranspose();
#endif
			PxReal stI[6][3]; //[column][row]
			for (PxU32 ind = 0; ind < jointDatum.dof; ++ind)
			{
#if FEATHERSTONE_DEBUG
				Cm::SpatialVectorF& sa = jointDatum.motionMatrix[ind];

				Cm::SpatialVectorF sat = Cm::SpatialVectorF(sa.bottom, sa.top);
				Cm::SpatialVectorF tstI = transpose6x6 * sat;
#endif
				const Cm::SpatialVectorF& Is = linkIs[ind];

#if FEATHERSTONE_DEBUG
				Cm::SpatialVectorF temp(Is.bottom, Is.top);
				const bool equal = isSpatialVectorEqual(temp, tstI);
				PX_ASSERT(equal);
#endif
				//(6x1)Is = [v0, v1]; (1x6)stI = [v1, v0]
				stI[0][ind] = Is.bottom.x;
				stI[1][ind] = Is.bottom.y;
				stI[2][ind] = Is.bottom.z;
				stI[3][ind] = Is.top.x;
				stI[4][ind] = Is.top.y;
				stI[5][ind] = Is.top.z;
			}

			Cm::SpatialVectorF columns[6];
			for (PxU32 ind = 0; ind < 6; ++ind)
			{
				columns[ind] = Cm::SpatialVectorF(PxVec3(0.f), PxVec3(0.f));
				for (PxU32 ind2 = 0; ind2 < jointDatum.dof; ++ind2)
				{
					columns[ind] += isInvD.isInvD[ind2] * stI[ind][ind2];
				}
			}

			spatialInertia = SpatialMatrix::constructSpatialMatrix(columns);
#if FEATHERSTONE_DEBUG
			Temp6x6Matrix result = bigIsInvD * stI;
			PX_ASSERT(result.isEqual(columns));
#endif
			break;
		}
		default:
			spatialInertia.setZero();
			break;
		}

		//(I - Is*Inv(sIs)*sI)
		spatialInertia = articulatedInertia - spatialInertia;

		return spatialInertia;
	}
	
	void FeatherstoneArticulation::computeArticulatedSpatialInertia(ArticulationData& data)
	{
		ArticulationLink* links = data.getLinks();
		ArticulationLinkData* linkData = data.getLinkData();
		ArticulationJointCoreData* jointData = data.getJointData();
		const PxU32 linkCount = data.getLinkCount();
		const PxU32 startIndex = PxU32(linkCount - 1);

		for (PxU32 linkID = startIndex; linkID > 0; --linkID)
		{
			ArticulationLink& link = links[linkID];
			ArticulationLinkData& linkDatum = linkData[linkID];
			
			ArticulationJointCoreData& jointDatum = jointData[linkID];
			computeIs(linkDatum, jointDatum, linkID);

			//(I - Is*Inv(sIs)*sI)
			SpatialMatrix spatialInertiaW = computePropagateSpatialInertia(link.inboundJoint->jointType, 
				jointDatum, data.mWorldSpatialArticulatedInertia[linkID], linkDatum.IsW, data.mInvStIs[linkID], data.mIsInvDW[linkID], data.mWorldMotionMatrix[linkID]);

			//transform spatial inertia into parent space
			FeatherstoneArticulation::translateInertia(constructSkewSymmetricMatrix(linkDatum.rw), spatialInertiaW);

			data.mWorldSpatialArticulatedInertia[link.parent] += spatialInertiaW;
		}

		//cache base link inverse spatial inertia
		data.mWorldSpatialArticulatedInertia[0].invertInertiaV(data.mBaseInvSpatialArticulatedInertiaW);
	}

	void FeatherstoneArticulation::computeArticulatedResponseMatrix(ArticulationData& data)
	{
		//PX_PROFILE_ZONE("ComputeResponseMatrix", 0);

		//We can work out impulse response vectors by propagating an impulse to the root link, then back down to the child link using existing data.
		//Alternatively, we can compute an impulse response matrix, which is a vector of 6x6 matrices, which can be multiplied by the impulse vector to
		//compute the response. This can be stored in world space, saving transforms. It can also be computed incrementally, meaning it should not be
		//dramatically more expensive than propagating the impulse for a single constraint. Furthermore, this will allow us to rapidly compute the 
		//impulse response with the TGS solver allowing us to improve quality of equality positional constraints by properly reflecting non-linear motion
		//of the articulation rather than approximating it with linear projections.

		//The input expected is a local-space impulse and the output is a local-space impulse response vector

		ArticulationLink* links = data.getLinks();
		const PxU32 linkCount = data.getLinkCount();

		const bool fixBase = data.getArticulationFlags() & PxArticulationFlag::eFIX_BASE;

		//(1) impulse response vector for root link
		SpatialImpulseResponseMatrix* responsesW = data.getImpulseResponseMatrixWorld();

		if (fixBase)
		{
			//Fixed base, so response is zero
			PxMemZero(responsesW, sizeof(SpatialImpulseResponseMatrix));
		}
		else
		{
			//Compute impulse response matrix. Compute the impulse response of unit responses on all 6 axes...
			const SpatialMatrix& inverseArticulatedInertiaW = data.mBaseInvSpatialArticulatedInertiaW;
			for (PxU32 i = 0; i < 6; ++i)
			{
				Cm::SpatialVectorF vec = Cm::SpatialVectorF::Zero();
				vec[i] = 1.f;
				responsesW[0].rows[i] = inverseArticulatedInertiaW * vec;
			}
		}

		for (PxU32 linkID = 1; linkID < linkCount; ++linkID)
		{
			PxVec3 offset = data.getLinkData(linkID).rw;

			for (PxU32 i = 0; i < 6; ++i)
			{
				//Impulse has to be negated!
				Cm::SpatialVectorF vec = Cm::SpatialVectorF::Zero();
				vec[i] = 1.f;

				Cm::SpatialVectorF temp = -vec;

				ArticulationLink& tLink = links[linkID];
				//(1) Propagate impulse to parent
				Cm::SpatialVectorF Zp = FeatherstoneArticulation::propagateImpulseW(data.mIsInvDW[linkID], offset,
					data.mWorldMotionMatrix[linkID], temp);				//(2) Get deltaV response for parent
				Cm::SpatialVectorF zR = -responsesW[tLink.parent].getResponse(Zp);

				//(3) Compute deltaV response for this link based on response from parent and original impulse
				Cm::SpatialVectorF deltaV = FeatherstoneArticulation::propagateVelocityTestImpulseW(offset,
					data.mWorldSpatialArticulatedInertia[linkID], data.mInvStIs[linkID], data.mWorldMotionMatrix[linkID], temp, zR);

				//Store in local space (required for propagation
				responsesW[linkID].rows[i] = deltaV;
			}
		}
	}

	void FeatherstoneArticulation::computeArticulatedSpatialZ(ArticulationData& data, ScratchData& scratchData)
	{
		ArticulationLink* links = data.getLinks();
		ArticulationLinkData* linkData = data.getLinkData();
		ArticulationJointCoreData* jointData = data.getJointData();

		const PxU32 linkCount = data.getLinkCount();
		const PxU32 startIndex = PxU32(linkCount - 1);

		Cm::SpatialVectorF* coriolisVectors = scratchData.coriolisVectors;
		Cm::SpatialVectorF* articulatedZA = scratchData.spatialZAVectors;

		PxReal* jointForces = scratchData.jointForces;
		
		for (PxU32 linkID = startIndex; linkID > 0; --linkID)
		{
			ArticulationLink& link = links[linkID];
			ArticulationLinkData& linkDatum = linkData[linkID];
		
			ArticulationJointCoreData& jointDatum = jointData[linkID];

			//calculate spatial zero acceleration force, this can move out of the loop
			Cm::SpatialVectorF Ic = data.mWorldSpatialArticulatedInertia[linkID] * coriolisVectors[linkID];
			Cm::SpatialVectorF ZIc = articulatedZA[linkID] + Ic;

			const PxReal* jF = &jointForces[jointDatum.jointOffset];
			
			Cm::SpatialVectorF ZA(PxVec3(0.f), PxVec3(0.f));
			for (PxU32 ind = 0; ind < jointDatum.dof; ++ind)
			{
				const Cm::UnAlignedSpatialVector& sa = data.mWorldMotionMatrix[linkID][ind];
				const PxReal stZ = sa.innerProduct(ZIc);

				//link.qstZIc[ind] = jF[ind] - stZ;
				const PxReal qstZic = jF[ind] - stZ;
				linkDatum.qstZIc[ind] = qstZic;
				PX_ASSERT(PxIsFinite(qstZic));

				ZA += data.mIsInvDW[linkID].isInvD[ind] * qstZic;
			}
			//accumulate childen's articulated zero acceleration force to parent's articulated zero acceleration
			ZA += ZIc;
			articulatedZA[link.parent] += FeatherstoneArticulation::translateSpatialVector(linkDatum.rw, ZA);
		}
	}

	void FeatherstoneArticulation::computeJointAccelerationW(ArticulationLinkData& linkDatum, ArticulationJointCoreData& jointDatum,
		const Cm::SpatialVectorF& pMotionAcceleration, PxReal* jointAcceleration, const PxU32 linkID)
	{
		PxReal tJAccel[6];
		for (PxU32 ind = 0; ind < jointDatum.dof; ++ind)
		{
			//stI * pAcceleration
			const PxReal temp = linkDatum.IsW[ind].innerProduct(pMotionAcceleration);

			tJAccel[ind] = (linkDatum.qstZIc[ind] - temp);
		}

		//calculate jointAcceleration

		const InvStIs& invStIs = mArticulationData.mInvStIs[linkID];

		for (PxU32 ind = 0; ind < jointDatum.dof; ++ind)
		{
			jointAcceleration[ind] = 0.f;
			for (PxU32 ind2 = 0; ind2 < jointDatum.dof; ++ind2)
			{
				jointAcceleration[ind] += invStIs.invStIs[ind2][ind] * tJAccel[ind2];
			}
			//PX_ASSERT(PxAbs(jointAcceleration[ind]) < 5000);
		}
	}

	void FeatherstoneArticulation::computeLinkAcceleration(ArticulationData& data, ScratchData& scratchData)
	{
		const PxU32 linkCount = data.getLinkCount();
		const PxReal dt = data.getDt();
		const bool fixBase = data.getArticulationFlags() & PxArticulationFlag::eFIX_BASE;
		//we have initialized motionVelocity and motionAcceleration to be zero in the root link if
		//fix based flag is raised
		Cm::SpatialVectorF* motionVelocities = scratchData.motionVelocities;
		Cm::SpatialVectorF* motionAccelerations = scratchData.motionAccelerations;
		Cm::SpatialVectorF* coriolisVectors = scratchData.coriolisVectors;
		Cm::SpatialVectorF* spatialZAForces = scratchData.spatialZAVectors;

		if (!fixBase)
		{
			//ArticulationLinkData& baseLinkDatum = data.getLinkData(0);
			SpatialMatrix invInertia = data.mBaseInvSpatialArticulatedInertiaW;//baseLinkDatum.spatialArticulatedInertia.invertInertia();

#if FEATHERSTONE_DEBUG
			SpatialMatrix result = invInertia * baseLinkDatum.spatialArticulatedInertia;

			bool isIdentity = result.isIdentity();

			PX_ASSERT(isIdentity);
			PX_UNUSED(isIdentity);
#endif
			//ArticulationLink& baseLink = data.getLink(0);
			//const PxTransform& body2World = baseLink.bodyCore->body2World;

			motionAccelerations[0] = -(invInertia * spatialZAForces[0]);
			Cm::SpatialVectorF deltaV = motionAccelerations[0] * dt;

			//Cm::SpatialVectorF oldMotionVel = motionVelocities[0];
			//Cm::SpatialVectorF oldVel = motionVelocities[0];

			motionVelocities[0].top += deltaV.top;
			motionVelocities[0].bottom += deltaV.bottom;
		}
#if FEATHERSTONE_DEBUG
		else
		{
			PX_ASSERT(isSpatialVectorZero(motionAccelerations[0]));
			PX_ASSERT(isSpatialVectorZero(motionVelocities[0]));
		}
#endif

		/*PxReal* jointAccelerations = data.getJointAccelerations();
		PxReal* jointVelocities = data.getJointVelocities();
		PxReal* jointPositions = data.getJointPositions();*/

		PxReal* jointAccelerations = scratchData.jointAccelerations;
		PxReal* jointVelocities = scratchData.jointVelocities;

		//printf("===========================\n");

		//calculate acceleration
		for (PxU32 linkID = 1; linkID < linkCount; ++linkID)
		{
			ArticulationLink& link = data.getLink(linkID);
			//ArticulationLink& plink = data.getLink(link.parent);
			ArticulationLinkData& linkDatum = data.getLinkData(linkID);

			ArticulationJointCore& joint = *link.inboundJoint;
			PX_UNUSED(joint);

			//const SpatialTransform& p2C = data.mChildToParent[linkID].getTranspose();
			//Cm::SpatialVectorF pMotionAcceleration = (p2C * motionAccelerations[link.parent].rotateInv(plink.bodyCore->body2World));

			Cm::SpatialVectorF pMotionAcceleration = FeatherstoneArticulation::translateSpatialVector(-linkDatum.rw, motionAccelerations[link.parent]);

			ArticulationJointCoreData& jointDatum = data.getJointData(linkID);

			//calculate jointAcceleration
			PxReal* jA = &jointAccelerations[jointDatum.jointOffset];
			computeJointAccelerationW(linkDatum, jointDatum, pMotionAcceleration, jA, linkID);
			//printf("jA %f\n", jA[0]);

			Cm::SpatialVectorF motionAcceleration(PxVec3(0.f), PxVec3(0.f));
			PxReal* jointVelocity = &jointVelocities[jointDatum.jointOffset];

			for (PxU32 ind = 0; ind < jointDatum.dof; ++ind)
			{
				PxReal jVel = jointVelocity[ind] + jA[ind] * dt;
				if (PxAbs(jVel) > joint.maxJointVelocity)
				{
					jVel = jVel < 0.f ? -joint.maxJointVelocity : joint.maxJointVelocity;
					jA[ind] = (jVel - jointVelocity[ind]) / dt;
				}

				jointVelocity[ind] = jVel;
				motionAcceleration.top += data.mWorldMotionMatrix[linkID][ind].top * jA[ind];
				motionAcceleration.bottom += data.mWorldMotionMatrix[linkID][ind].bottom * jA[ind];
			}

			//KS - can we just work out velocities by projecting out the joint velocities instead of accumulating all this?
			motionAccelerations[linkID] = pMotionAcceleration + coriolisVectors[linkID] + motionAcceleration;
			PX_ASSERT(motionAccelerations[linkID].isFinite());

			//const PxTransform body2World = link.bodyCore->body2World;
			//motionVelocities[linkID] += motionAccelerations[linkID] * dt;

			const Cm::SpatialVectorF deltaV = motionAccelerations[linkID] * dt;
			motionVelocities[linkID] += deltaV;
		}
	}

	void FeatherstoneArticulation::computeJointTransmittedFrictionForce(
		ArticulationData& data, ScratchData& scratchData, Cm::SpatialVectorF* /*Z*/, Cm::SpatialVectorF* /*DeltaV*/)
	{
		//const PxU32 linkCount = data.getLinkCount();
		const PxU32 startIndex = data.getLinkCount() - 1;

		//const PxReal frictionCoefficient =30.5f;
		Cm::SpatialVectorF* transmittedForce = scratchData.spatialZAVectors;

		for (PxU32 linkID = startIndex; linkID > 1; --linkID)
		{
			const ArticulationLink& link = data.getLink(linkID);
			const ArticulationLinkData& linkDatum = data.getLinkData(linkID);

			//joint force transmitted from parent to child
			//transmittedForce[link.parent] += data.mChildToParent[linkID] *  transmittedForce[linkID];
			transmittedForce[link.parent] += FeatherstoneArticulation::translateSpatialVector(linkDatum.rw, transmittedForce[linkID]);
		}

		transmittedForce[0] = Cm::SpatialVectorF::Zero();

		//const PxReal dt = data.getDt();
		//for (PxU32 linkID = 1; linkID < linkCount; ++linkID)
		//{
		//	//ArticulationLink& link = data.getLink(linkID);
		//	//ArticulationLinkData& linkDatum = data.getLinkData(linkID);
		//	transmittedForce[linkID] = transmittedForce[linkID] * (frictionCoefficient) * dt;
		//	//transmittedForce[link.parent] -= linkDatum.childToParent * transmittedForce[linkID];
		//}

		//

		//applyImpulses(transmittedForce, Z, DeltaV);

		//PxReal* deltaV = data.getJointDeltaVelocities();
		//PxReal* jointV = data.getJointVelocities();

		//for (PxU32 linkID = 1; linkID < data.getLinkCount(); ++linkID)
		//{
		//	ArticulationJointCoreData& tJointDatum = data.getJointData()[linkID];
		//	for (PxU32 i = 0; i < tJointDatum.dof; ++i)
		//	{
		//		jointV[i + tJointDatum.jointOffset] += deltaV[i + tJointDatum.jointOffset];
		//		deltaV[i + tJointDatum.jointOffset] = 0.f;
		//	}
		//}
	}

	//void FeatherstoneArticulation::computeJointFriction(ArticulationData& data,
	//	ScratchData& scratchData)
	//{
	//	PX_UNUSED(scratchData);
	//	const PxU32 linkCount = data.getLinkCount();
	//	PxReal* jointForces = scratchData.jointForces;
	//	PxReal* jointFrictionForces = data.getJointFrictionForces();
	//	PxReal* jointVelocities = data.getJointVelocities();

	//	const PxReal coefficient = 0.5f;

	//	for (PxU32 linkID = 1; linkID < linkCount; ++linkID)
	//	{
	//		ArticulationJointCoreData& jointDatum = data.getJointData(linkID);
	//		//compute generalized force
	//		PxReal* jFs = &jointForces[jointDatum.jointOffset];
	//		PxReal* jVs = &jointVelocities[jointDatum.jointOffset];
	//		PxReal* jFFs = &jointFrictionForces[jointDatum.jointOffset];

	//		for (PxU32 ind = 0; ind < jointDatum.dof; ++ind)
	//		{
	//			PxReal sign = jVs[ind] > 0 ? -1.f : 1.f;
	//			jFFs[ind] = coefficient * PxAbs(jFs[ind]) *sign;

	//			//jFFs[ind] = coefficient * jVs[ind];
	//		}
	//	}
	//}

	void FeatherstoneArticulation::applyExternalImpulse(ArticulationLink* links, const PxU32 linkCount,
		const bool fixBase, ArticulationData& data, Cm::SpatialVectorF* Z, Cm::SpatialVectorF* deltaV,
		const PxReal dt, const PxVec3& gravity, Cm::SpatialVector* acceleration)
	{
		PxReal* jointVelocities = data.getJointVelocities();
		PxReal* jointAccelerations = data.getJointAccelerations();
		PxReal* jointDeltaVelocities = data.getJointDeltaVelocities();

		const PxU32 totalDofs = data.getDofs();
		PxMemZero(jointDeltaVelocities, sizeof(PxReal)*totalDofs);

		ArticulationJointCoreData* jointData = data.getJointData();

		Cm::SpatialVectorF* motionVelcities = data.getMotionVelocities();

		//compute external impulse
		for (PxU32 linkID = 0; linkID < linkCount; ++linkID)
		{
			ArticulationLink& link = links[linkID];
			PxsBodyCore& core = *link.bodyCore;
			
			Cm::SpatialVector& externalAccel = acceleration[linkID];
			PxVec3 linearAccel = externalAccel.linear;
			if(!core.disableGravity)
				linearAccel += gravity;

			PxVec3 angularAccel = externalAccel.angular;

			Cm::SpatialVectorF a(angularAccel, linearAccel);

			Z[linkID] = data.mWorldSpatialArticulatedInertia[linkID] * a * -dt;

			externalAccel.linear = PxVec3(0.f); externalAccel.angular = PxVec3(0.f);
		}

		for (PxU32 linkID = PxU32(linkCount - 1); linkID > 0; --linkID)
		{
			ArticulationLink& tLink = links[linkID];
			Z[tLink.parent] += FeatherstoneArticulation::propagateImpulseW(data.mIsInvDW[linkID], data.mLinksData[linkID].rw, data.mWorldMotionMatrix[linkID], Z[linkID]);
		}

		if (fixBase)
		{
			deltaV[0] = Cm::SpatialVectorF(PxVec3(0.f), PxVec3(0.f));
		}
		else
		{
			//ArticulationLinkData& hLinkDatum = linkData[0];
		
			SpatialMatrix inverseArticulatedInertia = data.mWorldSpatialArticulatedInertia[0].getInverse();

			deltaV[0] = inverseArticulatedInertia * (-Z[0]);
			motionVelcities[0] += deltaV[0];

			PX_ASSERT(motionVelcities[0].isFinite());
		}

		for (PxU32 linkID = 1; linkID < linkCount; ++linkID)
		{
			ArticulationLink& tLink = links[linkID];
			ArticulationJointCoreData& tJointDatum = jointData[linkID];
			PxReal* jV = &jointDeltaVelocities[tJointDatum.jointOffset];
			deltaV[linkID] = FeatherstoneArticulation::propagateVelocityW(data.mLinksData[linkID].rw,
				data.mWorldSpatialArticulatedInertia[linkID], data.mInvStIs[linkID], data.mWorldMotionMatrix[linkID], Z[linkID], jV, deltaV[tLink.parent]);
			motionVelcities[linkID] += deltaV[linkID];
			PX_ASSERT(motionVelcities[linkID].isFinite());
		}

		const PxReal invDt = 1 / dt;
		//update joint acceleration
		for (PxU32 i = 0; i < data.getDofs(); ++i)
		{
			jointVelocities[i] += jointDeltaVelocities[i];
			jointAccelerations[i] = jointDeltaVelocities[i] * invDt;
		}
	}

	PxU32 FeatherstoneArticulation::computeUnconstrainedVelocities(
		const ArticulationSolverDesc& desc,
		PxReal dt,
		PxConstraintAllocator& allocator,
		PxSolverConstraintDesc* constraintDesc,
		PxU32& acCount,
		const PxVec3& gravity, PxU64 contextID,
		Cm::SpatialVectorF* Z, Cm::SpatialVectorF* deltaV)
	{
		PX_UNUSED(contextID);
		PX_UNUSED(desc);
		PX_UNUSED(allocator);
		PX_UNUSED(constraintDesc);
		FeatherstoneArticulation* articulation = static_cast<FeatherstoneArticulation*>(desc.articulation);
		ArticulationData& data = articulation->mArticulationData;
		data.setDt(dt);

		articulation->computeUnconstrainedVelocitiesInternal(gravity, Z, deltaV);

		const bool fixBase = data.getArticulationFlags() & PxArticulationFlag::eFIX_BASE;

		return articulation->setupSolverConstraints(data.getLinks(), data.getLinkCount(), fixBase, data, Z, acCount);
	}

	void FeatherstoneArticulation::computeUnconstrainedVelocitiesTGS(
		const ArticulationSolverDesc& desc,
		PxReal dt, const PxVec3& gravity,
		PxU64 contextID, Cm::SpatialVectorF* Z, Cm::SpatialVectorF* DeltaV)
	{
		PX_UNUSED(contextID);

		FeatherstoneArticulation* articulation = static_cast<FeatherstoneArticulation*>(desc.articulation);
		ArticulationData& data = articulation->mArticulationData;
		data.setDt(dt);

		articulation->computeUnconstrainedVelocitiesInternal(gravity, Z, DeltaV);
	}

	//void FeatherstoneArticulation::computeCounteractJointForce(const ArticulationSolverDesc& desc, ScratchData& /*scratchData*/, const PxVec3& gravity)
	//{
	//	const bool fixBase = mArticulationData.getCore()->flags & PxArticulationFlag::eFIX_BASE;

	//	const PxU32 linkCount = mArticulationData.getLinkCount();
	//	const PxU32 totalDofs = mArticulationData.getDofs();
	//	//common data
	//	computeRelativeTransform(mArticulationData);

	//	jcalc(mArticulationData);

	//	computeSpatialInertia(mArticulationData);

	//	DyScratchAllocator allocator(desc.scratchMemory, desc.scratchMemorySize);

	//	ScratchData tempScratchData;
	//	allocateScratchSpatialData(allocator, linkCount, tempScratchData);

	//	//PxReal* gravityJointForce = allocator.alloc<PxReal>(totalDofs);
	//	//{
	//	//	PxMemZero(gravityJointForce, sizeof(PxReal) * totalDofs);

	//	//	//compute joint force due to gravity
	//	//	tempScratchData.jointVelocities = NULL;
	//	//	tempScratchData.jointAccelerations = NULL;
	//	//	tempScratchData.jointForces = gravityJointForce;
	//	//	tempScratchData.externalAccels = NULL;

	//	//	if (fixBase)
	//	//		inverseDynamic(mArticulationData, gravity,tempScratchData);
	//	//	else
	//	//		inverseDynamicFloatingLink(mArticulationData, gravity, tempScratchData);
	//	//}

	//	////PxReal* jointForce = mArticulationData.getJointForces();
	//	//PxReal* tempJointForce = mArticulationData.getTempJointForces();
	//	//{
	//	//	PxMemZero(tempJointForce, sizeof(PxReal) * totalDofs);

	//	//	//compute joint force due to coriolis force
	//	//	tempScratchData.jointVelocities = mArticulationData.getJointVelocities();
	//	//	tempScratchData.jointAccelerations = NULL;
	//	//	tempScratchData.jointForces = tempJointForce;
	//	//	tempScratchData.externalAccels = NULL;

	//	//	if (fixBase)
	//	//		inverseDynamic(mArticulationData, PxVec3(0.f), tempScratchData);
	//	//	else
	//	//		inverseDynamicFloatingLink(mArticulationData, PxVec3(0.f), tempScratchData);
	//	//}

	//	//PxReal* jointForce = mArticulationData.getJointForces();
	//	//for (PxU32 i = 0; i < mArticulationData.getDofs(); ++i)
	//	//{
	//	//	jointForce[i] = tempJointForce[i] - gravityJointForce[i];
	//	//}

	//	//PxReal* jointForce = mArticulationData.getJointForces();
	//	PxReal* tempJointForce = mArticulationData.getTempJointForces();
	//	{
	//		PxMemZero(tempJointForce, sizeof(PxReal) * totalDofs);

	//		//compute joint force due to coriolis force
	//		tempScratchData.jointVelocities = mArticulationData.getJointVelocities();
	//		tempScratchData.jointAccelerations = NULL;
	//		tempScratchData.jointForces = tempJointForce;
	//		tempScratchData.externalAccels = mArticulationData.getExternalAccelerations();

	//		if (fixBase)
	//			inverseDynamic(mArticulationData, gravity, tempScratchData);
	//		else
	//			inverseDynamicFloatingLink(mArticulationData, gravity, tempScratchData);
	//	}

	//	PxReal* jointForce = mArticulationData.getJointForces();
	//	for (PxU32 i = 0; i < mArticulationData.getDofs(); ++i)
	//	{
	//		jointForce[i] = tempJointForce[i];
	//	}
	//}

	void FeatherstoneArticulation::updateArticulation(ScratchData& scratchData,
		const PxVec3& gravity, Cm::SpatialVectorF* Z, Cm::SpatialVectorF* DeltaV)
	{
		computeRelativeTransformC2P(mArticulationData);

		computeLinkVelocities(mArticulationData, scratchData);

		initLinks(mArticulationData, gravity, scratchData, Z, DeltaV);

		computeLinkAcceleration(mArticulationData, scratchData);
	}

	void FeatherstoneArticulation::computeUnconstrainedVelocitiesInternal(
		const PxVec3& gravity,
		Cm::SpatialVectorF* Z, Cm::SpatialVectorF* DeltaV)
	{
		//PX_PROFILE_ZONE("Articulations:computeUnconstrainedVelocities", 0);

		mStaticConstraints.forceSize_Unsafe(0);

		PxMemZero(mArticulationData.mNbStaticConstraints.begin(), mArticulationData.mNbStaticConstraints.size()*sizeof(PxU32));

		const PxU32 linkCount = mArticulationData.getLinkCount();

		mArticulationData.init();

		jcalc(mArticulationData);

		ScratchData scratchData;
		scratchData.motionVelocities = mArticulationData.getMotionVelocities();
		scratchData.motionAccelerations = mArticulationData.getMotionAccelerations();
		scratchData.coriolisVectors = mArticulationData.getCorioliseVectors();
		scratchData.spatialZAVectors = mArticulationData.getSpatialZAVectors();
		scratchData.jointAccelerations = mArticulationData.getJointAccelerations();
		scratchData.jointVelocities = mArticulationData.getJointVelocities();
		scratchData.jointPositions = mArticulationData.getJointPositions();
		scratchData.jointForces = mArticulationData.getJointForces();
		scratchData.externalAccels = mArticulationData.getExternalAccelerations();

		updateArticulation(scratchData, gravity, Z, DeltaV);

		
		if (mArticulationData.mLinkCount > 1)
		{
			//use individual zero acceleration force(we copy the initial Z value to the transmitted force buffers in initLink())
			scratchData.spatialZAVectors = mArticulationData.getTransmittedForces();
			computeZAForceInv(mArticulationData, scratchData);
			computeJointTransmittedFrictionForce(mArticulationData, scratchData, Z, DeltaV);
		}

		//the dirty flag is used in inverse dynamic
		mArticulationData.setDataDirty(true);

		//zero zero acceleration vector in the articulation data so that we can use this buffer to accumulated
		//impulse for the contacts/constraints in the PGS/TGS solvers
		PxMemZero(mArticulationData.getSpatialZAVectors(), sizeof(Cm::SpatialVectorF) * linkCount);

		// solver progress counters
		maxSolverNormalProgress = 0;
		maxSolverFrictionProgress = 0;
		solverProgress = 0;
		numTotalConstraints = 0;

		for (PxU32 a = 0; a < mArticulationData.getLinkCount(); ++a)
		{
			mArticulationData.mAccumulatedPoses[a] = mArticulationData.getLink(a).bodyCore->body2World;
			mArticulationData.mPreTransform[a] = mArticulationData.getLink(a).bodyCore->body2World;
			mArticulationData.mDeltaQ[a] = PxQuat(PxIdentity);
		}
	}

	void FeatherstoneArticulation::enforcePrismaticLimits(PxReal* jPosition, ArticulationJointCore* joint)
	{
		const PxU32 dofId = joint->dofIds[0];
		if (joint->motion[dofId] == PxArticulationMotion::eLIMITED)
		{
			if (jPosition[0] < (joint->limits[dofId].low))
				jPosition[0] = joint->limits[dofId].low;

			if (jPosition[0] > (joint->limits[dofId].high))
				jPosition[0] = joint->limits[dofId].high;
		}
	}

	PxQuat computeSphericalJointPositions(const PxQuat relativeQuat,
		const PxQuat newRot, const PxQuat pBody2WorldRot,
		PxReal* jPositions, const SpatialSubspaceMatrix& motionMatrix)
	{
		PxQuat newParentToChild = (newRot.getConjugate() * pBody2WorldRot).getNormalized();

		PxQuat jointRotation = newParentToChild * relativeQuat.getConjugate();

		PxReal radians;
		PxVec3 axis;
		jointRotation.toRadiansAndUnitAxis(radians, axis);

		axis *= radians;

		for (PxU32 d = 0; d < 3; ++d)
		{
			jPositions[d] = -motionMatrix[d].top.dot(axis);
		}

		return newParentToChild;
	}

	void FeatherstoneArticulation::computeAndEnforceJointPositions(ArticulationData& data, PxReal* jointPositions)
	{
		ArticulationLink* links = data.getLinks();
		const PxU32 linkCount = data.getLinkCount();

		ArticulationJointCoreData* jointData = data.getJointData();

		for (PxU32 linkID = 1; linkID < linkCount; ++linkID)
		{
			ArticulationLink& link = links[linkID];
			ArticulationJointCore* joint = link.inboundJoint;
			ArticulationJointCoreData& jointDatum = jointData[linkID];
			PxReal* jPositions = &jointPositions[jointDatum.jointOffset];

			if (joint->jointType == PxArticulationJointType::eSPHERICAL)
			{
				ArticulationLink& pLink = links[link.parent];
				//const PxTransform pBody2World = pLink.bodyCore->body2World;

				computeSphericalJointPositions(data.mRelativeQuat[linkID], link.bodyCore->body2World.q, 
					pLink.bodyCore->body2World.q, jPositions, data.getMotionMatrix(linkID));
			}
			else if (joint->jointType == PxArticulationJointType::eREVOLUTE)
			{
				/*if (jPositions[0] < -PxPi)
					jPositions[0] += PxTwoPi;
				else if (jPositions[0] > PxPi)
					jPositions[0] -= PxTwoPi;*/

				PxReal jPos = jPositions[0];

				if (jPos > PxTwoPi)
					jPos -= PxTwoPi*2.f;
				else if (jPos < -PxTwoPi)
					jPos += PxTwoPi*2.f;

				jPos = PxClamp(jPos, -PxTwoPi*2.f, PxTwoPi*2.f);

				jPositions[0] = jPos;
			}
			else if(joint->jointType == PxArticulationJointType::ePRISMATIC)
			{
				enforcePrismaticLimits(jPositions, joint);
			}
		}
	}


	static void computeSphericalJointVelocities(const Dy::ArticulationJointCoreBase& joint,
		const PxQuat& newRot, const PxVec3& worldRelAngVel,
		PxReal* jVelocities)
	{
		PxQuat cB2w = newRot * joint.childPose.q;

		PxVec3 relVel = cB2w.rotateInv(worldRelAngVel);

		PxU32 dofIndex = 0;
		if (joint.motion[PxArticulationAxis::eTWIST] != PxArticulationMotion::eLOCKED)
			jVelocities[dofIndex++] = relVel.x;
		if (joint.motion[PxArticulationAxis::eSWING1] != PxArticulationMotion::eLOCKED)
			jVelocities[dofIndex++] = relVel.y;
		if (joint.motion[PxArticulationAxis::eSWING2] != PxArticulationMotion::eLOCKED)
			jVelocities[dofIndex++] = relVel.z;
		if (joint.motion[PxArticulationAxis::eTWIST] == PxArticulationMotion::eLOCKED)
			jVelocities[dofIndex++] = relVel.x;
		if (joint.motion[PxArticulationAxis::eSWING1] == PxArticulationMotion::eLOCKED)
			jVelocities[dofIndex++] = relVel.y;
		if (joint.motion[PxArticulationAxis::eSWING2] == PxArticulationMotion::eLOCKED)
			jVelocities[dofIndex++] = relVel.z;
	}

	//static void computeSphericalJointVelocities(
	//	const SpatialSubspaceMatrix& motionMatrix, 
	//	const PxQuat& childRot, const PxVec3& worldRelAngVel,
	//	PxReal* jVelocities)
	//{

	//	const PxVec3 relVel = childRot.rotateInv(worldRelAngVel);

	//	//PxQuat cB2w = newRot * joint.childPose.q;

	//	//PxVec3 relVel = cB2w.rotateInv(worldRelAngVel);

	//	for (PxU32 i = 0; i < 3; ++i)
	//	{
	//		const PxVec3 axis = motionMatrix[i].top;

	//		const PxReal axisVel = axis.dot(relVel);

	//		jVelocities[i] = axisVel;
	//	}


	//	/*PxU32 dofIndex = 0;
	//	if (joint.motion[PxArticulationAxis::eTWIST] != PxArticulationMotion::eLOCKED)
	//		jVelocities[dofIndex++] = relVel.x;
	//	if (joint.motion[PxArticulationAxis::eSWING1] != PxArticulationMotion::eLOCKED)
	//		jVelocities[dofIndex++] = relVel.y;
	//	if (joint.motion[PxArticulationAxis::eSWING2] != PxArticulationMotion::eLOCKED)
	//		jVelocities[dofIndex++] = relVel.z;
	//	if (joint.motion[PxArticulationAxis::eTWIST] == PxArticulationMotion::eLOCKED)
	//		jVelocities[dofIndex++] = relVel.x;
	//	if (joint.motion[PxArticulationAxis::eSWING1] == PxArticulationMotion::eLOCKED)
	//		jVelocities[dofIndex++] = relVel.y;
	//	if (joint.motion[PxArticulationAxis::eSWING2] == PxArticulationMotion::eLOCKED)
	//		jVelocities[dofIndex++] = relVel.z;*/
	//}


	void FeatherstoneArticulation::updateJointProperties(const PxReal* jointDeltaVelocities,
		const Cm::SpatialVectorF* motionVelocities, PxReal* jointVelocities, PxReal* jointAccelerations)
	{
		using namespace Dy;

		ArticulationLink* links = mArticulationData.getLinks();

		ArticulationJointCoreData* jointData = mArticulationData.getJointData();

		const PxU32 linkCount = mArticulationData.getLinkCount();

		const PxReal invDt = 1.f / mArticulationData.getDt();

		for (PxU32 linkID = 1; linkID < linkCount; ++linkID)
		{
			ArticulationLink& link = links[linkID];

			ArticulationJointCoreData& jointDatum = jointData[linkID];
			//PxgArticulationLink& link = links[linkID];
			const PxU32 parent = link.parent;

			//PxTransform& body2World = preTransforms[linkID];
			PxTransform& body2World = link.bodyCore->body2World;
			
			ArticulationJointCoreBase& joint = *link.inboundJoint;

			PxReal* jVelocity = &jointVelocities[jointDatum.jointOffset];
			PxReal* jAccel = &jointAccelerations[jointDatum.jointOffset];
			const PxReal* jDeltaVelocity = &jointDeltaVelocities[jointDatum.jointOffset];

			switch (joint.jointType)
			{
			case PxArticulationJointType::ePRISMATIC:
			case PxArticulationJointType::eREVOLUTE:
			{
				jVelocity[0] += jDeltaVelocity[0];
				jAccel[0] += jDeltaVelocity[0] * invDt;
				break;
			}
			case PxArticulationJointType::eSPHERICAL:
			{
				if (jointDatum.dof < 3)
				{
					for (PxU32 i = 0; i < jointDatum.dof; ++i)
					{
						jVelocity[i] += jDeltaVelocity[i];
						jAccel[i] += jDeltaVelocity[i] * invDt;
					}
				}
				else
				{
					PxReal jTmpVel[3] = { jVelocity[0], jVelocity[1], jVelocity[2] };

					PxVec3 worldRelAngVel = motionVelocities[linkID].top - motionVelocities[parent].top;

					computeSphericalJointVelocities(joint, body2World.q, worldRelAngVel, jVelocity);

					for (PxU32 i = 0; i < 3; ++i)
					{
						const PxReal jDeltaV = jTmpVel[i] - jVelocity[i];
						jointAccelerations[i] += jDeltaV * invDt;
					}
				}

				break;
			}
			default:
				break;
			}
		}
	}

	void FeatherstoneArticulation::recomputeAccelerations(const PxReal dt)
	{
		ArticulationJointCoreData* jointData = mArticulationData.getJointData();
		
		const PxU32 linkCount = mArticulationData.getLinkCount();

		Cm::SpatialVectorF* motionAccels = mArticulationData.getMotionAccelerations();
		PxReal* jDeltaVelocities = mArticulationData.getJointDeltaVelocities();
		
		const PxReal invDt = 1.f / dt;

		const bool fixBase = mArticulationData.getArticulationFlags() & PxArticulationFlag::eFIX_BASE;

		//compute root link accelerations
		if (fixBase)
		{
			motionAccels[0] = Cm::SpatialVectorF::Zero();
		}
		else
		{

			Cm::SpatialVectorF tAccel = (mArticulationData.getMotionVelocity(0) - mArticulationData.mRootPreMotionVelocity) * invDt;
			motionAccels[0].top = tAccel.top;
			motionAccels[0].bottom = tAccel.bottom;
		}

		for (PxU32 linkID = 1; linkID < linkCount; ++linkID)
		{
			ArticulationJointCoreData& jointDatum = jointData[linkID];

			PxReal* jDeltaVelocity = &jDeltaVelocities[jointDatum.jointOffset];

			for (PxU32 i = 0; i < jointDatum.dof; ++i)
			{
				const PxReal deltaAccel = jDeltaVelocity[i] * invDt;
				motionAccels[linkID].top += mArticulationData.mWorldMotionMatrix[linkID][i].top * deltaAccel;
				motionAccels[linkID].bottom += mArticulationData.mWorldMotionMatrix[linkID][i].bottom * deltaAccel;
			}
		}
	}

	Cm::SpatialVector FeatherstoneArticulation::recomputeAcceleration(const PxU32 linkId, const PxReal dt) const
	{
		ArticulationLink* links = mArticulationData.getLinks();

		Cm::SpatialVectorF tMotionAccel;
		const PxReal invDt = 1.f / dt;
		if (linkId == 0)
		{
			const bool fixBase = mArticulationData.getArticulationFlags() & PxArticulationFlag::eFIX_BASE;
			if (fixBase)
			{
				tMotionAccel = Cm::SpatialVectorF::Zero();
			}
			else
			{
				ArticulationLink& baseLink = links[0];
				PxTransform& body2World = baseLink.bodyCore->body2World;

				Cm::SpatialVectorF tAccel = (mArticulationData.getMotionVelocity(0) - mArticulationData.mRootPreMotionVelocity) * invDt;
				tMotionAccel.top = body2World.rotate(tAccel.top);
				tMotionAccel.bottom = body2World.rotate(tAccel.bottom);
			}
		}
		else
		{

			ArticulationJointCoreData* jointData = mArticulationData.getJointData();

			const Cm::SpatialVectorF* motionAccels = mArticulationData.getMotionAccelerations();
			const PxReal* jDeltaVelocities = mArticulationData.getJointDeltaVelocities();

			ArticulationLink& link = links[linkId];

			ArticulationJointCoreData& jointDatum = jointData[linkId];

			PxTransform& body2World = link.bodyCore->body2World;

			const PxReal* jDeltaVelocity = &jDeltaVelocities[jointDatum.jointOffset];

			
			for (PxU32 i = 0; i < jointDatum.dof; ++i)
			{
				const PxReal deltaAccel = jDeltaVelocity[i] * invDt;
				tMotionAccel.top = motionAccels[linkId].top + mArticulationData.mMotionMatrix[linkId][i].top * deltaAccel;
				tMotionAccel.bottom = motionAccels[linkId].bottom + mArticulationData.mMotionMatrix[linkId][i].bottom * deltaAccel;
			}

			tMotionAccel.top = body2World.rotate(tMotionAccel.top);
			tMotionAccel.bottom = body2World.rotate(tMotionAccel.bottom);
		}

		return Cm::SpatialVector(tMotionAccel.bottom, tMotionAccel.top);
		
	}


	void FeatherstoneArticulation::propagateLinksDown(ArticulationData& data, PxReal* jointDeltaVelocities, PxReal* jointPositions,
		Cm::SpatialVectorF* motionVelocities)
	{
		ArticulationLink* links = mArticulationData.getLinks();

		PxTransform* preTransforms = mArticulationData.getPreTransform();
		PX_UNUSED(preTransforms);
		PX_UNUSED(motionVelocities);

		ArticulationJointCoreData* jointData = mArticulationData.getJointData();

		const PxQuat* const PX_RESTRICT relativeQuats = mArticulationData.mRelativeQuat.begin();

		const PxU32 linkCount = mArticulationData.getLinkCount();

		const PxReal dt = data.getDt();

		PxReal* jointVelocities = data.getJointVelocities();

		for (PxU32 linkID = 1; linkID < linkCount; ++linkID)
		{
			ArticulationLink& link = links[linkID];

			ArticulationJointCoreData& jointDatum = jointData[linkID];
			//ArticulationLinkData& linkDatum = mArticulationData.getLinkData(linkID);
			//const PxTransform oldTransform = preTransforms[linkID];

			ArticulationLink& pLink = links[link.parent];
			const PxTransform pBody2World = pLink.bodyCore->body2World;

			ArticulationJointCore* joint = link.inboundJoint;

			PxReal* jVelocity = &jointVelocities[jointDatum.jointOffset];
			PxReal* jDeltaVelocity = &jointDeltaVelocities[jointDatum.jointOffset];
			PxReal* jPosition = &jointPositions[jointDatum.jointOffset];

			PxQuat newParentToChild;
			PxQuat newWorldQ;
			PxVec3 r;

			const PxVec3 childOffset = -joint->childPose.p;
			const PxVec3 parentOffset = joint->parentPose.p;

			PxTransform& body2World = link.bodyCore->body2World;

			const PxQuat relativeQuat = relativeQuats[linkID];

			switch (joint->jointType)
			{
			case PxArticulationJointType::ePRISMATIC:
			{
				const PxReal delta = (jVelocity[0] + jDeltaVelocity[0]) * dt;

				jPosition[0] += delta;

				enforcePrismaticLimits(jPosition, joint);

				newParentToChild = relativeQuat;
				const PxVec3 e = newParentToChild.rotate(parentOffset);
				const PxVec3 d = childOffset;

				const PxVec3& u = data.mMotionMatrix[linkID][0].bottom;

				r = e + d + u * jPosition[0];
				break;
			}
			case PxArticulationJointType::eREVOLUTE:
			{
				//use positional iteration JointVelociy to integrate
				const PxReal delta = (jVelocity[0] + jDeltaVelocity[0]) * dt;

				PxReal jPos = jPosition[0] + delta;

				if (jPos > PxTwoPi)
					jPos -= PxTwoPi*2.f;
				else if (jPos < -PxTwoPi)
					jPos += PxTwoPi*2.f;

				jPos = PxClamp(jPos, -PxTwoPi*2.f, PxTwoPi*2.f);
				jPosition[0] = jPos;

				const PxVec3& u = data.mMotionMatrix[linkID][0].top;

				PxQuat jointRotation = PxQuat(-jPosition[0], u);
				if (jointRotation.w < 0)	//shortest angle.
					jointRotation = -jointRotation;

				newParentToChild = (jointRotation * relativeQuat).getNormalized();

				const PxVec3 e = newParentToChild.rotate(parentOffset);
				const PxVec3 d = childOffset;
				r = e + d;

				PX_ASSERT(r.isFinite());

				break;
			}
			case PxArticulationJointType::eSPHERICAL:  
			{
				if (jointDatum.dof < 3)
				{
					newParentToChild = PxQuat(PxIdentity);
					//We are simulating a revolute or 2d joint, so just integrate quaternions and joint positions as above...
					for (PxU32 i = 0; i < jointDatum.dof; ++i)
					{
						const PxReal delta = (jVelocity[i] + jDeltaVelocity[i]) * dt;

						PxReal jPos = jPosition[i] + delta;

						if (jPos > PxTwoPi)
							jPos -= PxTwoPi*2.f;
						else if (jPos < -PxTwoPi)
							jPos += PxTwoPi*2.f;

						jPos = PxClamp(jPos, -PxTwoPi*2.f, PxTwoPi*2.f);
						jPosition[i] = jPos;

						const PxVec3& u = data.mMotionMatrix[linkID][i].top;

						PxQuat jointRotation = PxQuat(-jPosition[i], u);
						if (jointRotation.w < 0)	//shortest angle.
							jointRotation = -jointRotation;

						newParentToChild = newParentToChild * (jointRotation * relativeQuat).getNormalized();
					}
					const PxVec3 e = newParentToChild.rotate(parentOffset);
					const PxVec3 d = childOffset;
					r = e + d;

					PX_ASSERT(r.isFinite());
				}
				else
				{
					const PxTransform oldTransform = preTransforms[linkID];

					PxVec3 worldAngVel = motionVelocities[linkID].top;

					newWorldQ = Ps::exp(worldAngVel*dt) * oldTransform.q;

					/*PxReal jPos[3];
					jPos[0] = jPosition[0] + (jVelocity[0] + jDeltaVelocity[0])*dt;
					jPos[1] = jPosition[1] + (jVelocity[1] + jDeltaVelocity[1])*dt;
					jPos[2] = jPosition[2] + (jVelocity[2] + jDeltaVelocity[2])*dt;*/

					newParentToChild = computeSphericalJointPositions(data.mRelativeQuat[linkID], newWorldQ,
						pBody2World.q, jPosition, data.getMotionMatrix(linkID));

					/*for (PxU32 i = 0; i < 3; ++i)
					{
						PxReal diff = jPosition[i] - jPos[i];

						PX_ASSERT(PxAbs(diff) < 1e-2f);

					}*/

					const PxVec3 e = newParentToChild.rotate(parentOffset);
					const PxVec3 d = childOffset;
					r = e + d;
				}

				break;
			}
			case PxArticulationJointType::eFIX:
			{
				//this is fix joint so joint don't have velocity
				newParentToChild = relativeQuat;

				const PxVec3 e = newParentToChild.rotate(parentOffset);
				const PxVec3 d = childOffset;

				r = e + d;
				break;
			}
			default:
				break;
			}

			body2World.q = (pBody2World.q * newParentToChild.getConjugate()).getNormalized();
			body2World.p = pBody2World.p + body2World.q.rotate(r);

			PX_ASSERT(body2World.isSane());
			PX_ASSERT(body2World.isValid());	
		}
	}

	void FeatherstoneArticulation::updateBodies(const ArticulationSolverDesc& desc, PxReal dt)
	{
		updateBodies(static_cast<FeatherstoneArticulation*>(desc.articulation), dt, true);
	}

	void FeatherstoneArticulation::updateBodiesTGS(const ArticulationSolverDesc& desc, PxReal dt)
	{
		updateBodies(static_cast<FeatherstoneArticulation*>(desc.articulation), dt, false);
	}

	void FeatherstoneArticulation::updateBodies(FeatherstoneArticulation* articulation, PxReal dt, bool integrateJointPositions)
	{
		ArticulationData& data = articulation->mArticulationData;
		ArticulationLink* links = data.getLinks();
		const PxU32 linkCount = data.getLinkCount();

		Cm::SpatialVectorF* motionVelocities = data.getMotionVelocities();

		Cm::SpatialVector* externalAccels = data.getExternalAccelerations();
		Cm::SpatialVector zero = Cm::SpatialVector::zero();

		data.setDt(dt);
		
		PxTransform* preTransforms = data.getPreTransform();

		if (articulation->mHasSphericalJoint)
		{
			for (PxU32 i = 0; i < linkCount; ++i)
			{
				ArticulationLink& link = links[i];

				const PxsBodyCore* bodyCore = link.bodyCore;

				//record link's previous transform
				preTransforms[i] = bodyCore->body2World;
			}
		}

		if (!integrateJointPositions)
		{
			//TGS
			for (PxU32 linkID = 0; linkID < linkCount; ++linkID)
			{
				links[linkID].bodyCore->body2World = data.mAccumulatedPoses[linkID].getNormalized();
			}

			articulation->computeAndEnforceJointPositions(data, data.getJointPositions());
		}
		else
		{
			const bool fixBase = data.getArticulationFlags() & PxArticulationFlag::eFIX_BASE;

			//PGS
			if (!fixBase)
			{
				//body2World store new body transform integrated from solver linear/angular velocity

				ArticulationLink& link = links[0];

				const PxTransform& preTrans = link.bodyCore->body2World;

				const Cm::SpatialVectorF& posVel = data.getPosIterMotionVelocity(0);

				updateRootBody(posVel, preTrans, data, dt);
			}
			//using the original joint velocities and delta velocities changed in the positional iter to update joint position/body transform
			articulation->propagateLinksDown(data, data.getPosIterJointDeltaVelocities(), data.getJointPositions(), data.getPosIterMotionVelocities());
		}

		//update joint velocities/accelerations due to contacts/constraints
		if (data.mJointDirty)
		{
			//update delta joint velocity and motion velocity due to velocity iteration changes
			Cm::SpatialVectorF deltaV[64];

			//update motionVelocities
			PxcFsFlushVelocity(*articulation, deltaV);

			//update joint velocity/accelerations
			PxReal* jointVelocities = data.getJointVelocities();
			PxReal* jointAccelerations = data.getJointAccelerations();
			PxReal* jointDeltaVelocities = data.getJointDeltaVelocities();

			articulation->updateJointProperties(jointDeltaVelocities, motionVelocities, jointVelocities, jointAccelerations);
		}

		for (PxU32 linkID = 0; linkID < linkCount; ++linkID)
		{
			ArticulationLink& link = links[linkID];
			PxsBodyCore* bodyCore = link.bodyCore;

			bodyCore->linearVelocity = motionVelocities[linkID].bottom;
			bodyCore->angularVelocity = motionVelocities[linkID].top;
			//zero external accelerations
			externalAccels[linkID] = zero;
		}
	}

	void FeatherstoneArticulation::updateRootBody(const Cm::SpatialVectorF& motionVelocity, 
		const PxTransform& preTransform, ArticulationData& data, const PxReal dt)
	{
		ArticulationLink* links = data.getLinks();
		//body2World store new body transform integrated from solver linear/angular velocity

		PX_ASSERT(motionVelocity.top.isFinite());
		PX_ASSERT(motionVelocity.bottom.isFinite());

		ArticulationLink& baseLink = links[0];

		PxsBodyCore* baseBodyCore = baseLink.bodyCore;

		//(1) project the current body's velocity (based on its pre-pose) to the geometric COM that we're integrating around...

		PxVec3 comLinVel = motionVelocity.bottom;

		//using the position iteration motion velocity to compute the body2World
		PxVec3 newP = (preTransform.p) + comLinVel * dt;

		PxQuat deltaQ = Ps::exp(motionVelocity.top*dt);

		baseBodyCore->body2World = PxTransform(newP, (deltaQ* preTransform.q).getNormalized());

		PX_ASSERT(baseBodyCore->body2World.isFinite() && baseBodyCore->body2World.isValid());
	}

	void FeatherstoneArticulation::updateBodies()
	{
		ArticulationData& data = mArticulationData;
		const PxReal dt = data.getDt();

		ArticulationLink* links = data.getLinks();

		Cm::SpatialVectorF* motionVelocities = data.getMotionVelocities();
		PxTransform* preTransforms = data.getPreTransform();

		const PxU32 linkCount = data.getLinkCount();

		if (mHasSphericalJoint)
		{
			for (PxU32 i = 0; i < linkCount; ++i)
			{
				ArticulationLink& link = links[i];

				const PxsBodyCore* bodyCore = link.bodyCore;

				//record link's previous transform
				PX_ASSERT(bodyCore->body2World.isFinite() && bodyCore->body2World.isValid());
				preTransforms[i] = bodyCore->body2World;
			}
		}

		const bool fixBase = data.getArticulationFlags() & PxArticulationFlag::eFIX_BASE;

		if (!fixBase)
		{
			ArticulationLink& link = links[0];

			const PxTransform& preTrans = link.bodyCore->body2World;
			
			Cm::SpatialVectorF& motionVelocity = motionVelocities[0];

			updateRootBody(motionVelocity, preTrans, data, dt);
		}

		propagateLinksDown(data, data.getJointVelocities(), data.getJointPositions(), data.getMotionVelocities());
	}

	void FeatherstoneArticulation::getJointAcceleration(const PxVec3& gravity, PxArticulationCache& cache)
	{
		if (mArticulationData.getDataDirty())
		{
			Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Articulation::getJointAcceleration() commonInit need to be called first to initialize data!");
			return;
		}

		const PxU32 linkCount = mArticulationData.getLinkCount();
		PxcScratchAllocator* allocator = reinterpret_cast<PxcScratchAllocator*>(cache.scratchAllocator);

		ScratchData scratchData;
		PxU8* tempMemory = allocateScratchSpatialData(allocator, linkCount, scratchData);

		scratchData.jointVelocities = cache.jointVelocity;
		scratchData.jointForces = cache.jointForce;
	
		computeLinkVelocities(mArticulationData, scratchData);

		//compute individual link's spatial inertia tensor
		//[0, M]
		//[I, 0]
		computeSpatialInertia(mArticulationData);

		//compute inidividual zero acceleration force
		computeZ(mArticulationData, gravity, scratchData);

		computeArticulatedSpatialInertia(mArticulationData);

		//compute corolis and centrifugal force
		computeC(mArticulationData, scratchData);

		computeArticulatedSpatialZ(mArticulationData, scratchData);

		const bool fixBase = mArticulationData.getArticulationFlags() & PxArticulationFlag::eFIX_BASE;
		//we have initialized motionVelocity and motionAcceleration to be zero in the root link if
		//fix based flag is raised
		//ArticulationLinkData& baseLinkDatum = mArticulationData.getLinkData(0);

		Cm::SpatialVectorF* motionAccelerations = scratchData.motionAccelerations;
		Cm::SpatialVectorF* spatialZAForces = scratchData.spatialZAVectors;
		Cm::SpatialVectorF* coriolisVectors = scratchData.coriolisVectors;

		if (!fixBase)
		{
			SpatialMatrix inverseArticulatedInertia = mArticulationData.mWorldSpatialArticulatedInertia[0].getInverse();
			motionAccelerations[0] = -(inverseArticulatedInertia * spatialZAForces[0]);
		}
#if FEATHERSTONE_DEBUG
		else
		{
			PX_ASSERT(isSpatialVectorZero(motionAccelerations[0]));
		}
#endif

		PxReal* jointAccelerations = cache.jointAcceleration;
		//calculate acceleration
		for (PxU32 linkID = 1; linkID < linkCount; ++linkID)
		{
			ArticulationLink& link = mArticulationData.getLink(linkID);

			ArticulationLinkData& linkDatum = mArticulationData.getLinkData(linkID);

			//SpatialTransform p2C = linkDatum.childToParent.getTranspose();
			//Cm::SpatialVectorF pMotionAcceleration = mArticulationData.mChildToParent[linkID].transposeTransform(motionAccelerations[link.parent]);

			Cm::SpatialVectorF pMotionAcceleration = FeatherstoneArticulation::translateSpatialVector(-linkDatum.rw, motionAccelerations[link.parent]);

			ArticulationJointCoreData& jointDatum = mArticulationData.getJointData(linkID);
			//calculate jointAcceleration
			PxReal* jA = &jointAccelerations[jointDatum.jointOffset];
			computeJointAccelerationW(linkDatum, jointDatum, pMotionAcceleration, jA, linkID);

			Cm::SpatialVectorF motionAcceleration(PxVec3(0.f), PxVec3(0.f));

			for (PxU32 ind = 0; ind < jointDatum.dof; ++ind)
			{
				motionAcceleration.top += mArticulationData.mWorldMotionMatrix[linkID][ind].top * jA[ind];
				motionAcceleration.bottom += mArticulationData.mWorldMotionMatrix[linkID][ind].bottom * jA[ind];
			}

			motionAccelerations[linkID] = pMotionAcceleration + coriolisVectors[linkID] + motionAcceleration;
			PX_ASSERT(motionAccelerations[linkID].isFinite());
		}

		allocator->free(tempMemory);
	}

}//namespace Dy
}
