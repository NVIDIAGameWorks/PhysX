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

#include "PxPhysics.h"
#include "PxCooking.h"
#include "PxDefaultStreams.h"
#include "PxShape.h"
#include "foundation/PxMath.h"
#include "PxRigidDynamic.h"
#include "PxConvexMesh.h"
#include "foundation/PxMat44.h"
#include "foundation/PxMathUtils.h"

#include "CompoundGeometry.h"
#include "Shader.h"
#include "PolygonTriangulator.h"
#include "XMLParser.h"

#include "Convex.h"

#include "PhysXMacros.h"

class physx::PxPhysics;
class physx::PxCooking;
class physx::PxActor;
class physx::PxScene;
class physx::PxConvexMesh;

#define COOK_TRIANGLES 0

#include <algorithm>

// --------------------------------------------------------------------------------------------
void Convex::draw(bool debug)
{
//if (!mIsGhostConvex)
//	return;	// foo

	bool drawConvex = true;
	bool drawVisMesh = true;
	bool wireframe = false;
	bool drawWorldBounds = false;
	bool drawInsideTest = false;
	bool drawVisPolys = true;
	bool drawTangents = false;
	bool drawNewFlag = false;

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0f, 1.0f);

	if (!debug) {
		drawConvex = false;
		drawVisMesh = true;
		wireframe = false;
		drawWorldBounds = false;
		drawInsideTest = false;
		drawVisPolys = false;
		drawTangents = false;
	}

	if (drawConvex) {
		if (mHasExplicitVisMesh)
			glColor3f(0.0f, 0.0f, 1.0f);
		else if (mIsGhostConvex)
			glColor3f(0.0f, 1.0f, 0.0f);
		else
			glColor3f(1.0f, 1.0f, 1.0f);
		glDisable(GL_LIGHTING);
		if (mPxActor != NULL) {
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			PxMat44 mat(getGlobalPose());
			glMultMatrixf((GLfloat*)mat.front());
		}

		glBegin(GL_LINES);
		for (int i = 0; i < (int)mFaces.size(); i++) {
			Face &f = mFaces[i];
			for (int j = 0; j < f.numIndices; j++) {
				PxVec3 &p0 = mVertices[mIndices[f.firstIndex + j]];
				PxVec3 &p1 = mVertices[mIndices[f.firstIndex + (j+1)%f.numIndices]];
				glVertex3f(p0.x, p0.y, p0.z);
				glVertex3f(p1.x, p1.y, p1.z);
			}

			if (drawNewFlag && (f.flags & CompoundGeometry::FF_NEW) && f.numIndices > 0) {
				float r = 0.01f;

				PxVec3 c(0.0f, 0.0f, 0.0f);
				for (int j = 0; j < f.numIndices; j++) 
					c += mVertices[mIndices[f.firstIndex + j]];
				c /= (float)f.numIndices;
				glVertex3f(c.x - r, c.y, c.z); glVertex3f(c.x + r, c.y, c.z);
				glVertex3f(c.x, c.y - r, c.z); glVertex3f(c.x, c.y + r, c.z);
				glVertex3f(c.x, c.y, c.z - r); glVertex3f(c.x, c.y, c.z + r);
			}
		}
		glEnd();
		glEnable(GL_LIGHTING);

		if (mPxActor != NULL) {
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}
	}

	if (mVisVertices.empty())
		return;

	if (drawVisPolys) {
		if (mPxActor != NULL) {
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			PxMat44 mat(getGlobalPose());
			glMultMatrixf((GLfloat*)mat.front());
		}

		glColor3f(1.0f, 0.0f, 0.0f);
		glDisable(GL_LIGHTING);
		glBegin(GL_LINES);
		for (int i = 0; i < (int)mVisPolyStarts.size()-1; i++) {
			int first = mVisPolyStarts[i];
			int num = mVisPolyStarts[i+1] - first;

			for (int j = 0; j < num; j++) {
				PxVec3 &p0 = mVisVertices[mVisPolyIndices[first + j]];
				PxVec3 &p1 = mVisVertices[mVisPolyIndices[first + (j+1)%num]];
				glVertex3f(p0.x, p0.y, p0.z);
				glVertex3f(p1.x, p1.y, p1.z);
			}
		}
		glEnd();
		glEnable(GL_LIGHTING);

		if (mPxActor != NULL) {
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}
	}

	if (drawTangents) {
		const float r = 0.1f;
		if (mPxActor != NULL) {
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			PxMat44 mat(getGlobalPose());
			glMultMatrixf((GLfloat*)mat.front());
		}

		glColor3f(1.0f, 1.0f, 0.0f);
		glDisable(GL_LIGHTING);
		glBegin(GL_LINES);
		for (int i = 0; i < (int)mVisVertices.size()-1; i++) {
			PxVec3 p0 = mVisVertices[i];
			PxVec3 p1 = p0 + r * mVisTangents[i];
			glVertex3f(p0.x, p0.y, p0.z);
			glVertex3f(p1.x, p1.y, p1.z);
		}
		glEnd();
		glEnable(GL_LIGHTING);

		if (mPxActor != NULL) {
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}
	}

	if (drawVisMesh && mVisTriIndices.size() > 0) {
		if (mPxActor != NULL) {
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			PxMat44 mat(getGlobalPose());
			glMultMatrixf((GLfloat*)mat.front());
		}

		if (wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDisable(GL_LIGHTING);
		}

		if (debug)
			glColor3f(1.0f, 1.0f, 1.0f);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);

		glVertexPointer(3, GL_FLOAT, 0, &mVisVertices[0]);
		glNormalPointer(GL_FLOAT, sizeof(PxVec3), &mVisNormals[0]);

		glDrawElements(GL_TRIANGLES, mVisTriIndices.size(), GL_UNSIGNED_INT, &mVisTriIndices[0]);	

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);

		if (mPxActor != NULL) {
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}

		if (wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glEnable(GL_LIGHTING);
		}
	}


	if (drawWorldBounds) {
		static const int corners[12 * 2][3] = {
			{0,0,0}, {1,0,0},  {0,1,0}, {1,1,0},  {0,1,1}, {1,1,1},  {0,0,1}, {1,0,1}, 
			{0,0,0}, {0,1,0},  {1,0,0}, {1,1,0},  {1,0,1}, {1,1,1},  {0,0,1}, {0,1,1}, 
			{0,0,0}, {0,0,1},  {1,0,0}, {1,0,1},  {1,1,0}, {1,1,1},  {0,1,0}, {0,1,1}};
		PxBounds3 b;
		getWorldBounds(b);
		glBegin(GL_LINES);
		for (int i = 0; i < 24; i++) {
			glVertex3f(
				corners[i][0] ? b.minimum.x : b.maximum.x, 
				corners[i][1] ? b.minimum.y : b.maximum.y, 
				corners[i][2] ? b.minimum.z : b.maximum.z);
		}
		glEnd();
	}

	if (drawInsideTest) {
		PxBounds3 bounds;
		bounds.setEmpty();
		for (int i = 0; i < (int)mVertices.size(); i++) 
			bounds.include(mVertices[i]);

		int num = 20;
		float r = 0.05f * bounds.getDimensions().magnitude() / (float)num;
		PxVec3 p;
		glColor3f(1.0f, 0.0f, 0.0f);
		glBegin(GL_LINES);
		for (int xi = 0; xi < num; xi++) {
			for (int yi = 0; yi < num; yi++) {
				for (int zi = 0; zi < num; zi++) {
					p.x = bounds.minimum.x + xi * (bounds.maximum.x - bounds.minimum.x)/num;
					p.y = bounds.minimum.y + yi * (bounds.maximum.y - bounds.minimum.y)/num;
					p.z = bounds.minimum.z + zi * (bounds.maximum.z - bounds.minimum.z)/num;
					if (insideVisualMesh(p)) {
						glVertex3f(p.x - r, p.y, p.z); glVertex3f(p.x + r, p.y, p.z);
						glVertex3f(p.x, p.y - r, p.z); glVertex3f(p.x, p.y + r, p.z);
						glVertex3f(p.x, p.y, p.z - r); glVertex3f(p.x, p.y, p.z + r);
					}
				}
			}
		}
		glEnd();
		
	}

	glDisable(GL_POLYGON_OFFSET_FILL);
}

