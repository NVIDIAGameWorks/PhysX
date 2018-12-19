//--------------------------------------------------------------------------------------
// File: DXUTgui.h
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

#include <usp10.h>
#include <dimm.h>

#ifdef DXUT_AUTOLIB
#pragma comment( lib, "usp10.lib" )
#endif

//--------------------------------------------------------------------------------------
// Defines and macros 
//--------------------------------------------------------------------------------------
#define EVENT_BUTTON_CLICKED                0x0101
#define EVENT_COMBOBOX_SELECTION_CHANGED    0x0201
#define EVENT_RADIOBUTTON_CHANGED           0x0301
#define EVENT_CHECKBOX_CHANGED              0x0401
#define EVENT_SLIDER_VALUE_CHANGED          0x0501
#define EVENT_SLIDER_VALUE_CHANGED_UP       0x0502

#define EVENT_EDITBOX_STRING                0x0601
// EVENT_EDITBOX_CHANGE is sent when the listbox content changes
// due to user input.
#define EVENT_EDITBOX_CHANGE                0x0602
#define EVENT_LISTBOX_ITEM_DBLCLK           0x0701
// EVENT_LISTBOX_SELECTION is fired off when the selection changes in
// a single selection list box.
#define EVENT_LISTBOX_SELECTION             0x0702
#define EVENT_LISTBOX_SELECTION_END         0x0703


//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
class CDXUTDialogResourceManager;
class CDXUTControl;
class CDXUTButton;
class CDXUTStatic;
class CDXUTCheckBox;
class CDXUTRadioButton;
class CDXUTComboBox;
class CDXUTSlider;
class CDXUTEditBox;
class CDXUTListBox;
class CDXUTScrollBar;
class CDXUTElement;
struct DXUTElementHolder;
struct DXUTTextureNode;
struct DXUTFontNode;
typedef void ( CALLBACK*PCALLBACKDXUTGUIEVENT )( _In_ UINT nEvent, _In_ int nControlID, _In_ CDXUTControl* pControl,
                                                 _In_opt_ void* pUserContext );


//--------------------------------------------------------------------------------------
// Enums for pre-defined control types
//--------------------------------------------------------------------------------------
enum DXUT_CONTROL_TYPE
{
    DXUT_CONTROL_BUTTON,
    DXUT_CONTROL_STATIC,
    DXUT_CONTROL_CHECKBOX,
    DXUT_CONTROL_RADIOBUTTON,
    DXUT_CONTROL_COMBOBOX,
    DXUT_CONTROL_SLIDER,
    DXUT_CONTROL_EDITBOX,
    DXUT_CONTROL_IMEEDITBOX,
    DXUT_CONTROL_LISTBOX,
    DXUT_CONTROL_SCROLLBAR,
};

enum DXUT_CONTROL_STATE
{
    DXUT_STATE_NORMAL = 0,
    DXUT_STATE_DISABLED,
    DXUT_STATE_HIDDEN,
    DXUT_STATE_FOCUS,
    DXUT_STATE_MOUSEOVER,
    DXUT_STATE_PRESSED,
};

#define MAX_CONTROL_STATES 6

struct DXUTBlendColor
{
    void Init( _In_ DWORD defaultColor, _In_ DWORD disabledColor = D3DCOLOR_ARGB( 200, 128, 128, 128 ), _In_ DWORD hiddenColor = 0 );
    void Blend( _In_ UINT iState, _In_ float fElapsedTime, _In_ float fRate = 0.7f );

    DWORD States[ MAX_CONTROL_STATES ]; // Modulate colors for all possible control states
    DirectX::XMFLOAT4 Current;

    void SetCurrent( DWORD color );
};


//-----------------------------------------------------------------------------
// Contains all the display tweakables for a sub-control
//-----------------------------------------------------------------------------
class CDXUTElement
{
public:
    void SetTexture( _In_ UINT texture, _In_ RECT* prcTexture, _In_ DWORD defaultTextureColor = D3DCOLOR_ARGB( 255, 255, 255, 255 ) );
    void SetFont( _In_ UINT font, _In_ DWORD defaultFontColor = D3DCOLOR_ARGB( 255, 255, 255, 255 ), DWORD textFormat = DT_CENTER | DT_VCENTER );

    void Refresh();

    UINT iTexture;          // Index of the texture for this Element 
    UINT iFont;             // Index of the font for this Element
    DWORD dwTextFormat;     // The format argument to DrawText 

    RECT rcTexture;         // Bounding rect of this element on the composite texture

    DXUTBlendColor TextureColor;
    DXUTBlendColor FontColor;
};


//-----------------------------------------------------------------------------
// All controls must be assigned to a dialog, which handles
// input and rendering for the controls.
//-----------------------------------------------------------------------------
class CDXUTDialog
{
    friend class CDXUTDialogResourceManager;

public:
    CDXUTDialog();
    ~CDXUTDialog();

    // Need to call this now
    void Init( _In_ CDXUTDialogResourceManager* pManager, _In_ bool bRegisterDialog = true );
    void Init( _In_ CDXUTDialogResourceManager* pManager, _In_ bool bRegisterDialog,
               _In_z_ LPCWSTR pszControlTextureFilename );
    void Init( _In_ CDXUTDialogResourceManager* pManager, _In_ bool bRegisterDialog,
               _In_z_ LPCWSTR szControlTextureResourceName, _In_ HMODULE hControlTextureResourceModule );

    // Windows message handler
    bool MsgProc( _In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam );

