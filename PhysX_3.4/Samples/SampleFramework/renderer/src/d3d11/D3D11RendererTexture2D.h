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
#ifndef D3D11_RENDERER_TEXTURE_2D_H
#define D3D11_RENDERER_TEXTURE_2D_H

#include <RendererConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D11)

#include <RendererTexture2D.h>

#include "D3D11Renderer.h"
#include "D3D11RendererTextureCommon.h"

namespace SampleRenderer
{

class D3D11RendererTexture2D : public RendererTexture2D, public D3D11RendererResource
{
	friend class D3D11RendererTarget;
	friend class D3D11RendererSpotLight;
public:
	D3D11RendererTexture2D(ID3D11Device& d3dDevice, ID3D11DeviceContext& d3dDeviceContext, const RendererTexture2DDesc& desc);
	virtual ~D3D11RendererTexture2D(void);

public:
	virtual void* lockLevel(PxU32 level, PxU32& pitch);
	virtual void  unlockLevel(PxU32 level);

	void bind(PxU32 samplerIndex, PxU32 flags = BIND_PIXEL);

	virtual void select(PxU32 stageIndex)
	{
		bind(stageIndex);
	}

private:
	virtual void onDeviceLost(void);
	virtual void onDeviceReset(void);

	void loadTextureDesc(const RendererTexture2DDesc&);
	void loadSamplerDesc(const RendererTexture2DDesc&);
	void loadResourceDesc(const RendererTexture2DDesc&);
	void loadTargetDesc(const RendererTexture2DDesc&);
	void loadDepthStencilDesc(const RendererTexture2DDesc&);

private:
	ID3D11Device&               m_d3dDevice;
	ID3D11DeviceContext&        m_d3dDeviceContext;
	ID3D11Texture2D*            m_d3dTexture;
	D3D11_TEXTURE2D_DESC        m_d3dTextureDesc;

	ID3D11SamplerState*         m_d3dSamplerState;
	D3D11_SAMPLER_DESC          m_d3dSamplerDesc;

	ID3D11ShaderResourceView*       m_d3dSRV;
	D3D11_SHADER_RESOURCE_VIEW_DESC m_d3dSRVDesc;

	ID3D11RenderTargetView*         m_d3dRTV;
	D3D11_RENDER_TARGET_VIEW_DESC   m_d3dRTVDesc;

	ID3D11DepthStencilView*         m_d3dDSV;
	D3D11_DEPTH_STENCIL_VIEW_DESC   m_d3dDSVDesc;

	PxU8**                      m_data;
	D3D11_SUBRESOURCE_DATA*     m_resourceData;
};

} // namespace SampleRenderer

#endif // #if defined(RENDERER_ENABLE_DIRECT3D11)
#endif
