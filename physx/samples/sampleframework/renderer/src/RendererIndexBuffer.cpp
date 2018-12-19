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
#include <RendererIndexBuffer.h>
#include <RendererIndexBufferDesc.h>

using namespace SampleRenderer;

PxU32 RendererIndexBuffer::getFormatByteSize(Format format)
{
	PxU32 size = 0;
	switch(format)
	{
	case FORMAT_UINT16: size = sizeof(PxU16); break;
	case FORMAT_UINT32: size = sizeof(PxU32); break;
	default: break;
	}
	RENDERER_ASSERT(size, "Unable to determine size of format.");
	return size;
}

RendererIndexBuffer::RendererIndexBuffer(const RendererIndexBufferDesc &desc) :
RendererInteropableBuffer(desc.registerInCUDA, desc.interopContext),
	m_hint(desc.hint),
	m_format(desc.format)
{
	m_maxIndices = 0;
}

RendererIndexBuffer::~RendererIndexBuffer(void)
{

}

RendererIndexBuffer::Hint RendererIndexBuffer::getHint(void) const
{
	return m_hint;
}

RendererIndexBuffer::Format RendererIndexBuffer::getFormat(void) const
{
	return m_format;
}

PxU32 RendererIndexBuffer::getMaxIndices(void) const
{
	return m_maxIndices;
}
