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

#ifndef RENDERER_MATERIAL_H
#define RENDERER_MATERIAL_H

#include <RendererConfig.h>
#include <vector>

namespace SampleRenderer
{
    class Renderer;
	class RendererMaterialDesc;
    class RendererMaterialInstance;

	class RendererTexture;
	class RendererTextureDesc;
	
	class RendererMaterial
	{
		friend class Renderer;
		friend class RendererMaterialInstance;
	public:
		typedef enum Type
		{
			TYPE_UNLIT = 0,
			TYPE_LIT,		
			NUM_TYPES,
		} Type;

		typedef enum AlphaTestFunc
		{
			ALPHA_TEST_ALWAYS = 0, // disabled alpha testing...
			ALPHA_TEST_EQUAL,
			ALPHA_TEST_NOT_EQUAL,
			ALPHA_TEST_LESS,
			ALPHA_TEST_LESS_EQUAL,
			ALPHA_TEST_GREATER,
			ALPHA_TEST_GREATER_EQUAL,

			NUM_ALPHA_TEST_FUNCS,
		} AlphaTestFunc;

		typedef enum BlendFunc
		{
			BLEND_ZERO = 0,
			BLEND_ONE,
			BLEND_SRC_COLOR,
			BLEND_ONE_MINUS_SRC_COLOR,
			BLEND_SRC_ALPHA,
			BLEND_ONE_MINUS_SRC_ALPHA,
			BLEND_DST_ALPHA,
			BLEND_ONE_MINUS_DST_ALPHA,
			BLEND_DST_COLOR,
			BLEND_ONE_MINUS_DST_COLOR,
			BLEND_SRC_ALPHA_SATURATE,
		} BlendFunc;

		typedef enum Pass
		{
			PASS_UNLIT = 0,

			PASS_AMBIENT_LIGHT,
			PASS_POINT_LIGHT,
			PASS_DIRECTIONAL_LIGHT,
			PASS_SPOT_LIGHT_NO_SHADOW,
			PASS_SPOT_LIGHT,

			PASS_NORMALS,
			PASS_DEPTH,

			// LRR: The deferred pass causes compiles with the ARB_draw_buffers profile option, creating 
			// multiple color draw buffers.  This doesn't work in OGL on ancient Intel parts.
			//PASS_DEFERRED,

			NUM_PASSES,
		} Pass;

		typedef enum VariableType
		{
			VARIABLE_FLOAT = 0,
			VARIABLE_FLOAT2,
			VARIABLE_FLOAT3,
			VARIABLE_FLOAT4,
			VARIABLE_FLOAT4x4,
			VARIABLE_INT,
			VARIABLE_SAMPLER2D,
			VARIABLE_SAMPLER3D,

			NUM_VARIABLE_TYPES
		} VariableType;

		class Variable
		{
			friend class RendererMaterial;
		protected:
			Variable(const char *name, VariableType type, PxU32 offset);
			virtual ~Variable(void);

		public:
			const char  *getName(void) const;
			VariableType getType(void) const;
			PxU32        getDataOffset(void) const;
			PxU32        getDataSize(void) const;

			void		 setSize(PxU32);

		private:
			char        *m_name;
			VariableType m_type;
			PxU32        m_offset;
			PxI32		 m_size;
		};

	public:
		static const char *getPassName(Pass pass);
		virtual void setModelMatrix(const PxF32 *matrix) = 0;

	protected:
		RendererMaterial(const RendererMaterialDesc &desc, bool enableMaterialCaching);
		virtual ~RendererMaterial(void);

	public:
		void incRefCount() { m_refCount++; }
		void release(void);
		const Variable *findVariable(const char *name, VariableType varType);

		PX_FORCE_INLINE	Type			getType(void)						const { return m_type; }
		PX_FORCE_INLINE	AlphaTestFunc	getAlphaTestFunc(void)				const { return m_alphaTestFunc; }
		PX_FORCE_INLINE	float			getAlphaTestRef(void)				const { return m_alphaTestRef; }
		PX_FORCE_INLINE	bool			getBlending(void)					const { return m_blending && !rendererBlendingOverrideEnabled(); }
		PX_FORCE_INLINE	BlendFunc		getSrcBlendFunc(void)				const { return m_srcBlendFunc; }
		PX_FORCE_INLINE	BlendFunc		getDstBlendFunc(void)				const { return m_dstBlendFunc; }
		PX_FORCE_INLINE	PxU32	getMaterialInstanceDataSize(void)	const { return m_variableBufferSize; }

	protected:
		virtual const Renderer& getRenderer() const = 0;
		virtual void bind(RendererMaterial::Pass pass, RendererMaterialInstance *materialInstance, bool instanced) const;
		virtual void bindMeshState(bool instanced) const = 0;
		virtual void unbind(void) const = 0;
		virtual void bindVariable(Pass pass, const Variable &variable, const void *data) const = 0;

		RendererMaterial &operator=(const RendererMaterial&) { return *this; }

		bool rendererBlendingOverrideEnabled() const;

	protected:
		const Type          m_type;

		const AlphaTestFunc m_alphaTestFunc;
		float               m_alphaTestRef;

		bool                m_blending;
		const BlendFunc     m_srcBlendFunc;
		const BlendFunc     m_dstBlendFunc;

		std::vector<Variable*> m_variables;
		PxU32               m_variableBufferSize;

		PxI32               m_refCount;
		bool				mEnableMaterialCaching;
	};
} // namespace SampleRenderer

#endif
