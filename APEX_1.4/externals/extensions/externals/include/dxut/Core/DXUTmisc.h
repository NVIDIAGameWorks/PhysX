//--------------------------------------------------------------------------------------
// File: DXUTMisc.h
//
// Helper functions for Direct3D programming.
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

//--------------------------------------------------------------------------------------
// XInput helper state/function
// This performs extra processing on XInput gamepad data to make it slightly more convenient to use
// 
// Example usage:
//
//      DXUT_GAMEPAD gamepad[4];
//      for( DWORD iPort=0; iPort<DXUT_MAX_CONTROLLERS; iPort++ )
//          DXUTGetGamepadState( iPort, gamepad[iPort] );
//
//--------------------------------------------------------------------------------------
#define DXUT_MAX_CONTROLLERS 4  // XInput handles up to 4 controllers 

struct DXUT_GAMEPAD
{
    // From XINPUT_GAMEPAD
    WORD wButtons;
    BYTE bLeftTrigger;
    BYTE bRightTrigger;
    SHORT sThumbLX;
    SHORT sThumbLY;
    SHORT sThumbRX;
    SHORT sThumbRY;

    // Device properties
    XINPUT_CAPABILITIES caps;
    bool bConnected; // If the controller is currently connected
    bool bInserted;  // If the controller was inserted this frame
    bool bRemoved;   // If the controller was removed this frame

    // Thumb stick values converted to range [-1,+1]
    float fThumbRX;
    float fThumbRY;
    float fThumbLX;
    float fThumbLY;

    // Records which buttons were pressed this frame.
    // These are only set on the first frame that the button is pressed
    WORD wPressedButtons;
    bool bPressedLeftTrigger;
    bool bPressedRightTrigger;

    // Last state of the buttons
    WORD wLastButtons;
    bool bLastLeftTrigger;
    bool bLastRightTrigger;
};

HRESULT DXUTGetGamepadState( _In_ DWORD dwPort, _In_ DXUT_GAMEPAD* pGamePad, _In_ bool bThumbstickDeadZone = true,
                             _In_ bool bSnapThumbstickToCardinals = true );
HRESULT DXUTStopRumbleOnAllControllers();
void DXUTEnableXInput( _In_ bool bEnable );


//--------------------------------------------------------------------------------------
// Takes a screen shot of a 32bit D3D11 back buffer and saves the images to a BMP or DDS file
//--------------------------------------------------------------------------------------

HRESULT DXUTSnapD3D11Screenshot( _In_z_ LPCWSTR szFileName, _In_ bool usedds = true );

//--------------------------------------------------------------------------------------
// Performs timer operations
// Use DXUTGetGlobalTimer() to get the global instance
//--------------------------------------------------------------------------------------
class CDXUTTimer
{
public:
    CDXUTTimer();

    void            Reset(); // resets the timer
    void            Start(); // starts the timer
    void            Stop();  // stop (or pause) the timer
    void            Advance(); // advance the timer by 0.1 seconds
    double          GetAbsoluteTime() const; // get the absolute system time
    double          GetTime() const; // get the current time
    float           GetElapsedTime(); // get the time that elapsed between Get*ElapsedTime() calls
    void            GetTimeValues( _Out_ double* pfTime, _Out_ double* pfAbsoluteTime, _Out_ float* pfElapsedTime ); // get all time values at once
    bool            IsStopped() const { return m_bTimerStopped; } // returns true if timer stopped

    // Limit the current thread to one processor (the current one). This ensures that timing code runs
    // on only one processor, and will not suffer any ill effects from power management.
    void            LimitThreadAffinityToCurrentProc();

protected:
    LARGE_INTEGER   GetAdjustedCurrentTime() const;

    bool m_bUsingQPF;
    bool m_bTimerStopped;
    LONGLONG m_llQPFTicksPerSec;

    LONGLONG m_llStopTime;
    LONGLONG m_llLastElapsedTime;
    LONGLONG m_llBaseTime;
};

CDXUTTimer*                 WINAPI DXUTGetGlobalTimer();


//--------------------------------------------------------------------------------------
// Returns the string for the given DXGI_FORMAT.
//       bWithPrefix determines whether the string should include the "DXGI_FORMAT_"
//--------------------------------------------------------------------------------------
LPCWSTR WINAPI DXUTDXGIFormatToString( _In_ DXGI_FORMAT format, _In_ bool bWithPrefix );


//--------------------------------------------------------------------------------------
// Debug printing support
// See dxerr.h for more debug printing support
//--------------------------------------------------------------------------------------
void WINAPI DXUTOutputDebugStringW( _In_z_ LPCWSTR strMsg, ... );
void WINAPI DXUTOutputDebugStringA( _In_z_ LPCSTR strMsg, ... );
HRESULT WINAPI DXUTTrace( _In_z_ const CHAR* strFile, _In_ DWORD dwLine, _In_ HRESULT hr, _In_z_ const WCHAR* strMsg, _In_ bool bPopMsgBox );
const WCHAR* WINAPI DXUTTraceWindowsMessage( _In_ UINT uMsg );