    // Control creation
    HRESULT AddStatic( _In_ int ID, _In_z_ LPCWSTR strText, _In_ int x, _In_ int y, _In_ int width, _In_ int height, _In_ bool bIsDefault=false,
                       _Out_opt_ CDXUTStatic** ppCreated = nullptr );
    HRESULT AddButton( _In_ int ID, _In_z_ LPCWSTR strText, _In_ int x, _In_ int y, _In_ int width, _In_ int height, _In_ UINT nHotkey=0,
                       _In_ bool bIsDefault=false, _Out_opt_ CDXUTButton** ppCreated = nullptr );
    HRESULT AddCheckBox( _In_ int ID, _In_z_ LPCWSTR strText, _In_ int x, _In_ int y, _In_ int width, _In_ int height, _In_ bool bChecked=false,
                         _In_ UINT nHotkey=0, _In_ bool bIsDefault=false, _Out_opt_ CDXUTCheckBox** ppCreated = nullptr );
    HRESULT AddRadioButton( _In_ int ID, _In_ UINT nButtonGroup, _In_z_ LPCWSTR strText, _In_ int x, _In_ int y, _In_ int width,
                            _In_ int height, _In_ bool bChecked=false, _In_ UINT nHotkey=0, _In_ bool bIsDefault=false,
                            _Out_opt_ CDXUTRadioButton** ppCreated = nullptr );
    HRESULT AddComboBox( _In_ int ID, _In_ int x, _In_ int y, _In_ int width, _In_ int height, _In_ UINT nHotKey=0, _In_ bool bIsDefault=false,
                         _Out_opt_ CDXUTComboBox** ppCreated = nullptr );
    HRESULT AddSlider( _In_ int ID, _In_ int x, _In_ int y, _In_ int width, _In_ int height, _In_ int min=0, _In_ int max=100, _In_ int value=50,
                       _In_ bool bIsDefault=false, _Out_opt_ CDXUTSlider** ppCreated = nullptr );
    //      AddIMEEditBox has been renamed into DXUTguiIME.cpp as CDXUTIMEEditBox::CreateIMEEditBox
    HRESULT AddEditBox( _In_ int ID, _In_z_ LPCWSTR strText, _In_ int x, _In_ int y, _In_ int width, _In_ int height, _In_ bool bIsDefault=false,
                        _Out_opt_ CDXUTEditBox** ppCreated = nullptr );
    HRESULT AddListBox( _In_ int ID, _In_ int x, _In_ int y, _In_ int width, _In_ int height, _In_ DWORD dwStyle=0,
                        _Out_opt_ CDXUTListBox** ppCreated = nullptr );
    HRESULT AddControl( _In_ CDXUTControl* pControl );
    HRESULT InitControl( _In_ CDXUTControl* pControl );

    // Control retrieval
    CDXUTStatic* GetStatic( _In_ int ID ) const
    {
        return reinterpret_cast<CDXUTStatic*>( GetControl( ID, DXUT_CONTROL_STATIC ) );
    }
    CDXUTButton* GetButton( _In_ int ID ) const
    {
        return reinterpret_cast<CDXUTButton*>( GetControl(ID, DXUT_CONTROL_BUTTON) );
    }
    CDXUTCheckBox* GetCheckBox( _In_ int ID ) const
    {
        return reinterpret_cast<CDXUTCheckBox*>( GetControl(ID, DXUT_CONTROL_CHECKBOX) );
    }
    CDXUTRadioButton* GetRadioButton( _In_ int ID ) const
    {
        return reinterpret_cast<CDXUTRadioButton*>( GetControl(ID, DXUT_CONTROL_RADIOBUTTON) );
    }
    CDXUTComboBox* GetComboBox( _In_ int ID ) const
    {
        return reinterpret_cast<CDXUTComboBox*>( GetControl(ID, DXUT_CONTROL_COMBOBOX) );
    }
    CDXUTSlider* GetSlider( _In_ int ID ) const
    {
        return reinterpret_cast<CDXUTSlider*>( GetControl(ID, DXUT_CONTROL_SLIDER) );
    }
    CDXUTEditBox* GetEditBox( _In_ int ID ) const
    {
        return reinterpret_cast<CDXUTEditBox*>( GetControl(ID, DXUT_CONTROL_EDITBOX) );
    }
    CDXUTListBox* GetListBox( _In_ int ID ) const
    {
        return reinterpret_cast<CDXUTListBox*>( GetControl(ID, DXUT_CONTROL_LISTBOX) );
    }

    CDXUTControl* GetControl( _In_ int ID ) const;
    CDXUTControl* GetControl( _In_ int ID, _In_ UINT nControlType ) const;
    CDXUTControl* GetControlAtPoint( _In_ const POINT& pt ) const;

    bool GetControlEnabled( _In_ int ID ) const;
    void SetControlEnabled( _In_ int ID, _In_ bool bEnabled );

    void ClearRadioButtonGroup( _In_ UINT nGroup );
    void ClearComboBox( _In_ int ID );

    // Access the default display Elements used when adding new controls
    HRESULT SetDefaultElement( _In_ UINT nControlType, _In_ UINT iElement, _In_ CDXUTElement* pElement );
    CDXUTElement* GetDefaultElement( _In_ UINT nControlType, _In_ UINT iElement ) const;

    // Methods called by controls
    void SendEvent( _In_ UINT nEvent, _In_ bool bTriggeredByUser, _In_ CDXUTControl* pControl );
    void RequestFocus( _In_ CDXUTControl* pControl );

    // Render helpers
    HRESULT DrawRect( _In_ const RECT* pRect, _In_ DWORD color );
    HRESULT DrawSprite( _In_ CDXUTElement* pElement, _In_ const RECT* prcDest, _In_ float fDepth );
    HRESULT DrawSprite11( _In_ CDXUTElement* pElement, _In_ const RECT* prcDest, _In_ float fDepth );
    HRESULT CalcTextRect( _In_z_ LPCWSTR strText, _In_ CDXUTElement* pElement, _In_ const RECT* prcDest, _In_ int nCount = -1 );
    HRESULT DrawText( _In_z_ LPCWSTR strText, _In_ CDXUTElement* pElement, _In_ const RECT* prcDest, _In_ bool bShadow = false,
                      _In_ bool bCenter = false  );

