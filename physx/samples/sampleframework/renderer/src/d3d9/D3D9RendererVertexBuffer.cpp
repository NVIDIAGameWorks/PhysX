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

#include "D3D9RendererVertexBuffer.h"
#include <RendererVertexBufferDesc.h>

#if PX_WINDOWS
#include <task/PxTask.h>
#endif

using namespace SampleRenderer;

static D3DVERTEXELEMENT9 buildVertexElement(WORD stream, WORD offset, D3DDECLTYPE type, BYTE method, BYTE usage, BYTE usageIndex)
{
	D3DVERTEXELEMENT9 element;
	element.Stream     = stream;
	element.Offset     = offset;
#if defined(RENDERER_WINDOWS)
	element.Type       = (BYTE)type;
#else
	element.Type       = type;
#endif
	element.Method     = method;
	element.Usage      = usage;
	element.UsageIndex = usageIndex;
	return element;
}

static D3DDECLTYPE getD3DType(RendererVertexBuffer::Format format)
{
	D3DDECLTYPE d3dType = D3DDECLTYPE_UNUSED;
	switch(format)
	{
	case RendererVertexBuffer::FORMAT_FLOAT1:		d3dType = D3DDECLTYPE_FLOAT1;   break;
	case RendererVertexBuffer::FORMAT_FLOAT2:		d3dType = D3DDECLTYPE_FLOAT2;   break;
	case RendererVertexBuffer::FORMAT_FLOAT3:		d3dType = D3DDECLTYPE_FLOAT3;   break;
	case RendererVertexBuffer::FORMAT_FLOAT4:		d3dType = D3DDECLTYPE_FLOAT4;   break;
	case RendererVertexBuffer::FORMAT_UBYTE4:		d3dType = D3DDECLTYPE_UBYTE4;   break;
	case RendererVertexBuffer::FORMAT_USHORT4:		d3dType = D3DDECLTYPE_SHORT4;   break;
	case RendererVertexBuffer::FORMAT_COLOR_BGRA:	d3dType = D3DDECLTYPE_D3DCOLOR; break;
	case RendererVertexBuffer::FORMAT_COLOR_RGBA:	d3dType = D3DDECLTYPE_D3DCOLOR; break;
	case RendererVertexBuffer::FORMAT_COLOR_NATIVE:	d3dType = D3DDECLTYPE_D3DCOLOR; break;
	}
	RENDERER_ASSERT(d3dType != D3DDECLTYPE_UNUSED, "Invalid Direct3D9 vertex type.");
	return d3dType;
}

static D3DDECLUSAGE getD3DUsage(RendererVertexBuffer::Semantic semantic, PxU8 &usageIndex)
{
	D3DDECLUSAGE d3dUsage = D3DDECLUSAGE_FOG;
	usageIndex = 0;
	if(semantic >= RendererVertexBuffer::SEMANTIC_TEXCOORD0 && semantic <= RendererVertexBuffer::SEMANTIC_TEXCOORDMAX)
	{
		d3dUsage   = D3DDECLUSAGE_TEXCOORD;
		usageIndex = (PxU8)(semantic - RendererVertexBuffer::SEMANTIC_TEXCOORD0);
	}
	else
	{
		switch(semantic)
		{
		case RendererVertexBuffer::SEMANTIC_POSITION:  d3dUsage = D3DDECLUSAGE_POSITION; break;
		case RendererVertexBuffer::SEMANTIC_NORMAL:    d3dUsage = D3DDECLUSAGE_NORMAL;   break;
		case RendererVertexBuffer::SEMANTIC_TANGENT:   d3dUsage = D3DDECLUSAGE_TANGENT;  break;
		case RendererVertexBuffer::SEMANTIC_COLOR:     d3dUsage = D3DDECLUSAGE_COLOR;    break;
		case RendererVertexBuffer::SEMANTIC_BONEINDEX:
			d3dUsage   = D3DDECLUSAGE_TEXCOORD;
			usageIndex = RENDERER_BONEINDEX_CHANNEL;
			break;
		case RendererVertexBuffer::SEMANTIC_BONEWEIGHT:
			d3dUsage   = D3DDECLUSAGE_TEXCOORD;
			usageIndex = RENDERER_BONEWEIGHT_CHANNEL;
			break;
		}
	}
	// There's absolutely no reason why there can't be semantics for which Direct3D9 has no support
	//RENDERER_ASSERT(d3dUsage != D3DDECLUSAGE_FOG, "Invalid Direct3D9 vertex usage.");
	return d3dUsage;
}

