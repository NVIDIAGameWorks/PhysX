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
#if defined(RENDERER_ENABLE_DIRECT3D11)

#include "D3D11RendererInstanceBuffer.h"
#include <RendererInstanceBufferDesc.h>

#if PX_SUPPORT_GPU_PHYSX
#include <cudamanager/PxCudaContextManager.h>
#endif

using namespace SampleRenderer;

static D3D11_INPUT_ELEMENT_DESC buildVertexElement(LPCSTR name, UINT index, DXGI_FORMAT format, UINT inputSlot, UINT alignedByOffset, D3D11_INPUT_CLASSIFICATION inputSlotClass, UINT instanceDataStepRate)
{
	D3D11_INPUT_ELEMENT_DESC element;
	element.SemanticName = name;
	element.SemanticIndex = index;
	element.Format = format;
	element.InputSlot = inputSlot;
	element.AlignedByteOffset = alignedByOffset;
	element.InputSlotClass = inputSlotClass;
	element.InstanceDataStepRate = instanceDataStepRate;
	return element;
}

static DXGI_FORMAT getD3DFormat(RendererInstanceBuffer::Format format)
{
	DXGI_FORMAT d3dFormat = DXGI_FORMAT_UNKNOWN;
	switch (format)
	{
	case RendererInstanceBuffer::FORMAT_FLOAT1:
		d3dFormat = DXGI_FORMAT_R32_FLOAT;
		break;
	case RendererInstanceBuffer::FORMAT_FLOAT2:
		d3dFormat = DXGI_FORMAT_R32G32_FLOAT;
		break;
	case RendererInstanceBuffer::FORMAT_FLOAT3:
		d3dFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		break;
	case RendererInstanceBuffer::FORMAT_FLOAT4:
		d3dFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		break;
	}
	RENDERER_ASSERT(d3dFormat != DXGI_FORMAT_UNKNOWN, "Invalid DIRECT3D11 vertex type.");
	return d3dFormat;
}

static void getD3DUsage(RendererInstanceBuffer::Semantic semantic, LPCSTR& usageName, PxU8& usageIndex)
{
	if (semantic >= RendererInstanceBuffer::SEMANTIC_POSITION && semantic < RendererInstanceBuffer::NUM_SEMANTICS)
	{
		usageName = "TEXCOORD";
		switch (semantic)
		{
		case RendererInstanceBuffer::SEMANTIC_POSITION:
			usageIndex = RENDERER_INSTANCE_POSITION_CHANNEL;
			break;
		case RendererInstanceBuffer::SEMANTIC_NORMALX:
			usageIndex = RENDERER_INSTANCE_NORMALX_CHANNEL;
			break;
		case RendererInstanceBuffer::SEMANTIC_NORMALY:
			usageIndex = RENDERER_INSTANCE_NORMALY_CHANNEL;
			break;
		case RendererInstanceBuffer::SEMANTIC_NORMALZ:
			usageIndex = RENDERER_INSTANCE_NORMALZ_CHANNEL;
			break;
		case RendererInstanceBuffer::SEMANTIC_VELOCITY_LIFE:
			usageIndex = RENDERER_INSTANCE_VEL_LIFE_CHANNEL;
			break;
		case RendererInstanceBuffer::SEMANTIC_DENSITY:
			usageIndex = RENDERER_INSTANCE_DENSITY_CHANNEL;
			break;
		case RendererInstanceBuffer::SEMANTIC_UV_OFFSET:
			usageIndex = RENDERER_INSTANCE_UV_CHANNEL;
			break;
		case RendererInstanceBuffer::SEMANTIC_LOCAL_OFFSET:
			usageIndex = RENDERER_INSTANCE_LOCAL_CHANNEL;
			break;
		}
	}
	else
	{
		RENDERER_ASSERT(false, "Invalid Direct3D11 instance usage.");
	}
}

