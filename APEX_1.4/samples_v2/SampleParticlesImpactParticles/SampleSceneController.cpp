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
#include "ApexRenderer.h" // for matrix conversion
#include <DirectXMath.h>
#include "XInput.h"
#include "DXUTMisc.h"
#pragma warning(push)
#pragma warning(disable : 4481) // Suppress "nonstandard extension used" warning
#include "DXUTCamera.h"
#pragma warning(pop)

#include "PxPhysicsAPI.h"
#include "PxMath.h"

#include "ApexResourceCallback.h"
#include "PhysXPrimitive.h"

#include "ImpactEmitterAsset.h"
#include "ImpactEmitterActor.h"

using namespace physx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SampleSceneController::SampleSceneController(CFirstPersonCamera* camera, ApexController& apex)
	: mApex(apex), mCamera(camera), mActor(NULL), mAsset(NULL)
{
}

SampleSceneController::~SampleSceneController()
{
}

void SampleSceneController::onSampleStart()
{
	PX_ASSERT_WITH_MESSAGE(mApex.getModuleParticles(), "Particle dll can't be found or ApexFramework was built withoud particles support");
	if (mApex.getModuleParticles())
	{
		// setup camera
		DirectX::XMVECTORF32 lookAtPt = {0, 2, 0, 0};
		DirectX::XMVECTORF32 eyePt = {0, 5, 10, 0};
		mCamera->SetViewParams(eyePt, lookAtPt);
		mCamera->SetRotateButtons(false, false, true, false);
		mCamera->SetEnablePositionMovement(true);

		// spawn mesh emitter
		mAsset = (nvidia::apex::ImpactEmitterAsset *)mApex.getApexSDK()->getNamedResourceProvider()->getResource(IMPACT_EMITTER_AUTHORING_TYPE_NAME, "testImpactEmitter");
		NvParameterized::Interface *defaultActorDesc = mAsset->getDefaultActorDesc();
		NvParameterized::setParamTransform(*defaultActorDesc, "InitialPose", PxTransform(PxIdentity));
		mActor = (ImpactEmitterActor *)mAsset->createApexActor(*defaultActorDesc, *(mApex.getApexScene()));
	}
}


void SampleSceneController::fire(float mouseX, float mouseY)
{
	PxVec3 eyePos, pickDir;
	mApex.getEyePoseAndPickDir(mouseX, mouseY, eyePos, pickDir);

	if (mApex.getModuleParticles() == NULL || pickDir.normalize() <= 0)
	{
		return;
	}

	PxHitFlags outputFlags;
	outputFlags |= PxHitFlag::ePOSITION;
	outputFlags |= PxHitFlag::eDISTANCE;
	PxRaycastBuffer rcBuffer;
	bool collision = false;

	PxScene* scene = mApex.getApexScene()->getPhysXScene();
	scene->lockRead(__FILE__, __LINE__);
	collision = scene->raycast(eyePos, pickDir, 1e6, rcBuffer, outputFlags);
	scene->unlockRead();
	
	uint32_t surfType = mAsset->querySetID("meshParticleEvent");

	if (mActor)
	{
		mActor->registerImpact(rcBuffer.block.position, pickDir, rcBuffer.block.normal, surfType);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

