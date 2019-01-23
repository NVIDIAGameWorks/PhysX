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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#include "PxPhysicsAPI.h"
#include "extensions/PxExtensionsAPI.h"
#include "PxTkStream.h"
#include "SampleNorthPole.h"
#include "RenderBaseActor.h"

#ifdef _MSC_VER
#pragma warning(disable:4305)
#endif

using namespace PxToolkit;

//#define USE_RAYCAST_CCD_FOR_SNOWBALLS

void SampleNorthPole::detach()
{
	std::vector<PxShape*>::const_iterator ite;
	for(ite=mDetaching.begin(); ite<mDetaching.end(); ++ite)
	{
		PxSceneWriteLock scopedLock(*mScene);

		PxShape* shape = *ite;
		PxRigidActor* actor = shape->getActor();
		
		PxMaterial* mat;
		shape->getMaterials(&mat,1);
		PxGeometryHolder geometry = shape->getGeometry();
		PxTransform newActorPose = PxShapeExt::getGlobalPose(*shape, *actor);

		PxRigidDynamic* newActor = PxCreateDynamic(*mPhysics, newActorPose, geometry.any(), *mat, 1);
		if(!newActor) 
			fatalError("creating detached actor failed");
		getActiveScene().addActor(*newActor);
		mPhysicsActors.push_back(newActor);

		// the only convex shape is the nose, so we can just test the type
		if(geometry.getType() == PxGeometryType::eCONVEXMESH)
		{
			newActor->addForce(PxVec3(0,0.1f,0),PxForceMode::eFORCE);
			size_t index = reinterpret_cast<size_t>(actor->userData);
			mSnowman[index].changeMood();
		}

		// reuse the old shape's rendering actor
		RenderBaseActor* render = getRenderActor(actor, shape);
		PxShape* newShape;
		newActor->getShapes(&newShape,1);
		unlink(render, shape, actor);
		link(render, newShape, newActor);
		setCCDActive(*newShape, newActor);

		actor->detachShape(*shape);
	}
	mDetaching.clear();
}

void SampleNorthPole::cookCarrotConvexMesh()
{
	static const PxVec3 carrotVerts[] = {PxVec3(0,0,.3),PxVec3(.05,0,0),PxVec3(-.05,0,0),PxVec3(0,.05,0),PxVec3(0,-.05,0)};

	mCarrotConvex = PxToolkit::createConvexMesh(getPhysics(), getCooking(), carrotVerts, 5, PxConvexFlag::eCOMPUTE_CONVEX);
	if(!mCarrotConvex)
		fatalError("creating the convex mesh failed");
}

PxRigidDynamic* SampleNorthPole::throwBall()
{
	PxSceneWriteLock scopedLock(*mScene);

	static unsigned int numBall = 0;

	PxVec3 vel = getCamera().getViewDir() * 20.0f;

	PxRigidDynamic* ballActor = getPhysics().createRigidDynamic(PxTransform(getCamera().getPos()+getCamera().getViewDir()));
	if(!ballActor)
		fatalError("creating ball actor failed");

	ballActor->setLinearVelocity(vel);

	PxShape* ballShape = PxRigidActorExt::createExclusiveShape(*ballActor, PxSphereGeometry(0.1f),getDefaultMaterial());
	if(!ballShape)
		fatalError("creating ball shape failed");

	setSnowball(*ballShape);

	PxRigidBodyExt::updateMassAndInertia(*ballActor,1);

#ifndef USE_RAYCAST_CCD_FOR_SNOWBALLS
	setCCDActive(*ballShape, ballActor);
#endif
	
	getActiveScene().addActor(*ballActor);

	RenderBaseActor* actor = mSnowBallsRenderActors[numBall];;
	if(actor)
	{
		link(actor, ballShape, ballActor);
	}
	else
	{
		actor = createRenderObjectFromShape(ballActor, ballShape, mSnowMaterial);
		mSnowBallsRenderActors[numBall] = actor;
	}	

#ifdef USE_RAYCAST_CCD_FOR_SNOWBALLS
	if(ballShape)
	{
		RenderBaseActor* renderActor = getRenderActor(ballActor, ballShape);
		renderActor->setRaycastCCD(true);
	}
#endif

	PxRigidDynamic* oldBall = mSnowBalls[numBall];

	if(oldBall) 
	{
		removeActor(oldBall);
		oldBall->release();
	}	

	mSnowBalls[numBall] = ballActor;
	if(! (++numBall < NUM_BALLS)) 
		numBall=0;

	return ballActor;
}

void SampleNorthPole::createSnowMen()
{
	PxU32 numSnowmen = NUM_SNOWMEN; // can't have NUM_SNOWMEN in the loop since setting to 0 causes compiler warnings
	for(PxU32 i=0; i<numSnowmen; i++)
	{	
		PxVec3 pos(0,1,-8);
		PxQuat rot(2*PxPi*i/numSnowmen,PxVec3(0,1,0));
		createSnowMan(PxTransform(rot.rotate(pos),rot),i,i);
	}

}

