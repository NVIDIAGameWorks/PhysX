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

#include "characterkinematic/PxControllerManager.h"

#include "PxPhysicsAPI.h"

#include "SampleCustomGravity.h"
#include "SampleCustomGravityCameraController.h"
#include "SampleCustomGravityInputEventIds.h"

#include "SampleUtils.h"
#include "SampleCommandLine.h"
#include "SampleConsole.h"
#include "SampleAllocatorSDKClasses.h"
#include "RendererMemoryMacros.h"
#include "RenderMaterial.h"
#include "RenderBaseActor.h"
#include "RenderPhysX3Debug.h"

#include "characterkinematic/PxBoxController.h"
#include "characterkinematic/PxCapsuleController.h"


REGISTER_SAMPLE(SampleCustomGravity, "SampleCustomGravity")

#define PLANET_RADIUS	20.0f
//#define PLANET_RADIUS	100.0f
//#define PLANET_RADIUS	10.0f

using namespace SampleFramework;
using namespace SampleRenderer;

///////////////////////////////////////////////////////////////////////////////

SampleCustomGravity::SampleCustomGravity(PhysXSampleApplication& app) :
	PhysXSample				(app),
	mCCTCamera				(NULL),
#ifdef USE_BOX_CONTROLLER
	mBoxController			(NULL),
#else
	mCapsuleController		(NULL),
#endif
	mControllerManager		(NULL),
	mSnowMaterial			(NULL),
	mPlatformMaterial		(NULL),
	mDrawInvisibleWalls		(false),
	mEnableCCTDebugRender	(false)
{
	mValidTouchedTriangle	= false;

	mCreateGroundPlane	= false;
	//mStepperType = FIXED_STEPPER;
	mControllerRadius	= 0.5f;
	mControllerInitialPosition = PxExtendedVec3(0.0, PLANET_RADIUS + 4.0f, 0.0);
//	mUseDebugStepper	= true;

	mRenderActor = NULL;
	
	mPlanet.mPos = PxVec3(0.0f, 0.0f, 0.0f);
	mPlanet.mRadius = PLANET_RADIUS;
}

SampleCustomGravity::~SampleCustomGravity()
{
}

///////////////////////////////////////////////////////////////////////////////

void SampleCustomGravity::customizeSample(SampleSetup& setup)
{
	setup.mName	= "SampleCustomGravity";
}

///////////////////////////////////////////////////////////////////////////////

extern PxF32 gJumpForce;
static void gJump(Console* console, const char* text, void* userData)
{
	if(!text)
	{
		console->out("Usage: jump <float>");
		return;
	}

	const float val = (float)::atof(text);
//	shdfnd::printFormatted("value: %f\n", val);
	gJumpForce = val;
}

// PT: TODO: use PhysXSampleApplication helper for this

static PxRigidActor* createRigidActor(	PxScene& scene, PxPhysics& physics,
										const PxTransform& pose, const PxGeometry& geometry, PxMaterial& material,
										const PxFilterData* fd, const PxReal* density, const PxReal* mass, PxU32 flags)
{
	const bool isDynamic = (density && *density) || (mass && *mass);

	PxRigidActor* actor = isDynamic ? static_cast<PxRigidActor*>(physics.createRigidDynamic(pose))
									: static_cast<PxRigidActor*>(physics.createRigidStatic(pose));
	if(!actor)
		return NULL;

	PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor, geometry, material);
	if(!shape)
	{
		actor->release();
		return NULL;
	}

	if(fd)
		shape->setSimulationFilterData(*fd);

	if(isDynamic)
	{
		PxRigidDynamic* body = static_cast<PxRigidDynamic*>(actor);
		{
			if(density)
				PxRigidBodyExt::updateMassAndInertia(*body, *density);
			else
				PxRigidBodyExt::setMassAndUpdateInertia(*body, *mass);
		}
	}

	scene.addActor(*actor);

	return actor;
}

static PxRigidActor* createBox(	PxScene& scene, PxPhysics& physics, 
								const PxTransform& pose, const PxVec3& dimensions, const PxReal density, PxMaterial& m, const PxFilterData* fd, PxU32 flags)
{
	return createRigidActor(scene, physics, pose, PxBoxGeometry(dimensions), m, fd, &density, NULL, flags);
}

