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

#include "PhysXSample.h"

#include "SampleCharacterClothCameraController.h"

#include "SampleCharacterCloth.h"
#include "SampleCharacterClothInputEventIds.h"
#include "SampleUtils.h"

#include "PxRigidDynamic.h"
#include "foundation/PxMath.h"
#include <SampleBaseInputEventIds.h>
#include <SamplePlatform.h>
#include <SampleUserInput.h>
#include <SampleUserInputIds.h>
#include <SampleUserInputDefines.h>

using namespace physx;
using namespace SampleRenderer;
using namespace SampleFramework;

///////////////////////////////////////////////////////////////////////////////
SampleCharacterClothCameraController::SampleCharacterClothCameraController(PxController& controlled, SampleCharacterCloth& base) :
	mCamera						(NULL),
	mController						(controlled),
	mSample						(base),
	mTargetYaw					(0.0f-PxPi/2),
	mTargetPitch				(0.0f),
	mPitchMin					(-PxHalfPi),
	mPitchMax					(PxHalfPi),
	mGamepadPitchInc			(0.0f),
	mGamepadYawInc				(0.0f),
	mGamepadForwardInc			(0.0f),
	mGamepadLateralInc			(0.0f),
	mSensibility				(0.01f),
	mFwd						(false),
	mBwd						(false),
	mLeft						(false),
	mRight						(false),
	mKeyShiftDown				(false),
	mRunningSpeed				(8.0f),
	mWalkingSpeed				(4.0f)
{
}

//////////////////////////////////////////////////////////////////////////

