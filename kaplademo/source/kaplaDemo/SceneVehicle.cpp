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

#include "SceneVehicle.h"
#include "Convex.h"
#include "PxSimpleFactory.h"
#include "PxRigidStatic.h"
#include "PxShape.h"
#include "foundation/PxMathUtils.h"
#include "foundation/PxMat44.h"
#include "PsString.h"
#include "CmPhysxCommon.h"

#include <stdio.h>
#include <GL/glut.h>

#include "SimScene.h"
#include "CompoundCreator.h"
#include "TerrainMesh.h"

#include "PhysXMacros.h"

#include "PxRigidBodyExt.h"

#include "vehicle/PxVehicleSDK.h"
#include "vehicle/PxVehicleDrive4W.h"
#include "vehicle/PxVehicleUtilSetup.h"
#include "vehicle/PxVehicleUtil.h"
#include "SceneVehicleCooking.h"
#include "SceneVehicleSceneQuery.h"
#include "SampleViewerGamepad.h"
#include "RawLoader.h"
#include "TerrainRandomSamplePrecompute.h"
#include "MediaPath.h"
#include "glmesh.h"
#include "Texture.h"
#include "PxShapeExt.h"

static bool createVehicleDemo = true;



///////////////////////////////////
//VEHICLE START PARAMETERS
///////////////////////////////////


#define NUM_PLAYER_CARS 1
static PxTransform gPlayerCarStartTransforms[NUM_PLAYER_CARS] =
{
	PxTransform(PxVec3(-154.699753f, 9.863837f, 87.684113f), PxQuat(-0.000555f, -0.267015, 0.000155, -0.963692))
};

////////////////////////////////////////////////////////////////
//VEHICLE SETUP DATA 
////////////////////////////////////////////////////////////////

PxF32 gChassisMass = 1500.0f;
PxF32 gSuspensionShimHeight = 0.125f;


////////////////////////////////////////////////////////////////
//RENDER USER DATA TO ASSOCIATE EACH RENDER MESH WITH A 
//VEHICLE PHYSICS COMPONENT
////////////////////////////////////////////////////////////////

enum
{
	CAR_PART_FRONT_LEFT_WHEEL = 0,
	CAR_PART_FRONT_RIGHT_WHEEL,
	CAR_PART_REAR_LEFT_WHEEL,
	CAR_PART_REAR_RIGHT_WHEEL,
	CAR_PART_CHASSIS,
	CAR_PART_WINDOWS,
	NUM_CAR4W_RENDER_COMPONENTS,
	CAR_PART_EXTRA_WHEEL0 = NUM_CAR4W_RENDER_COMPONENTS,
	CAR_PART_EXTRA_WHEEL1,
	NUM_CAR6W_RENDER_COMPONENTS
};

enum MaterialID
{
	MATERIAL_TERRAIN_MUD = 1000,
	MATERIAL_ROAD_TARMAC = 1001,
	MATERIAL_ROAD_SNOW = 1002,
	MATERIAL_ROAD_GRASS = 1003,
};


static const char gCarPartNames[NUM_CAR4W_RENDER_COMPONENTS][64] =
{
	"frontwheelleftshape",
	"frontwheelrightshape",
	"backwheelleftshape",
	"backwheelrightshape",
	"car_02_visshape",
	"car_02_windowsshape"
};

////////////////////////////////////////////////////////////////
//TRANSFORM APPLIED TO CHASSIS RENDER MESH VERTS 
//THAT IS REQUIRED TO PLACE AABB OF CHASSIS RENDER MESH AT ORIGIN
//AT CENTRE-POINT OF WHEELS.
////////////////////////////////////////////////////////////////

static PxVec3 gChassisMeshTransform(0, 0, 0);

////////////////////////////////////////////////////////////////
//WHEEL CENTRE OFFSETS FROM CENTRE OF CHASSIS RENDER MESH AABB
//OF 4-WHEELED VEHICLE 
////////////////////////////////////////////////////////////////

static PxVec3 gWheelCentreOffsets4[4];

////////////////////////////////////////////////////////////////
//CONVEX HULL OF RENDER MESH FOR CHASSIS AND WHEELS OF
//4-WHEELED VEHICLE
////////////////////////////////////////////////////////////////

static PxConvexMesh* gChassisConvexMesh = NULL;
static PxConvexMesh* gWheelConvexMeshes4[4] = { NULL, NULL, NULL, NULL };
static GLMesh* gCarPartRenderMesh[6] = { NULL, NULL, NULL, NULL, NULL, NULL };

// ----------------------------------------------------------------------------------------------
SceneVehicle::SceneVehicle(PxPhysics* pxPhysics, PxCooking *pxCooking, bool isGrb,
	Shader *defaultShader, const char *resourcePath, float slowMotionFactor) :
	SceneKapla(pxPhysics, pxCooking, isGrb, defaultShader, resourcePath, slowMotionFactor)
{
	mChassisMaterialDrivable = NULL;
	mChassisMaterialNonDrivable = NULL;
	if (createVehicleDemo)
		mCameraDisable = true;
	else
		mCameraDisable = false;
	mNumTextures = 0;
	mNumMaterials = 0;
	//onInit(pxPhysics, pxCooking, pxScene);
}


