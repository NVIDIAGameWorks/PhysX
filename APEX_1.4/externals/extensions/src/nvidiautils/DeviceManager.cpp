// TAGRELEASE: PUBLIC

#include "DeviceManager.h"
#include <WinUser.h>
#include <Windows.h>
#include <assert.h>
#include <sstream>
#include <algorithm>

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

#define WINDOW_CLASS_NAME   L"NvDX11"

#define WINDOW_STYLE_NORMAL         (WS_OVERLAPPEDWINDOW | WS_VISIBLE)
#define WINDOW_STYLE_FULLSCREEN     (WS_POPUP | WS_SYSMENU | WS_VISIBLE)

// A singleton, sort of... To pass the events from WindowProc to the object.
DeviceManager* g_DeviceManagerInstance = NULL;

#undef min
#undef max

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if(g_DeviceManagerInstance)
        return g_DeviceManagerInstance->MsgProc(hWnd, uMsg, wParam, lParam);
    else
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

DeviceManager* GetDeviceManager()
{
    return g_DeviceManagerInstance;
}

namespace
{
	bool IsNvDeviceID(UINT id)
	{
		return id == 0x10DE;
	}

	// Find an adapter whose name contains the given string.
	IDXGIAdapter* FindAdapter(const WCHAR* targetName, bool& isNv)
	{
		IDXGIAdapter* targetAdapter = NULL;
		IDXGIFactory* IDXGIFactory_0001 = NULL;
		HRESULT hres = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&IDXGIFactory_0001);
		if (hres != S_OK) 
		{
			printf("ERROR in CreateDXGIFactory, %s@%d.\nFor more info, get log from debug D3D runtime: (1) Install DX SDK, and enable Debug D3D from DX Control Panel Utility. (2) Install and start DbgView. (3) Try running the program again.\n",__FILE__,__LINE__); 
			return targetAdapter;
		}

		unsigned int adapterNo = 0;
		while (SUCCEEDED(hres))
		{
			IDXGIAdapter* pAdapter = NULL;
			hres = IDXGIFactory_0001->EnumAdapters(adapterNo, (IDXGIAdapter**)&pAdapter);

			if (SUCCEEDED(hres))
			{
				DXGI_ADAPTER_DESC aDesc;
				pAdapter->GetDesc(&aDesc);

				// If no name is specified, return the first adapater.  This is the same behaviour as the 
				// default specified for D3D11CreateDevice when no adapter is specified.
				if (wcslen(targetName) == 0)
				{
					targetAdapter = pAdapter;
					isNv = IsNvDeviceID(aDesc.VendorId);
					break;
				}

				std::wstring aName = aDesc.Description;
				if (aName.find(targetName) != std::string::npos)
				{
					targetAdapter = pAdapter;
					isNv = IsNvDeviceID(aDesc.VendorId);
				}
				else
				{
					pAdapter->Release();
				}
			}

			adapterNo++;
		}

		if (IDXGIFactory_0001)
			IDXGIFactory_0001->Release();

		return targetAdapter;
	}

	// Adjust window rect so that it is centred on the given adapter.  Clamps to fit if it's too big.
	RECT MoveWindowOntoAdapter(IDXGIAdapter* targetAdapter, const RECT& rect)
	{
		assert(targetAdapter != NULL);

		RECT result = rect;
		HRESULT hres = S_OK;
		unsigned int outputNo = 0;
		while (SUCCEEDED(hres))
		{
			IDXGIOutput* pOutput = NULL;
			hres = targetAdapter->EnumOutputs(outputNo++, &pOutput);

			if (SUCCEEDED(hres) && pOutput)
			{
				DXGI_OUTPUT_DESC OutputDesc;
				pOutput->GetDesc( &OutputDesc );
				const RECT desktop = OutputDesc.DesktopCoordinates;
				const int centreX = (int) desktop.left + (int)(desktop.right - desktop.left) / 2;
				const int centreY = (int) desktop.top + (int)(desktop.bottom - desktop.top) / 2;
				const int winW = rect.right - rect.left;
				const int winH = rect.bottom - rect.top;
				int left = centreX - winW/2;
				int right = left + winW;
				int top = centreY - winH/2;
				int bottom = top + winH;
				result.left = std::max(left, (int) desktop.left);
				result.right = std::min(right, (int) desktop.right);
				result.bottom = std::min(bottom, (int) desktop.bottom);
				result.top = std::max(top, (int) desktop.top);
                pOutput->Release();

				// If there is more than one output, go with the first found.  Multi-monitor support could go here.
				break;
			}
		}
		return result;
	}
}

