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
#ifndef D3D11_RENDERER_INSTANCEBUFFER_H
#define D3D11_RENDERER_INSTANCEBUFFER_H

#include <RendererConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D11)

#include <RendererInstanceBuffer.h>
#include "D3D11Renderer.h"

namespace SampleRenderer
{

class D3D11RendererInstanceBuffer : public RendererInstanceBuffer, public D3D11RendererResource
{
public:
	D3D11RendererInstanceBuffer(ID3D11Device& d3dDevice, ID3D11DeviceContext& d3dDeviceContext, const RendererInstanceBufferDesc& desc, bool bUseMapForLocking = FALSE);
	virtual ~D3D11RendererInstanceBuffer(void);

	void addVertexElements(PxU32 streamIndex, std::vector<D3D11_INPUT_ELEMENT_DESC> &vertexElements) const;

protected:
	virtual void* lock(void);
	virtual void  unlock(void);

	virtual void  bind(PxU32 streamID, PxU32 firstInstance) const;
	virtual void  unbind(PxU32 streamID) const;

private:
	virtual void onDeviceLost(void);
	virtual void onDeviceReset(void);

	void *internalLock(D3D11_MAP MapType);

private:
	ID3D11Device&           m_d3dDevice;
	ID3D11DeviceContext&    m_d3dDeviceContext;
	ID3D11Buffer*           m_d3dInstanceBuffer;
	D3D11_BUFFER_DESC       m_d3dBufferDesc;

	bool                    m_bUseMapForLocking;
	PxU8*                   m_buffer;
};

} // namespace SampleRenderer

#endif // #if defined(RENDERER_ENABLE_DIRECT3D11)
#endif
