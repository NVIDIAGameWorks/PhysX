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
#include <RendererMemoryMacros.h>
#include <linux/LinuxSamplePlatform.h>
#include <SampleApplication.h>
#include <Cg/cg.h>
#include <stdio.h>

#include <Ps.h>
#include <PsString.h>
//#include <PsFile.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>


using namespace SampleFramework;
using SampleRenderer::RendererWindow;

namespace Ps = physx::shdfnd;

// filters OS key auto repeat, otherwise you get repeatedly key released and pressed events
// returns true if the supplied event was a auto repeat one.
// XAutoRepeatOff(m_display); does change the behaviour for the entire display.
// So you see the effect also in other apps while the samples are running, which is bad.
bool LinuxPlatform::filterKeyRepeat(const XEvent& keyReleaseEvent)
{
	if(keyReleaseEvent.type == KeyRelease && XEventsQueued(m_display, QueuedAfterReading)) 
	{
		XEvent nev;
		XPeekEvent(m_display, &nev);
		if (nev.type == KeyPress &&
		nev.xkey.time == keyReleaseEvent.xkey.time && 
		nev.xkey.keycode == keyReleaseEvent.xkey.keycode) 
		{
			// Key wasn't actually released, eat event
			XNextEvent(m_display, &nev);
			return true;
		}
	}
	return false;
}

SamplePlatform* SampleFramework::createPlatform(SampleRenderer::RendererWindow* _app)
{
	printf("Creating linux platform abstraction.\n");	
	SamplePlatform::setPlatform(new LinuxPlatform(_app));
	return SamplePlatform::platform();
}

void* LinuxPlatform::compileProgram(void * context, 
									  const char* assetDir, 
									  const char *programPath, 
									  physx::PxU64 profile, 
									  const char* passString, 
									  const char *entry, 
									  const char **args)

{
	char fullpath[1024];
	Ps::strlcpy(fullpath, 1024, assetDir);
	Ps::strlcat(fullpath, 1024, "shaders/");
	Ps::strlcat(fullpath, 1024, programPath);
	CGprogram program = cgCreateProgramFromFile(static_cast<CGcontext>(context), CG_SOURCE, fullpath, static_cast<CGprofile>(profile), entry, args);
	
	return program;
}

static PxI32 errorHandler(Display* d, XErrorEvent* e)
{
	printf("Xerror!\n");
	return 0;
}

static PxI32 errorHandlerIO(Display* d)
{
	printf("X IO error!\n");
	return 0;
}


LinuxPlatform::LinuxPlatform(SampleRenderer::RendererWindow* _app) 
: SamplePlatform(_app)
, m_display(NULL)
, m_visualInfo(NULL)
, m_hasFocus(false)
, m_hasContentFocus(false)
, m_isFullScreen(false)
, m_screen(0)
, m_mouseCursorPos(0)
, m_recenterMouseCursor(false)
, m_showCursor(true)
{
	XInitThreads();
	XSetIOErrorHandler(errorHandlerIO);
	XSetErrorHandler(errorHandler);
}

void LinuxPlatform::setCWDToEXE(void) 
{
	char exepath[1024] = {0};
	if(getcwd(exepath, 1024))
	{
		if(chdir(exepath) != 0)
		{
			printf("LinuxPlatform::setCWDToEXE chdir failed!\n");
		}
	}
}

void LinuxPlatform::setupRendererDescription(SampleRenderer::RendererDesc& renDesc) 
{
	renDesc.driver = SampleRenderer::Renderer::DRIVER_OPENGL;
	renDesc.windowHandle = 0;
}

void LinuxPlatform::postRendererSetup(SampleRenderer::Renderer* renderer) 
{
	if(!renderer)
	{
		// quit if no renderer was created.  Nothing else to do.
		// error was output in createRenderer.
		exit(1);
	}
	char windowTitle[1024] = {0};
	m_app->getTitle(windowTitle, 1024);
	strcat(windowTitle, " : ");
	strcat(windowTitle, SampleRenderer::Renderer::getDriverTypeName(renderer->getDriverType()));
	m_app->setTitle(windowTitle);
}

void LinuxPlatform::setMouseCursorRecentering(bool val)
{
	if (m_recenterMouseCursor != val)
	{
		m_recenterMouseCursor = val;
		if (m_recenterMouseCursor)
			recenterMouseCursor(false);
	}
}

