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

#include "characterkinematic/PxControllerManager.h"

#include "PxPhysicsAPI.h"

#include "SampleNorthPole.h"
#include "SampleNorthPoleCameraController.h"

#include "SampleUtils.h"
#include "SampleCommandLine.h"
#include "SampleAllocatorSDKClasses.h"
#include "RendererMemoryMacros.h"
#include "RenderMaterial.h"
#include "SampleNorthPoleInputEventIds.h"

#include <SamplePlatform.h>
#include <SampleUserInput.h>

REGISTER_SAMPLE(SampleNorthPole, "SampleNorthPole")

using namespace SampleFramework;
using namespace SampleRenderer;

///////////////////////////////////////////////////////////////////////////////

SampleNorthPole::SampleNorthPole(PhysXSampleApplication& app) :
	PhysXSample					(app),
	mNorthPoleCamera			(NULL),
	mController					(NULL),
	mControllerManager			(NULL),
	mDoStandup					(false)
{
	mCreateGroundPlane	= false;
	//mStepperType = FIXED_STEPPER;
	mStandingSize = 1.0f;
	mCrouchingSize = 0.20f;
	mControllerRadius = 0.3f;
	mControllerInitialPosition = PxExtendedVec3(5,mStandingSize,5);

	memset(mSnowBalls,0,sizeof(mSnowBalls));
	memset(mSnowBallsRenderActors,0,sizeof(mSnowBallsRenderActors));
}

SampleNorthPole::~SampleNorthPole()
{
}

///////////////////////////////////////////////////////////////////////////////

void SampleNorthPole::customizeSample(SampleSetup& setup)
{
	setup.mName	= "SampleNorthPole";
}

///////////////////////////////////////////////////////////////////////////////

void SampleNorthPole::onInit()
{
	PhysXSample::onInit();

	PxSceneWriteLock scopedLock(*mScene);

	mApplication.setMouseCursorHiding(true);
	mApplication.setMouseCursorRecentering(true);

	getRenderer()->setAmbientColor(RendererColor(100, 100, 100));

	// some colors for the rendering
	mSnowMaterial	= SAMPLE_NEW(RenderMaterial)(*getRenderer(), PxVec3(0.85f, 0.85f, 0.95f), 1.0f, false, 0, NULL);
	mCarrotMaterial = SAMPLE_NEW(RenderMaterial)(*getRenderer(), PxVec3(1.00f, 0.50f, 0.00f), 1.0f, false, 1, NULL);
	mButtonMaterial = SAMPLE_NEW(RenderMaterial)(*getRenderer(), PxVec3(0.00f, 0.00f, 0.00f), 1.0f, false, 2, NULL);

	mRenderMaterials.push_back(mSnowMaterial);
	mRenderMaterials.push_back(mCarrotMaterial);
	mRenderMaterials.push_back(mButtonMaterial);

	
	// PhysX
	buildHeightField();
	buildIglooTriMesh();

	// add some stand-up characters
	cookCarrotConvexMesh();

	createSnowMen();

	mControllerManager = PxCreateControllerManager(getActiveScene());

	mController = createCharacter(mControllerInitialPosition);

	mNorthPoleCamera = SAMPLE_NEW(SampleNorthPoleCameraController)(*mController,*this);
	setCameraController(mNorthPoleCamera);

	mNorthPoleCamera->setView(0,0);
}

void SampleNorthPole::onShutdown()
{
	{
		PxSceneWriteLock scopedLock(*mScene);
		DELETESINGLE(mNorthPoleCamera);
		mControllerManager->release();
	}
	
	PhysXSample::onShutdown();
}

