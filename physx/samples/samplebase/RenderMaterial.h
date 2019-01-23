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

#ifndef RENDER_MATERIAL_H
#define RENDER_MATERIAL_H

#include "RenderBaseObject.h"
#include "common/PxPhysXCommonConfig.h"
#include "foundation/PxVec3.h"

namespace SampleRenderer
{
	class Renderer;
	class RendererMaterial;
	class RendererMaterialInstance;
}

	class RenderTexture;

	class RenderMaterial : public RenderBaseObject
	{
		public:
															RenderMaterial(SampleRenderer::Renderer& renderer, 
																			const PxVec3& diffuseColor, 
																			PxReal opacity, 
																			bool doubleSided, 
																			PxU32 id, 
																			RenderTexture* texture,
																			bool lit = true,
																			bool flat = false,
																			bool instanced = false);

															RenderMaterial(SampleRenderer::Renderer& renderer, 
																			SampleRenderer::RendererMaterial* mat, 
																			SampleRenderer::RendererMaterialInstance* matInstance, 
																			PxU32 id);
		virtual												~RenderMaterial();

		// the intent of this function is to update shaders variables, when needed (e.g. on resize)
		virtual void										update(SampleRenderer::Renderer& renderer);
				void										setDiffuseColor(const PxVec4& color);
				void										setParticleSize(const PxReal particleSize);
				void										setShadeMode(bool flat);

				SampleRenderer::RendererMaterial*			mRenderMaterial;
				SampleRenderer::RendererMaterialInstance*	mRenderMaterialInstance;
				PxU32										mID;
				bool										mDoubleSided;
				bool										mOwnsRendererMaterial;
	};

#endif
