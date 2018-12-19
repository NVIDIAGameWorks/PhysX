//--------------------------------------------------------------------------------------
// File: DXUTMisc.cpp
//
// Shortcut macros and functions for using DX objects
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
#include "dxut.h"
#include <xinput.h>

#include "ScreenGrab.h"


#define DXUT_GAMEPAD_TRIGGER_THRESHOLD      30

CDXUTTimer* WINAPI DXUTGetGlobalTimer()
{
    // Using an accessor function gives control of the construction order
    static CDXUTTimer timer;
    return &timer;
}


//--------------------------------------------------------------------------------------
CDXUTTimer::CDXUTTimer()
{
    m_bTimerStopped = true;
    m_llQPFTicksPerSec = 0;

    m_llStopTime = 0;
    m_llLastElapsedTime = 0;
    m_llBaseTime = 0;

    // Use QueryPerformanceFrequency to get the frequency of the counter
    LARGE_INTEGER qwTicksPerSec = { 0 };
    QueryPerformanceFrequency( &qwTicksPerSec );
    m_llQPFTicksPerSec = qwTicksPerSec.QuadPart;
}


//--------------------------------------------------------------------------------------
void CDXUTTimer::Reset()
{
    LARGE_INTEGER qwTime = GetAdjustedCurrentTime();

    m_llBaseTime = qwTime.QuadPart;
    m_llLastElapsedTime = qwTime.QuadPart;
    m_llStopTime = 0;
    m_bTimerStopped = FALSE;
}


//--------------------------------------------------------------------------------------
void CDXUTTimer::Start()
{
    // Get the current time
    LARGE_INTEGER qwTime = { 0 };
    QueryPerformanceCounter( &qwTime );

    if( m_bTimerStopped )
        m_llBaseTime += qwTime.QuadPart - m_llStopTime;
    m_llStopTime = 0;
    m_llLastElapsedTime = qwTime.QuadPart;
    m_bTimerStopped = FALSE;
}


//--------------------------------------------------------------------------------------
void CDXUTTimer::Stop()
{
    if( !m_bTimerStopped )
    {
        LARGE_INTEGER qwTime = { 0 };
        QueryPerformanceCounter( &qwTime );
        m_llStopTime = qwTime.QuadPart;
        m_llLastElapsedTime = qwTime.QuadPart;
        m_bTimerStopped = TRUE;
    }
}


//--------------------------------------------------------------------------------------
void CDXUTTimer::Advance()
{
    m_llStopTime += m_llQPFTicksPerSec / 10;
}


//--------------------------------------------------------------------------------------
double CDXUTTimer::GetAbsoluteTime() const
{
    LARGE_INTEGER qwTime = { 0 };
    QueryPerformanceCounter( &qwTime );

    double fTime = qwTime.QuadPart / ( double )m_llQPFTicksPerSec;

    return fTime;
}


