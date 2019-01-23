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


#include "PxPhysicsAPI.h"
#include "PxTkStream.h"
#include "SampleNorthPole.h"

using namespace PxToolkit;

#define USE_MESH_LANDSCAPE	// PT: define this to use a triangle mesh for the landscape, instead of a heightfield

static PxReal randomScaled(PxReal value)
{
	PxReal val = value*(getSampleRandom().randomFloat());
	return val;
}

static void computeTerrain(bool* done, PxReal* pVB, PxU32 x0, PxU32 y0, PxU32 currentSize, PxReal value, PxU32 initSize)
{

	// Compute new size
	currentSize>>=1;
	if (currentSize > 0) {
		PxU32 x1 = (x0+currentSize)			   % initSize;
		PxU32 x2 = (x0+currentSize+currentSize) % initSize;
		PxU32 y1 = (y0+currentSize)			   % initSize;
		PxU32 y2 = (y0+currentSize+currentSize) % initSize;

		if(!done[x1 + y0*initSize])	pVB[(x1 + y0*initSize)] = randomScaled(value) + 0.5f * (pVB[(x0 + y0*initSize)] + pVB[(x2 + y0*initSize)]);
		if(!done[x0 + y1*initSize])	pVB[(x0 + y1*initSize)] = randomScaled(value) + 0.5f * (pVB[(x0 + y0*initSize)] + pVB[(x0 + y2*initSize)]);
		if(!done[x2 + y1*initSize])	pVB[(x2 + y1*initSize)] = randomScaled(value) + 0.5f * (pVB[(x2 + y0*initSize)] + pVB[(x2 + y2*initSize)]);
		if(!done[x1 + y2*initSize])	pVB[(x1 + y2*initSize)] = randomScaled(value) + 0.5f * (pVB[(x0 + y2*initSize)] + pVB[(x2 + y2*initSize)]);
		if(!done[x1 + y1*initSize])	pVB[(x1 + y1*initSize)] = randomScaled(value) + 0.5f * (pVB[(x0 + y1*initSize)] + pVB[(x2 + y1*initSize)]);

		done[x1 + y0*initSize] = true; done[x0 + y1*initSize] = true; done[x2 + y1*initSize] = true; done[x1 + y2*initSize] = true; done[x1 + y1*initSize] = true;

		// Recurse through 4 corners
		value *= 0.5f;
		computeTerrain(done, pVB, x0,	y0,	currentSize, value, initSize);
		computeTerrain(done, pVB, x0,	y1,	currentSize, value, initSize);
		computeTerrain(done, pVB, x1,	y0,	currentSize, value, initSize);
		computeTerrain(done, pVB, x1,	y1,	currentSize, value, initSize);
	}
}

static void fractalize(PxReal* heights, const PxU32 size, const PxReal roughness)
{
	PxU32 num = size*size;
	bool* done = (bool*)SAMPLE_ALLOC(sizeof(bool)*num);
	for(PxU32 i=0; i<num; i++)
		done[i]=false;
	computeTerrain(done,heights,0,0,size,roughness,size);
	SAMPLE_FREE(done);
}

PxRigidStatic* SampleNorthPole::buildHeightField()
{
	// create a height map

	PxU32 hfSize = 32; // some power of 2
	PxU32 hfNumVerts = hfSize*hfSize;

	PxReal hfScale = 8.0f; // this is how wide one heightfield square is

	PxReal* heightmap = (PxReal*)SAMPLE_ALLOC(sizeof(PxReal)*hfNumVerts);
	for(PxU32 i = 0; i < hfNumVerts; i++)
		heightmap[i]=-9.0f;

	getSampleRandom().setSeed(42);
	fractalize(heightmap,hfSize,16);

	{ // make a plateau to place the igloo
		const PxU32 i = hfSize*hfSize/2+hfSize/2;
		PxReal h = 0;
		heightmap[i-hfSize-1]=h;	heightmap[i-hfSize]=h;	heightmap[i-hfSize+1]=h;
		heightmap[i-1]=h;			heightmap[i]=h;			heightmap[i+1]=h;
		heightmap[i+hfSize-1]=h;	heightmap[i+hfSize]=h;	heightmap[i+hfSize+1]=h;
	}

	{ // make render terrain
		PxReal* hfVerts = (PxReal*)SAMPLE_ALLOC(sizeof(PxReal)*hfNumVerts*3);
		PxReal* hfNorms = (PxReal*)SAMPLE_ALLOC(sizeof(PxReal)*hfNumVerts*3);
		PxU32 hfNumTris = (hfSize-1)*(hfSize-1)*2;
		PxU32* hfTris = (PxU32*)SAMPLE_ALLOC(sizeof(PxU32)*hfNumTris*3);

		createLandscape(heightmap,hfSize,hfScale,hfVerts,hfNorms,hfTris);	

 		RAWMesh data;
		data.mName = "terrain";
		data.mTransform = PxTransform(PxIdentity);
		data.mTransform.p = PxVec3(-(hfSize/2*hfScale),0,-(hfSize/2*hfScale));
 		data.mNbVerts = hfNumVerts;
		data.mVerts = (PxVec3*)hfVerts;
		data.mVertexNormals = (PxVec3*)hfNorms;
		data.mUVs = 0;
		data.mMaterialID = 0;
		data.mNbFaces = hfNumTris;
		data.mIndices = hfTris;
#ifdef USE_MESH_LANDSCAPE
		setFilename(getSampleMediaFilename("SampleNorthPole_Mesh"));
		newMesh(data);
#else
		createRenderMeshFromRawMesh(data);
#endif
		SAMPLE_FREE(hfTris);
		SAMPLE_FREE(hfNorms);
		SAMPLE_FREE(hfVerts);
	}

#ifndef USE_MESH_LANDSCAPE
	// eventually create the physics heightfield
	PxRigidStatic* hfActor = createHeightField(heightmap,hfScale,hfSize);
#endif	

	SAMPLE_FREE(heightmap);
#ifdef USE_MESH_LANDSCAPE
	return 0;
#else
	return hfActor;
#endif
}

