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
#include <RendererMaterialDesc.h>

using namespace SampleRenderer;

RendererMaterialDesc::RendererMaterialDesc(void)
{
	type               = RendererMaterial::TYPE_UNLIT;
	alphaTestFunc      = RendererMaterial::ALPHA_TEST_ALWAYS;
	alphaTestRef       = 0;
	blending           = false;
	instanced		   = false;
	srcBlendFunc       = RendererMaterial::BLEND_ZERO;
	dstBlendFunc       = RendererMaterial::BLEND_ZERO;
	geometryShaderPath = 0;
	hullShaderPath     = 0;
	domainShaderPath   = 0;
	vertexShaderPath   = 0;
	fragmentShaderPath = 0;
}

bool RendererMaterialDesc::isValid(void) const
{
	bool ok = true;
	if(type >= RendererMaterial::NUM_TYPES) ok = false;
	if(alphaTestRef < 0 || alphaTestRef > 1) ok = false;
	// Note: Lighting and blending may not be properly supported, but that 
	//    shouldn't crash the system, so this check is disabled for now
	// if(blending && type != RendererMaterial::TYPE_UNLIT) ok = false;
	if(geometryShaderPath || domainShaderPath || fragmentShaderPath)
	{
		// Note: These should be completely optional! Disabling for now.
		//RENDERER_ASSERT(0, "Not implemented!");
		//ok = false;
	}
	if(!vertexShaderPath)   ok = false;
	if(!fragmentShaderPath) ok = false;
	return ok;
}