D3D9RendererVertexBuffer::D3D9RendererVertexBuffer(IDirect3DDevice9 &d3dDevice, D3D9Renderer& renderer, const RendererVertexBufferDesc &desc) :
RendererVertexBuffer(desc),
	m_d3dDevice(d3dDevice),
	m_savedData(NULL)
{
	m_d3dVertexBuffer = 0;

	m_usage      = D3DUSAGE_WRITEONLY;
	m_lockFlags  = 0;
	m_pool       = renderer.canUseManagedResources() ? D3DPOOL_MANAGED : D3DPOOL_DEFAULT;
	m_bufferSize = (UINT)(desc.maxVertices * m_stride);

	m_bufferWritten = false;

#if RENDERER_ENABLE_DYNAMIC_VB_POOLS
	if(desc.hint==RendererVertexBuffer::HINT_DYNAMIC )
	{
		if (!desc.registerInCUDA)
		{
			m_usage = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
			// discard all that data if we are going to overwrite it again (saves some time waiting to copy everything back)
			m_lockFlags = D3DLOCK_DISCARD;
		}
		else
		{
			m_usage = 0;
		}

		m_pool  = D3DPOOL_DEFAULT;
	}
#endif

	onDeviceReset();

	if(m_d3dVertexBuffer)
	{
		m_maxVertices = desc.maxVertices;
	}
}

D3D9RendererVertexBuffer::~D3D9RendererVertexBuffer(void)
{
	if(m_d3dVertexBuffer)
	{
#if PX_WINDOWS && PX_SUPPORT_GPU_PHYSX
		if(m_interopContext && m_registeredInCUDA)
		{
			m_registeredInCUDA = !m_interopContext->unregisterResourceInCuda(m_InteropHandle);
		}
#endif

		m_d3dVertexBuffer->Release();
	}

	delete[] m_savedData;
}

void D3D9RendererVertexBuffer::addVertexElements(PxU32 streamIndex, std::vector<D3DVERTEXELEMENT9> &vertexElements) const
{
	for(PxU32 i=0; i<NUM_SEMANTICS; i++)
	{
		Semantic semantic = (Semantic)i;
		const SemanticDesc &sm = m_semanticDescs[semantic];
		if(sm.format < NUM_FORMATS)
		{
			PxU8 d3dUsageIndex  = 0;
			D3DDECLUSAGE d3dUsage = getD3DUsage(semantic, d3dUsageIndex);
			if (D3DDECLUSAGE_FOG != d3dUsage)
			{
				vertexElements.push_back(buildVertexElement((WORD)streamIndex, (WORD)sm.offset, getD3DType(sm.format), D3DDECLMETHOD_DEFAULT, (BYTE)d3dUsage, d3dUsageIndex));
			}
		}
	}
}

void D3D9RendererVertexBuffer::swizzleColor(void *colors, PxU32 stride, PxU32 numColors, RendererVertexBuffer::Format inFormat)
{
	if (inFormat == RendererVertexBuffer::FORMAT_COLOR_RGBA)
	{
		const void *end = ((PxU8*)colors)+(stride*numColors);
		for(PxU8* iterator = (PxU8*)colors; iterator < end; iterator+=stride)
		{
			std::swap(((PxU8*)iterator)[0], ((PxU8*)iterator)[2]);
		}
	}
}

