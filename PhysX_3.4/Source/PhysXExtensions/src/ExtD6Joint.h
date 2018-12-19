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


#ifndef NP_D6JOINTCONSTRAINT_H
#define NP_D6JOINTCONSTRAINT_H

#include "ExtJoint.h"
#include "PxD6Joint.h"
#include "PsMathUtils.h"

namespace physx
{
struct PxD6JointGeneratedValues;
namespace Ext
{
	PX_FORCE_INLINE	PxReal tanHalfFromSin(PxReal sin)
	{
		return Ps::tanHalf(sin, PxSqrt(1.0f - sin*sin));
	}

	struct D6JointData : public JointData
	{
	//= ATTENTION! =====================================================================================
	// Changing the data layout of this class breaks the binary serialization format.  See comments for 
	// PX_BINARY_SERIAL_VERSION.  If a modification is required, please adjust the getBinaryMetaData 
	// function.  If the modification is made on a custom branch, please change PX_BINARY_SERIAL_VERSION
	// accordingly.
	//==================================================================================================

		PxD6Motion::Enum		motion[6]; 
		PxJointLinearLimit		linearLimit;
		PxJointAngularLimitPair	twistLimit;
		PxJointLimitCone		swingLimit;

		PxD6JointDrive			drive[PxD6Drive::eCOUNT];

		PxTransform				drivePosition;
		PxVec3					driveLinearVelocity;
		PxVec3					driveAngularVelocity;

		// derived quantities

		PxU32					locked;		// bitmap of locked DOFs
		PxU32					limited;	// bitmap of limited DOFs
		PxU32					driving;	// bitmap of active drives (implies driven DOFs not locked)

		// tan-half and tan-quarter angles

		PxReal					thSwingY;
		PxReal					thSwingZ;
		PxReal					thSwingPad;

		PxReal					tqSwingY;
		PxReal					tqSwingZ;
		PxReal					tqSwingPad;

		PxReal					tqTwistLow;
		PxReal					tqTwistHigh;
		PxReal					tqTwistPad;

		PxReal					linearMinDist;	// linear limit minimum distance to get a good direction

		// projection quantities
		PxReal					projectionLinearTolerance;
		PxReal					projectionAngularTolerance;

		// forestall compiler complaints about not being able to generate a constructor
	private:
		D6JointData(const PxJointLinearLimit& linear, const PxJointAngularLimitPair& twist, const PxJointLimitCone& swing):
			linearLimit(linear), twistLimit(twist), swingLimit(swing) {}
	};

	typedef Joint<PxD6Joint, PxD6JointGeneratedValues> D6JointT;
    
    class D6Joint : public Joint<PxD6Joint, PxD6JointGeneratedValues>
	{
	public:
// PX_SERIALIZATION
									D6Joint(PxBaseFlags baseFlags) : D6JointT(baseFlags) {}
		virtual		void			exportExtraData(PxSerializationContext& context);
					void			importExtraData(PxDeserializationContext& context);
					void			resolveReferences(PxDeserializationContext& context);
		static		D6Joint*		createObject(PxU8*& address, PxDeserializationContext& context);
		static		void			getBinaryMetaData(PxOutputStream& stream);
//~PX_SERIALIZATION
		virtual ~D6Joint()
		{
			if(getBaseFlags()&PxBaseFlag::eOWNS_MEMORY)
				PX_FREE(mData);
		}

		D6Joint(const PxTolerancesScale& scale,
				PxRigidActor* actor0, const PxTransform& localFrame0, 
				PxRigidActor* actor1, const PxTransform& localFrame1);


		PxReal				getTwist()							const;
		PxReal				getSwingYAngle()					const;
		PxReal				getSwingZAngle()					const;


		PxD6Motion::Enum	getMotion(PxD6Axis::Enum index)		const;
		void				setMotion(PxD6Axis::Enum index, PxD6Motion::Enum t);

		PxD6JointDrive		getDrive(PxD6Drive::Enum index)		const;
		void				setDrive(PxD6Drive::Enum index, const PxD6JointDrive& d);

		PxJointLinearLimit	getLinearLimit()					const;
		void				setLinearLimit(const PxJointLinearLimit& l);

		PxJointAngularLimitPair	getTwistLimit()					const;
		void				setTwistLimit(const PxJointAngularLimitPair& l);

		PxJointLimitCone	getSwingLimit()						const;
		void				setSwingLimit(const PxJointLimitCone& l);

		PxTransform			getDrivePosition()					const;
		void				setDrivePosition(const PxTransform& pose);

		void				getDriveVelocity(PxVec3& linear,
											 PxVec3& angular)	const;
																	
		void				setDriveVelocity(const PxVec3& linear,
											 const PxVec3& angular);

		void				setProjectionAngularTolerance(PxReal tolerance);
		PxReal				getProjectionAngularTolerance()					const;

		void				setProjectionLinearTolerance(PxReal tolerance);
		PxReal				getProjectionLinearTolerance()					const;

		void				visualize(PxRenderBuffer& out,
									  const void* constantBlock,
									  const PxTransform& body0Transform,
									  const PxTransform& body1Transform,
									  PxReal frameScale,
									  PxReal limitScale,
									  PxU32 flags)					const;

		bool				attach(PxPhysics &physics, PxRigidActor* actor0, PxRigidActor* actor1);

		static const PxConstraintShaderTable& getConstraintShaderTable() { return sShaders; }

		virtual PxConstraintSolverPrep getPrep() const { return sShaders.solverPrep; }

	private:

		static PxConstraintShaderTable sShaders;

		D6JointData& data() const				
		{	
			return *static_cast<D6JointData*>(mData);
		}

		bool active(const PxD6Drive::Enum index) const
		{
			PxD6JointDrive& d = data().drive[index];
			return d.stiffness!=0 || d.damping != 0;
		}

		void* prepareData();

		bool	mRecomputeMotion;
		bool	mRecomputeLimits;
		bool	mPadding[2];	// PT: padding from prev bools
	};

} // namespace Ext

namespace Ext
{
	extern "C" PxU32 D6JointSolverPrep(Px1DConstraint* constraints,
		PxVec3& body0WorldOffset,
		PxU32 maxConstraints,
		PxConstraintInvMassScale& invMassScale,
		const void* constantBlock,
		const PxTransform& bA2w,
		const PxTransform& bB2w);

	// global function to share the joint shaders with API capture	
	extern "C" const PxConstraintShaderTable* GetD6JointShaderTable();
}

}

#endif
