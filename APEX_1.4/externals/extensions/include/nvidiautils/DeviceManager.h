// TAGRELEASE: PUBLIC

#pragma once
#include <Windows.h>
#include <DXGI.h>
#include <D3D11.h>
#include <list>


struct DeviceCreationParameters
{
    bool startMaximized;
    bool startFullscreen;
    int backBufferWidth;
    int backBufferHeight;
    int refreshRate;
    int swapChainBufferCount;
    DXGI_FORMAT swapChainFormat;
    DXGI_FORMAT depthStencilFormat;
    DXGI_USAGE swapChainUsage;
    int swapChainSampleCount;
    int swapChainSampleQuality;
    UINT createDeviceFlags;
    D3D_DRIVER_TYPE driverType;
    D3D_FEATURE_LEVEL featureLevel;

	// For use in the case of multiple adapters.  If this is non-null, device creation will try to match
	// the given string against an adapter name.  If the specified string exists as a sub-string of the 
	// adapter name, the device and window will be created on that adapter.  Case sensitive.
	const WCHAR* adapterNameSubstring;

    DeviceCreationParameters() 
        : startMaximized(false)
        , startFullscreen(false)
        , backBufferWidth(1280)
        , backBufferHeight(720)
        , refreshRate(0)
        , swapChainBufferCount(1)
        , swapChainFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
        , depthStencilFormat(DXGI_FORMAT_D24_UNORM_S8_UINT)
        , swapChainUsage(DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT)
        , swapChainSampleCount(1)
        , swapChainSampleQuality(0)
        , createDeviceFlags(0)
        , driverType(D3D_DRIVER_TYPE_HARDWARE)
        , featureLevel(D3D_FEATURE_LEVEL_11_0)
		, adapterNameSubstring(L"")
    { }
};

#pragma warning(push)
#pragma warning(disable: 4100) // unreferenced formal parameter
class IVisualController
{
private:
    bool            m_Enabled;
public:
    IVisualController() : m_Enabled(true) { }

    virtual LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { return 1; }
    virtual void Render(ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext, ID3D11RenderTargetView* pRTV, ID3D11DepthStencilView* pDSV) { }
    virtual void Animate(double fElapsedTimeSeconds) { }
    virtual HRESULT DeviceCreated(ID3D11Device* pDevice) { return S_OK; }
    virtual void DeviceDestroyed() { }
    virtual void BackBufferResized(ID3D11Device* pDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc) { }

    virtual void EnableController() { m_Enabled = true; }
    virtual void DisableController() { m_Enabled = false; }
    virtual bool IsEnabled() { return m_Enabled; }
};
#pragma warning(pop)

class DeviceManager
{
public:
    enum WindowState
    {
        kWindowNone,
        kWindowNormal,
        kWindowMinimized,
        kWindowMaximized,
        kWindowFullscreen
    };

protected:
    ID3D11Device*           m_Device;
    ID3D11DeviceContext*    m_ImmediateContext;
    IDXGISwapChain*         m_SwapChain;
    ID3D11RenderTargetView* m_BackBufferRTV;
    ID3D11Texture2D*        m_DepthStencilBuffer;
    ID3D11DepthStencilView* m_DepthStencilDSV;
    DXGI_SWAP_CHAIN_DESC    m_SwapChainDesc;
    D3D11_TEXTURE2D_DESC    m_DepthStencilDesc;
	bool					m_IsNvidia;
    HWND                    m_hWnd;
    std::list<IVisualController*> m_vControllers;
    std::wstring            m_WindowTitle;
    double                  m_FixedFrameInterval;
    UINT                    m_SyncInterval;
    std::list<double>       m_vFrameTimes;
    double                  m_AverageFrameTime;
    double                  m_AverageTimeUpdateInterval;
    bool                    m_InSizingModalLoop;
    SIZE                    m_NewWindowSize;
private:
    HRESULT                 CreateRenderTargetAndDepthStencil();
    void                    ResizeSwapChain();
public:

    DeviceManager()
        : m_Device(NULL)
        , m_ImmediateContext(NULL)
        , m_SwapChain(NULL)
        , m_BackBufferRTV(NULL)
        , m_DepthStencilBuffer(NULL)
        , m_DepthStencilDSV(NULL)
		, m_IsNvidia(false)
        , m_hWnd(NULL)
        , m_WindowTitle(L"")
        , m_FixedFrameInterval(-1)
        , m_SyncInterval(0)
        , m_AverageFrameTime(0)
        , m_AverageTimeUpdateInterval(0.5)
        , m_InSizingModalLoop(false)
    { }

    virtual ~DeviceManager() 
    { Shutdown(); }

    virtual HRESULT CreateWindowDeviceAndSwapChain(const DeviceCreationParameters& params, LPWSTR windowTitle);
    virtual HRESULT ChangeBackBufferFormat(DXGI_FORMAT format, UINT sampleCount);
    virtual HRESULT ResizeWindow(int width, int height);
    virtual HRESULT EnterFullscreenMode(int width = 0, int height = 0);
    virtual HRESULT LeaveFullscreenMode(int windowWidth = 0, int windowHeight = 0);
    virtual HRESULT ToggleFullscreen();

    virtual void    Shutdown();
    virtual void    MessageLoop();
    virtual LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual void    Render();
    virtual void    Animate(double fElapsedTimeSeconds);
    virtual void    DeviceCreated();
    virtual void    DeviceDestroyed();
    virtual void    BackBufferResized();

    void            AddControllerToFront(IVisualController* pController);
    void            AddControllerToBack(IVisualController* pController);
    void            RemoveController(IVisualController* pController);

    void            SetFixedFrameInterval(double seconds) { m_FixedFrameInterval = seconds; }
    void            DisableFixedFrameInterval() { m_FixedFrameInterval = -1; }

	bool			IsNvidia() const { return m_IsNvidia; }
    HWND            GetHWND() { return m_hWnd; }
    ID3D11Device*   GetDevice() { return m_Device; }
    WindowState     GetWindowState();
    bool            GetVsyncEnabled() { return m_SyncInterval > 0; }
    void            SetVsyncEnabled(bool enabled) { m_SyncInterval = enabled ? 1 : 0; }
    HRESULT         GetDisplayResolution(int& width, int& height);
    IDXGIAdapter*   GetDXGIAdapter();
    double          GetAverageFrameTime() { return m_AverageFrameTime; }
    void            SetAverageTimeUpdateInterval(double value) { m_AverageTimeUpdateInterval = value; }
};


DeviceManager* GetDeviceManager();
