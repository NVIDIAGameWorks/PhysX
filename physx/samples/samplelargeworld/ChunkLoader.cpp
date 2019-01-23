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

#include "PxTkRandom.h"
#include "RenderMeshActor.h"
#include "SampleDirManager.h"
#include "ChunkLoader.h"
#include "SampleLargeWorld.h"
#include <algorithm>
#include "MeshBuilder.h"
#include "PsMathUtils.h"
#include "PxTkFile.h"
#include "wavefront.h"
#include "PsString.h"
#include "PxScene.h"
#include "geometry/PxGeometryQuery.h"
#include "extensions/PxCollectionExt.h"

#ifdef _MSC_VER
#pragma warning(disable:4305)
#endif

using namespace PxToolkit;
using namespace physx;
using namespace shdfnd;

#define MATERIAL_ID			0x01

#define CHUNK_DEBRIS_NUM	1
#define	ADD_TREES			1
#define ADD_WIND_MILL		1

static const ChunkID FARM_CHUNKID		= {{4, 3}};
static const ChunkID CITY_CHUNKID		= {{4, 2}};
static const ChunkID FORTRESS_CHUNKID	= {{1, 2}};
static const ChunkID FOREST_CHUNKID		= {{5, 3}};

static const char* gFarmName		= "Farm";
static const char* gCityName		= "City";
static const char* gTreeName		= "Tree";

static const float gFortrestScale	= 0.05f;

static const PxBounds3 gCityBounds( PxVec3(927.65833,17.403145,452.00970), PxVec3(1119.6967,256.83026,576.55121));
static const PxBounds3 gFarmBounds( PxVec3(1001.8151,9.4852743,732.46741), PxVec3(1036.2266,16.729010,787.07898));
static const PxBounds3 gFortressBounds( PxVec3(-584.924011,0.000122,-586.000305), PxVec3(584.924011,1875.750488,585.999939));

static const PxVec3 gCityPos(1024.0000,17.414299,512.00000);
static const PxVec3 gFarmPos(1024.0000, 9.4852762, 768.0f);
static const PxVec3 gFortressPos(256.00000,81.219383,512.000000);

static PX_INLINE ChunkID getChunkIdFromPosition(const PxVec3& position, CoordType terrainDim, PxF32 terrainSize)
{
	CoordType xCoor = (CoordType)((position.x + terrainSize*0.5f) / terrainSize);	
	xCoor = PxClamp((CoordType)xCoor, (CoordType)0, (CoordType)(terrainDim -1));
	
	CoordType yCoor = (CoordType)((position.z + terrainSize*0.5f) / terrainSize);
	yCoor = PxClamp((CoordType)yCoor, (CoordType)0, (CoordType)(terrainDim -1));

	ChunkID res;
	res.coord.x = xCoor;
	res.coord.y = yCoor;

	return res;
}

static PxI32 getFileSize(const char* name)
{
	if(!name)	
		return 0;

	SampleFramework::File* fp = NULL;
	PxToolkit::fopen_s(&fp, name, "rb");
	if( !fp )
		return 0;

	fseek(fp, 0, SEEK_END);
	PxI32 filesize = (PxI32)ftell(fp);
	fclose(fp);
	return filesize;
}