bool LinuxPlatform::getMouseCursorRecentering() const
{
	return m_recenterMouseCursor;
}

void LinuxPlatform::recenterMouseCursor(bool generateEvent)
{	
	if (m_recenterMouseCursor && m_hasContentFocus)
	{
		// returns relative window coordinates, opposed to absolute ones!
		// different than on other platforms.
		PxI32 x, y, xtmp, ytmp;
		PxU32 mtmp;
		Window root, child;
		XQueryPointer(m_display, m_window, &root, &child, &xtmp, &ytmp, &x, &y, &mtmp);

		PxVec2 current(static_cast<PxReal>(x), static_cast<PxReal>(m_windowHeight - y));
		
		PxI32 linuxCenterX = m_windowWidth >> 1;
		PxI32 linuxCenterY = m_windowHeight >> 1;
		
		XWarpPointer(m_display, 0, m_window, 0, 0, 0, 0, linuxCenterX, linuxCenterY);
		// sync here needed, otherwise the deltas will be (almost) zero
		XSync(m_display, false);
				
		if (generateEvent)
		{
			PxVec2 diff = current - PxVec2(static_cast<PxReal>(linuxCenterX), static_cast<PxReal>(m_windowHeight - linuxCenterY));
			getLinuxSampleUserInput().doOnMouseMove(linuxCenterX, m_windowHeight - linuxCenterY, diff.x, diff.y, MOUSE_MOVE);
		}
	
		m_mouseCursorPos = current;
	}
}

void LinuxPlatform::showCursorInternal(bool show)
{
	if(show)
	{
		XUndefineCursor(m_display, m_window);
	}
	else
	{
		Pixmap bm_no;
		Colormap cmap;
        Cursor no_ptr;
        XColor black, dummy;
        static char bm_no_data[] = {0, 0, 0, 0, 0, 0, 0, 0};

        cmap = DefaultColormap(m_display, DefaultScreen(m_display));
        XAllocNamedColor(m_display, cmap, "black", &black, &dummy);
        bm_no = XCreateBitmapFromData(m_display, m_window, bm_no_data, 8, 8);
        no_ptr = XCreatePixmapCursor(m_display, bm_no, bm_no, &black, &black, 0, 0);

        XDefineCursor(m_display, m_window, no_ptr);
        XFreeCursor(m_display, no_ptr);
        if(bm_no != None)
			XFreePixmap(m_display, bm_no);
        XFreeColors(m_display, cmap, &black.pixel, 1, 0);
	}	
}

void LinuxPlatform::showCursor(bool show)
{
	if(m_showCursor == show)
		return;

	m_showCursor = show;
	showCursorInternal(show);
}

void LinuxPlatform::doInput()
{
	/* do nothing */
}

