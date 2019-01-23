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

#include "foundation/PxMemory.h"
#include <PsString.h>
#include "SampleLargeWorld.h"
#include "RenderMeshActor.h"
#include "RenderMaterial.h"
#include "RenderPhysX3Debug.h"

#include <SampleBaseInputEventIds.h>
#include <SamplePlatform.h>
#include <SampleUserInput.h>
#include <SampleUserInputIds.h>
#include <SampleUserInputDefines.h>
#include "SampleLargeWorldInputEventIds.h"

using namespace SampleRenderer;
using namespace SampleFramework;

#include "ChunkLoader.h"
#include "wavefront.h"
#include "characterkinematic/PxControllerManager.h"
#include "SampleCCTActor.h"
#include "SampleCCTCameraController.h"

#include "extensions/PxDefaultSimulationFilterShader.h"

static const PxF32 gContactOffset			= 0.01f;
static const PxF32 gStepOffset				= 0.05f;
static const PxF32 gSlopeLimit				= 0.0f;
static const PxF32 gInvisibleWallsHeight	= 0.0f;
static const PxF32 gMaxJumpHeight			= 0.0f;

static const PxF32 gScaleFactor				= 1.5f;
static const PxF32 gStandingSize			= 1.0f * gScaleFactor;
static const PxF32 gCrouchingSize			= 0.25f * gScaleFactor;
static const PxF32 gControllerRadius		= 0.3f * gScaleFactor;

static const PxF32 CAMERA_MAX_SPEED			= 120.0f;

const char* gDynamic = "Dynamic";

REGISTER_SAMPLE(SampleLargeWorld, "SampleLargeWorld")	

///////////////////////////////////////////////////////////////////////////////
SampleLargeWorld::SampleLargeWorld(PhysXSampleApplication& app) 
	: PhysXSample(app)
	, mCCTCamera(NULL)
	, mActor(NULL)
	, mControllerManager(NULL)
	, mDiskIOTime(0)
	, mPhysxStreaming(0)
	, mGraphicStreaming(0)
	, mReadyToSyncCCT(false)
	, mAddRenderActor(false)
	, mPick(false)
	, mKeyShiftDown(false)
	, mStringTable(NULL)
{
	mCreateGroundPlane	= false;
	mControllerInitialPosition = PxExtendedVec3(1035.0f, 49.0f, 989.0f);
	mLastCCTPosition = mControllerInitialPosition;
	mDefaultCameraSpeed = mCameraController.getCameraSpeed();

	PxMemZero(mSkybox, sizeof(mSkybox));
}

SampleLargeWorld::~SampleLargeWorld()
{
	DELETESINGLE(mActor);
	for(ObjMeshMap::Iterator iter = mRenderMeshCache.getIterator(); !iter.done(); ++iter)
	{
		RAWMesh& mesh = iter->second;

		PxVec3 *verts = const_cast<PxVec3*>(mesh.mVerts);
		SAMPLE_FREE(verts);
		mesh.mVerts = NULL;

		PxVec3 *normals = const_cast<PxVec3*>(mesh.mVertexNormals);
		SAMPLE_FREE(normals);
		mesh.mVertexNormals = NULL;

		PxVec3 *colors = const_cast<PxVec3*>(mesh.mVertexColors);
		SAMPLE_FREE(colors);
		mesh.mVertexColors = NULL;

		PxReal *uvs = const_cast<PxReal*>(mesh.mUVs);
		SAMPLE_FREE(uvs);
		mesh.mUVs = NULL;

		PxU32 *indices = const_cast<PxU32*>(mesh.mIndices);
		SAMPLE_FREE(indices);
		mesh.mIndices = NULL;
	}
}

void SampleLargeWorld::setCCDActive(PxShape& shape)
{	
	PxFilterData fd = shape.getSimulationFilterData();
	fd.word3 |= CCD_FLAG;
	shape.setSimulationFilterData(fd);
}

bool SampleLargeWorld::isCCDActive(PxFilterData& filterData)
{
	return filterData.word3 & CCD_FLAG ? true : false;
}

PxFilterFlags SampleLargeWorld::filter(	PxFilterObjectAttributes	attributes0, 
										PxFilterData				filterData0, 
										PxFilterObjectAttributes	attributes1, 
										PxFilterData				filterData1,
										PxPairFlags&				pairFlags,
										const void*					constantBlock,
										PxU32						constantBlockSize)
{
	PxFilterFlags filterFlags = PxDefaultSimulationFilterShader(attributes0, 
			filterData0, attributes1, filterData1, pairFlags, constantBlock, constantBlockSize);

	if (isCCDActive(filterData0) && isCCDActive(filterData1))
	{
		pairFlags |= PxPairFlag::eSOLVE_CONTACT;
		pairFlags |= PxPairFlag::eDETECT_CCD_CONTACT;
	}

	return filterFlags;
}


