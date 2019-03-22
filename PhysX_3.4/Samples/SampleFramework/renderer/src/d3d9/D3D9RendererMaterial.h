//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
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
