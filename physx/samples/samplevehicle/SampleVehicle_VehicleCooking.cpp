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

#include "SampleVehicle_VehicleCooking.h"
#include "PxTkStream.h"
#include "extensions/PxDefaultStreams.h"


PxConvexMesh* createConvexMesh(const PxVec3* verts, const PxU32 numVerts, PxPhysics& physics, PxCooking& cooking)
{
	// Create descriptor for convex mesh
	PxConvexMeshDesc convexDesc;
	convexDesc.points.count			= numVerts;
	convexDesc.points.stride		= sizeof(PxVec3);
	convexDesc.points.data			= verts;
	convexDesc.flags				= PxConvexFlag::eCOMPUTE_CONVEX;

	PxConvexMesh* convexMesh = NULL;
	PxDefaultMemoryOutputStream buf;
	if(cooking.cookConvexMesh(convexDesc, buf))
	{
		PxDefaultMemoryInputData id(buf.getData(), buf.getSize());
		convexMesh = physics.createConvexMesh(id);
	}

	return convexMesh;
}

PxConvexMesh* createCuboidConvexMesh(const PxVec3& halfExtents, PxPhysics& physics, PxCooking& cooking)
{
	PxVec3 verts[8]=
	{
		PxVec3(-halfExtents.x, -halfExtents.y, -halfExtents.z),
		PxVec3(-halfExtents.x, -halfExtents.y, +halfExtents.z),
		PxVec3(-halfExtents.x, +halfExtents.y, -halfExtents.z),
		PxVec3(-halfExtents.x, +halfExtents.y, +halfExtents.z),
		PxVec3(+halfExtents.x, -halfExtents.y, -halfExtents.z),
		PxVec3(+halfExtents.x, -halfExtents.y, +halfExtents.z),
		PxVec3(+halfExtents.x, +halfExtents.y, -halfExtents.z),
		PxVec3(+halfExtents.x, +halfExtents.y, +halfExtents.z)
	};
	PxU32 numVerts=8;

	return createConvexMesh(verts,numVerts,physics,cooking);
}

PxConvexMesh* createWedgeConvexMesh(const PxVec3& halfExtents, PxPhysics& physics, PxCooking& cooking)
{
	PxVec3 verts[6]=
	{
		PxVec3(-halfExtents.x, -halfExtents.y, -halfExtents.z),
		PxVec3(-halfExtents.x, -halfExtents.y, +halfExtents.z),
		PxVec3(-halfExtents.x, +halfExtents.y, -halfExtents.z),
		PxVec3(+halfExtents.x, -halfExtents.y, -halfExtents.z),
		PxVec3(+halfExtents.x, -halfExtents.y, +halfExtents.z),
		PxVec3(+halfExtents.x, +halfExtents.y, -halfExtents.z)
	};	
	PxU32 numVerts=6;

	return createConvexMesh(verts,numVerts,physics,cooking);
}

PxConvexMesh* createCylinderConvexMesh(const PxF32 width, const PxF32 radius, const PxU32 numCirclePoints, PxPhysics& physics, PxCooking& cooking)
{
#define  MAX_NUM_VERTS_IN_CIRCLE 16
	PX_ASSERT(numCirclePoints<MAX_NUM_VERTS_IN_CIRCLE);
	PxVec3 verts[2*MAX_NUM_VERTS_IN_CIRCLE];
	PxU32 numVerts=2*numCirclePoints;
	const PxF32 dtheta=2*PxPi/(1.0f*numCirclePoints);
	for(PxU32 i=0;i<MAX_NUM_VERTS_IN_CIRCLE;i++)
	{
		const PxF32 theta=dtheta*i;
		const PxF32 cosTheta=radius*PxCos(theta);
		const PxF32 sinTheta=radius*PxSin(theta);
		verts[2*i+0]=PxVec3(-0.5f*width, cosTheta, sinTheta);
		verts[2*i+1]=PxVec3(+0.5f*width, cosTheta, sinTheta);
	}

	return createConvexMesh(verts,numVerts,physics,cooking);
}

PxConvexMesh* createSquashedCuboidMesh(const PxF32 baseLength, const PxF32 baseDepth, const PxF32 height1, const PxF32 height2, PxPhysics& physics, PxCooking& cooking)
{
	const PxF32 x=baseLength*0.5f;
	const PxF32 z=baseDepth*0.5f;
	PxVec3 verts[8]=
	{
		PxVec3(-x,-0.5f*height1,-z),
		PxVec3(-x,-0.5f*height1,+z),
		PxVec3(+x,-0.5f*height1,-z),
		PxVec3(+x,-0.5f*height1,+z),
		PxVec3(-x,-0.5f*height1+height2,-z),
		PxVec3(-x,+0.5f*height1,+z),
		PxVec3(+x,-0.5f*height1+height2,-z),
		PxVec3(+x,+0.5f*height1,+z)
	};
	PxU32 numVerts=8;

	return createConvexMesh(verts,numVerts,physics,cooking);
}


PxConvexMesh* createPrismConvexMesh(const PxF32 baseLength, const PxF32 baseDepth, const PxF32 height, PxPhysics& physics, PxCooking& cooking)
{
	const PxF32 x=baseLength*0.5f;
	const PxF32 z=baseDepth*0.5f;

	PxVec3 verts[6]=
	{
		PxVec3(-x, 0, -z),
		PxVec3(-x, 0, +z),
		PxVec3(+x, 0, -z),
		PxVec3(+x, 0, +z),
		PxVec3(-x, height, 0),
		PxVec3(+x, height, 0),
	};	
	PxU32 numVerts=6;

	return createConvexMesh(verts,numVerts,physics,cooking);
}

PxConvexMesh* createChassisConvexMesh(const PxVec3* verts, const PxU32 numVerts, PxPhysics& physics, PxCooking& cooking)
{
	return createConvexMesh(verts,numVerts,physics,cooking);
}

PxConvexMesh* createWheelConvexMesh(const PxVec3* verts, const PxU32 numVerts, PxPhysics& physics, PxCooking& cooking)
{
	//Extract the wheel radius and width from the aabb of the wheel convex mesh.
	PxVec3 wheelMin(PX_MAX_F32,PX_MAX_F32,PX_MAX_F32);
	PxVec3 wheelMax(-PX_MAX_F32,-PX_MAX_F32,-PX_MAX_F32);
	for(PxU32 i=0;i<numVerts;i++)
	{
		wheelMin.x=PxMin(wheelMin.x,verts[i].x);
		wheelMin.y=PxMin(wheelMin.y,verts[i].y);
		wheelMin.z=PxMin(wheelMin.z,verts[i].z);
		wheelMax.x=PxMax(wheelMax.x,verts[i].x);
		wheelMax.y=PxMax(wheelMax.y,verts[i].y);
		wheelMax.z=PxMax(wheelMax.z,verts[i].z);
	}
	const PxF32 wheelWidth=wheelMax.x-wheelMin.x;
	const PxF32 wheelRadius=PxMax(wheelMax.y,wheelMax.z);

	return createCylinderConvexMesh(wheelWidth,wheelRadius,8,physics,cooking);
}
