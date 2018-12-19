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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.
#include <PsUtilities.h>

#include <RendererParticleSystemShape.h>

#include <Renderer.h>

#include <RendererMemoryMacros.h>
#include <RendererIndexBuffer.h>
#include <RendererIndexBufferDesc.h>
#include <RendererVertexBuffer.h>
#include <RendererVertexBufferDesc.h>
#include <RendererInstanceBuffer.h>
#include <RendererInstanceBufferDesc.h>
#include <RendererMesh.h>
#include <RendererMeshDesc.h>
#include "PsUtilities.h"
#include "PsBitUtils.h"

using namespace SampleRenderer;
using namespace physx::shdfnd;

PxReal convexVertices[] = { 0.0f, 1.0f, 0.0f,	// 1 up face
							0.5f, 0.0f, 0.5f,
							0.5f, 0.0f, -0.5f,
							0.0f, 1.0f, 0.0f,	// 2 up face
							0.5f, 0.0f, -0.5f,
							-0.5f, 0.0f, -0.5f,
							0.0f, 1.0f, 0.0f,	// 3 up face
							-0.5f, 0.0f, -0.5f,
							-0.5f, 0.0f, 0.5f,
							0.0f, 1.0f, 0.0f,	// 4 up face
							-0.5f, 0.0f, 0.5f,
							0.5f, 0.0f, 0.5f,
							0.0f, -1.0f, 0.0f,	// 1 down face
							0.5f, 0.0f, 0.5f,
							0.5f, 0.0f, -0.5f,
							0.0f, -1.0f, 0.0f,	// 2 down face
							0.5f, 0.0f, -0.5f,
							-0.5f, 0.0f, -0.5f,
							0.0f, -1.0f, 0.0f,	// 3 down face
							-0.5f, 0.0f, -0.5f,
							-0.5f, 0.0f, 0.5f,
							0.0f, -1.0f, 0.0f,	// 4 down face
							-0.5f, 0.0f, 0.5f,
							0.5f, 0.0f, 0.5f };
PxReal convexTexcoords[] = { 0.0f, 0.0f,		// each face
							0.0f, 1.0f,
							1.0f, 0.0f };
PxReal convexNormals[] = { 1.0f, -0.5f, 0.0f,	// 1 up face
							1.0f, -0.5f, 0.0f,
							1.0f, -0.5f, 0.0f,
							0.0f, -0.5f, -1.0f,	// 2 up face
							0.0f, -0.5f, -1.0f,
							0.0f, -0.5f, -1.0f,
							-1.0f, -0.5f, 0.0f,	// 3 up face
							-1.0f, -0.5f, 0.0f,
							-1.0f, -0.5f, 0.0f,
							0.0f, -0.5f, 1.0f,	// 4 up face
							0.0f, -0.5f, 1.0f,
							0.0f, -0.5f, 1.0f,
							1.0f, 0.5f, 0.0f,	// 1 down face
							1.0f, 0.5f, 0.0f,
							1.0f, 0.5f, 0.0f,
							0.0f, 0.5f, -1.0f,	// 2 down face
							0.0f, 0.5f, -1.0f,
							0.0f, 0.5f, -1.0f,
							-1.0f, -0.5f, 0.0f,	// 3 down face
							-1.0f, -0.5f, 0.0f,
							-1.0f, -0.5f, 0.0f,
							0.0f, -0.5f, 1.0f,	// 4 down face
							0.0f, -0.5f, 1.0f,
							0.0f, -0.5f, 1.0f };
PxU32 convexIndices[] = { 0, 1, 2, 
						3, 4, 5, 
						6, 7, 8,
						9, 10, 11,
						12, 13, 14,
						15, 16, 17,
						18, 19, 20,
						21, 22, 23 };

const PxU32 convexVerticesCount = PX_ARRAY_SIZE(convexVertices) / 3 ;
const PxU32 convexIndicesCount = PX_ARRAY_SIZE(convexIndices);
//const PxU32 convexNormalsCount = PX_ARRAY_SIZE(convexNormals) / 3;
//const PxU32 convexTexcoordsCount = PX_ARRAY_SIZE(convexTexcoords) / 2;