    // Attributes
    bool GetVisible() const { return m_bVisible; }
    void SetVisible( _In_ bool bVisible ) { m_bVisible = bVisible; }
    bool GetMinimized() const { return m_bMinimized; }
    void SetMinimized( _In_ bool bMinimized ) {m_bMinimized = bMinimized; }
    void SetBackgroundColors( _In_ DWORD colorAllCorners ) { SetBackgroundColors( colorAllCorners, colorAllCorners, colorAllCorners, colorAllCorners ); }
    void SetBackgroundColors( _In_ DWORD colorTopLeft, _In_ DWORD colorTopRight, _In_ DWORD colorBottomLeft, _In_ DWORD colorBottomRight );
    void EnableCaption( _In_ bool bEnable ) { m_bCaption = bEnable; }
    int GetCaptionHeight() const { return m_nCaptionHeight; }
    void SetCaptionHeight( _In_ int nHeight ) { m_nCaptionHeight = nHeight; }
    void SetCaptionText( _In_ const WCHAR* pwszText ) { wcscpy_s( m_wszCaption, sizeof( m_wszCaption ) / sizeof( m_wszCaption[0] ), pwszText ); }
    void GetLocation( _Out_ POINT& Pt ) const
    {
        Pt.x = m_x;
        Pt.y = m_y;
    }
    void SetLocation( _In_ int x, _In_ int y )
    {
        m_x = x;
        m_y = y;
    }
    void SetSize( _In_ int width, _In_ int height )
    {
        m_width = width;
        m_height = height;
    }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

    static void WINAPI SetRefreshTime( _In_ float fTime ) { s_fTimeRefresh = fTime; }

    static CDXUTControl* WINAPI GetNextControl( _In_ CDXUTControl* pControl );
    static CDXUTControl* WINAPI GetPrevControl( _In_ CDXUTControl* pControl );

    void RemoveControl( _In_ int ID );
    void RemoveAllControls();

    // Sets the callback used to notify the app of control events
    void SetCallback( _In_ PCALLBACKDXUTGUIEVENT pCallback, _In_opt_ void* pUserContext = nullptr );
    void EnableNonUserEvents( _In_ bool bEnable ) { m_bNonUserEvents = bEnable; }
    void EnableKeyboardInput( _In_ bool bEnable ) { m_bKeyboardInput = bEnable; }
    void EnableMouseInput( _In_ bool bEnable ) { m_bMouseInput = bEnable; }
    bool IsKeyboardInputEnabled() const { return m_bKeyboardInput; }

    // Device state notification
    void Refresh();
    HRESULT OnRender( _In_ float fElapsedTime );

    // Shared resource access. Indexed fonts and textures are shared among
    // all the controls.
    HRESULT SetFont( _In_ UINT index, _In_z_ LPCWSTR strFaceName, _In_ LONG height, _In_ LONG weight );
    DXUTFontNode* GetFont( _In_ UINT index ) const;

    HRESULT SetTexture( _In_ UINT index, _In_z_ LPCWSTR strFilename );
    HRESULT SetTexture( _In_ UINT index, _In_z_ LPCWSTR strResourceName, _In_ HMODULE hResourceModule );
    DXUTTextureNode* GetTexture( _In_ UINT index ) const;

    CDXUTDialogResourceManager* GetManager() const { return m_pManager; }

    static void WINAPI  ClearFocus();
    void FocusDefaultControl();

    bool m_bNonUserEvents;
    bool m_bKeyboardInput;
    bool m_bMouseInput;

private:
    int m_nDefaultControlID;

    static double s_fTimeRefresh;
    double m_fTimeLastRefresh;

    // Initialize default Elements
    void InitDefaultElements();

    // Windows message handlers
    void OnMouseMove( _In_ const POINT& pt );
    void OnMouseUp( _In_ const POINT& pt );

    void SetNextDialog( _In_ CDXUTDialog* pNextDialog );

    // Control events
    bool OnCycleFocus( _In_ bool bForward );

    static CDXUTControl* s_pControlFocus;        // The control which has focus
    static CDXUTControl* s_pControlPressed;      // The control currently pressed

    CDXUTControl* m_pControlMouseOver;           // The control which is hovered over

    bool m_bVisible;
    bool m_bCaption;
    bool m_bMinimized;
    bool m_bDrag;
    WCHAR m_wszCaption[256];

    int m_x;
    int m_y;
    int m_width;
    int m_height;
    int m_nCaptionHeight;

    DWORD m_colorTopLeft;
    DWORD m_colorTopRight;
    DWORD m_colorBottomLeft;
    DWORD m_colorBottomRight;

    CDXUTDialogResourceManager* m_pManager;
    PCALLBACKDXUTGUIEVENT m_pCallbackEvent;
    void* m_pCallbackEventUserContext;

    std::vector<int> m_Textures;   // Index into m_TextureCache;
    std::vector<int> m_Fonts;      // Index into m_FontCache;

    std::vector<CDXUTControl*> m_Controls;
    std::vector<DXUTElementHolder*> m_DefaultElements;

    CDXUTElement m_CapElement;  // Element for the caption

    CDXUTDialog* m_pNextDialog;
    CDXUTDialog* m_pPrevDialog;
};


//--------------------------------------------------------------------------------------
// Structs for shared resources
//--------------------------------------------------------------------------------------
struct DXUTTextureNode
{
    bool bFileSource;  // True if this texture is loaded from a file. False if from resource.
    HMODULE hResourceModule;
    int nResourceID;   // Resource ID. If 0, string-based ID is used and stored in strFilename.
    WCHAR strFilename[MAX_PATH];
    DWORD dwWidth;
    DWORD dwHeight;
    ID3D11Texture2D* pTexture11;
    ID3D11ShaderResourceView* pTexResView11;
};

struct DXUTFontNode
{
    WCHAR strFace[MAX_PATH];
    LONG nHeight;
    LONG nWeight;
};

