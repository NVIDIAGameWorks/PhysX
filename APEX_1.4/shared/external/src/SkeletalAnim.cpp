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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.


#include <ApexUsingNamespace.h>
#include <PsFileBuffer.h>
#include <MeshImport.h>
#include <AutoGeometry.h>
#include <PsString.h>

#include "SkeletalAnim.h"
#include "TriangleMesh.h"
#include <RenderDebugInterface.h>
#include "PsMathUtils.h"

#include "PxInputDataFromPxFileBuf.h"

#include <algorithm>

namespace Samples
{

// -------------------------------------------------------------------
void SkeletalBone::clear()
{
	name = "";
	id = -1;
	pose = physx::PxTransform(physx::PxIdentity);
	bindWorldPose = physx::PxMat44(physx::PxIdentity);
	invBindWorldPose = physx::PxMat44(physx::PxIdentity);
	currentWorldPose = physx::PxMat44(physx::PxIdentity);

	scale = physx::PxVec3(1.0f, 1.0f, 1.0f);
	parent = -1;
	firstChild = -1;
	numChildren = 0;
	firstVertex = -1;
	boneOption = 0;
	inflateConvex = 0.0f;
	minimalBoneWeight = 0.4f;
	numShapes = 0;
	selected = false;
	allowPrimitives = true;
	dirtyParams = false;
	manualShapes = false;
	isRoot = false;
	isRootLock = false;
}

// -------------------------------------------------------------------
void BoneKeyFrame::clear()
{
	relPose = physx::PxTransform(physx::PxIdentity);
	time = 0.0f;
	scale = physx::PxVec3(1.0f, 1.0f, 1.0f);
}

// -------------------------------------------------------------------
void BoneTrack::clear()
{
	firstFrame = -1;
	numFrames = 0;
}

// -------------------------------------------------------------------
void SkeletalAnimation::clear()
{
	name = "";
	mBoneTracks.clear();
	minTime = 0.0f;
	maxTime = 0.0f;
};

// -------------------------------------------------------------------
SkeletalAnim::SkeletalAnim()
{
	clear();
}

// -------------------------------------------------------------------
SkeletalAnim::~SkeletalAnim()
{
	clear();
}

// -------------------------------------------------------------------
void SkeletalAnim::clear()
{
	ragdollMode = false;

	mBones.clear();
	mBones.resize(0);

	for (uint32_t i = 0; i < mAnimations.size(); i++)
	{
		delete mAnimations[i];
	}

	mAnimations.clear();
	mAnimations.resize(0);

	mKeyFrames.clear();
	mKeyFrames.resize(0);

	mSkinningMatrices.clear();
	mSkinningMatrices.resize(0);
	mSkinningMatricesWorld.clear();
	mSkinningMatricesWorld.resize(0);

	mParent = NULL;
}

// -------------------------------------------------------------------
int SkeletalAnim::findBone(const std::string& name)
{
	for (uint32_t i = 0; i < mBones.size(); i++)
	{
		if (mBones[i].name == name)
		{
			return (int)i;
		}
	}
	return -1;
}

// -------------------------------------------------------------------
void SkeletalAnim::interpolateBonePose(int animNr, int boneNr, float time, physx::PxTransform& pose, physx::PxVec3& scale)
{
	// the default
	pose = physx::PxTransform(physx::PxIdentity);
	scale = physx::PxVec3(1.0f, 1.0f, 1.0f);

	const std::vector<SkeletalAnimation*>& animations = mParent == NULL ? mAnimations : mParent->mAnimations;
	const std::vector<BoneKeyFrame>& keyFrames = mParent == NULL ? mKeyFrames : mParent->mKeyFrames;

	if (animNr < 0 || animNr >= (int)animations.size())
	{
		return;
	}
	if (boneNr < 0 || boneNr >= (int)animations[(uint32_t)animNr]->mBoneTracks.size())
	{
		return;
	}

	BoneTrack& t = animations[(uint32_t)animNr]->mBoneTracks[(uint32_t)boneNr];
	if (t.numFrames == 0)
	{
		return;
	}

	// special cases
	int frameNr = -1;
	if (t.numFrames == 1)
	{
		frameNr = t.firstFrame;
	}
	else if (time <= keyFrames[(uint32_t)t.firstFrame].time)
	{
		frameNr = t.firstFrame;
	}
	else if (time >= keyFrames[uint32_t(t.firstFrame + t.numFrames - 1)].time)
	{
		frameNr = t.firstFrame + t.numFrames - 1;
	}

	if (frameNr >= 0)
	{
		pose = keyFrames[(uint32_t)frameNr].relPose;
		scale = keyFrames[(uint32_t)frameNr].scale;
		return;
	}
	// binary search
	uint32_t l = (uint32_t)t.firstFrame;
	uint32_t r = uint32_t(t.firstFrame + t.numFrames - 1);
	while (r > l + 1)
	{
		uint32_t m = (l + r) / 2;
		if (keyFrames[m].time == time)
		{
			pose = keyFrames[m].relPose;
			scale = keyFrames[m].scale;
			return;
		}
		else if (keyFrames[m].time > time)
		{
			r = m;
		}
		else
		{
			l = m;
		}
	}
	float dt = keyFrames[r].time - keyFrames[l].time;
	// avoid singular case
	if (dt == 0.0f)
	{
		pose = keyFrames[l].relPose;
		scale = keyFrames[l].scale;
	}

	// interpolation
	float sr = (time - keyFrames[l].time) / dt;
	float sl = 1.0f - sr;

	scale = keyFrames[l].scale * sl + keyFrames[r].scale * sr;
	pose.p = keyFrames[l].relPose.p * sl + keyFrames[r].relPose.p * sr;
	pose.q = physx::shdfnd::slerp(sr, keyFrames[l].relPose.q, keyFrames[r].relPose.q);
}

// -------------------------------------------------------------------
void SkeletalAnim::setBindPose()
{
	PX_ASSERT(mBones.size() == mSkinningMatrices.size());
	PX_ASSERT(mBones.size() == mSkinningMatricesWorld.size());
	for (uint32_t i = 0; i < mBones.size(); i++)
	{
		mSkinningMatrices[i] = physx::PxMat44(physx::PxIdentity);
		mBones[i].currentWorldPose = mBones[i].bindWorldPose;
		mSkinningMatricesWorld[i] = mBones[i].currentWorldPose;
	}
}

// -------------------------------------------------------------------
void SkeletalAnim::setAnimPose(int animNr, float time, bool lockRootbone /* = false */)
{
	if (animNr >= 0)
	{
		for (uint32_t i = 0; i < mBones.size(); i++)
		{
			if (mBones[i].parent < 0)
			{
				setAnimPoseRec(animNr, (int)i, time, lockRootbone);
			}
		}

		PX_ASSERT(mBones.size() == mSkinningMatrices.size());
		PX_ASSERT(mBones.size() == mSkinningMatricesWorld.size());
		for (uint32_t i = 0; i < mBones.size(); i++)
		{
			SkeletalBone& b = mBones[i];
			mSkinningMatrices[i] = b.currentWorldPose * b.invBindWorldPose;
			mSkinningMatricesWorld[i] = b.currentWorldPose;
		}
	}
	else
	{
		for (uint32_t i = 0; i < mBones.size(); i++)
		{
			mSkinningMatrices[i] = physx::PxMat44(physx::PxIdentity);
			mSkinningMatricesWorld[i] = mBones[i].bindWorldPose;
		}
	}
}

// -------------------------------------------------------------------
void SkeletalAnim::setBoneCollision(uint32_t boneNr, int option)
{
	if (mBones[boneNr].boneOption != option)
	{
		if (mBones[boneNr].boneOption == HACD::BO_COLLAPSE || option == HACD::BO_COLLAPSE)
		{
			// PH: Follow up the hierarchy until something is not set to collapse and mark all dirty
			int current = mBones[boneNr].parent;
			while (current != -1)
			{
				mBones[(uint32_t)current].dirtyParams = true;
				if (mBones[(uint32_t)current].boneOption != HACD::BO_COLLAPSE)
				{
					break;
				}

				if (mBones[(uint32_t)current].parent == current)
				{
					break;
				}

				current = mBones[(uint32_t)current].parent;
			}
		}
		mBones[boneNr].dirtyParams = true;

		// Find all children that collapse into this bone and mark them dirty
		for (uint32_t i = 0; i < mBones.size(); i++)
		{
			// See whether boneNr is one of its parents
			bool found = false;
			uint32_t current = i;
			while (current != (uint32_t)-1 && !found)
			{
				if (current == boneNr)
				{
					found = true;
				}

				if (mBones[current].boneOption != HACD::BO_COLLAPSE)
				{
					break;
				}

				if ((int)current == mBones[current].parent)
				{
					break;
				}

				current = (uint32_t)mBones[current].parent;
			}
			if (found)
			{
				mBones[i].dirtyParams = true;
			}
		}
	}

	mBones[boneNr].boneOption = option;
}

// -------------------------------------------------------------------
void SkeletalAnim::setAnimPoseRec(int animNr, int boneNr, float time, bool lockBoneTranslation)
{
	SkeletalBone& b = mBones[(uint32_t)boneNr];

	{
		physx::PxTransform keyPose;
		physx::PxVec3 keyScale;

		// query the first frame instead of the current one if you want to lock this bone
		float myTime = (lockBoneTranslation && b.isRootLock) ? 0.0f : time;
		interpolateBonePose(animNr, boneNr, myTime, keyPose, keyScale);

		// todo: consider scale
		physx::PxTransform combinedPose;
		combinedPose.p = b.pose.p + keyPose.p;
		combinedPose.q = b.pose.q * keyPose.q;

		if (b.parent < 0)
		{
			b.currentWorldPose = combinedPose;
		}
		else
		{
			b.currentWorldPose = mBones[(uint32_t)b.parent].currentWorldPose * combinedPose;
		}
	}

	const int* children = mParent == NULL ? &mChildren[0] : &mParent->mChildren[0];
	for (int i = b.firstChild; i < b.firstChild + b.numChildren; i++)
	{
		setAnimPoseRec(animNr, children[i], time, lockBoneTranslation);
	}
}

// -------------------------------------------------------------------
bool SkeletalAnim::loadFromMeshImport(mimp::MeshSystemContainer* msc, std::string& error, bool onlyAddAnimation)
{
	bool ret = false;

	if (!onlyAddAnimation)
	{
		mBones.clear();
		mSkinningMatrices.clear();
		mSkinningMatricesWorld.clear();

		for (unsigned int i = 0; i < mAnimations.size(); i++)
		{
			SkeletalAnimation* a = mAnimations[i];
			delete a;
		}
		mAnimations.clear();
	}

	bool addAnimation = true;

	if (msc)
	{
		mimp::MeshSystem* ms = mimp::gMeshImport->getMeshSystem(msc);
		if (onlyAddAnimation && ms->mSkeletonCount > 0)
		{
			std::vector<int> overwriteBindPose(ms->mSkeletons[0]->mBoneCount, -1);
			// figure out how those bones map to each other.
			int numNotEqual = 0;
			int numNonZeroMatches = 0;
			for (int i = 0; i < ms->mSkeletons[0]->mBoneCount; i++)
			{
				mimp::MeshBone& bone = ms->mSkeletons[0]->mBones[i];
				for (size_t j = 0; j < mBones.size(); j++)
				{
					if (mBones[j].name.compare(ms->mSkeletons[0]->mBones[i].mName) == 0)
					{
						// found one, but let's see if the bind pose also matches more or less
						physx::PxTransform inputPose;
						inputPose.p = *(physx::PxVec3*)bone.mPosition;
						inputPose.q = *(physx::PxQuat*)bone.mOrientation;


						physx::PxTransform pose(inputPose);

						physx::PxTransform poseOld(mBones[j].pose);

						bool equal = (pose.p - poseOld.p).magnitude() <= ((0.5f * pose.p + 0.5f * poseOld.p).magnitude() * 0.01f);

						if (equal && !pose.p.isZero())
						{
							numNonZeroMatches++;
						}

						if (!equal && (inputPose.p.isZero() || numNonZeroMatches > 0))
						{
							// identity, skip the new bone's bind pose
							continue;
						}

						if (!equal)
						{
							char buf[128];
							physx::shdfnd::snprintf(buf, 128, "Bone %d (%s) does not match bind pose\n", (int)i, ms->mSkeletons[0]->mBones[i].mName);
							error.append(buf);

							numNotEqual++;
						}

						overwriteBindPose[(uint32_t)i] = (int)j;
						break;
					}
				}
			}

			if (numNotEqual > 0)
			{
				error = std::string("Failed to load animation:\n") + error;
				addAnimation = false;
			}
			else
			{
				// reset all bind poses exactly, now that we know they match pretty well
				for (uint32_t i = 0; i < (uint32_t)ms->mSkeletons[0]->mBoneCount; i++)
				{
					mimp::MeshBone& bone = ms->mSkeletons[0]->mBones[i];

					if (overwriteBindPose[i] != -1)
					{
						physx::PxTransform inputPose;
						inputPose.p = *(physx::PxVec3*)bone.mPosition;
						inputPose.q = *(physx::PxQuat*)bone.mOrientation;
						physx::PxTransform pose(inputPose);

						mBones[(uint32_t)overwriteBindPose[i]].pose = pose;
					}
				}
			}
		}
		else if (ms->mSkeletonCount)
		{
			mimp::MeshSkeleton* sk = ms->mSkeletons[0];

			for (int i = 0; i < sk->mBoneCount; i++)
			{
				mimp::MeshBone& b = sk->mBones[i];
				SkeletalBone sb;
				sb.clear();
				sb.name = b.mName;
				sb.id   = i;
				sb.parent = b.mParentIndex;
				sb.firstChild = 0;
				sb.numChildren = 0;
				sb.isRootLock = b.mParentIndex == 0; // lock the second bone in the hierarchy, first one is the scene root, not the anim root (for fbx files)

				PxQuatFromArray(sb.pose.q, b.mOrientation);
				PxVec3FromArray(sb.pose.p, b.mPosition);
				PxVec3FromArray(sb.scale, b.mScale);

				for (uint32_t bi = 0; bi < mBones.size(); bi++)
				{
					if (mBones[bi].name == b.mName)
					{
						if (error.empty())
						{
							error = "Duplicated Bone Names, rename one:\n";
						}

						error.append(b.mName);
						error.append("\n");
					}
				}

				mBones.push_back(sb);
			}

			ret = true; // allow loading a skeleton without animation
		}

		if (ms->mAnimationCount && addAnimation)
		{
			for (unsigned int i = 0; i < ms->mAnimationCount; i++)
			{
				mimp::MeshAnimation* animation = ms->mAnimations[i];
				SkeletalAnimation* anim = new SkeletalAnimation;
				anim->clear();
				if (ms->mAnimationCount == 1 && ms->mAssetName != NULL)
				{
					const char* lastDir = std::max(strrchr(ms->mAssetName, '/'), strrchr(ms->mAssetName, '\\'));
					anim->name = lastDir != NULL ? lastDir + 1 : ms->mAssetName;
				}
				else
				{
					anim->name = animation->mName;
				}

				size_t numBones = mBones.size();
				anim->mBoneTracks.resize(numBones);
				for (uint32_t j = 0; j < numBones; j++)
				{
					anim->mBoneTracks[j].clear();
				}

				for (uint32_t j = 0; j < (uint32_t)animation->mTrackCount; j++)
				{
					mimp::MeshAnimTrack* track = animation->mTracks[j];
					std::string boneName = track->mName;
					int boneNr = findBone(boneName);
					if (boneNr >= 0 && boneNr < (int32_t)numBones)
					{
						anim->mBoneTracks[(uint32_t)boneNr].firstFrame = (int)mKeyFrames.size();
						anim->mBoneTracks[(uint32_t)boneNr].numFrames  = track->mFrameCount;

						physx::PxTransform parent = mBones[(uint32_t)boneNr].pose;

						float ftime = 0;

						for (uint32_t k = 0; k < (uint32_t)track->mFrameCount; k++)
						{
							mimp::MeshAnimPose& pose = track->mPose[k];
							BoneKeyFrame frame;
							frame.clear();

							physx::PxTransform mat;
							PxQuatFromArray(mat.q, pose.mQuat);
							PxVec3FromArray(mat.p, pose.mPos);

							frame.time = ftime;
							PxVec3FromArray(frame.scale, pose.mScale);

							frame.relPose.p = mat.p - parent.p;
							frame.relPose.q = parent.q.getConjugate() * mat.q;

							mKeyFrames.push_back(frame);

							// eazymesh samples at 60 Hz, not 1s
							ftime += track->mDtime / 200.f;
						}
					}
					else if (!onlyAddAnimation)
					{
						// if onlyAddAnimation is set, the bone count does not have to match up, additional bones are just ignored
						PX_ASSERT(0);
					}

				}

				mAnimations.push_back(anim);
			}

			ret = true;
		}

		physx::PxTransform matId(physx::PxIdentity);
		mSkinningMatrices.resize((uint32_t)mBones.size(), matId);
		mSkinningMatricesWorld.resize((uint32_t)mBones.size(), matId);
		init(!onlyAddAnimation);
	}

	return ret;
}

// -------------------------------------------------------------------
bool SkeletalAnim::saveToMeshImport(mimp::MeshSystemContainer* msc)
{
#if PX_WINDOWS_FAMILY == 0
	PX_UNUSED(msc);
	return false;
#else

	if (msc == NULL)
	{
		return false;
	}

	mimp::MeshSystem* ms = mimp::gMeshImport->getMeshSystem(msc);
	if (ms == NULL)
	{
		return false;
	}

	ms->mSkeletonCount = 1;
	ms->mSkeletons = (mimp::MeshSkeleton**)::malloc(sizeof(mimp::MeshSkeleton*));
	ms->mSkeletons[0] = new mimp::MeshSkeleton;

	ms->mSkeletons[0]->mBoneCount = (int)mBones.size();
	ms->mSkeletons[0]->mBones = (mimp::MeshBone*)::malloc(sizeof(mimp::MeshBone) * mBones.size());
	for (size_t i = 0; i < mBones.size(); i++)
	{
		mimp::MeshBone& bone = ms->mSkeletons[0]->mBones[i];

		size_t nameLen = mBones[i].name.length() + 1;
		bone.mName = (char*)::malloc(sizeof(char) * nameLen);
		strcpy_s((char*)bone.mName, nameLen, mBones[i].name.c_str());

		(physx::PxQuat&)bone.mOrientation = mBones[i].pose.q;
		
		bone.mParentIndex = mBones[i].parent;

		(physx::PxVec3&)bone.mPosition = mBones[i].pose.p;

		(physx::PxVec3&)bone.mScale = mBones[i].scale;
	}

	ms->mAnimationCount = (unsigned int)mAnimations.size();
	ms->mAnimations = (mimp::MeshAnimation**)::malloc(sizeof(mimp::MeshAnimation*) * mAnimations.size());
	for (unsigned int a = 0; a < ms->mAnimationCount; a++)
	{
		ms->mAnimations[a] = new mimp::MeshAnimation;

		PX_ASSERT(mAnimations[a] != NULL);
		size_t nameLen = mAnimations[a]->name.length() + 1;
		ms->mAnimations[a]->mName = (char*)::malloc(sizeof(char) * nameLen);
		strcpy_s((char*)ms->mAnimations[a]->mName, nameLen, mAnimations[a]->name.c_str());

		unsigned int trackCount = 0;
		for (size_t i = 0; i < mBones.size(); i++)
		{
			trackCount += mAnimations[a]->mBoneTracks[i].numFrames > 0 ? 1 : 0;
		}

		ms->mAnimations[a]->mTrackCount = (int32_t)trackCount;
		ms->mAnimations[a]->mTracks = (mimp::MeshAnimTrack**)::malloc(sizeof(mimp::MeshAnimTrack*) * trackCount);
		ms->mAnimations[a]->mDuration = 0.0f;
		ms->mAnimations[a]->mDtime = 0.0f;

		unsigned int curTrack = 0;
		for (size_t t = 0; t < mBones.size(); t++)
		{
			if (mAnimations[a]->mBoneTracks[t].numFrames <= 0)
			{
				continue;
			}

			mimp::MeshAnimTrack* track = ms->mAnimations[a]->mTracks[curTrack++] = new mimp::MeshAnimTrack;

			track->mName = ms->mSkeletons[0]->mBones[t].mName; // just use the same name as the bone array already does
			const unsigned int firstFrame = (uint32_t)mAnimations[a]->mBoneTracks[t].firstFrame;
			track->mFrameCount = mAnimations[a]->mBoneTracks[t].numFrames;
			ms->mAnimations[a]->mFrameCount = std::max(ms->mAnimations[a]->mFrameCount, track->mFrameCount);

			track->mPose = (mimp::MeshAnimPose*)::malloc(sizeof(mimp::MeshAnimPose) * track->mFrameCount);

			track->mDuration = 0.0f;

			for (int f  = 0; f < track->mFrameCount; f++)
			{
				mimp::MeshAnimPose& pose = track->mPose[f];
				BoneKeyFrame& frame = mKeyFrames[firstFrame + f];

				physx::PxTransform mat;

				mat.q = frame.relPose.q * mBones[t].pose.q;
				mat.p = frame.relPose.p + mBones[t].pose.p;

				(physx::PxVec3&)pose.mScale = frame.scale;
				(physx::PxVec3&)pose.mPos = mat.p;

				(physx::PxQuat&)pose.mQuat = mat.q;

				track->mDuration = std::max(track->mDuration, frame.time);
			}

			track->mDtime = track->mDuration / (float)track->mFrameCount * 200.0f;

			ms->mAnimations[a]->mDuration = std::max(ms->mAnimations[a]->mDuration, track->mDuration);
			ms->mAnimations[a]->mDtime = std::max(ms->mAnimations[a]->mDtime, track->mDtime);
		}
	}

	return true;
#endif
}

// -------------------------------------------------------------------
bool SkeletalAnim::initFrom(nvidia::apex::RenderMeshAssetAuthoring& mesh)
{
	PX_ASSERT(mesh.getPartCount() == 1);

	uint32_t numBones = 0;
	for (uint32_t submeshIndex = 0; submeshIndex < mesh.getSubmeshCount(); submeshIndex++)
	{
		const nvidia::VertexBuffer& vb = mesh.getSubmesh(submeshIndex).getVertexBuffer();
		const nvidia::VertexFormat& vf = vb.getFormat();
		uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(nvidia::apex::RenderVertexSemantic::BONE_INDEX));

		nvidia::apex::RenderDataFormat::Enum format;
		const uint16_t* boneIndices = (const uint16_t*)vb.getBufferAndFormat(format, bufferIndex);

		unsigned int numBonesPerVertex = 0;
		switch (format)
		{
		case nvidia::apex::RenderDataFormat::USHORT1:
			numBonesPerVertex = 1;
			break;
		case nvidia::apex::RenderDataFormat::USHORT2:
			numBonesPerVertex = 2;
			break;
		case nvidia::apex::RenderDataFormat::USHORT3:
			numBonesPerVertex = 3;
			break;
		case nvidia::apex::RenderDataFormat::USHORT4:
			numBonesPerVertex = 4;
			break;
		default:
			break;
		}

		if (boneIndices == NULL || numBonesPerVertex == 0)
		{
			return false;
		}

		const unsigned int numElements = numBonesPerVertex * vb.getVertexCount();

		for (unsigned int i = 0; i < numElements; i++)
		{
			numBones = std::max(numBones, boneIndices[i] + 1u);
		}
	}

