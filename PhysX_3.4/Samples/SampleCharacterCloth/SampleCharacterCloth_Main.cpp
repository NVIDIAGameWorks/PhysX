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

#include "PxPhysXConfig.h"

#if PX_USE_CLOTH_API

#include "SampleCharacterCloth.h"

#include "SampleAllocatorSDKClasses.h"
#include "SampleCharacterClothCameraController.h"
#include "SampleCharacterClothInputEventIds.h"
#include "SampleCharacterClothSettings.h"
#include "SampleCharacterClothFlag.h"
#include "SamplePlatform.h"

#include "RenderMaterial.h"
#include "RenderBaseActor.h"

#include "characterkinematic/PxCapsuleController.h"
#include "characterkinematic/PxControllerManager.h"
#include "PxPhysicsAPI.h"

#include "SampleCharacterClothInputEventIds.h"
#include <SampleUserInputIds.h>
#include <SampleUserInput.h>
#include <SampleUserInputDefines.h>

REGISTER_SAMPLE(SampleCharacterCloth, "SampleCharacterCloth")

using namespace SampleFramework;
using namespace SampleRenderer;
using namespace PxToolkit;

///////////////////////////////////////////////////////////////////////////////

SampleCharacterCloth::SampleCharacterCloth(PhysXSampleApplication& app)
	: PhysXSample				(app, 2)
	, mCCTCamera				(NULL)
	, mController				(NULL)
	, mControllerManager		(NULL)
	, mCCTDisplacement			(0.0f)
	, mCCTDisplacementPrevStep	(0.0f)
	, mCCTTimeStep				(0.0f)
	, mCape                   	(NULL)
#if PX_SUPPORT_GPU_PHYSX || PX_XBOXONE
	, mUseGPU               	(true)
#endif
{
	mCreateGroundPlane  = false;
	//mStepperType = FIXED_STEPPER;
	mControllerInitialPosition = PxVec3(0.0, 3.0f, 0.0);

	mCCTActive = false;

	mNbVerts	= 0;
	mNbTris		= 0;
	mVerts		= NULL;
	mTris		= NULL;
}

///////////////////////////////////////////////////////////////////////////////
SampleCharacterCloth::~SampleCharacterCloth()
{
	DELETESINGLE(mCCTCamera);

	SAMPLE_FREE(mTris);
	SAMPLE_FREE(mVerts);

	const size_t nbPlatforms = mPlatforms.size();
	for(PxU32 i = 0;i < nbPlatforms;i++)
		mPlatforms[i]->release();

//	releaseFlags();
}

///////////////////////////////////////////////////////////////////////////////
// create render materials used for render objects.
// The newly created materials are appended to PhysXSample::mRenderMaterials array
void SampleCharacterCloth::createRenderMaterials()
{
	getRenderer()->setAmbientColor(RendererColor(100, 100, 100));

	// create render materials used in this sample
	mSnowMaterial = SAMPLE_NEW(RenderMaterial)(*getRenderer(), PxVec3(0.70f, 0.70f, 0.8f), 1.0f, false, 0, NULL);
	mRenderMaterials.push_back(mSnowMaterial);

	mFlagMaterial = createRenderMaterialFromTextureFile("nvidia_flag_d.bmp");
}

///////////////////////////////////////////////////////////////////////////////

void SampleCharacterCloth::customizeSample(SampleSetup& setup)
{
	setup.mName	= "SampleCharacterCloth";
}

///////////////////////////////////////////////////////////////////////////////
void SampleCharacterCloth::collectInputEvents(std::vector<const InputEvent*>& inputEvents)
{
	PhysXSample::collectInputEvents(inputEvents);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(SPAWN_DEBUG_OBJECT);

	//digital keyboard events
	DIGITAL_INPUT_EVENT_DEF(RESET_SCENE,			WKEY_R,		XKEY_R,		X1KEY_R,	PS3KEY_R,	PS4KEY_R,	AKEY_UNKNOWN,	OSXKEY_R,	IKEY_UNKNOWN,	LINUXKEY_R,	WIIUKEY_UNKNOWN);
	DIGITAL_INPUT_EVENT_DEF(CHANGE_CLOTH_CONTENT,	WKEY_M,		XKEY_M,		X1KEY_M,	PS3KEY_M,	PS4KEY_M,	AKEY_UNKNOWN,	OSXKEY_M,	IKEY_UNKNOWN,	LINUXKEY_M,	WIIUKEY_UNKNOWN);

	//digital mouse events

    //touch events
    TOUCH_INPUT_EVENT_DEF(RESET_SCENE,		"Reset",			ABUTTON_5,		IBUTTON_5);
}