///////////////////////////////////////////////////////////////////////////////

#if PX_INTEL_FAMILY
static const bool gFlip = false;
#elif PX_PPC
static const bool gFlip = true;
#elif PX_ARM_FAMILY
static const bool gFlip = false;
#else
#error Unknown platform!
#endif

static PX_INLINE void Flip(PxU32& v)
{
	PxU8* b = (PxU8*)&v;

	PxU8 temp = b[0];
	b[0] = b[3];
	b[3] = temp;
	temp = b[1];
	b[1] = b[2];
	b[2] = temp;
}

static PX_INLINE void Flip(PxF32& v)
{
	Flip((PxU32&)v);
}

class PxBinFile: public PxDefaultFileInputData
{
public:
	PxBinFile(const char* name):PxDefaultFileInputData(name){}
	~PxBinFile(){}

	PX_FORCE_INLINE PxU32 LoadDword(){ PxU32 t; read(&t, sizeof(PxU32)); if(gFlip) Flip(t); return t;}
	PX_FORCE_INLINE PxF32 LoadFloat(){ PxF32 t; read(&t, sizeof(PxF32)); if(gFlip) Flip(t); return t;}
};

void SampleLargeWorld::BinData::serialize( SampleLargeWorld* parent, const char* terrainFile)
{
	PxBinFile binFile(terrainFile);

	if( !binFile.isValid())
	{
		char errMsg[256];
		Ps::snprintf(errMsg, 256, "Couldn't load %s\n", terrainFile);
		parent->fatalError(errMsg);

		return;
	}

	PxU32 nbMeshes = binFile.LoadDword();

	//Suppose the terrain is N*N
	mDim = (CoordType)PxSqrt((PxF32)nbMeshes);	

	PxU32 totalNbTris = 0;
	PxU32 totalNbVerts = 0;

	mTerrainVertices.resize( nbMeshes );
	mTerrainIndices.resize( nbMeshes );

	for(PxU32 i = 0; i < nbMeshes; ++i)
	{
		binFile.LoadDword();
		binFile.LoadDword();

		const PxU32 nbVerts = binFile.LoadDword();
		const PxU32 nbFaces = binFile.LoadDword();

		totalNbTris += nbFaces;
		totalNbVerts += nbVerts;
	
		SampleArray<PxVec3>& vertices = mTerrainVertices[i];
		vertices.resize(nbVerts);

		for(PxU32 j = 0; j < nbVerts; ++j)
		{
			vertices[j].x = binFile.LoadFloat();
			vertices[j].y = binFile.LoadFloat();
			vertices[j].z = binFile.LoadFloat();
		}

		SampleArray<PxU32>& indices = mTerrainIndices[i];
		indices.resize(nbFaces * 3);

		for(PxU32 j = 0; j < nbFaces * 3; ++j)
		{
			indices[j] = binFile.LoadDword();
		}
	}	
}

