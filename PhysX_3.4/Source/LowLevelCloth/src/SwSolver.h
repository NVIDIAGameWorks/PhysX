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

#pragma once

#include "Solver.h"
#include "Allocator.h"
#include "SwInterCollision.h"
#include "CmTask.h"

namespace physx
{

namespace cloth
{

class SwCloth;
class SwFactory;

/// CPU/SSE based cloth solver
class SwSolver : public UserAllocated, public Solver
{
	struct StartSimulationTask : public Cm::Task
	{
		using physx::PxLightCpuTask::mRefCount;
		using physx::PxLightCpuTask::mTm;

		StartSimulationTask() : Cm::Task(0)	{}

		virtual void runInternal();
		virtual const char* getName() const;
		SwSolver* mSolver;
	};

	struct EndSimulationTask : public Cm::Task
	{
		using physx::PxLightCpuTask::mRefCount;

		EndSimulationTask() : Cm::Task(0)	{}

		virtual void runInternal();
		virtual const char* getName() const;
		SwSolver* mSolver;
		float mDt;
	};

	struct CpuClothSimulationTask : public Cm::Task
	{
		CpuClothSimulationTask(SwCloth&, EndSimulationTask&);
		virtual void runInternal();
		virtual const char* getName() const;
		virtual void release();

		SwCloth* mCloth;
		EndSimulationTask* mContinuation;
		uint32_t mScratchMemorySize;
		void* mScratchMemory;
		float mInvNumIterations;
	};

  public:
	SwSolver(physx::PxTaskManager*);
	virtual ~SwSolver();

	virtual void addCloth(Cloth*);
	virtual void removeCloth(Cloth*);

	virtual physx::PxBaseTask& simulate(float dt, physx::PxBaseTask&);

	virtual void setInterCollisionDistance(float distance)
	{
		mInterCollisionDistance = distance;
	}
	virtual float getInterCollisionDistance() const
	{
		return mInterCollisionDistance;
	}

	virtual void setInterCollisionStiffness(float stiffness)
	{
		mInterCollisionStiffness = stiffness;
	}
	virtual float getInterCollisionStiffness() const
	{
		return mInterCollisionStiffness;
	}

	virtual void setInterCollisionNbIterations(uint32_t nbIterations)
	{
		mInterCollisionIterations = nbIterations;
	}
	virtual uint32_t getInterCollisionNbIterations() const
	{
		return mInterCollisionIterations;
	}

	virtual void setInterCollisionFilter(InterCollisionFilter filter)
	{
		mInterCollisionFilter = filter;
	}

	virtual bool hasError() const
	{
		return false;
	}

  private:
	void beginFrame() const;
	void endFrame() const;

	void interCollision();

  private:
	StartSimulationTask mStartSimulationTask;

	typedef Vector<CpuClothSimulationTask>::Type CpuClothSimulationTaskVector;
	CpuClothSimulationTaskVector mCpuClothSimulationTasks;

	EndSimulationTask mEndSimulationTask;

	float mInterCollisionDistance;
	float mInterCollisionStiffness;
	uint32_t mInterCollisionIterations;
	InterCollisionFilter mInterCollisionFilter;

	void* mInterCollisionScratchMem;
	uint32_t mInterCollisionScratchMemSize;
	shdfnd::Array<SwInterCollisionData> mInterCollisionInstances;
};
}
}
