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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "Planet.h"
#include "RendererMemoryMacros.h"
#include "foundation/PxPlane.h"
#include "foundation/PxAssert.h"
#include "PxTkRandom.h"

PlanetMesh::PlanetMesh() :
	mNbVerts	(0),
	mNbTris		(0),
	mVerts		(NULL),
	mIndices	(NULL)
{
}

PlanetMesh::~PlanetMesh()
{
	DELETEARRAY(mIndices);
	DELETEARRAY(mVerts);
}

PxU8 PlanetMesh::checkCulling(const PxVec3& center, PxU32 index0, PxU32 index1, PxU32 index2) const
{
	const PxPlane tmp(mVerts[index0], mVerts[index1], mVerts[index2]);
	return tmp.distance(center)>0.0f;
}

void PlanetMesh::generate(const PxVec3& center, PxF32 radius, PxU32 nbX, PxU32 nbY)
{
	DELETEARRAY(mIndices);
	DELETEARRAY(mVerts);

	mNbVerts = 6*nbX*nbY;
	mVerts = new PxVec3[mNbVerts];
	PxU32 index=0;
	for(PxU32 i=0;i<6;i++)
	{
		for(PxU32 y=0;y<nbY;y++)
		{
			const PxF32 coeffY = float(y)/float(nbY-1);
			for(PxU32 x=0;x<nbX;x++)
			{
				const PxF32 coeffX = float(x)/float(nbX-1);
				const PxF32 xr = coeffX-0.5f;
				const PxF32 yr = coeffY-0.5f;

				PxVec3 offset;
				if(i==0)
				{
					offset = PxVec3(xr, yr, 0.5f);
				}
				else if(i==1)
				{
					offset = PxVec3(xr, yr, -0.5f);
				}
				else if(i==2)
				{
					offset = PxVec3(xr, 0.5f, yr);
				}
				else if(i==3)
				{
					offset = PxVec3(xr, -0.5f, yr);
				}
				else if(i==4)
				{
					offset = PxVec3(0.5f, xr, yr);
				}
				else
				{
					offset = PxVec3(-0.5f, xr, yr);
				}

				offset.normalize();

				const float h = sinf(offset.z*10.0f)*sinf(offset.x*10.0f)*sinf(offset.y*10.0f)*2.0f;

				offset *= (radius + h);

				mVerts[index++] = center + offset;
			}
		}
	}
	PX_ASSERT(index==mNbVerts);

	mNbTris = 6*(nbX-1)*(nbY-1)*2;
	mIndices = new PxU32[mNbTris*3];
	PxU32 offset = 0;
	PxU32 baseOffset = 0;
	for(PxU32 i=0;i<6;i++)
	{
		for(PxU32 y=0;y<nbY-1;y++)
		{
			for(PxU32 x=0;x<nbX-1;x++)
			{
				const PxU32 index0a = baseOffset + x;
				const PxU32 index1a = baseOffset + x + nbX;
				const PxU32 index2a = baseOffset + x + nbX + 1;

				const PxU32 index0b = index0a;
				const PxU32 index1b = index2a;
				const PxU32 index2b = baseOffset + x + 1;

				{
					const PxU8 c = checkCulling(center, index0a, index1a, index2a);
					mIndices[offset]		= index0a;
					mIndices[offset+1+c]	= index1a;
					mIndices[offset+2-c]	= index2a;
					offset+=3;
				}
				{
					const PxU8 c = checkCulling(center, index0b, index1b, index2b);
					mIndices[offset]		= index0b;
					mIndices[offset+1+c]	= index1b;
					mIndices[offset+2-c]	= index2b;
					offset+=3;
				}
			}
			baseOffset += nbX;
		}
		baseOffset += nbX;
	}
	PX_ASSERT(offset==mNbTris*3);
}