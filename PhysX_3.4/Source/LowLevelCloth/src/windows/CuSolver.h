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
#include "CuClothData.h"
#include "CuPinnedAllocator.h"
#include "CuContextLock.h"
#include "CuDeviceVector.h"
#include "CudaKernelWrangler.h"
#include "CmTask.h"

#include "SwInterCollision.h"

namespace physx
{

namespace cloth
{

class CuCloth;
class CuFabric;
struct PhaseConfig;
struct CuKernelData;

class CuSolver : public UserAllocated, private CuContextLock, public Solver
{
#if PX_VC
#pragma warning(push)
#pragma warning(disable : 4371) // layout of class may have changed from a previous version of the compiler due to
                                // better packing of member
#endif
	struct ClothSolverTask : public Cm::Task
	{
		typedef void (CuSolver::*FunctionPtr)();

		ClothSolverTask(FunctionPtr, const char*);
		virtual void runInternal();
		virtual const char* getName() const;

		CuSolver* mSolver;
		FunctionPtr mFunctionPtr;
		const char* mName;
	};
#if PX_VC
#pragma warning(pop)
#endif

	PX_NOCOPY(CuSolver)
  public:
	CuSolver(CuFactory&);
	~CuSolver();

	virtual void addCloth(Cloth*);
	virtual void removeCloth(Cloth*);

	virtual physx::PxBaseTask& simulate(float dt, physx::PxBaseTask&);

	virtual bool hasError() const
	{
		return mCudaError;
	}

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

  private:
	void updateKernelData(); // context needs to be acquired

	// simulate helper functions
	void beginFrame();
	void executeKernel();
	void endFrame();

	void interCollision();

	physx::PxGpuDispatcher& getDispatcher() const;

  private:
	CuFactory& mFactory;

	typedef Vector<CuCloth*>::Type ClothVector;
	ClothVector mCloths;

	CuDeviceVector<CuClothData> mClothData;
	CuPinnedVector<CuClothData>::Type mClothDataHostCopy;
	bool mClothDataDirty;

	CuPinnedVector<CuFrameData>::Type mFrameData;

	CuPinnedVector<CuIterationData>::Type mIterationData;
	CuIterationData* mIterationDataBegin; // corresponding device ptr

	float mFrameDt;

	uint32_t mSharedMemorySize;
	uint32_t mSharedMemoryLimit;

	ClothSolverTask mStartSimulationTask;
	ClothSolverTask mKernelSimulationTask;
	ClothSolverTask mEndSimulationTask;

	CUstream mStream;
	CUmodule mKernelModule;
	CUfunction mKernelFunction;
	int mKernelSharedMemorySize;
	CuDevicePointer<CuKernelData> mKernelData;
	CuDevicePointer<uint32_t> mClothIndex;

	float mInterCollisionDistance;
	float mInterCollisionStiffness;
	uint32_t mInterCollisionIterations;
	InterCollisionFilter mInterCollisionFilter;
	void* mInterCollisionScratchMem;
	uint32_t mInterCollisionScratchMemSize;
	shdfnd::Array<SwInterCollisionData> mInterCollisionInstances;

	physx::KernelWrangler mKernelWrangler;

	uint64_t mSimulateNvtxRangeId;

	bool mCudaError;

	friend void record(const CuSolver&);
};
}
}
