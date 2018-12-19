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
*  File Name  : Bitmap.C
*  Visual C++ 4.0 base by Julio Jerez
*
****************************************************************************/
#include "dgHeap.h"
#include "dgStack.h"
#include "dgList.h"
#include "dgMatrix.h"
#include "dgPolygonSoupBuilder.h"
#include "dgAABBPolygonSoup.h"
#include "dgIntersections.h"

#define DG_STACK_DEPTH 63

class dgAABBTree
{
	class TreeNode
	{
		#define DG_INDEX_COUNT_BITS 6

		public:
		inline TreeNode ()
		{
			HACD_ALWAYS_ASSERT();
		}

		inline TreeNode (uint32_t node)
		{
			m_node = node;
			HACD_ASSERT (!IsLeaf());
		}

		inline uint32_t IsLeaf () const 
		{
			return m_node & 0x80000000;
		}

		inline uint32_t GetCount() const 
		{
			HACD_ASSERT (IsLeaf());
			return (m_node & (~0x80000000)) >> (32 - DG_INDEX_COUNT_BITS - 1);
		}

		inline uint32_t GetIndex() const 
		{
			HACD_ASSERT (IsLeaf());
			return m_node & (~(-(1 << (32 - DG_INDEX_COUNT_BITS - 1))));
		}


		inline TreeNode (uint32_t faceIndexCount, uint32_t faceIndexStart)
		{
			HACD_ASSERT (faceIndexCount < (1<<DG_INDEX_COUNT_BITS));
			m_node = 0x80000000 | (faceIndexCount << (32 - DG_INDEX_COUNT_BITS - 1)) | faceIndexStart;
		}

		inline dgAABBTree* GetNode (const void* root) const
		{
			return ((dgAABBTree*) root) + m_node;
		}

		union {
			uint32_t m_node;
		};
	};

	class dgHeapNodePair
	{
		public:
		int32_t m_nodeA;
		int32_t m_nodeB;
	};

	class dgConstructionTree : public UANS::UserAllocated
	{
		public:
		dgConstructionTree ()
		{
		}

		~dgConstructionTree ()
		{
			if (m_back) {
				delete m_back;
			}
			if (m_front) {
				delete m_front;
			}
		}

		dgVector m_p0;
		dgVector m_p1;
		int32_t m_boxIndex;
		float m_surfaceArea;
		dgConstructionTree* m_back;
		dgConstructionTree* m_front;
		dgConstructionTree* m_parent;
	};



	public: 
	void CalcExtends (dgTriplex* const vertex, int32_t indexCount, const int32_t* const indexArray) 
	{
		dgVector minP ( float (1.0e15f),  float (1.0e15f),  float (1.0e15f), float (0.0f)); 
		dgVector maxP (-float (1.0e15f), -float (1.0e15f), -float (1.0e15f), float (0.0f)); 
		for (int32_t i = 1; i < indexCount; i ++) {
			int32_t index = indexArray[i];
			dgVector p (&vertex[index].m_x);

			minP.m_x = GetMin (p.m_x, minP.m_x); 
			minP.m_y = GetMin (p.m_y, minP.m_y); 
			minP.m_z = GetMin (p.m_z, minP.m_z); 

			maxP.m_x = GetMax (p.m_x, maxP.m_x); 
			maxP.m_y = GetMax (p.m_y, maxP.m_y); 
			maxP.m_z = GetMax (p.m_z, maxP.m_z); 
		}

		vertex[m_minIndex].m_x = minP.m_x - float (1.0e-3f);
		vertex[m_minIndex].m_y = minP.m_y - float (1.0e-3f);
		vertex[m_minIndex].m_z = minP.m_z - float (1.0e-3f);
		vertex[m_maxIndex].m_x = maxP.m_x + float (1.0e-3f);
		vertex[m_maxIndex].m_y = maxP.m_y + float (1.0e-3f);
		vertex[m_maxIndex].m_z = maxP.m_z + float (1.0e-3f);
	}

	int32_t BuildTree (dgConstructionTree* const root, dgAABBTree* const boxArray, dgAABBTree* const boxCopy, dgTriplex* const vertexArrayOut, int32_t &treeVCount)
	{
		TreeNode* parent[128];
		dgConstructionTree* pool[128];

		int32_t index = 0;
		int32_t stack = 1;
		parent[0] = NULL;
		pool[0] = root;
		while (stack) {
			stack --;

			dgConstructionTree* const node = pool[stack];
			TreeNode* const parentNode = parent[stack];
			if (node->m_boxIndex != -1) {
				if (parentNode) {
					*parentNode = boxCopy[node->m_boxIndex].m_back;
				} else {

					//HACD_ASSERT (boxCount == 1);
					dgAABBTree* const newNode = &boxArray[index];
					*newNode = boxCopy[node->m_boxIndex];
					index ++;
				}

			} else {
				dgAABBTree* const newNode = &boxArray[index];

				newNode->m_minIndex = treeVCount;
				vertexArrayOut[treeVCount].m_x = node->m_p0.m_x;
				vertexArrayOut[treeVCount].m_y = node->m_p0.m_y;
				vertexArrayOut[treeVCount].m_z = node->m_p0.m_z;

				newNode->m_maxIndex = treeVCount + 1;
				vertexArrayOut[treeVCount + 1].m_x = node->m_p1.m_x;
				vertexArrayOut[treeVCount + 1].m_y = node->m_p1.m_y;
				vertexArrayOut[treeVCount + 1].m_z = node->m_p1.m_z;
				treeVCount += 2;

				if (parentNode) {
					*parentNode = TreeNode (uint32_t(index));
				}
				index ++;

				pool[stack] = node->m_front;
				parent[stack] = &newNode->m_front;
				stack ++;
				pool[stack] = node->m_back;
				parent[stack] = &newNode->m_back;
				stack ++;
			}
		}

		// this object is not to be deleted when using stack allocation
		//delete root;
		return index;
	}

