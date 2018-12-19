//--------------------------------------------------------------------------------------
// File: DXUTSettingsDlg.cpp
//
// Dialog for selection of device settings 
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
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"

//--------------------------------------------------------------------------------------
// Internal functions forward declarations
//--------------------------------------------------------------------------------------
const WCHAR*        DXUTPresentIntervalToString( _In_ UINT pi );
const WCHAR*        DXUTDeviceTypeToString( _In_ D3D_DRIVER_TYPE devType );
const WCHAR*        DXUTVertexProcessingTypeToString( _In_ DWORD vpt );


HRESULT DXUTSnapDeviceSettingsToEnumDevice( DXUTDeviceSettings* pDeviceSettings, bool forceEnum, D3D_FEATURE_LEVEL forceFL = D3D_FEATURE_LEVEL(0)  );

//--------------------------------------------------------------------------------------
// Global state
//--------------------------------------------------------------------------------------
DXUTDeviceSettings  g_DeviceSettings;

CD3DSettingsDlg* WINAPI DXUTGetD3DSettingsDialog()
{
    // Using an accessor function gives control of the construction order
    static CD3DSettingsDlg dlg;
    return &dlg;
}


//--------------------------------------------------------------------------------------
CD3DSettingsDlg::CD3DSettingsDlg() :
    m_bActive( false ),
    m_pActiveDialog( nullptr )
{
    m_Levels[0] = D3D_FEATURE_LEVEL_9_1;
    m_Levels[1] = D3D_FEATURE_LEVEL_9_2;
    m_Levels[2] = D3D_FEATURE_LEVEL_9_3;
    m_Levels[3] = D3D_FEATURE_LEVEL_10_0;
    m_Levels[4] = D3D_FEATURE_LEVEL_10_1;
    m_Levels[5] = D3D_FEATURE_LEVEL_11_0;
    m_Levels[6] = D3D_FEATURE_LEVEL_11_1;
}


//--------------------------------------------------------------------------------------
CD3DSettingsDlg::~CD3DSettingsDlg()
{
    // Release the memory used to hold the D3D11 refresh data in the combo box
    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_REFRESH_RATE );
    if( pComboBox )
        for( UINT i = 0; i < pComboBox->GetNumItems(); ++i )
        {
            auto pRate = reinterpret_cast<DXGI_RATIONAL*>( pComboBox->GetItemData( i ) );
            delete pRate;
        }
}


//--------------------------------------------------------------------------------------
void CD3DSettingsDlg::Init( _In_ CDXUTDialogResourceManager* pManager )
{
    assert( pManager );
    m_Dialog.Init( pManager, false );  // Don't register this dialog.
    m_RevertModeDialog.Init( pManager, false ); // Don't register this dialog.
    m_pActiveDialog = &m_Dialog;
    CreateControls();
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CD3DSettingsDlg::Init( CDXUTDialogResourceManager* pManager, LPCWSTR szControlTextureFileName )
{
    assert( pManager );
    m_Dialog.Init( pManager, false, szControlTextureFileName );  // Don't register this dialog.
    m_RevertModeDialog.Init( pManager, false, szControlTextureFileName );   // Don't register this dialog.
    m_pActiveDialog = &m_Dialog;
    CreateControls();
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CD3DSettingsDlg::Init( CDXUTDialogResourceManager* pManager, LPCWSTR pszControlTextureResourcename,
                            HMODULE hModule )
{
    assert( pManager );
    m_Dialog.Init( pManager, false, pszControlTextureResourcename, hModule );  // Don't register this dialog.
    m_RevertModeDialog.Init( pManager, false, pszControlTextureResourcename, hModule ); // Don't register this dialog
    m_pActiveDialog = &m_Dialog;
    CreateControls();
}


//--------------------------------------------------------------------------------------
void CD3DSettingsDlg::CreateControls()
{
    // Set up main settings dialog
    m_Dialog.EnableKeyboardInput( true );
    m_Dialog.SetFont( 0, L"Arial", 15, FW_NORMAL );
    m_Dialog.SetFont( 1, L"Arial", 28, FW_BOLD );

    // Right-justify static controls
    auto pElement = m_Dialog.GetDefaultElement( DXUT_CONTROL_STATIC, 0 );
    if( pElement )
    {
        pElement->dwTextFormat = DT_VCENTER | DT_RIGHT;

        // Title
        CDXUTStatic* pStatic = nullptr;
        m_Dialog.AddStatic( DXUTSETTINGSDLG_STATIC, L"Direct3D Settings", 10, 5, 400, 50, false, &pStatic );
        pElement = pStatic->GetElement( 0 );
        pElement->iFont = 1;
        pElement->dwTextFormat = DT_TOP | DT_LEFT;
    }

    //DXUTSETTINGSDLG_D3D11_FEATURE_LEVEL
    m_Dialog.AddStatic( DXUTSETTINGSDLG_D3D11_FEATURE_LEVEL_LABEL, L"Feature Level", 10, 60, 180, 23 );
    m_Dialog.AddComboBox( DXUTSETTINGSDLG_D3D11_FEATURE_LEVEL, 200, 60, 400, 23 );
    m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_FEATURE_LEVEL )->SetDropHeight( 106 );

    // DXUTSETTINGSDLG_ADAPTER
    m_Dialog.AddStatic( DXUTSETTINGSDLG_STATIC, L"Display Adapter", 10, 85, 180, 23 );
    m_Dialog.AddComboBox( DXUTSETTINGSDLG_ADAPTER, 200, 85, 400, 23 );

    // DXUTSETTINGSDLG_DEVICE_TYPE
    m_Dialog.AddStatic( DXUTSETTINGSDLG_STATIC, L"Render Device", 10, 110, 180, 23 );
    m_Dialog.AddComboBox( DXUTSETTINGSDLG_DEVICE_TYPE, 200, 110, 400, 23 );

    // DXUTSETTINGSDLG_WINDOWED, DXUTSETTINGSDLG_FULLSCREEN
    m_Dialog.AddRadioButton( DXUTSETTINGSDLG_WINDOWED, DXUTSETTINGSDLG_WINDOWED_GROUP, L"Windowed", 
                          360, 157, 100, 16 );
    m_Dialog.AddRadioButton( DXUTSETTINGSDLG_FULLSCREEN, DXUTSETTINGSDLG_WINDOWED_GROUP, L"Full Screen",
                          220, 157, 100, 16 );

    // DXUTSETTINGSDLG_RES_SHOW_ALL
    m_Dialog.AddCheckBox( DXUTSETTINGSDLG_RESOLUTION_SHOW_ALL, L"Show All Aspect Ratios", 420, 200, 200, 23, false );

    // DXUTSETTINGSDLG_D3D11_ADAPTER_OUTPUT
    m_Dialog.AddStatic( DXUTSETTINGSDLG_D3D11_ADAPTER_OUTPUT_LABEL, L"Adapter Output", 10, 175, 180, 23 );
    m_Dialog.AddComboBox( DXUTSETTINGSDLG_D3D11_ADAPTER_OUTPUT, 200, 175, 400, 23 );

    // DXUTSETTINGSDLG_D3D11_RESOLUTION
    m_Dialog.AddStatic( DXUTSETTINGSDLG_D3D11_RESOLUTION_LABEL, L"Resolution", 10, 200, 180, 23 );
    m_Dialog.AddComboBox( DXUTSETTINGSDLG_D3D11_RESOLUTION, 200, 200, 200, 23 );
    m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_RESOLUTION )->SetDropHeight( 106 );

    // DXUTSETTINGSDLG_D3D11_REFRESH_RATE
    m_Dialog.AddStatic( DXUTSETTINGSDLG_D3D11_REFRESH_RATE_LABEL, L"Refresh Rate", 10, 225, 180, 23 );
    m_Dialog.AddComboBox( DXUTSETTINGSDLG_D3D11_REFRESH_RATE, 200, 225, 400, 23 );

    // DXUTSETTINGSDLG_D3D11_BACK_BUFFER_FORMAT
    m_Dialog.AddStatic( DXUTSETTINGSDLG_D3D11_BACK_BUFFER_FORMAT_LABEL, L"Back Buffer Format", 10, 260, 180, 23 );
    m_Dialog.AddComboBox( DXUTSETTINGSDLG_D3D11_BACK_BUFFER_FORMAT, 200, 260, 400, 23 );

    // DXUTSETTINGSDLG_D3D11_MULTISAMPLE_COUNT
    m_Dialog.AddStatic( DXUTSETTINGSDLG_D3D11_MULTISAMPLE_COUNT_LABEL, L"Multisample Count", 10, 285, 180, 23 );
    m_Dialog.AddComboBox( DXUTSETTINGSDLG_D3D11_MULTISAMPLE_COUNT, 200, 285, 400, 23 );

    // DXUTSETTINGSDLG_D3D11_MULTISAMPLE_QUALITY
    m_Dialog.AddStatic( DXUTSETTINGSDLG_D3D11_MULTISAMPLE_QUALITY_LABEL, L"Multisample Quality", 10, 310, 180, 23 );
    m_Dialog.AddComboBox( DXUTSETTINGSDLG_D3D11_MULTISAMPLE_QUALITY, 200, 310, 400, 23 );

    // DXUTSETTINGSDLG_D3D11_PRESENT_INTERVAL
    m_Dialog.AddStatic( DXUTSETTINGSDLG_D3D11_PRESENT_INTERVAL_LABEL, L"Vertical Sync", 10, 335, 180, 23 );
    m_Dialog.AddComboBox(DXUTSETTINGSDLG_D3D11_PRESENT_INTERVAL, 200, 335, 400, 23);

    auto pPresentIntervalComboBox = m_Dialog.GetComboBox(DXUTSETTINGSDLG_D3D11_PRESENT_INTERVAL);
    if (pPresentIntervalComboBox)
    {
        pPresentIntervalComboBox->AddItem(L"On", ULongToPtr(1));
        pPresentIntervalComboBox->AddItem(L"Off", ULongToPtr(0));
    }

    // DXUTSETTINGSDLG_D3D11_DEBUG_DEVICE
    m_Dialog.AddCheckBox( DXUTSETTINGSDLG_D3D11_DEBUG_DEVICE, L"Create Debug Device", 200, 365, 180, 23 );

    // DXUTSETTINGSDLG_OK, DXUTSETTINGSDLG_CANCEL
    m_Dialog.AddButton( DXUTSETTINGSDLG_OK, L"OK", 230, 440, 73, 31 );
    m_Dialog.AddButton( DXUTSETTINGSDLG_CANCEL, L"Cancel", 315, 440, 73, 31, 0, true );

    // Set up mode change dialog
    m_RevertModeDialog.EnableKeyboardInput( true );
    m_RevertModeDialog.EnableNonUserEvents( true );
    m_RevertModeDialog.SetFont( 0, L"Arial", 15, FW_NORMAL );
    m_RevertModeDialog.SetFont( 1, L"Arial", 28, FW_BOLD );

    pElement = m_RevertModeDialog.GetDefaultElement( DXUT_CONTROL_STATIC, 0 );
    if( pElement )
    {
        pElement->dwTextFormat = DT_VCENTER | DT_RIGHT;

        // Title
        CDXUTStatic* pStatic = nullptr;
        if ( SUCCEEDED(m_RevertModeDialog.AddStatic( DXUTSETTINGSDLG_STATIC, L"Do you want to keep these display settings?", 10, 5,
                                      640, 50, false, &pStatic ) ) )
            pElement = pStatic->GetElement( 0 );
        pElement->iFont = 1;
        pElement->dwTextFormat = DT_TOP | DT_LEFT;

        // Timeout static text control
        if ( SUCCEEDED(m_RevertModeDialog.AddStatic( DXUTSETTINGSDLG_STATIC_MODE_CHANGE_TIMEOUT, L"", 10, 90, 640, 30,
                                      false, &pStatic ) ) )
            pElement = pStatic->GetElement( 0 );
        pElement->iFont = 0;
        pElement->dwTextFormat = DT_TOP | DT_LEFT;
    }

    // DXUTSETTINGSDLG_MODE_CHANGE_ACCEPT, DXUTSETTINGSDLG_MODE_CHANGE_REVERT
    m_RevertModeDialog.AddButton( DXUTSETTINGSDLG_MODE_CHANGE_ACCEPT, L"Yes", 230, 50, 73, 31 );
    m_RevertModeDialog.AddButton( DXUTSETTINGSDLG_MODE_CHANGE_REVERT, L"No", 315, 50, 73, 31, 0, true );
}


