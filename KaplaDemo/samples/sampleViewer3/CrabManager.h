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

#ifndef CRAB_MANAGER_H
#define	CRAB_MANAGER_H

#include "foundation/Px.h"
#include "foundation/PxSimpleTypes.h"
#include "common/PxPhysXCommonConfig.h"
#include "PsSync.h"
#include "CmTask.h"
#include "PsArray.h"

class SceneCrab;
class Crab;
class Compound;
struct ShaderMaterial;
class ShaderShadow;
class CrabManager;


class CompletionTask : public physx::Cm::Task
{
	physx::shdfnd::Sync& mSync;

public:

	CompletionTask(physx::shdfnd::Sync& sync) : physx::Cm::Task(0), mSync(sync)
	{
	}

	virtual void runInternal() { mSync.set(); }

	virtual const char* getName() const { return "CrabCompletionTask"; }
};

class CrabUpdateStateTask : public physx::Cm::Task
{
public:
	CrabManager* mCrabManager;
	physx::PxU32 mStartIndex;
	physx::PxU32 mEndIndex;


	CrabUpdateStateTask(CrabManager* crabManager, const physx::PxU32 startIndex, const physx::PxU32 endIndex) :
		physx::Cm::Task(0), mCrabManager(crabManager), mStartIndex(startIndex), mEndIndex(endIndex)
	{
	}

	virtual void runInternal();


	virtual const char* getName() const { return "CrabCompletionTask"; }
};

class CrabManager
{
public:
	CrabManager();
	~CrabManager();

	void initialize(const physx::PxU32 nbCrabs);
	void setCrabScene(SceneCrab* sceneCrab) { mSceneCrab = sceneCrab; }

	void createCrab(const physx::PxVec3& crabPos, const physx::PxReal crabDepth, const physx::PxReal scale, const physx::PxReal legMass, const physx::PxU32 nbLegs);
	void update(const physx::PxReal dt);

	physx::PxScene& getScene();
	physx::PxPhysics& getPhysics();
	//SimScene* getSimScene();

	void setScene(physx::PxScene* scene);

	void setSceneCrab(SceneCrab* sceneCrab);
	SceneCrab* getSceneCrab();

	Compound* createObject(const physx::PxTransform &pose, const physx::PxVec3 &vel, const physx::PxVec3 &omega,
		bool particles, const ShaderMaterial &mat, bool useSecondaryPattern = false, ShaderShadow* shader = NULL);

	void syncWork();
	Crab** getCrabs() { return mCrabs.begin();  }

	

private:

	SceneCrab* mSceneCrab;
	//Crab*	   mCrabs;
	//physx::PxU32	   mNbCrabs;

	physx::shdfnd::Array<Crab*> mCrabs;

	physx::shdfnd::Sync mSync;

	CompletionTask mCompletionTask;
	//CrabUpdateStateTask* mUpdateStateTask;
	physx::shdfnd::Array<CrabUpdateStateTask*> mUpdateStateTask;
};

#endif
