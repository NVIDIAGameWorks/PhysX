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

/****************************************************************************
*
*  Visual C++ 6.0 created by: Julio Jerez
*
****************************************************************************/
#include "dgStack.h"
#include "dgMatrix.h"
#include "dgPolyhedra.h"
#include "dgPolygonSoupBuilder.h"

#define DG_POINTS_RUN (512 * 1024)

class dgPolySoupFilterAllocator: public dgPolyhedra
{
	public: 
	dgPolySoupFilterAllocator (void)
		:dgPolyhedra ()
	{
	}

	~dgPolySoupFilterAllocator ()
	{
	}

	int32_t AddFilterFace (uint32_t count, int32_t* const pool)
	{
		BeginFace();
		HACD_ASSERT (count);
		bool reduction = true;
		while (reduction && !AddFace (int32_t (count), pool)) {
			reduction = false;
			if (count >3) {
				for (uint32_t i = 0; i < count; i ++) {
					for (uint32_t j = i + 1; j < count; j ++) {
						if (pool[j] == pool[i]) {
							for (i = j; i < count - 1; i ++) {
								pool[i] =  pool[i + 1];
							}
							count --;
							i = count;
							reduction = true;
							break;
						}
					}
				}
			}
		}
		EndFace();

		HACD_ASSERT (reduction);
		return reduction ? int32_t (count) : 0;
	}
};


dgPolygonSoupDatabaseBuilder::dgPolygonSoupDatabaseBuilder (void)
	:m_faceVertexCount(), m_vertexIndex(), m_normalIndex(), m_vertexPoints(), m_normalPoints()
{
	m_run = DG_POINTS_RUN;
	m_faceCount = 0;
	m_indexCount = 0;
	m_vertexCount = 0;
	m_normalCount = 0;
}


dgPolygonSoupDatabaseBuilder::~dgPolygonSoupDatabaseBuilder ()
{
}


void dgPolygonSoupDatabaseBuilder::Begin()
{
	m_run = DG_POINTS_RUN;
	m_faceCount = 0;
	m_indexCount = 0;
	m_vertexCount = 0;
	m_normalCount = 0;
}


void dgPolygonSoupDatabaseBuilder::AddMesh (const float* const vertex, int32_t vertexCount, int32_t strideInBytes, int32_t faceCount,	
	const int32_t* const faceArray, const int32_t* const indexArray, const int32_t* const faceTagsData, const dgMatrix& worldMatrix) 
{
	int32_t faces[256];
	int32_t pool[2048];


	m_vertexPoints[m_vertexCount + vertexCount].m_x = double (0.0f);
	dgBigVector* const vertexPool = &m_vertexPoints[m_vertexCount];

	worldMatrix.TransformTriplex (&vertexPool[0].m_x, sizeof (dgBigVector), vertex, strideInBytes, vertexCount);
	for (int32_t i = 0; i < vertexCount; i ++) {
		vertexPool[i].m_w = double (0.0f);
	}

	int32_t totalIndexCount = faceCount;
	for (int32_t i = 0; i < faceCount; i ++) {
		totalIndexCount += faceArray[i];
	}

	m_vertexIndex[m_indexCount + totalIndexCount] = 0;
	m_faceVertexCount[m_faceCount + faceCount] = 0;

	int32_t k = 0;
	for (int32_t i = 0; i < faceCount; i ++) {
		int32_t count = faceArray[i];
		for (int32_t j = 0; j < count; j ++) {
			int32_t index = indexArray[k];
			pool[j] = index + m_vertexCount;
			k ++;
		}

		int32_t convexFaces = 0;
		if (count == 3) {
			convexFaces = 1;
			dgBigVector p0 (m_vertexPoints[pool[2]]);
			for (int32_t i = 0; i < 3; i ++) {
				dgBigVector p1 (m_vertexPoints[pool[i]]);
				dgBigVector edge (p1 - p0);
				double mag2 = edge % edge;
				if (mag2 < float (1.0e-6f)) {
					convexFaces = 0;
				}
				p0 = p1;
			}

			if (convexFaces) {
				dgBigVector edge0 (m_vertexPoints[pool[2]] - m_vertexPoints[pool[0]]);
				dgBigVector edge1 (m_vertexPoints[pool[1]] - m_vertexPoints[pool[0]]);
				dgBigVector normal (edge0 * edge1);
				double mag2 = normal % normal;
				if (mag2 < float (1.0e-8f)) {
					convexFaces = 0;
				}
			}

			if (convexFaces) {
				faces[0] = 3;
			}

		} else {
			convexFaces = AddConvexFace (count, pool, faces);
		}

		int32_t index = 0;
		for (int32_t k = 0; k < convexFaces; k ++) {
			int32_t count = faces[k];
			m_vertexIndex[m_indexCount] = faceTagsData[i];
			m_indexCount ++;
			for (int32_t j = 0; j < count; j ++) {
				m_vertexIndex[m_indexCount] = pool[index];
				index ++;
				m_indexCount ++;
			}
			m_faceVertexCount[m_faceCount] = count + 1;
			m_faceCount ++;
		}
	}
	m_vertexCount += vertexCount;
	m_run -= vertexCount;
	if (m_run <= 0) {
		PackArray();
	}
}

