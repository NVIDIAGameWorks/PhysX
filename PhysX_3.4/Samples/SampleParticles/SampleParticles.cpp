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

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "extensions/PxExtensionsAPI.h"
#include "SampleParticles.h"
#include "SampleMaterialAsset.h"
#include "SampleTextureAsset.h"
#include "SampleAllocatorSDKClasses.h"

#include "RenderTexture.h"
#include "RenderMaterial.h"
#include "RenderMeshActor.h"
#include "RenderBoxActor.h"
#include "RenderCapsuleActor.h"
#include "RenderParticleSystemActor.h"
#include "RendererMemoryMacros.h"
#include "RendererTexture2D.h"
#include "RenderPhysX3Debug.h"

#include "PxTkStream.h"
#include <PsString.h>
#include <PsThread.h>

#include "ParticleEmitterRate.h"
#include "ParticleEmitterPressure.h"
#include "SampleParticlesInputEventIds.h"
#include <SamplePlatform.h>
#include <SampleUserInput.h>
#include <SampleUserInputIds.h>
#include <SampleUserInputDefines.h>

using namespace PxToolkit;
using namespace SampleRenderer;
using namespace SampleFramework;

REGISTER_SAMPLE(SampleParticles, "SampleParticles")

static const char* smoke_material_path = "materials/particle_smoke.xml";
static const char* waterfall_material_path = "materials/particle_water.xml";
static const char* particle_hf_material_path = "materials/particle_sample_heightmap.xml";

static const char* debris_texture = "debris_texture.dds";
static const char* terrain_hf = "particles_heightmap.tga";

static const float RAYGUN_FORCE_GROW_TIME = 2.0f;
static const float RAYGUN_FORCE_SIZE_MIN = 0.1f;
static const float RAYGUN_FORCE_SIZE_MAX = 3.0f;
static const float RAYGUN_RB_DEBRIS_RATE = 0.2f;
static const float RAYGUN_RB_DEBRIS_LIFETIME = 3.0f;
static const float RAYGUN_RB_DEBRIS_ANGLE_RANDOMNESS = 0.2f;
static const float RAYGUN_RB_DEBRIS_SCALE = 0.3f;

static const PxFilterData collisionGroupDrain		(0, 0, 0, 1);
static const PxFilterData collisionGroupForceSmoke	(0, 0, 0, 2);
static const PxFilterData collisionGroupForceWater	(0, 0, 0, 3);
static const PxFilterData collisionGroupWaterfall	(0, 0, 1, 0);
static const PxFilterData collisionGroupSmoke		(0, 0, 1, 1);
static const PxFilterData collisionGroupDebris		(0, 0, 1, 2);

static bool isParticleGroup(const PxFilterData& f)
{
	return f.word2 == 1;
}

static bool isDrainGroup(const PxFilterData& f)
{
	return !isParticleGroup(f) && f.word3 == 1;
}

static bool isForce0Group(const PxFilterData& f)
{
	return !isParticleGroup(f) && f.word3 == 2;
}

static bool isForce1Group(const PxFilterData& f)
{
	return !isParticleGroup(f) && f.word3 == 3;
}

PxFilterFlags SampleParticlesFilterShader(	PxFilterObjectAttributes attributes0, PxFilterData filterData0, 
	PxFilterObjectAttributes attributes1, PxFilterData filterData1,
	PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	// generate contacts for all that were not filtered above
	pairFlags = PxPairFlag::eCONTACT_DEFAULT;

	// trigger the contact callback for pairs (A,B) where 
	// the filtermask of A contains the ID of B and vice versa.
	if((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1)) 
	{
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
	}

	// allows only particle <-> drain collision (well, to drain particles)
	// all other collision with the drain ignored
	if(isDrainGroup(filterData0) || isDrainGroup(filterData1)) 
	{
		PxFilterData filterDataOther = isDrainGroup(filterData0) ? filterData1 : filterData0;
		if(!isParticleGroup(filterDataOther))
			return PxFilterFlag::eKILL;
	}

	// force0 group just collides with smoke
	if(isForce0Group(filterData0) || isForce0Group(filterData1)) 
	{
		PxFilterData filterDataOther = isForce0Group(filterData0) ? filterData1 : filterData0;
		if(filterDataOther != collisionGroupSmoke) 
			return PxFilterFlag::eKILL;
	}

	// force1 group just collides with waterfall
	if(isForce1Group(filterData0) || isForce1Group(filterData1)) 
	{
		PxFilterData filterDataOther = isForce1Group(filterData0) ? filterData1 : filterData0;
		if(filterDataOther != collisionGroupWaterfall) 
			return PxFilterFlag::eKILL;
	}

	return PxFilterFlags();
}

enum SampleParticlesMaterials 
{
	MATERIAL_SMOKE = 100,
	MATERIAL_WATERFALL,
	MATERIAL_HEIGHTFIELD,
	MATERIAL_LAKE,
	MATERIAL_DEBRIS,
	MATERIAL_RAY,
};

static PxQuat directionToQuaternion(const PxVec3& direction) 
{
    PxVec3 vUp(0.0f, 1.0f, 0.0f);
    PxVec3 vRight = vUp.cross(direction);
    vUp = direction.cross(vRight);

	PxQuat qrot(PxMat33(vRight, vUp, direction));
	qrot.normalize();

    return qrot;
}

SampleParticles::SampleParticles(PhysXSampleApplication& app) : 
	PhysXSample(app), mWaterfallParticleSystem(NULL), mSmokeParticleSystem(NULL), mDebrisParticleSystem(NULL)
{
}

SampleParticles::~SampleParticles() 
{
}

void SampleParticles::onTickPreRender(float dtime) 
{
	if (mPause)
		mRaygun.setEnabled(false);

	mRaygun.update(dtime);

	// start the simulation
	PhysXSample::onTickPreRender(dtime);
	
	for(size_t i = 0; i < mRenderMaterials.size(); ++i)
		mRenderMaterials[i]->update(*getRenderer());
}

void SampleParticles::onSubstepSetup(float dtime, PxBaseTask* cont)
{
	PxSceneWriteLock scopedLock(*mScene);

	for (PxU32 i = 0; i < mEmitters.size(); ++i)
		mEmitters[i].update(dtime);
		
	mRaygun.onSubstep(dtime);
	
	if (mWaterfallParticleSystem)
		mWaterfallParticleSystem->updateSubstep(dtime);
	
	if (mSmokeParticleSystem)
		mSmokeParticleSystem->updateSubstep(dtime);
	
	if (mDebrisParticleSystem)
		mDebrisParticleSystem->updateSubstep(dtime);
}

void SampleParticles::helpRender(PxU32 x, PxU32 y, PxU8 textAlpha)
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
		renderer->print(x, y += yInc, "Use mouse or right stick to rotate", scale, shadowOffset, textColor);
	else if (isMouseSupported)
		renderer->print(x, y += yInc, "Use mouse to rotate", scale, shadowOffset, textColor);
	else if (isPadSupported)
		renderer->print(x, y += yInc, "Use right stick to rotate", scale, shadowOffset, textColor);
	if (isPadSupported)
		renderer->print(x, y += yInc, "Use left stick to move",scale, shadowOffset, textColor);
	msg = mApplication.inputMoveInfoMsg("Press "," to move", CAMERA_MOVE_FORWARD,CAMERA_MOVE_BACKWARD, CAMERA_MOVE_LEFT, CAMERA_MOVE_RIGHT);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press "," to move fast", CAMERA_SHIFT_SPEED, -1);
	if(msg)
		renderer->print(x, y += yInc, msg, scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press "," to shoot raygun", RAYGUN_SHOT, -1);
	if(msg)	
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press "," to toggle various debug visualization", TOGGLE_VISUALIZATION, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press "," to change particle count", CHANGE_PARTICLE_CONTENT,-1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);

