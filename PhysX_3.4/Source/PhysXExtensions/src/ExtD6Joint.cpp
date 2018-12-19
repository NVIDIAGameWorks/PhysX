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


#include "ExtD6Joint.h"
#include "ExtConstraintHelper.h"
#include "CmRenderOutput.h"
#include "CmConeLimitHelper.h"
#include "PxTolerancesScale.h"
#include "CmUtils.h"
#include "PxConstraint.h"

#include "common/PxSerialFramework.h"

using namespace physx;
using namespace Ext;

PxD6Joint* physx::PxD6JointCreate(PxPhysics& physics,
								  PxRigidActor* actor0, const PxTransform& localFrame0,
								  PxRigidActor* actor1, const PxTransform& localFrame1)
{
	PX_CHECK_AND_RETURN_NULL(localFrame0.isSane(), "PxD6JointCreate: local frame 0 is not a valid transform"); 
	PX_CHECK_AND_RETURN_NULL(localFrame1.isSane(), "PxD6JointCreate: local frame 1 is not a valid transform"); 
	PX_CHECK_AND_RETURN_NULL(actor0 != actor1, "PxD6JointCreate: actors must be different");
	PX_CHECK_AND_RETURN_NULL((actor0 && actor0->is<PxRigidBody>()) || (actor1 && actor1->is<PxRigidBody>()), "PxD6JointCreate: at least one actor must be dynamic");

	D6Joint *j;
	PX_NEW_SERIALIZED(j,D6Joint)(physics.getTolerancesScale(), actor0, localFrame0, actor1, localFrame1);
	if(j->attach(physics, actor0, actor1))
		return j;

	PX_DELETE(j);
	return NULL;
}



D6Joint::D6Joint(const PxTolerancesScale& scale,
				 PxRigidActor* actor0, const PxTransform& localFrame0, 
				 PxRigidActor* actor1, const PxTransform& localFrame1)
:	D6JointT(PxJointConcreteType::eD6, PxBaseFlag::eOWNS_MEMORY | PxBaseFlag::eIS_RELEASABLE)	
,	mRecomputeMotion(true)
,	mRecomputeLimits(true)
{
	D6JointData* data = reinterpret_cast<D6JointData*>(PX_ALLOC(sizeof(D6JointData), "D6JointData"));
	Cm::markSerializedMem(data, sizeof(D6JointData));
	mData = data;

	initCommonData(*data,actor0, localFrame0, actor1, localFrame1);
	for(PxU32 i=0;i<6;i++)
		data->motion[i] = PxD6Motion::eLOCKED;

	data->twistLimit = PxJointAngularLimitPair(-PxPi/2, PxPi/2);
	data->swingLimit = PxJointLimitCone(PxPi/2, PxPi/2);
	data->linearLimit = PxJointLinearLimit(scale, PX_MAX_F32);
	data->linearMinDist = 1e-6f*scale.length;

	for(PxU32 i=0;i<PxD6Drive::eCOUNT;i++)
		data->drive[i] = PxD6JointDrive();

	data->drivePosition = PxTransform(PxIdentity);
	data->driveLinearVelocity = PxVec3(0);
	data->driveAngularVelocity = PxVec3(0);

	data->projectionLinearTolerance = 1e10;
	data->projectionAngularTolerance = PxPi;
}

PxD6Motion::Enum D6Joint::getMotion(PxD6Axis::Enum index) const
{	
	return data().motion[index];	
}

void D6Joint::setMotion(PxD6Axis::Enum index, PxD6Motion::Enum t)
{	
	data().motion[index] = t; 
	mRecomputeMotion = true; 
	markDirty(); 
}

PxReal D6Joint::getTwist() const
{
	PxQuat q = getRelativeTransform().q, swing, twist;
	Ps::separateSwingTwist(q, swing, twist);
	if (twist.x < 0)
		twist = -twist;
	PxReal angle = twist.getAngle();				// angle is in range [0, 2pi]
	return angle < PxPi ? angle : angle - PxTwoPi;
}