#if defined(RENDERER_ENABLE_CUDA_INTEROP)
#include <task/PxTask.h>
#include <cuda.h>

// pre-compiled fatbin
#if defined(_M_X64)
#include "cuda/RenderParticles_x64.cuh"
#else
#include "cuda/RenderParticles_x86.cuh"
#endif

namespace
{
	CUmodule gParticlesCuRenderModule;
	CUfunction gParticlesCuUpdateInstanceFunc;
	CUfunction gParticlesCuUpdateBillboardFunc;

	void checkSuccess(CUresult r)
	{
		PX_ASSERT(r == CUDA_SUCCESS);
	}
}

#endif // RENDERER_ENABLE_CUDA_INTEROP

RendererParticleSystemShape::RendererParticleSystemShape(Renderer &renderer, 
	PxU32 _mMaxParticles, bool _mInstanced, bool _mFading,
	PxReal fadingPeriod, PxReal debriScaleFactor, PxCudaContextManager* ctxMgr) : 
											RendererShape(renderer), 
											mInstanced(_mInstanced),
											mFading(_mFading),
											mFadingPeriod(fadingPeriod),
											mMaxParticles(_mMaxParticles),
											mInstanceBuffer(NULL),
											mIndexBuffer(NULL),
											mCtxMgr(ctxMgr)
{
	for(PxU32 i = 0; i < NUM_VERTEX_BUFFERS; i++ )
	{
		mVertexBuffer[i] = NULL;
	}
	RendererVertexBufferDesc vbdesc;

#if defined(RENDERER_ENABLE_CUDA_INTEROP)
	if (ctxMgr)
	{
		vbdesc.registerInCUDA = true;
		vbdesc.interopContext = ctxMgr;

		ctxMgr->acquireContext();

		// find module containing our kernel
		CUresult success;

		success = cuModuleLoadDataEx(&gParticlesCuRenderModule, updateParticleVertexBuffer, 0, NULL, NULL);
		RENDERER_ASSERT(success == CUDA_SUCCESS, "Could not load cloth render CUDA module.");

		success = cuModuleGetFunction(&gParticlesCuUpdateInstanceFunc, gParticlesCuRenderModule, "updateInstancedVB");
		RENDERER_ASSERT(success == CUDA_SUCCESS, "Could not find cloth render CUDA function.");

		success = cuModuleGetFunction(&gParticlesCuUpdateBillboardFunc, gParticlesCuRenderModule, "updateBillboardVB");
		RENDERER_ASSERT(success == CUDA_SUCCESS, "Could not find cloth render CUDA function.");

		ctxMgr->releaseContext();
	}
#endif //RENDERER_ENABLE_CUDA_INTEROP

	if(mInstanced) 
	{
		vbdesc.hint = RendererVertexBuffer::HINT_STATIC;
		vbdesc.maxVertices = convexVerticesCount;
	} 
	else 
	{
#if defined(RENDERER_TABLET)
		vbdesc.hint = RendererVertexBuffer::HINT_STATIC;
#else
		vbdesc.hint = RendererVertexBuffer::HINT_DYNAMIC;
#endif
		vbdesc.maxVertices = mMaxParticles;
	}
	{
		RendererVertexBufferDesc vbdesc_l = vbdesc;
		vbdesc_l.semanticFormats[RendererVertexBuffer::SEMANTIC_POSITION] = RendererVertexBuffer::FORMAT_FLOAT3;
		mVertexBuffer[DYNAMIC_POS_VB] = m_renderer.createVertexBuffer(vbdesc_l);
		RENDERER_ASSERT(mVertexBuffer[DYNAMIC_POS_VB], "Failed to create Vertex Buffer.");
	}
	{
		RendererVertexBufferDesc vbdesc_l = vbdesc;
		vbdesc_l.semanticFormats[RendererVertexBuffer::SEMANTIC_COLOR] = RendererVertexBuffer::FORMAT_COLOR_NATIVE;
		mVertexBuffer[DYNAMIC_COL_VB] = m_renderer.createVertexBuffer(vbdesc_l);
		RENDERER_ASSERT(mVertexBuffer[DYNAMIC_COL_VB], "Failed to create Vertex Buffer.");
	}
	vbdesc.hint = RendererVertexBuffer::HINT_STATIC;
	vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_NORMAL] = RendererVertexBuffer::FORMAT_FLOAT3;
	vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_TEXCOORD0] = RendererVertexBuffer::FORMAT_FLOAT2;
	mVertexBuffer[STATIC_VB] = m_renderer.createVertexBuffer(vbdesc);
	RENDERER_ASSERT(mVertexBuffer[STATIC_VB], "Failed to create Vertex Buffer.");
	// if enabled mInstanced meshes -> create instance buffer
	if(mInstanced) 
	{
		// create index buffer
		RendererIndexBufferDesc inbdesc;
		inbdesc.hint = RendererIndexBuffer::HINT_STATIC;
		inbdesc.format = RendererIndexBuffer::FORMAT_UINT32;
		inbdesc.maxIndices = convexIndicesCount;
		mIndexBuffer = m_renderer.createIndexBuffer(inbdesc);
		RENDERER_ASSERT(mIndexBuffer, "Failed to create Instance Buffer.");
		// create instance buffer
		RendererInstanceBufferDesc ibdesc;
		ibdesc.hint = RendererInstanceBuffer::HINT_DYNAMIC;
		ibdesc.semanticFormats[RendererInstanceBuffer::SEMANTIC_POSITION] = RendererInstanceBuffer::FORMAT_FLOAT3;
		ibdesc.semanticFormats[RendererInstanceBuffer::SEMANTIC_NORMALX] = RendererInstanceBuffer::FORMAT_FLOAT3;
		ibdesc.semanticFormats[RendererInstanceBuffer::SEMANTIC_NORMALY] = RendererInstanceBuffer::FORMAT_FLOAT3;
		ibdesc.semanticFormats[RendererInstanceBuffer::SEMANTIC_NORMALZ] = RendererInstanceBuffer::FORMAT_FLOAT3;
		ibdesc.semanticFormats[RendererInstanceBuffer::SEMANTIC_VELOCITY_LIFE] = RendererInstanceBuffer::FORMAT_FLOAT4;
		ibdesc.maxInstances = mMaxParticles;

		if (ctxMgr)
		{
			ibdesc.registerInCUDA = true;
			ibdesc.interopContext = ctxMgr;
		}

		mInstanceBuffer = m_renderer.createInstanceBuffer(ibdesc);
		RENDERER_ASSERT(mInstanceBuffer, "Failed to create Instance Buffer.");		
	}
	PxU32 color = renderer.convertColor(RendererColor(255, 255, 255, 255));
	PxU32 numInstances = 0;
	RendererMesh::Primitive primitive = RendererMesh::PRIMITIVE_POINT_SPRITES;
	if(mInstanced) 
	{
		numInstances = mMaxParticles;
		initializeBuffersAsSimpleConvex(color, debriScaleFactor);
		initializeInstanceBuffer();
		primitive = RendererMesh::PRIMITIVE_TRIANGLES;
	} 
	else 
	{
		initializeVertexBuffer(color);			
	}
	if(mVertexBuffer[DYNAMIC_POS_VB] && mVertexBuffer[DYNAMIC_COL_VB] && mVertexBuffer[STATIC_VB])
	{
		RendererMeshDesc meshdesc;
		meshdesc.primitives			= primitive;
		meshdesc.vertexBuffers		= mVertexBuffer;
		meshdesc.numVertexBuffers	= NUM_VERTEX_BUFFERS;
		meshdesc.firstVertex		= 0;
		meshdesc.numVertices		= mVertexBuffer[STATIC_VB]->getMaxVertices();
		meshdesc.indexBuffer		= mIndexBuffer;
		meshdesc.firstIndex			= 0;
		if(mIndexBuffer) 
		{
			meshdesc.numIndices		= mIndexBuffer->getMaxIndices();
		} 
		else 
		{
			meshdesc.numIndices		= 0;
		}
		meshdesc.instanceBuffer		= mInstanceBuffer;
		meshdesc.firstInstance		= 0;
		meshdesc.numInstances		= numInstances;
		m_mesh = m_renderer.createMesh(meshdesc);
		RENDERER_ASSERT(m_mesh, "Failed to create Mesh.");
	}
}

