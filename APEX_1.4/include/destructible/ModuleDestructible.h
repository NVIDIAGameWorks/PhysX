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



#ifndef MODULE_DESTRUCTIBLE_H
#define MODULE_DESTRUCTIBLE_H

#include "foundation/Px.h"
#include "foundation/PxBounds3.h"
#include "Module.h"

#ifndef APEX_RUNTIME_FRACTURE
#define APEX_RUNTIME_FRACTURE 1
#else
#undef APEX_RUNTIME_FRACTURE
#define APEX_RUNTIME_FRACTURE 0
#endif

#if PX_ANDROID
#undef APEX_RUNTIME_FRACTURE
#define APEX_RUNTIME_FRACTURE 0
#endif

namespace physx
{
class PxRigidActor;
class PxRigidDynamic;
class PxScene;
}

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

class DestructibleAsset;
class DestructibleAssetAuthoring;
class DestructibleActor;
class DestructibleActorJoint;
class DestructibleChunkDesc;
class DestructibleActorDesc;
class DestructibleActorJointDesc;

/**
	Flags returned by an UserChunkReport
*/
struct ApexChunkFlag
{
	/**
		Enum of apex chunk flag
	*/
	enum Enum
	{
		/** The chunk is dynamic */
		DYNAMIC							=	1 << 0,

		/** The chunk has environmental support, so will remain kinematic until fractured */
		EXTERNALLY_SUPPORTED			=	1 << 1,

		/** Only true if EXTERNALLY_SUPPORTED is true.  In addition, this means that it gained support via static geometry overlap. */
		WORLD_SUPPORTED					=	1 << 2,

		/** The chunk has been fractured */
		FRACTURED						=	1 << 3,

		/** The chunk has been destroyed because the PxActor FIFO was full */
		DESTROYED_FIFO_FULL				=	1 << 4,

		/** The chunk has been destroyed because it has exceeded the maximum debris lifetime */
		DESTROYED_TIMED_OUT				=	1 << 5,

		/** The chunk has been destroyed because it has exceeded the maximum debris distance */
		DESTROYED_EXCEEDED_MAX_DISTANCE	=	1 << 6,

		/** The destroyed chunk has generated crumble particles */
		DESTROYED_CRUMBLED				=	1 << 7,

		/** The destroyed chunk has moved beyond the destructible actor's valid bounds. */
		DESTROYED_LEFT_VALID_BOUNDS		=	1 << 8,

		/** The destroyed chunk has moved beyond the user-defined bounding box. */
		DESTROYED_LEFT_USER_BOUNDS		=	1 << 9,

		/** The destroyed chunk has moved into the user-defined bounding box. */
		DESTROYED_ENTERED_USER_BOUNDS	=	1 << 10
	};
};

/**
	Per-chunk data returned in DamageEventReportData
*/
struct ChunkData
{
	/** The index of the chunk within the destructible asset */
	uint32_t			index;

	/** The hierarchy depth of the chunk */
	uint32_t			depth;

	/**
		The chunk's axis-aligned bounding box, in world space.
	*/
	PxBounds3			worldBounds;

	/**
		How much damage the chunk has taken
	*/
	float				damage;

	/**
		Several flags holding chunk information 
		\see ApexChunkFlag
	*/
	uint32_t			flags;
};

/**
	Per-actor damage event data returned by an UserChunkReport
*/
struct DamageEventReportData
{
	/**
		The DestructibleActor instance that these chunks belong to
	*/
	DestructibleActor*	destructible;

	/**
		Damage event hitDirection in world space.
	*/
	PxVec3				hitDirection;

	/**
		The axis-aligned bounding box of all chunk fractures caused by this damage event,
		which have flags that overlap the module's chunkReportBitMask (see ModuleDestructible::setChunkReportBitMask).
	*/
	PxBounds3			worldBounds;

	/**
		Total number of fracture events caused by this damage event,
		which have flags that overlap the module's chunkReportBitMask (see ModuleDestructible::setChunkReportBitMask).
	*/
	uint32_t			totalNumberOfFractureEvents;

	/**
		Min depth of chunk fracture events caused by this damage event,
		which have flags that overlap the module's chunkReportBitMask (see ModuleDestructible::setChunkReportBitMask).
	*/
	uint16_t			minDepth;
	
	/**
		Max depth of chunk fracture events caused by this damage event,
		which have flags that overlap the module's chunkReportBitMask (see ModuleDestructible::setChunkReportBitMask).
	*/
	uint16_t			maxDepth;

