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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.

#include "SceneKaplaTower.h"
#include "Convex.h"
#include "PxSimpleFactory.h"
#include "PxRigidStatic.h"
#include "PxShape.h"
#include "foundation/PxMathUtils.h"
#include "foundation/PxMat44.h"

#include <stdio.h>
#include <GL/glut.h>

#include "SimScene.h"
#include "CompoundCreator.h"
#include "Mesh.h"
#include "TerrainMesh.h"

#include "PhysXMacros.h"

#include "MathUtils.h"

#include "PxRigidBodyExt.h"
#include "PxD6Joint.h"

static PxU32 gMaxNbPermittedProjectiles = 0;

#define PROJECTILES_USE_GOLD_MATERIAL 1


// ----------------------------------------------------------------------------------------------
SceneKaplaTower::SceneKaplaTower(PxPhysics* pxPhysics, PxCooking *pxCooking, bool isGrb,
	Shader *defaultShader, const char *resourcePath, float slowMotionFactor) :
	SceneKapla(pxPhysics, pxCooking, isGrb, defaultShader, resourcePath, slowMotionFactor)
{
	hadInteraction = false;
	mAccumulatedTime = -5.f;
	nbProjectiles = 0;
}

static void finishBody(PxRigidDynamic* dyn, PxReal density, PxReal inertiaScale)
{
	dyn->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
	dyn->setMaxDepenetrationVelocity(2.f);
	PxRigidBodyExt::updateMassAndInertia(*dyn, density);
	dyn->setMassSpaceInertiaTensor(dyn->getMassSpaceInertiaTensor() * inertiaScale);
	dyn->setAngularDamping(0.15f);
}

void SceneKaplaTower::preSim(float dt)
{
	SceneKapla::preSim(dt);
}

void SceneKaplaTower::postSim(float dt)
{

	if (hadInteraction)
	{
		mAccumulatedTime = -5.f;
		hadInteraction = false;
	}

	mAccumulatedTime += dt;


	if (mAccumulatedTime > 0.25f && nbProjectiles < gMaxNbPermittedProjectiles)
	{
		nbProjectiles++;
		mAccumulatedTime = 0.f;

		float wx = randRange(0.5f, 1.5f);
		float wz = randRange(0.5f, 1.0f);
		float h = randRange(0.5f, 0.8f);

		ShaderMaterial mat;
		mat.init();

		PxReal angle = randRange(0.f, 3.14159*2.f); //Random direction...

		PxReal range = randRange(30.f, 40.f);

		PxVec3 pos(range * cosf(angle), 2.f, range * sinf(angle) - 22.f);

		PxReal speed = randRange(10.f, 60.f);

		PxVec3 vel = (PxVec3(0, 0, -22) - pos).getNormalized() * speed + PxVec3(0.f, 250.f/speed, 0.f);

		PxVec3 omega(10.0f, 30.0f, 20.0f);
		int randomNumber = rand();

		bool createBody = true;

		switch (randomNumber % 4)
		{
			case 0:
			{
				float r = randRange(0.1f, 0.2f);
				float R = r + randRange(0.4f, 0.5f);
				mSimScene->getCompoundCreator()->createTorus(R, r, 15, 10);
				break;
			}
			case 1:
			{
				float minLen = 0.6f;
				float maxLen = 0.9f;
				PxVec3 dims(randRange(minLen, maxLen), randRange(minLen, maxLen), randRange(minLen, maxLen));
				mSimScene->getCompoundCreator()->createBox(dims);
				break;
			}
			case 2:
			{
				float r = randRange(0.5f, 0.5f);
				float h = randRange(0.6f, 0.8f);
				mSimScene->getCompoundCreator()->createCylinder(r, h, 15);
				break;
			}
			case 3:
			{
				vel += PxVec3(0.f, 5.f, 0.f);
				createRagdoll(pos, vel, mat);
				createBody = false;
				break;
			}
		}
		if (createBody)
		{
			Compound* compound = createObject(pos, vel, omega, false, mat, false

#if PROJECTILES_USE_GOLD_MATERIAL				
				, 0, 1, 1
#endif
				);
			PxRigidDynamic* dyn = compound->getPxActor();
			finishBody(dyn, 2.f, 1.f);

			if ((randomNumber % 4) == 0)
			{
				PxU32 nbLinks = ((randomNumber / 4) & 3);
				PxTransform trans(pos);

				PxQuat deltaRot(3.1415f / 2.f, PxVec3(1.f, 0.f, 0.f));

				for (PxU32 a = 1; a < nbLinks; ++a)
				{
					trans.q = (trans.q * deltaRot).getNormalized();
					trans.p.x += 0.8f;
					Compound* compound = createObject(trans, vel, omega, false, mat, false
#if PROJECTILES_USE_GOLD_MATERIAL		
						, 0, 1, 1
#endif
						);
					PxRigidDynamic* dyn = compound->getPxActor();
					finishBody(dyn, 2.f, 1.f);
				}
			}

		}
		
	}

	SceneKapla::postSim(dt);
}

