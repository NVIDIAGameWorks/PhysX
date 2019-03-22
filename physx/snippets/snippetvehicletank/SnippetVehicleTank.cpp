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

// ****************************************************************************
// This snippet illustrates simple use of PxVehicleDriveTank.
//
// It creates a tank on a plane and then controls the tank so that it performs a 
// number of choreographed manoeuvres such as accelerate, reverse, soft turns, 
// and hard turns.

// It is a good idea to record and playback with pvd (PhysX Visual Debugger).
// ****************************************************************************

#include <ctype.h>

#include "PxPhysicsAPI.h"

#include "../snippetvehiclecommon/SnippetVehicleSceneQuery.h"
#include "../snippetvehiclecommon/SnippetVehicleFilterShader.h"
#include "../snippetvehiclecommon/SnippetVehicleTireFriction.h"
#include "../snippetvehiclecommon/SnippetVehicleCreate.h"

#include "../snippetcommon/SnippetPrint.h"
#include "../snippetcommon/SnippetPVD.h"
#include "../snippetutils/SnippetUtils.h"

using namespace physx;
using namespace snippetvehicle;

PxDefaultAllocator		gAllocator;
PxDefaultErrorCallback	gErrorCallback;

PxFoundation*			gFoundation = NULL;
PxPhysics*				gPhysics	= NULL;

PxDefaultCpuDispatcher*	gDispatcher = NULL;
PxScene*				gScene		= NULL;

PxCooking*				gCooking	= NULL;

PxMaterial*				gMaterial	= NULL;

PxPvd*                  gPvd        = NULL;

VehicleSceneQueryData*	gVehicleSceneQueryData = NULL;
PxBatchQuery*			gBatchQuery = NULL;

PxVehicleDrivableSurfaceToTireFrictionPairs* gFrictionPairs = NULL;

PxRigidStatic*			gGroundPlane = NULL;
PxVehicleDriveTank*		gTank		= NULL;

PxVehicleKeySmoothingData gKeySmoothingData=
{
	{
		6.0f,	//rise rate eANALOG_INPUT_ACCEL=0,
		6.0f,	//rise rate eANALOG_INPUT_BRAKE,		
		6.0f,	//rise rate eANALOG_INPUT_HANDBRAKE,	
		2.5f,	//rise rate eANALOG_INPUT_STEER_LEFT,	
		2.5f,	//rise rate eANALOG_INPUT_STEER_RIGHT,	
	},
	{
		10.0f,	//fall rate eANALOG_INPUT_ACCEL=0,
		10.0f,	//fall rate eANALOG_INPUT_BRAKE,		
		10.0f,	//fall rate eANALOG_INPUT_HANDBRAKE,	
		5.0f,	//fall rate eANALOG_INPUT_STEER_LEFT,	
		5.0f	//fall rate eANALOG_INPUT_STEER_RIGHT,	
	}
};

PxVehiclePadSmoothingData gPadSmoothingData=
{
	{
		6.0f,	//rise rate eANALOG_INPUT_ACCEL=0,
		6.0f,	//rise rate eANALOG_INPUT_BRAKE,		
		6.0f,	//rise rate eANALOG_INPUT_HANDBRAKE,	
		2.5f,	//rise rate eANALOG_INPUT_STEER_LEFT,	
		2.5f,	//rise rate eANALOG_INPUT_STEER_RIGHT,	
	},
	{
		10.0f,	//fall rate eANALOG_INPUT_ACCEL=0
		10.0f,	//fall rate eANALOG_INPUT_BRAKE_LEFT	
		10.0f,	//fall rate eANALOG_INPUT_BRAKE_RIGHT	
		5.0f,	//fall rate eANALOG_INPUT_THRUST_LEFT	
		5.0f	//fall rate eANALOG_INPUT_THRUST_RIGHT
	}
};

PxVehicleDriveTankRawInputData gVehicleInputData(PxVehicleDriveTankControlModel::eSTANDARD);