//--------------------------------------------------------------------------------------
// Changes the UI defaults to the current device settings
//--------------------------------------------------------------------------------------
HRESULT CD3DSettingsDlg::Refresh()
{
    g_DeviceSettings = DXUTGetDeviceSettings();

    auto pD3DEnum = DXUTGetD3D11Enumeration();

    // Fill the UI with the current settings
    AddD3D11DeviceType( g_DeviceSettings.d3d11.DriverType );

    m_Dialog.SetControlEnabled( DXUTSETTINGSDLG_WINDOWED, true );
    m_Dialog.SetControlEnabled( DXUTSETTINGSDLG_FULLSCREEN, (g_DeviceSettings.d3d11.DriverType != D3D_DRIVER_TYPE_WARP) );

    SetWindowed( FALSE != g_DeviceSettings.d3d11.sd.Windowed );
    auto pOutputInfo = GetCurrentD3D11OutputInfo();
    AddD3D11AdapterOutput( pOutputInfo->Desc.DeviceName, g_DeviceSettings.d3d11.Output );
            
    AddD3D11Resolution( g_DeviceSettings.d3d11.sd.BufferDesc.Width,
                        g_DeviceSettings.d3d11.sd.BufferDesc.Height );
    AddD3D11RefreshRate( g_DeviceSettings.d3d11.sd.BufferDesc.RefreshRate );
    AddD3D11BackBufferFormat( g_DeviceSettings.d3d11.sd.BufferDesc.Format );
    AddD3D11MultisampleCount( g_DeviceSettings.d3d11.sd.SampleDesc.Count );
    AddD3D11MultisampleQuality( g_DeviceSettings.d3d11.sd.SampleDesc.Quality );

    auto pBestDeviceSettingsCombo = pD3DEnum->GetDeviceSettingsCombo(
                g_DeviceSettings.d3d11.AdapterOrdinal, g_DeviceSettings.d3d11.sd.BufferDesc.Format,
                ( g_DeviceSettings.d3d11.sd.Windowed != 0 ) );

    if( !pBestDeviceSettingsCombo )
        return DXUT_ERR_MSGBOX( L"GetDeviceSettingsCombo", E_INVALIDARG );

    CDXUTComboBox *pFeatureLevelBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_FEATURE_LEVEL );
    pFeatureLevelBox->RemoveAllItems();

    D3D_FEATURE_LEVEL clampFL;
    if ( g_DeviceSettings.d3d11.DriverType == D3D_DRIVER_TYPE_WARP )
        clampFL = DXUTGetD3D11Enumeration()->GetWARPFeaturevel();
    else if ( g_DeviceSettings.d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE )
        clampFL = DXUTGetD3D11Enumeration()->GetREFFeaturevel();
    else
        clampFL = pBestDeviceSettingsCombo->pDeviceInfo->MaxLevel;

    for (int fli = 0; fli < TOTAL_FEATURE_LEVELS; fli++)
    {
        if (m_Levels[fli] >= g_DeviceSettings.MinimumFeatureLevel 
            && m_Levels[fli] <= clampFL)
        {
            AddD3D11FeatureLevel( m_Levels[fli] );
        }
    } 
    pFeatureLevelBox->SetSelectedByData( ULongToPtr( g_DeviceSettings.d3d11.DeviceFeatureLevel ) );
          
    // Get the adapters list from CD3D11Enumeration object
    auto pAdapterInfoList = pD3DEnum->GetAdapterInfoList();

    if( pAdapterInfoList->empty() )
        return DXUT_ERR_MSGBOX( L"CD3DSettingsDlg::OnCreatedDevice", DXUTERR_NOCOMPATIBLEDEVICES );

    auto pAdapterCombo = m_Dialog.GetComboBox( DXUTSETTINGSDLG_ADAPTER );
    pAdapterCombo->RemoveAllItems();

    // Add adapters
    for( auto it = pAdapterInfoList->cbegin(); it != pAdapterInfoList->cend(); ++it )
    {
        AddAdapter( (*it)->szUniqueDescription, (*it)->AdapterOrdinal );
    }

    pAdapterCombo->SetSelectedByData( ULongToPtr( g_DeviceSettings.d3d11.AdapterOrdinal ) );

    // DXUTSETTINGSDLG_D3D11_RESOLUTION
    HRESULT hr = UpdateD3D11Resolutions();
    if ( FAILED(hr) )
        return hr;

    // DXUTSETTINGSDLG_D3D11_REFRESH_RATE
    hr = UpdateD3D11RefreshRates();
    if ( FAILED(hr) )
        return hr;

    // Windowed mode
    bool bWindowed = IsWindowed();

    // Backbuffer Format/Driver Type
    auto pAdapterInfo = GetCurrentD3D11AdapterInfo();
    if (pAdapterInfo)
    {
        auto pBackBufferFormatComboBox = m_Dialog.GetComboBox(DXUTSETTINGSDLG_D3D11_BACK_BUFFER_FORMAT);
        pBackBufferFormatComboBox->RemoveAllItems();

        for (size_t idc = 0; idc < pAdapterInfo->deviceSettingsComboList.size(); idc++)
        {
            auto pDeviceCombo = pAdapterInfo->deviceSettingsComboList[idc];
            if ((pDeviceCombo->Windowed == TRUE) == bWindowed)
            {
                AddD3D11BackBufferFormat(pDeviceCombo->BackBufferFormat);
            }
        }

        pBackBufferFormatComboBox->SetSelectedByData( ULongToPtr(g_DeviceSettings.d3d11.sd.BufferDesc.Format) );

        auto pDeviceTypeComboBox = m_Dialog.GetComboBox(DXUTSETTINGSDLG_DEVICE_TYPE);
        pDeviceTypeComboBox->RemoveAllItems();

        for (size_t iDeviceInfo = 0; iDeviceInfo < pAdapterInfo->deviceInfoList.size(); iDeviceInfo++)
        {
            auto pDeviceInfo = pAdapterInfo->deviceInfoList[iDeviceInfo];
            AddD3D11DeviceType(pDeviceInfo->DeviceType);
        }

        pDeviceTypeComboBox->SetSelectedByData( ULongToPtr(g_DeviceSettings.d3d11.DriverType) );
    }

    // MSAA settings
    auto pDeviceSettingsCombo = GetCurrentD3D11DeviceSettingsCombo();
    if ( pDeviceSettingsCombo )
    {
        auto pMultisampleCountCombo = m_Dialog.GetComboBox(DXUTSETTINGSDLG_D3D11_MULTISAMPLE_COUNT);
        pMultisampleCountCombo->RemoveAllItems();

        for (auto it = pDeviceSettingsCombo->multiSampleCountList.cbegin(); it != pDeviceSettingsCombo->multiSampleCountList.cend(); ++it)
            AddD3D11MultisampleCount(*it);

        pMultisampleCountCombo->SetSelectedByData( ULongToPtr(g_DeviceSettings.d3d11.sd.SampleDesc.Count) );

        UINT MaxQuality = 0;
        for (size_t iCount = 0; iCount < pDeviceSettingsCombo->multiSampleCountList.size(); iCount++)
        {
            UINT Count = pDeviceSettingsCombo->multiSampleCountList[iCount];
            if ( Count == g_DeviceSettings.d3d11.sd.SampleDesc.Count )
            {
                MaxQuality = pDeviceSettingsCombo->multiSampleQualityList[iCount];
                break;
            }
        }

        auto pMultisampleQualityCombo = m_Dialog.GetComboBox(DXUTSETTINGSDLG_D3D11_MULTISAMPLE_QUALITY);
        pMultisampleQualityCombo->RemoveAllItems();

        for (UINT iQuality = 0; iQuality < MaxQuality; iQuality++)
        {
            AddD3D11MultisampleQuality(iQuality);
        }

        pMultisampleQualityCombo->SetSelectedByData(ULongToPtr(g_DeviceSettings.d3d11.sd.SampleDesc.Quality));
    }

    // Misc settings
    auto pDebugCheckBox = m_Dialog.GetCheckBox(DXUTSETTINGSDLG_D3D11_DEBUG_DEVICE);
    if ( pDebugCheckBox )
    {
        pDebugCheckBox->SetChecked(0 != (g_DeviceSettings.d3d11.CreateFlags & D3D11_CREATE_DEVICE_DEBUG));
    }

    auto pPresentIntervalComboBox = m_Dialog.GetComboBox(DXUTSETTINGSDLG_D3D11_PRESENT_INTERVAL);
    if ( pPresentIntervalComboBox )
    {
        pPresentIntervalComboBox->SetSelectedByData(ULongToPtr(g_DeviceSettings.d3d11.SyncInterval) );
    }

    CDXUTDialog::SetRefreshTime( ( float )DXUTGetTime() );

    return S_OK;
}


