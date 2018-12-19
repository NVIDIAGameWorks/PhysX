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

#include "dgStack.h"
#include "dgTree.h"
#include "dgGoogol.h"
#include "dgConvexHull3d.h"
#include "dgSmallDeterminant.h"

#define DG_VERTEX_CLUMP_SIZE_3D		8 

class dgAABBPointTree3d
{
	public:
#ifdef _DEBUG
	dgAABBPointTree3d()
	{
		static int32_t id = 0;
		m_id = id;
		id ++;
	}
	int32_t m_id;
#endif

	dgBigVector m_box[2];
	dgAABBPointTree3d* m_left;
	dgAABBPointTree3d* m_right;
	dgAABBPointTree3d* m_parent;
};

class dgHullVertex: public dgBigVector
{
	public:
	int32_t m_index;
};	

class dgAABBPointTree3dClump: public dgAABBPointTree3d
{
	public:
	int32_t m_count;
	int32_t m_indices[DG_VERTEX_CLUMP_SIZE_3D];
};


dgConvexHull3DFace::dgConvexHull3DFace()
{
	m_mark = 0; 
	m_twin[0] = NULL;
	m_twin[1] = NULL;
	m_twin[2] = NULL;
}

double dgConvexHull3DFace::Evalue (const dgBigVector* const pointArray, const dgBigVector& point) const
{
	const dgBigVector& p0 = pointArray[m_index[0]];
	const dgBigVector& p1 = pointArray[m_index[1]];
	const dgBigVector& p2 = pointArray[m_index[2]];

	double matrix[3][3];
	for (int32_t i = 0; i < 3; i ++) {
		matrix[0][i] = p2[i] - p0[i];
		matrix[1][i] = p1[i] - p0[i];
		matrix[2][i] = point[i] - p0[i];
	}

	double error;
	double det = Determinant3x3 (matrix, &error);
	double precision  = double (1.0f) / double (1<<24);
	double errbound = error * precision; 
	if (fabs(det) > errbound) {
		return det;
	}

	dgGoogol exactMatrix[3][3];
	for (int32_t i = 0; i < 3; i ++) {
		exactMatrix[0][i] = dgGoogol(p2[i]) - dgGoogol(p0[i]);
		exactMatrix[1][i] = dgGoogol(p1[i]) - dgGoogol(p0[i]);
		exactMatrix[2][i] = dgGoogol(point[i]) - dgGoogol(p0[i]);
	}

	dgGoogol exactDet (Determinant3x3(exactMatrix));
	det = exactDet.GetAproximateValue();
	return det;
}

dgBigPlane dgConvexHull3DFace::GetPlaneEquation (const dgBigVector* const pointArray) const
{
	const dgBigVector& p0 = pointArray[m_index[0]];
	const dgBigVector& p1 = pointArray[m_index[1]];
	const dgBigVector& p2 = pointArray[m_index[2]];
	dgBigPlane plane (p0, p1, p2);
	plane = plane.Scale (1.0f / sqrt (plane % plane));
	return plane;
}


dgConvexHull3d::dgConvexHull3d ()
	:dgList<dgConvexHull3DFace>()
	,m_count (0)
	,m_diag()
	,m_aabbP0(dgBigVector (double (0.0), double (0.0), double (0.0), double (0.0)))
	,m_aabbP1(dgBigVector (double (0.0), double (0.0), double (0.0), double (0.0)))
	,m_points(1024)
{
}

dgConvexHull3d::dgConvexHull3d(const dgConvexHull3d& source)
	:dgList<dgConvexHull3DFace>()
	,m_count (source.m_count)
	,m_diag(source.m_diag)
	,m_aabbP0 (source.m_aabbP0)
	,m_aabbP1 (source.m_aabbP1)
	,m_points(source.m_count) 
{
	m_points[m_count-1].m_w = double (0.0f);
	for (int i = 0; i < m_count; i ++) {
		m_points[i] = source.m_points[i];
	}
	dgTree<dgListNode*, dgListNode*> map;
	for(dgListNode* sourceNode = source.GetFirst(); sourceNode; sourceNode = sourceNode->GetNext() ) {
		dgListNode* const node = Append();
		map.Insert(node, sourceNode);
	}

	for(dgListNode* sourceNode = source.GetFirst(); sourceNode; sourceNode = sourceNode->GetNext() ) {
		dgListNode* const node = map.Find(sourceNode)->GetInfo();

		dgConvexHull3DFace& face = node->GetInfo();
		dgConvexHull3DFace& srcFace = sourceNode->GetInfo();

		face.m_mark = 0;
		for (int32_t i = 0; i < 3; i ++) {
			face.m_index[i] = srcFace.m_index[i];
			face.m_twin[i] = map.Find (srcFace.m_twin[i])->GetInfo();
		}
	}
}

