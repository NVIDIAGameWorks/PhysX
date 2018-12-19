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
#ifndef D3D9_RENDERER_H
#define D3D9_RENDERER_H

#include <RendererConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <Renderer.h>
#include <vector>

#if defined(RENDERER_DEBUG)
	//#define D3D_DEBUG_INFO 1
#endif

#include <d3d9.h>
#include <d3dx9.h>

// Enables/Disables use of non-managed resources for vertex/instance buffers.
// Disabled for now as it appears to actually slow us down significantly on particles.
#define RENDERER_ENABLE_DYNAMIC_VB_POOLS 1
#define RENDERER_ENABLE_DIRECT3D9_SWAPCHAIN 1

struct IDirect3DSwapChain9;

namespace SampleRenderer
{

	class RendererDesc;
	class RendererColor;

	void convertToD3D9(D3DCOLOR &dxcolor, const RendererColor &color);
	void convertToD3D9(float *dxvec, const PxVec3 &vec);
	void convertToD3D9(D3DMATRIX &dxmat, const physx::PxMat44 &mat);
	void convertToD3D9(D3DMATRIX &dxmat, const RendererProjection &mat);

	class D3D9RendererResource;

	class D3D9Renderer : public Renderer
	{
		friend class D3D9RendererResource;
	public:
		class D3DXInterface
		{
			friend class D3D9Renderer;
		private:
			D3DXInterface(void);
			~D3DXInterface(void);

		public:
			HRESULT CompileShaderFromFileA(LPCSTR srcFile, CONST D3DXMACRO *defines, LPD3DXINCLUDE include, LPCSTR functionName, LPCSTR profile, DWORD flags, LPD3DXBUFFER *shader, LPD3DXBUFFER *errorMsgs, LPD3DXCONSTANTTABLE *constantTable);
			HRESULT GetShaderConstantTable(const DWORD* pFunction, LPD3DXCONSTANTTABLE *constantTable);
			LPCSTR  GetVertexShaderProfile(LPDIRECT3DDEVICE9 device);
			LPCSTR  GetPixelShaderProfile(LPDIRECT3DDEVICE9 device);
			HRESULT SaveSurfaceToFileInMemory(LPD3DXBUFFER *ppDestBuf, D3DXIMAGE_FILEFORMAT DestFormat, LPDIRECT3DSURFACE9 pSrcSurface, const PALETTEENTRY *pSrcPalette, const RECT *pSrcRect);
			HRESULT CreateBuffer(DWORD NumBytes, LPD3DXBUFFER *ppBuffer);
			HRESULT SaveSurfaceToFileA( LPCSTR pDestFile, D3DXIMAGE_FILEFORMAT DestFormat, LPDIRECT3DSURFACE9 pSrcSurface, CONST PALETTEENTRY* pSrcPalette, CONST RECT*  pSrcRect);

		public:
#if defined(RENDERER_WINDOWS) 
			HMODULE  m_library;
			HMODULE  m_compiler_library;

#define DEFINE_D3DX_FUNCTION(_name, _return, _params) \
	typedef _return (WINAPI* LP##_name) _params;      \
	LP##_name m_##_name;

			DEFINE_D3DX_FUNCTION(D3DXCompileShaderFromFileA, HRESULT, (LPCSTR, CONST D3DXMACRO*, LPD3DXINCLUDE, LPCSTR, LPCSTR, DWORD, LPD3DXBUFFER*, LPD3DXBUFFER*, LPD3DXCONSTANTTABLE *))
			DEFINE_D3DX_FUNCTION(D3DXGetVertexShaderProfile, LPCSTR, (LPDIRECT3DDEVICE9));
			DEFINE_D3DX_FUNCTION(D3DXGetPixelShaderProfile,  LPCSTR, (LPDIRECT3DDEVICE9));
			DEFINE_D3DX_FUNCTION(D3DXSaveSurfaceToFileInMemory, HRESULT, (LPD3DXBUFFER*, D3DXIMAGE_FILEFORMAT, LPDIRECT3DSURFACE9, const PALETTEENTRY*, const RECT*));
			DEFINE_D3DX_FUNCTION(D3DXSaveSurfaceToFileA, HRESULT, (LPCSTR ,D3DXIMAGE_FILEFORMAT , LPDIRECT3DSURFACE9 , CONST PALETTEENTRY* , CONST RECT* ));
			DEFINE_D3DX_FUNCTION(D3DXGetShaderConstantTable, HRESULT, (const DWORD* , LPD3DXCONSTANTTABLE*));
			DEFINE_D3DX_FUNCTION(D3DXCreateBuffer, HRESULT, (DWORD, LPD3DXBUFFER*));

#undef DEFINE_D3DX_FUNCTION
#endif
		};