void dgPolygonSoupDatabaseBuilder::PackArray()
{
	dgStack<int32_t> indexMapPool (m_vertexCount);
	int32_t* const indexMap = &indexMapPool[0];
	m_vertexCount = dgVertexListToIndexList (&m_vertexPoints[0].m_x, sizeof (dgBigVector), 3, m_vertexCount, &indexMap[0], float (1.0e-6f));

	int32_t k = 0;
	for (int32_t i = 0; i < m_faceCount; i ++) {
		k ++;

		int32_t count = m_faceVertexCount[i];
		for (int32_t j = 1; j < count; j ++) {
			int32_t index = m_vertexIndex[k];
			index = indexMap[index];
			m_vertexIndex[k] = index;
			k ++;
		}
	}

	m_run = DG_POINTS_RUN;
}

void dgPolygonSoupDatabaseBuilder::SingleFaceFixup()
{
	if (m_faceCount == 1) {
		int32_t index = 0;
		int32_t count = m_faceVertexCount[0];
		for (int32_t j = 0; j < count; j ++) {
			m_vertexIndex[m_indexCount] = m_vertexIndex[index];
			index ++;
			m_indexCount ++;
		}
		m_faceVertexCount[m_faceCount] = count;
		m_faceCount ++;
	}
}

void dgPolygonSoupDatabaseBuilder::EndAndOptimize(bool optimize)
{
	if (m_faceCount) {
		dgStack<int32_t> indexMapPool (m_indexCount + m_vertexCount);

		int32_t* const indexMap = &indexMapPool[0];
		m_vertexCount = dgVertexListToIndexList (&m_vertexPoints[0].m_x, sizeof (dgBigVector), 3, m_vertexCount, &indexMap[0], float (1.0e-4f));

		int32_t k = 0;
		for (int32_t i = 0; i < m_faceCount; i ++) {
			k ++;
			int32_t count = m_faceVertexCount[i];
			for (int32_t j = 1; j < count; j ++) {
				int32_t index = m_vertexIndex[k];
				index = indexMap[index];
				m_vertexIndex[k] = index;
				k ++;
			}
		}

		OptimizeByIndividualFaces();
		if (optimize) {
			OptimizeByGroupID();
			OptimizeByIndividualFaces();
		}
	}
}


