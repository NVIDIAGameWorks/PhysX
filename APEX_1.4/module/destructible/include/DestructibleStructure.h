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



#ifndef __DESTRUCTIBLESTRUCTURE_H__
#define __DESTRUCTIBLESTRUCTURE_H__

#include "Apex.h"
#include "DestructibleAssetProxy.h"

#include "ScopedPhysXLock.h"

#include "PsMutex.h"
#include "PxShape.h"
#include "PxRigidActor.h"
#include <PxRigidDynamic.h>
#include <PxRigidBodyExt.h>
#include <PxShapeExt.h>

#ifndef USE_CHUNK_RWLOCK
#define USE_CHUNK_RWLOCK 0
#endif

namespace nvidia
{
namespace destructible
{

class ModuleDestructibleImpl;
class DestructibleScene;
typedef class DestructibleStructureStressSolver StressSolver;

#define ENFORCE(condition) extern char unusableName[(condition)?1:-1]
#define GET_OFFSET(Class, Member) uint64_t(&(static_cast<Class*>(0)->Member))
struct CachedChunk : public ChunkTransformUnit
{
    CachedChunk(uint32_t chunkIndex_, PxMat44 chunkPose_)
    {
        chunkIndex = chunkIndex_;
        chunkPosition = chunkPose_.getPosition();
        chunkOrientation = PxQuat(PxMat33(chunkPose_.column0.getXYZ(), chunkPose_.column1.getXYZ(), chunkPose_.column2.getXYZ()));
#if defined WIN32
        ENFORCE(GET_OFFSET(CachedChunk, chunkIndex)			== GET_OFFSET(ChunkTransformUnit, chunkIndex));
        ENFORCE(GET_OFFSET(CachedChunk, chunkPosition)		== GET_OFFSET(ChunkTransformUnit, chunkPosition));
        ENFORCE(GET_OFFSET(CachedChunk, chunkOrientation)	== GET_OFFSET(ChunkTransformUnit, chunkOrientation));
        ENFORCE(static_cast<uint64_t>(sizeof(*this))	== static_cast<uint64_t>(sizeof(ChunkTransformUnit)));
#endif // WIN32
    }
    ~CachedChunk() {}
private:
    CachedChunk();
};
typedef CachedChunk ControlledChunk;
#undef GET_OFFSET
#undef ENFORCE

struct SyncDamageEventCoreDataParams : public DamageEventCoreData
{
	SyncDamageEventCoreDataParams()
		:
		destructibleID(0xFFFFFFFF)
	{
		DamageEventCoreData::chunkIndexInAsset = 0;
		DamageEventCoreData::damage = 0.0f;
		DamageEventCoreData::radius = 0.0f;
		DamageEventCoreData::position = PxVec3(0.0f);
	}

	uint32_t	destructibleID;			// The ID of the destructible actor that is being damaged.
};

struct FractureEvent
{
	FractureEvent() : chunkIndexInAsset(0xFFFFFFFF), destructibleID(0xFFFFFFFF), flags(0), impactDamageActor(NULL), appliedDamageUserData(NULL), deletionWeight(0.0f), damageFraction(1.0f) {}

	enum Flag
	{
		DamageFromImpact =	(1U << 0),
        CrumbleChunk =      (1U << 1),
		DeleteChunk =		(1U << 2),

		SyncDirect	= (1U << 24),		// fracture event is directly sync-ed
		SyncDerived = (1U << 25),		// fracture event is a derivative of a sync-ed damage event
		Manual		= (1U << 26),		// fracture event is manually invoked by the user
		Snap		= (1U << 27),		// fracture event is generated from the destructible stress solver
		Forced		= (1U << 28),
		Silent		= (1U << 29),
		Virtual		= (1U << 30),

		Invalid =	(1U << 31)
	};