void SceneKaplaTower::onInit(PxScene* pxScene)
{
	SceneKapla::onInit(pxScene);
	createGroundPlane();

	mAccumulatedTime = 0.f;
	const PxVec3 dims(0.08f, 0.25f, 1.0f);
	PxMaterial* DefaultMaterial = mPxPhysics->createMaterial(0.5f, 0.25f, 0.1f);

	ShaderMaterial mat;
	mat.init();

	const PxU32 nbInnerInnerRadialLayouts = 40;
	const PxU32 nbInnerRadialLayouts = 48;
	const PxU32 nbMidRadialLayouts = 72;
	const PxU32 nbOuterRadialLayouts = 96;
	const PxU32 nbOuterOuterRadialLayouts = 128;

	const PxReal innerInnerRadius = 2.5f;
	const PxReal innerRadius = 4.5f;
	const PxReal midRadius = 6.5f;
	const PxReal outerRadius = 8.5f;
	const PxReal outerOuterRadius = 10.5f;


	/*createRectangularTower(10, 10, 4, dims, PxVec3(10, 0.f, -30), DefaultMaterial, mat);
	createRectangularTower(8, 8, 7, dims, PxVec3(10, 0.f, -30), DefaultMaterial, mat);
	createRectangularTower(6, 6, 10, dims, PxVec3(10, 0.f, -30), DefaultMaterial, mat);
	createRectangularTower(4, 4, 14, dims, PxVec3(10, 0.f, -30), DefaultMaterial, mat);
	createRectangularTower(2, 2, 18, dims, PxVec3(10, 0.f, -30), DefaultMaterial, mat);*/

	PxFilterData queryFilterData;
	PxFilterData simFilterData;
	createCylindricalTower(nbInnerInnerRadialLayouts, innerInnerRadius, innerInnerRadius, 22, dims, PxVec3(0, 0.f, -22), DefaultMaterial, mat, simFilterData, queryFilterData,1,false,true);
	createCylindricalTower(nbInnerRadialLayouts, innerRadius, innerRadius, 15, dims, PxVec3(0, 0.f, -22), DefaultMaterial, mat, simFilterData, queryFilterData, 1, false, true);
	createCylindricalTower(nbMidRadialLayouts, midRadius, midRadius, 11, dims, PxVec3(0, 0.f, -22), DefaultMaterial, mat, simFilterData, queryFilterData, 1, false, true);
	createCylindricalTower(nbOuterRadialLayouts, outerRadius, outerRadius, 8, dims, PxVec3(0, 0.f, -22), DefaultMaterial, mat, simFilterData, queryFilterData, 1, false, true);
	createCylindricalTower(nbOuterOuterRadialLayouts, outerOuterRadius, outerOuterRadius, 6, dims, PxVec3(0, 0.f, -22), DefaultMaterial, mat, simFilterData, queryFilterData, 1, false, true);

	createTwistTower(30, 6, dims, PxVec3(13, 0, -28), DefaultMaterial, mat);
	createTwistTower(30, 6, dims, PxVec3(13, 0, -23), DefaultMaterial, mat);
	createTwistTower(30, 6, dims, PxVec3(13, 0, -18), DefaultMaterial, mat);
	createTwistTower(30, 6, dims, PxVec3(13, 0, -13), DefaultMaterial, mat);


	

	const PxReal capRadius = PxSqrt(dims.z*dims.z + dims.z*dims.z);

	mSimScene->getCompoundCreator()->createCylinder(capRadius, 2*dims.y, 20);

	Compound* compound0 = createObject(PxTransform(PxVec3(-20, dims.y + 15 * 2 * dims.y, -35.5), PxQuat(3.1415 / 2.f, PxVec3(1.f, 0.f, 0.f))), PxVec3(0), PxVec3(0), false, mat);
	Compound* compound1 = createObject(PxTransform(PxVec3(20, dims.y + 15 * 2 * dims.y, -35.5), PxQuat(3.1415 / 2.f, PxVec3(1.f, 0.f, 0.f))), PxVec3(0), PxVec3(0), false, mat);
	Compound* compound2 = createObject(PxTransform(PxVec3(-20, dims.y + 15 * 2 * dims.y, -9.5), PxQuat(3.1415 / 2.f, PxVec3(1.f, 0.f, 0.f))), PxVec3(0), PxVec3(0), false, mat);
	Compound* compound3 = createObject(PxTransform(PxVec3(20, dims.y + 15 * 2 * dims.y, -9.5), PxQuat(3.1415 / 2.f, PxVec3(1.f, 0.f, 0.f))), PxVec3(0), PxVec3(0), false, mat);

	PxRigidDynamic* cylinder0 = compound0->getPxActor();
	PxRigidDynamic* cylinder1 = compound1->getPxActor();
	PxRigidDynamic* cylinder2 = compound2->getPxActor();
	PxRigidDynamic* cylinder3 = compound3->getPxActor();

	finishBody(cylinder0, 0.2f, 4.f);
	finishBody(cylinder1, 0.2f, 4.f);
	finishBody(cylinder2, 0.2f, 4.f);
	finishBody(cylinder3, 0.2f, 4.f);

	createGeometricTower(15, 4, dims, PxVec3(-20, 0, -35.5), 1.f, DefaultMaterial, mat);
	createGeometricTower(15, 4, dims, PxVec3(20, 0, -35.5), 1.f, DefaultMaterial, mat);
	
	createGeometricTower(40, 4, dims, PxVec3(-20, 16 * 2 * dims.y, -35.5), 0.74f, DefaultMaterial, mat);
	createGeometricTower(40, 4, dims, PxVec3(20, 16 * 2 * dims.y, -35.5), 0.74f, DefaultMaterial, mat);

	createGeometricTower(15, 4, dims, PxVec3(-20, 0, -9.5), 1.f, DefaultMaterial, mat);
	createGeometricTower(15, 4, dims, PxVec3(20, 0, -9.5), 1.f, DefaultMaterial, mat);

	createGeometricTower(40, 4, dims, PxVec3(-20, 16 * 2 * dims.y, -9.5), 0.74f, DefaultMaterial, mat);
	createGeometricTower(40, 4, dims, PxVec3(20, 16 * 2 * dims.y, -9.5), 0.74f, DefaultMaterial, mat);

	

	createCommunicationWire(PxVec3(-20, dims.y / 2.f + 15 * 2 * dims.y, -35.5), PxVec3(20, dims.y / 2.f + 15 * 2 * dims.y, -35.5), 0.15f, 0.75f, 0.05f, capRadius,
		cylinder0, cylinder1, DefaultMaterial, mat, PxQuat(3.14159f/2.f, PxVec3(0,1,0)));

	createCommunicationWire(PxVec3(-20, dims.y / 2.f + 15 * 2 * dims.y, -9.5), PxVec3(20, dims.y / 2.f + 15 * 2 * dims.y, -9.5), 0.15f, 0.75f, 0.05f, capRadius,
		cylinder2, cylinder3, DefaultMaterial, mat, PxQuat(3.14159f / 2.f, PxVec3(0, 1, 0)));

	createCommunicationWire(PxVec3(-20, dims.y / 2.f + 15 * 2 * dims.y, -35.5), PxVec3(-20, dims.y / 2.f + 15 * 2 * dims.y, -9.5), 0.15f, 0.75f, 0.05f, capRadius,
		cylinder0, cylinder2, DefaultMaterial, mat, PxQuat(PxIdentity));

	createCommunicationWire(PxVec3(20, dims.y / 2.f + 15 * 2 * dims.y, -35.5), PxVec3(20, dims.y / 2.f + 15 * 2 * dims.y, -9.5), 0.15f, 0.75f, 0.05f, capRadius,
		cylinder1, cylinder3, DefaultMaterial, mat, PxQuat(PxIdentity));

	



	createRectangularTower(27, 19, 6, dims, PxVec3(0.f, 0.f, -20.f), DefaultMaterial, mat);
}