RendererParticleSystemShape::~RendererParticleSystemShape(void)
{
	SAFE_RELEASE(mInstanceBuffer);
	SAFE_RELEASE(mIndexBuffer);
	for(PxU32 i = 0; i < NUM_VERTEX_BUFFERS; i++ )
	{
		SAFE_RELEASE(mVertexBuffer[i]);
	}
	SAFE_RELEASE(m_mesh);
}

void RendererParticleSystemShape::initializeBuffersAsSimpleConvex(PxU32 color, PxReal scaleFactor) 
{
	// initialize vertex buffer
	PxU32 positionStride = 0, colorStride = 0, texcoordStride = 0, normalStride = 0;
	PxU8* locked_normals = static_cast<PxU8*>(mVertexBuffer[STATIC_VB]->lockSemantic(RendererVertexBuffer::SEMANTIC_NORMAL, normalStride));
	PxU8* locked_texcoords = static_cast<PxU8*>(mVertexBuffer[STATIC_VB]->lockSemantic(RendererVertexBuffer::SEMANTIC_TEXCOORD0, texcoordStride));
	PxU8* locked_positions = static_cast<PxU8*>(mVertexBuffer[DYNAMIC_POS_VB]->lockSemantic(RendererVertexBuffer::SEMANTIC_POSITION, positionStride));
	PxU8* locked_colors = static_cast<PxU8*>(mVertexBuffer[DYNAMIC_COL_VB]->lockSemantic(RendererVertexBuffer::SEMANTIC_COLOR, colorStride));
	PxU32 maxVertices = mVertexBuffer[STATIC_VB]->getMaxVertices();
	for(PxU32 i = 0; i < maxVertices; ++i, 
									locked_positions += positionStride, 
									locked_colors += colorStride,
									locked_texcoords += texcoordStride,
									locked_normals += normalStride) 
	{
		PxReal localConvexVertices[] = { convexVertices[3 * i + 0] * scaleFactor, 
										convexVertices[3 * i + 1] * scaleFactor, 
										convexVertices[3 * i + 2] * scaleFactor };
		PxReal localConvexNormals[] = { convexNormals[3 * i + 0] / 2.25f, 
										convexNormals[3 * i + 1] / 2.25f, 
										convexNormals[3 * i + 2] / 2.25f };		
		memcpy(locked_positions, localConvexVertices, sizeof(PxReal) * 3);
		memcpy(locked_normals, localConvexNormals, sizeof(PxReal) * 3);
		memcpy(locked_texcoords, convexTexcoords + 2 * (i % 3), sizeof(PxReal) * 2);
		memset(locked_colors, color, sizeof(PxU32));				
	}
	mVertexBuffer[STATIC_VB]->unlockSemantic(RendererVertexBuffer::SEMANTIC_TEXCOORD0);
	mVertexBuffer[STATIC_VB]->unlockSemantic(RendererVertexBuffer::SEMANTIC_NORMAL);
	mVertexBuffer[DYNAMIC_COL_VB]->unlockSemantic(RendererVertexBuffer::SEMANTIC_COLOR);
	mVertexBuffer[DYNAMIC_POS_VB]->unlockSemantic(RendererVertexBuffer::SEMANTIC_POSITION);
	// initialize index buffer
	PxU32* locked_indices = static_cast<PxU32*>(mIndexBuffer->lock());
	memcpy(locked_indices, convexIndices, mIndexBuffer->getMaxIndices() * sizeof(PxU32));
	mIndexBuffer->unlock();
}

