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
#include "PsHashMap.h"
#include "SimSceneBase.h"
#include <foundation/PxMat44.h>
#include "PxRigidBodyExt.h"
#include "PxPhysics.h"
#include "PxCooking.h"
#include "PxShape.h"

#include "ActorBase.h"
#include "CompoundBase.h"
#include "ConvexBase.h"
#include "CompoundCreatorBase.h"
#include "PolygonTriangulatorBase.h"

#include "RTdef.h"

#include "PhysXMacros.h"
#include "PxScene.h"
#include "PxD6Joint.h"

#define USE_CONVEX_RENDERER 1
#define NUM_NO_SOUND_FRAMES 1

#define REL_VEL_FRACTURE_THRESHOLD 5.0f
//#define REL_VEL_FRACTURE_THRESHOLD 0.1f

namespace physx
{
namespace fracture
{
namespace base
{

SimScene* SimScene::createSimScene(PxPhysics *pxPhysics, PxCooking *pxCooking, bool isGrbScene, float minConvexSize, PxMaterial* defaultMat, const char *resourcePath)
{
	SimScene* s = PX_NEW(SimScene)(pxPhysics,pxCooking, isGrbScene,minConvexSize,defaultMat,resourcePath);
	s->createSingletons();
	return s;
}

void SimScene::createSingletons()
{
	mCompoundCreator = PX_NEW(CompoundCreator)(this);
	mPolygonTriangulator = PX_NEW(PolygonTriangulator)(this);
	addActor(createActor()); // make default actor
}

Convex* SimScene::createConvex()
{
	return PX_NEW(Convex)(this);
}

Compound* SimScene::createCompound(const FracturePattern *pattern, const FracturePattern *secondaryPattern, PxReal contactOffset, PxReal restOffset)
{
	return PX_NEW(Compound)(this,contactOffset,restOffset);
}

Actor* SimScene::createActor()
{
	return PX_NEW(Actor)(this);
}

shdfnd::Array<Compound*> SimScene::getCompounds()
{
	return mActors[0]->getCompounds();
}

///*extern*/ std::vector<Compound*> delCompoundList;

// --------------------------------------------------------------------------------------------
SimScene::SimScene(PxPhysics *pxPhysics, PxCooking *pxCooking, bool isGrbScene, float minConvexSize, PxMaterial* defaultMat, const char *resourcePath)
{
	mPxPhysics = pxPhysics;		// used for cooking
	mPxCooking = pxCooking;
	mPxDefaultMaterial = defaultMat;
	mResourcePath = resourcePath;

	clear();
	mRenderBuffers.init();
	mSceneVersion = 1;
	mRenderBufferVersion = 0;
	mOptixBufferVersion = 0;

	//create3dTexture();

	mFractureForceThreshold = 500.0f;
	mContactImpactRadius = 0.5f;

	mNoFractureFrames = 0;
	mNoSoundFrames = 0;
	mFrameNr = 0;
	mDebugDraw = false;
//	mDebugDraw = true;	// foo

	mPickDepth = 0.0f;
	mPickActor = NULL;
	mPickJoint = NULL;
	mPickPos = mPickLocalPos = PxVec3(0.0f, 0.0f, 0.0f);

	mMinConvexSize = minConvexSize;
	mNumNoFractureFrames = 2;

	bumpTextureUVScale = 0.1f;
	roughnessScale = 0.2f;
	extraNoiseScale = 2.0f;

	particleBumpTextureUVScale = 0.2f;
	particleRoughnessScale = 0.8f;
	particleExtraNoiseScale = 2.0f;


	mPlaySounds = false;
	mRenderDebris = true;

	mCompoundCreator = NULL;
	mPolygonTriangulator = NULL;

	mAppNotify = NULL;
}

// --------------------------------------------------------------------------------------------
SimScene::~SimScene()
{
	clear();

	// Delete actors if not already deleted
	for(int i = ((int)mActors.size() - 1) ; i >= 0; i--)
	{
		PX_DELETE(mActors[i]);
	}

	PX_DELETE(mCompoundCreator);
	PX_DELETE(mPolygonTriangulator);

	mCompoundCreator = NULL;
	mPolygonTriangulator = NULL;
}

// --------------------------------------------------------------------------------------------
void SimScene::clear()
{
	for(int i = 0 ; i < (int)mActors.size(); i++)
	{
		mActors[i]->clear();
	}
	deleteCompounds();
	mPickActor = NULL;

	++mSceneVersion;
}

void SimScene::addActor(Actor* a)
{
	mActors.pushBack(a);
}

void SimScene::removeActor(Actor* a)
{
	for(int i = 0; i < (int)mActors.size(); i++)
	{
		if( a == mActors[i] )
		{
			mActors[i] = mActors[mActors.size()-1];
			mActors.popBack();
		}
	}
}

// --------------------------------------------------------------------------------------------
void SimScene::addCompound(Compound *c)
{
	mActors[0]->addCompound(c);
}

// --------------------------------------------------------------------------------------------
void SimScene::removeCompound(Compound *c)
{
	mActors[0]->removeCompound(c);
}

// --------------------------------------------------------------------------------------------
void SimScene::deleteCompounds()
{
	for (int i = 0; i < (int)delCompoundList.size(); i++) 
	{
		PX_DELETE(delCompoundList[i]);
	}
	delCompoundList.clear();
}

// --------------------------------------------------------------------------------------------
void SimScene::preSim(float dt)
{
	for(int i = 0 ; i < (int)mActors.size(); i++)
	{
		mActors[i]->preSim(dt);
	}


	// make sure we use the apex user notify... if the application
	// changes their custom one make sure we map to it.
	mScene->lockWrite();
	PxSimulationEventCallback* userNotify = mScene->getSimulationEventCallback();
	if (userNotify != this)
	{
		mAppNotify = userNotify;
		mScene->setSimulationEventCallback(this);
	}
	mScene->unlockWrite();
}

//float4* posVelG = 0;
//int numPosVelG = 0;
// --------------------------------------------------------------------------------------------
void SimScene::postSim(float dt)
{
	for(int i = 0 ; i < (int)mActors.size(); i++)
	{
		mActors[i]->postSim(dt);
	}

	mFrameNr++;
	mNoFractureFrames--;
	mNoSoundFrames--;
}


//-----------------------------------------------------------------------------
bool SimScene::pickStart(const PxVec3 &orig, const PxVec3 &dir)
{
	PX_ASSERT(mPickActor == NULL);

	float dist = 1000.f;

	PxRaycastBuffer buffer;
	if (!getScene()->raycast(orig, dir, dist, buffer, PxHitFlag::eDEFAULT, PxQueryFilterData(PxQueryFlag::eDYNAMIC)))
		return false;

	PxRigidDynamic* pickedActor = buffer.getAnyHit(0).actor->is<PxRigidDynamic>();

	dist = buffer.getAnyHit(0).distance;



	//mPickActor = mActors[actorNr]->mCompounds[compoundNr]->getPxActor();
	mPickPos = orig + dir * dist;

	mPickActor = PxGetPhysics().createRigidDynamic(PxTransform(mPickPos));
	mPickActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

	getScene()->addActor(*mPickActor);
	
	mPickLocalPos = PxVec3(0.f);

	mPickJoint = PxD6JointCreate(PxGetPhysics(), mPickActor, PxTransform(PxIdentity), pickedActor, PxTransform(pickedActor->getGlobalPose().transformInv(mPickPos)));
	mPickJoint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
	mPickJoint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);
	mPickJoint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);

