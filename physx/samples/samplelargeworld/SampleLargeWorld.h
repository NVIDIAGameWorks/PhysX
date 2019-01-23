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


#ifndef SAMPLE_LARGE_WORLD_H
#define SAMPLE_LARGE_WORLD_H

#include "PhysXSample.h"
#include "ChunkLoader.h"
#include "characterkinematic/PxController.h"
#include "characterkinematic/PxControllerBehavior.h"
#include "PxTkRandom.h"


namespace physx
{
	class PxControllerManager;
}

class SampleCCTCameraController;
class ControlledActor;
class RenderBaseActor;
class BackgroundLoader;

typedef Ps::HashMap<const char*, RAWMesh, Ps::Hash<const char*>, Ps::RawAllocator> ObjMeshMap;

class SampleLargeWorld : public PhysXSample
	, public PxUserControllerHitReport, public PxControllerBehaviorCallback, public PxQueryFilterCallback
{
	friend class BackgroundLoader;
public:
											SampleLargeWorld(PhysXSampleApplication& app);
	virtual									~SampleLargeWorld();

	///////////////////////////////////////////////////////////////////////////////

	// Implements SampleApplication
	virtual	void							onInit();
    virtual	void						    onInit(bool restart) { onInit(); }
	virtual	void							onShutdown();
	virtual	void							onTickPreRender(PxF32 dtime);
	virtual void							onDigitalInputEvent(const SampleFramework::InputEvent& , bool val);

	//virtual void							onRelease(const PxBase* , void* , PxDeletionEventFlag::Enum ) {}
	///////////////////////////////////////////////////////////////////////////////

	// Implements PhysXSampleApplication	
	virtual	void							customizeSceneDesc(PxSceneDesc&);
	virtual	void							customizeSample(SampleSetup&);
	virtual	void							onSubstep(PxF32 dtime);	
	virtual void							collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents);
	virtual void							helpRender(PxU32 x, PxU32 y, PxU8 textAlpha);
	virtual	void							descriptionRender(PxU32 x, PxU32 y, PxU8 textAlpha);
	virtual	void							customizeRender();
	virtual	void							newMesh(const RAWMesh&);

	///////////////////////////////////////////////////////////////////////////////

	// Implements PxUserControllerHitReport
	virtual void							onShapeHit(const PxControllerShapeHit& hit);
	virtual void							onControllerHit(const PxControllersHit& hit)		{}
	virtual void							onObstacleHit(const PxControllerObstacleHit& hit)	{}

	///////////////////////////////////////////////////////////////////////////////

	// Implements PxControllerBehaviorCallback
	virtual PxControllerBehaviorFlags		getBehaviorFlags(const PxShape& shape, const PxActor& actor);
	virtual PxControllerBehaviorFlags		getBehaviorFlags(const PxController& controller);
	virtual PxControllerBehaviorFlags		getBehaviorFlags(const PxObstacle& obstacle);

	///////////////////////////////////////////////////////////////////////////////

	// Implements PxQueryFilterCallback
	virtual	PxQueryHitType::Enum		preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags);
	virtual	PxQueryHitType::Enum		postFilter(const PxFilterData& filterData, const PxQueryHit& hit);

	///////////////////////////////////////////////////////////////////////////////

	// Implements PxSimulationFilterShader
	enum
	{
											CCD_FLAG = 1<<29,
	};

	static PxFilterFlags					filter(PxFilterObjectAttributes attributes0, 
														PxFilterData filterData0, 
														PxFilterObjectAttributes attributes1, 
														PxFilterData filterData1,
														PxPairFlags& pairFlags,
														const void* constantBlock,
														PxU32 constantBlockSize);

	///////////////////////////////////////////////////////////////////////////////
	
	//Create chunk_i_j file
	void									createChunk(CoordType coordX, CoordType coordY, PxCollection& outCollection);
	

private:
	RAWMesh*								createRawMeshFromMeshGeom(const PxTriangleMeshGeometry& mesh, RAWMesh &rawMesh);
	RAWMesh*								createRAWMeshFromObjMesh(const char* inObjFileName, const PxTransform& inPos, PxU32 inMaterialID, RAWMesh &outRawMesh);

	//Deep cope 
	void									cloneMesh(const RAWMesh& inSrc, RAWMesh& outDst);

	void									renderProgressBar(PxF32 ratio, bool needFlush = false);
	void									releaseJoints();
	bool									readyToSyncCCT();
	void									attachNearestObjectsToCCT();

	PxRigidDynamic*							createBox(const PxVec3& pos, const PxVec3& dims, const PxVec3* linVel,
														RenderMaterial* material, PxReal density);
	void									addWindMills(const PxTriangleMeshGeometry& meshGeom, PxToolkit::BasicRandom& random, 
														const PxBounds3& bound, PxCollection& outCollection);
	void									addTrees(const PxTriangleMeshGeometry& meshGeom, const PxBounds3& bound, PxCollection& outCollection);
	void									addCity(SampleArray<PxVec3>& terrainVertices, const PxBounds3& bound, PxCollection& outCollection);
	void									addFarm(SampleArray<PxVec3>& terrainVertices, const PxBounds3& bound, PxCollection& outCollection);
	void									addFortress(SampleArray<PxVec3>& terrainVertices, const PxBounds3& bound, PxCollection& outCollection);
	
	static void								setCCDActive(PxShape& shape);
	static bool								isCCDActive(PxFilterData& filterData);

	PxTriangleMesh*							generateTriMesh(const SampleArray<PxVec3>* vertices, const SampleArray<PxU32>* indices);

	void									setCollisionGroup(PxRigidActor* actor, PxU32 group);

	//Terrain template data from Terrain.bin
	struct BinData
	{
		SampleArray<SampleArray<PxVec3> >	mTerrainVertices;
		SampleArray<SampleArray<PxU32> >	mTerrainIndices;	

		CoordType							mDim;

		//Need parent to render progress bar
		void								serialize(SampleLargeWorld* parent, const char* terrainFile);

	}binData;

	ObjMeshMap								mRenderMeshCache;
	
	BackgroundLoader*						mBGLoader;
	SampleCCTCameraController*				mCCTCamera;
	ControlledActor*						mActor;
	PxControllerManager*					mControllerManager;
	PxExtendedVec3							mControllerInitialPosition;
	PxExtendedVec3							mLastCCTPosition;
	std::vector<PxFixedJoint*>				mFixedJoints;

	//Sky box
	RenderBaseActor*						mSkybox[5];

	//For progress bar renderering
	PxReal									mProgressBarRatio;
	Ps::Time::Second						mDiskIOTime;
	Ps::Time::Second						mPhysxStreaming;
	Ps::Time::Second						mGraphicStreaming;
	
	//Bound of the whole world
	PxBounds3								mWorldBound;

	bool									mReadyToSyncCCT;
	bool									mAddRenderActor;
	bool									mPick;
	bool									mKeyShiftDown;

	PxReal									mDefaultCameraSpeed;

public:
	static const PxU32						DEFAULT_COLLISION_GROUP = 0;
	static const PxU32						FAN_COLLISION_GROUP		= 1;
	static const PxU32						PICKING_COLLISION_GROUP = 2;

	PxStringTable*							mStringTable;
};

#endif
