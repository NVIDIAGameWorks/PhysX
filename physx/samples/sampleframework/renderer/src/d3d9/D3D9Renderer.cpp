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

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include "D3D9Renderer.h"

#include <RendererDesc.h>

#include <RendererProjection.h>

#include <RendererVertexBufferDesc.h>
#include "D3D9RendererVertexBuffer.h"

#include <RendererIndexBufferDesc.h>
#include "D3D9RendererIndexBuffer.h"

#include <RendererInstanceBufferDesc.h>
#include "D3D9RendererInstanceBuffer.h"

#include <RendererMeshDesc.h>
#include <RendererMeshContext.h>
#include "D3D9RendererMesh.h"

#include <RendererMaterialDesc.h>
#include <RendererMaterialInstance.h>
#include "D3D9RendererMaterial.h"

#include <RendererLightDesc.h>
#include <RendererDirectionalLightDesc.h>
#include "D3D9RendererDirectionalLight.h"
#include <RendererSpotLightDesc.h>
#include "D3D9RendererSpotLight.h"

#include <RendererTexture2DDesc.h>
#include "D3D9RendererTexture2D.h"

#include <RendererTargetDesc.h>
#include "D3D9RendererTarget.h"

#include <SamplePlatform.h>

using namespace SampleRenderer;

void SampleRenderer::convertToD3D9(D3DCOLOR &dxcolor, const RendererColor &color)
{
	const float inv255 = 1.0f / 255.0f;
	dxcolor = D3DXCOLOR(color.r*inv255, color.g*inv255, color.b*inv255, color.a*inv255);
}

void SampleRenderer::convertToD3D9(float *dxvec, const PxVec3 &vec)
{
	dxvec[0] = vec.x;
	dxvec[1] = vec.y;
	dxvec[2] = vec.z;
}

void SampleRenderer::convertToD3D9(D3DMATRIX &dxmat, const physx::PxMat44 &mat)
{
	PxMat44 mat44 = mat.getTranspose();
	memcpy(&dxmat._11, mat44.front(), 4 * 4 * sizeof (float));
}

void SampleRenderer::convertToD3D9(D3DMATRIX &dxmat, const RendererProjection &mat)
{
	float temp[16];
	mat.getColumnMajor44(temp);
	for(PxU32 r=0; r<4; r++)
		for(PxU32 c=0; c<4; c++)
		{
			dxmat.m[r][c] = temp[c*4+r];
		}
}

/******************************
* D3D9Renderer::D3DXInterface *
******************************/ 

