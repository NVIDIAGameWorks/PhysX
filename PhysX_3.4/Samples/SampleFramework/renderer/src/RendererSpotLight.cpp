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
#include <RendererSpotLight.h>
#include <RendererSpotLightDesc.h>

using namespace SampleRenderer;

RendererSpotLight::RendererSpotLight(const RendererSpotLightDesc &desc) :
	RendererLight(desc)
{
	setPosition(desc.position);
	setDirection(desc.direction);
	setRadius(desc.innerRadius, desc.outerRadius);
	setCone(desc.innerCone, desc.outerCone);
}

RendererSpotLight::~RendererSpotLight(void)
{

}

const PxVec3 &RendererSpotLight::getPosition(void) const
{
	return m_position;
}

void RendererSpotLight::setPosition(const PxVec3 &pos)
{
	m_position = pos;
}

const PxVec3 &RendererSpotLight::getDirection(void) const
{
	return m_direction;
}

void RendererSpotLight::setDirection(const PxVec3 &dir)
{
	RENDERER_ASSERT(dir.magnitudeSquared() >= 0.1f, "Trying to give Direction Light invalid Direction value.");
	if(dir.magnitudeSquared() >= 0.1f)
	{
		m_direction = dir;
		m_direction.normalize();
	}
}

PxF32 RendererSpotLight::getInnerRadius(void) const
{
	return m_innerRadius;
}

PxF32 RendererSpotLight::getOuterRadius(void) const
{
	return m_outerRadius;
}

void RendererSpotLight::setRadius(PxF32 innerRadius, PxF32 outerRadius)
{
	RENDERER_ASSERT(innerRadius>=0 && innerRadius<=outerRadius, "Invalid Spot Light radius values.");
	if(innerRadius>=0 && innerRadius<=outerRadius)
	{
		m_innerRadius = innerRadius;
		m_outerRadius = outerRadius;
	}
}

PxF32 RendererSpotLight::getInnerCone(void) const
{
	return m_innerCone;
}

PxF32 RendererSpotLight::getOuterCone(void) const
{
	return m_outerCone;
}

void RendererSpotLight::setCone(PxF32 innerCone, PxF32 outerCone)
{
	RENDERER_ASSERT(innerCone<=1 && innerCone>=outerCone, "Invalid Spot Light cone values.");
	if(innerCone<=1 && innerCone>=outerCone)
	{
		m_innerCone = innerCone;
		m_outerCone = outerCone;
	}
}
