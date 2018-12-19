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


#include "ScArticulationSim.h"
#include "ScArticulationCore.h"
#include "ScArticulationJointSim.h"
#include "ScArticulationJointCore.h"
#include "ScBodySim.h"
#include "ScScene.h"

#include "DyArticulation.h"
#include "PxsContext.h"
#include "CmSpatialVector.h"
#include "PsVecMath.h"
#include "PxsSimpleIslandManager.h"

using namespace physx;
using namespace physx::Dy;

Sc::ArticulationSim::ArticulationSim(ArticulationCore& core, Scene& scene, BodyCore& root) : 
	mLLArticulation(NULL),
	mScene(scene),
	mCore(core),
	mLinks				(PX_DEBUG_EXP("ScArticulationSim::links")),
	mBodies				(PX_DEBUG_EXP("ScArticulationSim::bodies")),
	mJoints				(PX_DEBUG_EXP("ScArticulationSim::joints")),
	mInternalLoads		(PX_DEBUG_EXP("ScArticulationSim::internalLoads")),
	mExternalLoads		(PX_DEBUG_EXP("ScArticulationSim::externalLoads")),
	mPose				(PX_DEBUG_EXP("ScArticulationSim::poses")),
	mMotionVelocity		(PX_DEBUG_EXP("ScArticulationSim::motion velocity")),
	mFsDataBytes		(PX_DEBUG_EXP("ScArticulationSim::fsData")),
	mScratchMemory		(PX_DEBUG_EXP("ScArticulationSim::scratchMemory")),					
	mUpdateSolverData(true)
{
	mLinks.reserve(16);
	mJoints.reserve(16);
	mBodies.reserve(16);

	mLLArticulation = mScene.createLLArticulation(this);

	mIslandNodeIndex = scene.getSimpleIslandManager()->addArticulation(this, mLLArticulation, false);

	if(!mLLArticulation)
	{
		Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "Articulation: could not allocate low-level resources.");
		return;
	}

	PX_ASSERT(root.getSim());

	addBody(*root.getSim(), NULL, NULL);

	

	mCore.setSim(this);

	mSolverData.core			= &core.getCore();
	mSolverData.internalLoads	= NULL;
	mSolverData.externalLoads	= NULL;
	mSolverData.fsData			= NULL;
	mSolverData.poses			= NULL;
	mSolverData.motionVelocity	= NULL;
	mSolverData.totalDataSize	= 0;
	mSolverData.solverDataSize	= 0;
	mSolverData.linkCount		= 0;
	mSolverData.scratchMemory	= NULL;
	mSolverData.scratchMemorySize = 0;
}


Sc::ArticulationSim::~ArticulationSim()
{
	if (!mLLArticulation)
		return;

	mScene.destroyLLArticulation(*mLLArticulation);

	mScene.getSimpleIslandManager()->removeNode(mIslandNodeIndex);

	mCore.setSim(NULL);
}

PxU32 Sc::ArticulationSim::findBodyIndex(BodySim& body) const
{
	for(PxU32 i=0; i<mBodies.size(); i++)
	{
		if(mBodies[i]==&body)
			return i;
	}
	PX_ASSERT(0);
	return 0x80000000;
}

void Sc::ArticulationSim::updateCached(Cm::BitMapPinned* shapeChangedMap)
{
	for(PxU32 i=0; i<mBodies.size(); i++)
		mBodies[i]->updateCached(shapeChangedMap);
}

void Sc::ArticulationSim::updateContactDistance(PxReal* contactDistance, const PxReal dt, Bp::BoundsArray& boundsArray)
{
	for (PxU32 i = 0; i<mBodies.size(); i++)
		mBodies[i]->updateContactDistance(contactDistance, dt, boundsArray);
}

ArticulationLinkHandle Sc::ArticulationSim::getLinkHandle(BodySim &body) const
{
	return reinterpret_cast<size_t>(mLLArticulation) | findBodyIndex(body);
}

