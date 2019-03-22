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

#include "SamplePreprocessor.h"
#include "KinematicPlatform.h"
#include "RendererMemoryMacros.h"
#include "PxScene.h"
#include "PxSceneLock.h"
#include "PxRigidDynamic.h"
#include "PxTkMatrixUtils.h"

PlatformState::PlatformState() :
	mCurrentTime		(0.0f),
	mCurrentRotationTime(0.0f),
	mFlip				(false)
{
	mPrevPose	= PxTransform(PxIdentity);
}

KinematicPlatform::KinematicPlatform() :
	mNbPts			(0),
	mPts			(NULL),
	mActor			(NULL),
	mBoxObstacle	(NULL),
	mObstacle		(INVALID_OBSTACLE_HANDLE),
	mTravelTime		(0.0f),
	mRotationSpeed	(0.0f),
	mMode			(LOOP_FLIP)
{
}

KinematicPlatform::~KinematicPlatform()
{
	DELETEARRAY(mPts);
}

void KinematicPlatform::release()
{
	delete this;
}

void KinematicPlatform::init(PxU32 nbPts, const PxVec3* pts, const PxTransform& globalPose, const PxQuat& localRot, PxRigidDynamic* actor, PxReal travelTime, PxReal rotationSpeed, LoopMode mode)
{
	DELETEARRAY(mPts);
	mNbPts	= nbPts;
	mPts	= SAMPLE_NEW(PxVec3Alloc)[nbPts];
	PxVec3* dst = mPts;
	for(PxU32 i=0;i<nbPts;i++)
		dst[i] = globalPose.transform(pts[i]);

	mLocalRot		= localRot;
	mActor			= actor;
	mTravelTime		= travelTime;
	mRotationSpeed	= rotationSpeed;
	mMode			= mode;
	PX_ASSERT(travelTime>0.0f);
}

void KinematicPlatform::setBoxObstacle(ObstacleHandle handle, const PxBoxObstacle* boxObstacle)
{
	mObstacle		= handle;
	mBoxObstacle	= boxObstacle;
}

PxU32 KinematicPlatform::getNbSegments() const
{
	return mNbPts - 1;
}

PxReal KinematicPlatform::computeLength() const
{
	const PxU32 nbSegments = getNbSegments();

	float totalLength = 0.0f;
	for(PxU32 i=0;i<nbSegments;i++)
	{
		const PxU32 a = i % mNbPts;
		const PxU32 b = (i+1) % mNbPts;
		totalLength += (mPts[b] - mPts[a]).magnitude();
	}
	return totalLength;
}

bool KinematicPlatform::getPoint(PxVec3& p, PxU32 seg, PxReal t) const
{
	const PxU32 a = seg % mNbPts;
	const PxU32 b = (seg+1) % mNbPts;
	const PxVec3& p0 = mPts[a];
	const PxVec3& p1 = mPts[b];
	p = (1.0f - t) * p0 + t * p1;
	return true;
}

bool KinematicPlatform::getPoint(PxVec3& p, PxReal t) const
{
	// ### Not really optimized

	const PxReal totalLength = computeLength();
	const PxReal coeff = 1.0f / totalLength;

	const PxU32 nbSegments = getNbSegments();

	PxReal currentLength = 0.0f;
	for(PxU32 i=0;i<nbSegments;i++)
	{
		const PxU32 a = i % mNbPts;
		const PxU32 b = (i+1) % mNbPts;
		const PxReal length = coeff * (mPts[b] - mPts[a]).magnitude();

		if(t>=currentLength && t<=currentLength + length)
		{
			// Desired point is on current segment
			// currentLength maps to 0.0
			// currentLength+length maps to 1.0
			const PxReal nt = (t-currentLength)/(length);
			return getPoint(p, i, nt);
		}
		currentLength += length;
	}
	return false;
}

void KinematicPlatform::setT(PxF32 t)
{
	if(t<0.0f || t>1.0f)
	{
		PX_ASSERT(0);
		return;
	}

	const PxF32 curTime = mTravelTime*t;

	mPhysicsState.mCurrentTime = curTime;
	mRenderState.mCurrentTime = curTime;
}

