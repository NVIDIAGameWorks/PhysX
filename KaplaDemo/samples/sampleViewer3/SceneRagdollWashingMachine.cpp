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

#include "SceneRagdollWashingMachine.h"
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

using namespace physx;
using namespace physx::shdfnd;

static PxReal speeds[] = { 3.141692f*0.125f, 3.141592f*0.25f, 3.141592f*0.75f, 3.141592f*0.5f, 3.141592f*0.25f };
static PxReal spinDurations[] = { 10, 15, 5, 5, 10 };

SceneRagdollWashingMachine::SceneRagdollWashingMachine(PxPhysics* pxPhysics, PxCooking *pxCooking, bool isGrb,
	Shader *defaultShader, const char *resourcePath, float slowMotionFactor) : SceneKapla(pxPhysics, pxCooking, isGrb, defaultShader, resourcePath, slowMotionFactor),
	mWashingMachine(NULL)
{
	mChangeSpeedTime = 15.f;
	mSpeedState = 0;
}

void SceneRagdollWashingMachine::preSim(float dt)
{
	mChangeSpeedTime -= dt;
	if (mChangeSpeedTime < 0.f)
	{
		mSpeedState = (mSpeedState + 1)%5;
		mChangeSpeedTime = spinDurations[mSpeedState];
	}

	PxQuat rotationPerFrame(speeds[mSpeedState]*dt, PxVec3(0,0,1));

	PxTransform trans = mWashingMachine->getGlobalPose();
	trans.q = trans.q * rotationPerFrame;
	mWashingMachine->setKinematicTarget(trans);

	SceneKapla::preSim(dt);
}

void SceneRagdollWashingMachine::onInit(PxScene* pxScene)
{
	//(1) Create the washing machine :)

	SceneKapla::onInit(pxScene);

	ShaderMaterial mat;
	mat.init();

	PxVec3 dims(3.f, 1.f, 35.f);

	PxReal radius = 7.f;

	PxReal circumference = 2.f * radius * 3.141592f;

	//Now work out how many slabs you need to satisfy this...

	PxU32 nbSlabs = PxU32((circumference / dims.x)) + 5;

	PxVec3 offset(0, radius, 0);

	for (PxU32 a = 0; a < nbSlabs; ++a)
	{
		PxQuat rotation(a*2.f*3.141592 / PxReal(nbSlabs), PxVec3(0, 0, 1));

		PxTransform localTransform(rotation.rotate(offset), rotation);

		mSimScene->getCompoundCreator()->createBox(dims, &localTransform, a == 0);
	}

	PxTransform localTransform(PxVec3(0, radius - dims.x/2.f, 0.f), PxQuat(3.141592*0.5f, PxVec3(0, 0, 1)));
	mSimScene->getCompoundCreator()->createBox(dims, &localTransform, false);

	Compound* compound = createObject(PxTransform(PxVec3(0, radius+5.f, -dims.z)), PxVec3(0), PxVec3(0), false, mat,false);
	mWashingMachine = compound->getPxActor();
	mWashingMachine->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

	for (PxU32 a = 0; a < 20; ++a)
	{
		for (PxU32 b = 0; b < 35; ++b)
		{
			createRagdoll(PxVec3(-radius + 2.f + a * 0.5f, radius+5.f, -dims.z*0.8f - b * 0.5f), PxVec3(0), mat);
		}
	}

	createGroundPlane(PxFilterData(), PxVec3(0, -2.f, 0));

	mSimScene->getCompoundCreator()->createBox(PxVec3(20.f, 20.f, 2.f));
	Compound* staticCompound = createObject(PxTransform(PxVec3(0.f, radius+5.f, -1.5f*dims.z)), PxVec3(0), PxVec3(0),false,mat,false);
	staticCompound->getPxActor()->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
}