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

#include "dgTypes.h"
#include "dgHeap.h"
#include "dgStack.h"
#include "dgSphere.h"
#include "dgPolyhedra.h"
#include "dgConvexHull3d.h"
#include "dgSmallDeterminant.h"
#include <string.h>


//#define DG_MIN_EDGE_ASPECT_RATIO  double (0.02f)

class dgDiagonalEdge
{
	public:
	dgDiagonalEdge (dgEdge* const edge)
		:m_i0(edge->m_incidentVertex), m_i1(edge->m_twin->m_incidentVertex)
	{
	}
	int32_t m_i0;
	int32_t m_i1;
};


struct dgEdgeCollapseEdgeHandle
{
	dgEdgeCollapseEdgeHandle (dgEdge* const newEdge)
		:m_inList(false), m_edge(newEdge)
	{
	}

	dgEdgeCollapseEdgeHandle (const dgEdgeCollapseEdgeHandle &dataHandle)
		:m_inList(true), m_edge(dataHandle.m_edge)
	{
		dgEdgeCollapseEdgeHandle* const handle = (dgEdgeCollapseEdgeHandle *)IntToPointer (m_edge->m_userData);
		if (handle) {
			HACD_ASSERT (handle != this);
			handle->m_edge = NULL;
		}
		m_edge->m_userData = uint64_t (PointerToInt(this));
	}

	~dgEdgeCollapseEdgeHandle ()
	{
		if (m_inList) {
			if (m_edge) {
				dgEdgeCollapseEdgeHandle* const handle = (dgEdgeCollapseEdgeHandle *)IntToPointer (m_edge->m_userData);
				if (handle == this) {
					m_edge->m_userData = PointerToInt (NULL);
				}
			}
		}
		m_edge = NULL;
	}

	uint32_t m_inList;
	dgEdge* m_edge;
};


class dgVertexCollapseVertexMetric
{
	public:
	double elem[10];

	dgVertexCollapseVertexMetric (const dgBigPlane &plane) 
	{
		elem[0] = plane.m_x * plane.m_x;  
		elem[1] = plane.m_y * plane.m_y;  
		elem[2] = plane.m_z * plane.m_z;  
		elem[3] = plane.m_w * plane.m_w;  
		elem[4] = double (2.0) * plane.m_x * plane.m_y;  
		elem[5] = double (2.0) * plane.m_x * plane.m_z;  
		elem[6] = double (2.0) * plane.m_x * plane.m_w;  
		elem[7] = double (2.0) * plane.m_y * plane.m_z;  
		elem[8] = double (2.0) * plane.m_y * plane.m_w;  
		elem[9] = double (2.0) * plane.m_z * plane.m_w;  
	}

	void Clear ()
	{
		memset (elem, 0, 10 * sizeof (double));
	}

	void Accumulate (const dgVertexCollapseVertexMetric& p) 
	{
		elem[0] += p.elem[0]; 
		elem[1] += p.elem[1]; 
		elem[2] += p.elem[2]; 
		elem[3] += p.elem[3]; 
		elem[4] += p.elem[4]; 
		elem[5] += p.elem[5]; 
		elem[6] += p.elem[6]; 
		elem[7] += p.elem[7]; 
		elem[8] += p.elem[8]; 
		elem[9] += p.elem[9]; 
	}

	void Accumulate (const dgBigPlane& plane) 
	{
		elem[0] += plane.m_x * plane.m_x;  
		elem[1] += plane.m_y * plane.m_y;  
		elem[2] += plane.m_z * plane.m_z;  
		elem[3] += plane.m_w * plane.m_w;  

		elem[4] += double (2.0f) * plane.m_x * plane.m_y;  
		elem[5] += double (2.0f) * plane.m_x * plane.m_z;  
		elem[7] += double (2.0f) * plane.m_y * plane.m_z;  

		elem[6] += double (2.0f) * plane.m_x * plane.m_w;  
		elem[8] += double (2.0f) * plane.m_y * plane.m_w;  
		elem[9] += double (2.0f) * plane.m_z * plane.m_w;  
	}


	double Evalue (const dgVector &p) const 
	{
		double acc = elem[0] * p.m_x * p.m_x + elem[1] * p.m_y * p.m_y + elem[2] * p.m_z * p.m_z + 
						elem[4] * p.m_x * p.m_y + elem[5] * p.m_x * p.m_z + elem[7] * p.m_y * p.m_z + 
						elem[6] * p.m_x + elem[8] * p.m_y + elem[9] * p.m_z + elem[3];  
		return fabs (acc);
	}
};



dgPolyhedra::dgPolyhedra (void)
	:dgTree <dgEdge, int64_t>()
	,m_baseMark(0)
	,m_edgeMark(0)
	,m_faceSecuence(0)
{
}

dgPolyhedra::dgPolyhedra (const dgPolyhedra &polyhedra)
	:dgTree <dgEdge, int64_t>()
	,m_baseMark(0)
	,m_edgeMark(0)
	,m_faceSecuence(0)
{
	dgStack<int32_t> indexPool (1024 * 16);
	dgStack<uint64_t> userPool (1024 * 16);
	int32_t* const index = &indexPool[0];
	uint64_t* const user = &userPool[0];

	BeginFace ();
	Iterator iter(polyhedra);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if (edge->m_incidentFace < 0) {
			continue;
		}

		if (!FindEdge(edge->m_incidentVertex, edge->m_twin->m_incidentVertex))	{
			int32_t indexCount = 0;
			dgEdge* ptr = edge;
			do {
				user[indexCount] = ptr->m_userData;
				index[indexCount] = ptr->m_incidentVertex;
				indexCount ++;
				ptr = ptr->m_next;
			} while (ptr != edge);

			dgEdge* const face = AddFace (indexCount, index, (int64_t*) user);
			ptr = face;
			do {
				ptr->m_incidentFace = edge->m_incidentFace;
				ptr = ptr->m_next;
			} while (ptr != face);
		}
	}
	EndFace();

	m_faceSecuence = polyhedra.m_faceSecuence;

#ifdef __ENABLE_SANITY_CHECK 
	HACD_ASSERT (SanityCheck());
#endif
}

dgPolyhedra::~dgPolyhedra ()
{
}


int32_t dgPolyhedra::GetFaceCount() const
{
	Iterator iter (*this);
	int32_t count = 0;
	int32_t mark = IncLRU();
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if (edge->m_mark == mark) {
			continue;
		}

		if (edge->m_incidentFace < 0) {
			continue;
		}

		count ++;
		dgEdge* ptr = edge;
		do {
			ptr->m_mark = mark;
			ptr = ptr->m_next;
		} while (ptr != edge);
	}
	return count;
}



dgEdge* dgPolyhedra::AddFace ( int32_t count, const int32_t* const index, const int64_t* const userdata)
{
	class IntersectionFilter
	{
		public:
		IntersectionFilter ()
		{
			m_count = 0;
		}

		bool Insert (int32_t /*dummy*/, int64_t value)
		{
			int32_t i;				
			for (i = 0 ; i < m_count; i ++) {
				if (m_array[i] == value) {
					return false;
				}
			}
			m_array[i] = value;
			m_count ++;
			return true;
		}

		int32_t m_count;
		int64_t m_array[2048];
	};

	IntersectionFilter selfIntersectingFaceFilter;

	int32_t dummyValues = 0;
	int32_t i0 = index[count-1];
	for (int32_t i = 0; i < count; i ++) {
		int32_t i1 = index[i];
		dgPairKey code0 (i0, i1);

		if (!selfIntersectingFaceFilter.Insert (dummyValues, code0.GetVal())) {
			return NULL;
		}

		dgPairKey code1 (i1, i0);
		if (!selfIntersectingFaceFilter.Insert (dummyValues, code1.GetVal())) {
			return NULL;
		}


		if (i0 == i1) {
			return NULL;
		}
		if (FindEdge (i0, i1)) {
			return NULL;
		}
		i0 = i1;
	}

	m_faceSecuence ++;

	i0 = index[count-1];
	int32_t i1 = index[0];
	uint64_t udata0 = 0;
	uint64_t udata1 = 0;
	if (userdata) {
		udata0 = uint64_t (userdata[count-1]);
		udata1 = uint64_t (userdata[0]);
	} 

	bool state;
	dgPairKey code (i0, i1);
	dgEdge tmpEdge (i0, m_faceSecuence, udata0);
	dgTreeNode* node = Insert (tmpEdge, code.GetVal(), state); 
	HACD_ASSERT (!state);
	dgEdge* edge0 = &node->GetInfo();
	dgEdge* const first = edge0;

	for (int32_t i = 1; i < count; i ++) {
		i0 = i1;
		i1 = index[i];
		udata0 = udata1;
		udata1 = uint64_t (userdata ? userdata[i] : 0);

		dgPairKey code (i0, i1);
		dgEdge tmpEdge (i0, m_faceSecuence, udata0);
		node = Insert (tmpEdge, code.GetVal(), state); 
		HACD_ASSERT (!state);

		dgEdge* const edge1 = &node->GetInfo();
		edge0->m_next = edge1;
		edge1->m_prev = edge0;
		edge0 = edge1;
	}

	first->m_prev = edge0;
	edge0->m_next = first;

	return first->m_next;
}


void dgPolyhedra::EndFace ()
{
	dgPolyhedra::Iterator iter (*this);

	// Connect all twin edge
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if (!edge->m_twin) {
			edge->m_twin = FindEdge (edge->m_next->m_incidentVertex, edge->m_incidentVertex);
			if (edge->m_twin) {
				edge->m_twin->m_twin = edge; 
			}
		}
	}

#ifdef __ENABLE_SANITY_CHECK 
	HACD_ASSERT (SanityCheck());
#endif
	dgStack<dgEdge*> edgeArrayPool(GetCount() * 2 + 256);

	int32_t edgeCount = 0;
	dgEdge** const edgeArray = &edgeArrayPool[0];
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if (!edge->m_twin) {
			bool state;
			dgPolyhedra::dgPairKey code (edge->m_next->m_incidentVertex, edge->m_incidentVertex);
			dgEdge tmpEdge (edge->m_next->m_incidentVertex, -1);
			tmpEdge.m_incidentFace = -1; 
			dgPolyhedra::dgTreeNode* const node = Insert (tmpEdge, code.GetVal(), state); 
			HACD_ASSERT (!state);
			edge->m_twin = &node->GetInfo();
			edge->m_twin->m_twin = edge; 
			edgeArray[edgeCount] = edge->m_twin;
			edgeCount ++;
		}
	}

	for (int32_t i = 0; i < edgeCount; i ++) {
		dgEdge* const edge = edgeArray[i];
		HACD_ASSERT (!edge->m_prev);
		dgEdge *ptr = edge->m_twin;
		for (; ptr->m_next; ptr = ptr->m_next->m_twin){}
		ptr->m_next = edge;
		edge->m_prev = ptr;
	}

