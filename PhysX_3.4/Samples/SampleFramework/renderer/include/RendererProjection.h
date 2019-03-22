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
