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

#include "PxPhysXConfig.h"
#if PX_USE_CLOTH_API

#include "RenderClothActor.h"
#include "RenderCapsuleActor.h"
#include "RenderSphereActor.h"
#include "RenderMeshActor.h"
#include "RenderMaterial.h"
#include "RendererMeshShape.h"
#include "RendererClothShape.h"
#include "RendererCapsuleShape.h"
#include "PhysXSample.h"
#include "foundation/PxBounds3.h"

#include "cloth/PxClothParticleData.h"
#include "cloth/PxCloth.h"

#include "task/PxTask.h"

#include "extensions/PxSmoothNormals.h"

using namespace SampleRenderer;

/// These functions can probably move to PxToolkit or PxExtension
namespace 
{

// create uv from planar projection onto xz plane (where most cloth grids are created)
void createUVWithPlanarProjection(PxReal* uvs, PxU32 numVerts, const PxVec3* verts)
{
	PxBounds3 bounds = PxBounds3::empty();

    for(PxU32 i = 0; i < numVerts; i++)
		bounds.include(verts[i]);

	PxVec3 dim = bounds.getDimensions();
	PxReal minVal = PX_MAX_REAL;
	PxU32 minAxis = 0;

	// find smallest axis
	if (dim.x < minVal) { minVal = dim.x; minAxis = 0; }
	if (dim.y < minVal) { minVal = dim.y; minAxis = 1; }
	if (dim.z < minVal) { minVal = dim.z; minAxis = 2; }

	PxU32 uAxis = 0, vAxis = 1;
	switch (minAxis)
	{
		case 0: uAxis = 1; vAxis = 2; break; 
		case 1: uAxis = 0; vAxis = 2; break; 
		case 2: uAxis = 0; vAxis = 1; break; 
	}
	
	const float sizeU = dim[uAxis];
	const float sizeV = dim[vAxis];
	const float minU = bounds.minimum[uAxis];
	const float minV = bounds.minimum[vAxis];

	for (PxU32 i = 0; i < numVerts; ++i)
    {
        uvs[i*2+0] = (verts[i][uAxis] - minU) / sizeU;
        uvs[i*2+1] = 1.f - (verts[i][vAxis]-minV) / sizeV;
    }
    
}

// extract current cloth particle position from PxCloth
// verts is supposed to have been pre-allocated to be at least size of particle array
bool getVertsFromCloth(PxVec3* verts, const PxCloth& cloth)
{
	PxClothParticleData* readData = const_cast<PxCloth&>(cloth).lockParticleData();
    if (!readData)
        return false;

    // copy vertex positions
    PxU32 numVerts = cloth.getNbParticles();
	for (PxU32 i = 0; i < numVerts; ++i)
		verts[i] = readData->particles[i].pos;

	readData->unlock();

    return true;
}

// copy face structure from PxClothMeshDesc
PxU16* createFacesFromMeshDesc(const PxClothMeshDesc& desc)
{
    PxU32 numTriangles = desc.triangles.count;
	PxU32 numQuads = desc.quads.count;

    PxU16* faces = (PxU16*)SAMPLE_ALLOC(sizeof(PxU16)* (numTriangles*3 + numQuads*6));
	PxU16* fIt = faces;

    PxU8* triangles = (PxU8*)desc.triangles.data;
    for (PxU32 i = 0; i < numTriangles; i++)
    {
        if (desc.flags & PxMeshFlag::e16_BIT_INDICES)
        {
            PxU16* triangle = (PxU16*)triangles;
            *fIt++ = triangle[ 0 ];
            *fIt++ = triangle[ 1 ];
            *fIt++ = triangle[ 2 ];
        } 
        else
        {
            PxU32* triangle = (PxU32*)triangles;
            *fIt++ = triangle[ 0 ];
            *fIt++ = triangle[ 1 ];
            *fIt++ = triangle[ 2 ];
        }
        triangles += desc.triangles.stride;
    }

	PxU8* quads = (PxU8*)desc.quads.data;
	for (PxU32 i = 0; i < numQuads; i++)
	{
		if (desc.flags & PxMeshFlag::e16_BIT_INDICES)
		{
			PxU16* quad = (PxU16*)quads;
			*fIt++ = quad[ 0 ];
			*fIt++ = quad[ 1 ];
			*fIt++ = quad[ 2 ];
			*fIt++ = quad[ 2 ];
			*fIt++ = quad[ 3 ];
			*fIt++ = quad[ 0 ];
		} 
		else
		{
			PxU32* quad = (PxU32*)quads;
			*fIt++ = quad[ 0 ];
			*fIt++ = quad[ 1 ];
			*fIt++ = quad[ 2 ];
			*fIt++ = quad[ 2 ];
			*fIt++ = quad[ 3 ];
			*fIt++ = quad[ 0 ];
		}
		quads += desc.quads.stride;
	}

    return faces;
}

} // anonymous namespace