	mPickDepth = dist;
	return true;
}

//-----------------------------------------------------------------------------
void SimScene::pickMove(const PxVec3 &orig, const PxVec3 &dir)
{
	if (mPickActor == NULL)
		return;

	mPickPos = orig + dir * mPickDepth;
	mPickActor->setKinematicTarget(PxTransform(mPickPos));
}

//-----------------------------------------------------------------------------
void SimScene::pickRelease()
{
	if (mPickJoint)
		mPickJoint->release();
	mPickJoint = NULL;
	if (mPickActor)
		mPickActor->release();
	mPickActor = NULL;
}

bool SimScene::findCompound(const Compound* c, int& actorNr, int& compoundNr)
{
	actorNr = -1;
	compoundNr = -1;
	for(int i = 0; i < (int)mActors.size(); i++)
	{
		if (mActors[i]->findCompound(c,compoundNr))
		{
			actorNr = i;
			return true;
		}
	}
	return false;
}

// --------------------------------------------------------------------------------------------
// CPU
void SimScene::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
{
	// Pass along to application's callback, if defined
	if (mAppNotify != NULL)
	{
		mAppNotify->onContact(pairHeader, pairs, nbPairs);
	}
}

shdfnd::Array<PxVec3>& SimScene::getDebugPoints() {
	return debugPoints;
}

bool SimScene::mapShapeToConvex(const PxShape& shape, Convex& convex)
{
	return mShapeMap.insert(&shape, &convex);
}

bool SimScene::unmapShape(const PxShape& shape)
{
	return mShapeMap.erase(&shape);
}

Convex* SimScene::findConvexForShape(const PxShape& shape)
{
	const physx::shdfnd::HashMap<const PxShape*, Convex*>::Entry* entry = mShapeMap.find(&shape);
	return entry != NULL ? entry->second : NULL;	// Since we don't expect *entry to be NULL, we shouldn't lose information here
}

}
}
}

#endif