	SkeletalBone initBone;
	initBone.clear();
	mBones.resize(numBones, initBone);

	for (unsigned int i = 0; i < numBones; i++)
	{
		mBones[i].id = (int32_t)i;
	}

	mSkinningMatrices.resize(numBones);
	mSkinningMatricesWorld.resize(numBones);

	init(true);

	return numBones > 0;
}

// -------------------------------------------------------------------
bool SkeletalAnim::loadFromXML(const std::string& xmlFile, std::string& error)
{
	clear();

	physx::PsFileBuffer fb(xmlFile.c_str(), physx::PxFileBuf::OPEN_READ_ONLY);
	physx::PxInputDataFromPxFileBuf id(fb);

	if (!fb.isOpen())
	{
		return false;
	}

	physx::shdfnd::FastXml* fastXml = physx::shdfnd::createFastXml(this);
	fastXml->processXml(id);

	int errorLineNumber = -1;
	const char* xmlError = fastXml->getError(errorLineNumber);
	if (xmlError != NULL)
	{
		char temp[1024];
		physx::shdfnd::snprintf(temp, 1024, "Xml parse error in %s on line %d:\n\n%s", xmlFile.c_str(), errorLineNumber, xmlError);
		error = temp;
		return false;
	}

	fastXml->release();

	physx::PxTransform matId(physx::PxIdentity);
	mSkinningMatrices.resize((uint32_t)mBones.size(), matId);
	mSkinningMatricesWorld.resize((uint32_t)mBones.size(), matId);

	init(true);
	return true;
}

