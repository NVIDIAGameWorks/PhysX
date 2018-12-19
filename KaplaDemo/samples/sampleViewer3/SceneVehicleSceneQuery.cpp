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

#include "SceneVehicleSceneQuery.h"
#include "vehicle/PxVehicleSDK.h"
#include "PxFiltering.h"
#include "PsFoundation.h"
#include "PsUtilities.h"

#define CHECK_MSG(exp, msg) (!!(exp) || (physx::shdfnd::getFoundation().error(physx::PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, msg), 0) )
#define SIZEALIGN16(size) (((unsigned)(size)+15)&((unsigned)(~15)));

void VehicleSetupDrivableShapeQueryFilterData(PxFilterData* qryFilterData)
{
	CHECK_MSG(0 == qryFilterData->word3, "word3 is reserved for filter data for vehicle raycast queries");
	qryFilterData->word3 = (PxU32)VEHICLE_DRIVABLE_SURFACE;
}

void VehicleSetupNonDrivableShapeQueryFilterData(PxFilterData* qryFilterData)
{
	CHECK_MSG(0 == qryFilterData->word3, "word3 is reserved for filter data for vehicle raycast queries");
	qryFilterData->word3 = (PxU32)VEHICLE_UNDRIVABLE_SURFACE;
}

void VehicleSetupVehicleShapeQueryFilterData(PxFilterData* qryFilterData)
{
	CHECK_MSG(0 == qryFilterData->word3, "word3 is reserved for filter data for vehicle raycast queries");
	qryFilterData->word3 = (PxU32)VEHICLE_UNDRIVABLE_SURFACE;
}

VehicleSceneQueryData* VehicleSceneQueryData::allocate(const PxU32 maxNumWheels)
{
	
	VehicleSceneQueryData* sqData = (VehicleSceneQueryData*)PX_ALLOC(sizeof(VehicleSceneQueryData), "PxVehicleNWSceneQueryData");
	sqData->init();
	sqData->mSqResults = (PxRaycastQueryResult*)PX_ALLOC(sizeof(PxRaycastQueryResult)*maxNumWheels, "PxRaycastQueryResult");
	sqData->mNbSqResults = maxNumWheels;
	sqData->mSqHitBuffer = (PxRaycastHit*)PX_ALLOC(sizeof(PxRaycastHit)*maxNumWheels, "PxRaycastHit");
	sqData->mNumQueries = maxNumWheels;

	sqData->mSqSweepResults = (PxSweepQueryResult*)PX_ALLOC(sizeof(PxSweepQueryResult) * maxNumWheels, "PxSweepQueryResult");
	sqData->mNbSqSweepResults = maxNumWheels;
	sqData->mSqSweepHitBuffer = (PxSweepHit*)PX_ALLOC(sizeof(PxSweepHit) * maxNumWheels, "PxSweepHit");
	sqData->mNumSweepQueries = maxNumWheels;

	return sqData;
}

void VehicleSceneQueryData::free()
{
	PX_FREE(this);
}

PxBatchQuery* VehicleSceneQueryData::setUpBatchedSceneQuery(PxScene* scene)
{
	PxBatchQueryDesc sqDesc(mNbSqResults, 0, 0);
	sqDesc.queryMemory.userRaycastResultBuffer = mSqResults;
	sqDesc.queryMemory.userRaycastTouchBuffer = mSqHitBuffer;
	sqDesc.queryMemory.raycastTouchBufferSize = mNumQueries;
	sqDesc.preFilterShader = mPreFilterShader;
	return scene->createBatchQuery(sqDesc);
}

PxBatchQuery* VehicleSceneQueryData::setUpBatchedSceneQuerySweep(PxScene* scene)
{
	PxBatchQueryDesc sqDesc(0, mNbSqResults, 0);
	sqDesc.queryMemory.userSweepResultBuffer = mSqSweepResults;
	sqDesc.queryMemory.userSweepTouchBuffer = mSqSweepHitBuffer;
	sqDesc.queryMemory.sweepTouchBufferSize = mNumQueries;
	sqDesc.preFilterShader = mPreFilterShader;
	return scene->createBatchQuery(sqDesc);
}