void SceneVehicle::onInit(PxScene* pxScene)
{
	SceneKapla::onInit(pxScene);
	createStandardMaterials();
	if (createVehicleDemo)
		createVehicle(mPxPhysics);
	SceneKapla::createTerrain("terrain_ll2.bmp", "");

	const PxVec3 dims(0.08f, 0.25f, 1.0f);

	PxMaterial* DefaultMaterial = mPxPhysics->createMaterial(0.5f, 0.25f, 0.1f);

	ShaderMaterial mat;
	mat.init();

	PxFilterData simFilterData;
	simFilterData.word0 = COLLISION_FLAG_OBSTACLE;
	simFilterData.word1 = COLLISION_FLAG_OBSTACLE_AGAINST;
	//simFilterData.word2 = PxPairFlag::eMODIFY_CONTACTS;

	PxFilterData queryFilterData;
	//VehicleSetupDrivableShapeQueryFilterData(&queryFilterData);

	createCylindricalTower(96, 3.6f, 3.6f, 14, dims, PxVec3(-10.f, -1.10f, 20.f), DefaultMaterial, mat, simFilterData, queryFilterData, 10.f);
	createCylindricalTower(96, 5.8f, 5.8f, 10, dims, PxVec3(-10.f, -1.10f, 20.f), DefaultMaterial, mat, simFilterData, queryFilterData, 10.f);
	createCylindricalTower(128, 8.f, 8.f, 8, dims, PxVec3(-10.f, -1.10f, 20.f), DefaultMaterial, mat, simFilterData, queryFilterData, 10.f);
	createCylindricalTower(196, 10.2f, 10.2f, 6, dims, PxVec3(-10.f, -1.10f, 20.f), DefaultMaterial, mat, simFilterData, queryFilterData, 10.f);


	createCylindricalTower(384, 35.f, 35.f, 6, dims, PxVec3(0.f, -1.10f, 0.f), DefaultMaterial, mat, simFilterData, queryFilterData, 10.f);


	/*PxFilterData planeFilterData;
	simFilterData.word0 = COLLISION_FLAG_GROUND;
	simFilterData.word1 = COLLISION_FLAG_GROUND_AGAINST;

	VehicleSetupDrivableShapeQueryFilterData(&queryFilterData);

	createGroundPlane(planeFilterData, PxVec3(0,0,0), queryFilterData);

	PxTransform pose = mVehicleManager.getVehicle()->getRigidDynamicActor()->getGlobalPose();
	pose.p.y += 3.f;

	mVehicleManager.getVehicle()->getRigidDynamicActor()->setGlobalPose(pose);*/
	
}

void SceneVehicle::setScene(PxScene* pxScene)
{
	SceneKapla::setScene(pxScene);
	mVehicleManager.clearBatchQuery();
}

void SceneVehicle::createStandardMaterials()
{
	const PxF32 restitutions[MAX_NUM_SURFACE_TYPES] = { 0.2f, 0.2f, 0.2f, 0.2f };
	const PxF32 staticFrictions[MAX_NUM_SURFACE_TYPES] = { 0.5f, 0.5f, 0.5f, 0.5f };
	const PxF32 dynamicFrictions[MAX_NUM_SURFACE_TYPES] = { 0.5f, 0.5f, 0.5f, 0.5f };

	for (PxU32 i = 0; i<MAX_NUM_SURFACE_TYPES; i++)
	{
		//Create a new material.
		mStandardMaterials[i] = getPhysics().createMaterial(staticFrictions[i], dynamicFrictions[i], restitutions[i]);
		if (!mStandardMaterials[i])
		{
			printf("SceneVehicle : create mStandardMaterials failed");
		}

		//Set up the drivable surface type that will be used for the new material.
		mVehicleDrivableSurfaceTypes[i].mType = i;
	}

	mChassisMaterialDrivable = getPhysics().createMaterial(0.0f, 0.0f, 0.0f);
	if (!mChassisMaterialDrivable)
	{
		printf("SceneVehicle : create mChassisMaterialDrivable failed");
	}

	mChassisMaterialNonDrivable = getPhysics().createMaterial(1.0f, 1.0f, 0.0f);
	if (!mChassisMaterialNonDrivable)
	{
		printf("SceneVehicle : create mChassisMaterialNonDrivable failed");
	}
}

static void computeTerrain(bool* done, float* pVB, PxU32 x0, PxU32 y0, PxU32 currentSize, float value, PxU32 initSize, TerrainRandomSamplePrecompute& randomPrecomputed)
{
	// Compute new size
	currentSize >>= 1;
	if (currentSize > 0)
	{
		const PxU32 x1 = (x0 + currentSize) % initSize;
		const PxU32 x2 = (x0 + currentSize + currentSize) % initSize;
		const PxU32 y1 = (y0 + currentSize) % initSize;
		const PxU32 y2 = (y0 + currentSize + currentSize) % initSize;

		if (!done[x1 + y0*initSize])	pVB[(x1 + y0*initSize) * 9 + 1] = randomPrecomputed.getRandomInRange(-0.5f*value, 0.5f*value) + 0.5f * (pVB[(x0 + y0*initSize) * 9 + 1] + pVB[(x2 + y0*initSize) * 9 + 1]);
		if (!done[x0 + y1*initSize])	pVB[(x0 + y1*initSize) * 9 + 1] = randomPrecomputed.getRandomInRange(-0.5f*value, 0.5f*value) + 0.5f * (pVB[(x0 + y0*initSize) * 9 + 1] + pVB[(x0 + y2*initSize) * 9 + 1]);
		if (!done[x2 + y1*initSize])	pVB[(x2 + y1*initSize) * 9 + 1] = randomPrecomputed.getRandomInRange(-0.5f*value, 0.5f*value) + 0.5f * (pVB[(x2 + y0*initSize) * 9 + 1] + pVB[(x2 + y2*initSize) * 9 + 1]);
		if (!done[x1 + y2*initSize])	pVB[(x1 + y2*initSize) * 9 + 1] = randomPrecomputed.getRandomInRange(-0.5f*value, 0.5f*value) + 0.5f * (pVB[(x0 + y2*initSize) * 9 + 1] + pVB[(x2 + y2*initSize) * 9 + 1]);
		if (!done[x1 + y1*initSize])	pVB[(x1 + y1*initSize) * 9 + 1] = randomPrecomputed.getRandomInRange(-0.5f*value, 0.5f*value) + 0.5f * (pVB[(x0 + y1*initSize) * 9 + 1] + pVB[(x2 + y1*initSize) * 9 + 1]);

		done[x1 + y0*initSize] = true;
		done[x0 + y1*initSize] = true;
		done[x2 + y1*initSize] = true;
		done[x1 + y2*initSize] = true;
		done[x1 + y1*initSize] = true;

		// Recurse through 4 corners
		value *= 0.5f;
		computeTerrain(done, pVB, x0, y0, currentSize, value, initSize, randomPrecomputed);
		computeTerrain(done, pVB, x0, y1, currentSize, value, initSize, randomPrecomputed);
		computeTerrain(done, pVB, x1, y0, currentSize, value, initSize, randomPrecomputed);
		computeTerrain(done, pVB, x1, y1, currentSize, value, initSize, randomPrecomputed);
	}
}

