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

#include "SceneKapla.h"
#include "Convex.h"
#include "PxSimpleFactory.h"
#include "PxRigidStatic.h"
#include "PxShape.h"
#include "foundation/PxMathUtils.h"
#include "foundation/PxMat44.h"

#include <stdio.h>
#include <GL/glut.h>

#include "ShaderShadow.h"
#include "SimScene.h"
#include "CompoundCreator.h"
#include "Mesh.h"
#include "TerrainMesh.h"

#include "PhysXMacros.h"

#include "MathUtils.h"

#include "PxRigidBodyExt.h"
#include "MediaPath.h"

#include "PxD6Joint.h"
#include "PxScene.h"

extern bool gDrawGroundPlane;
extern PxVec3 gGroupPlanePose;
extern float gGroundY;
SceneKapla::Weapon SceneKapla::mWeapon = wpBullet;

std::vector<Compound*> meteors;



extern bool LoadTexture(const char *filename, GLuint &texId, bool createMipmaps, GLuint type = GL_TEXTURE_2D, int *width = NULL, int *height = NULL);

// ----------------------------------------------------------------------------------------------
SceneKapla::SceneKapla(PxPhysics* pxPhysics, PxCooking *pxCooking, bool isGrb,
		Shader *defaultShader, const char *resourcePath, float slowMotionFactor) : 
		SampleViewerScene(pxPhysics, pxCooking, isGrb, defaultShader, resourcePath, slowMotionFactor),
	mDefaultContactOffset(0.02f), mDefaultRestOffset(0.0f)

{
	mShaders.clear();
	mFrameNr = 0;
	mWeaponLifeTime = 500;	// frames
	mLaserImpulse = 0.0f;
	mWeaponImpulse = 5.0f;

	float minConvexSize = 0.02f;
	mMeteorDistance = 10.0f;
	mMeteorVelocity = 20.0f;
	mMeteorSize = 1.0f;

	mSimScene = SimScene::createSimScene(pxPhysics, pxCooking, isGrb, minConvexSize, mDefaultMaterial, resourcePath);
	mSimScene->setContactImpactRadius(0.0f);

	ShaderShadow::SHADER3D_CHOICES shader3d = ShaderShadow::COMBINE;
	ShaderShadow::PS_MODE psMode = ShaderShadow::PS_SHADE3D;

	mSimSceneShader = new ShaderShadow(ShaderShadow::VS_TEXINSTANCE, psMode, shader3d);
	mSimSceneShader->init();
	mShaders.push_back(mSimSceneShader);

	ShaderMaterial mat;
	mat.init();

	// For asteroid
	float bumpTextureUVScale = 0.1f;
	float roughnessScale = 0.2f;
	float extraNoiseScale = 2.0f;
	
	LoadTexture("stoneBump.bmp", mat.texId, true);
	mSimScene->setShaderMaterial(mSimSceneShader, mat);

	mBulletMesh = PX_NEW(Mesh)();
	mBulletMesh->loadFromObjFile(std::string(mResourcePath) + "\\bullet.obj");
	mBulletMesh->normalize(PxVec3(0.0f, 0.0f, 0.0f), 0.6f);

	mAsteroidMesh = PX_NEW(Mesh)();
	mAsteroidMesh->loadFromObjFile(std::string(mResourcePath) + "\\asteroid.obj");

	

	mGunActive = false;
	mMouseX = 1024 / 2;
	mMouseY = 768 / 2;
	mMouseDown = false;

	mPickQuadric = NULL;

	mTerrain = NULL;
	gDrawGroundPlane = false;

	const PxVec3 dims(0.08f, 0.25f, 1.0f);

	mSimScene->diffuseTexNames.push_back(std::string(mResourcePath) + "\\checker_tex0.bmp");
	mSimScene->diffuseTexNames.push_back(std::string(mResourcePath) + "\\checker_tex1.bmp");
	mSimScene->diffuseTexNames.push_back(std::string(mResourcePath) + "\\checker_tex2.bmp");
	mSimScene->diffuseTexNames.push_back(std::string(mResourcePath) + "\\checker_tex3.bmp");
	mSimScene->diffuseTexNames.push_back(std::string(mResourcePath) + "\\checker_tex4.bmp");

	PxMaterial* DefaultMaterial = pxPhysics->createMaterial(0.4f, 0.15f, 0.1f);

	mSimScene->setFractureForceThreshold(PX_MAX_F32);

	mAsteroidMesh->normalize(PxVec3(0.0f, 0.0f, 0.0f), mMeteorSize);


	debugDrawNumConvexes = 0;
	mSimScene->createRenderBuffers();

}

void SceneKapla::onInit(PxScene* pxScene)
{
	SampleViewerScene::onInit(pxScene);
	mSimScene->setScene(pxScene);
}

void SceneKapla::setScene(PxScene* pxScene) 
{ 
	SampleViewerScene::setScene(pxScene); mSimScene->setScene(pxScene); 
}

void SceneKapla::createGroundPlane(PxFilterData simFilterData, PxVec3 pose, PxFilterData queryFilterData)
{
	// ground plane
	PxRigidStatic* groundPlane = PxCreatePlane(*mPxPhysics, PxPlane(0, 1.f, 0, 0.f), *mDefaultMaterial);

	gGroupPlanePose =pose;
	PxTransform transform(gGroupPlanePose, groundPlane->getGlobalPose().q);
	groundPlane->setGlobalPose(transform);

	
	PxShape * groundShape;
	groundPlane->getShapes(&groundShape, 1);
	groundShape->setSimulationFilterData(simFilterData);
	//groundShape->setContactOffset(mDefaultContactOffset);
	//groundShape->setRestOffset(mDefaultRestOffset);

	groundShape->setQueryFilterData(queryFilterData);


	mPxScene->addActor(*groundPlane);

	gDrawGroundPlane = true;
}

