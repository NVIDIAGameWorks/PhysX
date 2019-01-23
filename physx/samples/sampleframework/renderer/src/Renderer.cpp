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
#include "RendererFoundation.h"
#include <Renderer.h>
#include <RendererDesc.h>
#include "ogl/OGLRenderer.h"
#include "d3d9/D3D9Renderer.h"
#include "d3d11/D3D11Renderer.h"
#include "null/NULLRenderer.h"
#include <RendererMesh.h>
#include <RendererMeshContext.h>

#include <RendererMaterial.h>
#include <RendererMaterialDesc.h>
#include <RendererMaterialInstance.h>
#include <RendererProjection.h>

#include <RendererTarget.h>
#include <RendererTexture2D.h>
#include <RendererTexture2DDesc.h>

#include <RendererVertexBuffer.h>
#include <RendererVertexBufferDesc.h>
#include <RendererMesh.h>
#include <RendererMeshDesc.h>

#include <RendererLight.h>
#include "PsUtilities.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "PsUtilities.h"
#include "extensions/PxDefaultStreams.h"


// for PsString.h
#include <PsString.h>
#include <PxTkFile.h>

namespace Ps = physx::shdfnd;

using namespace SampleRenderer;

Renderer *Renderer::createRenderer(const RendererDesc &desc, const char* assetDir, bool enableMaterialCaching)
{
	Renderer *renderer = 0;
	const bool valid = desc.isValid();
	if(valid)
	{
		switch(desc.driver)
		{
		case DRIVER_OPENGL:
#if defined(RENDERER_ENABLE_OPENGL)
			renderer = new OGLRenderer(desc, assetDir);
#endif
			break;

		case DRIVER_DIRECT3D9:
#if defined(RENDERER_ENABLE_DIRECT3D9)
			renderer = new D3D9Renderer(desc, assetDir);
#endif
			break;

		case DRIVER_DIRECT3D11:
#if defined(RENDERER_ENABLE_DIRECT3D11)
			renderer = new D3D11Renderer(desc, assetDir);
#endif
			break;

		case DRIVER_NULL:
			renderer = new NullRenderer(desc,assetDir);
			break;
		}

		if(renderer)
		{
			renderer->setEnableMaterialCaching(enableMaterialCaching);
		}
	}
	if(renderer && !renderer->isOk())
	{
		renderer->release();
		renderer = 0;
	}
	RENDERER_ASSERT(renderer, "Failed to create renderer!");
	return renderer;
}

const char *Renderer::getDriverTypeName(DriverType type)
{
	const char *name = 0;
	switch(type)
	{
	case DRIVER_OPENGL:     name = "OpenGL";			break;
	case DRIVER_DIRECT3D9:  name = "Direct3D9";			break;
	case DRIVER_DIRECT3D11: name = "Direct3D11"; 		break;
	case DRIVER_NULL:		name = "NullRenderer";	 	break;
	}
	RENDERER_ASSERT(name, "Unable to find Name String for Renderer Driver Type.");
	return name;
}


Renderer::Renderer(DriverType driver, PxErrorCallback* errorCallback, const char* assetDir) :
	m_driver							(driver),
	m_errorCallback						(errorCallback),
	m_textMaterial						(NULL),
	m_textMaterialInstance				(NULL),
	m_screenquadOpaqueMaterial			(NULL),
	m_screenquadOpaqueMaterialInstance	(NULL),
	m_screenquadAlphaMaterial			(NULL),
	m_screenquadAlphaMaterialInstance	(NULL),
	m_useShadersForTextRendering		(true),
	m_assetDir							(assetDir),
	m_cacheDir							(NULL),
	m_enableTessellation				(false),
	m_enableWireframe					(false),
	m_enableBlendingOverride			(false),
	m_enableBlendingCull				(false),
	mEnableMaterialCaching				(true)
{
	m_pixelCenterOffset = 0;
	setAmbientColor(RendererColor(64,64,64, 255));
	setFog(RendererColor(0,0,10,255), 20000.0f);
	setClearColor(RendererColor(133,153,181,255));
	Ps::strlcpy(m_deviceName, sizeof(m_deviceName), "UNKNOWN");
}

Renderer::~Renderer(void)
{
	PX_ASSERT(!m_screenquadOpaqueMaterial);
	PX_ASSERT(!m_screenquadOpaqueMaterialInstance);
	PX_ASSERT(!m_screenquadAlphaMaterial);
	PX_ASSERT(!m_screenquadAlphaMaterialInstance);

	PX_ASSERT(!m_textMaterial);
	PX_ASSERT(!m_textMaterialInstance);

	for (tMaterialCache::iterator it = m_materialCache.begin(); it != m_materialCache.end(); ++it)
	{
		::free((void*)it->first.vertexShaderPath);
		::free((void*)it->first.fragmentShaderPath);
		::free((void*)it->first.geometryShaderPath);
		::free((void*)it->first.domainShaderPath);
		::free((void*)it->first.hullShaderPath);

		PX_ASSERT(it->second == NULL);
	}
}


void Renderer::release(void)
{
	closeScreenquad();
	delete this;
}

// Create a 2D or 3D texture, depending on texture depth
RendererTexture* SampleRenderer::Renderer::createTexture(const RendererTextureDesc &desc)
{
	return desc.depth > 1 ? createTexture3D(desc) : createTexture2D(desc);
}

// get the driver type for this renderer.
Renderer::DriverType Renderer::getDriverType(void) const
{
	return m_driver;
}

// get the offset to the center of a pixel relative to the size of a pixel (so either 0 or 0.5).
PxF32 Renderer::getPixelCenterOffset(void) const
{
	return m_pixelCenterOffset;
}

// get the name of the hardware device.
const char *Renderer::getDeviceName(void) const
{
	return m_deviceName;
}

const char *Renderer::getAssetDir() 
{ 
	return m_assetDir.c_str(); 
}

void Renderer::setAssetDir(const char * assetDir) 
{ 
	m_assetDir = assetDir; 
}

// adds a mesh to the render queue.
void Renderer::queueMeshForRender(RendererMeshContext &mesh)
{
	RENDERER_ASSERT( mesh.isValid(),  "Mesh Context is invalid.");
	if(mesh.isValid())
	{
		if (mesh.screenSpace)
		{
			m_screenSpaceMeshes.push_back(mesh);
		}
		else
		{
			switch (mesh.material->getType())
			{
			case  RendererMaterial::TYPE_LIT:
				if (mesh.material->getBlending())
					m_visibleLitTransparentMeshes.push_back(mesh);
				else
					m_visibleLitMeshes.push_back(mesh);
				break;
			default: //case RendererMaterial::TYPE_UNLIT:
				m_visibleUnlitMeshes.push_back(mesh);
				//	break;
			}
		}
	}
}

struct RenderMeshContextHasMesh
{
	RenderMeshContextHasMesh(const RendererMesh* mesh) : mMesh(mesh) { }

	bool operator()(const RendererMeshContext& meshContext) 
	{
		return meshContext.mesh == mMesh;
	}

	const RendererMesh* mMesh;
};

