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

#include "D3D11RendererVertexBuffer.h"
#include <RendererVertexBufferDesc.h>

#if PX_SUPPORT_GPU_PHYSX
#include <cudamanager/PxCudaContextManager.h>
#endif

using namespace SampleRenderer;

static D3D_FEATURE_LEVEL gFeatureLevel = D3D_FEATURE_LEVEL_9_1;

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

static DXGI_FORMAT getD3DFormat(RendererVertexBuffer::Format format)
{
	DXGI_FORMAT d3dFormat = DXGI_FORMAT_UNKNOWN;
	if(gFeatureLevel <= D3D_FEATURE_LEVEL_9_3)
	{
		switch (format)
		{
		case RendererVertexBuffer::FORMAT_FLOAT1:
			d3dFormat = DXGI_FORMAT_R32_FLOAT;
			break;
		case RendererVertexBuffer::FORMAT_FLOAT2:
			d3dFormat = DXGI_FORMAT_R32G32_FLOAT;
			break;
		case RendererVertexBuffer::FORMAT_FLOAT3:
			d3dFormat = DXGI_FORMAT_R32G32B32_FLOAT;
			break;
		case RendererVertexBuffer::FORMAT_FLOAT4:
			d3dFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
			break;
		case RendererVertexBuffer::FORMAT_UBYTE4:
			d3dFormat = DXGI_FORMAT_R8G8B8A8_UINT;
			break;
		case RendererVertexBuffer::FORMAT_USHORT4:
			d3dFormat = DXGI_FORMAT_R16G16B16A16_UINT;
			break;	
		case RendererVertexBuffer::FORMAT_COLOR_BGRA:
			d3dFormat = DXGI_FORMAT_R8G8B8A8_UINT;
			break;
		case RendererVertexBuffer::FORMAT_COLOR_RGBA:
			d3dFormat = DXGI_FORMAT_R8G8B8A8_UINT;
			break;
		case RendererVertexBuffer::FORMAT_COLOR_NATIVE:
			d3dFormat = DXGI_FORMAT_R32_FLOAT;
			break;
		}
	}
	else
	{
		switch (format)
		{
		case RendererVertexBuffer::FORMAT_FLOAT1:
			d3dFormat = DXGI_FORMAT_R32_FLOAT;
			break;
		case RendererVertexBuffer::FORMAT_FLOAT2:
			d3dFormat = DXGI_FORMAT_R32G32_FLOAT;
			break;
		case RendererVertexBuffer::FORMAT_FLOAT3:
			d3dFormat = DXGI_FORMAT_R32G32B32_FLOAT;
			break;
		case RendererVertexBuffer::FORMAT_FLOAT4:
			d3dFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
			break;
		case RendererVertexBuffer::FORMAT_UBYTE4:
			d3dFormat = DXGI_FORMAT_R8G8B8A8_UINT;
			break;
		case RendererVertexBuffer::FORMAT_USHORT4:
			d3dFormat = DXGI_FORMAT_R16G16B16A16_UINT;
			break;
		case RendererVertexBuffer::FORMAT_COLOR_BGRA:
			d3dFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
			break;
		case RendererVertexBuffer::FORMAT_COLOR_RGBA:
			d3dFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			break;
		case RendererVertexBuffer::FORMAT_COLOR_NATIVE:
			//d3dFormat = DXGI_FORMAT_R32G32B32_FLOAT;
			d3dFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			break;
		}
	}
	RENDERER_ASSERT(d3dFormat != DXGI_FORMAT_UNKNOWN, "Invalid DIRECT3D11 vertex type.");
	return d3dFormat;
}

