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

#ifndef SNIPPET_VEHICLE_WHEELQUERYRESULT_H
#define SNIPPET_VEHICLE_WHEELQUERYRESULT_H

#include "PxPhysicsAPI.h"
#include <new>

namespace snippetvehicle
{
using namespace physx;

//Data structure for quick setup of wheel query data structures.
class VehicleWheelQueryResults
{
public:

	VehicleWheelQueryResults()
		: mVehicleWheelQueryResults(NULL)
	{
	}

	~VehicleWheelQueryResults()
	{
	}

	//Allocate wheel results for up to maxNumVehicles with up to maxNumWheelsPerVehicle.
	static VehicleWheelQueryResults* allocate(const PxU32 maxNumVehicles, const PxU32 maxNumWheelsPerVehicle, PxAllocatorCallback& allocator)
	{
		const PxU32 byteSize = sizeof(VehicleWheelQueryResults) + sizeof(PxVehicleWheelQueryResult)*maxNumVehicles + sizeof(PxWheelQueryResult)*maxNumWheelsPerVehicle*maxNumVehicles;

		PxU8* buffer = static_cast<PxU8*>(allocator.allocate(byteSize, NULL, NULL, 0));

		VehicleWheelQueryResults* vwqr = reinterpret_cast<VehicleWheelQueryResults*>(buffer);
		buffer += sizeof(VehicleWheelQueryResults);

		vwqr->mVehicleWheelQueryResults = reinterpret_cast<PxVehicleWheelQueryResult*>(buffer);
		buffer+=sizeof(PxVehicleWheelQueryResult)*maxNumVehicles;

		for(PxU32 i=0;i<maxNumVehicles;i++)
		{
			new(buffer) PxWheelQueryResult();
			vwqr->mVehicleWheelQueryResults[i].wheelQueryResults = reinterpret_cast<PxWheelQueryResult*>(buffer);
			vwqr->mVehicleWheelQueryResults[i].nbWheelQueryResults = maxNumWheelsPerVehicle;
			buffer += sizeof(PxWheelQueryResult)*maxNumWheelsPerVehicle;
		}

		return vwqr;
	}

	//Free allocated buffer for scene queries of suspension raycasts.
	void free(PxAllocatorCallback& allocator)
	{
		allocator.deallocate(this);
	}

	//Return the PxVehicleWheelQueryResult for a vehicle specified by an index.
	PxVehicleWheelQueryResult* getVehicleWheelQueryResults(const PxU32 id)
	{
		return (mVehicleWheelQueryResults + id);
	}

private:

	PxVehicleWheelQueryResult* mVehicleWheelQueryResults;
};

} // namespace snippetvehicle

#endif //SNIPPET_VEHICLE_WHEELQUERYRESULT_H
