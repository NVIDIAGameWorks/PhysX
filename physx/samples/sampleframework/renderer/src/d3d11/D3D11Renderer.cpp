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

#if defined(RENDERER_ENABLE_DIRECT3D11)

#include "D3D11Renderer.h"

#include <RendererDesc.h>

#include <RendererProjection.h>

#include <RendererVertexBufferDesc.h>
#include "D3D11RendererVertexBuffer.h"

#include <RendererIndexBufferDesc.h>
#include "D3D11RendererIndexBuffer.h"

#include <RendererInstanceBufferDesc.h>
#include "D3D11RendererInstanceBuffer.h"

#include <RendererMeshDesc.h>
#include <RendererMeshContext.h>
#include "D3D11RendererMesh.h"

#include <RendererMaterialDesc.h>
#include <RendererMaterialInstance.h>
#include "D3D11RendererMaterial.h"

#include <RendererLightDesc.h>
#include <RendererDirectionalLightDesc.h>
#include "D3D11RendererDirectionalLight.h"
#include <RendererSpotLightDesc.h>
#include "D3D11RendererSpotLight.h"

#include <RendererTextureDesc.h>
#include "D3D11RendererTexture2D.h"
#include "D3D11RendererTexture3D.h"

#include <RendererTargetDesc.h>
#include "D3D11RendererTarget.h"

#include "D3D11RendererResourceManager.h"
#include "D3D11RendererVariableManager.h"
#include "D3D11RendererMemoryMacros.h"
#include "D3D11RendererUtils.h"

#include <PsUtilities.h>
#include <SamplePlatform.h>
#include <deque>

using namespace SampleRenderer;
using std::tr1::get;

void SampleRenderer::convertToD3D11(PxVec3& dxcolor, const RendererColor& color)
{
	static const float inv255 = 1.0f / 255.0f;
	dxcolor = PxVec3(color.r * inv255, color.g * inv255, color.b * inv255);
}

void SampleRenderer::convertToD3D11(PxVec4& dxcolor, const RendererColor& color)
{
	static const float inv255 = 1.0f / 255.0f;
	dxcolor = PxVec4(color.r * inv255, color.g * inv255, color.b * inv255, color.a * inv255);
}

void SampleRenderer::convertToD3D11(PxMat44& dxmat, const physx::PxMat44 &mat)
{
	dxmat = mat.getTranspose();
}

void SampleRenderer::convertToD3D11(PxMat44& dxmat, const RendererProjection& mat)
{
	convertToD3D11(dxmat, mat.getPxMat44());	
}

/****************************************
* D3D11Renderer::D3D11ShaderEnvironment *
****************************************/

D3D11Renderer::D3D11ShaderEnvironment::D3D11ShaderEnvironment()
{
	memset(this, 0, sizeof(*this));
	vfs = 1.;
}

#define SHADER_VAR_NAME(_name) PX_STRINGIZE(PX_CONCAT(g_, _name))

void D3D11Renderer::D3D11ShaderEnvironment::bindFrame() const
{
	constantManager->setSharedVariable("cbFrame",    SHADER_VAR_NAME(viewMatrix), &viewMatrix);
	constantManager->setSharedVariable("cbFrame",    SHADER_VAR_NAME(projMatrix), &projMatrix);
	constantManager->setSharedVariable("cbFrame",    SHADER_VAR_NAME(eyePosition),  &eyePosition.x);
	constantManager->setSharedVariable("cbFrame",    SHADER_VAR_NAME(eyeDirection), &eyeDirection.x);
	constantManager->setSharedVariable("cbFrameInv", SHADER_VAR_NAME(invViewProjMatrix), &invViewProjMatrix);
}

void D3D11Renderer::D3D11ShaderEnvironment::bindLight(PxU32 lightIndex) const
{
	RENDERER_ASSERT(lightIndex < RENDERER_MAX_LIGHTS, "Invalid light index");
	if (lightIndex < RENDERER_MAX_LIGHTS)
	{
		if (numLights < ((int)lightIndex + 1)) numLights = lightIndex + 1;
		const char* lightName = RENDERER_ENABLE_SINGLE_PASS_LIGHTING ? "cbLights" : "cbLight";
		constantManager->setSharedVariable(lightName, SHADER_VAR_NAME(lightColor),       &lightColor[lightIndex].x,     sizeof(PxVec3), lightIndex*sizeof(PxVec3));
		constantManager->setSharedVariable(lightName, SHADER_VAR_NAME(lightDirection),   &lightDirection[lightIndex].x, sizeof(PxVec3), lightIndex*sizeof(PxVec3));
		constantManager->setSharedVariable(lightName, SHADER_VAR_NAME(lightPosition),    &lightPosition[lightIndex].x,  sizeof(PxVec3), lightIndex*sizeof(PxVec3));
		constantManager->setSharedVariable(lightName, SHADER_VAR_NAME(lightIntensity),   &lightIntensity[lightIndex],   sizeof(float),    lightIndex*sizeof(float));
		constantManager->setSharedVariable(lightName, SHADER_VAR_NAME(lightInnerRadius), &lightInnerRadius[lightIndex], sizeof(float),    lightIndex*sizeof(float));
		constantManager->setSharedVariable(lightName, SHADER_VAR_NAME(lightOuterRadius), &lightOuterRadius[lightIndex], sizeof(float),    lightIndex*sizeof(float));
		constantManager->setSharedVariable(lightName, SHADER_VAR_NAME(lightInnerCone),   &lightInnerCone[lightIndex],   sizeof(float),    lightIndex*sizeof(float));
		constantManager->setSharedVariable(lightName, SHADER_VAR_NAME(lightOuterCone),   &lightOuterCone[lightIndex],   sizeof(float),    lightIndex*sizeof(float));
#if RENDERER_ENABLE_SINGLE_PASS_LIGHTING
		constantManager->setSharedVariable(lightName, SHADER_VAR_NAME(lightType),        &lightType[lightIndex],        sizeof(int),      lightIndex*sizeof(int));
		constantManager->setSharedVariable(lightName, SHADER_VAR_NAME(numLights),        &numLights,                    sizeof(int));
#endif

#if RENDERER_ENABLE_SHADOWS
		if (lightShadowMap)
		{
			// The actual shadow map texture will be bound as a variable of the material
			constantManager->setSharedVariable("cbLightShadow", SHADER_VAR_NAME(lightShadowMatrix),   &lightShadowMatrix);
		}
#endif
	}
}

void D3D11Renderer::D3D11ShaderEnvironment::bindModel() const
{
	constantManager->setSharedVariable("cbMesh", SHADER_VAR_NAME(modelMatrix),         &modelMatrix);
	constantManager->setSharedVariable("cbMesh", SHADER_VAR_NAME(modelViewMatrix),     &modelViewMatrix);
	constantManager->setSharedVariable("cbMesh", SHADER_VAR_NAME(modelViewProjMatrix), &modelViewProjMatrix);
}

void D3D11Renderer::D3D11ShaderEnvironment::bindBones() const
{
	if (numBones > 0)
	{
		constantManager->setSharedVariable("cbBones", SHADER_VAR_NAME(boneMatrices), &boneMatrices[0], numBones * sizeof(PxMat44));
	}
}

void D3D11Renderer::D3D11ShaderEnvironment::bindFogState() const
{
	constantManager->setSharedVariable("cbFog", SHADER_VAR_NAME(fogColorAndDistance), &fogColorAndDistance.x);
}