dgConvexHull3d::dgConvexHull3d(const double* const vertexCloud, int32_t strideInBytes, int32_t count, double distTol, int32_t maxVertexCount)
	:m_count (0)
	,m_diag()
	,m_aabbP0 (dgBigVector (double (0.0), double (0.0), double (0.0), double (0.0)))
	,m_aabbP1 (dgBigVector (double (0.0), double (0.0), double (0.0), double (0.0)))
	,m_points(count) 
{
	BuildHull (vertexCloud, strideInBytes, count, distTol, maxVertexCount);
}

dgConvexHull3d::~dgConvexHull3d(void)
{
}

void dgConvexHull3d::BuildHull (const double* const vertexCloud, int32_t strideInBytes, int32_t count, double distTol, int32_t maxVertexCount)
{
	int32_t treeCount = count / (DG_VERTEX_CLUMP_SIZE_3D>>1); 
	if (treeCount < 4) {
		treeCount = 4;
	}
	treeCount *= 2;

	dgStack<dgHullVertex> points (count);
	dgStack<dgAABBPointTree3dClump> treePool (treeCount + 256);
	count = InitVertexArray(&points[0], vertexCloud, strideInBytes, count, &treePool[0], treePool.GetSizeInBytes());

	if (m_count >= 4) {
		CalculateConvexHull (&treePool[0], &points[0], count, distTol, maxVertexCount);
	}
}

int32_t dgConvexHull3d::ConvexCompareVertex(const dgHullVertex* const  A, const dgHullVertex* const B, void* const context)
{
	HACD_FORCE_PARAMETER_REFERENCE(context);
	for (int32_t i = 0; i < 3; i ++) {
		if ((*A)[i] < (*B)[i]) {
			return -1;
		} else if ((*A)[i] > (*B)[i]) {
			return 1;
		}
	}
	return 0;
}



dgAABBPointTree3d* dgConvexHull3d::BuildTree (dgAABBPointTree3d* const parent, dgHullVertex* const points, int32_t count, int32_t baseIndex, int8_t** memoryPool, int32_t& maxMemSize) const
{
	dgAABBPointTree3d* tree = NULL;

	HACD_ASSERT (count);
	dgBigVector minP ( float (1.0e15f),  float (1.0e15f),  float (1.0e15f), float (0.0f)); 
	dgBigVector maxP (-float (1.0e15f), -float (1.0e15f), -float (1.0e15f), float (0.0f)); 
	if (count <= DG_VERTEX_CLUMP_SIZE_3D) {

		dgAABBPointTree3dClump* const clump = new (*memoryPool) dgAABBPointTree3dClump;
		*memoryPool += sizeof (dgAABBPointTree3dClump);
		maxMemSize -= sizeof (dgAABBPointTree3dClump);
		HACD_ASSERT (maxMemSize >= 0);


		clump->m_count = count;
		for (int32_t i = 0; i < count; i ++) {
			clump->m_indices[i] = i + baseIndex;

			const dgBigVector& p = points[i];
			minP.m_x = GetMin (p.m_x, minP.m_x); 
			minP.m_y = GetMin (p.m_y, minP.m_y); 
			minP.m_z = GetMin (p.m_z, minP.m_z); 

			maxP.m_x = GetMax (p.m_x, maxP.m_x); 
			maxP.m_y = GetMax (p.m_y, maxP.m_y); 
			maxP.m_z = GetMax (p.m_z, maxP.m_z); 
		}

		clump->m_left = NULL;
		clump->m_right = NULL;
		tree = clump;

	} else {
		dgBigVector median (float (0.0f), float (0.0f), float (0.0f), float (0.0f));
		dgBigVector varian (float (0.0f), float (0.0f), float (0.0f), float (0.0f));
		for (int32_t i = 0; i < count; i ++) {

			const dgBigVector& p = points[i];
			minP.m_x = GetMin (p.m_x, minP.m_x); 
			minP.m_y = GetMin (p.m_y, minP.m_y); 
			minP.m_z = GetMin (p.m_z, minP.m_z); 

			maxP.m_x = GetMax (p.m_x, maxP.m_x); 
			maxP.m_y = GetMax (p.m_y, maxP.m_y); 
			maxP.m_z = GetMax (p.m_z, maxP.m_z); 

			median += p;
			varian += p.CompProduct (p);
		}

		varian = varian.Scale (float (count)) - median.CompProduct(median);
		int32_t index = 0;
		double maxVarian = double (-1.0e10f);
		for (int32_t i = 0; i < 3; i ++) {
			if (varian[i] > maxVarian) {
				index = i;
				maxVarian = varian[i];
			}
		}
		dgBigVector center = median.Scale (double (1.0f) / double (count));

		double test = center[index];

		int32_t i0 = 0;
		int32_t i1 = count - 1;
		do {    
			for (; i0 <= i1; i0 ++) {
				double val = points[i0][index];
				if (val > test) {
					break;
				}
			}

			for (; i1 >= i0; i1 --) {
				double val = points[i1][index];
				if (val < test) {
					break;
				}
			}

			if (i0 < i1)	{
				Swap(points[i0], points[i1]);
				i0++; 
				i1--;
			}
		} while (i0 <= i1);

		if (i0 == 0){
			i0 = count / 2;
		}
		if (i0 == (count - 1)){
			i0 = count / 2;
		}

		tree = new (*memoryPool) dgAABBPointTree3d;
		*memoryPool += sizeof (dgAABBPointTree3d);
		maxMemSize -= sizeof (dgAABBPointTree3d);
		HACD_ASSERT (maxMemSize >= 0);

		HACD_ASSERT (i0);
		HACD_ASSERT (count - i0);

		tree->m_left = BuildTree (tree, points, i0, baseIndex, memoryPool, maxMemSize);
		tree->m_right = BuildTree (tree, &points[i0], count - i0, i0 + baseIndex, memoryPool, maxMemSize);
	}

	HACD_ASSERT (tree);
	tree->m_parent = parent;
	tree->m_box[0] = minP - dgBigVector (double (1.0e-3f), double (1.0e-3f), double (1.0e-3f), double (1.0f));
	tree->m_box[1] = maxP + dgBigVector (double (1.0e-3f), double (1.0e-3f), double (1.0e-3f), double (1.0f));
	return tree;
}