void SampleRenderer::Renderer::removeMeshFromRenderQueue(RendererMesh& mesh)
{
	MeshVector* meshContextQueues[] = { &m_visibleLitMeshes, 
	                                    &m_visibleUnlitMeshes,
	                                    &m_screenSpaceMeshes,
	                                    &m_visibleLitTransparentMeshes };

	// All queues must be searched, as a single mesh can be used in multiple contexts
	for (PxU32 i = 0; i < PX_ARRAY_SIZE(meshContextQueues); ++i)
	{
		MeshVector& meshContextQueue = *meshContextQueues[i];
		meshContextQueue.erase(std::remove_if(meshContextQueue.begin(), 
		                                      meshContextQueue.end(), 
		                                      RenderMeshContextHasMesh(&mesh)), 
		                       meshContextQueue.end());
	}
}

// adds a light to the render queue.
void Renderer::queueLightForRender(RendererLight &light)
{
	RENDERER_ASSERT(!light.isLocked(), "Light is already locked to a Renderer.");
	if(!light.isLocked())
	{
		light.m_renderer = this;
		m_visibleLights.push_back(&light);
	}
}

void SampleRenderer::Renderer::removeLightFromRenderQueue(RendererLight &light)
{
	m_visibleLights.erase(std::remove(m_visibleLights.begin(), m_visibleLights.end(), &light));
}

// renders the current scene to the offscreen buffers. empties the render queue when done.
void Renderer::render(const physx::PxMat44 &eye, const RendererProjection &proj, RendererTarget *target, bool depthOnly)
{
	PX_PROFILE_ZONE("Renderer_render",0);
	const PxU32 numLights = (PxU32)m_visibleLights.size();
	if(target)
	{
		target->bind();
	}
	// TODO: Sort meshes by material..
	ScopedRender renderSection(*this);
	if(renderSection)
	{
		if(!depthOnly)
		{
			// YOU CAN PASS THE PROJECTION MATIX RIGHT INTO THIS FUNCTION!
			// TODO: Get rid of this.
			if (m_screenSpaceMeshes.size())
			{
				physx::PxMat44 id = physx::PxMat44(PxIdentity);
				bindViewProj(id, RendererProjection(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f));	//TODO: pass screen space matrices
				renderMeshes(m_screenSpaceMeshes, RendererMaterial::PASS_UNLIT);	//render screen space stuff first so stuff we occlude doesn't waste time on shading.
			}
		}

		sortMeshes(eye);

		if(depthOnly)
		{
			PX_PROFILE_ZONE("Renderer_render_depthOnly",0);
			bindAmbientState(RendererColor(0,0,0,255));
			bindViewProj(eye, proj);
			renderMeshes(m_visibleLitMeshes,   RendererMaterial::PASS_DEPTH);
			renderMeshes(m_visibleUnlitMeshes, RendererMaterial::PASS_DEPTH);
		}
		else  if(numLights > RENDERER_DEFERRED_THRESHOLD)
		{
			PX_PROFILE_ZONE("Renderer_render_deferred",0);
			bindDeferredState();
			bindViewProj(eye, proj);
			renderMeshes(m_visibleLitMeshes,   RendererMaterial::PASS_UNLIT);
			renderMeshes(m_visibleUnlitMeshes, RendererMaterial::PASS_UNLIT);
			renderMeshes(m_visibleLitTransparentMeshes,   RendererMaterial::PASS_UNLIT);
			renderDeferredLights();
		}
		else if(numLights > 0)
		{
			PX_PROFILE_ZONE("Renderer_render_lit",0);
			bindAmbientState(m_ambientColor);
			bindFogState(m_fogColor, m_fogDistance);
			bindViewProj(eye, proj);

#if RENDERER_ENABLE_SINGLE_PASS_LIGHTING
			for(PxU32 i=0; i<numLights; i++)
			{
				m_visibleLights[i]->bind(i);
			}
			renderMeshes(m_visibleLitMeshes, m_visibleLights[0]->getPass());
			for(PxU32 i=0; i<numLights; i++)
			{
				m_visibleLights[i]->m_renderer = NULL;
			}
#else
			RendererLight &light0 = *m_visibleLights[0];
			light0.bind();
			renderMeshes(m_visibleLitMeshes, light0.getPass());

			bindAmbientState(RendererColor(0,0,0,255));
			beginMultiPass();
			for(PxU32 i=1; i<numLights; i++)
			{
				RendererLight &light = *m_visibleLights[i];
				light.bind();
				renderMeshes(m_visibleLitMeshes, light.getPass());
			}
			endMultiPass();
#endif
			renderMeshes(m_visibleUnlitMeshes, RendererMaterial::PASS_UNLIT);

			///////////////////////////////////////////////////////////////////////////
			if (m_visibleLitTransparentMeshes.size() > 0) 
			{
				light0.bind();
				renderMeshes(m_visibleLitTransparentMeshes, light0.getPass());

				bindAmbientState(RendererColor(0,0,0,255));
				beginTransparentMultiPass();
				for(PxU32 i=1; i<numLights; i++)
				{
					RendererLight &light = *m_visibleLights[i];
					light.bind();
					renderMeshes(m_visibleLitTransparentMeshes, light.getPass());
				}
				endTransparentMultiPass();
			}
			///////////////////////////////////////////////////////////////////////////

			for (physx::PxU32 i = 0; i < numLights; ++i)
				m_visibleLights[i]->m_renderer = 0;
		}
		else
		{
			PX_PROFILE_ZONE("Renderer_render_unlit",0);
			bindAmbientState(RendererColor(0,0,0,255));
			bindViewProj(eye, proj);
			renderMeshes(m_visibleLitMeshes,   RendererMaterial::PASS_UNLIT);
			renderMeshes(m_visibleUnlitMeshes, RendererMaterial::PASS_UNLIT);
			renderMeshes(m_visibleLitTransparentMeshes, RendererMaterial::PASS_UNLIT);
		}
	}
	{
		PX_PROFILE_ZONE("Cleanup", 0);
		if (target) target->unbind();
		m_visibleLitMeshes.clear();
		m_visibleUnlitMeshes.clear();
		m_visibleLitTransparentMeshes.clear();
		m_screenSpaceMeshes.clear();
		m_visibleLights.clear();
	}
}

// sets the ambient lighting color.
void Renderer::setAmbientColor(const RendererColor &ambientColor)
{
	m_ambientColor   = ambientColor;
	m_ambientColor.a = 255;
}

void Renderer::setFog(const RendererColor &fogColor, float fogDistance)
{
	m_fogColor    = fogColor;
	m_fogDistance = fogDistance;
}

void Renderer::setClearColor(const RendererColor &clearColor)
{
	m_clearColor   = clearColor;
	m_clearColor.a = 255;
}

Renderer::TessellationParams::TessellationParams()
{
	setDefault();
}

void Renderer::TessellationParams::setDefault()
{
	tessFactor                = PxVec4(6.f, 6.f, 3.f, 100.f);
	tessMinMaxDistance[0]     = 5.0f; 
	tessMinMaxDistance[1]     = 50;
	tessHeightScaleAndBias[0] = 1.0f;
	tessHeightScaleAndBias[1] = 0.5f;
	tessUVScale[0]            = 1.0f;
	tessUVScale[1]            = 1.0f;
}

