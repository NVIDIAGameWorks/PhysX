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

#include "dgMeshEffect.h"
#include "dgConvexHull3d.h"
#include "dgStack.h"
#include "dgSphere.h"
#include "dgGraph.h"
#include "dgAABBPolygonSoup.h"
#include "dgPolygonSoupBuilder.h"
#include "SparseArray.h"

#include <string.h>

#if PX_X86
#define PTR_TO_UINT64(x) ((uint64_t)(uint32_t)(x))
#else
#define PTR_TO_UINT64(x) ((uint64_t)(x))
#endif


class dgFlatClipEdgeAttr
{
	public:
	int32_t m_rightIndex;
	int32_t m_leftIndex;
	int32_t m_leftEdgeAttr;
	int32_t m_leftTwinAttr;
	int32_t m_rightEdgeAttr;
	int32_t m_rightTwinAttr;
	dgEdge* m_edge;
	dgEdge* m_twin;
};

dgMeshEffect::dgMeshEffect(bool preAllocaBuffers)
{
	Init(preAllocaBuffers);
}

dgMeshEffect::dgMeshEffect (const dgMatrix& planeMatrix, float witdth, float breadth, int32_t material, const dgMatrix& textureMatrix0, const dgMatrix& textureMatrix1)
{
	int32_t index[4];
	int64_t attrIndex[4];
	dgBigVector face[4];

	Init(true);

	face[0] = dgBigVector (float (0.0f), -witdth, -breadth, float (0.0f));
	face[1] = dgBigVector (float (0.0f),  witdth, -breadth, float (0.0f));
	face[2] = dgBigVector (float (0.0f),  witdth,  breadth, float (0.0f));
	face[3] = dgBigVector (float (0.0f), -witdth,  breadth, float (0.0f));

	for (int32_t i = 0; i < 4; i ++) {
		dgBigVector uv0 (textureMatrix0.TransformVector(face[i]));
		dgBigVector uv1 (textureMatrix1.TransformVector(face[i]));

		m_points[i] = planeMatrix.TransformVector(face[i]);

		m_attib[i].m_vertex.m_x = m_points[i].m_x;
		m_attib[i].m_vertex.m_y = m_points[i].m_y;
		m_attib[i].m_vertex.m_z = m_points[i].m_z;
		m_attib[i].m_vertex.m_w = double (0.0f);

		m_attib[i].m_normal_x = planeMatrix.m_front.m_x;
		m_attib[i].m_normal_y = planeMatrix.m_front.m_y;
		m_attib[i].m_normal_z = planeMatrix.m_front.m_z;
		
		m_attib[i].m_u0 = uv0.m_y;
		m_attib[i].m_v0 = uv0.m_z;

		m_attib[i].m_u1 = uv1.m_y;
		m_attib[i].m_v1 = uv1.m_z;

		m_attib[i].m_material = material;

		index[i] = i;
		attrIndex[i] = i;
	}

	m_pointCount = 4;
	m_atribCount = 4;
	BeginFace();
	AddFace (4, index, attrIndex);
	EndFace();
}


dgMeshEffect::dgMeshEffect(dgPolyhedra& mesh, const dgMeshEffect& source)
	:dgPolyhedra (mesh) 
{
	m_pointCount = source.m_pointCount;
	m_maxPointCount = source.m_maxPointCount;
	m_points = (dgBigVector*) HACD_ALLOC(size_t (m_maxPointCount * sizeof(dgBigVector)));
	memcpy (m_points, source.m_points, m_pointCount * sizeof(dgBigVector));

	m_atribCount = source.m_atribCount;
	m_maxAtribCount = source.m_maxAtribCount;
	m_attib = (dgVertexAtribute*) HACD_ALLOC(size_t (m_maxAtribCount * sizeof(dgVertexAtribute)));
	memcpy (m_attib, source.m_attib, m_atribCount * sizeof(dgVertexAtribute));
}


dgMeshEffect::dgMeshEffect(const dgMeshEffect& source)
	:dgPolyhedra (source) 
{
	m_pointCount = source.m_pointCount;
	m_maxPointCount = source.m_maxPointCount;
	m_points = (dgBigVector*) HACD_ALLOC(size_t (m_maxPointCount * sizeof(dgBigVector)));
	memcpy (m_points, source.m_points, m_pointCount * sizeof(dgBigVector));

	m_atribCount = source.m_atribCount;
	m_maxAtribCount = source.m_maxAtribCount;
	m_attib = (dgVertexAtribute*) HACD_ALLOC(size_t (m_maxAtribCount * sizeof(dgVertexAtribute)));
	memcpy (m_attib, source.m_attib, m_atribCount * sizeof(dgVertexAtribute));
}


dgMeshEffect::~dgMeshEffect(void)
{
	HACD_FREE (m_points);
	HACD_FREE (m_attib);
}

void dgMeshEffect::Init(bool preAllocaBuffers)
{
	m_pointCount = 0;
	m_atribCount = 0;
	m_maxPointCount = DG_MESH_EFFECT_INITIAL_VERTEX_SIZE;
	m_maxAtribCount = DG_MESH_EFFECT_INITIAL_VERTEX_SIZE;

	m_points = NULL;
	m_attib = NULL;
	if (preAllocaBuffers) {
		m_points = (dgBigVector*) HACD_ALLOC(size_t (m_maxPointCount * sizeof(dgBigVector)));
		m_attib = (dgVertexAtribute*) HACD_ALLOC(size_t (m_maxAtribCount * sizeof(dgVertexAtribute)));
	}
}


void dgMeshEffect::Triangulate  ()
{
	dgPolyhedra polygon;

	int32_t mark = IncLRU();
	polygon.BeginFace();
	dgPolyhedra::Iterator iter (*this);
	for (iter.Begin(); iter; iter ++){
		dgEdge* const face = &(*iter);

		if ((face->m_mark != mark) && (face->m_incidentFace > 0)) {
			int32_t	index[DG_MESH_EFFECT_POINT_SPLITED];

			dgEdge* ptr = face;
			int32_t indexCount = 0;
			do {
				int32_t attribIndex = int32_t (ptr->m_userData);
				m_attib[attribIndex].m_vertex.m_w = double (ptr->m_incidentVertex);
				ptr->m_mark = mark;
				index[indexCount] = attribIndex;
				indexCount ++;
				ptr = ptr->m_next;
			} while (ptr != face);
			polygon.AddFace(indexCount, index);
		}
	}
	polygon.EndFace();


	dgPolyhedra leftOversOut;
	polygon.Triangulate(&m_attib[0].m_vertex.m_x, sizeof (dgVertexAtribute), &leftOversOut);
	HACD_ASSERT (leftOversOut.GetCount() == 0);


	RemoveAll();
	SetLRU (0);

	mark = polygon.IncLRU();
	BeginFace();
	dgPolyhedra::Iterator iter1 (polygon);
	for (iter1.Begin(); iter1; iter1 ++){
		dgEdge* const face = &(*iter1);
		if ((face->m_mark != mark) && (face->m_incidentFace > 0)) {
			int32_t	index[DG_MESH_EFFECT_POINT_SPLITED];
			int64_t	userData[DG_MESH_EFFECT_POINT_SPLITED];

			dgEdge* ptr = face;
			int32_t indexCount = 0;
			do {
				ptr->m_mark = mark;
				index[indexCount] = int32_t (m_attib[ptr->m_incidentVertex].m_vertex.m_w);

				userData[indexCount] = ptr->m_incidentVertex;
				indexCount ++;
				ptr = ptr->m_next;
			} while (ptr != face);
			AddFace(indexCount, index, userData);
		}
	}
	EndFace();

	for (iter.Begin(); iter; iter ++){
		dgEdge* const face = &(*iter);
		if (face->m_incidentFace > 0) {
			int32_t attribIndex = int32_t (face->m_userData);
			m_attib[attribIndex].m_vertex.m_w = m_points[face->m_incidentVertex].m_w;
		}
	}

}

void dgMeshEffect::ConvertToPolygons ()
{
	dgPolyhedra polygon;

	int32_t mark = IncLRU();
	polygon.BeginFace();
	dgPolyhedra::Iterator iter (*this);
	for (iter.Begin(); iter; iter ++){
		dgEdge* const face = &(*iter);
		if ((face->m_mark != mark) && (face->m_incidentFace > 0)) {
			int32_t	index[DG_MESH_EFFECT_POINT_SPLITED];

			dgEdge* ptr = face;
			int32_t indexCount = 0;
			do {
				int32_t attribIndex = int32_t (ptr->m_userData);

				m_attib[attribIndex].m_vertex.m_w = float (ptr->m_incidentVertex);
				ptr->m_mark = mark;
				index[indexCount] = attribIndex;
				indexCount ++;
				ptr = ptr->m_next;
			} while (ptr != face);
			polygon.AddFace(indexCount, index);
		}
	}
	polygon.EndFace();

	dgPolyhedra leftOversOut;
	polygon.ConvexPartition (&m_attib[0].m_vertex.m_x, sizeof (dgVertexAtribute), &leftOversOut);
	HACD_ASSERT (leftOversOut.GetCount() == 0);

	RemoveAll();
	SetLRU (0);

	mark = polygon.IncLRU();
	BeginFace();
	dgPolyhedra::Iterator iter1 (polygon);
	for (iter1.Begin(); iter1; iter1 ++){
		dgEdge* const face = &(*iter1);
		if ((face->m_mark != mark) && (face->m_incidentFace > 0)) {
			int32_t	index[DG_MESH_EFFECT_POINT_SPLITED];
			int64_t	userData[DG_MESH_EFFECT_POINT_SPLITED];

			dgEdge* ptr = face;
			int32_t indexCount = 0;
			do {
				ptr->m_mark = mark;
				index[indexCount] = int32_t (m_attib[ptr->m_incidentVertex].m_vertex.m_w);

				userData[indexCount] = ptr->m_incidentVertex;
				indexCount ++;
				ptr = ptr->m_next;
			} while (ptr != face);
			AddFace(indexCount, index, userData);
		}
	}
	EndFace();

	for (iter.Begin(); iter; iter ++){
		dgEdge* const face = &(*iter);
		if (face->m_incidentFace > 0) {
			int32_t attribIndex = int32_t (face->m_userData);
			m_attib[attribIndex].m_vertex.m_w = m_points[face->m_incidentVertex].m_w;
		}
	}

	RepairTJoints (false);
}

void dgMeshEffect::RemoveUnusedVertices(int32_t* const vertexMap)
{
	dgPolyhedra polygon;
	dgStack<int32_t>attrbMap(m_atribCount);

	memset(&vertexMap[0], -1, m_pointCount * sizeof (int));
	memset(&attrbMap[0], -1, m_atribCount * sizeof (int));

	int attribCount = 0;
	int vertexCount = 0;

	dgStack<dgBigVector>points (m_pointCount);
	dgStack<dgVertexAtribute>atributes (m_atribCount);

	int32_t mark = IncLRU();
	polygon.BeginFace();
	dgPolyhedra::Iterator iter (*this);
	for (iter.Begin(); iter; iter ++){
		dgEdge* const face = &(*iter);
		if ((face->m_mark != mark) && (face->m_incidentFace > 0)) {
			int32_t	vertex[DG_MESH_EFFECT_POINT_SPLITED];
			int64_t	userData[DG_MESH_EFFECT_POINT_SPLITED];
			int indexCount = 0;
			dgEdge* ptr = face;
			do {
				ptr->m_mark = mark;

				int index = ptr->m_incidentVertex;
				if (vertexMap[index] == -1) {
					vertexMap[index] = vertexCount;
					points[vertexCount] = m_points[index];
					vertexCount ++;
				}
				vertex[indexCount] = vertexMap[index];

				index = int (ptr->m_userData);
				if (attrbMap[index] == -1) {
					attrbMap[index] = attribCount;
					atributes[attribCount] = m_attib[index];
					attribCount ++;
				}
				userData[indexCount] = attrbMap[index];
				indexCount ++;

				ptr = ptr->m_next;
			} while (ptr != face);
			polygon.AddFace(indexCount, vertex, userData);
		}
	}
	polygon.EndFace();

	m_pointCount = vertexCount;
	memcpy (&m_points[0].m_x, &points[0].m_x, m_pointCount * sizeof (dgBigVector));
	 
	m_atribCount = attribCount;
	memcpy (&m_attib[0].m_vertex.m_x, &atributes[0].m_vertex.m_x, m_atribCount * sizeof (dgVertexAtribute));


	RemoveAll();
	SetLRU (0);

	BeginFace();
	dgPolyhedra::Iterator iter1 (polygon);
	for (iter1.Begin(); iter1; iter1 ++){
		dgEdge* const face = &(*iter1);
		if ((face->m_mark != mark) && (face->m_incidentFace > 0)) {
			int32_t	index[DG_MESH_EFFECT_POINT_SPLITED];
			int64_t	userData[DG_MESH_EFFECT_POINT_SPLITED];

			dgEdge* ptr = face;
			int32_t indexCount = 0;
			do {
				ptr->m_mark = mark;
				index[indexCount] = ptr->m_incidentVertex;
				userData[indexCount] = int64_t (ptr->m_userData);
				indexCount ++;
				ptr = ptr->m_next;
			} while (ptr != face);
			AddFace(indexCount, index, userData);
		}
	}
	EndFace();
	PackVertexArrays ();
}

dgMatrix dgMeshEffect::CalculateOOBB (dgBigVector& size) const
{
	dgSphere sphere (CalculateSphere (&m_points[0].m_x, sizeof (dgBigVector), NULL));
	size = sphere.m_size;

	dgMatrix permuation (dgGetIdentityMatrix());
	permuation[0][0] = float (0.0f);
	permuation[0][1] = float (1.0f);
	permuation[1][1] = float (0.0f);
	permuation[1][2] = float (1.0f);
	permuation[2][2] = float (0.0f);
	permuation[2][0] = float (1.0f);

	while ((size.m_x < size.m_y) || (size.m_x < size.m_z)) {
		sphere = permuation * sphere;
		size = permuation.UnrotateVector(size);
	}

	return sphere;
}

void dgMeshEffect::CalculateAABB (dgBigVector& minBox, dgBigVector& maxBox) const
{
	dgBigVector minP ( double (1.0e15f),  double (1.0e15f),  double (1.0e15f), double (0.0f)); 
	dgBigVector maxP (-double (1.0e15f), -double (1.0e15f), -double (1.0e15f), double (0.0f)); 

	dgPolyhedra::Iterator iter (*this);
	const dgBigVector* const points = &m_points[0];
	for (iter.Begin(); iter; iter ++){
		dgEdge* const edge = &(*iter);
		const dgBigVector& p (points[edge->m_incidentVertex]);

		minP.m_x = GetMin (p.m_x, minP.m_x); 
		minP.m_y = GetMin (p.m_y, minP.m_y); 
		minP.m_z = GetMin (p.m_z, minP.m_z); 

		maxP.m_x = GetMax (p.m_x, maxP.m_x); 
		maxP.m_y = GetMax (p.m_y, maxP.m_y); 
		maxP.m_z = GetMax (p.m_z, maxP.m_z); 
	}

	minBox = minP;
	maxBox = maxP;
}

int32_t dgMeshEffect::EnumerateAttributeArray (dgVertexAtribute* const attib)
{
	int32_t index = 0;
	dgPolyhedra::Iterator iter (*this);	
	for(iter.Begin(); iter; iter ++){
		dgEdge* const edge = &(*iter);
		HACD_ASSERT (index < GetCount());
		attib[index] = m_attib[int32_t (edge->m_userData)];
		edge->m_userData = uint64_t (index);
		index ++;
	}
	return index;
}

void dgMeshEffect::ApplyAttributeArray (dgVertexAtribute* const attib, int32_t maxCount)
{
	dgStack<int32_t>indexMap (GetCount());

	m_atribCount = dgVertexListToIndexList (&attib[0].m_vertex.m_x, sizeof (dgVertexAtribute), sizeof (dgVertexAtribute) / sizeof(double), maxCount, &indexMap[0], DG_VERTEXLIST_INDEXLIST_TOL);
	m_maxAtribCount = m_atribCount;

	HACD_FREE (m_attib);
	m_attib = (dgVertexAtribute*)HACD_ALLOC(size_t (m_atribCount * sizeof(dgVertexAtribute)));
	memcpy (m_attib, attib, m_atribCount * sizeof(dgVertexAtribute));

	dgPolyhedra::Iterator iter (*this);	
	for(iter.Begin(); iter; iter ++){
		dgEdge* const edge = &(*iter);
		if (edge->m_incidentFace > 0) {
			int32_t index = indexMap[int32_t (edge->m_userData)];
			HACD_ASSERT (index >= 0);
			HACD_ASSERT (index < m_atribCount);
			edge->m_userData = uint64_t (index);
		}
	}
}



dgBigVector dgMeshEffect::GetOrigin ()const
{
	dgBigVector origin (double (0.0f), double (0.0f), double (0.0f), double (0.0f));	
	for (int32_t i = 0; i < m_pointCount; i ++) {
		origin += m_points[i];
	}	
	return origin.Scale (double (1.0f) / m_pointCount);
}


void dgMeshEffect::FixCylindricalMapping (dgVertexAtribute* attribArray) const
{
	dgPolyhedra::Iterator iter (*this);	
	for(iter.Begin(); iter; iter ++){
		dgEdge* const edge = &(*iter);
		dgVertexAtribute& attrib0 = attribArray[int32_t (edge->m_userData)];
		dgVertexAtribute& attrib1 = attribArray[int32_t (edge->m_next->m_userData)];

		double error = fabs (attrib0.m_u0 - attrib1.m_u0);
		if (error > float (0.6f)) {
			if (attrib0.m_u0 < attrib1.m_u0) {
				attrib0.m_u0 += float (1.0f);
				attrib0.m_u1 = attrib0.m_u0;
			} else {
				attrib1.m_u0 += float (1.0f);
				attrib1.m_u1 = attrib1.m_u0;
			}
			
		}
	}

	for(iter.Begin(); iter; iter ++){
		dgEdge* const edge = &(*iter);
		dgVertexAtribute& attrib0 = attribArray[int32_t (edge->m_userData)];
		dgVertexAtribute& attrib1 = attribArray[int32_t (edge->m_next->m_userData)];

		double error = fabs (attrib0.m_u0 - attrib1.m_u0);
		if (error > float (0.6f)) {
			if (attrib0.m_u0 < attrib1.m_u0) {
				attrib0.m_u0 += float (1.0f);
				attrib0.m_u1 = attrib0.m_u0;
			} else {
				attrib1.m_u0 += float (1.0f);
				attrib1.m_u1 = attrib1.m_u0;
			}
		}
	}
}


void dgMeshEffect::SphericalMapping (int32_t material)
{
	dgBigVector origin (GetOrigin());

	dgStack<dgBigVector>sphere (m_pointCount);
	for (int32_t i = 0; i < m_pointCount; i ++) {
		dgBigVector point (m_points[i] - origin);
		point = point.Scale (1.0f / dgSqrt (point % point));

		double u = dgAsin (point.m_y);
		double v = dgAtan2 (point.m_x, point.m_z);

		u = (double (3.1416f/2.0f) - u) / double (3.1416f);
		v = (double (3.1416f) - v) / double (2.0f * 3.1416f);
		sphere[i].m_x = v;
		sphere[i].m_y = u;
	}


	dgStack<dgVertexAtribute>attribArray (GetCount());
	int32_t count = EnumerateAttributeArray (&attribArray[0]);

	dgPolyhedra::Iterator iter (*this);	
	for(iter.Begin(); iter; iter ++){
		dgEdge* const edge = &(*iter);
		dgVertexAtribute& attrib = attribArray[int32_t (edge->m_userData)];
		attrib.m_u0 = sphere[edge->m_incidentVertex].m_x;
		attrib.m_v0 = sphere[edge->m_incidentVertex].m_y;
		attrib.m_u1 = sphere[edge->m_incidentVertex].m_x;
		attrib.m_v1 = sphere[edge->m_incidentVertex].m_y;
		attrib.m_material = material;
	}

	FixCylindricalMapping (&attribArray[0]);
	ApplyAttributeArray (&attribArray[0],count);
}