void dgPolygonSoupDatabaseBuilder::OptimizeByGroupID()
{
	dgTree<int, int> attribFilter;
	dgPolygonSoupDatabaseBuilder builder;
	dgPolygonSoupDatabaseBuilder builderAux;
	dgPolygonSoupDatabaseBuilder builderLeftOver;

	builder.Begin();
	int32_t polygonIndex = 0;
	for (int32_t i = 0; i < m_faceCount; i ++) {
		int32_t attribute = m_vertexIndex[polygonIndex];
		if (!attribFilter.Find(attribute)) {
			attribFilter.Insert (attribute, attribute);
			builder.OptimizeByGroupID (*this, i, polygonIndex, builderLeftOver); 
			for (int32_t j = 0; builderLeftOver.m_faceCount && (j < 64); j ++) {
				builderAux.m_faceVertexCount[builderLeftOver.m_faceCount] = 0;
				builderAux.m_vertexIndex[builderLeftOver.m_indexCount] = 0;
				builderAux.m_vertexPoints[builderLeftOver.m_vertexCount].m_x = float (0.0f);

				memcpy (&builderAux.m_faceVertexCount[0], &builderLeftOver.m_faceVertexCount[0], sizeof (int32_t) * builderLeftOver.m_faceCount);
				memcpy (&builderAux.m_vertexIndex[0], &builderLeftOver.m_vertexIndex[0], sizeof (int32_t) * builderLeftOver.m_indexCount);
				memcpy (&builderAux.m_vertexPoints[0], &builderLeftOver.m_vertexPoints[0], sizeof (dgBigVector) * builderLeftOver.m_vertexCount);

				builderAux.m_faceCount = builderLeftOver.m_faceCount;
				builderAux.m_indexCount = builderLeftOver.m_indexCount;
				builderAux.m_vertexCount =  builderLeftOver.m_vertexCount;

				int32_t prevFaceCount = builderLeftOver.m_faceCount;
				builderLeftOver.m_faceCount = 0;
				builderLeftOver.m_indexCount = 0;
				builderLeftOver.m_vertexCount = 0;
				
				builder.OptimizeByGroupID (builderAux, 0, 0, builderLeftOver); 
				if (prevFaceCount == builderLeftOver.m_faceCount) {
					break;
				}
			}
			HACD_ASSERT (builderLeftOver.m_faceCount == 0);
		}
		polygonIndex += m_faceVertexCount[i];
	}
//	builder.End();
	builder.Optimize(false);

	m_faceVertexCount[builder.m_faceCount] = 0;
	m_vertexIndex[builder.m_indexCount] = 0;
	m_vertexPoints[builder.m_vertexCount].m_x = float (0.0f);

	memcpy (&m_faceVertexCount[0], &builder.m_faceVertexCount[0], sizeof (int32_t) * builder.m_faceCount);
	memcpy (&m_vertexIndex[0], &builder.m_vertexIndex[0], sizeof (int32_t) * builder.m_indexCount);
	memcpy (&m_vertexPoints[0], &builder.m_vertexPoints[0], sizeof (dgBigVector) * builder.m_vertexCount);
	
	m_faceCount = builder.m_faceCount;
	m_indexCount = builder.m_indexCount;
	m_vertexCount = builder.m_vertexCount;
	m_normalCount = builder.m_normalCount;
}


void dgPolygonSoupDatabaseBuilder::OptimizeByGroupID (dgPolygonSoupDatabaseBuilder& source, int32_t faceNumber, int32_t faceIndexNumber, dgPolygonSoupDatabaseBuilder& leftOver) 
{
	int32_t indexPool[1024 * 1];
	int32_t atributeData[1024 * 1];
	dgVector vertexPool[1024 * 1];
	dgPolyhedra polyhedra;

	int32_t attribute = source.m_vertexIndex[faceIndexNumber];
	for (int32_t i = 0; i < int32_t (sizeof(atributeData) / sizeof (int32_t)); i ++) {
		indexPool[i] = i;
		atributeData[i] = attribute;
	}

	leftOver.Begin();
	polyhedra.BeginFace ();
	for (int32_t i = faceNumber; i < source.m_faceCount; i ++) {
		int32_t indexCount;
		indexCount = source.m_faceVertexCount[i];
		HACD_ASSERT (indexCount < 1024);

		if (source.m_vertexIndex[faceIndexNumber] == attribute) {
			dgEdge* const face = polyhedra.AddFace(indexCount - 1, &source.m_vertexIndex[faceIndexNumber + 1]);
			if (!face) {
				int32_t faceArray;
				for (int32_t j = 0; j < indexCount - 1; j ++) {
					int32_t index;
					index = source.m_vertexIndex[faceIndexNumber + j + 1];
					vertexPool[j] = source.m_vertexPoints[index];
				}
				faceArray = indexCount - 1;
				leftOver.AddMesh (&vertexPool[0].m_x, indexCount - 1, sizeof (dgVector), 1, &faceArray, indexPool, atributeData, dgGetIdentityMatrix());
			} else {
				// set the attribute
				dgEdge* ptr = face;
				do {
					ptr->m_userData = uint64_t (attribute);
					ptr = ptr->m_next;
				} while (ptr != face);
			}
		}
		faceIndexNumber += indexCount; 
	} 

	leftOver.Optimize(false);
	polyhedra.EndFace();


	dgPolyhedra facesLeft;
	facesLeft.BeginFace();
	polyhedra.ConvexPartition (&source.m_vertexPoints[0].m_x, sizeof (dgBigVector), &facesLeft);
	facesLeft.EndFace();


	int32_t mark = polyhedra.IncLRU();
	dgPolyhedra::Iterator iter (polyhedra);
	for (iter.Begin(); iter; iter ++) {
		dgEdge* const edge = &(*iter);
		if (edge->m_incidentFace < 0) {
			continue;
		}
		if (edge->m_mark == mark) {
			continue;
		}

		dgEdge* ptr = edge;
		int32_t indexCount = 0;
		do {
			ptr->m_mark = mark;
			vertexPool[indexCount] = source.m_vertexPoints[ptr->m_incidentVertex];
			indexCount ++;
			ptr = ptr->m_next;
 		} while (ptr != edge);

		if (indexCount >= 3) {
			AddMesh (&vertexPool[0].m_x, indexCount, sizeof (dgVector), 1, &indexCount, indexPool, atributeData, dgGetIdentityMatrix());
		}
	}


	mark = facesLeft.IncLRU();
	dgPolyhedra::Iterator iter1 (facesLeft);
	for (iter1.Begin(); iter1; iter1 ++) {
		dgEdge* const edge = &(*iter1);
		if (edge->m_incidentFace < 0) {
			continue;
		}
		if (edge->m_mark == mark) {
			continue;
		}

		dgEdge* ptr = edge;
		int32_t indexCount = 0;
		do {
			ptr->m_mark = mark;
			vertexPool[indexCount] = source.m_vertexPoints[ptr->m_incidentVertex];
			indexCount ++;
			ptr = ptr->m_next;
 		} while (ptr != edge);
		if (indexCount >= 3) {
			AddMesh (&vertexPool[0].m_x, indexCount, sizeof (dgVector), 1, &indexCount, indexPool, atributeData, dgGetIdentityMatrix());
		}
	}
}