#ifdef UNICODE
#define DXUTOutputDebugString DXUTOutputDebugStringW
#else
#define DXUTOutputDebugString DXUTOutputDebugStringA
#endif

// These macros are very similar to dxerr's but it special cases the HRESULT defined
// by DXUT to pop better message boxes. 
#if defined(DEBUG) || defined(_DEBUG)
#define DXUT_ERR(str,hr)           DXUTTrace( __FILE__, (DWORD)__LINE__, hr, str, false )
#define DXUT_ERR_MSGBOX(str,hr)    DXUTTrace( __FILE__, (DWORD)__LINE__, hr, str, true )
#define DXUTTRACE                  DXUTOutputDebugString
#else
#define DXUT_ERR(str,hr)           (hr)
#define DXUT_ERR_MSGBOX(str,hr)    (hr)
#define DXUTTRACE                  (__noop)
#endif


//--------------------------------------------------------------------------------------
// Direct3D dynamic linking support -- calls top-level D3D APIs with graceful
// failure if APIs are not present.
//--------------------------------------------------------------------------------------

int WINAPI DXUT_Dynamic_D3DPERF_BeginEvent( _In_ DWORD col, _In_z_ LPCWSTR wszName );
int WINAPI DXUT_Dynamic_D3DPERF_EndEvent( void );
void WINAPI DXUT_Dynamic_D3DPERF_SetMarker( _In_ DWORD col, _In_z_ LPCWSTR wszName );
void WINAPI DXUT_Dynamic_D3DPERF_SetRegion( _In_ DWORD col, _In_z_ LPCWSTR wszName );
BOOL WINAPI DXUT_Dynamic_D3DPERF_QueryRepeatFrame( void );
void WINAPI DXUT_Dynamic_D3DPERF_SetOptions( _In_ DWORD dwOptions );
DWORD WINAPI DXUT_Dynamic_D3DPERF_GetStatus();
HRESULT WINAPI DXUT_Dynamic_CreateDXGIFactory1( _In_ REFIID rInterface, _Out_ void** ppOut );
HRESULT WINAPI DXUT_Dynamic_DXGIGetDebugInterface( _In_ REFIID rInterface, _Out_ void** ppOut );

HRESULT WINAPI DXUT_Dynamic_D3D11CreateDevice( _In_opt_ IDXGIAdapter* pAdapter,
                                               _In_ D3D_DRIVER_TYPE DriverType,
                                               _In_opt_ HMODULE Software,
                                               _In_ UINT32 Flags,
                                               _In_reads_(FeatureLevels) D3D_FEATURE_LEVEL* pFeatureLevels,
                                               _In_ UINT FeatureLevels,
                                               _In_ UINT32 SDKVersion,
                                               _Deref_out_ ID3D11Device** ppDevice,
                                               _Out_opt_ D3D_FEATURE_LEVEL* pFeatureLevel,
                                               _Out_opt_ ID3D11DeviceContext** ppImmediateContext );

bool DXUT_EnsureD3D11APIs();


//--------------------------------------------------------------------------------------
// Profiling/instrumentation support
//--------------------------------------------------------------------------------------

// Use DXUT_SetDebugName() to attach names to D3D objects for use by 
// SDKDebugLayer, PIX's object table, etc.
#if defined(PROFILE) || defined(DEBUG)
inline void DXUT_SetDebugName( _In_ IDXGIObject* pObj, _In_z_ const CHAR* pstrName )
{
    if ( pObj )
       pObj->SetPrivateData( WKPDID_D3DDebugObjectName, (UINT)strlen(pstrName), pstrName );
}
inline void DXUT_SetDebugName( _In_ ID3D11Device* pObj, _In_z_ const CHAR* pstrName )
{
    if ( pObj )
        pObj->SetPrivateData( WKPDID_D3DDebugObjectName, (UINT)strlen(pstrName), pstrName );
}
inline void DXUT_SetDebugName( _In_ ID3D11DeviceChild* pObj, _In_z_ const CHAR* pstrName )
{
    if ( pObj )
        pObj->SetPrivateData( WKPDID_D3DDebugObjectName, (UINT)strlen(pstrName), pstrName );
}
#else
#define DXUT_SetDebugName( pObj, pstrName )
#endif


//--------------------------------------------------------------------------------------
// Some D3DPERF APIs take a color that can be used when displaying user events in 
// performance analysis tools.  The following constants are provided for your 
// convenience, but you can use any colors you like.
//--------------------------------------------------------------------------------------
const DWORD DXUT_PERFEVENTCOLOR = 0xFFC86464;
const DWORD DXUT_PERFEVENTCOLOR2 = 0xFF64C864;
const DWORD DXUT_PERFEVENTCOLOR3 = 0xFF6464C8;