void SampleNorthPole::createLandscape(PxReal* heightmap, PxU32 width, PxReal hfScale, PxReal* outVerts, PxReal* outNorms, PxU32* outTris)
{
	for(PxU32 y=0; y<width; y++)
	{
		for(PxU32 x=0; x<width; x++)
		{
			PxU32 index=(x+y*width)*3;

			outVerts[index+0]=(PxReal(x))* hfScale;
			outVerts[index+1]=heightmap[x+y*width];
			outVerts[index+2]=(PxReal(y))*hfScale;
			
			outNorms[index+0]=0;
			outNorms[index+1]=1;
			outNorms[index+2]=0;
		}
	}

	// Sobel filter
	for (PxU32 y=1;y<width-1;y++) {
		for (PxU32 x=1;x<width-1;x++) {

			//  1  0 -1
			//  2  0 -2
			//  1  0 -1
			PxReal dx;
			dx  =   outVerts[((x-1)+(y-1)*width)*3+1];
			dx -=   outVerts[((x+1)+(y-1)*width)*3+1];
			dx += 2.0f*outVerts[((x-1)+(y+0)*width)*3+1];
			dx -= 2.0f*outVerts[((x+1)+(y+0)*width)*3+1];
			dx +=   outVerts[((x-1)+(y+1)*width)*3+1];
			dx -=   outVerts[((x+1)+(y+1)*width)*3+1];

			//  1  2  1
			//  0  0  0
			// -1 -2 -1
			PxReal dy;
			dy  =   outVerts[((x-1)+(y-1)*width)*3+1];
			dy += 2.0f*outVerts[((x+0)+(y-1)*width)*3+1];
			dy +=   outVerts[((x+1)+(y-1)*width)*3+1];
			dy -=   outVerts[((x-1)+(y+1)*width)*3+1];
			dy -= 2.0f*outVerts[((x+0)+(y+1)*width)*3+1];
			dy -=   outVerts[((x+1)+(y+1)*width)*3+1];

			PxReal nx = dx/hfScale*0.15f;
			PxReal ny = 1.0f;
			PxReal nz = dy/hfScale*0.15f;

			PxReal len = sqrtf(nx*nx+ny*ny+nz*nz);

			outNorms[(x+y*width)*3+0] = nx/len;
			outNorms[(x+y*width)*3+1] = ny/len;
			outNorms[(x+y*width)*3+2] = nz/len;
		}
	}

	PxU32 numTris = 0;
	for(PxU32 j=0; j<width-1; j++)
	{
		for(PxU32 i=0; i<width-1; i++)
		{
			outTris[3*numTris+0]= i   +  j   *(width);
			outTris[3*numTris+1]= i   + (j+1)*(width);
			outTris[3*numTris+2]= i+1 +  j   *(width);
			outTris[3*numTris+3]= i   + (j+1)*(width);
			outTris[3*numTris+4]= i+1 + (j+1)*(width);
			outTris[3*numTris+5]= i+1 +  j   *(width);
			numTris+=2;
		}
	}
}

