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

#include "SampleVehicle.h"
#include "SampleVehicle_SceneQuery.h"
#include "SampleRandomPrecomputed.h"
#include "SampleAllocatorSDKClasses.h"
#include "RendererMemoryMacros.h"
#include "RenderMaterial.h"
#include "RenderMeshActor.h"
#include "cooking/PxTriangleMeshDesc.h"
#include "geometry/PxHeightFieldGeometry.h"
#include "geometry/PxHeightFieldSample.h"
#include "geometry/PxHeightFieldDesc.h"
#include "cooking/PxCooking.h"
#include "PxScene.h"
#include "PxRigidStatic.h"
#include "PxTkStream.h"
#include "PxTkFile.h"

//using namespace physx;
using namespace PxToolkit;

//Use a mesh (instead of a height field)
static bool gRecook = false;

enum MaterialID
{
	MATERIAL_TERRAIN_MUD	 = 1000,
	MATERIAL_ROAD_TARMAC = 1001,
	MATERIAL_ROAD_SNOW	 = 1002,
	MATERIAL_ROAD_GRASS = 1003,
};

static void computeTerrain(bool* done, float* pVB, PxU32 x0, PxU32 y0, PxU32 currentSize, float value, PxU32 initSize, SampleRandomPrecomputed& randomPrecomputed)
{
	// Compute new size
	currentSize>>=1;
	if(currentSize > 0)
	{
		const PxU32 x1 = (x0+currentSize)			   % initSize;
		const PxU32 x2 = (x0+currentSize+currentSize) % initSize;
		const PxU32 y1 = (y0+currentSize)			   % initSize;
		const PxU32 y2 = (y0+currentSize+currentSize) % initSize;

		if(!done[x1 + y0*initSize])	pVB[(x1 + y0*initSize)*9+1] = randomPrecomputed.getRandomInRange(-0.5f*value, 0.5f*value) + 0.5f * (pVB[(x0 + y0*initSize)*9+1] + pVB[(x2 + y0*initSize)*9+1]);
		if(!done[x0 + y1*initSize])	pVB[(x0 + y1*initSize)*9+1] = randomPrecomputed.getRandomInRange(-0.5f*value, 0.5f*value) + 0.5f * (pVB[(x0 + y0*initSize)*9+1] + pVB[(x0 + y2*initSize)*9+1]);
		if(!done[x2 + y1*initSize])	pVB[(x2 + y1*initSize)*9+1] = randomPrecomputed.getRandomInRange(-0.5f*value, 0.5f*value) + 0.5f * (pVB[(x2 + y0*initSize)*9+1] + pVB[(x2 + y2*initSize)*9+1]);
		if(!done[x1 + y2*initSize])	pVB[(x1 + y2*initSize)*9+1] = randomPrecomputed.getRandomInRange(-0.5f*value, 0.5f*value) + 0.5f * (pVB[(x0 + y2*initSize)*9+1] + pVB[(x2 + y2*initSize)*9+1]);
		if(!done[x1 + y1*initSize])	pVB[(x1 + y1*initSize)*9+1] = randomPrecomputed.getRandomInRange(-0.5f*value, 0.5f*value) + 0.5f * (pVB[(x0 + y1*initSize)*9+1] + pVB[(x2 + y1*initSize)*9+1]);

		done[x1 + y0*initSize] = true;
		done[x0 + y1*initSize] = true;
		done[x2 + y1*initSize] = true;
		done[x1 + y2*initSize] = true;
		done[x1 + y1*initSize] = true;

		// Recurse through 4 corners
		value *= 0.5f;
		computeTerrain(done, pVB, x0,	y0,	currentSize, value, initSize, randomPrecomputed);
		computeTerrain(done, pVB, x0,	y1,	currentSize, value, initSize, randomPrecomputed);
		computeTerrain(done, pVB, x1,	y0,	currentSize, value, initSize, randomPrecomputed);
		computeTerrain(done, pVB, x1,	y1,	currentSize, value, initSize, randomPrecomputed);
	}
}

