//--------------------------------------------------------------------------------------
// File: DXUT.h
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=320437
//--------------------------------------------------------------------------------------
#pragma once

#ifndef UNICODE
#error "DXUT requires a Unicode build."
#endif

#ifndef STRICT
#define STRICT
#endif

// If app hasn't choosen, set to work with Windows Vista and beyond
#ifndef WINVER
#define WINVER         0x0600
#endif
#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0600
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT   0x0600
#endif

// #define DXUT_AUTOLIB to automatically include the libs needed for DXUT 
#ifdef DXUT_AUTOLIB
#pragma comment( lib, "comctl32.lib" )
#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "d3dcompiler.lib" )
#pragma comment( lib, "ole32.lib" )
#pragma comment( lib, "uuid.lib" )
#endif

#pragma warning( disable : 4481 )

// Standard Windows includes
#if !defined(NOMINMAX)
#define NOMINMAX
#endif

#include <windows.h>
#include <initguid.h>
#include <assert.h>
#include <commctrl.h> // for InitCommonControls() 
#include <shellapi.h> // for ExtractIcon()
#include <new.h>      // for placement new
#include <shlobj.h>
#include <math.h>
#include <limits.h>
#include <stdio.h>

// CRT's memory leak detection
#if defined(DEBUG) || defined(_DEBUG)
#include <crtdbg.h>
#endif

// Direct3D11 includes
#include <d3dcommon.h>
#include <dxgi.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#ifdef USE_DIRECT3D11_2
#include <d3d11_2.h>
#endif

// DirectXMath includes
#include <DirectXMath.h>
#include <DirectXColors.h>

// WIC includes
// VS 2010's stdint.h conflicts with intsafe.h
#pragma warning(push)
#pragma warning(disable : 4005)
#include <wincodec.h>
#pragma warning(pop)

// XInput includes
#include <xinput.h>

// HRESULT translation for Direct3D and other APIs 
#include "dxerr.h"

// STL includes
#include <algorithm>
#include <memory>
#include <vector>

#if defined(DEBUG) || defined(_DEBUG)
#ifndef V
#define V(x)           { hr = (x); if( FAILED(hr) ) { DXUTTrace( __FILE__, (DWORD)__LINE__, hr, L#x, true ); } }
#endif
#ifndef V_RETURN
#define V_RETURN(x)    { hr = (x); if( FAILED(hr) ) { return DXUTTrace( __FILE__, (DWORD)__LINE__, hr, L#x, true ); } }
#endif
#else
#ifndef V
#define V(x)           { hr = (x); }
#endif
#ifndef V_RETURN
#define V_RETURN(x)    { hr = (x); if( FAILED(hr) ) { return hr; } }
#endif
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if (p) { delete (p);     (p) = nullptr; } }
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p) = nullptr; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p) = nullptr; } }
#endif

