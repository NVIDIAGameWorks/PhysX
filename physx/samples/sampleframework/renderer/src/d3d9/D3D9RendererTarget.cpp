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