void SceneKapla::createTerrain(const char* terrainName, const char* textureName, PxReal maxHeight, bool invert)
{
	string fullTerrainName = FindMediaFile(terrainName, mResourcePath);
	string fullTerrainTexName = (strcmp(textureName, "") == 0) ? textureName:FindMediaFile(textureName, mResourcePath);

	mTerrain = new TerrainMesh(getPhysics(), getCooking(), getScene(), *mDefaultMaterial, PxVec3(-1.0f, -30.85f, -1.0f), fullTerrainName.c_str(), 
		fullTerrainTexName.c_str(), 10.f, maxHeight, mDefaultShader, invert);
}

static void configD6Joint(PxReal swing0, PxReal swing1, PxReal twistLo, PxReal twistHi, PxD6Joint* joint)
{
	joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
	joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLIMITED);
	joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLIMITED);

	joint->setSwingLimit(PxJointLimitCone(swing0, swing1));
	joint->setTwistLimit(PxJointAngularLimitPair(twistLo, twistHi));
}

static void finishBody(PxRigidDynamic* dyn, PxReal density, PxReal inertiaScale)
{
	dyn->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
	dyn->setMaxDepenetrationVelocity(2.f);
	PxRigidBodyExt::updateMassAndInertia(*dyn, density);
	dyn->setMassSpaceInertiaTensor(dyn->getMassSpaceInertiaTensor() * inertiaScale);
	dyn->setAngularDamping(0.15f);
}