PxReal D6Joint::getSwingYAngle()	const
{
	PxQuat q = getRelativeTransform().q, swing, twist;
	Ps::separateSwingTwist(q, swing, twist);
	if (swing.w < 0)		// choose the shortest rotation
		swing = -swing;

	PxReal angle = 4 * PxAtan2(swing.y, 1 + swing.w);	// tan (t/2) = sin(t)/(1+cos t), so this is the quarter angle
	PX_ASSERT(angle>-PxPi && angle<=PxPi);				// since |y| < w+1, the atan magnitude is < PI/4
	return angle;
}

PxReal D6Joint::getSwingZAngle()	const
{
	PxQuat q = getRelativeTransform().q, swing, twist;
	Ps::separateSwingTwist(q, swing, twist);
	if (swing.w < 0)		// choose the shortest rotation
		swing = -swing;

	PxReal angle = 4 * PxAtan2(swing.z, 1 + swing.w);	// tan (t/2) = sin(t)/(1+cos t), so this is the quarter angle
	PX_ASSERT(angle>-PxPi && angle <= PxPi);			// since |y| < w+1, the atan magnitude is < PI/4
	return angle;
}


PxD6JointDrive D6Joint::getDrive(PxD6Drive::Enum index) const
{	
	return data().drive[index];	
}

void D6Joint::setDrive(PxD6Drive::Enum index, const PxD6JointDrive& d)
{	
	PX_CHECK_AND_RETURN(d.isValid(), "PxD6Joint::setDrive: drive is invalid"); 

	data().drive[index] = d; 
	mRecomputeMotion = true; 
	markDirty(); 
}

PxJointLinearLimit D6Joint::getLinearLimit() const
{	

	return data().linearLimit;	
}

void D6Joint::setLinearLimit(const PxJointLinearLimit& l)
{	
	PX_CHECK_AND_RETURN(l.isValid(), "PxD6Joint::setLinearLimit: limit invalid");
	data().linearLimit = l; 
	mRecomputeLimits = true; 
	markDirty(); 
}

PxJointAngularLimitPair D6Joint::getTwistLimit() const
{	
	return data().twistLimit;	
}

void D6Joint::setTwistLimit(const PxJointAngularLimitPair& l)
{	
	PX_CHECK_AND_RETURN(l.isValid(), "PxD6Joint::setTwistLimit: limit invalid");
	PX_CHECK_AND_RETURN(l.lower>-PxTwoPi && l.upper<PxTwoPi , "PxD6Joint::twist limit must be strictly -2*PI and 2*PI");
	PX_CHECK_AND_RETURN(l.upper - l.lower < PxTwoPi, "PxD6Joint::twist limit range must be strictly less than 2*PI");

	data().twistLimit = l; 
	mRecomputeLimits = true; 
	markDirty(); 
}

PxJointLimitCone D6Joint::getSwingLimit() const
{	
	return data().swingLimit;	
}

void D6Joint::setSwingLimit(const PxJointLimitCone& l)
{	
	PX_CHECK_AND_RETURN(l.isValid(), "PxD6Joint::setSwingLimit: limit invalid");

	data().swingLimit = l; 
	mRecomputeLimits = true; 
	markDirty(); 
}

PxTransform D6Joint::getDrivePosition() const
{	
	return data().drivePosition;	
}

void D6Joint::setDrivePosition(const PxTransform& pose)
{	
	PX_CHECK_AND_RETURN(pose.isSane(), "PxD6Joint::setDrivePosition: pose invalid");
	data().drivePosition = pose.getNormalized(); 
	markDirty(); 
}

void D6Joint::getDriveVelocity(PxVec3& linear, PxVec3& angular)	const
{	
	linear = data().driveLinearVelocity;
	angular = data().driveAngularVelocity; 
}

