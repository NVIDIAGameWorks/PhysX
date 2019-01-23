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

#include "SampleNorthPole.h"
#include "SampleNorthPoleCameraController.h"
#include "PxRigidDynamic.h"
#include "geometry/PxCapsuleGeometry.h"
#include "PxShape.h"
#include "characterkinematic/PxCapsuleController.h"
#include <SampleBaseInputEventIds.h>
#include <SamplePlatform.h>
#include <SampleUserInput.h>
#include <SampleUserInputIds.h>
#include <SampleUserInputDefines.h>

using namespace SampleRenderer;
using namespace SampleFramework;

SampleNorthPoleCameraController::SampleNorthPoleCameraController(PxCapsuleController& controlled, SampleNorthPole& base) :
	mCCT						(controlled),
	mBase						(base),
	mTargetYaw					(0.0f-PxPi/2),
	mTargetPitch				(0.0f),
	mPitchMin					(-PxHalfPi*.99f),
	mPitchMax					(PxHalfPi*.99f),
	mGamepadPitchInc			(0.0f),
	mGamepadYawInc				(0.0f),
	mGamepadForwardInc			(0.0f),
	mGamepadLateralInc			(0.0f),
	mSensibility				(0.001f),
	mFwd						(false),
	mBwd						(false),
	mLeft						(false),
	mRight						(false),
	mKeyShiftDown				(false),
	mRunningSpeed				(10.0f),
	mWalkingSpeed				(2.5f),
	mFilterMemory				(0.0f)
{
	mFilterMemory = float(computeCameraTarget().y);
}

void SampleNorthPoleCameraController::collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents)
{
	//digital keyboard events
	DIGITAL_INPUT_EVENT_DEF(CAMERA_MOVE_FORWARD,											SCAN_CODE_FORWARD,		SCAN_CODE_FORWARD,		SCAN_CODE_FORWARD	);
	DIGITAL_INPUT_EVENT_DEF(CAMERA_MOVE_BACKWARD,											SCAN_CODE_BACKWARD,		SCAN_CODE_BACKWARD,		SCAN_CODE_BACKWARD	);
	DIGITAL_INPUT_EVENT_DEF(CAMERA_MOVE_LEFT,												SCAN_CODE_LEFT,			SCAN_CODE_LEFT,			SCAN_CODE_LEFT		);
	DIGITAL_INPUT_EVENT_DEF(CAMERA_MOVE_RIGHT,												SCAN_CODE_RIGHT,		SCAN_CODE_RIGHT,		SCAN_CODE_RIGHT		);
	DIGITAL_INPUT_EVENT_DEF(CAMERA_SHIFT_SPEED,												SCAN_CODE_LEFT_SHIFT,	OSXKEY_SHIFT,			LINUXKEY_SHIFT		);

	//analog gamepad events
	ANALOG_INPUT_EVENT_DEF(CAMERA_GAMEPAD_ROTATE_LEFT_RIGHT, GAMEPAD_ROTATE_SENSITIVITY,	GAMEPAD_RIGHT_STICK_X,	GAMEPAD_RIGHT_STICK_X,	LINUXKEY_UNKNOWN	);
	ANALOG_INPUT_EVENT_DEF(CAMERA_GAMEPAD_ROTATE_UP_DOWN, GAMEPAD_ROTATE_SENSITIVITY,		GAMEPAD_RIGHT_STICK_Y,	GAMEPAD_RIGHT_STICK_Y,	LINUXKEY_UNKNOWN	);
	ANALOG_INPUT_EVENT_DEF(CAMERA_GAMEPAD_MOVE_LEFT_RIGHT, GAMEPAD_DEFAULT_SENSITIVITY,		GAMEPAD_LEFT_STICK_X,	GAMEPAD_LEFT_STICK_X,	LINUXKEY_UNKNOWN	);
	ANALOG_INPUT_EVENT_DEF(CAMERA_GAMEPAD_MOVE_FORWARD_BACK, GAMEPAD_DEFAULT_SENSITIVITY,	GAMEPAD_LEFT_STICK_Y,	GAMEPAD_LEFT_STICK_Y,	LINUXKEY_UNKNOWN	);
}

void SampleNorthPoleCameraController::onDigitalInputEvent(const SampleFramework::InputEvent& ie, bool val)
{
	if(val)
	{
		if(ie.m_Id == CAMERA_MOVE_FORWARD)	mFwd = true;		
		else	if(ie.m_Id == CAMERA_MOVE_BACKWARD)	mBwd = true;
		else	if(ie.m_Id == CAMERA_MOVE_LEFT)	mLeft = true;
		else	if(ie.m_Id == CAMERA_MOVE_RIGHT)	mRight = true;
		else	if(ie.m_Id == CAMERA_SHIFT_SPEED)	mKeyShiftDown = true;
	}
	else
	{
		if(ie.m_Id == CAMERA_MOVE_FORWARD)	mFwd = false;
		else	if(ie.m_Id == CAMERA_MOVE_BACKWARD)	mBwd = false;
		else	if(ie.m_Id == CAMERA_MOVE_LEFT)	mLeft = false;
		else	if(ie.m_Id == CAMERA_MOVE_RIGHT)	mRight = false;
		else	if(ie.m_Id == CAMERA_SHIFT_SPEED)	mKeyShiftDown = false;
	}
}

