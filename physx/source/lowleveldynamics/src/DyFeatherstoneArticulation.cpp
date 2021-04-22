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
#include "PxArticulation.h"
#include "PsFoundation.h"
#include "DyFeatherstoneArticulationLink.h"
#include "DyFeatherstoneArticulationJointData.h"
#include "DySolverConstraint1DStep.h"
#include "DyTGSDynamics.h"
#include "DyConstraintPrep.h"
#include "common/PxProfileZone.h"
#include "PxsContactManager.h"
#include "DyContactPrep.h"
#include "DySolverContext.h"
#include "DyTGSContactPrep.h"

#ifndef FEATURESTONE_DEBUG
#define FEATURESTONE_DEBUG 0
#endif

// we encode articulation link handles in the lower bits of the pointer, so the
// articulation has to be aligned, which in an aligned pool means we need to size it
// appropriately

namespace physx
{

namespace Dy
{

	extern PxcCreateFinalizeSolverContactMethod createFinalizeMethods[3];

	void SolverCoreRegisterArticulationFns();

	void SolverCoreRegisterArticulationFnsCoulomb();

	void PxvRegisterArticulationsReducedCoordinate()
	{
		const Articulation::Enum type = Articulation::eReducedCoordinate;
		ArticulationPImpl::sComputeUnconstrainedVelocities[type] = &FeatherstoneArticulation::computeUnconstrainedVelocities;
		ArticulationPImpl::sUpdateBodies[type] = &FeatherstoneArticulation::updateBodies;
		ArticulationPImpl::sUpdateBodiesTGS[type] = &FeatherstoneArticulation::updateBodiesTGS;
		ArticulationPImpl::sSaveVelocity[type] = &FeatherstoneArticulation::saveVelocity;
		ArticulationPImpl::sSaveVelocityTGS[type] = &FeatherstoneArticulation::saveVelocityTGS;

		ArticulationPImpl::sUpdateDeltaMotion[type] = &FeatherstoneArticulation::recordDeltaMotion;
		ArticulationPImpl::sDeltaMotionToMotionVel[type] = &FeatherstoneArticulation::deltaMotionToMotionVelocity;
		ArticulationPImpl::sComputeUnconstrainedVelocitiesTGS[type] = &FeatherstoneArticulation::computeUnconstrainedVelocitiesTGS;
		ArticulationPImpl::sSetupInternalConstraintsTGS[type] = &FeatherstoneArticulation::setupSolverConstraintsTGS;

		SolverCoreRegisterArticulationFns();
		SolverCoreRegisterArticulationFnsCoulomb();
	}

	ArticulationData::~ArticulationData()
	{
		if (mLinksData)
			PX_FREE_AND_RESET(mLinksData);

		if (mJointData)
			PX_FREE_AND_RESET(mJointData);

		if(mJointTranData)
			PX_FREE_AND_RESET(mJointTranData);
	}

	void ArticulationData::resizeLinkData(const PxU32 linkCount)
	{
		mMotionVelocities.reserve(linkCount);
		mMotionVelocities.forceSize_Unsafe(linkCount);

		mMotionAccelerations.reserve(linkCount);
		mMotionAccelerations.forceSize_Unsafe(linkCount);

		mCorioliseVectors.reserve(linkCount);
		mCorioliseVectors.forceSize_Unsafe(linkCount);

		mZAForces.reserve(linkCount);
		mZAForces.forceSize_Unsafe(linkCount);

		mNbStaticConstraints.reserve(linkCount);
		mNbStaticConstraints.forceSize_Unsafe(linkCount);

		mStaticConstraintStartIndex.reserve(linkCount);
		mStaticConstraintStartIndex.forceSize_Unsafe(linkCount);

		mDeltaMotionVector.reserve(linkCount);
		mDeltaMotionVector.forceSize_Unsafe(linkCount);

		mPreTransform.reserve(linkCount);
		mPreTransform.forceSize_Unsafe(linkCount);

		mResponseMatrixW.reserve(linkCount);
		mResponseMatrixW.forceSize_Unsafe(linkCount);

		mWorldSpatialArticulatedInertia.reserve(linkCount);
		mWorldSpatialArticulatedInertia.forceSize_Unsafe(linkCount);

		mInvStIs.reserve(linkCount);
		mInvStIs.forceSize_Unsafe(linkCount);

		mMotionMatrix.resize(linkCount);

		mWorldMotionMatrix.reserve(linkCount);
		mWorldMotionMatrix.forceSize_Unsafe(linkCount);

		mIsInvDW.reserve(linkCount);
		mIsInvDW.forceSize_Unsafe(linkCount);

		mAccumulatedPoses.reserve(linkCount);
		mAccumulatedPoses.forceSize_Unsafe(linkCount);

		mDeltaQ.reserve(linkCount);
		mDeltaQ.forceSize_Unsafe(linkCount);

		mPosIterMotionVelocities.reserve(linkCount);
		mPosIterMotionVelocities.forceSize_Unsafe(linkCount);

		mJointTransmittedForce.reserve(linkCount);
		mJointTransmittedForce.forceSize_Unsafe(linkCount);

		mRelativeQuat.resize(linkCount);


		if (mLinksData)
			PX_FREE_AND_RESET(mLinksData);

		if (mJointData)
			PX_FREE_AND_RESET(mJointData);

		if(mJointTranData)
			PX_FREE_AND_RESET(mJointTranData);
		
		mLinksData = PX_PLACEMENT_NEW(PX_ALLOC(sizeof(ArticulationLinkData) * linkCount, PX_DEBUG_EXP("ArticulationLinkData")), ArticulationLinkData)();
		mJointData = PX_PLACEMENT_NEW(PX_ALLOC(sizeof(ArticulationJointCoreData) * linkCount, PX_DEBUG_EXP("ArticulationJointCoreData")), ArticulationJointCoreData)();
		mJointTranData = PX_PLACEMENT_NEW(PX_ALLOC(sizeof(ArticulationJointTargetData)*linkCount, PX_DEBUG_EXP("ArticulationJointTargetData")), ArticulationJointTargetData)();
		
		const PxU32 size = sizeof(Cm::SpatialVectorF) * linkCount;

		PxMemZero(mMotionVelocities.begin(), size);
		PxMemZero(mMotionAccelerations.begin(), size);
		PxMemZero(mCorioliseVectors.begin(), size);
		PxMemZero(mZAForces.begin(), size);
		PxMemZero(mDeltaMotionVector.begin(), size);

		PxMemZero(mPreTransform.begin(), sizeof(PxTransform) * linkCount);

		PxMemZero(mLinksData, sizeof(ArticulationLinkData) * linkCount);
		PxMemZero(mJointData, sizeof(ArticulationJointCoreData) * linkCount);
		PxMemZero(mJointTranData, sizeof(ArticulationJointTargetData)*linkCount);
	}

	void ArticulationData::resizeJointData(const PxU32 dofs)
	{
		mJointAcceleration.reserve(dofs);
		mJointAcceleration.forceSize_Unsafe(dofs);

		mJointVelocity.reserve(dofs);
		mJointVelocity.forceSize_Unsafe(dofs);

		mJointDeltaVelocity.reserve(dofs);
		mJointDeltaVelocity.forceSize_Unsafe(dofs);

		mJointPosition.reserve(dofs);
		mJointPosition.forceSize_Unsafe(dofs);

		mJointForce.reserve(dofs);
		mJointForce.forceSize_Unsafe(dofs);

	/*	mJointFrictionForce.reserve(dofs);
		mJointFrictionForce.forceSize_Unsafe(dofs);*/

		mPosIterJointDeltaVelocities.reserve(dofs);
		mPosIterJointDeltaVelocities.forceSize_Unsafe(dofs);

		/*mTempData.mJointForce.reserve(dofs);
		mTempData.mJointForce.forceSize_Unsafe(dofs);*/

		PxMemZero(mJointAcceleration.begin(), sizeof(PxReal) * dofs);
		PxMemZero(mJointVelocity.begin(), sizeof(PxReal) * dofs);
		PxMemZero(mJointDeltaVelocity.begin(), sizeof(PxReal) * dofs);
		PxMemZero(mPosIterJointDeltaVelocities.begin(), sizeof(PxReal)*dofs);
		PxMemZero(mJointPosition.begin(), sizeof(PxReal) * dofs);
		PxMemZero(mJointForce.begin(), sizeof(PxReal) * dofs);
		//PxMemZero(mJointFrictionForce.begin(), sizeof(PxReal) * dofs);
	}

	ArticulationLinkData& ArticulationData::getLinkData(PxU32 index) const
	{
		PX_ASSERT(index < mLinkCount);
		return mLinksData[index];
	}

	//ArticulationJointCoreData&	ArticulationData::getJointData(PxU32 index) const
	//{
	//	PX_ASSERT(index < mLinkCount);
	//	return mJointData[index];
	//}

	void ArticulationJointCore::setJointPose(ArticulationJointCoreData& jointDatum, SpatialSubspaceMatrix& motionMatrix, bool forceUpdate,
		PxQuat& relativeQuat)
	{
		if (dirtyFlag & ArticulationJointCoreDirtyFlag::ePOSE || forceUpdate)
		{
			relativeQuat = (childPose.q * (parentPose.q.getConjugate())).getNormalized();

			jointDatum.computeMotionMatrix(this, motionMatrix);

			dirtyFlag &= ~ArticulationJointCoreDirtyFlag::ePOSE;
		}
	}

	PX_COMPILE_TIME_ASSERT((sizeof(Articulation)&(DY_ARTICULATION_MAX_SIZE - 1)) == 0);

	FeatherstoneArticulation::FeatherstoneArticulation(void* userData)
		: ArticulationV(userData, Articulation::eReducedCoordinate), mHasSphericalJoint(false)
	{
		PX_ASSERT((reinterpret_cast<size_t>(this) & (DY_ARTICULATION_MAX_SIZE - 1)) == 0);
		mGPUDirtyFlags = 0;
	}

	FeatherstoneArticulation::~FeatherstoneArticulation()
	{
	}

	void FeatherstoneArticulation::copyJointData(ArticulationData& data, PxReal* toJointData, const PxReal* fromJointData)
	{
		const PxU32 dofCount = data.getDofs();

		PxMemCopy(toJointData, fromJointData, sizeof(PxReal)*dofCount);
	}

	void FeatherstoneArticulation::computeDofs()
	{
		const PxU32 linkCount = mArticulationData.getLinkCount();
		PxU32 totalDofs = 0;
		PxU32 totalLocks = 0;
		for (PxU32 linkID = 1; linkID < linkCount; ++linkID)
		{
			const ArticulationLink& link = mArticulationData.getLink(linkID);

			ArticulationJointCoreData& jointDatum = mArticulationData.getJointData(linkID);
			
			jointDatum.computeJointDof(link.inboundJoint, true);
			jointDatum.jointOffset = totalDofs;
			totalDofs += jointDatum.dof;
			totalLocks += jointDatum.lockedAxes;
		}

		if (totalDofs != mArticulationData.getDofs())
		{
			mArticulationData.resizeJointData(totalDofs);
		}

		mArticulationData.setDofs(totalDofs);
		mArticulationData.setLocks(totalLocks);
	}

	bool FeatherstoneArticulation::resize(const PxU32 linkCount)
	{
		if (Dy::ArticulationV::resize(linkCount))
		{
			if (linkCount != mSolverDesc.linkCount)
			{
				//compute scratchSize
				const PxU32 linkCount4 = (linkCount + 3)&(~3);
				const PxU32 scratchSize = sizeof(Cm::SpatialVectorF) * linkCount4 * 4
					+ sizeof(Cm::SpatialVector) * linkCount4
					+ sizeof(Dy::SpatialMatrix) * linkCount4
					+ sizeof(PxReal) * linkCount4 * 4;

				mScratchMemory.resize(scratchSize);
				mSolverDesc.scratchMemory = mScratchMemory.begin();
				mSolverDesc.scratchMemorySize = Ps::to16(scratchSize);

				mArticulationData.resizeLinkData(linkCount);
				
			}
			return true;
		}
		return false;
	}

	void FeatherstoneArticulation::getDataSizes(PxU32 /*linkCount*/, PxU32& solverDataSize, PxU32& totalSize, PxU32& scratchSize)
	{		
		solverDataSize = 0;
		totalSize = 0;
		scratchSize = 0;
	}

	void FeatherstoneArticulation::onUpdateSolverDesc()
	{
		Dy::ArticulationV::onUpdateSolverDesc();
		mArticulationData.mLinks				= mSolverDesc.links;
		mArticulationData.mLinkCount			= mSolverDesc.linkCount;		
		mArticulationData.mFlags				= mSolverDesc.core ? &mSolverDesc.core->flags : mSolverDesc.flags;	// PT: PX-1399
		mArticulationData.mExternalAcceleration	= mSolverDesc.acceleration;
		mArticulationData.mSolverDataSize		= mSolverDesc.solverDataSize;
		mArticulationData.mArticulation			= this;

		computeDofs();
	}

	PxU32 FeatherstoneArticulation::getDofs()
	{
		PxU32 dofs = mArticulationData.getDofs();
		
		if (dofs == 0xffffffff)
		{
			computeDofs();
		}

		return mArticulationData.getDofs();
	}

	PxU32 FeatherstoneArticulation::getDof(const PxU32 linkID)
	{
		const ArticulationJointCoreData& jointDatum = mArticulationData.getJointData(linkID);
		return jointDatum.dof;
	}

	bool FeatherstoneArticulation::applyCache(PxArticulationCache& cache, const PxArticulationCacheFlags flag)
	{
		return applyCacheToDest(mArticulationData, cache, mArticulationData.getJointVelocities(), mArticulationData.getJointAccelerations(),
			mArticulationData.getJointPositions(), mArticulationData.getJointForces(), flag);
	}

	void FeatherstoneArticulation::copyInternalStateToCache(PxArticulationCache& cache,
		const PxArticulationCacheFlags flag)
	{
		if (flag & PxArticulationCache::eVELOCITY)
		{
			copyJointData(mArticulationData, cache.jointVelocity, mArticulationData.getJointVelocities());
		}

		if (flag & PxArticulationCache::eACCELERATION)
		{
			copyJointData(mArticulationData, cache.jointAcceleration, mArticulationData.getJointAccelerations());
		}

		if (flag & PxArticulationCache::ePOSITION)
		{
			copyJointData(mArticulationData, cache.jointPosition, mArticulationData.getJointPositions());
		}

		if (flag & PxArticulationCache::eFORCE)
		{
			copyJointData(mArticulationData, cache.jointForce, mArticulationData.getJointForces());
		}

		if (flag & PxArticulationCache::eLINKVELOCITY)
		{
			const Cm::SpatialVectorF* vels = mArticulationData.getMotionVelocities();
			const PxU32 numLinks = mArticulationData.getLinkCount();
			for (PxU32 i = 0; i < numLinks; ++i)
			{
				const Cm::SpatialVectorF& vel = vels[i];
				cache.linkVelocity[i].linear = vel.bottom;
				cache.linkVelocity[i].angular = vel.top;
			}
		}

		if (flag & PxArticulationCache::eLINKACCELERATION)
		{
			const PxU32 numLinks = mArticulationData.getLinkCount();

			if (mArticulationData.getDt() > 0.f)
				recomputeAccelerations(mArticulationData.getDt());

			const Cm::SpatialVectorF* accels = mArticulationData.getMotionAccelerations();

			for (PxU32 i = 0; i < numLinks; ++i)
			{
				const Cm::SpatialVectorF& accel = accels[i];
				cache.linkAcceleration[i].linear = accel.bottom;
				cache.linkAcceleration[i].angular = accel.top;
			}			
		}

		if (flag & PxArticulationCache::eROOT)
		{
			const ArticulationLink& rLink = mArticulationData.getLink(0);
			const Cm::SpatialVectorF& vel = mArticulationData.getMotionVelocity(0);
			const PxsBodyCore& rBodyCore = *rLink.bodyCore;
			const PxTransform& body2World = rBodyCore.body2World;
			cache.rootLinkData->transform = body2World * rBodyCore.getBody2Actor().getInverse();
			cache.rootLinkData->worldLinVel = vel.bottom;
			cache.rootLinkData->worldAngVel = vel.top;

			const Cm::SpatialVectorF* accels = mArticulationData.getMotionAccelerations();
			cache.rootLinkData->worldLinAccel = accels[0].bottom;
			cache.rootLinkData->worldAngAccel = accels[0].top;
		}
	}

	static PX_FORCE_INLINE Mat33V loadPxMat33(const PxMat33& m)
	{
		return Mat33V(Vec3V_From_Vec4V(V4LoadU(&m.column0.x)), 
			Vec3V_From_Vec4V(V4LoadU(&m.column1.x)), V3LoadU(&m.column2.x));
	}

	static PX_FORCE_INLINE void storePxMat33(const Mat33V& src, PxMat33& dst)
	{
		V3StoreU(src.col0, dst.column0);
		V3StoreU(src.col1, dst.column1);
		V3StoreU(src.col2, dst.column2);
	}

	void FeatherstoneArticulation::transformInertia(const SpatialTransform& sTod, SpatialMatrix& spatialInertia)
	{
#if 1
		const SpatialTransform dTos = sTod.getTranspose();

		Mat33V tL = loadPxMat33(spatialInertia.topLeft);
		Mat33V tR = loadPxMat33(spatialInertia.topRight);
		Mat33V bL = loadPxMat33(spatialInertia.bottomLeft);

		Mat33V R = loadPxMat33(sTod.R);
		Mat33V T = loadPxMat33(sTod.T);

		Mat33V tl = M33MulM33(R, tL);
		Mat33V tr = M33MulM33(R, tR);
		Mat33V bl = M33Add(M33MulM33(T, tL), M33MulM33(R, bL));
		Mat33V br = M33Add(M33MulM33(T, tR), M33MulM33(R, M33Trnsps(tL)));

		Mat33V dR = loadPxMat33(dTos.R);
		Mat33V dT = loadPxMat33(dTos.T);

		tL = M33Add(M33MulM33(tl, dR), M33MulM33(tr, dT));
		tR = M33MulM33(tr, dR);
		bL = M33Add(M33MulM33(bl, dR), M33MulM33(br, dT));

		bL = M33Scale(M33Add(bL, M33Trnsps(bL)), FHalf());

		storePxMat33(tL, spatialInertia.topLeft);
		storePxMat33(tR, spatialInertia.topRight);
		storePxMat33(bL, spatialInertia.bottomLeft);
#else
		const SpatialTransform dTos = sTod.getTranspose();

		PxMat33 tl = sTod.R * spatialInertia.topLeft;
		PxMat33 tr = sTod.R * spatialInertia.topRight;
		PxMat33 bl = sTod.T * spatialInertia.topLeft + sTod.R * spatialInertia.bottomLeft;
		PxMat33 br = sTod.T * spatialInertia.topRight + sTod.R * spatialInertia.getBottomRight();

		spatialInertia.topLeft = tl * dTos.R + tr * dTos.T;
		spatialInertia.topRight = tr * dTos.R;
		spatialInertia.bottomLeft = bl * dTos.R + br * dTos.T;

		//aligned inertia
		spatialInertia.bottomLeft = (spatialInertia.bottomLeft + spatialInertia.bottomLeft.getTranspose()) * 0.5f;
#endif
	}

	void FeatherstoneArticulation::translateInertia(const PxMat33& sTod, SpatialMatrix& inertia)
	{
#if 1
		Mat33V sTodV = loadPxMat33(sTod);
		Mat33V dTos = M33Trnsps(sTodV);

		const Mat33V tL = loadPxMat33(inertia.topLeft);
		const Mat33V tR = loadPxMat33(inertia.topRight);
		const Mat33V bL = loadPxMat33(inertia.bottomLeft);

		const Mat33V bl = M33Add(M33MulM33(sTodV, tL), bL);
		const Mat33V br = M33Add(M33MulM33(sTodV, tR), M33Trnsps(tL));
		const Mat33V bottomLeft = M33Add(bl, M33MulM33(br, dTos));

		storePxMat33(M33Add(tL, M33MulM33(tR, dTos)), inertia.topLeft);
		storePxMat33(M33Scale(M33Add(bottomLeft, M33Trnsps(bottomLeft)), FHalf()), inertia.bottomLeft);
#else
		const PxMat33 dTos = sTod.getTranspose();

		PxMat33 bl = sTod * inertia.topLeft + inertia.bottomLeft;
		PxMat33 br = sTod * inertia.topRight + inertia.getBottomRight();

		inertia.topLeft = inertia.topLeft + inertia.topRight * dTos;
		inertia.bottomLeft = bl + br * dTos;

		//aligned inertia - make it symmetrical! OPTIONAL!!!!
		inertia.bottomLeft = (inertia.bottomLeft + inertia.bottomLeft.getTranspose()) * 0.5f;
#endif
	}

	void FeatherstoneArticulation::getImpulseResponse(
		PxU32 linkID,
		Cm::SpatialVectorF* Z,
		const Cm::SpatialVector& impulse,
		Cm::SpatialVector& deltaVV) const
	{
		PX_UNUSED(Z);
		PX_ASSERT(impulse.pad0 == 0.f && impulse.pad1 == 0.f);

		//impulse lin is contact normal, and ang is raxn. R is body2World, R(t) is world2Body
		//| R(t),	0	|
		//| R(t)*r, R(t)|
		//r is the vector from center of mass to contact point
		//p(impluse) =	|n|
		//				|0|

#if 0
		const PxTransform& body2World = mArticulationData.getPreTransform(linkID);

		//transform p(impulse) from world space to the local space of linkId
		const Cm::SpatialVectorF impl(body2World.rotateInv(impulse.linear), body2World.rotateInv(impulse.angular));

		getZ(linkID, mArticulationData, Z, impl);

		const bool fixBase = mArticulationData.getArticulationFlags() & PxArticulationFlag::eFIX_BASE;
		
		const Cm::SpatialVectorF deltaV = getDeltaV(fixBase, linkID, mArticulationData, Z);
		PX_ASSERT(deltaV.pad0 == 0.f && deltaV.pad1 == 0.f);
		
		////Cm::SpatialVectorF resp = mArticulationData.getImpulseResponseMatrix()[linkID].getResponse(Cm::SpatialVectorF(impulse.linear, impulse.angular));
		//Cm::SpatialVectorF resp = mArticulationData.getImpulseResponseMatrix()[linkID].getResponse(impl);

		//Cm::SpatialVectorF test = resp - deltaV;

		//PX_ASSERT(test.magnitude() < 1e-5f);

		//this is in world space
		deltaVV.linear = body2World.rotate(deltaV.bottom);
		deltaVV.angular = body2World.rotate(deltaV.top);

#else
		Cm::SpatialVectorF deltaV = mArticulationData.getImpulseResponseMatrixWorld()[linkID].getResponse(reinterpret_cast<const Cm::SpatialVectorF&>(impulse));

		deltaVV.linear = deltaV.bottom;
		deltaVV.angular = deltaV.top;		
#endif

		
	}

	void FeatherstoneArticulation::getImpulseResponse(
		PxU32 linkID,
		Cm::SpatialVectorV* /*Z*/,
		const Cm::SpatialVectorV& impulse,
		Cm::SpatialVectorV& deltaVV) const
	{
#if 0
		const PxTransform& body2World = mArticulationData.getPreTransform(linkID);

		QuatV rot = QuatVLoadU(&body2World.q.x);

		Cm::SpatialVectorV impl(QuatRotateInv(rot, impulse.linear), QuatRotateInv(rot, impulse.angular));

		//transform p(impluse) from world space to the local space of linkId

		//Cm::SpatialVectorF impl(impulse.linear, impulse.angular);
		Cm::SpatialVectorV deltaV = mArticulationData.getImpulseResponseMatrix()[linkID].getResponse(impl);
		deltaVV.linear = QuatRotate(rot, deltaV.angular);
		deltaVV.angular = QuatRotate(rot, deltaV.linear);
#else
		Cm::SpatialVectorV deltaV = mArticulationData.getImpulseResponseMatrixWorld()[linkID].getResponse(impulse);
		deltaVV.linear = deltaV.angular;
		deltaVV.angular = deltaV.linear;
#endif
	}

	//This will return world space SpatialVectorV
	Cm::SpatialVectorV FeatherstoneArticulation::getLinkVelocity(const PxU32 linkID) const
	{
		//This is in the world space
		const Cm::SpatialVectorF& motionVelocity = mArticulationData.getMotionVelocity(linkID);

		Cm::SpatialVectorV velocity;
		velocity.linear = V3LoadA(motionVelocity.bottom);
		velocity.angular = V3LoadA(motionVelocity.top);

		return velocity;
	}

	Cm::SpatialVectorV FeatherstoneArticulation::getLinkMotionVector(const PxU32 linkID) const
	{
		const Cm::SpatialVectorF& motionVector = mArticulationData.getDeltaMotionVector(linkID);

		Cm::SpatialVectorV velocity;
		velocity.linear = V3LoadU(motionVector.bottom);
		velocity.angular = V3LoadU(motionVector.top);

		return velocity;
	}