std::string SampleRenderer::Renderer::TessellationParams::toString()
{
	std::stringstream ss;
	ss << "TessParams =  "      << std::endl;
	ss << "\tTessFactor:      " << tessFactor.x << " " <<  tessFactor.y << " " << tessFactor.z << " " << tessFactor.w << std::endl;
	ss << "\tTessMinMax:      " << tessMinMaxDistance[0] << " " << tessMinMaxDistance[1] << std::endl;
	ss << "\tTessHeightScale: " << tessHeightScaleAndBias[0] << " " << tessHeightScaleAndBias[1] << std::endl;
	ss << "\tTessUVScale:     " << tessUVScale[0] << " " << tessUVScale[1] << std::endl;
	return ss.str();
}

void Renderer::setTessellationParams(const TessellationParams& params)
{
	m_tessellationParams = params;
}

static PX_INLINE PxU32 createPixel(PxU8 r, PxU8 g, PxU8 b)
{
	// G = bits 0-7
	// B = bits 8-15
	// R = bits 16-23
	return (r << 16) | (g << 8) | b;
}

void Renderer::formatScreenshot(PxU32 width, PxU32 height, PxU32 sizeInBytes, int rPosition, int gPosition, int bPosition, bool bFlipY, void* screenshotData)
{
	if (width <= 0 || height <= 0 || sizeInBytes <= 0)
		return;

	PxU32 srcByteStride = sizeInBytes / (width * height);
	RENDERER_ASSERT((srcByteStride > 0) && (srcByteStride <= sizeof(PxU32)), "Invalid image format.");
	if (srcByteStride <= 0 || srcByteStride > sizeof(PxU32))
		return;

	PxU32* pSrc    = static_cast<PxU32*>(screenshotData);
	PxU8* pSrcByte = static_cast<PxU8*>(screenshotData);
	if (bFlipY)
	{
		for (PxU32 i = 0; i < height/2; ++i)
		{
			PxU32* pSrc2     = static_cast<PxU32*>(screenshotData) +                 width * (height - i - 1);
			PxU8*  pSrc2Byte = static_cast<PxU8*>(screenshotData)  + srcByteStride * width * (height - i - 1);
			for (PxU32 j = 0; j < width; ++j, pSrcByte+=srcByteStride, pSrc2Byte+=srcByteStride, ++pSrc, ++pSrc2)
			{
				PxU32 temp = createPixel(pSrcByte[rPosition], pSrcByte[gPosition], pSrcByte[bPosition]);
				*pSrc      = createPixel(pSrc2Byte[rPosition], pSrc2Byte[gPosition], pSrc2Byte[bPosition]);
				*pSrc2     = temp;
			}
		}
		// Format the middle scanline
		if (height % 2 == 1)
		{
			for (PxU32 j = 0; j < width; ++j, pSrcByte+=srcByteStride,++pSrc)
			{
				*pSrc = createPixel(pSrcByte[rPosition], pSrcByte[gPosition], pSrcByte[bPosition]);
			}
		}
	}
	else
	{
		for (PxU32 i = 0; i < height; ++i)
		{
			for (PxU32 j = 0; j < width; ++j, pSrcByte+=srcByteStride,++pSrc)
			{
				*pSrc = createPixel(pSrcByte[rPosition], pSrcByte[gPosition], pSrcByte[bPosition]);
			}
		}
	}
}

void Renderer::renderMeshes(std::vector<RendererMeshContext>& meshes, RendererMaterial::Pass pass)
{
	PX_PROFILE_ZONE("Renderer_renderMeshes",0);

	RendererMaterial         *lastMaterial         = 0;
	RendererMaterialInstance *lastMaterialInstance = 0;
	const RendererMesh       *lastMesh             = 0;

	const PxU32 numMeshes = (PxU32)meshes.size();
	for(PxU32 i=0; i<numMeshes; i++)
	{
		RendererMeshContext& context = meshes[i];

		if(!context.mesh->willRender())
		{
			continue;
		}

		// the mesh context should be bound before the material, this is 
		// necessary because the materials read state from the graphics device
		// for instance to determine if two-sided lighting should be enabled
		bindMeshContext(context);
		
		bool instanced = context.mesh->getInstanceBuffer()?true:false;

		if(context.materialInstance && context.materialInstance != lastMaterialInstance)
		{
			if(lastMaterial) lastMaterial->unbind();
			lastMaterialInstance =  context.materialInstance;
			lastMaterial         = &context.materialInstance->getMaterial();
			lastMaterial->bind(pass, lastMaterialInstance, instanced);
		}
		else if(context.material != lastMaterial)
		{
			if(lastMaterial) lastMaterial->unbind();
			lastMaterialInstance = 0;
			lastMaterial         = context.material;
			lastMaterial->bind(pass, lastMaterialInstance, instanced);
		}

		if(lastMaterial) lastMaterial->bindMeshState(instanced);
		if(context.mesh != lastMesh)
		{
			if(lastMesh) lastMesh->unbind();
			lastMesh = context.mesh;
			if(lastMesh) lastMesh->bind();
		}
		if(lastMesh) context.mesh->render(context.material);
	}
	if(lastMesh)     lastMesh->unbind();
	if(lastMaterial) lastMaterial->unbind();
}

struct CompareMeshCameraDist
{
	CompareMeshCameraDist(const physx::PxVec3& t_) : t(t_) { }
	bool operator()(const RendererMeshContext& c1, const RendererMeshContext& c2) const
	{
		// Equivalent, return false
		if (NULL == c1.transform && NULL == c2.transform)
			return false;
		// NULL transforms will be rendered first
		if (NULL == c1.transform)
			return true;
		if (NULL == c2.transform)
			return false;
		// A larger magnitude means further away, which should be ordered first
		// TODO: Use bones
		return (c1.transform->getPosition() - t).magnitudeSquared() > (c2.transform->getPosition() - t).magnitudeSquared();
	}
	physx::PxVec3 t;
};

void Renderer::sortMeshes(const physx::PxMat44& eye)
{
	CompareMeshCameraDist meshComp(eye.getPosition());
	std::sort(m_visibleLitTransparentMeshes.begin(), m_visibleLitTransparentMeshes.end(), meshComp);
}

void Renderer::renderDeferredLights(void)
{
	PX_PROFILE_ZONE("Renderer_renderDeferredLights",0);
	const PxU32 numLights = (PxU32)m_visibleLights.size();
	for(PxU32 i=0; i<numLights; i++)
	{
		renderDeferredLight(*m_visibleLights[i]);
	}
}

