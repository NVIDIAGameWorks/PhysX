//--------------------------------------------------------------------------------------
// File: DXUTDevice11.cpp
//
// Enumerates D3D adapters, devices, modes, etc.
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
#include "DXUT.h"

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
extern void DXUTGetCallbackD3D11DeviceAcceptable( LPDXUTCALLBACKISD3D11DEVICEACCEPTABLE* ppCallbackIsDeviceAcceptable, void** ppUserContext );

static int __cdecl SortModesCallback( const void* arg1, const void* arg2 );

CD3D11Enumeration*  g_pDXUTD3D11Enumeration = nullptr;

HRESULT WINAPI DXUTCreateD3D11Enumeration()
{
    if( !g_pDXUTD3D11Enumeration )
    {
        g_pDXUTD3D11Enumeration = new (std::nothrow) CD3D11Enumeration();
        if( !g_pDXUTD3D11Enumeration )
            return E_OUTOFMEMORY;
    }
    return S_OK;
}

void WINAPI DXUTDestroyD3D11Enumeration()
{
    SAFE_DELETE( g_pDXUTD3D11Enumeration );
}

class DXUTMemoryHelperD3D11Enum
{
public:
DXUTMemoryHelperD3D11Enum() { DXUTCreateD3D11Enumeration(); }
~DXUTMemoryHelperD3D11Enum() { DXUTDestroyD3D11Enumeration(); }
};


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
CD3D11Enumeration* WINAPI DXUTGetD3D11Enumeration( bool bForceEnumerate, bool bEnumerateAllAdapterFormats, D3D_FEATURE_LEVEL forceFL )
{
    // Using an static class with accessor function to allow control of the construction order
    static DXUTMemoryHelperD3D11Enum d3d11enumMemory;
    if( g_pDXUTD3D11Enumeration && ( !g_pDXUTD3D11Enumeration->HasEnumerated() || bForceEnumerate ) )
    {
        g_pDXUTD3D11Enumeration->SetEnumerateAllAdapterFormats( bEnumerateAllAdapterFormats );
        LPDXUTCALLBACKISD3D11DEVICEACCEPTABLE pCallbackIsDeviceAcceptable;
        void* pUserContext;
        DXUTGetCallbackD3D11DeviceAcceptable( &pCallbackIsDeviceAcceptable, &pUserContext );
        g_pDXUTD3D11Enumeration->SetForceFeatureLevel(forceFL);

        g_pDXUTD3D11Enumeration->Enumerate( pCallbackIsDeviceAcceptable, pUserContext );
    }

    return g_pDXUTD3D11Enumeration;
}


//--------------------------------------------------------------------------------------
CD3D11Enumeration::CD3D11Enumeration() :
    m_bHasEnumerated(false),
    m_IsD3D11DeviceAcceptableFunc(nullptr),
    m_pIsD3D11DeviceAcceptableFuncUserContext(nullptr),
    m_bEnumerateAllAdapterFormats(false),
    m_forceFL(D3D_FEATURE_LEVEL(0)),
    m_warpFL(D3D_FEATURE_LEVEL_10_1),
    m_refFL(D3D_FEATURE_LEVEL_11_0)
{
    ResetPossibleDepthStencilFormats();
}


//--------------------------------------------------------------------------------------
CD3D11Enumeration::~CD3D11Enumeration()
{
    ClearAdapterInfoList();
}


