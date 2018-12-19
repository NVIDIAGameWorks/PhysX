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


#include "ApexController.h"
#include "ApexRenderResourceManager.h"
#include "ApexResourceCallback.h"
#include "ApexRenderer.h"
#include "ApexRenderMaterial.h"
#include "PhysXPrimitive.h"
#include "legacy/ModuleIofxLegacy.h"

#include "XInput.h"
#include "DXUTMisc.h"
#include "DXUTCamera.h"

#define PVD_TO_FILE 0

#define USE_GPU_RB_SIMULATION 1

const DirectX::XMFLOAT3 PLANE_COLOR(0.25f, 0.25f, 0.25f); 

ApexController::ApexController(PxSimulationFilterShader filterShader, CFirstPersonCamera* camera)
: mCamera(camera)
, mApexRenderDebug(NULL)
, mFilterShader(filterShader)
, mLastSimulationTime(0)
, mPaused(false)
, mUseFixedTimestep(true)
, mFixedTimestep(1.0/60.0)
, mTimeRemainder(0.0)
, mModuleDestructible(NULL)
, mModuleParticles(NULL)
, mModuleIofx(NULL)
, mModuleTurbulenceFS(NULL)
, mModuleLegacy(NULL)
, mModuleClothing(NULL)
, mRenderVolume(NULL)
{
	QueryPerformanceFrequency(&mPerformanceFreq);
}

ApexController::~ApexController()
{
}

void ApexController::onInitialize()
{
	initPhysX();
	initApex();
	initPhysXPrimitives();
}

void ApexController::onTerminate()
{
	releaseApex();
	releasePhysX();
	releasePhysXPrimitives();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												PhysX init/release
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



void ApexController::initPhysX()
{
	mFoundation = PxCreateFoundation(PX_FOUNDATION_VERSION, mAllocator, mErrorCallback);

	mPvd = PxCreatePvd(*mFoundation);

	PxTolerancesScale scale;

	mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, scale, true, mPvd);

	PxCookingParams cookingParams(scale);
#if USE_GPU_RB_SIMULATION
	cookingParams.buildGPUData = true;
#endif
	mCooking = PxCreateCooking(PX_PHYSICS_VERSION, mPhysics->getFoundation(), cookingParams);

	PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	mDispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = mDispatcher;
	sceneDesc.filterShader = mFilterShader;
	sceneDesc.flags |= PxSceneFlag::eENABLE_STABILIZATION;
	sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
	
#if APEX_CUDA_SUPPORT
	PxCudaContextManagerDesc ctxMgrDesc;
	mCudaContext = CreateCudaContextManager(ctxMgrDesc, mErrorCallback);
	if (mCudaContext && !mCudaContext->contextIsValid())
	{
		mCudaContext->release();
		mCudaContext = NULL;
	}
	sceneDesc.gpuDispatcher = mCudaContext != NULL ? mCudaContext->getGpuDispatcher() : NULL;
#if USE_GPU_RB_SIMULATION
	sceneDesc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
	sceneDesc.broadPhaseType = PxBroadPhaseType::eGPU;
#endif
#endif
	mPhysicsScene = mPhysics->createScene(sceneDesc);

	mDefaultMaterial = mPhysics->createMaterial(0.8f, 0.7f, 0.1f);

	PxPvdSceneClient* pvdClient = mPhysicsScene->getScenePvdClient();
	if(pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	mPhysicsScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1);
}