void Sc::ArticulationSim::addBody(BodySim& body, 
								  BodySim* parent, 
								  ArticulationJointSim* joint)
{
	mBodies.pushBack(&body);
	mJoints.pushBack(joint);
	mAcceleration.pushBack(Cm::SpatialVector(PxVec3(0.f), PxVec3(0.f)));

	PxU32 index = mLinks.size();

	PX_ASSERT((((index==0) && (joint == 0)) && (parent == 0)) ||
			  (((index!=0) && joint) && (parent && (parent->getArticulation() == this))));

	ArticulationLink &link = mLinks.insert();
	link.body = &body.getLowLevelBody();
	link.bodyCore = &body.getBodyCore().getCore();
	link.children = 0;
	bool shouldSleep;
	bool currentlyAsleep;
	bool bodyReadyForSleep = body.checkSleepReadinessBesidesWakeCounter();
	PxReal wakeCounter = getCore().getWakeCounter();

	if(parent)
	{
		currentlyAsleep = !mBodies[0]->isActive();
		shouldSleep = currentlyAsleep && bodyReadyForSleep;

		PxU32 parentIndex = findBodyIndex(*parent);
		link.parent = parentIndex;
		link.pathToRoot = mLinks[parentIndex].pathToRoot | ArticulationBitField(1)<<index;
		link.inboundJoint = &joint->getCore().getCore();
		mLinks[parentIndex].children |= ArticulationBitField(1)<<index;
	}
	else
	{
		currentlyAsleep = (wakeCounter == 0.0f);
		shouldSleep = currentlyAsleep && bodyReadyForSleep;

		link.parent = DY_ARTICULATION_LINK_NONE;
		link.pathToRoot = 1;
		link.inboundJoint = NULL;
	}

	if (currentlyAsleep && (!shouldSleep))
	{
		for(PxU32 i=0; i < (mBodies.size() - 1); i++)
			mBodies[i]->internalWakeUpArticulationLink(wakeCounter);
	}

	body.setArticulation(this, wakeCounter, shouldSleep, index);

	mUpdateSolverData = true;

}


void Sc::ArticulationSim::removeBody(BodySim &body)
{
	PX_ASSERT(body.getArticulation() == this);
	PxU32 index = findBodyIndex(body);
	body.setArticulation(NULL, 0.0f, true, 0);

	ArticulationLink &link0 = mLinks[index];

	PX_ASSERT(link0.children == 0);
	PX_UNUSED(link0);

	// copy all the later links down by one
	for(PxU32 i=index+1;i<mLinks.size();i++)
	{
		mLinks[i-1] = mLinks[i];
		mBodies[i-1] = mBodies[i];
		mJoints[i-1] = mJoints[i];
		//setIslandHandle(*mBodies[i-1], i-1);
	}

	// adjust parent/child indices
	ArticulationBitField fixedIndices = (ArticulationBitField(1)<<index)-1;
	ArticulationBitField shiftIndices = ~(fixedIndices|(ArticulationBitField(1)<<index));

	for(PxU32 i=0;i<mLinks.size();i++)
	{
		ArticulationLink &link = mLinks[i];

		if(link.parent != DY_ARTICULATION_LINK_NONE && link.parent>index)
			link.pathToRoot = (link.pathToRoot&fixedIndices) | (link.pathToRoot&shiftIndices)>>1;
		link.children = (link.children&fixedIndices) | (link.children&shiftIndices)>>1;
	}

	mLinks.popBack();

	mUpdateSolverData = true;
}


void Sc::ArticulationSim::checkResize() const
{
	if(!mBodies.size())
		return;

	if(!mUpdateSolverData)
		return;

	if(mLinks.size()!=mSolverData.linkCount)
	{
		PxU32 linkCount = mLinks.size();

		mMotionVelocity.resize(linkCount, Cm::SpatialVector(PxVec3(0.0f), PxVec3(0.0f)));
		mPose.resize(linkCount, PxTransform(PxIdentity));
		mExternalLoads.resize(linkCount, Ps::aos::M33Identity());
		mInternalLoads.resize(linkCount, Ps::aos::M33Identity());

		PxU32 solverDataSize, totalSize, scratchSize;
		Articulation::getDataSizes(linkCount, solverDataSize, totalSize, scratchSize);

		PX_ASSERT(mFsDataBytes.size()!=totalSize);
		PX_ASSERT(!(totalSize&15) && !(solverDataSize&15));
		mFsDataBytes.resize(totalSize);

		mSolverData.motionVelocity			= mMotionVelocity.begin();
		mSolverData.externalLoads			= mExternalLoads.begin();
		mSolverData.internalLoads			= mInternalLoads.begin();
		mSolverData.poses					= mPose.begin();
		mSolverData.solverDataSize			= Ps::to16(solverDataSize);
		mSolverData.totalDataSize			= Ps::to16(totalSize);
		mSolverData.fsData					= reinterpret_cast<FsData *>(mFsDataBytes.begin());
		mSolverData.acceleration			= mAcceleration.begin();

		mScratchMemory.resize(scratchSize);
		mSolverData.scratchMemory			= mScratchMemory.begin();
		mSolverData.scratchMemorySize		= Ps::to16(scratchSize);
	}


	// something changed... e.g. a link deleted and one added - we need to change the warm start

	PxMemZero(mExternalLoads.begin(), sizeof(Ps::aos::Mat33V) * mExternalLoads.size());
	PxMemZero(mInternalLoads.begin(), sizeof(Ps::aos::Mat33V) * mExternalLoads.size());

	mSolverData.links			= mLinks.begin();
	mSolverData.linkCount		= Ps::to8(mLinks.size());

	mLLArticulation->setSolverDesc(mSolverData);

	mUpdateSolverData = false;
}

