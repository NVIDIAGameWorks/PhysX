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

#include "SwSolver.h"
#include "SwCloth.h"
#include "ClothImpl.h"
#include "SwFabric.h"
#include "SwFactory.h"
#include "SwClothData.h"
#include "SwSolverKernel.h"
#include "SwInterCollision.h"
#include "IterationState.h"
#include "PxCpuDispatcher.h"
#include "PxProfileZone.h"
#include "PsFPU.h"
#include "PsSort.h"

namespace nvidia
{
namespace cloth
{
bool neonSolverKernel(SwCloth const&, SwClothData&, SwKernelAllocator&, IterationStateFactory&, profile::PxProfileZone*);
}
}

#if NVMATH_SIMD
typedef Simd4f Simd4fType;
#else
typedef Scalar4f Simd4fType;
#endif

using namespace nvidia;
using namespace cloth;

cloth::SwSolver::SwSolver(nvidia::profile::PxProfileZone* profiler, PxTaskManager* taskMgr)
: mProfiler(profiler)
, mSimulateEventId(mProfiler ? mProfiler->getEventIdForName("cloth::SwSolver::simulate") : uint16_t(-1))
#if APEX_UE4
, mDt(0.0f)
#endif
, mInterCollisionDistance(0.0f)
, mInterCollisionStiffness(1.0f)
, mInterCollisionIterations(1)
, mInterCollisionScratchMem(NULL)
, mInterCollisionScratchMemSize(0)
{
	mStartSimulationTask.mSolver = this;
	mEndSimulationTask.mSolver = this;

	PX_UNUSED(taskMgr);
}

cloth::SwSolver::~SwSolver()
{
	if(mInterCollisionScratchMem)
		PX_FREE(mInterCollisionScratchMem);

	PX_ASSERT(mCpuClothSimulationTasks.empty());
}

namespace
{
template <typename T>
bool clothSizeGreater(const T& t0, const T& t1)
{
#if APEX_UE4
	return t0->mCloth->mCurParticles.size() > t1->mCloth->mCurParticles.size();
#else
	return t0.mCloth->mCurParticles.size() > t1.mCloth->mCurParticles.size();
#endif
}

template <typename T>
void sortTasks(nvidia::Array<T, nvidia::NonTrackingAllocator>& tasks)
{
	nvidia::sort(tasks.begin(), tasks.size(), &clothSizeGreater<T>);
}
}

void cloth::SwSolver::addCloth(Cloth* cloth)
{
	SwCloth& swCloth = static_cast<SwClothImpl&>(*cloth).mCloth;

#if APEX_UE4
	mCpuClothSimulationTasks.pushBack(new CpuClothSimulationTask(swCloth, *this));
#else
	mCpuClothSimulationTasks.pushBack(CpuClothSimulationTask(swCloth, mEndSimulationTask));
#endif

	sortTasks(mCpuClothSimulationTasks);
}

void cloth::SwSolver::removeCloth(Cloth* cloth)
{
	SwCloth& swCloth = static_cast<SwClothImpl&>(*cloth).mCloth;

	CpuClothSimulationTaskVector::Iterator tIt = mCpuClothSimulationTasks.begin();
	CpuClothSimulationTaskVector::Iterator tEnd = mCpuClothSimulationTasks.end();

	while (tIt != tEnd &&
#if APEX_UE4
		(*tIt)->mCloth != &swCloth
#else
		tIt->mCloth != &swCloth
#endif
		)
		++tIt;

	if(tIt != tEnd)
	{
#if APEX_UE4
		delete *tIt;
#else
		deallocate(tIt->mScratchMemory);
#endif
		mCpuClothSimulationTasks.replaceWithLast(tIt);
		sortTasks(mCpuClothSimulationTasks);
	}
}

PxBaseTask& cloth::SwSolver::simulate(float dt, PxBaseTask& continuation)
{
	if (mCpuClothSimulationTasks.empty()
#if APEX_UE4
		|| dt == 0.0f
#endif
		)
	{
		continuation.addReference();
		return continuation;
	}

	mEndSimulationTask.setContinuation(&continuation);
#if APEX_UE4
	mDt = dt;
#else
	mEndSimulationTask.mDt = dt;
#endif

	mStartSimulationTask.setContinuation(&mEndSimulationTask);

	mEndSimulationTask.removeReference();

	return mStartSimulationTask;
}

