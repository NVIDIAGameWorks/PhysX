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

#include "PxScene.h"
#include "PxQueryReport.h"
#include "PxBatchQueryDesc.h"
#include "extensions/PxJoint.h"
#include "PxRigidDynamic.h"
#include "extensions/PxDistanceJoint.h"
#include "extensions/PxSphericalJoint.h"
#include "PxArticulationLink.h"
#include "PxShape.h"
#include "Picking.h"
#include "RendererMemoryMacros.h"

#if PX_UNIX_FAMILY
#include <stdio.h>
#endif

using namespace physx;	// PT: please DO NOT indent the whole file

Picking::Picking(PhysXSample& frame) :
	mSelectedActor		(NULL),
	mMouseJoint			(NULL),
	mMouseActor			(NULL),
	mMouseActorToDelete	(NULL),
	mDistanceToPicked	(0.0f),
	mMouseScreenX		(0),
	mMouseScreenY		(0),
	mFrame				(frame)
{
}
Picking::~Picking() {}

bool Picking::isPicked() const
{
	return mMouseJoint!=0;
}

void Picking::moveCursor(PxI32 x, PxI32 y)
{
	mMouseScreenX = x;
	mMouseScreenY = y;
}

/*void Picking::moveCursor(PxReal deltaDepth)
{
	const PxReal range[2] = { 0.0f, 1.0f };

	const PxReal r = (range[1] - range[0]);
	const PxReal d = (mMouseDepth - range[0])/r;
	const PxReal delta = deltaDepth*0.02f*(1.0f - d);

	mMouseDepth = PxClamp(mMouseDepth + delta, range[0], range[1]); 
}*/

void Picking::tick()
{
	if(mMouseJoint)
		moveActor(mMouseScreenX,mMouseScreenY);

	// PT: delete mouse actor one frame later to avoid crashes
	SAFE_RELEASE(mMouseActorToDelete);
}

void Picking::computeCameraRay(PxVec3& orig, PxVec3& dir, PxI32 x, PxI32 y) const
{
	const PxVec3& camPos = mFrame.getCamera().getPos();

	// compute picking ray
//	const PxVec3 rayOrig = unProject(x, y, 0.0f);	// PT: what the frell is that?
	const PxVec3 rayOrig = camPos;
	const PxVec3 rayDir = (unProject(x, y, 1.0f) - rayOrig).getNormalized();

	orig = rayOrig;
	dir = rayDir;
}

bool Picking::pick(int x, int y)
{
	PxScene& scene = mFrame.getActiveScene();

	PxVec3 rayOrig, rayDir;
	computeCameraRay(rayOrig, rayDir, x, y);

	// raycast rigid bodies in scene
	PxRaycastHit hit; hit.shape = NULL;
	PxRaycastBuffer hit1;
	scene.raycast(rayOrig, rayDir, PX_MAX_F32, hit1, PxHitFlag::ePOSITION);
	hit = hit1.block;

	if(hit.shape)
	{ 
		const char* shapeName = hit.shape->getName();
		if(shapeName)
			shdfnd::printFormatted("Picked shape name: %s\n", shapeName);

		PxRigidActor* actor = hit.actor;
		PX_ASSERT(actor);
		mSelectedActor = static_cast<PxRigidActor*>(actor->is<PxRigidDynamic>());
		if(!mSelectedActor)
			mSelectedActor = static_cast<PxRigidActor*>(actor->is<PxArticulationLink>());

		//ML::this is very useful to debug some collision problem
		PxTransform t = actor->getGlobalPose();
		PX_UNUSED(t);
		shdfnd::printFormatted("id = %i\n PxTransform transform(PxVec3(%f, %f, %f), PxQuat(%f, %f, %f, %f))\n", reinterpret_cast<size_t>(actor->userData), t.p.x, t.p.y, t.p.z, t.q.x, t.q.y, t.q.z, t.q.w);
	}
	else
		mSelectedActor = 0;

	if(mSelectedActor)
	{
		shdfnd::printFormatted("Actor '%s' picked! (userData: %p)\n", mSelectedActor->getName(), mSelectedActor->userData);

		//if its a dynamic rigid body, joint it for dragging purposes:
		grabActor(hit.position, rayOrig);
	}

#ifdef VISUALIZE_PICKING_RAYS
	Ray ray;
	ray.origin = rayOrig;
	ray.dir = rayDir;
	mRays.push_back(ray);
#endif
	return true;
}


//----------------------------------------------------------------------------//