void RendererParticleSystemShape::initializeVertexBuffer(PxU32 color) 
{
	PxU32 positionStride = 0, colorStride = 0;
	PxU8* locked_positions = static_cast<PxU8*>(mVertexBuffer[DYNAMIC_POS_VB]->lockSemantic(RendererVertexBuffer::SEMANTIC_POSITION, positionStride));
	PxU8* locked_colors = static_cast<PxU8*>(mVertexBuffer[DYNAMIC_COL_VB]->lockSemantic(RendererVertexBuffer::SEMANTIC_COLOR, colorStride));
	for(PxU32 i = 0; i < mMaxParticles; ++i, 
										locked_positions += positionStride, 
										locked_colors += colorStride) 
	{
		memset(locked_positions, 0, sizeof(PxReal) * 3);
		memset(locked_colors, color, sizeof(PxU32));				
	}
	mVertexBuffer[DYNAMIC_COL_VB]->unlockSemantic(RendererVertexBuffer::SEMANTIC_COLOR);
	mVertexBuffer[DYNAMIC_POS_VB]->unlockSemantic(RendererVertexBuffer::SEMANTIC_POSITION);
}

void RendererParticleSystemShape::initializeInstanceBuffer() 
{
	PxU32 positionStride = 0, 
			normalXStride = 0,
			normalYStride = 0,
			normalZStride = 0,
			velocityLifeStride = 0;
	PxU8* locked_positions = static_cast<PxU8*>(mInstanceBuffer->lockSemantic(
						RendererInstanceBuffer::SEMANTIC_POSITION, positionStride));
	PxU8* locked_normalsx = static_cast<PxU8*>(mInstanceBuffer->lockSemantic(
						RendererInstanceBuffer::SEMANTIC_NORMALX, normalXStride));
	PxU8* locked_normalsy = static_cast<PxU8*>(mInstanceBuffer->lockSemantic(
						RendererInstanceBuffer::SEMANTIC_NORMALY, normalYStride));
	PxU8* locked_normalsz = static_cast<PxU8*>(mInstanceBuffer->lockSemantic(
						RendererInstanceBuffer::SEMANTIC_NORMALZ, normalZStride));
	PxU8* locked_velocitylife = static_cast<PxU8*>(mInstanceBuffer->lockSemantic(
						RendererInstanceBuffer::SEMANTIC_VELOCITY_LIFE, velocityLifeStride));
	// fill buffer here
	PxReal normalx[] = { 1.0f, 0.0f, 0.0f };
	PxReal normaly[] = { 0.0f, 1.0f, 0.0f };
	PxReal normalz[] = { 0.0f, 0.0f, 1.0f };
	for(PxU32 i = 0; i < mMaxParticles; ++i, 
										locked_positions += positionStride,
										locked_normalsx += normalXStride,
										locked_normalsy += normalYStride,
										locked_normalsz += normalZStride,
										locked_velocitylife += velocityLifeStride) 
	{
		memset(locked_positions, 0, sizeof(PxReal) * 3);			
		memcpy(locked_normalsx, normalx, sizeof(physx::PxVec3));			
		memcpy(locked_normalsy, normaly, sizeof(physx::PxVec3));			
		memcpy(locked_normalsz, normalz, sizeof(physx::PxVec3));			
		memset(locked_velocitylife, 0, sizeof(PxReal) * 4);	
	}
	mInstanceBuffer->unlockSemantic(RendererInstanceBuffer::SEMANTIC_VELOCITY_LIFE);
	mInstanceBuffer->unlockSemantic(RendererInstanceBuffer::SEMANTIC_NORMALZ);
	mInstanceBuffer->unlockSemantic(RendererInstanceBuffer::SEMANTIC_NORMALY);
	mInstanceBuffer->unlockSemantic(RendererInstanceBuffer::SEMANTIC_NORMALX);
	mInstanceBuffer->unlockSemantic(RendererInstanceBuffer::SEMANTIC_POSITION);
}

