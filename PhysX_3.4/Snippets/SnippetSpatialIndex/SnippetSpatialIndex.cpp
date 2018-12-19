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
// This snippet illustrates simple use of the PxSpatialIndex data structure
//
// Note that spatial index has been marked as deprecated and will be removed 
// in future releases
//
// We create a number of spheres, and raycast against them in a random direction 
// from the origin. When a raycast hits a sphere, we teleport it to a random 
// location
// ****************************************************************************

#include <ctype.h>

#include "PxPhysicsAPI.h"

#include "../SnippetCommon/SnippetPrint.h"
#include "../SnippetUtils/SnippetUtils.h"

using namespace physx;

PxDefaultAllocator		gAllocator;
PxDefaultErrorCallback	gErrorCallback;
PxFoundation*			gFoundation = NULL;

static const PxU32 SPHERE_COUNT = 500;

float rand01()
{
	return float(rand())/RAND_MAX;
}

struct Sphere : public PxSpatialIndexItem
{
	Sphere() 
	{
		radius = 1;
		resetPosition();
	}

	Sphere(PxReal r)
	: radius(r)
	{
		resetPosition();
	}

	void resetPosition()
	{
		do
		{
			position = PxVec3(rand01()-0.5f, rand01()-0.5f, rand01()-0.5f)*(5+rand01()*5);
		}
		while (position.normalize()==0.0f);
	}

	PxBounds3 getBounds()
	{
		// Geometry queries err on the side of reporting positive results in the face of floating point inaccuracies. 
		// To ensure that a geometry query only reports true when the bounding boxes in the BVH overlap, use 
		// getWorldBounds, which has a third parameter that scales the bounds slightly (default is scaling by 1.01f)

		return PxGeometryQuery::getWorldBounds(PxSphereGeometry(radius), PxTransform(position));
	}

	PxVec3 position;
	PxReal radius;
	PxSpatialIndexItemId id;
};

Sphere					gSpheres[SPHERE_COUNT];
PxSpatialIndex*			gBvh;

void init()
{
	gFoundation = PxCreateFoundation(PX_FOUNDATION_VERSION, gAllocator, gErrorCallback);
	gBvh = PxCreateSpatialIndex();

	// insert the spheres into the BVH, recording the ID so we can later update them
	for(PxU32 i=0;i<SPHERE_COUNT;i++)
		gSpheres[i].id = gBvh->insert(gSpheres[i], gSpheres[i].getBounds());

	// force a full rebuild of the BVH
	gBvh->rebuildFull();

	// hint that should rebuild over the course of approximately 20 rebuildStep() calls
	gBvh->setIncrementalRebuildRate(20);
}


struct HitCallback : public PxSpatialLocationCallback
{
	HitCallback(const PxVec3 p, const PxVec3& d): position(p), direction(d), closest(FLT_MAX), hitSphere(NULL) {}
	PxAgain onHit(PxSpatialIndexItem& item, PxReal distance, PxReal& shrunkDistance)
	{
		PX_UNUSED(distance);

		Sphere& s = static_cast<Sphere&>(item);
		PxRaycastHit hitData;

		// the ray hit the sphere's AABB, now we do a ray-sphere intersection test to find out if the ray hit the sphere

		PxU32 hit = PxGeometryQuery::raycast(position, direction, 
											 PxSphereGeometry(s.radius), PxTransform(s.position),
											 1e6, PxHitFlag::eDEFAULT,
											 1, &hitData);

		// if the raycast hit and it's closer than what we had before, shrink the maximum length of the raycast

		if(hit && hitData.distance < closest)
		{
			closest = hitData.distance;
			hitSphere = &s;
			shrunkDistance = hitData.distance;
		}

		// and continue the query

		return true;
	}

	PxVec3 position, direction;
	PxReal closest;
	Sphere* hitSphere;
};

void step()
{
	for(PxU32 hits=0; hits<10;)
	{
		// raycast in a random direction from the origin, and teleport the closest sphere we find

		PxVec3 dir = PxVec3(rand01()-0.5f, rand01()-0.5f, rand01()-0.5f).getNormalized();
		HitCallback callback(PxVec3(0), dir);
		gBvh->raycast(PxVec3(0), dir, FLT_MAX, callback);
		Sphere* hit = callback.hitSphere;
		if(hit)
		{
			hit->resetPosition();
			gBvh->update(hit->id, hit->getBounds());
			hits++;
		}
	}

	// run an incremental rebuild step in the background
	gBvh->rebuildStep();

}

	
void cleanup()
{
	gBvh->release();
	gFoundation->release();
	
	printf("SnippetSpatialIndex done.\n");
}

int snippetMain(int, const char*const*)
{
	static const PxU32 frameCount = 100;
	init();
	for(PxU32 i=0; i<frameCount; i++)
		step();
	cleanup();

	return 0;
}
