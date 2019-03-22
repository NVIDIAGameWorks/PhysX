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
#ifndef D3D11_RENDERER_TARGET_H
#define D3D11_RENDERER_TARGET_H

#include <RendererConfig.h>

#if defined(RENDERER_WINDOWS)
#define RENDERER_ENABLE_DIRECT3D11_TARGET
#endif

#if defined(RENDERER_ENABLE_DIRECT3D11) && defined(RENDERER_ENABLE_DIRECT3D11_TARGET)

#include <RendererTarget.h>
#include "D3D11Renderer.h"

namespace SampleRenderer
{
class D3D11RendererTexture2D;

class D3D11RendererTarget : public RendererTarget, public D3D11RendererResource
{
public:
	D3D11RendererTarget(ID3D11Device& d3dDevice, ID3D11DeviceContext& d3dDeviceContext, const RendererTargetDesc& desc);
	virtual ~D3D11RendererTarget(void);

private:
	D3D11RendererTarget& operator=(const D3D11RendererTarget&) {}
	virtual void bind(void);
	virtual void unbind(void);

private:
	virtual void onDeviceLost(void);
	virtual void onDeviceReset(void);

private:
	ID3D11Device&                        m_d3dDevice;
	ID3D11DeviceContext&                 m_d3dDeviceContext;

	std::vector<D3D11RendererTexture2D*> m_textures;
	D3D11RendererTexture2D*              m_depthStencilSurface;

	std::vector<ID3D11RenderTargetView*> m_d3dRTVs;
	std::vector<ID3D11RenderTargetView*> m_d3dLastRTVs;

	ID3D11DepthStencilView*              m_d3dDSV;
	ID3D11DepthStencilView*              m_d3dLastDSV;

	ID3D11RasterizerState*               m_d3dRS;
	ID3D11RasterizerState*               m_d3dLastRS;
};

} // namespace SampleRenderer

#endif // #if defined(RENDERER_ENABLE_DIRECT3D11) && defined(RENDERER_ENABLE_DIRECT3D11_TARGET)
#endif