void dgMeshEffect::CylindricalMapping (int32_t cylinderMaterial, int32_t capMaterial)
{
/*
	dgVector origin (GetOrigin());

	dgStack<dgVector>cylinder (m_pointCount);

	float xMax;
	float xMin;

	xMin= float (1.0e10f);
	xMax= float (-1.0e10f);
	for (int32_t i = 0; i < m_pointCount; i ++) {
		cylinder[i] = m_points[i] - origin;
		xMin = GetMin (xMin, cylinder[i].m_x);
		xMax = GetMax (xMax, cylinder[i].m_x);
	}

	float xscale = float (1.0f)/ (xMax - xMin);
	for (int32_t i = 0; i < m_pointCount; i ++) {
		float u;
		float v;
		float y;
		float z;
		y = cylinder[i].m_y;
		z = cylinder[i].m_z;
		u = dgAtan2 (z, y);
		if (u < float (0.0f)) {
			u += float (3.141592f * 2.0f);
		}
		v = (cylinder[i].m_x - xMin) * xscale;

		cylinder[i].m_x = float (1.0f) - u * float (1.0f / (2.0f * 3.141592f));
		cylinder[i].m_y = v;
	}

	dgStack<dgVertexAtribute>attribArray (GetCount());
	EnumerateAttributeArray (&attribArray[0]);

	dgPolyhedra::Iterator iter (*this);	
	for(iter.Begin(); iter; iter ++){
		dgEdge* const edge;
		edge = &(*iter);
		dgVertexAtribute& attrib = attribArray[int32_t (edge->m_userData)];
		attrib.m_u0 = cylinder[edge->m_incidentVertex].m_x;
		attrib.m_v0 = cylinder[edge->m_incidentVertex].m_y;
		attrib.m_u1 = cylinder[edge->m_incidentVertex].m_x;
		attrib.m_v1 = cylinder[edge->m_incidentVertex].m_y;
		attrib.m_material = cylinderMaterial;
	}

	FixCylindricalMapping (&attribArray[0]);

	int32_t mark;
	mark = IncLRU();
	for(iter.Begin(); iter; iter ++){
		dgEdge* const edge;
		edge = &(*iter);
		if (edge->m_mark < mark){
			const dgVector& p0 = m_points[edge->m_incidentVertex];
			const dgVector& p1 = m_points[edge->m_next->m_incidentVertex];
			const dgVector& p2 = m_points[edge->m_prev->m_incidentVertex];

			edge->m_mark = mark;
			edge->m_next->m_mark = mark;
			edge->m_prev->m_mark = mark;

			dgVector e0 (p1 - p0);
			dgVector e1 (p2 - p0);
			dgVector n (e0 * e1);
			if ((n.m_x * n.m_x) > (float (0.99f) * (n % n))) {
				dgEdge* ptr;

				ptr = edge;
				do {
					dgVertexAtribute& attrib = attribArray[int32_t (ptr->m_userData)];
					dgVector p (m_points[ptr->m_incidentVertex] - origin);
					p.m_x = float (0.0f);
					p = p.Scale (float (dgRsqrt(p % p)));
					attrib.m_u0 = float (0.5f) + p.m_y * float (0.5f);
					attrib.m_v0 = float (0.5f) + p.m_z * float (0.5f);
					attrib.m_u1 = float (0.5f) + p.m_y * float (0.5f);
					attrib.m_v1 = float (0.5f) + p.m_z * float (0.5f);
					attrib.m_material = capMaterial;

					ptr = ptr->m_next;
				}while (ptr !=  edge);
			}
		}
	}

	ApplyAttributeArray (&attribArray[0]);
*/

	dgBigVector origin (GetOrigin());
	dgStack<dgBigVector>cylinder (m_pointCount);

	dgBigVector pMin (double (1.0e10f), double (1.0e10f), double (1.0e10f), double (0.0f));
	dgBigVector pMax (double (-1.0e10f), double (-1.0e10f), double (-1.0e10f), double (0.0f));
	for (int32_t i = 0; i < m_pointCount; i ++) {
		dgBigVector tmp (m_points[i] - origin);
		pMin.m_x = GetMin (pMin.m_x, tmp.m_x);
		pMax.m_x = GetMax (pMax.m_x, tmp.m_x);
		pMin.m_y = GetMin (pMin.m_y, tmp.m_y);
		pMax.m_y = GetMax (pMax.m_y, tmp.m_y);
		pMin.m_z = GetMin (pMin.m_z, tmp.m_z);
		pMax.m_z = GetMax (pMax.m_z, tmp.m_z);
	}

	dgBigVector scale (double (1.0f)/ (pMax.m_x - pMin.m_x), double (1.0f)/ (pMax.m_x - pMin.m_x), double (1.0f)/ (pMax.m_x - pMin.m_x), double (0.0f));
	for (int32_t i = 0; i < m_pointCount; i ++) {
		dgBigVector point (m_points[i] - origin);
		double u = (point.m_x - pMin.m_x) * scale.m_x;

		point = point.Scale (1.0f / dgSqrt (point % point));
		double v = dgAtan2 (point.m_y, point.m_z);

		//u = (double (3.1416f/2.0f) - u) / double (3.1416f);
		v = (double (3.1416f) - v) / double (2.0f * 3.1416f);
		cylinder[i].m_x = v;
		cylinder[i].m_y = u;
	}


	dgStack<dgVertexAtribute>attribArray (GetCount());
	int32_t count = EnumerateAttributeArray (&attribArray[0]);

	dgPolyhedra::Iterator iter (*this);	
	for(iter.Begin(); iter; iter ++){
		dgEdge* const edge = &(*iter);
		dgVertexAtribute& attrib = attribArray[int32_t (edge->m_userData)];
		attrib.m_u0 = cylinder[edge->m_incidentVertex].m_x;
		attrib.m_v0 = cylinder[edge->m_incidentVertex].m_y;
		attrib.m_u1 = cylinder[edge->m_incidentVertex].m_x;
		attrib.m_v1 = cylinder[edge->m_incidentVertex].m_y;
		attrib.m_material = cylinderMaterial;
	}

	FixCylindricalMapping (&attribArray[0]);

	// apply cap mapping
	int32_t mark = IncLRU();
	for(iter.Begin(); iter; iter ++){
		dgEdge* const edge = &(*iter);
		if (edge->m_mark < mark){
			const dgVector& p0 = m_points[edge->m_incidentVertex];
			const dgVector& p1 = m_points[edge->m_next->m_incidentVertex];
			const dgVector& p2 = m_points[edge->m_prev->m_incidentVertex];

			edge->m_mark = mark;
			edge->m_next->m_mark = mark;
			edge->m_prev->m_mark = mark;

			dgVector e0 (p1 - p0);
			dgVector e1 (p2 - p0);
			dgVector n (e0 * e1);
			if ((n.m_x * n.m_x) > (float (0.99f) * (n % n))) {
				dgEdge* ptr = edge;
				do {
					dgVertexAtribute& attrib = attribArray[int32_t (ptr->m_userData)];
					dgVector p (m_points[ptr->m_incidentVertex] - origin);
					double u = (p.m_y - pMin.m_y) * scale.m_y;
					double v = (p.m_z - pMin.m_z) * scale.m_z;
					attrib.m_u0 = u;
					attrib.m_v0 = v;
					attrib.m_u1 = u;
					attrib.m_v1 = v;
					attrib.m_material = capMaterial;

					ptr = ptr->m_next;
				}while (ptr !=  edge);
			}
		}
	}

	ApplyAttributeArray (&attribArray[0],count);
}

void dgMeshEffect::BoxMapping (int32_t front, int32_t side, int32_t top)
{
	dgBigVector minVal;
	dgBigVector maxVal;
	int32_t materialArray[3];

	GetMinMax (minVal, maxVal, &m_points[0][0], m_pointCount, sizeof (dgBigVector));
	dgBigVector dist (maxVal - minVal);
	dgBigVector scale (double (1.0f)/ dist[0], double (1.0f)/ dist[1], double (1.0f)/ dist[2], double (0.0f));

	dgStack<dgVertexAtribute>attribArray (GetCount());
	int32_t count = EnumerateAttributeArray (&attribArray[0]);

	materialArray[0] = front;
	materialArray[1] = side;
	materialArray[2] = top;

	int32_t mark = IncLRU();
	dgPolyhedra::Iterator iter (*this);	
	for(iter.Begin(); iter; iter ++){
		dgEdge* const edge = &(*iter);
		if (edge->m_mark < mark){
			const dgBigVector& p0 = m_points[edge->m_incidentVertex];
			const dgBigVector& p1 = m_points[edge->m_next->m_incidentVertex];
			const dgBigVector& p2 = m_points[edge->m_prev->m_incidentVertex];

			edge->m_mark = mark;
			edge->m_next->m_mark = mark;
			edge->m_prev->m_mark = mark;

			dgBigVector e0 (p1 - p0);
			dgBigVector e1 (p2 - p0);
			dgBigVector n (e0 * e1);
			
			int32_t index = 0;
			double maxProjection = float (0.0f);

			for (int32_t i = 0; i < 3; i ++) {
				double proj = fabs (n[i]);
				if (proj > maxProjection) {
					index = i;
					maxProjection = proj;
				}
			}

			int32_t u = (index + 1) % 3;
			int32_t v = (u + 1) % 3;
			if (index == 1) {
				Swap (u, v);
			}
			dgEdge* ptr = edge;
			do {
				dgVertexAtribute& attrib = attribArray[int32_t (ptr->m_userData)];
				dgBigVector p (scale.CompProduct(m_points[ptr->m_incidentVertex] - minVal));
				attrib.m_u0 = p[u];
				attrib.m_v0 = p[v];
				attrib.m_u1 = double(0.0f);
				attrib.m_v1 = double(0.0f);
				attrib.m_material = materialArray[index];

				ptr = ptr->m_next;
			}while (ptr !=  edge);
		}
	}

	ApplyAttributeArray (&attribArray[0],count);
}

void dgMeshEffect::UniformBoxMapping (int32_t material, const dgMatrix& textureMatrix)
{
	dgStack<dgVertexAtribute>attribArray (GetCount());
	int32_t count = EnumerateAttributeArray (&attribArray[0]);

	int32_t mark = IncLRU();
	for (int32_t i = 0; i < 3; i ++) {
		dgMatrix rotationMatrix (dgGetIdentityMatrix());
		if (i == 1) {
			rotationMatrix = dgYawMatrix(float (90.0f * 3.1416f / 180.0f));
		} else if (i == 2) {
			rotationMatrix = dgPitchMatrix(float (90.0f * 3.1416f / 180.0f));
		}

		dgPolyhedra::Iterator iter (*this);	

		for(iter.Begin(); iter; iter ++){
			dgEdge* const edge = &(*iter);
			if (edge->m_mark < mark){
				dgBigVector n (FaceNormal(edge, &m_points[0].m_x, sizeof (dgBigVector)));
				dgVector normal (rotationMatrix.RotateVector(dgVector (n.Scale (double (1.0f) / sqrt (n % n)))));
				normal.m_x = dgAbsf (normal.m_x);
				normal.m_y = dgAbsf (normal.m_y);
				normal.m_z = dgAbsf (normal.m_z);
				if ((normal.m_z >= (normal.m_x - float (1.0e-4f))) && (normal.m_z >= (normal.m_y - float (1.0e-4f)))) {
					dgEdge* ptr = edge;
					do {
						ptr->m_mark = mark;
						dgVertexAtribute& attrib = attribArray[int32_t (ptr->m_userData)];
						dgVector p (textureMatrix.TransformVector(rotationMatrix.RotateVector(m_points[ptr->m_incidentVertex])));
						attrib.m_u0 = p.m_x;
						attrib.m_v0 = p.m_y;
						attrib.m_u1 = float (0.0f);
						attrib.m_v1 = float (0.0f);
						attrib.m_material = material;
						ptr = ptr->m_next;
					}while (ptr !=  edge);
				}
			}
		}
	}

	ApplyAttributeArray (&attribArray[0],count);
}

void dgMeshEffect::CalculateNormals (double angleInRadians)
{
	dgStack<dgBigVector>faceNormal (GetCount());
	dgStack<dgVertexAtribute>attribArray (GetCount());
	int32_t count = EnumerateAttributeArray (&attribArray[0]);

	int32_t faceIndex = 1;
	int32_t mark = IncLRU();
	dgPolyhedra::Iterator iter (*this);	
	for(iter.Begin(); iter; iter ++){
		dgEdge* const edge = &(*iter);
		if ((edge->m_mark < mark) && (edge->m_incidentFace > 0)) {
			dgEdge* ptr = edge;
			do {
				ptr->m_incidentFace = faceIndex;
				ptr->m_mark = mark;
				ptr = ptr->m_next;
			} while (ptr !=  edge);

			dgBigVector normal (FaceNormal (edge, &m_points[0].m_x, sizeof (m_points[0])));
			normal = normal.Scale (float (1.0f) / (sqrt(normal % normal) + float(1.0e-16f)));
			faceNormal[faceIndex] = normal;
			faceIndex ++;
		}
	}


	float smoothValue = dgCos (angleInRadians); 
//smoothValue = -1;
	for(iter.Begin(); iter; iter ++){
		dgEdge* const edge = &(*iter);
		if (edge->m_incidentFace > 0) {
			dgBigVector normal0 (faceNormal[edge->m_incidentFace]);

			dgEdge* startEdge = edge;
			for (dgEdge* ptr = edge->m_prev->m_twin ; (ptr != edge) && (ptr->m_incidentFace > 0); ptr = ptr->m_prev->m_twin) {
				const dgBigVector& normal1 (faceNormal[ptr->m_incidentFace]);
				double dot = normal0 % normal1;
				if (dot < smoothValue) {
					break;
				}
				startEdge = ptr;
				normal0 = normal1;
			}

			dgBigVector normal (faceNormal[startEdge->m_incidentFace]);
			normal0 = normal;
			for (dgEdge* ptr = startEdge->m_twin->m_next; (ptr != startEdge) && (ptr->m_incidentFace > 0); ptr = ptr->m_twin->m_next) { 
				
				const dgBigVector& normal1 (faceNormal[ptr->m_incidentFace]);
				double dot = normal0 % normal1;
				if (dot < smoothValue)  {
					break;
				}
				normal += normal1;
				normal0 = normal1;
			} 

			normal = normal.Scale (float (1.0f) / (sqrt(normal % normal) + float(1.0e-16f)));
			int32_t edgeIndex = int32_t (edge->m_userData);
			dgVertexAtribute& attrib = attribArray[edgeIndex];
			attrib.m_normal_x = normal.m_x;
			attrib.m_normal_y = normal.m_y;
			attrib.m_normal_z = normal.m_z;
		}
	}

	ApplyAttributeArray (&attribArray[0],count);
}


void dgMeshEffect::BeginPolygon ()
{
	m_pointCount = 0;
	m_atribCount = 0;
	RemoveAll();
	BeginFace();
}


void dgMeshEffect::AddAtribute (const dgVertexAtribute& attib)
{
	if (m_atribCount >= m_maxAtribCount) {
		m_maxAtribCount *= 2;
		dgVertexAtribute* const attibArray = (dgVertexAtribute*) HACD_ALLOC(size_t (m_maxAtribCount * sizeof(dgVertexAtribute)));
		memcpy (attibArray, m_attib, m_atribCount * sizeof(dgVertexAtribute));
		HACD_FREE(m_attib);
		m_attib = attibArray;
	}

	m_attib[m_atribCount] = attib;
	m_attib[m_atribCount].m_vertex.m_x = QuantizeCordinade(m_attib[m_atribCount].m_vertex.m_x);
	m_attib[m_atribCount].m_vertex.m_y = QuantizeCordinade(m_attib[m_atribCount].m_vertex.m_y);
	m_attib[m_atribCount].m_vertex.m_z = QuantizeCordinade(m_attib[m_atribCount].m_vertex.m_z);
	m_atribCount ++;
}

void dgMeshEffect::AddVertex(const dgBigVector& vertex)
{
	if (m_pointCount >= m_maxPointCount) {
		m_maxPointCount *= 2;
		dgBigVector* const points = (dgBigVector*) HACD_ALLOC(size_t (m_maxPointCount * sizeof(dgBigVector)));
		memcpy (points, m_points, m_pointCount * sizeof(dgBigVector));
		HACD_FREE(m_points);
		m_points = points;
	}
	
	m_points[m_pointCount].m_x = QuantizeCordinade(vertex[0]);
	m_points[m_pointCount].m_y = QuantizeCordinade(vertex[1]);
	m_points[m_pointCount].m_z = QuantizeCordinade(vertex[2]);
	m_points[m_pointCount].m_w = vertex.m_w;
	m_pointCount ++;
}


void dgMeshEffect::AddPoint(const double* vertex, int32_t material)
{
	dgVertexAtribute attib;
	AddVertex(dgBigVector (vertex[0], vertex[1], vertex[2], vertex[3]));
	
	attib.m_vertex.m_x = m_points[m_pointCount - 1].m_x;
	attib.m_vertex.m_y = m_points[m_pointCount - 1].m_y;
	attib.m_vertex.m_z = m_points[m_pointCount - 1].m_z;
	attib.m_vertex.m_w = m_points[m_pointCount - 1].m_w;

	attib.m_normal_x = vertex[4];
	attib.m_normal_y = vertex[5];
	attib.m_normal_z = vertex[6];
	attib.m_u0 = vertex[7];
	attib.m_v0 = vertex[8];
	attib.m_u1 = vertex[9];
	attib.m_v1 = vertex[10];
	attib.m_material = material;

	AddAtribute (attib);
}

void dgMeshEffect::PackVertexArrays ()
{
	if (m_maxPointCount > m_pointCount) {
		dgBigVector* const points = (dgBigVector*) HACD_ALLOC(size_t (m_pointCount * sizeof(dgBigVector)));
		memcpy (points, m_points, m_pointCount * sizeof(dgBigVector));
		HACD_FREE(m_points);
		m_points = points;
		m_maxPointCount = m_pointCount;
	}


	if (m_maxAtribCount > m_atribCount) {
		dgVertexAtribute* const attibArray = (dgVertexAtribute*) HACD_ALLOC(size_t (m_atribCount * sizeof(dgVertexAtribute)));
		memcpy (attibArray, m_attib, m_atribCount * sizeof(dgVertexAtribute));
		HACD_FREE(m_attib);
		m_attib = attibArray;
		m_maxAtribCount = m_atribCount;
	}
};


void dgMeshEffect::AddPolygon (int32_t count, const double* const vertexList, int32_t strideIndBytes, int32_t material)
{
	int32_t stride = int32_t (strideIndBytes / sizeof (double));

	if (count > 3) {
		dgPolyhedra polygon;
		int32_t indexList[256];
		HACD_ASSERT (count < int32_t (sizeof (indexList)/sizeof(indexList[0])));
		for (int32_t i = 0; i < count; i ++) {
			indexList[i] = i;
		}

		polygon.BeginFace();
		polygon.AddFace(count, indexList, NULL);
		polygon.EndFace();
		polygon.Triangulate(vertexList, strideIndBytes, NULL);

		int32_t mark = polygon.IncLRU();
		dgPolyhedra::Iterator iter (polygon);
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &iter.GetNode()->GetInfo();
			if ((edge->m_incidentFace > 0) && (edge->m_mark < mark)) {
				int32_t i0 = edge->m_incidentVertex;
				int32_t i1 = edge->m_next->m_incidentVertex;
				int32_t i2 = edge->m_next->m_next->m_incidentVertex;
				edge->m_mark = mark;
				edge->m_next->m_mark = mark;
				edge->m_next->m_next->m_mark = mark;

//				#ifdef _DEBUG
//					dgBigVector p0_ (&vertexList[i0 * stride]);
//					dgBigVector p1_ (&vertexList[i1 * stride]);
//					dgBigVector p2_ (&vertexList[i2 * stride]);
//					dgBigVector e1_ (p1_ - p0_);
//					dgBigVector e2_ (p2_ - p0_);
//					dgBigVector n_ (e1_ * e2_);
//					double mag2_ = n_ % n_;
//					HACD_ASSERT (mag2_ > float (DG_MESH_EFFECT_PRECISION_SCALE_INV * DG_MESH_EFFECT_PRECISION_SCALE_INV)); 
//				#endif

				AddPoint(vertexList + i0 * stride, material);
				AddPoint(vertexList + i1 * stride, material);
				AddPoint(vertexList + i2 * stride, material);
			}
		}

	} else {

		AddPoint(vertexList, material);
		AddPoint(vertexList + stride, material);
		AddPoint(vertexList + stride + stride, material);

		const dgBigVector& p0 = m_points[m_pointCount - 3];
		const dgBigVector& p1 = m_points[m_pointCount - 2];
		const dgBigVector& p2 = m_points[m_pointCount - 1];
		dgBigVector e1 (p1 - p0);
		dgBigVector e2 (p2 - p0);
		dgBigVector n (e1 * e2);
		double mag3 = n % n;
		if (mag3 < double (DG_MESH_EFFECT_PRECISION_SCALE_INV * DG_MESH_EFFECT_PRECISION_SCALE_INV)) {
			m_pointCount -= 3;
			m_atribCount -= 3;
		}
	}
}


void dgMeshEffect::AddPolygon (int32_t count, const float* const vertexList, int32_t strideIndBytes, int32_t material)
{
	dgVertexAtribute points[256];
	HACD_ASSERT (count < int32_t (sizeof (points)/sizeof (points[0])));

	uint32_t stride = (uint32_t)strideIndBytes / sizeof (float);
	for (uint32_t i = 0; i < (uint32_t)count; i ++) 
	{
		points[i].m_vertex.m_x = vertexList[i * stride + 0];
		points[i].m_vertex.m_y = vertexList[i * stride + 1];
		points[i].m_vertex.m_z = vertexList[i * stride + 2];
		points[i].m_vertex.m_w = vertexList[i * stride + 3];

		points[i].m_normal_x = vertexList[i * stride + 4];
		points[i].m_normal_y = vertexList[i * stride + 5];
		points[i].m_normal_z = vertexList[i * stride + 6];

		points[i].m_u0 = vertexList[i * stride + 7];
		points[i].m_v0 = vertexList[i * stride + 8];

		points[i].m_u1 = vertexList[i * stride + 9];
		points[i].m_u1 = vertexList[i * stride + 10];
	}

	AddPolygon (count, &points[0].m_vertex.m_x, sizeof (dgVertexAtribute), material);
}

void dgMeshEffect::EndPolygon (double tol)
{
	dgStack<int32_t>indexMap(m_pointCount);
	dgStack<int32_t>attrIndexMap(m_atribCount);

	int32_t triangCount = m_pointCount / 3;
	m_pointCount = dgVertexListToIndexList (&m_points[0].m_x, sizeof (dgBigVector), sizeof (dgBigVector)/sizeof (double), m_pointCount, &indexMap[0], tol);
	m_atribCount = dgVertexListToIndexList (&m_attib[0].m_vertex.m_x, sizeof (dgVertexAtribute), sizeof (dgVertexAtribute)/sizeof (double), m_atribCount, &attrIndexMap[0], tol);

	for (int32_t i = 0; i < triangCount; i ++) {
		int32_t index[3];
		int64_t userdata[3];

		index[0] = indexMap[i * 3 + 0];
		index[1] = indexMap[i * 3 + 1];
		index[2] = indexMap[i * 3 + 2];


		dgBigVector e1 (m_points[index[1]] - m_points[index[0]]);
		dgBigVector e2 (m_points[index[2]] - m_points[index[0]]);

		dgBigVector n (e1 * e2);
		double mag2 = n % n;
		if (mag2 > double (1.0e-12f)) {
			userdata[0] = attrIndexMap[i * 3 + 0];
			userdata[1] = attrIndexMap[i * 3 + 1];
			userdata[2] = attrIndexMap[i * 3 + 2];
			dgEdge* const edge = AddFace (3, index, userdata);
			if (!edge) {
				HACD_ASSERT ((m_pointCount + 3) <= m_maxPointCount);

				m_points[m_pointCount + 0] = m_points[index[0]];
				m_points[m_pointCount + 1] = m_points[index[1]];
				m_points[m_pointCount + 2] = m_points[index[2]];

				index[0] = m_pointCount + 0;
				index[1] = m_pointCount + 1;
				index[2] = m_pointCount + 2;

				m_pointCount += 3;

				AddFace (3, index, userdata);
			}
		}
	}
	EndFace();

	RepairTJoints (true);


}


