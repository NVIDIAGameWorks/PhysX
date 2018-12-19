//--------------------------------------------------------------------------------------
// File: DXUTDevice11.h
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
#pragma once

void DXUTApplyDefaultDeviceSettings(DXUTDeviceSettings *modifySettings);

//--------------------------------------------------------------------------------------
// Functions to get bit depth from formats
//--------------------------------------------------------------------------------------
HRESULT WINAPI DXUTGetD3D11AdapterDisplayMode( _In_ UINT AdapterOrdinal, _In_ UINT Output, _Out_ DXGI_MODE_DESC* pModeDesc ); 




//--------------------------------------------------------------------------------------
// Optional memory create/destory functions.  If not call, these will be called automatically
//--------------------------------------------------------------------------------------
HRESULT WINAPI DXUTCreateD3D11Enumeration();
void WINAPI DXUTDestroyD3D11Enumeration();




//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
class CD3D11EnumAdapterInfo;
class CD3D11EnumDeviceInfo;
class CD3D11EnumOutputInfo;
struct CD3D11EnumDeviceSettingsCombo;



//--------------------------------------------------------------------------------------
// Enumerates available Direct3D11 adapters, devices, modes, etc.
//--------------------------------------------------------------------------------------
class CD3D11Enumeration
{
public:
    // These should be called before Enumerate(). 
    //
    // Use these calls and the IsDeviceAcceptable to control the contents of 
    // the enumeration object, which affects the device selection and the device settings dialog.
    void SetResolutionMinMax( _In_ UINT nMinWidth, _In_ UINT nMinHeight, _In_ UINT nMaxWidth, _In_ UINT nMaxHeight );  
    void SetRefreshMinMax( _In_ UINT nMin, _In_ UINT nMax );
    void SetForceFeatureLevel( _In_ D3D_FEATURE_LEVEL forceFL) { m_forceFL = forceFL; }
    void SetMultisampleQualityMax( _In_ UINT nMax );
    void ResetPossibleDepthStencilFormats();
    void SetEnumerateAllAdapterFormats( _In_ bool bEnumerateAllAdapterFormats );
    
    // Call Enumerate() to enumerate available D3D11 adapters, devices, modes, etc.
    bool HasEnumerated() { return m_bHasEnumerated; }
    HRESULT Enumerate( _In_ LPDXUTCALLBACKISD3D11DEVICEACCEPTABLE IsD3D11DeviceAcceptableFunc,
                       _In_opt_ void* pIsD3D11DeviceAcceptableFuncUserContext );

    // These should be called after Enumerate() is called
    std::vector<CD3D11EnumAdapterInfo*>*     GetAdapterInfoList();
    CD3D11EnumAdapterInfo*                   GetAdapterInfo( _In_ UINT AdapterOrdinal ) const;
    CD3D11EnumDeviceInfo*                    GetDeviceInfo( _In_ UINT AdapterOrdinal, _In_ D3D_DRIVER_TYPE DeviceType ) const;
    CD3D11EnumOutputInfo*                    GetOutputInfo( _In_ UINT AdapterOrdinal, _In_ UINT Output ) const;
    CD3D11EnumDeviceSettingsCombo*           GetDeviceSettingsCombo( _In_ DXUTD3D11DeviceSettings* pDeviceSettings ) const { return GetDeviceSettingsCombo( pDeviceSettings->AdapterOrdinal, pDeviceSettings->sd.BufferDesc.Format, pDeviceSettings->sd.Windowed ); }
    CD3D11EnumDeviceSettingsCombo*           GetDeviceSettingsCombo( _In_ UINT AdapterOrdinal, _In_ DXGI_FORMAT BackBufferFormat, _In_ BOOL Windowed ) const;
    D3D_FEATURE_LEVEL                        GetWARPFeaturevel() const { return m_warpFL; }
    D3D_FEATURE_LEVEL                        GetREFFeaturevel() const { return m_refFL; }

    ~CD3D11Enumeration();

private:
    friend HRESULT WINAPI DXUTCreateD3D11Enumeration();

    // Use DXUTGetD3D11Enumeration() to access global instance
    CD3D11Enumeration();

    bool m_bHasEnumerated;
    LPDXUTCALLBACKISD3D11DEVICEACCEPTABLE m_IsD3D11DeviceAcceptableFunc;
    void* m_pIsD3D11DeviceAcceptableFuncUserContext;

    std::vector<DXGI_FORMAT> m_DepthStencilPossibleList;

    bool m_bEnumerateAllAdapterFormats;
    D3D_FEATURE_LEVEL m_forceFL;
    D3D_FEATURE_LEVEL m_warpFL;
    D3D_FEATURE_LEVEL m_refFL;

    std::vector<CD3D11EnumAdapterInfo*> m_AdapterInfoList;

