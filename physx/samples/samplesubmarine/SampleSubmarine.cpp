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

#include "SampleSubmarine.h"
#include "SampleUtils.h"
#include "SampleAllocatorSDKClasses.h"
#include "RenderBaseActor.h"
#include "RendererMemoryMacros.h"
#include "RenderMaterial.h"

#include "PxTkFile.h"
#include "PxPhysicsAPI.h"
#include "extensions/PxExtensionsAPI.h"

#include <SamplePlatform.h>
#include <SampleUserInput.h>
#include <SampleUserInputIds.h>
#include "SampleSubmarineInputEventIds.h"
#include <SampleUserInputDefines.h>

#include "Crab.h"

#include "SubmarineCameraController.h"

// for loading the HeightField
#include "RenderMeshActor.h"
#include "PxTkBmpLoader.h"
//////////////////////////////

#include <algorithm>

using namespace PxToolkit;
using namespace SampleRenderer;
using namespace SampleFramework;

REGISTER_SAMPLE(SampleSubmarine, "SampleSubmarine")

static PxVec3				gBuoyancy = PxVec3(0, 1.0f, 0);
static PxRigidDynamic*		gTreasureActor = NULL;
static PxVec3				gForce = PxVec3(0, 0, 0);
static PxVec3				gTorque = PxVec3(0, 0, 0);
static const PxReal			gLinPower = 200.0f;
static const PxReal			gAngPower = 5000.0f;
static const PxReal			gSubMarineDensity = 3.0f;
static PxI32				gSubmarineHealth = 100;
static PxU32				gKeyFlags = 0;
static bool					gTreasureFound = false;
static bool					gResetScene = false;

static Crab*				gCrab = NULL;

void setupFiltering(PxRigidActor* actor, PxU32 filterGroup, PxU32 filterMask);

struct Movement
{
	enum Enum
	{
		eCRAB_FWD				= (1 << 0),
		eCRAB_BCKWD				= (1 << 1),
		eCRAB_ROTATE_LEFT		= (1 << 2),
		eCRAB_ROTATE_RIGHT		= (1 << 3),
		eSUBMAINE_FWD			= (1 << 4),
		eSUBMAINE_BCKWD			= (1 << 5),
		eSUBMAINE_UP			= (1 << 6),
		eSUBMAINE_DOWN			= (1 << 7),
	};
};


enum MaterialID
{
	MATERIAL_TERRAIN_MUD	 = 1000,
};

///////////////////////////////////////////////////////////////////////////////


SampleSubmarine::SampleSubmarine(PhysXSampleApplication& app)
	: PhysXSample(app)
	, mSubmarineActor(NULL)
	, mCameraAttachedToActor(NULL)
	, mSubmarineCameraController(NULL)
{
	mCreateGroundPlane	= false;
	//mStepperType = FIXED_STEPPER;
}

SampleSubmarine::~SampleSubmarine()
{
}

///////////////////////////////////////////////////////////////////////////////

void SampleSubmarine::customizeSample(SampleSetup& setup)
{
	setup.mName	= "SampleSubmarine";
}

///////////////////////////////////////////////////////////////////////////////

void SampleSubmarine::onInit()
{
	mNbThreads = PxMax(PxI32(shdfnd::Thread::getNbPhysicalCores())-1, 0);

	mCreateCudaCtxManager = true;

	PhysXSample::onInit();

	PxSceneWriteLock scopedLock(*mScene);

	mSubmarineCameraController = SAMPLE_NEW(SubmarineCameraController)();
	setCameraController(mSubmarineCameraController);

	mApplication.setMouseCursorHiding(true);
	mApplication.setMouseCursorRecentering(true);
	mSubmarineCameraController->init(PxTransform(PxIdentity));
	mSubmarineCameraController->setMouseSensitivity(0.5f);
	mSubmarineCameraController->setMouseLookOnMouseButton(false);

	getRenderer()->setAmbientColor(RendererColor(60, 60, 60));

	PxMaterial& material = getDefaultMaterial();
	material.setRestitution(0);
	material.setDynamicFriction(50.0f);
	material.setStaticFriction(50.0f);

	// set gravity
	getActiveScene().setGravity(PxVec3(0, -10.0f, 0));

	createMaterials();

	getRenderer()->setFog(SampleRenderer::RendererColor(16,16,40), 125.0f);

	PxRigidActor* heightField = loadTerrain("submarine_heightmap.bmp", 0.4f, 3.0f, 3.0f);
	if (!heightField) fatalError("Sample can not load file submarine_heightmap.bmp\n");

	setupFiltering(heightField, FilterGroup::eHEIGHTFIELD, FilterGroup::eCRAB);

	// create ceiling plane
	PxReal d = 60.0f;
	PxTransform pose = PxTransform(PxVec3(0.0f, d, 0.0f), PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, -1.0f)));
	PxRigidStatic* plane = getPhysics().createRigidStatic(pose);
	if(!plane) fatalError("createRigidStatic failed!");
	PxShape* shape = PxRigidActorExt::createExclusiveShape(*plane, PxPlaneGeometry(), material);
	if(!shape) fatalError("createShape failed!");
	getActiveScene().addActor(*plane);

	resetScene();
}

void SampleSubmarine::createMaterials()
{
	RAWTexture data;
	data.mName = "rock_diffuse2.dds";
	RenderTexture* gravelTexture = createRenderTextureFromRawTexture(data);

	RenderMaterial* terrainMaterial = SAMPLE_NEW(RenderMaterial)(*getRenderer(), PxVec3(0.5f, 0.25f, 0.125f), 1.0f, false, MATERIAL_TERRAIN_MUD, gravelTexture);
	mRenderMaterials.push_back(terrainMaterial);

	mSeamineMaterial = SAMPLE_NEW(RenderMaterial)(*getRenderer(),  PxVec3(0.50f, 0.50f, 0.5f), 1.0f, false, 0xffffffff, NULL);
	mRenderMaterials.push_back(mSeamineMaterial);
}


