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

#ifndef RENDERER_PARTICLE_SYSTEM_SHAPE_H
#define RENDERER_PARTICLE_SYSTEM_SHAPE_H

#include <RendererShape.h>
#include "gpu/PxGpu.h"

// fwd to avoid including cuda.h
#if defined(__x86_64) || defined(AMD64) || defined(_M_AMD64)
typedef unsigned long long CUdeviceptr;
#else
typedef unsigned int CUdeviceptr;
#endif

namespace SampleRenderer
{

	class RendererVertexBuffer;
	class RendererInstanceBuffer;
	class RendererIndexBuffer;

	class RendererParticleSystemShape : public RendererShape
	{
		enum VertexBufferEnum
		{
			STATIC_VB,
			DYNAMIC_POS_VB,
			DYNAMIC_COL_VB,
			NUM_VERTEX_BUFFERS
		};

	public:
								RendererParticleSystemShape(Renderer &renderer, 
									physx::PxU32 num_vertices, 
									bool _mInstanced,
									bool _mFading,
									PxReal fadingPeriod = 1.0f,
									PxReal debriScaleFactor = 1.0f,
									PxCudaContextManager* ctxMgr=NULL
									);
		virtual					~RendererParticleSystemShape(void);

		bool					updateBillboard(PxU32 validParticleRange, 
										CUdeviceptr positions, 
										CUdeviceptr validParticleBitmap,
										CUdeviceptr lifetimes,
										PxU32 numParticles);

		bool					updateInstanced(PxU32 validParticleRange, 
										CUdeviceptr positions, 
										CUdeviceptr validParticleBitmap,
										CUdeviceptr orientations,
										PxU32 numParticles);

		void					updateBillboard(PxU32 validParticleRange, 
										 const PxVec3* positions, 
										 const PxU32* validParticleBitmap,
										 const PxReal* lifetime = NULL);

		void					updateInstanced(PxU32 validParticleRange, 
										const PxVec3* positions, 
										const PxU32* validParticleBitmap,
										const PxMat33* orientation);

		bool					isInteropEnabled() { return mCtxMgr != NULL; }

	private:
		bool					mInstanced;
		bool					mFading;
		PxReal					mFadingPeriod;
		PxU32					mMaxParticles;
		RendererVertexBuffer*	mVertexBuffer[NUM_VERTEX_BUFFERS];
		RendererInstanceBuffer*	mInstanceBuffer;
		RendererIndexBuffer*	mIndexBuffer;

		PxCudaContextManager*	mCtxMgr;

		void					initializeVertexBuffer(PxU32 color);
		void					initializeBuffersAsSimpleConvex(PxU32 color, PxReal scaleFactor);
		void					initializeInstanceBuffer();
	};

} // namespace SampleRenderer

#endif