void SampleCharacterClothCameraController::collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents)
{
	//digital keyboard events
	DIGITAL_INPUT_EVENT_DEF(CAMERA_MOVE_FORWARD,											SCAN_CODE_FORWARD,		XKEY_W,					X1KEY_W,				PS3KEY_W,				PS4KEY_W,				AKEY_UNKNOWN,			SCAN_CODE_FORWARD,		IKEY_UNKNOWN,			SCAN_CODE_FORWARD,	WIIUKEY_UNKNOWN);
	DIGITAL_INPUT_EVENT_DEF(CAMERA_MOVE_BACKWARD,											SCAN_CODE_BACKWARD,		XKEY_S,					X1KEY_S,				PS3KEY_S,				PS4KEY_S,				AKEY_UNKNOWN,			SCAN_CODE_BACKWARD,		IKEY_UNKNOWN,			SCAN_CODE_BACKWARD,	WIIUKEY_UNKNOWN);
	DIGITAL_INPUT_EVENT_DEF(CAMERA_MOVE_LEFT,												SCAN_CODE_LEFT,			XKEY_A,					X1KEY_A,				PS3KEY_A,				PS4KEY_A,				AKEY_UNKNOWN,			SCAN_CODE_LEFT,			IKEY_UNKNOWN,			SCAN_CODE_LEFT,		WIIUKEY_UNKNOWN);
	DIGITAL_INPUT_EVENT_DEF(CAMERA_MOVE_RIGHT,												SCAN_CODE_RIGHT,		XKEY_D,					X1KEY_D,				PS3KEY_D,				PS4KEY_D,				AKEY_UNKNOWN,			SCAN_CODE_RIGHT,		IKEY_UNKNOWN,			SCAN_CODE_RIGHT,	WIIUKEY_UNKNOWN);
	DIGITAL_INPUT_EVENT_DEF(CAMERA_SHIFT_SPEED,												SCAN_CODE_LEFT_SHIFT,	XKEY_SHIFT,				X1KEY_SHIFT,			PS3KEY_SHIFT,			PS4KEY_SHIFT,			AKEY_UNKNOWN,			OSXKEY_SHIFT,			IKEY_UNKNOWN,			LINUXKEY_SHIFT,		WIIUKEY_UNKNOWN);
	DIGITAL_INPUT_EVENT_DEF(CAMERA_JUMP,													SCAN_CODE_SPACE,		XKEY_SPACE,				X1KEY_SPACE,			PS3KEY_SPACE,			PS4KEY_SPACE,			AKEY_UNKNOWN,			OSXKEY_SPACE,			IKEY_UNKNOWN,			LINUXKEY_SPACE,		WIIUKEY_UNKNOWN);
																
	//digital gamepad events																
	DIGITAL_INPUT_EVENT_DEF(CAMERA_JUMP,													GAMEPAD_SOUTH,			GAMEPAD_SOUTH,			GAMEPAD_SOUTH, 			GAMEPAD_SOUTH,			GAMEPAD_SOUTH,			AKEY_UNKNOWN,			GAMEPAD_SOUTH,			IKEY_UNKNOWN,			LINUXKEY_UNKNOWN,	GAMEPAD_SOUTH);

	//analog gamepad events
	ANALOG_INPUT_EVENT_DEF(CAMERA_GAMEPAD_ROTATE_LEFT_RIGHT,GAMEPAD_ROTATE_SENSITIVITY,		GAMEPAD_RIGHT_STICK_X,	GAMEPAD_RIGHT_STICK_X,	GAMEPAD_RIGHT_STICK_X,	GAMEPAD_RIGHT_STICK_X,	GAMEPAD_RIGHT_STICK_X,	GAMEPAD_RIGHT_STICK_X,	GAMEPAD_RIGHT_STICK_X,	GAMEPAD_RIGHT_STICK_X,	LINUXKEY_UNKNOWN,	GAMEPAD_RIGHT_STICK_X);
	ANALOG_INPUT_EVENT_DEF(CAMERA_GAMEPAD_ROTATE_UP_DOWN,GAMEPAD_ROTATE_SENSITIVITY,		GAMEPAD_RIGHT_STICK_Y,	GAMEPAD_RIGHT_STICK_Y,	GAMEPAD_RIGHT_STICK_Y,	GAMEPAD_RIGHT_STICK_Y,	GAMEPAD_RIGHT_STICK_Y,	GAMEPAD_RIGHT_STICK_Y,	GAMEPAD_RIGHT_STICK_Y,	GAMEPAD_RIGHT_STICK_Y,	LINUXKEY_UNKNOWN,	GAMEPAD_RIGHT_STICK_Y);
	ANALOG_INPUT_EVENT_DEF(CAMERA_GAMEPAD_MOVE_LEFT_RIGHT,GAMEPAD_DEFAULT_SENSITIVITY,		GAMEPAD_LEFT_STICK_X,	GAMEPAD_LEFT_STICK_X,	GAMEPAD_LEFT_STICK_X,	GAMEPAD_LEFT_STICK_X,	GAMEPAD_LEFT_STICK_X,	GAMEPAD_LEFT_STICK_X,	GAMEPAD_LEFT_STICK_X,	GAMEPAD_LEFT_STICK_X,	LINUXKEY_UNKNOWN,	GAMEPAD_LEFT_STICK_X);
	ANALOG_INPUT_EVENT_DEF(CAMERA_GAMEPAD_MOVE_FORWARD_BACK,GAMEPAD_DEFAULT_SENSITIVITY,	GAMEPAD_LEFT_STICK_Y,	GAMEPAD_LEFT_STICK_Y,	GAMEPAD_LEFT_STICK_Y,	GAMEPAD_LEFT_STICK_Y,	GAMEPAD_LEFT_STICK_Y,	GAMEPAD_LEFT_STICK_Y,	GAMEPAD_LEFT_STICK_Y,	GAMEPAD_LEFT_STICK_Y,	LINUXKEY_UNKNOWN,	GAMEPAD_LEFT_STICK_Y);

    //touch events
    TOUCH_INPUT_EVENT_DEF(CAMERA_JUMP,	"Jump",	AQUICK_BUTTON_1,	IQUICK_BUTTON_1);
}