int32_t dgConvexHull3d::InitVertexArray(dgHullVertex* const points, const double* const vertexCloud, int32_t strideInBytes, int32_t count, void* const memoryPool, int32_t maxMemSize)
{
	int32_t stride = int32_t (strideInBytes / sizeof (double));
	if (stride >= 4) {
		for (int32_t i = 0; i < count; i ++) {
			int32_t index = i * stride;
			dgBigVector& vertex = points[i];
			vertex = dgBigVector (vertexCloud[index], vertexCloud[index + 1], vertexCloud[index + 2], vertexCloud[index + 3]);
			HACD_ASSERT (dgCheckVector(vertex));
			points[i].m_index = 0;
		}
	} else {
		for (int32_t i = 0; i < count; i ++) {
			int32_t index = i * stride;
			dgBigVector& vertex = points[i];
			vertex = dgBigVector (vertexCloud[index], vertexCloud[index + 1], vertexCloud[index + 2], double (0.0f));
			HACD_ASSERT (dgCheckVector(vertex));
			points[i].m_index = 0;
		}
	}

	dgSort(points, count, ConvexCompareVertex);

	int32_t indexCount = 0;
	for (int i = 1; i < count; i ++) {
		for (; i < count; i ++) {
			if (ConvexCompareVertex (&points[indexCount], &points[i], NULL)) {
				indexCount ++;
				points[indexCount] = points[i];
				break;
			}
		}
	}
	count = indexCount + 1;
	if (count < 4) {
		m_count = 0;
		return count;
	}

	dgAABBPointTree3d* tree = BuildTree (NULL, points, count, 0, (int8_t**) &memoryPool, maxMemSize);

	m_aabbP0 = tree->m_box[0];
	m_aabbP1 = tree->m_box[1];

	dgBigVector boxSize (tree->m_box[1] - tree->m_box[0]);	
	m_diag = float (sqrt (boxSize % boxSize));

	dgStack<dgBigVector> normalArrayPool (256);
	dgBigVector* const normalArray = &normalArrayPool[0];
	int32_t normalCount = BuildNormalList (&normalArray[0]);

	int32_t index = SupportVertex (&tree, points, normalArray[0]);
	m_points[0] = points[index];
	points[index].m_index = 1;

	bool validTetrahedrum = false;
	dgBigVector e1 (double (0.0f), double (0.0f), double (0.0f), double (0.0f)) ;
	for (int32_t i = 1; i < normalCount; i ++) {
		int32_t index = SupportVertex (&tree, points, normalArray[i]);
		HACD_ASSERT (index >= 0);

		e1 = points[index] - m_points[0];
		double error2 = e1 % e1;
		if (error2 > (float (1.0e-4f) * m_diag * m_diag)) {
			m_points[1] = points[index];
			points[index].m_index = 1;
			validTetrahedrum = true;
			break;
		}
	}
	if (!validTetrahedrum) {
		m_count = 0;
		HACD_ASSERT (0);
		return count;
	}

	validTetrahedrum = false;
	dgBigVector e2(float (0.0f), float (0.0f), float (0.0f), float (0.0f));;
	dgBigVector normal (float (0.0f), float (0.0f), float (0.0f), float (0.0f));
	for (int32_t i = 2; i < normalCount; i ++) {
		int32_t index = SupportVertex (&tree, points, normalArray[i]);
		HACD_ASSERT (index >= 0);
		e2 = points[index] - m_points[0];
		normal = e1 * e2;
		double error2 = sqrt (normal % normal);
		if (error2 > (float (1.0e-4f) * m_diag * m_diag)) {
			m_points[2] = points[index];
			points[index].m_index = 1;
			validTetrahedrum = true;
			break;
		}
	}

	if (!validTetrahedrum) {
		m_count = 0;
		HACD_ASSERT (0);
		return count;
	}

	// find the largest possible tetrahedron
	validTetrahedrum = false;
	dgBigVector e3(float (0.0f), float (0.0f), float (0.0f), float (0.0f));

	index = SupportVertex (&tree, points, normal);
	e3 = points[index] - m_points[0];
	double error2 = normal % e3;
	if (fabs (error2) > (double (1.0e-6f) * m_diag * m_diag)) {
		// we found a valid tetrahedra, about and start build the hull by adding the rest of the points
		m_points[3] = points[index];
		points[index].m_index = 1;
		validTetrahedrum = true;
	}
	if (!validTetrahedrum) {
		dgVector n (normal.Scale(double (-1.0f)));
		int32_t index = SupportVertex (&tree, points, n);
		e3 = points[index] - m_points[0];
		double error2 = normal % e3;
		if (fabs (error2) > (double (1.0e-6f) * m_diag * m_diag)) {
			// we found a valid tetrahedra, about and start build the hull by adding the rest of the points
			m_points[3] = points[index];
			points[index].m_index = 1;
			validTetrahedrum = true;
		}
	}
	if (!validTetrahedrum) {
	for (int32_t i = 3; i < normalCount; i ++) {
		int32_t index = SupportVertex (&tree, points, normalArray[i]);
		HACD_ASSERT (index >= 0);

		//make sure the volume of the fist tetrahedral is no negative
		e3 = points[index] - m_points[0];
		double error2 = normal % e3;
		if (fabs (error2) > (double (1.0e-6f) * m_diag * m_diag)) {
			// we found a valid tetrahedra, about and start build the hull by adding the rest of the points
			m_points[3] = points[index];
			points[index].m_index = 1;
			validTetrahedrum = true;
			break;
		}
	}
	}
	if (!validTetrahedrum) {
		// the points do not form a convex hull
		m_count = 0;
		//HACD_ASSERT (0);
		return count;
	}

	m_count = 4;
	double volume = TetrahedrumVolume (m_points[0], m_points[1], m_points[2], m_points[3]);
	if (volume > double (0.0f)) {
		Swap(m_points[2], m_points[3]);
	}
	HACD_ASSERT (TetrahedrumVolume(m_points[0], m_points[1], m_points[2], m_points[3]) < double(0.0f));

	return count;
}