	void PushNodes (dgConstructionTree* const root, dgList<dgConstructionTree*>& list) const
	{
		if (root->m_back) {
			PushNodes (root->m_back, list);
		}
		if (root->m_front) {
			PushNodes (root->m_front, list);
		}
		if (root->m_boxIndex == -1) {
			list.Append(root);
		}
	}

	int32_t CalculateMaximunDepth (dgConstructionTree* tree) const 
	{
		int32_t depthPool[128];
		dgConstructionTree* pool[128];

		depthPool[0] = 0;
		pool[0] = tree;
		int32_t stack = 1;

		int32_t maxDepth = -1;
		while (stack) {
			stack --;

			int32_t depth = depthPool[stack];
			dgConstructionTree* const node = pool[stack];
			maxDepth = GetMax(maxDepth, depth);

			if (node->m_boxIndex == -1) {
				HACD_ASSERT (node->m_back);
				HACD_ASSERT (node->m_front);

				depth ++;
				depthPool[stack] = depth;
				pool[stack] = node->m_back;
				stack ++;

				depthPool[stack] = depth;
				pool[stack] = node->m_front;
				stack ++;
			}
		}

		return maxDepth + 1;
	}

	float CalculateArea (dgConstructionTree* const node0, dgConstructionTree* const node1) const
	{
		dgVector p0 (GetMin (node0->m_p0.m_x, node1->m_p0.m_x), GetMin (node0->m_p0.m_y, node1->m_p0.m_y), GetMin (node0->m_p0.m_z, node1->m_p0.m_z), float (0.0f));
		dgVector p1 (GetMax (node0->m_p1.m_x, node1->m_p1.m_x), GetMax (node0->m_p1.m_y, node1->m_p1.m_y), GetMax (node0->m_p1.m_z, node1->m_p1.m_z), float (0.0f));		
		dgVector side0 (p1 - p0);
		dgVector side1 (side0.m_y, side0.m_z, side0.m_x, float (0.0f));
		return side0 % side1;
	}

	void SetBox (dgConstructionTree* const node) const
	{
		node->m_p0.m_x = GetMin (node->m_back->m_p0.m_x, node->m_front->m_p0.m_x);
		node->m_p0.m_y = GetMin (node->m_back->m_p0.m_y, node->m_front->m_p0.m_y);
		node->m_p0.m_z = GetMin (node->m_back->m_p0.m_z, node->m_front->m_p0.m_z);
		node->m_p1.m_x = GetMax (node->m_back->m_p1.m_x, node->m_front->m_p1.m_x);
		node->m_p1.m_y = GetMax (node->m_back->m_p1.m_y, node->m_front->m_p1.m_y);
		node->m_p1.m_z = GetMax (node->m_back->m_p1.m_z, node->m_front->m_p1.m_z);
		dgVector side0 (node->m_p1 - node->m_p0);
		dgVector side1 (side0.m_y, side0.m_z, side0.m_x, float (0.0f));
		node->m_surfaceArea = side0 % side1;
	}

	void ImproveNodeFitness (dgConstructionTree* const node) const
	{
		HACD_ASSERT (node->m_back);
		HACD_ASSERT (node->m_front);

		if (node->m_parent)	{
			if (node->m_parent->m_back == node) {
				float cost0 = node->m_surfaceArea;
				float cost1 = CalculateArea (node->m_front, node->m_parent->m_front);
				float cost2 = CalculateArea (node->m_back, node->m_parent->m_front);
				if ((cost1 <= cost0) && (cost1 <= cost2)) {
					dgConstructionTree* const parent = node->m_parent;
					node->m_p0 = parent->m_p0;
					node->m_p1 = parent->m_p1;
					node->m_surfaceArea = parent->m_surfaceArea; 
					if (parent->m_parent) {
						if (parent->m_parent->m_back == parent) {
							parent->m_parent->m_back = node;
						} else {
							HACD_ASSERT (parent->m_parent->m_front == parent);
							parent->m_parent->m_front = node;
						}
					}
					node->m_parent = parent->m_parent;
					parent->m_parent = node;
					node->m_front->m_parent = parent;
					parent->m_back = node->m_front;
					node->m_front = parent;
					SetBox (parent);
				} else if ((cost2 <= cost0) && (cost2 <= cost1)) {
					dgConstructionTree* const parent = node->m_parent;
					node->m_p0 = parent->m_p0;
					node->m_p1 = parent->m_p1;
					node->m_surfaceArea = parent->m_surfaceArea; 
					if (parent->m_parent) {
						if (parent->m_parent->m_back == parent) {
							parent->m_parent->m_back = node;
						} else {
							HACD_ASSERT (parent->m_parent->m_front == parent);
							parent->m_parent->m_front = node;
						}
					}
					node->m_parent = parent->m_parent;
					parent->m_parent = node;
					node->m_back->m_parent = parent;
					parent->m_back = node->m_back;
					node->m_back = parent;
					SetBox (parent);
				}

			} else {

				float cost0 = node->m_surfaceArea;
				float cost1 = CalculateArea (node->m_back, node->m_parent->m_back);
				float cost2 = CalculateArea (node->m_front, node->m_parent->m_back);
				if ((cost1 <= cost0) && (cost1 <= cost2)) {
					dgConstructionTree* const parent = node->m_parent;
					node->m_p0 = parent->m_p0;
					node->m_p1 = parent->m_p1;
					node->m_surfaceArea = parent->m_surfaceArea; 
					if (parent->m_parent) {
						if (parent->m_parent->m_back == parent) {
							parent->m_parent->m_back = node;
						} else {
							HACD_ASSERT (parent->m_parent->m_front == parent);
							parent->m_parent->m_front = node;
						}
					}
					node->m_parent = parent->m_parent;
					parent->m_parent = node;
					node->m_back->m_parent = parent;
					parent->m_front = node->m_back;
					node->m_back = parent;
					SetBox (parent);
				} else if ((cost2 <= cost0) && (cost2 <= cost1)) {
					dgConstructionTree* const parent = node->m_parent;
					node->m_p0 = parent->m_p0;
					node->m_p1 = parent->m_p1;
					node->m_surfaceArea = parent->m_surfaceArea; 
					if (parent->m_parent) {
						if (parent->m_parent->m_back == parent) {
							parent->m_parent->m_back = node;
						} else {
							HACD_ASSERT (parent->m_parent->m_front == parent);
							parent->m_parent->m_front = node;
						}
					}
					node->m_parent = parent->m_parent;
					parent->m_parent = node;
					node->m_front->m_parent = parent;
					parent->m_front = node->m_front;
					node->m_front = parent;
					SetBox (parent);
				}
			}
		}
	}

