//--------------------------------------------------------------------------------------
// File: SDKmisc.cpp
//
// Various helper functionality that is shared between SDK samples
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
#include "SDKmisc.h"
#include "DXUTres.h"

#include "DXUTGui.h"

#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include "ScreenGrab.h"

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Global/Static Members
//--------------------------------------------------------------------------------------
CDXUTResourceCache& WINAPI DXUTGetGlobalResourceCache()
{
    // Using an accessor function gives control of the construction order
    static CDXUTResourceCache* s_cache = nullptr;
    if ( !s_cache )
    {
#if defined(DEBUG) || defined(_DEBUG)
        int flag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
        _CrtSetDbgFlag( flag & ~_CRTDBG_ALLOC_MEM_DF );
#endif
        s_cache = new CDXUTResourceCache;
#if defined(DEBUG) || defined(_DEBUG)
        _CrtSetDbgFlag( flag );
#endif
    }
    return *s_cache;
}


//--------------------------------------------------------------------------------------
// Internal functions forward declarations
//--------------------------------------------------------------------------------------
bool DXUTFindMediaSearchTypicalDirs( _Out_writes_(cchSearch) WCHAR* strSearchPath, 
                                     _In_ int cchSearch, 
                                     _In_ LPCWSTR strLeaf, 
                                     _In_ const WCHAR* strExePath,
                                     _In_ const WCHAR* strExeName );
bool DXUTFindMediaSearchParentDirs( _Out_writes_(cchSearch) WCHAR* strSearchPath, 
                                    _In_ int cchSearch, 
                                    _In_ const WCHAR* strStartAt, 
                                    _In_ const WCHAR* strLeafName );

INT_PTR CALLBACK DisplaySwitchToREFWarningProc( _In_ HWND hDlg, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam );


//--------------------------------------------------------------------------------------
// Shared code for samples to ask user if they want to use a REF device or quit
//--------------------------------------------------------------------------------------
void WINAPI DXUTDisplaySwitchingToREFWarning()
{
    if( DXUTGetShowMsgBoxOnError() )
    {
        DWORD dwSkipWarning = 0, dwRead = 0, dwWritten = 0;
        HANDLE hFile = nullptr;

        // Read previous user settings
        WCHAR strPath[MAX_PATH];
        if ( SUCCEEDED(SHGetFolderPath(DXUTGetHWND(), CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, strPath)) )
        {
            wcscat_s( strPath, MAX_PATH, L"\\DXUT\\SkipRefWarning.dat" );
            if( ( hFile = CreateFile( strPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0,
                                      nullptr ) ) != INVALID_HANDLE_VALUE )
            {
                (void)ReadFile( hFile, &dwSkipWarning, sizeof( DWORD ), &dwRead, nullptr );
                CloseHandle( hFile );
            }
        }

        if( dwSkipWarning == 0 )
        {
            // Compact code to create a custom dialog box without using a template in a resource file.
            // If this dialog were in a .rc file, this would be a lot simpler but every sample calling this function would
            // need a copy of the dialog in its own .rc file. Also MessageBox API could be used here instead, but 
            // the MessageBox API is simpler to call but it can't provide a "Don't show again" checkbox
            typedef struct
            {
                DLGITEMTEMPLATE a;
                WORD b;
                WORD c;
                WORD d;
                WORD e;
                WORD f;
            } DXUT_DLG_ITEM;
            typedef struct
            {
                DLGTEMPLATE a;
                WORD b;
                WORD c;
                WCHAR   d[2];
                WORD e;
                WCHAR   f[16];
                DXUT_DLG_ITEM i1;
                DXUT_DLG_ITEM i2;
                DXUT_DLG_ITEM i3;
                DXUT_DLG_ITEM i4;
                DXUT_DLG_ITEM i5;
            } DXUT_DLG_DATA;

            DXUT_DLG_DATA dtp =
            {
                {WS_CAPTION | WS_POPUP | WS_VISIBLE | WS_SYSMENU | DS_ABSALIGN | DS_3DLOOK | DS_SETFONT |
                    DS_MODALFRAME | DS_CENTER,0,5,0,0,269,82},0,0,L" ",8,L"MS Shell Dlg 2",
                { {WS_CHILD | WS_VISIBLE | SS_ICON | SS_CENTERIMAGE,0,7,7,24,24,0x100},0xFFFF,0x0082,0,0,0}, // icon
                { {WS_CHILD | WS_VISIBLE,0,40,7,230,25,0x101},0xFFFF,0x0082,0,0,0}, // static text
                { {WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,0,80,39,50,14,IDYES},0xFFFF,0x0080,0,0,0}, // Yes button
                { {WS_CHILD | WS_VISIBLE | WS_TABSTOP,0,133,39,50,14,IDNO},0xFFFF,0x0080,0,0,0}, // No button
                { {WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_CHECKBOX,0,7,59,70,16,IDIGNORE},0xFFFF,0x0080,0,0,0}, // checkbox
            };

            LPARAM lParam;
            lParam = 11;
            int nResult = ( int )DialogBoxIndirectParam( DXUTGetHINSTANCE(), ( DLGTEMPLATE* )&dtp, DXUTGetHWND(),
                                                         DisplaySwitchToREFWarningProc, lParam );

            if( ( nResult & 0x80 ) == 0x80 ) // "Don't show again" checkbox was checked
            {
                // Save user settings
                dwSkipWarning = 1;
                if ( SUCCEEDED(SHGetFolderPath(DXUTGetHWND(), CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, strPath)) )
                {
                    wcscat_s( strPath, MAX_PATH, L"\\DXUT" );
                    CreateDirectory( strPath, nullptr );
                    wcscat_s( strPath, MAX_PATH, L"\\SkipRefWarning.dat" );
                    if( ( hFile = CreateFile( strPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0,
                                              nullptr ) ) != INVALID_HANDLE_VALUE )
                    {
                        WriteFile( hFile, &dwSkipWarning, sizeof( DWORD ), &dwWritten, nullptr );
                        CloseHandle( hFile );
                    }
                }
            }

            // User choose not to continue
            if( ( nResult & 0x0F ) == IDNO )
                DXUTShutdown( 1 );
        }
    }
}


