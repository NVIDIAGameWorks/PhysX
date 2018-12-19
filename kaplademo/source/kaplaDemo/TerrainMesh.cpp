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

#include <algorithm>
#include "TerrainMesh.h"
#include "PxPhysics.h"
#include "PxCooking.h"
#include "PxScene.h"
#include "Shader.h"
#include "geometry/PxHeightFieldSample.h"
#include "geometry/PxHeightFieldDesc.h"
#include "geometry/PxHeightFieldGeometry.h"
#include "PxRigidStatic.h"
#include "Texture.h"
#include "TerrainMesh.h"

#include "PhysXMacros.h"
#include "SceneVehicleSceneQuery.h"
#include "VehicleManager.h"
#include "PxDefaultStreams.h"

#include"PxRigidActorExt.h"

#define USE_HEIGHTFIELD 1


TerrainMesh::~TerrainMesh() {
	//myScene->removeActor(*hfActor);
	hfActor->release();
	if (mMesh) delete mMesh;
}

TerrainMesh::TerrainMesh(PxPhysics& physics, PxCooking& cooking, PxScene& scene, PxMaterial& material, PxVec3 midPoint, const char* heightMapName, const char* textureName, float hfScale, float maxHeight, Shader* shader,
	bool invert) {
	mShader = shader;
	mMesh = 0;
	myScene = &scene;
	BmpLoaderBuffer heightMapFile;
	if (!heightMapFile.loadFile(heightMapName)) {
		fprintf(stderr, "Error loading bmp '%s'\n");
		return;  
	}

	const PxReal heightScale = 0.005f;
	//const PxReal heightScale = 0.01f;

	unsigned int hfWidth = heightMapFile.mWidth;
	unsigned int hfHeight = heightMapFile.mHeight;
	unsigned char* heightmap = heightMapFile.mRGB;
	const double i255 = 1.0/255.0;

	PxTransform pose = PX_TRANSFORM_ID;
	pose.p = midPoint + PxVec3(-((hfWidth - 1)*0.5f*hfScale), 0, -((hfHeight - 1)*0.5f*hfScale));
	mHFPose = pose;


	PxU32 hfNumVerts = hfWidth*hfHeight;

	PxHeightFieldSample* samples = new PxHeightFieldSample[hfNumVerts];
	memset(samples,0,hfNumVerts*sizeof(PxHeightFieldSample));


	PxFilterData simulationFilterData;
	simulationFilterData.word0 = COLLISION_FLAG_GROUND;
	simulationFilterData.word1 = COLLISION_FLAG_GROUND_AGAINST;
	PxFilterData queryFilterData;
	VehicleSetupDrivableShapeQueryFilterData(&queryFilterData);
	static PxI32 maxH = -2000000;
	static PxI32 minH = 20000000;
	for(PxU32 y = 0; y < hfHeight; y++)
	{
		for(PxU32 x = 0; x < hfWidth; x++) 
		{
			
			PxU8 heightVal = heightmap[(y + x*hfWidth) * 3];
			if (invert)
				heightVal = 255 - heightVal;
			PxI32 h = PxI32((maxHeight*heightVal*i255) / heightScale);
			//Do a snap-to-grid...

			h = ((h >> 4) << 4);

			maxH = PxMax(h, maxH);
			minH = PxMin(h, minH);

			//h = h & (~0x7f);

			//printf("%d ", h);
			PX_ASSERT(h<=0xffff);

			const PxU32 Index = x + y*hfWidth;
			samples[Index].height = (PxI16)(h);
			samples[Index].materialIndex0 = 0;
			samples[Index].materialIndex1 = 0;
			samples[Index].clearTessFlag();
		}
	}


	


	// texture
	if (strcmp(textureName, "") == 0) hfTexture = 0; else
	hfTexture = loadImgTexture(textureName);
	mMesh = new GLMesh(GL_TRIANGLES);
	mMesh->vertices.resize(hfNumVerts);
	mMesh->normals.resize(hfNumVerts);
	mMesh->texCoords.resize(hfNumVerts*2);
	mMesh->indices.clear();
	float us = 1.0f / hfWidth;
	float vs = 1.0f / hfHeight;
	terrainHeights.clear();
	terrainNormals.clear();
	for (int i = 0; i < hfHeight; i++) 
	{
		for (int j = 0; j < hfWidth; j++) 
		{
			PxReal height = PxReal(samples[i + j*hfWidth].height) * heightScale;

			mMesh->vertices[i*hfWidth + j] = PxVec3(hfScale*j, height, hfScale*i) + pose.p;
			terrainHeights.push_back(mMesh->vertices[i*hfWidth+j].y);
			mMesh->texCoords[(i*hfWidth + j)*2] = j*us;
			mMesh->texCoords[(i*hfWidth + j)*2+1] = i*vs;
		}
	}

	xStart = pose.p.x;
	zStart = pose.p.z;
	dx = hfScale;

	for (int i = 0; i < hfHeight-1; i++) {
		for (int j = 0; j < hfWidth-1; j++) {
			//mMesh->vertices[i*hfWidth + j] = PxVec3(hfScale*j, maxHeight*heightmap[(j+i*hfWidth)*3]*i255, hfScale*i);
			

			mMesh->indices.push_back(j + i*hfWidth);		
			mMesh->indices.push_back(j + (i + 1)*hfWidth);
			mMesh->indices.push_back((j + 1) + i*hfWidth);

			mMesh->indices.push_back((j + 1) + i*hfWidth);
			mMesh->indices.push_back(j + (i + 1)*hfWidth);		
			mMesh->indices.push_back((j + 1) + (i + 1)*hfWidth);

		}
	}	
	mMesh->normals.resize(hfNumVerts);
	for (int i = 0; i < hfNumVerts; i++) {
		mMesh->normals[i] = PxVec3(0.0f, 0.0f, 0.0f);
	}

	int numTris = mMesh->indices.size() / 3;
	for (int i = 0; i < numTris; i++) {
		int i0 = mMesh->indices[i*3];
		int i1 = mMesh->indices[i*3+1];
		int i2 = mMesh->indices[i*3+2];
		PxVec3 p0 = mMesh->vertices[i0];
		PxVec3 p1 = mMesh->vertices[i1];
		PxVec3 p2 = mMesh->vertices[i2];

		PxVec3 normal = (p1-p0).cross(p2-p0);
		mMesh->normals[i0] += normal;
		mMesh->normals[i1] += normal;
		mMesh->normals[i2] += normal;
	}
	for (int i = 0; i < hfNumVerts; i++) {
		mMesh->normals[i].normalize();    
	}
	mMesh->genVBOIBO();
	mMesh->updateVBOIBO(false);
	mMaterial.init();
	mMaterial.texId = hfTexture;

	terrainNormals = mMesh->normals;

	width = hfWidth;
	height = hfHeight;
	idx = 1.0f/dx;


#if USE_HEIGHTFIELD

	PxHeightFieldDesc hfDesc;
	//hfDesc.format = PxHeightFieldFormat::eS16_TM;
	hfDesc.nbColumns = hfWidth;
	hfDesc.nbRows = hfHeight;
	hfDesc.samples.data = samples;
	hfDesc.samples.stride = sizeof(PxHeightFieldSample);

	PxHeightField* heightField = cooking.createHeightField(hfDesc, physics.getPhysicsInsertionCallback());

	

	hfActor = physics.createRigidStatic(pose);

	PxHeightFieldGeometry hfGeom(heightField, PxMeshGeometryFlags(), heightScale, hfScale, hfScale);
	PxShape* hfShape = PxRigidActorExt::createExclusiveShape(*hfActor, hfGeom, material);

	hfShape->setQueryFilterData(queryFilterData);
	hfShape->setSimulationFilterData(simulationFilterData);

	scene.addActor(*hfActor);

	delete[] samples;
#else
	PxTriangleMeshDesc meshDesc;

	meshDesc.points.data = &mMesh->vertices[0];
	meshDesc.points.count = mMesh->vertices.size();
	meshDesc.points.stride = sizeof(PxVec3);

	meshDesc.triangles.stride = sizeof(PxU32)*3;
	meshDesc.triangles.count = mMesh->indices.size()/3;
	meshDesc.triangles.data = &mMesh->indices[0];

	meshDesc.flags = PxMeshFlags();

	PxTriangleMeshGeometry geom;

	PxCookingParams params = cooking.getParams();
	params.buildGRBData = true;

	cooking.setParams(params);

	PxDefaultMemoryOutputStream writeBuffer;
	bool status = cooking.cookTriangleMesh(meshDesc, writeBuffer);
	PX_ASSERT(status);
	PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
	PxTriangleMesh* triangleMesh = physics.createTriangleMesh(readBuffer);

	geom.triangleMesh = triangleMesh;

	hfActor = physics.createRigidStatic(PxTransform(PxIdentity));

	PxShape* meshShape = hfActor->createShape(geom, material);

	meshShape->setQueryFilterData(queryFilterData);
	meshShape->setSimulationFilterData(simulationFilterData);

	scene.addActor(*hfActor);


#endif


}

