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

#ifndef RENDER_CLOTH_ACTOR_H
#define RENDER_CLOTH_ACTOR_H

#include "RenderBaseActor.h"
#include "RenderPhysX3Debug.h"

#include "SampleAllocator.h"
#include "SampleArray.h"

#include "cloth/PxCloth.h"
#include "extensions/PxClothMeshDesc.h"
#include "RenderMaterial.h"

namespace SampleRenderer
{
	class Renderer;
    class RendererClothShape;
}

class RenderCapsuleActor;
class RenderSphereActor;
class RenderMeshActor;

class RenderClothActor : public RenderBaseActor
{
public:
	RenderClothActor( SampleRenderer::Renderer& renderer, const PxCloth& cloth, const PxClothMeshDesc &desc, const PxVec2* uvs = NULL, PxReal capsuleScale = 1.0f);

	virtual								~RenderClothActor();

	virtual void						update(float deltaTime);
	virtual	void						render(SampleRenderer::Renderer& renderer, RenderMaterial* material, bool wireFrame);
	void								setConvexMaterial(RenderMaterial* material);

	virtual SampleRenderer::RendererClothShape*	getRenderClothShape() const { return mClothRenderShape; }
	virtual const PxCloth*				getCloth() const { return &mCloth; }

private:
	void								updateRenderShape();

private:

	RenderClothActor& operator=(const RenderClothActor&);

	void								freeCollisionRenderSpheres();
	void								freeCollisionRenderCapsules();

	SampleRenderer::Renderer&			mRenderer;
	const PxCloth&                      mCloth; 

	// copied mesh structure 
	PxU32                               mNumFaces;
	PxU16*                              mFaces;

	// collision data used for debug rendering
	PxClothCollisionSphere*             mSpheres;
	PxU32*                              mCapsules;
	PxClothCollisionPlane*				mPlanes;
	PxU32*								mConvexes;
	PxClothCollisionTriangle*			mTriangles;
	PxU32                               mNumSpheres, mNumCapsules, mNumPlanes, mNumConvexes, mNumTriangles;

	// texture uv (used only for render)
	PxReal*								mUV;

	PxVec3								mRendererColor;
	PxReal                              mCapsuleScale;

    SampleRenderer::RendererClothShape* mClothRenderShape;
	RenderMaterial*						mConvexMaterial;

	// collision shapes render actors
	shdfnd::Array<RenderSphereActor*>	mSphereActors;
	shdfnd::Array<RenderCapsuleActor*>	mCapsuleActors;

	RenderMeshActor*					mMeshActor;
	shdfnd::Array<PxU16>				mMeshIndices;

	RenderMeshActor*					mConvexActor;
	shdfnd::Array<PxVec3>				mConvexVertices;
	shdfnd::Array<PxU16>				mConvexIndices;

	shdfnd::Array<PxVec3>				mClothVertices;
	shdfnd::Array<PxVec3>				mClothNormals;

};

#endif