PxRigidActor* SampleSubmarine::loadTerrain(const char* name, const PxReal heightScale, const PxReal rowScale, const PxReal columnScale) 
{
	PxRigidActor* heightFieldActor = NULL;
	BmpLoader loader;
	if(loader.loadBmp(getSampleMediaFilename(name))) 
	{
		PxU16 nbColumns = PxU16(loader.mWidth), nbRows = PxU16(loader.mHeight);
		PxHeightFieldDesc heightFieldDesc;
		heightFieldDesc.nbColumns = nbColumns;
		heightFieldDesc.nbRows = nbRows;
		PxU32* samplesData = (PxU32*)SAMPLE_ALLOC(sizeof(PxU32)*nbColumns * nbRows);
		heightFieldDesc.samples.data = samplesData;
		heightFieldDesc.samples.stride = sizeof(PxU32);
		PxU8* currentByte = (PxU8*)heightFieldDesc.samples.data;
		PxU8* loader_ptr = loader.mRGB;
		PxVec3Alloc* vertexesA = SAMPLE_NEW(PxVec3Alloc)[nbRows * nbColumns];
		PxF32* uvs = (PxF32*)SAMPLE_ALLOC(sizeof(PxF32) * nbRows * nbColumns * 2);
		PxVec3* vertexes = vertexesA;
		for (PxU32 row = 0; row < nbRows; row++) 
		{
			for (PxU32 column = 0; column < nbColumns; column++) 
			{
				PxHeightFieldSample* currentSample = (PxHeightFieldSample*)currentByte;
				currentSample->height = *loader_ptr;
				vertexes[row * nbColumns + column] = PxVec3(PxReal(row)*rowScale, 
					PxReal(currentSample->height * heightScale), 
					PxReal(column)*columnScale);

				uvs[(row * nbColumns + column)*2 + 0] = (float)column/7.0f;
				uvs[(row * nbColumns + column)*2 + 1] = (float)row/7.0f;

				currentSample->materialIndex0 = 0;
				currentSample->materialIndex1 = 0;
				currentSample->clearTessFlag();
				currentByte += heightFieldDesc.samples.stride;
				loader_ptr += 3 * sizeof(PxU8);
			}
		}
		PxHeightField* heightField = getCooking().createHeightField(heightFieldDesc, getPhysics().getPhysicsInsertionCallback());
		if(!heightField) fatalError("createHeightField failed!");
		// create shape for heightfield		
		PxTransform pose(PxVec3(-((PxReal)nbRows*rowScale) / 2.0f, 
			0.0f, 
			-((PxReal)nbColumns*columnScale) / 2.0f), 
			PxQuat(PxIdentity));
		heightFieldActor = getPhysics().createRigidStatic(pose);
		if(!heightFieldActor) fatalError("createRigidStatic failed!");
		PxShape* shape = PxRigidActorExt::createExclusiveShape(*heightFieldActor, PxHeightFieldGeometry(heightField, PxMeshGeometryFlags(), heightScale, rowScale, columnScale), getDefaultMaterial());
		if(!shape) fatalError("createShape failed!");
		// add actor to the scene
		getActiveScene().addActor(*heightFieldActor);
		// create indices
		PxU32* indices = (PxU32*)SAMPLE_ALLOC(sizeof(PxU32)*((nbColumns - 1) * (nbRows - 1) * 3 * 2));
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
		data.mName = name;
		data.mTransform = PxTransform(PxIdentity);
		data.mNbVerts = nbColumns * nbRows;
		data.mVerts = vertexes;
		data.mVertexNormals = NULL;
		data.mUVs = uvs;
		data.mMaterialID = MATERIAL_TERRAIN_MUD;
		data.mNbFaces = (nbColumns - 1) * (nbRows - 1) * 2;
		data.mIndices = indices;

		RenderMeshActor* hf_mesh = createRenderMeshFromRawMesh(data);
		if(!hf_mesh) fatalError("createRenderMeshFromRawMesh failed!");
		hf_mesh->setPhysicsShape(shape, heightFieldActor);
		shape->setFlag(PxShapeFlag::eVISUALIZATION, false);
		SAMPLE_FREE(indices);
		SAMPLE_FREE(uvs);
		DELETEARRAY(vertexesA);
		SAMPLE_FREE(samplesData);
	}

	return heightFieldActor;
}

///////////////////////////////////////////////////////////////////////////////

void setupFiltering(PxRigidActor* actor, PxU32 filterGroup, PxU32 filterMask)
{
	PxFilterData filterData;
	filterData.word0 = filterGroup; // word0 = own ID
	filterData.word1 = filterMask;	// word1 = ID mask to filter pairs that trigger a contact callback;
	const PxU32 numShapes = actor->getNbShapes();
	PxShape** shapes = (PxShape**)SAMPLE_ALLOC(sizeof(PxShape*)*numShapes);
	actor->getShapes(shapes, numShapes);
	for(PxU32 i = 0; i < numShapes; i++)
	{
		PxShape* shape = shapes[i];
		shape->setSimulationFilterData(filterData);
	}
	SAMPLE_FREE(shapes);
}

///////////////////////////////////////////////////////////////////////////////

PxRigidDynamic* SampleSubmarine::createSubmarine(const PxVec3& inPosition, const PxReal yRot)
{
	PX_ASSERT(mSubmarineActor == NULL);

	std::vector<PxTransform> localPoses;
	std::vector<const PxGeometry*> geometries;

	// cabin
	PxSphereGeometry cabinGeom(1.5f);
	PxTransform	cabinPose = PxTransform(PxIdentity); 
	cabinPose.p.x = -0.5f;

	// engine
	PxBoxGeometry engineGeom(0.25f, 1.0f, 1.0f);
	PxTransform	enginePose = PxTransform(PxIdentity); 
	enginePose.p.x = cabinPose.p.x + cabinGeom.radius + engineGeom.halfExtents.x;

	// tanks
	PxCapsuleGeometry tankGeom(0.5f, 1.8f);
	PxTransform	tank1Pose = PxTransform(PxIdentity); 
	tank1Pose.p = PxVec3(0,-cabinGeom.radius, cabinGeom.radius);
	PxTransform	tank2Pose = PxTransform(PxIdentity); 
	tank2Pose.p = PxVec3(0,-cabinGeom.radius, -cabinGeom.radius);

	localPoses.push_back(cabinPose);
	geometries.push_back(&cabinGeom);
	localPoses.push_back(enginePose);
	geometries.push_back(&engineGeom);
	localPoses.push_back(tank1Pose);
	geometries.push_back(&tankGeom);
	localPoses.push_back(tank2Pose);
	geometries.push_back(&tankGeom);

	// put the shapes together into one actor
	mSubmarineActor = PhysXSample::createCompound(inPosition, localPoses, geometries, 0, mManagedMaterials[MATERIAL_YELLOW], gSubMarineDensity)->is<PxRigidDynamic>();
	
	if(!mSubmarineActor) fatalError("createCompound failed!");

	//disable the current and buoyancy effect for the sub.
	mSubmarineActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);

	// set the filtering group for the submarine
	setupFiltering(mSubmarineActor, FilterGroup::eSUBMARINE, FilterGroup::eMINE_HEAD | FilterGroup::eMINE_LINK);

	mSubmarineActor->setLinearDamping(0.15f);
	mSubmarineActor->setAngularDamping(15.0f);

	PxTransform globalPose; 
	globalPose.p = inPosition;
	globalPose.q = PxQuat(yRot, PxVec3(0,1,0));
	mSubmarineActor->setGlobalPose(globalPose);

	mSubmarineActor->setCMassLocalPose(PxTransform(PxIdentity));

	return mSubmarineActor;
}

