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



#include "RTdef.h"
#if RT_COMPILE
#include <PsUserAllocated.h>

#include "DestructibleActorImpl.h"

#include "Actor.h"
#include "Compound.h"
#include "Convex.h"
#include "CompoundCreator.h"
#include "Delaunay2d.h"
#include "Delaunay3d.h"
#include "PolygonTriangulator.h"
#include "IslandDetector.h"
#include "MeshClipper.h"
#include "FracturePattern.h"

#include "SimScene.h"

namespace nvidia
{
namespace fracture
{

void SimScene::onWake(PxActor** actors, uint32_t count){ 
	if(mAppNotify != NULL)
		mAppNotify->onWake(actors,count);
	
	for(uint32_t i = 0; i < count; i++)
	{
		PxActor* actor = actors[i];
		if(actor != NULL && actor->is<physx::PxRigidDynamic>())
		{
			uint32_t shapeCount = actor->is<physx::PxRigidDynamic>()->getNbShapes();
			PxShape** shapes = (PxShape**)PX_ALLOC(sizeof(PxShape*)*shapeCount,"onWake Shapes Temp Buffer");
			actor->is<physx::PxRigidDynamic>()->getShapes(shapes,sizeof(PxShape*)*shapeCount,0);
			::nvidia::destructible::DestructibleActorImpl* prevActor = NULL;
			for(uint32_t j = 0; j < shapeCount; j++)
			{
				nvidia::fracture::base::Convex* convex = findConvexForShape(*shapes[j]);
				if(convex == NULL || convex->getParent() == NULL)
					continue;
				nvidia::fracture::Compound* parent = (nvidia::fracture::Compound*)convex->getParent();
				::nvidia::destructible::DestructibleActorImpl* curActor = parent->getDestructibleActor();
				if(convex && convex->getParent() && curActor != prevActor && curActor)
				{
					curActor->incrementWakeCount();
					prevActor = curActor;
				}
				
			}
			PX_FREE(shapes);
		}
	}
}

void SimScene::onSleep(PxActor** actors, uint32_t count){ 
	if(mAppNotify != NULL)
		mAppNotify->onSleep(actors,count);
	
	for(uint32_t i = 0; i < count; i++)
	{
		PxActor* actor = actors[i];
		if(actor != NULL && actor->is<physx::PxRigidDynamic>())
		{
			uint32_t shapeCount = actor->is<physx::PxRigidDynamic>()->getNbShapes();
			PxShape** shapes = (PxShape**)PX_ALLOC(sizeof(PxShape*)*shapeCount,"onSleep Shapes Temp Buffer");
			actor->is<physx::PxRigidDynamic>()->getShapes(shapes,sizeof(PxShape*)*shapeCount,0);
			::nvidia::destructible::DestructibleActorImpl* prevActor = NULL;
			for(uint32_t j = 0; j < shapeCount; j++)
			{
				nvidia::fracture::base::Convex* convex = findConvexForShape(*shapes[j]);
				if(convex == NULL || convex->getParent() == NULL)
					continue;
				nvidia::fracture::Compound* parent = (nvidia::fracture::Compound*)convex->getParent();
				::nvidia::destructible::DestructibleActorImpl* curActor = parent->getDestructibleActor();
				if(convex && convex->getParent() && curActor != prevActor && curActor)
				{
					curActor->decrementWakeCount();;
					prevActor = curActor;
				}
				
			}
			PX_FREE(shapes);
		}
	}
}
SimScene* SimScene::createSimScene(PxPhysics *pxPhysics, PxCooking *pxCooking, PxScene *scene, float minConvexSize, PxMaterial* defaultMat, const char *resourcePath)
{
	SimScene* s = PX_NEW(SimScene)(pxPhysics,pxCooking,scene,minConvexSize,defaultMat,resourcePath);
	s->createSingletons();
	return s;
}

void SimScene::createSingletons()
{
	mCompoundCreator = PX_NEW(CompoundCreator)(this);
	mDelaunay2d = PX_NEW(Delaunay2d)(this);
	mDelaunay3d = PX_NEW(Delaunay3d)(this);
	mPolygonTriangulator = PX_NEW(PolygonTriangulator)(this);
	mIslandDetector = PX_NEW(IslandDetector)(this);
	mMeshClipper = PX_NEW(MeshClipper)(this);
	mDefaultGlass = PX_NEW(FracturePattern)(this);
	mDefaultGlass->createGlass(10.0f,0.25f,30,0.3f,0.03f,1.4f,0.3f);
	//mDefaultGlass->create3dVoronoi(PxVec3(10.0f, 10.0f, 10.0f), 50, 10.0f);
	addActor(createActor(NULL));
}

base::Actor* SimScene::createActor(::nvidia::destructible::DestructibleActorImpl* actor)
{
	return (base::Actor*)PX_NEW(Actor)(this,actor);
}

base::Convex* SimScene::createConvex()
{
	return (base::Convex*)PX_NEW(Convex)(this);
}

base::Compound* SimScene::createCompound(const base::FracturePattern *pattern, const base::FracturePattern *secondaryPattern, float contactOffset, float restOffset)
{
	return (base::Compound*)PX_NEW(Compound)(this,pattern,secondaryPattern,contactOffset,restOffset);
}

base::FracturePattern* SimScene::createFracturePattern()
{
	return (base::FracturePattern*)PX_NEW(FracturePattern)(this);
}

SimScene::SimScene(PxPhysics *pxPhysics, PxCooking *pxCooking, PxScene *scene, float minConvexSize, PxMaterial* defaultMat, const char *resourcePath):
		base::SimScene(pxPhysics,pxCooking,scene,minConvexSize,defaultMat,resourcePath) 
{
}

// --------------------------------------------------------------------------------------------
SimScene::~SimScene()
{
	PX_DELETE(mDefaultGlass);

	mDefaultGlass = NULL;
}

}
}
#endif