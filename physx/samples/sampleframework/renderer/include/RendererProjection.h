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

#ifndef RENDERER_PROJECTION_H
#define RENDERER_PROJECTION_H

#include <RendererConfig.h>

namespace SampleRenderer
{

	class RendererProjection
	{
		public:
			RendererProjection(float fov, float aspectRatio, float nearPlane, float farPlane);
			RendererProjection(float left, float right, float bottom, float top, float near, float far);
			RendererProjection(const PxMat44 mat);
			
			void getColumnMajor44(float *f) const;
			void getRowMajor44(float *f) const;
			operator PxMat44 () const { return *reinterpret_cast<const PxMat44*>( m_matrix ); }
			PxMat44& getPxMat44() { return reinterpret_cast<PxMat44&>(*m_matrix ); }
			const PxMat44& getPxMat44() const { return reinterpret_cast<const PxMat44&>(*m_matrix ); }
			
		private:
			float m_matrix[16];
	};

	void   buildProjectMatrix(float *dst, const RendererProjection &proj, const physx::PxTransform &view);
	void   buildUnprojectMatrix(float *dst, const RendererProjection &proj, const physx::PxTransform &view);
	PxVec3 unproject(const RendererProjection &proj, const physx::PxTransform &view, PxF32 x, PxF32 y, PxF32 z = 0);
	PxVec3 project(  const RendererProjection &proj, const physx::PxTransform &view, const PxVec3& pos);

} // namespace SampleRenderer

#endif