	PxVec3			position;					// The position of a single fracture event.
	uint32_t			chunkIndexInAsset;			// The chunk index which is being fractured.
	PxVec3			impulse;					// The impulse vector to apply for this fracture event.
	uint32_t			destructibleID;				// The ID of the destructible actor that is being damaged.
	uint32_t			flags;						// Bit flags describing behavior of this fracture event.
	PxVec3			hitDirection;				// The direction vector this damage being applied to this fracture event.
	physx::PxActor const*	impactDamageActor;			// Other PhysX actor that caused damage to ApexDamageEventReportData.

	void*					appliedDamageUserData;		// User data from applyDamage or applyRadiusDamage.
	float			deletionWeight;				// A weighting factor for probabilistic deletion
	float			damageFraction;				// Calculated from damage spread functions, it's good to store this for later use (e.g. impulse scaling)
};

enum ChunkState
{
	ChunkVisible =			    0x01,
	ChunkDynamic =			    0x02,
    //ChunkControlled =         0x04,   // chunk behavior is not locally-determined //unused

	ChunkTemp0  =			    0x10,   // chunk state has been cached
	ChunkTemp1  =			    0x20,   // chunk exists
	ChunkTemp2  =			    0x40,   // chunk is visible
	ChunkTemp3  =			    0x80,   // chunk is dynamic
	ChunkTempMask =			    0xF0,
};

enum ChunkFlag
{
	ChunkCrumbled =				0x01,
	ChunkBelowSupportDepth =	0x02,
	ChunkExternallySupported =	0x04,
	ChunkWorldSupported =		0x08,
	ChunkMissingChild =			0x20,
	ChunkRuntime =				0x80,
    //ChunkGraphical =            0x80,   // chunk has no attached PxShape //unused
};

/* A struct for adding forces to actors after they are added to the scene */
struct ActorForceAtPosition
{
	ActorForceAtPosition() : force(0.0f), pos(0.0f), mode(physx::PxForceMode::eFORCE), wakeup(true), usePosition(true) {}

	ActorForceAtPosition(const PxVec3& _force, const PxVec3& _pos, physx::PxForceMode::Enum _mode, bool _wakeup, bool _usePosition)
		: force(_force)
		, pos(_pos)
		, mode(_mode)
		, wakeup(_wakeup) 
		, usePosition(_usePosition)
	{}

	PxVec3 force;
	PxVec3 pos;
	physx::PxForceMode::Enum mode;
	bool wakeup;
	bool usePosition;
};


class DestructibleStructure : public UserAllocated
{
public:

	enum
	{
		InvalidID =				0xFFFFFFFF,
		InvalidChunkIndex =		0xFFFFFFFF
	};

	struct Chunk
	{
		uint32_t			destructibleID;				// The GUID of the destructible actor this chunk is associated with.
		uint32_t			reportID;					// A GUID to report state about this chunk
		uint16_t			indexInAsset;				// The index into the master asset for this destructible
		uint8_t				state;						// bit flags controlling the current 'state' of this chunk.
		uint8_t				flags;						// Overall Chunk flags
		float				damage;						// How damaged this chunk is.
		PxVec3				localSphereCenter;			// A local bounding sphere for this chunk (center).
		float				localSphereRadius;			// A local bounding sphere for this chunk (radius).
		const ControlledChunk *	controlledChunk;		// Chunk data given by user
#if USE_CHUNK_RWLOCK
		shdfnd::ReadWriteLock*	lock;
#endif

		PxVec3			localOffset;				// If this chunk is instanced, this may be non-zero.  It needs to be stored somewhere in case we use 
															// the transform of a parent chunk which has a different offset.  Actually, this can all be looked up
															// through a chain of indirection, but I'm storing it here for efficiency.
		int32_t			visibleAncestorIndex;		// Index (in structure) of this chunks' visible ancestor, if any.  If none exists, it's InvalidChunkIndex.

		uint32_t			islandID;					// The GUID of the actor associated with the chunk. Used for island reconstruction.

