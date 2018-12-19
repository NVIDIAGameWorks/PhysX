// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.

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