void SceneVehicle::createTerrain(PxU32 size, float width, float chaos)
{
	mNbTerrainVerts = size*size;

	// Vertex buffer
	mTerrainVB = new PxF32[(sizeof(float)*mNbTerrainVerts * 3 * 3)];
	for (PxU32 y = 0; y<size; y++)
	{
		for (PxU32 x = 0; x<size; x++)
		{
			mTerrainVB[(x + y*size) * 9 + 0] = (float(x) - (float(size - 1)*0.5f))* width;
			mTerrainVB[(x + y*size) * 9 + 1] = 0.0f;
			mTerrainVB[(x + y*size) * 9 + 2] = (float(y) - (float(size - 1)*0.5f))* width;
			mTerrainVB[(x + y*size) * 9 + 3] = 0.0f; mTerrainVB[(x + y*size) * 9 + 4] = 1.0f; mTerrainVB[(x + y*size) * 9 + 5] = 0.0f;
			mTerrainVB[(x + y*size) * 9 + 6] = 0.5f; mTerrainVB[(x + y*size) * 9 + 7] = 0.4f; mTerrainVB[(x + y*size) * 9 + 8] = 0.2f;
		}
	}

	// Fractalize
	bool* doneBuffer = new bool[(sizeof(bool)*mNbTerrainVerts)];
	PxU32* tagBuffer = new PxU32[(sizeof(PxU32)*mNbTerrainVerts)];
	for (PxU32 i = 0; i<mNbTerrainVerts; i++)
	{
		doneBuffer[i] = false;
		tagBuffer[i] = 0;
	}
	mTerrainVB[1] = 10.0f;
	mTerrainVB[(size - 1) * 9 + 1] = 10.0f;
	mTerrainVB[(size*(size - 1)) * 9 + 1] = 10.0f;
	mTerrainVB[(mNbTerrainVerts - 1) * 9 + 1] = 10.0f;

	TerrainRandomSamplePrecompute randomPrecomputed(*this,  mResourcePath);
	computeTerrain(doneBuffer, mTerrainVB, 0, 0, size, chaos / 16.0f, size, randomPrecomputed);

	const PxU32 street0 = (PxU32)(size / 3.0f);
	const PxU32 streetSize = (PxU32)(size / 30.0f);
	float ay = 0.0f;

	for (PxU32 y = 0; y<size; y++)
	{
		for (PxU32 x = street0; x<street0 + streetSize; x++)
		{
			ay += mTerrainVB[(x + y*size) * 9 + 1];
			ay += mTerrainVB[(y + x*size) * 9 + 1];
		}
	}

	const float cx = size / 2.0f;
	const float cy = size / 2.0f;
	const float r = size / 3.0f;
	const float g = streetSize / 2.0f;

	for (PxU32 i = 0; i<mNbTerrainVerts; i++)
		tagBuffer[i] = false;

	ay /= streetSize*size;
	ay -= streetSize;
	for (PxU32 y = 15; y<size - 15; y++)
	{
		bool smoothBorder = true;

		for (PxU32 x = street0; x<street0 + streetSize; x++)
		{
			if (y > size*0.5f && y < size*0.7f)
			{
				mTerrainVB[(x + y*size) * 9 + 1] = ay + sinf(((float)y)*12.0f + 4.0f)*2.0f;
				smoothBorder = false;
			}
			else
			{
				mTerrainVB[(x + y*size) * 9 + 1] = ay;
			}

			if (y > size*0.55f && y < size*0.75f)
			{
				mTerrainVB[(y + x*size) * 9 + 1] = ay + sinf(y*12.0f)*0.75f;
				//mTerrainVB[(y+x*size)*9+1]=ay;
				tagBuffer[y + x*size] = 3;
				tagBuffer[x + y*size] = 3;
				smoothBorder = false;
			}
			else if (y < size*0.15f)
			{
				const float s = size*0.15f - (float)y;
				mTerrainVB[(y + x*size) * 9 + 1] = ay + s*0.25f;
				smoothBorder = false;
			}
			else if (y > size*0.85f)
			{
				const float s = (float)y - size*0.85f;
				mTerrainVB[(y + x*size) * 9 + 1] = ay + s*0.7f;
				smoothBorder = false;
			}
			else
			{
				mTerrainVB[(y + x*size) * 9 + 1] = ay;
				tagBuffer[y + x*size] = 1;
				tagBuffer[x + y*size] = 1;
			}

		}
		if (smoothBorder)
		{
			mTerrainVB[((street0 - 1) + y*size) * 9 + 1] = ay*0.5f + mTerrainVB[((street0 - 1) + y*size) * 9 + 1] * 0.5f;
			mTerrainVB[(y + (street0 - 1)*size) * 9 + 1] = ay*0.5f + mTerrainVB[(y + (street0 - 1)*size) * 9 + 1] * 0.5f;
			mTerrainVB[((street0 + 1) + y*size) * 9 + 1] = ay*0.5f + mTerrainVB[((street0 + 1) + y*size) * 9 + 1] * 0.5f;
			mTerrainVB[(y + (street0 + 1)*size) * 9 + 1] = ay*0.5f + mTerrainVB[(y + (street0 + 1)*size) * 9 + 1] * 0.5f;
		}
	}

	// Circle street
	for (PxU32 y = 0; y<size; y++)
	{
		for (PxU32 x = 0; x<size; x++)
		{
			const float x0 = x - cx;
			const float y0 = y - cy;
			const float d = sqrtf(x0*x0 + y0*y0);
			if (d >= r && d < r + streetSize)
			{
				mTerrainVB[(y + x*size) * 9 + 1] = ay;

				if (y > size*0.55f && y < size*0.75f)
					tagBuffer[y + x*size] = 2;
				else
					tagBuffer[y + x*size] = 1;

			}
			else if (d >= r + streetSize && d < r + streetSize + g)
			{
				const float a = (d - (r + streetSize)) / g;
				mTerrainVB[(y + x*size) * 9 + 1] = ay*(1.0f - a) + mTerrainVB[(y + x*size) * 9 + 1] * a;
			}
			else if (d >= r - g && d < r)
			{
				const float a = (d - (r - g)) / g;
				mTerrainVB[(y + x*size) * 9 + 1] = ay*a + mTerrainVB[(y + x*size) * 9 + 1] * (1.0f - a);
			}
		}
	}

	// Borders
	const float b = size / 25.0f;
	const float bd = size / 2.0f - b;
	for (PxU32 y = 0; y<size; y++)
	{
		for (PxU32 x = 0; x<size; x++)
		{
			const float x0 = fabsf(x - cx);
			const float y0 = fabsf(y - cy);
			if (x0 > bd || y0 > bd)
			{
				float a0 = (x0 - bd) / b;
				float a1 = (y0 - bd) / b;
				if (a1 > a0)
					a0 = a1;
				mTerrainVB[(y + x*size) * 9 + 1] = 20.0f*a0 + mTerrainVB[(y + x*size) * 9 + 1] * (1 - a0);
			}
		}
	}

	// Sobel filter
	for (PxU32 y = 1; y<size - 1; y++)
	{
		for (PxU32 x = 1; x<size - 1; x++)
		{
			//  1  0 -1
			//  2  0 -2
			//  1  0 -1
			float dx;
			dx = mTerrainVB[((x - 1) + (y - 1)*size) * 9 + 1];
			dx -= mTerrainVB[((x + 1) + (y - 1)*size) * 9 + 1];
			dx += 2.0f*mTerrainVB[((x - 1) + (y + 0)*size) * 9 + 1];
			dx -= 2.0f*mTerrainVB[((x + 1) + (y + 0)*size) * 9 + 1];
			dx += mTerrainVB[((x - 1) + (y + 1)*size) * 9 + 1];
			dx -= mTerrainVB[((x + 1) + (y + 1)*size) * 9 + 1];

			//  1  2  1
			//  0  0  0
			// -1 -2 -1
			float dy;
			dy = mTerrainVB[((x - 1) + (y - 1)*size) * 9 + 1];
			dy += 2.0f*mTerrainVB[((x + 0) + (y - 1)*size) * 9 + 1];
			dy += mTerrainVB[((x + 1) + (y - 1)*size) * 9 + 1];
			dy -= mTerrainVB[((x - 1) + (y + 1)*size) * 9 + 1];
			dy -= 2.0f*mTerrainVB[((x + 0) + (y + 1)*size) * 9 + 1];
			dy -= mTerrainVB[((x + 1) + (y + 1)*size) * 9 + 1];

			const float nx = dx / width*0.15f;
			const float ny = 1.0f;
			const float nz = dy / width*0.15f;

			const float len = sqrtf(nx*nx + ny*ny + nz*nz);

			mTerrainVB[(x + y*size) * 9 + 3] = nx / len;
			mTerrainVB[(x + y*size) * 9 + 4] = ny / len;
			mTerrainVB[(x + y*size) * 9 + 5] = nz / len;
		}
	}

	// Static lighting (two directional lights)
	const float l0[3] = { 0.25f / 0.8292f, 0.75f / 0.8292f, 0.25f / 0.8292f };
	const float l1[3] = { 0.65f / 0.963f, 0.55f / 0.963f, 0.45f / 0.963f };
	//const float len = sqrtf(l1[0]*l1[0]+l1[1]*l1[1]+l1[2]*l1[2]);
	for (PxU32 y = 0; y<size; y++)
	{
		for (PxU32 x = 0; x<size; x++)
		{
			const float nx = mTerrainVB[(x + y*size) * 9 + 3], ny = mTerrainVB[(x + y*size) * 9 + 4], nz = mTerrainVB[(x + y*size) * 9 + 5];

			const float a = 0.3f;
			float dot0 = l0[0] * nx + l0[1] * ny + l0[2] * nz;
			float dot1 = l1[0] * nx + l1[1] * ny + l1[2] * nz;
			if (dot0 < 0.0f) { dot0 = 0.0f; }
			if (dot1 < 0.0f) { dot1 = 0.0f; }

			const float l = dot0*0.7f + dot1*0.3f;

			mTerrainVB[(x + y*size) * 9 + 6] = mTerrainVB[(x + y*size) * 9 + 6] * (l + a);
			mTerrainVB[(x + y*size) * 9 + 7] = mTerrainVB[(x + y*size) * 9 + 7] * (l + a);
			mTerrainVB[(x + y*size) * 9 + 8] = mTerrainVB[(x + y*size) * 9 + 8] * (l + a);

			/*mTerrainVB[(x+y*size)*9+3] = 0.0f;
			mTerrainVB[(x+y*size)*9+4] = -1.0f;
			mTerrainVB[(x+y*size)*9+5] = 0.0f;*/
		}
	}

	// Index buffers
	const PxU32 maxNbTerrainTriangles = (size - 1)*(size - 1) * 2;

	mNbIB = 4;
	mRenderMaterial[0] = MATERIAL_TERRAIN_MUD;
	mRenderMaterial[1] = MATERIAL_ROAD_TARMAC;
	mRenderMaterial[2] = MATERIAL_ROAD_SNOW;
	mRenderMaterial[3] = MATERIAL_ROAD_GRASS;

	for (PxU32 i = 0; i<mNbIB; i++)
	{
		mIB[i] = new PxU32[(sizeof(PxU32)*maxNbTerrainTriangles * 3)];
		mNbTriangles[i] = 0;
	}

	for (PxU32 j = 0; j<size - 1; j++)
	{
		for (PxU32 i = 0; i<size - 1; i++)
		{
			PxU32 tris[6];
			tris[0] = i + j*size; tris[1] = i + (j + 1)*size; tris[2] = i + 1 + (j + 1)*size;
			tris[3] = i + j*size; tris[4] = i + 1 + (j + 1)*size; tris[5] = i + 1 + j*size;

			for (PxU32 t = 0; t<2; t++)
			{
				const PxU32 vt0 = tagBuffer[tris[t * 3 + 0]];
				const PxU32 vt1 = tagBuffer[tris[t * 3 + 1]];
				const PxU32 vt2 = tagBuffer[tris[t * 3 + 2]];

				PxU32 buffer = 0;
				if (vt0 == vt1 && vt0 == vt2)
					buffer = vt0;

				mIB[buffer][mNbTriangles[buffer] * 3 + 0] = tris[t * 3 + 0];
				mIB[buffer][mNbTriangles[buffer] * 3 + 1] = tris[t * 3 + 1];
				mIB[buffer][mNbTriangles[buffer] * 3 + 2] = tris[t * 3 + 2];
				mNbTriangles[buffer]++;
			}
		}
	}

	delete[] tagBuffer;
	delete[] doneBuffer;
}