void SampleLargeWorld::addWindMills(const PxTriangleMeshGeometry& meshGeom, BasicRandom& random, const PxBounds3& bound, PxCollection& outCollection)
{
	PxPhysics& physics = getPhysics();
	PxMaterial& material = getDefaultMaterial();

	const PxReal millHalfHeight = 20.0f;
	const PxReal millRadius =9.0f;
	const PxReal fanRadius = 22.0f;
	const PxReal fanWidth = 3.0f;
	const PxReal fanThin = 0.5f;
	const PxReal axisHalfHeight = 5.0f;
	const PxReal axisRadius = 1.0f;
	const PxVec3 millExtent(
		millRadius*0.5f + axisHalfHeight + axisRadius + fanThin,
		fanRadius*0.5f + millRadius + millHalfHeight,
		millRadius*0.5f + axisHalfHeight + axisRadius + fanThin
		) ;

	PxBounds3 millBornBound = bound;
	millBornBound.fattenFast(-millRadius*2.0f);

	//Store mill position to check if new mill is overlaped with old mills
	PxVec3 millPts[5];

	PxU32 mills = 0, retryNum = 1;
	do
	{
		//Raycast against terrain from random pos to get mill foot point
		PxVec3 rayStart(
			random.rand( millBornBound.minimum.x, millBornBound.maximum.x),
			bound.maximum.y,
			random.rand( millBornBound.minimum.z, millBornBound.maximum.z));

		PxReal maxDist = 2*(bound.maximum.y - bound.minimum.y);				

		PxRaycastHit hit;

		PxU32 hitsNum = PxGeometryQuery::raycast(
												rayStart, PxVec3(0.0f,-1.0f,0.0f), 
												meshGeom, PxTransform(PxIdentity), maxDist, 
												PxHitFlag::ePOSITION, 1, &hit);

		if( hitsNum )
		{
			PxVec3 millPt = hit.position;
			millPt.y += millHalfHeight - millRadius;

			PxVec3 axisPt = millPt;
			axisPt.y += millHalfHeight + millRadius;
			axisPt.x += millRadius;

			PxVec3 fanPt = axisPt;
			fanPt.x += axisHalfHeight;

		    //Roughly check if fans of mill collided with terrain	
			PxQueryFilterData filter( PxQueryFlag::eANY_HIT);
			PxOverlapBuffer buf;
			PxBoxGeometry fanBox(fanThin, fanRadius, fanRadius);
			if(PxGeometryQuery::overlap(fanBox, PxTransform(fanPt), meshGeom, PxTransform(PxIdentity)))			
			{
				continue;
			}


			PxBounds3 curBounds = PxBounds3::centerExtents( millPt, millExtent);

			bool needRegenerate = false;

			for(PxU32 t = 0; t < mills; t++)
			{
				PxBounds3 otherBounds = PxBounds3::centerExtents( millPts[t], millExtent);
				if( otherBounds.intersects(curBounds) )
					needRegenerate = true;
			}

			if(needRegenerate)
			{
				continue;
			}
			
			millPts[mills] = millPt;
			millPts[mills].x = millPts[mills].x + millRadius - millExtent.x;
			++mills;

			PxCapsuleGeometry millGeom(millRadius, millHalfHeight);
			PxCapsuleGeometry axisGeom(axisRadius, axisHalfHeight);
			PxBoxGeometry fanGeom(fanRadius, fanWidth, fanThin);

			PxQuat verticleRote( PxHalfPi, PxVec3(0.0f,0.0f,1.0f));
			PxQuat y90Rote( PxHalfPi, PxVec3(0.0f,1.0f,0.0f));

			PxRigidStatic* millBody = physics.createRigidStatic(PxTransform(millPt));
			PxShape* millShape0 = PxRigidActorExt::createExclusiveShape(*millBody, millGeom, material);
			millShape0->setLocalPose(PxTransform(verticleRote));
			PxShape* millShape1 = PxRigidActorExt::createExclusiveShape(*millBody, axisGeom, material);
			millShape1->setLocalPose(PxTransform(axisPt - millPt));

			PxRigidDynamic* fan = physics.createRigidDynamic(PxTransform(fanPt));
			PxShape* fanShape0 = PxRigidActorExt::createExclusiveShape(*fan, fanGeom, material);
			fanShape0->setLocalPose(PxTransform(PxIdentity));
			PxShape* fanShape1 = PxRigidActorExt::createExclusiveShape(*fan, fanGeom, material);
			fanShape1->setLocalPose(PxTransform(verticleRote));

			//Disable the collision between fans and other objects in scene
			setCollisionGroup(fan, FAN_COLLISION_GROUP);

			PxRevoluteJoint* joint = PxRevoluteJointCreate(physics,
				millBody, PxTransform(fanPt - millPt),
				fan, PxTransform(PxVec3(0), y90Rote));

			joint->setDriveVelocity(10.0f);
			joint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, true);
			
			outCollection.add(*millBody);
			outCollection.add(*fan);
			outCollection.add(*joint);
		}

	}while( mills < retryNum);
}

void SampleLargeWorld::addTrees(const PxTriangleMeshGeometry& meshGeom, const PxBounds3& bound, PxCollection& outCollection)
{
	const char* filename = getSampleMediaFilename("tree.obj"); 
	const char* filenameCooked = getSampleOutputFilePath("tree.obj", ""); 
	
	PxReal maxDist = 2*(bound.maximum.y - bound.minimum.y);			
	for(PxU32 i = (PxU32)bound.minimum.z; i < (PxU32)bound.maximum.z; i += 80)
	{
		PxVec3 rayStart(bound.minimum.x, bound.maximum.y, (PxReal)i);
		
		PxRaycastBuffer hit;
		PxQueryFilterData filter(PxQueryFlag::eSTATIC);		
		PxScene& scene = getActiveScene();		
		bool ret = false;	
		{
			PxSceneReadLock scopedLock(scene);
			ret = scene.raycast(rayStart, PxVec3(0.0f,-1.0f,0.0f), maxDist, hit, PxHitFlag::ePOSITION, filter);
		}
		if( ret )
		{
			MeshBuilder::addObjMeshToPxCollection(
									getPhysics(), getCooking(), getDefaultMaterial(),
									filename, filenameCooked,
									PxTransform(hit.block.position, PxQuat(-PxHalfPi, PxVec3(1,0,0))), PxVec3(200, 200, 200), 
									outCollection, gTreeName);

		}
	}

}

static void flattenChunk(SampleArray<PxVec3>& terrainVertices, const PxBounds3& bound, PxReal dataY)
{
	PxBounds3 bound1 = bound;
	PxReal deltaX = bound1.maximum.x - bound1.minimum.x;
	PxReal deltaZ = bound1.maximum.z- bound1.minimum.z;
	PxReal delta = deltaX < deltaZ ? deltaX : deltaZ;
	bound1.fattenFast(delta/32);

	const PxU32 size = terrainVertices.size();
	
	for(PxU32 i = 0; i < size; ++i)
	{
		if((terrainVertices[i].x <= bound1.maximum.x && terrainVertices[i].x >= bound1.minimum.x)
			&& (terrainVertices[i].z <= bound1.maximum.z && terrainVertices[i].z >= bound1.minimum.z))
		{
			terrainVertices[i].y = dataY;
		}
	}
}

void SampleLargeWorld::addCity(SampleArray<PxVec3>& terrainVertices, const PxBounds3& bound, PxCollection& outCollection)
{	
	const char* filename = getSampleMediaFilename("city.obj"); 
	const char* filenameCooked = getSampleOutputFilePath("city.obj", ""); 

	MeshBuilder::addObjMeshToPxCollection(
									getPhysics(), getCooking(), getDefaultMaterial(),
									filename, filenameCooked,
									PxTransform(gCityPos, PxQuat(-PxHalfPi, PxVec3(1,0,0))), PxVec3(100, 100, 120), 
									outCollection, gCityName);
}