void D6Joint::setDriveVelocity(const PxVec3& linear,
									 const PxVec3& angular)
{	
	PX_CHECK_AND_RETURN(linear.isFinite() && angular.isFinite(), "PxD6Joint::setDriveVelocity: velocity invalid");
	data().driveLinearVelocity = linear; 
	data().driveAngularVelocity = angular; 
	markDirty();
}

void D6Joint::setProjectionAngularTolerance(PxReal tolerance)
{	
	PX_CHECK_AND_RETURN(PxIsFinite(tolerance) && tolerance >=0 && tolerance <= PxPi, "PxD6Joint::setProjectionAngularTolerance: tolerance invalid");
	data().projectionAngularTolerance = tolerance;	
	markDirty();
}

PxReal D6Joint::getProjectionAngularTolerance()	const
{	
	return data().projectionAngularTolerance; 
}

void D6Joint::setProjectionLinearTolerance(PxReal tolerance)
{	
	PX_CHECK_AND_RETURN(PxIsFinite(tolerance) && tolerance >=0, "PxD6Joint::setProjectionLinearTolerance: invalid parameter");
	data().projectionLinearTolerance = tolerance;	
	markDirty(); 
}

PxReal D6Joint::getProjectionLinearTolerance() const	
{	
	return data().projectionLinearTolerance;		
}



void* D6Joint::prepareData()
{
	D6JointData& d = data();

	if(mRecomputeLimits)
	{
		d.thSwingY = PxTan(d.swingLimit.yAngle/2);
		d.thSwingZ = PxTan(d.swingLimit.zAngle/2);
		d.thSwingPad = PxTan(d.swingLimit.contactDistance/2);

		d.tqSwingY = PxTan(d.swingLimit.yAngle/4);
		d.tqSwingZ = PxTan(d.swingLimit.zAngle/4);
		d.tqSwingPad = PxTan(d.swingLimit.contactDistance/4);

		d.tqTwistLow  = PxTan(d.twistLimit.lower/4);
		d.tqTwistHigh = PxTan(d.twistLimit.upper/4);
		d.tqTwistPad = PxTan(d.twistLimit.contactDistance/4);
		mRecomputeLimits = false;
	}

	if(mRecomputeMotion)
	{
		d.driving = 0;
		d.limited = 0;
		d.locked = 0;

		for(PxU32 i=0;i<PxD6Axis::eCOUNT;i++)
		{
			if(d.motion[i] == PxD6Motion::eLIMITED)
				d.limited |= 1<<i;
			else if(d.motion[i] == PxD6Motion::eLOCKED)
				d.locked |= 1<<i;
		}

		// a linear direction isn't driven if it's locked
		if(active(PxD6Drive::eX) && d.motion[PxD6Axis::eX]!=PxD6Motion::eLOCKED) d.driving |= 1<< PxD6Drive::eX;
		if(active(PxD6Drive::eY) && d.motion[PxD6Axis::eY]!=PxD6Motion::eLOCKED) d.driving |= 1<< PxD6Drive::eY;
		if(active(PxD6Drive::eZ) && d.motion[PxD6Axis::eZ]!=PxD6Motion::eLOCKED) d.driving |= 1<< PxD6Drive::eZ;

		// SLERP drive requires all angular dofs unlocked, and inhibits swing/twist

		bool swing1Locked = d.motion[PxD6Axis::eSWING1] == PxD6Motion::eLOCKED;
		bool swing2Locked = d.motion[PxD6Axis::eSWING2] == PxD6Motion::eLOCKED;
		bool twistLocked  = d.motion[PxD6Axis::eTWIST]  == PxD6Motion::eLOCKED;

		if(active(PxD6Drive::eSLERP) && !swing1Locked && !swing2Locked && !twistLocked)
			d.driving |= 1<<PxD6Drive::eSLERP;
		else
		{
			if(active(PxD6Drive::eTWIST) && !twistLocked) 
				d.driving |= 1<<PxD6Drive::eTWIST;
			if(active(PxD6Drive::eSWING) && (!swing1Locked || !swing2Locked)) 
				d.driving |= 1<< PxD6Drive::eSWING;
		}

		mRecomputeMotion = false;
	}

	this->D6JointT::prepareData();

	return mData;
}