#if PX_SUPPORT_GPU_PHYSX
	msg = mApplication.inputInfoMsg("Press "," to toggle CPU/GPU", TOGGLE_CPU_GPU, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
#endif

}

void SampleParticles::descriptionRender(PxU32 x, PxU32 y, PxU8 textAlpha)
{
	bool print=(textAlpha!=0.0f);

	if(print)
	{
		Renderer* renderer = getRenderer();
		const PxU32 yInc = 24;
		const PxReal scale = 0.5f;
		const PxReal shadowOffset = 6.0f;
		const RendererColor textColor(255, 255, 255, textAlpha);

		char line0[256]="This sample demonstrates the creation of particle systems and particle";
		char line1[256]="fluids.  The waterfall provides an example of a self-interacting particle";
		char line2[256]="fluid, while the volcano debris and smoke are examples of particle systems";
		char line3[256]="without self-interaction.  The interaction between dynamic physics objects";
		char line4[256]="and particles is also introduced:  the raygun is modelled as a kinematic";
		char line5[256]="capsule set up to deflect smoke and waterfall particles.";
#if PX_WINDOWS
		char line6[256]="Finally, the sample illustrates the steps required to enable GPU";
		char line7[256]="acceleration of PhysX particles.";
#endif

		renderer->print(x, y+=yInc, line0, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line1, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line2, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line3, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line4, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line5, scale, shadowOffset, textColor);
#if PX_WINDOWS
		renderer->print(x, y+=yInc, line6, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line7, scale, shadowOffset, textColor);
#endif
	}
}


void SampleParticles::customizeRender()
{
	SampleRenderer::Renderer* renderer = getRenderer();

	// render the sight
	SampleRenderer::RendererColor sightColor(200, 40, 40, 127);
	PxReal sight[] = {  0.47f, 0.50f, 0.49f, 0.50f,	// line 1
						0.51f, 0.50f, 0.53f, 0.50f,	// line 2
						0.50f, 0.47f, 0.50f, 0.49f,	// line 3
						0.50f, 0.51f, 0.50f, 0.53f	// line 4
					 };

	renderer->drawLines2D(2, sight, sightColor);
	renderer->drawLines2D(2, sight + 4, sightColor);
	renderer->drawLines2D(2, sight + 8, sightColor);
	renderer->drawLines2D(2, sight + 12, sightColor);

	//settings for right lower corner info text (same hight as fps display)
	const PxU32 yInc = 18;
	const PxReal scale = 0.5f;
	const PxReal shadowOffset = 6.0f;
	PxU32 width, height;
	renderer->getWindowSize(width, height);

	PxU32 y = height-2*yInc;
	PxU32 x = width - 360;

	if (mLoadTextAlpha > 0)
	{
		const RendererColor textColor(255, 0, 0, PxU8(mLoadTextAlpha*255.0f));
		const char* message[3] = { "Particle count: Moderate", "Particle count: Medium", "Particle count: High" };
		renderer->print(x, y, message[mParticleLoadIndex], scale, shadowOffset, textColor);
		y -= yInc;
	}

#if PX_SUPPORT_GPU_PHYSX
	{
		const RendererColor textColor(255, 0, 0, 255);

		const char* deviceName = (mRunOnGpu)?getActiveScene().getTaskManager()->getGpuDispatcher()->getCudaContextManager()->getDeviceName():"";
		char buf[2048];
		sprintf(buf, (mRunOnGpu)?"Running on GPU (%s)":"Running on CPU %s", deviceName);

		renderer->print(x, y, buf, scale, shadowOffset, textColor);
	}
#endif
}

void SampleParticles::customizeSample(SampleSetup& setup) 
{
	setup.mName	= "SampleParticles";
}

void SampleParticles::loadAssets() 
{
	// load hotpot PS material
	{
		SampleFramework::SampleAsset* ps_asset = getAsset(smoke_material_path, SampleFramework::SampleAsset::ASSET_MATERIAL);
		mManagedAssets.push_back(ps_asset);
		PX_ASSERT(ps_asset->getType() == SampleFramework::SampleAsset::ASSET_MATERIAL);	
		SampleFramework::SampleMaterialAsset* mat_ps_asset = static_cast<SampleFramework::SampleMaterialAsset*>(ps_asset);
		if(mat_ps_asset->getNumVertexShaders() > 0) 
		{
			RenderMaterial* mat = SAMPLE_NEW(RenderMaterial)(*getRenderer(), mat_ps_asset->getMaterial(0), mat_ps_asset->getMaterialInstance(0), MATERIAL_SMOKE);
			mat->setDiffuseColor(PxVec4(0.2f, 0.2f, 0.2f, 0.3f));
			mRenderMaterials.push_back(mat);
		}
	}
	// load waterfall PS material
	{
		SampleFramework::SampleAsset* ps_asset = getAsset(waterfall_material_path, SampleFramework::SampleAsset::ASSET_MATERIAL);
		mManagedAssets.push_back(ps_asset);
		PX_ASSERT(ps_asset->getType() == SampleFramework::SampleAsset::ASSET_MATERIAL);	
		SampleFramework::SampleMaterialAsset* mat_ps_asset = static_cast<SampleFramework::SampleMaterialAsset*>(ps_asset);
		if(mat_ps_asset->getNumVertexShaders() > 0) 
		{
			RenderMaterial* mat = SAMPLE_NEW(RenderMaterial)(*getRenderer(), mat_ps_asset->getMaterial(0), mat_ps_asset->getMaterialInstance(0), MATERIAL_WATERFALL);
			mat->setDiffuseColor(PxVec4(0.2f, 0.2f, 0.5f, 0.5f));
			mRenderMaterials.push_back(mat);
		}
	}
	// load hf material
	{
		SampleFramework::SampleAsset* ps_asset = getAsset(particle_hf_material_path, SampleFramework::SampleAsset::ASSET_MATERIAL);
		mManagedAssets.push_back(ps_asset);
		PX_ASSERT(ps_asset->getType() == SampleFramework::SampleAsset::ASSET_MATERIAL);	
		SampleFramework::SampleMaterialAsset* mat_ps_asset = static_cast<SampleFramework::SampleMaterialAsset*>(ps_asset);
		if(mat_ps_asset->getNumVertexShaders() > 0) 
		{
			RenderMaterial* mat = SAMPLE_NEW(RenderMaterial)(*getRenderer(), mat_ps_asset->getMaterial(0), mat_ps_asset->getMaterialInstance(0), MATERIAL_HEIGHTFIELD);
			mRenderMaterials.push_back(mat);
		}
	}
	// drain lake material
	{
		RenderMaterial* mat = SAMPLE_NEW(RenderMaterial)(*getRenderer(), PxVec3(0.2f, 0.2f, 0.5f), 0.95f, false, MATERIAL_LAKE, NULL);
		mRenderMaterials.push_back(mat);
	}
	// ray material
	{
		RenderMaterial* mat = SAMPLE_NEW(RenderMaterial)(*getRenderer(), PxVec3(1.0f, 0.1f, 0.1f), 0.5f, false, MATERIAL_RAY, NULL, false);
		mRenderMaterials.push_back(mat);		
	}
	// load debris texture
	{
		SampleFramework::SampleAsset* ps_asset = getAsset(debris_texture, SampleFramework::SampleAsset::ASSET_TEXTURE);
		mManagedAssets.push_back(ps_asset);
		PX_ASSERT(ps_asset->getType() == SampleFramework::SampleAsset::ASSET_TEXTURE);	
		SampleFramework::SampleTextureAsset*tex_ps_asset = static_cast<SampleFramework::SampleTextureAsset*>(ps_asset);
		if(tex_ps_asset) 
		{
			RenderTexture* tex = SAMPLE_NEW(RenderTexture)(*getRenderer(), MATERIAL_DEBRIS, tex_ps_asset->getTexture());
			mRenderTextures.push_back(tex);
			RenderMaterial* mat = SAMPLE_NEW(RenderMaterial)(*getRenderer(), PxVec3(0.5f, 0.5f, 0.5f), 1.0f, true, MATERIAL_DEBRIS, tex, true, false, true);
			mRenderMaterials.push_back(mat);
		}
	}
}