double dgConvexHull3d::TetrahedrumVolume (const dgBigVector& p0, const dgBigVector& p1, const dgBigVector& p2, const dgBigVector& p3) const
{
	dgBigVector p1p0 (p1 - p0);
	dgBigVector p2p0 (p2 - p0);
	dgBigVector p3p0 (p3 - p0);
	return (p1p0 * p2p0) % p3p0;
}


void dgConvexHull3d::TessellateTriangle (int32_t level, const dgVector& p0, const dgVector& p1, const dgVector& p2, int32_t& count, dgBigVector* const ouput, int32_t& start) const
{
	if (level) {
		HACD_ASSERT (dgAbsf (p0 % p0 - float (1.0f)) < float (1.0e-4f));
		HACD_ASSERT (dgAbsf (p1 % p1 - float (1.0f)) < float (1.0e-4f));
		HACD_ASSERT (dgAbsf (p2 % p2 - float (1.0f)) < float (1.0e-4f));
		dgVector p01 (p0 + p1);
		dgVector p12 (p1 + p2);
		dgVector p20 (p2 + p0);

		p01 = p01.Scale (float (1.0f) / dgSqrt(p01 % p01));
		p12 = p12.Scale (float (1.0f) / dgSqrt(p12 % p12));
		p20 = p20.Scale (float (1.0f) / dgSqrt(p20 % p20));

		HACD_ASSERT (dgAbsf (p01 % p01 - float (1.0f)) < float (1.0e-4f));
		HACD_ASSERT (dgAbsf (p12 % p12 - float (1.0f)) < float (1.0e-4f));
		HACD_ASSERT (dgAbsf (p20 % p20 - float (1.0f)) < float (1.0e-4f));

		TessellateTriangle  (level - 1, p0,  p01, p20, count, ouput, start);
		TessellateTriangle  (level - 1, p1,  p12, p01, count, ouput, start);
		TessellateTriangle  (level - 1, p2,  p20, p12, count, ouput, start);
		TessellateTriangle  (level - 1, p01, p12, p20, count, ouput, start);

	} else {
		dgBigPlane n (p0, p1, p2);
		n = n.Scale (double(1.0f) / sqrt (n % n));
		n.m_w = double(0.0f);
		ouput[start] = n;
		start += 8;
		count ++;
	}
}


