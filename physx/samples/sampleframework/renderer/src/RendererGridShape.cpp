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