HRESULT
DeviceManager::CreateWindowDeviceAndSwapChain(const DeviceCreationParameters& params, LPWSTR title)
{
    g_DeviceManagerInstance = this;
    m_WindowTitle = title;

    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASSEX windowClass = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WindowProc,
                        0L, 0L, hInstance, NULL, NULL, NULL, NULL, WINDOW_CLASS_NAME, NULL };

    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClassEx(&windowClass);

    UINT windowStyle = params.startFullscreen 
        ? WINDOW_STYLE_FULLSCREEN
        : params.startMaximized 
            ? (WINDOW_STYLE_NORMAL | WS_MAXIMIZE) 
            : WINDOW_STYLE_NORMAL;
        
    RECT rect = { 0, 0, params.backBufferWidth, params.backBufferHeight };
    AdjustWindowRect(&rect, windowStyle, FALSE);

	IDXGIAdapter* targetAdapter = FindAdapter(params.adapterNameSubstring, m_IsNvidia);
	if (targetAdapter)
	{
		rect = MoveWindowOntoAdapter(targetAdapter, rect);
	}
	else
	{
		// We could silently use a default adapter in this case.  I think it's better to choke.
		std::wostringstream ostr;
		ostr << L"Could not find an adapter matching \"" << params.adapterNameSubstring << "\"" << std::ends;
		MessageBox(NULL, ostr.str().c_str(), m_WindowTitle.c_str(), MB_OK | MB_ICONERROR);
        return E_FAIL;
    }

    m_hWnd = CreateWindowEx(
        0,
        WINDOW_CLASS_NAME, 
        title, 
        windowStyle, 
        rect.left, 
        rect.top, 
        rect.right - rect.left, 
        rect.bottom - rect.top, 
        GetDesktopWindow(),
        NULL,
        hInstance,
        NULL
    );

    if(!m_hWnd)
    {
#ifdef DEBUG
        DWORD errorCode = GetLastError();    
        printf("CreateWindowEx error code = 0x%x\n", errorCode);
#endif

        MessageBox(NULL, L"Cannot create window", m_WindowTitle.c_str(), MB_OK | MB_ICONERROR);
        return E_FAIL;
    }

    UpdateWindow(m_hWnd);

    HRESULT hr = E_FAIL;

    RECT clientRect;
    GetClientRect(m_hWnd, &clientRect);
    UINT width = clientRect.right - clientRect.left;
    UINT height = clientRect.bottom - clientRect.top;

    ZeroMemory(&m_SwapChainDesc, sizeof(m_SwapChainDesc));
    m_SwapChainDesc.BufferCount = params.swapChainBufferCount;
    m_SwapChainDesc.BufferDesc.Width = width;
    m_SwapChainDesc.BufferDesc.Height = height;
    m_SwapChainDesc.BufferDesc.Format = params.swapChainFormat;
    m_SwapChainDesc.BufferDesc.RefreshRate.Numerator = params.refreshRate;
    m_SwapChainDesc.BufferDesc.RefreshRate.Denominator = 0;
    m_SwapChainDesc.BufferUsage = params.swapChainUsage;
    m_SwapChainDesc.OutputWindow = m_hWnd;
    m_SwapChainDesc.SampleDesc.Count = params.swapChainSampleCount;
    m_SwapChainDesc.SampleDesc.Quality = params.swapChainSampleQuality;
    m_SwapChainDesc.Windowed = !params.startFullscreen;
    m_SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// The D3D documentation says that if adapter is non-null, driver type must be unknown.  Why not put
	// this logic in the CreateDevice fns then?!?
	const D3D_DRIVER_TYPE dType = (targetAdapter)? D3D_DRIVER_TYPE_UNKNOWN: params.driverType;

    hr = D3D11CreateDeviceAndSwapChain(
        targetAdapter,          // pAdapter
        dType,					// DriverType
        NULL,                   // Software
        params.createDeviceFlags, // Flags
        &params.featureLevel,   // pFeatureLevels
        1,                      // FeatureLevels
        D3D11_SDK_VERSION,      // SDKVersion
        &m_SwapChainDesc,       // pSwapChainDesc
        &m_SwapChain,           // ppSwapChain
        &m_Device,              // ppDevice
        NULL,                   // pFeatureLevel
        &m_ImmediateContext     // ppImmediateContext
    );

	if (targetAdapter)
		targetAdapter->Release();
    
    if(FAILED(hr))
        return hr;

    m_DepthStencilDesc.ArraySize = 1;
    m_DepthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    m_DepthStencilDesc.CPUAccessFlags = 0;
    m_DepthStencilDesc.Format = params.depthStencilFormat;
    m_DepthStencilDesc.Width = width;
    m_DepthStencilDesc.Height = height;
    m_DepthStencilDesc.MipLevels = 1;
    m_DepthStencilDesc.MiscFlags = 0;
    m_DepthStencilDesc.SampleDesc.Count = params.swapChainSampleCount;
    m_DepthStencilDesc.SampleDesc.Quality = 0;
    m_DepthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    
    hr = CreateRenderTargetAndDepthStencil();

    if(FAILED(hr))
        return hr;

    DeviceCreated();
    BackBufferResized();

    return S_OK;
}