namespace
{

#if defined(RENDERER_ENABLE_CUDA_INTEROP)

	void cuuAlignUp(int* offset, int alignment)
	{
		*offset = ((*offset) + (alignment) - 1) & ~((alignment) - 1);
	}

	template <typename T, typename T2> CUresult
		cuuParamSet(CUfunction hf, int* offset, const T2& var)
	{
		cuuAlignUp(offset, __alignof(T));
		CUresult err = cuParamSetv(hf, *offset, (void*)&var, sizeof(T));
		*offset += sizeof(T);
		return err;
	}

#endif // RENDERER_ENABLE_CUDA_INTEROP

} // anonymous namespace


bool RendererParticleSystemShape::updateInstanced(PxU32 validParticleRange, 
										 CUdeviceptr positions, 
										 CUdeviceptr validParticleBitmap,
										 CUdeviceptr orientations,
										 PxU32 numParticles) 
{
#if defined(RENDERER_ENABLE_CUDA_INTEROP)

	CUgraphicsResource res;
	if (mInstanceBuffer->getInteropResourceHandle(res))
	{
		mCtxMgr->acquireContext();

		checkSuccess(cuGraphicsMapResources(1, &res, 0));
		
		CUdeviceptr destBuffer;
		size_t destSize;
		cuGraphicsResourceGetMappedPointer(&destBuffer, &destSize, res);

		// if either buffers could not be mapped abort
		if (!destBuffer)
		{
			mCtxMgr->releaseContext();
			return false;
		}

		int offset = 0;
		checkSuccess(cuuParamSet<CUdeviceptr>(gParticlesCuUpdateInstanceFunc, &offset, destBuffer + mInstanceBuffer->getOffsetForSemantic(RendererInstanceBuffer::SEMANTIC_POSITION)));
		checkSuccess(cuuParamSet<CUdeviceptr>(gParticlesCuUpdateInstanceFunc, &offset, destBuffer + mInstanceBuffer->getOffsetForSemantic(RendererInstanceBuffer::SEMANTIC_NORMALX)));
		checkSuccess(cuuParamSet<CUdeviceptr>(gParticlesCuUpdateInstanceFunc, &offset, destBuffer + mInstanceBuffer->getOffsetForSemantic(RendererInstanceBuffer::SEMANTIC_NORMALY)));
		checkSuccess(cuuParamSet<CUdeviceptr>(gParticlesCuUpdateInstanceFunc, &offset, destBuffer + mInstanceBuffer->getOffsetForSemantic(RendererInstanceBuffer::SEMANTIC_NORMALZ)));
		checkSuccess(cuuParamSet<PxU32>(gParticlesCuUpdateInstanceFunc, &offset, mInstanceBuffer->getStride()));
		checkSuccess(cuuParamSet<CUdeviceptr>(gParticlesCuUpdateInstanceFunc, &offset, positions));
		checkSuccess(cuuParamSet<CUdeviceptr>(gParticlesCuUpdateInstanceFunc, &offset, orientations));
		checkSuccess(cuuParamSet<CUdeviceptr>(gParticlesCuUpdateInstanceFunc, &offset, validParticleBitmap));
		checkSuccess(cuuParamSet<PxU32>(gParticlesCuUpdateInstanceFunc, &offset, validParticleRange));
		checkSuccess(cuParamSetSize(gParticlesCuUpdateInstanceFunc, offset));

		const PxU32 blockSize = 512;
		const PxU32 numBlocks = 1;

		checkSuccess(cuFuncSetBlockShape(gParticlesCuUpdateInstanceFunc, blockSize, 1, 1));
		checkSuccess(cuLaunchGridAsync(gParticlesCuUpdateInstanceFunc, numBlocks, 1, 0));

		checkSuccess(cuGraphicsUnmapResources(1, &res, 0));
		
		mCtxMgr->releaseContext();
	}	

	m_mesh->setInstanceBufferRange(0, numParticles);

#endif // RENDERER_ENABLE_CUDA_INTEROP

	return true;

}