	private:
		physx::Array<PxShape*> 	shapes;						// The rigid body shapes for this chunk.

	public:

		Chunk() {}
		Chunk(const Chunk& other)
		{
			*this = other;
		}

		Chunk&	operator = (const Chunk& other)
		{
			destructibleID			=	other.destructibleID;
			reportID				=	other.reportID;
			indexInAsset			=	other.indexInAsset;
			state					=	other.state;
			flags					=	other.flags;
			damage					=	other.damage;
			localSphereCenter		=	other.localSphereCenter;
			localSphereRadius		=	other.localSphereRadius;
			controlledChunk			=	other.controlledChunk;
#if USE_CHUNK_RWLOCK
#error USE_CONTROL_RWLOCK non-zero, but lock is not supported in assignment operator and copy constructor
#endif
			localOffset				=	other.localOffset;
			visibleAncestorIndex	=	other.visibleAncestorIndex;
			islandID				=	other.islandID;
			shapes					=	physx::Array<PxShape*>(other.shapes);
			return *this;
		}

		uint32_t	getShapeCount() const
		{
			return shapes.size();
		}

		const PxShape*	getShape(uint32_t shapeIndex) const
		{
			return shapeIndex < shapes.size() ? shapes[shapeIndex] : NULL;
		}

		PxShape*	getShape(uint32_t shapeIndex)
		{
			return shapeIndex < shapes.size() ? shapes[shapeIndex] : NULL;
		}

		bool	isFirstShape(const PxShape* shape) const
		{
			return shapes.size() ? shapes[0] == shape : false;
		}

		void	setShapes(PxShape* const* newShapes, uint32_t shapeCount)
		{
			shapes.resize(shapeCount);
			for (uint32_t i = 0; i < shapeCount; ++i)
			{
				shapes[i] = newShapes[i];
			}
			visibleAncestorIndex = InvalidChunkIndex;
		}

		void	clearShapes()
		{
			shapes.reset();
			visibleAncestorIndex = InvalidChunkIndex;
		}

		bool	isDestroyed() const
		{
			return shapes.empty() && visibleAncestorIndex == (int32_t)InvalidChunkIndex;
		}

		friend class DestructibleStructure;
	};

#if USE_CHUNK_RWLOCK
	class ChunkScopedReadLock : public physx::ScopedReadLock
	{
	public:
		ChunkScopedReadLock(Chunk& chunk) : physx::ScopedReadLock(*chunk.lock) {}
	};

	class ChunkScopedWriteLock : public physx::ScopedWriteLock
	{
	public:
		ChunkScopedWriteLock(Chunk& chunk) : physx::ScopedWriteLock(*chunk.lock) {}
	};
#endif

	DestructibleScene* 			dscene;						// The scene that this destructible structure belongs to
	Array<DestructibleActorImpl*>	destructibles;				// The array of destructible actors associated with this destructible structure
	Array<Chunk>				chunks;						// The array of chunks associated with this structure.
	Array<uint32_t>				supportDepthChunks;			//
	Array<uint32_t>				overlaps;
	Array<uint32_t>				firstOverlapIndices;		// Size = chunks.size()+1, firstOverlapsIndices[chunks.size()] = overlaps.size()
	uint32_t						ID;							// The unique GUID associated with this destructible structure
	uint32_t						supportDepthChunksNotExternallySupportedCount;
	bool						supportInvalid;
	PxRigidDynamic*					actorForStaticChunks;
	StressSolver *				stressSolver;
	
	typedef HashMap<PxRigidDynamic*, uint32_t> ActorToIslandMap;
	typedef HashMap<uint32_t, PxRigidDynamic*>	IslandToActorMap;
	// As internal, cache-type containers, these structures do not affect external state
	mutable ActorToIslandMap	actorToIsland;
	mutable IslandToActorMap	islandToActor;