int32_t dgConvexHull3d::SupportVertex (dgAABBPointTree3d** const treePointer, const dgHullVertex* const points, const dgBigVector& dir) const
{
/*
	double dist = float (-1.0e10f);
	int32_t index = -1;
	for (int32_t i = 0; i < count; i ++) {
		//double dist1 = dir.DotProduct4 (points[i]);
		double dist1 = dir % points[i];
		if (dist1 > dist) {
			dist = dist1;
			index = i;
		}
	}
	HACD_ASSERT (index != -1);
	return index;
*/

	#define DG_STACK_DEPTH_3D 64
	double aabbProjection[DG_STACK_DEPTH_3D];
	const dgAABBPointTree3d *stackPool[DG_STACK_DEPTH_3D];

	int32_t index = -1;
	int32_t stack = 1;
	stackPool[0] = *treePointer;
	aabbProjection[0] = float (1.0e20f);
	double maxProj = double (-1.0e20f); 
	int32_t ix = (dir[0] > double (0.0f)) ? 1 : 0;
	int32_t iy = (dir[1] > double (0.0f)) ? 1 : 0;
	int32_t iz = (dir[2] > double (0.0f)) ? 1 : 0;
	while (stack) {
		stack--;
		double boxSupportValue = aabbProjection[stack];
		if (boxSupportValue > maxProj) {
			const dgAABBPointTree3d* const me = stackPool[stack];

			if (me->m_left && me->m_right) {
				dgBigVector leftSupportPoint (me->m_left->m_box[ix].m_x, me->m_left->m_box[iy].m_y, me->m_left->m_box[iz].m_z, float (0.0));
				double leftSupportDist = leftSupportPoint % dir;

				dgBigVector rightSupportPoint (me->m_right->m_box[ix].m_x, me->m_right->m_box[iy].m_y, me->m_right->m_box[iz].m_z, float (0.0));
				double rightSupportDist = rightSupportPoint % dir;


				if (rightSupportDist >= leftSupportDist) {
					aabbProjection[stack] = leftSupportDist;
					stackPool[stack] = me->m_left;
					stack++;
					HACD_ASSERT (stack < DG_STACK_DEPTH_3D);
					aabbProjection[stack] = rightSupportDist;
					stackPool[stack] = me->m_right;
					stack++;
					HACD_ASSERT (stack < DG_STACK_DEPTH_3D);
				} else {
					aabbProjection[stack] = rightSupportDist;
					stackPool[stack] = me->m_right;
					stack++;
					HACD_ASSERT (stack < DG_STACK_DEPTH_3D);
					aabbProjection[stack] = leftSupportDist;
					stackPool[stack] = me->m_left;
					stack++;
					HACD_ASSERT (stack < DG_STACK_DEPTH_3D);
				}

			} else {
				dgAABBPointTree3dClump* const clump = (dgAABBPointTree3dClump*) me;
				for (int32_t i = 0; i < clump->m_count; i ++) {
					const dgHullVertex& p = points[clump->m_indices[i]];
					HACD_ASSERT (p.m_x >= clump->m_box[0].m_x);
					HACD_ASSERT (p.m_x <= clump->m_box[1].m_x);
					HACD_ASSERT (p.m_y >= clump->m_box[0].m_y);
					HACD_ASSERT (p.m_y <= clump->m_box[1].m_y);
					HACD_ASSERT (p.m_z >= clump->m_box[0].m_z);
					HACD_ASSERT (p.m_z <= clump->m_box[1].m_z);
					if (!p.m_index) {
						double dist = p % dir;
						if (dist > maxProj) {
							maxProj = dist;
							index = clump->m_indices[i];
						}
					} else {
						clump->m_indices[i] = clump->m_indices[clump->m_count - 1];
						clump->m_count = clump->m_count - 1;
						i --;
					}
				}

				if (clump->m_count == 0) {
					dgAABBPointTree3d* const parent = clump->m_parent;
					if (parent) {	
						dgAABBPointTree3d* const sibling = (parent->m_left != clump) ? parent->m_left : parent->m_right;
						HACD_ASSERT (sibling != clump);
						dgAABBPointTree3d* const grandParent = parent->m_parent;
						if (grandParent) {
							sibling->m_parent = grandParent;
							if (grandParent->m_right == parent) {
								grandParent->m_right = sibling;
							} else {
								grandParent->m_left = sibling;
							}
						} else {
							sibling->m_parent = NULL;
							*treePointer = sibling;
						}
					}
				}
			}
		}
	}

	HACD_ASSERT (index != -1);
	return index;
}


