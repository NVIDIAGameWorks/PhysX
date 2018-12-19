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
//
// RendererBoxShape : convenience class for generating a box mesh.
//
#include <RendererBoxShape.h>

#include <Renderer.h>

#include <RendererVertexBuffer.h>
#include <RendererVertexBufferDesc.h>

#include <RendererIndexBuffer.h>
#include <RendererIndexBufferDesc.h>

#include <RendererMesh.h>
#include <RendererMeshDesc.h>

#include <RendererMemoryMacros.h>

using namespace SampleRenderer;

typedef struct
{
	PxVec3 positions[4];
	PxVec3 normal;
} BoxFace;

static const BoxFace g_BoxFaces[] =
{
	{ // Z+
		{PxVec3(-1,-1, 1), PxVec3(-1, 1, 1), PxVec3( 1, 1, 1), PxVec3( 1,-1, 1)},
			PxVec3( 0, 0, 1)
	},
	{ // X+
		{PxVec3( 1,-1, 1), PxVec3( 1, 1, 1), PxVec3( 1, 1,-1), PxVec3( 1,-1,-1)},
			PxVec3( 1, 0, 0)
	},
	{ // Z-
		{PxVec3( 1,-1,-1), PxVec3( 1, 1,-1), PxVec3(-1, 1,-1), PxVec3(-1,-1,-1)},
			PxVec3( 0, 0,-1)
	},
	{ // X-
		{PxVec3(-1,-1,-1), PxVec3(-1, 1,-1), PxVec3(-1, 1, 1), PxVec3(-1,-1, 1)},
			PxVec3(-1, 0, 0)
	},
	{ // Y+
		{PxVec3(-1, 1, 1), PxVec3(-1, 1,-1), PxVec3( 1, 1,-1), PxVec3( 1, 1, 1)},
			PxVec3( 0, 1, 0)
	},
	{ // Y-
		{PxVec3(-1,-1,-1), PxVec3(-1,-1, 1), PxVec3( 1,-1, 1), PxVec3( 1,-1,-1)},
			PxVec3( 0,-1, 0)
	}
};

static const PxVec3 g_BoxUVs[] =
{
	PxVec3(0,1,0), PxVec3(0,0,0),
	PxVec3(1,0,0), PxVec3(1,1,0),
};

namespace physx
{
	static PxVec3 operator*(const PxVec3 &a, const PxVec3 &b)
	{
		return PxVec3(a.x*b.x, a.y*b.y, a.z*b.z);
	}
}

RendererBoxShape::RendererBoxShape(Renderer &renderer, const PxVec3 &extents, const PxReal* userUVs) :
RendererShape(renderer)
{
	const PxU32 numVerts     = 24;
	const PxU32 numIndices   = 36;

	RendererVertexBufferDesc vbdesc;
	vbdesc.hint = RendererVertexBuffer::HINT_STATIC;
	vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_POSITION]  = RendererVertexBuffer::FORMAT_FLOAT3;
	vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_NORMAL]    = RendererVertexBuffer::FORMAT_FLOAT3;
	vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_TEXCOORD0] = RendererVertexBuffer::FORMAT_FLOAT2;
	vbdesc.maxVertices = numVerts;
	m_vertexBuffer = m_renderer.createVertexBuffer(vbdesc);
	RENDERER_ASSERT(m_vertexBuffer, "Failed to create Vertex Buffer.");
	if(m_vertexBuffer)
	{
		PxU32 positionStride = 0;
		void *positions = m_vertexBuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_POSITION, positionStride);
		PxU32 normalStride = 0;
		void *normals = m_vertexBuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_NORMAL, normalStride);
		PxU32 uvStride = 0;
		void *uvs = m_vertexBuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_TEXCOORD0, uvStride);
		if(positions && normals && uvs)
		{
			for(PxU32 i=0; i<6; i++)
			{
				const BoxFace &bf = g_BoxFaces[i];
				for(PxU32 j=0; j<4; j++)
				{
					PxVec3 &p  = *(PxVec3*)positions; positions = ((PxU8*)positions) + positionStride;
					PxVec3 &n  = *(PxVec3*)normals;   normals   = ((PxU8*)normals)   + normalStride;
					PxF32 *uv  =  (PxF32*)uvs;        uvs       = ((PxU8*)uvs)       + uvStride;
					n = bf.normal;
					p = bf.positions[j] * extents;
					if(userUVs)
					{
						uv[0] = *userUVs++;
						uv[1] = *userUVs++;
					}
					else
					{
						uv[0] = g_BoxUVs[j].x;
						uv[1] = g_BoxUVs[j].y;
					}
				}
			}
		}
		m_vertexBuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_TEXCOORD0);
		m_vertexBuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_NORMAL);
		m_vertexBuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_POSITION);
	}

	RendererIndexBufferDesc ibdesc;
	ibdesc.hint       = RendererIndexBuffer::HINT_STATIC;
	ibdesc.format     = RendererIndexBuffer::FORMAT_UINT16;
	ibdesc.maxIndices = numIndices;
	m_indexBuffer = m_renderer.createIndexBuffer(ibdesc);
	RENDERER_ASSERT(m_indexBuffer, "Failed to create Index Buffer.");
	if(m_indexBuffer)
	{
		PxU16 *indices = (PxU16*)m_indexBuffer->lock();
		if(indices)
		{
			for(PxU8 i=0; i<6; i++)
			{
				const PxU16 base = i*4;
				*(indices++) = base+0;
				*(indices++) = base+1;
				*(indices++) = base+2;
				*(indices++) = base+0;
				*(indices++) = base+2;
				*(indices++) = base+3;
			}
		}
		m_indexBuffer->unlock();
	}

	if(m_vertexBuffer && m_indexBuffer)
	{
		RendererMeshDesc meshdesc;
		meshdesc.primitives       = RendererMesh::PRIMITIVE_TRIANGLES;
		meshdesc.vertexBuffers    = &m_vertexBuffer;
		meshdesc.numVertexBuffers = 1;
		meshdesc.firstVertex      = 0;
		meshdesc.numVertices      = numVerts;
		meshdesc.indexBuffer      = m_indexBuffer;
		meshdesc.firstIndex       = 0;
		meshdesc.numIndices       = numIndices;
		m_mesh = m_renderer.createMesh(meshdesc);
		RENDERER_ASSERT(m_mesh, "Failed to create Mesh.");
	}
}

RendererBoxShape::~RendererBoxShape(void)
{
	SAFE_RELEASE(m_vertexBuffer);
	SAFE_RELEASE(m_indexBuffer);
	SAFE_RELEASE(m_mesh);
}