//--------------------------------------------------------------------------------------
// MsgProc for DXUTDisplaySwitchingToREFWarning() dialog box
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
INT_PTR CALLBACK DisplaySwitchToREFWarningProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch( message )
    {
        case WM_INITDIALOG:
            // Easier to set text here than in the DLGITEMTEMPLATE
            SetWindowText( hDlg, DXUTGetWindowTitle() );
            SendMessage( GetDlgItem( hDlg, 0x100 ), STM_SETIMAGE, IMAGE_ICON, ( LPARAM )LoadIcon( 0, IDI_QUESTION ) );
            WCHAR sz[512];
            swprintf_s( sz, 512,
                             L"This program needs to use the Direct3D %Iu reference device.  This device implements the entire Direct3D %Iu feature set, but runs very slowly.  Do you wish to continue?", lParam, lParam );
            SetDlgItemText( hDlg, 0x101, sz );
            SetDlgItemText( hDlg, IDYES, L"&Yes" );
            SetDlgItemText( hDlg, IDNO, L"&No" );
            SetDlgItemText( hDlg, IDIGNORE, L"&Don't show again" );
            break;

        case WM_COMMAND:
            switch( LOWORD( wParam ) )
            {
                case IDIGNORE:
                    CheckDlgButton( hDlg, IDIGNORE, ( IsDlgButtonChecked( hDlg,
                                                                          IDIGNORE ) == BST_CHECKED ) ? BST_UNCHECKED :
                                    BST_CHECKED );
                    EnableWindow( GetDlgItem( hDlg, IDNO ), ( IsDlgButtonChecked( hDlg, IDIGNORE ) != BST_CHECKED ) );
                    break;
                case IDNO:
                    EndDialog( hDlg, ( IsDlgButtonChecked( hDlg, IDIGNORE ) == BST_CHECKED ) ? IDNO | 0x80 : IDNO |
                               0x00 ); return TRUE;
                case IDCANCEL:
                case IDYES:
                    EndDialog( hDlg, ( IsDlgButtonChecked( hDlg, IDIGNORE ) == BST_CHECKED ) ? IDYES | 0x80 : IDYES |
                               0x00 ); return TRUE;
            }
            break;
    }
    return FALSE;
}


//--------------------------------------------------------------------------------------
// Returns pointer to static media search buffer
//--------------------------------------------------------------------------------------
WCHAR* DXUTMediaSearchPath()
{
    static WCHAR s_strMediaSearchPath[MAX_PATH] =
    {
        0
    };
    return s_strMediaSearchPath;

}


//--------------------------------------------------------------------------------------
LPCWSTR WINAPI DXUTGetMediaSearchPath()
{
    return DXUTMediaSearchPath();
}


