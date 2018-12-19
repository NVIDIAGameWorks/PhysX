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



#ifndef RAND_STATE_H
#define RAND_STATE_H

// This is shared by legacy IOFX and shaders

namespace nvidia
{
namespace apex
{

struct LCG_PRNG
{
	unsigned int a, c;

	PX_CUDA_CALLABLE PX_INLINE LCG_PRNG()
	{
	}
	PX_CUDA_CALLABLE PX_INLINE LCG_PRNG(unsigned int a, unsigned int c)
	{
		this->a = a;
		this->c = c;
	}

	static PX_CUDA_CALLABLE PX_INLINE LCG_PRNG getIdentity()
	{
		return LCG_PRNG(1, 0);
	}

	static PX_CUDA_CALLABLE PX_INLINE LCG_PRNG getDefault()
	{
		return LCG_PRNG(1103515245u, 12345u);
	}

	PX_CUDA_CALLABLE PX_INLINE LCG_PRNG& operator *= (const LCG_PRNG& rhs)
	{
		a *= rhs.a;
		c *= rhs.a; c += rhs.c;
		return *this;
	}

	PX_CUDA_CALLABLE PX_INLINE LCG_PRNG leapFrog(unsigned int leap) const
	{
		LCG_PRNG ret = getIdentity();
		for (unsigned int i = 0; i < leap; ++i)
		{
			ret *= (*this);
		}
		return ret;
	}

	PX_CUDA_CALLABLE PX_INLINE unsigned int operator()(unsigned int x) const
	{
		return x * a + c;
	}
};

struct RandState
{
	explicit PX_CUDA_CALLABLE PX_INLINE RandState(unsigned int seed)
	{
		curr = seed;
	}

	PX_CUDA_CALLABLE PX_INLINE unsigned int next()
	{
		return (curr = LCG_PRNG::getDefault()(curr));
	}

	PX_CUDA_CALLABLE PX_INLINE float nextFloat()
	{
		return float(next()) * 0.00000000023283064365386962890625f;
	}
	PX_CUDA_CALLABLE PX_INLINE float nextFloat(float min, float max)
	{
		return min + nextFloat() * (max - min);
	}

private:
	unsigned int curr;
};

// For CUDA PRNG
struct PRNGInfo
{
	unsigned int* g_stateSpawnSeed;
	nvidia::LCG_PRNG* g_randBlock;
	unsigned int seed;
	nvidia::LCG_PRNG randThread;
	nvidia::LCG_PRNG randGrid;
};

// For CUDA PRNG: device part
#ifdef __CUDACC__
//*
#if __CUDA_ARCH__ >= 300
#define RAND_SCAN_OP(ofs) \
	{ \
		unsigned int a = aData[scanIdx], c = cData[scanIdx]; \
		unsigned int aOfs = __shfl_up(a, ofs), cOfs = __shfl_up(c, ofs); \
		if (idxInWarp >= ofs) { a = a * aOfs; c = c * aOfs + cOfs; } \
		aData[scanIdx] = a; cData[scanIdx] = c; \
	}
#else
#define RAND_SCAN_OP(ofs) \
	{ \
		unsigned int a = aData[scanIdx], c = cData[scanIdx]; \
		unsigned int aOfs = aData[scanIdx - ofs], cOfs = cData[scanIdx - ofs]; \
		aData[scanIdx] = a * aOfs; cData[scanIdx] = c * aOfs + cOfs; \
	}
#endif
/*/
//THIS CODE CRASH ON CUDA 5.0.35
#define RAND_SCAN_OP(ofs) \
	{ \
		nvidia::LCG_PRNG val(aData[scanIdx], cData[scanIdx]); \
		nvidia::LCG_PRNG valOfs(aData[scanIdx - ofs], cData[scanIdx - ofs]); \
		val *= valOfs; \
		aData[scanIdx] = val.a; cData[scanIdx] = val.c; \
	}
//*/
PX_INLINE __device__ void randScanWarp(unsigned int scanIdx, volatile unsigned int* aData, volatile unsigned int* cData, unsigned int idxInWarp)
{
	RAND_SCAN_OP(1);
	RAND_SCAN_OP(2);
	RAND_SCAN_OP(4);
	RAND_SCAN_OP(8);
	RAND_SCAN_OP(16);
}

PX_INLINE __device__ nvidia::LCG_PRNG randScanBlock(nvidia::LCG_PRNG val, volatile unsigned int* aData, volatile unsigned int* cData)
{
	const unsigned int idx = threadIdx.x;
	const unsigned int idxInWarp = idx & (WARP_SIZE-1);
	const unsigned int warpIdx = (idx >> LOG2_WARP_SIZE);

	//setup scan
	unsigned int scanIdx = (warpIdx << (LOG2_WARP_SIZE + 1)) + idxInWarp;
	//write identity
	aData[scanIdx] = 1;
	cData[scanIdx] = 0;

	scanIdx += WARP_SIZE;
	//write value
	aData[scanIdx] = val.a;
	cData[scanIdx] = val.c;

	randScanWarp(scanIdx, aData, cData, idxInWarp);

	//read value
	val.a = aData[scanIdx]; 
	val.c = cData[scanIdx]; 

	__syncthreads();

	if (idxInWarp == WARP_SIZE-1)
	{
		const unsigned int idxWrite = warpIdx + WARP_SIZE;
		aData[idxWrite] = val.a;
		cData[idxWrite] = val.c;
	}
	__syncthreads();

	if (warpIdx == 0)
	{
		randScanWarp(scanIdx, aData, cData, idxInWarp);
	}
	__syncthreads();

	if (warpIdx > 0)
	{
		const unsigned int idxRead = warpIdx + WARP_SIZE - 1;
		const nvidia::LCG_PRNG valWarp(aData[idxRead], cData[idxRead]);
		val *= valWarp;
	}
	return val;
}

#endif

}
} // nvidia::apex::

#endif