enum DriveMode
{
	eDRIVE_MODE_ACCEL_FORWARDS=0,
	eDRIVE_MODE_ACCEL_REVERSE,
	eDRIVE_MODE_HARD_TURN_LEFT,
	eDRIVE_MODE_SOFT_TURN_LEFT,
	eDRIVE_MODE_HARD_TURN_RIGHT,
	eDRIVE_MODE_SOFT_TURN_RIGHT,
	eDRIVE_MODE_BRAKE,
	eDRIVE_MODE_NONE
};

DriveMode gDriveModeOrder[] =
{
	eDRIVE_MODE_BRAKE,
	eDRIVE_MODE_ACCEL_FORWARDS,
	eDRIVE_MODE_BRAKE,
	eDRIVE_MODE_ACCEL_REVERSE,
	eDRIVE_MODE_BRAKE,
	eDRIVE_MODE_HARD_TURN_LEFT, 
	eDRIVE_MODE_BRAKE,
	eDRIVE_MODE_HARD_TURN_RIGHT,
	eDRIVE_MODE_BRAKE,
	eDRIVE_MODE_SOFT_TURN_LEFT,
	eDRIVE_MODE_BRAKE,
	eDRIVE_MODE_SOFT_TURN_RIGHT,
	eDRIVE_MODE_NONE
};

PxF32					gTankModeLifetime = 4.0f;
PxF32					gTankModeTimer = 0.0f;
PxU32					gTankOrderProgress = 0;
bool					gTankOrderComplete = false;
bool					gMimicKeyInputs = false;

VehicleDesc initTankDesc()
{
	//Set up the chassis mass, dimensions, moment of inertia, and center of mass offset.
	//The moment of inertia is just the moment of inertia of a cuboid but modified for easier steering.
	//Center of mass offset is 0.65m above the base of the chassis and 0.25m towards the front.
	const PxF32 chassisMass = 1500.0f;
	const PxVec3 chassisDims(3.5f,2.0f,9.0f);
	const PxVec3 chassisMOI
		((chassisDims.y*chassisDims.y + chassisDims.z*chassisDims.z)*chassisMass/12.0f,
		 (chassisDims.x*chassisDims.x + chassisDims.z*chassisDims.z)*0.8f*chassisMass/12.0f,
		 (chassisDims.x*chassisDims.x + chassisDims.y*chassisDims.y)*chassisMass/12.0f);
	const PxVec3 chassisCMOffset(0.0f, -chassisDims.y*0.5f + 0.65f, 0.25f);

	//Set up the wheel mass, radius, width, moment of inertia, and number of wheels.
	//Moment of inertia is just the moment of inertia of a cylinder.
	const PxF32 wheelMass = 20.0f;
	const PxF32 wheelRadius = 0.5f;
	const PxF32 wheelWidth = 0.4f;
	const PxF32 wheelMOI = 0.5f*wheelMass*wheelRadius*wheelRadius;
	const PxU32 nbWheels = 14;

	VehicleDesc tankDesc;

	tankDesc.chassisMass = chassisMass;
	tankDesc.chassisDims = chassisDims;
	tankDesc.chassisMOI = chassisMOI;
	tankDesc.chassisCMOffset = chassisCMOffset;
	tankDesc.chassisMaterial = gMaterial;
	tankDesc.chassisSimFilterData = PxFilterData(COLLISION_FLAG_CHASSIS, COLLISION_FLAG_CHASSIS_AGAINST, 0, 0);

	tankDesc.wheelMass = wheelMass;
	tankDesc.wheelRadius = wheelRadius;
	tankDesc.wheelWidth = wheelWidth;
	tankDesc.wheelMOI = wheelMOI;
	tankDesc.numWheels = nbWheels;
	tankDesc.wheelMaterial = gMaterial;
	tankDesc.chassisSimFilterData = PxFilterData(COLLISION_FLAG_WHEEL, COLLISION_FLAG_WHEEL_AGAINST, 0, 0);

	return tankDesc;
}