KinematicPlatform* SampleCustomGravity::createPlatform(PxU32 nbPts, const PxVec3* pts, const PxQuat& localRot, const PxVec3& extents, PxF32 platformSpeed, PxF32 rotationSpeed)
{
	const PxTransform idt = PxTransform(PxIdentity);

	PxRigidDynamic* platformActor = (PxRigidDynamic*)::createBox(getActiveScene(), getPhysics(), idt, extents, 1.0f, getDefaultMaterial(), NULL, 0);
/*#ifdef CCT_ON_BRIDGES
	platformActor->setName(gPlatformName);
#endif*/
	platformActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

	PxShape* platformShape = NULL;
	platformActor->getShapes(&platformShape, 1);
	PX_ASSERT(platformShape);
	createRenderObjectFromShape(platformActor, platformShape, mPlatformMaterial);

/*#ifdef PLATFORMS_AS_OBSTACLES
//	platformShape->userData = NULL;
	renderActor->setPhysicsShape(NULL);
#endif*/

	return mPlatformManager.createPlatform(nbPts, pts, idt, localRot, platformActor, platformSpeed, rotationSpeed, LOOP_WRAP);
}

#include "RendererDirectionalLightDesc.h"
void SampleCustomGravity::onInit()
{
	if(getConsole())
		getConsole()->addCmd("jump", gJump);

	mCreateCudaCtxManager = true;
	PhysXSample::onInit();

	PxSceneWriteLock scopedLock(*mScene);

	mApplication.setMouseCursorHiding(true);
	mApplication.setMouseCursorRecentering(true);

//	getRenderer()->setAmbientColor(RendererColor(100, 100, 100));
//	getRenderer()->setAmbientColor(RendererColor(255, 255, 255));
	getRenderer()->setAmbientColor(RendererColor(160, 160, 160));

	if(0)
	{
		RendererDirectionalLightDesc lightdesc;
		lightdesc.intensity = 1;

		lightdesc.color     = RendererColor(250, 125, 64, 255);
		lightdesc.direction = PxVec3(4.0f, 5.0f, 3.0f);
		lightdesc.direction.normalizeFast();
		
		RendererLight* newLight = getRenderer()->createLight(lightdesc);
		mApplication.registerLight(newLight);
	}

	// some colors for the rendering
	mSnowMaterial = SAMPLE_NEW(RenderMaterial)(*getRenderer(), PxVec3(0.90f, 0.90f, 1.00f), 1.0f, false, 0, NULL);
	mRenderMaterials.push_back(mSnowMaterial);

	{
		RAWTexture data;
		data.mName = "divingBoardFloor_diffuse.dds";
		RenderTexture* platformTexture = createRenderTextureFromRawTexture(data);

		mPlatformMaterial = SAMPLE_NEW(RenderMaterial)(*getRenderer(), PxVec3(1.0f, 1.0f, 1.0f), 1.0f, false, 0xffffffff, platformTexture);
		mRenderMaterials.push_back(mPlatformMaterial);
	}

	buildScene();

	// PhysX
	mControllerManager = PxCreateControllerManager(getActiveScene());

#ifdef USE_BOX_CONTROLLER
	mBoxController = createCharacter(mControllerInitialPosition);
#else
	mCapsuleController = createCharacter(mControllerInitialPosition);
#endif

#ifdef USE_BOX_CONTROLLER
	mCCTCamera = SAMPLE_NEW(SampleCustomGravityCameraController)(*mBoxController, *this);
#else
	mCCTCamera = SAMPLE_NEW(SampleCustomGravityCameraController)(*mCapsuleController, *this);
#endif
	setCameraController(mCCTCamera);

	mCCTCamera->setView(0,0);
}

void SampleCustomGravity::onShutdown()
{
	{
		PxSceneWriteLock scopedLock(*mScene);
		DELETESINGLE(mCCTCamera);
		mControllerManager->release();
		mPlatformManager.release();
	}
	PhysXSample::onShutdown();
}