//Hardcoded terrain.bin terrain. 
void SampleLargeWorld::onInit()
{
	PhysXSample::onInit();
	mCameraController.setCameraSpeed( 2.0f );

	mStringTable = &PxStringTableExt::createStringTable( Ps::getAllocator() );

	mApplication.setMouseCursorHiding(true);
	mApplication.setMouseCursorRecentering(true);

	//Disable collision between differnt groups
	PxSetGroupCollisionFlag(DEFAULT_COLLISION_GROUP,FAN_COLLISION_GROUP,false);
	PxSetGroupCollisionFlag(DEFAULT_COLLISION_GROUP,PICKING_COLLISION_GROUP,false);
	PxSetGroupCollisionFlag(FAN_COLLISION_GROUP,PICKING_COLLISION_GROUP,false);
	
	mBGLoader = SAMPLE_NEW(BackgroundLoader)(*this, N1, N2-1, CHUNK_WIDTH);
	
	const char* terrainFile = getSampleMediaFilename("Terrain.bin"); 
	binData.serialize(this, terrainFile);
	
	//Load the skydome.
	importRAWFile("sky_mission_race1.raw", 2.0f);

	//Load the terrain material
	{
		RAWTexture data;
		data.mName = "SampleLargeWorld_simplegrass.bmp";
		RenderTexture* grassTexture = createRenderTextureFromRawTexture(data);
		RenderMaterial* roadGravelMaterial = SAMPLE_NEW(RenderMaterial)(*getRenderer(), 
			PxVec3(1.0f, 1.0f, 1.0f), 1.0f, false, MATERIAL_ROAD_GRASS, grassTexture);
		mRenderMaterials.push_back(roadGravelMaterial);
	}

	{
		RAWTexture data;
		data.mName = "SampleLargeWorld_simplecity.bmp";
		RenderTexture* bricksTexture = createRenderTextureFromRawTexture(data);
		RenderMaterial* bricksMaterial = SAMPLE_NEW(RenderMaterial)(*getRenderer(), 
			PxVec3(1.0f, 1.0f, 1.0f), 1.0f, false, MATERIAL_BUILDING, bricksTexture);
		mRenderMaterials.push_back(bricksMaterial);
	}	
	{
		RAWTexture data;
		data.mName = "SampleLargeWorld_simplefence.bmp";
		RenderTexture* fencingTexture = createRenderTextureFromRawTexture(data);
		RenderMaterial* fencingMaterial = SAMPLE_NEW(RenderMaterial)(*getRenderer(), 
			PxVec3(1.0f, 1.0f, 1.0f), 1.0f, false, MATERIAL_FARM, fencingTexture);
		mRenderMaterials.push_back(fencingMaterial);
	}	
	{
		RAWTexture data;
		data.mName = "SampleLargeWorld_simpletree.bmp";
		RenderTexture* treeTexture = createRenderTextureFromRawTexture(data);
		RenderMaterial* treeMaterial = SAMPLE_NEW(RenderMaterial)(*getRenderer(), 
			PxVec3(1.0f, 1.0f, 1.0f), 1.0f, false, MATERIAL_TREE, treeTexture);
		mRenderMaterials.push_back(treeMaterial);
	}	
	//Set up the fog.
	getRenderer()->setFog(SampleRenderer::RendererColor(40,40,40), 225.0f);

	mControllerManager = PxCreateControllerManager(getActiveScene());
	mControllerManager->setOverlapRecoveryModule(false);

	ControlledActorDesc desc;
	desc.mType					= PxControllerShapeType::eCAPSULE;
	desc.mPosition				= mControllerInitialPosition;
	desc.mSlopeLimit			= gSlopeLimit;
	desc.mContactOffset			= gContactOffset;
	desc.mStepOffset			= gStepOffset;
	desc.mInvisibleWallHeight	= gInvisibleWallsHeight;
	desc.mMaxJumpHeight			= gMaxJumpHeight;
	desc.mRadius				= gControllerRadius;
	desc.mHeight				= gStandingSize;
	desc.mCrouchHeight			= gCrouchingSize;	
	desc.mReportCallback		= this;
	desc.mBehaviorCallback		= this;
	{
		mActor = SAMPLE_NEW(ControlledActor)(*this);
		
		{
			PxSceneWriteLock scopedLock(*mScene);
			mActor->init(desc, mControllerManager);
		}

		mCCTCamera = SAMPLE_NEW(SampleCCTCameraController)(*this);
		mCCTCamera->setControlled(&mActor, 0, 1);
		mCCTCamera->setFilterCallback(this);
		mCCTCamera->setGravity(-20.0f);
		
		setCameraController(mCCTCamera);	

		mCCTCamera->setView(0,0);
		mCCTCamera->setCameraMaxSpeed( CAMERA_MAX_SPEED );
		mCCTCamera->enableCCT(false);
	}
	
	mWorldBound.minimum = PxVec3(-128.0f,-256.0f, -128.0f);
	mWorldBound.maximum = PxVec3(-128.0f + 256.0f*N2,256.0f, -128.0f + 256.0f*N2);
}

void SampleLargeWorld::onShutdown()
{
	mCameraController.setCameraSpeed(mDefaultCameraSpeed);
	
	if(isPaused())
	{
		togglePause();
	}

	mScene->fetchResults(true);
	DELETESINGLE(mBGLoader);

	{
		PxSceneWriteLock scopedLock(*mScene);

		if( !mAddRenderActor )
		{
			RenderBaseActor* actor = mActor->getRenderActorStanding();
			DELETESINGLE( actor );

			actor = mActor->getRenderActorCrouching();
			DELETESINGLE( actor );
		}

		DELETESINGLE(mCCTCamera);
		mControllerManager->release();
		
		releaseJoints();
	}
	
	if(mStringTable)
	{
		mStringTable->release();
		mStringTable = NULL;
	}

	PhysXSample::onShutdown();
}

