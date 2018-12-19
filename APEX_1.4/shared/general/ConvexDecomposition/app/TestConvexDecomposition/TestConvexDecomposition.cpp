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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <conio.h>

#pragma warning(disable:4996 4100)

#include "ConvexDecomposition.h"
#include "wavefront.h"
#include "PxAllocatorCallback.h"
#include "PxErrorCallback.h"
#include "PsShare.h"
#include "PxFoundation.h"
#include "PsFoundation.h"

namespace CONVEX_DECOMPOSITION
{
	ConvexDecomposition *gConvexDecomposition = NULL;
};

float getFloatArg(int arg,int argc,const char **argv)
{
	float ret = 0;
	if ( arg < argc )
	{
		ret = (float)atof(argv[arg]);
	}
	else
	{
		printf("Error: Missing input argument value at argument location %d.\r\n",arg+1);
	}
	return ret;
}

int getIntArg(int arg,int argc,const char **argv)
{
	int ret = 0;
	if ( arg < argc )
	{
		ret = atoi(argv[arg]);
	}
	else
	{
		printf("Error: Missing input argument value at argument location %d.\r\n",arg+1);
	}
	return ret;
}

class AppUserAllocator : public physx::PxAllocatorCallback
{
public:

	virtual void* allocate(size_t size, const char * , const char* , int ) 
	{
#ifdef PX_WINDOWS
		return ::_aligned_malloc(size, 16);
#else
		return ::malloc(size);
#endif
	}

	// this method needs to be removed
	virtual void* allocate(size_t size, physx::PxU32 , const char* , int ) 
	{
#ifdef PX_WINDOWS
		return ::_aligned_malloc(size, 16);
#else
		return ::malloc(size);
#endif
	}

	virtual void deallocate(void* ptr) 
	{
#ifdef PX_WINDOWS
		::_aligned_free(ptr);
#else
		::free(ptr);
#endif
	}

};

class AppUserOutputStream : public physx::PxErrorCallback
{
public:
	virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) 
	{

	}

};


static AppUserAllocator appAlloc;
static AppUserOutputStream appOutputStream;

