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


#ifndef SAMPLE_VEHICLE_H
#define SAMPLE_VEHICLE_H

#include "PhysXSample.h"
#include "SampleVehicle_ControlInputs.h"
#include "SampleVehicle_CameraController.h"
#include "SampleVehicle_VehicleController.h"
#include "SampleVehicle_VehicleManager.h"
#include "SampleVehicle_GameLogic.h"
#include "vehicle/PxVehicleTireFriction.h"

class SampleVehicle : public PhysXSample
{

public:

											SampleVehicle(PhysXSampleApplication& app);
	virtual									~SampleVehicle();


	///////////////////////////////////////////////////////////////////////////////

	// Implements RAWImportCallback
	virtual	void							newMesh(const RAWMesh&);

	///////////////////////////////////////////////////////////////////////////////

	// Implements SampleApplication
	virtual	void							onInit();
    virtual	void						    onInit(bool restart) { onInit(); }
	virtual	void							onShutdown();

	virtual	void							onTickPreRender(PxF32 dtime);
	virtual	void							onTickPostRender(PxF32 dtime);

	virtual void							onDigitalInputEvent(const SampleFramework::InputEvent& , bool val);
	virtual void							onAnalogInputEvent(const SampleFramework::InputEvent& , float val);

	///////////////////////////////////////////////////////////////////////////////

	// Implements PhysXSampleApplication
	virtual	void							helpRender(PxU32 x, PxU32 y, PxU8 textAlpha);
	virtual	void							descriptionRender(PxU32 x, PxU32 y, PxU8 textAlpha);
	virtual	void							customizeSample(SampleSetup&);
	virtual	void							customizeSceneDesc(PxSceneDesc&);
	virtual	void							customizeRender();
	virtual	void							onSubstep(PxF32 dtime);	
	virtual void							collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents);

private:

	SampleVehicle_ControlInputs		mControlInputs;
	SampleVehicle_CameraController	mCameraController;
	SampleVehicle_VehicleController	mVehicleController;


	//Terrain

	PxF32*							mTerrainVB;
	PxU32							mNbTerrainVerts;

	enum
	{
		MAX_NUM_INDEX_BUFFERS = 16
	};
	PxU32							mNbIB;
	PxU32*							mIB[MAX_NUM_INDEX_BUFFERS];
	PxU32							mNbTriangles[MAX_NUM_INDEX_BUFFERS];
	PxU32							mRenderMaterial[MAX_NUM_INDEX_BUFFERS];

	//Materials

	PxVehicleDrivableSurfaceType	mVehicleDrivableSurfaceTypes[MAX_NUM_INDEX_BUFFERS];
	PxMaterial*						mStandardMaterials[MAX_NUM_INDEX_BUFFERS];
	PxMaterial*						mChassisMaterialDrivable;
	PxMaterial*						mChassisMaterialNonDrivable;


	RenderMaterial*					mTerrainMaterial;
	RenderMaterial*					mRoadMaterial;
	RenderMaterial*					mRoadIceMaterial;
	RenderMaterial*					mRoadGravelMaterial;

	void							createStandardMaterials();

	enum eFocusVehicleType
	{
		ePLAYER_VEHICLE_TYPE_VEHICLE4W=0,
		ePLAYER_VEHICLE_TYPE_VEHICLE6W,
		ePLAYER_VEHICLE_TYPE_TANK4W,
		ePLAYER_VEHICLE_TYPE_TANK6W,
		eMAX_NUM_FOCUS_VEHICLE_TYPES
	};

	// Vehicles
	SampleVehicle_VehicleManager	mVehicleManager;
	std::vector<RenderMeshActor*>	mVehicleGraphics;
	PxU32							mPlayerVehicle;
	eFocusVehicleType				mPlayerVehicleType;
	PxVehicleDriveTankControlModel::Enum mTankDriveModel;

	const char*						getFocusVehicleName();
	void							createVehicles();

	PxU32							mTerrainSize;
	PxF32							mTerrainWidth;
	PxRigidActor*					mHFActor;

	void							createTrack(PxU32 size, PxF32 width, PxF32 chaos);
	void							createTerrain(PxU32 size, PxF32 width, PxF32 chaos);
	void							addRenderMesh(PxF32* verts, PxU32 nVerts, PxU32* indices, PxU32 mIndices, PxU32 matID);
	void							addMesh(PxRigidActor* actor, PxF32* verts, PxU32 nVerts, PxU32* indices, PxU32 mIndices, PxU32 materialIndex, const char* filename);
	void							createLandscapeMesh();

	//Obstacles

	void							createObstacles();
	PxRigidStatic* 					addStaticObstacle(const PxTransform& transform, const PxU32 numShapes, PxTransform* shapeTransforms, PxGeometry** shapeGeometries, PxMaterial** shapeMaterials);
	PxRigidDynamic*					addDynamicObstacle(const PxTransform& transform, const PxF32 mass, const PxU32 numShapes, PxTransform* transforms,  PxGeometry** geometries, PxMaterial** materials);
	PxRigidDynamic* 				addDynamicDrivableObstacle(const PxTransform& transform, const PxF32 mass, const PxU32 numShapes, PxTransform* transforms,  PxGeometry** geometries, PxMaterial** materials);
	void							createStack(PxU32 size, PxF32 boxSize, const PxVec3& pos, const PxQuat& quat);
	void							createWall(const PxU32 numHorizontalBoxes, const PxU32 numVerticalBoxes, const PxF32 boxSize, const PxVec3& pos, const PxQuat& quat);

	//Debug render

	bool							mHideScreenText;
	bool							mDebugRenderFlag;
#if PX_DEBUG_VEHICLE_ON
	PxU32							mDebugRenderActiveGraphChannelWheel;
	PxU32							mDebugRenderActiveGraphChannelEngine;
	PxVehicleTelemetryData*			mTelemetryData4W;
	PxVehicleTelemetryData*			mTelemetryData6W;
#endif
	void							setupTelemetryData();
	void							clearTelemetryData();

	void							drawWheels();
	void							drawVehicleDebug();
	void							drawHud();
	void							drawGraphsAndPrintTireSurfaceTypes(const PxVehicleWheels& focusVehicle, const PxVehicleWheelQueryResult& focusVehicleWheelQueryResults);
	void							drawFocusVehicleGraphsAndPrintTireSurfaces();

	//Waypoints
	SampleVehicleWayPoints			mWayPoints;
	bool							mFixCar;
	bool							mBackToStart;

	//3W and 4W modes
	bool							m3WModeIncremented;
	PxU32							m3WMode;

	PxF32							mForwardSpeedHud;	

#if defined(SERIALIZE_VEHICLE_BINARY)
	void*							mMemory;
#endif

	void							updateCameraController(const PxF32 dtime, PxScene& scene);
	void							updateVehicleController(const PxF32 dtime);
	void							updateVehicleManager(const PxF32 dtime, const PxVec3& gravity);

	void							resetFocusVehicleAtWaypoint();
	PxRigidDynamic*					getFocusVehicleRigidDynamicActor();
	bool							getFocusVehicleUsesAutoGears();
	char							mVehicleFilePath[256];
};

#endif
