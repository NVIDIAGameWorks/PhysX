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

#include "PxPhysicsAPI.h"
#include "extensions/PxExtensionsAPI.h"
#include "Crab.h"
#include "CrabManager.h"
#include "SceneCrab.h"
#include "SimScene.h"
#include "CompoundCreator.h"
#include <GL/glut.h>

#include "PxTkStream.h"
#include "PxTkFile.h"
using namespace PxToolkit;
// if enabled: runs the crab AI in sync, not as a parallel task to physx.
#define DEBUG_RENDERING 0


void setupFiltering(PxRigidActor* actor, PxU32 filterGroup, PxU32 filterMask);

// table with default times in seconds how the crab AI will try to stay in a state
static const PxReal gDefaultStateTime[CrabState::eNUM_STATES] = { 5.0f, 30.0f, 30.0f, 10.0f, 10.0f, 6.0f };


Crab::Crab(CrabManager* crabManager, PxU32 updateFrequency)
{
	mUpdateFrequency = updateFrequency;
	mManager = crabManager;
	initMembers();
}


void Crab::initMembers()
{
	mCrabBody = NULL;
	//mSqRayBuffer = NULL;
	mSqSweepBuffer = NULL; 
	mLegHeight = 0;
	mMaterial = mManager->getPhysics().createMaterial(0.9f, 0.7f, 0.01f);
	mCrabState = CrabState::eWAITING;
	mStateTime = gDefaultStateTime[CrabState::eWAITING];
	mAccumTime = 0;
	mElapsedTime = 0;
	
	mAcceleration[0] = 0;
	mAcceleration[1] = 0;

	mAccelerationBuffer[0] = 0;
	mAccelerationBuffer[1] = 0;
	for (PxU32 a = 0; a < 6; ++a)
		mDistances[a] = 0;

	// setup buffer for 10 batched rays and 10 hits
	//mSqRayBuffer = PX_PLACEMENT_NEW(PX_ALLOC(sizeof(SqRayBuffer), PX_DEBUG_EXP("SqRayBuffer")), SqRayBuffer)(this, 10, 10);
	mSqSweepBuffer = PX_PLACEMENT_NEW(PX_ALLOC(sizeof(SqSweepBuffer), PX_DEBUG_EXP("SqSweepBuffer")), SqSweepBuffer)(this, 6, 6);

}

Crab::~Crab()
{
	//mSqRayBuffer->~SqRayBuffer();
	//PX_FREE(mSqRayBuffer);

	mSqSweepBuffer->~SqSweepBuffer();
	PX_FREE(mSqSweepBuffer);
}

void Crab::setScene(PxScene* scene)
{
	mSqSweepBuffer->setScene(scene);
}


static void setShapeFlag(PxRigidActor* actor, PxShapeFlag::Enum flag, bool flagValue)
{
	const PxU32 numShapes = actor->getNbShapes();
	PxShape** shapes = (PxShape**)PX_ALLOC(sizeof(PxShape*)*numShapes, PX_DEBUG_EXP("SceneCrab:setShapeFlag"));
	actor->getShapes(shapes, numShapes);
	for (PxU32 i = 0; i < numShapes; i++)
	{
		PxShape* shape = shapes[i];
		shape->setFlag(flag, flagValue);
	}
	PX_FREE(shapes);
}

PxVec3 Crab::getPlaceOnFloor(PxVec3 start)
{
	PxRaycastBuffer rayHit;
	mManager->getScene().raycast(start, PxVec3(0, -1, 0), 1000.0f, rayHit);

	return rayHit.block.position + PxVec3(0, mLegHeight, 0);
}

//static const PxSerialObjectId mMaterial_id = (PxSerialObjectId)0x01;
//static const PxSerialObjectId mCrabBody_id = (PxSerialObjectId)0x02;
//static const PxSerialObjectId mMotorJoint0_id = (PxSerialObjectId)0x03;
//static const PxSerialObjectId mMotorJoint1_id = (PxSerialObjectId)0x04;
//
//struct FilterGroup
//{
//	enum Enum
//	{
//		eSUBMARINE = (1 << 0),
//		eMINE_HEAD = (1 << 1),
//		eMINE_LINK = (1 << 2),
//		eCRAB = (1 << 3),
//		eHEIGHTFIELD = (1 << 4),
//	};
//};
//
//void setupFiltering(PxRigidActor* actor, PxU32 filterGroup, PxU32 filterMask)
//{
//	PxFilterData filterData;
//	filterData.word0 = filterGroup; // word0 = own ID
//	filterData.word1 = filterMask;	// word1 = ID mask to filter pairs that trigger a contact callback;
//	const PxU32 numShapes = actor->getNbShapes();
//	PxShape** shapes = (PxShape**)PX_ALLOC(sizeof(PxShape*)*numShapes, PX_DEBUG_EXP("setupFiltering"));
//	actor->getShapes(shapes, numShapes);
//	for (PxU32 i = 0; i < numShapes; i++)
//	{
//		PxShape* shape = shapes[i];
//		shape->setSimulationFilterData(filterData);
//	}
//	PX_FREE(shapes);
//}

