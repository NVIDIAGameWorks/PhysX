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


#include "GLES2Renderer.h"

#if defined(RENDERER_ENABLE_GLES2)

#include <RendererDesc.h>

#include <RendererVertexBufferDesc.h>
#include "GLES2RendererVertexBuffer.h"

#include <RendererIndexBufferDesc.h>
#include "GLES2RendererIndexBuffer.h"

#include <RendererInstanceBufferDesc.h>
#include "GLES2RendererInstanceBuffer.h"

#include <RendererMeshDesc.h>
#include <RendererMeshContext.h>
#include "GLES2RendererMesh.h"

#include <RendererMaterialDesc.h>
#include <RendererMaterialInstance.h>
#include "GLES2RendererMaterial.h"

#include <RendererLightDesc.h>
#include <RendererDirectionalLightDesc.h>
#include "GLES2RendererDirectionalLight.h"

#include <RendererTexture2DDesc.h>
#include "GLES2RendererTexture2D.h"

#include <RendererProjection.h>

#include <SamplePlatform.h>

#include "foundation/PxMat44.h"

#if defined(RENDERER_ANDROID)
PFNGLMAPBUFFEROESPROC   glMapBufferOES = 0;
PFNGLUNMAPBUFFEROESPROC glUnmapBufferOES = 0;
#endif
bool GLEW_ARB_vertex_buffer_object = true;

void SampleRenderer::PxToGL(GLfloat *gl44, const physx::PxMat44 &mat)
{
	PxMat44 mat44 = mat.getTranspose();
	memcpy(gl44, mat44.front(), 4 * 4 * sizeof (GLfloat));
}

void SampleRenderer::PxToGLColumnMajor(GLfloat *gl44, const physx::PxMat44 &mat44)
{
	memcpy(gl44, mat44.front(), 4 * 4 * sizeof (GLfloat));
}

void SampleRenderer::RenToGL(GLfloat *gl44, const RendererProjection &proj)
{
	proj.getRowMajor44(gl44);
}

void SampleRenderer::RenToGLColumnMajor(GLfloat *gl44, const RendererProjection &proj)
{
	proj.getColumnMajor44(gl44);
}

