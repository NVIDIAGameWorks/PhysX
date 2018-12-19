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

#include "PxMath.h"

using namespace nvidia;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Scenes Setup
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SampleSceneController::AssetDescription SampleSceneController::ASSETS[] = 
{
	{ "Wall_PC", "Wall" },
	{ "arch", "arch" },
	{ "Bunny", "Bunny" },
	{ "block", "block" },
	{ "Fence", "Fence" },
	{ "Post", "Post" },
	{ "Sheet", "Sheet" },
	{ "Moai_PC", "Moai_PC" }
};

int SampleSceneController::getAssetsCount()
{
	return sizeof(ASSETS) / sizeof(ASSETS[0]);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SampleSceneController::SampleSceneController(CFirstPersonCamera* camera, ApexController& apex)
: mApex(apex), mCamera(camera), mActor(NULL)
{
}

SampleSceneController::~SampleSceneController()
{
}

void SampleSceneController::onSampleStart()
{
	// setup camera
	DirectX::XMVECTORF32 lookAtPt = { 0, 10, 0, 0 };
	DirectX::XMVECTORF32 eyePt = { 0, 20, 60, 0 };
	mCamera->SetViewParams(eyePt, lookAtPt);
	mCamera->SetRotateButtons(false, false, true, false);
	mCamera->SetEnablePositionMovement(true);

	// spawn actor
	setCurrentAsset(0);
}

void SampleSceneController::setCurrentAsset(int num)
{
	int assetsCount = getAssetsCount();
	num = nvidia::PxClamp(num, 0, assetsCount - 1);

	mCurrentAsset = num;
	if(mActor != NULL)
	{
		mApex.removeActor(mActor);
		mActor = NULL;
	}
	mActor = mApex.spawnDestructibleActor(ASSETS[num].model);
}

void SampleSceneController::fire(float mouseX, float mouseY)
{
	PxVec3 eyePos, pickDir;
	mApex.getEyePoseAndPickDir(mouseX, mouseY, eyePos, pickDir);

	if (pickDir.normalize() > 0)
	{
		float time;
		PxVec3 normal;
		const int chunkIndex = mActor->rayCast(time, normal, eyePos, pickDir, DestructibleActorRaycastFlags::AllChunks);
		if (chunkIndex != ModuleDestructibleConst::INVALID_CHUNK_INDEX)
		{
			mActor->applyDamage(PX_MAX_F32, 0.0f, eyePos + time * pickDir, pickDir, chunkIndex);
		}
	}
}