void D3D11Renderer::D3D11ShaderEnvironment::bindAmbientState() const
{
	constantManager->setSharedVariable("cbAmbient", SHADER_VAR_NAME(ambientColor), &ambientColor.x);
}

void D3D11Renderer::D3D11ShaderEnvironment::bindVFaceScale() const
{
#if defined(RENDERER_ENABLE_VFACE_SCALE)
	constantManager->setSharedVariable("cbScale", SHADER_VAR_NAME(vfaceScale), &vfs);
#endif
}

void D3D11Renderer::D3D11ShaderEnvironment::reset()
{
	numLights = 0;
}

#undef SHADER_VAR_NAME

/***************
* D3D11Renderer *
***************/

static std::deque<ID3D11BlendState*>        gBlendState;
static std::deque<ID3D11DepthStencilState*> gDepthStencilState;
static std::deque<ID3D11RasterizerState*>   gRasterizerState;
static std::deque<D3D11_VIEWPORT>           gViewport;
static std::deque<PxVec4> gBlendFactor;
static std::deque<UINT>   gBlendMask;
static std::deque<UINT>   gDepthStencilMask;

D3D11Renderer::D3D11Renderer(const RendererDesc& desc, const char* assetDir) :
	Renderer(DRIVER_DIRECT3D11, desc.errorCallback, assetDir),
	m_displayWidth(0),
	m_displayHeight(0),
	m_displayBuffer(0),
	m_vsync(desc.vsync),
	m_multipassDepthBias(desc.multipassDepthBias ? -1 : 0),
	m_d3d(NULL),
	m_d3dSwapChain(NULL),
	m_d3dDevice(NULL),
	m_d3dDeviceContext(NULL),
	m_d3dDeviceFeatureLevel(D3D_FEATURE_LEVEL_11_0),
	m_d3dDepthStencilBuffer(NULL),
	m_d3dDepthStencilView(NULL),
	m_d3dRenderTargetBuffer(NULL),
	m_d3dRenderTargetView(NULL),
	m_currentTextMesh(0),
	m_linesMesh(NULL),
	m_linesVertices(NULL),
	m_d3dx(NULL),
	m_constantManager(NULL),
	m_resourceManager(NULL)
{
	for (PxU32 i = 0; i < NUM_BLEND_STATES; ++i)
		m_d3dBlendStates[i] = NULL;
	for (PxU32 i = 0; i < NUM_DEPTH_STENCIL_STATES; ++i)
		m_d3dDepthStencilStates[i] = NULL;

	m_useShadersForTextRendering = true;
	m_pixelCenterOffset          = 0.f;
	m_viewMatrix = physx::PxMat44(PxIdentity);

	SampleFramework::SamplePlatform* m_platform = SampleFramework::SamplePlatform::platform();
	m_d3d = static_cast<IDXGIFactory*>(m_platform->initializeD3D11());
	RENDERER_ASSERT(m_d3d, "Could not create Direct3D11 Interface.");
	if (m_d3d)
	{
		// Setup swap chain configuration
		memset(&m_swapChainDesc, 0, sizeof(m_swapChainDesc));
		m_swapChainDesc.BufferCount                        = 1;
		m_swapChainDesc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		m_swapChainDesc.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_swapChainDesc.BufferDesc.Height                  = m_displayHeight;
		m_swapChainDesc.BufferDesc.RefreshRate.Numerator   = 60;
		m_swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		m_swapChainDesc.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
		m_swapChainDesc.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		m_swapChainDesc.BufferDesc.Width                   = m_displayWidth;
		m_swapChainDesc.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH ;
		m_swapChainDesc.SampleDesc.Count                   = 1;
		m_swapChainDesc.SampleDesc.Quality                 = 0;
		m_swapChainDesc.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;
		m_swapChainDesc.Windowed                           = 1;

		HRESULT hr = m_platform->initializeD3D11Display(
		             &m_swapChainDesc,
		             m_deviceName,
		             m_displayWidth,
		             m_displayHeight,
		             &m_d3dDevice,
		             &m_d3dDeviceContext,
		             &m_d3dSwapChain);
		RENDERER_ASSERT(SUCCEEDED(hr), "Failed to create Direct3D11 Device.");
		if (SUCCEEDED(hr))
		{
			m_d3dDeviceFeatureLevel = m_d3dDevice->GetFeatureLevel();
			setEnableTessellation(true);
			checkResize(false);
		}
	}

	D3D11RendererVariableManager::StringSet cbNames;
	cbNames.insert("cbAllData");
	cbNames.insert("cbMesh");
	cbNames.insert("cbFrame");
	cbNames.insert("cbFrameInv");
	cbNames.insert("cbBones");
	cbNames.insert("cbFog");
	cbNames.insert("cbAmbient");
	cbNames.insert("cbLight");
	cbNames.insert("cbLights");
	cbNames.insert("cbLightShadow");
	cbNames.insert("cbScale");
	cbNames.insert("cbTessellation");

	m_d3dx            = new D3DX11();
	m_constantManager = new D3D11RendererVariableManager(*this, cbNames /*, D3D11RendererVariableManager::BIND_MAP*/);
	m_resourceManager = new D3D11RendererResourceManager( );
	m_environment.constantManager = m_constantManager;
}

D3D11Renderer::~D3D11Renderer(void)
{
	releaseAllMaterials();

	if (m_d3dDeviceContext)
	{
		m_d3dDeviceContext->ClearState();
		m_d3dDeviceContext->Flush();
	}

	dxSafeRelease(m_displayBuffer);

	dxSafeRelease(m_d3d);
	dxSafeRelease(m_d3dSwapChain);
	dxSafeRelease(m_d3dDevice);
	dxSafeRelease(m_d3dDeviceContext);

	dxSafeRelease(m_d3dDepthStencilBuffer);
	dxSafeRelease(m_d3dDepthStencilView);
	dxSafeRelease(m_d3dRenderTargetBuffer);
	dxSafeRelease(m_d3dRenderTargetView);

	for (PxU32 i = 0; i < NUM_BLEND_STATES; ++i)
		dxSafeRelease(m_d3dBlendStates[i]);
	for (PxU32 i = 0; i < NUM_DEPTH_STENCIL_STATES; ++i)
		dxSafeRelease(m_d3dDepthStencilStates[i]);

	dxSafeReleaseAll(gRasterizerState);
	dxSafeReleaseAll(gBlendState);
	dxSafeReleaseAll(gDepthStencilState);

	deleteAll(m_textMeshes);
	deleteAll(m_textVertices);
	deleteAll(m_textIndices);

	delete m_linesMesh;
	delete m_linesVertices;

	delete m_constantManager;
	delete m_resourceManager;
	delete m_d3dx;
}