D3D9Renderer::D3DXInterface::D3DXInterface(void)
{
	memset(this, 0, sizeof(*this));
#if defined(RENDERER_WINDOWS)
#define D3DX_DLL "d3dx9_" RENDERER_TEXT2(D3DX_SDK_VERSION) ".dll"

	m_library = LoadLibraryA(D3DX_DLL);
	if(!m_library)
	{
		MessageBoxA(0, "Unable to load " D3DX_DLL ". Please install the latest DirectX End User Runtime available at www.microsoft.com/directx.", "Renderer Error.", MB_OK);
	}
	if(m_library)
	{
#define FIND_D3DX_FUNCTION(_name)                             \
m_##_name = (LP##_name)GetProcAddress(m_library, #_name); \
RENDERER_ASSERT(m_##_name, "Unable to find D3DX9 Function " #_name " in " D3DX_DLL ".");

		FIND_D3DX_FUNCTION(D3DXCompileShaderFromFileA)
		FIND_D3DX_FUNCTION(D3DXGetVertexShaderProfile)
		FIND_D3DX_FUNCTION(D3DXGetPixelShaderProfile)
		FIND_D3DX_FUNCTION(D3DXSaveSurfaceToFileInMemory)
		FIND_D3DX_FUNCTION(D3DXCreateBuffer)
		FIND_D3DX_FUNCTION(D3DXSaveSurfaceToFileA)
		FIND_D3DX_FUNCTION(D3DXGetShaderConstantTable)

#undef FIND_D3DX_FUNCTION
	}

#define D3DCOMPILER_DLL "D3DCompiler_" RENDERER_TEXT2(D3DX_SDK_VERSION) ".dll"
	m_compiler_library = LoadLibraryA(D3DCOMPILER_DLL);
	if(!m_library)
	{
		MessageBoxA(0, "Unable to load " D3DCOMPILER_DLL ". Please install the latest DirectX End User Runtime available at www.microsoft.com/directx.", "Renderer Error.", MB_OK);
	}


#undef D3DX_DLL
#endif
}

D3D9Renderer::D3DXInterface::~D3DXInterface(void)
{
#if defined(RENDERER_WINDOWS)
	if(m_library)
	{
		FreeLibrary(m_library);
		m_library = 0;
		FreeLibrary(m_compiler_library);
		m_compiler_library = 0;
	}
#endif
}

#if defined(RENDERER_WINDOWS)
#define CALL_D3DX_FUNCTION(_name, _params)   if(m_##_name) result = m_##_name _params;
#else
#define CALL_D3DX_FUNCTION(_name, _params)   result = _name _params;
#endif

HRESULT D3D9Renderer::D3DXInterface::CompileShaderFromFileA(LPCSTR srcFile, CONST D3DXMACRO *defines, LPD3DXINCLUDE include, LPCSTR functionName, LPCSTR profile, DWORD flags, LPD3DXBUFFER *shader, LPD3DXBUFFER *errorMsgs, LPD3DXCONSTANTTABLE *constantTable)
{

	HRESULT result = D3DERR_NOTAVAILABLE;
	CALL_D3DX_FUNCTION(D3DXCompileShaderFromFileA, (srcFile, defines, include, functionName, profile, flags, shader, errorMsgs, constantTable));
	return result;

}

HRESULT D3D9Renderer::D3DXInterface::GetShaderConstantTable(const DWORD* pFunction, LPD3DXCONSTANTTABLE *constantTable)
{
	HRESULT result = D3DERR_NOTAVAILABLE;
	CALL_D3DX_FUNCTION(D3DXGetShaderConstantTable, (pFunction, constantTable));
	return result;
}

HRESULT D3D9Renderer::D3DXInterface::SaveSurfaceToFileA( LPCSTR pDestFile, D3DXIMAGE_FILEFORMAT DestFormat, LPDIRECT3DSURFACE9 pSrcSurface, CONST PALETTEENTRY* pSrcPalette, CONST RECT*  pSrcRect)
{

	HRESULT result = D3DERR_NOTAVAILABLE;
	CALL_D3DX_FUNCTION(D3DXSaveSurfaceToFileA, (pDestFile, DestFormat, pSrcSurface, pSrcPalette, pSrcRect));
	return result;

}

LPCSTR D3D9Renderer::D3DXInterface::GetVertexShaderProfile(LPDIRECT3DDEVICE9 device)
{

	LPCSTR result = 0;
	CALL_D3DX_FUNCTION(D3DXGetVertexShaderProfile, (device));
	return result;

}

LPCSTR D3D9Renderer::D3DXInterface::GetPixelShaderProfile(LPDIRECT3DDEVICE9 device)
{
	LPCSTR result = 0;
	CALL_D3DX_FUNCTION(D3DXGetPixelShaderProfile, (device));
	return result;
}

HRESULT D3D9Renderer::D3DXInterface::SaveSurfaceToFileInMemory(LPD3DXBUFFER *ppDestBuf, D3DXIMAGE_FILEFORMAT DestFormat, LPDIRECT3DSURFACE9 pSrcSurface, const PALETTEENTRY *pSrcPalette, const RECT *pSrcRect)
{
	HRESULT result = D3DERR_NOTAVAILABLE;
	CALL_D3DX_FUNCTION(D3DXSaveSurfaceToFileInMemory, (ppDestBuf, DestFormat, pSrcSurface, pSrcPalette, pSrcRect));
	return result;
}

HRESULT D3D9Renderer::D3DXInterface::CreateBuffer(DWORD NumBytes, LPD3DXBUFFER *ppBuffer)
{
	HRESULT result = D3DERR_NOTAVAILABLE;
	CALL_D3DX_FUNCTION(D3DXCreateBuffer, (NumBytes, ppBuffer));
	return result;
}


#undef CALL_D3DX_FUNCTION

/**********************************
* D3D9Renderer::ShaderEnvironment *
**********************************/ 

D3D9Renderer::ShaderEnvironment::ShaderEnvironment(void)
{
	memset(this, 0, sizeof(*this));
}

/***************
* D3D9Renderer *
***************/ 

D3D9Renderer::D3D9Renderer(IDirect3D9* inDirect3d, const char* devName, PxU32 dispWidth, PxU32 dispHeight, IDirect3DDevice9* inDevice, bool inIsDeviceEx, const char* assetDir)
	: Renderer( DRIVER_DIRECT3D9, NULL, assetDir )
{
	initialize(inIsDeviceEx);
	m_d3d = inDirect3d;
	strcpy_s( m_deviceName, 256, devName );
	m_displayWidth = dispWidth;
	m_displayHeight = dispHeight;
	m_d3dDevice = inDevice;
	//Users must call 
	// checkResize(false);
	// onDeviceReset();
}


void D3D9Renderer::initialize( bool isDeviceEx )
{
	m_textVDecl						= NULL;
	m_useShadersForTextRendering	= true;
	m_pixelCenterOffset      = 0.5f;
	m_displayWidth           = 0;
	m_displayHeight          = 0;
	m_displayBuffer          = 0;
	m_d3d                    = 0;
	m_d3dDevice              = 0;
	m_d3dDepthStencilSurface = 0;
	m_d3dSurface             = 0;
	m_d3dSwapChain           = 0;
	m_d3dSwapDepthStencilSurface = 0;
	m_d3dSwapSurface         = 0;
	m_isDeviceEx			 = isDeviceEx;

	m_viewMatrix				= PxMat44(PxIdentity);
}

D3D9Renderer::D3D9Renderer(const RendererDesc &desc, const char* assetDir) :
Renderer	(DRIVER_DIRECT3D9, desc.errorCallback, assetDir)
{
	initialize(false);
	SampleFramework::SamplePlatform* m_platform = SampleFramework::SamplePlatform::platform();
	m_d3d = static_cast<IDirect3D9*>(m_platform->initializeD3D9());
	RENDERER_ASSERT(m_d3d, "Could not create Direct3D9 Interface.");
	if(m_d3d)
	{
		memset(&m_d3dPresentParams, 0, sizeof(m_d3dPresentParams));
		m_d3dPresentParams.Windowed               = 1;
		m_d3dPresentParams.SwapEffect             = D3DSWAPEFFECT_DISCARD;
		m_d3dPresentParams.BackBufferFormat       = D3DFMT_X8R8G8B8;
		m_d3dPresentParams.EnableAutoDepthStencil = 0;
		m_d3dPresentParams.AutoDepthStencilFormat = D3DFMT_D24S8;
		m_d3dPresentParams.PresentationInterval   = desc.vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;

		HRESULT res = m_platform->initializeD3D9Display(&m_d3dPresentParams, 
														m_deviceName, 
														m_displayWidth, 
														m_displayHeight, 
														&m_d3dDevice);

		RENDERER_ASSERT(res==D3D_OK, "Failed to create Direct3D9 Device.");
		if(res==D3D_OK)
		{
			m_d3dPresentParamsChanged = false;
			checkResize(false);
			onDeviceReset();
		}
	}
}

D3D9Renderer::~D3D9Renderer(void)
{
	assert(!m_textVDecl);
	SampleFramework::SamplePlatform* m_platform = SampleFramework::SamplePlatform::platform();

	releaseAllMaterials();

	releaseSwapchain();

	if(m_d3dDepthStencilSurface)
	{
		m_d3dDevice->SetDepthStencilSurface(NULL);
		m_platform->D3D9BlockUntilNotBusy(m_d3dDepthStencilSurface);
		m_d3dDepthStencilSurface->Release();
	}
	m_platform->D3D9DeviceBlockUntilIdle();
	if(m_d3dDevice)              m_d3dDevice->Release();
	if(m_d3d)                    m_d3d->Release();
	if(m_displayBuffer)          m_displayBuffer->Release();
}

bool D3D9Renderer::checkResize(bool isDeviceLost)
{
	bool isDeviceReset = false;
#if defined(RENDERER_WINDOWS)
	if(SampleFramework::SamplePlatform::platform()->getWindowHandle() && m_d3dDevice)
	{
		PxU32 width  = 0;
		PxU32 height = 0;
		SampleFramework::SamplePlatform::platform()->getWindowSize(width, height);
		if(width && height && (width != m_displayWidth || height != m_displayHeight) || isDeviceLost)
		{
			bool needsReset = (m_displayWidth&&m_displayHeight&&(isDeviceLost||!useSwapchain()) ? true : false);
			m_displayWidth  = width;
			m_displayHeight = height;

			m_d3dPresentParams.BackBufferWidth  = (UINT)m_displayWidth;
			m_d3dPresentParams.BackBufferHeight = (UINT)m_displayHeight;

			D3DVIEWPORT9 viewport = {0};
			m_d3dDevice->GetViewport(&viewport);
			viewport.Width  = (DWORD)m_displayWidth;
			viewport.Height = (DWORD)m_displayHeight;
			viewport.MinZ =  0.0f;
			viewport.MaxZ =  1.0f;

			if(needsReset)
			{
				physx::PxU64 res = m_d3dDevice->TestCooperativeLevel();
				if(res == D3D_OK || res == D3DERR_DEVICENOTRESET)	//if device is lost, device has to be ready for reset
				{
					onDeviceLost();
					isDeviceReset = resetDevice();
					if (isDeviceReset)
					{
						onDeviceReset();
						m_d3dDevice->SetViewport(&viewport);
					}
				}
			}
			else
			{
				if(m_d3dSurface == NULL)
					m_d3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_d3dSurface);
				buildSwapchain();
				m_d3dDevice->SetViewport(&viewport);
			}
		}
	}
#endif
	return isDeviceReset;
}