RenderClothActor::RenderClothActor(SampleRenderer::Renderer& renderer, const PxCloth& cloth, const PxClothMeshDesc &desc, const PxVec2* uv, const PxReal capsuleScale)
    : mRenderer(renderer)
	, mCloth(cloth)
	, mNumFaces(0), mFaces(NULL)
	, mSpheres(0), mCapsules(0), mPlanes(0), mConvexes(0), mTriangles(0)
	, mNumSpheres(0), mNumCapsules(0), mNumPlanes(0), mNumConvexes(0), mNumTriangles(0)
    , mUV(NULL)
	, mCapsuleScale(capsuleScale)
    , mClothRenderShape(NULL)
	, mConvexMaterial(NULL)
	, mMeshActor(NULL)
	, mConvexActor(NULL)
{
	int numVerts = desc.points.count;

    mNumFaces = desc.triangles.count + 2*desc.quads.count;
    mFaces = createFacesFromMeshDesc(desc);

	const PxVec3* verts = reinterpret_cast<const PxVec3*>(desc.points.data);

    mUV = (PxReal*)SAMPLE_ALLOC(sizeof(PxReal)*numVerts*2);		
	if (uv)
		memcpy( mUV, uv, sizeof(PxReal) * 2 * numVerts);
	else
        createUVWithPlanarProjection(mUV, numVerts, verts);

	mClothVertices.resize(numVerts);
	mClothNormals.resize(numVerts);

	updateRenderShape();
}

RenderClothActor::~RenderClothActor() 
{
	SAMPLE_FREE( mUV );
    SAMPLE_FREE( mFaces );
	SAMPLE_FREE( mSpheres );
	SAMPLE_FREE( mCapsules );
	SAMPLE_FREE( mPlanes );
	SAMPLE_FREE( mConvexes );
	SAMPLE_FREE( mTriangles );
	
	freeCollisionRenderSpheres();
	freeCollisionRenderCapsules();

	delete mMeshActor;
	delete mConvexActor;
}

void RenderClothActor::setConvexMaterial(RenderMaterial* material)
{
	mConvexMaterial = material;
}

void RenderClothActor::freeCollisionRenderSpheres()
{
	for (PxU32 i=0; i < mSphereActors.size(); ++i)
		delete mSphereActors[i];

	mSphereActors.clear();
}

void RenderClothActor::freeCollisionRenderCapsules()
{
	for (PxU32 i=0; i < mCapsuleActors.size(); ++i)
		delete mCapsuleActors[i];

	mCapsuleActors.clear();
}

namespace
{
	void BuildNormals(const PxVec3* PX_RESTRICT vertices, PxU32 numVerts, const PxU16* PX_RESTRICT faces, PxU32 numFaces, PxVec3* PX_RESTRICT normals)
	{
		memset(normals, 0, sizeof(PxVec3)*numVerts);
		
		const PxU32 numIndices = numFaces*3;

		// accumulate area weighted face normals in each vertex
		for (PxU32 t=0; t < numIndices; t+=3)
		{
			PxU16 i = faces[t];
			PxU16 j = faces[t+1];
			PxU16 k = faces[t+2];

			PxVec3 e1 = vertices[j]-vertices[i];
			PxVec3 e2 = vertices[k]-vertices[i];

			PxVec3 n = e2.cross(e1);

			normals[i] += n;
			normals[j] += n;
			normals[k] += n;
		}

		// average
		for (PxU32 i=0; i < numVerts; ++i)
			normals[i].normalize();
	}
}


