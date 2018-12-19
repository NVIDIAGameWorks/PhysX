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

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include "D3D9RendererSpotLight.h"
#include "D3D9RendererTexture2D.h"

using namespace SampleRenderer;

D3D9RendererSpotLight::D3D9RendererSpotLight(D3D9Renderer &renderer, const RendererSpotLightDesc &desc) :
	RendererSpotLight(desc),
m_renderer(renderer)
{

}

D3D9RendererSpotLight::~D3D9RendererSpotLight(void)
{

}

void D3D9RendererSpotLight::bind(void) const
{
	D3D9Renderer::ShaderEnvironment &shaderEnv = m_renderer.getShaderEnvironment();
	convertToD3D9(shaderEnv.lightColor, m_color);
	shaderEnv.lightIntensity = m_intensity;
	convertToD3D9(shaderEnv.lightPosition,  m_position);
	convertToD3D9(shaderEnv.lightDirection, m_direction);
	shaderEnv.lightInnerRadius = m_innerRadius;
	shaderEnv.lightOuterRadius = m_outerRadius;
	shaderEnv.lightInnerCone   = m_innerCone;
	shaderEnv.lightOuterCone   = m_outerCone;
	shaderEnv.lightShadowMap   = m_shadowMap ? static_cast<D3D9RendererTexture2D*>(m_shadowMap)->m_d3dTexture : 0;
	buildProjectMatrix(shaderEnv.lightShadowMatrix, m_shadowProjection, m_shadowTransform);
}

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