PxRigidStatic* SampleNorthPole::createHeightField(PxReal* heightmap, PxReal hfScale, PxU32 hfSize)
{
	const PxReal heightScale = 0.001f;

	PxU32 hfNumVerts = hfSize*hfSize;

	PxHeightFieldSample* samples = (PxHeightFieldSample*)SAMPLE_ALLOC(sizeof(PxHeightFieldSample)*hfNumVerts);
	memset(samples,0,hfNumVerts*sizeof(PxHeightFieldSample));
	
	for(PxU32 x = 0; x < hfSize; x++)
	for(PxU32 y = 0; y < hfSize; y++)
	{
		samples[x+y*hfSize].height = (PxI16)(heightmap[y+x*hfSize]/heightScale);
		samples[x+y*hfSize].setTessFlag();
		samples[x+y*hfSize].materialIndex0=1;
		samples[x+y*hfSize].materialIndex1=1;
	}

	PxHeightFieldDesc hfDesc;
	hfDesc.format = PxHeightFieldFormat::eS16_TM;
	hfDesc.nbColumns = hfSize;
	hfDesc.nbRows = hfSize;
	hfDesc.samples.data = samples;
	hfDesc.samples.stride = sizeof(PxHeightFieldSample);
	
	PxHeightField* heightField = getCooking().createHeightField(hfDesc, getPhysics().getPhysicsInsertionCallback());
	if(!heightField)
		fatalError("creating the heightfield failed");

	PxTransform pose = PxTransform(PxIdentity);
	pose.p = PxVec3(-(hfSize/2*hfScale),0,-(hfSize/2*hfScale));

	PxRigidStatic* hfActor = getPhysics().createRigidStatic(pose);
	if(!hfActor) 
		fatalError("creating heightfield actor failed");

	PxHeightFieldGeometry hfGeom(heightField, PxMeshGeometryFlags(), heightScale, hfScale, hfScale);
	PxShape* hfShape = PxRigidActorExt::createExclusiveShape(*hfActor, hfGeom, getDefaultMaterial());
	//setCCDActive(*hfShape);
	if(!hfShape) 
		fatalError("creating heightfield shape failed");

	getActiveScene().addActor(*hfActor);

	SAMPLE_FREE(samples);

	return hfActor;
}