	//this is called by island gen to determine whether the articulation should be awake or sleep
	Cm::SpatialVector FeatherstoneArticulation::getMotionVelocity(const PxU32 linkID) const
	{
		//This is in the world space
		const Cm::SpatialVectorF& motionVelocity = mArticulationData.getMotionVelocity(linkID);
		return Cm::SpatialVector(motionVelocity.bottom, motionVelocity.top);
	}

	Cm::SpatialVector FeatherstoneArticulation::getMotionAcceleration(const PxU32 linkID) const
	{
		const PxReal dt = mArticulationData.getDt();
		return recomputeAcceleration(linkID, dt);
	}

	void FeatherstoneArticulation::fillIndexedManager(const PxU32 linkId, Dy::ArticulationLinkHandle& handle, PxU8& indexType)
	{
		ArticulationLink& link = mArticulationData.getLink(linkId);

		//turn the kinematic link to static for the solver
		if (link.bodyCore->kinematicLink)
		{
			indexType = PxsIndexedInteraction::eWORLD;
		}
		else
		{
			indexType = PxsIndexedInteraction::eARTICULATION;
			handle = size_t(this) | linkId;
		}
	}

	PxReal FeatherstoneArticulation::getLinkMaxPenBias(const PxU32 linkID) const
	{
		return mArticulationData.getLinkData(linkID).maxPenBias;
	}

	void PxcFsFlushVelocity(FeatherstoneArticulation& articulation, Cm::SpatialVectorF* deltaV)
	{
		ArticulationData& data = articulation.mArticulationData;
		const bool fixBase = data.getArticulationFlags() & PxArticulationFlag::eFIX_BASE;
		Cm::SpatialVectorF* motionVelocities = data.getMotionVelocities();
		Cm::SpatialVectorF* deferredZ = data.getSpatialZAVectors();
		ArticulationLink* links = data.getLinks();
		ArticulationJointCoreData* jointData = data.getJointData();
		ArticulationLinkData* linkData = data.getLinkData();

		//PxTransform* poses = data.getAccumulatedPoses();
		//const PxTransform* poses = data.getPreTransform();

		//This will be zero at the begining of the frame
		PxReal* jointDeltaVelocities = data.getJointDeltaVelocities();

		if (fixBase)
		{
			deltaV[0] = Cm::SpatialVectorF(PxVec3(0.f), PxVec3(0.f));
		}
		else
		{
			//ArticulationLink& link = links[0];

			deltaV[0] = data.getBaseInvSpatialArticulatedInertiaW() * (-deferredZ[0]);

			motionVelocities[0] += deltaV[0];

			PX_ASSERT(motionVelocities[0].isFinite());
		}

		const PxU32 linkCount = data.getLinkCount();

		for (PxU32 i = 1; i < linkCount; i++)
		{
			const ArticulationLink& tLink = links[i];
			const ArticulationJointCoreData& tJointDatum = jointData[i];

			const Cm::SpatialVectorF dV = FeatherstoneArticulation::propagateVelocityW(linkData[i].rw, data.getWorldSpatialArticulatedInertia(i),
				data.getInvStIs(i), data.getWorldMotionMatrix(i), deferredZ[i], &jointDeltaVelocities[tJointDatum.jointOffset], deltaV[tLink.parent]);

			deltaV[i] = dV;

			//const PxTransform& tBody2World = poses[i];
			motionVelocities[i] += dV;

			PX_ASSERT(motionVelocities[i].isFinite());
		}

		PxMemZero(deferredZ, sizeof(Cm::SpatialVectorF)*linkCount);
	}

	void FeatherstoneArticulation::recordDeltaMotion(const ArticulationSolverDesc& desc, 
		const PxReal dt, Cm::SpatialVectorF* deltaV, const PxReal totalInvDt)
	{
		FeatherstoneArticulation* articulation = static_cast<FeatherstoneArticulation*>(desc.articulation);
		ArticulationData& data = articulation->mArticulationData;
		const PxU32 linkCount = data.getLinkCount();

		if(data.mJointDirty)
			PxcFsFlushVelocity(*articulation, deltaV);

		Cm::SpatialVectorF* deltaMotion = data.getDeltaMotionVector();
		const Cm::SpatialVectorF* motionVeloties = data.getMotionVelocities();
		PX_UNUSED(motionVeloties);

		PxReal* jointPosition = data.getJointPositions();
		PxReal* jointVelocities = data.getJointVelocities();
		PxReal* jointDeltaVelocities = data.getJointDeltaVelocities();
		PxReal* jointAcceleration = data.getJointAccelerations();
		
		//data.mAccumulatedDt += dt;
		data.setDt(dt);

		const bool fixBase = data.getArticulationFlags() & PxArticulationFlag::eFIX_BASE;

		if (!fixBase)
		{
			const Cm::SpatialVectorF& motionVelocity = data.getMotionVelocity(0);
			PX_ASSERT(motionVelocity.top.isFinite());
			PX_ASSERT(motionVelocity.bottom.isFinite());

			const PxTransform preTrans = data.mAccumulatedPoses[0];

			const PxVec3 lin = motionVelocity.bottom;
			const PxVec3 ang = motionVelocity.top;

			const PxVec3 newP = preTrans.p + lin * dt;

			const PxTransform newPose = PxTransform(newP, Ps::exp(ang*dt) * preTrans.q);

			//PxVec3 lin, ang;
			/*calculateNewVelocity(newPose, data.mPreTransform[0],
				1.f, lin, ang);		*/	

			data.mAccumulatedPoses[0] = newPose;

			PxQuat dq = newPose.q * data.mPreTransform[0].q.getConjugate();

			if (dq.w < 0.f)
				dq = -dq;

			data.mDeltaQ[0] = dq;

			deltaMotion[0] += motionVelocity * dt;
			/*deltaMotion[0].top = ang;
			deltaMotion[0].bottom = lin;*/
		}

		for (PxU32 linkID = 1; linkID < linkCount; linkID++)
		{
			ArticulationJointCoreData& jointDatum = data.getJointData(linkID);

			for (PxU32 i = 0, idx = jointDatum.jointOffset; i < jointDatum.dof; ++i, idx++)
			{
				jointAcceleration[idx] += jointDeltaVelocities[idx] * totalInvDt;
			}
			
			const PxTransform newPose = articulation->propagateTransform(linkID, data.getLinks(), jointDatum, data.getMotionVelocities(),
				dt, data.mAccumulatedPoses[data.getLink(linkID).parent], data.mAccumulatedPoses[linkID], 
				jointVelocities, jointDeltaVelocities, jointPosition, data.getMotionMatrix(linkID), data.getWorldMotionMatrix(linkID));

			//data.mDeltaQ[linkID] = data.mPreTransform[linkID].q.getConjugate() * newPose.q;
			PxQuat dq = newPose.q * data.mPreTransform[linkID].q.getConjugate();

			if(dq.w < 0.f)
				dq = -dq;

			data.mDeltaQ[linkID] = dq;

			

			/*PxVec3 lin, ang;
			calculateNewVelocity(newPose, data.mPreTransform[linkID],
				1.f, lin, ang);*/

			PxVec3 lin = (newPose.p - data.mPreTransform[linkID].p);
			
			//deltaMotion[linkID].top = ang;// motionVeloties[linkID].top * dt;
			deltaMotion[linkID].top += motionVeloties[linkID].top * dt;
			deltaMotion[linkID].bottom = lin;// motionVeloties[linkID].top * dt;

			//Record the new current pose
			data.mAccumulatedPoses[linkID] = newPose;
		}
	}

	void FeatherstoneArticulation::deltaMotionToMotionVelocity(const ArticulationSolverDesc& desc, PxReal invDt)
	{
		FeatherstoneArticulation* articulation = static_cast<FeatherstoneArticulation*>(desc.articulation);
		ArticulationData& data = articulation->mArticulationData;
		const PxU32 linkCount = data.getLinkCount();
		const Cm::SpatialVectorF* deltaMotion = data.getDeltaMotionVector();

		for (PxU32 linkID = 0; linkID<linkCount; linkID++)
		{
			Cm::SpatialVectorF& v = data.getMotionVelocity(linkID);

			Cm::SpatialVectorF delta = deltaMotion[linkID] * invDt;

			v = delta;

			desc.motionVelocity[linkID] = reinterpret_cast<Cm::SpatialVectorV&>(delta);
		}
	}

	//This is used in the solveExt1D, solveExtContact
	Cm::SpatialVectorV FeatherstoneArticulation::pxcFsGetVelocity(PxU32 linkID)
	{
		Cm::SpatialVectorF* deferredZ = mArticulationData.getSpatialZAVectors();

		const bool fixBase = mArticulationData.getArticulationFlags() & PxArticulationFlag::eFIX_BASE;
	
		ArticulationLink* links = mArticulationData.getLinks();

		Cm::SpatialVectorF deltaV(PxVec3(0.f), PxVec3(0.f));

		if (!fixBase)
		{
			//deltaV = mArticulationData.mBaseInvSpatialArticulatedInertia * (-deferredZ[0]);
			//DeferredZ now in world space!
			deltaV = mArticulationData.mBaseInvSpatialArticulatedInertiaW * (-deferredZ[0]);
		}

		for (ArticulationBitField i = links[linkID].pathToRoot - 1; i; i &= (i - 1))
		{
			//index of child of link h on path to link linkID
			const PxU32 index = ArticulationLowestSetBit(i);
			PX_ASSERT(links[index].parent < index);
			
			Dy::ArticulationLinkData& linkDatum = mArticulationData.getLinkData(index);

			deltaV = propagateVelocityTestImpulseW(linkDatum.rw, mArticulationData.mWorldSpatialArticulatedInertia[index],
				mArticulationData.mInvStIs[index], mArticulationData.mWorldMotionMatrix[index], deferredZ[index], deltaV);
		}

		//ArticulationLink& link = mArticulationData.getLink(linkID);
		//PxTransform& body2World = link.bodyCore->body2World;
		//PxTransform& body2World = mArticulationData.getAccumulatedPoses()[linkID];
		//const PxTransform& body2World = mArticulationData.getPreTransform(linkID);
		Cm::SpatialVectorF vel = mArticulationData.getMotionVelocity(linkID) + deltaV;

		return Cm::SpatialVector(vel.bottom, vel.top);
	}

	void FeatherstoneArticulation::pxcFsGetVelocities(PxU32 linkID, PxU32 linkID1, Cm::SpatialVectorV& v0, Cm::SpatialVectorV& v1)
	{
		{
			Cm::SpatialVectorF* deferredZ = mArticulationData.getSpatialZAVectors();

			const bool fixBase = mArticulationData.getArticulationFlags() & PxArticulationFlag::eFIX_BASE;

			ArticulationLink* links = mArticulationData.getLinks();

			Cm::SpatialVectorF deltaV(PxVec3(0.f), PxVec3(0.f));

			if (!fixBase)
			{
				//deltaV = mArticulationData.mBaseInvSpatialArticulatedInertia * (-deferredZ[0]);
				deltaV = mArticulationData.mBaseInvSpatialArticulatedInertiaW * (-deferredZ[0]);
			}

			PxU64 common = links[linkID].pathToRoot & links[linkID1].pathToRoot;
			PxU64 exclusive0 = links[linkID].pathToRoot ^ common;
			PxU64 exclusive1 = links[linkID1].pathToRoot ^ common;

			for (ArticulationBitField i = common - 1; i; i &= (i - 1))
			{
				//index of child of link h on path to link linkID
				const PxU32 index = ArticulationLowestSetBit(i);
				PX_ASSERT(links[index].parent < index);

				Dy::ArticulationLinkData& linkDatum = mArticulationData.getLinkData(index);
				deltaV = propagateVelocityTestImpulseW(linkDatum.rw, mArticulationData.mWorldSpatialArticulatedInertia[index],
					mArticulationData.mInvStIs[index], mArticulationData.mWorldMotionMatrix[index], deferredZ[index], deltaV);
			}

			Cm::SpatialVectorF deltaV1 = deltaV;

			for (ArticulationBitField i = exclusive0; i; i &= (i - 1))
			{
				//index of child of link h on path to link linkID
				const PxU32 index = ArticulationLowestSetBit(i);
				PX_ASSERT(links[index].parent < index);
				Dy::ArticulationLinkData& linkDatum = mArticulationData.getLinkData(index);
				deltaV = propagateVelocityTestImpulseW(linkDatum.rw, mArticulationData.mWorldSpatialArticulatedInertia[index],
					mArticulationData.mInvStIs[index], mArticulationData.mWorldMotionMatrix[index], deferredZ[index], deltaV);
			}

			for (ArticulationBitField i = exclusive1; i; i &= (i - 1))
			{
				//index of child of link h on path to link linkID
				const PxU32 index = ArticulationLowestSetBit(i);
				PX_ASSERT(links[index].parent < index);
				Dy::ArticulationLinkData& linkDatum = mArticulationData.getLinkData(index);
				deltaV1 = propagateVelocityTestImpulseW(linkDatum.rw, mArticulationData.mWorldSpatialArticulatedInertia[index],
					mArticulationData.mInvStIs[index], mArticulationData.mWorldMotionMatrix[index], deferredZ[index], deltaV1);
			}

			Cm::SpatialVectorF vel = mArticulationData.getMotionVelocity(linkID) + deltaV;

			v0 = Cm::SpatialVector(vel.bottom, vel.top);

			Cm::SpatialVectorF vel1 = mArticulationData.getMotionVelocity(linkID1) + deltaV1;

			v1 = Cm::SpatialVector(vel1.bottom, vel1.top);
		}
	}

	/*Cm::SpatialVectorV FeatherstoneArticulation::pxcFsGetVelocity(PxU32 linkID)
	{

	Cm::SpatialVectorF& vel = mArticulationData.getMotionVelocity(linkID);

	return Cm::SpatialVector(vel.bottom, vel.top);
	}*/

	Cm::SpatialVectorV FeatherstoneArticulation::pxcFsGetVelocityTGS(PxU32 linkID)
	{
		return getLinkVelocity(linkID);
	}

	//This is used in the solveExt1D, solveExtContact
	void FeatherstoneArticulation::pxcFsApplyImpulse(PxU32 linkID, 
		Ps::aos::Vec3V linear, Ps::aos::Vec3V angular,
		Cm::SpatialVectorF* /*Z*/, Cm::SpatialVectorF* /*deltaV*/)
	{
		const ArticulationSolverDesc* desc = &mSolverDesc;

		ArticulationLink* links = static_cast<ArticulationLink*>(desc->links);

		//initialize all zero acceration impulse to be zero
		ArticulationData& data = mArticulationData;

		Cm::SpatialVectorF* deferredZ = data.getSpatialZAVectors();

		//This will be zero at the begining of the frame
		//PxReal* jointDeltaVelocities = data.getJointDeltaVelocities();

		//Cm::SpatialVectorF* motionVelocity = data.getMotionVelocities();

		ArticulationLinkData* linkData = data.getLinkData();

		data.mJointDirty = true;
		
		//set all zero acceleration forces to be zero
		//PxMemZero(Z, sizeof(Cm::SpatialVectorF) * desc->linkCount);

		//impulse is in world space
		Cm::SpatialVector impulse;
		V4StoreA(Vec4V_From_Vec3V(angular), &impulse.angular.x);
		V4StoreA(Vec4V_From_Vec3V(linear), &impulse.linear.x);

		//transform p(impluse) from world space to the local space of link
		//ArticulationLink& collidedlink = links[linkID];
		//PxTransform& body2World = collidedlink.bodyCore->body2World;
		//PxTransform& body2World = data.getAccumulatedPoses()[linkID];
		//PxTransform& body2World = data.getPreTransform(linkID);

		//Cm::SpatialVectorF Z0(-body2World.rotateInv(impulse.linear), -body2World.rotateInv(impulse.angular));
		Cm::SpatialVectorF Z0(-impulse.linear, -impulse.angular);
		deferredZ[linkID] += Z0;

		for (PxU32 i = linkID; i; i = links[i].parent)
		{
			ArticulationLink& tLink = links[i];
			ArticulationLinkData& tLinkDatum = linkData[i];
			Z0 = propagateImpulseW(data.mIsInvDW[i], tLinkDatum.rw, data.mWorldMotionMatrix[i], Z0);
			deferredZ[tLink.parent] += Z0;
		}
	}

	void FeatherstoneArticulation::pxcFsApplyImpulses(Cm::SpatialVectorF* Z)
	{
		ArticulationLink* links = mArticulationData.getLinks();

		//initialize all zero acceration impulse to be zero
		ArticulationData& data = mArticulationData;

		const PxU32 linkCount = mArticulationData.getLinkCount();

		Cm::SpatialVectorF* deferredZ = data.getSpatialZAVectors();

		const PxU32 startIndex = PxU32(linkCount - 1);

		data.mJointDirty = true;

		for (PxU32 linkID = startIndex; linkID > 0; --linkID)
		{
			ArticulationLink& tLink = links[linkID];
			ArticulationLinkData& linkDatum = mArticulationData.getLinkData(linkID);

			Z[tLink.parent] += propagateImpulseW(data.mIsInvDW[linkID], linkDatum.rw, data.mWorldMotionMatrix[linkID], Z[linkID]);
			deferredZ[linkID] += Z[linkID];
		}

		deferredZ[0] += Z[0];
	}

	void FeatherstoneArticulation::pxcFsApplyImpulses(PxU32 linkID, const Ps::aos::Vec3V& linear,
		const Ps::aos::Vec3V& angular, PxU32 linkID2, const Ps::aos::Vec3V& linear2,
		const Ps::aos::Vec3V& angular2, Cm::SpatialVectorF* /*Z*/, Cm::SpatialVectorF* /*deltaV*/)
	{
		if (0)
		{
			pxcFsApplyImpulse(linkID, linear, angular, NULL, NULL);
			pxcFsApplyImpulse(linkID2, linear2, angular2, NULL, NULL);
		}
		else
		{
			const ArticulationSolverDesc* desc = &mSolverDesc;
			ArticulationData& data = mArticulationData;
			ArticulationLinkData* linkData = data.getLinkData();
			data.mJointDirty = true;
			Cm::SpatialVectorF* deferredZ = data.getSpatialZAVectors();
			ArticulationLink* links = static_cast<ArticulationLink*>(desc->links);

			//impulse is in world space
			Cm::SpatialVector impulse0;
			V3StoreU(angular, impulse0.angular);
			V3StoreU(linear, impulse0.linear);

			Cm::SpatialVector impulse1;
			V3StoreU(angular2, impulse1.angular);
			V3StoreU(linear2, impulse1.linear);

			PxU64 commonId = links[linkID].pathToRoot & links[linkID2].pathToRoot;
			PxU32 commonLink = Dy::ArticulationHighestSetBit(commonId); //Now, work from one to that common, then the other to that common, then go from there upwards...

			Cm::SpatialVectorF Z1(-impulse0.linear, -impulse0.angular);
			Cm::SpatialVectorF Z2(-impulse1.linear, -impulse1.angular);

			deferredZ[linkID2] += Z2;

			//The common link will either be linkID2, or its ancestors.
			//The common link cannot be an index before either linkID2 or linkID
			for (PxU32 i = linkID2; i != commonLink; i = links[i].parent)
			{
				ArticulationLink& tLink = links[i];
				Z2 = propagateImpulseW(data.mIsInvDW[i], linkData[i].rw, data.mWorldMotionMatrix[i], Z2);
				deferredZ[tLink.parent] += Z2;
			}

			deferredZ[linkID] += Z1;

			for (PxU32 i = linkID; i != commonLink; i = links[i].parent)
			{
				ArticulationLink& tLink = links[i];
				Z1 = propagateImpulseW(data.mIsInvDW[i], linkData[i].rw, data.mWorldMotionMatrix[i], Z1);
				deferredZ[tLink.parent] += Z1;
			}

			Cm::SpatialVectorF ZCommon = Z1 + Z2;

			for (PxU32 i = commonLink; i; i = links[i].parent)
			{
				ArticulationLink& tLink = links[i];
				ZCommon = propagateImpulseW(data.mIsInvDW[i], linkData[i].rw, data.mWorldMotionMatrix[i], ZCommon);
				deferredZ[tLink.parent] += ZCommon;
			}
		}
	}

	//Z is the link space(drag force)
	void FeatherstoneArticulation::applyImpulses(Cm::SpatialVectorF* Z, Cm::SpatialVectorF* deltaV)
	{
		ArticulationLink* links = mArticulationData.getLinks();

		//initialize all zero acceration impulse to be zero
		ArticulationData& data = mArticulationData;
		

		const PxU32 linkCount = mArticulationData.getLinkCount();

		const PxU32 startIndex = PxU32(linkCount - 1);

		for (PxU32 linkID = startIndex; linkID > 0; --linkID)
		{
			ArticulationLink& tLink = links[linkID];
			ArticulationLinkData& linkDatum = data.getLinkData(linkID);
			
			Z[tLink.parent] += propagateImpulseW(data.mIsInvDW[linkID], linkDatum.rw, data.mWorldMotionMatrix[linkID], Z[linkID]);
		}

		getDeltaV(Z, deltaV);
	}

	void FeatherstoneArticulation::getDeltaV(Cm::SpatialVectorF* Z, Cm::SpatialVectorF* deltaV)
	{
		const bool fixBase = mArticulationData.getArticulationFlags() & PxArticulationFlag::eFIX_BASE;
		Cm::SpatialVectorF* motionVelocities = mArticulationData.getMotionVelocities();
		ArticulationLink* links = mArticulationData.getLinks();
		ArticulationJointCoreData* jointData = mArticulationData.getJointData();

		//This will be zero at the begining of the frame
		PxReal* jointDeltaVelocities = mArticulationData.getJointDeltaVelocities();

		if (fixBase)
		{
			deltaV[0] = Cm::SpatialVectorF(PxVec3(0.f), PxVec3(0.f));
		}
		else
		{

			deltaV[0] = mArticulationData.mBaseInvSpatialArticulatedInertiaW * (-Z[0]);
			motionVelocities[0] += deltaV[0];

			PX_ASSERT(motionVelocities[0].isFinite());
		}

		const PxU32 linkCount = mArticulationData.getLinkCount();
		
		for (PxU32 i = 1; i < linkCount; i++)
		{
			ArticulationLink& tLink = links[i];
			ArticulationJointCoreData& tJointDatum = jointData[i];
			Cm::SpatialVectorF dV = propagateVelocityW(mArticulationData.mLinksData[i].rw, mArticulationData.mWorldSpatialArticulatedInertia[i], 
				mArticulationData.mInvStIs[i], mArticulationData.mWorldMotionMatrix[i], Z[i], &jointDeltaVelocities[tJointDatum.jointOffset], deltaV[tLink.parent]);

			deltaV[i] = dV;

			motionVelocities[i] += dV;

			PX_ASSERT(motionVelocities[i].isFinite());
		}
	}

	PxQuat computeSphericalJointPositions(const PxQuat& relativeQuat,
		const PxQuat& newRot, const PxQuat& pBody2WorldRot,
		PxReal* jPositions, const SpatialSubspaceMatrix& motionMatrix);