D3D11RendererInstanceBuffer::D3D11RendererInstanceBuffer(ID3D11Device& d3dDevice, ID3D11DeviceContext& d3dDeviceContext, const RendererInstanceBufferDesc& desc, bool bUseMapForLocking) :
	RendererInstanceBuffer(desc)
	, m_d3dDevice(d3dDevice)
	, m_d3dDeviceContext(d3dDeviceContext)
	, m_d3dInstanceBuffer(NULL)
	, m_bUseMapForLocking(bUseMapForLocking)
	, m_buffer(NULL)
{
	memset(&m_d3dBufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
	m_d3dBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	m_d3dBufferDesc.ByteWidth = (UINT)(desc.maxInstances * m_stride);

	if (m_bUseMapForLocking)
	{
		m_d3dBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		m_d3dBufferDesc.Usage          = D3D11_USAGE_DYNAMIC;
	}
	else
	{
		m_d3dBufferDesc.CPUAccessFlags = 0;
		m_d3dBufferDesc.Usage          = D3D11_USAGE_DEFAULT;
		m_buffer = new PxU8[m_d3dBufferDesc.ByteWidth];
		memset(m_buffer, 0, sizeof(PxU8)*m_d3dBufferDesc.ByteWidth);
	}

	onDeviceReset();

	if (m_d3dInstanceBuffer)
	{
		m_maxInstances = desc.maxInstances;
	}
}

D3D11RendererInstanceBuffer::~D3D11RendererInstanceBuffer(void)
{
	if (m_d3dInstanceBuffer)
	{
#if PX_WINDOWS && PX_SUPPORT_GPU_PHYSX
		if (m_interopContext && m_registeredInCUDA)
		{
			m_registeredInCUDA = !m_interopContext->unregisterResourceInCuda(m_InteropHandle);
		}
#endif
		m_d3dInstanceBuffer->Release();
		m_d3dInstanceBuffer = NULL;
	}

	delete [] m_buffer;
}

void D3D11RendererInstanceBuffer::addVertexElements(PxU32 streamIndex, std::vector<D3D11_INPUT_ELEMENT_DESC> &vertexElements) const
{
	for (PxU32 i = 0; i < NUM_SEMANTICS; i++)
	{
		Semantic semantic = (Semantic)i;
		const SemanticDesc& sm = m_semanticDescs[semantic];
		if (sm.format < NUM_FORMATS)
		{
			PxU8 d3dUsageIndex  = 0;
			LPCSTR d3dUsageName = "";
			getD3DUsage(semantic, d3dUsageName, d3dUsageIndex);
			vertexElements.push_back(buildVertexElement(d3dUsageName,
			                         d3dUsageIndex,
			                         getD3DFormat(sm.format),
			                         streamIndex,
			                         (UINT)sm.offset,
			                         D3D11_INPUT_PER_INSTANCE_DATA,
			                         1));
		}
	}
}

void* D3D11RendererInstanceBuffer::lock(void)
{
	return internalLock(getHint() == HINT_STATIC ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE);
}

void* D3D11RendererInstanceBuffer::internalLock(D3D11_MAP MapType)
{
	void* lockedBuffer = 0;
	if (m_d3dInstanceBuffer)
	{
		if (m_bUseMapForLocking)
		{
			D3D11_MAPPED_SUBRESOURCE mappedRead;
			m_d3dDeviceContext.Map(m_d3dInstanceBuffer, 0, MapType, NULL, &mappedRead);
			RENDERER_ASSERT(mappedRead.pData, "Failed to lock DIRECT3D11 Vertex Buffer.");
			lockedBuffer = mappedRead.pData;
		}
		else
		{
			lockedBuffer = m_buffer;
		}
	}
	return lockedBuffer;
}

void D3D11RendererInstanceBuffer::unlock(void)
{
	if (m_d3dInstanceBuffer)
	{
		if (m_bUseMapForLocking)
		{
			m_d3dDeviceContext.Unmap(m_d3dInstanceBuffer, 0);	
		}
		else
		{
			m_d3dDeviceContext.UpdateSubresource(m_d3dInstanceBuffer, 0, NULL, m_buffer, m_d3dBufferDesc.ByteWidth, 0);
		}
	}
}

void D3D11RendererInstanceBuffer::bind(PxU32 streamID, PxU32 firstInstance) const
{
	if (m_d3dInstanceBuffer)
	{
		ID3D11Buffer* pBuffers[1] = { m_d3dInstanceBuffer };
		UINT strides[1] = { m_stride };
		UINT offsets[1] = { firstInstance* m_stride };
		m_d3dDeviceContext.IASetVertexBuffers(streamID, 1, pBuffers, strides, offsets);
	}
}

void D3D11RendererInstanceBuffer::unbind(PxU32 streamID) const
{
	ID3D11Buffer* pBuffers[1] = { NULL };
	UINT strides[1] = { 0 };
	UINT offsets[1] = { 0 };
	m_d3dDeviceContext.IASetVertexBuffers(streamID, 1, pBuffers, strides, offsets);
}

void D3D11RendererInstanceBuffer::onDeviceLost(void)
{
#if PX_WINDOWS && PX_SUPPORT_GPU_PHYSX
	if (m_interopContext && m_registeredInCUDA)
	{
		PX_ASSERT(m_d3dInstanceBuffer);
		m_interopContext->unregisterResourceInCuda(m_InteropHandle);
	}

	m_registeredInCUDA = false;
#endif
	if (m_d3dInstanceBuffer)
	{
		m_d3dInstanceBuffer->Release();
		m_d3dInstanceBuffer = 0;
	}
}

void D3D11RendererInstanceBuffer::onDeviceReset(void)
{
	if (!m_d3dInstanceBuffer)
	{
		m_d3dDevice.CreateBuffer(&m_d3dBufferDesc, NULL, &m_d3dInstanceBuffer);
		RENDERER_ASSERT(m_d3dInstanceBuffer, "Failed to create DIRECT3D11 Vertex Buffer.");
#if PX_WINDOWS && PX_SUPPORT_GPU_PHYSX
		if (m_interopContext && m_d3dInstanceBuffer && m_mustBeRegisteredInCUDA)
		{
			m_registeredInCUDA = m_interopContext->registerResourceInCudaD3D(m_InteropHandle, m_d3dInstanceBuffer);
		}
#endif
	}
}

#endif // #if defined(RENDERER_ENABLE_DIRECT3D11)
