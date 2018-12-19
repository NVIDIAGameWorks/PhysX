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

#ifndef PX_PHYSICS_SCB_ARTICULATION_JOINT
#define PX_PHYSICS_SCB_ARTICULATION_JOINT

#include "ScArticulationJointCore.h"
#include "ScbBody.h"
#include "ScbBase.h"
#include "ScbDefs.h"

namespace physx
{
namespace Scb
{

struct ArticulationJointBuffer
{	
	template <PxU32 I, PxU32 Dummy> struct Fns {};
	typedef Sc::ArticulationJointCore Core;
	typedef ArticulationJointBuffer Buf;

	SCB_REGULAR_ATTRIBUTE(0,  PxTransform,	ParentPose)
	SCB_REGULAR_ATTRIBUTE(1,  PxTransform,	ChildPose)
	SCB_REGULAR_ATTRIBUTE(2,  PxQuat,		TargetOrientation)
	SCB_REGULAR_ATTRIBUTE(3,  PxVec3,		TargetVelocity)
	SCB_REGULAR_ATTRIBUTE(4,  PxReal,		Stiffness)
	SCB_REGULAR_ATTRIBUTE(5,  PxReal,		Damping)
	SCB_REGULAR_ATTRIBUTE(6,  PxReal,		InternalCompliance)
	SCB_REGULAR_ATTRIBUTE(7,  PxReal,		ExternalCompliance)
	SCB_REGULAR_ATTRIBUTE(8,  PxReal,		SwingLimitContactDistance)
	SCB_REGULAR_ATTRIBUTE(9,  bool,			SwingLimitEnabled)
	SCB_REGULAR_ATTRIBUTE(10, PxReal,		TangentialStiffness)
	SCB_REGULAR_ATTRIBUTE(11, PxReal,		TangentialDamping)
	SCB_REGULAR_ATTRIBUTE(12, PxReal,		TwistLimitContactDistance)
	SCB_REGULAR_ATTRIBUTE(13, bool,			TwistLimitEnabled)
	SCB_REGULAR_ATTRIBUTE(14, PxArticulationJointDriveType::Enum, DriveType)

	enum	{ BF_SwingLimit = 1<<15 };
	enum	{ BF_TwistLimit = 1<<16 };

	PxReal				mSwingLimitY;
	PxReal				mSwingLimitZ;
	PxReal				mTwistLimitLower;
	PxReal				mTwistLimitUpper;
};
	
class ArticulationJoint : public Base
{
//= ATTENTION! =====================================================================================
// Changing the data layout of this class breaks the binary serialization format.  See comments for 
// PX_BINARY_SERIAL_VERSION.  If a modification is required, please adjust the getBinaryMetaData 
// function.  If the modification is made on a custom branch, please change PX_BINARY_SERIAL_VERSION
// accordingly.
//==================================================================================================

	typedef ArticulationJointBuffer Buf;
	typedef Sc::ArticulationJointCore Core;

public:
// PX_SERIALIZATION
										ArticulationJoint(const PxEMPTY) : Base(PxEmpty), mJoint(PxEmpty)	{}
	static		void					getBinaryMetaData(PxOutputStream& stream);
//~PX_SERIALIZATION

	PX_INLINE							ArticulationJoint(const PxTransform& parentFrame,
														  const PxTransform& childFrame);
	PX_INLINE							~ArticulationJoint();

	//---------------------------------------------------------------------------------
	// Wrapper for Sc::ArticulationJoint interface
	//---------------------------------------------------------------------------------

	PX_INLINE PxTransform				getParentPose() const					{ return read<Buf::BF_ParentPose>(); }
	PX_INLINE void						setParentPose(const PxTransform& v)		{ write<Buf::BF_ParentPose>(v); }

	PX_INLINE PxTransform				getChildPose() const					{ return read<Buf::BF_ChildPose>(); }
	PX_INLINE void						setChildPose(const PxTransform& v)		{ write<Buf::BF_ChildPose>(v); }

	PX_INLINE PxQuat					getTargetOrientation() const			{ return read<Buf::BF_TargetOrientation>(); }
	PX_INLINE void						setTargetOrientation(const PxQuat& v)	{ write<Buf::BF_TargetOrientation>(v); }