void SampleNorthPole::helpRender(PxU32 x, PxU32 y, PxU8 textAlpha)
{
	Renderer* renderer = getRenderer();
	const PxU32 yInc = 18;
	const PxReal scale = 0.5f;
	const PxReal shadowOffset = 6.0f;
	const RendererColor textColor(255, 255, 255, textAlpha);
	const bool isMouseSupported = getApplication().getPlatform()->getSampleUserInput()->mouseSupported();
	const bool isPadSupported = getApplication().getPlatform()->getSampleUserInput()->gamepadSupported();
	const char* msg;

	if (isMouseSupported && isPadSupported)
		renderer->print(x, y += yInc, "Use mouse or right stick to rotate the camera", scale, shadowOffset, textColor);
	else if (isMouseSupported)
		renderer->print(x, y += yInc, "Use mouse to rotate the camera", scale, shadowOffset, textColor);
	else if (isPadSupported)
		renderer->print(x, y += yInc, "Use right stick to rotate the camera", scale, shadowOffset, textColor);
	if (isPadSupported)
		renderer->print(x, y += yInc, "Use left stick to move",scale, shadowOffset, textColor);
	msg = mApplication.inputMoveInfoMsg("Press "," to move", CAMERA_MOVE_FORWARD,CAMERA_MOVE_BACKWARD, CAMERA_MOVE_LEFT, CAMERA_MOVE_RIGHT);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press "," to move fast", CAMERA_SHIFT_SPEED, -1);
	if(msg)
		renderer->print(x, y += yInc, msg, scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press "," to crouch", CROUCH, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press "," to reset scene", RESET_SCENE, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press "," to throw a ball", THROW_BALL, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
}

void SampleNorthPole::descriptionRender(PxU32 x, PxU32 y, PxU8 textAlpha)
{
	bool print=(textAlpha!=0.0f);

	if(print)
	{
		Renderer* renderer = getRenderer();
		const PxU32 yInc = 24;
		const PxReal scale = 0.5f;
		const PxReal shadowOffset = 6.0f;
		const RendererColor textColor(255, 255, 255, textAlpha);

		char line0[256]="This sample demonstrates the creation of dynamic objects (snowmen) with";
		char line1[256]="multiple shapes.  The snowmen, though visibly identical, are configured"; 
		char line2[256]="with different masses, inertias and centres of mass in order to show how";
		char line3[256]="to set up these properties using the sdk, and the effect they have on";
		char line4[256]="behavior. A technical description of the snowmen's rigid body properties";
		char line5[256]="can be found in the PhysX Guide documentation in Rigid Body Dynamics:";
		char line6[256]="Mass Properties. The sample also introduces contact notification reports as";
		char line7[256]="a mechanism to detach snowman body parts, and applies ccd flags in order";
		char line8[256]="to prevent small objects such as snowballs and detached snowman body parts ";
		char line9[256]="from tunneling through collision geometry.";
		
		renderer->print(x, y+=yInc, line0, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line1, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line2, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line3, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line4, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line5, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line6, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line7, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line8, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line9, scale, shadowOffset, textColor);
	}
}

void SampleNorthPole::onTickPreRender(PxReal dtime)
{
	if(mDoStandup)
		tryStandup();

	PhysXSample::onTickPreRender(dtime);
}

void SampleNorthPole::onSubstep(float dtime)
{
	detach();
}

void SampleNorthPole::onPointerInputEvent(const SampleFramework::InputEvent& ie, physx::PxU32 x, physx::PxU32 y, physx::PxReal dx, physx::PxReal dy, bool val)
{
	if((ie.m_Id == THROW_BALL) && val)
	{
		throwBall();
	}

	if((ie.m_Id == RAYCAST_HIT) && val)
	{
		PxRaycastBuffer hit;
		getActiveScene().raycast(getCamera().getPos()+getCamera().getViewDir(), getCamera().getViewDir(), 1.0f, hit);
		shdfnd::printFormatted("hits: %p\n",hit.block.shape);
	}

	PhysXSample::onPointerInputEvent(ie,x,y,dx,dy,val);
}

///////////////////////////////////////////////////////////////////////////////
