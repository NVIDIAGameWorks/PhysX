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

#include "SamplePreprocessor.h"
#include "SampleBridges.h"
#include "SampleUtils.h"
#include "RendererMemoryMacros.h"
#include "KinematicPlatform.h"
#include "RenderMaterial.h"
#include "RenderBaseActor.h"
#include "RenderPhysX3Debug.h"
#include <SamplePlatform.h>
#include <SampleUserInput.h>
#include <SampleBaseInputEventIds.h>
#include "SampleBridgesInputEventIds.h"

#include "PxPhysicsAPI.h"
#include "extensions/PxExtensionsAPI.h"
#include "PsMathUtils.h"

#ifdef CCT_ON_BRIDGES
#include "characterkinematic/PxControllerManager.h"
#include "SampleCCTActor.h"
#include "SampleCCTCameraController.h"

	#define CONTACT_OFFSET			0.01f
//	#define CONTACT_OFFSET			0.1f
//	#define STEP_OFFSET				0.01f
	#define STEP_OFFSET				0.05f
//	#define STEP_OFFSET				0.1f
//	#define STEP_OFFSET				0.2f

//	#define SLOPE_LIMIT				0.8f
	#define SLOPE_LIMIT				0.0f
//	#define INVISIBLE_WALLS_HEIGHT	6.0f
	#define INVISIBLE_WALLS_HEIGHT	0.0f
//	#define MAX_JUMP_HEIGHT			4.0f
	#define MAX_JUMP_HEIGHT			0.0f

	static const float gScaleFactor			= 1.5f;
	static const float gStandingSize		= 1.0f * gScaleFactor;
	static const float gCrouchingSize		= 0.25f * gScaleFactor;
	static const float gControllerRadius	= 0.3f * gScaleFactor;

	const char* gPlankName		= "Plank";
	const char* gPlatformName	= "Platform";

#endif

using namespace SampleRenderer;
using namespace SampleFramework;

REGISTER_SAMPLE(SampleBridges, "SampleBridges")

//using namespace physx;

static const bool gBreakableBridges = false;
static const PxReal gBreakForce = 1.0f;
//static const PxReal gPlatformSpeed = 100.0f;
static const PxReal gPlatformSpeed = 10.0f;
//static const PxReal gPlatformSpeed = 5.0f;
static const PxReal gGlobalScale = 2.0f;

static const PxReal gBoxUVs[] =
{
	0.0f, 180.0f/256.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 180.0f/256.0f,

	0.0f, 180.0f/256.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 180.0f/256.0f,

	0.0f, 180.0f/256.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 180.0f/256.0f,

	0.0f, 180.0f/256.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 180.0f/256.0f,

	0.0f, 180.0f/256.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 180.0f/256.0f,

	0.0f, 180.0f/256.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 180.0f/256.0f,
};

///////////////////////////////////////////////////////////////////////////////

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

enum BridgeJoint
{
	BRIDGE_REVOLUTE,
	BRIDGE_SPHERICAL,
	BRIDGE_FIXED,

	BRIDGE_COUNT
};

