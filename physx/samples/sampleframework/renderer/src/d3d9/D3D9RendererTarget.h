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