bool D3D9Renderer::resetDevice(void)
{
	
	HRESULT res = m_d3dDevice->Reset(&m_d3dPresentParams);
	RENDERER_ASSERT(res == D3D_OK, "Failed to reset Direct3D9 Device.");
	if(res == D3D_OK)
	{
		m_d3dPresentParamsChanged = false;
		return true;
	}
	else
	{
		return false;
	}
}

void D3D9Renderer::buildSwapchain(void)
{
	if (!useSwapchain())
		return;

#if RENDERER_ENABLE_DIRECT3D9_SWAPCHAIN
	// Set the DX9 surfaces back to the originals
	m_d3dDevice->SetRenderTarget(0, m_d3dSurface);
	m_d3dDevice->SetDepthStencilSurface(m_d3dDepthStencilSurface);

	// Release the swapchain resources
	releaseSwapchain();

	// Recreate the swapchain resources
	m_d3dDevice->CreateAdditionalSwapChain(&m_d3dPresentParams, &m_d3dSwapChain);
	m_d3dSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &m_d3dSwapSurface);
	m_d3dDevice->CreateDepthStencilSurface(m_displayWidth,m_displayHeight,D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, false, &m_d3dSwapDepthStencilSurface, 0);

	// Re-assign the DX9 surfaces to use the swapchain
	m_d3dDevice->SetRenderTarget(0, m_d3dSwapSurface);
	m_d3dDevice->SetDepthStencilSurface(m_d3dSwapDepthStencilSurface);
#endif
}