///////////////////////////////////////////////////////////////////////////////

Seamine* SampleSubmarine::createSeamine(const PxVec3& inPosition, PxReal inHeight)
{
	static const PxReal chainLinkLength = 2.0f;
	static const PxReal linkSpacing = 0.05f;
	static const PxReal mineHeadRadius = 1.5f;
	const PxVec3 mineStartPos = inPosition;
	static const PxVec3 linkOffset = PxVec3(0, chainLinkLength + linkSpacing, 0);
	static const PxVec3 halfLinkOffset = linkOffset * 0.5f;
	static const PxVec3 linkDim = PxVec3(chainLinkLength*0.125f, chainLinkLength*0.5f, chainLinkLength*0.125f);
	PxU32 numLinks = PxU32((inHeight - 2.0f*mineHeadRadius) / (chainLinkLength + linkSpacing));
	numLinks = numLinks ? numLinks : 1;

	Seamine* seamine = SAMPLE_NEW(Seamine);
	mSeamines.push_back(seamine);

	// create links from floor
	PxVec3 linkPos = mineStartPos + halfLinkOffset;
	PxRigidActor* prevActor = NULL;
	for(PxU32 i = 0; i < numLinks; i++)
	{
		// create the link actor
		PxRigidDynamic* link = createBox(linkPos, linkDim, NULL, mSeamineMaterial, 1.0f)->is<PxRigidDynamic>();
		if(!link) fatalError("createBox failed!");
		// back reference to mineHead
		link->userData = seamine;
		seamine->mLinks.push_back(link);

		setupFiltering(link, FilterGroup::eMINE_LINK, FilterGroup::eSUBMARINE);
		link->setLinearDamping(0.5f);
		link->setAngularDamping(0.5f);
		link->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);

		// create distance joint between link and prevActor
		PxTransform linkFrameA = prevActor ? PxTransform(halfLinkOffset, PxQuat(PxIdentity)) : PxTransform(mineStartPos, PxQuat(PxIdentity));
		PxTransform linkFrameB = PxTransform(-halfLinkOffset, PxQuat(PxIdentity));
		PxDistanceJoint *joint = PxDistanceJointCreate(getPhysics(), prevActor, linkFrameA, link, linkFrameB);
		if(!joint) fatalError("PxDistanceJointCreate failed!");

		// set min & max distance to 0
		joint->setMaxDistance(0.0f);
		joint->setMinDistance(0.0f);
		// setup damping & spring
		joint->setDamping(1.0f * link->getMass());
		joint->setStiffness(400.0f * link->getMass());
		joint->setDistanceJointFlags(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED | PxDistanceJointFlag::eMIN_DISTANCE_ENABLED | PxDistanceJointFlag::eSPRING_ENABLED);

		// add to joints array for cleanup
		mJoints.push_back(joint);

		prevActor = link;
		linkPos += linkOffset;
	}

	// create mine head
	linkPos.y += mineHeadRadius - (chainLinkLength*0.5f);
	PxRigidDynamic* mineHead = createSphere(linkPos, mineHeadRadius, NULL, mSeamineMaterial, 1.0f)->is<PxRigidDynamic>();
	mineHead->userData = seamine;
	seamine->mMineHead = mineHead;
	
	mineHead->setLinearDamping(0.5f);
	mineHead->setAngularDamping(0.5f);
	mineHead->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
	
	// setup filtering to trigger contact reports when submarine touches the minehead
	setupFiltering(mineHead, FilterGroup::eMINE_HEAD, FilterGroup::eSUBMARINE);


	// create distance joint between mine head and prevActor
	PxTransform linkFrameA = PxTransform(halfLinkOffset, PxQuat(PxIdentity));
	PxTransform linkFrameB = PxTransform(PxVec3(0, -mineHeadRadius - linkSpacing*0.5f, 0), PxQuat(PxIdentity));
	PxDistanceJoint *joint = PxDistanceJointCreate(getPhysics(), prevActor, linkFrameA, mineHead, linkFrameB);
	if(!joint) fatalError("PxDistanceJointCreate failed!");

	// set min & max distance to 0
	joint->setMaxDistance(0.0f);
	joint->setMinDistance(0.0f);
	// setup damping & spring
	joint->setDamping(1.0f * mineHead->getMass());
	joint->setStiffness(400.0f * mineHead->getMass());
	joint->setDistanceJointFlags(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED | PxDistanceJointFlag::eMIN_DISTANCE_ENABLED | PxDistanceJointFlag::eSPRING_ENABLED);

	// add to joints array for cleanup
	mJoints.push_back(joint);

	return seamine;
}

///////////////////////////////////////////////////////////////////////////////
static const char* getPlatformName()
{
#if PX_X86
	return "PC32";
#elif PX_X64
	return "PC64";
#elif PX_ARM_FAMILY
	return "ARM";
#else
	return "";
#endif
}