struct DXUTSpriteVertex
{
    DirectX::XMFLOAT3 vPos;
    DirectX::XMFLOAT4 vColor;
    DirectX::XMFLOAT2 vTex;
};


//-----------------------------------------------------------------------------
// Manages shared resources of dialogs
//-----------------------------------------------------------------------------
class CDXUTDialogResourceManager
{
public:
    CDXUTDialogResourceManager();
    ~CDXUTDialogResourceManager();

    bool MsgProc( _In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam );

    // D3D11 specific
    HRESULT OnD3D11CreateDevice( _In_ ID3D11Device* pd3dDevice, _In_ ID3D11DeviceContext* pd3d11DeviceContext );
    HRESULT OnD3D11ResizedSwapChain( _In_ ID3D11Device* pd3dDevice, _In_ const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc );
    void OnD3D11ReleasingSwapChain();
    void OnD3D11DestroyDevice();
    void StoreD3D11State( _In_ ID3D11DeviceContext* pd3dImmediateContext );
    void RestoreD3D11State( _In_ ID3D11DeviceContext* pd3dImmediateContext );
    void ApplyRenderUI11( _In_ ID3D11DeviceContext* pd3dImmediateContext );
    void ApplyRenderUIUntex11( _In_ ID3D11DeviceContext* pd3dImmediateContext );
    void BeginSprites11( );
    void EndSprites11( _In_ ID3D11Device* pd3dDevice, _In_ ID3D11DeviceContext* pd3dImmediateContext );

    ID3D11Device* GetD3D11Device() const { return m_pd3d11Device; }
    ID3D11DeviceContext* GetD3D11DeviceContext() const { return m_pd3d11DeviceContext; }

    DXUTFontNode* GetFontNode( _In_ size_t iIndex ) const { return m_FontCache[ iIndex ]; }
    DXUTTextureNode* GetTextureNode( _In_ size_t iIndex ) const { return m_TextureCache[ iIndex ]; }

    int AddFont( _In_z_ LPCWSTR strFaceName, _In_ LONG height, _In_ LONG weight );
    int AddTexture( _In_z_ LPCWSTR strFilename );
    int AddTexture( _In_z_ LPCWSTR strResourceName, _In_ HMODULE hResourceModule );

    bool RegisterDialog( _In_ CDXUTDialog* pDialog );
    void UnregisterDialog( _In_ CDXUTDialog* pDialog );
    void EnableKeyboardInputForAllDialogs();

    // Shared between all dialogs

    // D3D11
    // Shaders
    ID3D11VertexShader* m_pVSRenderUI11;
    ID3D11PixelShader* m_pPSRenderUI11;
    ID3D11PixelShader* m_pPSRenderUIUntex11;

    // States
    ID3D11DepthStencilState* m_pDepthStencilStateUI11;
    ID3D11RasterizerState* m_pRasterizerStateUI11;
    ID3D11BlendState* m_pBlendStateUI11;
    ID3D11SamplerState* m_pSamplerStateUI11;

    // Stored states
    ID3D11DepthStencilState* m_pDepthStencilStateStored11;
    UINT m_StencilRefStored11;
    ID3D11RasterizerState* m_pRasterizerStateStored11;
    ID3D11BlendState* m_pBlendStateStored11;
    float m_BlendFactorStored11[4];
    UINT m_SampleMaskStored11;
    ID3D11SamplerState* m_pSamplerStateStored11;

    ID3D11InputLayout* m_pInputLayout11;
    ID3D11Buffer* m_pVBScreenQuad11;

    // Sprite workaround
    ID3D11Buffer* m_pSpriteBuffer11;
    UINT m_SpriteBufferBytes11;
    std::vector<DXUTSpriteVertex> m_SpriteVertices;

    UINT m_nBackBufferWidth;
    UINT m_nBackBufferHeight;

    std::vector<CDXUTDialog*> m_Dialogs;            // Dialogs registered

protected:
    // D3D11 specific
    ID3D11Device* m_pd3d11Device;
    ID3D11DeviceContext* m_pd3d11DeviceContext;
    HRESULT CreateTexture11( _In_ UINT index );

    std::vector<DXUTTextureNode*> m_TextureCache;   // Shared textures
    std::vector<DXUTFontNode*> m_FontCache;         // Shared fonts
};


//-----------------------------------------------------------------------------
// Base class for controls
//-----------------------------------------------------------------------------
class CDXUTControl
{
public:
    CDXUTControl( _In_opt_ CDXUTDialog* pDialog = nullptr );
    virtual ~CDXUTControl();

    virtual HRESULT OnInit() { return S_OK; }
    virtual void Refresh();
    virtual void Render( _In_ float fElapsedTime ) { UNREFERENCED_PARAMETER(fElapsedTime); }

    // Windows message handler
    virtual bool MsgProc( _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam )
    {
        UNREFERENCED_PARAMETER(uMsg);
        UNREFERENCED_PARAMETER(wParam);
        UNREFERENCED_PARAMETER(lParam);
        return false;
    }

    virtual bool HandleKeyboard( _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam )
    {
        UNREFERENCED_PARAMETER(uMsg);
        UNREFERENCED_PARAMETER(wParam);
        UNREFERENCED_PARAMETER(lParam);
        return false;
    }
    virtual bool HandleMouse( _In_ UINT uMsg, _In_ const POINT& pt, _In_ WPARAM wParam, _In_ LPARAM lParam )
    {
        UNREFERENCED_PARAMETER(uMsg);
        UNREFERENCED_PARAMETER(pt);
        UNREFERENCED_PARAMETER(wParam);
        UNREFERENCED_PARAMETER(lParam);
        return false;
    }

    virtual bool CanHaveFocus() { return false; }
    virtual void OnFocusIn() { m_bHasFocus = true; }
    virtual void OnFocusOut() { m_bHasFocus = false; }
    virtual void OnMouseEnter() { m_bMouseOver = true; }
    virtual void OnMouseLeave() { m_bMouseOver = false; }
    virtual void OnHotkey() { }