void dgMeshEffect::BuildFromVertexListIndexList(
	int32_t faceCount, const int32_t* const faceIndexCount, const int32_t* const faceMaterialIndex, 
	const float* const vertex, int32_t vertexStrideInBytes, const int32_t* const vertexIndex,
	const float* const normal, int32_t  normalStrideInBytes, const int32_t* const normalIndex,
	const float* const uv0, int32_t  uv0StrideInBytes, const int32_t* const uv0Index,
	const float* const uv1, int32_t  uv1StrideInBytes, const int32_t* const uv1Index)
{
	BeginPolygon ();

	// calculate vertex Count
	int32_t acc = 0;
	int32_t vertexCount = 0;
	for (int32_t j = 0; j < faceCount; j ++) {
		int count = faceIndexCount[j];
		for (int i = 0; i < count; i ++) {
			vertexCount = GetMax(vertexCount, vertexIndex[acc + i] + 1);
		}
		acc += count;
	}
		
	int32_t layerCountBase = 0;
	int32_t vertexStride = int32_t (vertexStrideInBytes / sizeof (float));
	for (int i = 0; i < vertexCount; i ++) {
		int index = i * vertexStride;
		AddVertex (dgBigVector (vertex[index + 0], vertex[index + 1], vertex[index + 2], vertex[index + 3]));
		layerCountBase += (vertex[index + 3]) > float(layerCountBase);
	}


	acc = 0;
	int32_t normalStride = int32_t (normalStrideInBytes / sizeof (float));
	int32_t uv0Stride = int32_t (uv0StrideInBytes / sizeof (float));
	int32_t uv1Stride = int32_t (uv1StrideInBytes / sizeof (float));
	for (int32_t j = 0; j < faceCount; j ++) {
		int32_t indexCount = faceIndexCount[j];
		int32_t materialIndex = faceMaterialIndex[j];
		for (int32_t i = 0; i < indexCount; i ++) {
			dgVertexAtribute point;
			int32_t index = vertexIndex[acc + i];
			point.m_vertex = m_points[index];
			
			index = normalIndex[(acc + i)] * normalStride;
			point.m_normal_x =  normal[index + 0];
			point.m_normal_y =  normal[index + 1];
			point.m_normal_z =  normal[index + 2];

			index = uv0Index[(acc + i)] * uv0Stride;
			point.m_u0 = uv0[index + 0];
			point.m_v0 = uv0[index + 1];
			
			index = uv1Index[(acc + i)] * uv1Stride;
			point.m_u1 = uv1[index + 0];
			point.m_v1 = uv1[index + 1];

			point.m_material = materialIndex;
			AddAtribute(point);
		}
		acc += indexCount;
	}


	dgStack<int32_t>attrIndexMap(m_atribCount);
	m_atribCount = dgVertexListToIndexList (&m_attib[0].m_vertex.m_x, sizeof (dgVertexAtribute), sizeof (dgVertexAtribute) / sizeof (double), m_atribCount, &attrIndexMap[0], DG_VERTEXLIST_INDEXLIST_TOL);

	bool hasFaces = true;
	dgStack<int8_t> faceMark (faceCount);
	memset (&faceMark[0], 1, size_t (faceMark.GetSizeInBytes()));
	
	int32_t layerCount = 0;
	while (hasFaces) {
		acc = 0;
		hasFaces = false;
		int32_t vertexBank = layerCount * vertexCount;
		for (int32_t j = 0; j < faceCount; j ++) {
			int32_t index[256];
			int64_t userdata[256];

			int indexCount = faceIndexCount[j];
			HACD_ASSERT (indexCount < int32_t (sizeof (index) / sizeof (index[0])));

			if (faceMark[j]) {
				for (int i = 0; i < indexCount; i ++) {
					index[i] = vertexIndex[acc + i] + vertexBank;
					userdata[i] = attrIndexMap[acc + i];
				}
				dgEdge* const edge = AddFace (indexCount, index, userdata);
				if (edge) {
					faceMark[j] = 0;
				} else {
					// check if the face is not degenerated
					bool degeneratedFace = false;
					for (int i = 0; i < indexCount - 1; i ++) {
						for (int k = i + 1; k < indexCount; k ++) {
							if (index[i] == index[k]) {
								degeneratedFace = true;		
							}
						}
					}
					if (degeneratedFace) {
						faceMark[j] = 0;
					} else {
						hasFaces = true;
					}
				}
			}
			acc += indexCount;
		}
		if (hasFaces) {
			layerCount ++;
			for (int i = 0; i < vertexCount; i ++) {
				int index = i * vertexStride;
				AddVertex (dgBigVector (vertex[index + 0], vertex[index + 1], vertex[index + 2], double (layerCount + layerCountBase)));
			}
		}
	}

	EndFace();
	PackVertexArrays ();
//	RepairTJoints (true);
}


int32_t dgMeshEffect::GetTotalFaceCount() const
{
	return GetFaceCount();
}

int32_t dgMeshEffect::GetTotalIndexCount() const
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
		
		dgEdge* ptr = edge;
		do {
			count ++;
			ptr->m_mark = mark;
			ptr = ptr->m_next;
		} while (ptr != edge);
	}
	return count;
}

void dgMeshEffect::GetFaces (int32_t* const facesIndex, int32_t* const materials, void** const faceNodeList) const
{
	Iterator iter (*this);

	int32_t faces = 0;
	int32_t indexCount = 0;
	int32_t mark = IncLRU();
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if (edge->m_mark == mark) {
			continue;
		}

		if (edge->m_incidentFace < 0) {
			continue;
		}

		int32_t faceCount = 0;
		dgEdge* ptr = edge;
		do {
//			indexList[indexCount] = int32_t (ptr->m_userData);
			faceNodeList[indexCount] = GetNodeFromInfo (*ptr);
			indexCount ++;
			faceCount ++;
			ptr->m_mark = mark;
			ptr = ptr->m_next;
		} while (ptr != edge);

		facesIndex[faces] = faceCount;
		materials[faces] = dgFastInt(m_attib[int32_t (edge->m_userData)].m_material);
		faces ++;
	}
}

void* dgMeshEffect::GetFirstVertex ()
{
	Iterator iter (*this);
	iter.Begin();

	dgTreeNode* node = NULL;
	if (iter) {
		int32_t mark = IncLRU();
		node = iter.GetNode();

		dgEdge* const edge = &node->GetInfo();
		dgEdge* ptr = edge;
		do {
			ptr->m_mark = mark;
			ptr = ptr->m_twin->m_next;
		} while (ptr != edge);
	}
	return node; 
}

void* dgMeshEffect::GetNextVertex (const void* const vertex)
{
	dgTreeNode* node = (dgTreeNode*) vertex;
	int32_t mark = node->GetInfo().m_mark;

	Iterator iter (*this);
	iter.Set (node);
	for (iter ++; iter; iter ++) {
		dgTreeNode* node = iter.GetNode();
		if (node->GetInfo().m_mark != mark) {
			dgEdge* const edge = &node->GetInfo();
			dgEdge* ptr = edge;
			do {
				ptr->m_mark = mark;
				ptr = ptr->m_twin->m_next;
			} while (ptr != edge);
			return node; 
		}
	}
	return NULL; 
}

int32_t dgMeshEffect::GetVertexIndex (const void* const vertex) const
{
	dgTreeNode* const node = (dgTreeNode*) vertex;
	dgEdge* const edge = &node->GetInfo();
	return edge->m_incidentVertex;
}


void* dgMeshEffect::GetFirstPoint ()
{
	Iterator iter (*this);
	for (iter.Begin(); iter; iter ++) {
		dgTreeNode* const node = iter.GetNode();
		dgEdge* const edge = &node->GetInfo();
		if (edge->m_incidentFace > 0) {
			return node;
		}
	}
	return NULL; 
}

void* dgMeshEffect::GetNextPoint (const void* const point)
{
	Iterator iter (*this);
	iter.Set ((dgTreeNode*) point);
	for (iter ++; iter; iter ++) {
		dgTreeNode* const node = iter.GetNode();
		dgEdge* const edge = &node->GetInfo();
		if (edge->m_incidentFace > 0) {
			return node; 
		}
	}
	return NULL; 
}

int32_t dgMeshEffect::GetPointIndex (const void* const point) const
{
	dgTreeNode* const node = (dgTreeNode*) point;
	dgEdge* const edge = &node->GetInfo();
	return int (edge->m_userData);
}

int32_t dgMeshEffect::GetVertexIndexFromPoint (const void* const point) const
{
	return GetVertexIndex (point);
}


dgEdge* dgMeshEffect::ConectVertex (dgEdge* const e0, dgEdge* const e1)
{
	dgEdge* const edge = AddHalfEdge(e1->m_incidentVertex, e0->m_incidentVertex);
	dgEdge* const twin = AddHalfEdge(e0->m_incidentVertex, e1->m_incidentVertex);
	HACD_ASSERT ((edge && twin) || !(edge || twin));
	if (edge) {
		edge->m_twin = twin;
		twin->m_twin = edge;

		edge->m_incidentFace = e0->m_incidentFace;
		twin->m_incidentFace = e1->m_incidentFace;

		edge->m_userData = e1->m_userData;
		twin->m_userData = e0->m_userData;

		edge->m_next = e0;
		edge->m_prev = e1->m_prev;

		twin->m_next = e1;
		twin->m_prev = e0->m_prev;

		e0->m_prev->m_next = twin;
		e0->m_prev = edge;

		e1->m_prev->m_next = edge;
		e1->m_prev = twin;
	}

	return edge;
}


//int32_t dgMeshEffect::GetVertexAttributeIndex (const void* vertex) const
//{
//	dgTreeNode* node = (dgTreeNode*) vertex;
//	dgEdge* const edge = &node->GetInfo();
//	return int (edge->m_userData);
//}


void* dgMeshEffect::GetFirstEdge ()
{
	Iterator iter (*this);
	iter.Begin();

	dgTreeNode* node = NULL;
	if (iter) {
		int32_t mark = IncLRU();

		node = iter.GetNode();

		dgEdge* const edge = &node->GetInfo();
		edge->m_mark = mark;
		edge->m_twin->m_mark = mark;
	}
	return node; 
}

void* dgMeshEffect::GetNextEdge (const void* const edge)
{
	dgTreeNode* node = (dgTreeNode*) edge;
	int32_t mark = node->GetInfo().m_mark;

	Iterator iter (*this);
	iter.Set (node);
	for (iter ++; iter; iter ++) {
		dgTreeNode* node = iter.GetNode();
		if (node->GetInfo().m_mark != mark) {
			node->GetInfo().m_mark = mark;
			node->GetInfo().m_twin->m_mark = mark;
			return node; 
		}
	}
	return NULL; 
}

void dgMeshEffect::GetEdgeIndex (const void* const edge, int32_t& v0, int32_t& v1) const
{
	dgTreeNode* node = (dgTreeNode*) edge;
	v0 = node->GetInfo().m_incidentVertex;
	v1 = node->GetInfo().m_twin->m_incidentVertex;
}

//void dgMeshEffect::GetEdgeAttributeIndex (const void* edge, int32_t& v0, int32_t& v1) const
//{
//	dgTreeNode* node = (dgTreeNode*) edge;
//	v0 = int (node->GetInfo().m_userData);
//	v1 = int (node->GetInfo().m_twin->m_userData);
//}


void* dgMeshEffect::GetFirstFace ()
{
	Iterator iter (*this);
	iter.Begin();

	dgTreeNode* node = NULL;
	if (iter) {
		int32_t mark = IncLRU();
		node = iter.GetNode();

		dgEdge* const edge = &node->GetInfo();
		dgEdge* ptr = edge;
		do {
			ptr->m_mark = mark;
			ptr = ptr->m_next;
		} while (ptr != edge);
	}

	return node;
}

void* dgMeshEffect::GetNextFace (const void* const face)
{
	dgTreeNode* node = (dgTreeNode*) face;
	int32_t mark = node->GetInfo().m_mark;

	Iterator iter (*this);
	iter.Set (node);
	for (iter ++; iter; iter ++) {
		dgTreeNode* node = iter.GetNode();
		if (node->GetInfo().m_mark != mark) {
			dgEdge* const edge = &node->GetInfo();
			dgEdge* ptr = edge;
			do {
				ptr->m_mark = mark;
				ptr = ptr->m_next;
			} while (ptr != edge);
			return node; 
		}
	}
	return NULL; 
}


int32_t dgMeshEffect::IsFaceOpen (const void* const face) const
{
	dgTreeNode* node = (dgTreeNode*) face;
	dgEdge* const edge = &node->GetInfo();
	return (edge->m_incidentFace > 0) ? 0 : 1;
}

int32_t dgMeshEffect::GetFaceMaterial (const void* const face) const
{
	dgTreeNode* const node = (dgTreeNode*) face;
	dgEdge* const edge = &node->GetInfo();
	return int32_t (m_attib[edge->m_userData].m_material);
}

int32_t dgMeshEffect::GetFaceIndexCount (const void* const face) const
{
	int count = 0;
	dgTreeNode* node = (dgTreeNode*) face;
	dgEdge* const edge = &node->GetInfo();
	dgEdge* ptr = edge;
	do {
		count ++;
		ptr = ptr->m_next;
	} while (ptr != edge);
	return count; 
}

void dgMeshEffect::GetFaceIndex (const void* const face, int* const indices) const
{
	int count = 0;
	dgTreeNode* node = (dgTreeNode*) face;
	dgEdge* const edge = &node->GetInfo();
	dgEdge* ptr = edge;
	do {
		indices[count] =  ptr->m_incidentVertex;
		count ++;
		ptr = ptr->m_next;
	} while (ptr != edge);
}

void dgMeshEffect::GetFaceAttributeIndex (const void* const face, int* const indices) const
{
	int count = 0;
	dgTreeNode* node = (dgTreeNode*) face;
	dgEdge* const edge = &node->GetInfo();
	dgEdge* ptr = edge;
	do {
		indices[count] = int (ptr->m_userData);
		count ++;
		ptr = ptr->m_next;
	} while (ptr != edge);
}




/*
int32_t GetTotalFaceCount() const;
{
	int32_t mark;
	int32_t count;
	int32_t materialCount;
	int32_t materials[256];
	int32_t streamIndexMap[256];
	dgIndexArray* array; 

	count = 0;
	materialCount = 0;

	array = (dgIndexArray*) HACD_ALLOC (4 * sizeof (int32_t) * GetCount() + sizeof (dgIndexArray) + 2048);
	array->m_indexList = (int32_t*)&array[1];

	mark = IncLRU();
	dgPolyhedra::Iterator iter (*this);	
	memset(streamIndexMap, 0, sizeof (streamIndexMap));
	for(iter.Begin(); iter; iter ++){

		dgEdge* const edge;
		edge = &(*iter);
		if ((edge->m_incidentFace >= 0) && (edge->m_mark != mark)) {
			dgEdge* ptr;
			int32_t hashValue;
			int32_t index0;
			int32_t index1;

			ptr = edge;
			ptr->m_mark = mark;
			index0 = int32_t (ptr->m_userData);

			ptr = ptr->m_next;
			ptr->m_mark = mark;
			index1 = int32_t (ptr->m_userData);

			ptr = ptr->m_next;
			do {
				ptr->m_mark = mark;

				array->m_indexList[count * 4 + 0] = index0;
				array->m_indexList[count * 4 + 1] = index1;
				array->m_indexList[count * 4 + 2] = int32_t (ptr->m_userData);
				array->m_indexList[count * 4 + 3] = m_attib[int32_t (edge->m_userData)].m_material;
				index1 = int32_t (ptr->m_userData);

				hashValue = array->m_indexList[count * 4 + 3] & 0xff;
				streamIndexMap[hashValue] ++;
				materials[hashValue] = array->m_indexList[count * 4 + 3];
				count ++;

				ptr = ptr->m_next;
			} while (ptr != edge);
		}
	}
*/




void dgMeshEffect::GetVertexStreams (int32_t vetexStrideInByte, float* const vertex, 
									 int32_t normalStrideInByte, float* const normal, 
									 int32_t uvStrideInByte0, float* const uv0, 
									 int32_t uvStrideInByte1, float* const uv1)
{
	uvStrideInByte0 /= sizeof (float);
	uvStrideInByte1 /= sizeof (float);
	vetexStrideInByte /= sizeof (float);
	normalStrideInByte /= sizeof (float);
	for (int32_t i = 0; i < m_atribCount; i ++)	{
		int32_t j = i * vetexStrideInByte;
		vertex[j + 0] = float (m_attib[i].m_vertex.m_x);
		vertex[j + 1] = float (m_attib[i].m_vertex.m_y);
		vertex[j + 2] = float (m_attib[i].m_vertex.m_z);

		j = i * normalStrideInByte;
		normal[j + 0] = float (m_attib[i].m_normal_x);
		normal[j + 1] = float (m_attib[i].m_normal_y);
		normal[j + 2] = float (m_attib[i].m_normal_z);

		j = i * uvStrideInByte1;
		uv1[j + 0] = float (m_attib[i].m_u1);
		uv1[j + 1] = float (m_attib[i].m_v1);

		j = i * uvStrideInByte0;
		uv0[j + 0] = float (m_attib[i].m_u0);
		uv0[j + 1] = float (m_attib[i].m_v0);
	}
}


void dgMeshEffect::GetIndirectVertexStreams(
	int32_t vetexStrideInByte, 
	double* const vertex, 
	int32_t* const vertexIndices, 
	int32_t* const vertexCount,
	int32_t normalStrideInByte, 
	double* const normal, 
	int32_t* const normalIndices, 
	int32_t* const normalCount,
	int32_t uvStrideInByte0, 
	double* const uv0, 
	int32_t* const uvIndices0, 
	int32_t* const uvCount0,
	int32_t uvStrideInByte1, 
	double* const uv1, 
	int32_t* const uvIndices1, 
	int32_t* const uvCount1)
{
	HACD_UNUSED(vetexStrideInByte); 
		HACD_UNUSED(vertex);
		HACD_UNUSED(vertexIndices);
		HACD_UNUSED(vertexCount);
		HACD_UNUSED(normalStrideInByte);
		HACD_UNUSED(normal);
		HACD_UNUSED(normalIndices);
		HACD_UNUSED(normalCount);
		HACD_UNUSED(uvStrideInByte0);
		HACD_UNUSED(uv0);
		HACD_UNUSED(uvIndices0);
		HACD_UNUSED(uvCount0);
		HACD_UNUSED(uvStrideInByte1); 
		HACD_UNUSED(uv1);
		HACD_UNUSED(uvIndices1);
		HACD_UNUSED(uvCount1);
/*
	GetVertexStreams (vetexStrideInByte, vertex, normalStrideInByte, normal, uvStrideInByte0, uv0, uvStrideInByte1, uv1);

	*vertexCount = dgVertexListToIndexList(vertex, vetexStrideInByte, vetexStrideInByte, 0, m_atribCount, vertexIndices, float (0.0f));
	*normalCount = dgVertexListToIndexList(normal, normalStrideInByte, normalStrideInByte, 0, m_atribCount, normalIndices, float (0.0f));

	dgTriplex* const tmpUV = (dgTriplex*) HACD_ALLOC (int32_t (sizeof (dgTriplex) * m_atribCount));
	int32_t stride = int32_t (uvStrideInByte1 /sizeof (float));
	for (int32_t i = 0; i < m_atribCount; i ++){
		tmpUV[i].m_x = uv1[i * stride + 0];
		tmpUV[i].m_y = uv1[i * stride + 1];
		tmpUV[i].m_z = float (0.0f);
	}

	int32_t count = dgVertexListToIndexList(&tmpUV[0].m_x, sizeof (dgTriplex), sizeof (dgTriplex), 0, m_atribCount, uvIndices1, float (0.0f));
	for (int32_t i = 0; i < count; i ++){
		uv1[i * stride + 0] = tmpUV[i].m_x;
		uv1[i * stride + 1] = tmpUV[i].m_y;
	}
	*uvCount1 = count;

	stride = int32_t (uvStrideInByte0 /sizeof (float));
	for (int32_t i = 0; i < m_atribCount; i ++){
		tmpUV[i].m_x = uv0[i * stride + 0];
		tmpUV[i].m_y = uv0[i * stride + 1];
		tmpUV[i].m_z = float (0.0f);
	}
	count = dgVertexListToIndexList(&tmpUV[0].m_x, sizeof (dgTriplex), sizeof (dgTriplex), 0, m_atribCount, uvIndices0, float (0.0f));
	for (int32_t i = 0; i < count; i ++){
		uv0[i * stride + 0] = tmpUV[i].m_x;
		uv0[i * stride + 1] = tmpUV[i].m_y;
	}
	*uvCount0 = count;

	HACD_FREE (tmpUV);
*/
}

dgMeshEffect::dgIndexArray* dgMeshEffect::MaterialGeometryBegin()
{
	int32_t materials[256];
	int32_t streamIndexMap[256];

	int32_t count = 0;
	int32_t materialCount = 0;
	
	dgIndexArray* const array = (dgIndexArray*) HACD_ALLOC (size_t (4 * sizeof (int32_t) * GetCount() + sizeof (dgIndexArray) + 2048));
	array->m_indexList = (int32_t*)&array[1];
	
	int32_t mark = IncLRU();
	dgPolyhedra::Iterator iter (*this);	
	memset(streamIndexMap, 0, sizeof (streamIndexMap));
	for(iter.Begin(); iter; iter ++){
		dgEdge* const edge = &(*iter);
		if ((edge->m_incidentFace >= 0) && (edge->m_mark != mark)) {
			dgEdge* ptr = edge;
			ptr->m_mark = mark;
			int32_t index0 = int32_t (ptr->m_userData);

			ptr = ptr->m_next;
			ptr->m_mark = mark;
			int32_t index1 = int32_t (ptr->m_userData);

			ptr = ptr->m_next;
			do {
				ptr->m_mark = mark;

				array->m_indexList[count * 4 + 0] = index0;
				array->m_indexList[count * 4 + 1] = index1;
				array->m_indexList[count * 4 + 2] = int32_t (ptr->m_userData);
				array->m_indexList[count * 4 + 3] = int32_t (m_attib[int32_t (edge->m_userData)].m_material);
				index1 = int32_t (ptr->m_userData);

				int32_t hashValue = array->m_indexList[count * 4 + 3] & 0xff;
				streamIndexMap[hashValue] ++;
				materials[hashValue] = array->m_indexList[count * 4 + 3];
				count ++;

				ptr = ptr->m_next;
			} while (ptr != edge);
		}
	}

	array->m_indexCount = count;
	array->m_materialCount = materialCount;

	count = 0;
	for (int32_t i = 0; i < 256;i ++) {
		if (streamIndexMap[i]) {
			array->m_materials[count] = materials[i];
			array->m_materialsIndexCount[count] = streamIndexMap[i] * 3;
			count ++;
		}
	}

	array->m_materialCount = count;

	return array;
}

