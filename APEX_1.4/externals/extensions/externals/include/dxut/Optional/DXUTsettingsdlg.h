//--------------------------------------------------------------------------------------
// File: DXUTSettingsDlg.h
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
// Header Includes
//--------------------------------------------------------------------------------------
#include "DXUTgui.h"

//--------------------------------------------------------------------------------------
// Control IDs
//--------------------------------------------------------------------------------------
#define DXUTSETTINGSDLG_STATIC                          -1
#define DXUTSETTINGSDLG_OK                              1
#define DXUTSETTINGSDLG_CANCEL                          2
#define DXUTSETTINGSDLG_ADAPTER                         3
#define DXUTSETTINGSDLG_DEVICE_TYPE                     4
#define DXUTSETTINGSDLG_WINDOWED                        5
#define DXUTSETTINGSDLG_FULLSCREEN                      6
#define DXUTSETTINGSDLG_RESOLUTION_SHOW_ALL             26
#define DXUTSETTINGSDLG_D3D11_ADAPTER_OUTPUT            28
#define DXUTSETTINGSDLG_D3D11_ADAPTER_OUTPUT_LABEL      29
#define DXUTSETTINGSDLG_D3D11_RESOLUTION                30
#define DXUTSETTINGSDLG_D3D11_RESOLUTION_LABEL          31
#define DXUTSETTINGSDLG_D3D11_REFRESH_RATE              32
#define DXUTSETTINGSDLG_D3D11_REFRESH_RATE_LABEL        33
#define DXUTSETTINGSDLG_D3D11_BACK_BUFFER_FORMAT        34
#define DXUTSETTINGSDLG_D3D11_BACK_BUFFER_FORMAT_LABEL  35
#define DXUTSETTINGSDLG_D3D11_MULTISAMPLE_COUNT         36
#define DXUTSETTINGSDLG_D3D11_MULTISAMPLE_COUNT_LABEL   37
#define DXUTSETTINGSDLG_D3D11_MULTISAMPLE_QUALITY       38
#define DXUTSETTINGSDLG_D3D11_MULTISAMPLE_QUALITY_LABEL 39
#define DXUTSETTINGSDLG_D3D11_PRESENT_INTERVAL          40
#define DXUTSETTINGSDLG_D3D11_PRESENT_INTERVAL_LABEL    41
#define DXUTSETTINGSDLG_D3D11_DEBUG_DEVICE              42
#define DXUTSETTINGSDLG_D3D11_FEATURE_LEVEL             43
#define DXUTSETTINGSDLG_D3D11_FEATURE_LEVEL_LABEL       44

#define DXUTSETTINGSDLG_MODE_CHANGE_ACCEPT              58
#define DXUTSETTINGSDLG_MODE_CHANGE_REVERT              59
#define DXUTSETTINGSDLG_STATIC_MODE_CHANGE_TIMEOUT      60
#define DXUTSETTINGSDLG_WINDOWED_GROUP                  0x0100

#define TOTAL_FEATURE_LEVELS                            7

//--------------------------------------------------------------------------------------
// Dialog for selection of device settings 
// Use DXUTGetD3DSettingsDialog() to access global instance
// To control the contents of the dialog, use the CD3D11Enumeration class.
//--------------------------------------------------------------------------------------
class CD3DSettingsDlg
{
public:
    CD3DSettingsDlg();
    ~CD3DSettingsDlg();

    void Init( _In_ CDXUTDialogResourceManager* pManager );
    void Init( _In_ CDXUTDialogResourceManager* pManager, _In_z_ LPCWSTR szControlTextureFileName );
    void Init( _In_ CDXUTDialogResourceManager* pManager, _In_z_ LPCWSTR pszControlTextureResourcename,
               _In_ HMODULE hModule );

    HRESULT Refresh();
    void OnRender( _In_ float fElapsedTime );

    HRESULT OnD3D11CreateDevice( _In_ ID3D11Device* pd3dDevice );
    HRESULT OnD3D11ResizedSwapChain( _In_ ID3D11Device* pd3dDevice,
                                     _In_ const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc );
    void OnD3D11DestroyDevice();

    CDXUTDialog* GetDialogControl() { return &m_Dialog; }
    bool IsActive() const { return m_bActive; }
    void SetActive( _In_ bool bActive )
    {
        m_bActive = bActive;
        if( bActive ) Refresh();
    }

