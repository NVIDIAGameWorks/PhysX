//--------------------------------------------------------------------------------------
// File: DXUTgui.cpp
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
#include "DXUTres.h"

#include "SDKMisc.h"

#include "DDSTextureLoader.h"

using namespace DirectX;

#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN 0x020B // (not always defined)
#endif
#ifndef WM_XBUTTONUP
#define WM_XBUTTONUP 0x020C // (not always defined)
#endif
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A // (not always defined)
#endif
#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120 // (not always defined)
#endif

// Minimum scroll bar thumb size
#define SCROLLBAR_MINTHUMBSIZE 8

// Delay and repeat period when clicking on the scroll bar arrows
#define SCROLLBAR_ARROWCLICK_DELAY  0.33
#define SCROLLBAR_ARROWCLICK_REPEAT 0.05

#define DXUT_NEAR_BUTTON_DEPTH 0.6f
#define DXUT_FAR_BUTTON_DEPTH 0.8f

#define DXUT_MAX_GUI_SPRITES 500

inline XMFLOAT4 D3DCOLOR_TO_D3DCOLORVALUE( DWORD c )
{
    return XMFLOAT4 ( ( ( c >> 16 ) & 0xFF ) / 255.0f,
                      ( ( c >> 8 ) & 0xFF ) / 255.0f,
                      ( c & 0xFF ) / 255.0f,
                      ( ( c >> 24 ) & 0xFF ) / 255.0f );
}

#define IMM32_DLLNAME L"imm32.dll"
#define VER_DLLNAME L"version.dll"

CHAR g_strUIEffectFile[] = \
    "Texture2D g_Texture;"\
    ""\
    "SamplerState Sampler"\
    "{"\
    "    Filter = MIN_MAG_MIP_LINEAR;"\
    "    AddressU = Wrap;"\
    "    AddressV = Wrap;"\
    "};"\
    ""\
    "BlendState UIBlend"\
    "{"\
    "    AlphaToCoverageEnable = FALSE;"\
    "    BlendEnable[0] = TRUE;"\
    "    SrcBlend = SRC_ALPHA;"\
    "    DestBlend = INV_SRC_ALPHA;"\
    "    BlendOp = ADD;"\
    "    SrcBlendAlpha = ONE;"\
    "    DestBlendAlpha = ZERO;"\
    "    BlendOpAlpha = ADD;"\
    "    RenderTargetWriteMask[0] = 0x0F;"\
    "};"\
    ""\
    "BlendState NoBlending"\
    "{"\
    "    BlendEnable[0] = FALSE;"\
    "    RenderTargetWriteMask[0] = 0x0F;"\
    "};"\
    ""\
    "DepthStencilState DisableDepth"\
    "{"\
    "    DepthEnable = false;"\
    "};"\
    "DepthStencilState EnableDepth"\
    "{"\
    "    DepthEnable = true;"\
    "};"\
    "struct VS_OUTPUT"\
    "{"\
    "    float4 Pos : POSITION;"\
    "    float4 Dif : COLOR;"\
    "    float2 Tex : TEXCOORD;"\
    "};"\
    ""\
    "VS_OUTPUT VS( float3 vPos : POSITION,"\
    "              float4 Dif : COLOR,"\
    "              float2 vTexCoord0 : TEXCOORD )"\
    "{"\
    "    VS_OUTPUT Output;"\
    ""\
    "    Output.Pos = float4( vPos, 1.0f );"\
    "    Output.Dif = Dif;"\
    "    Output.Tex = vTexCoord0;"\
    ""\
    "    return Output;"\
    "}"\
    ""\
    "float4 PS( VS_OUTPUT In ) : SV_Target"\
    "{"\
    "    return g_Texture.Sample( Sampler, In.Tex ) * In.Dif;"\
    "}"\
    ""\
    "float4 PSUntex( VS_OUTPUT In ) : SV_Target"\
    "{"\
    "    return In.Dif;"\
    "}"\
    ""\
    "technique10 RenderUI"\
    "{"\
    "    pass P0"\
    "    {"\
    "        SetVertexShader( CompileShader( vs_4_0, VS() ) );"\
    "        SetGeometryShader( NULL );"\
    "        SetPixelShader( CompileShader( ps_4_0, PS() ) );"\
    "        SetDepthStencilState( DisableDepth, 0 );"\
    "        SetBlendState( UIBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );"\
    "    }"\
    "}"\
    "technique10 RenderUIUntex"\
    "{"\
    "    pass P0"\
    "    {"\
    "        SetVertexShader( CompileShader( vs_4_0, VS() ) );"\
    "        SetGeometryShader( NULL );"\
    "        SetPixelShader( CompileShader( ps_4_0, PSUntex() ) );"\
    "        SetDepthStencilState( DisableDepth, 0 );"\
    "        SetBlendState( UIBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );"\
    "    }"\
    "}"\
    "technique10 RestoreState"\
    "{"\
    "    pass P0"\
    "    {"\
    "        SetDepthStencilState( EnableDepth, 0 );"\
    "        SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );"\
    "    }"\
    "}";
const UINT              g_uUIEffectFileSize = sizeof( g_strUIEffectFile );


// DXUT_MAX_EDITBOXLENGTH is the maximum string length allowed in edit boxes,
// including the nul terminator.
// 
// Uniscribe does not support strings having bigger-than-16-bits length.
// This means that the string must be less than 65536 characters long,
// including the nul terminator.
#define DXUT_MAX_EDITBOXLENGTH 0xFFFF


double                  CDXUTDialog::s_fTimeRefresh = 0.0f;
CDXUTControl*           CDXUTDialog::s_pControlFocus = nullptr;        // The control which has focus
CDXUTControl*           CDXUTDialog::s_pControlPressed = nullptr;      // The control currently pressed


struct DXUT_SCREEN_VERTEX
{
    float x, y, z, h;
    DWORD color;
    float tu, tv;
};

struct DXUT_SCREEN_VERTEX_UNTEX
{
    float x, y, z, h;
    DWORD color;
};

struct DXUT_SCREEN_VERTEX_10
{
    float x, y, z;
    XMFLOAT4 color;
    float tu, tv;
};


inline int RectWidth( RECT& rc )
{
    return ( ( rc ).right - ( rc ).left );
}
inline int RectHeight( RECT& rc )
{
    return ( ( rc ).bottom - ( rc ).top );
}


//======================================================================================
// Font11
//======================================================================================

ID3D11Buffer* g_pFontBuffer11 = nullptr;
UINT g_FontBufferBytes11 = 0;
std::vector<DXUTSpriteVertex> g_FontVertices;
ID3D11ShaderResourceView* g_pFont11 = nullptr;
ID3D11InputLayout* g_pInputLayout11 = nullptr;

//--------------------------------------------------------------------------------------
HRESULT InitFont11( _In_ ID3D11Device* pd3d11Device, _In_ ID3D11InputLayout* pInputLayout )
{
    HRESULT hr = S_OK;
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"UI\\Font.dds" ) );
    
    V_RETURN( CreateDDSTextureFromFile( pd3d11Device, str, nullptr, &g_pFont11 ) );

    g_pInputLayout11 = pInputLayout;
    return hr;
}


//--------------------------------------------------------------------------------------
void EndFont11()
{
    SAFE_RELEASE( g_pFontBuffer11 );
    g_FontBufferBytes11 = 0;
    SAFE_RELEASE( g_pFont11 );
}


