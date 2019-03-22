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

#include "D3D11RendererTexture3D.h"
#include "D3D11RendererMemoryMacros.h"
#include "D3D11RendererTextureCommon.h"

#include <RendererTextureDesc.h>

using namespace SampleRenderer;

D3D11RendererTexture3D::D3D11RendererTexture3D(ID3D11Device& d3dDevice, ID3D11DeviceContext& d3dDeviceContext, const RendererTexture3DDesc& desc) :
	RendererTexture3D(desc),
	m_d3dDevice(d3dDevice),
	m_d3dDeviceContext(d3dDeviceContext),
	m_d3dTexture(NULL),
	m_d3dSamplerState(NULL),
	m_d3dSRV(NULL)
{
	loadTextureDesc(desc);
	onDeviceReset();
}

D3D11RendererTexture3D::~D3D11RendererTexture3D(void)
{
	dxSafeRelease(m_d3dTexture);
	dxSafeRelease(m_d3dSamplerState);
	dxSafeRelease(m_d3dSRV);

	if (m_data)
	{
		for (PxU32 i = 0; i < getNumLevels(); i++)
		{
			delete [] m_data[i];
		}
		delete [] m_data;
	}
	if (m_resourceData)
	{
		delete [] m_resourceData;
	}
}

void* D3D11RendererTexture3D::lockLevel(PxU32 level, PxU32& pitch)
{
	void* buffer = 0;
	RENDERER_ASSERT(level < getNumLevels(), "Level out of range!");
	if (level < getNumLevels())
	{
		buffer = m_data[level];
		pitch  = getFormatNumBlocks(getWidth() >> level, getFormat()) * getBlockSize();
	}
	return buffer;
}

void D3D11RendererTexture3D::unlockLevel(PxU32 level)
{
	RENDERER_ASSERT(level < getNumLevels(), "Level out of range!");

	if (m_d3dTexture && level < getNumLevels())
	{
		PxU32 w = getLevelDimension(getWidth(),  level);
		PxU32 h = getLevelDimension(getHeight(), level);
		m_d3dDeviceContext.UpdateSubresource(m_d3dTexture,
		                                     level,
		                                     NULL,
		                                     m_data[level],
		                                     getFormatNumBlocks(w, getFormat()) * getBlockSize(),
		                                     computeImageByteSize(w, h, 1, getFormat()));
	}
}

void D3D11RendererTexture3D::bind(PxU32 samplerIndex, PxU32 flags)
{
	if (flags)
	{
		if (m_d3dSRV)
		{
			if (flags & BIND_VERTEX)
				m_d3dDeviceContext.VSSetShaderResources(samplerIndex, 1, &m_d3dSRV);
			if (flags & BIND_GEOMETRY)
				m_d3dDeviceContext.GSSetShaderResources(samplerIndex, 1, &m_d3dSRV);
			if (flags & BIND_PIXEL)
				m_d3dDeviceContext.PSSetShaderResources(samplerIndex, 1, &m_d3dSRV);
			if (flags & BIND_HULL)
				m_d3dDeviceContext.HSSetShaderResources(samplerIndex, 1, &m_d3dSRV);
			if (flags & BIND_DOMAIN)
				m_d3dDeviceContext.DSSetShaderResources(samplerIndex, 1, &m_d3dSRV);
		}
		if (m_d3dSamplerState)
		{
			if (flags & BIND_VERTEX)
				m_d3dDeviceContext.VSSetSamplers(samplerIndex, 1, &m_d3dSamplerState);
			if (flags & BIND_GEOMETRY)
				m_d3dDeviceContext.GSSetSamplers(samplerIndex, 1, &m_d3dSamplerState);
			if (flags & BIND_PIXEL)
				m_d3dDeviceContext.PSSetSamplers(samplerIndex, 1, &m_d3dSamplerState);
			if (flags & BIND_HULL)
				m_d3dDeviceContext.HSSetSamplers(samplerIndex, 1, &m_d3dSamplerState);
			if (flags & BIND_DOMAIN)
				m_d3dDeviceContext.DSSetSamplers(samplerIndex, 1, &m_d3dSamplerState);
		}
	}
	else
	{
		ID3D11ShaderResourceView* nullResources[] = { NULL };
		m_d3dDeviceContext.VSSetShaderResources(samplerIndex, 1, nullResources);
		m_d3dDeviceContext.GSSetShaderResources(samplerIndex, 1, nullResources);
		m_d3dDeviceContext.PSSetShaderResources(samplerIndex, 1, nullResources);
		m_d3dDeviceContext.HSSetShaderResources(samplerIndex, 1, nullResources);
		m_d3dDeviceContext.DSSetShaderResources(samplerIndex, 1, nullResources);
		ID3D11SamplerState* nullSamplers[] = { NULL };
		m_d3dDeviceContext.VSSetSamplers(samplerIndex, 1, nullSamplers);
		m_d3dDeviceContext.GSSetSamplers(samplerIndex, 1, nullSamplers);
		m_d3dDeviceContext.PSSetSamplers(samplerIndex, 1, nullSamplers);
		m_d3dDeviceContext.HSSetSamplers(samplerIndex, 1, nullSamplers);
		m_d3dDeviceContext.DSSetSamplers(samplerIndex, 1, nullSamplers);
	}
}

