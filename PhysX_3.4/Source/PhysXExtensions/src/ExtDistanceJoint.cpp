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


#include "ExtDistanceJoint.h"
#include "common/PxSerialFramework.h"

using namespace physx;
using namespace Ext;

namespace physx
{
	PxDistanceJoint* PxDistanceJointCreate(PxPhysics& physics,
		PxRigidActor* actor0, const PxTransform& localFrame0,
		PxRigidActor* actor1, const PxTransform& localFrame1);
}

PxDistanceJoint* physx::PxDistanceJointCreate(PxPhysics& physics,
											  PxRigidActor* actor0, const PxTransform& localFrame0,
											  PxRigidActor* actor1, const PxTransform& localFrame1)
{
	PX_CHECK_AND_RETURN_NULL(localFrame0.isSane(), "PxDistanceJointCreate: local frame 0 is not a valid transform"); 
	PX_CHECK_AND_RETURN_NULL(localFrame1.isSane(), "PxDistanceJointCreate: local frame 1 is not a valid transform"); 
	PX_CHECK_AND_RETURN_NULL(actor0 != actor1, "PxDistanceJointCreate: actors must be different");
	PX_CHECK_AND_RETURN_NULL((actor0 && actor0->is<PxRigidBody>()) || (actor1 && actor1->is<PxRigidBody>()), "PxD6JointCreate: at least one actor must be dynamic");

	DistanceJoint* j;
	PX_NEW_SERIALIZED(j,DistanceJoint)(physics.getTolerancesScale(), actor0, localFrame0, actor1, localFrame1);
	if(j->attach(physics, actor0, actor1))
		return j;

	PX_DELETE(j);
	return NULL;
}

PxReal DistanceJoint::getDistance() const
{
	return getRelativeTransform().p.magnitudeSquared();
}

void DistanceJoint::setMinDistance(PxReal distance)	
{ 
	PX_CHECK_AND_RETURN(PxIsFinite(distance), "PxDistanceJoint::setMinDistance: invalid parameter");
	data().minDistance = distance;
	markDirty();
}

PxReal DistanceJoint::getMinDistance() const
{ 
	return data().minDistance;		
}

void DistanceJoint::setMaxDistance(PxReal distance)	
{ 
	PX_CHECK_AND_RETURN(PxIsFinite(distance), "PxDistanceJoint::setMaxDistance: invalid parameter");
	data().maxDistance = distance;
	markDirty();
}

PxReal DistanceJoint::getMaxDistance() const	
{ 
	return data().maxDistance;			
}

void DistanceJoint::setTolerance(PxReal tolerance) 
{ 
	PX_CHECK_AND_RETURN(PxIsFinite(tolerance), "PxDistanceJoint::setTolerance: invalid parameter");
	data().tolerance = tolerance;
	markDirty();
}

PxReal DistanceJoint::getTolerance() const
{ 
	return data().tolerance;			
}

void DistanceJoint::setStiffness(PxReal stiffness)
{ 
	PX_CHECK_AND_RETURN(PxIsFinite(stiffness), "PxDistanceJoint::setStiffness: invalid parameter");
	data().stiffness = stiffness;
	markDirty();
}

PxReal DistanceJoint::getStiffness() const	
{ 
	return data().stiffness;
}

void DistanceJoint::setDamping(PxReal damping)
{ 
	PX_CHECK_AND_RETURN(PxIsFinite(damping), "PxDistanceJoint::setDamping: invalid parameter");
	data().damping = damping;
	markDirty();	
}

PxReal DistanceJoint::getDamping() const
{ 
	return data().damping;
}

PxDistanceJointFlags DistanceJoint::getDistanceJointFlags(void) const
{ 
	return data().jointFlags;		
}

void DistanceJoint::setDistanceJointFlags(PxDistanceJointFlags flags) 
{ 
	data().jointFlags = flags; 
	markDirty();	
}

void DistanceJoint::setDistanceJointFlag(PxDistanceJointFlag::Enum flag, bool value)
{
	if(value)
		data().jointFlags |= flag;
	else
		data().jointFlags &= ~flag;
	markDirty();
}



namespace
{
static void DistanceJointVisualize(PxConstraintVisualizer& viz, const void* constantBlock, const PxTransform& body0Transform, const PxTransform& body1Transform, PxU32 flags)
{
	const DistanceJointData& data = *reinterpret_cast<const DistanceJointData*>(constantBlock);

	const PxTransform cA2w = body0Transform.transform(data.c2b[0]);
	const PxTransform cB2w = body1Transform.transform(data.c2b[1]);

	if(flags & PxConstraintVisualizationFlag::eLOCAL_FRAMES)
		viz.visualizeJointFrames(cA2w, cB2w);

	// PT: we consider the following is part of the joint's "limits" since that's the only available flag we have
	if(flags & PxConstraintVisualizationFlag::eLIMITS)
	{
		const bool enforceMax = (data.jointFlags & PxDistanceJointFlag::eMAX_DISTANCE_ENABLED);
		const bool enforceMin = (data.jointFlags & PxDistanceJointFlag::eMIN_DISTANCE_ENABLED);
		if(!enforceMin && !enforceMax)
			return;

		PxVec3 dir = cB2w.p - cA2w.p;
		const float currentDist = dir.normalize();

		PxU32 color = 0x00ff00;
		if(enforceMax && currentDist>data.maxDistance)
			color = 0xff0000;
		if(enforceMin && currentDist<data.minDistance)
			color = 0x0000ff;

		viz.visualizeLine(cA2w.p, cB2w.p, color);
	}
}

void DistanceJointProject(const void* /*constantBlock*/,
						  PxTransform& /*bodyAToWorld*/,
						  PxTransform& /*bodyBToWorld*/,
						  bool /*projectToA*/)
{
	// TODO
}


}

bool Ext::DistanceJoint::attach(PxPhysics &physics, PxRigidActor* actor0, PxRigidActor* actor1)
{
	mPxConstraint = physics.createConstraint(actor0, actor1, *this, sShaders, sizeof(DistanceJointData));
	return mPxConstraint!=NULL;
}

void DistanceJoint::exportExtraData(PxSerializationContext& stream)
{
	if(mData)
	{
		stream.alignData(PX_SERIAL_ALIGN);
		stream.writeData(mData, sizeof(DistanceJointData));
	}
	stream.writeName(mName);
}

void DistanceJoint::importExtraData(PxDeserializationContext& context)
{
	if(mData)
		mData = context.readExtraData<DistanceJointData, PX_SERIAL_ALIGN>();

	context.readName(mName);
}

void DistanceJoint::resolveReferences(PxDeserializationContext& context)
{
	setPxConstraint(resolveConstraintPtr(context, getPxConstraint(), getConnector(), sShaders));	
}

DistanceJoint* DistanceJoint::createObject(PxU8*& address, PxDeserializationContext& context)
{
	DistanceJoint* obj = new (address) DistanceJoint(PxBaseFlag::eIS_RELEASABLE);
	address += sizeof(DistanceJoint);	
	obj->importExtraData(context);
	obj->resolveReferences(context);
	return obj;
}

// global function to share the joint shaders with API capture	
const PxConstraintShaderTable* Ext::GetDistanceJointShaderTable() 
{ 
	return &DistanceJoint::getConstraintShaderTable();
}

//~PX_SERIALIZATION
PxConstraintShaderTable Ext::DistanceJoint::sShaders = { Ext::DistanceJointSolverPrep, DistanceJointProject, DistanceJointVisualize, PxConstraintFlag::Enum(0) };