static PX_FORCE_INLINE PxReal remapAxisValue(PxReal absolutePosition)
{
	return absolutePosition * absolutePosition * absolutePosition * 5.0f;
}

void SampleNorthPoleCameraController::onAnalogInputEvent(const SampleFramework::InputEvent& ie, float val)
{
	if(ie.m_Id == CAMERA_GAMEPAD_ROTATE_LEFT_RIGHT)
	{
		mGamepadYawInc = -remapAxisValue(val);
	}
	else if(ie.m_Id == CAMERA_GAMEPAD_ROTATE_UP_DOWN)
	{
		// PT: ideally we'd need an option to "invert Y axis" here
//		mGamepadPitchInc = - remapAxisValue(val);
		mGamepadPitchInc = remapAxisValue(val);
	}
	else if(ie.m_Id == CAMERA_GAMEPAD_MOVE_LEFT_RIGHT)
	{
		mGamepadLateralInc = val;
	}
	else if(ie.m_Id == CAMERA_GAMEPAD_MOVE_FORWARD_BACK)
	{
		mGamepadForwardInc = val;
	}
}

void SampleNorthPoleCameraController::onPointerInputEvent(const SampleFramework::InputEvent &ie, physx::PxU32 x, physx::PxU32 y, physx::PxReal dx, physx::PxReal dy, bool val)
{
	if (ie.m_Id == CAMERA_MOUSE_LOOK)
	{
		mTargetYaw		-= dx * mSensibility;
		mTargetPitch	+= dy * mSensibility;
	}
}

void SampleNorthPoleCameraController::setView(PxReal pitch, PxReal yaw)
{
	mTargetPitch = pitch;
	mTargetYaw   = yaw;
}

PxExtendedVec3 SampleNorthPoleCameraController::computeCameraTarget()
{
	PxRigidActor* characterActor = mCCT.getActor();
	PxShape* shape;
	characterActor->getShapes(&shape,1);
	PxCapsuleGeometry geom;
	shape->getCapsuleGeometry(geom);

	const PxExtendedVec3 headPos = PxExtendedVec3(0,geom.halfHeight+geom.radius,0);
	return mCCT.getPosition() + headPos;
}

static PX_INLINE float feedbackFilter(float val, float& memory, float sharpness)
{
			if(sharpness<0.0f)	sharpness = 0.0f;
	else	if(sharpness>1.0f)	sharpness = 1.0f;
	return memory = val * sharpness + memory * (1.0f - sharpness);
}

void SampleNorthPoleCameraController::update(Camera& camera, PxReal dtime)
{
	PxSceneReadLock scopedLock(mBase.getActiveScene());

	// Update CCT
	if(!mBase.isPaused())
	{
		PxVec3 targetKeyDisplacement(0);
		PxVec3 targetPadDisplacement(0);

		PxVec3 forward = camera.getViewDir();
		forward.y = 0;
		forward.normalize();
		PxVec3 up = PxVec3(0,1,0);
		PxVec3 right = forward.cross(up);

		if(mFwd)	targetKeyDisplacement += forward;
		if(mBwd)	targetKeyDisplacement -= forward;

		if(mRight)	targetKeyDisplacement += right;
		if(mLeft)	targetKeyDisplacement -= right;

		targetKeyDisplacement *= mKeyShiftDown ? mRunningSpeed : mWalkingSpeed;
		targetKeyDisplacement += PxVec3(0,-9.81f,0);
		targetKeyDisplacement *= dtime;

		targetPadDisplacement += forward * mGamepadForwardInc * mRunningSpeed;
		targetPadDisplacement += right * mGamepadLateralInc * mRunningSpeed;
		targetPadDisplacement += PxVec3(0,-9.81f,0);
		targetPadDisplacement *= dtime;

//		PxU32 flags = mCCT.move(targetKeyDisplacement + targetPadDisplacement, 0.001f, dtime, PxControllerFilters(0));
		PxU32 flags = mCCT.move(targetKeyDisplacement + targetPadDisplacement, 0.0f, dtime, PxControllerFilters(0));
		PX_UNUSED(flags);
	}
	// Update camera
	{
		mTargetYaw		+= mGamepadYawInc * dtime;
		mTargetPitch	+= mGamepadPitchInc * dtime;
		
		// Clamp pitch
		if(mTargetPitch<mPitchMin)	mTargetPitch = mPitchMin;
		if(mTargetPitch>mPitchMax)	mTargetPitch = mPitchMax;
		
		camera.setRot(PxVec3(-mTargetPitch,-mTargetYaw,0));

		PxExtendedVec3 camTarget = computeCameraTarget();
		const float filteredHeight = feedbackFilter((float)camTarget.y, mFilterMemory, dtime*6.0f);
		camTarget.y = filteredHeight;

		const PxF32 distanceToTarget = 0.0f;
		const PxVec3 target = toVec3(camTarget) - camera.getViewDir()*distanceToTarget;
		camera.setPos(target);
	}
}