	/**
		Array of chunk fracture event data for all chunks above a size threshold, which have flags that overlap the
		module's chunkReportBitMask (see ModuleDestructible::setChunkReportBitMask).  Currently
		the size cutoff is determined by the ModuleDestructible's chunkReportMaxFractureEventDepth (See
		ModuleDestructible::setChunkReportMaxFractureEventDepth).  All chunks up to that depth, but no deeper,
		are reported in this list.  The size of this array is given by fractureEventListSize.  fractureEventList may
		be NULL if fractureEventListSize = 0.
	*/
	const ChunkData* 	fractureEventList;

	/**
		Size of the fractureEventList array.  This may be less than totalNumberOfFractureEvents
		if some of the fracture events involve chunks which do not meet the size criterion
		described in the notes for the fractureEventList array.
	*/
	uint32_t			fractureEventListSize;

	/**
		Other PhysX actor that caused damage to DamageEventReportData.
	*/
	physx::PxActor const*	impactDamageActor;


	/**
		Impact damage position in world-space.
	*/
	PxVec3				hitPosition;

	/**
		User data from applyDamage or applyRadiusDamage.
	*/
	void*				appliedDamageUserData;
};


/**
	An event structure for an optional chunk event buffer.  Contains a chunk index and an event field.

	Note: currently the only chunk state event is for visibility changes, so the VisibilityChanged bit will always be set when this struct is used.
*/
struct DestructibleChunkEvent
{
	/**
		Enum of event mask
	*/
	enum EventMask
	{
		VisibilityChanged	=	(1 << 0),
		ChunkVisible	=		(1 << 1)
	};

	uint16_t	chunkIndex; ///< Chunk index
	uint16_t	event; ///< Event
};


/**
	Chunk state event data pushed to the user, if the user requests it via ModuleDestructible::scheduleChunkStateEventCallback.
*/
struct ChunkStateEventData
{
	/**
		The DestructibleActor instance that these chunks belong to
	*/
	DestructibleActor*			destructible;

	/**
		Array of chunk state event data collected since the last UserChunkReport::onStateChangeNotify call.
	*/
	const DestructibleChunkEvent* stateEventList;

	/**
		Size of the stateEventList array
	*/
	uint32_t					stateEventListSize;
};


/**
	UserChunkReport - API for a user-defined callback to get information about fractured or damaged chunks
*/
class UserChunkReport
{
public:
	/**
		User implementation of UserChunkReport must overload this function.
		This function will get called when a chunk is fractured or destroyed.
		See the definition of DamageEventReportData for the information provided
		to the function.
	*/
	virtual void	onDamageNotify(const DamageEventReportData& damageEvent) = 0;

	/**
		User implementation of UserChunkReport must overload this function.
		This function gets called when chunk visibility changes occur, if the user has selected
		this option via ModuleDestructible::scheduleChunkStateEventCallback.
		See the definition of ChunkStateEventData for the information provided
		to the function.

		*Please note* the user must also set the NxParameterized actor parameter 'createChunkEvents' to true,
		on individual destructible actors, to receive state change events from that actor.
	*/
	virtual void	onStateChangeNotify(const ChunkStateEventData& visibilityEvent) = 0;

	/**
		Called when an DestructibleActor contains no visible chunks.  If the user returns true,
		APEX will release the destructible actor.  If the user returns false, they should not
		release the destructible actor from within the callback, and instead must wait until
		the completion of Scene::fetchResults().

		Default implementation returns false, which is the legacy behavior.

		If this class (UserChunkReport) is not implemented, APEX will not destroy the DestructibleActor.
	*/
	virtual bool	releaseOnNoChunksVisible(const DestructibleActor* destructible) { PX_UNUSED(destructible);  return false; }

	/**
		List of destructible actors that have just become awake (any associated PhysX actor has become awake).
	**/
	virtual void	onDestructibleWake(DestructibleActor** destructibles, uint32_t count) = 0;

	/**
		List of destructible actors that have just gone to sleep (all associated PhysX actors have gone to sleep).
	**/
	virtual void	onDestructibleSleep(DestructibleActor** destructibles, uint32_t count) = 0;

protected:
	virtual			~UserChunkReport() {}
};


/**
	Particle buffer data returned with UserChunkParticleReport
*/
struct ChunkParticleReportData
{
	/** Position buffer.  The length of this buffer is given by positionCount. */
	const PxVec3* positions;