void SceneKaplaTower::createCommunicationWire(PxVec3 startPos, PxVec3 endPos,
	PxReal connectRadius, PxReal connectHeight, PxReal density, PxReal offset, PxRigidDynamic* startBody, PxRigidDynamic* endBody, PxMaterial* material, ShaderMaterial& mat,
	PxQuat& rot)
{
	mSimScene->getCompoundCreator()->createCylinder(connectRadius, connectHeight, 8);

	PxVec3 layDir = (endPos - startPos).getNormalized();

	PxReal distance = (startPos - endPos).magnitude() - 2.f * offset;

	PxU32 nbToConnect = (distance) / connectHeight;

	PxReal gap = distance / nbToConnect;

	PxVec3 pos = startPos + layDir* (offset + connectHeight / 2.f);

	PxRigidDynamic* rootActor = startBody;

	PxVec3 anchorPos = startPos + layDir * offset;

	PxPhysics* physics = getSimScene()->getPxPhysics();

	PxRigidDynamic** dyns = new PxRigidDynamic*[nbToConnect];

	for (PxU32 a = 0; a < nbToConnect; ++a)
	{
		Compound* compound = createObject(PxTransform(pos, rot), PxVec3(0), PxVec3(0), false, mat);

		PxRigidDynamic* dyn = compound->getPxActor();

		dyns[a] = dyn;

		finishBody(dyn, density, 10.f);
		dyn->setLinearDamping(0.1f);
		dyn->setAngularDamping(0.15f);
		dyn->setStabilizationThreshold(0.f);

		//Now create the constraint
		PxD6Joint *joint = PxD6JointCreate(*physics, rootActor,
			PxTransform(rootActor->getGlobalPose().transformInv(anchorPos)), dyn, PxTransform(dyn->getGlobalPose().transformInv(anchorPos)));

		joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
		joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);
		joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);

		//if (a == 0)
		{
			joint->setBreakForce((dyn->getMass() + rootActor->getMass()) * 3000.f, (dyn->getMass() + rootActor->getMass()) * 2000.f);
		}

		joint->setDrive(PxD6Drive::eSWING, PxD6JointDrive(0.f, 0.15f, PX_MAX_F32));
		joint->setDrive(PxD6Drive::eTWIST, PxD6JointDrive(0.f, 0.15f, PX_MAX_F32));

		pos += layDir * gap;
		anchorPos += layDir * gap;
		rootActor = dyn;
	}

	PxD6Joint *joint = PxD6JointCreate(*physics, rootActor,
		PxTransform(rootActor->getGlobalPose().transformInv(anchorPos)), endBody, PxTransform(endBody->getGlobalPose().transformInv(anchorPos)));

	joint->setBreakForce((endBody->getMass() + rootActor->getMass()) * 3000.f, (endBody->getMass() + rootActor->getMass()) * 2000.f);

	joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
	joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);
	joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);

	joint->setDrive(PxD6Drive::eSWING, PxD6JointDrive(0.f, 0.15f, PX_MAX_F32));
	joint->setDrive(PxD6Drive::eTWIST, PxD6JointDrive(0.f, 0.15f, PX_MAX_F32));

	//Now add some bunting to the connections...

	mSimScene->getCompoundCreator()->createBox(PxVec3(connectRadius, connectHeight, connectHeight));

	for (PxU32 a = 2; a < nbToConnect; a += 3)
	{
		//Create a little box to attach

		PxRigidDynamic* rootActor = dyns[a];
		PxTransform transform = rootActor->getGlobalPose();

		transform.p += PxVec3(0.f, -(connectRadius + 0.5*connectHeight), 0.f);

		Compound* compound = createObject(transform, PxVec3(0), PxVec3(0), false, mat);

		PxRigidDynamic* dyn = compound->getPxActor();

		finishBody(dyn, density*0.5f, 10.f);
		dyn->setLinearDamping(0.1f);
		dyn->setAngularDamping(0.15f);
		dyn->setStabilizationThreshold(0.f);

		anchorPos = transform.p + PxVec3(0, 0.5f*connectHeight, 0);

		PxD6Joint *joint = PxD6JointCreate(*physics, rootActor,
			PxTransform(rootActor->getGlobalPose().transformInv(anchorPos)), dyn, PxTransform(dyn->getGlobalPose().transformInv(anchorPos)));

		joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLOCKED);
		joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLOCKED);
		joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLOCKED);




	}


	delete dyns;



}


