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



#include "Apex.h"
#include "ApexQuadricSimplifier.h"

#include "ApexSharedUtils.h"

#define MERGE_THRESHOLD 1.0e-6f

// PH: Verification code, must be set to 0 unless you're debugging this code
#define TESTING 0

namespace nvidia
{
namespace apex
{

ApexQuadricSimplifier::ApexQuadricSimplifier() : mNumDeletedTriangles(0), mNumDeletedVertices(0), mNumDeletedHeapElements(0)
{
	mBounds.setEmpty();
}



ApexQuadricSimplifier::~ApexQuadricSimplifier()
{
	clear();
}



void ApexQuadricSimplifier::clear()
{
	for (uint32_t i = 0; i < mVertices.size(); i++)
	{
		delete mVertices[i];
	}

	mVertices.clear();
	mEdges.clear();
	mTriangles.clear();
	mHeap.clear();
	mVertexRefs.clear();
	mBounds.setEmpty();

	mNumDeletedTriangles = 0;
	mNumDeletedVertices = 0;
	mNumDeletedHeapElements = 0;
}



void ApexQuadricSimplifier::registerVertex(const PxVec3& pos)
{
	mVertices.pushBack(PX_NEW(QuadricVertex)(pos));
	mBounds.include(pos);
}



void ApexQuadricSimplifier::registerTriangle(uint32_t v0, uint32_t v1, uint32_t v2)
{
	const uint32_t numVertices = mVertices.size();
	PX_ASSERT(v0 < numVertices);
	PX_ASSERT(v1 < numVertices);
	PX_ASSERT(v2 < numVertices);
	PX_UNUSED(numVertices);

	QuadricTriangle t;
	t.init(v0, v1, v2);
	uint32_t tNr = mTriangles.size();
	mVertices[t.vertexNr[0]]->mTriangles.pushBack(tNr);
	mVertices[t.vertexNr[0]]->bReferenced = 1;
	mVertices[t.vertexNr[1]]->mTriangles.pushBack(tNr);
	mVertices[t.vertexNr[1]]->bReferenced = 1;
	mVertices[t.vertexNr[2]]->mTriangles.pushBack(tNr);
	mVertices[t.vertexNr[2]]->bReferenced = 1;
	mTriangles.pushBack(t);
}



bool ApexQuadricSimplifier::endRegistration(bool mergeCloseVertices, IProgressListener* progressListener)
{
	HierarchicalProgressListener progress(100, progressListener);

	if (mergeCloseVertices)
	{
		progress.setSubtaskWork(20, "Merge Vertices");
		mergeVertices();
		progress.completeSubtask();
	}

	// remove unreferenced vertices
	for (uint32_t i = 0; i < mVertices.size(); i++)
	{
		if (mVertices[i]->bReferenced == 0)
		{
			mVertices[i]->bDeleted = 1;
			mNumDeletedVertices++;
		}
	}
	progress.setSubtaskWork(20, "Init Edge List");

	mEdges.clear();
	for (uint32_t i = 0; i < mTriangles.size(); i++)
	{
		QuadricTriangle& t = mTriangles[i];
		QuadricVertex* qv0 = mVertices[t.vertexNr[0]];
		QuadricVertex* qv1 = mVertices[t.vertexNr[1]];
		QuadricVertex* qv2 = mVertices[t.vertexNr[2]];
		Quadric q;
		q.setFromPlane(qv0->pos, qv1->pos, qv2->pos);
		qv0->q += q;
		qv1->q += q;
		qv2->q += q;
		QuadricEdge e;
		e.init((int32_t)t.vertexNr[0], (int32_t)t.vertexNr[1]);
		mEdges.pushBack(e);
		e.init((int32_t)t.vertexNr[1], (int32_t)t.vertexNr[2]);
		mEdges.pushBack(e);
		e.init((int32_t)t.vertexNr[2], (int32_t)t.vertexNr[0]);
		mEdges.pushBack(e);
	}

	progress.completeSubtask();

	// remove duplicated edges
	progress.setSubtaskWork(10, "Sort Edges");
	quickSortEdges(0, (int32_t)mEdges.size() - 1);
	progress.completeSubtask();

	progress.setSubtaskWork(10, "Process Edges");

	physx::Array<QuadricEdge> edges2;
	uint32_t i = 0;
	while (i < mEdges.size())
	{
		QuadricEdge& e = mEdges[i];
		edges2.pushBack(e);
		i++;

		bool border = true;
		while (i < mEdges.size() && mEdges[i] == e)
		{
			i++;
			border = false;
		}
		if (border)
		{
			edges2.back().border = true;
			mVertices[e.vertexNr[0]]->bBorder = 1;
			mVertices[e.vertexNr[1]]->bBorder = 1;
		}
	}
	progress.completeSubtask();




	progress.setSubtaskWork(10, "Init Edges");

	mEdges.clear();
	mEdges.resize(edges2.size());

	for (i = 0; i < edges2.size(); i++)
	{
		mEdges[i] = edges2[i];
		QuadricEdge& e = mEdges[i];
		computeCost(e);
		mVertices[e.vertexNr[0]]->mEdges.pushBack(i);
		mVertices[e.vertexNr[1]]->mEdges.pushBack(i);
	}

	progress.completeSubtask();

	progress.setSubtaskWork(10, "Insert Heap");

	// make heap
	uint32_t num = mEdges.size();
	mHeap.clear();
	mHeap.pushBack(NULL);	// dummy, root must be at position 1!
	for (i = 0; i < num; i++)
	{
		mHeap.pushBack(&mEdges[i]);
		mEdges[i].heapPos = (int32_t)i + 1;
	}

	progress.completeSubtask();

	progress.setSubtaskWork(mergeCloseVertices ? 20 : 40, "Create Heap");

	for (i = mHeap.size() >> 1; i >= 1; i--)
	{
		heapSift(i);
	}

	progress.completeSubtask();

	mNumDeletedTriangles = 0;
	mNumDeletedHeapElements = 0;
	return true;
}



uint32_t ApexQuadricSimplifier::simplify(uint32_t subdivision, int32_t maxSteps, float maxError, IProgressListener* progressListener)
{
	float maxLength = 0.0f;

	uint32_t nbCollapsed = 0;

	if (subdivision > 0)
	{
		maxLength = (mBounds.minimum - mBounds.maximum).magnitude() / subdivision;
	}

	uint32_t progressCounter = 0;
	uint32_t maximum = maxSteps >= 0 ? maxSteps : mHeap.size();

	HierarchicalProgressListener progress(100, progressListener);
	progress.setSubtaskWork(90, "Isomesh simplicifaction");
#if TESTING
	testHeap();
#endif

	while (maxSteps == -1 || (maxSteps-- > 0))
	{

		if ((++progressCounter & 0xff) == 0)
		{
			const int32_t percent = (int32_t)(100 * progressCounter / maximum);
			progress.setProgress(percent);
		}

		bool edgeFound = false;
		QuadricEdge* e = NULL;
		while (mHeap.size() - mNumDeletedHeapElements > 1)
		{
			e = mHeap[1];

			if (maxError >= 0 && e->cost > maxError)
			{
				// get me out of here
				edgeFound = false;
				break;
			}

			if (legalCollapse(*e, maxLength))
			{
				heapRemove(1, false);
#if TESTING
				testHeap();
#endif
				edgeFound = true;
				break;
			}
			uint32_t vNr0 = e->vertexNr[0];
			uint32_t vNr1 = e->vertexNr[1];
			QuadricVertex* qv0 = mVertices[vNr0];
			QuadricVertex* qv1 = mVertices[vNr1];
			heapRemove(1, qv0->bDeleted == 0 && qv1->bDeleted == 0);
#if TESTING
			testHeap();
#endif
		}

		if (!edgeFound)
		{
			break;
		}

		collapseEdge(*e);
		nbCollapsed++;
	}

	progress.completeSubtask();
	progress.setSubtaskWork(10, "Heap rebuilding");

	progressCounter = mNumDeletedHeapElements;
	while (mNumDeletedHeapElements > 0)
	{
		if ((mNumDeletedHeapElements & 0x7f) == 0)
		{
			const int32_t percent =  (int32_t)(100 * (progressCounter - mNumDeletedHeapElements) / progressCounter);
			progress.setProgress(percent);
		}
#if TESTING
		testHeap();
#endif
		mNumDeletedHeapElements--;
		heapUpdate(mHeap.size() - 1 - mNumDeletedHeapElements);
	}

	progress.completeSubtask();
#if TESTING
	testHeap();
#endif
	return nbCollapsed;
}



int32_t ApexQuadricSimplifier::getTriangleNr(uint32_t v0, uint32_t v1, uint32_t v2) const
{
	uint32_t num = mVertices.size();

	if (v0 >= num || v1 >= num || v2 >= num)
	{
		return -1;
	}

	QuadricVertex* qv0 = mVertices[v0];

	if (qv0->bDeleted == 1)
	{
		return -1;
	}

	for (unsigned i = 0; i < qv0->mTriangles.size(); i++)
	{
		const QuadricTriangle& t = mTriangles[qv0->mTriangles[i]];
		if (!t.containsVertex(v1) || !t.containsVertex(v2))
		{
			continue;
		}

		return (int32_t)qv0->mTriangles[i];
	}

	return -1;
}



bool ApexQuadricSimplifier::getTriangle(uint32_t i, uint32_t& v0, uint32_t& v1, uint32_t& v2) const
{
	if (i >= mTriangles.size())
	{
		return false;
	}

	const QuadricTriangle& t = mTriangles[i];
	if (t.deleted)
	{
		return false;
	}

	v0 = t.vertexNr[0];
	v1 = t.vertexNr[1];
	v2 = t.vertexNr[2];
	return true;
}








void ApexQuadricSimplifier::computeCost(QuadricEdge& edge)
{
	const uint32_t numSteps = 10;

	QuadricVertex* qv0 = mVertices[edge.vertexNr[0]];
	QuadricVertex* qv1 = mVertices[edge.vertexNr[1]];

	edge.cost  = FLT_MAX;
	edge.lengthSquared = (qv0->pos - qv1->pos).magnitudeSquared();
	edge.ratio = -1.0f;

	Quadric q;
	q = qv0->q + qv1->q;

	float sumCost = 0;
	for (uint32_t i = 0; i <= numSteps; i++)
	{
		const float ratio = 1.0f / numSteps * i;
		const PxVec3 pos = qv0->pos * (1.0f - ratio) + qv1->pos * ratio;

		const float cost = PxAbs(q.outerProduct(pos));
		sumCost += cost;
		if (cost < edge.cost)
		{
			edge.cost = cost;
			edge.ratio = ratio;
		}
	}

	if (sumCost < 0.0001f)
	{
		edge.cost = 0;
		edge.ratio = 0.5f;
	}
}



bool ApexQuadricSimplifier::legalCollapse(QuadricEdge& edge, float maxLength)
{
	uint32_t vNr0 = edge.vertexNr[0];
	uint32_t vNr1 = edge.vertexNr[1];
	QuadricVertex* qv0 = mVertices[vNr0];
	QuadricVertex* qv1 = mVertices[vNr1];

	// here we make sure that the border does not shrink
	uint32_t numBorder = 0;
	if (qv0->bBorder == 1)
	{
		numBorder++;
	}
	if (qv1->bBorder == 1)
	{
		numBorder++;
	}
	if (numBorder == 1)
	{
		return false;
	}

	if (qv0->bDeleted == 1 || qv1->bDeleted == 1)
	{
		return false;
	}

	// this is a test to make sure edges don't get too long
	if (maxLength != 0.0f)
	{
		if ((qv0->pos - qv1->pos).magnitudeSquared() > maxLength * maxLength)
		{
			return false;
		}
	}

	// not legal if there exists v != v0,v1 with (v0,v) and (v1,v) edges
	// but (v,v0,v1) is not a triangle

	for (uint32_t i = 0; i < qv0->mEdges.size(); i++)
	{
		uint32_t v = mEdges[qv0->mEdges[i]].otherVertex(vNr0);
		bool edgeFound = false;
		for (uint32_t j = 0; j < qv1->mEdges.size(); j++)
		{
			if (mEdges[qv1->mEdges[j]].otherVertex(vNr1) == v)
			{
				edgeFound = true;
				break;
			}
		}
		if (!edgeFound)
		{
			continue;
		}

		bool triFound = false;
		for (uint32_t j = 0; j < qv0->mTriangles.size(); j++)
		{
			QuadricTriangle& t = mTriangles[qv0->mTriangles[j]];
			if (t.containsVertex(vNr0) && t.containsVertex(vNr1) && t.containsVertex(v))
			{
				triFound = true;
				break;
			}
		}
		if (!triFound)
		{
			return false;
		}

	}

	// any triangle flips?
	PxVec3 newPos = qv0->pos * (1.0f - edge.ratio) + qv1->pos * edge.ratio;
	for (uint32_t i = 0; i < 2; i++)
	{
		QuadricVertex* v = (i == 0 ? qv0 : qv1);
		for (uint32_t j = 0; j < v->mTriangles.size(); j++)
		{
			QuadricTriangle& t = mTriangles[v->mTriangles[j]];
			if (t.deleted)
			{
				continue;
			}
			if (t.containsVertex(vNr0) && t.containsVertex(vNr1))	// this one will be deleted
			{
				continue;
			}
			PxVec3 p[3], q[3];
			for (uint32_t k = 0; k < 3; k++)
			{
				p[k] = mVertices[t.vertexNr[k]]->pos;
				if (t.vertexNr[k] == vNr0 || t.vertexNr[k] == vNr1)
				{
					q[k] = newPos;
				}
				else
				{
					q[k] = p[k];
				}
			}
			PxVec3 n0 = (p[1] - p[0]).cross(p[2] - p[0]);
			// n0.normalize(); // not needed for 90 degree checks
			PxVec3 n1 = (q[1] - q[0]).cross(q[2] - q[0]);
			//n1.normalize(); // not needed for 90 degree checks
			if (n0.dot(n1) < 0.0f)
			{
				return false;
			}
		}
	}

	return true;
}



void ApexQuadricSimplifier::collapseEdge(QuadricEdge& edge)
{
	uint32_t vNr0 = edge.vertexNr[0];
	uint32_t vNr1 = edge.vertexNr[1];
	QuadricVertex* qv0 = mVertices[vNr0];
	QuadricVertex* qv1 = mVertices[vNr1];

	PX_ASSERT(qv0->bDeleted == 0);
	PX_ASSERT(qv1->bDeleted == 0);

	//FILE* f = NULL;
	//fopen_s(&f, "c:\\collapse.txt", "a");
	//fprintf_s(f, "Collapse Vertex %d -> %d\n", vNr1, vNr0);

	// contract edge to the vertex0
	qv0->pos = qv0->pos * (1.0f - edge.ratio) + qv1->pos * edge.ratio;
	qv0->q += qv1->q;

	// merge the edges
	for (uint32_t i = 0; i < qv1->mEdges.size(); i++)
	{
		QuadricEdge& ei = mEdges[qv1->mEdges[i]];
		uint32_t vi = ei.otherVertex(vNr1);
		if (vi == vNr0)
		{
			continue;
		}

		// test whether we already have this neighbor
		bool found = false;
		for (uint32_t j = 0; j < qv0->mEdges.size(); j++)
		{
			QuadricEdge& ej = mEdges[qv0->mEdges[j]];
			if (ej.otherVertex(vNr0) == vi)
			{
				found = true;
				break;
			}
		}
		if (found)
		{
			mVertices[vi]->removeEdge((int32_t)qv1->mEdges[i]);
			ei.deleted = true;
			if (ei.heapPos >= 0)
			{
				heapRemove((uint32_t)ei.heapPos, false);
			}
#if TESTING
			testHeap();
#endif
		}
		else
		{
			ei.replaceVertex(vNr1, vNr0);
			qv0->mEdges.pushBack(qv1->mEdges[i]);
		}
	}
	// remove common edge and update adjacent edges
	for (int32_t i = (int32_t)qv0->mEdges.size() - 1; i >= 0; i--)
	{
		QuadricEdge& ei = mEdges[qv0->mEdges[(uint32_t)i]];
		if (ei.otherVertex(vNr0) == vNr1)
		{
			qv0->mEdges.replaceWithLast((uint32_t)i);
		}
		else
		{
			computeCost(ei);
			if (ei.heapPos >= 0)
			{
				heapUpdate((uint32_t)ei.heapPos);
			}
#if TESTING
			testHeap();
#endif
		}
	}

	// delete collapsed triangles
	for (int32_t i = (int32_t)qv0->mTriangles.size() - 1; i >= 0; i--)
	{
		uint32_t triangleIndex = qv0->mTriangles[(uint32_t)i];
		QuadricTriangle& t = mTriangles[triangleIndex];
		if (!t.deleted && t.containsVertex(vNr1))
		{
			mNumDeletedTriangles++;
			t.deleted = true;
			//fprintf_s(f, "Delete Triangle %d\n", triangleIndex);

			PX_ASSERT(t.containsVertex(vNr0));

			for (uint32_t j = 0; j < 3; j++)
			{
				mVertices[t.vertexNr[j]]->removeTriangle((int32_t)triangleIndex);
				//fprintf_s(f, "  v %d\n", t.vertexNr[j]);
			}
		}
	}
	// update triangles
	for (uint32_t i = 0; i < qv1->mTriangles.size(); i++)
	{
		QuadricTriangle& t = mTriangles[qv1->mTriangles[i]];
		if (t.deleted)
		{
			continue;
		}
		if (t.containsVertex(vNr1))
		{
			qv0->mTriangles.pushBack(qv1->mTriangles[i]);
		}
		t.replaceVertex(vNr1, vNr0);
	}

	mNumDeletedVertices += qv1->bDeleted == 1 ? 0 : 1;
	qv1->bDeleted = 1;
	edge.deleted = true;
	//fclose(f);

#if TESTING
	testMesh();
	testHeap();
#endif
}



void ApexQuadricSimplifier::quickSortEdges(int32_t l, int32_t r)
{
	uint32_t i, j, mi;
	QuadricEdge k, m;

	i = (uint32_t)l;
	j = (uint32_t)r;
	mi = (uint32_t)(l + r) / 2;
	m = mEdges[mi];
	while (i <= j)
	{
		while (mEdges[i] < m)
		{
			i++;
		}

		while (m < mEdges[j])
		{
			j--;
		}

		if (i <= j)
		{
			k = mEdges[i];
			mEdges[i] = mEdges[j];
			mEdges[j] = k;
			i++;
			j--;
		}
	}

	if (l < (int32_t)j)
	{
		quickSortEdges(l, (int32_t)j);
	}

	if ((int32_t)i < r)
	{
		quickSortEdges((int32_t)i, r);
	}
}



void ApexQuadricSimplifier::quickSortVertexRefs(int32_t l, int32_t r)
{
	uint32_t i, j, mi;
	QuadricVertexRef k, m;

	i = (uint32_t)l;
	j = (uint32_t)r;
	mi = (uint32_t)(l + r) / 2;
	m = mVertexRefs[mi];
	while (i <= j)
	{
		while (mVertexRefs[i] < m)
		{
			i++;
		}

		while (m < mVertexRefs[j])
		{
			j--;
		}

		if (i <= j)
		{
			k = mVertexRefs[i];
			mVertexRefs[i] = mVertexRefs[j];
			mVertexRefs[j] = k;
			i++;
			j--;
		}
	}

	if (l < (int32_t)j)
	{
		quickSortVertexRefs(l, (int32_t)j);
	}

	if ((int32_t)i < r)
	{
		quickSortVertexRefs((int32_t)i, r);
	}
}



void ApexQuadricSimplifier::mergeVertices()
{
	float d = (mBounds.minimum - mBounds.maximum).magnitude() * MERGE_THRESHOLD;
	float d2 = d * d;
	mVertexRefs.clear();
	QuadricVertexRef vr;

	for (uint32_t i = 0; i < mVertices.size(); i++)
	{
		QuadricVertex* v = mVertices[i];
		vr.init(v->pos, (int32_t)i);
		mVertexRefs.pushBack(vr);
	}

	quickSortVertexRefs(0, (int32_t)mVertexRefs.size() - 1);

	for (uint32_t i = 0; i < mVertexRefs.size() - 1; i++)
	{
		uint32_t iNr = mVertexRefs[i].vertexNr;
		QuadricVertex* vi = mVertices[iNr];

		if (vi->bDeleted == 1)
		{
			continue;
		}

		PxVec3 pos = mVertexRefs[i].pos;
		uint32_t j = i + 1;
		while (j < mVertexRefs.size() && fabs(mVertexRefs[j].pos.x - pos.x) < MERGE_THRESHOLD)
		{
			if ((mVertexRefs[j].pos - pos).magnitudeSquared() < d2)
			{
				uint32_t jNr = mVertexRefs[j].vertexNr;
				QuadricVertex* vj = mVertices[jNr];
				for (uint32_t k = 0; k < vj->mTriangles.size(); k++)
				{
					mTriangles[vj->mTriangles[k]].replaceVertex(jNr, iNr);
					vi->addTriangle((int32_t)vj->mTriangles[k]);
				}
				mNumDeletedVertices += vj->bDeleted == 1 ? 0 : 1;
				vj->bDeleted = 1;
			}
			j++;
		}
	}
}



bool ApexQuadricSimplifier::heapElementSmaller(QuadricEdge* e0, QuadricEdge* e1)
{
	// if both costs are 0 use edge length
	if (e0->cost == 0 && e1->cost == 0)
	{
		return e0->lengthSquared < e1->lengthSquared;
	}

	const float costDiff = e0->cost - e1->cost;

	if (costDiff != 0.0f)
	{
		return costDiff < 0;
	}

	// else use edge length
	return e0->lengthSquared < e1->lengthSquared;
}



void ApexQuadricSimplifier::heapUpdate(uint32_t i)
{
	const uint32_t num = mHeap.size() - mNumDeletedHeapElements;
	PX_ASSERT(1 <= i && i < num);
	QuadricEdge* e = mHeap[i];
	while (i > 1)
	{
		uint32_t j = i >> 1;
		if (heapElementSmaller(e, mHeap[j]))
		{
			mHeap[i] = mHeap[j];
			mHeap[i]->heapPos = (int32_t)i;
			i = j;
		}
		else
		{
			break;
		}
	}

	while ((i << 1) < num)
	{
		uint32_t j = i << 1;
		if (j + 1 < num && heapElementSmaller(mHeap[j + 1], mHeap[j]))
		{
			j++;
		}

		if (heapElementSmaller(mHeap[j], e))
		{
			mHeap[i] = mHeap[j];
			mHeap[i]->heapPos = (int32_t)i;
			i = j;
		}
		else
		{
			break;
		}
	}
	mHeap[i] = e;
	mHeap[i]->heapPos = (int32_t)i;
}



void ApexQuadricSimplifier::heapSift(uint32_t i)
{
	const uint32_t num = mHeap.size() - mNumDeletedHeapElements;
	PX_ASSERT(1 <= i && i < num);
	QuadricEdge* e = mHeap[i];
	while ((i << 1) < num)
	{
		uint32_t j = i << 1;
		if (j + 1 < num && heapElementSmaller(mHeap[j + 1], mHeap[j]))
		{
			j++;
		}

		if (heapElementSmaller(mHeap[j], e))
		{
			mHeap[i] = mHeap[j];
			mHeap[i]->heapPos = (int32_t)i;
			i = j;
		}
		else
		{
			break;
		}
	}

	mHeap[i] = e;
	mHeap[i]->heapPos = (int32_t)i;
}



void ApexQuadricSimplifier::heapRemove(uint32_t i, bool append)
{
	const uint32_t num = mHeap.size() - mNumDeletedHeapElements;
	PX_ASSERT(1 <= i && i < num);
	QuadricEdge* e = mHeap[i];
	mHeap[i]->heapPos = -1;
	mHeap[i] = mHeap[num - 1];
	if (append)
	{
		mHeap[num - 1] = e;
		mNumDeletedHeapElements++;
	}
	else if (mNumDeletedHeapElements > 0)
	{
		mHeap[num - 1] = mHeap.back();
		mHeap[num - 1]->heapPos = -1;
		mHeap.popBack();
	}
	else
	{
		mHeap.popBack();
	}

	PX_ASSERT(e->heapPos == -1);
	if (i < num - 1)
	{
		mHeap[i]->heapPos = (int32_t)i;
		heapUpdate(i);
	}
	PX_ASSERT(e->heapPos == -1);
}



void ApexQuadricSimplifier::testHeap()
{
	const uint32_t num = mHeap.size() - mNumDeletedHeapElements;
	for (uint32_t i = 1; i < num; i++)
	{
		PX_ASSERT(mHeap[i]->heapPos == (int32_t) i);
		if ((i << 1) < num)
		{
			uint32_t j = i << 1;
			if (j + 1 < num && heapElementSmaller(mHeap[j + 1], mHeap[j]))
			{
				j++;
			}
			PX_ASSERT(!heapElementSmaller(mHeap[j], mHeap[i]));
		}
	}

	for (uint32_t i = num; i < mHeap.size(); i++)
	{
		PX_ASSERT(mHeap[i]->heapPos == -1);
	}
}



void ApexQuadricSimplifier::testMesh()
{
	for (uint32_t i = 0; i < mVertices.size(); i++)
	{
		QuadricVertex* v = mVertices[i];
		if (v->bDeleted == 1)
		{
			continue;
		}

		for (uint32_t j = 0; j < v->mEdges.size(); j++)
		{
			uint32_t eNr = v->mEdges[j];
			PX_ASSERT(eNr < mEdges.size());
			QuadricEdge& e = mEdges[eNr];
			PX_ASSERT(e.vertexNr[0] == i || e.vertexNr[1] == i);
			PX_ASSERT(!e.deleted);
			PX_UNUSED(e);
		}

		for (uint32_t j = 0; j < (uint32_t)v->mTriangles.size(); j++)
		{
			uint32_t tNr = v->mTriangles[j];
			PX_ASSERT(tNr < mTriangles.size());
			QuadricTriangle& t = mTriangles[tNr];
			PX_ASSERT(t.vertexNr[0] == i || t.vertexNr[1] == i || t.vertexNr[2] == i);
			PX_ASSERT(!t.deleted);
			PX_UNUSED(t);
		}
	}

	for (uint32_t i = 0; i < mEdges.size(); i++)
	{
		QuadricEdge& e = mEdges[i];
		if (e.deleted)
		{
			continue;
		}

		for (uint32_t j = 0; j < 2; j++)
		{
			uint32_t vNr = e.vertexNr[j];
			PX_ASSERT(vNr < mVertices.size());
			QuadricVertex* v = mVertices[vNr];
			PX_ASSERT(v->bDeleted == 0);
			uint32_t found = 0;
			for (uint32_t k = 0; k < v->mEdges.size(); k++)
			{
				found += (v->mEdges[k] == i) ? 1 : 0;
			}

			PX_ASSERT(found == 1);
		}
	}
	for (uint32_t i = 0; i < mTriangles.size(); i++)
	{
		QuadricTriangle& t = mTriangles[i];
		if (t.deleted)
		{
			continue;
		}

		for (uint32_t j = 0; j < 3; j++)
		{
			uint32_t vNr = t.vertexNr[j];
			PX_ASSERT(vNr < mVertices.size());
			QuadricVertex* v = mVertices[vNr];
			PX_ASSERT(v->bDeleted == 0);
			uint32_t found = 0;
			for (uint32_t k = 0; k < v->mTriangles.size(); k++)
			{
				found += (v->mTriangles[k] == i) ? 1 : 0;
			}

			PX_ASSERT(found == 1);
		}
	}
}




// --- Quadric Vertex -----------------------------------------

void ApexQuadricSimplifier::QuadricVertex::removeEdge(int32_t edgeNr)
{
	for (int32_t i = (int32_t)mEdges.size() - 1; i >= 0; i--)
	{
		if (mEdges[(uint32_t)i] == (uint32_t) edgeNr)
		{
			mEdges.replaceWithLast((uint32_t)i);
		}
	}
}



void ApexQuadricSimplifier::QuadricVertex::addTriangle(int32_t triangleNr)
{
	for (uint32_t i = 0; i < mTriangles.size(); i++)
	{
		if (mTriangles[i] == (uint32_t) triangleNr)
		{
			return;
		}
	}
	mTriangles.pushBack((uint32_t)triangleNr);
}



void ApexQuadricSimplifier::QuadricVertex::removeTriangle(int32_t triangleNr)
{
	uint32_t found = 0;
	for (int32_t i = (int32_t)mTriangles.size() - 1; i >= 0; i--)
	{
		if (mTriangles[(uint32_t)i] == (uint32_t) triangleNr)
		{
			mTriangles.replaceWithLast((uint32_t)i);
			found++;
		}
	}
	PX_ASSERT(found == 1);
}

}
} // end namespace nvidia::apex

