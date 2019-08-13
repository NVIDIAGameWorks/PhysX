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


#ifndef PX_PHYSICS_SCP_ARTICULATION_JOINT_CORE
#define PX_PHYSICS_SCP_ARTICULATION_JOINT_CORE

#include "foundation/PxTransform.h"
#include "common/PxMetaData.h"
#include "CmPhysXCommon.h"
#include "PsUserAllocated.h"
#include "DyArticulation.h"

namespace physx
{
namespace Sc
{
	class BodyCore;
	class ArticulationJointSim;
	class ArticulationCore;

	class ArticulationJointDesc
	{
	public:
		BodyCore*	parent;
		BodyCore*	child;
		PxTransform	parentPose;
		PxTransform	childPose;
	};

	class ArticulationJointCore : public Ps::UserAllocated
	{
	//= ATTENTION! =====================================================================================
	// Changing the data layout of this class breaks the binary serialization format.  See comments for 
	// PX_BINARY_SERIAL_VERSION.  If a modification is required, please adjust the getBinaryMetaData 
	// function.  If the modification is made on a custom branch, please change PX_BINARY_SERIAL_VERSION
	// accordingly.
	//==================================================================================================

		//---------------------------------------------------------------------------------
		// Construction, destruction & initialization
		//---------------------------------------------------------------------------------
	public:
// PX_SERIALIZATION
															ArticulationJointCore(const PxEMPTY) : mSim(NULL), mCore(PxEmpty)	{}
						void								preExportDataReset() { mCore.dirtyFlag = Dy::ArticulationJointCoreDirtyFlag::eALL; }
		static			void								getBinaryMetaData(PxOutputStream& stream);
//~PX_SERIALIZATION
															ArticulationJointCore( const PxTransform& parentFrame, const PxTransform& childFrame, bool reducedCoordinate);
															~ArticulationJointCore();

		//---------------------------------------------------------------------------------
		// External API
		//---------------------------------------------------------------------------------

		PX_FORCE_INLINE	const PxTransform&					getParentPose() const { return mCore.parentPose; }
						void								setParentPose(const PxTransform&);

		PX_FORCE_INLINE	const PxTransform&					getChildPose() const { return mCore.childPose; }
						void								setChildPose(const PxTransform&);

		PX_FORCE_INLINE	const PxQuat&						getTargetOrientation() const { return mCore.targetPosition; }
						void								setTargetOrientation(const PxQuat&);

		PX_FORCE_INLINE	const PxVec3&						getTargetVelocity() const { return mCore.targetVelocity; }
						void								setTargetVelocity(const PxVec3&);

		PX_FORCE_INLINE	PxReal								getStiffness() const { return mCore.spring; }
						void								setStiffness(PxReal);

		PX_FORCE_INLINE	PxReal								getDamping() const { return mCore.damping; }
						void								setDamping(PxReal);

		PX_FORCE_INLINE	PxReal								getInternalCompliance() const { return mCore.internalCompliance; }
						void								setInternalCompliance(PxReal);

		PX_FORCE_INLINE	PxReal								getExternalCompliance() const { return mCore.externalCompliance; }
						void								setExternalCompliance(PxReal);

		PX_FORCE_INLINE	void								getSwingLimit(PxReal& yLimit, PxReal& zLimit) const { yLimit = mCore.limits[PxArticulationAxis::eSWING1].low; zLimit = mCore.limits[PxArticulationAxis::eSWING2].low; }
						void								setSwingLimit(PxReal yLimit, PxReal zLimit);

		PX_FORCE_INLINE	PxReal								getTangentialStiffness() const { return mCore.tangentialStiffness; }
						void								setTangentialStiffness(PxReal);

		PX_FORCE_INLINE	PxReal								getTangentialDamping() const { return mCore.tangentialDamping; }
						void								setTangentialDamping(PxReal);

		PX_FORCE_INLINE	bool								getSwingLimitEnabled() const { return mCore.swingLimited; }
						void								setSwingLimitEnabled(bool);

		PX_FORCE_INLINE	PxReal								getSwingLimitContactDistance() const { return mCore.swingLimitContactDistance; }
						void								setSwingLimitContactDistance(PxReal);