void SampleSubmarine::createDynamicActors()
{
	// create mines
	static const PxU32 numMines = 20;
	static const PxVec3 mineFieldCenter = PxVec3(0, 64, 0);
	static const PxReal minMineDistance = 3.0f;
	static const PxI32 mineFieldRadius = 30;
	for(PxU32 i = 0; i < numMines; i++)
	{
		// raycast against floor (height field) to find the height to attach the mine
		PxRaycastBuffer rayHit;
		bool hit = false;
		do
		{
			PxVec3 offset = PxVec3(PxReal(getSampleRandom().rand(-mineFieldRadius, mineFieldRadius)), 0, PxReal(getSampleRandom().rand(-mineFieldRadius, mineFieldRadius)));
			PxVec3 raycastStart = mineFieldCenter + offset*minMineDistance;
			hit = getActiveScene().raycast(raycastStart, PxVec3(0,-1,0), 100.0f, rayHit);
		} while(!hit || (rayHit.block.position.y > 25.0f) || rayHit.block.actor->is<PxRigidDynamic>());
		createSeamine(rayHit.block.position, getSampleRandom().rand(10.0f, 25.0f));
	}

	// create treasure
	{
		static const PxVec3 treasureDim = PxVec3(5, 3, 4)*0.5f;

		PxRaycastBuffer rayHit;
		PxVec3 raycastStart = PxVec3(-19, 64, -24);
		getActiveScene().raycast(raycastStart, PxVec3(0,-1,0), 100.0f, rayHit);

		gTreasureActor = createBox(rayHit.block.position+treasureDim, treasureDim, NULL, mManagedMaterials[MATERIAL_BLUE], 1)->is<PxRigidDynamic>();

		if(!gTreasureActor) fatalError("createBox failed!");
		gTreasureActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
		PxShape* treasureShape;
		gTreasureActor->getShapes(&treasureShape, 1);
		treasureShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
		treasureShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
	}

	// create submarine
	createSubmarine(PxVec3(-110, 50, 110), 2.1f);
	
	char theCrabName[256];
	sprintf(theCrabName, "crab_%s.bin", getPlatformName());
	// If we have already had crab copy, just load it, or will create crab and export it
	char thePathBuffer[1024];
	const char* theCrabPath = getSampleOutputDirManager().getFilePath( theCrabName, thePathBuffer, false );
	SampleFramework::File* fp = NULL;
	PxToolkit::fopen_s(&fp, theCrabPath, "r" );
	if( fp )
	{
		shdfnd::printFormatted("loading the crab from file status: \n");
		gCrab = SAMPLE_NEW(Crab)(*this, theCrabPath, mManagedMaterials[MATERIAL_RED]);
		if (gCrab && !gCrab->getCrabBody())
		{
			delete gCrab;
			gCrab = NULL;
		}
		shdfnd::printFormatted(gCrab ? "successful\n":"failed\n");						
		fclose (fp); 
	}

	if( !gCrab )
	{
		gCrab = SAMPLE_NEW(Crab)(*this, PxVec3(0, 50, 0), mManagedMaterials[MATERIAL_RED]);
		shdfnd::printFormatted("crab file not found ... exporting crab file\n");
		gCrab->save(theCrabPath);
	}
	
	PX_ASSERT( gCrab );
	mCrabs.push_back(gCrab);

	static const PxU32 numCrabs = 3;
	for(PxU32 i = 0; i < numCrabs; i++)
	{
		Crab* crab = SAMPLE_NEW(Crab)(*this, PxVec3(getSampleRandom().rand(-20.0f,20.0f), 50, getSampleRandom().rand(-20.0f,20.0f)), mManagedMaterials[MATERIAL_RED]);
		mCrabs.push_back(crab);
	}
}

///////////////////////////////////////////////////////////////////////////////

void SampleSubmarine::resetScene()
{
	gResetScene = false;
	const size_t nbCrabs = mCrabs.size();
	for(PxU32 i=0;i<nbCrabs;i++)
	{
		delete mCrabs[i];
	}
	mCrabs.clear();

	const size_t nbJoints = mJoints.size();
	for(PxU32 i=0;i<nbJoints;i++)
		mJoints[i]->release();
	mJoints.clear();

	const size_t nbSeamines = mSeamines.size();
	for(PxU32 i=0;i<nbSeamines;i++)
	{
		const size_t nbLink = mSeamines[i]->mLinks.size();
		for(PxU32 j=0;j<nbLink;j++)
			removeActor(mSeamines[i]->mLinks[j]);
		removeActor(mSeamines[i]->mMineHead);
		delete mSeamines[i];
	}
	mSeamines.clear();

	if(mSubmarineActor)
	{
		removeActor(mSubmarineActor);
		mSubmarineActor = NULL;
	}
	gSubmarineHealth = 100;

	if (gTreasureActor)
		removeActor(gTreasureActor);
	gTreasureActor = NULL;
	gTreasureFound = false;

	while(mPhysicsActors.size())
	{
		removeActor(mPhysicsActors[0]);
	}

	freeDeletedActors();

	createDynamicActors();

	// init camera orientation
	mSubmarineCameraController->init(PxVec3(-90, 60, 50), PxVec3(0.28f, 2.05f, 0.0f));
	mSubmarineCameraController->setMouseSensitivity(0.5f);
	mSubmarineCameraController->setFollowingMode(true);
	mCameraAttachedToActor = mSubmarineActor;
}

///////////////////////////////////////////////////////////////////////////////

void SampleSubmarine::onShutdown()
{
	{
		PxSceneWriteLock scopedLock(*mScene);

		const size_t nbCrabs = mCrabs.size();
		for(PxU32 i=0;i<nbCrabs;i++)
			delete mCrabs[i];
		mCrabs.clear();

		// free crabs' memory
		const size_t nbDelCrabsMem = mCrabsMemoryDeleteList.size();
		for(PxU32 i = 0; i < nbDelCrabsMem; i++)
			SAMPLE_FREE(mCrabsMemoryDeleteList[i]);
		mCrabsMemoryDeleteList.clear();

		gCrab = NULL;

		const size_t nbJoints = mJoints.size();
		for(PxU32 i=0;i<nbJoints;i++)
			mJoints[i]->release();
		mJoints.clear();

		const size_t nbSeamines = mSeamines.size();
		for(PxU32 i=0;i<nbSeamines;i++)
			delete mSeamines[i];
		mSeamines.clear();

		gTreasureActor = NULL;

		DELETESINGLE(mSubmarineCameraController);
	}
	
	PhysXSample::onShutdown();
}

///////////////////////////////////////////////////////////////////////////////

void SampleSubmarine::onSubstep(float dtime)
{
	// user input -> forces
	handleInput();

	// delay free crabs' memory
	const size_t nbDelCrabsMem = mCrabsMemoryDeleteList.size();
	for(PxU32 i = 0; i < nbDelCrabsMem; i++)
		SAMPLE_FREE(mCrabsMemoryDeleteList[i]);
	mCrabsMemoryDeleteList.clear();

	// change current every 0.01s
	static PxReal sElapsedTime = 0.0f;
	sElapsedTime += mPause ? 0 : dtime;
	if(sElapsedTime > 0.01f)
	{
		static PxReal angle = 0;
		angle += sElapsedTime*0.01f;
		angle = angle < (PxTwoPi) ? angle : angle - PxTwoPi;
		sElapsedTime = 0;
		
		gBuoyancy.z = 0.15f * PxSin(angle * 50);
		PxQuat yRot = PxQuat(angle, PxVec3(0,1,0));
		gBuoyancy = yRot.rotate(gBuoyancy);

		// apply external forces to seamines
		PxSceneWriteLock scopedLock(*mScene);
		const size_t nbSeamines = mSeamines.size();
		for(PxU32 i = 0; i < nbSeamines; i++)
		{
			Seamine* mine = mSeamines[i];
			mine->mMineHead->addForce(gBuoyancy, PxForceMode::eACCELERATION);
			const size_t nbLinks = mine->mLinks.size();
			for(PxU32 j = 0; j < nbLinks; j++)
			{
				mine->mLinks[j]->addForce(gBuoyancy, PxForceMode::eACCELERATION);
			}
		}
	}

	if(mSubmarineActor)
	{
		PxSceneWriteLock scopedLock(*mScene);

		//convert forces from submarine the submarine's body local space to global space
		PxQuat submarineOrientation = mSubmarineActor->getGlobalPose().q;
		gForce = submarineOrientation.rotate(gForce);
		gTorque = submarineOrientation.rotate(gTorque);

		// add also current forces to submarine
		gForce.z += gBuoyancy.z * 5.0f;

		// apply forces in global space and reset
		mSubmarineActor->addForce(gForce);
		mSubmarineActor->addTorque(gTorque);
		gForce = PxVec3(0);
		gTorque = PxVec3(0);
	}
}