#ifdef __ENABLE_SANITY_CHECK 
	HACD_ASSERT (SanityCheck ());
#endif
}


void dgPolyhedra::DeleteFace(dgEdge* const face)
{
	dgEdge* edgeList[1024 * 16];

	if (face->m_incidentFace > 0) {
		int32_t count = 0;
		dgEdge* ptr = face;
		do {
			ptr->m_incidentFace = -1;
			int32_t i = 0;
			for (; i < count; i ++) {
				if ((edgeList[i] == ptr) || (edgeList[i]->m_twin == ptr)) {
					break;
				}
			}
			if (i == count) {
				edgeList[count] = ptr;
				count ++;
			}
			ptr = ptr->m_next;
		} while (ptr != face);


		for (int32_t i = 0; i < count; i ++) {
			dgEdge* const ptr = edgeList[i];
			if (ptr->m_twin->m_incidentFace < 0) {
				DeleteEdge (ptr);
			}
		}
	}
}



dgBigVector dgPolyhedra::FaceNormal (dgEdge* const face, const double* const pool, int32_t strideInBytes) const
{
	int32_t stride = strideInBytes / (int32_t)sizeof (double);
	dgEdge* edge = face;
	dgBigVector p0 (&pool[edge->m_incidentVertex * stride]);
	edge = edge->m_next;
	dgBigVector p1 (&pool[edge->m_incidentVertex * stride]);
	dgBigVector e1 (p1 - p0);

	dgBigVector normal (float (0.0f), float (0.0f), float (0.0f), float (0.0f));
	for (edge = edge->m_next; edge != face; edge = edge->m_next) {
		dgBigVector p2 (&pool[edge->m_incidentVertex * stride]);
		dgBigVector e2 (p2 - p0);
		normal += e1 * e2;
		e1 = e2;
	} 
	return normal;
}


dgEdge* dgPolyhedra::AddHalfEdge (int32_t v0, int32_t v1)
{
	if (v0 != v1) {
		dgPairKey pairKey (v0, v1);
		dgEdge tmpEdge (v0, -1);

		dgTreeNode* node = Insert (tmpEdge, pairKey.GetVal()); 
		return node ? &node->GetInfo() : NULL;
	} else {
		return NULL;
	}
}


void dgPolyhedra::DeleteEdge (dgEdge* const edge)
{
	dgEdge *const twin = edge->m_twin;

	edge->m_prev->m_next = twin->m_next;
	twin->m_next->m_prev = edge->m_prev;
	edge->m_next->m_prev = twin->m_prev;
	twin->m_prev->m_next = edge->m_next;

	dgTreeNode *const nodeA = GetNodeFromInfo (*edge);
	dgTreeNode *const nodeB = GetNodeFromInfo (*twin);

	HACD_ASSERT (&nodeA->GetInfo() == edge);
	HACD_ASSERT (&nodeB->GetInfo() == twin);

	Remove (nodeA);
	Remove (nodeB);
}


dgEdge* dgPolyhedra::SpliteEdge (int32_t newIndex,	dgEdge* const edge)
{
	dgEdge* const edge00 = edge->m_prev;
	dgEdge* const edge01 = edge->m_next;
	dgEdge* const twin00 = edge->m_twin->m_next;
	dgEdge* const twin01 = edge->m_twin->m_prev;

	int32_t i0 = edge->m_incidentVertex;
	int32_t i1 = edge->m_twin->m_incidentVertex;

	int32_t f0 = edge->m_incidentFace;
	int32_t f1 = edge->m_twin->m_incidentFace;

	DeleteEdge (edge);

	dgEdge* const edge0 = AddHalfEdge (i0, newIndex);
	dgEdge* const edge1 = AddHalfEdge (newIndex, i1);

	dgEdge* const twin0 = AddHalfEdge (newIndex, i0);
	dgEdge* const twin1 = AddHalfEdge (i1, newIndex);
	HACD_ASSERT (edge0);
	HACD_ASSERT (edge1);
	HACD_ASSERT (twin0);
	HACD_ASSERT (twin1);

	edge0->m_twin = twin0;
	twin0->m_twin = edge0;

	edge1->m_twin = twin1;
	twin1->m_twin = edge1;

	edge0->m_next = edge1;
	edge1->m_prev = edge0;

	twin1->m_next = twin0;
	twin0->m_prev = twin1;

	edge0->m_prev = edge00;
	edge00 ->m_next = edge0;

	edge1->m_next = edge01;
	edge01->m_prev = edge1;

	twin0->m_next = twin00;
	twin00->m_prev = twin0;

	twin1->m_prev = twin01;
	twin01->m_next = twin1;

	edge0->m_incidentFace = f0;
	edge1->m_incidentFace = f0;

	twin0->m_incidentFace = f1;
	twin1->m_incidentFace = f1;

#ifdef __ENABLE_SANITY_CHECK 
	//	HACD_ASSERT (SanityCheck ());
#endif

	return edge0;
}



bool dgPolyhedra::FlipEdge (dgEdge* const edge)
{
	//	dgTreeNode *node;
	if (edge->m_next->m_next->m_next != edge) {
		return false;
	}

	if (edge->m_twin->m_next->m_next->m_next != edge->m_twin) {
		return false;
	}

	if (FindEdge(edge->m_prev->m_incidentVertex, edge->m_twin->m_prev->m_incidentVertex)) {
		return false;
	}

	dgEdge *const prevEdge = edge->m_prev;
	dgEdge *const prevTwin = edge->m_twin->m_prev;

	dgPairKey edgeKey (prevTwin->m_incidentVertex, prevEdge->m_incidentVertex);
	dgPairKey twinKey (prevEdge->m_incidentVertex, prevTwin->m_incidentVertex);

	ReplaceKey (GetNodeFromInfo (*edge), edgeKey.GetVal());
	//	HACD_ASSERT (node);

	ReplaceKey (GetNodeFromInfo (*edge->m_twin), twinKey.GetVal());
	//	HACD_ASSERT (node);

	edge->m_incidentVertex = prevTwin->m_incidentVertex;
	edge->m_twin->m_incidentVertex = prevEdge->m_incidentVertex;

	edge->m_userData = prevTwin->m_userData;
	edge->m_twin->m_userData = prevEdge->m_userData;

	prevEdge->m_next = edge->m_twin->m_next;
	prevTwin->m_prev->m_prev = edge->m_prev;

	prevTwin->m_next = edge->m_next;
	prevEdge->m_prev->m_prev = edge->m_twin->m_prev;

	edge->m_prev = prevTwin->m_prev;
	edge->m_next = prevEdge;

	edge->m_twin->m_prev = prevEdge->m_prev;
	edge->m_twin->m_next = prevTwin;

	prevTwin->m_prev->m_next = edge;
	prevTwin->m_prev = edge->m_twin;

	prevEdge->m_prev->m_next = edge->m_twin;
	prevEdge->m_prev = edge;

	edge->m_next->m_incidentFace = edge->m_incidentFace;
	edge->m_prev->m_incidentFace = edge->m_incidentFace;

	edge->m_twin->m_next->m_incidentFace = edge->m_twin->m_incidentFace;
	edge->m_twin->m_prev->m_incidentFace = edge->m_twin->m_incidentFace;


#ifdef __ENABLE_SANITY_CHECK 
	HACD_ASSERT (SanityCheck ());
#endif

	return true;
}



bool dgPolyhedra::GetConectedSurface (dgPolyhedra &polyhedra) const
{
	if (!GetCount()) {
		return false;
	}

	dgEdge* edge = NULL;
	Iterator iter(*this);
	for (iter.Begin (); iter; iter ++) {
		edge = &(*iter);
		if ((edge->m_mark < m_baseMark) && (edge->m_incidentFace > 0)) {
			break;
		}
	}

	if (!iter) {
		return false;
	}

	int32_t faceIndex[4096];
	int64_t faceDataIndex[4096];
	dgStack<dgEdge*> stackPool (GetCount()); 
	dgEdge** const stack = &stackPool[0];

	int32_t mark = IncLRU();

	polyhedra.BeginFace ();
	stack[0] = edge;
	int32_t index = 1;
	while (index) {
		index --;
		dgEdge* const edge = stack[index];

		if (edge->m_mark == mark) {
			continue;
		}

		int32_t count = 0;
		dgEdge* ptr = edge;
		do {
			ptr->m_mark = mark;
			faceIndex[count] = ptr->m_incidentVertex;
			faceDataIndex[count] = int64_t (ptr->m_userData);
			count ++;
			HACD_ASSERT (count <  int32_t ((sizeof (faceIndex)/sizeof(faceIndex[0]))));

			if ((ptr->m_twin->m_incidentFace > 0) && (ptr->m_twin->m_mark != mark)) {
				stack[index] = ptr->m_twin;
				index ++;
				HACD_ASSERT (index < GetCount());
			}

			ptr = ptr->m_next;
		} while (ptr != edge);

		polyhedra.AddFace (count, &faceIndex[0], &faceDataIndex[0]);
	}

	polyhedra.EndFace ();

	return true;
}


void dgPolyhedra::ChangeEdgeIncidentVertex (dgEdge* const edge, int32_t newIndex)
{
	dgEdge* ptr = edge;
	do {
		dgTreeNode* node = GetNodeFromInfo(*ptr);
		dgPairKey Key0 (newIndex, ptr->m_twin->m_incidentVertex);
		ReplaceKey (node, Key0.GetVal());

		node = GetNodeFromInfo(*ptr->m_twin);
		dgPairKey Key1 (ptr->m_twin->m_incidentVertex, newIndex);
		ReplaceKey (node, Key1.GetVal());

		ptr->m_incidentVertex = newIndex;

		ptr = ptr->m_twin->m_next;
	} while (ptr != edge);
}


void dgPolyhedra::DeleteDegenerateFaces (const double* const pool, int32_t strideInBytes, double area)
{
	if (!GetCount()) {
		return;
	}

#ifdef __ENABLE_SANITY_CHECK 
	HACD_ASSERT (SanityCheck ());
#endif
	dgStack <dgPolyhedra::dgTreeNode*> faceArrayPool(GetCount() / 2 + 100);

	int32_t count = 0;
	dgPolyhedra::dgTreeNode** const faceArray = &faceArrayPool[0];
	int32_t mark = IncLRU();
	Iterator iter (*this);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);

		if ((edge->m_mark != mark) && (edge->m_incidentFace > 0)) {
			faceArray[count] = iter.GetNode();
			count ++;
			dgEdge* ptr = edge;
			do	{
				ptr->m_mark = mark;
				ptr = ptr->m_next;
			} while (ptr != edge);
		}
	}

	double area2 = area * area;
	area2 *= double (4.0f);

	for (int32_t i = 0; i < count; i ++) {
		dgPolyhedra::dgTreeNode* const faceNode = faceArray[i];
		dgEdge* const edge = &faceNode->GetInfo();

		dgBigVector normal (FaceNormal (edge, pool, strideInBytes));

		double faceArea = normal % normal;
		if (faceArea < area2) {
			DeleteFace (edge);
		}
	}