//--------------------------------------------------------------------------------------
void BeginText11()
{
    g_FontVertices.clear();
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void DrawText11DXUT( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3d11DeviceContext,
                 LPCWSTR strText, const RECT& rcScreen, XMFLOAT4 vFontColor,
                 float fBBWidth, float fBBHeight, bool bCenter )
{
    float fCharTexSizeX = 0.010526315f;
    //float fGlyphSizeX = 14.0f / fBBWidth;
    //float fGlyphSizeY = 32.0f / fBBHeight;
    float fGlyphSizeX = 15.0f / fBBWidth;
    float fGlyphSizeY = 42.0f / fBBHeight;


    float fRectLeft = rcScreen.left / fBBWidth;
    float fRectTop = 1.0f - rcScreen.top / fBBHeight;

    fRectLeft = fRectLeft * 2.0f - 1.0f;
    fRectTop = fRectTop * 2.0f - 1.0f;

    int NumChars = (int)wcslen( strText );
    if (bCenter) {
        float fRectRight = rcScreen.right / fBBWidth;
        fRectRight = fRectRight * 2.0f - 1.0f;
        float fRectBottom = 1.0f - rcScreen.bottom / fBBHeight;
        fRectBottom = fRectBottom * 2.0f - 1.0f;
        float fcenterx = ((fRectRight - fRectLeft) - (float)NumChars*fGlyphSizeX) *0.5f;
        float fcentery = ((fRectTop - fRectBottom) - (float)1*fGlyphSizeY) *0.5f;
        fRectLeft += fcenterx ;    
        fRectTop -= fcentery;
    }
    float fOriginalLeft = fRectLeft;
    float fTexTop = 0.0f;
    float fTexBottom = 1.0f;

    float fDepth = 0.5f;
    for( int i=0; i<NumChars; i++ )
    {
        if( strText[i] == '\n' )
        {
            fRectLeft = fOriginalLeft;
            fRectTop -= fGlyphSizeY;

            continue;
        }
        else if( strText[i] < 32 || strText[i] > 126 )
        {
            continue;
        }

        // Add 6 sprite vertices
        DXUTSpriteVertex SpriteVertex;
        float fRectRight = fRectLeft + fGlyphSizeX;
        float fRectBottom = fRectTop - fGlyphSizeY;
        float fTexLeft = ( strText[i] - 32 ) * fCharTexSizeX;
        float fTexRight = fTexLeft + fCharTexSizeX;

        // tri1
        SpriteVertex.vPos = XMFLOAT3( fRectLeft, fRectTop, fDepth );
        SpriteVertex.vTex = XMFLOAT2( fTexLeft, fTexTop );
        SpriteVertex.vColor = vFontColor;
        g_FontVertices.push_back( SpriteVertex );

        SpriteVertex.vPos = XMFLOAT3( fRectRight, fRectTop, fDepth );
        SpriteVertex.vTex = XMFLOAT2( fTexRight, fTexTop );
        SpriteVertex.vColor = vFontColor;
        g_FontVertices.push_back( SpriteVertex );

        SpriteVertex.vPos = XMFLOAT3( fRectLeft, fRectBottom, fDepth );
        SpriteVertex.vTex = XMFLOAT2( fTexLeft, fTexBottom );
        SpriteVertex.vColor = vFontColor;
        g_FontVertices.push_back( SpriteVertex );

        // tri2
        SpriteVertex.vPos = XMFLOAT3( fRectRight, fRectTop, fDepth );
        SpriteVertex.vTex = XMFLOAT2( fTexRight, fTexTop );
        SpriteVertex.vColor = vFontColor;
        g_FontVertices.push_back( SpriteVertex );

        SpriteVertex.vPos = XMFLOAT3( fRectRight, fRectBottom, fDepth );
        SpriteVertex.vTex = XMFLOAT2( fTexRight, fTexBottom );
        SpriteVertex.vColor = vFontColor;
        g_FontVertices.push_back( SpriteVertex );

        SpriteVertex.vPos = XMFLOAT3( fRectLeft, fRectBottom, fDepth );
        SpriteVertex.vTex = XMFLOAT2( fTexLeft, fTexBottom );
        SpriteVertex.vColor = vFontColor;
        g_FontVertices.push_back( SpriteVertex );

        fRectLeft += fGlyphSizeX;

    }

    // We have to end text after every line so that rendering order between sprites and fonts is preserved
    EndText11( pd3dDevice, pd3d11DeviceContext );
}

_Use_decl_annotations_
void EndText11( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3d11DeviceContext )
{
    if ( g_FontVertices.empty() )
        return;

    // ensure our buffer size can hold our sprites
    UINT FontDataBytes = static_cast<UINT>( g_FontVertices.size() * sizeof( DXUTSpriteVertex ) );
    if( g_FontBufferBytes11 < FontDataBytes )
    {
        SAFE_RELEASE( g_pFontBuffer11 );
        g_FontBufferBytes11 = FontDataBytes;

        D3D11_BUFFER_DESC BufferDesc;
        BufferDesc.ByteWidth = g_FontBufferBytes11;
        BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        BufferDesc.MiscFlags = 0;

        if (FAILED(pd3dDevice->CreateBuffer(&BufferDesc, nullptr, &g_pFontBuffer11)))
        {
            g_pFontBuffer11 = nullptr;
            g_FontBufferBytes11 = 0;
            return;
        }
        DXUT_SetDebugName( g_pFontBuffer11, "DXUT Text11" );
    }

    // Copy the sprites over
    D3D11_BOX destRegion;
    destRegion.left = 0;
    destRegion.right = FontDataBytes;
    destRegion.top = 0;
    destRegion.bottom = 1;
    destRegion.front = 0;
    destRegion.back = 1;
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    if ( S_OK == pd3d11DeviceContext->Map( g_pFontBuffer11, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) )
    { 
        memcpy( MappedResource.pData, (void*)&g_FontVertices[0], FontDataBytes );
        pd3d11DeviceContext->Unmap(g_pFontBuffer11, 0);
    }

    ID3D11ShaderResourceView* pOldTexture = nullptr;
    pd3d11DeviceContext->PSGetShaderResources( 0, 1, &pOldTexture );
    pd3d11DeviceContext->PSSetShaderResources( 0, 1, &g_pFont11 );

    // Draw
    UINT Stride = sizeof( DXUTSpriteVertex );
    UINT Offset = 0;
    pd3d11DeviceContext->IASetVertexBuffers( 0, 1, &g_pFontBuffer11, &Stride, &Offset );
    pd3d11DeviceContext->IASetInputLayout( g_pInputLayout11 );
    pd3d11DeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    pd3d11DeviceContext->Draw( static_cast<UINT>( g_FontVertices.size() ), 0 );

    pd3d11DeviceContext->PSSetShaderResources( 0, 1, &pOldTexture );
    SAFE_RELEASE( pOldTexture );

    g_FontVertices.clear();
}


//======================================================================================
// CDXUTDialog class
//======================================================================================

CDXUTDialog::CDXUTDialog() :
    m_x( 0 ),
    m_y( 0 ),
    m_width( 0 ),
    m_height( 0 ),
    m_pManager( nullptr ),
    m_bVisible( true ),
    m_bCaption( false ),
    m_bMinimized( false ),
    m_bDrag( false ),
    m_nCaptionHeight( 18 ),
    m_colorTopLeft( 0 ),
    m_colorTopRight( 0 ),
    m_colorBottomLeft( 0 ),
    m_colorBottomRight( 0 ),
    m_pCallbackEvent( nullptr ),
    m_pCallbackEventUserContext( nullptr ),
    m_fTimeLastRefresh( 0 ),
    m_pControlMouseOver( nullptr ),
    m_nDefaultControlID( 0xffff ),
    m_bNonUserEvents( false ),
    m_bKeyboardInput( false ),
    m_bMouseInput( true )
{
    m_wszCaption[0] = L'\0';

    m_pNextDialog = this;
    m_pPrevDialog = this;
}


//--------------------------------------------------------------------------------------
CDXUTDialog::~CDXUTDialog()
{
    RemoveAllControls();

    m_Fonts.clear();
    m_Textures.clear();

    for( auto it = m_DefaultElements.begin(); it != m_DefaultElements.end(); ++it )
    {
        SAFE_DELETE( *it );
    }

    m_DefaultElements.clear();
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTDialog::Init( CDXUTDialogResourceManager* pManager, bool bRegisterDialog )
{
    m_pManager = pManager;
    if( bRegisterDialog )
        pManager->RegisterDialog( this );

    SetTexture( 0, MAKEINTRESOURCE( 0xFFFF ), ( HMODULE )0xFFFF );
    InitDefaultElements();
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTDialog::Init( CDXUTDialogResourceManager* pManager, bool bRegisterDialog, LPCWSTR pszControlTextureFilename )
{
    m_pManager = pManager;
    if( bRegisterDialog )
        pManager->RegisterDialog( this );
    SetTexture( 0, pszControlTextureFilename );
    InitDefaultElements();
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTDialog::Init( CDXUTDialogResourceManager* pManager, bool bRegisterDialog,
                        LPCWSTR szControlTextureResourceName, HMODULE hControlTextureResourceModule )
{
    m_pManager = pManager;
    if( bRegisterDialog )
        pManager->RegisterDialog( this );

    SetTexture( 0, szControlTextureResourceName, hControlTextureResourceModule );
    InitDefaultElements();
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTDialog::SetCallback( PCALLBACKDXUTGUIEVENT pCallback, void* pUserContext )
{
    // If this assert triggers, you need to call CDXUTDialog::Init() first.  This change
    // was made so that the DXUT's GUI could become seperate and optional from DXUT's core.  The 
    // creation and interfacing with CDXUTDialogResourceManager is now the responsibility 
    // of the application if it wishes to use DXUT's GUI.
    assert( m_pManager && L"To fix call CDXUTDialog::Init() first.  See comments for details." );

    m_pCallbackEvent = pCallback;
    m_pCallbackEventUserContext = pUserContext;
}


//--------------------------------------------------------------------------------------
void CDXUTDialog::RemoveControl( _In_ int ID )
{
    for( auto it = m_Controls.begin(); it != m_Controls.end(); ++it )
    {
        if( (*it)->GetID() == ID )
        {
            // Clean focus first
            ClearFocus();

            // Clear references to this control
            if( s_pControlFocus == (*it) )
                s_pControlFocus = nullptr;
            if( s_pControlPressed == (*it) )
                s_pControlPressed = nullptr;
            if( m_pControlMouseOver == (*it) )
                m_pControlMouseOver = nullptr;

            SAFE_DELETE( (*it) );
            m_Controls.erase( it );

            return;
        }
    }
}


//--------------------------------------------------------------------------------------
void CDXUTDialog::RemoveAllControls()
{
    if( s_pControlFocus && s_pControlFocus->m_pDialog == this )
        s_pControlFocus = nullptr;
    if( s_pControlPressed && s_pControlPressed->m_pDialog == this )
        s_pControlPressed = nullptr;
    m_pControlMouseOver = nullptr;

    for( auto it = m_Controls.begin(); it != m_Controls.end(); ++it )
    {
        SAFE_DELETE( *it );
    }

    m_Controls.clear();
}


//--------------------------------------------------------------------------------------
void CDXUTDialog::Refresh()
{
    if( s_pControlFocus )
        s_pControlFocus->OnFocusOut();

    if( m_pControlMouseOver )
        m_pControlMouseOver->OnMouseLeave();

    s_pControlFocus = nullptr;
    s_pControlPressed = nullptr;
    m_pControlMouseOver = nullptr;

    for( auto it = m_Controls.begin(); it != m_Controls.end(); ++it )
    {
        (*it)->Refresh();
    }

    if( m_bKeyboardInput )
        FocusDefaultControl();
}


//--------------------------------------------------------------------------------------
HRESULT CDXUTDialog::OnRender( _In_ float fElapsedTime )
{
    // If this assert triggers, you need to call CDXUTDialogResourceManager::On*Device() from inside
    // the application's device callbacks.  See the SDK samples for an example of how to do this.
    assert( m_pManager->GetD3D11Device() &&
            L"To fix hook up CDXUTDialogResourceManager to device callbacks.  See comments for details" );

    // See if the dialog needs to be refreshed
    if( m_fTimeLastRefresh < s_fTimeRefresh )
    {
        m_fTimeLastRefresh = DXUTGetTime();
        Refresh();
    }

    // For invisible dialog, out now.
    if( !m_bVisible ||
        ( m_bMinimized && !m_bCaption ) )
        return S_OK;

    auto pd3dDevice = m_pManager->GetD3D11Device();
    auto pd3dDeviceContext = m_pManager->GetD3D11DeviceContext();

    // Set up a state block here and restore it when finished drawing all the controls
    m_pManager->StoreD3D11State( pd3dDeviceContext );

    BOOL bBackgroundIsVisible = ( m_colorTopLeft | m_colorTopRight | m_colorBottomRight | m_colorBottomLeft ) & 0xff000000;
    if( !m_bMinimized && bBackgroundIsVisible )
    {
        // Convert the draw rectangle from screen coordinates to clip space coordinates.
        float Left, Right, Top, Bottom;
        Left = m_x * 2.0f / m_pManager->m_nBackBufferWidth - 1.0f;
        Right = ( m_x + m_width ) * 2.0f / m_pManager->m_nBackBufferWidth - 1.0f;
        Top = 1.0f - m_y * 2.0f / m_pManager->m_nBackBufferHeight;
        Bottom = 1.0f - ( m_y + m_height ) * 2.0f / m_pManager->m_nBackBufferHeight;

        DXUT_SCREEN_VERTEX_10 vertices[4] =
        {
            Left,  Top,    0.5f, D3DCOLOR_TO_D3DCOLORVALUE( m_colorTopLeft ), 0.0f, 0.0f,
            Right, Top,    0.5f, D3DCOLOR_TO_D3DCOLORVALUE( m_colorTopRight ), 1.0f, 0.0f,
            Left,  Bottom, 0.5f, D3DCOLOR_TO_D3DCOLORVALUE( m_colorBottomLeft ), 0.0f, 1.0f,
            Right, Bottom, 0.5f, D3DCOLOR_TO_D3DCOLORVALUE( m_colorBottomRight ), 1.0f, 1.0f,
        };

        //DXUT_SCREEN_VERTEX_10 *pVB;
        D3D11_MAPPED_SUBRESOURCE MappedData;
        if( SUCCEEDED( pd3dDeviceContext->Map( m_pManager->m_pVBScreenQuad11, 0, D3D11_MAP_WRITE_DISCARD,
                                               0, &MappedData ) ) )
        {
            memcpy( MappedData.pData, vertices, sizeof( vertices ) );
            pd3dDeviceContext->Unmap( m_pManager->m_pVBScreenQuad11, 0 );
        }

        // Set the quad VB as current
        UINT stride = sizeof( DXUT_SCREEN_VERTEX_10 );
        UINT offset = 0;
        pd3dDeviceContext->IASetVertexBuffers( 0, 1, &m_pManager->m_pVBScreenQuad11, &stride, &offset );
        pd3dDeviceContext->IASetInputLayout( m_pManager->m_pInputLayout11 );
        pd3dDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

        // Setup for rendering
        m_pManager->ApplyRenderUIUntex11( pd3dDeviceContext );
        pd3dDeviceContext->Draw( 4, 0 );
    }

    auto pTextureNode = GetTexture( 0 );
    pd3dDeviceContext->PSSetShaderResources( 0, 1, &pTextureNode->pTexResView11 );

    // Sort depth back to front
    m_pManager->BeginSprites11();
    BeginText11();

    m_pManager->ApplyRenderUI11( pd3dDeviceContext );

    // Render the caption if it's enabled.
    if( m_bCaption )
    {
        // DrawSprite will offset the rect down by
        // m_nCaptionHeight, so adjust the rect higher
        // here to negate the effect.
        RECT rc = { 0, -m_nCaptionHeight, m_width, 0 };
        DrawSprite( &m_CapElement, &rc, 0.99f );
        rc.left += 5; // Make a left margin
        WCHAR wszOutput[256];
        wcscpy_s( wszOutput, 256, m_wszCaption );
        if( m_bMinimized )
            wcscat_s( wszOutput, 256, L" (Minimized)" );
        DrawText( wszOutput, &m_CapElement, &rc, true );
    }

    // If the dialog is minimized, skip rendering
    // its controls.
    if( !m_bMinimized )
    {
        for( auto it = m_Controls.cbegin(); it != m_Controls.cend(); ++it )
        {
            // Focused control is drawn last
            if( *it == s_pControlFocus )
                continue;

            (*it)->Render( fElapsedTime );
        }

        if( s_pControlFocus && s_pControlFocus->m_pDialog == this )
            s_pControlFocus->Render( fElapsedTime );
    }

    // End sprites
    if( m_bCaption )
    {
        m_pManager->EndSprites11( pd3dDevice, pd3dDeviceContext );
        EndText11( pd3dDevice, pd3dDeviceContext );
    }
    m_pManager->RestoreD3D11State( pd3dDeviceContext );

    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
VOID CDXUTDialog::SendEvent( UINT nEvent, bool bTriggeredByUser, CDXUTControl* pControl )
{
    // If no callback has been registered there's nowhere to send the event to
    if( !m_pCallbackEvent )
        return;

    // Discard events triggered programatically if these types of events haven't been
    // enabled
    if( !bTriggeredByUser && !m_bNonUserEvents )
        return;

    m_pCallbackEvent( nEvent, pControl->GetID(), pControl, m_pCallbackEventUserContext );
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTDialog::SetFont( UINT index, LPCWSTR strFaceName, LONG height, LONG weight )
{
    // If this assert triggers, you need to call CDXUTDialog::Init() first.  This change
    // was made so that the DXUT's GUI could become seperate and optional from DXUT's core.  The 
    // creation and interfacing with CDXUTDialogResourceManager is now the responsibility 
    // of the application if it wishes to use DXUT's GUI.
    assert( m_pManager && L"To fix call CDXUTDialog::Init() first.  See comments for details." );
    _Analysis_assume_( m_pManager );

    // Make sure the list is at least as large as the index being set
    for( size_t i = m_Fonts.size(); i <= index; i++ )
    {
        m_Fonts.push_back( -1 );
    }

    int iFont = m_pManager->AddFont( strFaceName, height, weight );
    m_Fonts[ index ] = iFont;

    return S_OK;
}


//--------------------------------------------------------------------------------------
DXUTFontNode* CDXUTDialog::GetFont( _In_ UINT index ) const
{
    if( !m_pManager )
        return nullptr;
    return m_pManager->GetFontNode( m_Fonts[ index ] );
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTDialog::SetTexture( UINT index, LPCWSTR strFilename )
{
    // If this assert triggers, you need to call CDXUTDialog::Init() first.  This change
    // was made so that the DXUT's GUI could become seperate and optional from DXUT's core.  The 
    // creation and interfacing with CDXUTDialogResourceManager is now the responsibility 
    // of the application if it wishes to use DXUT's GUI.
    assert( m_pManager && L"To fix this, call CDXUTDialog::Init() first.  See comments for details." );
    _Analysis_assume_( m_pManager );

    // Make sure the list is at least as large as the index being set
    for( size_t i = m_Textures.size(); i <= index; i++ )
    {
        m_Textures.push_back( -1 );
    }

    int iTexture = m_pManager->AddTexture( strFilename );

    m_Textures[ index] = iTexture;
    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTDialog::SetTexture( UINT index, LPCWSTR strResourceName, HMODULE hResourceModule )
{
    // If this assert triggers, you need to call CDXUTDialog::Init() first.  This change
    // was made so that the DXUT's GUI could become seperate and optional from DXUT's core.  The 
    // creation and interfacing with CDXUTDialogResourceManager is now the responsibility 
    // of the application if it wishes to use DXUT's GUI.
    assert( m_pManager && L"To fix this, call CDXUTDialog::Init() first.  See comments for details." );
    _Analysis_assume_( m_pManager );

    // Make sure the list is at least as large as the index being set
    for( size_t i = m_Textures.size(); i <= index; i++ )
    {
        m_Textures.push_back( -1 );
    }

    int iTexture = m_pManager->AddTexture( strResourceName, hResourceModule );

    m_Textures[ index ] = iTexture;
    return S_OK;
}


//--------------------------------------------------------------------------------------
DXUTTextureNode* CDXUTDialog::GetTexture( _In_ UINT index ) const
{
    if( !m_pManager )
        return nullptr;
    return m_pManager->GetTextureNode( m_Textures[ index ] );
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTDialog::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    bool bHandled = false;

    // For invisible dialog, do not handle anything.
    if( !m_bVisible )
        return false;

    // If automation command-line switch is on, enable this dialog's keyboard input
    // upon any key press or mouse click.
    if( DXUTGetAutomation() &&
        ( WM_LBUTTONDOWN == uMsg || WM_LBUTTONDBLCLK == uMsg || WM_KEYDOWN == uMsg ) )
    {
        m_pManager->EnableKeyboardInputForAllDialogs();
    }

    // If caption is enable, check for clicks in the caption area.
    if( m_bCaption )
    {
        if( uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONDBLCLK )
        {
            POINT mousePoint =
            {
                short( LOWORD( lParam ) ), short( HIWORD( lParam ) )
            };

            if( mousePoint.x >= m_x && mousePoint.x < m_x + m_width &&
                mousePoint.y >= m_y && mousePoint.y < m_y + m_nCaptionHeight )
            {
                m_bDrag = true;
                SetCapture( DXUTGetHWND() );
                return true;
            }
        }
        else if( uMsg == WM_LBUTTONUP && m_bDrag )
        {
            POINT mousePoint =
            {
                short( LOWORD( lParam ) ), short( HIWORD( lParam ) )
            };

            if( mousePoint.x >= m_x && mousePoint.x < m_x + m_width &&
                mousePoint.y >= m_y && mousePoint.y < m_y + m_nCaptionHeight )
            {
                ReleaseCapture();
                m_bDrag = false;
                m_bMinimized = !m_bMinimized;
                return true;
            }
        }
    }

    // If the dialog is minimized, don't send any messages to controls.
    if( m_bMinimized )
        return false;

    // If a control is in focus, it belongs to this dialog, and it's enabled, then give
    // it the first chance at handling the message.
    if( s_pControlFocus &&
        s_pControlFocus->m_pDialog == this &&
        s_pControlFocus->GetEnabled() )
    {
        // If the control MsgProc handles it, then we don't.
        if( s_pControlFocus->MsgProc( uMsg, wParam, lParam ) )
            return true;
    }

    switch( uMsg )
    {
        case WM_SIZE:
        case WM_MOVE:
            {
                // Handle sizing and moving messages so that in case the mouse cursor is moved out
                // of an UI control because of the window adjustment, we can properly
                // unhighlight the highlighted control.
                POINT pt =
                {
                    -1, -1
                };
                OnMouseMove( pt );
                break;
            }

        case WM_ACTIVATEAPP:
            // Call OnFocusIn()/OnFocusOut() of the control that currently has the focus
            // as the application is activated/deactivated.  This matches the Windows
            // behavior.
            if( s_pControlFocus &&
                s_pControlFocus->m_pDialog == this &&
                s_pControlFocus->GetEnabled() )
            {
                if( wParam )
                    s_pControlFocus->OnFocusIn();
                else
                    s_pControlFocus->OnFocusOut();
            }
            break;

            // Keyboard messages
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
            {
                // If a control is in focus, it belongs to this dialog, and it's enabled, then give
                // it the first chance at handling the message.
                if( s_pControlFocus &&
                    s_pControlFocus->m_pDialog == this &&
                    s_pControlFocus->GetEnabled() )
    for( auto it = m_Controls.cbegin(); it != m_Controls.cend(); ++it )
                {
                    if( s_pControlFocus->HandleKeyboard( uMsg, wParam, lParam ) )
                        return true;
                }

                // Not yet handled, see if this matches a control's hotkey
                // Activate the hotkey if the focus doesn't belong to an
                // edit box.
                if( uMsg == WM_KEYDOWN && ( !s_pControlFocus ||
                                            ( s_pControlFocus->GetType() != DXUT_CONTROL_EDITBOX
                                              && s_pControlFocus->GetType() != DXUT_CONTROL_IMEEDITBOX ) ) )
                {
                    for( auto it = m_Controls.begin(); it != m_Controls.end(); ++it )
                    {
                        if( (*it)->GetHotkey() == wParam )
                        {
                            (*it)->OnHotkey();
                            return true;
                        }
                    }
                }

                // Not yet handled, check for focus messages
                if( uMsg == WM_KEYDOWN )
                {
                    // If keyboard input is not enabled, this message should be ignored
                    if( !m_bKeyboardInput )
                        return false;

                    switch( wParam )
                    {
                        case VK_RIGHT:
                        case VK_DOWN:
                            if( s_pControlFocus )
                            {
                                return OnCycleFocus( true );
                            }
                            break;

                        case VK_LEFT:
                        case VK_UP:
                            if( s_pControlFocus )
                            {
                                return OnCycleFocus( false );
                            }
                            break;

                        case VK_TAB:
                        {
                            bool bShiftDown = ( ( GetKeyState( VK_SHIFT ) & 0x8000 ) != 0 );
                            return OnCycleFocus( !bShiftDown );
                        }
                    }
                }

                break;
            }


            // Mouse messages
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_XBUTTONDBLCLK:
        case WM_MOUSEWHEEL:
            {
                // If not accepting mouse input, return false to indicate the message should still 
                // be handled by the application (usually to move the camera).
                if( !m_bMouseInput )
                    return false;

                POINT mousePoint =
                {
                    short( LOWORD( lParam ) ), short( HIWORD( lParam ) )
                };
                mousePoint.x -= m_x;
                mousePoint.y -= m_y;

                // If caption is enabled, offset the Y coordinate by the negative of its height.
                if( m_bCaption )
                    mousePoint.y -= m_nCaptionHeight;

                // If a control is in focus, it belongs to this dialog, and it's enabled, then give
                // it the first chance at handling the message.
                if( s_pControlFocus &&
                    s_pControlFocus->m_pDialog == this &&
                    s_pControlFocus->GetEnabled() )
                {
                    if( s_pControlFocus->HandleMouse( uMsg, mousePoint, wParam, lParam ) )
                        return true;
                }

                // Not yet handled, see if the mouse is over any controls
                auto pControl = GetControlAtPoint( mousePoint );
                if( pControl && pControl->GetEnabled() )
                {
                    bHandled = pControl->HandleMouse( uMsg, mousePoint, wParam, lParam );
                    if( bHandled )
                        return true;
                }
                else
                {
                    // Mouse not over any controls in this dialog, if there was a control
                    // which had focus it just lost it
                    if( uMsg == WM_LBUTTONDOWN &&
                        s_pControlFocus &&
                        s_pControlFocus->m_pDialog == this )
                    {
                        s_pControlFocus->OnFocusOut();
                        s_pControlFocus = nullptr;
                    }
                }

                // Still not handled, hand this off to the dialog. Return false to indicate the
                // message should still be handled by the application (usually to move the camera).
                switch( uMsg )
                {
                    case WM_MOUSEMOVE:
                        OnMouseMove( mousePoint );
                        return false;
                }

                break;
            }

        case WM_CAPTURECHANGED:
        {
            // The application has lost mouse capture.
            // The dialog object may not have received
            // a WM_MOUSEUP when capture changed. Reset
            // m_bDrag so that the dialog does not mistakenly
            // think the mouse button is still held down.
            if( ( HWND )lParam != hWnd )
                m_bDrag = false;
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------
CDXUTControl* CDXUTDialog::GetControlAtPoint( _In_ const POINT& pt ) const
{
    // Search through all child controls for the first one which
    // contains the mouse point
    for( auto it = m_Controls.cbegin(); it != m_Controls.cend(); ++it )
    {
        if( !*it )
        {
            continue;
        }

        // We only return the current control if it is visible
        // and enabled.  Because GetControlAtPoint() is used to do mouse
        // hittest, it makes sense to perform this filtering.
        if( (*it)->ContainsPoint( pt ) && (*it)->GetEnabled() && (*it)->GetVisible() )
        {
            return *it;
        }
    }

    return nullptr;
}


//--------------------------------------------------------------------------------------
bool CDXUTDialog::GetControlEnabled( _In_ int ID ) const
{
    auto pControl = GetControl( ID );
    if( !pControl )
        return false;

    return pControl->GetEnabled();
}



//--------------------------------------------------------------------------------------
void CDXUTDialog::SetControlEnabled( _In_ int ID, _In_ bool bEnabled )
{
    auto pControl = GetControl( ID );
    if( !pControl )
        return;

    pControl->SetEnabled( bEnabled );
}


//--------------------------------------------------------------------------------------
void CDXUTDialog::OnMouseUp( _In_ const POINT& pt )
{
    UNREFERENCED_PARAMETER(pt);
    s_pControlPressed = nullptr;
    m_pControlMouseOver = nullptr;
}


//--------------------------------------------------------------------------------------
void CDXUTDialog::OnMouseMove( _In_ const POINT& pt )
{
    // Figure out which control the mouse is over now
    auto pControl = GetControlAtPoint( pt );

    // If the mouse is still over the same control, nothing needs to be done
    if( pControl == m_pControlMouseOver )
        return;

    // Handle mouse leaving the old control
    if( m_pControlMouseOver )
        m_pControlMouseOver->OnMouseLeave();

    // Handle mouse entering the new control
    m_pControlMouseOver = pControl;
    if( pControl )
        m_pControlMouseOver->OnMouseEnter();
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTDialog::SetDefaultElement( UINT nControlType, UINT iElement, CDXUTElement* pElement )
{
    // If this Element type already exist in the list, simply update the stored Element
    for( auto it = m_DefaultElements.begin(); it != m_DefaultElements.end(); ++it )
    {
        if( (*it)->nControlType == nControlType &&
            (*it)->iElement == iElement )
        {
            (*it)->Element = *pElement;
            return S_OK;
        }
    }

    // Otherwise, add a new entry
    DXUTElementHolder* pNewHolder;
    pNewHolder = new (std::nothrow) DXUTElementHolder;
    if( !pNewHolder )
        return E_OUTOFMEMORY;

    pNewHolder->nControlType = nControlType;
    pNewHolder->iElement = iElement;
    pNewHolder->Element = *pElement;

    m_DefaultElements.push_back( pNewHolder );

    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
CDXUTElement* CDXUTDialog::GetDefaultElement( UINT nControlType, UINT iElement ) const
{
    for( auto it = m_DefaultElements.cbegin(); it != m_DefaultElements.cend(); ++it )
    {
        if( (*it)->nControlType == nControlType &&
            (*it)->iElement == iElement )
        {
            return &(*it)->Element;
        }
    }

    return nullptr;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTDialog::AddStatic( int ID, LPCWSTR strText, int x, int y, int width, int height, bool bIsDefault,
                                CDXUTStatic** ppCreated )
{
    HRESULT hr = S_OK;

    auto pStatic = new (std::nothrow) CDXUTStatic( this );

    if( ppCreated )
        *ppCreated = pStatic;

    if( !pStatic )
        return E_OUTOFMEMORY;

    hr = AddControl( pStatic );
    if( FAILED( hr ) )
        return hr;

    // Set the ID and list index
    pStatic->SetID( ID );
    pStatic->SetText( strText );
    pStatic->SetLocation( x, y );
    pStatic->SetSize( width, height );
    pStatic->m_bIsDefault = bIsDefault;

    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTDialog::AddButton( int ID, LPCWSTR strText, int x, int y, int width, int height, UINT nHotkey,
                                bool bIsDefault, CDXUTButton** ppCreated )
{
    HRESULT hr = S_OK;

    auto pButton = new (std::nothrow) CDXUTButton( this );

    if( ppCreated )
        *ppCreated = pButton;

    if( !pButton )
        return E_OUTOFMEMORY;

    hr = AddControl( pButton );
    if( FAILED( hr ) )
        return hr;

    // Set the ID and list index
    pButton->SetID( ID );
    pButton->SetText( strText );
    pButton->SetLocation( x, y );
    pButton->SetSize( width, height );
    pButton->SetHotkey( nHotkey );
    pButton->m_bIsDefault = bIsDefault;

    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTDialog::AddCheckBox( int ID, LPCWSTR strText, int x, int y, int width, int height, bool bChecked,
                                  UINT nHotkey, bool bIsDefault, CDXUTCheckBox** ppCreated )
{
    HRESULT hr = S_OK;

    auto pCheckBox = new (std::nothrow) CDXUTCheckBox( this );

    if( ppCreated )
        *ppCreated = pCheckBox;

    if( !pCheckBox )
        return E_OUTOFMEMORY;

    hr = AddControl( pCheckBox );
    if( FAILED( hr ) )
        return hr;

    // Set the ID and list index
    pCheckBox->SetID( ID );
    pCheckBox->SetText( strText );
    pCheckBox->SetLocation( x, y );
    pCheckBox->SetSize( width, height );
    pCheckBox->SetHotkey( nHotkey );
    pCheckBox->m_bIsDefault = bIsDefault;
    pCheckBox->SetChecked( bChecked );

    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTDialog::AddRadioButton( int ID, UINT nButtonGroup, LPCWSTR strText, int x, int y, int width, int height,
                                     bool bChecked, UINT nHotkey, bool bIsDefault, CDXUTRadioButton** ppCreated )
{
    HRESULT hr = S_OK;

    auto pRadioButton = new (std::nothrow) CDXUTRadioButton( this );

    if( ppCreated )
        *ppCreated = pRadioButton;

    if( !pRadioButton )
        return E_OUTOFMEMORY;

    hr = AddControl( pRadioButton );
    if( FAILED( hr ) )
        return hr;

    // Set the ID and list index
    pRadioButton->SetID( ID );
    pRadioButton->SetText( strText );
    pRadioButton->SetButtonGroup( nButtonGroup );
    pRadioButton->SetLocation( x, y );
    pRadioButton->SetSize( width, height );
    pRadioButton->SetHotkey( nHotkey );
    pRadioButton->SetChecked( bChecked );
    pRadioButton->m_bIsDefault = bIsDefault;
    pRadioButton->SetChecked( bChecked );

    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTDialog::AddComboBox( int ID, int x, int y, int width, int height, UINT nHotkey, bool bIsDefault,
                                  CDXUTComboBox** ppCreated )
{
    HRESULT hr = S_OK;

    auto pComboBox = new (std::nothrow) CDXUTComboBox( this );

    if( ppCreated )
        *ppCreated = pComboBox;

    if( !pComboBox )
        return E_OUTOFMEMORY;

    hr = AddControl( pComboBox );
    if( FAILED( hr ) )
        return hr;

    // Set the ID and list index
    pComboBox->SetID( ID );
    pComboBox->SetLocation( x, y );
    pComboBox->SetSize( width, height );
    pComboBox->SetHotkey( nHotkey );
    pComboBox->m_bIsDefault = bIsDefault;

    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTDialog::AddSlider( int ID, int x, int y, int width, int height, int min, int max, int value,
                                bool bIsDefault, CDXUTSlider** ppCreated )
{
    HRESULT hr = S_OK;

    auto pSlider = new (std::nothrow) CDXUTSlider( this );

    if( ppCreated )
        *ppCreated = pSlider;

    if( !pSlider )
        return E_OUTOFMEMORY;

    hr = AddControl( pSlider );
    if( FAILED( hr ) )
        return hr;

    // Set the ID and list index
    pSlider->SetID( ID );
    pSlider->SetLocation( x, y );
    pSlider->SetSize( width, height );
    pSlider->m_bIsDefault = bIsDefault;
    pSlider->SetRange( min, max );
    pSlider->SetValue( value );
    pSlider->UpdateRects();

    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTDialog::AddEditBox( int ID, LPCWSTR strText, int x, int y, int width, int height, bool bIsDefault,
                                 CDXUTEditBox** ppCreated )
{
    HRESULT hr = S_OK;

    auto pEditBox = new (std::nothrow) CDXUTEditBox( this );

    if( ppCreated )
        *ppCreated = pEditBox;

    if( !pEditBox )
        return E_OUTOFMEMORY;

    hr = AddControl( pEditBox );
    if( FAILED( hr ) )
        return hr;

    // Set the ID and position
    pEditBox->SetID( ID );
    pEditBox->SetLocation( x, y );
    pEditBox->SetSize( width, height );
    pEditBox->m_bIsDefault = bIsDefault;

    if( strText )
        pEditBox->SetText( strText );

    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTDialog::AddListBox( int ID, int x, int y, int width, int height, DWORD dwStyle, CDXUTListBox** ppCreated )
{
    HRESULT hr = S_OK;
    auto pListBox = new (std::nothrow) CDXUTListBox( this );

    if( ppCreated )
        *ppCreated = pListBox;

    if( !pListBox )
        return E_OUTOFMEMORY;

    hr = AddControl( pListBox );
    if( FAILED( hr ) )
        return hr;

    // Set the ID and position
    pListBox->SetID( ID );
    pListBox->SetLocation( x, y );
    pListBox->SetSize( width, height );
    pListBox->SetStyle( dwStyle );

    return S_OK;
}


//--------------------------------------------------------------------------------------
HRESULT CDXUTDialog::InitControl( _In_ CDXUTControl* pControl )
{
    HRESULT hr;

    if( !pControl )
        return E_INVALIDARG;

    pControl->m_Index = static_cast<UINT>( m_Controls.size() );

    // Look for a default Element entries
    for( auto it = m_DefaultElements.begin(); it != m_DefaultElements.end(); ++it )
    {
        if( (*it)->nControlType == pControl->GetType() )
            pControl->SetElement( (*it)->iElement, &(*it)->Element );
    }

    V_RETURN( pControl->OnInit() );

    return S_OK;
}


//--------------------------------------------------------------------------------------
HRESULT CDXUTDialog::AddControl( _In_ CDXUTControl* pControl )
{
    HRESULT hr = S_OK;

    hr = InitControl( pControl );
    if( FAILED( hr ) )
        return DXTRACE_ERR( L"CDXUTDialog::InitControl", hr );

    // Add to the list
    m_Controls.push_back( pControl );

    return S_OK;
}


//--------------------------------------------------------------------------------------
CDXUTControl* CDXUTDialog::GetControl( _In_ int ID ) const
{
    // Try to find the control with the given ID
    for( auto it = m_Controls.cbegin(); it != m_Controls.cend(); ++it )
    {
        if( (*it)->GetID() == ID )
        {
            return *it;
        }
    }

    // Not found
    return nullptr;
}


//--------------------------------------------------------------------------------------
CDXUTControl* CDXUTDialog::GetControl( _In_  int ID, _In_  UINT nControlType ) const
{
    // Try to find the control with the given ID
    for( auto it = m_Controls.cbegin(); it != m_Controls.cend(); ++it )
    {
        if( (*it)->GetID() == ID && (*it)->GetType() == nControlType )
        {
            return *it;
        }
    }

    // Not found
    return nullptr;
}


//--------------------------------------------------------------------------------------
CDXUTControl* CDXUTDialog::GetNextControl( _In_ CDXUTControl* pControl )
{
    int index = pControl->m_Index + 1;

    auto pDialog = pControl->m_pDialog;

    // Cycle through dialogs in the loop to find the next control. Note
    // that if only one control exists in all looped dialogs it will
    // be the returned 'next' control.
    while( index >= ( int )pDialog->m_Controls.size() )
    {
        pDialog = pDialog->m_pNextDialog;
        index = 0;
    }

    return pDialog->m_Controls[ index ];
}


//--------------------------------------------------------------------------------------
CDXUTControl* CDXUTDialog::GetPrevControl( _In_ CDXUTControl* pControl )
{
    int index = pControl->m_Index - 1;

    auto pDialog = pControl->m_pDialog;

    // Cycle through dialogs in the loop to find the next control. Note
    // that if only one control exists in all looped dialogs it will
    // be the returned 'previous' control.
    while( index < 0 )
    {
        pDialog = pDialog->m_pPrevDialog;
        if( !pDialog )
            pDialog = pControl->m_pDialog;

        index = int( pDialog->m_Controls.size() ) - 1;
    }

    return pDialog->m_Controls[ index ];
}


//--------------------------------------------------------------------------------------
void CDXUTDialog::ClearRadioButtonGroup( _In_ UINT nButtonGroup )
{
    // Find all radio buttons with the given group number
    for( auto it = m_Controls.cbegin(); it != m_Controls.cend(); ++it )
    {
        if( (*it)->GetType() == DXUT_CONTROL_RADIOBUTTON )
        {
            auto pRadioButton = ( CDXUTRadioButton* )*it;

            if( pRadioButton->GetButtonGroup() == nButtonGroup )
                pRadioButton->SetChecked( false, false );
        }
    }
}


//--------------------------------------------------------------------------------------
void CDXUTDialog::ClearComboBox( _In_ int ID )
{
    auto pComboBox = GetComboBox( ID );
    if( !pComboBox )
        return;

    pComboBox->RemoveAllItems();
}


//--------------------------------------------------------------------------------------
void CDXUTDialog::RequestFocus( _In_ CDXUTControl* pControl )
{
    if( s_pControlFocus == pControl )
        return;

    if( !pControl->CanHaveFocus() )
        return;

    if( s_pControlFocus )
        s_pControlFocus->OnFocusOut();

    pControl->OnFocusIn();
    s_pControlFocus = pControl;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTDialog::DrawRect( const RECT* pRect, DWORD color )
{
    UNREFERENCED_PARAMETER(pRect);
    UNREFERENCED_PARAMETER(color);
    // TODO -
    return E_FAIL;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTDialog::DrawSprite( CDXUTElement* pElement, const RECT* prcDest, float fDepth )
{
    // No need to draw fully transparent layers
    if( pElement->TextureColor.Current.w == 0 )
        return S_OK;

    RECT rcTexture = pElement->rcTexture;

    RECT rcScreen = *prcDest;
    OffsetRect( &rcScreen, m_x, m_y );

    // If caption is enabled, offset the Y position by its height.
    if( m_bCaption )
        OffsetRect( &rcScreen, 0, m_nCaptionHeight );

    auto pTextureNode = GetTexture( pElement->iTexture );
    if( !pTextureNode )
        return E_FAIL;

    float fBBWidth = ( float )m_pManager->m_nBackBufferWidth;
    float fBBHeight = ( float )m_pManager->m_nBackBufferHeight;
    float fTexWidth = ( float )pTextureNode->dwWidth;
    float fTexHeight = ( float )pTextureNode->dwHeight;

    float fRectLeft = rcScreen.left / fBBWidth;
    float fRectTop = 1.0f - rcScreen.top / fBBHeight;
    float fRectRight = rcScreen.right / fBBWidth;
    float fRectBottom = 1.0f - rcScreen.bottom / fBBHeight;

    fRectLeft = fRectLeft * 2.0f - 1.0f;
    fRectTop = fRectTop * 2.0f - 1.0f;
    fRectRight = fRectRight * 2.0f - 1.0f;
    fRectBottom = fRectBottom * 2.0f - 1.0f;
    
    float fTexLeft = rcTexture.left / fTexWidth;
    float fTexTop = rcTexture.top / fTexHeight;
    float fTexRight = rcTexture.right / fTexWidth;
    float fTexBottom = rcTexture.bottom / fTexHeight;

    // Add 6 sprite vertices
    DXUTSpriteVertex SpriteVertex;

    // tri1
    SpriteVertex.vPos = XMFLOAT3( fRectLeft, fRectTop, fDepth );
    SpriteVertex.vTex = XMFLOAT2( fTexLeft, fTexTop );
    SpriteVertex.vColor = pElement->TextureColor.Current;
    m_pManager->m_SpriteVertices.push_back( SpriteVertex );

    SpriteVertex.vPos = XMFLOAT3( fRectRight, fRectTop, fDepth );
    SpriteVertex.vTex = XMFLOAT2( fTexRight, fTexTop );
    SpriteVertex.vColor = pElement->TextureColor.Current;
    m_pManager->m_SpriteVertices.push_back( SpriteVertex );

    SpriteVertex.vPos = XMFLOAT3( fRectLeft, fRectBottom, fDepth );
    SpriteVertex.vTex = XMFLOAT2( fTexLeft, fTexBottom );
    SpriteVertex.vColor = pElement->TextureColor.Current;
    m_pManager->m_SpriteVertices.push_back( SpriteVertex );

    // tri2
    SpriteVertex.vPos = XMFLOAT3( fRectRight, fRectTop, fDepth );
    SpriteVertex.vTex = XMFLOAT2( fTexRight, fTexTop );
    SpriteVertex.vColor = pElement->TextureColor.Current;
    m_pManager->m_SpriteVertices.push_back( SpriteVertex );

    SpriteVertex.vPos = XMFLOAT3( fRectRight, fRectBottom, fDepth );
    SpriteVertex.vTex = XMFLOAT2( fTexRight, fTexBottom );
    SpriteVertex.vColor = pElement->TextureColor.Current;
    m_pManager->m_SpriteVertices.push_back( SpriteVertex );

    SpriteVertex.vPos = XMFLOAT3( fRectLeft, fRectBottom, fDepth );
    SpriteVertex.vTex = XMFLOAT2( fTexLeft, fTexBottom );
    SpriteVertex.vColor = pElement->TextureColor.Current;
    m_pManager->m_SpriteVertices.push_back( SpriteVertex );

    // Why are we drawing the sprite every time?  This is very inefficient, but the sprite workaround doesn't have support for sorting now, so we have to
    // draw a sprite every time to keep the order correct between sprites and text.
    m_pManager->EndSprites11( DXUTGetD3D11Device(), DXUTGetD3D11DeviceContext() );

    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTDialog::CalcTextRect( LPCWSTR strText, CDXUTElement* pElement, const RECT* prcDest, int nCount )
{
    auto pFontNode = GetFont( pElement->iFont );
    if( !pFontNode )
        return E_FAIL;

    UNREFERENCED_PARAMETER(strText);
    UNREFERENCED_PARAMETER(prcDest);
    UNREFERENCED_PARAMETER(nCount);
    // TODO -

    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTDialog::DrawText( LPCWSTR strText, CDXUTElement* pElement, const RECT* prcDest, bool bShadow, bool bCenter   )
{
    // No need to draw fully transparent layers
    if( pElement->FontColor.Current.w == 0 )
        return S_OK;

    RECT rcScreen = *prcDest;
    OffsetRect( &rcScreen, m_x, m_y);

    // If caption is enabled, offset the Y position by its height.
    if( m_bCaption )
        OffsetRect( &rcScreen, 0, m_nCaptionHeight );

    float fBBWidth = ( float )m_pManager->m_nBackBufferWidth;
    float fBBHeight = ( float )m_pManager->m_nBackBufferHeight;

    auto pd3dDevice = m_pManager->GetD3D11Device();
    auto pd3d11DeviceContext =  m_pManager->GetD3D11DeviceContext();

    if( bShadow )
    {
        RECT rcShadow = rcScreen;
        OffsetRect( &rcShadow, 1, 1 );

        XMFLOAT4 vShadowColor( 0,0,0, 1.0f );
        DrawText11DXUT( pd3dDevice, pd3d11DeviceContext,
                 strText, rcShadow, vShadowColor,
                 fBBWidth, fBBHeight, bCenter );

    }

    XMFLOAT4 vFontColor( pElement->FontColor.Current.x, pElement->FontColor.Current.y, pElement->FontColor.Current.z, 1.0f );
    DrawText11DXUT( pd3dDevice, pd3d11DeviceContext,
             strText, rcScreen, vFontColor,
             fBBWidth, fBBHeight, bCenter );

    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTDialog::SetBackgroundColors( DWORD colorTopLeft, DWORD colorTopRight, DWORD colorBottomLeft,
                                       DWORD colorBottomRight )
{
    m_colorTopLeft = colorTopLeft;
    m_colorTopRight = colorTopRight;
    m_colorBottomLeft = colorBottomLeft;
    m_colorBottomRight = colorBottomRight;
}


//--------------------------------------------------------------------------------------
void CDXUTDialog::SetNextDialog( _In_ CDXUTDialog* pNextDialog )
{
    if( !pNextDialog )
        pNextDialog = this;

    m_pNextDialog = pNextDialog;
    if( pNextDialog )
        m_pNextDialog->m_pPrevDialog = this;
}


//--------------------------------------------------------------------------------------
void CDXUTDialog::ClearFocus()
{
    if( s_pControlFocus )
    {
        s_pControlFocus->OnFocusOut();
        s_pControlFocus = nullptr;
    }

    ReleaseCapture();
}


//--------------------------------------------------------------------------------------
void CDXUTDialog::FocusDefaultControl()
{
    // Check for default control in this dialog
    for( auto it = m_Controls.cbegin(); it != m_Controls.cend(); ++it )
    {
        if( (*it)->m_bIsDefault )
        {
            // Remove focus from the current control
            ClearFocus();

            // Give focus to the default control
            s_pControlFocus = *it;
            s_pControlFocus->OnFocusIn();
            return;
        }
    }
}


//--------------------------------------------------------------------------------------
bool CDXUTDialog::OnCycleFocus( _In_ bool bForward )
{
    CDXUTControl* pControl = nullptr;
    CDXUTDialog* pDialog = nullptr; // pDialog and pLastDialog are used to track wrapping of
    CDXUTDialog* pLastDialog;    // focus from first control to last or vice versa.

    if( !s_pControlFocus )
    {
        // If s_pControlFocus is nullptr, we focus the first control of first dialog in
        // the case that bForward is true, and focus the last control of last dialog when
        // bForward is false.
        //
        if( bForward )
        {
            // Search for the first control from the start of the dialog
            // array.
            for( auto it = m_pManager->m_Dialogs.cbegin(); it != m_pManager->m_Dialogs.cend(); ++it )
            {
                pDialog = pLastDialog = *it;
                if( pDialog && !pDialog->m_Controls.empty() )
                {
                    pControl = pDialog->m_Controls[ 0 ];
                    break;
                }
            }

            if( !pDialog || !pControl )
            {
                // No dialog has been registered yet or no controls have been
                // added to the dialogs. Cannot proceed.
                return true;
            }
        }
        else
        {
            // Search for the first control from the end of the dialog
            // array.
            for( auto it = m_pManager->m_Dialogs.crbegin(); it != m_pManager->m_Dialogs.crend(); ++it )
            {
                pDialog = pLastDialog = *it;
                if( pDialog && !pDialog->m_Controls.empty() )
                {
                    pControl = pDialog->m_Controls[ pDialog->m_Controls.size() - 1 ];
                    break;
                }
            }

            if( !pDialog || !pControl )
            {
                // No dialog has been registered yet or no controls have been
                // added to the dialogs. Cannot proceed.
                return true;
            }
        }
    }
    else if( s_pControlFocus->m_pDialog != this )
    {
        // If a control belonging to another dialog has focus, let that other
        // dialog handle this event by returning false.
        //
        return false;
    }
    else
    {
        // Focused control belongs to this dialog. Cycle to the
        // next/previous control.
        assert( pControl != 0 );
        _Analysis_assume_( pControl != 0 );
        pLastDialog = s_pControlFocus->m_pDialog;
        pControl = ( bForward ) ? GetNextControl( s_pControlFocus ) : GetPrevControl( s_pControlFocus );
        pDialog = pControl->m_pDialog;
    }

    assert( pControl != 0 );
    _Analysis_assume_( pControl != 0 );

    for( int i = 0; i < 0xffff; i++ )
    {
        // If we just wrapped from last control to first or vice versa,
        // set the focused control to nullptr. This state, where no control
        // has focus, allows the camera to work.
        int nLastDialogIndex = -1;
        auto fit = std::find( m_pManager->m_Dialogs.cbegin(), m_pManager->m_Dialogs.cend(), pLastDialog );
        if ( fit != m_pManager->m_Dialogs.cend() )
        {
            nLastDialogIndex = int( fit - m_pManager->m_Dialogs.begin() );
        }

        int nDialogIndex = -1;
        fit = std::find( m_pManager->m_Dialogs.cbegin(), m_pManager->m_Dialogs.cend(), pDialog );
        if ( fit != m_pManager->m_Dialogs.cend() )
        {
            nDialogIndex = int( fit - m_pManager->m_Dialogs.begin() );
        }

        if( ( !bForward && nLastDialogIndex < nDialogIndex ) ||
            ( bForward && nDialogIndex < nLastDialogIndex ) )
        {
            if( s_pControlFocus )
                s_pControlFocus->OnFocusOut();
            s_pControlFocus = nullptr;
            return true;
        }

        // If we've gone in a full circle then focus doesn't change
        if( pControl == s_pControlFocus )
            return true;

        // If the dialog accepts keybord input and the control can have focus then
        // move focus
        if( pControl->m_pDialog->m_bKeyboardInput && pControl->CanHaveFocus() )
        {
            if( s_pControlFocus )
                s_pControlFocus->OnFocusOut();
            s_pControlFocus = pControl;
            if( s_pControlFocus )
            s_pControlFocus->OnFocusIn();
            return true;
        }

        pLastDialog = pDialog;
        pControl = ( bForward ) ? GetNextControl( pControl ) : GetPrevControl( pControl );
        pDialog = pControl->m_pDialog;
    }

    // If we reached this point, the chain of dialogs didn't form a complete loop
    DXTRACE_ERR( L"CDXUTDialog: Multiple dialogs are improperly chained together", E_FAIL );
    return false;
}


//--------------------------------------------------------------------------------------
void CDXUTDialog::InitDefaultElements()
{
    SetFont( 0, L"Arial", 14, FW_NORMAL );

    CDXUTElement Element;
    RECT rcTexture;

    //-------------------------------------
    // Element for the caption
    //-------------------------------------
    m_CapElement.SetFont( 0 );
    SetRect( &rcTexture, 17, 269, 241, 287 );
    m_CapElement.SetTexture( 0, &rcTexture );
    m_CapElement.TextureColor.States[ DXUT_STATE_NORMAL ] = D3DCOLOR_ARGB( 255, 255, 255, 255 );
    m_CapElement.FontColor.States[ DXUT_STATE_NORMAL ] = D3DCOLOR_ARGB( 255, 255, 255, 255 );
    m_CapElement.SetFont( 0, D3DCOLOR_ARGB( 255, 255, 255, 255 ), DT_LEFT | DT_VCENTER );
    // Pre-blend as we don't need to transition the state
    m_CapElement.TextureColor.Blend( DXUT_STATE_NORMAL, 10.0f );
    m_CapElement.FontColor.Blend( DXUT_STATE_NORMAL, 10.0f );

    //-------------------------------------
    // CDXUTStatic
    //-------------------------------------
    Element.SetFont( 0 );
    Element.FontColor.States[ DXUT_STATE_DISABLED ] = D3DCOLOR_ARGB( 200, 200, 200, 200 );

    // Assign the Element
    SetDefaultElement( DXUT_CONTROL_STATIC, 0, &Element );


    //-------------------------------------
    // CDXUTButton - Button
    //-------------------------------------
    SetRect( &rcTexture, 0, 0, 136, 54 );
    Element.SetTexture( 0, &rcTexture );
    Element.SetFont( 0 );
    Element.TextureColor.States[ DXUT_STATE_NORMAL ] = D3DCOLOR_ARGB( 150, 255, 255, 255 );
    Element.TextureColor.States[ DXUT_STATE_PRESSED ] = D3DCOLOR_ARGB( 200, 255, 255, 255 );
    Element.FontColor.States[ DXUT_STATE_MOUSEOVER ] = D3DCOLOR_ARGB( 255, 0, 0, 0 );

    // Assign the Element
    SetDefaultElement( DXUT_CONTROL_BUTTON, 0, &Element );


    //-------------------------------------
    // CDXUTButton - Fill layer
    //-------------------------------------
    SetRect( &rcTexture, 136, 0, 252, 54 );
    Element.SetTexture( 0, &rcTexture, D3DCOLOR_ARGB( 0, 255, 255, 255 ) );
    Element.TextureColor.States[ DXUT_STATE_MOUSEOVER ] = D3DCOLOR_ARGB( 160, 255, 255, 255 );
    Element.TextureColor.States[ DXUT_STATE_PRESSED ] = D3DCOLOR_ARGB( 60, 0, 0, 0 );
    Element.TextureColor.States[ DXUT_STATE_FOCUS ] = D3DCOLOR_ARGB( 30, 255, 255, 255 );


    // Assign the Element
    SetDefaultElement( DXUT_CONTROL_BUTTON, 1, &Element );


    //-------------------------------------
    // CDXUTCheckBox - Box
    //-------------------------------------
    SetRect( &rcTexture, 0, 54, 27, 81 );
    Element.SetTexture( 0, &rcTexture );
    Element.SetFont( 0, D3DCOLOR_ARGB( 255, 255, 255, 255 ), DT_LEFT | DT_VCENTER );
    Element.FontColor.States[ DXUT_STATE_DISABLED ] = D3DCOLOR_ARGB( 200, 200, 200, 200 );
    Element.TextureColor.States[ DXUT_STATE_NORMAL ] = D3DCOLOR_ARGB( 150, 255, 255, 255 );
    Element.TextureColor.States[ DXUT_STATE_FOCUS ] = D3DCOLOR_ARGB( 200, 255, 255, 255 );
    Element.TextureColor.States[ DXUT_STATE_PRESSED ] = D3DCOLOR_ARGB( 255, 255, 255, 255 );

    // Assign the Element
    SetDefaultElement( DXUT_CONTROL_CHECKBOX, 0, &Element );


    //-------------------------------------
    // CDXUTCheckBox - Check
    //-------------------------------------
    SetRect( &rcTexture, 27, 54, 54, 81 );
    Element.SetTexture( 0, &rcTexture );

    // Assign the Element
    SetDefaultElement( DXUT_CONTROL_CHECKBOX, 1, &Element );


    //-------------------------------------
    // CDXUTRadioButton - Box
    //-------------------------------------
    SetRect( &rcTexture, 54, 54, 81, 81 );
    Element.SetTexture( 0, &rcTexture );
    Element.SetFont( 0, D3DCOLOR_ARGB( 255, 255, 255, 255 ), DT_LEFT | DT_VCENTER );
    Element.FontColor.States[ DXUT_STATE_DISABLED ] = D3DCOLOR_ARGB( 200, 200, 200, 200 );
    Element.TextureColor.States[ DXUT_STATE_NORMAL ] = D3DCOLOR_ARGB( 150, 255, 255, 255 );
    Element.TextureColor.States[ DXUT_STATE_FOCUS ] = D3DCOLOR_ARGB( 200, 255, 255, 255 );
    Element.TextureColor.States[ DXUT_STATE_PRESSED ] = D3DCOLOR_ARGB( 255, 255, 255, 255 );

    // Assign the Element
    SetDefaultElement( DXUT_CONTROL_RADIOBUTTON, 0, &Element );


    //-------------------------------------
    // CDXUTRadioButton - Check
    //-------------------------------------
    SetRect( &rcTexture, 81, 54, 108, 81 );
    Element.SetTexture( 0, &rcTexture );

    // Assign the Element
    SetDefaultElement( DXUT_CONTROL_RADIOBUTTON, 1, &Element );


    //-------------------------------------
    // CDXUTComboBox - Main
    //-------------------------------------
    SetRect( &rcTexture, 7, 81, 247, 123 );
    Element.SetTexture( 0, &rcTexture );
    Element.SetFont( 0 );
    Element.TextureColor.States[ DXUT_STATE_NORMAL ] = D3DCOLOR_ARGB( 150, 200, 200, 200 );
    Element.TextureColor.States[ DXUT_STATE_FOCUS ] = D3DCOLOR_ARGB( 170, 230, 230, 230 );
    Element.TextureColor.States[ DXUT_STATE_DISABLED ] = D3DCOLOR_ARGB( 70, 200, 200, 200 );
    Element.FontColor.States[ DXUT_STATE_MOUSEOVER ] = D3DCOLOR_ARGB( 255, 0, 0, 0 );
    Element.FontColor.States[ DXUT_STATE_PRESSED ] = D3DCOLOR_ARGB( 255, 0, 0, 0 );
    Element.FontColor.States[ DXUT_STATE_DISABLED ] = D3DCOLOR_ARGB( 200, 200, 200, 200 );


    // Assign the Element
    SetDefaultElement( DXUT_CONTROL_COMBOBOX, 0, &Element );


    //-------------------------------------
    // CDXUTComboBox - Button
    //-------------------------------------
    SetRect( &rcTexture, 98, 189, 151, 238 );
    Element.SetTexture( 0, &rcTexture );
    Element.TextureColor.States[ DXUT_STATE_NORMAL ] = D3DCOLOR_ARGB( 150, 255, 255, 255 );
    Element.TextureColor.States[ DXUT_STATE_PRESSED ] = D3DCOLOR_ARGB( 255, 150, 150, 150 );
    Element.TextureColor.States[ DXUT_STATE_FOCUS ] = D3DCOLOR_ARGB( 200, 255, 255, 255 );
    Element.TextureColor.States[ DXUT_STATE_DISABLED ] = D3DCOLOR_ARGB( 70, 255, 255, 255 );

    // Assign the Element
    SetDefaultElement( DXUT_CONTROL_COMBOBOX, 1, &Element );


    //-------------------------------------
    // CDXUTComboBox - Dropdown
    //-------------------------------------
    SetRect( &rcTexture, 13, 123, 241, 160 );
    Element.SetTexture( 0, &rcTexture );
    Element.SetFont( 0, D3DCOLOR_ARGB( 255, 0, 0, 0 ), DT_LEFT | DT_TOP );

    // Assign the Element
    SetDefaultElement( DXUT_CONTROL_COMBOBOX, 2, &Element );


    //-------------------------------------
    // CDXUTComboBox - Selection
    //-------------------------------------
    SetRect( &rcTexture, 12, 163, 239, 183 );
    Element.SetTexture( 0, &rcTexture );
    Element.SetFont( 0, D3DCOLOR_ARGB( 255, 255, 255, 255 ), DT_LEFT | DT_TOP );

    // Assign the Element
    SetDefaultElement( DXUT_CONTROL_COMBOBOX, 3, &Element );


    //-------------------------------------
    // CDXUTSlider - Track
    //-------------------------------------
    SetRect( &rcTexture, 1, 187, 93, 228 );
    Element.SetTexture( 0, &rcTexture );
    Element.TextureColor.States[ DXUT_STATE_NORMAL ] = D3DCOLOR_ARGB( 150, 255, 255, 255 );
    Element.TextureColor.States[ DXUT_STATE_FOCUS ] = D3DCOLOR_ARGB( 200, 255, 255, 255 );
    Element.TextureColor.States[ DXUT_STATE_DISABLED ] = D3DCOLOR_ARGB( 70, 255, 255, 255 );

    // Assign the Element
    SetDefaultElement( DXUT_CONTROL_SLIDER, 0, &Element );

    //-------------------------------------
    // CDXUTSlider - Button
    //-------------------------------------
    SetRect( &rcTexture, 151, 193, 192, 234 );
    Element.SetTexture( 0, &rcTexture );

    // Assign the Element
    SetDefaultElement( DXUT_CONTROL_SLIDER, 1, &Element );

    //-------------------------------------
    // CDXUTScrollBar - Track
    //-------------------------------------
    int nScrollBarStartX = 196;
    int nScrollBarStartY = 191;
    SetRect( &rcTexture, nScrollBarStartX + 0, nScrollBarStartY + 21, nScrollBarStartX + 22, nScrollBarStartY + 32 );
    Element.SetTexture( 0, &rcTexture );
    Element.TextureColor.States[ DXUT_STATE_DISABLED ] = D3DCOLOR_ARGB( 255, 200, 200, 200 );

    // Assign the Element
    SetDefaultElement( DXUT_CONTROL_SCROLLBAR, 0, &Element );

    //-------------------------------------
    // CDXUTScrollBar - Up Arrow
    //-------------------------------------
    SetRect( &rcTexture, nScrollBarStartX + 0, nScrollBarStartY + 1, nScrollBarStartX + 22, nScrollBarStartY + 21 );
    Element.SetTexture( 0, &rcTexture );
    Element.TextureColor.States[ DXUT_STATE_DISABLED ] = D3DCOLOR_ARGB( 255, 200, 200, 200 );


    // Assign the Element
    SetDefaultElement( DXUT_CONTROL_SCROLLBAR, 1, &Element );

    //-------------------------------------
    // CDXUTScrollBar - Down Arrow
    //-------------------------------------
    SetRect( &rcTexture, nScrollBarStartX + 0, nScrollBarStartY + 32, nScrollBarStartX + 22, nScrollBarStartY + 53 );
    Element.SetTexture( 0, &rcTexture );
    Element.TextureColor.States[ DXUT_STATE_DISABLED ] = D3DCOLOR_ARGB( 255, 200, 200, 200 );


    // Assign the Element
    SetDefaultElement( DXUT_CONTROL_SCROLLBAR, 2, &Element );

    //-------------------------------------
    // CDXUTScrollBar - Button
    //-------------------------------------
    SetRect( &rcTexture, 220, 192, 238, 234 );
    Element.SetTexture( 0, &rcTexture );

    // Assign the Element
    SetDefaultElement( DXUT_CONTROL_SCROLLBAR, 3, &Element );


    //-------------------------------------
    // CDXUTEditBox
    //-------------------------------------
    // Element assignment:
    //   0 - text area
    //   1 - top left border
    //   2 - top border
    //   3 - top right border
    //   4 - left border
    //   5 - right border
    //   6 - lower left border
    //   7 - lower border
    //   8 - lower right border

    Element.SetFont( 0, D3DCOLOR_ARGB( 255, 0, 0, 0 ), DT_LEFT | DT_TOP );

    // Assign the style
    SetRect( &rcTexture, 14, 90, 241, 113 );
    Element.SetTexture( 0, &rcTexture );
    SetDefaultElement( DXUT_CONTROL_EDITBOX, 0, &Element );
    SetRect( &rcTexture, 8, 82, 14, 90 );
    Element.SetTexture( 0, &rcTexture );
    SetDefaultElement( DXUT_CONTROL_EDITBOX, 1, &Element );
    SetRect( &rcTexture, 14, 82, 241, 90 );
    Element.SetTexture( 0, &rcTexture );
    SetDefaultElement( DXUT_CONTROL_EDITBOX, 2, &Element );
    SetRect( &rcTexture, 241, 82, 246, 90 );
    Element.SetTexture( 0, &rcTexture );
    SetDefaultElement( DXUT_CONTROL_EDITBOX, 3, &Element );
    SetRect( &rcTexture, 8, 90, 14, 113 );
    Element.SetTexture( 0, &rcTexture );
    SetDefaultElement( DXUT_CONTROL_EDITBOX, 4, &Element );
    SetRect( &rcTexture, 241, 90, 246, 113 );
    Element.SetTexture( 0, &rcTexture );
    SetDefaultElement( DXUT_CONTROL_EDITBOX, 5, &Element );
    SetRect( &rcTexture, 8, 113, 14, 121 );
    Element.SetTexture( 0, &rcTexture );
    SetDefaultElement( DXUT_CONTROL_EDITBOX, 6, &Element );
    SetRect( &rcTexture, 14, 113, 241, 121 );
    Element.SetTexture( 0, &rcTexture );
    SetDefaultElement( DXUT_CONTROL_EDITBOX, 7, &Element );
    SetRect( &rcTexture, 241, 113, 246, 121 );
    Element.SetTexture( 0, &rcTexture );
    SetDefaultElement( DXUT_CONTROL_EDITBOX, 8, &Element );

    //-------------------------------------
    // CDXUTListBox - Main
    //-------------------------------------
    SetRect( &rcTexture, 13, 123, 241, 160 );
    Element.SetTexture( 0, &rcTexture );
    Element.SetFont( 0, D3DCOLOR_ARGB( 255, 0, 0, 0 ), DT_LEFT | DT_TOP );

    // Assign the Element
    SetDefaultElement( DXUT_CONTROL_LISTBOX, 0, &Element );

    //-------------------------------------
    // CDXUTListBox - Selection
    //-------------------------------------

    SetRect( &rcTexture, 16, 166, 240, 183 );
    Element.SetTexture( 0, &rcTexture );
    Element.SetFont( 0, D3DCOLOR_ARGB( 255, 255, 255, 255 ), DT_LEFT | DT_TOP );

    // Assign the Element
    SetDefaultElement( DXUT_CONTROL_LISTBOX, 1, &Element );
}


//======================================================================================
// CDXUTDialogResourceManager
//======================================================================================

//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager::CDXUTDialogResourceManager() :
    m_pVSRenderUI11(nullptr),
    m_pPSRenderUI11(nullptr),
    m_pPSRenderUIUntex11(nullptr),
    m_pDepthStencilStateUI11(nullptr),
    m_pRasterizerStateUI11(nullptr),
    m_pBlendStateUI11(nullptr),
    m_pSamplerStateUI11(nullptr),
    m_pDepthStencilStateStored11(nullptr),
    m_pRasterizerStateStored11(nullptr),
    m_pBlendStateStored11(nullptr),
    m_pSamplerStateStored11(nullptr),
    m_pInputLayout11(nullptr),
    m_pVBScreenQuad11(nullptr),
    m_pSpriteBuffer11(nullptr),
    m_SpriteBufferBytes11(0)
{
}


//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager::~CDXUTDialogResourceManager()
{
    for( auto it = m_FontCache.begin(); it != m_FontCache.end(); ++it )
    {
        SAFE_DELETE( *it );
    }
    m_FontCache.clear();

    for( auto it = m_TextureCache.begin(); it != m_TextureCache.end(); ++it )
    {
        SAFE_DELETE( *it );
    }
    m_TextureCache.clear();
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTDialogResourceManager::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    return false;
}


_Use_decl_annotations_
HRESULT CDXUTDialogResourceManager::OnD3D11CreateDevice( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3d11DeviceContext )
{
    m_pd3d11Device = pd3dDevice;
    m_pd3d11DeviceContext = pd3d11DeviceContext;

    HRESULT hr = S_OK;

    // Compile Shaders
    ID3DBlob* pVSBlob = nullptr;
    ID3DBlob* pPSBlob = nullptr;
    ID3DBlob* pPSUntexBlob = nullptr;
    V_RETURN( D3DCompile( g_strUIEffectFile, g_uUIEffectFileSize, "none", nullptr, nullptr, "VS", "vs_4_0_level_9_1", 
         D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY, 0, &pVSBlob, nullptr ) );
    V_RETURN( D3DCompile( g_strUIEffectFile, g_uUIEffectFileSize, "none", nullptr, nullptr, "PS", "ps_4_0_level_9_1", 
         D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY, 0, &pPSBlob, nullptr ) );
    V_RETURN( D3DCompile( g_strUIEffectFile, g_uUIEffectFileSize, "none", nullptr, nullptr, "PSUntex", "ps_4_0_level_9_1", 
         D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY, 0, &pPSUntexBlob, nullptr ) );

    // Create Shaders
    V_RETURN( pd3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVSRenderUI11 ) );
    DXUT_SetDebugName( m_pVSRenderUI11, "CDXUTDialogResourceManager" );

    V_RETURN( pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPSRenderUI11 ) );
    DXUT_SetDebugName( m_pPSRenderUI11, "CDXUTDialogResourceManager" );

    V_RETURN( pd3dDevice->CreatePixelShader( pPSUntexBlob->GetBufferPointer(), pPSUntexBlob->GetBufferSize(), nullptr, &m_pPSRenderUIUntex11 ) );
    DXUT_SetDebugName( m_pPSRenderUIUntex11, "CDXUTDialogResourceManager" );
    
    // States
    D3D11_DEPTH_STENCIL_DESC DSDesc;
    ZeroMemory( &DSDesc, sizeof( D3D11_DEPTH_STENCIL_DESC ) );
    DSDesc.DepthEnable = FALSE;
    DSDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    DSDesc.DepthFunc = D3D11_COMPARISON_LESS;
    DSDesc.StencilEnable = FALSE;
    V_RETURN( pd3dDevice->CreateDepthStencilState( &DSDesc, &m_pDepthStencilStateUI11 ) );
    DXUT_SetDebugName( m_pDepthStencilStateUI11, "CDXUTDialogResourceManager" );

    D3D11_RASTERIZER_DESC RSDesc;
    RSDesc.AntialiasedLineEnable = FALSE;
    RSDesc.CullMode = D3D11_CULL_BACK;
    RSDesc.DepthBias = 0;
    RSDesc.DepthBiasClamp = 0.0f;
    RSDesc.DepthClipEnable = TRUE;
    RSDesc.FillMode = D3D11_FILL_SOLID;
    RSDesc.FrontCounterClockwise = FALSE;
    RSDesc.MultisampleEnable = TRUE;
    RSDesc.ScissorEnable = FALSE;
    RSDesc.SlopeScaledDepthBias = 0.0f;
    V_RETURN( pd3dDevice->CreateRasterizerState( &RSDesc, &m_pRasterizerStateUI11 ) );
    DXUT_SetDebugName( m_pRasterizerStateUI11, "CDXUTDialogResourceManager" );

    D3D11_BLEND_DESC BSDesc;
    ZeroMemory( &BSDesc, sizeof( D3D11_BLEND_DESC ) );
    
    BSDesc.RenderTarget[0].BlendEnable = TRUE;
    BSDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    BSDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    BSDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    BSDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    BSDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    BSDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    BSDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;

    V_RETURN( pd3dDevice->CreateBlendState( &BSDesc, &m_pBlendStateUI11 ) );
    DXUT_SetDebugName( m_pBlendStateUI11, "CDXUTDialogResourceManager" );

    D3D11_SAMPLER_DESC SSDesc;
    ZeroMemory( &SSDesc, sizeof( D3D11_SAMPLER_DESC ) );
    SSDesc.Filter = D3D11_FILTER_ANISOTROPIC   ;
    SSDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    SSDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    SSDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    SSDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    SSDesc.MaxAnisotropy = 16;
    SSDesc.MinLOD = 0;
    SSDesc.MaxLOD = D3D11_FLOAT32_MAX;
    if ( pd3dDevice->GetFeatureLevel() < D3D_FEATURE_LEVEL_9_3 )
    {
        SSDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        SSDesc.MaxAnisotropy = 0;
    }
    V_RETURN( pd3dDevice->CreateSamplerState( &SSDesc, &m_pSamplerStateUI11 ) );
    DXUT_SetDebugName( m_pSamplerStateUI11, "CDXUTDialogResourceManager" );

    // Create the texture objects in the cache arrays.
    for( size_t i = 0; i < m_TextureCache.size(); i++ )
    {
        hr = CreateTexture11( static_cast<UINT>( i ) );
        if( FAILED( hr ) )
            return hr;
    }

    // Create input layout
    const D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,       0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

    V_RETURN( pd3dDevice->CreateInputLayout( layout, ARRAYSIZE( layout ), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayout11 ) );
    DXUT_SetDebugName( m_pInputLayout11, "CDXUTDialogResourceManager" );

    // Release the blobs
    SAFE_RELEASE( pVSBlob );
    SAFE_RELEASE( pPSBlob );
    SAFE_RELEASE( pPSUntexBlob );

    // Create a vertex buffer quad for rendering later
    D3D11_BUFFER_DESC BufDesc;
    BufDesc.ByteWidth = sizeof( DXUT_SCREEN_VERTEX_10 ) * 4;
    BufDesc.Usage = D3D11_USAGE_DYNAMIC;
    BufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    BufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    BufDesc.MiscFlags = 0;
    V_RETURN( pd3dDevice->CreateBuffer( &BufDesc, nullptr, &m_pVBScreenQuad11 ) );
    DXUT_SetDebugName( m_pVBScreenQuad11, "CDXUTDialogResourceManager" );

    // Init the D3D11 font
    InitFont11( pd3dDevice, m_pInputLayout11 );

    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTDialogResourceManager::OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice,
                                                             const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc )
{
    UNREFERENCED_PARAMETER(pd3dDevice);

    HRESULT hr = S_OK;

    m_nBackBufferWidth = pBackBufferSurfaceDesc->Width;
    m_nBackBufferHeight = pBackBufferSurfaceDesc->Height;

    return hr;
}


//--------------------------------------------------------------------------------------
void CDXUTDialogResourceManager::OnD3D11ReleasingSwapChain()
{
}


//--------------------------------------------------------------------------------------
void CDXUTDialogResourceManager::OnD3D11DestroyDevice()
{
    // Release the resources but don't clear the cache, as these will need to be
    // recreated if the device is recreated

    for( auto it = m_TextureCache.begin(); it != m_TextureCache.end(); ++it )
    {
        SAFE_RELEASE( (*it)->pTexResView11 );
        SAFE_RELEASE( (*it)->pTexture11 );
    }

    // D3D11
    SAFE_RELEASE( m_pVBScreenQuad11 );
    SAFE_RELEASE( m_pSpriteBuffer11 );
    m_SpriteBufferBytes11 = 0;
    SAFE_RELEASE( m_pInputLayout11 );

    // Shaders
    SAFE_RELEASE( m_pVSRenderUI11 );
    SAFE_RELEASE( m_pPSRenderUI11 );
    SAFE_RELEASE( m_pPSRenderUIUntex11 );

    // States
    SAFE_RELEASE( m_pDepthStencilStateUI11 );
    SAFE_RELEASE( m_pRasterizerStateUI11 );
    SAFE_RELEASE( m_pBlendStateUI11 );
    SAFE_RELEASE( m_pSamplerStateUI11 );

    SAFE_RELEASE( m_pDepthStencilStateStored11 );
    SAFE_RELEASE( m_pRasterizerStateStored11 );
    SAFE_RELEASE( m_pBlendStateStored11 );
    SAFE_RELEASE( m_pSamplerStateStored11 );

    EndFont11();
}


//--------------------------------------------------------------------------------------
void CDXUTDialogResourceManager::StoreD3D11State( _In_ ID3D11DeviceContext* pd3dImmediateContext )
{
    pd3dImmediateContext->OMGetDepthStencilState( &m_pDepthStencilStateStored11, &m_StencilRefStored11 );
    pd3dImmediateContext->RSGetState( &m_pRasterizerStateStored11 );
    pd3dImmediateContext->OMGetBlendState( &m_pBlendStateStored11, m_BlendFactorStored11, &m_SampleMaskStored11 );
    pd3dImmediateContext->PSGetSamplers( 0, 1, &m_pSamplerStateStored11 );
}


//--------------------------------------------------------------------------------------
void CDXUTDialogResourceManager::RestoreD3D11State( _In_ ID3D11DeviceContext* pd3dImmediateContext )
{
    pd3dImmediateContext->OMSetDepthStencilState( m_pDepthStencilStateStored11, m_StencilRefStored11 );
    pd3dImmediateContext->RSSetState( m_pRasterizerStateStored11 );
    pd3dImmediateContext->OMSetBlendState( m_pBlendStateStored11, m_BlendFactorStored11, m_SampleMaskStored11 );
    pd3dImmediateContext->PSSetSamplers( 0, 1, &m_pSamplerStateStored11 );

    SAFE_RELEASE( m_pDepthStencilStateStored11 );
    SAFE_RELEASE( m_pRasterizerStateStored11 );
    SAFE_RELEASE( m_pBlendStateStored11 );
    SAFE_RELEASE( m_pSamplerStateStored11 );
}


//--------------------------------------------------------------------------------------
void CDXUTDialogResourceManager::ApplyRenderUI11( _In_ ID3D11DeviceContext* pd3dImmediateContext )
{
    // Shaders
    pd3dImmediateContext->VSSetShader( m_pVSRenderUI11, nullptr, 0 );
    pd3dImmediateContext->HSSetShader( nullptr, nullptr, 0 );
    pd3dImmediateContext->DSSetShader( nullptr, nullptr, 0 );
    pd3dImmediateContext->GSSetShader( nullptr, nullptr, 0 );
    pd3dImmediateContext->PSSetShader( m_pPSRenderUI11, nullptr, 0 );

    // States
    pd3dImmediateContext->OMSetDepthStencilState( m_pDepthStencilStateUI11, 0 );
    pd3dImmediateContext->RSSetState( m_pRasterizerStateUI11 );
    float BlendFactor[4] = { 0, 0, 0, 0 };
    pd3dImmediateContext->OMSetBlendState( m_pBlendStateUI11, BlendFactor, 0xFFFFFFFF );
    pd3dImmediateContext->PSSetSamplers( 0, 1, &m_pSamplerStateUI11 );
}


//--------------------------------------------------------------------------------------
void CDXUTDialogResourceManager::ApplyRenderUIUntex11( _In_ ID3D11DeviceContext* pd3dImmediateContext )
{
    // Shaders
    pd3dImmediateContext->VSSetShader( m_pVSRenderUI11, nullptr, 0 );
    pd3dImmediateContext->HSSetShader( nullptr, nullptr, 0 );
    pd3dImmediateContext->DSSetShader( nullptr, nullptr, 0 );
    pd3dImmediateContext->GSSetShader( nullptr, nullptr, 0 );
    pd3dImmediateContext->PSSetShader( m_pPSRenderUIUntex11, nullptr, 0 );

    // States
    pd3dImmediateContext->OMSetDepthStencilState( m_pDepthStencilStateUI11, 0 );
    pd3dImmediateContext->RSSetState( m_pRasterizerStateUI11 );
    float BlendFactor[4] = { 0, 0, 0, 0 };
    pd3dImmediateContext->OMSetBlendState( m_pBlendStateUI11, BlendFactor, 0xFFFFFFFF );
    pd3dImmediateContext->PSSetSamplers( 0, 1, &m_pSamplerStateUI11 );
}


//--------------------------------------------------------------------------------------
void CDXUTDialogResourceManager::BeginSprites11( )
{
    m_SpriteVertices.clear();
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTDialogResourceManager::EndSprites11( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext )
{

    // ensure our buffer size can hold our sprites
    UINT SpriteDataBytes = static_cast<UINT>( m_SpriteVertices.size() * sizeof( DXUTSpriteVertex ) );
    if( m_SpriteBufferBytes11 < SpriteDataBytes )
    {
        SAFE_RELEASE( m_pSpriteBuffer11 );
        m_SpriteBufferBytes11 = SpriteDataBytes;

        D3D11_BUFFER_DESC BufferDesc;
        BufferDesc.ByteWidth = m_SpriteBufferBytes11;
        BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        BufferDesc.MiscFlags = 0;

        if ( FAILED(pd3dDevice->CreateBuffer(&BufferDesc, nullptr, &m_pSpriteBuffer11)) )
        {
            m_pSpriteBuffer11 = nullptr;
            m_SpriteBufferBytes11 = 0;
            return;
        }
        DXUT_SetDebugName( m_pSpriteBuffer11, "CDXUTDialogResourceManager" );
    }

    // Copy the sprites over
    D3D11_BOX destRegion;
    destRegion.left = 0;
    destRegion.right = SpriteDataBytes;
    destRegion.top = 0;
    destRegion.bottom = 1;
    destRegion.front = 0;
    destRegion.back = 1;
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    if ( S_OK == pd3dImmediateContext->Map( m_pSpriteBuffer11, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) )
    { 
        memcpy( MappedResource.pData, (const void*)&m_SpriteVertices[0], SpriteDataBytes );
        pd3dImmediateContext->Unmap(m_pSpriteBuffer11, 0);
    }

    // Draw
    UINT Stride = sizeof( DXUTSpriteVertex );
    UINT Offset = 0;
    pd3dImmediateContext->IASetVertexBuffers( 0, 1, &m_pSpriteBuffer11, &Stride, &Offset );
    pd3dImmediateContext->IASetInputLayout( m_pInputLayout11 );
    pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    pd3dImmediateContext->Draw( static_cast<UINT>( m_SpriteVertices.size() ), 0 );

    m_SpriteVertices.clear();
}


//--------------------------------------------------------------------------------------
bool CDXUTDialogResourceManager::RegisterDialog( _In_ CDXUTDialog* pDialog )
{
    // Check that the dialog isn't already registered.
    for( auto it = m_Dialogs.cbegin(); it != m_Dialogs.cend(); ++it )
    {
        if( *it == pDialog )
            return true;
    }

    // Add to the list.
    m_Dialogs.push_back( pDialog );

    // Set up next and prev pointers.
    if( m_Dialogs.size() > 1 )
        m_Dialogs[m_Dialogs.size() - 2]->SetNextDialog( pDialog );
    m_Dialogs[m_Dialogs.size() - 1]->SetNextDialog( m_Dialogs[0] );

    return true;
}


//--------------------------------------------------------------------------------------
void CDXUTDialogResourceManager::UnregisterDialog( _In_ CDXUTDialog* pDialog )
{
    // Search for the dialog in the list.
    for( size_t i = 0; i < m_Dialogs.size(); ++i )
    {
        if( m_Dialogs[ i ] == pDialog )
        {
            m_Dialogs.erase( m_Dialogs.begin() + i );
            if( !m_Dialogs.empty() )
            {
                int l, r;

                if( 0 == i )
                    l = int( m_Dialogs.size() - 1 );
                else
                    l = int(i) - 1;

                if( m_Dialogs.size() == i )
                    r = 0;
                else
                    r = int( i );

                m_Dialogs[l]->SetNextDialog( m_Dialogs[r] );
            }
            return;
        }
    }
}


//--------------------------------------------------------------------------------------
void CDXUTDialogResourceManager::EnableKeyboardInputForAllDialogs()
{
    // Enable keyboard input for all registered dialogs
    for( auto it = m_Dialogs.begin(); it != m_Dialogs.end(); ++it )
        (*it)->EnableKeyboardInput( true );
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
int CDXUTDialogResourceManager::AddFont( LPCWSTR strFaceName, LONG height, LONG weight ) 
{
    // See if this font already exists
    for( size_t i = 0; i < m_FontCache.size(); ++i )
    {
        auto pFontNode = m_FontCache[ i ];
        size_t nLen = 0;
        nLen = wcsnlen( strFaceName, MAX_PATH);
        if( 0 == _wcsnicmp( pFontNode->strFace, strFaceName, nLen ) &&
            pFontNode->nHeight == height &&
            pFontNode->nWeight == weight )
        {
            return static_cast<int>( i );
        }
    }

    // Add a new font and try to create it
    auto pNewFontNode = new (std::nothrow) DXUTFontNode;
    if( !pNewFontNode )
        return -1;

    ZeroMemory( pNewFontNode, sizeof( DXUTFontNode ) );
    wcscpy_s( pNewFontNode->strFace, MAX_PATH, strFaceName );
    pNewFontNode->nHeight = height;
    pNewFontNode->nWeight = weight;
    m_FontCache.push_back( pNewFontNode );

    int iFont = (int)m_FontCache.size() - 1;

    // If a device is available, try to create immediately
    return iFont;
}


//--------------------------------------------------------------------------------------
int CDXUTDialogResourceManager::AddTexture( _In_z_ LPCWSTR strFilename )
{
    // See if this texture already exists
    for( size_t i = 0; i < m_TextureCache.size(); ++i )
    {
        auto pTextureNode = m_TextureCache[ i ];
        size_t nLen = 0;
        nLen = wcsnlen( strFilename, MAX_PATH);
        if( pTextureNode->bFileSource &&  // Sources must match
            0 == _wcsnicmp( pTextureNode->strFilename, strFilename, nLen ) )
        {
            return static_cast<int>( i );
        }
    }

    // Add a new texture and try to create it
    auto pNewTextureNode = new (std::nothrow) DXUTTextureNode;
    if( !pNewTextureNode )
        return -1;

    ZeroMemory( pNewTextureNode, sizeof( DXUTTextureNode ) );
    pNewTextureNode->bFileSource = true;
    wcscpy_s( pNewTextureNode->strFilename, MAX_PATH, strFilename );

    m_TextureCache.push_back( pNewTextureNode );

    int iTexture = int( m_TextureCache.size() ) - 1;

    // If a device is available, try to create immediately

    return iTexture;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
int CDXUTDialogResourceManager::AddTexture( LPCWSTR strResourceName, HMODULE hResourceModule )
{
    // See if this texture already exists
    for( size_t i = 0; i < m_TextureCache.size(); i++ )
    {
        auto pTextureNode = m_TextureCache[ i ];
        if( !pTextureNode->bFileSource &&      // Sources must match
            pTextureNode->hResourceModule == hResourceModule ) // Module handles must match
        {
            if( IS_INTRESOURCE( strResourceName ) )
            {
                // Integer-based ID
                if( ( INT_PTR )strResourceName == pTextureNode->nResourceID )
                    return static_cast<int>( i );
            }
            else
            {
                // String-based ID
                size_t nLen = 0;
                nLen = wcsnlen ( strResourceName, MAX_PATH );
                if( 0 == _wcsnicmp( pTextureNode->strFilename, strResourceName, nLen ) )
                    return static_cast<int>( i );
            }
        }
    }

    // Add a new texture and try to create it
    auto pNewTextureNode = new (std::nothrow) DXUTTextureNode;
    if( !pNewTextureNode )
        return -1;

    ZeroMemory( pNewTextureNode, sizeof( DXUTTextureNode ) );
    pNewTextureNode->hResourceModule = hResourceModule;
    if( IS_INTRESOURCE( strResourceName ) )
    {
        pNewTextureNode->nResourceID = ( int )( size_t )strResourceName;
    }
    else
    {
        pNewTextureNode->nResourceID = 0;
        wcscpy_s( pNewTextureNode->strFilename, MAX_PATH, strResourceName );
    }

    m_TextureCache.push_back( pNewTextureNode );

    int iTexture = int( m_TextureCache.size() ) - 1;

    // If a device is available, try to create immediately

    return iTexture;
}


//--------------------------------------------------------------------------------------
HRESULT CDXUTDialogResourceManager::CreateTexture11( _In_ UINT iTexture )
{
    HRESULT hr = S_OK;

    auto pTextureNode = m_TextureCache[ iTexture ];

    if( !pTextureNode->bFileSource )
    {
        if( pTextureNode->nResourceID == 0xFFFF && pTextureNode->hResourceModule == ( HMODULE )0xFFFF )
        {
            hr = DXUTCreateGUITextureFromInternalArray( m_pd3d11Device, &pTextureNode->pTexture11 );
            if( FAILED( hr ) )
                return DXTRACE_ERR( L"DXUTCreateGUITextureFromInternalArray", hr );
            DXUT_SetDebugName( pTextureNode->pTexture11, "DXUT GUI Texture" );
        }
    }

    // Store dimensions
    D3D11_TEXTURE2D_DESC desc;
    pTextureNode->pTexture11->GetDesc( &desc );
    pTextureNode->dwWidth = desc.Width;
    pTextureNode->dwHeight = desc.Height;

    // Create resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Format = desc.Format;
    SRVDesc.Texture2D.MipLevels = 1;
    SRVDesc.Texture2D.MostDetailedMip = 0;
    hr = m_pd3d11Device->CreateShaderResourceView( pTextureNode->pTexture11, &SRVDesc, &pTextureNode->pTexResView11 );
    if ( FAILED(hr) )
        return hr;

    DXUT_SetDebugName( pTextureNode->pTexResView11, "DXUT GUI Texture" );

    return hr;
}


//======================================================================================
// CDXUTControl class
//======================================================================================

CDXUTControl::CDXUTControl( _In_opt_ CDXUTDialog* pDialog )
{
    m_Type = DXUT_CONTROL_BUTTON;
    m_pDialog = pDialog;
    m_ID = 0;
    m_nHotkey = 0; 
    m_Index = 0;
    m_pUserData = nullptr;

    m_bEnabled = true;
    m_bVisible = true;
    m_bMouseOver = false;
    m_bHasFocus = false;
    m_bIsDefault = false;

    m_pDialog = nullptr;

    m_x = 0;
    m_y = 0;
    m_width = 0;
    m_height = 0;

    ZeroMemory( &m_rcBoundingBox, sizeof( m_rcBoundingBox ) );
}


//--------------------------------------------------------------------------------------
CDXUTControl::~CDXUTControl()
{
    for( auto it = m_Elements.begin(); it != m_Elements.end(); ++it )
    {
        auto pElement = *it;
        delete pElement;
    }
    m_Elements.clear();
}


//--------------------------------------------------------------------------------------
void CDXUTControl::SetTextColor( _In_ DWORD Color )
{
    auto pElement = m_Elements[ 0 ];

    if( pElement )
        pElement->FontColor.States[DXUT_STATE_NORMAL] = Color;
}


//--------------------------------------------------------------------------------------
HRESULT CDXUTControl::SetElement( _In_ UINT iElement, _In_ CDXUTElement* pElement )
{
    if( !pElement )
        return E_INVALIDARG;

    // Make certain the array is this large
    for( size_t i = m_Elements.size(); i <= iElement; i++ )
    {
        auto pNewElement = new (std::nothrow) CDXUTElement();
        if( !pNewElement )
            return E_OUTOFMEMORY;

        m_Elements.push_back( pNewElement );
    }

    // Update the data
    auto pCurElement = m_Elements[ iElement ];
    *pCurElement = *pElement;

    return S_OK;
}


//--------------------------------------------------------------------------------------
void CDXUTControl::Refresh()
{
    m_bMouseOver = false;
    m_bHasFocus = false;

    for( auto it = m_Elements.begin(); it != m_Elements.end(); ++it )
    {
        (*it)->Refresh();
    }
}


//--------------------------------------------------------------------------------------
void CDXUTControl::UpdateRects()
{
    SetRect( &m_rcBoundingBox, m_x, m_y, m_x + m_width, m_y + m_height );
}


//======================================================================================
// CDXUTStatic class
//======================================================================================

//--------------------------------------------------------------------------------------
CDXUTStatic::CDXUTStatic( _In_opt_ CDXUTDialog* pDialog )
{
    m_Type = DXUT_CONTROL_STATIC;
    m_pDialog = pDialog;

    ZeroMemory( &m_strText, sizeof( m_strText ) );

    for( auto it = m_Elements.begin(); it != m_Elements.end(); ++it )
    {
        auto pElement = *it;
        SAFE_DELETE( pElement );
    }

    m_Elements.clear();
}


//--------------------------------------------------------------------------------------
void CDXUTStatic::Render( _In_ float fElapsedTime )
{
    if( m_bVisible == false )
        return;

    DXUT_CONTROL_STATE iState = DXUT_STATE_NORMAL;

    if( m_bEnabled == false )
        iState = DXUT_STATE_DISABLED;

    auto pElement = m_Elements[ 0 ];

    pElement->FontColor.Blend( iState, fElapsedTime );

    m_pDialog->DrawText( m_strText, pElement, &m_rcBoundingBox, false, false);
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTStatic::GetTextCopy( LPWSTR strDest, UINT bufferCount ) const
{
    // Validate incoming parameters
    if( !strDest || bufferCount == 0 )
    {
        return E_INVALIDARG;
    }

    // Copy the window text
    wcscpy_s( strDest, bufferCount, m_strText );

    return S_OK;
}


//--------------------------------------------------------------------------------------
HRESULT CDXUTStatic::SetText( _In_z_ LPCWSTR strText )
{
    if( !strText )
    {
        m_strText[0] = 0;
        return S_OK;
    }

    wcscpy_s( m_strText, MAX_PATH, strText );
    return S_OK;
}


//======================================================================================
// CDXUTButton class
//======================================================================================

CDXUTButton::CDXUTButton( _In_opt_ CDXUTDialog* pDialog )
{
    m_Type = DXUT_CONTROL_BUTTON;
    m_pDialog = pDialog;

    m_bPressed = false;
    m_nHotkey = 0;
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTButton::HandleKeyboard( UINT uMsg, WPARAM wParam, LPARAM lParam )

{
    UNREFERENCED_PARAMETER(lParam);

    if( !m_bEnabled || !m_bVisible )
        return false;

    switch( uMsg )
    {
        case WM_KEYDOWN:
        {
            switch( wParam )
            {
                case VK_SPACE:
                    m_bPressed = true;
                    return true;
            }
        }

        case WM_KEYUP:
        {
            switch( wParam )
            {
                case VK_SPACE:
                    if( m_bPressed == true )
                    {
                        m_bPressed = false;
                        m_pDialog->SendEvent( EVENT_BUTTON_CLICKED, true, this );
                    }
                    return true;
            }
        }
    }
    return false;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTButton::HandleMouse( UINT uMsg, const POINT& pt, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    if( !m_bEnabled || !m_bVisible )
        return false;

    switch( uMsg )
    {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            {
                if( ContainsPoint( pt ) )
                {
                    // Pressed while inside the control
                    m_bPressed = true;
                    SetCapture( DXUTGetHWND() );

                    if( !m_bHasFocus )
                        m_pDialog->RequestFocus( this );

                    return true;
                }

                break;
            }

        case WM_LBUTTONUP:
        {
            if( m_bPressed )
            {
                m_bPressed = false;
                ReleaseCapture();

                if( !m_pDialog->m_bKeyboardInput )
                    m_pDialog->ClearFocus();

                // Button click
                if( ContainsPoint( pt ) )
                    m_pDialog->SendEvent( EVENT_BUTTON_CLICKED, true, this );

                return true;
            }

            break;
        }
    };

    return false;
}

//--------------------------------------------------------------------------------------
void CDXUTButton::Render( _In_ float fElapsedTime )
{
    if( m_bVisible == false )
        return;

    int nOffsetX = 0;
    int nOffsetY = 0;

    DXUT_CONTROL_STATE iState = DXUT_STATE_NORMAL;

    if( m_bVisible == false )
    {
        iState = DXUT_STATE_HIDDEN;
    }
    else if( m_bEnabled == false )
    {
        iState = DXUT_STATE_DISABLED;
    }
    else if( m_bPressed )
    {
        iState = DXUT_STATE_PRESSED;

        nOffsetX = 1;
        nOffsetY = 2;
    }
    else if( m_bMouseOver )
    {
        iState = DXUT_STATE_MOUSEOVER;

        nOffsetX = -1;
        nOffsetY = -2;
    }
    else if( m_bHasFocus )
    {
        iState = DXUT_STATE_FOCUS;
    }

    // Background fill layer
    auto pElement = m_Elements[ 0 ];

    float fBlendRate = ( iState == DXUT_STATE_PRESSED ) ? 0.0f : 0.8f;

    RECT rcWindow = m_rcBoundingBox;
    OffsetRect( &rcWindow, nOffsetX, nOffsetY );


    // Blend current color
    pElement->TextureColor.Blend( iState, fElapsedTime, fBlendRate );
    pElement->FontColor.Blend( iState, fElapsedTime, fBlendRate );

    m_pDialog->DrawSprite( pElement, &rcWindow, DXUT_FAR_BUTTON_DEPTH );
    m_pDialog->DrawText( m_strText, pElement, &rcWindow, false, true );

    // Main button
    pElement = m_Elements[ 1 ];

    // Blend current color
    pElement->TextureColor.Blend( iState, fElapsedTime, fBlendRate );
    pElement->FontColor.Blend( iState, fElapsedTime, fBlendRate );

    m_pDialog->DrawSprite( pElement, &rcWindow, DXUT_NEAR_BUTTON_DEPTH );
    m_pDialog->DrawText( m_strText, pElement, &rcWindow, false, true );
}



//======================================================================================
// CDXUTCheckBox class
//======================================================================================

CDXUTCheckBox::CDXUTCheckBox( _In_opt_ CDXUTDialog* pDialog )
{
    m_Type = DXUT_CONTROL_CHECKBOX;
    m_pDialog = pDialog;

    m_bChecked = false;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTCheckBox::HandleKeyboard( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER(lParam);

    if( !m_bEnabled || !m_bVisible )
        return false;

    switch( uMsg )
    {
        case WM_KEYDOWN:
        {
            switch( wParam )
            {
                case VK_SPACE:
                    m_bPressed = true;
                    return true;
            }
        }

        case WM_KEYUP:
        {
            switch( wParam )
            {
                case VK_SPACE:
                    if( m_bPressed == true )
                    {
                        m_bPressed = false;
                        SetCheckedInternal( !m_bChecked, true );
                    }
                    return true;
            }
        }
    }
    return false;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTCheckBox::HandleMouse( UINT uMsg, const POINT& pt, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    if( !m_bEnabled || !m_bVisible )
        return false;

    switch( uMsg )
    {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            {
                if( ContainsPoint( pt ) )
                {
                    // Pressed while inside the control
                    m_bPressed = true;
                    SetCapture( DXUTGetHWND() );

                    if( !m_bHasFocus )
                        m_pDialog->RequestFocus( this );

                    return true;
                }

                break;
            }

        case WM_LBUTTONUP:
        {
            if( m_bPressed )
            {
                m_bPressed = false;
                ReleaseCapture();

                // Button click
                if( ContainsPoint( pt ) )
                    SetCheckedInternal( !m_bChecked, true );

                return true;
            }

            break;
        }
    };

    return false;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTCheckBox::SetCheckedInternal( bool bChecked, bool bFromInput )
{
    m_bChecked = bChecked;

    m_pDialog->SendEvent( EVENT_CHECKBOX_CHANGED, bFromInput, this );
}


//--------------------------------------------------------------------------------------
bool CDXUTCheckBox::ContainsPoint( _In_ const POINT& pt )
{
    return ( PtInRect( &m_rcBoundingBox, pt ) ||
             PtInRect( &m_rcButton, pt ) );
}


//--------------------------------------------------------------------------------------
void CDXUTCheckBox::UpdateRects()
{
    CDXUTButton::UpdateRects();

    m_rcButton = m_rcBoundingBox;
    m_rcButton.right = m_rcButton.left + RectHeight( m_rcButton );

    m_rcText = m_rcBoundingBox;
    m_rcText.left += ( int )( 1.25f * RectWidth( m_rcButton ) );
}


//--------------------------------------------------------------------------------------
void CDXUTCheckBox::Render( _In_ float fElapsedTime )
{
    if( m_bVisible == false )
        return;
    DXUT_CONTROL_STATE iState = DXUT_STATE_NORMAL;

    if( m_bVisible == false )
        iState = DXUT_STATE_HIDDEN;
    else if( m_bEnabled == false )
        iState = DXUT_STATE_DISABLED;
    else if( m_bPressed )
        iState = DXUT_STATE_PRESSED;
    else if( m_bMouseOver )
        iState = DXUT_STATE_MOUSEOVER;
    else if( m_bHasFocus )
        iState = DXUT_STATE_FOCUS;

    auto pElement = m_Elements[ 0 ];

    float fBlendRate = ( iState == DXUT_STATE_PRESSED ) ? 0.0f : 0.8f;

    pElement->TextureColor.Blend( iState, fElapsedTime, fBlendRate );
    pElement->FontColor.Blend( iState, fElapsedTime, fBlendRate );

    m_pDialog->DrawSprite( pElement, &m_rcButton, DXUT_NEAR_BUTTON_DEPTH );
    m_pDialog->DrawText( m_strText, pElement, &m_rcText, false, false );

    if( !m_bChecked )
        iState = DXUT_STATE_HIDDEN;

    pElement = m_Elements[ 1 ];

    pElement->TextureColor.Blend( iState, fElapsedTime, fBlendRate );
    m_pDialog->DrawSprite( pElement, &m_rcButton, DXUT_FAR_BUTTON_DEPTH );
}


//======================================================================================
// CDXUTRadioButton class
//======================================================================================

CDXUTRadioButton::CDXUTRadioButton( _In_opt_ CDXUTDialog* pDialog )
{
    m_Type = DXUT_CONTROL_RADIOBUTTON;
    m_pDialog = pDialog;
}



//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTRadioButton::HandleKeyboard( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER(lParam);

    if( !m_bEnabled || !m_bVisible )
        return false;

    switch( uMsg )
    {
        case WM_KEYDOWN:
        {
            switch( wParam )
            {
                case VK_SPACE:
                    m_bPressed = true;
                    return true;
            }
        }

        case WM_KEYUP:
        {
            switch( wParam )
            {
                case VK_SPACE:
                    if( m_bPressed == true )
                    {
                        m_bPressed = false;

                        m_pDialog->ClearRadioButtonGroup( m_nButtonGroup );
                        m_bChecked = !m_bChecked;

                        m_pDialog->SendEvent( EVENT_RADIOBUTTON_CHANGED, true, this );
                    }
                    return true;
            }
        }
    }
    return false;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTRadioButton::HandleMouse( UINT uMsg, const POINT& pt, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    if( !m_bEnabled || !m_bVisible )
        return false;

    switch( uMsg )
    {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            {
                if( ContainsPoint( pt ) )
                {
                    // Pressed while inside the control
                    m_bPressed = true;
                    SetCapture( DXUTGetHWND() );

                    if( !m_bHasFocus )
                        m_pDialog->RequestFocus( this );

                    return true;
                }

                break;
            }

        case WM_LBUTTONUP:
        {
            if( m_bPressed )
            {
                m_bPressed = false;
                ReleaseCapture();

                // Button click
                if( ContainsPoint( pt ) )
                {
                    m_pDialog->ClearRadioButtonGroup( m_nButtonGroup );
                    m_bChecked = !m_bChecked;

                    m_pDialog->SendEvent( EVENT_RADIOBUTTON_CHANGED, true, this );
                }

                return true;
            }

            break;
        }
    };

    return false;
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTRadioButton::SetCheckedInternal( bool bChecked, bool bClearGroup, bool bFromInput )
{
    if( bChecked && bClearGroup )
        m_pDialog->ClearRadioButtonGroup( m_nButtonGroup );

    m_bChecked = bChecked;
    m_pDialog->SendEvent( EVENT_RADIOBUTTON_CHANGED, bFromInput, this );
}


//======================================================================================
// CDXUTComboBox class
//======================================================================================

CDXUTComboBox::CDXUTComboBox( _In_opt_ CDXUTDialog* pDialog ) : m_ScrollBar( pDialog )
{
    m_Type = DXUT_CONTROL_COMBOBOX;
    m_pDialog = pDialog;

    m_nDropHeight = 100;

    m_nSBWidth = 16;
    m_bOpened = false;
    m_iSelected = -1;
    m_iFocused = -1;
}


//--------------------------------------------------------------------------------------
CDXUTComboBox::~CDXUTComboBox()
{
    RemoveAllItems();
}


//--------------------------------------------------------------------------------------
void CDXUTComboBox::SetTextColor( _In_ DWORD Color )
{
    auto pElement = m_Elements[ 0 ];

    if( pElement )
        pElement->FontColor.States[DXUT_STATE_NORMAL] = Color;

    pElement = m_Elements[ 2 ];

    if( pElement )
        pElement->FontColor.States[DXUT_STATE_NORMAL] = Color;
}


//--------------------------------------------------------------------------------------
void CDXUTComboBox::UpdateRects()
{

    CDXUTButton::UpdateRects();

    m_rcButton = m_rcBoundingBox;
    m_rcButton.left = m_rcButton.right - RectHeight( m_rcButton );

    m_rcText = m_rcBoundingBox;
    m_rcText.right = m_rcButton.left;

    m_rcDropdown = m_rcText;
    OffsetRect( &m_rcDropdown, 0, static_cast<int>( 0.90f * RectHeight( m_rcText ) ) );
    m_rcDropdown.bottom += m_nDropHeight;
    m_rcDropdown.right -= m_nSBWidth;

    m_rcDropdownText = m_rcDropdown;
    m_rcDropdownText.left += static_cast<int>(0.1f * RectWidth(m_rcDropdown));
    m_rcDropdownText.right -= static_cast<int>(0.1f * RectWidth(m_rcDropdown));
    m_rcDropdownText.top += static_cast<int>(0.1f * RectHeight(m_rcDropdown));
    m_rcDropdownText.bottom -= static_cast<int>(0.1f * RectHeight(m_rcDropdown));

    // Update the scrollbar's rects
    m_ScrollBar.SetLocation( m_rcDropdown.right, m_rcDropdown.top + 2 );
    m_ScrollBar.SetSize( m_nSBWidth, RectHeight( m_rcDropdown ) - 2 );
    auto pFontNode = m_pDialog->GetManager()->GetFontNode( m_Elements[ 2 ]->iFont );
    if( pFontNode && pFontNode->nHeight )
    {
        m_ScrollBar.SetPageSize( RectHeight( m_rcDropdownText ) / pFontNode->nHeight );

        // The selected item may have been scrolled off the page.
        // Ensure that it is in page again.
        m_ScrollBar.ShowItem( m_iSelected );
    }
}


//--------------------------------------------------------------------------------------
void CDXUTComboBox::OnFocusOut()
{
    CDXUTButton::OnFocusOut();

    m_bOpened = false;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTComboBox::HandleKeyboard( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    const DWORD REPEAT_MASK = ( 0x40000000 );

    if( !m_bEnabled || !m_bVisible )
        return false;

    // Let the scroll bar have a chance to handle it first
    if( m_ScrollBar.HandleKeyboard( uMsg, wParam, lParam ) )
        return true;

    switch( uMsg )
    {
        case WM_KEYDOWN:
        {
            switch( wParam )
            {
                case VK_RETURN:
                    if( m_bOpened )
                    {
                        if( m_iSelected != m_iFocused )
                        {
                            m_iSelected = m_iFocused;
                            m_pDialog->SendEvent( EVENT_COMBOBOX_SELECTION_CHANGED, true, this );
                        }
                        m_bOpened = false;

                        if( !m_pDialog->m_bKeyboardInput )
                            m_pDialog->ClearFocus();

                        return true;
                    }
                    break;

                case VK_F4:
                    // Filter out auto-repeats
                    if( lParam & REPEAT_MASK )
                        return true;

                    m_bOpened = !m_bOpened;

                    if( !m_bOpened )
                    {
                        m_pDialog->SendEvent( EVENT_COMBOBOX_SELECTION_CHANGED, true, this );

                        if( !m_pDialog->m_bKeyboardInput )
                            m_pDialog->ClearFocus();
                    }

                    return true;

                case VK_LEFT:
                case VK_UP:
                    if( m_iFocused > 0 )
                    {
                        m_iFocused--;
                        m_iSelected = m_iFocused;

                        if( !m_bOpened )
                            m_pDialog->SendEvent( EVENT_COMBOBOX_SELECTION_CHANGED, true, this );
                    }

                    return true;

                case VK_RIGHT:
                case VK_DOWN:
                    if( m_iFocused + 1 < ( int )GetNumItems() )
                    {
                        m_iFocused++;
                        m_iSelected = m_iFocused;

                        if( !m_bOpened )
                            m_pDialog->SendEvent( EVENT_COMBOBOX_SELECTION_CHANGED, true, this );
                    }

                    return true;
            }
            break;
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTComboBox::HandleMouse( UINT uMsg, const POINT& pt, WPARAM wParam, LPARAM lParam )
{
    if( !m_bEnabled || !m_bVisible )
        return false;

    // Let the scroll bar handle it first.
    if( m_ScrollBar.HandleMouse( uMsg, pt, wParam, lParam ) )
        return true;

    switch( uMsg )
    {
        case WM_MOUSEMOVE:
        {
            if( m_bOpened && PtInRect( &m_rcDropdown, pt ) )
            {
                // Determine which item has been selected
                for( size_t i = 0; i < m_Items.size(); i++ )
                {
                    auto pItem = m_Items[ i ];
                    if( pItem->bVisible &&
                        PtInRect( &pItem->rcActive, pt ) )
                    {
                        m_iFocused = static_cast<int>( i );
                    }
                }
                return true;
            }
            break;
        }

        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            {
                if( ContainsPoint( pt ) )
                {
                    // Pressed while inside the control
                    m_bPressed = true;
                    SetCapture( DXUTGetHWND() );

                    if( !m_bHasFocus )
                        m_pDialog->RequestFocus( this );

                    // Toggle dropdown
                    if( m_bHasFocus )
                    {
                        m_bOpened = !m_bOpened;

                        if( !m_bOpened )
                        {
                            if( !m_pDialog->m_bKeyboardInput )
                                m_pDialog->ClearFocus();
                        }
                    }

                    return true;
                }

                // Perhaps this click is within the dropdown
                if( m_bOpened && PtInRect( &m_rcDropdown, pt ) )
                {
                    // Determine which item has been selected
                    for( size_t i = m_ScrollBar.GetTrackPos(); i < m_Items.size(); i++ )
                    {
                        auto pItem = m_Items[ i ];
                        if( pItem->bVisible &&
                            PtInRect( &pItem->rcActive, pt ) )
                        {
                            m_iFocused = m_iSelected = static_cast<int>( i );
                            m_pDialog->SendEvent( EVENT_COMBOBOX_SELECTION_CHANGED, true, this );
                            m_bOpened = false;

                            if( !m_pDialog->m_bKeyboardInput )
                                m_pDialog->ClearFocus();

                            break;
                        }
                    }

                    return true;
                }

                // Mouse click not on main control or in dropdown, fire an event if needed
                if( m_bOpened )
                {
                    m_iFocused = m_iSelected;

                    m_pDialog->SendEvent( EVENT_COMBOBOX_SELECTION_CHANGED, true, this );
                    m_bOpened = false;
                }

                // Make sure the control is no longer in a pressed state
                m_bPressed = false;

                // Release focus if appropriate
                if( !m_pDialog->m_bKeyboardInput )
                {
                    m_pDialog->ClearFocus();
                }

                break;
            }

        case WM_LBUTTONUP:
        {
            if( m_bPressed && ContainsPoint( pt ) )
            {
                // Button click
                m_bPressed = false;
                ReleaseCapture();
                return true;
            }

            break;
        }

        case WM_MOUSEWHEEL:
        {
            int zDelta = ( short )HIWORD( wParam ) / WHEEL_DELTA;
            if( m_bOpened )
            {
                UINT uLines = 0;
                if ( !SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &uLines, 0) )
                    uLines = 0;
                m_ScrollBar.Scroll( -zDelta * uLines );
            }
            else
            {
                if( zDelta > 0 )
                {
                    if( m_iFocused > 0 )
                    {
                        m_iFocused--;
                        m_iSelected = m_iFocused;

                        if( !m_bOpened )
                            m_pDialog->SendEvent( EVENT_COMBOBOX_SELECTION_CHANGED, true, this );
                    }
                }
                else
                {
                    if( m_iFocused + 1 < ( int )GetNumItems() )
                    {
                        m_iFocused++;
                        m_iSelected = m_iFocused;

                        if( !m_bOpened )
                            m_pDialog->SendEvent( EVENT_COMBOBOX_SELECTION_CHANGED, true, this );
                    }
                }
            }
            return true;
        }
    };

    return false;
}


//--------------------------------------------------------------------------------------
void CDXUTComboBox::OnHotkey()
{
    if( m_bOpened )
        return;

    if( m_iSelected == -1 )
        return;

    if( m_pDialog->IsKeyboardInputEnabled() )
        m_pDialog->RequestFocus( this );

    m_iSelected++;

    if( m_iSelected >= ( int )m_Items.size() )
        m_iSelected = 0;

    m_iFocused = m_iSelected;
    m_pDialog->SendEvent( EVENT_COMBOBOX_SELECTION_CHANGED, true, this );
}


//--------------------------------------------------------------------------------------
void CDXUTComboBox::Render( _In_ float fElapsedTime )
{
    if( m_bVisible == false )
        return;

    DXUT_CONTROL_STATE iState = DXUT_STATE_NORMAL;

    if( !m_bOpened )
        iState = DXUT_STATE_HIDDEN;

    // Dropdown box
    auto pElement = m_Elements[ 2 ];

    // If we have not initialized the scroll bar page size,
    // do that now.
    static bool bSBInit;
    if( !bSBInit )
    {
        // Update the page size of the scroll bar
        if( m_pDialog->GetManager()->GetFontNode( pElement->iFont )->nHeight )
            m_ScrollBar.SetPageSize( RectHeight( m_rcDropdownText ) /
                                     m_pDialog->GetManager()->GetFontNode( pElement->iFont )->nHeight );
        else
            m_ScrollBar.SetPageSize( RectHeight( m_rcDropdownText ) );
        bSBInit = true;
    }

    // Scroll bar
    if( m_bOpened )
        m_ScrollBar.Render( fElapsedTime );

    // Blend current color
    pElement->TextureColor.Blend( iState, fElapsedTime );
    pElement->FontColor.Blend( iState, fElapsedTime );

    m_pDialog->DrawSprite( pElement, &m_rcDropdown, DXUT_NEAR_BUTTON_DEPTH );

    // Selection outline
    auto pSelectionElement = m_Elements[ 3 ];
    pSelectionElement->TextureColor.Current = pElement->TextureColor.Current;
    pSelectionElement->FontColor.SetCurrent( pSelectionElement->FontColor.States[ DXUT_STATE_NORMAL ] );

    auto pFont = m_pDialog->GetFont( pElement->iFont );
    if( pFont )
    {
        int curY = m_rcDropdownText.top;
        int nRemainingHeight = RectHeight( m_rcDropdownText );
        
        for( size_t i = m_ScrollBar.GetTrackPos(); i < m_Items.size(); i++ )
        {
            auto pItem = m_Items[ i ];

            // Make sure there's room left in the dropdown
            nRemainingHeight -= pFont->nHeight;
            if( nRemainingHeight < 0 )
            {
                pItem->bVisible = false;
                continue;
            }

            SetRect( &pItem->rcActive, m_rcDropdownText.left, curY, m_rcDropdownText.right, curY + pFont->nHeight );
            curY += pFont->nHeight;

            pItem->bVisible = true;

            if( m_bOpened )
            {
                if( ( int )i == m_iFocused )
                {
                    RECT rc;
                    SetRect( &rc, m_rcDropdown.left, pItem->rcActive.top, m_rcDropdown.right,
                             pItem->rcActive.bottom + 2 );
                    m_pDialog->DrawSprite( pSelectionElement, &rc, DXUT_NEAR_BUTTON_DEPTH );
                    m_pDialog->DrawText( pItem->strText, pSelectionElement, &pItem->rcActive );
                }
                else
                {
                    m_pDialog->DrawText( pItem->strText, pElement, &pItem->rcActive );
                }
            }
        }
    }

    int nOffsetX = 0;
    int nOffsetY = 0;

    iState = DXUT_STATE_NORMAL;

    if( m_bVisible == false )
        iState = DXUT_STATE_HIDDEN;
    else if( m_bEnabled == false )
        iState = DXUT_STATE_DISABLED;
    else if( m_bPressed )
    {
        iState = DXUT_STATE_PRESSED;

        nOffsetX = 1;
        nOffsetY = 2;
    }
    else if( m_bMouseOver )
    {
        iState = DXUT_STATE_MOUSEOVER;

        nOffsetX = -1;
        nOffsetY = -2;
    }
    else if( m_bHasFocus )
        iState = DXUT_STATE_FOCUS;

    float fBlendRate = ( iState == DXUT_STATE_PRESSED ) ? 0.0f : 0.8f;

    // Button
    pElement = m_Elements[ 1 ];

    // Blend current color
    pElement->TextureColor.Blend( iState, fElapsedTime, fBlendRate );

    RECT rcWindow = m_rcButton;
    OffsetRect( &rcWindow, nOffsetX, nOffsetY );
    m_pDialog->DrawSprite( pElement, &rcWindow, DXUT_FAR_BUTTON_DEPTH );

    if( m_bOpened )
        iState = DXUT_STATE_PRESSED;

    // Main text box
    pElement = m_Elements[ 0 ];

    // Blend current color
    pElement->TextureColor.Blend( iState, fElapsedTime, fBlendRate );
    pElement->FontColor.Blend( iState, fElapsedTime, fBlendRate );

    m_pDialog->DrawSprite( pElement, &m_rcText, DXUT_NEAR_BUTTON_DEPTH );

    if( m_iSelected >= 0 && m_iSelected < ( int )m_Items.size() )
    {
        auto pItem = m_Items[ m_iSelected ];
        if( pItem )
        {
            m_pDialog->DrawText( pItem->strText, pElement, &m_rcText, false, true );

        }
    }
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTComboBox::AddItem( const WCHAR* strText, void* pData )
{
    // Validate parameters
    if( !strText )
    {
        return E_INVALIDARG;
    }

    // Create a new item and set the data
    auto pItem = new (std::nothrow) DXUTComboBoxItem;
    if( !pItem )
    {
        return DXTRACE_ERR_MSGBOX( L"new", E_OUTOFMEMORY );
    }

    ZeroMemory( pItem, sizeof( DXUTComboBoxItem ) );
    wcscpy_s( pItem->strText, 256, strText );
    pItem->pData = pData;

    m_Items.push_back( pItem );

    // Update the scroll bar with new range
    m_ScrollBar.SetTrackRange( 0, (int)m_Items.size() );

    // If this is the only item in the list, it's selected
    if( GetNumItems() == 1 )
    {
        m_iSelected = 0;
        m_iFocused = 0;
        m_pDialog->SendEvent( EVENT_COMBOBOX_SELECTION_CHANGED, false, this );
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
void CDXUTComboBox::RemoveItem( _In_ UINT index )
{
    auto it = m_Items.begin() + index;
    auto pItem = *it;
    SAFE_DELETE( pItem );
    m_Items.erase( it );
    m_ScrollBar.SetTrackRange( 0, (int)m_Items.size() );
    if( m_iSelected >= (int)m_Items.size() )
        m_iSelected = (int)m_Items.size() - 1;
}


//--------------------------------------------------------------------------------------
void CDXUTComboBox::RemoveAllItems()
{
    for( auto it = m_Items.begin(); it != m_Items.end(); ++it )
    {
        auto pItem = *it;
        SAFE_DELETE( pItem );
    }

    m_Items.clear();
    m_ScrollBar.SetTrackRange( 0, 1 );
    m_iFocused = m_iSelected = -1;
}


//--------------------------------------------------------------------------------------
bool CDXUTComboBox::ContainsItem( _In_z_ const WCHAR* strText, _In_ UINT iStart )
{
    return ( -1 != FindItem( strText, iStart ) );
}


//--------------------------------------------------------------------------------------
int CDXUTComboBox::FindItem( _In_z_ const WCHAR* strText, _In_ UINT iStart ) const
{
    if( !strText )
        return -1;

    for( size_t i = iStart; i < m_Items.size(); i++ )
    {
        auto pItem = m_Items[ i ];

        if( 0 == wcscmp( pItem->strText, strText ) )
        {
            return static_cast<int>( i );
        }
    }

    return -1;
}


//--------------------------------------------------------------------------------------
void* CDXUTComboBox::GetSelectedData() const
{
    if( m_iSelected < 0 )
        return nullptr;

    auto pItem = m_Items[ m_iSelected ];
    return pItem->pData;
}


//--------------------------------------------------------------------------------------
DXUTComboBoxItem* CDXUTComboBox::GetSelectedItem() const
{
    if( m_iSelected < 0 )
        return nullptr;

    return m_Items[ m_iSelected ];
}


//--------------------------------------------------------------------------------------
void* CDXUTComboBox::GetItemData( _In_z_ const WCHAR* strText ) const
{
    int index = FindItem( strText );
    if( index == -1 )
    {
        return nullptr;
    }

    auto pItem = m_Items[ index ];
    if( !pItem )
    {
        DXTRACE_ERR( L"CDXUTComboBox::GetItemData", E_FAIL );
        return nullptr;
    }

    return pItem->pData;
}


//--------------------------------------------------------------------------------------
void* CDXUTComboBox::GetItemData( _In_ int nIndex ) const
{
    if( nIndex < 0 || nIndex >= (int)m_Items.size() )
        return nullptr;

    return m_Items[ nIndex ]->pData;
}


//--------------------------------------------------------------------------------------
HRESULT CDXUTComboBox::SetSelectedByIndex( _In_ UINT index )
{
    if( index >= GetNumItems() )
        return E_INVALIDARG;

    m_iFocused = m_iSelected = index;
    m_pDialog->SendEvent( EVENT_COMBOBOX_SELECTION_CHANGED, false, this );

    return S_OK;
}



//--------------------------------------------------------------------------------------
HRESULT CDXUTComboBox::SetSelectedByText( _In_z_  const WCHAR* strText )
{
    if( !strText )
        return E_INVALIDARG;

    int index = FindItem( strText );
    if( index == -1 )
        return E_FAIL;

    m_iFocused = m_iSelected = index;
    m_pDialog->SendEvent( EVENT_COMBOBOX_SELECTION_CHANGED, false, this );

    return S_OK;
}



//--------------------------------------------------------------------------------------
HRESULT CDXUTComboBox::SetSelectedByData( _In_ void* pData )
{
    for( size_t i = 0; i < m_Items.size(); i++ )
    {
        auto pItem = m_Items[ i ];

        if( pItem->pData == pData )
        {
            m_iFocused = m_iSelected = static_cast<int>( i );
            m_pDialog->SendEvent( EVENT_COMBOBOX_SELECTION_CHANGED, false, this );
            return S_OK;
        }
    }

    return E_FAIL;
}


//======================================================================================
// CDXUTSlider class
//======================================================================================

CDXUTSlider::CDXUTSlider( _In_opt_ CDXUTDialog* pDialog )
{
    m_Type = DXUT_CONTROL_SLIDER;
    m_pDialog = pDialog;

    m_nMin = 0;
    m_nMax = 100;
    m_nValue = 50;

    m_bPressed = false;
}


//--------------------------------------------------------------------------------------
bool CDXUTSlider::ContainsPoint( _In_ const POINT& pt )
{
    return ( PtInRect( &m_rcBoundingBox, pt ) ||
             PtInRect( &m_rcButton, pt ) );
}


//--------------------------------------------------------------------------------------
void CDXUTSlider::UpdateRects()
{
    CDXUTControl::UpdateRects();

    m_rcButton = m_rcBoundingBox;
    m_rcButton.right = m_rcButton.left + RectHeight( m_rcButton );
    OffsetRect( &m_rcButton, -RectWidth( m_rcButton ) / 2, 0 );

    m_nButtonX = ( int )( ( m_nValue - m_nMin ) * ( float )RectWidth( m_rcBoundingBox ) / ( m_nMax - m_nMin ) );
    OffsetRect( &m_rcButton, m_nButtonX, 0 );
}


//--------------------------------------------------------------------------------------
int CDXUTSlider::ValueFromPos( _In_ int x )
{
    float fValuePerPixel = ( float )( m_nMax - m_nMin ) / RectWidth( m_rcBoundingBox );
    return ( int )( 0.5f + m_nMin + fValuePerPixel * ( x - m_rcBoundingBox.left ) );
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTSlider::HandleKeyboard( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER(lParam);

    if( !m_bEnabled || !m_bVisible )
        return false;

    switch( uMsg )
    {
        case WM_KEYDOWN:
        {
            switch( wParam )
            {
                case VK_HOME:
                    SetValueInternal( m_nMin, true );
                    return true;

                case VK_END:
                    SetValueInternal( m_nMax, true );
                    return true;

                case VK_LEFT:
                case VK_DOWN:
                    SetValueInternal( m_nValue - 1, true );
                    return true;

                case VK_RIGHT:
                case VK_UP:
                    SetValueInternal( m_nValue + 1, true );
                    return true;

                case VK_NEXT:
                    SetValueInternal( m_nValue - ( 10 > ( m_nMax - m_nMin ) / 10 ? 10 : ( m_nMax - m_nMin ) / 10 ),
                                      true );
                    return true;

                case VK_PRIOR:
                    SetValueInternal( m_nValue + ( 10 > ( m_nMax - m_nMin ) / 10 ? 10 : ( m_nMax - m_nMin ) / 10 ),
                                      true );
                    return true;
            }
            break;
        }
    }


    return false;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTSlider::HandleMouse( UINT uMsg, const POINT& pt, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER(lParam);

    if( !m_bEnabled || !m_bVisible )
        return false;

    switch( uMsg )
    {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            {
                if( PtInRect( &m_rcButton, pt ) )
                {
                    // Pressed while inside the control
                    m_bPressed = true;
                    SetCapture( DXUTGetHWND() );

                    m_nDragX = pt.x;
                    //m_nDragY = pt.y;
                    m_nDragOffset = m_nButtonX - m_nDragX;

                    //m_nDragValue = m_nValue;

                    if( !m_bHasFocus )
                        m_pDialog->RequestFocus( this );

                    return true;
                }

                if( PtInRect( &m_rcBoundingBox, pt ) )
                {
                    m_nDragX = pt.x;
                    m_nDragOffset = 0;
                    m_bPressed = true;

                    if( !m_bHasFocus )
                        m_pDialog->RequestFocus( this );

                    if( pt.x > m_nButtonX + m_x )
                    {
                        SetValueInternal( m_nValue + 1, true );
                        return true;
                    }

                    if( pt.x < m_nButtonX + m_x )
                    {
                        SetValueInternal( m_nValue - 1, true );
                        return true;
                    }
                }

                break;
            }

        case WM_LBUTTONUP:
        {
            if( m_bPressed )
            {
                m_bPressed = false;
                ReleaseCapture();
                m_pDialog->SendEvent( EVENT_SLIDER_VALUE_CHANGED_UP, true, this );

                return true;
            }

            break;
        }

        case WM_MOUSEMOVE:
        {
            if( m_bPressed )
            {
                SetValueInternal( ValueFromPos( m_x + pt.x + m_nDragOffset ), true );
                return true;
            }

            break;
        }

        case WM_MOUSEWHEEL:
        {
            int nScrollAmount = int( ( short )HIWORD( wParam ) ) / WHEEL_DELTA;
            SetValueInternal( m_nValue - nScrollAmount, true );
            return true;
        }
    };

    return false;
}


//--------------------------------------------------------------------------------------
void CDXUTSlider::SetRange( _In_ int nMin, _In_ int nMax )
{
    m_nMin = nMin;
    m_nMax = nMax;

    SetValueInternal( m_nValue, false );
}


//--------------------------------------------------------------------------------------
void CDXUTSlider::SetValueInternal( _In_ int nValue, _In_ bool bFromInput )
{
    // Clamp to range
    nValue = std::max( m_nMin, nValue );
    nValue = std::min( m_nMax, nValue );

    if( nValue == m_nValue )
        return;

    m_nValue = nValue;
    UpdateRects();

    m_pDialog->SendEvent( EVENT_SLIDER_VALUE_CHANGED, bFromInput, this );
}


//--------------------------------------------------------------------------------------
void CDXUTSlider::Render( _In_ float fElapsedTime )
{
    if( m_bVisible == false )
        return;

    int nOffsetX = 0;
    int nOffsetY = 0;

    DXUT_CONTROL_STATE iState = DXUT_STATE_NORMAL;

    if( m_bVisible == false )
    {
        iState = DXUT_STATE_HIDDEN;
    }
    else if( m_bEnabled == false )
    {
        iState = DXUT_STATE_DISABLED;
    }
    else if( m_bPressed )
    {
        iState = DXUT_STATE_PRESSED;

        nOffsetX = 1;
        nOffsetY = 2;
    }
    else if( m_bMouseOver )
    {
        iState = DXUT_STATE_MOUSEOVER;

        nOffsetX = -1;
        nOffsetY = -2;
    }
    else if( m_bHasFocus )
    {
        iState = DXUT_STATE_FOCUS;
    }

    float fBlendRate = ( iState == DXUT_STATE_PRESSED ) ? 0.0f : 0.8f;

    auto pElement = m_Elements[ 0 ];

    // Blend current color
    pElement->TextureColor.Blend( iState, fElapsedTime, fBlendRate );
    m_pDialog->DrawSprite( pElement, &m_rcBoundingBox, DXUT_FAR_BUTTON_DEPTH );

    pElement = m_Elements[ 1 ];

    // Blend current color
    pElement->TextureColor.Blend( iState, fElapsedTime, fBlendRate );
    m_pDialog->DrawSprite( pElement, &m_rcButton, DXUT_NEAR_BUTTON_DEPTH );
}


//======================================================================================
// CDXUTScrollBar class
//======================================================================================

CDXUTScrollBar::CDXUTScrollBar( _In_opt_ CDXUTDialog* pDialog )
{
    m_Type = DXUT_CONTROL_SCROLLBAR;
    m_pDialog = pDialog;

    m_bShowThumb = true;
    m_bDrag = false;

    SetRect( &m_rcUpButton, 0, 0, 0, 0 );
    SetRect( &m_rcDownButton, 0, 0, 0, 0 );
    SetRect( &m_rcTrack, 0, 0, 0, 0 );
    SetRect( &m_rcThumb, 0, 0, 0, 0 );
    m_nPosition = 0;
    m_nPageSize = 1;
    m_nStart = 0;
    m_nEnd = 1;
    m_Arrow = CLEAR;
    m_dArrowTS = 0.0;
}


//--------------------------------------------------------------------------------------
CDXUTScrollBar::~CDXUTScrollBar()
{
}


//--------------------------------------------------------------------------------------
void CDXUTScrollBar::UpdateRects()
{
    CDXUTControl::UpdateRects();

    // Make the buttons square

    SetRect( &m_rcUpButton, m_rcBoundingBox.left, m_rcBoundingBox.top,
             m_rcBoundingBox.right, m_rcBoundingBox.top + RectWidth( m_rcBoundingBox ) );
    SetRect( &m_rcDownButton, m_rcBoundingBox.left, m_rcBoundingBox.bottom - RectWidth( m_rcBoundingBox ),
             m_rcBoundingBox.right, m_rcBoundingBox.bottom );
    SetRect( &m_rcTrack, m_rcUpButton.left, m_rcUpButton.bottom,
             m_rcDownButton.right, m_rcDownButton.top );
    m_rcThumb.left = m_rcUpButton.left;
    m_rcThumb.right = m_rcUpButton.right;

    UpdateThumbRect();
}


//--------------------------------------------------------------------------------------
// Compute the dimension of the scroll thumb
void CDXUTScrollBar::UpdateThumbRect()
{
    if( m_nEnd - m_nStart > m_nPageSize )
    {
        int nThumbHeight = std::max( RectHeight( m_rcTrack ) * m_nPageSize / ( m_nEnd - m_nStart ),
                                  SCROLLBAR_MINTHUMBSIZE );
        int nMaxPosition = m_nEnd - m_nStart - m_nPageSize;
        m_rcThumb.top = m_rcTrack.top + ( m_nPosition - m_nStart ) * ( RectHeight( m_rcTrack ) - nThumbHeight )
            / nMaxPosition;
        m_rcThumb.bottom = m_rcThumb.top + nThumbHeight;
        m_bShowThumb = true;

    }
    else
    {
        // No content to scroll
        m_rcThumb.bottom = m_rcThumb.top;
        m_bShowThumb = false;
    }
}


//--------------------------------------------------------------------------------------
// Scroll() scrolls by nDelta items.  A positive value scrolls down, while a negative
// value scrolls up.
void CDXUTScrollBar::Scroll( _In_ int nDelta )
{
    // Perform scroll
    m_nPosition += nDelta;

    // Cap position
    Cap();

    // Update thumb position
    UpdateThumbRect();
}


//--------------------------------------------------------------------------------------
void CDXUTScrollBar::ShowItem( _In_ int nIndex )
{
    // Cap the index

    if( nIndex < 0 )
        nIndex = 0;

    if( nIndex >= m_nEnd )
        nIndex = m_nEnd - 1;

    // Adjust position

    if( m_nPosition > nIndex )
        m_nPosition = nIndex;
    else if( m_nPosition + m_nPageSize <= nIndex )
        m_nPosition = nIndex - m_nPageSize + 1;

    UpdateThumbRect();
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTScrollBar::HandleKeyboard( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    return false;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTScrollBar::HandleMouse( UINT uMsg, const POINT& pt, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    static int ThumbOffsetY;

    m_LastMouse = pt;
    switch( uMsg )
    {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            {
                // Check for click on up button

                if( PtInRect( &m_rcUpButton, pt ) )
                {
                    SetCapture( DXUTGetHWND() );
                    if( m_nPosition > m_nStart )
                        --m_nPosition;
                    UpdateThumbRect();
                    m_Arrow = CLICKED_UP;
                    m_dArrowTS = DXUTGetTime();
                    return true;
                }

                // Check for click on down button

                if( PtInRect( &m_rcDownButton, pt ) )
                {
                    SetCapture( DXUTGetHWND() );
                    if( m_nPosition + m_nPageSize <= m_nEnd )
                        ++m_nPosition;
                    UpdateThumbRect();
                    m_Arrow = CLICKED_DOWN;
                    m_dArrowTS = DXUTGetTime();
                    return true;
                }

                // Check for click on thumb

                if( PtInRect( &m_rcThumb, pt ) )
                {
                    SetCapture( DXUTGetHWND() );
                    m_bDrag = true;
                    ThumbOffsetY = pt.y - m_rcThumb.top;
                    return true;
                }

                // Check for click on track

                if( m_rcThumb.left <= pt.x &&
                    m_rcThumb.right > pt.x )
                {
                    SetCapture( DXUTGetHWND() );
                    if( m_rcThumb.top > pt.y &&
                        m_rcTrack.top <= pt.y )
                    {
                        Scroll( -( m_nPageSize - 1 ) );
                        return true;
                    }
                    else if( m_rcThumb.bottom <= pt.y &&
                             m_rcTrack.bottom > pt.y )
                    {
                        Scroll( m_nPageSize - 1 );
                        return true;
                    }
                }

                break;
            }

        case WM_LBUTTONUP:
        {
            m_bDrag = false;
            ReleaseCapture();
            UpdateThumbRect();
            m_Arrow = CLEAR;
            break;
        }

        case WM_MOUSEMOVE:
        {
            if( m_bDrag )
            {
                m_rcThumb.bottom += pt.y - ThumbOffsetY - m_rcThumb.top;
                m_rcThumb.top = pt.y - ThumbOffsetY;
                if( m_rcThumb.top < m_rcTrack.top )
                    OffsetRect( &m_rcThumb, 0, m_rcTrack.top - m_rcThumb.top );
                else if( m_rcThumb.bottom > m_rcTrack.bottom )
                    OffsetRect( &m_rcThumb, 0, m_rcTrack.bottom - m_rcThumb.bottom );

                // Compute first item index based on thumb position

                int nMaxFirstItem = m_nEnd - m_nStart - m_nPageSize + 1;  // Largest possible index for first item
                int nMaxThumb = RectHeight( m_rcTrack ) - RectHeight( m_rcThumb );  // Largest possible thumb position from the top

                m_nPosition = m_nStart +
                    ( m_rcThumb.top - m_rcTrack.top +
                      nMaxThumb / ( nMaxFirstItem * 2 ) ) * // Shift by half a row to avoid last row covered by only one pixel
                    nMaxFirstItem / nMaxThumb;

                return true;
            }

            break;
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTScrollBar::MsgProc( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER(wParam);

    if( WM_CAPTURECHANGED == uMsg )
    {
        // The application just lost mouse capture. We may not have gotten
        // the WM_MOUSEUP message, so reset m_bDrag here.
        if( ( HWND )lParam != DXUTGetHWND() )
            m_bDrag = false;
    }

    return false;
}


//--------------------------------------------------------------------------------------
void CDXUTScrollBar::Render( _In_ float fElapsedTime )
{
    if( m_bVisible == false )
        return;

    // Check if the arrow button has been held for a while.
    // If so, update the thumb position to simulate repeated
    // scroll.
    if( m_Arrow != CLEAR )
    {
        double dCurrTime = DXUTGetTime();
        if( PtInRect( &m_rcUpButton, m_LastMouse ) )
        {
            switch( m_Arrow )
            {
                case CLICKED_UP:
                    if( SCROLLBAR_ARROWCLICK_DELAY < dCurrTime - m_dArrowTS )
                    {
                        Scroll( -1 );
                        m_Arrow = HELD_UP;
                        m_dArrowTS = dCurrTime;
                    }
                    break;
                case HELD_UP:
                    if( SCROLLBAR_ARROWCLICK_REPEAT < dCurrTime - m_dArrowTS )
                    {
                        Scroll( -1 );
                        m_dArrowTS = dCurrTime;
                    }
                    break;
            }
        }
        else if( PtInRect( &m_rcDownButton, m_LastMouse ) )
        {
            switch( m_Arrow )
            {
                case CLICKED_DOWN:
                    if( SCROLLBAR_ARROWCLICK_DELAY < dCurrTime - m_dArrowTS )
                    {
                        Scroll( 1 );
                        m_Arrow = HELD_DOWN;
                        m_dArrowTS = dCurrTime;
                    }
                    break;
                case HELD_DOWN:
                    if( SCROLLBAR_ARROWCLICK_REPEAT < dCurrTime - m_dArrowTS )
                    {
                        Scroll( 1 );
                        m_dArrowTS = dCurrTime;
                    }
                    break;
            }
        }
    }

    DXUT_CONTROL_STATE iState = DXUT_STATE_NORMAL;

    if( m_bVisible == false )
        iState = DXUT_STATE_HIDDEN;
    else if( m_bEnabled == false || m_bShowThumb == false )
        iState = DXUT_STATE_DISABLED;
    else if( m_bMouseOver )
        iState = DXUT_STATE_MOUSEOVER;
    else if( m_bHasFocus )
        iState = DXUT_STATE_FOCUS;


    float fBlendRate = ( iState == DXUT_STATE_PRESSED ) ? 0.0f : 0.8f;

    // Background track layer
    auto pElement = m_Elements[ 0 ];

    // Blend current color
    pElement->TextureColor.Blend( iState, fElapsedTime, fBlendRate );
    m_pDialog->DrawSprite( pElement, &m_rcTrack, DXUT_FAR_BUTTON_DEPTH );

    // Up Arrow
    pElement = m_Elements[ 1 ];

    // Blend current color
    pElement->TextureColor.Blend( iState, fElapsedTime, fBlendRate );
    m_pDialog->DrawSprite( pElement, &m_rcUpButton, DXUT_NEAR_BUTTON_DEPTH );

    // Down Arrow
    pElement = m_Elements[ 2 ];

    // Blend current color
    pElement->TextureColor.Blend( iState, fElapsedTime, fBlendRate );
    m_pDialog->DrawSprite( pElement, &m_rcDownButton, DXUT_NEAR_BUTTON_DEPTH );

    // Thumb button
    pElement = m_Elements[ 3 ];

    // Blend current color
    pElement->TextureColor.Blend( iState, fElapsedTime, fBlendRate );
    m_pDialog->DrawSprite( pElement, &m_rcThumb, DXUT_NEAR_BUTTON_DEPTH );

}


//--------------------------------------------------------------------------------------
void CDXUTScrollBar::SetTrackRange( _In_ int nStart, _In_ int nEnd )
{
    m_nStart = nStart; m_nEnd = nEnd;
    Cap();
    UpdateThumbRect();
}


//--------------------------------------------------------------------------------------
void CDXUTScrollBar::Cap()  // Clips position at boundaries. Ensures it stays within legal range.
{
    if( m_nPosition < m_nStart ||
        m_nEnd - m_nStart <= m_nPageSize )
    {
        m_nPosition = m_nStart;
    }
    else if( m_nPosition + m_nPageSize > m_nEnd )
        m_nPosition = m_nEnd - m_nPageSize + 1;
}


//======================================================================================
// CDXUTListBox class
//======================================================================================

CDXUTListBox::CDXUTListBox( _In_opt_ CDXUTDialog* pDialog ) : m_ScrollBar( pDialog )
{
    m_Type = DXUT_CONTROL_LISTBOX;
    m_pDialog = pDialog;

    m_dwStyle = 0;
    m_nSBWidth = 16;
    m_nSelected = -1;
    m_nSelStart = 0;
    m_bDrag = false;
    m_nBorder = 6;
    m_nMargin = 5;
    m_nTextHeight = 0;
}


//--------------------------------------------------------------------------------------
CDXUTListBox::~CDXUTListBox()
{
    RemoveAllItems();
}


//--------------------------------------------------------------------------------------
void CDXUTListBox::UpdateRects()
{
    CDXUTControl::UpdateRects();

    m_rcSelection = m_rcBoundingBox;
    m_rcSelection.right -= m_nSBWidth;
    InflateRect( &m_rcSelection, -m_nBorder, -m_nBorder );
    m_rcText = m_rcSelection;
    InflateRect( &m_rcText, -m_nMargin, 0 );

    // Update the scrollbar's rects
    m_ScrollBar.SetLocation( m_rcBoundingBox.right - m_nSBWidth, m_rcBoundingBox.top );
    m_ScrollBar.SetSize( m_nSBWidth, m_height );
    auto pFontNode = m_pDialog->GetManager()->GetFontNode( m_Elements[ 0 ]->iFont );
    if( pFontNode && pFontNode->nHeight )
    {
        m_ScrollBar.SetPageSize( RectHeight( m_rcText ) / pFontNode->nHeight );

        // The selected item may have been scrolled off the page.
        // Ensure that it is in page again.
        m_ScrollBar.ShowItem( m_nSelected );
    }
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTListBox::AddItem( const WCHAR* wszText, void* pData )
{
    auto pNewItem = new (std::nothrow) DXUTListBoxItem;
    if( !pNewItem )
        return E_OUTOFMEMORY;

    wcscpy_s( pNewItem->strText, 256, wszText );
    pNewItem->pData = pData;
    SetRect( &pNewItem->rcActive, 0, 0, 0, 0 );
    pNewItem->bSelected = false;

    m_Items.push_back( pNewItem );
    m_ScrollBar.SetTrackRange( 0, (int)m_Items.size() );

    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTListBox::InsertItem( int nIndex, const WCHAR* wszText, void* pData )
{
    auto pNewItem = new (std::nothrow) DXUTListBoxItem;
    if( !pNewItem )
        return E_OUTOFMEMORY;

    wcscpy_s( pNewItem->strText, 256, wszText );
    pNewItem->pData = pData;
    SetRect( &pNewItem->rcActive, 0, 0, 0, 0 );
    pNewItem->bSelected = false;

    m_Items[ nIndex ] = pNewItem;
    m_ScrollBar.SetTrackRange( 0, (int)m_Items.size() );

    return S_OK;
}


//--------------------------------------------------------------------------------------
void CDXUTListBox::RemoveItem( _In_ int nIndex )
{
    if( nIndex < 0 || nIndex >= ( int )m_Items.size() )
        return;

    auto it = m_Items.begin() + nIndex;
    auto pItem = *it;
    delete pItem;
    m_Items.erase(it);
    m_ScrollBar.SetTrackRange( 0, (int)m_Items.size() );
    if( m_nSelected >= ( int )m_Items.size() )
        m_nSelected = int( m_Items.size() ) - 1;

    m_pDialog->SendEvent( EVENT_LISTBOX_SELECTION, true, this );
}


//--------------------------------------------------------------------------------------
void CDXUTListBox::RemoveAllItems()
{
    for( auto it = m_Items.begin(); it != m_Items.end(); ++it )
    {
        auto pItem = *it;
        delete pItem;
    }

    m_Items.clear();
    m_ScrollBar.SetTrackRange( 0, 1 );
}


//--------------------------------------------------------------------------------------
DXUTListBoxItem* CDXUTListBox::GetItem( _In_ int nIndex ) const
{
    if( nIndex < 0 || nIndex >= ( int )m_Items.size() )
        return nullptr;

    return m_Items[nIndex];
}


//--------------------------------------------------------------------------------------
// For single-selection listbox, returns the index of the selected item.
// For multi-selection, returns the first selected item after the nPreviousSelected position.
// To search for the first selected item, the app passes -1 for nPreviousSelected.  For
// subsequent searches, the app passes the returned index back to GetSelectedIndex as.
// nPreviousSelected.
// Returns -1 on error or if no item is selected.
int CDXUTListBox::GetSelectedIndex( _In_ int nPreviousSelected ) const
{
    if( nPreviousSelected < -1 )
        return -1;

    if( m_dwStyle & MULTISELECTION )
    {
        // Multiple selection enabled. Search for the next item with the selected flag.
        for( int i = nPreviousSelected + 1; i < ( int )m_Items.size(); ++i )
        {
            auto pItem = m_Items[ i ];

            if( pItem->bSelected )
                return i;
        }

        return -1;
    }
    else
    {
        // Single selection
        return m_nSelected;
    }
}


//--------------------------------------------------------------------------------------
void CDXUTListBox::SelectItem( _In_ int nNewIndex )
{
    // If no item exists, do nothing.
    if( m_Items.size() == 0 )
        return;

    int nOldSelected = m_nSelected;

    // Adjust m_nSelected
    m_nSelected = nNewIndex;

    // Perform capping
    if( m_nSelected < 0 )
        m_nSelected = 0;
    if( m_nSelected >= ( int )m_Items.size() )
        m_nSelected = int( m_Items.size() ) - 1;

    if( nOldSelected != m_nSelected )
    {
        if( m_dwStyle & MULTISELECTION )
        {
            m_Items[m_nSelected]->bSelected = true;
        }

        // Update selection start
        m_nSelStart = m_nSelected;

        // Adjust scroll bar
        m_ScrollBar.ShowItem( m_nSelected );
    }

    m_pDialog->SendEvent( EVENT_LISTBOX_SELECTION, true, this );
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTListBox::HandleKeyboard( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    if( !m_bEnabled || !m_bVisible )
        return false;

    // Let the scroll bar have a chance to handle it first
    if( m_ScrollBar.HandleKeyboard( uMsg, wParam, lParam ) )
        return true;

    switch( uMsg )
    {
        case WM_KEYDOWN:
            switch( wParam )
            {
                case VK_UP:
                case VK_DOWN:
                case VK_NEXT:
                case VK_PRIOR:
                case VK_HOME:
                case VK_END:
                    {
                        // If no item exists, do nothing.
                        if( m_Items.size() == 0 )
                            return true;

                        int nOldSelected = m_nSelected;

                        // Adjust m_nSelected
                        switch( wParam )
                        {
                            case VK_UP:
                                --m_nSelected; break;
                            case VK_DOWN:
                                ++m_nSelected; break;
                            case VK_NEXT:
                                m_nSelected += m_ScrollBar.GetPageSize() - 1; break;
                            case VK_PRIOR:
                                m_nSelected -= m_ScrollBar.GetPageSize() - 1; break;
                            case VK_HOME:
                                m_nSelected = 0; break;
                            case VK_END:
                                m_nSelected = int( m_Items.size() ) - 1; break;
                        }

                        // Perform capping
                        if( m_nSelected < 0 )
                            m_nSelected = 0;
                        if( m_nSelected >= ( int )m_Items.size() )
                            m_nSelected = int( m_Items.size() ) - 1;

                        if( nOldSelected != m_nSelected )
                        {
                            if( m_dwStyle & MULTISELECTION )
                            {
                                // Multiple selection

                                // Clear all selection
                                for( int i = 0; i < ( int )m_Items.size(); ++i )
                                {
                                    auto pItem = m_Items[i];
                                    pItem->bSelected = false;
                                }

                                if( GetKeyState( VK_SHIFT ) < 0 )
                                {
                                    // Select all items from m_nSelStart to
                                    // m_nSelected
                                    int nEnd = std::max( m_nSelStart, m_nSelected );

                                    for( int n = std::min( m_nSelStart, m_nSelected ); n <= nEnd; ++n )
                                        m_Items[n]->bSelected = true;
                                }
                                else
                                {
                                    m_Items[m_nSelected]->bSelected = true;

                                    // Update selection start
                                    m_nSelStart = m_nSelected;
                                }
                            }
                            else
                                m_nSelStart = m_nSelected;

                            // Adjust scroll bar

                            m_ScrollBar.ShowItem( m_nSelected );

                            // Send notification

                            m_pDialog->SendEvent( EVENT_LISTBOX_SELECTION, true, this );
                        }
                        return true;
                    }

                    // Space is the hotkey for double-clicking an item.
                    //
                case VK_SPACE:
                    m_pDialog->SendEvent( EVENT_LISTBOX_ITEM_DBLCLK, true, this );
                    return true;
            }
            break;
    }

    return false;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTListBox::HandleMouse( UINT uMsg, const POINT& pt, WPARAM wParam, LPARAM lParam )
{
    if( !m_bEnabled || !m_bVisible )
        return false;

    // First acquire focus
    if( WM_LBUTTONDOWN == uMsg )
        if( !m_bHasFocus )
            m_pDialog->RequestFocus( this );

    // Let the scroll bar handle it first.
    if( m_ScrollBar.HandleMouse( uMsg, pt, wParam, lParam ) )
        return true;

    switch( uMsg )
    {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            // Check for clicks in the text area
            if( !m_Items.empty() && PtInRect( &m_rcSelection, pt ) )
            {
                // Compute the index of the clicked item

                int nClicked;
                if( m_nTextHeight )
                    nClicked = m_ScrollBar.GetTrackPos() + ( pt.y - m_rcText.top ) / m_nTextHeight;
                else
                    nClicked = -1;

                // Only proceed if the click falls on top of an item.

                if( nClicked >= m_ScrollBar.GetTrackPos() &&
                    nClicked < ( int )m_Items.size() &&
                    nClicked < m_ScrollBar.GetTrackPos() + m_ScrollBar.GetPageSize() )
                {
                    SetCapture( DXUTGetHWND() );
                    m_bDrag = true;

                    // If this is a double click, fire off an event and exit
                    // since the first click would have taken care of the selection
                    // updating.
                    if( uMsg == WM_LBUTTONDBLCLK )
                    {
                        m_pDialog->SendEvent( EVENT_LISTBOX_ITEM_DBLCLK, true, this );
                        return true;
                    }

                    m_nSelected = nClicked;
                    if( !( wParam & MK_SHIFT ) )
                        m_nSelStart = m_nSelected;

                    // If this is a multi-selection listbox, update per-item
                    // selection data.

                    if( m_dwStyle & MULTISELECTION )
                    {
                        // Determine behavior based on the state of Shift and Ctrl

                        auto pSelItem = m_Items[ m_nSelected ];
                        if( ( wParam & ( MK_SHIFT | MK_CONTROL ) ) == MK_CONTROL )
                        {
                            // Control click. Reverse the selection of this item.

                            pSelItem->bSelected = !pSelItem->bSelected;
                        }
                        else if( ( wParam & ( MK_SHIFT | MK_CONTROL ) ) == MK_SHIFT )
                        {
                            // Shift click. Set the selection for all items
                            // from last selected item to the current item.
                            // Clear everything else.

                            int nBegin = std::min( m_nSelStart, m_nSelected );
                            int nEnd = std::max( m_nSelStart, m_nSelected );

                            for( int i = 0; i < nBegin; ++i )
                            {
                                auto pItem = m_Items[ i ];
                                pItem->bSelected = false;
                            }

                            for( int i = nEnd + 1; i < ( int )m_Items.size(); ++i )
                            {
                                auto pItem = m_Items[ i ];
                                pItem->bSelected = false;
                            }

                            for( int i = nBegin; i <= nEnd; ++i )
                            {
                                auto pItem = m_Items[ i ];
                                pItem->bSelected = true;
                            }
                        }
                        else if( ( wParam & ( MK_SHIFT | MK_CONTROL ) ) == ( MK_SHIFT | MK_CONTROL ) )
                        {
                            // Control-Shift-click.

                            // The behavior is:
                            //   Set all items from m_nSelStart to m_nSelected to
                            //     the same state as m_nSelStart, not including m_nSelected.
                            //   Set m_nSelected to selected.

                            int nBegin = std::min( m_nSelStart, m_nSelected );
                            int nEnd = std::max( m_nSelStart, m_nSelected );

                            // The two ends do not need to be set here.

                            bool bLastSelected = m_Items[ m_nSelStart ]->bSelected;
                            for( int i = nBegin + 1; i < nEnd; ++i )
                            {
                                auto pItem = m_Items[ i ];
                                pItem->bSelected = bLastSelected;
                            }

                            pSelItem->bSelected = true;

                            // Restore m_nSelected to the previous value
                            // This matches the Windows behavior

                            m_nSelected = m_nSelStart;
                        }
                        else
                        {
                            // Simple click.  Clear all items and select the clicked
                            // item.


                            for( int i = 0; i < ( int )m_Items.size(); ++i )
                            {
                                auto pItem = m_Items[ i ];
                                pItem->bSelected = false;
                            }

                            pSelItem->bSelected = true;
                        }
                    }  // End of multi-selection case

                    m_pDialog->SendEvent( EVENT_LISTBOX_SELECTION, true, this );
                }

                return true;
            }
            break;

        case WM_LBUTTONUP:
        {
            ReleaseCapture();
            m_bDrag = false;

            if( m_nSelected != -1 )
            {
                // Set all items between m_nSelStart and m_nSelected to
                // the same state as m_nSelStart
                int nEnd = std::max( m_nSelStart, m_nSelected );

                for( int n = std::min( m_nSelStart, m_nSelected ) + 1; n < nEnd; ++n )
                    m_Items[n]->bSelected = m_Items[m_nSelStart]->bSelected;
                m_Items[m_nSelected]->bSelected = m_Items[m_nSelStart]->bSelected;

                // If m_nSelStart and m_nSelected are not the same,
                // the user has dragged the mouse to make a selection.
                // Notify the application of this.
                if( m_nSelStart != m_nSelected )
                    m_pDialog->SendEvent( EVENT_LISTBOX_SELECTION, true, this );

                m_pDialog->SendEvent( EVENT_LISTBOX_SELECTION_END, true, this );
            }
            return false;
        }

        case WM_MOUSEMOVE:
            if( m_bDrag )
            {
                // Compute the index of the item below cursor

                int nItem;
                if( m_nTextHeight )
                    nItem = m_ScrollBar.GetTrackPos() + ( pt.y - m_rcText.top ) / m_nTextHeight;
                else
                    nItem = -1;

                // Only proceed if the cursor is on top of an item.

                if( nItem >= ( int )m_ScrollBar.GetTrackPos() &&
                    nItem < ( int )m_Items.size() &&
                    nItem < m_ScrollBar.GetTrackPos() + m_ScrollBar.GetPageSize() )
                {
                    m_nSelected = nItem;
                    m_pDialog->SendEvent( EVENT_LISTBOX_SELECTION, true, this );
                }
                else if( nItem < ( int )m_ScrollBar.GetTrackPos() )
                {
                    // User drags the mouse above window top
                    m_ScrollBar.Scroll( -1 );
                    m_nSelected = m_ScrollBar.GetTrackPos();
                    m_pDialog->SendEvent( EVENT_LISTBOX_SELECTION, true, this );
                }
                else if( nItem >= m_ScrollBar.GetTrackPos() + m_ScrollBar.GetPageSize() )
                {
                    // User drags the mouse below window bottom
                    m_ScrollBar.Scroll( 1 );
                    m_nSelected = std::min( ( int )m_Items.size(), m_ScrollBar.GetTrackPos() +
                                         m_ScrollBar.GetPageSize() ) - 1;
                    m_pDialog->SendEvent( EVENT_LISTBOX_SELECTION, true, this );
                }
            }
            break;

        case WM_MOUSEWHEEL:
        {
            UINT uLines = 0;
            if ( !SystemParametersInfo( SPI_GETWHEELSCROLLLINES, 0, &uLines, 0 ) )
                uLines = 0;
            int nScrollAmount = int( ( short )HIWORD( wParam ) ) / WHEEL_DELTA * uLines;
            m_ScrollBar.Scroll( -nScrollAmount );
            return true;
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTListBox::MsgProc( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER(wParam);

    if( WM_CAPTURECHANGED == uMsg )
    {
        // The application just lost mouse capture. We may not have gotten
        // the WM_MOUSEUP message, so reset m_bDrag here.
        if( ( HWND )lParam != DXUTGetHWND() )
            m_bDrag = false;
    }

    return false;
}


//--------------------------------------------------------------------------------------
void CDXUTListBox::Render( _In_ float fElapsedTime )
{
    if( m_bVisible == false )
        return;

    auto pElement = m_Elements[ 0 ];
    pElement->TextureColor.Blend( DXUT_STATE_NORMAL, fElapsedTime );
    pElement->FontColor.Blend( DXUT_STATE_NORMAL, fElapsedTime );

    auto pSelElement = m_Elements[ 1 ];
    pSelElement->TextureColor.Blend( DXUT_STATE_NORMAL, fElapsedTime );
    pSelElement->FontColor.Blend( DXUT_STATE_NORMAL, fElapsedTime );

    m_pDialog->DrawSprite( pElement, &m_rcBoundingBox, DXUT_FAR_BUTTON_DEPTH );

    // Render the text
    if( !m_Items.empty() )
    {
        // Find out the height of a single line of text
        RECT rc = m_rcText;
        RECT rcSel = m_rcSelection;
        rc.bottom = rc.top + m_pDialog->GetManager()->GetFontNode( pElement->iFont )->nHeight;

        // Update the line height formation
        m_nTextHeight = rc.bottom - rc.top;

        static bool bSBInit;
        if( !bSBInit )
        {
            // Update the page size of the scroll bar
            if( m_nTextHeight )
                m_ScrollBar.SetPageSize( RectHeight( m_rcText ) / m_nTextHeight );
            else
                m_ScrollBar.SetPageSize( RectHeight( m_rcText ) );
            bSBInit = true;
        }

        rc.right = m_rcText.right;
        for( int i = m_ScrollBar.GetTrackPos(); i < ( int )m_Items.size(); ++i )
        {
            if( rc.bottom > m_rcText.bottom )
                break;

            auto pItem = m_Items[ i ];

            // Determine if we need to render this item with the
            // selected element.
            bool bSelectedStyle = false;

            if( !( m_dwStyle & MULTISELECTION ) && i == m_nSelected )
                bSelectedStyle = true;
            else if( m_dwStyle & MULTISELECTION )
            {
                if( m_bDrag &&
                    ( ( i >= m_nSelected && i < m_nSelStart ) ||
                      ( i <= m_nSelected && i > m_nSelStart ) ) )
                    bSelectedStyle = m_Items[m_nSelStart]->bSelected;
                else if( pItem->bSelected )
                    bSelectedStyle = true;
            }

            if( bSelectedStyle )
            {
                rcSel.top = rc.top; rcSel.bottom = rc.bottom;
                m_pDialog->DrawSprite( pSelElement, &rcSel, DXUT_NEAR_BUTTON_DEPTH );
                m_pDialog->DrawText( pItem->strText, pSelElement, &rc );
            }
            else
                m_pDialog->DrawText( pItem->strText, pElement, &rc );

            OffsetRect( &rc, 0, m_nTextHeight );
        }
    }

    // Render the scroll bar

    m_ScrollBar.Render( fElapsedTime );
}


//======================================================================================
// CDXUTEditBox class
//======================================================================================

// Static member initialization
bool CDXUTEditBox::s_bHideCaret;   // If true, we don't render the caret.

// When scrolling, EDITBOX_SCROLLEXTENT is reciprocal of the amount to scroll.
// If EDITBOX_SCROLLEXTENT = 4, then we scroll 1/4 of the control each time.
#define EDITBOX_SCROLLEXTENT 4

//--------------------------------------------------------------------------------------
CDXUTEditBox::CDXUTEditBox( _In_opt_ CDXUTDialog* pDialog )
{
    m_Type = DXUT_CONTROL_EDITBOX;
    m_pDialog = pDialog;

    m_nBorder = 5;  // Default border width
    m_nSpacing = 4;  // Default spacing

    m_bCaretOn = true;
    m_dfBlink = GetCaretBlinkTime() * 0.001f;
    m_dfLastBlink = DXUTGetGlobalTimer()->GetAbsoluteTime();
    s_bHideCaret = false;
    m_nFirstVisible = 0;
    m_TextColor = D3DCOLOR_ARGB( 255, 16, 16, 16 );
    m_SelTextColor = D3DCOLOR_ARGB( 255, 255, 255, 255 );
    m_SelBkColor = D3DCOLOR_ARGB( 255, 40, 50, 92 );
    m_CaretColor = D3DCOLOR_ARGB( 255, 0, 0, 0 );
    m_nCaret = m_nSelStart = 0;
    m_bInsertMode = true;

    m_bMouseDrag = false;
}


//--------------------------------------------------------------------------------------
CDXUTEditBox::~CDXUTEditBox()
{
}


//--------------------------------------------------------------------------------------
// PlaceCaret: Set the caret to a character position, and adjust the scrolling if
//             necessary.
//--------------------------------------------------------------------------------------
void CDXUTEditBox::PlaceCaret( _In_ int nCP )
{
    assert( nCP >= 0 && nCP <= m_Buffer.GetTextSize() );
    m_nCaret = nCP;

    // Obtain the X offset of the character.
    int nX1st, nX, nX2;
    m_Buffer.CPtoX( m_nFirstVisible, FALSE, &nX1st );  // 1st visible char
    m_Buffer.CPtoX( nCP, FALSE, &nX );  // LEAD
    // If nCP is the nul terminator, get the leading edge instead of trailing.
    if( nCP == m_Buffer.GetTextSize() )
        nX2 = nX;
    else
        m_Buffer.CPtoX( nCP, TRUE, &nX2 );  // TRAIL

    // If the left edge of the char is smaller than the left edge of the 1st visible char,
    // we need to scroll left until this char is visible.
    if( nX < nX1st )
    {
        // Simply make the first visible character the char at the new caret position.
        m_nFirstVisible = nCP;
    }
    else // If the right of the character is bigger than the offset of the control's
    // right edge, we need to scroll right to this character.
    if( nX2 > nX1st + RectWidth( m_rcText ) )
    {
        // Compute the X of the new left-most pixel
        int nXNewLeft = nX2 - RectWidth( m_rcText );

        // Compute the char position of this character
        int nCPNew1st, nNewTrail;
        m_Buffer.XtoCP( nXNewLeft, &nCPNew1st, &nNewTrail );

        // If this coordinate is not on a character border,
        // start from the next character so that the caret
        // position does not fall outside the text rectangle.
        int nXNew1st;
        m_Buffer.CPtoX( nCPNew1st, FALSE, &nXNew1st );
        if( nXNew1st < nXNewLeft )
            ++nCPNew1st;

        m_nFirstVisible = nCPNew1st;
    }
}


//--------------------------------------------------------------------------------------
void CDXUTEditBox::ClearText()
{
    m_Buffer.Clear();
    m_nFirstVisible = 0;
    PlaceCaret( 0 );
    m_nSelStart = 0;
}


//--------------------------------------------------------------------------------------
void CDXUTEditBox::SetText( _In_z_ LPCWSTR wszText, _In_ bool bSelected )
{
    assert( wszText );

    m_Buffer.SetText( wszText );
    m_nFirstVisible = 0;
    // Move the caret to the end of the text
    PlaceCaret( m_Buffer.GetTextSize() );
    m_nSelStart = bSelected ? 0 : m_nCaret;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTEditBox::GetTextCopy( LPWSTR strDest,  UINT bufferCount  ) const
{
    assert( strDest );

    wcscpy_s( strDest, bufferCount, m_Buffer.GetBuffer() );

    return S_OK;
}


//--------------------------------------------------------------------------------------
void CDXUTEditBox::DeleteSelectionText()
{
    int nFirst = std::min( m_nCaret, m_nSelStart );
    int nLast = std::max( m_nCaret, m_nSelStart );
    // Update caret and selection
    PlaceCaret( nFirst );
    m_nSelStart = m_nCaret;
    // Remove the characters
    for( int i = nFirst; i < nLast; ++i )
        m_Buffer.RemoveChar( nFirst );
}


//--------------------------------------------------------------------------------------
void CDXUTEditBox::UpdateRects()
{
    CDXUTControl::UpdateRects();

    // Update the text rectangle
    m_rcText = m_rcBoundingBox;
    // First inflate by m_nBorder to compute render rects
    InflateRect( &m_rcText, -m_nBorder, -m_nBorder );

    // Update the render rectangles
    m_rcRender[0] = m_rcText;
    SetRect( &m_rcRender[1], m_rcBoundingBox.left, m_rcBoundingBox.top, m_rcText.left, m_rcText.top );
    SetRect( &m_rcRender[2], m_rcText.left, m_rcBoundingBox.top, m_rcText.right, m_rcText.top );
    SetRect( &m_rcRender[3], m_rcText.right, m_rcBoundingBox.top, m_rcBoundingBox.right, m_rcText.top );
    SetRect( &m_rcRender[4], m_rcBoundingBox.left, m_rcText.top, m_rcText.left, m_rcText.bottom );
    SetRect( &m_rcRender[5], m_rcText.right, m_rcText.top, m_rcBoundingBox.right, m_rcText.bottom );
    SetRect( &m_rcRender[6], m_rcBoundingBox.left, m_rcText.bottom, m_rcText.left, m_rcBoundingBox.bottom );
    SetRect( &m_rcRender[7], m_rcText.left, m_rcText.bottom, m_rcText.right, m_rcBoundingBox.bottom );
    SetRect( &m_rcRender[8], m_rcText.right, m_rcText.bottom, m_rcBoundingBox.right, m_rcBoundingBox.bottom );

    // Inflate further by m_nSpacing
    InflateRect( &m_rcText, -m_nSpacing, -m_nSpacing );
}


#pragma warning(push)
#pragma warning( disable : 4616 6386 )
void CDXUTEditBox::CopyToClipboard()
{
    // Copy the selection text to the clipboard
    if( m_nCaret != m_nSelStart && OpenClipboard( nullptr ) )
    {
        EmptyClipboard();

        HGLOBAL hBlock = GlobalAlloc( GMEM_MOVEABLE, sizeof( WCHAR ) * ( m_Buffer.GetTextSize() + 1 ) );
        if( hBlock )
        {
            auto pwszText = reinterpret_cast<WCHAR*>( GlobalLock( hBlock ) );
            if( pwszText )
            {
                int nFirst = std::min( m_nCaret, m_nSelStart );
                int nLast = std::max( m_nCaret, m_nSelStart );
                if( nLast - nFirst > 0 )
                {
                    memcpy( pwszText, m_Buffer.GetBuffer() + nFirst, ( nLast - nFirst ) * sizeof( WCHAR ) );
                }
                pwszText[nLast - nFirst] = L'\0';  // Terminate it
                GlobalUnlock( hBlock );
            }
            SetClipboardData( CF_UNICODETEXT, hBlock );
        }
        CloseClipboard();
        // We must not free the object until CloseClipboard is called.
        if( hBlock )
            GlobalFree( hBlock );
    }
}


void CDXUTEditBox::PasteFromClipboard()
{
    DeleteSelectionText();

    if( OpenClipboard( nullptr ) )
    {
        HANDLE handle = GetClipboardData( CF_UNICODETEXT );
        if( handle )
        {
            // Convert the ANSI string to Unicode, then
            // insert to our buffer.
            auto pwszText = reinterpret_cast<WCHAR*>( GlobalLock( handle ) );
            if( pwszText )
            {
                // Copy all characters up to null.
                if( m_Buffer.InsertString( m_nCaret, pwszText ) )
                    PlaceCaret( m_nCaret + (int)wcslen( pwszText ) );
                m_nSelStart = m_nCaret;
                GlobalUnlock( handle );
            }
        }
        CloseClipboard();
    }
}
#pragma warning(pop)


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTEditBox::HandleKeyboard( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER(lParam);

    if( !m_bEnabled || !m_bVisible )
        return false;

    bool bHandled = false;

    switch( uMsg )
    {
        case WM_KEYDOWN:
        {
            switch( wParam )
            {
                case VK_TAB:
                    // We don't process Tab in case keyboard input is enabled and the user
                    // wishes to Tab to other controls.
                    break;

                case VK_HOME:
                    PlaceCaret( 0 );
                    if( GetKeyState( VK_SHIFT ) >= 0 )
                        // Shift is not down. Update selection
                        // start along with the caret.
                        m_nSelStart = m_nCaret;
                    ResetCaretBlink();
                    bHandled = true;
                    break;

                case VK_END:
                    PlaceCaret( m_Buffer.GetTextSize() );
                    if( GetKeyState( VK_SHIFT ) >= 0 )
                        // Shift is not down. Update selection
                        // start along with the caret.
                        m_nSelStart = m_nCaret;
                    ResetCaretBlink();
                    bHandled = true;
                    break;

                case VK_INSERT:
                    if( GetKeyState( VK_CONTROL ) < 0 )
                    {
                        // Control Insert. Copy to clipboard
                        CopyToClipboard();
                    }
                    else if( GetKeyState( VK_SHIFT ) < 0 )
                    {
                        // Shift Insert. Paste from clipboard
                        PasteFromClipboard();
                    }
                    else
                    {
                        // Toggle caret insert mode
                        m_bInsertMode = !m_bInsertMode;
                    }
                    break;

                case VK_DELETE:
                    // Check if there is a text selection.
                    if( m_nCaret != m_nSelStart )
                    {
                        DeleteSelectionText();
                        m_pDialog->SendEvent( EVENT_EDITBOX_CHANGE, true, this );
                    }
                    else
                    {
                        // Deleting one character
                        if( m_Buffer.RemoveChar( m_nCaret ) )
                            m_pDialog->SendEvent( EVENT_EDITBOX_CHANGE, true, this );
                    }
                    ResetCaretBlink();
                    bHandled = true;
                    break;

                case VK_LEFT:
                    if( GetKeyState( VK_CONTROL ) < 0 )
                    {
                        // Control is down. Move the caret to a new item
                        // instead of a character.
                        m_Buffer.GetPriorItemPos( m_nCaret, &m_nCaret );
                        PlaceCaret( m_nCaret );
                    }
                    else if( m_nCaret > 0 )
                        PlaceCaret( m_nCaret - 1 );
                    if( GetKeyState( VK_SHIFT ) >= 0 )
                        // Shift is not down. Update selection
                        // start along with the caret.
                        m_nSelStart = m_nCaret;
                    ResetCaretBlink();
                    bHandled = true;
                    break;

                case VK_RIGHT:
                    if( GetKeyState( VK_CONTROL ) < 0 )
                    {
                        // Control is down. Move the caret to a new item
                        // instead of a character.
                        m_Buffer.GetNextItemPos( m_nCaret, &m_nCaret );
                        PlaceCaret( m_nCaret );
                    }
                    else if( m_nCaret < m_Buffer.GetTextSize() )
                        PlaceCaret( m_nCaret + 1 );
                    if( GetKeyState( VK_SHIFT ) >= 0 )
                        // Shift is not down. Update selection
                        // start along with the caret.
                        m_nSelStart = m_nCaret;
                    ResetCaretBlink();
                    bHandled = true;
                    break;

                case VK_UP:
                case VK_DOWN:
                    // Trap up and down arrows so that the dialog
                    // does not switch focus to another control.
                    bHandled = true;
                    break;

                default:
                    bHandled = wParam != VK_ESCAPE;  // Let the application handle Esc.
            }
        }
    }
    return bHandled;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTEditBox::HandleMouse( UINT uMsg, const POINT& pt, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    if( !m_bEnabled || !m_bVisible )
        return false;

    switch( uMsg )
    {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            {
                if( !m_bHasFocus )
                    m_pDialog->RequestFocus( this );

                if( !ContainsPoint( pt ) )
                    return false;

                m_bMouseDrag = true;
                SetCapture( DXUTGetHWND() );
                // Determine the character corresponding to the coordinates.
                int nCP, nTrail, nX1st;
                m_Buffer.CPtoX( m_nFirstVisible, FALSE, &nX1st );  // X offset of the 1st visible char
                if( m_Buffer.XtoCP( pt.x - m_rcText.left + nX1st, &nCP, &nTrail ) )
                {
                    // Cap at the nul character.
                    if( nTrail && nCP < m_Buffer.GetTextSize() )
                        PlaceCaret( nCP + 1 );
                    else
                        PlaceCaret( nCP );
                    m_nSelStart = m_nCaret;
                    ResetCaretBlink();
                }
                return true;
            }

        case WM_LBUTTONUP:
            ReleaseCapture();
            m_bMouseDrag = false;
            break;

        case WM_MOUSEMOVE:
            if( m_bMouseDrag )
            {
                // Determine the character corresponding to the coordinates.
                int nCP, nTrail, nX1st;
                m_Buffer.CPtoX( m_nFirstVisible, FALSE, &nX1st );  // X offset of the 1st visible char
                if( m_Buffer.XtoCP( pt.x - m_rcText.left + nX1st, &nCP, &nTrail ) )
                {
                    // Cap at the nul character.
                    if( nTrail && nCP < m_Buffer.GetTextSize() )
                        PlaceCaret( nCP + 1 );
                    else
                        PlaceCaret( nCP );
                }
            }
            break;
    }

    return false;
}


//--------------------------------------------------------------------------------------
void CDXUTEditBox::OnFocusIn()
{
    CDXUTControl::OnFocusIn();

    ResetCaretBlink();
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CDXUTEditBox::MsgProc( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER(lParam);

    if( !m_bEnabled || !m_bVisible )
        return false;

    switch( uMsg )
    {
            // Make sure that while editing, the keyup and keydown messages associated with 
            // WM_CHAR messages don't go to any non-focused controls or cameras
        case WM_KEYUP:
        case WM_KEYDOWN:
            return true;

        case WM_CHAR:
        {
            switch( ( WCHAR )wParam )
            {
                    // Backspace
                case VK_BACK:
                {
                    // If there's a selection, treat this
                    // like a delete key.
                    if( m_nCaret != m_nSelStart )
                    {
                        DeleteSelectionText();
                        m_pDialog->SendEvent( EVENT_EDITBOX_CHANGE, true, this );
                    }
                    else if( m_nCaret > 0 )
                    {
                        // Move the caret, then delete the char.
                        PlaceCaret( m_nCaret - 1 );
                        m_nSelStart = m_nCaret;
                        m_Buffer.RemoveChar( m_nCaret );
                        m_pDialog->SendEvent( EVENT_EDITBOX_CHANGE, true, this );
                    }
                    ResetCaretBlink();
                    break;
                }

                case 24:        // Ctrl-X Cut
                case VK_CANCEL: // Ctrl-C Copy
                    {
                        CopyToClipboard();

                        // If the key is Ctrl-X, delete the selection too.
                        if( ( WCHAR )wParam == 24 )
                        {
                            DeleteSelectionText();
                            m_pDialog->SendEvent( EVENT_EDITBOX_CHANGE, true, this );
                        }

                        break;
                    }

                    // Ctrl-V Paste
                case 22:
                {
                    PasteFromClipboard();
                    m_pDialog->SendEvent( EVENT_EDITBOX_CHANGE, true, this );
                    break;
                }

                    // Ctrl-A Select All
                case 1:
                    if( m_nSelStart == m_nCaret )
                    {
                        m_nSelStart = 0;
                        PlaceCaret( m_Buffer.GetTextSize() );
                    }
                    break;

                case VK_RETURN:
                    // Invoke the callback when the user presses Enter.
                    m_pDialog->SendEvent( EVENT_EDITBOX_STRING, true, this );
                    break;

                    // Junk characters we don't want in the string
                case 26:  // Ctrl Z
                case 2:   // Ctrl B
                case 14:  // Ctrl N
                case 19:  // Ctrl S
                case 4:   // Ctrl D
                case 6:   // Ctrl F
                case 7:   // Ctrl G
                case 10:  // Ctrl J
                case 11:  // Ctrl K
                case 12:  // Ctrl L
                case 17:  // Ctrl Q
                case 23:  // Ctrl W
                case 5:   // Ctrl E
                case 18:  // Ctrl R
                case 20:  // Ctrl T
                case 25:  // Ctrl Y
                case 21:  // Ctrl U
                case 9:   // Ctrl I
                case 15:  // Ctrl O
                case 16:  // Ctrl P
                case 27:  // Ctrl [
                case 29:  // Ctrl ]
                case 28:  // Ctrl \ 
                    break;

                default:
                {
                    // If there's a selection and the user
                    // starts to type, the selection should
                    // be deleted.
                    if( m_nCaret != m_nSelStart )
                        DeleteSelectionText();

                    // If we are in overwrite mode and there is already
                    // a char at the caret's position, simply replace it.
                    // Otherwise, we insert the char as normal.
                    if( !m_bInsertMode && m_nCaret < m_Buffer.GetTextSize() )
                    {
                        m_Buffer[m_nCaret] = ( WCHAR )wParam;
                        PlaceCaret( m_nCaret + 1 );
                        m_nSelStart = m_nCaret;
                    }
                    else
                    {
                        // Insert the char
                        if( m_Buffer.InsertChar( m_nCaret, ( WCHAR )wParam ) )
                        {
                            PlaceCaret( m_nCaret + 1 );
                            m_nSelStart = m_nCaret;
                        }
                    }
                    ResetCaretBlink();
                    m_pDialog->SendEvent( EVENT_EDITBOX_CHANGE, true, this );
                }
            }
            return true;
        }
    }
    return false;
}


//--------------------------------------------------------------------------------------
void CDXUTEditBox::Render( _In_ float fElapsedTime )
{
    if( m_bVisible == false )
        return;

    int nSelStartX = 0, nCaretX = 0;  // Left and right X cordinates of the selection region

    auto pElement = GetElement( 0 );
    if( pElement )
    {
        m_Buffer.SetFontNode( m_pDialog->GetFont( pElement->iFont ) );
        PlaceCaret( m_nCaret );  // Call PlaceCaret now that we have the font info (node),
        // so that scrolling can be handled.
    }

    // Render the control graphics
    for( int e = 0; e < 9; ++e )
    {
        pElement = m_Elements[ e ];
        pElement->TextureColor.Blend( DXUT_STATE_NORMAL, fElapsedTime );

        m_pDialog->DrawSprite( pElement, &m_rcRender[e], DXUT_FAR_BUTTON_DEPTH );
    }

    //
    // Compute the X coordinates of the first visible character.
    //
    int nXFirst;
    m_Buffer.CPtoX( m_nFirstVisible, FALSE, &nXFirst );

    //
    // Compute the X coordinates of the selection rectangle
    //
    m_Buffer.CPtoX( m_nCaret, FALSE, &nCaretX );
    if( m_nCaret != m_nSelStart )
        m_Buffer.CPtoX( m_nSelStart, FALSE, &nSelStartX );
    else
        nSelStartX = nCaretX;

    //
    // Render the selection rectangle
    //
    RECT rcSelection;  // Make this available for rendering selected text
    if( m_nCaret != m_nSelStart )
    {
        int nSelLeftX = nCaretX, nSelRightX = nSelStartX;
        // Swap if left is bigger than right
        if( nSelLeftX > nSelRightX )
        {
            int nTemp = nSelLeftX; nSelLeftX = nSelRightX; nSelRightX = nTemp;
        }

        SetRect( &rcSelection, nSelLeftX, m_rcText.top, nSelRightX, m_rcText.bottom );
        OffsetRect( &rcSelection, m_rcText.left - nXFirst, 0 );
        IntersectRect( &rcSelection, &m_rcText, &rcSelection );

        m_pDialog->DrawRect( &rcSelection, m_SelBkColor );
    }

    //
    // Render the text
    //
    // Element 0 for text
    m_Elements[ 0 ]->FontColor.SetCurrent( m_TextColor );
    m_pDialog->DrawText( m_Buffer.GetBuffer() + m_nFirstVisible, m_Elements[ 0 ], &m_rcText );

    // Render the selected text
    if( m_nCaret != m_nSelStart )
    {
        int nFirstToRender = std::max( m_nFirstVisible, std::min( m_nSelStart, m_nCaret ) );
        m_Elements[ 0 ]->FontColor.SetCurrent( m_SelTextColor );
        m_pDialog->DrawText( m_Buffer.GetBuffer() + nFirstToRender,
                             m_Elements[ 0 ], &rcSelection, false );
    }

    //
    // Blink the caret
    //
    if( DXUTGetGlobalTimer()->GetAbsoluteTime() - m_dfLastBlink >= m_dfBlink )
    {
        m_bCaretOn = !m_bCaretOn;
        m_dfLastBlink = DXUTGetGlobalTimer()->GetAbsoluteTime();
    }

    //
    // Render the caret if this control has the focus
    //
    if( m_bHasFocus && m_bCaretOn && !s_bHideCaret )
    {
        // Start the rectangle with insert mode caret
        RECT rcCaret =
        {
            m_rcText.left - nXFirst + nCaretX - 1, m_rcText.top,
            m_rcText.left - nXFirst + nCaretX + 1, m_rcText.bottom
        };

        // If we are in overwrite mode, adjust the caret rectangle
        // to fill the entire character.
        if( !m_bInsertMode )
        {
            // Obtain the right edge X coord of the current character
            int nRightEdgeX;
            m_Buffer.CPtoX( m_nCaret, TRUE, &nRightEdgeX );
            rcCaret.right = m_rcText.left - nXFirst + nRightEdgeX;
        }

        m_pDialog->DrawRect( &rcCaret, m_CaretColor );
    }
}


#define IN_FLOAT_CHARSET( c ) \
    ( (c) == L'-' || (c) == L'.' || ( (c) >= L'0' && (c) <= L'9' ) )

_Use_decl_annotations_
void CDXUTEditBox::ParseFloatArray( float* pNumbers, int nCount )
{
    int nWritten = 0;  // Number of floats written
    const WCHAR* pToken, *pEnd;
    WCHAR wszToken[60];

    pToken = m_Buffer.GetBuffer();
    while( nWritten < nCount && *pToken != L'\0' )
    {
        // Skip leading spaces
        while( *pToken == L' ' )
            ++pToken;

        if( *pToken == L'\0' )
            break;

        // Locate the end of number
        pEnd = pToken;
        while( IN_FLOAT_CHARSET( *pEnd ) )
            ++pEnd;

        // Copy the token to our buffer
        int nTokenLen = std::min<int>( sizeof( wszToken ) / sizeof( wszToken[0] ) - 1, int( pEnd - pToken ) );
        wcscpy_s( wszToken, nTokenLen, pToken );
        *pNumbers = ( float )wcstod( wszToken, nullptr );
        ++nWritten;
        ++pNumbers;
        pToken = pEnd;
    }
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTEditBox::SetTextFloatArray( const float* pNumbers, int nCount )
{
    WCHAR wszBuffer[512] =
    {
        0
    };
    WCHAR wszTmp[64];

    if( !pNumbers )
        return;

    for( int i = 0; i < nCount; ++i )
    {
        swprintf_s( wszTmp, 64, L"%.4f ", pNumbers[i] );
        wcscat_s( wszBuffer, 512, wszTmp );
    }

    // Don't want the last space
    if( nCount > 0 && wcslen( wszBuffer ) > 0 )
        wszBuffer[wcslen( wszBuffer ) - 1] = 0;

    SetText( wszBuffer );
}


//--------------------------------------------------------------------------------------
void CDXUTEditBox::ResetCaretBlink()
{
    m_bCaretOn = true;
    m_dfLastBlink = DXUTGetGlobalTimer()->GetAbsoluteTime();
}


//======================================================================================
// CUniBuffer
//======================================================================================

//--------------------------------------------------------------------------------------
bool CUniBuffer::SetBufferSize( _In_ int nNewSize )
{
    // If the current size is already the maximum allowed,
    // we can't possibly allocate more.
    if( m_nBufferSize >= DXUT_MAX_EDITBOXLENGTH )
        return false;

    int nAllocateSize = ( nNewSize == -1 || nNewSize < m_nBufferSize * 2 ) ? ( m_nBufferSize ? m_nBufferSize *
                                                                               2 : 256 ) : nNewSize * 2;

    // Cap the buffer size at the maximum allowed.
    if( nAllocateSize > DXUT_MAX_EDITBOXLENGTH )
        nAllocateSize = DXUT_MAX_EDITBOXLENGTH;

    auto pTempBuffer = new (std::nothrow) WCHAR[nAllocateSize];
    if( !pTempBuffer )
        return false;

    ZeroMemory( pTempBuffer, sizeof( WCHAR ) * nAllocateSize );

    if( m_pwszBuffer )
    {
        memcpy( pTempBuffer, m_pwszBuffer, m_nBufferSize * sizeof( WCHAR ) );
        delete[] m_pwszBuffer;
    }

    m_pwszBuffer = pTempBuffer;
    m_nBufferSize = nAllocateSize;
    return true;
}


//--------------------------------------------------------------------------------------
// Uniscribe -- Analyse() analyses the string in the buffer
//--------------------------------------------------------------------------------------
HRESULT CUniBuffer::Analyse()
{
    if( m_Analysis )
        (void)ScriptStringFree( &m_Analysis );

    SCRIPT_CONTROL ScriptControl; // For uniscribe
    SCRIPT_STATE ScriptState;   // For uniscribe
    ZeroMemory( &ScriptControl, sizeof( ScriptControl ) );
    ZeroMemory( &ScriptState, sizeof( ScriptState ) );

#pragma warning(push)
#pragma warning(disable : 4616 6309 6387 )
    HRESULT hr = ScriptApplyDigitSubstitution( nullptr, &ScriptControl, &ScriptState );
    if ( FAILED(hr)  )
        return hr;
#pragma warning(pop)

    if( !m_pFontNode )
        return E_FAIL;

    HDC hDC = nullptr;
    hr = ScriptStringAnalyse( hDC,
                                m_pwszBuffer,
                                (int)wcslen( m_pwszBuffer ) + 1,  // nul is also analyzed.
                                (int)wcslen( m_pwszBuffer ) * 3 / 2 + 16,
                                -1,
                                SSA_BREAK | SSA_GLYPHS | SSA_FALLBACK | SSA_LINK,
                                0,
                                &ScriptControl,
                                &ScriptState,
                                nullptr,
                                nullptr,
                                nullptr,
                                &m_Analysis );
    if( SUCCEEDED( hr ) )
        m_bAnalyseRequired = false;  // Analysis is up-to-date
    return hr;
}


//--------------------------------------------------------------------------------------
CUniBuffer::CUniBuffer( _In_ int nInitialSize )
{
    m_nBufferSize = 0;
    m_pwszBuffer = nullptr;
    m_bAnalyseRequired = true;
    m_Analysis = nullptr;
    m_pFontNode = nullptr;

    if( nInitialSize > 0 )
        SetBufferSize( nInitialSize );
}


//--------------------------------------------------------------------------------------
CUniBuffer::~CUniBuffer()
{
    delete[] m_pwszBuffer;
    if( m_Analysis )
        (void)ScriptStringFree( &m_Analysis );
}


//--------------------------------------------------------------------------------------
WCHAR& CUniBuffer::operator[]( _In_ int n )  // No param checking
{
    // This version of operator[] is called only
    // if we are asking for write access, so
    // re-analysis is required.
    m_bAnalyseRequired = true;
    return m_pwszBuffer[n];
}


//--------------------------------------------------------------------------------------
void CUniBuffer::Clear()
{
    *m_pwszBuffer = L'\0';
    m_bAnalyseRequired = true;
}


//--------------------------------------------------------------------------------------
// Inserts the char at specified index.
// If nIndex == -1, insert to the end.
//--------------------------------------------------------------------------------------
bool CUniBuffer::InsertChar( _In_ int nIndex, _In_ WCHAR wChar )
{
    assert( nIndex >= 0 );

    if( nIndex < 0 || nIndex > (int)wcslen( m_pwszBuffer ) )
        return false;  // invalid index

    // Check for maximum length allowed
    if( GetTextSize() + 1 >= DXUT_MAX_EDITBOXLENGTH )
        return false;

    if( (int)wcslen( m_pwszBuffer ) + 1 >= m_nBufferSize )
    {
        if( !SetBufferSize( -1 ) )
            return false;  // out of memory
    }

    assert( m_nBufferSize >= 2 );

    // Shift the characters after the index, start by copying the null terminator
    WCHAR* dest = m_pwszBuffer + wcslen( m_pwszBuffer ) + 1;
    WCHAR* stop = m_pwszBuffer + nIndex;
    WCHAR* src = dest - 1;

    while( dest > stop )
    {
        *dest-- = *src--;
    }

    // Set new character
    m_pwszBuffer[ nIndex ] = wChar;
    m_bAnalyseRequired = true;

    return true;
}


//--------------------------------------------------------------------------------------
// Removes the char at specified index.
// If nIndex == -1, remove the last char.
//--------------------------------------------------------------------------------------
bool CUniBuffer::RemoveChar( _In_ int nIndex )
{
    if( !wcslen( m_pwszBuffer ) || nIndex < 0 || nIndex >= (int)wcslen( m_pwszBuffer ) )
        return false;  // Invalid index

    MoveMemory( m_pwszBuffer + nIndex, m_pwszBuffer + nIndex + 1, sizeof( WCHAR ) *
                ( wcslen( m_pwszBuffer ) - nIndex ) );
    m_bAnalyseRequired = true;
    return true;
}


//--------------------------------------------------------------------------------------
// Inserts the first nCount characters of the string pStr at specified index.
// If nCount == -1, the entire string is inserted.
// If nIndex == -1, insert to the end.
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CUniBuffer::InsertString( int nIndex, const WCHAR* pStr, int nCount )
{
    assert( nIndex >= 0 );
    if( nIndex < 0 )
        return false;

    if( nIndex > (int)wcslen( m_pwszBuffer ) )
        return false;  // invalid index

    if( -1 == nCount )
        nCount = (int)wcslen( pStr );

    // Check for maximum length allowed
    if( GetTextSize() + nCount >= DXUT_MAX_EDITBOXLENGTH )
        return false;

    if( (int)wcslen( m_pwszBuffer ) + nCount >= m_nBufferSize )
    {
        if( !SetBufferSize( (int)wcslen( m_pwszBuffer ) + nCount + 1 ) )
            return false;  // out of memory
    }

    MoveMemory( m_pwszBuffer + nIndex + nCount, m_pwszBuffer + nIndex, sizeof( WCHAR ) *
                ( wcslen( m_pwszBuffer ) - nIndex + 1 ) );
    memcpy( m_pwszBuffer + nIndex, pStr, nCount * sizeof( WCHAR ) );
    m_bAnalyseRequired = true;

    return true;
}


//--------------------------------------------------------------------------------------
bool CUniBuffer::SetText( _In_z_ LPCWSTR wszText )
{
    assert( wszText );

    size_t nRequired = wcslen( wszText ) + 1;

    // Check for maximum length allowed
    if( nRequired >= DXUT_MAX_EDITBOXLENGTH )
        return false;

    while( GetBufferSize() < nRequired )
        if( !SetBufferSize( -1 ) )
            break;
    // Check again in case out of memory occurred inside while loop.
    if( GetBufferSize() >= nRequired )
    {
        wcscpy_s( m_pwszBuffer, GetBufferSize(), wszText );
        m_bAnalyseRequired = true;
        return true;
    }
    else
        return false;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CUniBuffer::CPtoX( int nCP, bool bTrail, int* pX )
{
    assert( pX );
    *pX = 0;  // Default

    HRESULT hr = S_OK;
    if( m_bAnalyseRequired )
        hr = Analyse();

    if( SUCCEEDED( hr ) )
        hr = ScriptStringCPtoX( m_Analysis, nCP, bTrail, pX );

    if ( FAILED(hr) )
    {
        *pX = 0;
        return false;
    }

    return true;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool CUniBuffer::XtoCP( int nX, int* pCP, int* pnTrail )
{
    assert( pCP && pnTrail );
    *pCP = 0; *pnTrail = FALSE;  // Default

    HRESULT hr = S_OK;
    if( m_bAnalyseRequired )
        hr = Analyse();

    if (SUCCEEDED(hr))
    {
        hr = ScriptStringXtoCP( m_Analysis, nX, pCP, pnTrail );
        if (FAILED(hr))
        {
            *pCP = 0; *pnTrail = FALSE;
            return false;
        }
    }

    // If the coordinate falls outside the text region, we
    // can get character positions that don't exist.  We must
    // filter them here and convert them to those that do exist.
    if( *pCP == -1 && *pnTrail == TRUE )
    {
        *pCP = 0; *pnTrail = FALSE;
    }
    else if( *pCP > (int)wcslen( m_pwszBuffer ) && *pnTrail == FALSE )
    {
        *pCP = (int)wcslen( m_pwszBuffer ); *pnTrail = TRUE;
    }

    if (FAILED(hr))
    {
        *pCP = 0; *pnTrail = FALSE;
        return false;
    }
    return true;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CUniBuffer::GetPriorItemPos( int nCP, int* pPrior )
{
    *pPrior = nCP;  // Default is the char itself

    if( m_bAnalyseRequired )
        if( FAILED( Analyse() ) )
            return;

    const SCRIPT_LOGATTR* pLogAttr = ScriptString_pLogAttr( m_Analysis );
    if( !pLogAttr )
        return;

    if( !ScriptString_pcOutChars( m_Analysis ) )
        return;
    int nInitial = *ScriptString_pcOutChars( m_Analysis );
    if( nCP - 1 < nInitial )
        nInitial = nCP - 1;
    for( int i = nInitial; i > 0; --i )
        if( pLogAttr[i].fWordStop ||       // Either the fWordStop flag is set
            ( !pLogAttr[i].fWhiteSpace &&  // Or the previous char is whitespace but this isn't.
              pLogAttr[i - 1].fWhiteSpace ) )
        {
            *pPrior = i;
            return;
        }
    // We have reached index 0.  0 is always a break point, so simply return it.
    *pPrior = 0;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CUniBuffer::GetNextItemPos( int nCP, int* pPrior )
{
    *pPrior = nCP;  // Default is the char itself

    HRESULT hr = S_OK;
    if( m_bAnalyseRequired )
        hr = Analyse();
    if( FAILED( hr ) )
        return;

    const SCRIPT_LOGATTR* pLogAttr = ScriptString_pLogAttr( m_Analysis );
    if( !pLogAttr )
        return;

    if( !ScriptString_pcOutChars( m_Analysis ) )
        return;
    int nInitial = *ScriptString_pcOutChars( m_Analysis );
    if( nCP + 1 < nInitial )
        nInitial = nCP + 1;

    int i = nInitial;
    int limit = *ScriptString_pcOutChars( m_Analysis );
    while( limit > 0 && i < limit - 1 )
    {
        if( pLogAttr[i].fWordStop )      // Either the fWordStop flag is set
        {
            *pPrior = i;
            return;
        }
        else if( pLogAttr[i].fWhiteSpace &&  // Or this whitespace but the next char isn't.
                 !pLogAttr[i + 1].fWhiteSpace )
        {
            *pPrior = i + 1;  // The next char is a word stop
            return;
        }

        ++i;
        limit = *ScriptString_pcOutChars( m_Analysis );
    }
    // We have reached the end. It's always a word stop, so simply return it.
    *pPrior = *ScriptString_pcOutChars( m_Analysis ) - 1;
}


//======================================================================================
// DXUTBlendColor
//======================================================================================

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void DXUTBlendColor::Init( DWORD defaultColor, DWORD disabledColor, DWORD hiddenColor )
{
    for( int i = 0; i < MAX_CONTROL_STATES; i++ )
    {
        States[ i ] = defaultColor;
    }

    States[ DXUT_STATE_DISABLED ] = disabledColor;
    States[ DXUT_STATE_HIDDEN ] = hiddenColor;
    SetCurrent( hiddenColor );
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void DXUTBlendColor::Blend( UINT iState, float fElapsedTime, float fRate )
{
    XMFLOAT4 destColor = D3DCOLOR_TO_D3DCOLORVALUE( States[ iState ] );
    XMVECTOR clr1 = XMLoadFloat4( &destColor );
    XMVECTOR clr = XMLoadFloat4( &Current );
    clr = XMVectorLerp( clr, clr1, 1.0f - powf( fRate, 30 * fElapsedTime ) );
    XMStoreFloat4( &Current, clr );
}


//--------------------------------------------------------------------------------------
void DXUTBlendColor::SetCurrent( DWORD color )
{
    Current = D3DCOLOR_TO_D3DCOLORVALUE( color );
}


//======================================================================================
// CDXUTElement
//======================================================================================

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTElement::SetTexture( UINT texture, RECT* prcTexture, DWORD defaultTextureColor )
{
    iTexture = texture;

    if( prcTexture )
        rcTexture = *prcTexture;
    else
        SetRectEmpty( &rcTexture );

    TextureColor.Init( defaultTextureColor );
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTElement::SetFont( UINT font, DWORD defaultFontColor, DWORD textFormat )
{
    iFont = font;
    dwTextFormat = textFormat;

    FontColor.Init( defaultFontColor );
}


//--------------------------------------------------------------------------------------
void CDXUTElement::Refresh()
{
    TextureColor.SetCurrent( TextureColor.States[ DXUT_STATE_HIDDEN ] );
    FontColor.SetCurrent( FontColor.States[ DXUT_STATE_HIDDEN ] );
}
