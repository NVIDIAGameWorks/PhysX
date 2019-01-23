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

#include "SampleDirManager.h"
#include "SampleLargeWorld.h"
#include "MeshBuilder.h"
#include "wavefront.h"

using namespace PxToolkit;
using namespace physx;
using namespace shdfnd;

#define MATERIAL_ID	1


MeshBuilder::MeshBuilder(SampleLargeWorld& sample, const char* filename)
	: mSampleLargeWorld(&sample)
	, mPhysics(sample.getPhysics())
	, mScene(sample.getActiveScene())
	, mCooking(sample.getCooking())
	, mMaterial(sample.getDefaultMaterial())
	, mStringTable(sample.mStringTable)
{
	PxDefaultFileInputData inputStream(filename);	
	PxSerializationRegistry* sr = PxSerialization::createSerializationRegistry(mPhysics);
	
	PxCollection* c = PxSerialization::createCollectionFromXml(inputStream, mCooking, *sr, NULL, mStringTable);		
	PX_ASSERT( c );
	
	c->add( mMaterial, MATERIAL_ID );
	PxSerialization::complete(*c, *sr);
	
	PxU32 count = c->getNbObjects();
	for(PxU32 i = 0; i < count; i++)
	{
		PxBase* object = &c->getObject(i);
		switch ( object->getConcreteType() )
		{
		case PxConcreteType::eCONVEX_MESH:
			{
				PxConvexMesh* j = static_cast<PxConvexMesh*>(object);
				mConvexMeshes.push_back( j );
				break;
			}
		case PxConcreteType::eTRIANGLE_MESH_BVH33:
		case PxConcreteType::eTRIANGLE_MESH_BVH34:
			{
				PxTriangleMesh* j = static_cast<PxTriangleMesh*>(object);
				mTriMeshes.push_back(j);
				break;
			}
		default:
			break;
		}
	}
	c->release();
	sr->release();
}

void MeshBuilder::addObjMeshToPxCollection(
										PxPhysics&				physics,
										PxCooking&				cooking,
										PxMaterial&				material,
										const char*				filename,
										const char*				filenameCooked,
										const PxTransform&		pose, 
										const PxVec3&			scale, 
										PxCollection&			collection,
										const char*				name)
{
	PxTriangleMesh* mesh = NULL;
	
	PxDefaultFileInputData cooked(filenameCooked);
	if (cooked.isValid())
	{
		//load from disc
		mesh = physics.createTriangleMesh(cooked);	//if this load fails, maybe old format.  try to recook below.
	}
	if(!mesh)
	{
		WavefrontObj wfo;
		if (!wfo.loadObj(filename, false))
		{
			shdfnd::printFormatted("Error loading file: %s\n", filename);
		}

		PxTriangleMeshDesc meshDesc;
		meshDesc.points.count				= wfo.mVertexCount;
		meshDesc.triangles.count			= wfo.mTriCount;
		meshDesc.points.stride				= sizeof(float) * 3;
		meshDesc.triangles.stride			= sizeof(int) * 3;
		meshDesc.points.data				= wfo.mVertices;
		meshDesc.triangles.data				= wfo.mIndices;
		
		bool ok;
		{
			PxDefaultFileOutputStream stream(filenameCooked);
			ok = cooking.cookTriangleMesh(meshDesc, stream);
		}
		if ( ok )
		{
			PxDefaultFileInputData stream(filenameCooked);
			mesh = physics.createTriangleMesh(stream);
		}
	}
	PX_ASSERT( mesh );

	PxRigidStatic* actor = physics.createRigidStatic(pose);
	PX_ASSERT( actor );
	PxMeshScale meshScale = PxMeshScale(scale, PxQuat(PxIdentity));
	PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor, PxTriangleMeshGeometry(mesh, meshScale), material);
	PX_ASSERT( shape );
	shape->setFlag(PxShapeFlag::eVISUALIZATION, false);
	if( name ) 
	{
		actor->setName(name);
	}
	
	collection.add(*actor);
}

void MeshBuilder::addRepXToPxCollection(
										const PxTransform&	pose, 
										const PxVec3&		scale, 
										PxCollection&		collection,
										const char*			name)
{
	
	PxRigidStatic* actor = mPhysics.createRigidStatic(pose);
	PX_ASSERT( actor );
	
	size_t size = mConvexMeshes.size();
	PxMeshScale meshScale = PxMeshScale(scale, PxQuat(PxIdentity));
	for(PxU32 i = 0; i < size; ++i)
	{
		PxConvexMesh* mesh = mConvexMeshes[i];
		PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor, PxConvexMeshGeometry(mesh, meshScale), mMaterial);
		PX_ASSERT( shape );
		shape->setFlag(PxShapeFlag::eVISUALIZATION, false);
	}
	
	size = mTriMeshes.size();
	for(PxU32 i = 0; i < size; ++i)
	{
		PxTriangleMesh* mesh = mTriMeshes[i];
		PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor, PxTriangleMeshGeometry(mesh, meshScale), mMaterial);
		PX_ASSERT( shape );
		shape->setFlag(PxShapeFlag::eVISUALIZATION, false);
	}
	
	if( name ) 
		actor->setName(name);
	
	collection.add(*actor);
}