void SampleVehicle::createTerrain(PxU32 size, float width, float chaos)
{
	mNbTerrainVerts = size*size;

	// Vertex buffer
	mTerrainVB = (float*)SAMPLE_ALLOC(sizeof(float)*mNbTerrainVerts*3*3);
	for(PxU32 y=0;y<size;y++)
	{
		for(PxU32 x=0;x<size;x++)
		{
			mTerrainVB[(x+y*size)*9+0] = (float(x)-(float(size-1)*0.5f))* width;
			mTerrainVB[(x+y*size)*9+1] = 0.0f;
			mTerrainVB[(x+y*size)*9+2] = (float(y)-(float(size-1)*0.5f))* width;
			mTerrainVB[(x+y*size)*9+3] = 0.0f; mTerrainVB[(x+y*size)*9+4] = 1.0f; mTerrainVB[(x+y*size)*9+5] = 0.0f;
			mTerrainVB[(x+y*size)*9+6] = 0.5f; mTerrainVB[(x+y*size)*9+7] = 0.4f; mTerrainVB[(x+y*size)*9+8] = 0.2f;
		}
	}

	// Fractalize
	bool* doneBuffer = (bool*)SAMPLE_ALLOC(sizeof(bool)*mNbTerrainVerts);
	PxU32* tagBuffer = (PxU32*)SAMPLE_ALLOC(sizeof(PxU32)*mNbTerrainVerts);
	for(PxU32 i=0;i<mNbTerrainVerts;i++)
	{ 
		doneBuffer[i] = false;
		tagBuffer[i] = 0;
	}
	mTerrainVB[1] = 10.0f;
	mTerrainVB[(size-1)*9+1] = 10.0f;
	mTerrainVB[(size*(size-1))*9+1] = 10.0f;
	mTerrainVB[(mNbTerrainVerts-1)*9+1] = 10.0f;

	SampleRandomPrecomputed randomPrecomputed(*this);
	computeTerrain(doneBuffer, mTerrainVB, 0, 0, size, chaos/16.0f, size, randomPrecomputed);

	const PxU32 street0 = (PxU32)(size/3.0f);
	const PxU32 streetSize = (PxU32)(size/30.0f);
	float ay = 0.0f;

	for(PxU32 y=0;y<size;y++)
	{
		for(PxU32 x=street0;x<street0+streetSize;x++)
		{
			ay+=mTerrainVB[(x+y*size)*9+1];
			ay+=mTerrainVB[(y+x*size)*9+1];
		}
	}

	const float cx = size/2.0f;
	const float cy = size/2.0f;
	const float r = size/3.0f;
	const float g = streetSize/2.0f;

	for(PxU32 i=0;i<mNbTerrainVerts;i++)
		tagBuffer[i] = false;

	ay/=streetSize*size;
	ay-=streetSize;
	for(PxU32 y=15;y<size-15;y++)
	{
		bool smoothBorder = true;

		for(PxU32 x=street0;x<street0+streetSize;x++)
		{
			if(y > size*0.5f && y < size*0.7f)
			{
				mTerrainVB[(x+y*size)*9+1]=ay+sinf(((float)y)*12.0f+4.0f)*2.0f;
				smoothBorder = false;
			}
			else
			{
				mTerrainVB[(x+y*size)*9+1]=ay;
			}

			if(y > size*0.55f && y < size*0.75f)
			{
				mTerrainVB[(y+x*size)*9+1]=ay+sinf(y*12.0f)*0.75f;
				//mTerrainVB[(y+x*size)*9+1]=ay;
				tagBuffer[y+x*size] = 3;
				tagBuffer[x+y*size] = 3;
				smoothBorder = false;
			}
			else if(y < size*0.15f)
			{
				const float s = size*0.15f-(float)y;
				mTerrainVB[(y+x*size)*9+1]=ay+s*0.25f;
				smoothBorder = false;
			}
			else if(y > size*0.85f)
			{
				const float s = (float)y-size*0.85f;
				mTerrainVB[(y+x*size)*9+1]=ay+s*0.7f;
				smoothBorder = false;
			}
			else
			{
				mTerrainVB[(y+x*size)*9+1]=ay;
				tagBuffer[y+x*size] = 1;
				tagBuffer[x+y*size] = 1;
			}

		}
		if(smoothBorder)
		{
			mTerrainVB[((street0-1)+y*size)*9+1]=ay*0.5f+mTerrainVB[((street0-1)+y*size)*9+1]*0.5f;
			mTerrainVB[(y+(street0-1)*size)*9+1]=ay*0.5f+mTerrainVB[(y+(street0-1)*size)*9+1]*0.5f;
			mTerrainVB[((street0+1)+y*size)*9+1]=ay*0.5f+mTerrainVB[((street0+1)+y*size)*9+1]*0.5f;
			mTerrainVB[(y+(street0+1)*size)*9+1]=ay*0.5f+mTerrainVB[(y+(street0+1)*size)*9+1]*0.5f;
		}
	}

	// Circle street
	for(PxU32 y=0;y<size;y++)
	{
		for(PxU32 x=0;x<size;x++)
		{
			const float x0 = x-cx;
			const float y0 = y-cy;
			const float d = sqrtf(x0*x0+y0*y0);
			if(d >= r && d < r+streetSize)
			{
				mTerrainVB[(y+x*size)*9+1]=ay;
				
				if(y > size*0.55f && y < size*0.75f)
					tagBuffer[y+x*size] = 2;
				else
					tagBuffer[y+x*size] = 1;

			}
			else if(d >= r+streetSize && d < r+streetSize+g)
			{
				const float a = (d-(r+streetSize))/g;
				mTerrainVB[(y+x*size)*9+1]=ay*(1.0f-a) + mTerrainVB[(y+x*size)*9+1]*a;
			}
			else if(d >= r-g && d < r)
			{
				const float a = (d-(r-g))/g;
				mTerrainVB[(y+x*size)*9+1]=ay*a+mTerrainVB[(y+x*size)*9+1]*(1.0f-a);
			}
		}
	}

	// Borders
	const float b = size/25.0f;
	const float bd = size/2.0f-b;
	for(PxU32 y=0;y<size;y++)
	{
		for(PxU32 x=0;x<size;x++)
		{
			const float x0 = fabsf(x-cx);
			const float y0 = fabsf(y-cy);
			if(x0 > bd || y0 > bd)
			{
				float a0 = (x0-bd)/b;
				float a1 = (y0-bd)/b;
				if(a1 > a0)
					a0 = a1;
				mTerrainVB[(y+x*size)*9+1]=20.0f*a0 + mTerrainVB[(y+x*size)*9+1]*(1-a0);
			}
		}
	}

	// Sobel filter
	for(PxU32 y=1;y<size-1;y++)
	{
		for(PxU32 x=1;x<size-1;x++)
		{
			//  1  0 -1
			//  2  0 -2
			//  1  0 -1
			float dx;
			dx  =   mTerrainVB[((x-1)+(y-1)*size)*9+1];
			dx -=   mTerrainVB[((x+1)+(y-1)*size)*9+1];
			dx += 2.0f*mTerrainVB[((x-1)+(y+0)*size)*9+1];
			dx -= 2.0f*mTerrainVB[((x+1)+(y+0)*size)*9+1];
			dx +=   mTerrainVB[((x-1)+(y+1)*size)*9+1];
			dx -=   mTerrainVB[((x+1)+(y+1)*size)*9+1];

			//  1  2  1
			//  0  0  0
			// -1 -2 -1
			float dy;
			dy  =   mTerrainVB[((x-1)+(y-1)*size)*9+1];
			dy += 2.0f*mTerrainVB[((x+0)+(y-1)*size)*9+1];
			dy +=   mTerrainVB[((x+1)+(y-1)*size)*9+1];
			dy -=   mTerrainVB[((x-1)+(y+1)*size)*9+1];
			dy -= 2.0f*mTerrainVB[((x+0)+(y+1)*size)*9+1];
			dy -=   mTerrainVB[((x+1)+(y+1)*size)*9+1];

			const float nx = dx/width*0.15f;
			const float ny = 1.0f;
			const float nz = dy/width*0.15f;

			const float len = sqrtf(nx*nx+ny*ny+nz*nz);

			mTerrainVB[(x+y*size)*9+3] = nx/len;
			mTerrainVB[(x+y*size)*9+4] = ny/len;
			mTerrainVB[(x+y*size)*9+5] = nz/len;
		}
	}

	// Static lighting (two directional lights)
	const float l0[3] = {0.25f/0.8292f, 0.75f/0.8292f, 0.25f/0.8292f};
	const float l1[3] = {0.65f/0.963f, 0.55f/0.963f, 0.45f/0.963f};
	//const float len = sqrtf(l1[0]*l1[0]+l1[1]*l1[1]+l1[2]*l1[2]);
	for(PxU32 y=0;y<size;y++)
	{
		for(PxU32 x=0;x<size;x++)
		{
			const float nx = mTerrainVB[(x+y*size)*9+3], ny = mTerrainVB[(x+y*size)*9+4], nz = mTerrainVB[(x+y*size)*9+5];

			const float a = 0.3f;
			float dot0 = l0[0]*nx + l0[1]*ny + l0[2]*nz;
			float dot1 = l1[0]*nx + l1[1]*ny + l1[2]*nz;
			if(dot0 < 0.0f) { dot0 = 0.0f; }
			if(dot1 < 0.0f) { dot1 = 0.0f; }

			const float l = dot0*0.7f + dot1*0.3f;

			mTerrainVB[(x+y*size)*9+6] = mTerrainVB[(x+y*size)*9+6]*(l + a);
			mTerrainVB[(x+y*size)*9+7] = mTerrainVB[(x+y*size)*9+7]*(l + a);
			mTerrainVB[(x+y*size)*9+8] = mTerrainVB[(x+y*size)*9+8]*(l + a);

			/*mTerrainVB[(x+y*size)*9+3] = 0.0f;
			mTerrainVB[(x+y*size)*9+4] = -1.0f;
			mTerrainVB[(x+y*size)*9+5] = 0.0f;*/
		}
	}

	// Index buffers
	const PxU32 maxNbTerrainTriangles = (size-1)*(size-1)*2;

	mNbIB = 4;
	mRenderMaterial[0] = MATERIAL_TERRAIN_MUD;
	mRenderMaterial[1] = MATERIAL_ROAD_TARMAC;
	mRenderMaterial[2] = MATERIAL_ROAD_SNOW;
	mRenderMaterial[3] = MATERIAL_ROAD_GRASS;

	for(PxU32 i=0;i<mNbIB;i++)
	{
		mIB[i] = (PxU32*)SAMPLE_ALLOC(sizeof(PxU32)*maxNbTerrainTriangles*3);
		mNbTriangles[i] = 0;
	}

	for(PxU32 j=0;j<size-1;j++)
	{
		for(PxU32 i=0;i<size-1;i++)
		{
			PxU32 tris[6];
			tris[0] = i   + j*size; tris[1] = i   + (j+1)*size; tris[2] = i+1 + (j+1)*size;
			tris[3] = i   + j*size; tris[4] = i+1 + (j+1)*size; tris[5] = i+1 + j*size;

			for(PxU32 t=0;t<2;t++)
			{
				const PxU32 vt0 = tagBuffer[tris[t*3+0]];
				const PxU32 vt1 = tagBuffer[tris[t*3+1]];
				const PxU32 vt2 = tagBuffer[tris[t*3+2]];

				PxU32 buffer = 0;
				if(vt0 == vt1 && vt0 == vt2)
					buffer = vt0;
				
				mIB[buffer][mNbTriangles[buffer]*3+0] = tris[t*3+0];
				mIB[buffer][mNbTriangles[buffer]*3+1] = tris[t*3+1];
				mIB[buffer][mNbTriangles[buffer]*3+2] = tris[t*3+2];
				mNbTriangles[buffer]++;
			}
		}
	}

	SAMPLE_FREE(tagBuffer);
	SAMPLE_FREE(doneBuffer);
}

