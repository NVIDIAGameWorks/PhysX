/* Copyright (c) <2003-2011> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __dgPolyhedra__
#define __dgPolyhedra__

#include "dgTypes.h"
#include "dgList.h"
#include "dgTree.h"
#include "dgHeap.h"


class dgEdge;
class dgPlane;
class dgSphere;
class dgMatrix;
class dgPolyhedra;


typedef int64_t dgEdgeKey;

class dgEdge
{
	public:
	dgEdge ();
	dgEdge (int32_t vertex, int32_t face, uint64_t userdata = 0);

	int32_t m_incidentVertex;
	int32_t m_incidentFace;
	uint64_t m_userData;
	dgEdge* m_next;
	dgEdge* m_prev;
	dgEdge* m_twin;
	int32_t m_mark;
};


class dgPolyhedra: public dgTree <dgEdge, dgEdgeKey>
{
	public:

	struct dgPairKey
	{
		dgPairKey ();
		dgPairKey (int64_t val);
		dgPairKey (int32_t v0, int32_t v1);
		int64_t GetVal () const; 
		int32_t GetLowKey () const;
		int32_t GetHighKey () const;

		private:
		uint64_t m_key;
	};

	dgPolyhedra (void);
	dgPolyhedra (const dgPolyhedra &polyhedra);
	virtual ~dgPolyhedra();

	void BeginFace();
	dgEdge* AddFace (int32_t v0, int32_t v1, int32_t v2);
	dgEdge* AddFace (int32_t count, const int32_t* const index);
	dgEdge* AddFace (int32_t count, const int32_t* const index, const int64_t* const userdata);
	void EndFace ();
	void DeleteFace(dgEdge* const edge);

	int32_t GetFaceCount() const;
	int32_t GetEdgeCount() const;
	int32_t GetLastVertexIndex() const;

	int32_t IncLRU() const;
	void SetLRU(int32_t lru) const;

	dgEdge* FindEdge (int32_t v0, int32_t v1) const;
	dgTreeNode* FindEdgeNode (int32_t v0, int32_t v1) const;

	dgEdge* AddHalfEdge (int32_t v0, int32_t v1);
	void DeleteEdge (dgEdge* const edge);
	void DeleteEdge (int32_t v0, int32_t v1);
	
	bool FlipEdge (dgEdge* const edge);
	dgEdge* SpliteEdge (int32_t newIndex, dgEdge* const edge);
	dgBigVector FaceNormal (dgEdge* const face, const double* const vertex, int32_t strideInBytes) const;

	void BeginConectedSurface() const;
	bool GetConectedSurface (dgPolyhedra &polyhedra) const;
	void EndConectedSurface() const;

	dgSphere CalculateSphere (const double* const vertex, int32_t strideInBytes, const dgMatrix* const basis = NULL) const;
	void ChangeEdgeIncidentVertex (dgEdge* const edge, int32_t newIndex);	
	void DeleteDegenerateFaces (const double* const pool, int32_t dstStrideInBytes, double minArea);

	void Optimize (const double* const pool, int32_t strideInBytes, double tol);
	void Triangulate (const double* const vertex, int32_t strideInBytes, dgPolyhedra* const leftOversOut);
	void ConvexPartition (const double* const vertex, int32_t strideInBytes, dgPolyhedra* const leftOversOut);

	private:
	
	void RefineTriangulation (const double* const vertex, int32_t stride);
	void RefineTriangulation (const double* const vertex, int32_t stride, dgBigVector* const normal, int32_t perimeterCount, dgEdge** const perimeter);
	void OptimizeTriangulation (const double* const vertex, int32_t strideInBytes);
	void MarkAdjacentCoplanarFaces (dgPolyhedra& polyhedraOut, dgEdge* const face, const double* const pool, int32_t strideInBytes);
	dgEdge* FindEarTip (dgEdge* const face, const double* const pool, int32_t stride, dgDownHeap<dgEdge*, double>& heap, const dgBigVector &normal) const;
	dgEdge* TriangulateFace (dgEdge* face, const double* const pool, int32_t stride, dgDownHeap<dgEdge*, double>& heap, dgBigVector* const faceNormalOut);
	
	double EdgePenalty (const dgBigVector* const pool, dgEdge* const edge) const;

	mutable int32_t m_baseMark;
	mutable int32_t m_edgeMark;
	mutable int32_t m_faceSecuence;

	friend class dgPolyhedraDescriptor;
	
};



inline dgEdge::dgEdge ()
{
}

inline dgEdge::dgEdge (int32_t vertex, int32_t face, uint64_t userdata)
	:m_incidentVertex(vertex)
	,m_incidentFace(face)
	,m_userData(userdata)
	,m_next(NULL)
	,m_prev(NULL)
	,m_twin(NULL)
	,m_mark(0)
{
}


inline dgPolyhedra::dgPairKey::dgPairKey ()
{
}

inline dgPolyhedra::dgPairKey::dgPairKey (int64_t val)
	:m_key(uint64_t (val))
{
}

inline dgPolyhedra::dgPairKey::dgPairKey (int32_t v0, int32_t v1)
	:m_key (uint64_t ((int64_t (v0) << 32) | v1))
{
}

inline int64_t dgPolyhedra::dgPairKey::GetVal () const 
{
	return int64_t (m_key);
}

inline int32_t dgPolyhedra::dgPairKey::GetLowKey () const 
{
	return int32_t (m_key>>32);
}

inline int32_t dgPolyhedra::dgPairKey::GetHighKey () const 
{
	return int32_t (m_key & 0xffffffff);
}

inline void dgPolyhedra::BeginFace ()
{
}

inline dgEdge* dgPolyhedra::AddFace (int32_t count, const int32_t* const index) 
{
	return AddFace (count, index, NULL);
}

inline dgEdge* dgPolyhedra::AddFace (int32_t v0, int32_t v1, int32_t v2)
{
	int32_t vertex[3];

	vertex [0] = v0;
	vertex [1] = v1;
	vertex [2] = v2;
	return AddFace (3, vertex, NULL);
}

inline int32_t dgPolyhedra::GetEdgeCount() const
{
#ifdef _DEBUG
	int32_t edgeCount = 0;
	Iterator iter(*this);
	for (iter.Begin(); iter; iter ++) {
		edgeCount ++;
	}
	HACD_ASSERT (edgeCount == GetCount());;
#endif
	return GetCount();
}

inline int32_t dgPolyhedra::GetLastVertexIndex() const
{
	int32_t maxVertexIndex = -1;
	Iterator iter(*this);
	for (iter.Begin(); iter; iter ++) {
		const dgEdge* const edge = &(*iter);
		if (edge->m_incidentVertex > maxVertexIndex) {
			maxVertexIndex = edge->m_incidentVertex;
		}
	}
	return maxVertexIndex + 1;
}


inline int32_t dgPolyhedra::IncLRU() const
{	
	m_edgeMark ++;
	HACD_ASSERT (m_edgeMark < 0x7fffffff);
	return m_edgeMark;
}

inline void dgPolyhedra::SetLRU(int32_t lru) const
{
	if (lru > m_edgeMark) {
		m_edgeMark = lru;
	}
}

inline void dgPolyhedra::BeginConectedSurface() const
{
	m_baseMark = IncLRU();
}

inline void dgPolyhedra::EndConectedSurface() const
{
}


inline dgPolyhedra::dgTreeNode* dgPolyhedra::FindEdgeNode (int32_t i0, int32_t i1) const
{
	dgPairKey key (i0, i1);
	return Find (key.GetVal());
}


inline dgEdge *dgPolyhedra::FindEdge (int32_t i0, int32_t i1) const
{
	//	dgTreeNode *node;
	//	dgPairKey key (i0, i1);
	//	node = Find (key.GetVal());
	//	return node ? &node->GetInfo() : NULL;
	dgTreeNode* const node = FindEdgeNode (i0, i1);
	return node ? &node->GetInfo() : NULL;
}

inline void dgPolyhedra::DeleteEdge (int32_t v0, int32_t v1)
{
	dgPairKey pairKey (v0, v1);
	dgTreeNode* const node = Find(pairKey.GetVal());
	dgEdge* const edge = node ? &node->GetInfo() : NULL;
	if (!edge) {
		return;
	}
	DeleteEdge (edge);
}


#endif