//--------------------------------------------------------------------------------------
double CDXUTTimer::GetTime() const
{
    LARGE_INTEGER qwTime = GetAdjustedCurrentTime();

    double fAppTime = ( double )( qwTime.QuadPart - m_llBaseTime ) / ( double )m_llQPFTicksPerSec;

    return fAppTime;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTTimer::GetTimeValues( double* pfTime, double* pfAbsoluteTime, float* pfElapsedTime )
{
    assert( pfTime && pfAbsoluteTime && pfElapsedTime );

    LARGE_INTEGER qwTime = GetAdjustedCurrentTime();

    float fElapsedTime = (float) ((double) ( qwTime.QuadPart - m_llLastElapsedTime ) / (double) m_llQPFTicksPerSec);
    m_llLastElapsedTime = qwTime.QuadPart;

    // Clamp the timer to non-negative values to ensure the timer is accurate.
    // fElapsedTime can be outside this range if processor goes into a 
    // power save mode or we somehow get shuffled to another processor.  
    // However, the main thread should call SetThreadAffinityMask to ensure that 
    // we don't get shuffled to another processor.  Other worker threads should NOT call 
    // SetThreadAffinityMask, but use a shared copy of the timer data gathered from 
    // the main thread.
    if( fElapsedTime < 0.0f )
        fElapsedTime = 0.0f;

    *pfAbsoluteTime = qwTime.QuadPart / ( double )m_llQPFTicksPerSec;
    *pfTime = ( qwTime.QuadPart - m_llBaseTime ) / ( double )m_llQPFTicksPerSec;
    *pfElapsedTime = fElapsedTime;
}


//--------------------------------------------------------------------------------------
float CDXUTTimer::GetElapsedTime()
{
    LARGE_INTEGER qwTime = GetAdjustedCurrentTime();

    double fElapsedTime = (float) ((double) ( qwTime.QuadPart - m_llLastElapsedTime ) / (double) m_llQPFTicksPerSec);
    m_llLastElapsedTime = qwTime.QuadPart;

    // See the explanation about clamping in CDXUTTimer::GetTimeValues()
    if( fElapsedTime < 0.0f )
        fElapsedTime = 0.0f;

    return ( float )fElapsedTime;
}


//--------------------------------------------------------------------------------------
// If stopped, returns time when stopped otherwise returns current time
//--------------------------------------------------------------------------------------
LARGE_INTEGER CDXUTTimer::GetAdjustedCurrentTime() const
{
    LARGE_INTEGER qwTime;
    if( m_llStopTime != 0 )
        qwTime.QuadPart = m_llStopTime;
    else
        QueryPerformanceCounter( &qwTime );
    return qwTime;
}

//--------------------------------------------------------------------------------------
// Limit the current thread to one processor (the current one). This ensures that timing code 
// runs on only one processor, and will not suffer any ill effects from power management.
// See "Game Timing and Multicore Processors" for more details
//--------------------------------------------------------------------------------------
void CDXUTTimer::LimitThreadAffinityToCurrentProc()
{
    HANDLE hCurrentProcess = GetCurrentProcess();

    // Get the processor affinity mask for this process
    DWORD_PTR dwProcessAffinityMask = 0;
    DWORD_PTR dwSystemAffinityMask = 0;

    if( GetProcessAffinityMask( hCurrentProcess, &dwProcessAffinityMask, &dwSystemAffinityMask ) != 0 &&
        dwProcessAffinityMask )
    {
        // Find the lowest processor that our process is allows to run against
        DWORD_PTR dwAffinityMask = ( dwProcessAffinityMask & ( ( ~dwProcessAffinityMask ) + 1 ) );

        // Set this as the processor that our thread must always run against
        // This must be a subset of the process affinity mask
        HANDLE hCurrentThread = GetCurrentThread();
        if( INVALID_HANDLE_VALUE != hCurrentThread )
        {
            SetThreadAffinityMask( hCurrentThread, dwAffinityMask );
            CloseHandle( hCurrentThread );
        }
    }

    CloseHandle( hCurrentProcess );
}


//--------------------------------------------------------------------------------------
// Returns the string for the given DXGI_FORMAT.
//--------------------------------------------------------------------------------------
#define DXUTDXGIFMTSTR( a ) case a: pstr = L#a; break;

_Use_decl_annotations_
LPCWSTR WINAPI DXUTDXGIFormatToString( DXGI_FORMAT format, bool bWithPrefix )
{
    WCHAR* pstr = nullptr;
    switch( format )
    {
        DXUTDXGIFMTSTR(DXGI_FORMAT_R32G32B32A32_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R32G32B32A32_FLOAT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R32G32B32A32_UINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R32G32B32A32_SINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R32G32B32_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R32G32B32_FLOAT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R32G32B32_UINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R32G32B32_SINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R16G16B16A16_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R16G16B16A16_FLOAT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R16G16B16A16_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R16G16B16A16_UINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R16G16B16A16_SNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R16G16B16A16_SINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R32G32_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R32G32_FLOAT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R32G32_UINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R32G32_SINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R32G8X24_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_D32_FLOAT_S8X24_UINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_X32_TYPELESS_G8X24_UINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R10G10B10A2_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R10G10B10A2_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R10G10B10A2_UINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R11G11B10_FLOAT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R8G8B8A8_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R8G8B8A8_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R8G8B8A8_UINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R8G8B8A8_SNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R8G8B8A8_SINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R16G16_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R16G16_FLOAT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R16G16_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R16G16_UINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R16G16_SNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R16G16_SINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R32_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_D32_FLOAT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R32_FLOAT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R32_UINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R32_SINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R24G8_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_D24_UNORM_S8_UINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R24_UNORM_X8_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_X24_TYPELESS_G8_UINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R8G8_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R8G8_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R8G8_UINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R8G8_SNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R8G8_SINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R16_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R16_FLOAT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_D16_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R16_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R16_UINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R16_SNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R16_SINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R8_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R8_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R8_UINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R8_SNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R8_SINT)
        DXUTDXGIFMTSTR(DXGI_FORMAT_A8_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R1_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R9G9B9E5_SHAREDEXP)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R8G8_B8G8_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_G8R8_G8B8_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC1_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC1_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC1_UNORM_SRGB)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC2_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC2_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC2_UNORM_SRGB)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC3_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC3_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC3_UNORM_SRGB)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC4_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC4_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC4_SNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC5_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC5_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC5_SNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_B5G6R5_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_B5G5R5A1_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_B8G8R8A8_UNORM)

        // DXGI 1.1
        DXUTDXGIFMTSTR(DXGI_FORMAT_B8G8R8X8_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_B8G8R8A8_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)
        DXUTDXGIFMTSTR(DXGI_FORMAT_B8G8R8X8_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC6H_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC6H_UF16)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC6H_SF16)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC7_TYPELESS)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC7_UNORM)
        DXUTDXGIFMTSTR(DXGI_FORMAT_BC7_UNORM_SRGB)

        // DXGI 1.2
        DXUTDXGIFMTSTR(DXGI_FORMAT_B4G4R4A4_UNORM)

        default:
            pstr = L"Unknown format"; break;
    }
    if( bWithPrefix || !wcsstr( pstr, L"DXGI_FORMAT_" ) )
        return pstr;
    else
        return pstr + wcslen( L"DXGI_FORMAT_" );
}

#undef DXUTDXGIFMTSTR


//--------------------------------------------------------------------------------------
// Outputs to the debug stream a formatted Unicode string with a variable-argument list.
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
VOID WINAPI DXUTOutputDebugStringW( LPCWSTR strMsg, ... )
{
#if defined(DEBUG) || defined(_DEBUG)
    WCHAR strBuffer[512];

    va_list args;
    va_start(args, strMsg);
    vswprintf_s( strBuffer, 512, strMsg, args );
    strBuffer[511] = L'\0';
    va_end(args);

    OutputDebugString( strBuffer );
#else
    UNREFERENCED_PARAMETER( strMsg );
#endif
}


//--------------------------------------------------------------------------------------
// Outputs to the debug stream a formatted MBCS string with a variable-argument list.
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
VOID WINAPI DXUTOutputDebugStringA( LPCSTR strMsg, ... )
{
#if defined(DEBUG) || defined(_DEBUG)
    CHAR strBuffer[512];

    va_list args;
    va_start(args, strMsg);
    sprintf_s( strBuffer, 512, strMsg, args );
    strBuffer[511] = '\0';
    va_end(args);

    OutputDebugStringA( strBuffer );
#else
    UNREFERENCED_PARAMETER( strMsg );
#endif
}


//--------------------------------------------------------------------------------------
// Direct3D dynamic linking support -- calls top-level D3D APIs with graceful
// failure if APIs are not present.
//--------------------------------------------------------------------------------------

// Function prototypes
typedef INT         (WINAPI * LPD3DPERF_BEGINEVENT)(DWORD, LPCWSTR);
typedef INT         (WINAPI * LPD3DPERF_ENDEVENT)(void);
typedef VOID        (WINAPI * LPD3DPERF_SETMARKER)(DWORD, LPCWSTR);
typedef VOID        (WINAPI * LPD3DPERF_SETREGION)(DWORD, LPCWSTR);
typedef BOOL        (WINAPI * LPD3DPERF_QUERYREPEATFRAME)(void);
typedef VOID        (WINAPI * LPD3DPERF_SETOPTIONS)( DWORD dwOptions );
typedef DWORD       (WINAPI * LPD3DPERF_GETSTATUS)();
typedef HRESULT     (WINAPI * LPCREATEDXGIFACTORY)(REFIID, void ** );
typedef HRESULT     (WINAPI * LPDXGIGETDEBUGINTERFACE)(REFIID, void ** );