bool LinuxPlatform::openWindow(physx::PxU32& width, physx::PxU32& height,const char* title, bool fullscreen)
{
	Window                  rootWindow;
	GLint                   attributes[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	Colormap                colorMap;
	XSetWindowAttributes    setWindowAttributes;
	XWindowAttributes       xWindowAttributes;
	XEvent                  xEvent;
	
	int 					glxMajor, glxMinor, vmMajor, vmMinor;
	XF86VidModeModeInfo**	modes;             
	int 					modeNum;	
	int 					bestMode = 0; // set best mode to current
	                  
	m_display = XOpenDisplay(NULL);
	m_screen = DefaultScreen(m_display);

	if(!m_display)
	{
		printf("Cannot connect to X server!\n");
		closeWindow();
		return false;
	}
	
	XF86VidModeQueryVersion(m_display, &vmMajor, &vmMinor);
	printf("XF86 VideoMode extension version %d.%d\n", vmMajor, vmMinor);
	
	glXQueryVersion(m_display, &glxMajor, &glxMinor);                                     
	printf("GLX-Version %d.%d\n", glxMajor, glxMinor);
	
	rootWindow = RootWindow(m_display, m_screen);                                                    
	
	m_visualInfo = glXChooseVisual(m_display, m_screen, attributes);
	if(!m_visualInfo)
	{
		printf("no appropriate visual found!\n");
		closeWindow();
		return false;
	}
	
	colorMap = XCreateColormap(m_display, rootWindow, m_visualInfo->visual, AllocNone);
	setWindowAttributes.colormap = colorMap;
	setWindowAttributes.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask;

	m_windowWidth = width;
	m_windowHeight = height;
	//fullscreen = true;
	m_isFullScreen = fullscreen;
	int windowFlags = CWColormap | CWEventMask;

	if(fullscreen)
	{
		XF86VidModeGetAllModeLines(m_display, m_screen, &modeNum, &modes);       
		// save desktop-resolution before switching modes              
		m_desktopMode = *modes[0];
											  
		// look for mode with requested resolution                   
		for (PxU32 i = 0; i < modeNum; i++)                                        
		{                                                                    
			if ((modes[i]->hdisplay == width) && (modes[i]->vdisplay == height))
				bestMode = i;                                                   
		}             
		// switch to fullscreen
		XF86VidModeSwitchToMode(m_display, m_screen, modes[bestMode]);
		XF86VidModeSetViewPort(m_display, m_screen, 0, 0);            
		m_windowWidth = modes[bestMode]->hdisplay;                     
		m_windowHeight = modes[bestMode]->vdisplay;                    
		printf("switched to fullscreen resolution %dx%d\n", m_windowWidth, m_windowHeight);        
		XFree(modes);
		setWindowAttributes.override_redirect = True;
		windowFlags |= CWBorderPixel | CWOverrideRedirect;                               
	}                                                                        
	
	m_window = XCreateWindow(m_display, rootWindow, 0, 0, m_windowWidth, m_windowHeight, 0, m_visualInfo->depth, InputOutput, m_visualInfo->visual, windowFlags, &setWindowAttributes);

	XMapRaised(m_display, m_window);
	XStoreName(m_display, m_window, title);   
	
	if(fullscreen)
	{           
		XWarpPointer(m_display, None, m_window, 0, 0, 0, 0, 0, 0);                                            
		XGrabKeyboard(m_display, m_window, True, GrabModeAsync, GrabModeAsync, CurrentTime);                                     
		XGrabPointer(m_display, m_window, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, m_window, None, CurrentTime);	   
	}
	else
	{
		m_wmDelete = XInternAtom(m_display, "WM_DELETE_WINDOW", True);                 
		XSetWMProtocols(m_display, m_window, &m_wmDelete, 1);
		XMapWindow(m_display, m_window);
	}
	
	m_hasContentFocus = true;
	m_showCursor = true;
	recenterMouseCursor(false);
	
	return true;
}

void LinuxPlatform::initializeOGLDisplay(const SampleRenderer::RendererDesc& desc,
									physx::PxU32& width, 
									physx::PxU32& height)
{
	m_glxContext = glXCreateContext(m_display, m_visualInfo, NULL, GL_TRUE);
	glXMakeCurrent(m_display, m_window, m_glxContext);
	getWindowSize(width, height);
}

void LinuxPlatform::postInitializeOGLDisplay()
{
	glewInit();
}

bool LinuxPlatform::closeWindow()
{
	if(m_glxContext)
	{
		glXMakeCurrent(m_display, None, NULL);
		glXDestroyContext(m_display, m_glxContext);
		m_glxContext = 0;
		XDestroyWindow(m_display, m_window);
		if(m_isFullScreen)                                                         
		{                                                                        
			XF86VidModeSwitchToMode(m_display, m_screen, &m_desktopMode);              
			XF86VidModeSetViewPort(m_display, m_screen, 0, 0);                       
		} 
		
		if(m_visualInfo)
			XFree(m_visualInfo);
		m_visualInfo = NULL;	
		
		if(m_display)
			XCloseDisplay(m_display);
		m_display = NULL;
	}
	return true;
}

void LinuxPlatform::freeDisplay()
{
}

void LinuxPlatform::getWindowSize(PxU32& width, PxU32& height)
{
	if(!m_display)
		return;
	XWindowAttributes attr;
	XGetWindowAttributes(m_display, m_window, &attr);
	width = attr.width;
	height = attr.height;
}

void LinuxPlatform::setFocus(bool b)
{
	m_hasFocus = b;
}

bool LinuxPlatform::hasFocus() const
{
	return m_hasContentFocus;
}

void LinuxPlatform::getTitle(char *title, physx::PxU32 maxLength) const
{
	if(!m_display)
		return;

	char* t;
	XFetchName(m_display, m_window, &t);
	strncpy(title, t, maxLength);
	XFree(t);
}

void LinuxPlatform::setTitle(const char *title)
{
	// unsafe!
	if(m_display && title)
		XStoreName(m_display, m_window, title);
}

bool LinuxPlatform::updateWindow()
{
	return true;
}

void LinuxPlatform::handleMouseEvent(const XEvent& event)
{
	PxU32 x = event.xbutton.x;
	PxU32 y = m_windowHeight - event.xbutton.y;
	PxVec2 current(static_cast<PxReal>(x), static_cast<PxReal>(y));
	setMouseCursorPos(current);
	
	switch (event.type)
	{
		case ButtonPress:
		{
			m_linuxSampleUserInput.doOnMouseDown(x, y, event.xbutton.button);
		}
		break;
		case ButtonRelease:
		{
			m_linuxSampleUserInput.doOnMouseUp(x, y, event.xbutton.button);
		}
		break;
		case MotionNotify:
		{
			if (!getMouseCursorRecentering())
			{
				PxVec2 diff = current - getMouseCursorPos();
				m_linuxSampleUserInput.doOnMouseMove(x, y, diff.x, diff.y, MOUSE_MOVE);
			}
		}
		break;
	}
}

void LinuxPlatform::update()
{
	XEvent event;
	bool close = false;
	
	if(!m_display || !m_app || !m_app->isOpen())
		return;
		
	// handle the events in the queue 
	while (m_display && !close && m_app->isOpen() && (XPending(m_display) > 0))        
	{                                  
		XNextEvent(m_display, &event);     
		switch (event.type)              
		{                                
			case Expose:             
				if (event.xexpose.count == 0)
				{      
					PxU32 width, height;
					getWindowSize(width, height);
					m_app->onResize(width, height);
					m_windowWidth = width;
					m_windowHeight = height;
				}
				break;                       
			case FocusIn:
				m_app->setFocus(true);        
			break; 
			case FocusOut:
				m_app->setFocus(false); 
				m_hasContentFocus = false;  
			break;
			case ButtonPress:
				// fixes recentering issue: jumping window with first context switch.
				if(!m_hasContentFocus)
				{
					m_hasContentFocus = true;
					recenterMouseCursor(false);
				}
				else
					handleMouseEvent(event);
				break;
			case ButtonRelease:
				handleMouseEvent(event);
				break;
			case MotionNotify:
				handleMouseEvent(event);
				break;
			case KeyPress:
			case KeyRelease:
				{
					// and now some code to filter out the key releases 
					// which are generated by auto repeat
					if(filterKeyRepeat(event)) 
						break;			

					char keyName;
					KeySym keySym;
					XLookupString(&event.xkey, &keyName, 1, &keySym, NULL);
					bool keyDown = (event.type == KeyPress);
					if(keyDown)
						m_linuxSampleUserInput.doOnKeyDown(keySym, event.xkey.keycode, keyName);
					else
						m_linuxSampleUserInput.doOnKeyUp(keySym, event.xkey.keycode, keyName);	
				}
				break;
			case ClientMessage:
				if((Atom)event.xclient.data.l[0] == m_wmDelete)
					close = true;
				break;
			default:
				printf("unhandled event type: %d\n", event.type);
				break;
		}
	}

	recenterMouseCursor(true);
	
	if(close)
		m_app->close();
}

void LinuxPlatform::swapBuffers()
{
	if(m_display)
		glXSwapBuffers(m_display, m_window);
}

const char* LinuxPlatform::getPathSeparator()
{
	return "/";
}

static bool doesDirectoryExist(const char* path)
{
	bool exists = false;
	DIR* dir = NULL;
	dir = opendir(path);
	if(dir)
	{
		closedir(dir);
		exists = true;
	}
	return exists;
}

bool LinuxPlatform::makeSureDirectoryPathExists(const char* dirPath)
{
	bool ok = doesDirectoryExist(dirPath);
	if (!ok)
		ok = mkdir(dirPath, S_IRWXU|S_IRWXG|S_IRWXO) == 0;
	return ok;
}

void LinuxPlatform::showMessage(const char* title, const char* message)
{
	printf("%s: %s\n", title, message);
}

