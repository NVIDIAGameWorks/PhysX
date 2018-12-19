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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.


#include "common_buffers.hlsl"

static const float att_c = 1.0f;
static const float att_l = 0.014f;
static const float att_q = 0.0007f;


float CalcAttenuation(float distance)
{
	return 1 / (att_c + att_l * distance + att_q * distance * distance);
};


float4 CalcLight(float4 textureColor, float3 lightDir, float3 viewDir, float3 normal, float3 lightColor, float specPower, float specIntensity, float attenuation)
{
	normal = normalize(normal);

	// diffuse
	float3 dirToLight = normalize(-lightDir);
	float diffuseFactor = max(dot(normal, dirToLight), 0.0);
	float4 diffuse = float4(lightColor, 1) * textureColor * diffuseFactor * attenuation;

	// specular (Blinn-Phong)
	float3 halfwayDir = normalize(dirToLight + viewDir);
	float specFactor = pow(max(dot(viewDir, halfwayDir), 0.0), specPower);
	float4 spec = float4(lightColor, 1) * specFactor * attenuation * specIntensity;
	
	return diffuse + spec;
};

float4 CalcPixelLight(float4 diffuseColor, float4 worldPos, float3 normal)
{
	float3 viewDir = normalize(viewPos - worldPos);

	// ambient
	float4 ambient = float4(ambientColor, 1) * diffuseColor;

	// dir light
	float4 dirLight = CalcLight(diffuseColor, dirLightDir, viewDir, normal, dirLightColor, specularPower, specularIntensity, 1);

	// point light
	float3 pointLightDir = worldPos - pointLightPos;
	float distance = length(pointLightDir);
    float attenuation = CalcAttenuation(distance);
	float4 pointLight = CalcLight(diffuseColor, pointLightDir, viewDir, normal, pointLightColor, specularPower, specularIntensity, attenuation);
	
	return ambient + dirLight + pointLight;
};