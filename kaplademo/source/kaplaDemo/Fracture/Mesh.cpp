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

#include <foundation/PxBounds3.h>

#include "Mesh.h"
#include <algorithm>
#include <iostream>

// -------------------------------------------------------------------
struct SimpleVertexRef {
	int vert, normal, texCoord;
	int indexNr, subMeshNr;
	bool operator < (const SimpleVertexRef &r) const { 
		if (vert < r.vert) return true;
		if (vert > r.vert) return false;
		if (normal < r.normal) return true;
		if (normal > r.normal) return false;
		return texCoord < r.texCoord;
	}
	bool operator == (const SimpleVertexRef &r) const {
		return vert == r.vert && normal == r.normal && texCoord == r.texCoord;
	}
	void parse(char *s, int indexNr) {
		int nr[3] = {0,0,0};
		char *p = s;
		for (int i = 0; i < 3; i++) {
			while (*p != 0 && *p != '/') p++;
			bool end = (*p == 0);
			*p = 0;
			sscanf_s(s, "%i", &nr[i]); 
			if (end) break;
			p++; s = p;
		}
		vert = nr[0]-1; texCoord = nr[1]-1; normal = nr[2]-1;
		this->indexNr = indexNr;
	}
};

#define OBJ_STR_LEN 256
// -------------------------------------------------------------------
bool Mesh::loadFromObjFile(const std::string &filename)
{
	const int maxVertsPerFace = 8;

	char s[OBJ_STR_LEN], ps[maxVertsPerFace][OBJ_STR_LEN], sub[OBJ_STR_LEN];
	int matNr = -1;
	PxVec3 p;
	float u,v;
	shdfnd::Array<SimpleVertexRef> refs;
	SimpleVertexRef ref[maxVertsPerFace];

	shdfnd::Array<PxVec3> vertices;
	shdfnd::Array<PxVec3> normals;
	shdfnd::Array<float> texCoords;
	int numIndices = 0;

	mSubMeshes.resize(1);
	mNames.resize(1);
	mSubMeshes[0].init();
	mNames[0] = "";
	mSubMeshes[0].firstIndex = 0;

	FILE *f;
	
        if (fopen_s(&f, filename.c_str(), "r") != 0) {
                std::cerr << "Could not load OBJ file: " << filename << std::endl;
		return false;
        }

	// first a vertex ref is generated for each v/n/t combination
	while (!feof(f)) {
		if (fgets(s, OBJ_STR_LEN, f) == NULL) break;

		if (strncmp(s, "usemtl", 6) == 0 || strncmp(s, "g ", 2) == 0) {  // new group
			bool isGroup = strncmp(s, "g ", 2) == 0;
			if (isGroup)
				strcpy(sub, &s[2]);
			else
				strcpy(sub, &s[6]);
			int numSubs = mSubMeshes.size();			
			if (mSubMeshes[numSubs-1].numIndices > 0) {
				mSubMeshes.resize(numSubs+1);
				mNames.resize(numSubs+1);
				mSubMeshes[numSubs].init();
				mNames[numSubs] = "";
				mSubMeshes[numSubs].firstIndex = mIndices.size();
			}
			int subNr = mSubMeshes.size() - 1;
			if (mNames[subNr] == "" || isGroup)
				mNames[subNr] = std::string(sub);
		}
		else if (strncmp(s, "v ", 2) == 0) {	// vertex
			sscanf_s(s, "v %f %f %f", &p.x, &p.y, &p.z);
			vertices.pushBack(p);
		}
		else if (strncmp(s, "vn ", 3) == 0) {	// normal
			sscanf_s(s, "vn %f %f %f", &p.x, &p.y, &p.z);
			normals.pushBack(p);
		}
		else if (strncmp(s, "vt ", 3) == 0) {	// texture coords
			sscanf_s(s, "vt %f %f", &u, &v);
			texCoords.pushBack(u);
			texCoords.pushBack(v);
		}
		else if (strncmp(s, "f ", 2) == 0) {	// face, tri or quad
			int nr;
			nr = sscanf_s(s, "f %s %s %s %s %s %s", 
				ps[0], OBJ_STR_LEN, ps[1], OBJ_STR_LEN, ps[2], OBJ_STR_LEN, ps[3], OBJ_STR_LEN,
				 ps[4], OBJ_STR_LEN, ps[5], OBJ_STR_LEN, ps[6], OBJ_STR_LEN, ps[7], OBJ_STR_LEN);

			if (nr >= 3) {
				for (int i = 0; i < nr; i++) 
					ref[i].parse(ps[i], 0);
				for (int i = 1; i < nr-1; i++) {
					ref[0].indexNr = numIndices++; refs.pushBack(ref[0]); mIndices.pushBack(0);
					ref[i].indexNr = numIndices++; refs.pushBack(ref[i]); mIndices.pushBack(0);
					ref[i+1].indexNr = numIndices++; refs.pushBack(ref[i+1]); mIndices.pushBack(0);
					mSubMeshes[mSubMeshes.size()-1].numIndices += 3;
				}
			}
		}
	}
	fclose(f);

	// now we merge multiple v/n/t triplets
	std::sort(refs.begin(), refs.end());

	int i = 0;
	PxVec3 defNormal(1.0f, 0.0f, 0.0f);
	bool normalsOK = true;
	bool mTextured = true;
	int numTexCoords = texCoords.size();

	int baseVertNr = 0;

	while (i < (int)refs.size()) {
		int vertNr = mVertices.size();
		SimpleVertexRef &r = refs[i];
		mVertices.pushBack(vertices[r.vert]);

		if (r.normal >= 0) mNormals.pushBack(normals[r.normal]);
		else { mNormals.pushBack(defNormal); normalsOK = false; }

		if (r.texCoord >= 0 && r.texCoord < numTexCoords) {
			//mTexCoords.pushBack(texCoords[2*r.texCoord]);
			//mTexCoords.pushBack(texCoords[2*r.texCoord+1]);
			mTexCoords.pushBack(PxVec2(texCoords[2*r.texCoord],texCoords[2*r.texCoord+1]));
		}
		else { 
			//mTexCoords.pushBack(0.0f);
			//mTexCoords.pushBack(0.0f);
			mTexCoords.pushBack(PxVec2(0.0f,0.0f));
		}

		mIndices[r.indexNr] = vertNr;
		i++;
		while (i < (int)refs.size() && r == refs[i]) {
			mIndices[refs[i].indexNr] = vertNr;
			i++;
		}
	}

	if (!normalsOK)
		updateNormals();

	return true;
}