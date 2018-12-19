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

#include "foundation/PxProfiler.h"
#include "CuSolver.h"
#include "CuCloth.h"
#include "ClothImpl.h"
#include "CuFabric.h"
#include "CuFactory.h"
#include "CuSolverKernel.h"
#include "CuContextLock.h"
#include "CuCheckSuccess.h"
#include "IterationState.h"
#include "CudaKernelWrangler.h"
#include "PsUtilities.h"
#include "PsSort.h"
#include "PsFoundation.h"

#if PX_NVTX
#include "nvToolsExt.h"
#endif

//#define ENABLE_CUDA_PRINTF PX_DEBUG // warning: not thread safe
#define ENABLE_CUDA_PRINTF 0

#if ENABLE_CUDA_PRINTF
extern "C" cudaError_t cudaPrintfInit(CUmodule hmod, size_t bufferLen = 1048576);
extern "C" void cudaPrintfEnd();
extern "C" cudaError_t cudaPrintfDisplay(CUmodule hmod, void* outputFP = NULL, bool showThreadID = false);
#endif

using namespace physx;

namespace
{
//for KernelWrangler interface
const char* gKernelName = cloth::getKernelFunctionName();
}

namespace
{
template <typename T>
struct CuDeviceAllocator
{
	CuDeviceAllocator(physx::PxCudaContextManager* ctx) : mManager(ctx->getMemoryManager())
	{
	}

	T* allocate(size_t n)
	{
		return reinterpret_cast<T*>(mManager->alloc(physx::PxCudaBufferMemorySpace::T_GPU, n * sizeof(T)));
	}

	void deallocate(T* ptr)
	{
		mManager->free(physx::PxCudaBufferMemorySpace::T_GPU, reinterpret_cast<physx::PxCudaBufferPtr>(ptr));
	}

	physx::PxCudaMemoryManager* mManager;
};
}

cloth::CuSolver::CuSolver(CuFactory& factory)
: CuContextLock(factory)
, mFactory(factory)
, mClothData(mFactory.mContextManager)
, mClothDataHostCopy(CuHostAllocator(mFactory.mContextManager, cudaHostAllocWriteCombined))
, mClothDataDirty(false)
, mFrameData(getMappedAllocator<CuFrameData>(mFactory.mContextManager))
, mIterationData(getMappedAllocator<CuIterationData>(mFactory.mContextManager))
, mIterationDataBegin(0)
, mFrameDt(0.0f)
, mSharedMemorySize(0)
, mSharedMemoryLimit(0)
, mStartSimulationTask(&CuSolver::beginFrame, "cloth.CuSolver.startSimulation")
, mKernelSimulationTask(&CuSolver::executeKernel, "cloth.CuSolver.kernelSimulation")
, mEndSimulationTask(&CuSolver::endFrame, "cloth.CuSolver.endSimulation")
, mStream(0)
, mKernelModule(0)
, mKernelFunction(0)
, mKernelSharedMemorySize(0)
, mClothIndex(CuDeviceAllocator<uint32_t>(mFactory.mContextManager).allocate(1))
, mInterCollisionDistance(0.0f)
, mInterCollisionStiffness(1.0f)
, mInterCollisionIterations(1)
, mInterCollisionScratchMem(NULL)
, mInterCollisionScratchMemSize(0)
, mKernelWrangler(getDispatcher(), physx::shdfnd::getFoundation().getErrorCallback(), &gKernelName, 1)
, mSimulateNvtxRangeId(0)
, mCudaError(mKernelWrangler.hadError())
{
	if(mCudaError)
	{
		CuContextLock::release();
		return;
	}

	mStartSimulationTask.mSolver = this;
	mKernelSimulationTask.mSolver = this;
	mEndSimulationTask.mSolver = this;

	if(mFactory.mContextManager->getUsingConcurrentStreams())
		checkSuccess(cuStreamCreate(&mStream, 0));

	if(1)
	{
		mKernelModule = mKernelWrangler.getCuModule(0);
		mKernelFunction = mKernelWrangler.getCuFunction(0);
	}
	else
	{
		// load from ptx instead of embedded SASS, for iterating without recompile
		checkSuccess(cuModuleLoad(&mKernelModule, "CuSolverKernel.ptx"));
		checkSuccess(cuModuleGetFunction(&mKernelFunction, mKernelModule, getKernelFunctionName()));
		shdfnd::getFoundation().error(PX_INFO, "Cloth kernel code loaded from CuSolverKernel.ptx");
	}

	// get amount of statically allocated shared memory
	checkSuccess(cuFuncGetAttribute(&mKernelSharedMemorySize, CU_FUNC_ATTRIBUTE_SHARED_SIZE_BYTES, mKernelFunction));

	// extract CuKernelData device pointer
	size_t size = 0;
	CUdeviceptr ptr = 0;
	checkSuccess(cuModuleGetGlobal(&ptr, &size, mKernelModule, getKernelDataName()));
	mKernelData = CuDevicePointer<CuKernelData>(reinterpret_cast<CuKernelData*>(ptr));

	// initialize cloth index
	checkSuccess(cuMemsetD32(mClothIndex.dev(), 0, 1));

	CuContextLock::release();
}

