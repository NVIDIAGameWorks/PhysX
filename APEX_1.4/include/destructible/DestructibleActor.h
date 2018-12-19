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



#ifndef DESTRUCTIBLE_ACTOR_H
#define DESTRUCTIBLE_ACTOR_H

#include "foundation/Px.h"
#include "Actor.h"
#include "Renderable.h"
#include "ModuleDestructible.h"
#include "PxForceMode.h"

namespace physx
{
	class PxRigidDynamic;
};

#define DESTRUCTIBLE_ACTOR_TYPE_NAME "DestructibleActor"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

struct DestructibleParameters;
class RenderVolume;
class EmitterActor;
class DestructibleRenderable;
class DestructibleBehaviorGroupDesc;


/**
	Determines which type of emitter to associate with a render volume
*/
struct DestructibleEmitterType
{
	/**
		Enum of destructible emitter types
	*/
	enum Enum
	{
		Crumble,
		Dust,	// Note: this is a placeholder.  Its implementation has been removed in 1.2.0, and will be restored in later versions.
		Count
	};
};

/**
	Provides access to specific NvParamterized types provided by the destructible actor
*/
struct DestructibleParameterizedType
{
	/**
		Enum of destructible parameterized types
	*/
	enum Enum
	{
		State,	// The full state of the actor (including params, chunk data, etc...)
		Params,	// The parameters used to initialize the actor
	};
};

/**
	Hit chunk info.  Hit chunks are those directly affected by fracture events.  See getHitChunkHistory and forceChunkHits.
*/
struct DestructibleHitChunk
{
	uint32_t chunkIndex; 	///< chunk index
	uint32_t hitChunkFlags; ///< hit chunk flag
};

/**
	Flags for managing the sync state of the destructible actor
*/
struct DestructibleActorSyncFlags
{
	/**
		Enum of destructible actor sync flags
	*/
    enum Enum
    {
		None				= 0,
		CopyDamageEvents	= (1 << 0),
		ReadDamageEvents	= (1 << 1),
		CopyFractureEvents	= (1 << 2),
		ReadFractureEvents	= (1 << 3),
		CopyChunkTransform	= (1 << 4),
		ReadChunkTransform	= (1 << 5),
		Last				= (1 << 6),
    };
};

/**
	Flags which control which actors are returned by DestructibleActor::acquirePhysXActorBuffer
*/
struct DestructiblePhysXActorQueryFlags
{
	/**
		Enum of destructible PhysX actor query flags
	*/
	enum Enum
	{
		None = 0,

		// Actor states
		Static = (1 << 0),	/// Destructible-static, which is a kinematic PhysX actor that hasn't been turned dynamic yet
		Dynamic = (1 << 1),	/// Dynamic, not dormant (not kinematic)
		Dormant = (1 << 2),	/// Dynamic, but dormant (had been made dynamic, but is now in a dormant, PhysX-kinematic state)

		AllStates = Static | Dynamic | Dormant,

		// Other filters
		/**
		Whether or not to ensure that PhysX actors are not listed more than once when this NxDestructibleActor is
		part of an extended structure.  If this is true, then some NxDestructibleActors may not return all PhysX actors associated with
		all of their chunks (and in fact may return no PhysX actors), but after querying all NxDestructibleActors in a given structure,
		every PhysX actor will be accounted for.
		*/
		AllowRedundancy = (1 << 3),

		/**
		Whether or not to allow actors not yet put into the PxScene (e.g. there has not been a simulation step since the actor was created) are also returned.
		*/
		AllowActorsNotInScenes = (1 << 4)
	};
};

/**
	Tweak-able parameters on source-side for controlling the actor sync state.
*/
struct DestructibleActorSyncState
{
	uint32_t	damageEventFilterDepth;		///< dictates the (inclusive) maximum depth at which damage events will be buffered.
	uint32_t	fractureEventFilterDepth;	///< dictates the (inclusive) maximum depth at which fracture events will be buffered.
};

/**
	Tweak-able parameters on source-side for controlling the chunk sync state. DestructibleActorSyncFlags::CopyChunkTransform must first be set.
*/
struct DestructibleChunkSyncState
{
	bool		disableTransformBuffering;	///< a handy switch for controlling whether chunk transforms will be buffered this instance.
	bool		excludeSleepingChunks;		///< dictates whether chunks that are sleeping will be buffered.
	uint32_t	chunkTransformCopyDepth;	///< dictates the (inclusive) maximum depth at which chunk transforms will be buffered.
};

