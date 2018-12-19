//--------------------------------------------------------------------------------------
// File: DXUTguiIME.h
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
#include "ImeUi.h"

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
class CDXUTIMEEditBox;


//-----------------------------------------------------------------------------
// IME-enabled EditBox control
//-----------------------------------------------------------------------------
#define MAX_COMPSTRING_SIZE 256


class CDXUTIMEEditBox : public CDXUTEditBox
{
public:

    static HRESULT CreateIMEEditBox( _In_ CDXUTDialog* pDialog, _In_ int ID, _In_z_ LPCWSTR strText, _In_ int x, _In_ int y, _In_ int width,
                                     _In_ int height, _In_ bool bIsDefault=false, _Outptr_opt_ CDXUTIMEEditBox** ppCreated=nullptr );

    CDXUTIMEEditBox( _In_opt_ CDXUTDialog* pDialog = nullptr );
    virtual ~CDXUTIMEEditBox();

    static void InitDefaultElements( _In_ CDXUTDialog* pDialog );

    static void WINAPI Initialize( _In_ HWND hWnd );
    static void WINAPI Uninitialize();

    static  HRESULT WINAPI  StaticOnCreateDevice();
    static  bool WINAPI     StaticMsgProc( _In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam );

    static  void WINAPI     SetImeEnableFlag( _In_ bool bFlag );

    virtual void Render( _In_ float fElapsedTime ) override;
    virtual bool MsgProc( _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam ) override;
    virtual bool HandleMouse( _In_ UINT uMsg, _In_ const POINT& pt, _In_ WPARAM wParam, _In_ LPARAM lParam ) override;
    virtual void UpdateRects() override;
    virtual void OnFocusIn() override;
    virtual void OnFocusOut() override;

    void PumpMessage();

    virtual void RenderCandidateReadingWindow( _In_ bool bReading );
    virtual void RenderComposition();
    virtual void RenderIndicator( _In_ float fElapsedTime );

protected:
    static void WINAPI      EnableImeSystem( _In_ bool bEnable );

    static WORD WINAPI      GetLanguage()
    {
        return ImeUi_GetLanguage();
    }
    static WORD WINAPI      GetPrimaryLanguage()
    {
        return ImeUi_GetPrimaryLanguage();
    }
    static void WINAPI      SendKey( _In_ BYTE nVirtKey );
    static DWORD WINAPI     GetImeId( _In_ UINT uIndex = 0 )
    {
        return ImeUi_GetImeId( uIndex );
    };
    static void WINAPI      CheckInputLocale();
    static void WINAPI      CheckToggleState();
    static void WINAPI      SetupImeApi();
    static void WINAPI      ResetCompositionString();


    static void             SetupImeUiCallback();

protected:
    enum
    {
        INDICATOR_NON_IME,
        INDICATOR_CHS,
        INDICATOR_CHT,
        INDICATOR_KOREAN,
        INDICATOR_JAPANESE
    };

    struct CCandList
    {
        CUniBuffer HoriCand; // Candidate list string (for horizontal candidate window)
        int nFirstSelected; // First character position of the selected string in HoriCand
        int nHoriSelectedLen; // Length of the selected string in HoriCand
        RECT rcCandidate;   // Candidate rectangle computed and filled each time before rendered
    };

    static POINT s_ptCompString;        // Composition string position. Updated every frame.
    static int s_nFirstTargetConv;    // Index of the first target converted char in comp string.  If none, -1.
    static CUniBuffer s_CompString;       // Buffer to hold the composition string (we fix its length)
    static DWORD            s_adwCompStringClause[MAX_COMPSTRING_SIZE];
    static CCandList s_CandList;          // Data relevant to the candidate list
    static WCHAR            s_wszReadingString[32];// Used only with horizontal reading window (why?)
    static bool s_bImeFlag;			  // Is ime enabled 
	
    // Color of various IME elements
    DWORD m_ReadingColor;        // Reading string color
    DWORD m_ReadingWinColor;     // Reading window color
    DWORD m_ReadingSelColor;     // Selected character in reading string
    DWORD m_ReadingSelBkColor;   // Background color for selected char in reading str
    DWORD m_CandidateColor;      // Candidate string color
    DWORD m_CandidateWinColor;   // Candidate window color
    DWORD m_CandidateSelColor;   // Selected candidate string color
    DWORD m_CandidateSelBkColor; // Selected candidate background color
    DWORD m_CompColor;           // Composition string color
    DWORD m_CompWinColor;        // Composition string window color
    DWORD m_CompCaretColor;      // Composition string caret color
    DWORD m_CompTargetColor;     // Composition string target converted color
    DWORD m_CompTargetBkColor;   // Composition string target converted background
    DWORD m_CompTargetNonColor;  // Composition string target non-converted color
    DWORD m_CompTargetNonBkColor;// Composition string target non-converted background
    DWORD m_IndicatorImeColor;   // Indicator text color for IME
    DWORD m_IndicatorEngColor;   // Indicator text color for English
    DWORD m_IndicatorBkColor;    // Indicator text background color

    // Edit-control-specific data
    int m_nIndicatorWidth;     // Width of the indicator symbol
    RECT m_rcIndicator;         // Rectangle for drawing the indicator button

#if defined(DEBUG) || defined(_DEBUG)
    static bool    m_bIMEStaticMsgProcCalled;
#endif
};
