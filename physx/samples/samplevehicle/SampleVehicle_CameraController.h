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


#ifndef SAMPLE_VEHICLE_CAMERA_CONTROLLER_H
#define SAMPLE_VEHICLE_CAMERA_CONTROLLER_H

#include "common/PxPhysXCommonConfig.h"
#include "foundation/PxVec3.h"
#include "foundation/PxTransform.h"

using namespace physx;

namespace physx
{
	class PxScene;
	class PxVehicleWheels;
	class PxRigidDynamic;
}

class Camera;

class SampleVehicle_CameraController 
{
public:

	SampleVehicle_CameraController();
	~SampleVehicle_CameraController();

	void setInputs(const PxF32 rotateInputY, const PxF32 rotateInputZ)
	{
		mRotateInputY=rotateInputY;
		mRotateInputZ=rotateInputZ;
	}

	void update(const PxF32 dtime, const PxVehicleWheels& focusVehicle, PxScene& scene);

	void restart() {}

	bool getIsLockedOnVehicleTransform() const {return mLockOnFocusVehTransform;}
	void toggleLockOnVehTransform() {mLockOnFocusVehTransform = !mLockOnFocusVehTransform;}
	
	const PxVec3& getCameraPos() const {return mCameraPos;}
	const PxVec3& getCameraTar() const {return mCameraTargetPos;}

private:

	PxF32			mRotateInputY;
	PxF32			mRotateInputZ;

	PxF32			mMaxCameraRotateSpeed;
	PxF32			mCameraRotateAngleY;
	PxF32			mCameraRotateAngleZ;
	PxVec3			mCameraPos;
	PxVec3			mCameraTargetPos;
	PxVec3			mLastCarPos;
	PxVec3			mLastCarVelocity;
	bool			mCameraInit;

	bool			mLockOnFocusVehTransform;
	PxTransform		mLastFocusVehTransform;

	void update(const PxReal dtime, const PxRigidDynamic* actor, PxScene& scene);
};

#endif //SAMPLE_VEHICLE_CAMERA_CONTROLLER_H