cloth::CuSolver::~CuSolver()
{
	PX_ASSERT(mCloths.empty());

	CuContextLock::acquire();

	CuKernelData kernelData = {};
	*mKernelData = kernelData;

	CuDeviceAllocator<uint32_t>(mFactory.mContextManager).deallocate(mClothIndex.get());

	if(mStream)
		checkSuccess(cuStreamDestroy(mStream));

	if(mInterCollisionScratchMem)
		PX_FREE(mInterCollisionScratchMem);
}

void cloth::CuSolver::updateKernelData()
{
	CuKernelData kernelData;

	kernelData.mClothIndex = mClothIndex.get();
	kernelData.mClothData = mClothData.begin().get();
	kernelData.mFrameData = getDevicePointer(mFrameData);

	*mKernelData = kernelData;
}

physx::PxGpuDispatcher& cloth::CuSolver::getDispatcher() const
{
	return *mFactory.mContextManager->getGpuDispatcher();
}

namespace
{
struct ClothSimCostGreater
{
	bool operator()(const cloth::CuCloth* left, const cloth::CuCloth* right) const
	{
		return left->mNumParticles * left->mSolverFrequency > right->mNumParticles * right->mSolverFrequency;
	}
};
}

void cloth::CuSolver::addCloth(Cloth* cloth)
{
	CuCloth& cuCloth = static_cast<CuClothImpl&>(*cloth).mCloth;

	PX_ASSERT(mCloths.find(&cuCloth) == mCloths.end());

	mCloths.pushBack(&cuCloth);
	// trigger update of mClothData array
	cuCloth.notifyChanged();

	// sort cloth instances by size
	shdfnd::sort(mCloths.begin(), mCloths.size(), ClothSimCostGreater());

	CuContextLock contextLock(mFactory);

	// resize containers and update kernel data
	mClothDataHostCopy.resize(mCloths.size());
	mClothData.resize(mCloths.size());
	mFrameData.resize(mCloths.size());
	updateKernelData();
}

void cloth::CuSolver::removeCloth(Cloth* cloth)
{
	CuCloth& cuCloth = static_cast<CuClothImpl&>(*cloth).mCloth;

	ClothVector::Iterator begin = mCloths.begin(), end = mCloths.end();
	ClothVector::Iterator it = mCloths.find(&cuCloth);

	if(it == end)
		return; // not found

	uint32_t index = uint32_t(it - begin);

	mCloths.remove(index);
	mClothDataHostCopy.remove(index);
	mClothData.resize(mCloths.size());
	mClothDataDirty = true;
}

