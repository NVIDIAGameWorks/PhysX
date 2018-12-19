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

#include <RendererConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include "D3D9RendererIndexBuffer.h"
#include <RendererIndexBufferDesc.h>

#if PX_WINDOWS
#include <task/PxTask.h>
#endif

using namespace SampleRenderer;

static D3DFORMAT getD3D9Format(RendererIndexBuffer::Format format)
{
	D3DFORMAT d3dFormat = D3DFMT_UNKNOWN;
	switch(format)
	{
	case RendererIndexBuffer::FORMAT_UINT16: d3dFormat = D3DFMT_INDEX16; break;
	case RendererIndexBuffer::FORMAT_UINT32: d3dFormat = D3DFMT_INDEX32; break;
	}
	RENDERER_ASSERT(d3dFormat != D3DFMT_UNKNOWN, "Unable to convert to D3DFORMAT.");
	return d3dFormat;
}

D3D9RendererIndexBuffer::D3D9RendererIndexBuffer(IDirect3DDevice9 &d3dDevice, D3D9Renderer& renderer, const RendererIndexBufferDesc &desc) :
	RendererIndexBuffer(desc),
	m_d3dDevice(d3dDevice)
{
	m_d3dIndexBuffer = 0;

	m_usage      = D3DUSAGE_WRITEONLY;
	m_pool       = renderer.canUseManagedResources() ? D3DPOOL_MANAGED : D3DPOOL_DEFAULT;
	PxU32	indexSize  = getFormatByteSize(desc.format);
	m_format     = getD3D9Format(desc.format);
	m_bufferSize = indexSize * desc.maxIndices;

#if RENDERER_ENABLE_DYNAMIC_VB_POOLS
	if(desc.hint == RendererIndexBuffer::HINT_DYNAMIC )
	{
		m_usage = desc.registerInCUDA ? 0 : D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
		m_pool  = D3DPOOL_DEFAULT;
	}
#endif

	onDeviceReset();

	if(m_d3dIndexBuffer)
	{
		m_maxIndices = desc.maxIndices;
	}
}

D3D9RendererIndexBuffer::~D3D9RendererIndexBuffer(void)
{
	if(m_d3dIndexBuffer)
	{
#if PX_WINDOWS && PX_SUPPORT_GPU_PHYSX
		if(m_interopContext && m_registeredInCUDA)
		{
			m_registeredInCUDA = !m_interopContext->unregisterResourceInCuda(m_InteropHandle);
		}
#endif
		m_d3dIndexBuffer->Release();
	}
}

void D3D9RendererIndexBuffer::onDeviceLost(void)
{
	m_registeredInCUDA = false;

	if(m_pool != D3DPOOL_MANAGED && m_d3dIndexBuffer)
	{
#if PX_WINDOWS && PX_SUPPORT_GPU_PHYSX
		if(m_interopContext && m_registeredInCUDA)
		{
			m_registeredInCUDA = !m_interopContext->unregisterResourceInCuda(m_InteropHandle);
		}
#endif
		m_d3dIndexBuffer->Release();
		m_d3dIndexBuffer = 0;
	}
}

void D3D9RendererIndexBuffer::onDeviceReset(void)
{
	if(!m_d3dIndexBuffer)
	{
		m_d3dDevice.CreateIndexBuffer(m_bufferSize, m_usage, m_format, m_pool, &m_d3dIndexBuffer, 0);
		RENDERER_ASSERT(m_d3dIndexBuffer, "Failed to create Direct3D9 Index Buffer.");
#if PX_WINDOWS && PX_SUPPORT_GPU_PHYSX
		if(m_interopContext && m_d3dIndexBuffer && m_mustBeRegisteredInCUDA)
		{
			m_registeredInCUDA = m_interopContext->registerResourceInCudaD3D(m_InteropHandle, m_d3dIndexBuffer);
		}
#endif
	}
}

void *D3D9RendererIndexBuffer::lock(void)
{
	void *buffer = 0;
	if(m_d3dIndexBuffer)
	{
		const Format format     = getFormat();
		const PxU32  maxIndices = getMaxIndices();
		const PxU32  bufferSize = maxIndices * getFormatByteSize(format);
		if(bufferSize > 0)
		{
			m_d3dIndexBuffer->Lock(0, (UINT)bufferSize, &buffer, 0);
		}
	}
	return buffer;
}

void D3D9RendererIndexBuffer::unlock(void)
{
	if(m_d3dIndexBuffer)
	{
		m_d3dIndexBuffer->Unlock();
	}
}

void D3D9RendererIndexBuffer::bind(void) const
{
	m_d3dDevice.SetIndices(m_d3dIndexBuffer);
}

void D3D9RendererIndexBuffer::unbind(void) const
{
	m_d3dDevice.SetIndices(0);
}

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