int32_t dgConvexHull3d::BuildNormalList (dgBigVector* const normalArray) const
{
	dgVector p0 ( float (1.0f), float (0.0f), float (0.0f), float (0.0f)); 
	dgVector p1 (-float (1.0f), float (0.0f), float (0.0f), float (0.0f)); 
	dgVector p2 ( float (0.0f), float (1.0f), float (0.0f), float (0.0f)); 
	dgVector p3 ( float (0.0f),-float (1.0f), float (0.0f), float (0.0f));
	dgVector p4 ( float (0.0f), float (0.0f), float (1.0f), float (0.0f));
	dgVector p5 ( float (0.0f), float (0.0f),-float (1.0f), float (0.0f));

	int32_t count = 0;
	int32_t subdivitions = 1;

	int32_t start = 0;
	TessellateTriangle  (subdivitions, p4, p0, p2, count, normalArray, start);
	start = 1;
	TessellateTriangle  (subdivitions, p5, p3, p1, count, normalArray, start);
	start = 2;
	TessellateTriangle  (subdivitions, p5, p1, p2, count, normalArray, start);
	start = 3;
	TessellateTriangle  (subdivitions, p4, p3, p0, count, normalArray, start);
	start = 4;
	TessellateTriangle  (subdivitions, p4, p2, p1, count, normalArray, start);
	start = 5;
	TessellateTriangle  (subdivitions, p5, p0, p3, count, normalArray, start);
	start = 6;
	TessellateTriangle  (subdivitions, p5, p2, p0, count, normalArray, start);
	start = 7;
	TessellateTriangle  (subdivitions, p4, p1, p3, count, normalArray, start);
	return count;
}

dgConvexHull3d::dgListNode* dgConvexHull3d::AddFace (int32_t i0, int32_t i1, int32_t i2)
{
	dgListNode* const node = Append();
	dgConvexHull3DFace& face = node->GetInfo();

	face.m_index[0] = i0; 
	face.m_index[1] = i1; 
	face.m_index[2] = i2; 
	return node;
}

void dgConvexHull3d::DeleteFace (dgListNode* const node) 
{
	Remove (node);
}

bool dgConvexHull3d::Sanity() const
{
/*
	for (dgListNode* node = GetFirst(); node; node = node->GetNext()) {
		dgConvexHull3DFace* const face = &node->GetInfo();		
		for (int32_t i = 0; i < 3; i ++) {
			dgListNode* const twinNode = face->m_twin[i];
			if (!twinNode) {
				return false;
			}

			int32_t count = 0;
			dgListNode* me = NULL;
			dgConvexHull3DFace* const twinFace = &twinNode->GetInfo();
			for (int32_t j = 0; j < 3; j ++) {
				if (twinFace->m_twin[j] == node) {
					count ++;
					me = twinFace->m_twin[j];
				}
			}
			if (count != 1) {
				return false;
			}
			if (me != node) {
				return false;
			}
		}
	}
*/
	return true;
}