	/** The array length of the positions buffer. */
	uint32_t positionCount;

	/**
		Velocity buffer.  The length of this buffer is given by velocityCount.
		N.B.:  The velocity buffer might not have the same length as the positions buffer.
			It will be one of three lengths:
			velocityCount = 0: There is no velocity data with these particles.
			velocityCount = 1: All of the particle velocities are the same, given by *velocities.
			velocityCount = positionCount: Each particle velocity is given.
	*/
	const PxVec3* velocities;

	/** The array length of the velocities buffer.  (See the description above.)*/
	uint32_t velocityCount;
};

/**
	UserChunkParticleReport - API for a user-defined callback to get particle buffer data when a chunk crumbles or emits dust particles.
	Note - only crumble emitters are implemented in 1.2.0
*/
class UserChunkParticleReport
{
public:
	/**
		User implementation of UserChunkParticleReport must overload this function.
		This function will get called when an DestructibleActor generates crumble particle or
		dust particle effect data.  Crumble particles are generated whenever a chunk is destroyed completely and
		the DestructibleActorFlag CRUMBLE_SMALLEST_CHUNKS is set.  Dust line particles are
		generated along chunk boundaries whenever they are fractured free from other chunks.

		Note - only crumble emitters are implemented in 1.2.0
	*/
	virtual void	onParticleEmission(const ChunkParticleReportData& particleData) = 0;

protected:
	virtual			~UserChunkParticleReport() {}
};


/**
	UserDestructiblePhysXActorReport - API for user-defined callbacks giving notification of PhysX actor creation and release from the
	destruction module.
*/
class UserDestructiblePhysXActorReport
{
public:
	/** Called immediately after a PxActor is created in the Destruction module. */
	virtual void	onPhysXActorCreate(const physx::PxActor& actor) = 0;

	/** Called immediately before a PxActor is released in the Destruction module. */
	virtual void	onPhysXActorRelease(const physx::PxActor& actor) = 0;
protected:
	virtual		~UserDestructiblePhysXActorReport() {}
};


/**
	Destructible module constants
*/
struct ModuleDestructibleConst
{
	/**
		Enum for invalid chunk index (-1)
	*/
	enum Enum
	{
		/**
			When a function returns a chunk index, or takes a chunk index as a parameter, this
			value indicates "no chunk."
		*/
		INVALID_CHUNK_INDEX	= -1
	};
};

/**
	Render mesh distinction, skinned vs. static
*/
struct DestructibleActorMeshType
{
	/**
		Enum for destructible actor mesh type.
	*/
	enum Enum
	{
		Skinned,
		Static,
		Count
	};
};

/**
	The core data of Damage Event. It is used for sync damage coloring.
*/
struct DamageEventCoreData
{
	int32_t	chunkIndexInAsset;	///< Which chunk the damage is being applied to (non-range based damage)
	PxVec3	position;			///< The position, in world space, where the damage event is originating from.
	float	damage;				///< The amount of damage
	float	radius;				///< The radius of the damage, if this is a range based damage event.
};

/**
	Extended damage event data, used for impact damage reports
*/
struct ImpactDamageEventData : public DamageEventCoreData
{
	nvidia::DestructibleActor*	destructible;		///< The destructible hit by the impacting actor
	PxVec3						direction;			///< The position, in world space, from which the damage is applied
	physx::PxActor const*		impactDamageActor;	///< Other PhysX actor that caused damage
};

/**
	UserImpactDamageReport - API for a user-defined callback to get a buffer of impact damage events
*/
class UserImpactDamageReport
{
public:
	/**
		User implementation of UserImpactDamageReport must overload this function.
		If an instance of this object is passed to ModuleDestructible::setImpactDamageReportCallback,
		this function will get called once during ApexScene::fetchResults, passing back an array of
		ImpactDamageEventData reporting all impact damage events.
	*/
	virtual void	onImpactDamageNotify(const ImpactDamageEventData* buffer, uint32_t bufferSize) = 0;
};

/** 
	Sync-able Callback Class.
*/
template<typename DestructibleSyncHeader>
class UserDestructibleSyncHandler
{
public:
	/**
		Called when write begins.
	*/
	virtual void	onWriteBegin(DestructibleSyncHeader *& bufferStart, uint32_t bufferSize) = 0;
	