void dgMeshEffect::MaterialGeomteryEnd(dgIndexArray* const handle)
{
	HACD_FREE (handle);
}


int32_t dgMeshEffect::GetFirstMaterial (dgIndexArray* const handle)
{
	return GetNextMaterial (handle, -1);
}

int32_t dgMeshEffect::GetNextMaterial (dgIndexArray* const handle, int32_t materialId)
{
	materialId ++;
	if(materialId >= handle->m_materialCount) {
		materialId = -1;
	}
	return materialId;
}

void dgMeshEffect::GetMaterialGetIndexStream (dgIndexArray* const handle, int32_t materialHandle, int32_t* const indexArray)
{
	int32_t index;
	int32_t textureID;

	index = 0;
	textureID = handle->m_materials[materialHandle];
	for (int32_t j = 0; j < handle->m_indexCount; j ++) {
		if (handle->m_indexList[j * 4 + 3] == textureID) {
			indexArray[index + 0] = handle->m_indexList[j * 4 + 0];
			indexArray[index + 1] = handle->m_indexList[j * 4 + 1];
			indexArray[index + 2] = handle->m_indexList[j * 4 + 2];

			index += 3;
		}
	}
}

void dgMeshEffect::GetMaterialGetIndexStreamShort (dgIndexArray* const handle, int32_t materialHandle, int16_t* const indexArray)
{
	int32_t index;
	int32_t textureID;

	index = 0;
	textureID = handle->m_materials[materialHandle];
	for (int32_t j = 0; j < handle->m_indexCount; j ++) {
		if (handle->m_indexList[j * 4 + 3] == textureID) {
			indexArray[index + 0] = (int16_t)handle->m_indexList[j * 4 + 0];
			indexArray[index + 1] = (int16_t)handle->m_indexList[j * 4 + 1];
			indexArray[index + 2] = (int16_t)handle->m_indexList[j * 4 + 2];
			index += 3;
		}
	}
}

/*
int32_t dgMeshEffect::GetEffectiveVertexCount() const
{
	int32_t mark;
	int32_t count;

	count = 0;
	mark = IncLRU();
	dgPolyhedra::Iterator iter (*this);
	for (iter.Begin(); iter; iter ++){
		dgEdge* vertex;
	
		vertex = &(*iter);
		if (vertex->m_mark != mark) {
			dgEdge* ptr;

			ptr = vertex;
			do {
				ptr->m_mark = mark;
				ptr = ptr->m_twin->m_next;
			} while (ptr != vertex);
			count ++;
		}
	}
	return count;
}
*/

dgConvexHull3d * dgMeshEffect::CreateConvexHull(double tolerance,int32_t maxVertexCount) const
{
	dgConvexHull3d *ret = NULL;

	dgStack<dgBigVector> poolPtr(m_pointCount * 2); 
	dgBigVector* const pool = &poolPtr[0];

	int32_t count = 0;
	int32_t mark = IncLRU();
	dgPolyhedra::Iterator iter (*this);
	for (iter.Begin(); iter; iter ++)
	{
		dgEdge* const vertex = &(*iter);
		if (vertex->m_mark != mark) 
		{
			dgEdge* ptr = vertex;
			do {
				ptr->m_mark = mark;
				ptr = ptr->m_twin->m_next;
			} while (ptr != vertex);

			if (count < int32_t (poolPtr.GetElementsCount())) 
			{
				const dgBigVector p = m_points[vertex->m_incidentVertex];
				pool[count] = p;
				count++;
			}
		}
	}

	ret = HACD_NEW(dgConvexHull3d)((const double *)pool,sizeof(dgBigVector),count,tolerance,maxVertexCount);

	return ret;
}


void dgMeshEffect::TransformMesh (const dgMatrix& matrix)
{
	dgMatrix normalMatrix (matrix);
	normalMatrix.m_posit = dgVector (float (0.0f), float (0.0f), float (0.0f), float (1.0f));

	matrix.TransformTriplex (&m_points->m_x, sizeof (dgBigVector), &m_points->m_x, sizeof (dgBigVector), m_pointCount);
	matrix.TransformTriplex (&m_attib[0].m_vertex.m_x, sizeof (dgVertexAtribute), &m_attib[0].m_vertex.m_x, sizeof (dgVertexAtribute), m_atribCount);
	normalMatrix.TransformTriplex (&m_attib[0].m_normal_x, sizeof (dgVertexAtribute), &m_attib[0].m_normal_x, sizeof (dgVertexAtribute), m_atribCount);
}


dgMeshEffect::dgVertexAtribute dgMeshEffect::InterpolateEdge (dgEdge* const edge, double param) const
{
	dgVertexAtribute attrEdge;
	double t1 = param;
	double t0 = double (1.0f) - t1;
	HACD_ASSERT (t1 >= double(0.0f));
	HACD_ASSERT (t1 <= double(1.0f));

	const dgVertexAtribute& attrEdge0 = m_attib[edge->m_userData];
	const dgVertexAtribute& attrEdge1 = m_attib[edge->m_next->m_userData];

	attrEdge.m_vertex.m_x = attrEdge0.m_vertex.m_x * t0 + attrEdge1.m_vertex.m_x * t1;
	attrEdge.m_vertex.m_y = attrEdge0.m_vertex.m_y * t0 + attrEdge1.m_vertex.m_y * t1;
	attrEdge.m_vertex.m_z = attrEdge0.m_vertex.m_z * t0 + attrEdge1.m_vertex.m_z * t1;
	attrEdge.m_vertex.m_w = float(0.0f);
	attrEdge.m_normal_x = attrEdge0.m_normal_x * t0 +  attrEdge1.m_normal_x * t1; 
	attrEdge.m_normal_y = attrEdge0.m_normal_y * t0 +  attrEdge1.m_normal_y * t1; 
	attrEdge.m_normal_z = attrEdge0.m_normal_z * t0 +  attrEdge1.m_normal_z * t1; 
	attrEdge.m_u0 = attrEdge0.m_u0 * t0 +  attrEdge1.m_u0 * t1;
	attrEdge.m_v0 = attrEdge0.m_v0 * t0 +  attrEdge1.m_v0 * t1;
	attrEdge.m_u1 = attrEdge0.m_u1 * t0 +  attrEdge1.m_u1 * t1;
	attrEdge.m_v1 = attrEdge0.m_v1 * t0 +  attrEdge1.m_v1 * t1;
	attrEdge.m_material = attrEdge0.m_material;
	return attrEdge;
}

bool dgMeshEffect::Sanity () const
{
	HACD_ASSERT (0);
return false;
/*
	Iterator iter (*this);
	for (iter.Begin(); iter; iter ++) {
		const dgEdge* const edge = &iter.GetNode()->GetInfo();
		if (edge->m_incidentFace > 0) {
			const dgVertexAtribute& attrEdge0 = m_attib[edge->m_userData];
			dgVector p0 (m_points[edge->m_incidentVertex]);
			dgVector q0 (attrEdge0.m_vertex);
			dgVector delta0 (p0 - q0);
			float error0 = delta0 % delta0;
			if (error0 > float (1.0e-15f)) {
				return false;
			}

			const dgVertexAtribute& attrEdge1 = m_attib[edge->m_next->m_userData];
			dgVector p1 (m_points[edge->m_next->m_incidentVertex]);
			dgVector q1 (attrEdge1.m_vertex);
			dgVector delta1 (p1 - q1);
			float error1 = delta1 % delta1;
			if (error1 > float (1.0e-15f)) {
				return false;
			}
		}
	}
	return true;
*/
}


dgEdge* dgMeshEffect::InsertEdgeVertex (dgEdge* const edge, double param)
{

	dgEdge* const twin = edge->m_twin;
	dgVertexAtribute attrEdge (InterpolateEdge (edge, param));
	dgVertexAtribute attrTwin (InterpolateEdge (twin, float (1.0f) - param));

	attrTwin.m_vertex = attrEdge.m_vertex;
	AddPoint(&attrEdge.m_vertex.m_x, dgFastInt (attrEdge.m_material));
	AddAtribute (attrTwin);

	int32_t edgeAttrV0 = int32_t (edge->m_userData);
	int32_t twinAttrV0 = int32_t (twin->m_userData);

	dgEdge* const faceA0 = edge->m_next;
	dgEdge* const faceA1 = edge->m_prev;
	dgEdge* const faceB0 = twin->m_next;
	dgEdge* const faceB1 = twin->m_prev;

//	SpliteEdgeAndTriangulate (m_pointCount - 1, edge);
	SpliteEdge (m_pointCount - 1, edge);

	faceA0->m_prev->m_userData = uint64_t (m_atribCount - 2);
	faceA1->m_next->m_userData = uint64_t (edgeAttrV0);

	faceB0->m_prev->m_userData = uint64_t (m_atribCount - 1);
	faceB1->m_next->m_userData = uint64_t (twinAttrV0);

	return faceA1->m_next;
}



dgMeshEffect::dgVertexAtribute dgMeshEffect::InterpolateVertex (const dgBigVector& srcPoint, dgEdge* const face) const
{
	//this should use Googol extended precision floats, because some face coming from Voronoi decomposition and booleans
	//clipping has extreme aspect ratios, for now just use float64
	const dgBigVector point (srcPoint);

	dgVertexAtribute attribute;
	memset (&attribute, 0, sizeof (attribute));
	double tol = float (1.0e-4f);
	for (int32_t i = 0; i < 4; i ++) {
		dgEdge* ptr = face;
		dgEdge* const edge0 = ptr;
		dgBigVector q0 (m_points[ptr->m_incidentVertex]);

		ptr = ptr->m_next;
		const dgEdge* edge1 = ptr;
		dgBigVector q1 (m_points[ptr->m_incidentVertex]);

		ptr = ptr->m_next;
		const dgEdge* edge2 = ptr;
		do {
			const dgBigVector q2 (m_points[ptr->m_incidentVertex]);

			dgBigVector p10 (q1 - q0);
			dgBigVector p20 (q2 - q0);
			dgBigVector p_p0 (point - q0);
			dgBigVector p_p1 (point - q1);
			dgBigVector p_p2 (point - q2);

			double alpha1 = p10 % p_p0;
			double alpha2 = p20 % p_p0;
			double alpha3 = p10 % p_p1;
			double alpha4 = p20 % p_p1;
			double alpha5 = p10 % p_p2;
			double alpha6 = p20 % p_p2;

			double vc = alpha1 * alpha4 - alpha3 * alpha2;
			double vb = alpha5 * alpha2 - alpha1 * alpha6;
			double va = alpha3 * alpha6 - alpha5 * alpha4;
			double den = va + vb + vc;
			double minError = den * (-tol);
			double maxError = den * (float (1.0f) + tol);
			if ((va > minError) && (vb > minError) && (vc > minError) && (va < maxError) && (vb < maxError) && (vc < maxError)) {
				edge2 = ptr;

				den = double (1.0f) / (va + vb + vc);

				double alpha0 = float (va * den);
				double alpha1 = float (vb * den);
				double alpha2 = float (vc * den);

				const dgVertexAtribute& attr0 = m_attib[edge0->m_userData];
				const dgVertexAtribute& attr1 = m_attib[edge1->m_userData];
				const dgVertexAtribute& attr2 = m_attib[edge2->m_userData];
				dgBigVector normal (attr0.m_normal_x * alpha0 + attr1.m_normal_x * alpha1 + attr2.m_normal_x * alpha2,
									attr0.m_normal_y * alpha0 + attr1.m_normal_y * alpha1 + attr2.m_normal_y * alpha2,
									attr0.m_normal_z * alpha0 + attr1.m_normal_z * alpha1 + attr2.m_normal_z * alpha2, float (0.0f));
				normal = normal.Scale (double (1.0f) / sqrt (normal % normal));

	#ifdef _DEBUG
				dgBigVector testPoint (attr0.m_vertex.m_x * alpha0 + attr1.m_vertex.m_x * alpha1 + attr2.m_vertex.m_x * alpha2,
									   attr0.m_vertex.m_y * alpha0 + attr1.m_vertex.m_y * alpha1 + attr2.m_vertex.m_y * alpha2,
									   attr0.m_vertex.m_z * alpha0 + attr1.m_vertex.m_z * alpha1 + attr2.m_vertex.m_z * alpha2, float (0.0f));
				HACD_ASSERT (fabs (testPoint.m_x - point.m_x) < float (1.0e-2f));
				HACD_ASSERT (fabs (testPoint.m_y - point.m_y) < float (1.0e-2f));
				HACD_ASSERT (fabs (testPoint.m_z - point.m_z) < float (1.0e-2f));
	#endif


				attribute.m_vertex.m_x = point.m_x;
				attribute.m_vertex.m_y = point.m_y;
				attribute.m_vertex.m_z = point.m_z;
				attribute.m_vertex.m_w = point.m_w;
				attribute.m_normal_x = normal.m_x;
				attribute.m_normal_y = normal.m_y;
				attribute.m_normal_z = normal.m_z;
				attribute.m_u0 = attr0.m_u0 * alpha0 +  attr1.m_u0 * alpha1 + attr2.m_u0 * alpha2;
				attribute.m_v0 = attr0.m_v0 * alpha0 +  attr1.m_v0 * alpha1 + attr2.m_v0 * alpha2;
				attribute.m_u1 = attr0.m_u1 * alpha0 +  attr1.m_u1 * alpha1 + attr2.m_u1 * alpha2;
				attribute.m_v1 = attr0.m_v1 * alpha0 +  attr1.m_v1 * alpha1 + attr2.m_v1 * alpha2;

				attribute.m_material = attr0.m_material;
				HACD_ASSERT (attr0.m_material == attr1.m_material);
				HACD_ASSERT (attr0.m_material == attr2.m_material);
				return attribute; 
			}
			
			q1 = q2;
			edge1 = ptr;

			ptr = ptr->m_next;
		} while (ptr != face);
		tol *= double (2.0f);
	}
	// this should never happens
	HACD_ASSERT (0);
	return attribute;
}




void dgMeshEffect::MergeFaces (const dgMeshEffect* const source)
{
	int32_t mark = source->IncLRU();
	dgPolyhedra::Iterator iter (*source);
	for(iter.Begin(); iter; iter ++){
		dgEdge* const edge = &(*iter);
		if ((edge->m_incidentFace > 0) && (edge->m_mark < mark)) {
			dgVertexAtribute face[DG_MESH_EFFECT_POINT_SPLITED];

			int32_t count = 0;
			dgEdge* ptr = edge;
			do {
				ptr->m_mark = mark;
				face[count] = source->m_attib[ptr->m_userData];
				count ++;
				HACD_ASSERT (count < int32_t (sizeof (face) / sizeof (face[0])));
				ptr = ptr->m_next;
			} while (ptr != edge);
			AddPolygon(count, &face[0].m_vertex.m_x, sizeof (dgVertexAtribute), dgFastInt (face[0].m_material));
		}
	}
}


void dgMeshEffect::ReverseMergeFaces (dgMeshEffect* const source)
{
	int32_t mark = source->IncLRU();
	dgPolyhedra::Iterator iter (*source);
	for(iter.Begin(); iter; iter ++){
		dgEdge* const edge = &(*iter);
		if ((edge->m_incidentFace > 0) && (edge->m_mark < mark)) {
			dgVertexAtribute face[DG_MESH_EFFECT_POINT_SPLITED];

			int32_t count = 0;
			dgEdge* ptr = edge;
			do {
				ptr->m_mark = mark;
				face[count] = source->m_attib[ptr->m_userData];
				face[count].m_normal_x *= float (-1.0f);
				face[count].m_normal_y *= float (-1.0f);
				face[count].m_normal_z *= float (-1.0f);
				count ++;
				HACD_ASSERT (count < int32_t (sizeof (face) / sizeof (face[0])));
				ptr = ptr->m_prev;
			} while (ptr != edge);
			AddPolygon(count, &face[0].m_vertex.m_x, sizeof (dgVertexAtribute), dgFastInt (face[0].m_material));
		}
	}
}



void dgMeshEffect::FilterCoplanarFaces (const dgMeshEffect* const coplanarFaces, float sign)
{
	const double tol = double (1.0e-5f);
	const double tol2 = tol * tol;

	int32_t mark = IncLRU();
	Iterator iter (*this);
	for (iter.Begin(); iter; ) {
		dgEdge* const face = &(*iter);
		iter ++;
		if ((face->m_mark != mark) && (face->m_incidentFace > 0)) {
			dgEdge* ptr = face;
			do {
				ptr->m_mark = mark;
				ptr = ptr->m_next;
			} while (ptr != face);

			dgBigVector normal (FaceNormal(face, &m_points[0].m_x, sizeof (dgBigVector)));
			normal = normal.Scale (sign);
			dgBigVector origin (m_points[face->m_incidentVertex]);

			double error2 = (normal % normal) * tol2;
			int32_t capMark = coplanarFaces->IncLRU();

			Iterator capIter (*coplanarFaces);
			for (capIter.Begin (); capIter; capIter ++) {
				dgEdge* const capFace = &(*capIter);
				if ((capFace->m_mark != capMark) && (capFace->m_incidentFace > 0)) {
					dgEdge* ptr = capFace;
					do {
						ptr->m_mark = capMark;
						ptr = ptr->m_next;
					} while (ptr != capFace);

					dgBigVector capNormal (coplanarFaces->FaceNormal(capFace, &coplanarFaces->m_points[0].m_x, sizeof (dgBigVector)));

					if ((capNormal % normal) > double (0.0f)) {
						dgBigVector capOrigin (coplanarFaces->m_points[capFace->m_incidentVertex]);
						double dist = normal % (capOrigin - origin);
						if ((dist * dist) < error2) {
							DeleteFace(face);
							iter.Begin();
							break;
						}
					}
				}
			}
		}
	}
}


bool dgMeshEffect::HasOpenEdges () const
{
	dgPolyhedra::Iterator iter (*this);
	for (iter.Begin(); iter; iter ++){
		dgEdge* const face = &(*iter);
		if (face->m_incidentFace < 0){
			return true;
		}
	}
	return false;
}



bool dgMeshEffect::SeparateDuplicateLoops (dgEdge* const face)
{
	for (dgEdge* ptr0 = face; ptr0 != face->m_prev; ptr0 = ptr0->m_next) {
		int32_t index = ptr0->m_incidentVertex;

		dgEdge* ptr1 = ptr0->m_next; 
		do {
			if (ptr1->m_incidentVertex == index) {
				dgEdge* const ptr00 = ptr0->m_prev;
				dgEdge* const ptr11 = ptr1->m_prev;

				ptr00->m_next = ptr1;
				ptr1->m_prev = ptr00;

				ptr11->m_next = ptr0;
				ptr0->m_prev = ptr11;

				return true;
			}

			ptr1 = ptr1->m_next;
		} while (ptr1 != face);
	}

	return false;
}


dgMeshEffect* dgMeshEffect::GetNextLayer (int32_t mark)
{
	Iterator iter(*this);
	dgEdge* edge = NULL;
	for (iter.Begin (); iter; iter ++) {
		edge = &(*iter);
		if ((edge->m_mark < mark) && (edge->m_incidentFace > 0)) {
			break;
		}
	}

	if (!edge) {
		return NULL;
	}

	int32_t layer = int32_t (m_points[edge->m_incidentVertex].m_w);
	dgPolyhedra polyhedra;

	polyhedra.BeginFace ();
	for (iter.Begin (); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if ((edge->m_mark < mark) && (edge->m_incidentFace > 0)) {
			int32_t thislayer = int32_t (m_points[edge->m_incidentVertex].m_w);
			if (thislayer == layer) {
				dgEdge* ptr = edge;
				uint32_t count = 0;
				int32_t faceIndex[256];
				int64_t faceDataIndex[256];
				do {
					ptr->m_mark = mark;
					faceIndex[count] = ptr->m_incidentVertex;
					faceDataIndex[count] = (int64_t)ptr->m_userData;
					count ++;
					HACD_ASSERT (count < int32_t (sizeof (faceIndex)/ sizeof(faceIndex[0])));
					ptr = ptr->m_next;
				} while (ptr != edge);
				polyhedra.AddFace ((int32_t)count, &faceIndex[0], &faceDataIndex[0]);
			}
		}
	}
	polyhedra.EndFace ();

	dgMeshEffect* solid = NULL;
	if (polyhedra.GetCount()) {
		solid = HACD_NEW(dgMeshEffect)(polyhedra, *this);
		solid->SetLRU(mark);
	}
	return solid;
}


void dgMeshEffect::RepairTJoints (bool triangulate)
{
	int32_t mark = IncLRU();
	dgPolyhedra::Iterator iter (*this);
#ifdef _DEBUG
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const face = &(*iter);
		if ((face->m_incidentFace < 0) && (face->m_mark != mark)) {
			for (dgEdge* ptr = face; ptr != face->m_prev; ptr = ptr->m_next) {
				dgBigVector p0 (m_points[ptr->m_incidentVertex]);
				for (dgEdge* ptr1 = ptr->m_next; ptr1 != face; ptr1 = ptr1->m_next) {
					if (ptr->m_incidentVertex != ptr1->m_incidentVertex) {
						dgBigVector p1 (m_points[ptr1->m_incidentVertex]);
						dgBigVector dp (p1 - p0);
						double err2 (dp % dp);
						if (err2 < double (1.0e-16f)) {
//							HACD_ASSERT (0);
						}
					}
				} 
			}
		}
	}
	mark = IncLRU();
