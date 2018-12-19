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


#ifndef APEX_RENDER_RESOURCE_MANAGER_H
#define APEX_RENDER_RESOURCE_MANAGER_H

#include "Apex.h"
#include "ApexRenderResources.h"

#include "PsString.h"

#pragma warning(push)
#pragma warning(disable : 4917)
#pragma warning(disable : 4365)
#include <d3d11.h>
#pragma warning(pop)

using namespace nvidia::apex;

class ApexRenderResourceManager : public UserRenderResourceManager
{
  public:
	ApexRenderResourceManager(ID3D11Device* d) : mDevice(d) {}

	virtual UserRenderVertexBuffer* createVertexBuffer(const UserRenderVertexBufferDesc& desc)
	{
		return new SampleApexRendererVertexBuffer(*mDevice, desc);
	}
	virtual void releaseVertexBuffer(UserRenderVertexBuffer& buffer)
	{
		delete &buffer;
	}

	virtual UserRenderIndexBuffer* createIndexBuffer(const UserRenderIndexBufferDesc& desc)
	{
		return new SampleApexRendererIndexBuffer(*mDevice, desc);
	}
	virtual void releaseIndexBuffer(UserRenderIndexBuffer& buffer)
	{
		delete &buffer;
	}

	virtual UserRenderBoneBuffer* createBoneBuffer(const UserRenderBoneBufferDesc& desc)
	{
		return new SampleApexRendererBoneBuffer(*mDevice, desc);
	}
	virtual void releaseBoneBuffer(UserRenderBoneBuffer& buffer)
	{
		delete &buffer;
	}

	virtual UserRenderInstanceBuffer* createInstanceBuffer(const UserRenderInstanceBufferDesc& desc)
	{
		return new SampleApexRendererInstanceBuffer(*mDevice, desc);
	}
	virtual void releaseInstanceBuffer(UserRenderInstanceBuffer& buffer)
	{
		delete &buffer;
	}

	virtual UserRenderSpriteBuffer* createSpriteBuffer(const UserRenderSpriteBufferDesc& desc)
	{
		return new SampleApexRendererSpriteBuffer(*mDevice, desc);
	}
	virtual void releaseSpriteBuffer(UserRenderSpriteBuffer& buffer)
	{
		delete &buffer;
	}

	virtual UserRenderSurfaceBuffer* createSurfaceBuffer(const UserRenderSurfaceBufferDesc&)
	{
		physx::shdfnd::printString("createSurfaceBuffer");
		return NULL;
	}
	virtual void releaseSurfaceBuffer(UserRenderSurfaceBuffer&)
	{
	}

	virtual UserRenderResource* createResource(const UserRenderResourceDesc& desc)
	{
		return new SampleApexRendererMesh(*mDevice, desc);
	}
	virtual void releaseResource(UserRenderResource& buffer)
	{
		delete &buffer;
	}

	virtual uint32_t getMaxBonesForMaterial(void*)
	{
		return 0;
	}

	virtual bool getSpriteLayoutData(uint32_t spriteCount, uint32_t spriteSemanticsBitmap, UserRenderSpriteBufferDesc* vertexDescArray);
	virtual bool getInstanceLayoutData(uint32_t spriteCount, uint32_t particleSemanticsBitmap, UserRenderInstanceBufferDesc* instanceDescArray);

  private:
	ID3D11Device* mDevice;
};

#endif