	/**
		Called when write done.
	*/
	virtual void	onWriteDone(uint32_t headerCount) = 0;
	
	/**
		Called when pre-process read begins.
	*/
	virtual void	onPreProcessReadBegin(DestructibleSyncHeader *& bufferStart, uint32_t & bufferSize, bool & continuePointerSwizzling) = 0;
	
	/**
		Called when pre-process read done.
	*/
	virtual void	onPreProcessReadDone(uint32_t headerCount) = 0;
	
	/**
		Called when read begins.
	*/
	virtual void	onReadBegin(const DestructibleSyncHeader *& bufferStart) = 0;
	
	/**
		Called when read done.
	*/
	virtual void	onReadDone(const char * debugMessage) = 0;
protected:
	virtual ~UserDestructibleSyncHandler() {}
};

/*** Sync-able Damage Data ***/

struct DamageEventUnit;
/**
	Damage event header.
*/
struct DamageEventHeader
{
	uint32_t					userActorID; 			///< User actor ID
	uint32_t					damageEventCount; 		///< Damage event count
	DamageEventUnit *			damageEventBufferStart; ///< damage event buffer size
	DamageEventHeader *			next; 					///< Pointer to next DamageEventHeader structure
};

/**
	Damage event unit.
*/
struct DamageEventUnit
{
	uint32_t	chunkIndex;			///< Chunk index
	uint32_t	damageEventFlags;	///< Damage event flags
	float		damage;				///< Damage value
	float		momentum;			///< Momentum
	float		radius;				///< Radius
	PxVec3		position;			///< Position
	PxVec3		direction;			///< Direction
};

/*** Sync-able Fracture Data ***/

struct FractureEventUnit;

/**
	Fracture event header.
*/
struct FractureEventHeader
{
    uint32_t					userActorID;				///< User actor ID
    uint32_t					fractureEventCount;			///< Fracture event count
    FractureEventUnit *			fractureEventBufferStart;	///< Fracture event buffer start
    FractureEventHeader *		next; 						///< Pointer to next FractureEventHeader structure
};

/**
	Fracture event unit.
*/
struct FractureEventUnit
{
    uint32_t	chunkIndex;			///< Chunk index
    uint32_t	fractureEventFlags;	///< Fracture event flags
	PxVec3		position;			///< Position
	PxVec3		direction;			///< Direction
	PxVec3		impulse;			///< Impulse
};

/*** Sync-able Transform Data ***/

struct ChunkTransformUnit;

/**
	Chunk transform header.
*/
struct ChunkTransformHeader
{
    uint32_t					userActorID;				///< User actor ID
    uint32_t					chunkTransformCount;		///< Chunk transform count
    ChunkTransformUnit *		chunkTransformBufferStart;	///< Chunk transform buffer start
    ChunkTransformHeader *		next; 						///< Pointer to next ChunkTransformHeader structure
};

/**
	Chunk transform unit.
*/
struct ChunkTransformUnit
{
	uint32_t	chunkIndex;			///< Chunk index
	PxVec3		chunkPosition;		///< Position
    PxQuat		chunkOrientation;	///< Chunk orientation
};


/** 
	Flags for DestructibleActor::raycast() 
*/
struct DestructibleActorRaycastFlags
{
	/**
		Enum of destructible actor raycast flags.
	*/
	enum Enum
	{
		NoChunks =		(0),
		StaticChunks =	(1 << 0),
		DynamicChunks =	(1 << 1),

		AllChunks =					StaticChunks | DynamicChunks,

		SegmentIntersect =	(1 << 2),	///< Do not consider intersect times > 1

		ForceAccurateRaycastsOn =	(1 << 3),
		ForceAccurateRaycastsOff =	(1 << 4),
	};
};


/** 
	Enum to control when callbacks are fired.
*/
struct DestructibleCallbackSchedule
{
	/**
		Enum of destructible callback schedule
	*/
	enum Enum
	{
		Disabled =		(0),
		BeforeTick,		///< Called by the simulation thread
		FetchResults,	///< Called by an application thread

		Count
	};
};


/**
	Descriptor used to create the Destructible APEX module.
*/
class ModuleDestructible : public Module
{
public:
	/** Object creation */