//--------------------------------------------------------------------------------------
// The following macros provide a convenient way for your code to call the D3DPERF 
// functions only when PROFILE is defined.  If PROFILE is not defined (as for the final 
// release version of a program), these macros evaluate to nothing, so no detailed event
// information is embedded in your shipping program.  It is recommended that you create
// and use three build configurations for your projects:
//     Debug (nonoptimized code, asserts active, PROFILE defined to assist debugging)
//     Profile (optimized code, asserts disabled, PROFILE defined to assist optimization)
//     Release (optimized code, asserts disabled, PROFILE not defined)
//--------------------------------------------------------------------------------------
#ifdef PROFILE
// PROFILE is defined, so these macros call the D3DPERF functions
#define DXUT_BeginPerfEvent( color, pstrMessage )   DXUT_Dynamic_D3DPERF_BeginEvent( color, pstrMessage )
#define DXUT_EndPerfEvent()                         DXUT_Dynamic_D3DPERF_EndEvent()
#define DXUT_SetPerfMarker( color, pstrMessage )    DXUT_Dynamic_D3DPERF_SetMarker( color, pstrMessage )
#else
// PROFILE is not defined, so these macros do nothing
#define DXUT_BeginPerfEvent( color, pstrMessage )   (__noop)
#define DXUT_EndPerfEvent()                         (__noop)
#define DXUT_SetPerfMarker( color, pstrMessage )    (__noop)
#endif

//--------------------------------------------------------------------------------------
// CDXUTPerfEventGenerator is a helper class that makes it easy to attach begin and end
// events to a block of code.  Simply define a CDXUTPerfEventGenerator variable anywhere 
// in a block of code, and the class's constructor will call DXUT_BeginPerfEvent when 
// the block of code begins, and the class's destructor will call DXUT_EndPerfEvent when 
// the block ends.
//--------------------------------------------------------------------------------------
class CDXUTPerfEventGenerator
{
public:
CDXUTPerfEventGenerator( _In_ DWORD color, _In_z_ LPCWSTR pstrMessage )
{
#ifdef PROFILE
    DXUT_BeginPerfEvent( color, pstrMessage );
#else
    UNREFERENCED_PARAMETER(color);
    UNREFERENCED_PARAMETER(pstrMessage);
#endif
}
~CDXUTPerfEventGenerator()
{
    DXUT_EndPerfEvent();
}
};


//--------------------------------------------------------------------------------------
// Multimon handling to support OSes with or without multimon API support.  
// Purposely avoiding the use of multimon.h so DXUT.lib doesn't require 
// COMPILE_MULTIMON_STUBS and cause complication with MFC or other users of multimon.h
//--------------------------------------------------------------------------------------
#ifndef MONITOR_DEFAULTTOPRIMARY
#define MONITORINFOF_PRIMARY        0x00000001
#define MONITOR_DEFAULTTONULL       0x00000000
#define MONITOR_DEFAULTTOPRIMARY    0x00000001
#define MONITOR_DEFAULTTONEAREST    0x00000002
typedef struct tagMONITORINFO
{
    DWORD cbSize;
    RECT rcMonitor;
    RECT rcWork;
    DWORD dwFlags;
}                           MONITORINFO, *LPMONITORINFO;
typedef struct tagMONITORINFOEXW : public tagMONITORINFO
{
    WCHAR szDevice[CCHDEVICENAME];
}                           MONITORINFOEXW, *LPMONITORINFOEXW;
typedef MONITORINFOEXW      MONITORINFOEX;
typedef LPMONITORINFOEXW    LPMONITORINFOEX;
#endif

HMONITOR WINAPI DXUTMonitorFromWindow( _In_ HWND hWnd, _In_ DWORD dwFlags );
HMONITOR WINAPI DXUTMonitorFromRect( _In_ LPCRECT lprcScreenCoords, _In_ DWORD dwFlags );
BOOL WINAPI DXUTGetMonitorInfo( _In_ HMONITOR hMonitor, _Out_ LPMONITORINFO lpMonitorInfo );
void WINAPI DXUTGetDesktopResolution( _In_ UINT AdapterOrdinal, _Out_ UINT* pWidth, _Out_ UINT* pHeight );


//--------------------------------------------------------------------------------------
// Helper functions to create SRGB formats from typeless formats and vice versa
//--------------------------------------------------------------------------------------
DXGI_FORMAT MAKE_SRGB( _In_ DXGI_FORMAT format );
DXGI_FORMAT MAKE_TYPELESS( _In_ DXGI_FORMAT format );
