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



#include "ApexSubdivider.h"

#include "ApexSDKIntl.h"

#include "PsSort.h"

namespace nvidia
{
namespace apex
{

ApexSubdivider::ApexSubdivider() :
	mMarkedVertices(0),
	mTriangleListEmptyElement(-1)
{
	mBound.setEmpty();
	mRand.setSeed(0);
}




void ApexSubdivider::clear()
{
	mBound.setEmpty();
	mRand.setSeed(0);
}



void ApexSubdivider::registerVertex(const PxVec3& v, uint32_t bitFlagPayload)
{
	SubdividerVertex vertex(v, bitFlagPayload);
	mVertices.pushBack(vertex);
	mBound.include(v);
}



void ApexSubdivider::registerTriangle(uint32_t i0, uint32_t i1, uint32_t i2)
{
	PX_ASSERT(i0 < mVertices.size());
	PX_ASSERT(i1 < mVertices.size());
	PX_ASSERT(i2 < mVertices.size());

	SubdividerTriangle t;
	t.init(i0, i1, i2);
	uint32_t triangleNumber = mTriangles.size();
	mTriangles.pushBack(t);
	addTriangleToVertex(i0, triangleNumber);
	addTriangleToVertex(i1, triangleNumber);
	addTriangleToVertex(i2, triangleNumber);
}



void ApexSubdivider::endRegistration()
{

}



void ApexSubdivider::mergeVertices(IProgressListener* progress)
{
	PX_UNUSED(progress);
	if (mVertices.empty())
	{
		APEX_INVALID_OPERATION("no vertices available");
		return;
	}

	const float MERGE_THRESHOLD = 1.0e-6f;

	const float d = (mBound.minimum - mBound.maximum).magnitude() * MERGE_THRESHOLD;
	const float d2 = d * d;

	Array<SubdividerVertexRef> refs;
	refs.reserve(mVertices.size());

	for (uint32_t i = 0; i < mVertices.size(); i++)
	{
		SubdividerVertex& v = mVertices[i];
		v.marked = false;
		SubdividerVertexRef vr(v.pos, i);
		refs.pushBack(vr);
	}
	mMarkedVertices = 0;

	shdfnd::sort(refs.begin(), refs.size(), SubdividerVertexRef());

	for (uint32_t i = 0; i < refs.size() - 1; i++)
	{
		uint32_t iNr = refs[i].vertexNr;
		SubdividerVertex& vi = mVertices[iNr];

		if (vi.marked)
		{
			continue;
		}

		const PxVec3 pos = refs[i].pos;
		uint32_t j = i + 1;
		while (j < refs.size() && fabs(refs[j].pos.x - pos.x) < MERGE_THRESHOLD)
		{
			if ((refs[j].pos - pos).magnitudeSquared() < d2)
			{
				uint32_t jNr = refs[j].vertexNr;
				SubdividerVertex& vj = mVertices[jNr];

				const uint32_t payload = vi.payload | vj.payload;
				vi.payload = vj.payload = payload;

				if (vj.firstTriangle != -1)
				{
					// find vi's last triangle, add the others there
					int32_t lastTri = vi.firstTriangle;

					for (;;)
					{
						TriangleList& t = mTriangleList[(uint32_t)lastTri];
						if (t.nextTriangle == -1)
						{
							break;
						}

						PX_ASSERT(t.triangleNumber < mTriangles.size());

						lastTri = t.nextTriangle;
					}
					PX_ASSERT(lastTri != -1);
					PX_ASSERT((uint32_t)lastTri < mTriangleList.size());
					PX_ASSERT(mTriangleList[(uint32_t)lastTri].nextTriangle == -1);
					mTriangleList[(uint32_t)lastTri].nextTriangle = vj.firstTriangle;
					vj.firstTriangle = -1;
					lastTri = mTriangleList[(uint32_t)lastTri].nextTriangle;
					while (lastTri != -1)
					{
						uint32_t tNr = mTriangleList[(uint32_t)lastTri].triangleNumber;
						PX_ASSERT(tNr < mTriangles.size());
						mTriangles[tNr].replaceVertex(refs[j].vertexNr, refs[i].vertexNr);

						lastTri = mTriangleList[(uint32_t)lastTri].nextTriangle;
					}
				}
				vj.marked = true;
				mMarkedVertices++;
			}
			j++;
		}
	}

	if (mMarkedVertices > 0)
	{
		compress();
	}
}



void ApexSubdivider::closeMesh(IProgressListener* progress)
{
	PX_UNUSED(progress);
	Array<SubdividerEdge> edges, borderEdges;
	SubdividerEdge edge;

	edges.reserve(mTriangles.size() * 3);

	for (uint32_t i = 0; i < mTriangles.size(); i++)
	{
		SubdividerTriangle& t = mTriangles[i];
		edge.init(t.vertexNr[0], t.vertexNr[1], i);
		edges.pushBack(edge);
		edge.init(t.vertexNr[1], t.vertexNr[2], i);
		edges.pushBack(edge);
		edge.init(t.vertexNr[2], t.vertexNr[0], i);
		edges.pushBack(edge);
	}

	shdfnd::sort(edges.begin(), edges.size(), SubdividerEdge());

	for (uint32_t i = 0; i < edges.size(); i++)
	{
		SubdividerEdge& ei = edges[i];
		uint32_t j = i + 1;
		while (j < edges.size() && edges[j] == ei)
		{
			j++;
		}
		if (j == i + 1)
		{
			borderEdges.pushBack(ei);
		}
		i = j - 1;
	}

	// find border circles
	Array<uint32_t> borderVertices;
	borderVertices.reserve(borderEdges.size());
	while (!borderEdges.empty())
	{
		edge = borderEdges.back();
		borderEdges.popBack();

		borderVertices.clear();
		// find orientation

		const SubdividerTriangle& triangle = mTriangles[edge.triangleNr];
		PX_ASSERT(triangle.containsVertex(edge.v0));
		PX_ASSERT(triangle.containsVertex(edge.v1));
		bool swap = triangle.vertexNr[0] == edge.v0 || (triangle.vertexNr[1] == edge.v0 && triangle.vertexNr[0] != edge.v1);

		if (swap)
		{
			borderVertices.pushBack(edge.v1);
			borderVertices.pushBack(edge.v0);
		}
		else
		{
			borderVertices.pushBack(edge.v0);
			borderVertices.pushBack(edge.v1);
		}
		uint32_t currentV = borderVertices.back();
		int32_t nextV = -1;
		do
		{
			nextV = -1;
			uint32_t i = 0;
			for (; i < borderEdges.size(); i++)
			{
				SubdividerEdge& ei = borderEdges[i];
				if (ei.v0 == currentV)
				{
					nextV = (int32_t)ei.v1;
					break;
				}
				else if (ei.v1 == currentV)
				{
					nextV = (int32_t)ei.v0;
					break;
				}
			}
			if (nextV < 0)
			{
				break;    // chain ended
			}

			PX_ASSERT(i < borderEdges.size());
			borderEdges.replaceWithLast(i);
			borderVertices.pushBack((uint32_t)nextV);
			currentV = (uint32_t)nextV;
		}
		while (nextV >= 0);

		if (borderVertices[0] == borderVertices[borderVertices.size() - 1])
		{
			borderVertices.popBack();
		}

		closeHole(borderVertices.begin(), borderVertices.size());
	}
}



void ApexSubdivider::subdivide(uint32_t subdivisionGridSize, IProgressListener*)
{
	PX_ASSERT(subdivisionGridSize > 0);
	if (subdivisionGridSize == 0)
	{
		return;
	}

	const float maxLength = (mBound.minimum - mBound.maximum).magnitude() / (float)subdivisionGridSize;
	const float threshold = 2.0f * maxLength;
	const float threshold2 = threshold * threshold;

	PX_ASSERT(threshold2 > 0.0f);
	if (threshold2 <= 0.0f)
	{
		return;
	}

	Array<SubdividerEdge> edges;
	edges.reserve(mTriangles.size() * 3);
	for (uint32_t i = 0; i < mTriangles.size(); i++)
	{
		SubdividerEdge edge;
		SubdividerTriangle& t = mTriangles[i];
		edge.init(t.vertexNr[0], t.vertexNr[1], i);
		edges.pushBack(edge);
		edge.init(t.vertexNr[1], t.vertexNr[2], i);
		edges.pushBack(edge);
		edge.init(t.vertexNr[2], t.vertexNr[0], i);
		edges.pushBack(edge);
	}

	shdfnd::sort(edges.begin(), edges.size(), SubdividerEdge());

	uint32_t i = 0;
	while (i < edges.size())
	{
		SubdividerEdge& ei = edges[i];
		uint32_t newI = i + 1;
		while (newI < edges.size() && edges[newI] == ei)
		{
			newI++;
		}

		const PxVec3 p0 = mVertices[ei.v0].pos;
		const PxVec3 p1 = mVertices[ei.v1].pos;
		const float d2 = (p0 - p1).magnitudeSquared();

		if (d2 < threshold2)
		{
			i = newI;
			continue;
		}

		uint32_t newVertex = mVertices.size();
		const float eps = 1.0e-4f;
		SubdividerVertex vertex;
		vertex.pos = (p0 + p1) * 0.5f + PxVec3(mRand.getNext(), mRand.getNext(), mRand.getNext()) * eps;
		vertex.payload = mVertices[ei.v0].payload | mVertices[ei.v1].payload;
		mVertices.pushBack(vertex);

		uint32_t newEdgeNr = edges.size();
		for (uint32_t j = i; j < newI; j++)
		{
			SubdividerEdge ej = edges[j];
			SubdividerTriangle tj = mTriangles[ej.triangleNr];

			uint32_t v2 = 0; // the vertex not contained in the edge
			if (tj.vertexNr[1] != ej.v0 && tj.vertexNr[1] != ej.v1)
			{
				v2 = 1;
			}
			else if (tj.vertexNr[2] != ej.v0 && tj.vertexNr[2] != ej.v1)
			{
				v2 = 2;
			}

			const uint32_t v0 = (v2 + 1) % 3;
			const uint32_t v1 = (v0 + 1) % 3;

			// generate new triangle
			const uint32_t newTriangle = mTriangles.size();

			SubdividerTriangle tNew;
			tNew.init(tj.vertexNr[v0], newVertex, tj.vertexNr[v2]);
			mTriangles.pushBack(tNew);
			addTriangleToVertex(tNew.vertexNr[0], newTriangle);
			addTriangleToVertex(tNew.vertexNr[1], newTriangle);
			addTriangleToVertex(tNew.vertexNr[2], newTriangle);

			// modify existing triangle
			removeTriangleFromVertex(tj.vertexNr[v0], ej.triangleNr);
			tj.vertexNr[v0] = newVertex;
			mTriangles[ej.triangleNr].vertexNr[v0] = newVertex;
			addTriangleToVertex(newVertex, ej.triangleNr);

			// update edges
			int32_t k = binarySearchEdges(edges, tNew.vertexNr[2], tNew.vertexNr[0], ej.triangleNr);
			PX_ASSERT(k >= 0);
			edges[(uint32_t)k].triangleNr = newTriangle;

			SubdividerEdge edge;
			edge.init(tj.vertexNr[v2], tj.vertexNr[v0], ej.triangleNr);
			edges.pushBack(edge);
			edge.init(tj.vertexNr[v0], tj.vertexNr[v1], ej.triangleNr);
			edges.pushBack(edge);
			edge.init(tNew.vertexNr[0], tNew.vertexNr[1], newTriangle);
			edges.pushBack(edge);
			edge.init(tNew.vertexNr[1], tNew.vertexNr[2], newTriangle);
			edges.pushBack(edge);
		}
		i = newI;
		PX_ASSERT(newEdgeNr < edges.size());

		shdfnd::sort(edges.begin() + newEdgeNr, edges.size() - newEdgeNr, SubdividerEdge());
	}
}



uint32_t ApexSubdivider::getNumVertices() const
{
	PX_ASSERT(mMarkedVertices == 0);
	return mVertices.size();
}



uint32_t ApexSubdivider::getNumTriangles() const
{
	return mTriangles.size();
}



void ApexSubdivider::getVertex(uint32_t i, PxVec3& v, uint32_t& bitFlagPayload) const
{
	PX_ASSERT(mMarkedVertices == 0);
	PX_ASSERT(i < mVertices.size());
	v = mVertices[i].pos;
	bitFlagPayload = mVertices[i].payload;
}



void ApexSubdivider::getTriangle(uint32_t i, uint32_t& i0, uint32_t& i1, uint32_t& i2) const
{
	PX_ASSERT(i < mTriangles.size());
	PX_ASSERT(mTriangles[i].isValid());

	const SubdividerTriangle& t = mTriangles[i];
	i0 = t.vertexNr[0];
	i1 = t.vertexNr[1];
	i2 = t.vertexNr[2];
}



void ApexSubdivider::compress()
{
	Array<uint32_t> oldToNew;
	oldToNew.resize(mVertices.size());
	Array<SubdividerVertex> newVertices;
	newVertices.reserve(mVertices.size() - mMarkedVertices);

	for (uint32_t i = 0; i < mVertices.size(); i++)
	{
		if (mVertices[i].marked)
		{
			oldToNew[i] = (uint32_t) - 1;
			mMarkedVertices--;
		}
		else
		{
			oldToNew[i] = newVertices.size();
			newVertices.pushBack(mVertices[i]);
		}
	}

	mVertices.resize(newVertices.size());
	for (uint32_t i = 0; i < newVertices.size(); i++)
	{
		mVertices[i] = newVertices[i];
	}

	for (uint32_t i = 0; i < mTriangles.size(); i++)
	{
		if (mTriangles[i].isValid())
		{
			SubdividerTriangle& t = mTriangles[i];
			t.vertexNr[0] = oldToNew[t.vertexNr[0]];
			t.vertexNr[1] = oldToNew[t.vertexNr[1]];
			t.vertexNr[2] = oldToNew[t.vertexNr[2]];
		}
		else
		{
			mTriangles.replaceWithLast(i);
			i--;
		}
	}

	PX_ASSERT(mMarkedVertices == 0);
}



void ApexSubdivider::closeHole(uint32_t* indices, uint32_t numIndices)
{
	if (numIndices < 3)
	{
		return;
	}

	SubdividerTriangle triangle;
	triangle.init(0, 0, 0);

	// fill hole
	while (numIndices > 3)
	{
		PxVec3 normal(0.0f);
		for (uint32_t i = 0; i < numIndices; i++)
		{
			const PxVec3& p0 = mVertices[indices[i]].pos;
			const PxVec3& p1 = mVertices[indices[(i + 1) % numIndices]].pos;
			const PxVec3& p2 = mVertices[indices[(i + 2) % numIndices]].pos;

			PxVec3 normalI = (p0 - p1).cross(p2 - p1);
			normalI.normalize();
			normal += normalI;
		}
		normal.normalize();

		float maxQuality = -1.0f;
		int32_t bestI = -1;
		for (uint32_t i = 0; i < numIndices; i++)
		{
			const uint32_t i2 = (i + 2) % numIndices;

			const uint32_t b0 = indices[i];
			const uint32_t b1 = indices[(i + 1) % numIndices];
			const uint32_t b2 = indices[i2];
			const uint32_t b3 = indices[(i + 3) % numIndices];
			const uint32_t b4 = indices[(i + 4) % numIndices];

			if (getTriangleNr(b1, b2, b3) != -1)
			{
				continue;
			}


			// init best
			//if (i == 0)
			//{
			//	t.init(b1,b2,b3);
			//	bestI = i2;
			//}

			// check whether triangle is an ear
			PxVec3 normalI = (mVertices[b1].pos - mVertices[b2].pos).cross(mVertices[b3].pos - mVertices[b2].pos);
			normalI.normalize(); ///< \todo, remove again, only for debugging
			if (normalI.dot(normal) < 0.0f)
			{
				continue;
			}

			float quality = qualityOfTriangle(b1, b2, b3) - qualityOfTriangle(b0, b1, b2) - qualityOfTriangle(b2, b3, b4);
			if (maxQuality < 0.0f || quality > maxQuality)
			{
				maxQuality = quality;
				triangle.init(b1, b2, b3);
				bestI = (int32_t)i2;
			}
		}
		PX_ASSERT(bestI != -1);
		PX_ASSERT(triangle.isValid());


		// remove ear vertex from temporary border
		for (uint32_t i = (uint32_t)bestI; i < numIndices - 1; i++)
		{
			indices[i] = indices[i + 1];
		}

		numIndices--;

		// do we have the triangle already?
		//if (getTriangleNr(triangle.vertexNr[0], triangle.vertexNr[1], triangle.vertexNr[2]) >= 0)
		//	continue;

		// TODO:   triangle is potentially uninitialized.
		// do we have to subdivide the triangle?
		//PxVec3& p0 = mVertices[triangle.vertexNr[0]].pos;
		//PxVec3& p2 = mVertices[triangle.vertexNr[2]].pos;
		//PxVec3 dir = p2 - p0;
		//float d = dir.normalize();
		uint32_t triangleNr = mTriangles.size();
		mTriangles.pushBack(triangle);
		addTriangleToVertex(triangle.vertexNr[0], triangleNr);
		addTriangleToVertex(triangle.vertexNr[1], triangleNr);
		addTriangleToVertex(triangle.vertexNr[2], triangleNr);
	}

	triangle.init(indices[0], indices[1], indices[2]);
	if (getTriangleNr(triangle.vertexNr[0], triangle.vertexNr[1], triangle.vertexNr[2]) < 0)
	{
		uint32_t triangleNr = mTriangles.size();
		mTriangles.pushBack(triangle);
		addTriangleToVertex(triangle.vertexNr[0], triangleNr);
		addTriangleToVertex(triangle.vertexNr[1], triangleNr);
		addTriangleToVertex(triangle.vertexNr[2], triangleNr);
	}
}



float ApexSubdivider::qualityOfTriangle(uint32_t v0, uint32_t v1, uint32_t v2) const
{
	const PxVec3& p0 = mVertices[v0].pos;
	const PxVec3& p1 = mVertices[v1].pos;
	const PxVec3& p2 = mVertices[v2].pos;

	const float a = (p0 - p1).magnitude();
	const float b = (p1 - p2).magnitude();
	const float c = (p2 - p0).magnitude();

	if (a > b && a > c) // a is biggest
	{
		return b + c - a;
	}
	else if (b > c)
	{
		return a + c - b;    // b is biggest
	}
	return a + b - c; // c is biggest
}



int32_t ApexSubdivider::getTriangleNr(const uint32_t v0, const uint32_t v1, const uint32_t v2) const
{
	const uint32_t num = mVertices.size();
	if (v0 >= num || v1 >= num || v2 >= num)
	{
		return -1;
	}

	int32_t triangleListIndex = mVertices[v0].firstTriangle;
	while (triangleListIndex != -1)
	{
		const TriangleList& tl = mTriangleList[(uint32_t)triangleListIndex];
		const uint32_t triangleIndex = tl.triangleNumber;
		const SubdividerTriangle& triangle = mTriangles[triangleIndex];
		if (triangle.containsVertex(v1) && triangle.containsVertex(v2))
		{
			return (int32_t)triangleIndex;
		}

		triangleListIndex = tl.nextTriangle;
	}

	return -1;
}



int32_t ApexSubdivider::binarySearchEdges(const Array<SubdividerEdge>& edges, uint32_t v0, uint32_t v1, uint32_t triangleNr) const
{
	if (edges.empty())
	{
		return -1;
	}

	SubdividerEdge edge;
	edge.init(v0, v1, (uint32_t) - 1);

	uint32_t l = 0;
	uint32_t r = edges.size() - 1;
	int32_t m = 0;
	while (l <= r)
	{
		m = (int32_t)(l + r) / 2;
		if (edges[(uint32_t)m] < edge)
		{
			l = (uint32_t)m + 1;
		}
		else if (edge < edges[(uint32_t)m])
		{
			r = (uint32_t)m - 1;
		}
		else
		{
			break;
		}
	}
	if (!(edges[(uint32_t)m] == edge))
	{
		return -1;
	}

	while (m >= 0 && edges[(uint32_t)m] == edge)
	{
		m--;
	}
	m++;

	PX_ASSERT(m >= 0);
	PX_ASSERT((uint32_t)m < edges.size());
	while ((uint32_t)m < edges.size() && edges[(uint32_t)m] == edge && edges[(uint32_t)m].triangleNr != triangleNr)
	{
		m++;
	}

	if (edges[(uint32_t)m] == edge && edges[(uint32_t)m].triangleNr == triangleNr)
	{
		return m;
	}

	return -1;
}



void ApexSubdivider::addTriangleToVertex(uint32_t vertexNumber, uint32_t triangleNumber)
{
	PX_ASSERT(vertexNumber < mVertices.size());
	PX_ASSERT(triangleNumber < mTriangles.size());

	TriangleList& t = allocateTriangleElement();

	t.triangleNumber = triangleNumber;
	t.nextTriangle = mVertices[vertexNumber].firstTriangle;

	mVertices[vertexNumber].firstTriangle = (int32_t)(&t - &mTriangleList[0]);
	//int a = 0;
}



void ApexSubdivider::removeTriangleFromVertex(uint32_t vertexNumber, uint32_t triangleNumber)
{
	PX_ASSERT(vertexNumber < mVertices.size());
	PX_ASSERT(triangleNumber < mTriangles.size());

	int32_t* lastPointer = &mVertices[vertexNumber].firstTriangle;
	int32_t triangleListIndex = *lastPointer;
	while (triangleListIndex != -1)
	{
		if (mTriangleList[(uint32_t)triangleListIndex].triangleNumber == triangleNumber)
		{
			*lastPointer = mTriangleList[(uint32_t)triangleListIndex].nextTriangle;

			freeTriangleElement((uint32_t)triangleListIndex);

			break;
		}
		lastPointer = &mTriangleList[(uint32_t)triangleListIndex].nextTriangle;
		triangleListIndex = *lastPointer;
	}
}



ApexSubdivider::TriangleList& ApexSubdivider::allocateTriangleElement()
{
	if (mTriangleListEmptyElement == -1)
	{
		return mTriangleList.insert();
	}
	else
	{
		PX_ASSERT((uint32_t)mTriangleListEmptyElement < mTriangleList.size());
		TriangleList& elem = mTriangleList[(uint32_t)mTriangleListEmptyElement];
		mTriangleListEmptyElement = elem.nextTriangle;
		elem.nextTriangle = -1;

		return elem;
	}
}



void ApexSubdivider::freeTriangleElement(uint32_t index)
{
	PX_ASSERT(index < mTriangleList.size());
	mTriangleList[index].nextTriangle = mTriangleListEmptyElement;
	mTriangleListEmptyElement = (int32_t)index;
}

}
} // end namespace nvidia::apex