#ifdef __ENABLE_SANITY_CHECK 
	mark = IncLRU();
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if ((edge->m_mark != mark) && (edge->m_incidentFace > 0)) {
			//HACD_ASSERT (edge->m_next->m_next->m_next == edge);
			dgEdge* ptr = edge;
			do	{
				ptr->m_mark = mark;
				ptr = ptr->m_next;
			} while (ptr != edge);

			dgBigVector normal (FaceNormal (edge, pool, strideInBytes));

			double faceArea = normal % normal;
			HACD_ASSERT (faceArea >= area2);
		}
	}
	HACD_ASSERT (SanityCheck ());
#endif
}


static void NormalizeVertex (int32_t count, dgBigVector* const dst, const double* const src, int32_t stride)
{
//	dgBigVector min;
//	dgBigVector max;
//	GetMinMax (min, max, src, count, int32_t (stride * sizeof (double)));
//	dgBigVector centre (float (0.0f), float (0.0f), float (0.0f), float (0.0f));
	for (int32_t i = 0; i < count; i ++) {
//		dst[i].m_x = centre.m_x + src[i * stride + 0];
//		dst[i].m_y = centre.m_y + src[i * stride + 1];
//		dst[i].m_z = centre.m_z + src[i * stride + 2];
		dst[i].m_x = src[i * stride + 0];
		dst[i].m_y = src[i * stride + 1];
		dst[i].m_z = src[i * stride + 2];
		dst[i].m_w= double (0.0f);
	}
}

static dgBigPlane EdgePlane (int32_t i0, int32_t i1, int32_t i2, const dgBigVector* const pool)
{
	const dgBigVector& p0 = pool[i0];
	const dgBigVector& p1 = pool[i1];
	const dgBigVector& p2 = pool[i2];

	dgBigPlane plane (p0, p1, p2);
	double mag = sqrt (plane % plane);
	if (mag < double (1.0e-12f)) {
		mag = double (1.0e-12f);
	}
	mag = double (1.0f) / mag;

	plane.m_x *= mag;
	plane.m_y *= mag;
	plane.m_z *= mag;
	plane.m_w *= mag;

	return plane;
}


static dgBigPlane UnboundedLoopPlane (int32_t i0, int32_t i1, int32_t i2, const dgBigVector* const pool)
{
	const dgBigVector p0 = pool[i0];
	const dgBigVector p1 = pool[i1];
	const dgBigVector p2 = pool[i2];
	dgBigVector E0 (p1 - p0); 
	dgBigVector E1 (p2 - p0); 

	dgBigVector N ((E0 * E1) * E0); 
	double dist = - (N % p0);
	dgBigPlane plane (N, dist);

	double mag = sqrt (plane % plane);
	if (mag < double (1.0e-12f)) {
		mag = double (1.0e-12f);
	}
	mag = double (10.0f) / mag;

	plane.m_x *= mag;
	plane.m_y *= mag;
	plane.m_z *= mag;
	plane.m_w *= mag;

	return plane;
}


static void CalculateAllMetrics (const dgPolyhedra* const polyhedra, dgVertexCollapseVertexMetric* const table, const dgBigVector* const pool)
{
	int32_t edgeMark = polyhedra->IncLRU();
	dgPolyhedra::Iterator iter (*polyhedra);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);

		HACD_ASSERT (edge);
		if (edge->m_mark != edgeMark) {

			if (edge->m_incidentFace > 0) {
				int32_t i0 = edge->m_incidentVertex;
				int32_t i1 = edge->m_next->m_incidentVertex;
				int32_t i2 = edge->m_prev->m_incidentVertex;

				dgBigPlane constrainPlane (EdgePlane (i0, i1, i2, pool));
				dgVertexCollapseVertexMetric tmp (constrainPlane);

				dgEdge* ptr = edge;
				do {
					ptr->m_mark = edgeMark;
					i0 = ptr->m_incidentVertex;
					table[i0].Accumulate(tmp);

					ptr = ptr->m_next;
				} while (ptr != edge);

			} else {
				HACD_ASSERT (edge->m_twin->m_incidentFace > 0);
				int32_t i0 = edge->m_twin->m_incidentVertex;
				int32_t i1 = edge->m_twin->m_next->m_incidentVertex;
				int32_t i2 = edge->m_twin->m_prev->m_incidentVertex;

				edge->m_mark = edgeMark;
				dgBigPlane constrainPlane (UnboundedLoopPlane (i0, i1, i2, pool));
				dgVertexCollapseVertexMetric tmp (constrainPlane);

				i0 = edge->m_incidentVertex;
				table[i0].Accumulate(tmp);

				i0 = edge->m_twin->m_incidentVertex;
				table[i0].Accumulate(tmp);
			}
		}
	}
}


double dgPolyhedra::EdgePenalty (const dgBigVector* const pool, dgEdge* const edge) const
{
	int32_t i0 = edge->m_incidentVertex;
	int32_t i1 = edge->m_next->m_incidentVertex;

	const dgBigVector& p0 = pool[i0];
	const dgBigVector& p1 = pool[i1];
	dgBigVector dp (p1 - p0);

	double dot = dp % dp;
	if (dot < double(1.0e-6f)) {
		return double (-1.0f);
	}

	if ((edge->m_incidentFace > 0) && (edge->m_twin->m_incidentFace > 0)) {
		dgBigVector edgeNormal (FaceNormal (edge, &pool[0].m_x, sizeof (dgBigVector)));
		dgBigVector twinNormal (FaceNormal (edge->m_twin, &pool[0].m_x, sizeof (dgBigVector)));

		double mag0 = edgeNormal % edgeNormal;
		double mag1 = twinNormal % twinNormal;
		if ((mag0 < double (1.0e-24f)) || (mag1 < double (1.0e-24f))) {
			return double (-1.0f);
		}

		edgeNormal = edgeNormal.Scale (double (1.0f) / sqrt(mag0));
		twinNormal = twinNormal.Scale (double (1.0f) / sqrt(mag1));

		dot = edgeNormal % twinNormal;
		if (dot < double (-0.9f)) {
			return float (-1.0f);
		}

		dgEdge* ptr = edge;
		do {
			if ((ptr->m_incidentFace <= 0) || (ptr->m_twin->m_incidentFace <= 0)){
				dgEdge* const adj = edge->m_twin;
				ptr = edge;
				do {
					if ((ptr->m_incidentFace <= 0) || (ptr->m_twin->m_incidentFace <= 0)){
						return float (-1.0);
					}
					ptr = ptr->m_twin->m_next;
				} while (ptr != adj);
			}
			ptr = ptr->m_twin->m_next;
		} while (ptr != edge);
	}

	int32_t faceA = edge->m_incidentFace;
	int32_t faceB = edge->m_twin->m_incidentFace;

	i0 = edge->m_twin->m_incidentVertex;
	dgBigVector p (pool[i0].m_x, pool[i0].m_y, pool[i0].m_z, float (0.0f));

	bool penalty = false;
	dgEdge* ptr = edge;
	do {
		dgEdge* const adj = ptr->m_twin;

		int32_t face = adj->m_incidentFace;
		if ((face != faceB) && (face != faceA) && (face >= 0) && (adj->m_next->m_incidentFace == face) && (adj->m_prev->m_incidentFace == face)){

			int32_t i0 = adj->m_next->m_incidentVertex;
			const dgBigVector& p0 = pool[i0];

			int32_t i1 = adj->m_incidentVertex;
			const dgBigVector& p1 = pool[i1];

			int32_t i2 = adj->m_prev->m_incidentVertex;
			const dgBigVector& p2 = pool[i2];

			dgBigVector n0 ((p1 - p0) * (p2 - p0));
			dgBigVector n1 ((p1 - p) * (p2 - p));

//			double mag0 = n0 % n0;
//			HACD_ASSERT (mag0 > double(1.0e-16f));
//			mag0 = sqrt (mag0);

//			double mag1 = n1 % n1;
//			mag1 = sqrt (mag1);

			double dot = n0 % n1;
			if (dot < double (0.0f)) {
//			if (dot <= (mag0 * mag1 * float (0.707f)) || (mag0 > (double(16.0f) * mag1))) {
				penalty = true;
				break;
			}
		}

		ptr = ptr->m_twin->m_next;
	} while (ptr != edge);

	double aspect = float (-1.0f);
	if (!penalty) {
		int32_t i0 = edge->m_twin->m_incidentVertex;
		dgBigVector p0 (pool[i0]);

		aspect = float (1.0f);
		for (dgEdge* ptr = edge->m_twin->m_next->m_twin->m_next; ptr != edge; ptr = ptr->m_twin->m_next) {
			if (ptr->m_incidentFace > 0) {
				int32_t i0 = ptr->m_next->m_incidentVertex;
				const dgBigVector& p1 = pool[i0];

				int32_t i1 = ptr->m_prev->m_incidentVertex;
				const dgBigVector& p2 = pool[i1];

				dgBigVector e0 (p1 - p0);
				dgBigVector e1 (p2 - p1);
				dgBigVector e2 (p0 - p2);

				double mag0 = e0 % e0;
				double mag1 = e1 % e1;
				double mag2 = e2 % e2;
				double maxMag = GetMax (mag0, mag1, mag2);
				double minMag = GetMin (mag0, mag1, mag2);
				double ratio = minMag / maxMag;

				if (ratio < aspect) {
					aspect = ratio;
				}
			}
		}
		aspect = sqrt (aspect);
		//aspect = 1.0f;
	}

	return aspect;
}

static void CalculateVertexMetrics (dgVertexCollapseVertexMetric table[], const dgBigVector* const pool, dgEdge* const edge)
{
	int32_t i0 = edge->m_incidentVertex;

//	const dgBigVector& p0 = pool[i0];
	table[i0].Clear ();
	dgEdge* ptr = edge;
	do {

		if (ptr->m_incidentFace > 0) {
			int32_t i1 = ptr->m_next->m_incidentVertex;
			int32_t i2 = ptr->m_prev->m_incidentVertex;
			dgBigPlane constrainPlane (EdgePlane (i0, i1, i2, pool));
			table[i0].Accumulate (constrainPlane);

		} else {
			int32_t i1 = ptr->m_twin->m_incidentVertex;
			int32_t i2 = ptr->m_twin->m_prev->m_incidentVertex;
			dgBigPlane constrainPlane (UnboundedLoopPlane (i0, i1, i2, pool));
			table[i0].Accumulate (constrainPlane);

			i1 = ptr->m_prev->m_incidentVertex;
			i2 = ptr->m_prev->m_twin->m_prev->m_incidentVertex;
			constrainPlane = UnboundedLoopPlane (i0, i1, i2, pool);
			table[i0].Accumulate (constrainPlane);
		}

		ptr = ptr->m_twin->m_next;
	} while (ptr != edge);
}

