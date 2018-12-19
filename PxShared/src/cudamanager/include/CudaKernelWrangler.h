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

#ifndef __CUDA_KERNEL_WRANGLER__
#define __CUDA_KERNEL_WRANGLER__

// Make this header is safe for inclusion in headers that are shared with device code.
#if !defined(__CUDACC__)

#include "task/PxTaskDefine.h"
#include "task/PxGpuDispatcher.h"

#include "PsUserAllocated.h"
#include "PsArray.h"

#include <cuda.h>

namespace physx
{

class KernelWrangler : public shdfnd::UserAllocated
{
	PX_NOCOPY(KernelWrangler)
public:
	KernelWrangler(PxGpuDispatcher& gd, PxErrorCallback& errorCallback, const char** funcNames, uint16_t numFuncs);
	~KernelWrangler();

	CUfunction getCuFunction(uint16_t funcIndex) const
	{
		return mCuFunctions[ funcIndex ];
	}

	CUmodule getCuModule(uint16_t funcIndex) const
	{
		uint16_t modIndex = mCuFuncModIndex[ funcIndex ];
		return mCuModules[ modIndex ];
	}

	static void const* const* getImages();
	static int getNumImages();

	bool hadError() const { return mError; }

protected:
	bool						mError;
	shdfnd::Array<CUfunction>	mCuFunctions;
	shdfnd::Array<uint16_t>		mCuFuncModIndex;
	shdfnd::Array<CUmodule>	    mCuModules;
	PxGpuDispatcher&			mGpuDispatcher;
	PxErrorCallback&			mErrorCallback;
};

/* SJB - These were "borrowed" from an Ignacio Llamas email to devtech-compute.
 * If we feel this is too clumsy, we can steal the boost based bits from APEX
 */

class ExplicitCudaFlush
{
public:
	ExplicitCudaFlush(int cudaFlushCount) : mCudaFlushCount(cudaFlushCount), mDefaultCudaFlushCount(mCudaFlushCount) {}
	~ExplicitCudaFlush() {}

	void setCudaFlushCount(int value) { mCudaFlushCount = mDefaultCudaFlushCount = value; }
	unsigned int getCudaFlushCount() const	{ return (unsigned int)mCudaFlushCount; }
	void resetCudaFlushCount() { mCudaFlushCount = mDefaultCudaFlushCount; }

	void decrementFlushCount()
	{
		if (mCudaFlushCount == 0) return;

		if (--mCudaFlushCount == 0)
		{
			CUresult ret = cuStreamQuery(0); // flushes current push buffer
			PX_UNUSED(ret);
			PX_ASSERT(ret == CUDA_SUCCESS || ret == CUDA_ERROR_NOT_READY);

			// For current implementation, disable resetting of cuda flush count
			// reset cuda flush count
			// mCudaFlushCount = mDefaultCudaFlushCount;
		}
	}

private:
	int mCudaFlushCount;
	int mDefaultCudaFlushCount;
};

}

template <typename T0>
PX_NOINLINE CUresult launchKernel(CUfunction func, unsigned int numBlocks, unsigned int numThreads, unsigned int sharedMem, CUstream stream,
								  T0 v0)
{
	void* kernelParams[] =
	{
		&v0,
	};
	return cuLaunchKernel(func, numBlocks, 1, 1, numThreads, 1, 1, sharedMem, stream, kernelParams, NULL);
}

template <typename T0, typename T1>
PX_NOINLINE CUresult launchKernel(CUfunction func, unsigned int numBlocks, unsigned int numThreads, unsigned int sharedMem, CUstream stream,
								  T0 v0, T1 v1)
{
	void* kernelParams[] =
	{
		&v0, &v1,
	};
	return cuLaunchKernel(func, numBlocks, 1, 1, numThreads, 1, 1, sharedMem, stream, kernelParams, NULL);
}

template <typename T0, typename T1, typename T2>
PX_NOINLINE CUresult launchKernel(CUfunction func, unsigned int numBlocks, unsigned int numThreads, unsigned int sharedMem, CUstream stream,
								  T0 v0, T1 v1, T2 v2)
{
	void* kernelParams[] =
	{
		&v0, &v1, &v2,
	};
	return cuLaunchKernel(func, numBlocks, 1, 1, numThreads, 1, 1, sharedMem, stream, kernelParams, NULL);
}