void startAccelerateForwardsMode()
{
	if(gMimicKeyInputs)
	{
		gVehicleInputData.setDigitalAccel(true);
		gVehicleInputData.setDigitalLeftThrust(true);
		gVehicleInputData.setDigitalRightThrust(true);
	}
	else
	{
		gVehicleInputData.setAnalogAccel(1.0f);
		gVehicleInputData.setAnalogLeftThrust(1.0f);
		gVehicleInputData.setAnalogRightThrust(1.0f);
	}
}

void startAccelerateReverseMode()
{
	gTank->mDriveDynData.forceGearChange(PxVehicleGearsData::eREVERSE);

	if(gMimicKeyInputs)
	{
		gVehicleInputData.setDigitalAccel(true);
		gVehicleInputData.setDigitalLeftThrust(true);
		gVehicleInputData.setDigitalRightThrust(true);
	}
	else
	{
		gVehicleInputData.setAnalogAccel(1.0f);
		gVehicleInputData.setAnalogLeftThrust(1.0f);
		gVehicleInputData.setAnalogRightThrust(1.0f);
	}
}

void startBrakeMode()
{
	if(gMimicKeyInputs)
	{
		gVehicleInputData.setDigitalLeftBrake(true);
		gVehicleInputData.setDigitalRightBrake(true);
	}
	else
	{
		gVehicleInputData.setAnalogLeftBrake(1.0f);
		gVehicleInputData.setAnalogRightBrake(1.0f);
	}
}

void startTurnHardLeftMode()
{
	if(gMimicKeyInputs)
	{
		gVehicleInputData.setDigitalAccel(true);
		gVehicleInputData.setDigitalLeftThrust(true);
		gVehicleInputData.setDigitalRightBrake(true);
	}
	else
	{
		gVehicleInputData.setAnalogAccel(1.0f);
		gVehicleInputData.setAnalogLeftThrust(1.0f);
		gVehicleInputData.setAnalogRightBrake(1.0f);
	}
}

void startTurnHardRightMode()
{
	if(gMimicKeyInputs)
	{
		gVehicleInputData.setDigitalAccel(true);
		gVehicleInputData.setDigitalRightThrust(true);
		gVehicleInputData.setDigitalLeftBrake(true);
	}
	else
	{
		gVehicleInputData.setAnalogAccel(1.0f);
		gVehicleInputData.setAnalogRightThrust(1.0f);
		gVehicleInputData.setAnalogLeftBrake(1.0f);
	}
}

void startTurnSoftLeftMode()
{
	if(gMimicKeyInputs)
	{
		gVehicleInputData.setDigitalAccel(true);
		gVehicleInputData.setDigitalLeftThrust(true);
	}
	else
	{
		gVehicleInputData.setAnalogAccel(1.0f);
		gVehicleInputData.setAnalogLeftThrust(1.0f);
		gVehicleInputData.setAnalogRightThrust(0.3f);
	}
}

void startTurnSoftRightMode()
{
	if(gMimicKeyInputs)
	{
		gVehicleInputData.setDigitalAccel(true);
		gVehicleInputData.setDigitalRightThrust(true);
	}
	else
	{
		gVehicleInputData.setAnalogAccel(1.0f);
		gVehicleInputData.setAnalogRightThrust(1.0f);
		gVehicleInputData.setAnalogLeftThrust(0.3f);
	}
}

void releaseAllControls()
{
	if(gMimicKeyInputs)
	{
		gVehicleInputData.setDigitalAccel(false);
		gVehicleInputData.setDigitalRightThrust(false);
		gVehicleInputData.setDigitalLeftThrust(false);
		gVehicleInputData.setDigitalRightBrake(false);
		gVehicleInputData.setDigitalLeftBrake(false);
	}
	else
	{
		gVehicleInputData.setAnalogAccel(0.0f);
		gVehicleInputData.setAnalogRightThrust(0.0f);
		gVehicleInputData.setAnalogLeftThrust(0.0f);
		gVehicleInputData.setAnalogRightBrake(0.0f);
		gVehicleInputData.setAnalogLeftBrake(0.0f);
	}
}

