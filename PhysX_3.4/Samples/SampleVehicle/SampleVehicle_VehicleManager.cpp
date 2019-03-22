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

#include "SampleVehicle.h"
#include "SampleVehicle_VehicleManager.h"
#include "SampleVehicle_VehicleCooking.h"
#include "SampleVehicle_SceneQuery.h"
#include "SampleVehicle_WheelQueryResults.h"
#include "RendererMemoryMacros.h"
#include "vehicle/PxVehicleSDK.h"
#include "vehicle/PxVehicleDrive4W.h"
#include "vehicle/PxVehicleDriveNW.h"
#include "vehicle/PxVehicleDriveTank.h"
#include "vehicle/PxVehicleUpdate.h"
#include "vehicle/PxVehicleTireFriction.h"
#include "vehicle/PxVehicleUtilSetup.h"
#include "PxRigidDynamic.h"
#include "PxScene.h"
#include "geometry/PxConvexMesh.h"
#include "geometry/PxConvexMeshGeometry.h"
#include "SampleAllocatorSDKClasses.h"

///////////////////////////////////

SampleVehicle_VehicleManager::SampleVehicle_VehicleManager() 
:	mNumVehicles(0),
	mSqWheelRaycastBatchQuery(NULL),
	mIsIn3WMode(false),
	mSerializationRegistry(NULL)
{
}

SampleVehicle_VehicleManager::~SampleVehicle_VehicleManager()
{
}

void SampleVehicle_VehicleManager::init(PxPhysics& physics, const PxMaterial** drivableSurfaceMaterials, const PxVehicleDrivableSurfaceType* drivableSurfaceTypes)
{
#if defined(SERIALIZE_VEHICLE_RPEX) || defined(SERIALIZE_VEHICLE_BINARY)
	mSerializationRegistry = PxSerialization::createSerializationRegistry(physics);
#endif

	//Initialise the sdk.
	PxInitVehicleSDK(physics, mSerializationRegistry);

	//Set the basis vectors.
	PxVec3 up(0,1,0);
	PxVec3 forward(0,0,1);
	PxVehicleSetBasisVectors(up,forward);

	//Set the vehicle update mode to be immediate velocity changes.
	PxVehicleSetUpdateMode(PxVehicleUpdateMode::eVELOCITY_CHANGE);
	
	//Initialise all vehicle ptrs to null.
	for(PxU32 i=0;i<MAX_NUM_4W_VEHICLES+MAX_NUM_6W_VEHICLES;i++)
	{
		mVehicles[i]=NULL;
	}

	//Allocate simulation data so we can switch from 3-wheeled to 4-wheeled cars by switching simulation data.
	mWheelsSimData4W = PxVehicleWheelsSimData::allocate(4);

	//Scene query data for to allow raycasts for all suspensions of all vehicles.
	mSqData = SampleVehicleSceneQueryData::allocate(MAX_NUM_4W_VEHICLES*4 + MAX_NUM_6W_VEHICLES*6);

	//Data to store reports for each wheel.
	mWheelQueryResults = SampleVehicleWheelQueryResults::allocate(MAX_NUM_4W_VEHICLES*4 + MAX_NUM_6W_VEHICLES*6);

	//Set up the friction values arising from combinations of tire type and surface type.
	mSurfaceTirePairs=PxVehicleDrivableSurfaceToTireFrictionPairs::allocate(MAX_NUM_TIRE_TYPES,MAX_NUM_SURFACE_TYPES);
	mSurfaceTirePairs->setup(MAX_NUM_TIRE_TYPES,MAX_NUM_SURFACE_TYPES,drivableSurfaceMaterials,drivableSurfaceTypes);
	for(PxU32 i=0;i<MAX_NUM_SURFACE_TYPES;i++)
	{
		for(PxU32 j=0;j<MAX_NUM_TIRE_TYPES;j++)
		{
			mSurfaceTirePairs->setTypePairFriction(i,j,TireFrictionMultipliers::getValue(i, j));
		}
	}
}

void SampleVehicle_VehicleManager::shutdown()
{
	//Remove the N-wheeled vehicles.
	for(PxU32 i=0;i<mNumVehicles;i++)
	{
		switch(mVehicles[i]->getVehicleType())
		{
		case PxVehicleTypes::eDRIVE4W:
			{
				PxVehicleDrive4W* veh=(PxVehicleDrive4W*)mVehicles[i];
				veh->free();
			}
			break;
		case PxVehicleTypes::eDRIVENW:
			{
				PxVehicleDriveNW* veh=(PxVehicleDriveNW*)mVehicles[i];
				veh->free();
			}
			break;
		case PxVehicleTypes::eDRIVETANK:
			{
				PxVehicleDriveTank* veh=(PxVehicleDriveTank*)mVehicles[i];
				veh->free();
			}
			break;
		default:
			PX_ASSERT(false);
			break;
		}
	}

	//Deallocate simulation data that was used to switch from 3-wheeled to 4-wheeled cars by switching simulation data.
	mWheelsSimData4W->free();

	//Deallocate scene query data that was used for suspension raycasts.
	mSqData->free();

	//Deallocate buffers that store wheel reports.
	mWheelQueryResults->free();

	//Release the  friction values used for combinations of tire type and surface type.
	mSurfaceTirePairs->release();

	//Scene query.
	if(mSqWheelRaycastBatchQuery)
	{
		mSqWheelRaycastBatchQuery=NULL;
	}

	PxCloseVehicleSDK(mSerializationRegistry);

	if(mSerializationRegistry)
		mSerializationRegistry->release();
}

void SampleVehicle_VehicleManager::addVehicle(const PxU32 i, PxVehicleWheels* vehicle)		
{ 
	mVehicles[i] = vehicle; 

	const PxU32 numWheels = vehicle->mWheelsSimData.getNbWheels();
	mVehicleWheelQueryResults[i].nbWheelQueryResults = numWheels;
	mVehicleWheelQueryResults[i].wheelQueryResults = mWheelQueryResults->addVehicle(numWheels);

	mNumVehicles++;
}


void SampleVehicle_VehicleManager::resetNWCar(const PxTransform& transform, const PxU32 vehicleId)
{
	PX_ASSERT(vehicleId<mNumVehicles);
	resetNWCar(transform,mVehicles[vehicleId]);
}

void SampleVehicle_VehicleManager::suspensionRaycasts(PxScene* scene)
{
	//Create a scene query if we haven't already done so.
	if(NULL==mSqWheelRaycastBatchQuery)
	{
		mSqWheelRaycastBatchQuery=mSqData->setUpBatchedSceneQuery(scene);
	}
	//Raycasts.
	PxSceneReadLock scopedLock(*scene);
	PxVehicleSuspensionRaycasts(mSqWheelRaycastBatchQuery,mNumVehicles,mVehicles,mSqData->getRaycastQueryResultBufferSize(),mSqData->getRaycastQueryResultBuffer());
}


#if PX_DEBUG_VEHICLE_ON

void SampleVehicle_VehicleManager::updateAndRecordTelemetryData
(const PxF32 timestep, const PxVec3& gravity, PxVehicleWheels* focusVehicle, PxVehicleTelemetryData* telemetryData)
{
	PX_ASSERT(focusVehicle && telemetryData);

	//Update all vehicles except for focusVehicle.
	PxVehicleWheels* vehicles[MAX_NUM_4W_VEHICLES+MAX_NUM_6W_VEHICLES];
	PxVehicleWheelQueryResult vehicleWheelQueryResults[MAX_NUM_4W_VEHICLES+MAX_NUM_6W_VEHICLES];
	PxVehicleWheelQueryResult focusVehicleWheelQueryResults[1];
	PxU32 numVehicles=0;
	for(PxU32 i=0;i<mNumVehicles;i++)
	{
		if(focusVehicle!=mVehicles[i])
		{
			vehicles[numVehicles]=mVehicles[i];
			vehicleWheelQueryResults[numVehicles]=mVehicleWheelQueryResults[i];
			numVehicles++;
		}
		else
		{
			focusVehicleWheelQueryResults[0]=mVehicleWheelQueryResults[i];
		}
	}
	PxVehicleUpdates(timestep,gravity,*mSurfaceTirePairs,numVehicles,vehicles,vehicleWheelQueryResults);


	//Update the vehicle for which we want to record debug data.
	PxVehicleUpdateSingleVehicleAndStoreTelemetryData(timestep,gravity,*mSurfaceTirePairs,focusVehicle,focusVehicleWheelQueryResults,*telemetryData);
}