    HRESULT EnumerateOutputs( _In_ CD3D11EnumAdapterInfo *pAdapterInfo );
    HRESULT EnumerateDevices( _In_ CD3D11EnumAdapterInfo *pAdapterInfo );
    HRESULT EnumerateDeviceCombos(  _In_ CD3D11EnumAdapterInfo* pAdapterInfo );
    HRESULT EnumerateDeviceCombosNoAdapter( _In_ CD3D11EnumAdapterInfo* pAdapterInfo );
    
    HRESULT EnumerateDisplayModes( _In_ CD3D11EnumOutputInfo *pOutputInfo );
    void BuildMultiSampleQualityList( _In_ DXGI_FORMAT fmt, _In_ CD3D11EnumDeviceSettingsCombo* pDeviceCombo );
    void ClearAdapterInfoList();
};

CD3D11Enumeration* WINAPI DXUTGetD3D11Enumeration(_In_ bool bForceEnumerate = false, _In_ bool EnumerateAllAdapterFormats = true, _In_ D3D_FEATURE_LEVEL forceFL = ((D3D_FEATURE_LEVEL )0)  );


#define DXGI_MAX_DEVICE_IDENTIFIER_STRING 128

//--------------------------------------------------------------------------------------
// A class describing an adapter which contains a unique adapter ordinal 
// that is installed on the system
//--------------------------------------------------------------------------------------
class CD3D11EnumAdapterInfo
{
    const CD3D11EnumAdapterInfo &operator = ( const CD3D11EnumAdapterInfo &rhs );

public:
    CD3D11EnumAdapterInfo() :
        AdapterOrdinal( 0 ),
        m_pAdapter( nullptr ),
        bAdapterUnavailable(false)
    {
       *szUniqueDescription = 0;
       memset( &AdapterDesc, 0, sizeof(AdapterDesc) );
    }
    ~CD3D11EnumAdapterInfo();

    UINT AdapterOrdinal;
    DXGI_ADAPTER_DESC AdapterDesc;
    WCHAR szUniqueDescription[DXGI_MAX_DEVICE_IDENTIFIER_STRING];
    IDXGIAdapter *m_pAdapter;
    bool bAdapterUnavailable;

    std::vector<CD3D11EnumOutputInfo*> outputInfoList; // Array of CD3D11EnumOutputInfo*
    std::vector<CD3D11EnumDeviceInfo*> deviceInfoList; // Array of CD3D11EnumDeviceInfo*
    // List of CD3D11EnumDeviceSettingsCombo* with a unique set 
    // of BackBufferFormat, and Windowed
    std::vector<CD3D11EnumDeviceSettingsCombo*> deviceSettingsComboList;
};


class CD3D11EnumOutputInfo
{
    const CD3D11EnumOutputInfo &operator = ( const CD3D11EnumOutputInfo &rhs );

public:
    CD3D11EnumOutputInfo() :
        AdapterOrdinal( 0 ),
        Output( 0 ),
        m_pOutput( nullptr ) {}
    ~CD3D11EnumOutputInfo();

    UINT AdapterOrdinal;
    UINT Output;
    IDXGIOutput* m_pOutput;
    DXGI_OUTPUT_DESC Desc;

    std::vector <DXGI_MODE_DESC> displayModeList; // Array of supported D3DDISPLAYMODEs
};


//--------------------------------------------------------------------------------------
// A class describing a Direct3D11 device that contains a unique supported driver type
//--------------------------------------------------------------------------------------
class CD3D11EnumDeviceInfo
{
    const CD3D11EnumDeviceInfo& operator =( const CD3D11EnumDeviceInfo& rhs );

public:
    ~CD3D11EnumDeviceInfo();

    UINT AdapterOrdinal;
    D3D_DRIVER_TYPE DeviceType;
    D3D_FEATURE_LEVEL SelectedLevel;
    D3D_FEATURE_LEVEL MaxLevel;
    BOOL ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x;
};


//--------------------------------------------------------------------------------------
// A struct describing device settings that contains a unique combination of 
// adapter format, back buffer format, and windowed that is compatible with a 
// particular Direct3D device and the app.
//--------------------------------------------------------------------------------------
struct CD3D11EnumDeviceSettingsCombo
{
    UINT AdapterOrdinal;
    D3D_DRIVER_TYPE DeviceType;
    DXGI_FORMAT BackBufferFormat;
    BOOL Windowed;
    UINT Output;

    std::vector <UINT> multiSampleCountList; // List of valid sampling counts (multisampling)
    std::vector <UINT> multiSampleQualityList; // List of number of quality levels for each multisample count

    CD3D11EnumAdapterInfo* pAdapterInfo;
    CD3D11EnumDeviceInfo* pDeviceInfo;
    CD3D11EnumOutputInfo* pOutputInfo;
};

float   DXUTRankD3D11DeviceCombo( _In_ CD3D11EnumDeviceSettingsCombo* pDeviceSettingsCombo, 
                                  _In_ DXUTD3D11DeviceSettings* pOptimalDeviceSettings, 
                                  _Out_ int &bestModeIndex,
                                  _Out_ int &bestMSAAIndex
                                 );
