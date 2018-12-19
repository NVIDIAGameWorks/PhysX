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

#ifndef __GLMESH_H__
#define __GLMESH_H__
#include <GL/glew.h>

#include "PxPhysics.h"
#include <vector>
using namespace physx;
class GLMesh{
public:
	GLMesh(GLuint elementTypei = GL_TRIANGLES);
	~GLMesh();
	void draw();

	// For indices
	std::vector<PxU32> indices;
	std::vector<PxVec3> vertices;
	std::vector<PxVec3> normals;
	std::vector<PxVec3> colors;
	std::vector<float> texCoords; // treats as u v
	std::vector<PxVec3> tangents;
	std::vector<PxVec3> bitangents;

	// For raw
	std::vector<PxVec3> rawVertices;
	std::vector<PxVec3> rawNormals;

	void reset();
	void genVBOIBO();
	void updateVBOIBO(bool dynamicVB = true);
	void drawVBOIBO(bool enable = true, bool draw = true, bool disable = true, bool drawpoints = false);

	// For vertex buffer
	GLuint vbo;
	GLuint ibo;
	bool firstTimeBO;
	bool withTexture, withColor, withNormal, withTangent;
	GLuint elementType;


}; 
#endif
