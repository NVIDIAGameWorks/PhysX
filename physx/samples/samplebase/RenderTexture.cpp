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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "SamplePreprocessor.h"
#include "RendererMemoryMacros.h"
#include "RenderTexture.h"
#include "Renderer.h"
#include "RendererTexture2DDesc.h"

using namespace physx;
using namespace SampleRenderer;

RenderTexture::RenderTexture(Renderer& renderer, PxU32 id, PxU32 width, PxU32 height, const void* data) :
	mID				(id),
	mTexture		(NULL),
	mOwnsTexture	(true)
{
	RendererTexture2DDesc tdesc;
  tdesc.format  = RendererTexture2D::FORMAT_B8G8R8A8;
	tdesc.width		= width;
	tdesc.height	= height;
	tdesc.numLevels	= 1;
/*
	tdesc.filter;
	tdesc.addressingU;
	tdesc.addressingV;
	tdesc.renderTarget;
*/
	PX_ASSERT(tdesc.isValid());
	mTexture		= renderer.createTexture2D(tdesc);
	PX_ASSERT(mTexture);

	const PxU32 componentCount = 4;

	if(mTexture)
	{
		PxU32 pitch = 0;
		void* buffer = mTexture->lockLevel(0, pitch);
		PX_ASSERT(buffer);
		if(buffer)
		{
			PxU8* levelDst			= (PxU8*)buffer;
			const PxU8* levelSrc	= (PxU8*)data;
			const PxU32 levelWidth	= mTexture->getWidthInBlocks();
			const PxU32 levelHeight	= mTexture->getHeightInBlocks();
			const PxU32 rowSrcSize	= levelWidth * mTexture->getBlockSize();
			PX_UNUSED(rowSrcSize);
			PX_ASSERT(rowSrcSize <= pitch); // the pitch can't be less than the source row size.
			for(PxU32 row=0; row<levelHeight; row++)
			{ 
				// copy per pixel to handle RBG case, based on component count
				for(PxU32 col=0; col<levelWidth; col++)
				{
					*levelDst++ = levelSrc[0];
					*levelDst++ = levelSrc[1];
					*levelDst++ = levelSrc[2];
					*levelDst++ = 0xFF; //alpha
					levelSrc += componentCount;
				}
			}
		}
		mTexture->unlockLevel(0);
	}
}

RenderTexture::RenderTexture(Renderer& renderer, PxU32 id, RendererTexture2D* texture) :
	mID				(id),
	mTexture		(texture),
	mOwnsTexture	(false)
{
}

RenderTexture::~RenderTexture()
{
	if(mOwnsTexture)
		SAFE_RELEASE(mTexture);
}