static void RemoveHalfEdge (dgPolyhedra* const polyhedra, dgEdge* const edge)
{
	dgEdgeCollapseEdgeHandle* const handle = (dgEdgeCollapseEdgeHandle *) IntToPointer (edge->m_userData);
	if (handle) { 
		handle->m_edge = NULL;
	}

	dgPolyhedra::dgTreeNode* const node = polyhedra->GetNodeFromInfo(*edge);
	HACD_ASSERT (node);
	polyhedra->Remove (node);
}


static dgEdge* CollapseEdge(dgPolyhedra* const polyhedra, dgEdge* const edge)
{
	int32_t v0 = edge->m_incidentVertex;
	int32_t v1 = edge->m_twin->m_incidentVertex;

#ifdef __ENABLE_SANITY_CHECK 
	dgPolyhedra::dgPairKey TwinKey (v1, v0);
	dgPolyhedra::dgTreeNode* const node = polyhedra->Find (TwinKey.GetVal());
	dgEdge* const twin1 = node ? &node->GetInfo() : NULL;
	HACD_ASSERT (twin1);
	HACD_ASSERT (edge->m_twin == twin1);
	HACD_ASSERT (twin1->m_twin == edge);
	HACD_ASSERT (edge->m_incidentFace != 0);
	HACD_ASSERT (twin1->m_incidentFace != 0);
#endif


	dgEdge* retEdge = edge->m_twin->m_prev->m_twin;
	if (retEdge	== edge->m_twin->m_next) {
		return NULL;
	}
	if (retEdge	== edge->m_twin) {
		return NULL;
	}
	if (retEdge	== edge->m_next) {
		retEdge = edge->m_prev->m_twin;
		if (retEdge	== edge->m_twin->m_next) {
			return NULL;
		}
		if (retEdge	== edge->m_twin) {
			return NULL;
		}
	}

	dgEdge* lastEdge = NULL;
	dgEdge* firstEdge = NULL;
	if ((edge->m_incidentFace >= 0)	&& (edge->m_twin->m_incidentFace >= 0)) {	
		lastEdge = edge->m_prev->m_twin;
		firstEdge = edge->m_twin->m_next->m_twin->m_next;
	} else if (edge->m_twin->m_incidentFace >= 0) {
		firstEdge = edge->m_twin->m_next->m_twin->m_next;
		lastEdge = edge;
	} else {
		lastEdge = edge->m_prev->m_twin;
		firstEdge = edge->m_twin->m_next;
	}

	for (dgEdge* ptr = firstEdge; ptr != lastEdge; ptr = ptr->m_twin->m_next) {
		dgEdge* badEdge = polyhedra->FindEdge (edge->m_twin->m_incidentVertex, ptr->m_twin->m_incidentVertex);
		if (badEdge) {
			return NULL;
		}
	} 

	dgEdge* const twin = edge->m_twin;
	if (twin->m_next == twin->m_prev->m_prev) {
		twin->m_prev->m_twin->m_twin = twin->m_next->m_twin;
		twin->m_next->m_twin->m_twin = twin->m_prev->m_twin;

		RemoveHalfEdge (polyhedra, twin->m_prev);
		RemoveHalfEdge (polyhedra, twin->m_next);
	} else {
		twin->m_next->m_prev = twin->m_prev;
		twin->m_prev->m_next = twin->m_next;
	}

	if (edge->m_next == edge->m_prev->m_prev) {
		edge->m_next->m_twin->m_twin = edge->m_prev->m_twin;
		edge->m_prev->m_twin->m_twin = edge->m_next->m_twin;
		RemoveHalfEdge (polyhedra, edge->m_next);
		RemoveHalfEdge (polyhedra, edge->m_prev);
	} else {
		edge->m_next->m_prev = edge->m_prev;
		edge->m_prev->m_next = edge->m_next;
	}

	HACD_ASSERT (twin->m_twin->m_incidentVertex == v0);
	HACD_ASSERT (edge->m_twin->m_incidentVertex == v1);
	RemoveHalfEdge (polyhedra, twin);
	RemoveHalfEdge (polyhedra, edge);

	dgEdge* ptr = retEdge;
	do {
		dgPolyhedra::dgPairKey pairKey (v0, ptr->m_twin->m_incidentVertex);

		dgPolyhedra::dgTreeNode* node = polyhedra->Find (pairKey.GetVal());
		if (node) {
			if (&node->GetInfo() == ptr) {
				dgPolyhedra::dgPairKey key (v1, ptr->m_twin->m_incidentVertex);
				ptr->m_incidentVertex = v1;
				node = polyhedra->ReplaceKey (node, key.GetVal());
				HACD_ASSERT (node);
			} 
		}

		dgPolyhedra::dgPairKey TwinKey (ptr->m_twin->m_incidentVertex, v0);
		node = polyhedra->Find (TwinKey.GetVal());
		if (node) {
			if (&node->GetInfo() == ptr->m_twin) {
				dgPolyhedra::dgPairKey key (ptr->m_twin->m_incidentVertex, v1);
				node = polyhedra->ReplaceKey (node, key.GetVal());
				HACD_ASSERT (node);
			}
		}

		ptr = ptr->m_twin->m_next;
	} while (ptr != retEdge);

	return retEdge;
}



void dgPolyhedra::Optimize (const double* const array, int32_t strideInBytes, double tol)
{
	dgList <dgEdgeCollapseEdgeHandle>::dgListNode *handleNodePtr;

	int32_t stride = int32_t (strideInBytes / sizeof (double));

#ifdef __ENABLE_SANITY_CHECK 
	HACD_ASSERT (SanityCheck ());
#endif

	int32_t edgeCount = GetEdgeCount() * 4 + 1024 * 16;
	int32_t maxVertexIndex = GetLastVertexIndex();

	dgStack<dgBigVector> vertexPool (maxVertexIndex); 
	dgStack<dgVertexCollapseVertexMetric> vertexMetrics (maxVertexIndex + 512); 

	dgList <dgEdgeCollapseEdgeHandle> edgeHandleList;
	dgStack<char> heapPool (2 * edgeCount * int32_t (sizeof (double) + sizeof (dgEdgeCollapseEdgeHandle*) + sizeof (int32_t))); 
	dgDownHeap<dgList <dgEdgeCollapseEdgeHandle>::dgListNode* , double> bigHeapArray(&heapPool[0], heapPool.GetSizeInBytes());

	NormalizeVertex (maxVertexIndex, &vertexPool[0], array, stride);
	memset (&vertexMetrics[0], 0, maxVertexIndex * sizeof (dgVertexCollapseVertexMetric));
	CalculateAllMetrics (this, &vertexMetrics[0], &vertexPool[0]);


	double tol2 = tol * tol;
	Iterator iter (*this);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);

		edge->m_userData = 0;
		int32_t index0 = edge->m_incidentVertex;
		int32_t index1 = edge->m_twin->m_incidentVertex;

		dgVertexCollapseVertexMetric &metric = vertexMetrics[index0];
		dgVector p	(&vertexPool[index1].m_x);
		double cost = metric.Evalue (p); 
		if (cost < tol2) {
			cost = EdgePenalty (&vertexPool[0], edge);

			if (cost > double (0.0f)) {
				dgEdgeCollapseEdgeHandle handle (edge);
				handleNodePtr = edgeHandleList.Addtop (handle);
				bigHeapArray.Push (handleNodePtr, cost);
			}
		}
	}


	while (bigHeapArray.GetCount()) {
		handleNodePtr = bigHeapArray[0];

		dgEdge* edge = handleNodePtr->GetInfo().m_edge;
		bigHeapArray.Pop();
		edgeHandleList.Remove (handleNodePtr);

		if (edge) {
			CalculateVertexMetrics (&vertexMetrics[0], &vertexPool[0], edge);

			int32_t index0 = edge->m_incidentVertex;
			int32_t index1 = edge->m_twin->m_incidentVertex;
			dgVertexCollapseVertexMetric &metric = vertexMetrics[index0];
			dgBigVector p (vertexPool[index1]);

			if ((metric.Evalue (p) < tol2) && (EdgePenalty (&vertexPool[0], edge)  > double (0.0f))) {

#ifdef __ENABLE_SANITY_CHECK 
				HACD_ASSERT (SanityCheck ());
#endif

				edge = CollapseEdge(this, edge);

#ifdef __ENABLE_SANITY_CHECK 
				HACD_ASSERT (SanityCheck ());
#endif
				if (edge) {
					// Update vertex metrics
					CalculateVertexMetrics (&vertexMetrics[0], &vertexPool[0], edge);

					// Update metrics for all surrounding vertex
					dgEdge* ptr = edge;
					do {
						CalculateVertexMetrics (&vertexMetrics[0], &vertexPool[0], ptr->m_twin);
						ptr = ptr->m_twin->m_next;
					} while (ptr != edge);

					// calculate edge cost of all incident edges
					int32_t mark = IncLRU();
					ptr = edge;
					do {
						HACD_ASSERT (ptr->m_mark != mark);
						ptr->m_mark = mark;

						index0 = ptr->m_incidentVertex;
						index1 = ptr->m_twin->m_incidentVertex;

						dgVertexCollapseVertexMetric &metric = vertexMetrics[index0];
						dgBigVector p (vertexPool[index1]);

						double cost = float (-1.0f);
						if (metric.Evalue (p) < tol2) {
							cost = EdgePenalty (&vertexPool[0], ptr);
						}

						if (cost  > double (0.0f)) {
							dgEdgeCollapseEdgeHandle handle (ptr);
							handleNodePtr = edgeHandleList.Addtop (handle);
							bigHeapArray.Push (handleNodePtr, cost);
						} else {
							dgEdgeCollapseEdgeHandle* const handle = (dgEdgeCollapseEdgeHandle*)IntToPointer (ptr->m_userData);
							if (handle) {
								handle->m_edge = NULL;
							}
							ptr->m_userData = uint32_t (NULL);

						}

						ptr = ptr->m_twin->m_next;
					} while (ptr != edge);


					// calculate edge cost of all incident edges to a surrounding vertex
					ptr = edge;
					do {
						dgEdge* const incidentEdge = ptr->m_twin;		

						dgEdge* ptr1 = incidentEdge;
						do {
							index0 = ptr1->m_incidentVertex;
							index1 = ptr1->m_twin->m_incidentVertex;

							if (ptr1->m_mark != mark) {
								ptr1->m_mark = mark;
								dgVertexCollapseVertexMetric &metric = vertexMetrics[index0];
								dgBigVector p (vertexPool[index1]);

								double cost = float (-1.0f);
								if (metric.Evalue (p) < tol2) {
									cost = EdgePenalty (&vertexPool[0], ptr1);
								}

								if (cost  > double (0.0f)) {
									HACD_ASSERT (cost > double(0.0f));
									dgEdgeCollapseEdgeHandle handle (ptr1);
									handleNodePtr = edgeHandleList.Addtop (handle);
									bigHeapArray.Push (handleNodePtr, cost);
								} else {
									dgEdgeCollapseEdgeHandle *handle;
									handle = (dgEdgeCollapseEdgeHandle*)IntToPointer (ptr1->m_userData);
									if (handle) {
										handle->m_edge = NULL;
									}
									ptr1->m_userData = uint32_t (NULL);

								}
							}

							if (ptr1->m_twin->m_mark != mark) {
								ptr1->m_twin->m_mark = mark;
								dgVertexCollapseVertexMetric &metric = vertexMetrics[index1];
								dgBigVector p (vertexPool[index0]);

								double cost = float (-1.0f);
								if (metric.Evalue (p) < tol2) {
									cost = EdgePenalty (&vertexPool[0], ptr1->m_twin);
								}

								if (cost  > double (0.0f)) {
									HACD_ASSERT (cost > double(0.0f));
									dgEdgeCollapseEdgeHandle handle (ptr1->m_twin);
									handleNodePtr = edgeHandleList.Addtop (handle);
									bigHeapArray.Push (handleNodePtr, cost);
								} else {
									dgEdgeCollapseEdgeHandle *handle;
									handle = (dgEdgeCollapseEdgeHandle*) IntToPointer (ptr1->m_twin->m_userData);
									if (handle) {
										handle->m_edge = NULL;
									}
									ptr1->m_twin->m_userData = uint32_t (NULL);

								}
							}

							ptr1 = ptr1->m_twin->m_next;
						} while (ptr1 != incidentEdge);

						ptr = ptr->m_twin->m_next;
					} while (ptr != edge);
				}
			}
		}
	}
}