void RenderClothActor::updateRenderShape()
{
	PX_PROFILE_ZONE("RenderClothActor::updateRenderShape",0);
	
	int numVerts = mCloth.getNbParticles();

    PX_ASSERT(numVerts > 0);

    if(!mClothRenderShape)
    {
		PxCudaContextManager* ctxMgr = NULL;

#if defined(RENDERER_ENABLE_CUDA_INTEROP)
		PxGpuDispatcher* dispatcher = mCloth.getScene()->getTaskManager()->getGpuDispatcher();
		
		// contxt must be created in at least one valid interop mode
		if (dispatcher && (ctxMgr = dispatcher->getCudaContextManager()) && 
			ctxMgr->getInteropMode() != PxCudaInteropMode::D3D10_INTEROP &&
			ctxMgr->getInteropMode() != PxCudaInteropMode::D3D11_INTEROP)
		{		
			ctxMgr = NULL;
		}
#endif

        mClothRenderShape = new RendererClothShape(
            mRenderer,
            &mClothVertices[0],
            numVerts,
            &mClothNormals[0],
            mUV,
            &mFaces[0],
            mNumFaces, false, ctxMgr);

        setRenderShape(mClothRenderShape);
    }

	{
		PX_PROFILE_ZONE("RenderClothShape::update",0);
	
		bool needUpdate = true;

#if defined(RENDERER_ENABLE_CUDA_INTEROP)

		if (mCloth.getClothFlags()&PxClothFlag::eGPU && mClothRenderShape->isInteropEnabled())
		{
			PxClothParticleData* data = const_cast<PxCloth&>(mCloth).lockParticleData(PxDataAccessFlag::eDEVICE);
			{
				PX_PROFILE_ZONE("updateInterop",0);
				bool success = mClothRenderShape->update(reinterpret_cast<CUdeviceptr>(data->particles), numVerts);

				// if CUDA update succeeded then skip CPU update
				if (success)
					needUpdate = false;
			}
			data->unlock();
		}
#endif // RENDERER_ENABLE_CUDA_INTEROP

		if (needUpdate)
		{
			bool result;	
			result = getVertsFromCloth(&mClothVertices[0], mCloth);
			PX_UNUSED(result);
			PX_ASSERT(result == true);

			{
				// update render normals
				PX_PROFILE_ZONE("BuildSmoothNormals",0);
				BuildNormals(&mClothVertices[0], mClothVertices.size(), mFaces, mNumFaces, &mClothNormals[0]);
			}

			mClothRenderShape->update(&mClothVertices[0], numVerts, &mClothNormals[0]);
		}
	}

	setTransform(mCloth.getGlobalPose());
}

namespace
{
	struct ConvexMeshBuilder
	{
		ConvexMeshBuilder(const PxVec4* planes)
			: mPlanes(planes)
		{}

		void operator()(PxU32 mask, float scale=1.0f);

		const PxVec4* mPlanes;
		shdfnd::Array<PxVec3> mVertices;
		shdfnd::Array<PxU16> mIndices;
	};

	/*
	void test()
	{
		int data[4*32];
		float planes[4*32];

		for(PxU32 numPlanes = 1; numPlanes<=32; ++numPlanes)
		{
			int seed = 0;

			const PxU32 planeMask = (PxU64(1) << numPlanes) - 1;
			for(PxU32 i=0; i<100000; ++i)
			{
				srand(seed);

				for(PxU32 j=0; j<4*numPlanes; ++j)
					planes[j] = float(data[j] = rand()) / RAND_MAX * 2.0f - 1.0f;

				ConvexMeshBuilder builder(reinterpret_cast<PxVec4*>(planes));
				builder(planeMask, 0.0f);

				seed = data[numPlanes*4-1];
			}
		}
	}
	*/
}