	DestructibleStructure(DestructibleScene* inScene, uint32_t inID);
	~DestructibleStructure();

	bool			addActors(const physx::Array<class DestructibleActorImpl*>& destructiblesToAdd);
	bool			removeActor(DestructibleActorImpl* destructibleToRemove);
	void			setSupportInvalid(bool supportIsInvalid);

	void			updateIslands();
	void			tickStressSolver(float deltaTime);
	void			visualizeSupport(RenderDebugInterface* debugRender);

	uint32_t	damageChunk(Chunk& chunk, const PxVec3& position, const PxVec3& direction, bool fromImpact, float damage, float damageRadius,
								physx::Array<FractureEvent> outputs[], uint32_t& possibleDeleteChunks, float& totalDeleteChunkRelativeDamage,
								uint32_t& maxDepth, uint32_t depth, uint16_t stopDepth, float padding);
	void			fractureChunk(const FractureEvent& fractureEvent);
#if APEX_RUNTIME_FRACTURE
	void			runtimeFractureChunk(const FractureEvent& fractureEvent, Chunk& chunk);
#endif
	void			crumbleChunk(const FractureEvent& fractureEvent, Chunk& chunk, const PxVec3* impulse = NULL);	// Add an impulse - used when actor is static
	void			addDust(Chunk& chunk);
	void			removeChunk(Chunk& chunk);
	void			separateUnsupportedIslands();
	void			createDynamicIsland(const physx::Array<uint32_t>& indices);
	void			calculateExternalSupportChunks();
	void			buildSupportGraph();
	void			invalidateBounds(const PxBounds3* bounds, uint32_t boundsCount);
	void			postBuildSupportGraph();
	void			evaluateForHitChunkList(const physx::Array<uint32_t> & chunkIndices) const;

	PxMat44	getActorForStaticChunksPose()
	{
		if (NULL != actorForStaticChunks)
		{
			SCOPED_PHYSX_LOCK_READ(actorForStaticChunks->getScene());
			return PxMat44(actorForStaticChunks->getGlobalPose());
		}
		else
		{
			return PxMat44(PxIdentity);
		}
	}

	physx::Array<PxShape*>&	getChunkShapes(Chunk& chunk)
	{
		return (chunk.visibleAncestorIndex == (int32_t)InvalidChunkIndex) ? chunk.shapes : chunks[(uint32_t)chunk.visibleAncestorIndex].shapes;
	}

	const physx::Array<PxShape*>&	getChunkShapes(const Chunk& chunk) const
	{
		return (chunk.visibleAncestorIndex == (int32_t)InvalidChunkIndex) ? chunk.shapes : chunks[(uint32_t)chunk.visibleAncestorIndex].shapes;
	}

	PxRigidDynamic* getChunkActor(Chunk& chunk);

	const PxRigidDynamic* getChunkActor(const Chunk& chunk) const;

	bool chunkIsSolitary(Chunk& chunk)
	{
		PxRigidDynamic* actor = getChunkActor(chunk);
		SCOPED_PHYSX_LOCK_READ(actor->getScene());
		return (actor == NULL) ? false : (getChunkShapes(chunk).size() == actor->getNbShapes());
	}

	Chunk*	getRootChunk(Chunk& chunk)
	{
		if (chunk.isDestroyed())
		{
			return NULL;
		}
		return chunk.visibleAncestorIndex == (int32_t)InvalidChunkIndex ? &chunk : &chunks[(uint32_t)chunk.visibleAncestorIndex];
	}

	PxTransform getChunkLocalPose(const Chunk& chunk) const;

	void    setChunkGlobalPose(Chunk& chunk, PxTransform pose)
	{
		physx::Array<PxShape*>& shapes = getChunkShapes(chunk);
		PX_ASSERT(!shapes.empty());

		for (uint32_t i = 0; i < shapes.size(); ++i)
		{
			const PxTransform shapeLocalPose = shapes[i]->getLocalPose();
			const PxTransform inverseShapeLocalPose = shapeLocalPose.getInverse();
			PxTransform newGlobalPose = pose * inverseShapeLocalPose;

			shapes[i]->getActor()->setGlobalPose(newGlobalPose);
		}
	}