// PT: TODO: share this function with original code in Visual Tests?
static void buildBridge(PxScene& scene, PxPhysics& physics, PxMaterial& material, std::vector<PxRigidActor*>& actors, std::vector<PxJoint*>& joints, const PxTransform& tr, PxReal scale, BridgeJoint type)
{
	const PxVec3 offset(0,0,0);

	const PxReal groundLength = 5.0f;
	const PxReal groundHeight = 5.0f;

	// create ground under character
	if(1)
	{
		PxTransform	boxPose = tr * PxTransform(PxVec3(0.0f, -groundHeight, -10.0f-groundLength)*scale, PxQuat(PxIdentity));
		boxPose.p += offset;

		const PxVec3 boxSize = PxVec3(1.0f, groundHeight, groundLength) * scale;
		PxRigidActor* box = createBox(scene, physics, boxPose, boxSize, 0.0f, material, NULL, 0);
		actors.push_back(box);
	}

	// create bridge...
	if(1)
	{
		const PxReal plankWidth		= 1.0f;
		const PxReal plankHeight	= 0.02f;
		const PxReal plankDepth		= 0.5f;
		const PxReal plankDensity	= 0.03f;
//		const PxReal plankDensity	= 0.3f;
		PxRigidDynamic* lastPlank	= 0;

		for(PxReal z = -10.0f+plankDepth; z<10.0f; z+= plankDepth*2)
		{
			const PxVec3 boxSize = PxVec3(plankWidth, plankHeight, plankDepth) * scale;

			PxTransform boxPose = tr * PxTransform(PxVec3(0,-plankHeight,z)*scale);

			PxRigidDynamic* plank = createBox(scene, physics, boxPose, boxSize, plankDensity, material, NULL, 0)->is<PxRigidDynamic>();
			if(plank)
			{
#ifdef CCT_ON_BRIDGES
				plank->setName(gPlankName);
#endif
				actors.push_back(plank);

				//plank->setSolverIterationCounts(1, 1);
//				plank->setSolverIterationCounts(255, 255);
//				plank->setSolverIterationCounts(10, 10);
				plank->setSolverIterationCounts(8, 8);
				plank->setLinearDamping(1.0f);
				plank->setAngularDamping(1.0f);
//				plank->putToSleep();

				if(!lastPlank)
				{
					// plank = first plank...
					plank->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
				}
				else
				{
					// joint together plank and lastPlank...
					if(type==BRIDGE_REVOLUTE)
					{
						PxRevoluteJoint* j = PxRevoluteJointCreate(physics, 
																   lastPlank, PxTransform(PxVec3(0,0, plankDepth)*scale),
																   plank, PxTransform(PxVec3(0,0,-plankDepth)*scale));
						if(j)
						{
							j->setProjectionLinearTolerance(0.5f);
							joints.push_back(j);
						}
					}
					else if(type==BRIDGE_FIXED)
					{
						PxFixedJoint* j = PxFixedJointCreate(physics, 
															 lastPlank, PxTransform(PxVec3(0,0, plankDepth)*scale),
															 plank, PxTransform(PxVec3(0,0,-plankDepth)*scale));
						if(j)
						{
							j->setProjectionLinearTolerance(0.5f);
							joints.push_back(j);
						}
					}
					else if(type==BRIDGE_SPHERICAL)
					{
						const PxReal plankX = 0.1f;
						PxSphericalJoint* j1 = PxSphericalJointCreate(physics, 
															          lastPlank, PxTransform(PxVec3(plankX, 0.0f, plankDepth)*scale),
															          plank, PxTransform(PxVec3(plankX, 0.0f, -plankDepth)*scale));
						if(j1)
						{
							j1->setProjectionLinearTolerance(0.5f);
							joints.push_back(j1);
						}

						PxSphericalJoint* j2 = PxSphericalJointCreate(physics, 
															          lastPlank, PxTransform(PxVec3(-plankX, 0.0f, plankDepth)*scale),
															          plank, PxTransform(PxVec3(-plankX, 0.0f, -plankDepth)*scale));
						if(j2)
						{
							j2->setProjectionLinearTolerance(0.5f);
							joints.push_back(j2);
						}
					}
					else PX_ASSERT(0);
				}
			}

			if(gBreakableBridges)
			{
				for(PxU32 i=0;i<joints.size();i++)
					joints[i]->setBreakForce(gBreakForce * scale, gBreakForce * scale);
			}

			lastPlank = plank;
		}
		if(lastPlank)
		{
			lastPlank->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
		}
	}

	// create ground after bridge
	if(1)
	{
		PxTransform	boxPose = tr * PxTransform(PxVec3(0.0f, -groundHeight, 10.0f+groundLength)*scale);
		boxPose.p += offset;

		const PxVec3 boxSize = PxVec3(1.0f, groundHeight, groundLength) * scale;
		PxRigidActor* box = createBox(scene, physics, boxPose, boxSize, 0.0f, material, NULL, 0);
		actors.push_back(box);
	}
}

