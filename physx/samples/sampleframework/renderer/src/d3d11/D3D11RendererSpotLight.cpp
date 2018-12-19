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

#if defined(RENDERER_ENABLE_DIRECT3D11)

#include "D3D11RendererSpotLight.h"
#include "D3D11RendererTexture2D.h"

using namespace SampleRenderer;

D3D11RendererSpotLight::D3D11RendererSpotLight(D3D11Renderer& renderer, const RendererSpotLightDesc& desc) :
	RendererSpotLight(desc),
	m_renderer(renderer)
{

}

D3D11RendererSpotLight::~D3D11RendererSpotLight(void)
{

}

void D3D11RendererSpotLight::bind(PxU32 lightIndex) const
{
	if (lightIndex < RENDERER_MAX_LIGHTS)
	{
		D3D11Renderer::D3D11ShaderEnvironment& shaderEnv = m_renderer.getShaderEnvironment();
		convertToD3D11(shaderEnv.lightColor[lightIndex], m_color);
		shaderEnv.lightPosition[lightIndex] = m_position;
		shaderEnv.lightDirection[lightIndex] = m_direction;
		shaderEnv.lightIntensity[lightIndex] = m_intensity;
		shaderEnv.lightInnerRadius[lightIndex] = m_innerRadius;
		shaderEnv.lightOuterRadius[lightIndex] = m_outerRadius;
		shaderEnv.lightInnerCone[lightIndex]   = m_innerCone;
		shaderEnv.lightOuterCone[lightIndex]   = m_outerCone;
		shaderEnv.lightType[lightIndex]        = RendererMaterial::PASS_SPOT_LIGHT;
		shaderEnv.lightShadowMap   = m_shadowMap ? static_cast<D3D11RendererTexture2D*>(m_shadowMap) : NULL;
		buildProjectMatrix(&shaderEnv.lightShadowMatrix.column0.x, m_shadowProjection, m_shadowTransform);
		shaderEnv.bindLight(lightIndex);
	}
}

#endif // #if defined(RENDERER_ENABLE_DIRECT3D11)
