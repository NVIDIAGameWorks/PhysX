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

#ifndef WINDOWS_SAMPLE_PLATFORM_H
#define WINDOWS_SAMPLE_PLATFORM_H

#include <SamplePlatform.h>
#include <windows/WindowsSampleUserInput.h>

struct IDirect3D9;
struct IDirect3DDevice9;

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGIFactory1;
struct IDXGISwapChain;

namespace SampleFramework
{
	class WindowsPlatform : public SamplePlatform 
	{
	public:
		explicit							WindowsPlatform(SampleRenderer::RendererWindow* _app);
		virtual								~WindowsPlatform();
		// System
		virtual void						showCursor(bool);
		virtual void						postRendererSetup(SampleRenderer::Renderer* renderer);
		virtual size_t						getCWD(char* path, size_t len);
		virtual void						setCWDToEXE(void);
		virtual bool						openWindow(physx::PxU32& width, 
														physx::PxU32& height,
														const char* title,
														bool fullscreen);
		virtual bool						useWindow(physx::PxU64 hwnd);
		virtual void						update();
		virtual bool						isOpen();
		virtual bool						closeWindow();
		virtual bool						hasFocus() const;
		virtual void						getTitle(char *title, physx::PxU32 maxLength) const;
		virtual void						setTitle(const char *title);
		virtual void						setFocus(bool b);
		virtual physx::PxU64				getWindowHandle();
		virtual void						setWindowSize(physx::PxU32 width, 
															physx::PxU32 height);
		virtual void						getWindowSize(physx::PxU32& width, physx::PxU32& height);
		virtual void						showMessage(const char* title, const char* message);
		virtual bool						saveBitmap(const char* fileName, 
														physx::PxU32 width, 
														physx::PxU32 height, 
														physx::PxU32 sizeInBytes, 
														const void* data);
		virtual void*						compileProgram(void * context, 
															const char* assetDir, 
															const char *programPath, 
															physx::PxU64 profile, 
															const char* passString, 
															const char *entry, 
															const char **args);
		virtual void*						initializeD3D9();
		virtual bool						isD3D9ok();
		virtual void*						initializeD3D11();
		virtual bool						isD3D11ok();
		// Rendering
		virtual void						initializeOGLDisplay(const SampleRenderer::RendererDesc& desc, 
																physx::PxU32& width, 
																physx::PxU32& height);
		virtual void						postInitializeOGLDisplay();
		virtual void						setOGLVsync(bool on);
		virtual physx::PxU32				initializeD3D9Display(void * presentParameters, 
																char* m_deviceName, 
																physx::PxU32& width, 
																physx::PxU32& height,
																void * m_d3dDevice_out);
		virtual physx::PxU32				initializeD3D11Display(void *dxgiSwapChainDesc, 
																char *m_deviceName, 
																physx::PxU32& width, 
																physx::PxU32& height,
																void *m_d3dDevice_out,
																void *m_d3dDeviceContext_out,
																void *m_dxgiSwap_out);

		virtual physx::PxU32				D3D9Present();
		virtual physx::PxU64				getD3D9TextureFormat(SampleRenderer::RendererTexture2D::Format format);
		virtual physx::PxU32				D3D11Present(bool vsync);
		virtual physx::PxU64				getD3D11TextureFormat(SampleRenderer::RendererTexture2D::Format format);
		virtual bool						makeContextCurrent();
		virtual void						freeDisplay();
		virtual bool						isContextValid();
		virtual void						swapBuffers();
		virtual void						setupRendererDescription(SampleRenderer::RendererDesc& renDesc);
		// Input
		virtual void						doInput();
		// File System
		virtual bool						makeSureDirectoryPathExists(const char* dirPath);

		virtual const SampleUserInput*		getSampleUserInput() const { return &m_windowsSampleUserInput; }
		virtual SampleUserInput*			getSampleUserInput() { return &m_windowsSampleUserInput; }

		WindowsSampleUserInput&				getWindowsSampleUserInput() { return m_windowsSampleUserInput; }
		const WindowsSampleUserInput&		getWindowsSampleUserInput() const { return m_windowsSampleUserInput; }

		virtual const char*					getPlatformName() const { return m_platformName; }

		virtual void						setMouseCursorRecentering(bool val);
		virtual bool						getMouseCursorRecentering() const;
		

		physx::PxVec2						getMouseCursorPos() const { return m_mouseCursorPos; }
		void								setMouseCursorPos(const physx::PxVec2& pos) { m_mouseCursorPos = pos; }
		void								recenterMouseCursor(bool generateEvent);
		void								setWorkaroundMouseMoved() { m_workaroundMouseMoved = true;  }
	
	protected:	
		IDirect3D9*							m_d3d;
		IDirect3DDevice9*					m_d3dDevice;
		IDXGIFactory1*						m_dxgiFactory;
		IDXGISwapChain*						m_dxgiSwap;
		ID3D11Device*						m_d3d11Device;
		ID3D11DeviceContext*				m_d3d11DeviceContext;
		HWND								m_hwnd;
		HDC									m_hdc;
		HGLRC								m_hrc;
		HMODULE								m_library;
		HMODULE								m_dxgiLibrary;
		HMODULE								m_d3d11Library;
		bool								m_ownsWindow;
		bool								m_isHandlingMessages;
		bool								m_destroyWindow;
		bool								m_hasFocus;
		bool								m_showCursor;
		char								m_platformName[256];
		WindowsSampleUserInput				m_windowsSampleUserInput;
		physx::PxVec2						m_mouseCursorPos;
		bool								m_workaroundMouseMoved;
		bool								m_recenterMouseCursor;
		bool								m_vsync;
	};
}

#endif
