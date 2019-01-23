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

#ifndef RENDERER_COLOR_H
#define RENDERER_COLOR_H

#include <RendererConfig.h>

namespace SampleRenderer
{

	class RendererColor
	{
		public:

			// use the API color format for OpenGL platforms
			PxU8 b, g, r, a;

		public:
			RendererColor(void);
			RendererColor(PxU8 r, PxU8 g, PxU8 b, PxU8 a = 255);

			// conversion constructor, format must be 0xAARRGGBB (but alpha is ignored)
			RendererColor(PxU32 rgba);
			void swizzleRB(void);
	};

	PX_INLINE RendererColor lerp( const RendererColor& start, const RendererColor& end, float s )
	{
		return RendererColor(
			start.r + PxU8(( end.r - start.r ) * s),
			start.g + PxU8(( end.g - start.g ) * s),
			start.b + PxU8(( end.b - start.b ) * s),
			start.a + PxU8(( end.a - start.a ) * s));
	}

} // namespace SampleRenderer

#endif