void SampleParticles::newMesh(const RAWMesh& data)
{
	createRenderMeshFromRawMesh(data);
}

void SampleParticles::onInit() 
{
	// try to get gpu support
	mCreateCudaCtxManager = true;
	
	// we don't need ground plane
	mCreateGroundPlane = false;

	mNbThreads = PxMax(PxI32(shdfnd::Thread::getNbPhysicalCores())-1, 0);

	PhysXSample::onInit();

	PxSceneWriteLock scopedLock(*mScene);

	mApplication.setMouseCursorHiding(true);
	mApplication.setMouseCursorRecentering(true);

	mCameraController.init(PxVec3(13.678102f, 39.238392f, 3.8855467f), PxVec3(0.23250008f,-0.54960984f, 0.00000000f));
	mCameraController.setMouseSensitivity(0.5f);

	getRenderer()->setAmbientColor(RendererColor(180, 180, 180));
	// enable loading meterial assets from local media dir
	std::string sampleMediaPath = getApplication().getAssetPathPrefix();//m_assetPathPrefix;
	sampleMediaPath.append("/PhysX/3.0/Samples");
	SampleFramework::addSearchPath(sampleMediaPath.c_str());
		
#if PX_SUPPORT_GPU_PHYSX
	// particles support GPU execution from arch 1.1 onwards. 
	mRunOnGpu = isGpuSupported() && mCudaContextManager->supportsArchSM11();
#endif

	// select particle load
	mParticleLoadIndex = 0;
	mParticleLoadIndexMax = 2;
	mLoadTextAlpha = 0.0f;

	// load materials and textures
	loadAssets();

	//Load the skydome
	importRAWFile("sky_mission_race1.raw", 1.0f);
	
	// load terrain 
	loadTerrain(terrain_hf, 2.0f, 2.0f, 0.3f);
	
	// create invisible cube where the waterfall particles will drain (get removed)
	createDrain();

	//debug visualization parameter
	mScene->setVisualizationParameter(PxVisualizationParameter::ePARTICLE_SYSTEM_BROADPHASE_BOUNDS, 1.0f);
	mScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 0.0f);

	// all particle related setup
	createParticleContent();
}

void SampleParticles::onShutdown()
{
	releaseParticleContent();
	PhysXSample::onShutdown();
}

void SampleParticles::createParticleContent()
{
	PxSceneWriteLock scopedLock(*mScene);

	switch (mParticleLoadIndex)
	{
	case 0:
		{
			// moderate
			mFluidParticleCount = 1000;
			mFluidEmitterWidth = 1.5f;
			mFluidEmitterVelocity = 9.0f;
			mSmokeParticleCount = 200;
			mSmokeLifetime = 2.5f;
			mSmokeEmitterRate = 20.0f;
			mDebrisParticleCount = 200;	
		}
		break;
	case 1:
		{
			// medium
			mFluidParticleCount = 8000;
			mFluidEmitterWidth = 2.0f;
			mFluidEmitterVelocity = 10.0f;
			mSmokeParticleCount = 1200;
			mSmokeLifetime = 3.5f;
			mSmokeEmitterRate = 70.0f;
			mDebrisParticleCount = 800;	
		}
		break;
	case 2:
		{
			// high
			mFluidParticleCount = 20000;
			mFluidEmitterWidth = 3.5f;
			mFluidEmitterVelocity = 50.0f;
			mSmokeParticleCount = 2000;
			mSmokeLifetime = 4.0f;
			mSmokeEmitterRate = 70.0f;
			mDebrisParticleCount = 1600;
		}
		break;
	}

	// create particle systems
	createParticleSystems();

	// create waterfall emitters
	if (mParticleLoadIndex == 0)
	{
		PxTransform transform(PxVec3(-71.8f, 43.0f, -62.0f), PxQuat(-PxHalfPi, PxVec3(1, 0, 0)));	
		createWaterfallEmitter(transform);
	}
	else if (mParticleLoadIndex == 1)
	{
		PxTransform transform0(PxVec3(-39.9, 36.3f, -80.2f), PxQuat(-PxHalfPi, PxVec3(1, 0, 0)));	
		createWaterfallEmitter(transform0);

		PxTransform transform1(PxVec3(-71.8f, 43.0f, -62.0f), PxQuat(-PxHalfPi, PxVec3(1, 0, 0)));	
		createWaterfallEmitter(transform1);
	}
	else
	{
		PxTransform transform(PxVec3(-120.0f, 75.0f, -120.0f), PxQuat(PxHalfPi*0.5f, PxVec3(0, 1, 0)));	
		createWaterfallEmitter(transform);
	}

	// create several hotpots emitters
	createHotpotEmitter(PxVec3(-25.455374f, 20.830761f, -11.995798f));
	createHotpotEmitter(PxVec3(10.154306f, 16.634144f, -97.960060f));
	createHotpotEmitter(PxVec3(14.048334f, 8.3961439f, -32.113029f));

	// initialize raygun, it needs to access samples' functions
	mRaygun.init(this);
}

void SampleParticles::releaseParticleContent()
{
	mRaygun.clear();

	for (PxU32 i = 0; i < mEmitters.size(); ++i)
		DELETESINGLE(mEmitters[i].emitter);

	mEmitters.clear();

	releaseParticleSystems();
}

