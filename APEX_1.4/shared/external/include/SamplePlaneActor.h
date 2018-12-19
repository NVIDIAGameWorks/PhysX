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


#ifndef __SAMPLE_PLANE_ACTOR_H__
#define __SAMPLE_PLANE_ACTOR_H__

#include "SampleShapeActor.h"

#include "PxScene.h"
#include "PxRigidStatic.h"
#include "geometry/PxPlaneGeometry.h"
namespace physx
{
class PxMaterial;
}

#include <PsMathUtils.h>

#include <Renderer.h>
#include <RendererMeshContext.h>
#include <RendererGridShape.h>


class SamplePlaneActor : public SampleShapeActor
{
public:
	SamplePlaneActor(SampleRenderer::Renderer* renderer,
	                 SampleFramework::SampleMaterialAsset& material,
	                 physx::PxScene& physxScene,
	                 const physx::PxVec3& pos,
	                 const physx::PxVec3& normal,
	                 physx::PxMaterial* PxMaterial,
	                 bool useGroupsMask,
	                 nvidia::apex::RenderDebugInterface* rdebug = NULL)
		: SampleShapeActor(rdebug)
		, mRendererGridShape(NULL)
	{
		mRenderer = renderer;
		if (!PxMaterial)
			physxScene.getPhysics().getMaterials(&PxMaterial, 1);
		createActor(physxScene, pos, normal, PxMaterial, useGroupsMask);

		// default is X_UP
		mRendererGridShape = new SampleRenderer::RendererGridShape(*mRenderer, 10, 1.0f, false, SampleRenderer::RendererGridShape::UP_X);

		mRendererMeshContext.material         = material.getMaterial();
		mRendererMeshContext.materialInstance = material.getMaterialInstance();
		mRendererMeshContext.mesh             = mRendererGridShape->getMesh();
		mRendererMeshContext.transform        = &mTransform;

		if (rdebug)
		{
			mBlockId = RENDER_DEBUG_IFACE(rdebug)->beginDrawGroup(mTransform);
			RENDER_DEBUG_IFACE(rdebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::SolidShaded);
			static uint32_t pcount /* = 0 */;
			RENDER_DEBUG_IFACE(rdebug)->setCurrentColor(0xFFFFFF);
			RENDER_DEBUG_IFACE(rdebug)->setCurrentTextScale(0.5f);
			RENDER_DEBUG_IFACE(rdebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::CenterText);
			RENDER_DEBUG_IFACE(rdebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::CameraFacing);
			RENDER_DEBUG_IFACE(rdebug)->debugText(physx::PxVec3(0, 1.0f + 0.01f, 0), "Sample Plane:%d", pcount++);
			RENDER_DEBUG_IFACE(rdebug)->endDrawGroup();
		}
	}

	virtual ~SamplePlaneActor()
	{
		if (mRendererGridShape)
		{
			delete mRendererGridShape;
			mRendererGridShape = NULL;
		}
	}

private:
	void createActor(physx::PxScene& physxScene,
	                 const physx::PxVec3& pos,
	                 const physx::PxVec3& normal,
	                 physx::PxMaterial* PxMaterial,
	                 bool useGroupsMask)
	{
		//physx::PxMat33 m33(nvidia::rotFrom2Vectors(physx::PxVec3(0, 1, 0), normal));
		physx::PxMat33 m33(physx::shdfnd::rotFrom2Vectors(physx::PxVec3(1, 0, 0), normal));
		mTransform = physx::PxMat44(m33, pos);

		//physx::PxTransform pose(pos, physx::PxQuat(nvidia::rotFrom2Vectors(physx::PxVec3(1, 0, 0), normal)));
		physx::PxTransform pose	= physx::PxTransform(mTransform);
		pose.q.normalize();
		physx::PxRigidStatic* actor = physxScene.getPhysics().createRigidStatic(pose);
		PX_ASSERT(actor);

		physx::PxPlaneGeometry planeGeom;
		physx::PxShape* shape = actor->createShape(planeGeom, *PxMaterial);
		PX_ASSERT(shape);
		if (shape && useGroupsMask)
		{
			shape->setSimulationFilterData(physx::PxFilterData(1, 0, ~0u, 0));
			shape->setQueryFilterData(physx::PxFilterData(1, 0, ~0u, 0));
		}
		SCOPED_PHYSX_LOCK_WRITE(&physxScene);
		physxScene.addActor(*actor);
		mPhysxActor = actor;
	}

private:
	SampleRenderer::RendererGridShape* mRendererGridShape;
};

#endif
