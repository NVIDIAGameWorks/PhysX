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

#ifndef RENDERER_CLOTH_SHAPE_H
#define RENDERER_CLOTH_SHAPE_H

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
	class RendererIndexBuffer;

	class RendererClothShape : public RendererShape
	{
		enum VertexBufferEnum
		{
			STATIC_VB,
			DYNAMIC_VB,
			NUM_VERTEX_BUFFERS
		};
	public:
		RendererClothShape(Renderer& renderer, 
			const PxVec3* verts, PxU32 numVerts,
			const PxVec3* normals,
			const PxReal* uvs,
			const PxU16* faces, PxU32 numFaces, bool flipWinding=false,
			PxCudaContextManager* ctxMgr=NULL);
        
		// must acquire the cuda context and map buffers before calling
		bool update(CUdeviceptr srcVerts, PxU32 numVerts);
        void update(const PxVec3* verts, PxU32 numVerts, const PxVec3* normals);

		// helper functions to map the buffers for multiple shapes 
		// at once because interop mapping has a large per-call overhead
		// thread must acquire the cuda context before calling
		static void mapShapes(RendererClothShape** shapes, PxU32 n);
		static void unmapShapes(RendererClothShape** shapes, PxU32 n);

		virtual ~RendererClothShape();

		bool isInteropEnabled() const { return m_ctxMgr != NULL; }

	private:
		RendererVertexBuffer*	m_vertexBuffer[NUM_VERTEX_BUFFERS];
		RendererIndexBuffer*	m_indexBuffer;

		PxU32 m_numFaces;

		PxCudaContextManager* m_ctxMgr;
	};

} // namespace SampleRenderer

#endif