	double TotalFitness (dgList<dgConstructionTree*>& nodeList) const
	{
		double cost = float (0.0f);
		for (dgList<dgConstructionTree*>::dgListNode* node = nodeList.GetFirst(); node; node = node->GetNext()) {
			dgConstructionTree* const box = node->GetInfo();
			cost += box->m_surfaceArea;
		}
		return cost;
	}

	dgConstructionTree* ImproveTotalFitness (dgConstructionTree* const root, dgAABBTree* const boxArray)
	{
		HACD_FORCE_PARAMETER_REFERENCE(boxArray);
		dgList<dgConstructionTree*> nodesList;

		dgConstructionTree* newRoot = root;
		PushNodes (root, nodesList);
		if (nodesList.GetCount()) {
			int32_t maxPasses = CalculateMaximunDepth (root) * 2;
			double newCost = TotalFitness (nodesList);
			double prevCost = newCost;
			do {
				prevCost = newCost;
				for (dgList<dgConstructionTree*>::dgListNode* node = nodesList.GetFirst(); node; node = node->GetNext()) {
					dgConstructionTree* const box = node->GetInfo();
					ImproveNodeFitness (box);
				}

				newCost = TotalFitness (nodesList);
				maxPasses --;
			} while (maxPasses && (newCost < prevCost));

			newRoot = nodesList.GetLast()->GetInfo();
			while (newRoot->m_parent) {
				newRoot = newRoot->m_parent;
			}
		}

		return newRoot;
	}

	dgConstructionTree* BuildTree (int32_t firstBox, int32_t lastBox, dgAABBTree* const boxArray, const dgTriplex* const vertexArray, dgConstructionTree* parent)
	{
		dgConstructionTree* const tree = HACD_NEW(dgConstructionTree);
		HACD_ASSERT (firstBox >= 0);
		HACD_ASSERT (lastBox >= 0);

		tree->m_parent = parent;

		if (lastBox == firstBox) {
			int32_t j0 = boxArray[firstBox].m_minIndex;
			int32_t j1 = boxArray[firstBox].m_maxIndex;
			tree->m_p0 = dgVector (vertexArray[j0].m_x, vertexArray[j0].m_y, vertexArray[j0].m_z, float (0.0f));
			tree->m_p1 = dgVector (vertexArray[j1].m_x, vertexArray[j1].m_y, vertexArray[j1].m_z, float (0.0f));
			
			tree->m_boxIndex = firstBox;
			tree->m_back = NULL;
			tree->m_front = NULL;
		} else {

			struct dgSpliteInfo
			{
				dgSpliteInfo (dgAABBTree* const boxArray, int32_t boxCount, const dgTriplex* const vertexArray, const dgConstructionTree* const /*tree*/)
				{

					dgVector minP ( float (1.0e15f),  float (1.0e15f),  float (1.0e15f), float (0.0f)); 
					dgVector maxP (-float (1.0e15f), -float (1.0e15f), -float (1.0e15f), float (0.0f)); 

					if (boxCount == 2) {
						m_axis = 1;

						for (int32_t i = 0; i < boxCount; i ++) {
							int32_t j0 = boxArray[i].m_minIndex;
							int32_t j1 = boxArray[i].m_maxIndex;

							dgVector p0 (vertexArray[j0].m_x, vertexArray[j0].m_y, vertexArray[j0].m_z, float (0.0f));
							dgVector p1 (vertexArray[j1].m_x, vertexArray[j1].m_y, vertexArray[j1].m_z, float (0.0f));

							minP.m_x = GetMin (p0.m_x, minP.m_x); 
							minP.m_y = GetMin (p0.m_y, minP.m_y); 
							minP.m_z = GetMin (p0.m_z, minP.m_z); 
							
							maxP.m_x = GetMax (p1.m_x, maxP.m_x); 
							maxP.m_y = GetMax (p1.m_y, maxP.m_y); 
							maxP.m_z = GetMax (p1.m_z, maxP.m_z); 
						}

					} else {
						dgVector median (float (0.0f), float (0.0f), float (0.0f), float (0.0f));
						dgVector varian (float (0.0f), float (0.0f), float (0.0f), float (0.0f));
						for (int32_t i = 0; i < boxCount; i ++) {

							int32_t j0 = boxArray[i].m_minIndex;
							int32_t j1 = boxArray[i].m_maxIndex;

							dgVector p0 (vertexArray[j0].m_x, vertexArray[j0].m_y, vertexArray[j0].m_z, float (0.0f));
							dgVector p1 (vertexArray[j1].m_x, vertexArray[j1].m_y, vertexArray[j1].m_z, float (0.0f));

							minP.m_x = GetMin (p0.m_x, minP.m_x); 
							minP.m_y = GetMin (p0.m_y, minP.m_y); 
							minP.m_z = GetMin (p0.m_z, minP.m_z); 

							maxP.m_x = GetMax (p1.m_x, maxP.m_x); 
							maxP.m_y = GetMax (p1.m_y, maxP.m_y); 
							maxP.m_z = GetMax (p1.m_z, maxP.m_z); 

							dgVector p ((p0 + p1).Scale (0.5f));

							median += p;
							varian += p.CompProduct (p);
						}

						varian = varian.Scale (float (boxCount)) - median.CompProduct(median);


						int32_t index = 0;
						float maxVarian = float (-1.0e10f);
						for (int32_t i = 0; i < 3; i ++) {
							if (varian[i] > maxVarian) {
								index = i;
								maxVarian = varian[i];
							}
						}
	
						dgVector center = median.Scale (float (1.0f) / float (boxCount));

						float test = center[index];

						int32_t i0 = 0;
						int32_t i1 = boxCount - 1;
						int32_t step = sizeof (dgTriplex) / sizeof (float);
						const float* const points = &vertexArray[0].m_x;
						do {    
							for (; i0 <= i1; i0 ++) {
								int32_t j0 = boxArray[i0].m_minIndex;
								int32_t j1 = boxArray[i0].m_maxIndex;

								float val = (points[j0 * step + index] + points[j1 * step + index]) * float (0.5f);
								if (val > test) {
									break;
								}
							}

							for (; i1 >= i0; i1 --) {
								int32_t j0 = boxArray[i1].m_minIndex;
								int32_t j1 = boxArray[i1].m_maxIndex;

								float val = (points[j0 * step + index] + points[j1 * step + index]) * float (0.5f);
								if (val < test) {
									break;
								}
							}

							if (i0 < i1)	{
								Swap(boxArray[i0], boxArray[i1]);
								i0++; 
								i1--;
							}
						} while (i0 <= i1);
						
						if (i0 > 0){
							i0 --;
						}
						if ((i0 + 1) >= boxCount) {
							i0 = boxCount - 2;
						}

						m_axis = i0 + 1;
					}

					HACD_ASSERT (maxP.m_x - minP.m_x >= float (0.0f));
					HACD_ASSERT (maxP.m_y - minP.m_y >= float (0.0f));
					HACD_ASSERT (maxP.m_z - minP.m_z >= float (0.0f));
					m_p0 = minP;
					m_p1 = maxP;
				}

				int32_t m_axis;
				dgVector m_p0;
				dgVector m_p1;
			};

			dgSpliteInfo info (&boxArray[firstBox], lastBox - firstBox + 1, vertexArray, tree);

			tree->m_boxIndex = -1;
			tree->m_p0 = info.m_p0;
			tree->m_p1 = info.m_p1;

			tree->m_front = BuildTree (firstBox + info.m_axis, lastBox, boxArray, vertexArray, tree);
			tree->m_back = BuildTree (firstBox, firstBox + info.m_axis - 1, boxArray, vertexArray, tree);
		}

		dgVector side0 (tree->m_p1 - tree->m_p0);
		dgVector side1 (side0.m_y, side0.m_z, side0.m_x, float (0.0f));
		tree->m_surfaceArea = side0 % side1; 
	
		return tree;
	}


