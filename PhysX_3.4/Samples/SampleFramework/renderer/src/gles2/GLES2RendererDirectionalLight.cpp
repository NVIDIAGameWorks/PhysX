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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.


#include "GLES2RendererDirectionalLight.h"
#include "GLES2RendererMaterial.h"
#include "GLES2Renderer.h"
#include "RendererMaterialInstance.h"

#if defined(RENDERER_ENABLE_GLES2)
namespace SampleRenderer
{

GLES2RendererDirectionalLight::GLES2RendererDirectionalLight(const RendererDirectionalLightDesc &desc, 
															GLES2Renderer &renderer, GLfloat (&_lightColor)[3], 
															GLfloat& _lightIntensity, GLfloat (&_lightDirection)[3]) :
	RendererDirectionalLight(desc), m_lightColor(_lightColor), m_lightIntensity(_lightIntensity), m_lightDirection(_lightDirection)
{
}

GLES2RendererDirectionalLight::~GLES2RendererDirectionalLight(void)
{
}

extern GLES2RendererMaterial* g_hackCurrentMat;
extern RendererMaterialInstance* g_hackCurrentMatInstance;

void GLES2RendererDirectionalLight::bind(void) const
{
	m_lightColor[0] = m_color.r/255.0f;
	m_lightColor[1] = m_color.g/255.0f;
	m_lightColor[2] = m_color.b/255.0f;
	m_lightIntensity = m_intensity;
	m_lightDirection[0] = m_direction.x;
	m_lightDirection[1] = m_direction.y;
	m_lightDirection[2] = m_direction.z;
}

}

#endif // #if defined(RENDERER_ENABLE_GLES2)