// -------------------------------------------------------------------
bool SkeletalAnim::loadFromParent(const SkeletalAnim* parent)
{
	if (parent == NULL)
	{
		return false;
	}

	mParent = parent;

	mBones.resize(mParent->mBones.size());
	physx::PxTransform matId(physx::PxIdentity);
	mSkinningMatrices.resize((uint32_t)mBones.size(), matId);
	mSkinningMatricesWorld.resize((uint32_t)mBones.size(), matId);
	for (uint32_t i = 0; i < mBones.size(); i++)
	{
		mBones[i] = mParent->mBones[i];
	}

	return true;
}

// -------------------------------------------------------------------
bool SkeletalAnim::saveToXML(const std::string& xmlFile) const
{
#if PX_WINDOWS_FAMILY == 0
	PX_UNUSED(xmlFile);
	return false;
#else
	FILE* f = 0;
	if (::fopen_s(&f, xmlFile.c_str(), "w") != 0)
	{
		return false;
	}

	fprintf(f, "<skeleton>\n\n");

	fprintf(f, "  <bones>\n");
	for (uint32_t i = 0; i < mBones.size(); i++)
	{
		const SkeletalBone& bone = mBones[i];

		float angle;
		physx::PxVec3 axis;
		bone.pose.q.toRadiansAndUnitAxis(angle, axis);
		angle = bone.pose.q.getAngle();

		fprintf(f, "    <bone id = \"%i\" name = \"%s\">\n", bone.id, bone.name.c_str());
		fprintf(f, "      <position x=\"%f\" y=\"%f\" z=\"%f\" />\n", bone.pose.p.x, bone.pose.p.y, bone.pose.p.z);
		fprintf(f, "      <rotation angle=\"%f\">\n", angle);
		fprintf(f, "        <axis x=\"%f\" y=\"%f\" z=\"%f\" />\n", axis.x, axis.y, axis.z);
		fprintf(f, "      </rotation>\n");
		fprintf(f, "      <scale x=\"%f\" y=\"%f\" z=\"%f\" />\n", 1.0f, 1.0f, 1.0f);
//		dont' use bone.scale.x, bone.scale.y, bone.scale.z because the length is baked into the bones
		fprintf(f, "    </bone>\n");
	}
	fprintf(f, "  </bones>\n\n");

	fprintf(f, "  <bonehierarchy>\n");
	for (uint32_t i = 0; i < mBones.size(); i++)
	{
		const SkeletalBone& bone = mBones[i];
		if (bone.parent < 0)
		{
			continue;
		}
		fprintf(f, "    <boneparent bone=\"%s\" parent=\"%s\" />\n", bone.name.c_str(), mBones[(uint32_t)bone.parent].name.c_str());
	}
	fprintf(f, "  </bonehierarchy>\n\n");

	fprintf(f, "  <animations>\n");
	for (uint32_t i = 0; i < mAnimations.size(); i++)
	{
		const SkeletalAnimation* anim = mAnimations[i];

		fprintf(f, "    <animation name = \"%s\" length=\"%f\">\n", anim->name.c_str(), anim->maxTime);
		fprintf(f, "      <tracks>\n");

		for (uint32_t j = 0; j < anim->mBoneTracks.size(); j++)
		{
			const BoneTrack& track = anim->mBoneTracks[j];
			if (track.numFrames == 0)
			{
				continue;
			}

			fprintf(f, "        <track bone = \"%s\">\n", mBones[j].name.c_str());
			fprintf(f, "          <keyframes>\n");

			for (int k = track.firstFrame; k < track.firstFrame + track.numFrames; k++)
			{
				const BoneKeyFrame& frame = mKeyFrames[(uint32_t)k];
				float angle;
				physx::PxVec3 axis;
				frame.relPose.q.toRadiansAndUnitAxis(angle, axis);
				angle = frame.relPose.q.getAngle();

				fprintf(f, "            <keyframe time = \"%f\">\n", frame.time);
				fprintf(f, "              <translate x=\"%f\" y=\"%f\" z=\"%f\" />\n", frame.relPose.p.x, frame.relPose.p.y, frame.relPose.p.z);
				fprintf(f, "              <rotate angle=\"%f\">\n", angle);
				fprintf(f, "                <axis x=\"%f\" y=\"%f\" z=\"%f\" />\n", axis.x, axis.y, axis.z);
				fprintf(f, "              </rotate>\n");
				fprintf(f, "              <scale x=\"%f\" y=\"%f\" z=\"%f\" />\n", frame.scale.x, frame.scale.y, frame.scale.z);
				fprintf(f, "            </keyframe>\n");
			}
			fprintf(f, "          </keyframes>\n");
			fprintf(f, "        </track>\n");
		}
		fprintf(f, "      </tracks>\n");
		fprintf(f, "    </animation>\n");
	}
	fprintf(f, "  </animations>\n");
	fprintf(f, "</skeleton>\n\n");

	fclose(f);


	return true;
#endif
}