	int32_t BuildTopDown (int32_t boxCount, dgAABBTree* const boxArray, dgTriplex* const vertexArrayOut, int32_t &treeVCount, bool optimizedBuild)
	{
		dgStack <dgAABBTree> boxCopy (boxCount);
		memcpy (&boxCopy[0], boxArray, boxCount * sizeof (dgAABBTree));

		dgConstructionTree* tree = BuildTree (0, boxCount - 1, &boxCopy[0], vertexArrayOut, NULL);

		optimizedBuild = true;
		if (optimizedBuild) {
			tree = ImproveTotalFitness (tree, &boxCopy[0]);
		}

		int32_t count = BuildTree (tree, boxArray, &boxCopy[0], vertexArrayOut, treeVCount);
		delete tree;
		return count;
	}

	HACD_FORCE_INLINE int32_t BoxIntersect (const dgTriplex* const vertexArray, const dgVector& min, const dgVector& max) const
	{
		dgVector boxP0 (vertexArray[m_minIndex].m_x, vertexArray[m_minIndex].m_y, vertexArray[m_minIndex].m_z, float (0.0f));
		dgVector boxP1 (vertexArray[m_maxIndex].m_x, vertexArray[m_maxIndex].m_y, vertexArray[m_maxIndex].m_z, float (0.0f));
		return dgOverlapTest (boxP0, boxP1, min, max) - 1;
	}


	HACD_FORCE_INLINE int32_t RayTest (const dgFastRayTest& ray, const dgTriplex* const vertexArray) const
	{
		float tmin = 0.0f;          
		float tmax = 1.0f;

		dgVector minBox (&vertexArray[m_minIndex].m_x);
		dgVector maxBox (&vertexArray[m_maxIndex].m_x);

		for (int32_t i = 0; i < 3; i++) {
			if (ray.m_isParallel[i]) {
				if (ray.m_p0[i] < minBox[i] || ray.m_p0[i] > maxBox[i]) {
					return 0;
				}
			} else {
				float t1 = (minBox[i] - ray.m_p0[i]) * ray.m_dpInv[i];
				float t2 = (maxBox[i] - ray.m_p0[i]) * ray.m_dpInv[i];

				if (t1 > t2) {
					Swap(t1, t2);
				}
				if (t1 > tmin) {
					tmin = t1;
				}
				if (t2 < tmax) {
					tmax = t2;
				}
				if (tmin > tmax) {
					return 0;
				}
			}
		}

		return 0xffffffff;
	}


	void ForAllSectors (const int32_t* const indexArray, const float* const vertexArray, const dgVector& min, const dgVector& max, dgAABBIntersectCallback callback, void* const context) const
	{
		int32_t stack;
		const dgAABBTree *stackPool[DG_STACK_DEPTH];

		stack = 1;
		stackPool[0] = this;
		while (stack) {
			stack	--;
			const dgAABBTree* const me = stackPool[stack];

			if (me->BoxIntersect ((dgTriplex*) vertexArray, min, max) >= 0) {

				if (me->m_back.IsLeaf()) {
					int32_t index = int32_t (me->m_back.GetIndex());
					int32_t vCount = int32_t ((me->m_back.GetCount() >> 1) - 1);
					if ((vCount > 0) && callback(context, vertexArray, sizeof (dgTriplex), &indexArray[index + 1], vCount) == t_StopSearh) {
						return;
					}

				} else {
					HACD_ASSERT (stack < DG_STACK_DEPTH);
					stackPool[stack] = me->m_back.GetNode(this);
					stack++;
				}

				if (me->m_front.IsLeaf()) {
					int32_t index = int32_t (me->m_front.GetIndex());
					int32_t vCount = int32_t ((me->m_front.GetCount() >> 1) - 1);
					if ((vCount > 0) && callback(context, vertexArray, sizeof (dgTriplex), &indexArray[index + 1], vCount) == t_StopSearh) {
						return;
					}

				} else {
					HACD_ASSERT (stack < DG_STACK_DEPTH);
					stackPool[stack] = me->m_front.GetNode(this);
					stack ++;
				}
			}
		}
	}