template <typename T0, typename T1, typename T2, typename T3>
PX_NOINLINE CUresult launchKernel(CUfunction func, unsigned int numBlocks, unsigned int numThreads, unsigned int sharedMem, CUstream stream,
								  T0 v0, T1 v1, T2 v2, T3 v3)
{
	void* kernelParams[] =
	{
		&v0, &v1, &v2, &v3,
	};
	return cuLaunchKernel(func, numBlocks, 1, 1, numThreads, 1, 1, sharedMem, stream, kernelParams, NULL);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4>
PX_NOINLINE CUresult launchKernel(CUfunction func, unsigned int numBlocks, unsigned int numThreads, unsigned int sharedMem, CUstream stream,
								  T0 v0, T1 v1, T2 v2, T3 v3, T4 v4)
{
	void* kernelParams[] =
	{
		&v0, &v1, &v2, &v3, &v4,
	};
	return cuLaunchKernel(func, numBlocks, 1, 1, numThreads, 1, 1, sharedMem, stream, kernelParams, NULL);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
PX_NOINLINE CUresult launchKernel(CUfunction func, unsigned int numBlocks, unsigned int numThreads, unsigned int sharedMem, CUstream stream,
								  T0 v0, T1 v1, T2 v2, T3 v3, T4 v4, T5 v5)
{
	void* kernelParams[] =
	{
		&v0, &v1, &v2, &v3, &v4, &v5,
	};
	return cuLaunchKernel(func, numBlocks, 1, 1, numThreads, 1, 1, sharedMem, stream, kernelParams, NULL);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
PX_NOINLINE CUresult launchKernel(CUfunction func, unsigned int numBlocks, unsigned int numThreads, unsigned int sharedMem, CUstream stream,
								  T0 v0, T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6)
{
	void* kernelParams[] =
	{
		&v0, &v1, &v2, &v3, &v4, &v5, &v6,
	};
	return cuLaunchKernel(func, numBlocks, 1, 1, numThreads, 1, 1, sharedMem, stream, kernelParams, NULL);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
PX_NOINLINE CUresult launchKernel(CUfunction func, unsigned int numBlocks, unsigned int numThreads, unsigned int sharedMem, CUstream stream,
								  T0 v0, T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7)
{
	void* kernelParams[] =
	{
		&v0, &v1, &v2, &v3, &v4, &v5, &v6, &v7,
	};
	return cuLaunchKernel(func, numBlocks, 1, 1, numThreads, 1, 1, sharedMem, stream, kernelParams, NULL);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8>
PX_NOINLINE CUresult launchKernel(CUfunction func, unsigned int numBlocks, unsigned int numThreads, unsigned int sharedMem, CUstream stream,
								  T0 v0, T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8)
{
	void* kernelParams[] =
	{
		&v0, &v1, &v2, &v3, &v4, &v5, &v6, &v7, &v8,
	};
	return cuLaunchKernel(func, numBlocks, 1, 1, numThreads, 1, 1, sharedMem, stream, kernelParams, NULL);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9>
PX_NOINLINE CUresult launchKernel(CUfunction func, unsigned int numBlocks, unsigned int numThreads, unsigned int sharedMem, CUstream stream,
								  T0 v0, T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9)
{
	void* kernelParams[] =
	{
		&v0, &v1, &v2, &v3, &v4, &v5, &v6, &v7, &v8, &v9,
	};
	return cuLaunchKernel(func, numBlocks, 1, 1, numThreads, 1, 1, sharedMem, stream, kernelParams, NULL);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10>
PX_NOINLINE CUresult launchKernel(CUfunction func, unsigned int numBlocks, unsigned int numThreads, unsigned int sharedMem, CUstream stream,
								  T0 v0, T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10)
{
	void* kernelParams[] =
	{
		&v0, &v1, &v2, &v3, &v4, &v5, &v6, &v7, &v8, &v9, &v10,
	};
	return cuLaunchKernel(func, numBlocks, 1, 1, numThreads, 1, 1, sharedMem, stream, kernelParams, NULL);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename T11>
PX_NOINLINE CUresult launchKernel(CUfunction func, unsigned int numBlocks, unsigned int numThreads, unsigned int sharedMem, CUstream stream,
								  T0 v0, T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11)
{
	void* kernelParams[] =
	{
		&v0, &v1, &v2, &v3, &v4, &v5, &v6, &v7, &v8, &v9, &v10, &v11,
	};
	return cuLaunchKernel(func, numBlocks, 1, 1, numThreads, 1, 1, sharedMem, stream, kernelParams, NULL);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename T11, typename T12>
PX_NOINLINE CUresult launchKernel(CUfunction func, unsigned int numBlocks, unsigned int numThreads, unsigned int sharedMem, CUstream stream,
								  T0 v0, T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12)
{
	void* kernelParams[] =
	{
		&v0, &v1, &v2, &v3, &v4, &v5, &v6, &v7, &v8, &v9, &v10, &v11, &v12,
	};
	return cuLaunchKernel(func, numBlocks, 1, 1, numThreads, 1, 1, sharedMem, stream, kernelParams, NULL);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename T11, typename T12, typename T13>
PX_NOINLINE CUresult launchKernel(CUfunction func, unsigned int numBlocks, unsigned int numThreads, unsigned int sharedMem, CUstream stream,
								  T0 v0, T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12,
								  T13 v13)
{
	void* kernelParams[] =
	{
		&v0, &v1, &v2, &v3, &v4, &v5, &v6, &v7, &v8, &v9, &v10, &v11, &v12, &v13,
	};
	return cuLaunchKernel(func, numBlocks, 1, 1, numThreads, 1, 1, sharedMem, stream, kernelParams, NULL);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14>
PX_NOINLINE CUresult launchKernel(CUfunction func, unsigned int numBlocks, unsigned int numThreads, unsigned int sharedMem, CUstream stream,
								  T0 v0, T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12,
								  T13 v13, T14 v14)
{
	void* kernelParams[] =
	{
		&v0, &v1, &v2, &v3, &v4, &v5, &v6, &v7, &v8, &v9, &v10, &v11, &v12, &v13, &v14,
	};
	return cuLaunchKernel(func, numBlocks, 1, 1, numThreads, 1, 1, sharedMem, stream, kernelParams, NULL);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15>
PX_NOINLINE CUresult launchKernel(CUfunction func, unsigned int numBlocks, unsigned int numThreads, unsigned int sharedMem, CUstream stream,
								  T0 v0, T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12,
								  T13 v13, T14 v14, T15 v15)
{
	void* kernelParams[] =
	{
		&v0, &v1, &v2, &v3, &v4, &v5, &v6, &v7, &v8, &v9, &v10, &v11, &v12, &v13, &v14, &v15,
	};
	return cuLaunchKernel(func, numBlocks, 1, 1, numThreads, 1, 1, sharedMem, stream, kernelParams, NULL);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15,
          typename T16>
PX_NOINLINE CUresult launchKernel(CUfunction func, unsigned int numBlocks, unsigned int numThreads, unsigned int sharedMem, CUstream stream,
								  T0 v0, T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12,
								  T13 v13, T14 v14, T15 v15, T16 v16)
{
	void* kernelParams[] =
	{
		&v0, &v1, &v2, &v3, &v4, &v5, &v6, &v7, &v8, &v9, &v10, &v11, &v12, &v13, &v14, &v15, &v16,
	};
	return cuLaunchKernel(func, numBlocks, 1, 1, numThreads, 1, 1, sharedMem, stream, kernelParams, NULL);
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15,
          typename T16, typename T17>
PX_NOINLINE CUresult launchKernel(CUfunction func, unsigned int numBlocks, unsigned int numThreads, unsigned int sharedMem, CUstream stream,
								  T0 v0, T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12,
								  T13 v13, T14 v14, T15 v15, T16 v16, T17 v17)
{
	void* kernelParams[] =
	{
		&v0, &v1, &v2, &v3, &v4, &v5, &v6, &v7, &v8, &v9, &v10, &v11, &v12, &v13, &v14, &v15, &v16, &v17,
	};
	return cuLaunchKernel(func, numBlocks, 1, 1, numThreads, 1, 1, sharedMem, stream, kernelParams, NULL);
}

#endif

#endif
