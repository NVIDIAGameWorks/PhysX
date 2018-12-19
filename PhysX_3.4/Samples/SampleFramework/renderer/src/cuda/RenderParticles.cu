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

#include "PxPhysics.h"
#include "PxVec4.h"
#include "PxVec3.h"
#include "PxVec2.h"
#include "PxMat33.h"
#include "PxStrideIterator.h"

namespace physx
{

template <typename T>
__device__ T* ptrOffset(T* p, PxU32 byteOffset)
{
	return (T*)((unsigned char*)(p) + byteOffset);
}

#if __CUDA_ARCH__ < 200
__device__ PxU32 gOffset;
#else
__device__ __shared__ PxU32 gOffset;
#endif


// copies orientations and positions to the destination vertex
// buffer based on the validityBitmap state
extern "C" __global__ void updateInstancedVB(
	PxVec3* destPositions,
	PxVec3* destRotation0,
	PxVec3* destRotation1,
	PxVec3* destRotation2,
	PxU32 destStride,
	const PxVec4* srcPositions,
	const PxMat33* srcRotations,
	const PxU32* validParticleBitmap,
	PxU32 validParticleRange)
{
	if (!threadIdx.x)
		gOffset = 0;

	__syncthreads();

	if (validParticleRange)
	{
		for (PxU32 w=threadIdx.x; w <= (validParticleRange) >> 5; w+=blockDim.x)
		{
			const PxU32 srcBaseIndex = w << 5;

			// reserve space in the output vertex buffer based on
			// population count of validity bitmap (avoids excess atomic ops)
			PxU32 destIndex = atomicAdd(&gOffset, __popc(validParticleBitmap[w]));
			
			for (PxU32 b=validParticleBitmap[w]; b; b &= b-1) 
			{
				const PxU32 index = srcBaseIndex | __ffs(b)-1;

				const PxU32 offset = destIndex*destStride;

				*ptrOffset(destRotation0, offset) = srcRotations[index].column0;
				*ptrOffset(destRotation1, offset) = srcRotations[index].column1;
				*ptrOffset(destRotation2, offset) = srcRotations[index].column2;

				PxVec3* p = ptrOffset(destPositions, offset);
				p->x = srcPositions[index].x;
				p->y = srcPositions[index].y;
				p->z = srcPositions[index].z;

				++destIndex;
			}
		}
	}
}


// copies positions and alpha to the destination vertex buffer based on 
// validity bitmap and particle life times
extern "C" __global__ void updateBillboardVB(
	PxVec3* destPositions,
	PxU8* destAlphas,
	PxU32 destStride,
	PxF32 fadingPeriod,
	const PxVec4* srcPositions, 
	const PxReal* srcLifetimes,
	const PxU32* validParticleBitmap,
	PxU32 validParticleRange)
{
	if (!threadIdx.x)
		gOffset = 0;

	__syncthreads();

	if (validParticleRange)
	{
		for (PxU32 w=threadIdx.x; w <= (validParticleRange) >> 5; w+=blockDim.x)
		{
			const PxU32 srcBaseIndex = w << 5;

			// reserve space in the output vertex buffer based on
			// population count of validity bitmap (avoids excess atomic ops)
			PxU32 destIndex = atomicAdd(&gOffset, __popc(validParticleBitmap[w]));

			for (PxU32 b=validParticleBitmap[w]; b; b &= b-1) 
			{
				PxU32 index = srcBaseIndex | __ffs(b)-1;

				const PxU32 offset = destIndex*destStride;

				// copy position
				PxVec3* p = ptrOffset(destPositions, offset);
				p->x = srcPositions[index].x;
				p->y = srcPositions[index].y;
				p->z = srcPositions[index].z;

				// update alpha
				if (srcLifetimes)
				{
					PxU8 lifetime = 0;
					if(srcLifetimes[index] >= fadingPeriod)
						lifetime = 255;
					else
					{
						if(srcLifetimes[index] <= 0.0f)
							lifetime = 0; 
						else
							lifetime = static_cast<PxU8>(srcLifetimes[index] * 255 / fadingPeriod);
					}

					destAlphas[destIndex*4] = lifetime;
				}

				++destIndex;
			}
		}
	}
}

}