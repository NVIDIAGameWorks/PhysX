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

