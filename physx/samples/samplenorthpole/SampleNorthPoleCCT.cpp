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

#include "PxPhysicsAPI.h"
#include "extensions/PxExtensionsAPI.h"

#include "characterkinematic/PxControllerManager.h"
#include "characterkinematic/PxCapsuleController.h"

#include "SampleNorthPole.h"
#include "SampleNorthPoleCameraController.h"
#include "SampleNorthPoleInputEventIds.h"
#include <SamplePlatform.h>
#include <SampleUserInput.h>
#include <SampleUserInputIds.h>
#include <SampleUserInputDefines.h>

using namespace SampleRenderer;
using namespace SampleFramework;

void SampleNorthPole::tryStandup()
{
	PxSceneWriteLock scopedLock(*mScene);

	// overlap with upper part
	PxReal r = mController->getRadius();
	PxReal dh = mStandingSize-mCrouchingSize-2*r;
	PxCapsuleGeometry geom(r, dh*.5f);

	PxExtendedVec3 position = mController->getPosition();
	PxVec3 pos((float)position.x,(float)position.y+mStandingSize*.5f+r,(float)position.z);
	PxQuat orientation(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f));

	PxOverlapBuffer hit;
	if(getActiveScene().overlap(geom, PxTransform(pos,orientation),hit,
		PxQueryFilterData(PxQueryFlag::eANY_HIT|PxQueryFlag::eSTATIC|PxQueryFlag::eDYNAMIC)))
		return;

	// if no hit, we can stand up
	resizeController(mStandingSize);
	mDoStandup = false;
}

void SampleNorthPole::resizeController(PxReal height)
{
	PxSceneWriteLock scopedLock(*mScene);
	mController->resize(height);
}

PxCapsuleController* SampleNorthPole::createCharacter(const PxExtendedVec3& position)
{
	PxCapsuleControllerDesc cDesc;
	cDesc.material			= &getDefaultMaterial();
	cDesc.position			= position;
	cDesc.height			= mStandingSize;
	cDesc.radius			= mControllerRadius;
	cDesc.slopeLimit		= 0.0f;
	cDesc.contactOffset		= 0.1f;
	cDesc.stepOffset		= 0.02f;
	cDesc.reportCallback	= this;
	cDesc.behaviorCallback	= this;

	mControllerInitialPosition = cDesc.position;

	PxCapsuleController* ctrl = static_cast<PxCapsuleController*>(mControllerManager->createController(cDesc));

	// remove controller shape from scene query for standup overlap test
	PxRigidDynamic* actor = ctrl->getActor();
	if(actor)
	{
		if(actor->getNbShapes())
		{
			PxShape* ctrlShape;
			actor->getShapes(&ctrlShape,1);
			ctrlShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE,false);
		}
		else
			fatalError("character actor has no shape");
	}
	else
		fatalError("character could not create actor");

	// uncomment the next line to render the character
	//createRenderObjectsFromActor(ctrl->getActor());

	return ctrl;
}


void SampleNorthPole::onShapeHit(const PxControllerShapeHit& hit)
{
	PxRigidDynamic* actor = hit.shape->getActor()->is<PxRigidDynamic>();
	if(actor)
	{
		// We only allow horizontal pushes. Vertical pushes when we stand on dynamic objects creates
		// useless stress on the solver. It would be possible to enable/disable vertical pushes on
		// particular objects, if the gameplay requires it.
		if(hit.dir.y==0.0f)
		{
			PxReal coeff = actor->getMass() * hit.length;
			PxRigidBodyExt::addForceAtLocalPos(*actor,hit.dir*coeff, PxVec3(0,0,0), PxForceMode::eIMPULSE);
		}
	}
}

void SampleNorthPole::resetScene()
{	
	PxSceneWriteLock scopedLock(*mScene);

	mController->setPosition(mControllerInitialPosition);
	mNorthPoleCamera->setView(0,0);

	while(mPhysicsActors.size())
	{
		PxRigidActor* actor = mPhysicsActors.back();
		removeActor(actor);
	}

	createSnowMen();
	buildHeightField();

	for(unsigned int b = 0; b < NUM_BALLS; b++)
	{
		PxRigidDynamic* ball = mSnowBalls[b];
		if(ball)
		{
			removeActor(ball);
			ball->release();
			mSnowBalls[b] = 0;
			
			// render actor is released inside of removeActor()
			mSnowBallsRenderActors[b] = 0;
		}
	}
}

void SampleNorthPole::collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents)
{
	PhysXSample::collectInputEvents(inputEvents);

	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(SPAWN_DEBUG_OBJECT);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(CAMERA_MOVE_BUTTON);

	//digital keyboard events
	DIGITAL_INPUT_EVENT_DEF(CROUCH,			SCAN_CODE_DOWN,				SCAN_CODE_DOWN,				SCAN_CODE_DOWN		);
	DIGITAL_INPUT_EVENT_DEF(RESET_SCENE,	WKEY_R,						OSXKEY_R,					LINUXKEY_R			);

	//digital gamepad events
	DIGITAL_INPUT_EVENT_DEF(CROUCH,			GAMEPAD_WEST,				GAMEPAD_WEST,				LINUXKEY_UNKNOWN	);
	DIGITAL_INPUT_EVENT_DEF(RESET_SCENE,	GAMEPAD_NORTH,				GAMEPAD_NORTH,				LINUXKEY_UNKNOWN	);
	DIGITAL_INPUT_EVENT_DEF(THROW_BALL,		GAMEPAD_RIGHT_SHOULDER_BOT, GAMEPAD_RIGHT_SHOULDER_BOT, LINUXKEY_UNKNOWN	);

	//digital mouse events
	if (!isPaused())
	{
		DIGITAL_INPUT_EVENT_DEF(THROW_BALL,	MOUSE_BUTTON_LEFT, 			MOUSE_BUTTON_LEFT, 			MOUSE_BUTTON_LEFT	);
	}
}

void SampleNorthPole::onDigitalInputEvent(const SampleFramework::InputEvent& ie, bool val)
{
	switch (ie.m_Id)
	{
	case CROUCH:
		{
			if(val)
			{
				resizeController(mCrouchingSize);
			}
			else
			{
				mDoStandup = true;
			}
		}
		break;
 	case RESET_SCENE:
		{
			if(val)
			{
				resetScene();
			}
		}
		break;
	case THROW_BALL:
		{
			if(val)
			{
				throwBall();
			}
		}
		break;
	}

	PhysXSample::onDigitalInputEvent(ie,val);
}