bool RendererParticleSystemShape::updateBillboard(PxU32 validParticleRange, 
							   CUdeviceptr positions, 
							   CUdeviceptr validParticleBitmap,
							   CUdeviceptr lifetimes,
							   PxU32 numParticles)
{

#if defined(RENDERER_ENABLE_CUDA_INTEROP)

	CUgraphicsResource res[2];
	mVertexBuffer[DYNAMIC_POS_VB]->getInteropResourceHandle(res[0]);
	mVertexBuffer[DYNAMIC_COL_VB]->getInteropResourceHandle(res[1]);

	if (res[0] && res[1])
	{
		mCtxMgr->acquireContext();

		checkSuccess(cuGraphicsMapResources(2, res, 0));

		CUdeviceptr posBuffer, colBuffer;
		size_t destSize;
		checkSuccess(cuGraphicsResourceGetMappedPointer(&posBuffer, &destSize, res[0]));
		checkSuccess(cuGraphicsResourceGetMappedPointer(&colBuffer, &destSize, res[1]));

		// if either buffers could not be mapped abort
		if (!posBuffer || !colBuffer)
		{
			mCtxMgr->releaseContext();
			return false;
		}

		int offset = 0;
		checkSuccess(cuuParamSet<CUdeviceptr>(gParticlesCuUpdateBillboardFunc, &offset, posBuffer));
		checkSuccess(cuuParamSet<CUdeviceptr>(gParticlesCuUpdateBillboardFunc, &offset, colBuffer+3));
		checkSuccess(cuuParamSet<PxU32>(gParticlesCuUpdateBillboardFunc, &offset, mVertexBuffer[DYNAMIC_POS_VB]->getStride()));
		checkSuccess(cuuParamSet<PxF32>(gParticlesCuUpdateBillboardFunc, &offset, mFadingPeriod));
		checkSuccess(cuuParamSet<CUdeviceptr>(gParticlesCuUpdateBillboardFunc, &offset, positions));
		checkSuccess(cuuParamSet<CUdeviceptr>(gParticlesCuUpdateBillboardFunc, &offset, lifetimes));
		checkSuccess(cuuParamSet<CUdeviceptr>(gParticlesCuUpdateBillboardFunc, &offset, validParticleBitmap));
		checkSuccess(cuuParamSet<PxU32>(gParticlesCuUpdateBillboardFunc, &offset, validParticleRange));
		checkSuccess(cuParamSetSize(gParticlesCuUpdateBillboardFunc, offset));

		const PxU32 blockSize = 512;
		const PxU32 numBlocks = 1;

		checkSuccess(cuFuncSetBlockShape(gParticlesCuUpdateBillboardFunc, blockSize, 1, 1));
		checkSuccess(cuLaunchGridAsync(gParticlesCuUpdateBillboardFunc, numBlocks, 1, 0));

		checkSuccess(cuGraphicsUnmapResources(2, res, 0));

		mCtxMgr->releaseContext();
	}	

	m_mesh->setVertexBufferRange(0, numParticles);

#endif // RENDERER_ENABLE_CUDA_INTEROP

	return true;

}