static void getD3DUsage(RendererVertexBuffer::Semantic semantic, LPCSTR& usageName, PxU8& usageIndex)
{
	usageName = "POSITION";
	usageIndex = 0;
	if (semantic >= RendererVertexBuffer::SEMANTIC_TEXCOORD0 && semantic <= RendererVertexBuffer::SEMANTIC_TEXCOORDMAX)
	{
		usageName  = "TEXCOORD";
		usageIndex = (PxU8)(semantic - RendererVertexBuffer::SEMANTIC_TEXCOORD0);
	}
	else
	{
		switch (semantic)
		{
		case RendererVertexBuffer::SEMANTIC_POSITION:
			usageName = "POSITION";
			break;
		case RendererVertexBuffer::SEMANTIC_NORMAL:
			usageName = "NORMAL";
			break;
		case RendererVertexBuffer::SEMANTIC_TANGENT:
			usageName = "TANGENT";
			break;
		case RendererVertexBuffer::SEMANTIC_COLOR:
			usageName = "COLOR";
			break;
		case RendererVertexBuffer::SEMANTIC_BONEINDEX:
			usageName   = "TEXCOORD";
			usageIndex = RENDERER_BONEINDEX_CHANNEL;
			break;
		case RendererVertexBuffer::SEMANTIC_BONEWEIGHT:
			usageName   = "TEXCOORD";
			usageIndex = RENDERER_BONEWEIGHT_CHANNEL;
			break;
		case RendererVertexBuffer::SEMANTIC_DISPLACEMENT_TEXCOORD:
			usageName   = "TEXCOORD";
			usageIndex = RENDERER_DISPLACEMENT_CHANNEL;
			break;
		case RendererVertexBuffer::SEMANTIC_DISPLACEMENT_FLAGS:
			usageName   = "TEXCOORD";
			usageIndex = RENDERER_DISPLACEMENT_FLAGS_CHANNEL;
			break;
		}
	}
}

D3D11RendererVertexBuffer::D3D11RendererVertexBuffer(ID3D11Device& d3dDevice, ID3D11DeviceContext& d3dDeviceContext, const RendererVertexBufferDesc& desc, bool bUseMapForLocking) :
	RendererVertexBuffer(desc),
	m_d3dDevice(d3dDevice),
	m_d3dDeviceContext(d3dDeviceContext),
	m_d3dVertexBuffer(NULL),
	m_bUseMapForLocking(bUseMapForLocking && (!desc.registerInCUDA)),
	m_buffer(NULL)
{
	gFeatureLevel = d3dDevice.GetFeatureLevel();

	memset(&m_d3dBufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
	m_d3dBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	m_d3dBufferDesc.ByteWidth = (UINT)(desc.maxVertices * m_stride);

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

	m_bBufferWritten = false;

	onDeviceReset();

	if (m_d3dVertexBuffer)
	{
		m_maxVertices = desc.maxVertices;
	}
}

D3D11RendererVertexBuffer::~D3D11RendererVertexBuffer(void)
{
	if (m_d3dVertexBuffer)
	{
#if PX_WINDOWS && PX_SUPPORT_GPU_PHYSX
		if (m_interopContext && m_registeredInCUDA)
		{
			m_registeredInCUDA = !m_interopContext->unregisterResourceInCuda(m_InteropHandle);
		}
#endif
		m_d3dVertexBuffer->Release();
		m_d3dVertexBuffer = NULL;
	}

	delete [] m_buffer;
}

void D3D11RendererVertexBuffer::addVertexElements(PxU32 streamIndex, std::vector<D3D11_INPUT_ELEMENT_DESC> &vertexElements) const
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
			                         D3D11_INPUT_PER_VERTEX_DATA,
			                         0));
		}
	}
}

void D3D11RendererVertexBuffer::swizzleColor(void* colors, PxU32 stride, PxU32 numColors, RendererVertexBuffer::Format inFormat)
{
/*
	if (inFormat == RendererVertexBuffer::FORMAT_COLOR_BGRA)
	{
		const void* end = ((PxU8*)colors) + (stride * numColors);
		for (PxU8* iterator = (PxU8*)colors; iterator < end; iterator += stride)
		{
			std::swap(((PxU8*)iterator)[0], ((PxU8*)iterator)[2]);
		}
	}
*/
}