void SceneKapla::createRagdoll(PxVec3 pos, PxVec3 vel, ShaderMaterial& mat)
{

	PxVec3 tempVel = vel;
	vel = vel * 0.75f;

	PxReal density = 4.f;
	//Head (a sphere)

	PxPhysics* physics = getSimScene()->getPxPhysics();

	mSimScene->getCompoundCreator()->createSphere(PxVec3(0.4f), 4); //Spheres defined by dims - not radius!

	Compound* head = createObject(PxTransform(pos + PxVec3(0.f, 0.9f, 0.f)), vel, PxVec3(0), false, mat);

	mSimScene->getCompoundCreator()->createCylinder(0.1f, 0.25f, 10);

	Compound* neck = createObject(PxTransform(pos + PxVec3(0.f, 0.65f, 0.f), PxQuat(3.14159 / 2.f, PxVec3(1.f, 0.f, 0.f))), vel, PxVec3(0), false, mat);

	PxRigidDynamic* headBody = head->getPxActor();
	PxRigidDynamic* neckBody = neck->getPxActor();

	finishBody(headBody, density, 4.f);
	finishBody(neckBody, density, 4.f);

	PxD6Joint *neckJoint = PxD6JointCreate(*physics, headBody,
		headBody->getGlobalPose().transformInv(PxTransform(pos + PxVec3(0.f, 0.75f, -0.05f))), neckBody, neckBody->getGlobalPose().transformInv(PxTransform(pos + PxVec3(0.f, 0.75f, -0.05f))));

	configD6Joint(3.14 / 8.f, 3.14f / 8.f, -3.14f / 8.f, 3.14f / 8.f, neckJoint);

	mSimScene->getCompoundCreator()->createCylinder(0.25f, 0.55f, 10);

	Compound* upperTorso = createObject(PxTransform(pos + PxVec3(0.f, 0.375f, 0.f), PxQuat(3.14159 / 2.f, PxVec3(0.f, 1.f, 0.f))), tempVel, PxVec3(0), false, mat);

	mSimScene->getCompoundCreator()->createCylinder(0.2f, 0.45f, 10);

	Compound* lowerTorso = createObject(PxTransform(pos + PxVec3(0.f, 0.075f, 0.f), PxQuat(3.14159 / 2.f, PxVec3(0.f, 1.f, 0.f))), tempVel, PxVec3(0), false, mat);

	Compound* hips = createObject(PxTransform(pos + PxVec3(0.f, -0.225f, 0.f), PxQuat(3.14159 / 2.f, PxVec3(0.f, 1.f, 0.f))), tempVel, PxVec3(0), false, mat);


	finishBody(upperTorso->getPxActor(), density, 4.f);
	finishBody(lowerTorso->getPxActor(), density, 4.f);
	finishBody(hips->getPxActor(), density, 4.f);


	PxD6Joint *neckTorsoJoint = PxD6JointCreate(*physics, neckBody,
		neckBody->getGlobalPose().transformInv(PxTransform(pos + PxVec3(0.f, 0.6f, -0.05f))), upperTorso->getPxActor(), upperTorso->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(0.f, 0.6f, -0.05f))));

	configD6Joint(3.14 / 8.f, 3.14f / 8.f, -3.14f / 8.f, 3.14f / 8.f, neckTorsoJoint);

	PxD6Joint *TorsoTorsoJoint = PxD6JointCreate(*physics, upperTorso->getPxActor(),
		upperTorso->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(0.f, 0.25f, -0.1f))), lowerTorso->getPxActor(), 
		lowerTorso->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(0.f, 0.25f, -0.1f))));

	configD6Joint(3.14 / 4.f, 3.14f / 4.f, -3.14f / 4.f, 3.14f / 4.f, neckTorsoJoint);

	PxD6Joint *hipTorsoJoint = PxD6JointCreate(*physics, lowerTorso->getPxActor(),
		lowerTorso->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(0.f, -0.1f, -0.1f))), hips->getPxActor(), 
		hips->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(0.f, -0.1f, -0.1f))));

	configD6Joint(3.14 / 4.f, 3.14f / 4.f, -3.14f / 4.f, 3.14f / 4.f, neckTorsoJoint);

	mSimScene->getCompoundCreator()->createCylinder(0.1f, 0.55f, 10);

	Compound* leftShoulder = createObject(PxTransform(pos + PxVec3(-0.48f, 0.375f, 0.f), PxQuat(3.14159 / 2.f, PxVec3(0.f, 1.f, 0.f))), vel, PxVec3(0), false, mat);
	Compound* rightShoulder = createObject(PxTransform(pos + PxVec3(0.48f, 0.375f, 0.f), PxQuat(3.14159 / 2.f, PxVec3(0.f, 1.f, 0.f))), vel, PxVec3(0), false, mat);

	finishBody(leftShoulder->getPxActor(), density, 4.f);
	finishBody(rightShoulder->getPxActor(), density, 4.f);

	PxD6Joint *leftShoulderJoint = PxD6JointCreate(*physics, upperTorso->getPxActor(),
		upperTorso->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(-0.28f, 0.375f, 0.f))), 
		leftShoulder->getPxActor(), leftShoulder->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(-0.28f, 0.375f, 0.f))));

	configD6Joint(3.14 / 2.5f, 3.14f / 2.5f, -3.14f / 2.5f, 3.14f / 2.5f, leftShoulderJoint);

	PxD6Joint *rightShoulderJoint = PxD6JointCreate(*physics, upperTorso->getPxActor(),
		upperTorso->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(0.28f, 0.375f, -0.f))),
		rightShoulder->getPxActor(), rightShoulder->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(0.28f, 0.375f, -0.f))));

	configD6Joint(3.14 / 2.5f, 3.14f / 2.5f, -3.14f / 2.5f, 3.14f / 2.5f, rightShoulderJoint);

	Compound* leftForearm = createObject(PxTransform(pos + PxVec3(-0.88f, 0.375f, 0.f), PxQuat(3.14159 / 2.f, PxVec3(0.f, 1.f, 0.f))), vel, PxVec3(0), false, mat);
	Compound* rightForearm = createObject(PxTransform(pos + PxVec3(0.88f, 0.375f, 0.f), PxQuat(3.14159 / 2.f, PxVec3(0.f, 1.f, 0.f))), vel, PxVec3(0), false, mat);

	finishBody(leftForearm->getPxActor(), density, 4.f);
	finishBody(rightForearm->getPxActor(), density, 4.f);

	PxD6Joint *leftElbowJoint = PxD6JointCreate(*physics, leftShoulder->getPxActor(),
		leftShoulder->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(-0.68f, 0.375f, 0.0f), PxQuat(3.141592f / 2.f, PxVec3(0.f, 1.f, 0.f)))),
		leftForearm->getPxActor(), leftForearm->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(-0.68f, 0.375f, 0.f), PxQuat(3.141592f / 2.f, PxVec3(0.f, 1.f, 0.f)))));

	configD6Joint(3.14 / 16.f, 3.14f / 16.f, -3.14 / 16.f, 3.14f / 2.5f, leftElbowJoint);

	PxD6Joint *rightElbowJoint = PxD6JointCreate(*physics, rightShoulder->getPxActor(),
		rightShoulder->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(0.68f, 0.375f, -0.f), PxQuat(3.141592f/2.f, PxVec3(0.f, 1.f, 0.f)))),
		rightForearm->getPxActor(), rightForearm->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(0.68f, 0.375f, -0.f), PxQuat(3.141592f/2.f, PxVec3(0.f, 1.f, 0.f)))));

	configD6Joint(3.14 / 16.f, 3.14f / 16.f, -3.14 / 16.f, 3.14f / 2.5f, rightElbowJoint);

	mSimScene->getCompoundCreator()->createBox(PxVec3(0.2f, 0.05f, 0.2f));

	Compound* leftHand = createObject(PxTransform(pos + PxVec3(-1.18f, 0.375f, 0.f), PxQuat(3.14159 / 2.f, PxVec3(0.f, 1.f, 0.f))), vel, PxVec3(0), false, mat);
	Compound* rightHand = createObject(PxTransform(pos + PxVec3(1.18f, 0.375f, 0.f), PxQuat(3.14159 / 2.f, PxVec3(0.f, 1.f, 0.f))), vel, PxVec3(0), false, mat);

	finishBody(leftHand->getPxActor(), density, 4.f);
	finishBody(rightHand->getPxActor(), density, 4.f);

	PxD6Joint *leftWristJoint = PxD6JointCreate(*physics, leftForearm->getPxActor(),
		leftForearm->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(-1.13f, 0.375f, 0.0f), PxQuat(3.141592f / 2.f, PxVec3(0.f, 1.f, 0.f)))),
		leftHand->getPxActor(), leftHand->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(-1.13f, 0.375f, 0.f), PxQuat(3.141592f / 2.f, PxVec3(0.f, 1.f, 0.f)))));

	configD6Joint(3.14 / 8.f, 3.14f / 8.f, -3.14 / 2.f, 3.14f / 2.f, leftWristJoint);

	PxD6Joint *rightWristJoint = PxD6JointCreate(*physics, rightForearm->getPxActor(),
		rightForearm->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(1.13f, 0.375f, 0.0f), PxQuat(3.141592f / 2.f, PxVec3(0.f, 1.f, 0.f)))),
		rightHand->getPxActor(), rightHand->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(1.13f, 0.375f, 0.f), PxQuat(3.141592f / 2.f, PxVec3(0.f, 1.f, 0.f)))));

	configD6Joint(3.14 / 8.f, 3.14f / 8.f, -3.14 / 2.f, 3.14f / 2.f, rightWristJoint);


	mSimScene->getCompoundCreator()->createCylinder(0.125f, 0.7f, 10);

	Compound* leftThigh = createObject(PxTransform(pos + PxVec3(-0.15f, -0.7f, 0.f), PxQuat(3.14159 / 2.f, PxVec3(1.f, 0.f, 0.f))), vel, PxVec3(0), false, mat);
	Compound* rightThigh = createObject(PxTransform(pos + PxVec3(0.15f, -0.7f, 0.f), PxQuat(3.14159 / 2.f, PxVec3(1.f, 0.f, 0.f))), vel, PxVec3(0), false, mat);

		finishBody(leftThigh->getPxActor(), density, 4.f);
		finishBody(rightThigh->getPxActor(), density, 4.f);

	PxD6Joint *leftHipJoint = PxD6JointCreate(*physics, hips->getPxActor(),
		hips->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(-0.15f, -0.4f, 0.f))),
		leftThigh->getPxActor(), leftThigh->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(-0.15f, -0.4f, 0.f))));

	configD6Joint(3.14 / 2.5f, 3.14f / 2.5f, -3.14f / 2.5f, 3.14f / 2.5f, leftHipJoint);

	PxD6Joint *rightHipJoint = PxD6JointCreate(*physics, hips->getPxActor(),
		hips->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(0.15f, -0.4f, 0.f))),
		rightThigh->getPxActor(), rightThigh->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(0.15f, -0.4f, 0.f))));

	configD6Joint(3.14 / 2.5f, 3.14f / 2.5f, -3.14f / 2.5f, 3.14f / 2.5f, rightHipJoint);

	mSimScene->getCompoundCreator()->createCylinder(0.1f, 0.7f, 10);

	Compound* leftShin = createObject(PxTransform(pos + PxVec3(-0.15f, -1.3f, 0.f), PxQuat(3.14159 / 2.f, PxVec3(1.f, 0.f, 0.f))), vel, PxVec3(0), false, mat);
	Compound* rightShin = createObject(PxTransform(pos + PxVec3(0.15f, -1.3f, 0.f), PxQuat(3.14159 / 2.f, PxVec3(1.f, 0.f, 0.f))), vel, PxVec3(0), false, mat);

		finishBody(leftShin->getPxActor(), density, 4.f);
		finishBody(rightShin->getPxActor(), density, 4.f);

	PxD6Joint *leftKneeJoint = PxD6JointCreate(*physics, leftThigh->getPxActor(),
		leftThigh->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(-0.15f, -1.f, 0.0f), PxQuat(3.141592f / 2.f, PxVec3(0.f, 1.f, 0.f)))),
		leftShin->getPxActor(), leftShin->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(-0.15f, -1.f, 0.f), PxQuat(3.141592f / 2.f, PxVec3(0.f, 1.f, 0.f)))));

	configD6Joint(3.14 / 16.f, 3.14f / 16.f, -3.14 / 16.f, 3.14f / 2.5f, leftKneeJoint);

	PxD6Joint *rightKneeJoint = PxD6JointCreate(*physics, rightThigh->getPxActor(),
		rightThigh->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(0.15f, -1.f, 0.0f), PxQuat(3.141592f / 2.f, PxVec3(0.f, 1.f, 0.f)))),
		rightShin->getPxActor(), rightShin->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(0.15f, -1.f, 0.f), PxQuat(3.141592f / 2.f, PxVec3(0.f, 1.f, 0.f)))));

	configD6Joint(3.14 / 16.f, 3.14f / 16.f, -3.14 / 16.f, 3.14f / 2.5f, rightKneeJoint);

	mSimScene->getCompoundCreator()->createBox(PxVec3(0.2f, 0.1f, 0.4f));

	Compound* leftFoot = createObject(PxTransform(pos + PxVec3(-0.15f, -1.65f, 0.15f)), vel, PxVec3(0), false, mat);
	Compound* rightFoot = createObject(PxTransform(pos + PxVec3(0.15f, -1.65f, 0.15f)), vel, PxVec3(0), false, mat);

		finishBody(leftFoot->getPxActor(), density, 4.f);
		finishBody(rightFoot->getPxActor(), density, 4.f);

	PxD6Joint *leftAnkle = PxD6JointCreate(*physics, leftShin->getPxActor(),
		leftShin->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(-0.15f, -1.6f, 0.0f), PxQuat(3.141592f / 2.f, PxVec3(0.f, 1.f, 0.f)))),
		leftFoot->getPxActor(), leftFoot->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(-0.15f, -1.6f, 0.f), PxQuat(3.141592f / 2.f, PxVec3(0.f, 1.f, 0.f)))));

	configD6Joint(3.14 / 16.f, 3.14f / 16.f, -3.14 / 16.f, 3.14f / 8.f, leftAnkle);

	PxD6Joint *rightAnkle = PxD6JointCreate(*physics, rightShin->getPxActor(),
		rightShin->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(0.15f, -1.6f, 0.0f), PxQuat(3.141592f / 2.f, PxVec3(0.f, 1.f, 0.f)))),
		rightFoot->getPxActor(), rightFoot->getPxActor()->getGlobalPose().transformInv(PxTransform(pos + PxVec3(0.15f, -1.6f, 0.f), PxQuat(3.141592f / 2.f, PxVec3(0.f, 1.f, 0.f)))));

	configD6Joint(3.14 / 16.f, 3.14f / 16.f, -3.14 / 16.f, 3.14f / 8.f, rightAnkle);


}