void SampleVehicle::addMesh(PxRigidActor* actor, float* verts, PxU32 nVerts, PxU32* indices, PxU32 mIndices, PxU32 materialIndex, const char* filename)
{
	const char* filenameCooked = getSampleOutputFilePath(filename, ""); 
	PX_ASSERT(NULL != filenameCooked);

	bool ok = false;
	if(!gRecook)
	{
		SampleFramework::File* fp = NULL;
		PxToolkit::fopen_s(&fp, filenameCooked, "rb");
		if(fp)
		{
			fseek(fp, 0, SEEK_END);
			PxU32 filesize = (PxU32)ftell(fp);

			fclose(fp);


			ok = (filesize != 0);
		}
	}

	if(!ok)
	{
		PxTriangleMeshDesc meshDesc;
		meshDesc.points.count		= nVerts;
		meshDesc.triangles.count	= mIndices;
		meshDesc.points.stride		= sizeof(float)*3*3;
		meshDesc.triangles.stride	= sizeof(PxU32)*3;
		meshDesc.points.data		= verts;
		meshDesc.triangles.data		= indices;
		meshDesc.flags = PxMeshFlags(0);

		//
		shdfnd::printFormatted("Cooking object... %s", filenameCooked);
		PxDefaultFileOutputStream stream(filenameCooked);
		ok = getCooking().cookTriangleMesh(meshDesc, stream);
		shdfnd::printFormatted(" - Done\n");
	}

	{
		PxDefaultFileInputData stream(filenameCooked);
		PxTriangleMesh* triangleMesh = getPhysics().createTriangleMesh(stream);

		if(triangleMesh)
		{
			PxRigidActorExt::createExclusiveShape(*actor, PxTriangleMeshGeometry(triangleMesh), *mStandardMaterials[materialIndex] /**mStandardMaterial*/);
		}
	}
}