void SceneKaplaTower::createGeometricTower(PxU32 height, PxU32 nbFacets, PxVec3 dims, PxVec3 startPos, PxReal startDensity, PxMaterial* material, ShaderMaterial& mat)
{
	PX_UNUSED(nbFacets);
	//Create a tower of geometric shaps
	PxReal angle = (3.14159 / 2.f); //the angle between each facet...
	PxQuat zRot(3.1415 / 2.f, PxVec3(0, 1, 0));

	PxQuat rotation[] = { PxQuat(PxIdentity), PxQuat(3.1415 / 4.f, PxVec3(0, 1, 0)) };

	PxReal offset = dims.z;
	PxReal density = startDensity;

	PxShape* shape;

	PxVec3 vel(0), omega(0);

	mSimScene->getCompoundCreator()->createBox(2.f*dims);//0.5f, 0.2f, 20, 20);

	for (PxU32 i = 0; i < height; ++i)
	{
		PxVec3 yOffset(0, dims.y + i * dims.y * 2, 0);
		PxQuat rot = rotation[i & 1];

		PxVec3 basis0 = rot.getBasisVector0();
		PxVec3 basis2 = rot.getBasisVector2();

		PxVec3 pos0 = -basis0 * offset + basis2 * dims.x;
		PxVec3 pos1 = basis0 * offset - basis2 * dims.x;
		PxVec3 pos2 = basis0 * dims.x + basis2 * offset;
		PxVec3 pos3 = -basis0 * dims.x - basis2 * offset;

		//PxRigidDynamic* box0 = PxTkCreateDynamicBox(*mSDK, mScene, material, mObjectCreationParams, pos0 + startPos + yOffset, rot, dims);
		//PxRigidDynamic* box1 = PxTkCreateDynamicBox(*mSDK, mScene, material, mObjectCreationParams, pos1 + startPos + yOffset, rot, dims);
		//PxRigidDynamic* box2 = PxTkCreateDynamicBox(*mSDK, mScene, material, mObjectCreationParams, pos2 + startPos + yOffset, zRot * rot, dims);
		//PxRigidDynamic* box3 = PxTkCreateDynamicBox(*mSDK, mScene, material, mObjectCreationParams, pos3 + startPos + yOffset, zRot * rot, dims);

		Compound* compound0 = createObject(PxTransform(pos0 + startPos + yOffset, rot), vel, omega, false, mat);
		PxRigidDynamic* box0 = compound0->getPxActor();
		box0->getShapes(&shape, 1);
		shape->setMaterials(&material, 1);

		Compound* compound1 = createObject(PxTransform(pos1 + startPos + yOffset, rot), vel, omega, false, mat);
		PxRigidDynamic* box1 = compound1->getPxActor();
		box1->getShapes(&shape, 1);
		shape->setMaterials(&material, 1);

		Compound* compound2 = createObject(PxTransform(pos2 + startPos + yOffset, zRot * rot), vel, omega, false, mat);
		PxRigidDynamic* box2 = compound2->getPxActor();
		box2->getShapes(&shape, 1);
		shape->setMaterials(&material, 1);

		Compound* compound3 = createObject(PxTransform(pos3 + startPos + yOffset, zRot * rot), vel, omega, false, mat);
		PxRigidDynamic* box3 = compound3->getPxActor();
		box3->getShapes(&shape, 1);
		shape->setMaterials(&material, 1);

		box0->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
		box0->setMaxDepenetrationVelocity(2.f);
		PxRigidBodyExt::updateMassAndInertia(*box0, density);
		box0->setMassSpaceInertiaTensor(box0->getMassSpaceInertiaTensor() * 4.f);

		box1->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
		box1->setMaxDepenetrationVelocity(2.f);
		PxRigidBodyExt::updateMassAndInertia(*box1, density);
		box1->setMassSpaceInertiaTensor(box1->getMassSpaceInertiaTensor() * 4.f);

		box2->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
		box2->setMaxDepenetrationVelocity(2.f);
		PxRigidBodyExt::updateMassAndInertia(*box2, density);
		box2->setMassSpaceInertiaTensor(box2->getMassSpaceInertiaTensor() * 4.f);

		box3->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
		box3->setMaxDepenetrationVelocity(2.f);
		PxRigidBodyExt::updateMassAndInertia(*box3, density);
		box3->setMassSpaceInertiaTensor(box3->getMassSpaceInertiaTensor() * 4.f);

		density = density * 0.98f;

	}



}


