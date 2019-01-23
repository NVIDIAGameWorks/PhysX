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

#include "PhysXSample.h"
#include "SampleCameraController.h"
#include "characterkinematic/PxController.h"

class SampleNorthPoleCameraController : public CameraController
{
	public:
		SampleNorthPoleCameraController(PxCapsuleController& controlled, SampleNorthPole& base);

		virtual void			onDigitalInputEvent(const SampleFramework::InputEvent& , bool val);
		virtual void			onAnalogInputEvent(const SampleFramework::InputEvent& , float val);
		virtual void			onPointerInputEvent(const SampleFramework::InputEvent&, PxU32 x, PxU32 y, PxReal dx, PxReal dy, bool val);

		virtual void			collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents);

		virtual	void			update(Camera& camera, PxReal dtime);

				void			setView(PxReal pitch, PxReal yaw);

	private:
		SampleNorthPoleCameraController& operator=(const SampleNorthPoleCameraController&);
				PxExtendedVec3	computeCameraTarget();

		PxCapsuleController&	mCCT;
		SampleNorthPole&		mBase;	// PT: TODO: find a way to decouple us from PhysXSampleApplication. Only needed for "recenterCursor". Maybe the app could inherit from the cam...

				PxReal			mTargetYaw, mTargetPitch;
				PxReal			mPitchMin,	mPitchMax;

				PxReal			mGamepadPitchInc, mGamepadYawInc;
				PxReal			mGamepadForwardInc, mGamepadLateralInc;
				PxReal			mSensibility;

				bool			mFwd,mBwd,mLeft,mRight,mKeyShiftDown;

				PxReal			mRunningSpeed;
				PxReal			mWalkingSpeed;

				PxF32			mFilterMemory;
};
