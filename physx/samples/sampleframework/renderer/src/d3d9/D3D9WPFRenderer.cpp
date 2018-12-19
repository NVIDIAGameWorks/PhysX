// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
#ifdef WIN32 // win32 only

#include "D3D9WPFRenderer.h"
#include "SamplePlatform.h"

#define APP_NAME "D3D9WPFRenderer"

namespace SampleRenderer
{
	

	static volatile LONG classRefCount = 0;

	static LONG incrementClassReference()
	{
		return InterlockedIncrement( &classRefCount );
	}
	static LONG decrementClassReference()
	{
		return InterlockedDecrement( &classRefCount );
	}
		
	D3D9WPFRenderer::D3D9WPFRenderer(  HWND hWnd
									, IDirect3D9*	inDirect3d
									, IDirect3DDevice9* inDevice
									, const char* devName
									, PxU32 dispWidth
									, PxU32 dispHeight
									, const char* assetDir
									, bool lockable
									, bool isDeviceEx )
		: D3D9Renderer( inDirect3d, devName, 0, 0, inDevice, isDeviceEx, assetDir )
		, mHwnd( hWnd )
		, mDesiredWidth( dispWidth )
		, mDesiredHeight( dispHeight )
		, mCanonicalSurface( NULL )
		, mLockable( lockable )
		, mListener( NULL )
	{
		SampleFramework::createPlatform(NULL);
	}

	D3D9WPFRenderer::~D3D9WPFRenderer()
	{
		releaseSurface();
		DestroyWindow(mHwnd); 
		if ( mListener ) mListener->release(); mListener = NULL;
		if ( decrementClassReference() == 0 )
			UnregisterClass(TEXT(APP_NAME), NULL);
	}
	
	WPFRendererListener* D3D9WPFRenderer::getListener() { return mListener; }
	void D3D9WPFRenderer::setListener( WPFRendererListener* listener )
	{
		if ( mListener != listener )
		{
			if ( mListener ) mListener->release(); mListener = NULL;
			mListener = listener;
			if ( mCanonicalSurface && mListener ) mListener->afterSurfaceCreated( mCanonicalSurface );
		}
	}

	void D3D9WPFRenderer::onResize( PxU32 newWidth, PxU32 newHeight )
	{
		mDesiredWidth = newWidth;
		mDesiredHeight = newHeight;
		checkResize( false );
	}

	bool D3D9WPFRenderer::swapBuffers()
	{
		return true; //does nothing, that is up for the parent context to do.
	}

	bool D3D9WPFRenderer::isOk() const
	{
		return true;}
	
	void D3D9WPFRenderer::onDeviceLost()
	{
		releaseSurface();
		D3D9Renderer::onDeviceLost();
	}

	void D3D9WPFRenderer::onDeviceReset()
	{
		allocateSurface();
		D3D9Renderer::onDeviceReset();
	}

	void D3D9WPFRenderer::releaseSurface()
	{
		if( mListener ) mListener->beforeSurfaceRelease( mCanonicalSurface );
		if ( mCanonicalSurface ) mCanonicalSurface->Release();
		mCanonicalSurface = NULL;
	}

	void D3D9WPFRenderer::allocateSurface()
	{
		PX_ASSERT( mCanonicalSurface == NULL );

		HRESULT result = m_d3dDevice->CreateRenderTarget(
				m_displayWidth
				, m_displayHeight 
				, D3DFMT_X8R8G8B8
				, D3DMULTISAMPLE_NONE
				, 0
				, mLockable
				, &mCanonicalSurface
				, NULL );
		PX_ASSERT( mCanonicalSurface );
		PX_ASSERT( result == D3D_OK );

		if ( mCanonicalSurface != NULL )
			m_d3dDevice->SetRenderTarget(0, mCanonicalSurface);

		if ( mListener ) mListener->afterSurfaceCreated( mCanonicalSurface );
	}