void SceneKapla::createCylindricalTower(PxU32 nbRadialPoints, PxReal maxRadius, PxReal minRadius, PxU32 height, PxVec3 dims, PxVec3 centerPos, PxMaterial* material, ShaderMaterial& mat,
	PxFilterData& simFilterData, PxFilterData& queryFilterData, PxReal density, bool bUseSweeps, bool bStartAsleep)
{

	

	PxShape* shape;
	PxVec3 vel(0), omega(0);

	mSimScene->getCompoundCreator()->createBox(2.f*dims);//0.5f, 0.2f, 20, 20);

	PxReal startHeight = 0.f;

	PxTransform objectTransform(PxVec3(3.172 - 2.3 - 39.87), PxQuat());

	for (PxU32 i = 0; i < height; ++i)
	{
		PxReal radius = minRadius + (maxRadius - minRadius) * (1.f - PxReal(i) / PxReal(height));
		for (PxU32 a = 0; a < nbRadialPoints; ++a)
		{
			PxF32 angle = 6.28f*PxF32(a) / PxF32(nbRadialPoints);
			PxVec3 innerPos(cosf(angle)*radius, dims.y + startHeight, sinf(angle)*radius);

			PxQuat rot(3.14 / 2.f - angle, PxVec3(0.f, 1.f, 0.f));

			PxTransform transform(innerPos + centerPos, rot);


			Compound* compound = createObject(transform, vel, omega, false, mat);
			PxRigidDynamic* innerBox = compound->getPxActor();
			innerBox->getShapes(&shape, 1);
			shape->setMaterials(&material, 1);
			shape->setQueryFilterData(queryFilterData);
			shape->setSimulationFilterData(simFilterData);

		
		

			innerBox->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
			innerBox->setMaxDepenetrationVelocity(2.f);
			PxRigidBodyExt::updateMassAndInertia(*innerBox, density);
			innerBox->setMassSpaceInertiaTensor(innerBox->getMassSpaceInertiaTensor() * 4.f);

			if (bStartAsleep)
				innerBox->putToSleep();

		}

		PxReal innerCircumference = (radius - (dims.z - dims.x)) * 3.1415928 * 2.f;
		PxReal midCircumference = (radius)* 3.1415928 * 2.f;
		PxReal outerCircumference = (radius + (dims.z - dims.x)) * 3.1415928 * 2.f;

		PxU32 nbInnerSlabs = innerCircumference / (dims.z * 2);
		PxU32 nbMidSlabs = midCircumference / (dims.z * 2);
		PxU32 nbOuterSlabs = outerCircumference / (dims.z * 2);


		for (PxU32 a = 0; a < nbInnerSlabs; a++)
		{
			PxF32 angle = 6.28f*PxF32(a) / PxF32(nbInnerSlabs);
			
			PxVec3 innerPos(cosf(angle)*(radius - (dims.z - dims.x)), 3.f * dims.y + startHeight, sinf(angle)*(radius - (dims.z - dims.x)));

			PxQuat rot(-angle, PxVec3(0.f, 1.f, 0.f));

			Compound* compound = createObject(PxTransform(innerPos + centerPos, rot), vel, omega, false, mat);
			PxRigidDynamic* innerBox = compound->getPxActor();
			innerBox->getShapes(&shape, 1);
			shape->setMaterials(&material, 1);

			shape->setQueryFilterData(queryFilterData);
			shape->setSimulationFilterData(simFilterData);

			innerBox->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
			innerBox->setMaxDepenetrationVelocity(2.f);
			PxRigidBodyExt::updateMassAndInertia(*innerBox, density);
			innerBox->setMassSpaceInertiaTensor(innerBox->getMassSpaceInertiaTensor() * 4.f);

			if (bStartAsleep)
				innerBox->putToSleep();
		}

		for (PxU32 a = 0; a < nbMidSlabs; a++)
		{
			PxF32 angle = 6.28f*PxF32(a) / PxF32(nbMidSlabs);
			PxVec3 innerPos(cosf(angle)*(radius), 3.f * dims.y + startHeight, sinf(angle)*(radius));

			PxQuat rot(-angle, PxVec3(0.f, 1.f, 0.f));

			Compound* compound = createObject(PxTransform(innerPos + centerPos, rot), vel, omega, false, mat);
			PxRigidDynamic* innerBox = compound->getPxActor();
			innerBox->getShapes(&shape, 1);
			shape->setMaterials(&material, 1);

			shape->setQueryFilterData(queryFilterData);
			shape->setSimulationFilterData(simFilterData);

			innerBox->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
			innerBox->setMaxDepenetrationVelocity(2.f);
			PxRigidBodyExt::updateMassAndInertia(*innerBox, density);
			innerBox->setMassSpaceInertiaTensor(innerBox->getMassSpaceInertiaTensor() * 4.f);

			if (bStartAsleep)
				innerBox->putToSleep();
		}

		for (PxU32 a = 0; a < nbOuterSlabs; a++)
		{
			PxF32 angle = 6.28f*PxF32(a) / PxF32(nbOuterSlabs);
			PxVec3 outerPos(cosf(angle)*(radius + (dims.z - dims.x)), 3.f * dims.y + startHeight, sinf(angle)*(radius + (dims.z - dims.x)));

			PxQuat rot(-angle, PxVec3(0.f, 1.f, 0.f));

			Compound* compound = createObject(PxTransform(outerPos + centerPos, rot), vel, omega, false, mat);
			PxRigidDynamic* outerBox = compound->getPxActor();
			outerBox->getShapes(&shape, 1);
			shape->setMaterials(&material, 1);

			shape->setQueryFilterData(queryFilterData);
			shape->setSimulationFilterData(simFilterData);

			outerBox->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
			outerBox->setMaxDepenetrationVelocity(2.f);
			PxRigidBodyExt::updateMassAndInertia(*outerBox, density);
			outerBox->setMassSpaceInertiaTensor(outerBox->getMassSpaceInertiaTensor() * 4.f);

			if (bStartAsleep)
				outerBox->putToSleep();
		}

		startHeight += 4.f * dims.y;
		density *= 0.975f;
	}

	//Now place the lid on the structure...

	PxReal midCircumference = (minRadius - dims.z) * 3.1415928 * 2.f;

	PxU32 nbMidSlabs = midCircumference / (dims.y * 2);

	PxQuat baseRotation(3.1415 / 2.f, PxVec3(0.f, 0.f, 1.f));

	for (PxU32 a = 0; a < nbMidSlabs; ++a)
	{
		const PxF32 angle = 6.28f*PxF32(a) / PxF32(nbMidSlabs);
		PxVec3 innerPos(cosf(angle)*minRadius, dims.x + startHeight, sinf(angle)*minRadius);

		PxQuat rot(3.14 / 2.f - angle, PxVec3(0.f, 1.f, 0.f));

		Compound* compound = createObject(PxTransform(innerPos + centerPos, rot*baseRotation), vel, omega, false, mat);
		PxRigidDynamic* innerBox = compound->getPxActor();
		innerBox->getShapes(&shape, 1);
		shape->setMaterials(&material, 1);

		shape->setQueryFilterData(queryFilterData);
		shape->setSimulationFilterData(simFilterData);

		innerBox->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
		innerBox->setMaxDepenetrationVelocity(2.f);
		PxRigidBodyExt::updateMassAndInertia(*innerBox, density);
		innerBox->setMassSpaceInertiaTensor(innerBox->getMassSpaceInertiaTensor() * 4.f);

		if (bStartAsleep)
			innerBox->putToSleep();
	}

}