void SampleParticles::loadTerrain(const char* path, PxReal xScale, PxReal yScale, PxReal zScale) 
{
	SampleFramework::SampleAsset* asset = getAsset(terrain_hf, SampleFramework::SampleAsset::ASSET_TEXTURE);
	mManagedAssets.push_back(asset);
	PX_ASSERT(asset->getType() == SampleFramework::SampleAsset::ASSET_TEXTURE);	
	SampleFramework::SampleTextureAsset* texAsset = static_cast<SampleFramework::SampleTextureAsset*>(asset);
	SampleRenderer::RendererTexture2D* heightfieldTexture = texAsset->getTexture();
	// NOTE: Assuming that heightfield texture has B8G8R8A8 format.
	if(heightfieldTexture) 
	{
		PxU16 nbColumns = PxU16(heightfieldTexture->getWidth());
		PxU16 nbRows = PxU16(heightfieldTexture->getHeight());
		PxHeightFieldDesc heightFieldDesc;
		heightFieldDesc.nbColumns = nbColumns;
		heightFieldDesc.nbRows = nbRows;
		PxU32* samplesData = (PxU32*)SAMPLE_ALLOC(sizeof(PxU32) * nbColumns * nbRows);
		heightFieldDesc.samples.data = samplesData;
		heightFieldDesc.samples.stride = sizeof(PxU32);
		heightFieldDesc.convexEdgeThreshold = 0;
		PxU8* currentByte = (PxU8*)heightFieldDesc.samples.data;
		PxU32 texturePitch = 0;
		PxU8* loaderPtr = static_cast<PxU8*>(heightfieldTexture->lockLevel(0, texturePitch));
		PxVec3Alloc* verticesA = SAMPLE_NEW(PxVec3Alloc)[nbRows * nbColumns];
		PxVec3* vertices = verticesA;
		PxReal* UVs = (PxReal*)SAMPLE_ALLOC(sizeof(PxReal) * nbRows * nbColumns * 2);
		for (PxU32 row = 0; row < nbRows; row++) 
		{
			for (PxU32 column = 0; column < nbColumns; column++) 
			{
				PxHeightFieldSample* currentSample = (PxHeightFieldSample*)currentByte;
				currentSample->height = *loaderPtr;
				vertices[row * nbColumns + column] = 
					PxVec3(PxReal(row) * xScale, 
					PxReal(currentSample->height * zScale), 
					PxReal(column) * yScale);
				
				UVs[2 * (row * nbColumns + column)] = (PxReal(row) / PxReal(nbRows)) * 7.0f;
				UVs[2 * (row * nbColumns + column) + 1] = (PxReal(column) / PxReal(nbColumns)) * 7.0f;
				currentSample->materialIndex0 = 0;
				currentSample->materialIndex1 = 0;
				currentSample->clearTessFlag();
				currentByte += heightFieldDesc.samples.stride;
				loaderPtr += 4 * sizeof(PxU8);
			}
		}
		PxHeightField* heightField = getCooking().createHeightField(heightFieldDesc, getPhysics().getPhysicsInsertionCallback());
		// free allocated memory for heightfield samples description
		SAMPLE_FREE(samplesData);
		// create shape for heightfield		
		PxTransform pose(PxVec3(-((PxReal)nbRows*yScale) / 2.0f, 
								0.0f, 
								-((PxReal)nbColumns*xScale) / 2.0f), 
						PxQuat(PxIdentity));
		PxRigidActor* hf = getPhysics().createRigidStatic(pose);
		runtimeAssert(hf, "PxPhysics::createRigidStatic returned NULL\n");
		PxShape* shape = PxRigidActorExt::createExclusiveShape(*hf, PxHeightFieldGeometry(heightField, PxMeshGeometryFlags(), zScale, xScale, yScale), getDefaultMaterial());
		runtimeAssert(shape, "PxRigidActor::createShape returned NULL\n");
		shape->setFlag(PxShapeFlag::ePARTICLE_DRAIN, false);
		shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
		// add actor to the scene
		getActiveScene().addActor(*hf);
		mPhysicsActors.push_back(hf);
		// create indices and UVs
		PxU32* indices = (PxU32*)SAMPLE_ALLOC(sizeof(PxU32) * (nbColumns - 1) * (nbRows - 1) * 3 * 2);
		for(int i = 0; i < (nbColumns - 1); ++i) 
		{
			for(int j = 0; j < (nbRows - 1); ++j) 
			{
				// first triangle
				indices[6 * (i * (nbRows - 1) + j) + 0] = (i + 1) * nbRows + j; 
				indices[6 * (i * (nbRows - 1) + j) + 1] = i * nbRows + j;
				indices[6 * (i * (nbRows - 1) + j) + 2] = i * nbRows + j + 1;
				// second triangle
				indices[6 * (i * (nbRows - 1) + j) + 3] = (i + 1) * nbRows + j + 1;
				indices[6 * (i * (nbRows - 1) + j) + 4] = (i + 1) * nbRows + j;
				indices[6 * (i * (nbRows - 1) + j) + 5] = i * nbRows + j + 1;
			}
		}
		// add mesh to renderer
		RAWMesh data;
		data.mName = "terrain";
		data.mTransform = PxTransform(PxIdentity);
		data.mNbVerts = nbColumns * nbRows;
		data.mVerts = (PxVec3*)vertices;
		data.mVertexNormals = NULL;
		data.mUVs = UVs;
		data.mMaterialID = MATERIAL_HEIGHTFIELD;
		data.mNbFaces = (nbColumns - 1) * (nbRows - 1) * 2;
		data.mIndices = indices;
		RenderMeshActor* hf_mesh = createRenderMeshFromRawMesh(data);
		hf_mesh->setPhysicsShape(shape, hf);
		SAMPLE_FREE(indices);
		SAMPLE_FREE(UVs);
		DELETEARRAY(verticesA);
	} 
	else 
	{ 
		char errMsg[256];
		Ps::snprintf(errMsg, 256, "Couldn't load %s\n", path);
		fatalError(errMsg);
	}
}

void SampleParticles::customizeSceneDesc(PxSceneDesc& setup) 
{
	setup.filterShader = SampleParticlesFilterShader;
	setup.flags |= PxSceneFlag::eREQUIRE_RW_LOCK;
}

void SampleParticles::collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents)
{
	PhysXSample::collectInputEvents(inputEvents);

	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(SPAWN_DEBUG_OBJECT);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(CAMERA_SPEED_INCREASE);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(CAMERA_SPEED_DECREASE);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(CAMERA_MOVE_BUTTON);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(MOUSE_LOOK_BUTTON);

	//digital keyboard events
	DIGITAL_INPUT_EVENT_DEF(RAYGUN_SHOT,				SCAN_CODE_SPACE,	XKEY_SPACE,		X1KEY_SPACE,	PS3KEY_SPACE,	PS4KEY_SPACE,	AKEY_UNKNOWN,	OSXKEY_SPACE,		IKEY_UNKNOWN, 	LINUXKEY_SPACE,		WIIUKEY_UNKNOWN);
	DIGITAL_INPUT_EVENT_DEF(CHANGE_PARTICLE_CONTENT,	WKEY_M,				XKEY_M,			X1KEY_M,		PS3KEY_M,		PS4KEY_M,		AKEY_UNKNOWN,	OSXKEY_M,			IKEY_UNKNOWN,	LINUXKEY_M,			WIIUKEY_UNKNOWN);
	
	//digital mouse events
	DIGITAL_INPUT_EVENT_DEF(RAYGUN_SHOT,				MOUSE_BUTTON_LEFT,	XKEY_UNKNOWN,	X1KEY_UNKNOWN,	PS3KEY_UNKNOWN,	PS4KEY_UNKNOWN,	AKEY_UNKNOWN,	MOUSE_BUTTON_LEFT,	KEY_UNKNOWN,	MOUSE_BUTTON_LEFT,	WIIUKEY_UNKNOWN);

	//digital gamepad events
	DIGITAL_INPUT_EVENT_DEF(RAYGUN_SHOT,				GAMEPAD_RIGHT_SHOULDER_BOT, GAMEPAD_RIGHT_SHOULDER_BOT, GAMEPAD_RIGHT_SHOULDER_BOT, GAMEPAD_RIGHT_SHOULDER_BOT, GAMEPAD_RIGHT_SHOULDER_BOT, AKEY_UNKNOWN, GAMEPAD_RIGHT_SHOULDER_BOT, IKEY_UNKNOWN, LINUXKEY_UNKNOWN, GAMEPAD_RIGHT_SHOULDER_BOT);

    //touch events
    TOUCH_INPUT_EVENT_DEF(RAYGUN_SHOT,			"Ray Gun",      AQUICK_BUTTON_1,    IQUICK_BUTTON_1);
}