void RendererParticleSystemShape::updateInstanced(PxU32 validParticleRange, 
										 const PxVec3* positions, 
										 const PxU32* validParticleBitmap,
										 const PxMat33* orientation) 
{
	PxU32 positionStride = 0, 
			normalXStride = 0,
			normalYStride = 0,
			normalZStride = 0;
	PxU8 *locked_positions = NULL, 
			*locked_normalsx = NULL,
			*locked_normalsy = NULL,
			*locked_normalsz = NULL;
	locked_positions = static_cast<PxU8*>(mInstanceBuffer->lockSemantic(
							RendererInstanceBuffer::SEMANTIC_POSITION, positionStride));
	locked_normalsx = static_cast<PxU8*>(mInstanceBuffer->lockSemantic(
						RendererInstanceBuffer::SEMANTIC_NORMALX, normalXStride));
	locked_normalsy = static_cast<PxU8*>(mInstanceBuffer->lockSemantic(
						RendererInstanceBuffer::SEMANTIC_NORMALY, normalYStride));
	locked_normalsz = static_cast<PxU8*>(mInstanceBuffer->lockSemantic(
						RendererInstanceBuffer::SEMANTIC_NORMALZ, normalZStride));

	// update vertex or instance buffer here
	PxU32 numParticles = 0;
	if(validParticleRange > 0)
	{
		for (PxU32 w = 0; w <= (validParticleRange-1) >> 5; w++)
		{
			for (PxU32 b = validParticleBitmap[w]; b; b &= b-1) 
			{
				PxU32 index = (w << 5 | physx::shdfnd::lowestSetBit(b));
	
				memcpy(locked_normalsx, &(orientation[index].column0), sizeof(PxVec3));
				memcpy(locked_normalsy, &(orientation[index].column1), sizeof(PxVec3));
				memcpy(locked_normalsz, &(orientation[index].column2), sizeof(PxVec3));
				memcpy(locked_positions, positions + index, sizeof(PxVec3));

				locked_positions += positionStride;
				locked_normalsx += normalXStride;
				locked_normalsy += normalYStride;
				locked_normalsz += normalZStride;
				numParticles++;
			}
		}
	}

	mInstanceBuffer->unlockSemantic(RendererInstanceBuffer::SEMANTIC_NORMALZ);
	mInstanceBuffer->unlockSemantic(RendererInstanceBuffer::SEMANTIC_NORMALY);
	mInstanceBuffer->unlockSemantic(RendererInstanceBuffer::SEMANTIC_NORMALX);
	mInstanceBuffer->unlockSemantic(RendererInstanceBuffer::SEMANTIC_POSITION);
	m_mesh->setInstanceBufferRange(0, numParticles);
}

