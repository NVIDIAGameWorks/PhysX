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
