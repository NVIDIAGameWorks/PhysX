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

#include "PxPhysicsAPI.h"
#include "extensions/PxExtensionsAPI.h"
#include "SceneCrab.h"
#include "Crab.h"
#include "CrabManager.h"
#include "SimScene.h"
#include "CompoundCreator.h"
#include <GL/glut.h>

#include "PxTkStream.h"
#include "PxTkFile.h"
#include "CmTask.h"
using namespace PxToolkit;


CrabManager::CrabManager() : mSceneCrab(NULL), mCrabs(NULL), mCompletionTask(mSync)
{
	mSync.set();
}

CrabManager::~CrabManager()
{
	for (PxU32 i = 0; i < mCrabs.size(); ++i)
	{
		mCrabs[i]->~Crab();
		PX_FREE(mCrabs[i]);
	}

	mCrabs.clear();

	for (PxU32 i = 0; i < mUpdateStateTask.size(); ++i)
	{
		mUpdateStateTask[i]->~CrabUpdateStateTask();
		PX_FREE(mUpdateStateTask[i]);
	}

	mUpdateStateTask.clear();
}

PxScene& CrabManager::getScene()
{
	return mSceneCrab->getScene();
}

PxPhysics& CrabManager::getPhysics()
{
	return mSceneCrab->getPhysics();
}

void CrabManager::setSceneCrab(SceneCrab* sceneCrab)
{
	mSceneCrab = sceneCrab; 
}

void CrabManager::setScene(PxScene* scene)
{
	for (PxU32 a = 0; a < mCrabs.size(); ++a)
	{
		mCrabs[a]->setScene(scene);
	}
}

SceneCrab* CrabManager::getSceneCrab()
{ 
	return mSceneCrab; 
}

Compound* CrabManager::createObject(const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega,
	bool particles, const ShaderMaterial &mat, bool useSecondaryPattern, ShaderShadow* shader)
{
	return mSceneCrab->createObject(pose, vel, omega, particles, mat, useSecondaryPattern, shader,1,1);
}

void CrabManager::initialize(const PxU32 nbCrabs)
{
	mCrabs.reserve(nbCrabs);
}

void CrabManager::createCrab(const PxVec3& crabPos, const physx::PxReal crabDepth, const PxReal scale, const PxReal legMass, const PxU32 numLegs)
{
	PxU32 idx = mCrabs.size();
	mCrabs.pushBack(PX_PLACEMENT_NEW(PX_ALLOC(sizeof(Crab), PX_DEBUG_EXP("Crabs")), Crab)(this, idx%UPDATE_FREQUENCY_RESET));
	mCrabs[idx]->create(crabPos, crabDepth, scale, legMass, numLegs);
	PxU32 taskSize = mUpdateStateTask.size();
	if (taskSize == 0 || ((mCrabs.size() - mUpdateStateTask[taskSize - 1]->mStartIndex) == 64))
	{
		//Create a new task...
		mUpdateStateTask.pushBack(PX_PLACEMENT_NEW(PX_ALLOC(sizeof(CrabUpdateStateTask), PX_DEBUG_EXP("CrabUpdateTask")), CrabUpdateStateTask)(this, idx, mCrabs.size()));
	}
	mUpdateStateTask[mUpdateStateTask.size() - 1]->mEndIndex = mCrabs.size();
}

void CrabManager::update(const PxReal dt)
{
	mSync.reset();
	PxSceneWriteLock scopedLock(getScene());
	PxTaskManager* manager = getScene().getTaskManager();

	mCompletionTask.setContinuation(*manager, NULL);
	
	for (PxU32 i = 0; i < mCrabs.size(); ++i)
	{
		mCrabs[i]->update(dt);
	}

	for (PxU32 i = 0; i < mUpdateStateTask.size(); ++i)
	{
		mUpdateStateTask[i]->setContinuation(&mCompletionTask);
		mUpdateStateTask[i]->removeReference();
	}

	mCompletionTask.removeReference();

	//mSync.wait();
}

void CrabManager::syncWork()
{
	mSync.wait();
}

void CrabUpdateStateTask::runInternal()
{
	Crab** crab = mCrabManager->getCrabs();

	for (PxU32 i = mStartIndex; i < mEndIndex; ++i)
	{
		crab[i]->run();
	}
}