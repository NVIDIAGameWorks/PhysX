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

// ****************************************************************************
// This snippet illustrates how to use PhysX clothing.
//
// It creates a square cloth mesh, cooks a fabric from the mesh, adds virtual 
// particles and shows how to set up a ground plane, a sphere,
// a capsule and two tetrahedra as collision shapes.
// ****************************************************************************

#include <ctype.h>

#include "PxPhysicsAPI.h"

#include "../SnippetUtils/SnippetUtils.h"
#include "../SnippetCommon/SnippetPrint.h"
#include "../SnippetCommon/SnippetPVD.h"


using namespace physx;

PxDefaultAllocator		 gAllocator;
PxDefaultErrorCallback	 gErrorCallback;

PxFoundation*			 gFoundation = NULL;
PxPhysics*				 gPhysics	= NULL;

PxDefaultCpuDispatcher*	 gDispatcher = NULL;
PxScene*				 gScene		= NULL;

PxCloth*				 gCloth      = NULL;
PxPvd*                   gPvd        = NULL;

// two spheres for a tapered capsule and one by itself
PxClothCollisionSphere   gSpheres[3] = {
	PxClothCollisionSphere(PxVec3( 0.0f, 0.3f, 1.0f), 0.3f),
	PxClothCollisionSphere(PxVec3(-0.5f, 0.2f, 0.0f), 0.2f),
	PxClothCollisionSphere(PxVec3(-1.5f, 0.2f, 0.0f), 0.2f)
};

// a tetrahedron
PxClothCollisionTriangle gTriangles[4] = {
	PxClothCollisionTriangle(PxVec3(-0.3f, 0.0f, -1.3f), PxVec3( 0.3f, 0.6f, -1.3f), PxVec3( 0.3f, 0.0f, -0.7f)),
	PxClothCollisionTriangle(PxVec3( 0.3f, 0.6f, -1.3f), PxVec3(-0.3f, 0.6f, -0.7f), PxVec3( 0.3f, 0.0f, -0.7f)),
	PxClothCollisionTriangle(PxVec3(-0.3f, 0.6f, -0.7f), PxVec3(-0.3f, 0.0f, -1.3f), PxVec3( 0.3f, 0.0f, -0.7f)),
	PxClothCollisionTriangle(PxVec3(-0.3f, 0.0f, -1.3f), PxVec3(-0.3f, 0.6f, -0.7f), PxVec3( 0.3f, 0.6f, -1.3f)),
};

PxPlane gPlanes[5] = {
	PxPlane(PxVec3(  0.0f,  1.0f,  0.0f),  0.0f),
	PxPlane(PxVec3( -1.0f, -1.0f,  0.0f),  1.0f),
	PxPlane(PxVec3(  1.0f, -1.0f,  0.0f), -1.0f),
	PxPlane(PxVec3(  0.0f,  1.0f, -1.0f), -0.5f),
	PxPlane(PxVec3(  0.0f,  1.0f,  1.0f), -0.5f),
};

PxTransform gPose = PxTransform(PxVec3(0), PxQuat(PxPi/4, PxVec3(0, 1, 0))) * PxTransform(PxVec3(1, 0.5f, 0));

void createPhysicsAndScene()
{
	gFoundation = PxCreateFoundation(PX_FOUNDATION_VERSION, gAllocator, gErrorCallback);
	gPvd = PxCreatePvd(*gFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	gPvd->connect(*transport,PxPvdInstrumentationFlag::eALL);

	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(),true,gPvd);

	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	
	PxU32 numCores = SnippetUtils::getNbPhysicalCores();
	gDispatcher = PxDefaultCpuDispatcherCreate(numCores == 0 ? 0 : numCores - 1);
	sceneDesc.cpuDispatcher	= gDispatcher;
	sceneDesc.filterShader	= PxDefaultSimulationFilterShader;
	gScene = gPhysics->createScene(sceneDesc);

}