// Notes:
/*

This used to be in the linear drive model:

	if(motion[PxD6Axis::eX+i] == PxD6Motion::eLIMITED)
	{
		if(data.driveLinearVelocity[i] < 0.0f && cB2cA.p[i] < -mLimits[PxD6Limit::eLINEAR].mValue ||
			data.driveLinearVelocity[i] > 0.0f && cB2cA.p[i] > mLimits[PxD6Limit::eLINEAR].mValue)
			continue;
	}

it doesn't seem like a good idea though, because it turns off drive altogether, despite the fact that positional
drive might pull us back in towards the limit. Might be better to make the drive unilateral so it can only pull
us in from the limit

This used to be in angular locked:

	// Angular locked
	//TODO fix this properly. 	
	if(PxAbs(cB2cA.q.x) < 0.0001f) cB2cA.q.x = 0;
	if(PxAbs(cB2cA.q.y) < 0.0001f) cB2cA.q.y = 0;
	if(PxAbs(cB2cA.q.z) < 0.0001f) cB2cA.q.z = 0;
	if(PxAbs(cB2cA.q.w) < 0.0001f) cB2cA.q.w = 0;
	

*/

namespace
{
PxQuat truncate(const PxQuat& qIn, PxReal minCosHalfTol, bool& truncated)
{
	PxQuat q = qIn.w >= 0 ? qIn : -qIn;
	truncated = q.w < minCosHalfTol;
	if (!truncated)
		return q;
	PxVec3 v = q.getImaginaryPart().getNormalized() * PxSqrt(1 - minCosHalfTol * minCosHalfTol);
	return PxQuat(v.x, v.y, v.z, minCosHalfTol);
}

// we decompose the quaternion as q1 * q2, where q1 is a rotation orthogonal to the unit axis, and q2 a rotation around it.
// (so for example if 'axis' is the twist axis, this is separateSwingTwist).

PxQuat project(const PxQuat& q, const PxVec3& axis, PxReal cosHalfTol, bool& truncated)
{
	PxReal a = q.getImaginaryPart().dot(axis);
	PxQuat q2 = PxAbs(a) >= 1e-6f ? PxQuat(a*axis.x, a*axis.y, a*axis.z, q.w).getNormalized() : PxQuat(PxIdentity);
	PxQuat q1 = q * q2.getConjugate();

	PX_ASSERT(PxAbs(q1.getImaginaryPart().dot(q2.getImaginaryPart())) < 1e-6f);

	return truncate(q1, cosHalfTol, truncated) * q2;
}
}

namespace physx
{
// Here's how the angular part works:
// * if no DOFs are locked, there's nothing to do.
// * if all DOFs are locked, we just truncate the rotation
// * if two DOFs are locked
//  * we decompose the rotation into swing * twist, where twist is a rotation around the free DOF and swing is a rotation around an axis orthogonal to the free DOF
//  * then we truncate swing
// The case of one locked DOF is currently unimplemented, but one option would be:
// * if one DOF is locked (the tricky case), we define the 'free' axis as follows (as the velocity solver prep function does)
// TWIST: cB[0]
// SWING1: cB[0].cross(cA[2])
// SWING2: cB[0].cross(cA[1])
// then, as above, we decompose into swing * free, and truncate the free rotation

//export this in the physx namespace so we can unit test it
PxQuat angularProject(PxU32 lockedDofs, const PxQuat& q, PxReal cosHalfTol, bool& truncated)
{
	PX_ASSERT(lockedDofs <= 7);
	truncated = false;

	switch (lockedDofs)
	{
	case 0: return q;
	case 1: return q;		// currently unimplemented
	case 2: return q;		// currently unimplemented
	case 3: return project(q, PxVec3(0.0f, 0.0f, 1.0f), cosHalfTol, truncated);
	case 4: return q;		// currently unimplemented
	case 5: return project(q, PxVec3(0.0f, 1.0f, 0.0f), cosHalfTol, truncated);
	case 6: return project(q, PxVec3(1.0f, 0.0f, 0.0f), cosHalfTol, truncated);
	case 7: return truncate(q, cosHalfTol, truncated);
	default: return PxQuat(PxIdentity);
	}
}
}

