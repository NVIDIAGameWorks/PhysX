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
#include <windows/WindowsSamplePlatform.h>

#if defined(RENDERER_ENABLE_CG)
#include <cg/cg.h>
#endif

#include <direct.h>
#include <stdio.h>

#include <PsString.h>
#include <PxTkFile.h>
#include <PsUtilities.h>

#if defined(RENDERER_ENABLE_DIRECT3D9)
	#include <d3d9.h>
	#include <d3dx9.h>
	#include <XInput.h>
#endif

#if defined(RENDERER_ENABLE_DIRECT3D11)
#pragma warning(push)
// Disable macro redefinition warnings
#pragma warning(disable: 4005)
	#include <d3d11.h>
#pragma warning(pop)
#endif

#if defined(RENDERER_ENABLE_OPENGL)
	#define GLEW_STATIC
	#include <GL/glew.h>
	#include <GL/wglew.h>

	#pragma comment(lib, "OpenGL32.lib")
	#pragma comment(lib, "GLU32.lib")
#endif

using std::min;
using std::max;
#pragma warning(push)
#pragma warning(disable : 4244)
#include <atlimage.h>
#include <Gdiplusimaging.h>
#pragma warning(pop)

using namespace SampleFramework;
using SampleRenderer::RendererWindow;
using namespace physx;

static void setDCPixelFormat(HDC &dc)
{
	const PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW |
		PFD_SUPPORT_OPENGL |
		PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		8, 0, 8, 0, 8, 0,
		8,
		0,
		0,
		0, 0, 0, 0,
		24,
		8,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};
	int pfindex = ChoosePixelFormat(dc, &pfd);
	SetPixelFormat(dc, pfindex, &pfd);
}

static const char *g_windowClassName = "RendererWindow";
static const DWORD g_windowStyle     = WS_OVERLAPPEDWINDOW;
static const DWORD g_fullscreenStyle = WS_POPUP;

static void handleMouseEvent(UINT msg, LPARAM lParam, HWND hwnd, RendererWindow* window)
{
	if (!window || !window->getPlatform())
		return;

	RECT rect;
	GetClientRect(hwnd, &rect);
	PxU32 height = (PxU32)(rect.bottom-rect.top);
	PxU32 x = (PxU32)LOWORD(lParam);
	PxU32 y = height-(PxU32)HIWORD(lParam);
	PxVec2 current(static_cast<PxReal>(x), static_cast<PxReal>(y));

	WindowsPlatform& platform = *((WindowsPlatform*)window->getPlatform());
	PxVec2 diff = current - platform.getMouseCursorPos();
	platform.setMouseCursorPos(current);
	
	switch (msg)
	{
	case WM_MOUSEMOVE:
		if (!SamplePlatform::platform()->getMouseCursorRecentering())
		{
			platform.getWindowsSampleUserInput().doOnMouseMove(x, y, diff.x, diff.y, MOUSE_MOVE);
		}
		else
		{
			//needed in WindowsPlatform::recenterMouseCursor to filter out erroneous cursor difference, even though mouse is not moving.
			((WindowsPlatform*)window->getPlatform())->setWorkaroundMouseMoved();
		}
		break;
	case WM_LBUTTONDOWN:
		SetCapture(hwnd);
		platform.getWindowsSampleUserInput().doOnMouseButton(x, y, WindowsSampleUserInput::LEFT_MOUSE_BUTTON, true);
		break;
	case WM_LBUTTONUP:
		ReleaseCapture();
		platform.getWindowsSampleUserInput().doOnMouseButton(x, y, WindowsSampleUserInput::LEFT_MOUSE_BUTTON, false);
		break;
	case WM_RBUTTONDOWN:
		SetCapture(hwnd);
		platform.getWindowsSampleUserInput().doOnMouseButton(x, y, WindowsSampleUserInput::RIGHT_MOUSE_BUTTON, true);
		break;
	case WM_RBUTTONUP:
		ReleaseCapture();
		platform.getWindowsSampleUserInput().doOnMouseButton(x, y, WindowsSampleUserInput::RIGHT_MOUSE_BUTTON, false);
		break;
	case WM_MBUTTONDOWN:
		SetCapture(hwnd);
		platform.getWindowsSampleUserInput().doOnMouseButton(x, y, WindowsSampleUserInput::CENTER_MOUSE_BUTTON, true);
		break;
	case WM_MBUTTONUP:
		ReleaseCapture();
		platform.getWindowsSampleUserInput().doOnMouseButton(x, y, WindowsSampleUserInput::CENTER_MOUSE_BUTTON, false);
		break;
	}
}

