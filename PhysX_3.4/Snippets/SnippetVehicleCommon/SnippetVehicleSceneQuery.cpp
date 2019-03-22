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

#include <new>
#include "SnippetVehicleSceneQuery.h"
#include "PxPhysicsAPI.h"

namespace snippetvehicle
{

using namespace physx;

void setupDrivableSurface(PxFilterData& filterData)
{
	filterData.word3 = static_cast<PxU32>(DRIVABLE_SURFACE);
}

void setupNonDrivableSurface(PxFilterData& filterData)
{
	filterData.word3 = UNDRIVABLE_SURFACE;
}

PxQueryHitType::Enum WheelSceneQueryPreFilterBlocking
(PxFilterData filterData0, PxFilterData filterData1,
 const void* constantBlock, PxU32 constantBlockSize,
 PxHitFlags& queryFlags)
{
	//filterData0 is the vehicle suspension query.
	//filterData1 is the shape potentially hit by the query.
	PX_UNUSED(filterData0);
	PX_UNUSED(constantBlock);
	PX_UNUSED(constantBlockSize);
	PX_UNUSED(queryFlags);
	return ((0 == (filterData1.word3 & DRIVABLE_SURFACE)) ? PxQueryHitType::eNONE : PxQueryHitType::eBLOCK);
}

PxQueryHitType::Enum WheelSceneQueryPostFilterBlocking
(PxFilterData filterData0, PxFilterData filterData1,
 const void* constantBlock, PxU32 constantBlockSize,
 const PxQueryHit& hit)
{
	PX_UNUSED(filterData0);
	PX_UNUSED(filterData1);
	PX_UNUSED(constantBlock);
	PX_UNUSED(constantBlockSize);
	if((static_cast<const PxSweepHit&>(hit)).hadInitialOverlap())
		return PxQueryHitType::eNONE;
	return PxQueryHitType::eBLOCK;
}

PxQueryHitType::Enum WheelSceneQueryPreFilterNonBlocking
(PxFilterData filterData0, PxFilterData filterData1,
const void* constantBlock, PxU32 constantBlockSize,
PxHitFlags& queryFlags)
{
	//filterData0 is the vehicle suspension query.
	//filterData1 is the shape potentially hit by the query.
	PX_UNUSED(filterData0);
	PX_UNUSED(constantBlock);
	PX_UNUSED(constantBlockSize);
	PX_UNUSED(queryFlags);
	return ((0 == (filterData1.word3 & DRIVABLE_SURFACE)) ? PxQueryHitType::eNONE : PxQueryHitType::eTOUCH);
}

PxQueryHitType::Enum WheelSceneQueryPostFilterNonBlocking
(PxFilterData filterData0, PxFilterData filterData1,
const void* constantBlock, PxU32 constantBlockSize,
const PxQueryHit& hit)
{
	PX_UNUSED(filterData0);
	PX_UNUSED(filterData1);
	PX_UNUSED(constantBlock);
	PX_UNUSED(constantBlockSize);
	if ((static_cast<const PxSweepHit&>(hit)).hadInitialOverlap())
		return PxQueryHitType::eNONE;
	return PxQueryHitType::eTOUCH;
}

VehicleSceneQueryData::VehicleSceneQueryData()
:  mNumQueriesPerBatch(0),
   mNumHitResultsPerQuery(0),
   mRaycastResults(NULL),
   mRaycastHitBuffer(NULL),
   mPreFilterShader(NULL),
   mPostFilterShader(NULL)
{
}

VehicleSceneQueryData::~VehicleSceneQueryData()
{
}

VehicleSceneQueryData* VehicleSceneQueryData::allocate
(const PxU32 maxNumVehicles, const PxU32 maxNumWheelsPerVehicle, const PxU32 maxNumHitPointsPerWheel, const PxU32 numVehiclesInBatch, 
 PxBatchQueryPreFilterShader preFilterShader, PxBatchQueryPostFilterShader postFilterShader, 
 PxAllocatorCallback& allocator)
{
	const PxU32 sqDataSize = ((sizeof(VehicleSceneQueryData) + 15) & ~15);

	const PxU32 maxNumWheels = maxNumVehicles*maxNumWheelsPerVehicle;
	const PxU32 raycastResultSize = ((sizeof(PxRaycastQueryResult)*maxNumWheels + 15) & ~15);
	const PxU32 sweepResultSize = ((sizeof(PxSweepQueryResult)*maxNumWheels + 15) & ~15);

	const PxU32 maxNumHitPoints = maxNumWheels*maxNumHitPointsPerWheel;
	const PxU32 raycastHitSize = ((sizeof(PxRaycastHit)*maxNumHitPoints + 15) & ~15);
	const PxU32 sweepHitSize = ((sizeof(PxSweepHit)*maxNumHitPoints + 15) & ~15);

	const PxU32 size = sqDataSize + raycastResultSize + raycastHitSize + sweepResultSize + sweepHitSize;
	PxU8* buffer = static_cast<PxU8*>(allocator.allocate(size, NULL, NULL, 0));
	
	VehicleSceneQueryData* sqData = new(buffer) VehicleSceneQueryData();
	sqData->mNumQueriesPerBatch = numVehiclesInBatch*maxNumWheelsPerVehicle;
	sqData->mNumHitResultsPerQuery = maxNumHitPointsPerWheel;
	buffer += sqDataSize;
	
	sqData->mRaycastResults = reinterpret_cast<PxRaycastQueryResult*>(buffer);
	buffer += raycastResultSize;

	sqData->mRaycastHitBuffer = reinterpret_cast<PxRaycastHit*>(buffer);
	buffer += raycastHitSize;

	sqData->mSweepResults = reinterpret_cast<PxSweepQueryResult*>(buffer);
	buffer += sweepResultSize;

	sqData->mSweepHitBuffer = reinterpret_cast<PxSweepHit*>(buffer);
	buffer += sweepHitSize;

	for (PxU32 i = 0; i < maxNumWheels; i++)
	{
		new(sqData->mRaycastResults + i) PxRaycastQueryResult();
		new(sqData->mSweepResults + i) PxSweepQueryResult();
	}

	for (PxU32 i = 0; i < maxNumHitPoints; i++)
	{
		new(sqData->mRaycastHitBuffer + i) PxRaycastHit();
		new(sqData->mSweepHitBuffer + i) PxSweepHit();
	}

	sqData->mPreFilterShader = preFilterShader;
	sqData->mPostFilterShader = postFilterShader;

	return sqData;
}

void VehicleSceneQueryData::free(PxAllocatorCallback& allocator)
{
	allocator.deallocate(this);
}

PxBatchQuery* VehicleSceneQueryData::setUpBatchedSceneQuery(const PxU32 batchId, const VehicleSceneQueryData& vehicleSceneQueryData, PxScene* scene)
{
	const PxU32 maxNumQueriesInBatch =  vehicleSceneQueryData.mNumQueriesPerBatch;
	const PxU32 maxNumHitResultsInBatch = vehicleSceneQueryData.mNumQueriesPerBatch*vehicleSceneQueryData.mNumHitResultsPerQuery;

	PxBatchQueryDesc sqDesc(maxNumQueriesInBatch, maxNumQueriesInBatch, 0);

	sqDesc.queryMemory.userRaycastResultBuffer = vehicleSceneQueryData.mRaycastResults + batchId*maxNumQueriesInBatch;
	sqDesc.queryMemory.userRaycastTouchBuffer = vehicleSceneQueryData.mRaycastHitBuffer + batchId*maxNumHitResultsInBatch;
	sqDesc.queryMemory.raycastTouchBufferSize = maxNumHitResultsInBatch;

	sqDesc.queryMemory.userSweepResultBuffer = vehicleSceneQueryData.mSweepResults + batchId*maxNumQueriesInBatch;
	sqDesc.queryMemory.userSweepTouchBuffer = vehicleSceneQueryData.mSweepHitBuffer + batchId*maxNumHitResultsInBatch;
	sqDesc.queryMemory.sweepTouchBufferSize = maxNumHitResultsInBatch;

	sqDesc.preFilterShader = vehicleSceneQueryData.mPreFilterShader;

	sqDesc.postFilterShader = vehicleSceneQueryData.mPostFilterShader;

	return scene->createBatchQuery(sqDesc);
}

PxRaycastQueryResult* VehicleSceneQueryData::getRaycastQueryResultBuffer(const PxU32 batchId) 
{
	return (mRaycastResults + batchId*mNumQueriesPerBatch);
}

PxSweepQueryResult* VehicleSceneQueryData::getSweepQueryResultBuffer(const PxU32 batchId)
{
	return (mSweepResults + batchId*mNumQueriesPerBatch);
}


PxU32 VehicleSceneQueryData::getQueryResultBufferSize() const 
{
	return mNumQueriesPerBatch;
}

} // namespace snippetvehicle