#else

void SampleVehicle_VehicleManager::update(const PxF32 timestep, const PxVec3& gravity)
{
	PxVehicleUpdates(timestep,gravity,*mSurfaceTirePairs,mNumVehicles,mVehicles,mVehicleWheelQueryResults);
}

#endif //PX_DEBUG_VEHICLE_ON

void SampleVehicle_VehicleManager::resetNWCar(const PxTransform& startTransform, PxVehicleWheels* vehWheels)
{
	switch(vehWheels->getVehicleType())
	{
	case PxVehicleTypes::eDRIVE4W:
		{
			PxVehicleDrive4W* vehDrive4W=(PxVehicleDrive4W*)vehWheels;
			//Set the car back to its rest state.
			vehDrive4W->setToRestState();
			//Set the car to first gear.
			vehDrive4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
		}
		break;
	case PxVehicleTypes::eDRIVENW:
		{
			PxVehicleDriveNW* vehDriveNW=(PxVehicleDriveNW*)vehWheels;
			//Set the car back to its rest state.
			vehDriveNW->setToRestState();
			//Set the car to first gear.
			vehDriveNW->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
		}
		break;
	case PxVehicleTypes::eDRIVETANK:
		{
			PxVehicleDriveTank* vehDriveTank=(PxVehicleDriveTank*)vehWheels;
			//Set the car back to its rest state.
			vehDriveTank->setToRestState();
			//Set the car to first gear.
			vehDriveTank->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
		}
		break;
	default:
		PX_ASSERT(false);
		break;
	}

	//Set the car's transform to be the start transform.
	PxRigidDynamic* actor=vehWheels->getRigidDynamicActor();
	PxSceneWriteLock scopedLock(*actor->getScene());
	actor->setGlobalPose(startTransform);
}

void setupActor
(PxRigidDynamic* vehActor, 
 const PxFilterData& vehQryFilterData,
 const PxGeometry** wheelGeometries, const PxTransform* wheelLocalPoses, const PxU32 numWheelGeometries, const PxMaterial* wheelMaterial, const PxFilterData& wheelCollFilterData,
 const PxGeometry** chassisGeometries, const PxTransform* chassisLocalPoses, const PxU32 numChassisGeometries, const PxMaterial* chassisMaterial, const PxFilterData& chassisCollFilterData,
 const PxVehicleChassisData& chassisData,
 PxPhysics* physics)
{
	//Add all the wheel shapes to the actor.
	for(PxU32 i=0;i<numWheelGeometries;i++)
	{
		PxShape* wheelShape=PxRigidActorExt::createExclusiveShape(*vehActor, *wheelGeometries[i],*wheelMaterial);
		wheelShape->setQueryFilterData(vehQryFilterData);
		wheelShape->setSimulationFilterData(wheelCollFilterData);
		wheelShape->setLocalPose(wheelLocalPoses[i]);
	}

	//Add the chassis shapes to the actor.
	for(PxU32 i=0;i<numChassisGeometries;i++)
	{
		PxShape* chassisShape=PxRigidActorExt::createExclusiveShape(*vehActor, *chassisGeometries[i],*chassisMaterial);
		chassisShape->setQueryFilterData(vehQryFilterData);
		chassisShape->setSimulationFilterData(chassisCollFilterData);
		chassisShape->setLocalPose(chassisLocalPoses[i]);
	}

	vehActor->setMass(chassisData.mMass);
	vehActor->setMassSpaceInertiaTensor(chassisData.mMOI);
	vehActor->setCMassLocalPose(PxTransform(chassisData.mCMOffset,PxQuat(PxIdentity)));
}

PxRigidDynamic* createVehicleActor4W
(const PxVehicleChassisData& chassisData,
 PxConvexMesh** wheelConvexMeshes, PxConvexMesh* chassisConvexMesh, 
 PxScene& scene, PxPhysics& physics, const PxMaterial& material)
{
	//We need a rigid body actor for the vehicle.
	//Don't forget to add the actor the scene after setting up the associated vehicle.
	PxRigidDynamic* vehActor=physics.createRigidDynamic(PxTransform(PxIdentity));

	//We need to add wheel collision shapes, their local poses, a material for the wheels, and a simulation filter for the wheels.
	PxConvexMeshGeometry frontLeftWheelGeom(wheelConvexMeshes[0]);
	PxConvexMeshGeometry frontRightWheelGeom(wheelConvexMeshes[1]);
	PxConvexMeshGeometry rearLeftWheelGeom(wheelConvexMeshes[2]);
	PxConvexMeshGeometry rearRightWheelGeom(wheelConvexMeshes[3]);
	const PxGeometry* wheelGeometries[4]={&frontLeftWheelGeom,&frontRightWheelGeom,&rearLeftWheelGeom,&rearRightWheelGeom};
	const PxTransform wheelLocalPoses[4]={PxTransform(PxIdentity),PxTransform(PxIdentity),PxTransform(PxIdentity),PxTransform(PxIdentity)};
	const PxMaterial& wheelMaterial=material;
	PxFilterData wheelCollFilterData;
	wheelCollFilterData.word0=COLLISION_FLAG_WHEEL;
	wheelCollFilterData.word1=COLLISION_FLAG_WHEEL_AGAINST;

	//We need to add chassis collision shapes, their local poses, a material for the chassis, and a simulation filter for the chassis.
	PxConvexMeshGeometry chassisConvexGeom(chassisConvexMesh);
	const PxGeometry* chassisGeoms[1]={&chassisConvexGeom};
	const PxTransform chassisLocalPoses[1]={PxTransform(PxIdentity)};
	const PxMaterial& chassisMaterial=material;
	PxFilterData chassisCollFilterData;
	chassisCollFilterData.word0=COLLISION_FLAG_CHASSIS;
	chassisCollFilterData.word1=COLLISION_FLAG_CHASSIS_AGAINST;

	//Create a query filter data for the car to ensure that cars
	//do not attempt to drive on themselves.
	PxFilterData vehQryFilterData;
	SampleVehicleSetupVehicleShapeQueryFilterData(&vehQryFilterData);

	//Set up the physx rigid body actor with shapes, local poses, and filters.
	setupActor
		(vehActor,
		vehQryFilterData,
		wheelGeometries,wheelLocalPoses,4,&wheelMaterial,wheelCollFilterData,
		chassisGeoms,chassisLocalPoses,1,&chassisMaterial,chassisCollFilterData,
		chassisData,
		&physics);

	return vehActor;
}