dgEdge* dgPolyhedra::FindEarTip (dgEdge* const face, const double* const pool, int32_t stride, dgDownHeap<dgEdge*, double>& heap, const dgBigVector &normal) const
{
	dgEdge* ptr = face;
	dgBigVector p0 (&pool[ptr->m_prev->m_incidentVertex * stride]);
	dgBigVector p1 (&pool[ptr->m_incidentVertex * stride]);
	dgBigVector d0 (p1 - p0);
	double f = sqrt (d0 % d0);
	if (f < double (1.0e-10f)) {
		f = double (1.0e-10f);
	}
	d0 = d0.Scale (double (1.0f) / f);

	double minAngle = float (10.0f);
	do {
		dgBigVector p2 (&pool [ptr->m_next->m_incidentVertex * stride]);
		dgBigVector d1 (p2 - p1);
		float f = dgSqrt (d1 % d1);
		if (f < float (1.0e-10f)) {
			f = float (1.0e-10f);
		}
		d1 = d1.Scale (float (1.0f) / f);
		dgBigVector n (d0 * d1);

		double angle = normal %  n;
		if (angle >= double (0.0f)) {
			heap.Push (ptr, angle);
		}

		if (angle < minAngle) {
			minAngle = angle;
		}

		d0 = d1;
		p1 = p2;
		ptr = ptr->m_next;
	} while (ptr != face);

	if (minAngle > float (0.1f)) {
		return heap[0];
	}

	dgEdge* ear = NULL;
	while (heap.GetCount()) {
		ear = heap[0];
		heap.Pop();

		if (FindEdge (ear->m_prev->m_incidentVertex, ear->m_next->m_incidentVertex)) {
			continue;
		}

		dgBigVector p0 (&pool [ear->m_prev->m_incidentVertex * stride]);
		dgBigVector p1 (&pool [ear->m_incidentVertex * stride]);
		dgBigVector p2 (&pool [ear->m_next->m_incidentVertex * stride]);

		dgBigVector p10 (p1 - p0);
		dgBigVector p21 (p2 - p1);
		dgBigVector p02 (p0 - p2);

		for (ptr = ear->m_next->m_next; ptr != ear->m_prev; ptr = ptr->m_next) {
			dgBigVector p (&pool [ptr->m_incidentVertex * stride]);

			double side = ((p - p0) * p10) % normal;
			if (side < double (0.05f)) {
				side = ((p - p1) * p21) % normal;
				if (side < double (0.05f)) {
					side = ((p - p2) * p02) % normal;
					if (side < float (0.05f)) {
						break;
					}
				}
			}
		}

		if (ptr == ear->m_prev) {
			break;
		}
	}

	return ear;
}





//dgEdge* TriangulateFace (dgPolyhedra& polyhedra, dgEdge* face, const float* const pool, int32_t stride, dgDownHeap<dgEdge*, float>& heap, dgVector* const faceNormalOut)
dgEdge* dgPolyhedra::TriangulateFace (dgEdge* face, const double* const pool, int32_t stride, dgDownHeap<dgEdge*, double>& heap, dgBigVector* const faceNormalOut)
{
#ifdef _DEBUG
	dgEdge* perimeter [1024 * 16]; 
	dgEdge* ptr = face;
	int32_t perimeterCount = 0;
	do {
		perimeter[perimeterCount] = ptr;
		perimeterCount ++;
		HACD_ASSERT (perimeterCount < int32_t (sizeof (perimeter) / sizeof (perimeter[0])));
		ptr = ptr->m_next;
	} while (ptr != face);
	perimeter[perimeterCount] = face;
	HACD_ASSERT ((perimeterCount + 1) < int32_t (sizeof (perimeter) / sizeof (perimeter[0])));
#endif
	dgBigVector normal (FaceNormal (face, pool, int32_t (stride * sizeof (double))));

	double dot = normal % normal;
	if (dot < double (1.0e-12f)) {
		if (faceNormalOut) {
			*faceNormalOut = dgBigVector (float (0.0f), float (0.0f), float (0.0f), float (0.0f)); 
		}
		return face;
	}
	normal = normal.Scale (double (1.0f) / sqrt (dot));
	if (faceNormalOut) {
		*faceNormalOut = normal;
	}


	while (face->m_next->m_next->m_next != face) {
		dgEdge* const ear = FindEarTip (face, pool, stride, heap, normal); 
		if (!ear) {
			return face;
		}
		if ((face == ear)	|| (face == ear->m_prev)) {
			face = ear->m_prev->m_prev;
		}
		dgEdge* const edge = AddHalfEdge (ear->m_next->m_incidentVertex, ear->m_prev->m_incidentVertex);
		if (!edge) {
			return face;
		}
		dgEdge* const twin = AddHalfEdge (ear->m_prev->m_incidentVertex, ear->m_next->m_incidentVertex);
		if (!twin) {
			return face;
		}
		HACD_ASSERT (twin);


		edge->m_mark = ear->m_mark;
		edge->m_userData = ear->m_next->m_userData;
		edge->m_incidentFace = ear->m_incidentFace;

		twin->m_mark = ear->m_mark;
		twin->m_userData = ear->m_prev->m_userData;
		twin->m_incidentFace = ear->m_incidentFace;

		edge->m_twin = twin;
		twin->m_twin = edge;

		twin->m_prev = ear->m_prev->m_prev;
		twin->m_next = ear->m_next;
		ear->m_prev->m_prev->m_next = twin;
		ear->m_next->m_prev = twin;

		edge->m_next = ear->m_prev;
		edge->m_prev = ear;
		ear->m_prev->m_prev = edge;
		ear->m_next = edge;

		heap.Flush ();
	}
	return NULL;
}


void dgPolyhedra::MarkAdjacentCoplanarFaces (dgPolyhedra& polyhedraOut, dgEdge* const face, const double* const pool, int32_t strideInBytes)
{
	const double normalDeviation = double (0.9999f);
	const double distanceFromPlane = double (1.0f / 128.0f);

	int32_t faceIndex[1024 * 4];
	dgEdge* stack[1024 * 4];
	dgEdge* deleteEdge[1024 * 4];

	int32_t deleteCount = 1;
	deleteEdge[0] = face;
	int32_t stride = int32_t (strideInBytes / sizeof (double));

	HACD_ASSERT (face->m_incidentFace > 0);

	dgBigVector normalAverage (FaceNormal (face, pool, strideInBytes));
	double dot = normalAverage % normalAverage;
	if (dot > double (1.0e-12f)) {
		int32_t testPointsCount = 1;
		dot = double (1.0f) / sqrt (dot);
		dgBigVector normal (normalAverage.Scale (dot));

		dgBigVector averageTestPoint (&pool[face->m_incidentVertex * stride]);
		dgBigPlane testPlane(normal, - (averageTestPoint % normal));

		polyhedraOut.BeginFace();

		IncLRU();
		int32_t faceMark = IncLRU();

		int32_t faceIndexCount = 0;
		dgEdge* ptr = face;
		do {
			ptr->m_mark = faceMark;
			faceIndex[faceIndexCount] = ptr->m_incidentVertex;
			faceIndexCount ++;
			HACD_ASSERT (faceIndexCount < int32_t (sizeof (faceIndex) / sizeof (faceIndex[0])));
			ptr = ptr ->m_next;
		} while (ptr != face);
		polyhedraOut.AddFace(faceIndexCount, faceIndex);

		int32_t index = 1;
		deleteCount = 0;
		stack[0] = face;
		while (index) 
		{
			index --;
			dgEdge* const face = stack[index];
			deleteEdge[deleteCount] = face;
			deleteCount ++;
			HACD_ASSERT (deleteCount < int32_t (sizeof (deleteEdge) / sizeof (deleteEdge[0])));
// TODO:JWR Temporarily commented out...			HACD_ASSERT (face->m_next->m_next->m_next == face);

			dgEdge* edge = face;
			do 
			{
				dgEdge* const ptr = edge->m_twin;
				if (ptr->m_incidentFace > 0) 
				{
					if (ptr->m_mark != faceMark) 
					{
						dgEdge* ptr1 = ptr;
						faceIndexCount = 0;
						do 
						{
							ptr1->m_mark = faceMark;
							faceIndex[faceIndexCount] = ptr1->m_incidentVertex;
							HACD_ASSERT (faceIndexCount < int32_t (sizeof (faceIndex) / sizeof (faceIndex[0])));
							faceIndexCount ++;
							ptr1 = ptr1 ->m_next;
						} while (ptr1 != ptr);

						dgBigVector normal1 (FaceNormal (ptr, pool, strideInBytes));
						dot = normal1 % normal1;
						if (dot < double (1.0e-12f)) {
							deleteEdge[deleteCount] = ptr;
							deleteCount ++;
							HACD_ASSERT (deleteCount < int32_t (sizeof (deleteEdge) / sizeof (deleteEdge[0])));
						} else {
							//normal1 = normal1.Scale (double (1.0f) / sqrt (dot));
							dgBigVector testNormal (normal1.Scale (double (1.0f) / sqrt (dot)));
							dot = normal % testNormal;
							if (dot >= normalDeviation) {
								dgBigVector testPoint (&pool[ptr->m_prev->m_incidentVertex * stride]);
								double dist = fabs (testPlane.Evalue (testPoint));
								if (dist < distanceFromPlane) {
									testPointsCount ++;

									averageTestPoint += testPoint;
									testPoint = averageTestPoint.Scale (double (1.0f) / double(testPointsCount));

									normalAverage += normal1;
									testNormal = normalAverage.Scale (double (1.0f) / sqrt (normalAverage % normalAverage));
									testPlane = dgBigPlane (testNormal, - (testPoint % testNormal));

									polyhedraOut.AddFace(faceIndexCount, faceIndex);;
									stack[index] = ptr;
									index ++;
									HACD_ASSERT (index < int32_t (sizeof (stack) / sizeof (stack[0])));
								}
							}
						}
					}
				}

				edge = edge->m_next;
			} while (edge != face);
		}
		polyhedraOut.EndFace();
	}

	for (int32_t index = 0; index < deleteCount; index ++) {
		DeleteFace (deleteEdge[index]);
	}
}