// Module and function pointers
static HMODULE                              s_hModD3D9 = nullptr;
static LPD3DPERF_BEGINEVENT                 s_DynamicD3DPERF_BeginEvent = nullptr;
static LPD3DPERF_ENDEVENT                   s_DynamicD3DPERF_EndEvent = nullptr;
static LPD3DPERF_SETMARKER                  s_DynamicD3DPERF_SetMarker = nullptr;
static LPD3DPERF_SETREGION                  s_DynamicD3DPERF_SetRegion = nullptr;
static LPD3DPERF_QUERYREPEATFRAME           s_DynamicD3DPERF_QueryRepeatFrame = nullptr;
static LPD3DPERF_SETOPTIONS                 s_DynamicD3DPERF_SetOptions = nullptr;
static LPD3DPERF_GETSTATUS                  s_DynamicD3DPERF_GetStatus = nullptr;
static HMODULE                              s_hModDXGI = nullptr;
static HMODULE                              s_hModDXGIDebug = nullptr;
static LPCREATEDXGIFACTORY                  s_DynamicCreateDXGIFactory = nullptr;
static LPDXGIGETDEBUGINTERFACE              s_DynamicDXGIGetDebugInterface = nullptr;
static HMODULE                              s_hModD3D11 = nullptr;
static PFN_D3D11_CREATE_DEVICE              s_DynamicD3D11CreateDevice = nullptr;

// Ensure function pointers are initialized
static bool DXUT_EnsureD3D9APIs()
{
    // If the module is non-NULL, this function has already been called.  Note
    // that this doesn't guarantee that all ProcAddresses were found.
    if( s_hModD3D9 )
        return true;

    // This could fail in theory, but not on any modern version of Windows
    s_hModD3D9 = LoadLibraryEx( L"d3d9.dll", nullptr, 0x00000800 /* LOAD_LIBRARY_SEARCH_SYSTEM32 */ );
    if( s_hModD3D9 )
    {
        // TODO - Use 11.1 perf APIs instead?
        s_DynamicD3DPERF_BeginEvent = reinterpret_cast<LPD3DPERF_BEGINEVENT>( GetProcAddress( s_hModD3D9, "D3DPERF_BeginEvent" ) );
        s_DynamicD3DPERF_EndEvent = reinterpret_cast<LPD3DPERF_ENDEVENT>( GetProcAddress( s_hModD3D9, "D3DPERF_EndEvent" ) );
        s_DynamicD3DPERF_SetMarker = reinterpret_cast<LPD3DPERF_SETMARKER>( GetProcAddress( s_hModD3D9, "D3DPERF_SetMarker" ) );
        s_DynamicD3DPERF_SetRegion = reinterpret_cast<LPD3DPERF_SETREGION>( GetProcAddress( s_hModD3D9, "D3DPERF_SetRegion" ) );
        s_DynamicD3DPERF_QueryRepeatFrame = reinterpret_cast<LPD3DPERF_QUERYREPEATFRAME>( GetProcAddress( s_hModD3D9, "D3DPERF_QueryRepeatFrame" ) );
        s_DynamicD3DPERF_SetOptions = reinterpret_cast<LPD3DPERF_SETOPTIONS>( GetProcAddress( s_hModD3D9, "D3DPERF_SetOptions" ) );
        s_DynamicD3DPERF_GetStatus = reinterpret_cast<LPD3DPERF_GETSTATUS>( GetProcAddress( s_hModD3D9, "D3DPERF_GetStatus" ) );
    }

    return s_hModD3D9 != nullptr;
}

bool DXUT_EnsureD3D11APIs()
{
    // If both modules are non-NULL, this function has already been called.  Note
    // that this doesn't guarantee that all ProcAddresses were found.
    if( s_hModD3D11 && s_hModDXGI )
        return true;

    // This may fail if Direct3D 11 isn't installed
    s_hModD3D11 = LoadLibraryEx( L"d3d11.dll", nullptr, 0x00000800 /* LOAD_LIBRARY_SEARCH_SYSTEM32 */ );
    if( s_hModD3D11 )
    {
        s_DynamicD3D11CreateDevice = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>( GetProcAddress( s_hModD3D11, "D3D11CreateDevice" ) );
    }

    if( !s_DynamicCreateDXGIFactory )
    {
        s_hModDXGI = LoadLibraryEx( L"dxgi.dll", nullptr, 0x00000800 /* LOAD_LIBRARY_SEARCH_SYSTEM32 */ );
        if( s_hModDXGI )
        {
            s_DynamicCreateDXGIFactory = reinterpret_cast<LPCREATEDXGIFACTORY>( GetProcAddress( s_hModDXGI, "CreateDXGIFactory1" ) );
        }

        if ( !s_DynamicDXGIGetDebugInterface )
        {
            s_hModDXGIDebug = LoadLibraryEx( L"dxgidebug.dll", nullptr, 0x00000800 /* LOAD_LIBRARY_SEARCH_SYSTEM32 */ );
            if ( s_hModDXGIDebug )
            {
                s_DynamicDXGIGetDebugInterface = reinterpret_cast<LPDXGIGETDEBUGINTERFACE>( GetProcAddress( s_hModDXGIDebug, "DXGIGetDebugInterface" ) );
            }
        }

        return ( s_hModDXGI ) && ( s_hModD3D11 );
    }

    return s_hModD3D11 != nullptr;
}

int WINAPI DXUT_Dynamic_D3DPERF_BeginEvent( _In_ DWORD col, _In_z_ LPCWSTR wszName )
{
    if( DXUT_EnsureD3D9APIs() && s_DynamicD3DPERF_BeginEvent )
        return s_DynamicD3DPERF_BeginEvent( col, wszName );
    else
        return -1;
}

int WINAPI DXUT_Dynamic_D3DPERF_EndEvent()
{
    if( DXUT_EnsureD3D9APIs() && s_DynamicD3DPERF_EndEvent )
        return s_DynamicD3DPERF_EndEvent();
    else
        return -1;
}

void WINAPI DXUT_Dynamic_D3DPERF_SetMarker( _In_ DWORD col, _In_z_ LPCWSTR wszName )
{
    if( DXUT_EnsureD3D9APIs() && s_DynamicD3DPERF_SetMarker )
        s_DynamicD3DPERF_SetMarker( col, wszName );
}

void WINAPI DXUT_Dynamic_D3DPERF_SetRegion( _In_ DWORD col, _In_z_ LPCWSTR wszName )
{
    if( DXUT_EnsureD3D9APIs() && s_DynamicD3DPERF_SetRegion )
        s_DynamicD3DPERF_SetRegion( col, wszName );
}

