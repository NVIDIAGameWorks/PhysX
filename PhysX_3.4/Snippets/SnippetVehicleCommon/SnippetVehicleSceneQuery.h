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

#ifndef SNIPPET_VEHICLE_SCENEQUERY_H
#define SNIPPET_VEHICLE_SCENEQUERY_H

#include "PxPhysicsAPI.h"

namespace snippetvehicle
{

using namespace physx;

enum
{
	DRIVABLE_SURFACE = 0xffff0000,
	UNDRIVABLE_SURFACE = 0x0000ffff
};

void setupDrivableSurface(PxFilterData& filterData);

void setupNonDrivableSurface(PxFilterData& filterData);


PxQueryHitType::Enum WheelSceneQueryPreFilterBlocking
(PxFilterData filterData0, PxFilterData filterData1,
 const void* constantBlock, PxU32 constantBlockSize,
 PxHitFlags& queryFlags);

PxQueryHitType::Enum WheelSceneQueryPostFilterBlocking
(PxFilterData queryFilterData, PxFilterData objectFilterData,
 const void* constantBlock, PxU32 constantBlockSize,
 const PxQueryHit& hit);

PxQueryHitType::Enum WheelSceneQueryPreFilterNonBlocking
(PxFilterData filterData0, PxFilterData filterData1,
const void* constantBlock, PxU32 constantBlockSize,
PxHitFlags& queryFlags);

PxQueryHitType::Enum WheelSceneQueryPostFilterNonBlocking
(PxFilterData queryFilterData, PxFilterData objectFilterData,
const void* constantBlock, PxU32 constantBlockSize,
const PxQueryHit& hit);


//Data structure for quick setup of scene queries for suspension queries.
class VehicleSceneQueryData
{
public:
	VehicleSceneQueryData();
	~VehicleSceneQueryData();

	//Allocate scene query data for up to maxNumVehicles and up to maxNumWheelsPerVehicle with numVehiclesInBatch per batch query.
	static VehicleSceneQueryData* allocate
		(const PxU32 maxNumVehicles, const PxU32 maxNumWheelsPerVehicle, const PxU32 maxNumHitPointsPerWheel, const PxU32 numVehiclesInBatch,
		 PxBatchQueryPreFilterShader preFilterShader, PxBatchQueryPostFilterShader postFilterShader, 
		 PxAllocatorCallback& allocator);

	//Free allocated buffers.
	void free(PxAllocatorCallback& allocator);

	//Create a PxBatchQuery instance that will be used for a single specified batch.
	static PxBatchQuery* setUpBatchedSceneQuery(const PxU32 batchId, const VehicleSceneQueryData& vehicleSceneQueryData, PxScene* scene);

	//Return an array of scene query results for a single specified batch.
	PxRaycastQueryResult* getRaycastQueryResultBuffer(const PxU32 batchId); 

	//Return an array of scene query results for a single specified batch.
	PxSweepQueryResult* getSweepQueryResultBuffer(const PxU32 batchId); 

	//Get the number of scene query results that have been allocated for a single batch.
	PxU32 getQueryResultBufferSize() const; 

private:

	//Number of queries per batch
	PxU32 mNumQueriesPerBatch;

	//Number of hit results per query
	PxU32 mNumHitResultsPerQuery;

	//One result for each wheel.
	PxRaycastQueryResult* mRaycastResults;
	PxSweepQueryResult* mSweepResults;

	//One hit for each wheel.
	PxRaycastHit* mRaycastHitBuffer;
	PxSweepHit* mSweepHitBuffer;

	//Filter shader used to filter drivable and non-drivable surfaces
	PxBatchQueryPreFilterShader mPreFilterShader;

	//Filter shader used to reject hit shapes that initially overlap sweeps.
	PxBatchQueryPostFilterShader mPostFilterShader;

};

} // namespace snippetvehicle

#endif //SNIPPET_VEHICLE_SCENEQUERY_H