namespace SampleRenderer
{

GLES2Renderer::GLES2Renderer(const RendererDesc &desc, const char* assetDir) :
	Renderer(DRIVER_GLES2, desc.errorCallback, assetDir), m_platform(SampleFramework::SamplePlatform::platform())
{
	m_displayWidth  = 0;
	m_displayHeight = 0;

	m_viewMatrix	= PxMat44(PxIdentity);
	m_platform->initializeOGLDisplay(desc, m_displayWidth, m_displayHeight);

#if defined RENDERER_ANDROID
	glMapBufferOES =   (PFNGLMAPBUFFEROESPROC)   eglGetProcAddress("glMapBufferOES");
    glUnmapBufferOES = (PFNGLUNMAPBUFFEROESPROC) eglGetProcAddress("glUnmapBufferOES");

	if (!glMapBufferOES || !glUnmapBufferOES)
    {
        GLEW_ARB_vertex_buffer_object = false;
    }
#endif

	checkResize();

    RendererColor& clearColor = getClearColor();
    glClearColor(clearColor.r/255.0f, clearColor.g/255.0f, clearColor.b/255.0f, clearColor.a/255.0f);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
}

RendererTexture3D* GLES2Renderer::createTexture3D(const RendererTexture3DDesc &desc)
{
	// TODO: Implement
	return 0;
}

void GLES2Renderer::setVsync(bool on)
{
	// TODO: Implement
}

void GLES2Renderer::bindFogState(const RendererColor &fogColor, float fogDistance)
{
	m_fogColorAndDistance[0] = fogColor.r;
	m_fogColorAndDistance[1] = fogColor.g;
	m_fogColorAndDistance[2] = fogColor.b;
	m_fogColorAndDistance[3] = fogDistance;
}

GLES2Renderer::~GLES2Renderer(void)
{
	m_platform->freeDisplay();
}

bool GLES2Renderer::begin(void)
{
	return true;
}

void GLES2Renderer::end(void)
{

}
void* GLES2Renderer::getDevice()
{
	/* @dduka, TODO: return something valid here */
	return NULL;
}

bool GLES2Renderer::captureScreen(const char*)
{
	return false;
}

bool GLES2Renderer::captureScreen(PxU32 &, PxU32&, PxU32&, const void*&)
{
	return false;
}

void GLES2Renderer::getWindowSize(PxU32 &width, PxU32 &height) const
{
	RENDERER_ASSERT(m_displayHeight * m_displayWidth > 0, "variables not initialized properly");
	width = m_displayWidth;
	height = m_displayHeight;
}

PxU32 GLES2Renderer::convertColor(const RendererColor& color) const
{
	return GLES2RendererVertexBuffer::convertColor(color);
}

void GLES2Renderer::checkResize(void)
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

void GLES2Renderer::finishRendering()
{
	glFinish();
	glFlush();
}

// clears the offscreen buffers.
void GLES2Renderer::clearBuffers(void)
{
	if(begin())
	{
		glFinish();
		GLbitfield glbuf = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
		//glDepthMask(1);
        RendererColor& clearColor = getClearColor();
		glClearColor(clearColor.r/255.0f, clearColor.g/255.0f, clearColor.b/255.0f, clearColor.a/255.0f);
		glClear(glbuf);
	}
	end();
}

// presents the current color buffer to the screen.
bool GLES2Renderer::swapBuffers(void)
{
	if(begin())
	{
        m_platform->swapBuffers();
		checkResize();
	}
	end();
	return false;
}

RendererVertexBuffer *GLES2Renderer::createVertexBuffer(const RendererVertexBufferDesc &desc)
{
	GLES2RendererVertexBuffer *vb = 0;
	RENDERER_ASSERT(desc.isValid(), "Invalid Vertex Buffer Descriptor.");
	if(desc.isValid())
	{
		vb = new GLES2RendererVertexBuffer(desc);
	}
	return vb;
}

RendererIndexBuffer *GLES2Renderer::createIndexBuffer(const RendererIndexBufferDesc &desc)
{
	GLES2RendererIndexBuffer *ib = 0;
	RENDERER_ASSERT(desc.isValid(), "Invalid Index Buffer Descriptor.");
	if(desc.isValid())
	{
		ib = new GLES2RendererIndexBuffer(desc);
	}
	return ib;
}

RendererInstanceBuffer *GLES2Renderer::createInstanceBuffer(const RendererInstanceBufferDesc &desc)
{
	GLES2RendererInstanceBuffer *ib = 0;
	RENDERER_ASSERT(desc.isValid(), "Invalid Instance Buffer Descriptor.");
	if(desc.isValid())
	{
		ib = new GLES2RendererInstanceBuffer(desc);
	}
	return ib;
}

RendererTexture2D *GLES2Renderer::createTexture2D(const RendererTexture2DDesc &desc)
{
	GLES2RendererTexture2D *texture = 0;
	RENDERER_ASSERT(desc.isValid(), "Invalid Texture 2D Descriptor.");
	if(desc.isValid())
	{
		texture = new GLES2RendererTexture2D(desc);
	}
	return texture;
}

RendererTarget *GLES2Renderer::createTarget(const RendererTargetDesc &desc)
{
	RENDERER_ASSERT(0, "Not Implemented.");
	return 0;
}

RendererMaterial *GLES2Renderer::createMaterial(const RendererMaterialDesc &desc)
{
	GLES2RendererMaterial *mat = 0;
	RENDERER_ASSERT(desc.isValid(), "Invalid Material Descriptor.");
	if(desc.isValid())
	{
		mat = new GLES2RendererMaterial(*this, desc);
	}
	return mat;
}

RendererMesh *GLES2Renderer::createMesh(const RendererMeshDesc &desc)
{
	GLES2RendererMesh *mesh = 0;
	RENDERER_ASSERT(desc.isValid(), "Invalid Mesh Descriptor.");
	if(desc.isValid())
	{
		mesh = new GLES2RendererMesh(*this, desc);
	}
	return mesh;
}

RendererLight *GLES2Renderer::createLight(const RendererLightDesc &desc)
{
	RendererLight *light = 0;
	RENDERER_ASSERT(desc.isValid(), "Invalid Light Descriptor.");
	if(desc.isValid())
	{
		switch(desc.type)
		{
			case RendererLight::TYPE_DIRECTIONAL:
				light = new GLES2RendererDirectionalLight(*static_cast<const RendererDirectionalLightDesc*>(&desc), *this, m_lightColor, m_intensity, m_lightDirection);
				break;
			default:
				break;
		}
	}
	return light;
}


void GLES2Renderer::bindViewProj(const physx::PxMat44 &eye, const RendererProjection &proj)
{
		physx::PxMat44 inveye = eye.inverseRT();

		// load the projection matrix...
		RenToGL(m_glProjectionMatrix, proj);
		RenToGL(m_glProjectionMatrixC, proj);

		// load the view matrix...
		m_viewMatrix = inveye;

		m_eyePosition  =  eye.getPosition();
		m_eyeDirection = -eye.getBasis(2);
}

extern GLES2RendererMaterial* g_hackCurrentMat;
extern RendererMaterialInstance* g_hackCurrentMatInstance;

void GLES2Renderer::bindAmbientState(const RendererColor &ambient)
{
	m_ambientColor[0] = ambient.r/255.0f;
	m_ambientColor[1] = ambient.g/255.0f;
	m_ambientColor[2] = ambient.b/255.0f;
}

void GLES2Renderer::bindDeferredState(void)
{
	RENDERER_ASSERT(0, "Not implemented!");
}

void GLES2Renderer::setCommonRendererParameters()
{
	// if we use MojoShader
	GLES2RendererMaterial::shaderProgram& program = g_hackCurrentMat->m_program[g_hackCurrentMat->m_currentPass];

	if((program.vertexMojoResult || program.fragmentMojoResult) && (g_hackCurrentMatInstance != NULL))
	{
		// eye position
		const RendererMaterial::Variable* var = g_hackCurrentMat->findVariable("g_eyePosition", RendererMaterial::VARIABLE_FLOAT3);
		if(var) g_hackCurrentMatInstance->writeData(*var, &m_eyePosition.x);
		// eye direction
		var = g_hackCurrentMat->findVariable("g_eyeDirection", RendererMaterial::VARIABLE_FLOAT3);
		if(var)
		{
			g_hackCurrentMatInstance->writeData(*var, &m_eyeDirection.x);
		}
		// ambient color
		var = g_hackCurrentMat->findVariable("g_ambientColor", RendererMaterial::VARIABLE_FLOAT3);
		if(var)
		{
			g_hackCurrentMatInstance->writeData(*var, m_ambientColor);
		}
		// light color
		m_lightColor[0] = .8f;
		m_lightColor[1] = .8f;
		m_lightColor[2] = .8f;
		var = g_hackCurrentMat->findVariable("g_lightColor", RendererMaterial::VARIABLE_FLOAT3);
		if(var) g_hackCurrentMatInstance->writeData(*var, m_lightColor);
		// light intensity
		var = g_hackCurrentMat->findVariable("g_lightIntensity", RendererMaterial::VARIABLE_FLOAT);
		if(var)
		{
			g_hackCurrentMatInstance->writeData(*var, &m_intensity);
		}
		// light direction
		var = g_hackCurrentMat->findVariable("g_lightDirection", RendererMaterial::VARIABLE_FLOAT3);
		if(var)
		{
			g_hackCurrentMatInstance->writeData(*var, m_lightDirection);
		}
		// fog color and distance
		var = g_hackCurrentMat->findVariable("g_fogColorAndDistance", RendererMaterial::VARIABLE_FLOAT3);
		if(var) g_hackCurrentMatInstance->writeData(*var, m_fogColorAndDistance);
	}
}

void GLES2Renderer::bindMeshContext(const RendererMeshContext &context)
{
	physx::PxMat44 model = physx::PxMat44(PxIdentity);
	physx::PxMat44 modelView;

	if(context.transform) model = *context.transform;

	GLfloat glmvp[16];
    GLfloat glmodel[16];
	GLfloat glview[16];
	GLfloat glmodelview[16];

	GLES2RendererMaterial::shaderProgram& program = g_hackCurrentMat->m_program[g_hackCurrentMat->m_currentPass];

	// if we don't use MojoShader
    if (program.modelMatrixUniform != -1)
	{
		PxToGLColumnMajor(glmodel, model);
		glUniformMatrix4fv(program.modelMatrixUniform, 1, GL_FALSE, glmodel);
	}
    if (program.viewMatrixUniform != -1)
	{
		PxToGLColumnMajor(glview, m_viewMatrix);
		glUniformMatrix4fv(program.viewMatrixUniform, 1, GL_FALSE, glview);
	}
    if (program.projMatrixUniform != -1)
	{
		glUniformMatrix4fv(program.projMatrixUniform, 1, GL_FALSE, m_glProjectionMatrix);
	}
    if (program.modelViewMatrixUniform != -1)
	{
		modelView = m_viewMatrix * model;
		PxToGLColumnMajor(glmodelview, modelView);
		glUniformMatrix4fv(program.modelViewMatrixUniform, 1, GL_FALSE, glmodelview);
	}
	if (program.modelViewProjMatrixUniform != -1)
	{
		physx::PxMat44 MVP = physx::PxMat44(m_glProjectionMatrix);
		modelView = m_viewMatrix * model;
		PxToGL(glmodelview, modelView);

		MVP = modelView.getTranspose() * MVP;
		memcpy(glmvp, MVP.front(), sizeof(MVP));

		glUniformMatrix4fv(program.modelViewProjMatrixUniform, 1, GL_FALSE, glmvp);
	}

	RENDERER_ASSERT(context.numBones <= RENDERER_MAX_BONES, "Too many bones.");
	if(context.boneMatrices && context.numBones>0 && context.numBones <= RENDERER_MAX_BONES &&
		g_hackCurrentMat->m_program[g_hackCurrentMat->m_currentPass].boneMatricesUniform != -1)
	{
		GLfloat glbonematrix[16 * RENDERER_MAX_BONES];
		for(PxU32 i=0; i<context.numBones; i++)
		{
			PxToGL(glbonematrix+(16*i), context.boneMatrices[i]);
		}
        glUniformMatrix4fv(g_hackCurrentMat->m_program[g_hackCurrentMat->m_currentPass].boneMatricesUniform, context.numBones, GL_FALSE, glbonematrix);
	}

	// if we use MojoShader
	if(program.vertexMojoResult || program.fragmentMojoResult)
	{
		// view matrix
		const RendererMaterial::Variable* var = g_hackCurrentMat->findVariable("g_viewMatrix", RendererMaterial::VARIABLE_FLOAT4x4);
		if(var)
		{
			GLfloat glview[16];
			PxToGL(glview, m_viewMatrix);
			g_hackCurrentMatInstance->writeData(*var, glview);
		}
		// projection matrix
		var = g_hackCurrentMat->findVariable("g_projMatrix", RendererMaterial::VARIABLE_FLOAT4x4);
		if(var)
		{
			g_hackCurrentMatInstance->writeData(*var, m_glProjectionMatrix);
		}
		// boneMatrices matrix
		RENDERER_ASSERT(context.numBones <= RENDERER_MAX_BONES, "Too many bones."); // Not enough cash? CALL CASH BONE!
		var = g_hackCurrentMat->findVariable("g_boneMatrices", RendererMaterial::VARIABLE_FLOAT4x4);
		if(var)
		{
			GLfloat glbonematrices[16 * RENDERER_MAX_BONES];
			if(context.boneMatrices && context.numBones > 0 && context.numBones <= RENDERER_MAX_BONES) {
				for(size_t i = 0; i < context.numBones; ++i) {
					PxToGL(glbonematrices + 16 * i, context.boneMatrices[i]);
				}
				g_hackCurrentMat->bindVariable(g_hackCurrentMat->m_currentPass, *var, glbonematrices);
			}
		}
		// model matrix
		var = g_hackCurrentMat->findVariable("g_modelMatrix", RendererMaterial::VARIABLE_FLOAT4x4);
		if(var)
		{
			PxToGL(glmodel, model);
			g_hackCurrentMat->bindVariable(g_hackCurrentMat->m_currentPass, *var, glmodel);
		}
		// model-view matrix
		var = g_hackCurrentMat->findVariable("g_modelViewMatrix", RendererMaterial::VARIABLE_FLOAT4x4);
		if(var)
		{
			modelView = m_viewMatrix * model;
			PxToGL(glmodelview, modelView);
			g_hackCurrentMat->bindVariable(g_hackCurrentMat->m_currentPass, *var, glmodelview);
		}
		// model-view-projection matrix
		var = g_hackCurrentMat->findVariable("g_MVP", RendererMaterial::VARIABLE_FLOAT4x4);
		if(var)
		{
			physx::PxMat44 MVP = physx::PxMat44(m_glProjectionMatrix);
			modelView = m_viewMatrix * model;
			PxToGL(glmodelview, modelView);

			MVP = modelView.getTranspose() * MVP;
			memcpy(glmvp, MVP.front(), sizeof(MVP));

			g_hackCurrentMat->bindVariable(g_hackCurrentMat->m_currentPass, *var, glmvp);
		}
	} else LOGI("Couldn't bind uniform values! vr = %p, fr = %p, inst = %p, -- %p", g_hackCurrentMat->m_program[g_hackCurrentMat->m_currentPass].vertexMojoResult,
		g_hackCurrentMat->m_program[g_hackCurrentMat->m_currentPass].fragmentMojoResult, g_hackCurrentMatInstance, g_hackCurrentMat);

	g_hackCurrentMat->submitUniforms();

    switch(context.cullMode)
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
}

void GLES2Renderer::beginMultiPass(void)
{
}

void GLES2Renderer::endMultiPass(void)
{
}

void GLES2Renderer::beginTransparentMultiPass(void)
{
}

void GLES2Renderer::endTransparentMultiPass(void)
{
}

void GLES2Renderer::renderDeferredLight(const RendererLight &/*light*/)
{
	RENDERER_ASSERT(0, "Not implemented!");
}

bool GLES2Renderer::isOk(void) const
{
	bool ok = true;
	return ok;
}

void GLES2Renderer::setupTextRenderStates()
{
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(false);
	glEnable(GL_TEXTURE_2D);
}

void GLES2Renderer::resetTextRenderStates()
{
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(true);
	glDisable(GL_TEXTURE_2D);
}

bool GLES2Renderer::initTexter()
{
	Renderer::initTexter();

	PxU32 width, height;
	getWindowSize(width, height);
	const PxU32 MIN_CHARACTER_WIDTH = 8;
	const PxU32 TEXT_MAX_VERTICES = 4 * (width / MIN_CHARACTER_WIDTH);
	const PxU32 TEXT_MAX_INDICES = TEXT_MAX_VERTICES * 1.5f; // we need 1.5 times more indices than vertices to describe text quads

	// Although text buffers (vertex and index buffers) are constantly updating, I received information that STATIC hint set on these buffers
	// results in slightly better performance due to a bug (possibly) in GLES2 drivers on Android/Tegra.
	// initialize vertex buffer -- will be used by all texts
	RendererVertexBufferDesc vbdesc;
	vbdesc.hint = RendererVertexBuffer::HINT_STATIC;
	vbdesc.maxVertices = TEXT_MAX_VERTICES;
	vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_POSITION] = RendererVertexBuffer::FORMAT_FLOAT3;
	vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_TEXCOORD0] = RendererVertexBuffer::FORMAT_FLOAT2;
	vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_COLOR] = RendererVertexBuffer::FORMAT_COLOR_NATIVE;
	m_textVertexBuffer = createVertexBuffer(vbdesc);
	RENDERER_ASSERT(m_textVertexBuffer, "Failed to create Vertex Buffer.");
	// initialize index buffer
	RendererIndexBufferDesc inbdesc;
	inbdesc.hint = RendererIndexBuffer::HINT_STATIC;
	inbdesc.format = RendererIndexBuffer::FORMAT_UINT16;
	inbdesc.maxIndices = TEXT_MAX_INDICES;
	m_textIndexBuffer = createIndexBuffer(inbdesc);
	RENDERER_ASSERT(m_textIndexBuffer, "Failed to create Instance Buffer.");
	RendererMeshDesc meshdesc;
	meshdesc.primitives			= RendererMesh::PRIMITIVE_TRIANGLES;
	meshdesc.vertexBuffers		= &m_textVertexBuffer;
	meshdesc.numVertexBuffers	= 1;
	meshdesc.firstVertex		= 0;
	meshdesc.numVertices		= m_textVertexBuffer->getMaxVertices();
	meshdesc.indexBuffer		= m_textIndexBuffer;
	meshdesc.firstIndex			= 0;
	meshdesc.numIndices			= m_textIndexBuffer->getMaxIndices();
	meshdesc.instanceBuffer		= NULL;
	meshdesc.firstInstance		= 0;
	meshdesc.numInstances		= 0;
	m_textMesh = createMesh(meshdesc);
	RENDERER_ASSERT(m_textMesh, "Failed to create Mesh.");

    //m_textMaterial = NULL;
    //m_textVertexBufferOffset = 0;
    //m_textIndexBufferOffset = 0;
	return true;
}