void dgConvexHull3d::CalculateConvexHull (dgAABBPointTree3d* vertexTree, dgHullVertex* const points, int32_t count, double distTol, int32_t maxVertexCount)
{
	distTol = fabs (distTol) * m_diag;
	dgListNode* const f0Node = AddFace (0, 1, 2);
	dgListNode* const f1Node = AddFace (0, 2, 3);
	dgListNode* const f2Node = AddFace (2, 1, 3);
	dgListNode* const f3Node = AddFace (1, 0, 3);

	dgConvexHull3DFace* const f0 = &f0Node->GetInfo();
	dgConvexHull3DFace* const f1 = &f1Node->GetInfo();
	dgConvexHull3DFace* const f2 = &f2Node->GetInfo();
	dgConvexHull3DFace* const f3 = &f3Node->GetInfo();

	f0->m_twin[0] = (dgList<dgConvexHull3DFace>::dgListNode*)f3Node; 
	f0->m_twin[1] = (dgList<dgConvexHull3DFace>::dgListNode*)f2Node; 
	f0->m_twin[2] = (dgList<dgConvexHull3DFace>::dgListNode*)f1Node;

	f1->m_twin[0] = (dgList<dgConvexHull3DFace>::dgListNode*)f0Node; 
	f1->m_twin[1] = (dgList<dgConvexHull3DFace>::dgListNode*)f2Node; 
	f1->m_twin[2] = (dgList<dgConvexHull3DFace>::dgListNode*)f3Node;

	f2->m_twin[0] = (dgList<dgConvexHull3DFace>::dgListNode*)f0Node; 
	f2->m_twin[1] = (dgList<dgConvexHull3DFace>::dgListNode*)f3Node; 
	f2->m_twin[2] = (dgList<dgConvexHull3DFace>::dgListNode*)f1Node;

	f3->m_twin[0] = (dgList<dgConvexHull3DFace>::dgListNode*)f0Node; 
	f3->m_twin[1] = (dgList<dgConvexHull3DFace>::dgListNode*)f1Node; 
	f3->m_twin[2] = (dgList<dgConvexHull3DFace>::dgListNode*)f2Node;
	
	dgList<dgListNode*> boundaryFaces;

	boundaryFaces.Append(f0Node);
	boundaryFaces.Append(f1Node);
	boundaryFaces.Append(f2Node);
	boundaryFaces.Append(f3Node);

	dgStack<dgListNode*> stackPool(1024 + m_count);
	dgStack<dgListNode*> coneListPool(1024 + m_count);
	dgStack<dgListNode*> deleteListPool(1024 + m_count);

	dgListNode** const stack = &stackPool[0];
	dgListNode** const coneList = &stackPool[0];
	dgListNode** const deleteList = &deleteListPool[0];

	count -= 4;
	maxVertexCount -= 4;
	int32_t currentIndex = 4;

	while (boundaryFaces.GetCount() && count && (maxVertexCount > 0)) {
		// my definition of the optimal convex hull of a given vertex count, 
		// is the convex hull formed by a subset of vertex from the input array  
		// that minimized the volume difference between the perfect hull form those vertex, and the hull of the sub set of vertex.
		// Only using a priority heap we can be sure that it will generate the best hull selecting the best points from the vertex array.
		// Since all our tools do not have a limit on the point count of a hull I can use either a list of a queue.
		// a stack maximize construction speed, a Queue tend to maximize the volume of the generated Hull. for now we use a queue.
		// For perfect hulls it does not make a difference if we use a stack, queue, or a priority heap, 
		// this only apply for when build hull of a limited vertex count.
		//
		// Also when building Hulls of a limited vertex count, this function runs in constant time.
		// yes that is correct, it does not makes a difference if you build a N point hull from 100 vertex 
		// or from 100000 vertex input array.
		
		#if 0
			// using stack (faster)
			dgListNode* const faceNode = boundaryFaces.GetFirst()->GetInfo();
		#else 
			// using a queue (some what slower by better hull for reduce vertex count)
			dgListNode* const faceNode = boundaryFaces.GetLast()->GetInfo();
		#endif

		dgConvexHull3DFace* const face = &faceNode->GetInfo();
		dgBigPlane planeEquation (face->GetPlaneEquation (&m_points[0]));

		int32_t index = SupportVertex (&vertexTree, points, planeEquation);
		const dgBigVector& p = points[index];
		double dist = planeEquation.Evalue(p);

		if ((dist >= distTol) && (face->Evalue(&m_points[0], p) > double(0.0f))) {
			HACD_ASSERT (Sanity());
			
			HACD_ASSERT (faceNode);
			stack[0] = faceNode;

			int32_t stackIndex = 1;
			int32_t deletedCount = 0;

			while (stackIndex) {
				stackIndex --;
				dgListNode* const node = stack[stackIndex];
				dgConvexHull3DFace* const face = &node->GetInfo();

				if (!face->m_mark && (face->Evalue(&m_points[0], p) > double(0.0f))) { 
					#ifdef _DEBUG
					for (int32_t i = 0; i < deletedCount; i ++) {
						HACD_ASSERT (deleteList[i] != node);
					}
					#endif

					deleteList[deletedCount] = node;
					deletedCount ++;
					HACD_ASSERT (deletedCount < int32_t (deleteListPool.GetElementsCount()));
					face->m_mark = 1;
					for (int32_t i = 0; i < 3; i ++) {
						dgListNode* const twinNode = (dgListNode*)face->m_twin[i];
						HACD_ASSERT (twinNode);
						dgConvexHull3DFace* const twinFace = &twinNode->GetInfo();
						if (!twinFace->m_mark) {
							stack[stackIndex] = twinNode;
							stackIndex ++;
							HACD_ASSERT (stackIndex < int32_t (stackPool.GetElementsCount()));
						}
					}
				}
			}

//			Swap (hullVertexArray[index], hullVertexArray[currentIndex]);
			m_points[currentIndex] = points[index];
			points[index].m_index = 1;

			int32_t newCount = 0;
			for (int32_t i = 0; i < deletedCount; i ++) {
				dgListNode* const node = deleteList[i];
				dgConvexHull3DFace* const face = &node->GetInfo();
				HACD_ASSERT (face->m_mark == 1);
				for (int32_t j0 = 0; j0 < 3; j0 ++) {
					dgListNode* const twinNode = face->m_twin[j0];
					dgConvexHull3DFace* const twinFace = &twinNode->GetInfo();
					if (!twinFace->m_mark) {
						int32_t j1 = (j0 == 2) ? 0 : j0 + 1;
						dgListNode* const newNode = AddFace (currentIndex, face->m_index[j0], face->m_index[j1]);
						boundaryFaces.Addtop(newNode);

						dgConvexHull3DFace* const newFace = &newNode->GetInfo();
						newFace->m_twin[1] = twinNode;
						for (int32_t k = 0; k < 3; k ++) {
							if (twinFace->m_twin[k] == node) {
								twinFace->m_twin[k] = newNode;
							}
						}
						coneList[newCount] = newNode;
						newCount ++;
						HACD_ASSERT (newCount < int32_t (coneListPool.GetElementsCount()));
					}
				}
			}
			
			for (int32_t i = 0; i < newCount - 1; i ++) {
				dgListNode* const nodeA = coneList[i];
				dgConvexHull3DFace* const faceA = &nodeA->GetInfo();
				HACD_ASSERT (faceA->m_mark == 0);
				for (int32_t j = i + 1; j < newCount; j ++) {
					dgListNode* const nodeB = coneList[j];
					dgConvexHull3DFace* const faceB = &nodeB->GetInfo();
					HACD_ASSERT (faceB->m_mark == 0);
					if (faceA->m_index[2] == faceB->m_index[1]) {
						faceA->m_twin[2] = nodeB;
						faceB->m_twin[0] = nodeA;
						break;
					}
				}

				for (int32_t j = i + 1; j < newCount; j ++) {
					dgListNode* const nodeB = coneList[j];
					dgConvexHull3DFace* const faceB = &nodeB->GetInfo();
					HACD_ASSERT (faceB->m_mark == 0);
					if (faceA->m_index[1] == faceB->m_index[2]) {
						faceA->m_twin[0] = nodeB;
						faceB->m_twin[2] = nodeA;
						break;
					}
				}
			}

			for (int32_t i = 0; i < deletedCount; i ++) {
				dgListNode* const node = deleteList[i];
				boundaryFaces.Remove (node);
				DeleteFace (node); 
			}

			maxVertexCount --;
			currentIndex ++;
			count --;
		} else {
			boundaryFaces.Remove (faceNode);
		}
	}
	m_count = currentIndex;
}


