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

using namespace physx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Scenes Setup
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SampleSceneController::AssetDescription SampleSceneController::ASSETS[] =
{
	{ "defaultEffectPackage", "defaultEffectPackage" },
	{ "hotlava", "hotlava" },
	{ "pulseblast", "pulseblast" },
	{ "bigtesteffect", "bigtesteffect" },
	{ "ThreeEmitterScaleTest", "ThreeEmitterScaleTest" },
	{ "smoketest", "smoketest" }
};

int SampleSceneController::getAssetsCount()
{
	return sizeof(ASSETS) / sizeof(ASSETS[0]);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SampleSceneController::SampleSceneController(CFirstPersonCamera* camera, ApexController& apex)
	: mApex(apex), mCamera(camera), mDraggingActor(NULL)
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

		// load effect package DB
		loadEffectPackageDatabase();

		// spawn actor
		setCurrentAsset(0);
	}
}

void SampleSceneController::loadEffectPackageDatabase()
{
	NvParameterized::Interface *iface;

	ApexResourceCallback* resourceCallback = mApex.getResourceCallback();
	ModuleParticles* moduleParticles = mApex.getModuleParticles();
	if (moduleParticles == NULL)
	{
		return;
	}

	// Load the EffectPackage database
	iface = resourceCallback->deserializeFromFile("EffectPackageEffectPackagesDatabase.apb");
	PX_ASSERT(iface);
	if (iface)
	{
		moduleParticles->setEffectPackageDatabase(iface);
		iface->destroy();
	}

	// Load the Emitter database
	iface = resourceCallback->deserializeFromFile("EffectPackageEmittersDatabase.apb");
	PX_ASSERT(iface);
	if (iface)
	{
		moduleParticles->setEffectPackageEmitterDatabase(iface);
		iface->destroy();
	}

	// Load the FieldSampler database
	iface = resourceCallback->deserializeFromFile("EffectPackageFieldSamplersDatabase.apb");
	PX_ASSERT(iface);
	if (iface)
	{
		moduleParticles->setEffectPackageFieldSamplerDatabase(iface);
		iface->destroy();
	}
	// Load the GraphicsEffects database (IOFX)
	iface = resourceCallback->deserializeFromFile("EffectPackageGraphicsEffectsDatabase.apb");
	PX_ASSERT(iface);
	if (iface)
	{
		moduleParticles->setEffectPackageIOFXDatabase(iface);
		iface->destroy();
	}

	// Load the GraphicsMaterials database
	iface = resourceCallback->deserializeFromFile("EffectPackageGraphicsMaterialsDatabase.apb");
	PX_ASSERT(iface);
	if (iface)
	{
		moduleParticles->setEffectPackageGraphicsMaterialsDatabase(iface);
		iface->destroy();
	}

	// Load the Particle Simulations (IOS) database
	iface = resourceCallback->deserializeFromFile("EffectPackageParticleSimulationsDatabase.apb");
	PX_ASSERT(iface);
	if (iface)
	{
		moduleParticles->setEffectPackageIOSDatabase(iface);
		iface->destroy();
	}
}

void SampleSceneController::setCurrentAsset(int num)
{
	if (mApex.getModuleParticles())
	{
		int assetsCount = getAssetsCount();
		num = nvidia::PxClamp(num, 0, assetsCount - 1);

		mCurrentAsset = num;
		if (mActor != NULL)
		{
			mApex.removeActor(mActor);
			mActor = NULL;
		}
		mActor = mApex.spawnEffectPackageActor(ASSETS[num].model);
	}
}

void SampleSceneController::Animate(double dt)
{
	if (mDraggingActor != NULL)
	{
		const float DRAGGING_FORCE_FACTOR = 10.0f;
		const float DRAGGING_VELOCITY_FACTOR = 2.0f;
		PxVec3 direction = (mDragAttractionPoint - mDraggingActor->getGlobalPose().transform(mDraggingActorHookLocalPoint));
		nvidia::PxVec3 force = (direction * DRAGGING_FORCE_FACTOR - DRAGGING_VELOCITY_FACTOR * mDraggingActor->getLinearVelocity()) * mDraggingActor->getMass() * dt;
		physx::PxRigidBodyExt::addForceAtLocalPos(*mDraggingActor, force, mDraggingActorHookLocalPoint, physx::PxForceMode::eIMPULSE, true);
	}
}

void SampleSceneController::throwCube()
{
	PxVec3 eyePos = XMVECTORToPxVec4(mCamera->GetEyePt()).getXYZ();
	PxVec3 lookAtPos = XMVECTORToPxVec4(mCamera->GetLookAtPt()).getXYZ();
	PhysXPrimitive* box = mApex.spawnPhysXPrimitiveBox(PxTransform(eyePos));
	PxRigidDynamic* rigidDynamic = box->getActor()->is<PxRigidDynamic>();

	const float CUBE_SPEED = 30.0f;
	PxVec3 dir = (lookAtPos - eyePos).getNormalized();
	rigidDynamic->setLinearVelocity(dir * CUBE_SPEED);
}

void SampleSceneController::onTouchEvent(TouchEvent touchEvent, float mouseX, float mouseY)
{
	PxVec3 eyePos, pickDir;
	mApex.getEyePoseAndPickDir(mouseX, mouseY, eyePos, pickDir);
	pickDir = pickDir.getNormalized();

	if (touchEvent == TouchEvent::ePRESS)
	{
		if (pickDir.magnitude() > 0)
		{
			physx::PxRaycastHit	hit;
			physx::PxRaycastBuffer rcBuffer(&hit, 1);
			bool isHit = mApex.getApexScene()->getPhysXScene()->raycast(eyePos, pickDir, PX_MAX_F32, rcBuffer, physx::PxHitFlag::ePOSITION, physx::PxQueryFilterData(physx::PxQueryFlag::eDYNAMIC));
			if (isHit)
			{
				mDragDistance = (eyePos - hit.position).magnitude();
				mDraggingActor = hit.actor->is<PxRigidDynamic>();
				mDraggingActorHookLocalPoint = mDraggingActor->getGlobalPose().getInverse().transform(hit.position);
				mDraggingActor->setLinearVelocity(PxVec3(0, 0, 0));
				mDraggingActor->setAngularVelocity(PxVec3(0, 0, 0));
			}
		}
	}
	else if (touchEvent == TouchEvent::eDRAG)
	{
		physx::PxRaycastHit	hit;
		physx::PxRaycastBuffer rcBuffer(&hit, 1);
		bool isHit = mApex.getApexScene()->getPhysXScene()->raycast(eyePos, pickDir, PX_MAX_F32, rcBuffer, physx::PxHitFlag::ePOSITION, physx::PxQueryFilterData(physx::PxQueryFlag::eSTATIC));
		if (isHit)
		{
			mDragDistance = PxMin(mDragDistance, (eyePos - hit.position).magnitude());
		}

		mDragAttractionPoint = eyePos + pickDir * mDragDistance;
	}
	else if (touchEvent == TouchEvent::eRELEASE)
	{
		mDraggingActor = NULL;
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