void ApexController::releasePhysX()
{
	mDefaultMaterial->release();
	mPhysicsScene->release();
#if APEX_CUDA_SUPPORT
	if (mCudaContext)
		mCudaContext->release();
#endif
	mDispatcher->release();
	mPhysics->release();
	PxPvdTransport* transport = mPvd->getTransport();
	mPvd->release();
	transport->release();
	mCooking->release();
	mFoundation->release();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												APEX SDK init/release
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ApexController::initApex()
{
	// Fill out the Apex SDKriptor
	ApexSDKDesc apexDesc;

	// Apex needs an allocator and error stream.  By default it uses those of the PhysX SDK.

	// Let Apex know about our PhysX SDK and cooking library
	apexDesc.physXSDK = mPhysics;
	apexDesc.cooking = mCooking;
	apexDesc.pvd = mPvd;

	// Our custom dummy render resource manager
	mApexRenderResourceManager = new ApexRenderResourceManager(GetDeviceManager()->GetDevice());
	apexDesc.renderResourceManager = mApexRenderResourceManager;

	// Our custom named resource handler
	mApexResourceCallback = new ApexResourceCallback();
	apexDesc.resourceCallback = mApexResourceCallback;
	apexDesc.foundation = mFoundation;

	// Finally, create the Apex SDK
	ApexCreateError error;
	mApexSDK = CreateApexSDK(apexDesc, &error);
	PX_ASSERT(mApexSDK);

	// Initialize destructible module
	mModuleDestructible = static_cast<ModuleDestructible*>(mApexSDK->createModule("Destructible"));
	PX_ASSERT(mModuleDestructible);
	NvParameterized::Interface* params = mModuleDestructible->getDefaultModuleDesc();
	mModuleDestructible->init(*params);

	// Initialize particles module
	mModuleParticles = static_cast<ModuleParticles*>(mApexSDK->createModule("Particles"));
	if (mModuleParticles != NULL)
	{
		mModuleParticles->init(*mModuleParticles->getDefaultModuleDesc());
		mModuleTurbulenceFS = mApexSDK->createModule("TurbulenceFS");
		mModuleIofx = static_cast<ModuleIofx*>(mModuleParticles->getModule("IOFX"));
	}

	mModuleLegacy = mApexSDK->createModule("Legacy");

	// Initialize clothing module
	mModuleClothing = static_cast<ModuleClothing*>(mApexSDK->createModule("Clothing"));
	PX_ASSERT(mModuleClothing);
	mModuleClothing->init(*mModuleClothing->getDefaultModuleDesc());

	mApexResourceCallback->setApexSDK(mApexSDK);
	mApexResourceCallback->setModuleParticles(mModuleParticles);

	// render debug
	PX_ASSERT(mApexRenderDebug == NULL);
	mApexRenderDebug = mApexSDK->createApexRenderDebug(mRenderDebugInterface, false);

	// Create Apex scene
	SceneDesc sceneDesc;
	sceneDesc.scene = mPhysicsScene;
	sceneDesc.debugInterface = mApexRenderDebug;
	mApexScene = mApexSDK->createScene(sceneDesc);
	mApexScene->allocViewMatrix(ViewMatrixType::LOOK_AT_LH);
	mApexScene->allocProjMatrix(ProjMatrixType::USER_CUSTOMIZED);

	// IofxLegacy render callback
	if (mModuleIofx)
	{
		ModuleIofxLegacy* iofxLegacyModule = static_cast<ModuleIofxLegacy*>(mApexSDK->createModule("IOFX_Legacy"));
		if (iofxLegacyModule)
		{
			IofxRenderCallback* iofxLegacyRCB = iofxLegacyModule->getIofxLegacyRenderCallback(*mApexScene);
			mModuleIofx->setIofxRenderCallback(*mApexScene, iofxLegacyRCB);
		}

		// render volume
		nvidia::PxBounds3 infBounds;
		infBounds.setMaximal();
		mRenderVolume = mModuleIofx->createRenderVolume(*mApexScene, infBounds, 0, true);
	}

	// connect pvd
#if PVD_TO_FILE
	PxPvdTransport* transport = PxDefaultPvdFileTransportCreate("d:\\snippet.pxd2");
#else
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
#endif
	if (transport)
		mPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);
}