// -------------------------------------------------------------------
void SkeletalAnim::init(bool firstTime)
{
	if (firstTime)
	{
		setupConnectivity();

		// init bind poses
		physx::PxVec3 oneOneOne(1.0f, 1.0f, 1.0f);
		for (uint32_t i = 0; i < mBones.size(); i++)
		{
			if (mBones[i].parent < 0)
			{
				initBindPoses((int32_t)i, oneOneOne);
			}

			// collapse finger and toes
			if (
				mBones[i].name.find("finger") != std::string::npos ||
				mBones[i].name.find("Finger") != std::string::npos ||
				mBones[i].name.find("FINGER") != std::string::npos ||
				mBones[i].name.find("toe") != std::string::npos ||
				mBones[i].name.find("Toe") != std::string::npos ||
				mBones[i].name.find("TOE") != std::string::npos)
			{
				mBones[i].boneOption = 2; // this is collapse
			}
		}
	}

	PX_ASSERT(mBones.size() == mSkinningMatrices.size());
	PX_ASSERT(mBones.size() == mSkinningMatricesWorld.size());
	for (uint32_t i = 0; i < mBones.size(); i++)
	{
		SkeletalBone& b = mBones[i];
		b.invBindWorldPose = b.bindWorldPose.inverseRT();
		b.currentWorldPose = mBones[i].bindWorldPose;
		mSkinningMatrices[i] = physx::PxMat44(physx::PxIdentity);
		mSkinningMatricesWorld[i] = b.currentWorldPose;
	}

	// init time interval of animations
	for (uint32_t i = 0; i < mAnimations.size(); i++)
	{
		SkeletalAnimation* a = mAnimations[i];
		bool first = true;
		for (uint32_t j = 0; j < a->mBoneTracks.size(); j++)
		{
			BoneTrack& b = a->mBoneTracks[j];
			for (int k = b.firstFrame; k < b.firstFrame + b.numFrames; k++)
			{
				float time = mKeyFrames[(uint32_t)k].time;
				if (first)
				{
					a->minTime = time;
					a->maxTime = time;
					first = false;
				}
				else
				{
					if (time < a->minTime)
					{
						a->minTime = time;
					}
					if (time > a->maxTime)
					{
						a->maxTime = time;
					}
				}
			}
		}
	}
}

