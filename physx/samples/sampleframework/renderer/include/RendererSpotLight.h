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
