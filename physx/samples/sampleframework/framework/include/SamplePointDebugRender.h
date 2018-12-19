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


#ifndef SAMPLE_POINT_DEBUG_RENDER_H
#define SAMPLE_POINT_DEBUG_RENDER_H

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

	class SamplePointDebugRender
	{
	public:
		void addPoint(const PxVec3 &p0, const SampleRenderer::RendererColor &color);

		void queueForRenderPoint(void);

	protected:
		SamplePointDebugRender(SampleRenderer::Renderer &renderer, SampleAssetManager &assetmanager);
		virtual ~SamplePointDebugRender(void);

		void checkResizePoint(PxU32 maxVerts);
		void clearPoint(void);

	private:
		void checkLock(void);
		void checkUnlock(void);
		void addVert(const PxVec3 &p, const SampleRenderer::RendererColor &color);

		SamplePointDebugRender &operator=(const SamplePointDebugRender&) { return *this; }

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