//--------------------------------------------------------------------------------------
HRESULT WINAPI DXUTSetMediaSearchPath( _In_z_ LPCWSTR strPath )
{
    HRESULT hr;

    WCHAR* s_strSearchPath = DXUTMediaSearchPath();

    hr = wcscpy_s( s_strSearchPath, MAX_PATH, strPath );
    if( SUCCEEDED( hr ) )
    {
        // append slash if needed
        size_t ch = 0;
        ch = wcsnlen( s_strSearchPath, MAX_PATH);
        if( SUCCEEDED( hr ) && s_strSearchPath[ch - 1] != L'\\' )
        {
            hr = wcscat_s( s_strSearchPath, MAX_PATH, L"\\" );
        }
    }

    return hr;
}


//--------------------------------------------------------------------------------------
// Tries to find the location of a SDK media file
//       cchDest is the size in WCHARs of strDestPath.  Be careful not to 
//       pass in sizeof(strDest) on UNICODE builds.
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT WINAPI DXUTFindDXSDKMediaFileCch( WCHAR* strDestPath, int cchDest, 
                                          LPCWSTR strFilename )
{
    bool bFound;
    WCHAR strSearchFor[MAX_PATH];

    if( !strFilename || strFilename[0] == 0 || !strDestPath || cchDest < 10 )
        return E_INVALIDARG;

    // Get the exe name, and exe path
    WCHAR strExePath[MAX_PATH] =
    {
        0
    };
    WCHAR strExeName[MAX_PATH] =
    {
        0
    };
    WCHAR* strLastSlash = nullptr;
    GetModuleFileName( nullptr, strExePath, MAX_PATH );
    strExePath[MAX_PATH - 1] = 0;
    strLastSlash = wcsrchr( strExePath, TEXT( '\\' ) );
    if( strLastSlash )
    {
        wcscpy_s( strExeName, MAX_PATH, &strLastSlash[1] );

        // Chop the exe name from the exe path
        *strLastSlash = 0;

        // Chop the .exe from the exe name
        strLastSlash = wcsrchr( strExeName, TEXT( '.' ) );
        if( strLastSlash )
            *strLastSlash = 0;
    }

    // Typical directories:
    //      .\
    //      ..\
    //      ..\..\
    //      %EXE_DIR%\
    //      %EXE_DIR%\..\
    //      %EXE_DIR%\..\..\
    //      %EXE_DIR%\..\%EXE_NAME%
    //      %EXE_DIR%\..\..\%EXE_NAME%

    // Typical directory search
    bFound = DXUTFindMediaSearchTypicalDirs( strDestPath, cchDest, strFilename, strExePath, strExeName );
    if( bFound )
        return S_OK;

    // Typical directory search again, but also look in a subdir called "\media\" 
    swprintf_s( strSearchFor, MAX_PATH, L"media\\%s", strFilename );
    bFound = DXUTFindMediaSearchTypicalDirs( strDestPath, cchDest, strSearchFor, strExePath, strExeName );
    if( bFound )
        return S_OK;

    WCHAR strLeafName[MAX_PATH] =
    {
        0
    };

    // Search all parent directories starting at .\ and using strFilename as the leaf name
    wcscpy_s( strLeafName, MAX_PATH, strFilename );
    bFound = DXUTFindMediaSearchParentDirs( strDestPath, cchDest, L".", strLeafName );
    if( bFound )
        return S_OK;

    // Search all parent directories starting at the exe's dir and using strFilename as the leaf name
    bFound = DXUTFindMediaSearchParentDirs( strDestPath, cchDest, strExePath, strLeafName );
    if( bFound )
        return S_OK;

    // Search all parent directories starting at .\ and using "media\strFilename" as the leaf name
    swprintf_s( strLeafName, MAX_PATH, L"media\\%s", strFilename );
    bFound = DXUTFindMediaSearchParentDirs( strDestPath, cchDest, L".", strLeafName );
    if( bFound )
        return S_OK;

    // Search all parent directories starting at the exe's dir and using "media\strFilename" as the leaf name
    bFound = DXUTFindMediaSearchParentDirs( strDestPath, cchDest, strExePath, strLeafName );
    if( bFound )
        return S_OK;

    // On failure, return the file as the path but also return an error code
    wcscpy_s( strDestPath, cchDest, strFilename );

    return DXUTERR_MEDIANOTFOUND;
}