void SceneKaplaTower::createTwistTower(PxU32 height, PxU32 nbBlocksPerLayer, PxVec3 dims, PxVec3 startPos, PxMaterial* material, ShaderMaterial& mat)
{
	PxQuat rotation(PxIdentity);//3.14/2.f, PxVec3(0,0,1));
	PxQuat rotationDelta(3.14 / 10.f, PxVec3(0, 1, 0));
	PxReal density = 1.f;
	PxShape* shape;
	PxVec3 vel(0), omega(0);

	mSimScene->getCompoundCreator()->createBox(2.f*dims);//0.5f, 0.2f, 20, 20);

	for (PxU32 i = 0; i < height; ++i)
	{
		PxReal startY = i * dims.y * 2 + dims.y;
		PxVec3 xVec = rotation.getBasisVector0();
		for (PxU32 a = 0; a < nbBlocksPerLayer; ++a)
		{
			PxVec3 pos = xVec * dims.x * 2 * (PxReal(a) - PxReal(nbBlocksPerLayer) / 2.f) + PxVec3(0, startY, 0);
			//PxRigidDynamic* box = PxTkCreateDynamicBox(*mSDK, mScene, material, mObjectCreationParams, pos + startPos, rotation, dims);
			Compound* compound = createObject(PxTransform(pos + startPos, rotation), vel, omega, false, mat);
			PxRigidDynamic* box = compound->getPxActor();
			box->getShapes(&shape, 1);
			shape->setMaterials(&material, 1);

			box->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
			box->setMaxDepenetrationVelocity(2.f);
			PxRigidBodyExt::updateMassAndInertia(*box, density);
			box->setMassSpaceInertiaTensor(box->getMassSpaceInertiaTensor() * 4.f);
		}

		rotation = rotationDelta * rotation;
		density *= 0.95f;
	}
}