void SampleRenderer::D3D9Renderer::releaseSwapchain()
{
#if RENDERER_ENABLE_DIRECT3D9_SWAPCHAIN
	if(m_d3dSwapChain) 
	{
		m_d3dSwapChain->Release();
		m_d3dSwapChain = 0;
	}
	if(m_d3dSwapDepthStencilSurface) 
	{
		m_d3dSwapDepthStencilSurface->Release();
		m_d3dSwapDepthStencilSurface = 0;
	}
	if(m_d3dSwapSurface) 
	{
		m_d3dSwapSurface->Release();
		m_d3dSwapSurface = 0;
	}
#endif
}

bool SampleRenderer::D3D9Renderer::useSwapchain() const
{
	return RENDERER_ENABLE_DIRECT3D9_SWAPCHAIN==1;
}

bool SampleRenderer::D3D9Renderer::validSwapchain() const
{
#if RENDERER_ENABLE_DIRECT3D9_SWAPCHAIN
	return m_d3dSwapChain && m_d3dSwapDepthStencilSurface && m_d3dSwapSurface;
#else
	return false;
#endif
}

HRESULT SampleRenderer::D3D9Renderer::presentSwapchain()
{
#if RENDERER_ENABLE_DIRECT3D9_SWAPCHAIN
	RENDERER_ASSERT(m_d3dSwapChain, "Invalid D3D9 swapchain");
	if(m_d3dSwapChain)
		return m_d3dSwapChain->Present(0, 0, 0, 0, 0);
	else
#endif
		return D3DERR_NOTAVAILABLE;
}

void D3D9Renderer::releaseDepthStencilSurface(void)
{
	if(m_d3dDepthStencilSurface)
	{
		m_d3dDepthStencilSurface->Release();
		m_d3dDepthStencilSurface = 0;
	}
}

void D3D9Renderer::onDeviceLost(void)
{
	notifyResourcesLostDevice();
	if(m_d3dDepthStencilSurface)
	{
		m_d3dDepthStencilSurface->Release();
		m_d3dDepthStencilSurface = 0;
	}
	if (m_d3dSurface)
	{
		m_d3dSurface->Release();
		m_d3dSurface = 0;
	}
	releaseSwapchain();
}

void D3D9Renderer::onDeviceReset(void)
{
	if(m_d3dDevice)
	{
		// set out initial states...
		m_d3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
#if defined(RENDERER_WINDOWS)
		m_d3dDevice->SetRenderState(D3DRS_LIGHTING, 0);
#endif
		m_d3dDevice->SetRenderState(D3DRS_ZENABLE,  1);
		m_d3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_d3dSurface);
		buildDepthStencilSurface();
		buildSwapchain();
	}
	notifyResourcesResetDevice();
}

void D3D9Renderer::buildDepthStencilSurface(void)
{
	if(m_d3dDevice)
	{
		PxU32 width  = m_displayWidth;
		PxU32 height = m_displayHeight;
		if(m_d3dDepthStencilSurface)
		{
			D3DSURFACE_DESC dssdesc;
			m_d3dDepthStencilSurface->GetDesc(&dssdesc);
			if(width != (PxU32)dssdesc.Width || height != (PxU32)dssdesc.Height)
			{
				m_d3dDepthStencilSurface->Release();
				m_d3dDepthStencilSurface = 0;
			}
		}
		if(!m_d3dDepthStencilSurface)
		{
			const D3DFORMAT           depthFormat        = D3DFMT_D24S8;
			const D3DMULTISAMPLE_TYPE multisampleType    = D3DMULTISAMPLE_NONE;
			const DWORD               multisampleQuality = 0;
			const BOOL                discard            = 0;
			IDirect3DSurface9 *depthSurface = 0;
			HRESULT result = m_d3dDevice->CreateDepthStencilSurface((UINT)width, (UINT)height, depthFormat, multisampleType, multisampleQuality, discard, &depthSurface, 0);
			RENDERER_ASSERT(result == D3D_OK, "Failed to create Direct3D9 DepthStencil Surface.");
			if(result == D3D_OK)
			{
				m_d3dDepthStencilSurface = depthSurface;
			}
		}
		m_d3dDevice->SetDepthStencilSurface(m_d3dDepthStencilSurface);
	}
}

// clears the offscreen buffers.
void D3D9Renderer::clearBuffers(void)
{
	if(m_d3dDevice)
	{
		const DWORD flags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL;
		m_d3dDevice->Clear(0, NULL, flags, D3DCOLOR_RGBA(getClearColor().r, getClearColor().g, getClearColor().b, getClearColor().a), 1.0f, 0);
	}
}

// presents the current color buffer to the screen.
// returns true on device reset and if buffers need to be rewritten.
bool D3D9Renderer::swapBuffers(void)
{
	bool isDeviceReset = false;
	if(m_d3dDevice)
	{
		HRESULT result = S_OK;
		if (useSwapchain() && validSwapchain())
			result = presentSwapchain();
		else
			result = SampleFramework::SamplePlatform::platform()->D3D9Present();
		RENDERER_ASSERT(result == D3D_OK || result == D3DERR_DEVICELOST, "Unknown Direct3D9 error when swapping buffers.");
		if(result == D3D_OK || result == D3DERR_DEVICELOST)
		{
			isDeviceReset = checkResize(result == D3DERR_DEVICELOST || m_d3dPresentParamsChanged);
		}
	}
	return isDeviceReset;
}