void
DeviceManager::Shutdown() 
{   
    if(m_SwapChain && GetWindowState() == kWindowFullscreen)
        m_SwapChain->SetFullscreenState(false, NULL);

    DeviceDestroyed();

    SAFE_RELEASE(m_BackBufferRTV);
    SAFE_RELEASE(m_DepthStencilDSV);
    SAFE_RELEASE(m_DepthStencilBuffer);

    g_DeviceManagerInstance = NULL;
    SAFE_RELEASE(m_ImmediateContext);
    SAFE_RELEASE(m_SwapChain);

    ID3D11Debug * d3dDebug = nullptr;
    if (nullptr != m_Device)
    {
        ID3D11DeviceContext* pCtx;
        m_Device->GetImmediateContext(&pCtx);
        pCtx->ClearState();
        pCtx->Flush();
        pCtx->Release();
        if (SUCCEEDED(m_Device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&d3dDebug))))
        {
            d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
            d3dDebug->Release();
        }
    }
    SAFE_RELEASE(m_Device);

    if(m_hWnd)
    {
        DestroyWindow(m_hWnd);
        m_hWnd = NULL;
    }
}

HRESULT
DeviceManager::CreateRenderTargetAndDepthStencil()
{
    HRESULT hr;

    ID3D11Texture2D *backBuffer = NULL; 
    hr = m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
    if (FAILED(hr))
        return hr;

    hr = m_Device->CreateRenderTargetView(backBuffer, NULL, &m_BackBufferRTV);
    backBuffer->Release();
    if (FAILED(hr))
        return hr;

    if(m_DepthStencilDesc.Format != DXGI_FORMAT_UNKNOWN)
    {
        hr = m_Device->CreateTexture2D(&m_DepthStencilDesc, NULL, &m_DepthStencilBuffer);
        if (FAILED(hr))
            return hr;

        hr = m_Device->CreateDepthStencilView(m_DepthStencilBuffer, NULL, &m_DepthStencilDSV);
        if (FAILED(hr))
            return hr;
    }

    return S_OK;
}

void
DeviceManager::MessageLoop() 
{
    MSG msg = {0};

    LARGE_INTEGER perfFreq, previousTime;
    QueryPerformanceFrequency(&perfFreq);
    QueryPerformanceCounter(&previousTime);
    
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            LARGE_INTEGER newTime;
            QueryPerformanceCounter(&newTime);
            
            double elapsedSeconds = (m_FixedFrameInterval >= 0) 
                ? m_FixedFrameInterval  
                : (double)(newTime.QuadPart - previousTime.QuadPart) / (double)perfFreq.QuadPart;

            if(m_SwapChain && GetWindowState() != kWindowMinimized)
            {
                Animate(elapsedSeconds);
                Render(); 
                m_SwapChain->Present(m_SyncInterval, 0);
                Sleep(0);
            }
            else
            {
                // Release CPU resources when idle
                Sleep(1);
            }

            {
                m_vFrameTimes.push_back(elapsedSeconds);
                double timeSum = 0;
                for(auto it = m_vFrameTimes.begin(); it != m_vFrameTimes.end(); it++)
                    timeSum += *it;

                if(timeSum > m_AverageTimeUpdateInterval)
                {
                    m_AverageFrameTime = timeSum / (double)m_vFrameTimes.size();
                    m_vFrameTimes.clear();
                }
            }

            previousTime = newTime;
        }
    }
}

