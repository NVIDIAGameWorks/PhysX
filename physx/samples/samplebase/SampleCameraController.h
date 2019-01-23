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

#ifndef SAMPLE_CAMERA_CONTROLLER_H
#define SAMPLE_CAMERA_CONTROLLER_H

#include "SampleAllocator.h"
#include "RendererWindow.h"
#include <SampleUserInput.h>
#include "foundation/PxVec3.h"

namespace SampleFramework {
	class SamplePlatform;
}

class Camera;

class CameraController : public SampleAllocateable
{
	public:
	virtual ~CameraController() {}

	virtual void		onPointerInputEvent(const SampleFramework::InputEvent& ie, physx::PxU32 x, physx::PxU32 y, physx::PxReal dx, physx::PxReal dy, bool val) {}

	virtual void		onAnalogInputEvent(const SampleFramework::InputEvent& , float val) {}
	virtual void		onDigitalInputEvent(const SampleFramework::InputEvent& , bool val) {}

	virtual	void		update(Camera& camera, PxReal dtime)												{}
	virtual void		collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents)	{}
	virtual PxReal		getCameraSpeed() { return 0; }
};

class DefaultCameraController : public CameraController
{
	public:
						DefaultCameraController();
	virtual				~DefaultCameraController();

			void		init(const PxVec3& pos, const PxVec3& rot);
			void		init(const PxTransform& pose);
			void		setCameraSpeed(const PxReal speed) { mCameraSpeed = speed; }
			PxReal		getCameraSpeed() { return mCameraSpeed; }
			void		setMouseLookOnMouseButton(bool mouseLookOnMB) { mMouseLookOnMB = mouseLookOnMB; }
			void		setMouseSensitivity(PxReal mouseSensitivity) { mMouseSensitivity = mouseSensitivity; }

	// Implements CameraController
			void		onMouseDelta(PxI32 dx, PxI32 dy);

	virtual void		onAnalogInputEvent(const SampleFramework::InputEvent& , float val);
	virtual void		onDigitalInputEvent(const SampleFramework::InputEvent& , bool val);
	virtual void		onPointerInputEvent(const SampleFramework::InputEvent& ie, physx::PxU32 x, physx::PxU32 y, physx::PxReal dx, physx::PxReal dy, bool val);

	virtual void		collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents);

	virtual	void		update(Camera& camera, PxReal dtime);

	protected:
			PxVec3								mTargetEyePos;
			PxVec3								mTargetEyeRot;
			PxVec3								mEyePos;
			PxVec3								mEyeRot;

			bool								mMouseButtonDown;
			bool								mKeyFWDown;
			bool								mKeyBKDown;
			bool								mKeyRTDown;
			bool								mKeyLTDown;
			bool								mKeyUpDown;
			bool								mKeyDownDown;
			bool								mKeyShiftDown;
			PxReal								mCameraSpeed;
			PxReal								mCameraSpeedMultiplier;
			bool								mMouseLookOnMB;
			PxReal								mMouseSensitivity;
	};

#endif