//--------------------------------------------------------------------------------------
// Enumerate for each adapter all of the supported display modes, 
// device types, adapter formats, back buffer formats, window/full screen support, 
// depth stencil formats, multisampling types/qualities, and presentations intervals.
//
// For each combination of device type (HAL/REF), adapter format, back buffer format, and
// IsWindowed it will call the app's ConfirmDevice callback.  This allows the app
// to reject or allow that combination based on its caps/etc.  It also allows the 
// app to change the BehaviorFlags.  The BehaviorFlags defaults non-pure HWVP 
// if supported otherwise it will default to SWVP, however the app can change this 
// through the ConfirmDevice callback.
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CD3D11Enumeration::Enumerate( LPDXUTCALLBACKISD3D11DEVICEACCEPTABLE IsD3D11DeviceAcceptableFunc,
                                      void* pIsD3D11DeviceAcceptableFuncUserContext )
{
    CDXUTPerfEventGenerator eventGenerator( DXUT_PERFEVENTCOLOR, L"DXUT D3D11 Enumeration" );
    HRESULT hr;
    auto pFactory = DXUTGetDXGIFactory();
    if( !pFactory )
        return E_FAIL;

    m_bHasEnumerated = true;
    m_IsD3D11DeviceAcceptableFunc = IsD3D11DeviceAcceptableFunc;
    m_pIsD3D11DeviceAcceptableFuncUserContext = pIsD3D11DeviceAcceptableFuncUserContext;

    ClearAdapterInfoList();

    for( int index = 0; ; ++index )
    {
        IDXGIAdapter* pAdapter = nullptr;
        hr = pFactory->EnumAdapters( index, &pAdapter );
        if( FAILED( hr ) ) // DXGIERR_NOT_FOUND is expected when the end of the list is hit
            break;

        IDXGIAdapter2* pAdapter2 = nullptr;
        if ( SUCCEEDED( pAdapter->QueryInterface( __uuidof(IDXGIAdapter2), ( LPVOID* )&pAdapter2 ) ) )
        {
            // Succeeds on DirectX 11.1 Runtime systems
            DXGI_ADAPTER_DESC2 desc;
            hr = pAdapter2->GetDesc2( &desc );
            pAdapter2->Release();

            if ( SUCCEEDED(hr) && ( desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE ) )
            {
                // Skip "always there" Microsoft Basics Display Driver
                pAdapter->Release();
                continue;
            }
        }

        auto pAdapterInfo = new (std::nothrow) CD3D11EnumAdapterInfo;
        if( !pAdapterInfo )
        {
            SAFE_RELEASE( pAdapter );
            return E_OUTOFMEMORY;
        }
        pAdapterInfo->AdapterOrdinal = index;
        pAdapter->GetDesc( &pAdapterInfo->AdapterDesc );
        pAdapterInfo->m_pAdapter = pAdapter;

        // Enumerate the device driver types on the adapter.
        hr = EnumerateDevices( pAdapterInfo );
        if( FAILED( hr ) )
        {
            delete pAdapterInfo;
            continue;
        }

        hr = EnumerateOutputs( pAdapterInfo );
        if( FAILED( hr ) || pAdapterInfo->outputInfoList.empty() )
        {
            delete pAdapterInfo;
            continue;
        }

        // Get info for each devicecombo on this device
        if( FAILED( hr = EnumerateDeviceCombos( pAdapterInfo ) ) )
        {
            delete pAdapterInfo;
            continue;
        }

        m_AdapterInfoList.push_back( pAdapterInfo );
    }

    //  If we did not get an adapter then we should still enumerate WARP and Ref.
    if (m_AdapterInfoList.size() == 0)
    {
        auto pAdapterInfo = new (std::nothrow) CD3D11EnumAdapterInfo;
        if( !pAdapterInfo )
        {
            return E_OUTOFMEMORY;
        }
        pAdapterInfo->bAdapterUnavailable = true;

        hr = EnumerateDevices( pAdapterInfo );

        // Get info for each devicecombo on this device
        if( FAILED( hr = EnumerateDeviceCombosNoAdapter(  pAdapterInfo ) ) )
        {
            delete pAdapterInfo;
        }

        if (SUCCEEDED(hr)) m_AdapterInfoList.push_back( pAdapterInfo );
    }

    //
    // Check for 2 or more adapters with the same name. Append the name
    // with some instance number if that's the case to help distinguish
    // them.
    //
    bool bUniqueDesc = true;
    for( size_t i = 0; i < m_AdapterInfoList.size(); i++ )
    {
        auto pAdapterInfo1 = m_AdapterInfoList[ i ];

        for( size_t j = i + 1; j < m_AdapterInfoList.size(); j++ )
        {
            auto pAdapterInfo2 = m_AdapterInfoList[ j ];
            if( wcsncmp( pAdapterInfo1->AdapterDesc.Description,
                pAdapterInfo2->AdapterDesc.Description, DXGI_MAX_DEVICE_IDENTIFIER_STRING ) == 0 )
            {
                bUniqueDesc = false;
                break;
            }
        }

        if( !bUniqueDesc )
            break;
    }

    for( auto it = m_AdapterInfoList.begin(); it != m_AdapterInfoList.end(); ++it )
    {
        wcscpy_s((*it)->szUniqueDescription, DXGI_MAX_DEVICE_IDENTIFIER_STRING, (*it)->AdapterDesc.Description);
        if( !bUniqueDesc )
        {
            WCHAR sz[32];
            swprintf_s( sz, 32, L" (#%u)", (*it)->AdapterOrdinal );
            wcscat_s( (*it)->szUniqueDescription, DXGI_MAX_DEVICE_IDENTIFIER_STRING, sz );
        }
    }

    D3D_FEATURE_LEVEL fLvl[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };

    // Check WARP max feature level
    {
        ID3D11Device* pDevice = nullptr;
        hr = DXUT_Dynamic_D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_WARP, 0, 0, fLvl, _countof(fLvl),
                                             D3D11_SDK_VERSION, &pDevice, &m_warpFL, nullptr );
        if ( hr == E_INVALIDARG )
        {
            // DirectX 11.0 runtime will not recognize FL 11.1, so try without it
            hr = DXUT_Dynamic_D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_WARP, 0, 0, &fLvl[1], _countof(fLvl) - 1,
                                                 D3D11_SDK_VERSION, &pDevice, &m_warpFL, nullptr );
        }

        if ( SUCCEEDED(hr) )
        {
            pDevice->Release();
        }
        else
            m_warpFL = D3D_FEATURE_LEVEL_10_1;
    }

    // Check REF max feature level
    {
        ID3D11Device* pDevice = nullptr;
        hr = DXUT_Dynamic_D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_REFERENCE, 0, 0, fLvl, _countof(fLvl),
                                             D3D11_SDK_VERSION, &pDevice, &m_refFL, nullptr );
        if ( hr == E_INVALIDARG )
        {
            // DirectX 11.0 runtime will not recognize FL 11.1, so try without it
            hr = DXUT_Dynamic_D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_REFERENCE, 0, 0, &fLvl[1], _countof(fLvl) - 1,
                                                 D3D11_SDK_VERSION, &pDevice, &m_refFL, nullptr );
        }

        if ( SUCCEEDED(hr) )
        {
            pDevice->Release();
        }
        else
            m_refFL = D3D_FEATURE_LEVEL_11_0;
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
HRESULT CD3D11Enumeration::EnumerateOutputs( _In_ CD3D11EnumAdapterInfo* pAdapterInfo )
{
    HRESULT hr;
    IDXGIOutput* pOutput;

    for( int iOutput = 0; ; ++iOutput )
    {
        pOutput = nullptr;
        hr = pAdapterInfo->m_pAdapter->EnumOutputs( iOutput, &pOutput );
        if( DXGI_ERROR_NOT_FOUND == hr )
        {
            return S_OK;
        }
        else if( FAILED( hr ) )
        {
            return hr;	//Something bad happened.
        }
        else //Success!
        {
            auto pOutputInfo = new (std::nothrow) CD3D11EnumOutputInfo;
            if( !pOutputInfo )
            {
                SAFE_RELEASE( pOutput );
                return E_OUTOFMEMORY;
            }
            pOutputInfo->Output = iOutput;
            pOutputInfo->m_pOutput = pOutput;
            pOutput->GetDesc( &pOutputInfo->Desc );

            EnumerateDisplayModes( pOutputInfo );
            if( pOutputInfo->displayModeList.empty() )
            {
                // If this output has no valid display mode, do not save it.
                delete pOutputInfo;
                continue;
            }

            pAdapterInfo->outputInfoList.push_back( pOutputInfo );
        }
    }
}