	PxTransform FeatherstoneArticulation::propagateTransform(const PxU32 linkID, ArticulationLink* links,
		ArticulationJointCoreData& jointDatum, Cm::SpatialVectorF* motionVelocities, const PxReal dt, const PxTransform& pBody2World, 
		const PxTransform& currentTransform, PxReal* jointVelocities, PxReal* jointDeltaVelocities, PxReal* jointPositions,
		const SpatialSubspaceMatrix& motionMatrix, const SpatialSubspaceMatrix& worldMotionMatrix)
	{
		ArticulationLink& link = links[linkID];

		const PxQuat relativeQuat = mArticulationData.mRelativeQuat[linkID];
		
		ArticulationJointCore* joint = link.inboundJoint;
		
		PxReal* jVelocity = &jointVelocities[jointDatum.jointOffset];
		PxReal* jDeltaVelocity = &jointDeltaVelocities[jointDatum.jointOffset];
		PxReal* jPosition = &jointPositions[jointDatum.jointOffset];
		
		PxQuat newParentToChild;
		PxQuat newWorldQ;
		PxVec3 r;
		
		const PxVec3 childOffset = -joint->childPose.p;
		const PxVec3 parentOffset = joint->parentPose.p;
		
		switch (joint->jointType)
		{
		case PxArticulationJointType::ePRISMATIC:
		{
			PxReal tJointPosition = jPosition[0] + (jVelocity[0] + jDeltaVelocity[0]) * dt;

			const PxU32 dofId = link.inboundJoint->dofIds[0];

			if (link.inboundJoint->motion[dofId] == PxArticulationMotion::eLIMITED)
			{
				if (tJointPosition < (link.inboundJoint->limits[dofId].low))
					tJointPosition = link.inboundJoint->limits[dofId].low;
				if (tJointPosition >(link.inboundJoint->limits[dofId].high))
					tJointPosition = link.inboundJoint->limits[dofId].high;
			}

			jPosition[0] = tJointPosition;
			jVelocity[0] += jDeltaVelocity[0];
			jDeltaVelocity[0] = 0.f;

			newParentToChild = relativeQuat;
			const PxVec3 e = newParentToChild.rotate(parentOffset);
			const PxVec3 d = childOffset;
		
			r = e + d + mArticulationData.mMotionMatrix[linkID][0].bottom * tJointPosition;
			break;
		}
		case PxArticulationJointType::eREVOLUTE:
		{
			PxReal tJointPosition = jPosition[0] + (jVelocity[0] + jDeltaVelocity[0]) * dt;

			if (link.inboundJoint->twistLimited)
			{
				if (tJointPosition < (link.inboundJoint->limits[link.inboundJoint->dofIds[0]].low))
					tJointPosition = link.inboundJoint->limits[link.inboundJoint->dofIds[0]].low;
				if (tJointPosition >(link.inboundJoint->limits[link.inboundJoint->dofIds[0]].high))
					tJointPosition = link.inboundJoint->limits[link.inboundJoint->dofIds[0]].high;
			}

			jPosition[0] = tJointPosition;
			jVelocity[0] += jDeltaVelocity[0];
			jDeltaVelocity[0] = 0.f;
		
			const PxVec3& u = mArticulationData.mMotionMatrix[linkID][0].top;
		
			PxQuat jointRotation = PxQuat(-tJointPosition, u);
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
				PxQuat jointRotation(PxIdentity);
				for (PxU32 i = 0; i < jointDatum.dof; ++i)
				{
					const PxReal delta = (jVelocity[i] + jDeltaVelocity[i]) * dt;

					jVelocity[i] += jDeltaVelocity[i];

					PxReal jPos = jPosition[i] + delta;

					jPosition[i] = jPos;

					jDeltaVelocity[i] = 0.f;

					const PxVec3& u = mArticulationData.mMotionMatrix[linkID][i].top;

					PxQuat jRotation = PxQuat(-jPosition[i], u);
					if (jRotation.w < 0)	//shortest angle.
						jRotation = -jRotation;

					jointRotation = jointRotation * jRotation;	
				}
				newParentToChild = (jointRotation * relativeQuat).getNormalized();
				const PxVec3 e = newParentToChild.rotate(parentOffset);
				const PxVec3 d = childOffset;
				r = e + d;

				PX_ASSERT(r.isFinite());
			}
			else
			{
				PxVec3 worldAngVel = motionVelocities[linkID].top;

				newWorldQ = Ps::exp(worldAngVel*dt) * currentTransform.q;

				newParentToChild = computeSphericalJointPositions(mArticulationData.mRelativeQuat[linkID], newWorldQ,
					pBody2World.q, jPosition, motionMatrix);

				/*PxQuat newQ = (pBody2World.q * newParentToChild.getConjugate()).getNormalized();

				const PxQuat cB2w = newQ * joint->childPose.q;

				const PxMat33 cB2w_m(cB2w);

				const PxVec3* axis = &cB2w_m.column0;

				PxU32 dofIdx = 0;*/
				PxVec3 relAngVel = worldAngVel - motionVelocities[link.parent].top;

				for (PxU32 i = PxArticulationAxis::eTWIST; i < PxArticulationAxis::eX; ++i)
				{
					jVelocity[i] = worldMotionMatrix.getColumns()[i].top.dot(relAngVel);
				}
			}
		
			const PxVec3 e = newParentToChild.rotate(parentOffset);
			const PxVec3 d = childOffset;
			r = e + d;
		
			PX_ASSERT(r.isFinite());
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
		
		PxTransform cBody2World;
		cBody2World.q = (pBody2World.q * newParentToChild.getConjugate()).getNormalized();
		cBody2World.p = pBody2World.p + cBody2World.q.rotate(r);
		
		PX_ASSERT(cBody2World.isSane());
		
		return cBody2World;
	}

	const PxTransform& FeatherstoneArticulation::getCurrentTransform(PxU32 linkID) const
	{
		return mArticulationData.mAccumulatedPoses[linkID];
	}

	const PxQuat& FeatherstoneArticulation::getDeltaQ(PxU32 linkID) const
	{
		return mArticulationData.mDeltaQ[linkID];
	}

	////Z is the spatial acceleration impulse of links[linkID]
	//Cm::SpatialVectorF FeatherstoneArticulation::propagateVelocity(const Dy::SpatialTransform& c2p, const Dy::SpatialMatrix& spatialInertia, 
	//	const InvStIs& invStIs, const SpatialSubspaceMatrix& motionMatrix, const Cm::SpatialVectorF& Z, PxReal* jointVelocity, const Cm::SpatialVectorF& hDeltaV)
	//{
	//	const PxU32 dofCount = motionMatrix.getNumColumns();
	//	Cm::SpatialVectorF pDeltaV = c2p.transposeTransform(hDeltaV); //parent velocity change

	//	Cm::SpatialVectorF temp = spatialInertia * pDeltaV + Z;

	//	PxReal tJointDelta[6];
	//	for (PxU32 ind = 0; ind < dofCount; ++ind)
	//	{
	//		const Cm::SpatialVectorF& sa = motionMatrix[ind];
	//		tJointDelta[ind] = -sa.innerProduct(temp);
	//	}

	//	Cm::SpatialVectorF jointSpatialDeltaV(PxVec3(0.f), PxVec3(0.f));

	//	for (PxU32 ind = 0; ind < dofCount; ++ind)
	//	{
	//		PxReal jDelta = 0.f;
	//		for (PxU32 ind2 = 0; ind2 < dofCount; ++ind2)
	//		{
	//			jDelta += invStIs.invStIs[ind2][ind] * tJointDelta[ind2];
	//		}

	//		jointVelocity[ind] += jDelta;

	//		const Cm::SpatialVectorF& sa = motionMatrix[ind];
	//		jointSpatialDeltaV += sa * jDelta;
	//	}

	//	return pDeltaV + jointSpatialDeltaV;
	//}

	////This method calculate the velocity change due to collision/constraint impulse
	//Cm::SpatialVectorF FeatherstoneArticulation::propagateVelocityTestImpulse(const Dy::SpatialTransform& c2p, const Dy::SpatialMatrix& spatialInertia, 
	//	const InvStIs& invStIs, const SpatialSubspaceMatrix& motionMatrix, const Cm::SpatialVectorF& Z,
	//	const Cm::SpatialVectorF& hDeltaV)
	//{
	//	const PxU32 dofCount = motionMatrix.getNumColumns();
	//	Cm::SpatialVectorF pDeltaV = c2p.transposeTransform(hDeltaV); //parent velocity change

	//	Cm::SpatialVectorF temp = spatialInertia * pDeltaV + Z;

	//	PxReal tJointDelta[6];
	//	for (PxU32 ind = 0; ind < dofCount; ++ind)
	//	{
	//		const Cm::SpatialVectorF& sa = motionMatrix[ind];
	//		tJointDelta[ind] = -sa.innerProduct(temp);
	//	}

	//	Cm::SpatialVectorF jointSpatialDeltaV(PxVec3(0.f), PxVec3(0.f));

	//	for (PxU32 ind = 0; ind < dofCount; ++ind)
	//	{
	//		PxReal jDelta = 0.f;
	//		for (PxU32 ind2 = 0; ind2 < dofCount; ++ind2)
	//		{
	//			jDelta += invStIs.invStIs[ind2][ind] * tJointDelta[ind2];
	//		}

	//		const Cm::SpatialVectorF& sa = motionMatrix[ind];
	//		jointSpatialDeltaV += sa * jDelta;
	//	}

	//	return pDeltaV + jointSpatialDeltaV;
	//}

	PX_CUDA_CALLABLE PX_FORCE_INLINE Cm::SpatialVectorF translateImpulse(const Cm::SpatialVectorF& s, const PxVec3& offset)
	{
		return Cm::SpatialVectorF(s.top, offset.cross(s.top) + s.bottom);
	}

	//This method calculate the velocity change due to collision/constraint impulse, record joint velocity and acceleration
	Cm::SpatialVectorF FeatherstoneArticulation::propagateVelocityW(const PxVec3& c2p, const Dy::SpatialMatrix& spatialInertia,
		const InvStIs& invStIs, const SpatialSubspaceMatrix& motionMatrix, const Cm::SpatialVectorF& Z,
		PxReal* jointVelocity, const Cm::SpatialVectorF& hDeltaV)
	{
		const PxU32 dofCount = motionMatrix.getNumColumns();
		Cm::SpatialVectorF pDeltaV = translateImpulse(hDeltaV, -c2p); //parent velocity change

		Cm::SpatialVectorF temp = spatialInertia * pDeltaV + Z;

		PxReal tJointDelta[6];
		for (PxU32 ind = 0; ind < dofCount; ++ind)
		{
			const Cm::UnAlignedSpatialVector& sa = motionMatrix[ind];
			tJointDelta[ind] = -sa.innerProduct(temp);
		}

		Cm::SpatialVectorF jointSpatialDeltaV(PxVec3(0.f), PxVec3(0.f));

		for (PxU32 ind = 0; ind < dofCount; ++ind)
		{
			PxReal jDelta = 0.f;
			for (PxU32 ind2 = 0; ind2 < dofCount; ++ind2)
			{
				jDelta += invStIs.invStIs[ind2][ind] * tJointDelta[ind2];
			}

			jointVelocity[ind] += jDelta;

			const Cm::UnAlignedSpatialVector& sa = motionMatrix[ind];
			jointSpatialDeltaV.top += sa.top * jDelta;
			jointSpatialDeltaV.bottom += sa.bottom * jDelta;
		}

		return pDeltaV + jointSpatialDeltaV;
	}

	//This method calculate the velocity change due to collision/constraint impulse
	Cm::SpatialVectorF FeatherstoneArticulation::propagateVelocityTestImpulseW(const PxVec3& c2p, const Dy::SpatialMatrix& spatialInertia, const InvStIs& invStIs,
		const SpatialSubspaceMatrix& motionMatrix, const Cm::SpatialVectorF& Z, const Cm::SpatialVectorF& hDeltaV)
	{
		const PxU32 dofCount = motionMatrix.getNumColumns();
		Cm::SpatialVectorF pDeltaV = translateImpulse(hDeltaV, -c2p); //parent velocity change

		//Convert parent velocity change into an impulse
		Cm::SpatialVectorF temp = spatialInertia * pDeltaV + Z;

		PxReal tJointDelta[6];
		for (PxU32 ind = 0; ind < dofCount; ++ind)
		{
			const Cm::UnAlignedSpatialVector& sa = motionMatrix[ind];
			tJointDelta[ind] = -sa.innerProduct(temp);
		}

		Cm::SpatialVectorF jointSpatialDeltaV(PxVec3(0.f), PxVec3(0.f));

		for (PxU32 ind = 0; ind < dofCount; ++ind)
		{
			PxReal jDelta = 0.f;
			for (PxU32 ind2 = 0; ind2 < dofCount; ++ind2)
			{
				jDelta += invStIs.invStIs[ind2][ind] * tJointDelta[ind2];
			}

			const Cm::UnAlignedSpatialVector& sa = motionMatrix[ind];
			jointSpatialDeltaV.top += sa.top * jDelta;
			jointSpatialDeltaV.bottom += sa.bottom * jDelta;
		}

		return pDeltaV + jointSpatialDeltaV;
	}


	//Cm::SpatialVectorF FeatherstoneArticulation::propagateImpulse(const IsInvD& isInvD, 
	//	const SpatialTransform& childToParent, const SpatialSubspaceMatrix& motionMatrix, const Cm::SpatialVectorF& Z)
	//{
	//	const PxU32 dofCount = motionMatrix.getNumColumns();
	//	Cm::SpatialVectorF temp(PxVec3(0.f), PxVec3(0.f));

	//	for (PxU32 ind = 0; ind < dofCount; ++ind)
	//	{
	//		const Cm::SpatialVectorF& sa = motionMatrix[ind];
	//		const PxReal stZ = sa.innerProduct(Z);
	//		temp += isInvD.isInvD[ind] * stZ;
	//	}

	//	//parent space's spatial zero acceleration impulse
	//	return  childToParent * (Z - temp);
	//}

	Cm::SpatialVectorF FeatherstoneArticulation::propagateImpulseW(const IsInvD& isInvD, const PxVec3& childToParent,
		const SpatialSubspaceMatrix& motionMatrix, const Cm::SpatialVectorF& Z)
	{
		const PxU32 dofCount = motionMatrix.getNumColumns();
		Cm::SpatialVectorF temp(PxVec3(0.f), PxVec3(0.f));

		for (PxU32 ind = 0; ind < dofCount; ++ind)
		{
			const Cm::UnAlignedSpatialVector& sa = motionMatrix[ind];
			const PxReal stZ = sa.innerProduct(Z);
			temp += isInvD.isInvD[ind] * stZ;
		}

		//parent space's spatial zero acceleration impulse

		return FeatherstoneArticulation::translateSpatialVector(childToParent, (Z-temp));
		/*Cm::SpatialVectorF temp2 = (Z - temp);
		temp2.bottom += childToParent.cross(temp2.top);
		return  temp2;*/
	}

	Cm::SpatialVectorF FeatherstoneArticulation::getDeltaVWithDeltaJV(const bool fixBase, const PxU32 linkID, 
		const ArticulationData& data, Cm::SpatialVectorF* Z,
		PxReal* jointVelocities)
	{
		Cm::SpatialVectorF deltaV = Cm::SpatialVectorF::Zero();
		if (!fixBase)
		{
			//velocity change
			//SpatialMatrix inverseArticulatedInertia = hLinkDatum.spatialArticulatedInertia.getInverse();
			const SpatialMatrix& inverseArticulatedInertia = data.mBaseInvSpatialArticulatedInertiaW;
			deltaV = inverseArticulatedInertia * (-Z[0]);
		}

		for (ArticulationBitField i = data.getLink(linkID).pathToRoot - 1; i; i &= (i - 1))
		{
			const PxU32 index = ArticulationLowestSetBit(i);
			ArticulationJointCoreData& tJointDatum = data.getJointData(index);
			PxReal* jVelocity = &jointVelocities[tJointDatum.jointOffset];
			deltaV = FeatherstoneArticulation::propagateVelocityW(data.mLinksData[index].rw, data.mWorldSpatialArticulatedInertia[index], 
				data.mInvStIs[index], data.mWorldMotionMatrix[index], Z[index], jVelocity, deltaV);
		}

		return deltaV;
	}

	Cm::SpatialVectorF FeatherstoneArticulation::getDeltaV(const bool fixBase, const PxU32 linkID,
		const ArticulationData& data, Cm::SpatialVectorF* Z)
	{
		Cm::SpatialVectorF deltaV = Cm::SpatialVectorF::Zero();
		if (!fixBase)
		{
			//velocity change
			//SpatialMatrix inverseArticulatedInertia = hLinkDatum.spatialArticulatedInertia.getInverse();
			const SpatialMatrix& inverseArticulatedInertia = data.mBaseInvSpatialArticulatedInertiaW;
			deltaV = inverseArticulatedInertia * (-Z[0]);
		}

		for (ArticulationBitField i = data.getLink(linkID).pathToRoot - 1; i; i &= (i - 1))
		{
			const PxU32 index = ArticulationLowestSetBit(i);
			deltaV = FeatherstoneArticulation::propagateVelocityTestImpulseW(data.mLinksData[index].rw, data.mWorldSpatialArticulatedInertia[index],
				data.mInvStIs[index], data.mWorldMotionMatrix[index], Z[index], deltaV);
		}

		return deltaV;
	}

	void  FeatherstoneArticulation::getZ(const PxU32 linkID,
		const ArticulationData& data, Cm::SpatialVectorF* Z, 
		const Cm::SpatialVectorF& impulse)
	{
		ArticulationLink* links = data.getLinks();

		//impulse need to be in linkID space!!!
		Z[linkID] = -impulse;

		for (PxU32 i = linkID; i; i = links[i].parent)
		{
			ArticulationLink& tLink = links[i];
			Z[tLink.parent] = FeatherstoneArticulation::propagateImpulseW(data.mIsInvDW[i], data.mLinksData[i].rw,
				data.mWorldMotionMatrix[i], Z[i]);
		}
	}

	Cm::SpatialVectorF FeatherstoneArticulation::getImpulseResponseW(
		const PxU32 linkID,
		const ArticulationData& data,
		const Cm::SpatialVectorF& impulse)
	{
		return data.getImpulseResponseMatrixWorld()[linkID].getResponse(impulse);
	}

	//This method use in impulse self response. The input impulse is in the link space
	Cm::SpatialVectorF FeatherstoneArticulation::getImpulseResponseWithJ(
		const PxU32 linkID,
		const bool fixBase,
		const ArticulationData& data,
		Cm::SpatialVectorF* Z,
		const Cm::SpatialVectorF& impulse,
		PxReal* jointVelocites)
	{
		getZ(linkID, data, Z, impulse);

		return getDeltaVWithDeltaJV(fixBase, linkID, data, Z, jointVelocites);
	}

	void FeatherstoneArticulation::saveVelocity(const ArticulationSolverDesc& d, Cm::SpatialVectorF* deltaV)
	{
		FeatherstoneArticulation* arti = static_cast<FeatherstoneArticulation*>(d.articulation);
		ArticulationData& data = arti->mArticulationData;

		//update all links' motion velocity, joint delta velocity if there are contacts/constraints
		if (data.mJointDirty)
			PxcFsFlushVelocity(*arti, deltaV);

		const PxU32 linkCount = data.getLinkCount();
		//copy motion velocites
		Cm::SpatialVectorF* vels = data.getMotionVelocities();
		Cm::SpatialVectorF* posVels = data.getPosIterMotionVelocities();
		PxMemCopy(posVels, vels, sizeof(Cm::SpatialVectorF) * linkCount);
	
		//copy joint velocities
		const PxU32 dofs = data.getDofs();

		PxReal* jPosDeltaVels = data.getPosIterJointDeltaVelocities();

		const PxReal* jDeltaVels = data.getJointDeltaVelocities();

		PxMemCopy(jPosDeltaVels, jDeltaVels, sizeof(PxReal) * dofs);

		static_cast<FeatherstoneArticulation*>(d.articulation)->concludeInternalConstraints(false);

	/*	for (PxU32 i = 0; i < dofs; ++i)
		{
			PX_ASSERT(PxAbs(jPosDeltaVels[i]) < 30.f);
		}*/
	}

	void FeatherstoneArticulation::saveVelocityTGS(const ArticulationSolverDesc& d, PxReal invDtF32)
	{
		PX_UNUSED(d);
		PX_UNUSED(invDtF32);
	}

	void FeatherstoneArticulation::getImpulseSelfResponse(
		PxU32 linkID0,
		PxU32 linkID1,
		Cm::SpatialVectorF* Z,
		const Cm::SpatialVector& impulse0,
		const Cm::SpatialVector& impulse1,
		Cm::SpatialVector& deltaV0,
		Cm::SpatialVector& deltaV1) const
	{
		FeatherstoneArticulation::getImpulseSelfResponse(mArticulationData.getLinks(), mArticulationData.getArticulationFlags() & PxArticulationFlag::eFIX_BASE,
			Z, mArticulationData, linkID0, reinterpret_cast<const Cm::SpatialVectorV&>(impulse0), 
			reinterpret_cast<Cm::SpatialVectorV&>(deltaV0), linkID1, reinterpret_cast<const Cm::SpatialVectorV&>(impulse1), 
			reinterpret_cast<Cm::SpatialVectorV&>(deltaV1));
	}

	void FeatherstoneArticulation::getImpulseResponseSlow(Dy::ArticulationLink* links,
		const ArticulationData& data,
		PxU32 linkID0_,
		const Cm::SpatialVector& impulse0,
		Cm::SpatialVector& deltaV0,
		PxU32 linkID1_,
		const Cm::SpatialVector& impulse1,
		Cm::SpatialVector& deltaV1,
		Cm::SpatialVectorF* Z)
	{
		PxU32 stack[DY_ARTICULATION_MAX_SIZE];

		PxU32 i0, i1;//, ic;

		PxU32 linkID0 = linkID0_;
		PxU32 linkID1 = linkID1_;
		
		for (i0 = linkID0, i1 = linkID1; i0 != i1;)	// find common path
		{
			if (i0<i1)
				i1 = links[i1].parent;
			else
				i0 = links[i0].parent;
		}

		PxU32 common = i0;

		Cm::SpatialVectorF Z0(-impulse0.linear, -impulse0.angular);
		Cm::SpatialVectorF Z1(-impulse1.linear, -impulse1.angular);

		Z[linkID0] = Z0;
		Z[linkID1] = Z1;

		for (i0 = 0; linkID0 != common; linkID0 = links[linkID0].parent)
		{
			Z0 = FeatherstoneArticulation::propagateImpulseW(data.getWorldIsInvD(linkID0), data.getLinkData(linkID0).rw,
				data.getWorldMotionMatrix(linkID0), Z0);
			Z[links[linkID0].parent] = Z0;
			stack[i0++] = linkID0;
		}

		for (i1 = i0; linkID1 != common; linkID1 = links[linkID1].parent)
		{
			Z1 = FeatherstoneArticulation::propagateImpulseW(data.getWorldIsInvD(linkID1), data.getLinkData(linkID1).rw,
				data.getWorldMotionMatrix(linkID1), Z1);
			Z[links[linkID1].parent] = Z1;
			stack[i1++] = linkID1;
		}

		Cm::SpatialVectorF ZZ = Z0 + Z1;
		
		Cm::SpatialVectorF v = data.getImpulseResponseMatrixWorld()[common].getResponse(-ZZ); 
	
		Cm::SpatialVectorF dv1 = v;
		for (PxU32 index = i1; (index--) > i0;)
		{
			//Dy::ArticulationLinkData& tLinkDatum = data.getLinkData(stack[index]);
			const PxU32 id = stack[index];
			dv1 = FeatherstoneArticulation::propagateVelocityTestImpulseW(data.getLinkData(id).rw, data.getWorldSpatialArticulatedInertia(id),
				data.getInvStIs(id), data.getWorldMotionMatrix(id), Z[id], dv1);
		}

		Cm::SpatialVectorF dv0= v;
		for (PxU32 index = i0; (index--) > 0;)
		{
			const PxU32 id = stack[index];
			dv0 = FeatherstoneArticulation::propagateVelocityTestImpulseW(data.getLinkData(id).rw, data.getWorldSpatialArticulatedInertia(id),
				data.getInvStIs(id), data.getWorldMotionMatrix(id), Z[id], dv0);
		}

		deltaV0.linear = dv0.bottom;
		deltaV0.angular = dv0.top;

		deltaV1.linear = dv1.bottom;
		deltaV1.angular = dv1.top;
	}

	void FeatherstoneArticulation::getImpulseSelfResponse(ArticulationLink* links,
		const bool /*fixBase*/,
		Cm::SpatialVectorF* Z,
		const ArticulationData& data,
		PxU32 linkID0,
		const Cm::SpatialVectorV& impulse0,
		Cm::SpatialVectorV& deltaV0,
		PxU32 linkID1,
		const Cm::SpatialVectorV& impulse1,
		Cm::SpatialVectorV& deltaV1)
	{
		
		ArticulationLink& link = links[linkID1];
//		if (link.parent == linkID0)
//		{
//			PX_ASSERT(linkID0 == link.parent);
//			PX_ASSERT(linkID0 < linkID1);
//
//			
//			//impulse is in world space
//			Cm::SpatialVectorF imp1;
//			V4StoreA(impulse1.angular, &imp1.top.x);
//			V4StoreA(impulse1.linear, &imp1.bottom.x);
//
//			Cm::SpatialVectorF imp0;
//			V4StoreA(impulse0.angular, &imp0.top.x);
//			V4StoreA(impulse0.linear, &imp0.bottom.x);
//
//			ArticulationLink& pLink = links[linkID0];
//			PxTransform& pBody2World = pLink.bodyCore->body2World;
//
//			Cm::SpatialVectorF pImpulse(pBody2World.rotateInv(imp0.top), pBody2World.rotateInv(imp0.bottom));
//
//			PX_ASSERT(linkID0 == link.parent);
//
//			PxTransform& body2World = link.bodyCore->body2World;
//
//			//initialize child link spatial zero acceleration impulse
//			Cm::SpatialVectorF Z1(-body2World.rotateInv(imp1.bottom), -body2World.rotateInv(imp1.top));
//			//this calculate parent link spatial zero acceleration impulse
//			Cm::SpatialVectorF Z0 = FeatherstoneArticulation::propagateImpulse(data.mIsInvD[linkID1], data.mChildToParent[linkID1], 
//				data.getMotionMatrix(linkID1), Z1);
//
//			//in parent space
//			const Cm::SpatialVectorF impulseDif = pImpulse - Z0;
//
//			//calculate velocity change start from the parent link to the root
//			const Cm::SpatialVectorF delV0 = FeatherstoneArticulation::getImpulseResponse(linkID0, data.getArticulationFlags() & PxArticulationFlag::eFIX_BASE, data, Z, impulseDif);
//
//			//calculate velocity change for child link
//			const Cm::SpatialVectorF delV1 = FeatherstoneArticulation::propagateVelocityTestImpulse(data.mChildToParent[linkID1],
//				data.mSpatialArticulatedInertia[linkID1], data.mInvStIs[linkID1], data.getMotionMatrix(linkID1), Z1, delV0);
//
//			const PxVec3 lin0 = pBody2World.rotate(delV0.bottom);
//			const PxVec3 ang0 = pBody2World.rotate(delV0.top);
//			const PxVec3 lin1 = body2World.rotate(delV1.bottom);
//			const PxVec3 ang1 = body2World.rotate(delV1.top);
//
//
//			Cm::SpatialVectorF Z1W(-imp1.bottom, -imp1.top);
//
//			Cm::SpatialVectorF Z0W = FeatherstoneArticulation::propagateImpulseW(data.mIsInvDW[linkID1], data.getLinkData(linkID1).rw,
//				data.mWorldMotionMatrix[linkID1], Z1W);
//
//			//in parent space
//			const Cm::SpatialVectorF impulseDifW = imp0 - Z0W;
//
//			//calculate velocity change start from the parent link to the root
//			const Cm::SpatialVectorF delV0W = FeatherstoneArticulation::getImpulseResponseW(linkID0, data, impulseDifW);
//
//			//calculate velocity change for child link
//			const Cm::SpatialVectorF delV1W = FeatherstoneArticulation::propagateVelocityTestImpulseW(data.getLinkData(linkID1).rw,
//				data.mWorldSpatialArticulatedInertia[linkID1], data.mInvStIsW[linkID1], data.mWorldMotionMatrix[linkID1], Z1W, delV0W);
//
//
//			//translate delV0 and delV1 into world space again
//
//			PX_ASSERT((lin0 - delV0W.bottom).magnitude() < 1e-4f);
//			PX_ASSERT((ang0 - delV0W.top).magnitude() < 1e-4f);
//			PX_ASSERT((lin1 - delV1W.bottom).magnitude() < 1e-4f);
//			PX_ASSERT((ang1 - delV1W.top).magnitude() < 1e-4f);
//
//
//#if 1
//			deltaV0.linear = V3LoadU(lin0);
//			deltaV0.angular = V3LoadU(ang0);
//			deltaV1.linear = V3LoadU(lin1);
//			deltaV1.angular = V3LoadU(ang1);
//#else
//			deltaV0.linear = V3LoadA(delV0W.bottom);
//			deltaV0.angular = V3LoadA(delV0W.top);
//			deltaV1.linear = V3LoadA(delV1W.bottom);
//			deltaV1.angular = V3LoadA(delV1W.top);
//#endif
//		}

		if (link.parent == linkID0)
		{
			PX_ASSERT(linkID0 == link.parent);
			PX_ASSERT(linkID0 < linkID1);

			//impulse is in world space
			Cm::SpatialVectorF imp1;
			V4StoreA(Vec4V_From_Vec3V(impulse1.angular), &imp1.bottom.x);
			V4StoreA(Vec4V_From_Vec3V(impulse1.linear), &imp1.top.x);

			Cm::SpatialVectorF imp0;
			V4StoreA(Vec4V_From_Vec3V(impulse0.angular), &imp0.bottom.x);
			V4StoreA(Vec4V_From_Vec3V(impulse0.linear), &imp0.top.x);

			//ArticulationLink& pLink = links[linkID0];
			//PxTransform& pBody2World = pLink.bodyCore->body2World;

			//Cm::SpatialVectorF pImpulse(pBody2World.rotateInv(imp0.top), pBody2World.rotateInv(imp0.bottom));

			//PX_ASSERT(linkID0 == link.parent);

			//PxTransform& body2World = link.bodyCore->body2World;

			////initialize child link spatial zero acceleration impulse
			//Cm::SpatialVectorF Z1(-body2World.rotateInv(imp1.top), -body2World.rotateInv(imp1.bottom));
			////this calculate parent link spatial zero acceleration impulse
			//Cm::SpatialVectorF Z0 = FeatherstoneArticulation::propagateImpulse(data.mIsInvD[linkID1], data.mChildToParent[linkID1],
			//	data.getMotionMatrix(linkID1), Z1);

			////in parent space
			//const Cm::SpatialVectorF impulseDif = pImpulse - Z0;

			////calculate velocity change start from the parent link to the root
			//const Cm::SpatialVectorF delV0 = FeatherstoneArticulation::getImpulseResponse(linkID0, data.getArticulationFlags() & PxArticulationFlag::eFIX_BASE, data, Z, impulseDif);

			////calculate velocity change for child link
			//const Cm::SpatialVectorF delV1 = FeatherstoneArticulation::propagateVelocityTestImpulse(data.mChildToParent[linkID1],
			//	data.mSpatialArticulatedInertia[linkID1], data.mInvStIs[linkID1], data.getMotionMatrix(linkID1), Z1, delV0);

			////translate delV0 and delV1 into world space again
			//const PxVec3 lin0 = pBody2World.rotate(delV0.bottom);
			//const PxVec3 ang0 = pBody2World.rotate(delV0.top);
			//const PxVec3 lin1 = body2World.rotate(delV1.bottom);
			//const PxVec3 ang1 = body2World.rotate(delV1.top);
			//translate delV0 and delV1 into world space again
			/*deltaV0.linear = V3LoadU(lin0);
			deltaV0.angular = V3LoadU(ang0);
			deltaV1.linear = V3LoadU(lin1);
			deltaV1.angular = V3LoadU(ang1);*/


			Cm::SpatialVectorF Z1W(-imp1.top, -imp1.bottom);
			
			Cm::SpatialVectorF Z0W = FeatherstoneArticulation::propagateImpulseW(data.mIsInvDW[linkID1], data.getLinkData(linkID1).rw,
				data.mWorldMotionMatrix[linkID1], Z1W);
			
			//in parent space
			const Cm::SpatialVectorF impulseDifW = imp0 - Z0W;
			
			//calculate velocity change start from the parent link to the root
			const Cm::SpatialVectorF delV0W = FeatherstoneArticulation::getImpulseResponseW(linkID0, data, impulseDifW);
			
			//calculate velocity change for child link
			const Cm::SpatialVectorF delV1W = FeatherstoneArticulation::propagateVelocityTestImpulseW(data.getLinkData(linkID1).rw,
				data.mWorldSpatialArticulatedInertia[linkID1], data.mInvStIs[linkID1], data.mWorldMotionMatrix[linkID1], Z1W, delV0W);
			
			deltaV0.linear = Vec3V_From_Vec4V(V4LoadA(&delV0W.bottom.x));
			deltaV0.angular = Vec3V_From_Vec4V(V4LoadA(&delV0W.top.x));
			deltaV1.linear = Vec3V_From_Vec4V(V4LoadA(&delV1W.bottom.x));
			deltaV1.angular = Vec3V_From_Vec4V(V4LoadA(&delV1W.top.x));
			
		}
		else
		{
			getImpulseResponseSlow(links, data, linkID0, reinterpret_cast<const Cm::SpatialVector&>(impulse0), 
				reinterpret_cast<Cm::SpatialVector&>(deltaV0), linkID1, 
				reinterpret_cast<const Cm::SpatialVector&>(impulse1), reinterpret_cast<Cm::SpatialVector&>(deltaV1),
				Z);
		}
	}

	void FeatherstoneArticulation::createHardLimit(
		ArticulationLink* links,
		const bool fixBase,
		Cm::SpatialVectorF* Z,
		ArticulationData& data,
		PxU32 linkIndex,
		SolverConstraint1DExt& s,
		const PxVec3& axis,
		PxReal err,
		PxReal recipDt)
	{
		init(s, PxVec3(0), PxVec3(0), axis, axis, 0, PX_MAX_F32);

		FeatherstoneArticulation::getImpulseSelfResponse(links, fixBase, Z, data,
			links[linkIndex].parent, Cm::SpatialVector(PxVec3(0), axis), s.deltaVA,
			linkIndex, Cm::SpatialVector(PxVec3(0), -axis), s.deltaVB);

		const PxReal unitResponse = axis.dot(reinterpret_cast<PxVec3&>(s.deltaVA.angular)) - axis.dot(reinterpret_cast<PxVec3&>(s.deltaVB.angular));
		if (unitResponse<0.0f)
			Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, "Warning: articulation ill-conditioned or under severe stress, joint limit ignored");

		const PxReal recipResponse = unitResponse> DY_ARTICULATION_MIN_RESPONSE ? 1.0f / unitResponse : 0.0f;

		s.constant = recipResponse * -err * recipDt;
		s.unbiasedConstant = err>0.0f ? s.constant : 0.0f;
		s.velMultiplier = -recipResponse;
		s.impulseMultiplier = 1.0f;
	}

