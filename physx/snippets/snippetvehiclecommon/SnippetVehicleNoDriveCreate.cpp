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

#include "SnippetVehicleCreate.h"
#include "SnippetVehicleTireFriction.h"
#include "SnippetVehicleSceneQuery.h"



namespace snippetvehicle
{

using namespace physx;

namespace nodrive
{

void computeWheelCenterActorOffsets
(const PxF32 wheelFrontZ, const PxF32 wheelRearZ, const PxVec3& chassisDims, const PxF32 wheelWidth, const PxF32 wheelRadius, const PxU32 numWheels, PxVec3* wheelCentreOffsets)
{
	//chassisDims.z is the distance from the rear of the chassis to the front of the chassis.
	//The front has z = 0.5*chassisDims.z and the rear has z = -0.5*chassisDims.z.
	//Compute a position for the front wheel and the rear wheel along the z-axis.
	//Compute the separation between each wheel along the z-axis.
	const PxF32 numLeftWheels = numWheels/2.0f;
	const PxF32 deltaZ = (wheelFrontZ - wheelRearZ)/(numLeftWheels-1.0f);
	//Set the outside of the left and right wheels to be flush with the chassis.
	//Set the top of the wheel to be just touching the underside of the chassis.
	for(PxU32 i = 0; i < numWheels; i+=2)
	{
		//Left wheel offset from origin.
		wheelCentreOffsets[i + 0] = PxVec3((-chassisDims.x + wheelWidth)*0.5f, -(chassisDims.y/2 + wheelRadius), wheelRearZ + i*deltaZ*0.5f);
		//Right wheel offsets from origin.
		wheelCentreOffsets[i + 1] = PxVec3((+chassisDims.x - wheelWidth)*0.5f, -(chassisDims.y/2 + wheelRadius), wheelRearZ + i*deltaZ*0.5f);
	}
}

void setupWheelsSimulationData
(const PxF32 wheelMass, const PxF32 wheelMOI, const PxF32 wheelRadius, const PxF32 wheelWidth, 
 const PxU32 numWheels, const PxVec3* wheelCenterActorOffsets,
 const PxVec3& chassisCMOffset, const PxF32 chassisMass,
 PxVehicleWheelsSimData* wheelsSimData)
{
	//Set up the wheels.
	PxVehicleWheelData wheels[PX_MAX_NB_WHEELS];
	{
		//Set up the wheel data structures with mass, moi, radius, width.
		for(PxU32 i = 0; i < numWheels; i++)
		{
			wheels[i].mMass = wheelMass;
			wheels[i].mMOI = wheelMOI;
			wheels[i].mRadius = wheelRadius;
			wheels[i].mWidth = wheelWidth;
		}
	}

	//Set up the tires.
	PxVehicleTireData tires[PX_MAX_NB_WHEELS];
	{
		//Set up the tires.
		for(PxU32 i = 0; i < numWheels; i++)
		{
			tires[i].mType = TIRE_TYPE_NORMAL;
		}
	}

	//Set up the suspensions
	PxVehicleSuspensionData suspensions[PX_MAX_NB_WHEELS];
	{
		//Compute the mass supported by each suspension spring.
		PxF32 suspSprungMasses[PX_MAX_NB_WHEELS];
		PxVehicleComputeSprungMasses(numWheels, wheelCenterActorOffsets, chassisCMOffset, chassisMass, 1, suspSprungMasses);

		//Set the suspension data.
		for(PxU32 i = 0; i < numWheels; i++)
		{
			suspensions[i].mMaxCompression = 0.3f;
			suspensions[i].mMaxDroop = 0.1f;
			suspensions[i].mSpringStrength = 35000.0f;	
			suspensions[i].mSpringDamperRate = 4500.0f;
			suspensions[i].mSprungMass = suspSprungMasses[i];
		}

		//Set the camber angles.
		const PxF32 camberAngleAtRest=0.0;
		const PxF32 camberAngleAtMaxDroop=0.01f;
		const PxF32 camberAngleAtMaxCompression=-0.01f;
		for(PxU32 i = 0; i < numWheels; i+=2)
		{
			suspensions[i + 0].mCamberAtRest =  camberAngleAtRest;
			suspensions[i + 1].mCamberAtRest =  -camberAngleAtRest;
			suspensions[i + 0].mCamberAtMaxDroop = camberAngleAtMaxDroop;
			suspensions[i + 1].mCamberAtMaxDroop = -camberAngleAtMaxDroop;
			suspensions[i + 0].mCamberAtMaxCompression = camberAngleAtMaxCompression;
			suspensions[i + 1].mCamberAtMaxCompression = -camberAngleAtMaxCompression;
		}
	}

	//Set up the wheel geometry.
	PxVec3 suspTravelDirections[PX_MAX_NB_WHEELS];
	PxVec3 wheelCentreCMOffsets[PX_MAX_NB_WHEELS];
	PxVec3 suspForceAppCMOffsets[PX_MAX_NB_WHEELS];
	PxVec3 tireForceAppCMOffsets[PX_MAX_NB_WHEELS];
	{
		//Set the geometry data.
		for(PxU32 i = 0; i < numWheels; i++)
		{
			//Vertical suspension travel.
			suspTravelDirections[i] = PxVec3(0,-1,0);

			//Wheel center offset is offset from rigid body center of mass.
			wheelCentreCMOffsets[i] = wheelCenterActorOffsets[i] - chassisCMOffset;

			//Suspension force application point 0.3 metres below rigid body center of mass.
			suspForceAppCMOffsets[i]=PxVec3(wheelCentreCMOffsets[i].x,-0.3f,wheelCentreCMOffsets[i].z);

			//Tire force application point 0.3 metres below rigid body center of mass.
			tireForceAppCMOffsets[i]=PxVec3(wheelCentreCMOffsets[i].x,-0.3f,wheelCentreCMOffsets[i].z);
		}
	}

	//Set up the filter data of the raycast that will be issued by each suspension.
	PxFilterData qryFilterData;
	setupNonDrivableSurface(qryFilterData);

	//Set the wheel, tire and suspension data.
	//Set the geometry data.
	//Set the query filter data
	for(PxU32 i = 0; i < numWheels; i++)
	{
		wheelsSimData->setWheelData(i, wheels[i]);
		wheelsSimData->setTireData(i, tires[i]);
		wheelsSimData->setSuspensionData(i, suspensions[i]);
		wheelsSimData->setSuspTravelDirection(i, suspTravelDirections[i]);
		wheelsSimData->setWheelCentreOffset(i, wheelCentreCMOffsets[i]);
		wheelsSimData->setSuspForceAppPointOffset(i, suspForceAppCMOffsets[i]);
		wheelsSimData->setTireForceAppPointOffset(i, tireForceAppCMOffsets[i]);
		wheelsSimData->setSceneQueryFilterData(i, qryFilterData);
		wheelsSimData->setWheelShapeMapping(i, PxI32(i)); 
	}
}

}// namespace nodrive

PxVehicleNoDrive* createVehicleNoDrive(const VehicleDesc& vehicleDesc, PxPhysics* physics, PxCooking* cooking)
{
	const PxVec3 chassisDims = vehicleDesc.chassisDims;
	const PxF32 wheelWidth = vehicleDesc.wheelWidth;
	const PxF32 wheelRadius = vehicleDesc.wheelRadius;
	const PxU32 numWheels = vehicleDesc.numWheels;

	const PxFilterData& chassisSimFilterData = vehicleDesc.chassisSimFilterData;
	const PxFilterData& wheelSimFilterData = vehicleDesc.wheelSimFilterData;

	//Construct a physx actor with shapes for the chassis and wheels.
	//Set the rigid body mass, moment of inertia, and center of mass offset.
	PxRigidDynamic* vehActor = NULL;
	{
		//Construct a convex mesh for a cylindrical wheel.
		PxConvexMesh* wheelMesh = createWheelMesh(wheelWidth, wheelRadius, *physics, *cooking);
		//Assume all wheels are identical for simplicity.
		PxConvexMesh* wheelConvexMeshes[PX_MAX_NB_WHEELS];
		PxMaterial* wheelMaterials[PX_MAX_NB_WHEELS];

		//Set the meshes and materials for the driven wheels.
		for(PxU32 i = 0; i < numWheels; i++)
		{
			wheelConvexMeshes[i] = wheelMesh;
			wheelMaterials[i] = vehicleDesc.wheelMaterial;
		}

		//Chassis just has a single convex shape for simplicity.
		PxConvexMesh* chassisConvexMesh = createChassisMesh(chassisDims, *physics, *cooking);
		PxConvexMesh* chassisConvexMeshes[1] = {chassisConvexMesh};
		PxMaterial* chassisMaterials[1] = {vehicleDesc.chassisMaterial};

		//Rigid body data.
		PxVehicleChassisData rigidBodyData;
		rigidBodyData.mMOI = vehicleDesc.chassisMOI;
		rigidBodyData.mMass = vehicleDesc.chassisMass;
		rigidBodyData.mCMOffset = vehicleDesc.chassisCMOffset;

		vehActor = createVehicleActor
			(rigidBodyData,
			wheelMaterials, wheelConvexMeshes, numWheels, wheelSimFilterData,
			chassisMaterials, chassisConvexMeshes, 1, chassisSimFilterData,
			*physics);
	}

	//Set up the sim data for the wheels.
	PxVehicleWheelsSimData* wheelsSimData = PxVehicleWheelsSimData::allocate(numWheels);
	{
		//Compute the wheel center offsets from the origin.
		PxVec3 wheelCentreActorOffsets[PX_MAX_NB_WHEELS];
		const PxF32 frontZ = chassisDims.z*0.3f;
		const PxF32 rearZ = -chassisDims.z*0.3f;
		nodrive::computeWheelCenterActorOffsets(frontZ, rearZ, chassisDims, wheelWidth, wheelRadius, numWheels, wheelCentreActorOffsets);

		nodrive::setupWheelsSimulationData
			(vehicleDesc.wheelMass, vehicleDesc.wheelMOI, wheelRadius, wheelWidth, 
			 numWheels, wheelCentreActorOffsets,
			 vehicleDesc.chassisCMOffset, vehicleDesc.chassisMass,
			 wheelsSimData);
	}

	//Create a vehicle from the wheels and drive sim data.
	PxVehicleNoDrive* vehDriveNoDrive = PxVehicleNoDrive::allocate(numWheels);
	vehDriveNoDrive->setup(physics, vehActor, *wheelsSimData);

	//Configure the userdata
	configureUserData(vehDriveNoDrive, vehicleDesc.actorUserData, vehicleDesc.shapeUserDatas);

	//Free the sim data because we don't need that any more.
	wheelsSimData->free();

	return vehDriveNoDrive;
}

} // namespace snippetvehicle

