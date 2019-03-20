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

#include <RendererConfig.h>
#if defined(RENDERER_ENABLE_DIRECT3D9)

#include "D3D9RendererInstanceBuffer.h"
#include <RendererInstanceBufferDesc.h>

#if PX_SUPPORT_GPU_PHYSX
#include <cudamanager/PxCudaContextManager.h>
#endif

using namespace SampleRenderer;

static D3DVERTEXELEMENT9 buildVertexElement(WORD stream, WORD offset, D3DDECLTYPE type, BYTE method, BYTE usage, BYTE usageIndex)
{
	D3DVERTEXELEMENT9 element;
	element.Stream     = stream;
	element.Offset     = offset;
#if defined(RENDERER_WINDOWS)
	element.Type       = (BYTE)type;
#else
	element.Type       = type;
#endif
	element.Method     = method;
	element.Usage      = usage;
	element.UsageIndex = usageIndex;
	return element;
}

static D3DDECLTYPE getD3DType(RendererInstanceBuffer::Format format)
{
	D3DDECLTYPE d3dType = D3DDECLTYPE_UNUSED;
	switch(format)
	{
	case RendererInstanceBuffer::FORMAT_FLOAT1:  d3dType = D3DDECLTYPE_FLOAT1;   break;
	case RendererInstanceBuffer::FORMAT_FLOAT2:  d3dType = D3DDECLTYPE_FLOAT2;   break;
	case RendererInstanceBuffer::FORMAT_FLOAT3:  d3dType = D3DDECLTYPE_FLOAT3;   break;
	case RendererInstanceBuffer::FORMAT_FLOAT4:  d3dType = D3DDECLTYPE_FLOAT4;   break;
	}
	RENDERER_ASSERT(d3dType != D3DDECLTYPE_UNUSED, "Invalid Direct3D9 vertex type.");
	return d3dType;
}

static D3DDECLUSAGE getD3DUsage(RendererInstanceBuffer::Semantic semantic, PxU8 &usageIndex)
{
	D3DDECLUSAGE d3dUsage = D3DDECLUSAGE_FOG;
	usageIndex = 0;
	switch(semantic)
	{
	case RendererInstanceBuffer::SEMANTIC_POSITION:
		d3dUsage   = D3DDECLUSAGE_TEXCOORD;
		usageIndex = RENDERER_INSTANCE_POSITION_CHANNEL;
		break;
	case RendererInstanceBuffer::SEMANTIC_NORMALX:
		d3dUsage   = D3DDECLUSAGE_TEXCOORD;
		usageIndex = RENDERER_INSTANCE_NORMALX_CHANNEL;
		break;
	case RendererInstanceBuffer::SEMANTIC_NORMALY:
		d3dUsage   = D3DDECLUSAGE_TEXCOORD;
		usageIndex = RENDERER_INSTANCE_NORMALY_CHANNEL;
		break;
	case RendererInstanceBuffer::SEMANTIC_NORMALZ:
		d3dUsage   = D3DDECLUSAGE_TEXCOORD;
		usageIndex = RENDERER_INSTANCE_NORMALZ_CHANNEL;
		break;
	case RendererInstanceBuffer::SEMANTIC_VELOCITY_LIFE:
		d3dUsage   = D3DDECLUSAGE_TEXCOORD;
		usageIndex = RENDERER_INSTANCE_VEL_LIFE_CHANNEL;
		break;
	case RendererInstanceBuffer::SEMANTIC_DENSITY:
		d3dUsage   = D3DDECLUSAGE_TEXCOORD;
		usageIndex = RENDERER_INSTANCE_DENSITY_CHANNEL;
		break;
	case RendererInstanceBuffer::SEMANTIC_UV_OFFSET:
		d3dUsage   = D3DDECLUSAGE_TEXCOORD;
		usageIndex = RENDERER_INSTANCE_UV_CHANNEL;
		break;
	case RendererInstanceBuffer::SEMANTIC_LOCAL_OFFSET:
		d3dUsage   = D3DDECLUSAGE_TEXCOORD;
		usageIndex = RENDERER_INSTANCE_LOCAL_CHANNEL;
		break;
	}
	RENDERER_ASSERT(d3dUsage != D3DDECLUSAGE_FOG, "Invalid Direct3D9 vertex usage.");
	return d3dUsage;
}

D3D9RendererInstanceBuffer::D3D9RendererInstanceBuffer(IDirect3DDevice9 &d3dDevice, D3D9Renderer& renderer, const RendererInstanceBufferDesc &desc) :
RendererInstanceBuffer(desc)
#if RENDERER_INSTANCING
	,m_d3dDevice(d3dDevice)
#endif
{
#if RENDERER_INSTANCING
	m_d3dVertexBuffer = 0;
#endif

	m_usage      = D3DUSAGE_WRITEONLY;
	m_pool       = renderer.canUseManagedResources() ? D3DPOOL_MANAGED : D3DPOOL_DEFAULT;
	m_bufferSize = (UINT)(desc.maxInstances * m_stride);

#if RENDERER_ENABLE_DYNAMIC_VB_POOLS
	if(desc.hint==RendererInstanceBuffer::HINT_DYNAMIC )
	{
		m_usage = desc.registerInCUDA ? 0 : D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
		m_pool  = D3DPOOL_DEFAULT;
	}
#endif

	onDeviceReset();

#if RENDERER_INSTANCING	
	if(m_d3dVertexBuffer)
	{
		m_maxInstances = desc.maxInstances;
	}
#else
	m_maxInstances = desc.maxInstances;
	mInstanceBuffer = malloc(m_maxInstances*m_stride); // PX_ALLOC(m_maxInstances*m_stride);
#endif
}

