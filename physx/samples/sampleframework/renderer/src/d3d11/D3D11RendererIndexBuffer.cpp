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

#include "D3D11RendererIndexBuffer.h"
#include <RendererIndexBufferDesc.h>

#if PX_SUPPORT_GPU_PHYSX
#include <cudamanager/PxCudaContextManager.h>
#endif

using namespace SampleRenderer;

static DXGI_FORMAT getD3D11Format(RendererIndexBuffer::Format format)
{
	DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;
	switch (format)
	{
	case RendererIndexBuffer::FORMAT_UINT16:
		dxgiFormat = DXGI_FORMAT_R16_UINT;
		break;
	case RendererIndexBuffer::FORMAT_UINT32:
		dxgiFormat = DXGI_FORMAT_R32_UINT;
		break;
	}
	RENDERER_ASSERT(dxgiFormat != DXGI_FORMAT_UNKNOWN, "Unable to convert to DXGI_FORMAT.");
	return dxgiFormat;
}

D3D11RendererIndexBuffer::D3D11RendererIndexBuffer(ID3D11Device& d3dDevice, ID3D11DeviceContext& d3dDeviceContext, const RendererIndexBufferDesc& desc, bool bUseMapForLocking) :
	RendererIndexBuffer(desc),
	m_d3dDevice(d3dDevice),
	m_d3dDeviceContext(d3dDeviceContext),
	m_d3dIndexBuffer(NULL),
	m_bUseMapForLocking(bUseMapForLocking && (!desc.registerInCUDA)),
	m_buffer(NULL)
{
	memset(&m_d3dBufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
	m_d3dBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	m_d3dBufferDesc.ByteWidth = (UINT)(getFormatByteSize(desc.format) * desc.maxIndices);
	m_d3dBufferFormat = getD3D11Format(desc.format);

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

	if (m_d3dIndexBuffer)
	{
		m_maxIndices = desc.maxIndices;
	}

}

D3D11RendererIndexBuffer::~D3D11RendererIndexBuffer(void)
{
	if (m_d3dIndexBuffer)
	{
#if PX_WINDOWS && PX_SUPPORT_GPU_PHYSX
		if (m_interopContext && m_registeredInCUDA)
		{
			m_registeredInCUDA = !m_interopContext->unregisterResourceInCuda(m_InteropHandle);
		}
#endif
		m_d3dIndexBuffer->Release();
		m_d3dIndexBuffer = NULL;
	}

	delete [] m_buffer;
}


void D3D11RendererIndexBuffer::onDeviceLost(void)
{
	m_registeredInCUDA = false;

	if (m_d3dIndexBuffer)
	{
#if PX_WINDOWS && PX_SUPPORT_GPU_PHYSX
		if (m_interopContext && m_registeredInCUDA)
		{
			m_registeredInCUDA = !m_interopContext->unregisterResourceInCuda(m_InteropHandle);
		}
#endif
		m_d3dIndexBuffer->Release();
		m_d3dIndexBuffer = 0;
	}
}

void D3D11RendererIndexBuffer::onDeviceReset(void)
{
	if (!m_d3dIndexBuffer)
	{
		m_d3dDevice.CreateBuffer(&m_d3dBufferDesc, NULL, &m_d3dIndexBuffer);
		RENDERER_ASSERT(m_d3dIndexBuffer, "Failed to create DIRECT3D11 Index Buffer.");
#if PX_WINDOWS && PX_SUPPORT_GPU_PHYSX
		if (m_interopContext && m_d3dIndexBuffer && m_mustBeRegisteredInCUDA)
		{
			m_registeredInCUDA = m_interopContext->registerResourceInCudaD3D(m_InteropHandle, m_d3dIndexBuffer);
		}
#endif
	}
}

void* D3D11RendererIndexBuffer::lock(void)
{
	// For now NO_OVERWRITE is the only mapping that functions properly
	return internalLock(getHint() == HINT_STATIC ? /* D3D11_MAP_WRITE_DISCARD */ D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE_NO_OVERWRITE);
}

void* D3D11RendererIndexBuffer::internalLock(D3D11_MAP MapType)
{
	void* buffer = 0;
	if (m_d3dIndexBuffer)
	{
		if (m_bUseMapForLocking)
		{
			D3D11_MAPPED_SUBRESOURCE mappedRead;
			m_d3dDeviceContext.Map(m_d3dIndexBuffer, 0, MapType, NULL, &mappedRead);
			RENDERER_ASSERT(mappedRead.pData, "Failed to lock DIRECT3D11 Index Buffer.");
			buffer = mappedRead.pData;	
		}
		else
		{
			buffer = m_buffer;
		}
	}
	return buffer;
}

void D3D11RendererIndexBuffer::unlock(void)
{
	if (m_d3dIndexBuffer)
	{
		if (m_bUseMapForLocking)
		{
			m_d3dDeviceContext.Unmap(m_d3dIndexBuffer, 0);
		}
		else
		{
			m_d3dDeviceContext.UpdateSubresource(m_d3dIndexBuffer, 0, NULL, m_buffer, m_d3dBufferDesc.ByteWidth, 0);
		}
	}
}

void D3D11RendererIndexBuffer::bind(void) const
{
	m_d3dDeviceContext.IASetIndexBuffer(m_d3dIndexBuffer, m_d3dBufferFormat, 0);
}

void D3D11RendererIndexBuffer::unbind(void) const
{
	m_d3dDeviceContext.IASetIndexBuffer(NULL, DXGI_FORMAT(), 0);
}

#endif // #if defined(RENDERER_ENABLE_DIRECT3D11)