		class ShaderEnvironment
		{
		public:
			D3DMATRIX          modelMatrix;
			D3DMATRIX          viewMatrix;
			D3DMATRIX          projMatrix;
			D3DMATRIX          modelViewMatrix;
			D3DMATRIX          modelViewProjMatrix;

			D3DXMATRIX         boneMatrices[RENDERER_MAX_BONES];
			PxU32              numBones;

			float              fogColorAndDistance[4];

			float              eyePosition[3];
			float              eyeDirection[3];

			D3DCOLOR           ambientColor;

			D3DCOLOR           lightColor;
			float              lightIntensity;
			float              lightDirection[3];
			float              lightPosition[3];
			float              lightInnerRadius;
			float              lightOuterRadius;
			float              lightInnerCone;
			float              lightOuterCone;
			IDirect3DTexture9 *lightShadowMap;
			D3DXMATRIX         lightShadowMatrix;

		public:
			ShaderEnvironment(void);
		};

	protected:
		D3D9Renderer(IDirect3D9* inDirect3d, const char* devName, PxU32 dispWidth, PxU32 dispHeight, IDirect3DDevice9* inDevice, bool isDeviceEx, const char* assetDir);
		void initialize(bool isDeviceEx);

	public:
		D3D9Renderer(const RendererDesc &desc, const char* assetDir);
		virtual ~D3D9Renderer(void);

		IDirect3DDevice9        *getD3DDevice(void)               { return m_d3dDevice; }
		D3DXInterface           &getD3DX(void)                    { return m_d3dx; }
		ShaderEnvironment       &getShaderEnvironment(void)       { return m_environment; }
		const ShaderEnvironment &getShaderEnvironment(void) const { return m_environment; }

	protected:
		virtual bool checkResize(bool isDeviceLost);
		void buildDepthStencilSurface(void);
		void buildSwapchain();
		void releaseSwapchain();
		bool useSwapchain() const;
		bool validSwapchain() const;
		HRESULT presentSwapchain();

		void releaseDepthStencilSurface(void);
	public:
		bool resetDevice(void);
		void onDeviceLost(void);
		void onDeviceReset(void);

	public:
		// clears the offscreen buffers.
		virtual void clearBuffers(void);

		// presents the current color buffer to the screen.
		// returns true on device reset and if buffers need to be rewritten.
		virtual bool swapBuffers(void);

		// get the device pointer (void * abstraction)
		virtual void *getDevice()                                 { return static_cast<void*>(getD3DDevice()); }

		// get the window size
		void getWindowSize(PxU32 &width, PxU32 &height) const;

		// gets a handle to the current frame's data, in bitmap format
		//    note: subsequent calls will invalidate any previously returned data
		//    return true on successful screenshot capture
		virtual bool					captureScreen(PxU32 &width, PxU32& height, PxU32& sizeInBytes, const void*& screenshotData);

