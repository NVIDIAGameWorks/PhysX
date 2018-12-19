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

#include "EmitterAsset.h"
#include "EmitterActor.h"

using namespace physx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SampleSceneController::SampleSceneController(CFirstPersonCamera* camera, ApexController& apex)
	: mApex(apex), mCamera(camera)
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
		nvidia::apex::Asset *asset = (nvidia::apex::Asset *)mApex.getApexSDK()->getNamedResourceProvider()->getResource(EMITTER_AUTHORING_TYPE_NAME, "testMeshEmitter4ParticleIos");
		NvParameterized::Interface *defaultActorDesc = asset->getDefaultActorDesc();
		NvParameterized::setParamTransform(*defaultActorDesc, "InitialPose", PxTransform(PxIdentity));
		EmitterActor *actor = (EmitterActor*)asset->createApexActor(*defaultActorDesc, *(mApex.getApexScene()));

		actor->startEmit(true);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