static INT_PTR CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#if defined(RENDERER_64BIT)
	RendererWindow* window = (RendererWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
#else
	RendererWindow* window = (RendererWindow*)LongToPtr(GetWindowLongPtr(hwnd, GWLP_USERDATA));
#endif

	bool customHandle = false;

	if(!customHandle)
	{
		switch(msg)
		{
		case WM_SETFOCUS:
			if(window)
				window->setFocus(true);
			break;

		case WM_KILLFOCUS:
			if(window)
				window->setFocus(false);
			break;

		case WM_EXITSIZEMOVE:
			if (window)
			{
				((WindowsPlatform*)window->getPlatform())->recenterMouseCursor(false);
			}
			break;

		case WM_CREATE:
			::UpdateWindow(hwnd);
			break;

		case WM_CLOSE:
			if(window)
			{
				((WindowsPlatform*)window->getPlatform())->getWindowsSampleUserInput().onKeyDown(VK_ESCAPE, 65537);
			}
			break;

		case WM_SIZE:
			if(window)
			{
				RECT rect;
				GetClientRect(hwnd, &rect);
				PxU32 width  = (PxU32)(rect.right-rect.left);
				PxU32 height = (PxU32)(rect.bottom-rect.top);
				window->onResize(width, height);
			}
			break;

		case WM_MOUSEMOVE:
			handleMouseEvent(WM_MOUSEMOVE, lParam, hwnd, window);
			break;

		case WM_LBUTTONDOWN:
			((WindowsPlatform*)window->getPlatform())->recenterMouseCursor(false);
			handleMouseEvent(WM_LBUTTONDOWN, lParam, hwnd, window);
			break;

		case WM_LBUTTONUP:
			handleMouseEvent(WM_LBUTTONUP, lParam, hwnd, window);
			break;

		case WM_RBUTTONDOWN:
			handleMouseEvent(WM_RBUTTONDOWN, lParam, hwnd, window);
			break;

		case WM_RBUTTONUP:
			handleMouseEvent(WM_RBUTTONUP, lParam, hwnd, window);
			break;

		case WM_MBUTTONDOWN:
			handleMouseEvent(WM_MBUTTONDOWN, lParam, hwnd, window);
			break;

		case WM_MBUTTONUP:
			handleMouseEvent(WM_MBUTTONUP, lParam, hwnd, window);
			break;

		case WM_CHAR:
			// PT: we need to catch this message to make a difference between lower & upper case characters
			if(window)
			{
				((WindowsPlatform*)window->getPlatform())->getWindowsSampleUserInput().onKeyDownEx(wParam);
			}
			break;

		case WM_SYSKEYDOWN:	// For F10 or ALT
			if (VK_F4 == wParam && (KF_ALTDOWN & HIWORD(lParam)))
			{
				if(window)
				{
					((WindowsPlatform*)window->getPlatform())->getWindowsSampleUserInput().onKeyDown(VK_ESCAPE, 65537);
					break;
				}
			}
			// no break, fall through
		case WM_KEYDOWN:
			if(window)
			{
				((WindowsPlatform*)window->getPlatform())->getWindowsSampleUserInput().onKeyDown(wParam, lParam);				
			}
			break;

		case WM_SYSKEYUP:
		case WM_KEYUP:
			if(window)
			{
				((WindowsPlatform*)window->getPlatform())->getWindowsSampleUserInput().onKeyUp(wParam, lParam);
			}
			break;

		case WM_PAINT:
			ValidateRect(hwnd, 0);
			break;
		default:
			return ::DefWindowProc(hwnd, msg, wParam, lParam);
		}
	}
	return 0; 
}

static ATOM registerWindowClass(HINSTANCE hInstance)
{
	static ATOM atom = 0;
	if(!atom)
	{
		WNDCLASSEX wcex;
		wcex.cbSize         = sizeof(WNDCLASSEX); 
		wcex.style          = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc    = (WNDPROC)windowProc;
		wcex.cbClsExtra     = 0;
		wcex.cbWndExtra     = sizeof(void*);
		wcex.hInstance      = hInstance;
		wcex.hIcon          = ::LoadIcon(hInstance, "SampleApplicationIcon");
		wcex.hCursor        = ::LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
		wcex.lpszMenuName   = 0;
		wcex.lpszClassName  = g_windowClassName;
		wcex.hIconSm        = ::LoadIcon(wcex.hInstance, "SampleApplicationIcon");
		atom = ::RegisterClassEx(&wcex);
	}
	return atom;
}

SamplePlatform*		SampleFramework::createPlatform(SampleRenderer::RendererWindow* _app)
{
	printf("Creating Windows platform abstraction.\n");
	SamplePlatform::setPlatform(new WindowsPlatform(_app));
	return SamplePlatform::platform();
}

void* WindowsPlatform::initializeD3D9()
{
	m_library   = 0;
	if(m_hwnd)
	{
#if defined(D3D_DEBUG_INFO)
#define D3D9_DLL "d3d9d.dll"
#else
#define D3D9_DLL "d3d9.dll"
#endif
		m_library = LoadLibraryA(D3D9_DLL);
		RENDERER_ASSERT(m_library, "Could not load " D3D9_DLL ".");
		if(!m_library)
		{
			MessageBoxA(0, "Could not load " D3D9_DLL ". Please install the latest DirectX End User Runtime available at www.microsoft.com/directx.", "Renderer Error.", MB_OK);
		}
#undef D3D9_DLL
		if(m_library)
		{
			typedef IDirect3D9* (WINAPI* LPDIRECT3DCREATE9)(UINT SDKVersion);
			LPDIRECT3DCREATE9 pDirect3DCreate9 = (LPDIRECT3DCREATE9)GetProcAddress(m_library, "Direct3DCreate9");
			RENDERER_ASSERT(pDirect3DCreate9, "Could not find Direct3DCreate9 function.");
			if(pDirect3DCreate9)
			{
				m_d3d = pDirect3DCreate9(D3D_SDK_VERSION);
			}
		}
	}
	return m_d3d;
}