	void FeatherstoneArticulation::createHardLimits(
		SolverConstraint1DExt& s0,
		SolverConstraint1DExt& s1,
		const PxVec3& axis,
		PxReal err0,
		PxReal err1,
		PxReal recipDt,
		const Cm::SpatialVectorV& deltaVA,
		const Cm::SpatialVectorV& deltaVB,
		PxReal recipResponse)
	{
		init(s0, PxVec3(0), PxVec3(0), axis, axis, 0, PX_MAX_F32);
		init(s1, PxVec3(0), PxVec3(0), -axis, -axis, 0, PX_MAX_F32);

		s0.deltaVA = deltaVA;
		s0.deltaVB = deltaVB;
		s1.deltaVA = -deltaVA;
		s1.deltaVB = -deltaVB;

		s0.constant = recipResponse * -err0 * recipDt;
		s0.unbiasedConstant = err0>0.0f ? s0.constant : 0.0f;
		s0.velMultiplier = -recipResponse;
		s0.impulseMultiplier = 1.0f;

		s1.constant = recipResponse * -err1 * recipDt;
		s1.unbiasedConstant = err1>0.0f ? s1.constant : 0.0f;
		s1.velMultiplier = -recipResponse;
		s1.impulseMultiplier = 1.0f;
	}

	void FeatherstoneArticulation::createTangentialSpring(
		ArticulationLink* links,
		const bool fixBase,
		Cm::SpatialVectorF* Z,
		ArticulationData& data,
		PxU32 linkIndex,
		SolverConstraint1DExt& s,
		const PxVec3& axis,
		PxReal stiffness,
		PxReal damping,
		PxReal dt)
	{
		init(s, PxVec3(0), PxVec3(0), axis, axis, -PX_MAX_F32, PX_MAX_F32);

		Cm::SpatialVector axis6(PxVec3(0), axis);
		PxU32 parent = links[linkIndex].parent;
		FeatherstoneArticulation::getImpulseSelfResponse(links, fixBase, Z, data, parent, axis6, s.deltaVA, linkIndex, -axis6, s.deltaVB);

		const PxReal unitResponse = axis.dot(reinterpret_cast<PxVec3&>(s.deltaVA.angular)) - axis.dot(reinterpret_cast<PxVec3&>(s.deltaVB.angular));
		if (unitResponse<0.0f)
			Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, "Warning: articulation ill-conditioned or under severe stress, tangential spring ignored");
		const PxReal recipResponse = unitResponse> DY_ARTICULATION_MIN_RESPONSE ? 1.0f / unitResponse : 0.0f;

		// this is a specialization of the spring code in setSolverConstants() for acceleration springs.
		// general case is  b = dt * (c.mods.spring.damping * c.velocityTarget - c.mods.spring.stiffness * geomError);
		// but geomError and velocityTarget are both zero

		const PxReal a = dt * dt * stiffness + dt * damping;
		const PxReal x = 1.0f / (1.0f + a);
		s.constant = s.unbiasedConstant = 0.0f;
		s.velMultiplier = -x * recipResponse * a;
		s.impulseMultiplier = 1.0f - x;
	}

	void createDriveOrLimit(Dy::SolverConstraint1DExt& c, PxReal bias, const PxReal minImpulse, const PxReal maxImpulse,
		bool keepBias, bool isLimit, const Cm::SpatialVectorV& deltaVA, const Cm::SpatialVectorV& deltaVB, const PxReal recipResponse)
	{
		c.deltaVA = deltaVA;
		c.deltaVB = deltaVB;

		c.minImpulse = minImpulse;
		c.maxImpulse = maxImpulse;

		c.velMultiplier = -recipResponse;
		c.impulseMultiplier = 1.0f;

		{
			// see usage of 'for internal use' in preprocessRows()					
			c.constant = recipResponse * (-bias);
			if (!keepBias && (!isLimit || bias < 0.f))
				c.unbiasedConstant = 0.f;
			else
				c.unbiasedConstant = c.constant;
		}
		c.appliedForce = 0.f;
		c.flags = 0;
		c.ang0Writeback = c.ang0;
	}

	void createSpringDrive(Dy::SolverConstraint1DExt& c, PxReal error, PxReal targetVelocity, PxReal maxImpulse,
		PxReal stiffness, PxReal damping, PxReal dt, const Cm::SpatialVectorV& deltaVA, const Cm::SpatialVectorV& deltaVB,
		const PxReal unitResponse)
	{
		c.deltaVA = deltaVA;
		c.deltaVB = deltaVB;
		
		const PxReal a = dt * dt * stiffness + dt * damping;
		const PxReal b = dt * (damping * targetVelocity - stiffness * error);

		const PxReal x = 1.0f / (1.0f + a*unitResponse);
		c.constant = c.unbiasedConstant = x * b;
		c.velMultiplier = -x*a;
		c.impulseMultiplier = 1.0f - x;

		c.minImpulse = -maxImpulse;
		c.maxImpulse = maxImpulse;

		c.appliedForce = 0.f;
		c.flags = 0;
		c.ang0Writeback = c.ang0;
	}

	void createSpringDrive(Dy::SolverConstraint1DExtStep& c, PxReal error, PxReal targetVelocity, PxReal maxImpulse,
		PxReal stiffness, PxReal damping, PxReal dt, PxReal totalDt, const Cm::SpatialVectorV& deltaVA, const Cm::SpatialVectorV& deltaVB,
		PxReal unitResponse)
	{
		PX_UNUSED(dt);

		c.deltaVA = deltaVA;
		c.deltaVB = deltaVB;

		const PxReal a2 = totalDt *  (totalDt*stiffness + damping);
		const PxReal b = totalDt * (damping * targetVelocity - stiffness * error);

		const PxReal x = 1.0f / (1.0f + a2*unitResponse);

		c.velTarget = x * b * unitResponse;
		c.velMultiplier = -x*a2 * unitResponse;
		c.biasScale = 0.f;//c.velMultiplier / dt;
		c.impulseMultiplier = 1.0f -x; //KS - add back in -x term?

		c.minImpulse = -maxImpulse;
		c.maxImpulse = maxImpulse;
		c.recipResponse = unitResponse > DY_ARTICULATION_MIN_RESPONSE ? 1.f / unitResponse : 0.f;
		c.error = 0.f;
		c.maxBias = 100.f;
		c.angularErrorScale = 1.f;

		c.appliedForce = 0.f;
		c.flags = 0;
	}

	void createDriveOrLimit(Dy::SolverConstraint1DExtStep& c, PxReal error, const PxReal minImpulse, const PxReal maxImpulse,
		bool keepBias, PxReal maxBias, const PxReal velTarget, const PxReal biasCoefficient, bool isLimit,
		const Cm::SpatialVectorV& deltaVA, const Cm::SpatialVectorV& deltaVB, PxReal recipResponse)
	{
		c.deltaVA = deltaVA;
		c.deltaVB = deltaVB;

		c.minImpulse = minImpulse;
		c.maxImpulse = maxImpulse;

		c.velMultiplier = -1.f;
		c.impulseMultiplier = 1.0f;

		c.error = error;
		c.velTarget = velTarget;
		c.biasScale = -biasCoefficient;
		c.recipResponse = recipResponse;
		c.maxBias = maxBias;
		c.appliedForce = 0.f;

		c.flags = PxU32(keepBias || (isLimit && error > 0.f) ? DY_SC_FLAG_KEEP_BIAS | DY_SC_FLAG_INEQUALITY : 0);
		c.angularErrorScale = 0.f;
	}

	bool FeatherstoneArticulation::storeStaticConstraint(const PxSolverConstraintDesc& desc)
	{
		mStaticConstraints.pushBack(desc);
		return true;
	}


	struct ArticulationStaticConstraintSortPredicate
	{
		bool operator()(const PxSolverConstraintDesc& left, const PxSolverConstraintDesc& right) const
		{
			PxU32 linkIndexA = left.linkIndexA != PxSolverConstraintDesc::NO_LINK ? left.linkIndexA : left.linkIndexB;
			PxU32 linkIndexB = right.linkIndexA != PxSolverConstraintDesc::NO_LINK ? right.linkIndexA : right.linkIndexB;

			return linkIndexA < linkIndexB;
		}
	};

	bool createFinalizeSolverContactsStep(PxTGSSolverContactDesc& contactDesc,
		PxsContactManagerOutput& output,
		ThreadContext& threadContext,
		const PxReal invDtF32,
		const PxReal invTotalDt,
		PxReal bounceThresholdF32,
		PxReal frictionOffsetThreshold,
		PxReal correlationDistance,
		PxConstraintAllocator& constraintAllocator);

	void FeatherstoneArticulation::prepareStaticConstraintsTGS(const PxReal stepDt, const PxReal totalDt, const PxReal invStepDt, const PxReal invTotalDt, 
		PxsContactManagerOutputIterator& outputs, Dy::ThreadContext& threadContext, PxReal correlationDist, PxReal bounceThreshold, PxReal frictionOffsetThreshold,
		PxTGSSolverBodyData* solverBodyData, PxTGSSolverBodyTxInertia* txInertia, PxsConstraintBlockManager& blockManager,
		Dy::ConstraintWriteback* constraintWritebackPool, const PxU32 /*nbSubsteps*/, const PxReal lengthScale)
	{
		BlockAllocator blockAllocator(blockManager, threadContext.mConstraintBlockStream, threadContext.mFrictionPatchStreamPair, threadContext.mConstraintSize);

		PxTransform id(PxIdentity);

		Ps::sort<PxSolverConstraintDesc, ArticulationStaticConstraintSortPredicate>(mStaticConstraints.begin(), mStaticConstraints.size(), ArticulationStaticConstraintSortPredicate());

		for (PxU32 i = 0; i < mStaticConstraints.size(); ++i)
		{
			PxSolverConstraintDesc& desc = mStaticConstraints[i];

			PxU16 linkIndex = desc.linkIndexA != PxSolverConstraintDesc::NO_LINK ? desc.linkIndexA : desc.linkIndexB;

			

			if (desc.constraintLengthOver16 == DY_SC_TYPE_RB_CONTACT)
			{

				PxTGSSolverContactDesc blockDesc;
				PxsContactManager* cm = reinterpret_cast<PxsContactManager*>(desc.constraint);
				PxcNpWorkUnit& unit = cm->getWorkUnit();
				PxsContactManagerOutput* cmOutput = &outputs.getContactManager(unit.mNpIndex);

				PxTGSSolverBodyVel& b0 = *desc.tgsBodyA;
				PxTGSSolverBodyVel& b1 = *desc.tgsBodyB;

				PxTGSSolverBodyData& data0 = desc.linkIndexA != PxSolverConstraintDesc::NO_LINK ? solverBodyData[0] : solverBodyData[desc.bodyADataIndex];
				PxTGSSolverBodyData& data1 = desc.linkIndexB != PxSolverConstraintDesc::NO_LINK ? solverBodyData[0] : solverBodyData[desc.bodyBDataIndex];

				PxTGSSolverBodyTxInertia& txI0 = txInertia[desc.bodyADataIndex];
				PxTGSSolverBodyTxInertia& txI1 = txInertia[desc.bodyBDataIndex];

				PxU8 flags = unit.rigidCore0->mFlags;
				flags |= PxU8(unit.rigidCore1->mFlags);

				blockDesc.bodyFrame0 = unit.rigidCore0->body2World;
				blockDesc.bodyFrame1 = unit.rigidCore1->body2World;
				blockDesc.shapeInteraction = cm->getShapeInteraction();
				blockDesc.contactForces = cmOutput->contactForces;
				blockDesc.desc = static_cast<PxSolverConstraintDesc*>(&desc);
				blockDesc.body0 = &b0;
				blockDesc.body1 = &b1;
				blockDesc.body0TxI = &txI0;
				blockDesc.body1TxI = &txI1;
				blockDesc.bodyData0 = &data0;
				blockDesc.bodyData1 = &data1;
				blockDesc.hasForceThresholds = !!(unit.flags & PxcNpWorkUnitFlag::eFORCE_THRESHOLD);
				blockDesc.disableStrongFriction = !!(unit.flags & PxcNpWorkUnitFlag::eDISABLE_STRONG_FRICTION);
				blockDesc.bodyState0 = (unit.flags & PxcNpWorkUnitFlag::eARTICULATION_BODY0) ? PxSolverContactDesc::eARTICULATION : PxSolverContactDesc::eDYNAMIC_BODY;
				blockDesc.bodyState1 = (unit.flags & PxcNpWorkUnitFlag::eARTICULATION_BODY1) ? PxSolverContactDesc::eARTICULATION : (unit.flags & PxcNpWorkUnitFlag::eHAS_KINEMATIC_ACTOR) ? PxSolverContactDesc::eKINEMATIC_BODY :
					((unit.flags & PxcNpWorkUnitFlag::eDYNAMIC_BODY1) ? PxSolverContactDesc::eDYNAMIC_BODY : PxSolverContactDesc::eSTATIC_BODY);
				//blockDesc.flags = unit.flags;

				PxReal maxImpulse0 = (unit.flags & PxcNpWorkUnitFlag::eARTICULATION_BODY0) ? static_cast<const PxsBodyCore*>(unit.rigidCore0)->maxContactImpulse : data0.maxContactImpulse;
				PxReal maxImpulse1 = (unit.flags & PxcNpWorkUnitFlag::eARTICULATION_BODY1) ? static_cast<const PxsBodyCore*>(unit.rigidCore1)->maxContactImpulse : data1.maxContactImpulse;

				PxReal dominance0 = unit.dominance0 ? 1.f : 0.f;
				PxReal dominance1 = unit.dominance1 ? 1.f : 0.f;

				blockDesc.invMassScales.linear0 = blockDesc.invMassScales.angular0 = dominance0;
				blockDesc.invMassScales.linear1 = blockDesc.invMassScales.angular1 = dominance1;
				blockDesc.restDistance = unit.restDistance;
				blockDesc.frictionPtr = unit.frictionDataPtr;
				blockDesc.frictionCount = unit.frictionPatchCount;
				blockDesc.maxCCDSeparation = PX_MAX_F32;
				blockDesc.maxImpulse = PxMin(maxImpulse0, maxImpulse1);
				blockDesc.torsionalPatchRadius = unit.mTorsionalPatchRadius;
				blockDesc.minTorsionalPatchRadius = unit.mMinTorsionalPatchRadius;

				createFinalizeSolverContactsStep(blockDesc, *cmOutput, threadContext,
					invStepDt, invTotalDt, bounceThreshold, frictionOffsetThreshold, correlationDist, blockAllocator);

				getContactManagerConstraintDesc(*cmOutput, *cm, desc);

				unit.frictionDataPtr = blockDesc.frictionPtr;
				unit.frictionPatchCount = blockDesc.frictionCount;
				//KS - Don't track this for now!
				//axisConstraintCount += blockDesc.axisConstraintCount;

				if (desc.constraint)
				{
					if(mArticulationData.mNbStaticConstraints[linkIndex] == 0)
						mArticulationData.mStaticConstraintStartIndex[linkIndex] = i;
					mArticulationData.mNbStaticConstraints[linkIndex]++;
				}
				else
				{
					//Shuffle down 
					mStaticConstraints.remove(i);
					i--;
				}

			}
			else
			{
				PX_ASSERT(desc.constraintLengthOver16 == DY_SC_TYPE_RB_1D);
				const Constraint* constraint = reinterpret_cast<const Constraint*>(desc.constraint);

				SolverConstraintShaderPrepDesc shaderPrepDesc;
				PxTGSSolverConstraintPrepDesc prepDesc;

				const PxConstraintSolverPrep solverPrep = constraint->solverPrep;
				const void* constantBlock = constraint->constantBlock;
				const PxU32 constantBlockByteSize = constraint->constantBlockSize;
				const PxTransform& pose0 = (constraint->body0 ? constraint->body0->getPose() : id);
				const PxTransform& pose1 = (constraint->body1 ? constraint->body1->getPose() : id);
				const PxTGSSolverBodyVel* sbody0 = desc.tgsBodyA;
				const PxTGSSolverBodyVel* sbody1 = desc.tgsBodyB;
				PxTGSSolverBodyData* sbodyData0 = &solverBodyData[desc.linkIndexA != PxSolverConstraintDesc::NO_LINK ? 0 : desc.bodyADataIndex];
				PxTGSSolverBodyData* sbodyData1 = &solverBodyData[desc.linkIndexB != PxSolverConstraintDesc::NO_LINK ? 0 : desc.bodyBDataIndex];
				PxTGSSolverBodyTxInertia& txI0 = txInertia[desc.bodyADataIndex];
				PxTGSSolverBodyTxInertia& txI1 = txInertia[desc.bodyBDataIndex];

				shaderPrepDesc.constantBlock = constantBlock;
				shaderPrepDesc.constantBlockByteSize = constantBlockByteSize;
				shaderPrepDesc.constraint = constraint;
				shaderPrepDesc.solverPrep = solverPrep;

				prepDesc.desc = static_cast<PxSolverConstraintDesc*>(&desc);
				prepDesc.bodyFrame0 = pose0;
				prepDesc.bodyFrame1 = pose1;
				prepDesc.body0 = sbody0;
				prepDesc.body1 = sbody1;
				prepDesc.body0TxI = &txI0;
				prepDesc.body1TxI = &txI1;
				prepDesc.bodyData0 = sbodyData0;
				prepDesc.bodyData1 = sbodyData1;
				prepDesc.linBreakForce = constraint->linBreakForce;
				prepDesc.angBreakForce = constraint->angBreakForce;
				prepDesc.writeback = &constraintWritebackPool[constraint->index];
				prepDesc.disablePreprocessing = !!(constraint->flags & PxConstraintFlag::eDISABLE_PREPROCESSING);
				prepDesc.improvedSlerp = !!(constraint->flags & PxConstraintFlag::eIMPROVED_SLERP);
				prepDesc.driveLimitsAreForces = !!(constraint->flags & PxConstraintFlag::eDRIVE_LIMITS_ARE_FORCES);
				prepDesc.extendedLimits = !!(constraint->flags & PxConstraintFlag::eENABLE_EXTENDED_LIMITS);
				prepDesc.minResponseThreshold = constraint->minResponseThreshold;

				prepDesc.bodyState0 = desc.linkIndexA == PxSolverConstraintDesc::NO_LINK ? PxSolverContactDesc::eDYNAMIC_BODY : PxSolverContactDesc::eARTICULATION;
				prepDesc.bodyState1 = desc.linkIndexB == PxSolverConstraintDesc::NO_LINK ? PxSolverContactDesc::eDYNAMIC_BODY : PxSolverContactDesc::eARTICULATION;

				SetupSolverConstraintStep(shaderPrepDesc, prepDesc, blockAllocator, stepDt, totalDt, invStepDt, invTotalDt, lengthScale);

				if (desc.constraint)
				{
					if (mArticulationData.mNbStaticConstraints[linkIndex] == 0)
						mArticulationData.mStaticConstraintStartIndex[linkIndex] = i;
					mArticulationData.mNbStaticConstraints[linkIndex]++;
				}
				else
				{
					//Shuffle down 
					mStaticConstraints.remove(i);
					i--;
				}

			}
		}
	}


