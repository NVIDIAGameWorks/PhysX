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
// This snippet illustrates how to use contact modification and sweeps to allow wheels to 
// collide and react more naturally with the scene.  In particular, the snippet shows how to 
// use contact modification and ccd contact modification to select or ignore rigid body contact 
// points between a shape representing a wheel and any other shape in the scene.  The snippet 
// also demonstrates the use of suspension sweeps instead of suspension raycasts.

// The snippet creates various static capsules with different radii, a ground plane, and a 
// vehicle.  The capsules are configured to be drivable surfaces.  Additionally, the 
// capsule and wheel shapes are configured with simulation filter data so that 
// they 
//  (i)  collide with each other with discrete collision detection
// (ii)  collide with each other with continuous collision detection (CCD)
//(iii)  trigger a contact modification callback  
// (iv)  trigger a ccd contact modification callback

// The contact modification callback is implemented with the class WheelContactModifyCallback.  
// The function WheelContactModifyCallback::onContactModify identifies shape pairs that contain
// a wheel.  Contact points for the shape pair are ignored or accepted with the SDK function 
// PxVehicleModifyWheelContacts.  CCD contact modification is implemented with the class 
// WheelCCDContactModifyCallback.  The function WheelCCDContactModifyCallback::onContactModify
// performs exactly the same role as WheelContactModifyCallback::onContactModify

// The threshold values POINT_REJECT_ANGLE and NORMAL_REJECT_ANGLE can be tuned
// to modify the conditions under which wheel contact points are ignored or accepted.

// It is a good idea to record and playback with pvd (PhysX Visual Debugger).
// ****************************************************************************

#include "PxPhysicsAPI.h"
#include <ctype.h>

#include "vehicle/PxVehicleUtil.h"
#include "../snippetvehiclecommon/SnippetVehicleSceneQuery.h"
#include "../snippetvehiclecommon/SnippetVehicleFilterShader.h"
#include "../snippetvehiclecommon/SnippetVehicleTireFriction.h"
#include "../snippetvehiclecommon/SnippetVehicleCreate.h"
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

PxPvd*                  gPvd = NULL;

VehicleSceneQueryData*	gVehicleSceneQueryData = NULL;
PxBatchQuery*			gBatchQuery = NULL;

PxVehicleDrivableSurfaceToTireFrictionPairs* gFrictionPairs = NULL;

#define NUM_VEHICLES 2
PxRigidStatic*			gGroundPlane = NULL;
PxVehicleDrive4W*		gVehicle4W[NUM_VEHICLES] = { NULL, NULL };

ActorUserData gActorUserData[NUM_VEHICLES];
ShapeUserData gShapeUserDatas[NUM_VEHICLES][PX_MAX_NB_WHEELS];
const PxF32 xCoordVehicleStarts[NUM_VEHICLES] = { 0.0f, 20.0f };

//Angle thresholds used to categorize contacts as suspension contacts or rigid body contacts.
#define POINT_REJECT_ANGLE PxPi/4.0f
#define NORMAL_REJECT_ANGLE PxPi/4.0f

//Contact modification values.
#define WHEEL_TANGENT_VELOCITY_MULTIPLIER 0.1f
#define MAX_IMPULSE PX_MAX_F32

//PhysX Vehicles support blocking and non-blocking sweeps.
//Experiment with this define to switch between the two regimes.
#define BLOCKING_SWEEPS 0

//Define the maximum acceleration for dynamic bodies under the wheel.
#define MAX_ACCELERATION 50.0f

//Blocking sweeps require sweep hit buffers for just 1 hit per wheel.
//Non-blocking sweeps require more hits per wheel because they return all touches on the swept shape.
#if BLOCKING_SWEEPS
PxU16					gNbQueryHitsPerWheel = 1;
#else
PxU16					gNbQueryHitsPerWheel = 8;
#endif

//The class WheelContactModifyCallback identifies and modifies rigid body contacts
//that involve a wheel.  Contacts that can be identified and managed by the suspension
//system are ignored.  Any contacts that remain are modified to account for the rotation
//speed of the wheel around the rolling axis.
class WheelContactModifyCallback : public PxContactModifyCallback
{
public:

	WheelContactModifyCallback()
		: PxContactModifyCallback()
	{
	}

	~WheelContactModifyCallback(){}

	void onContactModify(PxContactModifyPair* const pairs, PxU32 count) 
	{
		for(PxU32 i = 0; i < count ; i++)
		{
			const PxRigidActor** actors = pairs[i].actor;
			const PxShape** shapes = pairs[i].shape;

			//Search for actors that represent vehicles and shapes that represent wheels.
			for(PxU32 j = 0; j < 2; j++)
			{
				const PxActor* actor = actors[j];
				if(actor->userData && (static_cast<ActorUserData*>(actor->userData))->vehicle)
				{
					const PxVehicleWheels* vehicle = (static_cast<ActorUserData*>(actor->userData))->vehicle;
					PX_ASSERT(vehicle->getRigidDynamicActor() == actors[j]);

					const PxShape* shape = shapes[j];
					if(shape->userData && (static_cast<ShapeUserData*>(shape->userData))->isWheel)
					{
						const PxU32 wheelId = (static_cast<ShapeUserData*>(shape->userData))->wheelId;
						PX_ASSERT(wheelId < vehicle->mWheelsSimData.getNbWheels());

						//Modify wheel contacts.
						PxVehicleModifyWheelContacts(*vehicle, wheelId, WHEEL_TANGENT_VELOCITY_MULTIPLIER, MAX_IMPULSE, pairs[i]);
					}
				}
			}
		}
	}

private:

};
WheelContactModifyCallback gWheelContactModifyCallback;

//The class WheelCCDContactModifyCallback identifies and modifies ccd contacts
//that involve a wheel.  Contacts that can be identified and managed by the suspension
//system are ignored.  Any contacts that remain are modified to account for the rotation
//speed of the wheel around the rolling axis.
class WheelCCDContactModifyCallback : public PxCCDContactModifyCallback
{
public:

	WheelCCDContactModifyCallback()
		: PxCCDContactModifyCallback()
	{
	}

	~WheelCCDContactModifyCallback(){}

	void onCCDContactModify(PxContactModifyPair* const pairs, PxU32 count) 
	{
		for(PxU32 i = 0; i < count ; i++)
		{
			const PxRigidActor** actors = pairs[i].actor;
			const PxShape** shapes = pairs[i].shape;

			//Search for actors that represent vehicles and shapes that represent wheels.
			for(PxU32 j = 0; j < 2; j++)
			{
				const PxActor* actor = actors[j];
				if(actor->userData && (static_cast<ActorUserData*>(actor->userData))->vehicle)
				{
					const PxVehicleWheels* vehicle = (static_cast<ActorUserData*>(actor->userData))->vehicle;
					PX_ASSERT(vehicle->getRigidDynamicActor() == actors[j]);

					const PxShape* shape = shapes[j];
					if(shape->userData && (static_cast<ShapeUserData*>(shape->userData))->isWheel)
					{
						const PxU32 wheelId = (static_cast<ShapeUserData*>(shape->userData))->wheelId;
						PX_ASSERT(wheelId < vehicle->mWheelsSimData.getNbWheels());

						//Modify wheel contacts.
						PxVehicleModifyWheelContacts(*vehicle, wheelId, WHEEL_TANGENT_VELOCITY_MULTIPLIER, MAX_IMPULSE, pairs[i]);
					}
				}
			}
		}
	}
};
WheelCCDContactModifyCallback gWheelCCDContactModifyCallback;