void SceneVehicle::newMesh(const RAWMesh& data)
{
	
	PxU32 carPart = 0xffffffff;
	if (0 == Ps::strcmp(data.mName, gCarPartNames[CAR_PART_CHASSIS]))
	{
		carPart = CAR_PART_CHASSIS;

		//Find the min and max of the set of verts.
		PxVec3 vmin(PX_MAX_F32, PX_MAX_F32, PX_MAX_F32);
		PxVec3 vmax(-PX_MAX_F32, -PX_MAX_F32, -PX_MAX_F32);
		for (PxU32 i = 0; i < data.mNbVerts; i++)
		{
			vmin.x = PxMin(vmin.x, data.mVerts[i].x);
			vmin.y = PxMin(vmin.y, data.mVerts[i].y);
			vmin.z = PxMin(vmin.z, data.mVerts[i].z);
			vmax.x = PxMax(vmax.x, data.mVerts[i].x);
			vmax.y = PxMax(vmax.y, data.mVerts[i].y);
			vmax.z = PxMax(vmax.z, data.mVerts[i].z);
		}

		//Make sure the chassis has an aabb that is centred at (0,0,0).
		//This just makes it a lot easier to set the centre of mass offset relative to the centre of the actor.
		gChassisMeshTransform = (vmin + vmax)*0.5f;

		//Make sure wheel offsets are symmetric (they are not quite symmetric left to right or front to back).
		gWheelCentreOffsets4[CAR_PART_FRONT_LEFT_WHEEL].x = -gWheelCentreOffsets4[CAR_PART_FRONT_RIGHT_WHEEL].x;
		gWheelCentreOffsets4[CAR_PART_REAR_LEFT_WHEEL].x = -gWheelCentreOffsets4[CAR_PART_REAR_RIGHT_WHEEL].x;
		gWheelCentreOffsets4[CAR_PART_FRONT_LEFT_WHEEL].y = gWheelCentreOffsets4[CAR_PART_REAR_LEFT_WHEEL].y;
		gWheelCentreOffsets4[CAR_PART_FRONT_RIGHT_WHEEL].y = gWheelCentreOffsets4[CAR_PART_REAR_RIGHT_WHEEL].y;

		//Make an adjustment so that the the centre of the four wheels is at the origin.
		//Again, this just makes it a lot easier to set the centre of mass because we have a known point 
		//that represents the origin.  Also, it kind of makes sense that the default centre of mass is at the 
		//centre point of the wheels.
		PxF32 wheelOffsetZ = 0;
		for (PxU32 i = 0; i <= CAR_PART_REAR_RIGHT_WHEEL; i++)
		{
			wheelOffsetZ += gWheelCentreOffsets4[i].z;
		}
		wheelOffsetZ *= 0.25f;
		gChassisMeshTransform.z += wheelOffsetZ;

		//Reposition the mesh verts.
		PxVec3* verts = const_cast<PxVec3*>(data.mVerts);
		for (PxU32 i = 0; i < data.mNbVerts; i++)
		{
			verts[i] -= gChassisMeshTransform;
		}

		//Need a convex mesh for the chassis.
		gChassisConvexMesh = createChassisConvexMesh(data.mVerts, data.mNbVerts, getPhysics(), getCooking());
	}
	else if (0 == Ps::strcmp(data.mName, gCarPartNames[CAR_PART_FRONT_LEFT_WHEEL]))
	{
		carPart = CAR_PART_FRONT_LEFT_WHEEL;
		gWheelConvexMeshes4[CAR_PART_FRONT_LEFT_WHEEL] = createWheelConvexMesh(data.mVerts, data.mNbVerts, getPhysics(), getCooking());
	}
	else if (0 == Ps::strcmp(data.mName, gCarPartNames[CAR_PART_FRONT_RIGHT_WHEEL]))
	{
		carPart = CAR_PART_FRONT_RIGHT_WHEEL;
		gWheelConvexMeshes4[CAR_PART_FRONT_RIGHT_WHEEL] = createWheelConvexMesh(data.mVerts, data.mNbVerts, getPhysics(), getCooking());
	}
	else if (0 == Ps::strcmp(data.mName, gCarPartNames[CAR_PART_REAR_LEFT_WHEEL]))
	{
		carPart = CAR_PART_REAR_LEFT_WHEEL;
		gWheelConvexMeshes4[CAR_PART_REAR_LEFT_WHEEL] = createWheelConvexMesh(data.mVerts, data.mNbVerts, getPhysics(), getCooking());
	}
	else if (0 == Ps::strcmp(data.mName, gCarPartNames[CAR_PART_REAR_RIGHT_WHEEL]))
	{
		carPart = CAR_PART_REAR_RIGHT_WHEEL;
		gWheelConvexMeshes4[CAR_PART_REAR_RIGHT_WHEEL] = createWheelConvexMesh(data.mVerts, data.mNbVerts, getPhysics(), getCooking());
	}
	else if (0 == Ps::strcmp(data.mName, gCarPartNames[CAR_PART_WINDOWS]))
	{
		carPart = CAR_PART_WINDOWS;
		//Take the offset that was required to centre the chassis and apply it to everything that is dependent on the chassis transform.
		PxVec3* verts = const_cast<PxVec3*>(data.mVerts);
		for (PxU32 i = 0; i<data.mNbVerts; i++)
		{
			verts[i] -= gChassisMeshTransform;
		}
		for (PxU32 i = 0; i <= CAR_PART_REAR_RIGHT_WHEEL; i++)
		{
			gWheelCentreOffsets4[i] -= gChassisMeshTransform;
		}
	}

	gCarPartRenderMesh[carPart] = createRenderMesh(data);

	//Store the wheel offsets from the centre.
	for (PxU32 i = 0; i <= CAR_PART_REAR_RIGHT_WHEEL; i++)
	{
		if (0 == Ps::strcmp(data.mName, gCarPartNames[i]))
		{
			// gWheelCentreOffsets4[i] = meshActor->getTransform().p;

			gWheelCentreOffsets4[i] = data.mTransform.p;
			gWheelCentreOffsets4[i].y -= gSuspensionShimHeight;

			//The left and right wheels seem to be mixed up.
			//Swap them to correct this.
			gWheelCentreOffsets4[i].x *= -1.0f;
		}
	}
	
}