physx::PxBaseTask& cloth::CuSolver::simulate(float dt, physx::PxBaseTask& continuation)
{
	mFrameDt = dt;

	if(mCloths.empty() || mCudaError)
	{
		continuation.addReference();
		return continuation;
	}

	physx::PxGpuDispatcher& disp = getDispatcher();
	mEndSimulationTask.setContinuation(&continuation);
	disp.addPostLaunchDependent(mEndSimulationTask);
	mKernelSimulationTask.setContinuation(&disp.getPostLaunchTask());
	disp.getPostLaunchTask().removeReference();
	disp.addPreLaunchDependent(mKernelSimulationTask);
	mStartSimulationTask.setContinuation(&disp.getPreLaunchTask());
	disp.getPreLaunchTask().removeReference();

	mEndSimulationTask.removeReference();
	mKernelSimulationTask.removeReference();

	return mStartSimulationTask;
}

void cloth::CuSolver::beginFrame()
{
	CuContextLock contextLock(mFactory);

	PX_PROFILE_START_CROSSTHREAD("cloth.CuSolver.simulate", 0);

	CuIterationData* iterationDataBegin = mIterationData.empty() ? 0 : &mIterationData.front();

	mFrameData.resize(0);
	mIterationData.resize(0);

	// update cloth data
	ClothVector::Iterator cIt, cEnd = mCloths.end();
	CuPinnedVector<CuClothData>::Type::Iterator dIt = mClothDataHostCopy.begin();
	for(cIt = mCloths.begin(); cIt != cEnd; ++cIt, ++dIt)
		mClothDataDirty |= (*cIt)->updateClothData(*dIt);

	if(mClothDataDirty)
	{
		/* find optimal number of cloths per SM */

		// at least 192 threads per block (e.g. CuCollision::buildAcceleration)
		uint32_t numSMs = (uint32_t)mFactory.mContextManager->getMultiprocessorCount();
		uint32_t maxClothsPerSM = PxMin(mFactory.mMaxThreadsPerBlock / 192, (mCloths.size() + numSMs - 1) / numSMs);

		// tuning parameters: relative performance per numSharedPositions
		float weights[3] = { 0.4f, 0.8f, 1.0f };

		// try all possible number of cloths per SM and estimate performance
		float maxWeightSum = 0.0f;
		uint32_t numClothsPerSM = 0;
		for(uint32_t i = 1; i <= maxClothsPerSM; ++i)
		{
			uint32_t sharedMemoryLimit = (mFactory.mContextManager->getSharedMemPerBlock() / i) - mKernelSharedMemorySize;

			float weightSum = 0.0f;
			for(cIt = mCloths.begin(); cIt != cEnd; ++cIt)
			{
				uint32_t sharedMemorySize = (*cIt)->mSharedMemorySize;
				uint32_t positionsSize = (*cIt)->mNumParticles * sizeof(PxVec4);

				if(sharedMemorySize > sharedMemoryLimit)
					break;

				uint32_t numSharedPositions = PxMin(2u, (sharedMemoryLimit - sharedMemorySize) / positionsSize);

				weightSum += weights[numSharedPositions] * positionsSize;
			}
			// tuning parameter: inverse performance for running i cloths per SM
			weightSum *= 2.0f + i;

			if(cIt == cEnd && weightSum > maxWeightSum)
			{
				maxWeightSum = weightSum;
				numClothsPerSM = i;
			}
		}
		PX_ASSERT(numClothsPerSM);

		// update block size
		uint32_t numThreadsPerBlock = mFactory.mMaxThreadsPerBlock / numClothsPerSM & ~31;

		// Workaround for nvbug 1709919: theoretically, register usage should allow us to launch at least
		// mFactory.mMaxThreadsPerBlock threads, because that value corresponds to __launch_bounds__(maxThreadsPerBlock).
		CUdevice device = 0;
		checkSuccess(cuCtxGetDevice(&device));
		int registersPerBlock = 0, kernelRegisterCount = 0;
		checkSuccess(cuDeviceGetAttribute(&registersPerBlock, CU_DEVICE_ATTRIBUTE_MAX_REGISTERS_PER_BLOCK, device));
		checkSuccess(cuFuncGetAttribute(&kernelRegisterCount, CU_FUNC_ATTRIBUTE_NUM_REGS, mKernelFunction));
		numThreadsPerBlock = PxMin(numThreadsPerBlock, uint32_t(registersPerBlock / kernelRegisterCount));
		PX_ASSERT(numThreadsPerBlock >= 192);

		if(mFactory.mNumThreadsPerBlock != numThreadsPerBlock)
		{
			checkSuccess(
			    cuFuncSetBlockShape(mKernelFunction, int(mFactory.mNumThreadsPerBlock = numThreadsPerBlock), 1, 1));
		}

		// remember num cloths per SM in terms of max shared memory per block
		mSharedMemoryLimit =
		    (mFactory.mContextManager->getSharedMemPerBlock() / numClothsPerSM) - mKernelSharedMemorySize;
	}

	uint32_t maxSharedMemorySize = 0;
	for(cIt = mCloths.begin(); cIt != cEnd; ++cIt)
	{
		CuCloth& cloth = **cIt;

		uint32_t sharedMemorySize = cloth.mSharedMemorySize;
		uint32_t positionsSize = cloth.mNumParticles * sizeof(PxVec4);

		uint32_t numSharedPositions = PxMin(2u, (mSharedMemoryLimit - sharedMemorySize) / positionsSize);

		maxSharedMemorySize = PxMax(maxSharedMemorySize, sharedMemorySize + numSharedPositions * positionsSize);

		IterationStateFactory factory(cloth, mFrameDt);
		IterationState<Simd4f> state = factory.create<Simd4f>(cloth);

		mFrameData.pushBack(CuFrameData(cloth, numSharedPositions, state, mIterationDataBegin + mIterationData.size()));

		while(state.mRemainingIterations)
		{
			mIterationData.pushBack(CuIterationData(state));
			state.update();
		}
	}
	mSharedMemorySize = maxSharedMemorySize;

	// add dummy element because we read past the end
	mIterationData.pushBack(CuIterationData());

	if(&mIterationData.front() != iterationDataBegin)
	{
		// mIterationData grew, update pointers
		iterationDataBegin = getDevicePointer(mIterationData);

		ptrdiff_t diff = (char*)iterationDataBegin - (char*)mIterationDataBegin;
		CuPinnedVector<CuFrameData>::Type::Iterator fIt = mFrameData.begin(), fEnd;
		for(fEnd = mFrameData.end(); fIt != fEnd; ++fIt)
			reinterpret_cast<const char*&>(fIt->mIterationData) += diff;

		mIterationDataBegin = iterationDataBegin;
	}
}

