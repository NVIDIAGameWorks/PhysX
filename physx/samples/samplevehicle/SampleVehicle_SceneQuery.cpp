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

#include "SampleVehicle_SceneQuery.h"
#include "vehicle/PxVehicleSDK.h"
#include "PxFiltering.h"
#include "PsFoundation.h"
#include "PsUtilities.h"

#define CHECK_MSG(exp, msg) (!!(exp) || (physx::shdfnd::getFoundation().error(physx::PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, msg), 0) )
#define SIZEALIGN16(size) (((unsigned)(size)+15)&((unsigned)(~15)));

void SampleVehicleSetupDrivableShapeQueryFilterData(PxFilterData* qryFilterData)
{
	CHECK_MSG(0==qryFilterData->word3, "word3 is reserved for filter data for vehicle raycast queries");
	qryFilterData->word3 = (PxU32)SAMPLEVEHICLE_DRIVABLE_SURFACE;
}

void SampleVehicleSetupNonDrivableShapeQueryFilterData(PxFilterData* qryFilterData)
{
	CHECK_MSG(0==qryFilterData->word3, "word3 is reserved for filter data for vehicle raycast queries");
	qryFilterData->word3 = (PxU32)SAMPLEVEHICLE_UNDRIVABLE_SURFACE;
}

void SampleVehicleSetupVehicleShapeQueryFilterData(PxFilterData* qryFilterData)
{
	CHECK_MSG(0==qryFilterData->word3, "word3 is reserved for filter data for vehicle raycast queries");
	qryFilterData->word3 = (PxU32)SAMPLEVEHICLE_UNDRIVABLE_SURFACE;
}

SampleVehicleSceneQueryData* SampleVehicleSceneQueryData::allocate(const PxU32 maxNumWheels)
{
	const PxU32 size0 = SIZEALIGN16(sizeof(SampleVehicleSceneQueryData));
	const PxU32 size1 = SIZEALIGN16(sizeof(PxRaycastQueryResult)*maxNumWheels);
	const PxU32 size2 = SIZEALIGN16(sizeof(PxRaycastHit)*maxNumWheels);
	const PxU32 size = size0 + size1 + size2;
	SampleVehicleSceneQueryData* sqData = (SampleVehicleSceneQueryData*)PX_ALLOC(size, "PxVehicleNWSceneQueryData");
	sqData->init();
	PxU8* ptr = (PxU8*) sqData;
	ptr += size0;
	sqData->mSqResults = (PxRaycastQueryResult*)ptr;
	sqData->mNbSqResults = maxNumWheels;
	ptr += size1;
	sqData->mSqHitBuffer = (PxRaycastHit*)ptr;
	ptr += size2;
	sqData->mNumQueries = maxNumWheels;
	return sqData;
}

void SampleVehicleSceneQueryData::free()
{
	PX_FREE(this);
}

PxBatchQuery* SampleVehicleSceneQueryData::setUpBatchedSceneQuery(PxScene* scene)
{
	PxBatchQueryDesc sqDesc(mNbSqResults, 0, 0);
	sqDesc.queryMemory.userRaycastResultBuffer = mSqResults;
	sqDesc.queryMemory.userRaycastTouchBuffer = mSqHitBuffer;
	sqDesc.queryMemory.raycastTouchBufferSize = mNumQueries;
	sqDesc.preFilterShader = mPreFilterShader;
	return scene->createBatchQuery(sqDesc);
}