void RenderClothActor::update(float deltaTime)
{
	PX_PROFILE_ZONE("RenderClothActor::update",0);

	updateRenderShape();

	// update collision shapes
	PxU32 numSpheres = mCloth.getNbCollisionSpheres();
	PxU32 numCapsules = mCloth.getNbCollisionCapsules();
	PxU32 numPlanes = mCloth.getNbCollisionPlanes();
	PxU32 numConvexes = mCloth.getNbCollisionConvexes();
	PxU32 numTriangles = mCloth.getNbCollisionTriangles();

	if (numSpheres == 0 && mNumSpheres == 0 &&
        numTriangles == 0 && mNumTriangles == 0 &&
		numConvexes == 0 && mNumConvexes == 0)
		return;

	if(numSpheres != mNumSpheres)
	{
		SAMPLE_FREE(mSpheres);
		mSpheres = (PxClothCollisionSphere*)SAMPLE_ALLOC(sizeof(PxClothCollisionSphere) * (mNumSpheres = numSpheres));
	}

	if(numCapsules != mNumCapsules)
	{
		SAMPLE_FREE(mCapsules);
		mCapsules = (PxU32*)SAMPLE_ALLOC(sizeof(PxU32) * 2 * (mNumCapsules = numCapsules));
	}

	if(numPlanes != mNumPlanes)
	{
		SAMPLE_FREE(mPlanes);
		mPlanes = (PxClothCollisionPlane*)SAMPLE_ALLOC(sizeof(PxClothCollisionPlane) * (mNumPlanes = numPlanes));
	}

	if(numConvexes != mNumConvexes)
	{
		SAMPLE_FREE(mConvexes);
		mConvexes = (PxU32*)SAMPLE_ALLOC(sizeof(PxU32) * (mNumConvexes = numConvexes));
	}

	if(numTriangles != mNumTriangles)
	{
		SAMPLE_FREE(mTriangles);
		mTriangles = (PxClothCollisionTriangle*)SAMPLE_ALLOC(sizeof(PxClothCollisionTriangle) * (mNumTriangles = numTriangles));
		mMeshIndices.resize(0); mMeshIndices.reserve(mNumTriangles*3);
		for(PxU32 i=0; i<mNumTriangles*3; ++i)
			mMeshIndices.pushBack(PxU16(i));
	}

	{
		PX_PROFILE_ZONE("mCloth.getCollisionData",0);
		mCloth.getCollisionData(mSpheres, mCapsules, mPlanes, mConvexes, mTriangles);
	}

	PxTransform clothPose = mCloth.getGlobalPose();

	// see if we need to recreate the collision shapes (count has changed)
	if (numSpheres != mSphereActors.size())
	{
		freeCollisionRenderSpheres();

		// create sphere actors (we reuse the RenderCapsuleActor type)
		for (PxU32 i=0; i < numSpheres; i++)
		{
			RenderSphereActor* sphere = SAMPLE_NEW(RenderSphereActor)(mClothRenderShape->getRenderer(), 1.0f);
			mSphereActors.pushBack(sphere);
		}
	}

	if (numCapsules != mCapsuleActors.size())
	{
		freeCollisionRenderCapsules();

		// create capsule actors
		for (PxU32 i=0; i < numCapsules; i++)
		{
			RenderCapsuleActor* capsule = SAMPLE_NEW(RenderCapsuleActor)(mClothRenderShape->getRenderer(), 1.0f, 1.0f);
			mCapsuleActors.pushBack(capsule);
		}
	}

	
	{
		PX_PROFILE_ZONE("updateRenderSpheres",0);

		// update all spheres
		for (PxU32 i=0; i < numSpheres; ++i)
		{
			float r = mSpheres[i].radius*mCapsuleScale;

			mSphereActors[i]->setRendering(true);
			mSphereActors[i]->setTransform(PxTransform(clothPose.transform(mSpheres[i].pos)));
			mSphereActors[i]->setMeshScale(PxMeshScale(PxVec3(r, r, r), PxQuat(PxIdentity)));
		}
	}


	{
		PX_PROFILE_ZONE("updateRenderCapsules",0);

		// capsule needs to be flipped to match PxTransformFromSegment
		PxTransform flip(PxVec3(0.0f), PxQuat(-PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f)));

		// update all capsules
		for (PxU32 i=0; i < numCapsules; ++i)
		{
			PxU32 i0 = mCapsules[i*2];
			PxU32 i1 = mCapsules[i*2+1];

			PxClothCollisionSphere& sphere0 = mSpheres[i0];
			PxClothCollisionSphere& sphere1 = mSpheres[i1];
			
			// disable individual rendering for spheres belonging to a capsule
			mSphereActors[i0]->setRendering(false);
			mSphereActors[i1]->setRendering(false);

			PxVec3 p0 = clothPose.transform(sphere0.pos);
			PxVec3 p1 = clothPose.transform(sphere1.pos);

			mCapsuleActors[i]->setDimensions((p1-p0).magnitude()*0.5f, mCapsuleScale*sphere0.radius, mCapsuleScale*sphere1.radius);
			mCapsuleActors[i]->setTransform(PxTransformFromSegment(p0, p1)*flip);		
		}
	}

	// update all triangles
	SAMPLE_FREE(mMeshActor);
	if(mNumTriangles)
	{
		mMeshActor = SAMPLE_NEW(RenderMeshActor)(mRenderer, reinterpret_cast<const PxVec3*>(
			mTriangles), mNumTriangles*3, 0, 0, mMeshIndices.begin(), NULL, mNumTriangles, true);
		mMeshActor->setTransform(clothPose);
	}

	SAMPLE_FREE(mConvexActor);
	if(mNumConvexes)
	{
		ConvexMeshBuilder builder(reinterpret_cast<const PxVec4*>(mPlanes));
		for(PxU32 i=0; i<mNumConvexes; ++i)
			builder(mConvexes[i], mCloth.getWorldBounds().getExtents().maxElement());
		mConvexVertices = builder.mVertices;
		mConvexIndices = builder.mIndices;
		if(!mConvexIndices.empty())
		{
			mConvexActor = SAMPLE_NEW(RenderMeshActor)(mRenderer, mConvexVertices.begin(), 
				mConvexVertices.size(), 0, 0, mConvexIndices.begin(), NULL, mConvexIndices.size()/3, true);
			mConvexActor->setTransform(clothPose);
		}
	}

}

