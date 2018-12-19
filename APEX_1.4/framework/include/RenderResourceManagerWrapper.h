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



#ifndef RENDER_RESOURCE_MANAGER_WRAPPER_H
#define RENDER_RESOURCE_MANAGER_WRAPPER_H

#include "ApexSDKImpl.h"

namespace nvidia
{
namespace apex
{


class RenderResourceManagerWrapper : public UserRenderResourceManager, public UserAllocated
{
	PX_NOCOPY(RenderResourceManagerWrapper);
public:
	RenderResourceManagerWrapper(UserRenderResourceManager& rrm) :
	  mRrm(rrm)
	{
	}


	virtual UserRenderVertexBuffer*   createVertexBuffer(const UserRenderVertexBufferDesc& desc)
	{
		URR_CHECK;
		return mRrm.createVertexBuffer(desc);
	}

	virtual void                        releaseVertexBuffer(UserRenderVertexBuffer& buffer)
	{
		mRrm.releaseVertexBuffer(buffer);
	}

	virtual UserRenderIndexBuffer*    createIndexBuffer(const UserRenderIndexBufferDesc& desc)
	{
		URR_CHECK;
		return mRrm.createIndexBuffer(desc);
	}

	virtual void                        releaseIndexBuffer(UserRenderIndexBuffer& buffer) 
	{
		mRrm.releaseIndexBuffer(buffer);
	}

	virtual UserRenderBoneBuffer*     createBoneBuffer(const UserRenderBoneBufferDesc& desc)
	{
		URR_CHECK;
		return mRrm.createBoneBuffer(desc);
	}

	virtual void                        releaseBoneBuffer(UserRenderBoneBuffer& buffer)
	{
		mRrm.releaseBoneBuffer(buffer);
	}

	virtual UserRenderInstanceBuffer* createInstanceBuffer(const UserRenderInstanceBufferDesc& desc)
	{
		URR_CHECK;
		return mRrm.createInstanceBuffer(desc);
	}

	virtual void                        releaseInstanceBuffer(UserRenderInstanceBuffer& buffer)
	{
		mRrm.releaseInstanceBuffer(buffer);
	}

	virtual UserRenderSpriteBuffer*   createSpriteBuffer(const UserRenderSpriteBufferDesc& desc)
	{
		URR_CHECK;
		return mRrm.createSpriteBuffer(desc);
	}

	virtual void                        releaseSpriteBuffer(UserRenderSpriteBuffer& buffer)
	{
		mRrm.releaseSpriteBuffer(buffer);
	}

	virtual UserRenderSurfaceBuffer*  createSurfaceBuffer(const UserRenderSurfaceBufferDesc& desc)
	{
		URR_CHECK;
		return mRrm.createSurfaceBuffer(desc);
	}

	virtual void                        releaseSurfaceBuffer(UserRenderSurfaceBuffer& buffer)
	{
		mRrm.releaseSurfaceBuffer(buffer);
	}

	virtual UserRenderResource*       createResource(const UserRenderResourceDesc& desc)
	{
		URR_CHECK;
		return mRrm.createResource(desc);
	}

	virtual void                        releaseResource(UserRenderResource& resource)
	{
		mRrm.releaseResource(resource);
	}

	virtual uint32_t                       getMaxBonesForMaterial(void* material)
	{
		return mRrm.getMaxBonesForMaterial(material);
	}

	virtual bool getSpriteLayoutData(uint32_t spriteCount, uint32_t spriteSemanticsBitmap, UserRenderSpriteBufferDesc* textureDescArray)
	{
		return mRrm.getSpriteLayoutData(spriteCount, spriteSemanticsBitmap, textureDescArray);
	}

	virtual bool getInstanceLayoutData(uint32_t spriteCount, uint32_t spriteSemanticsBitmap, UserRenderInstanceBufferDesc* instanceDescArray)
	{
		return mRrm.getInstanceLayoutData(spriteCount, spriteSemanticsBitmap, instanceDescArray);
	}

private:
	UserRenderResourceManager& mRrm;
};


}
} // end namespace nvidia::apex


#endif // RENDER_RESOURCE_MANAGER_WRAPPER_H
