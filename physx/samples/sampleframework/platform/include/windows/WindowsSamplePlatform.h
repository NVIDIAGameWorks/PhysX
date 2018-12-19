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
