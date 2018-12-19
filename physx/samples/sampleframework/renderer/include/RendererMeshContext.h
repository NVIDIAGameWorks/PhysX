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

#ifndef RENDERER_MESH_CONTEXT_H
#define RENDERER_MESH_CONTEXT_H

#include <RendererConfig.h>

namespace SampleRenderer
{

	class Renderer;
	class RendererMesh;
	class RendererMaterial;
	class RendererMaterialInstance;

	class RendererMeshContext
	{
		friend class Renderer;
	public:
		const RendererMesh       *mesh;
		RendererMaterial         *material;
		RendererMaterialInstance *materialInstance;
		const physx::PxMat44	 *transform;
		const physx::PxF32		 *shaderData;

		bool					 negativeScale;

		// TODO: this is kind of hacky, would prefer a more generalized
		//       solution via RendererMatrialInstance.
		const physx::PxMat44	 *boneMatrices;
		PxU32                     numBones;

		enum CullMode
		{
			CLOCKWISE = 0,
			COUNTER_CLOCKWISE,
			NONE
		};

		CullMode				cullMode;
		bool					screenSpace;		//TODO: I am not sure if this is needed!

		enum FillMode
		{
			SOLID,
			LINE,
			POINT,
		};
		FillMode				fillMode;

	public:
		RendererMeshContext(void);
		~RendererMeshContext(void);

		bool isValid(void) const;
	};

} // namespace SampleRenderer

#endif