// PT: the renderer also expects 16bit indices so we still have to cut a big mesh to pieces
void SampleVehicle::addRenderMesh(float* verts, PxU32 nVerts, PxU32* indices, PxU32 mIndices, PxU32 matID)
{
	float minX=1000000.0f;
	float minZ=1000000.0f;
	float maxX=-1000000.0f;
	float maxZ=-1000000.0f;

	// PT: the VB uses interleaved data so we need a temp buffer
	PxVec3Alloc* v = SAMPLE_NEW(PxVec3Alloc)[nVerts];
	float* curVerts = verts;
	PxBounds3 meshBound = PxBounds3::empty();
	for(PxU32 i=0;i<nVerts;i++)
	{
		v[i].x = curVerts[0];
		v[i].y = curVerts[1];
		v[i].z = curVerts[2];
		if (v[i].x < minX) { minX = v[i].x; }
		if (v[i].z < minZ) { minZ = v[i].z; }
		if (v[i].x > maxX) { maxX = v[i].x; }
		if (v[i].z > maxZ) { maxZ = v[i].z; }
		curVerts += 3*3;

		meshBound.include(v[i]);
	}

	PxReal* uv = (PxReal*)SAMPLE_ALLOC(sizeof(PxReal)*nVerts*2);
	curVerts = verts;
	const float scaleX = (maxX-minX)/64.0f;
	const float scaleZ = (maxZ-minZ)/64.0f;
	for(PxU32 i=0;i<nVerts;i++)
	{
		uv[i*2+0] = (curVerts[0]-minX)/scaleX;
		uv[i*2+1] = (curVerts[2]-minZ)/scaleZ;
		curVerts += 3*3;
	}
		
	RAWMesh data;
	data.mMaterialID	= matID;
	data.mNbVerts		= nVerts;
	data.mNbFaces		= mIndices;
	data.mVerts			= v;
	data.mUVs			= uv;
	data.mIndices		= indices;
	RenderMeshActor* renderActor = createRenderMeshFromRawMesh(data);
	if( renderActor != NULL )
	{
		renderActor->setWorldBounds(meshBound);
		renderActor->setEnableCameraCull(true);
	}

	SAMPLE_FREE(uv);
	DELETEARRAY(v);
}

