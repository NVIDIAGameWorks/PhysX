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

#include "PxImmediateMode.h"
#include "../../lowleveldynamics/src/DyBodyCoreIntegrator.h"
#include "../../lowleveldynamics/src/DyContactPrep.h"
#include "../../lowleveldynamics/src/DyCorrelationBuffer.h"
#include "../../lowleveldynamics/src/DyConstraintPrep.h"
#include "../../lowleveldynamics/src/DySolverControl.h"
#include "../../lowleveldynamics/src/DySolverContext.h"
#include "../../lowlevel/common/include/collision/PxcContactMethodImpl.h"
#include "../../lowleveldynamics/src/DyTGSContactPrep.h"
#include "GuPersistentContactManifold.h"
#include "NpConstraint.h"

// PT: just for Dy::DY_ARTICULATION_MAX_SIZE
#include "../../lowleveldynamics/include/DyFeatherstoneArticulation.h"

#include "../../lowlevel/common/include/utils/PxcScratchAllocator.h"

using namespace physx;
using namespace Dy;
using namespace immediate;

void immediate::PxConstructSolverBodies(const PxRigidBodyData* inRigidData, PxSolverBodyData* outSolverBodyData, const PxU32 nbBodies, const PxVec3& gravity, const PxReal dt)
{
	PX_ASSERT((reinterpret_cast<size_t>(inRigidData) & 0xf) == 0);
	PX_ASSERT((reinterpret_cast<size_t>(outSolverBodyData) & 0xf) == 0);

	for(PxU32 a=0; a<nbBodies; a++)
	{
		const PxRigidBodyData& rigidData = inRigidData[a];
		PxVec3 lv = rigidData.linearVelocity, av = rigidData.angularVelocity;
		Dy::bodyCoreComputeUnconstrainedVelocity(gravity, dt, rigidData.linearDamping, rigidData.angularDamping, 1.0f, rigidData.maxLinearVelocitySq, rigidData.maxAngularVelocitySq, lv, av, false);
		Dy::copyToSolverBodyData(lv, av, rigidData.invMass, rigidData.invInertia, rigidData.body2World, -rigidData.maxDepenetrationVelocity, rigidData.maxContactImpulse, IG_INVALID_NODE, PX_MAX_F32, outSolverBodyData[a], 0);
	}
}

void immediate::PxConstructStaticSolverBody(const PxTransform& globalPose, PxSolverBodyData& solverBodyData)
{
	PX_ASSERT((reinterpret_cast<size_t>(&solverBodyData) & 0xf) == 0);

	const PxVec3 zero(0.0f);
	Dy::copyToSolverBodyData(zero, zero, 0.f, zero, globalPose, -PX_MAX_F32, PX_MAX_F32, IG_INVALID_NODE, PX_MAX_F32, solverBodyData, 0);
}

void immediate::PxIntegrateSolverBodies(PxSolverBodyData* solverBodyData, PxSolverBody* solverBody, const PxVec3* linearMotionVelocity, const PxVec3* angularMotionState, const PxU32 nbBodiesToIntegrate, const PxReal dt)
{
	PX_ASSERT((reinterpret_cast<size_t>(solverBodyData) & 0xf) == 0);
	PX_ASSERT((reinterpret_cast<size_t>(solverBody) & 0xf) == 0);

	for (PxU32 i = 0; i < nbBodiesToIntegrate; ++i)
	{
		PxVec3 lmv = linearMotionVelocity[i];
		PxVec3 amv = angularMotionState[i];
		Dy::integrateCore(lmv, amv, solverBody[i], solverBodyData[i], dt);
	}
}

namespace
{
	// PT: this class is makes it possible to call the FeatherstoneArticulation protected functions from here.
	class immArticulation : public FeatherstoneArticulation
	{
		public:
														immArticulation(const PxFeatherstoneArticulationData& data);
														~immArticulation();

		PX_FORCE_INLINE	void							immSolveInternalConstraints(PxReal dt, PxReal invDt, Cm::SpatialVectorF* impulses, Cm::SpatialVectorF* DeltaV, const PxReal elapsedTime, bool velocityIteration, bool isTGS)
														{
															FeatherstoneArticulation::solveInternalConstraints(dt, invDt, impulses, DeltaV, velocityIteration, isTGS, elapsedTime);
														}

		PX_FORCE_INLINE	void							immComputeUnconstrainedVelocitiesTGS(PxReal dt, PxReal totalDt, PxReal invDt, PxReal invTotalDt, const PxVec3& gravity)
														{
															PX_UNUSED(invTotalDt);
															PX_UNUSED(totalDt);
															PX_UNUSED(dt);
															mArticulationData.setDt(totalDt);

															Cm::SpatialVectorF Z[DY_ARTICULATION_MAX_SIZE];
															Cm::SpatialVectorF deltaV[DY_ARTICULATION_MAX_SIZE];

															FeatherstoneArticulation::computeUnconstrainedVelocitiesInternal(gravity, Z, deltaV);

															setupInternalConstraints(mArticulationData.getLinks(), mArticulationData.getLinkCount(), 
																mArticulationData.getArticulationFlags() & PxArticulationFlag::eFIX_BASE, mArticulationData, Z, dt, totalDt, invDt, 0.7f, true);
														}

		PX_FORCE_INLINE	void							immComputeUnconstrainedVelocities(PxReal dt, const PxVec3& gravity)
		{
														mArticulationData.setDt(dt);

														Cm::SpatialVectorF Z[DY_ARTICULATION_MAX_SIZE];
														Cm::SpatialVectorF deltaV[DY_ARTICULATION_MAX_SIZE];

														FeatherstoneArticulation::computeUnconstrainedVelocitiesInternal(gravity, Z, deltaV);
														const PxReal invDt = 1.f/dt;
														setupInternalConstraints(mArticulationData.getLinks(), mArticulationData.getLinkCount(),
															mArticulationData.getArticulationFlags() & PxArticulationFlag::eFIX_BASE, mArticulationData, Z, dt, dt, invDt, 1.f, false);
		}

						PxU32							addLink(const PxFeatherstoneArticulationLinkData& data);
			
						void							complete();

						// PT: TODO: why do we have a Ps::Array here if we're limited to DY_ARTICULATION_MAX_SIZE?
						Ps::Array<Dy::ArticulationLink>	mLinks;
						// No dynamic array because we want to store the pointer to these items in ArticulationLink, and we don't want to deal with remapping
						PxsBodyCore						mBodyCores[DY_ARTICULATION_MAX_SIZE];
						Dy::ArticulationJointCore		mArticulationJointCore[DY_ARTICULATION_MAX_SIZE];
						PxArticulationFlags				mFlags;	// PT: PX-1399. Stored in Dy::ArticulationCore for retained mode.

						bool							mImmDirty;
		private:
						void							initJointCore(Dy::ArticulationJointCore& core, const PxFeatherstoneArticulationLinkData& data);
	};

#define MAX_NUM_PARTITIONS 32u

	class RigidBodyClassification
	{
		PX_NOCOPY(RigidBodyClassification)
		PxU8* PX_RESTRICT mBodies;
		const PxU32 mBodySize;
		const PxU32 mBodyStride;
		const PxU32 mBodyCount;

	public:
		RigidBodyClassification(PxU8* PX_RESTRICT bodies, PxU32 bodyCount, PxU32 bodyStride) : mBodies(bodies), mBodySize(bodyCount*bodyStride), mBodyStride(bodyStride), mBodyCount(bodyCount)
		{
		}

		//Returns true if it is a dynamic-dynamic constraint; false if it is a dynamic-static or dynamic-kinematic constraint
		PX_FORCE_INLINE bool classifyConstraint(const PxSolverConstraintDesc& desc, uintptr_t& indexA, uintptr_t& indexB,
			bool& activeA, bool& activeB, PxU32& bodyAProgress, PxU32& bodyBProgress) const
		{
			indexA = uintptr_t(reinterpret_cast<PxU8*>(desc.bodyA) - mBodies) / mBodyStride;
			indexB = uintptr_t(reinterpret_cast<PxU8*>(desc.bodyB) - mBodies) / mBodyStride;
			activeA = indexA < mBodyCount;
			activeB = indexB < mBodyCount;
			bodyAProgress = desc.bodyA->solverProgress;
			bodyBProgress = desc.bodyB->solverProgress;
			return activeA && activeB;
		}

		PX_FORCE_INLINE void reserveSpaceForStaticConstraints(PxU32* numConstraintsPerPartition)
		{
			for (PxU32 a = 0; a < mBodySize; a += mBodyStride)
			{
				PxSolverBody& body = *reinterpret_cast<PxSolverBody*>(mBodies + a);
				body.solverProgress = 0;

				for (PxU32 b = 0; b < body.maxSolverFrictionProgress; ++b)
				{
					PxU32 partId = PxMin(PxU32(body.maxSolverNormalProgress + b), MAX_NUM_PARTITIONS);
					numConstraintsPerPartition[partId]++;
				}
			}
		}

		PX_FORCE_INLINE void storeProgress(const PxSolverConstraintDesc& desc, const PxU32 bodyAProgress, const PxU32 bodyBProgress,
			const PxU16 availablePartition)
		{
			desc.bodyA->solverProgress = bodyAProgress;
			desc.bodyA->maxSolverNormalProgress = PxMax(desc.bodyA->maxSolverNormalProgress, availablePartition);
			desc.bodyB->solverProgress = bodyBProgress;
			desc.bodyB->maxSolverNormalProgress = PxMax(desc.bodyB->maxSolverNormalProgress, availablePartition);
		}


		PX_FORCE_INLINE PxU32 getStaticContactWriteIndex(const PxSolverConstraintDesc& desc,
			bool activeA, bool activeB)
		{
			if (activeA)
				return PxU32(desc.bodyA->maxSolverNormalProgress + desc.bodyA->maxSolverFrictionProgress++);
			else if (activeB)
				return PxU32(desc.bodyB->maxSolverNormalProgress + desc.bodyB->maxSolverFrictionProgress++);

			return 0xffffffff;

		}

		PX_FORCE_INLINE void recordStaticConstraint(const PxSolverConstraintDesc& desc, bool& activeA, bool& activeB) const
		{
			if (activeA)
			{
				desc.bodyA->maxSolverFrictionProgress++;
			}

			if (activeB)
			{
				desc.bodyB->maxSolverFrictionProgress++;
			}
		}

		PX_FORCE_INLINE void zeroBodies()
		{
			for (PxU32 a = 0; a < mBodySize; a += mBodyStride)
			{
				PxSolverBody& body = *reinterpret_cast<PxSolverBody*>(mBodies + a);
				body.solverProgress = 0;
				body.maxSolverFrictionProgress = 0;
				body.maxSolverNormalProgress = 0;
			}
		}

		PX_FORCE_INLINE void afterClassification()
		{
			for (PxU32 a = 0; a < mBodySize; a += mBodyStride)
			{
				PxSolverBody& body = *reinterpret_cast<PxSolverBody*>(mBodies + a);
				body.solverProgress = 0;
				//Keep the dynamic constraint count but bump the static constraint count back to 0.
				//This allows us to place the static constraints in the appropriate place when we see them
				//because we know the maximum index for the dynamic constraints...
				body.maxSolverFrictionProgress = 0;
			}
		}
	};

	class ExtendedRigidBodyClassification
	{
		PX_NOCOPY(ExtendedRigidBodyClassification)

		PxU8* PX_RESTRICT mBodies;
		const PxU32 mBodyCount;
		const PxU32 mBodySize;
		const PxU32 mStride;
		Dy::ArticulationV** PX_RESTRICT mArticulations;
		const PxU32 mNumArticulations;

	public:

		ExtendedRigidBodyClassification(PxU8* PX_RESTRICT bodies, PxU32 numBodies, PxU32 stride, Dy::ArticulationV** articulations, PxU32 numArticulations)
			: mBodies(bodies), mBodyCount(numBodies), mBodySize(numBodies*stride), mStride(stride), mArticulations(articulations), mNumArticulations(numArticulations)
		{
		}

		//Returns true if it is a dynamic-dynamic constriant; false if it is a dynamic-static or dynamic-kinematic constraint
		PX_FORCE_INLINE bool classifyConstraint(const PxSolverConstraintDesc& desc, uintptr_t& indexA, uintptr_t& indexB,
			bool& activeA, bool& activeB, PxU32& bodyAProgress, PxU32& bodyBProgress) const
		{
			bool hasStatic = false;
			if (PxSolverConstraintDesc::NO_LINK == desc.linkIndexA)
			{
				indexA = uintptr_t(reinterpret_cast<PxU8*>(desc.bodyA) - mBodies) / mStride;
				activeA = indexA < mBodyCount;
				hasStatic = !activeA;//desc.bodyADataIndex == 0;
				bodyAProgress = activeA ? desc.bodyA->solverProgress : 0;
			}
			else
			{

				Dy::ArticulationV* articulationA = desc.articulationA;
				indexA = mBodyCount;
				//bodyAProgress = articulationA->getFsDataPtr()->solverProgress;
				bodyAProgress = articulationA->solverProgress;
				activeA = true;
			}
			if (PxSolverConstraintDesc::NO_LINK == desc.linkIndexB)
			{
				indexB = uintptr_t(reinterpret_cast<PxU8*>(desc.bodyB) - mBodies) / mStride;
				activeB = indexB < mBodyCount;
				hasStatic = hasStatic || !activeB;
				bodyBProgress = activeB ? desc.bodyB->solverProgress : 0;
			}
			else
			{
				Dy::ArticulationV* articulationB = desc.articulationB;
				indexB = mBodyCount;
				activeB = true;
				bodyBProgress = articulationB->solverProgress;
			}
			return !hasStatic;
		}

		PX_FORCE_INLINE void recordStaticConstraint(const PxSolverConstraintDesc& desc, bool& activeA, bool& activeB)
		{
			if (activeA)
			{
				if (PxSolverConstraintDesc::NO_LINK == desc.linkIndexA)
					desc.bodyA->maxSolverFrictionProgress++;
				else
				{
					Dy::ArticulationV* articulationA = desc.articulationA;
					articulationA->maxSolverFrictionProgress++;
				}
			}

			if (activeB)
			{
				if (PxSolverConstraintDesc::NO_LINK == desc.linkIndexB)
					desc.bodyB->maxSolverFrictionProgress++;
				else
				{
					Dy::ArticulationV* articulationB = desc.articulationB;
					articulationB->maxSolverFrictionProgress++;
				}
			}
		}

		PX_FORCE_INLINE PxU32 getStaticContactWriteIndex(const PxSolverConstraintDesc& desc,
			bool activeA, bool activeB)
		{
			if (activeA)
			{
				if (PxSolverConstraintDesc::NO_LINK == desc.linkIndexA)
				{
					return PxU32(desc.bodyA->maxSolverNormalProgress + desc.bodyA->maxSolverFrictionProgress++);
				}
				else
				{
					Dy::ArticulationV* articulationA = desc.articulationA;
					return PxU32(articulationA->maxSolverNormalProgress + articulationA->maxSolverFrictionProgress++);
				}
			}
			else if (activeB)
			{
				if (PxSolverConstraintDesc::NO_LINK == desc.linkIndexB)
				{
					return PxU32(desc.bodyB->maxSolverNormalProgress + desc.bodyB->maxSolverFrictionProgress++);
				}
				else
				{
					Dy::ArticulationV* articulationB = desc.articulationB;
					return PxU32(articulationB->maxSolverNormalProgress + articulationB->maxSolverFrictionProgress++);
				}
			}

			return 0xffffffff;
		}

		PX_FORCE_INLINE void storeProgress(const PxSolverConstraintDesc& desc, const PxU32 bodyAProgress, const PxU32 bodyBProgress,
			const PxU16 availablePartition)
		{
			if (PxSolverConstraintDesc::NO_LINK == desc.linkIndexA)
			{
				desc.bodyA->solverProgress = bodyAProgress;
				desc.bodyA->maxSolverNormalProgress = PxMax(desc.bodyA->maxSolverNormalProgress, availablePartition);
			}
			else
			{
				Dy::ArticulationV* articulationA = desc.articulationA;
				articulationA->solverProgress = bodyAProgress;
				articulationA->maxSolverNormalProgress = PxMax(articulationA->maxSolverNormalProgress, availablePartition);
			}

			if (PxSolverConstraintDesc::NO_LINK == desc.linkIndexB)
			{
				desc.bodyB->solverProgress = bodyBProgress;
				desc.bodyB->maxSolverNormalProgress = PxMax(desc.bodyB->maxSolverNormalProgress, availablePartition);
			}
			else
			{
				Dy::ArticulationV* articulationB = desc.articulationB;
				articulationB->solverProgress = bodyBProgress;
				articulationB->maxSolverNormalProgress = PxMax(articulationB->maxSolverNormalProgress, availablePartition);
			}
		}

		PX_FORCE_INLINE void reserveSpaceForStaticConstraints(PxU32* numConstraintsPerPartition)
		{
			for (PxU32 a = 0; a < mBodySize; a += mStride)
			{
				PxSolverBody& body = *reinterpret_cast<PxSolverBody*>(mBodies + a);
				body.solverProgress = 0;

				for (PxU32 b = 0; b < body.maxSolverFrictionProgress; ++b)
				{
					PxU32 partId = PxMin(PxU32(body.maxSolverNormalProgress + b), MAX_NUM_PARTITIONS);
					numConstraintsPerPartition[partId]++;
				}
			}

			for (PxU32 a = 0; a < mNumArticulations; ++a)
			{
				Dy::ArticulationV* articulation = mArticulations[a];
				articulation->solverProgress = 0;

				for (PxU32 b = 0; b < articulation->maxSolverFrictionProgress; ++b)
				{
					PxU32 partId = PxMin(PxU32(articulation->maxSolverNormalProgress + b), MAX_NUM_PARTITIONS);
					numConstraintsPerPartition[partId]++;
				}
			}
		}

		PX_FORCE_INLINE void zeroBodies()
		{
			for (PxU32 a = 0; a < mBodySize; a += mStride)
			{
				PxSolverBody& body = *reinterpret_cast<PxSolverBody*>(mBodies + a);
				body.solverProgress = 0;
				body.maxSolverFrictionProgress = 0;
				body.maxSolverNormalProgress = 0;
			}

			for (PxU32 a = 0; a < mNumArticulations; ++a)
			{
				Dy::ArticulationV* articulation = mArticulations[a];
				articulation->solverProgress = 0;
				articulation->maxSolverFrictionProgress = 0;
				articulation->maxSolverNormalProgress = 0;
			}
		}

		PX_FORCE_INLINE void afterClassification()
		{
			for (PxU32 a = 0; a < mBodySize; a += mStride)
			{
				PxSolverBody& body = *reinterpret_cast<PxSolverBody*>(mBodies + a);
				body.solverProgress = 0;
				//Keep the dynamic constraint count but bump the static constraint count back to 0.
				//This allows us to place the static constraints in the appropriate place when we see them
				//because we know the maximum index for the dynamic constraints...
				body.maxSolverFrictionProgress = 0;
			}

			for (PxU32 a = 0; a < mNumArticulations; ++a)
			{
				Dy::ArticulationV* articulation = mArticulations[a];
				articulation->solverProgress = 0;
				articulation->maxSolverFrictionProgress = 0;
			}
		}
	};

	template <typename Classification>
	void classifyConstraintDesc(const PxSolverConstraintDesc* PX_RESTRICT descs, const PxU32 numConstraints, Classification& classification,
		PxU32* numConstraintsPerPartition)
	{
		const PxSolverConstraintDesc* _desc = descs;
		const PxU32 numConstraintsMin1 = numConstraints - 1;

		PxMemZero(numConstraintsPerPartition, sizeof(PxU32) * (MAX_NUM_PARTITIONS+1));

		for (PxU32 i = 0; i < numConstraints; ++i, _desc++)
		{
			const PxU32 prefetchOffset = PxMin(numConstraintsMin1 - i, 4u);
			Ps::prefetchLine(_desc[prefetchOffset].constraint);
			Ps::prefetchLine(_desc[prefetchOffset].bodyA);
			Ps::prefetchLine(_desc[prefetchOffset].bodyB);
			Ps::prefetchLine(_desc + 8);

			uintptr_t indexA, indexB;
			bool activeA, activeB;

			PxU32 partitionsA, partitionsB;
			const bool notContainsStatic = classification.classifyConstraint(*_desc, indexA, indexB, activeA, activeB,
				partitionsA, partitionsB);

			if (notContainsStatic)
			{
				PxU32 availablePartition;
				{
					const PxU32 combinedMask = (~partitionsA & ~partitionsB);
					availablePartition = combinedMask == 0 ? MAX_NUM_PARTITIONS : Ps::lowestSetBit(combinedMask);
					if (availablePartition == MAX_NUM_PARTITIONS)
					{
						//Write to overflow partition...
						numConstraintsPerPartition[availablePartition]++;
						classification.storeProgress(*_desc, partitionsA, partitionsB, PxU16(MAX_NUM_PARTITIONS));
						continue;
					}

					const PxU32 partitionBit = (1u << availablePartition);
					partitionsA |= partitionBit;
					partitionsB |= partitionBit;
				}

				numConstraintsPerPartition[availablePartition]++;
				availablePartition++;
				classification.storeProgress(*_desc, partitionsA, partitionsB, PxU16(availablePartition));
			}
			else
			{
				//Just count the number of static constraints and store in maxSolverFrictionProgress...
				classification.recordStaticConstraint(*_desc, activeA, activeB);
			}
		}

		classification.reserveSpaceForStaticConstraints(numConstraintsPerPartition);
	}

	template <typename Classification>
	void writeConstraintDesc(const PxSolverConstraintDesc* PX_RESTRICT descs, const PxU32 numConstraints, Classification& classification,
		PxU32* accumulatedConstraintsPerPartition, PxSolverConstraintDesc* PX_RESTRICT eaOrderedConstraintDesc)
	{
		const PxSolverConstraintDesc* _desc = descs;
		const PxU32 numConstraintsMin1 = numConstraints - 1;
		for (PxU32 i = 0; i < numConstraints; ++i, _desc++)
		{
			const PxU32 prefetchOffset = PxMin(numConstraintsMin1 - i, 4u);
			Ps::prefetchLine(_desc[prefetchOffset].constraint);
			Ps::prefetchLine(_desc[prefetchOffset].bodyA);
			Ps::prefetchLine(_desc[prefetchOffset].bodyB);
			Ps::prefetchLine(_desc + 8);

			uintptr_t indexA, indexB;
			bool activeA, activeB;
			PxU32 partitionsA, partitionsB;
			const bool notContainsStatic = classification.classifyConstraint(*_desc, indexA, indexB, activeA, activeB,
				partitionsA, partitionsB);

			if (notContainsStatic)
			{
				PxU32 availablePartition;
				{
					const PxU32 combinedMask = (~partitionsA & ~partitionsB);
					availablePartition = combinedMask == 0 ? MAX_NUM_PARTITIONS : Ps::lowestSetBit(combinedMask);
					if (availablePartition == MAX_NUM_PARTITIONS)
					{
						eaOrderedConstraintDesc[accumulatedConstraintsPerPartition[availablePartition]++] = *_desc;
						continue;
					}

					const PxU32 partitionBit = (1u << availablePartition);

					partitionsA |= partitionBit;
					partitionsB |= partitionBit;
				}

				classification.storeProgress(*_desc, partitionsA, partitionsB, PxU16(availablePartition+1));

				eaOrderedConstraintDesc[accumulatedConstraintsPerPartition[availablePartition]++] = *_desc;
			}
			else
			{
				//Just count the number of static constraints and store in maxSolverFrictionProgress...
				//KS - TODO - handle registration of static contacts with articulations here!
				PxU32 index = classification.getStaticContactWriteIndex(*_desc, activeA, activeB);
				eaOrderedConstraintDesc[accumulatedConstraintsPerPartition[index]++] = *_desc;
			}
		}
	}