void cloth::SwSolver::interCollision()
{
	if(!mInterCollisionIterations || mInterCollisionDistance == 0.0f)
		return;

	float elasticity = 1.0f;

	// rebuild cloth instance array
	mInterCollisionInstances.resize(0);
	for(uint32_t i = 0; i < mCpuClothSimulationTasks.size(); ++i)
	{
#if APEX_UE4
		SwCloth* c = mCpuClothSimulationTasks[i]->mCloth;
		float invNumIterations = mCpuClothSimulationTasks[i]->mInvNumIterations;
#else
		SwCloth* c = mCpuClothSimulationTasks[i].mCloth;
		float invNumIterations = mCpuClothSimulationTasks[i].mInvNumIterations;
#endif

		mInterCollisionInstances.pushBack(SwInterCollisionData(
		    c->mCurParticles.begin(), c->mPrevParticles.begin(),
		    c->mSelfCollisionIndices.empty() ? c->mCurParticles.size() : c->mSelfCollisionIndices.size(),
		    c->mSelfCollisionIndices.empty() ? NULL : &c->mSelfCollisionIndices[0], c->mTargetMotion,
		    c->mParticleBoundsCenter, c->mParticleBoundsHalfExtent, elasticity * invNumIterations, c->mUserData));
	}

	const uint32_t requiredTempMemorySize = uint32_t(SwInterCollision<Simd4fType>::estimateTemporaryMemory(
	    &mInterCollisionInstances[0], mInterCollisionInstances.size()));

	// realloc temp memory if necessary
	if(mInterCollisionScratchMemSize < requiredTempMemorySize)
	{
		if(mInterCollisionScratchMem)
			PX_FREE(mInterCollisionScratchMem);

		mInterCollisionScratchMem = PX_ALLOC(requiredTempMemorySize, "cloth::SwSolver::mInterCollisionScratchMem");
		mInterCollisionScratchMemSize = requiredTempMemorySize;
	}

	SwKernelAllocator allocator(mInterCollisionScratchMem, mInterCollisionScratchMemSize);

	// run inter-collision
	SwInterCollision<Simd4fType> collider(mInterCollisionInstances.begin(), mInterCollisionInstances.size(),
	                                      mInterCollisionDistance, mInterCollisionStiffness, mInterCollisionIterations,
	                                      mInterCollisionFilter, allocator, mProfiler);

	collider();
}

void cloth::SwSolver::beginFrame() const
{
	if(mProfiler)
		mProfiler->startEvent(mSimulateEventId, uint64_t(intptr_t(this)), uint32_t(intptr_t(this)));
}

void cloth::SwSolver::endFrame() const
{
	if(mProfiler)
		mProfiler->stopEvent(mSimulateEventId, uint64_t(intptr_t(this)), uint32_t(intptr_t(this)));
}

#if APEX_UE4
void cloth::SwSolver::simulate(void* task, float dt)
{
	if (task)
		static_cast<cloth::SwSolver::CpuClothSimulationTask*>(task)->simulate(dt);
}
#endif

void cloth::SwSolver::StartSimulationTask::runInternal()
{
	mSolver->beginFrame();

	CpuClothSimulationTaskVector::Iterator tIt = mSolver->mCpuClothSimulationTasks.begin();
	CpuClothSimulationTaskVector::Iterator tEnd = mSolver->mCpuClothSimulationTasks.end();

	for(; tIt != tEnd; ++tIt)
	{
#if APEX_UE4
		if (!(*tIt)->mCloth->isSleeping())
		{
			(*tIt)->setContinuation(mCont);
			(*tIt)->removeReference();
		}
#else
		if(!tIt->mCloth->isSleeping())
		{
			tIt->setContinuation(mCont);
			tIt->removeReference();
		}
#endif
	}
}

const char* cloth::SwSolver::StartSimulationTask::getName() const
{
	return "cloth.SwSolver.startSimulation";
}

void cloth::SwSolver::EndSimulationTask::runInternal()
{
	mSolver->interCollision();
	mSolver->endFrame();
}

const char* cloth::SwSolver::EndSimulationTask::getName() const
{
	return "cloth.SwSolver.endSimulation";
}

#if !APEX_UE4
cloth::SwSolver::CpuClothSimulationTask::CpuClothSimulationTask(SwCloth& cloth, EndSimulationTask& continuation)
: mCloth(&cloth), mContinuation(&continuation), mScratchMemorySize(0), mScratchMemory(0), mInvNumIterations(0.0f)
{
}
#endif

