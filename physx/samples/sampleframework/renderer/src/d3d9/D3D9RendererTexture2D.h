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
#ifndef D3D9_RENDERER_TEXTURE_2D_H
#define D3D9_RENDERER_TEXTURE_2D_H

#include <RendererConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <RendererTexture2D.h>
#include "D3D9Renderer.h"

namespace SampleRenderer
{
	class D3D9Renderer;
	class D3D9RendererTexture2D : public RendererTexture2D, public D3D9RendererResource
	{
		friend class D3D9RendererTarget;
		friend class D3D9RendererSpotLight;
	public:
		D3D9RendererTexture2D(IDirect3DDevice9 &d3dDevice, D3D9Renderer& renderer, const RendererTexture2DDesc &desc);
		virtual ~D3D9RendererTexture2D(void);

	public:
		virtual void *lockLevel(PxU32 level, PxU32 &pitch);
		virtual void  unlockLevel(PxU32 level);

		void bind(PxU32 samplerIndex);

		virtual	void	select(PxU32 stageIndex)
		{
			bind(stageIndex);
		}

	private:

		virtual void onDeviceLost(void);
		virtual void onDeviceReset(void);

	private:
		IDirect3DDevice9          &m_d3dDevice;
		IDirect3DTexture9         *m_d3dTexture;

		DWORD                      m_usage;
		D3DPOOL                    m_pool;
		D3DFORMAT                  m_format;

		D3DTEXTUREFILTERTYPE       m_d3dMinFilter;
		D3DTEXTUREFILTERTYPE       m_d3dMagFilter;
		D3DTEXTUREFILTERTYPE       m_d3dMipFilter;
		D3DTEXTUREADDRESS          m_d3dAddressingU;
		D3DTEXTUREADDRESS          m_d3dAddressingV;
	};

} // namespace SampleRenderer

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
#endif
