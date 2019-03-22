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


#ifndef RENDERER_LIGHT_H
#define RENDERER_LIGHT_H

#include <RendererConfig.h>
#include <RendererColor.h>
#include <RendererMaterial.h>
#include <RendererProjection.h>

namespace SampleRenderer
{

	class RendererLightDesc;
	class Renderer;
	class RendererLight
	{
		friend class Renderer;
	public:
		enum Type
		{
			TYPE_POINT = 0,
			TYPE_DIRECTIONAL,
			TYPE_SPOT,

			NUM_TYPES
		}_Type;

	protected:
		RendererLight(const RendererLightDesc &desc);
		virtual ~RendererLight(void);

	public:
		void                      release(void);

		Type                      getType(void) const;

		RendererMaterial::Pass    getPass(void) const;

		const RendererColor      &getColor(void) const;
		void                      setColor(const RendererColor &color);

		float                     getIntensity(void) const;
		void                      setIntensity(float intensity);

		bool                      isLocked(void) const;

		RendererTexture2D        *getShadowMap(void) const;
		void                      setShadowMap(RendererTexture2D *shadowMap);

		const physx::PxTransform &getShadowTransform(void) const	{ return m_shadowTransform; }
		void                      setShadowTransform(const physx::PxTransform &shadowTransform) { m_shadowTransform = shadowTransform; }

		const RendererProjection &getShadowProjection(void) const;
		void                      setShadowProjection(const RendererProjection &shadowProjection);

	private:
		RendererLight &operator=(const RendererLight &) { return *this; }

		virtual void bind(void) const = 0;

	protected:
		const Type         m_type;

		RendererColor      m_color;
		float              m_intensity;

		RendererTexture2D *m_shadowMap;
		physx::PxTransform m_shadowTransform;
		RendererProjection m_shadowProjection;

	private:
		Renderer          *m_renderer;
	};

} // namespace SampleRenderer

#endif