	PX_INLINE PxVec3					getTargetVelocity() const				{ return read<Buf::BF_TargetVelocity>(); }
	PX_INLINE void						setTargetVelocity(const PxVec3& v)		{ write<Buf::BF_TargetVelocity>(v); }

	PX_INLINE PxReal					getStiffness() const					{ return read<Buf::BF_Stiffness>(); }
	PX_INLINE void						setStiffness(PxReal v)					{ write<Buf::BF_Stiffness>(v); }

	PX_INLINE PxReal					getDamping() const						{ return read<Buf::BF_Damping>(); }
	PX_INLINE void						setDamping(PxReal v)					{ write<Buf::BF_Damping>(v); }

	PX_INLINE PxReal					getInternalCompliance() const			{ return read<Buf::BF_InternalCompliance>(); }
	PX_INLINE void						setInternalCompliance(PxReal v)			{ write<Buf::BF_InternalCompliance>(v); }

	PX_INLINE PxReal					getExternalCompliance() const			{ return read<Buf::BF_ExternalCompliance>(); }
	PX_INLINE void						setExternalCompliance(PxReal v)			{ write<Buf::BF_ExternalCompliance>(v); }

	PX_INLINE PxReal					getTangentialStiffness() const 			{ return read<Buf::BF_TangentialStiffness>(); }
	PX_INLINE void						setTangentialStiffness(PxReal v)		{ write<Buf::BF_TangentialStiffness>(v); }

	PX_INLINE PxReal					getTangentialDamping() const			{ return read<Buf::BF_TangentialDamping>(); }
	PX_INLINE void						setTangentialDamping(PxReal v)			{ write<Buf::BF_TangentialDamping>(v); }

	PX_INLINE PxReal					getSwingLimitContactDistance() const	{ return read<Buf::BF_SwingLimitContactDistance>(); }
	PX_INLINE void						setSwingLimitContactDistance(PxReal v)	{ write<Buf::BF_SwingLimitContactDistance>(v); }

	PX_INLINE PxReal					getTwistLimitContactDistance() const	{ return read<Buf::BF_TwistLimitContactDistance>(); }
	PX_INLINE void						setTwistLimitContactDistance(PxReal v)  { write<Buf::BF_TwistLimitContactDistance>(v); }

	PX_INLINE PxArticulationJointDriveType::Enum
										getDriveType() const					{ return read<Buf::BF_DriveType>(); }
	PX_INLINE void						setDriveType(PxArticulationJointDriveType::Enum v) 
																				{ write<Buf::BF_DriveType>(v); }

	PX_INLINE bool						getSwingLimitEnabled() const			{ return read<Buf::BF_SwingLimitEnabled>(); }
	PX_INLINE void						setSwingLimitEnabled(bool v)			{ write<Buf::BF_SwingLimitEnabled>(v); }

	PX_INLINE bool						getTwistLimitEnabled() const			{ return read<Buf::BF_TwistLimitEnabled>(); }
	PX_INLINE void						setTwistLimitEnabled(bool v)			{ write<Buf::BF_TwistLimitEnabled>(v); }

	PX_INLINE void						getSwingLimit(PxReal& yLimit, PxReal& zLimit) const;
	PX_INLINE void						setSwingLimit(PxReal yLimit, PxReal zLimit);

	PX_INLINE void						getTwistLimit(PxReal &lower, PxReal &upper) const;
	PX_INLINE void						setTwistLimit(PxReal lower, PxReal upper);

	//---------------------------------------------------------------------------------
	// Data synchronization
	//---------------------------------------------------------------------------------
	PX_INLINE void						syncState();

	PX_FORCE_INLINE const Core&			getScArticulationJoint()		const	{ return mJoint; }  // Only use if you know what you're doing!
	PX_FORCE_INLINE Core&				getScArticulationJoint()				{ return mJoint; }  // Only use if you know what you're doing!

private:
	Core mJoint;

	PX_FORCE_INLINE	const Buf*			getBuffer()	const	{ return reinterpret_cast<const Buf*>(getStream()); }
	PX_FORCE_INLINE	Buf*				getBuffer()			{ return reinterpret_cast<Buf*>(getStream()); }

	//---------------------------------------------------------------------------------
	// Infrastructure for regular attributes
	//---------------------------------------------------------------------------------