//--------------------------------------------------------------------------------------
void CD3DSettingsDlg::SetSelectedD3D11RefreshRate( _In_ DXGI_RATIONAL RefreshRate )
{
    auto pRefreshRateComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_REFRESH_RATE );

    for( UINT i = 0; i < pRefreshRateComboBox->GetNumItems(); ++i )
    {
        auto pRate = reinterpret_cast<DXGI_RATIONAL*>( pRefreshRateComboBox->GetItemData( i ) );

        if( pRate && pRate->Numerator == RefreshRate.Numerator && pRate->Denominator == RefreshRate.Denominator )
        {
            pRefreshRateComboBox->SetSelectedByIndex( i );
            return;
        }
    }
}

//--------------------------------------------------------------------------------------
void CD3DSettingsDlg::OnRender( _In_ float fElapsedTime )
{
    // Render the scene
    m_pActiveDialog->OnRender( fElapsedTime );
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
LRESULT CD3DSettingsDlg::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    m_pActiveDialog->MsgProc( hWnd, uMsg, wParam, lParam );
    if( uMsg == WM_KEYDOWN && wParam == VK_F2 )
        SetActive( false );
    return 0;
}

//--------------------------------------------------------------------------------------
HRESULT CD3DSettingsDlg::OnD3D11CreateDevice( _In_ ID3D11Device* pd3dDevice )
{
    if( !pd3dDevice )
        return DXUT_ERR_MSGBOX( L"CD3DSettingsDlg::OnCreatedDevice", E_INVALIDARG );

    // Create the fonts/textures 
    m_Dialog.SetCallback( StaticOnEvent, ( void* )this );
    m_RevertModeDialog.SetCallback( StaticOnEvent, ( void* )this );

    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CD3DSettingsDlg::OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc )
{
    UNREFERENCED_PARAMETER(pd3dDevice);

    m_Dialog.SetLocation( 0, 0 );
    m_Dialog.SetSize( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    m_Dialog.SetBackgroundColors( D3DCOLOR_ARGB( 255, 98, 138, 206 ),
                                  D3DCOLOR_ARGB( 255, 54, 105, 192 ),
                                  D3DCOLOR_ARGB( 255, 54, 105, 192 ),
                                  D3DCOLOR_ARGB( 255, 10, 73, 179 ) );

    m_RevertModeDialog.SetLocation( 0, 0 );
    m_RevertModeDialog.SetSize( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    m_RevertModeDialog.SetBackgroundColors( D3DCOLOR_ARGB( 255, 98, 138, 206 ),
                                            D3DCOLOR_ARGB( 255, 54, 105, 192 ),
                                            D3DCOLOR_ARGB( 255, 54, 105, 192 ),
                                            D3DCOLOR_ARGB( 255, 10, 73, 179 ) );

    return S_OK;
}


//--------------------------------------------------------------------------------------
void CD3DSettingsDlg::OnD3D11DestroyDevice()
{


}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void WINAPI CD3DSettingsDlg::StaticOnEvent( UINT nEvent, int nControlID,
                                            CDXUTControl* pControl, void* pUserData )
{
    auto pD3DSettings = reinterpret_cast<CD3DSettingsDlg*>( pUserData );
    if( pD3DSettings )
        pD3DSettings->OnEvent( nEvent, nControlID, pControl );
}

//--------------------------------------------------------------------------------------
// Name: CD3DSettingsDlg::StaticOnModeChangeTimer()
// Desc: Timer callback registered by a call to DXUTSetTimer.  It is called each second
//       until mode change timeout limit.
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void WINAPI CD3DSettingsDlg::StaticOnModeChangeTimer( UINT nIDEvent, void* pUserContext )
{
    UNREFERENCED_PARAMETER(nIDEvent);

    auto pD3DSettings = reinterpret_cast<CD3DSettingsDlg*>( pUserContext );
    assert( pD3DSettings );
    _Analysis_assume_( pD3DSettings );
    assert( pD3DSettings->m_pActiveDialog == &pD3DSettings->m_RevertModeDialog );
    assert( pD3DSettings->m_nIDEvent == nIDEvent );

    if( 0 == --pD3DSettings->m_nRevertModeTimeout )
    {
        auto pControl = pD3DSettings->m_RevertModeDialog.GetControl( DXUTSETTINGSDLG_MODE_CHANGE_REVERT );
        assert( pControl );
        _Analysis_assume_( pControl );
        pD3DSettings->m_RevertModeDialog.SendEvent( EVENT_BUTTON_CLICKED, false, pControl );
    }
    pD3DSettings->UpdateModeChangeTimeoutText( pD3DSettings->m_nRevertModeTimeout );
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CD3DSettingsDlg::OnEvent( UINT nEvent, int nControlID, CDXUTControl* pControl )
{
    UNREFERENCED_PARAMETER(nEvent);
    UNREFERENCED_PARAMETER(pControl);

    switch( nControlID )
    {
        case DXUTSETTINGSDLG_ADAPTER:
            OnAdapterChanged(); break;
        case DXUTSETTINGSDLG_DEVICE_TYPE:
            OnDeviceTypeChanged(); break;
        case DXUTSETTINGSDLG_WINDOWED:
            OnWindowedFullScreenChanged(); break;
        case DXUTSETTINGSDLG_FULLSCREEN:
            OnWindowedFullScreenChanged(); break;
        case DXUTSETTINGSDLG_RESOLUTION_SHOW_ALL:
            OnBackBufferFormatChanged();   break;
        case DXUTSETTINGSDLG_D3D11_RESOLUTION:
            OnD3D11ResolutionChanged(); break;
        case DXUTSETTINGSDLG_D3D11_FEATURE_LEVEL:
            OnFeatureLevelChanged(); break;
        case DXUTSETTINGSDLG_D3D11_ADAPTER_OUTPUT:
            OnAdapterOutputChanged(); break;
        case DXUTSETTINGSDLG_D3D11_REFRESH_RATE:
            OnRefreshRateChanged(); break;
        case DXUTSETTINGSDLG_D3D11_BACK_BUFFER_FORMAT:
            OnBackBufferFormatChanged(); break;
        case DXUTSETTINGSDLG_D3D11_MULTISAMPLE_COUNT:
            OnMultisampleTypeChanged(); break;
        case DXUTSETTINGSDLG_D3D11_MULTISAMPLE_QUALITY:
            OnMultisampleQualityChanged(); break;
        case DXUTSETTINGSDLG_D3D11_PRESENT_INTERVAL:
            OnPresentIntervalChanged(); break;
        case DXUTSETTINGSDLG_D3D11_DEBUG_DEVICE:
            OnDebugDeviceChanged(); break;

        case DXUTSETTINGSDLG_OK:
        {
            bool bFullScreenModeChange = false;
            auto currentSettings = DXUTGetDeviceSettings();
            g_DeviceSettings.MinimumFeatureLevel = currentSettings.MinimumFeatureLevel;
            if( g_DeviceSettings.d3d11.sd.Windowed )
            {
                g_DeviceSettings.d3d11.sd.BufferDesc.RefreshRate.Denominator =
                g_DeviceSettings.d3d11.sd.BufferDesc.RefreshRate.Numerator = 0;

                RECT rcClient;
                if( DXUTIsWindowed() )
                    GetClientRect( DXUTGetHWND(), &rcClient );
                else
                    rcClient = DXUTGetWindowClientRectAtModeChange();
                DWORD dwWindowWidth = rcClient.right - rcClient.left;
                DWORD dwWindowHeight = rcClient.bottom - rcClient.top;

                g_DeviceSettings.d3d11.sd.BufferDesc.Width = dwWindowWidth;
                g_DeviceSettings.d3d11.sd.BufferDesc.Height = dwWindowHeight;
            }
            else
            {
                // Check for fullscreen mode change
                bFullScreenModeChange = g_DeviceSettings.d3d11.sd.BufferDesc.Width !=
                    currentSettings.d3d11.sd.BufferDesc.Width ||
                    g_DeviceSettings.d3d11.sd.BufferDesc.Height != currentSettings.d3d11.sd.BufferDesc.Height ||
                    g_DeviceSettings.d3d11.sd.BufferDesc.RefreshRate.Denominator !=
                    currentSettings.d3d11.sd.BufferDesc.RefreshRate.Denominator ||
                    g_DeviceSettings.d3d11.sd.BufferDesc.RefreshRate.Numerator !=
                    currentSettings.d3d11.sd.BufferDesc.RefreshRate.Numerator;
            }

            if( bFullScreenModeChange )
            {
                // set appropriate global device settings to that of the current device
                // settings.  These will get set to the user-defined settings once the
                // user accepts the mode change
                DXUTDeviceSettings tSettings = g_DeviceSettings;
                    g_DeviceSettings.d3d11.sd.BufferDesc.Width = 
                        currentSettings.d3d11.sd.BufferDesc.Width;
                    g_DeviceSettings.d3d11.sd.BufferDesc.Height = 
                        currentSettings.d3d11.sd.BufferDesc.Height;
                    g_DeviceSettings.d3d11.sd.BufferDesc.RefreshRate.Denominator =
                        currentSettings.d3d11.sd.BufferDesc.RefreshRate.Denominator;
                    g_DeviceSettings.d3d11.sd.BufferDesc.RefreshRate.Numerator =
                        currentSettings.d3d11.sd.BufferDesc.RefreshRate.Numerator;
                    g_DeviceSettings.d3d11.sd.Windowed = currentSettings.d3d11.sd.Windowed;

                // apply the user-defined settings
                DXUTCreateDeviceFromSettings( &tSettings );
                // create the mode change timeout dialog
                m_pActiveDialog = &m_RevertModeDialog;
                m_nRevertModeTimeout = 15;
                UpdateModeChangeTimeoutText( m_nRevertModeTimeout );
                // activate a timer for 1-second updates
                DXUTSetTimer( StaticOnModeChangeTimer, 1.0f, &m_nIDEvent, ( void* )this );
            }
            else
            {
                DXUTCreateDeviceFromSettings( &g_DeviceSettings );
                SetActive( false );
            }
            break;
        }

        case DXUTSETTINGSDLG_CANCEL:
        {
            SetActive( false );
            break;
        }

        case DXUTSETTINGSDLG_MODE_CHANGE_ACCEPT:
        {
            DXUTKillTimer( m_nIDEvent );
            g_DeviceSettings = DXUTGetDeviceSettings();
            m_pActiveDialog = &m_Dialog;
            SetActive( false );
            break;
        }

        case DXUTSETTINGSDLG_MODE_CHANGE_REVERT:
        {
            DXUTKillTimer( m_nIDEvent );
            m_pActiveDialog = &m_Dialog;
            m_nIDEvent = 0;
            m_nRevertModeTimeout = 0;
            DXUTCreateDeviceFromSettings( &g_DeviceSettings );
            Refresh();
            break;
        }
    }
}


//-------------------------------------------------------------------------------------
CD3D11EnumAdapterInfo* CD3DSettingsDlg::GetCurrentD3D11AdapterInfo() const
{
    auto pD3DEnum = DXUTGetD3D11Enumeration();
    return pD3DEnum->GetAdapterInfo( g_DeviceSettings.d3d11.AdapterOrdinal );
}


//-------------------------------------------------------------------------------------
CD3D11EnumDeviceInfo* CD3DSettingsDlg::GetCurrentD3D11DeviceInfo() const
{
    auto pD3DEnum = DXUTGetD3D11Enumeration();
    return pD3DEnum->GetDeviceInfo( g_DeviceSettings.d3d11.AdapterOrdinal,
                                    g_DeviceSettings.d3d11.DriverType );
}


//-------------------------------------------------------------------------------------
CD3D11EnumOutputInfo* CD3DSettingsDlg::GetCurrentD3D11OutputInfo() const
{
    auto pD3DEnum = DXUTGetD3D11Enumeration();
    return pD3DEnum->GetOutputInfo( g_DeviceSettings.d3d11.AdapterOrdinal,
                                    g_DeviceSettings.d3d11.Output );
}

//-------------------------------------------------------------------------------------
CD3D11EnumDeviceSettingsCombo* CD3DSettingsDlg::GetCurrentD3D11DeviceSettingsCombo() const
{
    auto pD3DEnum = DXUTGetD3D11Enumeration();
    return pD3DEnum->GetDeviceSettingsCombo( g_DeviceSettings.d3d11.AdapterOrdinal,
                                             g_DeviceSettings.d3d11.sd.BufferDesc.Format,
                                             ( g_DeviceSettings.d3d11.sd.Windowed == TRUE ) );
}

HRESULT CD3DSettingsDlg::OnD3D11ResolutionChanged ()
{
    if ( g_DeviceSettings.d3d11.sd.Windowed )
        return S_OK;

    DWORD dwWidth, dwHeight;
    GetSelectedD3D11Resolution( &dwWidth, &dwHeight );
    g_DeviceSettings.d3d11.sd.BufferDesc.Width = dwWidth;
    g_DeviceSettings.d3d11.sd.BufferDesc.Height = dwHeight;

    // DXUTSETTINGSDLG_D3D11_REFRESH_RATE
    HRESULT hr = UpdateD3D11RefreshRates();
    if ( FAILED(hr) )
        return hr;

    return S_OK;
}

HRESULT CD3DSettingsDlg::OnFeatureLevelChanged ()
{
    HRESULT hr = E_FAIL;

    if (g_DeviceSettings.d3d11.DeviceFeatureLevel == GetSelectedFeatureLevel()) return S_OK;

    // Obtain a set of valid D3D11 device settings.
    UINT CreateFlags = g_DeviceSettings.d3d11.CreateFlags;
    DXGI_FORMAT BackBufferFormat = g_DeviceSettings.d3d11.sd.BufferDesc.Format;
    UINT Count = g_DeviceSettings.d3d11.sd.SampleDesc.Count;
    UINT Quality = g_DeviceSettings.d3d11.sd.SampleDesc.Quality;
    DXGI_RATIONAL RefreshRate = g_DeviceSettings.d3d11.sd.BufferDesc.RefreshRate;
    ZeroMemory(&g_DeviceSettings, sizeof(g_DeviceSettings));

    DXUTApplyDefaultDeviceSettings(&g_DeviceSettings);
    g_DeviceSettings.d3d11.CreateFlags = CreateFlags;
    hr = DXUTSnapDeviceSettingsToEnumDevice(&g_DeviceSettings, true, GetSelectedFeatureLevel());
    g_DeviceSettings.d3d11.sd.BufferDesc.Format = BackBufferFormat;
    g_DeviceSettings.d3d11.sd.SampleDesc.Count = Count;
    g_DeviceSettings.d3d11.sd.SampleDesc.Quality = Quality;
    g_DeviceSettings.d3d11.sd.BufferDesc.RefreshRate = RefreshRate;

    auto pD3DEnum = DXUTGetD3D11Enumeration();
    auto pAdapterInfoList = pD3DEnum->GetAdapterInfoList();

    // DXUTSETTINGSDLG_ADAPTER
    auto pAdapterComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_ADAPTER );
    pAdapterComboBox->RemoveAllItems();

    for( auto it = pAdapterInfoList->cbegin(); it != pAdapterInfoList->cend(); ++it )
    {
        AddAdapter( (*it)->szUniqueDescription, (*it)->AdapterOrdinal );
    }

    pAdapterComboBox->SetSelectedByData( ULongToPtr( g_DeviceSettings.d3d11.AdapterOrdinal ) );

    // DXUTSETTINGSDLG_D3D11_BACK_BUFFER_FORMAT
    auto pBackBufferFormatComboBox = m_Dialog.GetComboBox(DXUTSETTINGSDLG_D3D11_BACK_BUFFER_FORMAT);
    pBackBufferFormatComboBox->RemoveAllItems();

    auto pAdapterInfo = GetCurrentD3D11AdapterInfo();
    if (!pAdapterInfo)
        return E_FAIL;

    bool bWindowed = IsWindowed();

    for (size_t idc = 0; idc < pAdapterInfo->deviceSettingsComboList.size(); idc++)
    {
        auto pDeviceCombo = pAdapterInfo->deviceSettingsComboList[idc];
        if ((pDeviceCombo->Windowed == TRUE) == bWindowed)
        {
            AddD3D11BackBufferFormat(pDeviceCombo->BackBufferFormat);
        }
    }

    pBackBufferFormatComboBox->SetSelectedByData( ULongToPtr(g_DeviceSettings.d3d11.sd.BufferDesc.Format) );

    hr = OnBackBufferFormatChanged();
    if (FAILED(hr))
        return hr;

    auto pCheckBox = m_Dialog.GetCheckBox( DXUTSETTINGSDLG_D3D11_DEBUG_DEVICE );
    pCheckBox->SetChecked( 0 != ( g_DeviceSettings.d3d11.CreateFlags & D3D11_CREATE_DEVICE_DEBUG ) );

    hr = OnAdapterChanged();
    if( FAILED( hr ) )
        return hr;
    
    return hr;
}


//-------------------------------------------------------------------------------------
HRESULT CD3DSettingsDlg::OnAdapterChanged()
{
    // Store the adapter index
    g_DeviceSettings.d3d11.AdapterOrdinal = GetSelectedAdapter();

    // DXUTSETTINGSDLG_DEVICE_TYPE
    auto pDeviceTypeComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_DEVICE_TYPE );
    pDeviceTypeComboBox->RemoveAllItems();

    auto pAdapterInfo = GetCurrentD3D11AdapterInfo();
    if( !pAdapterInfo )
        return E_FAIL;

    for( size_t iDeviceInfo = 0; iDeviceInfo < pAdapterInfo->deviceInfoList.size(); iDeviceInfo++ )
    {
        auto pDeviceInfo = pAdapterInfo->deviceInfoList[ iDeviceInfo ];
        AddD3D11DeviceType( pDeviceInfo->DeviceType );
    }

    pDeviceTypeComboBox->SetSelectedByData( ULongToPtr( g_DeviceSettings.d3d11.DriverType ) );

    HRESULT hr = OnDeviceTypeChanged();
    if( FAILED( hr ) )
        return hr;

    return S_OK;
}



//-------------------------------------------------------------------------------------
HRESULT CD3DSettingsDlg::OnDeviceTypeChanged()
{
    HRESULT hr = S_OK;

    g_DeviceSettings.d3d11.DriverType = GetSelectedD3D11DeviceType();

    // DXUTSETTINGSDLG_WINDOWED, DXUTSETTINGSDLG_FULLSCREEN
    m_Dialog.SetControlEnabled( DXUTSETTINGSDLG_WINDOWED, true );
    if (g_DeviceSettings.d3d11.DriverType == D3D_DRIVER_TYPE_WARP )
    {
        m_Dialog.SetControlEnabled(DXUTSETTINGSDLG_FULLSCREEN, false );
        g_DeviceSettings.d3d11.sd.Windowed = TRUE;
    }
    else
    {
        m_Dialog.SetControlEnabled(DXUTSETTINGSDLG_FULLSCREEN, true );
    }

    SetWindowed( g_DeviceSettings.d3d11.sd.Windowed != 0 );

    auto pBestDeviceSettingsCombo = DXUTGetD3D11Enumeration()->GetDeviceSettingsCombo(
        g_DeviceSettings.d3d11.AdapterOrdinal, g_DeviceSettings.d3d11.sd.BufferDesc.Format,
        ( g_DeviceSettings.d3d11.sd.Windowed != 0 ) );

    if( !pBestDeviceSettingsCombo )
        return DXUT_ERR_MSGBOX( L"GetDeviceSettingsCombo", E_INVALIDARG );

    D3D_FEATURE_LEVEL clampFL;
    if ( g_DeviceSettings.d3d11.DriverType == D3D_DRIVER_TYPE_WARP )
        clampFL = DXUTGetD3D11Enumeration()->GetWARPFeaturevel();
    else if ( g_DeviceSettings.d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE )
        clampFL = DXUTGetD3D11Enumeration()->GetREFFeaturevel();
    else
        clampFL = pBestDeviceSettingsCombo->pDeviceInfo->MaxLevel;

    if ( g_DeviceSettings.d3d11.DeviceFeatureLevel > clampFL
         || clampFL > pBestDeviceSettingsCombo->pDeviceInfo->MaxLevel )
    {
        g_DeviceSettings.d3d11.DeviceFeatureLevel = std::min<D3D_FEATURE_LEVEL>( g_DeviceSettings.d3d11.DeviceFeatureLevel,
                                                                                 clampFL );
                
        CDXUTComboBox *pFeatureLevelBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_FEATURE_LEVEL );
        pFeatureLevelBox->RemoveAllItems();
        for (int fli = 0; fli < TOTAL_FEATURE_LEVELS; fli++)
        {
            if (m_Levels[fli] >= g_DeviceSettings.MinimumFeatureLevel 
                && m_Levels[fli] <= clampFL)
            {
                AddD3D11FeatureLevel( m_Levels[fli] );
            }
        } 
        pFeatureLevelBox->SetSelectedByData( ULongToPtr( g_DeviceSettings.d3d11.DeviceFeatureLevel ) );

        hr = OnFeatureLevelChanged();
        if( FAILED( hr ) )
            return hr;
    }

    hr = OnWindowedFullScreenChanged();
    if( FAILED( hr ) )
        return hr;

    return S_OK;
}