VehicleDesc initVehicleDesc(const PxFilterData& chassisSimFilterData, const PxFilterData& wheelSimFilterData, const PxU32 vehicleId)
{
	//Set up the chassis mass, dimensions, moment of inertia, and center of mass offset.
	//The moment of inertia is just the moment of inertia of a cuboid but modified for easier steering.
	//Center of mass offset is 0.65m above the base of the chassis and 0.25m towards the front.
	const PxF32 chassisMass = 1500.0f;
	const PxVec3 chassisDims(2.5f,2.0f,5.0f);
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
	const PxU32 nbWheels = 4;

	VehicleDesc vehicleDesc;

	vehicleDesc.chassisMass = chassisMass;
	vehicleDesc.chassisDims = chassisDims;
	vehicleDesc.chassisMOI = chassisMOI;
	vehicleDesc.chassisCMOffset = chassisCMOffset;
	vehicleDesc.chassisMaterial = gMaterial;
	vehicleDesc.chassisSimFilterData = chassisSimFilterData;

	vehicleDesc.wheelMass = wheelMass;
	vehicleDesc.wheelRadius = wheelRadius;
	vehicleDesc.wheelWidth = wheelWidth;
	vehicleDesc.wheelMOI = wheelMOI;
	vehicleDesc.numWheels = nbWheels;
	vehicleDesc.wheelMaterial = gMaterial;
	vehicleDesc.wheelSimFilterData = wheelSimFilterData;

	vehicleDesc.actorUserData = &gActorUserData[vehicleId];
	vehicleDesc.shapeUserDatas = gShapeUserDatas[vehicleId];

	return vehicleDesc;
}