void cloth::CuSolver::executeKernel()
{
	CuContextLock contextLock(mFactory);

#if ENABLE_CUDA_PRINTF
	if(cudaError result = cudaPrintfInit(mKernelModule))
	{
		shdfnd::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "cudaPrintfInit() returned %u.",
		                              result);
	}
#endif

	if(mClothDataDirty)
	{
		PX_ASSERT(mClothDataHostCopy.size() == mClothData.size());
		size_t numBytes = mClothData.size() * sizeof(CuClothData);
		checkSuccess(cuMemcpyHtoDAsync(mClothData.begin().dev(), mClothDataHostCopy.begin(), numBytes, mStream));
		mClothDataDirty = false;
	}

#if 0
	static int frame = 0;
	if(++frame == 100)
		record(*this);
#endif

	// launch kernel
	CUresult result = cuLaunchKernel(mKernelFunction, mCloths.size(), 1, 1, mFactory.mNumThreadsPerBlock, 1, 1,
	                                 mSharedMemorySize, mStream, 0, 0);

#if ENABLE_CUDA_PRINTF
	cudaPrintfDisplay(mKernelModule);
	cudaPrintfEnd();
#endif

#if PX_DEBUG
	// in debug builds check kernel result
	checkSuccess(result);
	checkSuccess(cuStreamSynchronize(mStream));
#endif

	// mark the solver as being in an error state
	// all cloth instances will be migrated to software
	if(result != CUDA_SUCCESS)
		mCudaError = true;
}