///////////////////////////////////////////////////////////////////////////////

SampleBridges::SampleBridges(PhysXSampleApplication& app) :
	PhysXSample				(app),
#ifdef CCT_ON_BRIDGES
	mCCTCamera				(NULL),
	mActor					(NULL),
	mControllerManager		(NULL),
#endif
#ifdef PLATFORMS_AS_OBSTACLES
	mObstacleContext		(NULL),
	mBoxObstacles			(NULL),
#endif
	mGlobalScale			(gGlobalScale),
	mCurrentPlatform		(0xffffffff),
	mElapsedRenderTime		(0.0f),
	mRockMaterial			(NULL),
	mWoodMaterial			(NULL),
	mPlatformMaterial		(NULL),
	mBomb					(NULL),
	mBombTimer				(0.0f),
	mWaterY					(0.0f),
	mMustResync				(false),
	mEnableCCTDebugRender	(false)
{
#ifdef CCT_ON_BRIDGES
	mControllerInitialPosition = PxExtendedVec3(18.0f, 2.0f, -64.0f);
#endif
	mCreateGroundPlane	= false;
	//mStepperType = FIXED_STEPPER;
	mDefaultDensity		= 0.1f;
//	mDefaultDensity		= 1.0f;

}

SampleBridges::~SampleBridges()
{
#ifdef CCT_ON_BRIDGES
	DELETESINGLE(mActor);
#endif
}

///////////////////////////////////////////////////////////////////////////////

void SampleBridges::customizeSample(SampleSetup& setup)
{
	setup.mName	= "SampleBridges";
}

///////////////////////////////////////////////////////////////////////////////

// PT: this is called from the RAW-loader when loading a scene. The default behavior
// in PhysXSampleApplication creates a physics and a graphics mesh for each exported object. We
// intercept the call here to disable the physics mesh for the "water plane".
void SampleBridges::newMesh(const RAWMesh& data)
{
	if(data.mName && (Ps::strcmp(data.mName, "WaterPlane")==0 || Ps::strcmp(data.mName, "skyshape")==0))
	{
		// PT: create render mesh only (not physics) for the waterplane & skyboxes
		createRenderMeshFromRawMesh(data);
		if(Ps::strcmp(data.mName, "WaterPlane")==0)
			mWaterY = data.mTransform.p.y;
		return;
	}
	PhysXSample::newMesh(data);
}