void dgPolygonSoupDatabaseBuilder::OptimizeByIndividualFaces()
{
	int32_t* const faceArray = &m_faceVertexCount[0];
	int32_t* const indexArray = &m_vertexIndex[0];

	int32_t* const oldFaceArray = &m_faceVertexCount[0];
	int32_t* const oldIndexArray = &m_vertexIndex[0];

	int32_t polygonIndex = 0;
	int32_t newFaceCount = 0;
	int32_t newIndexCount = 0;
	for (int32_t i = 0; i < m_faceCount; i ++) {
		int32_t oldCount = oldFaceArray[i];
		int32_t count = FilterFace (oldCount - 1, &oldIndexArray[polygonIndex + 1]);
		if (count) {
			faceArray[newFaceCount] = count + 1;
			for (int32_t j = 0; j < count + 1; j ++) {
				indexArray[newIndexCount + j] = oldIndexArray[polygonIndex + j];
			}
			newFaceCount ++;
			newIndexCount += (count + 1);
		}
		polygonIndex += oldCount;
	}
	HACD_ASSERT (polygonIndex == m_indexCount);
	m_faceCount = newFaceCount;
	m_indexCount = newIndexCount;
}




void dgPolygonSoupDatabaseBuilder::End(bool optimize)
{
	Optimize(optimize);

	// build the normal array and adjacency array
	// calculate all face the normals
	int32_t indexCount = 0;
	m_normalPoints[m_faceCount].m_x = double (0.0f);
	for (int32_t i = 0; i < m_faceCount; i ++) {
		int32_t faceIndexCount = m_faceVertexCount[i];

		int32_t* const ptr = &m_vertexIndex[indexCount + 1];
		dgBigVector v0 (&m_vertexPoints[ptr[0]].m_x);
		dgBigVector v1 (&m_vertexPoints[ptr[1]].m_x);
		dgBigVector e0 (v1 - v0);
		dgBigVector normal (float (0.0f), float (0.0f), float (0.0f), float (0.0f));
		for (int32_t j = 2; j < faceIndexCount - 1; j ++) {
			dgBigVector v2 (&m_vertexPoints[ptr[j]].m_x);
			dgBigVector e1 (v2 - v0);
			normal += e0 * e1;
			e0 = e1;
		}
		normal = normal.Scale (dgRsqrt (normal % normal));

		m_normalPoints[i].m_x = normal.m_x;
		m_normalPoints[i].m_y = normal.m_y;
		m_normalPoints[i].m_z = normal.m_z;
		indexCount += faceIndexCount;
	}
	// compress normals array
	m_normalIndex[m_faceCount] = 0;
	m_normalCount = dgVertexListToIndexList(&m_normalPoints[0].m_x, sizeof (dgBigVector), 3, m_faceCount, &m_normalIndex[0], float (1.0e-4f));
}