void D3D9Renderer::getWindowSize(PxU32 &width, PxU32 &height) const
{
	RENDERER_ASSERT(m_displayHeight * m_displayWidth > 0, "variables not initialized properly");
	width = m_displayWidth;
	height = m_displayHeight;
}

RendererVertexBuffer *D3D9Renderer::createVertexBuffer(const RendererVertexBufferDesc &desc)
{
	PX_PROFILE_ZONE("D3D9Renderer_createVertexBuffer",0);
	D3D9RendererVertexBuffer *vb = 0;
	if(m_d3dDevice)
	{
		RENDERER_ASSERT(desc.isValid(), "Invalid Vertex Buffer Descriptor.");
		if(desc.isValid())
		{
			vb = new D3D9RendererVertexBuffer(*m_d3dDevice, *this, desc);
		}
	}
	if(vb) addResource(*vb);
	return vb;
}

RendererIndexBuffer *D3D9Renderer::createIndexBuffer(const RendererIndexBufferDesc &desc)
{
	PX_PROFILE_ZONE("D3D9Renderer_createIndexBuffer",0);
	D3D9RendererIndexBuffer *ib = 0;
	if(m_d3dDevice)
	{
		RENDERER_ASSERT(desc.isValid(), "Invalid Index Buffer Descriptor.");
		if(desc.isValid())
		{
			ib = new D3D9RendererIndexBuffer(*m_d3dDevice, *this, desc);
		}
	}
	if(ib) addResource(*ib);
	return ib;
}

RendererInstanceBuffer *D3D9Renderer::createInstanceBuffer(const RendererInstanceBufferDesc &desc)
{
	PX_PROFILE_ZONE("D3D9Renderer_createInstanceBuffer",0);
	D3D9RendererInstanceBuffer *ib = 0;
	if(m_d3dDevice)
	{
		RENDERER_ASSERT(desc.isValid(), "Invalid Instance Buffer Descriptor.");
		if(desc.isValid())
		{
			ib = new D3D9RendererInstanceBuffer(*m_d3dDevice, *this, desc);
		}
	}
	if(ib) addResource(*ib);
	return ib;
}

RendererTexture2D *D3D9Renderer::createTexture2D(const RendererTexture2DDesc &desc)
{
	PX_PROFILE_ZONE("D3D9Renderer_createTexture2D",0);
	D3D9RendererTexture2D *texture = 0;
	if(m_d3dDevice)
	{
		RENDERER_ASSERT(desc.isValid(), "Invalid Texture 2D Descriptor.");
		if(desc.isValid())
		{
			texture = new D3D9RendererTexture2D(*m_d3dDevice, *this, desc);
		}
	}
	if(texture) addResource(*texture);
	return texture;
}

RendererTexture3D *D3D9Renderer::createTexture3D(const RendererTexture3DDesc &desc)
{
	//RENDERER_ASSERT(0, "Not implemented!");
	// TODO: Properly implement. 
	return 0;
}

RendererTarget *D3D9Renderer::createTarget(const RendererTargetDesc &desc)
{
	PX_PROFILE_ZONE("D3D9Renderer_createTarget",0);
	RendererTarget *target = 0;
#if defined(RENDERER_ENABLE_DIRECT3D9_TARGET)
	D3D9RendererTarget *d3dTarget = 0;
	RENDERER_ASSERT(desc.isValid(), "Invalid Target Descriptor.");
	if(desc.isValid())
	{
		d3dTarget = new D3D9RendererTarget(*m_d3dDevice, desc);
	}
	if(d3dTarget) addResource(*d3dTarget);
	target = d3dTarget;
#endif
	return target;
}

RendererMaterial *D3D9Renderer::createMaterial(const RendererMaterialDesc &desc)
{
	RendererMaterial *mat = hasMaterialAlready(desc);
	RENDERER_ASSERT(desc.isValid(), "Invalid Material Descriptor.");
	if(!mat && desc.isValid())
	{
		PX_PROFILE_ZONE("D3D9Renderer_createMaterial",0);
		mat = new D3D9RendererMaterial(*this, desc);

		registerMaterial(desc, mat);
	}
	return mat;
}

RendererMesh *D3D9Renderer::createMesh(const RendererMeshDesc &desc)
{
	PX_PROFILE_ZONE("D3D9Renderer_createMesh",0);
	D3D9RendererMesh *mesh = 0;
	RENDERER_ASSERT(desc.isValid(), "Invalid Mesh Descriptor.");
	if(desc.isValid())
	{
		mesh = new D3D9RendererMesh(*this, desc);
	}
	return mesh;
}

RendererLight *D3D9Renderer::createLight(const RendererLightDesc &desc)
{
	PX_PROFILE_ZONE("D3D9Renderer_createLight",0);
	RendererLight *light = 0;
	if(m_d3dDevice)
	{
		RENDERER_ASSERT(desc.isValid(), "Invalid Light Descriptor.");
		if(desc.isValid())
		{
			switch(desc.type)
			{
			case RendererLight::TYPE_DIRECTIONAL:
				light = new D3D9RendererDirectionalLight(*this, *(RendererDirectionalLightDesc*)&desc);
				break;
			case RendererLight::TYPE_SPOT:
				light = new D3D9RendererSpotLight(*this, *(RendererSpotLightDesc*)&desc);
				break;
			default:
				RENDERER_ASSERT(0, "Not implemented!");
			}
		}
	}
	return light;
}