void cloth::CuSolver::endFrame()
{
	CuPinnedVector<CuFrameData>::Type::ConstIterator fIt = mFrameData.begin();
	ClothVector::Iterator cIt, cEnd = mCloths.end();
	for(cIt = mCloths.begin(); cIt != cEnd; ++cIt, ++fIt)
	{
		CuCloth& cloth = **cIt;

		cloth.mHostParticlesDirty = false;
		cloth.mDeviceParticlesDirty = false;

		cloth.mMotionConstraints.pop();
		cloth.mMotionConstraints.mHostCopy.resize(0);

		cloth.mSeparationConstraints.pop();
		cloth.mSeparationConstraints.mHostCopy.resize(0);

		if(!cloth.mTargetCollisionSpheres.empty())
		{
			shdfnd::swap(cloth.mStartCollisionSpheres, cloth.mTargetCollisionSpheres);
			cloth.mTargetCollisionSpheres.resize(0);
		}

		if(!cloth.mTargetCollisionPlanes.empty())
		{
			shdfnd::swap(cloth.mStartCollisionPlanes, cloth.mTargetCollisionPlanes);
			cloth.mTargetCollisionPlanes.resize(0);
		}

		if(!cloth.mTargetCollisionTriangles.empty())
		{
			shdfnd::swap(cloth.mStartCollisionTriangles, cloth.mTargetCollisionTriangles);
			cloth.mTargetCollisionTriangles.resize(0);
		}

		for(uint32_t i = 0; i < 3; ++i)
		{
			float upper = fIt->mParticleBounds[i * 2 + 0];
			float negativeLower = fIt->mParticleBounds[i * 2 + 1];
			cloth.mParticleBoundsCenter[i] = (upper - negativeLower) * 0.5f;
			cloth.mParticleBoundsHalfExtent[i] = (upper + negativeLower) * 0.5f;
		}

		cloth.mSleepPassCounter = fIt->mSleepPassCounter;
		cloth.mSleepTestCounter = fIt->mSleepTestCounter;
	}

	interCollision();

	PX_PROFILE_STOP_CROSSTHREAD("cloth::CuSolver::simulate", 0);
}

void cloth::CuSolver::interCollision()
{
	if(!mInterCollisionIterations || mInterCollisionDistance == 0.0f)
		return;

	typedef SwInterCollision<Simd4f> SwInterCollision;

	// rebuild cloth instance array
	mInterCollisionInstances.resize(0);
	for(uint32_t i = 0, n = mCloths.size(); i < n; ++i)
	{
		CuCloth& cloth = *mCloths[i];

		float elasticity = 1.0f / mFrameData[i].mNumIterations;
		PX_ASSERT(!cloth.mHostParticlesDirty);
		PxVec4* particles = cloth.mParticlesHostCopy.begin();
		uint32_t* indices = NULL, numIndices = cloth.mNumParticles;
		if(!cloth.mSelfCollisionIndices.empty())
		{
			indices = cloth.mSelfCollisionIndicesHost.begin();
			numIndices = uint32_t(cloth.mSelfCollisionIndices.size());
		}

		mInterCollisionInstances.pushBack(SwInterCollisionData(
		    particles, particles + cloth.mNumParticles, numIndices, indices, cloth.mTargetMotion,
		    cloth.mParticleBoundsCenter, cloth.mParticleBoundsHalfExtent, elasticity, cloth.mUserData));

		cloth.mDeviceParticlesDirty = true;
	}

	uint32_t requiredTempMemorySize = uint32_t(
	    SwInterCollision::estimateTemporaryMemory(&mInterCollisionInstances[0], mInterCollisionInstances.size()));

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
	SwInterCollision(mInterCollisionInstances.begin(), mInterCollisionInstances.size(), mInterCollisionDistance,
	                 mInterCollisionStiffness, mInterCollisionIterations, mInterCollisionFilter, allocator)();
}

cloth::CuSolver::ClothSolverTask::ClothSolverTask(FunctionPtr functionPtr, const char* name)
: Cm::Task(0), mSolver(0), mFunctionPtr(functionPtr), mName(name)
{
}

void cloth::CuSolver::ClothSolverTask::runInternal()
{
	(mSolver->*mFunctionPtr)();
}

const char* cloth::CuSolver::ClothSolverTask::getName() const
{
	return mName;
}