void SampleParticles::onDigitalInputEvent(const SampleFramework::InputEvent& ie, bool val)
{
	switch(ie.m_Id) 
	{
	case RAYGUN_SHOT:
		mRaygun.setEnabled(val);
		break;
	case CHANGE_PARTICLE_CONTENT:
		if (val)
		{
			mParticleLoadIndex = (mParticleLoadIndex+1) % (mParticleLoadIndexMax+1);
			releaseParticleContent();
			createParticleContent();
			mLoadTextAlpha = 1.0f;
		}
		break;
#if PX_SUPPORT_GPU_PHYSX
	case TOGGLE_CPU_GPU:
		if (val) toggleGpu();
		break;
#endif
	default:
		PhysXSample::onDigitalInputEvent(ie,val);
		break;
	}	
}

void SampleParticles::onPointerInputEvent(const SampleFramework::InputEvent& ie, physx::PxU32 x, physx::PxU32 y, physx::PxReal dx, physx::PxReal dy, bool val)
{
	if(ie.m_Id == RAYGUN_SHOT) 
		mRaygun.setEnabled(val);
}

PxParticleFluid* SampleParticles::createFluid(PxU32 maxParticles, PxReal restitution, PxReal viscosity,
									PxReal stiffness, PxReal dynamicFriction, PxReal particleDistance) 
{	
	PxParticleFluid* fluid = getPhysics().createParticleFluid(maxParticles);
	runtimeAssert(fluid, "PxPhysics::createParticleFluid returned NULL\n");
	fluid->setGridSize(5.0f);
	fluid->setMaxMotionDistance(0.3f);
	fluid->setRestOffset(particleDistance*0.3f);
	fluid->setContactOffset(particleDistance*0.3f*2);
	fluid->setDamping(0.0f);
	fluid->setRestitution(restitution);
	fluid->setDynamicFriction(dynamicFriction);
	fluid->setRestParticleDistance(particleDistance);
	fluid->setViscosity(viscosity);
	fluid->setStiffness(stiffness);
	fluid->setParticleReadDataFlag(PxParticleReadDataFlag::eVELOCITY_BUFFER, true);
#if PX_SUPPORT_GPU_PHYSX
	fluid->setParticleBaseFlag(PxParticleBaseFlag::eGPU, mRunOnGpu);
#endif
	getActiveScene().addActor(*fluid);
	runtimeAssert(fluid->getScene(), "PxScene::addActor failed\n");

#if PX_SUPPORT_GPU_PHYSX
	//check gpu flags after adding to scene, cpu fallback might have been used.
	mRunOnGpu = mRunOnGpu && (fluid->getParticleBaseFlags() & PxParticleBaseFlag::eGPU);
#endif

	return fluid;
}

PxParticleSystem* SampleParticles::createParticleSystem(PxU32 maxParticles) 
{	
	PxParticleSystem* ps = getPhysics().createParticleSystem(maxParticles);
	runtimeAssert(ps, "PxPhysics::createParticleSystem returned NULL\n");
	ps->setGridSize(3.0f);
	ps->setMaxMotionDistance(0.43f);
	ps->setRestOffset(0.0143f);
	ps->setContactOffset(0.0143f*2);
	ps->setDamping(0.0f);
	ps->setRestitution(0.2f);
	ps->setDynamicFriction(0.05f);
	ps->setParticleReadDataFlag(PxParticleReadDataFlag::eVELOCITY_BUFFER, true);
#if PX_SUPPORT_GPU_PHYSX
	ps->setParticleBaseFlag(PxParticleBaseFlag::eGPU, mRunOnGpu);
#endif
	getActiveScene().addActor(*ps);
	runtimeAssert(ps->getScene(), "PxScene::addActor failed\n");

#if PX_SUPPORT_GPU_PHYSX
	//check gpu flags after adding to scene, cpu fallback might have been used.
	mRunOnGpu = mRunOnGpu && (ps->getParticleBaseFlags() & PxParticleBaseFlag::eGPU);
#endif
	return ps;
}

void SampleParticles::createDrain() 
{
	float lakeHeight = 5.5f;
	
	//Creates a drain plane for the particles. This is good practice to avoid unnecessary 
	//spreading of particles, which is bad for performance. The drain represents a lake in this case.
	{
		PxRigidStatic* actor = getPhysics().createRigidStatic(PxTransform(PxVec3(0.0f, lakeHeight - 1.0f, 0.0f), PxQuat(PxHalfPi, PxVec3(0,0,1))));
		runtimeAssert(actor, "PxPhysics::createRigidStatic returned NULL\n");
		PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor, PxPlaneGeometry(), getDefaultMaterial());
		runtimeAssert(shape, "PxRigidStatic::createShape returned NULL\n");
		shape->setSimulationFilterData(collisionGroupDrain);
		shape->setFlags(PxShapeFlag::ePARTICLE_DRAIN | PxShapeFlag::eSIMULATION_SHAPE);
		getActiveScene().addActor(*actor);
		mPhysicsActors.push_back(actor);
	}
	
	//Creates the surface of the lake (the particles actually just collide with the underlaying drain).
	{
		PxBoxGeometry bg;
		bg.halfExtents = PxVec3(130.0f, lakeHeight + 1.0f, 130.0f);
		PxRigidStatic* actor = getPhysics().createRigidStatic(PxTransform(PxVec3(0.0f, 0.0f, 0.0f), PxQuat(PxIdentity)));
		runtimeAssert(actor, "PxPhysics::createRigidStatic returned NULL\n");
		PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor, bg, getDefaultMaterial());
		runtimeAssert(shape, "PxRigidStatic::createShape returned NULL\n");
		shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
		shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		getActiveScene().addActor(*actor);
		mPhysicsActors.push_back(actor);
		createRenderObjectsFromActor(actor, getMaterial(MATERIAL_LAKE));
	}
}

