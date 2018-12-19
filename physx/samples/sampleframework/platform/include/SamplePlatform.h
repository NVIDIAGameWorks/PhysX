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

#ifndef SAMPLE_PLATFORM_H
#define SAMPLE_PLATFORM_H

#include <RendererDesc.h>
#include <RendererWindow.h>
#include <RendererTexture2D.h>

namespace SampleFramework
{
	class SampleApplication;
	class SampleUserInput;

	/* This class declares and partly implements platform-abstraction level.
		Do not use platform-specific types and functions here.
	*/
	class SamplePlatform 
	{
	protected:
		SampleRenderer::RendererWindow*		m_app;
		SampleApplication*					m_sf_app;
		static SamplePlatform*				m_platform;

	public:
		// access
		static SamplePlatform*				platform();
		static void							setPlatform(SamplePlatform*);
		SampleApplication*					application();

		// creation
		explicit							SamplePlatform(SampleRenderer::RendererWindow* _app);
		virtual								~SamplePlatform() = 0;
		// System
		virtual void						showCursor(bool);

		virtual size_t						getCWD(char* path, size_t len);
		virtual void						setCWDToEXE(void);
		virtual void						popPathSpec(char *path);
		virtual bool						preOpenWindow(void * ptr);
		virtual bool						openWindow(physx::PxU32& width, 
														physx::PxU32& height,
														const char* title,
														bool fullscreen);
		virtual bool						useWindow(physx::PxU64 hwnd);
		virtual void						update();
		virtual bool						closeWindow();
		virtual bool						updateWindow();
		virtual bool						hasFocus() const;
		virtual void						setFocus(bool b);
		virtual bool						isOpen();
		virtual physx::PxU64				getWindowHandle();
		virtual void						setWindowSize(physx::PxU32 width, 
															physx::PxU32 height);
		virtual void						getWindowSize(physx::PxU32& width, physx::PxU32& height);
		virtual void						getTitle(char *title, physx::PxU32 maxLength) const;
		virtual void						setTitle(const char *title);

		virtual void						setMouseCursorRecentering(bool val) {}
		virtual bool						getMouseCursorRecentering() const { return false; }

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
		virtual void*						initializeD3D11();
		virtual bool						isD3D9ok();
		virtual const char*					getPathSeparator();
		virtual bool						isD3D11ok();
		// Rendering
		virtual void						initializeCGRuntimeCompiler();
		virtual void						initializeOGLDisplay(const SampleRenderer::RendererDesc& desc,
																 physx::PxU32& width, 
																 physx::PxU32& height);
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
		virtual void						D3D9BlockUntilNotBusy(void * resource);
		virtual void						D3D9DeviceBlockUntilIdle();
		virtual physx::PxU64				getD3D9TextureFormat(SampleRenderer::RendererTexture2D::Format format);
		virtual physx::PxU32				D3D11Present(bool vsync);
		virtual physx::PxU64				getD3D11TextureFormat(SampleRenderer::RendererTexture2D::Format format);
		virtual void						postInitializeOGLDisplay();
		virtual void						setOGLVsync(bool on);
		virtual bool						makeContextCurrent();
		virtual bool						isContextValid();
		virtual void						freeDisplay();
		virtual void						swapBuffers();
		virtual void						postRendererRelease();
		virtual void						preRendererSetup();
		virtual void						postRendererSetup(SampleRenderer::Renderer* renderer);
		virtual void						setupRendererDescription(SampleRenderer::RendererDesc& renDesc);
		// Input
		virtual void						doInput();

		virtual const SampleUserInput*		getSampleUserInput() const = 0;
		virtual SampleUserInput*			getSampleUserInput() = 0;

		virtual const char*					getPlatformName() const { return NULL; }
		// File System
		virtual bool						makeSureDirectoryPathExists(const char* dirPath);
	};

	SamplePlatform*		createPlatform(SampleRenderer::RendererWindow* _app);
}

#endif