// ----------------------------------------------------------------------------------------------
std::string SceneKapla::getWeaponName()
{
	switch (mWeapon) {
		case wpBullet : return "Bullet";
		case wpBall : return "Ball";
		case wpMeteor : return "Meteor";
	}
	return "";
}

// ----------------------------------------------------------------------------------------------
Compound* SceneKapla::createObject(const PxVec3 &pos, const PxVec3 &vel, const PxVec3 &omega,
	bool particles, const ShaderMaterial &mat, bool useSecondaryPattern, ShaderShadow* shader, int matID, int surfMatID)
{
	Compound *cp = (Compound*)mSimScene->createCompound(mDefaultContactOffset, mDefaultRestOffset);

	PxTransform pose(pos);
	cp->createFromGeometry(mSimScene->getCompoundCreator()->getGeometry(), pose, vel, omega, shader, matID, surfMatID);
	cp->setShader(shader ? shader : mSimSceneShader, mat);
	mSimScene->addCompound(cp);


	return cp;	
}

Compound* SceneKapla::createObject(const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega,
	bool particles, const ShaderMaterial &mat, bool useSecondaryPattern, ShaderShadow* shader, int matID, int surfMatID)
{
	Compound *cp = (Compound*)mSimScene->createCompound(mDefaultContactOffset, mDefaultRestOffset);

	ShaderShadow* myShader = shader ? shader : mSimSceneShader;

	cp->createFromGeometry(mSimScene->getCompoundCreator()->getGeometry(), pose, vel, omega, myShader, matID, surfMatID);
	cp->setShader(shader ? shader : mSimSceneShader, mat);
	mSimScene->addCompound(cp);
	return cp;	
}