void SampleLargeWorld::addFarm(SampleArray<PxVec3>& terrainVertices, const PxBounds3& bound, PxCollection& outCollection)
{
	const char* filename = getSampleMediaFilename("barn.obj"); 
	const char* filenameCooked = getSampleOutputFilePath("barn.obj", ""); 

	MeshBuilder::addObjMeshToPxCollection(
										getPhysics(), getCooking(), getDefaultMaterial(),
										filename, filenameCooked,
										PxTransform(gFarmPos, PxQuat(PxPi, PxVec3(1,0,0))), PxVec3(100, 100, 100), 
										outCollection, gFarmName);

}


void SampleLargeWorld::addFortress(SampleArray<PxVec3>& terrainVertices, const PxBounds3& bound, PxCollection& outCollection)
{
	const char* pathname = getSampleMediaFilename("WallTower.repx"); 

	MeshBuilder* mb = SAMPLE_NEW(MeshBuilder)(*this, pathname );
	const PxQuat q = PxShortestRotation(PxVec3(0.0f, 0.0f, 1.0f), PxVec3(0.0f, 1.0f, 0.0f));
	
	mb->addRepXToPxCollection( PxTransform(gFortressPos, q), PxVec3(0.05f, 0.05f, 0.05f), outCollection);

	DELETESINGLE( mb );
}

PxTriangleMesh* SampleLargeWorld::generateTriMesh(const SampleArray<PxVec3>* vertices, const SampleArray<PxU32>* indices)
{
	//Cooking mesh
	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count		= vertices->size();
	meshDesc.triangles.count	= indices->size()/3;
	meshDesc.points.stride		= sizeof(PxVec3);
	meshDesc.triangles.stride	= sizeof(PxU32)*3;
	meshDesc.points.data		= &(*vertices)[0];
	meshDesc.triangles.data		= &(*indices)[0];
	meshDesc.flags = PxMeshFlags(0);

	PxDefaultMemoryOutputStream stream;
	bool ok = getCooking().cookTriangleMesh(meshDesc, stream);
	if( !ok )
		return NULL;

	PxDefaultMemoryInputData rb(stream.getData(), stream.getSize());
	
	return getPhysics().createTriangleMesh(rb);
}

