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
#include "D3D11RendererTarget.h"

#if defined(RENDERER_ENABLE_DIRECT3D11) && defined(RENDERER_ENABLE_DIRECT3D11_TARGET)

#include <RendererTargetDesc.h>
#include "D3D11RendererTexture2D.h"
#include "D3D11RendererMemoryMacros.h"

using namespace SampleRenderer;

D3D11RendererTarget::D3D11RendererTarget(ID3D11Device& d3dDevice, ID3D11DeviceContext& d3dDeviceContext, const RendererTargetDesc& desc) :
	m_d3dDevice(d3dDevice),
	m_d3dDeviceContext(d3dDeviceContext),
	m_depthStencilSurface(NULL),
	m_d3dDSV(NULL),
	m_d3dLastDSV(NULL),
	m_d3dRS(NULL),
	m_d3dLastRS(NULL)
{
	for (PxU32 i = 0; i < desc.numTextures; i++)
	{
		D3D11RendererTexture2D& texture = *static_cast<D3D11RendererTexture2D*>(desc.textures[i]);
		m_textures.push_back(&texture);
		RENDERER_ASSERT(texture.m_d3dRTV, "Invalid render target specification");
		if (texture.m_d3dRTV)
		{
			m_d3dRTVs.push_back(texture.m_d3dRTV);
		}
	}
	m_depthStencilSurface = static_cast<D3D11RendererTexture2D*>(desc.depthStencilSurface);
	RENDERER_ASSERT(m_depthStencilSurface && m_depthStencilSurface->m_d3dTexture, "Invalid Target Depth Stencil Surface!");
	m_d3dDSV = m_depthStencilSurface->m_d3dDSV;
	onDeviceReset();
}

D3D11RendererTarget::~D3D11RendererTarget(void)
{
	dxSafeRelease(m_d3dRS);
}

void D3D11RendererTarget::bind(void)
{
	RENDERER_ASSERT(m_d3dRS && m_d3dLastDSV == NULL && m_d3dLastRTVs.size() == 0, "Render target in a bad state");
	if (m_d3dRS && !m_d3dLastDSV && m_d3dLastRTVs.size() == 0)
	{
		m_d3dLastRTVs.resize(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, NULL);
		m_d3dLastDSV = NULL;

		m_d3dDeviceContext.OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,
		                                      &m_d3dLastRTVs[0],
		                                      &m_d3dLastDSV);
		m_d3dDeviceContext.RSGetState(&m_d3dLastRS);

		static const PxF32 black[4] = {0.f, 0.f, 0.f, 0.f};
		for (PxU32 i = 0; i < m_d3dRTVs.size(); ++i)
		{
			m_d3dDeviceContext.ClearRenderTargetView(m_d3dRTVs[i], black);
		}
		m_d3dDeviceContext.ClearDepthStencilView(m_d3dDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1., 0);

		m_d3dDeviceContext.OMSetRenderTargets((UINT)m_d3dRTVs.size(), &m_d3dRTVs[0], m_d3dDSV);
		m_d3dDeviceContext.RSSetState(m_d3dRS);
	}
}

void D3D11RendererTarget::unbind(void)
{
	RENDERER_ASSERT(m_d3dLastDSV && m_d3dLastRTVs.size() > 0, "Render Target in a bad state.");
	if (m_d3dLastDSV && m_d3dLastRTVs.size() > 0)
	{
		m_d3dDeviceContext.OMSetRenderTargets((UINT)m_d3dLastRTVs.size(), &m_d3dLastRTVs[0], m_d3dLastDSV);
		for (PxU32 i = 0; i < m_d3dLastRTVs.size(); ++i)
		{
			dxSafeRelease(m_d3dLastRTVs[i]);
		}
		m_d3dLastRTVs.clear();
		dxSafeRelease(m_d3dLastDSV);
	}
	if (m_d3dLastRS)
	{
		m_d3dDeviceContext.RSSetState(m_d3dLastRS);
		dxSafeRelease(m_d3dLastRS);
	}
}

void D3D11RendererTarget::onDeviceLost(void)
{
	RENDERER_ASSERT(m_d3dLastRS == NULL,  "Render Target in bad state!");
	RENDERER_ASSERT(m_d3dRS,              "Render Target in bad state!");
	dxSafeRelease(m_d3dRS);
}

void D3D11RendererTarget::onDeviceReset(void)
{
	RENDERER_ASSERT(m_d3dRS == NULL, "Render Target in a bad state!");
	if (!m_d3dRS)
	{
		D3D11_RASTERIZER_DESC rasterizerDesc =
		{
			D3D11_FILL_SOLID, // D3D11_FILL_MODE FillMode;
			D3D11_CULL_NONE,  // D3D11_CULL_MODE CullMode;
			FALSE,            // BOOL FrontCounterClockwise;
			0,                // INT DepthBias;
			0,                // FLOAT DepthBiasClamp;
			1.0,              // FLOAT SlopeScaledDepthBias;
			TRUE,             // BOOL DepthClipEnable;
			FALSE,            // BOOL ScissorEnable;
			TRUE,             // BOOL MultisampleEnable;
			FALSE,            // BOOL AntialiasedLineEnable;
		};

		//float depthBias = 0.0001f;
		//float biasSlope = 1.58f;
#if RENDERER_ENABLE_DRESSCODE
		//depthBias = dcParam("depthBias", depthBias, 0.0f, 0.01f);
		//biasSlope = dcParam("biasSlope", biasSlope, 0.0f, 5.0f);
#endif

		m_d3dDevice.CreateRasterizerState(&rasterizerDesc, &m_d3dRS);
	}
}

#endif //#if defined(RENDERER_ENABLE_DIRECT3D11) && defined(RENDERER_ENABLE_DIRECT3D11_TARGET)