    virtual bool ContainsPoint( _In_ const POINT& pt ) { return PtInRect( &m_rcBoundingBox, pt ) != 0; }

    virtual void SetEnabled( _In_ bool bEnabled ) { m_bEnabled = bEnabled; }
    virtual bool GetEnabled() const { return m_bEnabled; }
    virtual void SetVisible( _In_ bool bVisible ) { m_bVisible = bVisible; }
    virtual bool GetVisible() const { return m_bVisible; }

    UINT GetType() const { return m_Type; }

    int GetID() const { return m_ID; }
    void SetID( _In_ int ID ) { m_ID = ID; }

    void SetLocation( _In_ int x, _In_ int y )
    {
        m_x = x;
        m_y = y;
        UpdateRects();
    }
    void SetSize( int width, int height )
    {
        m_width = width;
        m_height = height;
        UpdateRects();
    }

    void SetHotkey( _In_ UINT nHotkey ) { m_nHotkey = nHotkey; }
    UINT GetHotkey() const { return m_nHotkey; }

    void SetUserData( _In_opt_ void* pUserData ) { m_pUserData = pUserData; }
    void* GetUserData() const { return m_pUserData; }

    virtual void SetTextColor( _In_ DWORD Color );
    CDXUTElement* GetElement( _In_ UINT iElement ) const { return m_Elements[ iElement ]; }
    HRESULT SetElement( _In_ UINT iElement, _In_ CDXUTElement* pElement );

    bool m_bVisible;                // Shown/hidden flag
    bool m_bMouseOver;              // Mouse pointer is above control
    bool m_bHasFocus;               // Control has input focus
    bool m_bIsDefault;              // Is the default control

    // Size, scale, and positioning members
    int m_x, m_y;
    int m_width, m_height;

    // These members are set by the container
    CDXUTDialog* m_pDialog;    // Parent container
    UINT m_Index;              // Index within the control list

    std::vector<CDXUTElement*> m_Elements;  // All display elements

protected:
    virtual void UpdateRects();

    int m_ID;                 // ID number
    DXUT_CONTROL_TYPE m_Type;  // Control type, set once in constructor  
    UINT m_nHotkey;            // Virtual key code for this control's hotkey
    void* m_pUserData;         // Data associated with this control that is set by user.

    bool m_bEnabled;           // Enabled/disabled flag

    RECT m_rcBoundingBox;      // Rectangle defining the active region of the control
};


//-----------------------------------------------------------------------------
// Contains all the display information for a given control type
//-----------------------------------------------------------------------------
struct DXUTElementHolder
{
    UINT nControlType;
    UINT iElement;

    CDXUTElement Element;
};


//-----------------------------------------------------------------------------
// Static control
//-----------------------------------------------------------------------------
class CDXUTStatic : public CDXUTControl
{
public:
    CDXUTStatic( _In_opt_ CDXUTDialog* pDialog = nullptr );

    virtual void Render( _In_ float fElapsedTime ) override;
    virtual bool ContainsPoint( _In_ const POINT& pt ) override
    {
        UNREFERENCED_PARAMETER( pt );
        return false;
    }

    HRESULT GetTextCopy( _Out_writes_(bufferCount) LPWSTR strDest, _In_ UINT bufferCount ) const;
    LPCWSTR GetText() const { return m_strText; }
    HRESULT SetText( _In_z_ LPCWSTR strText );

protected:
    WCHAR m_strText[MAX_PATH];      // Window text  
};


//-----------------------------------------------------------------------------
// Button control
//-----------------------------------------------------------------------------
class CDXUTButton : public CDXUTStatic
{
public:
    CDXUTButton( _In_opt_ CDXUTDialog* pDialog = nullptr );

    virtual bool HandleKeyboard( _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam ) override;
    virtual bool HandleMouse( _In_ UINT uMsg, _In_ const POINT& pt, _In_ WPARAM wParam, _In_ LPARAM lParam ) override;
    virtual void OnHotkey() override
    {
        if( m_pDialog->IsKeyboardInputEnabled() ) m_pDialog->RequestFocus( this );
        m_pDialog->SendEvent( EVENT_BUTTON_CLICKED, true, this );
    }

    virtual bool ContainsPoint( _In_ const POINT& pt ) override
    {
        return PtInRect( &m_rcBoundingBox, pt ) != 0;
    }
    virtual bool CanHaveFocus() override
    {
        return ( m_bVisible && m_bEnabled );
    }

    virtual void Render( _In_ float fElapsedTime ) override;

protected:
    bool m_bPressed;
};


//-----------------------------------------------------------------------------
// CheckBox control
//-----------------------------------------------------------------------------
class CDXUTCheckBox : public CDXUTButton
{
public:
    CDXUTCheckBox( _In_opt_ CDXUTDialog* pDialog = nullptr );

    virtual bool HandleKeyboard( _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam ) override;
    virtual bool HandleMouse( _In_ UINT uMsg, _In_ const POINT& pt, _In_ WPARAM wParam, _In_ LPARAM lParam ) override;
    virtual void OnHotkey() override
    {
        if( m_pDialog->IsKeyboardInputEnabled() ) m_pDialog->RequestFocus( this );
        SetCheckedInternal( !m_bChecked, true );
    }

    virtual bool ContainsPoint( _In_ const POINT& pt ) override;
    virtual void UpdateRects() override;

    virtual void Render( _In_ float fElapsedTime ) override;

    bool GetChecked() const { return m_bChecked; }
    void SetChecked( _In_ bool bChecked ) { SetCheckedInternal( bChecked, false ); }

protected:
    virtual void SetCheckedInternal( _In_ bool bChecked, _In_ bool bFromInput );

    bool m_bChecked;
    RECT m_rcButton;
    RECT m_rcText;
};


//-----------------------------------------------------------------------------
// RadioButton control
//-----------------------------------------------------------------------------
class CDXUTRadioButton : public CDXUTCheckBox
{
public:
    CDXUTRadioButton( _In_opt_ CDXUTDialog* pDialog = nullptr );