void SampleLargeWorld::createChunk(CoordType coordX, CoordType coordY, PxCollection& outCollection)
{
	CoordType dim = binData.mDim;

	SampleArray<SampleArray<PxVec3> >& terrainVertices = binData.mTerrainVertices;
	
	SampleArray<SampleArray<PxU32> >& terrainIndices = binData.mTerrainIndices;

	SampleArray<PxVec3> tmpV;
	SampleArray<PxU32> tmpI;

	SampleArray<PxVec3>* vertices;
	SampleArray<PxU32>* indices;
	PxVec3 xAxis( (coordX / dim ) * CHUNK_WIDTH * dim - 0.5f * CHUNK_WIDTH, 0.0f, 0.0f );
	PxVec3 zAxis( 0.0f, 0.0f, ( coordY / dim ) * CHUNK_WIDTH * dim - 0.5f * CHUNK_WIDTH );
	CoordType x = coordX %( dim << 1 ), z = coordY % ( dim <<1 );
	PxVec3 xRevert(1.0f), zRevert(1.0f);

	bool needRevertMesh = false;
	bool useRawDirectly = true;

	//The terrain template is 16*16, if chunk is out of the range, use mirror to create new one
	if(coordX >= dim || coordY >= dim)
		useRawDirectly = false;

	if( useRawDirectly )
	{
		vertices = &terrainVertices[coordY * dim + coordX];
		indices = &terrainIndices[coordY * dim + coordX];
	}
	else	//Just use mirror to create new terrain, so terrain can be expand to infinite size
	{
		if( x >= dim )
		{
			x = x % dim;
			xRevert.x = -1.0f;
			needRevertMesh = !needRevertMesh;
			x = dim - x - 1;
		}

		if( z >= dim )
		{
			z = z % dim;
			zRevert.z = -1.0f;
			needRevertMesh = !needRevertMesh;
			z = dim - z - 1;
		}

		SampleArray<PxVec3>& templateV = terrainVertices[z*dim + x];			
		
		for( PxU32 v = 0; v < templateV.size(); v++)
		{
			PxVec3 vert = templateV[v];
			if(xRevert.x < 0.0f)
			{
				vert.x =  CHUNK_WIDTH * dim - 0.5f * CHUNK_WIDTH - templateV[v].x + xAxis.x; 
			}
			else
			{
				vert.x = templateV[v].x - -0.5f * CHUNK_WIDTH + xAxis.x;
			}

			if(zRevert.z < 0.0f)
			{
				vert.z =  CHUNK_WIDTH * dim - 0.5f * CHUNK_WIDTH - templateV[v].z + zAxis.z; 
			}
			else
			{
				vert.z = templateV[v].z - -0.5f * CHUNK_WIDTH + zAxis.z;
			}		

			tmpV.pushBack(vert);
		}

		vertices = &tmpV;

		if(needRevertMesh)
		{				
			SampleArray<PxU32>& templateI = terrainIndices[z*dim + x];

			for(PxU32 d = 0; d < templateI.size(); d += 3)
			{
				tmpI.pushBack(templateI[d]);
				tmpI.pushBack(templateI[d+2]);
				tmpI.pushBack(templateI[d+1]);

			}

			indices = &tmpI;
		}
		else
			indices = &terrainIndices[z*dim + x];

	}

	//Both pose and scale are identity, so local bounds == global bounds
	PxBounds3 bound = PxBounds3::empty();

	for( SampleArray<PxVec3>::Iterator iter = vertices->begin();
		iter != vertices->end(); iter++ )
	{
		bound.include( *iter );
	}

	ChunkID id; 
	id.coord.x = coordX;
	id.coord.y = coordY;
	BasicRandom random(id.id);

	PxBounds3	objBounds[3];
	PxF32		flatHeight[3];
	const float flattenScale = 1.1f;
	
	objBounds[0] = gCityBounds;
	objBounds[1] = gFarmBounds;
	objBounds[1].fattenFast(1.5f);

	PxBounds3 fortressScaledBound( gFortressBounds.minimum * gFortrestScale, gFortressBounds.maximum * gFortrestScale );
	objBounds[2] = PxBounds3::centerExtents( gFortressPos + fortressScaledBound.getCenter(), fortressScaledBound.getExtents() * flattenScale);

	flatHeight[0] = gCityPos.y;
	flatHeight[1] = gFarmPos.y;
	flatHeight[2] = gFortressPos.y;

	for(PxU32 i = 0; i < 3; i++)
	{
		if(bound.intersects(objBounds[i]))
		{
			flattenChunk(*vertices, objBounds[i], flatHeight[i]);		
		}
	}
	
	bool isEmpty = false;	
	if(id.id == FARM_CHUNKID.id)
	{
		addFarm(*vertices, bound, outCollection);
	}
	else if(id.id == CITY_CHUNKID.id)
	{
		addCity(*vertices, bound, outCollection);
	}
	else if(id.id == FORTRESS_CHUNKID.id)
	{
		addFortress(*vertices, bound, outCollection);
	}	
	else
	{
		isEmpty = true;
	}

	PxTriangleMesh* triangleMesh = generateTriMesh(vertices, indices);
	if( !triangleMesh )
		return;

	PxTriangleMeshGeometry meshGeom(triangleMesh);
	PxMaterial& material = getDefaultMaterial();
	//Create terrain actor
	PxRigidStatic* actor = PxCreateStatic(getPhysics(), PxTransform(PxIdentity), meshGeom, material);

	//Enable ccd for terrain
	PxShape* meshShape = getShape(*actor);
	SampleLargeWorld::setCCDActive(*meshShape);
	meshShape->setFlag(PxShapeFlag::eVISUALIZATION, false);

	if(id.id == FOREST_CHUNKID.id)
	{		
		addTrees(meshGeom, bound, outCollection);
		isEmpty = false;
	}

	if(isEmpty)
	{
#if ADD_WIND_MILL
		addWindMills(meshGeom, random, bound, outCollection);		
#endif
	}

	outCollection.add(*actor);
	return;
}

BackgroundLoader::BackgroundLoader(SampleLargeWorld& sample,  CoordType halfRange, CoordType terrainRange, PxF32 chunkWidth)
	: mSampleLargeWorld(&sample)
	, mPhysics(sample.getPhysics())
	, mScene(sample.getActiveScene())
	, mMaterial(sample.getDefaultMaterial())
	, mHalfRange(halfRange)
	, mTerrainRange(terrainRange)
	, mChunkWidth(chunkWidth)
{
	mCurChunkId.id = 0;
	mDiskIOTime = 0;
	mPhyXStreamTime = 0;
	mGraphicStreamTime = 0;

	mLoaderThread = PX_NEW(Thread)(loaderThread, this, "LoaderThread");
	mLoaderThread->start(0);
	mSr = PxSerialization::createSerializationRegistry(mPhysics);
}

BackgroundLoader::~BackgroundLoader()
{
	mLoaderThread->signalQuit();
	mRequestReady.set();
	mLoaderThread->waitForQuit();
	delete mLoaderThread;
	
	deleteLoadedRenderQueue();

	deleteCollections();
	mSr->release();
}