	bool D3D9WPFRenderer::checkResize( bool isDeviceLost )
	{
		bool isDeviceReset = false;
		//resize the system if the desired width or height is more than we can support.
		if ( mDesiredWidth > m_displayWidth
			|| mDesiredHeight > m_displayHeight
			|| isDeviceLost )
		{
			m_displayWidth = PxMax( mDesiredWidth, m_displayWidth );
			m_displayHeight = PxMax( mDesiredHeight, m_displayHeight );
			if ( isDeviceLost )
			{
				physx::PxU64 res = m_d3dDevice->TestCooperativeLevel();
				if(res == D3D_OK || res == D3DERR_DEVICENOTRESET)	//if device is lost, device has to be ready for reset
				{
					isDeviceReset = true;
					onDeviceLost();
					onDeviceReset();
				}
			}
			else
			{
				releaseSurface();
				releaseDepthStencilSurface();
				allocateSurface();
				buildDepthStencilSurface();
				// set out initial states...
				m_d3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
				m_d3dDevice->SetRenderState(D3DRS_LIGHTING, 0);
				m_d3dDevice->SetRenderState(D3DRS_ZENABLE,  1);
			}
		}
		//else just mess with the viewport so we only render pixel-by-pixel
		D3DVIEWPORT9 viewport = {0};
		m_d3dDevice->GetViewport(&viewport);
		if ( viewport.Width != mDesiredWidth
			|| viewport.Height != mDesiredHeight )
		{
			viewport.X = 0;
			viewport.Y = 0;
			viewport.Width  = (DWORD)mDesiredWidth;
			viewport.Height = (DWORD)mDesiredHeight;
			viewport.MinZ =  0.0f;
			viewport.MaxZ =  1.0f;
			m_d3dDevice->SetViewport(&viewport);
		}
		return isDeviceReset;
	}
	
