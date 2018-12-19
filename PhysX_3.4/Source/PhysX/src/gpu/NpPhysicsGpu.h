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


#ifndef PX_PHYSICS_NP_PHYSICS_GPU
#define PX_PHYSICS_NP_PHYSICS_GPU

#include "PxPhysXCommonConfig.h"
#include "PxPhysXConfig.h"

#if PX_SUPPORT_GPU_PHYSX

#include "CmPhysXCommon.h"
#include "PsArray.h"

namespace physx
{
	class PxCudaContextManager;
}

namespace physx
{

	struct PxTriangleMeshCacheStatistics;
	
	class NpPhysicsGpu
	{
	public:

		bool createTriangleMeshMirror(const class PxTriangleMesh& triangleMesh, physx::PxCudaContextManager& contextManager);
		void releaseTriangleMeshMirror(const class PxTriangleMesh& triangleMesh, physx::PxCudaContextManager* contextManager = NULL);
		
		bool createHeightFieldMirror(const class PxHeightField& heightField, physx::PxCudaContextManager& contextManager);
		void releaseHeightFieldMirror(const class PxHeightField& heightField, physx::PxCudaContextManager* contextManager = NULL);
		
		bool createConvexMeshMirror(const class PxConvexMesh& convexMesh, physx::PxCudaContextManager& contextManager);
		void releaseConvexMeshMirror(const class PxConvexMesh& convexMesh, physx::PxCudaContextManager* contextManager = NULL);

		void setExplicitCudaFlushCountHint(const class PxScene& scene, PxU32 cudaFlushCount);
		bool setTriangleMeshCacheSizeHint(const class PxScene& scene, PxU32 size);
		PxTriangleMeshCacheStatistics getTriangleMeshCacheStatistics(const class PxScene& scene);
	
		NpPhysicsGpu(class NpPhysics& npPhysics);
		virtual ~NpPhysicsGpu();
	
	private:

		NpPhysicsGpu& operator=(const NpPhysicsGpu&);

		struct MeshMirror 
		{
			PxU32 mirrorHandle;
			physx::PxCudaContextManager* ctx;
			const void* meshAddress;
		};
	
		Ps::Array<MeshMirror>::Iterator findMeshMirror(const void* meshAddress);
		Ps::Array<MeshMirror>::Iterator findMeshMirror(const void* meshAddress, physx::PxCudaContextManager& ctx);
		class PxPhysXGpu* getPhysXGpu(bool createIfNeeded, const char* functionName, bool doWarn = true);
		bool checkNewMirror(const void* meshPtr, physx::PxCudaContextManager& ctx, const char* functionName);
		bool checkMirrorExists(Ps::Array<MeshMirror>::Iterator it, const char* functionName);
		bool checkMirrorHandle(PxU32 mirrorHandle, const char* functionName);
		void addMeshMirror(const void* meshPtr, PxU32 mirrorHandle, physx::PxCudaContextManager& ctx);
		void removeMeshMirror(const void* meshPtr, PxU32 mirrorHandle);
		void releaseMeshMirror(const void* meshPtr, const char* functionName, physx::PxCudaContextManager* ctx);
	
		Ps::Array<MeshMirror> mMeshMirrors;
		class NpPhysics& mNpPhysics;
	};

}

#endif
#endif