void dgPolyhedra::RefineTriangulation (const double* const vertex, int32_t stride, dgBigVector* const normal, int32_t perimeterCount, dgEdge** const perimeter)
{
	dgList<dgDiagonalEdge> dignonals;

	for (int32_t i = 1; i <= perimeterCount; i ++) {
		dgEdge* const last = perimeter[i - 1];
		for (dgEdge* ptr = perimeter[i]->m_prev; ptr != last; ptr = ptr->m_twin->m_prev) {
			dgList<dgDiagonalEdge>::dgListNode* node = dignonals.GetFirst();
			for (; node; node = node->GetNext()) {
				const dgDiagonalEdge& key = node->GetInfo();
				if (((key.m_i0 == ptr->m_incidentVertex) && (key.m_i1 == ptr->m_twin->m_incidentVertex)) ||
					((key.m_i1 == ptr->m_incidentVertex) && (key.m_i0 == ptr->m_twin->m_incidentVertex))) {
						break;
				}
			}
			if (!node) {
				dgDiagonalEdge key (ptr);
				dignonals.Append(key);
			}
		}
	}

	dgEdge* const face = perimeter[0];
	int32_t i0 = face->m_incidentVertex * stride;
	int32_t i1 = face->m_next->m_incidentVertex * stride;
	dgBigVector p0 (vertex[i0], vertex[i0 + 1], vertex[i0 + 2], float (0.0f));
	dgBigVector p1 (vertex[i1], vertex[i1 + 1], vertex[i1 + 2], float (0.0f));

	dgBigVector p1p0 (p1 - p0);
	double mag2 = p1p0 % p1p0;
	for (dgEdge* ptr = face->m_next->m_next; mag2 < float (1.0e-12f); ptr = ptr->m_next) {
		int32_t i1 = ptr->m_incidentVertex * stride;
		dgBigVector p1 (vertex[i1], vertex[i1 + 1], vertex[i1 + 2], float (0.0f));
		p1p0 = p1 - p0;
		mag2 = p1p0 % p1p0;
	}

	dgMatrix matrix (dgGetIdentityMatrix());
	matrix.m_posit = p0;
	matrix.m_front = dgVector (p1p0.Scale (double (1.0f) / sqrt (mag2)));
	matrix.m_right = dgVector (normal->Scale (double (1.0f) / sqrt (*normal % *normal)));
	matrix.m_up = matrix.m_right * matrix.m_front;
	matrix = matrix.Inverse();
	matrix.m_posit.m_w = float (1.0f);

	int32_t maxCount = dignonals.GetCount() * dignonals.GetCount();
	while (dignonals.GetCount() && maxCount) {
		maxCount --;
		dgList<dgDiagonalEdge>::dgListNode* const node = dignonals.GetFirst();
		dgDiagonalEdge key (node->GetInfo());
		dignonals.Remove(node);
		dgEdge* const edge = FindEdge(key.m_i0, key.m_i1);
		if (edge) {
			int32_t i0 = edge->m_incidentVertex * stride;
			int32_t i1 = edge->m_next->m_incidentVertex * stride;
			int32_t i2 = edge->m_next->m_next->m_incidentVertex * stride;
			int32_t i3 = edge->m_twin->m_prev->m_incidentVertex * stride;

			dgBigVector p0 (vertex[i0], vertex[i0 + 1], vertex[i0 + 2], double (0.0f));
			dgBigVector p1 (vertex[i1], vertex[i1 + 1], vertex[i1 + 2], double (0.0f));
			dgBigVector p2 (vertex[i2], vertex[i2 + 1], vertex[i2 + 2], double (0.0f));
			dgBigVector p3 (vertex[i3], vertex[i3 + 1], vertex[i3 + 2], double (0.0f));

			p0 = matrix.TransformVector(p0);
			p1 = matrix.TransformVector(p1);
			p2 = matrix.TransformVector(p2);
			p3 = matrix.TransformVector(p3);

			double circleTest[3][3];
			circleTest[0][0] = p0[0] - p3[0];
			circleTest[0][1] = p0[1] - p3[1];
			circleTest[0][2] = circleTest[0][0] * circleTest[0][0] + circleTest[0][1] * circleTest[0][1];

			circleTest[1][0] = p1[0] - p3[0];
			circleTest[1][1] = p1[1] - p3[1];
			circleTest[1][2] = circleTest[1][0] * circleTest[1][0] + circleTest[1][1] * circleTest[1][1];

			circleTest[2][0] = p2[0] - p3[0];
			circleTest[2][1] = p2[1] - p3[1];
			circleTest[2][2] = circleTest[2][0] * circleTest[2][0] + circleTest[2][1] * circleTest[2][1];

			double error;
			double det = Determinant3x3 (circleTest, &error);
			if (det < float (0.0f)) {
				dgEdge* frontFace0 = edge->m_prev;
				dgEdge* backFace0 = edge->m_twin->m_prev;

				FlipEdge(edge);

				if (perimeterCount > 4) {
					dgEdge* backFace1 = backFace0->m_next;
					dgEdge* frontFace1 = frontFace0->m_next;
					for (int32_t i = 0; i < perimeterCount; i ++) {
						if (frontFace0 == perimeter[i]) {
							frontFace0 = NULL;
						}
						if (frontFace1 == perimeter[i]) {
							frontFace1 = NULL;
						}

						if (backFace0 == perimeter[i]) {
							backFace0 = NULL;
						}
						if (backFace1 == perimeter[i]) {
							backFace1 = NULL;
						}
					}

					if (backFace0 && (backFace0->m_incidentFace > 0) && (backFace0->m_twin->m_incidentFace > 0)) {
						dgDiagonalEdge key (backFace0);
						dignonals.Append(key);
					}
					if (backFace1 && (backFace1->m_incidentFace > 0) && (backFace1->m_twin->m_incidentFace > 0)) {
						dgDiagonalEdge key (backFace1);
						dignonals.Append(key);
					}

					if (frontFace0 && (frontFace0->m_incidentFace > 0) && (frontFace0->m_twin->m_incidentFace > 0)) {
						dgDiagonalEdge key (frontFace0);
						dignonals.Append(key);
					}

					if (frontFace1 && (frontFace1->m_incidentFace > 0) && (frontFace1->m_twin->m_incidentFace > 0)) {
						dgDiagonalEdge key (frontFace1);
						dignonals.Append(key);
					}
				}
			}
		}
	}
}


void dgPolyhedra::RefineTriangulation (const double* const vertex, int32_t stride)
{
	dgEdge* edgePerimeters[1024 * 16];
	int32_t perimeterCount = 0;

	dgPolyhedra::Iterator iter (*this);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if (edge->m_incidentFace < 0) {
			dgEdge* ptr = edge;
			do {
				edgePerimeters[perimeterCount] = ptr->m_twin;
				perimeterCount ++;
				HACD_ASSERT (perimeterCount < int32_t (sizeof (edgePerimeters) / sizeof (edgePerimeters[0])));
				ptr = ptr->m_prev;
			} while (ptr != edge);
			break;
		}
	}
	HACD_ASSERT (perimeterCount);
	HACD_ASSERT (perimeterCount < int32_t (sizeof (edgePerimeters) / sizeof (edgePerimeters[0])));
	edgePerimeters[perimeterCount] = edgePerimeters[0];

	dgBigVector normal (FaceNormal(edgePerimeters[0], vertex, int32_t (stride * sizeof (double))));
	if ((normal % normal) > float (1.0e-12f)) {
		RefineTriangulation (vertex, stride, &normal, perimeterCount, edgePerimeters);
	}
}


void dgPolyhedra::OptimizeTriangulation (const double* const vertex, int32_t strideInBytes)
{
	int32_t polygon[1024 * 8];
	int32_t stride = int32_t (strideInBytes / sizeof (double));

	dgPolyhedra leftOver;
	dgPolyhedra buildConvex;

	buildConvex.BeginFace();
	dgPolyhedra::Iterator iter (*this);

	for (iter.Begin(); iter; ) {
		dgEdge* const edge = &(*iter);
		iter++;

		if (edge->m_incidentFace > 0) {
			dgPolyhedra flatFace;
			MarkAdjacentCoplanarFaces (flatFace, edge, vertex, strideInBytes);
			//HACD_ASSERT (flatFace.GetCount());

			if (flatFace.GetCount()) {
				//flatFace.Triangulate (vertex, strideInBytes, &leftOver);
				//HACD_ASSERT (!leftOver.GetCount());
				flatFace.RefineTriangulation (vertex, stride);

				int32_t mark = flatFace.IncLRU();
				dgPolyhedra::Iterator iter (flatFace);
				for (iter.Begin(); iter; iter ++) {
					dgEdge* const edge = &(*iter);
					if (edge->m_mark != mark) {
						if (edge->m_incidentFace > 0) {
							dgEdge* ptr = edge;
							int32_t vertexCount = 0;
							do {
								polygon[vertexCount] = ptr->m_incidentVertex;				
								vertexCount ++;
								HACD_ASSERT (vertexCount < int32_t (sizeof (polygon) / sizeof (polygon[0])));
								ptr->m_mark = mark;
								ptr = ptr->m_next;
							} while (ptr != edge);
							if (vertexCount >= 3) {
								buildConvex.AddFace (vertexCount, polygon);
							}
						}
					}
				}
			}
			iter.Begin();
		}
	}
	buildConvex.EndFace();
	HACD_ASSERT (GetCount() == 0);
	SwapInfo(buildConvex);
}