void D3D9Renderer::setVsync(bool on)
{
	UINT newVal = on ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
	m_d3dPresentParamsChanged |= m_d3dPresentParams.PresentationInterval != newVal;
	m_d3dPresentParams.PresentationInterval = newVal;
	//RENDERER_ASSERT(0, "Not implemented!");
}

void D3D9Renderer::disableDepthTest()
{
	m_d3dDevice->SetRenderState(D3DRS_ZENABLE,  0);
}

void D3D9Renderer::enableDepthTest()
{
	m_d3dDevice->SetRenderState(D3DRS_ZENABLE,  1);
}

bool D3D9Renderer::beginRender(void)
{
	bool ok = false;
	if(m_d3dDevice)
	{
		ok = m_d3dDevice->BeginScene() == D3D_OK;
	}
	return ok;
}

void D3D9Renderer::endRender(void)
{
	if(m_d3dDevice)
	{
		m_d3dDevice->EndScene();
	}
}

void D3D9Renderer::bindViewProj(const physx::PxMat44 &eye, const RendererProjection &proj)
{
	m_viewMatrix = eye.inverseRT();
	convertToD3D9(m_environment.viewMatrix, m_viewMatrix);
	convertToD3D9(m_environment.projMatrix, proj);

	const PxVec3 eyePosition  =  eye.getPosition();
	const PxVec3 eyeDirection = -eye.getBasis(2);
	memcpy(m_environment.eyePosition,  &eyePosition.x,  sizeof(float)*3);
	memcpy(m_environment.eyeDirection, &eyeDirection.x, sizeof(float)*3);
}

void D3D9Renderer::bindFogState(const RendererColor &fogColor, float fogDistance)
{
	const float inv255 = 1.0f / 255.0f;

	m_environment.fogColorAndDistance[0] = fogColor.r*inv255;
	m_environment.fogColorAndDistance[1] = fogColor.g*inv255;
	m_environment.fogColorAndDistance[2] = fogColor.b*inv255;
	m_environment.fogColorAndDistance[3] = fogDistance;
}

void D3D9Renderer::bindAmbientState(const RendererColor &ambientColor)
{
	convertToD3D9(m_environment.ambientColor, ambientColor);
}

void D3D9Renderer::bindDeferredState(void)
{
	RENDERER_ASSERT(0, "Not implemented!");
}

void D3D9Renderer::bindMeshContext(const RendererMeshContext &context)
{
	physx::PxMat44 model;
	physx::PxMat44 modelView;
	if(context.transform) model = *context.transform;
	else                  model = PxMat44(PxIdentity);
	modelView = m_viewMatrix * model;

	convertToD3D9(m_environment.modelMatrix,     model);
	convertToD3D9(m_environment.modelViewMatrix, modelView);

	// it appears that D3D winding is backwards, so reverse them...
	DWORD cullMode = D3DCULL_CCW;
	switch(context.cullMode)
	{
	case RendererMeshContext::CLOCKWISE: 
		cullMode = context.negativeScale ? D3DCULL_CW : D3DCULL_CCW;
		break;
	case RendererMeshContext::COUNTER_CLOCKWISE: 
		cullMode = context.negativeScale ? D3DCULL_CCW : D3DCULL_CW;
		break;
	case RendererMeshContext::NONE: 
		cullMode = D3DCULL_NONE;
		break;
	default:
		RENDERER_ASSERT(0, "Invalid Cull Mode");
	}
	if (!blendingCull() && NULL != context.material && context.material->getBlending())
		cullMode = D3DCULL_NONE;

	m_d3dDevice->SetRenderState(D3DRS_CULLMODE, cullMode);

	DWORD fillMode = D3DFILL_SOLID;
	switch(context.fillMode)
	{
	case RendererMeshContext::SOLID:
		fillMode = D3DFILL_SOLID;
		break;
	case RendererMeshContext::LINE:
		fillMode = D3DFILL_WIREFRAME;
		break;
	case RendererMeshContext::POINT:
		fillMode = D3DFILL_POINT;
		break;
	}
	m_d3dDevice->SetRenderState(D3DRS_FILLMODE, fillMode);

	RENDERER_ASSERT(context.numBones <= RENDERER_MAX_BONES, "Too many bones.");
	if(context.boneMatrices && context.numBones>0 && context.numBones <= RENDERER_MAX_BONES)
	{
		for(PxU32 i=0; i<context.numBones; i++)
		{
			convertToD3D9(m_environment.boneMatrices[i], context.boneMatrices[i]);
		}
		m_environment.numBones = context.numBones;
	}
}

void D3D9Renderer::beginMultiPass(void)
{
	if(m_d3dDevice)
	{
		m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,  1);
		m_d3dDevice->SetRenderState(D3DRS_SRCBLEND,          D3DBLEND_ONE);
		m_d3dDevice->SetRenderState(D3DRS_DESTBLEND,         D3DBLEND_ONE);
		m_d3dDevice->SetRenderState(D3DRS_ZFUNC,             D3DCMP_EQUAL);
	}
}

void D3D9Renderer::endMultiPass(void)
{
	if(m_d3dDevice)
	{
		m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,  0);
		m_d3dDevice->SetRenderState(D3DRS_ZFUNC,             D3DCMP_LESS);
	}
}

