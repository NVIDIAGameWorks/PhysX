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
#ifndef D3D9_RENDERER_MATERIAL_H
#define D3D9_RENDERER_MATERIAL_H

#include <RendererConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <RendererMaterial.h>

#include "D3D9Renderer.h"

namespace SampleRenderer
{

	class D3D9Renderer;

	class D3D9RendererMaterial : public RendererMaterial
	{
	public:
		D3D9RendererMaterial(D3D9Renderer &renderer, const RendererMaterialDesc &desc);
		virtual ~D3D9RendererMaterial(void);
		virtual void setModelMatrix(const float *matrix);

	private:
		virtual const Renderer& getRenderer() const { return m_renderer; }
		virtual void bind(RendererMaterial::Pass pass, RendererMaterialInstance *materialInstance, bool instanced) const;
		virtual void bindMeshState(bool instanced) const;
		virtual void unbind(void) const;
		virtual void bindVariable(Pass pass, const Variable &variable, const void *data) const;

		void loadCustomConstants(ID3DXConstantTable &table, Pass pass);
		HRESULT loadCacheShader(LPCSTR shaderFilePath, const char* suffix, CONST D3DXMACRO *defines, LPD3DXINCLUDE shaderIncluder,
			                    LPCSTR functionName, LPCSTR profile, DWORD flags, LPD3DXBUFFER &shader, LPD3DXCONSTANTTABLE *constantTable);
		
	private:
		class ShaderConstants
		{
		public:
			LPD3DXCONSTANTTABLE table;

			D3DXHANDLE          modelMatrix;
			D3DXHANDLE          viewMatrix;
			D3DXHANDLE          projMatrix;
			D3DXHANDLE          modelViewMatrix;
			D3DXHANDLE          modelViewProjMatrix;

			D3DXHANDLE          boneMatrices;

			D3DXHANDLE          fogColorAndDistance;

			D3DXHANDLE          eyePosition;
			D3DXHANDLE          eyeDirection;

			D3DXHANDLE          ambientColor;

			D3DXHANDLE          lightColor;
			D3DXHANDLE          lightIntensity;
			D3DXHANDLE          lightDirection;
			D3DXHANDLE          lightPosition;
			D3DXHANDLE          lightInnerRadius;
			D3DXHANDLE          lightOuterRadius;
			D3DXHANDLE          lightInnerCone;
			D3DXHANDLE          lightOuterCone;
			D3DXHANDLE          lightShadowMap;
			D3DXHANDLE          lightShadowMatrix;

			D3DXHANDLE          vfaceScale;

		public:
			ShaderConstants(void);
			~ShaderConstants(void);

			void loadConstants(void);

			void bindEnvironment(IDirect3DDevice9 &d3dDevice, const D3D9Renderer::ShaderEnvironment &shaderEnv) const;
		};

		class D3D9Variable : public Variable
		{
			friend class D3D9RendererMaterial;
		public:
			D3D9Variable(const char *name, VariableType type, PxU32 offset);
			virtual ~D3D9Variable(void);

			void addVertexHandle(ID3DXConstantTable &table, D3DXHANDLE handle);
			void addFragmentHandle(ID3DXConstantTable &table, D3DXHANDLE handle, Pass pass);

		private:
			D3D9Variable &operator=(const D3D9Variable&) { return *this; }

		private:
			D3DXHANDLE          m_vertexHandle;
			UINT                m_vertexRegister;
			D3DXHANDLE          m_fragmentHandles[NUM_PASSES];
			UINT                m_fragmentRegisters[NUM_PASSES];
		};

	private:
		D3D9RendererMaterial &operator=(const D3D9RendererMaterial&) { return *this; }

	private:
		D3D9Renderer           &m_renderer;

		D3DCMPFUNC              m_d3dAlphaTestFunc;
		D3DBLEND                m_d3dSrcBlendFunc;
		D3DBLEND                m_d3dDstBlendFunc;

		IDirect3DVertexShader9 *m_vertexShader;
		IDirect3DVertexShader9 *m_instancedVertexShader;
		IDirect3DPixelShader9  *m_fragmentPrograms[NUM_PASSES];

		ShaderConstants         m_vertexConstants;
		ShaderConstants         m_instancedVertexConstants;
		ShaderConstants         m_fragmentConstants[NUM_PASSES];
	};

} // namespace SampleRenderer

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
#endif