D3D9RendererInstanceBuffer::~D3D9RendererInstanceBuffer(void)
{
#if RENDERER_INSTANCING
#if PX_WINDOWS && PX_SUPPORT_GPU_PHYSX
	if(m_interopContext && m_registeredInCUDA)
	{
		PX_ASSERT(m_d3dVertexBuffer);
		m_registeredInCUDA = !m_interopContext->unregisterResourceInCuda(m_InteropHandle);
	}
#endif
	if(m_d3dVertexBuffer) m_d3dVertexBuffer->Release();
#else
	free(mInstanceBuffer); // PX_FREE(mInstanceBuffer);
#endif
}

void D3D9RendererInstanceBuffer::addVertexElements(PxU32 streamIndex, std::vector<D3DVERTEXELEMENT9> &vertexElements) const
{
	for(PxU32 i=0; i<NUM_SEMANTICS; i++)
	{
		Semantic semantic = (Semantic)i;
		const SemanticDesc &sm = m_semanticDescs[semantic];
		if(sm.format < NUM_FORMATS)
		{
			PxU8 d3dUsageIndex  = 0;
			D3DDECLUSAGE d3dUsage = getD3DUsage(semantic, d3dUsageIndex);
			vertexElements.push_back(buildVertexElement((WORD)streamIndex, (WORD)sm.offset, getD3DType(sm.format), D3DDECLMETHOD_DEFAULT, (BYTE)d3dUsage, d3dUsageIndex));
		}
	}
}

void *D3D9RendererInstanceBuffer::lock(void)
{
	PX_PROFILE_ZONE("D3D9RenderIBlock",0);

	void *lockedBuffer = 0;
#if RENDERER_INSTANCING
	if(m_d3dVertexBuffer)
	{
		const PxU32 bufferSize = m_maxInstances * m_stride;
		m_d3dVertexBuffer->Lock(0, (UINT)bufferSize, &lockedBuffer, 0);
		RENDERER_ASSERT(lockedBuffer, "Failed to lock Direct3D9 Vertex Buffer.");
	}
#else
	lockedBuffer = mInstanceBuffer;
#endif
	return lockedBuffer;
}

void D3D9RendererInstanceBuffer::unlock(void)
{
	PX_PROFILE_ZONE("D3D9RenderIBunlock",0);
#if RENDERER_INSTANCING
	if(m_d3dVertexBuffer)
	{
		m_d3dVertexBuffer->Unlock();
	}
#endif
}

void D3D9RendererInstanceBuffer::bind(PxU32 streamID, PxU32 firstInstance) const
{
#if RENDERER_INSTANCING
	if(m_d3dVertexBuffer)
	{
		m_d3dDevice.SetStreamSourceFreq((UINT)streamID, (UINT)(D3DSTREAMSOURCE_INSTANCEDATA | 1));
		m_d3dDevice.SetStreamSource(    (UINT)streamID, m_d3dVertexBuffer, firstInstance*m_stride, m_stride);
	}
#endif
}

void D3D9RendererInstanceBuffer::unbind(PxU32 streamID) const
{
#if RENDERER_INSTANCING
	m_d3dDevice.SetStreamSource((UINT)streamID, 0, 0, 0);
#endif
}

void D3D9RendererInstanceBuffer::onDeviceLost(void)
{
#if RENDERER_INSTANCING

#if PX_WINDOWS && PX_SUPPORT_GPU_PHYSX
	if(m_interopContext && m_registeredInCUDA)
	{
		PX_ASSERT(m_d3dVertexBuffer);
		m_interopContext->unregisterResourceInCuda(m_InteropHandle);
	}
#endif

	m_registeredInCUDA = false;

	if(m_pool != D3DPOOL_MANAGED && m_d3dVertexBuffer)
	{
		m_d3dVertexBuffer->Release();
		m_d3dVertexBuffer = 0;
	}
#endif
}

void D3D9RendererInstanceBuffer::onDeviceReset(void)
{
#if RENDERER_INSTANCING
	if(!m_d3dVertexBuffer)
	{
		m_d3dDevice.CreateVertexBuffer(m_bufferSize, m_usage, 0, m_pool, &m_d3dVertexBuffer, 0);
		RENDERER_ASSERT(m_d3dVertexBuffer, "Failed to create Direct3D9 Vertex Buffer.");

#if PX_WINDOWS && PX_SUPPORT_GPU_PHYSX
		if(m_interopContext && m_d3dVertexBuffer && m_mustBeRegisteredInCUDA)
		{
			m_registeredInCUDA = m_interopContext->registerResourceInCudaD3D(m_InteropHandle, m_d3dVertexBuffer);
		}
#endif
	}
#endif
}

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