void createCloth()
{
	// create regular mesh
	PxU32 resolution = 10;
	PxU32 numParticles = resolution*resolution;
	PxU32 numQuads = (resolution-1)*(resolution-1);

	// create particles
	PxClothParticle* particles = new PxClothParticle[numParticles];
	PxVec3 center(0.5f, 0.3f, 0.0f);
	PxVec3 delta = 1.0f/(resolution-1) * PxVec3(1.0f, 0.8f, 0.6f);
	PxClothParticle* pIt = particles;
	for(PxU32 i=0; i<resolution; ++i)
	{
		for(PxU32 j=0; j<resolution; ++j, ++pIt)
		{
			pIt->invWeight = j+1<resolution ? 1.0f : 0.0f;
			pIt->pos = delta.multiply(PxVec3(PxReal(i), 
				PxReal(j), -PxReal(j))) - center;
		}
	}

	// create quads
	PxU32* quads = new PxU32[4*numQuads];
	PxU32* iIt = quads;
	for(PxU32 i=0; i<resolution-1; ++i)
	{
		for(PxU32 j=0; j<resolution-1; ++j)
		{
			*iIt++ = (i+0)*resolution + (j+0);
			*iIt++ = (i+1)*resolution + (j+0);
			*iIt++ = (i+1)*resolution + (j+1);
			*iIt++ = (i+0)*resolution + (j+1);
		}
	}

	// create fabric from mesh
	PxClothMeshDesc meshDesc;
	meshDesc.points.count = numParticles;
	meshDesc.points.stride = sizeof(PxClothParticle);
	meshDesc.points.data = particles;
	meshDesc.invMasses.count = numParticles;
	meshDesc.invMasses.stride = sizeof(PxClothParticle);
	meshDesc.invMasses.data = &particles->invWeight;
	meshDesc.quads.count = numQuads;
	meshDesc.quads.stride = 4*sizeof(PxU32);
	meshDesc.quads.data = quads;

	// cook fabric
	PxClothFabric* fabric = PxClothFabricCreate(*gPhysics, meshDesc, PxVec3(0, 1, 0));

	delete[] quads;

	// create cloth and add to scene
	gCloth = gPhysics->createCloth(gPose, *fabric, particles, PxClothFlags(0));
	gScene->addActor(*gCloth);

	fabric->release();
	delete[] particles;

	// 240 iterations per/second (4 per-60hz frame)
	gCloth->setSolverFrequency(240.0f);

	// add virtual particles
	PxU32* indices = new PxU32[4*4*numQuads];
	PxU32* vIt = indices;
	for(PxU32 i=0; i<resolution-1; ++i)
	{
		for(PxU32 j=0; j<resolution-1; ++j)
		{
			*vIt++ = i*resolution + j;
			*vIt++ = i*resolution + (j+1);
			*vIt++ = (i+1)*resolution + (j+1);
			*vIt++ = 0;

			*vIt++ = i*resolution + (j+1);
			*vIt++ = (i+1)*resolution + (j+1);
			*vIt++ = (i+1)*resolution + j;
			*vIt++ = 0;

			*vIt++ = (i+1)*resolution + (j+1);
			*vIt++ = (i+1)*resolution + j;
			*vIt++ = i*resolution + j;
			*vIt++ = 0;

			*vIt++ = (i+1)*resolution + j;
			*vIt++ = i*resolution + j;
			*vIt++ = i*resolution + (j+1);
			*vIt++ = 0;
		}
	}

	// barycentric weights specifying virtual particle position
	PxVec3 weights = PxVec3(0.6, 0.2, 0.2);
	gCloth->setVirtualParticles(4*numQuads, indices, 1, &weights);

	delete[] indices;

	// add capsule (referencing spheres[0] and spheres[1] added later)
	gCloth->addCollisionCapsule(1, 2);

	// add ground plane and tetrahedron convex (referencing planes added later)
	gCloth->addCollisionConvex(0x01);
	gCloth->addCollisionConvex(0x1e);
}

void initPhysics()
{
	createPhysicsAndScene();
	createCloth();
}

void stepPhysics()
{
	// update the cloth local frame
	gPose = PxTransform(PxVec3(0), PxQuat(PxPi/240, PxVec3(0, 1, 0))) * gPose;
	gCloth->setTargetPose(gPose);

	// transform colliders into local cloth space
	PxTransform invPose = gPose.getInverse();

	// 1 sphere plus 1 capsule made from two spheres
	PxClothCollisionSphere spheres[3];
	for(int i=0; i<3; ++i)
	{
		spheres[i].pos = invPose.transform(gSpheres[i].pos);
		spheres[i].radius = gSpheres[i].radius;
	}
	gCloth->setCollisionSpheres(spheres, 3);

	// tetrahedron made from 4 triangles
	PxClothCollisionTriangle triangles[4];
	for(int i=0; i<4; ++i)
	{
		triangles[i].vertex0 = invPose.transform(gTriangles[i].vertex0);
		triangles[i].vertex1 = invPose.transform(gTriangles[i].vertex1);
		triangles[i].vertex2 = invPose.transform(gTriangles[i].vertex2);
	}
	gCloth->setCollisionTriangles(triangles, 4);

	// tetrahedron made from 4 planes
	PxPlane planes[5];
	for(int i=0; i<5; ++i)
	{
		planes[i] = invPose.transform(gPlanes[i]);
	}
	gCloth->setCollisionPlanes(reinterpret_cast<const PxClothCollisionPlane*>(planes), 5);

	gScene->simulate(1.0f/60.0f);
	gScene->fetchResults(true);
}
	
void cleanupPhysics()
{
	gCloth->release();
	gScene->release();
	gDispatcher->release();
	gPhysics->release();
	PxPvdTransport* transport = gPvd->getTransport();
	gPvd->release();
	transport->release();
	
	gFoundation->release();
	
	printf("SnippetCloth done.\n");
}

int snippetMain(int, const char*const*)
{
	initPhysics();

	for(PxU32 i=0; i<480; ++i)
		stepPhysics();

	cleanupPhysics();

	return 0;
}
