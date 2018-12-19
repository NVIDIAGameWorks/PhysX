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
#include "SimSceneBase.h"
#include <PxMat44.h>
#include "PxRigidBodyExt.h"
#include "PxPhysics.h"
#include "PxCooking.h"
#include "PxShape.h"
#include "PxScene.h"
#include "PxMaterial.h"

#include "ActorBase.h"
#include "CompoundBase.h"
#include "ConvexBase.h"
#include "CompoundCreatorBase.h"
#include "Delaunay2dBase.h"
#include "Delaunay3dBase.h"
#include "PolygonTriangulatorBase.h"
#include "IslandDetectorBase.h"
#include "MeshClipperBase.h"
#include "FracturePatternBase.h"

#include "RTdef.h"

#define USE_CONVEX_RENDERER 1
#define NUM_NO_SOUND_FRAMES 1

//#define REL_VEL_FRACTURE_THRESHOLD 5.0f
#define REL_VEL_FRACTURE_THRESHOLD 0.1f

namespace nvidia
{
namespace fracture
{
namespace base
{

using namespace physx;

SimScene* SimScene::createSimScene(PxPhysics *pxPhysics, PxCooking *pxCooking, PxScene *scene, float minConvexSize, PxMaterial* defaultMat, const char *resourcePath)
{
	SimScene* s = PX_NEW(SimScene)(pxPhysics,pxCooking,scene,minConvexSize,defaultMat,resourcePath);
	s->createSingletons();
	return s;
}

void SimScene::createSingletons()
{
	mCompoundCreator = PX_NEW(CompoundCreator)(this);
	mDelaunay2d = PX_NEW(Delaunay2d)(this);
	mDelaunay3d = PX_NEW(Delaunay3d)(this);
	mPolygonTriangulator = PX_NEW(PolygonTriangulator)(this);
	mIslandDetector = PX_NEW(IslandDetector)(this);
	mMeshClipper = PX_NEW(MeshClipper)(this);
	addActor(createActor()); // make default actor
}

Convex* SimScene::createConvex()
{
	return PX_NEW(Convex)(this);
}

Compound* SimScene::createCompound(const FracturePattern *pattern, const FracturePattern *secondaryPattern, float contactOffset, float restOffset)
{
	return PX_NEW(Compound)(this,pattern,secondaryPattern,contactOffset,restOffset);
}

FracturePattern* SimScene::createFracturePattern()
{
	return PX_NEW(FracturePattern)(this);
}

Actor* SimScene::createActor()
{
	return PX_NEW(Actor)(this);
}

nvidia::Array<Compound*> SimScene::getCompounds()
{
	return mActors[0]->getCompounds();
}

///*extern*/ std::vector<Compound*> delCompoundList;

// --------------------------------------------------------------------------------------------
SimScene::SimScene(PxPhysics *pxPhysics, PxCooking *pxCooking, PxScene *scene, float minConvexSize, PxMaterial* defaultMat, const char *resourcePath)
{
	mPxPhysics = pxPhysics;		// used for cooking
	mPxCooking = pxCooking;
	mScene = scene;
	mPxDefaultMaterial = defaultMat;
	if (mPxDefaultMaterial != NULL)
	{
		mPxDefaultMaterial->acquireReference();
	}
	else
	{
		mPxDefaultMaterial = pxPhysics->createMaterial(0.5f, 0.5f, 0.1f);
	}
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
	mDelaunay2d = NULL;
	mDelaunay3d = NULL;
	mPolygonTriangulator = NULL;
	mIslandDetector = NULL;

	mAppNotify = NULL;

	//SDLWrapper::init();
}

// --------------------------------------------------------------------------------------------
SimScene::~SimScene()
{
	clear();

	if (mPxDefaultMaterial)
	{
		mPxDefaultMaterial->release();
		mPxDefaultMaterial = NULL;
	}

	// Delete actors if not already deleted
	for(int i = ((int)mActors.size() - 1) ; i >= 0; i--)
	{
		PX_DELETE(mActors[(uint32_t)i]);
	}

	PX_DELETE(mCompoundCreator);
	PX_DELETE(mDelaunay2d);
	PX_DELETE(mDelaunay3d);
	PX_DELETE(mPolygonTriangulator);
	PX_DELETE(mIslandDetector);
	PX_DELETE(mMeshClipper);

	mCompoundCreator = NULL;
	mDelaunay2d = NULL;
	mDelaunay3d = NULL;
	mPolygonTriangulator = NULL;
	mIslandDetector = NULL;
	mMeshClipper = NULL;

	//SDLWrapper::done();
}

// --------------------------------------------------------------------------------------------
void SimScene::restoreUserCallbacks()
{
	if (mScene != NULL)
	{
		// restore user callbacks
		mScene->lockWrite();
		mScene->setSimulationEventCallback(mAppNotify);
		mScene->unlockWrite();
	}
}

// --------------------------------------------------------------------------------------------
void SimScene::clear()
{
	for(uint32_t i = 0 ; i < mActors.size(); i++)
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
	for(uint32_t i = 0; i < mActors.size(); i++)
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
	for (uint32_t i = 0; i < delCompoundList.size(); i++) 
	{
		PX_DELETE(delCompoundList[i]);
	}
	delCompoundList.clear();
}

// --------------------------------------------------------------------------------------------
void SimScene::preSim(float dt)
{
	const float pickStiffness = 2.0f;

	if (mPickActor != NULL) {
		PxVec3 pos = mPickActor->getGlobalPose().transform(mPickLocalPos);
		float m = mPickActor->getMass();

		PxRigidBodyExt::addForceAtPos(*mPickActor, (mPickPos - pos) * pickStiffness * m, pos, PxForceMode::eIMPULSE);
	}

	for(uint32_t i = 0 ; i < mActors.size(); i++)
	{
		mActors[i]->preSim(dt);
	}

	mFractureEvents.clear();

	// make sure we use the apex user notify... if the application
	// changes their custom one make sure we map to it.
	if (mScene != NULL)
	{
		mScene->lockWrite();
		PxSimulationEventCallback* userNotify = mScene->getSimulationEventCallback();
		if (userNotify != this)
		{
			mAppNotify = userNotify;
			mScene->setSimulationEventCallback(this);
		}
		mScene->unlockWrite();
	}
}

//float4* posVelG = 0;
//int numPosVelG = 0;
// --------------------------------------------------------------------------------------------
void SimScene::postSim(float dt)
{
//	Profiler *p = Profiler::getInstance();

	//processFractureEvents(fluidSim);
	for(uint32_t i = 0 ; i < mActors.size(); i++)
	{
		mActors[i]->postSim(dt);
	}
	//posVelG = &posVel[0];
	//numPosVelG = posVel.size();
//	mFractureEvents.clear();	// foo

	mFrameNr++;
	mNoFractureFrames--;
	mNoSoundFrames--;
}

// --------------------------------------------------------------------------------------------
bool SimScene::rayCast(const PxVec3 &orig, const PxVec3 &dir, float &dist, int &actorNr , int &compoundNr, int &convexNr, PxVec3 &normal) const
{
	dist = PX_MAX_F32;
	actorNr = -1;
	compoundNr = -1;
	convexNr = -1;

	for (int i = 0; i < (int)mActors.size(); i++)
	{
		float d;
		int comNr;
		int conNr;
		PxVec3 n;
		if (mActors[(uint32_t)i]->rayCast(orig,dir,d,comNr,conNr,n))
		{
			if( d < dist )
			{
				dist = d;
				actorNr = i;
				compoundNr = comNr;
				convexNr = conNr;
				normal = n;
			}
		}
	}

	return actorNr >= 0;
}

//-----------------------------------------------------------------------------
bool SimScene::pickStart(const PxVec3 &orig, const PxVec3 &dir)
{
	float dist;
	int actorNr, compoundNr, convexNr;
	PxVec3 normal;

	if (!rayCast(orig, dir, dist, actorNr, compoundNr, convexNr, normal))
		return false;

	mPickActor = mActors[(uint32_t)actorNr]->mCompounds[(uint32_t)compoundNr]->getPxActor();
	mPickPos = orig + dir * dist;
	mPickLocalPos = mPickActor->getGlobalPose().transformInv(mPickPos);
	mPickDepth = dist;
	return true;
}

//-----------------------------------------------------------------------------
void SimScene::pickMove(const PxVec3 &orig, const PxVec3 &dir)
{
	if (mPickActor == NULL)
		return;

	mPickPos = orig + dir * mPickDepth;
}

//-----------------------------------------------------------------------------
void SimScene::pickRelease()
{
	mPickActor = NULL;
}

// --------------------------------------------------------------------------------------------
bool SimScene::patternFracture(const PxVec3 &orig, const PxVec3 &dir, const PxMat33 patternTransform, float impactRadius, float radialImpulse, float directionalImpulse)
{
	float dist;
	//float objectSize = 0.0f;
	int actorNr;
	int compoundNr;
	int convexNr;
	PxVec3 normal; 

	if (!rayCast(orig, dir, dist, actorNr, compoundNr, convexNr, normal))
		return false;

	return mActors[(uint32_t)actorNr]->patternFracture(orig+dist*dir,normal,compoundNr,patternTransform,impactRadius,radialImpulse,directionalImpulse);
}

bool SimScene::findCompound(const Compound* c, int& actorNr, int& compoundNr)
{
	actorNr = -1;
	compoundNr = -1;
	for(uint32_t i = 0; i < mActors.size(); i++)
	{
		if (mActors[i]->findCompound(c,compoundNr))
		{
			actorNr = (int32_t)i;
			return true;
		}
	}
	return false;
}

// --------------------------------------------------------------------------------------------
void SimScene::processFractureEvents(bool& valid,bool* addFireDustP)
{
	valid = false;
	debugPoints.clear();
	int numPieces = 0;

	if (mFractureEvents.size() <= 0)
		return;

	nvidia::Array<Compound*> compounds;
	++mSceneVersion;

	float objectSize = 0.0f;

	// here the fracture pattern could be randomized
	float scale = 1.0f;
	PxMat33 fractureTransform;
	fractureTransform = PxMat33(PxIdentity);
	fractureTransform *= scale;

	bool addFireDust = false;

	for (uint32_t i = 0; i < mFractureEvents.size(); i++) {
		const FractureEvent &e = mFractureEvents[i];

		if (!e.compound->patternFracture(e.pos, mMinConvexSize, compounds, fractureTransform, debugPoints, mContactImpactRadius, 
			e.additionalRadialImpulse, -e.normal * e.additionalNormalImpulse))
			continue;

		if (e.additionalNormalImpulse > 0.0f || e.additionalRadialImpulse > 0.0f) {
			addFireDust = true;
			playSound("explosion", 1);
		}
		else {
//			playSound("explosion", 1);
		}

		if (compounds.empty())
			continue;

		PxBounds3 bounds;
		e.compound->getWorldBounds(bounds);
		float size = bounds.getDimensions().magnitude();
		if (size > objectSize)
			objectSize = size;

		for (uint32_t j = 0; j < compounds.size(); j++) {
			PxRigidDynamic *a = compounds[j]->getPxActor();
//			a->setContactReportFlags(Px_NOTIFY_ON_TOUCH_FORCE_THRESHOLD);
			a->setContactReportThreshold(mFractureForceThreshold);
		}

		//int compoundNr = 0;
		//while (compoundNr < (int)mCompounds.size() && mCompounds[compoundNr] != e.compound)
		//	compoundNr++;

		//int first = 0;
		//if (compoundNr < (int)mCompounds.size()) {
		//	mCompounds[compoundNr] = compounds[0];
		//	first++;
		//}

		int first = 0;
		int actorNr = 0;
		int compoundNr = 0;
		if (findCompound(e.compound,actorNr,compoundNr) )
		{
			mActors[(uint32_t)actorNr]->mCompounds[(uint32_t)compoundNr] = compounds[0];
			first++;
		}

		if (e.compound->getPxActor() == mPickActor)
			mPickActor = NULL;

		e.compound->clear();
		//delCompoundList.push_back(e.compound);
		delCompoundList.pushBack(e.compound);
		
		//delete e.compound;

		for (int j = first; j < (int)compounds.size(); j++)
			mActors[(uint32_t)actorNr]->mCompounds.pushBack(compounds[(uint32_t)j]);

		numPieces += compounds.size();
	}

	mFractureEvents.clear();

	if (objectSize > 0.0f) {
		// playShatterSound(objectSize);
		mNoFractureFrames = mNumNoFractureFrames;		
	}
	if(addFireDustP)
	{
		*addFireDustP = addFireDust;
	}
	valid = true;
	return;
}

void SimScene::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, uint32_t nbPairs)
{
	// Pass along to application's callback, if defined
	if (mAppNotify != NULL)
	{
		mAppNotify->onContact(pairHeader, pairs, nbPairs);
	}

	if (mNoFractureFrames > 0)
		return;

	for (int i = 0; i < (int)nbPairs; i++) {

		const PxContactPair &pair = pairs[i];
		uint32_t num = pair.contactCount;
		if (num == 0)
			continue;

		mContactPoints.resize(num);
		pairs[i].extractContacts(&mContactPoints[0], num);

		// pick first point (arbitrary)
		Convex* convexes[2];
		convexes[0] = findConvexForShape(*pair.shapes[0]);
		convexes[1] = findConvexForShape(*pair.shapes[1]);

		PxVec3 &pos = mContactPoints[0].position;
		PxVec3 &normal = mContactPoints[0].normal;

		PxVec3 relVel(0.0f, 0.0f, 0.0f);

		for (int j = 0; j < 2; j++) {
			if (convexes[j] == NULL)
				continue;
			PxRigidDynamic *a = convexes[j]->getParent()->getPxActor();
			PxVec3 v = PxRigidBodyExt::getVelocityAtPos(*a, pos);
			relVel += (j == 0) ? v : -v;
		}

		float normalImpulse = 0;
		float radialImpulse = 0;
		for (int j = 0; j < 2; j++) {
			if (convexes[j] != NULL) {
				normalImpulse = PxMax(normalImpulse, convexes[j]->getParent()->getAdditionalImpactNormalImpulse());
				radialImpulse = PxMax(radialImpulse, convexes[j]->getParent()->getAdditionalImpactRadialImpulse());
			}
		}

		for (int j = 0; j < 2; j++) {
			Convex *c = convexes[j];
			if (c == NULL)
				continue;
			if (relVel.magnitude() < REL_VEL_FRACTURE_THRESHOLD)
				continue;

			Compound *compound = c->getParent();

			FractureEvent e;
			e.init();
			e.compound = compound;
			e.pos = pos;
			e.normal = normal;
			e.withStatic = (convexes[1-j] == NULL);
			e.additionalNormalImpulse = normalImpulse;
			e.additionalRadialImpulse = radialImpulse;

			uint32_t k = 0;
			while (k < mFractureEvents.size() && mFractureEvents[k].compound != compound)
				k++;

			if (k < mFractureEvents.size()) {
				if (mFractureEvents[k].withStatic && !e.withStatic)	// prioritize dynamic collisions
					mFractureEvents[k] = e;
			}
			else 
				mFractureEvents.pushBack(e);
		}
	}
}

void SimScene::onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count)
{
	PX_UNUSED(bodyBuffer);
	PX_UNUSED(poseBuffer);
	PX_UNUSED(count);
}

void SimScene::onTrigger(PxTriggerPair* pairs, uint32_t count)
{
	// Pass along to application's callback, if defined
	if (mAppNotify != NULL)
	{
		mAppNotify->onTrigger(pairs, count);
	}
}

void SimScene::onWake(PxActor** actors, uint32_t count)
{
	// Pass along to application's callback, if defined
	if (mAppNotify != NULL)
	{
		mAppNotify->onWake(actors, count);
	}
}

void SimScene::onSleep(PxActor** actors, uint32_t count)
{
	// Pass along to application's callback, if defined
	if (mAppNotify != NULL)
	{
		mAppNotify->onSleep(actors, count);
	}
}

void SimScene::onConstraintBreak(physx::PxConstraintInfo* constraints, uint32_t count)
{
	// Pass along to application's callback, if defined
	if (mAppNotify != NULL)
	{
		mAppNotify->onConstraintBreak(constraints, count);
	}
}

nvidia::Array<PxVec3>& SimScene::getDebugPoints() {
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
	const shdfnd::HashMap<const PxShape*, Convex*>::Entry* entry = mShapeMap.find(&shape);
	return entry != NULL ? entry->second : NULL;	// Since we don't expect *entry to be NULL, we shouldn't lose information here
}

}
}
}

#endif