void WindowsPlatform::showCursor(bool show)
{
	if(show != m_showCursor)
	{
		m_showCursor = show;
		PxI32 count = ShowCursor(show);
		PX_ASSERT((m_showCursor && (count == 0)) || (!m_showCursor && (count == -1)));
		PX_UNUSED(count);
	}
}

void* WindowsPlatform::compileProgram(void * context, 
										const char* assetDir, 
										const char *programPath, 
										physx::PxU64 profile, 
										const char* passString, 
										const char *entry, 
										const char **args)

{
#if defined(RENDERER_ENABLE_CG)
	char fullpath[1024];
	Ps::strlcpy(fullpath, 1024, assetDir);
	Ps::strlcat(fullpath, 1024, "shaders/");
	Ps::strlcat(fullpath, 1024, programPath);
	CGprogram program = cgCreateProgramFromFile(static_cast<CGcontext>(context), CG_SOURCE, fullpath, static_cast<CGprofile>(profile), entry, args);

	if (!program)
	{
		static bool ignoreErrors = false;
		if (!ignoreErrors)
		{
			const char* compileError = cgGetLastListing(static_cast<CGcontext>(context));
			int ret = MessageBoxA(0, compileError, "CG cgCreateProgramFromFile Error", MB_ABORTRETRYIGNORE);

			if (ret == IDABORT)
			{
				exit(0);
			}
			else if (ret == IDIGNORE)
			{
				ignoreErrors = true;
			}
			else
			{
				DebugBreak();
			}
		}
	}

	return program;
#else
	return NULL;
#endif
}

WindowsPlatform::WindowsPlatform(SampleRenderer::RendererWindow* _app) :
SamplePlatform(_app), 
m_d3d(NULL),
m_d3dDevice(NULL),
m_dxgiFactory(NULL),
m_dxgiSwap(NULL),
m_d3d11Device(NULL),
m_d3d11DeviceContext(NULL),
m_hwnd(0),
m_hdc(0),
m_hrc(0),
m_library(NULL),
m_dxgiLibrary(NULL),
m_d3d11Library(NULL),
m_ownsWindow(false),
m_isHandlingMessages(false),
m_destroyWindow(false),
m_hasFocus(true),
m_vsync(false)
{
	m_library = 0;

	// adjust ShowCursor display counter to be 0 or -1
	m_showCursor = true;
	PxI32 count = ShowCursor(true);
	while(count != 0)
		count = ShowCursor(count < 0);
	strcpy(m_platformName, "windows");

	m_mouseCursorPos = PxVec2(0);
	m_recenterMouseCursor = false;
	m_workaroundMouseMoved = false;
}

WindowsPlatform::~WindowsPlatform()
{
	RENDERER_ASSERT(!m_ownsWindow || m_hwnd==0, "RendererWindow was not closed before being destroyed.");
	if(m_d3d)
	{
		m_d3d->Release();
	}
	if(m_library) 
	{	
		FreeLibrary(m_library);
		m_library = 0;
	}
	if(m_dxgiLibrary) 
	{
		FreeLibrary(m_dxgiLibrary);
		m_dxgiLibrary = 0;
	}
	if(m_d3d11Library) 
	{
		FreeLibrary(m_d3d11Library);
		m_d3d11Library = 0;
	}
}

bool WindowsPlatform::isD3D9ok()
{
	if(m_library) 
	{
		return true;
	}
	return false;
}

bool WindowsPlatform::hasFocus() const
{
	return m_hasFocus;
}

void WindowsPlatform::setFocus(bool b)
{
	m_hasFocus = b;
}

bool WindowsPlatform::isOpen()
{
	if(m_hwnd && !m_destroyWindow) return true;
	return false;
}

void WindowsPlatform::getTitle(char *title, physx::PxU32 maxLength) const
{
	RENDERER_ASSERT(m_hwnd, "Tried to get the title of a window that was not opened.");
	if(m_hwnd)
	{
		GetWindowTextA(m_hwnd, title, maxLength);
	}
}

void WindowsPlatform::setTitle(const char *title)
{
	RENDERER_ASSERT(m_hwnd, "Tried to set the title of a window that was not opened.");
	if(m_hwnd)
	{
		::SetWindowTextA(m_hwnd, title);
	}
}

void WindowsPlatform::showMessage(const char* title, const char* message)
{
	printf("%s: %s\n", title, message);
}

bool WindowsPlatform::saveBitmap(const char* pFileName, physx::PxU32 width, physx::PxU32 height, physx::PxU32 sizeInBytes, const void* pData)
{
	bool bSuccess = false;
	HBITMAP bitmap = CreateBitmap(width, height, 1, 32, pData);
	if (bitmap)
	{
		CImage image;
		image.Attach(bitmap, CImage::DIBOR_TOPDOWN);
		bSuccess = SUCCEEDED(image.Save(pFileName, Gdiplus::ImageFormatBMP));
		DeleteObject(bitmap);
	}
	return bSuccess;
}