void initPhysics()
{
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
	gPvd = PxCreatePvd(*gFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	gPvd->connect(*transport,PxPvdInstrumentationFlag::eALL);

	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(),true, gPvd);

	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	
	PxU32 numWorkers = 1;
	gDispatcher = PxDefaultCpuDispatcherCreate(numWorkers);
	sceneDesc.cpuDispatcher	= gDispatcher;
	sceneDesc.filterShader	= VehicleFilterShader;							//Set the filter shader
	sceneDesc.contactModifyCallback = &gWheelContactModifyCallback;			//Enable contact modification
	sceneDesc.ccdContactModifyCallback = &gWheelCCDContactModifyCallback;	//Enable ccd contact modification
	sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;							//Enable ccd

	gScene = gPhysics->createScene(sceneDesc);
	PxPvdSceneClient* pvdClient = gScene->getScenePvdClient();
	if(pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}
	gMaterial = gPhysics->createMaterial(0.1f, 0.1f, 0.01f);

	gCooking = 	PxCreateCooking(PX_PHYSICS_VERSION, *gFoundation, PxCookingParams(PxTolerancesScale()));	

	/////////////////////////////////////////////

	PxInitVehicleSDK(*gPhysics);
	PxVehicleSetBasisVectors(PxVec3(0,1,0), PxVec3(0,0,1));
	PxVehicleSetUpdateMode(PxVehicleUpdateMode::eVELOCITY_CHANGE);
	PxVehicleSetSweepHitRejectionAngles(POINT_REJECT_ANGLE, NORMAL_REJECT_ANGLE);
	PxVehicleSetMaxHitActorAcceleration(MAX_ACCELERATION);

	//Create the batched scene queries for the suspension sweeps.
	//Use the post-filter shader to reject hit shapes that overlap the swept wheel at the start pose of the sweep.
	PxQueryHitType::Enum(*sceneQueryPreFilter)(PxFilterData, PxFilterData, const void*, PxU32, PxHitFlags&);
	PxQueryHitType::Enum(*sceneQueryPostFilter)(PxFilterData, PxFilterData, const void*, PxU32, const PxQueryHit&);
#if BLOCKING_SWEEPS
	sceneQueryPreFilter = &WheelSceneQueryPreFilterBlocking;
	sceneQueryPostFilter = &WheelSceneQueryPostFilterBlocking;
#else
	sceneQueryPreFilter = &WheelSceneQueryPreFilterNonBlocking;
	sceneQueryPostFilter = &WheelSceneQueryPostFilterNonBlocking;
#endif 
	gVehicleSceneQueryData = VehicleSceneQueryData::allocate(NUM_VEHICLES, PX_MAX_NB_WHEELS, gNbQueryHitsPerWheel, NUM_VEHICLES, sceneQueryPreFilter, sceneQueryPostFilter, gAllocator);
	gBatchQuery = VehicleSceneQueryData::setUpBatchedSceneQuery(0, *gVehicleSceneQueryData, gScene);

	//Create the friction table for each combination of tire and surface type.
	gFrictionPairs = createFrictionPairs(gMaterial);
	
	//Create a plane to drive on.
	PxFilterData groundPlaneSimFilterData(COLLISION_FLAG_GROUND, COLLISION_FLAG_GROUND_AGAINST, 0, 0);
	gGroundPlane = createDrivablePlane(groundPlaneSimFilterData, gMaterial, gPhysics);
	gScene->addActor(*gGroundPlane);

	//Create several static obstacles for the first vehicle to drive on.
	//  (i) collide only with wheel shapes
	// (ii) have continuous collision detection (CCD) enabled
	//(iii) have contact modification enabled
	// (iv) are configured to be drivable surfaces
	const PxF32 capsuleRadii[4] = {0.05f, 	0.1f, 0.125f, 0.135f};
	const PxF32 capsuleZ[4] = {5.0f, 10.0f, 15.0f, 20.0f};
	for(PxU32 i = 0; i < 4; i++)
	{
		PxTransform t(PxVec3(xCoordVehicleStarts[0], capsuleRadii[i], capsuleZ[i]), PxQuat(PxIdentity));
		PxRigidStatic* rd = gPhysics->createRigidStatic(t);
		PxCapsuleGeometry capsuleGeom(capsuleRadii[i], 3.0f);
		PxShape* shape = PxRigidActorExt::createExclusiveShape(*rd, capsuleGeom, *gMaterial);
		PxFilterData simFilterData(COLLISION_FLAG_OBSTACLE, COLLISION_FLAG_WHEEL, PxPairFlag::eMODIFY_CONTACTS | PxPairFlag::eDETECT_CCD_CONTACT, 0);
		shape->setSimulationFilterData(simFilterData);
		PxFilterData qryFilterData;
		setupDrivableSurface(qryFilterData);
		shape->setQueryFilterData(qryFilterData);
		gScene->addActor(*rd);
	}
	const PxF32 boxHalfHeights[1] = {1.0f};
	const PxF32 boxZ[1] = {30.0f};
	for(PxU32 i = 0; i < 1; i++)
	{
		PxTransform t(PxVec3(xCoordVehicleStarts[0], boxHalfHeights[i], boxZ[i]), PxQuat(PxIdentity));
		PxRigidStatic* rd = gPhysics->createRigidStatic(t);

		PxBoxGeometry boxGeom(PxVec3(3.0f, boxHalfHeights[i], 3.0f));
		PxShape* shape = PxRigidActorExt::createExclusiveShape(*rd, boxGeom, *gMaterial);
		
		PxFilterData simFilterData(COLLISION_FLAG_OBSTACLE, COLLISION_FLAG_WHEEL, PxPairFlag::eMODIFY_CONTACTS | PxPairFlag::eDETECT_CCD_CONTACT, 0);
		shape->setSimulationFilterData(simFilterData);
		PxFilterData qryFilterData;
		setupDrivableSurface(qryFilterData);
		shape->setQueryFilterData(qryFilterData);
		
		gScene->addActor(*rd);
	}

	//Create a pile of dynamic objects for the second vehicle to drive on.
	//  (i) collide only with wheel shapes
	// (ii) have continuous collision detection (CCD) enabled
	//(iii) have contact modification enabled
	// (iv) are configured to be drivable surfaces
	{
		for (PxU32 i = 0; i < 64; i++)
		{
			PxTransform t(PxVec3(xCoordVehicleStarts[1] + i*0.01f, 2.0f + i*0.25f, 20.0f + i*0.025f), PxQuat(PxPi*0.5f, PxVec3(0, 1, 0)));
			PxRigidDynamic* rd = gPhysics->createRigidDynamic(t);
	
			PxBoxGeometry boxGeom(PxVec3(0.08f, 0.25f, 1.0f));
			PxShape* shape = PxRigidActorExt::createExclusiveShape(*rd, boxGeom, *gMaterial);

			PxFilterData simFilterData(COLLISION_FLAG_OBSTACLE, COLLISION_FLAG_OBSTACLE_AGAINST, PxPairFlag::eMODIFY_CONTACTS | PxPairFlag::eDETECT_CCD_CONTACT, 0);
			shape->setSimulationFilterData(simFilterData);
			PxFilterData qryFilterData;
			setupDrivableSurface(qryFilterData);
			shape->setQueryFilterData(qryFilterData);

			PxRigidBodyExt::updateMassAndInertia(*rd, 30.0f);

			gScene->addActor(*rd);
		}
	}

	//Create two vehicles that will drive on the obstacles.
	//The vehicles are configured with wheels that 
	//  (i) collide with obstacles
	// (ii) have continuous collision detection (CCD) enabled
	//(iii) have contact modification enabled
	//The vehicle chassis only collides with the ground to highlight the collision between the wheels and the obstacles.
	for (PxU32 i = 0; i < NUM_VEHICLES; i++)
	{
		PxFilterData chassisSimFilterData(COLLISION_FLAG_CHASSIS, COLLISION_FLAG_GROUND, 0, 0);
		PxFilterData wheelSimFilterData(COLLISION_FLAG_WHEEL, COLLISION_FLAG_WHEEL, PxPairFlag::eDETECT_CCD_CONTACT | PxPairFlag::eMODIFY_CONTACTS, 0);
		VehicleDesc vehicleDesc = initVehicleDesc(chassisSimFilterData, wheelSimFilterData, i);
		gVehicle4W[i] = createVehicle4W(vehicleDesc, gPhysics, gCooking);
		PxTransform startTransform(PxVec3(xCoordVehicleStarts[i], (vehicleDesc.chassisDims.y*0.5f + vehicleDesc.wheelRadius + 1.0f), 0), PxQuat(PxIdentity));
		gVehicle4W[i]->getRigidDynamicActor()->setGlobalPose(startTransform);
		gVehicle4W[i]->getRigidDynamicActor()->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
		gScene->addActor(*gVehicle4W[i]->getRigidDynamicActor());

		//Set the vehicle to rest in first gear.
		//Set the vehicle to use auto-gears.
		gVehicle4W[i]->setToRestState();
		gVehicle4W[i]->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
		gVehicle4W[i]->mDriveDynData.setUseAutoGears(true);
	}
}

