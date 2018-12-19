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
#ifndef RT_RENDERABLE_H
#define RT_RENDERABLE_H

#include "PsArray.h"
#include "PsUserAllocated.h"
#include "PxVec2.h"

namespace nvidia
{
namespace apex
{
	class UserRenderer;
	class UserRenderVertexBuffer;
	class UserRenderIndexBuffer;
	class UserRenderBoneBuffer;
	class UserRenderResource;
}
using namespace shdfnd;

namespace fracture
{

class Actor;
class Convex;

class Renderable : public UserAllocated
{
public:
	Renderable();
	~Renderable();

	// Called by rendering thread
	void updateRenderResources(bool rewriteBuffers, void* userRenderData);
	void dispatchRenderResources(UserRenderer& api);

	// Per tick bone update, unless Actor is dirty
	void updateRenderCache(Actor* actor);

	// Returns the bounds of all of the convexes
	PxBounds3	getBounds() const;

private:
	// Called by actor after a patternFracture (On Game Thread)
	void updateRenderCacheFull(Actor* actor);

	// To Handle Multiple Materials
	struct SubMesh
	{
		SubMesh(): renderResource(NULL) {}

		Array<uint32_t>			mIndexCache;
		UserRenderResource*	renderResource;
	};
	// To Handle Bone Limit
	struct ConvexGroup
	{
		Array<SubMesh>			mSubMeshes;
		Array<Convex*>			mConvexCache;
		Array<PxVec3>			mVertexCache;
		Array<PxVec3>			mNormalCache;
		Array<PxVec2>			mTexcoordCache;
		Array<uint16_t>			mBoneIndexCache;
		Array<PxMat44>			mBoneCache;
	};
	// Shared by SubMeshes
	struct MaterialInfo
	{
		MaterialInfo(): mMaxBones(0), mMaterialID(0) {}

		uint32_t		mMaxBones;
		ResID		mMaterialID;
	};
	//
	Array<ConvexGroup>	mConvexGroups;
	Array<MaterialInfo> mMaterialInfo;

	UserRenderVertexBuffer*	mVertexBuffer;
	UserRenderIndexBuffer*	mIndexBuffer;
	UserRenderBoneBuffer*		mBoneBuffer;
	uint32_t						mVertexBufferSize;
	uint32_t						mIndexBufferSize;
	uint32_t						mBoneBufferSize;
	uint32_t						mVertexBufferSizeLast;
	uint32_t						mIndexBufferSizeLast;
	uint32_t						mBoneBufferSizeLast;
	bool						mFullBufferDirty;
	bool valid;
};

}
}

#endif
#endif