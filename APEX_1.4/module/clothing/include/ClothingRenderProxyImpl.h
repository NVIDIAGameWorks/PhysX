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



#ifndef CLOTHING_RENDER_PROXY_IMPL_H
#define CLOTHING_RENDER_PROXY_IMPL_H

#include "ClothingRenderProxy.h"
#include "PxMat44.h"
#include "PxBounds3.h"
#include "PsHashMap.h"
#include "ApexString.h"
#include "ApexRWLockable.h"

namespace nvidia
{
namespace apex
{
class RenderMeshActorIntl;
class RenderMeshAssetIntl;
}
namespace clothing
{
class ClothingActorParam;
class ClothingScene;


class ClothingRenderProxyImpl : public ClothingRenderProxy, public UserAllocated, public ApexRWLockable
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	ClothingRenderProxyImpl(RenderMeshAssetIntl* rma, bool useFallbackSkinning, bool useCustomVertexBuffer, const nvidia::HashMap<uint32_t, ApexSimpleString>& overrideMaterials, const PxVec3* morphTargetNewPositions, const uint32_t* morphTargetVertexOffsets, ClothingScene* scene);
	virtual ~ClothingRenderProxyImpl();

	// from ApexInterface
	virtual void release();

	// from Renderable
	virtual void dispatchRenderResources(UserRenderer& api);
	virtual PxBounds3 getBounds() const
	{
		return mBounds;
	}
	void setBounds(const PxBounds3& bounds)
	{
		mBounds = bounds;
	}

	// from RenderDataProvider.h
	virtual void lockRenderResources();
	virtual void unlockRenderResources();
	virtual void updateRenderResources(bool rewriteBuffers = false, void* userRenderData = 0);

	void setPose(const PxMat44& pose)
	{
		mPose = pose;
	}

	// from ClothingRenderProxy.h
	virtual bool hasSimulatedData() const;

	RenderMeshActorIntl* getRenderMeshActor();
	RenderMeshAssetIntl* getRenderMeshAsset();

	bool usesFallbackSkinning() const
	{
		return mUseFallbackSkinning;
	}

	bool usesCustomVertexBuffer() const
	{
		return renderingDataPosition != NULL;
	}

	const PxVec3* getMorphTargetBuffer() const
	{
		return mMorphTargetNewPositions;
	}

	void setOverrideMaterial(uint32_t i, const char* overrideMaterialName);
	bool overrideMaterialsEqual(const nvidia::HashMap<uint32_t, ApexSimpleString>& overrideMaterials);

	uint32_t getTimeInPool()  const;
	void setTimeInPool(uint32_t time);

	void notifyAssetRelease();

	PxVec3* renderingDataPosition;
	PxVec3* renderingDataNormal;
	PxVec4* renderingDataTangent;

private:
	RenderMeshActorIntl* createRenderMeshActor(RenderMeshAssetIntl* renderMeshAsset, ClothingActorParam* actorDesc);

	PxBounds3 mBounds;
	PxMat44 mPose;

	RenderMeshActorIntl* mRenderMeshActor;
	RenderMeshAssetIntl* mRenderMeshAsset;

	ClothingScene* mScene;

	bool mUseFallbackSkinning;
	HashMap<uint32_t, ApexSimpleString> mOverrideMaterials;
	const PxVec3* mMorphTargetNewPositions; // just to compare, only read it in constructor (it may be released)

	uint32_t mTimeInPool;

	Mutex mRMALock;
};

}
} // namespace nvidia

#endif // CLOTHING_RENDER_PROXY_IMPL_H