Compound* SceneKapla::createObject(PxRigidDynamic* body, const ShaderMaterial &mat, ShaderShadow* shader, int matID, int surfMatID)
{
	Compound *cp = (Compound*)mSimScene->createCompound(mDefaultContactOffset, mDefaultRestOffset);

	ShaderShadow* myShader = shader ? shader : mSimSceneShader;
	cp->createFromGeometry(mSimScene->getCompoundCreator()->getGeometry(), body, myShader, matID, surfMatID);
	cp->setShader(shader ? shader : mSimSceneShader, mat);
	mSimScene->addCompound(cp);
	return cp;
}
// ----------------------------------------------------------------------------------------------
SceneKapla::~SceneKapla() 
{
	if (mTerrain) delete mTerrain;
	mTerrain = NULL;
	if (mSimScene != NULL)
		delete mSimScene;

	if (mPickQuadric != NULL)
		gluDeleteQuadric(mPickQuadric);

	if (mBulletMesh)
		delete mBulletMesh;

	if (mAsteroidMesh)
		delete mAsteroidMesh;

	meteors.clear();
}

// ----------------------------------------------------------------------------------------------

void SceneKapla::preSim(float dt)
{
	if (mSimScene)
		mSimScene->preSim(dt);

	if (meteors.size() > 0) 
	{
		/*static vector<PxVec3> sites;
		sites.clear();*/

		int num = 0;
		const int numSamples = 20;
		for (int i = 0; i < (int)meteors.size(); i++) 
		{
			if (meteors[i]->getLifeFrames() > 1 && meteors[i]->getPxActor() != NULL) 
			{
				meteors[num] = meteors[i];
				num++;
			}
		}
		meteors.resize(num);
	}

}