PxU32 Sc::ArticulationSim::getCCDLinks(BodySim** sims)
{
	PxU32 nbCCDBodies = 0;
	for (PxU32 a = 0; a < mBodies.size(); ++a)
	{
		if (mBodies[a]->getLowLevelBody().getCore().mFlags & PxRigidBodyFlag::eENABLE_CCD)
		{
			sims[nbCCDBodies++] = mBodies[a];
		}
	}
	return nbCCDBodies;
}

void Sc::ArticulationSim::sleepCheck(PxReal dt)
{
	if(!mBodies.size())
		return;

#if PX_CHECKED
	{
		PxReal maxTimer = 0.0f, minTimer = PX_MAX_F32;
		bool allActive = true, noneActive = true;
		for(PxU32 i=0;i<mLinks.size();i++)
		{
			PxReal timer = mBodies[i]->getBodyCore().getWakeCounter();
			maxTimer = PxMax(maxTimer, timer);
			minTimer = PxMin(minTimer, timer);
			bool active = mBodies[i]->isActive();
			allActive &= active;
			noneActive &= !active;
		}
		// either all links are asleep, or no links are asleep
		PX_ASSERT(maxTimer==0 || minTimer!=0);
		PX_ASSERT(allActive || noneActive);
	}

#endif

	if(!mBodies[0]->isActive())
		return;

	PxReal sleepThreshold = getCore().getCore().sleepThreshold;

	PxReal maxTimer = 0.0f, minTimer = PX_MAX_F32;

	for(PxU32 i=0;i<mLinks.size();i++)
	{
		PxReal timer = mBodies[i]->updateWakeCounter(dt, sleepThreshold, reinterpret_cast<Cm::SpatialVector&>(mMotionVelocity[i]));//enableStabilization);
		maxTimer = PxMax(maxTimer, timer);
		minTimer = PxMin(minTimer, timer);
	}

	mCore.setWakeCounterInternal(maxTimer);

	if(maxTimer != 0.0f)
	{
		if(minTimer == 0.0f)
		{
			// make sure nothing goes to sleep unless everything does
			for(PxU32 i=0;i<mLinks.size();i++)
				mBodies[i]->getBodyCore().setWakeCounterFromSim(PxMax(1e-6f, mBodies[i]->getBodyCore().getWakeCounter()));
		}
		return;
	}

	for(PxU32 i=0;i<mLinks.size();i++)
	{
		mBodies[i]->notifyReadyForSleeping();
		mBodies[i]->resetSleepFilter();
	}

	mScene.getSimpleIslandManager()->deactivateNode(mIslandNodeIndex);
}

bool Sc::ArticulationSim::isSleeping() const
{
	return (mBodies.size() > 0) ? (!mBodies[0]->isActive()) : true;
}

void Sc::ArticulationSim::internalWakeUp(PxReal wakeCounter)
{
	if(mCore.getWakeCounter() < wakeCounter)
	{
		mCore.setWakeCounterInternal(wakeCounter);
		for(PxU32 i=0;i<mLinks.size();i++)
			mBodies[i]->internalWakeUpArticulationLink(wakeCounter);
	}
}

void Sc::ArticulationSim::setActive(const bool b, const PxU32 infoFlag)
{
	for(PxU32 i=0;i<mBodies.size();i++)
	{
		if (i+1 < mBodies.size())
		{
			Ps::prefetchLine(mBodies[i+1],0);
			Ps::prefetchLine(mBodies[i+1],128);
		}
		mBodies[i]->setActive(b, infoFlag);
	}
}

void Sc::ArticulationSim::updateForces(PxReal dt, bool simUsesAdaptiveForce)
{
	PxU32 count = 0;
	for(PxU32 i=0;i<mBodies.size();i++)
	{
		if (i+1 < mBodies.size())
		{
			Ps::prefetchLine(mBodies[i+1],128);
			Ps::prefetchLine(mBodies[i+1],256);
		}
		mBodies[i]->updateForces(dt, NULL, NULL, count, &mAcceleration[i], simUsesAdaptiveForce);
	}
}

void Sc::ArticulationSim::saveLastCCDTransform()
{
	for(PxU32 i=0;i<mBodies.size();i++)
	{
		if (i+1 < mBodies.size())
		{
			Ps::prefetchLine(mBodies[i+1],128);
			Ps::prefetchLine(mBodies[i+1],256);
		}
		mBodies[i]->getLowLevelBody().saveLastCCDTransform();
	}
}