void dgPolyhedra::Triangulate (const double* const /*vertex*/, int32_t /*strideInBytes*/, dgPolyhedra* const /*leftOver*/)
{
	int32_t count = GetCount() / 2;
	dgStack<char> memPool (int32_t ((count + 512) * (2 * sizeof (double)))); 
	dgDownHeap<dgEdge*, double> heap(&memPool[0], memPool.GetSizeInBytes());
#if 0
	int32_t stride = int32_t (strideInBytes / sizeof (double));
	int32_t mark = IncLRU();
	Iterator iter (*this);
	for (iter.Begin(); iter; ) 
	{
		dgEdge* const thisEdge = &(*iter);
		iter ++;

		if (thisEdge->m_mark == mark) 
		{
			continue;
		}
		if (thisEdge->m_incidentFace < 0) 
		{
			continue;
		}

		count = 0;
		dgEdge* ptr = thisEdge;
		do 
		{
			count ++;
			ptr->m_mark = mark;
			ptr = ptr->m_next;
		} while (ptr != thisEdge);

		if (count > 3) 
		{
			dgEdge* const edge = TriangulateFace (thisEdge, vertex, stride, heap, NULL);
			heap.Flush ();

			if (edge) 
			{
				HACD_ASSERT (edge->m_incidentFace > 0);

				if (leftOver) 
				{
					int32_t* const index = (int32_t *) &heap[0];
					int64_t* const data = (int64_t *)&index[count];
					int32_t i = 0;
					dgEdge* ptr = edge;
					do {
						index[i] = ptr->m_incidentVertex;
						data[i] = int64_t (ptr->m_userData);
						i ++;
						ptr = ptr->m_next;
					} while (ptr != edge);
					leftOver->AddFace(i, index, data);
				} 
				DeleteFace (edge);
				iter.Begin();
			}
		}
	}
	OptimizeTriangulation (vertex, strideInBytes);
	mark = IncLRU();
	m_faceSecuence = 1;
	for (iter.Begin(); iter; iter ++) 
	{
		dgEdge* edge = &(*iter);
		if (edge->m_mark == mark) 
		{
			continue;
		}
		if (edge->m_incidentFace < 0) 
		{
			continue;
		}
		HACD_ASSERT (edge == edge->m_next->m_next->m_next);

		for (int32_t i = 0; i < 3; i ++) 
		{
			edge->m_incidentFace = m_faceSecuence; 
			edge->m_mark = mark;
			edge = edge->m_next;
		}
		m_faceSecuence ++;
	}
#endif
}


static void RemoveColinearVertices (dgPolyhedra& flatFace, const double* const vertex, int32_t stride)
{
	dgEdge* edgePerimeters[1024];

	int32_t perimeterCount = 0;
	int32_t mark = flatFace.IncLRU();
	dgPolyhedra::Iterator iter (flatFace);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if ((edge->m_incidentFace < 0) && (edge->m_mark != mark)) {
			dgEdge* ptr = edge;
			do {
				ptr->m_mark = mark;
				ptr = ptr->m_next;
			} while (ptr != edge);
			edgePerimeters[perimeterCount] = edge;
			perimeterCount ++;
			HACD_ASSERT (perimeterCount < int32_t (sizeof (edgePerimeters) / sizeof (edgePerimeters[0])));
		}
	}

	for (int32_t i = 0; i < perimeterCount; i ++) {
		dgEdge* edge = edgePerimeters[i];
		dgEdge* ptr = edge;
		dgVector p0 (&vertex[ptr->m_incidentVertex * stride]);
		dgVector p1 (&vertex[ptr->m_next->m_incidentVertex * stride]);
		dgVector e0 (p1 - p0) ;
		e0 = e0.Scale (float (1.0f) / (dgSqrt (e0 % e0) + float (1.0e-12f)));
		int32_t ignoreTest = 1;
		do {
			ignoreTest = 0;
			dgVector p2 (&vertex[ptr->m_next->m_next->m_incidentVertex * stride]);
			dgVector e1 (p2 - p1);
			e1 = e1.Scale (float (1.0f) / (dgSqrt (e1 % e1) + float (1.0e-12f)));
			float dot = e1 % e0;
			if (dot > float (float (0.9999f))) {

				for (dgEdge* interiorEdge = ptr->m_next->m_twin->m_next; interiorEdge != ptr->m_twin; interiorEdge = ptr->m_next->m_twin->m_next) {
					flatFace.DeleteEdge (interiorEdge);
				} 

				if (ptr->m_twin->m_next->m_next->m_next == ptr->m_twin) {
					HACD_ASSERT (ptr->m_twin->m_next->m_incidentFace > 0);
					flatFace.DeleteEdge (ptr->m_twin->m_next);
				}

				HACD_ASSERT (ptr->m_next->m_twin->m_next->m_twin == ptr);
				edge = ptr->m_next;

				if (!flatFace.FindEdge (ptr->m_incidentVertex, edge->m_twin->m_incidentVertex) && 
					!flatFace.FindEdge (edge->m_twin->m_incidentVertex, ptr->m_incidentVertex)) {
						ptr->m_twin->m_prev = edge->m_twin->m_prev;
						edge->m_twin->m_prev->m_next = ptr->m_twin;

						edge->m_next->m_prev = ptr;
						ptr->m_next = edge->m_next;

						edge->m_next = edge->m_twin;
						edge->m_prev = edge->m_twin;
						edge->m_twin->m_next = edge;
						edge->m_twin->m_prev = edge;
						flatFace.DeleteEdge (edge);								
						flatFace.ChangeEdgeIncidentVertex (ptr->m_twin, ptr->m_next->m_incidentVertex);

						e1 = e0;
						p1 = p2;
						edge = ptr;
						ignoreTest = 1;
						continue;
				}
			}

			e0 = e1;
			p1 = p2;
			ptr = ptr->m_next;
		} while ((ptr != edge) || ignoreTest);
	}
}


static int32_t GetInteriorDiagonals (dgPolyhedra& polyhedra, dgEdge** const diagonals, int32_t maxCount)
{
	int32_t count = 0;
	int32_t mark = polyhedra.IncLRU();
	dgPolyhedra::Iterator iter (polyhedra);
	for (iter.Begin(); iter; iter++) {
		dgEdge* const edge = &(*iter);
		if (edge->m_mark != mark) { 
			if (edge->m_incidentFace > 0) {
				if (edge->m_twin->m_incidentFace > 0) {
					edge->m_twin->m_mark = mark;
					if (count < maxCount){
						diagonals[count] = edge;
						count ++;
					}
					HACD_ASSERT (count <= maxCount);
				}
			}
		}
		edge->m_mark = mark;
	}

	return count;
}

static bool IsEssensialPointDiagonal (dgEdge* const diagonal, const dgBigVector& normal, const double* const pool, int32_t stride)
{
	double dot;
	dgBigVector p0 (&pool[diagonal->m_incidentVertex * stride]);
	dgBigVector p1 (&pool[diagonal->m_twin->m_next->m_twin->m_incidentVertex * stride]);
	dgBigVector p2 (&pool[diagonal->m_prev->m_incidentVertex * stride]);

	dgBigVector e1 (p1 - p0);
	dot = e1 % e1;
	if (dot < double (1.0e-12f)) {
		return false;
	}
	e1 = e1.Scale (double (1.0f) / sqrt(dot));

	dgBigVector e2 (p2 - p0);
	dot = e2 % e2;
	if (dot < double (1.0e-12f)) {
		return false;
	}
	e2 = e2.Scale (double (1.0f) / sqrt(dot));

	dgBigVector n1 (e1 * e2); 

	dot = normal % n1;
	//if (dot > double (float (0.1f)f)) {
	//if (dot >= double (-1.0e-6f)) {
	if (dot >= double (0.0f)) {
		return false;
	}
	return true;
}


static bool IsEssensialDiagonal (dgEdge* const diagonal, const dgBigVector& normal, const double* const pool,  int32_t stride)
{
	return IsEssensialPointDiagonal (diagonal, normal, pool, stride) || IsEssensialPointDiagonal (diagonal->m_twin, normal, pool, stride); 
}