    virtual bool HandleKeyboard( _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam ) override;
    virtual bool HandleMouse( _In_ UINT uMsg, _In_ const POINT& pt, _In_ WPARAM wParam, _In_ LPARAM lParam ) override;
    virtual void OnHotkey() override
    {
        if( m_pDialog->IsKeyboardInputEnabled() ) m_pDialog->RequestFocus( this );
        SetCheckedInternal( true, true, true );
    }

    void SetChecked( _In_ bool bChecked, _In_ bool bClearGroup=true ) { SetCheckedInternal( bChecked, bClearGroup, false ); }
    void SetButtonGroup( _In_ UINT nButtonGroup ) { m_nButtonGroup = nButtonGroup; }
    UINT GetButtonGroup() const { return m_nButtonGroup; }

protected:
    virtual void SetCheckedInternal( _In_ bool bChecked, _In_ bool bClearGroup, _In_ bool bFromInput );
    UINT m_nButtonGroup;
};


//-----------------------------------------------------------------------------
// Scrollbar control
//-----------------------------------------------------------------------------
class CDXUTScrollBar : public CDXUTControl
{
public:
    CDXUTScrollBar( _In_opt_ CDXUTDialog* pDialog = nullptr );
    virtual ~CDXUTScrollBar();

    virtual bool HandleKeyboard( _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam ) override;
    virtual bool HandleMouse( _In_ UINT uMsg, _In_ const POINT& pt, _In_ WPARAM wParam, _In_ LPARAM lParam ) override;
    virtual bool MsgProc( _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam ) override;

    virtual void Render( _In_ float fElapsedTime ) override;
    virtual void UpdateRects() override;

    void SetTrackRange( _In_ int nStart, _In_ int nEnd );
    int GetTrackPos() const { return m_nPosition; }
    void SetTrackPos( _In_ int nPosition )
    {
        m_nPosition = nPosition;
        Cap();
        UpdateThumbRect();
    }
    int GetPageSize() const { return m_nPageSize; }
    void SetPageSize( _In_ int nPageSize )
    {
        m_nPageSize = nPageSize;
        Cap();
        UpdateThumbRect();
    }

    void Scroll( _In_ int nDelta );    // Scroll by nDelta items (plus or minus)
    void ShowItem( _In_ int nIndex );  // Ensure that item nIndex is displayed, scroll if necessary

protected:
    // ARROWSTATE indicates the state of the arrow buttons.
    // CLEAR            No arrow is down.
    // CLICKED_UP       Up arrow is clicked.
    // CLICKED_DOWN     Down arrow is clicked.
    // HELD_UP          Up arrow is held down for sustained period.
    // HELD_DOWN        Down arrow is held down for sustained period.
    enum ARROWSTATE
    {
        CLEAR,
        CLICKED_UP,
        CLICKED_DOWN,
        HELD_UP,
        HELD_DOWN
    };

    void            UpdateThumbRect();
    void            Cap();  // Clips position at boundaries. Ensures it stays within legal range.

    bool m_bShowThumb;
    bool m_bDrag;
    RECT m_rcUpButton;
    RECT m_rcDownButton;
    RECT m_rcTrack;
    RECT m_rcThumb;
    int m_nPosition;  // Position of the first displayed item
    int m_nPageSize;  // How many items are displayable in one page
    int m_nStart;     // First item
    int m_nEnd;       // The index after the last item
    POINT m_LastMouse;// Last mouse position
    ARROWSTATE m_Arrow; // State of the arrows
    double m_dArrowTS;  // Timestamp of last arrow event.
};


//-----------------------------------------------------------------------------
// ListBox control
//-----------------------------------------------------------------------------
struct DXUTListBoxItem
{
    WCHAR strText[256];
    void* pData;

    RECT rcActive;
    bool bSelected;
};

class CDXUTListBox : public CDXUTControl
{
public:
    CDXUTListBox( _In_opt_ CDXUTDialog* pDialog = nullptr );
    virtual ~CDXUTListBox();

    virtual HRESULT OnInit() override
    {
        return m_pDialog->InitControl( &m_ScrollBar );
    }
    virtual bool CanHaveFocus() override
    {
        return ( m_bVisible && m_bEnabled );
    }
    virtual bool HandleKeyboard( _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam )  override;
    virtual bool HandleMouse( _In_ UINT uMsg, _In_ const POINT& pt, _In_ WPARAM wParam, _In_ LPARAM lParam )  override;
    virtual bool MsgProc( _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam )  override;

    virtual void Render( _In_ float fElapsedTime ) override;
    virtual void UpdateRects() override;

    DWORD GetStyle() const { return m_dwStyle; }
    size_t GetSize() const { return m_Items.size(); }
    void SetStyle( _In_ DWORD dwStyle ) { m_dwStyle = dwStyle; }
    int GetScrollBarWidth() const{ return m_nSBWidth; }
    void SetScrollBarWidth( _In_ int nWidth )
    {
        m_nSBWidth = nWidth;
        UpdateRects();
    }
    void SetBorder( _In_ int nBorder, _In_ int nMargin )
    {
        m_nBorder = nBorder;
        m_nMargin = nMargin;
    }
    HRESULT AddItem( _In_z_ const WCHAR* wszText, _In_opt_ void* pData );
    HRESULT InsertItem( _In_ int nIndex, _In_z_ const WCHAR* wszText, _In_opt_ void* pData );
    void    RemoveItem( _In_ int nIndex );
    void    RemoveAllItems();

    DXUTListBoxItem* GetItem( _In_ int nIndex ) const;
    int              GetSelectedIndex( _In_ int nPreviousSelected = -1 ) const;
    DXUTListBoxItem* GetSelectedItem( _In_ int nPreviousSelected = -1 ) const
    {
        return GetItem( GetSelectedIndex( nPreviousSelected ) );
    }
    void             SelectItem( _In_ int nNewIndex );