BOOL WINAPI DXUT_Dynamic_D3DPERF_QueryRepeatFrame()
{
    if( DXUT_EnsureD3D9APIs() && s_DynamicD3DPERF_QueryRepeatFrame )
        return s_DynamicD3DPERF_QueryRepeatFrame();
    else
        return FALSE;
}

void WINAPI DXUT_Dynamic_D3DPERF_SetOptions( _In_ DWORD dwOptions )
{
    if( DXUT_EnsureD3D9APIs() && s_DynamicD3DPERF_SetOptions )
        s_DynamicD3DPERF_SetOptions( dwOptions );
}

DWORD WINAPI DXUT_Dynamic_D3DPERF_GetStatus()
{
    if( DXUT_EnsureD3D9APIs() && s_DynamicD3DPERF_GetStatus )
        return s_DynamicD3DPERF_GetStatus();
    else
        return 0;
}

_Use_decl_annotations_
HRESULT WINAPI DXUT_Dynamic_CreateDXGIFactory1( REFIID rInterface, void** ppOut )
{
    if( DXUT_EnsureD3D11APIs() && s_DynamicCreateDXGIFactory )
        return s_DynamicCreateDXGIFactory( rInterface, ppOut );
    else
        return DXUTERR_NODIRECT3D;
}

_Use_decl_annotations_
HRESULT WINAPI DXUT_Dynamic_DXGIGetDebugInterface( REFIID rInterface, void** ppOut )
{
    if( DXUT_EnsureD3D11APIs() && s_DynamicDXGIGetDebugInterface )
        return s_DynamicDXGIGetDebugInterface( rInterface, ppOut );
    else
        return E_NOTIMPL;
}

_Use_decl_annotations_
HRESULT WINAPI DXUT_Dynamic_D3D11CreateDevice( IDXGIAdapter* pAdapter,
                                               D3D_DRIVER_TYPE DriverType,
                                               HMODULE Software,
                                               UINT32 Flags,
                                               D3D_FEATURE_LEVEL* pFeatureLevels,
                                               UINT FeatureLevels,
                                               UINT32 SDKVersion,
                                               ID3D11Device** ppDevice,
                                               D3D_FEATURE_LEVEL* pFeatureLevel,
                                               ID3D11DeviceContext** ppImmediateContext )
{
    if( DXUT_EnsureD3D11APIs() && s_DynamicD3D11CreateDevice )
        return s_DynamicD3D11CreateDevice( pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels,
                                           SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext );
    else
        return DXUTERR_NODIRECT3D;
}

#define TRACE_ID(iD) case iD: return L#iD;