bool D3D11Renderer::checkResize(bool resetDevice)
{
	bool isDeviceReset = false;
#if defined(RENDERER_WINDOWS)
	if (SampleFramework::SamplePlatform::platform()->getWindowHandle() && m_d3dDevice)
	{
		PxU32 width  = 0;
		PxU32 height = 0;
		SampleFramework::SamplePlatform::platform()->getWindowSize(width, height);
		if (width && height && (width != m_displayWidth || height != m_displayHeight) || resetDevice)
		{
			m_displayWidth  = width;
			m_displayHeight = height;
			m_d3dDeviceContext->ClearState();
			m_d3dDeviceContext->OMSetRenderTargets(0, NULL, NULL);

			dxSafeRelease(m_d3dRenderTargetView);
			dxSafeRelease(m_d3dRenderTargetBuffer);
			dxSafeRelease(m_d3dDepthStencilView);
			dxSafeRelease(m_d3dDepthStencilBuffer);

			HRESULT hr = S_OK;
			DXGI_SWAP_CHAIN_DESC desc;
			m_d3dSwapChain->GetDesc(&desc);
			if(desc.BufferDesc.Width != width || desc.BufferDesc.Height != height)
				hr = m_d3dSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
			RENDERER_ASSERT(SUCCEEDED(hr), "Failed to resize swap chain buffers.");

			D3D11_TEXTURE2D_DESC depthDesc;
			depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			if (SUCCEEDED(hr))
			{
				depthDesc.Width              = m_displayWidth;
				depthDesc.Height             = m_displayHeight;
				depthDesc.MipLevels          = 1;
				depthDesc.ArraySize          = 1;
				depthDesc.SampleDesc.Count   = m_swapChainDesc.SampleDesc.Count;
				depthDesc.SampleDesc.Quality = m_swapChainDesc.SampleDesc.Quality;
				depthDesc.Usage              = D3D11_USAGE_DEFAULT;
				depthDesc.BindFlags          = D3D11_BIND_DEPTH_STENCIL;
				depthDesc.CPUAccessFlags     = 0;
				depthDesc.MiscFlags          = 0;

				hr = m_d3dDevice->CreateTexture2D(&depthDesc, NULL, &m_d3dDepthStencilBuffer);
				RENDERER_ASSERT(SUCCEEDED(hr), "Failed to create depth stencil buffer.");
			}

			if (SUCCEEDED(hr))
			{
				hr = m_d3dSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&m_d3dRenderTargetBuffer);
				RENDERER_ASSERT(SUCCEEDED(hr), "Failed to acquire swap chain buffer.");
			}

			if (SUCCEEDED(hr) && m_d3dRenderTargetBuffer)
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
				rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
				//rtvDesc.Texture2D.MipSlice = 0;
				hr = m_d3dDevice->CreateRenderTargetView(m_d3dRenderTargetBuffer, &rtvDesc, &m_d3dRenderTargetView);
				RENDERER_ASSERT(SUCCEEDED(hr), "Failed to create render target view.");
			}

			if (SUCCEEDED(hr))
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
				dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
				dsvDesc.Format = depthDesc.Format;
				dsvDesc.Flags = 0;
				//dsvDesc.Texture2D.MipSlice = 0;
				hr = m_d3dDevice->CreateDepthStencilView(m_d3dDepthStencilBuffer, &dsvDesc, &m_d3dDepthStencilView);
				RENDERER_ASSERT(SUCCEEDED(hr), "Failed to create depth stencil view.");
			}

			m_d3dDeviceContext->OMSetRenderTargets(1, &m_d3dRenderTargetView, m_d3dDepthStencilView);

			if (resetDevice)
			{
				onDeviceReset();
				isDeviceReset = true;
			}

			PxU32 NUM_VIEWPORTS = 1;
			D3D11_VIEWPORT viewport[1];
			ZeroMemory(viewport, sizeof(D3D11_VIEWPORT)*NUM_VIEWPORTS);
			m_d3dDeviceContext->RSGetViewports(&NUM_VIEWPORTS, viewport);
			viewport[0].Width  = (FLOAT)m_displayWidth;
			viewport[0].Height = (FLOAT)m_displayHeight;
			viewport[0].MinDepth =  0.0f;
			viewport[0].MaxDepth =  1.0f;
			m_d3dDeviceContext->RSSetViewports(1, viewport);
		}
	}

#endif
	return isDeviceReset;
}

void D3D11Renderer::onDeviceLost(void)
{
	notifyResourcesLostDevice();
}

void D3D11Renderer::onDeviceReset(void)
{
	notifyResourcesResetDevice();
}

// clears the offscreen buffers.
void D3D11Renderer::clearBuffers(void)
{
	if (m_d3dDeviceContext && m_d3dRenderTargetView)
	{
		PxVec4 clearColor;
		convertToD3D11(clearColor, getClearColor());
		m_d3dDeviceContext->ClearRenderTargetView(m_d3dRenderTargetView, &clearColor.x);
	}
	if (m_d3dDeviceContext && m_d3dDepthStencilView)
	{
		m_d3dDeviceContext->ClearDepthStencilView(m_d3dDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);
	}
}

// presents the current color buffer to the screen.
// returns true on device reset and if buffers need to be rewritten.
bool D3D11Renderer::swapBuffers(void)
{
	PX_PROFILE_ZONE("SwapBuffers", 0);
	bool isDeviceReset = false;
	if (m_d3dDevice)
	{
		HRESULT result = SampleFramework::SamplePlatform::platform()->D3D11Present(m_vsync);
		if ( DXGI_STATUS_OCCLUDED == result )
		{
			// This is fine, though we should set a flag to use a "test" D3D11Present 
			//      for the next frame until the window is no longer occluded
		}
		else if (SUCCEEDED(result) || DXGI_ERROR_DEVICE_RESET == result)
		{
			isDeviceReset = checkResize(DXGI_ERROR_DEVICE_RESET == result);
		}
	}
	return isDeviceReset;
}

void D3D11Renderer::getWindowSize(PxU32& width, PxU32& height) const
{
	RENDERER_ASSERT(m_displayHeight * m_displayWidth > 0, "variables not initialized properly");
	width = m_displayWidth;
	height = m_displayHeight;
}

RendererVertexBuffer* D3D11Renderer::createVertexBuffer(const RendererVertexBufferDesc& desc)
{
	D3D11RendererVertexBuffer* vb = 0;
	if (m_d3dDevice)
	{
		RENDERER_ASSERT(desc.isValid(), "Invalid Vertex Buffer Descriptor.");
		if (desc.isValid())
		{
			vb = new D3D11RendererVertexBuffer(*m_d3dDevice, *m_d3dDeviceContext, desc);
		}
	}
	if (vb)
	{
		addResource(*vb);
	}
	return vb;
}

RendererIndexBuffer* D3D11Renderer::createIndexBuffer(const RendererIndexBufferDesc& desc)
{
	D3D11RendererIndexBuffer* ib = 0;
	if (m_d3dDevice)
	{
		RENDERER_ASSERT(desc.isValid(), "Invalid Index Buffer Descriptor.");
		if (desc.isValid())
		{
			ib = new D3D11RendererIndexBuffer(*m_d3dDevice, *m_d3dDeviceContext, desc);
		}
	}
	if (ib)
	{
		addResource(*ib);
	}
	return ib;
}

RendererInstanceBuffer* D3D11Renderer::createInstanceBuffer(const RendererInstanceBufferDesc& desc)
{
	D3D11RendererInstanceBuffer* ib = 0;
	if (m_d3dDevice)
	{
		RENDERER_ASSERT(desc.isValid(), "Invalid Instance Buffer Descriptor.");
		if (desc.isValid())
		{
			ib = new D3D11RendererInstanceBuffer(*m_d3dDevice, *m_d3dDeviceContext, desc);
		}
	}
	if (ib)
	{
		addResource(*ib);
	}
	return ib;
}