void SampleParticles::createParticleSystems()
{
	// create waterfall fluid
	{
		PxParticleFluid* px_fluid = createFluid(mFluidParticleCount, 0.3f, 60.0f, 45.0f, 0.001f, 0.8f);
		px_fluid->setSimulationFilterData(collisionGroupWaterfall);
		// This will be deleted by RenderParticleSystemActor
		ParticleSystem* fluid = SAMPLE_NEW(ParticleSystem)(px_fluid);
		mWaterfallParticleSystem = SAMPLE_NEW(RenderParticleSystemActor)(*getRenderer(), fluid, false);
		mWaterfallParticleSystem->setRenderMaterial(getMaterial(MATERIAL_WATERFALL));
		addRenderParticleSystemActor(*mWaterfallParticleSystem);
	}

	// create smoke particle system
	{
		PxParticleBase* px_ps = createParticleSystem(mSmokeParticleCount);
		px_ps->setDamping(0.5f);
		px_ps->setRestitution(0.0f);
		px_ps->setDynamicFriction(0.0f);
		px_ps->setExternalAcceleration(PxVec3(0.0f, 17.0f, 0.0f));
		px_ps->setSimulationFilterData(collisionGroupSmoke);
		// This will be deleted by RenderParticleSystemActor
		ParticleSystem* ps = SAMPLE_NEW(ParticleSystem)(px_ps, false);
		ps->setUseLifetime(true);
		ps->setLifetime(mSmokeLifetime);
		mSmokeParticleSystem = SAMPLE_NEW(RenderParticleSystemActor)(*getRenderer(), ps, false, true, mSmokeLifetime*1.5f);
		mSmokeParticleSystem->setRenderMaterial(getMaterial(MATERIAL_SMOKE));
		addRenderParticleSystemActor(*mSmokeParticleSystem);
	}

	// create debris particle system
#if !defined(RENDERER_ENABLE_GLES2)
	{
		PxParticleBase* px_ps = createParticleSystem(mDebrisParticleCount);
		px_ps->setStaticFriction(10.0f);
		px_ps->setSimulationFilterData(collisionGroupDebris);
		// This will be deleted by RenderParticleSystemActor
		ParticleSystem* ps = SAMPLE_NEW(ParticleSystem)(px_ps, true);
		ps->setUseLifetime(true);
		ps->setLifetime(5.0f);
		mDebrisParticleSystem = SAMPLE_NEW(RenderParticleSystemActor)(*getRenderer(), ps, true, false, 4.0f, 0.3f);
		mDebrisParticleSystem->setRenderMaterial(getMaterial(MATERIAL_DEBRIS));
		addRenderParticleSystemActor(*mDebrisParticleSystem);
	}
#endif
}

void SampleParticles::releaseParticleSystems()
{	
	PxSceneWriteLock scopedLock(*mScene);

	if (mWaterfallParticleSystem)
	{
		removeRenderParticleSystemActor(*mWaterfallParticleSystem);
		mWaterfallParticleSystem->release();
		mWaterfallParticleSystem = NULL;
	}

	if (mSmokeParticleSystem)
	{
		removeRenderParticleSystemActor(*mSmokeParticleSystem);
		mSmokeParticleSystem->release();
		mSmokeParticleSystem = NULL;
	}

	if (mDebrisParticleSystem)
	{
		removeRenderParticleSystemActor(*mDebrisParticleSystem);
		mDebrisParticleSystem->release();
		mDebrisParticleSystem = NULL;
	}
}

void SampleParticles::createWaterfallEmitter(const PxTransform& transform) 
{
	// setup emitter
	ParticleEmitterPressure* pressureEmitter = SAMPLE_NEW(ParticleEmitterPressure)(
		ParticleEmitter::Shape::eRECTANGLE, mFluidEmitterWidth, mFluidEmitterWidth, 0.81f);	// shape and size of emitter and spacing between particles being emitted

	pressureEmitter->setMaxRate(PxF32(mFluidParticleCount));
	pressureEmitter->setVelocity(mFluidEmitterVelocity);
	pressureEmitter->setRandomAngle(0.0f);
	pressureEmitter->setRandomPos(PxVec3(0.0f,0.0f,0.0f));
	pressureEmitter->setLocalPose(transform);

	Emitter emitter;
	emitter.emitter = pressureEmitter;
	emitter.isEnabled = true;
	emitter.particleSystem = mWaterfallParticleSystem;
	mEmitters.push_back(emitter);
}

void SampleParticles::createHotpotEmitter(const PxVec3& position) 
{
	// setup smoke emitter
	{
		ParticleEmitterRate* rateEmitter = SAMPLE_NEW(ParticleEmitterRate)(
			ParticleEmitter::Shape::eRECTANGLE, 1.0f, 1.0f, 0.1f);				// shape and size of emitter and spacing between particles being emitted

		rateEmitter->setRate(mSmokeEmitterRate);
		rateEmitter->setVelocity(3.0f);
		rateEmitter->setRandomAngle(PxHalfPi / 8);
		rateEmitter->setLocalPose(PxTransform(PxVec3(position.x, position.y, position.z), PxQuat(-PxHalfPi, PxVec3(1, 0, 0))));
	
		Emitter emitter;
		emitter.emitter = rateEmitter;
		emitter.isEnabled = true;
		emitter.particleSystem = mSmokeParticleSystem;
		mEmitters.push_back(emitter);
	}
	
	// setup debris emitter
#if !defined(RENDERER_ENABLE_GLES2)
	{
		ParticleEmitterRate* rateEmitter = SAMPLE_NEW(ParticleEmitterRate)(
			ParticleEmitter::Shape::eRECTANGLE, 1.0f, 1.0f, 0.1f);				// shape and size of emitter and spacing between particles being emitted

		rateEmitter->setRate(10.0f);
		rateEmitter->setVelocity(12.0f);
		rateEmitter->setRandomAngle(PxHalfPi / 8);
		rateEmitter->setLocalPose(PxTransform(PxVec3(position.x, position.y, position.z), PxQuat(-PxHalfPi, PxVec3(1, 0, 0))));
	
		Emitter emitter;
		emitter.emitter = rateEmitter;
		emitter.isEnabled = true;
		emitter.particleSystem = mDebrisParticleSystem;
		mEmitters.push_back(emitter);		
	}
#endif
}
RenderMaterial* SampleParticles::getMaterial(PxU32 materialID) 
{
	for(PxU32 i = 0; i < mRenderMaterials.size(); ++i) 
	{
		if(mRenderMaterials[i]->mID == materialID) 
		{
			return mRenderMaterials[i];
		}
	}

	return NULL;
}

template<typename T>
void SampleParticles::runtimeAssert(const T x, 
									const char* msg, 
									const char* extra1, 
									const char* extra2, 
									const char* extra3) 
{

	if(!x) 
	{
		std::string s(msg);
		if(extra1) s.append(extra1);
		if(extra2) s.append(extra2);
		if(extra3) s.append(extra3);
		s.append("\n");
		fatalError(s.c_str());
	}
}

void SampleParticles::Emitter::update(float dtime)
{
	if(!isEnabled || !particleSystem)
		return; 

	ParticleSystem& ps = *particleSystem->getParticleSystem();

	PxVec3 acceleration = ps.getPxParticleBase()->getExternalAcceleration() + ps.getPxParticleBase()->getScene()->getGravity();
	float maxVeloctiy = ps.getPxParticleBase()->getMaxMotionDistance() / (dtime);	

	ParticleData initData(ps.getPxParticleBase()->getMaxParticles());
	emitter->step(initData, dtime, acceleration, maxVeloctiy);

	ps.createParticles(initData);
}