		virtual RendererVertexBuffer   *createVertexBuffer(  const RendererVertexBufferDesc   &desc);
		virtual RendererIndexBuffer    *createIndexBuffer(   const RendererIndexBufferDesc    &desc);
		virtual RendererInstanceBuffer *createInstanceBuffer(const RendererInstanceBufferDesc &desc);
		virtual RendererTexture2D      *createTexture2D(     const RendererTexture2DDesc      &desc);
		virtual RendererTexture3D      *createTexture3D(     const RendererTexture3DDesc      &desc);
		virtual RendererTarget         *createTarget(        const RendererTargetDesc         &desc);
		virtual RendererMaterial       *createMaterial(      const RendererMaterialDesc       &desc);
		virtual RendererMesh           *createMesh(          const RendererMeshDesc           &desc);
		virtual RendererLight          *createLight(         const RendererLightDesc          &desc);
		
		virtual void disableDepthTest();
		virtual void enableDepthTest();

		virtual void                    setVsync(bool on);

		virtual	bool					initTexter();
		virtual	void					closeTexter();

		virtual bool beginRender(void);
		virtual void endRender(void);

		bool canUseManagedResources() { return !m_isDeviceEx; }

		virtual void bindViewProj(const physx::PxMat44 &eye, const RendererProjection &proj);
		virtual void bindFogState(const RendererColor &fogColor, float fogDistance);
		virtual void bindAmbientState(const RendererColor &ambientColor);
		virtual void bindDeferredState(void);
		virtual void bindMeshContext(const RendererMeshContext &context);
		virtual void beginMultiPass(void);
		virtual void endMultiPass(void);
		virtual void beginTransparentMultiPass(void);
		virtual void endTransparentMultiPass(void);
		virtual void renderDeferredLight(const RendererLight &light);
		virtual PxU32 convertColor(const RendererColor& color) const;

		virtual bool isOk(void) const;

		virtual	void setupTextRenderStates();
		virtual	void resetTextRenderStates();
		virtual	void renderTextBuffer(const void* vertices, PxU32 nbVerts, const PxU16* indices, PxU32 nbIndices, RendererMaterial* material);
		virtual	void renderLines2D(const void* vertices, PxU32 nbVerts);
		virtual	void setupScreenquadRenderStates();
		virtual	void resetScreenquadRenderStates();

	private:
		void addResource(D3D9RendererResource &resource);
		void removeResource(D3D9RendererResource &resource);
		void notifyResourcesLostDevice(void);
		void notifyResourcesResetDevice(void);

	protected:
		PxU32                          m_displayWidth;
		PxU32                          m_displayHeight;
		ID3DXBuffer                   *m_displayBuffer;

		IDirect3D9                    *m_d3d;
		IDirect3DDevice9              *m_d3dDevice;
		IDirect3DSurface9             *m_d3dDepthStencilSurface;
		IDirect3DSurface9             *m_d3dSurface;

		IDirect3DSwapChain9           *m_d3dSwapChain;
		IDirect3DSurface9             *m_d3dSwapDepthStencilSurface;
		IDirect3DSurface9             *m_d3dSwapSurface;
	private:
		bool						   m_isDeviceEx; //implications all over the place

		D3DXInterface                  m_d3dx;

		ShaderEnvironment              m_environment;

		D3DPRESENT_PARAMETERS          m_d3dPresentParams;
		bool                           m_d3dPresentParamsChanged;

		physx::PxMat44				   m_viewMatrix;

		// non-managed resources...
		std::vector<D3D9RendererResource*> m_resources;

		IDirect3DVertexDeclaration9*	m_textVDecl;
	};

	class D3D9RendererResource
	{
		friend class D3D9Renderer;
	public:
		D3D9RendererResource(void)
		{
			m_d3dRenderer = 0;
		}

		virtual ~D3D9RendererResource(void)
		{
			if(m_d3dRenderer) m_d3dRenderer->removeResource(*this);
		}

	public:
		virtual void onDeviceLost(void)=0;
		virtual void onDeviceReset(void)=0;

	private:
		D3D9Renderer *m_d3dRenderer;
	};

} // namespace SampleRenderer

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
#endif
