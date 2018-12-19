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


#ifndef __SAMPLE_CONVEX_MESH_ACTOR_H__
#define __SAMPLE_CONVEX_MESH_ACTOR_H__

#include "SampleShapeActor.h"
#include "RendererMeshShape.h"
#include "PsMemoryBuffer.h"
#include "PsArray.h"
#include "PxStreamFromFileBuf.h"
#include "ApexSDK.h"

#include "cooking/PxCooking.h"
#include "cooking/PxConvexMeshDesc.h"
#include "geometry/PxConvexMeshGeometry.h"

#include "PxRigidDynamic.h"
#include "PxRigidStatic.h"
#include "extensions/PxExtensionsAPI.h"



namespace physx
{
class PxMaterial;
}

#include "RenderDebugInterface.h"
#include <Renderer.h>
#include <RendererMeshContext.h>

typedef physx::PxConvexMesh ConvexMesh;

class SampleConvexMeshActor : public SampleShapeActor
{
public:
	SampleConvexMeshActor(SampleRenderer::Renderer* renderer,
	               SampleFramework::SampleMaterialAsset& material,
	               physx::PxScene& physxScene,
				   physx::PxCooking& cooking,
				   const physx::PxVec3* verts, 
				   const uint32_t nbVerts,
	               const physx::PxVec3& pos,
	               const physx::PxVec3& vel,
	               float density,
	               physx::PxMaterial* PxMaterial,
	               bool useGroupsMask,
	               nvidia::apex::RenderDebugInterface* rdebug = NULL)
		: SampleShapeActor(rdebug)
		, mVerts(NULL)
		, mNormals(NULL)
		, mUvs(NULL)
		, mFaces(NULL)
		, mNbVerts(0)
		, mNbFaces(0)
		, mConvexMesh(NULL)
		, mRendererMeshShape(NULL)
	{
		mRenderer = renderer;

		createActor(physxScene, cooking, verts, nbVerts, pos, vel, density, PxMaterial, useGroupsMask);

		const uint32_t   nbPolygons		= mConvexMesh->getNbPolygons();
		const uint8_t*   indexBuffer	= mConvexMesh->getIndexBuffer();
		const physx::PxVec3* vertices		= mConvexMesh->getVertices();

		for (uint32_t i = 0; i < nbPolygons; i++)
		{
			physx::PxHullPolygon data;
			bool status = mConvexMesh->getPolygonData(i, data);
			PX_ASSERT(status);
			PX_UNUSED(status);

			uint32_t nbPolyVerts = data.mNbVerts;
			mNbVerts += nbPolyVerts;
			mNbFaces += (nbPolyVerts - 2)*3;
		}

		mVerts		= new physx::PxVec3[mNbVerts];
		mNormals	= new physx::PxVec3[mNbVerts];
		mFaces		= new uint16_t[mNbFaces];

		uint32_t vertCounter = 0;
		uint32_t facesCounter = 0;
		for (uint32_t i = 0; i < nbPolygons; i++)
		{
			physx::PxHullPolygon data;
			bool status = mConvexMesh->getPolygonData(i, data);
			PX_ASSERT(status);
			PX_UNUSED(status);

			physx::PxVec3 normal(data.mPlane[0], data.mPlane[1], data.mPlane[2]);

			uint32_t vI0 = vertCounter;
			for (uint32_t vI = 0; vI < data.mNbVerts; vI++)
			{
				mVerts[vertCounter] = vertices[indexBuffer[data.mIndexBase + vI]];
				mNormals[vertCounter] = normal;
				vertCounter++;
			}

			for (uint32_t vI = 1; vI < uint32_t(data.mNbVerts) - 1; vI++)
			{
				mFaces[facesCounter++]	= uint16_t(vI0);
				mFaces[facesCounter++]	= uint16_t(vI0 + vI + 1);
				mFaces[facesCounter++]	= uint16_t(vI0 + vI);
			}
		}

		mRendererMeshShape = new SampleRenderer::RendererMeshShape(*mRenderer, mVerts, mNbVerts, mNormals, mUvs, mFaces, mNbFaces / 3);

		mRendererMeshContext.material         = material.getMaterial();
		mRendererMeshContext.materialInstance = material.getMaterialInstance();
		mRendererMeshContext.mesh             = mRendererMeshShape->getMesh();
		mRendererMeshContext.transform        = &mTransform;

		if (rdebug)
		{
			mBlockId = RENDER_DEBUG_IFACE(rdebug)->beginDrawGroup(mTransform);
			RENDER_DEBUG_IFACE(rdebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::SolidShaded);
			static uint32_t bcount /* = 0 */;
			RENDER_DEBUG_IFACE(rdebug)->setCurrentColor(0xFFFFFF);
			RENDER_DEBUG_IFACE(rdebug)->setCurrentTextScale(0.5f);
			RENDER_DEBUG_IFACE(rdebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::CenterText);
			RENDER_DEBUG_IFACE(rdebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::CameraFacing);
			RENDER_DEBUG_IFACE(rdebug)->debugText(physx::PxVec3(0, 1.0f + 0.01f, 0), "Sample Convex Mesh:%d", bcount++);
			RENDER_DEBUG_IFACE(rdebug)->endDrawGroup();
		}
	}