void BackgroundLoader::onTick()
{
	//Use local queue here to minimize the lock time, prevent blocking loader thread.	
	std::vector<ChunkCommand> theCmdQueue;
	std::vector<DeferredLoadedRenderData> theLoadedRenderDataQueue;

	mQueueLock.lock();
	theCmdQueue = mChunkCmds;
	theLoadedRenderDataQueue = mLoadedRenderDataQueue;
	mLoadedRenderDataQueue.clear();
	mChunkCmds.clear();
	mQueueLock.unlock();
	
	size_t cmdQueueSize = theCmdQueue.size();
	if(cmdQueueSize == 0)
		return;
	
	//filter the same chunk with different commands
	for(PxU32 i = 0; i < cmdQueueSize; ++i)
	{
		ChunkCommand cmdQueue = theCmdQueue[i];
		
		bool isOutdatedCommand = false;
		for(PxU32 j = i + 1; j < cmdQueueSize && !isOutdatedCommand; ++j)
		{
			isOutdatedCommand = (cmdQueue.id == theCmdQueue[j].id);
		}
		
		if(isOutdatedCommand)
			continue;

		ChunkCommandType::Enum	type = cmdQueue.type;
		if(type == ChunkCommandType::eAdd)
		{
			addReadyChunkToScene(cmdQueue.id);	
		}
		else if(type == ChunkCommandType::eRemove)
		{
			destroyChunk(cmdQueue.id);
		}
	}

	Ps::Time localTimer;
	for(PxU32 i = 0; i < theLoadedRenderDataQueue.size(); i++)
	{
		DeferredLoadedRenderData& cq = theLoadedRenderDataQueue[i];
		RenderBaseActor* renderMesh = mSampleLargeWorld->createRenderMeshFromRawMesh( cq);

		//Link physic actor to render actor
		cq.shape->userData = renderMesh;
		
		{
			PxSceneReadLock scopedLock(mSampleLargeWorld->getActiveScene());
			renderMesh->setPhysicsShape(cq.shape, cq.shape->getActor());
			renderMesh->setEnableCameraCull(true);
			
			PxTriangleMeshGeometry geometry;
			cq.shape->getTriangleMeshGeometry(geometry);
			renderMesh->setMeshScale(geometry.scale);
		}
		PxVec3 *verts = const_cast<PxVec3*>(cq.mVerts);
		SAMPLE_FREE(verts);

		PxU32 *indices = const_cast<PxU32*>(cq.mIndices);
		SAMPLE_FREE(indices);

		PxReal *normals = (PxReal*)const_cast<PxVec3*>(cq.mVertexNormals);
		SAMPLE_FREE(normals);
		
		PxReal* uvs = const_cast<PxReal*>(cq.mUVs);
		SAMPLE_FREE(uvs);
	}

	Ps::Time::Second timeCost = localTimer.getElapsedSeconds();

	mLoaderStatusLock.lockWriter();
	mGraphicStreamTime += timeCost;
	mLoaderStatusLock.unlockWriter();

	{
		PxSceneWriteLock scopedLock(mSampleLargeWorld->getActiveScene());
		for(PxU32 i = 0; i < mRemovingActors.size(); i++)
		{
			mRemovingActors[i]->release();
		}
	}

	mRemovingActors.clear();
}

void BackgroundLoader::updateChunk(const PxVec3& cameraPos)
{
	ChunkID id = getChunkIdFromPosition(cameraPos, mTerrainRange, mChunkWidth);
	if( id.id == mCurChunkId.id)
		return;

	mLocalBounds = PxBounds3::centerExtents( PxVec3(id.coord.x*mChunkWidth, 0.0f, id.coord.y*mChunkWidth), PxVec3(mChunkWidth));
	updateDynamicChunkId();

	mCurChunkId = id;
	CoordType x = PxClamp(id.coord.x, (CoordType)mHalfRange, (CoordType)(mTerrainRange - mHalfRange));
	CoordType y = PxClamp(id.coord.y, (CoordType)mHalfRange, (CoordType)(mTerrainRange - mHalfRange));
	
	std::vector<ChunkID> requestQueue;
	for( CoordType i = PxMax( (CoordType)0, (CoordType)(x - mHalfRange) ); i <= PxMin( (CoordType)N2, (CoordType)(x + mHalfRange)); i++ )
	{
		for( CoordType j = PxMax( (CoordType)0, (CoordType)(y - mHalfRange)); j <= PxMin( (CoordType)N2, (CoordType)(y + mHalfRange)); j++ )
		{
			ChunkID id;
			id.coord.x = i;
			id.coord.y = j;
			requestQueue.push_back(id);
		}	
	}
	
	mQueueLock.lock();
	mRequestQueue = requestQueue;
	mQueueLock.unlock();
	mRequestReady.set();
}

static const char* getPlatformName()
{
#if PX_X86
	return "PC32";
#elif PX_X64
	return "PC64";
#elif PX_ARM_FAMILY
	return "ARM";
#else
	return "";
#endif
}

const char* BackgroundLoader::getPathname(ChunkID id)
{
	char tmpFilename[256];
	sprintf(tmpFilename, "chunk_%d_%d_%s.bin", id.coord.x, id.coord.y, getPlatformName());

	return mSampleLargeWorld->getSampleOutputFilePath(tmpFilename,"");
}

void BackgroundLoader::serialize(PxCollection* collection, ChunkID id)
{	
	const char* theBinPath = getPathname( id );	
	SampleFramework::File* fp = NULL;
	PxToolkit::fopen_s(&fp, theBinPath, "rb");
	if( fp )
	{
		fclose(fp);
		return;
	}

	PxCollection* theExtRef = PxCreateCollection();
	theExtRef->add(mMaterial, MATERIAL_ID);
	PxDefaultMemoryOutputStream memBuf;
	
	PxSerialization::complete(*collection, *mSr, theExtRef);			
	bool bRet = PxSerialization::serializeCollectionToBinary(memBuf, *collection, *mSr, theExtRef, true );
	PX_ASSERT(bRet);
	PX_UNUSED(bRet);
	
	PxDefaultFileOutputStream s( getPathname( id ) );
	
	Ps::Time localTimer;
	localTimer.getElapsedSeconds();
	s.write( memBuf.getData(), memBuf.getSize());
	mDiskIOTimeCounter += localTimer.getElapsedSeconds();

	theExtRef->release();
}