//--------------------------------------------------------------------------------------
const WCHAR* WINAPI DXUTTraceWindowsMessage( _In_ UINT uMsg )
{
    switch( uMsg )
    {
        TRACE_ID(WM_NULL);
        TRACE_ID(WM_CREATE);
        TRACE_ID(WM_DESTROY);
        TRACE_ID(WM_MOVE);
        TRACE_ID(WM_SIZE);
        TRACE_ID(WM_ACTIVATE);
        TRACE_ID(WM_SETFOCUS);
        TRACE_ID(WM_KILLFOCUS);
        TRACE_ID(WM_ENABLE);
        TRACE_ID(WM_SETREDRAW);
        TRACE_ID(WM_SETTEXT);
        TRACE_ID(WM_GETTEXT);
        TRACE_ID(WM_GETTEXTLENGTH);
        TRACE_ID(WM_PAINT);
        TRACE_ID(WM_CLOSE);
        TRACE_ID(WM_QUERYENDSESSION);
        TRACE_ID(WM_QUERYOPEN);
        TRACE_ID(WM_ENDSESSION);
        TRACE_ID(WM_QUIT);
        TRACE_ID(WM_ERASEBKGND);
        TRACE_ID(WM_SYSCOLORCHANGE);
        TRACE_ID(WM_SHOWWINDOW);
        TRACE_ID(WM_WININICHANGE);
        TRACE_ID(WM_DEVMODECHANGE);
        TRACE_ID(WM_ACTIVATEAPP);
        TRACE_ID(WM_FONTCHANGE);
        TRACE_ID(WM_TIMECHANGE);
        TRACE_ID(WM_CANCELMODE);
        TRACE_ID(WM_SETCURSOR);
        TRACE_ID(WM_MOUSEACTIVATE);
        TRACE_ID(WM_CHILDACTIVATE);
        TRACE_ID(WM_QUEUESYNC);
        TRACE_ID(WM_GETMINMAXINFO);
        TRACE_ID(WM_PAINTICON);
        TRACE_ID(WM_ICONERASEBKGND);
        TRACE_ID(WM_NEXTDLGCTL);
        TRACE_ID(WM_SPOOLERSTATUS);
        TRACE_ID(WM_DRAWITEM);
        TRACE_ID(WM_MEASUREITEM);
        TRACE_ID(WM_DELETEITEM);
        TRACE_ID(WM_VKEYTOITEM);
        TRACE_ID(WM_CHARTOITEM);
        TRACE_ID(WM_SETFONT);
        TRACE_ID(WM_GETFONT);
        TRACE_ID(WM_SETHOTKEY);
        TRACE_ID(WM_GETHOTKEY);
        TRACE_ID(WM_QUERYDRAGICON);
        TRACE_ID(WM_COMPAREITEM);
        TRACE_ID(WM_GETOBJECT);
        TRACE_ID(WM_COMPACTING);
        TRACE_ID(WM_COMMNOTIFY);
        TRACE_ID(WM_WINDOWPOSCHANGING);
        TRACE_ID(WM_WINDOWPOSCHANGED);
        TRACE_ID(WM_POWER);
        TRACE_ID(WM_COPYDATA);
        TRACE_ID(WM_CANCELJOURNAL);
        TRACE_ID(WM_NOTIFY);
        TRACE_ID(WM_INPUTLANGCHANGEREQUEST);
        TRACE_ID(WM_INPUTLANGCHANGE);
        TRACE_ID(WM_TCARD);
        TRACE_ID(WM_HELP);
        TRACE_ID(WM_USERCHANGED);
        TRACE_ID(WM_NOTIFYFORMAT);
        TRACE_ID(WM_CONTEXTMENU);
        TRACE_ID(WM_STYLECHANGING);
        TRACE_ID(WM_STYLECHANGED);
        TRACE_ID(WM_DISPLAYCHANGE);
        TRACE_ID(WM_GETICON);
        TRACE_ID(WM_SETICON);
        TRACE_ID(WM_NCCREATE);
        TRACE_ID(WM_NCDESTROY);
        TRACE_ID(WM_NCCALCSIZE);
        TRACE_ID(WM_NCHITTEST);
        TRACE_ID(WM_NCPAINT);
        TRACE_ID(WM_NCACTIVATE);
        TRACE_ID(WM_GETDLGCODE);
        TRACE_ID(WM_SYNCPAINT);
        TRACE_ID(WM_NCMOUSEMOVE);
        TRACE_ID(WM_NCLBUTTONDOWN);
        TRACE_ID(WM_NCLBUTTONUP);
        TRACE_ID(WM_NCLBUTTONDBLCLK);
        TRACE_ID(WM_NCRBUTTONDOWN);
        TRACE_ID(WM_NCRBUTTONUP);
        TRACE_ID(WM_NCRBUTTONDBLCLK);
        TRACE_ID(WM_NCMBUTTONDOWN);
        TRACE_ID(WM_NCMBUTTONUP);
        TRACE_ID(WM_NCMBUTTONDBLCLK);
        TRACE_ID(WM_NCXBUTTONDOWN);
        TRACE_ID(WM_NCXBUTTONUP);
        TRACE_ID(WM_NCXBUTTONDBLCLK);
        TRACE_ID(WM_INPUT);
        TRACE_ID(WM_KEYDOWN);
        TRACE_ID(WM_KEYUP);
        TRACE_ID(WM_CHAR);
        TRACE_ID(WM_DEADCHAR);
        TRACE_ID(WM_SYSKEYDOWN);
        TRACE_ID(WM_SYSKEYUP);
        TRACE_ID(WM_SYSCHAR);
        TRACE_ID(WM_SYSDEADCHAR);
        TRACE_ID(WM_UNICHAR);
        TRACE_ID(WM_IME_STARTCOMPOSITION);
        TRACE_ID(WM_IME_ENDCOMPOSITION);
        TRACE_ID(WM_IME_COMPOSITION);
        TRACE_ID(WM_INITDIALOG);
        TRACE_ID(WM_COMMAND);
        TRACE_ID(WM_SYSCOMMAND);
        TRACE_ID(WM_TIMER);
        TRACE_ID(WM_HSCROLL);
        TRACE_ID(WM_VSCROLL);
        TRACE_ID(WM_INITMENU);
        TRACE_ID(WM_INITMENUPOPUP);
        TRACE_ID(WM_MENUSELECT);
        TRACE_ID(WM_MENUCHAR);
        TRACE_ID(WM_ENTERIDLE);
        TRACE_ID(WM_MENURBUTTONUP);
        TRACE_ID(WM_MENUDRAG);
        TRACE_ID(WM_MENUGETOBJECT);
        TRACE_ID(WM_UNINITMENUPOPUP);
        TRACE_ID(WM_MENUCOMMAND);
        TRACE_ID(WM_CHANGEUISTATE);
        TRACE_ID(WM_UPDATEUISTATE);
        TRACE_ID(WM_QUERYUISTATE);
        TRACE_ID(WM_CTLCOLORMSGBOX);
        TRACE_ID(WM_CTLCOLOREDIT);
        TRACE_ID(WM_CTLCOLORLISTBOX);
        TRACE_ID(WM_CTLCOLORBTN);
        TRACE_ID(WM_CTLCOLORDLG);
        TRACE_ID(WM_CTLCOLORSCROLLBAR);
        TRACE_ID(WM_CTLCOLORSTATIC);
        TRACE_ID(MN_GETHMENU);
        TRACE_ID(WM_MOUSEMOVE);
        TRACE_ID(WM_LBUTTONDOWN);
        TRACE_ID(WM_LBUTTONUP);
        TRACE_ID(WM_LBUTTONDBLCLK);
        TRACE_ID(WM_RBUTTONDOWN);
        TRACE_ID(WM_RBUTTONUP);
        TRACE_ID(WM_RBUTTONDBLCLK);
        TRACE_ID(WM_MBUTTONDOWN);
        TRACE_ID(WM_MBUTTONUP);
        TRACE_ID(WM_MBUTTONDBLCLK);
        TRACE_ID(WM_MOUSEWHEEL);
        TRACE_ID(WM_XBUTTONDOWN);
        TRACE_ID(WM_XBUTTONUP);
        TRACE_ID(WM_XBUTTONDBLCLK);
        TRACE_ID(WM_PARENTNOTIFY);
        TRACE_ID(WM_ENTERMENULOOP);
        TRACE_ID(WM_EXITMENULOOP);
        TRACE_ID(WM_NEXTMENU);
        TRACE_ID(WM_SIZING);
        TRACE_ID(WM_CAPTURECHANGED);
        TRACE_ID(WM_MOVING);
        TRACE_ID(WM_POWERBROADCAST);
        TRACE_ID(WM_DEVICECHANGE);
        TRACE_ID(WM_MDICREATE);
        TRACE_ID(WM_MDIDESTROY);
        TRACE_ID(WM_MDIACTIVATE);
        TRACE_ID(WM_MDIRESTORE);
        TRACE_ID(WM_MDINEXT);
        TRACE_ID(WM_MDIMAXIMIZE);
        TRACE_ID(WM_MDITILE);
        TRACE_ID(WM_MDICASCADE);
        TRACE_ID(WM_MDIICONARRANGE);
        TRACE_ID(WM_MDIGETACTIVE);
        TRACE_ID(WM_MDISETMENU);
        TRACE_ID(WM_ENTERSIZEMOVE);
        TRACE_ID(WM_EXITSIZEMOVE);
        TRACE_ID(WM_DROPFILES);
        TRACE_ID(WM_MDIREFRESHMENU);
        TRACE_ID(WM_IME_SETCONTEXT);
        TRACE_ID(WM_IME_NOTIFY);
        TRACE_ID(WM_IME_CONTROL);
        TRACE_ID(WM_IME_COMPOSITIONFULL);
        TRACE_ID(WM_IME_SELECT);
        TRACE_ID(WM_IME_CHAR);
        TRACE_ID(WM_IME_REQUEST);
        TRACE_ID(WM_IME_KEYDOWN);
        TRACE_ID(WM_IME_KEYUP);
        TRACE_ID(WM_MOUSEHOVER);
        TRACE_ID(WM_MOUSELEAVE);
        TRACE_ID(WM_NCMOUSEHOVER);
        TRACE_ID(WM_NCMOUSELEAVE);
        TRACE_ID(WM_WTSSESSION_CHANGE);
        TRACE_ID(WM_TABLET_FIRST);
        TRACE_ID(WM_TABLET_LAST);
        TRACE_ID(WM_CUT);
        TRACE_ID(WM_COPY);
        TRACE_ID(WM_PASTE);
        TRACE_ID(WM_CLEAR);
        TRACE_ID(WM_UNDO);
        TRACE_ID(WM_RENDERFORMAT);
        TRACE_ID(WM_RENDERALLFORMATS);
        TRACE_ID(WM_DESTROYCLIPBOARD);
        TRACE_ID(WM_DRAWCLIPBOARD);
        TRACE_ID(WM_PAINTCLIPBOARD);
        TRACE_ID(WM_VSCROLLCLIPBOARD);
        TRACE_ID(WM_SIZECLIPBOARD);
        TRACE_ID(WM_ASKCBFORMATNAME);
        TRACE_ID(WM_CHANGECBCHAIN);
        TRACE_ID(WM_HSCROLLCLIPBOARD);
        TRACE_ID(WM_QUERYNEWPALETTE);
        TRACE_ID(WM_PALETTEISCHANGING);
        TRACE_ID(WM_PALETTECHANGED);
        TRACE_ID(WM_HOTKEY);
        TRACE_ID(WM_PRINT);
        TRACE_ID(WM_PRINTCLIENT);
        TRACE_ID(WM_APPCOMMAND);
        TRACE_ID(WM_THEMECHANGED);
        TRACE_ID(WM_HANDHELDFIRST);
        TRACE_ID(WM_HANDHELDLAST);
        TRACE_ID(WM_AFXFIRST);
        TRACE_ID(WM_AFXLAST);
        TRACE_ID(WM_PENWINFIRST);
        TRACE_ID(WM_PENWINLAST);
        TRACE_ID(WM_APP);
        default:
            return L"Unknown";
    }
}