#endif



	for (iter.Begin(); iter; ) {
		dgEdge* const face = &(*iter);
		iter ++;


		if ((face->m_incidentFace < 0) && (face->m_mark != mark)) {
			// vertices project 

			while (SeparateDuplicateLoops (face));

			dgBigVector dir (double (0.0f), double (0.0f), double (0.0f), double (0.0f));
			double lengh2 = double (0.0f);
			dgEdge* ptr = face;
			do {
				dgBigVector dir1 (m_points[ptr->m_next->m_incidentVertex] - m_points[ptr->m_incidentVertex]);
				double val = dir1 % dir1;
				if (val > lengh2) {
					lengh2 = val;
					dir = dir1;
				}
				ptr = ptr->m_next;
			} while (ptr != face);

			HACD_ASSERT (lengh2 > float (0.0f));

			dgEdge* lastEdge = NULL;
			dgEdge* firstEdge = NULL;
			double minVal = double (-1.0e10f);
			double maxVal = double (-1.0e10f);
			ptr = face;
			do {
				const dgBigVector& p = m_points[ptr->m_incidentVertex];
				double val = p % dir;
				if (val > maxVal) {
					maxVal = val;
					lastEdge = ptr;
				}
				val *= double (-1.0f);
				if (val > minVal) {
					minVal = val;
					firstEdge = ptr;
				}

				ptr->m_mark = mark;
				ptr = ptr->m_next;
			} while (ptr != face);

			HACD_ASSERT (firstEdge);
			HACD_ASSERT (lastEdge);

			bool isTJoint = true;
			dgBigVector p0 (m_points[firstEdge->m_incidentVertex]);
			dgBigVector p1 (m_points[lastEdge->m_incidentVertex]);
			dgBigVector p1p0 (p1 - p0);
			double den = p1p0 % p1p0;
			ptr = firstEdge->m_next;
			do {
				dgBigVector p2 (m_points[ptr->m_incidentVertex]);
				double num = (p2 - p0) % p1p0;
				dgBigVector q (p0 + p1p0.Scale (num / den));
				dgBigVector dist (p2 - q);
				double err2 = dist % dist;
				isTJoint &= (err2 < (double (1.0e-4f) * double (1.0e-4f)));
				ptr = ptr->m_next;
			} while (isTJoint && (ptr != firstEdge));

			if (isTJoint) {
				do {
					dgEdge* next = NULL;

					const dgBigVector p0 = m_points[firstEdge->m_incidentVertex];
					const dgBigVector p1 = m_points[firstEdge->m_next->m_incidentVertex];
					const dgBigVector p2 = m_points[firstEdge->m_prev->m_incidentVertex];

					dgBigVector p1p0 (p1 - p0);
					dgBigVector p2p0 (p2 - p0);
					double dist10 = p1p0 % p1p0;
					double dist20 = p2p0 % p2p0;

					dgEdge* begin = NULL;
					dgEdge* last = NULL;
					if (dist20 > dist10) {
						double t = (p1p0 % p2p0) / dist20;
						HACD_ASSERT (t > float (0.0f));
						HACD_ASSERT (t < float (1.0f));

						if (firstEdge->m_next->m_next->m_next != firstEdge) {
							ConectVertex (firstEdge->m_prev, firstEdge->m_next);
							next = firstEdge->m_next->m_twin->m_next;
						}
						HACD_ASSERT (firstEdge->m_next->m_next->m_next == firstEdge);

#ifdef _DEBUG
						dgEdge* tmp = firstEdge->m_twin;
						do {
							HACD_ASSERT (tmp->m_incidentFace > 0);
							tmp = tmp->m_next;
						} while (tmp != firstEdge->m_twin); 
#endif

						begin = firstEdge->m_next;
						last = firstEdge;
						firstEdge->m_userData = firstEdge->m_prev->m_twin->m_userData;
						firstEdge->m_incidentFace = firstEdge->m_prev->m_twin->m_incidentFace;
						dgVertexAtribute attrib (InterpolateEdge (firstEdge->m_prev->m_twin, t));
						attrib.m_vertex = m_points[firstEdge->m_next->m_incidentVertex];
						AddAtribute(attrib);
						firstEdge->m_next->m_incidentFace = firstEdge->m_prev->m_twin->m_incidentFace;
						firstEdge->m_next->m_userData = uint64_t (m_atribCount - 1);

						bool restart = false;
						if ((firstEdge->m_prev == &(*iter)) || (firstEdge->m_prev->m_twin == &(*iter))) {
							restart = true;
						}
						DeleteEdge(firstEdge->m_prev);
						if (restart) {
							iter.Begin();
						}

					} else {

						HACD_ASSERT (dist20 < dist10);

						double t = (p1p0 % p2p0) / dist10;
						HACD_ASSERT (t > float (0.0f));
						HACD_ASSERT (t < float (1.0f));

						if (firstEdge->m_next->m_next->m_next != firstEdge) {
							ConectVertex (firstEdge->m_next, firstEdge->m_prev);
							next = firstEdge->m_next->m_twin;
						}
						HACD_ASSERT (firstEdge->m_next->m_next->m_next == firstEdge);

#ifdef _DEBUG
						dgEdge* tmp = firstEdge->m_twin;
						do {
							HACD_ASSERT (tmp->m_incidentFace > 0);
							tmp = tmp->m_next;
						} while (tmp != firstEdge->m_twin); 
#endif

						begin = firstEdge->m_prev;
						last = firstEdge->m_next;
						firstEdge->m_next->m_userData = firstEdge->m_twin->m_userData;
						firstEdge->m_next->m_incidentFace = firstEdge->m_twin->m_incidentFace;
						dgVertexAtribute attrib (InterpolateEdge (firstEdge->m_twin, double (1.0f) - t));
						attrib.m_vertex = m_points[firstEdge->m_prev->m_incidentVertex];
						AddAtribute(attrib);
						firstEdge->m_prev->m_incidentFace = firstEdge->m_twin->m_incidentFace;
						firstEdge->m_prev->m_userData = uint64_t (m_atribCount - 1);

						bool restart = false;
						if ((firstEdge == &(*iter)) || (firstEdge->m_twin == &(*iter))) {
							restart = true;
						}
						DeleteEdge(firstEdge);
						if (restart) {
							iter.Begin();
						}
					}

					if (triangulate) {
						HACD_ASSERT (begin);
						HACD_ASSERT (last);
						for (dgEdge* ptr = begin->m_next->m_next; ptr != last; ptr = ptr->m_next) {
							dgEdge* const e = AddHalfEdge (begin->m_incidentVertex, ptr->m_incidentVertex);
							dgEdge* const t = AddHalfEdge (ptr->m_incidentVertex, begin->m_incidentVertex);
							if (e && t) {
								HACD_ASSERT (e);
								HACD_ASSERT (t);
								e->m_twin = t;
								t->m_twin = e;

								e->m_incidentFace = ptr->m_incidentFace;
								t->m_incidentFace = ptr->m_incidentFace;

								e->m_userData = last->m_next->m_userData;
								t->m_userData = ptr->m_userData;

								t->m_prev = ptr->m_prev;
								ptr->m_prev->m_next = t;
								e->m_next = ptr;
								ptr->m_prev = e;
								t->m_next = last->m_next;
								e->m_prev = last;
								last->m_next->m_prev = t;
								last->m_next = e;
							}
						}
					}

					firstEdge = next;
				} while (firstEdge);
			}
		}
	}

	DeleteDegenerateFaces(&m_points[0].m_x, sizeof (m_points[0]), double (1.0e-7f));
}

// create a convex hull
dgMeshEffect::dgMeshEffect (const double* const vertexCloud, int32_t count, int32_t strideInByte, double distTol)
:dgPolyhedra()
{
	Init(true);
	if (count >= 4) {
		dgConvexHull3d convexHull (vertexCloud, strideInByte, count, distTol);
		if (convexHull.GetCount()) {

			int32_t vertexCount = convexHull.GetVertexCount();
			dgStack<dgVector> pointsPool (convexHull.GetVertexCount());
			dgVector* const points = &pointsPool[0];
			for (int32_t i = 0; i < vertexCount; i ++) {
				points[i] = convexHull.GetVertex(i);
			}
			dgVector uv(float (0.0f), float (0.0f), float (0.0f), float (0.0f));
			dgVector normal (float (0.0f), float (1.0f), float (0.0f), float (0.0f));

			int32_t triangleCount = convexHull.GetCount();
			dgStack<int32_t> faceCountPool (triangleCount);
			dgStack<int32_t> materialsPool (triangleCount);
			dgStack<int32_t> vertexIndexListPool (triangleCount * 3);
			dgStack<int32_t> normalIndexListPool (triangleCount * 3);


			memset (&materialsPool[0], 0, triangleCount * sizeof (int32_t));
			memset (&normalIndexListPool[0], 0, 3 * triangleCount * sizeof (int32_t));

			int32_t index = 0;
			int32_t* const faceCount = &faceCountPool[0];
			int32_t* const vertexIndexList = &vertexIndexListPool[0];
			for (dgConvexHull3d::dgListNode* faceNode = convexHull.GetFirst(); faceNode; faceNode = faceNode->GetNext()) {
				dgConvexHull3DFace& face = faceNode->GetInfo();
				faceCount[index] = 3;
				vertexIndexList[index * 3 + 0] = face.m_index[0]; 
				vertexIndexList[index * 3 + 1] = face.m_index[1]; 
				vertexIndexList[index * 3 + 2] = face.m_index[2]; 
				index ++;
			}

			BuildFromVertexListIndexList(triangleCount, faceCount, &materialsPool[0], 
				&points[0].m_x, sizeof (dgVector), vertexIndexList,
				&normal.m_x, sizeof (dgVector), &normalIndexListPool[0],
				&uv.m_x, sizeof (dgVector), &normalIndexListPool[0],
				&uv.m_x, sizeof (dgVector), &normalIndexListPool[0]);
		}
	}
}

//**** Note, from this point out is a copy of 'MeshEffect3.cpp' from the Newton Physics Engine


dgMeshEffect* dgMeshEffect::CreateSimplification(int32_t maxVertexCount,hacd::ICallback* /*reportProgressCallback*/) const
{
	if (GetVertexCount() <= maxVertexCount) 
	{
		return HACD_NEW(dgMeshEffect)(*this); 
	}
	return HACD_NEW(dgMeshEffect)(*this); 
}

// based of the paper Hierarchical Approximate Convex Decomposition by Khaled Mamou 
// available http://sourceforge.net/projects/hacd/
// for the details http://kmamou.blogspot.com/
// with his permission to adapt his algorithm to be more efficient.
// also making some addition to his heuristic for better conve clusters seltections


#define DG_BUILD_HIERACHICAL_HACD

#define DG_CONCAVITY_SCALE double (100.0f)


class dgHACDEdge
{
	public:
	dgHACDEdge ()
		:m_mark(0)
		,m_proxyListNode(NULL)
		,m_backFaceHandicap(double (1.0))
	{
	}
	~dgHACDEdge ()
	{
	}

	int32_t m_mark;
	void* m_proxyListNode;
	double m_backFaceHandicap;
};

class dgHACDClusterFace
{
	public:
	dgHACDClusterFace()
		:m_edge(NULL)
		,m_area(double(0.0f))
	{
	}
	~dgHACDClusterFace()
	{
	}

	dgEdge* m_edge;
	double m_area;
	dgBigVector m_normal;
};


class dgHACDCluster: public dgList<dgHACDClusterFace>
{
	public:
	dgHACDCluster ()
		:m_color(0)
		,m_hierachicalClusterIndex(0)
		,m_area(double (0.0f))
		,m_concavity(double (0.0f))
	{
	}

	bool IsCoplanar(const dgBigPlane& plane, const dgMeshEffect& mesh, double tolerance) const
	{
		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();
		for (dgListNode* node = GetFirst(); node; node = node->GetNext()) {
			const dgHACDClusterFace& info = node->GetInfo();
			dgEdge* ptr = info.m_edge;
			do {
				const dgBigVector& p = points[ptr->m_incidentVertex];
				double dist = fabs(plane.Evalue(p));
				if (dist > tolerance) {
					return false;
				}
				ptr = ptr->m_next;
			} while (ptr != info.m_edge);
		}
		return true;
	}


	int32_t m_color;
	int32_t m_hierachicalClusterIndex;
	double m_area;
	double m_concavity;
};


class dgHACDClusterGraph :public dgGraph<dgHACDCluster, dgHACDEdge> ,public dgAABBPolygonSoup 
{
	public:

	class dgHACDConveHull: public dgConvexHull3d
	{
		class dgConvexHullRayCastData
		{
			public:
			double m_normalProjection;
			const dgConvexHull3DFace* m_face;
		};

		public: 
		dgHACDConveHull (const dgHACDConveHull& hull)
			:dgConvexHull3d(hull)
			,m_mark(1)
		{
		}

		dgHACDConveHull (const dgBigVector* const points, int32_t count)
			:dgConvexHull3d(&points[0].m_x, sizeof (dgBigVector),count, double (0.0f))
			,m_mark(1)
		{

		}

		double CalculateTriangleConcavity(const dgBigVector& normal,
												int32_t i0,
												int32_t i1,
												int32_t i2,
												const dgBigVector* const points)
		{
			uint32_t head = 1;
			uint32_t tail = 0;
			dgBigVector pool[1<<8][3];

			pool[0][0] = points[i0];
			pool[0][1] = points[i1];
			pool[0][2] = points[i2];

			const dgBigVector step(normal.Scale(double(4.0f) * GetDiagonal()));

			double concavity = float(0.0f);
			double minArea = float(0.125f);
			double minArea2 = minArea * minArea * 0.5f;

			int32_t maxCount = 4;
			uint32_t mask = (sizeof (pool) / (3 * sizeof (pool[0][0]))) - 1;

			const dgConvexHull3DFace* firstGuess = NULL;
			while ((tail != head) && (maxCount >= 0)) 
			{
				maxCount --;
				dgBigVector p0(pool[tail][0]);
				dgBigVector p1(pool[tail][1]);
				dgBigVector p2(pool[tail][2]);
				tail = (tail + 1) & mask;

				dgBigVector q1((p0 + p1 + p2).Scale(double(1.0f / 3.0f)));
				dgBigVector q0(q1 + step);

				double param = FastRayCast(q0, q1, &firstGuess);
				if (param > double(1.0f)) 
				{
					param = double(1.0f);
				}
				dgBigVector dq(step.Scale(float(1.0f) - param));
				double lenght2 = sqrt (dq % dq);
				if (lenght2 > concavity) 
				{
					concavity = lenght2;
				}

				if (((head + 1) & mask) != tail) 
				{
					dgBigVector edge10(p1 - p0);
					dgBigVector edge20(p2 - p0);
					dgBigVector n(edge10 * edge20);
					double area2 = n % n;
					if (area2 > minArea2) 
					{
						dgBigVector p01((p0 + p1).Scale(double(0.5f)));
						dgBigVector p12((p1 + p2).Scale(double(0.5f)));
						dgBigVector p20((p2 + p0).Scale(double(0.5f)));

						pool[head][0] = p0;
						pool[head][1] = p01;
						pool[head][2] = p20;
						head = (head + 1) & mask;

						if (((head + 1) & mask) != tail) 
						{
							pool[head][0] = p1;
							pool[head][1] = p12;
							pool[head][2] = p01;
							head = (head + 1) & mask;

							if (((head + 1) & mask) != tail)	
							{
								pool[head][0] = p2;
								pool[head][1] = p20;
								pool[head][2] = p12;
								head = (head + 1) & mask;
							}
						}
					}
				}
			}
			return concavity;
		}

		double FaceRayCast (const dgConvexHull3DFace* const face, const dgBigVector& origin, const dgBigVector& dist, double& normalProjection) const
		{
			int32_t i0 = face->m_index[0];
			int32_t i1 = face->m_index[1];
			int32_t i2 = face->m_index[2];

			const dgBigVector& p0 = m_points[i0];
			dgBigVector normal ((m_points[i1] - p0) * (m_points[i2] - p0));

			double N = (origin - p0) % normal;
			double D = dist % normal;

			if (fabs(D) < double (1.0e-16f)) { // 
				normalProjection = float (0.0);
				if (N > double (0.0f)) {
					return float (-1.0e30);
				} else {

					return float (1.0e30);
				}
			}
			normalProjection = D;
			return - N / D;
		}

		dgConvexHull3DFace* ClosestFaceVertexToPoint (const dgBigVector& point)
		{
			// note, for this function to be effective point should be an already close point to the Hull.
			// for example casting the point to the OBB or the AABB of the full is a good first guess. 
			dgConvexHull3DFace* closestFace = &GetFirst()->GetInfo();	
			int8_t pool[256 * (sizeof (dgConvexHull3DFace*) + sizeof (double))];
			dgUpHeap<dgConvexHull3DFace*,double> heap (pool, sizeof (pool));

			for (int32_t i = 0; i < 3; i ++) {
				dgBigVector dist (m_points[closestFace->m_index[i]] - point);
				heap.Push(closestFace, dist % dist);
			}

			m_mark ++;	
			double minDist = heap.Value();
			while (heap.GetCount()) {
				dgConvexHull3DFace* const face = heap[0];	
				if (heap.Value() < minDist) {
					minDist = heap.Value();
					closestFace = face;
				}
				heap.Pop();
				//face->m_mark = m_mark;
				face->SetMark(m_mark);
				for (int32_t i = 0; i < 3; i ++) {
					//const dgConvexHull3DFace* twin = &face->m_twin[i]->GetInfo();	
					dgConvexHull3DFace* twin = &face->GetTwin(i)->GetInfo();	
					//if (twin->m_mark != m_mark) {
					if (twin->GetMark() != m_mark) {
						dgBigVector dist (m_points[twin->m_index[i]] - point);
						// use hysteresis to prevent stops at a local minimal, but at the same time fast descend
						double dist2 = dist % dist;
						if (dist2 < (minDist * double (1.001f))) {
							heap.Push(twin, dist2);
						}
					}
				}
			}

			return closestFace;
		}

		// this version have input sensitive complexity (approximately  log2)
		// when casting parallel rays and using the last face as initial guess this version has const time complexity 
		double RayCast (const dgBigVector& localP0, const dgBigVector& localP1,const dgConvexHull3DFace** firstFaceGuess)
		{
			const dgConvexHull3DFace* face = &GetFirst()->GetInfo();
			if (firstFaceGuess && *firstFaceGuess) 
			{
				face = *firstFaceGuess;
			} 
			else 
			{
				if (GetCount() > 32) 
				{
					dgVector q0 (localP0);
					dgVector q1 (localP1);
					if (dgRayBoxClip (q0, q1, m_aabbP0, m_aabbP1)) 
					{
						face = ClosestFaceVertexToPoint (q0);
					}
				}
			}

			SparseArrayFixed< hacd::HaSizeT, 32, 2048 > tested;
			hacd::HaSizeT index = (hacd::HaSizeT)face;
			tested[index] = index;

			int8_t pool[256 * (sizeof (dgConvexHullRayCastData) + sizeof (double))];
			dgDownHeap<dgConvexHullRayCastData,double> heap (pool, sizeof (pool));

			double t0 = double (-1.0e20);			//for the maximum entering segment parameter;
			double t1 = double ( 1.0e20);			//for the minimum leaving segment parameter;
			dgBigVector dS (localP1 - localP0);		// is the segment direction vector;
			dgConvexHullRayCastData data;
			data.m_face = face;
			double t = FaceRayCast (face, localP0, dS, data.m_normalProjection);
			if (data.m_normalProjection >= float (0.0)) 
			{
				t = double (-1.0e30);
			}

			heap.Push (data, t);
			while (heap.GetCount()) 
			{
				dgConvexHullRayCastData data (heap[0]);
				double t = heap.Value();
				const dgConvexHull3DFace* face = data.m_face;
				double normalDistProjection = data.m_normalProjection;
				heap.Pop();
				bool foundThisBestFace = true;
				if (normalDistProjection < double (0.0f)) 
				{
					if (t > t0) 
					{
						t0 = t;
					}
					if (t0 > t1) 
					{
						return double (1.2f);
					}
				} 
				else 
				{
					foundThisBestFace = false;
				}

				for (int32_t i = 0; i < 3; i ++) 
				{
					dgConvexHull3DFace* const face1 = &face->GetTwin(i)->GetInfo();

					hacd::HaSizeT index = (hacd::HaSizeT)face1;
					if ( !tested.find(index) )
					{
						tested[index] = index;
						dgConvexHullRayCastData data;
						data.m_face = face1;
						double t = FaceRayCast (face1, localP0, dS, data.m_normalProjection);
						if (data.m_normalProjection >= float (0.0)) 
						{
							t = double (-1.0e30);
						} 
						else if (t > t0) 
						{
							foundThisBestFace = false;
						} 
						else if (fabs (t - t0) < double (1.0e-10f)) 
						{
							return dgConvexHull3d::RayCast (localP0, localP1);
						}
						if ((heap.GetCount() + 2)>= heap.GetMaxCount()) 
						{
							// remove t values that are old and way outside interval [0.0, 1.0]  
							for (int32_t i = heap.GetCount() - 1; i >= 0; i--) 
							{
								double val = heap.Value(i);
								if ((val < double (-100.0f)) || (val > double (100.0f))) 
								{
									heap.Remove(i);
								}
							}
						}
						heap.Push (data, t);
					}
				}
				if (foundThisBestFace) 
				{
					if ((t0 >= double (0.0f)) && (t0 <= double (1.0f))) 
					{
						if (firstFaceGuess) 
						{
							*firstFaceGuess = face;
						}
						return t0;
					}
					break;
				}
			}

			return double (1.2f);

		}

		double FastRayCast (const dgBigVector& localP0, const dgBigVector& localP1,const dgConvexHull3DFace** guess)
		{
			return RayCast (localP0, localP1, guess);
		}

		int32_t m_mark;
	};