void setupSQFiltering(PxRigidActor* actor, PxU32 crabId)
{
	PxFilterData filterData(0, 0, 0, crabId);
	PxShape* shape;
	PxU32 nbShapes = actor->getNbShapes();
	for (PxU32 a = 0; a < nbShapes; ++a)
	{
		actor->getShapes(&shape, 1, a);
		shape->setQueryFilterData(filterData);
	}
}

Crab* Crab::create(const PxVec3& _crabPos, const PxReal crabDepth, const PxReal scale, const PxReal legMass, const PxU32 numLegs)
{
	static PxU32 crabId = 0;
	crabId++;
	/*static const PxReal scale = 0.8f;
	static const PxReal crabDepth = 2.0f;
	static const PxVec3 crabBodyDim = PxVec3(0.8f, 0.8f, crabDepth*0.5f)*scale;
	static const PxReal legMass = 0.03f;*/

	const PxVec3 crabBodyDim = PxVec3(0.8f, 0.8f, crabDepth*0.5f)*scale;
	const PxReal velocity = 0.0f;
	//static const PxReal maxForce = 4000.0f;

	const PxReal maxForce = 50000.0f;

	LegParameters params; // check edge ascii art in Crab.h
	params.a = 0.5f;
	params.b = 0.6f;
	params.c = 0.5f;
	params.d = 0.5f;
	params.e = 1.5f;
	params.m = 0.3f;
	params.n = 0.1f;

	mLegHeight = scale*2.0f*(params.d + params.c);
	mLegHeight += 0.5f;
	PxVec3 crabPos = getPlaceOnFloor(_crabPos);

	ShaderMaterial mat;
	mat.init();

	PxVec3 vel(0), omega(0);
	PxTransform localPose = PxTransform(PxQuat(PxHalfPi*0.5f, PxVec3(0, 0, 1)));
	SimScene* simScene = mManager->getSceneCrab()->getSimScene();
	simScene->getCompoundCreator()->createBox(2.f*crabBodyDim, &localPose);
	mCrabBodyDimensions = crabBodyDim;
	//mCrabBody = PxCreateDynamic(getPhysics(), PxTransform(crabPos), PxBoxGeometry(crabBodyDim), *mMaterial, 10.f);
	Compound* compound = mManager->createObject(PxTransform(crabPos), vel, omega, false, mat);
	mCrabBody = compound->getPxActor();


	/*PxShape* shape; mCrabBody->getShapes(&shape, 1);
	shape->setLocalPose(PxTransform(PxQuat(PxHalfPi*0.5f, PxVec3(0, 0, 1))));*/
	PxRigidBodyExt::setMassAndUpdateInertia(*mCrabBody, legMass*10.0f);
	PxTransform cmPose = mCrabBody->getCMassLocalPose();
	cmPose.p.y -= 1.8f;
	mCrabBody->setCMassLocalPose(cmPose);
	//mCrabBody->setAngularDamping(100.0f);
	mCrabBody->setAngularDamping(0.5f);
	mCrabBody->userData = this;
	//mActors.push_back(mCrabBody);

	PxQuat rotation(3.1415 / 2.f, PxVec3(0.f, 0.f, 1.f));

	PxD6Joint* joint = PxD6JointCreate(mManager->getPhysics(), NULL, PxTransform(rotation), mCrabBody, PxTransform(mCrabBody->getGlobalPose().q.getConjugate() * rotation));

	joint->setMotion(PxD6Axis::eX, PxD6Motion::eFREE);
	joint->setMotion(PxD6Axis::eY, PxD6Motion::eFREE);
	joint->setMotion(PxD6Axis::eZ, PxD6Motion::eFREE);

	joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
	joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
	joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLIMITED);

	PxJointLimitCone limitCone(3.1415 / 6.f, 3.1415 / 6.f);
	joint->setSwingLimit(limitCone);



	// legs
	PxReal recipNumLegs = 1.0f / PxReal(numLegs);
	PxReal recipNumLegsMinus1 = 1.0f / PxReal(numLegs - 1);
	//PX_COMPILE_TIME_ASSERT((numLegs & 1) == 0);

	PxRigidDynamic* motor[2];
	{
		const PxReal density = 1.0f;
		const PxReal m = params.m * scale;
		const PxReal n = params.n * scale;
		const PxBoxGeometry boxGeomM = PxBoxGeometry(m, m, crabBodyDim.z * 0.5f + 0.15f);

		// create left and right motor
		PxVec3 motorPos = crabPos + PxVec3(0, n, 0);
		for (PxU32 i = 0; i < 2; i++)
		{
			PxVec3 motorOfs = i == 0 ? PxVec3(0, 0, boxGeomM.halfExtents.z) : -PxVec3(0, 0, boxGeomM.halfExtents.z);
			simScene->getCompoundCreator()->createBox(2.f*boxGeomM.halfExtents);
			Compound* motorCompound = mManager->createObject(PxTransform(motorPos + motorOfs), vel, omega, false, mat);
			motor[i] = motorCompound->getPxActor();

			PxRigidBodyExt::setMassAndUpdateInertia(*motor[i], legMass);
			motor[i]->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
			setShapeFlag(motor[i], PxShapeFlag::eSIMULATION_SHAPE, false);
			setShapeFlag(motor[i], PxShapeFlag::eSCENE_QUERY_SHAPE, false);

			/*mMotorJoint[i] = PxRevoluteJointCreate(mManager->getPhysics(),
				mCrabBody, PxTransform(motorOfs, PxQuat(-PxHalfPi, PxVec3(0, 1, 0))),
				motor[i], PxTransform(PxVec3(0, 0, 0), PxQuat(-PxHalfPi, PxVec3(0, 1, 0))));

			mMotorJoint[i]->setDriveVelocity(velocity);
			mMotorJoint[i]->setDriveForceLimit(maxForce);
			mMotorJoint[i]->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, true);*/  

			mMotorJoint[i] = PxD6JointCreate(mManager->getPhysics(),
				mCrabBody, PxTransform(motorOfs, PxQuat(-PxHalfPi, PxVec3(0, 1, 0))),
				motor[i], PxTransform(PxVec3(0, 0, 0), PxQuat(-PxHalfPi, PxVec3(0, 1, 0))));

			mMotorJoint[i]->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
			mMotorJoint[i]->setDriveVelocity(PxVec3(0), PxVec3(velocity, 0.f, 0.f));
			mMotorJoint[i]->setDrive(PxD6Drive::eTWIST, PxD6JointDrive(0.f, 30.f, maxForce));

			//mActors.push_back(motor[i]);
			//mJoints.push_back(mMotorJoint[i]);
		}

		// create legs and attach to left and right motor
		PxReal legSpacing = crabDepth*recipNumLegsMinus1*scale;
		PxVec3 bodyToLegPos0 = PxVec3(0, 0, crabBodyDim.z);
		PxVec3 bodyToLegPos1 = PxVec3(0, 0, crabBodyDim.z - (numLegs / 2)*legSpacing);
		PxVec3 motorToLegPos0 = PxVec3(0, 0, crabBodyDim.z*0.5f);
		PxVec3 motorToLegPos1 = PxVec3(0, 0, (crabBodyDim.z - legSpacing)*0.5f);
		for (PxU32 i = 0; i < numLegs / 2; i++)
		{
			PxReal angle0 = -PxHalfPi + PxTwoPi*recipNumLegs*i;
			PxReal angle1 = angle0 + PxPi;

			createLeg(mCrabBody, bodyToLegPos0, legMass, params, scale, motor[0], motorToLegPos0 + m * PxVec3(PxCos(angle0), PxSin(angle0), 0));
			createLeg(mCrabBody, bodyToLegPos1, legMass, params, scale, motor[1], motorToLegPos1 + m * PxVec3(PxCos(angle1), PxSin(angle1), 0));
			bodyToLegPos0.z -= legSpacing;
			bodyToLegPos1.z -= legSpacing;
			motorToLegPos0.z -= legSpacing;
			motorToLegPos1.z -= legSpacing;
		}
	}

	setupSQFiltering(mCrabBody, crabId);

	return this;
}