void SceneVehicle::importRAWFile(const char* fname, PxReal scale, bool recook)
{
	
	bool status = loadRAWfile(FindMediaFile(fname, mResourcePath), *this, scale);
	if (!status)
	{
		std::string msg = "Sample can not load file ";
		msg += FindMediaFile(fname, mResourcePath);
		msg += "\n";
		printf("%s", msg.c_str());
	}
}

void SceneVehicle::createVehicle(PxPhysics* pxPhysics)
{
	//Initialise the sdk.

	mVehicleManager.init(getPhysics(), (const PxMaterial**)mStandardMaterials, mVehicleDrivableSurfaceTypes);

	importRAWFile("car2.raw", 1.0f);
	
	mVehicleManager.create4WVehicle(getScene(), getPhysics(), getCooking(), *mChassisMaterialDrivable, gChassisMass, gWheelCentreOffsets4, gChassisConvexMesh,
		gWheelConvexMeshes4, gPlayerCarStartTransforms[0], true);
}

//-----------------------------------------------------------------------------------------------

void SceneVehicle::newMaterial(const RAWMaterial& data)
{
	ShaderMaterial& materials = mCarPartMaterial[mNumMaterials++];
	materials.init(mTextureIds[mNumMaterials - 1]);
	//materials.setColor(data.mDiffuseColor.x, data.mDiffuseColor.y, data.mDiffuseColor.z);
	materials.setColor(1, 0, 0);
	materials.diffuseCoeff = 0.5f;
	materials.specularCoeff = 0.707f;
}