//-------------------------------------------------------------------------------------
HRESULT CD3DSettingsDlg::OnWindowedFullScreenChanged()
{
    HRESULT hr = S_OK;
    bool bWindowed = IsWindowed();

    m_Dialog.SetControlEnabled( DXUTSETTINGSDLG_D3D11_ADAPTER_OUTPUT_LABEL, !bWindowed );
    m_Dialog.SetControlEnabled( DXUTSETTINGSDLG_D3D11_RESOLUTION_LABEL, !bWindowed );
    m_Dialog.SetControlEnabled( DXUTSETTINGSDLG_D3D11_REFRESH_RATE_LABEL, !bWindowed );

    m_Dialog.SetControlEnabled( DXUTSETTINGSDLG_RESOLUTION_SHOW_ALL, !bWindowed );
    m_Dialog.SetControlEnabled( DXUTSETTINGSDLG_D3D11_ADAPTER_OUTPUT, !bWindowed );
    m_Dialog.SetControlEnabled( DXUTSETTINGSDLG_D3D11_RESOLUTION, !bWindowed );
    m_Dialog.SetControlEnabled( DXUTSETTINGSDLG_D3D11_REFRESH_RATE, !bWindowed );

    g_DeviceSettings.d3d11.sd.Windowed = bWindowed;

    // Get available adapter output
    auto pD3DEnum = DXUTGetD3D11Enumeration();

    auto pOutputComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_ADAPTER_OUTPUT );
    pOutputComboBox->RemoveAllItems();

    auto pAdapterInfo = pD3DEnum->GetAdapterInfo( g_DeviceSettings.d3d11.AdapterOrdinal );
    for( size_t ioutput = 0; ioutput < pAdapterInfo->outputInfoList.size(); ++ioutput )
    {
        auto pOutputInfo = pAdapterInfo->outputInfoList[ ioutput ];
        AddD3D11AdapterOutput( pOutputInfo->Desc.DeviceName, pOutputInfo->Output );
    }

    pOutputComboBox->SetSelectedByData( ULongToPtr( g_DeviceSettings.d3d11.Output ) );

    hr = OnAdapterOutputChanged();
    if( FAILED( hr ) )
        return hr;

    hr = UpdateD3D11Resolutions();
    if (FAILED(hr))
        return hr;

    hr = UpdateD3D11RefreshRates();
    if ( FAILED(hr) )
        return hr;

    return S_OK;
}


