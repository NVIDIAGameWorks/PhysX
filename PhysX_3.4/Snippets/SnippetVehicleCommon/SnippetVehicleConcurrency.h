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

#ifndef SNIPPET_VEHICLE_CONCURRENCY_H
#define SNIPPET_VEHICLE_CONCURRENCY_H

#include "PxPhysicsAPI.h"
#include <new>

namespace snippetvehicle
{
using namespace physx;

//Data structure for quick setup of wheel query data structures.
class VehicleConcurrency
{
public:

	VehicleConcurrency()
		: mMaxNumVehicles(0),
		  mMaxNumWheelsPerVehicle(0),
		  mVehicleConcurrentUpdates(NULL)
	{
	}

	~VehicleConcurrency()
	{
	}

	static VehicleConcurrency* allocate(const PxU32 maxNumVehicles, const PxU32 maxNumWheelsPerVehicle, PxAllocatorCallback& allocator)
	{
		const PxU32 byteSize = 
			sizeof(VehicleConcurrency) + 
			sizeof(PxVehicleConcurrentUpdateData)*maxNumVehicles + 
			sizeof(PxVehicleWheelConcurrentUpdateData)*maxNumWheelsPerVehicle*maxNumVehicles;

		PxU8* buffer = static_cast<PxU8*>(allocator.allocate(byteSize, NULL, NULL, 0));

		VehicleConcurrency* vc = reinterpret_cast<VehicleConcurrency*>(buffer);
		new(vc) VehicleConcurrency();
		buffer += sizeof(VehicleConcurrency);

		vc->mMaxNumVehicles = maxNumVehicles;
		vc->mMaxNumWheelsPerVehicle = maxNumWheelsPerVehicle;

		vc->mVehicleConcurrentUpdates = reinterpret_cast<PxVehicleConcurrentUpdateData*>(buffer);
		buffer += sizeof(PxVehicleConcurrentUpdateData)*maxNumVehicles;

		for(PxU32 i=0;i<maxNumVehicles;i++)
		{
			new(vc->mVehicleConcurrentUpdates + i) PxVehicleConcurrentUpdateData();
			
			vc->mVehicleConcurrentUpdates[i].nbConcurrentWheelUpdates = maxNumWheelsPerVehicle;
			
			vc->mVehicleConcurrentUpdates[i].concurrentWheelUpdates = reinterpret_cast<PxVehicleWheelConcurrentUpdateData*>(buffer);
			buffer += sizeof(PxVehicleWheelConcurrentUpdateData)*maxNumWheelsPerVehicle;

			for(PxU32 j = 0; j < maxNumWheelsPerVehicle; j++)
			{
				new(vc->mVehicleConcurrentUpdates[i].concurrentWheelUpdates + j) PxVehicleWheelConcurrentUpdateData();
			}

		}


		return vc;
	}

	//Free allocated buffer for scene queries of suspension raycasts.
	void free(PxAllocatorCallback& allocator)
	{
		allocator.deallocate(this);
	}

	//Return the PxVehicleConcurrentUpdate for a vehicle specified by an index.
	PxVehicleConcurrentUpdateData* getVehicleConcurrentUpdate(const PxU32 id)
	{
		return (mVehicleConcurrentUpdates + id);
	}

	//Return the entire array of PxVehicleConcurrentUpdates
	PxVehicleConcurrentUpdateData* getVehicleConcurrentUpdateBuffer()
	{
		return mVehicleConcurrentUpdates;
	}

private:

	PxU32 mMaxNumVehicles;
	PxU32 mMaxNumWheelsPerVehicle;
	PxVehicleConcurrentUpdateData* mVehicleConcurrentUpdates;
};

} // namespace snippetvehicle

#endif //SNIPPET_VEHICLE_CONCURRENCY_H
