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

#ifndef CRAB_H
#define CRAB_H

#include "foundation/Px.h"
#include "foundation/PxSimpleTypes.h"
#include "common/PxPhysXCommonConfig.h"
#include "task/PxTask.h"
#include <vector>

#define UPDATE_FREQUENCY_RESET 10


struct CrabState
{
	enum Enum
	{
		eWAITING,
		eMOVE_FWD,
		eMOVE_BKWD,
		eROTATE_LEFT,
		eROTATE_RIGHT,
		ePANIC,
		eNUM_STATES,
	};
};



namespace physx
{
	class PxRigidDynamic;
	class PxRevoluteJoint;
	class PxJoint;
	class PxScene;
}

//  Edge Labels for Theo Jansen's Mechanism
//		       _.
//		     ,' |       m
//		   ,'   |2b   .....
//		 ,' 2a  |    e    |n
//		+-------+---------+
//		|       |
//		|       |2c
//		|       |
//		+-------+
//		 `.     |
//		   `.   |2d
//		     `. |
//		       `|

struct LegParameters
{
	physx::PxReal a;	// half x-dim leg
	physx::PxReal b;	// half height upper triangle
	physx::PxReal c;	// half height distance joints (square)
	physx::PxReal d;	// half height lower triangle
	physx::PxReal e;	// x distance from main body centre
	physx::PxReal m;	// half extent of the motor box
	physx::PxReal n;	// y offset of the motor from the main body centre
};

class Crab;
class CrabManager;

struct SqRayBuffer
{
	SqRayBuffer(Crab* crab, physx::PxU32 numRays, physx::PxU32 numHits);
	~SqRayBuffer();

	Crab*						mCrab;
	physx::PxBatchQuery*					mBatchQuery;

	physx::PxRaycastQueryResult*			mRayCastResults;
	physx::PxRaycastHit*					mRayCastHits;

	physx::PxU32 							mQueryResultSize;
	physx::PxU32 							mHitSize;
	void*							mOrigAddresses[2];

	void setScene(physx::PxScene* scene);
private:
	SqRayBuffer& operator=(const SqRayBuffer&);
};

struct SqSweepBuffer
{
	SqSweepBuffer(Crab* crab, physx::PxU32 numRays, physx::PxU32 numHits);
	~SqSweepBuffer();

	Crab*									mCrab;
	physx::PxBatchQuery*					mBatchQuery;

	physx::PxSweepQueryResult*				mSweepResults;
	physx::PxSweepHit*						mSweepHits;

	physx::PxU32 							mQueryResultSize;
	physx::PxU32 							mHitSize;
	void*									mOrigAddresses[2];

	void setScene(physx::PxScene* scene);
private:
	SqSweepBuffer& operator=(const SqSweepBuffer&);
};

class Crab
{
public:

	Crab(CrabManager* crabManager, physx::PxU32 updateFrequency);
	~Crab();

	void	update(physx::PxReal dt);
	void	setAcceleration(physx::PxReal leftAcc, physx::PxReal rightAcc);
	void	flushAccelerationBuffer();

	PX_INLINE const physx::PxRigidDynamic*		getCrabBody() const		{ return mCrabBody; }
	PX_INLINE physx::PxRigidDynamic*			getCrabBody()			{ return mCrabBody; }


	void								run();

	void setScene(physx::PxScene* scene);



	void	initMembers();
	Crab*	create(const physx::PxVec3& _crabPos, const physx::PxReal crabDepth, const physx::PxReal scale, const physx::PxReal legMass, const physx::PxU32 numLegs);

	physx::PxVec3	getPlaceOnFloor(physx::PxVec3 pos);
	void	createLeg(physx::PxRigidDynamic* mainBody, physx::PxVec3 localPos, physx::PxReal mass, const LegParameters& params, physx::PxReal scale, physx::PxRigidDynamic* motor, physx::PxVec3 motorAttachmentPos);
	void	scanForObstacles();
	void	updateState();
	void	initState(CrabState::Enum state);

	void setManager(CrabManager* crabManager) { mManager = crabManager;  }
	CrabManager* getManager(){ return mManager;  }

private:
	CrabManager*								mManager;
	physx::PxRigidDynamic*						mCrabBody;
	physx::PxVec3								mCrabBodyDimensions;
	//physx::PxRevoluteJoint*						mMotorJoint[2];
	physx::PxD6Joint*							mMotorJoint[2];
	physx::PxMaterial*							mMaterial;

	physx::PxReal								mAcceleration[2];
	//SqRayBuffer*								mSqRayBuffer;
	SqSweepBuffer*								mSqSweepBuffer;

	physx::PxReal								mLegHeight;

	CrabState::Enum								mCrabState;
	physx::PxReal								mStateTime;
	physx::PxReal								mDistances[10];
	physx::PxReal								mAccumTime;
	physx::PxReal								mElapsedTime;
	physx::PxReal								mAccelerationBuffer[2];
	physx::PxU32								mUpdateFrequency;
};

#endif
