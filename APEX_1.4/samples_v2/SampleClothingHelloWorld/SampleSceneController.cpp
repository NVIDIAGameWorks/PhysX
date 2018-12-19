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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.


#include "SampleSceneController.h"
#include "Apex.h"
#include "ApexRenderer.h" // for matrix conversion
#include <DirectXMath.h>
#include "XInput.h"
#include "DXUTMisc.h"
#pragma warning(push)
#pragma warning(disable : 4481) // Suppress "nonstandard extension used" warning
#include "DXUTCamera.h"
#pragma warning(pop)

#include "ApexResourceCallback.h"
#include "ClothingCollision.h"

#include "PxMath.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SampleSceneController::SampleSceneController(CFirstPersonCamera* camera, ApexController& apex)
: mApex(apex), mCamera(camera), mActor(NULL), mTime(0)
{
}

SampleSceneController::~SampleSceneController()
{
}

void SampleSceneController::onSampleStart()
{
	// setup camera
	DirectX::XMVECTORF32 lookAtPt = { 0, 45, 0, 0 };
	DirectX::XMVECTORF32 eyePt = { 0, 50, 100, 0 };
	mCamera->SetViewParams(eyePt, lookAtPt);
	mCamera->SetRotateButtons(false, false, true, false);
	mCamera->SetEnablePositionMovement(true);

	// spawn actor
	mActor = mApex.spawnClothingActor("plane32");

	nvidia::PxMat44 globalPose = nvidia::PxMat44(nvidia::PxIdentity);
	globalPose(1, 1) = 0;
	globalPose(2, 2) = 0;
	globalPose(1, 2) = 1;
	globalPose(2, 1) = -1;

	globalPose.setPosition(PxVec3(0, -40, 0));

	mActor->updateState(globalPose, NULL, 0, 0, ClothingTeleportMode::TeleportAndReset);
}

void SampleSceneController::Animate(double dt)
{
	for (std::map<PxRigidDynamic*, ClothingSphere*>::iterator it = mSpheres.begin(); it != mSpheres.end(); it++)
	{
		(*it).second->setPosition((*it).first->getGlobalPose().p);
	}

	mTime += dt;

	mActor->setWind(0.9f, PxVec3(30 + 7 * PxSin(mTime), 0, 40 + 10 * PxCos(mTime)));
}

void SampleSceneController::throwSphere()
{
	PxVec3 eyePos = XMVECTORToPxVec4(mCamera->GetEyePt()).getXYZ();
	PxVec3 lookAtPos = XMVECTORToPxVec4(mCamera->GetLookAtPt()).getXYZ();
	PhysXPrimitive* box = mApex.spawnPhysXPrimitiveBox(PxTransform(eyePos));
	PxRigidDynamic* rigidDynamic = box->getActor()->is<PxRigidDynamic>();
	box->setColor(DirectX::XMFLOAT3(1, 1, 1));

	PxVec3 dir = (lookAtPos - eyePos).getNormalized();
	rigidDynamic->setLinearVelocity(dir * 50);

	ClothingSphere* sphere = mActor->createCollisionSphere(rigidDynamic->getGlobalPose().p, 5);
	mSpheres[rigidDynamic] = sphere;
}