	PX_FORCE_INLINE bool isArticulationConstraint(const PxSolverConstraintDesc& desc)
	{
		return desc.linkIndexA != PxSolverConstraintDesc::NO_LINK || desc.linkIndexB != PxSolverConstraintDesc::NO_LINK;
	}

	template <typename Classification>
	PxU32 BatchConstraints(const PxSolverConstraintDesc* solverConstraintDescs, const PxU32 nbConstraints, PxConstraintBatchHeader* outBatchHeaders, 
		PxSolverConstraintDesc* outOrderedConstraintDescs, Classification& classification)
	{
		PxU32 constraintsPerPartition[MAX_NUM_PARTITIONS + 1];

		classification.zeroBodies();

		classifyConstraintDesc(solverConstraintDescs, nbConstraints, classification, constraintsPerPartition);

		PxU32 accumulation = 0;
		for (PxU32 a = 0; a < MAX_NUM_PARTITIONS + 1; ++a)
		{
			PxU32 count = constraintsPerPartition[a];
			constraintsPerPartition[a] = accumulation;
			accumulation += count;
		}

		classification.afterClassification();

		writeConstraintDesc(solverConstraintDescs, nbConstraints, classification, constraintsPerPartition, outOrderedConstraintDescs);

		PxU32 numHeaders = 0;
		PxU32 currentPartition = 0;
		PxU32 maxJ = nbConstraints == 0 ? 0 : constraintsPerPartition[0];

		const PxU32 maxBatchPartition = MAX_NUM_PARTITIONS;
		for (PxU32 a = 0; a < nbConstraints;)
		{
			PxConstraintBatchHeader& header = outBatchHeaders[numHeaders++];
			header.startIndex = a;

			PxU32 loopMax = PxMin(maxJ - a, 4u);
			PxU16 j = 0;
			if (loopMax > 0)
			{
				j = 1;
				PxSolverConstraintDesc& desc = outOrderedConstraintDescs[a];

				if(isArticulationConstraint(desc))
					loopMax = 1;

				if (currentPartition < maxBatchPartition)
				{
					for (; j < loopMax && desc.constraintLengthOver16 == outOrderedConstraintDescs[a + j].constraintLengthOver16
						&& !isArticulationConstraint(outOrderedConstraintDescs[a + j]); ++j);
				}
				header.stride = j;
				header.constraintType = desc.constraintLengthOver16;
			}
			if (maxJ == (a + j) && maxJ != nbConstraints)
			{
				currentPartition++;
				maxJ = constraintsPerPartition[currentPartition];
			}
			a += j;
		}
		return numHeaders;
	}
}

PxU32 immediate::PxBatchConstraints(const PxSolverConstraintDesc* solverConstraintDescs, const PxU32 nbConstraints, PxSolverBody* solverBodies, const PxU32 nbBodies,
									PxConstraintBatchHeader* outBatchHeaders, PxSolverConstraintDesc* outOrderedConstraintDescs,
									Dy::ArticulationV** articulations, const PxU32 nbArticulations)
{
	PX_ASSERT((reinterpret_cast<size_t>(solverBodies) & 0xf) == 0);

	if(!nbArticulations)
	{
		RigidBodyClassification classification(reinterpret_cast<PxU8*>(solverBodies), nbBodies, sizeof(PxSolverBody));
		return BatchConstraints(solverConstraintDescs, nbConstraints, outBatchHeaders, outOrderedConstraintDescs, classification);
	}
	else
	{
		ExtendedRigidBodyClassification classification(reinterpret_cast<PxU8*>(solverBodies), nbBodies, sizeof(PxSolverBody), articulations, nbArticulations);
		return BatchConstraints(solverConstraintDescs, nbConstraints, outBatchHeaders, outOrderedConstraintDescs, classification);
	}
}

bool immediate::PxCreateContactConstraints(PxConstraintBatchHeader* batchHeaders, const PxU32 nbHeaders, PxSolverContactDesc* contactDescs,
	PxConstraintAllocator& allocator, const PxReal invDt, const PxReal bounceThreshold, const PxReal frictionOffsetThreshold, const PxReal correlationDistance)
{
	PX_ASSERT(invDt > 0.f && PxIsFinite(invDt));
	PX_ASSERT(bounceThreshold < 0.f);
	PX_ASSERT(frictionOffsetThreshold > 0.f);
	PX_ASSERT(correlationDistance > 0.f);

	Dy::SolverConstraintPrepState::Enum state = Dy::SolverConstraintPrepState::eUNBATCHABLE;

	Dy::CorrelationBuffer cb;

	PxU32 currentContactDescIdx = 0;

	for (PxU32 i = 0; i < nbHeaders; ++i)
	{
		PxConstraintBatchHeader& batchHeader = batchHeaders[i];
		if (batchHeader.stride == 4)
		{
			PxU32 totalContacts = contactDescs[currentContactDescIdx].numContacts + contactDescs[currentContactDescIdx + 1].numContacts + 
				contactDescs[currentContactDescIdx + 2].numContacts + contactDescs[currentContactDescIdx + 3].numContacts;

			if (totalContacts <= 64)
			{
				state = Dy::createFinalizeSolverContacts4(cb,
					contactDescs + currentContactDescIdx,
					invDt,
					bounceThreshold,
					frictionOffsetThreshold,
					correlationDistance,
					0.f,
					allocator);
			}
		}

		if (state == Dy::SolverConstraintPrepState::eUNBATCHABLE)
		{
			Cm::SpatialVectorF Z[Dy::DY_ARTICULATION_MAX_SIZE];
			for (PxU32 a = 0; a < batchHeader.stride; ++a)
			{
				Dy::createFinalizeSolverContacts(contactDescs[currentContactDescIdx + a], cb, invDt, bounceThreshold, 
					frictionOffsetThreshold, correlationDistance, 0.f, allocator, Z);
			}
		}
		PxU8 type = *contactDescs[currentContactDescIdx].desc->constraint;

		if (type == DY_SC_TYPE_STATIC_CONTACT)
		{
			//Check if any block of constraints is classified as type static (single) contact constraint.
			//If they are, iterate over all constraints grouped with it and switch to "dynamic" contact constraint
			//type if there's a dynamic contact constraint in the group.
			for (PxU32 c = 1; c < batchHeader.stride; ++c)
			{
				if (*contactDescs[currentContactDescIdx + c].desc->constraint == DY_SC_TYPE_RB_CONTACT)
				{
					type = DY_SC_TYPE_RB_CONTACT;
					break;
				}
			}
		}

		batchHeader.constraintType = type;

		currentContactDescIdx += batchHeader.stride;
	}
	return true;
}

bool immediate::PxCreateJointConstraints(PxConstraintBatchHeader* batchHeaders, const PxU32 nbHeaders, PxSolverConstraintPrepDesc* jointDescs, PxConstraintAllocator& allocator, const PxReal dt, const PxReal invDt)
{
	PX_ASSERT(dt > 0.f);
	PX_ASSERT(invDt > 0.f && PxIsFinite(invDt));

	Dy::SolverConstraintPrepState::Enum state = Dy::SolverConstraintPrepState::eUNBATCHABLE; 
		
	PxU32 currentDescIdx = 0;
	for (PxU32 i = 0; i < nbHeaders; ++i)
	{
		PxConstraintBatchHeader& batchHeader = batchHeaders[i];

		PxU8 type = DY_SC_TYPE_BLOCK_1D;
		if (batchHeader.stride == 4)
		{
			PxU32 totalRows = 0;
			PxU32 maxRows = 0;
			bool batchable = true;
			for (PxU32 a = 0; a < batchHeader.stride; ++a)
			{
				if (jointDescs[currentDescIdx + a].numRows == 0)
				{
					batchable = false;
					break;
				}
				totalRows += jointDescs[currentDescIdx + a].numRows;
				maxRows = PxMax(maxRows, jointDescs[currentDescIdx + a].numRows);
			}

			if (batchable)
			{
				state = Dy::setupSolverConstraint4
					(jointDescs + currentDescIdx,
					dt, invDt, totalRows,
					allocator, maxRows);
			}
		}

		if (state == Dy::SolverConstraintPrepState::eUNBATCHABLE)
		{
			type = DY_SC_TYPE_RB_1D;
			Cm::SpatialVectorF Z[Dy::DY_ARTICULATION_MAX_SIZE];
			for (PxU32 a = 0; a < batchHeader.stride; ++a)
			{
				// PT: TODO: And "isExtended" is already computed in Dy::ConstraintHelper::setupSolverConstraint
				PxSolverConstraintDesc& desc = *jointDescs[currentDescIdx + a].desc;
				const bool isExtended = desc.linkIndexA != PxSolverConstraintDesc::NO_LINK || desc.linkIndexB != PxSolverConstraintDesc::NO_LINK;
				if(isExtended)
					type = DY_SC_TYPE_EXT_1D;

				Dy::ConstraintHelper::setupSolverConstraint(jointDescs[currentDescIdx + a], allocator, dt, invDt, Z);
			}
		}

		batchHeader.constraintType = type;
		currentDescIdx += batchHeader.stride;
	}

	return true;
}

template<class LeafTestT, class ParamsT>
static bool PxCreateJointConstraintsWithShadersT(PxConstraintBatchHeader* batchHeaders, const PxU32 nbHeaders, ParamsT* params, PxSolverConstraintPrepDesc* jointDescs,
	PxConstraintAllocator& allocator, const PxReal dt, const PxReal invDt)
{
	Px1DConstraint allRows[Dy::MAX_CONSTRAINT_ROWS * 4];

	//Runs shaders to fill in rows...

	PxU32 currentDescIdx = 0;

	for(PxU32 i=0; i<nbHeaders; i++)
	{
		PxU32 numRows = 0;
		PxU32 preppedIndex = 0;
		PxU32 maxRows = 0;

		PxConstraintBatchHeader& batchHeader = batchHeaders[i];

		for(PxU32 a=0; a<batchHeader.stride; a++)
		{
			Px1DConstraint* rows = allRows + numRows;
			PxSolverConstraintPrepDesc& desc = jointDescs[currentDescIdx + a];

			PxConstraintSolverPrep prep;
			const void* constantBlock;
			const bool useExtendedLimits = LeafTestT::getData(params, currentDescIdx + a, &prep, &constantBlock);
			PX_ASSERT(prep);

			PxMemZero(rows + preppedIndex, sizeof(Px1DConstraint)*(Dy::MAX_CONSTRAINT_ROWS));
			for (PxU32 b = preppedIndex; b < Dy::MAX_CONSTRAINT_ROWS; ++b)
			{
				Px1DConstraint& c = rows[b];
				//Px1DConstraintInit(c);
				c.minImpulse = -PX_MAX_REAL;
				c.maxImpulse = PX_MAX_REAL;
			}

			desc.invMassScales.linear0 = desc.invMassScales.linear1 = desc.invMassScales.angular0 = desc.invMassScales.angular1 = 1.0f;

			desc.body0WorldOffset = PxVec3(0.0f);

			PxVec3 unused_cA2w, unused_cB2w;
			const PxU32 constraintCount = prep(rows,
				desc.body0WorldOffset,
				Dy::MAX_CONSTRAINT_ROWS,
				desc.invMassScales,
				constantBlock,
				desc.bodyFrame0, desc.bodyFrame1, 
				useExtendedLimits,
				unused_cA2w, unused_cB2w);

			preppedIndex = Dy::MAX_CONSTRAINT_ROWS - constraintCount;

			maxRows = PxMax(constraintCount, maxRows);

			desc.rows = rows;
			desc.numRows = constraintCount;
			numRows += constraintCount;
		}

		PxCreateJointConstraints(&batchHeader, 1, jointDescs + currentDescIdx, allocator, dt, invDt);

		currentDescIdx += batchHeader.stride;
	}
	return true; //KS - TODO - do some error reporting/management...
}