Sc::ArticulationDriveCache* Sc::ArticulationSim::createDriveCache(PxReal compliance,
																  PxU32 driveIterations) const
{
	checkResize();
	PxU32 solverDataSize, totalSize, scratchSize;
	Articulation::getDataSizes(mLinks.size(), solverDataSize, totalSize, scratchSize);

	// In principle we should only need solverDataSize here. But right now prepareFsData generates the auxiliary data 
	// for use in potential debugging, which takes up extra space. 
	FsData* data = reinterpret_cast<FsData*>(PX_ALLOC(totalSize,"Articulation Drive Cache"));
	PxvArticulationDriveCache::initialize(*data, Ps::to16(mLinks.size()), mLinks.begin(), compliance, driveIterations, mScratchMemory.begin(), mScratchMemory.size());
	return data;
}


void Sc::ArticulationSim::updateDriveCache(ArticulationDriveCache& cache,
										   PxReal compliance,
										   PxU32 driveIterations) const
{
	checkResize();
	PxvArticulationDriveCache::initialize(cache,  Ps::to16(mLinks.size()), mLinks.begin(), compliance, driveIterations, mScratchMemory.begin(), mScratchMemory.size());
}


void Sc::ArticulationSim::releaseDriveCache(Sc::ArticulationDriveCache& driveCache) const
{
	PX_FREE(&driveCache);
}


void Sc::ArticulationSim::applyImpulse(Sc::BodyCore& link,
									   const Sc::ArticulationDriveCache& driveCache,
									   const PxVec3& force,
									   const PxVec3& torque)
{
	Cm::SpatialVectorV v[DY_ARTICULATION_MAX_SIZE], z[DY_ARTICULATION_MAX_SIZE];
	PxMemZero(z, mLinks.size()*sizeof(Cm::SpatialVector));
	PxMemZero(v, mLinks.size()*sizeof(Cm::SpatialVector));

	PxU32 bodyIndex = findBodyIndex(*link.getSim());
	z[bodyIndex].linear = Ps::aos::V3LoadU(-force);
	z[bodyIndex].angular = Ps::aos::V3LoadU(-torque);

	PxvArticulationDriveCache::applyImpulses(driveCache, z, v);
	for(PxU32 i=0;i<mLinks.size();i++)
	{
		Sc::BodyCore& body = mBodies[i]->getBodyCore();
		PxVec3 lv, av;
		Ps::aos::V3StoreU(v[i].linear, lv);
		Ps::aos::V3StoreU(v[i].angular, av);

		body.setLinearVelocity(body.getLinearVelocity()+lv);
		body.setAngularVelocity(body.getAngularVelocity()+av);
	}
}

void Sc::ArticulationSim::computeImpulseResponse(Sc::BodyCore& link,
												  PxVec3& linearResponse, 
												  PxVec3& angularResponse,
												  const Sc::ArticulationDriveCache& driveCache,
												  const PxVec3& force,
												  const PxVec3& torque) const
{
	Cm::SpatialVectorV v;
	PxvArticulationDriveCache::getImpulseResponse(driveCache, findBodyIndex(*link.getSim()), Cm::SpatialVectorV(Ps::aos::V3LoadU(force), Ps::aos::V3LoadU(torque)), v);
	Ps::aos::V3StoreU(v.linear, linearResponse);
	Ps::aos::V3StoreU(v.angular, angularResponse);
}

void Sc::ArticulationSim::debugCheckWakeCounterOfLinks(PxReal wakeCounter) const
{
	PX_UNUSED(wakeCounter);

#ifdef _DEBUG
	// make sure the links are in sync with the articulation
	for(PxU32 i=0; i < mBodies.size(); i++)
	{
		PX_ASSERT(mBodies[i]->getBodyCore().getWakeCounter() == wakeCounter);
	}
#endif
}

void Sc::ArticulationSim::debugCheckSleepStateOfLinks(bool isSleeping) const
{
	PX_UNUSED(isSleeping);

#ifdef _DEBUG
	// make sure the links are in sync with the articulation
	for(PxU32 i=0; i < mBodies.size(); i++)
	{
		if (isSleeping)
		{
			PX_ASSERT(!mBodies[i]->isActive());
			PX_ASSERT(mBodies[i]->getBodyCore().getWakeCounter() == 0.0f);
			PX_ASSERT(mBodies[i]->checkSleepReadinessBesidesWakeCounter());
		}
		else
			PX_ASSERT(mBodies[i]->isActive());
	}
#endif
}