//-------------------------------------------------------------------------------------
HRESULT CD3DSettingsDlg::OnAdapterOutputChanged()
{
    HRESULT hr;

    bool bWindowed = IsWindowed();
    g_DeviceSettings.d3d11.sd.Windowed = bWindowed;

    // If windowed, get the appropriate adapter format from Direct3D
    if( g_DeviceSettings.d3d11.sd.Windowed )
    {
        DXGI_MODE_DESC mode;
        hr = DXUTGetD3D11AdapterDisplayMode( g_DeviceSettings.d3d11.AdapterOrdinal,
                                                g_DeviceSettings.d3d11.Output, &mode );
        if( FAILED( hr ) )
            return DXTRACE_ERR( L"GetD3D11AdapterDisplayMode", hr );

        // Default resolution to the fullscreen res that was last used
        RECT rc = DXUTGetFullsceenClientRectAtModeChange();
        if( rc.right == 0 || rc.bottom == 0 )
        {
            // If nothing last used, then default to the adapter desktop res
            g_DeviceSettings.d3d11.sd.BufferDesc.Width = mode.Width;
            g_DeviceSettings.d3d11.sd.BufferDesc.Height = mode.Height;
        }
        else
        {
            g_DeviceSettings.d3d11.sd.BufferDesc.Width = rc.right;
            g_DeviceSettings.d3d11.sd.BufferDesc.Height = rc.bottom;
        }

        g_DeviceSettings.d3d11.sd.BufferDesc.RefreshRate.Numerator =
        g_DeviceSettings.d3d11.sd.BufferDesc.RefreshRate.Denominator = 0;
    }

    auto pAdapterInfo = GetCurrentD3D11AdapterInfo();
    if( !pAdapterInfo )
        return E_FAIL;

    // DXUTSETTINGSDLG_D3D11_RESOLUTION
    hr = UpdateD3D11Resolutions();
    if( FAILED( hr ) )
        return hr;

    // DXUTSETTINGSDLG_D3D11_REFRESH_RATE
    hr = UpdateD3D11RefreshRates();
    if ( FAILED(hr) )
        return hr;

    // DXUTSETTINGSDLG_D3D11_BACK_BUFFER_FORMAT
    auto pBackBufferFormatComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_BACK_BUFFER_FORMAT );
    pBackBufferFormatComboBox->RemoveAllItems();

    for( size_t idc = 0; idc < pAdapterInfo->deviceSettingsComboList.size(); idc++ )
    {
        auto pDeviceCombo = pAdapterInfo->deviceSettingsComboList[ idc ];
        if( ( pDeviceCombo->Windowed == TRUE ) == bWindowed )
        {
            AddD3D11BackBufferFormat( pDeviceCombo->BackBufferFormat );
        }
    }

    pBackBufferFormatComboBox->SetSelectedByData( ULongToPtr( g_DeviceSettings.d3d11.sd.BufferDesc.Format ) );

    hr = OnBackBufferFormatChanged();
    if( FAILED( hr ) )
        return hr;

    hr = OnRefreshRateChanged();
    if( FAILED( hr ) )
        return hr;

    return S_OK;
}


