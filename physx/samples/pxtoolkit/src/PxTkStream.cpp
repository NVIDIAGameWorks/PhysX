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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "PxTkStream.h"
#include "foundation/PxAssert.h"
#include "PxTkFile.h"
#include "PxTkNamespaceMangle.h"
#include "PsIntrinsics.h"
#include "foundation/PxMath.h"
#include "PxPhysics.h"
#include "geometry/PxConvexMesh.h"
#include "cooking/PxCooking.h"
#include "foundation/PxBounds3.h"
#include "extensions/PxDefaultStreams.h"

using namespace PxToolkit;
///////////////////////////////////////////////////////////////////////////////

PxTriangleMesh* PxToolkit::createTriangleMesh32(PxPhysics& physics, PxCooking& cooking, const PxVec3* verts, PxU32 vertCount, const PxU32* indices32, PxU32 triCount, bool insert)
{
	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count			= vertCount;
	meshDesc.points.stride			= sizeof(PxVec3);
	meshDesc.points.data			= verts;

	meshDesc.triangles.count		= triCount;
	meshDesc.triangles.stride		= 3*sizeof(PxU32);
	meshDesc.triangles.data			= indices32;

	if(!insert)
	{
		PxDefaultMemoryOutputStream writeBuffer;
		bool status = cooking.cookTriangleMesh(meshDesc, writeBuffer);
		if(!status)
			return NULL;

		PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
		return physics.createTriangleMesh(readBuffer);
	}
	else
	{
		return cooking.createTriangleMesh(meshDesc,physics.getPhysicsInsertionCallback());
	}
}

PxTriangleMesh* PxToolkit::createTriangleMesh32(PxPhysics& physics, PxCooking& cooking, PxTriangleMeshDesc* meshDesc, bool insert)
{
	if(!insert)
	{
		PxDefaultMemoryOutputStream writeBuffer;
		bool status = cooking.cookTriangleMesh(*meshDesc, writeBuffer);
		if(!status)
			return NULL;

		PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
		return physics.createTriangleMesh(readBuffer);
	}
	else
	{
		return cooking.createTriangleMesh(*meshDesc,physics.getPhysicsInsertionCallback());
	}
}

PxConvexMesh* PxToolkit::createConvexMesh(PxPhysics& physics, PxCooking& cooking, const PxVec3* verts, PxU32 vertCount, PxConvexFlags flags)
{
	PxConvexMeshDesc convexDesc;
	convexDesc.points.count		= vertCount;
	convexDesc.points.stride	= sizeof(PxVec3);
	convexDesc.points.data		= verts;
	convexDesc.flags			= flags;
	return cooking.createConvexMesh(convexDesc, physics.getPhysicsInsertionCallback());
}

PxConvexMesh* PxToolkit::createConvexMeshSafe(PxPhysics& physics, PxCooking& cooking, const PxVec3* verts, PxU32 vertCount, PxConvexFlags flags, PxU32 vLimit)
{
	PxConvexMeshDesc convexDesc;
	convexDesc.points.count			= vertCount;
	convexDesc.points.stride		= sizeof(PxVec3);
	convexDesc.points.data			= verts;
	convexDesc.flags				= flags;
	convexDesc.vertexLimit			= vLimit;

	PxDefaultMemoryOutputStream buf;
	bool retVal = cooking.cookConvexMesh(convexDesc, buf);	
	if(!retVal)
	{
		// create AABB
		PxBounds3 aabb;
		aabb.setEmpty();
		for (PxU32 i = 0; i < vertCount; i++)
		{
			aabb.include(verts[i]);
		}

		PxVec3  aabbVerts[8];
		aabbVerts[0] = PxVec3(aabb.minimum.x,aabb.minimum.y,aabb.minimum.z);
		aabbVerts[1] = PxVec3(aabb.maximum.x,aabb.minimum.y,aabb.minimum.z);
		aabbVerts[2] = PxVec3(aabb.maximum.x,aabb.maximum.y,aabb.minimum.z);
		aabbVerts[3] = PxVec3(aabb.minimum.x,aabb.maximum.y,aabb.minimum.z);

		aabbVerts[4] = PxVec3(aabb.minimum.x,aabb.minimum.y,aabb.maximum.z);
		aabbVerts[5] = PxVec3(aabb.maximum.x,aabb.minimum.y,aabb.maximum.z);
		aabbVerts[6] = PxVec3(aabb.maximum.x,aabb.maximum.y,aabb.maximum.z);
		aabbVerts[7] = PxVec3(aabb.minimum.x,aabb.maximum.y,aabb.maximum.z);

		convexDesc.points.count			= 8;
		convexDesc.points.stride		= sizeof(PxVec3);
		convexDesc.points.data			= &aabbVerts[0];
		convexDesc.flags				= flags;

		retVal = cooking.cookConvexMesh(convexDesc, buf);
	}

	if(!retVal)
	{
		return NULL;
	}

	PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
	return physics.createConvexMesh(input);
}