bool Renderer::CompareRenderMaterialDesc::operator()(const RendererMaterialDesc& desc1, const RendererMaterialDesc& desc2) const
{
	if (desc1.type != desc2.type)
		return desc1.type < desc2.type;

	if (desc1.alphaTestFunc != desc2.alphaTestFunc)
		return desc1.alphaTestFunc < desc2.alphaTestFunc;

	if (desc1.alphaTestRef != desc2.alphaTestRef)
		return desc1.alphaTestRef < desc2.alphaTestRef;

	if (desc1.blending != desc2.blending)
		return desc1.blending < desc2.blending;

	if (desc1.srcBlendFunc != desc2.srcBlendFunc)
		return desc1.srcBlendFunc < desc2.srcBlendFunc;

	if (desc1.dstBlendFunc != desc2.dstBlendFunc)
		return desc1.dstBlendFunc < desc2.dstBlendFunc;

	int result = Ps::stricmp(desc1.vertexShaderPath, desc2.vertexShaderPath);
	if (result != 0)
		return result < 0;

	result = Ps::stricmp(desc1.fragmentShaderPath, desc2.fragmentShaderPath);
	if (result != 0)
		return result < 0;

	// Geometry, hull and domain shaders are optional, so only check when present
	//    * stricmp only works properly for well defined pointers, so only test when both present
	//    * if one material has a particular shader when the other material does not, the one WITH should be first
	if (desc1.geometryShaderPath || desc2.geometryShaderPath)
	{
		if (desc1.geometryShaderPath && desc2.geometryShaderPath)
			result = Ps::stricmp(desc1.geometryShaderPath, desc2.geometryShaderPath);
		else
			result = desc1.geometryShaderPath ? -1 : 1;
	}

	if (0 == result && (desc1.hullShaderPath || desc2.hullShaderPath))
	{
		if (desc1.hullShaderPath && desc2.hullShaderPath)
			result = Ps::stricmp(desc1.hullShaderPath, desc2.hullShaderPath);
		else
			result = desc1.hullShaderPath ? -1 : 1;
	}
	
	if (0 == result && (desc1.domainShaderPath || desc2.domainShaderPath))
	{
		if (desc1.domainShaderPath && desc2.domainShaderPath)
			result = Ps::stricmp(desc1.domainShaderPath, desc2.domainShaderPath);
		else
			result = desc1.domainShaderPath ? -1 : 1;
	}

	return result < 0;
}

RendererMaterial* Renderer::hasMaterialAlready(const RendererMaterialDesc& desc)
{
	if (mEnableMaterialCaching)
	{
		tMaterialCache::iterator it = m_materialCache.find(desc);

		if (it != m_materialCache.end())
		{
			PX_ASSERT(it->second != NULL);
			it->second->incRefCount();
			return it->second;
		}
	}

	return NULL;
}



static const char* local_strdup(const char* input)
{
	if (input == NULL)
	{
		return NULL;
	}

	unsigned int strLen = (unsigned int)strlen(input) + 1;
	char* retStr = (char*)::malloc(strLen);

	PX_ASSERT(retStr);

	if (NULL != retStr)
	{
#if PX_WINDOWS
		strcpy_s(retStr, strLen, input);
#else
		strncpy(retStr, input, strLen);
#endif
	}
	return retStr;
}



void Renderer::registerMaterial(const RendererMaterialDesc& desc, RendererMaterial* mat)
{
	if (mEnableMaterialCaching)
	{
		tMaterialCache::iterator it = m_materialCache.find(desc);

		if (it == m_materialCache.end())
		{
			RendererMaterialDesc descCopy = desc;
			descCopy.vertexShaderPath = local_strdup(desc.vertexShaderPath);
			descCopy.fragmentShaderPath = local_strdup(desc.fragmentShaderPath);
			descCopy.geometryShaderPath = local_strdup(desc.geometryShaderPath);
			descCopy.domainShaderPath = local_strdup(desc.domainShaderPath);
			descCopy.hullShaderPath = local_strdup(desc.hullShaderPath);

			PX_ASSERT(mat->m_refCount == 1);
			m_materialCache[descCopy] = mat;
		}
		else
		{
			PX_ASSERT(it->second == NULL);
			it->second = mat;
		}
	}
}



void Renderer::releaseAllMaterials()
{
	for (tMaterialCache::iterator it = m_materialCache.begin(); it != m_materialCache.end(); ++it)
	{
		PX_ASSERT(it->second != NULL);
		delete it->second;

		it->second = NULL;
	}
}



///////////////////////////////////////////////////////////////////////////////


#include "RendererMemoryMacros.h"
#include "RendererMaterialDesc.h"
#include "RendererTexture2DDesc.h"

// PT: text stuff from ICE. Adapted quickly, should be cleaned up when we have some time.

struct FntInfo
{
	PxReal		u0;
	PxReal		v0;
	PxReal		u1;
	PxReal		v1;
	PxU32		dx;
	PxU32		dy;
};

class FntData
{
public:
	FntData();
	~FntData();

	void			Reset();
	PxU32			ComputeSize(const char* text, PxReal& width, PxReal& height, PxReal scale, bool forceFixWidthNumbers) const;

	bool			Load(Renderer& renderer, const char* filename);

	PX_FORCE_INLINE	PxU32			GetNbFnts()	const	{ return mNbFnts;	}
	PX_FORCE_INLINE	const FntInfo*	GetFnts()	const	{ return mFnts;		}
	PX_FORCE_INLINE	PxU32			GetMaxDx()	const	{ return mMaxDx;	}
	PX_FORCE_INLINE	PxU32			GetMaxDy()	const	{ return mMaxDy;	}
	PX_FORCE_INLINE	const PxU8*		GetXRef()	const	{ return mXRef;		}
	PX_FORCE_INLINE PxU32			GetMaxDxNumbers()	const	{ return mMaxDxNumbers;	}
	PX_FORCE_INLINE bool			IsFixWidthCharacter(unsigned char c) const { return mFixWidthCharacters[c]; }

private:
	PxU32			mNbFnts;
	FntInfo*		mFnts;
	PxU32			mMaxDx, mMaxDy;
	PxU8			mXRef[256];
	bool			mFixWidthCharacters[256];
	PxU32			mMaxDxNumbers;
public:
	RendererTexture2D*			mTexture;
};

FntData::FntData()
{
	mNbFnts	= 0;
	mFnts	= NULL;
	mMaxDx	= 0;
	mMaxDy	= 0;
	memset(mXRef, 0, 256*sizeof(PxU8));
	memset(mFixWidthCharacters, 0, 256 * sizeof(bool));
	for (int i = '0'; i <= '9'; i++)
	{
		mFixWidthCharacters[i] = true;
	}
	//mFixWidthCharacters[size_t(' ')] = true;
	mFixWidthCharacters[size_t('.')] = true;
	mFixWidthCharacters[size_t('+')] = true;
	mFixWidthCharacters[size_t('-')] = true;
	mFixWidthCharacters[size_t('*')] = true;
	mFixWidthCharacters[size_t('/')] = true;
	mMaxDxNumbers = 0;
	mTexture	= NULL;
}

FntData::~FntData()
{
	Reset();
}

void FntData::Reset()
{
	SAFE_RELEASE(mTexture);
	DELETEARRAY(mFnts);
	mNbFnts	= 0;
	mMaxDx	= 0;
	mMaxDy	= 0;
	memset(mXRef, 0, 256*sizeof(PxU8));
	memset(mFixWidthCharacters, 0, 256 * sizeof(bool));
	mMaxDxNumbers = 0;
}