void TerrainMesh::draw(bool useShader) {
	if (useShader) mShader->activate(mMaterial);
	mMesh->drawVBOIBO();
	if (useShader) mShader->deactivate();
}


float TerrainMesh::getHeight(float x, float z) {

	float nx = (x-xStart)*idx;
	float nz = (z-zStart)*idx;

	int ix = floor(nx);
	int iz = floor(nz);
	float fx = nx-ix;
	float fz = nz-iz;

	int ixp1 = ix+1;
	int izp1 = iz+1;
	ix = min(max(ix,0), width-1);
	iz = min(max(iz,0), height-1);
	ixp1 = min(max(ixp1,0), width-1);
	izp1 = min(max(izp1,0), height-1);

	int i00 = ix+iz*width;
	int i10 = ixp1+iz*width;
	int i01 = ix+izp1*width;
	int i11 = ixp1+izp1*width;

	float w00 = (1.0f-fx)*(1.0f-fz);
	float w10 = (fx)*(1.0f-fz);
	float w01 = (1.0f-fx)*(fz);
	float w11 = (fx)*(fz);

	return w00*terrainHeights[i00] +
		   w10*terrainHeights[i10] +
		   w01*terrainHeights[i01] +
		   w11*terrainHeights[i11];
}

	
float TerrainMesh::getHeightNormal(float x, float z, PxVec3& normal) {

	float nx = (x-xStart)*idx;
	float nz = (z-zStart)*idx;

	int ix = floor(nx);
	int iz = floor(nz);
	float fx = nx-ix;
	float fz = nz-iz;

	int ixp1 = ix+1;
	int izp1 = iz+1;
	ix = min(max(ix,0), width-1);
	iz = min(max(iz,0), height-1);
	ixp1 = min(max(ixp1,0), width-1);
	izp1 = min(max(izp1,0), height-1);

	int i00 = ix+iz*width;
	int i10 = ixp1+iz*width;
	int i01 = ix+izp1*width;
	int i11 = ixp1+izp1*width;

	float w00 = (1.0f-fx)*(1.0f-fz);
	float w10 = (fx)*(1.0f-fz);
	float w01 = (1.0f-fx)*(fz);
	float w11 = (fx)*(fz);

	normal = w00*terrainNormals[i00] +
			w10*terrainNormals[i10] +
			w01*terrainNormals[i01] +
			w11*terrainNormals[i11];
	normal.normalize();

	return w00*terrainHeights[i00] +
		w10*terrainHeights[i10] +
		w01*terrainHeights[i01] +
		w11*terrainHeights[i11];

}