///////////////////////////////////////////////////////////////////////////////
void SampleCharacterClothCameraController::onDigitalInputEvent(const SampleFramework::InputEvent& ie, bool val)
{
	switch (ie.m_Id)
	{
	case CAMERA_MOVE_FORWARD:
		{
			mFwd = val;
		}
		break;
	case CAMERA_MOVE_BACKWARD:
		{
			mBwd = val;
		}
		break;
	case CAMERA_MOVE_LEFT:
		{
			mLeft = val;
		}
		break;
	case CAMERA_MOVE_RIGHT:
		{
			mRight = val;
		}
		break;
	case CAMERA_SHIFT_SPEED:
		{
			mKeyShiftDown = val;
		}
		break;
	case CAMERA_JUMP:
		{
			if(val)
				mSample.mJump.startJump(13.0f);
		}
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////
static PX_FORCE_INLINE PxReal remapAxisValue(PxReal absolutePosition)
{
	return absolutePosition * absolutePosition * absolutePosition * 5.0f;
}

///////////////////////////////////////////////////////////////////////////////
void SampleCharacterClothCameraController::onAnalogInputEvent(const SampleFramework::InputEvent& ie, float val)
{
	if(ie.m_Id == CAMERA_GAMEPAD_ROTATE_LEFT_RIGHT)
	{
		mGamepadYawInc = - remapAxisValue(val);
	}
	else if(ie.m_Id == CAMERA_GAMEPAD_ROTATE_UP_DOWN)
	{
		mGamepadPitchInc = - remapAxisValue(val);
	}
	else if(ie.m_Id == CAMERA_GAMEPAD_MOVE_LEFT_RIGHT)
	{
		mGamepadLateralInc = -val;
	}
	else if(ie.m_Id == CAMERA_GAMEPAD_MOVE_FORWARD_BACK)
	{
		mGamepadForwardInc = val;
	}
}

///////////////////////////////////////////////////////////////////////////////
void SampleCharacterClothCameraController::onPointerInputEvent(const SampleFramework::InputEvent &ie, physx::PxU32, physx::PxU32, physx::PxReal dx, physx::PxReal dy, bool val)
{
	if (ie.m_Id == CAMERA_MOUSE_LOOK)
	{
		mTargetYaw		-= dx * mSensibility;
		mTargetPitch	+= dy * mSensibility;
	}
}

///////////////////////////////////////////////////////////////////////////////
void SampleCharacterClothCameraController::setView(PxReal pitch, PxReal yaw)
{
	mTargetPitch = pitch;
	mTargetYaw   = yaw;
}

///////////////////////////////////////////////////////////////////////////////
// This function gets called whenever camera needs to be updated
void SampleCharacterClothCameraController::update(Camera& camera, PxReal dtime)
{
	mCamera = &camera;
	updateFromCCT(dtime);
}

///////////////////////////////////////////////////////////////////////////////
void  
SampleCharacterClothCameraController::updateFromCCT(PxReal dtime)
{
	if(!mSample.isPaused())
	{
		PxVec3 disp(0);

		if (mFwd || mBwd || mRight || mLeft || (fabsf(mGamepadForwardInc) > 0.1f) || (fabsf(mGamepadLateralInc) > 0.1f))
		{
			PxVec3 targetKeyDisplacement(0);
			PxVec3 targetPadDisplacement(0);

			// compute basis
			PxVec3 forward = mCamera->getViewDir();
			forward.y = 0;
			forward.normalize();
			PxVec3 up = PxVec3(0,1,0);
			PxVec3 right = forward.cross(up);
			Ps::computeBasis(forward, right, up);

			// increase target displacement due to key press
			if (mFwd)    targetKeyDisplacement += forward;
			if (mBwd)    targetKeyDisplacement -= forward;
			if (mLeft)   targetKeyDisplacement += right;
			if (mRight)  targetKeyDisplacement -= right;

			targetKeyDisplacement *= mKeyShiftDown ? mRunningSpeed : mWalkingSpeed;
			targetKeyDisplacement *= dtime;

			// displacement due to game pad control
			targetPadDisplacement += forward * mGamepadForwardInc * mRunningSpeed;
			targetPadDisplacement += right * mGamepadLateralInc * mRunningSpeed;
			targetPadDisplacement *= dtime;

			// compute total displacement due to user input
			disp = targetKeyDisplacement + targetPadDisplacement;

			mSample.mCCTActive = true;
		}
		else
			mSample.mCCTActive = false;

		mSample.bufferCCTMotion(disp, dtime);
	}
	else
		mSample.bufferCCTMotion(PxVec3(0.0f), dtime);

	// Update camera
	mTargetYaw		+= mGamepadYawInc * dtime;
	mTargetPitch	+= mGamepadPitchInc * dtime;

	// Clamp pitch
	if (mTargetPitch < mPitchMin)	mTargetPitch = mPitchMin;
	if (mTargetPitch > mPitchMax)	mTargetPitch = mPitchMax;
	
	mCamera->setRot(PxVec3(-mTargetPitch,-mTargetYaw,0));

	const PxExtendedVec3 camTarget = mController.getPosition();
	const PxVec3 target = PxVec3((PxReal)camTarget.x,(PxReal)camTarget.y,(PxReal)camTarget.z)
						- mCamera->getViewDir()*7.0f;
	mCamera->setPos(target);	
}

#endif // PX_USE_CLOTH_API