// Compute size of a given text
PxU32 FntData::ComputeSize(const char* text, PxReal& width, PxReal& height, PxReal scale, bool forceFixWidthNumbers) const
{
	// Get and check length
	if(!text)	return 0;
	PxU32 Nb = (PxU32)strlen((const char*)text);
	if(!Nb) return 0;

	PxReal x = 0.0f;
	PxReal y = 0.0f;

	width = -1.0f;
	height = -1.0f;

	// Loop through characters
	for(PxU32 j=0;j<Nb;j++)
	{
		if(text[j]!='\n')
		{
			// Catch current character index
			const PxU32 i = mXRef[(unsigned char)(text[j])];

			// Catch size of current character
			const PxReal sx = PxReal((forceFixWidthNumbers && mFixWidthCharacters[(size_t)text[j]]) ? mMaxDxNumbers : mFnts[i].dx) * scale;
			const PxReal sy = PxReal(mFnts[i].dy) * scale;

			// Keep track of greatest dimensions
			if((x+sx)>width)	width = x+sx;
			if((y+sy)>height)	height = y+sy;

			// Adjust x for next character
			x += sx + 1.0f;
		}
		else
		{
			// Jump to next line
			x = 0.0f;
			y += PxReal(mMaxDy) * scale;
		}
	}
	return Nb;
}

#include "PxTkFile.h"

#if PX_INTEL_FAMILY
static const bool gFlip = false;
#elif PX_PPC
static const bool gFlip = true;
#elif PX_ARM_FAMILY
static const bool gFlip = false;
#else
#error Unknown platform!
#endif

PX_INLINE void Flip(PxU32& v)
{
	PxU8* b = (PxU8*)&v;

	PxU8 temp = b[0];
	b[0] = b[3];
	b[3] = temp;
	temp = b[1];
	b[1] = b[2];
	b[2] = temp;
}

PX_INLINE void Flip(PxI32& v)
{
	Flip((PxU32&)v);
}

PX_INLINE void Flip(PxF32& v)
{
	Flip((PxU32&)v);
}

static PxU32 read32(File* fp)
{
	PxU32 data;
	size_t numRead = fread(&data, 1, 4, fp);
	if(numRead != 4) { fclose(fp); return 0; }
	if(gFlip)
		Flip(data);
	return data;
}

bool FntData::Load(Renderer& renderer, const char* filename)
{
	File* fp = NULL;
	PxToolkit::fopen_s(&fp, filename, "rb");
	if(!fp)
		return false;

	// Init texture
	{
		const PxU32 width	= read32(fp);
		const PxU32 height	= read32(fp);
		PxU8* data = new PxU8[width*height*4];
		const size_t size = width*height*4;
		size_t numRead = fread(data, 1, size, fp);
		if(numRead != size) { fclose(fp); return false; }
		
		/*		if(gFlip)	
		{
		PxU32* data32 = (PxU32*)data;
		for(PxU32 i=0;i<width*height;i++)
		{
		Flip(data32[i]);
		}
		}*/

		RendererTexture2DDesc tdesc;
		tdesc.format	= RendererTexture2D::FORMAT_B8G8R8A8;
		tdesc.width		= width;
		tdesc.height	= height;
		tdesc.filter	= RendererTexture2D::FILTER_ANISOTROPIC;
		tdesc.numLevels	= 1;
		/*
		tdesc.filter;
		tdesc.addressingU;
		tdesc.addressingV;
		tdesc.renderTarget;
		*/
		PX_ASSERT(tdesc.isValid());
		mTexture = renderer.createTexture2D(tdesc);
		PX_ASSERT(mTexture);
		if(!mTexture)
		{
			DELETEARRAY(data);
			fclose(fp);
			return false;
		}

		const PxU32 componentCount = 4;

		if(mTexture)
		{
			PxU32 pitch = 0;
			void* buffer = mTexture->lockLevel(0, pitch);
			PX_ASSERT(buffer);
			if(buffer)
			{
				PxU8* levelDst			= (PxU8*)buffer;
				const PxU8* levelSrc	= (PxU8*)data;
				const PxU32 levelWidth	= mTexture->getWidthInBlocks();
				const PxU32 levelHeight	= mTexture->getHeightInBlocks();
				PX_ASSERT(levelWidth * mTexture->getBlockSize() <= pitch); // the pitch can't be less than the source row size.
				for(PxU32 row=0; row<levelHeight; row++)
				{ 
					// copy per pixel to handle RBG case, based on component count
					for(PxU32 col=0; col<levelWidth; col++)
					{
						*levelDst++ = levelSrc[0];
						*levelDst++ = levelSrc[1];
						*levelDst++ = levelSrc[2];
						*levelDst++ = levelSrc[3];
						levelSrc += componentCount;
					}
				}
			}
			mTexture->unlockLevel(0);
		}
		DELETEARRAY(data);
	}

	mNbFnts = read32(fp);

	mFnts	= new FntInfo[mNbFnts];
	const size_t size = mNbFnts*sizeof(FntInfo);
	size_t numRead = fread(mFnts, 1, size, fp);
	if(numRead != size) { fclose(fp); return false; }
	if(gFlip)
	{
		for(PxU32 i=0;i<mNbFnts;i++)
		{
			Flip(mFnts[i].u0);
			Flip(mFnts[i].v0);
			Flip(mFnts[i].u1);
			Flip(mFnts[i].v1);
			Flip(mFnts[i].dx);
			Flip(mFnts[i].dy);
		}
	}

	mMaxDx	= read32(fp);
	mMaxDy	= read32(fp);

	const size_t xRefSize = 256*sizeof(PxU8);
	numRead = fread(mXRef, 1, xRefSize, fp);
	if(numRead != xRefSize) { fclose(fp); return false; }

	for (unsigned int c = 0; c < 256; c++)
	{
		if (mFixWidthCharacters[c])
		{
			mMaxDxNumbers = PxMax(mFnts[mXRef[c]].dx, mMaxDxNumbers);
		}
	}

	fclose(fp);
	return true;
}


struct ClipBox
{
	PxReal	mXMin;
	PxReal	mYMin;
	PxReal	mXMax;
	PxReal	mYMax;
};

static bool ClipQuad(Renderer::TextVertex* /*quad*/, const ClipBox& /*clip_box*/)
{
	return true;
}

