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
#include <PxBounds3.h>

#include "MeshBase.h"
#include <algorithm>

namespace nvidia
{
namespace fracture
{
namespace base
{

using namespace nvidia;

// ----------------------------------------------------------------------
void Mesh::updateNormals()
{
	if (mVertices.empty())
		return;

	PxVec3 zero(0.0f, 0.0f, 0.0f);

	mNormals.resize(mVertices.size());
	for (uint32_t i = 0; i < mNormals.size(); i++) 
		mNormals[i] = zero;
	PxVec3 n;

	uint32_t *idx = &mIndices[0];
	uint32_t numTriangles = mIndices.size() / 3;
	for (uint32_t i = 0; i < numTriangles; i++) {
		uint32_t i0 = *idx++;
		uint32_t i1 = *idx++;
		uint32_t i2 = *idx++;
		n = (mVertices[i1] - mVertices[i0]).cross(mVertices[i2] - mVertices[i0]);
		mNormals[i0] += n;
		mNormals[i1] += n;
		mNormals[i2] += n;
	}
	for (uint32_t i = 0; i < mNormals.size(); i++) {
		if (!mNormals[i].isZero())
			mNormals[i].normalize();
	}
}

// ------------------------------------------------------------------------------
void Mesh::getBounds(PxBounds3 &bounds, int subMeshNr) const
{
	bounds.setEmpty();
	if (subMeshNr < 0 || subMeshNr >= (int)mSubMeshes.size()) {
		for (uint32_t i = 0; i < mVertices.size(); i++)
			bounds.include(mVertices[i]);
	}
	else {
		const SubMesh &sm = mSubMeshes[(uint32_t)subMeshNr];
		for (int i = 0; i < sm.numIndices; i++) {
			bounds.include(mVertices[(uint32_t)mIndices[uint32_t(sm.firstIndex + i)]]);
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

	for (uint32_t i = 0; i < mVertices.size(); i++)
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
	for (uint32_t i = 0; i < mVertices.size(); i++)
		mVertices[i] *= s;
}

// --------------------------------------------------------------------------------------------
struct IdEdge {
	void set(uint32_t &i0, uint32_t &i1, int faceNr, int edgeNr) {
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
	uint32_t i0,i1;
	int faceNr, edgeNr;
};

// ------------------------------------------------------------------------------
bool Mesh::computeNeighbors()
{
	uint32_t numTris = mIndices.size() / 3;

	nvidia::Array<IdEdge> edges(3*numTris);

	for (uint32_t i = 0; i < numTris; i++) {
		for (uint32_t j = 0; j < 3; j++) 
			edges[3*i+j].set(mIndices[3*i+j], mIndices[3*i + (j+1)%3], (int32_t)i, (int32_t)j);
	}
	std::sort(edges.begin(), edges.end());

	mNeighbors.clear();
	mNeighbors.resize(mIndices.size(), -1);
	bool manifold = true;
	uint32_t i = 0;
	while (i < edges.size()) {
		IdEdge &e0 = edges[i];
		i++;
		if (i < edges.size() && edges[i] == e0) {
			IdEdge &e1 = edges[i];
			mNeighbors[uint32_t(3* e0.faceNr + e0.edgeNr)] = e1.faceNr;
			mNeighbors[uint32_t(3* e1.faceNr + e1.edgeNr)] = e0.faceNr;
			i++;
		}
		while (i < edges.size() && edges[i] == e0) {
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
	uint32_t numTris = mIndices.size() / 3;
	nvidia::Array<PosEdge> edges(3*numTris);

	for (uint32_t i = 0; i < numTris; i++) {
		for (uint32_t j = 0; j < 3; j++) 
			edges[3*i+j].set(mVertices[(uint32_t)mIndices[3*i+j]], mVertices[(uint32_t)mIndices[3*i + (j+1)%3]], (int32_t)i, (int32_t)j);
	}
	std::sort(edges.begin(), edges.end());

	mNeighbors.clear();
	mNeighbors.resize(mIndices.size(), -1);
	bool manifold = true;
	uint32_t i = 0;
	while (i < edges.size()) {
		PosEdge &e0 = edges[i];
		i++;
		if (i < edges.size() && edges[i] == e0) {
			PosEdge &e1 = edges[i];
			mNeighbors[uint32_t(3* e0.faceNr + e0.edgeNr)] = e1.faceNr;
			mNeighbors[uint32_t(3* e1.faceNr + e1.edgeNr)] = e0.faceNr;
			i++;
		}
		while (i < edges.size() && edges[i] == e0) {
			manifold = false;
			i++;
		}
	}
	return manifold;
}

void Mesh::flipV()
{
	for(uint32_t i = 0; i < mTexCoords.size(); i++)
	{
		mTexCoords[i].y = 1.0f - mTexCoords[i].y;
	}
}

}
}
}
#endif