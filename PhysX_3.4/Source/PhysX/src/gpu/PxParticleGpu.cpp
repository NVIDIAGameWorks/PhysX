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
  

#include "PxParticleGpu.h"

#if PX_SUPPORT_GPU_PHYSX

#include "NpPhysics.h"
#include "NpPhysicsGpu.h"

using namespace physx;

//-------------------------------------------------------------------------------------------------------------------//

bool PxParticleGpu::createTriangleMeshMirror(const class PxTriangleMesh& triangleMesh, PxCudaContextManager& contextManager)
{
	return NpPhysics::getInstance().getNpPhysicsGpu().createTriangleMeshMirror(triangleMesh, contextManager);
}

void PxParticleGpu::releaseTriangleMeshMirror(const class PxTriangleMesh& triangleMesh, PxCudaContextManager* contextManager)
{
	NpPhysics::getInstance().getNpPhysicsGpu().releaseTriangleMeshMirror(triangleMesh, contextManager);
}

bool PxParticleGpu::createHeightFieldMirror(const class PxHeightField& heightField, PxCudaContextManager& contextManager)
{
	return NpPhysics::getInstance().getNpPhysicsGpu().createHeightFieldMirror(heightField, contextManager);
}

void PxParticleGpu::releaseHeightFieldMirror(const class PxHeightField& heightField, PxCudaContextManager* contextManager)
{
	NpPhysics::getInstance().getNpPhysicsGpu().releaseHeightFieldMirror(heightField, contextManager);
}

bool PxParticleGpu::createConvexMeshMirror(const class PxConvexMesh& convexMesh, PxCudaContextManager& contextManager)
{
	return NpPhysics::getInstance().getNpPhysicsGpu().createConvexMeshMirror(convexMesh, contextManager);
}

void PxParticleGpu::releaseConvexMeshMirror(const class PxConvexMesh& convexMesh, PxCudaContextManager* contextManager)
{
	NpPhysics::getInstance().getNpPhysicsGpu().releaseConvexMeshMirror(convexMesh, contextManager);
}

void PxParticleGpu::setExplicitCudaFlushCountHint(const class PxScene& scene, PxU32 cudaFlushCount)
{
	NpPhysics::getInstance().getNpPhysicsGpu().setExplicitCudaFlushCountHint(scene, cudaFlushCount);
}

bool PxParticleGpu::setTriangleMeshCacheSizeHint(const class PxScene& scene, PxU32 size)
{
	return NpPhysics::getInstance().getNpPhysicsGpu().setTriangleMeshCacheSizeHint(scene, size);
}

PxTriangleMeshCacheStatistics PxParticleGpu::getTriangleMeshCacheStatistics(const class PxScene& scene)
{
	return NpPhysics::getInstance().getNpPhysicsGpu().getTriangleMeshCacheStatistics(scene);
}

//-------------------------------------------------------------------------------------------------------------------//

#endif // PX_SUPPORT_GPU_PHYSX