static bool GenerateTextQuads(	const char* text, PxU32 nb_characters,
	Renderer::TextVertex* fnt_verts, PxU16* fnt_indices, const ClipBox& clip_box, const FntData* fnt_data, PxReal& x, PxReal& y, PxReal scale_x, PxReal scale_y, PxU32 color,
	PxReal* x_min, PxReal* y_min, PxReal* x_max, PxReal* y_max, PxU32* nb_lines, PxU32* nb_active_characters, bool forceFixWidthNumbers)
{
	// Checkings
	if(!text || !fnt_verts || !fnt_indices || !fnt_data)	return false;

	PxReal mX = x;

	//////////

	Renderer::TextVertex*	V = fnt_verts;
	PxU16*					I = fnt_indices;
	PxU16					Offset = 0;
	PxU32					NbActiveCharacters	= 0;	// Number of non-NULL characters (the ones to render)

	PxReal					XMin = 100000.0f, XMax = -100000.0f;
	PxReal					YMin = 100000.0f, YMax = -100000.0f;

	PxU32					NbLines = 1;	// Number of lines

	// Loop through characters
	for(PxU32 j=0;j<nb_characters;j++)
	{
		if(text[j]!='\n')
		{
			PxU32 i = fnt_data->GetXRef()[(size_t)text[j]];
			const FntInfo character = fnt_data->GetFnts()[i];

			// Character size
			const PxReal sx = PxReal(character.dx) * scale_x;
			const PxReal sy = PxReal(character.dy) * scale_y;

			if (forceFixWidthNumbers && fnt_data->IsFixWidthCharacter(text[j]))
			{
				// move forward half the distance
				x += (fnt_data->GetMaxDxNumbers() - character.dx) * scale_x * 0.5f;
			}

			if(text[j]!=' ')
			{
				const PxReal rhw = 1.0f;


				// Initialize the vertices
				V[0].p.x = x;    V[0].p.y = y+sy; V[0].u = character.u0; V[0].v = character.v1;
				V[1].p.x = x;    V[1].p.y = y;    V[1].u = character.u0; V[1].v = character.v0;
				V[2].p.x = x+sx; V[2].p.y = y+sy; V[2].u = character.u1; V[2].v = character.v1;
				V[3].p.x = x+sx; V[3].p.y = y;    V[3].u = character.u1; V[3].v = character.v0;
				V[0].rhw = V[1].rhw = V[2].rhw = V[3].rhw = rhw;
				V[0].p.z = V[1].p.z = V[2].p.z = V[3].p.z = 0.0f;
				V[0].color = V[1].color = V[2].color = V[3].color = color;

				if(ClipQuad(V, clip_box))
				{
					V+=4;

					// Initialize the indices
					*I++ = Offset+0;
					*I++ = Offset+1;
					*I++ = Offset+2;
					*I++ = Offset+2;
					*I++ = Offset+1;
					*I++ = Offset+3;
					Offset+=4;

					NbActiveCharacters++;
				}
			}

			if (forceFixWidthNumbers && fnt_data->IsFixWidthCharacter(text[j]))
			{
				// move forward theo ther half of the distance
				x += (fnt_data->GetMaxDxNumbers() - character.dx) * scale_x * 0.5f;
			}

			//
			if((x+sx)>XMax)	XMax = x+sx;	if(x<XMin)	XMin = x;
			if((y+sy)>YMax)	YMax = y+sy;	if(y<YMin)	YMin = y;

			x += sx + 1.0f;
		}
		else
		{
			// Jump to next line
			x = mX;
			y += PxReal(fnt_data->GetMaxDy()) * scale_y;
			NbLines++;
		}
	}

	if(x_min)					*x_min = XMin;
	if(y_min)					*y_min = YMin;
	if(x_max)					*x_max = XMax;
	if(y_max)					*y_max = YMax;
	if(nb_lines)				*nb_lines = NbLines;
	if(nb_active_characters)	*nb_active_characters = NbActiveCharacters;

	return true;
}

enum RenderTextQuadFlag_
{
	RTQF_ALIGN_LEFT		= 0,
	RTQF_ALIGN_CENTER	= (1<<0),
	RTQF_ALIGN_RIGHT	= (1<<1),
};

struct RenderTextData
{
	RenderTextData(Renderer::TextVertex* pFntVerts,  PxU16* pFntIndices) 
		: mpFntVerts(pFntVerts), mpFntIndices(pFntIndices) { }

	~RenderTextData()
	{
		DELETEARRAY(mpFntVerts);
		DELETEARRAY(mpFntIndices);
	}

	Renderer::TextVertex* mpFntVerts;
	PxU16*                mpFntIndices;

private:
	// Disable default, copy and assign
	RenderTextData();
	RenderTextData(const RenderTextData&);
	void operator=(const RenderTextData&);
};

static void RenderTextQuads(Renderer* textRender,
	const FntData* fnts,
	const char* text, PxReal x, PxReal y, PxU32 text_color, PxU32 shadow_color,
	PxReal scale_x, PxReal scale_y,
	PxU32 align_flags, PxReal max_length,
	PxReal shadow_offset,
	PxReal* nx, PxReal* ny,
	const ClipBox* clip_box,
	PxReal text_y_offset,
	bool use_max_dy,
	bool forceFixWidthNumbers,
	RendererMaterial* material
	)
{
	// We want to render the whole text in one run...
	const PxReal text_x = x;
	const PxReal text_y = y;

	// Compute text size
	PxReal Width, Height;
	const PxU32 NbCharacters = fnts->ComputeSize(text, Width, Height, 1.0f, forceFixWidthNumbers);

	// Prepare clip box
	ClipBox CB;
	if(clip_box)
	{
		CB = *clip_box;
	}
	else
	{
		PxU32 width, height;
		textRender->getWindowSize(width, height);

		const PxReal Margin = 0.0f;
		CB.mXMin = Margin;
		CB.mYMin = Margin;
		CB.mXMax = PxReal(width) - Margin;
		CB.mYMax = PxReal(height) - Margin;
	}

	// Allocate space for vertices
	RenderTextData FntData(new Renderer::TextVertex[NbCharacters*4], new PxU16[NbCharacters*6]);

	// Generate quads
	PxReal XMin, YMin, XMax, YMax;
	PxU32 NbLines, NbActiveCharacters;
	GenerateTextQuads(text, NbCharacters, FntData.mpFntVerts, FntData.mpFntIndices, CB, fnts, x, y, scale_x, scale_y, text_color, &XMin, &YMin, &XMax, &YMax, &NbLines, &NbActiveCharacters, forceFixWidthNumbers);

	for(PxU32 i=0;i<NbActiveCharacters*4;i++)
		FntData.mpFntVerts[i].p.y += text_y_offset;

	if(use_max_dy)
		YMax = YMin + (PxReal)fnts->GetMaxDy();

	const bool centered = (align_flags & RTQF_ALIGN_CENTER)!=0;
	const bool align_right = (align_flags & RTQF_ALIGN_RIGHT)!=0;

	if(centered || align_right)
	{
		const PxReal L = XMax - XMin;
		XMax = XMin + max_length;
		const PxReal Offset = centered ? (-FntData.mpFntVerts[0].p.x + XMin + (max_length - L)*0.5f) :
			(-FntData.mpFntVerts[0].p.x + XMax - L);
		//										(-FntVerts[0].p.x + XMin + (max_length - L));
		for(PxU32 i=0;i<NbActiveCharacters*4;i++)
			FntData.mpFntVerts[i].p.x += Offset;
	}

	textRender->setupTextRenderStates();

	// Handle shadow
	if(shadow_offset!=0.0f)
	{
		// Allocate space for vertices
		RenderTextData SFntData(new Renderer::TextVertex[NbCharacters*4], new PxU16[NbCharacters*6]);

		// Generate quads
		PxReal SXMin, SYMin, SXMax, SYMax;
		PxU32 SNbLines, SNbActiveCharacters;
		PxReal ShX = text_x + shadow_offset;
		PxReal ShY = text_y + shadow_offset;
		GenerateTextQuads(text, NbCharacters, SFntData.mpFntVerts, SFntData.mpFntIndices, CB, fnts, ShX, ShY, scale_x, scale_y, shadow_color, &SXMin, &SYMin, &SXMax, &SYMax, &SNbLines, &SNbActiveCharacters, forceFixWidthNumbers);

		for(PxU32 i=0;i<SNbActiveCharacters*4;i++)
			SFntData.mpFntVerts[i].p.y += text_y_offset;

		if(centered || align_right)
		{
			const PxReal L = SXMax - SXMin;
			SXMax = SXMin + max_length;
			const PxReal Offset = centered ? (-SFntData.mpFntVerts[0].p.x + SXMin + (max_length - L)*0.5f) : 
				(-SFntData.mpFntVerts[0].p.x + SXMax - L);
			for(PxU32 i=0;i<SNbActiveCharacters*4;i++)
				SFntData.mpFntVerts[i].p.x += Offset;
		}

		textRender->renderTextBuffer(SFntData.mpFntVerts, 4*SNbActiveCharacters, SFntData.mpFntIndices, 6*SNbActiveCharacters, material);
	}

	textRender->renderTextBuffer(FntData.mpFntVerts, 4*NbActiveCharacters, FntData.mpFntIndices, 6*NbActiveCharacters, material);

	textRender->resetTextRenderStates();
}