//--------------------------------------------------------------------------------------
// Multimon API handling for OSes with or without multimon API support
//--------------------------------------------------------------------------------------
#define DXUT_PRIMARY_MONITOR ((HMONITOR)0x12340042)
typedef HMONITOR ( WINAPI* LPMONITORFROMWINDOW )( HWND, DWORD );
typedef BOOL ( WINAPI* LPGETMONITORINFO )( HMONITOR, LPMONITORINFO );
typedef HMONITOR ( WINAPI* LPMONITORFROMRECT )( LPCRECT lprcScreenCoords, DWORD dwFlags );

#pragma warning( suppress : 6101 )
_Use_decl_annotations_
BOOL WINAPI DXUTGetMonitorInfo( HMONITOR hMonitor, LPMONITORINFO lpMonitorInfo )
{
    static bool s_bInited = false;
    static LPGETMONITORINFO s_pFnGetMonitorInfo = nullptr;
    if( !s_bInited )
    {
        s_bInited = true;
        HMODULE hUser32 = GetModuleHandle( L"USER32" );
        if( hUser32 )
        {
            s_pFnGetMonitorInfo = reinterpret_cast<LPGETMONITORINFO>( GetProcAddress( hUser32, "GetMonitorInfoW" ) );
        }
    }

    if( s_pFnGetMonitorInfo )
        return s_pFnGetMonitorInfo( hMonitor, lpMonitorInfo );

    RECT rcWork;
    if( ( hMonitor == DXUT_PRIMARY_MONITOR ) && lpMonitorInfo && ( lpMonitorInfo->cbSize >= sizeof( MONITORINFO ) ) &&
        SystemParametersInfoA( SPI_GETWORKAREA, 0, &rcWork, 0 ) )
    {
        lpMonitorInfo->rcMonitor.left = 0;
        lpMonitorInfo->rcMonitor.top = 0;
        lpMonitorInfo->rcMonitor.right = GetSystemMetrics( SM_CXSCREEN );
        lpMonitorInfo->rcMonitor.bottom = GetSystemMetrics( SM_CYSCREEN );
        lpMonitorInfo->rcWork = rcWork;
        lpMonitorInfo->dwFlags = MONITORINFOF_PRIMARY;
        return TRUE;
    }
    return FALSE;
}


_Use_decl_annotations_
HMONITOR WINAPI DXUTMonitorFromWindow( HWND hWnd, DWORD dwFlags )
{
    static bool s_bInited = false;
    static LPMONITORFROMWINDOW s_pFnGetMonitorFromWindow = nullptr;
    if( !s_bInited )
    {
        s_bInited = true;
        HMODULE hUser32 = GetModuleHandle( L"USER32" );
        if( hUser32 ) s_pFnGetMonitorFromWindow = reinterpret_cast<LPMONITORFROMWINDOW>( GetProcAddress( hUser32,
                                                                                         "MonitorFromWindow" ) );
    }

    if( s_pFnGetMonitorFromWindow )
        return s_pFnGetMonitorFromWindow( hWnd, dwFlags );
    else
        return DXUT_PRIMARY_MONITOR;
}


_Use_decl_annotations_
HMONITOR WINAPI DXUTMonitorFromRect( LPCRECT lprcScreenCoords, DWORD dwFlags )
{
    static bool s_bInited = false;
    static LPMONITORFROMRECT s_pFnGetMonitorFromRect = nullptr;
    if( !s_bInited )
    {
        s_bInited = true;
        HMODULE hUser32 = GetModuleHandle( L"USER32" );
        if( hUser32 ) s_pFnGetMonitorFromRect = reinterpret_cast<LPMONITORFROMRECT>( GetProcAddress( hUser32, "MonitorFromRect" ) );
    }

    if( s_pFnGetMonitorFromRect )
        return s_pFnGetMonitorFromRect( lprcScreenCoords, dwFlags );
    else
        return DXUT_PRIMARY_MONITOR;
}