void* D3D11RendererVertexBuffer::lock(void)
{
	// For now NO_OVERWRITE is the only mapping that functions properly
	return internalLock(getHint() == HINT_STATIC ? /* D3D11_MAP_WRITE_DISCARD */ D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE_NO_OVERWRITE);
}

void* D3D11RendererVertexBuffer::internalLock(D3D11_MAP MapType)
{
	void* lockedBuffer = 0;
	if (m_d3dVertexBuffer)
	{
		if (m_bUseMapForLocking)
		{
			D3D11_MAPPED_SUBRESOURCE mappedRead;
			m_d3dDeviceContext.Map(m_d3dVertexBuffer, 0, MapType, NULL, &mappedRead);
			RENDERER_ASSERT(mappedRead.pData, "Failed to lock DIRECT3D11 Vertex Buffer.");
			lockedBuffer = mappedRead.pData;
		}
		else
		{
			lockedBuffer = m_buffer;
		}
	}
	m_bBufferWritten = true;
	return lockedBuffer;
}

void D3D11RendererVertexBuffer::unlock(void)
{
	if (m_d3dVertexBuffer)
	{
		if (m_bUseMapForLocking)
		{
			m_d3dDeviceContext.Unmap(m_d3dVertexBuffer, 0);
		}
		else
		{
			m_d3dDeviceContext.UpdateSubresource(m_d3dVertexBuffer, 0, NULL, m_buffer, m_d3dBufferDesc.ByteWidth, 0);
		}
	}
}

void D3D11RendererVertexBuffer::bind(PxU32 streamID, PxU32 firstVertex)
{
	prepareForRender();
	if (m_d3dVertexBuffer)
	{
		ID3D11Buffer* pBuffers[1] = { m_d3dVertexBuffer };
		UINT strides[1] = { m_stride };
		UINT offsets[1] = { firstVertex* m_stride };
		m_d3dDeviceContext.IASetVertexBuffers(streamID, 1, pBuffers, strides, offsets);
	}
}

void D3D11RendererVertexBuffer::unbind(PxU32 streamID)
{
	ID3D11Buffer* pBuffers[1] = { NULL };
	UINT strides[1] = { 0 };
	UINT offsets[1] = { 0 };
	m_d3dDeviceContext.IASetVertexBuffers(streamID, 1, pBuffers, strides, offsets);
}

void D3D11RendererVertexBuffer::onDeviceLost(void)
{
#if PX_WINDOWS && PX_SUPPORT_GPU_PHYSX
	if (m_interopContext && m_registeredInCUDA)
	{
		m_interopContext->unregisterResourceInCuda(m_InteropHandle);
	}
#endif

	m_registeredInCUDA = false;

	if (m_d3dVertexBuffer)
	{
		m_d3dVertexBuffer->Release();
		m_d3dVertexBuffer = 0;
	}
}

void D3D11RendererVertexBuffer::onDeviceReset(void)
{
	if (!m_d3dVertexBuffer)
	{
		m_d3dDevice.CreateBuffer(&m_d3dBufferDesc, NULL, &m_d3dVertexBuffer);
		RENDERER_ASSERT(m_d3dVertexBuffer, "Failed to create DIRECT3D11 Vertex Buffer.");
#if PX_WINDOWS && PX_SUPPORT_GPU_PHYSX
		if (m_interopContext && m_d3dVertexBuffer && m_mustBeRegisteredInCUDA)
		{
			RENDERER_ASSERT(m_deferredUnlock == false, "Deferred VB Unlock must be disabled when CUDA Interop is in use.")
			m_registeredInCUDA = m_interopContext->registerResourceInCudaD3D(m_InteropHandle, m_d3dVertexBuffer);
		}
#endif
		m_bBufferWritten = false;
	}
}

bool D3D11RendererVertexBuffer::checkBufferWritten(void)
{
	if (m_InteropHandle)
	{
		return true;
	}
	else
	{
		return m_bBufferWritten;
	}
}

#endif // #if defined(RENDERER_ENABLE_DIRECT3D11)