	void ForAllSectorsRayHit (const dgFastRayTest& raySrc, const int32_t* indexArray, const float* vertexArray, dgRayIntersectCallback callback, void* const context) const
	{
		const dgAABBTree *stackPool[DG_STACK_DEPTH];
		dgFastRayTest ray (raySrc);
		
		int32_t stack = 1;
		float maxParam = float (1.0f);

		stackPool[0] = this;
		while (stack) {
			stack --;
			const dgAABBTree *const me = stackPool[stack];
			if (me->RayTest (ray, (dgTriplex*) vertexArray)) {
				
				if (me->m_back.IsLeaf()) {
					int32_t vCount = int32_t ((me->m_back.GetCount() >> 1) - 1);
					if (vCount > 0) {
						int32_t index = int32_t (me->m_back.GetIndex());
						float param = callback(context, vertexArray, sizeof (dgTriplex), &indexArray[index + 1], vCount);
						HACD_ASSERT (param >= float (0.0f));
						if (param < maxParam) {
							maxParam = param;
							if (maxParam == float (0.0f)) {
								break;
							}
							ray.Reset (maxParam) ;
						}
					}

				} else {
					HACD_ASSERT (stack < DG_STACK_DEPTH);
					stackPool[stack] = me->m_back.GetNode(this);
					stack++;
				}

				if (me->m_front.IsLeaf()) {
					int32_t vCount = int32_t ((me->m_front.GetCount() >> 1) - 1);
					if (vCount > 0) {
						int32_t index = int32_t (me->m_front.GetIndex());
						float param = callback(context, vertexArray, sizeof (dgTriplex), &indexArray[index + 1], vCount);
						HACD_ASSERT (param >= float (0.0f));
						if (param < maxParam) {
							maxParam = param;
							if (maxParam == float (0.0f)) {
								break;
							}
							ray.Reset (maxParam);
						}
					}

				} else {
					HACD_ASSERT (stack < DG_STACK_DEPTH);
					stackPool[stack] = me->m_front.GetNode(this);
					stack ++;
				}
			}
		}
	}


	dgVector ForAllSectorsSupportVertex (const dgVector& dir, const int32_t* const indexArray, const float* const vertexArray) const
	{
		float aabbProjection[DG_STACK_DEPTH];
		const dgAABBTree *stackPool[DG_STACK_DEPTH];

		int32_t stack = 1;
		stackPool[0] = this;
		aabbProjection[0] = float (1.0e10f);
		const dgTriplex* const boxArray = (dgTriplex*)vertexArray;
		dgVector supportVertex (float (0.0f), float (0.0f), float (0.0f), float (0.0f));

		float maxProj = float (-1.0e20f); 
		int32_t ix = (dir[0] > float (0.0f)) ? 1 : 0;
		int32_t iy = (dir[1] > float (0.0f)) ? 1 : 0;
		int32_t iz = (dir[2] > float (0.0f)) ? 1 : 0;

		while (stack) {
			float boxSupportValue;

			stack--;
			boxSupportValue = aabbProjection[stack];
			if (boxSupportValue > maxProj) {
				float backSupportDist = float (0.0f);
				float frontSupportDist = float (0.0f);
				const dgAABBTree* const me = stackPool[stack];
				if (me->m_back.IsLeaf()) {
					backSupportDist = float (-1.0e20f);
					int32_t index = int32_t (me->m_back.GetIndex());
					int32_t vCount = int32_t ((me->m_back.GetCount() >> 1) - 1);
		
					dgVector vertex (float (0.0f), float (0.0f), float (0.0f), float (0.0f));
					for (int32_t j = 0; j < vCount; j ++) {
						int32_t i0 = indexArray[index + j + 1] * int32_t (sizeof (dgTriplex) / sizeof (float));
						dgVector p (&vertexArray[i0]);
						float dist = p % dir;
						if (dist > backSupportDist) {
							backSupportDist = dist;
							vertex = p;
						}
					}
					
					if (backSupportDist > maxProj) {
						maxProj = backSupportDist;
						supportVertex = vertex; 
					}

				} else {
					dgVector box[2];
					const dgAABBTree* const node = me->m_back.GetNode(this);
					box[0].m_x = boxArray[node->m_minIndex].m_x;
					box[0].m_y = boxArray[node->m_minIndex].m_y;
					box[0].m_z = boxArray[node->m_minIndex].m_z;
					box[1].m_x = boxArray[node->m_maxIndex].m_x;
					box[1].m_y = boxArray[node->m_maxIndex].m_y;
					box[1].m_z = boxArray[node->m_maxIndex].m_z;

					dgVector supportPoint (box[ix].m_x, box[iy].m_y, box[iz].m_z, float (0.0));
					backSupportDist = supportPoint % dir;
				}

				if (me->m_front.IsLeaf()) {
					frontSupportDist = float (-1.0e20f);
					int32_t index = int32_t (me->m_front.GetIndex());
					int32_t vCount = int32_t ((me->m_front.GetCount() >> 1) - 1);

					dgVector vertex (float (0.0f), float (0.0f), float (0.0f), float (0.0f));
					for (int32_t j = 0; j < vCount; j ++) {
						int32_t i0 = indexArray[index + j + 1] * int32_t (sizeof (dgTriplex) / sizeof (float));
						dgVector p (&vertexArray[i0]);
						float dist = p % dir;
						if (dist > frontSupportDist) {
							frontSupportDist = dist;
							vertex = p;
						}
					}
					if (frontSupportDist > maxProj) {
						maxProj = frontSupportDist;
						supportVertex = vertex; 
					}

				} else {
					dgVector box[2];
					const dgAABBTree* const node = me->m_front.GetNode(this);
					box[0].m_x = boxArray[node->m_minIndex].m_x;
					box[0].m_y = boxArray[node->m_minIndex].m_y;
					box[0].m_z = boxArray[node->m_minIndex].m_z;
					box[1].m_x = boxArray[node->m_maxIndex].m_x;
					box[1].m_y = boxArray[node->m_maxIndex].m_y;
					box[1].m_z = boxArray[node->m_maxIndex].m_z;

					dgVector supportPoint (box[ix].m_x, box[iy].m_y, box[iz].m_z, float (0.0));
					frontSupportDist = supportPoint % dir;
				}

				if (frontSupportDist >= backSupportDist) {
					if (!me->m_back.IsLeaf()) {
						aabbProjection[stack] = backSupportDist;
						stackPool[stack] = me->m_back.GetNode(this);
						stack++;
					}

					if (!me->m_front.IsLeaf()) {
						aabbProjection[stack] = frontSupportDist;
						stackPool[stack] = me->m_front.GetNode(this);
						stack++;
					}

				} else {

					if (!me->m_front.IsLeaf()) {
						aabbProjection[stack] = frontSupportDist;
						stackPool[stack] = me->m_front.GetNode(this);
						stack++;
					}

					if (!me->m_back.IsLeaf()) {
						aabbProjection[stack] = backSupportDist;
						stackPool[stack] = me->m_back.GetNode(this);
						stack++;
					}
				}
			}
		}

		return supportVertex;
	}