void computeWheelWidthsAndRadii(PxConvexMesh** wheelConvexMeshes, PxF32* wheelWidths, PxF32* wheelRadii)
{
	for(PxU32 i=0;i<4;i++)
	{
		const PxU32 numWheelVerts=wheelConvexMeshes[i]->getNbVertices();
		const PxVec3* wheelVerts=wheelConvexMeshes[i]->getVertices();
		PxVec3 wheelMin(PX_MAX_F32,PX_MAX_F32,PX_MAX_F32);
		PxVec3 wheelMax(-PX_MAX_F32,-PX_MAX_F32,-PX_MAX_F32);
		for(PxU32 j=0;j<numWheelVerts;j++)
		{
			wheelMin.x=PxMin(wheelMin.x,wheelVerts[j].x);
			wheelMin.y=PxMin(wheelMin.y,wheelVerts[j].y);
			wheelMin.z=PxMin(wheelMin.z,wheelVerts[j].z);
			wheelMax.x=PxMax(wheelMax.x,wheelVerts[j].x);
			wheelMax.y=PxMax(wheelMax.y,wheelVerts[j].y);
			wheelMax.z=PxMax(wheelMax.z,wheelVerts[j].z);
		}
		wheelWidths[i]=wheelMax.x-wheelMin.x;
		wheelRadii[i]=PxMax(wheelMax.y,wheelMax.z)*0.975f;
	}
}

PxVec3 computeChassisAABBDimensions(const PxConvexMesh* chassisConvexMesh)
{
	const PxU32 numChassisVerts=chassisConvexMesh->getNbVertices();
	const PxVec3* chassisVerts=chassisConvexMesh->getVertices();
	PxVec3 chassisMin(PX_MAX_F32,PX_MAX_F32,PX_MAX_F32);
	PxVec3 chassisMax(-PX_MAX_F32,-PX_MAX_F32,-PX_MAX_F32);
	for(PxU32 i=0;i<numChassisVerts;i++)
	{
		chassisMin.x=PxMin(chassisMin.x,chassisVerts[i].x);
		chassisMin.y=PxMin(chassisMin.y,chassisVerts[i].y);
		chassisMin.z=PxMin(chassisMin.z,chassisVerts[i].z);
		chassisMax.x=PxMax(chassisMax.x,chassisVerts[i].x);
		chassisMax.y=PxMax(chassisMax.y,chassisVerts[i].y);
		chassisMax.z=PxMax(chassisMax.z,chassisVerts[i].z);
	}
	const PxVec3 chassisDims=chassisMax-chassisMin;
	return chassisDims;
}