	PxTransform getChunkActorPose(const Chunk& chunk) const;

	PxTransform getChunkGlobalPose(const Chunk& chunk) const
	{
		return getChunkActorPose(chunk) * getChunkLocalPose(chunk);
	}

	PxTransform getChunkLocalTransform(const Chunk& chunk) const
	{
		const physx::Array<PxShape*>* shapes;
		PxVec3 offset;
		if (chunk.visibleAncestorIndex == (int32_t)InvalidChunkIndex)
		{
			shapes = &chunk.shapes;
			offset = PxVec3(0.0f);
		}
		else
		{
			shapes = &chunks[(uint32_t)chunk.visibleAncestorIndex].shapes;
			offset = chunk.localOffset - chunks[(uint32_t)chunk.visibleAncestorIndex].localOffset;
		}

		PX_ASSERT(!shapes->empty());
		PxTransform transform;
		if (!shapes->empty() && NULL != (*shapes)[0])
		{
			transform = (*shapes)[0]->getLocalPose();
		}
		else
		{
			transform = PxTransform(PxVec3(0, 0, 0), PxQuat(0, 0, 0, 1));
		}
		transform.p += offset;
		return transform;
	}

	PxTransform getChunkActorTransform(const Chunk& chunk) const
	{
		const physx::Array<PxShape*>& shapes = getChunkShapes(chunk);
		PX_ASSERT(!shapes.empty());
		if (!shapes.empty() && NULL != shapes[0])
		{
			// All shapes should have the same actor
			SCOPED_PHYSX_LOCK_READ(shapes[0]->getActor()->getScene());
			return shapes[0]->getActor()->getGlobalPose();
		}
		else
		{
			return PxTransform(PxVec3(0, 0, 0), PxQuat(0, 0, 0, 1));
		}
	}

	PxTransform getChunkGlobalTransform(const Chunk& chunk) const
	{
		return getChunkActorTransform(chunk).transform(getChunkLocalTransform(chunk));
	}

	void    addChunkImpluseForceAtPos(Chunk& chunk, const PxVec3& impulse, const PxVec3& position, bool wakeup = true);

	PxVec3	getChunkWorldCentroid(const Chunk& chunk) const
	{
		return getChunkGlobalPose(chunk).transform(chunk.localSphereCenter);
	}

	// This version saves a little time on consoles (saving a recalcualtion of the chunk global pose
	PX_INLINE PxVec3	getChunkWorldCentroid(const Chunk& chunk, const PxTransform& chuckGlobalPose)
	{
		return chuckGlobalPose.transform(chunk.localSphereCenter);
	}

	uint32_t	newPxActorIslandReference(Chunk& chunk, PxRigidDynamic& nxActor);
	void			removePxActorIslandReferences(PxRigidDynamic& nxActor) const;

	uint32_t	getSupportDepthChunkIndices(uint32_t* const OutChunkIndices, uint32_t  MaxOutIndices) const
	{
		PX_ASSERT( supportDepthChunksNotExternallySupportedCount <= supportDepthChunks.size() );

		uint32_t chunkNum = 0;
		for ( ; chunkNum < supportDepthChunksNotExternallySupportedCount && chunkNum < MaxOutIndices; ++chunkNum )
		{
			uint32_t chunkIndex = supportDepthChunks[chunkNum];
			Chunk const& chunk = chunks[chunkIndex];
			OutChunkIndices[chunkNum] = chunk.indexInAsset;
		}

		return chunkNum;
	}
};

}
} // end namespace nvidia

#endif // __DESTRUCTIBLESTRUCTURE_H__