	void FeatherstoneArticulation::prepareStaticConstraints(const PxReal dt, const PxReal invDt, PxsContactManagerOutputIterator& outputs,
		Dy::ThreadContext& threadContext, PxReal correlationDist, PxReal bounceThreshold, PxReal frictionOffsetThreshold, PxReal solverOffsetSlop,
		PxReal ccdMaxSeparation, PxSolverBodyData* solverBodyData, PxsConstraintBlockManager& blockManager,
		Dy::ConstraintWriteback* constraintWritebackPool)
	{
		BlockAllocator blockAllocator(blockManager, threadContext.mConstraintBlockStream, threadContext.mFrictionPatchStreamPair, threadContext.mConstraintSize);

		PxTransform id(PxIdentity);

		Cm::SpatialVectorF* Z = threadContext.mZVector.begin();

		Ps::sort<PxSolverConstraintDesc, ArticulationStaticConstraintSortPredicate>(mStaticConstraints.begin(), mStaticConstraints.size(), ArticulationStaticConstraintSortPredicate());

		for (PxU32 i = 0; i < mStaticConstraints.size(); ++i)
		{
			PxSolverConstraintDesc& desc = mStaticConstraints[i];

			PxU16 linkIndex = desc.linkIndexA != PxSolverConstraintDesc::NO_LINK ? desc.linkIndexA : desc.linkIndexB;

			

			if(desc.constraintLengthOver16 == DY_SC_TYPE_RB_CONTACT)
			{
				
				PxSolverContactDesc blockDesc;
				PxsContactManager* cm = reinterpret_cast<PxsContactManager*>(desc.constraint);
				PxcNpWorkUnit& unit = cm->getWorkUnit();
				PxsContactManagerOutput* cmOutput = &outputs.getContactManager(unit.mNpIndex);

			


				PxSolverBodyData& data0 = desc.linkIndexA != PxSolverConstraintDesc::NO_LINK ? solverBodyData[0] : solverBodyData[desc.bodyADataIndex];
				PxSolverBodyData& data1 = desc.linkIndexB != PxSolverConstraintDesc::NO_LINK ? solverBodyData[0] : solverBodyData[desc.bodyBDataIndex];

				blockDesc.data0 = &data0;
				blockDesc.data1 = &data1;

				PxU8 flags = unit.rigidCore0->mFlags;
				if (unit.rigidCore1)
					flags |= PxU8(unit.rigidCore1->mFlags);

				blockDesc.bodyFrame0 = unit.rigidCore0->body2World;
				blockDesc.bodyFrame1 = unit.rigidCore1 ? unit.rigidCore1->body2World : id;
				blockDesc.shapeInteraction = cm->getShapeInteraction();
				blockDesc.contactForces = cmOutput->contactForces;
				blockDesc.desc = &desc;
				blockDesc.body0 = desc.bodyA;
				blockDesc.body1 = desc.bodyB;
				blockDesc.hasForceThresholds = !!(unit.flags & PxcNpWorkUnitFlag::eFORCE_THRESHOLD);
				blockDesc.disableStrongFriction = !!(unit.flags & PxcNpWorkUnitFlag::eDISABLE_STRONG_FRICTION);
				blockDesc.bodyState0 = (unit.flags & PxcNpWorkUnitFlag::eARTICULATION_BODY0) ? PxSolverContactDesc::eARTICULATION : PxSolverContactDesc::eDYNAMIC_BODY;
				blockDesc.bodyState1 = (unit.flags & PxcNpWorkUnitFlag::eARTICULATION_BODY1) ? PxSolverContactDesc::eARTICULATION : (unit.flags & PxcNpWorkUnitFlag::eHAS_KINEMATIC_ACTOR) ? PxSolverContactDesc::eKINEMATIC_BODY :
					((unit.flags & PxcNpWorkUnitFlag::eDYNAMIC_BODY1) ? PxSolverContactDesc::eDYNAMIC_BODY : PxSolverContactDesc::eSTATIC_BODY);
				//blockDesc.flags = unit.flags;

				PxReal dominance0 = unit.dominance0 ? 1.f : 0.f;
				PxReal dominance1 = unit.dominance1 ? 1.f : 0.f;

				blockDesc.invMassScales.linear0 = blockDesc.invMassScales.angular0 = dominance0;
				blockDesc.invMassScales.linear1 = blockDesc.invMassScales.angular1 = dominance1;
				blockDesc.restDistance = unit.restDistance;
				blockDesc.frictionPtr = unit.frictionDataPtr;
				blockDesc.frictionCount = unit.frictionPatchCount;
				blockDesc.maxCCDSeparation = (flags & PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD) ? ccdMaxSeparation : PX_MAX_F32;

				createFinalizeSolverContacts(blockDesc, *cmOutput, threadContext, invDt, bounceThreshold, frictionOffsetThreshold,
					correlationDist, solverOffsetSlop, blockAllocator, Z);

				getContactManagerConstraintDesc(*cmOutput, *cm, desc);

				unit.frictionDataPtr = blockDesc.frictionPtr;
				unit.frictionPatchCount = blockDesc.frictionCount;
				//KS - Don't track this for now!
				//axisConstraintCount += blockDesc.axisConstraintCount;

				if (desc.constraint)
				{
					if (mArticulationData.mNbStaticConstraints[linkIndex] == 0)
						mArticulationData.mStaticConstraintStartIndex[linkIndex] = i;
					mArticulationData.mNbStaticConstraints[linkIndex]++;
				}
				else
				{
					mStaticConstraints.remove(i);
					i--;
				}

			}
			else
			{
				PX_ASSERT(desc.constraintLengthOver16 == DY_SC_TYPE_RB_1D);
				const Constraint* constraint = reinterpret_cast<const Constraint*>(desc.constraint);

				SolverConstraintShaderPrepDesc shaderPrepDesc;
				PxSolverConstraintPrepDesc prepDesc;

				const PxConstraintSolverPrep solverPrep = constraint->solverPrep;
				const void* constantBlock = constraint->constantBlock;
				const PxU32 constantBlockByteSize = constraint->constantBlockSize;
				const PxTransform& pose0 = (constraint->body0 ? constraint->body0->getPose() : id);
				const PxTransform& pose1 = (constraint->body1 ? constraint->body1->getPose() : id);
				const PxSolverBody* sbody0 = desc.bodyA;
				const PxSolverBody* sbody1 = desc.bodyB;
				PxSolverBodyData* sbodyData0 = &solverBodyData[desc.linkIndexA != PxSolverConstraintDesc::NO_LINK ? 0 : desc.bodyADataIndex];
				PxSolverBodyData* sbodyData1 = &solverBodyData[desc.linkIndexB != PxSolverConstraintDesc::NO_LINK ? 0 : desc.bodyBDataIndex];

				shaderPrepDesc.constantBlock = constantBlock;
				shaderPrepDesc.constantBlockByteSize = constantBlockByteSize;
				shaderPrepDesc.constraint = constraint;
				shaderPrepDesc.solverPrep = solverPrep;

				prepDesc.desc = &desc;
				prepDesc.bodyFrame0 = pose0;
				prepDesc.bodyFrame1 = pose1;
				prepDesc.data0 = sbodyData0;
				prepDesc.data1 = sbodyData1;
				prepDesc.body0 = sbody0;
				prepDesc.body1 = sbody1;
				prepDesc.linBreakForce = constraint->linBreakForce;
				prepDesc.angBreakForce = constraint->angBreakForce;
				prepDesc.writeback = &constraintWritebackPool[constraint->index];
				prepDesc.disablePreprocessing = !!(constraint->flags & PxConstraintFlag::eDISABLE_PREPROCESSING);
				prepDesc.improvedSlerp = !!(constraint->flags & PxConstraintFlag::eIMPROVED_SLERP);
				prepDesc.driveLimitsAreForces = !!(constraint->flags & PxConstraintFlag::eDRIVE_LIMITS_ARE_FORCES);
				prepDesc.extendedLimits = !!(constraint->flags & PxConstraintFlag::eENABLE_EXTENDED_LIMITS);
				prepDesc.minResponseThreshold = constraint->minResponseThreshold;

				SetupSolverConstraint(shaderPrepDesc, prepDesc, blockAllocator, dt, invDt, Z);

				if (desc.constraint)
				{
					if (mArticulationData.mNbStaticConstraints[linkIndex] == 0)
						mArticulationData.mStaticConstraintStartIndex[linkIndex] = i;
					mArticulationData.mNbStaticConstraints[linkIndex]++;
				}
				else
				{
					//Shuffle down 
					mStaticConstraints.remove(i);
					i--;
				}

			}
		}
	}

	void FeatherstoneArticulation::setupInternalConstraintsRecursive(
		ArticulationLink* links,
		const PxU32 linkCount,
		const bool fixBase,
		ArticulationData& data,
		Cm::SpatialVectorF* Z,
		const PxReal stepDt,
		const PxReal dt,
		const PxReal invDt,
		const PxReal erp,
		const PxReal cfm,
		const bool isTGSSolver, 
		const PxU32 linkID,
		const PxReal maxForceScale)
	{
		const ArticulationLink& link = links[linkID];

		ArticulationJointCoreData& jointDatum = data.getJointData(linkID);
		ArticulationJointTargetData& jointTranDatum = data.getJointTranData(linkID);
		const PxReal* jPosition = &data.getJointPositions()[jointDatum.jointOffset];
		PX_UNUSED(jPosition);

		const ArticulationLink& pLink = links[link.parent];

		const ArticulationJointCore& j = *link.inboundJoint;

		//const bool jointDrive = (j.driveType != PxArticulationJointDriveType::eNONE);

		bool hasFriction = j.frictionCoefficient > 0.f;

		const PxReal fCoefficient = j.frictionCoefficient * stepDt;
		

		const PxU32 limitedRows = 2u * jointDatum.limitedAxes;
		const PxU32 lockedRows = jointDatum.lockedAxes;


		PxU8 driveRows = 0;

		for (PxU32 i = 0; i < PxArticulationAxis::eCOUNT; ++i)
		{
			if (j.drives[i].maxForce > 0.f && (j.drives[i].stiffness > 0.f || j.drives[i].damping > 0.f))
				driveRows++;
		}

		const PxU8 frictionRows = hasFriction ? jointDatum.dof : PxU8(0);

		const PxU8 constraintCount = PxU8(limitedRows + driveRows + frictionRows + lockedRows);
		if (!constraintCount)
		{
			//Skip these constraints...
			//constraints += jointDatum.dof;
			jointDatum.dofInternalConstraintMask = 0;
		}
		else
		{
			const PxReal transmissionForce = data.getTransmittedForce(linkID).magnitude() * fCoefficient;

			const PxTransform cA2w = pLink.bodyCore->body2World.transform(j.parentPose);
			const PxTransform cB2w = link.bodyCore->body2World.transform(j.childPose);


			const PxQuat qB2qA = cA2w.q.getConjugate() * cB2w.q;
			

			PxU32 dofId = 0;

			PxU8 dofMask = 0;
			for (PxU32 i = 0; i < PxArticulationAxis::eX; ++i)
			{
				if (j.motion[i] != PxArticulationMotion::eLOCKED)
				{
					const bool hasDrive = (j.motion[i] != PxArticulationMotion::eLOCKED && j.drives[i].driveType != PxArticulationDriveType::eNONE);

					if (j.motion[i] == PxArticulationMotion::eLIMITED || hasDrive || frictionRows)
					{
						dofMask |= (1 << dofId);
						//Impulse response vector and axes are common for all constraints on this axis besides locked axis!!!
						const PxVec3 axis = data.mWorldMotionMatrix[linkID][dofId].top;

						Cm::SpatialVectorV deltaVA, deltaVB;
						FeatherstoneArticulation::getImpulseSelfResponse(links, fixBase, Z, data,
							links[linkID].parent, Cm::SpatialVector(PxVec3(0), axis), deltaVA,
							linkID, Cm::SpatialVector(PxVec3(0), -axis), deltaVB);

						const Cm::SpatialVector& deltaV0 = unsimdRef(deltaVA);
						const Cm::SpatialVector& deltaV1 = unsimdRef(deltaVB);

						const PxReal r0 = deltaV0.angular.dot(axis);
						const PxReal r1 = deltaV1.angular.dot(axis);

						const PxReal unitResponse = r0 - r1;

						const PxReal recipResponse = unitResponse > DY_ARTICULATION_MIN_RESPONSE ? 1.0f / (unitResponse+cfm) : 0.0f;

						const PxU32 count = data.mInternalConstraints.size();
						data.mInternalConstraints.forceSize_Unsafe(count + 1);
						ArticulationInternalConstraint* constraints = &data.mInternalConstraints[count];

						//constraints->unitResponse = unitResponse;
						constraints->recipResponse = recipResponse;
						constraints->response = unitResponse;
						constraints->row0 = Cm::SpatialVectorF(PxVec3(0), axis);
						constraints->row1 = Cm::SpatialVectorF(PxVec3(0), axis);
						constraints->deltaVA.top = unsimdRef(deltaVA).angular;
						constraints->deltaVA.bottom = unsimdRef(deltaVA).linear;
						constraints->deltaVB.top = unsimdRef(deltaVB).angular;
						constraints->deltaVB.bottom = unsimdRef(deltaVB).linear;
						constraints->erp = erp;
						constraints->isLinearConstraint = false;

						if (j.motion[i] == PxArticulationMotion::eLIMITED)
						{
							constraints->lowLimit = j.limits[i].low;
							constraints->highLimit = j.limits[i].high;
						}
						else
						{
							constraints->lowLimit = -PX_MAX_F32;
							constraints->highLimit = PX_MAX_F32;
						}

						constraints->lowImpulse = 0.f;
						constraints->highImpulse = 0.f;

						constraints->frictionForce = 0.f;
						constraints->maxFrictionForce = hasFriction ? transmissionForce : 0.f;
						constraints->frictionForceCoefficient = isTGSSolver ? 0.f : 1.f;


						if (hasDrive)
						{
							const PxReal targetVelocity = jointTranDatum.targetJointVelocity[dofId];
							PxReal targetPos = jointTranDatum.targetJointPosition[dofId];

							//KS - clamp drive target within limits - no point in having 2 parts fight against each-other
							if (j.motion[i] == PxArticulationMotion::eLIMITED)
								targetPos = PxClamp(targetPos, j.limits[i].low, j.limits[i].high);

							
							PxReal x = 0.f;

							PxReal stiffness = j.drives[i].stiffness;
							PxReal damping = j.drives[i].damping;

							PxArticulationDriveType::Enum type = j.drives[i].driveType;

							if (type == PxArticulationDriveType::eTARGET)
							{
								stiffness = 1e+25f;
								damping = 0.f;
								type = PxArticulationDriveType::eFORCE;
							}
							else if (type == PxArticulationDriveType::eVELOCITY)
							{
								damping = 1e+25f;
								stiffness = 0.f;
								type = PxArticulationDriveType::eFORCE;
							}

							const PxReal a = stepDt * (stepDt*stiffness + damping);
							const PxReal b = stepDt * (damping * targetVelocity);// + stiffness * (targetPos - jointPos));

							switch (type)
							{
							case PxArticulationDriveType::eFORCE:
							{
								
								x = unitResponse > 0.f ? 1.0f / (1.0f + a*unitResponse) : 0.f;
								constraints->driveTargetVel = x * b;
								constraints->driveVelMultiplier = -x*a;
								constraints->driveBiasCoefficient = stiffness * x * stepDt;
								break;
							}
							
							case PxArticulationDriveType::eACCELERATION:
							{
								x = 1.0f / (1.0f + a);
								constraints->driveTargetVel = x * b*recipResponse;
								constraints->driveVelMultiplier = -x*a*recipResponse;
								constraints->driveBiasCoefficient = stiffness * x * recipResponse * stepDt;
								break;
							}
							//KS - this is required to avoid a warning on Linux
							case PxArticulationDriveType::eTARGET:
							case PxArticulationDriveType::eVELOCITY:
							case PxArticulationDriveType::eNONE:
								break;
					
							}

							//constraints->driveBiasCoefficient = 0.f;//recipResponse * invDt;
							const PxReal im = 1.0f - x;
							constraints->driveTarget = targetPos;
							//constraints->driveTarget = targetPos;
							constraints->driveImpulseMultiplier = isTGSSolver ? 1.f : im;
							constraints->maxDriveForce = j.drives[i].maxForce* maxForceScale;// *dt;
							constraints->driveForce = 0.f;
							constraints->driveBackPropagateScale = 0.f; //*vm2;
						}
						else
						{
							constraints->driveTargetVel = 0.f;
							constraints->driveTarget = 0.f;
							constraints->driveBiasCoefficient = 0.f;
							constraints->driveVelMultiplier = 0.f;
							constraints->driveImpulseMultiplier = 0.f;
							constraints->maxDriveForce = 0.f;
							constraints->driveForce = 0.f;
							constraints->driveBackPropagateScale = 0.f;
						}
					}
					dofId++;
				}
			}

			for (PxU32 i = PxArticulationAxis::eX; i < PxArticulationAxis::eCOUNT; ++i)
			{
				if (j.motion[i] != PxArticulationMotion::eLOCKED)
				{
					const bool hasDrive = (j.motion[i] != PxArticulationMotion::eLOCKED && j.drives[i].maxForce > 0.f && (j.drives[i].stiffness > 0.f || j.drives[i].damping > 0.f));

					if (j.motion[i] == PxArticulationMotion::eLIMITED || hasDrive || frictionRows)
					{

						dofMask |= (1 << dofId);
						//Impulse response vector and axes are common for all constraints on this axis besides locked axis!!!
						const PxVec3 axis = data.mWorldMotionMatrix[linkID][dofId].bottom;
						const PxVec3 ang0 = (cA2w.p - pLink.bodyCore->body2World.p).cross(axis);
						const PxVec3 ang1 = (cB2w.p - link.bodyCore->body2World.p).cross(axis);

						Cm::SpatialVectorV deltaVA, deltaVB;
						FeatherstoneArticulation::getImpulseSelfResponse(links, fixBase, Z, data,
							links[linkID].parent, Cm::SpatialVector(axis, ang0), deltaVA,
							linkID, Cm::SpatialVector(-axis, -ang1), deltaVB);

						//Now add in friction rows, then limit rows, then drives...

						const Cm::SpatialVector& deltaV0 = unsimdRef(deltaVA);
						const Cm::SpatialVector& deltaV1 = unsimdRef(deltaVB);

						const PxReal r0 = deltaV0.linear.dot(axis) + deltaV0.angular.dot(ang0);
						const PxReal r1 = deltaV1.linear.dot(axis) +
							deltaV1.angular.dot(ang1);

						const PxReal unitResponse = r0 - r1;

						const PxReal recipResponse = unitResponse > DY_ARTICULATION_MIN_RESPONSE ? 1.0f / (unitResponse+cfm) : 0.0f;

						const PxU32 count = data.mInternalConstraints.size();
						data.mInternalConstraints.forceSize_Unsafe(count + 1);
						ArticulationInternalConstraint* constraints = &data.mInternalConstraints[count];

						constraints->response = unitResponse;
						constraints->recipResponse = recipResponse;
						constraints->row0 = Cm::SpatialVectorF(axis, ang0);
						constraints->row1 = Cm::SpatialVectorF(axis, ang1);
						constraints->deltaVA.top = unsimdRef(deltaVA).angular;
						constraints->deltaVA.bottom = unsimdRef(deltaVA).linear;
						constraints->deltaVB.top = unsimdRef(deltaVB).angular;
						constraints->deltaVB.bottom = unsimdRef(deltaVB).linear;
						constraints->erp = erp;
						constraints->isLinearConstraint = true;

						constraints->lowImpulse = 0.f;
						constraints->highImpulse = 0.f;

						constraints->frictionForce = 0.f;
						constraints->maxFrictionForce = hasFriction ? transmissionForce : 0.f;

						constraints->frictionForceCoefficient = isTGSSolver ? 0.f : 1.f;

						if (j.motion[i] == PxArticulationMotion::eLIMITED)
						{
							constraints->lowLimit = j.limits[i].low;
							constraints->highLimit = j.limits[i].high;
						}
						else
						{
							constraints->lowLimit = -PX_MAX_F32;
							constraints->highLimit = PX_MAX_F32;
						}

						if (hasDrive)
						{
							const PxReal targetVelocity = -jointTranDatum.targetJointVelocity[dofId];
							PxReal targetPos = jointTranDatum.targetJointPosition[dofId];

							//KS - clamp drive target within limits - no point in having 2 parts fight against each-other
							if (j.motion[i] == PxArticulationMotion::eLIMITED)
								targetPos = PxClamp(targetPos, j.limits[i].low, j.limits[i].high);

							PxReal x = 0.f;

							PxReal stiffness = j.drives[i].stiffness;
							PxReal damping = j.drives[i].damping;

							PxArticulationDriveType::Enum type = j.drives[i].driveType;

							if (type == PxArticulationDriveType::eTARGET)
							{
								stiffness = 1e+25f;
								damping = 0.f;
								type = PxArticulationDriveType::eFORCE;
							}
							else if (type == PxArticulationDriveType::eVELOCITY)
							{
								damping = 1e+25f;
								stiffness = 0.f;
								type = PxArticulationDriveType::eFORCE;
							}

							const PxReal a = stepDt * (stepDt * stiffness + damping);
							const PxReal b = stepDt * (damping * targetVelocity);// + stiffness * (targetPos - jointPos));

							switch (type)
							{
							case PxArticulationDriveType::eFORCE:
							{
								x = unitResponse > 0.f ? 1.0f / (1.0f + a*unitResponse) : 0.f;
								constraints->driveTargetVel = x * b;
								constraints->driveVelMultiplier = -x*a;
								constraints->driveBiasCoefficient = stiffness * x * stepDt;
								break;
							}
							case PxArticulationDriveType::eACCELERATION:
							{
								x = 1.0f / (1.0f + a);
								constraints->driveTargetVel = x * b*recipResponse;
								constraints->driveVelMultiplier = -x*a*recipResponse;
								constraints->driveBiasCoefficient = stiffness * x * recipResponse * stepDt;
								break;
							}
							//KS - this is required to avoid a warning on Linux
							case PxArticulationDriveType::eTARGET:
							case PxArticulationDriveType::eVELOCITY:
							case PxArticulationDriveType::eNONE:
								break;
						
							}

							const PxReal im = 1.f - x;

							constraints->driveTarget = targetPos;
							constraints->driveImpulseMultiplier = isTGSSolver ? 1.f : im;
							constraints->maxDriveForce = j.drives[i].maxForce* maxForceScale;
							constraints->driveForce = 0.f;
							constraints->driveBackPropagateScale = 0.f;
						}
						else
						{
							constraints->driveTargetVel = 0.f;
							constraints->driveTarget = 0.f;
							constraints->driveBiasCoefficient = 0.f;
							constraints->driveVelMultiplier = 0.f;
							constraints->driveImpulseMultiplier = 0.f;
							constraints->maxDriveForce = 0.f;
							constraints->driveForce = 0.f;
							constraints->driveBackPropagateScale = 0.f;
						}
					}
					dofId++;
				}
			}

			if (jointDatum.lockedAxes)
			{
				PxVec3 row[3];
				computeArticJacobianAxes(row, cA2w.q, cB2w.q);

				PxReal err[] = { -qB2qA.x, -qB2qA.y, -qB2qA.z };

				for (PxU32 i = PxArticulationAxis::eTWIST; i <= PxArticulationAxis::eSWING2; ++i)
				{
					//Get axis, then add lock constraint...
					if (j.motion[i] == PxArticulationMotion::eLOCKED)
					{
						//const PxVec3 axis = row[i].getNormalized();
						//PxReal error = 2.f*PxAsin(err[i]);
						//PxReal error = 4.f*PxAtan2(err[i], 1.f+qB2qA.w);

						const PxVec3 axis = row[i];
						PxReal error = err[i];

						Cm::SpatialVectorV deltaVA, deltaVB;
						FeatherstoneArticulation::getImpulseSelfResponse(links, fixBase, Z, data,
							links[linkID].parent, Cm::SpatialVector(PxVec3(0), axis), deltaVA,
							linkID, Cm::SpatialVector(PxVec3(0), -axis), deltaVB);

						const Cm::SpatialVector& deltaV0 = unsimdRef(deltaVA);
						const Cm::SpatialVector& deltaV1 = unsimdRef(deltaVB);

						const PxReal r0 = deltaV0.angular.dot(axis);
						const PxReal r1 = deltaV1.angular.dot(axis);

						const PxReal unitResponse = r0 - r1;

						const PxReal recipResponse = unitResponse > DY_ARTICULATION_MIN_RESPONSE ? 1.0f / (cfm+unitResponse) : 0.0f;

						const PxU32 count = data.mInternalLockedAxes.size();
						data.mInternalLockedAxes.forceSize_Unsafe(count + 1);
						ArticulationInternalLockedAxis* locks = &data.mInternalLockedAxes[count];

						locks->axis = axis;
						locks->deltaVA.top = unsimdRef(deltaVA).angular;
						locks->deltaVA.bottom = unsimdRef(deltaVA).linear;
						locks->deltaVB.top = unsimdRef(deltaVB).angular;
						locks->deltaVB.bottom = unsimdRef(deltaVB).linear;
						locks->recipResponse = recipResponse;
						locks->error = error;
						locks->biasScale = invDt*erp;//*0.7f;
						dofId++;
					}
				}
			}
			jointDatum.dofInternalConstraintMask = dofMask;
		}		

		for (ArticulationBitField children = link.children; children != 0; children &= (children - 1))
		{
			//index of child of link h on path to link linkID
			const PxU32 child = ArticulationLowestSetBit(children);

			setupInternalConstraintsRecursive(links, linkCount, fixBase, data, Z, stepDt, dt, invDt, erp, cfm, isTGSSolver, child, maxForceScale);

		}
	}