//--------------------------------------------------------------------------------------
// Get the desktop resolution of an adapter. This isn't the same as the current resolution 
// from GetAdapterDisplayMode since the device might be fullscreen 
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void WINAPI DXUTGetDesktopResolution( UINT AdapterOrdinal, UINT* pWidth, UINT* pHeight )
{
    auto DeviceSettings = DXUTGetDeviceSettings();

    WCHAR strDeviceName[256] = {0};
    DEVMODE devMode;
    ZeroMemory( &devMode, sizeof( DEVMODE ) );
    devMode.dmSize = sizeof( DEVMODE );

    auto pd3dEnum = DXUTGetD3D11Enumeration();
    assert( pd3dEnum );
    _Analysis_assume_( pd3dEnum );
    auto pOutputInfo = pd3dEnum->GetOutputInfo( AdapterOrdinal, DeviceSettings.d3d11.Output );
    if( pOutputInfo )
    {
        wcscpy_s( strDeviceName, 256, pOutputInfo->Desc.DeviceName );
    }

    EnumDisplaySettings( strDeviceName, ENUM_REGISTRY_SETTINGS, &devMode );

    if( pWidth )
        *pWidth = devMode.dmPelsWidth;
    if( pHeight )
        *pHeight = devMode.dmPelsHeight;
}


//--------------------------------------------------------------------------------------
// Display error msg box to help debug 
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT WINAPI DXUTTrace( const CHAR* strFile, DWORD dwLine, HRESULT hr,
                          const WCHAR* strMsg, bool bPopMsgBox )
{
    bool bShowMsgBoxOnError = DXUTGetShowMsgBoxOnError();
    if( bPopMsgBox && bShowMsgBoxOnError == false )
        bPopMsgBox = false;

    WCHAR buff[ MAX_PATH ];
    int result = MultiByteToWideChar( CP_ACP,
                                      MB_PRECOMPOSED, 
                                      strFile,
                                      -1,
                                      buff,
                                      MAX_PATH );
    if ( !result )
    {
        wcscpy_s( buff, L"*ERROR*" );
    }

    return DXTraceW( buff, dwLine, hr, strMsg, bPopMsgBox );
}

typedef DWORD ( WINAPI* LPXINPUTGETSTATE )( DWORD dwUserIndex, XINPUT_STATE* pState );
typedef DWORD ( WINAPI* LPXINPUTSETSTATE )( DWORD dwUserIndex, XINPUT_VIBRATION* pVibration );
typedef DWORD ( WINAPI* LPXINPUTGETCAPABILITIES )( DWORD dwUserIndex, DWORD dwFlags,
                                                   XINPUT_CAPABILITIES* pCapabilities );
typedef void ( WINAPI* LPXINPUTENABLE )( BOOL bEnable );

