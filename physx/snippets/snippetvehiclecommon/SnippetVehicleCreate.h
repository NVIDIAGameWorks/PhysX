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

#ifndef SNIPPET_VEHICLE_COMMON_H
#define SNIPPET_VEHICLE_COMMON_H

#include "PxPhysicsAPI.h"
#include "vehicle/PxVehicleDriveTank.h"
#include "vehicle/PxVehicleNoDrive.h"

namespace snippetvehicle
{

using namespace physx;

////////////////////////////////////////////////

PxRigidStatic* createDrivablePlane(const PxFilterData& simFilterData, PxMaterial* material, PxPhysics* physics);

////////////////////////////////////////////////

struct ActorUserData
{
	ActorUserData()
		: vehicle(NULL),
		  actor(NULL)
	{
	}

	const PxVehicleWheels* vehicle;
	const PxActor* actor;
};

struct ShapeUserData
{
	ShapeUserData()
		: isWheel(false),
		  wheelId(0xffffffff)
	{
	}

	bool isWheel;
	PxU32 wheelId;
};


struct VehicleDesc
{
	VehicleDesc()
		: chassisMass(0.0f),
		  chassisDims(PxVec3(0.0f, 0.0f, 0.0f)),
		  chassisMOI(PxVec3(0.0f, 0.0f, 0.0f)),
		  chassisCMOffset(PxVec3(0.0f, 0.0f, 0.0f)),
		  chassisMaterial(NULL),
		  wheelMass(0.0f),
		  wheelWidth(0.0f),
		  wheelRadius(0.0f),
		  wheelMOI(0.0f),
		  wheelMaterial(NULL),
		  actorUserData(NULL),
		  shapeUserDatas(NULL)
	{
	}

	PxF32 chassisMass;
	PxVec3 chassisDims;
	PxVec3 chassisMOI;
	PxVec3 chassisCMOffset;
	PxMaterial* chassisMaterial;
	PxFilterData chassisSimFilterData;  //word0 = collide type, word1 = collide against types, word2 = PxPairFlags

	PxF32 wheelMass;
	PxF32 wheelWidth;
	PxF32 wheelRadius;
	PxF32 wheelMOI;
	PxMaterial* wheelMaterial;
	PxU32 numWheels;
	PxFilterData wheelSimFilterData;	//word0 = collide type, word1 = collide against types, word2 = PxPairFlags

	ActorUserData* actorUserData;
	ShapeUserData* shapeUserDatas;
};

PxVehicleDrive4W* createVehicle4W(const VehicleDesc& vehDesc, PxPhysics* physics, PxCooking* cooking);

PxVehicleDriveTank* createVehicleTank(const VehicleDesc& vehDesc, PxPhysics* physics, PxCooking* cooking);

PxVehicleNoDrive* createVehicleNoDrive(const VehicleDesc& vehDesc, PxPhysics* physics, PxCooking* cooking);

////////////////////////////////////////////////

PxConvexMesh* createChassisMesh(const PxVec3 dims, PxPhysics& physics, PxCooking& cooking);

PxConvexMesh* createWheelMesh(const PxF32 width, const PxF32 radius, PxPhysics& physics, PxCooking& cooking);

////////////////////////////////////////////////

void customizeVehicleToLengthScale(const PxReal lengthScale, PxRigidDynamic* rigidDynamic, PxVehicleWheelsSimData* wheelsSimData, PxVehicleDriveSimData* driveSimData);

void customizeVehicleToLengthScale(const PxReal lengthScale, PxRigidDynamic* rigidDynamic, PxVehicleWheelsSimData* wheelsSimData, PxVehicleDriveSimData4W* driveSimData);

////////////////////////////////////////////////

PxRigidDynamic* createVehicleActor
(const PxVehicleChassisData& chassisData,
 PxMaterial** wheelMaterials, PxConvexMesh** wheelConvexMeshes, const PxU32 numWheels, const PxFilterData& wheelSimFilterData,
 PxMaterial** chassisMaterials, PxConvexMesh** chassisConvexMeshes, const PxU32 numChassisMeshes, const PxFilterData& chassisSimFilterData,
 PxPhysics& physics);

void configureUserData(PxVehicleWheels* vehicle, ActorUserData* actorUserData, ShapeUserData* shapeUserDatas);

////////////////////////////////////////////////

} // namespace snippetvehicle

#endif //SNIPPET_VEHICLE_COMMON_H