	void FeatherstoneArticulation::setupInternalConstraints(
		ArticulationLink* links,
		const PxU32 linkCount,
		const bool fixBase,
		ArticulationData& data,
		Cm::SpatialVectorF* Z,
		PxReal stepDt,
		PxReal dt,
		PxReal invDt,
		PxReal erp,
		bool isTGSSolver)
	{
		PX_UNUSED(linkCount);

		const PxReal cfm = DY_ARTICULATION_CFM;

		data.mInternalConstraints.forceSize_Unsafe(0);
		data.mInternalConstraints.reserve(data.getDofs());

		data.mInternalLockedAxes.forceSize_Unsafe(0);
		data.mInternalLockedAxes.reserve(data.getLocks());

		const PxReal maxForceScale = data.getArticulationFlags() & PxArticulationFlag::eDRIVE_LIMITS_ARE_FORCES ? dt : 1.f;

		for (ArticulationBitField children = links[0].children; children != 0; children &= (children - 1))
		{
			//index of child of link h on path to link linkID
			const PxU32 child = ArticulationLowestSetBit(children);

			setupInternalConstraintsRecursive(links, linkCount, fixBase, data, Z, stepDt, dt, invDt, erp, cfm, isTGSSolver, child, maxForceScale);

		}
	}


	PxU32 FeatherstoneArticulation::setupSolverConstraints(
		ArticulationLink* links,
		const PxU32 linkCount,
		const bool fixBase,
		ArticulationData& data,
		Cm::SpatialVectorF* Z,
		PxU32& acCount)
	{
		acCount = 0;

		setupInternalConstraints(links, linkCount, fixBase, data, Z, data.getDt(), data.getDt(), 1.f / data.getDt(), 1.f, false);

		return 0;
	}


	PxU32 FeatherstoneArticulation::setupSolverConstraintsTGS(const ArticulationSolverDesc& articDesc,
	PxcConstraintBlockStream& /*stream*/,
	PxSolverConstraintDesc* /*constraintDesc*/,
	PxReal dt,
	PxReal invDt,
	PxReal totalDt,
	PxU32& acCount,
	PxsConstraintBlockManager& /*constraintBlockManager*/,
	Cm::SpatialVectorF* Z)
	{
		PX_UNUSED(dt);
		PX_UNUSED(totalDt);
		acCount = 0;

		FeatherstoneArticulation* thisArtic = static_cast<FeatherstoneArticulation*>(articDesc.articulation);

		ArticulationLink* links = thisArtic->mArticulationData.getLinks();
		const PxU32 linkCount = thisArtic->mArticulationData.getLinkCount();
		const bool fixBase = thisArtic->mArticulationData.getArticulationFlags() & PxArticulationFlag::eFIX_BASE;

		thisArtic->setupInternalConstraints(links, linkCount, fixBase, thisArtic->mArticulationData, Z, dt, totalDt, invDt, 0.7f, true);

		return 0;
	}

	void FeatherstoneArticulation::createHardLimitTGS(
		SolverConstraint1DExtStep& s,
		const PxVec3& axis,
		PxReal err,
		PxReal recipDt,
		const Cm::SpatialVectorV& deltaVA,
		const Cm::SpatialVectorV& deltaVB,
		PxReal recipResponse)
	{
		PxReal error = err;
		init(s, PxVec3(0), PxVec3(0), axis, axis, 0, PX_MAX_F32);

		s.deltaVA = deltaVA;
		s.deltaVB = deltaVB;
		
		s.error = error;
		s.biasScale = -recipDt*0.7f;
		s.maxBias = 30.f;
		s.velMultiplier = -1.f;
		s.recipResponse = recipResponse;
		s.impulseMultiplier = 1.0f;
		s.velTarget = 0.f;
		s.angularErrorScale = 1.f;
		s.flags |= DY_SC_FLAG_INEQUALITY;
		if (error > 0.f)
			s.flags |= DY_SC_FLAG_KEEP_BIAS;
	}

	void FeatherstoneArticulation::createTangentialSpringTGS(
		ArticulationLink* links,
		const bool fixBase,
		Cm::SpatialVectorF* Z,
		ArticulationData& data,
		PxU32 linkIndex,
		SolverConstraint1DExtStep& s,
		const PxVec3& axis,
		PxReal stiffness,
		PxReal damping,
		PxReal dt)
	{
		init(s, PxVec3(0), PxVec3(0), axis, axis, -PX_MAX_F32, PX_MAX_F32);

		Cm::SpatialVector axis6(PxVec3(0), axis);
		const PxU32 parent = links[linkIndex].parent;
		FeatherstoneArticulation::getImpulseSelfResponse(links, fixBase, Z, data, parent, axis6, s.deltaVA, linkIndex, -axis6, s.deltaVB);

		const PxReal unitResponse = axis.dot(reinterpret_cast<PxVec3&>(s.deltaVA.angular)) - axis.dot(reinterpret_cast<PxVec3&>(s.deltaVB.angular));
		if (unitResponse<0.0f)
			Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, "Warning: articulation ill-conditioned or under severe stress, tangential spring ignored");
		const PxReal recipResponse = unitResponse> DY_ARTICULATION_MIN_RESPONSE ? 1.0f / unitResponse : 0.0f;

		// this is a specialization of the spring code in setSolverConstants() for acceleration springs.
		// general case is  b = dt * (c.mods.spring.damping * c.velocityTarget - c.mods.spring.stiffness * geomError);
		// but geomError and velocityTarget are both zero