	class dgHACDConvacityLookAheadTree : public UANS::UserAllocated
	{
		public:
			dgHACDConvacityLookAheadTree (dgEdge* const face, double concavity)
			:m_concavity(concavity)	
			,m_faceList ()
			,m_left (NULL)
			,m_right (NULL)
		{
			m_faceList.Append(face);
		}


		dgHACDConvacityLookAheadTree (dgHACDConvacityLookAheadTree* const leftChild, dgHACDConvacityLookAheadTree* const rightChild, double concavity)
			:m_concavity(concavity)	
			,m_faceList ()
			,m_left (leftChild)
			,m_right (rightChild)
		{
			HACD_ASSERT (leftChild);
			HACD_ASSERT (rightChild);

			double concavityTest = m_concavity - double (1.0e-5f);
			//if ((m_left->m_faceList.GetCount() == 1) || (m_right->m_faceList.GetCount() == 1)) {
			if ((((m_left->m_faceList.GetCount() == 1) || (m_right->m_faceList.GetCount() == 1))) ||
				((concavityTest <= m_left->m_concavity) && (concavityTest <= m_right->m_concavity))) {
					//The the parent has lower concavity this mean that the two do no add more detail, 
					//the can be deleted and replaced the parent node
					// for example the two children can be two convex strips that are part of a larger convex piece
					// but each part has a non zero concavity, while the convex part has a lower concavity 
					m_faceList.Merge (m_left->m_faceList);
					m_faceList.Merge (m_right->m_faceList);

					delete m_left;
					delete m_right;
					m_left = NULL;
					m_right = NULL;
			} else {
				for (dgList<dgEdge*>::dgListNode* node = m_left->m_faceList.GetFirst(); node; node = node->GetNext()) {
					m_faceList.Append(node->GetInfo());
				}
				for (dgList<dgEdge*>::dgListNode* node = m_right->m_faceList.GetFirst(); node; node = node->GetNext()) {
					m_faceList.Append(node->GetInfo());
				}
			}
		}

		~dgHACDConvacityLookAheadTree ()
		{
			if (m_left) {
				HACD_ASSERT (m_right);
				delete m_left;
				delete m_right;
			}
		}

		int32_t GetNodesCount () const
		{
			int32_t count = 0;
			int32_t stack = 1;
			const dgHACDConvacityLookAheadTree* pool[1024];
			pool[0] = this;
			while (stack) {
				stack --;
				count ++;
				const dgHACDConvacityLookAheadTree* const root = pool[stack];
				if (root->m_left) {
					HACD_ASSERT (root->m_right);
					pool[stack] = root->m_left;
					stack ++;
					HACD_ASSERT ((uint32_t)stack < sizeof (pool)/sizeof (pool[0]));
					pool[stack] = root->m_right;
					stack ++;
					HACD_ASSERT ((uint32_t)stack < sizeof (pool)/sizeof (pool[0]));
				}
			}
			return count;
		}

		void ReduceByCount (int32_t count, dgDownHeap<dgHACDConvacityLookAheadTree*, double>& approximation)
		{
			if (count < 1) {
				count = 1;
			}
//			int32_t nodesCount = GetNodesCount();

			approximation.Flush();
			dgHACDConvacityLookAheadTree* tmp = this;
			approximation.Push(tmp, m_concavity);
//			nodesCount --;
			//while (nodesCount && (approximation.GetCount() < count) && (approximation.Value() >= float (0.0f))) {
			while ((approximation.GetCount() < count) && (approximation.Value() >= float (0.0f))) {
				dgHACDConvacityLookAheadTree* worseCluster = approximation[0];
				if (!worseCluster->m_left && approximation.Value() >= float (0.0f)) {
					approximation.Pop();
					approximation.Push(worseCluster, float (-1.0f));
				} else {
					HACD_ASSERT (worseCluster->m_left);
					HACD_ASSERT (worseCluster->m_right);
					approximation.Pop();
					approximation.Push(worseCluster->m_left, worseCluster->m_left->m_concavity);
					approximation.Push(worseCluster->m_right, worseCluster->m_right->m_concavity);
//					nodesCount -= 2;
				}
			}
		}


		void ReduceByConcavity (double concavity, dgDownHeap<dgHACDConvacityLookAheadTree*, double>& approximation)
		{
			approximation.Flush();
			dgHACDConvacityLookAheadTree* tmp = this;

			approximation.Push(tmp, m_concavity);
			while (approximation.Value() > concavity) {
				dgHACDConvacityLookAheadTree* worseCluster = approximation[0];
				if (!worseCluster->m_left && approximation.Value() >= float (0.0f)) {
					approximation.Pop();
					approximation.Push(worseCluster, float (-1.0f));
				} else {
					HACD_ASSERT (worseCluster->m_left);
					HACD_ASSERT (worseCluster->m_right);
					approximation.Pop();
					approximation.Push(worseCluster->m_left, worseCluster->m_left->m_concavity);
					approximation.Push(worseCluster->m_right, worseCluster->m_right->m_concavity);
				}
			}
		}

		double m_concavity; 
		dgList<dgEdge*> m_faceList;
		dgHACDConvacityLookAheadTree* m_left;
		dgHACDConvacityLookAheadTree* m_right;
	};

	class dgPairProxy
	{
		public:
		dgPairProxy()
			:m_nodeA(NULL)
			,m_nodeB(NULL)
			,m_hierachicalClusterIndexA(0)
			,m_hierachicalClusterIndexB(0)
			,m_area(double(0.0f))
		{
		}

		~dgPairProxy()
		{
		}

		dgListNode* m_nodeA;
		dgListNode* m_nodeB;
		int32_t m_hierachicalClusterIndexA;
		int32_t m_hierachicalClusterIndexB;
		double m_area;
		double m_distanceConcavity;
	};

	class dgHACDRayCasterContext: public dgFastRayTest
	{
		public:
		dgHACDRayCasterContext (const dgVector& l0, const dgVector& l1, dgHACDClusterGraph* const me, int32_t mycolor)
			:dgFastRayTest (l0, l1)
			,m_myColor(mycolor)
			,m_colorHit(-1)
			,m_param (1.0f) 
			,m_me (me) 
		{
		}

		int32_t m_myColor;
		int32_t m_colorHit;
		float m_param;
		dgHACDClusterGraph* m_me;
	};


	dgHACDClusterGraph(dgMeshEffect& mesh, float backFaceDistanceFactor,hacd::ICallback* reportProgressCallback)
		:dgGraph<dgHACDCluster, dgHACDEdge> ()
		,dgAABBPolygonSoup()
		,m_mark(0)
		,m_faceCount(0)
		,m_vertexMark(0)
		,m_progress(0)
		,m_cancavityTreeIndex(0)
		,m_vertexMarks(NULL)
		,m_invFaceCount(float (1.0f))
		,m_diagonal(double(1.0f))
		,m_vertexPool(NULL)
		,m_proxyList()
		,m_concavityTreeArray(NULL)
		,m_convexProximation()
		,m_priorityHeap (mesh.GetCount() + 2048)
		,m_reportProgressCallback (reportProgressCallback)
//		,m_parallerConcavityCalculator()
	{
		
//		m_parallerConcavityCalculator.SetThreadsCount(DG_CONCAVITY_MAX_THREADS);

		// precondition the mesh for better approximation
		mesh.ConvertToPolygons();

		m_faceCount = mesh.GetTotalFaceCount();

		m_invFaceCount = float (1.0f) / (m_faceCount);

		// init some auxiliary structures
		int32_t vertexCount = mesh.GetVertexCount();
		m_vertexMarks =  (int32_t*) HACD_ALLOC(vertexCount * sizeof(int32_t));
		m_vertexPool =  (dgBigVector*) HACD_ALLOC(vertexCount * sizeof(dgBigVector));
		memset(m_vertexMarks, 0, vertexCount * sizeof(int32_t));

		m_cancavityTreeIndex = m_faceCount + 1;
		m_concavityTreeArray = (dgHACDConvacityLookAheadTree**) HACD_ALLOC(2 * m_cancavityTreeIndex * sizeof(dgHACDConvacityLookAheadTree*));
		memset(m_concavityTreeArray, 0, 2 * m_cancavityTreeIndex * sizeof(dgHACDConvacityLookAheadTree*));

		// scan the mesh and and add a node for each face
		int32_t color = 1;
		dgMeshEffect::Iterator iter(mesh);

		int32_t meshMask = mesh.IncLRU();
		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();
		for (iter.Begin(); iter; iter++) {
			dgEdge* const edge = &(*iter);
			if ((edge->m_mark != meshMask) && (edge->m_incidentFace > 0)) {

				// call the progress callback
				//ReportProgress();

				dgListNode* const clusterNode = AddNode ();
				dgHACDCluster& cluster = clusterNode->GetInfo().m_nodeData;

				double perimeter = double(0.0f);
				dgEdge* ptr = edge;
				do {
					dgBigVector p1p0(points[ptr->m_incidentVertex] - points[ptr->m_prev->m_incidentVertex]);
					perimeter += sqrt(p1p0 % p1p0);
					ptr->m_incidentFace = color;
					ptr->m_userData = PTR_TO_UINT64(clusterNode);
					ptr->m_mark = meshMask;
					ptr = ptr->m_next;
				} while (ptr != edge);

				dgBigVector normal = mesh.FaceNormal(edge, &points[0][0], sizeof(dgBigVector));
				double mag = sqrt(normal % normal);

				cluster.m_color = color;
				cluster.m_hierachicalClusterIndex = color;
				cluster.m_area = double(0.5f) * mag;
				cluster.m_concavity = CalculateConcavityMetric (double (0.0f), cluster.m_area, perimeter, 1, 0);

				dgHACDClusterFace& face = cluster.Append()->GetInfo();
				face.m_edge = edge;
				face.m_area = double(0.5f) * mag;
				face.m_normal = normal.Scale(double(1.0f) / mag);

				m_concavityTreeArray[color] = HACD_NEW(dgHACDConvacityLookAheadTree)(edge, double (0.0f));

				color ++;
			}
		}

		// add all link adjacent faces links
		for (dgListNode* clusterNode = GetFirst(); clusterNode; clusterNode = clusterNode->GetNext()) {

			// call the progress callback
			//ReportProgress();

			dgHACDCluster& cluster = clusterNode->GetInfo().m_nodeData;
			dgHACDClusterFace& face = cluster.GetFirst()->GetInfo();
			dgEdge* const edge = face.m_edge;
			dgEdge* ptr = edge; 
			do {
				if (ptr->m_twin->m_incidentFace > 0) {
					HACD_ASSERT (ptr->m_twin->m_userData);
					dgListNode* const twinClusterNode = (dgListNode*) ptr->m_twin->m_userData;
					HACD_ASSERT (twinClusterNode);

					bool doubleEdge = false;
					for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNode = clusterNode->GetInfo().GetFirst(); edgeNode; edgeNode = edgeNode->GetNext()) {
						if (edgeNode->GetInfo().m_node == twinClusterNode) {
							doubleEdge = true;
							break;
						}
					}
					if (!doubleEdge) {
						clusterNode->GetInfo().AddEdge (twinClusterNode);
					}
				}
				ptr = ptr->m_next;
			} while (ptr != edge);
		}

		Trace();

		// add links to back faces
		dgPolygonSoupDatabaseBuilder builder;
		dgVector polygon[64];
		int32_t indexList[64];

		dgMatrix matrix (dgGetIdentityMatrix());
		for (uint32_t i = 0; i < sizeof (polygon) / sizeof (polygon[0]); i ++) {
			indexList[i] = (int32_t)i;
		}

		dgBigVector minAABB;
		dgBigVector maxAABB;
		mesh.CalculateAABB (minAABB, maxAABB);
		maxAABB -= minAABB;
		float rayDiagonalLength = float (sqrt (maxAABB % maxAABB));
		m_diagonal = rayDiagonalLength;

		builder.Begin();
		dgTree<dgListNode*,int32_t> clusterMap;
		for (dgListNode* clusterNode = GetFirst(); clusterNode; clusterNode = clusterNode->GetNext()) {

			// call the progress callback
			//ReportProgress();

			dgHACDCluster& cluster = clusterNode->GetInfo().m_nodeData;
			clusterMap.Insert(clusterNode, cluster.m_color);
			dgHACDClusterFace& face = cluster.GetFirst()->GetInfo();
			dgEdge* const edge = face.m_edge;
			int32_t count = 0;
			dgEdge* ptr = edge;
			do {
				polygon[count] = points[ptr->m_incidentVertex];
				count ++;
				ptr = ptr->m_prev;
			} while (ptr != edge);

			builder.AddMesh(&polygon[0].m_x, count, sizeof (dgVector), 1, &count, indexList, &cluster.m_color, matrix);
		}
		builder.End(false);
		Create (builder, false);


		float distanceThreshold = rayDiagonalLength * backFaceDistanceFactor;
		for (dgListNode* clusterNodeA = GetFirst(); clusterNodeA; clusterNodeA = clusterNodeA->GetNext()) {

			// call the progress callback
			//ReportProgress();
			dgHACDCluster& clusterA = clusterNodeA->GetInfo().m_nodeData;
			dgHACDClusterFace& faceA = clusterA.GetFirst()->GetInfo();
			dgEdge* const edgeA = faceA.m_edge;
			dgEdge* ptr = edgeA;

			dgVector p0 (points[ptr->m_incidentVertex]);
			dgVector p1 (points[ptr->m_next->m_incidentVertex]);
			ptr = ptr->m_next->m_next;
			do {
				dgVector p2 (points[ptr->m_incidentVertex]);
				dgVector p01 ((p0 + p1).Scale (float (0.5f)));
				dgVector p12 ((p1 + p2).Scale (float (0.5f)));
				dgVector p20 ((p2 + p0).Scale (float (0.5f)));

				CastBackFace (clusterNodeA, p0, p01, p20, distanceThreshold, clusterMap);
				CastBackFace (clusterNodeA, p1, p12, p01, distanceThreshold, clusterMap);
				CastBackFace (clusterNodeA, p2, p20, p12, distanceThreshold, clusterMap);
				CastBackFace (clusterNodeA, p01, p12, p20, distanceThreshold, clusterMap);

				p1 = p2;
				ptr = ptr->m_next;
			} while (ptr != edgeA);
		}