void SampleCustomGravity::helpRender(PxU32 x, PxU32 y, PxU8 textAlpha)
{
	SampleRenderer::Renderer* renderer = getRenderer();
	const PxU32 yInc = 18;
	const PxReal scale = 0.5f;
	const PxReal shadowOffset = 6.0f;
	const RendererColor textColor(255, 255, 255, textAlpha);
	const bool isMouseSupported = getApplication().getPlatform()->getSampleUserInput()->mouseSupported();
	const bool isPadSupported = getApplication().getPlatform()->getSampleUserInput()->gamepadSupported();
	const char* msg;

	if (isMouseSupported && isPadSupported)
		renderer->print(x, y += yInc, "Use mouse or right stick to rotate the camera", scale, shadowOffset, textColor);
	else if (isMouseSupported)
		renderer->print(x, y += yInc, "Use mouse to rotate the camera", scale, shadowOffset, textColor);
	else if (isPadSupported)
		renderer->print(x, y += yInc, "Use right stick to rotate the camera", scale, shadowOffset, textColor);
	if (isPadSupported)
		renderer->print(x, y += yInc, "Use left stick to move",scale, shadowOffset, textColor);
	msg = mApplication.inputMoveInfoMsg("Press "," to move", CAMERA_MOVE_FORWARD,CAMERA_MOVE_BACKWARD, CAMERA_MOVE_LEFT, CAMERA_MOVE_RIGHT);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press "," to move fast", CAMERA_SHIFT_SPEED, -1);
	if(msg)
		renderer->print(x, y += yInc, msg, scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press ","  to jump", CAMERA_JUMP,-1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press "," to throw an object", SPAWN_DEBUG_OBJECT, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
}

void SampleCustomGravity::descriptionRender(PxU32 x, PxU32 y, PxU8 textAlpha)
{
	bool print=(textAlpha!=0.0f);

	if(print)
	{
		Renderer* renderer = getRenderer();
		const PxU32 yInc = 24;
		const PxReal scale = 0.5f;
		const PxReal shadowOffset = 6.0f;
		const RendererColor textColor(255, 255, 255, textAlpha);

		char line0[256]="This sample demonstrates the application of custom gravity to each object";
		char line1[256]="in the scene, rather than relying on a global gravity value.  In this scene";
		char line2[256]="gravity is applied as a force directed along the unit vector connecting the";
		char line3[256]="object position to the center of the planet.  The setup and run-time control";
		char line4[256]="of the PhysX kinematic character controller is also presented.";

		renderer->print(x, y+=yInc, line0, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line1, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line2, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line3, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line4, scale, shadowOffset, textColor);
	}
}

void SampleCustomGravity::onTickPreRender(PxReal dtime)
{
	mValidTouchedTriangle = false;

	PhysXSample::onTickPreRender(dtime);

	if(!mPause)
	{
#ifdef USE_BOX_CONTROLLER
		const PxExtendedVec3& newPos = mBoxController->getPosition();
#else
		const PxExtendedVec3& newPos = mCapsuleController->getPosition();
#endif
		PxTransform tr = PxTransform(PxIdentity);
		tr.p.x = float(newPos.x);
		tr.p.y = float(newPos.y);
		tr.p.z = float(newPos.z);
		tr.q = PxQuat(mCCTCamera->mTest);
		mRenderActor->setTransform(tr);
	}

//	PhysXSample::onTickPreRender(dtime);
}

void SampleCustomGravity::onTickPostRender(PxF32 dtime)
{
	PhysXSample::onTickPostRender(dtime);

	RenderPhysX3Debug* renderer = getDebugRenderer();

	// PT: add CCT's internal debug rendering
	if(mEnableCCTDebugRender)
	{
		mControllerManager->setDebugRenderingFlags(PxControllerDebugRenderFlag::eALL);
		PxRenderBuffer& renderBuffer = mControllerManager->getRenderBuffer();
		renderer->update(renderBuffer);
		renderBuffer.clear();
	}
	else
	{
		mControllerManager->setDebugRenderingFlags(PxControllerDebugRenderFlag::eNONE);
	}

	if(mValidTouchedTriangle)
	{
		PxVec3 normal = (mTouchedTriangle[0] - mTouchedTriangle[1]).cross(mTouchedTriangle[0] - mTouchedTriangle[2]);
		normal.normalize();
		normal *= 0.01f;
		renderer->addTriangle(mTouchedTriangle[0]+normal, mTouchedTriangle[1]+normal, mTouchedTriangle[2]+normal, 0x00ff00ff);
	}

	if(1)
	{
		PxU32 size = mCCTCamera->mNbRecords;
		if(size>1)
		{
			const PxVec3* pos = mCCTCamera->mHistory;
			for(PxU32 i=0;i<size-1;i++)
				renderer->addLine(pos[i], pos[i+1], RendererColor(255,0,255));
		}
	}

	PxController* ctrl;
#ifdef USE_BOX_CONTROLLER
	ctrl = mBoxController;
#else
	ctrl = mCapsuleController;
#endif

	if(1 && ctrl)
	{
		const PxVec3 currentPos = toVec3(ctrl->getPosition());

		PxVec3 up;
		mPlanet.getUpVector(up, currentPos);

		const float length = 2.0f;
		const PxMat33& rot = mCCTCamera->mTest;

		renderer->addLine(currentPos, currentPos + rot.column0 * length, RendererColor(255,0,0));
		renderer->addLine(currentPos, currentPos + rot.column1 * length, RendererColor(0,255,0));
		renderer->addLine(currentPos, currentPos + rot.column2 * length, RendererColor(0,0,255));

		const PxVec3 groundPos = toVec3(ctrl->getFootPosition());
		renderer->addLine(groundPos, groundPos + rot.column0 * length, RendererColor(255,0,0));
		renderer->addLine(groundPos, groundPos + rot.column1 * length, RendererColor(0,255,0));
		renderer->addLine(groundPos, groundPos + rot.column2 * length, RendererColor(0,0,255));
	}
}

void SampleCustomGravity::onSubstep(float dtime)
{
	mPlatformManager.updatePhysicsPlatforms(dtime);

	PxSceneWriteLock scopedLock(*mScene);

	PxU32 nb = (PxU32)mDebugActors.size();
	for(PxU32 i=0;i<nb;i++)
	{
		PxRigidDynamic* dyna = mDebugActors[i];
		// Don't accumulate forces in sleeping objects, else they explode when waking up
		if(dyna->isSleeping())
			continue;

		PxTransform pose = dyna->getGlobalPose();

		PxVec3 up;
		mPlanet.getUpVector(up, pose.p);

//		const PxVec3 force = -up * 9.81f;
//		const PxVec3 force = -up * 9.81f * 0.25f;
		const PxVec3 force = -up * 9.81f * 4.0f;
//		const PxVec3 force = -up * 9.81f * 10.0f;

		dyna->addForce(force, PxForceMode::eACCELERATION, false);
//		dyna->addForce(force, PxForceMode::eIMPULSE);
	}
}


void SampleCustomGravity::onPointerInputEvent(const SampleFramework::InputEvent& ie, physx::PxU32 x, physx::PxU32 y, physx::PxReal dx, physx::PxReal dy, bool val)
{
	if((ie.m_Id == RAYCAST_HIT) && val)
	{
		PxRaycastBuffer hit;
		getActiveScene().raycast(getCamera().getPos()+getCamera().getViewDir(),getCamera().getViewDir(),1,hit);
		shdfnd::printFormatted("hits: %p\n",hit.block.shape);
	}
}

void SampleCustomGravity::onDebugObjectCreation(PxRigidDynamic* actor)
{
	PxTransform pose;
	pose.p = mCCTCamera->mTarget;
	pose.q = PxQuat(PxIdentity);
	actor->setGlobalPose(pose);

	mDebugActors.push_back(actor);
}

PxU32 SampleCustomGravity::getDebugObjectTypes() const
{
	return DEBUG_OBJECT_BOX | DEBUG_OBJECT_SPHERE | DEBUG_OBJECT_CAPSULE | DEBUG_OBJECT_CONVEX;
}