// -------------------------------------------------------------------
void SkeletalAnim::initBindPoses(int boneNr, const physx::PxVec3& scale)
{
	SkeletalBone& b = mBones[(uint32_t)boneNr];
	b.pose.p = b.pose.p.multiply(scale);

	physx::PxVec3 newScale = scale.multiply(b.scale);

	if (b.parent < 0)
	{
		b.bindWorldPose = b.pose;
	}
	else
	{
		b.bindWorldPose = mBones[(uint32_t)b.parent].bindWorldPose * b.pose;
	}

	for (int i = b.firstChild; i < b.firstChild + b.numChildren; i++)
	{
		initBindPoses(mChildren[(uint32_t)i], newScale);
	}
}

// -------------------------------------------------------------------
void SkeletalAnim::setupConnectivity()
{
	size_t i;
	size_t numBones = mBones.size();
	for (i = 0; i < numBones; i++)
	{
		SkeletalBone& b = mBones[i];
		if (b.parent >= 0)
		{
			mBones[(uint32_t)b.parent].numChildren++;
		}
	}
	int first = 0;
	for (i = 0; i < numBones; i++)
	{
		mBones[i].firstChild = first;
		first += mBones[i].numChildren;
	}
	mChildren.resize((uint32_t)first);
	for (i = 0; i < numBones; i++)
	{
		if (mBones[i].parent < 0)
		{
			continue;
		}
		SkeletalBone& p = mBones[(uint32_t)mBones[i].parent];
		mChildren[(uint32_t)p.firstChild++] = (int)i;
	}
	for (i = 0; i < numBones; i++)
	{
		mBones[i].firstChild -= mBones[i].numChildren;
	}
}

