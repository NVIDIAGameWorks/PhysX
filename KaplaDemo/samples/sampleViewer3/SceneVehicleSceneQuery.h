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

#ifndef VEHICLE_UTILSCENEQUERY_H
#define VEHICLE_UTILSCENEQUERY_H

#include "common/PxPhysXCommonConfig.h"
#include "vehicle/PxVehicleSDK.h"
#include "foundation/PxPreprocessor.h"
#include "PxScene.h"
#include "PxBatchQueryDesc.h"

using namespace physx;

//Make sure that suspension raycasts only consider shapes flagged as drivable that don't belong to the owner vehicle.
enum
{
	VEHICLE_DRIVABLE_SURFACE = 0xffff0000,
	VEHICLE_UNDRIVABLE_SURFACE = 0x0000ffff
};

static PxQueryHitType::Enum SampleVehicleWheelRaycastPreFilter(
	PxFilterData filterData0,
	PxFilterData filterData1,
	const void* constantBlock, PxU32 constantBlockSize,
	PxHitFlags& queryFlags)
{
	//filterData0 is the vehicle suspension raycast.
	//filterData1 is the shape potentially hit by the raycast.
	PX_UNUSED(queryFlags);
	PX_UNUSED(constantBlockSize);
	PX_UNUSED(constantBlock);
	PX_UNUSED(filterData0);
	return ((0 == (filterData1.word3 & VEHICLE_DRIVABLE_SURFACE)) ? PxQueryHitType::eNONE : PxQueryHitType::eBLOCK);
}


//Set up query filter data so that vehicles can drive on shapes with this filter data.
//Note that we have reserved word3 of the PxFilterData for vehicle raycast query filtering.
void VehicleSetupDrivableShapeQueryFilterData(PxFilterData* qryFilterData);

//Set up query filter data so that vehicles cannot drive on shapes with this filter data.
//Note that we have reserved word3 of the PxFilterData for vehicle raycast query filtering.
void VehicleSetupNonDrivableShapeQueryFilterData(PxFilterData* qryFilterData);

//Set up query filter data for the shapes of a vehicle to ensure that vehicles cannot drive on themselves 
//but can drive on the shapes of other vehicles.
//Note that we have reserved word3 of the PxFilterData for vehicle raycast query filtering.
void VehicleSetupVehicleShapeQueryFilterData(PxFilterData* qryFilterData);

//Data structure for quick setup of scene queries for suspension raycasts.
class VehicleSceneQueryData
{
public:

	//Allocate scene query data for up to maxNumWheels suspension raycasts.
	static VehicleSceneQueryData* allocate(const PxU32 maxNumWheels);

	//Free allocated buffer for scene queries of suspension raycasts.
	void free();

	//Create a PxBatchQuery instance that will be used as a single batched raycast of multiple suspension lines of multiple vehicles
	PxBatchQuery* setUpBatchedSceneQuery(PxScene* scene);

	PxBatchQuery*  setUpBatchedSceneQuerySweep(PxScene* scene);

	//Get the buffer of scene query results that will be used by PxVehicleNWSuspensionRaycasts
	PxRaycastQueryResult* getRaycastQueryResultBuffer() { return mSqResults; }

	//Get the number of scene query results that have been allocated for use by PxVehicleNWSuspensionRaycasts
	PxU32 getRaycastQueryResultBufferSize() const { return mNumQueries; }


	//Get the buffer of scene query results that will be used by PxVehicleNWSuspensionRaycasts
	PxSweepQueryResult* getSweepQueryResultBuffer() { return mSqSweepResults; }

	//Get the number of scene query results that have been allocated for use by PxVehicleNWSuspensionRaycasts
	PxU32 getSweepQueryResultBufferSize() const { return mNumSweepQueries; }

	//Set the pre-filter shader 
	void setPreFilterShader(PxBatchQueryPreFilterShader preFilterShader) { mPreFilterShader = preFilterShader; }

private:

	//One result for each wheel.
	PxRaycastQueryResult* mSqResults;
	PxU32 mNbSqResults;

	//One hit for each wheel.
	PxRaycastHit* mSqHitBuffer;

	PxSweepQueryResult* mSqSweepResults;
	PxU32 mNbSqSweepResults;
	PxSweepHit* mSqSweepHitBuffer;

	//Filter shader used to filter drivable and non-drivable surfaces
	PxBatchQueryPreFilterShader mPreFilterShader;

	//Maximum number of suspension raycasts that can be supported by the allocated buffers 
	//assuming a single query and hit per suspension line.
	PxU32 mNumQueries;

	PxU32 mNumSweepQueries;

	void init()
	{
		mPreFilterShader = SampleVehicleWheelRaycastPreFilter;
	}

	VehicleSceneQueryData()
	{
		init();
	}

	~VehicleSceneQueryData()
	{
	}
};


#endif //SAMPLEVEHICLE_UTILSCENEQUERY_H