	private:

	int32_t GetAxis (dgConstructionTree** boxArray, int32_t boxCount) const
	{
		int32_t axis;
		float maxVal;
		dgVector median (float (0.0f), float (0.0f), float (0.0f), float (0.0f));
		dgVector varian (float (0.0f), float (0.0f), float (0.0f), float (0.0f));
		for (int32_t i = 0; i < boxCount; i ++) {

			median += boxArray[i]->m_p0;
			median += boxArray[i]->m_p1;

			varian += boxArray[i]->m_p0.CompProduct(boxArray[i]->m_p0);
			varian += boxArray[i]->m_p1.CompProduct(boxArray[i]->m_p1);
		}

		boxCount *= 2;
		varian.m_x = boxCount * varian.m_x - median.m_x * median.m_x;
		varian.m_y = boxCount * varian.m_y - median.m_y * median.m_y;
		varian.m_z = boxCount * varian.m_z - median.m_z * median.m_z;

		axis = 0;
		maxVal = varian[0];
		for (int32_t i = 1; i < 3; i ++) {
			if (varian[i] > maxVal) {
				axis = i;
				maxVal = varian[i];
			}
		}

		return axis;
	}

/*
	class DG_AABB_CMPBOX
	{
		public:
		int32_t m_axis;
		const dgTriplex* m_points;
	};
	static inline int32_t CompareBox (const dgAABBTree* const boxA, const dgAABBTree* const boxB, void* const context)
	{
		DG_AABB_CMPBOX& info = *((DG_AABB_CMPBOX*)context);

		int32_t axis = info.m_axis;
		const float* p0 = &info.m_points[boxA->m_minIndex].m_x;
		const float* p1 = &info.m_points[boxB->m_minIndex].m_x;

		if (p0[axis] < p1[axis]) {
			return -1;
		} else if (p0[axis] > p1[axis]) {
			return 1;
		}
		return 0;
	}
*/

	static inline int32_t CompareBox (const dgConstructionTree* const boxA, const dgConstructionTree* const boxB, void* const context)
	{
		int32_t axis;

		axis = *((int32_t*) context);

		if (boxA->m_p0[axis] < boxB->m_p0[axis]) {
			return -1;
		} else if (boxA->m_p0[axis] > boxB->m_p0[axis]) {
			return 1;
		}
		return 0;
	}




	int32_t m_minIndex;
	int32_t m_maxIndex;
	TreeNode m_back;
	TreeNode m_front;
	friend class dgAABBPolygonSoup;
};



dgAABBPolygonSoup::dgAABBPolygonSoup ()
	:dgPolygonSoupDatabase()
{
	m_aabb = NULL;
	m_indices = NULL;
	m_indexCount = 0;
	m_nodesCount = 0;
}

dgAABBPolygonSoup::~dgAABBPolygonSoup ()
{
	if (m_aabb) {
		HACD_FREE (m_aabb);
		HACD_FREE (m_indices);
	}
}


void* dgAABBPolygonSoup::GetRootNode() const
{
	return m_aabb;
}

void* dgAABBPolygonSoup::GetBackNode(const void* const root) const
{
	dgAABBTree* const node = (dgAABBTree*) root;
	return node->m_back.IsLeaf() ? NULL : node->m_back.GetNode(m_aabb);
}

void* dgAABBPolygonSoup::GetFrontNode(const void* const root) const
{
	dgAABBTree* const node = (dgAABBTree*) root;
	return node->m_front.IsLeaf() ? NULL : node->m_front.GetNode(m_aabb);
}


void dgAABBPolygonSoup::GetNodeAABB(const void* const root, dgVector& p0, dgVector& p1) const
{
	dgAABBTree* const node = (dgAABBTree*) root;
	const dgTriplex* const vertex = (dgTriplex*) m_localVertex;

	p0 = dgVector (vertex[node->m_minIndex].m_x, vertex[node->m_minIndex].m_y, vertex[node->m_minIndex].m_z, float (0.0f));
	p1 = dgVector (vertex[node->m_maxIndex].m_x, vertex[node->m_maxIndex].m_y, vertex[node->m_maxIndex].m_z, float (0.0f));
}


void dgAABBPolygonSoup::ForAllSectors (
	const dgVector& min, 
	const dgVector& max, 
	dgAABBIntersectCallback callback, 
	void* const context) const
{
	dgAABBTree* tree;

	if (m_aabb) {
		tree = (dgAABBTree*) m_aabb;
		tree->ForAllSectors (m_indices, m_localVertex, min, max, callback, context);
	}
}

dgVector dgAABBPolygonSoup::ForAllSectorsSupportVectex (const dgVector& dir) const
{
	dgAABBTree* tree;

	if (m_aabb) {
		tree = (dgAABBTree*) m_aabb;
		return tree->ForAllSectorsSupportVertex (dir, m_indices, m_localVertex);
	} else {
		return dgVector (0, 0, 0, 0);
	}
}


void dgAABBPolygonSoup::GetAABB (dgVector& p0, dgVector& p1) const
{
	if (m_aabb) { 
		dgAABBTree* tree;
		tree = (dgAABBTree*) m_aabb;
		p0 = dgVector (&m_localVertex[tree->m_minIndex * 3]);
		p1 = dgVector (&m_localVertex[tree->m_maxIndex * 3]);
	} else {
		p0 = dgVector (float (0.0f), float (0.0f), float (0.0f), float (0.0f));
		p1 = dgVector (float (0.0f), float (0.0f), float (0.0f), float (0.0f));
	}
}


void dgAABBPolygonSoup::ForAllSectorsRayHit (const dgFastRayTest& ray, dgRayIntersectCallback callback, void* const context) const
{
	dgAABBTree* tree;

	if (m_aabb) {
		tree = (dgAABBTree*) m_aabb;
		tree->ForAllSectorsRayHit (ray, m_indices, m_localVertex, callback, context);
	}
}

dgIntersectStatus dgAABBPolygonSoup::CalculateThisFaceEdgeNormals (void *context, const float* const polygon, int32_t strideInBytes, const int32_t* const indexArray, int32_t indexCount)
{
	AdjacentdFaces& adjacentFaces = *((AdjacentdFaces*)context);

	int32_t count = adjacentFaces.m_count;
	int32_t stride = int32_t (strideInBytes / sizeof (float));

			int32_t j0 = indexArray[indexCount - 1];
			for (int32_t j = 0; j < indexCount; j ++) {
				int32_t j1 = indexArray[j];
		int64_t key = (int64_t (j0) << 32) + j1;
		for (int32_t i = 0; i < count; i ++) {
			if (adjacentFaces.m_edgeMap[i] == key) {
					float maxDist = float (0.0f);
					for (int32_t k = 0; k < indexCount; k ++) {
						dgVector r (&polygon[indexArray[k] * stride]);
						float dist = adjacentFaces.m_normal.Evalue(r);
						if (dgAbsf (dist) > dgAbsf (maxDist)) {
							maxDist = dist;
						}
					}
				if (maxDist < float (1.0e-4f)) {
					adjacentFaces.m_index[i + count + 1] = indexArray[indexCount];
					}
					break;
				}
		}

				j0 = j1;
			}

	return t_ContinueSearh;
}


dgIntersectStatus dgAABBPolygonSoup::CalculateAllFaceEdgeNormals (void *context, const float* const polygon, int32_t strideInBytes, const int32_t* const indexArray, int32_t indexCount)
{
	dgAABBPolygonSoup* const me = (dgAABBPolygonSoup*) context;

	int32_t stride = int32_t (strideInBytes / sizeof (float));

	AdjacentdFaces adjacentFaces;
	adjacentFaces.m_count = indexCount;
	adjacentFaces.m_index = (int32_t*) indexArray;
	dgVector n (&polygon[indexArray[indexCount] * stride]);
	dgVector p (&polygon[indexArray[0] * stride]);
	adjacentFaces.m_normal = dgPlane (n, - (n % p));

	HACD_ASSERT (indexCount < int32_t (sizeof (adjacentFaces.m_edgeMap) / sizeof (adjacentFaces.m_edgeMap[0])));

	int32_t edgeIndex = indexCount - 1;
	int32_t i0 = indexArray[indexCount - 1];
	dgVector p0 ( float (1.0e15f),  float (1.0e15f),  float (1.0e15f), float (0.0f)); 
	dgVector p1 (-float (1.0e15f), -float (1.0e15f), -float (1.0e15f), float (0.0f)); 
	for (int32_t i = 0; i < indexCount; i ++) {
		int32_t i1 = indexArray[i];
		int32_t index = i1 * stride;
		dgVector p (&polygon[index]);

		p0.m_x = GetMin (p.m_x, p0.m_x); 
		p0.m_y = GetMin (p.m_y, p0.m_y); 
		p0.m_z = GetMin (p.m_z, p0.m_z); 
								
		p1.m_x = GetMax (p.m_x, p1.m_x); 
		p1.m_y = GetMax (p.m_y, p1.m_y); 
		p1.m_z = GetMax (p.m_z, p1.m_z); 

		adjacentFaces.m_edgeMap[edgeIndex] = (int64_t (i1) << 32) + i0;
		edgeIndex = i;
		i0 = i1;
	}

	p0.m_x -= float (0.5f);
	p0.m_y -= float (0.5f);
	p0.m_z -= float (0.5f);
	p1.m_x += float (0.5f);
	p1.m_y += float (0.5f);
	p1.m_z += float (0.5f);

	me->ForAllSectors (p0, p1, CalculateThisFaceEdgeNormals, &adjacentFaces);

	return t_ContinueSearh;
}

float dgAABBPolygonSoup::CalculateFaceMaxSize (dgTriplex* const vertex, int32_t indexCount, const int32_t* const indexArray) const
{
	float maxSize = float (0.0f);
	int32_t index = indexArray[indexCount - 1];
	dgVector p0 (vertex[index].m_x, vertex[index].m_y, vertex[index].m_z, float (0.0f));
	for (int32_t i = 0; i < indexCount; i ++) {
		int32_t index = indexArray[i];
		dgVector p1 (vertex[index].m_x, vertex[index].m_y, vertex[index].m_z, float (0.0f));

		dgVector dir (p1 - p0);
		dir = dir.Scale (dgRsqrt (dir % dir));

		float maxVal = float (-1.0e10f);
		float minVal = float ( 1.0e10f);
		for (int32_t j = 0; j < indexCount; j ++) {
			int32_t index = indexArray[j];
			dgVector q (vertex[index].m_x, vertex[index].m_y, vertex[index].m_z, float (0.0f));
			float val = dir % q;
			minVal = GetMin(minVal, val);
			maxVal = GetMax(maxVal, val);
		}

		float size = maxVal - minVal;
		maxSize = GetMax(maxSize, size);
		p0 = p1;
	}

	return maxSize;
}

void dgAABBPolygonSoup::CalculateAdjacendy ()
{
	dgVector p0;
	dgVector p1;
	GetAABB (p0, p1);
	ForAllSectors (p0, p1, CalculateAllFaceEdgeNormals, this);
}

void dgAABBPolygonSoup::Create (const dgPolygonSoupDatabaseBuilder& builder, bool optimizedBuild)
{
	if (builder.m_faceCount == 0) {
		return;
	}

	m_strideInBytes = sizeof (dgTriplex);
	m_indexCount = builder.m_indexCount * 2 + builder.m_faceCount;
	m_indices = (int32_t*) HACD_ALLOC (sizeof (int32_t) * m_indexCount);
	m_aabb = (dgAABBTree*) HACD_ALLOC (sizeof (dgAABBTree) * builder.m_faceCount);
	m_localVertex = (float*) HACD_ALLOC (sizeof (dgTriplex) * (builder.m_vertexCount + builder.m_normalCount + builder.m_faceCount * 4));

	dgAABBTree* const tree = (dgAABBTree*) m_aabb;
	dgTriplex* const tmpVertexArray = (dgTriplex*)m_localVertex;
	
	for (int32_t i = 0; i < builder.m_vertexCount; i ++) {
		tmpVertexArray[i].m_x = float (builder.m_vertexPoints[i].m_x);
		tmpVertexArray[i].m_y = float (builder.m_vertexPoints[i].m_y);
		tmpVertexArray[i].m_z = float (builder.m_vertexPoints[i].m_z);
	}

	for (int32_t i = 0; i < builder.m_normalCount; i ++) {
		tmpVertexArray[i + builder.m_vertexCount].m_x = float (builder.m_normalPoints[i].m_x); 
		tmpVertexArray[i + builder.m_vertexCount].m_y = float (builder.m_normalPoints[i].m_y); 
		tmpVertexArray[i + builder.m_vertexCount].m_z = float (builder.m_normalPoints[i].m_z); 
	}

	int32_t polygonIndex = 0;
	int32_t* indexMap = m_indices;
	const int32_t* const indices = &builder.m_vertexIndex[0];
	for (int32_t i = 0; i < builder.m_faceCount; i ++) {

		int32_t indexCount = builder.m_faceVertexCount[i];
		dgAABBTree& box = tree[i];

		box.m_minIndex = builder.m_normalCount + builder.m_vertexCount + i * 2;
		box.m_maxIndex = builder.m_normalCount + builder.m_vertexCount + i * 2 + 1;

		box.m_front = dgAABBTree::TreeNode(0, 0);
		box.m_back = dgAABBTree::TreeNode (uint32_t (indexCount * 2), uint32_t (indexMap - m_indices));
		box.CalcExtends (&tmpVertexArray[0], indexCount, &indices[polygonIndex]);

		for (int32_t j = 0; j < indexCount; j ++) {
			indexMap[0] = indices[polygonIndex + j];
			indexMap ++;
		}

		indexMap[0] = builder.m_vertexCount + builder.m_normalIndex[i];
		indexMap ++;
		for (int32_t j = 1; j < indexCount; j ++) {
			indexMap[0] = -1;
			indexMap ++;
		}

		float faceSize = CalculateFaceMaxSize (&tmpVertexArray[0], indexCount - 1, &indices[polygonIndex + 1]);
		indexMap[0] = int32_t (faceSize);
		indexMap ++;
		polygonIndex += indexCount;
	}

	int32_t extraVertexCount = builder.m_normalCount + builder.m_vertexCount + builder.m_faceCount * 2;
//	if (optimizedBuild) {
//		m_nodesCount = tree->BuildBottomUp (builder.m_allocator, builder.m_faceCount, tree, &tmpVertexArray[0], extraVertexCount);
//	} else {
//		m_nodesCount = tree->BuildTopDown (builder.m_allocator, builder.m_faceCount, tree, &tmpVertexArray[0], extraVertexCount);
//	}

//	m_nodesCount = tree->BuildBottomUp (builder.m_allocator, builder.m_faceCount, tree, &tmpVertexArray[0], extraVertexCount, optimizedBuild);
	m_nodesCount = tree->BuildTopDown (builder.m_faceCount, tree, &tmpVertexArray[0], extraVertexCount, optimizedBuild);

	m_localVertex[tree->m_minIndex * 3 + 0] -= float (0.1f);
	m_localVertex[tree->m_minIndex * 3 + 1] -= float (0.1f);
	m_localVertex[tree->m_minIndex * 3 + 2] -= float (0.1f);
	m_localVertex[tree->m_maxIndex * 3 + 0] += float (0.1f);
	m_localVertex[tree->m_maxIndex * 3 + 1] += float (0.1f);
	m_localVertex[tree->m_maxIndex * 3 + 2] += float (0.1f);


	extraVertexCount -= (builder.m_normalCount + builder.m_vertexCount);

	dgStack<int32_t> indexArray (extraVertexCount);
	extraVertexCount = dgVertexListToIndexList (&tmpVertexArray[builder.m_normalCount + builder.m_vertexCount].m_x, sizeof (dgTriplex), sizeof (dgTriplex), 0, extraVertexCount, &indexArray[0], float (1.0e-6f));

	for (int32_t i = 0; i < m_nodesCount; i ++) {
		dgAABBTree& box = tree[i];

		int32_t j = box.m_minIndex - builder.m_normalCount - builder.m_vertexCount;
		box.m_minIndex = indexArray[j] + builder.m_normalCount + builder.m_vertexCount;
		j = box.m_maxIndex - builder.m_normalCount - builder.m_vertexCount;
		box.m_maxIndex = indexArray[j] + builder.m_normalCount + builder.m_vertexCount;
	}

	m_vertexCount = extraVertexCount + builder.m_normalCount + builder.m_vertexCount;
}