	virtual ~SampleConvexMeshActor()
	{
		if (mRendererMeshShape)
		{
			delete[] mVerts;
			delete[] mNormals;

			delete mRendererMeshShape;
			mRendererMeshShape = NULL;
		}
	}

private:

	void createActor(physx::PxScene& physxScene,
					 physx::PxCooking& cooking,
					 const physx::PxVec3* verts,
					 const uint32_t nbVerts,
	                 const physx::PxVec3& pos,
	                 const physx::PxVec3& vel,
	                 float density,
	                 physx::PxMaterial* PxMaterial,
					 bool useGroupsMask) 
	{
		if (!PxMaterial)
		{
			physxScene.getPhysics().getMaterials(&PxMaterial, 1);
		}

		mTransform = physx::PxMat44(physx::PxIdentity);
		mTransform.setPosition(pos);

		physx::PxRigidActor* actor = NULL;
		if (density > 0)
		{
			actor = physxScene.getPhysics().createRigidDynamic(physx::PxTransform(mTransform));
			(static_cast<physx::PxRigidDynamic*>(actor))->setAngularDamping(0.5f); // Is it correct?
			(static_cast<physx::PxRigidDynamic*>(actor))->setLinearVelocity(vel);
		}
		else
		{
			actor = physxScene.getPhysics().createRigidStatic(physx::PxTransform(mTransform));
		}		
		PX_ASSERT(actor);

		physx::PxConvexMeshDesc convexMeshDesc;
		convexMeshDesc.points.count = nbVerts;
		convexMeshDesc.points.data = verts;
		convexMeshDesc.points.stride = sizeof(physx::PxVec3);
		convexMeshDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

		physx::PsMemoryBuffer stream;
		stream.setEndianMode(physx::PxFileBuf::ENDIAN_NONE);
		nvidia::apex::PxStreamFromFileBuf nvs(stream);

		if (cooking.cookConvexMesh(convexMeshDesc, nvs))
		{
			mConvexMesh = physxScene.getPhysics().createConvexMesh(nvs);
			PX_ASSERT(mConvexMesh);
		}

		physx::PxConvexMeshGeometry convexMeshGeom(mConvexMesh);
		physx::PxShape* shape = actor->createShape(convexMeshGeom, *PxMaterial);
		PX_ASSERT(shape);
		if (shape && useGroupsMask)
		{
			shape->setSimulationFilterData(physx::PxFilterData(1, 0, ~0u, 0));
			shape->setQueryFilterData(physx::PxFilterData(1, 0, ~0u, 0));
		}

		if (density > 0)
		{
			physx::PxRigidBodyExt::updateMassAndInertia(*(static_cast<physx::PxRigidDynamic*>(actor)), density); // () -> static_cast
		}
		SCOPED_PHYSX_LOCK_WRITE(&physxScene);
		physxScene.addActor(*actor);
		mPhysxActor = actor;
	}

private:
	physx::PxVec3*	mVerts;
	physx::PxVec3*	mNormals;
	float*	mUvs;
	uint16_t*	mFaces;

	uint32_t	mNbVerts;
	uint32_t	mNbFaces;

	ConvexMesh*		mConvexMesh;

	SampleRenderer::RendererMeshShape* mRendererMeshShape;
};

#endif
