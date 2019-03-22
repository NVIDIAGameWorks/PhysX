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

#ifndef RENDERER_SPOT_LIGHT_H
#define RENDERER_SPOT_LIGHT_H

#include <RendererLight.h>

namespace SampleRenderer
{

	class RendererSpotLightDesc;

	class RendererSpotLight : public RendererLight
	{
	protected:
		RendererSpotLight(const RendererSpotLightDesc &desc);
		virtual ~RendererSpotLight(void);

	public:
		const PxVec3 &getPosition(void) const;
		void          setPosition(const PxVec3 &pos);

		const PxVec3 &getDirection(void) const;
		void          setDirection(const PxVec3 &dir);

		PxF32         getInnerRadius(void) const;
		PxF32         getOuterRadius(void) const;
		void          setRadius(PxF32 innerRadius, PxF32 outerRadius);

		PxF32         getInnerCone(void) const;
		PxF32         getOuterCone(void) const;
		void          setCone(PxF32 innerCone, PxF32 outerCone);

	protected:
		PxVec3        m_position;
		PxVec3        m_direction;
		PxF32         m_innerRadius;
		PxF32         m_outerRadius;
		PxF32         m_innerCone;
		PxF32         m_outerCone;
	};

} // namespace SampleRenderer

#endif