//-------------------------------------------------------------------------------------
HRESULT CD3DSettingsDlg::OnRefreshRateChanged()
{
    // Set refresh rate
    g_DeviceSettings.d3d11.sd.BufferDesc.RefreshRate = GetSelectedD3D11RefreshRate();

    return S_OK;
}


//-------------------------------------------------------------------------------------
HRESULT CD3DSettingsDlg::OnBackBufferFormatChanged()
{
    HRESULT hr = S_OK;

    g_DeviceSettings.d3d11.sd.BufferDesc.Format = GetSelectedD3D11BackBufferFormat();

    DXGI_FORMAT backBufferFormat = g_DeviceSettings.d3d11.sd.BufferDesc.Format;

    auto pAdapterInfo = GetCurrentD3D11AdapterInfo();
    if( !pAdapterInfo )
        return E_FAIL;

    for( size_t idc = 0; idc < pAdapterInfo->deviceSettingsComboList.size(); idc++ )
    {
        auto pDeviceCombo = pAdapterInfo->deviceSettingsComboList[ idc ];

        if( pDeviceCombo->Windowed == ( g_DeviceSettings.d3d11.sd.Windowed == TRUE ) &&
            pDeviceCombo->BackBufferFormat == backBufferFormat &&
            pDeviceCombo->DeviceType == g_DeviceSettings.d3d11.DriverType )
        {
            auto pMultisampleCountCombo = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_MULTISAMPLE_COUNT );
            pMultisampleCountCombo->RemoveAllItems();

            for( auto it = pDeviceCombo->multiSampleCountList.cbegin(); it != pDeviceCombo->multiSampleCountList.cend(); ++it )
                AddD3D11MultisampleCount( *it );
            pMultisampleCountCombo->SetSelectedByData( ULongToPtr( g_DeviceSettings.d3d11.sd.SampleDesc.Count ) );

            hr = OnMultisampleTypeChanged();
            if( FAILED( hr ) )
                return hr;

            auto pPresentIntervalComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_PRESENT_INTERVAL );
            pPresentIntervalComboBox->SetSelectedByData( ULongToPtr( g_DeviceSettings.d3d11.SyncInterval ) );

            hr = OnPresentIntervalChanged();
            if( FAILED( hr ) )
                return hr;

            hr = UpdateD3D11Resolutions();
            if( FAILED( hr ) )
                return hr;

            hr = UpdateD3D11RefreshRates();
            if ( FAILED(hr) )
                return hr;

            break;
        }
    }

    return S_OK;
}


//-------------------------------------------------------------------------------------
HRESULT CD3DSettingsDlg::OnMultisampleTypeChanged()
{
    HRESULT hr = S_OK;

    UINT multisampleCount = GetSelectedD3D11MultisampleCount();
    g_DeviceSettings.d3d11.sd.SampleDesc.Count = multisampleCount;

    auto pDeviceSettingsCombo = GetCurrentD3D11DeviceSettingsCombo();
    if( !pDeviceSettingsCombo )
        return E_FAIL;

    UINT MaxQuality = 0;
    for( size_t iCount = 0; iCount < pDeviceSettingsCombo->multiSampleCountList.size(); iCount++ )
    {
        UINT Count = pDeviceSettingsCombo->multiSampleCountList[ iCount ];
        if( Count == multisampleCount )
        {
            MaxQuality = pDeviceSettingsCombo->multiSampleQualityList[ iCount ];
            break;
        }
    }

    // DXUTSETTINGSDLG_D3D11_MULTISAMPLE_QUALITY
    auto pMultisampleQualityCombo = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_MULTISAMPLE_QUALITY );
    pMultisampleQualityCombo->RemoveAllItems();

    for( UINT iQuality = 0; iQuality < MaxQuality; iQuality++ )
    {
        AddD3D11MultisampleQuality( iQuality );
    }

    pMultisampleQualityCombo->SetSelectedByData( ULongToPtr( g_DeviceSettings.d3d11.sd.SampleDesc.Quality ) );

    hr = OnMultisampleQualityChanged();
    if( FAILED( hr ) )
        return hr;

    return S_OK;
}


//-------------------------------------------------------------------------------------
HRESULT CD3DSettingsDlg::OnMultisampleQualityChanged()
{
            g_DeviceSettings.d3d11.sd.SampleDesc.Quality = GetSelectedD3D11MultisampleQuality();

    return S_OK;
}


//-------------------------------------------------------------------------------------
HRESULT CD3DSettingsDlg::OnPresentIntervalChanged()
{
            g_DeviceSettings.d3d11.SyncInterval = GetSelectedD3D11PresentInterval();

    return S_OK;
}


//-------------------------------------------------------------------------------------
HRESULT CD3DSettingsDlg::OnDebugDeviceChanged()
{
    bool bDebugDevice = GetSelectedDebugDeviceValue();

    if( bDebugDevice )
        g_DeviceSettings.d3d11.CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
    else
        g_DeviceSettings.d3d11.CreateFlags &= ~D3D11_CREATE_DEVICE_DEBUG;

    return S_OK;
}


//-------------------------------------------------------------------------------------
void CD3DSettingsDlg::AddAdapter( _In_z_ const WCHAR* strDescription, _In_ UINT iAdapter )
{
    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_ADAPTER );

    if( !pComboBox->ContainsItem( strDescription ) )
        pComboBox->AddItem( strDescription, ULongToPtr( iAdapter ) );
}