void KinematicPlatform::updateState(PlatformState& state, PxObstacleContext* obstacleContext, PxReal dtime, bool updateActor) const
{
	state.mCurrentTime += dtime;
	state.mCurrentRotationTime += dtime;

	// Compute current position on the path
	PxReal t = state.mCurrentTime/mTravelTime;
	if(t>1.0f)
	{
		if(mMode==LOOP_FLIP)
		{
			state.mFlip = !state.mFlip;
			// Make it loop
			state.mCurrentTime = fmodf(state.mCurrentTime, mTravelTime);
			t = state.mCurrentTime/mTravelTime;
		}
		else
		{
			PX_ASSERT(mMode==LOOP_WRAP);
//			state.mCurrentTime = fmodf(state.mCurrentTime, mTravelTime);
			t = 1.0f - t;
			state.mCurrentTime = t * mTravelTime;
		}
	}

	PxVec3 currentPos;
	if(getPoint(currentPos, state.mFlip ? 1.0f - t : t))
	{
		const PxVec3 wp = currentPos;

		PxMat33 rotY;
		PxToolkit::setRotX(rotY, state.mCurrentRotationTime*mRotationSpeed);
		const PxQuat rotation(rotY);

		const PxTransform tr(wp, mLocalRot * rotation);

//PxVec3 delta = wp - state.mPrevPos;
//shdfnd::printFormatted("Kine: %f | %f | %f\n", delta.x, delta.y, delta.z);
state.mPrevPose = tr;
		if(updateActor)
		{
			PxSceneWriteLock scopedLock(*mActor->getScene());
			mActor->setKinematicTarget(tr);	// *
/*PxVec3 test = mActor->getGlobalPose().p;
test -= tr.p;
shdfnd::printFormatted("%f | %f | %f\n", test.x, test.y, test.z);*/
		}
		else if(obstacleContext && mBoxObstacle)
		{
			PxBoxObstacle localBox = *mBoxObstacle;
			localBox.mPos.x = wp.x;
			localBox.mPos.y = wp.y;
			localBox.mPos.z = wp.z;
			localBox.mRot = tr.q;
			bool status = obstacleContext->updateObstacle(mObstacle, localBox);
			PX_ASSERT(status);
			PX_UNUSED(status);
		}
	}
}

void KinematicPlatform::resync()
{
	mPhysicsState = mRenderState;
}

///////////////////////////////////////////////////////////////////////////////

KinematicPlatformManager::KinematicPlatformManager() :
	mElapsedPlatformTime(0.0f)
{
}

KinematicPlatformManager::~KinematicPlatformManager()
{
}

void KinematicPlatformManager::release()
{
	const size_t nbPlatforms = mPlatforms.size();
	for(PxU32 i=0;i<nbPlatforms;i++)
		mPlatforms[i]->release();
	mPlatforms.clear();
}

KinematicPlatform* KinematicPlatformManager::createPlatform(PxU32 nbPts, const PxVec3* pts, const PxTransform& pose, const PxQuat& localRot, PxRigidDynamic* actor, PxReal platformSpeed, PxReal rotationSpeed, LoopMode mode)
{
	KinematicPlatform* kine = SAMPLE_NEW(KinematicPlatform);
	kine->init(nbPts, pts, pose, localRot, actor, 1.0f, rotationSpeed, mode);
	mPlatforms.push_back(kine);

	const PxReal pathLength = kine->computeLength();
	kine->setTravelTime(pathLength / platformSpeed);
	return kine;
}

void KinematicPlatformManager::updatePhysicsPlatforms(float dtime)
{
	// PT: keep track of time from the point of view of physics platforms.
	// - if we call this each substep using fixed timesteps, it is never exactly in sync with the render time.
	// - if we drop substeps because of the "well of despair", it can seriously lag behind the render time.
	mElapsedPlatformTime += dtime;

	// PT: compute new positions for (physics) platforms, then 'setKinematicTarget' their physics actors to these positions.
	const size_t nbPlatforms = mPlatforms.size();
	for(PxU32 i=0;i<nbPlatforms;i++)
		mPlatforms[i]->updatePhysics(dtime);
}
