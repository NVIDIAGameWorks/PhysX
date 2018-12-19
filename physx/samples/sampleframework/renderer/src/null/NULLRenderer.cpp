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