void ApexController::releaseApex()
{
	if (mRenderVolume)
	{
		mModuleIofx->releaseRenderVolume(*mRenderVolume);
		mRenderVolume = NULL;
	}

	for (std::map<DestructibleActor*, DestructibleRenderable*>::iterator it = mDestructibleActors.begin(); it != mDestructibleActors.end();
	    it++)
	{
		(*it).first->release();
		(*it).second->release();
	}
	mDestructibleActors.clear();

	for (std::set<ClothingActor*>::iterator it = mClothingActors.begin(); it != mClothingActors.end();
		it++)
	{
		(*it)->release();
	}
	mClothingActors.clear();


	for (std::set<EffectPackageActor*>::iterator it = mEffectPackageActors.begin(); it != mEffectPackageActors.end(); it++)
	{
		(*it)->release();
	}
	mEffectPackageActors.clear();

	for(std::set<Asset*>::iterator it = mAssets.begin(); it != mAssets.end(); it++)
	{
		(*it)->release();
	}
	mAssets.clear();

	if (mApexRenderDebug)
	{
		mApexSDK->releaseApexRenderDebug(*mApexRenderDebug);
	}

	mApexScene->release();
	mApexSDK->release();

	delete mApexRenderResourceManager;
	delete mApexResourceCallback;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Controller events
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ApexController::Animate(double dt)
{
	if (mPaused)
		return;

	if (mFixedTimestep)
	{
		mTimeRemainder += dt;
		if (mTimeRemainder <= 0.0)
		{
			return;
		}
		mTimeRemainder -= mFixedTimestep;
		if (mTimeRemainder > 0.0)
		{
			// slower physics if fps is below 1/mFixedTimestep
			mTimeRemainder = 0.0;
		}
		dt = mFixedTimestep;
	}
	else
	{
		// slower physics if fps is too low
		dt = PxClamp(dt, 0.001, 0.33);
	}

	LARGE_INTEGER time0;
	QueryPerformanceCounter(&time0);


	mApexScene->setViewMatrix(XMMATRIXToPxMat44(mCamera->GetViewMatrix()));
	mApexScene->setProjMatrix(XMMATRIXToPxMat44(mCamera->GetProjMatrix()));
	mApexScene->simulate(dt);
	PxU32 errorState;
	mApexScene->fetchResults(true, &errorState);

	LARGE_INTEGER time1;
	QueryPerformanceCounter(&time1);
	mLastSimulationTime = static_cast<float>(time1.QuadPart - time0.QuadPart) / static_cast<float>(mPerformanceFreq.QuadPart);

	if (mApexRenderDebug != NULL)
	{
		const physx::PxRenderBuffer & renderBuffer = mPhysicsScene->getRenderBuffer();
		mApexRenderDebug->addDebugRenderable(renderBuffer);
	}

	if (mModuleIofx)
	{
		mModuleIofx->prepareRenderables(*mApexScene);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//										   Destructible actor spawn/remove
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DestructibleActor* ApexController::spawnDestructibleActor(const char* assetPath)
{
	Asset* asset = static_cast<Asset*>(
	    mApexSDK->getNamedResourceProvider()->getResource(DESTRUCTIBLE_AUTHORING_TYPE_NAME, assetPath));

	DestructibleAsset* destructibleAsset = static_cast<DestructibleAsset*>(asset);

	NvParameterized::Interface* actorDesc = destructibleAsset->getDefaultActorDesc();

	nvidia::PxMat44 globalPose = nvidia::PxMat44(nvidia::PxIdentity);
	PxBounds3 b = destructibleAsset->getRenderMeshAsset()->getBounds();
	globalPose.setPosition(PxVec3(0, -destructibleAsset->getRenderMeshAsset()->getBounds().minimum.z, 0));
	globalPose.column1 = nvidia::PxVec4(0.0f, 0.0f, -1.0f, 0.0f);
	globalPose.column2 = nvidia::PxVec4(0.0f, 1.0f, 0.0f, 0.0f);

	NvParameterized::setParamTransform(*actorDesc, "globalPose", nvidia::PxTransform(globalPose));

	NvParameterized::setParamBool(*actorDesc, "dynamic", false);
	NvParameterized::setParamBool(*actorDesc, "formExtendedStructures", true);
	NvParameterized::setParamF32(*actorDesc, "supportStrength", 1.9f);

	NvParameterized::setParamF32(*actorDesc, "defaultBehaviorGroup.damageThreshold", 5.0f);
	NvParameterized::setParamI32(*actorDesc, "destructibleParameters.impactDamageDefaultDepth", 2);
	NvParameterized::setParamBool(*actorDesc, "structureSettings.useStressSolver", true);

	DestructibleActor* actor =
	    static_cast<DestructibleActor*>(destructibleAsset->createApexActor(*actorDesc, *mApexScene));

	while (asset->forceLoadAssets() > 0) {}

	mDestructibleActors[actor] = actor->acquireRenderableReference();
	mAssets.emplace(destructibleAsset);

	return actor;
}

void ApexController::removeActor(DestructibleActor* actor)
{
	if (mDestructibleActors.find(actor) == mDestructibleActors.end())
		return;

	mDestructibleActors.at(actor)->release();
	actor->release();
	mDestructibleActors.erase(actor);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											Clothing actor spawn/remove
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ClothingActor* ApexController::spawnClothingActor(const char* assetPath)
{
	Asset* asset = static_cast<Asset*>(
		mApexSDK->getNamedResourceProvider()->getResource(CLOTHING_AUTHORING_TYPE_NAME, assetPath));

	ClothingAsset* clothingAsset = static_cast<ClothingAsset*>(asset);

	NvParameterized::Interface* actorDesc = clothingAsset->getDefaultActorDesc();

	//NvParameterized::setParamF32(*actorDesc, "actorScale", 0.1f);

	ClothingActor* actor =
		static_cast<ClothingActor*>(clothingAsset->createApexActor(*actorDesc, *mApexScene));

	while (asset->forceLoadAssets() > 0) {}

	mClothingActors.emplace(actor);
	mAssets.emplace(clothingAsset);

	return actor;
}

void ApexController::removeActor(ClothingActor* actor)
{
	if (mClothingActors.find(actor) == mClothingActors.end())
		return;

	actor->release();
	mClothingActors.erase(actor);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//										Effect package actor spawn/remove
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


EffectPackageActor* ApexController::spawnEffectPackageActor(const char* assetPath)
{

	PxTransform globalPose = PxTransform(nvidia::PxIdentity);
	globalPose.p = PxVec3(0, 10, 0);

	nvidia::apex::Asset *asset = (nvidia::apex::Asset *)mApexSDK->getNamedResourceProvider()->getResource(PARTICLES_EFFECT_PACKAGE_AUTHORING_TYPE_NAME, assetPath);
	NvParameterized::Interface *defaultActorDesc = asset->getDefaultActorDesc();
	NvParameterized::setParamTransform(*defaultActorDesc, "InitialPose", globalPose);
	nvidia::apex::Actor *effectPackageActor = asset->createApexActor(*defaultActorDesc, *mApexScene);
	EffectPackageActor* actor = static_cast<EffectPackageActor *>(effectPackageActor);

	while (asset->forceLoadAssets() > 0) {}

	mEffectPackageActors.emplace(actor);
	mAssets.emplace(asset);

	return actor;
}

void ApexController::removeActor(EffectPackageActor* actor)
{
	if (mEffectPackageActors.find(actor) == mEffectPackageActors.end())
		return;

	actor->release();
	mEffectPackageActors.erase(actor);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												  PhysX Primitive
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ApexController::initPhysXPrimitives()
{
	// create plane
	PhysXPrimitive* plane = spawnPhysXPrimitivePlane(nvidia::PxPlane(PxVec3(0, 1, 0).getNormalized(), 0));
	plane->setColor(PLANE_COLOR);
}

void ApexController::releasePhysXPrimitives()
{
	for(std::set<PhysXPrimitive*>::iterator it = mPrimitives.begin(); it != mPrimitives.end(); it++)
	{
		delete (*it);
	}
	mPrimitives.clear();
}

PhysXPrimitive* ApexController::spawnPhysXPrimitiveBox(const nvidia::PxTransform& position, float density)
{
	PxBoxGeometry geom = PxBoxGeometry(PxVec3(1, 1, 1));
	PxRigidDynamic* actor = PxCreateDynamic(*mPhysics, position, geom, *mDefaultMaterial, density);

	return spawnPhysXPrimitive(actor, PxVec3(1, 1, 1));
}

PhysXPrimitive* ApexController::spawnPhysXPrimitivePlane(const nvidia::PxPlane& plane)
{
	PxRigidStatic* actor = PxCreatePlane(*mPhysics, plane, *mDefaultMaterial);
	return spawnPhysXPrimitive(actor, PxVec3(500));
}

PhysXPrimitive* ApexController::spawnPhysXPrimitive(PxRigidActor* actor, PxVec3 scale)
{
	mPhysicsScene->addActor(*actor);

	PhysXPrimitive* p = new PhysXPrimitive(actor, scale);

	mPrimitives.emplace(p);

	return p;
}

void ApexController::removePhysXPrimitive(PhysXPrimitive* primitive)
{
	if(mPrimitives.find(primitive) == mPrimitives.end())
		return;

	mPrimitives.erase(primitive);
	delete primitive;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Render
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ApexController::renderOpaque(ApexRenderer* renderer)
{
	for (std::map<DestructibleActor*, DestructibleRenderable*>::iterator it = mDestructibleActors.begin(); it != mDestructibleActors.end();
		it++)
	{
		(*it).first->lockRenderResources();
		(*it).first->updateRenderResources(false);
		(*it).first->dispatchRenderResources(*renderer);
		(*it).first->unlockRenderResources();

		(*it).second->lockRenderResources();
		(*it).second->updateRenderResources(false);
		(*it).second->dispatchRenderResources(*renderer);
		(*it).second->unlockRenderResources();
	}

	for (std::set<ClothingActor*>::iterator it = mClothingActors.begin(); it != mClothingActors.end();
		it++)
	{
		(*it)->lockRenderResources();
		(*it)->updateRenderResources(false);
		(*it)->dispatchRenderResources(*renderer);
		(*it)->unlockRenderResources();
	}

	mApexScene->lockRenderResources();
	mApexScene->updateRenderResources(false);
	mApexScene->dispatchRenderResources(*renderer);
	mApexScene->unlockRenderResources();

	renderParticles(renderer, IofxRenderable::MESH);

	for (std::set<PhysXPrimitive*>::iterator it = mPrimitives.begin(); it != mPrimitives.end(); it++)
	{
		renderer->renderResource(*it);
	}

	mApexRenderDebug->lockRenderResources();
	mApexRenderDebug->updateRenderResources(0);
	mApexRenderDebug->dispatchRenderResources(*renderer);
	mApexRenderDebug->unlockRenderResources();
}

void ApexController::renderTransparency(ApexRenderer* renderer)
{
	renderParticles(renderer, IofxRenderable::SPRITE);
}

void ApexController::renderParticles(ApexRenderer* renderer, IofxRenderable::Type type)
{
	if (mModuleParticles == NULL)
	{
		return;
	}
	uint32_t numActors;
	IofxActor* const* actors = mRenderVolume->lockIofxActorList(numActors);
	for (uint32_t j = 0; j < numActors; j++)
	{
		IofxRenderable* iofxRenderable = actors[j]->acquireRenderableReference();
		if (iofxRenderable)
		{
			if (iofxRenderable->getType() == type)
			{
				Renderable* apexRenderable = static_cast<Renderable*>(iofxRenderable->userData);
				if (apexRenderable && !apexRenderable->getBounds().isEmpty())
				{
					apexRenderable->lockRenderResources();
					apexRenderable->updateRenderResources(false);
					apexRenderable->dispatchRenderResources(*renderer);
					apexRenderable->unlockRenderResources();
				}
			}
			iofxRenderable->release();
		}
	}
	mRenderVolume->unlockIofxActorList();
}


PxVec3 unproject(PxMat44& proj, PxMat44& view, float x, float y)
{
	PxVec4 screenPoint(x, y, 0, 1);
	PxVec4 viewPoint = PxVec4(x / proj[0][0], y / proj[1][1], 1, 1);
	PxVec4 nearPoint = view.inverseRT().transform(viewPoint);
	if (nearPoint.w)
		nearPoint *= 1.0f / nearPoint.w;
	return PxVec3(nearPoint.x, nearPoint.y, nearPoint.z);
}


void ApexController::getEyePoseAndPickDir(float mouseX, float mouseY, PxVec3& eyePos, PxVec3& pickDir)
{
	PxMat44 view = XMMATRIXToPxMat44(mCamera->GetViewMatrix());
	PxMat44 proj = XMMATRIXToPxMat44(mCamera->GetProjMatrix());

	PxMat44 eyeTransform = view.inverseRT();
	eyePos = eyeTransform.getPosition();
	PxVec3 nearPos = unproject(proj, view, mouseX * 2 - 1, 1 - mouseY * 2);
	pickDir = nearPos - eyePos;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
