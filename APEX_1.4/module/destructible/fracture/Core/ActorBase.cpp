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



#include "RTdef.h"
#if RT_COMPILE
#include "ActorBase.h"

#include <PxMat44.h>
#include "PxRigidBodyExt.h"
#include "PxScene.h"

#include "SimSceneBase.h"
#include "CompoundBase.h"

namespace nvidia
{
namespace fracture
{
namespace base
{

using namespace nvidia;

Actor::Actor(SimScene* scene):
	mScene(scene),
	mMinConvexSize(scene->mMinConvexSize),
	mDepthLimit(100),
	mDestroyIfAtDepthLimit(false)
{

}

Actor::~Actor()
{
	PxScene* pxScene = mScene->getScene();
	if(pxScene != NULL)
	{
		pxScene->lockWrite();
	}
	clear();
	if(pxScene != NULL)
	{
		pxScene->unlockWrite();
	}
	mScene->removeActor(this);
}

void Actor::clear()
{
	for (uint32_t i = 0; i < mCompounds.size(); i++) {
		PX_DELETE(mCompounds[i]);
	}
	mCompounds.clear();
}

void Actor::addCompound(Compound *c)
{
	mCompounds.pushBack(c);
	PxRigidDynamic *a = c->getPxActor();
    if (a) {
//		a->setContactReportFlags(Px_NOTIFY_ON_TOUCH_FORCE_THRESHOLD | Px_NOTIFY_ON_START_TOUCH_FORCE_THRESHOLD);
		a->setContactReportThreshold(mScene->mFractureForceThreshold);
    }
	c->mActor = this;
	++(mScene->mSceneVersion);
}

void Actor::removeCompound(Compound *c)
{
	uint32_t num = 0;
	for (uint32_t i = 0; i < mCompounds.size(); i++) {
		if (mCompounds[i] != c) {
			mCompounds[num] = mCompounds[i];
			num++;
		}
	}
	if (mScene->mPickActor == c->getPxActor())
		mScene->mPickActor = NULL;

	c->clear();
	//delCompoundList.push_back(c);
	//delete c;
	mScene->delCompoundList.pushBack(c);
	mCompounds.resize(num);
	++mScene->mSceneVersion;
}

void Actor::preSim(float dt)
{
	uint32_t num = 0;
	for (uint32_t i = 0; i < (uint32_t)mCompounds.size(); i++) {
		mCompounds[i]->step(dt);
		if (mCompounds[i]->getLifeFrames() == 0) {
			mCompounds[i]->clear();
			//delCompoundList.push_back(mCompounds[i]);
			//delete mCompounds[i];
			mScene->delCompoundList.pushBack(mCompounds[i]);
		}
		else {
			mCompounds[num] = mCompounds[i];
			num++;
		}
	}
	mCompounds.resize(num);
}

void Actor::postSim(float /*dt*/)
{
}

bool Actor::rayCast(const PxVec3 &orig, const PxVec3 &dir, float &dist, int &compoundNr, int &convexNr, PxVec3 &normal) const
{
	dist = PX_MAX_F32;
	compoundNr = -1;
	convexNr = -1;

	for (uint32_t i = 0; i < mCompounds.size(); i++) {
		float d;
		int cNr;
		PxVec3 n;
		if (mCompounds[i]->rayCast(orig, dir, d, cNr, n)) {
			if (d < dist) {
				dist = d;
				compoundNr = (int)i;
				convexNr = cNr;
				normal = n;
			}
		}
	}
	return compoundNr >= 0;
}

bool Actor::patternFracture(const PxVec3 &orig, const PxVec3 &dir, const PxMat33 patternTransform, float impactRadius, float radialImpulse, float directionalImpulse)
{
	float dist;
	float objectSize = 0.0f;
	int actorNr;
	int compoundNr;
	int convexNr;
	PxVec3 normal; 

	// do global rayCast.
	if (!mScene->rayCast(orig, dir, dist, actorNr, compoundNr, convexNr, normal))
		return false;
	if (mScene->mActors[(uint32_t)actorNr] != this)
		return false;

	mScene->debugPoints.clear();
	nvidia::Array<Compound*> compounds;

	mScene->profileBegin("patternFracture");
	bool OK = mCompounds[(uint32_t)compoundNr]->patternFracture(orig + dir * dist, mMinConvexSize, 
		compounds, patternTransform, mScene->debugPoints, impactRadius, radialImpulse, normal * directionalImpulse);
	mScene->profileEnd("patternFracture");
	if (!OK)
		return false;

	if (compounds.empty())
		return false;

	if (mCompounds[(uint32_t)compoundNr]->getPxActor() == mScene->mPickActor)
		mScene->mPickActor = NULL;

	PxBounds3 bounds;
	mCompounds[(uint32_t)compoundNr]->getWorldBounds(bounds);
	objectSize = bounds.getDimensions().magnitude();

	//delCompoundList.push_back( mCompounds[compoundNr]);
	mScene->delCompoundList.pushBack( mCompounds[(uint32_t)compoundNr]);
	mCompounds[(uint32_t)compoundNr]->clear();
	//delete mCompounds[compoundNr];
	
	mCompounds[(uint32_t)compoundNr] = compounds[0];

	for (uint32_t i = 1; i < compounds.size(); i++)
		mCompounds.pushBack(compounds[i]);

	++mScene->mSceneVersion;

	
	// playShatterSound(objectSize);
	//if (fluidSim)
	//fluidSim->mistPS->seedDustParticles((PxVec3*)&debugPoints[0], debugPoints.size(), seedDustRadius, seedDustNumPerSite, dustMinLife, dustMaxLife, 0.0f, 1.0f);

	uint32_t numConvexes = 0;
	for (uint32_t i = 0; i < mCompounds.size(); i++)
		numConvexes += mCompounds[i]->getConvexes().size();

	//printf("\n------- pattern fracture------\n");
	//printf("i compounds, %i convexes after fracture\n", mCompounds.size(), numConvexes); 

	return true;
}

bool Actor::patternFracture(const PxVec3 &hitLocation, const PxVec3 &normal, const int &compoundNr, const PxMat33 patternTransform,  float impactRadius, float radialImpulse, float directionalImpulse)
{
	float objectSize = 0.0f;

	mScene->debugPoints.clear();
	nvidia::Array<Compound*> compounds;

	mScene->profileBegin("patternFracture");
	bool OK = mCompounds[(uint32_t)compoundNr]->patternFracture(hitLocation, mMinConvexSize, 
		compounds, patternTransform, mScene->debugPoints, impactRadius, radialImpulse, normal * directionalImpulse);
	mScene->profileEnd("patternFracture");
	if (!OK)
		return false;

	if (compounds.empty())
		return false;

	if (mCompounds[(uint32_t)compoundNr]->getPxActor() == mScene->mPickActor)
		mScene->mPickActor = NULL;

	PxBounds3 bounds;
	mCompounds[(uint32_t)compoundNr]->getWorldBounds(bounds);
	objectSize = bounds.getDimensions().magnitude();

	//delCompoundList.push_back( mCompounds[compoundNr]);
	mScene->delCompoundList.pushBack( mCompounds[(uint32_t)compoundNr]);
	mCompounds[(uint32_t)compoundNr]->clear();
	//delete mCompounds[compoundNr];
	
	mCompounds[(uint32_t)compoundNr] = compounds[0];

	for (uint32_t i = 1; i < compounds.size(); i++)
		mCompounds.pushBack(compounds[i]);

	++mScene->mSceneVersion;

	
	// playShatterSound(objectSize);
	//if (fluidSim)
	//fluidSim->mistPS->seedDustParticles((PxVec3*)&debugPoints[0], debugPoints.size(), seedDustRadius, seedDustNumPerSite, dustMinLife, dustMaxLife, 0.0f, 1.0f);

	uint32_t numConvexes = 0;
	for (uint32_t i = 0; i < mCompounds.size(); i++)
		numConvexes += mCompounds[i]->getConvexes().size();

	//printf("\n------- pattern fracture------\n");
	//printf("i compounds, %i convexes after fracture\n", mCompounds.size(), numConvexes); 

	return true;
}

bool Actor::findCompound(const Compound* c, int& compoundNr)
{
	for(uint32_t i = 0; i < mCompounds.size(); i++)
	{
		if(mCompounds[i] == c)
		{
			compoundNr = (int32_t)i;
			return true;
		}
	}
	return false;
}

}
}
}
#endif