///////////////////////////////////////////////////////////////////////////////
void SampleCharacterCloth::onDigitalInputEvent(const InputEvent& ie, bool val)
{
	if(val)
	{
		switch (ie.m_Id)
		{
		case RESET_SCENE:
			{
				resetScene();
			}
			break;
		case CHANGE_CLOTH_CONTENT:
			mClothFlagCountIndex = (mClothFlagCountIndex+1)% (mClothFlagCountIndexMax+1);
			releaseFlags();
			createFlags();
			break;

#if PX_SUPPORT_GPU_PHYSX || PX_XBOXONE
		case TOGGLE_CPU_GPU:
			toggleGPU();
			return;
			break;
#endif
		}
	}

	PhysXSample::onDigitalInputEvent(ie,val);
}

///////////////////////////////////////////////////////////////////////////////

void SampleCharacterCloth::helpRender(PxU32 x, PxU32 y, PxU8 textAlpha)
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
	msg = mApplication.inputInfoMsg("Press "," to jump", CAMERA_JUMP, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);

	msg = mApplication.inputInfoMsg("Press "," to toggle visualization", TOGGLE_VISUALIZATION, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);

	msg = mApplication.inputInfoMsg("Press "," to reset scene", RESET_SCENE, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);

	msg = mApplication.inputInfoMsg("Press "," to change flag count", CHANGE_CLOTH_CONTENT,-1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);

#if PX_SUPPORT_GPU_PHYSX || PX_XBOXONE
	msg = mApplication.inputInfoMsg("Press "," to toggle CPU/GPU", TOGGLE_CPU_GPU, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
#endif
}

void SampleCharacterCloth::descriptionRender(PxU32 x, PxU32 y, PxU8 textAlpha)
{
	bool print=(textAlpha!=0.0f);

	if(print)
	{
		Renderer* renderer = getRenderer();
		const PxU32 yInc = 24;
		const PxReal scale = 0.5f;
		const PxReal shadowOffset = 6.0f;
		const RendererColor textColor(255, 255, 255, textAlpha);

		char line0[256]="This sample demonstrates the PhysX cloth feature.  In particular, the ";
		char line1[256]="setup and simulation of a cape attached to an animated character is";
		char line2[256]="presented. This involves generating an approximate description of the";  
		char line3[256]="character's geometry, cloth instantiation, the binding of the cloth to a";
		char line4[256]="skinned character, and the setup of simulation parameters and values";
		char line5[256]="controlling the different phases of the cloth solver.  To clarify the";
		char line6[256]="process, and to offer some variety, a flag is also created and simulated.";
#if PX_WINDOWS
		char line7[256]="Finally, the sample illustrates the steps required to enable GPU";
		char line8[256]="acceleration of PhysX cloth.";
#endif

		renderer->print(x, y+=yInc, line0, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line1, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line2, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line3, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line4, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line5, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line6, scale, shadowOffset, textColor);
#if PX_WINDOWS
		renderer->print(x, y+=yInc, line7, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line8, scale, shadowOffset, textColor);
#endif
	}
}

///////////////////////////////////////////////////////////////////////////////
void SampleCharacterCloth::customizeRender()
{
#if PX_SUPPORT_GPU_PHYSX || PX_XBOXONE
	SampleRenderer::Renderer* renderer = getRenderer();

	const PxU32 yInc = 20;
	const PxReal scale = 0.5f;
	const PxReal shadowOffset = 6.0f;
	PxU32 width, height;
	renderer->getWindowSize(width, height);

	PxU32 y = height-2*yInc;
	PxU32 x = width - 360;

	{
		const RendererColor textColor(255, 0, 0, 255);
		const char* message[4] = { "Flag count: Moderate", "Flag count: Medium", "Flag count: High", "Flag count: Very High" };
		renderer->print(x, y, message[mClothFlagCountIndex], scale, shadowOffset, textColor);
	}

	{
		y -= yInc;
		const RendererColor textColor(255, 0, 0, 255);

		char buf[2048];
		sprintf(buf, mUseGPU ? "Running on GPU" : "Running on CPU");

		if(PX_SUPPORT_GPU_PHYSX && mUseGPU)
			sprintf(buf + strlen(buf), " (%s)", getActiveScene().getTaskManager()->getGpuDispatcher()->getCudaContextManager()->getDeviceName());

		renderer->print(x, y, buf, scale, shadowOffset, textColor);
	}

	{
		y -= yInc;

		char fpsText[512];
		sprintf(fpsText, "Average PhysX time: %3.3f ms", 1000.0f * mAverageSimulationTime);
	
		const RendererColor textColor(255, 255, 255, 255);
		renderer->print(x, y, fpsText, scale, shadowOffset, textColor);	
	}

#endif
}