void SampleLargeWorld::onTickPreRender(PxF32 dtime)
{
	if(!mPause)
	{
		mBGLoader->onTick();

		//add render actor here, otherwise it has issues on render
		if(!mReadyToSyncCCT)
		{
			mReadyToSyncCCT = readyToSyncCCT();
		}

		if( !mAddRenderActor  && mReadyToSyncCCT )
		{
			RenderBaseActor* actor0 = mActor->getRenderActorStanding();
			RenderBaseActor* actor1 = mActor->getRenderActorCrouching();
			
			if(actor0)
			{
				mRenderActors.push_back(actor0);
			}

			if(actor1)
			{
				mRenderActors.push_back(actor1);	
			}

			mAddRenderActor = true;
		}

		if( mReadyToSyncCCT && !mCCTCamera->getCCTState() )
		{
			mCCTCamera->enableCCT(true);
		}
	}

	for( PxU32 i = 0; i < 2; i++)
	{
		if(mSkybox[i])
			mSkybox[i]->setTransform(PxTransform(getCamera().getPos()));
	}

	PhysXSample::onTickPreRender(dtime);

	if(mIsFlyCamera)
	{
		getDebugRenderer()->addAABB(mWorldBound, RendererColor(0,255,0), RENDER_DEBUG_WIREFRAME);
	}
	
#if ENABLE_PROGRESS_BAR
	mBGLoader->mLoaderStatusLock.lockReader(true);
	mProgressBarRatio = mBGLoader->mQueryProgress;
	mDiskIOTime = mBGLoader->mDiskIOTime;
	mPhysxStreaming = mBGLoader->mPhyXStreamTime;
	mGraphicStreaming = mBGLoader->mGraphicStreamTime;
	mBGLoader->mLoaderStatusLock.unlockReader();
#endif
	
	mProgressBarRatio = PxClamp( mProgressBarRatio, 0.0f, 1.0f);

	if(!mPause && mReadyToSyncCCT)
	{
		mActor->sync();
	}
}

void SampleLargeWorld::onDigitalInputEvent(const SampleFramework::InputEvent& ie, bool val)
{
	if(val)
	{
		switch(ie.m_Id)
		{
			case RETRY:
			{
				PxSceneWriteLock scopedLock(*mScene);
				mActor->reset();
				mCCTCamera->setView(0,0);
			}
			break;

		case FLY_CAMERA:
			{
				toggleFlyCamera();
				mReadyToSyncCCT = false;
				mPause = !mPause;
				if( mPause )
				{
					mCCTCamera->enableCCT(false);
				}

			}
			break;
		case THROW_IMPORTANTOBJECT:
		case THROW_UNIMPORTANCTOBJECT:
			{
				const PxVec3 pos = getCamera().getPos();
				const PxVec3 vel = getCamera().getViewDir() * getDebugObjectsVelocity();
				
				bool isImportant = ie.m_Id == THROW_IMPORTANTOBJECT;
				
				PxSceneWriteLock scopedLock(*mScene);

				PxRigidDynamic* actor = createBox(pos, getDebugBoxObjectExtents(), &vel, 
					isImportant ? mManagedMaterials[MATERIAL_RED] : mManagedMaterials[MATERIAL_GREEN], mDefaultDensity); 
				
				actor->setName(gDynamic);

				mBGLoader->addDynamicObject(actor, isImportant);
			}
			break;
		
		case PICK_NEARSETOBJECT:
			{
				mPick = !mPick;
				if(mPick)
				{
					attachNearestObjectsToCCT();
				}
				else
				{
					releaseJoints();
				}
			}
			break;
		case CAMERA_SHIFT_SPEED:	
		{
			mKeyShiftDown = true;
			break;
		}
		case CAMERA_SPEED_INCREASE:
			{
				PxReal speed = getCurrentCameraController()->getCameraSpeed();
				if(speed * 2 >= CAMERA_MAX_SPEED)
				{
					if(mKeyShiftDown)
						shdfnd::printFormatted("running speed exceeds the maximum speed\n");
					else
						shdfnd::printFormatted("walking speed exceeds the maximum speed\n");
					return;
				}
			}
			break;
		default:
			break;
		}
	}
	else
	{
		if(ie.m_Id == CAMERA_SHIFT_SPEED)	mKeyShiftDown = false;
	}

	PhysXSample::onDigitalInputEvent(ie,val);
}



///////////////////////////////////////////////////////////////////////////////
void SampleLargeWorld::customizeSceneDesc(PxSceneDesc& desc)
{
	desc.flags			|= PxSceneFlag::eENABLE_CCD;
	desc.flags			|= PxSceneFlag::eREQUIRE_RW_LOCK;
	desc.filterShader	= filter;
}

///////////////////////////////////////////////////////////////////////////////

void SampleLargeWorld::customizeSample(SampleSetup& setup)
{
	setup.mName	= "SampleLargeWorld";
}

void SampleLargeWorld::onSubstep(PxF32 dtime)
{
	PhysXSample::onSubstep(dtime);

	mBGLoader->updateChunk(getCamera().getPos());

	PxExtendedVec3 curCCTPosition = mActor->getController()->getPosition();
	if(mWorldBound.contains(toVec3(curCCTPosition)))
	{
		mLastCCTPosition = curCCTPosition;
	}
	else
	{
		PxSceneWriteLock scopedLock(*mScene);
		mActor->getController()->setPosition(mLastCCTPosition);
	}
}

