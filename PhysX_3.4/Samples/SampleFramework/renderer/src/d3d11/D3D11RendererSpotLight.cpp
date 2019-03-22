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