void GLES2Renderer::renderTextBuffer(const void* verts, PxU32 nbVerts, const PxU16* indices, PxU32 nbIndices, RendererMaterial* material)
{
	PxU32 positionStride = 0, colorStride = 0, texcoordStride = 0;
	PxU8* locked_positions = static_cast<PxU8*>(m_textVertexBuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_POSITION, positionStride));
	PxU8* locked_texcoords = static_cast<PxU8*>(m_textVertexBuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_TEXCOORD0, texcoordStride));
	PxU8* locked_colors = static_cast<PxU8*>(m_textVertexBuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_COLOR, colorStride));

	// copy indices
	PxU16* locked_indices = static_cast<PxU16*>(m_textIndexBuffer->lock());
	memcpy(locked_indices, indices, nbIndices * sizeof(PxU16));
	m_textIndexBuffer->unlock();

	PxU32 windowWidth, windowHeight;
	getWindowSize(windowWidth, windowHeight);
	PxReal windowWidthHalf = windowWidth / 2.0f,
			windowHeightHalf = windowHeight / 2.0f;

	TextVertex* tv = (TextVertex*)verts;
	for(PxU32 i = 0; i < nbVerts; ++i,
									locked_positions += positionStride,
									locked_colors += colorStride,
									locked_texcoords += texcoordStride,
									tv += 1)
	{
		PxVec3 pos = PxVec3(tv->p.x / windowWidthHalf - 1.0f, 1.0f - tv->p.y / windowHeightHalf, 0.0f);
		memcpy(locked_positions, &(pos), sizeof(tv->p));
		memcpy(locked_colors, &(tv->color), sizeof(tv->color));
		memcpy(locked_texcoords, &(tv->u), sizeof(PxReal));
		memcpy(locked_texcoords + sizeof(PxReal), &(tv->v), sizeof(PxReal));
	}
	m_textVertexBuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_COLOR);
	m_textVertexBuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_TEXCOORD0);
	m_textVertexBuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_POSITION);

	// rendering goes here
	m_textMesh->setVertexBufferRange(0, nbVerts);
	m_textMesh->setIndexBufferRange(0, nbIndices);
	m_textMesh->bind();
	m_textMesh->render(m_textMaterial);
	m_textMesh->unbind();
}

void GLES2Renderer::renderLines2D(const void* vertices, PxU32 nbVerts)
{
	// @dduka, TODO: glEnableClientState is not available on GL ES 2.0
	/*
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
	*/
}

void GLES2Renderer::setupScreenquadRenderStates()
{
	//glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDepthMask(false);
}

void GLES2Renderer::resetScreenquadRenderStates()
{
	//glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDepthMask(true);
}

}
#endif // #if defined(RENDERER_ENABLE_GLES2)