//--------------------------------------------------------------------------------------
HRESULT CD3D11Enumeration::EnumerateDisplayModes( _In_ CD3D11EnumOutputInfo* pOutputInfo )
{
    HRESULT hr = S_OK;
    DXGI_FORMAT allowedAdapterFormatArray[] =
    {
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,     //This is DXUT's preferred mode

        DXGI_FORMAT_R8G8B8A8_UNORM,			
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_R10G10B10A2_UNORM
    };
    int allowedAdapterFormatArrayCount = sizeof( allowedAdapterFormatArray ) / sizeof( allowedAdapterFormatArray[0] );

    // Swap perferred modes for apps running in linear space
    DXGI_FORMAT RemoteMode = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    if( !DXUTIsInGammaCorrectMode() )
    {
        allowedAdapterFormatArray[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        allowedAdapterFormatArray[1] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        RemoteMode = DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    // The fast path only enumerates R8G8B8A8_UNORM_SRGB modes
    if( !m_bEnumerateAllAdapterFormats )
        allowedAdapterFormatArrayCount = 1;

    for( int f = 0; f < allowedAdapterFormatArrayCount; ++f )
    {
        // Fast-path: Try to grab at least 512 modes.
        //			  This is to avoid calling GetDisplayModeList more times than necessary.
        //			  GetDisplayModeList is an expensive call.
        UINT NumModes = 512;
        auto pDesc = new (std::nothrow) DXGI_MODE_DESC[ NumModes ];
        assert( pDesc );
        if( !pDesc )
            return E_OUTOFMEMORY;

        hr = pOutputInfo->m_pOutput->GetDisplayModeList( allowedAdapterFormatArray[f],
                                                         DXGI_ENUM_MODES_SCALING,
                                                         &NumModes,
                                                         pDesc );
        if( DXGI_ERROR_NOT_FOUND == hr )
        {
            SAFE_DELETE_ARRAY( pDesc );
            NumModes = 0;
            break;
        }
        else if( MAKE_DXGI_HRESULT( 34 ) == hr && RemoteMode == allowedAdapterFormatArray[f] )
        {
            // DXGI cannot enumerate display modes over a remote session.  Therefore, create a fake display
            // mode for the current screen resolution for the remote session.
            if( 0 != GetSystemMetrics( 0x1000 ) ) // SM_REMOTESESSION
            {
                DEVMODE DevMode;
                DevMode.dmSize = sizeof( DEVMODE );
                if( EnumDisplaySettings( nullptr, ENUM_CURRENT_SETTINGS, &DevMode ) )
                {
                    NumModes = 1;
                    pDesc[0].Width = DevMode.dmPelsWidth;
                    pDesc[0].Height = DevMode.dmPelsHeight;
                    pDesc[0].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    pDesc[0].RefreshRate.Numerator = 0;
                    pDesc[0].RefreshRate.Denominator = 0;
                    pDesc[0].ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
                    pDesc[0].Scaling = DXGI_MODE_SCALING_CENTERED;
                    hr = S_OK;
                }
            }
        }
        else if( DXGI_ERROR_MORE_DATA == hr )
        {
            // Slow path.  There were more than 512 modes.
            SAFE_DELETE_ARRAY( pDesc );
            hr = pOutputInfo->m_pOutput->GetDisplayModeList( allowedAdapterFormatArray[f],
                                                             DXGI_ENUM_MODES_SCALING,
                                                             &NumModes,
                                                             nullptr );
            if( FAILED( hr ) )
            {
                NumModes = 0;
                break;
            }

            pDesc = new (std::nothrow) DXGI_MODE_DESC[ NumModes ];
            assert( pDesc );
            if( !pDesc )
                return E_OUTOFMEMORY;

            hr = pOutputInfo->m_pOutput->GetDisplayModeList( allowedAdapterFormatArray[f],
                                                             DXGI_ENUM_MODES_SCALING,
                                                             &NumModes,
                                                             pDesc );
            if( FAILED( hr ) )
            {
                SAFE_DELETE_ARRAY( pDesc );
                NumModes = 0;
                break;
            }

        }

        if( 0 == NumModes && 0 == f )
        {
            // No R8G8B8A8_UNORM_SRGB modes!
            // Abort the fast-path if we're on it
            allowedAdapterFormatArrayCount = sizeof( allowedAdapterFormatArray ) / sizeof
                ( allowedAdapterFormatArray[0] );
            SAFE_DELETE_ARRAY( pDesc );
            continue;
        }

        if( SUCCEEDED( hr ) )
        {
            for( UINT m = 0; m < NumModes; m++ )
            {
#pragma warning ( suppress : 6385 )
                pOutputInfo->displayModeList.push_back( pDesc[m] );
            }
        }

        SAFE_DELETE_ARRAY( pDesc );
    }

    return hr;
}


//--------------------------------------------------------------------------------------
HRESULT CD3D11Enumeration::EnumerateDevices( _In_ CD3D11EnumAdapterInfo* pAdapterInfo )
{
    HRESULT hr;
    auto deviceSettings = DXUTGetDeviceSettings();
    const D3D_DRIVER_TYPE devTypeArray[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE
    };
    const UINT devTypeArrayCount = sizeof( devTypeArray ) / sizeof( devTypeArray[0] );

    // Enumerate each Direct3D device type
    for( UINT iDeviceType = 0; iDeviceType < devTypeArrayCount; iDeviceType++ )
    {
        auto pDeviceInfo = new (std::nothrow) CD3D11EnumDeviceInfo;
        if( !pDeviceInfo )
            return E_OUTOFMEMORY;

        // Fill struct w/ AdapterOrdinal and D3D_DRIVER_TYPE
        pDeviceInfo->AdapterOrdinal = pAdapterInfo->AdapterOrdinal;
        pDeviceInfo->DeviceType = devTypeArray[iDeviceType];

        D3D_FEATURE_LEVEL FeatureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1
        };
        UINT NumFeatureLevels = ARRAYSIZE( FeatureLevels );

        // Call D3D11CreateDevice to ensure that this is a D3D11 device.
        ID3D11Device* pd3dDevice = nullptr;
        ID3D11DeviceContext* pd3dDeviceContext = nullptr;
        hr = DXUT_Dynamic_D3D11CreateDevice( (devTypeArray[iDeviceType] == D3D_DRIVER_TYPE_HARDWARE) ? pAdapterInfo->m_pAdapter : nullptr,
                                             (devTypeArray[iDeviceType] == D3D_DRIVER_TYPE_HARDWARE) ? D3D_DRIVER_TYPE_UNKNOWN : devTypeArray[iDeviceType],
                                             ( HMODULE )0,
                                             0,
                                             FeatureLevels,
                                             NumFeatureLevels,
                                             D3D11_SDK_VERSION,
                                             &pd3dDevice,
                                             &pDeviceInfo->MaxLevel,
                                             &pd3dDeviceContext );

        if ( hr == E_INVALIDARG )
        {
            // DirectX 11.0 runtime will not recognize FL 11.1, so try without it
            hr = DXUT_Dynamic_D3D11CreateDevice( (devTypeArray[iDeviceType] == D3D_DRIVER_TYPE_HARDWARE) ? pAdapterInfo->m_pAdapter : nullptr,
                                                 (devTypeArray[iDeviceType] == D3D_DRIVER_TYPE_HARDWARE) ? D3D_DRIVER_TYPE_UNKNOWN : devTypeArray[iDeviceType],
                                                 ( HMODULE )0,
                                                 0,
                                                 &FeatureLevels[1],
                                                 NumFeatureLevels - 1,
                                                 D3D11_SDK_VERSION,
                                                 &pd3dDevice,
                                                 &pDeviceInfo->MaxLevel,
                                                 &pd3dDeviceContext );
        }

        if ( FAILED(hr) )
        {
            delete pDeviceInfo;
            continue;
        }
        else if ( pDeviceInfo->MaxLevel < deviceSettings.MinimumFeatureLevel )
        {
            delete pDeviceInfo;
            SAFE_RELEASE( pd3dDevice );
            SAFE_RELEASE( pd3dDeviceContext );        
            continue;
        }
        
        if (m_forceFL == 0 || m_forceFL == pDeviceInfo->MaxLevel)
        { 
            pDeviceInfo->SelectedLevel = pDeviceInfo->MaxLevel;
        }
        else if (m_forceFL > pDeviceInfo->MaxLevel)
        {
            delete pDeviceInfo;
            SAFE_RELEASE( pd3dDevice );
            SAFE_RELEASE( pd3dDeviceContext );        
            continue;
        }
        else
        {
            // A device was created with a higher feature level that the user-specified feature level.
            SAFE_RELEASE( pd3dDevice );
            SAFE_RELEASE( pd3dDeviceContext );
            D3D_FEATURE_LEVEL rtFL;
            hr = DXUT_Dynamic_D3D11CreateDevice( (devTypeArray[iDeviceType] == D3D_DRIVER_TYPE_HARDWARE) ? pAdapterInfo->m_pAdapter : nullptr,
                                                 (devTypeArray[iDeviceType] == D3D_DRIVER_TYPE_HARDWARE) ? D3D_DRIVER_TYPE_UNKNOWN : devTypeArray[iDeviceType],
                                                 ( HMODULE )0,
                                                 0,
                                                 &m_forceFL,
                                                 1,
                                                 D3D11_SDK_VERSION,
                                                 &pd3dDevice,
                                                 &rtFL,
                                                 &pd3dDeviceContext );

            if( SUCCEEDED( hr ) && rtFL == m_forceFL )
            {
                pDeviceInfo->SelectedLevel = m_forceFL;
            }
            else
            {
                delete pDeviceInfo;
                if ( SUCCEEDED(hr) )
                {
                    SAFE_RELEASE( pd3dDevice );
                    SAFE_RELEASE( pd3dDeviceContext );
                }
                continue;
            }
        }

        D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS ho;
        hr = pd3dDevice->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &ho, sizeof(ho));
        if ( FAILED(hr) )
            memset( &ho, 0, sizeof(ho) );

        pDeviceInfo->ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x = ho.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x; 
        SAFE_RELEASE( pd3dDeviceContext );             
        SAFE_RELEASE( pd3dDevice );
        pAdapterInfo->deviceInfoList.push_back( pDeviceInfo );
    }

    return S_OK;
}