///////////////////////////////////////////////////////////////////////////////
void SampleCharacterCloth::onInit()
{
	mNbThreads = PxMax(PxI32(shdfnd::Thread::getNbPhysicalCores())-1, 0);

	// try to get gpu support
	mCreateCudaCtxManager = true;

	PhysXSample::onInit();

	PxSceneWriteLock scopedLock(*mScene);

	mApplication.setMouseCursorHiding(true);
	mApplication.setMouseCursorRecentering(true);

	mClothFlagCountIndex = 0;
	mClothFlagCountIndexMax = 3;

	// create materials and render setup
	createRenderMaterials();
	
	// build static heightfield landscape
	buildHeightField();

	// create CCT controller 
	createCCTController();

	// create the stickman character
	createCharacter();

	// create a cape for the stickman
	createCape();

	// create flags
	createFlags();
	
	// reset scene
	resetScene();

	// apply global physx settings
	getActiveScene().setGravity(PxVec3(0,-10.0f,0));
}

///////////////////////////////////////////////////////////////////////////////
void SampleCharacterCloth::resetScene()
{	
	PxSceneWriteLock scopedLock(*mScene);

	// reset cct 
	PxVec3 pos = mControllerInitialPosition;
	mController->setPosition(PxExtendedVec3(pos.x, pos.y, pos.z));

	// compute global pose and local positions
	PxExtendedVec3 posExt = mController->getPosition();
	PxTransform pose = mController->getActor()->getGlobalPose();
	pose.p = PxVec3(PxReal(posExt.x), PxReal(posExt.y), PxReal(posExt.z)) - PxVec3(0.0f, 1.5, 0.0f);

	mCharacter.setGlobalPose(pose);

	resetCharacter();
	resetCape();

	// reset platforms
	const size_t nbPlatforms = mPlatforms.size();
	for(PxU32 i = 0;i < nbPlatforms;i++)
		mPlatforms[i]->reset();

	// reset camera
	mCCTCamera->setView(0.0f, 0.0f);

	// reset stats
	mFrameCount = 0;
	mAccumulatedSimulationTime = 0.0f;
	mAverageSimulationTime = 0.0f;
}

///////////////////////////////////////////////////////////////////////////////
void SampleCharacterCloth::onShutdown()
{
	{
		PxSceneWriteLock scopedLock(*mScene);
		DELETESINGLE(mCCTCamera);
		SAFE_RELEASE(mControllerManager);
	}
	PhysXSample::onShutdown();
}

///////////////////////////////////////////////////////////////////////////////
void SampleCharacterCloth::onTickPreRender(float dtime)
{
	PhysXSample::onTickPreRender(dtime);
}

///////////////////////////////////////////////////////////////////////////////
// called after each substep is done (can be called more than once per frame)
void SampleCharacterCloth::onSubstep(float dtime)
{
	PxSceneWriteLock scopedLock(*mScene);

	updatePlatforms(dtime);
	updateCCT(dtime);
	updateCharacter(dtime);
	updateCape(dtime);
	updateFlags(dtime);

#if PX_SUPPORT_GPU_PHYSX || PX_XBOXONE
	// update the GPU flag (may have been disabled if CUDA error, not enough shared memory, etc)
	mUseGPU = mCape->getClothFlags() & (PxClothFlag::eCUDA);
#endif
}

///////////////////////////////////////////////////////////////////////////////
void SampleCharacterCloth::onTickPostRender(float dtime)
{
	PhysXSample::onTickPostRender(dtime);

	mFrameCount++;
	mAccumulatedSimulationTime += getSimulationTime();
	PxReal delta = PxReal(mTimer.peekElapsedSeconds());
	if (delta > 1.0f)
	{
		mAverageSimulationTime = mAccumulatedSimulationTime / PxReal(mFrameCount);
		mFrameCount = 0;
		mAccumulatedSimulationTime = 0.0f;
		mTimer.getElapsedSeconds();
	}
}

///////////////////////////////////////////////////////////////////////////////
void SampleCharacterCloth::onPointerInputEvent(const SampleFramework::InputEvent& ie, physx::PxU32 x, physx::PxU32 y, physx::PxReal dx, physx::PxReal dy, bool val)
{
	PhysXSample::onPointerInputEvent(ie,x,y,dx,dy,val);
}

#endif // PX_USE_CLOTH_API