// -------------------------------------------------------------------
void SkeletalAnim::draw(nvidia::RenderDebugInterface* batcher)
{
	PX_ASSERT(batcher != NULL);
	if (batcher == NULL)
	{
		return;
	}

	using RENDER_DEBUG::DebugColors;
	const uint32_t colorWhite = RENDER_DEBUG_IFACE(batcher)->getDebugColor(DebugColors::White);
	const uint32_t colorBlack = RENDER_DEBUG_IFACE(batcher)->getDebugColor(DebugColors::Black);
	for (uint32_t i = 0; i < mBones.size(); i++)
	{
		SkeletalBone& bone = mBones[i];

		uint32_t color = bone.selected ? colorWhite : colorBlack;
		RENDER_DEBUG_IFACE(batcher)->setCurrentColor(color);

		if (bone.parent >= 0 /*&& mBones[bone.parent].parent >= 0*/)
		{
			SkeletalBone& parent = mBones[(uint32_t)bone.parent];

			RENDER_DEBUG_IFACE(batcher)->debugLine(bone.currentWorldPose.getPosition(), parent.currentWorldPose.getPosition());
		}
	}
}

// -------------------------------------------------------------------
void SkeletalAnim::copyFrom(const SkeletalAnim& anim)
{
	clear();

	mBones.resize(anim.mBones.size());
	for (uint32_t i = 0; i < anim.mBones.size(); i++)
	{
		mBones[i] = anim.mBones[i];
	}

	mSkinningMatrices.resize(anim.mSkinningMatrices.size());
	for (uint32_t i = 0; i < anim.mSkinningMatrices.size(); i++)
	{
		mSkinningMatrices[i] = anim.mSkinningMatrices[i];
	}

	mSkinningMatricesWorld.resize(anim.mSkinningMatricesWorld.size());
	for (uint32_t i = 0; i < anim.mSkinningMatricesWorld.size(); i++)
	{
		mSkinningMatricesWorld[i] = anim.mSkinningMatricesWorld[i];
	}

	mChildren.resize(anim.mChildren.size());
	for (uint32_t i = 0; i < anim.mChildren.size(); i++)
	{
		mChildren[i] = anim.mChildren[i];
	}

	for (uint32_t i = 0; i < anim.mAnimations.size(); i++)
	{
		SkeletalAnimation* a = anim.mAnimations[i];
		SkeletalAnimation* na = new SkeletalAnimation();
		na->minTime = a->minTime;
		na->maxTime = a->maxTime;
		na->name = a->name;
		na->mBoneTracks.resize(a->mBoneTracks.size());
		for (uint32_t j = 0; j < a->mBoneTracks.size(); j++)
		{
			na->mBoneTracks[j] = a->mBoneTracks[j];
		}
		mAnimations.push_back(na);
	}

	mKeyFrames.resize(anim.mKeyFrames.size());
	for (uint32_t i = 0; i < anim.mKeyFrames.size(); i++)
	{
		mKeyFrames[i] = anim.mKeyFrames[i];
	}
}