bool immediate::PxCreateJointConstraintsWithShaders(PxConstraintBatchHeader* batchHeaders, const PxU32 nbHeaders, PxConstraint** constraints, PxSolverConstraintPrepDesc* jointDescs,
	PxConstraintAllocator& allocator, const PxReal dt, const PxReal invDt)
{
	class PxConstraintAdapter
	{
	public:
		static PX_FORCE_INLINE bool getData(PxConstraint** constraints, PxU32 i, PxConstraintSolverPrep* prep, const void** constantBlock)
		{
			NpConstraint* npConstraint = static_cast<NpConstraint*>(constraints[i]);
			Sc::ConstraintCore& core = npConstraint->getScbConstraint().getScConstraint();

			if (npConstraint->isDirty())
			{
				core.getPxConnector()->prepareData();
				npConstraint->markClean();
			}

			*prep = core.getPxConnector()->getPrep();
			*constantBlock = core.getPxConnector()->getConstantBlock();
			return core.getFlags() & PxConstraintFlag::eENABLE_EXTENDED_LIMITS;
		}
	};

	return PxCreateJointConstraintsWithShadersT<PxConstraintAdapter>(batchHeaders, nbHeaders, constraints, jointDescs, allocator, dt, invDt);
}

bool immediate::PxCreateJointConstraintsWithImmediateShaders(PxConstraintBatchHeader* batchHeaders, const PxU32 nbHeaders, PxImmediateConstraint* constraints, PxSolverConstraintPrepDesc* jointDescs,
	PxConstraintAllocator& allocator, const PxReal dt, const PxReal invDt)
{
	class immConstraintAdapter
	{
	public:
		static PX_FORCE_INLINE bool getData(PxImmediateConstraint* constraints, PxU32 i, PxConstraintSolverPrep* prep, const void** constantBlock)
		{
			const PxImmediateConstraint& ic = constraints[i];
			*prep = ic.prep;
			*constantBlock = ic.constantBlock;
			return false;
		}
	};

	return PxCreateJointConstraintsWithShadersT<immConstraintAdapter>(batchHeaders, nbHeaders, constraints, jointDescs, allocator, dt, invDt);
}

/*static*/ PX_FORCE_INLINE bool PxIsZero(const PxSolverBody* bodies, PxU32 nbBodies)
{
	for(PxU32 i=0; i<nbBodies; i++)
	{
		if(	!bodies[i].linearVelocity.isZero() ||
			!bodies[i].angularState.isZero())
			return false;
	}
	return true;
}

void immediate::PxSolveConstraints(const PxConstraintBatchHeader* batchHeaders, const PxU32 nbBatchHeaders, const PxSolverConstraintDesc* solverConstraintDescs,
	const PxSolverBody* solverBodies, PxVec3* linearMotionVelocity, PxVec3* angularMotionVelocity, const PxU32 nbSolverBodies, const PxU32 nbPositionIterations, const PxU32 nbVelocityIterations,
	const float dt, const float invDt, const PxU32 nbSolverArticulations, Dy::ArticulationV** solverArticulations)
{
	PX_ASSERT(nbPositionIterations > 0);
	PX_ASSERT(nbVelocityIterations > 0);
	PX_ASSERT(PxIsZero(solverBodies, nbSolverBodies)); //Ensure that solver body velocities have been zeroed before solving
	PX_ASSERT((reinterpret_cast<size_t>(solverBodies) & 0xf) == 0);

	//Stage 1: solve the position iterations...
	Dy::SolveBlockMethod* solveTable = Dy::getSolveBlockTable();

	Dy::SolveBlockMethod* solveConcludeTable = Dy::getSolverConcludeBlockTable();

	Dy::SolveWriteBackBlockMethod* solveWritebackTable = Dy::getSolveWritebackBlockTable();

	Dy::SolverContext cache;
	cache.mThresholdStreamIndex = 0;
	cache.mThresholdStreamLength = 0xFFFFFFF;
		
	PX_ASSERT(nbPositionIterations > 0);
	PX_ASSERT(nbVelocityIterations > 0);

	Cm::SpatialVectorF Z[DY_ARTICULATION_MAX_SIZE];
	Cm::SpatialVectorF deltaV[DY_ARTICULATION_MAX_SIZE];

	cache.Z = Z;
	cache.deltaV = deltaV;

	struct Articulations
	{
		static PX_FORCE_INLINE void solveInternalConstraints(const float dt, const float invDt, const PxU32 nbSolverArticulations, Dy::ArticulationV** solverArticulations, Cm::SpatialVectorF* Z, Cm::SpatialVectorF* deltaV, bool velIter)
		{
			for(PxU32 a=0;a<nbSolverArticulations;a++)
			{
				immArticulation* immArt = static_cast<immArticulation*>(solverArticulations[a]);
				immArt->immSolveInternalConstraints(dt, invDt, Z, deltaV, 0.f, velIter, false);
			}
		}
	};

	for(PxU32 i=nbPositionIterations; i>1; --i)
	{
		cache.doFriction = i <= 3;
		for(PxU32 a=0; a<nbBatchHeaders; ++a)
		{
			const PxConstraintBatchHeader& batch = batchHeaders[a];
			solveTable[batch.constraintType](solverConstraintDescs + batch.startIndex, batch.stride, cache);
		}

		Articulations::solveInternalConstraints(dt, invDt, nbSolverArticulations, solverArticulations, Z, deltaV, false);
	}

	cache.doFriction = true;
	for(PxU32 a=0; a<nbBatchHeaders; ++a)
	{
		const PxConstraintBatchHeader& batch = batchHeaders[a];
		solveConcludeTable[batch.constraintType](solverConstraintDescs + batch.startIndex, batch.stride, cache);
	}
	Articulations::solveInternalConstraints(dt, invDt, nbSolverArticulations, solverArticulations, Z, deltaV, false);

	//Save motion velocities...

	for(PxU32 a=0; a<nbSolverBodies; a++)
	{
		linearMotionVelocity[a] = solverBodies[a].linearVelocity;
		angularMotionVelocity[a] = solverBodies[a].angularState;
	}

	for(PxU32 a=0;a<nbSolverArticulations;a++)
	{
		// PT: TODO: drop unusedDesc
		ArticulationSolverDesc unusedDesc;
		unusedDesc.articulation = solverArticulations[a];
		FeatherstoneArticulation::saveVelocity(unusedDesc, deltaV);
	}

	for(PxU32 i=nbVelocityIterations; i>1; --i)
	{
		for(PxU32 a=0; a<nbBatchHeaders; ++a)
		{
			const PxConstraintBatchHeader& batch = batchHeaders[a];
			solveTable[batch.constraintType](solverConstraintDescs + batch.startIndex, batch.stride, cache);
		}

		Articulations::solveInternalConstraints(dt, invDt, nbSolverArticulations, solverArticulations, Z, deltaV, true);
	}

	for(PxU32 a=0; a<nbBatchHeaders; ++a)
	{
		const PxConstraintBatchHeader& batch = batchHeaders[a];
		solveWritebackTable[batch.constraintType](solverConstraintDescs + batch.startIndex, batch.stride, cache);
	}
	Articulations::solveInternalConstraints(dt, invDt, nbSolverArticulations, solverArticulations, Z, deltaV, true);
}

static void createCache(Gu::Cache& cache, PxGeometryType::Enum geomType0, PxGeometryType::Enum geomType1, PxCacheAllocator& allocator)
{	
	if (gEnablePCMCaching[geomType0][geomType1])
	{
		if (geomType0 <= PxGeometryType::eCONVEXMESH &&
			geomType1 <= PxGeometryType::eCONVEXMESH)
		{
			if (geomType0 == PxGeometryType::eSPHERE || geomType1 == PxGeometryType::eSPHERE)
			{
				Gu::PersistentContactManifold* manifold = PX_PLACEMENT_NEW(allocator.allocateCacheData(sizeof(Gu::SpherePersistentContactManifold)), Gu::SpherePersistentContactManifold)();
				cache.setManifold(manifold);
			}
			else
			{
				Gu::PersistentContactManifold* manifold = PX_PLACEMENT_NEW(allocator.allocateCacheData(sizeof(Gu::LargePersistentContactManifold)), Gu::LargePersistentContactManifold)();
				cache.setManifold(manifold);

			}
			cache.getManifold().clearManifold();
		}
		else
		{
			//ML: raised 1 to indicate the manifold is multiManifold which is for contact gen in mesh/height field
			//cache.manifold = 1;
			cache.setMultiManifold(NULL);
		}
	}
	else
	{
		//cache.manifold =  0;
		cache.mCachedData = NULL;
		cache.mManifoldFlags = 0;
	}
}

bool immediate::PxGenerateContacts(const PxGeometry* const * geom0, const PxGeometry* const * geom1, const PxTransform* pose0, const PxTransform* pose1, PxCache* contactCache, const PxU32 nbPairs, PxContactRecorder& contactRecorder,
	const PxReal contactDistance, const PxReal meshContactMargin, const PxReal toleranceLength, PxCacheAllocator& allocator)
{
	PX_ASSERT(meshContactMargin > 0.f);
	PX_ASSERT(toleranceLength > 0.f);
	PX_ASSERT(contactDistance > 0.f);
	Gu::ContactBuffer contactBuffer;

	for (PxU32 i = 0; i < nbPairs; ++i)
	{
		contactBuffer.count = 0;
		PxGeometryType::Enum type0 = geom0[i]->getType();
		PxGeometryType::Enum type1 = geom1[i]->getType();

		const PxGeometry* tempGeom0 = geom0[i];
		const PxGeometry* tempGeom1 = geom1[i];

		PX_ALIGN(16, PxTransform) transform0 = pose0[i];
		PX_ALIGN(16, PxTransform) transform1 = pose1[i];

		const bool bSwap = type0 > type1;
		if (bSwap)
		{
			const PxGeometry* temp = tempGeom0;
			tempGeom0 = geom1[i];
			tempGeom1 = temp;
			PxGeometryType::Enum tempType = type0;
			type0 = type1;
			type1 = tempType;
			transform1 = pose0[i];
			transform0 = pose1[i];
		}

		const Gu::GeometryUnion geomUnion0(*tempGeom0);
		const Gu::GeometryUnion geomUnion1(*tempGeom1);

		//Now work out which type of PCM we need...

		Gu::Cache& cache = static_cast<Gu::Cache&>(contactCache[i]);

		bool needsMultiManifold = type1 > PxGeometryType::eCONVEXMESH;

		Gu::NarrowPhaseParams params(contactDistance, meshContactMargin, toleranceLength);

		if (needsMultiManifold)
		{
			Gu::MultiplePersistentContactManifold multiManifold;

			if (cache.isMultiManifold())
			{
				multiManifold.fromBuffer(reinterpret_cast<PxU8*>(&cache.getMultipleManifold()));
			}
			else
			{
				multiManifold.initialize();
			}
			cache.setMultiManifold(&multiManifold);

			//Do collision detection, then write manifold out...
			g_PCMContactMethodTable[type0][type1](geomUnion0, geomUnion1, transform0, transform1, params, cache, contactBuffer, NULL);

			const PxU32 size = (sizeof(Gu::MultiPersistentManifoldHeader) +
				multiManifold.mNumManifolds * sizeof(Gu::SingleManifoldHeader) +
				multiManifold.mNumTotalContacts * sizeof(Gu::CachedMeshPersistentContact));

			PxU8* buffer = allocator.allocateCacheData(size);

			multiManifold.toBuffer(buffer);

			cache.setMultiManifold(buffer);
		}
		else
		{
			//Allocate the type of manifold we need again...
			Gu::PersistentContactManifold* oldManifold = NULL;

			if (cache.isManifold())
				oldManifold = &cache.getManifold();

			//Allocates and creates the PCM...
			createCache(cache, type0, type1, allocator);

			//Copy PCM from old to new manifold...
			if (oldManifold)
			{
				Gu::PersistentContactManifold& manifold = cache.getManifold();
				manifold.mRelativeTransform = oldManifold->mRelativeTransform;
				manifold.mNumContacts = oldManifold->mNumContacts;
				manifold.mNumWarmStartPoints = oldManifold->mNumWarmStartPoints;
				manifold.mAIndice[0] = oldManifold->mAIndice[0]; manifold.mAIndice[1] = oldManifold->mAIndice[1];
				manifold.mAIndice[2] = oldManifold->mAIndice[2]; manifold.mAIndice[3] = oldManifold->mAIndice[3];
				manifold.mBIndice[0] = oldManifold->mBIndice[0]; manifold.mBIndice[1] = oldManifold->mBIndice[1];
				manifold.mBIndice[2] = oldManifold->mBIndice[2]; manifold.mBIndice[3] = oldManifold->mBIndice[3];
				PxMemCopy(manifold.mContactPoints, oldManifold->mContactPoints, sizeof(Gu::PersistentContact)*manifold.mNumContacts);
			}

			g_PCMContactMethodTable[type0][type1](geomUnion0, geomUnion1, transform0, transform1, params, cache, contactBuffer, NULL);
		}

		if (bSwap)
		{
			for (PxU32 a = 0; a < contactBuffer.count; ++a)
			{
				contactBuffer.contacts[a].normal = -contactBuffer.contacts[a].normal;
			}
		}

		if (contactBuffer.count != 0)
		{
			//Record this contact pair...
			contactRecorder.recordContacts(contactBuffer.contacts, contactBuffer.count, i);
		}
	}
	return true;
}