namespace
{
void D6JointProject(const void* constantBlock,
	PxTransform& bodyAToWorld,
	PxTransform& bodyBToWorld,
	bool projectToA)
{
	using namespace joint;
	const D6JointData &data = *reinterpret_cast<const D6JointData*>(constantBlock);

	PxTransform cA2w, cB2w, cB2cA, projected;
	computeDerived(data, bodyAToWorld, bodyBToWorld, cA2w, cB2w, cB2cA);

	PxVec3 v(data.locked & 1 ? cB2cA.p.x : 0,
		data.locked & 2 ? cB2cA.p.y : 0,
		data.locked & 4 ? cB2cA.p.z : 0);

	bool linearTrunc, angularTrunc = false;
	projected.p = truncateLinear(v, data.projectionLinearTolerance, linearTrunc) + (cB2cA.p - v);

	projected.q = angularProject(data.locked >> 3, cB2cA.q, PxCos(data.projectionAngularTolerance / 2), angularTrunc);

	if (linearTrunc || angularTrunc)
		projectTransforms(bodyAToWorld, bodyBToWorld, cA2w, cB2w, projected, data, projectToA);
}


void D6JointVisualize(PxConstraintVisualizer &viz,
 				      const void* constantBlock,
					  const PxTransform& body0Transform,
					  const PxTransform& body1Transform,
					  PxU32 flags)
{
	using namespace joint;

	const PxU32 SWING1_FLAG = 1<<PxD6Axis::eSWING1, 
			    SWING2_FLAG = 1<<PxD6Axis::eSWING2, 
				TWIST_FLAG  = 1<<PxD6Axis::eTWIST;

	const PxU32 ANGULAR_MASK = SWING1_FLAG | SWING2_FLAG | TWIST_FLAG;
	const PxU32 LINEAR_MASK = 1<<PxD6Axis::eX | 1<<PxD6Axis::eY | 1<<PxD6Axis::eZ;

	PX_UNUSED(ANGULAR_MASK);
	PX_UNUSED(LINEAR_MASK);

	const D6JointData & data = *reinterpret_cast<const D6JointData*>(constantBlock);

	PxTransform cA2w = body0Transform * data.c2b[0];
	PxTransform cB2w = body1Transform * data.c2b[1];

	if(flags & PxConstraintVisualizationFlag::eLOCAL_FRAMES)
		viz.visualizeJointFrames(cA2w, cB2w);

	if(flags & PxConstraintVisualizationFlag::eLIMITS)
	{
		if(cA2w.q.dot(cB2w.q)<0)
			cB2w.q = -cB2w.q;

		PxTransform cB2cA = cA2w.transformInv(cB2w);	

		PxQuat swing, twist;
		Ps::separateSwingTwist(cB2cA.q,swing,twist);

		PxMat33 cA2w_m(cA2w.q), cB2w_m(cB2w.q);
		PxVec3 bX = cB2w_m[0], aY = cA2w_m[1], aZ = cA2w_m[2];

		if(data.limited&TWIST_FLAG)
		{
			PxReal tqPhi = Ps::tanHalf(twist.x, twist.w);		// always support (-pi, +pi)

			// PT: TODO: refactor with similar code in revolute joint
			PxReal quarterAngle = tqPhi;
			PxReal lower = data.tqTwistLow;
			PxReal upper = data.tqTwistHigh;
			PxReal pad = data.tqTwistPad;

			if(data.twistLimit.isSoft())
				pad = 0.0f;

			bool active = false;
			PX_ASSERT(lower<upper);
			if(quarterAngle < lower+pad)
				active = true;
			if(quarterAngle > upper-pad)
				active = true;

			viz.visualizeAngularLimit(cA2w, data.twistLimit.lower, data.twistLimit.upper, active);
		}

		bool swing1Limited = (data.limited & SWING1_FLAG)!=0, swing2Limited = (data.limited & SWING2_FLAG)!=0;

		if(swing1Limited && swing2Limited)
		{
			PxVec3 tanQSwing = PxVec3(0, Ps::tanHalf(swing.z,swing.w), -Ps::tanHalf(swing.y,swing.w));
			const PxReal pad = data.swingLimit.isSoft() ? 0.0f : data.tqSwingPad;
			Cm::ConeLimitHelper coneHelper(data.tqSwingZ, data.tqSwingY, pad);
			viz.visualizeLimitCone(cA2w, data.tqSwingZ, data.tqSwingY, 
				!coneHelper.contains(tanQSwing));
		}
		else if(swing1Limited ^ swing2Limited)
		{
			PxTransform yToX = PxTransform(PxVec3(0), PxQuat(-PxPi/2, PxVec3(0,0,1.f)));
			PxTransform zToX = PxTransform(PxVec3(0), PxQuat(PxPi/2, PxVec3(0,1.f,0)));

			if(swing1Limited)
			{
				if(data.locked & SWING2_FLAG)
					viz.visualizeAngularLimit(cA2w * yToX, -data.swingLimit.yAngle, data.swingLimit.yAngle, 
						PxAbs(Ps::tanHalf(swing.y, swing.w)) > data.tqSwingY - data.tqSwingPad);
				else
					viz.visualizeDoubleCone(cA2w * zToX, data.swingLimit.yAngle, 
						PxAbs(tanHalfFromSin(aZ.dot(bX)))> data.thSwingY - data.thSwingPad);
			}
			else 
			{
				if(data.locked & SWING1_FLAG)
					viz.visualizeAngularLimit(cA2w * zToX, -data.swingLimit.zAngle, data.swingLimit.zAngle,
						PxAbs(Ps::tanHalf(swing.z, swing.w)) > data.tqSwingZ - data.tqSwingPad);
				else
					viz.visualizeDoubleCone(cA2w * yToX, data.swingLimit.zAngle,  
						PxAbs(tanHalfFromSin(aY.dot(bX)))> data.thSwingZ - data.thSwingPad);
			}
		}
	}
}
}

