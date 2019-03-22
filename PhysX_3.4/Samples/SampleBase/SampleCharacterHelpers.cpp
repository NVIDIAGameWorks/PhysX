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

#include <stdio.h>

#include "SampleCharacterHelpers.h"

#include "AcclaimLoader.h"
#include "PsMathUtils.h"
#include "SampleAllocatorSDKClasses.h"
#include "SampleArray.h"

///////////////////////////////////////////////////////////////////////////////
static Acclaim::Bone* getBoneFromName(Acclaim::ASFData &data, const char *name)
{
	// use a simple linear search -> probably we could use hash map if performance is an issue 
	for (PxU32 i = 0; i < data.mNbBones; i++)
	{
		if (strcmp(name, data.mBones[i].mName) == 0)
			return &data.mBones[i];
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
inline PxQuat EulerAngleToQuat(const PxVec3 &rot)
{
	PxQuat qx(Ps::degToRad(rot.x), PxVec3(1.0f, 0.0f, 0.0f));
	PxQuat qy(Ps::degToRad(rot.y), PxVec3(0.0f, 1.0f, 0.0f));
	PxQuat qz(Ps::degToRad(rot.z), PxVec3(0.0f, 0.0f, 1.0f));
	return qz * qy * qx;
}

///////////////////////////////////////////////////////////////////////////////
static PxTransform computeBoneTransform(PxTransform &rootTransform, Acclaim::Bone &bone, PxVec3* boneFrameData)
{		
	using namespace Acclaim;

	//PxTransform rootTransform(PxVec3(0.0f), PxQuat(PxIdentity));
	PxTransform parentTransform = (bone.mParent) ? 
		computeBoneTransform(rootTransform, *bone.mParent, boneFrameData) : rootTransform;
		
	PxQuat qWorld = EulerAngleToQuat(bone.mAxis);
	PxVec3 offset = bone.mLength * bone.mDirection;
	PxQuat qDelta = PxQuat(PxIdentity);
	PxVec3 boneData = boneFrameData[bone.mID-1];
	
	if (bone.mDOF & BoneDOFFlag::eRX)
		qDelta = PxQuat(Ps::degToRad(boneData.x), PxVec3(1.0f, 0.0f, 0.0f)) * qDelta;
	if (bone.mDOF & BoneDOFFlag::eRY)
		qDelta = PxQuat(Ps::degToRad(boneData.y), PxVec3(0.0f, 1.0f, 0.0f)) * qDelta;
	if (bone.mDOF & BoneDOFFlag::eRZ)
		qDelta = PxQuat(Ps::degToRad(boneData.z), PxVec3(0.0f, 0.0f, 1.0f)) * qDelta;

	PxQuat boneOrientation = qWorld * qDelta * qWorld.getConjugate();

	PxTransform boneTransform(boneOrientation.rotate(offset), boneOrientation);

	return parentTransform.transform(boneTransform);
}

///////////////////////////////////////////////////////////////////////////////
static PxTransform computeBoneTransformRest(Acclaim::Bone &bone)
{		
	using namespace Acclaim;

	PxTransform parentTransform = (bone.mParent) ? 
		computeBoneTransformRest(*bone.mParent) : PxTransform(PxVec3(0.0f), PxQuat(PxIdentity));
		
	PxVec3 offset = bone.mLength * bone.mDirection;

	PxTransform boneTransform(offset, PxQuat(PxIdentity));

	return parentTransform.transform(boneTransform);
}

	
///////////////////////////////////////////////////////////////////////////////
Character::Character() : 
	mCurrentMotion(NULL),
	mTargetMotion(NULL),
	mBlendCounter(0),
	mCharacterScale(1.0f),
	mASFData(NULL), 
	mCharacterPose(PxVec3(0.0f), PxQuat(PxIdentity)),
	mFrameTime(0.0f)
{
	
}

///////////////////////////////////////////////////////////////////////////////
int Character::addMotion(const char* amcFileName, PxU32 start, PxU32 end)
{
	Acclaim::AMCData AMCData;

	if (Acclaim::readAMCData(amcFileName, *mASFData, AMCData) == false)
	{
		AMCData.release();
		return -1;
	}

	if (AMCData.mNbFrames == 0)
	{
		AMCData.release();
		return -1;
	}

	if (end >= AMCData.mNbFrames)
		end = AMCData.mNbFrames - 1;

	Motion* motion = SAMPLE_NEW(Motion)();

	if (buildMotion(AMCData, *motion, start, end) == false)
	{
		SAMPLE_FREE(motion);
		AMCData.release();
		return -1;
	}

	mMotions.pushBack(motion);
	mCurrentMotion = motion;

	// set the frame counter to 0
	mFrameTime = 0.0f;
	
	AMCData.release();

	return mMotions.size() - 1;
}

///////////////////////////////////////////////////////////////////////////////
bool Character::buildMotion(Acclaim::AMCData &amcData, Motion &motion, PxU32 start, PxU32 end)
{
	using namespace Acclaim;

	if  (mASFData == NULL)
		return false;

	motion.mNbFrames = end - start + 1;
	motion.mMotionData = SAMPLE_NEW(MotionData)[motion.mNbFrames];

	// compute bounds of all the motion data on normalized frame
	PxBounds3 bounds = PxBounds3::empty();
	for (PxU32 i = start; i < end; i++)
	{
		Acclaim::FrameData &frameData = amcData.mFrameData[i];

		PxTransform rootTransform(PxVec3(0.0f), EulerAngleToQuat(frameData.mRootOrientation));

		for (PxU32 j = 0; j < mASFData->mNbBones; j++)
		{
			PxTransform t = computeBoneTransform(rootTransform, mASFData->mBones[j], frameData.mBoneFrameData);
			bounds.include(t.p);
		}
	}

	Acclaim::FrameData& firstFrame = amcData.mFrameData[0];
	Acclaim::FrameData& lastFrame = amcData.mFrameData[amcData.mNbFrames-1];

	// compute direction vector
	motion.mDistance = mCharacterScale * (lastFrame.mRootPosition - firstFrame.mRootPosition).magnitude();

	PxVec3 firstPosition = firstFrame.mRootPosition;
	PX_UNUSED(firstPosition);
	PxVec3 firstAngles = firstFrame.mRootOrientation;
	PxQuat firstOrientation = EulerAngleToQuat(PxVec3(0, firstAngles.y, 0));

	for (PxU32 i = 0; i < motion.mNbFrames; i++)
	{
		Acclaim::FrameData& frameData = amcData.mFrameData[i+start];
		MotionData &motionData = motion.mMotionData[i];

		// normalize y-rot by computing inverse quat from first frame
		// this makes all the motion aligned in the same (+ z) direction.
		PxQuat currentOrientation = EulerAngleToQuat(frameData.mRootOrientation);
		PxQuat qdel = firstOrientation.getConjugate() * currentOrientation;
		PxTransform rootTransform(PxVec3(0.0f), qdel);
		
		for (PxU32 j = 0; j < mNbBones; j++)
		{
			PxTransform boneTransform = computeBoneTransform(rootTransform, mASFData->mBones[j], frameData.mBoneFrameData);
			motionData.mBoneTransform[j] = boneTransform;
		}

		//PxReal y = mCharacterScale * (frameData.mRootPosition.y - firstPosition.y) - bounds.minimum.y;
		motionData.mRootTransform = PxTransform(PxVec3(0.0f, -bounds.minimum.y, 0.0f), PxQuat(PxIdentity));
	}

	// now make the motion cyclic by linear interpolating root position and joint angles
	const PxU32 windowSize = 10;
	for (PxU32 i = 0; i <= windowSize; i++)
	{
		PxU32 j = motion.mNbFrames - 1 - windowSize + i;

		PxReal t = PxReal(i) / PxReal(windowSize);

		MotionData& motion_i = motion.mMotionData[0];
		MotionData& motion_j = motion.mMotionData[j];

		// lerp root translation
		PxVec3 blendedRootPos = (1.0f - t) * motion_j.mRootTransform.p  + t * motion_i.mRootTransform.p;
		for (PxU32 k = 0; k < mNbBones; k++)
		{
			PxVec3 pj = motion_j.mRootTransform.p + motion_j.mBoneTransform[k].p;
			PxVec3 pi = motion_i.mRootTransform.p + motion_i.mBoneTransform[k].p;

			PxVec3 p = (1.0f - t) * pj + t * pi;
			motion_j.mBoneTransform[k].p = p - blendedRootPos;
		}
		motion_j.mRootTransform.p = blendedRootPos;
	}

	return true;
}


///////////////////////////////////////////////////////////////////////////////
// we apply pose blending if there is transition from one motion to another
bool
Character::computeFramePose()
{
	PxU32 frameNo = PxU32(mFrameTime);
	if (frameNo >= mCurrentMotion->mNbFrames)
		return false;

	MotionData& motionData = mCurrentMotion->mMotionData[frameNo];

	// compute blended motion when a target motion is set
	if (mTargetMotion)
	{
		// first pose from target motion data
		MotionData& targetData = mTargetMotion->mMotionData[0];
		PxReal t = PxReal(mBlendCounter) / 10.0f;

		for (PxU32 i = 0; i < mNbBones; i++)
		{
			mCurrentBoneTransform[i].p 	= t * motionData.mBoneTransform[i].p + 
					(1.0f - t) * targetData.mBoneTransform[i].p;
			mCurrentBoneTransform[i].q = motionData.mBoneTransform[i].q;
		}
		mCurrentRootTransform.p = t * motionData.mRootTransform.p + 
			(1.0f - t) * targetData.mRootTransform.p;
		mCurrentRootTransform.q = targetData.mRootTransform.q;

	}
	else
	{
		for (PxU32 i = 0; i < mNbBones; i++)
			mCurrentBoneTransform[i] = motionData.mBoneTransform[i];
		mCurrentRootTransform = motionData.mRootTransform;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool Character::create(const char *asfFileName, PxReal scale)
{
	mCharacterScale = scale;

	if (readSetup(asfFileName) == false)
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////
bool Character::faceToward(const PxVec3& targetDir, PxReal angleLimitPerFrame)
{
	PxVec3 oldDir = mCharacterPose.q.rotate(PxVec3(0,0,1));
	PxVec3 up(0,1,0);
	PxVec3 newDir = PxVec3(targetDir.x, 0, targetDir.z).getNormalized();
	PxVec3 right = -1.0f * oldDir.cross(up);

	PxReal cos = newDir.dot(oldDir);
	PxReal sin = newDir.dot(right);
	PxReal angle = atan2(sin, cos);

	PxReal limit = angleLimitPerFrame * (PxPi / 180.0f);
	if (angle > limit) angle = limit;
	else if (angle < -limit) angle = -limit;

	PxQuat qdel(angle, up);

	mCharacterPose.q = qdel * mCharacterPose.q;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool 
Character::getFramePose(PxTransform &rootTransform, SampleArray<PxVec3> &positions, SampleArray<PxU32> &indexBuffers)
{
	if (mCurrentMotion == NULL)
		return false;

	PxU32 frameNo = PxU32(mFrameTime);
	if (frameNo >= mCurrentMotion->mNbFrames)
		return false;

	positions.resize(mNbBones+1);

	// copy precomputed bone position in local space
	positions[0] = PxVec3(0.0f); // root position
	for (PxU32 i = 0; i < mNbBones; i++)
		positions[i+1] = mCurrentBoneTransform[i].p;

	// copy capsule index data
	indexBuffers.resize(mASFData->mNbBones * 2);
	for (PxU32 i = 0; i < mASFData->mNbBones; i++)
	{
		Acclaim::Bone& bone = mASFData->mBones[i];
		indexBuffers[i*2] = bone.mID;
		indexBuffers[i*2+1] = (bone.mParent) ? bone.mParent->mID : 0;
	}

	// compute root transform
	rootTransform = mCharacterPose.transform(mCurrentRootTransform);

	return true;
}

//////////////////////////////////////////////////////////////////////////////
bool Character::move(PxReal speed, bool jump, bool active )
{
	if (mCurrentMotion == NULL)
		return false;

	if (mBlendCounter > 0)
		mBlendCounter--;

	if (mTargetMotion && (mBlendCounter == 0))
	{
		mBlendCounter = 0;
		mCurrentMotion = mTargetMotion;
		mFrameTime = 0.0f;
		mTargetMotion = NULL;
	}

	PxU32 nbFrames = mCurrentMotion->mNbFrames;
	PxReal distance = mCurrentMotion->mDistance;

	PxReal frameDelta = 0.0f;
	const PxReal angleLimitPerFrame = 3.0f;

	if (jump)
	{
		frameDelta = 1.0f;
	}
	else if (active && (mBlendCounter == 0))
	{
		// compute target orientation
		PxVec3 dir = mTargetPosition - mCharacterPose.p;
		dir.y = 0.0f;
		PxReal curDistance = dir.magnitude();

		if (curDistance > 0.01f)
			faceToward(dir, angleLimitPerFrame);
	
		frameDelta = speed * PxReal(nbFrames) * (curDistance / distance);
	}
	else
	{
		frameDelta = 0;
	}

	mCharacterPose.p = mTargetPosition;

	mFrameTime += frameDelta;
	PxU32 frameNo = PxU32(mFrameTime);

	if (frameNo >= nbFrames)
	{
		if (jump)
			mFrameTime = PxReal(nbFrames) - 1.0f;
		else
			mFrameTime = 0.0f;
	}

	// compute pose of all the bones at current frame (results are used by both getFramePose and Skin)
	computeFramePose();

	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool Character::readSetup(const char* asfFileName)
{
	if (mASFData) mASFData->release();

	mASFData = new Acclaim::ASFData;

	if (Acclaim::readASFData(asfFileName, *mASFData) == false)
	{
		delete mASFData;
		mASFData = NULL;
		return false;
	}

	mNbBones = mASFData->mNbBones;

	// scale bone length
	for (PxU32 i = 0; i < mASFData->mNbBones; i++)
	{
		Acclaim::Bone& bone = mASFData->mBones[i];

		bone.mLength *= mCharacterScale;
	}

	return true;
}



///////////////////////////////////////////////////////////////////////////////
void Character::release()
{
	for (PxU32 i = 0; i < mMotions.size(); i++)
	{
		if (mMotions[i])
			mMotions[i]->release();
	}
	if (mASFData) mASFData->release();
}

///////////////////////////////////////////////////////////////////////////////
void Character::resetMotion(PxReal firstFrame)
{
	mFrameTime = firstFrame;
}

///////////////////////////////////////////////////////////////////////////////
bool Character::setGlobalPose(const PxTransform &transform)
{
	mCharacterPose.p = transform.p;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool Character::setGoalPosition(const PxVec3 pos)
{
	mGoalPosition = pos;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool Character::setMotion(PxU32 id, bool init)
{
	if (id >= mMotions.size())
		return false;

	if (init)
	{
		mCurrentMotion = mMotions[id];
		mTargetMotion = NULL;

		computeFramePose();
		return true;
	}

	if  (mCurrentMotion == NULL)
	{
		mCurrentMotion = mMotions[id];
		return true;
	}

	if (mCurrentMotion == mMotions[id])
		return true;

	if (mTargetMotion == mMotions[id])
		return true;

	mTargetMotion = mMotions[id];
	mBlendCounter = 10;
	
	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool Character::setTargetPosition(const PxVec3 pos)
{
	mTargetPosition = pos;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool Character::setForward(void)
{
	PxVec3 p = mCharacterPose.p;

	PxVec3 dir = mGoalPosition - p;
	dir.normalize();

	dir = mCharacterPose.q.rotate(dir);

	PxU32 nbFrames = mCurrentMotion->mNbFrames;
	PxReal distance = mCurrentMotion->mDistance;

	PxReal frameDelta = distance / PxReal(nbFrames);

	mTargetPosition = p + frameDelta * dir;

	return true;
}


///////////////////////////////////////////////////////////////////////////////
bool
Skin::bindToCharacter(Character &character, SampleArray<PxVec4> &positions)
{
	// currently we just bind everything to the 'thorax' (between neck and clavicles).
	// Modify this if you need to do more elaborate skin binding

	mBindPos.resize(positions.size());
	Acclaim::Bone* bone = getBoneFromName(*character.mASFData, "thorax");
	if (bone == NULL)
		return false;

	PxTransform boneTransform = computeBoneTransformRest(*bone);
	PxTransform boneTransformInv = boneTransform.getInverse();

	mBoneID = bone->mID - 1;

	for (PxU32 i = 0; i < positions.size(); i++)
	{
		mBindPos[i] = boneTransformInv.transform(
			reinterpret_cast<const PxVec3&>(positions[i]));
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool
Skin::computeNewPositions(Character &character, SampleArray<PxVec3> &particlePositions)
{
	if (character.mCurrentMotion == NULL)
		return false;

	PxTransform t = character.mCurrentBoneTransform[mBoneID];
	particlePositions.resize(mBindPos.size());

	for (PxU32 i = 0; i < mBindPos.size(); i++)
		particlePositions[i] = t.transform(mBindPos[i]);

	return true;
}