	/**
		Create an DestructibleActorJoint from the descriptor.  (See DestructibleActorJoint and
		DestructibleActorJointDesc.)
		This module will own the DestructibleActorJoint, so that any DestructibleAsset created by it will
		be released when this module is released.  You may also call the DestructibleActorJoint's release()
		method, as long as the DestructibleActorJoint is still valid in the scene. (See isDestructibleActorJointActive())
	*/
	virtual DestructibleActorJoint*			createDestructibleActorJoint(const DestructibleActorJointDesc& desc, Scene& scene) = 0;

	/**
	    Query the module on the validity of a created DestructibleActorJoint.
	    A DestructibleActorJoint will no longer be valid when it is destroyed in the scene, in which case the module calls the DestructibleActorJoint's release().
	    Therefore, this DestructibleActorJoint should no longer be referenced if it is not valid.
	*/
	virtual bool                            isDestructibleActorJointActive(const DestructibleActorJoint* candidateJoint, Scene& apexScene) const = 0;

	/** LOD */

	/**
		The maximum number of allowed PxActors which represent dynamic groups of chunks.  If a fracturing
		event would cause more PxActors to be created, then some PxActors are released and the chunks they
		represent destroyed.  The PxActors released to make room for others depends on the priority mode.
		If sorting by benefit (see setSortByBenefit), then chunks with the lowest LOD benefit are released
		first.  Otherwise, oldest chunks are released first.
	*/
	virtual void							setMaxDynamicChunkIslandCount(uint32_t maxCount) = 0;

	/**
		The maximum number of allowed visible chunks in the scene.  If a fracturing
		event would cause more chunks to be created, then PxActors are released to make room for the
		created chunks.  The priority for released PxActors is the same as described in
		setMaxDynamicChunkIslandCount.
	*/
	virtual void							setMaxChunkCount(uint32_t maxCount) = 0;

	/**
		Instead of keeping the youngest PxActors, keep the greatest benefit PxActors if sortByBenefit is true.
		By default, this module does not sort by benefit.  That is, oldest PxActors are released first.
	*/
	virtual void							setSortByBenefit(bool sortByBenefit) = 0;

	/**
		Deprecated
	*/
	virtual void                            setValidBoundsPadding(float pad) = 0;

	/**
		Effectively eliminates the deeper level (smaller) chunks from DestructibleAssets (see
		DestructibleAsset).  If maxChunkDepthOffset = 0, all chunks can be fractured.  If maxChunkDepthOffset = 1,
		the depest level (smallest) chunks are eliminated, etc.  This prevents too many chunks from being
		formed.  In other words, the higher maxChunkDepthOffset, the lower the "level of detail."
	*/
	virtual void							setMaxChunkDepthOffset(uint32_t maxChunkDepthOffset) = 0;

	/**
		Every destructible asset defines a min and max lifetime, and maximum separation distance for its chunks.
		Chunk islands are destroyed after this time or separation from their origins.  This parameter sets the
		lifetimes and max separations within their min-max ranges.  The valid range is [0,1].  Default is 0.5.
	*/
	virtual void							setMaxChunkSeparationLOD(float separationLOD) = 0;


	/** General */
	/**
		Sets the user chunk fracture/destroy callback. See UserChunkReport.
		Set to NULL (the default) to disable.  APEX does not take ownership of the report object.
	*/
	virtual void							setChunkReport(UserChunkReport* chunkReport) = 0;

	/**
		Sets the user impact damage report callback.  See UserImpactDamageReport.
		Set to NULL (the default) to disable.  APEX does not take ownership of the report object.
	*/
	virtual void							setImpactDamageReportCallback(UserImpactDamageReport* impactDamageReport) = 0;

	/**
		Set a bit mask of flags (see ApexChunkFlag) for the fracture/destroy callback (See setChunkReport.)
		Fracture events with flags that overlap this mask will contribute to the DamageEventReportData.
	*/
	virtual void							setChunkReportBitMask(uint32_t chunkReportBitMask) = 0;

	/**
		Sets the user callback to receive PhysX actor create/release notifications for destruction-associated PhysX actors.
		Set to NULL (the default) to disable.
	*/
	virtual void							setDestructiblePhysXActorReport(UserDestructiblePhysXActorReport* destructiblePhysXActorReport) = 0;

	/**
		Set the maximum depth at which individual chunk fracture events will be reported in an DamageEventReportData's
		fractureEventList through the UserChunkReport.  Fracture events deeper than this will still contribute to the
		DamageEventReportData's worldBounds and totalNumberOfFractureEvents.
	*/
	virtual void							setChunkReportMaxFractureEventDepth(uint32_t chunkReportMaxFractureEventDepth) = 0;