immArticulation::immArticulation(const PxFeatherstoneArticulationData& data) :
	FeatherstoneArticulation(this),
	mFlags					(data.flags),
	mImmDirty				(true)
{
	// From Sc::ArticulationSim ctor, not sure why it's not done in FeatherstoneArticulation ctor
	setDirty(true);

	// PT: TODO: we only need the flags here, maybe drop the solver desc?
	getSolverDesc().initData(NULL, &mFlags);
}

immArticulation::~immArticulation()
{
}

void immArticulation::initJointCore(Dy::ArticulationJointCore& core, const PxFeatherstoneArticulationLinkData& data)
{
	// From Sc::ArticulationJointCore, this should once again just be done in a Dy::ArticulationJointCore ctor
	// =====> hmmm there is a ctor, but it initializes things to different values like PxArticulationJointType::eFIX....
	// TODO: replicate ctor here
	{
		// TODO: in this one I initialized all data but I don't actually know which one is needed

		core.parentPose	= data.inboundJoint.parentPose;
		core.childPose	= data.inboundJoint.childPose;
		PX_ASSERT(core.parentPose.isValid());
		PX_ASSERT(core.childPose.isValid());
		core.dirtyFlag |= Dy::ArticulationJointCoreDirtyFlag::ePOSE;

		// TODO: is this used in the new articulations?
		core.targetPosition = PxQuat(PxIdentity);
		core.targetVelocity = PxVec3(0.f);

		core.driveType = PxArticulationJointDriveType::eTARGET;

		core.spring = 0.0f;
		core.damping = 0.0f;

		core.internalCompliance = 1.0f;
		core.externalCompliance = 1.0f;

		const PxU32* binP = reinterpret_cast<const PxU32*>(data.inboundJoint.targetPos);
		const PxU32* binV = reinterpret_cast<const PxU32*>(data.inboundJoint.targetVel);

		for(PxU32 i=0; i<PxArticulationAxis::eCOUNT; i++)
		{
			core.limits[i].low = data.inboundJoint.limits[i].low;
			core.limits[i].high = data.inboundJoint.limits[i].high;
			core.drives[i].stiffness = data.inboundJoint.drives[i].stiffness;
			core.drives[i].damping = data.inboundJoint.drives[i].damping;
			core.drives[i].maxForce = data.inboundJoint.drives[i].maxForce;
			// TODO: what about isAcceleration? missing in Sc::ArticulationJointCore
			core.drives[i].driveType = data.inboundJoint.drives[i].driveType;

			// See Sc::ArticulationJointCore::setTargetP and Sc::ArticulationJointCore::setTargetV
			core.targetP[i] = 0.f;
			core.targetV[i] = 0.f;
			if(binP[i]!=0xffffffff)
			{
				core.targetP[i] = data.inboundJoint.targetPos[i];
				core.dirtyFlag |= Dy::ArticulationJointCoreDirtyFlag::eTARGETPOSE;
			}
			if(binV[i]!=0xffffffff)
			{
				core.targetV[i] = data.inboundJoint.targetVel[i];
				core.dirtyFlag |= Dy::ArticulationJointCoreDirtyFlag::eTARGETVELOCITY;
			}
		}

		core.twistLimitContactDistance = 0.f;
		core.tanQSwingY = 0.f;
		core.tanQSwingZ = 0.f;
		core.tanQSwingPad = 0.f;

		core.tanQTwistHigh = 0.f;
		core.tanQTwistLow = 0.f;
		core.tanQTwistPad = 0.f;

		core.swingLimited = false;
		core.twistLimited = false;
		core.tangentialStiffness = 0.0f;
		core.tangentialDamping = 0.0f;

		core.frictionCoefficient = data.inboundJoint.frictionCoefficient;
		core.maxJointVelocity = data.inboundJoint.maxJointVelocity;

		core.jointType = PxU8(data.inboundJoint.type);
	}

	// from Sc::ArticulationJointCore::setMotion
	core.dirtyFlag |= Dy::ArticulationJointCoreDirtyFlag::eMOTION;
	for(PxU32 i=0; i<PxArticulationAxis::eCOUNT; i++)
		core.motion[i] = PxU8(data.inboundJoint.motion[i]);
}

PxU32 immArticulation::addLink(const PxFeatherstoneArticulationLinkData& data)
{
	PX_ASSERT(!data.parent || getArticulation(data.parent)==this);
	PX_ASSERT(data.pose.p.isFinite());
	PX_ASSERT(data.pose.q.isFinite());

	mImmDirty = true;
	setDirty(true);

	// Replicate ArticulationSim::addBody
	addBody();

	const PxU32 index = mLinks.size();

	const PxTransform& bodyPose = data.pose;
	//
	PxsBodyCore* bodyCore = &mBodyCores[index];
	{
		// PT: this function inits everything but we only need a fraction of the data there for articulations
		bodyCore->init(	bodyPose, data.inverseInertia, data.inverseMass, 0.0f, 0.0f,
						data.linearDamping, data.angularDamping,
						data.maxLinearVelocitySq, data.maxAngularVelocitySq);

		// PT: TODO: consider exposing all used data to immediate mode API (PX-1398)
//		bodyCore->maxPenBias			= -1e32f;	// <= this one is related to setMaxDepenetrationVelocity
//		bodyCore->linearVelocity		= PxVec3(0.0f);
//		bodyCore->angularVelocity		= PxVec3(0.0f);
//		bodyCore->linearVelocity		= PxVec3(0.0f, 10.0f, 0.0f);
//		bodyCore->angularVelocity		= PxVec3(0.0f, 10.0f, 0.0f);
	}

/*	PX_ASSERT((((index==0) && (joint == 0)) && (parent == 0)) ||
				(((index!=0) && joint) && (parent && (parent->getArticulation() == this))));*/

	// PT: TODO: add ctors everywhere
	ArticulationLink& link = mLinks.insert();

	// void BodySim::postActorFlagChange(PxU32 oldFlags, PxU32 newFlags)
	bodyCore->disableGravity = data.disableGravity;
	link.bodyCore	= bodyCore;
	link.children	= 0;

/*	bool shouldSleep;
	bool currentlyAsleep;
	bool bodyReadyForSleep = body.checkSleepReadinessBesidesWakeCounter();
	PxReal wakeCounter = getCore().getWakeCounter();*/

//	const PxU32 parentIndex = data.mParent;
	const PxU32 parentIndex = getLinkIndex(data.parent);
//	const bool isRoot = parentIndex==0xffffffff;
	const bool isRoot = !data.parent;
//	const bool isRoot = isArticulationRootLink(data.mParent);
	if(!isRoot)
/*	if(parent)*/
	{
/*		currentlyAsleep = !mBodies[0]->isActive();
		shouldSleep = currentlyAsleep && bodyReadyForSleep;

		PxU32 parentIndex = findBodyIndex(*parent);*/
		link.parent = parentIndex;
		link.pathToRoot = mLinks[parentIndex].pathToRoot | ArticulationBitField(1)<<index;
		link.inboundJoint = &mArticulationJointCore[index];//&joint->getCore().getCore();
		mLinks[parentIndex].children |= ArticulationBitField(1)<<index;

		initJointCore(*link.inboundJoint, data);
	}
	else
	{
/*		currentlyAsleep = (wakeCounter == 0.0f);
		shouldSleep = currentlyAsleep && bodyReadyForSleep;*/

		link.parent = DY_ARTICULATION_LINK_NONE;
		link.pathToRoot = 1;
		link.inboundJoint = NULL;
	}
	
/*
	const PxU32 low = PxU32(link.pathToRoot & 0xffffffff);
	const PxU32 high = PxU32(link.pathToRoot >> 32);
	const PxU32 depth = Ps::bitCount(low) + Ps::bitCount(high);
	PxU32 mMaxDepth = PxMax(depth, mMaxDepth);

	mImpl.setMaxDepth(mMaxDepth);*/
//	mLLArticulation->setMaxDepth(mMaxDepth);

/*	if (currentlyAsleep && (!shouldSleep))
	{
		for(PxU32 i=0; i < (mBodies.size() - 1); i++)
			mBodies[i]->internalWakeUpArticulationLink(wakeCounter);
	}

	body.setArticulation(this, wakeCounter, shouldSleep, index);*/
	return index;
}

void immArticulation::complete()
{
	// Based on Sc::ArticulationSim::checkResize()

	if(!mImmDirty)
		return;
	mImmDirty = false;

	setupLinks(mLinks.size(), const_cast<ArticulationLink*>(mLinks.begin()));
}

static bool gRegistration = false;
void immediate::PxRegisterImmediateArticulations()
{
	Dy::PxvRegisterArticulationsReducedCoordinate();
	gRegistration = true;
}

Dy::ArticulationV* immediate::PxCreateFeatherstoneArticulation(const PxFeatherstoneArticulationData& data)
{
	PX_ASSERT(gRegistration && "Please call PxRegisterImmediateArticulations() before creating immediate articulations.");
	void* memory = physx::shdfnd::AlignedAllocator<64>().allocate(sizeof(immArticulation), __FILE__, __LINE__);
	new (memory) immArticulation(data);
	return reinterpret_cast<immArticulation*>(memory);
}

void immediate::PxReleaseArticulation(Dy::ArticulationV* articulation)
{
	if(!articulation)
		return;

	immArticulation* immArt = static_cast<immArticulation*>(articulation);
	immArt->~immArticulation();
	physx::shdfnd::AlignedAllocator<64>().deallocate(articulation);
}