void RenderClothActor::render(SampleRenderer::Renderer& renderer, RenderMaterial* material, bool wireFrame)
{
	RenderBaseActor::render(renderer, material, wireFrame);

	// render collision shapes

	for (PxU32 i=0; i < mSphereActors.size(); ++i)
		mSphereActors[i]->render(renderer, material, wireFrame);

	for (PxU32 i=0; i < mCapsuleActors.size(); ++i)
		mCapsuleActors[i]->render(renderer, material, wireFrame);

	if(mMeshActor)
		mMeshActor->render(renderer, material, wireFrame);

	if(mConvexActor)
		mConvexActor->render(renderer, mConvexMaterial ? mConvexMaterial : material, wireFrame);
}

namespace
{
	PxReal det(PxVec4 v0, PxVec4 v1, PxVec4 v2, PxVec4 v3)
	{
		const PxVec3& d0 = reinterpret_cast<const PxVec3&>(v0);
		const PxVec3& d1 = reinterpret_cast<const PxVec3&>(v1);
		const PxVec3& d2 = reinterpret_cast<const PxVec3&>(v2);
		const PxVec3& d3 = reinterpret_cast<const PxVec3&>(v3);

		return v0.w * d1.cross(d2).dot(d3)
			- v1.w * d0.cross(d2).dot(d3)
			+ v2.w * d0.cross(d1).dot(d3)
			- v3.w * d0.cross(d1).dot(d2);
	}

	PxVec3 intersect(PxVec4 p0, PxVec4 p1, PxVec4 p2)
	{
		const PxVec3& d0 = reinterpret_cast<const PxVec3&>(p0);
		const PxVec3& d1 = reinterpret_cast<const PxVec3&>(p1);
		const PxVec3& d2 = reinterpret_cast<const PxVec3&>(p2);

		return (p0.w * d1.cross(d2) 
			  + p1.w * d2.cross(d0) 
			  + p2.w * d0.cross(d1))
			  / d0.dot(d2.cross(d1));
	}