void Crab::createLeg(PxRigidDynamic* mainBody, PxVec3 localPos, PxReal mass, const LegParameters& params, PxReal scale, PxRigidDynamic* motor, PxVec3 motorAttachmentPos)
{
	PxVec3 crabLegPos = mainBody->getGlobalPose().p + localPos;

	// params for Theo Jansen's machine
	// check edge ascii art in Crab.h
	const PxReal stickExt = 0.125f * 0.5f * scale;
	const PxReal a = params.a * scale;
	const PxReal b = params.b * scale;
	const PxReal c = params.c * scale;
	const PxReal d = params.d * scale;
	const PxReal e = params.e * scale;
	const PxReal m = params.m * scale;
	const PxReal n = params.n * scale;

	const PxReal density = 1.0f;

	PxBoxGeometry boxGeomA = PxBoxGeometry(a, stickExt, stickExt);
	PxBoxGeometry boxGeomB = PxBoxGeometry(stickExt, b, stickExt);
	PxBoxGeometry boxGeomC = PxBoxGeometry(stickExt, c, stickExt);

	//PxCapsuleGeometry capsGeomD = PxCapsuleGeometry(stickExt*2.0f, d);
	PxBoxGeometry boxGeomD = PxBoxGeometry(d, stickExt*3.0f, stickExt*3.0f);

	ShaderMaterial mat;
	mat.init();
	PxVec3 vel(0), omega(0);

	SimScene* simScene = mManager->getSceneCrab()->getSimScene();
	PxPhysics& physics = mManager->getPhysics();

	for (PxU32 leg = 0; leg < 2; leg++)
	{
		bool left = (leg == 0);
#define MIRROR(X) left ? -1.0f*(X) : (X)
		PxVec3 startPos = crabLegPos + PxVec3(MIRROR(e), 0, 0);

		// create upper triangle from boxes
		PxRigidDynamic* upperTriangle = NULL;
		{
			PxTransform poseA = PxTransform(PxVec3(MIRROR(a), 0, 0));
			PxTransform poseB = PxTransform(PxVec3(MIRROR(0), b, 0));
			simScene->getCompoundCreator()->createBox(2.f*boxGeomA.halfExtents, &poseA);
			simScene->getCompoundCreator()->createBox(2.f*boxGeomB.halfExtents, &poseB, false);

			Compound* upperTriangleCompound = mManager->createObject(PxTransform(startPos), vel, omega, false, mat);
			upperTriangle = upperTriangleCompound->getPxActor();
			upperTriangle->setAngularDamping(0.5f);
			PxRigidBodyExt::updateMassAndInertia(*upperTriangle, density);

			setShapeFlag(upperTriangle, PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		}

		// create lower triangle from boxes
		PxRigidDynamic* lowerTriangle = NULL;
		{
			PxTransform poseA = PxTransform(PxVec3(MIRROR(a), 0, 0));
			//PxTransform poseD = PxTransform(PxVec3(MIRROR(0), -d, 0));
			PxTransform poseD = PxTransform(PxVec3(MIRROR(0), -d, 0), PxQuat(PxHalfPi, PxVec3(0, 0, 1)));
			simScene->getCompoundCreator()->createBox(2.f*boxGeomA.halfExtents, &poseA);
			simScene->getCompoundCreator()->createBox(2.f*boxGeomD.halfExtents, &poseD, false);

			Compound* lowerTriangleCompound = mManager->createObject(PxTransform(startPos + PxVec3(0, -2.0f*c, 0)), vel, omega, false, mat);
			lowerTriangle = lowerTriangleCompound->getPxActor();
			lowerTriangle->setAngularDamping(0.5f);
			PxShape* shapes[2];

			lowerTriangle->getShapes(shapes, 2);
			shapes[1]->setMaterials(&mMaterial, 1);
			PxRigidBodyExt::updateMassAndInertia(*lowerTriangle, density);

			setShapeFlag(lowerTriangle, PxShapeFlag::eSCENE_QUERY_SHAPE, false);

		}

		// create vertical boxes to connect the triangles
		simScene->getCompoundCreator()->createBox(2.f*boxGeomC.halfExtents);
		Compound* verticalBox0Compound = mManager->createObject(PxTransform(startPos + PxVec3(0, -c, 0)), vel, omega, false, mat);
		PxRigidDynamic* verticalBox0 = verticalBox0Compound->getPxActor();
		PxRigidBodyExt::updateMassAndInertia(*verticalBox0, 10.f);

		setShapeFlag(verticalBox0, PxShapeFlag::eSCENE_QUERY_SHAPE, false);

		simScene->getCompoundCreator()->createBox(2.f*boxGeomC.halfExtents);
		Compound* verticalBox1Compound = mManager->createObject(PxTransform(startPos + PxVec3(MIRROR(2.0f*a), -c, 0)), vel, omega, false, mat);
		PxRigidDynamic* verticalBox1 = verticalBox1Compound->getPxActor();
		PxRigidBodyExt::updateMassAndInertia(*verticalBox1, 10.f);

		setShapeFlag(verticalBox1, PxShapeFlag::eSCENE_QUERY_SHAPE, false);


		// disable gravity
		/*upperTriangle->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
		lowerTriangle->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
		verticalBox0->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
		verticalBox1->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);*/

		// set mass
		PxRigidBodyExt::setMassAndUpdateInertia(*upperTriangle, mass);
		PxRigidBodyExt::setMassAndUpdateInertia(*lowerTriangle, mass);
		PxRigidBodyExt::setMassAndUpdateInertia(*verticalBox0, mass);
		PxRigidBodyExt::setMassAndUpdateInertia(*verticalBox1, mass);

		// turn off collision upper triangle and vertical boxes
		setShapeFlag(upperTriangle, PxShapeFlag::eSIMULATION_SHAPE, false);
		setShapeFlag(verticalBox0, PxShapeFlag::eSIMULATION_SHAPE, false);
		setShapeFlag(verticalBox1, PxShapeFlag::eSIMULATION_SHAPE, false);

		// d6 joint in lower corner of upper triangle
	
		PxD6Joint* jointD6 = PxD6JointCreate(physics, 
			mainBody, PxTransform(PxVec3(MIRROR(e), 0, 0) + localPos, PxQuat(-PxHalfPi, PxVec3(0, 1, 0))), 
			upperTriangle, PxTransform(PxVec3(0, 0, 0), PxQuat(-PxHalfPi, PxVec3(0, 1, 0))));

		jointD6->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
		

		// 4 d6 joints to connect triangles

		jointD6 = PxD6JointCreate(physics,
			upperTriangle, PxTransform(PxVec3(0, 0, 0), PxQuat(-PxHalfPi, PxVec3(0, 1, 0))),
			verticalBox0, PxTransform(PxVec3(0, c, 0), PxQuat(-PxHalfPi, PxVec3(0, 1, 0))));

		jointD6->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);


		jointD6 = PxD6JointCreate(physics,
			upperTriangle, PxTransform(PxVec3(MIRROR(2.0f*a), 0, 0), PxQuat(-PxHalfPi, PxVec3(0, 1, 0))),
			verticalBox1, PxTransform(PxVec3(0, c, 0), PxQuat(-PxHalfPi, PxVec3(0, 1, 0))));

		jointD6->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);


		jointD6 = PxD6JointCreate(physics,
			lowerTriangle, PxTransform(PxVec3(0, 0, 0), PxQuat(-PxHalfPi, PxVec3(0, 1, 0))),
			verticalBox0, PxTransform(PxVec3(0, -c, 0), PxQuat(-PxHalfPi, PxVec3(0, 1, 0))));

		jointD6->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);

		//mJoints.push_back(jointD6);

		jointD6 = PxD6JointCreate(physics,
			lowerTriangle, PxTransform(PxVec3(MIRROR(2.0f*a), 0, 0), PxQuat(-PxHalfPi, PxVec3(0, 1, 0))),
			verticalBox1, PxTransform(PxVec3(0, -c, 0), PxQuat(-PxHalfPi, PxVec3(0, 1, 0))));

		jointD6->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);

		// 2 distance constraints to connect motor with the triangles
		PxTransform motorTransform = PxTransform(motorAttachmentPos);
		PxReal dist0 = PxSqrt((2.0f*b - n)*(2.0f*b - n) + (e - m)*(e - m));
		PxReal dist1 = PxSqrt((2.0f*c + n)*(2.0f*c + n) + (e - m)*(e - m));

		PxDistanceJoint* distJoint0 = PxDistanceJointCreate(physics, upperTriangle, PxTransform(PxVec3(0, 2.0f*b, 0)), motor, motorTransform);

		// set min & max distance to dist0
		distJoint0->setMaxDistance(dist0);
		distJoint0->setMinDistance(dist0);
		// setup damping & spring
		//distJoint0->setDamping(0.1f);
		//distJoint0->setStiffness(100.0f);
		distJoint0->setDamping(0.2f);
		distJoint0->setStiffness(500.0f);
		distJoint0->setDistanceJointFlags(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED | PxDistanceJointFlag::eMIN_DISTANCE_ENABLED | PxDistanceJointFlag::eSPRING_ENABLED);

	/*	
		jointD6 = PxD6JointCreate(physics, upperTriangle, PxTransform(PxVec3(0, 2.0f*b, 0)), motor, motorTransform);
		jointD6->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
		jointD6->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
		jointD6->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);
		jointD6->setMotion(PxD6Axis::eX, PxD6Motion::eLIMITED);
		jointD6->setMotion(PxD6Axis::eY, PxD6Motion::eLIMITED);
		jointD6->setMotion(PxD6Axis::eZ, PxD6Motion::eLIMITED);
		jointD6->setLinearLimit(PxJointLinearLimit(dist0, PxSpring(100.f, 0.1f)));
		mJoints.push_back(jointD6);*/
	

		PxDistanceJoint* distJoint1 = PxDistanceJointCreate(physics, lowerTriangle, PxTransform(PxVec3(0, 0, 0)), motor, motorTransform);

		// set min & max distance to dist0
		distJoint1->setMaxDistance(dist1);
		distJoint1->setMinDistance(dist1);
		// setup damping & spring
	/*	distJoint1->setDamping(0.1f);
		distJoint1->setStiffness(100.0f);*/
		distJoint1->setDamping(0.2f);
		distJoint1->setStiffness(500.0f);
		distJoint1->setDistanceJointFlags(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED | PxDistanceJointFlag::eMIN_DISTANCE_ENABLED | PxDistanceJointFlag::eSPRING_ENABLED);

		/*jointD6 = PxD6JointCreate(physics, lowerTriangle, PxTransform(PxVec3(0, 0, 0)), motor, motorTransform);
		mJoints.push_back(jointD6);*/

		// one distance joint to ensure that the vertical boxes do not get stuck if they cross the diagonal.
		PxReal halfDiagDist = PxSqrt(a*a + c*c);
		PxDistanceJoint* noFlip = PxDistanceJointCreate(physics, lowerTriangle, PxTransform(PxVec3(MIRROR(2.0f*a), 0, 0)), upperTriangle, PxTransform(PxVec3(0)));

		// set min & max distance to dist0
		noFlip->setMaxDistance(2.0f * (a + c));
		noFlip->setMinDistance(halfDiagDist);
		// setup damping & spring
		noFlip->setDamping(1.0f);
		noFlip->setStiffness(100.0f);
		noFlip->setDistanceJointFlags(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED | PxDistanceJointFlag::eMIN_DISTANCE_ENABLED | PxDistanceJointFlag::eSPRING_ENABLED);
	}
}