PxRigidStatic* SampleNorthPole::buildIglooTriMesh()
{
	{ // construct the mesh data
		const PxReal innerR = 2.7f;
		const PxReal outerR = 3;
		const PxU32 numBlocks = 16;
		const PxU32 numRows = 8;
		const PxU32 doorHeight = 2;
		const PxReal accesslength = outerR*1.2f;
		const PxReal step = PxPi*2/numBlocks;
		const PxReal step2 = PxPi/2/numRows;
		const PxReal thickness = (outerR - innerR);

		const PxVec3 initpos(1,0,0);

		const PxU32 n = 2*numBlocks;
		PxVec3 bufferVerts[2][n+2];
		PxU32 buf = 0;

		PxVec3* currentVerts = bufferVerts[buf];

		for(int i = 0; i < (int)numBlocks; i++)
		{
			PxQuat rotY(i*step-step/2,PxVec3(0,1,0));
			PxVec3 pos = rotY.rotate(initpos);
			currentVerts[2*i] = pos*innerR;
			currentVerts[2*i+1] = pos*outerR;
		}
		currentVerts[n] = currentVerts[0];
		currentVerts[n+1] = currentVerts[1];

		for(int j = 0; j < (int)numRows; j++)
		{
			PxVec3* previousVerts = currentVerts;
			currentVerts = bufferVerts[buf=1-buf];

			PxQuat rotZ((j+1)*step2,PxVec3(0,0,1));
			PxVec3 posX = rotZ.rotate(initpos);

			for(int i = 0; i < (int)numBlocks; i++)
			{
				PxQuat rotY(i*step-step/2,PxVec3(0,1,0));
				PxVec3 pos = rotY.rotate(posX);
				currentVerts[2*i] = pos*innerR;
				currentVerts[2*i+1] = pos*outerR;
			}
			currentVerts[n] = currentVerts[0];
			currentVerts[n+1] = currentVerts[1];

			for(int i = 0; i < (int)numBlocks; i++)
			{
				if( (i==0 && (j<(int)doorHeight)) || (i==numBlocks/3 && j==numRows/2) )
				{
					// omit the door and window
					continue;
				}

				PxVec3 block[8] = 
				{
					previousVerts[2*i],previousVerts[2*i+1],previousVerts[2*i+2],previousVerts[2*i+3],
					 currentVerts[2*i], currentVerts[2*i+1], currentVerts[2*i+2], currentVerts[2*i+3]
				};

				addBlock(block);
			}
		}

		PxQuat rotZ((doorHeight)*step2,PxVec3(0,0,1));
		PxQuat rotY(step/2,PxVec3(0,1,0));

		PxVec3 topIL = rotY.rotate(rotZ.rotate(initpos))*innerR;
		PxVec3 topIR = rotY.rotateInv(rotZ.rotate(initpos))*innerR;

		PxVec3 topOL = topIL; topOL.x = accesslength;
		PxVec3 topOR = topIR; topOR.x = accesslength;

		PxVec3 bottomIL = rotY.rotate(initpos)*innerR;
		PxVec3 bottomIR = rotY.rotateInv(initpos)*innerR;

		PxVec3 bottomOL = topOL; bottomOL.y = 0;
		PxVec3 bottomOR = topOR; bottomOR.y = 0;

		PxVec3 dZ = PxVec3(0,0,thickness);
		PxVec3 dY = PxVec3(0,thickness,0);

		PxVec3 blockL[8] = 
		{
			bottomIL, bottomOL, bottomIL-dZ, bottomOL-dZ, 
			topIL, topOL, topIL-dZ, topOL-dZ 
		};
		addBlock(blockL);

		PxVec3 blockR[8] = 
		{
			bottomIR+dZ, bottomOR+dZ, bottomIR, bottomOR, 
			topIR+dZ, topOR+dZ, topIR, topOR 
		};
		addBlock(blockR);

		PxVec3 blockT[8] =
		{
			topIR+dZ, topOR+dZ, topIL-dZ, topOL-dZ,
			topIR+dY, topOR+dY, topIL+dY, topOL+dY
		};
		addBlock(blockT);
	}

	const PxU32 numTris = (PxU32)(mTris.size()/3);
	const PxU32 numVerts = (PxU32)(mVerts.size());

	// make render mesh
	RAWMesh data;
	data.mIndices = &mTris[0];
	data.mNbFaces = numTris;
	data.mVerts = &mVerts[0];
	data.mNbVerts = numVerts;
	data.mVertexNormals = &mVerts[0];
	data.mMaterialID = 0;
	data.mTransform = PxTransform(PxVec3(0,0,0));
	createRenderMeshFromRawMesh(data);

	PxTriangleMesh* triMesh = PxToolkit::createTriangleMesh32(getPhysics(), getCooking(), &mVerts[0], numVerts, &mTris[0], numTris);
	if(!triMesh) 
		fatalError("creating the triangle mesh failed");

	mVerts.clear();
	mTris.clear();
	mNorms.clear();

	// create actor
	PxRigidStatic* iglooActor = getPhysics().createRigidStatic(PxTransform(PxVec3(0,0,0)));
	if(!iglooActor)
		fatalError("creating igloo actor failed");

	PxTriangleMeshGeometry geom(triMesh);
	PxShape* iglooShape = PxRigidActorExt::createExclusiveShape(*iglooActor, geom,getDefaultMaterial());
	if(!iglooShape)
		fatalError("creating igloo shape failed");

	//setCCDActive(*iglooShape);

	getActiveScene().addActor(*iglooActor);

	return iglooActor;
}

void SampleNorthPole::addBlock(PxVec3 blockVerts[8])
{
	PxU32 n = 8;
	PxVec3* v = blockVerts;
	const PxU32 offset = (PxU32)mVerts.size();

	while(n--)
		mVerts.push_back(*v++);

	// bottom
	mTris.push_back(0 + offset); mTris.push_back(3 + offset); mTris.push_back(1 + offset);
	mTris.push_back(0 + offset); mTris.push_back(2 + offset); mTris.push_back(3 + offset);
	// top
	mTris.push_back(4 + offset); mTris.push_back(5 + offset); mTris.push_back(7 + offset);
	mTris.push_back(4 + offset); mTris.push_back(7 + offset); mTris.push_back(6 + offset);
	// right
	mTris.push_back(0 + offset); mTris.push_back(1 + offset); mTris.push_back(4 + offset);
	mTris.push_back(5 + offset); mTris.push_back(4 + offset); mTris.push_back(1 + offset);
	// left
	mTris.push_back(6 + offset); mTris.push_back(7 + offset); mTris.push_back(2 + offset);
	mTris.push_back(3 + offset); mTris.push_back(2 + offset); mTris.push_back(7 + offset);
	// front
	mTris.push_back(5 + offset); mTris.push_back(1 + offset); mTris.push_back(7 + offset);
	mTris.push_back(3 + offset); mTris.push_back(7 + offset); mTris.push_back(1 + offset);
	// back
	mTris.push_back(4 + offset); mTris.push_back(6 + offset); mTris.push_back(0 + offset);
	mTris.push_back(2 + offset); mTris.push_back(0 + offset); mTris.push_back(6 + offset);
}