	const PxU16 sInvalid = PxU16(-1);

	// restriction: only supports a single patch per vertex.
	struct HalfedgeMesh
	{
		struct Halfedge
		{
			Halfedge(PxU16 vertex = sInvalid, PxU16 face = sInvalid, 
				PxU16 next = sInvalid, PxU16 prev = sInvalid)
				: mVertex(vertex), mFace(face), mNext(next), mPrev(prev)
			{}

			PxU16 mVertex; // to
			PxU16 mFace; // left
			PxU16 mNext; // ccw
			PxU16 mPrev; // cw
		};

		HalfedgeMesh() : mNumTriangles(0) {}

		PxU16 findHalfedge(PxU16 v0, PxU16 v1)
		{
			PxU16 h = mVertices[v0], start = h;
			while(h != sInvalid && mHalfedges[h].mVertex != v1)
			{
				h = mHalfedges[h ^ 1].mNext;
				if(h == start) 
					return sInvalid;
			}
			return h;
		}

		void connect(PxU16 h0, PxU16 h1)
		{
			mHalfedges[h0].mNext = h1;
			mHalfedges[h1].mPrev = h0;
		}

		void addTriangle(PxU16 v0, PxU16 v1, PxU16 v2)
		{
			// add new vertices
			PxU16 n = PxMax(v0, PxMax(v1, v2))+1;
			if(mVertices.size() < n)
				mVertices.resize(n, sInvalid);

			// collect halfedges, prev and next of triangle
			PxU16 verts[] = { v0, v1, v2 };
			PxU16 handles[3], prev[3], next[3];
			for(PxU16 i=0; i<3; ++i)
			{
				PxU16 j = (i+1)%3;
				PxU16 h = findHalfedge(verts[i], verts[j]);
				if(h == sInvalid)
				{
					// add new edge
					h = mHalfedges.size();
					mHalfedges.pushBack(Halfedge(verts[j]));
					mHalfedges.pushBack(Halfedge(verts[i]));
				}
				handles[i] = h;
				prev[i] = mHalfedges[h].mPrev;
				next[i] = mHalfedges[h].mNext;
			}

			// patch connectivity
			for(PxU16 i=0; i<3; ++i)
			{
				PxU16 j = (i+1)%3;

				mHalfedges[handles[i]].mFace = mFaces.size();

				// connect prev and next
				connect(handles[i], handles[j]);

				if(next[j] == sInvalid) // new next edge, connect opposite
					connect(handles[j]^1, next[i]!=sInvalid ? next[i] : handles[i]^1);

				if(prev[i] == sInvalid) // new prev edge, connect opposite
					connect(prev[j]!=sInvalid ? prev[j] : handles[j]^1, handles[i]^1);

				// prev is boundary, update middle vertex
				if(mHalfedges[handles[i]^1].mFace == sInvalid)
					mVertices[verts[j]] = handles[i]^1;
			}

			PX_ASSERT(mNumTriangles < 0xffff);
			mFaces.pushBack(handles[2]);
			++mNumTriangles;
		}

		PxU16 removeTriangle(PxU16 f)
		{
			PxU16 result = sInvalid;

			for(PxU16 i=0, h = mFaces[f]; i<3; ++i)
			{
				PxU16 v0 = mHalfedges[h^1].mVertex;
				PxU16 v1 = mHalfedges[h].mVertex;

				mHalfedges[h].mFace = sInvalid;

				if(mHalfedges[h^1].mFace == sInvalid) // was boundary edge, remove
				{
					PxU16 v0Prev = mHalfedges[h  ].mPrev;
					PxU16 v0Next = mHalfedges[h^1].mNext;
					PxU16 v1Prev = mHalfedges[h^1].mPrev;
					PxU16 v1Next = mHalfedges[h  ].mNext;

					// update halfedge connectivity
					connect(v0Prev, v0Next);
					connect(v1Prev, v1Next);

					// update vertex boundary or delete
					mVertices[v0] = (v0Prev^1) == v0Next ? sInvalid : v0Next;
					mVertices[v1] = (v1Prev^1) == v1Next ? sInvalid : v1Next;
				} 
				else 
				{
					mVertices[v0] = h; // update vertex boundary
					result = v1;
				}

				h = mHalfedges[h].mNext;
			}

			mFaces[f] = sInvalid;
			--mNumTriangles;

			return result;
		}

