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

#include "D3D11RendererTexture2D.h"
#include "D3D11RendererMemoryMacros.h"

#include <RendererTexture2DDesc.h>

using namespace SampleRenderer;

D3D11RendererTexture2D::D3D11RendererTexture2D(ID3D11Device& d3dDevice, ID3D11DeviceContext& d3dDeviceContext, const RendererTexture2DDesc& desc) :
	RendererTexture2D(desc),
	m_d3dDevice(d3dDevice),
	m_d3dDeviceContext(d3dDeviceContext),
	m_d3dTexture(NULL),
	m_d3dSamplerState(NULL),
	m_d3dSRV(NULL),
	m_d3dRTV(NULL),
	m_d3dDSV(NULL)
{
	loadTextureDesc(desc);
	onDeviceReset();
}

D3D11RendererTexture2D::~D3D11RendererTexture2D(void)
{
	dxSafeRelease(m_d3dTexture);
	dxSafeRelease(m_d3dSamplerState);
	dxSafeRelease(m_d3dSRV);
	dxSafeRelease(m_d3dRTV);
	dxSafeRelease(m_d3dDSV);

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

void* D3D11RendererTexture2D::lockLevel(PxU32 level, PxU32& pitch)
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

void D3D11RendererTexture2D::unlockLevel(PxU32 level)
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

void D3D11RendererTexture2D::bind(PxU32 samplerIndex, PxU32 flags)
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

void D3D11RendererTexture2D::onDeviceLost(void)
{
	dxSafeRelease(m_d3dTexture);
	dxSafeRelease(m_d3dSamplerState);
	dxSafeRelease(m_d3dSRV);
	dxSafeRelease(m_d3dRTV);
	dxSafeRelease(m_d3dDSV);
}

void D3D11RendererTexture2D::onDeviceReset(void)
{
	HRESULT result = S_OK;
	if (!m_d3dTexture)
	{
		D3D11_SUBRESOURCE_DATA* pData = isDepthStencilFormat(getFormat()) ? NULL : m_resourceData;
		result = m_d3dDevice.CreateTexture2D(&m_d3dTextureDesc, pData, &m_d3dTexture);
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
	if (SUCCEEDED(result) && m_d3dTextureDesc.BindFlags & D3D11_BIND_RENDER_TARGET && !m_d3dRTV)
	{
		result = m_d3dDevice.CreateRenderTargetView(m_d3dTexture, &m_d3dRTVDesc, &m_d3dRTV);
		RENDERER_ASSERT(SUCCEEDED(result), "Unable to create D3D11 Render Target View.");
	}
	if (SUCCEEDED(result) && m_d3dTextureDesc.BindFlags & D3D11_BIND_DEPTH_STENCIL && !m_d3dDSV)
	{
		result = m_d3dDevice.CreateDepthStencilView(m_d3dTexture, &m_d3dDSVDesc, &m_d3dDSV);
		RENDERER_ASSERT(SUCCEEDED(result), "Unable to create D3D11 Depth Stencil View.");
	}
}


void D3D11RendererTexture2D::loadTextureDesc(const RendererTexture2DDesc& desc)
{
	RENDERER_ASSERT(desc.depth == 1, "Invalid depth for 2D Texture!");

	//memset(&m_d3dTextureDesc, 0, sizeof(m_d3dTextureDesc));
	m_d3dTextureDesc = D3D11_TEXTURE2D_DESC();
	m_d3dTextureDesc.Width = getWidth();
	m_d3dTextureDesc.Height = getHeight();
	m_d3dTextureDesc.MipLevels = getNumLevels();
	m_d3dTextureDesc.ArraySize = 1;
	m_d3dTextureDesc.Format = getD3D11TextureFormat(desc.format);
	m_d3dTextureDesc.SampleDesc.Count = 1;
	m_d3dTextureDesc.SampleDesc.Quality = 0;
	m_d3dTextureDesc.CPUAccessFlags = 0;
	m_d3dTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	m_d3dTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	loadResourceDesc(desc);

	if (isDepthStencilFormat(desc.format))
	{
		m_d3dTextureDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
		m_d3dTextureDesc.CPUAccessFlags = 0;
		m_d3dTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		loadDepthStencilDesc(desc);
	}
	else if (desc.renderTarget)
	{
		m_d3dTextureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		m_d3dTextureDesc.CPUAccessFlags = 0;
		m_d3dTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		loadTargetDesc(desc);
	} 

	loadSamplerDesc(desc);

	//if (m_d3dTextureDesc.CPUAccessFlags)
	{
		m_data = new PxU8*[getNumLevels()];
		m_resourceData = new D3D11_SUBRESOURCE_DATA[getNumLevels()];
		memset(m_data, 0, sizeof(PxU8)*getNumLevels());
		memset(m_resourceData, 0, sizeof(D3D11_SUBRESOURCE_DATA)*getNumLevels());

		for (PxU32 i = 0; i < desc.numLevels; i++)
		{
			PxU32 w = getLevelDimension(getWidth(),  i);
			PxU32 h = getLevelDimension(getHeight(), i);
			PxU32 levelSize = computeImageByteSize(w, h, 1, desc.format);
			m_data[i]       = new PxU8[levelSize];
			memset(m_data[i], 0, levelSize);
			m_resourceData[i].pSysMem = m_data[i];
			m_resourceData[i].SysMemPitch = levelSize / h;
			m_resourceData[i].SysMemSlicePitch = 0;
		}
	}
}

void D3D11RendererTexture2D::loadSamplerDesc(const RendererTexture2DDesc& desc)
{
	m_d3dSamplerDesc.Filter   = getD3D11TextureFilter(desc.filter);
	m_d3dSamplerDesc.AddressU = getD3D11TextureAddressing(desc.addressingU);
	m_d3dSamplerDesc.AddressV = getD3D11TextureAddressing(desc.addressingV);
	m_d3dSamplerDesc.AddressW = getD3D11TextureAddressing(desc.addressingW);
	m_d3dSamplerDesc.MipLODBias = 0.f;
	m_d3dSamplerDesc.MaxAnisotropy = 1;
	m_d3dSamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	m_d3dSamplerDesc.BorderColor[0] = m_d3dSamplerDesc.BorderColor[1] = m_d3dSamplerDesc.BorderColor[2] = m_d3dSamplerDesc.BorderColor[3] = 0.;
	m_d3dSamplerDesc.MinLOD = 0;
	if (desc.numLevels <= 1)
	{
		m_d3dSamplerDesc.MaxLOD = 0.;
	}
	else
	{
		m_d3dSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	}
	if(m_d3dDevice.GetFeatureLevel() <= D3D_FEATURE_LEVEL_9_3)
	{
		m_d3dSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	}
}

void D3D11RendererTexture2D::loadResourceDesc(const RendererTexture2DDesc& desc)
{
	m_d3dSRVDesc.Format = (m_d3dTextureDesc.Format == DXGI_FORMAT_R16_TYPELESS) ? DXGI_FORMAT_R16_UNORM : m_d3dTextureDesc.Format;
	m_d3dSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	m_d3dSRVDesc.Texture2D.MipLevels = m_d3dTextureDesc.MipLevels;
	m_d3dSRVDesc.Texture2D.MostDetailedMip = 0;
	//m_d3dSRVDesc.Texture2D.MostDetailedMip = m_d3dTextureDesc.MipLevels-1;
}

void D3D11RendererTexture2D::loadTargetDesc(const RendererTexture2DDesc& desc)
{
	m_d3dRTVDesc = D3D11_RENDER_TARGET_VIEW_DESC();
	m_d3dRTVDesc.Format = (m_d3dTextureDesc.Format == DXGI_FORMAT_R16_TYPELESS) ? DXGI_FORMAT_R16_UNORM : m_d3dTextureDesc.Format;
	m_d3dRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	m_d3dRTVDesc.Texture2D.MipSlice = 0;
}

void D3D11RendererTexture2D::loadDepthStencilDesc(const RendererTexture2DDesc& desc)
{
	m_d3dDSVDesc = D3D11_DEPTH_STENCIL_VIEW_DESC();
	m_d3dDSVDesc.Format = (m_d3dTextureDesc.Format == DXGI_FORMAT_R16_TYPELESS) ? DXGI_FORMAT_D16_UNORM : m_d3dTextureDesc.Format;
	m_d3dDSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	m_d3dDSVDesc.Texture2D.MipSlice = 0;
}

#endif // #if defined(RENDERER_ENABLE_DIRECT3D11)