void initPhysics()
{
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
	gPvd = PxCreatePvd(*gFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	gPvd->connect(*transport,PxPvdInstrumentationFlag::eALL);
	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);
		
	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	
	PxU32 numWorkers = 1;
	gDispatcher = PxDefaultCpuDispatcherCreate(numWorkers);
	sceneDesc.cpuDispatcher	= gDispatcher;
	sceneDesc.filterShader	= VehicleFilterShader;

	gScene = gPhysics->createScene(sceneDesc);
	PxPvdSceneClient* pvdClient = gScene->getScenePvdClient();
	if(pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}
	gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

	gCooking = 	PxCreateCooking(PX_PHYSICS_VERSION, *gFoundation, PxCookingParams(PxTolerancesScale()));	

	/////////////////////////////////////////////

	PxInitVehicleSDK(*gPhysics);
	PxVehicleSetBasisVectors(PxVec3(0,1,0), PxVec3(0,0,1));
	PxVehicleSetUpdateMode(PxVehicleUpdateMode::eVELOCITY_CHANGE);

	//Create the batched scene queries for the suspension raycasts.
	gVehicleSceneQueryData = VehicleSceneQueryData::allocate(1, PX_MAX_NB_WHEELS, 1, 1, WheelSceneQueryPreFilterBlocking , NULL, gAllocator);
	gBatchQuery = VehicleSceneQueryData::setUpBatchedSceneQuery(0, *gVehicleSceneQueryData, gScene);

	//Create the friction table for each combination of tire and surface type.
	gFrictionPairs = createFrictionPairs(gMaterial);
	
	//Create a plane to drive on.
	PxFilterData groundPlaneSimFilterData(COLLISION_FLAG_GROUND, COLLISION_FLAG_GROUND_AGAINST, 0, 0);
	gGroundPlane = createDrivablePlane(groundPlaneSimFilterData, gMaterial, gPhysics);
	gScene->addActor(*gGroundPlane);

	//Create a tank that will drive on the plane.
	VehicleDesc tankDesc = initTankDesc();
	gTank = createVehicleTank(tankDesc, gPhysics, gCooking);
	PxTransform startTransform(PxVec3(0, (tankDesc.chassisDims.y*0.5f + tankDesc.wheelRadius + 1.0f), 0), PxQuat(PxIdentity));
	gTank->getRigidDynamicActor()->setGlobalPose(startTransform);
	gScene->addActor(*gTank->getRigidDynamicActor());

	//Set the tank to rest in first gear.
	//Set the tank to use auto-gears.
	//Set the tank to use the standard control model
	gTank->setToRestState();
	gTank->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
	gTank->mDriveDynData.setUseAutoGears(true);
	gTank->setDriveModel(PxVehicleDriveTankControlModel::eSTANDARD);

	gTankModeTimer = 0.0f;
	gTankOrderProgress = 0;
	startBrakeMode();
}

void incrementDrivingMode(const PxF32 timestep)
{
	gTankModeTimer += timestep;
	if(gTankModeTimer > gTankModeLifetime)
	{
		//If the mode just completed was eDRIVE_MODE_ACCEL_REVERSE then switch back to forward gears.
		if(eDRIVE_MODE_ACCEL_REVERSE == gDriveModeOrder[gTankOrderProgress])
		{
			gTank->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
		}

		//Increment to next driving mode.
		gTankModeTimer = 0.0f;
		gTankOrderProgress++;
		releaseAllControls();

		//If we are at the end of the list of driving modes then start again.
		if(eDRIVE_MODE_NONE == gDriveModeOrder[gTankOrderProgress])
		{
			gTankOrderProgress = 0;
			gTankOrderComplete = true;
		}

		//Start driving in the selected mode.
		DriveMode eDriveMode = gDriveModeOrder[gTankOrderProgress];
		switch(eDriveMode)
		{
		case eDRIVE_MODE_ACCEL_FORWARDS:
			startAccelerateForwardsMode();
			break;
		case eDRIVE_MODE_ACCEL_REVERSE:
			startAccelerateReverseMode();
			break;
		case eDRIVE_MODE_HARD_TURN_LEFT:
			startTurnHardLeftMode();
			break;
		case eDRIVE_MODE_SOFT_TURN_LEFT:
			startTurnSoftLeftMode();
			break;
		case eDRIVE_MODE_HARD_TURN_RIGHT:
			startTurnHardRightMode();
			break;
		case eDRIVE_MODE_SOFT_TURN_RIGHT:
			startTurnSoftRightMode();
			break;
		case eDRIVE_MODE_BRAKE:
			startBrakeMode();
			break;
		case eDRIVE_MODE_NONE:
			break;
		};

		//If the mode about to start is eDRIVE_MODE_ACCEL_REVERSE then switch to reverse gears.
		if(eDRIVE_MODE_ACCEL_REVERSE == gDriveModeOrder[gTankOrderProgress])
		{
			gTank->mDriveDynData.forceGearChange(PxVehicleGearsData::eREVERSE);
		}
	}
}

