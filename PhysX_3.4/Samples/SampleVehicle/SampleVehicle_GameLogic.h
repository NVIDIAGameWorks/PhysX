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


#ifndef SAMPLE_VEHICLE_GAME_LOGIC_H
#define SAMPLE_VEHICLE_GAME_LOGIC_H

#include "common/PxPhysXCommonConfig.h"
#include "foundation/PxTransform.h"

using namespace physx;

class SampleVehicleWayPoints
{
public:

	SampleVehicleWayPoints()
		: mWayPoints(NULL),
		mNumWayPoints(0),
		mProgress(0),
		mTimeElapsed(0),
		mMinTimeElapsed(100000)
	{
	}

	~SampleVehicleWayPoints()
	{
	}

	//Setup.
	void setWayPoints(const PxTransform* wayPoints, const PxU32 numWayPoints) 
	{
		mWayPoints=wayPoints; 
		mNumWayPoints=numWayPoints;
	}

	//Update.
	void update(const PxTransform& playerTransform, const PxF32 timestep);

	//Imagine we are starting the lap again.
	PxTransform setBackAtStart()
	{
		mTimeElapsed=0;
		mProgress=0;
		return mWayPoints[0];
	}

	//Get the next three points and the crossing line of each way-point.
	void getNextWayPointsAndLineDirs(PxU32& numPoints, PxVec3& v0, PxVec3& v1, PxVec3& v2, PxVec3& w0, PxVec3& w1, PxVec3& w2) const;

	//Get lap time and best lap time.
	PxF32 getTimeElapsed() const {return mTimeElapsed;}
	PxF32 getMinTimeElapsed() const {return mMinTimeElapsed;}

	//Get the transform to reset the car at the last passed way-point.
	PxTransform getResetTransform() const {return mWayPoints[mProgress];}

private:

	//Array of way points.
	const PxTransform* mWayPoints;
	PxU32 mNumWayPoints;

	//Progress and time elapsed.
	PxU32 mProgress;
	PxF32 mTimeElapsed;
	PxF32 mMinTimeElapsed;
};


#endif //SAMPLE_VEHICLE_GAME_LOGIC_H