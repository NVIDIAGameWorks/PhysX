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

#include "VehicleCameraController.h"
#include "PxRigidDynamic.h"
#include "PxQueryFiltering.h"
#include "PxScene.h"
#include "PxSceneLock.h"
#include "vehicle/PxVehicleWheels.h"

///////////////////////////////////////////////////////////////////////////////



VehicleCameraController::VehicleCameraController()
	: mRotateInputY(0.0f),
	mRotateInputZ(0.0f),
	mMaxCameraRotateSpeed(5.0f),
	mCameraRotateAngleY(0.0f),
	mCameraRotateAngleZ(0.13f),
	mCameraPos(PxVec3(0, 0, 0)),
	mCameraTargetPos(PxVec3(0, 0, 0)),
	mLastCarPos(PxVec3(0, 0, 0)),
	mLastCarVelocity(PxVec3(0, 0, 0)),
	mCameraInit(false),
	mLockOnFocusVehTransform(true),
	mLastFocusVehTransform(PxTransform(PxIdentity)),
	mCamDist(5.f)
{
}

VehicleCameraController::~VehicleCameraController()
{
}

static void dampVec3(const PxVec3& oldPosition, PxVec3& newPosition, PxF32 timestep)
{
	PxF32 t = 0.7f * timestep * 8.0f;
	t = PxMin(t, 1.0f);
	newPosition = oldPosition * (1 - t) + newPosition * t;
}


void VehicleCameraController::update(const PxReal dtime, const PxRigidDynamic* actor, PxScene& scene)
{
	PxSceneReadLock scopedLock(scene);
	PxTransform carChassisTransfm;
	if (mLockOnFocusVehTransform)
	{
		carChassisTransfm = actor->getGlobalPose();
		mLastFocusVehTransform = carChassisTransfm;
	}
	else
	{
		carChassisTransfm = mLastFocusVehTransform;
	}

	PxF32 camDist = 5.f;
	PxF32 cameraYRotExtra = 0.0f;

	PxVec3 velocity = mLastCarPos - carChassisTransfm.p;

	if (mCameraInit)
	{
		//Work out the forward and sideways directions.
		PxVec3 unitZ(0, 0, 1);
		PxVec3 carDirection = carChassisTransfm.q.rotate(unitZ);
		PxVec3 unitX(1, 0, 0);
		PxVec3 carSideDirection = carChassisTransfm.q.rotate(unitX);

		//Acceleration (note that is not scaled by time).
		PxVec3 acclVec = mLastCarVelocity - velocity;

		//Higher forward accelerations allow the car to speed away from the camera.
		PxF32 velZ = PxAbs(carDirection.dot(velocity));
		camDist = PxMin(PxMax(camDist + velZ*100.0f, 10.0f), 15.f);

		//Higher sideways accelerations allow the car's rotation to speed away from the camera's rotation.
		PxF32 acclX = carSideDirection.dot(acclVec);
		cameraYRotExtra = -acclX*10.0f;
		//At very small sideways speeds the camera greatly amplifies any numeric error in the body and leads to a slight jitter.
		//Scale cameraYRotExtra by a value in range (0,1) for side speeds in range (0.1,1.0) and by zero for side speeds less than 0.1.
		PxFixedSizeLookupTable<4> table;
		table.addPair(0.0f, 0.0f);
		table.addPair(0.1f*dtime, 0);
		table.addPair(1.0f*dtime, 1);
		PxF32 velX = carSideDirection.dot(velocity);
		cameraYRotExtra *= table.getYVal(PxAbs(velX));
	}

	mCameraRotateAngleY += mRotateInputY*mMaxCameraRotateSpeed*dtime;
	mCameraRotateAngleY = physx::intrinsics::fsel(mCameraRotateAngleY - 10 * PxPi, mCameraRotateAngleY - 10 * PxPi, physx::intrinsics::fsel(-mCameraRotateAngleY - 10 * PxPi, mCameraRotateAngleY + 10 * PxPi, mCameraRotateAngleY));
	mCameraRotateAngleZ += mRotateInputZ*mMaxCameraRotateSpeed*dtime;
	mCameraRotateAngleZ = PxClamp(mCameraRotateAngleZ, -PxPi*0.05f, PxPi*0.45f);

	PxVec3 cameraDir = PxVec3(0, 0, 1)*PxCos(mCameraRotateAngleY + cameraYRotExtra) + PxVec3(1, 0, 0)*PxSin(mCameraRotateAngleY + cameraYRotExtra);

	cameraDir = cameraDir*PxCos(mCameraRotateAngleZ) - PxVec3(0, 1, 0)*PxSin(mCameraRotateAngleZ);

	const PxVec3 direction = carChassisTransfm.q.rotate(cameraDir);
	PxVec3 target = carChassisTransfm.p;
	target.y += 0.5f;

	PxRaycastBuffer hit;
	PxQueryFilterData filterData(PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC);
	scene.raycast(target, -direction, camDist, hit, PxHitFlag::eDEFAULT, filterData);
	if (hit.hasBlock && hit.block.shape != NULL)
	{
		camDist = hit.block.distance - 0.25f;
	}

	camDist = PxMax(4.0f, PxMin(camDist, 50.0f));

	mCamDist += PxMax(camDist - mCamDist, 0.f)*0.05f + PxMin(0.f, camDist - mCamDist);
	camDist = mCamDist;

	PxVec3 position = target - direction*camDist;

	if (mCameraInit)
	{
		dampVec3(mCameraPos, position, dtime);
		dampVec3(mCameraTargetPos, target, dtime);
	}

	mCameraPos = position;
	mCameraTargetPos = target;
	mCameraInit = true;

	mLastCarVelocity = velocity;
	mLastCarPos = carChassisTransfm.p;
}

void VehicleCameraController::update(const PxReal dtime, const PxVehicleWheels& focusVehicle, PxScene& scene)
{
	const PxRigidDynamic* actor = focusVehicle.getRigidDynamicActor();
	update(dtime, actor, scene);
}