void stepPhysics()
{
	const PxF32 timestep = 1.0f/60.0f;

	//Set the vehicles to accelerate forwards.
	for (PxU32 i = 0; i < 2; i++)
	{
		gVehicle4W[i]->mDriveDynData.setAnalogInput(PxVehicleDrive4WControl::eANALOG_INPUT_ACCEL, 0.55f);
	}

	//Scene update.
	gScene->simulate(timestep);
	gScene->fetchResults(true);

	//Suspension sweeps (instead of raycasts).
	//Sweeps provide more information about the geometry under the wheel.
	PxVehicleWheels* vehicles[NUM_VEHICLES] = {gVehicle4W[0], gVehicle4W[1]};
	PxSweepQueryResult* sweepResults = gVehicleSceneQueryData->getSweepQueryResultBuffer(0);
	const PxU32 sweepResultsSize = gVehicleSceneQueryData->getQueryResultBufferSize();
	PxVehicleSuspensionSweeps(gBatchQuery, NUM_VEHICLES, vehicles, sweepResultsSize, sweepResults, gNbQueryHitsPerWheel, NULL, 1.0f, 1.01f);

	//Vehicle update.
	const PxVec3 grav = gScene->getGravity();
	PxWheelQueryResult wheelQueryResults[PX_MAX_NB_WHEELS][NUM_VEHICLES];
	PxVehicleWheelQueryResult vehicleQueryResults[NUM_VEHICLES] = 
	{
		{ wheelQueryResults[0], gVehicle4W[0]->mWheelsSimData.getNbWheels() },
		{ wheelQueryResults[1] , gVehicle4W[1]->mWheelsSimData.getNbWheels() },
	};
	PxVehicleUpdates(timestep, grav, *gFrictionPairs, NUM_VEHICLES, vehicles, vehicleQueryResults);
}
	
void cleanupPhysics()
{
	for (PxU32 i = 0; i < NUM_VEHICLES; i++)
	{
		gVehicle4W[i]->getRigidDynamicActor()->release();
		gVehicle4W[i]->free();
	}
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

	printf("SnippetVehicleContactMod done.\n");
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
	PxU32 count = 0;
	PxU32 maxCount =1000;
	while(count < maxCount)
	{
		stepPhysics();
		count++;
	}
	cleanupPhysics();
#endif

	return 0;
}