void dgPolygonSoupDatabaseBuilder::Optimize(bool optimize)
{
	#define DG_PATITION_SIZE (1024 * 4)
	if (optimize && (m_faceCount > DG_PATITION_SIZE)) {

		dgBigVector median (float (0.0f), float (0.0f), float (0.0f), float (0.0f));
		dgBigVector varian (float (0.0f), float (0.0f), float (0.0f), float (0.0f));

		dgStack<dgVector> pool (1024 * 2);
		dgStack<int32_t> indexArray (1024 * 2);
		int32_t polygonIndex = 0;
		for (int32_t i = 0; i < m_faceCount; i ++) {

			dgBigVector p0 (float ( 1.0e10f), float ( 1.0e10f), float ( 1.0e10f), float (0.0f));
			dgBigVector p1 (float (-1.0e10f), float (-1.0e10f), float (-1.0e10f), float (0.0f));
			int32_t count = m_faceVertexCount[i];

			for (int32_t j = 1; j < count; j ++) {
				int32_t k = m_vertexIndex[polygonIndex + j];
				p0.m_x = GetMin (p0.m_x, double (m_vertexPoints[k].m_x));
				p0.m_y = GetMin (p0.m_y, double (m_vertexPoints[k].m_y));
				p0.m_z = GetMin (p0.m_z, double (m_vertexPoints[k].m_z));
				p1.m_x = GetMax (p1.m_x, double (m_vertexPoints[k].m_x));
				p1.m_y = GetMax (p1.m_y, double (m_vertexPoints[k].m_y));
				p1.m_z = GetMax (p1.m_z, double (m_vertexPoints[k].m_z));
			}

			dgBigVector p ((p0 + p1).Scale (0.5f));
			median += p;
			varian += p.CompProduct (p);
			polygonIndex += count;
		}

		varian = varian.Scale (float (m_faceCount)) - median.CompProduct(median);

		int32_t axis = 0;
		float maxVarian = float (-1.0e10f);
		for (int32_t i = 0; i < 3; i ++) {
			if (varian[i] > maxVarian) {
				axis = i;
				maxVarian = float (varian[i]);
			}
		}
		dgBigVector center = median.Scale (float (1.0f) / float (m_faceCount));
		double axisVal = center[axis];

		dgPolygonSoupDatabaseBuilder left;
		dgPolygonSoupDatabaseBuilder right;

		left.Begin();
		right.Begin();
		polygonIndex = 0;
		for (int32_t i = 0; i < m_faceCount; i ++) {
			int32_t side = 0;
			int32_t count = m_faceVertexCount[i];
			for (int32_t j = 1; j < count; j ++) {
				int32_t k;
				k = m_vertexIndex[polygonIndex + j];
				dgVector p (&m_vertexPoints[k].m_x);
				if (p[axis] > axisVal) {
					side = 1;
					break;
				}
			}

			int32_t faceArray = count - 1;
			int32_t faceTagsData = m_vertexIndex[polygonIndex];
			for (int32_t j = 1; j < count; j ++) {
				int32_t k = m_vertexIndex[polygonIndex + j];
				pool[j - 1] = m_vertexPoints[k];
				indexArray[j - 1] = j - 1;
			}

			if (!side) {
				left.AddMesh (&pool[0].m_x, count - 1, sizeof (dgVector), 1, &faceArray, &indexArray[0], &faceTagsData, dgGetIdentityMatrix()); 
			} else {
				right.AddMesh (&pool[0].m_x, count - 1, sizeof (dgVector), 1, &faceArray, &indexArray[0], &faceTagsData, dgGetIdentityMatrix()); 
			}
			polygonIndex += count;
		}

		left.Optimize(optimize);
		right.Optimize(optimize);

		m_faceCount = 0;
		m_indexCount = 0;
		m_vertexCount = 0;
		m_normalCount = 0;
		polygonIndex = 0;
		for (int32_t i = 0; i < left.m_faceCount; i ++) {
			int32_t count = left.m_faceVertexCount[i];
			int32_t faceArray = count - 1;
			int32_t faceTagsData = left.m_vertexIndex[polygonIndex];
			for (int32_t j = 1; j < count; j ++) {
				int32_t k = left.m_vertexIndex[polygonIndex + j];
				pool[j - 1] = left.m_vertexPoints[k];
				indexArray[j - 1] = j - 1;
			}
			AddMesh (&pool[0].m_x, count - 1, sizeof (dgVector), 1, &faceArray, &indexArray[0], &faceTagsData, dgGetIdentityMatrix()); 
			polygonIndex += count;
		}

		polygonIndex = 0;
		for (int32_t i = 0; i < right.m_faceCount; i ++) {
			int32_t count = right.m_faceVertexCount[i];
			int32_t faceArray = count - 1;
			int32_t faceTagsData = right.m_vertexIndex[polygonIndex];
			for (int32_t j = 1; j < count; j ++) {
				int32_t k = right.m_vertexIndex[polygonIndex + j];
				pool[j - 1] = right.m_vertexPoints[k];
				indexArray[j - 1] = j - 1;
			}
			AddMesh (&pool[0].m_x, count - 1, sizeof (dgVector), 1, &faceArray, &indexArray[0], &faceTagsData, dgGetIdentityMatrix()); 
			polygonIndex += count;
		}

		if (m_faceCount < DG_PATITION_SIZE) { 
			EndAndOptimize(optimize);
		} else {
			EndAndOptimize(false);
		}

	} else {
		EndAndOptimize(optimize);
	}
}



