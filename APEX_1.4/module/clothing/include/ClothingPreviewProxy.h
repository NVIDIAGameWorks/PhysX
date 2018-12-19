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



#ifndef CLOTHING_PREVIEW_PROXY_H
#define CLOTHING_PREVIEW_PROXY_H

#include "ClothingPreview.h"
#include "ClothingActorImpl.h"
#include "Renderable.h"
#include "ApexRWLockable.h"
#include "WriteCheck.h"
#include "ReadCheck.h"

namespace nvidia
{
namespace clothing
{

class ClothingPreviewProxy : public ClothingPreview, public UserAllocated, public ApexResourceInterface, public ApexRWLockable
{
	ClothingActorImpl impl;

public:
	APEX_RW_LOCKABLE_BOILERPLATE
#pragma warning( disable : 4355 ) // disable warning about this pointer in argument list
	ClothingPreviewProxy(const NvParameterized::Interface& desc, ClothingAssetImpl* asset, ResourceList* list) :
		impl(desc, NULL, this, asset, NULL)
	{
		list->add(*this);
	}

	virtual void release()
	{
		impl.release();
	}

	virtual uint32_t getListIndex() const
	{
		return impl.m_listIndex;
	}

	virtual void setListIndex(class ResourceList& list, uint32_t index)
	{
		impl.m_list = &list;
		impl.m_listIndex = index;
	}

	virtual void setPose(const PxMat44& pose)
	{
		WRITE_ZONE();
		impl.updateState(pose, NULL, 0, 0, ClothingTeleportMode::Continuous);
	}

	virtual const PxMat44 getPose() const
	{
		READ_ZONE();
		return impl.getGlobalPose();
	}

	virtual void lockRenderResources()
	{
		impl.lockRenderResources();
	}

	virtual void unlockRenderResources()
	{
		impl.unlockRenderResources();
	}

	virtual void updateRenderResources(bool rewriteBuffers = false, void* userRenderData = 0)
	{
		Renderable* renderable = impl.getRenderable();
		if (renderable != NULL)
		{
			renderable->updateRenderResources(rewriteBuffers, userRenderData);
		}
	}

	virtual void dispatchRenderResources(UserRenderer& renderer)
	{
		WRITE_ZONE();
		Renderable* renderable = impl.getRenderable();
		if (renderable != NULL)
		{
			renderable->dispatchRenderResources(renderer);
		}
	}

	virtual PxBounds3 getBounds() const
	{
		READ_ZONE();
		return impl.getBounds();
	}


	virtual void updateState(const PxMat44& globalPose, const PxMat44* newBoneMatrices, uint32_t boneMatricesByteStride, uint32_t numBoneMatrices)
	{
		WRITE_ZONE();
		impl.updateState(globalPose, newBoneMatrices, boneMatricesByteStride, numBoneMatrices, ClothingTeleportMode::Continuous);
	}

	void destroy()
	{
		impl.destroy();
		delete this;
	}

	virtual ~ClothingPreviewProxy() {}
};

}
} // namespace nvidia

#endif // CLOTHING_PREVIEW_PROXY_H