void Crab::update(PxReal dt)
{
	PxReal maxVelocity = 16.0f;
	PxReal velDamping = 0.8f;

	flushAccelerationBuffer();

	for (PxU32 i = 0; i < 2; i++)
	{
		/*PxReal prevVelocity = mMotorJoint[i]->getDriveVelocity();
		PxReal velocityChange = mAcceleration[i] ? mAcceleration[i] * dt : -prevVelocity*velDamping*dt;
		PxReal newVelocity = PxClamp(prevVelocity + velocityChange, -maxVelocity, maxVelocity);
		mMotorJoint[i]->setDriveVelocity(newVelocity);*/

		PxVec3 linear, angular;
		mMotorJoint[i]->getDriveVelocity(linear, angular);
		PxReal prevVelocity = -angular.x;
		PxReal velocityChange = mAcceleration[i] ? mAcceleration[i] * dt : -prevVelocity*velDamping*dt;
		PxReal newVelocity = PxClamp(prevVelocity + velocityChange, -maxVelocity, maxVelocity);
		mMotorJoint[i]->setDriveVelocity(linear, PxVec3(-newVelocity, 0.f, 0.f));

		if (mAcceleration[i] != 0.0f)
			mCrabBody->wakeUp();

		mAcceleration[i] = 0;
	}

	// add up elapsed time
	mAccumTime += dt;

	{
		mElapsedTime = mAccumTime;
		mAccumTime = 0;
	}
}