	/**
		Set whether or not the UserChunkReport::onStateChangeNotify interface will be used to deliver visibility
		change data to the user.

		Default = DestructibleCallbackSchedule::Disabled.
	*/
	virtual void							scheduleChunkStateEventCallback(DestructibleCallbackSchedule::Enum chunkStateEventCallbackSchedule) = 0;

	/**
		Sets the user chunk crumble particle buffer callback. See UserChunkParticleReport.
		Set to NULL (the default) to disable.  APEX does not take ownership of the report object.
	*/
	virtual void							setChunkCrumbleReport(UserChunkParticleReport* chunkCrumbleReport) = 0;

	/**
		Sets the user chunk dust particle buffer callback. See UserChunkParticleReport.
		Set to NULL (the default) to disable.  APEX does not take ownership of the report object.

		Note - this is a placeholder API.  The dust feature is disabled in 1.2.0.
	*/
	virtual void							setChunkDustReport(UserChunkParticleReport* chunkDustReport) = 0;

	/**
		PhysX SDK 3.X.
		Allows the user to specify an alternative PhysX scene to use for world support calculations.  If NULL (the default),
		the PhysX scene associated with the Scene will be used.
	*/
	virtual void							setWorldSupportPhysXScene(Scene& apexScene, PxScene* physxScene) = 0;

	/**
		PhysX SDK 3.X.
		Returns true iff the PxRigidActor was created by the Destructible module.  If true, the user must NOT destroy the actor.
	*/
	virtual bool							owns(const PxRigidActor* actor) const = 0;

#if APEX_RUNTIME_FRACTURE
	/**
		PhysX SDK 3.X.
		Returns true iff the PxShape was created by the runtime fracture system.
	*/
	virtual bool							isRuntimeFractureShape(const PxShape& shape) const = 0;
#endif

	/**
		PhysX SDK 3.X.
		Given an PxShape, returns the DestructibleActor and chunkIndex which belong to it,
		if the shape's PxActor is owned by the Destructible module (see the 'owns' function, above).  Otherwise returns NULL.
		Useful when a scene query such as a raycast returns a shape.
	*/
	virtual DestructibleActor*				getDestructibleAndChunk(const PxShape* shape, int32_t* chunkIndex = NULL) const = 0;

	/**
		Applies damage to all DestructibleActors within the given radius in the apex scene.
			damage = the amount of damage at the damage point
			momentum = the magnitude of the impulse to transfer to each chunk
			position = the damage location
			radius = distance from damage location at which chunks will be affected
			falloff = whether or not to decrease damage with distance from the damage location.  If true,
				damage will decrease linearly from the full damage (at zero distance) to zero damage (at radius).
				If false, full damage is applied to all chunks within the radius.
	*/
	virtual void							applyRadiusDamage(Scene& scene, float damage, float momentum,
	        const PxVec3& position, float radius, bool falloff) = 0;

	/**
		Lets the user throttle the number of SDK actor creates per frame (per scene) due to destruction, as this can be quite costly.
		The default is 0xffffffff (unlimited).
	*/
	virtual void							setMaxActorCreatesPerFrame(uint32_t maxActorsPerFrame) = 0;

	/**
		Lets the user throttle the number of fractures processed per frame (per scene) due to destruction, as this can be quite costly.
		The default is 0xffffffff (unlimited).
	*/
	virtual void							setMaxFracturesProcessedPerFrame(uint32_t maxFracturesProcessedPerFrame) = 0;

    /**
        Set the callback pointers from which APEX will use to return sync-able data.
    */
	virtual bool                            setSyncParams(UserDestructibleSyncHandler<DamageEventHeader> * userDamageEventHandler, UserDestructibleSyncHandler<FractureEventHeader> * userFractureEventHandler, UserDestructibleSyncHandler<ChunkTransformHeader> * userChunkMotionHandler) = 0;

	/** The following functions control the use of legacy behavior. */

	/** 
		By default, destruction damage application (either point or radius damage) now use the exact collision bounds
		of the chunk to determine damage application.  Before, bounding spheres were used on "leaf" chunks (deepest in the
		fracture hierarchy) and only the bounding sphere center (effectively radius 0) was used on all chunks higher in the
		hierarchy.  This led to inconsistent behavior when damaging destructibles at various levels of detail.

		If true is passed into this function, the legacy behavior is restored.
	*/
	virtual void							setUseLegacyChunkBoundsTesting(bool useLegacyChunkBoundsTesting) = 0;

