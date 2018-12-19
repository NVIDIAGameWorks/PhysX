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
// RendererGridShape : convenience class for generating a grid mesh.
//
#include <RendererGridShape.h>

#include <Renderer.h>

#include <RendererVertexBuffer.h>
#include <RendererVertexBufferDesc.h>

#include <RendererMesh.h>
#include <RendererMeshDesc.h>

#include <RendererMemoryMacros.h>

#include <foundation/PxStrideIterator.h>
#include <PsMathUtils.h>

using namespace SampleRenderer;

RendererGridShape::RendererGridShape(Renderer &renderer, PxU32 size, float cellSize, bool showAxis /*= false*/, UpAxis up/*= UP_Y*/) :
RendererShape(renderer), m_UpAxis(up)
{
	m_vertexBuffer = 0;

	const PxU32 numVerts    = size*8 + 8;

	RendererVertexBufferDesc vbdesc;
	vbdesc.hint                                                     = RendererVertexBuffer::HINT_STATIC;
	vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_POSITION] = RendererVertexBuffer::FORMAT_FLOAT3;
	vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_COLOR]    = RendererVertexBuffer::FORMAT_COLOR_NATIVE;
	vbdesc.maxVertices                                              = numVerts;
	m_vertexBuffer = m_renderer.createVertexBuffer(vbdesc);

	if(m_vertexBuffer)
	{
		RendererMeshDesc meshdesc;
		meshdesc.primitives       = RendererMesh::PRIMITIVE_LINES;
		meshdesc.vertexBuffers    = &m_vertexBuffer;
		meshdesc.numVertexBuffers = 1;
		meshdesc.firstVertex      = 0;
		meshdesc.numVertices      = numVerts;
		m_mesh = m_renderer.createMesh(meshdesc);
	}
	if(m_vertexBuffer && m_mesh)
	{
		PxU32 color1 = m_renderer.convertColor(RendererColor ( 94, 108, 127));
		PxU32 color2 = m_renderer.convertColor(RendererColor (120, 138, 163));
		PxU32 colorRed   = m_renderer.convertColor(RendererColor (255,   0,   0));
		PxU32 colorGreen = m_renderer.convertColor(RendererColor (  0, 255,   0));
		PxU32 colorBlue  = m_renderer.convertColor(RendererColor (  0,   0, 255));

		PxStrideIterator<PxVec3> positions;
		{
			PxU32 positionStride = 0;
			void* pos = m_vertexBuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_POSITION, positionStride);
			positions = PxMakeIterator((PxVec3*)pos, positionStride);
		}
		PxStrideIterator<PxU32> colors;
		{
			PxU32 colorStride = 0;
			void* color = m_vertexBuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_COLOR, colorStride);
			colors = PxMakeIterator((PxU32*)color, colorStride);
		}

		if(positions.ptr() && colors.ptr())
		{
			PxVec3 upAxis(0.0f);

			PxU32 colorX1 = color1;
			PxU32 colorX2 = color1;
			PxU32 colorZ1 = color1;
			PxU32 colorZ2 = color1;
			
			switch(up)
			{
			case UP_X: upAxis = PxVec3(1.0f, 0.0f, 0.0f); break;
			case UP_Y: upAxis = PxVec3(0.0f, 1.0f, 0.0f); break;
			case UP_Z: upAxis = PxVec3(0.0f, 0.0f, 1.0f); break;
			}
			PxMat33 rotation = physx::shdfnd::rotFrom2Vectors(PxVec3(0.0f, 1.0f, 0.0f), upAxis);

			if (showAxis)
			{
				switch(up)
				{
				case UP_X: colorX2 = colorGreen; colorZ1 = colorBlue; break;
				case UP_Y: colorX1 = colorRed; colorZ1 = colorBlue; break;
				case UP_Z: colorX1 = colorRed; colorZ2 = colorGreen; break;
				}
			}

			float radius   = size*cellSize;

			positions[0] = rotation * PxVec3(   0.0f, 0.0f,   0.0f); colors[0] = colorX1;
			positions[1] = rotation * PxVec3( radius, 0.0f,   0.0f); colors[1] = colorX1;
			positions[2] = rotation * PxVec3(   0.0f, 0.0f,   0.0f); colors[2] = colorX2;
			positions[3] = rotation * PxVec3(-radius, 0.0f,   0.0f); colors[3] = colorX2;
			positions[4] = rotation * PxVec3(   0.0f, 0.0f,   0.0f); colors[4] = colorZ1;
			positions[5] = rotation * PxVec3(   0.0f, 0.0f, radius); colors[5] = colorZ1;
			positions[6] = rotation * PxVec3(   0.0f, 0.0f,   0.0f); colors[6] = colorZ2;
			positions[7] = rotation * PxVec3(   0.0f, 0.0f,-radius); colors[7] = colorZ2;


			for (PxU32 i = 1; i <= size; i++)
			{
				positions[i*8+0] = rotation * PxVec3(-radius, 0.0f, cellSize * i);
				positions[i*8+1] = rotation * PxVec3( radius, 0.0f, cellSize * i);
				positions[i*8+2] = rotation * PxVec3(-radius, 0.0f,-cellSize * i);
				positions[i*8+3] = rotation * PxVec3( radius, 0.0f,-cellSize * i);
				positions[i*8+4] = rotation * PxVec3( cellSize * i, 0.0f,-radius);
				positions[i*8+5] = rotation * PxVec3( cellSize * i, 0.0f, radius);
				positions[i*8+6] = rotation * PxVec3(-cellSize * i, 0.0f,-radius);
				positions[i*8+7] = rotation * PxVec3(-cellSize * i, 0.0f, radius);

				for (PxU32 j = 0; j < 8; j++)
					colors[i*8+j] = color2;
			}
		}

		m_vertexBuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_COLOR);
		m_vertexBuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_POSITION);
	}
}

RendererGridShape::~RendererGridShape(void)
{
	SAFE_RELEASE(m_vertexBuffer);
	SAFE_RELEASE(m_mesh);
}