bool D6Joint::attach(PxPhysics &physics, PxRigidActor* actor0, PxRigidActor* actor1)
{
	mPxConstraint = physics.createConstraint(actor0, actor1, *this, sShaders, sizeof(D6JointData));
	return mPxConstraint!=NULL;
}

void D6Joint::exportExtraData(PxSerializationContext& stream)
{
	if(mData)
	{
		stream.alignData(PX_SERIAL_ALIGN);
		stream.writeData(mData, sizeof(D6JointData));
	}
	stream.writeName(mName);
}

void D6Joint::importExtraData(PxDeserializationContext& context)
{
	if(mData)
		mData = context.readExtraData<D6JointData, PX_SERIAL_ALIGN>();

	context.readName(mName);
}

void D6Joint::resolveReferences(PxDeserializationContext& context)
{
	setPxConstraint(resolveConstraintPtr(context, getPxConstraint(), getConnector(), sShaders));	
}

D6Joint* D6Joint::createObject(PxU8*& address, PxDeserializationContext& context)
{
	D6Joint* obj = new (address) D6Joint(PxBaseFlag::eIS_RELEASABLE);
	address += sizeof(D6Joint);	
	obj->importExtraData(context);
	obj->resolveReferences(context);
	return obj;
}

// global function to share the joint shaders with API capture	
const PxConstraintShaderTable* Ext::GetD6JointShaderTable() 
{ 
	return &D6Joint::getConstraintShaderTable();
}

//~PX_SERIALIZATION
PxConstraintShaderTable Ext::D6Joint::sShaders = { Ext::D6JointSolverPrep, D6JointProject, D6JointVisualize, PxConstraintFlag::eGPU_COMPATIBLE };