void createVehicle4WSimulationData
(const PxF32 chassisMass, PxConvexMesh* chassisConvexMesh, 
 const PxF32 wheelMass, PxConvexMesh** wheelConvexMeshes, const PxVec3* wheelCentreOffsets, 
 PxVehicleWheelsSimData& wheelsData, PxVehicleDriveSimData4W& driveData, PxVehicleChassisData& chassisData)
{
	//Extract the chassis AABB dimensions from the chassis convex mesh.
	const PxVec3 chassisDims=computeChassisAABBDimensions(chassisConvexMesh);

	//The origin is at the center of the chassis mesh.
	//Set the center of mass to be below this point and a little towards the front.
	const PxVec3 chassisCMOffset=PxVec3(0.0f,-chassisDims.y*0.5f+0.65f,0.25f);

	//Now compute the chassis mass and moment of inertia.
	//Use the moment of inertia of a cuboid as an approximate value for the chassis moi.
	PxVec3 chassisMOI
		((chassisDims.y*chassisDims.y + chassisDims.z*chassisDims.z)*chassisMass/12.0f,
		 (chassisDims.x*chassisDims.x + chassisDims.z*chassisDims.z)*chassisMass/12.0f,
		 (chassisDims.x*chassisDims.x + chassisDims.y*chassisDims.y)*chassisMass/12.0f);
	//A bit of tweaking here.  The car will have more responsive turning if we reduce the 	
	//y-component of the chassis moment of inertia.
	chassisMOI.y*=0.8f;

	//Let's set up the chassis data structure now.
	chassisData.mMass=chassisMass;
	chassisData.mMOI=chassisMOI;
	chassisData.mCMOffset=chassisCMOffset;

	//Compute the sprung masses of each suspension spring using a helper function.
	PxF32 suspSprungMasses[4];
	PxVehicleComputeSprungMasses(4,wheelCentreOffsets,chassisCMOffset,chassisMass,1,suspSprungMasses);

	//Extract the wheel radius and width from the wheel convex meshes.
	PxF32 wheelWidths[4];
	PxF32 wheelRadii[4];
	computeWheelWidthsAndRadii(wheelConvexMeshes,wheelWidths,wheelRadii);

	//Now compute the wheel masses and inertias components around the axle's axis.
	//http://en.wikipedia.org/wiki/List_of_moments_of_inertia
	PxF32 wheelMOIs[4];
	for(PxU32 i=0;i<4;i++)
	{
		wheelMOIs[i]=0.5f*wheelMass*wheelRadii[i]*wheelRadii[i];
	}
	//Let's set up the wheel data structures now with radius, mass, and moi.
	PxVehicleWheelData wheels[4];
	for(PxU32 i=0;i<4;i++)
	{
		wheels[i].mRadius=wheelRadii[i];
		wheels[i].mMass=wheelMass;
		wheels[i].mMOI=wheelMOIs[i];
		wheels[i].mWidth=wheelWidths[i];
	}
	//Disable the handbrake from the front wheels and enable for the rear wheels
	wheels[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mMaxHandBrakeTorque=0.0f;
	wheels[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mMaxHandBrakeTorque=0.0f;
	wheels[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mMaxHandBrakeTorque=4000.0f;
	wheels[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mMaxHandBrakeTorque=4000.0f;
	//Enable steering for the front wheels and disable for the front wheels.
	wheels[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mMaxSteer=PxPi*0.3333f;
	wheels[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mMaxSteer=PxPi*0.3333f;
	wheels[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mMaxSteer=0.0f;
	wheels[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mMaxSteer=0.0f;

	//Let's set up the tire data structures now.
	//Put slicks on the front tires and wets on the rear tires.
	PxVehicleTireData tires[4];
	tires[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mType=TIRE_TYPE_SLICKS;
	tires[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mType=TIRE_TYPE_SLICKS;
	tires[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mType=TIRE_TYPE_WETS;
	tires[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mType=TIRE_TYPE_WETS;

	//Let's set up the suspension data structures now.
	PxVehicleSuspensionData susps[4];
	for(PxU32 i=0;i<4;i++)
	{
		susps[i].mMaxCompression=0.3f;
		susps[i].mMaxDroop=0.1f;
		susps[i].mSpringStrength=35000.0f;	
		susps[i].mSpringDamperRate=4500.0f;
	}
	susps[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mSprungMass=suspSprungMasses[PxVehicleDrive4WWheelOrder::eFRONT_LEFT];
	susps[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mSprungMass=suspSprungMasses[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT];
	susps[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mSprungMass=suspSprungMasses[PxVehicleDrive4WWheelOrder::eREAR_LEFT];
	susps[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mSprungMass=suspSprungMasses[PxVehicleDrive4WWheelOrder::eREAR_RIGHT];

	//Set up the camber.
	//Remember that the left and right wheels need opposite camber so that the car preserves symmetry about the forward direction.
	//Set the camber to 0.0f when the spring is neither compressed or elongated.
	const PxF32 camberAngleAtRest=0.0;
	susps[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mCamberAtRest=camberAngleAtRest;
	susps[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mCamberAtRest=-camberAngleAtRest;
	susps[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mCamberAtRest=camberAngleAtRest;
	susps[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mCamberAtRest=-camberAngleAtRest;
	//Set the wheels to camber inwards at maximum droop (the left and right wheels almost form a V shape)
	const PxF32 camberAngleAtMaxDroop=0.001f;
	susps[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mCamberAtMaxDroop=camberAngleAtMaxDroop;
	susps[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mCamberAtMaxDroop=-camberAngleAtMaxDroop;
	susps[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mCamberAtMaxDroop=camberAngleAtMaxDroop;
	susps[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mCamberAtMaxDroop=-camberAngleAtMaxDroop;
	//Set the wheels to camber outwards at maximum compression (the left and right wheels almost form a A shape).
	const PxF32 camberAngleAtMaxCompression=-0.001f;
	susps[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mCamberAtMaxCompression=camberAngleAtMaxCompression;
	susps[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mCamberAtMaxCompression=-camberAngleAtMaxCompression;
	susps[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mCamberAtMaxCompression=camberAngleAtMaxCompression;
	susps[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mCamberAtMaxCompression=-camberAngleAtMaxCompression;

	//We need to set up geometry data for the suspension, wheels, and tires.
	//We already know the wheel centers described as offsets from the actor center and the center of mass offset from actor center.
	//From here we can approximate application points for the tire and suspension forces.
	//Lets assume that the suspension travel directions are absolutely vertical.
	//Also assume that we apply the tire and suspension forces 30cm below the center of mass.
	PxVec3 suspTravelDirections[4]={PxVec3(0,-1,0),PxVec3(0,-1,0),PxVec3(0,-1,0),PxVec3(0,-1,0)};
	PxVec3 wheelCentreCMOffsets[4];
	PxVec3 suspForceAppCMOffsets[4];
	PxVec3 tireForceAppCMOffsets[4];
	for(PxU32 i=0;i<4;i++)
	{
		wheelCentreCMOffsets[i]=wheelCentreOffsets[i]-chassisCMOffset;
		suspForceAppCMOffsets[i]=PxVec3(wheelCentreCMOffsets[i].x,-0.3f,wheelCentreCMOffsets[i].z);
		tireForceAppCMOffsets[i]=PxVec3(wheelCentreCMOffsets[i].x,-0.3f,wheelCentreCMOffsets[i].z);
	}

	//Now add the wheel, tire and suspension data.
	for(PxU32 i=0;i<4;i++)
	{
		wheelsData.setWheelData(i,wheels[i]);
		wheelsData.setTireData(i,tires[i]);
		wheelsData.setSuspensionData(i,susps[i]);
		wheelsData.setSuspTravelDirection(i,suspTravelDirections[i]);
		wheelsData.setWheelCentreOffset(i,wheelCentreCMOffsets[i]);
		wheelsData.setSuspForceAppPointOffset(i,suspForceAppCMOffsets[i]);
		wheelsData.setTireForceAppPointOffset(i,tireForceAppCMOffsets[i]);
	}

	//Set the car to perform 3 sub-steps when it moves with a forwards speed of less than 5.0 
	//and with a single step when it moves at speed greater than or equal to 5.0.
	wheelsData.setSubStepCount(5.0f, 3, 1);


	//Now set up the differential, engine, gears, clutch, and ackermann steering.

	//Diff
	PxVehicleDifferential4WData diff;
	diff.mType=PxVehicleDifferential4WData::eDIFF_TYPE_LS_4WD;
	driveData.setDiffData(diff);
	
	//Engine
	PxVehicleEngineData engine;
	engine.mPeakTorque=500.0f;
	engine.mMaxOmega=600.0f;//approx 6000 rpm
	driveData.setEngineData(engine);

	//Gears
	PxVehicleGearsData gears;
	gears.mSwitchTime=0.5f;
	driveData.setGearsData(gears);

	//Clutch
	PxVehicleClutchData clutch;
	clutch.mStrength=10.0f;
	driveData.setClutchData(clutch);

	//Ackermann steer accuracy
	PxVehicleAckermannGeometryData ackermann;
	ackermann.mAccuracy=1.0f;
	ackermann.mAxleSeparation=wheelCentreOffsets[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].z-wheelCentreOffsets[PxVehicleDrive4WWheelOrder::eREAR_LEFT].z;
	ackermann.mFrontWidth=wheelCentreOffsets[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].x-wheelCentreOffsets[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].x;
	ackermann.mRearWidth=wheelCentreOffsets[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].x-wheelCentreOffsets[PxVehicleDrive4WWheelOrder::eREAR_LEFT].x;
	driveData.setAckermannGeometryData(ackermann);
}

void SampleVehicle_VehicleManager::create4WVehicle
(PxScene& scene, PxPhysics& physics, PxCooking& cooking, const PxMaterial& material,
 const PxF32 chassisMass, const PxVec3* wheelCentreOffsets4, PxConvexMesh* chassisConvexMesh, PxConvexMesh** wheelConvexMeshes4,
 const PxTransform& startTransform, const bool useAutoGearFlag)
{
	PX_ASSERT(mNumVehicles<MAX_NUM_4W_VEHICLES);

	PxVehicleWheelsSimData* wheelsSimData=PxVehicleWheelsSimData::allocate(4);
	PxVehicleDriveSimData4W driveSimData;
	PxVehicleChassisData chassisData;
	createVehicle4WSimulationData
		(chassisMass,chassisConvexMesh,
		 20.0f,wheelConvexMeshes4,wheelCentreOffsets4,
		 *wheelsSimData,driveSimData,chassisData);
		
	//Instantiate and finalize the vehicle using physx.
	PxRigidDynamic* vehActor=createVehicleActor4W(chassisData,wheelConvexMeshes4,chassisConvexMesh,scene,physics,material);

	//Create a car.
	PxVehicleDrive4W* car = PxVehicleDrive4W::allocate(4);
	car->setup(&physics,vehActor,*wheelsSimData,driveSimData,0);

	//Free the sim data because we don't need that any more.
	wheelsSimData->free();

	//Don't forget to add the actor to the scene.
	{
		PxSceneWriteLock scopedLock(scene);
		scene.addActor(*vehActor);
	}


	//Set up the mapping between wheel and actor shape.
	car->mWheelsSimData.setWheelShapeMapping(0,0);
	car->mWheelsSimData.setWheelShapeMapping(1,1);
	car->mWheelsSimData.setWheelShapeMapping(2,2);
	car->mWheelsSimData.setWheelShapeMapping(3,3);

	//Set up the scene query filter data for each suspension line.
	PxFilterData vehQryFilterData;
	SampleVehicleSetupVehicleShapeQueryFilterData(&vehQryFilterData);
	car->mWheelsSimData.setSceneQueryFilterData(0, vehQryFilterData);
	car->mWheelsSimData.setSceneQueryFilterData(1, vehQryFilterData);
	car->mWheelsSimData.setSceneQueryFilterData(2, vehQryFilterData);
	car->mWheelsSimData.setSceneQueryFilterData(3, vehQryFilterData);

	//Set the transform and the instantiated car and set it be to be at rest.
	resetNWCar(startTransform,car);
	//Set the autogear mode of the instantiate car.
	car->mDriveDynData.setUseAutoGears(useAutoGearFlag);

	//Increment the number of vehicles
	mVehicles[mNumVehicles]=car;
	mVehicleWheelQueryResults[mNumVehicles].nbWheelQueryResults=4;
	mVehicleWheelQueryResults[mNumVehicles].wheelQueryResults=mWheelQueryResults->addVehicle(4);
	mNumVehicles++;
}

PxRigidDynamic* createVehicleActor6W
(const PxVehicleChassisData& chassisData,
 PxConvexMesh** wheelConvexMeshes, PxConvexMesh* chassisConvexMesh, 
 PxScene& scene, PxPhysics& physics, const PxMaterial& material)
{
	PxTransform ident=PxTransform(PxIdentity);

	//We need a rigid body actor for the vehicle.
	//Don't forget to add the actor the scene after setting up the associated vehicle.
	PxRigidDynamic* vehActor=physics.createRigidDynamic(PxTransform(PxIdentity));

	//We need to add wheel collision shapes, their local poses, a material for the wheels, and a simulation filter for the wheels.
	PxConvexMeshGeometry frontLeftWheelGeom(wheelConvexMeshes[0]);
	PxConvexMeshGeometry frontRightWheelGeom(wheelConvexMeshes[1]);
	PxConvexMeshGeometry rearLeftWheelGeom(wheelConvexMeshes[2]);
	PxConvexMeshGeometry rearRightWheelGeom(wheelConvexMeshes[3]);
	PxConvexMeshGeometry extraWheel0(wheelConvexMeshes[0]);
	PxConvexMeshGeometry extraWheel1(wheelConvexMeshes[1]);
	const PxGeometry* wheelGeoms[6]={&frontLeftWheelGeom,&frontRightWheelGeom,&rearLeftWheelGeom,&rearRightWheelGeom,&extraWheel0,&extraWheel1};
	const PxTransform wheelLocalPoses[6]={ident,ident,ident,ident,ident,ident};
	const PxMaterial& wheelMaterial=material;
	PxFilterData wheelCollFilterData;
	wheelCollFilterData.word0=COLLISION_FLAG_WHEEL;
	wheelCollFilterData.word1=COLLISION_FLAG_WHEEL_AGAINST;

	//We need to add chassis collision shapes, their local poses, a material for the chassis, and a simulation filter for the chassis.
	PxConvexMeshGeometry chassisConvexGeom(chassisConvexMesh);
	const PxGeometry* chassisGeoms[1]={&chassisConvexGeom};
	const PxTransform chassisLocalPoses[1]={ident};
	const PxMaterial& chassisMaterial=material;
	PxFilterData chassisCollFilterData;
	chassisCollFilterData.word0=COLLISION_FLAG_CHASSIS;
	chassisCollFilterData.word1=COLLISION_FLAG_CHASSIS_AGAINST;

	//Create a query filter data for the car to ensure that cars
	//do not attempt to drive on themselves.
	PxFilterData vehQryFilterData;
	SampleVehicleSetupVehicleShapeQueryFilterData(&vehQryFilterData);

	//Set up the physx rigid body actor with shapes, local poses, and filters.
	setupActor(
		vehActor,
		vehQryFilterData,
		wheelGeoms,wheelLocalPoses,6,&wheelMaterial,wheelCollFilterData,
		chassisGeoms,chassisLocalPoses,1,&chassisMaterial,chassisCollFilterData,
		chassisData,
		&physics);

	return vehActor;
}

void createVehicle6WSimulationData
(const PxF32 chassisMass, const PxVec3* wheelCentreOffsets4, PxConvexMesh* chassisConvexMesh, PxConvexMesh** wheelConvexMeshes4, 
 PxVehicleWheelsSimData& wheelsSimData6W, PxVehicleDriveSimData4W& driveSimData6W, PxVehicleChassisData& chassisData6W)
{
	//Start by constructing the simulation data for a 4-wheeled vehicle.
	PxVehicleWheelsSimData* wheelsSimData4W=PxVehicleWheelsSimData::allocate(4);
	PxVehicleDriveSimData4W driveData4W;
	PxVehicleChassisData chassisData4W;
	createVehicle4WSimulationData(
		chassisMass,chassisConvexMesh,
		20.0f,wheelConvexMeshes4,wheelCentreOffsets4,
		*wheelsSimData4W,driveData4W,chassisData4W);

	//Quickly setup the simulation data for the 6-wheeled vehicle. 
	//(this will copy wheel4 from wheel0 and wheel5 from wheel1).
	driveSimData6W=driveData4W;
	wheelsSimData6W.copy(*wheelsSimData4W,0,0);
	wheelsSimData6W.copy(*wheelsSimData4W,1,1);
	wheelsSimData6W.copy(*wheelsSimData4W,2,2);
	wheelsSimData6W.copy(*wheelsSimData4W,3,3);
	wheelsSimData6W.copy(*wheelsSimData4W,0,4);
	wheelsSimData6W.copy(*wheelsSimData4W,1,5);
	wheelsSimData6W.setTireLoadFilterData(wheelsSimData4W->getTireLoadFilterData());
	chassisData6W = chassisData4W;

	//Make sure that the two non-driven wheels don't respond to the handbrake.
	PxVehicleWheelData wheelData;
	wheelData=wheelsSimData6W.getWheelData(4);
	wheelData.mMaxHandBrakeTorque=0.0f;
	wheelsSimData6W.setWheelData(4,wheelData);
	wheelData=wheelsSimData6W.getWheelData(5);
	wheelData.mMaxHandBrakeTorque=0.0f;
	wheelsSimData6W.setWheelData(5,wheelData);

	//We've now got a 6-wheeled vehicle but the offsets of the 2 extra wheels are still incorrect.
	//Lets set up the 2 extra wheels to lie on an axle that goes through the centre of the car
	//and is parallel to the front and rear axles.
	{
		PxVec3 w;
		PxF32 offsetLeft;
		PxF32 offsetRight;

		offsetLeft=0.5f*(wheelsSimData6W.getWheelCentreOffset(0).z+wheelsSimData6W.getWheelCentreOffset(2).z);		
		w=wheelsSimData4W->getWheelCentreOffset(0);
		w.z=offsetLeft;
		wheelsSimData6W.setWheelCentreOffset(4,w);
		offsetRight=0.5f*(wheelsSimData6W.getWheelCentreOffset(1).z+wheelsSimData6W.getWheelCentreOffset(3).z);		
		w=wheelsSimData6W.getWheelCentreOffset(1);
		w.z=offsetRight;
		wheelsSimData6W.setWheelCentreOffset(5,w);

		offsetLeft=0.5f*(wheelsSimData6W.getSuspForceAppPointOffset(0).z+wheelsSimData6W.getSuspForceAppPointOffset(2).z);		
		w=wheelsSimData4W->getSuspForceAppPointOffset(0);
		w.z=offsetLeft;
		wheelsSimData6W.setSuspForceAppPointOffset(4,w);
		offsetRight=0.5f*(wheelsSimData6W.getSuspForceAppPointOffset(1).z+wheelsSimData6W.getSuspForceAppPointOffset(3).z);		
		w=wheelsSimData6W.getSuspForceAppPointOffset(1);
		w.z=offsetRight;
		wheelsSimData6W.setSuspForceAppPointOffset(5,w);

		offsetLeft=0.5f*(wheelsSimData6W.getTireForceAppPointOffset(0).z+wheelsSimData6W.getTireForceAppPointOffset(2).z);		
		w=wheelsSimData4W->getTireForceAppPointOffset(0);
		w.z=offsetLeft;
		wheelsSimData6W.setTireForceAppPointOffset(4,w);
		offsetRight=0.5f*(wheelsSimData6W.getTireForceAppPointOffset(1).z+wheelsSimData6W.getTireForceAppPointOffset(3).z);		
		w=wheelsSimData6W.getTireForceAppPointOffset(1);
		w.z=offsetRight;
		wheelsSimData6W.setTireForceAppPointOffset(5,w);
	}

	//The first 4 wheels were all set up to support a mass M but now that mass is
	//distributed between 6 wheels.  Adjust the suspension springs accordingly
	//and try to preserve the natural frequency and damping ratio of the springs.
	for(PxU32 i=0;i<4;i++)
	{
		PxVehicleSuspensionData suspData=wheelsSimData6W.getSuspensionData(i);
		suspData.mSprungMass*=0.6666f;
		suspData.mSpringStrength*=0.666f;
		suspData.mSpringDamperRate*=0.666f;
		wheelsSimData6W.setSuspensionData(i,suspData);
	}
	const PxF32 sprungMass=(wheelsSimData6W.getSuspensionData(0).mSprungMass+wheelsSimData6W.getSuspensionData(3).mSprungMass)/2.0f;
	const PxF32 strength=(wheelsSimData6W.getSuspensionData(0).mSpringStrength+wheelsSimData6W.getSuspensionData(3).mSpringStrength)/2.0f;
	const PxF32 damperRate=(wheelsSimData6W.getSuspensionData(0).mSpringDamperRate+wheelsSimData6W.getSuspensionData(3).mSpringDamperRate)/2.0f;
	PxVehicleSuspensionData suspData4=wheelsSimData6W.getSuspensionData(4);
	suspData4.mSprungMass=sprungMass;
	suspData4.mSpringStrength=strength;
	suspData4.mSpringDamperRate=damperRate;
	wheelsSimData6W.setSuspensionData(4,suspData4);
	PxVehicleSuspensionData suspData5=wheelsSimData6W.getSuspensionData(5);
	suspData5.mSprungMass=sprungMass;
	suspData5.mSpringStrength=strength;
	suspData5.mSpringDamperRate=damperRate;
	wheelsSimData6W.setSuspensionData(5,suspData5);

	//Free the 4W sim data because we don't need this any more.
	wheelsSimData4W->free();
}


void createVehicle6WSimulationData
(const PxF32 chassisMass, const PxVec3* wheelCentreOffsets4, PxConvexMesh* chassisConvexMesh, PxConvexMesh** wheelConvexMeshes4, 
 PxVehicleWheelsSimData& wheelsSimData6W, PxVehicleDriveSimDataNW& driveSimData6W, PxVehicleChassisData& chassisData6W)
{
	//Start by constructing the simulation data for a 4-wheeled vehicle.
	PxVehicleWheelsSimData* wheelsSimData4W=PxVehicleWheelsSimData::allocate(4);
	PxVehicleDriveSimData4W driveData4W;
	PxVehicleChassisData chassisData4W;
	createVehicle4WSimulationData(
		chassisMass,chassisConvexMesh,
		20.0f,wheelConvexMeshes4,wheelCentreOffsets4,
		*wheelsSimData4W,driveData4W,chassisData4W);

	//Quickly setup the simulation data for the 6-wheeled vehicle. 
	//(this will copy wheel4 from wheel0 and wheel5 from wheel1).
	driveSimData6W.setAutoBoxData(driveData4W.getAutoBoxData());
	driveSimData6W.setClutchData(driveData4W.getClutchData());
	driveSimData6W.setEngineData(driveData4W.getEngineData());
	driveSimData6W.setGearsData(driveData4W.getGearsData());
	PxVehicleDifferentialNWData diffNW;
	diffNW.setDrivenWheel(0,true);
	diffNW.setDrivenWheel(1,true);
	diffNW.setDrivenWheel(2,true);
	diffNW.setDrivenWheel(3,true);
	diffNW.setDrivenWheel(4,true);
	diffNW.setDrivenWheel(5,true);
	driveSimData6W.setDiffData(diffNW);
	wheelsSimData6W.copy(*wheelsSimData4W,0,0);
	wheelsSimData6W.copy(*wheelsSimData4W,1,1);
	wheelsSimData6W.copy(*wheelsSimData4W,2,2);
	wheelsSimData6W.copy(*wheelsSimData4W,3,3);
	wheelsSimData6W.copy(*wheelsSimData4W,0,4);
	wheelsSimData6W.copy(*wheelsSimData4W,1,5);
	wheelsSimData6W.setTireLoadFilterData(wheelsSimData4W->getTireLoadFilterData());
	chassisData6W = chassisData4W;

	//Make sure that the two non-driven wheels don't respond to the handbrake.
	PxVehicleWheelData wheelData;
	wheelData=wheelsSimData6W.getWheelData(4);
	wheelData.mMaxHandBrakeTorque=0.0f;
	wheelsSimData6W.setWheelData(4,wheelData);
	wheelData=wheelsSimData6W.getWheelData(5);
	wheelData.mMaxHandBrakeTorque=0.0f;
	wheelsSimData6W.setWheelData(5,wheelData);

	//We've now got a 6-wheeled vehicle but the offsets of the 2 extra wheels are still incorrect.
	//Lets set up the 2 extra wheels to lie on an axle that goes through the centre of the car
	//and is parallel to the front and rear axles.
	{
		PxVec3 w;
		PxF32 offsetLeft;
		PxF32 offsetRight;

		offsetLeft=0.5f*(wheelsSimData6W.getWheelCentreOffset(0).z+wheelsSimData6W.getWheelCentreOffset(2).z);		
		w=wheelsSimData4W->getWheelCentreOffset(0);
		w.z=offsetLeft;
		wheelsSimData6W.setWheelCentreOffset(4,w);
		offsetRight=0.5f*(wheelsSimData6W.getWheelCentreOffset(1).z+wheelsSimData6W.getWheelCentreOffset(3).z);		
		w=wheelsSimData6W.getWheelCentreOffset(1);
		w.z=offsetRight;
		wheelsSimData6W.setWheelCentreOffset(5,w);

		offsetLeft=0.5f*(wheelsSimData6W.getSuspForceAppPointOffset(0).z+wheelsSimData6W.getSuspForceAppPointOffset(2).z);		
		w=wheelsSimData4W->getSuspForceAppPointOffset(0);
		w.z=offsetLeft;
		wheelsSimData6W.setSuspForceAppPointOffset(4,w);
		offsetRight=0.5f*(wheelsSimData6W.getSuspForceAppPointOffset(1).z+wheelsSimData6W.getSuspForceAppPointOffset(3).z);		
		w=wheelsSimData6W.getSuspForceAppPointOffset(1);
		w.z=offsetRight;
		wheelsSimData6W.setSuspForceAppPointOffset(5,w);

		offsetLeft=0.5f*(wheelsSimData6W.getTireForceAppPointOffset(0).z+wheelsSimData6W.getTireForceAppPointOffset(2).z);		
		w=wheelsSimData4W->getTireForceAppPointOffset(0);
		w.z=offsetLeft;
		wheelsSimData6W.setTireForceAppPointOffset(4,w);
		offsetRight=0.5f*(wheelsSimData6W.getTireForceAppPointOffset(1).z+wheelsSimData6W.getTireForceAppPointOffset(3).z);		
		w=wheelsSimData6W.getTireForceAppPointOffset(1);
		w.z=offsetRight;
		wheelsSimData6W.setTireForceAppPointOffset(5,w);
	}

	//The first 4 wheels were all set up to support a mass M but now that mass is
	//distributed between 6 wheels.  Adjust the suspension springs accordingly
	//and try to preserve the natural frequency and damping ratio of the springs.
	for(PxU32 i=0;i<4;i++)
	{
		PxVehicleSuspensionData suspData=wheelsSimData6W.getSuspensionData(i);
		suspData.mSprungMass*=0.6666f;
		suspData.mSpringStrength*=0.666f;
		suspData.mSpringDamperRate*=0.666f;
		wheelsSimData6W.setSuspensionData(i,suspData);
	}
	const PxF32 sprungMass=(wheelsSimData6W.getSuspensionData(0).mSprungMass+wheelsSimData6W.getSuspensionData(3).mSprungMass)/2.0f;
	const PxF32 strength=(wheelsSimData6W.getSuspensionData(0).mSpringStrength+wheelsSimData6W.getSuspensionData(3).mSpringStrength)/2.0f;
	const PxF32 damperRate=(wheelsSimData6W.getSuspensionData(0).mSpringDamperRate+wheelsSimData6W.getSuspensionData(3).mSpringDamperRate)/2.0f;
	PxVehicleSuspensionData suspData4=wheelsSimData6W.getSuspensionData(4);
	suspData4.mSprungMass=sprungMass;
	suspData4.mSpringStrength=strength;
	suspData4.mSpringDamperRate=damperRate;
	wheelsSimData6W.setSuspensionData(4,suspData4);
	PxVehicleSuspensionData suspData5=wheelsSimData6W.getSuspensionData(5);
	suspData5.mSprungMass=sprungMass;
	suspData5.mSpringStrength=strength;
	suspData5.mSpringDamperRate=damperRate;
	wheelsSimData6W.setSuspensionData(5,suspData5);

	//Free the 4W sim data because we don't need this any more.
	wheelsSimData4W->free();
}

void SampleVehicle_VehicleManager::create6WVehicle
(PxScene& scene, PxPhysics& physics, PxCooking& cooking, const PxMaterial& material,
 const PxF32 chassisMass, const PxVec3* wheelCentreOffsets4, PxConvexMesh* chassisConvexMesh, PxConvexMesh** wheelConvexMeshes4,
 const PxTransform& startTransform, const bool useAutoGearFlag)
{
	PX_ASSERT(mNumVehicles<MAX_NUM_4W_VEHICLES+MAX_NUM_6W_VEHICLES);

	//We're going to create an 4-wheeled vehicle then quickly copy components to make an 6-wheeled vehicle.
	PxVehicleWheelsSimData* wheelsSimData=PxVehicleWheelsSimData::allocate(6);
	PxVehicleDriveSimDataNW driveSimData;
	PxVehicleChassisData chassisData;
	createVehicle6WSimulationData(chassisMass,wheelCentreOffsets4,chassisConvexMesh,wheelConvexMeshes4,*wheelsSimData,driveSimData,chassisData);

	//Instantiate and finalize the vehicle using physx.
	PxRigidDynamic* vehActor=createVehicleActor6W(chassisData,wheelConvexMeshes4,chassisConvexMesh,scene,physics,material);

	//Create a car.
	PxVehicleDriveNW* car = PxVehicleDriveNW::allocate(6);
	car->setup(&physics,vehActor,*wheelsSimData,driveSimData,6);

	//Free the sim data because we don't need that any more.
	wheelsSimData->free();

	//Don't forget to add the actor to the scene.
	{
		PxSceneWriteLock scopedLock(scene);
		scene.addActor(*vehActor);
	}

	//Set up the mapping between wheel and actor shape.
	car->mWheelsSimData.setWheelShapeMapping(0,0);
	car->mWheelsSimData.setWheelShapeMapping(1,1);
	car->mWheelsSimData.setWheelShapeMapping(2,2);
	car->mWheelsSimData.setWheelShapeMapping(3,3);
	car->mWheelsSimData.setWheelShapeMapping(4,4);
	car->mWheelsSimData.setWheelShapeMapping(5,5);

	//Set up the scene query filter data for each suspension line.
	PxFilterData vehQryFilterData;
	SampleVehicleSetupVehicleShapeQueryFilterData(&vehQryFilterData);
	car->mWheelsSimData.setSceneQueryFilterData(0, vehQryFilterData);
	car->mWheelsSimData.setSceneQueryFilterData(1, vehQryFilterData);
	car->mWheelsSimData.setSceneQueryFilterData(2, vehQryFilterData);
	car->mWheelsSimData.setSceneQueryFilterData(3, vehQryFilterData);
	car->mWheelsSimData.setSceneQueryFilterData(4, vehQryFilterData);
	car->mWheelsSimData.setSceneQueryFilterData(5, vehQryFilterData);

	//Set the transform and the instantiated car and set it be to be at rest.
	resetNWCar(startTransform,car);
	//Set the autogear mode of the instantiate car.
	car->mDriveDynData.setUseAutoGears(useAutoGearFlag);

	//Increment the number of vehicles
	mVehicles[mNumVehicles]=car;
	mVehicleWheelQueryResults[mNumVehicles].nbWheelQueryResults=6;
	mVehicleWheelQueryResults[mNumVehicles].wheelQueryResults=mWheelQueryResults->addVehicle(6);
	mNumVehicles++;
}

void setTank4WCustomisations(PxVehicleWheelsSimData& suspWheelTyreData, PxVehicleDriveSimData& coreData, const PxVehicleDriveTankControlModel::Enum tankDriveModel)
{
	//Increase damping, especially when the clutch isn't engaged.
	for(PxU32 i=0;i<4;i++)
	{
		PxVehicleWheelData wheelData=suspWheelTyreData.getWheelData(i);
		wheelData.mDampingRate=2.0f;
		suspWheelTyreData.setWheelData(i,wheelData);
	}
	PxVehicleEngineData engineData=coreData.getEngineData();
	engineData.mDampingRateZeroThrottleClutchEngaged=2.0f;
	engineData.mDampingRateZeroThrottleClutchDisengaged=0.5f;
	engineData.mDampingRateFullThrottle=0.5f;
	//engineData.mPeakTorque=1500;
	coreData.setEngineData(engineData);

	if(PxVehicleDriveTankControlModel::eSPECIAL==tankDriveModel)
	{
		PxVehicleGearsData gearsData=coreData.getGearsData();
		gearsData.mFinalRatio=16;
		coreData.setGearsData(gearsData);
	}
}

void SampleVehicle_VehicleManager::create4WTank
(PxScene& scene, PxPhysics& physics, PxCooking& cooking, const PxMaterial& material,
 const PxF32 chassisMass, const PxVec3* wheelCentreOffsets4, PxConvexMesh* chassisConvexMesh, PxConvexMesh** wheelConvexMeshes4,
 const PxTransform& startTransform, const bool useAutoGearFlag, const PxVehicleDriveTankControlModel::Enum tankDriveModel)
{
	PX_ASSERT(mNumVehicles<(MAX_NUM_4W_VEHICLES+MAX_NUM_6W_VEHICLES));

	//Create simulation data for 4W drive car.
	PxVehicleWheelsSimData* wheelsSimData = PxVehicleWheelsSimData::allocate(4);
	PxVehicleDriveSimData4W driveSimData;
	PxVehicleChassisData chassisData;
	createVehicle4WSimulationData(
		chassisMass,chassisConvexMesh,
		20.0f,wheelConvexMeshes4,wheelCentreOffsets4,
		*wheelsSimData,driveSimData,chassisData);

	//Copy the relevant tank data and customise.
	PxVehicleDriveSimData tankDriveSimData;
	tankDriveSimData.setEngineData(driveSimData.getEngineData());
	tankDriveSimData.setGearsData(driveSimData.getGearsData());
	tankDriveSimData.setClutchData(driveSimData.getClutchData());
	tankDriveSimData.setAutoBoxData(driveSimData.getAutoBoxData());
	setTank4WCustomisations(*wheelsSimData,tankDriveSimData,tankDriveModel);

	//Instantiate and finalize the vehicle using physx.
	PxRigidDynamic* vehActor=createVehicleActor4W(chassisData,wheelConvexMeshes4,chassisConvexMesh,scene,physics,material);

	//Create a tank.
	PxVehicleDriveTank* tank = PxVehicleDriveTank::allocate(4);
	tank->setup(&physics,vehActor,*wheelsSimData,tankDriveSimData,4);

	//Free the sim data because we don't need that any more.
	wheelsSimData->free();

	//Don't forget to add the actor to the scene.
	scene.addActor(*vehActor);

	//Set up the mapping between wheel and actor shape.
	tank->mWheelsSimData.setWheelShapeMapping(0,0);
	tank->mWheelsSimData.setWheelShapeMapping(1,1);
	tank->mWheelsSimData.setWheelShapeMapping(2,2);
	tank->mWheelsSimData.setWheelShapeMapping(3,3);

	//Set up the scene query filter data for each suspension line.
	PxFilterData vehQryFilterData;
	SampleVehicleSetupVehicleShapeQueryFilterData(&vehQryFilterData);
	tank->mWheelsSimData.setSceneQueryFilterData(0, vehQryFilterData);
	tank->mWheelsSimData.setSceneQueryFilterData(1, vehQryFilterData);
	tank->mWheelsSimData.setSceneQueryFilterData(2, vehQryFilterData);
	tank->mWheelsSimData.setSceneQueryFilterData(3, vehQryFilterData);

	//Set the transform and the instantiated car and set it be to be at rest.
	resetNWCar(startTransform,tank);
	//Set the autogear mode of the instantiate car.
	tank->mDriveDynData.setUseAutoGears(useAutoGearFlag);
	//Set the drive model.
	tank->setDriveModel(tankDriveModel);

	//Increment the number of vehicles
	mVehicles[mNumVehicles]=tank;
	mVehicleWheelQueryResults[mNumVehicles].nbWheelQueryResults=4;
	mVehicleWheelQueryResults[mNumVehicles].wheelQueryResults=mWheelQueryResults->addVehicle(4);
	mNumVehicles++;
}

void setTank6WCustomisations(PxVehicleWheelsSimData& suspWheelTyreData, PxVehicleDriveSimData& coreData, const PxVehicleDriveTankControlModel::Enum tankDriveModel)
{
	//Increase damping, especially when the clutch isn't engaged.
	for(PxU32 i=0;i<6;i++)
	{
		PxVehicleWheelData wheelData=suspWheelTyreData.getWheelData(i);
		wheelData.mDampingRate=2.0f;
		suspWheelTyreData.setWheelData(i,wheelData);
	}
	PxVehicleEngineData engineData=coreData.getEngineData();
	engineData.mDampingRateZeroThrottleClutchEngaged=2.0f;
	engineData.mDampingRateZeroThrottleClutchDisengaged=0.5f;
	engineData.mDampingRateFullThrottle=0.5f;
	//engineData.mPeakTorque=1500;
	coreData.setEngineData(engineData);

	if(PxVehicleDriveTankControlModel::eSPECIAL==tankDriveModel)
	{
		PxVehicleGearsData gearsData=coreData.getGearsData();
		gearsData.mFinalRatio=16;
		coreData.setGearsData(gearsData);
	}
}

void SampleVehicle_VehicleManager::create6WTank
(PxScene& scene, PxPhysics& physics, PxCooking& cooking, const PxMaterial& material,
 const PxF32 chassisMass, const PxVec3* wheelCentreOffsets4, PxConvexMesh* chassisConvexMesh, PxConvexMesh** wheelConvexMeshes4,
 const PxTransform& startTransform, const bool useAutoGearFlag, const PxVehicleDriveTankControlModel::Enum tankDriveModel)
{
	//Create simulation data for 4W drive car.
	PxVehicleWheelsSimData* wheelsSimData=PxVehicleWheelsSimData::allocate(6);
	PxVehicleDriveSimData4W driveSimData;
	PxVehicleChassisData chassisData;
	createVehicle6WSimulationData(chassisMass,wheelCentreOffsets4,chassisConvexMesh,wheelConvexMeshes4,*wheelsSimData,driveSimData,chassisData);

	//Copy the relevant tank data and customize.
	PxVehicleDriveSimData tankDriveSimData;
	tankDriveSimData.setEngineData(driveSimData.getEngineData());
	tankDriveSimData.setGearsData(driveSimData.getGearsData());
	tankDriveSimData.setClutchData(driveSimData.getClutchData());
	tankDriveSimData.setAutoBoxData(driveSimData.getAutoBoxData());
	setTank6WCustomisations(*wheelsSimData,tankDriveSimData,tankDriveModel);

	//Instantiate and finalize the vehicle using physx.
	PxRigidDynamic* vehActor=createVehicleActor6W(chassisData,wheelConvexMeshes4,chassisConvexMesh,scene,physics,material);

	//Create a tank.
	PxVehicleDriveTank* tank = PxVehicleDriveTank::allocate(6);
	tank->setup(&physics,vehActor,*wheelsSimData,tankDriveSimData,6);

	//Free the sim data because we don't need that any more.
	wheelsSimData->free();

	//Don't forget to add the actor to the scene.
	scene.addActor(*vehActor);

	//Set up the mapping between wheel and actor shape.
	tank->mWheelsSimData.setWheelShapeMapping(0,0);
	tank->mWheelsSimData.setWheelShapeMapping(1,1);
	tank->mWheelsSimData.setWheelShapeMapping(2,2);
	tank->mWheelsSimData.setWheelShapeMapping(3,3);
	tank->mWheelsSimData.setWheelShapeMapping(4,4);
	tank->mWheelsSimData.setWheelShapeMapping(5,5);

	//Set up the scene query filter data for each suspension line.
	PxFilterData vehQryFilterData;
	SampleVehicleSetupVehicleShapeQueryFilterData(&vehQryFilterData);
	tank->mWheelsSimData.setSceneQueryFilterData(0, vehQryFilterData);
	tank->mWheelsSimData.setSceneQueryFilterData(1, vehQryFilterData);
	tank->mWheelsSimData.setSceneQueryFilterData(2, vehQryFilterData);
	tank->mWheelsSimData.setSceneQueryFilterData(3, vehQryFilterData);
	tank->mWheelsSimData.setSceneQueryFilterData(4, vehQryFilterData);
	tank->mWheelsSimData.setSceneQueryFilterData(5, vehQryFilterData);

	//Set the transform and the instantiated car and set it be to be at rest.
	resetNWCar(startTransform,tank);
	//Set the autogear mode of the instantiate car.
	tank->mDriveDynData.setUseAutoGears(useAutoGearFlag);
	//set the drive model
	tank->setDriveModel(tankDriveModel);

	//Increment the number of vehicles
	mVehicles[mNumVehicles]=tank;
	mVehicleWheelQueryResults[mNumVehicles].nbWheelQueryResults=6;
	mVehicleWheelQueryResults[mNumVehicles].wheelQueryResults=mWheelQueryResults->addVehicle(6);
	mNumVehicles++;
}

void SampleVehicle_VehicleManager::switchTo3WDeltaMode(const PxU32 vehicleId)
{
	//Get the vehicle that will be in 3-wheeled mode.
	PX_ASSERT(mVehicles[vehicleId]->getVehicleType()==PxVehicleTypes::eDRIVE4W);
	PxVehicleDrive4W* veh=(PxVehicleDrive4W*)mVehicles[vehicleId];
	if(mIsIn3WMode)
	{
		switchTo4WMode(vehicleId);
	}
	else
	{
		veh->mWheelsSimData=*mWheelsSimData4W;
		veh->mDriveSimData=mDriveSimData4W;
	}
	PxVehicle4WEnable3WDeltaMode(veh->mWheelsSimData, veh->mWheelsDynData, veh->mDriveSimData);
	mIsIn3WMode=true;
}

void SampleVehicle_VehicleManager::switchTo3WTadpoleMode(const PxU32 vehicleId)
{
	//Get the vehicle that will be in 3-wheeled mode.
	PX_ASSERT(mVehicles[vehicleId]->getVehicleType()==PxVehicleTypes::eDRIVE4W);
	PxVehicleDrive4W* veh=(PxVehicleDrive4W*)mVehicles[vehicleId];
	if(mIsIn3WMode)
	{
		switchTo4WMode(vehicleId);
	}
	else
	{
		veh->mWheelsSimData=*mWheelsSimData4W;
		veh->mDriveSimData=mDriveSimData4W;
	}
	PxVehicle4WEnable3WTadpoleMode(veh->mWheelsSimData, veh->mWheelsDynData, veh->mDriveSimData);
	mIsIn3WMode=true;
}

void SampleVehicle_VehicleManager::switchTo4WMode(const PxU32 vehicleId)
{
	//Get the vehicle that will be in 3-wheeled mode.
	PX_ASSERT(mVehicles[vehicleId]->getVehicleType()==PxVehicleTypes::eDRIVE4W);
	PxVehicleDrive4W* veh=(PxVehicleDrive4W*)mVehicles[vehicleId];

	*mWheelsSimData4W=veh->mWheelsSimData;
	mDriveSimData4W=veh->mDriveSimData;
	mIsIn3WMode=false;
}