//--------------------------------------------------------------------------------------
// Does extra processing on XInput data to make it slightly more convenient to use
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DXUTGetGamepadState( DWORD dwPort, DXUT_GAMEPAD* pGamePad, bool bThumbstickDeadZone,
                             bool bSnapThumbstickToCardinals )
{
    if( dwPort >= DXUT_MAX_CONTROLLERS || !pGamePad )
        return E_FAIL;

    static LPXINPUTGETSTATE s_pXInputGetState = nullptr;
    static LPXINPUTGETCAPABILITIES s_pXInputGetCapabilities = nullptr;
    if( !s_pXInputGetState || !s_pXInputGetCapabilities )
    {
        HINSTANCE hInst = LoadLibraryEx( XINPUT_DLL, nullptr, 0x00000800 /* LOAD_LIBRARY_SEARCH_SYSTEM32 */ );
        if( hInst )
        {
            s_pXInputGetState = reinterpret_cast<LPXINPUTGETSTATE>( GetProcAddress( hInst, "XInputGetState" ) );
            s_pXInputGetCapabilities = reinterpret_cast<LPXINPUTGETCAPABILITIES>( GetProcAddress( hInst, "XInputGetCapabilities" ) );
        }
    }
    if( !s_pXInputGetState )
        return E_FAIL;

    XINPUT_STATE InputState;
    DWORD dwResult = s_pXInputGetState( dwPort, &InputState );

    // Track insertion and removals
    BOOL bWasConnected = pGamePad->bConnected;
    pGamePad->bConnected = ( dwResult == ERROR_SUCCESS );
    pGamePad->bRemoved = ( bWasConnected && !pGamePad->bConnected );
    pGamePad->bInserted = ( !bWasConnected && pGamePad->bConnected );

    // Don't update rest of the state if not connected
    if( !pGamePad->bConnected )
        return S_OK;

    // Store the capabilities of the device
    if( pGamePad->bInserted )
    {
        ZeroMemory( pGamePad, sizeof( DXUT_GAMEPAD ) );
        pGamePad->bConnected = true;
        pGamePad->bInserted = true;
        if( s_pXInputGetCapabilities )
            s_pXInputGetCapabilities( dwPort, XINPUT_DEVTYPE_GAMEPAD, &pGamePad->caps );
    }

    // Copy gamepad to local structure (assumes that XINPUT_GAMEPAD at the front in CONTROLER_STATE)
    memcpy( pGamePad, &InputState.Gamepad, sizeof( XINPUT_GAMEPAD ) );

    if( bSnapThumbstickToCardinals )
    {
        // Apply deadzone to each axis independantly to slightly snap to up/down/left/right
        if( pGamePad->sThumbLX < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
            pGamePad->sThumbLX > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE )
            pGamePad->sThumbLX = 0;
        if( pGamePad->sThumbLY < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
            pGamePad->sThumbLY > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE )
            pGamePad->sThumbLY = 0;
        if( pGamePad->sThumbRX < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
            pGamePad->sThumbRX > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE )
            pGamePad->sThumbRX = 0;
        if( pGamePad->sThumbRY < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
            pGamePad->sThumbRY > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE )
            pGamePad->sThumbRY = 0;
    }
    else if( bThumbstickDeadZone )
    {
        // Apply deadzone if centered
        if( ( pGamePad->sThumbLX < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
              pGamePad->sThumbLX > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ) &&
            ( pGamePad->sThumbLY < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
              pGamePad->sThumbLY > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ) )
        {
            pGamePad->sThumbLX = 0;
            pGamePad->sThumbLY = 0;
        }
        if( ( pGamePad->sThumbRX < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
              pGamePad->sThumbRX > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ) &&
            ( pGamePad->sThumbRY < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
              pGamePad->sThumbRY > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ) )
        {
            pGamePad->sThumbRX = 0;
            pGamePad->sThumbRY = 0;
        }
    }

    // Convert [-1,+1] range
    pGamePad->fThumbLX = pGamePad->sThumbLX / 32767.0f;
    pGamePad->fThumbLY = pGamePad->sThumbLY / 32767.0f;
    pGamePad->fThumbRX = pGamePad->sThumbRX / 32767.0f;
    pGamePad->fThumbRY = pGamePad->sThumbRY / 32767.0f;

    // Get the boolean buttons that have been pressed since the last call. 
    // Each button is represented by one bit.
    pGamePad->wPressedButtons = ( pGamePad->wLastButtons ^ pGamePad->wButtons ) & pGamePad->wButtons;
    pGamePad->wLastButtons = pGamePad->wButtons;

    // Figure out if the left trigger has been pressed or released
    bool bPressed = ( pGamePad->bLeftTrigger > DXUT_GAMEPAD_TRIGGER_THRESHOLD );
    pGamePad->bPressedLeftTrigger = ( bPressed ) ? !pGamePad->bLastLeftTrigger : false;
    pGamePad->bLastLeftTrigger = bPressed;

    // Figure out if the right trigger has been pressed or released
    bPressed = ( pGamePad->bRightTrigger > DXUT_GAMEPAD_TRIGGER_THRESHOLD );
    pGamePad->bPressedRightTrigger = ( bPressed ) ? !pGamePad->bLastRightTrigger : false;
    pGamePad->bLastRightTrigger = bPressed;

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Don't pause the game or deactive the window without first stopping rumble otherwise 
// the controller will continue to rumble
//--------------------------------------------------------------------------------------
void DXUTEnableXInput( _In_ bool bEnable )
{
    static LPXINPUTENABLE s_pXInputEnable = nullptr;
    if( !s_pXInputEnable )
    {
        HINSTANCE hInst = LoadLibraryEx( XINPUT_DLL, nullptr, 0x00000800 /* LOAD_LIBRARY_SEARCH_SYSTEM32 */ );
        if( hInst )
            s_pXInputEnable = reinterpret_cast<LPXINPUTENABLE>( GetProcAddress( hInst, "XInputEnable" ) );
    }

    if( s_pXInputEnable )
        s_pXInputEnable( bEnable );
}


//--------------------------------------------------------------------------------------
// Don't pause the game or deactive the window without first stopping rumble otherwise 
// the controller will continue to rumble
//--------------------------------------------------------------------------------------
HRESULT DXUTStopRumbleOnAllControllers()
{
    static LPXINPUTSETSTATE s_pXInputSetState = nullptr;
    if( !s_pXInputSetState )
    {
        HINSTANCE hInst = LoadLibraryEx( XINPUT_DLL, nullptr, 0x00000800 /* LOAD_LIBRARY_SEARCH_SYSTEM32 */ );
        if( hInst )
            s_pXInputSetState = reinterpret_cast<LPXINPUTSETSTATE>( GetProcAddress( hInst, "XInputSetState" ) );
    }
    if( !s_pXInputSetState )
        return E_FAIL;

    XINPUT_VIBRATION vibration;
    vibration.wLeftMotorSpeed = 0;
    vibration.wRightMotorSpeed = 0;
    for( int iUserIndex = 0; iUserIndex < DXUT_MAX_CONTROLLERS; iUserIndex++ )
        s_pXInputSetState( iUserIndex, &vibration );

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Helper functions to create SRGB formats from typeless formats and vice versa
//--------------------------------------------------------------------------------------
DXGI_FORMAT MAKE_SRGB( _In_ DXGI_FORMAT format )
{
    if( !DXUTIsInGammaCorrectMode() )
        return format;

    switch( format )
    {
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
            return DXGI_FORMAT_BC1_UNORM_SRGB;

        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
            return DXGI_FORMAT_BC2_UNORM_SRGB;

        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
            return DXGI_FORMAT_BC3_UNORM_SRGB;

        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;

        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
            return DXGI_FORMAT_BC7_UNORM_SRGB;
    };

    return format;
}

//--------------------------------------------------------------------------------------
DXGI_FORMAT MAKE_TYPELESS( _In_ DXGI_FORMAT format )
{
    switch( format )
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return DXGI_FORMAT_R32G32B32A32_TYPELESS;

    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return DXGI_FORMAT_R32G32B32_TYPELESS;

    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
        return DXGI_FORMAT_R16G16B16A16_TYPELESS;

    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
        return DXGI_FORMAT_R32G32_TYPELESS;

    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
        return DXGI_FORMAT_R10G10B10A2_TYPELESS;

    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
        return DXGI_FORMAT_R8G8B8A8_TYPELESS;

    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
        return DXGI_FORMAT_R16G16_TYPELESS;

    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
        return DXGI_FORMAT_R32_TYPELESS;

    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
        return DXGI_FORMAT_R8G8_TYPELESS;

    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
        return DXGI_FORMAT_R16_TYPELESS;

    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
        return DXGI_FORMAT_R8_TYPELESS;

    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
        return DXGI_FORMAT_BC1_TYPELESS;

    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
        return DXGI_FORMAT_BC2_TYPELESS;

    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
        return DXGI_FORMAT_BC3_TYPELESS;

    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return DXGI_FORMAT_BC4_TYPELESS;

    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
        return DXGI_FORMAT_BC5_TYPELESS;

    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8A8_TYPELESS;

    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8X8_TYPELESS;

    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
        return DXGI_FORMAT_BC6H_TYPELESS;

    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return DXGI_FORMAT_BC7_TYPELESS;

    default:
        return format;
    }
}

//-------------------------------------------------------------------------------------- 
HRESULT DXUTSnapD3D11Screenshot( _In_z_ LPCWSTR szFileName, _In_ bool usedds )
{
    IDXGISwapChain *pSwap = DXUTGetDXGISwapChain();

    if (!pSwap)
        return E_FAIL;
    
    ID3D11Texture2D* pBackBuffer;
    HRESULT hr = pSwap->GetBuffer( 0, __uuidof( *pBackBuffer ), ( LPVOID* )&pBackBuffer );
    if (hr != S_OK)
        return hr;

    auto dc  = DXUTGetD3D11DeviceContext();
    if (!dc) {
        SAFE_RELEASE(pBackBuffer);
        return E_FAIL;
    }

    if ( usedds )
    {
        hr = DirectX::SaveDDSTextureToFile( dc, pBackBuffer, szFileName );
    }
    else
    {
        hr = DirectX::SaveWICTextureToFile( dc, pBackBuffer, GUID_ContainerFormatBmp, szFileName );
    }

    SAFE_RELEASE(pBackBuffer);

    return hr;

}