PxRigidDynamic* SampleNorthPole::createSnowMan(const PxTransform& pos, const PxU32 mode, const PxU32 index)
{
	PxRigidDynamic* snowmanActor = getPhysics().createRigidDynamic(PxTransform(pos));
	if(!snowmanActor)
		fatalError("create snowman actor failed");

	PxMaterial&	material = getDefaultMaterial();
	PxShape* armL = NULL; PxShape* armR = NULL;

	switch(mode%5)
	{
	case 0: // with a weight at the bottom
		{
			PxShape* shape = NULL;
			shape = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxSphereGeometry(.2),material);
			if(!shape)
				fatalError("creating snowman shape failed");
			shape->setLocalPose(PxTransform(PxVec3(0,-.29,0)));
			
			PxRigidBodyExt::updateMassAndInertia(*snowmanActor,10);

			shape = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxSphereGeometry(.5),material);
			if(!shape)
				fatalError("creating snowman shape failed");
			
			shape = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxSphereGeometry(.4),material);
			if(!shape)
				fatalError("creating snowman shape failed");
			shape->setLocalPose(PxTransform(PxVec3(0,.6,0)));
			
			shape = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxSphereGeometry(.3),material);
			if(!shape)
				fatalError("creating snowman shape failed");
			shape->setLocalPose(PxTransform(PxVec3(0,1.1,0)));
			
			armL = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxCapsuleGeometry(.1,.1),material);
			if(!armL)
				fatalError("creating snowman shape failed");
			armL->setLocalPose(PxTransform(PxVec3(-.4,.7,0)));

			armR = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxCapsuleGeometry(.1,.1),material);
			if(!armR)
				fatalError("creating snowman shape failed");			
			armR->setLocalPose(PxTransform(PxVec3( .4,.7,0)));
		}
		break;
	case 1: // only considering lowest shape mass
		{
			PxShape* shape = NULL;
			shape = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxSphereGeometry(.5),material);
			if(!shape)
				fatalError("creating snowman shape failed");
			
			PxRigidBodyExt::updateMassAndInertia(*snowmanActor,1);

			shape = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxSphereGeometry(.4),material);
			if(!shape)
				fatalError("creating snowman shape failed");
			shape->setLocalPose(PxTransform(PxVec3(0,.6,0)));

			shape = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxSphereGeometry(.3),material);
			if(!shape)
				fatalError("creating snowman shape failed");
			shape->setLocalPose(PxTransform(PxVec3(0,1.1,0)));

			armL = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxCapsuleGeometry(.1,.1),material);
			if(!armL)
				fatalError("creating snowman shape failed");
			armL->setLocalPose(PxTransform(PxVec3(-.4,.7,0)));

			armR = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxCapsuleGeometry(.1,.1),material);
			if(!armR)
				fatalError("creating snowman shape failed");			
			armR->setLocalPose(PxTransform(PxVec3( .4,.7,0)));

			snowmanActor->setCMassLocalPose(PxTransform(PxVec3(0,-.5,0)));
		}
		break;
	case 2: // considering whole mass
		{
			PxShape* shape = NULL;
			shape = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxSphereGeometry(.5),material);
			if(!shape)
				fatalError("creating snowman shape failed");
			
			shape = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxSphereGeometry(.4),material);
			if(!shape)
				fatalError("creating snowman shape failed");
			shape->setLocalPose(PxTransform(PxVec3(0,.6,0)));

			shape = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxSphereGeometry(.3),material);
			if(!shape)
				fatalError("creating snowman shape failed");
			shape->setLocalPose(PxTransform(PxVec3(0,1.1,0)));
			
			armL = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxCapsuleGeometry(.1,.1),material);
			if(!armL)
				fatalError("creating snowman shape failed");
			armL->setLocalPose(PxTransform(PxVec3(-.4,.7,0)));

			armR = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxCapsuleGeometry(.1,.1),material);
			if(!armR)
				fatalError("creating snowman shape failed");			
			armR->setLocalPose(PxTransform(PxVec3( .4,.7,0)));

			PxRigidBodyExt::updateMassAndInertia(*snowmanActor,1);
			snowmanActor->setCMassLocalPose(PxTransform(PxVec3(0,-.5,0)));
		}
		break;
	case 3: // considering whole mass with low COM
		{
			PxShape* shape = NULL;
			shape = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxSphereGeometry(.5),material);
			if(!shape)
				fatalError("creating snowman shape failed");

			shape = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxSphereGeometry(.4),material);
			if(!shape)
				fatalError("creating snowman shape failed");
			shape->setLocalPose(PxTransform(PxVec3(0,.6,0)));

			shape = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxSphereGeometry(.3),material);
			if(!shape)
				fatalError("creating snowman shape failed");
			shape->setLocalPose(PxTransform(PxVec3(0,1.1,0)));

			armL = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxCapsuleGeometry(.1,.1),material);
			if(!armL)
				fatalError("creating snowman shape failed");
			armL->setLocalPose(PxTransform(PxVec3(-.4,.7,0)));

			armR = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxCapsuleGeometry(.1,.1),material);
			if(!armR)
				fatalError("creating snowman shape failed");			
			armR->setLocalPose(PxTransform(PxVec3( .4,.7,0)));

			const PxVec3 localPos = PxVec3(0,-.5,0);
			PxRigidBodyExt::updateMassAndInertia(*snowmanActor,1,&localPos);
		}
		break;
	case 4: // setting up mass properties manually
		{
			PxShape* shape = NULL;
			shape = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxSphereGeometry(.5),material);
			if(!shape)
				fatalError("creating snowman shape failed");
			
			shape = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxSphereGeometry(.4),material);
			if(!shape)
				fatalError("creating snowman shape failed");
			shape->setLocalPose(PxTransform(PxVec3(0,.6,0)));

			shape = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxSphereGeometry(.3),material);
			if(!shape)
				fatalError("creating snowman shape failed");
			shape->setLocalPose(PxTransform(PxVec3(0,1.1,0)));

			armL = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxCapsuleGeometry(.1,.1),material);
			if(!armL)
				fatalError("creating snowman shape failed");
			armL->setLocalPose(PxTransform(PxVec3(-.4,.7,0)));

			armR = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxCapsuleGeometry(.1,.1),material);
			if(!armR)
				fatalError("creating snowman shape failed");			
			armR->setLocalPose(PxTransform(PxVec3( .4,.7,0)));

			snowmanActor->setMass(1);
			snowmanActor->setCMassLocalPose(PxTransform(PxVec3(0,-.5,0)));
			snowmanActor->setMassSpaceInertiaTensor(PxVec3(.05,100,100));
		}
		break;
	default:
		break;
	}
	
	setDetachable(*armL);
	setDetachable(*armR);

	createRenderObjectsFromActor(snowmanActor,mSnowMaterial);

	PxShape* carrot = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxConvexMeshGeometry(mCarrotConvex),material);
	if(!carrot)
		fatalError("create snowman shape failed");
	carrot->setLocalPose(PxTransform(PxVec3(0,1.1,.3)));
	setDetachable(*carrot);

	createRenderObjectFromShape(snowmanActor, carrot, mCarrotMaterial);

	PxShape* button = NULL;
	
	button = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxCapsuleGeometry(.02,.05),material);
	if(!button)
		fatalError("create snowman shape failed");
	button->setLocalPose(PxTransform(PxVec3(.1,1.2,.3),PxQuat(PxHalfPi/3,PxVec3(0,0,1))));
	button->setFlag(PxShapeFlag::eSIMULATION_SHAPE,false);
	mSnowman[index].eyeL = button;
	createRenderObjectFromShape(snowmanActor, button, mButtonMaterial);

	button = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxCapsuleGeometry(.02,.05),material);
	if(!button)
		fatalError("create snowman shape failed");
	button->setLocalPose(PxTransform(PxVec3(-.1,1.2,.3),PxQuat(-PxHalfPi/3,PxVec3(0,0,1))));
	button->setFlag(PxShapeFlag::eSIMULATION_SHAPE,false);
	mSnowman[index].eyeR = button;
	createRenderObjectFromShape(snowmanActor, button, mButtonMaterial);

	button = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxSphereGeometry(.05),material);
	if(!button)
		fatalError("create snowman shape failed");
	button->setLocalPose(PxTransform(PxVec3(0,.8,.35)));
	setDetachable(*button);
	createRenderObjectFromShape(snowmanActor, button, mButtonMaterial);

	button = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxSphereGeometry(.05),material);
	if(!button)
		fatalError("create snowman shape failed");
	button->setLocalPose(PxTransform(PxVec3(0,.6,.4)));
	setDetachable(*button);
	createRenderObjectFromShape(snowmanActor, button, mButtonMaterial);
	
	button = PxRigidActorExt::createExclusiveShape(*snowmanActor, PxSphereGeometry(.05),material);
	if(!button)
		fatalError("create snowman shape failed");
	button->setLocalPose(PxTransform(PxVec3(0,.4,.35)));
	setDetachable(*button);
	createRenderObjectFromShape(snowmanActor, button, mButtonMaterial);

	getActiveScene().addActor(*snowmanActor);

	snowmanActor->userData = (void*)size_t(index);

	mPhysicsActors.push_back(snowmanActor);

	return snowmanActor;
}