	struct Access: public BufferedAccess<Buf, Core, ArticulationJoint> {};
	template<PxU32 f> PX_FORCE_INLINE typename Buf::Fns<f,0>::Arg read() const		{	return Access::read<Buf::Fns<f,0> >(*this, mJoint);	}
	template<PxU32 f> PX_FORCE_INLINE void write(typename Buf::Fns<f,0>::Arg v)		{	Access::write<Buf::Fns<f,0> >(*this, mJoint, v);	}
	template<PxU32 f> PX_FORCE_INLINE void flush(const Buf& buf)					{	Access::flush<Buf::Fns<f,0> >(*this, mJoint, buf);	}
};

ArticulationJoint::ArticulationJoint(const PxTransform& parentFrame, const PxTransform& childFrame) :
	mJoint(parentFrame, childFrame)
{
	setScbType(ScbType::eARTICULATION_JOINT);
}

ArticulationJoint::~ArticulationJoint()
{
}

PX_INLINE void ArticulationJoint::getSwingLimit(PxReal &yLimit, PxReal &zLimit) const
{
	if(isBuffered(Buf::BF_SwingLimit))
	{
		yLimit = getBuffer()->mSwingLimitY;
		zLimit = getBuffer()->mSwingLimitZ;
	}
	else
		mJoint.getSwingLimit(yLimit, zLimit);
}

PX_INLINE void ArticulationJoint::setSwingLimit(PxReal yLimit, PxReal zLimit)
{
	if(!isBuffering())
		mJoint.setSwingLimit(yLimit, zLimit);
	else
	{
		getBuffer()->mSwingLimitY = yLimit;
		getBuffer()->mSwingLimitZ = zLimit;
		markUpdated(Buf::BF_SwingLimit);
	}
}

PX_INLINE void ArticulationJoint::getTwistLimit(PxReal &lower, PxReal &upper) const
{
	if(isBuffered(Buf::BF_TwistLimit))
	{
		lower = getBuffer()->mTwistLimitLower;
		upper = getBuffer()->mTwistLimitUpper;
	}
	else
		mJoint.getTwistLimit(lower, upper);
}

PX_INLINE void ArticulationJoint::setTwistLimit(PxReal lower, PxReal upper)
{
	if(!isBuffering())
		mJoint.setTwistLimit(lower, upper);
	else
	{
		getBuffer()->mTwistLimitLower = lower;
		getBuffer()->mTwistLimitUpper = upper;
		markUpdated(Buf::BF_TwistLimit);
	}
}

//--------------------------------------------------------------
//
// Data synchronization
//
//--------------------------------------------------------------

PX_INLINE void ArticulationJoint::syncState()
{
	const PxU32 flags = getBufferFlags();
	if(flags)  // Optimization to avoid all the if-statements below if possible
	{
		const Buf& buffer = *getBuffer();

		flush<Buf::BF_ParentPose>(buffer);
		flush<Buf::BF_ChildPose>(buffer);
		flush<Buf::BF_TargetOrientation>(buffer);
		flush<Buf::BF_TargetVelocity>(buffer);
		flush<Buf::BF_Stiffness>(buffer);
		flush<Buf::BF_Damping>(buffer);
		flush<Buf::BF_InternalCompliance>(buffer);
		flush<Buf::BF_ExternalCompliance>(buffer);
		flush<Buf::BF_SwingLimitContactDistance>(buffer);
		flush<Buf::BF_SwingLimitEnabled>(buffer);
		flush<Buf::BF_TwistLimitContactDistance>(buffer);
		flush<Buf::BF_TwistLimitEnabled>(buffer);
		flush<Buf::BF_TangentialStiffness>(buffer);
		flush<Buf::BF_TangentialDamping>(buffer);
		flush<Buf::BF_DriveType>(buffer);

		if(isBuffered(Buf::BF_SwingLimit))
			mJoint.setSwingLimit(buffer.mSwingLimitY, buffer.mSwingLimitZ);

		if(isBuffered(Buf::BF_TwistLimit))
			mJoint.setTwistLimit(buffer.mTwistLimitLower, buffer.mTwistLimitUpper);
	}

	postSyncState();
}

}  // namespace Scb

}

#endif