SampleParticles::Raygun::Raygun() : 
	mForceSmokeCapsule(NULL), 
	mForceWaterCapsule(NULL),
	mRenderActor(NULL),
	mSample(NULL),
	mIsImpacting (false)
{
}

SampleParticles::Raygun::~Raygun() 
{
}

void SampleParticles::Raygun::init(SampleParticles* sample) 
{
	mSample = sample;
	mRbDebrisTimer = 0.0f;
	mForceTimer = 0.0f;
	
	// create smoke emitter
	{
		ParticleEmitterRate* rateEmitter = SAMPLE_NEW(ParticleEmitterRate)(
			ParticleEmitter::Shape::eRECTANGLE, 1.0f, 1.0f, 0.1f);				// shape and size of emitter and spacing between particles being emitted

		rateEmitter->setRate(sample->mSmokeEmitterRate/2);
		rateEmitter->setVelocity(2.0f);
		rateEmitter->setRandomAngle(PxHalfPi / 3);
		rateEmitter->setLocalPose(PxTransform(PxIdentity));

		mSmokeEmitter.emitter = rateEmitter;
		mSmokeEmitter.isEnabled = true;
		mSmokeEmitter.particleSystem = mSample->mSmokeParticleSystem;
	}

	// create debris emitter
#if !defined(RENDERER_ENABLE_GLES2)
	{
		ParticleEmitterRate* rateEmitter = SAMPLE_NEW(ParticleEmitterRate)(
			ParticleEmitter::Shape::eRECTANGLE, 1.0f, 1.0f, 0.1f);				// shape and size of emitter and spacing between particles being emitted

		rateEmitter->setRate(50.0f);
		rateEmitter->setVelocity(3.0f);
		rateEmitter->setRandomAngle(PxHalfPi / 6);
		rateEmitter->setLocalPose(PxTransform(PxIdentity));

		mDebrisEmitter.emitter = rateEmitter;
		mDebrisEmitter.isEnabled = true;
		mDebrisEmitter.particleSystem = mSample->mDebrisParticleSystem;
	}
#endif 
	setEnabled(false);
}

void SampleParticles::Raygun::clear() 
{
	DELETESINGLE(mSmokeEmitter.emitter);
	DELETESINGLE(mDebrisEmitter.emitter);
	
	mDebrisLifetime.clear();
	mIsImpacting = false;
	
	releaseRayCapsule(mForceSmokeCapsule);
	releaseRayCapsule(mForceWaterCapsule);
	if (mRenderActor)
	{
		mSample->removeRenderObject(mRenderActor);
		mRenderActor = NULL;
	}
}

bool SampleParticles::Raygun::isEnabled() 
{ 
	return mSmokeEmitter.isEnabled && mDebrisEmitter.isEnabled; 
}

void SampleParticles::Raygun::setEnabled(bool enabled) 
{
	if (enabled == isEnabled())
		return;

	if (enabled)
	{
		mForceTimer = 0.0f;
		
		// create invisible capsule shape colliding with smoke particles
		mForceSmokeCapsule = createRayCapsule(true, collisionGroupForceSmoke);		

		// create invisible capsule shape colliding with waterfall particles
		mForceWaterCapsule = createRayCapsule(true, collisionGroupForceWater);	

		// create visible box for rendering
		{
			Renderer* renderer = mSample->getRenderer();
			PX_ASSERT(renderer);
			mRenderActor = SAMPLE_NEW(RenderBoxActor)(*renderer, PxVec3(150.0f,0.1f, 0.1f));

			if(mRenderActor)
			{
				mSample->mRenderActors.push_back(mRenderActor);
				mRenderActor->setRenderMaterial(mSample->getMaterial(MATERIAL_RAY));
			}
		}
	}
	else
	{
		// remove obsolete actors
		releaseRayCapsule(mForceSmokeCapsule);
		releaseRayCapsule(mForceWaterCapsule);

		if (mRenderActor)
		{
			mSample->removeRenderObject(mRenderActor);
			mRenderActor = NULL;
		}
	}
	
	mSmokeEmitter.isEnabled = enabled;
	mDebrisEmitter.isEnabled = enabled;
}

void SampleParticles::Raygun::onSubstep(float dtime)
{
	// timing and emitters are run every substep to get 
	// as deterministic behavior as possible
	mRbDebrisTimer -= dtime;
	mForceTimer += dtime;
	
	if (mIsImpacting)
	{
		mSmokeEmitter.update(dtime);
		mDebrisEmitter.update(dtime);
	}
}

void SampleParticles::Raygun::update(float dtime)
{
	if(!isEnabled())
		return;
	
	PX_ASSERT(mSample && mForceSmokeCapsule && mForceWaterCapsule && mRenderActor);

	// access properties from sample
	PxScene& scene = mSample->getActiveScene();
	PxVec3 position = mSample->getCamera().getPos();
	PxTransform cameraPose = mSample->getCamera().getViewMatrix();
	PxMat33 cameraBase(cameraPose.q);
	PxVec3 cameraForward = -cameraBase[2];
	PxVec3 cameraUp = -cameraBase[1];

	PxSceneWriteLock scopedLock(scene);
	
	// perform raycast here and update impact point
	PxRaycastBuffer hit;
	mIsImpacting = scene.raycast(cameraPose.p, cameraForward, 500.0f, hit, PxHitFlags(0xffff));
	float impactParam = mIsImpacting ? (hit.block.position - position).magnitude() : FLT_MAX;	

	PxTransform rayPose(position + cameraUp * 0.5f, cameraPose.q*PxQuat(PxHalfPi, PxVec3(0,1,0)));

	updateRayCapsule(mForceSmokeCapsule, rayPose, 1.0f);
	updateRayCapsule(mForceWaterCapsule, rayPose, 0.3f);
	mRenderActor->setTransform(rayPose);

	// if we had an impact
	if (impactParam < FLT_MAX)
	{
		PxVec3 impactPos = position + cameraForward*impactParam;
		// update emitter with new impact point and direction
		if(mSmokeEmitter.emitter)
			mSmokeEmitter.emitter->setLocalPose(PxTransform(impactPos, directionToQuaternion(-cameraForward)));
		if(mDebrisEmitter.emitter)
			mDebrisEmitter.emitter->setLocalPose(PxTransform(impactPos, directionToQuaternion(-cameraForward)));

		// spawn new RB debris
		if(mRbDebrisTimer < 0.0f && impactParam < FLT_MAX) 
		{
			mRbDebrisTimer = RAYGUN_RB_DEBRIS_RATE;

			PxVec3 randDir(getSampleRandom().rand(-1.0f, 1.0f),
				getSampleRandom().rand(-1.0f, 1.0f),
				getSampleRandom().rand(-1.0f, 1.0f));
			PxVec3 vel = -7.0f * (cameraForward + RAYGUN_RB_DEBRIS_ANGLE_RANDOMNESS * randDir.getNormalized());
			PxVec3 dim(getSampleRandom().rand(0.0f, RAYGUN_RB_DEBRIS_SCALE),
				getSampleRandom().rand(0.0f, RAYGUN_RB_DEBRIS_SCALE),
				getSampleRandom().rand(0.0f, RAYGUN_RB_DEBRIS_SCALE));
			// give spawn position, initial velocity and dimensions, spawn convex
			// which will not act in scene queries
			PxConvexMesh* convexMesh = generateConvex(mSample->getPhysics(), mSample->getCooking(), RAYGUN_RB_DEBRIS_SCALE);
			mSample->runtimeAssert(convexMesh, "Error generating convex for debris.\n");
			PxRigidDynamic* debrisActor = PxCreateDynamic(
				mSample->getPhysics(), 
				PxTransform(impactPos - cameraForward * 0.5f), 
				PxConvexMeshGeometry(convexMesh), 
				mSample->getDefaultMaterial(), 1.f);  
			mSample->getActiveScene().addActor(*debrisActor);

			PX_ASSERT(debrisActor->getNbShapes() == 1);
			PxShape* debrisShape;
			debrisActor->getShapes(&debrisShape, 1);
			debrisShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, false);
			debrisActor->setLinearVelocity(vel);
			debrisActor->setActorFlag(PxActorFlag::eVISUALIZATION, true);
			debrisActor->setAngularDamping(0.5f);
			
			// default material is green for debris
			RenderMaterial* debriMaterial = mSample->getMaterial(MATERIAL_HEIGHTFIELD);
			if(!debriMaterial) 
			{
				debriMaterial = mSample->mRenderMaterials[MATERIAL_GREEN];				
			}
			mSample->createRenderObjectsFromActor(debrisActor, debriMaterial);
			mDebrisLifetime[debrisShape] = RAYGUN_RB_DEBRIS_LIFETIME;
		}
	}

	// update debris lifetime, remove if life ends
	DebrisLifetimeMap::iterator it = mDebrisLifetime.begin();
	while(it != mDebrisLifetime.end()) 
	{
		(*it).second -= dtime;
		if((*it).second < 0.0f) 
		{
			PxShape* debrisShape = (*it).first;
			PX_ASSERT(debrisShape);

			// remove convex mesh
			PxConvexMeshGeometry geometry;
			bool isConvex = debrisShape->getConvexMeshGeometry(geometry);
			PX_ASSERT(isConvex);
			PX_UNUSED(isConvex);
			geometry.convexMesh->release();
			
			// remove render and physics actor
			PxRigidActor* actorToRemove = debrisShape->getActor();
			mSample->removeActor(actorToRemove);			
			actorToRemove->release();
			
			// remove actor from lifetime map
			mDebrisLifetime.erase(it);
			it = mDebrisLifetime.begin();
			continue;
		}
		++it;
	}
}