int32_t dgPolygonSoupDatabaseBuilder::FilterFace (int32_t count, int32_t* const pool)
{
	if (count == 3) {
		dgBigVector p0 (m_vertexPoints[pool[2]]);
		for (int32_t i = 0; i < 3; i ++) {
			dgBigVector p1 (m_vertexPoints[pool[i]]);
			dgBigVector edge (p1 - p0);
			double mag2 = edge % edge;
			if (mag2 < float (1.0e-6f)) {
				count = 0;
			}
			p0 = p1;
		}

		if (count == 3) {
			dgBigVector edge0 (m_vertexPoints[pool[2]] - m_vertexPoints[pool[0]]);
			dgBigVector edge1 (m_vertexPoints[pool[1]] - m_vertexPoints[pool[0]]);
			dgBigVector normal (edge0 * edge1);
			double mag2 = normal % normal;
			if (mag2 < float (1.0e-8f)) {
				count = 0;
			}
		}
	} else {
	dgPolySoupFilterAllocator polyhedra;

	count = polyhedra.AddFilterFace (uint32_t (count), pool);

	if (!count) {
		return 0;
	}

	dgEdge* edge = &polyhedra.GetRoot()->GetInfo();
	if (edge->m_incidentFace < 0) {
		edge = edge->m_twin;
	}

	bool flag = true;
	while (flag) {
		flag = false;
		if (count >= 3) {
			dgEdge* ptr = edge;

			dgBigVector p0 (&m_vertexPoints[ptr->m_incidentVertex].m_x);
			do {
				dgBigVector p1 (&m_vertexPoints[ptr->m_next->m_incidentVertex].m_x);
				dgBigVector e0 (p1 - p0);
				double mag2 = e0 % e0;
				if (mag2 < float (1.0e-6f)) {
					count --;
					flag = true;
					edge = ptr->m_next;
					ptr->m_prev->m_next = ptr->m_next;
					ptr->m_next->m_prev = ptr->m_prev;
					ptr->m_twin->m_next->m_prev = ptr->m_twin->m_prev;
					ptr->m_twin->m_prev->m_next = ptr->m_twin->m_next;
					break;
				}
				p0 = p1;
				ptr = ptr->m_next;
			} while (ptr != edge);
		}
	}
	if (count >= 3) {
		flag = true;
		dgBigVector normal (polyhedra.FaceNormal (edge, &m_vertexPoints[0].m_x, sizeof (dgBigVector)));

		HACD_ASSERT ((normal % normal) > float (1.0e-10f)); 
		normal = normal.Scale (dgRsqrt (normal % normal + float (1.0e-20f)));

		while (flag) {
			flag = false;
			if (count >= 3) {
				dgEdge* ptr = edge;

				dgBigVector p0 (&m_vertexPoints[ptr->m_prev->m_incidentVertex].m_x);
				dgBigVector p1 (&m_vertexPoints[ptr->m_incidentVertex].m_x);
				dgBigVector e0 (p1 - p0);
				e0 = e0.Scale (dgRsqrt (e0 % e0 + float(1.0e-10f)));
				do {
					dgBigVector p2 (&m_vertexPoints[ptr->m_next->m_incidentVertex].m_x);
					dgBigVector e1 (p2 - p1);

					e1 = e1.Scale (dgRsqrt (e1 % e1 + float(1.0e-10f)));
					double mag2 = e1 % e0;
					if (mag2 > float (0.9999f)) {
						count --;
						flag = true;
						edge = ptr->m_next;
						ptr->m_prev->m_next = ptr->m_next;
						ptr->m_next->m_prev = ptr->m_prev;
						ptr->m_twin->m_next->m_prev = ptr->m_twin->m_prev;
						ptr->m_twin->m_prev->m_next = ptr->m_twin->m_next;
						break;
					}

					dgBigVector n (e0 * e1);
					mag2 = n % normal;
					if (mag2 < float (1.0e-5f)) {
						count --;
						flag = true;
						edge = ptr->m_next;
						ptr->m_prev->m_next = ptr->m_next;
						ptr->m_next->m_prev = ptr->m_prev;
						ptr->m_twin->m_next->m_prev = ptr->m_twin->m_prev;
						ptr->m_twin->m_prev->m_next = ptr->m_twin->m_next;
						break;
					}

					e0 = e1;
					p1 = p2;
					ptr = ptr->m_next;
				} while (ptr != edge);
			}
		}
	}

	dgEdge* first = edge;
	if (count >= 3) {
		double best = float (2.0f);
		dgEdge* ptr = edge;

		dgBigVector p0 (&m_vertexPoints[ptr->m_incidentVertex].m_x);
		dgBigVector p1 (&m_vertexPoints[ptr->m_next->m_incidentVertex].m_x);
		dgBigVector e0 (p1 - p0);
		e0 = e0.Scale (dgRsqrt (e0 % e0 + float(1.0e-10f)));
		do {
			dgBigVector p2 (&m_vertexPoints[ptr->m_next->m_next->m_incidentVertex].m_x);
			dgBigVector e1 (p2 - p1);

			e1 = e1.Scale (dgRsqrt (e1 % e1 + float(1.0e-10f)));
			double mag2 = fabs (e1 % e0);
			if (mag2 < best) {
				best = mag2;
				first = ptr;
			}

			e0 = e1;
			p1 = p2;
			ptr = ptr->m_next;
		} while (ptr != edge);

		count = 0;
		ptr = first;
		do {
			pool[count] = ptr->m_incidentVertex;
			count ++;
			ptr = ptr->m_next;
		} while (ptr != first);
	}

#ifdef _DEBUG
	if (count >= 3) {
		int32_t j0 = count - 2;  
		int32_t j1 = count - 1;  
		dgBigVector normal (polyhedra.FaceNormal (edge, &m_vertexPoints[0].m_x, sizeof (dgBigVector)));
		for (int32_t j2 = 0; j2 < count; j2 ++) { 
			dgBigVector p0 (&m_vertexPoints[pool[j0]].m_x);
			dgBigVector p1 (&m_vertexPoints[pool[j1]].m_x);
			dgBigVector p2 (&m_vertexPoints[pool[j2]].m_x);
			dgBigVector e0 ((p0 - p1));
			dgBigVector e1 ((p2 - p1));

			dgBigVector n (e1 * e0);
			HACD_ASSERT ((n % normal) > float (0.0f));
			j0 = j1;
			j1 = j2;
		}
	}
#endif
	}

	return (count >= 3) ? count : 0;
}


