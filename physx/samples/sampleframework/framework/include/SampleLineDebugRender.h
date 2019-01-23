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


#ifndef SAMPLE_LINE_DEBUG_RENDER_H
#define SAMPLE_LINE_DEBUG_RENDER_H

#include <RendererMeshContext.h>

#include <FrameworkFoundation.h>

namespace SampleRenderer
{
	class Renderer;
	class RendererColor;
	class RendererVertexBuffer;
}

namespace SampleFramework
{

	class SampleAssetManager;
	class SampleMaterialAsset;

	class SampleLineDebugRender
	{
	public:
		void addLine(const PxVec3 &p0, const PxVec3 &p1, const SampleRenderer::RendererColor &color);
		void checkResizeLine(PxU32 maxVerts);
		void queueForRenderLine(void);
		void clearLine(void);
	protected:
		SampleLineDebugRender(SampleRenderer::Renderer &renderer, SampleAssetManager &assetmanager);
		virtual ~SampleLineDebugRender(void);

	private:
		void checkLock(void);
		void checkUnlock(void);
		void addVert(const PxVec3 &p, const SampleRenderer::RendererColor &color);

	private:
		SampleLineDebugRender &operator=(const SampleLineDebugRender&) { return *this; }

	private:
		SampleRenderer::Renderer             &m_renderer;
		SampleAssetManager                   &m_assetmanager;

		SampleMaterialAsset                  *m_material;

		PxU32                                 m_maxVerts;
		PxU32                                 m_numVerts;
		SampleRenderer::RendererVertexBuffer *m_vertexbuffer;
		SampleRenderer::RendererMesh         *m_mesh;
		SampleRenderer::RendererMeshContext   m_meshContext;

		void                                 *m_lockedPositions;
		PxU32                                 m_positionStride;

		void                                 *m_lockedColors;
		PxU32                                 m_colorStride;
	};

} // namespace SampleFramework

#endif
