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

#ifndef SAMPLE_CHARACTER_HELPERS_H
#define SAMPLE_CHARACTER_HELPERS_H

#include "foundation/PxBounds3.h"
#include "foundation/PxFlags.h"
#include "foundation/PxTransform.h"

#include "SampleAllocator.h"
#include "SampleArray.h"
#include "AcclaimLoader.h"


class Character;

///////////////////////////////////////////////////////////////////////////////
class Skin
{
public:
	bool bindToCharacter(Character& character, SampleArray<PxVec4> &positions);
	bool computeNewPositions(Character& characeter, SampleArray<PxVec3> &positions); 

protected:
	SampleArray<PxVec3> mBindPos;
	int				    mBoneID;
};


///////////////////////////////////////////////////////////////////////////////
// Helper class to read and setup motion clip  
// This class provides enough functionality to create a moving stickman character and use it
// for samples and tests.
class Character
{
	///////////////////////////////////////////////////////////////////////////////
	struct MotionData : public SampleAllocateable
	{
		PxTransform mRootTransform;
		PxTransform mBoneTransform[MAX_BONE_NUMBER];
	};

	struct Motion : public SampleAllocateable
	{
		MotionData*    mMotionData;
		PxU32          mNbFrames;
		PxReal         mDistance; // distance from first to last frame
	public:
		void release() { SAMPLE_FREE(mMotionData); delete this; }
	};

public:
	Character();

	virtual ~Character() { release(); }

	/// read a motion clip from .amc file, returns a motion handle or -1 if amc file is invalid.
	int addMotion(const char* amcFileName, PxU32 start = 0, PxU32 end = 10000);

	/// create character from .asf setup
	bool create(const char* asfFileName, PxReal scale = 1.0f);

	/// orient this character in the given direction
	bool faceToward(const PxVec3 &dir, PxReal angleLimitPerFrame = 360.0f);

	/// create a pose info enough to create stickman
	bool getFramePose(PxTransform &rootTransform, SampleArray<PxVec3> &positions, SampleArray<PxU32> &indexBuffers);

	/// set next pose from the motion clip (speed = 1.0 will match original motion clip)
	bool move(PxReal speed = 1.0f, bool jump = false, bool active = true);

	/// reset motion to first frame
	void resetMotion(PxReal firstFrame = 0.0f);

	/// move forward
	bool setForward();

	/// set global pose
	bool setGlobalPose(const PxTransform &transform);

	/// set goal position
	bool setGoalPosition(const PxVec3 pos);

	/// select motion
	bool setMotion(PxU32 motionhandle, bool init = false);

	/// set target position
	bool setTargetPosition(const PxVec3 pos);

protected:

	/// build internal motion data from raw amc data
	bool buildMotion(Acclaim::AMCData&, Motion&, PxU32 start, PxU32 end);

	/// compute per-frame pose cache
	bool computeFramePose();

	/// read a motion clip from .amc file
	bool readSetup(const char* asfFileName);

	/// release memory
	void release();

private:
	SampleArray<Motion*>  mMotions;
	PxTransform			  mCurrentBoneTransform[MAX_BONE_NUMBER];
	PxTransform           mCurrentRootTransform;
	Motion*               mCurrentMotion;
	Motion*               mTargetMotion;
	PxU32                 mBlendCounter;
	PxReal                mCharacterScale;

	Acclaim::ASFData*     mASFData;
	PxU32                 mNbBones;

	PxVec3                mGoalPosition;
	PxVec3                mTargetPosition;
	PxTransform           mCharacterPose; // transformation of the character itself
	PxReal                mFrameTime;
	

	friend class Skin;
};

#endif // SAMPLE_CHARACTER_HELPERS