void dgConvexHull3d::CalculateVolumeAndSurfaceArea (double& volume, double& surfaceArea) const
{
	double areaAcc = float (0.0f);
	double  volumeAcc = float (0.0f);
	for (dgListNode* node = GetFirst(); node; node = node->GetNext()) {
		const dgConvexHull3DFace* const face = &node->GetInfo();
		int32_t i0 = face->m_index[0];
		int32_t i1 = face->m_index[1];
		int32_t i2 = face->m_index[2];
		const dgBigVector& p0 = m_points[i0];
		const dgBigVector& p1 = m_points[i1];
		const dgBigVector& p2 = m_points[i2];
		dgBigVector normal ((p1 - p0) * (p2 - p0));
		double area = sqrt (normal % normal);
		areaAcc += area;
		volumeAcc += (p0 * p1) % p2;
	}
	HACD_ASSERT (volumeAcc >= double (0.0f));
	volume = volumeAcc * double (1.0f/6.0f);
	surfaceArea = areaAcc * double (0.5f);
}

// this code has linear time complexity on the number of faces
double dgConvexHull3d::RayCast (const dgBigVector& localP0, const dgBigVector& localP1) const
{
	double interset = float (1.2f);
	double tE = double (0.0f);    // for the maximum entering segment parameter;
	double tL = double (1.0f);    // for the minimum leaving segment parameter;
	dgBigVector dS (localP1 - localP0); // is the segment direction vector;
	int32_t hasHit = 0;

	for (dgListNode* node = GetFirst(); node; node = node->GetNext()) 
	{
		const dgConvexHull3DFace* const face = &node->GetInfo();

		int32_t i0 = face->m_index[0];
		int32_t i1 = face->m_index[1];
		int32_t i2 = face->m_index[2];

		const dgBigVector& p0 = m_points[i0];
		dgBigVector normal ((m_points[i1] - p0) * (m_points[i2] - p0));

		double N = -((localP0 - p0) % normal);
		double D = dS % normal;

		if (fabs(D) < double (1.0e-12f)) 
		{ // 
			if (N < double (0.0f)) 
			{
				return double (1.2f);
			} 
			else 
			{
				continue; 
			}
		}

		double t = N / D;
		if (D < double (0.0f)) 
		{
			if (t > tE) 
			{
				tE = t;
				hasHit = 1;
			}
			if (tE > tL) 
			{
				return double (1.2f);
			}
		} 
		else 
		{
			HACD_ASSERT (D >= double (0.0f));
			tL = GetMin (tL, t);
			if (tL < tE) 
			{
				return double (1.2f);
			}
		}
	}

	if (hasHit) 
	{
		interset = tE;	
	}

	return interset;
}