//--------------------------------------------------------------------------------------
// Search a set of typical directories
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool DXUTFindMediaSearchTypicalDirs( WCHAR* strSearchPath, int cchSearch, LPCWSTR strLeaf, 
                                     const WCHAR* strExePath, const WCHAR* strExeName )
{
    // Typical directories:
    //      .\
    //      ..\
    //      ..\..\
    //      %EXE_DIR%\
    //      %EXE_DIR%\..\
    //      %EXE_DIR%\..\..\
    //      %EXE_DIR%\..\%EXE_NAME%
    //      %EXE_DIR%\..\..\%EXE_NAME%
    //      DXSDK media path

    // Search in .\  
    wcscpy_s( strSearchPath, cchSearch, strLeaf );
    if( GetFileAttributes( strSearchPath ) != 0xFFFFFFFF )
        return true;

    // Search in ..\  
    swprintf_s( strSearchPath, cchSearch, L"..\\%s", strLeaf );
    if( GetFileAttributes( strSearchPath ) != 0xFFFFFFFF )
        return true;

    // Search in ..\..\ 
    swprintf_s( strSearchPath, cchSearch, L"..\\..\\%s", strLeaf );
    if( GetFileAttributes( strSearchPath ) != 0xFFFFFFFF )
        return true;

    // Search in ..\..\ 
    swprintf_s( strSearchPath, cchSearch, L"..\\..\\%s", strLeaf );
    if( GetFileAttributes( strSearchPath ) != 0xFFFFFFFF )
        return true;

    // Search in the %EXE_DIR%\ 
    swprintf_s( strSearchPath, cchSearch, L"%s\\%s", strExePath, strLeaf );
    if( GetFileAttributes( strSearchPath ) != 0xFFFFFFFF )
        return true;

    // Search in the %EXE_DIR%\..\ 
    swprintf_s( strSearchPath, cchSearch, L"%s\\..\\%s", strExePath, strLeaf );
    if( GetFileAttributes( strSearchPath ) != 0xFFFFFFFF )
        return true;

    // Search in the %EXE_DIR%\..\..\ 
    swprintf_s( strSearchPath, cchSearch, L"%s\\..\\..\\%s", strExePath, strLeaf );
    if( GetFileAttributes( strSearchPath ) != 0xFFFFFFFF )
        return true;

    // Search in "%EXE_DIR%\..\%EXE_NAME%\".  This matches the DirectX SDK layout
    swprintf_s( strSearchPath, cchSearch, L"%s\\..\\%s\\%s", strExePath, strExeName, strLeaf );
    if( GetFileAttributes( strSearchPath ) != 0xFFFFFFFF )
        return true;

    // Search in "%EXE_DIR%\..\..\%EXE_NAME%\".  This matches the DirectX SDK layout
    swprintf_s( strSearchPath, cchSearch, L"%s\\..\\..\\%s\\%s", strExePath, strExeName, strLeaf );
    if( GetFileAttributes( strSearchPath ) != 0xFFFFFFFF )
        return true;

    // Search in media search dir 
    WCHAR* s_strSearchPath = DXUTMediaSearchPath();
    if( s_strSearchPath[0] != 0 )
    {
        swprintf_s( strSearchPath, cchSearch, L"%s%s", s_strSearchPath, strLeaf );
        if( GetFileAttributes( strSearchPath ) != 0xFFFFFFFF )
            return true;
    }

    return false;
}


//--------------------------------------------------------------------------------------
// Search parent directories starting at strStartAt, and appending strLeafName
// at each parent directory.  It stops at the root directory.
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
bool DXUTFindMediaSearchParentDirs( WCHAR* strSearchPath, int cchSearch, const WCHAR* strStartAt, 
                                    const WCHAR* strLeafName )
{
    WCHAR strFullPath[MAX_PATH] =
    {
        0
    };
    WCHAR strFullFileName[MAX_PATH] =
    {
        0
    };
    WCHAR strSearch[MAX_PATH] =
    {
        0
    };
    WCHAR* strFilePart = nullptr;

    if ( !GetFullPathName( strStartAt, MAX_PATH, strFullPath, &strFilePart ) )
        return false;

#pragma warning( disable : 6102 )
    while( strFilePart && *strFilePart != '\0' )
    {
        swprintf_s( strFullFileName, MAX_PATH, L"%s\\%s", strFullPath, strLeafName );
        if( GetFileAttributes( strFullFileName ) != 0xFFFFFFFF )
        {
            wcscpy_s( strSearchPath, cchSearch, strFullFileName );
            return true;
        }

        swprintf_s( strSearch, MAX_PATH, L"%s\\..", strFullPath );
        if ( !GetFullPathName( strSearch, MAX_PATH, strFullPath, &strFilePart ) )
            return false;
    }

    return false;
}