PxActor* Picking::letGo()
{
	// let go any picked actor
	if(mMouseJoint)
	{
        mMouseJoint->release(); 
        mMouseJoint = NULL;
       
        //	SAFE_RELEASE(mMouseActor);			// PT: releasing immediately crashes
        PX_ASSERT(!mMouseActorToDelete);
		mMouseActorToDelete = mMouseActor;	// PT: instead, we mark for deletion next frame
	}

	PxActor* returnedActor = mSelectedActor;
	
	mSelectedActor = NULL;

	return returnedActor;
}

//----------------------------------------------------------------------------//

void Picking::grabActor(const PxVec3& worldImpact, const PxVec3& rayOrigin)
{
	if(!mSelectedActor 
		|| (mSelectedActor->getType() != PxActorType::eRIGID_DYNAMIC 
		&& mSelectedActor->getType() != PxActorType::eARTICULATION_LINK))
		return;

	PxScene& scene = mFrame.getActiveScene();
	PxPhysics& physics = scene.getPhysics();

	//create a shape less actor for the mouse
	{
		mMouseActor = physics.createRigidDynamic(PxTransform(worldImpact, PxQuat(PxIdentity)));
		mMouseActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true); 
		mMouseActor->setMass(1.0f);
		mMouseActor->setMassSpaceInertiaTensor(PxVec3(1.0f, 1.0f, 1.0f));

		scene.addActor(*mMouseActor);
	}
	PxRigidActor* pickedActor = static_cast<PxRigidActor*>(mSelectedActor);

#if USE_D6_JOINT_FOR_MOUSE
	mMouseJoint = PxD6JointCreate(		physics,
										mMouseActor,
										PxTransform(PxIdentity),
										pickedActor,
										PxTransform(pickedActor->getGlobalPose().transformInv(worldImpact)));
	mMouseJoint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
	mMouseJoint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);
	mMouseJoint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
#elif USE_SPHERICAL_JOINT_FOR_MOUSE
	mMouseJoint = PxSphericalJointCreate(physics,
										mMouseActor,
										PxTransform(PxIdentity),
										pickedActor,
										PxTransform(pickedActor->getGlobalPose().transformInv(worldImpact)));
#else
	mMouseJoint = PxDistanceJointCreate(physics, 
										mMouseActor, 
										PxTransform(PxIdentity),
										pickedActor,
										PxTransform(pickedActor->getGlobalPose().transformInv(worldImpact)));
	mMouseJoint->setMaxDistance(0.0f);
	mMouseJoint->setMinDistance(0.0f);
	mMouseJoint->setDistanceJointFlags(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED);
#endif

	mDistanceToPicked = (worldImpact - rayOrigin).magnitude();
}

//----------------------------------------------------------------------------//

void Picking::moveActor(int x, int y)
{
	if(!mMouseActor) 
		return; 

	PxVec3 rayOrig, rayDir;
	computeCameraRay(rayOrig, rayDir, x, y);

	const PxVec3 pos = rayOrig + mDistanceToPicked * rayDir;

	mMouseActor->setKinematicTarget(PxTransform(pos, PxQuat(PxIdentity)));
}

//----------------------------------------------------------------------------//

PxVec3 Picking::unProject(int x, int y, float depth) const
{
	SampleRenderer::Renderer* renderer = mFrame.getRenderer();
	const SampleRenderer::RendererProjection& projection = mFrame.getCamera().getProjMatrix();
	const PxTransform view = mFrame.getCamera().getViewMatrix().getInverse();

	PxU32 windowWidth  = 0;
	PxU32 windowHeight = 0;
	renderer->getWindowSize(windowWidth, windowHeight);
	
	const PxF32 outX = (float)x / (float)windowWidth;
	const PxF32 outY = (float)y / (float)windowHeight;

	return SampleRenderer::unproject(projection, view, outX * 2 -1, outY * 2 -1, depth * 2 - 1);
}

//----------------------------------------------------------------------------//

void Picking::project(const physx::PxVec3& v, int& xi, int& yi, float& depth) const
{
	SampleRenderer::Renderer* renderer = mFrame.getRenderer();
	SampleRenderer::RendererProjection	projection = mFrame.getCamera().getProjMatrix();
	const PxTransform view = mFrame.getCamera().getViewMatrix().getInverse();

	PxVec3 pos = SampleRenderer::project(projection, view, v);
	///* Map x, y and z to range 0-1 */
	pos.x = (pos.x + 1 ) * 0.5f;
	pos.y = (pos.y + 1 ) * 0.5f;
	pos.z = (pos.z + 1 ) * 0.5f;
	
	PxU32 windowWidth  = 0;
	PxU32 windowHeight = 0;
	renderer->getWindowSize(windowWidth, windowHeight);

	/* Map x,y to viewport */
	pos.x *= windowWidth;
	pos.y *= windowHeight; 
	
	depth = (float)pos.z;

	xi = (int)(pos.x + 0.5); 
	yi = (int)(pos.y + 0.5); 
}