void WindowsPlatform::setMouseCursorRecentering(bool val)
{
	if (m_recenterMouseCursor != val)
	{
		m_recenterMouseCursor = val;
		if (m_recenterMouseCursor)
			recenterMouseCursor(false);
	}
}

bool WindowsPlatform::getMouseCursorRecentering() const
{
	return m_recenterMouseCursor;
}

void WindowsPlatform::recenterMouseCursor(bool generateEvent)
{
	if (m_recenterMouseCursor && m_app->hasFocus())
	{
		RECT rect;
		GetWindowRect(m_hwnd, (LPRECT)&rect);
		PxU32 winHeight = (PxU32)(rect.bottom-rect.top);
		PxU32 winWidth = (PxU32)(rect.right - rect.left);

		POINT winPos;
		GetCursorPos(&winPos);
		PxVec2 currentCoords(static_cast<PxReal>(winPos.x), winHeight - static_cast<PxReal>(winPos.y));

		PxI32 winCenterX = 0;
		PxI32 winCenterY = 0;
		{
			winCenterX = rect.left + PxI32(winWidth>>1);
			winCenterY = rect.top + PxI32(winHeight>>1);
		}
		SetCursorPos(winCenterX, winCenterY);
		PxVec2 centerCoords(static_cast<PxReal>(winCenterX), winHeight - static_cast<PxReal>(winCenterY));
		PxVec2 diff = currentCoords - centerCoords;

		//sometimes GetCursorPos [frame N+1] and SetCursorPos [frame N] don't agree by a few pixels, even if the mouse is not moving.
		//the windows event queue however knows that the mouse is not moving -> m_workaroundMouseMoved
		if (generateEvent && m_workaroundMouseMoved)
		{
			getWindowsSampleUserInput().doOnMouseMove(winCenterX, winHeight - winCenterY, diff.x, diff.y, MOUSE_MOVE);
		}
	
		m_mouseCursorPos = currentCoords;
		m_workaroundMouseMoved = false;
	}
}

void WindowsPlatform::setWindowSize(physx::PxU32 width, 
									physx::PxU32 height)
{
	bool fullscreen = false;
	RENDERER_ASSERT(m_hwnd, "Tried to resize a window that was not opened.");
	if(m_hwnd)
	{
		RECT rect;
		::GetWindowRect(m_hwnd, &rect);
		rect.right    = (LONG)(rect.left + width);
		rect.bottom   = (LONG)(rect.top  + height);
		RECT oldrect  = rect;
		DWORD dwstyle = (fullscreen ? g_fullscreenStyle : g_windowStyle);
		::AdjustWindowRect(&rect, dwstyle, 0);
		::MoveWindow(m_hwnd, (int)oldrect.left, (int)oldrect.top, (int)(rect.right-rect.left), (int)(rect.bottom-rect.top), 1);
	}
}