    LRESULT MsgProc( _In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam );

protected:
    friend CD3DSettingsDlg* WINAPI DXUTGetD3DSettingsDialog();

    void CreateControls();
    void SetSelectedD3D11RefreshRate( _In_ DXGI_RATIONAL RefreshRate );
    HRESULT UpdateD3D11Resolutions();
    HRESULT UpdateD3D11RefreshRates();

    void OnEvent( _In_ UINT nEvent, _In_ int nControlID, _In_ CDXUTControl* pControl );

    static void WINAPI StaticOnEvent( _In_ UINT nEvent, _In_ int nControlID, _In_ CDXUTControl* pControl, _In_opt_ void* pUserData );
    static void WINAPI StaticOnModeChangeTimer( _In_ UINT nIDEvent, _In_opt_ void* pUserContext );

    CD3D11EnumAdapterInfo* GetCurrentD3D11AdapterInfo() const;
    CD3D11EnumDeviceInfo* GetCurrentD3D11DeviceInfo() const;
    CD3D11EnumOutputInfo* GetCurrentD3D11OutputInfo() const;
    CD3D11EnumDeviceSettingsCombo* GetCurrentD3D11DeviceSettingsCombo() const;

    void AddAdapter( _In_z_ const WCHAR* strDescription, _In_ UINT iAdapter );
    UINT GetSelectedAdapter() const;

    void SetWindowed( _In_ bool bWindowed );
    bool IsWindowed() const;

    // D3D11
    void                AddD3D11DeviceType( _In_ D3D_DRIVER_TYPE devType );
    D3D_DRIVER_TYPE     GetSelectedD3D11DeviceType() const;

    void                AddD3D11AdapterOutput( _In_z_ const WCHAR* strName, _In_ UINT nOutput );
    UINT                GetSelectedD3D11AdapterOutput() const;

    void                AddD3D11Resolution( _In_ DWORD dwWidth, _In_ DWORD dwHeight );
    void                GetSelectedD3D11Resolution( _Out_ DWORD* pdwWidth, _Out_ DWORD* pdwHeight ) const;

    void                AddD3D11FeatureLevel( _In_ D3D_FEATURE_LEVEL fl );
    D3D_FEATURE_LEVEL   GetSelectedFeatureLevel() const;

    void                AddD3D11RefreshRate( _In_ DXGI_RATIONAL RefreshRate );
    DXGI_RATIONAL       GetSelectedD3D11RefreshRate() const;

    void                AddD3D11BackBufferFormat( _In_ DXGI_FORMAT format );
    DXGI_FORMAT         GetSelectedD3D11BackBufferFormat() const;

    void                AddD3D11MultisampleCount( _In_ UINT count );
    UINT                GetSelectedD3D11MultisampleCount() const;

    void                AddD3D11MultisampleQuality( _In_ UINT Quality );
    UINT                GetSelectedD3D11MultisampleQuality() const;

    DWORD               GetSelectedD3D11PresentInterval() const;
    bool                GetSelectedDebugDeviceValue() const;
    
    HRESULT             OnD3D11ResolutionChanged ();
    HRESULT             OnFeatureLevelChanged();
    HRESULT             OnAdapterChanged();
    HRESULT             OnDeviceTypeChanged();
    HRESULT             OnWindowedFullScreenChanged();
    HRESULT             OnAdapterOutputChanged();
    HRESULT             OnRefreshRateChanged();
    HRESULT             OnBackBufferFormatChanged();
    HRESULT             OnMultisampleTypeChanged();
    HRESULT             OnMultisampleQualityChanged();
    HRESULT             OnPresentIntervalChanged();
    HRESULT             OnDebugDeviceChanged();

    void                UpdateModeChangeTimeoutText( _In_ int nSecRemaining );

    CDXUTDialog* m_pActiveDialog;
    CDXUTDialog m_Dialog;
    CDXUTDialog m_RevertModeDialog;
    int m_nRevertModeTimeout;
    UINT m_nIDEvent;
    bool m_bActive;

    D3D_FEATURE_LEVEL m_Levels[TOTAL_FEATURE_LEVELS];

};


CD3DSettingsDlg* WINAPI DXUTGetD3DSettingsDialog();