//PhysX support buffer inserting actor, so no care about whether physic is simulating here
void BackgroundLoader::addReadyChunkToScene(ChunkID id)
{
	PxCollection *theCollection = NULL;
	{
		shdfnd::Mutex::ScopedLock al( mCollectionLock );
		CollectionIdMap::iterator it = mCollectionIdMap.find(id);
		if(it == mCollectionIdMap.end() ||  it->second.addToScene )
		{
			return;
		}

		theCollection = it->second.collection;
		if( !theCollection )
			return;		

		it->second.addToScene = true;
	}

	PxU32 count = theCollection->getNbObjects();
	for(PxU32 i = 0; i < count; i++)
	{
		PxBase *object = &theCollection->getObject(i);
		PX_ASSERT(object);
		PxType theType = object->getConcreteType();
		if( theType == PxConcreteType::eRIGID_DYNAMIC ) 
		{
			PxRigidActor *actor = static_cast<PxRigidActor*>(object);
			mSampleLargeWorld->createRenderObjectsFromActor(actor);
		}
		else if(theType == PxConcreteType::eRIGID_STATIC)
		{			
			PxRigidStatic* actor = static_cast<PxRigidStatic*>(object);
			PxShape* shape = getShape( *actor );
			PxTriangleMeshGeometry geometry;
			shape->getTriangleMeshGeometry(geometry);
			//We create static render mesh in another loop
			if(shape->getGeometryType() != PxGeometryType::eTRIANGLEMESH)
			{
				mSampleLargeWorld->createRenderObjectsFromActor(actor, NULL);
			}
		}
	}
	
	{
		PxSceneWriteLock scopedLock(mSampleLargeWorld->getActiveScene());
		mScene.addCollection(*theCollection);
	}

	//Melt the freezon important actors on this chunk
	for(unsigned i = 0; i < mDyncActors.size(); ++i)
	{
		DynamicObjects* obj = mDyncActors[i];
		if( obj->id.id == id.id &&  obj->isImportant )
		{
			PxSceneWriteLock scopedLock(mSampleLargeWorld->getActiveScene());
			obj->actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
		}	
	}
}

//just removed the actors from scene, do not release them
void BackgroundLoader::destroyChunk(ChunkID id)
{
	PxCollection *theCollection = NULL;
	bool inScene = false;
	void* memory = NULL;
	{
		shdfnd::Mutex::ScopedLock al(mCollectionLock);	
	
		CollectionIdMap::iterator it = mCollectionIdMap.find(id);
		if( it == mCollectionIdMap.end() )
			return;
		
		inScene = it->second.addToScene;
		theCollection = it->second.collection;
		memory = it->second.memory;
		mCollectionIdMap.erase(it);
	}
	

	if(inScene)
	{
		//remove objects from scene first
		PxU32 count = theCollection->getNbObjects();
		{
			PxSceneWriteLock scopedLock(mSampleLargeWorld->getActiveScene());
			for(PxU32 i = 0; i < count; i++)
			{
				PxBase *object = &theCollection->getObject(i);
				PX_ASSERT(object);
				PxType type = object->getConcreteType();
				if( type == PxConcreteType::eRIGID_STATIC || type == PxConcreteType::eRIGID_DYNAMIC )
				{
					mScene.removeActor( *static_cast<PxActor*>(object) );
				}
			}
		}
	}
	
	//serialize the objects when they are not used
	serialize(theCollection, id);
	PxCollectionExt::releaseObjects(*theCollection);
	theCollection->release();
	
	if(memory)
	{
		free(memory);
		memory = NULL;
	}
	
	//Remove the un-important actors on this chunk, freeze the important actors
	std::vector<DynamicObjects*>::iterator it = mDyncActors.begin();
	while(it != mDyncActors.end()) 
	{
		PxRigidDynamic* actor = (*it)->actor;
		PxVec3 actorPos;		
		{
			PxSceneReadLock scopedLock(mSampleLargeWorld->getActiveScene());
			actorPos = actor->getGlobalPose().p;
		}

		ChunkID theId = getChunkIdFromPosition(actorPos, mTerrainRange, mChunkWidth);
		if( theId.id == id.id  )
		{
			PxSceneWriteLock scopedLock(mSampleLargeWorld->getActiveScene());
			if( !(*it)->isImportant )
			{
				mSampleLargeWorld->removeActor(actor);
				mDyncActors.erase(it);
				it = mDyncActors.begin();
				continue;
			}			 
			else
			{
				actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
			}
		}
		++it;		
	}
}

static bool isActorNULL(DynamicObjects* o)
{
	return o->actor == NULL;
}

void BackgroundLoader::updateDynamicChunkId()
{
	for(unsigned i = 0; i < mDyncActors.size(); ++i)
	{
		DynamicObjects* obj = mDyncActors[i];
		PxRigidDynamic* actor = mDyncActors[i] ->actor;
		PxVec3 pos;
		{
			PxSceneReadLock scopedLock(mSampleLargeWorld->getActiveScene());
			pos = actor->getGlobalPose().p;
		}
		
		//Remove the un-important which go out of current grid.
		if(obj->isImportant == false && !mLocalBounds.contains(pos))
		{
			mRemovingActors.push_back(actor);
			obj->actor = NULL;
		}
		else
		{
			ChunkID theId = getChunkIdFromPosition(pos, mTerrainRange, mChunkWidth);
			obj->id.id = theId.id;
		}
	}

	std::vector<DynamicObjects*>::iterator new_end = std::remove_if( mDyncActors.begin(), mDyncActors.end(), isActorNULL);
	mDyncActors.erase( new_end, mDyncActors.end());
}
	