		// true if vertex v is in front of face f
		bool visible(PxU16 v, PxU16 f)
		{
			PxU16 h = mFaces[f];
			if(h == sInvalid)
				return false;

			PxU16 v0 = mHalfedges[h].mVertex;
			h = mHalfedges[h].mNext;
			PxU16 v1 = mHalfedges[h].mVertex;
			h = mHalfedges[h].mNext;
			PxU16 v2 = mHalfedges[h].mVertex;
			h = mHalfedges[h].mNext;

			return det(mPoints[v], mPoints[v0], mPoints[v1], mPoints[v2]) < -1e-5f;
		}

		/*
		void print() const
		{
			for(PxU32 i=0; i<mFaces.size(); ++i)
			{
				shdfnd::printFormatted("f%u: ", i);
				PxU16 h = mFaces[i];
				if(h == sInvalid)
				{
					shdfnd::printFormatted("deleted\n");
					continue;
				}

				for(int j=0; j<3; ++j)
				{
					shdfnd::printFormatted("h%u -> v%u -> ", PxU32(h), PxU32(mHalfedges[h].mVertex));
					h = mHalfedges[h].mNext;
				}

				shdfnd::printFormatted("\n");
			}

			for(PxU32 i=0; i<mVertices.size(); ++i)
			{
				shdfnd::printFormatted("v%u: ", i);
				PxU16 h = mVertices[i];
				if(h == sInvalid)
				{
					shdfnd::printFormatted("deleted\n");
					continue;
				}
				
				PxU16 start = h;
				do {
					shdfnd::printFormatted("h%u -> v%u, ", PxU32(h), PxU32(mHalfedges[h].mVertex));
					h = mHalfedges[h^1].mNext;
				} while (h != start);

				shdfnd::printFormatted("\n");
			}

			for(PxU32 i=0; i<mHalfedges.size(); ++i)
			{
				shdfnd::printFormatted("h%u: v%u, ", i, PxU32(mHalfedges[i].mVertex));

				if(mHalfedges[i].mFace == sInvalid)
					shdfnd::printFormatted("boundary, ");
				else
					shdfnd::printFormatted("f%u, ", PxU32(mHalfedges[i].mFace));

				shdfnd::printFormatted("p%u, n%u\n", PxU32(mHalfedges[i].mPrev), PxU32(mHalfedges[i].mNext));
			}
		}
		*/

		shdfnd::Array<Halfedge> mHalfedges;
		shdfnd::Array<PxU16> mVertices; // vertex -> (boundary) halfedge
		shdfnd::Array<PxU16> mFaces; // face -> halfedge
		shdfnd::Array<PxVec4> mPoints;
		PxU16 mNumTriangles;
	};
}