void Crab::run()
{
	scanForObstacles();
	updateState();
}


void Crab::setAcceleration(PxReal leftAcc, PxReal rightAcc)
{
	mAccelerationBuffer[0] = -leftAcc;
	mAccelerationBuffer[1] = -rightAcc;
}


void Crab::flushAccelerationBuffer()
{
	mAcceleration[0] = mAccelerationBuffer[0];
	mAcceleration[1] = mAccelerationBuffer[1];
}

void Crab::scanForObstacles()
{
	if (mCrabState != CrabState::eWAITING)
	{
		if (mUpdateFrequency == 0)
		{
			mUpdateFrequency = UPDATE_FREQUENCY_RESET;
			PxSceneReadLock scopedLock(mManager->getScene());

			PxTransform crabPose = mCrabBody->getGlobalPose();
			PxVec3 rayStart[2] = { PxVec3(2.0f, 0.0f, 0.0f), PxVec3(-2.0f, 0.0f, 0.0f) };
			rayStart[0] = crabPose.transform(rayStart[0]);
			rayStart[1] = crabPose.transform(rayStart[1]);
			PxReal rayDist = 100.0f;

			PxShape* shape;
			mCrabBody->getShapes(&shape, 1);

			PxBoxGeometry boxGeom;
			boxGeom.halfExtents = mCrabBodyDimensions;
			//bool result = shape->getBoxGeometry(boxGeom); //crab body isn't using a box anymore

			PxVec3 forward = crabPose.rotate(PxVec3(0, 0, 1));
			PxVec3 sideDir = (forward.cross(PxVec3(0, 1, 0)) - PxVec3(0, 0.4f, 0)).getNormalized();

			// setup raycasts
			// 3 front & 3 back
			for (PxU32 j = 0; j < 2; j++)
			{
				//PxVec3 rayDir = crabPose.rotate(PxVec3(j ? -1.0f : 1.0f, 0, 0));
				PxVec3 rayDir = j ? -sideDir : sideDir;
				PxQuat rotY = PxQuat(0.4f, PxVec3(0, 1, 0));
				rayDir = rotY.rotateInv(rayDir);
				for (PxU32 i = 0; i < 3; i++)
				{
					rayDir.normalize();
					//mSqRayBuffer->mBatchQuery->raycast(rayStart[j], rayDir, rayDist);
					mSqSweepBuffer->mBatchQuery->sweep(boxGeom, crabPose, rayDir, rayDist, 0, PxHitFlag::eDEFAULT, 
						PxQueryFilterData(shape->getQueryFilterData(), PxQueryFlags(PxQueryFlag::eDYNAMIC | PxQueryFlag::eSTATIC | PxQueryFlag::ePREFILTER)));
					rayDir = rotY.rotate(rayDir);
				}
			}

			mSqSweepBuffer->mBatchQuery->execute();
			for (PxU32 i = 0; i < mSqSweepBuffer->mQueryResultSize; i++)
			{
				PxSweepQueryResult& result = mSqSweepBuffer->mSweepResults[i];
				if (result.queryStatus == PxBatchQueryStatus::eSUCCESS && result.getNbAnyHits() == 1)
				{
					const PxSweepHit& hit = result.getAnyHit(0);
					mDistances[i] = hit.distance;

					// don't see flat terrain as wall
					//SampleRenderer::RendererColor rayColor(0, 0, 255);
					//PxReal angle = hit.normal.dot(crabPose.q.rotate(PxVec3(0, 1, 0)));
					PxReal angle = hit.normal.dot(PxVec3(0, 1, 0));
					if (angle > 0.92f) // = 11.5 degree difference
					{
						mDistances[i] = rayDist;
						//	rayColor = SampleRenderer::RendererColor(0, 255, 0);
					}
				}
				else
					mDistances[i] = rayDist;
			}
		}

		mUpdateFrequency--;
	}
}