void *D3D9RendererVertexBuffer::lock(void)
{
	PX_PROFILE_ZONE("D3D9RenderVBlock",0);
	void *lockedBuffer = 0;
	if(m_d3dVertexBuffer)
	{
		const PxU32 bufferSize = m_maxVertices * m_stride;
		HRESULT res = m_d3dVertexBuffer->Lock(0, (UINT)bufferSize, &lockedBuffer, m_lockFlags);
		if(res != S_OK)
			lockedBuffer = NULL;
		RENDERER_ASSERT(lockedBuffer, "Failed to lock Direct3D9 Vertex Buffer.");
	}
	m_bufferWritten = true;
	return lockedBuffer;
}

void D3D9RendererVertexBuffer::unlock(void)
{
	PX_PROFILE_ZONE("D3D9RenderVBunlock",0);
	if(m_d3dVertexBuffer)
	{
		m_d3dVertexBuffer->Unlock();
	}
}

void D3D9RendererVertexBuffer::bind(PxU32 streamID, PxU32 firstVertex)
{
	prepareForRender();
	if(m_d3dVertexBuffer)
	{
		m_d3dDevice.SetStreamSource((UINT)streamID, m_d3dVertexBuffer, firstVertex*m_stride, m_stride);
	}
}

void D3D9RendererVertexBuffer::unbind(PxU32 streamID)
{
	m_d3dDevice.SetStreamSource((UINT)streamID, 0, 0, 0);
}

void D3D9RendererVertexBuffer::onDeviceLost(void)
{
#if PX_WINDOWS && PX_SUPPORT_GPU_PHYSX
	if(m_interopContext && m_registeredInCUDA)
	{
		m_interopContext->unregisterResourceInCuda(m_InteropHandle);
	}
#endif
	m_registeredInCUDA = false;

	if(m_pool != D3DPOOL_MANAGED && m_d3dVertexBuffer)
	{
		if (m_bufferWritten)
		{
			if (NULL == m_savedData)
				m_savedData = new char[m_bufferSize];

			void* lockedBuffer = 0;
			m_d3dVertexBuffer->Lock(0, m_bufferSize, &lockedBuffer, m_lockFlags);
			RENDERER_ASSERT(lockedBuffer, "Failed to lock Direct3D9 Vertex Buffer.");

			memcpy(m_savedData, lockedBuffer, m_bufferSize);
			m_d3dVertexBuffer->Unlock();
		}

		m_d3dVertexBuffer->Release();
		m_d3dVertexBuffer = 0;
	}
}

void D3D9RendererVertexBuffer::onDeviceReset(void)
{
	if(!m_d3dVertexBuffer)
	{
		m_d3dDevice.CreateVertexBuffer(m_bufferSize, m_usage, 0, m_pool, &m_d3dVertexBuffer, 0);
		RENDERER_ASSERT(m_d3dVertexBuffer, "Failed to create Direct3D9 Vertex Buffer.");
#if PX_WINDOWS && PX_SUPPORT_GPU_PHYSX
		if(m_interopContext && m_d3dVertexBuffer && m_mustBeRegisteredInCUDA)
		{
			RENDERER_ASSERT(m_deferredUnlock == false, "Deferred VB Unlock must be disabled when CUDA Interop is in use.")
				m_registeredInCUDA = m_interopContext->registerResourceInCudaD3D(m_InteropHandle, m_d3dVertexBuffer);
		}
#endif

		if (m_bufferWritten)
		{
			void* lockedBuffer = 0;
			m_d3dVertexBuffer->Lock(0, m_bufferSize, &lockedBuffer, m_lockFlags);
			RENDERER_ASSERT(lockedBuffer, "Failed to lock Direct3D9 Vertex Buffer.");

			RENDERER_ASSERT(m_savedData != NULL, "Data buffer must have been allocated.");
			memcpy(lockedBuffer, m_savedData, m_bufferSize);
			m_d3dVertexBuffer->Unlock();
		}
	}
}

bool D3D9RendererVertexBuffer::checkBufferWritten(void)
{
	if(m_InteropHandle)
		return true;
	else
		return m_bufferWritten;
}

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