void ConvexMeshBuilder::operator()(PxU32 planeMask, float scale)
{
	PxU16 numPlanes = shdfnd::bitCount(planeMask);

	if (numPlanes == 1)
	{
		PxTransform t = PxTransformFromPlaneEquation(reinterpret_cast<const PxPlane&>(mPlanes[Ps::lowestSetBit(planeMask)]));

		if (!t.isValid())
			return;

		const PxU16 indices[] = { 0, 1, 2, 0, 2, 3 };
		const PxVec3 vertices[] = { 
			PxVec3(0.0f,  scale,  scale), 
			PxVec3(0.0f, -scale,  scale),
			PxVec3(0.0f, -scale, -scale),			
			PxVec3(0.0f,  scale, -scale) };

		PxU32 baseIndex = mVertices.size();

		for (PxU32 i=0; i < 4; ++i)
			mVertices.pushBack(t.transform(vertices[i]));

		for (PxU32 i=0; i < 6; ++i)
			mIndices.pushBack(indices[i] + baseIndex);

		return;
	}

	if(numPlanes < 4)
		return; // todo: handle degenerate cases

	HalfedgeMesh mesh;

	// gather points (planes, that is)
	mesh.mPoints.reserve(numPlanes);
	for(; planeMask; planeMask &= planeMask-1)
		mesh.mPoints.pushBack(mPlanes[shdfnd::lowestSetBit(planeMask)]);

	// initialize to tetrahedron
	mesh.addTriangle(0, 1, 2);
	mesh.addTriangle(0, 3, 1);
	mesh.addTriangle(1, 3, 2);
	mesh.addTriangle(2, 3, 0);

	// flip if inside-out
	if(mesh.visible(3, 0))
		shdfnd::swap(mesh.mPoints[0], mesh.mPoints[1]);

	// iterate through remaining points
	for(PxU16 i=4; i<mesh.mPoints.size(); ++i)
	{
		// remove any visible triangle
		PxU16 v0 = sInvalid;
		for(PxU16 j=0; j<mesh.mFaces.size(); ++j)
		{
			if(mesh.visible(i, j))
				v0 = PxMin(v0, mesh.removeTriangle(j));
		}

		if(v0 == sInvalid)
			continue; // no triangle removed

		if(!mesh.mNumTriangles)
			return; // empty mesh

		// find non-deleted boundary vertex
		for(PxU16 h=0; mesh.mVertices[v0] == sInvalid; h+=2)
		{
			if ((mesh.mHalfedges[h  ].mFace == sInvalid) ^ 
				(mesh.mHalfedges[h+1].mFace == sInvalid))
			{
				v0 = mesh.mHalfedges[h].mVertex;
			}
		}

		// tesselate hole
		PxU16 start = v0;
		do {
			PxU16 h = mesh.mVertices[v0];
			PxU16 v1 = mesh.mHalfedges[h].mVertex;
			mesh.addTriangle(v0, v1, i);
			v0 = v1;
		} while(v0 != start);
	}

	// convert triangles to vertices (intersection of 3 planes)
	shdfnd::Array<PxU32> face2Vertex(mesh.mFaces.size(), sInvalid);
	for(PxU32 i=0; i<mesh.mFaces.size(); ++i)
	{
		PxU16 h = mesh.mFaces[i];
		if(h == sInvalid)
			continue;

		PxU16 v0 = mesh.mHalfedges[h].mVertex;
		h = mesh.mHalfedges[h].mNext;
		PxU16 v1 = mesh.mHalfedges[h].mVertex;
		h = mesh.mHalfedges[h].mNext;
		PxU16 v2 = mesh.mHalfedges[h].mVertex;

		face2Vertex[i] = mVertices.size();
		mVertices.pushBack(intersect(mesh.mPoints[v0], mesh.mPoints[v1], mesh.mPoints[v2]));
	}

	// convert vertices to polygons (face one-ring)
	for(PxU32 i=0; i<mesh.mVertices.size(); ++i)
	{
		PxU16 h = mesh.mVertices[i];
		if(h == sInvalid)
			continue;

		PxU16 v0 = face2Vertex[mesh.mHalfedges[h].mFace];
		h = mesh.mHalfedges[h].mPrev^1;
		PxU16 v1 = face2Vertex[mesh.mHalfedges[h].mFace];

		while(true)
		{
			h = mesh.mHalfedges[h].mPrev^1;
			PxU16 v2 = face2Vertex[mesh.mHalfedges[h].mFace];

			if(v0 == v2) 
				break;

			PxVec3 e0 =  mVertices[v0] -  mVertices[v2];
			PxVec3 e1 =  mVertices[v1] -  mVertices[v2];
			if(e0.cross(e1).magnitudeSquared() < 1e-10f)
				continue;

			mIndices.pushBack(v0);
			mIndices.pushBack(v2);
			mIndices.pushBack(v1);

			v1 = v2; 
		}

	}
}

#endif // PX_USE_CLOTH_API