//-------------------------------------------------------------------------------------
UINT CD3DSettingsDlg::GetSelectedAdapter() const
{
    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_ADAPTER );

    return PtrToUlong( pComboBox->GetSelectedData() );
}


//-------------------------------------------------------------------------------------
void CD3DSettingsDlg::SetWindowed( _In_ bool bWindowed )
{
    auto pRadioButton = m_Dialog.GetRadioButton( DXUTSETTINGSDLG_WINDOWED );
    pRadioButton->SetChecked( bWindowed );

    pRadioButton = m_Dialog.GetRadioButton( DXUTSETTINGSDLG_FULLSCREEN );
    pRadioButton->SetChecked( !bWindowed );

    m_Dialog.SetControlEnabled(DXUTSETTINGSDLG_D3D11_ADAPTER_OUTPUT_LABEL, !bWindowed);
    m_Dialog.SetControlEnabled(DXUTSETTINGSDLG_D3D11_RESOLUTION_LABEL, !bWindowed);
    m_Dialog.SetControlEnabled(DXUTSETTINGSDLG_D3D11_REFRESH_RATE_LABEL, !bWindowed);

    m_Dialog.SetControlEnabled(DXUTSETTINGSDLG_RESOLUTION_SHOW_ALL, !bWindowed);
    m_Dialog.SetControlEnabled(DXUTSETTINGSDLG_D3D11_ADAPTER_OUTPUT, !bWindowed);
    m_Dialog.SetControlEnabled(DXUTSETTINGSDLG_D3D11_RESOLUTION, !bWindowed);
    m_Dialog.SetControlEnabled(DXUTSETTINGSDLG_D3D11_REFRESH_RATE, !bWindowed);
}


//-------------------------------------------------------------------------------------
bool CD3DSettingsDlg::IsWindowed() const
{
    auto pRadioButton = m_Dialog.GetRadioButton( DXUTSETTINGSDLG_WINDOWED );
    return pRadioButton->GetChecked();
}


//-------------------------------------------------------------------------------------
void CD3DSettingsDlg::AddD3D11AdapterOutput( _In_z_ const WCHAR* strName, _In_ UINT Output )
{
    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_ADAPTER_OUTPUT );

    if( !pComboBox->ContainsItem( strName ) )
        pComboBox->AddItem( strName, ULongToPtr( Output ) );
}


//-------------------------------------------------------------------------------------
UINT CD3DSettingsDlg::GetSelectedD3D11AdapterOutput() const
{
    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_ADAPTER_OUTPUT );

    return PtrToUlong( pComboBox->GetSelectedData() );
}


//-------------------------------------------------------------------------------------
_Use_decl_annotations_
void CD3DSettingsDlg::AddD3D11Resolution( DWORD dwWidth, DWORD dwHeight )
{
    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_RESOLUTION );

    DWORD dwResolutionData;
    WCHAR strResolution[50];
    dwResolutionData = MAKELONG( dwWidth, dwHeight );
    swprintf_s( strResolution, 50, L"%u by %u", dwWidth, dwHeight );

    if( !pComboBox->ContainsItem( strResolution ) )
        pComboBox->AddItem( strResolution, ULongToPtr( dwResolutionData ) );
}


//-------------------------------------------------------------------------------------
_Use_decl_annotations_
void CD3DSettingsDlg::GetSelectedD3D11Resolution( DWORD* pdwWidth, DWORD* pdwHeight ) const
{
    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_RESOLUTION );

    DWORD dwResolution = PtrToUlong( pComboBox->GetSelectedData() );

    *pdwWidth = LOWORD( dwResolution );
    *pdwHeight = HIWORD( dwResolution );
}


//-------------------------------------------------------------------------------------
void CD3DSettingsDlg::AddD3D11FeatureLevel( _In_ D3D_FEATURE_LEVEL fl)
{
    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_FEATURE_LEVEL );
    switch( fl )
    {
    case D3D_FEATURE_LEVEL_9_1: 
        {
            if( !pComboBox->ContainsItem( L"D3D_FEATURE_LEVEL_9_1" ) )
                pComboBox->AddItem( L"D3D_FEATURE_LEVEL_9_1", ULongToPtr( D3D_FEATURE_LEVEL_9_1 ) ); 
        }
        break;
    case D3D_FEATURE_LEVEL_9_2: 
        {
            if( !pComboBox->ContainsItem( L"D3D_FEATURE_LEVEL_9_2" ) )
                pComboBox->AddItem( L"D3D_FEATURE_LEVEL_9_2", ULongToPtr( D3D_FEATURE_LEVEL_9_2 ) ); 
        }
        break;
    case D3D_FEATURE_LEVEL_9_3: 
        {
            if( !pComboBox->ContainsItem( L"D3D_FEATURE_LEVEL_9_3" ) )
                pComboBox->AddItem( L"D3D_FEATURE_LEVEL_9_3", ULongToPtr( D3D_FEATURE_LEVEL_9_3 ) ); 
        }
        break;
    case D3D_FEATURE_LEVEL_10_0: 
        {
            if( !pComboBox->ContainsItem( L"D3D_FEATURE_LEVEL_10_0" ) )
                pComboBox->AddItem( L"D3D_FEATURE_LEVEL_10_0", ULongToPtr( D3D_FEATURE_LEVEL_10_0 ) ); 
        }
        break;
    case D3D_FEATURE_LEVEL_10_1: 
        {
            if( !pComboBox->ContainsItem( L"D3D_FEATURE_LEVEL_10_1" ) )
                pComboBox->AddItem( L"D3D_FEATURE_LEVEL_10_1", ULongToPtr( D3D_FEATURE_LEVEL_10_1 ) ); 
        }
        break;
    case D3D_FEATURE_LEVEL_11_0: 
        {
            if( !pComboBox->ContainsItem( L"D3D_FEATURE_LEVEL_11_0" ) )
                pComboBox->AddItem( L"D3D_FEATURE_LEVEL_11_0", ULongToPtr( D3D_FEATURE_LEVEL_11_0 ) ); 
        }
        break;
    case D3D_FEATURE_LEVEL_11_1: 
        {
            if( !pComboBox->ContainsItem( L"D3D_FEATURE_LEVEL_11_1" ) )
                pComboBox->AddItem( L"D3D_FEATURE_LEVEL_11_1", ULongToPtr( D3D_FEATURE_LEVEL_11_1 ) ); 
        }
        break;
    }

}

D3D_FEATURE_LEVEL CD3DSettingsDlg::GetSelectedFeatureLevel() const
{
    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_FEATURE_LEVEL );

    return (D3D_FEATURE_LEVEL)PtrToUlong( pComboBox->GetSelectedData() );
}


//-------------------------------------------------------------------------------------
void CD3DSettingsDlg::AddD3D11RefreshRate( _In_ DXGI_RATIONAL RefreshRate )
{
    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_REFRESH_RATE );

    WCHAR strRefreshRate[50];

    if( RefreshRate.Numerator == 0 && RefreshRate.Denominator == 0 )
        wcscpy_s( strRefreshRate, 50, L"Default Rate" );
    else
        swprintf_s( strRefreshRate, 50, L"%u Hz", RefreshRate.Numerator / RefreshRate.Denominator );

    if( !pComboBox->ContainsItem( strRefreshRate ) )
    {
        auto pNewRate = new (std::nothrow) DXGI_RATIONAL;
        if( pNewRate )
        {
            *pNewRate = RefreshRate;
            pComboBox->AddItem( strRefreshRate, pNewRate );
        }
    }
}


//-------------------------------------------------------------------------------------
DXGI_RATIONAL CD3DSettingsDlg::GetSelectedD3D11RefreshRate() const
{
    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_REFRESH_RATE );
    return *reinterpret_cast<DXGI_RATIONAL*>( pComboBox->GetSelectedData() );
}


//-------------------------------------------------------------------------------------
void CD3DSettingsDlg::AddD3D11BackBufferFormat( _In_ DXGI_FORMAT format )
{
    if ( g_DeviceSettings.d3d11.DeviceFeatureLevel < D3D_FEATURE_LEVEL_10_0 )
    {
        if ( (format == DXGI_FORMAT_R16G16B16A16_FLOAT) || (format == DXGI_FORMAT_R10G10B10A2_UNORM) )
            return;
    }

    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_BACK_BUFFER_FORMAT );

    if( !pComboBox->ContainsItem( DXUTDXGIFormatToString( format, TRUE ) ) )
        pComboBox->AddItem( DXUTDXGIFormatToString( format, TRUE ), ULongToPtr( format ) );
}


//-------------------------------------------------------------------------------------
DXGI_FORMAT CD3DSettingsDlg::GetSelectedD3D11BackBufferFormat() const
{
    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_BACK_BUFFER_FORMAT );

    return ( DXGI_FORMAT )PtrToUlong( pComboBox->GetSelectedData() );
}


