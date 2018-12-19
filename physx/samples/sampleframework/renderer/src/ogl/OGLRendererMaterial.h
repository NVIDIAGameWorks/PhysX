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
#ifndef OGL_RENDERER_MATERIAL_H
#define OGL_RENDERER_MATERIAL_H

#include <RendererConfig.h>

#if defined(RENDERER_ENABLE_OPENGL)

#include <RendererMaterial.h>

#include "OGLRenderer.h"

namespace SampleRenderer
{

	class OGLRendererMaterial : public RendererMaterial
	{
		friend class OGLRendererMesh;
	public:
		OGLRendererMaterial(OGLRenderer &renderer, const RendererMaterialDesc &desc);
		virtual ~OGLRendererMaterial(void);
		virtual void setModelMatrix(const float *matrix) 
		{
			PX_UNUSED(matrix);
			PX_ALWAYS_ASSERT();
		}

	private:
		virtual const Renderer& getRenderer() const { return m_renderer; }
		virtual void bind(RendererMaterial::Pass pass, RendererMaterialInstance *materialInstance, bool instanced) const;
		virtual void bindMeshState(bool instanced) const;
		virtual void unbind(void) const;
		virtual void bindVariable(Pass pass, const Variable &variable, const void *data) const;

#if defined(RENDERER_ENABLE_CG)
		void loadCustomConstants(CGprogram program, Pass pass);
#endif

	private:
#if defined(RENDERER_ENABLE_CG)
		class CGVariable : public Variable
		{
			friend class OGLRendererMaterial;
		public:
			CGVariable(const char *name, VariableType type, PxU32 offset);
			virtual ~CGVariable(void);

			void addVertexHandle(CGparameter handle);
			void addFragmentHandle(CGparameter handle, Pass pass);

		private:
			CGparameter m_vertexHandle;
			CGparameter m_fragmentHandles[NUM_PASSES];
		};
#endif

	private:
		OGLRendererMaterial &operator=(const OGLRendererMaterial&) { return *this; }

	private:
		OGLRenderer &m_renderer;

		GLenum       m_glAlphaTestFunc;

#if defined(RENDERER_ENABLE_CG)
		CGprofile    m_vertexProfile;
		CGprogram    m_vertexProgram;

		CGprofile    m_fragmentProfile;
		CGprogram    m_fragmentPrograms[NUM_PASSES];
#endif
	};

} // namespace SampleRenderer

#endif // #if defined(RENDERER_ENABLE_OPENGL)
#endif