void main(int argc,const char ** argv)
{
	PX_UNUSED(argc);
	PX_UNUSED(argv);

	physx::Foundation::createInstance(PX_PUBLIC_FOUNDATION_VERSION, appOutputStream, appAlloc);


	if ( argc == 1 )
	{
		printf("Usage: TestConvexDecomposition <wavefront.obj> (options)\r\n");
		printf("\r\n");
		printf("Options:\r\n");
		printf("\r\n");
		printf("-split				: Tests mesh splitting.\r\n");
		printf("-plane A B C D		: Specifies the plane equation to split the mesh by, if testing split only.\r\n");
		printf("-closed				: Indicates that the mesh should be closed when it is split.  Off by default; experimental.\r\n");
		printf("-depth <value>		: Specify the convex decomposition depth.\r\n");
		printf("-merge <value>		: Specify the merge threshold percentage.  Reasonable ranges 0-10.\r\n");
		printf("-concavity <value>	: Specify the concavity threshold as a percentage reasonable ranges 0-10.\r\n");
		printf("-hacd <concavity> <minCluster> : Use HACD (recommended).  Specify the concavity value for HACD (default 100) and the mininum number of clusters (hulls)\r\n");
		printf("-connect : Is using HACD; this specifies the connectivity distance.\r\n");
		printf("-maxhullverts <v>	: Specify the maxmium number of vertices in the output convex hulls.\r\n");
	}
	else
	{
		const char *wavefront = argv[1];

		bool			splitOnly = false;
		bool			closed = false;
		bool			useHACD = false;
		physx::PxF32	hacdConcavity = 100;
		physx::PxU32	hacdMinClusterSize = 10;
		physx::PxF32	connectionDistance = 0;

		physx::PxF32 plane[4] = { 1, 0, 0, 0 };
		physx::PxU32 depth = 5;
		physx::PxF32 mergePercentage = 3; //1;
		physx::PxF32 concavityPercentage = 3; //;
		physx::PxU32 maxHullVerts = 64;

		int scan = 2;
		while ( scan < argc )
		{
			const char *option = argv[scan];
			if ( strcmp(option,"-split") == 0 )
			{
				splitOnly = true;
				printf("Testing a single split operation.\r\n");
				scan++;
			}
			else if ( strcmp(option,"-plane") == 0 )
			{
				plane[0] = getFloatArg(scan+1,argc,argv);
				plane[1] = getFloatArg(scan+2,argc,argv);
				plane[2] = getFloatArg(scan+3,argc,argv);
				plane[3] = getFloatArg(scan+4,argc,argv);
				scan+=5;
			}
			else if ( strcmp(option,"-closed") == 0 )
			{
				closed = true;
				printf("Will produce closed split meshes.\r\n");
				scan++;
			}
			else if ( strcmp(option,"-depth") == 0 )
			{
				depth = getIntArg(scan+1,argc,argv);
				scan+=2;
			}
			else if ( strcmp(option,"-connect") == 0 )
			{
				connectionDistance = getFloatArg(scan+1,argc,argv);
				scan+=2;
			}
			else if ( strcmp(option,"-merge") == 0 )
			{
				mergePercentage = getFloatArg(scan+1,argc,argv);
				scan+=2;
			}
			else if ( strcmp(option,"-concavity") == 0 )
			{
				concavityPercentage = getFloatArg(scan+1,argc,argv);
				scan+=2;
			}
			else if ( strcmp(option,"-maxhullverts") == 0 )
			{
				maxHullVerts = getIntArg(scan+1,argc,argv);
				scan+=2;
			}
			else if ( strcmp(option,"-hacd")== 0)
			{
				useHACD = true;
				hacdConcavity = getFloatArg(scan+1,argc,argv);
				hacdMinClusterSize = getIntArg(scan+2,argc,argv);
				scan+=3;
			}
			else
			{
				printf("Unknown option: %s\r\n", option );
				scan++;
			}

		}

		CONVEX_DECOMPOSITION::gConvexDecomposition = CONVEX_DECOMPOSITION::createConvexDecomposition();
		if  ( CONVEX_DECOMPOSITION::gConvexDecomposition )
		{
			WavefrontObj obj;
			unsigned int tcount = obj.loadObj(wavefront,false);
			if ( tcount )
			{
				if ( splitOnly )
				{
					CONVEX_DECOMPOSITION::TriangleMesh input;
					CONVEX_DECOMPOSITION::TriangleMesh left;
					CONVEX_DECOMPOSITION::TriangleMesh right;
					input.mVcount = obj.mVertexCount;
					input.mVertices = obj.mVertices;
					input.mTriCount = obj.mTriCount;
					input.mIndices = (physx::PxU32 *)obj.mIndices;
					CONVEX_DECOMPOSITION::gConvexDecomposition->splitMesh(plane,input,left,right,closed);

					if ( left.mTriCount )
					{
						printf("Left Half of the split mesh has %d triangles.  Saving as 'left.obj'\r\n", left.mTriCount);
						WavefrontObj::saveObj("left.obj", left.mVcount, left.mVertices, left.mTriCount,(const int *) left.mIndices );
					}
					else
					{
						printf("No triangles on the left half of the split mesh.\r\n");
					}
					if ( right.mTriCount )
					{
						printf("Left Half of the split mesh has %d triangles.  Saving as 'right.obj'\r\n", right.mTriCount);
						WavefrontObj::saveObj("right.obj", right.mVcount, right.mVertices, right.mTriCount, (const int *)right.mIndices );
					}
					else
					{
						printf("No triangles on the right half of the split mesh.\r\n");
					}

					CONVEX_DECOMPOSITION::gConvexDecomposition->releaseTriangleMeshMemory(left);
					CONVEX_DECOMPOSITION::gConvexDecomposition->releaseTriangleMeshMemory(right);
				}
				else
				{
					CONVEX_DECOMPOSITION::DecompDesc desc;
					desc.mClosedSplit = closed;
					desc.mDepth		= depth;
					desc.mCpercent	= concavityPercentage;
					desc.mPpercent	= mergePercentage;
					desc.mTcount	= obj.mTriCount;
					desc.mVcount	= obj.mVertexCount;
					desc.mVertices	= obj.mVertices;
					desc.mIndices	= (physx::PxU32 *)obj.mIndices;
					desc.mUseHACD	= useHACD;
					desc.mConcavityHACD = hacdConcavity;
					desc.mMinClusterSizeHACD = hacdMinClusterSize;
					desc.mConnectionDistanceHACD = connectionDistance;


					printf("Performing convex decomposition.\r\n");

					physx::PxU32 count = CONVEX_DECOMPOSITION::gConvexDecomposition->performConvexDecomposition(desc);
					if ( count )
					{
						printf("Produced %d output convex hulls.\r\n", count );
						FILE *fph = fopen("ConvexDecomposition.obj", "wb");
						if ( fph )
						{
							fprintf(fph,"# Input mesh '%s' produced %d convex hulls.\r\n", wavefront, count );
							physx::PxU32 *baseVertex = new physx::PxU32[count];
							physx::PxU32 vertexCount = 0;
							for (physx::PxU32 i=0; i<count; i++)
							{
								baseVertex[i] = vertexCount;
								const CONVEX_DECOMPOSITION::ConvexResult &r = *CONVEX_DECOMPOSITION::gConvexDecomposition->getConvexResult(i);
								fprintf(fph,"## Hull %d has %d vertices.\r\n", i+1, r.mHullVcount );
								for (physx::PxU32 i=0; i<r.mHullVcount; i++)
								{
									const physx::PxF32 *p = &r.mHullVertices[i*3];
									fprintf(fph,"v %0.9f %0.9f %0.9f\r\n", p[0], p[1], p[2] );
								}
								vertexCount+=r.mHullVcount;
							}

							for (physx::PxU32 i=0; i<count; i++)
							{
								const CONVEX_DECOMPOSITION::ConvexResult &r = *CONVEX_DECOMPOSITION::gConvexDecomposition->getConvexResult(i);
								physx::PxU32 startVertex = baseVertex[i];
								fprintf(fph,"# Convex Hull %d contains %d triangles and %d vertices.  Starting vertex index is: %d It has a volume of: %0.9f\r\n", i+1, r.mHullTcount, r.mHullVcount, startVertex, r.mHullVolume );
								for (physx::PxU32 j=0; j<r.mHullTcount; j++)
								{
									physx::PxU32 i1 = r.mHullIndices[j*3+0]+startVertex+1;
									physx::PxU32 i2 = r.mHullIndices[j*3+1]+startVertex+1;
									physx::PxU32 i3 = r.mHullIndices[j*3+2]+startVertex+1;
									fprintf(fph,"f %d %d %d\r\n", i1, i2, i3 );
								}
							}
							fclose(fph);
							delete []baseVertex;
						}
						else
						{
							printf("Failed to open file 'ConvexDecomposition.obj' for output.\r\n");
						}
					}
					else
					{
						printf("Failed to produce any convex hull results!?\r\n");
					}

				}
			}
			else
			{
				printf("Failed to load Wavefront OBJ file '%s'\r\n",wavefront);
			}
		}
		else
		{
			printf("Failed to load the convex decomposition plugin DLL.\r\n");
		}
	}
}