void Crab::initState(CrabState::Enum state)
{
	//if the original crabstate is waitting, we need to do scan for obstacles. Otherwise, we have done the scan for obstacles before we updateState  
	if (mCrabState == CrabState::eWAITING)
		scanForObstacles();
	mCrabState = state;
	mStateTime = gDefaultStateTime[mCrabState];
}

void Crab::updateState()
{
	// update remaining time in current state
	// transition if needed
	mStateTime -= mElapsedTime;
	mElapsedTime = 0;
	if (mStateTime <= 0.0f)
	{
		initState(CrabState::Enum((rand()%(CrabState::eNUM_STATES-1))+1));
	}

	PxTransform crabPose;
	{
		PxSceneReadLock scopedLock(mManager->getScene());
		crabPose = mCrabBody->getGlobalPose();
	}


	PxReal leftAcc = 0, rightAcc = 0;
	// compute fwd and bkwd distances
	static const PxReal minDist = 10.0f;
	static const PxReal fullSpeedDist = 50.0f;
	static const PxReal recipFullSpeedDist = 1.0f / fullSpeedDist;
	PxReal fDist = 0, bDist = 0;
	fDist = PxMin(mDistances[0], PxMin(mDistances[1], mDistances[2]));
	bDist = PxMin(mDistances[3], PxMin(mDistances[4], mDistances[5]));

	// handle states
	if (mCrabState == CrabState::eMOVE_FWD)
	{
		if (fDist < minDist && fDist < bDist)
		{
			initState(CrabState::eMOVE_BKWD);
		}
		else
		{
			leftAcc = PxMin(fullSpeedDist, mDistances[0])*recipFullSpeedDist*2.0f - 1.0f;
			rightAcc = PxMin(fullSpeedDist, mDistances[2])*recipFullSpeedDist*2.0f - 1.0f;
			leftAcc *= 3.0f;
			rightAcc *= 3.0f;
		}
	}
	else if (mCrabState == CrabState::eMOVE_BKWD)
	{
		if (bDist < minDist && bDist < fDist)
		{
			// find rotation dir, where we have some free space
			bool rotateLeft = mDistances[0] < mDistances[2];
			initState(rotateLeft ? CrabState::eROTATE_LEFT : CrabState::eROTATE_RIGHT);
		}
		else
		{
			leftAcc = -(PxMin(fullSpeedDist, mDistances[5])*recipFullSpeedDist*2.0f - 1.0f);
			rightAcc = -(PxMin(fullSpeedDist, mDistances[3])*recipFullSpeedDist*2.0f - 1.0f);
			leftAcc *= 3.0f;
			rightAcc *= 3.0f;
		}
	}
	else if (mCrabState == CrabState::eROTATE_LEFT)
	{
		leftAcc = -3.0f;
		rightAcc = 3.0f;
		if (fDist > minDist)
		{
			initState(CrabState::eMOVE_FWD);
		}
	}
	else if (mCrabState == CrabState::eROTATE_RIGHT)
	{
		leftAcc = 3.0f;
		rightAcc = -3.0f;
		if (fDist > minDist)
		{
			initState(CrabState::eMOVE_FWD);
		}
	}
	else if (mCrabState == CrabState::ePANIC)
	{

	}
	else if (mCrabState == CrabState::eWAITING)
	{
		// have a break
	}

	// change acceleration
	setAcceleration(leftAcc, rightAcc);

}