void dgPolyhedra::ConvexPartition (const double* const vertex, int32_t strideInBytes, dgPolyhedra* const leftOversOut)
{
	if (GetCount()) {
		Triangulate (vertex, strideInBytes, leftOversOut);
		DeleteDegenerateFaces (vertex, strideInBytes, float (1.0e-5f));
		Optimize (vertex, strideInBytes, float (1.0e-4f));
		DeleteDegenerateFaces (vertex, strideInBytes, float (1.0e-5f));

		if (GetCount()) {
			int32_t removeCount = 0;
			int32_t stride = int32_t (strideInBytes / sizeof (double));

			int32_t polygon[1024 * 8];
			dgEdge* diagonalsPool[1024 * 8];
			dgPolyhedra buildConvex;

			buildConvex.BeginFace();
			dgPolyhedra::Iterator iter (*this);
			for (iter.Begin(); iter; ) {
				dgEdge* edge = &(*iter);
				iter++;
				if (edge->m_incidentFace > 0) {

					dgPolyhedra flatFace;
					MarkAdjacentCoplanarFaces (flatFace, edge, vertex, strideInBytes);

					if (flatFace.GetCount()) {
						flatFace.RefineTriangulation (vertex, stride);
						RemoveColinearVertices (flatFace, vertex, stride);

						int32_t diagonalCount = GetInteriorDiagonals (flatFace, diagonalsPool, sizeof (diagonalsPool) / sizeof (diagonalsPool[0]));
						if (diagonalCount) {
							edge = &flatFace.GetRoot()->GetInfo();
							if (edge->m_incidentFace < 0) {
								edge = edge->m_twin;
							}
							HACD_ASSERT (edge->m_incidentFace > 0);

							dgBigVector normal (FaceNormal (edge, vertex, strideInBytes));
							normal = normal.Scale (double (1.0f) / sqrt (normal % normal));

							edge = NULL;
							dgPolyhedra::Iterator iter (flatFace);
							for (iter.Begin(); iter; iter ++) {
								edge = &(*iter);
								if (edge->m_incidentFace < 0) {
									break;
								}
							}
							HACD_ASSERT (edge);

							int32_t isConvex = 1;
							dgEdge* ptr = edge;
							int32_t mark = flatFace.IncLRU();

							dgBigVector normal2 (normal);
							dgBigVector p0 (&vertex[ptr->m_prev->m_incidentVertex * stride]);
							dgBigVector p1 (&vertex[ptr->m_incidentVertex * stride]);
							dgBigVector e0 (p1 - p0);
							e0 = e0.Scale (float (1.0f) / (dgSqrt (e0 % e0) + float (1.0e-14f)));
							do {
								dgBigVector p2 (&vertex[ptr->m_next->m_incidentVertex * stride]);
								dgBigVector e1 (p2 - p1);
								e1 = e1.Scale (float (1.0f) / (sqrt (e1 % e1) + float (1.0e-14f)));
								double dot = (e0 * e1) % normal2;
								//if (dot > float (0.0f)) {
								if (dot > float (5.0e-3f)) {
									isConvex = 0;
									break;
								}
								ptr->m_mark = mark;
								e0 = e1;
								p1 = p2;
								ptr = ptr->m_next;
							} while (ptr != edge);

							if (isConvex) {
								dgPolyhedra::Iterator iter (flatFace);
								for (iter.Begin(); iter; iter ++) {
									ptr = &(*iter);
									if (ptr->m_incidentFace < 0) {
										if (ptr->m_mark < mark) {
											isConvex = 0;
											break;
										}
									}
								}
							}

							if (isConvex) {
								if (diagonalCount > 2) {
									int32_t count = 0;
									ptr = edge;
									do {
										polygon[count] = ptr->m_incidentVertex;				
										count ++;
										HACD_ASSERT (count < int32_t (sizeof (polygon) / sizeof (polygon[0])));
										ptr = ptr->m_next;
									} while (ptr != edge);

									for (int32_t i = 0; i < count - 1; i ++) {
										for (int32_t j = i + 1; j < count; j ++) {
											if (polygon[i] == polygon[j]) {
												i = count;
												isConvex = 0;
												break ;
											}
										}
									}
								}
							}

							if (isConvex) {
								for (int32_t j = 0; j < diagonalCount; j ++) {
									dgEdge* const diagonal = diagonalsPool[j];
									removeCount ++;
									flatFace.DeleteEdge (diagonal);
								}
							} else {
								for (int32_t j = 0; j < diagonalCount; j ++) {
									dgEdge* const diagonal = diagonalsPool[j];
									if (!IsEssensialDiagonal(diagonal, normal, vertex, stride)) {
										removeCount ++;
										flatFace.DeleteEdge (diagonal);
									}
								}
							}
						}

						int32_t mark = flatFace.IncLRU();
						dgPolyhedra::Iterator iter (flatFace);
						for (iter.Begin(); iter; iter ++) {
							dgEdge* const edge = &(*iter);
							if (edge->m_mark != mark) {
								if (edge->m_incidentFace > 0) {
									dgEdge* ptr = edge;
									int32_t diagonalCount = 0;
									do {
										polygon[diagonalCount] = ptr->m_incidentVertex;				
										diagonalCount ++;
										HACD_ASSERT (diagonalCount < int32_t (sizeof (polygon) / sizeof (polygon[0])));
										ptr->m_mark = mark;
										ptr = ptr->m_next;
									} while (ptr != edge);
									if (diagonalCount >= 3) {
										buildConvex.AddFace (diagonalCount, polygon);
									}
								}
							}
						}
					}
					iter.Begin();
				}
			}

			buildConvex.EndFace();
			HACD_ASSERT (GetCount() == 0);
			SwapInfo(buildConvex);
		}
	}
}


dgSphere dgPolyhedra::CalculateSphere (const double* const vertex, int32_t strideInBytes, const dgMatrix* const /*basis*/) const
{
/*
		// this si a degenerate mesh of a flat plane calculate OOBB by discrete rotations
		dgStack<int32_t> pool (GetCount() * 3 + 6); 
		int32_t* const indexList = &pool[0]; 

		dgMatrix axis (dgGetIdentityMatrix());
		dgBigVector p0 (float ( 1.0e10f), float ( 1.0e10f), float ( 1.0e10f), float (0.0f));
		dgBigVector p1 (float (-1.0e10f), float (-1.0e10f), float (-1.0e10f), float (0.0f));

		int32_t stride = int32_t (strideInBytes / sizeof (double));
		int32_t indexCount = 0;
		int32_t mark = IncLRU();
		dgPolyhedra::Iterator iter(*this);
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &(*iter);
			if (edge->m_mark != mark) {
				dgEdge *ptr = edge;
				do {
					ptr->m_mark = mark;
					ptr = ptr->m_twin->m_next;
				} while (ptr != edge);
				int32_t index = edge->m_incidentVertex;
				indexList[indexCount + 6] = edge->m_incidentVertex;
				dgBigVector point (vertex[index * stride + 0], vertex[index * stride + 1], vertex[index * stride + 2], float (0.0f));
				for (int32_t i = 0; i < 3; i ++) {
					if (point[i] < p0[i]) {
						p0[i] = point[i];
						indexList[i * 2 + 0] = index;
					}
					if (point[i] > p1[i]) {
						p1[i] = point[i];
						indexList[i * 2 + 1] = index;
					}
				}
				indexCount ++;
			}
		}
		indexCount += 6;


		dgBigVector size (p1 - p0);
		double volume = size.m_x * size.m_y * size.m_z;


		for (float pitch = float (0.0f); pitch < float (90.0f); pitch += float (10.0f)) {
			dgMatrix pitchMatrix (dgPitchMatrix(pitch * float (3.1416f) / float (180.0f)));
			for (float yaw = float (0.0f); yaw  < float (90.0f); yaw  += float (10.0f)) {
				dgMatrix yawMatrix (dgYawMatrix(yaw * float (3.1416f) / float (180.0f)));
				for (float roll = float (0.0f); roll < float (90.0f); roll += float (10.0f)) {
					int32_t tmpIndex[6];
					dgMatrix rollMatrix (dgRollMatrix(roll * float (3.1416f) / float (180.0f)));
					dgMatrix tmp (pitchMatrix * yawMatrix * rollMatrix);
					dgBigVector q0 (float ( 1.0e10f), float ( 1.0e10f), float ( 1.0e10f), float (0.0f));
					dgBigVector q1 (float (-1.0e10f), float (-1.0e10f), float (-1.0e10f), float (0.0f));

					float volume1 = float (1.0e10f);
					for (int32_t i = 0; i < indexCount; i ++) {
						int32_t index = indexList[i];
						dgBigVector point (vertex[index * stride + 0], vertex[index * stride + 1], vertex[index * stride + 2], float (0.0f));
						point = tmp.UnrotateVector(point);

						for (int32_t j = 0; j < 3; j ++) {
							if (point[j] < q0[j]) {
								q0[j] = point[j];
								tmpIndex[j * 2 + 0] = index;
							}
							if (point[j] > q1[j]) {
								q1[j] = point[j];
								tmpIndex[j * 2 + 1] = index;
							}
						}


						dgVector size1 (q1 - q0);
						volume1 = size1.m_x * size1.m_y * size1.m_z;
						if (volume1 >= volume) {
							break;
						}
					}

					if (volume1 < volume) {
						p0 = q0;
						p1 = q1;
						axis = tmp;
						volume = volume1;
						memcpy (indexList, tmpIndex, sizeof (tmpIndex));
					}
				}
			}
		}

		HACD_ASSERT (0);
		dgSphere sphere (axis);
		dgVector q0 (p0);
		dgVector q1 (p1);
		sphere.m_posit = axis.RotateVector((q1 + q0).Scale (float (0.5f)));
		sphere.m_size = (q1 - q0).Scale (float (0.5f));
		return sphere;
*/

	int32_t stride = int32_t (strideInBytes / sizeof (double));	

	int32_t vertexCount = 0;
	int32_t mark = IncLRU();
	dgPolyhedra::Iterator iter(*this);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if (edge->m_mark != mark) {
			dgEdge* ptr = edge;
			do {
				ptr->m_mark = mark;
				ptr = ptr->m_twin->m_next;
			} while (ptr != edge);
			vertexCount ++;
		}
	}
	HACD_ASSERT (vertexCount);

	mark = IncLRU();
	int32_t vertexCountIndex = 0;
	dgStack<dgBigVector> pool (vertexCount);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if (edge->m_mark != mark) {
			dgEdge* ptr = edge;
			do {
				ptr->m_mark = mark;
				ptr = ptr->m_twin->m_next;
			} while (ptr != edge);
			int32_t incidentVertex = edge->m_incidentVertex * stride;
			pool[vertexCountIndex] = dgBigVector (vertex[incidentVertex + 0], vertex[incidentVertex + 1], vertex[incidentVertex + 2], float (0.0f));
			vertexCountIndex ++;
		}
	}
	HACD_ASSERT (vertexCountIndex <= vertexCount);

	dgMatrix axis (dgGetIdentityMatrix());
	dgSphere sphere (axis);
	dgConvexHull3d convexHull (&pool[0].m_x, sizeof (dgBigVector), vertexCountIndex, 0.0f);
	if (convexHull.GetCount()) {
		dgStack<int32_t> triangleList (convexHull.GetCount() * 3); 				
		int32_t trianglesCount = 0;
		for (dgConvexHull3d::dgListNode* node = convexHull.GetFirst(); node; node = node->GetNext()) {
			dgConvexHull3DFace* const face = &node->GetInfo();
			triangleList[trianglesCount * 3 + 0] = face->m_index[0];
			triangleList[trianglesCount * 3 + 1] = face->m_index[1];
			triangleList[trianglesCount * 3 + 2] = face->m_index[2];
			trianglesCount ++;
			HACD_ASSERT ((trianglesCount * 3) <= triangleList.GetElementsCount());
		}

		dgVector* const dst = (dgVector*) &pool[0].m_x;
		for (int32_t i = 0; i < convexHull.GetVertexCount(); i ++) {
			dst[i] = convexHull.GetVertex(i);
		}
		sphere.SetDimensions (&dst[0].m_x, sizeof (dgVector), &triangleList[0], trianglesCount * 3, NULL);

	} else if (vertexCountIndex >= 3) {
		dgStack<int32_t> triangleList (GetCount() * 3 * 2); 
		int32_t mark = IncLRU();
		int32_t trianglesCount = 0;
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &(*iter);
			if (edge->m_mark != mark) {
				dgEdge* ptr = edge;
				do {
					ptr->m_mark = mark;
					ptr = ptr->m_twin->m_next;
				} while (ptr != edge);

				ptr = edge->m_next->m_next;
				do {
					triangleList[trianglesCount * 3 + 0] = edge->m_incidentVertex;
					triangleList[trianglesCount * 3 + 1] = ptr->m_prev->m_incidentVertex;
					triangleList[trianglesCount * 3 + 2] = ptr->m_incidentVertex;
					trianglesCount ++;
					HACD_ASSERT ((trianglesCount * 3) <= triangleList.GetElementsCount());
					ptr = ptr->m_twin->m_next;
				} while (ptr != edge);

				dgVector* const dst = (dgVector*) &pool[0].m_x;
				for (int32_t i = 0; i < vertexCountIndex; i ++) {
					dst[i] = pool[i];
				}
				sphere.SetDimensions (&dst[0].m_x, sizeof (dgVector), &triangleList[0], trianglesCount * 3, NULL);
			}
		}
	}
	return sphere;

}