RendererTexture2D* D3D11Renderer::createTexture2D(const RendererTexture2DDesc& desc)
{
	D3D11RendererTexture2D* texture = 0;
	if (m_d3dDevice)
	{
		RENDERER_ASSERT(desc.isValid(), "Invalid Texture 2D Descriptor.");
		if (desc.isValid())
		{
			texture = new D3D11RendererTexture2D(*m_d3dDevice, *m_d3dDeviceContext, desc);
		}
	}
	if (texture)
	{
		addResource(*texture);
	}
	return texture;
}

RendererTexture3D* D3D11Renderer::createTexture3D(const RendererTexture3DDesc& desc)
{
	D3D11RendererTexture3D* texture = 0;
	if (m_d3dDevice)
	{
		RENDERER_ASSERT(desc.isValid(), "Invalid Texture 3D Descriptor.");
		if (desc.isValid())
		{
			texture = new D3D11RendererTexture3D(*m_d3dDevice, *m_d3dDeviceContext, desc);
		}
	}
	if (texture)
	{
		addResource(*texture);
	}
	return texture;
}

RendererTarget* D3D11Renderer::createTarget(const RendererTargetDesc& desc)
{
	RendererTarget* target = 0;
#if defined(RENDERER_ENABLE_DIRECT3D11_TARGET)
	D3D11RendererTarget* d3dTarget = 0;
	RENDERER_ASSERT(desc.isValid(), "Invalid Target Descriptor.");
	if (desc.isValid())
	{
		d3dTarget = new D3D11RendererTarget(*m_d3dDevice, *m_d3dDeviceContext, desc);
	}
	if (d3dTarget)
	{
		addResource(*d3dTarget);
	}
	target = d3dTarget;
#endif
	return target;
}

RendererMaterial* D3D11Renderer::createMaterial(const RendererMaterialDesc& desc)
{
	RendererMaterial* mat = hasMaterialAlready(desc);
	RENDERER_ASSERT(desc.isValid(), "Invalid Material Descriptor.");
	if (!mat && desc.isValid())
	{
		mat = new D3D11RendererMaterial(*this, desc);

		registerMaterial(desc, mat);
	}
	return mat;
}

RendererMesh* D3D11Renderer::createMesh(const RendererMeshDesc& desc)
{
	D3D11RendererMesh* mesh = 0;
	RENDERER_ASSERT(desc.isValid(), "Invalid Mesh Descriptor.");
	if (desc.isValid())
	{
		mesh = new D3D11RendererMesh(*this, desc);
	}
	return mesh;
}

RendererLight* D3D11Renderer::createLight(const RendererLightDesc& desc)
{
	RendererLight* light = 0;
	if (m_d3dDevice)
	{
		RENDERER_ASSERT(desc.isValid(), "Invalid Light Descriptor.");
		if (desc.isValid())
		{
			switch (desc.type)
			{
			case RendererLight::TYPE_DIRECTIONAL:
				light = new D3D11RendererDirectionalLight(*this, *(RendererDirectionalLightDesc*)&desc);
				break;
			case RendererLight::TYPE_SPOT:
				light = new D3D11RendererSpotLight(*this, *(RendererSpotLightDesc*)&desc);
				break;
			default:
				RENDERER_ASSERT(0, "Not implemented!");
			}
		}
	}
	return light;
}

void D3D11Renderer::setVsync(bool on)
{
	m_vsync = on;
}

bool D3D11Renderer::beginRender(void)
{
	setBlendState();
	
	setRasterizerState();

	setTessellationState();

	m_environment.reset();
	m_currentTextMesh = 0;
	return true;
}

void D3D11Renderer::endRender(void)
{

}

void D3D11Renderer::bindViewProj(const physx::PxMat44& eye, const RendererProjection& proj)
{
	m_viewMatrix = eye.inverseRT();

	convertToD3D11(m_environment.viewMatrix, m_viewMatrix);
	convertToD3D11(m_environment.invViewProjMatrix, eye);
	convertToD3D11(m_environment.projMatrix, proj);

	m_environment.eyePosition = eye.getPosition();
	m_environment.eyeDirection = -eye.getBasis(2);

	m_environment.bindFrame();
}

void D3D11Renderer::bindFogState(const RendererColor& fogColor, float fogDistance)
{
	const float inv255 = 1.0f / 255.0f;

	m_environment.fogColorAndDistance.x = fogColor.r * inv255;
	m_environment.fogColorAndDistance.y = fogColor.g * inv255;
	m_environment.fogColorAndDistance.z = fogColor.b * inv255;
	m_environment.fogColorAndDistance.w = fogDistance;
	m_environment.bindFogState();
}

void D3D11Renderer::bindAmbientState(const RendererColor& ambientColor)
{
	convertToD3D11(m_environment.ambientColor, ambientColor);
	m_environment.bindAmbientState();
}

void D3D11Renderer::bindDeferredState(void)
{
	RENDERER_ASSERT(0, "Not implemented!");
}

void D3D11Renderer::bindMeshContext(const RendererMeshContext& context)
{
	physx::PxMat44 model;
	physx::PxMat44 modelView;
	if(context.transform) model = *context.transform;
	else                  model = PxMat44(PxIdentity);
	modelView = m_viewMatrix * model;

	convertToD3D11(m_environment.modelMatrix,     model);
	convertToD3D11(m_environment.modelViewMatrix, modelView);
	m_environment.bindModel();

	
	// it appears that D3D winding is backwards, so reverse them...
	D3D11_CULL_MODE cullMode = D3D11_CULL_BACK;
	switch (context.cullMode)
	{
	case RendererMeshContext::CLOCKWISE:
		cullMode = context.negativeScale ? D3D11_CULL_BACK : D3D11_CULL_FRONT;
		break;
	case RendererMeshContext::COUNTER_CLOCKWISE:
		cullMode = context.negativeScale ? D3D11_CULL_FRONT : D3D11_CULL_BACK;
		break;
	case RendererMeshContext::NONE:
		cullMode = D3D11_CULL_NONE;
		break;
	default:
		RENDERER_ASSERT(0, "Invalid Cull Mode");
	}
	if (!blendingCull() && NULL != context.material && context.material->getBlending())
		cullMode = D3D11_CULL_NONE;

	D3D11_FILL_MODE fillMode = D3D11_FILL_SOLID;
	switch (context.fillMode)
	{
	case RendererMeshContext::SOLID:
		fillMode = D3D11_FILL_SOLID;
		break;
	case RendererMeshContext::LINE:
		fillMode = D3D11_FILL_WIREFRAME;
		break;
	case RendererMeshContext::POINT:
		fillMode = D3D11_FILL_SOLID;
		break;
	default:
		RENDERER_ASSERT(0, "Invalid Fill Mode");
	}
	if (wireframeEnabled()) 
	{
		fillMode = D3D11_FILL_WIREFRAME;
	}

	setRasterizerState(fillMode, cullMode, get<2>(m_boundRasterizerStateKey));

	RENDERER_ASSERT(context.numBones <= RENDERER_MAX_BONES, "Too many bones.");
	if (context.boneMatrices && context.numBones > 0 && context.numBones <= RENDERER_MAX_BONES)
	{
		for (PxU32 i = 0; i < context.numBones; ++i)
		{
			convertToD3D11(m_environment.boneMatrices[i], context.boneMatrices[i]);
		}
		m_environment.numBones = context.numBones;
		m_environment.bindBones();
	}
}