		Trace();
	}

	~dgHACDClusterGraph ()
	{
		for (int32_t i = 0; i < m_faceCount * 2; i ++) {
			if (m_concavityTreeArray[i]) {
				delete m_concavityTreeArray[i];
			}
		}

		HACD_FREE(m_concavityTreeArray);
		HACD_FREE(m_vertexPool);
		HACD_FREE(m_vertexMarks);
	}


	void CastBackFace (
		dgListNode* const clusterNodeA,
		const dgVector& p0, 
		const dgVector& p1, 
		const dgVector& p2,
		float distanceThreshold,
		dgTree<dgListNode*,int32_t>& clusterMap)
	{
		dgVector origin ((p0 + p1 + p2).Scale (float (1.0f/3.0f)));

		float rayDistance = distanceThreshold * float (2.0f);

		dgHACDCluster& clusterA = clusterNodeA->GetInfo().m_nodeData;
		dgHACDClusterFace& faceA = clusterA.GetFirst()->GetInfo();
		dgVector end (origin - dgVector (faceA.m_normal).Scale (rayDistance));

		dgHACDRayCasterContext ray (origin, end, this, clusterA.m_color);
		ForAllSectorsRayHit(ray, RayHit, &ray);

		if (ray.m_colorHit != -1) {
			HACD_ASSERT (ray.m_colorHit != ray.m_myColor);
			float distance = rayDistance * ray.m_param;

			if (distance < distanceThreshold) {

				HACD_ASSERT (ray.m_colorHit != clusterA.m_color);
				HACD_ASSERT (clusterMap.Find(ray.m_colorHit));
				dgListNode* const clusterNodeB = clusterMap.Find(ray.m_colorHit)->GetInfo();
				dgHACDCluster& clusterB = clusterNodeB->GetInfo().m_nodeData;

				dgHACDClusterFace& faceB = clusterB.GetFirst()->GetInfo();
				dgEdge* const edgeB = faceB.m_edge;

				bool isAdjacent = false;
				dgEdge* ptrA = faceA.m_edge;
				do {
					dgEdge* ptrB = edgeB;
					do {
						if (ptrB->m_twin == ptrA) {
							ptrA = faceA.m_edge->m_prev;
							isAdjacent = true;
							break;
						}
						ptrB = ptrB->m_next;
					} while (ptrB != edgeB);

					ptrA = ptrA->m_next;
				} while (ptrA != faceA.m_edge);

				if (!isAdjacent) {

					isAdjacent = false;
					for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNode = clusterNodeA->GetInfo().GetFirst(); edgeNode; edgeNode = edgeNode->GetNext()) {
						if (edgeNode->GetInfo().m_node == clusterNodeB) {
							isAdjacent = true;
							break;
						}
					}

					if (!isAdjacent) {

						dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* const edgeNodeAB = clusterNodeA->GetInfo().AddEdge (clusterNodeB);
						dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* const edgeNodeBA = clusterNodeB->GetInfo().AddEdge (clusterNodeA);

						dgHACDEdge& edgeAB = edgeNodeAB->GetInfo().m_edgeData;
						dgHACDEdge& edgeBA = edgeNodeBA->GetInfo().m_edgeData;
						edgeAB.m_backFaceHandicap = double (0.5f);
						edgeBA.m_backFaceHandicap = double (0.5f);
					}
				}
			}
		}
	}


	void Trace() const
	{
	}


	// you can insert cal callback here  to print the progress as it collapse clusters
	void ReportProgress ()
	{
		m_progress ++;
		if (m_reportProgressCallback) 
		{
			float progress = float(m_progress) * m_invFaceCount;
			m_reportProgressCallback->ReportProgress("Performing HACD",progress);
		}
	}

	dgMeshEffect* CreatePatitionMesh (dgMeshEffect& mesh, int32_t maxVertexPerHull)
	{
		dgMeshEffect* const convexPartionMesh = HACD_NEW(dgMeshEffect)(true);

		dgMeshEffect::dgVertexAtribute polygon[256];
		memset(polygon, 0, sizeof(polygon));
		dgArray<dgBigVector> convexVertexBuffer(mesh.GetCount());
		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();

		convexPartionMesh->BeginPolygon();
		double layer = double (0.0f);
		for (dgList<dgHACDConvacityLookAheadTree*>::dgListNode* clusterNode = m_convexProximation.GetFirst(); clusterNode; clusterNode = clusterNode->GetNext()) {
			dgHACDConvacityLookAheadTree* const cluster = clusterNode->GetInfo();

			int32_t vertexCount = 0;
			for (dgList<dgEdge*>::dgListNode* faceNode = cluster->m_faceList.GetFirst(); faceNode; faceNode = faceNode->GetNext()) {
				dgEdge* const edge = faceNode->GetInfo();
				dgEdge* ptr = edge;
				do {
					int32_t index = ptr->m_incidentVertex;
					convexVertexBuffer[vertexCount] = points[index];
					vertexCount++;
					ptr = ptr->m_next;
				} while (ptr != edge);
			}
			dgConvexHull3d convexHull(&convexVertexBuffer[0].m_x, sizeof(dgBigVector), vertexCount, 0.0, maxVertexPerHull);
			if (convexHull.GetCount()) {
				const dgBigVector* const vertex = convexHull.GetVertexPool();
				for (dgConvexHull3d::dgListNode* node = convexHull.GetFirst(); node; node = node->GetNext()) {
					const dgConvexHull3DFace* const face = &node->GetInfo();

					int32_t i0 = face->m_index[0];
					int32_t i1 = face->m_index[1];
					int32_t i2 = face->m_index[2];

					polygon[0].m_vertex = vertex[i0];
					polygon[0].m_vertex.m_w = layer;

					polygon[1].m_vertex = vertex[i1];
					polygon[1].m_vertex.m_w = layer;

					polygon[2].m_vertex = vertex[i2];
					polygon[2].m_vertex.m_w = layer;

					convexPartionMesh->AddPolygon(3, &polygon[0].m_vertex.m_x, sizeof(dgMeshEffect::dgVertexAtribute), 0);
				}
				layer += double (1.0f);
			}
		}
		convexPartionMesh->EndPolygon(1.0e-5f);

		m_progress = m_faceCount - 1;
		ReportProgress();

		return convexPartionMesh;
	}



	static float RayHit (void *context, const float* const polygon, int32_t strideInBytes, const int32_t* const indexArray, int32_t indexCount)
	{
		dgHACDRayCasterContext& me = *((dgHACDRayCasterContext*) context);
		dgVector normal (&polygon[indexArray[indexCount] * (strideInBytes / sizeof (float))]);
		float t = me.PolygonIntersect (normal, polygon, strideInBytes, indexArray, indexCount);
		if (t < me.m_param) {
			int32_t faceColor = (int32_t)me.m_me->GetTagId(indexArray);
			if (faceColor != me.m_myColor) {
				me.m_param = t;
				me.m_colorHit = faceColor;
			}
		}
		return t;
	}


	double ConcavityByFaceMedian (int32_t faceCountA, int32_t faceCountB) const
	{
		double faceCountCost = DG_CONCAVITY_SCALE * double (0.1f) * (faceCountA + faceCountB) * m_invFaceCount;
		//faceCountCost *= 0;
		return faceCountCost;
	}

	double CalculateConcavityMetric (double convexConcavity, double area, double perimeter, int32_t faceCountA, int32_t faceCountB) const 
	{
		double edgeCost = perimeter * perimeter / (double(4.0f * 3.141592f) * area);
		return convexConcavity * DG_CONCAVITY_SCALE + edgeCost + ConcavityByFaceMedian (faceCountA, faceCountB);
	}

	void SubmitInitialEdgeCosts (dgMeshEffect& mesh) 
	{
		m_mark ++;
		for (dgListNode* clusterNodeA = GetFirst(); clusterNodeA; clusterNodeA = clusterNodeA->GetNext()) 
		{
			for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeAB = clusterNodeA->GetInfo().GetFirst(); edgeNodeAB; edgeNodeAB = edgeNodeAB->GetNext()) 
			{
				dgHACDEdge& edgeAB = edgeNodeAB->GetInfo().m_edgeData;
				double weight = edgeAB.m_backFaceHandicap; 
				if (edgeAB.m_mark != m_mark) 
				{
					edgeAB.m_mark = m_mark;
					dgListNode* const clusterNodeB = edgeNodeAB->GetInfo().m_node;
					for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeBA = clusterNodeB->GetInfo().GetFirst(); edgeNodeBA; edgeNodeBA = edgeNodeBA->GetNext()) 
					{
						dgListNode* const clusterNode = edgeNodeBA->GetInfo().m_node;
						if (clusterNode == clusterNodeA) 
						{
							dgHACDEdge& edgeBA = edgeNodeBA->GetInfo().m_edgeData;
							edgeBA.m_mark = m_mark;
							HACD_ASSERT (!edgeAB.m_proxyListNode);
							HACD_ASSERT (!edgeBA.m_proxyListNode);
							dgList<dgPairProxy>::dgListNode* const proxyNode = SubmitEdgeCost (mesh, clusterNodeA, clusterNodeB, weight * edgeBA.m_backFaceHandicap);
							edgeAB.m_proxyListNode = proxyNode;
							edgeBA.m_proxyListNode = proxyNode;
							break;
						}
					}
				}
			}
		}
	}

	int32_t CopyVertexToPool(const dgMeshEffect& mesh, const dgHACDCluster& cluster, int32_t start)
	{
		int32_t count = start;

		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();
		for (dgList<dgHACDClusterFace>::dgListNode* node = cluster.GetFirst(); node; node = node->GetNext()) {
			const dgHACDClusterFace& clusterFace = node->GetInfo();
			dgEdge* edge = clusterFace.m_edge;
			do {
				int32_t index = edge->m_incidentVertex;
				if (m_vertexMarks[index] != m_vertexMark) {
					m_vertexMarks[index] = m_vertexMark;
					m_vertexPool[count] = points[index];
					count++;
				}
				edge = edge->m_next;
			} while (edge != clusterFace.m_edge);
		}
		return count;
	}


	void MarkInteriorClusterEdges (dgMeshEffect& /*mesh*/, int32_t mark, const dgHACDCluster& cluster, int32_t colorA, int32_t colorB) const
	{
		HACD_ASSERT (colorA != colorB);
		for (dgList<dgHACDClusterFace>::dgListNode* node = cluster.GetFirst(); node; node = node->GetNext()) {
			dgHACDClusterFace& clusterFace = node->GetInfo();
			dgEdge* edge = clusterFace.m_edge;
			do {
				if ((edge->m_twin->m_incidentFace == colorA) || (edge->m_twin->m_incidentFace == colorB)) {
					edge->m_mark = mark;
					edge->m_twin->m_mark = mark;
				}
				edge = edge->m_next;
			} while (edge != clusterFace.m_edge);
		}
	}

	double CalculateClusterPerimeter (dgMeshEffect& mesh, int32_t /*mark*/, const dgHACDCluster& cluster, int32_t colorA, int32_t colorB) const
	{
		HACD_ASSERT (colorA != colorB);
		double perimeter = double (0.0f);
		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();
		for (dgList<dgHACDClusterFace>::dgListNode* node = cluster.GetFirst(); node; node = node->GetNext()) {
			dgHACDClusterFace& clusterFace = node->GetInfo();
			dgEdge* edge = clusterFace.m_edge;
			do {
				if (!((edge->m_twin->m_incidentFace == colorA) || (edge->m_twin->m_incidentFace == colorB))) {
					dgBigVector p1p0(points[edge->m_twin->m_incidentVertex] - points[edge->m_incidentVertex]);
					perimeter += sqrt(p1p0 % p1p0);
				}
				edge = edge->m_next;
			} while (edge != clusterFace.m_edge);
		}

		return perimeter;
	}

	void HeapCollectGarbage () 
	{
		if ((m_priorityHeap.GetCount() + 20) > m_priorityHeap.GetMaxCount()) {
			for (int32_t i = m_priorityHeap.GetCount() - 1; i >= 0; i--) {
				dgList<dgPairProxy>::dgListNode* const emptyNode = m_priorityHeap[i];
				dgPairProxy& emptyPair = emptyNode->GetInfo();
				if ((emptyPair.m_nodeA == NULL) && (emptyPair.m_nodeB == NULL)) {
					m_priorityHeap.Remove(i);
				}
			}
		}
	}


	
	double CalculateConcavity(dgHACDConveHull& hull,
									const dgMeshEffect& mesh,
									const dgHACDCluster& cluster,
									double &concavity)

	{
		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();
		for (dgList<dgHACDClusterFace>::dgListNode* node = cluster.GetFirst(); node; node = node->GetNext()) 
		{
			dgHACDClusterFace& clusterFace = node->GetInfo();
			dgEdge* edge = clusterFace.m_edge;
			int32_t i0 = edge->m_incidentVertex;
			int32_t i1 = edge->m_next->m_incidentVertex;
			for (dgEdge* ptr = edge->m_next->m_next; ptr != edge; ptr = ptr->m_next) 
			{
				int32_t i2 = ptr->m_incidentVertex;
				double val = hull.CalculateTriangleConcavity(clusterFace.m_normal, i0, i1, i2, points);
				if (val > concavity) 
				{
					concavity = val;
				}
				i1 = i2;
			}
		}
		return concavity;
	}

	double CalculateConcavitySingleThread (dgHACDConveHull& hull, dgMeshEffect& mesh, dgHACDCluster& clusterA, dgHACDCluster& clusterB)
	{
		double c1=0;
		double c2=0;

		// gather all of the work to be done to compute the concavity for both clusters in parallel
		CalculateConcavity(hull, mesh, clusterA, c1);
		CalculateConcavity(hull, mesh, clusterB, c2);
		return GetMax(c1,c2);
	}

	dgList<dgPairProxy>::dgListNode* SubmitEdgeCost (dgMeshEffect& mesh, dgListNode* const clusterNodeA, dgListNode* const clusterNodeB, double perimeterHandicap)
	{
		dgHACDCluster& clusterA = clusterNodeA->GetInfo().m_nodeData;
		dgHACDCluster& clusterB = clusterNodeB->GetInfo().m_nodeData;
		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();

		bool flatStrip = true;
		double tol = double (1.0e-5f) * m_diagonal;
		dgHACDClusterFace& clusterFaceA = clusterA.GetFirst()->GetInfo();
		dgBigPlane plane(clusterFaceA.m_normal, -(points[clusterFaceA.m_edge->m_incidentVertex] % clusterFaceA.m_normal));

		if (clusterA.GetCount() > 1) 
		{
			flatStrip = clusterA.IsCoplanar(plane, mesh, tol);
		}

		if (flatStrip) 
		{
			flatStrip = clusterB.IsCoplanar(plane, mesh, tol);
		}

		dgList<dgPairProxy>::dgListNode* pairNode = NULL;

		if (!flatStrip) 
		{
			m_vertexMark ++;
			int32_t vertexCount = CopyVertexToPool(mesh, clusterA, 0);
			vertexCount = CopyVertexToPool(mesh, clusterB, vertexCount);

			dgHACDConveHull convexHull(m_vertexPool, vertexCount);

			if (convexHull.GetVertexCount()) 
			{
				int32_t mark = mesh.IncLRU();
				MarkInteriorClusterEdges (mesh, mark, clusterA, clusterA.m_color, clusterB.m_color);
				MarkInteriorClusterEdges (mesh, mark, clusterB, clusterA.m_color, clusterB.m_color);

				double area = clusterA.m_area + clusterB.m_area;
				double perimeter = CalculateClusterPerimeter (mesh, mark, clusterA, clusterA.m_color, clusterB.m_color) +
									  CalculateClusterPerimeter (mesh, mark, clusterB, clusterA.m_color, clusterB.m_color);

	
				double concavity = double (0.0f);
				{
					concavity = CalculateConcavitySingleThread (convexHull, mesh, clusterA, clusterB);
				}

				if (concavity < double(1.0e-3f)) 
				{
					concavity = double(0.0f);
				}

				// see if the heap will overflow
				HeapCollectGarbage ();

				// add a new pair to the heap
				dgList<dgPairProxy>::dgListNode* pairNode = m_proxyList.Append();
				dgPairProxy& pair = pairNode->GetInfo();
				pair.m_nodeA = clusterNodeA;
				pair.m_nodeB = clusterNodeB;
				pair.m_distanceConcavity = concavity;
				pair.m_hierachicalClusterIndexA = clusterA.m_hierachicalClusterIndex;
				pair.m_hierachicalClusterIndexB = clusterB.m_hierachicalClusterIndex;

				pair.m_area = area;
				double cost = CalculateConcavityMetric (concavity, area, perimeter * perimeterHandicap, clusterA.GetCount(), clusterB.GetCount());
				m_priorityHeap.Push(pairNode, cost);

				return pairNode;
			}
		}
		return pairNode;
	}


	void CollapseEdge (dgList<dgPairProxy>::dgListNode* const pairNode, dgMeshEffect& mesh, double concavity)
	{
		dgListNode* adjacentNodes[1024];
		dgPairProxy& pair = pairNode->GetInfo();

		
		HACD_ASSERT((pair.m_nodeA && pair.m_nodeB) || (!pair.m_nodeA && !pair.m_nodeB));
		if (pair.m_nodeA && pair.m_nodeB) 
		{
			// call the progress callback
			ReportProgress();

			dgListNode* const clusterNodeA = pair.m_nodeA;
			dgListNode* const clusterNodeB = pair.m_nodeB;
			HACD_ASSERT (clusterNodeA != clusterNodeB);

			dgHACDCluster& clusterA = clusterNodeA->GetInfo().m_nodeData;
			dgHACDCluster& clusterB = clusterNodeB->GetInfo().m_nodeData;

			HACD_ASSERT (&clusterA != &clusterB);
			HACD_ASSERT(clusterA.m_color != clusterB.m_color);

			dgHACDConvacityLookAheadTree* const leftTree = m_concavityTreeArray[pair.m_hierachicalClusterIndexA];
			dgHACDConvacityLookAheadTree* const rightTree = m_concavityTreeArray[pair.m_hierachicalClusterIndexB];
			HACD_ASSERT (leftTree);
			HACD_ASSERT (rightTree);
			m_concavityTreeArray[pair.m_hierachicalClusterIndexA] = NULL;
			m_concavityTreeArray[pair.m_hierachicalClusterIndexB] = NULL;
			HACD_ASSERT (m_cancavityTreeIndex < (2 * (m_faceCount + 1)));

			double treeConcavity = pair.m_distanceConcavity;
//			 HACD_ASSERT (treeConcavity < 0.1);
			m_concavityTreeArray[m_cancavityTreeIndex] = HACD_NEW(dgHACDConvacityLookAheadTree)(leftTree, rightTree, treeConcavity);
			clusterA.m_hierachicalClusterIndex = m_cancavityTreeIndex;
			clusterB.m_hierachicalClusterIndex = m_cancavityTreeIndex;
			m_cancavityTreeIndex ++;

			// merge two clusters
			while (clusterB.GetCount()) {

				dgHACDCluster::dgListNode* const nodeB = clusterB.GetFirst();
				clusterB.Unlink(nodeB);
	
				// now color code all faces of the merged cluster
				dgHACDClusterFace& faceB = nodeB->GetInfo();
				dgEdge* ptr = faceB.m_edge;
				do {
					ptr->m_incidentFace = clusterA.m_color;
					ptr = ptr->m_next;
				} while (ptr != faceB.m_edge);
				clusterA.Append(nodeB);
			}
			clusterA.m_area = pair.m_area;
			clusterA.m_concavity = concavity;

			// invalidate all proxies that are still in the heap
			int32_t adjacentCount = 1;
			adjacentNodes[0] = clusterNodeA;
			for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeAB = clusterNodeA->GetInfo().GetFirst(); edgeNodeAB; edgeNodeAB = edgeNodeAB->GetNext()) {
				dgHACDEdge& edgeAB = edgeNodeAB->GetInfo().m_edgeData;
				dgList<dgPairProxy>::dgListNode* const proxyNode = (dgList<dgPairProxy>::dgListNode*) edgeAB.m_proxyListNode;
				if (proxyNode) {
					dgPairProxy& pairProxy = proxyNode->GetInfo();
					HACD_ASSERT ((edgeNodeAB->GetInfo().m_node == pairProxy.m_nodeA) || (edgeNodeAB->GetInfo().m_node == pairProxy.m_nodeB));
					pairProxy.m_nodeA = NULL;
					pairProxy.m_nodeB = NULL;
					edgeAB.m_proxyListNode = NULL;
				}

				adjacentNodes[adjacentCount] = edgeNodeAB->GetInfo().m_node;
				adjacentCount ++;
				HACD_ASSERT ((uint32_t)adjacentCount < sizeof (adjacentNodes)/ sizeof (adjacentNodes[0]));
			}

			for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeBA = clusterNodeB->GetInfo().GetFirst(); edgeNodeBA; edgeNodeBA = edgeNodeBA->GetNext()) {
				dgHACDEdge& edgeBA = edgeNodeBA->GetInfo().m_edgeData;
				dgList<dgPairProxy>::dgListNode* const proxyNode = (dgList<dgPairProxy>::dgListNode*) edgeBA.m_proxyListNode;
				if (proxyNode) {
					dgPairProxy& pairProxy = proxyNode->GetInfo();
					pairProxy.m_nodeA = NULL;
					pairProxy.m_nodeB = NULL;
					edgeBA.m_proxyListNode = NULL;
				}

				bool alreadyLinked = false;
				dgListNode* const node = edgeNodeBA->GetInfo().m_node;
				for (int32_t i = 0; i < adjacentCount; i ++) {
					if (node == adjacentNodes[i]) {
						alreadyLinked = true;
						break;
					}
				}
				if (!alreadyLinked) {
					clusterNodeA->GetInfo().AddEdge (node);
					node->GetInfo().AddEdge (clusterNodeA);
				}
			}
			DeleteNode (clusterNodeB);

			// submit all new costs for each edge connecting this new node to any other node 
			for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeAB = clusterNodeA->GetInfo().GetFirst(); edgeNodeAB; edgeNodeAB = edgeNodeAB->GetNext()) 
			{
				dgHACDEdge& edgeAB = edgeNodeAB->GetInfo().m_edgeData;
				dgListNode* const clusterNodeB = edgeNodeAB->GetInfo().m_node;
				double weigh = edgeAB.m_backFaceHandicap;
				for (dgGraphNode<dgHACDCluster, dgHACDEdge>::dgListNode* edgeNodeBA = clusterNodeB->GetInfo().GetFirst(); edgeNodeBA; edgeNodeBA = edgeNodeBA->GetNext()) 
				{
					dgListNode* const clusterNode = edgeNodeBA->GetInfo().m_node;
					if (clusterNode == clusterNodeA) 
					{
						dgHACDEdge& edgeBA = edgeNodeBA->GetInfo().m_edgeData;
						dgList<dgPairProxy>::dgListNode* const proxyNode = SubmitEdgeCost (mesh, clusterNodeA, clusterNodeB, weigh * edgeBA.m_backFaceHandicap);
						if (proxyNode) 
						{
							edgeBA.m_proxyListNode = proxyNode;
							edgeAB.m_proxyListNode = proxyNode;
						}
						break;
					}
				}
			}
		}
		m_proxyList.Remove(pairNode);
	}

#ifdef DG_BUILD_HIERACHICAL_HACD
	void CollapseClusters (dgMeshEffect& mesh, double maxConcavity, int32_t maxClustesCount)
	{

		maxConcavity *= (m_diagonal * DG_CONCAVITY_SCALE);
		while (m_priorityHeap.GetCount()) {
			double concavity =  m_priorityHeap.Value();
			dgList<dgPairProxy>::dgListNode* const pairNode = m_priorityHeap[0];
			m_priorityHeap.Pop();
			CollapseEdge (pairNode, mesh, concavity);

//if (m_progress == 24)
//break;

		}



		int32_t treeCounts = 0;
		for (int32_t i = 0; i < m_cancavityTreeIndex; i ++) {
			if (m_concavityTreeArray[i]) {
				m_concavityTreeArray[treeCounts] = m_concavityTreeArray[i];
				m_concavityTreeArray[i] = NULL;
				treeCounts ++;
			}
		}

		if (treeCounts > 1) {

			for (int32_t i = 0; i < treeCounts; i ++) {
				if (m_concavityTreeArray[i]->m_faceList.GetCount()==1) {
					delete m_concavityTreeArray[i];
					m_concavityTreeArray[i] = m_concavityTreeArray[treeCounts-1];
					m_concavityTreeArray[treeCounts-1]= NULL;
					treeCounts --;
					i--;
				}
			}


			float C = 10000;
			while (treeCounts > 1)	 {
				dgHACDConvacityLookAheadTree* const leftTree = m_concavityTreeArray[treeCounts-1];
				dgHACDConvacityLookAheadTree* const rightTree = m_concavityTreeArray[treeCounts-2];
				m_concavityTreeArray[treeCounts-1] = NULL;
				m_concavityTreeArray[treeCounts-2] = HACD_NEW(dgHACDConvacityLookAheadTree)(leftTree, rightTree, C);
				C *= 2;
				treeCounts --;
			}

		}

		dgHACDConvacityLookAheadTree* const tree = m_concavityTreeArray[0];
		dgDownHeap<dgHACDConvacityLookAheadTree*, double> approximation(maxClustesCount * 2);

		tree->ReduceByCount (maxClustesCount, approximation);
		//		tree->ReduceByConcavity (maxConcavity, approximation);

		while (approximation.GetCount()) {
			m_convexProximation.Append(approximation[0]);
			approximation.Pop();
		}
	}
#else 
	void CollapseClusters (dgMeshEffect& mesh, double maxConcavity, int32_t maxClustesCount)
	{
		maxConcavity *= (m_diagonal * DG_CONCAVITY_SCALE);

		bool terminate = false;
		while (m_priorityHeap.GetCount() && !terminate) {
			double concavity =  m_priorityHeap.Value();
			dgList<dgPairProxy>::dgListNode* const pairNode = m_priorityHeap[0];
			if ((concavity < maxConcavity) && (GetCount() < maxClustesCount)) {
				terminate  = true;
			} else {
				m_priorityHeap.Pop();
				CollapseEdge (pairNode, mesh, concavity);
			}
		}
	}
#endif

	int32_t m_mark;
	int32_t m_faceCount;
	int32_t m_vertexMark;
	int32_t m_progress;
	int32_t m_cancavityTreeIndex;
	int32_t* m_vertexMarks;
	float m_invFaceCount;
	double m_diagonal;
	dgBigVector* m_vertexPool;
	dgList<dgPairProxy> m_proxyList;
	dgHACDConvacityLookAheadTree** m_concavityTreeArray;	
	dgList<dgHACDConvacityLookAheadTree*> m_convexProximation;
	dgUpHeap<dgList<dgPairProxy>::dgListNode*, double> m_priorityHeap;
	hacd::ICallback* m_reportProgressCallback;
};


dgMeshEffect* dgMeshEffect::CreateConvexApproximation(
	float maxConcavity, 
	float backFaceDistanceFactor, 
	int32_t maxHullsCount, 
	int32_t maxVertexPerHull,
	hacd::ICallback* reportProgressCallback) const
{
	//	dgMeshEffect triangleMesh(*this);
	if (maxHullsCount <= 1) 
	{
		maxHullsCount = 1;
	}
	if (maxConcavity <= float (1.0e-5f)) 
	{
		maxConcavity = float (1.0e-5f);
	}

	if (maxVertexPerHull < 4) 
	{
		maxVertexPerHull = 4;
	}
	ClampValue(backFaceDistanceFactor, float (0.01f), float (1.0f));

	if ( reportProgressCallback )
	{
		reportProgressCallback->ReportProgress("Making a copy of the input mesh",0);
	}
	// make a copy of the mesh
	dgMeshEffect mesh(*this);
	mesh.ClearAttributeArray();

	// create a general connectivity graph    
	if ( reportProgressCallback )
	{
		reportProgressCallback->ReportProgress("Creating Connectivity Graph",0);
	}
	// make a copy of the mesh
	dgHACDClusterGraph graph (mesh, backFaceDistanceFactor, reportProgressCallback);

	if ( reportProgressCallback )
	{
		reportProgressCallback->ReportProgress("Submit Initial Edge Costs",0);
	}

	// calculate initial edge costs
	graph.SubmitInitialEdgeCosts(mesh);

	// collapse the graph
	if ( reportProgressCallback )
	{
		reportProgressCallback->ReportProgress("Collapse the Graph",0);
	}

	if ( reportProgressCallback )
	{
		reportProgressCallback->ReportProgress("Collapse Clusters",0);
	}

	graph.CollapseClusters (mesh, maxConcavity, maxHullsCount);

	if ( reportProgressCallback )
	{
		reportProgressCallback->ReportProgress("Creating Partition Mesh",0);
	}

	// Create Partition Mesh
	return graph.CreatePatitionMesh (mesh, maxVertexPerHull);
}


void dgMeshEffect::ClearAttributeArray ()
{
	dgStack<dgVertexAtribute>attribArray (m_pointCount);

	memset (&attribArray[0], 0, m_pointCount * sizeof (dgVertexAtribute));
	int32_t mark = IncLRU();
	dgPolyhedra::Iterator iter (*this);	
	for(iter.Begin(); iter; iter ++)
	{
		dgEdge* const edge = &(*iter);
		if (edge->m_mark < mark)
		{
			dgEdge* ptr = edge;

			int32_t index = ptr->m_incidentVertex;
			dgVertexAtribute& attrib = attribArray[index];
			attrib.m_vertex = m_points[index];
			do 
			{
				ptr->m_mark = mark;
				ptr->m_userData = (uint64_t)index;
				ptr = ptr->m_twin->m_next;
			} while (ptr !=  edge);
		}
	}
	ApplyAttributeArray (&attribArray[0], m_pointCount);
}


//*******************************************
// Original 'fast' implementation from Julio
//*******************************************

class dgClusterFace
{
public:
	dgClusterFace()
	{
	}
	~dgClusterFace()
	{
	}

	dgEdge* m_edge;
	double m_area;
	double m_perimeter;
	dgBigVector m_normal;
};

class dgPairProxi
{
public:
	dgPairProxi()
		:m_edgeA(NULL)
		,m_edgeB(NULL)
		,m_area(double(0.0f))
		,m_perimeter(double(0.0f))
	{
	}