// -------------------------------------------------------------------
void SkeletalAnim::clearShapeCount(int boneIndex)
{
	if (boneIndex < 0)
	{
		for (uint32_t i = 0; i < mBones.size(); i++)
		{
			mBones[i].numShapes = 0;
		}
	}
	else
	{
		PX_ASSERT((uint32_t)boneIndex < mBones.size());
		mBones[(uint32_t)boneIndex].numShapes = 0;
	}
}

// -------------------------------------------------------------------
void SkeletalAnim::incShapeCount(int boneIndex)
{
	if (boneIndex >= 0 && (uint32_t)boneIndex < mBones.size())
	{
		mBones[(uint32_t)boneIndex].numShapes++;
	}
}

// -------------------------------------------------------------------
void SkeletalAnim::decShapeCount(int boneIndex)
{
	if (boneIndex >= 0 && (uint32_t)boneIndex < mBones.size())
	{
		PX_ASSERT(mBones[(uint32_t)boneIndex].numShapes > 0);
		mBones[(uint32_t)boneIndex].numShapes--;
	}
}
// -------------------------------------------------------------------
bool SkeletalAnim::processElement(const char* elementName, const char* /*elementData*/, const physx::shdfnd::FastXml::AttributePairs& attr, int /*lineno*/)
	