int32_t dgPolygonSoupDatabaseBuilder::AddConvexFace (int32_t count, int32_t* const pool, int32_t* const facesArray)
{
	dgPolySoupFilterAllocator polyhedra;

	count = polyhedra.AddFilterFace(uint32_t (count), pool);

	dgEdge* edge = &polyhedra.GetRoot()->GetInfo();
	if (edge->m_incidentFace < 0) {
		edge = edge->m_twin;
	}

	
	int32_t isconvex = 1;
	int32_t facesCount = 0;

	int32_t flag = 1;
	while (flag) {
		flag = 0;
		if (count >= 3) {
			dgEdge* ptr = edge;

			dgBigVector p0 (&m_vertexPoints[ptr->m_incidentVertex].m_x);
			do {
				dgBigVector p1 (&m_vertexPoints[ptr->m_next->m_incidentVertex].m_x);
				dgBigVector e0 (p1 - p0);
				double mag2 = e0 % e0;
				if (mag2 < float (1.0e-6f)) {
					count --;
					flag = 1;
					edge = ptr->m_next;
					ptr->m_prev->m_next = ptr->m_next;
					ptr->m_next->m_prev = ptr->m_prev;
					ptr->m_twin->m_next->m_prev = ptr->m_twin->m_prev;
					ptr->m_twin->m_prev->m_next = ptr->m_twin->m_next;
					break;
				}
				p0 = p1;
				ptr = ptr->m_next;
			} while (ptr != edge);
		}
	}
	if (count >= 3) {
		flag = 1;

		while (flag) {
			flag = 0;
			if (count >= 3) {
				dgEdge* ptr = edge;

				dgBigVector p0 (&m_vertexPoints[ptr->m_prev->m_incidentVertex].m_x);
				dgBigVector p1 (&m_vertexPoints[ptr->m_incidentVertex].m_x);
				dgBigVector e0 (p1 - p0);
				e0 = e0.Scale (dgRsqrt (e0 % e0 + float(1.0e-10f)));
				do {
					dgBigVector p2 (&m_vertexPoints[ptr->m_next->m_incidentVertex].m_x);
					dgBigVector e1 (p2 - p1);

					e1 = e1.Scale (dgRsqrt (e1 % e1 + float(1.0e-10f)));
					double mag2 = e1 % e0;
					if (mag2 > float (0.9999f)) {
						count --;
						flag = 1;
						edge = ptr->m_next;
						ptr->m_prev->m_next = ptr->m_next;
						ptr->m_next->m_prev = ptr->m_prev;
						ptr->m_twin->m_next->m_prev = ptr->m_twin->m_prev;
						ptr->m_twin->m_prev->m_next = ptr->m_twin->m_next;
						break;
					}

					e0 = e1;
					p1 = p2;
					ptr = ptr->m_next;
				} while (ptr != edge);
			}
		}

		dgBigVector normal (polyhedra.FaceNormal (edge, &m_vertexPoints[0].m_x, sizeof (dgBigVector)));
		double mag2 = normal % normal;
		if (mag2 < float (1.0e-8f)) {
			return 0;
		}
		normal = normal.Scale (dgRsqrt (mag2));


		if (count >= 3) {
			dgEdge* ptr = edge;
			dgBigVector p0 (&m_vertexPoints[ptr->m_prev->m_incidentVertex].m_x);
			dgBigVector p1 (&m_vertexPoints[ptr->m_incidentVertex].m_x);
			dgBigVector e0 (p1 - p0);
			e0 = e0.Scale (dgRsqrt (e0 % e0 + float(1.0e-10f)));
			do {
				dgBigVector p2 (&m_vertexPoints[ptr->m_next->m_incidentVertex].m_x);
				dgBigVector e1 (p2 - p1);

				e1 = e1.Scale (dgRsqrt (e1 % e1 + float(1.0e-10f)));

				dgBigVector n (e0 * e1);
				double mag2 = n % normal;
				if (mag2 < float (1.0e-5f)) {
					isconvex = 0;
					break;
				}

				e0 = e1;
				p1 = p2;
				ptr = ptr->m_next;
			} while (ptr != edge);
		}
	}

	if (isconvex) {
		dgEdge* const first = edge;
		if (count >= 3) {
			count = 0;
			dgEdge* ptr = first;
			do {
				pool[count] = ptr->m_incidentVertex;
				count ++;
				ptr = ptr->m_next;
			} while (ptr != first);
			facesArray[facesCount] = count;
			facesCount = 1;
		}
	} else {
		dgPolyhedra leftOver;
		dgPolyhedra polyhedra2;
		dgEdge* ptr = edge;
		count = 0;
		do {
			pool[count] = ptr->m_incidentVertex;
			count ++;
			ptr = ptr->m_next;
		} while (ptr != edge);


		polyhedra2.BeginFace();
		polyhedra2.AddFace (count, pool);
		polyhedra2.EndFace();
		leftOver.BeginFace();
		polyhedra2.ConvexPartition (&m_vertexPoints[0].m_x, sizeof (dgTriplex), &leftOver);
		leftOver.EndFace();

		int32_t mark = polyhedra2.IncLRU();
		int32_t index = 0;
		dgPolyhedra::Iterator iter (polyhedra2);
		for (iter.Begin(); iter; iter ++) {
			dgEdge* const edge = &(*iter);
			if (edge->m_incidentFace < 0) {
				continue;
			}
			if (edge->m_mark == mark) {
				continue;
			}

			ptr = edge;
			count = 0;
			do {
				ptr->m_mark = mark;
				pool[index] = ptr->m_incidentVertex;
				index ++;
				count ++;
				ptr = ptr->m_next;
			} while (ptr != edge);

			facesArray[facesCount] = count;
			facesCount ++;
		}
	}

	return facesCount;
}
