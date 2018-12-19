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

#include "RendererConfig.h"
#include <SamplePlatform.h>

#include "NULLRendererVertexBuffer.h"

#include <RendererVertexBufferDesc.h>


using namespace SampleRenderer;

NullRendererVertexBuffer::NullRendererVertexBuffer(const RendererVertexBufferDesc &desc) 
	:RendererVertexBuffer(desc),m_buffer(0)
{			
	RENDERER_ASSERT(m_stride && desc.maxVertices, "Unable to create Vertex Buffer of zero size.");
	if(m_stride && desc.maxVertices)
	{		
		m_buffer = new PxU8[m_stride * desc.maxVertices];
		
		RENDERER_ASSERT(m_buffer, "Failed to create Vertex Buffer Object.");

		m_maxVertices = desc.maxVertices;
	}
}

NullRendererVertexBuffer::~NullRendererVertexBuffer(void)
{
	if(m_buffer)
	{
		delete [] m_buffer;
		m_buffer = NULL;
	}
}

void NullRendererVertexBuffer::swizzleColor(void *colors, PxU32 stride, PxU32 numColors, RendererVertexBuffer::Format inFormat)
{
}

void *NullRendererVertexBuffer::lock(void)
{	
	return m_buffer;
}

void NullRendererVertexBuffer::unlock(void)
{
}


void NullRendererVertexBuffer::bind(PxU32 streamID, PxU32 firstVertex)
{
}

void NullRendererVertexBuffer::unbind(PxU32 streamID)
{
}

