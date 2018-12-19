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


#ifndef __SAMPLE_SPHERE_ACTOR_H__
#define __SAMPLE_SPHERE_ACTOR_H__

#include "SampleShapeActor.h"
#include "RendererCapsuleShape.h"

#include "PxRigidDynamic.h"
#include "geometry/PxSphereGeometry.h"
#include "extensions/PxExtensionsAPI.h"
namespace physx
{
class PxMaterial;
}

#include <Renderer.h>
#include <RendererMeshContext.h>


class SampleSphereActor : public SampleShapeActor
{
public:
	SampleSphereActor(SampleRenderer::Renderer* renderer,
	                  SampleFramework::SampleMaterialAsset& material,
	                  physx::PxScene& physxScene,
	                  const physx::PxVec3& pos,
	                  const physx::PxVec3& vel,
	                  const physx::PxVec3& radius,
	                  float density,
	                  physx::PxMaterial* PxMaterial,
	                  bool useGroupsMask,
	                  nvidia::apex::RenderDebugInterface* rdebug = NULL)
		: SampleShapeActor(rdebug)
		, mRendererCapsuleShape(NULL)
		, mRadius(radius)
	{
		mRenderer = renderer;
		if (!PxMaterial)
			physxScene.getPhysics().getMaterials(&PxMaterial, 1);
		createActor(physxScene, pos, vel, mRadius, density, PxMaterial, useGroupsMask);

		mRendererCapsuleShape = new SampleRenderer::RendererCapsuleShape(*mRenderer, 0, radius.x);

		mRendererMeshContext.material         = material.getMaterial();
		mRendererMeshContext.materialInstance = material.getMaterialInstance();
		mRendererMeshContext.mesh             = mRendererCapsuleShape->getMesh();
		mRendererMeshContext.transform        = &mTransform;

		if (rdebug)
		{
			mBlockId = RENDER_DEBUG_IFACE(rdebug)->beginDrawGroup(mTransform);
			RENDER_DEBUG_IFACE(rdebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::SolidShaded);
			static uint32_t scount /* = 0 */;
			RENDER_DEBUG_IFACE(rdebug)->setCurrentColor(0xFFFFFF);
			RENDER_DEBUG_IFACE(rdebug)->setCurrentTextScale(0.5f);
			RENDER_DEBUG_IFACE(rdebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::CenterText);
			RENDER_DEBUG_IFACE(rdebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::CameraFacing);
			RENDER_DEBUG_IFACE(rdebug)->debugText(physx::PxVec3(0, 1.0f + 0.01f, 0), "Sample Sphere:%d", scount++);
			RENDER_DEBUG_IFACE(rdebug)->endDrawGroup();
		}
	}

	virtual ~SampleSphereActor()
	{
		if (mRendererCapsuleShape)
		{
			delete mRendererCapsuleShape;
			mRendererCapsuleShape = NULL;
		}
	}

private:
	void createActor(physx::PxScene& physxScene,
	                 const physx::PxVec3& pos,
	                 const physx::PxVec3& vel,
	                 const physx::PxVec3& extents,
	                 float density,
	                 physx::PxMaterial* PxMaterial,
	                 bool useGroupsMask)
	{
		mTransform = physx::PxMat44(physx::PxIdentity);
		mTransform.setPosition(pos);

		physx::PxRigidDynamic* actor = physxScene.getPhysics().createRigidDynamic(physx::PxTransform(mTransform));
		PX_ASSERT(actor);
		actor->setAngularDamping(0.5f);
		actor->setLinearVelocity(vel);

		physx::PxSphereGeometry sphereGeom(extents.x);
		physx::PxShape* shape = actor->createShape(sphereGeom, *PxMaterial);
		PX_ASSERT(shape);
		if (shape && useGroupsMask)
		{
			shape->setSimulationFilterData(physx::PxFilterData(1, 0, ~0u, 0));
			shape->setQueryFilterData(physx::PxFilterData(1, 0, ~0u, 0));
		}

		if (density > 0)
		{
			physx::PxRigidBodyExt::updateMassAndInertia(*actor, density);
		}
		else
		{
			actor->setMass(1.0f);
		}
		SCOPED_PHYSX_LOCK_WRITE(&physxScene);
		physxScene.addActor(*actor);
		mPhysxActor = actor;
	}

private:
	SampleRenderer::RendererCapsuleShape* mRendererCapsuleShape;
	physx::PxVec3 mRadius;
};

#endif