	void D3D9WPFRenderer::setupAlphaBlendState()
	{
		m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,		1);
		m_d3dDevice->SetRenderState(D3DRS_SRCBLEND,				D3DBLEND_ONE);
		m_d3dDevice->SetRenderState(D3DRS_DESTBLEND,			D3DBLEND_INVSRCALPHA);
	}

	void D3D9WPFRenderer::restoreAlphaBlendState()
	{
		m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,  0);
	}

	void D3D9WPFRenderer::disableZWrite()
	{
		m_d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE,  0);
	}
	void D3D9WPFRenderer::enableZWrite()
	{
		m_d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE,  1);
	}
	
	static DWORD GetVertexProcessingCaps(IDirect3D9* d3d)
	{
		D3DCAPS9 caps;
		DWORD dwVertexProcessing = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
		if (SUCCEEDED(d3d->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps)))
		{
			if ((caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == D3DDEVCAPS_HWTRANSFORMANDLIGHT)
			{
				dwVertexProcessing = D3DCREATE_HARDWARE_VERTEXPROCESSING;
			}
		}
		return dwVertexProcessing;
	}


	HRESULT InitializeD3D(HWND hWnd, D3DPRESENT_PARAMETERS d3dpp, IDirect3D9*& d3d, IDirect3DDevice9*& d3dDevice )
	{
		d3d = NULL;
		d3dDevice = NULL;
		// initialize Direct3D
		d3d = Direct3DCreate9(D3D_SDK_VERSION);
		if ( d3d != NULL )
		{
			// determine what type of vertex processing to use based on the device capabilities
			DWORD dwVertexProcessing = GetVertexProcessingCaps(d3d);
			// create the D3D device
			return d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
									dwVertexProcessing | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
									&d3dpp, &d3dDevice);
		}
		return -1;
	}

	typedef HRESULT  (WINAPI *D3DEXCreateFn)( DWORD version, IDirect3D9Ex** result );


	HRESULT InitializeD3DEx(HWND hWnd, D3DPRESENT_PARAMETERS d3dpp, IDirect3D9*& d3d, IDirect3DDevice9*& d3dDevice, D3DEXCreateFn createFn )
	{
		d3d = NULL;
		d3dDevice = NULL;
		IDirect3D9Ex* d3dEx = NULL;
		// initialize Direct3D using the Ex function
		HRESULT result = createFn( D3D_SDK_VERSION, &d3dEx );
		if ( FAILED( result ) ) return result;

		result = d3dEx->QueryInterface(__uuidof(IDirect3D9), reinterpret_cast<void **>(&d3d) );	
		if ( FAILED( result ) ) { d3dEx->Release(); return result; }
	
		// determine what type of vertex processing to use based on the device capabilities
		DWORD dwVertexProcessing = GetVertexProcessingCaps(d3d);

		// create the D3D device using the Ex function
		IDirect3DDevice9Ex* deviceEx = NULL;
		result = d3dEx->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
				dwVertexProcessing | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
				&d3dpp, NULL, &deviceEx);

		if ( FAILED(result) ) { d3dEx->Release(); d3d->Release();  return result; }
		
		// obtain the standard D3D device interface
		deviceEx->QueryInterface(__uuidof(IDirect3DDevice9), reinterpret_cast<void **>(&d3dDevice) );

		deviceEx->Release();
		d3dEx->Release();

		return 0;
	}

	D3D9WPFRenderer* D3D9WPFRenderer::createWPFRenderer( const char* inAssetDir, PxU32 initialWidth, PxU32 initialHeight )
	{
		if ( incrementClassReference() == 1 )
		{
			WNDCLASS wndclass;
			memset( &wndclass, 0, sizeof( WNDCLASS ) );

			wndclass.style = CS_HREDRAW | CS_VREDRAW;
			wndclass.lpfnWndProc = DefWindowProc;
			wndclass.lpszClassName = TEXT(APP_NAME);

			RegisterClass(&wndclass);
		}

        HWND hWnd = CreateWindow(TEXT(APP_NAME),
                            TEXT(APP_NAME),
                            WS_OVERLAPPEDWINDOW,
                            0,                   // Initial X
                            0,                   // Initial Y
                            0,                   // Width
                            0,                   // Height
                            NULL,
                            NULL,
                            NULL,
                            NULL);
		if ( hWnd == 0 ) return NULL;

		HMODULE hD3D9 = LoadLibrary(TEXT("d3d9.dll"));
		D3DEXCreateFn exCreateFn = reinterpret_cast<D3DEXCreateFn>( GetProcAddress(hD3D9, "Direct3DCreate9Ex") );
		bool is9x = exCreateFn != NULL;
		IDirect3D9*			d3d = NULL;
		IDirect3DDevice9*	d3dDevice = NULL;
		D3DPRESENT_PARAMETERS d3dpp;
		ZeroMemory(&d3dpp, sizeof(d3dpp));
		d3dpp.Windowed = TRUE;
		d3dpp.BackBufferHeight = 1;
		d3dpp.BackBufferWidth = 1;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
		d3dpp.hDeviceWindow = hWnd;
		HRESULT result;

		if (is9x)
		{
			result = InitializeD3DEx(NULL, d3dpp, d3d, d3dDevice, exCreateFn);
		}
		else
		{
			result = InitializeD3D(NULL, d3dpp, d3d, d3dDevice);
		}
		PX_ASSERT( result >= 0 );
		if ( result >= 0 )
		{
			bool lockable = !is9x; 
			//We only lock earlier interface, not later ones because the later ones use a hardware transfer mechanism
			//where you seem to not need to lock according to some...
			D3D9WPFRenderer* retval = new D3D9WPFRenderer( hWnd, d3d, d3dDevice, "Direct3d", initialWidth, initialHeight, inAssetDir, !is9x, is9x );
			retval->checkResize( false );	//setup viewport.
			//Can we create a vertex buffer here?
			PX_ASSERT( result == D3D_OK );
			return retval;
		}
		return NULL;
	}
}

#endif