void RendererParticleSystemShape::updateBillboard(PxU32 validParticleRange, 
										 const PxVec3* positions, 
										 const PxU32* validParticleBitmap,
										 const PxReal* lifetime) 
{
	PxU32 positionStride = 0, colorStride = 0;
	PxU8 *locked_positions = NULL, *locked_colors = NULL;
	locked_positions = static_cast<PxU8*>(mVertexBuffer[DYNAMIC_POS_VB]->lockSemantic(
							RendererVertexBuffer::SEMANTIC_POSITION, positionStride));
	if(mFading && lifetime)
	{
		locked_colors = static_cast<PxU8*>(mVertexBuffer[DYNAMIC_COL_VB]->lockSemantic(
							RendererVertexBuffer::SEMANTIC_COLOR, colorStride));
		// move ptr to alpha
#if !defined(RENDERER_XBOX360)
		locked_colors += 3 * sizeof(PxU8);
#endif
	}
	// update vertex or instance buffer here
	PxU32 numParticles = 0;
	if(validParticleRange > 0)
	{
		for (PxU32 w = 0; w <= (validParticleRange-1) >> 5; w++)
		{
			for (PxU32 b = validParticleBitmap[w]; b; b &= b-1) 
			{
				PxU32 index = (w << 5 | physx::shdfnd::lowestSetBit(b));
				*reinterpret_cast<PxVec3*>(locked_positions) = positions[index];
				locked_positions += positionStride;
				if(mFading && lifetime) 
				{
					PxU8 particle_lifetime = 0;
					if(lifetime[index] >= mFadingPeriod)
					{
						particle_lifetime = 255;
					}
					else
					{
						if(lifetime[index] <= 0.0f)
						{ 
							particle_lifetime = 0; 
						}
						else
						{
							particle_lifetime = static_cast<PxU8>(lifetime[index] * 255 / mFadingPeriod);
						}
					}
					locked_colors[0] = particle_lifetime;
					locked_colors += colorStride;
				}
				numParticles++;
			}
		}
	}
	
	if(mFading && lifetime) 
	{
		mVertexBuffer[DYNAMIC_COL_VB]->unlockSemantic(RendererVertexBuffer::SEMANTIC_COLOR);
	}
	mVertexBuffer[DYNAMIC_POS_VB]->unlockSemantic(RendererVertexBuffer::SEMANTIC_POSITION);
	m_mesh->setVertexBufferRange(0, numParticles);
}