void D3D11RendererTexture3D::onDeviceLost(void)
{
	dxSafeRelease(m_d3dTexture);
	dxSafeRelease(m_d3dSamplerState);
	dxSafeRelease(m_d3dSRV);
}

void D3D11RendererTexture3D::onDeviceReset(void)
{
	HRESULT result = S_OK;
	if (!m_d3dTexture)
	{
		D3D11_SUBRESOURCE_DATA* pData = isDepthStencilFormat(getFormat()) ? NULL : m_resourceData;
		result = m_d3dDevice.CreateTexture3D(&m_d3dTextureDesc, pData, &m_d3dTexture);
		RENDERER_ASSERT(SUCCEEDED(result), "Unable to create D3D11 Texture.");
	}
	if (SUCCEEDED(result) && !m_d3dSamplerState)
	{
		result = m_d3dDevice.CreateSamplerState(&m_d3dSamplerDesc, &m_d3dSamplerState);
		RENDERER_ASSERT(SUCCEEDED(result), "Unable to create D3D11 Sampler.");
	}
	if (SUCCEEDED(result) && m_d3dTextureDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE && !m_d3dSRV)
	{
		result = m_d3dDevice.CreateShaderResourceView(m_d3dTexture, &m_d3dSRVDesc, &m_d3dSRV);
		RENDERER_ASSERT(SUCCEEDED(result), "Unable to create D3D11 Shader Resource View.");
	}
	RENDERER_ASSERT((m_d3dTextureDesc.BindFlags & D3D11_BIND_RENDER_TARGET) == 0, "D3D11 3D Texture cannot be bound as render target.");
	RENDERER_ASSERT((m_d3dTextureDesc.BindFlags & D3D11_BIND_DEPTH_STENCIL) == 0, "D3D11 3D Texture cannot be bound as depth stencil.");
}


void D3D11RendererTexture3D::loadTextureDesc(const RendererTexture3DDesc& desc)
{
	//memset(&m_d3dTextureDesc, 0, sizeof(m_d3dTextureDesc));
	m_d3dTextureDesc = D3D11_TEXTURE3D_DESC();
	m_d3dTextureDesc.Width              = getWidth();
	m_d3dTextureDesc.Height             = getHeight();
	m_d3dTextureDesc.Depth              = getDepth();
	m_d3dTextureDesc.MipLevels          = getNumLevels();
	m_d3dTextureDesc.Format             = getD3D11TextureFormat(desc.format);
	m_d3dTextureDesc.CPUAccessFlags     = 0;
	m_d3dTextureDesc.Usage              = D3D11_USAGE_DEFAULT;
	m_d3dTextureDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;

	loadResourceDesc(desc);

	loadSamplerDesc(desc);

	{
		m_data = new PxU8*[getNumLevels()];
		m_resourceData = new D3D11_SUBRESOURCE_DATA[getNumLevels()];
		memset(m_data, 0, sizeof(PxU8)*getNumLevels());
		memset(m_resourceData, 0, sizeof(D3D11_SUBRESOURCE_DATA)*getNumLevels());

		for (PxU32 i = 0; i < desc.numLevels; i++)
		{
			PxU32 w = getLevelDimension(getWidth(),  i);
			PxU32 h = getLevelDimension(getHeight(), i);
			PxU32 d = getLevelDimension(getDepth(), i);
			PxU32 levelSize = computeImageByteSize(w, h, d, desc.format);
			m_data[i]       = new PxU8[levelSize];
			memset(m_data[i], 0, levelSize);
			m_resourceData[i].pSysMem = m_data[i];
			m_resourceData[i].SysMemPitch = levelSize / d / h;
			m_resourceData[i].SysMemSlicePitch = levelSize / d;
		}
	}
}

void D3D11RendererTexture3D::loadSamplerDesc(const RendererTexture3DDesc& desc)
{
	m_d3dSamplerDesc.Filter         = getD3D11TextureFilter(desc.filter);
	m_d3dSamplerDesc.AddressU       = getD3D11TextureAddressing(desc.addressingU);
	m_d3dSamplerDesc.AddressV       = getD3D11TextureAddressing(desc.addressingV);
	m_d3dSamplerDesc.AddressW       = getD3D11TextureAddressing(desc.addressingW);
	m_d3dSamplerDesc.MipLODBias     = 0.f;
	m_d3dSamplerDesc.MaxAnisotropy  = 1;
	m_d3dSamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	m_d3dSamplerDesc.BorderColor[0] = m_d3dSamplerDesc.BorderColor[1] = m_d3dSamplerDesc.BorderColor[2] = m_d3dSamplerDesc.BorderColor[3] = 0.;
	m_d3dSamplerDesc.MinLOD         = 0;
	if (desc.numLevels <= 1)
	{
		m_d3dSamplerDesc.MaxLOD = 0.;
	}
	else
	{
		m_d3dSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	}
}

void D3D11RendererTexture3D::loadResourceDesc(const RendererTexture3DDesc& desc)
{
	m_d3dSRVDesc.Format = (m_d3dTextureDesc.Format == DXGI_FORMAT_R16_TYPELESS) ? DXGI_FORMAT_R16_UNORM : m_d3dTextureDesc.Format;
	m_d3dSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	m_d3dSRVDesc.Texture3D.MipLevels = m_d3dTextureDesc.MipLevels;
	m_d3dSRVDesc.Texture3D.MostDetailedMip = 0;
}

#endif // #if defined(RENDERER_ENABLE_DIRECT3D11)