		PX_FORCE_INLINE	void								getTwistLimit(PxReal& lower, PxReal& upper) const { lower = mCore.limits[PxArticulationAxis::eTWIST].low; upper = mCore.limits[PxArticulationAxis::eTWIST].high; }
						void								setTwistLimit(PxReal lower, PxReal upper);

						void								getLimit(PxArticulationAxis::Enum axis, PxReal& lower, PxReal& upper) const
															{
																lower = mCore.limits[axis].low;
																upper = mCore.limits[axis].high;
															}

						void								setLimit(PxArticulationAxis::Enum axis, PxReal lower, PxReal upper);

						void								getDrive(PxArticulationAxis::Enum axis, PxReal& stiffness, PxReal& damping, PxReal& maxForce, PxArticulationDriveType::Enum& driveType) const
															{
																stiffness = mCore.drives[axis].stiffness;
																damping = mCore.drives[axis].damping;
																maxForce = mCore.drives[axis].maxForce;
																driveType = mCore.drives[axis].driveType;
															}

						void								setDrive(PxArticulationAxis::Enum axis, PxReal stiffness, PxReal damping, PxReal maxForce, PxArticulationDriveType::Enum driveType);

						void								setTargetP(PxArticulationAxis::Enum axis, PxReal targetP);
		PX_FORCE_INLINE	PxReal								getTargetP(PxArticulationAxis::Enum axis)	const	{ return mCore.targetP[axis];	}

						void								setTargetV(PxArticulationAxis::Enum axis, PxReal targetV);
		PX_FORCE_INLINE	PxReal								getTargetV(PxArticulationAxis::Enum axis)	const	{ return mCore.targetV[axis];	}

		PX_FORCE_INLINE	bool								getTwistLimitEnabled()						const	{ return mCore.twistLimited; }
						void								setTwistLimitEnabled(bool);

		PX_FORCE_INLINE	PxReal								getTwistLimitContactDistance()				const	{ return mCore.twistLimitContactDistance; }
						void								setTwistLimitContactDistance(PxReal);

						void								setDriveType(PxArticulationJointDriveType::Enum type);
						PxArticulationJointDriveType::Enum	getDriveType()								const	{ return PxArticulationJointDriveType::Enum(mCore.driveType); }

						void								setJointType(PxArticulationJointType::Enum type);
						PxArticulationJointType::Enum		getJointType()								const;

						void								setMotion(PxArticulationAxis::Enum axis, PxArticulationMotion::Enum motion);
						PxArticulationMotion::Enum			getMotion(PxArticulationAxis::Enum axis)	const;

						void								setFrictionCoefficient(const PxReal coefficient);
						PxReal								getFrictionCoefficient()					const;

						void								setMaxJointVelocity(const PxReal maxJointV);
						PxReal								getMaxJointVelocity()						const;

						PxArticulationJointBase*			getPxArticulationJointBase();
						const PxArticulationJointBase*		getPxArticulationJointBase()				const;

		//---------------------------------------------------------------------------------
		// Low Level data access - some wouldn't be needed if the interface wasn't virtual
		//---------------------------------------------------------------------------------

		PX_FORCE_INLINE	ArticulationJointSim*				getSim()									const	{ return mSim;	}
		PX_FORCE_INLINE	void								setSim(ArticulationJointSim* sim)
															{
																PX_ASSERT((sim==0) ^ (mSim == 0));
																mSim = sim;
															}

		PX_FORCE_INLINE	Dy::ArticulationJointCore&			getCore()											{ return mCore; }

		PX_FORCE_INLINE void								setArticulation(ArticulationCore* articulation)		{ mArticulation = articulation;	}
		PX_FORCE_INLINE	const ArticulationCore*				getArticulation()							const	{ return mArticulation; }

		PX_FORCE_INLINE void								setRoot(PxArticulationJointBase* base)				{ mRootType = base; }
		PX_FORCE_INLINE PxArticulationJointBase*			getRoot()									const	{ return mRootType; }

	private:

		void setDirty(Dy::ArticulationJointCoreDirtyFlag::Enum dirtyFlag);
						ArticulationJointSim*				mSim;
						Dy::ArticulationJointCore			mCore;
						ArticulationCore*					mArticulation;
						PxArticulationJointBase*			mRootType;
	};

} // namespace Sc

}

#endif