PxCollection* BackgroundLoader::loadChunk(ChunkID id)
{   
	{
		shdfnd::Mutex::ScopedLock al(mCollectionLock);
		if(  mCollectionIdMap.find(id) != mCollectionIdMap.end() )
			return NULL;
	}

	const char* theBinPath = getPathname( id );	

	PxCollection* theCollection = NULL;
	void *theMemory = NULL;

	Ps::Time localTimer;
	
	SampleFramework::File* fp = NULL;
	PxToolkit::fopen_s(&fp, theBinPath, "rb");
	//If cannot find/deserilize the chunk file, create one collection
	if( !fp )
	{
		theCollection = PxCreateCollection();
		mSampleLargeWorld->createChunk( id.coord.x, id.coord.y, *theCollection );
	}
	else
	{
		localTimer.getElapsedSeconds();
		PxU32 fileSize = getFileSize(theBinPath);
		theMemory = malloc(fileSize + PX_SERIAL_FILE_ALIGN);

		void* theMemoryA = (void*)((size_t(theMemory) + PX_SERIAL_FILE_ALIGN)&~(PX_SERIAL_FILE_ALIGN-1));
		size_t numRead = fread(theMemoryA, 1, fileSize, fp);
		fclose(fp);
		
		mDiskIOTimeCounter += localTimer.getElapsedSeconds();
		if(numRead != fileSize)
		{ 
			free(theMemory); 
			theMemory = NULL; 
			return NULL;
		}
		
		PxCollection* theExtRef = PxCreateCollection();
		theExtRef->add(mMaterial, MATERIAL_ID);

		theCollection = PxSerialization::createCollectionFromBinary(theMemoryA, *mSr, theExtRef);

		theExtRef->release();

		mPhyXStreamTimeCounter += localTimer.getElapsedSeconds();

	}

	CollectionMemory temp;
	temp.collection = theCollection;
	temp.memory = theMemory;		
	temp.addToScene = false;
	
	{
		shdfnd::Mutex::ScopedLock al(mCollectionLock);
		mCollectionIdMap[id] = temp;	
	}

	return theCollection;
}


void BackgroundLoader::deleteLoadedRenderQueue()
{
	std::vector<DeferredLoadedRenderData> theLoadedRenderDataQueue;
	
	mQueueLock.lock();
	theLoadedRenderDataQueue = mLoadedRenderDataQueue;
	mQueueLock.unlock();

	for(PxU32 i = 0; i < theLoadedRenderDataQueue.size(); i++)
	{
		DeferredLoadedRenderData& cq = theLoadedRenderDataQueue[i];
		
		PxVec3 *verts = const_cast<PxVec3*>(cq.mVerts);
		SAMPLE_FREE(verts);

		PxU32 *indices = const_cast<PxU32*>(cq.mIndices);
		SAMPLE_FREE(indices);

		PxReal *normals = (PxReal*)const_cast<PxVec3*>(cq.mVertexNormals);
		SAMPLE_FREE(normals);
		
		PxReal* uvs = const_cast<PxReal*>(cq.mUVs);
		SAMPLE_FREE(uvs);
	}
}

void BackgroundLoader::deleteCollections()
{
	CollectionIdMap::iterator it = mCollectionIdMap.begin(); 
	while(it!= mCollectionIdMap.end())
	{
		destroyChunk(it->first);
		it = mCollectionIdMap.begin(); 
	}
}

BackgroundLoader::DeferredLoadedRenderData* BackgroundLoader::createRawMeshFromObjMesh(const char* name, const PxTransform& pos, DeferredLoadedRenderData& rawMesh)
{
	PxU32 materialID = 0;
	const char* filename = NULL;
	if(Ps::stricmp(name, gCityName) == 0)
	{
		filename = "city.obj";
		materialID = MATERIAL_BUILDING;
	}
	else if(Ps::stricmp(name, gFarmName) == 0)
	{
		filename = "barn.obj";
		materialID = MATERIAL_FARM;
	}
	else if(Ps::stricmp(name, gTreeName) == 0)
	{
		filename = "tree.obj";
		materialID = MATERIAL_TREE;
	}

	if( mSampleLargeWorld->createRAWMeshFromObjMesh( filename, pos, materialID, rawMesh ) != NULL )
		return &rawMesh;
	else
		return NULL;
}