static FntData*	gFntData = NULL;


bool Renderer::initTexter()
{
	if(gFntData)
		return true;

	char filename[1024];
	Ps::strlcpy(filename, sizeof(filename), getAssetDir());
	Ps::strlcat(filename, sizeof(filename), "fonts/arial_black.bin");

	gFntData = new FntData;
	if(!gFntData->Load(*this, filename))
	{
		closeTexter();
		return false;
	}

	{
		RendererMaterialDesc matDesc;
		matDesc.alphaTestFunc		= RendererMaterial::ALPHA_TEST_ALWAYS;
		matDesc.alphaTestRef		= 0.0f;
		matDesc.type				= RendererMaterial::TYPE_UNLIT;
		matDesc.blending			= true;
		matDesc.srcBlendFunc	    = RendererMaterial::BLEND_SRC_ALPHA;
		matDesc.dstBlendFunc	    = RendererMaterial::BLEND_ONE_MINUS_SRC_ALPHA;
		matDesc.geometryShaderPath	= NULL;
		matDesc.vertexShaderPath	= "vertex/text.cg";
		matDesc.fragmentShaderPath	= "fragment/text.cg";
		PX_ASSERT(matDesc.isValid());

		m_textMaterial = createMaterial(matDesc);
		if(!m_textMaterial)
		{
			closeTexter();
			return false;
		}

		m_textMaterialInstance = new RendererMaterialInstance(*m_textMaterial);
		if(!m_textMaterialInstance)
		{
			closeTexter();
			return false;
		}

		const RendererMaterial::Variable* var = m_textMaterial->findVariable("diffuseTexture", RendererMaterial::VARIABLE_SAMPLER2D);
		if(!var)
		{
			closeTexter();
			return false;
		}
		m_textMaterialInstance->writeData(*var, &(gFntData->mTexture));
	}

	return true;
}

void Renderer::closeTexter()
{
	DELETESINGLE(m_textMaterialInstance);
	SAFE_RELEASE(m_textMaterial);
	DELETESINGLE(gFntData);
}

void SampleRenderer::Renderer::print(PxU32* x, PxU32* y, const char** text, PxU32 textCount, PxReal scale, PxReal shadowOffset, RendererColor* textColors, bool forceFixWidthNumbers)
{
	// we split string containing '\n' in a vector of strings. This prevents from need to have a really big vertex/index buffer for text in GLES2 renderer (sschirm: which is not supported anymore...)

	if(!gFntData || !gFntData->mTexture || !m_textMaterial || !m_textMaterialInstance || 0==textCount || NULL==x || NULL==y || NULL==text || 0 == *text || (*text)[0] == '\0')
		return;

	ScopedRender renderSection(*this);
	if (renderSection)
	{
		m_textMaterial->bind(RendererMaterial::PASS_UNLIT, m_textMaterialInstance, false);

		if(!m_useShadersForTextRendering)
		{
			m_textMaterial->unbind();
			gFntData->mTexture->select(0);
		}

		const PxU32 alignFlags  = 0;
		const float maxLength   = 0.0f;
		float* nx               = NULL;
		float* ny               = NULL;
		const ClipBox* clipBox  = NULL;
		const float textYOffset = 0.0f;
		const bool useMaxDy     = false;
		const RendererColor defaultColor(255, 255, 255, 255);

		for (PxU32 i = 0; i < textCount; ++i)
		{
			const PxU32 color       = convertColor(textColors ? textColors[i] : defaultColor);
			const PxU32 shadowColor = convertColor(textColors ? RendererColor(0,0,0,textColors[i].a) : RendererColor(0,0,0,defaultColor.a));

			RenderTextQuads(this,
				gFntData,
				text[i],
				PxReal(x[i]), PxReal(y[i]),
				color, shadowColor,
				scale, scale,
				alignFlags,
				maxLength,
				shadowOffset * scale,
				nx, ny,
				clipBox,
				textYOffset,
				useMaxDy,
				forceFixWidthNumbers,
				m_textMaterial
				);
		}

		if(m_useShadersForTextRendering)
			m_textMaterial->unbind();
	}
}

bool SampleRenderer::Renderer::captureScreen(const char* filename)
{
	if(!filename)
		return false;

	physx::PxU32 width, height, sizeInBytes;
	const void* data = NULL;
	if (captureScreen(width, height, sizeInBytes, data) && sizeInBytes > 0)
	{
		PxDefaultFileOutputStream fileBuffer(filename);
		return fileBuffer.write(data, sizeInBytes) > 0;
	}
	else
	{
		return false;
	}
}


void Renderer::print(PxU32 x, PxU32 y, const char* text, PxReal scale, PxReal shadowOffset, RendererColor textColor, bool forceFixWidthNumbers)
{
	if (NULL == text || strlen(text) <= 0)
		return;
	print(&x, &y, &text, 1, scale, shadowOffset, &textColor, forceFixWidthNumbers);
}

///////////////////////////////////////////////////////////////////////////////

bool Renderer::initScreenquad()
{
	RendererMaterialDesc matDesc;
	matDesc.alphaTestFunc		= RendererMaterial::ALPHA_TEST_ALWAYS;
	matDesc.alphaTestRef		= 0.0f;
	matDesc.type				= RendererMaterial::TYPE_UNLIT;
	matDesc.blending			= false;
	matDesc.vertexShaderPath	= "vertex/screenquad.cg";
	matDesc.fragmentShaderPath	= "fragment/screenquad.cg";	
	matDesc.geometryShaderPath	= NULL;

	matDesc.srcBlendFunc		= RendererMaterial::BLEND_ONE;
	matDesc.dstBlendFunc		= RendererMaterial::BLEND_ONE;

	PX_ASSERT(matDesc.isValid());

	m_screenquadOpaqueMaterial = createMaterial(matDesc);
	if(!m_screenquadOpaqueMaterial)
		{
		closeScreenquad();
		return false;
	}

	m_screenquadOpaqueMaterialInstance = new RendererMaterialInstance(*m_screenquadOpaqueMaterial);
	if(!m_screenquadOpaqueMaterialInstance)
	{
		closeScreenquad();
		return false;
	}

	matDesc.blending		= true;
	matDesc.srcBlendFunc	= RendererMaterial::BLEND_SRC_ALPHA;
	matDesc.dstBlendFunc	= RendererMaterial::BLEND_ONE_MINUS_SRC_ALPHA;

	m_screenquadAlphaMaterial = createMaterial(matDesc);
	if(!m_screenquadAlphaMaterial)
		{
		closeScreenquad();
		return false;
	}

	m_screenquadAlphaMaterialInstance = new RendererMaterialInstance(*m_screenquadAlphaMaterial);
	if(!m_screenquadAlphaMaterialInstance )
	{
		closeScreenquad();
		return false;
	}

	return true;
}