//-----------------------------------------------------------------------------------------------

void SceneVehicle::newTexture(const RAWTexture& data)
{
	char fullPath[512];
	int len = strlen(data.mName);
	for (PxU32 i = 0; i < len-4; ++i)
	{
		fullPath[i] = data.mName[i];
	}
	fullPath[len - 4] = '\0';
	strcat(fullPath, ".bmp");


	string fName = FindMediaFile(fullPath, mResourcePath);
	//string fName = FindMediaFile("car02_diffuse2.bmp", mResourcePath);

	mTextureIds[mNumTextures++] = loadImgTexture(fName.c_str());

}

// ----------------------------------------------------------------------------------------------
void SceneVehicle::handleMouseButton(int button, int state, int x, int y)
{
	mMouseX = x;
	mMouseY = y;

	mMouseDown = (state == GLUT_DOWN);

	PxVec3 orig, dir;
	getMouseRay(x, y, orig, dir);

	if (glutGetModifiers() & GLUT_ACTIVE_SHIFT) {
		if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
			if (mSimScene)
				mSimScene->pickStart(orig, dir);
		}
	}
	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
		if (mSimScene)
			mSimScene->pickRelease();
	}
}

// ----------------------------------------------------------------------------------------------
void SceneVehicle::handleMouseMotion(int x, int y)
{
	mMouseX = x;
	mMouseY = y;

	if (mSimScene && mSimScene->getPickActor() != NULL) {
		PxVec3 orig, dir;
		getMouseRay(x, y, orig, dir);
		mSimScene->pickMove(orig, dir);
	}
}
//ofstream orays("ray.txt");

// ----------------------------------------------------------------------------------------------
void SceneVehicle::handleKeyDown(unsigned char key, int x, int y)
{
	if (mCameraDisable)
	{
		switch (key)
		{
		case ' ':
		{
			float vel = 1.0f;
			PxVec3 orig, dir;
			getMouseRay(mMouseX, mMouseY, orig, dir);
			/*
			float sx = 0.0f, sy = 30.0f, sz = 0.0f;
			int nx = 10;
			int ny = 10;
			float dx = 1.0f;
			for (int i = 0; i < ny; i++) {
			for (int j = 0; j < nx; j++) {
			shoot(PxVec3(sx+j*dx, sy, sz + i*dx), PxVec3(0.0f,1.0f,0.0f));
			}
			}

			*/
			shoot(mCameraPos, dir);
			//orays<<"{"<<mCameraPos.x<<","<<mCameraPos.y<<","<<mCameraPos.z<<","<<dir.x<<","<<dir.y<<","<<dir.z<<"},\n";
			//orays.flush();
			//printf("{%f,%f,%f,%f,%f,%f},\n ", mCameraPos.x, mCameraPos.y, mCameraPos.z, dir.x, dir.y, dir.z);
			break;
		}
		case 'w':
		case 'W':
		{
			mControlInputs.setAccelKeyPressed(true);
			break;
		}
		case 's':
		case 'S':
		{
			mControlInputs.setBrakeKeyPressed(true);
			break;
		}
		case 'a':
		case 'A':
		{
			mControlInputs.setSteerRightKeyPressed(true);

			break;
		}
		case 'd':
		case 'D':
		{
			mControlInputs.setSteerLeftKeyPressed(true);
			break;
		}
		default:
		{
			break;
		}
		}
	}
	
}