HRESULT CD3D11Enumeration::EnumerateDeviceCombosNoAdapter( _In_ CD3D11EnumAdapterInfo* pAdapterInfo )
{
    // Iterate through each combination of device driver type, output,
    // adapter format, and backbuffer format to build the adapter's device combo list.
    //

    for( auto dit = pAdapterInfo->deviceInfoList.cbegin(); dit != pAdapterInfo->deviceInfoList.cend(); ++dit )
    {
        DXGI_FORMAT BufferFormatArray[] =
        {
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,   //This is DXUT's preferred mode

            DXGI_FORMAT_R8G8B8A8_UNORM,		
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            DXGI_FORMAT_R10G10B10A2_UNORM
        };

        // Swap perferred modes for apps running in linear space
        if( !DXUTIsInGammaCorrectMode() )
        {
            BufferFormatArray[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            BufferFormatArray[1] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        }

        for( UINT iBufferFormat = 0; iBufferFormat < _countof( BufferFormatArray ); iBufferFormat++ )
        {
            DXGI_FORMAT BufferFormat = BufferFormatArray[iBufferFormat];

            // determine if there are any modes for this particular format

            // If an application callback function has been provided, make sure this device
            // is acceptable to the app.
            if( m_IsD3D11DeviceAcceptableFunc )
            {
                if( !m_IsD3D11DeviceAcceptableFunc( pAdapterInfo, 
                                                    0,
                                                    *dit, 
                                                    BufferFormat,
                                                    TRUE,
                                                    m_pIsD3D11DeviceAcceptableFuncUserContext ) )
                    continue;
            }

            // At this point, we have an adapter/device/backbufferformat/iswindowed
            // DeviceCombo that is supported by the system. We still 
            // need to find one or more suitable depth/stencil buffer format,
            // multisample type, and present interval.
            CD3D11EnumDeviceSettingsCombo* pDeviceCombo = new (std::nothrow) CD3D11EnumDeviceSettingsCombo;
            if( !pDeviceCombo )
                return E_OUTOFMEMORY;

            pDeviceCombo->AdapterOrdinal = (*dit)->AdapterOrdinal;
            pDeviceCombo->DeviceType = (*dit)->DeviceType;
            pDeviceCombo->BackBufferFormat = BufferFormat;
            pDeviceCombo->Windowed = TRUE;
            pDeviceCombo->Output = 0;
            pDeviceCombo->pAdapterInfo = pAdapterInfo;
            pDeviceCombo->pDeviceInfo = (*dit);
            pDeviceCombo->pOutputInfo = nullptr;

            BuildMultiSampleQualityList( BufferFormat, pDeviceCombo );

            pAdapterInfo->deviceSettingsComboList.push_back( pDeviceCombo );
        }
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CD3D11Enumeration::EnumerateDeviceCombos( CD3D11EnumAdapterInfo* pAdapterInfo )
{
    // Iterate through each combination of device driver type, output,
    // adapter format, and backbuffer format to build the adapter's device combo list.
    //
    for( size_t output = 0; output < pAdapterInfo->outputInfoList.size(); ++output )
    {
        auto pOutputInfo = pAdapterInfo->outputInfoList[ output ];

        for( size_t device = 0; device < pAdapterInfo->deviceInfoList.size(); ++device )
        {
            auto pDeviceInfo = pAdapterInfo->deviceInfoList[ device ];

            DXGI_FORMAT backBufferFormatArray[] =
            {
                DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,   //This is DXUT's preferred mode

                DXGI_FORMAT_R8G8B8A8_UNORM,		
                DXGI_FORMAT_R16G16B16A16_FLOAT,
                DXGI_FORMAT_R10G10B10A2_UNORM
            };

            // Swap perferred modes for apps running in linear space
            if( !DXUTIsInGammaCorrectMode() )
            {
                backBufferFormatArray[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
                backBufferFormatArray[1] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            }

            for( UINT iBackBufferFormat = 0; iBackBufferFormat < _countof( backBufferFormatArray ); iBackBufferFormat++ )
            {
                DXGI_FORMAT backBufferFormat = backBufferFormatArray[iBackBufferFormat];

                for( int nWindowed = 0; nWindowed < 2; nWindowed++ )
                {
                    if( !nWindowed && pOutputInfo->displayModeList.size() == 0 )
                        continue;

                    // determine if there are any modes for this particular format
                    size_t iModes = 0;
                    for( size_t i = 0; i < pOutputInfo->displayModeList.size(); i++ )
                    {
                        if( backBufferFormat == pOutputInfo->displayModeList[ i ].Format )
                            ++iModes;
                    }
                    if( !iModes )
                        continue;

                    // If an application callback function has been provided, make sure this device
                    // is acceptable to the app.
                    if( m_IsD3D11DeviceAcceptableFunc )
                    {
                        if( !m_IsD3D11DeviceAcceptableFunc( pAdapterInfo, static_cast<UINT>( output ),
                                                            pDeviceInfo, backBufferFormat,
                                                            FALSE != nWindowed,
                                                            m_pIsD3D11DeviceAcceptableFuncUserContext ) )
                            continue;
                    }

                    // At this point, we have an adapter/device/backbufferformat/iswindowed
                    // DeviceCombo that is supported by the system. We still 
                    // need to find one or more suitable depth/stencil buffer format,
                    // multisample type, and present interval.
                    auto pDeviceCombo = new (std::nothrow) CD3D11EnumDeviceSettingsCombo;
                    if( !pDeviceCombo )
                        return E_OUTOFMEMORY;

                    pDeviceCombo->AdapterOrdinal = pDeviceInfo->AdapterOrdinal;
                    pDeviceCombo->DeviceType = pDeviceInfo->DeviceType;
                    pDeviceCombo->BackBufferFormat = backBufferFormat;
                    pDeviceCombo->Windowed = ( nWindowed != 0 );
                    pDeviceCombo->Output = pOutputInfo->Output;
                    pDeviceCombo->pAdapterInfo = pAdapterInfo;
                    pDeviceCombo->pDeviceInfo = pDeviceInfo;
                    pDeviceCombo->pOutputInfo = pOutputInfo;

                    BuildMultiSampleQualityList( backBufferFormat, pDeviceCombo );

                    pAdapterInfo->deviceSettingsComboList.push_back( pDeviceCombo );
                }
            }
        }
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Release all the allocated CD3D11EnumAdapterInfo objects and empty the list
//--------------------------------------------------------------------------------------
void CD3D11Enumeration::ClearAdapterInfoList()
{
    for( auto it = m_AdapterInfoList.begin(); it != m_AdapterInfoList.end(); ++it )
    {
        auto pAdapterInfo = *it;
        delete pAdapterInfo;
    }
    m_AdapterInfoList.clear();
}


//--------------------------------------------------------------------------------------
void CD3D11Enumeration::ResetPossibleDepthStencilFormats()
{
    m_DepthStencilPossibleList.clear();
    m_DepthStencilPossibleList.push_back( DXGI_FORMAT_D32_FLOAT_S8X24_UINT );
    m_DepthStencilPossibleList.push_back( DXGI_FORMAT_D32_FLOAT );
    m_DepthStencilPossibleList.push_back( DXGI_FORMAT_D24_UNORM_S8_UINT );
    m_DepthStencilPossibleList.push_back( DXGI_FORMAT_D16_UNORM );
}


//--------------------------------------------------------------------------------------
void CD3D11Enumeration::SetEnumerateAllAdapterFormats( _In_ bool bEnumerateAllAdapterFormats )
{
    m_bEnumerateAllAdapterFormats = bEnumerateAllAdapterFormats;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CD3D11Enumeration::BuildMultiSampleQualityList( DXGI_FORMAT fmt, CD3D11EnumDeviceSettingsCombo* pDeviceCombo )
{
    ID3D11Device* pd3dDevice = nullptr;
    ID3D11DeviceContext* pd3dDeviceContext = nullptr;
    IDXGIAdapter* pAdapter = nullptr;
    
    D3D_FEATURE_LEVEL *FeatureLevels = &(pDeviceCombo->pDeviceInfo->SelectedLevel);
    D3D_FEATURE_LEVEL returnedFeatureLevel;

    UINT NumFeatureLevels = 1;

    HRESULT hr = DXUT_Dynamic_D3D11CreateDevice( pAdapter, 
                                                pDeviceCombo->DeviceType,
                                                ( HMODULE )0,
                                                0,
                                                FeatureLevels,
                                                NumFeatureLevels,
                                                D3D11_SDK_VERSION,
                                                &pd3dDevice,
                                                &returnedFeatureLevel,
                                                &pd3dDeviceContext )  ;

    if( FAILED( hr))  return;

    if (returnedFeatureLevel != pDeviceCombo->pDeviceInfo->SelectedLevel) return;

    for( int i = 1; i <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; ++i )
    {
        UINT Quality;
        if( SUCCEEDED( pd3dDevice->CheckMultisampleQualityLevels( fmt, i, &Quality ) ) && Quality > 0 )
        {
            //From D3D10 docs: When multisampling a texture, the number of quality levels available for an adapter is dependent on the texture 
            //format used and the number of samples requested. The maximum sample count is defined by 
            //D3D10_MAX_MULTISAMPLE_SAMPLE_COUNT in d3d10.h. If the returned value of pNumQualityLevels is 0, 
            //the format and sample count combination is not supported for the installed adapter.

            if (Quality != 0) {
                pDeviceCombo->multiSampleCountList.push_back( i );
                pDeviceCombo->multiSampleQualityList.push_back( Quality );
            }
        }
    }

    SAFE_RELEASE( pAdapter );
    SAFE_RELEASE( pd3dDevice );
    SAFE_RELEASE (pd3dDeviceContext);
}


//--------------------------------------------------------------------------------------
// Call GetAdapterInfoList() after Enumerate() to get a STL vector of 
//       CD3D11EnumAdapterInfo* 
//--------------------------------------------------------------------------------------
std::vector <CD3D11EnumAdapterInfo*>* CD3D11Enumeration::GetAdapterInfoList()
{
    return &m_AdapterInfoList;
}


//--------------------------------------------------------------------------------------
CD3D11EnumAdapterInfo* CD3D11Enumeration::GetAdapterInfo( _In_ UINT AdapterOrdinal ) const
{
    for( auto it = m_AdapterInfoList.cbegin(); it != m_AdapterInfoList.cend(); ++it )
    {
        if( (*it)->AdapterOrdinal == AdapterOrdinal )
            return *it;
    }

    return nullptr;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
CD3D11EnumDeviceInfo* CD3D11Enumeration::GetDeviceInfo( UINT AdapterOrdinal, D3D_DRIVER_TYPE DeviceType ) const
{
    auto pAdapterInfo = GetAdapterInfo( AdapterOrdinal );
    if( pAdapterInfo )
    {
        for( auto it = pAdapterInfo->deviceInfoList.cbegin(); it != pAdapterInfo->deviceInfoList.cend(); ++it )
        {
            if( (*it)->DeviceType == DeviceType )
                return *it;
        }
    }

    return nullptr;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
CD3D11EnumOutputInfo* CD3D11Enumeration::GetOutputInfo( UINT AdapterOrdinal, UINT Output ) const
{
    auto pAdapterInfo = GetAdapterInfo( AdapterOrdinal );
    if( pAdapterInfo && pAdapterInfo->outputInfoList.size() > size_t( Output ) )
    {
        return pAdapterInfo->outputInfoList[ Output ];
    }

    return nullptr;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
CD3D11EnumDeviceSettingsCombo* CD3D11Enumeration::GetDeviceSettingsCombo( UINT AdapterOrdinal,
                                                                          DXGI_FORMAT BackBufferFormat, BOOL Windowed ) const
{
    auto pAdapterInfo = GetAdapterInfo( AdapterOrdinal );
    if( pAdapterInfo )
    {
        for( size_t iDeviceCombo = 0; iDeviceCombo < pAdapterInfo->deviceSettingsComboList.size(); iDeviceCombo++ )
        {
            auto pDeviceSettingsCombo = pAdapterInfo->deviceSettingsComboList[ iDeviceCombo ];
            if( pDeviceSettingsCombo->BackBufferFormat == BackBufferFormat &&
                pDeviceSettingsCombo->Windowed == Windowed )
                return pDeviceSettingsCombo;
        }
    }

    return nullptr;
}


//--------------------------------------------------------------------------------------
CD3D11EnumOutputInfo::~CD3D11EnumOutputInfo()
{
    SAFE_RELEASE( m_pOutput );
    displayModeList.clear();
}


//--------------------------------------------------------------------------------------
CD3D11EnumDeviceInfo::~CD3D11EnumDeviceInfo()
{
}


//--------------------------------------------------------------------------------------
CD3D11EnumAdapterInfo::~CD3D11EnumAdapterInfo()
{
    for( size_t j = 0; j < outputInfoList.size(); ++j )
    {
        auto pOutputInfo = outputInfoList[ j ];
        delete pOutputInfo;
    }
    outputInfoList.clear();

    for( size_t j = 0; j < deviceInfoList.size(); ++j )
    {
        auto pDeviceInfo = deviceInfoList[ j ];
        delete pDeviceInfo;
    }
    deviceInfoList.clear();

    for( size_t j = 0; j < deviceSettingsComboList.size(); ++j )
    {
        auto pDeviceCombo = deviceSettingsComboList[ j ];
        delete pDeviceCombo;
    }
    deviceSettingsComboList.clear();

    SAFE_RELEASE( m_pAdapter );
}

//--------------------------------------------------------------------------------------
// Returns the number of color channel bits in the specified DXGI_FORMAT
//--------------------------------------------------------------------------------------
UINT WINAPI DXUTGetDXGIColorChannelBits( DXGI_FORMAT fmt )
{
    switch( fmt )
    {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
            return 32;

        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
            return 16;

        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
            return 10;

        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return 8;

        case DXGI_FORMAT_B5G6R5_UNORM:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
            return 5;

        case DXGI_FORMAT_B4G4R4A4_UNORM:
            return 4;

        default:
            return 0;
    }
}

//--------------------------------------------------------------------------------------
// Returns a ranking number that describes how closely this device 
// combo matches the optimal combo based on the match options and the optimal device settings
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
float DXUTRankD3D11DeviceCombo( CD3D11EnumDeviceSettingsCombo* pDeviceSettingsCombo,
                                DXUTD3D11DeviceSettings* pOptimalDeviceSettings,
                                int &bestModeIndex,
                                int &bestMSAAIndex
                                )
{
    float fCurRanking = 0.0f;

    // Arbitrary weights.  Gives preference to the ordinal, device type, and windowed
    const float fAdapterOrdinalWeight   = 1000.0f;
    const float fAdapterOutputWeight    = 500.0f;
    const float fDeviceTypeWeight       = 100.0f;
    const float fWARPOverRefWeight       = 80.0f;

    const float fWindowWeight           = 10.0f;
    const float fResolutionWeight       = 1.0f;
    const float fBackBufferFormatWeight = 1.0f;
    const float fMultiSampleWeight      = 1.0f;

    //---------------------
    // Adapter ordinal
    //---------------------
    if( pDeviceSettingsCombo->AdapterOrdinal == pOptimalDeviceSettings->AdapterOrdinal )
        fCurRanking += fAdapterOrdinalWeight;

    //---------------------
    // Adapter ordinal
    //---------------------
    if( pDeviceSettingsCombo->Output == pOptimalDeviceSettings->Output )
        fCurRanking += fAdapterOutputWeight;

    //---------------------
    // Device type
    //---------------------
    if( pDeviceSettingsCombo->DeviceType == pOptimalDeviceSettings->DriverType )
        fCurRanking += fDeviceTypeWeight;
    else if (pDeviceSettingsCombo->DeviceType == D3D_DRIVER_TYPE_WARP && pOptimalDeviceSettings->DriverType == D3D_DRIVER_TYPE_HARDWARE) {
        fCurRanking += fWARPOverRefWeight;
    }

    // Slightly prefer HAL 
    if( pDeviceSettingsCombo->DeviceType == D3D_DRIVER_TYPE_HARDWARE )
        fCurRanking += 0.1f;

    //---------------------
    // Windowed
    //---------------------
    if( pDeviceSettingsCombo->Windowed == pOptimalDeviceSettings->sd.Windowed )
        fCurRanking += fWindowWeight;

    //---------------------
    // Resolution/Refresh Rate
    //---------------------
    bestModeIndex=0;

    if( pDeviceSettingsCombo->pOutputInfo )
    {
        bool bResolutionFound = false;
        float best = FLT_MAX;

        if ( !pDeviceSettingsCombo->Windowed
             && !pOptimalDeviceSettings->sd.Windowed
             && ( pOptimalDeviceSettings->sd.BufferDesc.RefreshRate.Numerator > 0 || pOptimalDeviceSettings->sd.BufferDesc.RefreshRate.Denominator > 0 ) )
        {
            // Match both Resolution & Refresh Rate
            for( size_t idm = 0; idm < pDeviceSettingsCombo->pOutputInfo->displayModeList.size() && !bResolutionFound; idm++ )
            {
                auto displayMode = pDeviceSettingsCombo->pOutputInfo->displayModeList[ idm ];

                float refreshDiff = fabs( ( float( displayMode.RefreshRate.Numerator ) / float( displayMode.RefreshRate.Denominator ) ) -
                                          ( float( pOptimalDeviceSettings->sd.BufferDesc.RefreshRate.Numerator ) / float( pOptimalDeviceSettings->sd.BufferDesc.RefreshRate.Denominator ) ) );

                if( displayMode.Width == pOptimalDeviceSettings->sd.BufferDesc.Width
                    && displayMode.Height == pOptimalDeviceSettings->sd.BufferDesc.Height
                    && ( refreshDiff < 0.1f ) )
                {
                    bResolutionFound = true;
                    bestModeIndex = static_cast<int>( idm );
                    break;
                }

                float current = refreshDiff
                                + fabs( float( displayMode.Width ) - float ( pOptimalDeviceSettings->sd.BufferDesc.Width ) ) 
                                + fabs( float( displayMode.Height ) - float ( pOptimalDeviceSettings->sd.BufferDesc.Height ) );

                if( current < best )
                {
                    best = current;
                    bestModeIndex = static_cast<int>( idm );
                }
            }
        }
        else
        {
            // Match just Resolution
            for( size_t idm = 0; idm < pDeviceSettingsCombo->pOutputInfo->displayModeList.size() && !bResolutionFound; idm++ )
            {
                auto displayMode = pDeviceSettingsCombo->pOutputInfo->displayModeList[ idm ];

                if( displayMode.Width == pOptimalDeviceSettings->sd.BufferDesc.Width
                    && displayMode.Height == pOptimalDeviceSettings->sd.BufferDesc.Height )
                {
                    bResolutionFound = true;
                    bestModeIndex = static_cast<int>( idm );
                    break;
                }

                float current = fabs( float( displayMode.Width ) - float ( pOptimalDeviceSettings->sd.BufferDesc.Width ) ) 
                                + fabs( float( displayMode.Height ) - float ( pOptimalDeviceSettings->sd.BufferDesc.Height ) );

                if( current < best )
                {
                    best = current;
                    bestModeIndex = static_cast<int>( idm );
                }
            }
        }

        if( bResolutionFound )
            fCurRanking += fResolutionWeight;
    }

    //---------------------
    // Back buffer format
    //---------------------
    if( pDeviceSettingsCombo->BackBufferFormat == pOptimalDeviceSettings->sd.BufferDesc.Format )
    {
        fCurRanking += fBackBufferFormatWeight;
    }
    else
    {
        int nBitDepthDelta = abs( ( long )DXUTGetDXGIColorChannelBits( pDeviceSettingsCombo->BackBufferFormat ) -
                                  ( long )DXUTGetDXGIColorChannelBits(
                                  pOptimalDeviceSettings->sd.BufferDesc.Format ) );
        float fScale = std::max<float>( 0.9f - ( float )nBitDepthDelta * 0.2f, 0.0f );
        fCurRanking += fScale * fBackBufferFormatWeight;
    }

    //---------------------
    // Back buffer count
    //---------------------
    // No caps for the back buffer count

    //---------------------
    // Multisample
    //---------------------
    bool bMultiSampleFound = false;
    bestMSAAIndex = 0;
    for( size_t i = 0; i < pDeviceSettingsCombo->multiSampleCountList.size(); i++ )
    {
        UINT Count = pDeviceSettingsCombo->multiSampleCountList[ i ];

        if( Count == pOptimalDeviceSettings->sd.SampleDesc.Count  )
        {
            bestMSAAIndex = static_cast<int>( i );
            bMultiSampleFound = true;
            break;
        }
    }
    if( bMultiSampleFound )
        fCurRanking += fMultiSampleWeight;

    //---------------------
    // Swap effect
    //---------------------
    // No caps for swap effects

    //---------------------
    // Depth stencil 
    //---------------------
    // No caps for swap effects

    //---------------------
    // Present flags
    //---------------------
    // No caps for the present flags

    //---------------------
    // Present interval
    //---------------------
    // No caps for the present flags

    return fCurRanking;
}


//--------------------------------------------------------------------------------------
// Returns the DXGI_MODE_DESC struct for a given adapter and output 
//--------------------------------------------------------------------------------------
#pragma warning ( suppress : 6101 )
_Use_decl_annotations_
HRESULT WINAPI DXUTGetD3D11AdapterDisplayMode( UINT AdapterOrdinal, UINT nOutput, DXGI_MODE_DESC* pModeDesc )
{
    if( !pModeDesc )
        return E_INVALIDARG;

    auto pD3DEnum = DXUTGetD3D11Enumeration();
    if ( !pD3DEnum )
        return E_POINTER;

    auto pOutputInfo = pD3DEnum->GetOutputInfo( AdapterOrdinal, nOutput );
    if( pOutputInfo )
    {
        pModeDesc->Width = 800;
        pModeDesc->Height = 600;
        pModeDesc->RefreshRate.Numerator = 0;
        pModeDesc->RefreshRate.Denominator = 0;
        pModeDesc->Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        pModeDesc->Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        pModeDesc->ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

        DXGI_OUTPUT_DESC Desc;
        if ( FAILED(pOutputInfo->m_pOutput->GetDesc(&Desc)))
            memset( &Desc, 0, sizeof(Desc) );
        pModeDesc->Width = Desc.DesktopCoordinates.right - Desc.DesktopCoordinates.left;
        pModeDesc->Height = Desc.DesktopCoordinates.bottom - Desc.DesktopCoordinates.top;

        // This should not be required with DXGI 1.1 support for BGRA...
        if( pModeDesc->Format == DXGI_FORMAT_B8G8R8A8_UNORM )
            pModeDesc->Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    return S_OK;
}