LRESULT 
DeviceManager::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_DESTROY:
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;

        case WM_SYSKEYDOWN:
            if(wParam == VK_F4)
            {
                PostQuitMessage(0);
                return 0;
            }
            break;

        case WM_ENTERSIZEMOVE:
            m_InSizingModalLoop = true;
            m_NewWindowSize.cx = m_SwapChainDesc.BufferDesc.Width;
            m_NewWindowSize.cy = m_SwapChainDesc.BufferDesc.Height;
            break;

        case WM_EXITSIZEMOVE:
            m_InSizingModalLoop = false;
            ResizeSwapChain();
            break;

        case WM_SIZE:
            // Ignore the WM_SIZE event if there is no device,
            // or if the window has been minimized (size == 0),
            // or if it has been restored to the previous size (this part is tested inside ResizeSwapChain)
            if (m_Device && (lParam != 0))
            {
                m_NewWindowSize.cx = LOWORD(lParam);
                m_NewWindowSize.cy = HIWORD(lParam);
                    
                if(!m_InSizingModalLoop)
                    ResizeSwapChain();
            }
    }

    if( uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST || 
        uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST )
    {
        // processing messages front-to-back
        for(auto it = m_vControllers.begin(); it != m_vControllers.end(); it++)
        {
            if((*it)->IsEnabled())
            {
                // for kb/mouse messages, 0 means the message has been handled
                if(0 == (*it)->MsgProc(hWnd, uMsg, wParam, lParam))
                    return 0;
            }
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void
DeviceManager::ResizeSwapChain()
{
    if(m_NewWindowSize.cx == (LONG)m_SwapChainDesc.BufferDesc.Width && 
       m_NewWindowSize.cy == (LONG)m_SwapChainDesc.BufferDesc.Height)
        return;

    m_SwapChainDesc.BufferDesc.Width = m_NewWindowSize.cx;
    m_SwapChainDesc.BufferDesc.Height = m_NewWindowSize.cy;

    ID3D11RenderTargetView *nullRTV = NULL;
    m_ImmediateContext->OMSetRenderTargets(1, &nullRTV, NULL);
    SAFE_RELEASE(m_BackBufferRTV);
    SAFE_RELEASE(m_DepthStencilDSV);
    SAFE_RELEASE(m_DepthStencilBuffer);

    if (m_SwapChain)
    {
        // Resize the swap chain
        m_SwapChain->ResizeBuffers(m_SwapChainDesc.BufferCount, m_SwapChainDesc.BufferDesc.Width, 
                                    m_SwapChainDesc.BufferDesc.Height, m_SwapChainDesc.BufferDesc.Format, 
                                    m_SwapChainDesc.Flags);
                    
        m_DepthStencilDesc.Width = m_NewWindowSize.cx;
        m_DepthStencilDesc.Height = m_NewWindowSize.cy;

        CreateRenderTargetAndDepthStencil();

        BackBufferResized();
    }
}

void
DeviceManager::Render()
{
    D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (float)m_SwapChainDesc.BufferDesc.Width, (float)m_SwapChainDesc.BufferDesc.Height, 0.0f, 1.0f };

    // rendering back-to-front
    for(auto it = m_vControllers.rbegin(); it != m_vControllers.rend(); it++)
    {
        if((*it)->IsEnabled())
        {
            m_ImmediateContext->OMSetRenderTargets(1, &m_BackBufferRTV, m_DepthStencilDSV);
            m_ImmediateContext->RSSetViewports(1, &viewport);

            (*it)->Render(m_Device, m_ImmediateContext, m_BackBufferRTV, m_DepthStencilDSV);
        }
    }

    m_ImmediateContext->OMSetRenderTargets(0, NULL, NULL);
}

void
DeviceManager::Animate(double fElapsedTimeSeconds)
{
    // front-to-back, but the order shouldn't matter
    for(auto it = m_vControllers.begin(); it != m_vControllers.end(); it++)
    {
        if((*it)->IsEnabled())
        {
            (*it)->Animate(fElapsedTimeSeconds);
        }
    }
}

void
DeviceManager::DeviceCreated()
{
    // creating resources front-to-back
    for(auto it = m_vControllers.begin(); it != m_vControllers.end(); it++)
    {
        (*it)->DeviceCreated(m_Device);
    }
}

void
DeviceManager::DeviceDestroyed()
{
    // releasing resources back-to-front
    for(auto it = m_vControllers.rbegin(); it != m_vControllers.rend(); it++)
    {
        (*it)->DeviceDestroyed();
    }
}

void
DeviceManager::BackBufferResized()
{
    if(m_SwapChain == NULL)
        return;

    DXGI_SURFACE_DESC backSD;
    backSD.Format = m_SwapChainDesc.BufferDesc.Format;
    backSD.Width = m_SwapChainDesc.BufferDesc.Width;
    backSD.Height = m_SwapChainDesc.BufferDesc.Height;
    backSD.SampleDesc = m_SwapChainDesc.SampleDesc;

    for(auto it = m_vControllers.begin(); it != m_vControllers.end(); it++)
    {
        (*it)->BackBufferResized(m_Device, &backSD);
    }
}

HRESULT
DeviceManager::ChangeBackBufferFormat(DXGI_FORMAT format, UINT sampleCount)
{
    HRESULT hr = E_FAIL;

    if((format == DXGI_FORMAT_UNKNOWN || format == m_SwapChainDesc.BufferDesc.Format) &&
       (sampleCount == 0 || sampleCount == m_SwapChainDesc.SampleDesc.Count))
        return S_FALSE;

    if(m_Device)
    {
        bool fullscreen = (GetWindowState() == kWindowFullscreen);
        if(fullscreen)
            m_SwapChain->SetFullscreenState(false, NULL);

        IDXGISwapChain* newSwapChain = NULL;
        DXGI_SWAP_CHAIN_DESC newSwapChainDesc = m_SwapChainDesc;

        if(format != DXGI_FORMAT_UNKNOWN)
            newSwapChainDesc.BufferDesc.Format = format;
        if(sampleCount != 0)
            newSwapChainDesc.SampleDesc.Count = sampleCount;

        IDXGIAdapter* pDXGIAdapter = GetDXGIAdapter();

        IDXGIFactory* pDXGIFactory = NULL;
        pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&pDXGIFactory));

        hr = pDXGIFactory->CreateSwapChain(m_Device, &newSwapChainDesc, &newSwapChain);

        pDXGIFactory->Release();
        pDXGIAdapter->Release();
        
        if (FAILED(hr))
        {
            if(fullscreen)
                m_SwapChain->SetFullscreenState(true, NULL);

            return hr;
        }

        SAFE_RELEASE(m_BackBufferRTV);
        SAFE_RELEASE(m_SwapChain);
        SAFE_RELEASE(m_DepthStencilBuffer);
        SAFE_RELEASE(m_DepthStencilDSV);

        m_SwapChain = newSwapChain;
        m_SwapChainDesc = newSwapChainDesc;

        m_DepthStencilDesc.SampleDesc.Count = sampleCount;

        if(fullscreen)                
            m_SwapChain->SetFullscreenState(true, NULL);

        CreateRenderTargetAndDepthStencil();
        BackBufferResized();
    }

    return S_OK;
}

