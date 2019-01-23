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


#include "PxPhysicsAPI.h"
#include "SampleNorthPole.h"

void SampleNorthPole::customizeSceneDesc(PxSceneDesc& sceneDesc)
{
	sceneDesc.gravity					= PxVec3(0,-9.81f,0);
	sceneDesc.filterShader				= filter;
	sceneDesc.simulationEventCallback	= this;
	sceneDesc.flags						|= PxSceneFlag::eENABLE_CCD;
	sceneDesc.flags						|= PxSceneFlag::eREQUIRE_RW_LOCK;
}

void SampleNorthPole::setSnowball(PxShape& shape)
{
	PxFilterData fd = shape.getSimulationFilterData();
	fd.word3 |= SNOWBALL_FLAG;
	shape.setSimulationFilterData(fd);
}

bool SampleNorthPole::needsContactReport(const PxFilterData& filterData0, const PxFilterData& filterData1)
{
	const PxU32 needsReport = PxU32(DETACHABLE_FLAG | SNOWBALL_FLAG);
	PxU32 flags = (filterData0.word3 | filterData1.word3);
	return (flags & needsReport) == needsReport;
}

void SampleNorthPole::setDetachable(PxShape& shape)
{
	PxFilterData fd = shape.getSimulationFilterData();
	fd.word3 |= PxU32(DETACHABLE_FLAG);
	shape.setSimulationFilterData(fd);
}

bool SampleNorthPole::isDetachable(PxFilterData& filterData)
{
	return filterData.word3 & PxU32(DETACHABLE_FLAG) ? true : false;
}

void SampleNorthPole::setCCDActive(PxShape& shape, PxRigidBody* rigidBody)
{
	rigidBody->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
	PxFilterData fd = shape.getSimulationFilterData();
	fd.word3 |= CCD_FLAG;
	shape.setSimulationFilterData(fd);
	
}

bool SampleNorthPole::isCCDActive(PxFilterData& filterData)
{
	return filterData.word3 & CCD_FLAG ? true : false;
}

PxFilterFlags SampleNorthPole::filter(	PxFilterObjectAttributes attributes0,
						PxFilterData filterData0, 
						PxFilterObjectAttributes attributes1,
						PxFilterData filterData1,
						PxPairFlags& pairFlags,
						const void* constantBlock,
						PxU32 constantBlockSize)
{

	if (isCCDActive(filterData0) || isCCDActive(filterData1))
	{
		pairFlags |= PxPairFlag::eSOLVE_CONTACT;
		pairFlags |= PxPairFlag::eDETECT_CCD_CONTACT;
	}

	if (needsContactReport(filterData0, filterData1))
	{
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
	}

	pairFlags |= PxPairFlag::eCONTACT_DEFAULT;
	return PxFilterFlags();
}

void SampleNorthPole::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
{
	for(PxU32 i=0; i < nbPairs; i++)
	{
		PxU32 n = 2;
		const PxContactPairFlag::Enum delShapeFlags[] = { PxContactPairFlag::eREMOVED_SHAPE_0, PxContactPairFlag::eREMOVED_SHAPE_1 };
		const PxContactPair& cp = pairs[i];
		while(n--)
		{
			if(!(cp.flags & delShapeFlags[n]))
			{
				PxShape* shape = cp.shapes[n];
				PxFilterData fd = shape->getSimulationFilterData();
				if(isDetachable(fd))
				{
					mDetaching.push_back(shape);
				}
			}
		}
	}
}