	~dgPairProxi()
	{
	}

	dgEdge* m_edgeA;
	dgEdge* m_edgeB;
	double m_area;
	double m_perimeter;
};

class dgClusterList: public dgList<dgClusterFace>
{
public:
	dgClusterList() 
		: dgList<dgClusterFace>()
		,m_area (float (0.0f))
		,m_perimeter (float (0.0f))
	{
	}

	~dgClusterList()
	{
	}

	int32_t AddVertexToPool(const dgMeshEffect& mesh, dgBigVector* const vertexPool, int32_t* const vertexMarks, int32_t vertexMark)
	{
		int32_t count = 0;

		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();
		for (dgListNode* node = GetFirst(); node; node = node->GetNext()) {
			dgClusterFace& face = node->GetInfo();

			dgEdge* edge = face.m_edge;
			do {
				int32_t index = edge->m_incidentVertex;
				if (vertexMarks[index] != vertexMark)
				{
					vertexMarks[index] = vertexMark;
					vertexPool[count] = points[index];
					count++;
				}
				edge = edge->m_next;
			} while (edge != face.m_edge);
		}
		return count;
	}

	double CalculateTriangleConcavity2(const dgConvexHull3d& convexHull, dgClusterFace& info, int32_t i0, int32_t i1, int32_t i2, const dgBigVector* const points) const
	{
		uint32_t head = 1;
		uint32_t tail = 0;
		dgBigVector pool[1<<8][3];

		pool[0][0] = points[i0];
		pool[0][1] = points[i1];
		pool[0][2] = points[i2];

		const dgBigVector step(info.m_normal.Scale(double(4.0f) * convexHull.GetDiagonal()));

		double concavity = float(0.0f);
		double minArea = float(0.125f);
		double minArea2 = minArea * minArea * 0.5f;

		// weight the area by the area of the face 
		//dgBigVector edge10(pool[0][1] - pool[0][0]);
		//dgBigVector edge20(pool[0][2] - pool[0][0]);
		//dgBigVector triangleArea = edge10 * edge20;
		//double triangleArea2 = triangleArea % triangleArea;
		//if ((triangleArea2 / minArea2)> float (64.0f)) {
		// minArea2 = triangleArea2 / float (64.0f);
		//}

		int32_t maxCount = 4;
		uint32_t mask = (sizeof (pool) / (3 * sizeof (pool[0][0]))) - 1;
		while ((tail != head) && (maxCount >= 0)) {
			//stack--;
			maxCount --;
			dgBigVector p0(pool[tail][0]);
			dgBigVector p1(pool[tail][1]);
			dgBigVector p2(pool[tail][2]);
			tail =  (tail + 1) & mask;

			dgBigVector q1((p0 + p1 + p2).Scale(double(1.0f / 3.0f)));
			dgBigVector q0(q1 + step);

			double param = convexHull.RayCast(q0, q1);
			if (param > double(1.0f)) {
				param = double(1.0f);
			}
			dgBigVector dq(step.Scale(float(1.0f) - param));
			double lenght2 = dq % dq;
			if (lenght2 > concavity) {
				concavity = lenght2;
			}

			if (((head + 1) & mask) != tail) {
				dgBigVector edge10(p1 - p0);
				dgBigVector edge20(p2 - p0);
				dgBigVector n(edge10 * edge20);
				double area2 = n % n;
				if (area2 > minArea2) {
					dgBigVector p01((p0 + p1).Scale(double(0.5f)));
					dgBigVector p12((p1 + p2).Scale(double(0.5f)));
					dgBigVector p20((p2 + p0).Scale(double(0.5f)));

					pool[head][0] = p0;
					pool[head][1] = p01;
					pool[head][2] = p20;
					head = (head + 1) & mask;

					if (((head + 1) & mask) != tail) {
						pool[head][0] = p1;
						pool[head][1] = p12;
						pool[head][2] = p01;
						head = (head + 1) & mask;

						if (((head + 1) & mask) != tail)	{
							pool[head][0] = p2;
							pool[head][1] = p20;
							pool[head][2] = p12;
							head = (head + 1) & mask;
						}
					}
				}
			}
		}
		return concavity;
	}

	double CalculateConcavity2(const dgConvexHull3d& convexHull, const dgMeshEffect& mesh)
	{
		double concavity = float(0.0f);

		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();

		for (dgListNode* node = GetFirst(); node; node = node->GetNext()) {
			dgClusterFace& info = node->GetInfo();
			int32_t i0 = info.m_edge->m_incidentVertex;
			int32_t i1 = info.m_edge->m_next->m_incidentVertex;
			for (dgEdge* edge = info.m_edge->m_next->m_next; edge != info.m_edge; edge = edge->m_next) {
				int32_t i2 = edge->m_incidentVertex;
				double val = CalculateTriangleConcavity2(convexHull, info, i0, i1, i2, points);
				if (val > concavity) {
					concavity = val;
				}
				i1 = i2;
			}
		}

		return concavity;
	}

	bool IsClusterCoplanar(const dgBigPlane& plane,
		const dgMeshEffect& mesh) const
	{
		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();
		for (dgListNode* node = GetFirst(); node; node = node->GetNext()) {
			dgClusterFace& info = node->GetInfo();

			dgEdge* ptr = info.m_edge;
			do {
				const dgBigVector& p = points[ptr->m_incidentVertex];
				double dist = fabs(plane.Evalue(p));
				if (dist > double(1.0e-5f)) {
					return false;
				}
				ptr = ptr->m_next;
			} while (ptr != info.m_edge);
		}

		return true;
	}

	bool IsEdgeConvex(const dgBigPlane& plane, const dgMeshEffect& mesh,
		dgEdge* const edge) const
	{
		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();
		dgEdge* const edge0 = edge->m_next;
		dgEdge* ptr = edge0->m_twin->m_next;
		do {
			if (ptr->m_twin->m_incidentFace == edge->m_twin->m_incidentFace) {
				HACD_ASSERT(edge0->m_incidentVertex == ptr->m_incidentVertex);
				dgBigVector e0(points[edge0->m_twin->m_incidentVertex] - points[edge0->m_incidentVertex]);
				dgBigVector e1(points[ptr->m_twin->m_incidentVertex] - points[edge0->m_incidentVertex]);
				dgBigVector normal(e0 * e1);
				return (normal % plane) > double(0.0f);
			}
			ptr = ptr->m_twin->m_next;
		} while (ptr != edge->m_twin);

		HACD_ASSERT(0);
		return true;
	}

	// calculate the convex hull of a conched group of faces,
	// and measure the concavity, according to Khaled convexity criteria, which is basically
	// has two components, 
	// the first is ratio  between the the perimeter of the group of faces
	// and the second the largest distance from any of the face to the surface of the hull 
	// when the faces are are a strip of a convex hull the perimeter ratio components is 1.0 and the distance to the hull is zero
	// this is the ideal concavity.
	// when the face are no part of the hull, then the worse distance to the hull is dominate the the metric
	// this matrix is used to place all possible combination of this cluster with any adjacent cluster into a priority heap and determine 
	// which pair of two adjacent cluster is the best selection for combining the into a larger cluster.
	void CalculateNodeCost(dgMeshEffect& mesh, int32_t meshMask,
		dgBigVector* const vertexPool, int32_t* const vertexMarks,
		int32_t& vertexMark, dgClusterList* const clusters, double diagonalInv,
		double aspectRatioCoeficent, dgList<dgPairProxi>& proxyList,
		dgUpHeap<dgList<dgPairProxi>::dgListNode*, double>& heap)
	{
		int32_t faceIndex = GetFirst()->GetInfo().m_edge->m_incidentFace;

		const dgBigVector* const points = (dgBigVector*) mesh.GetVertexPool();

		bool flatStrip = true;
		dgBigPlane plane(GetFirst()->GetInfo().m_normal, -(points[GetFirst()->GetInfo().m_edge->m_incidentVertex] % GetFirst()->GetInfo().m_normal));
		if (GetCount() > 1) {
			flatStrip = IsClusterCoplanar(plane, mesh);
		}

		vertexMark++;
		int32_t vertexCount = AddVertexToPool(mesh, vertexPool, vertexMarks, vertexMark);
		for (dgListNode* node = GetFirst(); node; node = node->GetNext()) {
			//dgClusterFace& clusterFaceA = GetFirst()->GetInfo();
			dgClusterFace& clusterFaceA = node->GetInfo();

			dgEdge* edge = clusterFaceA.m_edge;
			do {
				int32_t twinFaceIndex = edge->m_twin->m_incidentFace;
				if ((edge->m_mark != meshMask) && (twinFaceIndex != faceIndex) && (twinFaceIndex > 0)) {

					dgClusterList& clusterListB = clusters[twinFaceIndex];

					vertexMark++;
					int32_t extraCount = clusterListB.AddVertexToPool(mesh, &vertexPool[vertexCount], &vertexMarks[0], vertexMark);

					int32_t count = vertexCount + extraCount;
					dgConvexHull3d convexHull(&vertexPool[0].m_x, sizeof(dgBigVector), count, 0.0);

					double concavity = double(0.0f);
					if (convexHull.GetVertexCount()) {
						concavity = sqrt(GetMax(CalculateConcavity2(convexHull, mesh), clusterListB.CalculateConcavity2(convexHull, mesh)));
						if (concavity < double(1.0e-3f)) {
							concavity = double(0.0f);
						}
					}

					if ((concavity == double(0.0f)) && flatStrip)  {
						if (clusterListB.IsClusterCoplanar(plane, mesh)) {
							bool concaveEdge = !(IsEdgeConvex(plane, mesh, edge) && IsEdgeConvex(plane, mesh, edge->m_twin));
							if (concaveEdge) {
								concavity += 1000.0f;
							}
						}
					}

					dgBigVector p1p0(points[edge->m_twin->m_incidentVertex] - points[edge->m_incidentVertex]);
					double edgeLength = double(2.0f) * sqrt(p1p0 % p1p0);

					double area = m_area + clusterListB.m_area;
					double perimeter = m_perimeter + clusterListB.m_perimeter - edgeLength;
					double edgeCost = perimeter * perimeter / (double(4.0f * 3.141592f) * area);
					double cost = diagonalInv * (concavity + edgeCost * aspectRatioCoeficent);

					if ((heap.GetCount() + 20) > heap.GetMaxCount()) {
						for (int32_t i = heap.GetCount() - 1; i >= 0; i--) {
							dgList<dgPairProxi>::dgListNode* emptyNode = heap[i];
							dgPairProxi& emptyPair = emptyNode->GetInfo();
							if ((emptyPair.m_edgeA == NULL) && (emptyPair.m_edgeB == NULL)) {
								heap.Remove(i);
							}
						}
					}

					dgList<dgPairProxi>::dgListNode* pairNode = proxyList.Append();
					dgPairProxi& pair = pairNode->GetInfo();
					pair.m_edgeA = edge;
					pair.m_edgeB = edge->m_twin;
					pair.m_area = area;
					pair.m_perimeter = perimeter;
					edge->m_userData = PTR_TO_UINT64(pairNode);
					edge->m_twin->m_userData = PTR_TO_UINT64(pairNode);
					heap.Push(pairNode, cost);
				}

				edge->m_mark = meshMask;
				edge->m_twin->m_mark = meshMask;
				edge = edge->m_next;
			} while (edge != clusterFaceA.m_edge);
		}
	}


	double m_area;
	double m_perimeter;
};

dgMeshEffect::dgMeshEffect(const dgMeshEffect& source, float absoluteconcavity, int32_t maxCount,hacd::ICallback *callback,bool /*legacyVersion*/) 
	:dgPolyhedra()
{
	if ( callback )
	{
		callback->ReportProgress("Initializing",0);
	}
	Init(true);

	dgMeshEffect mesh(source);
	int32_t faceCount = mesh.GetTotalFaceCount() + 1;
	dgStack<dgClusterList> clusterPool(faceCount);
	dgClusterList* const clusters = &clusterPool[0];

	for (int32_t i = 0; i < faceCount; i++) 
	{
		clusters[i] = dgClusterList();
	}

	int32_t meshMask = mesh.IncLRU();
	const dgBigVector* const points = mesh.m_points;

	// enumerate all faces, and initialize cluster pool
	dgMeshEffect::Iterator iter(mesh);

	int32_t clusterIndex = 1;
	for (iter.Begin(); iter; iter++) 
	{
		dgEdge* const edge = &(*iter);
		edge->m_userData = uint64_t (-1);
		if ((edge->m_mark != meshMask) && (edge->m_incidentFace > 0)) 
		{
			double perimeter = double(0.0f);
			dgEdge* ptr = edge;
			do 
			{
				dgBigVector p1p0(points[ptr->m_incidentVertex] - points[ptr->m_prev->m_incidentVertex]);
				perimeter += sqrt(p1p0 % p1p0);
				ptr->m_incidentFace = clusterIndex;

				ptr->m_mark = meshMask;
				ptr = ptr->m_next;
			} while (ptr != edge);

			dgBigVector normal = mesh.FaceNormal(edge, &points[0][0], sizeof(dgBigVector));
			double mag = sqrt(normal % normal);

			dgClusterFace& faceInfo = clusters[clusterIndex].Append()->GetInfo();

			faceInfo.m_edge = edge;
			faceInfo.m_perimeter = perimeter;
			faceInfo.m_area = double(0.5f) * mag;
			faceInfo.m_normal = normal.Scale(double(1.0f) / mag);

			clusters[clusterIndex].m_perimeter = perimeter;
			clusters[clusterIndex].m_area = faceInfo.m_area;

			clusterIndex++;
		}
	}

	HACD_ASSERT(faceCount == clusterIndex);

	// recalculate all edge cost
	dgStack<int32_t> vertexMarksArray(mesh.GetVertexCount());
	dgStack<dgBigVector> vertexArray(mesh.GetVertexCount() * 2);
	
	dgBigVector* const vertexPool = &vertexArray[0];
	int32_t* const vertexMarks = &vertexMarksArray[0];
	memset(&vertexMarks[0], 0, (size_t)vertexMarksArray.GetSizeInBytes());

	dgList<dgPairProxi> proxyList;
	dgUpHeap<dgList<dgPairProxi>::dgListNode*, double> heap(mesh.GetCount() + 1000);

	int32_t vertexMark = 0;

	double diagonalInv = float(1.0f);
	double aspectRatioCoeficent = absoluteconcavity / float(10.0f);
	meshMask = mesh.IncLRU();

	// calculate all the initial cost of all clusters, which at this time are all a single faces
	for (int32_t faceIndex = 1; faceIndex < faceCount; faceIndex++) 
	{
		vertexMark++;
		dgClusterList& clusterList = clusters[faceIndex];
		HACD_ASSERT(clusterList.GetFirst()->GetInfo().m_edge->m_incidentFace == faceIndex);
		clusterList.CalculateNodeCost(mesh, meshMask, &vertexPool[0], &vertexMarks[0], vertexMark, &clusters[0], diagonalInv, aspectRatioCoeficent, proxyList, heap);
	}
	
	if ( callback )
	{
		callback->ReportProgress("Calculating Convex Clusters",0);
	}


	
	// calculate all essential convex clusters by merging the all possible clusters according 
	// which combined concavity es lower that the max absolute concavity 
	// select the pair with the smaller concavity and fuse then into a larger cluster
	int32_t essencialClustersCount = faceCount - 1;
	while (heap.GetCount() && ((heap.Value() < absoluteconcavity) || (essencialClustersCount > maxCount))) 
	{
		dgList<dgPairProxi>::dgListNode* const pairNode = heap[0];
		heap.Pop();
		dgPairProxi& pair = pairNode->GetInfo();

		HACD_ASSERT((pair.m_edgeA && pair.m_edgeB) || (!pair.m_edgeA && !pair.m_edgeB));
		if (pair.m_edgeA && pair.m_edgeB) 
		{

			HACD_ASSERT(pair.m_edgeA->m_incidentFace != pair.m_edgeB->m_incidentFace);

			// merge two clusters
			int32_t faceIndexA = pair.m_edgeA->m_incidentFace;
			int32_t faceIndexB = pair.m_edgeB->m_incidentFace;
			dgClusterList* listA = &clusters[faceIndexA];
			dgClusterList* listB = &clusters[faceIndexB];
			if (pair.m_edgeA->m_incidentFace > pair.m_edgeB->m_incidentFace) 
			{
				Swap(faceIndexA, faceIndexB);
				Swap(listA, listB);
			}
			
			while (listB->GetFirst()) 
			{
				dgClusterList::dgListNode* const nodeB = listB->GetFirst();
				listB->Unlink(nodeB);
				dgClusterFace& faceB = nodeB->GetInfo();

				dgEdge* ptr = faceB.m_edge;
				do {
					ptr->m_incidentFace = faceIndexA;
					ptr = ptr->m_next;
				} while (ptr != faceB.m_edge);
				listA->Append(nodeB);
			}
			essencialClustersCount --;

			listB->m_area = float (0.0f);
			listB->m_perimeter = float (0.0f);
			listA->m_area = pair.m_area;
			listA->m_perimeter = pair.m_perimeter;

			// recalculated the new metric for the new cluster, and tag the used cluster as invalid, so that 
			// other potential selection do not try merge with this this one, producing convex that re use a face more than once  
			int32_t mark = mesh.IncLRU();
			for (dgClusterList::dgListNode* node = listA->GetFirst(); node; node = node->GetNext()) {
				dgClusterFace& face = node->GetInfo();
				dgEdge* ptr = face.m_edge;
				do {
					if (ptr->m_userData != uint64_t (-1)) {
						dgList<dgPairProxi>::dgListNode* const pairNode = (dgList<dgPairProxi>::dgListNode*) ptr->m_userData;
						dgPairProxi& pairProxy = pairNode->GetInfo();
						pairProxy.m_edgeA = NULL;
						pairProxy.m_edgeB = NULL;
					}
					ptr->m_userData = uint64_t (-1);
					ptr->m_twin->m_userData = uint64_t (-1);

					if ((ptr->m_twin->m_incidentFace == faceIndexA) || (ptr->m_twin->m_incidentFace < 0)) {
						ptr->m_mark = mark;
						ptr->m_twin->m_mark = mark;
					}

					if (ptr->m_mark != mark) {
						dgClusterList& adjacentList = clusters[ptr->m_twin->m_incidentFace];
						for (dgClusterList::dgListNode* adjacentNode = adjacentList.GetFirst(); adjacentNode; adjacentNode = adjacentNode->GetNext()) {
							dgClusterFace& adjacentFace = adjacentNode->GetInfo();
							dgEdge* adjacentEdge = adjacentFace.m_edge;
							do {
								if (adjacentEdge->m_twin->m_incidentFace == faceIndexA)	{
									adjacentEdge->m_twin->m_mark = mark;
								}
								adjacentEdge = adjacentEdge->m_next;
							} while (adjacentEdge != adjacentFace.m_edge);
						}
						ptr->m_mark = mark - 1;
					}
					ptr = ptr->m_next;
				} while (ptr != face.m_edge);
			}

			// re generated the cost of merging this new all its adjacent clusters, that are still alive. 
			vertexMark++;
			listA->CalculateNodeCost(mesh, mark, &vertexPool[0], &vertexMarks[0], vertexMark, &clusters[0], diagonalInv, aspectRatioCoeficent, proxyList, heap);
		}

		proxyList.Remove(pairNode);
	}

	if ( callback )
	{
		callback->ReportProgress("Computing Concavity",0);
	}



	BeginPolygon();
	float layer = float(0.0f);

	dgVertexAtribute polygon[256];
	memset(polygon, 0, sizeof(polygon));
	dgArray<dgBigVector> convexVertexBuffer(1024);
	for (int32_t i = 0; i < faceCount; i++) {
		dgClusterList& clusterList = clusters[i];

		if (clusterList.GetCount())	{
			int32_t count = 0;
			for (dgClusterList::dgListNode* node = clusterList.GetFirst(); node; node = node->GetNext()) {
				dgClusterFace& face = node->GetInfo();
				dgEdge* edge = face.m_edge;

				dgEdge* sourceEdge = source.FindEdge(edge->m_incidentVertex, edge->m_twin->m_incidentVertex);
				do {
					int32_t index = edge->m_incidentVertex;
					convexVertexBuffer[count] = points[index];

					count++;
					sourceEdge = sourceEdge->m_next;
					edge = edge->m_next;
				} while (edge != face.m_edge);
			}

			dgConvexHull3d convexHull(&convexVertexBuffer[0].m_x, sizeof(dgBigVector), count, 0.0);

			if (convexHull.GetCount()) {
				const dgBigVector* const vertex = convexHull.GetVertexPool();
				for (dgConvexHull3d::dgListNode* node = convexHull.GetFirst(); node; node = node->GetNext()) {
					const dgConvexHull3DFace* const face = &node->GetInfo();

					int32_t i0 = face->m_index[0];
					int32_t i1 = face->m_index[1];
					int32_t i2 = face->m_index[2];

					polygon[0].m_vertex = vertex[i0];
					polygon[0].m_vertex.m_w = layer;

					polygon[1].m_vertex = vertex[i1];
					polygon[1].m_vertex.m_w = layer;

					polygon[2].m_vertex = vertex[i2];
					polygon[2].m_vertex.m_w = layer;

					AddPolygon(3, &polygon[0].m_vertex.m_x, sizeof(dgVertexAtribute), 0);
				}

				layer += float(1.0f);
				//break;
			}
		}
	}
	EndPolygon(1.0e-5f);

	for (int32_t i = 0; i < faceCount; i++) {
		clusters[i].RemoveAll();
	}
}


dgMeshEffect* dgMeshEffect::CreateConvexApproximationFast(float maxConcavity, int32_t maxCount,hacd::ICallback *callback) const
{
	dgMeshEffect triangleMesh(*this);
	if (maxCount <= 1) 
	{
		maxCount = 1;
	}
	if (maxConcavity <= float (1.0e-5f)) 
	{
		maxConcavity = float (1.0e-5f);
	}
	dgMeshEffect* const convexPartion = HACD_NEW(dgMeshEffect)(triangleMesh, maxConcavity, maxCount, callback, true );
	return convexPartion;
}