	/** 
		By default, the BehaviorGroup fields damageSpread.minimumRadius, damageSpread.radiusMultiplier, and
		damageSpread.falloffExponent are used for point and radius damage.  Impact damage still uses damageToRadius,
		but this radius does not scale with the size of the destructible.
		
		The old, deprecated behavior was to base damage spread upon the damageToRadius field of the BehaviorGroup used
		by the chunk.  Specifically, damage would spread throughout a damage radius calculated as
		radius = damageToRadius * (damage / damageThreshold) * (size of destructible).

		If true is passed into this function, the legacy behavior is restored.
	*/
	virtual void							setUseLegacyDamageRadiusSpread(bool useLegacyDamageRadiusSpread) = 0;

	/**
		Sets mass scaling properties in a given apex scene.

		massScale and scaledMassExponent are used for scaling dynamic chunk masses.
		The 'real' mass m of a chunk is calculated by the destructible actor's density multiplied by
		the total volume of the chunk's (scaled) collision shapes.  The mass used in
		the simulation is massScale*pow(m/massScale,scaledMassExponent).  Values less than 1 have the
		effect of reducing the ratio of different masses.  The closer scaledMassExponent is to zero, the
		more the ratio will be 'flattened.'  This helps PhysX converge when there is a very large number
		of interacting rigid bodies (such as a pile of destructible chunks).

		massScale valid range:  (0,infinity).  Default = 1.0.
		scaledMassExponent valid range: [0,1].  Default = 0.5.

		Returns true iff the parameters are valid and the apexScene is valid.
	*/
	virtual bool							setMassScaling(float massScale, float scaledMassExponent, Scene& apexScene) = 0;

	/**
		If a static PhysX actor is about to be removed, or has just been added, to the scene, then world support may change.
		Use this function to notify the destruction module that it may need to reconsider world support in the given volumes.
		This may be called multiple times, and the bounds list will accumulate.  This list will be processed upon the next APEX
		simulate call, and cleared.
	*/
	virtual void							invalidateBounds(const PxBounds3* bounds, uint32_t boundsCount, Scene& apexScene) = 0;

	/**
		When applyDamage, or damage from impact, is processed, a rayCast call is used internally to find a more accurate damage position.
		This allows the user to specify the rayCast behavior.  If no flags are set, no raycasting is done.  Raycasting will be peformed against
		static or dynamic chunks, or both, depending on the value of flags.  The flags are defined by DestructibleActorRaycastFlags::Enum.

		The default flag used is DestructibleActorRaycastFlags::StaticChunks, since static-only raycasts are faster than ones that include
		dynamic chunks.

		Note: only the flags DestructibleActorRaycastFlags::NoChunks, StaticChunks, and DynamicChunks are considered.
	*/
	virtual void							setDamageApplicationRaycastFlags(nvidia::DestructibleActorRaycastFlags::Enum flags, Scene& apexScene) = 0;

	/**
		In PhysX 3.x, scaling of collision shapes can be done in the shape, not the cooked hull data.  As a consequence, each collision hull only needs to be cooked
		once.  By default, this hull is not scaled from the original data.  But if the user desires, the hull can be scaled before cooking.  That scale will be removed from
		the DestructibleActor's scale before being applied to the shape using the hull.  So ideally, this user-set cooking scale does nothing.  Numerically, however,
		it may have an effect, so we leave it as an option.

		The input scale must be positive in all components.  If not, the cooking scale will not be set, and the function returns false.  Otherwise, the scale is set and
		the function returns true.
	*/
	virtual bool							setChunkCollisionHullCookingScale(const PxVec3& scale) = 0;

	/**
		Retrieves the cooking scale used for PhysX3 collision hulls, which can be set by setChunkCollisionHullCookingScale.
	*/
	virtual PxVec3							getChunkCollisionHullCookingScale() const = 0;

	/**
		\brief Get reference to FractureTools object.
	*/
	virtual class FractureToolsAPI*			getFractureTools() const = 0;

protected:
	virtual									~ModuleDestructible() {}
};


#if !defined(_USRDLL)
/**
* If this module is distributed as a static library, the user must call this
* function before calling ApexSDK::createModule("Destructible")
*/
void instantiateModuleDestructible();
#endif

PX_POP_PACK

}
} // end namespace nvidia

#endif // MODULE_DESTRUCTIBLE_H