// ----------------------------------------------------------------------------------------------
void SceneKapla::postSim(float dt)
{
	if (mSimScene)
		mSimScene->postSim(dt, NULL);

	mFrameNr++;
}

// ----------------------------------------------------------------------------------------------
void SceneKapla::handleMouseButton(int button, int state, int x, int y)
{
	mMouseX = x;
	mMouseY = y;

	mMouseDown = (state == GLUT_DOWN);

	PxVec3 orig, dir;
	getMouseRay(x,y, orig, dir);

	if (glutGetModifiers() & GLUT_ACTIVE_SHIFT) {
		if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
			if (mSimScene)
				mSimScene->pickStart(orig, dir);
		}
	}
	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
		if (mSimScene)
			mSimScene->pickRelease();
	}
}

// ----------------------------------------------------------------------------------------------
void SceneKapla::handleMouseMotion(int x, int y)
{
	mMouseX = x;
	mMouseY = y;

	if (mSimScene && mSimScene->getPickActor() != NULL) {
		PxVec3 orig, dir;
		getMouseRay(x,y, orig, dir);	
		mSimScene->pickMove(orig, dir);
	}
}
//ofstream orays("ray.txt");

// ----------------------------------------------------------------------------------------------
void SceneKapla::handleKeyDown(unsigned char key, int x, int y)
{
	switch (key) {
		case ' ':
			{
				if (!mGunActive)
				{
					float vel = 1.0f;
					PxVec3 orig, dir;
					getMouseRay(mMouseX, mMouseY, orig, dir);
					/*
					float sx = 0.0f, sy = 30.0f, sz = 0.0f;
					int nx = 10;
					int ny = 10;
					float dx = 1.0f;
					for (int i = 0; i < ny; i++) {
					for (int j = 0; j < nx; j++) {
					shoot(PxVec3(sx+j*dx, sy, sz + i*dx), PxVec3(0.0f,1.0f,0.0f));
					}
					}

					*/
					shoot(mCameraPos, dir);
				}
				mGunActive = true;
				//orays<<"{"<<mCameraPos.x<<","<<mCameraPos.y<<","<<mCameraPos.z<<","<<dir.x<<","<<dir.y<<","<<dir.z<<"},\n";
				//orays.flush();
				//printf("{%f,%f,%f,%f,%f,%f},\n ", mCameraPos.x, mCameraPos.y, mCameraPos.z, dir.x, dir.y, dir.z);
				break;
			}
			/*
		case 'j':
			{
				mSimScene->dumpSceneGeometry();
				break;
			}
			*/
	}
}

// ----------------------------------------------------------------------------------------------
void SceneKapla::handleKeyUp(unsigned char key, int x, int y)
{
	switch (key) {
		case 'v':
			if (mSimScene) mSimScene->toggleDebugDrawing(); break;
		case ' ':
			{
				mGunActive = false;
			}
	}
}

// ----------------------------------------------------------------------------------------------
void SceneKapla::handleSpecialKey(unsigned char key, int x, int y)
{
	switch (key)
    {
		case GLUT_KEY_F4: mWeapon = (Weapon)(((int)mWeapon+1)%numWeapons); break;					  
	}
}