// PT: TODO: add test vs DY_ARTICULATION_MAX_SIZE
Dy::ArticulationLinkHandle immediate::PxAddArticulationLink(Dy::ArticulationV* articulation, const PxFeatherstoneArticulationLinkData& data, bool isLastLink)
{
	if(!articulation)
		return 0;

	immArticulation* immArt = static_cast<immArticulation*>(articulation);
	const PxU32 id = immArt->addLink(data);

	if(isLastLink)
		immArt->complete();

	PX_ASSERT(!(size_t(articulation) & 63));
	return size_t(articulation) | id;
}

PxArticulationCache* immediate::PxCreateArticulationCache(Dy::ArticulationV* articulation) 
{
	immArticulation *immArt = static_cast<immArticulation *>(articulation);
	immArt->complete();

	const PxU32 totalDofs = immArt->getDofs();
	const PxU32 jointCount = immArt->getBodyCount() - 1;

	PxU32 totalSize =
		sizeof(Cm::SpatialVector) * immArt->getBodyCount()				//external force
		+ sizeof(PxReal) * (6 + totalDofs) * ((1 + jointCount) * 6)//offset to end of dense jacobian (assuming free floating base)
		+ sizeof(PxReal) * totalDofs * totalDofs				//mass matrix
		+ sizeof(PxReal) * totalDofs * 4						//jointVelocity, jointAcceleration, jointPosition, joint force
		+ sizeof(PxArticulationRootLinkData);			

	PxU8* tCache = reinterpret_cast<PxU8*>(PX_ALLOC(totalSize, "Articulation cache"));
	PxMemZero(tCache, totalSize);

	PxArticulationCache* cache = reinterpret_cast<PxArticulationCache*>(tCache);

	PxU32 offset = sizeof(PxArticulationCache);
	cache->externalForces = reinterpret_cast<PxSpatialForce*>(tCache + offset);
	offset += sizeof(PxSpatialForce) * immArt->getBodyCount();
	
	cache->denseJacobian = reinterpret_cast<PxReal*>(tCache + offset);
	offset += sizeof(PxReal) * (6 + totalDofs) * ((1 + jointCount) * 6);				//size of dense jacobian assuming free floating base link.

	cache->massMatrix = reinterpret_cast<PxReal*>(tCache + offset);

	offset += sizeof(PxReal) *totalDofs * totalDofs;
	cache->jointVelocity = reinterpret_cast<PxReal*>(tCache + offset);

	offset += sizeof(PxReal) * totalDofs;
	cache->jointAcceleration = reinterpret_cast<PxReal*>(tCache + offset);

	offset += sizeof(PxReal) * totalDofs;
	cache->jointPosition = reinterpret_cast<PxReal*>(tCache + offset);

	offset += sizeof(PxReal) * totalDofs;
	cache->jointForce = reinterpret_cast<PxReal*>(tCache + offset);

	offset += sizeof(PxReal) * totalDofs;
	cache->rootLinkData = reinterpret_cast<PxArticulationRootLinkData*>(tCache + offset);

	cache->coefficientMatrix = NULL;
	cache->lambda =NULL;

	const PxU32 scratchMemorySize = sizeof(Cm::SpatialVectorF) * immArt->getBodyCount() * 5	//motionVelocity, motionAccelerations, coriolisVectors, spatialZAVectors, externalAccels;
		+ sizeof(Dy::SpatialMatrix) * immArt->getBodyCount()					//compositeSpatialInertias;
		+ sizeof(PxReal) * totalDofs * 5;						//jointVelocity, jointAcceleration, jointForces, jointPositions, jointFrictionForces

	void* scratchMemory = PX_ALLOC(scratchMemorySize, "Cache scratch memory");
	cache->scratchMemory = scratchMemory;

	cache->scratchAllocator = PX_PLACEMENT_NEW(PX_ALLOC(sizeof(PxcScratchAllocator), "PxScrachAllocator"), PxcScratchAllocator)();

	reinterpret_cast<PxcScratchAllocator*>(cache->scratchAllocator)->setBlock(scratchMemory, scratchMemorySize);

	return cache;	
}

void immediate::PxCopyInternalStateToArticulationCache(Dy::ArticulationV* articulation, PxArticulationCache& cache, PxArticulationCacheFlags flag)
{
	immArticulation *immArt = static_cast<immArticulation *>(articulation);
	immArt->copyInternalStateToCache(cache, flag);	
}

void immediate::PxApplyArticulationCache(Dy::ArticulationV* articulation, PxArticulationCache& cache, PxArticulationCacheFlags flag) {
	immArticulation *immArt = static_cast<immArticulation *>(articulation);
	immArt->applyCache(cache, flag);
}

void immediate::PxReleaseArticulationCache(PxArticulationCache& cache)
{
	if (cache.scratchAllocator)
	{
		PxcScratchAllocator* scratchAlloc = reinterpret_cast<PxcScratchAllocator*>(cache.scratchAllocator);
		scratchAlloc->~PxcScratchAllocator();
		PX_FREE_AND_RESET(cache.scratchAllocator);
	}

	if (cache.scratchMemory)
		PX_FREE_AND_RESET(cache.scratchMemory);

	PX_FREE(&cache);
}

void immediate::PxComputeUnconstrainedVelocities(Dy::ArticulationV* articulation, const PxVec3& gravity, const PxReal dt)
{
	if(!articulation)
		return;

	immArticulation* immArt = static_cast<immArticulation*>(articulation);
	immArt->complete();
	immArt->immComputeUnconstrainedVelocities(dt, gravity);
}

void immediate::PxUpdateArticulationBodies(Dy::ArticulationV* articulation, const PxReal dt)
{
	if(!articulation)
		return;

	FeatherstoneArticulation::updateBodies(static_cast<immArticulation*>(articulation), dt, true);
}

void immediate::PxComputeUnconstrainedVelocitiesTGS(Dy::ArticulationV* articulation, const PxVec3& gravity, const PxReal dt,
	const PxReal totalDt, const PxReal invDt, const PxReal invTotalDt)
{
	if (!articulation)
		return;

	immArticulation* immArt = static_cast<immArticulation*>(articulation);
	immArt->complete();
	immArt->immComputeUnconstrainedVelocitiesTGS(dt, totalDt, invDt, invTotalDt, gravity);
}

void immediate::PxUpdateArticulationBodiesTGS(Dy::ArticulationV* articulation, const PxReal dt)
{
	if (!articulation)
		return;

	FeatherstoneArticulation::updateBodies(static_cast<immArticulation*>(articulation), dt, false);
}

Dy::ArticulationV* immediate::PxGetLinkArticulation(const Dy::ArticulationLinkHandle link)
{
	return Dy::getArticulation(link);
}

PxU32 immediate::PxGetLinkIndex(const Dy::ArticulationLinkHandle link)
{
	return Dy::getLinkIndex(link);
}

static void copyLinkData(PxLinkData& data, const immArticulation* immArt, PxU32 index)
{
	data.pose				= immArt->mBodyCores[index].body2World;
//	data.linearVelocity		= immArt->mBodyCores[index].linearVelocity;
//	data.angularVelocity	= immArt->mBodyCores[index].angularVelocity;
	const Cm::SpatialVectorF& velocity = immArt->getArticulationData().getMotionVelocity(index);
	data.linearVelocity	= velocity.bottom;
	data.angularVelocity	= velocity.top;
}

static PX_FORCE_INLINE const immArticulation* getFromLink(const Dy::ArticulationLinkHandle link, PxU32& index)
{
	const Dy::ArticulationV* articulation = Dy::getArticulation(link);
	if(!articulation)
		return NULL;

	const immArticulation* immArt = static_cast<const immArticulation*>(articulation);
	index = Dy::getLinkIndex(link);

	if(index>=immArt->mLinks.size())
		return NULL;

	return immArt;
}

bool immediate::PxGetLinkData(const Dy::ArticulationLinkHandle link, PxLinkData& data)
{
	PxU32 index;
	const immArticulation* immArt = getFromLink(link, index);
	if(!immArt)
		return false;

	copyLinkData(data, immArt, index);

	return true;
}

PxU32 immediate::PxGetAllLinkData(const Dy::ArticulationV* articulation, PxLinkData* data)
{
	if(!articulation)
		return 0;

	const immArticulation* immArt = static_cast<const immArticulation*>(articulation);

	const PxU32 nb = immArt->mLinks.size();
	if(data)
	{
		for(PxU32 i=0;i<nb;i++)
			copyLinkData(data[i], immArt, i);
	}

	return nb;
}

bool immediate::PxGetMutableLinkData(const Dy::ArticulationLinkHandle link, PxMutableLinkData& data)
{
	PxU32 index;
	const immArticulation* immArt = getFromLink(link, index);
	if(!immArt)
		return false;

	data.inverseInertia			= immArt->mBodyCores[index].inverseInertia;
	data.inverseMass			= immArt->mBodyCores[index].inverseMass;
	data.linearDamping			= immArt->mBodyCores[index].linearDamping;
	data.angularDamping			= immArt->mBodyCores[index].angularDamping;
	data.maxLinearVelocitySq	= immArt->mBodyCores[index].maxLinearVelocitySq;
	data.maxAngularVelocitySq	= immArt->mBodyCores[index].maxAngularVelocitySq;
	data.disableGravity			= immArt->mBodyCores[index].disableGravity!=0;

	return true;
}

bool immediate::PxSetMutableLinkData(Dy::ArticulationLinkHandle link, const PxMutableLinkData& data)
{
	PxU32 index;
	immArticulation* immArt = const_cast<immArticulation*>(getFromLink(link, index));
	if(!immArt)
		return false;

	immArt->mBodyCores[index].inverseInertia		= data.inverseInertia;			// See Sc::BodyCore::setInverseInertia
	immArt->mBodyCores[index].inverseMass			= data.inverseMass;				// See Sc::BodyCore::setInverseMass
	immArt->mBodyCores[index].linearDamping			= data.linearDamping;			// See Sc::BodyCore::setLinearDamping
	immArt->mBodyCores[index].angularDamping		= data.angularDamping;			// See Sc::BodyCore::setAngularDamping
	immArt->mBodyCores[index].maxLinearVelocitySq	= data.maxLinearVelocitySq;		// See Sc::BodyCore::setMaxLinVelSq
	immArt->mBodyCores[index].maxAngularVelocitySq	= data.maxAngularVelocitySq;	// See Sc::BodyCore::setMaxAngVelSq
	immArt->mBodyCores[index].disableGravity		= data.disableGravity;			// See BodySim::postActorFlagChange

	return true;
}

bool immediate::PxGetJointData(const Dy::ArticulationLinkHandle link, PxFeatherstoneArticulationJointData& data)
{
	PxU32 index;
	const immArticulation* immArt = getFromLink(link, index);
	if(!immArt)
		return false;

	const Dy::ArticulationJointCore& core = immArt->mArticulationJointCore[index];

	data.parentPose				= core.parentPose;
	data.childPose				= core.childPose;
	data.type					= PxArticulationJointType::Enum(core.jointType);
	data.frictionCoefficient	= core.frictionCoefficient;
	data.maxJointVelocity		= core.maxJointVelocity;
	for(PxU32 i=0;i<PxArticulationAxis::eCOUNT;i++)
	{
		data.motion[i]					= PxArticulationMotion::Enum(PxU8(core.motion[i]));
		data.limits[i].low				= core.limits[i].low;
		data.limits[i].high				= core.limits[i].high;
		data.drives[i].stiffness		= core.drives[i].stiffness;
		data.drives[i].damping			= core.drives[i].damping;
		data.drives[i].maxForce			= core.drives[i].maxForce;
		data.drives[i].driveType		= core.drives[i].driveType;
		data.targetPos[i]				= core.targetP[i];
		data.targetVel[i]				= core.targetV[i];
	}
	return true;
}

