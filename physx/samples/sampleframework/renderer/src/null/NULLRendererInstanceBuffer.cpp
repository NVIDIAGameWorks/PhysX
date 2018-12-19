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

#include "NULLRendererInstanceBuffer.h"
#include <RendererInstanceBufferDesc.h>

using namespace SampleRenderer;

NullRendererInstanceBuffer::NullRendererInstanceBuffer(const RendererInstanceBufferDesc &desc) :
	RendererInstanceBuffer(desc)
{	
	m_bufferSize   = (PxU32)(desc.maxInstances * m_stride);
	m_buffer       = new PxU8[m_bufferSize];

	RENDERER_ASSERT(m_buffer, "Failed to create Vertex Buffer Object.");
		
	m_maxInstances = desc.maxInstances;
}

NullRendererInstanceBuffer::~NullRendererInstanceBuffer(void)
{
	if(m_buffer)
	{
		delete [] m_buffer;
		m_buffer = NULL;
	}
}


void *NullRendererInstanceBuffer::lock(void)
{
	return m_buffer;
}

void NullRendererInstanceBuffer::unlock(void)
{

}

void NullRendererInstanceBuffer::bind(PxU32 streamID, PxU32 firstInstance) const
{
}

void NullRendererInstanceBuffer::unbind(PxU32 streamID) const
{

}