//--------------------------------------------------------------------------------------
// Compiles HLSL shaders
//--------------------------------------------------------------------------------------
#if D3D_COMPILER_VERSION < 46

namespace
{

struct handle_closer { void operator()(HANDLE h) { if (h) CloseHandle(h); } };

typedef public std::unique_ptr<void, handle_closer> ScopedHandle;

inline HANDLE safe_handle( HANDLE h ) { return (h == INVALID_HANDLE_VALUE) ? 0 : h; }

class CIncludeHandler : public ID3DInclude
    // Not as robust as D3D_COMPILE_STANDARD_FILE_INCLUDE, but it works in most cases
{
private:
    static const unsigned int MAX_INCLUDES = 9;

    struct sInclude
    {
        HANDLE         hFile;
        HANDLE         hFileMap;
        LARGE_INTEGER  FileSize;
        void           *pMapData;
    };

    struct sInclude     m_includeFiles[MAX_INCLUDES];
    size_t              m_nIncludes;
    bool                m_reset;
    WCHAR               m_workingPath[MAX_PATH];

public:
    CIncludeHandler() : m_nIncludes(0), m_reset(false)
    {
        if ( !GetCurrentDirectoryW( MAX_PATH, m_workingPath ) )
            *m_workingPath = 0;

        for ( size_t i = 0; i < MAX_INCLUDES; ++i )
        {
            m_includeFiles[i].hFile = INVALID_HANDLE_VALUE;
            m_includeFiles[i].hFileMap = INVALID_HANDLE_VALUE;
            m_includeFiles[i].pMapData = nullptr;
        }
    }
    ~CIncludeHandler()
    {
        for ( size_t i = 0; i < m_nIncludes; ++i )
        {
            UnmapViewOfFile( m_includeFiles[i].pMapData );

            if ( m_includeFiles[i].hFileMap != INVALID_HANDLE_VALUE)
                CloseHandle( m_includeFiles[i].hFileMap );

            if ( m_includeFiles[i].hFile != INVALID_HANDLE_VALUE)
                CloseHandle( m_includeFiles[i].hFile );
        }

        m_nIncludes = 0;

        if ( m_reset && *m_workingPath )
        {
            SetCurrentDirectoryW( m_workingPath );
        }
    }

    STDMETHOD(Open( D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes ) )
    {
        UNREFERENCED_PARAMETER(IncludeType);
        UNREFERENCED_PARAMETER(pParentData);

        size_t incIndex = m_nIncludes+1;

        // Make sure we have enough room for this include file
        if ( incIndex >= MAX_INCLUDES )
            return E_FAIL;

        // try to open the file
        m_includeFiles[incIndex].hFile  = CreateFileA( pFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr );
        if( INVALID_HANDLE_VALUE == m_includeFiles[incIndex].hFile )
        {
            return E_FAIL;
        }

        // Get the file size
        GetFileSizeEx( m_includeFiles[incIndex].hFile, &m_includeFiles[incIndex].FileSize );

        // Use Memory Mapped File I/O for the header data
        m_includeFiles[incIndex].hFileMap = CreateFileMappingA( m_includeFiles[incIndex].hFile, nullptr, PAGE_READONLY, m_includeFiles[incIndex].FileSize.HighPart, m_includeFiles[incIndex].FileSize.LowPart, pFileName);
        if( !m_includeFiles[incIndex].hFileMap )
        {
            if (m_includeFiles[incIndex].hFile != INVALID_HANDLE_VALUE)
                CloseHandle( m_includeFiles[incIndex].hFile );
            return E_FAIL;
        }

        // Create Map view
        *ppData = MapViewOfFile( m_includeFiles[incIndex].hFileMap, FILE_MAP_READ, 0, 0, 0 );
        *pBytes = m_includeFiles[incIndex].FileSize.LowPart;

        // Success - Increment the include file count
        m_nIncludes = incIndex;

        return S_OK;
    }

    STDMETHOD(Close( LPCVOID pData ))
    {
        UNREFERENCED_PARAMETER(pData);
        // Defer Closure until the container destructor 
        return S_OK;
    }

    void SetCWD( LPCWSTR pFileName )
    {
        WCHAR filePath[MAX_PATH];
        wcscpy_s( filePath, MAX_PATH, pFileName );

        WCHAR *strLastSlash = wcsrchr( filePath, L'\\' );
        if( strLastSlash )
        {
            // Chop the exe name from the exe path
            *strLastSlash = 0;
            m_reset = true;
            SetCurrentDirectoryW( filePath );
        }
    }
};

}; // namespace

#endif

