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