float TerrainMesh::getHeightNormalCondition(float x, float y, float z, PxVec3& normal) {

	float nx = (x-xStart)*idx;
	float nz = (z-zStart)*idx;

	int ix = floor(nx);
	int iz = floor(nz);
	float fx = nx-ix;
	float fz = nz-iz;

	int ixp1 = ix+1;
	int izp1 = iz+1;
	ix = min(max(ix,0), width-1);
	iz = min(max(iz,0), height-1);
	ixp1 = min(max(ixp1,0), width-1);
	izp1 = min(max(izp1,0), height-1);

	int i00 = ix+iz*width;
	int i10 = ixp1+iz*width;
	int i01 = ix+izp1*width;
	int i11 = ixp1+izp1*width;

	float w00 = (1.0f-fx)*(1.0f-fz);
	float w10 = (fx)*(1.0f-fz);
	float w01 = (1.0f-fx)*(fz);
	float w11 = (fx)*(fz);

	float h = w00*terrainHeights[i00] +
		w10*terrainHeights[i10] +
		w01*terrainHeights[i01] +
		w11*terrainHeights[i11];

	if (h <= y) {
		normal = w00*terrainNormals[i00] +
			w10*terrainNormals[i10] +
			w01*terrainNormals[i01] +
			w11*terrainNormals[i11];
		normal.normalize();

	}

	return h;
}