void* BackgroundLoader::loaderThread()
{
	std::vector<ChunkID> lastUpdateQueue;

#if ENABLE_PROGRESS_BAR		
	mLoaderStatusLock.lockWriter();
	mPhyXStreamTime = 0.0f;
	mGraphicStreamTime = 0.0f;
	mLoaderStatusLock.unlockWriter();
#endif
	while(mRequestReady.wait())
	{
		physx::shdfnd::Thread::yield();
		
		mRequestReady.reset();

		if (mLoaderThread->quitIsSignalled())
		{
			mLoaderThread->quit();
			break;
		}

#if ENABLE_PROGRESS_BAR		
		mLoaderStatusLock.lockWriter();
		mQueryLength = 0;
		mQueryProgress = 0.0f;
		mLoaderStatusLock.unlockWriter();
#endif
		
		bool bDirty = false;

		std::vector<ChunkID> requestedQueue;	
		std::vector<DeferredLoadedRenderData> loadedRenderDataQueue;	
		std::vector<ChunkCommand>		theChunkCmds;
		
		mQueueLock.lock();
		requestedQueue = mRequestQueue;
		mQueueLock.unlock();
		
		for(unsigned iLast = 0; iLast < lastUpdateQueue.size(); iLast++)
		{
			bool bFound = false;
			for(unsigned iRequest = 0; iRequest < requestedQueue.size(); iRequest++)
			{
				bFound = lastUpdateQueue[iLast].id == requestedQueue[iRequest].id;
				if(bFound)
					break;
			}
			if(!bFound)
			{
				bDirty = true;
				theChunkCmds.push_back(ChunkCommand(ChunkCommandType::eRemove,lastUpdateQueue[iLast]));
			}
		}		
		
		for(unsigned iRequest = 0; iRequest < requestedQueue.size(); iRequest++)
		{
			bool bFound = false;
			for(unsigned iLast = 0; iLast < lastUpdateQueue.size(); iLast++)
			{
				bFound = requestedQueue[iRequest].id == lastUpdateQueue[iLast].id;
				if(bFound)
					break;
			}
			if(!bFound)
			{
				bDirty = true;
				theChunkCmds.push_back(ChunkCommand(ChunkCommandType::eAdd,requestedQueue[iRequest]));
			}
		}	
		
		if(bDirty)
		{
			lastUpdateQueue = requestedQueue;
		}
		else
			continue;
		
		Ps::Time time;
		Ps::Time::Second normalTime = 0.0f;
		mDiskIOTimeCounter			= 0.0f;
		mPhyXStreamTimeCounter		= 0.0f;
		mGraphicStreamTimeCounter	= 0.0f;

		PxU32 loadedQueueSize = 0;
		PxU32 cmdQueueSize = static_cast<PxU32>(theChunkCmds.size());
		for(unsigned i = 0; i < cmdQueueSize; i++)
		{
			if(theChunkCmds[i].type == ChunkCommandType::eRemove )
			{
				continue;
			}

			++loadedQueueSize;
			
			PxCollection* theCollection = loadChunk(theChunkCmds[i].id);

			time.getElapsedSeconds();

			if(theCollection != NULL )
			{
				PxU32 nb = theCollection->getNbObjects();
				for(PxU32 o = 0; o < nb; o++)
				{
					PxBase* s = &theCollection->getObject( o );
					const PxType serialType = s->getConcreteType();

					if( serialType==PxConcreteType::eRIGID_STATIC )
					{
						PxRigidStatic* actor = static_cast<PxRigidStatic*>(s);
						const char* name = actor->getName();
						//If actor has name, it should have only one shape
						if(name)
						{
							PxShape* meshShape = NULL;
							PxTriangleMeshGeometry geometry;
							PxTransform actorPos;
							{
								meshShape = getShape(*actor);							
								meshShape->getTriangleMeshGeometry(geometry);
								actorPos = actor->getGlobalPose();
							}

							DeferredLoadedRenderData rawMesh;
							if( createRawMeshFromObjMesh(name, actorPos, rawMesh) != NULL )
							{
								rawMesh.shape = meshShape;
								loadedRenderDataQueue.push_back(rawMesh);
							}
						}
						else
						{
							const PxU32 numShapes = actor->getNbShapes();
							PxShape** shapes = (PxShape**)SAMPLE_ALLOC(sizeof(PxShape*)*numShapes);
							actor->getShapes(shapes, numShapes);
							for(PxU32 i = 0; i < numShapes; i++)
							{
								PxShape* shape = shapes[i];
								if(shape->getGeometryType() == PxGeometryType::eTRIANGLEMESH)
								{
									PxTriangleMeshGeometry mesh;
									shape->getTriangleMeshGeometry(mesh);
									DeferredLoadedRenderData rawMesh;
									if( mSampleLargeWorld->createRawMeshFromMeshGeom(mesh, rawMesh) != NULL )
									{
										rawMesh.mTransform = actor->getGlobalPose();
										rawMesh.shape = shape;
										loadedRenderDataQueue.push_back(rawMesh);
									}
								}			
							}
							SAMPLE_FREE(shapes);
						}
					}
				}
				physx::shdfnd::Thread::yield();
			}

			normalTime += time.getElapsedSeconds();

#if ENABLE_PROGRESS_BAR	
			mLoaderStatusLock.lockWriter();
			mQueryLength++;
			PX_ASSERT(cmdQueueSize != 0);
			mQueryProgress = (PxF32)i/(PxF32)cmdQueueSize;
			mLoaderStatusLock.unlockWriter();
#endif
			if (mLoaderThread->quitIsSignalled())
			{
				break;
			}

			physx::shdfnd::Thread::yield();
		}
		
		mGraphicStreamTimeCounter += normalTime;
		mQueueLock.lock();
		mChunkCmds = theChunkCmds;
		mLoadedRenderDataQueue = loadedRenderDataQueue;
		mQueueLock.unlock();
	
		if (mLoaderThread->quitIsSignalled())
		{
			mLoaderThread->quit();
			break;
		}
#if ENABLE_PROGRESS_BAR	
		mLoaderStatusLock.lockWriter();
		mQueryLength=0;
		mQueryProgress = 0.0f;
		mLoaderStatusLock.unlockWriter();
#endif

		mLoaderStatusLock.lockWriter();
		mDiskIOTime			= mDiskIOTimeCounter;;
		mPhyXStreamTime		= mPhyXStreamTimeCounter;
		mGraphicStreamTime	= mGraphicStreamTimeCounter;
		mLoaderStatusLock.unlockWriter();
	}

	return NULL;
}

void* BackgroundLoader::loaderThread(void* loader)
{
	return ((BackgroundLoader*) loader)->loaderThread();
}
