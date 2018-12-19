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

#include "SampleCharacterCloth.h"

#include "SampleCharacterClothPlatform.h"


///////////////////////////////////////////////////////////////////////////////
SampleCharacterClothPlatform::SampleCharacterClothPlatform() : 
	mActor(NULL), 
	mCurrentTime(0.0f), 
	mTravelTime(0.0f), 
	mStart(0.0f),
	mTarget(0.0f),
	mFlip(false),
	mType(ePLATFORM_TYPE_MOVING),
	mActive(true)
	{
	}

///////////////////////////////////////////////////////////////////////////////
void SampleCharacterClothPlatform::init(PxRigidDynamic* actor, PxReal travelTime, const PxVec3& offsetTarget)
{
	mActor = actor;
	mCurrentTime = 0.0f;
	mTravelTime = travelTime;

	mStart = mActor->getGlobalPose().p;
	mTarget = mStart + offsetTarget;

	reset();
}

///////////////////////////////////////////////////////////////////////////////
void SampleCharacterClothPlatform::release()
{
	delete this;
}

///////////////////////////////////////////////////////////////////////////////
void SampleCharacterClothPlatform::reset()
{
	mCurrentTime = 0.0f;
	mFlip = false;

	switch (mType)
	{
	case ePLATFORM_TYPE_MOVING:
		mActive = true;
		break;
	default:
		mActive = false;
	}

	update(0.0f);
}

///////////////////////////////////////////////////////////////////////////////
void SampleCharacterClothPlatform::update(PxReal dtime)
{
	PxReal t = mCurrentTime / mTravelTime;

	PxTransform pose = mActor->getGlobalPose();
	if (!mFlip)
	{
		pose.p = (1 - t) * mStart + t * mTarget;
	}
	else
		pose.p = t * mStart + (1-t) * mTarget;

	mActor->setKinematicTarget(pose);

	if (mActive == false)
		return;

	mCurrentTime += dtime;

	if (mCurrentTime > mTravelTime)
	{
		mCurrentTime -= mTravelTime;
		mFlip = !mFlip;
	}
}

///////////////////////////////////////////////////////////////////////////////
void SampleCharacterCloth::updatePlatforms(float dtime)
{
	const size_t nbPlatforms = mPlatforms.size();
	for(PxU32 i = 0;i < nbPlatforms;i++)
	{
		mPlatforms[i]->update(dtime);
	}
}