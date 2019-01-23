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

#if defined(RENDERER_ENABLE_OPENGL)

#include "OGLRenderer.h"

#include <RendererDesc.h>

#include <RendererVertexBufferDesc.h>
#include "OGLRendererVertexBuffer.h"

#include <RendererIndexBufferDesc.h>
#include "OGLRendererIndexBuffer.h"

#include <RendererInstanceBufferDesc.h>
#include "OGLRendererInstanceBuffer.h"

#include <RendererMeshDesc.h>
#include <RendererMeshContext.h>
#include "OGLRendererMesh.h"

#include <RendererMaterialDesc.h>
#include <RendererMaterialInstance.h>
#include "OGLRendererMaterial.h"

#include <RendererLightDesc.h>
#include <RendererDirectionalLightDesc.h>
#include "OGLRendererDirectionalLight.h"
#include <RendererSpotLightDesc.h>
#include "OGLRendererSpotLight.h"

#include <RendererTexture2DDesc.h>
#include "OGLRendererTexture2D.h"

#include <RendererProjection.h>

#include <SamplePlatform.h>

#include "PxTkFile.h"
#include "PsString.h"

namespace Ps = physx::shdfnd;
 
using namespace SampleRenderer;

#if defined(RENDERER_ENABLE_CG)
	void SampleRenderer::setColorParameter(CGparameter param, const RendererColor &color)
	{
		const float inv255 = 1.0f / 255.0f;
		const float fcolor[3] = { color.r*inv255, color.g*inv255, color.b*inv255 };
		cgSetParameter3fv(param, fcolor);
	}

	void SampleRenderer::setFloat4Parameter(CGparameter param, float* values)
	{
		cgSetParameter4fv(param, values);
	}

#endif

#if defined(RENDERER_ENABLE_CG)

	static void CGENTRY onCgError(CGcontext cgContext, CGerror err, void * data)
	{
		const char *errstr = cgGetErrorString(err);
		if(cgContext)
		{
			errstr = cgGetLastListing(cgContext);
		}
		static_cast<SampleFramework::SamplePlatform*>(data)->showMessage("Cg error", errstr);
	}

	OGLRenderer::CGEnvironment::CGEnvironment(void)
	{
		memset(this, 0, sizeof(*this));
	}

	OGLRenderer::CGEnvironment::CGEnvironment(CGcontext cgContext)
	{
		modelMatrix         = cgCreateParameter(cgContext, CG_FLOAT4x4);
		viewMatrix          = cgCreateParameter(cgContext, CG_FLOAT4x4);
		projMatrix          = cgCreateParameter(cgContext, CG_FLOAT4x4);
		modelViewMatrix     = cgCreateParameter(cgContext, CG_FLOAT4x4);
		modelViewProjMatrix = cgCreateParameter(cgContext, CG_FLOAT4x4);

		boneMatrices        = cgCreateParameterArray(cgContext, CG_FLOAT4x4, RENDERER_MAX_BONES);

		fogColorAndDistance = cgCreateParameter(cgContext, CG_FLOAT4);

		eyePosition         = cgCreateParameter(cgContext, CG_FLOAT3);
		eyeDirection        = cgCreateParameter(cgContext, CG_FLOAT3);

		ambientColor        = cgCreateParameter(cgContext, CG_FLOAT3);

		lightColor          = cgCreateParameter(cgContext, CG_FLOAT3);
		lightIntensity      = cgCreateParameter(cgContext, CG_FLOAT);
		lightDirection      = cgCreateParameter(cgContext, CG_FLOAT3);
		lightPosition       = cgCreateParameter(cgContext, CG_FLOAT3);
		lightInnerRadius    = cgCreateParameter(cgContext, CG_FLOAT);
		lightOuterRadius    = cgCreateParameter(cgContext, CG_FLOAT);
		lightInnerCone      = cgCreateParameter(cgContext, CG_FLOAT);
		lightOuterCone      = cgCreateParameter(cgContext, CG_FLOAT);
	}