/**
	Flags which describe an actor chunk (as opposed to an asset chunk) - takes into account the actor's condition, for example
	a chunk may be world supported because of the actor's placement
*/
struct DestructibleActorChunkFlags
{
	/**
		Enum of destructible actor chunk flags
	*/
	enum Enum
	{
		/**
			"Use world support" is set in the destructible parameters, and this chunk is a support-depth chunk that
			overlaps the physx scene's static geometry
		*/
		ChunkIsWorldSupported	=	(1<<0),
	};
};

/**
	Destructible actor API.  The destructible actor is instanced from an DestructibleAsset.
*/
class DestructibleActor : public Actor, public Renderable
#if PX_PHYSICS_VERSION_MAJOR == 3
	, public ActorSource
#endif
{
public:
	/**
		Get the render mesh actor for the specified mesh type.
	*/
	virtual const RenderMeshActor* 	getRenderMeshActor(DestructibleActorMeshType::Enum type = DestructibleActorMeshType::Skinned) const = 0;

	/**
		Gets the destructible's DestructibleParameter block of parameters.  These are initially set from the asset.
	*/
	virtual const DestructibleParameters& getDestructibleParameters() const = 0;

	/**
		Sets the destructible's DestructibleParameter block of parameters.  These may be set at runtime.
	*/
	virtual void 					setDestructibleParameters(const DestructibleParameters& destructibleParameters) = 0;

	/**
		Gets the global pose used when the actor is added to the scene, in the DestructibleActorDesc
	*/
	virtual PxMat44					getInitialGlobalPose() const = 0;

	/**
		Resets the initial global pose used for support calculations when the first simulation step is run.
	*/
	virtual void					setInitialGlobalPose(const PxMat44& pose) = 0;

	/**
		Gets the destructible actor's 3D (possibly nonuniform) scale
	*/
	virtual PxVec3					getScale() const = 0;

	/**
		Returns true iff the destructible actor starts off life dynamic.
	*/
	virtual bool					isInitiallyDynamic() const = 0;

	/**
		Returns an array of visibility data for each chunk.  Each byte in the array is 0 if the
		corresponding chunkIndex is invisible, 1 if visibile.
		
		\param visibilityArray		a pointer to the byte buffer to hold the visibility values.
		\param visibilityArraySize	the size of the visibilityArray
	*/
	virtual void					getChunkVisibilities(uint8_t* visibilityArray, uint32_t visibilityArraySize) const = 0;

	/**
		Returns the number of visible chunks.  This is the number of 1's written to the visibilityArray by getChunkVisibilities.
	*/
	virtual uint32_t				getNumVisibleChunks() const = 0;

	/**
		Returns a pointer to an array of visible chunk indices.
	*/
	virtual const uint16_t*			getVisibleChunks() const = 0;

	/**
		Locks the chunk event buffer, and (if successful) returns the head of the chunk event buffer in the buffer field and the length
		of the buffer (the number of events) in the bufferSize field.  To unlock the buffer, use releaseChunkEventBuffer().
		See DestructibleChunkEvent.  This buffer is filled with chunk events if the DestructibleActor parameter createChunkEvents is set to true.
		This buffer is not automatically cleared by APEX.  The user must clear it using releaseChunkEventBuffer(true).

		N.B. This function only works when the user has *not* requested chunk event callbacks via ModuleDestructible::scheduleChunkStateEventCallback.

		\return		Returns true if successful, false otherwise.
	*/
	virtual bool					acquireChunkEventBuffer(const nvidia::DestructibleChunkEvent*& buffer, uint32_t& bufferSize) = 0;

	/**
		Releases the chunk event buffer, which may have been locked by acquireChunkEventBuffer().
		If clearBuffer is true, the buffer will be erased before it is unlocked.

		\return		Returns true if successful, false otherwise.
	*/
	virtual bool					releaseChunkEventBuffer(bool clearBuffer = true) = 0;

	/**
		Locks a PhysX actor buffer, and (if successful) returns the head of the buffer in the buffer field and the length
		of the buffer (the number of PhysX actors) in the bufferSize field.
		To unlock the buffer, use releasePhysXActorBuffer().
		The user must release this buffer before another call to releasePhysXActorBuffer.
			
		\param buffer						returned buffer, if successful
		\param bufferSize					returned buffer size, if successful
		\param flags						flags which control which actors are returned.  See DestructiblePhysXActorQueryFlags.
		
		\return								Returns true if successful, false otherwise.
	*/
	virtual bool					acquirePhysXActorBuffer(physx::PxRigidDynamic**& buffer, uint32_t& bufferSize, uint32_t flags = DestructiblePhysXActorQueryFlags::AllStates) = 0;

	/**
		Releases the PhysX actor buffer, which may have been locked by acquirePhysXActorBuffer().
		The buffer will be erased before it is unlocked.
		
		\return Returns true if successful, false otherwise.
	*/
	virtual bool					releasePhysXActorBuffer() = 0;

	/**
		Returns the PhysX actor associated with the given chunk.  Note, more than one chunk may be associated with a given PhysX actor, and
		chunks from different DestructibleActors may even be associated with the same PhysX actor.
		Caution is recommended when using this function.  During APEX scene simulate/fetchResults, this actor may be deleted, replaced, or tampered with.
		When the chunk in question is not visible, but an ancestor of a visible chunk, the visible ancestor's shapes are returned.
			
		\param chunkIndex	the chunk index within the actor
	*/
	virtual physx::PxRigidDynamic*	getChunkPhysXActor(uint32_t chunkIndex) = 0;

	/**
		Returns the PhysX shapes associated with the given chunk.
		Caution is recommended when using this function.  During APEX scene simulate/fetchResults, this actor may be deleted, replaced, or tampered with.
		It is safe to use the results of this function during the chunk event callback (see ModuleDestructible::setChunkReport).

		\param shapes		returned pointer to array of shapes.  May be NULL.
		\param chunkIndex	the chunk index within the actor
		
		\return				size of array pointed to by shapes pointer.  0 if shapes == NULL.
	*/
	virtual uint32_t				getChunkPhysXShapes(physx::PxShape**& shapes, uint32_t chunkIndex) const = 0;

	/**
		Returns current pose of a chunk's reference frame, without scaling. This pose can be used to transform the chunk's hull data
		from the asset into global space.

		\param chunkIndex	the chunk index within the actor

		\note	This pose's translation might not be inside the chunk. Use getChunkBounds to get a better representation
				of where the chunk is located.
	*/
	virtual PxTransform				getChunkPose(uint32_t chunkIndex) const = 0;

	/**
		Returns current pose of a chunk's reference frame, without scaling. This pose can be used to transform the chunk's hull data
		from the asset into global space.

		\param chunkIndex	the chunk index within the actor

		\note	This pose's translation might not be inside the chunk. Use getChunkBounds to get a better representation
				of where the chunk is located.
	*/
	virtual PxTransform				getChunkTransform(uint32_t chunkIndex) const = 0;

	/**
		Returns a chunk's linear velocity in world space.

		\param chunkIndex	the chunk index within the actor
	*/
	virtual PxVec3					getChunkLinearVelocity(uint32_t chunkIndex) const = 0;

	/**
		Returns a chunk's angular velocity in world space.

		\param chunkIndex	the chunk index within the actor
	*/
	virtual PxVec3					getChunkAngularVelocity(uint32_t chunkIndex) const = 0;

	/**
		Returns the transform of the chunk's graphical representation.  This may have
		a scale component.

		\param chunkIndex	the chunk index within the actor
	*/
	virtual const PxMat44 			getChunkTM(uint32_t chunkIndex) const = 0;

	/**
		Returns the behavior group index associated with the chunk.  Use getBehaviorGroup() to access the behavior group data.
		The index will either be -1, in which case it is the default behavior group, or in the range [0, getCustomBehaviorGroupCount()-1].
		Any of those values is valid for getBehaviorGroup().

		\param chunkIndex	the chunk index within the actor
	*/
	virtual int32_t					getChunkBehaviorGroupIndex(uint32_t chunkIndex) const = 0;

	/**
		Returns the DestructibleActorChunkFlags for a chunk.  These are flags that describe
		aspects of the chunk that can only be determined at runtime.

		\param chunkIndex	the chunk index within the actor
	*/
	virtual uint32_t		 		getChunkActorFlags(uint32_t chunkIndex) const = 0;

	/**
		Set the destructible actor's global pose.  This will only be applied to the physx PxActor or PxActor
		for the static chunks, and therefore will apply to all static chunks in the structure which contains
		the this destructible actor.  This pose should not contain scale, as the scale is already contained
		in the actor's scale parameter.
	*/
	virtual void					setGlobalPose(const PxMat44& pose) = 0;

	/**
		Get the destructible actor's global pose.  This will be the pose of the physx PxActor or PxActor
		for the static chunks in the structure containing this actor, if there are static chunks.  If there
		are no static chunks in the structure, pose will not be modified and false will be returned.  Otherwise
		pose will be filled in with a scale-free transform, and true is returned.
	*/
	virtual bool					getGlobalPose(PxMat44& pose) = 0;

	/**
		Sets the linear velocity of every dynamic chunk to the given value.
	*/
	virtual void					setLinearVelocity(const PxVec3& linearVelocity) = 0;

	/**
		Sets the angular velocity of every dynamic chunk to the given value.
	*/
	virtual void					setAngularVelocity(const PxVec3& angularVelocity) = 0;

	/**
		If the indexed chunk is visible, it is made dynamic (if it is not already).
		If ModuleDestructibleConst::INVALID_CHUNK_INDEX is passed in, all visible chunks in the
		destructible actor are made dynamic, if they are not already.
	*/
	virtual void					setDynamic(int32_t chunkIndex = ModuleDestructibleConst::INVALID_CHUNK_INDEX) = 0;

	/**
		Returns true if the chunkIndex is valid, and the indexed chunk is dynamic.  Returns false otherwise.
	*/
	virtual bool					isDynamic(uint32_t chunkIndex) const = 0;

	/**
		If "hard sleeping" is enabled, physx actors for chunk islands that go to sleep will be turned kinematic.  If a chunk
		island has chunks from more than one DestructibleActor, then hard sleeping will be used if ANY of the destructibles
		have hard sleeping enabled.
	*/
	virtual void					enableHardSleeping() = 0;

	/**
		See the comments for enableHardSleeping() for a description of this feature.  The disableHardSleeping function takes
		a "wake" parameter, which (if true) will not only turn kinematic-sleeping actors dynamic, but wake them as well.
	*/
	virtual void					disableHardSleeping(bool wake = false)= 0;

	/**
		Returns true iff hard sleeping is selected for this DestructibleActor.
	*/
	virtual bool					isHardSleepingEnabled() const = 0;

	/**
		Puts the PxActor associated with the given chunk to sleep, or wakes it up, depending upon the value of the 'awake' bool.
			
		\param chunkIndex	the chunk index within the actor
		\param awake		if true, wakes the actor, otherwise puts the actor to sleep

		Returns true iff successful.
	*/
	virtual	bool					setChunkPhysXActorAwakeState(uint32_t chunkIndex, bool awake) = 0;

	/**
		Apply force to chunk's actor

		\param chunkIndex	the chunk index within the actor
		\param force		force, impulse, velocity change, or acceleration (depending on value of mode)
		\param mode			PhysX force mode (PxForceMode::Enum)
		\param position		if not null, applies force at position.  Otherwise applies force at center of mass
		\param wakeup		if true, the actor is awakened

		Returns true iff successful.
	*/
	virtual bool					addForce(uint32_t chunkIndex, const PxVec3& force, physx::PxForceMode::Enum mode, const PxVec3* position = NULL, bool wakeup = true) = 0;

	/**
		Sets the override material.
	*/
	virtual void					setSkinnedOverrideMaterial(uint32_t submeshIndex, const char* overrideMaterialName) = 0;

	/**
		Sets the override material.
	*/
	virtual void					setStaticOverrideMaterial(uint32_t submeshIndex, const char* overrideMaterialName) = 0;

	/**
		Sets the override fracture pattern.
	*/
	virtual void					setRuntimeFractureOverridePattern(const char* overridePatternName) = 0;

	/**
		Damage
	*/

	/**
		Apply damage at a point.  Damage will be propagated into the destructible based
		upon its DestructibleParameters.

		\param damage		the amount of damage at the damage point
		\param momentum		the magnitude of the impulse to transfer to the actor
		\param position		the damage location
		\param direction	direction of impact.  This is valid even if momentum = 0, for use in deformation calculations.
		\param chunkIndex	which chunk to damage (returned by rayCast and ModuleDestructible::getDestructibleAndChunk).
								If chunkIndex = ModuleDestructibleConst::INVALID_CHUNK_INDEX, then the nearest visible chunk hit is found.
		\param damageUserData		pointer which will be returned in damage and fracture event callbacks
	*/
	virtual void					applyDamage(float damage, float momentum, const PxVec3& position, const PxVec3& direction, int32_t chunkIndex = ModuleDestructibleConst::INVALID_CHUNK_INDEX, void* damageUserData = NULL) = 0;

	/**
		Apply damage to all chunks within a radius.  Damage will also propagate into the destructible
		based upon its DestructibleParameters.

		\param damage		the amount of damage at the damage point
		\param momentum		the magnitude of the impulse to transfer to each chunk
		\param position		the damage location
		\param radius		distance from damage location at which chunks will be affected
		\param falloff		whether or not to decrease damage with distance from the damage location.  If true,
								damage will decrease linearly from the full damage (at zero distance) to zero damage (at radius).
								If false, full damage is applied to all chunks within the radius.
		\param damageUserData		pointer which will be returned in damage and fracture event callbacks
	*/
	virtual void					applyRadiusDamage(float damage, float momentum, const PxVec3& position, float radius, bool falloff, void* damageUserData = NULL) = 0;

	/**
		Register a rigid body impact for impact-based damage.  Much like applyDamage, but multplies the input 'force' by the destructible's forceToDamage parameter, and also allows the user
		to report the impacting PhysX actor
	*/
	virtual void					takeImpact(const PxVec3& force, const PxVec3& position, uint16_t chunkIndex, PxActor const* damageImpactActor) = 0;

	/**
		PhysX SDK 3.X.
		Returns the index of the first visible chunk hit in the actor by worldRay, if any.
		Otherwise returns ModuleDestructibleConst::INVALID_CHUNK_INDEX.
		If a chunk is hit, the time and normal fields are modified.

		\param[out] time			(return value) of the time to the hit chunk, if any.
		\param[out] normal			(return value) the surface normal of the hit chunk's collision volume, if any.
		\param worldRayOrig			origin of the ray to fire at the actor (the direction need not be normalized)
		\param worldRayDir			direction of the ray to fire at the actor (the direction need not be normalized)
		\param flags				raycast control flags (see DestructibleActorRaycastFlags)
		\param parentChunkIndex		(if not equal to ModuleDestructibleConst::INVALID_CHUNK_INDEX)
										the chunk subhierarchy in which to confine the raycast.  If parentChunkIndex =
										ModuleDestructibleConst::INVALID_CHUNK_INDEX, then the whole actor is searched.
	*/
	virtual int32_t					rayCast(float& time, PxVec3& normal, const PxVec3& worldRayOrig, const PxVec3& worldRayDir, DestructibleActorRaycastFlags::Enum flags, int32_t parentChunkIndex = ModuleDestructibleConst::INVALID_CHUNK_INDEX) const = 0;

	/**
		Physx SDK 3.X.
		Returns the index of the first visible chunk hit in the actor by swept oriented bounding box, if any.
		Otherwise returns ModuleDestructibleConst::INVALID_CHUNK_INDEX.
		If a chunk is hit, the time and normal fields are modified.

		\param[out] time			(return value) of the time to the hit chunk, if any.
		\param[out] normal			(return value) the surface normal of the hit chunk's collision volume, if any.
		\param worldBoxCenter		the center of the obb to sweep against the actor, oriented in world space
		\param worldBoxExtents		the extents of the obb to sweep against the actor, oriented in world space
		\param worldBoxRot			the rotation of the obb to sweep against the actor, oriented in world space
		\param worldDisplacement	the displacement of the center of the worldBox through the sweep, in world space
		\param flags				raycast control flags (see DestructibleActorRaycastFlags)
	*/
	virtual int32_t					obbSweep(float& time, PxVec3& normal, const PxVec3& worldBoxCenter, const PxVec3& worldBoxExtents, const PxMat33& worldBoxRot, const PxVec3& worldDisplacement, DestructibleActorRaycastFlags::Enum flags) const = 0;

	/**
		Enable/disable the crumble emitter
	*/
	virtual void					setCrumbleEmitterState(bool enable) = 0;

	/**
		Enable/disable the dust emitter
		Note - this is a placeholder API.  The dust feature is disabled in 1.2.0.
	*/
	virtual void					setDustEmitterState(bool enable) = 0;

	/**
		Sets a preferred render volume for a dust or crumble emitter
		Note - only crumble emitters are implemented in 1.2.0
	*/
	virtual void					setPreferredRenderVolume(RenderVolume* volume, DestructibleEmitterType::Enum type) = 0;


	/**
		Returns the EmitterActor of either a dust or crumble emitter
		Note - only crumble emitters are implemented in 1.2.0
	*/
	virtual EmitterActor*			getApexEmitter(DestructibleEmitterType::Enum type) = 0;

	/**
	*   Recreates the Apex Emitter, if necessary.  Use this method to re-initialize the crumble or dust emitter after a change has been made to a dependent asset
	*	Note - only crumble emitters are implemented in 1.2.0
	*/
	virtual bool					recreateApexEmitter(DestructibleEmitterType::Enum type) = 0;

	/**
	 * \brief Returns the actor's NvParamaterized interface
	 * This cannot be directly modified!  It is read only to the user.
	 * This handle can be used to directly serialize the complete actor state.
	 */
	virtual const ::NvParameterized::Interface*	getNvParameterized(DestructibleParameterizedType::Enum type = DestructibleParameterizedType::State) const = 0;

	/**
	 * \brief Sets the actor's state via the NvParameterized object
	 * This can be used to update the state from deserialized data.
	 * The actor assumes control of the interface.
	 */
	virtual void					setNvParameterized(::NvParameterized::Interface*) = 0;

	/**
		Set the syncing properties of the destructible actor.

		\param userActorID		user-defined value used to identify the syncing actor.
									This value will be used to identify destructible actors between the server and client.
									userActorID = 0 is used for unregistering the actor as a syncing actor, and is the default value.
									The other arguments will then be forcibly set to the default (non-participating) values.
									userActorID != 0 registers the actor as a participating syncing actor.
									userActorID can be overwritten. In this case, the destructible actor which used to hold this userActorID
									will behave exactly like a call to set userActorID to 0.

		\param actorSyncFlags	describes the kind of actor information that participates in syncing. See struct DestructibleActorSyncFlags

		\param actorSyncState	describes information that allows finer control over the actor that participates in syncing. See struct DestructibleActorSyncState

		\param chunkSyncState	describes information that allows finer control over the chunk that participates in syncing. See struct DestructibleChunkSyncState

		\return					Returns true if arguments are accepted. Any one invalid argument will cause a return false. In such a case, no changes are made.
    */
    virtual bool                    setSyncParams(uint32_t userActorID, uint32_t actorSyncFlags = 0, const DestructibleActorSyncState * actorSyncState = NULL, const DestructibleChunkSyncState * chunkSyncState = NULL) = 0;

	/**
		Set the tracking properties of the actor for chunks that are hit. Chunks that are hit are chunks directly affected by fracture events.

		\param flushHistory		flushHistory == true indicates that both the cached chunk hit history and the cached damage event core data will be cleared. 
									To get the chunk hit history of this actor, see getHitChunkHistory()
									To get the damage coloring history of this actor, see getDamageColoringHistory()
		\param startTracking	startTracking == true indicates that chunk hits and damage coloring will begin caching internally. The actor does not cache chunk hits by default.
		\param trackingDepth	the depth at which hit chunks will be cached. This value should not exceed the maximum depth level.
		\param trackAllChunks	trackAllChunks == true indicates that all the chunks will be cached, trackAllChunks == false indicates that only static chunks will be cached.

		Returns true if the function executes correctly with the given arguments.
	*/
	virtual bool					setHitChunkTrackingParams(bool flushHistory, bool startTracking, uint32_t trackingDepth, bool trackAllChunks = true) = 0;

	/**
		Get the chunk hit history of the actor. To start caching chunk hits, see setHitChunkTrackingParams()

		\return Returns true if the function executes correctly with the given arguments.
	*/
	virtual bool					getHitChunkHistory(const DestructibleHitChunk *& hitChunkContainer, uint32_t & hitChunkCount) const = 0;

	/**
		Force the actor to register chunk hits.

		\param hitChunkContainer	should take in an argument that was generated from another destructible actor. See getHitChunkHistory()
		\param hitChunkCount		hit chunk count	
		\param removeChunks			removeChunks == true indicates that the chunks given by hitChunkContainer will be forcibly removed.
		\param deferredEvent		whether to enable deferred event mode. If true, fracture events won't get processed until the next tick.
		\param damagePosition		passed through to ApexDamageEventReportData::hitPosition and hitDirection in the damage notify output by APEX.
		\param damageDirection		passed through to ApexDamageEventReportData::hitPosition and hitDirection in the damage notify output by APEX.
	*/
	virtual bool					forceChunkHits(const DestructibleHitChunk * hitChunkContainer, uint32_t hitChunkCount, bool removeChunks = true, bool deferredEvent = false, PxVec3 damagePosition = PxVec3(0.0f), PxVec3 damageDirection = PxVec3(0.0f)) = 0;

	/**
		Get the damage coloring history of the actor. To start caching damage coloring, see setHitChunkTrackingParams()

		\return		Returns true if the function executes correctly with the given arguments.
	*/
	virtual bool					getDamageColoringHistory(const DamageEventCoreData *& damageEventCoreDataContainer, uint32_t & damageEventCoreDataCount) const = 0;

	/**
		Force the actor to register damage coloring.

		\param damageEventCoreDataContainer		should take in an argument that was generated from another destructible actor. See getDamageColoringHistory()
		\param damageEventCoreDataCount			the count of damageEventCoreDataContainer.

		\return									Returns true if the function executes correctly with the given arguments.
	*/
	virtual bool					forceDamageColoring(const DamageEventCoreData * damageEventCoreDataContainer, uint32_t damageEventCoreDataCount) = 0;

	/**
		Accessor to get the initial locally-aligned bounding box of a destructible actor.
	*/
	virtual PxBounds3				getLocalBounds() const = 0;

	/**
		Accessor to get the initial world axis-aligned bounding box of a destructible actor.
	*/
	virtual PxBounds3				getOriginalBounds() const = 0;

	/**
		Accessor to query if a chunk is part of a detached island.
	*/
	virtual bool					isChunkSolitary(int32_t chunkIndex) const = 0;

	/**
		Accessor to query the axis aligned bounding box of a given chunk in world-space.
	*/
	virtual PxBounds3				getChunkBounds(uint32_t chunkIndex) const = 0;

	/**
		Accessor to query the axis aligned bounding box of a given chunk in chunk local-space.
	*/
	virtual PxBounds3				getChunkLocalBounds(uint32_t chunkIndex) const = 0;

	/**
		Accessor to query if a chunk has been destroyed.
	*/
	virtual bool					isChunkDestroyed(int32_t chunkIndex) const = 0;

	/**
		Accessor to get the array of chunk indices at the support depth.
	*/
	virtual uint32_t				getSupportDepthChunkIndices(uint32_t* const OutChunkIndices, uint32_t MaxOutIndices) const = 0;

	/**
		Query the actor's support depth.
	*/
	virtual uint32_t				getSupportDepth() const = 0;

	/**
		Set the actor to delete its fractured chunks instead of simulating them.
    */
	virtual void					setDeleteFracturedChunks(bool inDeleteChunkMode) = 0;

	/**
		Acquire a pointer to the destructible's renderable proxy and increment its reference count.  The DestructibleRenderable will
		only be deleted when its reference count is zero.  Calls to DestructibleRenderable::release decrement the reference count, as does
		a call to DestructibleActor::release().  .
	*/
	virtual DestructibleRenderable*	acquireRenderableReference() = 0;

	/**
		Access to behavior groups created for this actor.  Each chunk has a behavior group index associated with it.

		\return This returns the number of custom (non-default) behavior groups.
	*/
	virtual uint32_t				getCustomBehaviorGroupCount() const = 0;

	/**
		Access to behavior groups created for this actor.  Each chunk has a behavior group index associated with it.

		This returns the indexed behavior group.  The index must be either -1 (for the default group) or in the range [0, getCustomBehaviorGroupCount()-1].
		If any other index is given, this function returns false.  Otherwise it returns true and the behavior descriptor is filled in.
	*/
	virtual bool					getBehaviorGroup(nvidia::DestructibleBehaviorGroupDesc& behaviorGroupDesc, int32_t index = -1) const = 0;

protected:
	virtual							~DestructibleActor() {}
};

PX_POP_PACK

}
} // end namespace nvidia

#endif // DESTRUCTIBLE_ACTOR_H
