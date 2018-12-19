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