void SceneKaplaTower::createRectangularTower(PxU32 nbX, PxU32 nbZ, PxU32 height, PxVec3 dims, PxVec3 centerPos, PxMaterial* material, ShaderMaterial& mat)
{
	PxReal startHeight = 0.f;
	PxReal density = 1.f;

	PxU32 nbXSupports = 1 + 2 * nbX;

	PxReal xSpacing = dims.z;

	PxU32 nbZSlabs = (2.f*dims.z * nbZ) / (2.f*dims.y);

	PxQuat rotation = PxQuat(3.14 / 2.f, PxVec3(0, 1, 0)) * PxQuat(3.14 / 2.f, PxVec3(0, 0, 1));

	mSimScene->getCompoundCreator()->createBox(2.f*dims);//0.5f, 0.2f, 20, 20);
	PxVec3 pos(-2.0f, 1.5f, 0.0f);
	PxVec3 vel(0.0f, 0.0f, 0.0f);
	PxVec3 omega(0.0f, 0.0f, 0.0f);

	for (PxU32 i = 0; i < height; ++i)
	{
		//First, lets lay down the supports...

		PxVec3 rowColExtents((nbX)* dims.z, 0.f, (nbZ)* dims.z);

		PxShape* shape;

		//Back row
		for (PxU32 a = 0; a < nbXSupports; ++a)
		{
			PxVec3 pos(a * xSpacing - rowColExtents.x, startHeight + dims.y, -rowColExtents.z);
			PxVec3 pos2(a * xSpacing - rowColExtents.x, startHeight + dims.y, rowColExtents.z);

			Compound* compound = createObject(PxTransform(pos + centerPos), vel, omega, false, mat);
			PxRigidDynamic* box = compound->getPxActor();
			box->getShapes(&shape, 1);
			shape->setMaterials(&material, 1);

			box->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
			box->setMaxDepenetrationVelocity(2.f);
			PxRigidBodyExt::updateMassAndInertia(*box, density);
			box->setMassSpaceInertiaTensor(box->getMassSpaceInertiaTensor() * 4.f);

			Compound* compound2 = createObject(PxTransform(pos2 + centerPos), vel, omega, false, mat);
			PxRigidDynamic* box2 = compound2->getPxActor();
			box2->getShapes(&shape, 1);
			shape->setMaterials(&material, 1);

			box2->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
			box2->setMaxDepenetrationVelocity(2.f);
			PxRigidBodyExt::updateMassAndInertia(*box2, density);
			box2->setMassSpaceInertiaTensor(box2->getMassSpaceInertiaTensor() * 4.f);
		}

		for (PxU32 a = 0; a < nbX; ++a)
		{
			PxVec3 pos(dims.z + a * dims.z * 2 - rowColExtents.x, startHeight + 2 * dims.y + dims.x, -rowColExtents.z - dims.y);
			PxVec3 pos2(dims.z + a * dims.z * 2 - rowColExtents.x, startHeight + 2 * dims.y + dims.x, -rowColExtents.z + dims.y);

			Compound* compound = createObject(PxTransform(pos + centerPos, rotation), vel, omega, false, mat);
			PxRigidDynamic* box = compound->getPxActor();
			box->getShapes(&shape, 1);
			shape->setMaterials(&material, 1);

			//PxRigidDynamic* box2 = PxTkCreateDynamicBox(*mSDK, mScene, material, mObjectCreationParams, pos2 + centerPos, rotation, dims);
			Compound* compound2 = createObject(PxTransform(pos2 + centerPos, rotation), vel, omega, false, mat);
			PxRigidDynamic* box2 = compound2->getPxActor();
			box2->getShapes(&shape, 1);
			shape->setMaterials(&material, 1);

			box->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
			box->setMaxDepenetrationVelocity(2.f);
			PxRigidBodyExt::updateMassAndInertia(*box, density);
			box->setMassSpaceInertiaTensor(box->getMassSpaceInertiaTensor() * 4.f);

			box2->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
			box2->setMaxDepenetrationVelocity(2.f);
			PxRigidBodyExt::updateMassAndInertia(*box2, density);
			box2->setMassSpaceInertiaTensor(box2->getMassSpaceInertiaTensor() * 4.f);


			PxVec3 pos3(dims.z + a * dims.z * 2 - rowColExtents.x, startHeight + 2 * dims.y + dims.x, rowColExtents.z - dims.y);
			PxVec3 pos4(dims.z + a * dims.z * 2 - rowColExtents.x, startHeight + 2 * dims.y + dims.x, rowColExtents.z + dims.y);

			//PxRigidDynamic* box3 = PxTkCreateDynamicBox(*mSDK, mScene, material, mObjectCreationParams, pos3 + centerPos, rotation, dims);

			Compound* compound3 = createObject(PxTransform(pos3 + centerPos, rotation), vel, omega, false, mat);
			PxRigidDynamic* box3 = compound3->getPxActor();
			box3->getShapes(&shape, 1);
			shape->setMaterials(&material, 1);

			//PxRigidDynamic* box4 = PxTkCreateDynamicBox(*mSDK, mScene, material, mObjectCreationParams, pos4 + centerPos, rotation, dims);
			Compound* compound4 = createObject(PxTransform(pos4 + centerPos, rotation), vel, omega, false, mat);
			PxRigidDynamic* box4 = compound4->getPxActor();
			box4->getShapes(&shape, 1);
			shape->setMaterials(&material, 1);

			box3->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
			box3->setMaxDepenetrationVelocity(2.f);
			PxRigidBodyExt::updateMassAndInertia(*box3, density);
			box3->setMassSpaceInertiaTensor(box3->getMassSpaceInertiaTensor() * 4.f);

			box4->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
			box4->setMaxDepenetrationVelocity(2.f);
			PxRigidBodyExt::updateMassAndInertia(*box4, density);
			box4->setMassSpaceInertiaTensor(box4->getMassSpaceInertiaTensor() * 4.f);

		}

		//Sides...
		for (PxU32 a = 1; a < nbZ; ++a)
		{
			for (PxU32 b = 0; b < 3; b++)
			{
				PxVec3 pos(b * xSpacing - rowColExtents.x, startHeight + dims.y, -rowColExtents.z + a * dims.z * 2);
				PxVec3 pos2(rowColExtents.x - b * xSpacing, startHeight + dims.y, -rowColExtents.z + a * dims.z * 2);

				//PxRigidDynamic* box = PxTkCreateDynamicBox(*mSDK, mScene, material, mObjectCreationParams, pos + centerPos, PxQuat(PxIdentity), dims);
				Compound* compound = createObject(PxTransform(pos + centerPos), vel, omega, false, mat);
				PxRigidDynamic* box = compound->getPxActor();
				box->getShapes(&shape, 1);
				shape->setMaterials(&material, 1);

				box->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
				box->setMaxDepenetrationVelocity(2.f);
				PxRigidBodyExt::updateMassAndInertia(*box, density);
				box->setMassSpaceInertiaTensor(box->getMassSpaceInertiaTensor() * 4.f);

				//PxRigidDynamic* box2 = PxTkCreateDynamicBox(*mSDK, mScene, material, mObjectCreationParams, pos2 + centerPos, PxQuat(PxIdentity), dims);
				Compound* compound2 = createObject(PxTransform(pos2 + centerPos), vel, omega, false, mat);
				PxRigidDynamic* box2 = compound2->getPxActor();
				box2->getShapes(&shape, 1);
				shape->setMaterials(&material, 1);

				box2->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
				box2->setMaxDepenetrationVelocity(2.f);
				PxRigidBodyExt::updateMassAndInertia(*box2, density);
				box2->setMassSpaceInertiaTensor(box2->getMassSpaceInertiaTensor() * 4.f);
			}
		}

		for (PxU32 a = 1; a < nbZSlabs - 1; a++)
		{
			PxVec3 pos(dims.z - rowColExtents.x, startHeight + 2 * dims.y + dims.x, -rowColExtents.z + dims.y + 2 * dims.y*a);

			//PxRigidDynamic* box = PxTkCreateDynamicBox(*mSDK, mScene, material, mObjectCreationParams, pos + centerPos, rotation, dims);
			Compound* compound = createObject(PxTransform(pos + centerPos, rotation), vel, omega, false, mat);
			PxRigidDynamic* box = compound->getPxActor();
			box->getShapes(&shape, 1);
			shape->setMaterials(&material, 1);

			box->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
			box->setMaxDepenetrationVelocity(2.f);
			PxRigidBodyExt::updateMassAndInertia(*box, density);
			box->setMassSpaceInertiaTensor(box->getMassSpaceInertiaTensor() * 4.f);
		}

		for (PxU32 a = 1; a < nbZSlabs - 1; a++)
		{
			PxVec3 pos(rowColExtents.x - dims.z, startHeight + 2 * dims.y + dims.x, -rowColExtents.z + dims.y + 2 * dims.y*a);

			//PxRigidDynamic* box = PxTkCreateDynamicBox(*mSDK, mScene, material, mObjectCreationParams, pos + centerPos, rotation, dims);
			Compound* compound = createObject(PxTransform(pos + centerPos, rotation), vel, omega, false, mat);
			PxRigidDynamic* box = compound->getPxActor();
			box->getShapes(&shape, 1);
			shape->setMaterials(&material, 1);

			box->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
			box->setMaxDepenetrationVelocity(2.f);
			PxRigidBodyExt::updateMassAndInertia(*box, density);
			box->setMassSpaceInertiaTensor(box->getMassSpaceInertiaTensor() * 4.f);
		}

		startHeight += 2 * (dims.y + dims.x);
		density *= 0.975f;

	}
}

// ----------------------------------------------------------------------------------------------
void SceneKaplaTower::handleKeyDown(unsigned char key, int x, int y)
{
	switch (key) {
	case 'b':
	case 'B':
	{
		if (gMaxNbPermittedProjectiles == 0)
			gMaxNbPermittedProjectiles = 512;
		else
			gMaxNbPermittedProjectiles = 0;

		break;
	}
	}

	SceneKapla::handleKeyDown(key, x, y);
}