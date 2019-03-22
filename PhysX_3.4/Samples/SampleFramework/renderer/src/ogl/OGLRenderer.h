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
#ifndef OGL_RENDERER_H
#define OGL_RENDERER_H

#include <RendererConfig.h>

#if defined(RENDERER_ENABLE_OPENGL)
#include <GLIncludes.h>

#if defined(RENDERER_ENABLE_CG)
	#include <Cg/cg.h>
	#include <Cg/cgGL.h>
#endif

#include <Renderer.h>

namespace SampleFramework {
	class SamplePlatform;
}

namespace SampleRenderer
{

#if defined(RENDERER_ENABLE_CG)
	void setColorParameter(CGparameter param, const RendererColor &color);
	void setFloat4Parameter(CGparameter param, float* values);
#endif

	class OGLRendererMaterial;


	void PxToGL(GLfloat *gl44, const physx::PxMat44 &mat);
	void RenToGL(GLfloat *gl44, const RendererProjection &proj);

	class OGLRenderer : public Renderer
	{
	public:
		OGLRenderer(const RendererDesc &desc, const char* assetDir);
		virtual ~OGLRenderer(void);

#if defined(RENDERER_ENABLE_CG)
		class CGEnvironment
		{
		public:
			CGparameter modelMatrix;
			CGparameter viewMatrix;
			CGparameter projMatrix;
			CGparameter modelViewMatrix;
			CGparameter modelViewProjMatrix;

			CGparameter boneMatrices;

			CGparameter fogColorAndDistance;

			CGparameter eyePosition;
			CGparameter eyeDirection;

			CGparameter ambientColor;

			CGparameter lightColor;
			CGparameter lightIntensity;
			CGparameter lightDirection;
			CGparameter lightPosition;
			CGparameter lightInnerRadius;
			CGparameter lightOuterRadius;
			CGparameter lightInnerCone;
			CGparameter lightOuterCone;

		public:
			CGEnvironment(void);
			CGEnvironment(CGcontext cgContext);
		};

		CGcontext            getCGContext(void)           { return m_cgContext; }
		CGEnvironment       &getCGEnvironment(void)       { return m_cgEnv; }
		const CGEnvironment &getCGEnvironment(void) const { return m_cgEnv; }
#endif

		const physx::PxMat44      &getViewMatrix(void) const { return m_viewMatrix; }

		void                       setCurrentMaterial(const OGLRendererMaterial *cm) { m_currMaterial = cm;   }
		const OGLRendererMaterial *getCurrentMaterial(void)                          { return m_currMaterial; }

	private:
		bool begin(void);
		void end(void);
		void checkResize(void);

	public:
		// clears the offscreen buffers.
		virtual void clearBuffers(void);

		// presents the current color buffer to the screen.
		// returns true on device reset and if buffers need to be rewritten.
		virtual bool swapBuffers(void);

		// get the device pointer (void * abstraction)
		virtual void *getDevice()                         { return static_cast<void*>(getCGContext()); }

        virtual bool captureScreen(const char* )  { PX_ASSERT("not supported"); return false; }

		// gets a handle to the current frame's data, in bitmap format
		//    note: subsequent calls will invalidate any previously returned data
		//    return true on successful screenshot capture
		virtual bool captureScreen(PxU32 &width, PxU32& height, PxU32& sizeInBytes, const void*& screenshotData);

		// get the window size
		void getWindowSize(PxU32 &width, PxU32 &height) const;

		virtual RendererVertexBuffer   *createVertexBuffer(  const RendererVertexBufferDesc   &desc);
		virtual RendererIndexBuffer    *createIndexBuffer(   const RendererIndexBufferDesc    &desc);
		virtual RendererInstanceBuffer *createInstanceBuffer(const RendererInstanceBufferDesc &desc);
		virtual RendererTexture2D      *createTexture2D(     const RendererTexture2DDesc      &desc);
		virtual RendererTexture3D      *createTexture3D(     const RendererTexture3DDesc      &desc);
		virtual RendererTarget         *createTarget(        const RendererTargetDesc         &desc);
		virtual RendererMaterial       *createMaterial(      const RendererMaterialDesc       &desc);
		virtual RendererMesh           *createMesh(          const RendererMeshDesc           &desc);
		virtual RendererLight          *createLight(         const RendererLightDesc          &desc);

		virtual void                    setVsync(bool on);

	private:
		virtual void bindViewProj(const physx::PxMat44 &inveye, const RendererProjection &proj);
		virtual void bindAmbientState(const RendererColor &ambientColor);
		virtual void bindFogState(const RendererColor &fogColor, float fogDistance);
		virtual void bindDeferredState(void);
		virtual void bindMeshContext(const RendererMeshContext &context);
		virtual void beginMultiPass(void);
		virtual void endMultiPass(void);
		virtual void beginTransparentMultiPass(void);
		virtual void endTransparentMultiPass(void);
		virtual void renderDeferredLight(const RendererLight &light);
		virtual PxU32 convertColor(const RendererColor& color) const;

		virtual bool isOk(void) const;

		virtual	void setupTextRenderStates();
		virtual	void resetTextRenderStates();
		virtual	void renderTextBuffer(const void* vertices, PxU32 nbVerts, const PxU16* indices, PxU32 nbIndices, RendererMaterial* material);
		virtual	void renderLines2D(const void* vertices, PxU32 nbVerts);
		virtual	void setupScreenquadRenderStates();
		virtual	void resetScreenquadRenderStates();

	private:
		SampleFramework::SamplePlatform*			   m_platform;
#if defined(RENDERER_ENABLE_CG)
		CGcontext                  m_cgContext;
		CGEnvironment              m_cgEnv;
#endif

		const OGLRendererMaterial *m_currMaterial;

		PxU32                      m_displayWidth;
		PxU32                      m_displayHeight;

		std::vector<GLubyte>       m_displayData;

		physx::PxMat44             m_viewMatrix;
	};

} // namespace SampleRenderer

#endif // #if defined(RENDERER_ENABLE_OPENGL)
#endif
