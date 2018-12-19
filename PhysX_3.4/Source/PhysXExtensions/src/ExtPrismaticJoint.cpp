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


#include "ExtPrismaticJoint.h"
#include "ExtConstraintHelper.h"
#include "CmRenderOutput.h"
#include "CmVisualization.h"

#include "common/PxSerialFramework.h"

using namespace physx;
using namespace Ext;

PxPrismaticJoint* physx::PxPrismaticJointCreate(PxPhysics& physics,
												PxRigidActor* actor0, const PxTransform& localFrame0,
												PxRigidActor* actor1, const PxTransform& localFrame1)
{
	PX_CHECK_AND_RETURN_NULL(localFrame0.isSane(), "PxPrismaticJointCreate: local frame 0 is not a valid transform"); 
	PX_CHECK_AND_RETURN_NULL(localFrame1.isSane(), "PxPrismaticJointCreate: local frame 1 is not a valid transform"); 
	PX_CHECK_AND_RETURN_NULL((actor0 && actor0->is<PxRigidBody>()) || (actor1 && actor1->is<PxRigidBody>()), "PxPrismaticJointCreate: at least one actor must be dynamic");
	PX_CHECK_AND_RETURN_NULL(actor0 != actor1, "PxPrismaticJointCreate: actors must be different");

	PrismaticJoint* j;
	PX_NEW_SERIALIZED(j,PrismaticJoint)(physics.getTolerancesScale(), actor0, localFrame0, actor1, localFrame1);

	if(j->attach(physics, actor0, actor1))
		return j;

	PX_DELETE(j);
	return NULL;
}

void PrismaticJoint::setProjectionAngularTolerance(PxReal tolerance)
{ 
	PX_CHECK_AND_RETURN(PxIsFinite(tolerance) && tolerance >=0 && tolerance <= PxPi, "PxPrismaticJoint::setProjectionAngularTolerance: invalid parameter");
	data().projectionAngularTolerance = tolerance;	
	markDirty();	
}

PxReal PrismaticJoint::getProjectionAngularTolerance() const
{ 
	return data().projectionAngularTolerance; 
}

void PrismaticJoint::setProjectionLinearTolerance(PxReal tolerance) 
{ 
	PX_CHECK_AND_RETURN(PxIsFinite(tolerance) && tolerance >=0, "PxPrismaticJoint::setProjectionLinearTolerance: invalid parameter");
	data().projectionLinearTolerance = tolerance;	
	markDirty(); 
}

PxReal PrismaticJoint::getProjectionLinearTolerance() const	
{ 
	return data().projectionLinearTolerance;		
}

PxPrismaticJointFlags PrismaticJoint::getPrismaticJointFlags(void) const
{ 
	return data().jointFlags;		
}

void PrismaticJoint::setPrismaticJointFlags(PxPrismaticJointFlags flags)
{ 
	data().jointFlags = flags; markDirty();	
}

void PrismaticJoint::setPrismaticJointFlag(PxPrismaticJointFlag::Enum flag, bool value)
{
	if(value)
		data().jointFlags |= flag;
	else
		data().jointFlags &= ~flag;
	markDirty();
}


PxJointLinearLimitPair PrismaticJoint::getLimit() const
{ 
	return data().limit;	
}

void PrismaticJoint::setLimit(const PxJointLinearLimitPair& limit)
{ 
	PX_CHECK_AND_RETURN(limit.isValid(), "PxPrismaticJoint::setLimit: invalid parameter");
	data().limit = limit;
	markDirty();
}



namespace
{


void PrismaticJointVisualize(PxConstraintVisualizer& viz,
							 const void* constantBlock,
							 const PxTransform& body0Transform,
							 const PxTransform& body1Transform,
							 PxU32 flags)
{
	const PrismaticJointData& data = *reinterpret_cast<const PrismaticJointData*>(constantBlock);

	const PxTransform& t0 = body0Transform * data.c2b[0];
	const PxTransform& t1 = body1Transform * data.c2b[1];

	if(flags & PxConstraintVisualizationFlag::eLOCAL_FRAMES)
		viz.visualizeJointFrames(t0, t1);

	if((flags & PxConstraintVisualizationFlag::eLIMITS) && (data.jointFlags & PxPrismaticJointFlag::eLIMIT_ENABLED))
	{
		const PxVec3 bOriginInA = t0.transformInv(t1.p);
		const PxReal ordinate = bOriginInA.x;

		viz.visualizeLinearLimit(t0, t1, data.limit.lower, ordinate < data.limit.lower + data.limit.contactDistance);
		viz.visualizeLinearLimit(t0, t1, data.limit.upper, ordinate > data.limit.upper - data.limit.contactDistance);
	}

}



void PrismaticJointProject(const void* constantBlock,
						   PxTransform& bodyAToWorld,
						   PxTransform& bodyBToWorld,
						   bool projectToA)
{
	using namespace joint;

	const PrismaticJointData& data = *reinterpret_cast<const PrismaticJointData*>(constantBlock);

	PxTransform cA2w, cB2w, cB2cA, projected;
	computeDerived(data, bodyAToWorld, bodyBToWorld, cA2w, cB2w, cB2cA);

	PxVec3 v(0,cB2cA.p.y,cB2cA.p.z);
	bool linearTrunc, angularTrunc;
	projected.p = truncateLinear(v, data.projectionLinearTolerance, linearTrunc);
	projected.q = truncateAngular(cB2cA.q, PxSin(data.projectionAngularTolerance/2), PxCos(data.projectionAngularTolerance/2), angularTrunc);
	
	if(linearTrunc || angularTrunc)
	{
		projected.p.x = cB2cA.p.x;
		projectTransforms(bodyAToWorld, bodyBToWorld, cA2w, cB2w, projected, data, projectToA);
	}
}
}

bool Ext::PrismaticJoint::attach(PxPhysics &physics, PxRigidActor* actor0, PxRigidActor* actor1)
{
	mPxConstraint = physics.createConstraint(actor0, actor1, *this, sShaders, sizeof(PrismaticJointData));
	return mPxConstraint!=NULL;
}

void PrismaticJoint::exportExtraData(PxSerializationContext& stream)
{
	if(mData)
	{
		stream.alignData(PX_SERIAL_ALIGN);
		stream.writeData(mData, sizeof(PrismaticJointData));
	}
	stream.writeName(mName);
}

void PrismaticJoint::importExtraData(PxDeserializationContext& context)
{
	if(mData)
		mData = context.readExtraData<PrismaticJointData, PX_SERIAL_ALIGN>();
	context.readName(mName);
}

void PrismaticJoint::resolveReferences(PxDeserializationContext& context)
{
	setPxConstraint(resolveConstraintPtr(context, getPxConstraint(), getConnector(), sShaders));	
}

PrismaticJoint* PrismaticJoint::createObject(PxU8*& address, PxDeserializationContext& context)
{
	PrismaticJoint* obj = new (address) PrismaticJoint(PxBaseFlag::eIS_RELEASABLE);
	address += sizeof(PrismaticJoint);	
	obj->importExtraData(context);
	obj->resolveReferences(context);
	return obj;
}

// global function to share the joint shaders with API capture	
const PxConstraintShaderTable* Ext::GetPrismaticJointShaderTable() 
{ 
	return &PrismaticJoint::getConstraintShaderTable();
}

//~PX_SERIALIZATION
PxConstraintShaderTable Ext::PrismaticJoint::sShaders = { Ext::PrismaticJointSolverPrep, PrismaticJointProject, PrismaticJointVisualize, PxConstraintFlag::Enum(0) };


