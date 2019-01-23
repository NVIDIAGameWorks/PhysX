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
