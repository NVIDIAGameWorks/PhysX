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
#ifndef D3D9_RENDERER_TARGET_H
#define D3D9_RENDERER_TARGET_H

#include <RendererConfig.h>

// TODO: 360 can't render directly into texture memory, so we need to create a pool of
//       surfaces to share internally and then "resolve" them into the textures inside of
//       the 'unbind' call.
#if defined(RENDERER_WINDOWS)
	#define RENDERER_ENABLE_DIRECT3D9_TARGET
#endif

#if defined(RENDERER_ENABLE_DIRECT3D9) && defined(RENDERER_ENABLE_DIRECT3D9_TARGET)

#include <RendererTarget.h>
#include "D3D9Renderer.h"

namespace SampleRenderer
{

	class D3D9RendererTexture2D;

	class D3D9RendererTarget : public RendererTarget, public D3D9RendererResource
	{
	public:
		D3D9RendererTarget(IDirect3DDevice9 &d3dDevice, const RendererTargetDesc &desc);
		virtual ~D3D9RendererTarget(void);

	private:
		D3D9RendererTarget& operator=( const D3D9RendererTarget& ) {}
		virtual void bind(void);
		virtual void unbind(void);

	private:
		virtual void onDeviceLost(void);
		virtual void onDeviceReset(void);

	private:
		IDirect3DDevice9               &m_d3dDevice;
		IDirect3DSurface9              *m_d3dLastSurface;
		IDirect3DSurface9              *m_d3dLastDepthStencilSurface;
		IDirect3DSurface9              *m_d3dDepthStencilSurface;
		std::vector<D3D9RendererTexture2D*> m_textures;
		D3D9RendererTexture2D          *m_depthStencilSurface;
	};

} // namespace SampleRenderer

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9) && defined(RENDERER_ENABLE_DIRECT3D9_TARGET)
#endif