PxQueryHitType::Enum myFilterShader(
	PxFilterData queryFilterData, PxFilterData objectFilterData,
	const void* constantBlock, PxU32 constantBlockSize,
	PxHitFlags& hitFlags)
{
	if (objectFilterData.word0 == 128 || objectFilterData.word3 == queryFilterData.word3)
		return PxQueryHitType::eNONE;
	return PxQueryHitType::eBLOCK;
}

SqRayBuffer::SqRayBuffer(Crab* crab, PxU32 numRays, PxU32 numHits) :
mQueryResultSize(numRays), mHitSize(numHits)
{
	mCrab = crab;
	mOrigAddresses[0] = malloc(mQueryResultSize*sizeof(PxRaycastQueryResult) + 15);
	mOrigAddresses[1] = malloc(mHitSize*sizeof(PxRaycastHit) + 15);

	mRayCastResults = reinterpret_cast<PxRaycastQueryResult*>((size_t(mOrigAddresses[0]) + 15) & ~15);
	mRayCastHits = reinterpret_cast<PxRaycastHit*>((size_t(mOrigAddresses[1]) + 15)& ~15);

	PxBatchQueryDesc batchQueryDesc(mQueryResultSize, 0, 0);
	batchQueryDesc.queryMemory.userRaycastResultBuffer = mRayCastResults;
	batchQueryDesc.queryMemory.userRaycastTouchBuffer = mRayCastHits;
	batchQueryDesc.queryMemory.raycastTouchBufferSize = mHitSize;
	batchQueryDesc.preFilterShader = myFilterShader;

	mBatchQuery = mCrab->getManager()->getScene().createBatchQuery(batchQueryDesc);
}