void
DeviceManager::AddControllerToFront(IVisualController* pController) 
{ 
    m_vControllers.remove(pController);
    m_vControllers.push_front(pController);
}

void
DeviceManager::AddControllerToBack(IVisualController* pController) 
{
    m_vControllers.remove(pController);
    m_vControllers.push_back(pController);
}

void
DeviceManager::RemoveController(IVisualController* pController) 
{ 
    m_vControllers.remove(pController);
}

HRESULT 
DeviceManager::ResizeWindow(int width, int height)
{
    if(m_SwapChain == NULL)
        return E_FAIL;

    RECT rect;
    GetWindowRect(m_hWnd, &rect);

    ShowWindow(m_hWnd, SW_RESTORE);

    if(!MoveWindow(m_hWnd, rect.left, rect.top, width, height, true))
        return E_FAIL;

    // No need to call m_SwapChain->ResizeBackBuffer because MoveWindow will send WM_SIZE, which calls that function.

    return S_OK;
}

HRESULT            
DeviceManager::EnterFullscreenMode(int width, int height)
{
    if(m_SwapChain == NULL)
        return E_FAIL;

    if(GetWindowState() == kWindowFullscreen)
        return S_FALSE;
    
    if(width <= 0 || height <= 0)
    {
        width = m_SwapChainDesc.BufferDesc.Width;
        height = m_SwapChainDesc.BufferDesc.Height;
    }
     
    SetWindowLong(m_hWnd, GWL_STYLE, WINDOW_STYLE_FULLSCREEN);
    MoveWindow(m_hWnd, 0, 0, width, height, true);

    HRESULT hr = m_SwapChain->SetFullscreenState(true, NULL);

    if(FAILED(hr)) 
    {
        SetWindowLong(m_hWnd, GWL_STYLE, WINDOW_STYLE_NORMAL);
        return hr;
    }

    UpdateWindow(m_hWnd);
    m_SwapChain->GetDesc(&m_SwapChainDesc);

    return S_OK;
}

