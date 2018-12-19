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


// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#pragma once

#include "Solver.h"
#include "Allocator.h"
#include "SwInterCollision.h"
#include "CmTask.h"

namespace nvidia
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
		using PxLightCpuTask::mRefCount;
		using PxLightCpuTask::mTm;

		virtual void runInternal();
		virtual const char* getName() const;

		SwSolver* mSolver;
	};

	struct EndSimulationTask : public Cm::Task
	{
		using PxLightCpuTask::mRefCount;

		virtual void runInternal();
		virtual const char* getName() const;

		SwSolver* mSolver;
#if !APEX_UE4
		float mDt;
#endif
	};

	struct CpuClothSimulationTask : public Cm::Task
	{
#if APEX_UE4
		void* operator new(size_t n){ return allocate(n); }
		void operator delete(void* ptr) { return deallocate(ptr); }

		CpuClothSimulationTask(SwCloth&, SwSolver&);
		~CpuClothSimulationTask();

		void simulate(float dt);

		SwSolver* mSolver;
#else
		CpuClothSimulationTask(SwCloth&, EndSimulationTask&);

		EndSimulationTask* mContinuation;
#endif
		virtual void runInternal();
		virtual const char* getName() const;
		virtual void release();

		SwCloth* mCloth;

		uint32_t mScratchMemorySize;
		void* mScratchMemory;
		float mInvNumIterations;
	};

  public:
	SwSolver(nvidia::profile::PxProfileZone*, PxTaskManager*);
	virtual ~SwSolver();

	virtual void addCloth(Cloth*);
	virtual void removeCloth(Cloth*);

	virtual PxBaseTask& simulate(float dt, PxBaseTask&);

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

	virtual uint32_t getNumSharedPositions( const Cloth* ) const 
	{ 
		return uint32_t(-1); 
	}

	virtual bool hasError() const
	{
		return false;
	}

#if APEX_UE4
	static void simulate(void*, float);
#endif

  private:
	void beginFrame() const;
	void endFrame() const;

	void interCollision();

  private:
	StartSimulationTask mStartSimulationTask;

#if APEX_UE4
	typedef Vector<CpuClothSimulationTask*>::Type CpuClothSimulationTaskVector;
	float mDt;
#else
	typedef Vector<CpuClothSimulationTask>::Type CpuClothSimulationTaskVector;
#endif

	CpuClothSimulationTaskVector mCpuClothSimulationTasks;

	EndSimulationTask mEndSimulationTask;

	profile::PxProfileZone* mProfiler;
	uint16_t mSimulateEventId;

	float mInterCollisionDistance;
	float mInterCollisionStiffness;
	uint32_t mInterCollisionIterations;
	InterCollisionFilter mInterCollisionFilter;

	void* mInterCollisionScratchMem;
	uint32_t mInterCollisionScratchMemSize;
	nvidia::Array<SwInterCollisionData> mInterCollisionInstances;

};
}
}