SqRayBuffer::~SqRayBuffer()
{
	mBatchQuery->release();
	free(mOrigAddresses[0]);
	free(mOrigAddresses[1]);
}

void SqRayBuffer::setScene(PxScene* scene)
{
	PxBatchQueryDesc batchQueryDesc(mQueryResultSize, 0, 0);
	batchQueryDesc.queryMemory.userRaycastResultBuffer = mRayCastResults;
	batchQueryDesc.queryMemory.userRaycastTouchBuffer = mRayCastHits;
	batchQueryDesc.queryMemory.raycastTouchBufferSize = mHitSize;
	batchQueryDesc.preFilterShader = myFilterShader;

	mBatchQuery = scene->createBatchQuery(batchQueryDesc);
}

SqSweepBuffer::SqSweepBuffer(Crab* crab, PxU32 numRays, PxU32 numHits) :
mQueryResultSize(numRays), mHitSize(numHits)
{
	mCrab = crab;
	mOrigAddresses[0] = malloc(mQueryResultSize*sizeof(PxSweepQueryResult) + 15);
	mOrigAddresses[1] = malloc(mHitSize*sizeof(PxSweepHit) + 15);

	mSweepResults = reinterpret_cast<PxSweepQueryResult*>((size_t(mOrigAddresses[0]) + 15) & ~15);
	mSweepHits = reinterpret_cast<PxSweepHit*>((size_t(mOrigAddresses[1]) + 15)& ~15);

	PxBatchQueryDesc batchQueryDesc(0, mQueryResultSize, 0);
	batchQueryDesc.queryMemory.userSweepResultBuffer = mSweepResults;
	batchQueryDesc.queryMemory.userSweepTouchBuffer = mSweepHits;
	batchQueryDesc.queryMemory.sweepTouchBufferSize = mHitSize;
	batchQueryDesc.preFilterShader = myFilterShader;

	mBatchQuery = mCrab->getManager()->getScene().createBatchQuery(batchQueryDesc);
}

void SqSweepBuffer::setScene(PxScene* scene)
{
	PxBatchQueryDesc batchQueryDesc(0, mQueryResultSize, 0);
	batchQueryDesc.queryMemory.userSweepResultBuffer = mSweepResults;
	batchQueryDesc.queryMemory.userSweepTouchBuffer = mSweepHits;
	batchQueryDesc.queryMemory.sweepTouchBufferSize = mHitSize;
	batchQueryDesc.preFilterShader = myFilterShader;

	mBatchQuery = scene->createBatchQuery(batchQueryDesc);
}

SqSweepBuffer::~SqSweepBuffer()
{
	mBatchQuery->release();
	free(mOrigAddresses[0]);
	free(mOrigAddresses[1]);
}