void WindowsPlatform::update()
{
	RENDERER_ASSERT(m_hwnd, "Tried to update a window that was not opened.");
	if(m_app->isOpen())
	{
		m_isHandlingMessages = true;
		MSG	msg;

		while(m_app->isOpen() && ::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		m_isHandlingMessages = false;
		if(m_hwnd && m_destroyWindow)
		{
			if(m_app->onClose())
			{
				::DestroyWindow(m_hwnd);
				m_hwnd = 0;
			}
			m_destroyWindow = false;
		}
		recenterMouseCursor(true);
	}
}

bool WindowsPlatform::openWindow(physx::PxU32& width, 
								physx::PxU32& height,
								const char* title,
								bool fullscreen) 
{
	bool ok = false;
	RENDERER_ASSERT(m_hwnd==0, "Attempting to open a window that is already opened");
	if(m_hwnd==0)
	{
		int offset = fullscreen ? 0 : 50;

		registerWindowClass((HINSTANCE)::GetModuleHandle(0));
		RECT winRect;
		winRect.left   = offset;
		winRect.top    = offset;
		winRect.right  = width + offset;
		winRect.bottom = height + offset;
		DWORD dwstyle  = (fullscreen ? g_fullscreenStyle : g_windowStyle);

		::AdjustWindowRect(&winRect, dwstyle, 0);

		// make sure the window fits in the main screen
		if (!fullscreen)
		{
			// check the work area of the primary display
			RECT screen;
			SystemParametersInfo(SPI_GETWORKAREA, 0, &screen, 0);

			if (winRect.right > screen.right)
			{
				int diff = winRect.right - screen.right;
				winRect.right -= diff;
				winRect.left = std::max<int>(0, winRect.left - diff);
			}

			if (winRect.bottom > screen.bottom)
			{
				int diff = winRect.bottom - screen.bottom;
				winRect.bottom -= diff;
				winRect.top = std::max<int>(0, winRect.top - diff);
			}
		}

		m_hwnd = ::CreateWindowA(g_windowClassName, title, dwstyle,
		                         winRect.left, winRect.top,
		                         winRect.right - winRect.left, winRect.bottom - winRect.top,
		                         0, 0, 0, 0);
		RENDERER_ASSERT(m_hwnd, "CreateWindow failed");
		if(m_hwnd)
		{
			ok = true;
			m_ownsWindow = true;
			ShowWindow(m_hwnd, SW_SHOWNORMAL);
			SetFocus(m_hwnd);
			SetWindowLongPtr(m_hwnd, GWLP_USERDATA, LONG_PTR(m_app));
		}
	}

	{
		RAWINPUTDEVICE rawInputDevice;
		rawInputDevice.usUsagePage	= 1;
		rawInputDevice.usUsage		= 6;
		rawInputDevice.dwFlags		= 0;
		rawInputDevice.hwndTarget	= NULL;

		BOOL status = RegisterRawInputDevices(&rawInputDevice, 1, sizeof(rawInputDevice));
		if(status!=TRUE)
		{
			DWORD err = GetLastError();
			printf("%d\n", err);
		}
	}

	return ok;
}

bool WindowsPlatform::useWindow(physx::PxU64 hwnd)
{
	m_hwnd = reinterpret_cast<HWND>(hwnd);
	return true;
}

bool WindowsPlatform::closeWindow() 
{
	if(m_hwnd)
	{
		if(m_isHandlingMessages)
		{
			m_destroyWindow = true;
		}
		else if(m_app->onClose())
		{
			::DestroyWindow(m_hwnd);
			m_hwnd = 0;
		}
	}

	return true;
}

size_t WindowsPlatform::getCWD(char* path, size_t len)
{
	return ::GetCurrentDirectory((DWORD)len, path);
}
void WindowsPlatform::setCWDToEXE(void) 
{
	char exepath[1024] = {0};
	GetModuleFileNameA(0, exepath, sizeof(exepath));

	if(exepath[0])
	{
		popPathSpec(exepath);
		(void)_chdir(exepath);
	}
}

void WindowsPlatform::setupRendererDescription(SampleRenderer::RendererDesc& renDesc) 
{
	renDesc.driver = SampleRenderer::Renderer::DRIVER_DIRECT3D11;
	renDesc.windowHandle = reinterpret_cast<physx::PxU64>(m_hwnd);
}

void WindowsPlatform::postRendererSetup(SampleRenderer::Renderer* renderer) 
{
	if(!renderer)
	{
		// quit if no renderer was created.  Nothing else to do.
		// error was output in createRenderer.
		exit(1);
	}
	char windowTitle[1024] = {0};
	m_app->getTitle(windowTitle, 1024);
	strcat_s(windowTitle, 1024, " : ");
	strcat_s(windowTitle, 1024, SampleRenderer::Renderer::getDriverTypeName(
												renderer->getDriverType()));
	m_app->setTitle(windowTitle);
}

void WindowsPlatform::doInput()
{
	m_windowsSampleUserInput.updateInput();
}

void WindowsPlatform::initializeOGLDisplay(const SampleRenderer::RendererDesc& desc,
										physx::PxU32& width, 
										physx::PxU32& height)
{
#if defined(RENDERER_ENABLE_OPENGL)
	m_hwnd = reinterpret_cast<HWND>(desc.windowHandle);

	RENDERER_ASSERT(m_hwnd, "Invalid window handle!");
	// Get the device context.
	m_hdc = GetDC(m_hwnd);

	RENDERER_ASSERT(m_hdc, "Invalid device context!");
	if(m_hdc)
	{
		setDCPixelFormat(m_hdc);
		m_hrc = wglCreateContext(m_hdc);
	}

	RENDERER_ASSERT(m_hrc, "Invalid render context!");
	if(m_hrc)
	{
		BOOL makeCurrentRet = wglMakeCurrent(m_hdc, m_hrc);
		RENDERER_ASSERT(m_hrc, "Unable to set current context!");
		if(!makeCurrentRet)
		{
			wglDeleteContext(m_hrc);
			m_hrc = 0;
		}
	}
#endif

	m_vsync = desc.vsync;
}

physx::PxU64 WindowsPlatform::getWindowHandle()
{
	return reinterpret_cast<physx::PxU64>(m_hwnd);
}

void WindowsPlatform::getWindowSize(PxU32& width, PxU32& height)
{
	if(m_hwnd)
	{
		RECT rect;
		GetClientRect(m_hwnd, &rect);
		width  = (PxU32)(rect.right  - rect.left);
		height = (PxU32)(rect.bottom - rect.top);
	}
}

void WindowsPlatform::freeDisplay() 
{
#if defined(RENDERER_ENABLE_OPENGL)
	if(m_hrc)
	{
		wglMakeCurrent(m_hdc, 0);
		wglDeleteContext(m_hrc);
		m_hrc = 0;
	}
#endif
}

bool WindowsPlatform::makeContextCurrent()
{
	bool ok = false;

#if defined(RENDERER_ENABLE_OPENGL)
	if(m_hdc && m_hrc)
	{
		if(wglGetCurrentContext()==m_hrc)
		{
			ok = true;
		}
		else
		{
			ok = wglMakeCurrent(m_hdc, m_hrc) ? true : false;
		}
	}
#endif

	return ok;
}

void WindowsPlatform::swapBuffers()
{
	SwapBuffers(m_hdc);
}

bool WindowsPlatform::isContextValid()
{
	if(!m_hdc) return false;
	if(!m_hrc) return false;
	return true;
}

void WindowsPlatform::postInitializeOGLDisplay()
{
#if defined(RENDERER_ENABLE_OPENGL)
	glewInit();

#if defined(GLEW_MX)
	wglewInit();
#endif

	if(WGLEW_EXT_swap_control)
	{
		wglSwapIntervalEXT(m_vsync ? -1 : 0);
	}
#endif
}

void WindowsPlatform::setOGLVsync(bool on)
{
#if defined(RENDERER_ENABLE_OPENGL)
	m_vsync = on;
	if(WGLEW_EXT_swap_control)
	{
		wglSwapIntervalEXT(m_vsync ? 1 : 0);
	}
#endif
}

physx::PxU32 WindowsPlatform::initializeD3D9Display(void * d3dPresentParameters, 
																char* m_deviceName, 
																physx::PxU32& width, 
																physx::PxU32& height,
																void * m_d3dDevice_out)
{
	D3DPRESENT_PARAMETERS* m_d3dPresentParams = static_cast<D3DPRESENT_PARAMETERS*>(d3dPresentParameters);

	UINT       adapter    = D3DADAPTER_DEFAULT;
	D3DDEVTYPE deviceType = D3DDEVTYPE_HAL;

	// check to see if fullscreen is requested...
	bool fullscreen = false;
	WINDOWINFO wininfo = {0};
	if(GetWindowInfo(m_hwnd, &wininfo))
	{
		if(wininfo.dwStyle & WS_POPUP)
		{
			fullscreen = true;
		}
	}

	// search for supported adapter mode.
	if(fullscreen)
	{
		RECT rect = {0};
		GetWindowRect(m_hwnd, &rect);
		m_d3dPresentParams->BackBufferFormat = D3DFMT_X8R8G8B8;
		width = (m_d3dPresentParams->BackBufferWidth  = rect.right-rect.left);
		height = (m_d3dPresentParams->BackBufferHeight = rect.bottom-rect.top);

		bool foundAdapterMode = false;
		const UINT numAdapterModes = m_d3d->GetAdapterModeCount(0, m_d3dPresentParams->BackBufferFormat);
		for(UINT i=0; i<numAdapterModes; i++)
		{
			D3DDISPLAYMODE mode = {0};
			m_d3d->EnumAdapterModes(0, m_d3dPresentParams->BackBufferFormat, i, &mode);
			if(mode.Width       == m_d3dPresentParams->BackBufferWidth  &&
				mode.Height      == m_d3dPresentParams->BackBufferHeight &&
				mode.RefreshRate >  m_d3dPresentParams->FullScreen_RefreshRateInHz)
			{
				m_d3dPresentParams->FullScreen_RefreshRateInHz = mode.RefreshRate;
				foundAdapterMode = true;
			}
		}
		RENDERER_ASSERT(foundAdapterMode, "Unable to find supported fullscreen Adapter Mode.");
		if(!foundAdapterMode) fullscreen = false;
	}

	// enable fullscreen mode.
	if(fullscreen)
	{
		m_d3dPresentParams->Windowed = 0;
	}

#if defined(RENDERER_ENABLE_NVPERFHUD)
	// NvPerfHud Support.
	UINT numAdapters = m_d3d->GetAdapterCount();
	for(UINT i=0; i<numAdapters; i++)
	{
		D3DADAPTER_IDENTIFIER9 identifier;
		m_d3d->GetAdapterIdentifier(i, 0, &identifier);
		if(strstr(identifier.Description, "PerfHUD"))
		{
			adapter    = i;
			deviceType = D3DDEVTYPE_REF;
			break;
		}
	}
#endif

	D3DADAPTER_IDENTIFIER9 adapterIdentifier;
	m_d3d->GetAdapterIdentifier(adapter, 0, &adapterIdentifier);
	strncpy_s(m_deviceName, 256, adapterIdentifier.Description, 256);

	HRESULT res = m_d3d->CreateDevice( adapter, deviceType,
		m_hwnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED,
		m_d3dPresentParams, &m_d3dDevice);
	*(static_cast<IDirect3DDevice9**>(m_d3dDevice_out)) = m_d3dDevice;
	return res;
}

physx::PxU32 WindowsPlatform::D3D9Present()
{
	return m_d3dDevice->Present(0, 0, m_hwnd, 0);
}
physx::PxU64 WindowsPlatform::getD3D9TextureFormat(SampleRenderer::RendererTexture2D::Format format)
{
	D3DFORMAT d3dFormat = D3DFMT_UNKNOWN;
	switch(format)
	{
	case SampleRenderer::RendererTexture2D::FORMAT_B8G8R8A8: d3dFormat = D3DFMT_A8R8G8B8; break;
	case SampleRenderer::RendererTexture2D::FORMAT_A8:       d3dFormat = D3DFMT_A8;       break;
	case SampleRenderer::RendererTexture2D::FORMAT_R32F:     d3dFormat = D3DFMT_R32F;     break;
	case SampleRenderer::RendererTexture2D::FORMAT_DXT1:     d3dFormat = D3DFMT_DXT1;     break;
	case SampleRenderer::RendererTexture2D::FORMAT_DXT3:     d3dFormat = D3DFMT_DXT3;     break;
	case SampleRenderer::RendererTexture2D::FORMAT_DXT5:     d3dFormat = D3DFMT_DXT5;     break;
	case SampleRenderer::RendererTexture2D::FORMAT_D16:      d3dFormat = D3DFMT_D16;      break;
	case SampleRenderer::RendererTexture2D::FORMAT_D24S8:    d3dFormat = D3DFMT_D24S8;    break;
	}
	return static_cast<physx::PxU64>(d3dFormat);
}

physx::PxU64 WindowsPlatform::getD3D11TextureFormat(SampleRenderer::RendererTexture2D::Format format)
{
#if defined(RENDERER_ENABLE_DIRECT3D11)
	DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;
	switch(format)
	{
	case SampleRenderer::RendererTexture2D::FORMAT_B8G8R8A8: dxgiFormat = DXGI_FORMAT_B8G8R8A8_UNORM; break;
	case SampleRenderer::RendererTexture2D::FORMAT_A8:       dxgiFormat = DXGI_FORMAT_A8_UNORM;       break;
	case SampleRenderer::RendererTexture2D::FORMAT_R32F:     dxgiFormat = DXGI_FORMAT_R32_FLOAT;     break;
	case SampleRenderer::RendererTexture2D::FORMAT_DXT1:     dxgiFormat = DXGI_FORMAT_BC1_UNORM;     break;
	case SampleRenderer::RendererTexture2D::FORMAT_DXT3:     dxgiFormat = DXGI_FORMAT_BC2_UNORM;     break;
	case SampleRenderer::RendererTexture2D::FORMAT_DXT5:     dxgiFormat = DXGI_FORMAT_BC3_UNORM;     break;
	case SampleRenderer::RendererTexture2D::FORMAT_D16:      dxgiFormat = DXGI_FORMAT_R16_TYPELESS;      break;
	}
#else
	PxU64 dxgiFormat = 0;
#endif
	return static_cast<physx::PxU64>(dxgiFormat);
}

void* WindowsPlatform::initializeD3D11()
{
	m_dxgiLibrary   = 0;
	m_d3d11Library  = 0;
#if defined(RENDERER_ENABLE_DIRECT3D11)
	if(m_hwnd)
	{
#if defined(D3D_DEBUG_INFO)
#define D3D11_DLL "d3d11d.dll"
#define DXGI_DLL "dxgid.dll"
#else
#define D3D11_DLL "d3d11.dll"
#define DXGI_DLL "dxgi.dll"
#endif
		m_dxgiLibrary = LoadLibraryA(DXGI_DLL);
		m_d3d11Library = LoadLibraryA(D3D11_DLL);
		RENDERER_ASSERT(m_dxgiLibrary, "Could not load " DXGI_DLL ".");
		RENDERER_ASSERT(m_d3d11Library, "Could not load " D3D11_DLL ".");
		if(!m_dxgiLibrary)
		{
			MessageBoxA(0, "Could not load " DXGI_DLL ". Please install the latest DirectX End User Runtime available at www.microsoft.com/directx.", "Renderer Error.", MB_OK);
		}
		if(!m_d3d11Library)
		{
			MessageBoxA(0, "Could not load " D3D11_DLL ". Please install the latest DirectX End User Runtime available at www.microsoft.com/directx.", "Renderer Error.", MB_OK);
		}
#undef D3D11_DLL
#undef DXGI_DLL
		if(m_dxgiLibrary)
		{
			typedef HRESULT     (WINAPI * LPCREATEDXGIFACTORY)(REFIID, void ** );
			LPCREATEDXGIFACTORY pCreateDXGIFactory = (LPCREATEDXGIFACTORY)GetProcAddress(m_dxgiLibrary, "CreateDXGIFactory1");
			RENDERER_ASSERT(pCreateDXGIFactory, "Could not find CreateDXGIFactory1 function.");
			if(pCreateDXGIFactory)
			{
				pCreateDXGIFactory(__uuidof(IDXGIFactory1), (void**)(&m_dxgiFactory));
			}
		}
	}
#endif
	return m_dxgiFactory;
}


bool WindowsPlatform::isD3D11ok()
{
	if(m_dxgiLibrary && m_d3d11Library) 
	{
		return true;
	}
	return false;
}

physx::PxU32 WindowsPlatform::initializeD3D11Display(void *dxgiSwapChainDesc, 
															  char *m_deviceName, 
													 physx::PxU32& width, 
													 physx::PxU32& height,
															  void *m_d3dDevice_out,
															  void *m_d3dDeviceContext_out,
															  void *m_dxgiSwap_out)
{
	HRESULT hr = S_OK;

#if defined(RENDERER_ENABLE_DIRECT3D11)
	ID3D11Device* pD3D11Device               = NULL;
	ID3D11DeviceContext* pD3D11DeviceContext = NULL;
	IDXGISwapChain* pSwapChain               = NULL;
	IDXGIFactory1* pDXGIFactory              = m_dxgiFactory;
	DXGI_SWAP_CHAIN_DESC* pSwapChainDesc     = static_cast<DXGI_SWAP_CHAIN_DESC*>(dxgiSwapChainDesc);
	
	hr = pDXGIFactory->MakeWindowAssociation(m_hwnd, 0);
	pSwapChainDesc->OutputWindow = m_hwnd;

	bool fullscreen = false;
	WINDOWINFO wininfo = {0};
	if(GetWindowInfo(m_hwnd, &wininfo))
	{
		if(wininfo.dwStyle & WS_POPUP)
		{
			fullscreen = true;
			pSwapChainDesc->Windowed = 0;
		}
	}
	
	PFN_D3D11_CREATE_DEVICE pD3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(m_d3d11Library, "D3D11CreateDevice");
	RENDERER_ASSERT(pD3D11CreateDevice, "Could not find D3D11CreateDeviceAndSwapChain.");
	if (pD3D11CreateDevice)
	{
		UINT i                  = 0; 
		IDXGIAdapter1* pAdapter = NULL; 
		std::vector<IDXGIAdapter1*> vAdapters; 
		while(pDXGIFactory->EnumAdapters1(i++, &pAdapter) != DXGI_ERROR_NOT_FOUND) 
		{ 
			vAdapters.push_back(pAdapter); 
		} 

		const D3D_FEATURE_LEVEL supportedFeatureLevels[] = 
		{
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			//D3D_FEATURE_LEVEL_9_3,
			//D3D_FEATURE_LEVEL_9_2,
			//D3D_FEATURE_LEVEL_9_1,
		};

		D3D11_CREATE_DEVICE_FLAG deviceFlags = (D3D11_CREATE_DEVICE_FLAG)(D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT);
#if PX_DEBUG
		deviceFlags = (D3D11_CREATE_DEVICE_FLAG)(deviceFlags | D3D11_CREATE_DEVICE_DEBUG);
#endif
		D3D_FEATURE_LEVEL deviceFeatureLevel = D3D_FEATURE_LEVEL_11_0;
		for (i = 0; i < vAdapters.size(); ++i)
		{
			hr = pD3D11CreateDevice(
				vAdapters[i],
				D3D_DRIVER_TYPE_UNKNOWN,
				NULL,
				deviceFlags,
				supportedFeatureLevels, 
				PX_ARRAY_SIZE(supportedFeatureLevels),
				D3D11_SDK_VERSION,
				&pD3D11Device,
				&deviceFeatureLevel,
				&pD3D11DeviceContext);

			if (SUCCEEDED(hr))
			{
				// Disable MSAA on sub DX10.1 devices
				if (deviceFeatureLevel <= D3D_FEATURE_LEVEL_10_0)
				{
					pSwapChainDesc->SampleDesc.Count   = 1;
					pSwapChainDesc->SampleDesc.Quality = 0;
				}
				hr = pDXGIFactory->CreateSwapChain(pD3D11Device, pSwapChainDesc, &pSwapChain);
			}

			if (SUCCEEDED(hr))
			{
				m_dxgiSwap = pSwapChain;
				m_d3d11Device = pD3D11Device;
				m_d3d11DeviceContext = pD3D11DeviceContext;
				*(static_cast<IDXGISwapChain**>(m_dxgiSwap_out)) = m_dxgiSwap;
				*(static_cast<ID3D11Device**>(m_d3dDevice_out)) = m_d3d11Device;
				*(static_cast<ID3D11DeviceContext**>(m_d3dDeviceContext_out)) = m_d3d11DeviceContext;
				break;
			}

			// If DXGI creation failed but device creation succeeded, we need to release both device and context
			if (pD3D11Device)        pD3D11Device->Release();
			if (pD3D11DeviceContext) pD3D11DeviceContext->Release();
		}

		for (i = 0; i < vAdapters.size(); ++i)
		{
			vAdapters[i]->Release();
		}
	} 
	RENDERER_ASSERT(m_dxgiSwap && m_d3d11Device && m_d3d11DeviceContext, "Unable to create D3D device and swap chain");
#endif

	return hr;
}

bool WindowsPlatform::makeSureDirectoryPathExists(const char* dirPath)
{
	// [Kai] Currently create the final folder only if the dirPath doesn't exist.
	// To create all the directories in the specified dirPath, use API
	// MakeSureDirectoryPathExists() declared in dbghelp.h
	bool ok = GetFileAttributes(dirPath) != -1;
	if (!ok)
		ok = CreateDirectory(dirPath, NULL) != 0;
	return ok;
}

physx::PxU32 WindowsPlatform::D3D11Present(bool vsync)
{
#if defined(RENDERER_ENABLE_DIRECT3D11)
	return m_dxgiSwap->Present(vsync ? 1 : 0, 0);
#else
	return 0;
#endif
}