void D3D9Renderer::beginTransparentMultiPass(void)
{
	if(m_d3dDevice)
	{
		setEnableBlendingOverride(true);
		m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,  1);
		m_d3dDevice->SetRenderState(D3DRS_SRCBLEND,          D3DBLEND_SRCALPHA);
		m_d3dDevice->SetRenderState(D3DRS_DESTBLEND,         D3DBLEND_ONE);
		//m_d3dDevice->SetRenderState(D3DRS_ZFUNC,             D3DCMP_EQUAL);
		m_d3dDevice->SetRenderState(D3DRS_ZFUNC,             D3DCMP_LESSEQUAL);
		m_d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE,      false);
	}
}

void D3D9Renderer::endTransparentMultiPass(void)
{
	if(m_d3dDevice)
	{
		setEnableBlendingOverride(false);
		m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,  0);
		m_d3dDevice->SetRenderState(D3DRS_ZFUNC,             D3DCMP_LESS);
		m_d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE,      true);
	}
}

void D3D9Renderer::renderDeferredLight(const RendererLight &light)
{
	RENDERER_ASSERT(0, "Not implemented!");
}

PxU32 D3D9Renderer::convertColor(const RendererColor& color) const
{
	D3DCOLOR outColor;
	convertToD3D9(outColor, color);

	return outColor;
}

bool D3D9Renderer::isOk(void) const
{
	bool ok = true;
	if(!m_d3d)            ok = false;
	if(!m_d3dDevice)      ok = false;
#if defined(RENDERER_WINDOWS)
	ok = SampleFramework::SamplePlatform::platform()->isD3D9ok();
	if(!m_d3dx.m_library) ok = false;
	// note: we could assert m_compiler_library here too, but actually loading that one is optional, so the app still may work without it.
#endif
	return ok;
}

void D3D9Renderer::addResource(D3D9RendererResource &resource)
{
	RENDERER_ASSERT(resource.m_d3dRenderer==0, "Resource already in added to the Renderer!");
	if(resource.m_d3dRenderer==0)
	{
		resource.m_d3dRenderer = this;
		m_resources.push_back(&resource);
	}
}

void D3D9Renderer::removeResource(D3D9RendererResource &resource)
{
	RENDERER_ASSERT(resource.m_d3dRenderer==this, "Resource not part of this Renderer!");
	if(resource.m_d3dRenderer==this)
	{
		resource.m_d3dRenderer = 0;
		const PxU32 numResources  = (PxU32)m_resources.size();
		for (PxU32 i = 0; i < numResources; i++)
		{
			if(m_resources[i] == &resource)
			{
				// the order of resources needs to remain intact, otherwise a render target that has a dependency on a texture might get onDeviceReset'ed earlier which is an error
				m_resources.erase(m_resources.begin() + i);
				break;
			}
		}
	}
}

void D3D9Renderer::notifyResourcesLostDevice(void)
{
	const PxU32 numResources  = (PxU32)m_resources.size();
	for(PxU32 i=0; i<numResources; i++)
	{
		m_resources[i]->onDeviceLost();
	}
}

void D3D9Renderer::notifyResourcesResetDevice(void)
{
	const PxU32 numResources  = (PxU32)m_resources.size();
	for(PxU32 i=0; i<numResources; i++)
	{
		m_resources[i]->onDeviceReset();
	}
}


///////////////////////////////////////////////////////////////////////////////

static DWORD gCullMode;
static DWORD gAlphaBlendEnable;
static DWORD gSrcBlend;
static DWORD gDstBlend;
static DWORD gFillMode;
static DWORD gZWrite;

bool D3D9Renderer::initTexter()
{
	if(!Renderer::initTexter())
		return false;

	if(!m_textVDecl)
	{
		D3DVERTEXELEMENT9 vdecl[4];
		vdecl[0].Stream		= 0;
		vdecl[0].Offset		= 0;
		vdecl[0].Type		= D3DDECLTYPE_FLOAT4;
		vdecl[0].Method		= D3DDECLMETHOD_DEFAULT;
#if defined(RENDERER_WINDOWS)
		vdecl[0].Usage		= D3DDECLUSAGE_POSITIONT;
#endif
		vdecl[0].UsageIndex	= 0;

		vdecl[1].Stream		= 0;
		vdecl[1].Offset		= 4*4;
		vdecl[1].Type		= D3DDECLTYPE_D3DCOLOR;
		vdecl[1].Method		= D3DDECLMETHOD_DEFAULT;
		vdecl[1].Usage		= D3DDECLUSAGE_COLOR;
		vdecl[1].UsageIndex	= 0;

		vdecl[2].Stream		= 0;
		vdecl[2].Offset		= 4*4 + 4;
		vdecl[2].Type		= D3DDECLTYPE_FLOAT2;
		vdecl[2].Method		= D3DDECLMETHOD_DEFAULT;
		vdecl[2].Usage		= D3DDECLUSAGE_TEXCOORD;
		vdecl[2].UsageIndex	= 0;

		vdecl[3].Stream		= 0xFF;
		vdecl[3].Offset		= 0;
		vdecl[3].Type		= (DWORD)D3DDECLTYPE_UNUSED;
		vdecl[3].Method		= 0;
		vdecl[3].Usage		= 0;
		vdecl[3].UsageIndex	= 0;

		m_d3dDevice->CreateVertexDeclaration(vdecl, &m_textVDecl);
		if(!m_textVDecl)
		{
			closeTexter();
			return false;
		}
	}

	return true;
}

