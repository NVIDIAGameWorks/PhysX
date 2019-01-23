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
