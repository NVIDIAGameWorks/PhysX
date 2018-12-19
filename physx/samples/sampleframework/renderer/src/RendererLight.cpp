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
#include <RendererLight.h>
#include <RendererLightDesc.h>

#include <Renderer.h>

using namespace SampleRenderer;

RendererLight::RendererLight(const RendererLightDesc &desc) :
m_type(desc.type),
m_shadowProjection(45, 1, 0.1f, 100.0f)
{
	m_renderer = 0;
	setColor(desc.color);
	setIntensity(desc.intensity);
	setShadowMap(desc.shadowMap);
	setShadowTransform(desc.shadowTransform);
	setShadowProjection(desc.shadowProjection);
}

RendererLight::~RendererLight(void)
{
	RENDERER_ASSERT(!isLocked(), "Light is locked by a Renderer during release.");
}

void SampleRenderer::RendererLight::release(void)
{
	if (m_renderer) m_renderer->removeLightFromRenderQueue(*this);
	delete this;
}

RendererLight::Type RendererLight::getType(void) const
{
	return m_type;
}

RendererMaterial::Pass RendererLight::getPass(void) const
{
	RendererMaterial::Pass pass = RendererMaterial::NUM_PASSES;
	switch(m_type)
	{
	case TYPE_POINT:       pass = RendererMaterial::PASS_POINT_LIGHT;       break;
	case TYPE_DIRECTIONAL: pass = RendererMaterial::PASS_DIRECTIONAL_LIGHT; break;
	case TYPE_SPOT:        pass = m_shadowMap != NULL ? RendererMaterial::PASS_SPOT_LIGHT : RendererMaterial::PASS_SPOT_LIGHT_NO_SHADOW;        break;
	default: break;
	}
	RENDERER_ASSERT(pass < RendererMaterial::NUM_PASSES, "Unable to compute the Pass for the Light.");
	return pass;
}

const RendererColor &RendererLight::getColor(void) const
{
	return m_color;
}

void RendererLight::setColor(const RendererColor &color)
{
	m_color = color;
}

float RendererLight::getIntensity(void) const
{
	return m_intensity;
}

void RendererLight::setIntensity(float intensity)
{
	RENDERER_ASSERT(intensity >= 0, "Light intensity is negative.");
	if(intensity >= 0)
	{
		m_intensity = intensity;
	}
}

bool RendererLight::isLocked(void) const
{
	return m_renderer ? true : false;
}

RendererTexture2D *RendererLight::getShadowMap(void) const
{
	return m_shadowMap;
}

void RendererLight::setShadowMap(RendererTexture2D *shadowMap)
{
	m_shadowMap = shadowMap;
}

const RendererProjection &RendererLight::getShadowProjection(void) const
{
	return m_shadowProjection;
}

void RendererLight::setShadowProjection(const RendererProjection &shadowProjection)
{
	m_shadowProjection = shadowProjection;
}