void Renderer::closeScreenquad()
{
	DELETESINGLE(m_screenquadAlphaMaterialInstance);
	SAFE_RELEASE(m_screenquadAlphaMaterial);
	DELETESINGLE(m_screenquadOpaqueMaterialInstance);
	SAFE_RELEASE(m_screenquadOpaqueMaterial);
}

ScreenQuad::ScreenQuad() :
	mLeftUpColor		(0xffffffff),
	mLeftDownColor		(0xffffffff),
	mRightUpColor		(0xffffffff),
	mRightDownColor		(0xffffffff),
	mAlpha				(1.0f),
	mX0					(0.0f),
	mY0					(0.0f),
	mX1					(1.0f),
	mY1					(1.0f)
{
}

bool Renderer::drawScreenQuad(const ScreenQuad& screenQuad)
{
	RendererMaterial* screenquadMaterial;
	RendererMaterialInstance* screenquadMaterialInstance;
	if(screenQuad.mAlpha==1.0f)
	{
		screenquadMaterial			= m_screenquadOpaqueMaterial;
		screenquadMaterialInstance	= m_screenquadOpaqueMaterialInstance;
	}
	else
	{
		screenquadMaterial			= m_screenquadAlphaMaterial;
		screenquadMaterialInstance	= m_screenquadAlphaMaterialInstance;
	}

	if(!screenquadMaterialInstance || !screenquadMaterial)
		return false;

	screenquadMaterial->bind(RendererMaterial::PASS_UNLIT, screenquadMaterialInstance, false);
	if(!m_useShadersForTextRendering)
		screenquadMaterial->unbind();

	ScopedRender renderSection(*this);
	if(renderSection)
	{
		setupScreenquadRenderStates();

		TextVertex Verts[4];
		const PxU16 Indices[6] = { 0,1,2,2,1,3 };

		PxU32 renderWidth, renderHeight;
		getWindowSize(renderWidth, renderHeight);

		// Initalize the vertices
		const PxReal xCoeff	= PxReal(renderWidth);
		const PxReal yCoeff	= PxReal(renderHeight);
		const PxReal x0		= screenQuad.mX0 * xCoeff;
		const PxReal y0		= screenQuad.mY0 * yCoeff;
		const PxReal sx		= screenQuad.mX1 * xCoeff;
		const PxReal sy		= screenQuad.mY1 * yCoeff;
		const PxReal rhw	= 1.0f;
		const PxReal z		= 0.0f;

		RendererColor leftUpColor		= screenQuad.mLeftUpColor;
		RendererColor leftDownColor		= screenQuad.mLeftDownColor;
		RendererColor rightUpColor		= screenQuad.mRightUpColor;
		RendererColor rightDownColor	= screenQuad.mRightDownColor;
		const PxU8 alpha = PxU8(screenQuad.mAlpha*255.0f);
		leftUpColor.a = alpha;
		leftDownColor.a = alpha;
		rightUpColor.a = alpha;
		rightDownColor.a = alpha;

		Verts[0].p	= PxVec3(x0, sy, z);		Verts[0].rhw	= rhw;		Verts[0].color	= convertColor(leftDownColor);
		Verts[1].p	= PxVec3(x0, y0, z);		Verts[1].rhw	= rhw;		Verts[1].color	= convertColor(leftUpColor);
		Verts[2].p	= PxVec3(sx, sy, z);		Verts[2].rhw	= rhw;		Verts[2].color	= convertColor(rightDownColor);
		Verts[3].p	= PxVec3(sx, y0, z);		Verts[3].rhw	= rhw;		Verts[3].color	= convertColor(rightUpColor);

		renderTextBuffer(Verts, 4, Indices, 6, screenquadMaterial);	
		resetScreenquadRenderStates();
	}

	if(m_useShadersForTextRendering)
		screenquadMaterial->unbind();

	return true;
}

bool Renderer::drawTouchControls()
{
	return true;
}

bool Renderer::drawLines2D(PxU32 nbVerts, const PxReal* vertices, const RendererColor& color)
{
	RendererMaterial* screenquadMaterial;
	RendererMaterialInstance* screenquadMaterialInstance;
	screenquadMaterial			= m_screenquadOpaqueMaterial;
	screenquadMaterialInstance	= m_screenquadOpaqueMaterialInstance;
	if(!screenquadMaterialInstance || !screenquadMaterial)
		return false;

	screenquadMaterial->bind(RendererMaterial::PASS_UNLIT, screenquadMaterialInstance, false);
	if(!m_useShadersForTextRendering)
		screenquadMaterial->unbind();

	setupScreenquadRenderStates();

	TextVertex* verts = new TextVertex[nbVerts];

	PxU32 renderWidth, renderHeight;
	getWindowSize(renderWidth, renderHeight);
	const PxReal xCoeff	= PxReal(renderWidth);
	const PxReal yCoeff	= PxReal(renderHeight);

	const PxU32 convertedColor = convertColor(color);

	for(PxU32 i=0;i<nbVerts;i++)
	{
		verts[i].p.x	= vertices[i*2+0] * xCoeff;
		verts[i].p.y	= vertices[i*2+1] * yCoeff;
		verts[i].p.z	= 0.0f;
		verts[i].rhw	= 1.0f;
		verts[i].color	= convertedColor;
		verts[i].u		= 0.0f;
		verts[i].v		= 0.0f;
	}

	renderLines2D(verts, nbVerts);

	DELETEARRAY(verts);

	resetScreenquadRenderStates();
	if(m_useShadersForTextRendering)
		screenquadMaterial->unbind();

	return true;
}

bool Renderer::drawLines2D(PxU32 nbVerts, const PxReal* vertices, const RendererColor* colors)
{
	RendererMaterial* screenquadMaterial;
	RendererMaterialInstance* screenquadMaterialInstance;
	screenquadMaterial			= m_screenquadOpaqueMaterial;
	screenquadMaterialInstance	= m_screenquadOpaqueMaterialInstance;
	if(!screenquadMaterialInstance || !screenquadMaterial)
		return false;

	screenquadMaterial->bind(RendererMaterial::PASS_UNLIT, screenquadMaterialInstance, false);
	if(!m_useShadersForTextRendering)
		screenquadMaterial->unbind();

	setupScreenquadRenderStates();

	TextVertex* verts = new TextVertex[nbVerts];

	PxU32 renderWidth, renderHeight;
	getWindowSize(renderWidth, renderHeight);
	const PxReal xCoeff	= PxReal(renderWidth);
	const PxReal yCoeff	= PxReal(renderHeight);

	for(PxU32 i=0;i<nbVerts;i++)
	{
		verts[i].p.x	= vertices[i*2+0] * xCoeff;
		verts[i].p.y	= vertices[i*2+1] * yCoeff;
		verts[i].p.z	= 0.0f;
		verts[i].rhw	= 1.0f;
		verts[i].color	= convertColor(colors[i]);
		verts[i].u		= 0.0f;
		verts[i].v		= 0.0f;
	}

	renderLines2D(verts, nbVerts);

	DELETEARRAY(verts);

	resetScreenquadRenderStates();
	if(m_useShadersForTextRendering)
		screenquadMaterial->unbind();

	return true;
}


