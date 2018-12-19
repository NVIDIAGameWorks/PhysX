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
