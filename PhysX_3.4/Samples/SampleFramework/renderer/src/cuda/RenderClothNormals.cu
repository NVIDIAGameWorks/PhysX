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

#include "cloth/PxClothTypes.h"

namespace physx
{

// interleaved format must match that used by RendererClothShape
struct Vertex
{
	PxVec3 position;
	PxVec3 normal;
};

namespace
{
	__device__ inline void PxAtomicFloatAdd(float* dest, float x)
	{
#if __CUDA_ARCH__ >= 200
		atomicAdd(dest, x);
#else
		union bits { float f; unsigned int i; };
		bits oldVal, newVal;

		do
		{
			// emulate atomic float add on 1.1 arch
			oldVal.f = *dest;
			newVal.f = oldVal.f + x;
		}
		while (atomicCAS((unsigned int*)dest, oldVal.i, newVal.i) != oldVal.i);
#endif
	}


	__device__ void PxAtomicVec3Add(PxVec3& dest, PxVec3 inc)
	{
		PxAtomicFloatAdd(&dest.x, inc.x);
		PxAtomicFloatAdd(&dest.y, inc.y);
		PxAtomicFloatAdd(&dest.z, inc.z);
	}
}

extern "C" __global__ void computeSmoothNormals(
	const PxClothParticle* particles,
	const PxU16* indices,
	Vertex* vertices,
	PxU32 numTris,
	PxU32 numParticles)
{
	// zero old normals
	for (PxU32 i=threadIdx.x; i < numParticles; i += blockDim.x)
		vertices[i].normal = PxVec3(0.0f);

	__syncthreads();

	for (PxU32 i=threadIdx.x; i < numTris; i += blockDim.x)
	{
		PxU16 a = indices[i*3];
		PxU16 b = indices[i*3+1];
		PxU16 c = indices[i*3+2];

		// calculate face normal
		PxVec3 e1 = particles[b].pos-particles[a].pos;
		PxVec3 e2 = particles[c].pos-particles[a].pos;
		PxVec3 n = e2.cross(e1);

		PxAtomicVec3Add(vertices[a].normal, n);
		PxAtomicVec3Add(vertices[b].normal, n);
		PxAtomicVec3Add(vertices[c].normal, n);
	}

	__syncthreads();

	// update vertex buffer
	for (PxU32 i=threadIdx.x; i < numParticles; i += blockDim.x)
	{
		vertices[i].position = particles[i].pos;
		vertices[i].normal = vertices[i].normal.getNormalized();
	}	
}

}