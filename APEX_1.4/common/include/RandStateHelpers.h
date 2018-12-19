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



#ifndef RAND_STATE_HELPERS_H
#define RAND_STATE_HELPERS_H

#include "PxTask.h"
#include "ApexMirroredArray.h"

namespace nvidia
{
namespace apex
{

// For CUDA PRNG: host part
PX_INLINE void InitDevicePRNGs(
    SceneIntl& scene,
    unsigned int blockSize,
    LCG_PRNG& threadLeap,
    LCG_PRNG& gridLeap,
    ApexMirroredArray<LCG_PRNG>& blockPRNGs)
{
	threadLeap = LCG_PRNG::getDefault().leapFrog(16);

	LCG_PRNG randBlock = LCG_PRNG::getIdentity();
	LCG_PRNG randBlockLeap = threadLeap.leapFrog(blockSize);

	const uint32_t numBlocks = 32; //Max Multiprocessor count
	blockPRNGs.setSize(numBlocks, ApexMirroredPlace::CPU_GPU);
	for (uint32_t i = 0; i < numBlocks; ++i)
	{
		blockPRNGs[i] = randBlock;
		randBlock *= randBlockLeap;
	}
	gridLeap = randBlock;

	{
		PxTaskManager* tm = scene.getTaskManager();
		PxCudaContextManager* ctx = tm->getGpuDispatcher()->getCudaContextManager();

		PxScopedCudaLock s(*ctx);

		PxGpuCopyDesc desc;
		blockPRNGs.copyHostToDeviceDesc(desc, 0, 0);
		tm->getGpuDispatcher()->launchCopyKernel(&desc, 1, 0);
	}
}

}
} // nvidia::apex::

#endif
