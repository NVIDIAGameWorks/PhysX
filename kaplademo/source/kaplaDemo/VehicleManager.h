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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.

#ifndef VEHICLE_H
#define VEHICLE_H

#include "vehicle/PxVehicleWheels.h"
#include "vehicle/PxVehicleDrive4W.h"
#include "vehicle/PxVehicleTireFriction.h"
#include "vehicle/PxVehicleUpdate.h"
#include "RawLoader.h"
#include "PxPhysics.h"
#include "PxCooking.h"

using namespace physx;

namespace physx
{
	class PxBatchQuery;

	namespace fracture
	{
		namespace base
		{
			class Compound;
		}
	}
}

class VehicleWheelQueryResults;
class VehicleSceneQueryData;

//Tire types.
enum
{
	TIRE_TYPE_WETS = 0,
	TIRE_TYPE_SLICKS,
	TIRE_TYPE_ICE,
	TIRE_TYPE_MUD,
	MAX_NUM_TIRE_TYPES
};

//Drivable surface types.
enum
{
	SURFACE_TYPE_MUD = 0,
	SURFACE_TYPE_TARMAC,
	SURFACE_TYPE_SNOW,
	SURFACE_TYPE_GRASS,
	MAX_NUM_SURFACE_TYPES
};


//Collision types and flags describing collision interactions of each collision type.
enum
{
	COLLISION_FLAG_GROUND = 1 << 0,
	COLLISION_FLAG_WHEEL = 1 << 1,
	COLLISION_FLAG_CHASSIS = 1 << 2,
	COLLISION_FLAG_OBSTACLE = 1 << 3,
	COLLISION_FLAG_DRIVABLE_OBSTACLE = 1 << 4,

	COLLISION_FLAG_GROUND_AGAINST = COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE,
	COLLISION_FLAG_WHEEL_AGAINST = COLLISION_FLAG_WHEEL | COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE,
	COLLISION_FLAG_CHASSIS_AGAINST = COLLISION_FLAG_GROUND | COLLISION_FLAG_WHEEL | COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE,
	COLLISION_FLAG_OBSTACLE_AGAINST = COLLISION_FLAG_GROUND | COLLISION_FLAG_WHEEL | COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE,
	COLLISION_FLAG_DRIVABLE_OBSTACLE_AGAINST = COLLISION_FLAG_GROUND | COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE,
};
// ---------------------------------------------------------------------


struct TireFrictionMultipliers
{
	static float getValue(PxU32 surfaceType, PxU32 tireType)
	{
		//Tire model friction for each combination of drivable surface type and tire type.
		static PxF32 tireFrictionMultipliers[MAX_NUM_SURFACE_TYPES][MAX_NUM_TIRE_TYPES] =
		{
			//WETS	SLICKS	ICE		MUD
			{ 0.95f, 0.95f, 0.95f, 0.95f },		//MUD
			{ 1.10f, 1.15f, 1.10f, 1.10f },		//TARMAC
			{ 0.70f, 0.70f, 0.70f, 0.70f },		//ICE
			{ 0.80f, 0.80f, 0.80f, 0.80f }		//GRASS
		};
		return tireFrictionMultipliers[surfaceType][tireType];
	}
};

class VehicleManager
{
public:
	VehicleManager();
	virtual ~VehicleManager();

	void init(PxPhysics& physics, const PxMaterial** drivableSurfaceMaterials, const PxVehicleDrivableSurfaceType* drivableSurfaceTypes);

	void create4WVehicle(PxScene& scene, PxPhysics& physics, PxCooking& cooking, const PxMaterial& material,
		const PxF32 chassisMass, const PxVec3* wheelCentreOffsets4, PxConvexMesh* chassisConvexMesh, PxConvexMesh** wheelConvexMeshes4,
		const PxTransform& startTransform, const bool useAutoGearFlag);
	void createVehicle4WSimulationData(const PxF32 chassisMass, PxConvexMesh* chassisConvexMesh, const PxF32 wheelMass, PxConvexMesh** wheelConvexMeshes,
		const PxVec3* wheelCentreOffsets, PxVehicleWheelsSimData& wheelsData, PxVehicleDriveSimData4W& driveData, PxVehicleChassisData& chassisData);
	PxVec3 computeChassisAABBDimensions(const PxConvexMesh* chassisConvexMesh);
	void computeWheelWidthsAndRadii(PxConvexMesh** wheelConvexMeshes, PxF32* wheelWidths, PxF32* wheelRadii);
	void resetNWCar(const PxTransform& startTransform, PxVehicleWheels* vehWheels);
	void suspensionRaycasts(PxScene* scene);
	void suspensionSweeps(PxScene* scene);
	void update(const PxF32 timestep, const PxVec3& gravity);
	
	PxVehicleWheels* getVehicle() { return mVehicle;  }

	PxVehicleWheelQueryResult& getWheelQueryResult() { return mVehicleWheelQueryResults; };

	void clearBatchQuery() { mSqWheelRaycastBatchQuery = NULL;  }

private:

	PxVehicleWheels* mVehicle;
	PxVehicleWheelQueryResult mVehicleWheelQueryResults;
	//sdk raycasts (for the suspension lines).
	VehicleSceneQueryData* mSqData;
	PxBatchQuery* mSqWheelRaycastBatchQuery;

	//Reports for each wheel.
	VehicleWheelQueryResults* mWheelQueryResults;

	//Cached simulation data of focus vehicle in 4W mode.
	PxVehicleWheelsSimData* mWheelsSimData4W;
	PxVehicleDriveSimData4W mDriveSimData4W;

	//Friction from combinations of tire and surface types.
	PxVehicleDrivableSurfaceToTireFrictionPairs* mSurfaceTirePairs;

public:
	fracture::base::Compound* mCompoundShape;

};
#endif  // SCENE_BOXES_H