void SampleLargeWorld::collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents)
{
	PhysXSample::collectInputEvents(inputEvents);

	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(STEP_ONE_FRAME);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(PAUSE_SAMPLE);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(SPAWN_DEBUG_OBJECT);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(CAMERA_JUMP);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(CAMERA_CONTROLLER_INCREASE);
	
	DIGITAL_INPUT_EVENT_DEF(CAMERA_SPEED_INCREASE,		WKEY_ADD,					OSXKEY_ADD,					LINUXKEY_ADD		);
	DIGITAL_INPUT_EVENT_DEF(CAMERA_SPEED_DECREASE,		WKEY_SUBTRACT,				OSXKEY_SUBTRACT,			LINUXKEY_SUBTRACT	);
														
	DIGITAL_INPUT_EVENT_DEF(THROW_IMPORTANTOBJECT,		WKEY_I,						OSXKEY_I,					LINUXKEY_I			);
	DIGITAL_INPUT_EVENT_DEF(THROW_UNIMPORTANCTOBJECT,	WKEY_U,						OSXKEY_U,					LINUXKEY_U			);
	DIGITAL_INPUT_EVENT_DEF(PICK_NEARSETOBJECT,			WKEY_E,						OSXKEY_E,					LINUXKEY_E			);
														
	DIGITAL_INPUT_EVENT_DEF(FLY_CAMERA,					WKEY_M,						OSXKEY_M,					LINUXKEY_M			);
	DIGITAL_INPUT_EVENT_DEF(FLY_CAMERA,					GAMEPAD_WEST,       		OSXKEY_UNKNOWN,				LINUXKEY_UNKNOWN	);
														
	DIGITAL_INPUT_EVENT_DEF(RETRY,						WKEY_R,						OSXKEY_R,					LINUXKEY_R			);
	
	DIGITAL_INPUT_EVENT_DEF(CAMERA_SPEED_INCREASE,		GAMEPAD_RIGHT_SHOULDER_TOP,	GAMEPAD_RIGHT_SHOULDER_TOP,	LINUXKEY_UNKNOWN	);
	DIGITAL_INPUT_EVENT_DEF(CAMERA_SPEED_DECREASE,		GAMEPAD_LEFT_SHOULDER_TOP,	GAMEPAD_LEFT_SHOULDER_TOP,	LINUXKEY_UNKNOWN	);
}

