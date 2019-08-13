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

#ifndef LINUX_SAMPLE_PLATFORM_H
#define LINUX_SAMPLE_PLATFORM_H

#include <SamplePlatform.h>
#include <linux/LinuxSampleUserInput.h>

#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h> 

#if defined(RENDERER_ENABLE_OPENGL)
	#define GLEW_STATIC
	#include <GL/glew.h>
#endif

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

namespace SampleFramework
{
	class LinuxPlatform : public SamplePlatform 
	{
	public:
		explicit							LinuxPlatform(SampleRenderer::RendererWindow* _app);
		// System
		virtual void						showCursor(bool);
		virtual void						setCWDToEXE(void);
		const char*							getPathSeparator();
		bool								makeSureDirectoryPathExists(const char* dirPath);
		void								showMessage(const char* title, const char* message);
		// Rendering
		virtual void						initializeOGLDisplay(const SampleRenderer::RendererDesc& desc,
															physx::PxU32& width, 
															physx::PxU32& height);
		virtual void						postInitializeOGLDisplay();
		virtual void						postRendererSetup(SampleRenderer::Renderer* renderer);
		virtual void						setupRendererDescription(SampleRenderer::RendererDesc& renDesc);
		virtual	bool						hasFocus() const;
		virtual void						setFocus(bool b);
		virtual	void						getTitle(char *title, physx::PxU32 maxLength) const;
		virtual	void						setTitle(const char *title);
		virtual	bool						updateWindow();
		virtual bool						openWindow(physx::PxU32& width, 
														physx::PxU32& height,
														const char* title,
														bool fullscreen);
		virtual void						getWindowSize(physx::PxU32& width, physx::PxU32& height);
		virtual void						update();
		virtual bool						closeWindow();
		virtual	void						freeDisplay();
		virtual void						swapBuffers();
		
		virtual	void*						compileProgram(void * context, 
														  const char* assetDir, 
														  const char *programPath, 
														  physx::PxU64 profile, 
														  const char* passString, 
														  const char *entry, 
														   const char **args);
		
		// Input
		virtual void						doInput();
														   
		virtual const SampleUserInput*		getSampleUserInput() const 		{ return &m_linuxSampleUserInput; }
		virtual SampleUserInput*			getSampleUserInput()			{ return &m_linuxSampleUserInput; }

		virtual const char*					getPlatformName() const 		{ return "linux"; }
		
		virtual void						setMouseCursorRecentering(bool val);
		virtual bool						getMouseCursorRecentering() const;
		
		physx::PxVec2						getMouseCursorPos() const { return m_mouseCursorPos; }
		void								setMouseCursorPos(const physx::PxVec2& pos) { m_mouseCursorPos = pos; }
		void								recenterMouseCursor(bool generateEvent);

		LinuxSampleUserInput&				getLinuxSampleUserInput() 		{ return m_linuxSampleUserInput; }
		const LinuxSampleUserInput&			getLinuxSampleUserInput() const { return m_linuxSampleUserInput; }
		
	protected:
		bool								filterKeyRepeat(const XEvent& keyReleaseEvent);
		void								handleMouseEvent(const XEvent& event);
		void								showCursorInternal(bool show);
		
	protected:
		Display*							m_display;
		XVisualInfo*						m_visualInfo;
		Window                				m_window;
		Atom								m_wmDelete;
		GLXContext             				m_glxContext;
		XF86VidModeModeInfo					m_desktopMode;
		int 								m_screen;  
		bool								m_isFullScreen;
		bool								m_hasFocus;
		bool								m_hasContentFocus;
		physx::PxU32						m_windowWidth;
		physx::PxU32						m_windowHeight;
		LinuxSampleUserInput				m_linuxSampleUserInput;
		physx::PxVec2						m_mouseCursorPos;
		bool								m_recenterMouseCursor;
		bool								m_showCursor;
	};
}

#endif
