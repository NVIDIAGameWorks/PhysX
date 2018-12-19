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

#include "NULLRendererTexture2D.h"
#include <RendererTexture2DDesc.h>

using namespace SampleRenderer;

NullRendererTexture2D::NullRendererTexture2D(const RendererTexture2DDesc &desc) :
RendererTexture2D(desc)
{	
	PxU32 width     = desc.width;
	PxU32 height    = desc.height;
	
	m_numLevels = desc.numLevels;
	m_data = new PxU8*[m_numLevels];
	m_width = new PxU32[m_numLevels];
	memset(m_data, 0, sizeof(PxU8*)*m_numLevels);
	
	for(PxU32 i=0; i<desc.numLevels; i++)
	{
		PxU32 w = getLevelDimension(width,  i);
		m_width[i] = w;
		PxU32 h = getLevelDimension(height, i);
		PxU32 levelSize = computeImageByteSize(w, h, 1, desc.format);		

		// allocate memory
		m_data[i] = new PxU8[levelSize];

		memset(m_data[i], 0, levelSize);
	}
}

NullRendererTexture2D::~NullRendererTexture2D(void)
{
	for(PxU32 i=0; i<m_numLevels; i++)
	{
		delete [] m_data[i];
	}

	if(m_data)
	{
		delete [] m_data;
	}

	if(m_width)
	{
		delete [] m_width;
	}
}

void *NullRendererTexture2D::lockLevel(PxU32 level, PxU32 &pitch)
{
	void *buffer = 0;
	pitch = 0;
	RENDERER_ASSERT(level < m_numLevels, "Level out of range!");
	if(level < m_numLevels)
	{
		buffer = m_data[level];
		PxU32 w = SampleRenderer::RendererTexture2D::getFormatNumBlocks(m_width[level],  getFormat());
		pitch = w * getBlockSize();
	}	
	return buffer;
}

void NullRendererTexture2D::unlockLevel(PxU32 level)
{
	RENDERER_ASSERT(level < m_numLevels, "Level out of range!");
}

void NullRendererTexture2D::bind(PxU32 textureUnit)
{
}

