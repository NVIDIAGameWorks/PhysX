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



#ifndef __DESTRUCTIBLE_RENDERABLE_IMPL_H__
#define __DESTRUCTIBLE_RENDERABLE_IMPL_H__

#include "Apex.h"
#include "DestructibleActor.h"
#include "DestructibleRenderable.h"
#include "ApexRWLockable.h"
#include "ApexActor.h"
#if APEX_RUNTIME_FRACTURE
#include "../fracture/Renderable.h"
#endif

#include "ReadCheck.h"

namespace nvidia
{
namespace apex
{
class RenderMeshActor;
}

namespace destructible
{

class DestructibleActorImpl;

class DestructibleRenderableImpl : public DestructibleRenderable, public ApexRenderable, public UserAllocated, public ApexRWLockable
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	DestructibleRenderableImpl(RenderMeshActor* renderMeshActors[DestructibleActorMeshType::Count], DestructibleAssetImpl* asset, int32_t listIndex);
	~DestructibleRenderableImpl();

	// Begin DestructibleRenderable methods
	virtual RenderMeshActor*	getRenderMeshActor(DestructibleActorMeshType::Enum type = DestructibleActorMeshType::Skinned) const
	{
		READ_ZONE();
		return (uint32_t)type < DestructibleActorMeshType::Count ? mRenderMeshActors[type] : NULL;
	}

	virtual void				release();
	// End DestructibleRenderable methods

	// Begin Renderable methods
	virtual	void				updateRenderResources(bool rewriteBuffers, void* userRenderData);

	virtual	void				dispatchRenderResources(UserRenderer& api);

	virtual	PxBounds3			getBounds() const
	{
		PxBounds3 bounds = ApexRenderable::getBounds();
#if APEX_RUNTIME_FRACTURE
		bounds.include(mRTrenderable.getBounds());
#endif
		return bounds;
	}

	virtual	void				lockRenderResources()
	{
		ApexRenderable::renderDataLock();
	}

	virtual	void				unlockRenderResources()
	{
		ApexRenderable::renderDataUnLock();
	}
	// End Renderable methods

	// Begin DestructibleRenderable methods
	// Returns this if successful, NULL otherwise
	DestructibleRenderableImpl*		incrementReferenceCount();

	int32_t						getReferenceCount()
	{
		return mRefCount;
	}

	void						setBounds(const PxBounds3& bounds)
	{
		mRenderBounds = bounds;
	}
	// End DestructibleRenderable methods

#if APEX_RUNTIME_FRACTURE
	::nvidia::fracture::Renderable& getRTrenderable() { return mRTrenderable; }
#endif

private:

	RenderMeshActor*	mRenderMeshActors[DestructibleActorMeshType::Count];	// Indexed by DestructibleActorMeshType::Enum
	DestructibleAssetImpl*	mAsset;
	int32_t				mListIndex;
	volatile int32_t		mRefCount;
#if APEX_RUNTIME_FRACTURE
	::nvidia::fracture::Renderable mRTrenderable;
#endif
};

}
} // end namespace nvidia

#endif // __DESTRUCTIBLE_RENDERABLE_IMPL_H__