// ----------------------------------------------------------------------------------------------
void SceneKapla::shoot(PxVec3 &orig, PxVec3 &dir)
{
	if (mSimScene == NULL) 
		return;
	
	mGunActive = true;
	if (mWeapon == wpBall || mWeapon == wpBullet || mWeapon == wpMeteor) {
		PxVec3 dims(0.4f, 0.4f, 0.4f);
		Compound* cp = (Compound*)mSimScene->createCompound( mDefaultContactOffset, mDefaultRestOffset);   // invincible
		PxTransform pose = PX_TRANSFORM_ID;
		PxVec3 vel, omega;
		omega = PxVec3(0.0f, 0.0f, 0.0f);
		if (mWeapon == wpMeteor) 
		{
			float idist = 1000.f;

			PxRaycastBuffer buffer;
			if (getScene().raycast(orig, dir, idist, buffer, PxHitFlag::eDEFAULT, PxQueryFilterData(PxQueryFlag::eDYNAMIC)))
				idist = buffer.getAnyHit(0).distance;

			PxVec3 impactPos = orig + dir*idist;

			PxVec3 cameraRight = mCameraDir.cross(mCameraUp);
			const float angle = 45.0f * physx::PxPi / 180.0f;
			const float distance = mMeteorDistance;
			PxVec3 meteorDirection = cos(angle)*cameraRight + sin(angle)*mCameraUp;
			pose.p = impactPos + meteorDirection*distance;
			vel = -meteorDirection*mMeteorVelocity* mSlowMotionFactor;
		} 
		else 
		{
			 {
				pose.p = mCameraPos;
				PxVec3 a0;
				if (fabs(dir.x) < fabs(dir.y) && fabs(dir.x) < fabs(dir.z))
					a0 = PxVec3(1.0f, 0.0f, 0.0f);
				else if (fabs(dir.y) < fabs(dir.z))
					a0 = PxVec3(0.f, 1.0f, 0.0f);
				else
					a0 = PxVec3(0.0f, 0.0f, 1.0f);
				PxVec3 a1 = dir.cross(a0);
				a1.normalize();
				a0 = a1.cross(dir);
				pose.q = PxQuat(PxMat33(a1, dir, a0));
				float dist = 1000.0f;

				vel = orig + dir * dist - mCameraPos;
				vel.normalize();
			}
			vel *= 30.0f * mSlowMotionFactor;
		}

		//vel *= 0.0f;
		ShaderMaterial mat;
		mat.init();
		mat.color[0] = 0.9f;
		mat.color[1] = 0.2f;
		mat.color[2] = 0.2f;
		cp->setShader(NULL, mat);

		if (mWeapon == wpMeteor) 
		{

			vel *= 2.0f;
			PxVec3 a0;
			PxVec3 dir = vel;
			dir.normalize();
			if (fabs(dir.x) < fabs(dir.y) && fabs(dir.x) < fabs(dir.z))
				a0 = PxVec3(1.0f, 0.0f, 0.0f);
			else if (fabs(dir.y) < fabs(dir.z))
				a0 = PxVec3(0.f, 1.0f, 0.0f);
			else
				a0 = PxVec3(0.0f, 0.0f, 1.0f);
			PxVec3 a1 = dir.cross(a0);
			a1.normalize();
			a0 = a1.cross(dir);
			pose.q = PxQuat(PxMat33(dir, a0, a1)).getNormalized();

			const float maxRot = 50.0f;
			omega = PxVec3(randRange(-maxRot, maxRot),randRange(-maxRot, maxRot),randRange(-maxRot, maxRot));

			mAsteroidMesh->normalize(PxVec3(0.0f, 0.0f, 0.0f), mMeteorSize);
			cp->createFromMesh(mAsteroidMesh, pose, vel, omega);
			cp->getPxActor()->setMass(10000000.0f);
			cp->getPxActor()->setMaxContactImpulse(2.5f);
			cp->getPxActor()->setMaxDepenetrationVelocity(1.f);
			
			cp->setKinematic(vel);
			cp->setLifeFrames(mWeaponLifeTime);
			cp->setAdditionalImpactImpulse(mWeaponImpulse, mWeaponImpulse);

			meteors.push_back(cp);
		} 
		else if (mWeapon == wpBall) 
		{
			mSimScene->getCompoundCreator()->createSphere(dims, 8);
			cp->createFromGeometry(mSimScene->getCompoundCreator()->getGeometry(), pose, vel, omega, NULL); 
			PxRigidBodyExt::updateMassAndInertia(*cp->getPxActor(), 1000.f);
			cp->getPxActor()->setAngularDamping(0.75f);
		}
		else 
		{
			cp->createFromMesh(mBulletMesh, pose, vel, omega);
			cp->setKinematic(vel);
			cp->setLifeFrames(mWeaponLifeTime);
			cp->setAdditionalImpactImpulse(mWeaponImpulse, mWeaponImpulse);
		}

		

		mSimScene->addCompound(cp);
	}
}

// ----------------------------------------------------------------------------------------------
void SceneKapla::render(bool useShader)
{
	SampleViewerScene::render(useShader);

	if (mSimScene != NULL) {
		mSimScene->setCamera( mCameraPos, mCameraDir, mCameraUp, mCameraFov );
		mSimScene->draw(useShader);
	}

	if (useShader && mTerrain) {
		mTerrain->draw(useShader);

	}

	pickDraw();
	//if (useShader)
		//renderCrossLines();

//	mFracPattern.getGeometry().debugDraw();
}

//-----------------------------------------------------------------------------
void SceneKapla::pickDraw()
{
	if (mSimScene && mSimScene->getPickActor() == NULL)
		return;

	PxRigidDynamic *pickActor = mSimScene->getPickActor();

	float r = 0.05f;

	if (mPickQuadric == NULL) {
		mPickQuadric = gluNewQuadric();
		gluQuadricDrawStyle(mPickQuadric, GLU_FILL);
	}

	glColor3f(1.0f, 0.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	const PxVec3 &pickPos = mSimScene->getPickPos();
	glTranslatef(pickPos.x, pickPos.y, pickPos.z);
	gluSphere(mPickQuadric, r, 10, 10);
	glPopMatrix();

	PxVec3 pos = pickActor->getGlobalPose().transform(mSimScene->getPickLocalPos());

	glColor3f(1.0f, 0.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(pos.x, pos.y, pos.z);
	gluSphere(mPickQuadric, r, 10, 10);
	glPopMatrix();

	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(pickPos.x, pickPos.y, pickPos.z);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(pos.x, pos.y, pos.z);
	glEnd();

	glColor3f(1, 1, 1);

}

//-----------------------------------------------------------------------------
void SceneKapla::printPerfInfo()
{
	if (mSimScene != NULL) {
		printf("num SDK bodies %i\n", mSimScene->getCompounds().size());
   /* if(mSimScene->getParticles())
      printf("num particles %i\n", mSimScene->getParticles()->getNumParticles());*/
	}
}