// --------------------------------------------------------------------------------------------
bool Convex::createFromXml(XMLParser *p, float scale, bool ignoreVisualMesh)
{
	clear();
	PxVec3 center(0.0f, 0.0f, 0.0f);

	std::vector<unsigned char> buffer(10000);

	for (int i = 0; i < (int)p->vars.size(); i++) {
		if (p->vars[i].name == "isGhost") {
			mIsGhostConvex = p->vars[i].value == "true";
		}
		else if (p->vars[i].name == "modelIslandNr")
			sscanf(p->vars[i].value.c_str(), "%i", &mModelIslandNr);			
		else if (p->vars[i].name == "centerX")
			sscanf(p->vars[i].value.c_str(), "%f", &center.x);
		else if (p->vars[i].name == "centerY")
			sscanf(p->vars[i].value.c_str(), "%f", &center.y);
		else if (p->vars[i].name == "centerZ")
			sscanf(p->vars[i].value.c_str(), "%f", &center.z);			
		else if (p->vars[i].name == "surfacMaterialNr") 
			sscanf(p->vars[i].value.c_str(), "%i", &mSurfaceMaterialId);			
		else if (p->vars[i].name == "indestructible") 
			mIndestructible = p->vars[i].value == "true";
		else if (p->vars[i].name == "materialId")
			sscanf(p->vars[i].value.c_str(), "%i", &mMaterialId);			
	}
	center *= scale;
	mLocalPose = PX_TRANSFORM_ID;
	mLocalPose.p = center;
	mMaterialOffset = center;

	p->readNextTag();
	while (!p->endOfFile() && p->tagName != "Convex") {
		if (p->tagName == "Vertices") {
			int numVerts = 0;
			bool binary = false;
			bool binDouble = false;
			for (int i = 0; i < (int)p->vars.size(); i++) {
				if (p->vars[i].name == "count") {
					sscanf(p->vars[i].value.c_str(), "%i", &numVerts);
				}
				else if (p->vars[i].name == "binary")
					binary = p->vars[i].value == "true";
				else if (p->vars[i].name == "double")
					binDouble = p->vars[i].value == "true";
			}
			if (binary) {
				if (binDouble) {
					int size = numVerts * 3 * sizeof(double);
					buffer.resize(size);
					p->getFileScanner().getBinaryData(&buffer[0], size);
					double *d = (double*)&buffer[0];
					for (int i = 0; i < numVerts; i++) {
						PxVec3 v;
						v.x = (float)*d++;
						v.y = (float)*d++;
						v.z = (float)*d++;
						mVertices.pushBack(scale * v);
					}
				}
				else {
					int size = numVerts * 3 * sizeof(float);
					buffer.resize(size);
					p->getFileScanner().getBinaryData(&buffer[0], size);
					float *f = (float*)&buffer[0];
					for (int i = 0; i < numVerts; i++) {
						PxVec3 v;
						v.x = *f++;
						v.y = *f++;
						v.z = *f++;
						mVertices.pushBack(scale * v);
					}
				}
			}
			else {
				for (int i = 0; i < numVerts; i++) {
					PxVec3 v;
					p->getFileScanner().getFloatSymbol(v.x);
					p->getFileScanner().getFloatSymbol(v.y);
					p->getFileScanner().getFloatSymbol(v.z);
					mVertices.pushBack(scale * v);
				}
			}
			p->readNextTag();		// </Vertices>
			p->readNextTag();
		}
		else if (p->tagName == "Faces") {
			p->readNextTag();
			while (!p->endOfFile() && p->tagName != "Faces") {
				if (p->tagName == "Face") {
					int size = 0;
					bool hasNormals = false;
					bool isSurface = false;
					for (int i = 0; i < (int)p->vars.size(); i++) {
						if (p->vars[i].name == "size") {
							sscanf(p->vars[i].value.c_str(), "%i", &size);
						}
						else if (p->vars[i].name == "hasNormals") {
							hasNormals = p->vars[i].value == "true";
						}
						else if (p->vars[i].name == "isSurface") {
							isSurface = p->vars[i].value == "true";
						}
					}
					Face f;
					f.init();
					f.firstIndex = mIndices.size();
					f.numIndices = size;
					f.firstNormal = mNormals.size();
					f.flags = 0;
					if (hasNormals)
						f.flags |= CompoundGeometry::FF_HAS_NORMALS;
					if (isSurface)
						f.flags |= CompoundGeometry::FF_OBJECT_SURFACE;

					for (int i = 0; i < size; i++) {
						int id;
						p->getFileScanner().getIntSymbol(id);
						mIndices.pushBack(id);
						if (hasNormals) {
							PxVec3 n;
							p->getFileScanner().getFloatSymbol(n.x);
							p->getFileScanner().getFloatSymbol(n.y);
							p->getFileScanner().getFloatSymbol(n.z);
							mNormals.pushBack(n);
						}
					}
					mFaces.pushBack(f);
					p->readNextTag();	// </Face>
					p->readNextTag();
				}
			}
			p->readNextTag();	
		}
		else if (p->tagName == "VisualVertices") {
			if (!ignoreVisualMesh)
				mHasExplicitVisMesh = true;

			int numVerts = 0;
			bool hasNormals = false;
			bool hasTexCoords = false;
			bool binary = false;
			bool binDouble = false;

			for (int i = 0; i < (int)p->vars.size(); i++) {
				if (p->vars[i].name == "count") {
					sscanf(p->vars[i].value.c_str(), "%i", &numVerts);
				}
				else if (p->vars[i].name == "normals") {
					hasNormals = p->vars[i].value == "true";
				}
				else if (p->vars[i].name == "texCoords") {
					hasTexCoords = p->vars[i].value == "true";
				}
				else if (p->vars[i].name == "binary")
					binary = p->vars[i].value == "true";
				else if (p->vars[i].name == "double")
					binDouble = p->vars[i].value == "true";
			}

			PxVec3 v, n, t;
			float tu, tv;

			if (binary) {
				int numScalars = 3;
				if (hasNormals) numScalars += 3;
				if (hasTexCoords) numScalars += 2;

				if (binDouble) {
					int size = numVerts * numScalars * sizeof(double);
					buffer.resize(size);
					p->getFileScanner().getBinaryData(&buffer[0], size);
					if (!ignoreVisualMesh) {
						double *d = (double*)&buffer[0];
						for (int i = 0; i < numVerts; i++) {
							v.x = (float)*d++;
							v.y = (float)*d++;
							v.z = (float)*d++;
							n = PxVec3(0.0f, 0.0f, 0.0f);
							if (hasNormals) {
								n.x = (float)*d++;
								n.y = (float)*d++;
								n.z = (float)*d++;
							}
							tu = 0.0f; tv = 0.0f;
							if (hasTexCoords) {
								tu = (float)*d++;
								tv = (float)*d++;
							}
							mVisVertices.pushBack(scale * v);
							mVisNormals.pushBack(n);
							mVisTexCoords.pushBack(tu);
							mVisTexCoords.pushBack(tv);
							mVisTangents.pushBack(PxVec3(0.0f, 0.0f, 0.0f));		// no bumps on the surface
						}
					}
				}
				else {
					int size = numVerts * numScalars * sizeof(float);
					buffer.resize(size);
					p->getFileScanner().getBinaryData(&buffer[0], size);
					if (!ignoreVisualMesh) {
						float *f = (float*)&buffer[0];
						for (int i = 0; i < numVerts; i++) {
							v.x = *f++;
							v.y = *f++;
							v.z = *f++;
							n = PxVec3(0.0f, 0.0f, 0.0f);
							if (hasNormals) {
								n.x = *f++;
								n.y = *f++;
								n.z = *f++;
							}
							tu = 0.0f; tv = 0.0f;
							if (hasTexCoords) {
								tu = *f++;
								tv = *f++;
							}
							mVisVertices.pushBack(scale * v);
							mVisNormals.pushBack(n);
							mVisTexCoords.pushBack(tu);
							mVisTexCoords.pushBack(tv);
							mVisTangents.pushBack(PxVec3(0.0f, 0.0f, 0.0f));		// no bumps on the surface
						}
					}
				}
			}
			else {
				for (int i = 0; i < numVerts; i++) {
					PxVec3 v(0.0f, 0.0f, 0.0f);
					PxVec3 n(0.0f, 0.0f, 0.0f);
					PxVec3 t(0.0f, 0.0f, 0.0f);
					float tu = 0.0f;
					float tv = 0.0f;
					p->getFileScanner().getFloatSymbol(v.x);
					p->getFileScanner().getFloatSymbol(v.y);
					p->getFileScanner().getFloatSymbol(v.z);

					if (hasNormals) {
						p->getFileScanner().getFloatSymbol(n.x);
						p->getFileScanner().getFloatSymbol(n.y);
						p->getFileScanner().getFloatSymbol(n.z);
					}
					if (hasTexCoords) {
						p->getFileScanner().getFloatSymbol(tu);
						p->getFileScanner().getFloatSymbol(tv);
					}

					if (!ignoreVisualMesh) {
						mVisVertices.pushBack(scale * v);
						mVisNormals.pushBack(n);
						mVisTexCoords.pushBack(tu);
						mVisTexCoords.pushBack(tv);
						mVisTangents.pushBack(PxVec3(0.0f, 0.0f, 0.0f));		// no bumps on the surface
					}
				}
			}
			p->readNextTag();		// </VisualVertices>
			p->readNextTag();
		}
		else if (p->tagName == "VisualFaces") {
			int numFaces = 0;
			for (int i = 0; i < (int)p->vars.size(); i++) {
				if (p->vars[i].name == "count")
					sscanf(p->vars[i].value.c_str(), "%i", &numFaces);
			}
			for (int i = 0; i < numFaces; i++) {
				if (!ignoreVisualMesh)
					mVisPolyStarts.pushBack(mVisPolyIndices.size());
				int id;
				while (!p->endOfFile()) {
					p->getFileScanner().getIntSymbol(id);
					if (id < 0)
						break; 
					if (!ignoreVisualMesh)
						mVisPolyIndices.pushBack(id);
				}
			}
			if (!ignoreVisualMesh)
				mVisPolyStarts.pushBack(mVisPolyIndices.size());
			p->readNextTag();	// </VisualFaces>
			p->readNextTag();
		}
		else 
			p->readNextTag();
	}

	p->readNextTag();

	finalize();

	if (mHasExplicitVisMesh) {
		computeVisMeshNeighbors();
		createVisTrisFromPolys();
		computeVisTangentsFromPoly();
	}

	// check();	// foo

	return true;
}