#ifndef D3DCOLOR_ARGB
#define D3DCOLOR_ARGB(a,r,g,b) \
    ((DWORD)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#endif

#define DXUT_VERSION 1107

//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------
struct DXUTD3D11DeviceSettings
{
    UINT AdapterOrdinal;
    D3D_DRIVER_TYPE DriverType;
    UINT Output;
    DXGI_SWAP_CHAIN_DESC sd;
    UINT32 CreateFlags;
    UINT32 SyncInterval;
    DWORD PresentFlags;
    bool AutoCreateDepthStencil; // DXUT will create the depth stencil resource and view if true
    DXGI_FORMAT AutoDepthStencilFormat;
    D3D_FEATURE_LEVEL DeviceFeatureLevel;
};

struct DXUTDeviceSettings
{
    D3D_FEATURE_LEVEL MinimumFeatureLevel;
    DXUTD3D11DeviceSettings d3d11;
};


//--------------------------------------------------------------------------------------
// Error codes
//--------------------------------------------------------------------------------------
#define DXUTERR_NODIRECT3D              MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0901)
#define DXUTERR_NOCOMPATIBLEDEVICES     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0902)
#define DXUTERR_MEDIANOTFOUND           MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0903)
#define DXUTERR_NONZEROREFCOUNT         MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0904)
#define DXUTERR_CREATINGDEVICE          MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0905)
#define DXUTERR_RESETTINGDEVICE         MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0906)
#define DXUTERR_CREATINGDEVICEOBJECTS   MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0907)
#define DXUTERR_RESETTINGDEVICEOBJECTS  MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0908)
#define DXUTERR_DEVICEREMOVED           MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x090A)


//--------------------------------------------------------------------------------------
// Callback registration 
//--------------------------------------------------------------------------------------

// General callbacks
typedef void    (CALLBACK *LPDXUTCALLBACKFRAMEMOVE)( _In_ double fTime, _In_ float fElapsedTime, _In_opt_ void* pUserContext );
typedef void    (CALLBACK *LPDXUTCALLBACKKEYBOARD)( _In_ UINT nChar, _In_ bool bKeyDown, _In_ bool bAltDown, _In_opt_ void* pUserContext );
typedef void    (CALLBACK *LPDXUTCALLBACKMOUSE)( _In_ bool bLeftButtonDown, _In_ bool bRightButtonDown, _In_ bool bMiddleButtonDown,
                                                 _In_ bool bSideButton1Down, _In_ bool bSideButton2Down, _In_ int nMouseWheelDelta,
                                                 _In_ int xPos, _In_ int yPos, _In_opt_ void* pUserContext );
typedef LRESULT (CALLBACK *LPDXUTCALLBACKMSGPROC)( _In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam,
                                                   _Out_ bool* pbNoFurtherProcessing, _In_opt_ void* pUserContext );
typedef void    (CALLBACK *LPDXUTCALLBACKTIMER)( _In_ UINT idEvent, _In_opt_ void* pUserContext );
typedef bool    (CALLBACK *LPDXUTCALLBACKMODIFYDEVICESETTINGS)( _In_ DXUTDeviceSettings* pDeviceSettings, _In_opt_ void* pUserContext );
typedef bool    (CALLBACK *LPDXUTCALLBACKDEVICEREMOVED)( _In_opt_ void* pUserContext );

class CD3D11EnumAdapterInfo;
class CD3D11EnumDeviceInfo;

// Direct3D 11 callbacks
typedef bool    (CALLBACK *LPDXUTCALLBACKISD3D11DEVICEACCEPTABLE)( _In_ const CD3D11EnumAdapterInfo *AdapterInfo, _In_ UINT Output, _In_ const CD3D11EnumDeviceInfo *DeviceInfo,
                                                                   _In_ DXGI_FORMAT BackBufferFormat, _In_ bool bWindowed, _In_opt_ void* pUserContext );
typedef HRESULT (CALLBACK *LPDXUTCALLBACKD3D11DEVICECREATED)( _In_ ID3D11Device* pd3dDevice, _In_ const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, _In_opt_ void* pUserContext );
typedef HRESULT (CALLBACK *LPDXUTCALLBACKD3D11SWAPCHAINRESIZED)( _In_ ID3D11Device* pd3dDevice, _In_ IDXGISwapChain *pSwapChain, _In_ const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, _In_opt_ void* pUserContext );
typedef void    (CALLBACK *LPDXUTCALLBACKD3D11FRAMERENDER)( _In_ ID3D11Device* pd3dDevice, _In_ ID3D11DeviceContext* pd3dImmediateContext, _In_ double fTime, _In_ float fElapsedTime, _In_opt_ void* pUserContext );
typedef void    (CALLBACK *LPDXUTCALLBACKD3D11SWAPCHAINRELEASING)( _In_opt_ void* pUserContext );
typedef void    (CALLBACK *LPDXUTCALLBACKD3D11DEVICEDESTROYED)( _In_opt_ void* pUserContext );

// General callbacks
void WINAPI DXUTSetCallbackFrameMove( _In_ LPDXUTCALLBACKFRAMEMOVE pCallback, _In_opt_ void* pUserContext = nullptr );
void WINAPI DXUTSetCallbackKeyboard( _In_ LPDXUTCALLBACKKEYBOARD pCallback, _In_opt_ void* pUserContext = nullptr );
void WINAPI DXUTSetCallbackMouse( _In_ LPDXUTCALLBACKMOUSE pCallback, bool bIncludeMouseMove = false, _In_opt_ void* pUserContext = nullptr );
void WINAPI DXUTSetCallbackMsgProc( _In_ LPDXUTCALLBACKMSGPROC pCallback, _In_opt_ void* pUserContext = nullptr );
void WINAPI DXUTSetCallbackDeviceChanging( _In_ LPDXUTCALLBACKMODIFYDEVICESETTINGS pCallback, _In_opt_ void* pUserContext = nullptr );
void WINAPI DXUTSetCallbackDeviceRemoved( _In_ LPDXUTCALLBACKDEVICEREMOVED pCallback, _In_opt_ void* pUserContext = nullptr );

// Direct3D 11 callbacks
void WINAPI DXUTSetCallbackD3D11DeviceAcceptable( _In_ LPDXUTCALLBACKISD3D11DEVICEACCEPTABLE pCallback, _In_opt_ void* pUserContext = nullptr );
void WINAPI DXUTSetCallbackD3D11DeviceCreated( _In_ LPDXUTCALLBACKD3D11DEVICECREATED pCallback, _In_opt_ void* pUserContext = nullptr );
void WINAPI DXUTSetCallbackD3D11SwapChainResized( _In_ LPDXUTCALLBACKD3D11SWAPCHAINRESIZED pCallback, _In_opt_ void* pUserContext = nullptr );
void WINAPI DXUTSetCallbackD3D11FrameRender( _In_ LPDXUTCALLBACKD3D11FRAMERENDER pCallback, _In_opt_ void* pUserContext = nullptr );
void WINAPI DXUTSetCallbackD3D11SwapChainReleasing( _In_ LPDXUTCALLBACKD3D11SWAPCHAINRELEASING pCallback, _In_opt_ void* pUserContext = nullptr );
void WINAPI DXUTSetCallbackD3D11DeviceDestroyed( _In_ LPDXUTCALLBACKD3D11DEVICEDESTROYED pCallback, _In_opt_ void* pUserContext = nullptr );


//--------------------------------------------------------------------------------------
// Initialization
//--------------------------------------------------------------------------------------
HRESULT WINAPI DXUTInit( _In_ bool bParseCommandLine = true, 
                         _In_ bool bShowMsgBoxOnError = true, 
                         _In_opt_ WCHAR* strExtraCommandLineParams = nullptr,
                         _In_ bool bThreadSafeDXUT = false );

// Choose either DXUTCreateWindow or DXUTSetWindow.  If using DXUTSetWindow, consider using DXUTStaticWndProc
HRESULT WINAPI DXUTCreateWindow( _In_z_ const WCHAR* strWindowTitle = L"Direct3D Window", 
                                 _In_opt_ HINSTANCE hInstance = nullptr, _In_opt_ HICON hIcon = nullptr, _In_opt_ HMENU hMenu = nullptr,
                                 _In_ int x = CW_USEDEFAULT, _In_ int y = CW_USEDEFAULT );
HRESULT WINAPI DXUTSetWindow( _In_ HWND hWndFocus, _In_ HWND hWndDeviceFullScreen, _In_ HWND hWndDeviceWindowed, _In_ bool bHandleMessages = true );
LRESULT CALLBACK DXUTStaticWndProc( _In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam );

// Choose either DXUTCreateDevice or DXUTCreateD3DDeviceFromSettings

HRESULT WINAPI DXUTCreateDevice(_In_ D3D_FEATURE_LEVEL reqFL, _In_ bool bWindowed= true, _In_ int nSuggestedWidth =0,_In_  int nSuggestedHeight =0 );
HRESULT WINAPI DXUTCreateDeviceFromSettings( _In_ DXUTDeviceSettings* pDeviceSettings, _In_ bool bClipWindowToSingleAdapter = true );

// Choose either DXUTMainLoop or implement your own main loop 
HRESULT WINAPI DXUTMainLoop( _In_opt_ HACCEL hAccel = nullptr );

// If not using DXUTMainLoop consider using DXUTRender3DEnvironment
void WINAPI DXUTRender3DEnvironment();


//--------------------------------------------------------------------------------------
// Common Tasks 
//--------------------------------------------------------------------------------------
HRESULT WINAPI DXUTToggleFullScreen();
HRESULT WINAPI DXUTToggleREF();
HRESULT WINAPI DXUTToggleWARP();
void    WINAPI DXUTPause( _In_ bool bPauseTime, _In_ bool bPauseRendering );
void    WINAPI DXUTSetConstantFrameTime( _In_ bool bConstantFrameTime, _In_ float fTimePerFrame = 0.0333f );
void    WINAPI DXUTSetCursorSettings( _In_ bool bShowCursorWhenFullScreen = false, _In_ bool bClipCursorWhenFullScreen = false );
void    WINAPI DXUTSetHotkeyHandling( _In_ bool bAltEnterToToggleFullscreen = true, _In_ bool bEscapeToQuit = true, _In_ bool bPauseToToggleTimePause = true );
void    WINAPI DXUTSetMultimonSettings( _In_ bool bAutoChangeAdapter = true );
void    WINAPI DXUTSetShortcutKeySettings( _In_ bool bAllowWhenFullscreen = false, _In_ bool bAllowWhenWindowed = true ); // Controls the Windows key, and accessibility shortcut keys
void    WINAPI DXUTSetWindowSettings( _In_ bool bCallDefWindowProc = true );
HRESULT WINAPI DXUTSetTimer( _In_ LPDXUTCALLBACKTIMER pCallbackTimer, _In_ float fTimeoutInSecs = 1.0f, _Out_opt_ UINT* pnIDEvent = nullptr, _In_opt_ void* pCallbackUserContext = nullptr );
HRESULT WINAPI DXUTKillTimer( _In_ UINT nIDEvent );
void    WINAPI DXUTResetFrameworkState();
void    WINAPI DXUTShutdown( _In_ int nExitCode = 0 );
void    WINAPI DXUTSetIsInGammaCorrectMode( _In_ bool bGammaCorrect );
bool    WINAPI DXUTGetMSAASwapChainCreated();


//--------------------------------------------------------------------------------------
// State Retrieval  
//--------------------------------------------------------------------------------------

// Direct3D 11.x (These do not addref unlike typical Get* APIs)
IDXGIFactory1*           WINAPI DXUTGetDXGIFactory(); 
IDXGISwapChain*          WINAPI DXUTGetDXGISwapChain();
const DXGI_SURFACE_DESC* WINAPI DXUTGetDXGIBackBufferSurfaceDesc();
HRESULT                  WINAPI DXUTSetupD3D11Views( _In_ ID3D11DeviceContext* pd3dDeviceContext ); // Supports immediate or deferred context
D3D_FEATURE_LEVEL        WINAPI DXUTGetD3D11DeviceFeatureLevel(); // Returns the D3D11 devices current feature level
ID3D11RenderTargetView*  WINAPI DXUTGetD3D11RenderTargetView();
ID3D11DepthStencilView*  WINAPI DXUTGetD3D11DepthStencilView();

ID3D11Device*            WINAPI DXUTGetD3D11Device();
ID3D11DeviceContext*     WINAPI DXUTGetD3D11DeviceContext();

ID3D11Device1*           WINAPI DXUTGetD3D11Device1();
ID3D11DeviceContext1*	 WINAPI DXUTGetD3D11DeviceContext1();

#ifdef USE_DIRECT3D11_2
ID3D11Device2*           WINAPI DXUTGetD3D11Device2();
ID3D11DeviceContext2*	 WINAPI DXUTGetD3D11DeviceContext2();
#endif

// General
DXUTDeviceSettings WINAPI DXUTGetDeviceSettings(); 
HINSTANCE WINAPI DXUTGetHINSTANCE();
HWND      WINAPI DXUTGetHWND();
HWND      WINAPI DXUTGetHWNDFocus();
HWND      WINAPI DXUTGetHWNDDeviceFullScreen();
HWND      WINAPI DXUTGetHWNDDeviceWindowed();
RECT      WINAPI DXUTGetWindowClientRect();
LONG      WINAPI DXUTGetWindowWidth();
LONG      WINAPI DXUTGetWindowHeight();
RECT      WINAPI DXUTGetWindowClientRectAtModeChange(); // Useful for returning to windowed mode with the same resolution as before toggle to full screen mode
RECT      WINAPI DXUTGetFullsceenClientRectAtModeChange(); // Useful for returning to full screen mode with the same resolution as before toggle to windowed mode
double    WINAPI DXUTGetTime();
float     WINAPI DXUTGetElapsedTime();
bool      WINAPI DXUTIsWindowed();
bool	  WINAPI DXUTIsInGammaCorrectMode();
float     WINAPI DXUTGetFPS();
LPCWSTR   WINAPI DXUTGetWindowTitle();
LPCWSTR   WINAPI DXUTGetFrameStats( _In_ bool bIncludeFPS = false );
LPCWSTR   WINAPI DXUTGetDeviceStats();

bool      WINAPI DXUTIsVsyncEnabled();
bool      WINAPI DXUTIsRenderingPaused();
bool      WINAPI DXUTIsTimePaused();
bool      WINAPI DXUTIsActive();
int       WINAPI DXUTGetExitCode();
bool      WINAPI DXUTGetShowMsgBoxOnError();
bool      WINAPI DXUTGetAutomation();  // Returns true if -automation parameter is used to launch the app
bool      WINAPI DXUTIsKeyDown( _In_ BYTE vKey ); // Pass a virtual-key code, ex. VK_F1, 'A', VK_RETURN, VK_LSHIFT, etc
bool      WINAPI DXUTWasKeyPressed( _In_ BYTE vKey );  // Like DXUTIsKeyDown() but return true only if the key was just pressed
bool      WINAPI DXUTIsMouseButtonDown( _In_ BYTE vButton ); // Pass a virtual-key code: VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2
HRESULT   WINAPI DXUTCreateState(); // Optional method to create DXUT's memory.  If its not called by the application it will be automatically called when needed
void      WINAPI DXUTDestroyState(); // Optional method to destroy DXUT's memory.  If its not called by the application it will be automatically called after the application exits WinMain 

//--------------------------------------------------------------------------------------
// DXUT core layer includes
//--------------------------------------------------------------------------------------
#include "DXUTmisc.h"
#include "DXUTDevice11.h"
