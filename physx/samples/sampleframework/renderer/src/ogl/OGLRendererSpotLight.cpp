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
#include <RendererConfig.h>

#if defined(RENDERER_ENABLE_OPENGL)

#include "OGLRendererSpotLight.h"

using namespace SampleRenderer;

OGLRendererSpotLight::OGLRendererSpotLight(const RendererSpotLightDesc &desc, OGLRenderer &renderer) 
:	RendererSpotLight(desc)
#if defined(RENDERER_ENABLE_CG)
,	m_cgenv(renderer.getCGEnvironment())
#endif
{

}

OGLRendererSpotLight::~OGLRendererSpotLight(void)
{

}

void OGLRendererSpotLight::bind(void) const
{
#if defined(RENDERER_ENABLE_CG)
	setColorParameter(m_cgenv.lightColor,        m_color);
	cgSetParameter1f( m_cgenv.lightIntensity,    m_intensity);
	cgSetParameter3fv(m_cgenv.lightDirection,   &m_direction.x);
	cgSetParameter3fv(m_cgenv.lightPosition,    &m_position.x);
	cgSetParameter1f( m_cgenv.lightInnerRadius,  m_innerRadius);
	cgSetParameter1f( m_cgenv.lightOuterRadius,  m_outerRadius);
	cgSetParameter1f( m_cgenv.lightInnerCone,    m_innerCone);
	cgSetParameter1f( m_cgenv.lightOuterCone,    m_outerCone);
#endif
}

#endif