// PT: this is a callback from the RAW-loader, used to import a new shape. We use
// simple segment shapes in the MAX scene to define where our bridges should be.
void SampleBridges::newShape(const RAWShape& data)
{
	if(data.mName)
	{
		if(::strcmp(data.mName, "Bridge")==0)
		{
			PX_ASSERT(data.mNbVerts==2);
			const PxVec3 p0 = data.mTransform.transform(data.mVerts[0]);
			const PxVec3 p1 = data.mTransform.transform(data.mVerts[1]);
			const PxVec3 center = (p0 + p1)*0.5f;

			PxVec3 dir, right, up;
			Ps::computeBasis(p0, p1, dir, right, up);

			PxMat33 m(right, up, dir);
			PxTransform tr;
			tr.p = center;
			tr.q = PxQuat(m);

			const float l = (p1 - p0).magnitude();
			const float coeff = l / 80.0f;

			float scale = 2.0f * coeff;
	//		float scale = 4.0f * coeff;
	//		float scale = 1.0f;

			BridgeJoint type = BRIDGE_REVOLUTE;
			{
				static int count=0;
				if(count==0)
					type = BRIDGE_REVOLUTE;
				else if(count==1)
					type = BRIDGE_SPHERICAL;
				else if(count==2)
					type = BRIDGE_FIXED;
				else PX_ASSERT(0);

				count++;
				if(count==BRIDGE_COUNT)
					count = 0;
			}

			std::vector<PxRigidActor*> bridgeActors;
			buildBridge(getActiveScene(), getPhysics(), getDefaultMaterial(), bridgeActors, mJoints, tr, scale, type);

			for(PxU32 i=0;i<bridgeActors.size();i++)
			{
				RenderMaterial* m = (i==0 || i==bridgeActors.size()-1) ? mRockMaterial : mWoodMaterial;

				PX_ASSERT(bridgeActors[i]->getNbShapes()==1);
				PxShape* boxShape;
				PxU32 nb = bridgeActors[i]->getShapes(&boxShape, 1);
				PX_ASSERT(nb==1);
				PX_UNUSED(nb);
				createRenderBoxFromShape(bridgeActors[i], boxShape, m, gBoxUVs);
			}
		}
		else if(::strcmp(data.mName, "Platform")==0)
		{
			const PxTransform idt = PxTransform(PxIdentity);

			PxRigidDynamic* platformActor = (PxRigidDynamic*)::createBox(getActiveScene(), getPhysics(), idt, gGlobalScale * PxVec3(1.0f, 5.0f, 5.0f), 1.0f, getDefaultMaterial(), NULL, 0);
#ifdef CCT_ON_BRIDGES
			platformActor->setName(gPlatformName);
#endif
			platformActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

//			createRenderObjectsFromActor(platformActor, mPlatformMaterial);
			PxShape* platformShape = NULL;
			platformActor->getShapes(&platformShape, 1);
			PX_ASSERT(platformShape);
			RenderBaseActor* renderActor = createRenderObjectFromShape(platformActor, platformShape, mPlatformMaterial);

#ifdef PLATFORMS_AS_OBSTACLES
//			platformShape->userData = NULL;
			renderActor->setPhysicsShape(NULL, NULL);
			PxBounds3 maxBounds;
			maxBounds.setMaximal();
			renderActor->setWorldBounds(maxBounds);
#else
			PX_UNUSED(renderActor);
#endif

#ifdef ROTATING_PLATFORMS
			const PxF32 rotationSpeed = 0.5f;
#else
			const PxF32 rotationSpeed = 0.0f;
#endif
			const PxVec3 up(0.0f, 1.0f, 0.0f);
			const PxQuat q = PxShortestRotation(PxVec3(1.0f, 0.0f, 0.0f), up);

			mPlatformManager.createPlatform(data.mNbVerts, data.mVerts, data.mTransform, q, platformActor, gPlatformSpeed, rotationSpeed);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void SampleBridges::onInit()
{
	PhysXSample::onInit();
	PxSceneWriteLock scopedLock(*mScene);

	mApplication.setMouseCursorHiding(true);
	mApplication.setMouseCursorRecentering(true);

	getRenderer()->setAmbientColor(RendererColor(170, 170, 170));
//	mScene->setGravity(PxVec3(0.0f, -98.1f, 0.0f));
	getActiveScene().setGravity(PxVec3(0.0f, -9.81f, 0.0f));

	{
		{
			RAWTexture data;
			data.mName = "rock_distance_diffuse.tga";
			RenderTexture* rockTexture = createRenderTextureFromRawTexture(data);

			mRockMaterial = SAMPLE_NEW(RenderMaterial)(*getRenderer(), PxVec3(1.0f, 1.0f, 1.0f), 1.0f, false, 0xffffffff, rockTexture);
			mRenderMaterials.push_back(mRockMaterial);
		}

		{
			RAWTexture data;
			data.mName = "pier_diffuse.dds";
			RenderTexture* woodTexture = createRenderTextureFromRawTexture(data);

			mWoodMaterial = SAMPLE_NEW(RenderMaterial)(*getRenderer(), PxVec3(1.0f, 1.0f, 1.0f), 1.0f, false, 0xffffffff, woodTexture);
			mRenderMaterials.push_back(mWoodMaterial);
		}

		{
			RAWTexture data;
			data.mName = "divingBoardFloor_diffuse.dds";
			RenderTexture* platformTexture = createRenderTextureFromRawTexture(data);

			mPlatformMaterial = SAMPLE_NEW(RenderMaterial)(*getRenderer(), PxVec3(1.0f, 1.0f, 1.0f), 1.0f, false, 0xffffffff, platformTexture);
			mRenderMaterials.push_back(mPlatformMaterial);
		}

		importRAWFile("SampleBridges.raw", gGlobalScale);
		importRAWFile("sky_mission_race1.raw", 1.0f);
	}

#ifdef CCT_ON_BRIDGES
	mControllerManager = PxCreateControllerManager(getActiveScene());

	ControlledActorDesc desc;
	desc.mType					= PxControllerShapeType::eCAPSULE;
	desc.mPosition				= mControllerInitialPosition;
	desc.mSlopeLimit			= SLOPE_LIMIT;
	desc.mContactOffset			= CONTACT_OFFSET;
	desc.mStepOffset			= STEP_OFFSET;
	desc.mInvisibleWallHeight	= INVISIBLE_WALLS_HEIGHT;
	desc.mMaxJumpHeight			= MAX_JUMP_HEIGHT;
	desc.mRadius				= gControllerRadius;
	desc.mHeight				= gStandingSize;
	desc.mCrouchHeight			= gCrouchingSize;
	desc.mReportCallback		= this;
	desc.mBehaviorCallback		= this;

	{
		mActor = SAMPLE_NEW(ControlledActor)(*this);
		mActor->init(desc, mControllerManager);

		RenderBaseActor* actor0 = mActor->getRenderActorStanding();
		RenderBaseActor* actor1 = mActor->getRenderActorCrouching();
		if(actor0)
			mRenderActors.push_back(actor0);
		if(actor1)
			mRenderActors.push_back(actor1);
	}

	mCCTCamera = SAMPLE_NEW(SampleCCTCameraController)(*this);
	mCCTCamera->setControlled(&mActor, 0, 1);
//	mCCTCamera->setFilterData();
	mCCTCamera->setFilterCallback(this);
	mCCTCamera->setGravity(-20.0f);

	setCameraController(mCCTCamera);

	mCCTCamera->setView(0,0);

	#ifdef PLATFORMS_AS_OBSTACLES
	mObstacleContext = mControllerManager->createObstacleContext();
	mCCTCamera->setObstacleContext(mObstacleContext);

	const PxVec3 up(0.0f, 1.0f, 0.0f);
	const PxQuat q = PxShortestRotation(PxVec3(1.0f, 0.0f, 0.0f), up);

	const PxU32 nbPlatforms = mPlatformManager.getNbPlatforms();
	KinematicPlatform** platforms = mPlatformManager.getPlatforms();
	mBoxObstacles = new PxBoxObstacle[nbPlatforms];
	for(PxU32 i=0;i<nbPlatforms;i++)
	{
		mBoxObstacles[i].mPos.x = 0.0;
		mBoxObstacles[i].mPos.y = 0.0;
		mBoxObstacles[i].mPos.z = 0.0;
		mBoxObstacles[i].mRot = q;
		mBoxObstacles[i].mHalfExtents = gGlobalScale * PxVec3(1.0f, 5.0f, 5.0f);
//		mBoxObstacles[i].mHalfExtents = gGlobalScale * PxVec3(5.0f, 1.0f, 5.0f);
		const ObstacleHandle handle = mObstacleContext->addObstacle(mBoxObstacles[i]);
		platforms[i]->setBoxObstacle(handle, mBoxObstacles + i);
	}
	#endif

//	mCCTCamera->setCameraLinkToPhysics(true);
#endif

//	mScene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_NORMAL, 1.0f);
//	mScene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES, 1.0f);

//	advanceSimulation(4.0f);
}

void SampleBridges::onShutdown()
{
	{
		PxSceneWriteLock scopedLock(*mScene);
#ifdef CCT_ON_BRIDGES
		DELETESINGLE(mCCTCamera);
		#ifdef PLATFORMS_AS_OBSTACLES
		SAFE_RELEASE(mObstacleContext);
		DELETEARRAY(mBoxObstacles);
		#endif
		mControllerManager->release();
#endif
		const size_t nbJoints = mJoints.size();
		for(PxU32 i=0;i<nbJoints;i++)
			if(mJoints[i])
				mJoints[i]->release();
		mJoints.clear();
	}
	

	mPlatformManager.release();

	PhysXSample::onShutdown();
}

// PT: very crude attempt here, just to have something running
static void applyBombForce(PxRigidActor* actor, const PxVec3& actorPos, const PxVec3& bombPos, PxReal bombRange)
{
	PxRigidDynamic* dyna = actor->is<PxRigidDynamic>();
	if(dyna)
	{
		PxVec3 dir = (actorPos - bombPos);
		const PxReal m = dir.magnitude();
		if(m<bombRange)
		{
			const PxReal coeff = (1.0f - m/bombRange) * 50.0f;
			dir /= m;
//			dyna->addForce(dir*coeff);

			PxBounds3 worldBounds = dyna->getWorldBounds();

			const PxVec3 center = worldBounds.getCenter();
			const PxVec3 extents = worldBounds.getExtents();
			const PxVec3 axis0(extents.x, 0.0f, 0.0f);
			const PxVec3 axis1(0.0f, extents.y, 0.0f);
			const PxVec3 axis2(0.0f, 0.0f, extents.z);

			PxVec3 pts[8];
			pts[0] = pts[3] = pts[4] = pts[7] = center - axis0;
			pts[1] = pts[2] = pts[5] = pts[6] = center + axis0;

			PxVec3 tmp = axis1 + axis2;
			pts[0] -= tmp;
			pts[1] -= tmp;
			pts[6] += tmp;
			pts[7] += tmp;

			tmp = axis1 - axis2;
			pts[2] += tmp;
			pts[3] += tmp;
			pts[4] -= tmp;
			pts[5] -= tmp;

			PxReal minDist = 1e+30f;
			PxU32 index = 0;
			for(PxU32 i=0;i<8;i++)
			{
				PxReal d2 = (pts[i] - bombPos).magnitudeSquared();
				if(d2<minDist)
				{
					minDist = d2;
					index = i;
				}
			}

			PxRigidBodyExt::addForceAtPos(*dyna, dir*coeff, pts[index]);
//			PxRigidBodyExt::addForceAtPos(*dyna, dir*coeff, actorPos);
		}
	}
}

template<class T>
bool findAndReplaceWithLast(std::vector<T*>& array, T* ptr)
{
	const size_t s = array.size();
	for(PxU32 j=0; j<s; j++)
	{
		if(array[j]==ptr)
		{
			T* last = array[s-1];
			array.pop_back();
			if(j!=s-1)
				array[j] = last;
			return true;
		}
	}
	return false;
}

void SampleBridges::onTickPreRender(float dtime)
{
//	shdfnd::printFormatted("SampleBridges::onTickPreRender\n");

	const bool paused = isPaused();

	// PT: keep track of elapsed render time
	if(!paused)
		mElapsedRenderTime += dtime;

#ifdef CCT_ON_BRIDGES
//	const bool postUpdate = mCCTCamera->getCameraLinkToPhysics();
	const bool postUpdate = true;
//	const bool postUpdate = false;

	if(!postUpdate && !mPause)
	{
//		shdfnd::printFormatted("CCT sync\n");
		mActor->sync();
	}
#endif

	if(!paused)
	{
//		mPlatformManager.updatePhysicsPlatforms(dtime);

		if(mBomb)
		{
			mBombTimer -= dtime;
			if(mBombTimer<0.0f)
			{
				PxSceneWriteLock scopedLock(*mScene);
				mBombTimer = 0.0f;

				{
					const PxVec3 bombPos = mBomb->getGlobalPose().p;
					const PxReal bombActorRange = 30.0f;
					const PxReal bombJointRange = 10.0f;
					size_t nbJoints = mJoints.size();
					for(PxU32 i=0;i<nbJoints;i++)
					{
						PxJoint* j = mJoints[i];
						if(!j)
							continue;

						PxRigidActor* actor0;
						PxRigidActor* actor1;
						j->getActors(actor0, actor1);
						const PxVec3 actorPos0 = actor0->getGlobalPose().p;
						const PxVec3 actorPos1 = actor1->getGlobalPose().p;

						applyBombForce(actor0, actorPos0, bombPos, bombActorRange);
						applyBombForce(actor1, actorPos1, bombPos, bombActorRange);

						const PxVec3 jointPos = (actorPos0 + actorPos1)*0.5f;
						const PxReal d2 = (bombPos - jointPos).magnitudeSquared();
						if(d2<bombJointRange*bombJointRange)
						{
							j->release();
							mJoints[i] = NULL;
						}
					}
				}

				{
					PxU32 nbShapes = mBomb->getNbShapes();
					PxShape** shapes = (PxShape**)SAMPLE_ALLOC(sizeof(PxShape*)*nbShapes);
					PxU32 nb = mBomb->getShapes(shapes, nbShapes);
					PX_ASSERT(nb==nbShapes);
					PX_UNUSED(nb);
					for(PxU32 i=0;i<nbShapes;i++)
					{
						PxShape* currentShape = shapes[i];

						RenderBaseActor* renderActor = getRenderActor(mBomb, currentShape);
						unlink(renderActor, currentShape, mBomb);

						findAndReplaceWithLast(mRenderActors, renderActor);

						renderActor->release();
					}
					SAMPLE_FREE(shapes);
				}

				findAndReplaceWithLast(mPhysicsActors, mBomb);

				SAFE_RELEASE(mBomb);
			}
		}
	}

	PhysXSample::onTickPreRender(dtime);

#ifdef PLATFORMS_AS_OBSTACLES
	if(!paused)
		updateRenderPlatforms(dtime);
#endif

#ifdef CCT_ON_BRIDGES
	if(postUpdate && !mPause)
	{
//		shdfnd::printFormatted("CCT sync\n");
		mActor->sync();
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////

void SampleBridges::onTickPostRender(float dtime)
{
	PhysXSample::onTickPostRender(dtime);

	// PT: add CCT's internal debug rendering
	if(mEnableCCTDebugRender)
	{
		mControllerManager->setDebugRenderingFlags(PxControllerDebugRenderFlag::eALL);
		PxRenderBuffer& renderBuffer = mControllerManager->getRenderBuffer();
		RenderPhysX3Debug* renderer = getDebugRenderer();
		renderer->update(renderBuffer);
		renderBuffer.clear();
	}
	else
	{
		mControllerManager->setDebugRenderingFlags(PxControllerDebugRenderFlag::eNONE);
	}

#ifdef CCT_ON_BRIDGES
//	mControllerManager->setDebugRenderingFlags(PxControllerDebugRenderFlag::eCACHED_BV|PxControllerDebugRenderFlag::eTEMPORAL_BV);
	PxRenderBuffer& renderBuffer = mControllerManager->getRenderBuffer();
	getDebugRenderer()->update(renderBuffer);
	renderBuffer.clear();
#endif

	const float lag = fabsf(mPlatformManager.getElapsedTime() - mElapsedRenderTime);
	mMustResync = lag > 1.0f/60.0f;

	if(0)
	{
		shdfnd::printFormatted("Render time:   %f\n", mElapsedRenderTime);
		shdfnd::printFormatted("Platform time: %f\n", mPlatformManager.getElapsedTime());
		shdfnd::printFormatted("Delta:         %f\n", lag);
	}
}

///////////////////////////////////////////////////////////////////////////////

#ifdef PLATFORMS_AS_OBSTACLES
void SampleBridges::updateRenderPlatforms(float dtime)
{
	// PT: compute new positions for (render) platforms, then move their corresponding obstacles to these positions.
	const PxU32 nbPlatforms = mPlatformManager.getNbPlatforms();
	KinematicPlatform** platforms = mPlatformManager.getPlatforms();

	{
		PxSceneReadLock scopedLock(*mScene);

		for(PxU32 i=0;i<nbPlatforms;i++)
		{
			if(1)
			{
				PxRigidDynamic* actor = platforms[i]->getPhysicsActor();
				PxShape* shape = NULL;
				actor->getShapes(&shape, 1);
				RenderBaseActor* renderActor = getRenderActor(actor, shape);
				renderActor->setTransform(platforms[i]->getRenderPose());
			}

			platforms[i]->updateRender(dtime, mObstacleContext);
		}
	}

	// PT: if render time & physics time varies too much, we must resync the physics to avoid large visual mismatchs
	if(mMustResync)
	{
		PxSceneWriteLock scopedLock(*mScene);

//		mElapsedPlatformTime = mElapsedRenderTime;
		mPlatformManager.syncElapsedTime(mElapsedRenderTime);
//		static int count=0;		shdfnd::printFormatted("Resync %d\n", count++);
		for(PxU32 i=0;i<nbPlatforms;i++)
		{
			PxRigidDynamic* actor = platforms[i]->getPhysicsActor();
			actor->setKinematicTarget(platforms[i]->getRenderPose());
			platforms[i]->resync();
		}
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////

void SampleBridges::onSubstep(float dtime)
{
//	shdfnd::printFormatted("onSubstep\n");
	mPlatformManager.updatePhysicsPlatforms(dtime);
}

///////////////////////////////////////////////////////////////////////////////

void SampleBridges::helpRender(PxU32 x, PxU32 y, PxU8 textAlpha)
{
	Renderer* renderer = getRenderer();
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
	msg = mApplication.inputInfoMsg("Press ","  to crouch", CAMERA_CROUCH,-1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);	;
	msg = mApplication.inputInfoMsg("Press "," to throw an object", SPAWN_DEBUG_OBJECT, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);	;
	msg = mApplication.inputInfoMsg("Press ","  to reset CCT position", RETRY,-1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press ","  to teleport from one platform to another", TELEPORT,-1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
}

void SampleBridges::descriptionRender(PxU32 x, PxU32 y, PxU8 textAlpha)
{
	bool print=(textAlpha!=0.0f);

	if(print)
	{
		Renderer* renderer = getRenderer();
		const PxU32 yInc = 24;
		const PxReal scale = 0.5f;
		const PxReal shadowOffset = 6.0f;
		const RendererColor textColor(255, 255, 255, textAlpha);

		char line0[256]="This sample demonstrates the configuration and run-time control of the";
		char line1[256]="PhysX kinematic character controller (cct).  In particular, the";
		char line2[256]="interaction between the cct and both kinematic and dynamic scene objects";
		char line3[256]="is presented. Jointed bridges with a variety of configurations are also";
		char line4[256]="added to the scene to illustrate the setup code and behavior of some of";
		char line5[256]="the different joint types supported by the PhysX SDK.";

		renderer->print(x, y+=yInc, line0, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line1, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line2, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line3, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line4, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line5, scale, shadowOffset, textColor);
	}
}

///////////////////////////////////////////////////////////////////////////////

void SampleBridges::customizeRender()
{
	// PT: add a simple screenquad when underwater 
	if(getCamera().getPos().y<mWaterY)
	{
		const RendererColor color(0, 90, 90);
		ScreenQuad sq;
		sq.mLeftUpColor		= color;
		sq.mRightUpColor	= color;
		sq.mLeftDownColor	= color;
		sq.mRightDownColor	= color;
		sq.mAlpha			= 0.75;

		getRenderer()->drawScreenQuad(sq);
	}
}

