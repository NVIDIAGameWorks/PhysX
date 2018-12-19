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
