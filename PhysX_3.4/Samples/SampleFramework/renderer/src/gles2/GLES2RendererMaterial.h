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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.


#ifndef GLES2_RENDERER_MATERIAL_H
#define GLES2_RENDERER_MATERIAL_H

#include <RendererConfig.h>

#if defined(RENDERER_ENABLE_GLES2)

#include <RendererMaterial.h>
#include "GLES2Renderer.h"

#if defined(RENDERER_ANDROID)
#include "android/AndroidSamplePlatform.h"
#elif defined(RENDERER_IOS)
#include "ios/IosSamplePlatform.h"
#endif

#include <set>

#include <set>

namespace SampleRenderer
{

class GLES2RendererMaterial : public RendererMaterial
{
	public:
		GLES2RendererMaterial(GLES2Renderer &renderer, const RendererMaterialDesc &desc);
		virtual ~GLES2RendererMaterial(void);
		virtual void setModelMatrix(const float *matrix) 
		{
			PX_UNUSED(matrix);
			PX_ALWAYS_ASSERT();
		}
		virtual const Renderer& getRenderer() const { return m_renderer; }
		
		/* Actually executes glUniform* and submits data saved in the m_program[m_currentPass].vsUniformsVec4/psUniformsVec4 */
		void submitUniforms();

	private:
		virtual void bind(RendererMaterial::Pass pass, RendererMaterialInstance *materialInstance, bool instanced) const;
		virtual void bindMeshState(bool instanced) const;
		virtual void unbind(void) const;
		virtual void bindVariable(Pass pass, const Variable &variable, const void *data) const;
    
        VariableType 	GLTypetoVariableType(GLenum type);
		GLint			getAttribLocation(size_t usage, size_t index, Pass pass);
		std::string 	mojoSampleNameToOriginal(Pass pass, const std::string& name);
		
	private:
		GLES2RendererMaterial &operator=(const GLES2RendererMaterial&) { return *this; }
		
	private:
        void loadCustomConstants(Pass pass);

        friend class GLES2Renderer;
        friend class GLES2RendererVertexBuffer;
        friend class GLES2RendererDirectionalLight;
        GLES2Renderer &m_renderer;
		
		GLenum       m_glAlphaTestFunc;

		mutable struct shaderProgram {
			GLuint      vertexShader;
			GLuint      fragmentShader;
			GLuint      program;

			GLint       modelMatrixUniform;
			GLint       viewMatrixUniform;
			GLint       projMatrixUniform;
			GLint       modelViewMatrixUniform;
			GLint       boneMatricesUniform;
			GLint		modelViewProjMatrixUniform;
		
			GLint       positionAttr;
			GLint       colorAttr;
			GLint       normalAttr;
			GLint       tangentAttr;
			GLint       boneIndexAttr;
			GLint       boneWeightAttr;

			GLint       texcoordAttr[8];
			
			mutable std::set<std::string>	vsUniformsCollected;
			mutable std::set<std::string>	psUniformsCollected;
			
			size_t		vsUniformsTotal;
			size_t		psUniformsTotal;
			
			char*		vsUniformsVec4;
			GLuint		vsUniformsVec4Location;
			size_t		vsUniformsVec4Size;
			size_t		vsUniformsVec4SizeInBytes;
			char*		psUniformsVec4;
			GLuint		psUniformsVec4Location;
			size_t		psUniformsVec4Size;
			size_t		psUniformsVec4SizeInBytes;
			
			mojoResult* vertexMojoResult;
			mojoResult* fragmentMojoResult;
			
			shaderProgram();
			~shaderProgram();
		}	m_program[NUM_PASSES];

		mutable Pass	m_currentPass;
};

}

#endif // #if defined(RENDERER_ENABLE_GLES2)
#endif