void stepPhysics()
{
	const PxF32 timestep = 1.0f/60.0f;

	//Cycle through the driving modes to demonstrate how to accelerate/reverse/brake/turn.
	incrementDrivingMode(timestep);

	//Update the control inputs for the tank.
	if(gMimicKeyInputs)
	{
		PxVehicleDriveTankSmoothDigitalRawInputsAndSetAnalogInputs(gKeySmoothingData, gVehicleInputData, timestep, *gTank);
	}
	else
	{
		PxVehicleDriveTankSmoothAnalogRawInputsAndSetAnalogInputs(gPadSmoothingData, gVehicleInputData, timestep, *gTank);
	}

	//Raycasts.
	PxVehicleWheels* vehicles[1] = {gTank};
	const PxU32 raycastQueryResultsSize = gVehicleSceneQueryData->getQueryResultBufferSize();
	PxRaycastQueryResult* raycastQueryResults = gVehicleSceneQueryData->getRaycastQueryResultBuffer(0);
	PxVehicleSuspensionRaycasts(gBatchQuery, 1, vehicles, raycastQueryResultsSize, raycastQueryResults);

	//Vehicle update.
	const PxVec3 grav = gScene->getGravity();
	PxWheelQueryResult wheelQueryResults[PX_MAX_NB_WHEELS];
	PxVehicleWheelQueryResult vehicleQueryResults[1] = {{wheelQueryResults, gTank->mWheelsSimData.getNbWheels()}};
	PxVehicleUpdates(timestep, grav, *gFrictionPairs, 1, vehicles, vehicleQueryResults);

	//Scene update.
	gScene->simulate(timestep);
	gScene->fetchResults(true);
}
	
void cleanupPhysics()
{
	gTank->getRigidDynamicActor()->release();
	gTank->free();
	PX_RELEASE(gGroundPlane);
	PX_RELEASE(gBatchQuery);
	gVehicleSceneQueryData->free(gAllocator);
	PX_RELEASE(gFrictionPairs);
	PxCloseVehicleSDK();

	PX_RELEASE(gMaterial);
	PX_RELEASE(gCooking);
	PX_RELEASE(gScene);
	PX_RELEASE(gDispatcher);

	PX_RELEASE(gPhysics);
	if(gPvd)
	{
		PxPvdTransport* transport = gPvd->getTransport();
		gPvd->release();	gPvd = NULL;
		PX_RELEASE(transport);
	}
 	PX_RELEASE(gFoundation);

	printf("SnippetVehicleTank done.\n");
}

void keyPress(unsigned char key, const PxTransform& camera)
{
	PX_UNUSED(camera);
	PX_UNUSED(key);
}

int snippetMain(int, const char*const*)
{
#ifdef RENDER_SNIPPET
	extern void renderLoop();
	renderLoop();
#else
	initPhysics();
	while(!gTankOrderComplete)
	{
		stepPhysics();
	}
	cleanupPhysics();
#endif

	return 0;
}
