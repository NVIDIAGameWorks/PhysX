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

#include "RTdef.h"
#if RT_COMPILE
#include <foundation/PxBounds3.h>

#include "MeshBase.h"
#include <algorithm>

namespace physx
{
namespace fracture
{
namespace base
{

// ----------------------------------------------------------------------
void Mesh::updateNormals()
{
	if (mVertices.empty())
		return;

	PxVec3 zero(0.0f, 0.0f, 0.0f);

	mNormals.resize(mVertices.size());
	for (int i = 0; i < (int)mNormals.size(); i++) 
		mNormals[i] = zero;
	PxVec3 n;

	PxU32 *idx = &mIndices[0];
	int numTriangles = mIndices.size() / 3;
	for (int i = 0; i < numTriangles; i++) {
		PxU32 i0 = *idx++;
		PxU32 i1 = *idx++;
		PxU32 i2 = *idx++;
		n = (mVertices[i1] - mVertices[i0]).cross(mVertices[i2] - mVertices[i0]);
		mNormals[i0] += n;
		mNormals[i1] += n;
		mNormals[i2] += n;
	}
	for (int i = 0; i < (int)mNormals.size(); i++) {
		if (!mNormals[i].isZero())
			mNormals[i].normalize();
	}
}

// ------------------------------------------------------------------------------
void Mesh::getBounds(PxBounds3 &bounds, int subMeshNr) const
{
	bounds.setEmpty();
	if (subMeshNr < 0 || subMeshNr >= (int)mSubMeshes.size()) {
		for (int i = 0; i < (int)mVertices.size(); i++)
			bounds.include(mVertices[i]);
	}
	else {
		const SubMesh &sm = mSubMeshes[subMeshNr];
		for (int i = 0; i < sm.numIndices; i++) {
			bounds.include(mVertices[mIndices[sm.firstIndex + i]]);
		}
	}
}

// ------------------------------------------------------------------------------
void Mesh::normalize(const PxVec3 &center, float diagonal)
{
	if (mVertices.size() < 3)
		return;

	PxBounds3 bounds;
	getBounds(bounds);

	float s = diagonal / bounds.getDimensions().magnitude();
	PxVec3 c = 0.5f * (bounds.minimum + bounds.maximum);

	for (int i = 0; i < (int)mVertices.size(); i++)
		mVertices[i] = center + (mVertices[i] - c) * s;
}

// ------------------------------------------------------------------------------
void Mesh::scale(float diagonal)
{
	if (mVertices.size() < 3)
		return;

	PxBounds3 bounds;
	getBounds(bounds);

	float s = diagonal / bounds.getDimensions().magnitude();
	for (int i = 0; i < (int)mVertices.size(); i++)
		mVertices[i] *= s;
}

// --------------------------------------------------------------------------------------------
struct IdEdge {
	void set(PxU32 &i0, PxU32 &i1, int faceNr, int edgeNr) {
		if (i0 < i1) { this->i0 = i0; this->i1 = i1; } 
		else { this->i0 = i1; this->i1 = i0; }
		this->faceNr = faceNr; this->edgeNr = edgeNr;
	}
	bool operator < (const IdEdge &e) const {
		if (i0 < e.i0) return true;
		if (i0 == e.i0 && i1 < e.i1) return true;
		return false;
	}
	bool operator == (const IdEdge &e) const { return i0 == e.i0 && i1 == e.i1; }
	PxU32 i0,i1;
	int faceNr, edgeNr;
};

// ------------------------------------------------------------------------------
bool Mesh::computeNeighbors()
{
	int numTris = mIndices.size() / 3;

	shdfnd::Array<IdEdge> edges(3*numTris);

	for (int i = 0; i < numTris; i++) {
		for (int j = 0; j < 3; j++) 
			edges[3*i+j].set(mIndices[3*i+j], mIndices[3*i + (j+1)%3], i, j);
	}
	std::sort(edges.begin(), edges.end());

	mNeighbors.clear();
	mNeighbors.resize(mIndices.size(), -1);
	bool manifold = true;
	int i = 0;
	while (i < (int)edges.size()) {
		IdEdge &e0 = edges[i];
		i++;
		if (i < (int)edges.size() && edges[i] == e0) {
			IdEdge &e1 = edges[i];
			mNeighbors[3* e0.faceNr + e0.edgeNr] = e1.faceNr;
			mNeighbors[3* e1.faceNr + e1.edgeNr] = e0.faceNr;
			i++;
		}
		while (i < (int)edges.size() && edges[i] == e0) {
			manifold = false;
			i++;
		}
	}
	return manifold;
}

// --------------------------------------------------------------------------------------------
struct PosEdge {
	// not using indices for edge match but positions
	// so duplicated vertices are handled as one vertex
	static bool smaller(const PxVec3 &v0, const PxVec3 &v1) {
		if (v0.x < v1.x) return true;
		if (v0.x > v1.x) return false;
		if (v0.y < v1.y) return true;
		if (v0.y > v1.y) return false;
		return (v0.z < v1.z);
	}
	void set(const PxVec3 &v0, const PxVec3 &v1, int faceNr, int edgeNr) {
		if (smaller(v0,v1)) { this->v0 = v0; this->v1 = v1; } 
		else { this->v0 = v1; this->v1 = v0; }
		this->faceNr = faceNr; this->edgeNr = edgeNr;
	}
	bool operator < (const PosEdge &e) const {
		if (smaller(v0, e.v0)) return true;
		if (v0 == e.v0 && smaller(v1, e.v1)) return true;
		return false;
	}
	bool operator == (const PosEdge &e) const { return v0 == e.v0 && v1 == e.v1; }
	PxVec3 v0,v1;
	int faceNr, edgeNr;
};

// ------------------------------------------------------------------------------
bool Mesh::computeWeldedNeighbors()
{
	int numTris = mIndices.size() / 3;
	shdfnd::Array<PosEdge> edges(3*numTris);

	for (int i = 0; i < numTris; i++) {
		for (int j = 0; j < 3; j++) 
			edges[3*i+j].set(mVertices[mIndices[3*i+j]], mVertices[mIndices[3*i + (j+1)%3]], i, j);
	}
	std::sort(edges.begin(), edges.end());

	mNeighbors.clear();
	mNeighbors.resize(mIndices.size(), -1);
	bool manifold = true;
	int i = 0;
	while (i < (int)edges.size()) {
		PosEdge &e0 = edges[i];
		i++;
		if (i < (int)edges.size() && edges[i] == e0) {
			PosEdge &e1 = edges[i];
			mNeighbors[3* e0.faceNr + e0.edgeNr] = e1.faceNr;
			mNeighbors[3* e1.faceNr + e1.edgeNr] = e0.faceNr;
			i++;
		}
		while (i < (int)edges.size() && edges[i] == e0) {
			manifold = false;
			i++;
		}
	}
	return manifold;
}

void Mesh::flipV()
{
	for(PxU32 i = 0; i < mTexCoords.size(); i++)
	{
		mTexCoords[i].y = 1.0f - mTexCoords[i].y;
	}
}

}
}
}
#endif