void D3D11Renderer::setRasterizerState(D3D11_FILL_MODE fillMode, D3D11_CULL_MODE cullMode, PxI32 depthBias)
{
	
	D3DTraits<ID3D11RasterizerState>::key_type rasterizerStateKey(fillMode, cullMode, depthBias);

	ID3D11RasterizerState* pRasterizerState = 
		getResourceManager()->hasResource<ID3D11RasterizerState>(rasterizerStateKey);
	if (NULL == pRasterizerState)
	{
		RENDERER_ASSERT(m_d3dDevice, "Invalid D3D11 device.");
		D3D11_RASTERIZER_DESC rasterizerDesc =
		{
			fillMode,               // D3D11_FILL_MODE FillMode;
			cullMode,               // D3D11_CULL_MODE CullMode;
			TRUE,                   // BOOL FrontCounterClockwise;
			0,                      // INT DepthBias;
			.05f * depthBias,       // FLOAT DepthBiasClamp;
			.05f * depthBias,       // FLOAT SlopeScaledDepthBias;
			FALSE,                  // BOOL DepthClipEnable;
			FALSE,                  // BOOL ScissorEnable;
			(fillMode ==  D3D11_FILL_WIREFRAME) ? FALSE : TRUE,                   // BOOL MultisampleEnable;
			FALSE,                   // BOOL AntialiasedLineEnable;
		};
		if(getFeatureLevel() <= D3D_FEATURE_LEVEL_9_3)
		{
			rasterizerDesc.DepthClipEnable = TRUE;
		}
		HRESULT result = m_d3dDevice->CreateRasterizerState(&rasterizerDesc, &pRasterizerState);
		RENDERER_ASSERT(SUCCEEDED(result) && pRasterizerState, "Error creating D3D11 rasterizer state.");
		if (SUCCEEDED(result))
		{
			getResourceManager()->registerResource<ID3D11RasterizerState>(rasterizerStateKey, pRasterizerState);
		}
	}
	RENDERER_ASSERT(pRasterizerState, "Invalid D3D11RasterizerState");
	if (pRasterizerState && m_d3dDeviceContext)
	{
		m_d3dDeviceContext->RSSetState(pRasterizerState);
		if (get<1>(m_boundRasterizerStateKey) != get<1>(rasterizerStateKey))
		{
			m_environment.vfs = (get<1>(rasterizerStateKey) == D3D11_CULL_BACK) ? 1.f : -1.f;
			m_environment.bindVFaceScale();
		}
		m_boundRasterizerStateKey = rasterizerStateKey;
	}
}


void SampleRenderer::D3D11Renderer::setDepthStencilState(DepthStencilState state)
{
	if (!m_d3dDeviceContext || state >= NUM_DEPTH_STENCIL_STATES)
	{
		return;
	}

	// The standard depth-stencil state
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc =
	{
		TRUE,                           // BOOL DepthEnable;
		D3D11_DEPTH_WRITE_MASK_ALL,     // D3D11_DEPTH_WRITE_MASK DepthWriteMask;
		D3D11_COMPARISON_LESS,          // D3D11_COMPARISON_FUNC DepthFunc;
		TRUE,                           // BOOL StencilEnable;
		0xFF,                           // UINT8 StencilReadMask;
		0xFF,                           // UINT8 StencilWriteMask;
		{
			// D3D11_DEPTH_STENCILOP_DESC FrontFace;
			D3D11_STENCIL_OP_KEEP,      // D3D11_STENCIL_OP StencilFailOp;
				D3D11_STENCIL_OP_INCR,      // D3D11_STENCIL_OP StencilDepthFailOp;
				D3D11_STENCIL_OP_KEEP,      // D3D11_STENCIL_OP StencilPassOp;
				D3D11_COMPARISON_ALWAYS,    // D3D11_COMPARISON_FUNC StencilFunc;
		},
		{
			// D3D11_DEPTH_STENCILOP_DESC BackFace;
			D3D11_STENCIL_OP_KEEP,      // D3D11_STENCIL_OP StencilFailOp;
				D3D11_STENCIL_OP_INCR,      // D3D11_STENCIL_OP StencilDepthFailOp;
				D3D11_STENCIL_OP_KEEP,      // D3D11_STENCIL_OP StencilPassOp;
				D3D11_COMPARISON_ALWAYS,    // D3D11_COMPARISON_FUNC StencilFunc;
			},
	};

	if (!m_d3dDepthStencilStates[DEPTHSTENCIL_DEFAULT])
	{
		m_d3dDevice->CreateDepthStencilState(&depthStencilDesc, &m_d3dDepthStencilStates[DEPTHSTENCIL_DEFAULT]);
		RENDERER_ASSERT(m_d3dDepthStencilStates[DEPTHSTENCIL_DEFAULT], "Error creating D3D depth stencil state");
	}
	if (!m_d3dDepthStencilStates[DEPTHSTENCIL_TRANSPARENT])
	{
		depthStencilDesc.DepthWriteMask   = D3D11_DEPTH_WRITE_MASK_ZERO;
		depthStencilDesc.DepthFunc        = D3D11_COMPARISON_LESS_EQUAL;
		depthStencilDesc.StencilWriteMask = 0x00;
		m_d3dDevice->CreateDepthStencilState(&depthStencilDesc, &m_d3dDepthStencilStates[DEPTHSTENCIL_TRANSPARENT]);
		RENDERER_ASSERT(m_d3dDepthStencilStates[DEPTHSTENCIL_TRANSPARENT], "Error creating D3D depth stencil state");
	}
	if (!m_d3dDepthStencilStates[DEPTHSTENCIL_DISABLED])
	{
		depthStencilDesc.DepthEnable      = FALSE;
		depthStencilDesc.DepthWriteMask   = D3D11_DEPTH_WRITE_MASK_ZERO;
		depthStencilDesc.DepthFunc        = D3D11_COMPARISON_NEVER;
		depthStencilDesc.StencilEnable    = FALSE;
		depthStencilDesc.StencilReadMask  = 0x00;
		depthStencilDesc.StencilWriteMask = 0x00;
		m_d3dDevice->CreateDepthStencilState(&depthStencilDesc, &m_d3dDepthStencilStates[DEPTHSTENCIL_DISABLED]);
		RENDERER_ASSERT(m_d3dDepthStencilStates[DEPTHSTENCIL_DISABLED], "Error creating D3D depth stencil state");
	}

	m_d3dDeviceContext->OMSetDepthStencilState(m_d3dDepthStencilStates[state], 1);
}

