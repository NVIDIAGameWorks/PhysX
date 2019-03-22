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
#include "NULLRenderer.h"

#include <RendererDesc.h>

#include "NULLRendererDirectionalLight.h"
#include "NULLRendererIndexBuffer.h"
#include "NULLRendererInstanceBuffer.h"
#include "NULLRendererMaterial.h"
#include "NULLRendererMesh.h"
#include "NULLRendererSpotLight.h"
#include "NULLRendererTexture2D.h"
#include "NULLRendererVertexBuffer.h"

#include "RendererVertexBufferDesc.h"
#include "RendererIndexBufferDesc.h"
#include "RendererInstanceBufferDesc.h"
#include "RendererTexture2DDesc.h"
#include "RendererMaterialDesc.h"
#include "RendererDirectionalLightDesc.h"
#include "RendererSpotLightDesc.h"
#include "RendererMeshDesc.h"

using namespace SampleRenderer;

NullRenderer::NullRenderer(const RendererDesc &desc, const char* assetDir) :
Renderer(DRIVER_NULL, desc.errorCallback, assetDir)
{

}

NullRenderer::~NullRenderer(void)
{
}

void NullRenderer::finishRendering()
{
}

bool NullRenderer::captureScreen(const char* filename)
{
	return false;
}

// clears the offscreen buffers.
void NullRenderer::clearBuffers(void)
{
}

// presents the current color buffer to the screen.
// returns true on device reset and if buffers need to be rewritten.
bool NullRenderer::swapBuffers(void)
{
	return false;
}

RendererVertexBuffer *NullRenderer::createVertexBuffer(const RendererVertexBufferDesc &desc)
{
	NullRendererVertexBuffer *vb = 0;
	RENDERER_ASSERT(desc.isValid(), "Invalid Vertex Buffer Descriptor.");
	if(desc.isValid())
	{
		vb = new NullRendererVertexBuffer(desc);
	}
	return vb;
}

RendererIndexBuffer *NullRenderer::createIndexBuffer(const RendererIndexBufferDesc &desc)
{	
	NullRendererIndexBuffer *ib = 0;
	RENDERER_ASSERT(desc.isValid(), "Invalid Index Buffer Descriptor.");
	if(desc.isValid())
	{
		ib = new NullRendererIndexBuffer(desc);
	}
	return ib;
}

void* NullRenderer::getDevice()
{
	return NULL;
}

void NullRenderer::getWindowSize(PxU32 &width, PxU32 &height) const
{
	width = 640;
	height = 480;
}

void NullRenderer::renderLines2D(const void* vertices, PxU32 nbVerts)
{

}

RendererInstanceBuffer *NullRenderer::createInstanceBuffer(const RendererInstanceBufferDesc &desc)
{
	NullRendererInstanceBuffer *ib = 0;
	RENDERER_ASSERT(desc.isValid(), "Invalid Instance Buffer Descriptor.");
	if(desc.isValid())
	{
		ib = new NullRendererInstanceBuffer(desc);
	}
	return ib;
}

RendererTexture2D *NullRenderer::createTexture2D(const RendererTexture2DDesc &desc)
{
	RendererTexture2D *texture = 0;
	RENDERER_ASSERT(desc.isValid(), "Invalid Texture 2D Descriptor.");
	if(desc.isValid())
	{
		texture = new NullRendererTexture2D(desc);
	}
	return texture;
}

RendererTexture3D *NullRenderer::createTexture3D(const RendererTexture3DDesc &desc)
{
	RendererTexture3D *texture = 0;
	RENDERER_ASSERT(desc.isValid(), "Invalid Texture 3D Descriptor.");
	if(desc.isValid())
	{
		// TODO: implement
		texture = NULL;
	}
	return texture;
}

RendererTarget *NullRenderer::createTarget(const RendererTargetDesc &desc)
{
	RENDERER_ASSERT(0, "Not Implemented.");
	return 0;
}

RendererMaterial *NullRenderer::createMaterial(const RendererMaterialDesc &desc)
{
	NullRendererMaterial *mat = 0;
	RENDERER_ASSERT(desc.isValid(), "Invalid Material Descriptor.");
	if(desc.isValid())
	{
		mat = new NullRendererMaterial(*this, desc);
	}	
	return mat;
}

RendererMesh *NullRenderer::createMesh(const RendererMeshDesc &desc)
{
	NullRendererMesh *mesh = 0;
	RENDERER_ASSERT(desc.isValid(), "Invalid Mesh Descriptor.");
	if(desc.isValid())
	{
		mesh = new NullRendererMesh(*this,desc);
	}
	return mesh;
}

RendererLight *NullRenderer::createLight(const RendererLightDesc &desc)
{
	RendererLight *light = 0;
	RENDERER_ASSERT(desc.isValid(), "Invalid Light Descriptor.");
	if(desc.isValid())
	{
		switch(desc.type)
		{
		case RendererLight::TYPE_DIRECTIONAL:
			light = new NullRendererDirectionalLight(*static_cast<const RendererDirectionalLightDesc*>(&desc));
			break;
		case RendererLight::TYPE_SPOT:
			{
				light = new NullRendererSpotLight(*static_cast<const RendererSpotLightDesc*>(&desc));
			}
			break;
		default:
			break;
		}
		
	}
	return light;
}

void NullRenderer::bindViewProj(const physx::PxMat44 &eye, const RendererProjection &proj)
{

}

void NullRenderer::bindFogState(const RendererColor &fogColor, float fogDistance)
{
}

void NullRenderer::bindAmbientState(const RendererColor &ambientColor)
{

}

void NullRenderer::bindDeferredState(void)
{
	RENDERER_ASSERT(0, "Not implemented!");
}

void NullRenderer::bindMeshContext(const RendererMeshContext &context)
{

}

void NullRenderer::beginMultiPass(void)
{
}

void NullRenderer::endMultiPass(void)
{
}

void NullRenderer::renderDeferredLight(const RendererLight &/*light*/)
{
	RENDERER_ASSERT(0, "Not implemented!");
}

PxU32 NullRenderer::convertColor(const RendererColor& color) const
{
	return color.a << 24 | color.b << 16 | color.g << 8 | color.r;
}


bool NullRenderer::isOk(void) const
{
	return true;
}

///////////////////////////////////////////////////////////////////////////////

void NullRenderer::setupTextRenderStates()
{
}

void NullRenderer::resetTextRenderStates()
{

}

bool NullRenderer::initTexter()
{

	return true;
}

void NullRenderer::renderTextBuffer(const void* verts, PxU32 nbVerts, const PxU16* indices, PxU32 nbIndices, RendererMaterial* material)
{		

}

void NullRenderer::renderLines2D(const void* vertices, PxU32 nbPassedVerts, RendererMaterial* material)
{	

}

void NullRenderer::setupScreenquadRenderStates()
{

}

void NullRenderer::resetScreenquadRenderStates()
{

}

