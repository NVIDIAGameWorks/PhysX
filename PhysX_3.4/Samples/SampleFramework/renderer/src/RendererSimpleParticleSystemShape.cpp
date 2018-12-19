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

#include <RendererSimpleParticleSystemShape.h>

#include <Renderer.h>

#include <RendererMemoryMacros.h>
#include <RendererIndexBuffer.h>
#include <RendererIndexBufferDesc.h>
#include <RendererVertexBuffer.h>
#include <RendererVertexBufferDesc.h>
#include <RendererMesh.h>
#include <RendererMeshDesc.h>
#include "PsUtilities.h"
#include "PsBitUtils.h"

using namespace SampleRenderer;
using namespace physx::shdfnd;

RendererSimpleParticleSystemShape::RendererSimpleParticleSystemShape(Renderer &renderer, 
	PxU32 _mMaxParticles) : 
											RendererShape(renderer), 
											mMaxParticles(_mMaxParticles),
											mVertexBuffer(NULL),
											mIndexBuffer(NULL)											
{
	RendererVertexBufferDesc vbdesc;

	vbdesc.maxVertices = mMaxParticles*4;
	vbdesc.hint = RendererVertexBuffer::HINT_DYNAMIC;
	vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_POSITION] = RendererVertexBuffer::FORMAT_FLOAT3;
	vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_COLOR] = RendererVertexBuffer::FORMAT_COLOR_NATIVE;
	vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_TEXCOORD0] = RendererVertexBuffer::FORMAT_FLOAT2;
	mVertexBuffer = m_renderer.createVertexBuffer(vbdesc);
	RENDERER_ASSERT(mVertexBuffer, "Failed to create Vertex Buffer.");
	// if enabled mInstanced meshes -> create instance buffer
	PxU32 color = renderer.convertColor(RendererColor(255, 255, 255, 255));
	RendererMesh::Primitive primitive = RendererMesh::PRIMITIVE_TRIANGLES;
	initializeVertexBuffer(color);	

	if(mVertexBuffer)
	{
		RendererMeshDesc meshdesc;
		meshdesc.primitives			= primitive;
		meshdesc.vertexBuffers		= &mVertexBuffer;
		meshdesc.numVertexBuffers	= 1;
		meshdesc.firstVertex		= 0;
		meshdesc.numVertices		= mVertexBuffer->getMaxVertices();
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
		meshdesc.instanceBuffer		= NULL;
		meshdesc.firstInstance		= 0;
		meshdesc.numInstances		= 0;
		m_mesh = m_renderer.createMesh(meshdesc);
		RENDERER_ASSERT(m_mesh, "Failed to create Mesh.");
	}
}

RendererSimpleParticleSystemShape::~RendererSimpleParticleSystemShape(void)
{
	SAFE_RELEASE(mIndexBuffer);
	SAFE_RELEASE(mVertexBuffer);
	SAFE_RELEASE(m_mesh);
}

void RendererSimpleParticleSystemShape::initializeVertexBuffer(PxU32 color) 
{
	PxU32 positionStride = 0, colorStride = 0, uvStride = 0;
	PxU8* locked_positions = static_cast<PxU8*>(mVertexBuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_POSITION, positionStride));	
	PxU8* locked_colors = static_cast<PxU8*>(mVertexBuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_COLOR, colorStride));
	PxU8* locked_uvs_base = static_cast<PxU8*>(mVertexBuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_TEXCOORD0, uvStride));	
	for(PxU32 i = 0; i < mMaxParticles; ++i) 
	{		

		for (PxU32 j = 0;j < 4; j++)
		{
			memset(locked_colors, color, sizeof(PxU32));
			memset(locked_positions, 0, sizeof(PxReal) * 3);
			locked_colors += colorStride;
			locked_positions += positionStride;
		}
		
		PxReal* locked_uvs = reinterpret_cast<PxReal*> (locked_uvs_base);
        *locked_uvs = 0.0f;
		locked_uvs++;
		*locked_uvs = 0.0f;
		locked_uvs_base += uvStride;

		locked_uvs = reinterpret_cast<PxReal*> (locked_uvs_base);
        *locked_uvs = 1.0f;
		locked_uvs++;
		*locked_uvs = 0.0f;
		locked_uvs_base += uvStride;

		locked_uvs = reinterpret_cast<PxReal*> (locked_uvs_base);
        *locked_uvs = 0.0f;
		locked_uvs++;
		*locked_uvs = 1.0f;
		locked_uvs_base += uvStride;

		locked_uvs = reinterpret_cast<PxReal*> (locked_uvs_base);
        *locked_uvs = 1.0f;
		locked_uvs++;
		*locked_uvs = 1.0f;
		locked_uvs_base += uvStride;
	}
	mVertexBuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_COLOR);
	mVertexBuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_POSITION);
	mVertexBuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_TEXCOORD0);

	RendererIndexBufferDesc inbdesc;
	inbdesc.hint = RendererIndexBuffer::HINT_STATIC;
	inbdesc.format = RendererIndexBuffer::FORMAT_UINT16;
	inbdesc.maxIndices = mMaxParticles*6;
	mIndexBuffer = m_renderer.createIndexBuffer(inbdesc);
	
	PxU16* ib = (PxU16*) mIndexBuffer->lock();
	for (PxU16 i = 0; i<mMaxParticles; i++)
	{
        ib[i * 6 + 0] = i * 4 + 0;
        ib[i * 6 + 1] = i * 4 + 1;
        ib[i * 6 + 2] = i * 4 + 2;
        ib[i * 6 + 3] = i * 4 + 1;
        ib[i * 6 + 4] = i * 4 + 3;
        ib[i * 6 + 5] = i * 4 + 2;
	}
	mIndexBuffer->unlock();
}

void RendererSimpleParticleSystemShape::updateBillboard(PxU32 validParticleRange, 
										 const PxVec3* positions, 
										 const PxU32* validParticleBitmap,
										 const PxReal* lifetime) 
{
	PxU32 positionStride = 0;
	PxU8 *locked_positions = NULL;
	locked_positions = static_cast<PxU8*>(mVertexBuffer->lockSemantic(
							RendererVertexBuffer::SEMANTIC_POSITION, positionStride));
	// update vertex buffer here
	PxU32 numParticles = 0;
	if(validParticleRange > 0)
	{
		for (PxU32 w = 0; w <= (validParticleRange-1) >> 5; w++)
		{
			for (PxU32 b = validParticleBitmap[w]; b; b &= b-1) 
			{
				PxU32 index = (w << 5 | physx::shdfnd::lowestSetBit(b));
				const PxVec3& basePos = positions[index];
				const float particleSize = 1.0f;
				PxVec3 offsets[4] =
				{
					PxVec3(-particleSize,  particleSize, 0.0f),
					PxVec3( particleSize,  particleSize, 0.0f),
					PxVec3(-particleSize, -particleSize, 0.0f),
					PxVec3( particleSize, -particleSize, 0.0f)
				};

				for (int p = 0; p < 4; p++)
				{
					*reinterpret_cast<PxVec3*>(locked_positions) = basePos + offsets[p];
					locked_positions += positionStride;
				}
				numParticles++;
			}
		}
	}
	
	mVertexBuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_POSITION);
	m_mesh->setVertexBufferRange(0, numParticles*4);
}
