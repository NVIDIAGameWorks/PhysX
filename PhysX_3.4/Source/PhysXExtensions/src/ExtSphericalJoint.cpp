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
#include "CmVisualization.h"

#include "common/PxSerialFramework.h"

using namespace physx;
using namespace Ext;

namespace physx
{
	PxSphericalJoint* PxSphericalJointCreate(PxPhysics& physics,
		PxRigidActor* actor0, const PxTransform& localFrame0,
		PxRigidActor* actor1, const PxTransform& localFrame1);
}

PxSphericalJoint* physx::PxSphericalJointCreate(PxPhysics& physics,
												PxRigidActor* actor0, const PxTransform& localFrame0,
												PxRigidActor* actor1, const PxTransform& localFrame1)
{
	PX_CHECK_AND_RETURN_NULL(localFrame0.isSane(), "PxSphericalJointCreate: local frame 0 is not a valid transform"); 
	PX_CHECK_AND_RETURN_NULL(localFrame1.isSane(), "PxSphericalJointCreate: local frame 1 is not a valid transform"); 
	PX_CHECK_AND_RETURN_NULL(actor0 != actor1, "PxSphericalJointCreate: actors must be different");
	PX_CHECK_AND_RETURN_NULL((actor0 && actor0->is<PxRigidBody>()) || (actor1 && actor1->is<PxRigidBody>()), "PxSphericalJointCreate: at least one actor must be dynamic");

	SphericalJoint* j;
	PX_NEW_SERIALIZED(j,SphericalJoint)(physics.getTolerancesScale(), actor0, localFrame0, actor1, localFrame1);

	if(j->attach(physics, actor0, actor1))
		return j;

	PX_DELETE(j);
	return NULL;
}

void SphericalJoint::setProjectionLinearTolerance(PxReal tolerance)
{	
	PX_CHECK_AND_RETURN(PxIsFinite(tolerance) && tolerance >=0, "PxSphericalJoint::setProjectionLinearTolerance: invalid parameter");
	data().projectionLinearTolerance = tolerance;
	markDirty(); 
}

PxReal SphericalJoint::getProjectionLinearTolerance() const	
{	
	return data().projectionLinearTolerance;		
}

void SphericalJoint::setLimitCone(const PxJointLimitCone &limit)
{	
	PX_CHECK_AND_RETURN(limit.isValid(), "PxSphericalJoint::setLimit: invalid parameter");
	data().limit = limit; 
	markDirty();
}

PxJointLimitCone SphericalJoint::getLimitCone() const
{	
	return data().limit; 
}


PxSphericalJointFlags SphericalJoint::getSphericalJointFlags(void) const
{ 
	return data().jointFlags; 
}

void SphericalJoint::setSphericalJointFlags(PxSphericalJointFlags flags)
{ 
	data().jointFlags = flags; 
}
void SphericalJoint::setSphericalJointFlag(PxSphericalJointFlag::Enum flag, bool value)
{
	if(value)
		data().jointFlags |= flag;
	else
		data().jointFlags &= ~flag;
	markDirty();
}


void* Ext::SphericalJoint::prepareData()
{
	SphericalJointData& d = data();
	if(d.jointFlags & PxSphericalJointFlag::eLIMIT_ENABLED)
	{
		d.tanQYLimit = PxTan(d.limit.yAngle/4);
		d.tanQZLimit = PxTan(d.limit.zAngle/4);
		d.tanQPad	 = PxTan(d.limit.contactDistance/4);
	}
	return SphericalJointT::prepareData();
}