static bool samePoses(const PxTransform& pose0, const PxTransform& pose1)
{
	return (pose0.p == pose1.p) && (pose0.q == pose1.q);
}

// PT: this is not super efficient if you only want to change one parameter. We could consider adding individual, atomic accessors (but that would
// bloat the API) or flags to validate the desired parameters.
bool immediate::PxSetJointData(Dy::ArticulationLinkHandle link, const PxFeatherstoneArticulationJointData& data)
{
	PxU32 index;
	immArticulation* immArt = const_cast<immArticulation*>(getFromLink(link, index));
	if(!immArt)
		return false;

	Dy::ArticulationJointCore& core = immArt->mArticulationJointCore[index];

	bool dirty = false;

	if(!samePoses(core.parentPose, data.parentPose))
	{
		core.parentPose	= data.parentPose;
		core.dirtyFlag |= Dy::ArticulationJointCoreDirtyFlag::ePOSE;
		dirty = true;
	}

	if(!samePoses(core.childPose, data.childPose))
	{
		core.childPose	= data.childPose;
		core.dirtyFlag |= Dy::ArticulationJointCoreDirtyFlag::ePOSE;
		dirty = true;
	}

	if(core.jointType!=PxU8(data.type))
	{
		core.jointType	= PxU8(data.type);
		dirty = true;
	}
	core.frictionCoefficient	= data.frictionCoefficient;
	core.maxJointVelocity		= data.maxJointVelocity;
	for(PxU32 i=0;i<PxArticulationAxis::eCOUNT;i++)
	{
		core.limits[i].low				= data.limits[i].low;
		core.limits[i].high				= data.limits[i].high;
		core.drives[i].stiffness		= data.drives[i].stiffness;
		core.drives[i].damping			= data.drives[i].damping;
		core.drives[i].maxForce			= data.drives[i].maxForce;
		core.drives[i].driveType		= data.drives[i].driveType;

		if(core.motion[i]!=data.motion[i])
		{
			core.motion[i] = PxU8(data.motion[i]);
			core.dirtyFlag |= Dy::ArticulationJointCoreDirtyFlag::eMOTION;
			dirty = true;
		}

		if(core.targetP[i] != data.targetPos[i])
		{
			core.targetP[i] = data.targetPos[i];
			core.dirtyFlag |= Dy::ArticulationJointCoreDirtyFlag::eTARGETPOSE;
			dirty = true;
		}

		if(core.targetV[i] != data.targetVel[i])
		{
			core.targetV[i] = data.targetVel[i];
			core.dirtyFlag |= Dy::ArticulationJointCoreDirtyFlag::eTARGETVELOCITY;
			dirty = true;
		}
	}

	if(dirty)
		immArt->setDirty(true);

	return true;
}

namespace physx
{
namespace Dy
{
	void copyToSolverBodyDataStep(const PxVec3& linearVelocity, const PxVec3& angularVelocity, const PxReal invMass, const PxVec3& invInertia, const PxTransform& globalPose,
		const PxReal maxDepenetrationVelocity, const PxReal maxContactImpulse, const PxU32 nodeIndex, const PxReal reportThreshold,
		const PxReal maxAngVelSq, PxU32 lockFlags, bool isKinematic,
		PxTGSSolverBodyVel& solverVel, PxTGSSolverBodyTxInertia& solverBodyTxInertia, PxTGSSolverBodyData& solverBodyData);

	void integrateCoreStep(PxTGSSolverBodyVel& vel, PxTGSSolverBodyTxInertia& txInertia, const PxF32 dt);
}
}

void immediate::PxConstructSolverBodiesTGS(const PxRigidBodyData* inRigidData, PxTGSSolverBodyVel* outSolverBodyVel, PxTGSSolverBodyTxInertia* outSolverBodyTxInertia, PxTGSSolverBodyData* outSolverBodyData, const PxU32 nbBodies, const PxVec3& gravity, const PxReal dt)
{
	for (PxU32 a = 0; a<nbBodies; a++)
	{
		const PxRigidBodyData& rigidData = inRigidData[a];
		PxVec3 lv = rigidData.linearVelocity, av = rigidData.angularVelocity;
		Dy::bodyCoreComputeUnconstrainedVelocity(gravity, dt, rigidData.linearDamping, rigidData.angularDamping, 1.0f, rigidData.maxLinearVelocitySq, rigidData.maxAngularVelocitySq, lv, av, false);

		Dy::copyToSolverBodyDataStep(lv, av, rigidData.invMass, rigidData.invInertia, rigidData.body2World, -rigidData.maxDepenetrationVelocity, rigidData.maxContactImpulse, IG_INVALID_NODE,
			PX_MAX_F32, rigidData.maxAngularVelocitySq, 0, false, outSolverBodyVel[a], outSolverBodyTxInertia[a], outSolverBodyData[a]);
	}
}

void immediate::PxConstructStaticSolverBodyTGS(const PxTransform& globalPose, PxTGSSolverBodyVel& solverBodyVel, PxTGSSolverBodyTxInertia& solverBodyTxInertia, PxTGSSolverBodyData& solverBodyData)
{
	const PxVec3 zero(0.0f);
	Dy::copyToSolverBodyDataStep(zero, zero, 0.f, zero, globalPose, -PX_MAX_F32, PX_MAX_F32, IG_INVALID_NODE, PX_MAX_F32, PX_MAX_F32, 0, true, solverBodyVel, solverBodyTxInertia, solverBodyData);
}

PxU32 immediate::PxBatchConstraintsTGS(const PxSolverConstraintDesc* solverConstraintDescs, const PxU32 nbConstraints, PxTGSSolverBodyVel* solverBodies, const PxU32 nbBodies,
	PxConstraintBatchHeader* outBatchHeaders, PxSolverConstraintDesc* outOrderedConstraintDescs,
	Dy::ArticulationV** articulations, const PxU32 nbArticulations)
{
	if (!nbArticulations)
	{
		RigidBodyClassification classification(reinterpret_cast<PxU8*>(solverBodies), nbBodies, sizeof(PxTGSSolverBodyVel));
		return BatchConstraints(solverConstraintDescs, nbConstraints, outBatchHeaders, outOrderedConstraintDescs, classification);
	}
	else
	{
		ExtendedRigidBodyClassification classification(reinterpret_cast<PxU8*>(solverBodies), nbBodies, sizeof(PxTGSSolverBodyVel), articulations, nbArticulations);
		return BatchConstraints(solverConstraintDescs, nbConstraints, outBatchHeaders, outOrderedConstraintDescs, classification);
	}
}

bool immediate::PxCreateContactConstraintsTGS(PxConstraintBatchHeader* batchHeaders, const PxU32 nbHeaders, PxTGSSolverContactDesc* contactDescs,
	PxConstraintAllocator& allocator, const PxReal invDt, const PxReal invTotalDt, const PxReal bounceThreshold, const PxReal frictionOffsetThreshold, const PxReal correlationDistance)
{
	PX_ASSERT(invDt > 0.f && PxIsFinite(invDt));
	PX_ASSERT(bounceThreshold < 0.f);
	PX_ASSERT(frictionOffsetThreshold > 0.f);
	PX_ASSERT(correlationDistance > 0.f);

	Dy::SolverConstraintPrepState::Enum state = Dy::SolverConstraintPrepState::eUNBATCHABLE;

	Dy::CorrelationBuffer cb;

	PxU32 currentContactDescIdx = 0;

	for (PxU32 i = 0; i < nbHeaders; ++i)
	{
		PxConstraintBatchHeader& batchHeader = batchHeaders[i];
		if (batchHeader.stride == 4)
		{
			PxU32 totalContacts = contactDescs[currentContactDescIdx].numContacts + contactDescs[currentContactDescIdx + 1].numContacts +
				contactDescs[currentContactDescIdx + 2].numContacts + contactDescs[currentContactDescIdx + 3].numContacts;

			if (totalContacts <= 64)
			{
				state = Dy::createFinalizeSolverContacts4Step(cb,
					contactDescs + currentContactDescIdx,
					invDt,
					invTotalDt,
					bounceThreshold,
					frictionOffsetThreshold,
					correlationDistance,
					0.f,
					allocator);
			}
		}

		if (state == Dy::SolverConstraintPrepState::eUNBATCHABLE)
		{
			for (PxU32 a = 0; a < batchHeader.stride; ++a)
			{
				Dy::createFinalizeSolverContactsStep(contactDescs[currentContactDescIdx + a], cb, invDt, invTotalDt, bounceThreshold,
					frictionOffsetThreshold, correlationDistance, allocator);
			}
		}
		PxU8 type = *contactDescs[currentContactDescIdx].desc->constraint;

		if (type == DY_SC_TYPE_STATIC_CONTACT)
		{
			//Check if any block of constraints is classified as type static (single) contact constraint.
			//If they are, iterate over all constraints grouped with it and switch to "dynamic" contact constraint
			//type if there's a dynamic contact constraint in the group.
			for (PxU32 c = 1; c < batchHeader.stride; ++c)
			{
				if (*contactDescs[currentContactDescIdx + c].desc->constraint == DY_SC_TYPE_RB_CONTACT)
				{
					type = DY_SC_TYPE_RB_CONTACT;
					break;
				}
			}
		}

		batchHeader.constraintType = type;

		currentContactDescIdx += batchHeader.stride;
	}
	return true;
}

bool immediate::PxCreateJointConstraintsTGS(PxConstraintBatchHeader* batchHeaders, const PxU32 nbHeaders, 
	PxTGSSolverConstraintPrepDesc* jointDescs, PxConstraintAllocator& allocator, const PxReal dt, const PxReal totalDt, const PxReal invDt,
	const PxReal invTotalDt, const PxReal lengthScale)
{
	PX_ASSERT(dt > 0.f);
	PX_ASSERT(invDt > 0.f && PxIsFinite(invDt));

	Dy::SolverConstraintPrepState::Enum state = Dy::SolverConstraintPrepState::eUNBATCHABLE;

	PxU32 currentDescIdx = 0;
	for (PxU32 i = 0; i < nbHeaders; ++i)
	{
		PxConstraintBatchHeader& batchHeader = batchHeaders[i];

		PxU8 type = DY_SC_TYPE_BLOCK_1D;
		if (batchHeader.stride == 4)
		{
			PxU32 totalRows = 0;
			PxU32 maxRows = 0;
			bool batchable = true;
			for (PxU32 a = 0; a < batchHeader.stride; ++a)
			{
				if (jointDescs[currentDescIdx + a].numRows == 0)
				{
					batchable = false;
					break;
				}
				totalRows += jointDescs[currentDescIdx + a].numRows;
				maxRows = PxMax(maxRows, jointDescs[currentDescIdx + a].numRows);
			}

			if (batchable)
			{
				state = Dy::setupSolverConstraintStep4
				(jointDescs + currentDescIdx,
					dt, totalDt, invDt, invTotalDt, totalRows,
					allocator, maxRows, lengthScale);
			}
		}

		if (state == Dy::SolverConstraintPrepState::eUNBATCHABLE)
		{
			type = DY_SC_TYPE_RB_1D;
			for (PxU32 a = 0; a < batchHeader.stride; ++a)
			{
				// PT: TODO: And "isExtended" is already computed in Dy::ConstraintHelper::setupSolverConstraint
				PxSolverConstraintDesc& desc = *jointDescs[currentDescIdx + a].desc;
				const bool isExtended = desc.linkIndexA != PxSolverConstraintDesc::NO_LINK || desc.linkIndexB != PxSolverConstraintDesc::NO_LINK;
				if (isExtended)
					type = DY_SC_TYPE_EXT_1D;


				Dy::setupSolverConstraintStep(jointDescs[currentDescIdx + a], allocator, dt, totalDt, invDt, invTotalDt, lengthScale);
			}
		}

		batchHeader.constraintType = type;
		currentDescIdx += batchHeader.stride;
	}

	return true;
}