PxConvexMesh* SampleParticles::Raygun::generateConvex(PxPhysics& sdk, PxCooking& cooking, PxReal scale) 
{
	const PxI32 minNb = 3;
	const PxI32 maxNb = 8;
	const int nbInsideCirclePts = getSampleRandom().rand(minNb, maxNb);
	const int nbOutsideCirclePts = getSampleRandom().rand(minNb, maxNb);
	const int nbVerts = nbInsideCirclePts + nbOutsideCirclePts;

	// Generate random vertices
	PxVec3Alloc* verts = SAMPLE_NEW(PxVec3Alloc)[nbVerts];

	for(int i = 0; i < nbVerts; ++i)
	{
		verts[i].x = scale * getSampleRandom().rand(-1.0f, 1.0f);
		verts[i].y = scale * getSampleRandom().rand(-1.0f, 1.0f);
		verts[i].z = scale * getSampleRandom().rand(-1.0f, 1.0f);
	}

	PxConvexMesh* convexMesh = PxToolkit::createConvexMesh(sdk, cooking, verts, nbVerts, PxConvexFlag::eCOMPUTE_CONVEX);
	mSample->runtimeAssert(convexMesh, "PxPhysics::createConvexMesh returned NULL\n");

	DELETEARRAY(verts);
	return convexMesh;
}

PxRigidDynamic* SampleParticles::Raygun::createRayCapsule(bool enableCollision, PxFilterData filterData)
{
	PxSceneWriteLock scopedLock(mSample->getActiveScene());
	PxRigidDynamic* actor = mSample->getPhysics().createRigidDynamic(PxTransform(PxIdentity));
	mSample->runtimeAssert(actor, "PxPhysics::createRigidDynamic returned NULL\n");
	PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor, PxCapsuleGeometry(0.1f, 150.0f), mSample->getDefaultMaterial());
	mSample->runtimeAssert(shape, "PxRigidDynamic::createShape returned NULL\n");
	shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, false);
	shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, enableCollision);
	shape->setSimulationFilterData(filterData);
	actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
	actor->setMass(1.0f);
	actor->setMassSpaceInertiaTensor(PxVec3(1.0f));
	mSample->getActiveScene().addActor(*actor);	
	return actor;
}

void SampleParticles::Raygun::releaseRayCapsule(PxRigidDynamic*& actor)
{
	if (actor)
	{
		PxSceneWriteLock scopedLock(mSample->getActiveScene());
		mSample->removeActor(actor);
		actor->release();
		actor = NULL;
	}
}

void SampleParticles::Raygun::updateRayCapsule(PxRigidDynamic* actor, const PxTransform& pose, float radiusMaxMultiplier)
{
	PxSceneWriteLock scopedLock(mSample->getActiveScene());
	// move and resize capsule
	actor->setGlobalPose(pose);

	// scale force (represented with capsule RB)
	if (mForceTimer < RAYGUN_FORCE_GROW_TIME)
	{
		float newRadius = PxMax(RAYGUN_FORCE_SIZE_MAX*radiusMaxMultiplier*mForceTimer/RAYGUN_FORCE_GROW_TIME, RAYGUN_FORCE_SIZE_MIN);
		PxShape* shape;
		actor->getShapes(&shape, 1);
		shape->setGeometry(PxCapsuleGeometry(newRadius, 150.0f));
	}
}

#if PX_SUPPORT_GPU_PHYSX
void SampleParticles::toggleGpu()
{
	bool canRunOnGpu = false;

	// particles support GPU execution from arch 1.1 onwards. 
	canRunOnGpu = isGpuSupported() && mCudaContextManager->supportsArchSM11();

	if (!canRunOnGpu)
		return;

	mRunOnGpu = !mRunOnGpu;

	// reset all the particles to switch between GPU and CPU
	static const PxU32 sActorBufferSize = 100;
	PxActor* actors[sActorBufferSize];

	PxSceneWriteLock scopedLock(*mScene);

	PxU32 numParticleFluidActors = getActiveScene().getActors(PxActorTypeFlag::ePARTICLE_FLUID, actors, sActorBufferSize);
	PX_ASSERT(numParticleFluidActors < sActorBufferSize);
	for (PxU32 i = 0; i < numParticleFluidActors; i++)
	{
		PxParticleFluid& fluid = *actors[i]->is<PxParticleFluid>();
		fluid.setParticleBaseFlag(PxParticleBaseFlag::eGPU, mRunOnGpu);
	}

	PxU32 numParticleSystemActors = getActiveScene().getActors(PxActorTypeFlag::ePARTICLE_SYSTEM, actors, sActorBufferSize);
	PX_ASSERT(numParticleSystemActors < sActorBufferSize);
	for (PxU32 i = 0; i < numParticleSystemActors; i++)
	{
		PxParticleSystem& fluid = *actors[i]->is<PxParticleSystem>();
		fluid.setParticleBaseFlag(PxParticleBaseFlag::eGPU, mRunOnGpu);
	}
}

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif





