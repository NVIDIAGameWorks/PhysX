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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.

#include "SplitMesh.h"
#include "FloatMath.h"
#include "PsArray.h"
#include "foundation/PxVec3.h"
#include "foundation/PxQuat.h"
#include "foundation/PxMat44.h"

using namespace physx::general_floatmath2;
using namespace physx::shdfnd;
using namespace physx;

#pragma warning(disable:4100)

namespace SPLIT_MESH
{

static void addTri(const PxF32 *p1,
			  const PxF32 *p2,
			  const PxF32 *p3,
			  Array< PxU32 > &indices,
			  fm_VertexIndex *vertices)
{
	bool newPos;

	PxU32 i1 = vertices->getIndex(p1,newPos);
	PxU32 i2 = vertices->getIndex(p2,newPos);
	PxU32 i3 = vertices->getIndex(p3,newPos);

	indices.pushBack(i1);
	indices.pushBack(i2);
	indices.pushBack(i3);
}

PX_INLINE void rotationArc(const PxVec3 &v0,const PxVec3 &v1,PxQuat &quat)
{
	PxVec3 cross = v0.cross(v1);
	PxF32 d = v0.dot(v1);

	if(d<=-1.0f) // 180 about x axis
	{
		quat.x = 1.0f;
		quat.y = quat.z = quat.w =0.0f;
		return;
	}

	PxF32 s = PxSqrt((1+d)*2);
	PxF32 recip = 1.0f / s;

	quat.x = cross.x * recip;
	quat.y = cross.y * recip;
	quat.z = cross.z * recip;
	quat.w = s * 0.5f;

}

void computePlaneQuad(const PxF32 *planeEquation,physx::PxVec3 *quad)
{
	PxVec3 ref(0,1,0);
	PxQuat quat;
	PxVec3 normal(planeEquation[0],planeEquation[1],planeEquation[2]);
	rotationArc(ref,normal,quat);
	PxMat44 matrix(quat);

	PxVec3 origin(0,-planeEquation[3],0);
	PxVec3 center = matrix.transform(origin);
#define PLANE_DIST 1000
	PxVec3 upperLeft(-PLANE_DIST,0,-PLANE_DIST);
	PxVec3 upperRight(PLANE_DIST,0,-PLANE_DIST);
	PxVec3 lowerRight(PLANE_DIST,0,PLANE_DIST);
	PxVec3 lowerLeft(-PLANE_DIST,0,PLANE_DIST);

	quad[0] = matrix.transform(upperLeft);
	quad[1] = matrix.transform(upperRight);
	quad[2] = matrix.transform(lowerRight);
	quad[3] = matrix.transform(lowerLeft);
}

void splitMesh(const PxF32 *planeEquation,const SimpleMesh &input,SimpleMesh &leftMesh,SimpleMesh &rightMesh,bool /*closedMesh*/)
{
	Array< PxU32 > leftIndices;
	Array< PxU32 > rightIndices;

	fm_VertexIndex *leftVertices = fm_createVertexIndex(0.00001f,false);
	fm_VertexIndex *rightVertices = fm_createVertexIndex(0.00001f,false);

	{
		for (PxU32 i=0; i<input.mTcount; i++)
		{
			PxU32 i1 = input.mIndices[i*3+0];
			PxU32 i2 = input.mIndices[i*3+1];
			PxU32 i3 = input.mIndices[i*3+2];

			PxF32 *p1 = &input.mVertices[i1*3];
			PxF32 *p2 = &input.mVertices[i2*3];
			PxF32 *p3 = &input.mVertices[i3*3];

			PxF32 tri[3*3];

			tri[0] = p1[0];
			tri[1] = p1[1];
			tri[2] = p1[2];

			tri[3] = p2[0];
			tri[4] = p2[1];
			tri[5] = p2[2];

			tri[6] = p3[0];
			tri[7] = p3[1];
			tri[8] = p3[2];

			PxF32	front[3*5];
			PxF32	back[3*5];

			PxU32 fcount,bcount;

			PlaneTriResult result = fm_planeTriIntersection(planeEquation,tri,sizeof(PxF32)*3,0.00001f,front,fcount,back,bcount);

			switch ( result )
			{
				case PTR_FRONT:
					addTri(p1,p2,p3,leftIndices,leftVertices);
					break;
				case PTR_BACK:
					addTri(p1,p2,p3,rightIndices,rightVertices);
					break;
				case PTR_SPLIT:
					if ( fcount )
					{
						addTri(&front[0],&front[3],&front[6],leftIndices,leftVertices);
						if ( fcount == 4 )
						{
							addTri(&front[0],&front[6],&front[9],leftIndices,leftVertices);
						}
					}
					if ( bcount )
					{
						addTri(&back[0],&back[3],&back[6],rightIndices,rightVertices);
						if ( bcount == 4 )
						{
							addTri(&back[0],&back[6],&back[9],rightIndices,rightVertices);
						}
					}
					break;
				case PTR_ON_PLANE: // Make compiler happy
					break;
			}
		}
	}

	if ( !leftIndices.empty() )
	{
		leftMesh.set(leftVertices->getVcount(),leftIndices.size()/3,leftVertices->getVerticesFloat(),&leftIndices[0]);
	}

	if ( !rightIndices.empty() )
	{
		rightMesh.set(rightVertices->getVcount(),rightIndices.size()/3,rightVertices->getVerticesFloat(),&rightIndices[0]);
	}
	fm_releaseVertexIndex(leftVertices);
	fm_releaseVertexIndex(rightVertices);
}

};