//-------------------------------------------------------------------------------------
void CD3DSettingsDlg::AddD3D11MultisampleCount( _In_ UINT Count )
{
    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_MULTISAMPLE_COUNT );

    WCHAR str[50];
    swprintf_s( str, 50, L"%u", Count );

    if( !pComboBox->ContainsItem( str ) )
        pComboBox->AddItem( str, ULongToPtr( Count ) );
}


//-------------------------------------------------------------------------------------
UINT CD3DSettingsDlg::GetSelectedD3D11MultisampleCount() const
{
    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_MULTISAMPLE_COUNT );

    return ( UINT )PtrToUlong( pComboBox->GetSelectedData() );
}


//-------------------------------------------------------------------------------------
void CD3DSettingsDlg::AddD3D11MultisampleQuality( _In_ UINT Quality )
{
    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_MULTISAMPLE_QUALITY );

    WCHAR strQuality[50];
    swprintf_s( strQuality, 50, L"%u", Quality );

    if( !pComboBox->ContainsItem( strQuality ) )
        pComboBox->AddItem( strQuality, ULongToPtr( Quality ) );
}


//-------------------------------------------------------------------------------------
UINT CD3DSettingsDlg::GetSelectedD3D11MultisampleQuality() const
{
    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_MULTISAMPLE_QUALITY );

    return ( UINT )PtrToUlong( pComboBox->GetSelectedData() );
}


//-------------------------------------------------------------------------------------
DWORD CD3DSettingsDlg::GetSelectedD3D11PresentInterval() const
{
    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_PRESENT_INTERVAL );

    return PtrToUlong( pComboBox->GetSelectedData() );
}

//-------------------------------------------------------------------------------------
bool CD3DSettingsDlg::GetSelectedDebugDeviceValue() const
{
    auto pCheckBox = m_Dialog.GetCheckBox( DXUTSETTINGSDLG_D3D11_DEBUG_DEVICE );

    return pCheckBox->GetChecked();
}


//--------------------------------------------------------------------------------------
// Updates the resolution list for D3D11
//--------------------------------------------------------------------------------------
HRESULT CD3DSettingsDlg::UpdateD3D11Resolutions()
{
    const DWORD dwWidth = g_DeviceSettings.d3d11.sd.BufferDesc.Width;
    const DWORD dwHeight = g_DeviceSettings.d3d11.sd.BufferDesc.Height;

    // DXUTSETTINGSDLG_D3D11_RESOLUTION
    auto pResolutionComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_RESOLUTION );
    pResolutionComboBox->RemoveAllItems();

    auto pOutputInfo = GetCurrentD3D11OutputInfo();
    if( !pOutputInfo )
        return E_FAIL;

    bool bWindowed = IsWindowed();
    if ( !bWindowed )
    {
        auto pShowAllCB = m_Dialog.GetCheckBox( DXUTSETTINGSDLG_RESOLUTION_SHOW_ALL );
        bool bShowAll = pShowAllCB->GetChecked();

        // Get the desktop aspect ratio
        DXGI_MODE_DESC dmDesktop;
        DXUTGetDesktopResolution(g_DeviceSettings.d3d11.AdapterOrdinal, &dmDesktop.Width, &dmDesktop.Height);
        float fDesktopAspectRatio = dmDesktop.Width / (float) dmDesktop.Height;

        if ( !bShowAll && !DXUTIsWindowed() )
        {
            float fAspect = (float) dwWidth / (float) dwHeight;
            if ( fabsf(fDesktopAspectRatio - fAspect) >= 0.05f )
            {
                // Our current fullscren resolution should be listed in the combo box despite the aspect ratio
                pShowAllCB->SetChecked(true);
                bShowAll = true;
            }
        }

        for (size_t idm = 0; idm < pOutputInfo->displayModeList.size(); idm++)
        {
            auto DisplayMode = pOutputInfo->displayModeList[idm];
            float fAspect = (float) DisplayMode.Width / (float) DisplayMode.Height;

            if (DisplayMode.Format == g_DeviceSettings.d3d11.sd.BufferDesc.Format)
            {
                // If "Show All" is not checked, then hide all resolutions
                // that don't match the aspect ratio of the desktop resolution
                if (bShowAll || (!bShowAll && fabsf(fDesktopAspectRatio - fAspect) < 0.05f))
                {
                    AddD3D11Resolution(DisplayMode.Width, DisplayMode.Height);
                }
            }
        }
    }
    else
    {
        pResolutionComboBox->RemoveAllItems();
        AddD3D11Resolution( dwWidth, dwHeight );
    }

    pResolutionComboBox->SetSelectedByData(ULongToPtr(MAKELONG(dwWidth, dwHeight)));
    OnD3D11ResolutionChanged();

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Updates the refresh list for D3D11
//--------------------------------------------------------------------------------------
HRESULT CD3DSettingsDlg::UpdateD3D11RefreshRates()
{
    const DWORD dwWidth = g_DeviceSettings.d3d11.sd.BufferDesc.Width;
    const DWORD dwHeight = g_DeviceSettings.d3d11.sd.BufferDesc.Height;
    DXGI_FORMAT backBuffer = g_DeviceSettings.d3d11.sd.BufferDesc.Format;
    const DXGI_RATIONAL RefreshRate = g_DeviceSettings.d3d11.sd.BufferDesc.RefreshRate;

    auto pRefreshRateComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_D3D11_REFRESH_RATE );
    for( UINT i = 0; i < pRefreshRateComboBox->GetNumItems(); ++i )
    {
        auto pRefreshRate = reinterpret_cast<DXGI_RATIONAL*>( pRefreshRateComboBox->GetItemData( i ) );
        delete pRefreshRate;
    }
    pRefreshRateComboBox->RemoveAllItems();

    bool bWindowed = IsWindowed();
    if( bWindowed )
    {
        DXGI_RATIONAL default;
        default.Denominator = default.Numerator = 0;
        AddD3D11RefreshRate( default );
    }
    else
    {
        auto pD3DEnum = DXUTGetD3D11Enumeration();
        if ( !pD3DEnum )
            return E_POINTER;

        auto pOutputInfo = pD3DEnum->GetOutputInfo( g_DeviceSettings.d3d11.AdapterOrdinal, g_DeviceSettings.d3d11.Output );
        if ( !pOutputInfo )
            return E_POINTER;

        for( auto it = pOutputInfo->displayModeList.cbegin(); it != pOutputInfo->displayModeList.cend(); ++it )
        {
            if ( it->Width == dwWidth && it->Height == dwHeight && it->Format == backBuffer )
                AddD3D11RefreshRate( it->RefreshRate );
        }

        SetSelectedD3D11RefreshRate( RefreshRate );
    }

    return S_OK;
}


//
//-------------------------------------------------------------------------------------
void CD3DSettingsDlg::AddD3D11DeviceType( _In_ D3D_DRIVER_TYPE devType )
{
    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_DEVICE_TYPE );

    if( !pComboBox->ContainsItem( DXUTDeviceTypeToString( devType ) ) )
        pComboBox->AddItem( DXUTDeviceTypeToString( devType ), ULongToPtr( devType ) );
}


//-------------------------------------------------------------------------------------
D3D_DRIVER_TYPE CD3DSettingsDlg::GetSelectedD3D11DeviceType() const
{
    auto pComboBox = m_Dialog.GetComboBox( DXUTSETTINGSDLG_DEVICE_TYPE );

    return ( D3D_DRIVER_TYPE )PtrToUlong( pComboBox->GetSelectedData() );
}


void CD3DSettingsDlg::UpdateModeChangeTimeoutText( _In_ int nSecRemaining )
{
    const WCHAR StrTimeout[] = L"Reverting to previous display settings in %d seconds";
    const DWORD CchBuf = sizeof( StrTimeout ) / sizeof( WCHAR ) + 16;
    WCHAR buf[CchBuf];

    swprintf_s( buf, CchBuf, StrTimeout, nSecRemaining );

    auto pStatic = m_RevertModeDialog.GetStatic( DXUTSETTINGSDLG_STATIC_MODE_CHANGE_TIMEOUT );
    pStatic->SetText( buf );
}

//--------------------------------------------------------------------------------------
// Returns the string for the given D3D_DRIVER_TYPE.
//--------------------------------------------------------------------------------------
const WCHAR* DXUTDeviceTypeToString( _In_ D3D_DRIVER_TYPE devType )
{
    switch( devType )
    {
        case D3D_DRIVER_TYPE_HARDWARE:
            return L"D3D_DRIVER_TYPE_HARDWARE";
        case D3D_DRIVER_TYPE_REFERENCE:
            return L"D3D_DRIVER_TYPE_REFERENCE";
        case D3D_DRIVER_TYPE_NULL:
            return L"D3D_DRIVER_TYPE_NULL";
        case D3D_DRIVER_TYPE_WARP:
            return L"D3D_DRIVER_TYPE_WARP";
        default:
            return L"Unknown devType";
    }
}