template<class LeafTestT, class ParamsT>
static bool PxCreateJointConstraintsWithShadersTGS_T(PxConstraintBatchHeader* batchHeaders, const PxU32 nbHeaders, ParamsT* params, PxTGSSolverConstraintPrepDesc* jointDescs,
	PxConstraintAllocator& allocator, const PxReal dt, const PxReal totalDt, const PxReal invDt, const PxReal invTotalDt, const PxReal lengthScale)
{
	Px1DConstraint allRows[Dy::MAX_CONSTRAINT_ROWS * 4];

	//Runs shaders to fill in rows...

	PxU32 currentDescIdx = 0;

	for (PxU32 i = 0; i<nbHeaders; i++)
	{
		PxU32 numRows = 0;
		PxU32 preppedIndex = 0;
		PxU32 maxRows = 0;

		PxConstraintBatchHeader& batchHeader = batchHeaders[i];

		for (PxU32 a = 0; a<batchHeader.stride; a++)
		{
			Px1DConstraint* rows = allRows + numRows;
			PxTGSSolverConstraintPrepDesc& desc = jointDescs[currentDescIdx + a];

			PxConstraintSolverPrep prep;
			const void* constantBlock;
			const bool useExtendedLimits = LeafTestT::getData(params, currentDescIdx + a, &prep, &constantBlock);
			PX_ASSERT(prep);

			PxMemZero(rows + preppedIndex, sizeof(Px1DConstraint)*(Dy::MAX_CONSTRAINT_ROWS));
			for (PxU32 b = preppedIndex; b < Dy::MAX_CONSTRAINT_ROWS; ++b)
			{
				Px1DConstraint& c = rows[b];
				//Px1DConstraintInit(c);
				c.minImpulse = -PX_MAX_REAL;
				c.maxImpulse = PX_MAX_REAL;
			}

			desc.invMassScales.linear0 = desc.invMassScales.linear1 = desc.invMassScales.angular0 = desc.invMassScales.angular1 = 1.0f;

			desc.body0WorldOffset = PxVec3(0.0f);

			const PxU32 constraintCount = prep(rows,
				desc.body0WorldOffset,
				Dy::MAX_CONSTRAINT_ROWS,
				desc.invMassScales,
				constantBlock,
				desc.bodyFrame0, desc.bodyFrame1,
				useExtendedLimits,
				desc.cA2w, desc.cB2w);

			preppedIndex = Dy::MAX_CONSTRAINT_ROWS - constraintCount;

			maxRows = PxMax(constraintCount, maxRows);

			desc.rows = rows;
			desc.numRows = constraintCount;
			numRows += constraintCount;
		}

		PxCreateJointConstraintsTGS(&batchHeader, 1, jointDescs + currentDescIdx, allocator, dt, totalDt, invDt,
			invTotalDt, lengthScale);

		currentDescIdx += batchHeader.stride;
	}
	return true; //KS - TODO - do some error reporting/management...
}


bool immediate::PxCreateJointConstraintsWithShadersTGS(PxConstraintBatchHeader* batchHeaders, const PxU32 nbBatchHeaders, PxConstraint** constraints, PxTGSSolverConstraintPrepDesc* jointDescs, PxConstraintAllocator& allocator, const PxReal dt,
	const PxReal totalDt, const PxReal invDt, const PxReal invTotalDt, const PxReal lengthScale)
{
	class PxConstraintAdapter
	{
	public:
		static PX_FORCE_INLINE bool getData(PxConstraint** constraints, PxU32 i, PxConstraintSolverPrep* prep, const void** constantBlock)
		{
			NpConstraint* npConstraint = static_cast<NpConstraint*>(constraints[i]);
			Sc::ConstraintCore& core = npConstraint->getScbConstraint().getScConstraint();

			if (npConstraint->isDirty())
			{
				core.getPxConnector()->prepareData();
				npConstraint->markClean();
			}

			*prep = core.getPxConnector()->getPrep();
			*constantBlock = core.getPxConnector()->getConstantBlock();
			return core.getFlags() & PxConstraintFlag::eENABLE_EXTENDED_LIMITS;
		}
	};

	return PxCreateJointConstraintsWithShadersTGS_T<PxConstraintAdapter>(batchHeaders, nbBatchHeaders, constraints, jointDescs, allocator, dt, totalDt, invDt, invTotalDt, lengthScale);
}

bool immediate::PxCreateJointConstraintsWithImmediateShadersTGS(PxConstraintBatchHeader* batchHeaders, const PxU32 nbHeaders, PxImmediateConstraint* constraints, PxTGSSolverConstraintPrepDesc* jointDescs,
	PxConstraintAllocator& allocator, const PxReal dt, const PxReal totalDt, const PxReal invDt, const PxReal invTotalDt, const PxReal lengthScale)
{
	class immConstraintAdapter
	{
	public:
		static PX_FORCE_INLINE bool getData(PxImmediateConstraint* constraints, PxU32 i, PxConstraintSolverPrep* prep, const void** constantBlock)
		{
			const PxImmediateConstraint& ic = constraints[i];
			*prep = ic.prep;
			*constantBlock = ic.constantBlock;
			return false;
		}
	};

	return PxCreateJointConstraintsWithShadersTGS_T<immConstraintAdapter>(batchHeaders, nbHeaders, constraints, jointDescs, allocator, dt, totalDt, invDt, invTotalDt, lengthScale);
}


void immediate::PxSolveConstraintsTGS(const PxConstraintBatchHeader* batchHeaders, const PxU32 nbBatchHeaders, const PxSolverConstraintDesc* solverConstraintDescs,
	PxTGSSolverBodyVel* solverBodies, PxTGSSolverBodyTxInertia* txInertias, const PxU32 nbSolverBodies, const PxU32 nbPositionIterations, const PxU32 nbVelocityIterations,
	const float dt, const float invDt, const PxU32 nbSolverArticulations, Dy::ArticulationV** solverArticulations)
{
	PX_UNUSED(solverBodies);
	PX_UNUSED(nbSolverBodies);
	PX_ASSERT(nbPositionIterations > 0);
	PX_ASSERT(nbVelocityIterations > 0);

													   //Stage 1: solve the position iterations...
	Dy::TGSSolveBlockMethod* solveTable = Dy::g_SolveTGSMethods;

	Dy::TGSSolveConcludeMethod* solveConcludeTable = Dy::g_SolveConcludeTGSMethods;

	Dy::TGSWriteBackMethod* writebackTable = Dy::g_WritebackTGSMethods;

	Dy::SolverContext cache;
	cache.mThresholdStreamIndex = 0;
	cache.mThresholdStreamLength = 0xFFFFFFF;

	PX_ASSERT(nbPositionIterations > 0);
	PX_ASSERT(nbVelocityIterations > 0);

	Cm::SpatialVectorF Z[DY_ARTICULATION_MAX_SIZE];
	Cm::SpatialVectorF deltaV[DY_ARTICULATION_MAX_SIZE];

	cache.Z = Z;
	cache.deltaV = deltaV;

	struct Articulations
	{
		static PX_FORCE_INLINE void solveInternalConstraints(const float dt, const float invDt, const PxU32 nbSolverArticulations, Dy::ArticulationV** solverArticulations, Cm::SpatialVectorF* Z, Cm::SpatialVectorF* deltaV, 
			const PxReal elapsedTime, bool velIter)
		{
			for (PxU32 a = 0; a<nbSolverArticulations; a++)
			{
				immArticulation* immArt = static_cast<immArticulation*>(solverArticulations[a]);
				immArt->immSolveInternalConstraints(dt, invDt, Z, deltaV, elapsedTime, velIter, true);
			}
		}

		static PX_FORCE_INLINE void stepArticulations(const float dt, const PxReal totalInvDt, const PxU32 nbSolverArticulations, Dy::ArticulationV** solverArticulations,
			Cm::SpatialVectorF* deltaV)
		{
			for (PxU32 a = 0; a<nbSolverArticulations; a++)
			{
				immArticulation* immArt = static_cast<immArticulation*>(solverArticulations[a]);
				immArt->recordDeltaMotion(immArt->getSolverDesc(), dt, deltaV, totalInvDt);
			}
		}
	};

	const PxReal invTotalDt = 1.f/(dt*nbPositionIterations);

	PxReal elapsedTime = 0.f;
	for (PxU32 i = nbPositionIterations; i>1; --i)
	{
		Articulations::solveInternalConstraints(dt, invDt, nbSolverArticulations, solverArticulations, Z, deltaV, elapsedTime, false);

		cache.doFriction = true;
		for (PxU32 a = 0; a<nbBatchHeaders; ++a)
		{
			const PxConstraintBatchHeader& batch = batchHeaders[a];
			solveTable[batch.constraintType](batch, solverConstraintDescs, txInertias, -PX_MAX_F32, elapsedTime, cache);
		}

		for(PxU32 j = 0; j < nbSolverBodies; ++j)
			Dy::integrateCoreStep(solverBodies[j], txInertias[j], dt);

		Articulations::stepArticulations(dt, invTotalDt, nbSolverArticulations, solverArticulations, deltaV);
		elapsedTime += dt;
	}

	cache.doFriction = true;

	Articulations::solveInternalConstraints(dt, invDt, nbSolverArticulations, solverArticulations, Z, deltaV, elapsedTime, false);

	for (PxU32 a = 0; a<nbBatchHeaders; ++a)
	{
		const PxConstraintBatchHeader& batch = batchHeaders[a];
		solveConcludeTable[batch.constraintType](batch, solverConstraintDescs, txInertias, elapsedTime, cache);
	}
	

	for (PxU32 j = 0; j < nbSolverBodies; ++j)
		Dy::integrateCoreStep(solverBodies[j], txInertias[j], dt);

	Articulations::stepArticulations(dt, invTotalDt, nbSolverArticulations, solverArticulations, deltaV);
	elapsedTime += dt;

	for (PxU32 i = nbVelocityIterations; i>1; --i)
	{
		Articulations::solveInternalConstraints(dt, invDt, nbSolverArticulations, solverArticulations, Z, deltaV, elapsedTime, true);
		for (PxU32 a = 0; a<nbBatchHeaders; ++a)
		{
			const PxConstraintBatchHeader& batch = batchHeaders[a];
			solveTable[batch.constraintType](batch, solverConstraintDescs, txInertias, 0.f, elapsedTime, cache);
		}
	}

	Articulations::solveInternalConstraints(dt, invDt, nbSolverArticulations, solverArticulations, Z, deltaV, elapsedTime, true);

	for (PxU32 a = 0; a<nbBatchHeaders; ++a)
	{
		const PxConstraintBatchHeader& batch = batchHeaders[a];
		solveTable[batch.constraintType](batch, solverConstraintDescs, txInertias, 0.f, elapsedTime, cache);
		writebackTable[batch.constraintType](batch, solverConstraintDescs, &cache);
	}
	
}

void immediate::PxIntegrateSolverBodiesTGS(PxTGSSolverBodyVel* solverBody, PxTGSSolverBodyTxInertia* txInertia, PxTransform* poses, const PxU32 nbBodiesToIntegrate, const PxReal dt)
{
	PX_UNUSED(dt);
	for (PxU32 i = 0; i < nbBodiesToIntegrate; ++i)
	{
		solverBody[i].angularVelocity = txInertia[i].sqrtInvInertia * solverBody[i].angularVelocity;
		poses[i].p = txInertia[i].deltaBody2World.p;
		poses[i].q = (txInertia[i].deltaBody2World.q * poses[i].q).getNormalized();
	}
}