// ----------------------------------------------------------------------------------------------
void SceneVehicle::handleKeyUp(unsigned char key, int x, int y)
{
	if (mCameraDisable)
	{
		switch (key)
		{
		case 'v':
			if (mSimScene) mSimScene->toggleDebugDrawing(); break;
		case ' ':
		{
			mGunActive = false;
		}
		case 'w':
		case 'W':
		{
			mControlInputs.setAccelKeyPressed(false);
			break;
		}
		case 's':
		case 'S':
		{
			mControlInputs.setBrakeKeyPressed(false);
			break;
		}
		case 'a':
		case 'A':
		{
			mControlInputs.setSteerRightKeyPressed(false);

			break;
		}
		case 'd':
		case 'D':
		{
			mControlInputs.setSteerLeftKeyPressed(false);
			break;
		}
		default:
		{
			break;
		}
		}
	}
}

// ----------------------------------------------------------------------------------------------
void SceneVehicle::handleSpecialKey(unsigned char key, int x, int y)
{
	SceneKapla::handleSpecialKey(key, x, y);
	switch (key)
	{
	case GLUT_KEY_F7: {
		mCameraDisable = !mCameraDisable;
		break;
	}					  
	}
}

//-------------------------------------------------------------------------------------------------

void SceneVehicle::handleGamepadButton(int button, bool state)
{
	if (mCameraDisable)
	{
		switch (button)
		{
		case SampleViewerGamepad::GAMEPAD_A:
		{
			float vel = 1.0f;
			PxVec3 orig, dir;
			getMouseRay(mMouseX, mMouseY, orig, dir);
			/*
			float sx = 0.0f, sy = 30.0f, sz = 0.0f;
			int nx = 10;
			int ny = 10;
			float dx = 1.0f;
			for (int i = 0; i < ny; i++) {
			for (int j = 0; j < nx; j++) {
			shoot(PxVec3(sx+j*dx, sy, sz + i*dx), PxVec3(0.0f,1.0f,0.0f));
			}
			}

			*/
			shoot(mCameraPos, dir);
			//orays<<"{"<<mCameraPos.x<<","<<mCameraPos.y<<","<<mCameraPos.z<<","<<dir.x<<","<<dir.y<<","<<dir.z<<"},\n";
			//orays.flush();
			//printf("{%f,%f,%f,%f,%f,%f},\n ", mCameraPos.x, mCameraPos.y, mCameraPos.z, dir.x, dir.y, dir.z);
			break;
		}
		case SampleViewerGamepad::GAMEPAD_DPAD_LEFT:
		{
			mControlInputs.setGearDown(state);
			break;
		}
		case SampleViewerGamepad::GAMEPAD_DPAD_RIGHT:
		{
			mControlInputs.setGearUp(state);
			break;
		}		
		case SampleViewerGamepad::GAMEPAD_DPAD_DOWN:
		{
			mControlInputs.setHandbrake(state);
			break;
		}
		default:
		{
			break;
		}
		}
	}

}

//-------------------------------------------------------------------------------------------------

void SceneVehicle::handleGamepadAxis(int axis, float val)
{
	switch (axis)
	{
	case SampleViewerGamepad::GAMEPAD_LEFT_STICK_X:
	{
		mControlInputs.setSteer(-val);
		break;
	}
	default:
		break;
	}
}

//-------------------------------------------------------------------------------------------------

void SceneVehicle::handleGamepadTrigger(int trigger, float val)
{
	if(trigger == SampleViewerGamepad::GAMEPAD_RIGHT_SHOULDER_TRIGGER)
	{
		mControlInputs.setAccel(val);
	}
	else
	{
		if (trigger == SampleViewerGamepad::GAMEPAD_LEFT_SHOULDER_TRIGGER)
		{
			mControlInputs.setBrake(val);
		}
	}
}

//-------------------------------------------------------------------------------------------------
void SceneVehicle::getCamera(PxVec3& pos, PxVec3& dir)
{
	if (mCameraDisable)
	{
		pos = mCameraPos;
		dir = mCameraDir.getNormalized();
	}
	
}

// ----------------------------------------------------------------------------------------------
void SceneVehicle::preSim(float dt)
{
	SceneKapla::preSim(dt);
	
	if (createVehicleDemo)
	{
		mVehicleController.setCarKeyboardInputs(
			mControlInputs.getAccelKeyPressed(),
			mControlInputs.getBrakeKeyPressed(),
			mControlInputs.getHandbrakeKeyPressed(),
			mControlInputs.getSteerLeftKeyPressed(),
			mControlInputs.getSteerRightKeyPressed(),
			mControlInputs.getGearUpKeyPressed(),
			mControlInputs.getGearDownKeyPressed());

		mVehicleController.setCarGamepadInputs(
			mControlInputs.getAccel(),
			mControlInputs.getBrake(),
			mControlInputs.getSteer(),
			mControlInputs.getGearUp(),
			mControlInputs.getGearDown(),
			mControlInputs.getHandbrake());

			mVehicleController.update(dt, mVehicleManager.getWheelQueryResult(), *mVehicleManager.getVehicle());
	}

	

}

