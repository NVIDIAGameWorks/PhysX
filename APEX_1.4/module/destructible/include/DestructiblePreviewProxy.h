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



#ifndef __DESTRUCTIBLEPREVIEW_PROXY_H__
#define __DESTRUCTIBLEPREVIEW_PROXY_H__

#include "Apex.h"
#include "DestructiblePreview.h"
#include "DestructiblePreviewImpl.h"
#include "PsUserAllocated.h"
#include "ApexRWLockable.h"
#include "ReadCheck.h"
#include "WriteCheck.h"

namespace nvidia
{
namespace destructible
{

class DestructiblePreviewProxy : public DestructiblePreview, public ApexResourceInterface, public UserAllocated, public ApexRWLockable
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	DestructiblePreviewImpl impl;

#pragma warning(disable : 4355) // disable warning about this pointer in argument list
	DestructiblePreviewProxy(DestructibleAssetImpl& asset, ResourceList& list, const NvParameterized::Interface* params)
		: impl(this, asset, params)
	{
		list.add(*this);
	}

	virtual ~DestructiblePreviewProxy()
	{
	}

	// AssetPreview methods
	virtual void setPose(const PxMat44& pose)
	{
		WRITE_ZONE();
		impl.setPose(pose);
	}

	virtual const PxMat44 getPose() const
	{
		READ_ZONE();
		return impl.getPose();
	}

	// DestructiblePreview methods

	virtual const RenderMeshActor* getRenderMeshActor() const
	{
		READ_ZONE();
		return const_cast<const RenderMeshActor*>(impl.getRenderMeshActor());
	}

	virtual void setExplodeView(uint32_t depth, float explode)
	{
		WRITE_ZONE();
		return impl.setExplodeView(depth, explode);
	}

	// ApexInterface methods
	virtual void release()
	{
		WRITE_ZONE();
		impl.release();
		delete this;
	}
	virtual void destroy()
	{
		impl.destroy();
	}

	// Renderable methods
	virtual void updateRenderResources(bool rewriteBuffers, void* userRenderData)
	{
		URR_SCOPE;
		impl.updateRenderResources(rewriteBuffers, userRenderData);
	}
	virtual void dispatchRenderResources(UserRenderer& api)
	{
		impl.dispatchRenderResources(api);
	}
	virtual PxBounds3 getBounds() const
	{
		return impl.getBounds();
	}
	virtual void lockRenderResources()
	{
		impl.ApexRenderable::renderDataLock();
	}
	virtual void unlockRenderResources()
	{
		impl.ApexRenderable::renderDataUnLock();
	}

	// ApexResourceInterface methods
	virtual void	setListIndex(ResourceList& list, uint32_t index)
	{
		impl.m_listIndex = index;
		impl.m_list = &list;
	}
	virtual uint32_t	getListIndex() const
	{
		return impl.m_listIndex;
	}
};

}
} // end namespace nvidia

#endif // __DESTRUCTIBLEPREVIEW_PROXY_H__
