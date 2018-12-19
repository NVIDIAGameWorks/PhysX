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


#ifndef __SAMPLE_TRI_MESH_ACTOR_H__
#define __SAMPLE_TRI_MESH_ACTOR_H__

#include "SampleShapeActor.h"
#include "RendererMeshShape.h"
#include "PsMemoryBuffer.h"
#include "PsArray.h"
#include "PxStreamFromFileBuf.h"
#include "ApexSDK.h"

#include "cooking/PxCooking.h"
#include "cooking/PxConvexMeshDesc.h"
#include "geometry/PxConvexMeshGeometry.h"

#include "geometry/PxTriangleMesh.h"
#include "cooking/PxTriangleMeshDesc.h"
#include "geometry/PxTriangleMeshGeometry.h"

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

//typedef physx::PxConvexMesh ConvexMesh;
typedef physx::PxTriangleMesh TriMesh;

class SampleTriMeshActor : public SampleShapeActor
{
public:

	SampleTriMeshActor(SampleRenderer::Renderer* renderer,
	               SampleFramework::SampleMaterialAsset& material,
	               physx::PxScene& physxScene,
				   physx::PxCooking& cooking,
				   const physx::PxVec3* verts, 
				   const uint32_t nbVerts,
				   const uint32_t* indices,
				   const uint32_t nbIndices,
				   float* uvs,
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
		, mTriMesh(NULL)
		, mRendererMeshShape(NULL)
	{
		mRenderer = renderer;

		createActor(physxScene, cooking, verts, nbVerts, indices, nbIndices, pos, vel, density, PxMaterial, useGroupsMask);

		const bool has16BitIndices = (mTriMesh->getTriangleMeshFlags() & physx::PxTriangleMeshFlag::e16_BIT_INDICES);

		const uint32_t   nbTris			= mTriMesh->getNbTriangles();
		const uint32_t*  indexBuffer	= (uint32_t*) (has16BitIndices ? NULL : mTriMesh->getTriangles());
		const uint16_t*  indexBuffer16	= (uint16_t*) (has16BitIndices ? mTriMesh->getTriangles() : NULL);
		const physx::PxVec3* vertices		= mTriMesh->getVertices();

		mNbVerts = 3*nbTris;
		mNbFaces = 3*nbTris;

		mVerts		= new physx::PxVec3[mNbVerts];
		mNormals	= new physx::PxVec3[mNbVerts];
		mFaces		= new uint16_t[mNbFaces];
		if (uvs != NULL)
		{
			mUvs		= new float[mNbVerts * 2];
		}

		if(indexBuffer)
		{
			for (uint32_t i = 0; i < nbTris; i++)
			{
				const physx::PxVec3& A( vertices[indexBuffer[3*i+0]] );
				const physx::PxVec3& B( vertices[indexBuffer[3*i+1]] );
				const physx::PxVec3& C( vertices[indexBuffer[3*i+2]] );

				physx::PxVec3 a(B-A),b(C-A);
				physx::PxVec3 normal = a.cross(b);
				normal.normalize();

				mVerts[3*i+0] = A;
				mVerts[3*i+1] = C;
				mVerts[3*i+2] = B;
				mNormals[3*i+0] = normal;
				mNormals[3*i+1] = normal;
				mNormals[3*i+2] = normal;
				mFaces[3*i+0] = uint16_t(3*i+0);
				mFaces[3*i+1] = uint16_t(3*i+1);
				mFaces[3*i+2] = uint16_t(3*i+2);
			}
		}
		else if(indexBuffer16)
		{
			for (uint32_t i = 0; i < nbTris; i++)
			{
				const physx::PxVec3& A( verts[indices[3*i+0]] );
				const physx::PxVec3& B( verts[indices[3*i+1]] );
				const physx::PxVec3& C( verts[indices[3*i+2]] );

				physx::PxVec3 a(B-A),b(C-A);
				physx::PxVec3 normal = a.cross(b);
				normal.normalize();

				mVerts[3*i+0] = A;
				mVerts[3*i+1] = C;
				mVerts[3*i+2] = B;
				if (uvs != NULL)
				{
					mUvs[6*i+0] = uvs[2*indices[3*i+0]];
					mUvs[6*i+1] = uvs[2*indices[3*i+0] + 1];
					mUvs[6*i+2] = uvs[2*indices[3*i+2]];
					mUvs[6*i+3] = uvs[2*indices[3*i+2] + 1];
					mUvs[6*i+4] = uvs[2*indices[3*i+1]];
					mUvs[6*i+5] = uvs[2*indices[3*i+1] + 1];
				}
				mNormals[3*i+0] = normal;
				mNormals[3*i+1] = normal;
				mNormals[3*i+2] = normal;
				mFaces[3*i+0] = uint16_t(3*i+0);
				mFaces[3*i+1] = uint16_t(3*i+1);
				mFaces[3*i+2] = uint16_t(3*i+2);
			}
		}
		else
		{
			PX_ASSERT(0 && "Invalid Index Data");
		}

		mRendererMeshShape = new SampleRenderer::RendererMeshShape(*mRenderer, mVerts, mNbVerts, mNormals, mUvs, mFaces, mNbFaces / 3);

		mRendererMeshContext.material         = material.getMaterial();
		mRendererMeshContext.materialInstance = material.getMaterialInstance();
		mRendererMeshContext.mesh             = mRendererMeshShape->getMesh();
		mRendererMeshContext.transform        = &mTransform;

		if (RENDER_DEBUG_IFACE(rdebug))
		{
			mBlockId = RENDER_DEBUG_IFACE(rdebug)->beginDrawGroup(mTransform);
			RENDER_DEBUG_IFACE(rdebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::SolidShaded);
			static uint32_t bcount /* = 0 */;
			RENDER_DEBUG_IFACE(rdebug)->setCurrentColor(0xFFFFFF);
			RENDER_DEBUG_IFACE(rdebug)->setCurrentTextScale(0.5f);
			RENDER_DEBUG_IFACE(rdebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::CenterText);
			RENDER_DEBUG_IFACE(rdebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::CameraFacing);
			RENDER_DEBUG_IFACE(rdebug)->debugText(physx::PxVec3(0, 1.0f + 0.01f, 0), "Sample Triangle Mesh:%d", bcount++);
			RENDER_DEBUG_IFACE(rdebug)->endDrawGroup();
		}
	}

	virtual ~SampleTriMeshActor()
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
					 const uint32_t* indices,
					 const uint32_t nbIndices,
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
		actor = physxScene.getPhysics().createRigidStatic(physx::PxTransform(mTransform));

		physx::PxTriangleMeshDesc triMeshDesc;
		triMeshDesc.points.count = nbVerts;
		triMeshDesc.points.data = verts;
		triMeshDesc.points.stride = sizeof(physx::PxVec3);
		triMeshDesc.triangles.count = nbIndices/3;
		triMeshDesc.triangles.data = indices;
		triMeshDesc.triangles.stride = 3*sizeof(uint32_t);

		physx::PsMemoryBuffer stream;
		stream.setEndianMode(physx::PxFileBuf::ENDIAN_NONE);
		nvidia::apex::PxStreamFromFileBuf nvs(stream);

		if (cooking.cookTriangleMesh(triMeshDesc, nvs))
		{
			mTriMesh = physxScene.getPhysics().createTriangleMesh(nvs);
			PX_ASSERT(mTriMesh);
		}

		physx::PxTriangleMeshGeometry triMeshGeom(mTriMesh);
		physx::PxShape* shape = actor->createShape(triMeshGeom, *PxMaterial);
		PX_ASSERT(shape);
		if (shape && useGroupsMask)
		{
			shape->setSimulationFilterData(physx::PxFilterData(1, 0, ~0u, 0));
			shape->setQueryFilterData(physx::PxFilterData(1, 0, ~0u, 0));
		}
		{
			physxScene.lockWrite(__FILE__, __LINE__);
			physxScene.addActor(*actor);
			physxScene.unlockWrite();
		}		
		mPhysxActor = actor;
	}

private:
	physx::PxVec3*	mVerts;
	physx::PxVec3*	mNormals;
	float*	mUvs;
	uint16_t*	mFaces;

	uint32_t	mNbVerts;
	uint32_t	mNbFaces;

	TriMesh* mTriMesh;

	SampleRenderer::RendererMeshShape* mRendererMeshShape;
};

#endif