    enum STYLE
    {
        MULTISELECTION = 1
    };

protected:
    RECT m_rcText;      // Text rendering bound
    RECT m_rcSelection; // Selection box bound
    CDXUTScrollBar m_ScrollBar;
    int m_nSBWidth;
    int m_nBorder;
    int m_nMargin;
    int m_nTextHeight;  // Height of a single line of text
    DWORD m_dwStyle;    // List box style
    int m_nSelected;    // Index of the selected item for single selection list box
    int m_nSelStart;    // Index of the item where selection starts (for handling multi-selection)
    bool m_bDrag;       // Whether the user is dragging the mouse to select

    std::vector<DXUTListBoxItem*> m_Items;
};


//-----------------------------------------------------------------------------
// ComboBox control
//-----------------------------------------------------------------------------
struct DXUTComboBoxItem
{
    WCHAR strText[256];
    void* pData;

    RECT rcActive;
    bool bVisible;
};

class CDXUTComboBox : public CDXUTButton
{
public:
    CDXUTComboBox( _In_opt_ CDXUTDialog* pDialog = nullptr );
    virtual ~CDXUTComboBox();

    virtual void SetTextColor( _In_ DWORD Color ) override;
    virtual HRESULT OnInit() override
    {
        return m_pDialog->InitControl( &m_ScrollBar );
    }

    virtual bool HandleKeyboard( _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam ) override;
    virtual bool HandleMouse( _In_ UINT uMsg, _In_ const POINT& pt, _In_ WPARAM wParam, _In_ LPARAM lParam ) override;
    virtual void OnHotkey() override;

    virtual bool CanHaveFocus() override
    {
        return ( m_bVisible && m_bEnabled );
    }
    virtual void OnFocusOut() override;
    virtual void Render( _In_ float fElapsedTime ) override;

    virtual void UpdateRects() override;

    HRESULT AddItem( _In_z_ const WCHAR* strText, _In_opt_ void* pData );
    void    RemoveAllItems();
    void    RemoveItem( _In_ UINT index );
    bool    ContainsItem( _In_z_ const WCHAR* strText, _In_ UINT iStart=0 );
    int     FindItem( _In_z_ const WCHAR* strText, _In_ UINT iStart=0 ) const;
    void*   GetItemData( _In_z_ const WCHAR* strText ) const;
    void*   GetItemData( _In_ int nIndex ) const;
    void    SetDropHeight( _In_ UINT nHeight )
    {
        m_nDropHeight = nHeight;
        UpdateRects();
    }
    int     GetScrollBarWidth() const { return m_nSBWidth; }
    void    SetScrollBarWidth( _In_ int nWidth )
    {
        m_nSBWidth = nWidth;
        UpdateRects();
    }

    int GetSelectedIndex() const { return m_iSelected; }
    void* GetSelectedData() const;
    DXUTComboBoxItem* GetSelectedItem() const;

    UINT GetNumItems() { return static_cast<UINT>( m_Items.size() ); }
    DXUTComboBoxItem* GetItem( _In_ UINT index ) { return m_Items[ index ]; }

    HRESULT SetSelectedByIndex( _In_ UINT index );
    HRESULT SetSelectedByText( _In_z_ const WCHAR* strText );
    HRESULT SetSelectedByData( _In_ void* pData );

protected:
    int m_iSelected;
    int m_iFocused;
    int m_nDropHeight;
    CDXUTScrollBar m_ScrollBar;
    int m_nSBWidth;

    bool m_bOpened;

    RECT m_rcText;
    RECT m_rcButton;
    RECT m_rcDropdown;
    RECT m_rcDropdownText;

    std::vector<DXUTComboBoxItem*> m_Items;
};


//-----------------------------------------------------------------------------
// Slider control
//-----------------------------------------------------------------------------
class CDXUTSlider : public CDXUTControl
{
public:
    CDXUTSlider( _In_opt_ CDXUTDialog* pDialog = nullptr );

    virtual bool ContainsPoint( _In_ const POINT& pt ) override;
    virtual bool CanHaveFocus() override
    {
        return ( m_bVisible && m_bEnabled );
    }
    virtual bool HandleKeyboard( _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam ) override;
    virtual bool HandleMouse( _In_ UINT uMsg, _In_ const POINT& pt, _In_ WPARAM wParam, _In_ LPARAM lParam ) override;

    virtual void UpdateRects() override;

    virtual void Render( _In_ float fElapsedTime ) override;

    void SetValue( int nValue ) { SetValueInternal( nValue, false ); }
    int GetValue() const { return m_nValue; }

    void GetRange( _Out_ int& nMin, _Out_ int& nMax ) const
    {
        nMin = m_nMin;
        nMax = m_nMax;
    }
    void SetRange( _In_ int nMin, _In_ int nMax );

protected:
    void SetValueInternal( _In_ int nValue, _In_ bool bFromInput );
    int ValueFromPos( _In_ int x );

    int m_nValue;

    int m_nMin;
    int m_nMax;

    int m_nDragX;      // Mouse position at start of drag
    int m_nDragOffset; // Drag offset from the center of the button
    int m_nButtonX;

    bool m_bPressed;
    RECT m_rcButton;
};


//-----------------------------------------------------------------------------
// CUniBuffer class for the edit control
//-----------------------------------------------------------------------------
class CUniBuffer
{
public:
    CUniBuffer( _In_ int nInitialSize = 1 );
    ~CUniBuffer();

    size_t GetBufferSize() const { return m_nBufferSize; }
    bool SetBufferSize( _In_ int nSize );
    int GetTextSize() const { return (int)wcslen( m_pwszBuffer ); }
    const WCHAR* GetBuffer() const
    {
        return m_pwszBuffer;
    }
    const WCHAR& operator[]( _In_ int n ) const
    {
        return m_pwszBuffer[n];
    }
    WCHAR& operator[]( _In_ int n );
    DXUTFontNode* GetFontNode() const { return m_pFontNode; }
    void SetFontNode( _In_opt_ DXUTFontNode* pFontNode ) { m_pFontNode = pFontNode; }
    void Clear();

