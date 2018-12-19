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
#ifndef D3D9_WPF_RENDERER_H
#define D3D9_WPF_RENDERER_H
#include "D3D9Renderer.h"

namespace SampleRenderer
{
	class WPFRendererListener
	{
	protected:
		virtual ~WPFRendererListener(){}
	public:

		virtual void beforeSurfaceRelease( IDirect3DSurface9* surface ) = 0;
		virtual void afterSurfaceCreated( IDirect3DSurface9* surface ) = 0;
		virtual void release() = 0;
	};

	//This renderer has special interop needs with WFP so it needs to use a slightly slowing
	//device specification.  Also it uses some special creation mechanisms so we get as much
	//performance as we can on various platforms taking the interop considerations into account.

	//One component is that SamplePlatform isn't initialized nor available thus forcing us to override
	//details on some functions
	class D3D9WPFRenderer : public D3D9Renderer
	{
		HWND					mHwnd;
		IDirect3DSurface9*		mCanonicalSurface;
		PxU32					mDesiredWidth;
		PxU32					mDesiredHeight;
		const bool				mLockable;
		WPFRendererListener*	mListener;

	protected:
		
		D3D9WPFRenderer(	HWND hWnd
							, IDirect3D9*	inDirect3d
							, IDirect3DDevice9* inDevice
							, const char* devName
							, PxU32 dispWidth
							, PxU32 dispHeight
							, const char* assetDir
							, bool lockable
							, bool isDeviceEx );

	public:
		static D3D9WPFRenderer* createWPFRenderer(const char* inAssetDir, PxU32 initialWidth = 1024, PxU32 initialHeight = 768 );

		virtual ~D3D9WPFRenderer();

		virtual IDirect3DSurface9* getSurface() { return mCanonicalSurface; }
		virtual WPFRendererListener* getListener();
		virtual void setListener( WPFRendererListener* listener );

		virtual void onResize( PxU32 newWidth, PxU32 newHeight );
		virtual bool swapBuffers();
		virtual bool isOk() const;
		virtual void onDeviceLost();
		virtual void onDeviceReset();

		virtual PxU32 getWidth() { return mDesiredWidth; }
		virtual PxU32 getHeight() { return mDesiredHeight; }
		virtual void render() { beginRender(); endRender(); }
		virtual void setupAlphaBlendState();
		virtual void restoreAlphaBlendState();
		virtual void disableZWrite();
		virtual void enableZWrite();
	protected:
		virtual bool checkResize( bool isDeviceLost );
		void releaseSurface();
		void allocateSurface();
	};
}

#endif