		const PxReal a = dt * dt * stiffness + dt * damping;
		const PxReal x = 1.0f / (1.0f + a);
		s.error = 0.f;
		s.biasScale = 0.f;
		s.maxBias = 0.f;
		s.velMultiplier = -x * a;
		s.impulseMultiplier = 1.0f - x;
		s.velTarget = 0.f;
		s.recipResponse = recipResponse;
		s.angularErrorScale = 1.f;
	}

	void FeatherstoneArticulation::teleportLinks(ArticulationData& data)
	{
		jcalc(data, true);

		ArticulationLink* links = mArticulationData.getLinks();
	
		ArticulationJointCoreData* jointData = mArticulationData.getJointData();

		const PxReal* jointPositions = data.getJointPositions();

		const PxU32 linkCount = mArticulationData.getLinkCount();

		for (PxU32 linkID = 1; linkID < linkCount; ++linkID)
		{
			const ArticulationLink& link = links[linkID];
			
			const ArticulationJointCoreData& jointDatum = jointData[linkID];

			const ArticulationLink& pLink = links[link.parent];
			const PxTransform pBody2World = pLink.bodyCore->body2World;

			const ArticulationJointCore* joint = link.inboundJoint;

			const PxReal* jPosition = &jointPositions[jointDatum.jointOffset];

			PxQuat newParentToChild;
			PxQuat newWorldQ;
			PxVec3 r;

			const PxVec3 childOffset = -joint->childPose.p;
			const PxVec3 parentOffset = joint->parentPose.p;

			const PxQuat relativeQuat = mArticulationData.mRelativeQuat[linkID];

			switch (joint->jointType)
			{
			case PxArticulationJointType::ePRISMATIC:
			{
				newParentToChild = relativeQuat;
				const PxVec3 e = newParentToChild.rotate(parentOffset);
				const PxVec3 d = childOffset;

				const PxVec3& u = data.mMotionMatrix[linkID][0].bottom;

				r = e + d + u * jPosition[0];
				break;
			}
			case PxArticulationJointType::eREVOLUTE:
			{
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
				PxQuat jointRotation(PxIdentity);

				if (jointDatum.dof < 3)
				{
					for (PxU32 d = jointDatum.dof; d > 0; --d)
					{
						PxQuat deltaRot(-jPosition[d - 1], data.mMotionMatrix[linkID][d - 1].top);
						jointRotation = jointRotation * deltaRot;
					}
				}
				else
				{
					PxVec3 ang(0.f);
					for (PxU32 d = 0; d < jointDatum.dof; ++d)
					{
						ang += data.mMotionMatrix[linkID][d].top * -jPosition[d];
					}
					jointRotation = Ps::exp(ang).getNormalized();

#if PX_DEBUG
					PxReal radians;
					PxVec3 axis;
					jointRotation.toRadiansAndUnitAxis(radians, axis);

					
					for (PxU32 d = 0; d < jointDatum.dof; ++d)
					{
						PxReal ang2 = data.mMotionMatrix[linkID][d].top.dot(axis)*radians;
						PxReal diff2 = ang2 + jPosition[d];
						PX_ASSERT(PxAbs(diff2) < 1e-3f);
					}
#endif
				}


				newParentToChild = (jointRotation * relativeQuat).getNormalized();

				const PxVec3 e = newParentToChild.rotate(parentOffset);
				const PxVec3 d = childOffset;
				r = e + d;
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

			PxTransform& body2World = link.bodyCore->body2World;
			body2World.q = (pBody2World.q * newParentToChild.getConjugate()).getNormalized();
			body2World.p = pBody2World.p + body2World.q.rotate(r);

			PX_ASSERT(body2World.isSane());
		}
	}

	void FeatherstoneArticulation::computeLinkVelocities(ArticulationData& data)
	{
		jcalc(data, true);

		ArticulationLink* links = data.getLinks();
		const PxU32 linkCount = data.getLinkCount();

		const PxReal* jointVelocities = data.mJointVelocity.begin();

		for (PxU32 linkID = 1; linkID < linkCount; ++linkID)
		{
			const ArticulationLink& link = links[linkID];
			
			PxsBodyCore& bodyCore = *link.bodyCore;

			PxTransform body2World = bodyCore.body2World;

			ArticulationLink& plink = links[link.parent];
			const PxsBodyCore& pbodyCore = *plink.bodyCore;

			Cm::SpatialVectorF parentVel(pbodyCore.angularVelocity, pbodyCore.linearVelocity);

			PxTransform pBody2World = pbodyCore.body2World;

			const PxVec3 rw = body2World.p - pBody2World.p;

			Cm::SpatialVectorF vel = FeatherstoneArticulation::translateSpatialVector(-rw,parentVel);

			if (jointVelocities)
			{
				ArticulationJointCoreData& jointDatum = data.getJointData(linkID);
				const PxReal* jVelocity = &jointVelocities[jointDatum.jointOffset];

				Cm::UnAlignedSpatialVector deltaV = Cm::UnAlignedSpatialVector::Zero();
				for (PxU32 ind = 0; ind < jointDatum.dof; ++ind)
				{
					deltaV += data.mMotionMatrix[linkID][ind] * jVelocity[ind];
				}

				vel.top += body2World.q.rotate(deltaV.top);
				vel.bottom += body2World.q.rotate(deltaV.bottom);
			}

			bodyCore.linearVelocity = vel.bottom;
			bodyCore.angularVelocity = vel.top;

		}
	}

	void FeatherstoneArticulation::jcalc(ArticulationData& data, bool forceUpdate)
	{	
		if (getDirty() || forceUpdate)
		{
			ArticulationLink* links = data.getLinks();
			ArticulationJointCoreData* jointData = data.getJointData();
			ArticulationJointTargetData* jointTranData = data.getJointTranData();
			const PxU32 linkCount = data.getLinkCount();

			PxU8 totalDof = 0;
			bool hasSphericalJoint = false;

			for (PxU32 linkID = 1; linkID < linkCount; ++linkID)
			{
				ArticulationLink& link = links[linkID];
				ArticulationJointCore* joint = link.inboundJoint;
				ArticulationJointCoreData& jointDatum = jointData[linkID];
				ArticulationJointTargetData& jointTranDatum = jointTranData[linkID];
				
				PX_CHECK_AND_RETURN(joint->jointType != PxArticulationJointType::eUNDEFINED, "FeatherstoneArticulation::jcalc application need to define valid joint type and motion");
				jointDatum.computeJointDof(joint, forceUpdate);
				joint->setJointPose(jointDatum, data.mMotionMatrix[linkID], forceUpdate, mArticulationData.mRelativeQuat[linkID]);
				jointTranDatum.setJointVelocityDrive(joint);
				jointTranDatum.setJointPoseDrive(joint);

				if (joint->jointType == PxArticulationJointType::eSPHERICAL)
					hasSphericalJoint = true;

				jointDatum.jointOffset = totalDof;
				joint->jointOffset = totalDof;
				totalDof += jointDatum.dof;
			}

			if (totalDof != mArticulationData.getDofs())
			{
				mArticulationData.resizeJointData(totalDof);
				mArticulationData.setDofs(totalDof);
			}

			mHasSphericalJoint = hasSphericalJoint;

			setDirty(false);
		}
	}

	//compute link's spatial inertia tensor
	void  FeatherstoneArticulation::computeSpatialInertia(ArticulationData& data)
	{
		for (PxU32 linkID = 0; linkID < data.getLinkCount(); ++linkID)
		{
			const ArticulationLink& link = data.getLink(linkID);
			//ArticulationLinkData& linkDatum = data.getLinkData(linkID);

			const PxsBodyCore& core = *link.bodyCore;

			const PxVec3& ii = core.inverseInertia;

			const PxReal m = core.inverseMass == 0.f ? 0.f : 1.0f / core.inverseMass;

			SpatialMatrix& worldArticulatedInertia = data.mWorldSpatialArticulatedInertia[linkID];

			//construct inertia matrix
			const PxVec3 inertiaTensor = PxVec3(ii.x == 0.f ? 0.f : (1.f / ii.x), ii.y == 0.f ? 0.f : (1.f / ii.y), ii.z == 0.f ? 0.f : (1.f / ii.z));

			PxMat33 rot(data.getLink(linkID).bodyCore->body2World.q);

			worldArticulatedInertia.topLeft = PxMat33(PxZero);
			worldArticulatedInertia.topRight = PxMat33::createDiagonal(PxVec3(m));
			Cm::transformInertiaTensor(inertiaTensor, rot, worldArticulatedInertia.bottomLeft);
		}
	}

	void FeatherstoneArticulation::computeZ(ArticulationData& data, 
		const PxVec3& gravity, ScratchData& scratchData)
	{
		const Cm::SpatialVectorF* motionVelocities = scratchData.motionVelocities;
		Cm::SpatialVectorF* spatialZAForces = scratchData.spatialZAVectors;
		Cm::SpatialVector* externalAccels = scratchData.externalAccels;

		const PxReal invDt = data.mDt < 1e-6f ? PX_MAX_F32 : 1.f/data.mDt;

		for (PxU32 linkID = 0; linkID < data.getLinkCount(); ++linkID)
		{
			ArticulationLink& link = data.getLink(linkID);

			const PxsBodyCore& core = *link.bodyCore;
			//const PxTransform& body2World = core.body2World;

			PxMat33& I = data.mWorldSpatialArticulatedInertia[linkID].bottomLeft;

			//construct spatial zero acceleration
			Cm::SpatialVectorF& z = spatialZAForces[linkID];

			//Cm::SpatialVectorF v;
			//v.top = motionVelocities[linkID].top;
			//v.bottom = motionVelocities[linkID].bottom;

			//KS - limit the magnitude of the angular velocity that contributes to the geometric term. This is a
			//v^2 term and can become unstable if it is too large!
			PxVec3 v = motionVelocities[linkID].top;
			PxReal mag = v.normalize();
			mag = PxMin(mag, invDt);
			v *= mag;

			PxVec3 gravLinAccel(0.f);
			if(!core.disableGravity)
				gravLinAccel = -gravity;

			PX_ASSERT(core.inverseMass != 0.f);

			const PxReal m = 1.0f / core.inverseMass;

			z.top = (gravLinAccel * m);
			z.bottom = v.cross(I * v);

			PX_ASSERT(z.top.isFinite());
			PX_ASSERT(z.bottom.isFinite());

			if (externalAccels)
			{
				const Cm::SpatialVector& externalAccel = externalAccels[linkID];

				const PxVec3 exLinAccel = -externalAccel.linear;
				const PxVec3 exAngAccel = -externalAccel.angular;

				z.top += (exLinAccel * m);
				z.bottom += I * exAngAccel;
			}

		}
	}

	//This can be called by forward dynamic
	void FeatherstoneArticulation::computeD(ArticulationData& data, ScratchData& scratchData,
		Cm::SpatialVectorF* /*Z*/, Cm::SpatialVectorF* DeltaV)
	{
		PX_UNUSED(DeltaV);
		Cm::SpatialVectorF* motionVelocities = scratchData.motionVelocities;
		
		//Cm::SpatialVectorF* dragImpulse = Z;

		Cm::SpatialVectorF* ZAForces = scratchData.spatialZAVectors;

		//PxMemZero(dragImpulse, sizeof(Cm::SpatialVectorF)*data.getLinkCount());

		const PxReal dt = data.getDt();
		const PxReal invDt = 1.f/dt;


		for (PxU32 linkID = 0; linkID < data.getLinkCount(); ++linkID)
		{
			const ArticulationLink& link = data.getLink(linkID);

			const PxsBodyCore& core = *link.bodyCore;
			//const PxTransform& body2World = core.body2World;

			const PxReal m = core.inverseMass == 0.f ? 0.f : 1.0f / core.inverseMass;

			//PxMat33& I = linkDatum.spatialArticulatedInertia.bottomLeft;
			const PxVec3 I = PxVec3(1.f / core.inverseInertia.x, 1.f / core.inverseInertia.y, 1.f / core.inverseInertia.z);

			Cm::SpatialVectorF d(PxVec3(0.f), PxVec3(0.f));

			Cm::SpatialVectorF v;
			v.top = motionVelocities[linkID].top;
			v.bottom = motionVelocities[linkID].bottom;  

			if (core.linearDamping > 0.f || core.angularDamping > 0.f)
			{
				const PxReal linDamp = PxMin(core.linearDamping*dt, 1.f);
				const PxReal angDamp = PxMin(core.angularDamping*dt, 1.f);

				
				ZAForces[linkID].top += (v.bottom * linDamp*m) -ZAForces[linkID].top * linDamp;
				ZAForces[linkID].bottom += core.body2World.rotate(I.multiply(core.body2World.rotateInv(v.top)* angDamp)) -ZAForces[linkID].bottom * angDamp;
			}

			const PxReal maxAng = core.maxAngularVelocitySq;
			const PxReal maxLin = core.maxLinearVelocitySq;

			const PxReal angMag = v.top.magnitudeSquared();
			const PxReal linMag = v.bottom.magnitudeSquared();

			if (angMag > maxAng || linMag > maxLin)
			{
				if (angMag > maxAng)
				{
					const PxReal scale = 1.f - PxSqrt(maxAng) / PxSqrt(angMag);
					const PxVec3 tmpaccelerationAng = core.body2World.rotate((I.multiply(core.body2World.rotateInv(v.top))*scale));
					PX_UNUSED(tmpaccelerationAng);
					d.bottom += tmpaccelerationAng;
				}

				if (linMag > maxLin)
				{
					const PxReal scale = 1.f - (PxSqrt(maxLin) / PxSqrt(linMag));
					const PxVec3 tmpaccelerationLin = (v.bottom*m*scale);
					PX_UNUSED(tmpaccelerationLin);
					d.top += tmpaccelerationLin;
				}
			}

			ZAForces[linkID] += d * invDt;
		}

		//if (bAppliedImpusle)
		//{
		//	//const PxReal invDt = 1.f/data.getDt();
		//	applyImpulses(dragImpulse, DeltaV);

		//	PxReal* deltaV = data.getJointDeltaVelocities();
		//	PxReal* jointV = data.getJointVelocities();

		//	for (PxU32 linkID = 1; linkID < data.getLinkCount(); ++linkID)
		//	{
		//		ArticulationJointCoreData& tJointDatum = data.getJointData()[linkID];
		//		for (PxU32 i = 0; i < tJointDatum.dof; ++i)
		//		{
		//			const PxReal dv = deltaV[i + tJointDatum.jointOffset];
		//			jointV[i + tJointDatum.jointOffset] += dv;
		//			deltaV[i + tJointDatum.jointOffset] = 0.f;
		//		}
		//	}
		//}

		////Last ditch effort - clamp velocities..
		//for (PxU32 linkID = 0; linkID < data.getLinkCount(); ++linkID)
		//{
		//	const ArticulationLink& link = data.getLink(linkID);

		//	const PxsBodyCore& core = *link.bodyCore;
		//	Cm::SpatialVectorF v;
		//	v.top = motionVelocities[linkID].top;
		//	v.bottom = motionVelocities[linkID].bottom;

		//	const PxReal maxAng = core.maxAngularVelocitySq;
		//	const PxReal maxLin = core.maxLinearVelocitySq;

		//	const PxReal angMag = v.top.magnitudeSquared();
		//	const PxReal linMag = v.bottom.magnitudeSquared();

		//	if (angMag > maxAng)
		//	{
		//		PxReal scale = PxSqrt(maxAng) / PxSqrt(angMag);
		//		motionVelocities[linkID].top = v.top*scale;

		//	}
		//	if (linMag > maxLin)
		//	{
		//		PxReal scale = PxSqrt(maxLin) / PxSqrt(linMag);
		//		motionVelocities[linkID].bottom = v.bottom*scale;
		//	}
		//}
	}

	//compute coriolis and centrifugal term
	void FeatherstoneArticulation::computeC(ArticulationData& data, ScratchData& scratchData)
	{
		const PxReal* jointVelocities = scratchData.jointVelocities;
		Cm::SpatialVectorF* coriolisVectors = scratchData.coriolisVectors;

		const PxU32 linkCount = data.getLinkCount();

		coriolisVectors[0] = Cm::SpatialVectorF::Zero();

		for (PxU32 linkID = 1; linkID < linkCount; ++linkID)
		{
			const ArticulationLink& link = data.getLink(linkID);
			const ArticulationLinkData& linkDatum = data.getLinkData(linkID);
			const ArticulationJointCoreData& jointDatum = data.getJointData(linkID);

			const PxReal* jVelocity = &jointVelocities[jointDatum.jointOffset];
			Cm::SpatialVectorF& coriolis = coriolisVectors[linkID];

			
			//const PxTransform& body2World = link.bodyCore->body2World;
			//transform parent link's angular velocity into current link's body space
			const PxVec3 pAngular = scratchData.motionVelocities[link.parent].top;
			
			PxVec3 torque = pAngular.cross(pAngular.cross(linkDatum.rw));

			//PX_ASSERT(parentAngular.magnitude() < 100.f);

			PxVec3 force(0.f);
			if (jointDatum.dof > 0)
			{
				Cm::SpatialVectorF relVel(PxVec3(0.f), PxVec3(0.f));
				for (PxU32 ind = 0; ind < jointDatum.dof; ++ind)
				{
					//Clamp joint velocity used in coriolis terms to reduce chances of unstable feed-back loops
					const PxReal jV = jVelocity[ind];
					relVel.top += data.mWorldMotionMatrix[linkID][ind].top * jV;
					relVel.bottom += data.mWorldMotionMatrix[linkID][ind].bottom * jV;
				}
				const PxVec3 aVec = relVel.top;
				force = pAngular.cross(aVec);

				//compute linear part
				const PxVec3 lVel = relVel.bottom;

				const PxVec3 temp1 = 2.f * pAngular.cross(lVel);
				const PxVec3 temp2 = aVec.cross(lVel);
				torque += temp1 + temp2;
			}

			PX_ASSERT(force.isFinite());
			PX_ASSERT(torque.isFinite());
	
			coriolis = Cm::SpatialVectorF(force, torque);//.rotateInv(body2World);
		}
	}

	

	void FeatherstoneArticulation::computeRelativeTransformC2P(ArticulationData& data)
	{
		const ArticulationLink* links = data.getLinks();
		ArticulationLinkData* linkData = data.getLinkData();
		const PxU32 linkCount = data.getLinkCount();

		for (PxU32 linkID = 1; linkID < linkCount; ++linkID)
		{
			const ArticulationLink& link = links[linkID];
			ArticulationLinkData& linkDatum = linkData[linkID];
			const PxsBodyCore& bodyCore = *link.bodyCore;

			const PxTransform& body2World = bodyCore.body2World;

			const ArticulationLink& pLink = links[link.parent];
			const PxsBodyCore& pBodyCore = *pLink.bodyCore;
			const PxTransform& pBody2World = pBodyCore.body2World;

			//const PxTransform tC2P = pBody2World.transformInv(body2World).getNormalized();
			
			linkDatum.r = body2World.rotateInv(body2World.p - pBody2World.p);//body space of link i
			linkDatum.rw =body2World.p - pBody2World.p;
			
			const SpatialSubspaceMatrix& motionMatrix = data.mMotionMatrix[linkID];
			SpatialSubspaceMatrix& worldMotionMatrix = data.mWorldMotionMatrix[linkID];

			

			const PxU32 nbColumns = motionMatrix.getNumColumns();
			worldMotionMatrix.setNumColumns(nbColumns);
			for (PxU32 i = 0; i < nbColumns; ++i)
			{
				const Cm::UnAlignedSpatialVector worldRow = motionMatrix.getColumns()[i].rotate(body2World);
				
				worldMotionMatrix.setColumn(i, worldRow.top, worldRow.bottom);
				//Compute and store world joint axis!
				data.mJointTranData[linkID].worldJointAxis[i] = data.mJointData[linkID].jointAxis[i].rotate(body2World);
			}

#if FEATURESTONE_DEBUG
			{
				//debug
				PxMat33 pToC = c2p.getTranspose();
				//parentToChild -rR
				PxMat33 T2 = skewMatrixPR * pToC;

				PX_ASSERT(SpatialMatrix::isTranspose(linkDatum.childToParent.T, T2));
			}
#endif
		}
	}

	void FeatherstoneArticulation::computeRelativeTransformC2B(ArticulationData& data)
	{
		ArticulationLink* links = data.getLinks();
		ArticulationLinkData* linkData = data.getLinkData();
		const PxU32 linkCount = data.getLinkCount();

		const ArticulationLink& bLink = links[0];
		const PxTransform& bBody2World = bLink.bodyCore->body2World;

		for (PxU32 linkID = 1; linkID < linkCount; ++linkID)
		{
			const ArticulationLink& link = links[linkID];
			ArticulationLinkData& linkDatum = linkData[linkID];
			const PxsBodyCore& bodyCore = *link.bodyCore;

			const PxTransform& body2World = bodyCore.body2World;

			const PxVec3 rw = body2World.p - bBody2World.p;//body space of link i

			//rotation matrix cToP's inverse is rotation matrix pToC 
			linkDatum.childToBase = rw;
		}
	}

	void FeatherstoneArticulation::getDenseJacobian(PxArticulationCache& cache, PxU32 & nRows, PxU32 & nCols)
	{
		//make sure motionMatrix has been set
		//jcalc(mArticulationData);
		initializeCommonData();

		const PxU32 linkCount = mArticulationData.getLinkCount();
		ArticulationLink* links = mArticulationData.getLinks();

#if 0	//Enable this if you want to compare results of this function with results of computeLinkVelocities().

		if (mArticulationData.getDataDirty())
		{
			Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Articulation::getDenseJacobian(): commonInit need to be called first to initialize data!");
			return;
		}

		PxcScratchAllocator* allocator = reinterpret_cast<PxcScratchAllocator*>(cache.scratchAllocator);

		ScratchData scratchData;
		PxU8* tempMemory = allocateScratchSpatialData(allocator, linkCount, scratchData);

		scratchData.jointVelocities = mArticulationData.getJointVelocities();


		computeLinkVelocities(mArticulationData, scratchData);
		const ArticulationLink& baseLink = links[0];
		PxsBodyCore& core0 = *baseLink.bodyCore;
#endif

		ArticulationLinkData* linkData = mArticulationData.getLinkData();

		const PxU32 totalDofs = getDofs();
		const PxU32 jointCount = mArticulationData.getLinkCount() - 1;
		const bool fixBase = mArticulationData.getArticulationFlags() & PxArticulationFlag::eFIX_BASE;
		//matrix dims is 
		nCols = (fixBase ? 0 : 6) + totalDofs;
		nRows = (fixBase ? 0 : 6) + jointCount * 6;

//		auto jacobian = [&](PxU32 row, PxU32 col) -> PxReal &  { return cache.denseJacobian[nCols * row + col]; } ;
		#define jacobian(row, col)	cache.denseJacobian[nCols * (row) + (col)]
		
		PxU32 destRow = 0;
		PxU32 destCol = 0;

		if (!fixBase)
		{
			jacobian(0, 0) = 1.0f;
			jacobian(0, 1) = 0.0f;
			jacobian(0, 2) = 0.0f;
			jacobian(0, 3) = 0.0f;
			jacobian(0, 4) = 0.0f;
			jacobian(0, 5) = 0.0f;

			jacobian(1, 0) = 0.0f;
			jacobian(1, 1) = 1.0f;
			jacobian(1, 2) = 0.0f;
			jacobian(1, 3) = 0.0f;
			jacobian(1, 4) = 0.0f;
			jacobian(1, 5) = 0.0f;

			jacobian(2, 0) = 0.0f;
			jacobian(2, 1) = 0.0f;
			jacobian(2, 2) = 1.0f;
			jacobian(2, 3) = 0.0f;
			jacobian(2, 4) = 0.0f;
			jacobian(2, 5) = 0.0f;

			jacobian(3, 0) = 0.0f;
			jacobian(3, 1) = 0.0f;
			jacobian(3, 2) = 0.0f;
			jacobian(3, 3) = 1.0f;
			jacobian(3, 4) = 0.0f;
			jacobian(3, 5) = 0.0f;

			jacobian(4, 0) = 0.0f;
			jacobian(4, 1) = 0.0f;
			jacobian(4, 2) = 0.0f;
			jacobian(4, 3) = 0.0f;
			jacobian(4, 4) = 1.0f;
			jacobian(4, 5) = 0.0f;

			jacobian(5, 0) = 0.0f;
			jacobian(5, 1) = 0.0f;
			jacobian(5, 2) = 0.0f;
			jacobian(5, 3) = 0.0f;
			jacobian(5, 4) = 0.0f;
			jacobian(5, 5) = 1.0f;

			destRow += 6;
			destCol += 6;
		}

		for (PxU32 linkID = 1; linkID < linkCount; ++linkID)//each iteration of this writes 6 rows in the matrix
		{
			const ArticulationLink& link = links[linkID];
			ArticulationLinkData& linkDatum = linkData[linkID];
			const PxsBodyCore& bodyCore = *link.bodyCore;

			linkDatum.maxPenBias = bodyCore.maxPenBias;

			const PxTransform& body2World = bodyCore.body2World;

			const ArticulationJointCoreData& jointDatum = mArticulationData.getJointData(linkID);
			const PxU32 parentLinkID = link.parent;

			if (parentLinkID || !fixBase)
			{
				const ArticulationJointCoreData& parentJointDatum = mArticulationData.getJointData(parentLinkID);
				const PxU32 parentsFirstDestCol = parentJointDatum.jointOffset + (fixBase ? 0 : 6);
				const PxU32 parentsLastDestCol = parentsFirstDestCol + parentJointDatum.dof;

				const PxU32 parentsDestRow = (fixBase ? 0 : 6) + (parentLinkID - 1) * 6;

				for (PxU32 col = 0; col <= parentsLastDestCol; col++)
				{
					//copy downward the 6 cols from parent
					const PxVec3 parentAng(
						jacobian(parentsDestRow + 3, col),
						jacobian(parentsDestRow + 4, col),
						jacobian(parentsDestRow + 5, col)
						);

					const PxVec3 parentAngxRw = parentAng.cross(linkDatum.rw);

					jacobian(destRow + 0, col) = jacobian(parentsDestRow + 0, col) + parentAngxRw.x;
					jacobian(destRow + 1, col) = jacobian(parentsDestRow + 1, col) + parentAngxRw.y;
					jacobian(destRow + 2, col) = jacobian(parentsDestRow + 2, col) + parentAngxRw.z;

					jacobian(destRow + 3, col) = parentAng.x;
					jacobian(destRow + 4, col) = parentAng.y;
					jacobian(destRow + 5, col) = parentAng.z;
				}

				for (PxU32 col = parentsLastDestCol + 1; col < destCol; col++)
				{
					//fill with zeros.
					jacobian(destRow + 0, col) = 0.0f;
					jacobian(destRow + 1, col) = 0.0f;
					jacobian(destRow + 2, col) = 0.0f;

					jacobian(destRow + 3, col) = 0.0f;
					jacobian(destRow + 4, col) = 0.0f;
					jacobian(destRow + 5, col) = 0.0f;
				}
			}

			//diagonal block:
			for (PxU32 ind = 0; ind < jointDatum.dof; ++ind)
			{
				const Cm::UnAlignedSpatialVector& v = mArticulationData.mMotionMatrix[linkID][ind];

				const PxVec3 ang = body2World.rotate(v.top);
				const PxVec3 lin = body2World.rotate(v.bottom);

				jacobian(destRow + 0, destCol) = lin.x;
				jacobian(destRow + 1, destCol) = lin.y;
				jacobian(destRow + 2, destCol) = lin.z;

				jacobian(destRow + 3, destCol) = ang.x;
				jacobian(destRow + 4, destCol) = ang.y;
				jacobian(destRow + 5, destCol) = ang.z;

				destCol++;
			}

			//above diagonal block: always zero
			for (PxU32 col = destCol; col < nCols; col++)
			{
				jacobian(destRow + 0, col) = 0.0f;
				jacobian(destRow + 1, col) = 0.0f;
				jacobian(destRow + 2, col) = 0.0f;

				jacobian(destRow + 3, col) = 0.0f;
				jacobian(destRow + 4, col) = 0.0f;
				jacobian(destRow + 5, col) = 0.0f;
			}

			destRow += 6;
		}
		#undef jacobian

#if 0  //Enable this if you want to compare results of this function with results of computeLinkVelocities().

		PxReal * jointVels = mArticulationData.getJointVelocities();//size is totalDofs

		PxReal * jointSpaceVelsVector = new PxReal[nCols];
		PxReal * worldSpaceVelsVector = new PxReal[nRows];

		PxU32 offset = 0;

		//stack input:

		if (!fixBase)
		{
			jointSpaceVelsVector[0] = core0.linearVelocity[0];
			jointSpaceVelsVector[1] = core0.linearVelocity[1];
			jointSpaceVelsVector[2] = core0.linearVelocity[2];

			jointSpaceVelsVector[3] = core0.angularVelocity[0];
			jointSpaceVelsVector[4] = core0.angularVelocity[1];
			jointSpaceVelsVector[5] = core0.angularVelocity[2];

			offset = 6;
		}

		for (PxU32 i = 0; i < totalDofs; i++)
			jointSpaceVelsVector[i + offset] = jointVels[i];

		//multiply:

		for (PxU32 row = 0; row < nRows; row++)
		{
			worldSpaceVelsVector[row] = 0.0f;
			for (PxU32 col = 0; col < nCols; col++)
			{
				worldSpaceVelsVector[row] += jacobian(row, col)*jointSpaceVelsVector[col];
			}
		}

		//worldSpaceVelsVector should now contain the same result as scratchData.motionVelocities (except for swapped linear/angular vec3 order).

		delete[] jointSpaceVelsVector;
		delete[] worldSpaceVelsVector;

		allocator->free(tempMemory);
#endif
	}

	//compute all links velocities
	void FeatherstoneArticulation::computeLinkVelocities(ArticulationData& data,
		ScratchData& scratchData)
	{
		ArticulationLink* links = data.getLinks();
		ArticulationLinkData* linkData = data.getLinkData();
		const PxU32 linkCount = data.getLinkCount();
		const bool fixBase = data.getArticulationFlags() & PxArticulationFlag::eFIX_BASE;

		//motion velocities has to be in world space to avoid numerical errors caused by space 
		Cm::SpatialVectorF* motionVelocities = scratchData.motionVelocities;
		Cm::SpatialVectorF* motionAccelerations = scratchData.motionAccelerations;

		PxReal* jointVelocities = scratchData.jointVelocities;

		const ArticulationLink& baseLink = links[0];
		ArticulationLinkData& baseLinkDatum = linkData[0];

		PxsBodyCore& core0 = *baseLink.bodyCore;

		baseLinkDatum.maxPenBias = core0.maxPenBias;

		if (fixBase)
		{
			motionVelocities[0] = Cm::SpatialVectorF(PxVec3(0.f), PxVec3(0.f));
			motionAccelerations[0] = Cm::SpatialVectorF(PxVec3(0.f), PxVec3(0.f));
			
		}
		else
		{
			motionVelocities[0] = Cm::SpatialVectorF(core0.angularVelocity, core0.linearVelocity);

			//PX_ASSERT(core0.angularVelocity.isFinite() && PxAbs(core0.angularVelocity.x) < 10000.f && PxAbs(core0.angularVelocity.y) < 10000.f && PxAbs(core0.angularVelocity.z) < 10000.f);
			//PX_ASSERT(core0.linearVelocity.isFinite() && PxAbs(core0.linearVelocity.x) < 10000.f && PxAbs(core0.linearVelocity.y) < 10000.f && PxAbs(core0.linearVelocity.z) < 10000.f);
		}

		data.mRootPreMotionVelocity = motionVelocities[0];

		for (PxU32 linkID = 1; linkID < linkCount; ++linkID)
		{
			const ArticulationLink& link = links[linkID];
			ArticulationLinkData& linkDatum = linkData[linkID];
			const PxsBodyCore& bodyCore = *link.bodyCore;

			linkDatum.maxPenBias = bodyCore.maxPenBias;

			Cm::SpatialVectorF vel = FeatherstoneArticulation::translateSpatialVector(-linkDatum.rw, motionVelocities[link.parent]);

			if (jointVelocities)
			{
				ArticulationJointCoreData& jointDatum = data.getJointData(linkID);
				PxReal* jVelocity = &jointVelocities[jointDatum.jointOffset];

				const PxReal maxJVelocity = link.inboundJoint->maxJointVelocity;

				Cm::UnAlignedSpatialVector deltaV = Cm::UnAlignedSpatialVector::Zero();
				for (PxU32 ind = 0; ind < jointDatum.dof; ++ind)
				{
					PxReal jVel = PxClamp(jVelocity[ind], -maxJVelocity, maxJVelocity);
					deltaV += data.mWorldMotionMatrix[linkID][ind] * jVel;
					jVelocity[ind] = jVel;
				}

				vel.top += deltaV.top;
				vel.bottom += deltaV.bottom;
			}

			//PX_ASSERT(vel.top.isFinite() && PxAbs(vel.top.x) < 10000.f && PxAbs(vel.top.y) < 10000.f && PxAbs(vel.top.z) < 10000.f);
			//PX_ASSERT(vel.bottom.isFinite() && PxAbs(vel.bottom.x) < 10000.f && PxAbs(vel.bottom.y) < 10000.f && PxAbs(vel.bottom.z) < 10000.f);

			motionVelocities[linkID] = vel;
		}
	}

	void solveExtContact(const PxSolverConstraintDesc& desc, Vec3V& linVel0, Vec3V& linVel1, Vec3V& angVel0, Vec3V& angVel1,
		Vec3V& linImpulse0, Vec3V& linImpulse1, Vec3V& angImpulse0, Vec3V& angImpulse1, bool doFriction);

	void solveExt1D(const PxSolverConstraintDesc& desc, Vec3V& linVel0, Vec3V& linVel1, Vec3V& angVel0, Vec3V& angVel1,
		Vec3V& li0, Vec3V& li1, Vec3V& ai0, Vec3V& ai1);

	void solveExt1D(const PxSolverConstraintDesc& desc, Vec3V& linVel0, Vec3V& linVel1, Vec3V& angVel0, Vec3V& angVel1,
		const Vec3V& linMotion0, const Vec3V& linMotion1, const Vec3V& angMotion0, const Vec3V& angMotion1,
		const QuatV& rotA, const QuatV& rotB, const PxReal elapsedTimeF32, Vec3V& linImpulse0, Vec3V& linImpulse1, Vec3V& angImpulse0,
		Vec3V& angImpulse1);
	
	void solveExtContactStep(const PxSolverConstraintDesc& desc, Vec3V& linVel0, Vec3V& linVel1, Vec3V& angVel0, Vec3V& angVel1,
		Vec3V& linDelta0, Vec3V& linDelta1, Vec3V& angDelta0, Vec3V& angDelta1, Vec3V& linImpulse0, Vec3V& linImpulse1, Vec3V& angImpulse0, Vec3V& angImpulse1,
		bool doFriction, const PxReal minPenetration, const PxReal elapsedTimeF32);




	void solveStaticConstraint(const PxSolverConstraintDesc& desc, Cm::SpatialVectorF& linkV,
		Cm::SpatialVectorF& impulse, Cm::SpatialVectorF& deltaV, const Cm::SpatialVectorF& motion,
		const PxQuat& rot, bool isTGS, PxReal elapsedTime,	const PxReal minPenetration)
	{
		PX_UNUSED(isTGS);
		PX_UNUSED(elapsedTime);
		PX_UNUSED(minPenetration);
		Vec3V linVel = V3LoadA(linkV.bottom);
		Vec3V angVel = V3LoadA(linkV.top);

		Vec3V linVel0, linVel1, angVel0, angVel1;
		Vec3V li0 = V3Zero(), li1 = V3Zero(), ai0 = V3Zero(), ai1 = V3Zero();

		if (isTGS)
		{
			PxQuat idt(PxIdentity);
			Vec3V linMotion0, angMotion0, linMotion1, angMotion1;
			QuatV rotA, rotB;
			if (desc.linkIndexA != PxSolverConstraintDesc::NO_LINK)
			{
				linVel0 = linVel;
				angVel0 = angVel;
				linMotion0 = V3LoadA(motion.bottom);
				angMotion0 = V3LoadA(motion.top);
				rotA = QuatVLoadU(&rot.x);
				rotB = QuatVLoadU(&idt.x);
				linVel1 = angVel1 = linMotion1 = angMotion1 = V3Zero();
			}
			else
			{
				linVel1 = linVel;
				angVel1 = angVel;
				linMotion1 = V3LoadA(motion.bottom);
				angMotion1 = V3LoadA(motion.top);
				rotB = QuatVLoadU(&rot.x);
				rotA = QuatVLoadU(&idt.x);
				linVel0 = angVel0 = linMotion0 = angMotion0 = V3Zero();
			}

			if (*desc.constraint == DY_SC_TYPE_EXT_CONTACT)
			{
				Dy::solveExtContactStep(desc, linVel0, linVel1, angVel0, angVel1, linMotion0, linMotion1, angMotion0, angMotion1,
					li0, li1, ai0, ai1, true, minPenetration, elapsedTime);
			}
			else
			{
				Dy::solveExt1D(desc, linVel0, linVel1, angVel0, angVel1, linMotion0, linMotion1, angMotion0, angMotion1, 
					rotA, rotB, elapsedTime, li0, li1, ai0, ai1);
			}
		}
		else
		{
			if (desc.linkIndexA != PxSolverConstraintDesc::NO_LINK)
			{
				linVel0 = linVel;
				angVel0 = angVel;
				linVel1 = angVel1 = V3Zero();
			}
			else
			{
				linVel1 = linVel;
				angVel1 = angVel;
				linVel0 = angVel0 = V3Zero();
			}

			if (*desc.constraint == DY_SC_TYPE_EXT_CONTACT)
			{
				Dy::solveExtContact(desc, linVel0, linVel1, angVel0, angVel1, li0, li1, ai0, ai1, true);
			}
			else
			{
				Dy::solveExt1D(desc, linVel0, linVel1, angVel0, angVel1, li0, li1, ai0, ai1);
			}
		}

		Cm::SpatialVectorF newVel, newImp;

		if (desc.linkIndexA != PxSolverConstraintDesc::NO_LINK)
		{
			V4StoreA(Vec4V_From_Vec3V(linVel0), &newVel.bottom.x);
			V4StoreA(Vec4V_From_Vec3V(angVel0), &newVel.top.x);

			V4StoreA(Vec4V_From_Vec3V(li0), &newImp.top.x);
			V4StoreA(Vec4V_From_Vec3V(ai0), &newImp.bottom.x);
		}
		else
		{
			V4StoreA(Vec4V_From_Vec3V(linVel1), &newVel.bottom.x);
			V4StoreA(Vec4V_From_Vec3V(angVel1), &newVel.top.x);

			V4StoreA(Vec4V_From_Vec3V(li1), &newImp.top.x);
			V4StoreA(Vec4V_From_Vec3V(ai1), &newImp.bottom.x);
		}

		deltaV.top += (newVel.top - linkV.top);
		deltaV.bottom += (newVel.bottom - linkV.bottom);
		linkV.top = newVel.top;
		linkV.bottom = newVel.bottom;
		impulse -= newImp;
	}

	void writeBackContact(const PxSolverConstraintDesc& desc, SolverContext& cache,
			PxSolverBodyData& bd0, PxSolverBodyData& bd1);

	void writeBack1D(const PxSolverConstraintDesc& desc, SolverContext&, 
		PxSolverBodyData&, PxSolverBodyData&);

	void writeBackContact(const PxSolverConstraintDesc& desc, SolverContext* cache);
	void writeBack1D(const PxSolverConstraintDesc& desc);

	void FeatherstoneArticulation::writebackInternalConstraints(bool isTGS)
	{
		SolverContext context;
		PxSolverBodyData data;
		for (PxU32 i = 0; i < mStaticConstraints.size(); ++i)
		{
			PxSolverConstraintDesc& desc = mStaticConstraints[i];
			if (isTGS)
			{
				if (*desc.constraint == DY_SC_TYPE_EXT_CONTACT)
				{
					writeBackContact(static_cast<PxSolverConstraintDesc&>(desc), NULL);
				}
				else
				{
					writeBack1D(static_cast<PxSolverConstraintDesc&>(desc));
				}
			}
			else
			{
				
				if (*desc.constraint == DY_SC_TYPE_EXT_CONTACT)
				{
					writeBackContact(desc, context, data, data);
				}
				else
				{
					writeBack1D(desc, context, data, data);
				}
			}
		}
	}

	void concludeContact(const PxSolverConstraintDesc& desc, SolverContext& cache);

	void conclude1D(const PxSolverConstraintDesc& desc, SolverContext& cache);

	void concludeContact(const PxSolverConstraintDesc& desc);

	void conclude1DStep(const PxSolverConstraintDesc& desc);


	void FeatherstoneArticulation::concludeInternalConstraints(bool isTGS)
	{
		PX_UNUSED(isTGS);
		SolverContext context;
		for (PxU32 i = 0; i < mStaticConstraints.size(); ++i)
		{
			PxSolverConstraintDesc& desc = mStaticConstraints[i];
			if (isTGS)
			{
				if (*desc.constraint == DY_SC_TYPE_EXT_CONTACT)
				{
					concludeContact(desc);
				}
				else
				{
					conclude1DStep(desc);
				}
			}
			else
			{

				if (*desc.constraint == DY_SC_TYPE_EXT_CONTACT)
				{
					concludeContact(desc, context);
				}
				else
				{
					conclude1D(desc, context);
				}
			}
		}
	}


	Cm::SpatialVectorF FeatherstoneArticulation::solveInternalConstraintRecursive(const PxReal dt, const PxReal invDt,
		Cm::SpatialVectorF* impulses, Cm::SpatialVectorF* DeltaV, bool velocityIteration, bool isTGS,
		const PxReal elapsedTime, const PxU32 linkID, const Cm::SpatialVectorF& parentDeltaV, PxU32& dofId, PxU32& lockId)
	{
		//PxU32 linkID = stack[stackSize];
		const ArticulationLink& link = mArticulationData.mLinks[linkID];
		//const ArticulationLink& plink = links[link.parent];
		ArticulationLinkData& linkDatum = mArticulationData.getLinkData(linkID);

		//PxTransform* transforms = mArticulationData.mPreTransform.begin();

		PX_UNUSED(linkDatum);

		const ArticulationJointCoreData& jointDatum = mArticulationData.getJointData(linkID);
	//	const ArticulationJointTargetData& jointTranDatum = mArticulationData.getJointTranData(linkID);

		Cm::SpatialVectorF i1(PxVec3(0.f), PxVec3(0.f));

		//We know the absolute parentDeltaV from the call to this function so no need to modify it. 
		Cm::SpatialVectorF parentV = parentDeltaV + mArticulationData.mMotionVelocities[link.parent];

		Cm::SpatialVectorF parentVelContrib = propagateVelocityTestImpulseW(linkDatum.rw,
			mArticulationData.mWorldSpatialArticulatedInertia[linkID], mArticulationData.mInvStIs[linkID], mArticulationData.mWorldMotionMatrix[linkID], mArticulationData.getSpatialZAVector(linkID), parentDeltaV);

		Cm::SpatialVectorF childV = mArticulationData.mMotionVelocities[linkID] + parentVelContrib;

		Cm::UnAlignedSpatialVector i0(PxVec3(0.f), PxVec3(0.f));

		Cm::SpatialVectorF dv1 = parentVelContrib;

		PxU32 tempDofId = dofId;

		const PxReal maxJointVel = link.inboundJoint->maxJointVelocity;

		//If we have any internal constraints to process (parent/child limits/locks/drives)
		if (jointDatum.dofInternalConstraintMask || jointDatum.lockedAxes)
		{
			PxReal* jPosition = &mArticulationData.mJointPosition[jointDatum.jointOffset];
			for (PxU32 dof = 0; dof < jointDatum.dof; ++dof)
			{
				if (jointDatum.dofInternalConstraintMask & (1 << dof))
				{
					ArticulationInternalConstraint& constraint = mArticulationData.mInternalConstraints[dofId++];

					PxReal jointP = jPosition[dof];
					//printf("LinkId %i JointPosition = %f\n", linkID, jointP);

					PxReal driveTarget = constraint.driveTarget;
					if (!constraint.isLinearConstraint)
					{
						//Clamp jointP within +/- PxPi
						if (jointP > PxTwoPi)
						{
							jointP -= 2.f*PxTwoPi;
							driveTarget -= 2.f*PxTwoPi;
						}
						else if (jointP < -PxTwoPi)
						{
							jointP += 2.f*PxTwoPi;
							driveTarget += 2.f*PxTwoPi;
						}
					}

					PxReal error = ((driveTarget + constraint.driveTargetVel*constraint.response*elapsedTime) - jointP);
					
					PxReal jointV = constraint.row1.innerProduct(childV) - constraint.row0.innerProduct(parentV);

					const PxReal appliedFriction = constraint.frictionForce*constraint.frictionForceCoefficient;

					PxReal frictionForce = PxClamp(-jointV *constraint.recipResponse + appliedFriction,
						-constraint.maxFrictionForce, constraint.maxFrictionForce);

					PxReal frictionDeltaF = frictionForce - appliedFriction;

					constraint.frictionForce += frictionDeltaF;

					jointV += frictionDeltaF * constraint.response;



					PxReal unclampedForce = constraint.driveImpulseMultiplier * constraint.driveForce +
						jointV * constraint.driveVelMultiplier + constraint.driveTargetVel + error * constraint.driveBiasCoefficient;

					PxReal clampedForce = PxClamp(unclampedForce, -constraint.maxDriveForce, constraint.maxDriveForce);
					PxReal driveDeltaF = (clampedForce - constraint.driveForce);

					//Where we will be next frame - we use this to compute error bias terms to correct limits and drives...

					jointV += driveDeltaF * constraint.response;

					driveDeltaF += frictionDeltaF;

					//printf("LinkID %i driveDeltaV = %f, jointV = %f\n", linkID, driveDeltaF, jointV);


					PxReal deltaF = 0.f;

					bool limited = false;

					if(!velocityIteration)
					{
						PxReal futureP = jointP + jointV * dt;
						if (jointP > constraint.highLimit || futureP > constraint.highLimit)
						{
							PxReal erp = jointP > constraint.highLimit ? constraint.erp : 1.f;
							limited = true;
							PxReal deltaV = (constraint.highLimit - futureP)*invDt*erp;
							PxReal newJointV = PxClamp(jointV + deltaV, -maxJointVel, maxJointVel);
							deltaV = newJointV - jointV;
							deltaF = PxMin(constraint.highImpulse + deltaV * constraint.recipResponse, 0.f) - constraint.highImpulse;

							constraint.highImpulse += deltaF;

						}
						else if (jointP < constraint.lowLimit || futureP < constraint.lowLimit)
						{
							PxReal erp = jointP < constraint.lowLimit ? constraint.erp : 1.f;
							limited = true;
							PxReal deltaV = (constraint.lowLimit - futureP)*invDt*erp;
							PxReal newJointV = PxClamp(jointV + deltaV, -maxJointVel, maxJointVel);
							deltaV = newJointV - jointV;
							deltaF = PxMax(constraint.lowImpulse + deltaV * constraint.recipResponse, 0.f) - constraint.lowImpulse;
							constraint.lowImpulse += deltaF;
						}
					}

					if (!limited)
					{
						const PxReal forceLimit = -jointV*constraint.recipResponse;
						if (jointV > 0.f)
						{
							deltaF = PxMax(forceLimit, -constraint.lowImpulse);
							constraint.lowImpulse += deltaF;
						}
						else
						{
							deltaF = PxMin(forceLimit, -constraint.highImpulse);
							constraint.highImpulse += deltaF;
						}
					}

					deltaF += driveDeltaF;

					if (deltaF != 0.f)
					{
						//impulse = true;
						constraint.driveForce = clampedForce;

						i0 += constraint.row0 * deltaF;
						i1.top -= constraint.row1.top * deltaF;
						i1.bottom -= constraint.row1.bottom * deltaF;

						const Cm::UnAlignedSpatialVector deltaVP = constraint.deltaVA * (-deltaF);
						const Cm::UnAlignedSpatialVector deltaVC = constraint.deltaVB * (-deltaF);

						parentV += Cm::SpatialVectorF(deltaVP.top, deltaVP.bottom);
						childV += Cm::SpatialVectorF(deltaVC.top, deltaVC.bottom);

						dv1.top += deltaVC.top;
						dv1.bottom += deltaVC.bottom;
					}

				}
			}

			PxU32 startIdx = PxU32(jointDatum.dof - jointDatum.lockedAxes);

			for (PxU32 i = startIdx; i < jointDatum.dof; ++i)
			{
				ArticulationInternalLockedAxis& lockAxis = mArticulationData.mInternalLockedAxes[lockId++];

				PxReal jointV = lockAxis.axis.dot(childV.top) - lockAxis.axis.dot(parentV.top);

				PxReal deltaJointP = lockAxis.axis.dot(mArticulationData.mDeltaMotionVector[linkID].top) - lockAxis.axis.dot(mArticulationData.mDeltaMotionVector[link.parent].top);

				PxReal deltaV = -jointV;
				//We aim to zero velocity but also correct bias...
				if (!velocityIteration)
					deltaV += (lockAxis.error - deltaJointP) * lockAxis.biasScale;

				//Clamp in the off chance that we are applying an insanely huge angular velocity to correct a large joint error
				PxReal newJV = PxClamp(jointV + deltaV, -maxJointVel, maxJointVel);
				deltaV = newJV - jointV;

				PxReal deltaF = deltaV * lockAxis.recipResponse;

				if (deltaF != 0.f)
				{
					//impulse = true;

					i0.bottom += lockAxis.axis * deltaF;
					i1.bottom -= lockAxis.axis * deltaF;

					const Cm::UnAlignedSpatialVector tDeltaVP = lockAxis.deltaVA * (-deltaF);
					const Cm::UnAlignedSpatialVector tDeltaVC = lockAxis.deltaVB * (-deltaF);

					const Cm::SpatialVectorF deltaVP = Cm::SpatialVectorF(tDeltaVP.top, tDeltaVP.bottom);
					const Cm::SpatialVectorF deltaVC = Cm::SpatialVectorF(tDeltaVC.top, tDeltaVC.bottom);

					parentV += deltaVP;
					childV += deltaVC;
					dv1.top += deltaVC.top;
					dv1.bottom += deltaVC.bottom;
				}

			}
		}



		const PxU32 nbStaticConstraints = mArticulationData.mNbStaticConstraints[linkID];

		const Cm::SpatialVectorF& deltaMotion = mArticulationData.getDeltaMotionVector(linkID);
		const PxQuat& deltaQ = getDeltaQ(linkID);

		PxU32 startIdx = mArticulationData.mStaticConstraintStartIndex[linkID];
		for (PxU32 i = 0; i < nbStaticConstraints; ++i)
		{
			PxSolverConstraintDesc& desc = mStaticConstraints[startIdx++];
			solveStaticConstraint(desc, childV, i1, dv1, deltaMotion, deltaQ, isTGS, elapsedTime, velocityIteration ? 0.f : -PX_MAX_F32);
		}

		if (link.children)
		{
			Cm::SpatialVectorF extraForces(PxVec3(0.f), PxVec3(0.f));
			for (ArticulationBitField children = link.children; children != 0;)
			{
				const PxU32 child = ArticulationLowestSetBit(children);
				Cm::SpatialVectorF childImp = solveInternalConstraintRecursive(dt, invDt, impulses, DeltaV, velocityIteration, isTGS, elapsedTime, child, dv1, dofId, lockId);
				
				if(0)
				{
					if (jointDatum.dofInternalConstraintMask || jointDatum.lockedAxes)
					{
						for (PxU32 dof = 0; dof < jointDatum.dof; ++dof)
						{
							if (jointDatum.dofInternalConstraintMask & (1 << dof))
							{
								//Force back from child links
								ArticulationInternalConstraint& constraint = mArticulationData.mInternalConstraints[tempDofId + dof];

								const Cm::UnAlignedSpatialVector& sa = mArticulationData.mMotionMatrix[linkID][dof];
								const PxReal stZ = sa.innerProduct(childImp);

								//OK. Now subtract the change in vel...
								PxReal unclampedForce = constraint.driveImpulseMultiplier *constraint.driveForce + stZ*constraint.driveBackPropagateScale;

								PxReal clampedForce = PxClamp(unclampedForce, -constraint.maxDriveForce, constraint.maxDriveForce);

								PxReal driveDeltaF = (clampedForce - constraint.driveForce);

								constraint.driveForce = clampedForce;

								//Now apply that force back on itself...

								Cm::UnAlignedSpatialVector impulse0 = constraint.row0 * driveDeltaF;
								Cm::UnAlignedSpatialVector impulse1 = constraint.row1 * driveDeltaF;

								childImp.top -= impulse0.top;
								childImp.bottom -= impulse0.bottom;

								extraForces.top -= impulse1.top;
								extraForces.bottom -= impulse1.bottom;
								//childImpulses.bottom -= jointDatum.jointAxis[dof] * propagatedForce;

							}
						}
					}					
				}

				i1 += childImp;

				//Now work out how those impulses impacted my velocity (if we have a next child)
				children &= (children - 1);
				if (children != 0)
				{
					//Propagate the childImp to my dv1 so that the next constraint gets to see an updated velocity state based
					//on the propagation of the child velocities
					Cm::SpatialVectorF deltaV = mArticulationData.mResponseMatrixW[linkID].getResponse(-childImp);
					dv1 += deltaV;

				}
			}

			//Cm::SpatialVectorF parentImpulses = extraForces.rotate(transforms[linkID]);//propagateImpulse(mArticulationData.mIsInvD[linkID], mArticulationData.mChildToParent[linkID], mArticulationData.mMotionMatrix[linkID], extraForces).rotate(transforms[link.parent]);

			//(1) project impulses onto joints. Resist motion around driven axes (where possible). 
			//Allow rest to propagate through the system...
			i0.top += extraForces.top;
			i0.bottom += extraForces.bottom;
		}
		
		
		//Store my impulses (including the propagation of any child links' impulses)
		impulses[linkID] = i1;

		return Cm::SpatialVectorF(i0.top, i0.bottom) +
			propagateImpulseW(mArticulationData.mIsInvDW[linkID], linkDatum.rw, mArticulationData.mWorldMotionMatrix[linkID], i1);
	}


	void FeatherstoneArticulation::solveInternalConstraints(const PxReal dt, const PxReal invDt,
		Cm::SpatialVectorF* impulses, Cm::SpatialVectorF* DeltaV, bool velocityIteration, bool isTGS,
		const PxReal elapsedTime)
	{
		const PxU32 count = mArticulationData.getLinkCount();

		if (mArticulationData.mInternalConstraints.size() == 0 && mArticulationData.mInternalLockedAxes.size() == 0 
			&& mStaticConstraints.size() == 0)
			return;

		const bool fixBase = mArticulationData.getArticulationFlags() & PxArticulationFlag::eFIX_BASE;

		PxU32* staticConstraintCounts = mArticulationData.mNbStaticConstraints.begin();
		PxU32* staticConstraintStarts = mArticulationData.mStaticConstraintStartIndex.begin();
		
		ArticulationLink* links = mArticulationData.getLinks();
		Cm::SpatialVectorF* baseVelocities = mArticulationData.getMotionVelocities();
		//PxTransform* transforms = mArticulationData.mPreTransform.begin();		

		Cm::SpatialVectorF* deferredZ = mArticulationData.getSpatialZAVectors();

		const PxReal minPenetration = velocityIteration ? 0.f : -PX_MAX_F32;

		Cm::SpatialVectorF rootLinkV;
		{
			Cm::SpatialVectorF rootLinkDeltaV(PxVec3(0.f), PxVec3(0.f));

			if (!fixBase)
			{
				//Cm::SpatialVectorF temp =  mArticulationData.getBaseInvSpatialArticulatedInertia() * (-deferredZ[0]);
				//const PxTransform& body2World0 = transforms[0];
				//rootLinkDeltaV = temp.rotate(body2World0);
				//temp is now in world space!
				rootLinkDeltaV = mArticulationData.getBaseInvSpatialArticulatedInertiaW() * (-deferredZ[0]);				
			}

			rootLinkV = rootLinkDeltaV + baseVelocities[0];

			Cm::SpatialVectorF im0 = Cm::SpatialVectorF::Zero();
			{
				const PxU32 nbStaticConstraints = staticConstraintCounts[0];

				if (nbStaticConstraints)
				{
					const Cm::SpatialVectorF& deltaMotion = mArticulationData.getDeltaMotionVector(0);
					const PxQuat& deltaQ = getDeltaQ(0);

					PxU32 startIdx = staticConstraintStarts[0];
					for (PxU32 i = 0; i < nbStaticConstraints; ++i)
					{
						PxSolverConstraintDesc& desc = mStaticConstraints[startIdx++];

						solveStaticConstraint(desc, rootLinkV, im0, rootLinkDeltaV, deltaMotion, deltaQ, isTGS, elapsedTime, minPenetration);
					}

					//Impulses and deferredZ are now in world space, not link space!
					/*im0.top = transforms[0].rotateInv(im0.top);
					im0.bottom = transforms[0].rotateInv(im0.bottom);*/
				}
			}	

			PxU32 dofId = 0, lockId = 0;

			for (ArticulationBitField children = links[0].children; children != 0;)
			{
				//index of child of link h on path to link linkID
				const PxU32 child = ArticulationLowestSetBit(children);

				children &= (children - 1);

				Cm::SpatialVectorF imp = solveInternalConstraintRecursive(dt, invDt, impulses, DeltaV, velocityIteration, isTGS, elapsedTime, child, 
					rootLinkDeltaV, dofId, lockId);

				im0 += imp;

				//There's an impulse, we have to work out how it impacts our velocity (only if required (we have more children to traverse))!
				if (!fixBase && children != 0)
				{
					/*Cm::SpatialVectorF temp = mArticulationData.getBaseInvSpatialArticulatedInertia() * (-imp);

					const PxTransform& body2World0 = transforms[0];
					rootLinkDeltaV += temp.rotate(body2World0);*/

					//Impulses and deltaVs are all now in world space
					rootLinkDeltaV += mArticulationData.getBaseInvSpatialArticulatedInertiaW() * (-imp);
				}

			}

			impulses[0].top = im0.top;
			impulses[0].bottom = im0.bottom;

			if (1)
			{
				//Now propagate impulses...
				//if (impulse)
				{
					for (PxU32 i = 0; i < count; ++i)
					{
						deferredZ[i] += impulses[i];
					}
					mArticulationData.mJointDirty = true;
				}
			}
		}
	}

	//This method is for user update the root link transform so we need to
	//fix up other link's position. In this case, we should assume all joint
	//velocity/pose is to be zero
	void FeatherstoneArticulation::teleportRootLink()
	{
		//make sure motionMatrix has been set
		jcalc(mArticulationData);

		const PxU32 linkCount = mArticulationData.getLinkCount();
		ArticulationLink* links = mArticulationData.getLinks();
		PxReal* jointPositions = mArticulationData.getJointPositions();
		Cm::SpatialVectorF* motionVelocities = mArticulationData.getMotionVelocities();

		for (PxU32 linkID = 1; linkID < linkCount; ++linkID)
		{
			ArticulationLink& link = links[linkID];
			const PxTransform oldTransform = link.bodyCore->body2World;

			ArticulationLink& pLink = links[link.parent];
			const PxTransform pBody2World = pLink.bodyCore->body2World;

			ArticulationJointCore* joint = link.inboundJoint;
			ArticulationJointCoreData& jointDatum = mArticulationData.getJointData(linkID);

			PxReal* jPosition = &jointPositions[jointDatum.jointOffset];

			PxQuat newParentToChild;
			PxQuat newWorldQ;
			PxVec3 r;

			const PxVec3 childOffset = -joint->childPose.p;
			const PxVec3 parentOffset = joint->parentPose.p;

			const PxQuat relativeQuat = mArticulationData.mRelativeQuat[linkID];

			switch (joint->jointType)
			{
			case PxArticulationJointType::ePRISMATIC:
			{
				newParentToChild = relativeQuat;
				const PxVec3 e = newParentToChild.rotate(parentOffset);
				const PxVec3 d = childOffset;

				const PxVec3& u = mArticulationData.mMotionMatrix[linkID][0].bottom;

				r = e + d + u * jPosition[0];
				break;
			}
			case PxArticulationJointType::eREVOLUTE:
			{
				const PxVec3& u = mArticulationData.mMotionMatrix[linkID][0].top;

				PxQuat jointRotation = PxQuat(-jPosition[0], u);
				if (jointRotation.w < 0)	//shortest angle.
					jointRotation = -jointRotation;

				newParentToChild = (jointRotation * relativeQuat).getNormalized();

				const PxVec3 e = newParentToChild.rotate(parentOffset);
				const PxVec3 d = childOffset;
				r = e + d;

				/*PxVec3 worldAngVel = oldTransform.rotate(link.motionVelocity.top);

				newWorldQ = Ps::exp(worldAngVel*dt) * oldTransform.q;

				PxQuat newParentToChild2 = (newWorldQ.getConjugate() * joint->relativeQuat * pBody2World.q).getNormalized();

				const PxVec3 e2 = newParentToChild2.rotate(parentOffset);
				const PxVec3 d2 = childOffset;
				r = e2 + d2;*/

				PX_ASSERT(r.isFinite());

				break;
			}
			case PxArticulationJointType::eSPHERICAL:
			{

				//PxVec3 angVel(joint->jointVelocity[0], joint->jointVelocity[1], joint->jointVelocity[2]);
				//PxVec3 worldAngVel = pLink.bodyCore->angularVelocity + oldTransform.rotate(angVel);

				PxVec3 worldAngVel = motionVelocities[linkID].top;

				/*const PxReal eps = 0.001f;
				const PxVec3 dif = worldAngVel - worldAngVel2;
				PX_ASSERT(PxAbs(dif.x) < eps && PxAbs(dif.y) < eps && PxAbs(dif.z) < eps);*/

				newWorldQ = Ps::exp(worldAngVel) * oldTransform.q;

				newParentToChild = (newWorldQ.getConjugate() * relativeQuat * pBody2World.q).getNormalized();

				const PxVec3 e = newParentToChild.rotate(parentOffset);
				const PxVec3 d = childOffset;
				r = e + d;

				PX_ASSERT(r.isFinite());
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

			PxTransform& body2World = link.bodyCore->body2World;
			body2World.q = (pBody2World.q * newParentToChild.getConjugate()).getNormalized();
			body2World.p = pBody2World.p + body2World.q.rotate(r);

			PX_ASSERT(body2World.isSane());
		}
	}


	PxU8* FeatherstoneArticulation::allocateScratchSpatialData(PxcScratchAllocator* allocator,
		const PxU32 linkCount, ScratchData& scratchData, bool fallBackToHeap)
	{
		const PxU32 size = sizeof(Cm::SpatialVectorF) * linkCount;
		const PxU32 totalSize = size * 4 + sizeof(Dy::SpatialMatrix) * linkCount;

		PxU8* tempMemory = reinterpret_cast<PxU8*>(allocator->alloc(totalSize, fallBackToHeap));

		scratchData.motionVelocities = reinterpret_cast<Cm::SpatialVectorF*>(tempMemory);
		PxU32 offset = size;
		scratchData.motionAccelerations = reinterpret_cast<Cm::SpatialVectorF*>(tempMemory + offset);
		offset += size;
		scratchData.coriolisVectors = reinterpret_cast<Cm::SpatialVectorF*>(tempMemory + offset);
		offset += size;
		scratchData.spatialZAVectors = reinterpret_cast<Cm::SpatialVectorF*>(tempMemory + offset);
		offset += size;
		scratchData.compositeSpatialInertias = reinterpret_cast<Dy::SpatialMatrix*>(tempMemory + offset);

		return tempMemory;
	}

/*	void FeatherstoneArticulation::allocateScratchSpatialData(DyScratchAllocator& allocator,
		const PxU32 linkCount, ScratchData& scratchData)
	{
		const PxU32 size = sizeof(Cm::SpatialVectorF) * linkCount;
		const PxU32 totalSize = size * 5 + sizeof(Dy::SpatialMatrix) * linkCount;

		PxU8* tempMemory = allocator.alloc<PxU8>(totalSize);

		scratchData.motionVelocities = reinterpret_cast<Cm::SpatialVectorF*>(tempMemory);
		PxU32 offset = size;
		scratchData.motionAccelerations = reinterpret_cast<Cm::SpatialVectorF*>(tempMemory + offset);
		offset += size;
		scratchData.coriolisVectors = reinterpret_cast<Cm::SpatialVectorF*>(tempMemory + offset);
		offset += size;
		scratchData.spatialZAVectors = reinterpret_cast<Cm::SpatialVectorF*>(tempMemory + offset);
		offset += size;
		scratchData.externalAccels = reinterpret_cast<Cm::SpatialVector*>(tempMemory + offset);
		offset += size;
		scratchData.compositeSpatialInertias = reinterpret_cast<Dy::SpatialMatrix*>(tempMemory + offset);
		
	}*/


}//namespace Dy
}