void SampleSubmarine::onSubstepSetup(float dt, PxBaseTask* completionTask)
{
	// set Crabs continuation to ensure the completion task
	// is not run before Crab update has completed
	const size_t nbCrabs = mCrabs.size();
	for(PxU32 i = 0; i < nbCrabs; i++)
	{
		Crab* crab = mCrabs[i];
		crab->update(dt);
		crab->setContinuation(completionTask);
	}
}

void SampleSubmarine::onSubstepStart(float dtime)
{
	// kick off crab updates in parallel to simulate
	const size_t nbCrabs = mCrabs.size();
	for(PxU32 i = 0; i < nbCrabs; i++)
	{
		Crab* crab = mCrabs[i];
		// inverted stepper: skip crab updates right after creation, 
		// crab task is not in the pipeline at this point (onSubstepSetup not yet called).
		if(crab->getTaskManager() == NULL)
			continue;
		crab->removeReference();
	}
}

void SampleSubmarine::onTickPreRender(float dtime)
{
	mScene->lockWrite();

	if(gResetScene)
		resetScene();

	// handle mine (and submarine) explosion
	if(mSubmarineActor)
	{
		// explode all touched mines, apply damage and force to submarine
		PxVec3 subMarinePos = mSubmarineActor->getGlobalPose().p;
		const size_t nbExplodeSeamines = mMinesToExplode.size();
		for(PxU32 i=0; i < nbExplodeSeamines; i++)
		{
			Seamine* mine = mMinesToExplode[i];
			PxVec3 explosionPos = mine->mMineHead->getGlobalPose().p;
			
			// disable contact trigger callback of the chain & enable gravity
			const size_t nbLinks = mine->mLinks.size();
			for(PxU32 j = 0; j < nbLinks; j++)
			{
				PxRigidDynamic* link = mine->mLinks[j];
				setupFiltering(link, 0, 0);
				link->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, false);
			}
			
			// remove mine head
			std::vector<Seamine*>::iterator mineIter = std::find(mSeamines.begin(), mSeamines.end(), mine);
			if(mineIter != mSeamines.end())
				mSeamines.erase(mineIter);
			removeActor(mine->mMineHead);
			delete mine;

			// give damage to submarine
			static const PxReal strength = 400.0f;
			PxVec3 explosion = subMarinePos - explosionPos;
			PxReal len = explosion.normalize();
			PxReal damage = strength * (1.0f/len);
			explosion *= damage;
			mSubmarineActor->addForce(explosion*300);
			gSubmarineHealth = PxMax(gSubmarineHealth-PxI32(damage), 0);
			if(gSubmarineHealth == 0)
			{
				mSubmarineCameraController->init(subMarinePos - getCamera().getViewDir()*10.0f, getCamera().getRot());
				explode(mSubmarineActor, subMarinePos /*- explosion*0.2f*/, damage);
				mCameraAttachedToActor = NULL;
				mSubmarineCameraController->setFollowingMode(false);
				mSubmarineActor = NULL;
				break;
			}
		}
		mMinesToExplode.clear();
	}

	// respawn Crabs
	const size_t nbCrabs = mCrabs.size();
	for(PxU32 i = 0; i < nbCrabs; i++)
	{
		Crab* crab = mCrabs[i];
		if(crab->needsRespawn())
		{
			PxRigidDynamic* prevBody = crab->getCrabBody();
			PxVec3 prevPos = prevBody->getGlobalPose().p;
			delete crab;
			mCrabs[i] = SAMPLE_NEW(Crab)(*this, prevPos, mManagedMaterials[MATERIAL_RED]);
			if(gCrab == crab)
				gCrab = mCrabs[i];
			if(mCameraAttachedToActor == prevBody)
				mCameraAttachedToActor = mCrabs[i]->getCrabBody();
		}
	}

	// update camera
	if(mCameraAttachedToActor)
		mSubmarineCameraController->updateFollowingMode(getCamera(), dtime, mCameraAttachedToActor->getGlobalPose().p);

	mScene->unlockWrite();

	// start the simulation
	PhysXSample::onTickPreRender(dtime);
}

///////////////////////////////////////////////////////////////////////////////

void SampleSubmarine::explode(PxRigidActor* actor, const PxVec3& explosionPos, const PxReal explosionStrength)
{
	size_t numRenderActors = mRenderActors.size();
	for(PxU32 i = 0; i < numRenderActors; i++)
	{
		if(mRenderActors[i]->getPhysicsActor() == actor)
		{
			PxShape* shape = mRenderActors[i]->getPhysicsShape();
			PxTransform pose = PxShapeExt::getGlobalPose(*shape, *actor);

			PxGeometryHolder geom = shape->getGeometry();

			// create new actor from shape (to split compound)
			PxRigidDynamic* newActor = mPhysics->createRigidDynamic(pose);
			if(!newActor) fatalError("createRigidDynamic failed!");

			PxShape* newShape = PxRigidActorExt::createExclusiveShape(*newActor, geom.any(), *mMaterial);
			unlink(mRenderActors[i], shape, actor);
			link(mRenderActors[i], newShape, newActor);

			newActor->setActorFlag(PxActorFlag::eVISUALIZATION, true);
			newActor->setLinearDamping(10.5f);
			newActor->setAngularDamping(0.5f);
			PxRigidBodyExt::updateMassAndInertia(*newActor, 1.0f);
			mScene->addActor(*newActor);
			mPhysicsActors.push_back(newActor);

			PxVec3 explosion = pose.p - explosionPos;
			PxReal len = explosion.normalize();
			explosion *= (explosionStrength / len);
			newActor->setLinearVelocity(explosion);
			newActor->setAngularVelocity(PxVec3(1,2,3));

		}
	}

	removeActor(actor);
}
///////////////////////////////////////////////////////////////////////////////