{
	static int activeBoneTrack = -1;
	static BoneKeyFrame* activeKeyFrame;
	static bool isAnimation = false;

	if (::strcmp(elementName, "skeleton") == 0)
	{
		// ok, a start
	}
	else if (::strcmp(elementName, "bones") == 0)
	{
		// the list of bones
	}
	else if (::strcmp(elementName, "bone") == 0)
	{
		PX_ASSERT(attr.getNbAttr() == 2);
		PX_ASSERT(::strcmp(attr.getKey(0), "id") == 0);
		PX_ASSERT(::strcmp(attr.getKey(1), "name") == 0);
		SkeletalBone bone;
		bone.clear();
		bone.id = atoi(attr.getValue(0));
		bone.name = attr.getValue(1);
		mBones.push_back(bone);
	}
	else if (::strcmp(elementName, "position") == 0)
	{
		PX_ASSERT(attr.getNbAttr() == 3);
		PX_ASSERT(::strcmp(attr.getKey(0), "x") == 0);
		PX_ASSERT(::strcmp(attr.getKey(1), "y") == 0);
		PX_ASSERT(::strcmp(attr.getKey(2), "z") == 0);
		physx::PxVec3 pos;
		pos.x = (float)atof(attr.getValue(0));
		pos.y = (float)atof(attr.getValue(1));
		pos.z = (float)atof(attr.getValue(2));
		mBones.back().pose.p = pos;
	}
	else if (::strcmp(elementName, "rotation") == 0)
	{
		PX_ASSERT(attr.getNbAttr() == 1);
		PX_ASSERT(::strcmp(attr.getKey(0), "angle") == 0);
		mBones.back().pose.q = physx::PxQuat((float)atof(attr.getValue(0)));
		isAnimation = false;
	}
	else if (::strcmp(elementName, "axis") == 0 && !isAnimation)
	{
		PX_ASSERT(attr.getNbAttr() == 3);
		PX_ASSERT(::strcmp(attr.getKey(0), "x") == 0);
		PX_ASSERT(::strcmp(attr.getKey(1), "y") == 0);
		PX_ASSERT(::strcmp(attr.getKey(2), "z") == 0);
		physx::PxVec3 axis;
		axis.x = (float)atof(attr.getValue(0));
		axis.y = (float)atof(attr.getValue(1));
		axis.z = (float)atof(attr.getValue(2));
		float angle = mBones.back().pose.q.getAngle();
		mBones.back().pose.q = physx::PxQuat(angle, axis);
	}
	else if (::strcmp(elementName, "scale") == 0)
	{
		PX_ASSERT(attr.getNbAttr() == 3);
		PX_ASSERT(::strcmp(attr.getKey(0), "x") == 0);
		PX_ASSERT(::strcmp(attr.getKey(1), "y") == 0);
		PX_ASSERT(::strcmp(attr.getKey(2), "z") == 0);
		physx::PxVec3 scale;
		scale.x = (float)atof(attr.getValue(0));
		scale.y = (float)atof(attr.getValue(1));
		scale.z = (float)atof(attr.getValue(2));
		mBones.back().scale = scale;
	}
	else if (::strcmp(elementName, "bonehierarchy") == 0)
	{
		// ok
	}
	else if (::strcmp(elementName, "boneparent") == 0)
	{
		PX_ASSERT(attr.getNbAttr() == 2);
		PX_ASSERT(::strcmp(attr.getKey(0), "bone") == 0);
		PX_ASSERT(::strcmp(attr.getKey(1), "parent") == 0);
		int child = findBone(attr.getValue(0));
		int parent = findBone(attr.getValue(1));
		if (child >= 0 && child < (int)mBones.size() && parent >= 0 && parent < (int)mBones.size())
		{
			mBones[(uint32_t)child].parent = parent;
		}
	}
	else if (::strcmp(elementName, "animations") == 0)
	{
		// ok
	}
	else if (::strcmp(elementName, "animation") == 0)
	{
		PX_ASSERT(attr.getNbAttr() == 2);
		PX_ASSERT(::strcmp(attr.getKey(0), "name") == 0);

		SkeletalAnimation* anim = new SkeletalAnimation;
		anim->clear();
		anim->name = attr.getValue(0);
		anim->mBoneTracks.resize((uint32_t)mBones.size());

		mAnimations.push_back(anim);
	}
	else if (::strcmp(elementName, "tracks") == 0)
	{
		// ok
	}
	else if (::strcmp(elementName, "track") == 0)
	{
		PX_ASSERT(attr.getNbAttr() == 1);
		PX_ASSERT(::strcmp(attr.getKey(0), "bone") == 0);
		activeBoneTrack = findBone(attr.getValue(0));
		if (activeBoneTrack >= 0 && activeBoneTrack < (int)mBones.size())
		{
			mAnimations.back()->mBoneTracks[(uint32_t)activeBoneTrack].firstFrame = (int)(mKeyFrames.size());
			mAnimations.back()->mBoneTracks[(uint32_t)activeBoneTrack].numFrames = 0;
		}
	}
	else if (::strcmp(elementName, "keyframes") == 0)
	{
		// ok
	}
	else if (::strcmp(elementName, "keyframe") == 0)
	{
		PX_ASSERT(attr.getNbAttr() == 1);
		PX_ASSERT(::strcmp(attr.getKey(0), "time") == 0);

		mAnimations.back()->mBoneTracks[(uint32_t)activeBoneTrack].numFrames++;

		mKeyFrames.push_back(BoneKeyFrame());
		activeKeyFrame = &mKeyFrames.back();
		activeKeyFrame->clear();
		activeKeyFrame->time = (float)atof(attr.getValue(0));
	}
	else if (::strcmp(elementName, "translate") == 0)
	{
		PX_ASSERT(attr.getNbAttr() == 3);
		PX_ASSERT(::strcmp(attr.getKey(0), "x") == 0);
		PX_ASSERT(::strcmp(attr.getKey(1), "y") == 0);
		PX_ASSERT(::strcmp(attr.getKey(2), "z") == 0);
		activeKeyFrame->relPose.p.x = (float)atof(attr.getValue(0));
		activeKeyFrame->relPose.p.y = (float)atof(attr.getValue(1));
		activeKeyFrame->relPose.p.z = (float)atof(attr.getValue(2));
	}
	else if (::strcmp(elementName, "rotate") == 0)
	{
		PX_ASSERT(attr.getNbAttr() == 1);
		PX_ASSERT(::strcmp(attr.getKey(0), "angle") == 0);
		activeKeyFrame->relPose.q = physx::PxQuat((float)atof(attr.getValue(0)));
		isAnimation = true;
	}
	else if (::strcmp(elementName, "axis") == 0 && isAnimation)
	{
		PX_ASSERT(attr.getNbAttr() == 3);
		PX_ASSERT(::strcmp(attr.getKey(0), "x") == 0);
		PX_ASSERT(::strcmp(attr.getKey(1), "y") == 0);
		PX_ASSERT(::strcmp(attr.getKey(2), "z") == 0);
		physx::PxVec3 axis;
		axis.x = (float)atof(attr.getValue(0));
		axis.y = (float)atof(attr.getValue(1));
		axis.z = (float)atof(attr.getValue(2));
		axis.normalize();
		float angle = activeKeyFrame->relPose.q.getAngle();
		physx::PxQuat quat(angle, axis);
		activeKeyFrame->relPose.q = quat;
	}
	else
	{
		// always break here, at least in debug mode
		PX_ALWAYS_ASSERT();
	}

	return true;
}

} // namespace Samples
