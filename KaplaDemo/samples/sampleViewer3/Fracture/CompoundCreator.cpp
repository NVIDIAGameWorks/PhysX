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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.

#include "CompoundCreator.h"
#include <algorithm>
#include <assert.h>
#include "CompoundGeometry.h"

#define DEBUG_DRAW 1

#if DEBUG_DRAW
#include <windows.h>
#include <GL/gl.h>
#endif

// -----------------------------------------------------------------------------
void CompoundCreator::debugDraw()
{
#if DEBUG_DRAW

	const bool drawEdges = false;

	if (drawEdges) {
		glBegin(GL_LINES);
		for (int i = 0; i < (int)mTetEdges.size(); i++) {
			TetEdge &e = mTetEdges[i];
			if (e.onSurface)
				glColor3f(1.0f, 0.0f, 0.0f);
			else
				glColor3f(1.0f, 1.0f, 0.0f);

			PxVec3 &p0 = mTetVertices[e.i0];
			PxVec3 &p1 = mTetVertices[e.i1];
			glVertex3f(p0.x, p0.y, p0.z);
			glVertex3f(p1.x, p1.y, p1.z);
		}
		glEnd();
	}
	
#endif
}