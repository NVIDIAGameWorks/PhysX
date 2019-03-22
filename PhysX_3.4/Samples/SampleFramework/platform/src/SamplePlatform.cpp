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

#include <PsString.h>
#include <SamplePlatform.h>

using namespace SampleFramework;
using namespace physx;

SamplePlatform* SamplePlatform::m_platform = NULL;

SamplePlatform* SamplePlatform::platform()
{
	RENDERER_ASSERT(SamplePlatform::m_platform, "SamplePlatform was not initialized!");
	if(SamplePlatform::m_platform) 
	{
		return SamplePlatform::m_platform;
	}
	return NULL;
}

SampleApplication* SamplePlatform::application()
{
	return m_sf_app;
}

bool SamplePlatform::preOpenWindow(void * ptr)
{
	return true;
}

void SamplePlatform::showCursor(bool)
{
}

void SamplePlatform::setPlatform(SamplePlatform* ptr)
{
	SamplePlatform::m_platform = ptr;
}

void* SamplePlatform::initializeD3D9()
{
	return NULL;
}

bool SamplePlatform::isD3D9ok()
{
	return true;
}

void* SamplePlatform::initializeD3D11()
{
	return NULL;
}

bool SamplePlatform::isD3D11ok()
{
	return true;
}

void* SamplePlatform::compileProgram(void * context, 
										const char* assetDir, 
										const char *programPath, 
										physx::PxU64 profile, 
										const char* passString, 
										const char *entry, 
										const char **args)

{
	return NULL;
}

SamplePlatform::SamplePlatform(SampleRenderer::RendererWindow* _app) : m_app(_app)
{
}

SamplePlatform::~SamplePlatform()
{
}

bool SamplePlatform::hasFocus() const
{
	return true;
}

void SamplePlatform::setFocus(bool b)
{
}

void SamplePlatform::update()
{
}

void SamplePlatform::setWindowSize(physx::PxU32 width, 
									physx::PxU32 height)
{
}

void SamplePlatform::getTitle(char *title, PxU32 maxLength) const
{
}

void SamplePlatform::setTitle(const char *title)
{
}

bool SamplePlatform::openWindow(physx::PxU32& width, 
								physx::PxU32& height,
								const char* title,
								bool fullscreen) 
{
	return true;
}


bool SamplePlatform::useWindow(physx::PxU64 hwnd)
{
	return false;
}


bool SamplePlatform::isOpen()
{
	return true;
}

bool SamplePlatform::closeWindow() 
{
	return true;
}

bool SamplePlatform::updateWindow() 
{
	return true;
}

size_t SamplePlatform::getCWD(char* path, size_t len)
{
	RENDERER_ASSERT(path && len, "buffer should not be empty!");
	*path = '\0';
	return 0;
}
void SamplePlatform::setCWDToEXE(void) 
{
}

void SamplePlatform::popPathSpec(char *path)
{
	char *ls = 0;
	while(*path)
	{
		if(*path == '\\' || *path == '/') ls = path;
		path++;
	}
	if(ls) *ls = 0;
}

void SamplePlatform::preRendererSetup()
{
}

void SamplePlatform::postRendererSetup(SampleRenderer::Renderer* renderer)
{
	getSampleUserInput()->setRenderer(renderer);
}

void SamplePlatform::setupRendererDescription(SampleRenderer::RendererDesc& renDesc) 
{
	renDesc.driver = SampleRenderer::Renderer::DRIVER_OPENGL;
}

void SamplePlatform::doInput()
{
}

void SamplePlatform::postRendererRelease()
{
}

void SamplePlatform::initializeOGLDisplay(const SampleRenderer::RendererDesc& desc,
									   physx::PxU32& width, 
									   physx::PxU32& height)
{
}

void SamplePlatform::showMessage(const char* title, const char* message)
{
	shdfnd::printFormatted("%s: %s\n", title, message);
}

bool SamplePlatform::saveBitmap(const char*, physx::PxU32, physx::PxU32, physx::PxU32, const void*)
{
	return false;
}

void SamplePlatform::initializeCGRuntimeCompiler()
{
}

void SamplePlatform::getWindowSize(PxU32& width, PxU32& height)
{
}
	
void SamplePlatform::freeDisplay() 
{
}

bool SamplePlatform::makeContextCurrent()
{
	return true;
}

void SamplePlatform::swapBuffers()
{
}

bool SamplePlatform::isContextValid()
{
	return true;
}

void SamplePlatform::postInitializeOGLDisplay()
{
}

void SamplePlatform::setOGLVsync(bool on)
{
}

physx::PxU32 SamplePlatform::initializeD3D9Display(void * presentParameters, 
																char* m_deviceName, 
																physx::PxU32& width, 
																physx::PxU32& height,
																void * m_d3dDevice_out)
{
	return 0;
}

physx::PxU32 SamplePlatform::initializeD3D11Display(void *dxgiSwapChainDesc, 
													char *m_deviceName, 
													physx::PxU32& width, 
													physx::PxU32& height,
													void *m_d3dDevice_out,
													void *m_d3dDeviceContext_out,
													void *m_dxgiSwap_out)
{
	return 0;
}

physx::PxU64 SamplePlatform::getWindowHandle()
{
	return 0;
}

physx::PxU32 SamplePlatform::D3D9Present()
{
	return 0;
}


physx::PxU32 SamplePlatform::D3D11Present(bool vsync)
{
	return 0;
}

void SamplePlatform::D3D9BlockUntilNotBusy(void * resource)
{
}

void SamplePlatform::D3D9DeviceBlockUntilIdle()
{
}

physx::PxU64 SamplePlatform::getD3D9TextureFormat(SampleRenderer::RendererTexture2D::Format format)
{
	return 0;
}

const char* SamplePlatform::getPathSeparator()
{
	return "\\";
}

bool SamplePlatform::makeSureDirectoryPathExists(const char*)
{
	return false;
}
physx::PxU64 SamplePlatform::getD3D11TextureFormat(SampleRenderer::RendererTexture2D::Format format)
{
	return 0;
}