    bool InsertChar( _In_ int nIndex, _In_ WCHAR wChar );
        // Inserts the char at specified index. If nIndex == -1, insert to the end.

    bool RemoveChar( _In_ int nIndex );
        // Removes the char at specified index. If nIndex == -1, remove the last char.

    bool InsertString( _In_ int nIndex, _In_z_ const WCHAR* pStr, _In_ int nCount = -1 );
        // Inserts the first nCount characters of the string pStr at specified index.  If nCount == -1, the entire string is inserted. If nIndex == -1, insert to the end.

    bool SetText( _In_z_ LPCWSTR wszText );

    // Uniscribe
    bool CPtoX( _In_ int nCP, _In_ bool bTrail, _Out_ int* pX );
    bool XtoCP( _In_ int nX, _Out_ int* pCP, _Out_ int* pnTrail );
    void GetPriorItemPos( _In_ int nCP, _Out_ int* pPrior );
    void GetNextItemPos( _In_ int nCP, _Out_ int* pPrior );

private:
    HRESULT Analyse();      // Uniscribe -- Analyse() analyses the string in the buffer

    WCHAR* m_pwszBuffer;    // Buffer to hold text
    int m_nBufferSize;   // Size of the buffer allocated, in characters

    // Uniscribe-specific
    DXUTFontNode* m_pFontNode;          // Font node for the font that this buffer uses
    bool m_bAnalyseRequired;            // True if the string has changed since last analysis.
    SCRIPT_STRING_ANALYSIS m_Analysis;  // Analysis for the current string
};


//-----------------------------------------------------------------------------
// EditBox control
//-----------------------------------------------------------------------------
class CDXUTEditBox : public CDXUTControl
{
public:
    CDXUTEditBox( _In_opt_ CDXUTDialog* pDialog = nullptr );
    virtual ~CDXUTEditBox();

    virtual bool HandleKeyboard( _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam ) override;
    virtual bool HandleMouse( _In_ UINT uMsg, _In_ const POINT& pt, _In_ WPARAM wParam, _In_ LPARAM lParam ) override;
    virtual bool MsgProc( _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam ) override;
    virtual void UpdateRects() override;
    virtual bool CanHaveFocus() override
    {
        return ( m_bVisible && m_bEnabled );
    }
    virtual void Render( _In_ float fElapsedTime ) override;
    virtual void OnFocusIn() override;

    void    SetText( _In_z_ LPCWSTR wszText, _In_ bool bSelected = false );
    LPCWSTR GetText() const { return m_Buffer.GetBuffer(); }
    size_t  GetTextLength() const { return m_Buffer.GetTextSize(); }  // Returns text length in chars excluding nul.
    HRESULT GetTextCopy(  _Out_writes_(bufferCount) LPWSTR strDest, _In_ UINT bufferCount ) const;
    void    ClearText();

    virtual void SetTextColor( _In_ DWORD Color ) override { m_TextColor = Color; }  // Text color
    void SetSelectedTextColor( _In_ DWORD Color ) { m_SelTextColor = Color; }  // Selected text color
    void SetSelectedBackColor( _In_ DWORD Color ) { m_SelBkColor = Color; }  // Selected background color
    void SetCaretColor( _In_ DWORD Color ) { m_CaretColor = Color; }  // Caret color
    void SetBorderWidth( _In_ int nBorder )
    {
        m_nBorder = nBorder;
        UpdateRects();
    }  // Border of the window
    void SetSpacing( _In_ int nSpacing )
    {
        m_nSpacing = nSpacing;
        UpdateRects();
    }
    void ParseFloatArray( _In_reads_(nCount) float* pNumbers, _In_ int nCount );
    void SetTextFloatArray( _In_reads_(nCount) const float* pNumbers, _In_ int nCount );

protected:
    void PlaceCaret( _In_ int nCP );
    void DeleteSelectionText();
    void ResetCaretBlink();
    void CopyToClipboard();
    void PasteFromClipboard();

    CUniBuffer m_Buffer;     // Buffer to hold text
    int m_nBorder;      // Border of the window
    int m_nSpacing;     // Spacing between the text and the edge of border
    RECT m_rcText;       // Bounding rectangle for the text
    RECT            m_rcRender[9];  // Convenient rectangles for rendering elements
    double m_dfBlink;      // Caret blink time in milliseconds
    double m_dfLastBlink;  // Last timestamp of caret blink
    bool m_bCaretOn;     // Flag to indicate whether caret is currently visible
    int m_nCaret;       // Caret position, in characters
    bool m_bInsertMode;  // If true, control is in insert mode. Else, overwrite mode.
    int m_nSelStart;    // Starting position of the selection. The caret marks the end.
    int m_nFirstVisible;// First visible character in the edit control
    DWORD m_TextColor;    // Text color
    DWORD m_SelTextColor; // Selected text color
    DWORD m_SelBkColor;   // Selected background color
    DWORD m_CaretColor;   // Caret color

    // Mouse-specific
    bool m_bMouseDrag;       // True to indicate drag in progress

    // Static
    static bool s_bHideCaret;   // If true, we don't render the caret.
};


//-----------------------------------------------------------------------------
void BeginText11();
void DrawText11DXUT( _In_ ID3D11Device* pd3dDevice, _In_ ID3D11DeviceContext* pd3d11DeviceContext,
                     _In_z_ LPCWSTR strText, _In_ const RECT& rcScreen, _In_ DirectX::XMFLOAT4 vFontColor,
                     _In_ float fBBWidth, _In_ float fBBHeight, _In_ bool bCenter );
void EndText11( _In_ ID3D11Device* pd3dDevice, _In_ ID3D11DeviceContext* pd3d11DeviceContext );