#if APEX_UE4
cloth::SwSolver::CpuClothSimulationTask::CpuClothSimulationTask(SwCloth& cloth, SwSolver& solver)
	: mCloth(&cloth), mSolver(&solver), mScratchMemorySize(0), mScratchMemory(0), mInvNumIterations(0.0f)
{
	mCloth->mSimulationTask = this;
}

cloth::SwSolver::CpuClothSimulationTask::~CpuClothSimulationTask()
{
	deallocate(mScratchMemory);
	mCloth->mSimulationTask = NULL;
}

void cloth::SwSolver::CpuClothSimulationTask::runInternal()
{
	simulate(mSolver->mDt);
}


void cloth::SwSolver::CpuClothSimulationTask::simulate(float dt)
{
	// check if we need to reallocate the temp memory buffer
	// (number of shapes may have changed)
	uint32_t requiredTempMemorySize = uint32_t(SwSolverKernel<Simd4fType>::estimateTemporaryMemory(*mCloth));

	if (mScratchMemorySize < requiredTempMemorySize)
	{
		deallocate(mScratchMemory);

		mScratchMemory = allocate(requiredTempMemorySize);
		mScratchMemorySize = requiredTempMemorySize;
	}

	IterationStateFactory factory(*mCloth, dt);
	mInvNumIterations = factory.mInvNumIterations;

	nvidia::SIMDGuard simdGuard;

	SwClothData data(*mCloth, mCloth->mFabric);
	SwKernelAllocator allocator(mScratchMemory, uint32_t(mScratchMemorySize));
	nvidia::profile::PxProfileZone* profileZone = mSolver->mProfiler;

	// construct kernel functor and execute
#if PX_ANDROID
	// if(!neonSolverKernel(cloth, data, allocator, factory, profileZone))
#endif
	SwSolverKernel<Simd4fType>(*mCloth, data, allocator, factory, profileZone)();

	data.reconcile(*mCloth); // update cloth

	release();
}

#else

void cloth::SwSolver::CpuClothSimulationTask::runInternal()
{
	// check if we need to reallocate the temp memory buffer
	// (number of shapes may have changed)
	uint32_t requiredTempMemorySize = uint32_t(SwSolverKernel<Simd4fType>::estimateTemporaryMemory(*mCloth));

	if(mScratchMemorySize < requiredTempMemorySize)
	{
		deallocate(mScratchMemory);

		mScratchMemory = allocate(requiredTempMemorySize);
		mScratchMemorySize = requiredTempMemorySize;
	}

	if(mContinuation->mDt == 0.0f)
		return;

	IterationStateFactory factory(*mCloth, mContinuation->mDt);
	mInvNumIterations = factory.mInvNumIterations;

	nvidia::SIMDGuard simdGuard;

	SwClothData data(*mCloth, mCloth->mFabric);
	SwKernelAllocator allocator(mScratchMemory, uint32_t(mScratchMemorySize));
	nvidia::profile::PxProfileZone* profileZone = mContinuation->mSolver->mProfiler;

	// construct kernel functor and execute
#if PX_ANDROID
	// if(!neonSolverKernel(cloth, data, allocator, factory, profileZone))
#endif
	SwSolverKernel<Simd4fType>(*mCloth, data, allocator, factory, profileZone)();

	data.reconcile(*mCloth); // update cloth
}
#endif

const char* cloth::SwSolver::CpuClothSimulationTask::getName() const
{
	return "cloth.SwSolver.cpuClothSimulation";
}

void cloth::SwSolver::CpuClothSimulationTask::release()
{
	mCloth->mMotionConstraints.pop();
	mCloth->mSeparationConstraints.pop();

	if (!mCloth->mTargetCollisionSpheres.empty())
	{
		swap(mCloth->mStartCollisionSpheres, mCloth->mTargetCollisionSpheres);
		mCloth->mTargetCollisionSpheres.resize(0);
	}

	if (!mCloth->mTargetCollisionPlanes.empty())
	{
		swap(mCloth->mStartCollisionPlanes, mCloth->mTargetCollisionPlanes);
		mCloth->mTargetCollisionPlanes.resize(0);
	}

	if (!mCloth->mTargetCollisionTriangles.empty())
	{
		swap(mCloth->mStartCollisionTriangles, mCloth->mTargetCollisionTriangles);
		mCloth->mTargetCollisionTriangles.resize(0);
	}
#if !APEX_UE4
	mContinuation->removeReference();
#endif
}

#if APEX_UE4
void(*const cloth::SwCloth::sSimulationFunction)(void*, float) = &cloth::SwSolver::simulate;
#endif