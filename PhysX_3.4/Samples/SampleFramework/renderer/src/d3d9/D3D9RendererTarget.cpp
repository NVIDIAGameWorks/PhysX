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

// suppress LNK4221 on Xbox
namespace {char dummySymbol; }

#include <RendererConfig.h>
#include "D3D9RendererTarget.h"

#if defined(RENDERER_ENABLE_DIRECT3D9) && defined(RENDERER_ENABLE_DIRECT3D9_TARGET)

#include <RendererTargetDesc.h>
#include "D3D9RendererTexture2D.h"

using namespace SampleRenderer;

D3D9RendererTarget::D3D9RendererTarget(IDirect3DDevice9 &d3dDevice, const RendererTargetDesc &desc) :
	m_d3dDevice(d3dDevice)
{
	m_d3dLastSurface = 0;
	m_d3dLastDepthStencilSurface = 0;
	m_d3dDepthStencilSurface = 0;
	for(PxU32 i=0; i<desc.numTextures; i++)
	{
		D3D9RendererTexture2D &texture = *static_cast<D3D9RendererTexture2D*>(desc.textures[i]);
		m_textures.push_back(&texture);
	}
	m_depthStencilSurface = static_cast<D3D9RendererTexture2D*>(desc.depthStencilSurface);
	RENDERER_ASSERT(m_depthStencilSurface && m_depthStencilSurface->m_d3dTexture, "Invalid Target Depth Stencil Surface!");
	onDeviceReset();
}

D3D9RendererTarget::~D3D9RendererTarget(void)
{
	if(m_d3dDepthStencilSurface) m_d3dDepthStencilSurface->Release();
}

void D3D9RendererTarget::bind(void)
{
	RENDERER_ASSERT(m_d3dLastSurface==0 && m_d3dLastDepthStencilSurface==0, "Render Target in bad state!");
	if(m_d3dDepthStencilSurface && !m_d3dLastSurface && !m_d3dLastDepthStencilSurface)
	{
		m_d3dDevice.GetRenderTarget(0, &m_d3dLastSurface);
		m_d3dDevice.GetDepthStencilSurface(&m_d3dLastDepthStencilSurface);
		const PxU32 numTextures = (PxU32)m_textures.size();
		for(PxU32 i=0; i<numTextures; i++)
		{
			IDirect3DSurface9 *d3dSurcace = 0;
			D3D9RendererTexture2D &texture = *m_textures[i];
			/* HRESULT result = */ texture.m_d3dTexture->GetSurfaceLevel(0, &d3dSurcace);
			RENDERER_ASSERT(d3dSurcace, "Cannot get Texture Surface!");
			if(d3dSurcace)
			{
				m_d3dDevice.SetRenderTarget(i, d3dSurcace);
				d3dSurcace->Release();
			}
		}
		m_d3dDevice.SetDepthStencilSurface(m_d3dDepthStencilSurface);
		const DWORD flags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
		m_d3dDevice.Clear(0, 0, flags, 0x00000000, 1.0f, 0);
	}
	float depthBias = 0.0001f;
	float biasSlope = 1.58f;
#if RENDERER_ENABLE_DRESSCODE
	depthBias = dcParam("depthBias", depthBias, 0.0f, 0.01f);
	biasSlope = dcParam("biasSlope", biasSlope, 0.0f, 5.0f);
#endif
	m_d3dDevice.SetRenderState(D3DRS_DEPTHBIAS,           *(DWORD*)&depthBias);
	m_d3dDevice.SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&biasSlope);
}

void D3D9RendererTarget::unbind(void)
{
	RENDERER_ASSERT(m_d3dLastSurface && m_d3dLastDepthStencilSurface, "Render Target in bad state!");
	if(m_d3dDepthStencilSurface && m_d3dLastSurface && m_d3dLastDepthStencilSurface)
	{
		m_d3dDevice.SetDepthStencilSurface(m_d3dLastDepthStencilSurface);
		m_d3dDevice.SetRenderTarget(0, m_d3dLastSurface);
		const PxU32 numTextures = (PxU32)m_textures.size();
		for(PxU32 i=1; i<numTextures; i++)
		{
			m_d3dDevice.SetRenderTarget(i, 0);
		}
		m_d3dLastSurface->Release();
		m_d3dLastSurface = 0;
		m_d3dLastDepthStencilSurface->Release();
		m_d3dLastDepthStencilSurface = 0;
	}
	float depthBias = 0;
	float biasSlope = 0;
	m_d3dDevice.SetRenderState(D3DRS_DEPTHBIAS,           *(DWORD*)&depthBias);
	m_d3dDevice.SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&biasSlope);
}

void D3D9RendererTarget::onDeviceLost(void)
{
	RENDERER_ASSERT(m_d3dLastDepthStencilSurface==0, "Render Target in bad state!");
	RENDERER_ASSERT(m_d3dDepthStencilSurface,        "Render Target in bad state!");
	if(m_d3dDepthStencilSurface)
	{
		m_d3dDepthStencilSurface->Release();
		m_d3dDepthStencilSurface = 0;
	}
}

void D3D9RendererTarget::onDeviceReset(void)
{
	RENDERER_ASSERT(m_d3dDepthStencilSurface==0, "Render Target in bad state!");
	if(!m_d3dDepthStencilSurface && m_depthStencilSurface && m_depthStencilSurface->m_d3dTexture)
	{
		bool ok = m_depthStencilSurface->m_d3dTexture->GetSurfaceLevel(0, &m_d3dDepthStencilSurface) == D3D_OK;
		if(!ok)
		{
			RENDERER_ASSERT(ok, "Failed to create Render Target Depth Stencil Surface.");
		}
	}
}

#endif //#if defined(RENDERER_ENABLE_DIRECT3D9) && defined(RENDERER_ENABLE_DIRECT3D9_TARGET)