#endif

	void SampleRenderer::PxToGL(GLfloat *gl44, const physx::PxMat44 &mat44)
	{
		memcpy(gl44, mat44.front(), 4 * 4 * sizeof (GLfloat));
	}

	void SampleRenderer::RenToGL(GLfloat *gl44, const RendererProjection &proj)
	{
		proj.getColumnMajor44(gl44);
	}

	OGLRenderer::OGLRenderer(const RendererDesc &desc, const char* assetDir) :
	Renderer(DRIVER_OPENGL, desc.errorCallback, assetDir), m_platform(SampleFramework::SamplePlatform::platform())
	{
		m_useShadersForTextRendering	= false;
		m_displayWidth  = 0;
		m_displayHeight = 0;
		m_currMaterial  = 0;
		m_viewMatrix	= PxMat44(PxIdentity);

		m_platform->initializeOGLDisplay(desc, m_displayWidth, m_displayHeight);
		m_platform->postInitializeOGLDisplay();

		//added this to avoid garbage being displayed during initialization 
		RendererColor savedColor = getClearColor();
		setClearColor(RendererColor(0U));
		clearBuffers();
		swapBuffers();
		setClearColor(savedColor);

		checkResize();

#if defined(RENDERER_ENABLE_CG)
		m_cgContext = cgCreateContext();
		RENDERER_ASSERT(m_cgContext, "Unable to obtain Cg Context.");

		m_platform->initializeCGRuntimeCompiler();

		if(m_cgContext)
		{
			m_cgEnv = CGEnvironment(m_cgContext);

			cgSetErrorHandler(onCgError, m_platform);
			cgSetParameterSettingMode(m_cgContext, CG_DEFERRED_PARAMETER_SETTING);
#if defined(RENDERER_DEBUG)
			cgGLSetDebugMode(CG_TRUE);
#else
			cgGLSetDebugMode(CG_FALSE);
#endif
		}
#endif

		Ps::strlcpy(m_deviceName, sizeof(m_deviceName), (const char*)glGetString(GL_RENDERER));
		for(char *c=m_deviceName; *c; c++)
		{
			if(*c == '/')
			{
				*c = 0;
				break;
			}
		}

		RendererColor& clearColor = getClearColor();
		glClearColor(clearColor.r/255.0f, clearColor.g/255.0f, clearColor.b/255.0f, clearColor.a/255.0f);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CW);
		glEnable(GL_DEPTH_TEST);
	}

	OGLRenderer::~OGLRenderer(void)
	{
		// avoid displaying garbage on screen
		setClearColor(RendererColor(0U));
		clearBuffers();
		swapBuffers();
		releaseAllMaterials();

#if defined(RENDERER_ENABLE_CG)
		if(m_cgContext)
		{
			cgDestroyContext(m_cgContext);
		}
#endif

		m_platform->freeDisplay();
	}

	bool OGLRenderer::begin(void)
	{
		bool ok = false;
		ok = m_platform->makeContextCurrent();
		return ok;
	}

	void OGLRenderer::end(void)
	{
		GLenum error = glGetError();
		//RENDERER_ASSERT(GL_NO_ERROR == error, "An OpenGL error has occured");

		if (GL_NO_ERROR != error)
		{
			const char* errorString = "unknown";

#define CASE(MESSAGE) case MESSAGE: errorString = #MESSAGE; break;
			switch(error)
			{
				CASE(GL_INVALID_ENUM)
				CASE(GL_INVALID_VALUE)
				CASE(GL_INVALID_OPERATION)
				CASE(GL_STACK_OVERFLOW)
				CASE(GL_STACK_UNDERFLOW)
				CASE(GL_OUT_OF_MEMORY)
				CASE(GL_INVALID_FRAMEBUFFER_OPERATION)
			};

#undef CASE

			char buf[128];
			Ps::snprintf(buf, 128,  "GL error %x occurred (%s)\n", error, errorString);
			if (m_errorCallback)
				m_errorCallback->reportError(PxErrorCode::eINTERNAL_ERROR, buf, __FILE__, __LINE__);
		}
	}

	void OGLRenderer::checkResize(void)
	{
		PxU32 width  = m_displayWidth;
		PxU32 height = m_displayHeight;
		m_platform->getWindowSize(width, height);
		if(width != m_displayWidth || height != m_displayHeight)
		{
			m_displayWidth  = width;
			m_displayHeight = height;
			glViewport(0, 0, (GLsizei)m_displayWidth, (GLsizei)m_displayHeight);
			glScissor( 0, 0, (GLsizei)m_displayWidth, (GLsizei)m_displayHeight);
		}
	}

	// clears the offscreen buffers.
	void OGLRenderer::clearBuffers(void)
	{
		if(begin())
		{
			GLbitfield glbuf = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
			glDepthMask(1);

			RendererColor& clearColor = getClearColor();
			glClearColor(clearColor.r/255.0f, clearColor.g/255.0f, clearColor.b/255.0f, clearColor.a/255.0f);
			glClear(glbuf);
		}
		end();
	}

	// presents the current color buffer to the screen.
	// returns true on device reset and if buffers need to be rewritten.
	bool OGLRenderer::swapBuffers(void)
	{
		PX_PROFILE_ZONE("OGLRendererSwapBuffers",0);
		if(begin())
		{
			m_platform->swapBuffers();			
			checkResize();
		}
		end();
		return false;
	}

	void OGLRenderer::getWindowSize(PxU32 &width, PxU32 &height) const
	{
		RENDERER_ASSERT(m_displayHeight * m_displayWidth > 0, "variables not initialized properly");
		width = m_displayWidth;
		height = m_displayHeight;
	}

	RendererVertexBuffer *OGLRenderer::createVertexBuffer(const RendererVertexBufferDesc &desc)
	{
		PX_PROFILE_ZONE("OGLRenderer_createVertexBuffer",0);
		OGLRendererVertexBuffer *vb = 0;
		RENDERER_ASSERT(desc.isValid(), "Invalid Vertex Buffer Descriptor.");
		if(desc.isValid())
		{
			vb = new OGLRendererVertexBuffer(desc);
		}
		return vb;
	}

	RendererIndexBuffer *OGLRenderer::createIndexBuffer(const RendererIndexBufferDesc &desc)
	{
		PX_PROFILE_ZONE("OGLRenderer_createIndexBuffer",0);
		OGLRendererIndexBuffer *ib = 0;
		RENDERER_ASSERT(desc.isValid(), "Invalid Index Buffer Descriptor.");
		if(desc.isValid())
		{
			ib = new OGLRendererIndexBuffer(desc);
		}
		return ib;
	}

	RendererInstanceBuffer *OGLRenderer::createInstanceBuffer(const RendererInstanceBufferDesc &desc)
	{
		PX_PROFILE_ZONE("OGLRenderer_createInstanceBuffer",0);
		OGLRendererInstanceBuffer *ib = 0;
		RENDERER_ASSERT(desc.isValid(), "Invalid Instance Buffer Descriptor.");
		if(desc.isValid())
		{
			ib = new OGLRendererInstanceBuffer(desc);
		}
		return ib;
	}

	RendererTexture2D *OGLRenderer::createTexture2D(const RendererTexture2DDesc &desc)
	{
		PX_PROFILE_ZONE("OGLRenderer_createTexture2D",0);
		OGLRendererTexture2D *texture = 0;
		RENDERER_ASSERT(desc.isValid(), "Invalid Texture 2D Descriptor.");
		if(desc.isValid())
		{
			texture = new OGLRendererTexture2D(desc);
		}
		return texture;
	}

	RendererTexture3D *OGLRenderer::createTexture3D(const RendererTexture3DDesc &desc)
	{
		// TODO: Implement
		return 0;
	}

	RendererTarget *OGLRenderer::createTarget(const RendererTargetDesc &desc)
	{
		PX_PROFILE_ZONE("OGLRenderer_createTarget",0);
		RENDERER_ASSERT(0, "Not Implemented.");
		return 0;
	}

	RendererMaterial *OGLRenderer::createMaterial(const RendererMaterialDesc &desc)
	{
		RendererMaterial *mat = hasMaterialAlready(desc);
		RENDERER_ASSERT(desc.isValid(), "Invalid Material Descriptor.");
		if(!mat && desc.isValid())
		{
			PX_PROFILE_ZONE("OGLRenderer_createMaterial",0);
			mat = new OGLRendererMaterial(*this, desc);

			registerMaterial(desc, mat);
		}
		return mat;
	}

	RendererMesh *OGLRenderer::createMesh(const RendererMeshDesc &desc)
	{
		PX_PROFILE_ZONE("OGLRenderer_createMesh",0);
		OGLRendererMesh *mesh = 0;
		RENDERER_ASSERT(desc.isValid(), "Invalid Mesh Descriptor.");
		if(desc.isValid())
		{
			mesh = new OGLRendererMesh(*this, desc);
		}
		return mesh;
	}

	RendererLight *OGLRenderer::createLight(const RendererLightDesc &desc)
	{
		PX_PROFILE_ZONE("OGLRenderer_createLight",0);
		RendererLight *light = 0;
		RENDERER_ASSERT(desc.isValid(), "Invalid Light Descriptor.");
		if(desc.isValid())
		{
			switch(desc.type)
			{
			case RendererLight::TYPE_DIRECTIONAL:
				light = new OGLRendererDirectionalLight(*static_cast<const RendererDirectionalLightDesc*>(&desc), *this);
				break;
			case RendererLight::TYPE_SPOT:
				light = new OGLRendererSpotLight(*static_cast<const RendererSpotLightDesc*>(&desc), *this);
				break;
			default:
				RENDERER_ASSERT(0, "OGLRenderer does not support this light type");
				break;
			}
		}
		return light;
	}

	void OGLRenderer::setVsync(bool on)
	{
		m_platform->setOGLVsync(on);
	}

	void OGLRenderer::bindViewProj(const physx::PxMat44 &eye, const RendererProjection &proj)
	{
		PxMat44 inveye = eye.inverseRT();

		// load the projection matrix...
		const GLfloat* glproj = &proj.getPxMat44().column0.x;

		// load the view matrix...
		m_viewMatrix = inveye;

#if defined(RENDERER_ENABLE_CG)
		GLfloat glview[16];
		PxToGL(glview, inveye);
		const PxVec3 &eyePosition  =  eye.getPosition();
		const PxVec3  eyeDirection = -eye.getBasis(2);
		if(m_cgEnv.viewMatrix)	 cgGLSetMatrixParameterfc(m_cgEnv.viewMatrix,    glview);
		if(m_cgEnv.projMatrix)   cgGLSetMatrixParameterfc(m_cgEnv.projMatrix,    glproj);
		if(m_cgEnv.eyePosition)  cgGLSetParameter3fv(     m_cgEnv.eyePosition,  &eyePosition.x);
		if(m_cgEnv.eyeDirection) cgGLSetParameter3fv(     m_cgEnv.eyeDirection, &eyeDirection.x);
#endif
	}

	void OGLRenderer::bindFogState(const RendererColor &fogColor, float fogDistance)
	{
#if defined(RENDERER_ENABLE_CG)
		const float inv255 = 1.0f / 255.0f;

		float values[4];
		values[0] = fogColor.r*inv255;
		values[1] = fogColor.g*inv255;
		values[2] = fogColor.b*inv255;
		values[3] = fogDistance;

		setFloat4Parameter(m_cgEnv.fogColorAndDistance, values);
#endif
	}

	void OGLRenderer::bindAmbientState(const RendererColor &ambientColor)
	{
#if defined(RENDERER_ENABLE_CG)
		setColorParameter(m_cgEnv.ambientColor, ambientColor);
#endif
	}

	void OGLRenderer::bindDeferredState(void)
	{
		RENDERER_ASSERT(0, "Not implemented!");
	}

	void OGLRenderer::bindMeshContext(const RendererMeshContext &context)
	{
		PX_PROFILE_ZONE("OGLRendererBindMeshContext",0);
		physx::PxMat44 model;
		physx::PxMat44 modelView;

		if(context.transform) model = *context.transform;
		else                  model = PxMat44(PxIdentity);
		modelView = m_viewMatrix * model;

		GLfloat glmodelview[16];
		PxToGL(glmodelview, modelView);
		glLoadMatrixf(glmodelview);

		RendererMeshContext::CullMode cullMode = context.cullMode;
		if (!blendingCull() && NULL != context.material && context.material->getBlending())
			cullMode = RendererMeshContext::NONE;

		switch(cullMode)
		{
		case RendererMeshContext::CLOCKWISE: 
			glFrontFace( GL_CW );
			glCullFace( GL_BACK );
			glEnable( GL_CULL_FACE );
			break;
		case RendererMeshContext::COUNTER_CLOCKWISE: 
			glFrontFace( GL_CCW );
			glCullFace( GL_BACK );
			glEnable( GL_CULL_FACE );
			break;
		case RendererMeshContext::NONE: 
			glDisable( GL_CULL_FACE );
			break;
		default:
			RENDERER_ASSERT(0, "Invalid Cull Mode");
		}

		switch(context.fillMode)
		{
		case RendererMeshContext::SOLID:
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			break;
		case RendererMeshContext::LINE:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case RendererMeshContext::POINT:
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		}


#if defined(RENDERER_ENABLE_CG)
		GLfloat glmodel[16];
		PxToGL(glmodel, model);
		if(m_cgEnv.modelMatrix)     cgGLSetMatrixParameterfc(m_cgEnv.modelMatrix,     glmodel);
		if(m_cgEnv.modelViewMatrix) cgGLSetMatrixParameterfc(m_cgEnv.modelViewMatrix, glmodelview);

		RENDERER_ASSERT(context.numBones <= RENDERER_MAX_BONES, "Too many bones.");
		if(context.boneMatrices && context.numBones>0 && context.numBones <= RENDERER_MAX_BONES)
		{
#if 0
			for(PxU32 i=0; i<context.numBones; i++)
			{
				GLfloat glbonematrix[16];
				PxToGL(glbonematrix, context.boneMatrices[i]);
				cgSetMatrixParameterfc(cgGetArrayParameter(m_cgEnv.boneMatrices, i), glbonematrix);
			}
#else
			GLfloat glbonematrix[16*RENDERER_MAX_BONES];
			for(PxU32 i=0; i<context.numBones; i++)
			{
				PxToGL(glbonematrix+(16*i), context.boneMatrices[i]);			
			} 

			{
				PX_PROFILE_ZONE("cgGLSetMatrixParameter",0);
				cgGLSetMatrixParameterArrayfc(m_cgEnv.boneMatrices, 0, context.numBones, glbonematrix);
			}
#endif
		}

#endif
	}

	void OGLRenderer::beginMultiPass(void)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glDepthFunc(GL_EQUAL);
	}

	void OGLRenderer::endMultiPass(void)
	{
		glDisable(GL_BLEND);
		glDepthFunc(GL_LESS);
	}

	void OGLRenderer::beginTransparentMultiPass(void)
	{
		setEnableBlendingOverride(true);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(false);
	}

	void OGLRenderer::endTransparentMultiPass(void)
	{
		setEnableBlendingOverride(false);
		glDisable(GL_BLEND);
		glDepthFunc(GL_LESS);
		glDepthMask(true);
	}

	void OGLRenderer::renderDeferredLight(const RendererLight &/*light*/)
	{
		RENDERER_ASSERT(0, "Not implemented!");
	}

	PxU32 OGLRenderer::convertColor(const RendererColor& color) const
	{
		return OGLRendererVertexBuffer::convertColor(color);
	}


	bool OGLRenderer::isOk(void) const
	{
		bool ok = true;
		ok = m_platform->isContextValid();
		return ok;
	}

	///////////////////////////////////////////////////////////////////////////////

	/*static DWORD gCullMode;
	static DWORD gAlphaBlendEnable;
	static DWORD gSrcBlend;
	static DWORD gDstBlend;
	static DWORD gFillMode;
	static DWORD gZWrite;
	static DWORD gLighting;
	*/

	void OGLRenderer::setupTextRenderStates()
	{
		/*	// PT: save render states. Let's just hope this isn't a pure device, else the Get method won't work
		m_d3dDevice->GetRenderState(D3DRS_CULLMODE, &gCullMode);
		m_d3dDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &gAlphaBlendEnable);
		m_d3dDevice->GetRenderState(D3DRS_SRCBLEND, &gSrcBlend);
		m_d3dDevice->GetRenderState(D3DRS_DESTBLEND, &gDstBlend);
		m_d3dDevice->GetRenderState(D3DRS_FILLMODE, &gFillMode);
		m_d3dDevice->GetRenderState(D3DRS_ZWRITEENABLE, &gZWrite);
		m_d3dDevice->GetRenderState(D3DRS_LIGHTING, &gLighting);

		// PT: setup render states for text rendering
		m_d3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		m_d3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		m_d3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		m_d3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		m_d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		m_d3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);*/

		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDepthMask(false);
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
	}

	void OGLRenderer::resetTextRenderStates()
	{
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		//  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDepthMask(true);
		glEnable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);
	}

	void OGLRenderer::renderTextBuffer(const void* verts, PxU32 nbVerts, const PxU16* indices, PxU32 nbIndices, RendererMaterial* material)
	{
		const int stride = sizeof(TextVertex);
		char* data = (char*)verts;

		glEnableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		// pos
		glVertexPointer(3, GL_FLOAT, stride, data);
		data += 3*sizeof(float);

		// rhw
		PxU32 width, height;
		getWindowSize(width, height);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, width, height, 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		data += sizeof(float);

		// Diffuse color
		glColorPointer(4, GL_UNSIGNED_BYTE, stride, data);
		data += sizeof(int);

		// Texture coordinates
		glTexCoordPointer(2, GL_FLOAT, stride, data);
		data += 2*sizeof(float);

		glDrawRangeElements(GL_TRIANGLES, 0, nbVerts, nbIndices, GL_UNSIGNED_SHORT, indices);

		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	void OGLRenderer::renderLines2D(const void* vertices, PxU32 nbVerts)
	{
		const int stride = sizeof(TextVertex);
		char* data = (char*)vertices;

		glEnableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		// pos
		glVertexPointer(3, GL_FLOAT, stride, data);
		data += 3*sizeof(float);

		// rhw
		PxU32 width, height;
		getWindowSize(width, height);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, width, height, 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		data += sizeof(float);

		// Diffuse color
		glColorPointer(4, GL_UNSIGNED_BYTE, stride, data);
		data += sizeof(int);

		// Texture coordinates
		glTexCoordPointer(2, GL_FLOAT, stride, data);
		data += 2*sizeof(float);

		glDrawArrays(GL_LINE_STRIP, 0, nbVerts);

		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	void OGLRenderer::setupScreenquadRenderStates()
	{
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDepthMask(false);
		glDisable(GL_LIGHTING);
	}
	
	void OGLRenderer::resetScreenquadRenderStates()
	{
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDepthMask(true);
		glEnable(GL_LIGHTING);
	}

#pragma pack(push,2)
	typedef struct
	{
		unsigned short	bfType;
		unsigned int	bfSize;
		unsigned short	bfReserved1;
		unsigned short	bfReserved2;
		unsigned int	bfOffBits;
	} PXT_BITMAPFILEHEADER;
#pragma pack(pop)
	typedef struct _tagBITMAPINFO 
	{
		struct 
		{
			GLuint		biSize;
			GLuint      biWidth;
			GLuint      biHeight;
			GLushort    biPlanes;
			GLushort	biBitCount;
			GLuint      biCompression;
			GLuint      biSizeImage;
			GLuint      biXPelsPerMeter;
			GLuint      biYPelsPerMeter;
			GLuint      biClrUsed;
			GLuint      biClrImportant;
		} 	bmiHeader;
		unsigned int            bmiColors[1];
	} PXT_BITMAPINFO;

	static inline PxU32 screenshotDataOffset()
	{
		return sizeof(PXT_BITMAPFILEHEADER) + sizeof(PXT_BITMAPINFO);
	}
	static inline PxU32 screenshotSize(PxU32 width, PxU32 height)
	{
		return width * height * 4 *sizeof(GLubyte) + screenshotDataOffset();
	}

	bool OGLRenderer::captureScreen(PxU32 &width, PxU32& height, PxU32& sizeInBytes, const void*& screenshotData)
	{
		// Target buffer size
		const PxU32 newBufferSize = screenshotSize(m_displayWidth, m_displayHeight);
		
		// Resize of necessary
		if (newBufferSize != (PxU32)m_displayData.size())
			m_displayData.resize(newBufferSize);

		// Quit if resize failed
		if (newBufferSize != (PxU32)m_displayData.size())
			return false;

		// Write the screen data into the buffer
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glReadPixels(0, 0, m_displayWidth, m_displayHeight, GL_RGBA, GL_UNSIGNED_BYTE, &m_displayData[screenshotDataOffset()]);
		if (GL_NO_ERROR != glGetError())
			return false;

		// Write the header data into the buffer
		PXT_BITMAPFILEHEADER    bitmapFileHeader; 
		PXT_BITMAPINFO          bitmapInfoHeader; 

		bitmapFileHeader.bfType             = 0x4D42; //"BM" 
		bitmapFileHeader.bfReserved1        = 0; 
		bitmapFileHeader.bfReserved2        = 0; 
		bitmapFileHeader.bfOffBits          = sizeof(PXT_BITMAPFILEHEADER) + sizeof(bitmapInfoHeader.bmiHeader); 
		bitmapFileHeader.bfSize             = m_displayWidth * m_displayHeight * 4 + bitmapFileHeader.bfOffBits; 

		bitmapInfoHeader.bmiHeader.biSize             = sizeof(bitmapInfoHeader.bmiHeader); 
		bitmapInfoHeader.bmiHeader.biWidth            = m_displayWidth; 
		bitmapInfoHeader.bmiHeader.biHeight           = m_displayHeight; 
		bitmapInfoHeader.bmiHeader.biPlanes           = 1; 
		bitmapInfoHeader.bmiHeader.biBitCount         = 32; 
		bitmapInfoHeader.bmiHeader.biCompression      = 0 /*BI_RGB*/; 
		bitmapInfoHeader.bmiHeader.biSizeImage        = 0; 
		bitmapInfoHeader.bmiHeader.biXPelsPerMeter    = 0; 
		bitmapInfoHeader.bmiHeader.biYPelsPerMeter    = 0; 
		bitmapInfoHeader.bmiHeader.biClrUsed          = 0; 
		bitmapInfoHeader.bmiHeader.biClrImportant     = 0; 

		memcpy(&m_displayData[0],                        &bitmapFileHeader,           sizeof(bitmapFileHeader));
		memcpy(&m_displayData[sizeof(bitmapFileHeader)], &bitmapInfoHeader.bmiHeader, sizeof(bitmapInfoHeader.bmiHeader));

		// Output the result
		getWindowSize(width, height);
		sizeInBytes    = (PxU32)m_displayData.size();
		screenshotData = &m_displayData[0];

		return true;
	}

#endif // #if defined(RENDERER_ENABLE_OPENGL)