HRESULT            
DeviceManager::LeaveFullscreenMode(int windowWidth, int windowHeight)
{
    if(m_SwapChain == NULL)
        return E_FAIL;

    if(GetWindowState() != kWindowFullscreen)
        return S_FALSE;
     
    HRESULT hr = m_SwapChain->SetFullscreenState(false, NULL);
    if(FAILED(hr)) return hr;

    SetWindowLong(m_hWnd, GWL_STYLE, WINDOW_STYLE_NORMAL);

    if(windowWidth <= 0 || windowHeight <= 0)
    {
        windowWidth = m_SwapChainDesc.BufferDesc.Width;
        windowHeight = m_SwapChainDesc.BufferDesc.Height;
    }
    
    RECT rect = { 0, 0, windowWidth, windowHeight };
    AdjustWindowRect(&rect, WINDOW_STYLE_NORMAL, FALSE);
    MoveWindow(m_hWnd, 0, 0, rect.right - rect.left, rect.bottom - rect.top, true);
    UpdateWindow(m_hWnd);

    m_SwapChain->GetDesc(&m_SwapChainDesc);

    return S_OK;
}

HRESULT            
DeviceManager::ToggleFullscreen()
{
    if(GetWindowState() == kWindowFullscreen)
        return LeaveFullscreenMode();
    else
        return EnterFullscreenMode();
}

DeviceManager::WindowState
DeviceManager::GetWindowState() 
{ 
    if(m_SwapChain && !m_SwapChainDesc.Windowed)
        return kWindowFullscreen;

    if(m_hWnd == INVALID_HANDLE_VALUE)
        return kWindowNone;

    if(IsZoomed(m_hWnd))
        return kWindowMaximized;

    if(IsIconic(m_hWnd))
        return kWindowMinimized;

    return kWindowNormal;
}

HRESULT
DeviceManager::GetDisplayResolution(int& width, int& height)
{
    if(m_hWnd != INVALID_HANDLE_VALUE)
    {
        HMONITOR monitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO info;
        info.cbSize = sizeof(MONITORINFO);

        if(GetMonitorInfo(monitor, &info))
        {
            width = info.rcMonitor.right - info.rcMonitor.left;
            height = info.rcMonitor.bottom - info.rcMonitor.top;
            return S_OK;
        }
    }

    return E_FAIL;
}

IDXGIAdapter*   
DeviceManager::GetDXGIAdapter()
{   
    if(!m_Device)
        return NULL;

    IDXGIDevice* pDXGIDevice = NULL;
    m_Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&pDXGIDevice));

    IDXGIAdapter* pDXGIAdapter = NULL;
    pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&pDXGIAdapter));

    pDXGIDevice->Release();

    return pDXGIAdapter;
}