void SampleLargeWorld::helpRender(PxU32 x, PxU32 y, PxU8 textAlpha)
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
	msg = mApplication.inputInfoMsg("Press "," to throw an object", SPAWN_DEBUG_OBJECT, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);

	msg = mApplication.inputInfoMsg("Press "," to toggle fly camera", FLY_CAMERA, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);

	msg = mApplication.inputInfoMsg("Press "," to increase fly camera speed", CAMERA_SPEED_INCREASE, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);

	msg = mApplication.inputInfoMsg("Press "," to decrease fly camera speed", CAMERA_SPEED_DECREASE, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);

	msg = mApplication.inputInfoMsg("Press ","  to reset CCT position", RETRY, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);

	msg = mApplication.inputInfoMsg("Press ","  to throw an important object", THROW_IMPORTANTOBJECT, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);

	msg = mApplication.inputInfoMsg("Press ","  to throw an unimportant object", THROW_UNIMPORTANCTOBJECT, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
	
	msg = mApplication.inputInfoMsg("Press ","  to pick up the nearest objects maximum=5, or release objects", PICK_NEARSETOBJECT, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
}

void SampleLargeWorld::descriptionRender(PxU32 x, PxU32 y, PxU8 textAlpha)
{
	bool print=(textAlpha!=0.0f);

	if(print)
	{
		Renderer* renderer = getRenderer();
		const PxU32 yInc = 24;
		const PxReal scale = 0.5f;
		const PxReal shadowOffset = 6.0f;
		const RendererColor textColor(255, 255, 255, textAlpha);

		char line0[256]="This sample demonstrates the PhysX streaming feature. It can load";
		char line1[256]="and unload parts of map procedurally, treats it as an infinite map.";
		char line2[256]="We serialize/deserialize files in the background thread, to avoid";  
		char line3[256]="spikes in the framerate when your HD is lagging or something. We add";
		char line4[256]="one farm, one city, some trees and one fortress for gameplay activity.";

		renderer->print(x, y+=yInc, line0, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line1, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line2, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line3, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line4, scale, shadowOffset, textColor);
	}
}

void SampleLargeWorld::customizeRender()
{
	PhysXSample::customizeRender();

	renderProgressBar( mProgressBarRatio, false );
	Renderer* renderer = getRenderer();
	if(!renderer) return;

	//ScopedRender renderSection(*renderer);
	//if(renderSection)
	{
		const PxU32 yInc = 18;	
		char textBuf[256];

		Camera& camera = getCamera();	

		Ps::snprintf(textBuf, 256, "Disk IO\t\t\t%0.1f ms", mDiskIOTime*1000.0f);	
		renderer->print(10, camera.getScreenHeight() - yInc*6, textBuf);

		Ps::snprintf(textBuf, 256, "PhysX Streaming   %0.1f ms", mPhysxStreaming*1000.0f);	
		renderer->print(10, camera.getScreenHeight() - yInc*5, textBuf);

		Ps::snprintf(textBuf, 256, "Graphic Streaming %0.1f ms", mGraphicStreaming*1000.0f);	
		renderer->print(10, camera.getScreenHeight() - yInc*4, textBuf);

		PxVec3 cameraPos = camera.getPos();
		Ps::snprintf(textBuf, 256, "Position x=%0.1f,z=%0.1f", cameraPos.x, cameraPos.z);	
		renderer->print(camera.getScreenWidth() - 300, 10, textBuf);
	}
}

//Sky box should not be culled
void SampleLargeWorld::newMesh(const RAWMesh& data)
{
	static PxU32 submeshCount = 0;
	if(submeshCount + 1<= (sizeof(mSkybox)/sizeof(mSkybox[0])))
	{
		mSkybox[submeshCount] = createRenderMeshFromRawMesh(data);
		submeshCount++;
	}
}

RAWMesh* SampleLargeWorld::createRawMeshFromMeshGeom(const PxTriangleMeshGeometry& mesh, RAWMesh &rawMesh)
{
	// Get physics geometry
	PxTriangleMesh* tm = mesh.triangleMesh;

	const PxU32 nbVerts					= tm->getNbVertices();
	const PxU32 nbTris					= tm->getNbTriangles();
	const void* tris					= tm->getTriangles();
	const bool has16BitTriangleIndices	= tm->getTriangleMeshFlags() & PxTriangleMeshFlag::e16_BIT_INDICES ? true : false;
	PX_ASSERT(has16BitTriangleIndices);

	PxVec3* verts	 = (PxVec3*)SAMPLE_ALLOC(sizeof(PxVec3)*nbVerts); 
	PxMemCopy( verts, tm->getVertices(), sizeof(PxVec3)*nbVerts);

	PxReal* normals	 = (PxReal*)SAMPLE_ALLOC(sizeof(PxReal)*nbVerts*3); 
	PxU32* indices32 = NULL;

	if(has16BitTriangleIndices)
	{
		PxU16* indices16 = (PxU16*)tris;
		indices32 = (PxU32*)SAMPLE_ALLOC(sizeof(PxU32)*nbTris*3);
		for( PxU32 i = 0; i < nbTris; i++)
		{
			indices32[i*3+0] = indices16[i*3+0];
			indices32[i*3+1] = indices16[i*3+1];
			indices32[i*3+2] = indices16[i*3+2];
		}

		PxBuildSmoothNormals(nbTris, nbVerts, verts, NULL, indices16, (PxVec3*)normals, true);
	}
	else
	{
		indices32 = (PxU32*)tris;
		PxBuildSmoothNormals(nbTris, nbVerts, verts, indices32, NULL, (PxVec3*)normals, true);
	}

	PxBounds3 bound = mesh.triangleMesh->getLocalBounds();

	PxF32* uv =(PxF32*)SAMPLE_ALLOC(sizeof(PxF32)*nbVerts*2); 
	for(PxU32 i = 0; i < nbVerts; i++)
	{
		uv[i*2+0] = (verts[i].x - bound.minimum.x)/(bound.maximum.x - bound.minimum.x);
		uv[i*2+1] = (verts[i].z - bound.minimum.z)/(bound.maximum.z - bound.minimum.z);
	}

	rawMesh.mNbVerts		= nbVerts;
	rawMesh.mVerts			= verts;
	rawMesh.mNbFaces		= nbTris;
	rawMesh.mIndices		= indices32;
	rawMesh.mVertexNormals	= (PxVec3*)normals;
	rawMesh.mUVs			= uv;
	rawMesh.mMaterialID		= MATERIAL_ROAD_GRASS;

	return &rawMesh;
}

RAWMesh* SampleLargeWorld::createRAWMeshFromObjMesh( const char* inObjFileName, const PxTransform& inPos, PxU32 inMaterialID, RAWMesh &outRawMesh )
{
	const ObjMeshMap::Entry* entry = mRenderMeshCache.find(inObjFileName);
	if( entry != NULL )
	{
		cloneMesh( entry->second, outRawMesh );
	} 
	else
	{
		WavefrontObj wfo;
		if (!wfo.loadObj(getSampleMediaFilename(inObjFileName), true))
			return NULL;

		PxU32 nbTris = wfo.mTriCount;
		PxU32 nbVerts = wfo.mVertexCount;
		
		PxVec3*	verts = (PxVec3*)SAMPLE_ALLOC(sizeof(PxVec3)*nbVerts);
		PxMemCopy( verts, wfo.mVertices, sizeof(PxVec3)*nbVerts );

		PxU32* indices = (PxU32*)SAMPLE_ALLOC(sizeof(PxU32)*nbTris*3);
		PxMemCopy( indices, wfo.mIndices, sizeof(PxU32)*nbTris*3 );

		PxReal* uvs = (PxReal*)SAMPLE_ALLOC(sizeof(PxReal)*nbVerts*2);
		PxMemCopy( uvs, wfo.mTexCoords, sizeof(PxReal)*nbVerts*2 );

		PxVec3* normals = (PxVec3*)SAMPLE_ALLOC(sizeof(PxVec3)*nbVerts); 
		PxBuildSmoothNormals(
			nbTris, 
			nbVerts, 
			verts, 
			indices, 
			NULL, 
			normals, 
			true
			);

		outRawMesh.mNbVerts			= nbVerts;
		outRawMesh.mVerts			= verts;
		outRawMesh.mNbFaces			= nbTris;
		outRawMesh.mIndices			= indices;
		outRawMesh.mUVs				= uvs;
		outRawMesh.mVertexNormals	= normals;
		outRawMesh.mMaterialID		= inMaterialID;
		outRawMesh.mTransform		= inPos;

		RAWMesh cacheMesh;
		cloneMesh( outRawMesh, cacheMesh );
		mRenderMeshCache.insert( inObjFileName, cacheMesh );
	}

	return &outRawMesh;
}

void SampleLargeWorld::cloneMesh(const RAWMesh& inSrc, RAWMesh& outDst)
{
	PxVec3* verts = NULL;
	PxU32*	indices = NULL;
	PxReal* uvs = NULL;
	PxVec3* normals = NULL;
	PxVec3* colors = NULL;

	if( inSrc.mVerts )
	{
		verts	= (PxVec3*)SAMPLE_ALLOC(sizeof(PxVec3)*inSrc.mNbVerts);
		PxMemCopy( verts, inSrc.mVerts, sizeof(PxVec3)*inSrc.mNbVerts );
	}

	if( inSrc.mVertexNormals )
	{
		normals = (PxVec3*)SAMPLE_ALLOC(sizeof(PxVec3)*inSrc.mNbVerts); 
		PxMemCopy( normals, inSrc.mVertexNormals, sizeof(PxVec3)*inSrc.mNbVerts );
	}

	if( inSrc.mVertexColors )
	{
		colors = (PxVec3*)SAMPLE_ALLOC(sizeof(PxVec3)*inSrc.mNbVerts); 
		PxMemCopy( colors, inSrc.mVertexColors, sizeof(PxVec3)*inSrc.mNbVerts );
	}

	if( inSrc.mUVs )
	{
		uvs = (PxReal*)SAMPLE_ALLOC(sizeof(PxReal)*inSrc.mNbVerts*2);
		PxMemCopy( uvs, inSrc.mUVs, sizeof(PxReal)*inSrc.mNbVerts*2 );
	}
	
	if( inSrc.mIndices )
	{
		indices	= (PxU32*)SAMPLE_ALLOC(sizeof(PxU32)*inSrc.mNbFaces*3);
		PxMemCopy( indices, inSrc.mIndices, sizeof(PxU32)*inSrc.mNbFaces*3 );
	}

	outDst.mName = inSrc.mName;
	outDst.mTransform = inSrc.mTransform;
	outDst.mNbVerts = inSrc.mNbVerts;
	outDst.mNbFaces = inSrc.mNbFaces;
	outDst.mMaterialID = inSrc.mMaterialID;
	outDst.mVerts = verts;
	outDst.mVertexNormals = normals;
	outDst.mVertexColors = colors;
	outDst.mUVs = uvs;
	outDst.mIndices = indices;
}

void SampleLargeWorld::renderProgressBar( PxF32 ratio, bool needFlush )
{	
	const RendererColor backColor(0, 255, 39);

	ScreenQuad sq;
	sq.mX0				= 0.0f;
	sq.mY0				= 0.8f;
	sq.mX1				= ratio;
	sq.mY1				= 0.85f;
	sq.mLeftUpColor		= backColor;
	sq.mRightUpColor	= backColor;
	sq.mLeftDownColor	= backColor;
	sq.mRightDownColor	= backColor;
	sq.mAlpha			= 1.0f;

	if(needFlush)
		getRenderer()->clearBuffers();

	getRenderer()->drawScreenQuad(sq);

	if(needFlush)
		getRenderer()->swapBuffers();
}

void SampleLargeWorld::releaseJoints()
{
	PxSceneWriteLock scopedLock(*mScene);

	const size_t nbJoints = mFixedJoints.size();

	PxRigidActor* actor0 = NULL;
	PxRigidActor* actor1 = NULL;

	for(PxU32 i=0;i<nbJoints;i++)
	{
		mFixedJoints[i]->getActors(actor0, actor1);
		setCollisionGroup( actor1, DEFAULT_COLLISION_GROUP );

		mFixedJoints[i]->release();
	}
	mFixedJoints.clear();
}

bool SampleLargeWorld::readyToSyncCCT()
{
	PxSceneReadLock scopedLock(*mScene);
	PxVec3 pos = toVec3(mActor->getController()->getPosition());
	
	PxRigidDynamic* actor = mActor->getController()->getActor();
	PxShape* capsuleShape = getShape( *actor );
	
	PxCapsuleGeometry capGeom(capsuleShape->getGeometry().capsule());
	
	PxQueryFilterData objType = PxQueryFilterData(PxQueryFlag::eSTATIC);
	PxHitFlags hitFlag = PxHitFlag::ePOSITION|PxHitFlag::eNORMAL;
	PxSweepBuffer hit;
	return getActiveScene().sweep(capGeom, PxTransform(pos), PxVec3(0.0f, -1.0f,0.0f), 1000.f, hit, hitFlag, objType);
}

void SampleLargeWorld::attachNearestObjectsToCCT()
{
	PxSceneWriteLock scopedLock(*mScene);

	PxRigidDynamic* actor = mActor->getController()->getActor();
	PxSphereGeometry sg(3.f);

	PxI32 attachNum = 5;
	PxVec3 footPos = toVec3(mActor->getController()->getFootPosition());
	
	PxShape* capsuleShape = getShape( *actor );
	
	const PxCapsuleGeometry& capGeom = capsuleShape->getGeometry().capsule();

	PxF32 boxExtent = getDebugBoxObjectExtents().y + 0.1f;

	PxOverlapHit hitBuffer[5];
	PxMemZero(&hitBuffer, sizeof(hitBuffer));
	PxFilterData fd(PICKING_COLLISION_GROUP,0,0,0);
	PxOverlapBuffer buf(hitBuffer, 5);

	getActiveScene().overlap(sg, PxTransform(footPos), buf, PxQueryFilterData(fd, PxQueryFlag::eDYNAMIC));
	PxU32 hitNum = buf.getNbAnyHits();
	if( hitNum == 0 )
		return;
	
	attachNum = hitNum == (PxU32)-1 ? attachNum : hitNum;

	PxTransform		lastTransform = PxTransform(actor->getGlobalPose().q);
	PxVec3			hookPos = PxVec3( 0.0f, capGeom.halfHeight + capGeom.radius +boxExtent*2, capGeom.radius*2.0f);
	
	for( PxI32 i = 0; i < attachNum; ++i)
	{
		PxRigidDynamic* attachedActor = hitBuffer[i].actor->is<PxRigidDynamic>();
		setCollisionGroup( attachedActor, PICKING_COLLISION_GROUP );

		PxFixedJoint *joint = PxFixedJointCreate(getPhysics(), actor,lastTransform, attachedActor, PxTransform(hookPos));
		joint->setConstraintFlag( PxConstraintFlag::eCOLLISION_ENABLED, false );
		mFixedJoints.push_back( joint );

		hookPos.y += (boxExtent*1.5f);
	}
}

PxRigidDynamic* SampleLargeWorld::createBox(const PxVec3& pos, const PxVec3& dims, const PxVec3* linVel, RenderMaterial* material, PxReal density)
{
	PxSceneWriteLock scopedLock(*mScene);

	PxRigidDynamic* box = PxCreateDynamic(*mPhysics, PxTransform(pos), PxBoxGeometry(dims), *mMaterial, density);
	PX_ASSERT(box);

	box->setActorFlag(PxActorFlag::eVISUALIZATION, true);
	box->setAngularDamping(0.5f);
	
	//Enable ccd of dynamic generated box	
	PxShape* boxShape = getShape(*box);
	setCCDActive(*boxShape);

	boxShape->setQueryFilterData(PxFilterData(PICKING_COLLISION_GROUP,0,0,0));

	getActiveScene().addActor(*box);

	if(linVel)
		box->setLinearVelocity(*linVel);

	createRenderObjectsFromActor(box, material);

	return box;
}

void SampleLargeWorld::setCollisionGroup(PxRigidActor* actor, PxU32 group)
{
	PxSceneWriteLock scopedLock(*mScene);

	PxU32 nbShapes = actor->getNbShapes();
	if( nbShapes )
	{
		SampleArray<PxShape*> shapes(nbShapes);
		actor->getShapes( &shapes[0], nbShapes);
		for( PxU32 j = 0; j < nbShapes; j++)
		{
			PxFilterData fd = shapes[j]->getSimulationFilterData();
			fd.word0 = group;
			shapes[j]->setSimulationFilterData(fd);
		}
	}
}
///////////////////////////////////////////////////////////////////////////////

