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


#ifndef CHUNKLOADER_H
#define CHUNKLOADER_H

#include "PsMutex.h"
#include "PsThread.h"
#include "PhysXSample.h"

#include "PsTime.h"

#define ENABLE_PROGRESS_BAR 1

typedef PxU16 CoordType;
typedef PxU32 IDType;

#define N1			7
//The terrain has 16*16 grids 
#define	N2			256

//The width of each terrain chunk, this is the hardcoded number accroding to Terrain.bin
#define CHUNK_WIDTH 256.0f

#define MATERIAL_ROAD_GRASS		1003
#define MATERIAL_BRICKS			1004
#define MATERIAL_BUILDING		1005
#define MATERIAL_FARM			1006
#define MATERIAL_TREE			1007

PX_FORCE_INLINE PxShape* getShape(const PxRigidActor& actor)
{
	PX_ASSERT(actor.getNbShapes() >= 1);
	PxShape* shape = NULL;
	actor.getShapes(&shape, 1);
	return shape;
}

typedef union 
{
	struct{ 
		CoordType x, y;
	} coord;
	IDType id;

}ChunkID;

static PX_INLINE bool operator < (const ChunkID& _Left, const ChunkID& _Right) 
{
	return _Left.id < _Right.id;
}
static PX_INLINE bool operator == (const ChunkID& _Left, PxU32 _Right) 
{
	return _Left.id == _Right;
}

static PX_INLINE bool operator == (const ChunkID& _Left, ChunkID _Right) 
{
	return _Left.id == _Right.id;
}

struct ChunkCommandType
{
	enum Enum
	{
		eAdd,
		eRemove
	};
};

struct ChunkCommand
{
	ChunkCommand(ChunkCommandType::Enum _type, const ChunkID& _id) 
	{ 
		type = _type;
		id = _id;
	} 

	ChunkCommandType::Enum	type;
	ChunkID id;
};

struct DynamicObjects
{
	DynamicObjects(PxRigidDynamic* _actor, bool _isImportant)
	{
		actor = _actor;
		isImportant = _isImportant;
		id.id = 0;
	}

	PxRigidDynamic*		actor;
	bool				isImportant;
	ChunkID				id;
};

class SampleLargeWorld;
class BackgroundLoader : public SampleAllocateable
{
public:
	struct DeferredLoadedRenderData : public RAWMesh
	{
		DeferredLoadedRenderData():shape(NULL){}		
		//the shape to link with
		PxShape* shape;
	};
public:

	BackgroundLoader(SampleLargeWorld& sample, CoordType halfRange, CoordType terrainRange, PxF32 chunkWidth);
	~BackgroundLoader();
	
	void addDynamicObject(PxRigidDynamic* inActor, bool inIsImportant) 
	{
		mDyncActors.push_back(new DynamicObjects(inActor, inIsImportant)); 
		updateDynamicChunkId();
	}

	void onTick(); 
	void updateChunk(const PxVec3& cameraPos);
	
	//The lock for accessing streaming progress and timing status
	Ps::ReadWriteLock				mLoaderStatusLock;


#ifdef ENABLE_PROGRESS_BAR
	volatile PxU32					mQueryLength;
	volatile PxF32					mQueryProgress;
#endif

	volatile Ps::Time::Second		mDiskIOTime;
	volatile Ps::Time::Second		mPhyXStreamTime;
	volatile Ps::Time::Second		mGraphicStreamTime;

private:
	const char* getPathname( ChunkID id );
	
	void	serialize(PxCollection* collection, ChunkID id);
	
	void	addReadyChunkToScene(ChunkID id);
	void	destroyChunk(ChunkID id);
	
	void	updateDynamicChunkId();

	//Return NULL means already loaded or cannot load
	PxCollection* loadChunk(ChunkID id);

	void	createRenderObjectsFromCollection(PxCollection* collection);
	void	deleteLoadedRenderQueue();
	void	deleteCollections();

	void*	loaderThread();	
	static void* loaderThread(void* loader);
	
	DeferredLoadedRenderData* createRawMeshFromObjMesh(const char* name, const PxTransform& pos, DeferredLoadedRenderData& rawMesh);

private:

	struct CollectionMemory
	{
		CollectionMemory()
			: collection(NULL)
			, memory(NULL)
			, addToScene(false)
		{
			
		}

		PxCollection*	collection;
		void*			memory;
		bool			addToScene;
	};

	typedef std::map<ChunkID, CollectionMemory>	CollectionIdMap;
	CollectionIdMap					mCollectionIdMap;
	 
	std::vector<ChunkID>			mRequestQueue;
	std::vector<DynamicObjects*>	mDyncActors;
	std::vector<PxActor*>			mRemovingActors;

	//We calculate terrain's normals and store here
	std::vector<DeferredLoadedRenderData>	mLoadedRenderDataQueue;

	physx::shdfnd::Thread*			mLoaderThread;

	//The lock to access ChunkQueue arrays
	physx::shdfnd::Mutex			mQueueLock;

	//The lock to access mCollectionIdMap
	physx::shdfnd::Mutex			mCollectionLock;
	
	//A signal to inform background thread new jobs
	shdfnd::Sync		mRequestReady;

	SampleLargeWorld*	mSampleLargeWorld;
	PxPhysics&			mPhysics;
	PxScene&			mScene;
	PxMaterial&			mMaterial;
	
	ChunkID				mCurChunkId;
	CoordType			mHalfRange;
	CoordType			mTerrainRange;
	PxF32				mChunkWidth;
	
	//Internal Timing varialbe
	Ps::Time::Second	mDiskIOTimeCounter;
	Ps::Time::Second	mPhyXStreamTimeCounter;
	Ps::Time::Second	mGraphicStreamTimeCounter;
	PxBounds3			mLocalBounds;

	PxSerializationRegistry*		mSr;
	std::vector<ChunkCommand>		mChunkCmds;
}; 

#endif