void SampleRenderer::D3D11Renderer::setBlendState(BlendState state)
{
	if (!m_d3dDeviceContext || state >= NUM_BLEND_STATES)
	{
		return;
	}

	// BLEND_DEFAULT is purposefully NULL
	if (!m_d3dBlendStates[BLEND_MULTIPASS] && m_d3dDevice)
	{
		D3D11_BLEND_DESC blendDesc;
		memset(&blendDesc, 0, sizeof(blendDesc));
		if(getFeatureLevel() <= D3D_FEATURE_LEVEL_9_3)
		{
			blendDesc.AlphaToCoverageEnable                 = FALSE;
			blendDesc.IndependentBlendEnable                = FALSE;
		}
		else
		{
			blendDesc.AlphaToCoverageEnable                 = TRUE;
			blendDesc.IndependentBlendEnable                = TRUE;
		}
		blendDesc.RenderTarget[0].BlendEnable           = TRUE;
		blendDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		m_d3dDevice->CreateBlendState(&blendDesc, &m_d3dBlendStates[BLEND_MULTIPASS]);
		RENDERER_ASSERT(m_d3dBlendStates[BLEND_MULTIPASS], "Error creating D3D blend state");
	} 
	if (!m_d3dBlendStates[BLEND_TRANSPARENT_ALPHA_COVERAGE] && m_d3dDevice)
	{
		D3D11_BLEND_DESC blendDesc;
		memset(&blendDesc, 0, sizeof(blendDesc));
		if(getFeatureLevel() <= D3D_FEATURE_LEVEL_9_3)
		{
			blendDesc.AlphaToCoverageEnable                 = FALSE;
			blendDesc.IndependentBlendEnable                = FALSE;
		}
		else
		{
			blendDesc.AlphaToCoverageEnable                 = TRUE;
			blendDesc.IndependentBlendEnable                = TRUE;
		}
		blendDesc.RenderTarget[0].BlendEnable           = TRUE;
		blendDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		m_d3dDevice->CreateBlendState(&blendDesc, &m_d3dBlendStates[BLEND_TRANSPARENT_ALPHA_COVERAGE]);
		RENDERER_ASSERT(m_d3dBlendStates[BLEND_TRANSPARENT_ALPHA_COVERAGE], "Error creating D3D blend state");
	} 
	if (!m_d3dBlendStates[BLEND_TRANSPARENT] && m_d3dDevice)
	{
		D3D11_BLEND_DESC blendDesc;
		memset(&blendDesc, 0, sizeof(blendDesc));
		blendDesc.AlphaToCoverageEnable                 = FALSE;
		if(getFeatureLevel() <= D3D_FEATURE_LEVEL_9_3)
		{
			blendDesc.IndependentBlendEnable                = FALSE;
		}
		else
		{
			blendDesc.IndependentBlendEnable                = TRUE;
		}
		blendDesc.RenderTarget[0].BlendEnable           = TRUE;
		blendDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		m_d3dDevice->CreateBlendState(&blendDesc, &m_d3dBlendStates[BLEND_TRANSPARENT]);
		RENDERER_ASSERT(m_d3dBlendStates[BLEND_TRANSPARENT], "Error creating D3D blend state");
	}

	const FLOAT BlendFactor[4] = {0, 0, 0, 0};
	m_d3dDeviceContext->OMSetBlendState(m_d3dBlendStates[state], BlendFactor, 0xffffffff);
}

void SampleRenderer::D3D11Renderer::setTessellationState()
{
	getVariableManager()->setSharedVariable("cbTessellation", "g_tessFactor",             &getTessellationParams().tessFactor);
	getVariableManager()->setSharedVariable("cbTessellation", "g_tessMinMaxDistance",     &getTessellationParams().tessMinMaxDistance);
	getVariableManager()->setSharedVariable("cbTessellation", "g_tessHeightScaleAndBias", &getTessellationParams().tessHeightScaleAndBias);
	getVariableManager()->setSharedVariable("cbTessellation", "g_tessUVScale",            &getTessellationParams().tessUVScale);
}

bool D3D11Renderer::tessellationEnabled() const
{
	return getEnableTessellation();
}

bool D3D11Renderer::isTessellationSupported(void) const 
{ 
	return m_d3dDeviceFeatureLevel >= D3D_FEATURE_LEVEL_11_0; 
}

bool D3D11Renderer::multisamplingEnabled() const
{
	return m_swapChainDesc.SampleDesc.Count > 1 && m_swapChainDesc.SampleDesc.Quality > 0;
}

void D3D11Renderer::beginMultiPass(void)
{
	setBlendState(BLEND_MULTIPASS);
	setDepthStencilState(DEPTHSTENCIL_TRANSPARENT);
	setRasterizerState(get<0>(m_boundRasterizerStateKey), get<1>(m_boundRasterizerStateKey), m_multipassDepthBias);
}

void D3D11Renderer::endMultiPass(void)
{
	setBlendState(BLEND_DEFAULT);
	setDepthStencilState(DEPTHSTENCIL_DEFAULT);
	setRasterizerState(get<0>(m_boundRasterizerStateKey), get<1>(m_boundRasterizerStateKey), 0);
}

void D3D11Renderer::beginTransparentMultiPass(void)
{
	setEnableBlendingOverride(true);
	setBlendState(BLEND_TRANSPARENT_ALPHA_COVERAGE);
	setDepthStencilState(DEPTHSTENCIL_TRANSPARENT);
}

void D3D11Renderer::endTransparentMultiPass(void)
{
	setEnableBlendingOverride(false);
	setBlendState(BLEND_DEFAULT);
	setDepthStencilState(DEPTHSTENCIL_DEFAULT);
}

void D3D11Renderer::renderDeferredLight(const RendererLight& light)
{
	RENDERER_ASSERT(0, "Not implemented!");
}

PxU32 D3D11Renderer::convertColor(const RendererColor& color) const
{
	return color.a << 24 | color.r << 16 | color.g << 8 | color.b;
}

bool D3D11Renderer::isOk(void) const
{
	bool ok = (m_d3d && m_d3dDevice);
#if defined(RENDERER_WINDOWS)
	if (!SampleFramework::SamplePlatform::platform()->isD3D11ok() || !m_d3dx->m_library)
	{
		ok = false;
	}
#endif
	return ok;
}

void D3D11Renderer::addResource(D3D11RendererResource& resource)
{
	RENDERER_ASSERT(resource.m_d3dRenderer == 0, "Resource already in added to the Renderer!");
	if (resource.m_d3dRenderer == 0)
	{
		resource.m_d3dRenderer = this;
		m_resources.push_back(&resource);
	}
}

void D3D11Renderer::removeResource(D3D11RendererResource& resource)
{
	RENDERER_ASSERT(resource.m_d3dRenderer == this, "Resource not part of this Renderer!");
	if (resource.m_d3dRenderer == this)
	{
		resource.m_d3dRenderer = 0;
		const PxU32 numResources  = (PxU32)m_resources.size();
		PxU32       foundResource = numResources;
		for (PxU32 i = 0; i < numResources; ++i)
		{
			if (m_resources[i] == &resource)
			{
				foundResource = i;
				break;
			}
		}
		if (foundResource < numResources)
		{
			m_resources[foundResource] = m_resources.back();
			m_resources.pop_back();
		}
	}
}

void D3D11Renderer::notifyResourcesLostDevice(void)
{
	const PxU32 numResources  = (PxU32)m_resources.size();
	for (PxU32 i = 0; i < numResources; ++i)
	{
		m_resources[i]->onDeviceLost();
	}
}

void D3D11Renderer::notifyResourcesResetDevice(void)
{
	const PxU32 numResources  = (PxU32)m_resources.size();
	for (PxU32 i = 0; i < numResources; ++i)	{
		m_resources[i]->onDeviceReset();
	}
}