// ----------------------------------------------------------------------------------------------
void SceneVehicle::postSim(float dt)
{

	SceneKapla::postSim(dt);

	if (createVehicleDemo)
	{
		//update vehicle
		//mVehicleManager.suspensionRaycasts(&getScene());
		mVehicleManager.suspensionSweeps(&getScene());

		if (dt > 0.f)
		{
			mVehicleManager.update(dt, getScene().getGravity());
		}

		//Update the camera.
		mCameraController.setInputs(
			mControlInputs.getRotateY(),
			mControlInputs.getRotateZ());

		mCameraController.update(dt, *mVehicleManager.getVehicle(), getScene());

		setCamera(mCameraController.getCameraPos(), mCameraController.getCameraTar() - mCameraController.getCameraPos(), PxVec3(0.f, 1.f, 0.f), 40.f);
	}

	
}

void SceneVehicle::renderCar(bool useShader)
{
	PxRigidDynamic* dyn = mVehicleManager.getVehicle()->getRigidDynamicActor();

	PxShape* shapes[5];

	dyn->getShapes(shapes, 5);

	PxMat44 bodyGlobalPose(dyn->getGlobalPose());// PxShapeExt::getGlobalPose(*shapes[i], *dyn));

	if (useShader)
		mDefaultShader->activate(mCarPartMaterial[0]);

	for (PxU32 i = 0; i < 5; ++i)
	{

		PxMat44 globalPose(PxShapeExt::getGlobalPose(*shapes[i], *dyn));

		glMatrixMode(GL_MODELVIEW);

		glPushMatrix();

		glMultMatrixf(&globalPose.column0.x);

		gCarPartRenderMesh[i]->drawVBOIBO();

		glPopMatrix();

	}

	if (useShader)
		mDefaultShader->deactivate();

	
	if (useShader)
		mDefaultShader->activate(mCarPartMaterial[1]);

	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();

	glMultMatrixf(&bodyGlobalPose.column0.x);

	gCarPartRenderMesh[5]->drawVBOIBO();

	glPopMatrix();

	if (useShader)
		mDefaultShader->deactivate();

}

void SceneVehicle::render(bool useShader)
{
	if (createVehicleDemo)
		renderCar(useShader);

	SceneKapla::render(useShader);
}


PxFilterFlags VehicleFilterShader
(PxFilterObjectAttributes attributes0, PxFilterData filterData0,
PxFilterObjectAttributes attributes1, PxFilterData filterData1,
PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	PX_UNUSED(attributes0);
	PX_UNUSED(attributes1);
	PX_UNUSED(constantBlock);
	PX_UNUSED(constantBlockSize);

	if ((0 == (filterData0.word0 & filterData1.word1)) && (0 == (filterData1.word0 & filterData0.word1)))
	{
		if (filterData0.word0 != 0 && filterData1.word0 != 0)
			return PxFilterFlag::eSUPPRESS;
	}

	pairFlags = PxPairFlag::eCONTACT_DEFAULT;
	pairFlags |= PxPairFlags(PxU16(filterData0.word2 | filterData1.word2));

	return PxFilterFlags();

	//pairFlags = PxPairFlag::eDETECT_DISCRETE_CONTACT;
	//return PxFilterFlags();
}

struct ActorUserData
{
	ActorUserData()
		: vehicle(NULL),
		actor(NULL)
	{
	}

	const PxVehicleWheels* vehicle;
	const PxActor* actor;
};

struct ShapeUserData
{
	ShapeUserData()
		: isWheel(false),
		wheelId(0xffffffff)
	{
	}

	bool isWheel;
	PxU32 wheelId;
};

ActorUserData gActorUserData;
ShapeUserData gShapeUserDatas[PX_MAX_NB_WHEELS];

#define POINT_REJECT_ANGLE 45.0f*PxPi/180.0f
#define NORMAL_REJECT_ANGLE 30.0f*PxPi/180.0f
#define WHEEL_TANGENT_VELOCITY_MULTIPLIER 0.1f

//The class WheelContactModifyCallback identifies and modifies rigid body contacts
//that involve a wheel.  Contacts that can be identified and managed by the suspension
//system are ignored.  Any contacts that remain are modified to account for the rotation
//speed of the wheel around the rolling axis.
class WheelContactModifyCallback : public PxContactModifyCallback
{
public:

	WheelContactModifyCallback()
		: PxContactModifyCallback()
	{
	}

	~WheelContactModifyCallback(){}

	void onContactModify(PxContactModifyPair* const pairs, PxU32 count)
	{
		for (PxU32 i = 0; i < count; i++)
		{
			const PxRigidActor** actors = pairs[i].actor;
			const PxShape** shapes = pairs[i].shape;

			//Search for actors that represent vehicles and shapes that represent wheels.
			for (PxU32 j = 0; j < 2; j++)
			{
				const PxActor* actor = actors[j];
				if (actor->userData && (static_cast<ActorUserData*>(actor->userData))->vehicle)
				{
					const PxVehicleWheels* vehicle = (static_cast<ActorUserData*>(actor->userData))->vehicle;
					PX_ASSERT(vehicle->getRigidDynamicActor() == actors[j]);

					const PxShape* shape = shapes[j];
					if (shape->userData && (static_cast<ShapeUserData*>(shape->userData))->isWheel)
					{
						const PxU32 wheelId = (static_cast<ShapeUserData*>(shape->userData))->wheelId;
						PX_ASSERT(wheelId < vehicle->mWheelsSimData.getNbWheels());

						//Modify wheel contacts.
						PxVehicleModifyWheelContacts(*vehicle, wheelId, WHEEL_TANGENT_VELOCITY_MULTIPLIER, 0.5f, pairs[i]);
					}
				}
			}
		}
	}


private:

};
WheelContactModifyCallback gWheelContactModifyCallback;

//----------------------------------------------------------------------------------------------------
void SceneVehicle::customizeSceneDesc(PxSceneDesc& sceneDesc)
{
	sceneDesc.filterShader = VehicleFilterShader;							//Set the filter shader
	sceneDesc.contactModifyCallback = &gWheelContactModifyCallback;			//Enable contact modification
}