_Use_decl_annotations_
HRESULT WINAPI DXUTCompileFromFile( LPCWSTR pFileName,
                                    const D3D_SHADER_MACRO* pDefines,
                                    LPCSTR pEntrypoint, LPCSTR pTarget,
                                    UINT Flags1, UINT Flags2,
                                    ID3DBlob** ppCode )
{
    HRESULT hr;
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, pFileName ) );

#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    Flags1 |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob = nullptr;

#if D3D_COMPILER_VERSION >= 46

    hr = D3DCompileFromFile( str, pDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                             pEntrypoint, pTarget, Flags1, Flags2,
                             ppCode, &pErrorBlob );

#else

    ScopedHandle hFile( safe_handle( CreateFileW( str, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr ) ) );

    if ( !hFile )
        return HRESULT_FROM_WIN32( GetLastError() );

    LARGE_INTEGER FileSize = { 0 };

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
    FILE_STANDARD_INFO fileInfo;
    if ( !GetFileInformationByHandleEx( hFile.get(), FileStandardInfo, &fileInfo, sizeof(fileInfo) ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    FileSize = fileInfo.EndOfFile;
#else
    GetFileSizeEx( hFile.get(), &FileSize );
#endif

    if (!FileSize.LowPart || FileSize.HighPart > 0)
        return E_FAIL;

    std::unique_ptr<char[]> fxData;
    fxData.reset( new (std::nothrow) char[ FileSize.LowPart ] );
    if ( !fxData )
        return E_OUTOFMEMORY;

    DWORD BytesRead = 0;
    if ( !ReadFile( hFile.get(), fxData.get(), FileSize.LowPart, &BytesRead, nullptr ) )
        return HRESULT_FROM_WIN32( GetLastError() );

    if (BytesRead < FileSize.LowPart)
        return E_FAIL;

    char pSrcName[MAX_PATH];
    int result = WideCharToMultiByte( CP_ACP, WC_NO_BEST_FIT_CHARS, str, -1, pSrcName, MAX_PATH, nullptr, FALSE );
    if ( !result )
        return E_FAIL;
    
    const CHAR* pstrName = strrchr( pSrcName, '\\' );
    if (!pstrName)
    {
        pstrName = pSrcName;
    }
    else
    {
        pstrName++;
    }

    std::unique_ptr<CIncludeHandler> includes( new (std::nothrow) CIncludeHandler );
    if ( !includes )
        return E_OUTOFMEMORY;

    includes->SetCWD( str );

    hr = D3DCompile( fxData.get(), BytesRead, pstrName, pDefines, includes.get(),
                     pEntrypoint, pTarget, Flags1, Flags2,
                     ppCode, &pErrorBlob );

#endif

#pragma warning( suppress : 6102 )
    if ( pErrorBlob )
    {
        OutputDebugStringA( reinterpret_cast<const char*>( pErrorBlob->GetBufferPointer() ) );
        pErrorBlob->Release();
    }

    return hr;
}


//--------------------------------------------------------------------------------------
// Texture utilities
//--------------------------------------------------------------------------------------

_Use_decl_annotations_
HRESULT WINAPI DXUTCreateShaderResourceViewFromFile( ID3D11Device* d3dDevice, const wchar_t* szFileName, ID3D11ShaderResourceView** textureView )
{
    if ( !d3dDevice || !szFileName || !textureView )
        return E_INVALIDARG;

    WCHAR str[MAX_PATH];
    HRESULT hr = DXUTFindDXSDKMediaFileCch( str, MAX_PATH, szFileName );
    if ( FAILED(hr) )
        return hr;

    WCHAR ext[_MAX_EXT];
    _wsplitpath_s( str, nullptr, 0, nullptr, 0, nullptr, 0, ext, _MAX_EXT );

    if ( _wcsicmp( ext, L".dds" ) == 0 )
    {
        hr = DirectX::CreateDDSTextureFromFile( d3dDevice, str, nullptr, textureView );
    }
    else
    {
        hr = DirectX::CreateWICTextureFromFile( d3dDevice, nullptr, str, nullptr, textureView );
    }

    return hr;
}

_Use_decl_annotations_
HRESULT WINAPI DXUTCreateTextureFromFile( ID3D11Device* d3dDevice, const wchar_t* szFileName, ID3D11Resource** texture )
{
    if ( !d3dDevice || !szFileName || !texture )
        return E_INVALIDARG;

    WCHAR str[MAX_PATH];
    HRESULT hr = DXUTFindDXSDKMediaFileCch( str, MAX_PATH, szFileName );
    if ( FAILED(hr) )
        return hr;

    WCHAR ext[_MAX_EXT];
    _wsplitpath_s( str, nullptr, 0, nullptr, 0, nullptr, 0, ext, _MAX_EXT );

    if ( _wcsicmp( ext, L".dds" ) == 0 )
    {
        hr = DirectX::CreateDDSTextureFromFile( d3dDevice, str, texture, nullptr );
    }
    else
    {
        hr = DirectX::CreateWICTextureFromFile( d3dDevice, nullptr, str, texture, nullptr );
    }

    return hr;
}

_Use_decl_annotations_
HRESULT WINAPI DXUTSaveTextureToFile( ID3D11DeviceContext* pContext, ID3D11Resource* pSource, bool usedds, const wchar_t* szFileName )
{
    if ( !pContext || !pSource || !szFileName )
        return E_INVALIDARG;

    HRESULT hr;

    if ( usedds )
    {
        hr = DirectX::SaveDDSTextureToFile( pContext, pSource, szFileName );
    }
    else
    {
        hr = DirectX::SaveWICTextureToFile( pContext, pSource, GUID_ContainerFormatBmp, szFileName );
    }

    return hr;
}


//--------------------------------------------------------------------------------------
// Desc: Returns a view matrix for rendering to a face of a cubemap.
//--------------------------------------------------------------------------------------
XMMATRIX WINAPI DXUTGetCubeMapViewMatrix( _In_ DWORD dwFace )
{
    static const XMVECTORF32 s_vLookDir[] =
    {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { -1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, -1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f, 0.0f },
    };

    static const XMVECTORF32 s_vUpDir[] = 
    {
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
    };

    static_assert( _countof(s_vLookDir) == _countof(s_vUpDir), "arrays mismatch" );

    if ( dwFace >= _countof(s_vLookDir)
         || dwFace >= _countof(s_vUpDir) )
        return XMMatrixIdentity();

    // Set the view transform for this cubemap surface
    return XMMatrixLookAtLH( g_XMZero, s_vLookDir[ dwFace ], s_vUpDir[ dwFace ] );
}


//======================================================================================
// CDXUTResourceCache
//======================================================================================

CDXUTResourceCache::~CDXUTResourceCache()
{
    OnDestroyDevice();
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTResourceCache::CreateTextureFromFile( ID3D11Device* pDevice, ID3D11DeviceContext *pContext, LPCWSTR pSrcFile,
                                                   ID3D11ShaderResourceView** ppOutputRV, bool bSRGB )
{
    if ( !ppOutputRV )
        return E_INVALIDARG;

    *ppOutputRV = nullptr;

    for( auto it = m_TextureCache.cbegin(); it != m_TextureCache.cend(); ++it )
    {
        if( !wcscmp( it->wszSource, pSrcFile )
            && it->bSRGB == bSRGB
            && it->pSRV11 )
        {
            it->pSRV11->AddRef();
            *ppOutputRV = it->pSRV11;
            return S_OK;
        }
    }

    WCHAR ext[_MAX_EXT];
    _wsplitpath_s( pSrcFile, nullptr, 0, nullptr, 0, nullptr, 0, ext, _MAX_EXT );

    HRESULT hr;
    if ( _wcsicmp( ext, L".dds" ) == 0 )
    {
        hr = DirectX::CreateDDSTextureFromFileEx( pDevice, pSrcFile, 0,
                                                  D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, bSRGB,
                                                  nullptr, ppOutputRV, nullptr );
    }
    else
    {
        hr = DirectX::CreateWICTextureFromFileEx( pDevice, pContext, pSrcFile, 0,
                                                  D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, bSRGB,
                                                  nullptr, ppOutputRV );
    }

    if ( FAILED(hr) )
        return hr;

    DXUTCache_Texture entry;
    wcscpy_s( entry.wszSource, MAX_PATH, pSrcFile );
    entry.bSRGB = bSRGB;
    entry.pSRV11 = *ppOutputRV;
    entry.pSRV11->AddRef();
    m_TextureCache.push_back( entry );

    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTResourceCache::CreateTextureFromFile( ID3D11Device* pDevice, ID3D11DeviceContext *pContext, LPCSTR pSrcFile,
                                                   ID3D11ShaderResourceView** ppOutputRV, bool bSRGB )
{
    WCHAR szSrcFile[MAX_PATH];
    MultiByteToWideChar( CP_ACP, 0, pSrcFile, -1, szSrcFile, MAX_PATH );
    szSrcFile[MAX_PATH - 1] = 0;

    return CreateTextureFromFile( pDevice, pContext, szSrcFile, ppOutputRV, bSRGB );
}


//--------------------------------------------------------------------------------------
// Device event callbacks
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
HRESULT CDXUTResourceCache::OnDestroyDevice()
{
    // Release all resources
    for( size_t j = 0; j < m_TextureCache.size(); ++j )
    {
        SAFE_RELEASE( m_TextureCache[ j ].pSRV11 );
    }
    m_TextureCache.clear();
    m_TextureCache.shrink_to_fit();

    return S_OK;
}


//======================================================================================
// CDXUTTextHelper
//======================================================================================

_Use_decl_annotations_
CDXUTTextHelper::CDXUTTextHelper( ID3D11Device* pd3d11Device, ID3D11DeviceContext* pd3d11DeviceContext, CDXUTDialogResourceManager* pManager, int nLineHeight )
{
    Init( nLineHeight );
    m_pd3d11Device = pd3d11Device;
    m_pd3d11DeviceContext = pd3d11DeviceContext;
    m_pManager = pManager;
}

CDXUTTextHelper::~CDXUTTextHelper()
{

}


//--------------------------------------------------------------------------------------
void CDXUTTextHelper::Init( _In_ int nLineHeight )
{
    m_clr = XMFLOAT4( 1, 1, 1, 1 );
    m_pt.x = 0;
    m_pt.y = 0;
    m_nLineHeight = nLineHeight;
    m_pd3d11Device = nullptr;
    m_pd3d11DeviceContext = nullptr;
    m_pManager = nullptr; 

    // Create a blend state if a sprite is passed in
}


//--------------------------------------------------------------------------------------
HRESULT CDXUTTextHelper::DrawFormattedTextLine( _In_z_ const WCHAR* strMsg, ... )
{
    WCHAR strBuffer[512];

    va_list args;
    va_start( args, strMsg );
    vswprintf_s( strBuffer, 512, strMsg, args );
    strBuffer[511] = L'\0';
    va_end( args );

    return DrawTextLine( strBuffer );
}


//--------------------------------------------------------------------------------------
HRESULT CDXUTTextHelper::DrawTextLine( _In_z_ const WCHAR* strMsg )
{
    if( !m_pd3d11DeviceContext )
        return DXUT_ERR_MSGBOX( L"DrawTextLine", E_INVALIDARG );

    HRESULT hr = S_OK;
    RECT rc;
    SetRect( &rc, m_pt.x, m_pt.y, 0, 0 );
    DrawText11DXUT( m_pd3d11Device, m_pd3d11DeviceContext, strMsg, rc, m_clr,
                    (float)m_pManager->m_nBackBufferWidth, (float)m_pManager->m_nBackBufferHeight, false );

    if( FAILED( hr ) )
        return DXTRACE_ERR_MSGBOX( L"DrawText", hr );

    m_pt.y += m_nLineHeight;

    return S_OK;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTTextHelper::DrawFormattedTextLine( const RECT& rc, const WCHAR* strMsg, ... )
{
    WCHAR strBuffer[512];

    va_list args;
    va_start( args, strMsg );
    vswprintf_s( strBuffer, 512, strMsg, args );
    strBuffer[511] = L'\0';
    va_end( args );

    return DrawTextLine( rc, strBuffer );
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTTextHelper::DrawTextLine( const RECT& rc, const WCHAR* strMsg )
{
    if( !m_pd3d11DeviceContext )
        return DXUT_ERR_MSGBOX( L"DrawTextLine", E_INVALIDARG );

    HRESULT hr = S_OK;
    DrawText11DXUT( m_pd3d11Device, m_pd3d11DeviceContext, strMsg, rc, m_clr,
                    (float)m_pManager->m_nBackBufferWidth, (float)m_pManager->m_nBackBufferHeight, false );

    if( FAILED( hr ) )
        return DXTRACE_ERR_MSGBOX( L"DrawText", hr );

    m_pt.y += m_nLineHeight;

    return S_OK;
}


//--------------------------------------------------------------------------------------
void CDXUTTextHelper::Begin()
{
    if( m_pd3d11DeviceContext )
    {
        m_pManager->StoreD3D11State( m_pd3d11DeviceContext );
        m_pManager->ApplyRenderUI11( m_pd3d11DeviceContext );
    }


}


//--------------------------------------------------------------------------------------
void CDXUTTextHelper::End()
{
    if( m_pd3d11DeviceContext )
    {
        m_pManager->RestoreD3D11State( m_pd3d11DeviceContext );
    }
}