void SampleVehicle::createTrack(PxU32 size, float width, float chaos)
{
	createTerrain(size, width, chaos);
	createLandscapeMesh();
}

void SampleVehicle::createLandscapeMesh()
{
	RAWTexture data;
	data.mName = "gravel_diffuse.dds";
	RenderTexture* gravelTexture = createRenderTextureFromRawTexture(data);

	mTerrainMaterial = SAMPLE_NEW(RenderMaterial)(*getRenderer(), PxVec3(0.5f, 0.25f, 0.125f), 1.0f, false, MATERIAL_TERRAIN_MUD, gravelTexture);
	mRenderMaterials.push_back(mTerrainMaterial);

	data.mName = "asphalt_diffuse.dds";
	RenderTexture* asphaltTexture = createRenderTextureFromRawTexture(data);

	mRoadMaterial = SAMPLE_NEW(RenderMaterial)(*getRenderer(), PxVec3(1.0f, 1.0f, 1.0f), 1.0f, false, MATERIAL_ROAD_TARMAC, asphaltTexture);
	mRenderMaterials.push_back(mRoadMaterial);

	data.mName = "ice_diffuse.dds";
	RenderTexture* snowTexture = createRenderTextureFromRawTexture(data);

	mRoadIceMaterial = SAMPLE_NEW(RenderMaterial)(*getRenderer(), PxVec3(0.05f, 0.05f, 0.75f), 1.0f, false, MATERIAL_ROAD_SNOW, snowTexture);
	mRenderMaterials.push_back(mRoadIceMaterial);

	data.mName = "grass_diffuse.dds";
	RenderTexture* grassTexture = createRenderTextureFromRawTexture(data);

	mRoadGravelMaterial = SAMPLE_NEW(RenderMaterial)(*getRenderer(), PxVec3(1.0f, 1.0f, 1.0f), 1.0f, false, MATERIAL_ROAD_GRASS, grassTexture);
	mRenderMaterials.push_back(mRoadGravelMaterial);

	PxTransform pose;
	pose = PxTransform(PxIdentity);
//	pose.p.y -= 10.0f;
	mHFActor = getPhysics().createRigidStatic(pose);

	for(PxU32 i=0;i<mNbIB;i++)
	{
		if(mNbTriangles[i] > 0) 
		{
			char filename[512];
			sprintf(filename, "SampleVehicleGroundMeshes_Part%d", i);
			addMesh(mHFActor, mTerrainVB, mNbTerrainVerts, mIB[i], mNbTriangles[i], i, filename);
			if (mNbTriangles[i] > (1<<16)) 
			{
				PxU32 firstBatch = mNbTriangles[i]/2;
				addRenderMesh(mTerrainVB, mNbTerrainVerts, mIB[i], firstBatch, MATERIAL_TERRAIN_MUD);
				addRenderMesh(mTerrainVB, mNbTerrainVerts, mIB[i]+(firstBatch*3), mNbTriangles[i]-firstBatch, mRenderMaterial[i]);
			} 
			else 
			{
				addRenderMesh(mTerrainVB, mNbTerrainVerts, mIB[i], mNbTriangles[i], mRenderMaterial[i]);
			}
		}
	}

	PxSceneWriteLock scopedLock(*mScene);
	mScene->addActor(*mHFActor);

	PxShape* shapeBuffer[MAX_NUM_INDEX_BUFFERS];
	mHFActor->getShapes(shapeBuffer, MAX_NUM_INDEX_BUFFERS);
	PxFilterData simulationFilterData;
	simulationFilterData.word0=COLLISION_FLAG_GROUND;
	simulationFilterData.word1=COLLISION_FLAG_GROUND_AGAINST;
	PxFilterData queryFilterData;
	SampleVehicleSetupDrivableShapeQueryFilterData(&queryFilterData);
	
	for(PxU32 i=0;i<mNbIB;i++)
	{
		shapeBuffer[i]->setSimulationFilterData(simulationFilterData);
		shapeBuffer[i]->setQueryFilterData(queryFilterData);
		shapeBuffer[i]->setFlag(PxShapeFlag::eVISUALIZATION,false);
	}
}