namespace
{

void SphericalJointVisualize(PxConstraintVisualizer& viz,
							 const void* constantBlock,
							 const PxTransform& body0Transform,
							 const PxTransform& body1Transform,
							 PxU32 flags)
{
	using namespace joint;
	const SphericalJointData& data = *reinterpret_cast<const SphericalJointData*>(constantBlock);

	PxTransform cA2w = body0Transform * data.c2b[0];
	PxTransform cB2w = body1Transform * data.c2b[1];

	if(flags & PxConstraintVisualizationFlag::eLOCAL_FRAMES)
		viz.visualizeJointFrames(cA2w, cB2w);

	if((flags & PxConstraintVisualizationFlag::eLIMITS) && (data.jointFlags & PxSphericalJointFlag::eLIMIT_ENABLED))
	{
		if(cA2w.q.dot(cB2w.q)<0)
			cB2w.q = -cB2w.q;

		PxTransform cB2cA = cA2w.transformInv(cB2w);	
		PxQuat swing, twist;
		Ps::separateSwingTwist(cB2cA.q,swing,twist);

		PxVec3 tanQSwing = PxVec3(0, Ps::tanHalf(swing.z,swing.w), -Ps::tanHalf(swing.y,swing.w));
		const PxReal pad = data.limit.isSoft() ? 0.0f : data.tanQPad;
		Cm::ConeLimitHelper coneHelper(data.tanQZLimit, data.tanQYLimit, pad);
		viz.visualizeLimitCone(cA2w, data.tanQZLimit, data.tanQYLimit, 
			!coneHelper.contains(tanQSwing));
	}
}



void SphericalJointProject(const void* constantBlock,
						   PxTransform& bodyAToWorld,
						   PxTransform& bodyBToWorld,
						   bool projectToA)
{
	using namespace joint;

	const SphericalJointData& data = *reinterpret_cast<const SphericalJointData*>(constantBlock);

	PxTransform cA2w, cB2w, cB2cA, projected;
	joint::computeDerived(data, bodyAToWorld, bodyBToWorld, cA2w, cB2w, cB2cA);

	bool linearTrunc;
	projected.p = truncateLinear(cB2cA.p, data.projectionLinearTolerance, linearTrunc);

	if(linearTrunc)
	{
		projected.q = cB2cA.q;
		projectTransforms(bodyAToWorld, bodyBToWorld, cA2w, cB2w, projected, data, projectToA);
	}
}
}


bool Ext::SphericalJoint::attach(PxPhysics &physics, PxRigidActor* actor0, PxRigidActor* actor1)
{
	mPxConstraint = physics.createConstraint(actor0, actor1, *this, sShaders, sizeof(SphericalJointData));
	return mPxConstraint!=NULL;
}

void SphericalJoint::exportExtraData(PxSerializationContext& stream)
{
	if(mData)
	{
		stream.alignData(PX_SERIAL_ALIGN);
		stream.writeData(mData, sizeof(SphericalJointData));
	}
	stream.writeName(mName);
}

void SphericalJoint::importExtraData(PxDeserializationContext& context)
{
	if(mData)
		mData = context.readExtraData<SphericalJointData, PX_SERIAL_ALIGN>();
	context.readName(mName);
}

void SphericalJoint::resolveReferences(PxDeserializationContext& context)
{
	setPxConstraint(resolveConstraintPtr(context, getPxConstraint(), getConnector(), sShaders));	
}

SphericalJoint* SphericalJoint::createObject(PxU8*& address, PxDeserializationContext& context)
{
	SphericalJoint* obj = new (address) SphericalJoint(PxBaseFlag::eIS_RELEASABLE);
	address += sizeof(SphericalJoint);	
	obj->importExtraData(context);
	obj->resolveReferences(context);
	return obj;
}

// global function to share the joint shaders with API capture	
const PxConstraintShaderTable* Ext::GetSphericalJointShaderTable() 
{ 
	return &SphericalJoint::getConstraintShaderTable();
}

//~PX_SERIALIZATION
PxConstraintShaderTable Ext::SphericalJoint::sShaders = { Ext::SphericalJointSolverPrep, SphericalJointProject, SphericalJointVisualize, PxConstraintFlag::Enum(0) };