void D3D9Renderer::closeTexter()
{
	if(m_textVDecl)
	{
		IDirect3DVertexDeclaration9* currDecl = NULL;
		m_d3dDevice->GetVertexDeclaration(&currDecl);
		if (currDecl == m_textVDecl)
		{
			m_d3dDevice->SetVertexDeclaration(NULL);
		}
		m_textVDecl->Release();
		m_textVDecl = NULL;
	}

	Renderer::closeTexter();
}


void D3D9Renderer::setupTextRenderStates()
{
	// PT: save render states. Let's just hope this isn't a pure device, else the Get method won't work
	m_d3dDevice->GetRenderState(D3DRS_CULLMODE, &gCullMode);
	m_d3dDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &gAlphaBlendEnable);	
	m_d3dDevice->GetRenderState(D3DRS_SRCBLEND, &gSrcBlend);
	m_d3dDevice->GetRenderState(D3DRS_DESTBLEND, &gDstBlend);
	m_d3dDevice->GetRenderState(D3DRS_FILLMODE, &gFillMode);
	m_d3dDevice->GetRenderState(D3DRS_ZWRITEENABLE, &gZWrite);

	// PT: setup render states for text rendering
	m_d3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	m_d3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_d3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	m_d3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	m_d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);
}

void D3D9Renderer::resetTextRenderStates()
{
	// PT: restore render states. We want text rendering not to interfere with existing render states.
	// For example the text should never be rendered in wireframe, even if the scene is.
	m_d3dDevice->SetRenderState(D3DRS_CULLMODE, gCullMode);
	m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, gAlphaBlendEnable);
	m_d3dDevice->SetRenderState(D3DRS_SRCBLEND, gSrcBlend);
	m_d3dDevice->SetRenderState(D3DRS_DESTBLEND, gDstBlend);
	m_d3dDevice->SetRenderState(D3DRS_FILLMODE, gFillMode);
	m_d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, gZWrite);
}

void D3D9Renderer::renderTextBuffer(const void* vertices, PxU32 nbVerts, const PxU16* indices, PxU32 nbIndices, RendererMaterial* material)
{
	PX_UNUSED(material);
	// PT: font texture must have been selected prior to calling this function
	const int PrimCount = nbIndices/3;
	const int Stride = sizeof(TextVertex);

	if(m_textVDecl && FAILED(m_d3dDevice->SetVertexDeclaration(m_textVDecl)))
	{
		assert(0);
		return;
	}

	DWORD hr = m_d3dDevice->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, nbVerts, PrimCount, indices, D3DFMT_INDEX16, vertices, Stride);
	if(FAILED(hr))
	{
		//		printf("Error!\n");
	}
}

void D3D9Renderer::renderLines2D(const void* vertices, PxU32 nbVerts)
{
	const int PrimCount = nbVerts-1;
	const int Stride = sizeof(TextVertex);

	if(m_textVDecl && FAILED(m_d3dDevice->SetVertexDeclaration(m_textVDecl)))
	{
		assert(0);
		return;
	}

	DWORD hr = m_d3dDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, PrimCount, vertices, Stride);
	if(FAILED(hr))
	{
		//		printf("Error!\n");
	}
}

void D3D9Renderer::setupScreenquadRenderStates()
{
	m_d3dDevice->GetRenderState(D3DRS_CULLMODE, &gCullMode);
	m_d3dDevice->GetRenderState(D3DRS_FILLMODE, &gFillMode);

	m_d3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	m_d3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	m_d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);
}

void D3D9Renderer::resetScreenquadRenderStates()
{
	m_d3dDevice->SetRenderState(D3DRS_CULLMODE, gCullMode);
	m_d3dDevice->SetRenderState(D3DRS_FILLMODE, gFillMode);
	m_d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);
}

bool D3D9Renderer::captureScreen( PxU32 &width, PxU32& height, PxU32& sizeInBytes, const void*& screenshotData )
{
	bool bSuccess = false;

	IDirect3DSurface9* backBuffer = NULL;
	if (useSwapchain() && validSwapchain())
	{
#if RENDERER_ENABLE_DIRECT3D9_SWAPCHAIN	
		bSuccess = SUCCEEDED(m_d3dSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backBuffer));
#endif		
	}
	else
	{
		bSuccess = SUCCEEDED(m_d3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer));
	}

	if (bSuccess)
	{
		bSuccess = false;
		if(m_displayBuffer)
		{
			m_displayBuffer->Release();
			m_displayBuffer = 0;
		}

		if(SUCCEEDED(m_d3dx.SaveSurfaceToFileInMemory(&m_displayBuffer, D3DXIFF_BMP, backBuffer, NULL, NULL)))
		{
			getWindowSize(width, height);
			sizeInBytes    = (physx::PxU32)m_displayBuffer->GetBufferSize();
			screenshotData = m_displayBuffer->GetBufferPointer();
			bSuccess       = true;
		}
		backBuffer->Release();
	}

	return bSuccess;
}

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)