PxFilterFlags SampleSubmarineFilterShader(	
	PxFilterObjectAttributes attributes0, PxFilterData filterData0, 
	PxFilterObjectAttributes attributes1, PxFilterData filterData1,
	PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	// let triggers through
	if(PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
	{
		pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
		return PxFilterFlag::eDEFAULT;
	}
	// generate contacts for all that were not filtered above
	pairFlags = PxPairFlag::eCONTACT_DEFAULT;

	// trigger the contact callback for pairs (A,B) where 
	// the filtermask of A contains the ID of B and vice versa.
	if((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
	
	return PxFilterFlag::eDEFAULT;
}

///////////////////////////////////////////////////////////////////////////////

void SampleSubmarine::customizeSceneDesc(PxSceneDesc& sceneDesc)
{
	sceneDesc.filterShader				= SampleSubmarineFilterShader;
	sceneDesc.simulationEventCallback	= this;
	sceneDesc.flags						|= PxSceneFlag::eREQUIRE_RW_LOCK;
}

///////////////////////////////////////////////////////////////////////////////

void SampleSubmarine::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
{	
	for(PxU32 i=0; i < nbPairs; i++)
	{
		const PxContactPair& cp = pairs[i];

		if(cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
		{
			if((pairHeader.actors[0] == mSubmarineActor) || (pairHeader.actors[1] == mSubmarineActor))
			{
				PxActor* otherActor = (mSubmarineActor == pairHeader.actors[0]) ? pairHeader.actors[1] : pairHeader.actors[0];			
				Seamine* mine =  reinterpret_cast<Seamine*>(otherActor->userData);
				// insert only once
				if(std::find(mMinesToExplode.begin(), mMinesToExplode.end(), mine) == mMinesToExplode.end())
					mMinesToExplode.push_back(mine);

				break;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void SampleSubmarine::onTrigger(PxTriggerPair* pairs, PxU32 count)
{
	for(PxU32 i=0; i < count; i++)
	{
		// ignore pairs when shapes have been deleted
		if (pairs[i].flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
			continue;

		if((pairs[i].otherActor == mSubmarineActor) && (pairs[i].triggerActor == gTreasureActor))
		{
			gTreasureFound = true;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void SampleSubmarine::helpRender(PxU32 x, PxU32 y, PxU8 textAlpha)
{
	SampleRenderer::Renderer* renderer = getRenderer();
	const PxU32 yInc = 18;
	const PxReal scale = 0.5f;
	const PxReal shadowOffset = 6.0f;
	const RendererColor textColor(255, 255, 255, textAlpha);
	const bool isMouseSupported = getApplication().getPlatform()->getSampleUserInput()->mouseSupported();
	const bool isPadSupported = getApplication().getPlatform()->getSampleUserInput()->gamepadSupported();
	const char* msg;

	msg = mApplication.inputInfoMsg("Press "," to toggle various debug visualization", TOGGLE_VISUALIZATION, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press "," to restart", SCENE_RESET,-1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
	if (isMouseSupported && isPadSupported)
		renderer->print(x, y += yInc, "Use mouse or right stick to rotate the camera", scale, shadowOffset, textColor);
	else if (isMouseSupported)
		renderer->print(x, y += yInc, "Use mouse to rotate the camera", scale, shadowOffset, textColor);
	else if (isPadSupported)
		renderer->print(x, y += yInc, "Use right stick to rotate the camera", scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press "," to switch between submarine/crab/flyCam", CAMERA_SWITCH, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);

	if(mCameraAttachedToActor == mSubmarineActor)
	{
		renderer->print(x, y += yInc, "Submarine Controller:", scale, shadowOffset, textColor);
		const char* msg = mApplication.inputInfoMsg("Press "," to move along view direction", SUBMARINE_FORWARD, SUBMARINE_BACKWARD);
		if(msg)
			renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
		msg = mApplication.inputInfoMsg("Press "," to raise and dive", SUBMARINE_UP, SUBMARINE_DOWN);
		if(msg)
			renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
	}
	else if(gCrab && (mCameraAttachedToActor == gCrab->getCrabBody()))
	{
		renderer->print(x, y += yInc, "Crab Controller:", scale, shadowOffset, textColor);
		if (isPadSupported)
			renderer->print(x, y += yInc, "Use left stick to move the crab", scale, shadowOffset, textColor);
		const char* msg = mApplication.inputMoveInfoMsg("Press "," to move the crab", CRAB_FORWARD, CRAB_BACKWARD, CRAB_LEFT, CRAB_RIGHT);
		if(msg)
			renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
	}
	else
	{
		renderer->print(x, y += yInc, "Fly Cam Controller:", scale, shadowOffset, textColor);
		if (isPadSupported)
			renderer->print(x, y += yInc, "Use left stick to move",scale, shadowOffset, textColor);
		const char* msg = mApplication.inputMoveInfoMsg("Press ","  to move", CAMERA_MOVE_FORWARD,CAMERA_MOVE_BACKWARD, CAMERA_MOVE_LEFT, CAMERA_MOVE_RIGHT);
		if(msg)
			renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
		msg = mApplication.inputInfoMsg("Press "," to move fast", CAMERA_SHIFT_SPEED, -1);
		if(msg)
			renderer->print(x, y += yInc, msg, scale, shadowOffset, textColor);
	}
}

void SampleSubmarine::descriptionRender(PxU32 x, PxU32 y, PxU8 textAlpha)
{
	bool print=(textAlpha!=0.0f);

	if(print)
	{
		Renderer* renderer = getRenderer();
		const PxU32 yInc = 24;
		const PxReal scale = 0.5f;
		const PxReal shadowOffset = 6.0f;
		const RendererColor textColor(255, 255, 255, textAlpha);

		char line0[256]="This sample demonstrates the creation of jointed systems. In particular,";
		char line1[256]="a complex system of driven joints is introduced to model crab motion,";
		char line2[256]="while distance joints are used to model the tethering of exploding sea"; 
		char line3[256]="mines to the seabed.  Trigger shapes are presented to detect the";
		char line4[256]="arrival of the submarine at a treasure chest on the ocean floor.";
		char line5[256]="Similarly, contact notification is used to report the overlap of the";
		char line6[256]="submarine with the exploding sea mines.  Crab logic is governed by sdk";   
		char line7[256]="raycast results, and illustrates the application of the PhysX SDK task";
		char line8[256]="scheduler.";

		renderer->print(x, y+=yInc, line0, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line1, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line2, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line3, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line4, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line5, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line6, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line7, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line8, scale, shadowOffset, textColor);
	}
}

///////////////////////////////////////////////////////////////////////////////

void SampleSubmarine::customizeRender()
{
	SampleRenderer::Renderer* renderer = getRenderer();
	const PxU32 yInc = 18;
	const RendererColor textColor(255, 255, 255, 255);

	PxU32 width, height;
	renderer->getWindowSize(width, height);
	char healthBar[20];
	PxU32 h = gSubmarineHealth;
	sprintf(healthBar, "Health:%c%c%c%c%c%c%c%c%c%c", (h>90?'I':' '), (h>80?'I':' '), (h>70?'I':' '),(h>60?'I':' '),(h>50?'I':' '), 
	(h>40?'I':' '), (h>30?'I':' '), (h>20?'I':' '),(h>10?'I':' '),(h>0?'I':' '));
	renderer->print(width-130, height-yInc, healthBar);	
	if(gTreasureFound)
		renderer->print(width-160, height-2*yInc, "Treasure Found!");	
}

///////////////////////////////////////////////////////////////////////////////

static void setFlag(PxU32& flags, PxU32 flag, bool set)
{
	if(set)
		flags |= flag;
	else
		flags &= ~flag;
}

///////////////////////////////////////////////////////////////////////////////

void SampleSubmarine::onPointerInputEvent(const SampleFramework::InputEvent& ie, physx::PxU32 x, physx::PxU32 y, physx::PxReal dx, physx::PxReal dy, bool val)
{
	if(ie.m_Id== CAMERA_MOUSE_LOOK || !mSubmarineActor || mCameraAttachedToActor != mSubmarineActor)
		PhysXSample::onPointerInputEvent(ie,x,y,dx,dy, val);

	switch (ie.m_Id)
	{
	case SUBMARINE_FORWARD:
		{
			setFlag(gKeyFlags, Movement::eSUBMAINE_FWD, val);
		}
		break;
	case SUBMARINE_BACKWARD:
		{
			setFlag(gKeyFlags, Movement::eSUBMAINE_BCKWD, val);
		}
		break;
	default:
		break;
	}		
}

//////////////////////////////////////////////////////////////////////////

void SampleSubmarine::collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents)
{
	PhysXSample::collectInputEvents(inputEvents);

	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(CAMERA_MOVE_UP);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(CAMERA_MOVE_DOWN);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(CAMERA_SPEED_INCREASE);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(CAMERA_SPEED_DECREASE);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(CAMERA_MOVE_BUTTON);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(MOUSE_LOOK_BUTTON);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(SPAWN_DEBUG_OBJECT);

	//digital mouse events
	DIGITAL_INPUT_EVENT_DEF(SUBMARINE_FORWARD,											MOUSE_BUTTON_LEFT,			MOUSE_BUTTON_LEFT,			MOUSE_BUTTON_LEFT	);
	DIGITAL_INPUT_EVENT_DEF(SUBMARINE_BACKWARD,											MOUSE_BUTTON_RIGHT,			MOUSE_BUTTON_RIGHT,			MOUSE_BUTTON_RIGHT	);
										
	//digital keyboard events										
	DIGITAL_INPUT_EVENT_DEF(CRAB_FORWARD,												SCAN_CODE_FORWARD,			SCAN_CODE_FORWARD,			SCAN_CODE_FORWARD	);
	DIGITAL_INPUT_EVENT_DEF(CRAB_BACKWARD,												SCAN_CODE_BACKWARD,			SCAN_CODE_BACKWARD,			SCAN_CODE_BACKWARD	);
	DIGITAL_INPUT_EVENT_DEF(CRAB_LEFT,													SCAN_CODE_LEFT,				SCAN_CODE_LEFT,				SCAN_CODE_LEFT		);
	DIGITAL_INPUT_EVENT_DEF(CRAB_RIGHT,													SCAN_CODE_RIGHT,			SCAN_CODE_RIGHT,			SCAN_CODE_RIGHT		);
	DIGITAL_INPUT_EVENT_DEF(SUBMARINE_UP,												SCAN_CODE_FORWARD,			SCAN_CODE_FORWARD,			SCAN_CODE_FORWARD	);
	DIGITAL_INPUT_EVENT_DEF(SUBMARINE_DOWN,												SCAN_CODE_BACKWARD,			SCAN_CODE_BACKWARD,			SCAN_CODE_BACKWARD	);
	DIGITAL_INPUT_EVENT_DEF(CAMERA_SWITCH,												SCAN_CODE_DOWN,				SCAN_CODE_DOWN,				SCAN_CODE_DOWN		);
	DIGITAL_INPUT_EVENT_DEF(SCENE_RESET,												WKEY_R,						OSXKEY_R,					LINUXKEY_R			);
							
	//digital gamepad events							
	DIGITAL_INPUT_EVENT_DEF(SUBMARINE_FORWARD,											GAMEPAD_RIGHT_SHOULDER_BOT,	GAMEPAD_RIGHT_SHOULDER_BOT,	LINUXKEY_UNKNOWN	);
	DIGITAL_INPUT_EVENT_DEF(SUBMARINE_BACKWARD,											GAMEPAD_LEFT_SHOULDER_BOT,	GAMEPAD_LEFT_SHOULDER_BOT,	LINUXKEY_UNKNOWN	);
	DIGITAL_INPUT_EVENT_DEF(SUBMARINE_UP,												GAMEPAD_RIGHT_SHOULDER_TOP,	GAMEPAD_RIGHT_SHOULDER_TOP,	LINUXKEY_UNKNOWN	);
	DIGITAL_INPUT_EVENT_DEF(SUBMARINE_DOWN,												GAMEPAD_LEFT_SHOULDER_TOP,	GAMEPAD_LEFT_SHOULDER_TOP,	LINUXKEY_UNKNOWN	);
	DIGITAL_INPUT_EVENT_DEF(CAMERA_SWITCH,												GAMEPAD_RIGHT_STICK,		GAMEPAD_RIGHT_STICK,		LINUXKEY_UNKNOWN	);
	DIGITAL_INPUT_EVENT_DEF(SCENE_RESET,												GAMEPAD_LEFT_STICK,			GAMEPAD_LEFT_STICK,			LINUXKEY_UNKNOWN	);
		
	// analog gamepad events	
	ANALOG_INPUT_EVENT_DEF(CRAB_LEFT_RIGHT, GAMEPAD_DEFAULT_SENSITIVITY,				GAMEPAD_LEFT_STICK_X,		GAMEPAD_LEFT_STICK_X,		LINUXKEY_UNKNOWN	);
	ANALOG_INPUT_EVENT_DEF(CRAB_FORWARD_BACKWARD, GAMEPAD_DEFAULT_SENSITIVITY,			GAMEPAD_LEFT_STICK_Y,		GAMEPAD_LEFT_STICK_Y,		LINUXKEY_UNKNOWN	);
	ANALOG_INPUT_EVENT_DEF(SUBMARINE_FORWARD_BACKWARD, GAMEPAD_DEFAULT_SENSITIVITY,		GAMEPAD_LEFT_STICK_Y,		OSXKEY_UNKNOWN,         	LINUXKEY_UNKNOWN	);
}

///////////////////////////////////////////////////////////////////////////////

void SampleSubmarine::onDigitalInputEvent(const SampleFramework::InputEvent& ie, bool val)
{
	if(mSubmarineActor && mCameraAttachedToActor == mSubmarineActor)
	{
		switch (ie.m_Id)
		{
		case SUBMARINE_FORWARD:
			setFlag(gKeyFlags, Movement::eSUBMAINE_FWD, val);
			break;
		case SUBMARINE_BACKWARD:
			setFlag(gKeyFlags, Movement::eSUBMAINE_BCKWD, val);
			break;
		case SUBMARINE_UP:
			setFlag(gKeyFlags, Movement::eSUBMAINE_UP, val);
			break;
		case SUBMARINE_DOWN:
			setFlag(gKeyFlags, Movement::eSUBMAINE_DOWN, val);
			break;
		}
	}
	else if (gCrab && mCameraAttachedToActor == gCrab->getCrabBody())
	{
		switch (ie.m_Id)
		{
		case CRAB_FORWARD:
			setFlag(gKeyFlags, Movement::eCRAB_FWD, val);
			break;
		case CRAB_BACKWARD:
			setFlag(gKeyFlags, Movement::eCRAB_BCKWD, val);
			break;
		case CRAB_LEFT:
			setFlag(gKeyFlags, Movement::eCRAB_ROTATE_LEFT, val);
			break;
		case CRAB_RIGHT:
			setFlag(gKeyFlags, Movement::eCRAB_ROTATE_RIGHT, val);
			break;
		}
	}

	if (val)
	{
		switch (ie.m_Id)
		{
		case CAMERA_SWITCH:
			{
				// cycle camera
				if(mSubmarineActor && mCameraAttachedToActor == NULL)
					mCameraAttachedToActor = mSubmarineActor;
				else if(gCrab && mCameraAttachedToActor != gCrab->getCrabBody())
					mCameraAttachedToActor = gCrab->getCrabBody();
				else
					mCameraAttachedToActor = NULL;

				mSubmarineCameraController->init(getCamera().getPos(), getCamera().getRot());
				mSubmarineCameraController->setFollowingMode(mCameraAttachedToActor != NULL);
			}
			break;
		case SCENE_RESET:
			{
				gResetScene = true;
			}
			break;
		}
	}

	PhysXSample::onDigitalInputEvent(ie,val);
}

void SampleSubmarine::onAnalogInputEvent(const SampleFramework::InputEvent& ie, float val)
{
	if (mSubmarineActor && mCameraAttachedToActor == mSubmarineActor)
	{
		switch (ie.m_Id)
		{
		case SUBMARINE_FORWARD_BACKWARD:
			{
				setFlag(gKeyFlags, Movement::eSUBMAINE_FWD, val > 0.3f);
				setFlag(gKeyFlags, Movement::eSUBMAINE_BCKWD, val < -0.3f);
			}
			break;
		}
	}
	if(gCrab && mCameraAttachedToActor == gCrab->getCrabBody())
	{
		switch (ie.m_Id)
		{
		case CRAB_FORWARD_BACKWARD:
			{
				setFlag(gKeyFlags, Movement::eCRAB_FWD, val > 0.3f);
				setFlag(gKeyFlags, Movement::eCRAB_BCKWD, val < -0.3f);
			}
			break;
		case CRAB_LEFT_RIGHT:
			{
				setFlag(gKeyFlags, Movement::eCRAB_ROTATE_LEFT, val > 0.3f);
				setFlag(gKeyFlags, Movement::eCRAB_ROTATE_RIGHT, val < -0.3f);
			}
			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void SampleSubmarine::handleInput()
{
	if(gCrab && mCameraAttachedToActor == gCrab->getCrabBody())
	{
		PxReal accFactor = 3.0f;
		PxReal forward = 0, rot = 0;
		if(gKeyFlags & Movement::eCRAB_FWD)
			forward = 1.0f;
		if(gKeyFlags & Movement::eCRAB_BCKWD)
			forward = -1.0f;
		if(gKeyFlags & Movement::eCRAB_ROTATE_LEFT)
			rot = 1.0f;
		if(gKeyFlags & Movement::eCRAB_ROTATE_RIGHT)
			rot = -1.0f;
		PxReal left = rot + forward; 
		PxReal right = -rot + forward;
		gCrab->setAcceleration(left*accFactor, right*accFactor);
	}
	else if(mSubmarineActor && mCameraAttachedToActor == mSubmarineActor)
	{
		if(gKeyFlags & Movement::eSUBMAINE_FWD)
			gForce.x -= gLinPower;
		if(gKeyFlags & Movement::eSUBMAINE_BCKWD)
			gForce.x += gLinPower;
		if(gKeyFlags & Movement::eSUBMAINE_UP)
			gForce.y += gLinPower;
		if(gKeyFlags & Movement::eSUBMAINE_DOWN)
			gForce.y -= gLinPower;

		if(gKeyFlags & (Movement::eSUBMAINE_FWD|Movement::eSUBMAINE_BCKWD))
		{
			PxSceneReadLock scopedLock(*mScene);

			static const PxReal camEpsilon = 0.001f;
			PxTransform subPose = mSubmarineActor->getGlobalPose();
			PxVec3 cameraDir = getCamera().getViewDir();
			PxVec3 cameraDirInSub = subPose.rotateInv(cameraDir).getNormalized();
			PxVec3 cameraUpInSub = subPose.rotateInv(PxVec3(0,1,0)).getNormalized();

			if(PxAbs(cameraDirInSub.z) > camEpsilon)
				gTorque.y += gAngPower*cameraDirInSub.z;
			if(PxAbs(cameraDirInSub.y) > camEpsilon)
				gTorque.z -= gAngPower*cameraDirInSub.y;
			if(PxAbs(cameraUpInSub.z) > camEpsilon)
				gTorque.x += gAngPower*cameraUpInSub.z;	
		}
	}
}

//////////////////////////////////////////////////////////////////////////