void D3D11Renderer::bind(RendererMaterialInstance* materialInstance)
{
	m_boundMaterial = materialInstance;
#if RENDERER_ENABLE_SHADOWS
	if (m_boundMaterial && m_environment.lightShadowMap)
	{
		const RendererMaterial::Variable* pShadowMap = m_boundMaterial->findVariable("g_lightShadowMap", RendererMaterial::VARIABLE_SAMPLER2D);
		if (pShadowMap)
		{
			m_boundMaterial->writeData(*pShadowMap, &m_environment.lightShadowMap);
		}
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////

static void pushRasterizer(ID3D11DeviceContext& context)
{
	gRasterizerState.push_back(NULL);
	gViewport.push_back(D3D11_VIEWPORT());
	PxU32 NUM_VIEWPORTS = 1;
	context.RSGetState(&gRasterizerState.back());
	context.RSGetViewports(&NUM_VIEWPORTS, &gViewport.back());
}

static void pushBlend(ID3D11DeviceContext& context)
{
	gBlendState.push_back(NULL);
	gBlendFactor.push_back(PxVec4());
	gBlendMask.push_back(0);
	context.OMGetBlendState(&gBlendState.back(), &gBlendFactor.back()[0], &gBlendMask.back());
}

static void pushDepthStencil(ID3D11DeviceContext& context)
{
	gDepthStencilState.push_back(NULL);
	gDepthStencilMask.push_back(0);
	context.OMGetDepthStencilState(&gDepthStencilState.back(), &gDepthStencilMask.back());
}

static void popRasterizer(ID3D11DeviceContext& context)
{
	if (!gRasterizerState.empty())
	{
		context.RSSetState(gRasterizerState.back());
		dxSafeRelease(gRasterizerState.back());
		gRasterizerState.pop_back();
	}
	if (!gViewport.empty())
	{
		context.RSSetViewports(1, &gViewport.back());
		gViewport.pop_back();
	}
}

static void popBlend(ID3D11DeviceContext& context)
{
	if (!gBlendState.empty())
	{
		context.OMSetBlendState(gBlendState.back(), &gBlendFactor.back()[0], gBlendMask.back());
		dxSafeRelease(gBlendState.back());
		gBlendState.pop_back();
		gBlendFactor.pop_back();
		gBlendMask.pop_back();
	}
}

static void popDepthStencil(ID3D11DeviceContext& context)
{
	if (!gDepthStencilState.empty())
	{
		context.OMSetDepthStencilState(gDepthStencilState.back(), gDepthStencilMask.back());
		dxSafeRelease(gDepthStencilState.back());
		gDepthStencilState.pop_back();
		gDepthStencilMask.pop_back();
	}
}

void D3D11Renderer::pushState(StateType stateType)
{
	if (m_d3dDeviceContext)
	{
		switch (stateType)
		{
		case STATE_BLEND:
			pushBlend(*m_d3dDeviceContext);
			break;
		case STATE_RASTERIZER:
			pushRasterizer(*m_d3dDeviceContext);
			break;
		case STATE_DEPTHSTENCIL:
			pushDepthStencil(*m_d3dDeviceContext);
			break;
		default:
			break;
		}
	}
}

void D3D11Renderer::popState(StateType stateType)
{
	if (m_d3dDeviceContext)
	{
		switch (stateType)
		{
		case STATE_BLEND:
			popBlend(*m_d3dDeviceContext);
			break;
		case STATE_RASTERIZER:
			popRasterizer(*m_d3dDeviceContext);
			break;
		case STATE_DEPTHSTENCIL:
			popDepthStencil(*m_d3dDeviceContext);
			break;
		default:
			break;
		}
	}
}

bool D3D11Renderer::initTexter()
{
	if (!Renderer::initTexter() || !m_d3dDevice)
	{
		return false;
	}

	return true;
}

void D3D11Renderer::closeTexter()
{
	Renderer::closeTexter();
}

void D3D11Renderer::setupTextRenderStates()
{
	if (m_d3dDeviceContext)
	{
		pushState(STATE_BLEND);
		pushState(STATE_DEPTHSTENCIL);
		pushState(STATE_RASTERIZER);

		setBlendState(BLEND_TRANSPARENT);
		setDepthStencilState(DEPTHSTENCIL_DISABLED);
		setRasterizerState(D3D11_FILL_SOLID, D3D11_CULL_NONE);
	}
}

void D3D11Renderer::resetTextRenderStates()
{
	if (m_d3dDeviceContext)
	{
		popState(STATE_RASTERIZER);
		popState(STATE_DEPTHSTENCIL);
		popState(STATE_BLEND);
	}
}

void D3D11Renderer::renderTextBuffer(const void* vertices, PxU32 nbVerts, const PxU16* indices, PxU32 nbIndices, RendererMaterial* material)
{
	PX_UNUSED(material);
	// PT: font texture must have been selected prior to calling this function
	if (m_d3dDeviceContext && m_d3dDevice && m_boundMaterial)
	{
		const PxU32 minMeshSize = m_currentTextMesh + 1;
		if ((m_currentTextMesh + 1) > m_textVertices.size() || m_textVertices[m_currentTextMesh]->getMaxVertices() < nbVerts)
		{
			resizeIfSmallerThan(m_textVertices, minMeshSize, NULL);
			resizeIfSmallerThan(m_textMeshes,   minMeshSize, NULL);

			deleteSafe(m_textVertices[m_currentTextMesh]);
			deleteSafe(m_textMeshes[m_currentTextMesh]);

			RendererVertexBufferDesc desc;
			desc.maxVertices = nbVerts * 3;
			desc.semanticFormats[RendererVertexBuffer::SEMANTIC_POSITION] = RendererVertexBuffer::FORMAT_FLOAT4;
			desc.semanticFormats[RendererVertexBuffer::SEMANTIC_COLOR] = RendererVertexBuffer::FORMAT_COLOR_BGRA;
			desc.semanticFormats[RendererVertexBuffer::SEMANTIC_TEXCOORD0] = RendererVertexBuffer::FORMAT_FLOAT2;
			m_textVertices[m_currentTextMesh] = new D3D11RendererVertexBuffer(*m_d3dDevice, *m_d3dDeviceContext, desc, false);
		}
		if ((m_currentTextMesh + 1) > m_textIndices.size() || m_textIndices[m_currentTextMesh]->getMaxIndices() < nbIndices)
		{
			resizeIfSmallerThan(m_textIndices, minMeshSize, NULL);
			resizeIfSmallerThan(m_textMeshes,  minMeshSize, NULL);

			deleteSafe(m_textIndices[m_currentTextMesh]);
			deleteSafe(m_textMeshes[m_currentTextMesh]);

			RendererIndexBufferDesc desc;
			desc.maxIndices = nbIndices * 3;
			desc.hint       = RendererIndexBuffer::HINT_DYNAMIC;
			desc.format     = RendererIndexBuffer::FORMAT_UINT16;
			m_textIndices[m_currentTextMesh]   = new D3D11RendererIndexBuffer(*m_d3dDevice, *m_d3dDeviceContext, desc);
		}
		if ((m_currentTextMesh + 1) > m_textMeshes.size() || NULL == m_textMeshes[m_currentTextMesh] && m_textIndices[m_currentTextMesh] && m_textVertices[m_currentTextMesh])
		{
			resizeIfSmallerThan(m_textMeshes, minMeshSize, NULL);

			RendererMeshDesc meshdesc;
			meshdesc.primitives       = RendererMesh::PRIMITIVE_TRIANGLES;
			RendererVertexBuffer* vertexBuffer = m_textVertices[m_currentTextMesh];
			meshdesc.vertexBuffers    = &vertexBuffer;
			meshdesc.numVertexBuffers = 1;
			meshdesc.firstVertex      = 0;
			meshdesc.numVertices      = nbVerts;
			meshdesc.indexBuffer      = m_textIndices[m_currentTextMesh];
			meshdesc.firstIndex       = 0;
			meshdesc.numIndices       = nbIndices;
			m_textMeshes[m_currentTextMesh] = new D3D11RendererMesh(*this, meshdesc);
		}

		if (m_textMeshes[m_currentTextMesh])
		{
			// Assign the vertex transform and bind to the pipeline
			const RendererMaterial::Variable* pViewVariable = m_boundMaterial->findVariable("g_viewMatrix2D", RendererMaterial::VARIABLE_FLOAT4x4);
			RENDERER_ASSERT(pViewVariable, "Unable to locate view matrix variable in text vertex shader.");
			if (pViewVariable)
			{
				PxMat44 viewMatrix	(PxVec4(2.f / m_displayWidth   , 0,  0, -1),
				                     PxVec4(0, -2.f / m_displayHeight, 0,  1),
				                     PxVec4(0                    , 0,  1,  0),
									 PxVec4(0                    , 0,  0,  1));
				m_boundMaterial->writeData(*pViewVariable, &viewMatrix);
				static_cast<D3D11RendererMaterial&>(m_boundMaterial->getMaterial()).bindMeshState(false);
			}
			memcpy(m_textVertices[m_currentTextMesh]->internalLock(D3D11_MAP_WRITE_DISCARD), vertices, nbVerts * sizeof(TextVertex));
			m_textVertices[m_currentTextMesh]->unlock();
			memcpy(m_textIndices[m_currentTextMesh]->internalLock(D3D11_MAP_WRITE_DISCARD),  indices,  nbIndices * sizeof(PxU16));
			m_textIndices[m_currentTextMesh]->unlock();

			m_textMeshes[m_currentTextMesh]->setNumVerticesAndIndices(nbVerts, nbIndices);
			m_textMeshes[m_currentTextMesh]->bind();
			m_textMeshes[m_currentTextMesh]->render(&m_boundMaterial->getMaterial());
			++m_currentTextMesh;
		}
	}
}

bool D3D11Renderer::isSpriteRenderingSupported(void) const
{
	if(getFeatureLevel() <= D3D_FEATURE_LEVEL_9_3)
	{
		return false;
	}
	else
	{
		return true;
	}
}

void D3D11Renderer::renderLines2D(const void* vertices, PxU32 nbVerts)
{
	if (m_d3dDeviceContext && m_d3dDevice && m_boundMaterial)
	{
		if (!m_linesVertices || m_linesVertices->getMaxVertices() < nbVerts)
		{
			deleteSafe(m_linesVertices);
			deleteSafe(m_linesMesh);

			RendererVertexBufferDesc desc;
			desc.maxVertices = nbVerts * 3;
			desc.semanticFormats[RendererVertexBuffer::SEMANTIC_POSITION] = RendererVertexBuffer::FORMAT_FLOAT4;
			desc.semanticFormats[RendererVertexBuffer::SEMANTIC_COLOR] = RendererVertexBuffer::FORMAT_COLOR_BGRA;
			desc.semanticFormats[RendererVertexBuffer::SEMANTIC_TEXCOORD0] = RendererVertexBuffer::FORMAT_FLOAT2;
			m_linesVertices = new D3D11RendererVertexBuffer(*m_d3dDevice, *m_d3dDeviceContext, desc, false);
		}
		if (!m_linesMesh && m_linesVertices)
		{
			RendererMeshDesc meshdesc;
			meshdesc.primitives                = RendererMesh::PRIMITIVE_LINE_STRIP;
			RendererVertexBuffer* vertexBuffer = m_linesVertices;
			meshdesc.vertexBuffers             = &vertexBuffer;
			meshdesc.numVertexBuffers          = 1;
			meshdesc.firstVertex               = 0;
			meshdesc.numVertices               = nbVerts;
			m_linesMesh                        = new D3D11RendererMesh(*this, meshdesc);
		}

		if (m_linesMesh)
		{
			const RendererMaterial::Variable* pViewVariable = m_boundMaterial->findVariable("g_viewMatrix2D", RendererMaterial::VARIABLE_FLOAT4x4);
			if (pViewVariable)
			{
				PxMat44 viewMatrix	(PxVec4(2.f / m_displayWidth   , 0,  0, -1),
				                     PxVec4(0, -2.f / m_displayHeight, 0,  1),
				                     PxVec4(0                    , 0,  1,  0),
									 PxVec4(0                    , 0,  0,  1));
				m_boundMaterial->writeData(*pViewVariable, &viewMatrix);
				static_cast<D3D11RendererMaterial&>(m_boundMaterial->getMaterial()).bindMeshState(false);
			}
			memcpy(m_linesVertices->internalLock(D3D11_MAP_WRITE_DISCARD), vertices, nbVerts * sizeof(TextVertex));
			m_linesVertices->unlock();

			m_linesMesh->setNumVerticesAndIndices(nbVerts, 0);
			m_linesMesh->bind();
			m_linesMesh->render(&m_boundMaterial->getMaterial());
		}
	}
}

void D3D11Renderer::setupScreenquadRenderStates()
{
	if (m_d3dDeviceContext)
	{
		pushState(STATE_DEPTHSTENCIL);
		pushState(STATE_RASTERIZER);
		pushState(STATE_BLEND);

		setDepthStencilState(DEPTHSTENCIL_DISABLED);
		setRasterizerState(D3D11_FILL_SOLID, D3D11_CULL_NONE);
		setBlendState(BLEND_TRANSPARENT);
	}
}

void D3D11Renderer::resetScreenquadRenderStates()
{
	if (m_d3dDeviceContext)
	{
		popState(STATE_DEPTHSTENCIL);
		popState(STATE_RASTERIZER);
		popState(STATE_BLEND);
	}
}

bool D3D11Renderer::captureScreen( PxU32 &width, PxU32& height, PxU32& sizeInBytes, const void*& screenshotData )
{
	bool bSuccess = false;

	HRESULT hr;
	ID3D11Resource *backbufferRes;
	m_d3dRenderTargetView->GetResource(&backbufferRes);

	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.ArraySize          = 1;
	texDesc.BindFlags          = 0;
	texDesc.CPUAccessFlags     = 0;
	texDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Width              = m_displayWidth;
	texDesc.Height             = m_displayHeight;
	texDesc.MipLevels          = 1;
	texDesc.MiscFlags          = 0;
	texDesc.SampleDesc.Count   = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage              = D3D11_USAGE_DEFAULT;

	ID3D11Texture2D *texture = NULL;
	hr = m_d3dDevice->CreateTexture2D(&texDesc, 0, &texture);

	if (SUCCEEDED(hr))
	{
		dxSafeRelease(m_displayBuffer);
		m_d3dDeviceContext->CopyResource(texture, backbufferRes);
		hr = m_d3dx->saveTextureToMemory(m_d3dDeviceContext, texture, D3DX11_IFF_BMP, &m_displayBuffer, 0);
	}

	dxSafeRelease(texture);
	dxSafeRelease(backbufferRes);

	if(SUCCEEDED(hr))
	{
		getWindowSize(width, height);
		sizeInBytes    = (physx::PxU32)m_displayBuffer->GetBufferSize();
		screenshotData = m_displayBuffer->GetBufferPointer();
		bSuccess       = true;
	}
	return bSuccess;
}


#endif // #if defined(RENDERER_ENABLE_DIRECT3D11)
