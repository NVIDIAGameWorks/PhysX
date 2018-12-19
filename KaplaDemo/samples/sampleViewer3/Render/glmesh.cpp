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

#include "glmesh.h"
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <vector>
#include <iostream>
using namespace std;
GLMesh::GLMesh(GLuint elementTypei) {
	firstTimeBO = true;
	vbo = ibo = 0;
	withTexture = withColor = withNormal = withTangent = false;
	elementType = elementTypei;
}
GLMesh::~GLMesh() {
	firstTimeBO = true;
	if (vbo) {
		glDeleteBuffersARB(1, &vbo);
	}
	if (ibo) {
		glDeleteBuffersARB(1, &ibo);
	}
}
void GLMesh::reset() {
	firstTimeBO = true;
	if (vbo) {
		glDeleteBuffersARB(1, &vbo);
	}
	if (ibo) {
		glDeleteBuffersARB(1, &ibo);
	}
	vbo = ibo = 0;
	withTexture = withColor = withNormal = withTangent = false;

	indices.clear();
	vertices.clear();
	normals.clear();
	colors.clear();
	texCoords.clear(); // treats as u v
	tangents.clear();
	bitangents.clear();

	// For raw
	rawVertices.clear();
	rawNormals.clear();

}
void  GLMesh::genVBOIBO() {
	glGenBuffersARB(1, &vbo);	
	glGenBuffersARB(1, &ibo);
}
void  GLMesh::updateVBOIBO(bool dynamicVB) {
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, ibo);
	int bytesv = 0;
	if (firstTimeBO) {
		//if (!((vertices.size() > 0) && (indices.size() > 0) && (normals.size() == 0) && (texCoords.size() > 0) && (colors.size() > 0))) {
		//	// Only support pos, texcoord, color, no normal for now :P
		//	exit(-129);
		//}
		bytesv = vertices.size()*sizeof(float)*(3); //position 3, color 3, tex coord 2
		withColor = (colors.size() != 0);
		withTexture = (texCoords.size() != 0);
		withNormal = (normals.size() != 0);
		withTangent = (tangents.size() != 0) && (bitangents.size() != 0);
		if (withColor) bytesv += vertices.size()*sizeof(float)*(3);
		if (withTexture) bytesv += vertices.size()*sizeof(float)*(2);
		if (withNormal) bytesv += vertices.size()*sizeof(float)*(3);
		if (withTangent) bytesv += vertices.size()*sizeof(float)*(6);

		vector<char> dummy(bytesv, 0);


		if (dynamicVB) {
			glBufferDataARB(GL_ARRAY_BUFFER_ARB, bytesv, (const void*)&dummy[0], GL_DYNAMIC_DRAW_ARB);
		} else {
			//glBufferDataARB(GL_ARRAY_BUFFER_ARB, bytesv, (const void*)&dummy[0], GL_STATIC_DRAW_ARB);
		}

		int bytesi = indices.size()*sizeof(PxU32);

		dummy.resize(bytesi, 0);
		glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, bytesi, (const void*)&indices[0], GL_STATIC_DRAW_ARB); // index never change
		firstTimeBO = 0;

	}
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);


	float* vb = 0;
	
	if (dynamicVB) vb = (float*)glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB); else {
		vb = new float[bytesv / sizeof(float)];

	}


	int numV = vertices.size();
	int pos = 0;


	memcpy(vb, &vertices[0], sizeof(PxVec3)*numV); 
	pos+=3*numV;

	if (withColor) {
		memcpy(&vb[pos], &colors[0], sizeof(PxVec3)*numV);
		pos+=3*numV;
	}

	if (withTexture) {		
		memcpy(&vb[pos], &texCoords[0], sizeof(float)*numV*2);
		pos+=2*numV;
	}

	if (withNormal) {		
		memcpy(&vb[pos], &normals[0], sizeof(float)*numV*3);
		pos+=3*numV;
	}


	if (withTangent) {		

		memcpy(&vb[pos], &tangents[0], sizeof(float)*numV*3);
		pos+=3*numV;
		memcpy(&vb[pos], &bitangents[0], sizeof(float)*numV*3);
		pos+=3*numV;
	}
	if (dynamicVB) {
		glUnmapBuffer(GL_ARRAY_BUFFER_ARB);
	} else {
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, bytesv, (const void*)vb, GL_STATIC_DRAW_ARB);
		delete vb;
		
	}
	
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

}
void  GLMesh::drawVBOIBO(bool enable, bool draw, bool disable, bool drawpoints) {
	
	if ((vbo == 0) || (ibo == 0)) return;
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, ibo);

	
	// Only support pos, texcoord, color, no normal for now :P
	int numV = vertices.size();
	if(enable) glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(PxVec3), 0);
	GLuint pos = 3*sizeof(float)*numV;
	if (withColor) {
		if(enable) glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(3, GL_FLOAT, 0, (const GLvoid*) (pos));
		pos+=3*sizeof(float)*numV;
	}
	if (withTexture) {
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
		if(enable) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, (const GLvoid*) (pos));
		pos+=2*sizeof(float)*numV;
	}
	if (withNormal) {
		if(enable) glEnableClientState(GL_NORMAL_ARRAY);		
		glNormalPointer(GL_FLOAT, sizeof(PxVec3), (const GLvoid*) (pos));
		pos+=3*sizeof(float)*numV;
	}
	
	if (withTangent) {
 	    
			
		glClientActiveTextureARB(GL_TEXTURE1_ARB);
		if(enable) {
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		glTexCoordPointer(3, GL_FLOAT, 0, (const GLvoid*) (pos));
		pos+=3*sizeof(float)*numV;
		glClientActiveTextureARB(GL_TEXTURE2_ARB);
		if(enable) {
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		glTexCoordPointer(3, GL_FLOAT, 0, (const GLvoid*) (pos));
		pos+=3*sizeof(float)*numV;
	}
	
	if (draw) {
		if (!drawpoints) {
			glDrawElements(elementType, indices.size(), GL_UNSIGNED_INT, 0);
		} else {
			glDrawArrays(GL_POINTS, 0, numV);
		}
	}
	
	if (disable) {
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glClientActiveTextureARB(GL_TEXTURE2_ARB);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glClientActiveTextureARB(GL_TEXTURE1_ARB);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glDisableClientState(GL_NORMAL_ARRAY);

		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	}
	
}

void GLMesh::draw() {
	if ((vertices.size() > 0) && (indices.size() > 0)) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(PxVec3), &vertices[0]);

		if (normals.size() > 0) {
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT, sizeof(PxVec3), &normals[0]);
		}
		
		if (texCoords.size() > 0) {
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 0, &texCoords[0]);
		}

		if (colors.size() > 0) {
			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer(3, GL_FLOAT, 0, &colors[0]);
		}

		//if (mTexId > 0) {
		//	glColor3f(1.0f, 1.0f, 1.0f);
		//	glEnable(GL_TEXTURE_2D);
		//	glBindTexture(GL_TEXTURE_2D, mTexId);
		//}
		//else {
		//	glColor3f(0.5f, 0.5f, 0.5f);
		//	glDisable(GL_TEXTURE_2D);
		//}

		glDrawElements(elementType, indices.size(), GL_UNSIGNED_INT, &indices[0]);
	}
	if (texCoords.size() > 0) {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	if (colors.size() > 0) {
		glDisableClientState(GL_COLOR_ARRAY);
	}
	if (rawVertices.size() > 0) {
		// Also draw raw buffer

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(PxVec3), &rawVertices[0]);
		if (rawNormals.size() > 0) {
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT, sizeof(PxVec3), &rawNormals[0]);
		}
		glDrawArrays(elementType, 0, rawVertices.size());

		
	}
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);


	//glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	//glDisable(GL_TEXTURE_2D);


}

