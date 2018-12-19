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



#ifndef DESTRUCTIBLE_ASSET_H
#define DESTRUCTIBLE_ASSET_H

#define DESTRUCTIBLE_AUTHORING_TYPE_NAME "DestructibleAsset"

#include "foundation/Px.h"
#include "FractureToolsAPI.h"
// TODO: Remove this include when we remove the APEX_RUNTIME_FRACTURE define
#include "ModuleDestructible.h"

#if PX_PHYSICS_VERSION_MAJOR == 3
#include "PxFiltering.h"
#endif

namespace nvidia
{
namespace apex
{

struct IntPair;
class DestructibleActor;
class DestructiblePreview;

PX_PUSH_PACK_DEFAULT
#if !PX_PS4
	#pragma warning(push)
	#pragma warning(disable:4121)
#endif	//!PX_PS4

/**
	Flags that may be set for all chunks at a particular depth in the fracture hierarchy
*/
struct DestructibleDepthParametersFlag
{
	/**
		Enum of destructible depth parameters flag
	*/
	enum Enum
	{
		/**
			If true, chunks at this hierarchy depth level will take impact damage iff OVERRIDE_IMPACT_DAMAGE = TRUE, no matter the setting of impactDamageDefaultDepth..
			Note, DestructibleParameters::forceToDamage must also be positive for this
			to take effect.
		*/
		OVERRIDE_IMPACT_DAMAGE			= (1 << 0),

		/**
			If OVERRIDE_IMPACT_DAMAGE = TRUE, chunks at this hierarchy depth level will take impact damage iff OVERRIDE_IMPACT_DAMAGE = TRUE, no matter the setting of impactDamageDefaultDepth..
		*/
		OVERRIDE_IMPACT_DAMAGE_VALUE	= (1 << 1),

		/**
			Chunks at this depth should have pose updates ignored.
		*/
		IGNORE_POSE_UPDATES				= (1 << 2),

		/**
			Chunks at this depth should be ignored in raycast callbacks.
		*/
		IGNORE_RAYCAST_CALLBACKS		= (1 << 3),

		/**
			Chunks at this depth should be ignored in contact callbacks.
		*/
		IGNORE_CONTACT_CALLBACKS		= (1 << 4),

		/**
			User defined flags.
		*/
		USER_FLAG_0						= (1 << 24),
		USER_FLAG_1						= (1 << 25),
		USER_FLAG_2						= (1 << 26),
		USER_FLAG_3						= (1 << 27)
	};
};


/**
	Parameters that may be set for all chunks at a particular depth in the fracture hierarchy
*/
struct DestructibleDepthParameters
{
	/**
	\brief constructor sets to default.
	*/
	PX_INLINE		DestructibleDepthParameters();

	/**
	\brief (re)sets the structure to the default.
	*/
	PX_INLINE void	setToDefault();

	/**
	\brief (re)sets the structure to parameters which are likely to be the most computationally expensive.
	*/
	PX_INLINE void	setToMostExpensive();

	/**
		A convenience function to determine if the OVERRIDE_IMPACT_DAMAGE is set.
	*/
	PX_INLINE bool	overrideImpactDamage() const;

	/**
		A convenience function to read OVERRIDE_IMPACT_DAMAGE_VALUE is set.
	*/
	PX_INLINE bool	overrideImpactDamageValue() const;

	/**
		A convenience function to determine if the IGNORE_POSE_UPDATES is set.
	*/
	PX_INLINE bool	ignoresPoseUpdates() const;

	/**
		A convenience function to determine if the IGNORE_RAYCAST_CALLBACKS is set.
	*/
	PX_INLINE bool	ignoresRaycastCallbacks() const;

	/**
		A convenience function to determine if the IGNORE_CONTACT_CALLBACKS is set.
	*/
	PX_INLINE bool	ignoresContactCallbacks() const;

	/**
		A convenience function to determine if the USER_FLAG_0, USER_FLAG_1, USER_FLAG_2, or USER_FLAG_3 flag is set.
	*/
	PX_INLINE bool	hasUserFlagSet(uint32_t flagIndex) const;

	/**
		A collection of flags defined in DestructibleDepthParametersFlag.
	*/
	uint32_t		flags;
};

// DestructibleDepthParameters inline functions

PX_INLINE DestructibleDepthParameters::DestructibleDepthParameters()
{
	setToDefault();
}

PX_INLINE void DestructibleDepthParameters::setToDefault()
{
	flags = 0;
}

PX_INLINE bool DestructibleDepthParameters::overrideImpactDamage() const
{
	return (flags & DestructibleDepthParametersFlag::OVERRIDE_IMPACT_DAMAGE) != 0;
}

PX_INLINE bool DestructibleDepthParameters::overrideImpactDamageValue() const
{
	return (flags & DestructibleDepthParametersFlag::OVERRIDE_IMPACT_DAMAGE_VALUE) != 0;
}

PX_INLINE bool DestructibleDepthParameters::ignoresPoseUpdates() const
{
	return (flags & DestructibleDepthParametersFlag::IGNORE_POSE_UPDATES) != 0;
}

PX_INLINE bool DestructibleDepthParameters::ignoresRaycastCallbacks() const
{
	return (flags & DestructibleDepthParametersFlag::IGNORE_RAYCAST_CALLBACKS) != 0;
}

PX_INLINE bool DestructibleDepthParameters::ignoresContactCallbacks() const
{
	return (flags & DestructibleDepthParametersFlag::IGNORE_CONTACT_CALLBACKS) != 0;
}

PX_INLINE bool DestructibleDepthParameters::hasUserFlagSet(uint32_t flagIndex) const
{
	switch (flagIndex)
	{
	case 0:
		return (flags & DestructibleDepthParametersFlag::USER_FLAG_0) != 0;
	case 1:
		return (flags & DestructibleDepthParametersFlag::USER_FLAG_1) != 0;
	case 2:
		return (flags & DestructibleDepthParametersFlag::USER_FLAG_2) != 0;
	case 3:
		return (flags & DestructibleDepthParametersFlag::USER_FLAG_3) != 0;
	default:
		return false;
	}
}

/**
	Parameters for RT Fracture
*/
struct DestructibleRTFractureParameters
{
	/**
		If true, align fracture pattern to largest face. 
		If false, the fracture pattern will be aligned to the hit normal with each fracture.
	*/
	bool sheetFracture;

	/**
		Number of times deep a chunk can be fractured. Can help limit the number of chunks produced by
		runtime fracture.
	*/
	uint32_t depthLimit;

	/**
		If true, destroy chunks when they hit their depth limit. 
		If false, then chunks at their depth limit will not fracture but will have a force applied.
	*/
	bool destroyIfAtDepthLimit;

	/**
		Minimum Convex Size. Minimum size of convex produced by a fracture.
	*/
	float minConvexSize;

	/**
		Scales impulse applied by a fracture.
	*/
	float impulseScale;

	/**
		Parameters for glass fracture
	*/
	struct FractureGlass 
	{
		/**
			Number of angular slices in the glass fracture pattern.
		*/
		uint32_t numSectors;

		/**
			Creates variance in the angle of slices. A value of zero results in all angular slices having the same angle.
		*/
		float sectorRand;

		/**
			The minimum shard size. Shards below this size will not be created and thus not visible.
		*/
		float firstSegmentSize;

		/**
			Scales the radial spacing in the glass fracture pattern. A larger value results in radially longer shards.
		*/
		float segmentScale;

		/**
			Creates variance in the radial size of shards. A value of zero results in a low noise circular pattern.
		*/
		float segmentRand;
	}glass; ///< Instance of FractureGlass parameters

	/**
		Fracture attachment
	 */
	struct FractureAttachment 
	{
		/**
			If true, make the positive x side of the sheet an attachment point.
		*/
		bool posX;

		/**
			If true, make the negative x side of the sheet an attachment point.
		*/
		bool negX;

		/**
			If true, make the positive y side of the sheet an attachment point.
		*/
		bool posY;

		/**
			If true, make the negative y side of the sheet an attachment point.
		*/
		bool negY;

		/**
			If true, make the positive z side of the sheet an attachment point.
		*/
		bool posZ;

		/**
			If true, make the negative z side of the sheet an attachment point.
		*/
		bool negZ;
	}attachment; ///< Instance of FractureAttachment parameters

	/**
		Set default fracture parameters
	*/
	PX_INLINE void	setToDefault();
};

PX_INLINE void DestructibleRTFractureParameters::setToDefault()
{
	sheetFracture = true;
	depthLimit = 2;
	destroyIfAtDepthLimit = false;
	minConvexSize = 0.02f;
	impulseScale = 1.0f;
	glass.numSectors = 10;
	glass.sectorRand = 0.3f;
	glass.firstSegmentSize = 0.06f;
	glass.segmentScale = 1.4f;
	glass.segmentRand = 0.3f;
	attachment.posX = false;
	attachment.negX = false;
	attachment.posY = false;
	attachment.negY = false;
	attachment.posZ = false;
	attachment.negZ = false;
}

/**
	Flags that apply to a destructible actor, settable at runtime
*/
struct DestructibleParametersFlag
{
	/**
		Enum of destructible parameters flag
	*/
	enum Enum
	{
		/**
			If set, chunks will "remember" damage applied to them, so that many applications of a damage amount
			below damageThreshold will eventually fracture the chunk.  If not set, a single application of
			damage must exceed damageThreshold in order to fracture the chunk.
		*/
		ACCUMULATE_DAMAGE =	(1 << 0),

		/**
			Whether or not chunks at or deeper than the "debris" depth (see DestructibleParameters::debrisDepth)
			will time out.  The lifetime is a value between DestructibleParameters::debrisLifetimeMin and
			DestructibleParameters::debrisLifetimeMax, based upon the destructible module's LOD setting.
		*/
		DEBRIS_TIMEOUT =	(1 << 1),

		/**
			Whether or not chunks at or deeper than the "debris" depth (see DestructibleParameters::debrisDepth)
			will be removed if they separate too far from their origins.  The maxSeparation is a value between
			DestructibleParameters::debrisMaxSeparationMin and DestructibleParameters::debrisMaxSeparationMax,
			based upon the destructible module's LOD setting.
		*/
		DEBRIS_MAX_SEPARATION =	(1 << 2),

		/**
			If set, the smallest chunks may be further broken down, either by fluid crumbles (if a crumble particle
			system is specified in the DestructibleActorDesc), or by simply removing the chunk if no crumble
			particle system is specified.  Note: the "smallest chunks" are normally defined to be the deepest level
			of the fracture hierarchy.  However, they may be taken from higher levels of the hierarchy if
			ModuleDestructible::setMaxChunkDepthOffset is called with a non-zero value.
		*/
		CRUMBLE_SMALLEST_CHUNKS =	(1 << 3),

		/**
			If set, the DestructibleActor::rayCast function will search within the nearest visible chunk hit
			for collisions with child chunks.  This is used to get a better raycast position and normal, in
			case the parent collision volume does not tightly fit the graphics mesh.  The returned chunk index
			will always be that of the visible parent that is intersected, however.
		*/
		ACCURATE_RAYCASTS =	(1 << 4),

		/**
			If set, the validBounds field of DestructibleParameters will be used.  These bounds are translated
			(but not scaled or rotated) to the origin of the destructible actor.  If a chunk or chunk island moves
			outside of those bounds, it is destroyed.
		*/
		USE_VALID_BOUNDS =	(1 << 5),

		/**
			If set, chunk crumbling will be handled via the pattern-based runtime fracture pipeline.
			If no fracture pattern is specified in the DestructibleActorDesc, or no fracture pattern
			has been assigned to the destructible actor, chunks will simply be removed.
		*/
		CRUMBLE_VIA_RUNTIME_FRACTURE =	(1 << 6),
	};
};


/**
	Parameters that apply to a destructible actor
*/
struct DestructibleParameters
{
	/**
	\brief constructor sets to default.
	*/
	PX_INLINE		DestructibleParameters();

	/**
	\brief (re)sets the structure to the default.
	*/
	PX_INLINE void	setToDefault();

	/**
		Limits the amount of damage applied to a chunk.  This is useful for preventing the entire destructible
		from getting pulverized by a very large application of damage.  This can easily happen when impact damage is
		used, and the damage amount is proportional to the impact force (see forceToDamage).
	*/
	float			damageCap;

	/**
		If a chunk is at a depth which takes impact damage (see DestructibleDepthParameters),
		then when a chunk has a collision in the PxScene, it will take damage equal to forceToDamage mulitplied by
		the impact force.
		The default value is zero, which effectively disables impact damage.
	*/
	float			forceToDamage;

	/**
		Large impact force may be reported if rigid bodies are spawned inside one another.  In this case the relative velocity of the two
		objects will be low.  This variable allows the user to set a minimum velocity threshold for impacts to ensure that the objects are
		moving at a min velocity in order for the impact force to be considered.
		Default value is zero.
	*/
	float			impactVelocityThreshold;

	/**
		The chunks will not be broken free below this depth.
	*/
	uint32_t		minimumFractureDepth;

	/**
		The default depth to which chunks will take impact damage.  This default may be overridden in the depth settings.
		Negative values imply no default impact damage.
		Default value = -1.
	*/
	int32_t			impactDamageDefaultDepth;

	/**
		The chunk hierarchy depth at which chunks are considered to be "debris."  Chunks at this depth or
		below will be considered for various debris settings, such as debrisLifetime.
		Negative values indicate that no chunk depth is considered debris.
		Default value is -1.
	*/
	int32_t			debrisDepth;

	/**
		The chunk hierarchy depth up to which chunks will not be eliminated due to LOD considerations.
		These chunks are considered to be essential either for gameplay or visually.
		The minimum value is 0, meaning the level 0 chunk is always considered essential.
		Default value is 0.
	*/
	uint32_t		essentialDepth;

	/**
		"Debris chunks" (see debrisDepth, above) will be destroyed after a time (in seconds)
		separated from non-debris chunks.  The actual lifetime is interpolated between these
		two values, based upon the module's LOD setting.  To disable lifetime, clear the
		DestructibleDepthParametersFlag::DEBRIS_TIMEOUT flag in the flags field.
		If debrisLifetimeMax < debrisLifetimeMin, the mean of the two is used for both.
		Default debrisLifetimeMin = 1.0, debrisLifetimeMax = 10.0f.
	*/
	float			debrisLifetimeMin;
	
	/// \see DestructibleAsset::debrisLifetimeMin
	float			debrisLifetimeMax;

	/**
		"Debris chunks" (see debrisDepth, above) will be destroyed if they are separated from
		their origin by a distance greater than maxSeparation.  The actual maxSeparation is
		interpolated between these two values, based upon the module's LOD setting.  To disable
		maxSeparation, clear the DestructibleDepthParametersFlag::DEBRIS_MAX_SEPARATION flag in the flags field.
		If debrisMaxSeparationMax < debrisMaxSeparationMin, the mean of the two is used for both.
		Default debrisMaxSeparationMin = 1.0, debrisMaxSeparationMax = 10.0f.
	*/
	float			debrisMaxSeparationMin;
	
	/// \see DestructibleAsset::debrisMaxSeparationMin
	float			debrisMaxSeparationMax;

	/**
		The probablity that a debris chunk, when fractured, will simply be destroyed instead of becoming
		dynamic or breaking down further into child chunks.  Valid range = [0.0,1.0].  Default value = 0.0.'
	*/

	float			debrisDestructionProbability;

	/**
		A bounding box around each DestructibleActor created, defining a range of validity for chunks that break free.
		These bounds are scaled and translated with the DestructibleActor's scale and position, but they are *not*
		rotated with the DestructibleActor.
	*/
	PxBounds3		validBounds;

	/**
		If greater than 0, the chunks' speeds will not be allowed to exceed maxChunkSpeed.  Use 0
		to disable this feature (this is the default).
	*/
	float			maxChunkSpeed;

	/**
		A collection of flags defined in DestructibleParametersFlag.
	*/
	uint32_t		flags;

	/**
		Scale factor used to apply an impulse force along the normal of chunk when fractured.  This is used
		in order to "push" the pieces out as they fracture.
	*/
	float			fractureImpulseScale;

	/**
		How deep in the hierarchy damage will be propagated, relative to the chunk hit.
		Default = UINT16_MAX.
	*/
	uint16_t		damageDepthLimit;

	/**
		Optional dominance group for dynamic chunks created when fractured. (ignored if > 31)
	*/
	uint8_t			dynamicChunksDominanceGroup;

	/**
		Whether or not to use dynamicChunksGroupsMask.  If false, NULL will be passed into the DestructibleActor upon
		instantiation, through the DestructibleActorDesc.
	*/
	bool			useDynamicChunksGroupsMask;

#if PX_PHYSICS_VERSION_MAJOR == 3
	/**
		Optional groups mask (2.x) or filter data (3.x) for dynamic chunks created when fractured. (Used if useDynamicChunksGroupsMask is true.)
	*/
	physx::PxFilterData	dynamicChunksFilterData;
#endif

	/**
		The supportStrength is used for the stress solver.  As this value is increased it takes more force to break apart the chunks under the effect of stress solver.
	*/
	float	   		supportStrength;

	/**
		Whether or not to use the old chunk bounds testing for damage, or use the module setting.  A value of 0 forces the new method to be used.
		A positive value forces the old method to be used.  Negative values cause the global (ModuleDestructible) setting to be used.
		Default = -1
	*/
	int8_t			legacyChunkBoundsTestSetting;

	/**
		Whether or not to use the old damage spread method, or use the module setting.  A value of 0 forces the new method to be used.
		A positive value forces the old method to be used.  Negative values cause the global (ModuleDestructible) setting to be used.
		Default = -1
	*/
	int8_t			legacyDamageRadiusSpreadSetting;

	/**
		The maximum number of DestructibleDepthParameters (see depthParameters).
	*/
	enum { kDepthParametersCountMax = 16 };

	/**
		The number of DestructibleDepthParameters (see depthParameters).
		Must be in the range [0, kDepthParametersCountMax].
	*/
	uint32_t		depthParametersCount;

	/**
		Parameters that apply to every chunk at a given depth level (see DestructibleDepthParameters).
		The element [0] of the array applies to the depth 0 (unfractured) chunk, element [1] applies
		to the level 1 chunks, etc.
		The number of valid depth parameters must be given in depthParametersCount.
	*/
	DestructibleDepthParameters	depthParameters[kDepthParametersCountMax];

	/**
		Parameters for RT Fracture.
	*/
	DestructibleRTFractureParameters rtFractureParameters;

	/**
		This flag forces drawing of scatter mesh on chunks with depth > 1.
		By default is false.
	*/
	bool 			alwaysDrawScatterMesh;
};

// DestructibleParameters inline functions

PX_INLINE DestructibleParameters::DestructibleParameters()
{
	setToDefault();
}

PX_INLINE void DestructibleParameters::setToDefault()
{
	damageCap = 0;
	forceToDamage = 0;
	impactVelocityThreshold = 0.0f;
	minimumFractureDepth = 0;
	impactDamageDefaultDepth = -1;
	debrisDepth = -1;
	essentialDepth = 0;
	debrisLifetimeMin = 1.0f;
	debrisLifetimeMax = 10.0f;
	debrisMaxSeparationMin = 1.0f;
	debrisMaxSeparationMax = 10.0f;
	debrisDestructionProbability = 0.0f;
	validBounds = PxBounds3(PxVec3(-10000.0f), PxVec3(10000.0f));
	maxChunkSpeed = 0.0f;
	fractureImpulseScale = 0.0f;
	damageDepthLimit = UINT16_MAX;
	useDynamicChunksGroupsMask = false;
#if PX_PHYSICS_VERSION_MAJOR == 3
	dynamicChunksFilterData.word0 = dynamicChunksFilterData.word1 = dynamicChunksFilterData.word2 = dynamicChunksFilterData.word3 = 0;
#endif
	supportStrength = -1.0;
	legacyChunkBoundsTestSetting = -1;
	legacyDamageRadiusSpreadSetting = -1;
	dynamicChunksDominanceGroup = 0xFF;	// Out of range, so it won't be used.
	flags = DestructibleParametersFlag::ACCUMULATE_DAMAGE;
	depthParametersCount = 0;
	rtFractureParameters.setToDefault();
	alwaysDrawScatterMesh = false;
}

/**
	Flags that apply to a destructible actor when it created
*/
struct DestructibleInitParametersFlag
{
	/**
		Enum of destructible init parameters flag
	*/
	enum Enum
	{
		/**
			If set, then chunks which are tagged as "support" chunks (via DestructibleChunkDesc::isSupportChunk)
			will have environmental support in static destructibles.
			Note: if both ASSET_DEFINED_SUPPORT and WORLD_SUPPORT are set, then chunks must be tagged as
			"support" chunks AND overlap the PxScene's static geometry in order to be environmentally supported.
		*/
		ASSET_DEFINED_SUPPORT =	(1 << 0),

		/**
			If set, then chunks which overlap the PxScene's static geometry will have environmental support in
			static destructibles.
			Note: if both ASSET_DEFINED_SUPPORT and WORLD_SUPPORT are set, then chunks must be tagged as
			"support" chunks AND overlap the PxScene's static geometry in order to be environmentally supported.
		*/
		WORLD_SUPPORT =	(1 << 1),

		/**
			If this is set and the destructible is initially static, it will become part of an extended support
			structure if it is in contact with another static destructible that also has this flag set.
		*/
		FORM_EXTENDED_STRUCTURES =	(1 << 2)
	};
};

/**
	Parameters that apply to a destructible actor at initialization
*/
struct DestructibleInitParameters
{
	/**
	\brief constructor sets to default.
	*/
	PX_INLINE		DestructibleInitParameters();

	/**
	\brief (re)sets the structure to the default.
	*/
	PX_INLINE void	setToDefault();

	/**
		The chunk hierarchy depth at which to create a support graph.  Higher depth levels give more detailed support,
		but will give a higher computational load.  Chunks below the support depth will never be supported.
	*/
	uint32_t		supportDepth;

	/**
		A collection of flags defined in DestructibleInitParametersFlag.
	*/
	uint32_t		flags;
};


// DestructibleInitParameters inline functions

PX_INLINE DestructibleInitParameters::DestructibleInitParameters()
{
	setToDefault();
}

PX_INLINE void DestructibleInitParameters::setToDefault()
{
	supportDepth = 0;
	flags = 0;
}

/**
	DamageSpreadFunction
*/
struct DamageSpreadFunction
{
	PX_INLINE		DamageSpreadFunction()
	{
		setToDefault();
	}

	/**
	\brief (re)sets the structure to the default.
	*/
	PX_INLINE void	setToDefault()
	{
		minimumRadius = 0.0f;
		radiusMultiplier = 1.0f;
		falloffExponent = 1.0f;
	}

	/**
		Returns true iff an object can be created using this descriptor.
	*/
	PX_INLINE bool	isValid() const
	{
		return
			minimumRadius >= 0.0f &&
			radiusMultiplier >= 0.0f &&
			falloffExponent >= 0.0f;
	}

	float minimumRadius; 	///< minimum radius
	float radiusMultiplier; ///< radius multiplier
	float falloffExponent;	///< falloff exponent
};

/**
	Destructible authoring structure.

	Descriptor to build one chunk in a fracture hierarchy.
*/
class DestructibleBehaviorGroupDesc : public ApexDesc
{
public:
	/**
	\brief constructor sets to default.
	*/
	PX_INLINE		DestructibleBehaviorGroupDesc();

	/**
	\brief (re)sets the structure to the default.
	*/
	PX_INLINE void	setToDefault();

	/**
		Returns true iff an object can be created using this descriptor.
	*/
	PX_INLINE bool	isValid() const;

	/**
		Defines the chunk to be environmentally supported, if the appropriate DestructibleParametersFlag flags
		are set in DestructibleParameters.
	*/
	const char*		name; ///< name
	float			damageThreshold; ///< damage threshold
	float			damageToRadius; ///< damage to radius
	DamageSpreadFunction	damageSpread; ///< damage spread
	DamageSpreadFunction	damageColorSpread; ///< damage color spread
	PxVec4			damageColorChange; ///< damege color change
	float			materialStrength; ///< material strength
	float			density; ///< density
	float			fadeOut; ///<  fade out
	float			maxDepenetrationVelocity; ///< maximal depenetration velocity
	uint64_t		userData; ///< user data
};

// DestructibleChunkDesc inline functions

PX_INLINE DestructibleBehaviorGroupDesc::DestructibleBehaviorGroupDesc()
{
	setToDefault();
}

PX_INLINE void DestructibleBehaviorGroupDesc::setToDefault()
{
	ApexDesc::setToDefault();

	// TC_TODO: suitable default values?
	name = NULL;
	damageThreshold = 1.0f;
	damageToRadius = 0.1f;
	damageSpread.setToDefault();
	damageColorSpread.setToDefault();
	damageColorChange.setZero();
	materialStrength = 0.0f;
	density = 0;
	fadeOut = 1;
	maxDepenetrationVelocity = PX_MAX_F32;
	userData = (uint64_t)0;
}

PX_INLINE bool DestructibleBehaviorGroupDesc::isValid() const
{
	// TC_TODO: this good enough?
	if (damageThreshold < 0 ||
		damageToRadius < 0 ||
		!damageSpread.isValid() ||
		!damageColorSpread.isValid() ||
		materialStrength < 0 ||
		density < 0 ||
		fadeOut < 0 ||
		!(maxDepenetrationVelocity > 0.f))
	{
		return false;
	}

	return ApexDesc::isValid();
}
/**
	Destructible authoring structure.

	Descriptor to build one chunk in a fracture hierarchy.
*/
class DestructibleChunkDesc : public ApexDesc
{
public:
	/**
	\brief constructor sets to default.
	*/
	PX_INLINE				DestructibleChunkDesc();

	/**
	\brief (re)sets the structure to the default.
	*/
	PX_INLINE void			setToDefault();

	/**
		Returns true iff an object can be created using this descriptor.
	*/
	PX_INLINE bool			isValid() const;

	/**
		Defines the chunk to be environmentally supported, if the appropriate DestructibleParametersFlag flags
		are set in DestructibleParameters.
	*/
	bool					isSupportChunk;

	/**
		Defines the chunk to be unfractureable.  If this is true, then none of its children will be fractureable.
	*/
	bool					doNotFracture;

	/**
		Defines the chunk to be undamageable.  This means this chunk will not fracture, but its children might.
	*/
	bool					doNotDamage;

	/**
		Defines the chunk to be uncrumbleable.  This means this chunk will not be broken down into fluid mesh particles
		no matter how much damage it takes.  Note: this only applies to chunks with no children.  For a chunk with
		children, then:
		1) The chunk may be broken down into its children, and then its children may be crumbled, if the doNotCrumble flag
		is not set on them.
		2) If the Destructible module's chunk depth offset LOD may be set such that this chunk effectively has no children.
		In this case, the doNotCrumble flag will apply to it.
	*/
	bool					doNotCrumble;

#if APEX_RUNTIME_FRACTURE
	/**
		Defines the chunk to use run-time, dynamic fracturing.  The chunk will use the fracture pattern provided by the asset
		to guide it's fracture.  The resulting chunks will follow a similar process for subdivision.
	*/
	bool					runtimeFracture;
#endif

	/**
		Whether or not to use instancing when rendering this chunk.  If useInstancedRendering = TRUE, this chunk will
		share a draw call with all others that instance the mesh indexed by meshIndex.  This may extend to other
		destructible actors created from this asset.  If useInstancedRendering = FALSE, this chunk may share a draw
		call only with other chunks in this asset which have useInstancedRendering = FALSE.
		Default = FALSE.
	*/
	bool					useInstancedRendering;

	/**
		Translation for this chunk mesh within the asset. Normally a chunk needs no translation, but if a chunk is instanced within
		the asset, then this translation is needed.
		Default = (0,0,0).
	*/
	PxVec3					instancePositionOffset;

	/**
		UV translation for this chunk mesh within the asset. Normally a chunk needs no UV translation, but if a chunk is instanced within
		the asset, then this translation is usually needed.
		Default = (0,0).
	*/
	PxVec2					instanceUVOffset;

	/**
		If useInstancedRendering = TRUE, this index is the instanced mesh index.  If useInstancedRendering = FALSE,
		this index is the mesh part index for the skinned or statically rendered mesh.
		This must index a valid DestructibleGeometryDesc (see below).
		Default = 0xFFFF (invalid).
	*/
	uint16_t				meshIndex;

	/**
		The parent index of this chunk.  If the index is negative, this is a root chunk.
		Default = -1.
	*/
	int32_t					parentIndex;

	/**
		A direction used to move the chunk out of the destructible, if an impact kick is applied.
	*/
	PxVec3					surfaceNormal;

	/**
		Referring to the behavior group used of this chunk, please check DestructibleAssetCookingDesc::behaviorGroupDescs for more detail.
	*/
	int8_t					behaviorGroupIndex;

	/**
		The number of scatter mesh instances on this chunk.
		Default = 0.
	*/
	uint32_t				scatterMeshCount;

	/**
		Array of indices corresponding the scatter mesh assets set using DestructibleAssetAuthoring::setScatterMeshAssets().
		The array length must be at least scatterMeshCount.  This pointer may be NULL if scatterMeshCount = 0.
		Default = NULL.
	*/
	const uint8_t*			scatterMeshIndices;

	/**
		Array of chunk-relative transforms corresponding the scatter mesh assets set using
		DestructibleAssetAuthoring::setScatterMeshAssets().
		The array length must be at least scatterMeshCount.  This pointer may be NULL if scatterMeshCount = 0.
		Default = NULL.
	*/
	const PxMat44*			scatterMeshTransforms;
};

// DestructibleChunkDesc inline functions

PX_INLINE DestructibleChunkDesc::DestructibleChunkDesc()
{
	setToDefault();
}

PX_INLINE void DestructibleChunkDesc::setToDefault()
{
	ApexDesc::setToDefault();
	isSupportChunk = false;
	doNotFracture = false;
	doNotDamage = false;
	doNotCrumble = false;
#if APEX_RUNTIME_FRACTURE
	runtimeFracture = false;
#endif
	useInstancedRendering = false;
	instancePositionOffset = PxVec3(0.0f);
	instanceUVOffset = PxVec2(0.0f);
	meshIndex = 0xFFFF;
	parentIndex = -1;
	surfaceNormal = PxVec3(0.0f);
	behaviorGroupIndex = -1;
	scatterMeshCount = 0;
	scatterMeshIndices = NULL;
	scatterMeshTransforms = NULL;
}

PX_INLINE bool DestructibleChunkDesc::isValid() const
{
	if (meshIndex == 0xFFFF)
	{
		return false;
	}

	return ApexDesc::isValid();
}


/**
	Destructible authoring structure.

	Descriptor to build one chunk in a fracture hierarchy.
*/
class DestructibleGeometryDesc : public ApexDesc
{
public:
	/**
	\brief constructor sets to default.
	*/
	PX_INLINE		DestructibleGeometryDesc();

	/**
	\brief (re)sets the structure to the default.
	*/
	PX_INLINE void	setToDefault();

	/**
		Returns true iff an object can be created using this descriptor.
	*/
	PX_INLINE bool	isValid() const;

	/**
		The convex hulls associated with this chunk.  These may be obtained from ExplicitHierarchicalMesh::convexHulls()
		if authoring using an ExplicitHierarchicalMesh.  The length of the array is given by convexHullCount.
	*/
	const nvidia::ExplicitHierarchicalMesh::ConvexHull**	convexHulls;

	/**
		The length of the convexHulls array.  If this is positive, then convexHulls must point to a valid array of this size.
		If this is zero, then collisionVolumeDesc must not be NULL, and convex hulls will be automatically created for this
		geometry using collisionVolumeDesc.
	*/
	uint32_t												convexHullCount;

	/**
		If convexHullCount = 0, then collisionVolumeDesc must not be NULL.  In this case convex hulls will automatically be
		created for this geometry.  See CollisionVolumeDesc.
	*/
	const CollisionVolumeDesc*								collisionVolumeDesc;
};

// DestructibleGeometryDesc inline functions

PX_INLINE DestructibleGeometryDesc::DestructibleGeometryDesc()
{
	setToDefault();
}

PX_INLINE void DestructibleGeometryDesc::setToDefault()
{
	ApexDesc::setToDefault();
	convexHulls = NULL;
	convexHullCount = 0;
	collisionVolumeDesc = NULL;
}

PX_INLINE bool DestructibleGeometryDesc::isValid() const
{
	if (convexHullCount == 0 && collisionVolumeDesc == NULL)
	{
		return false;
	}

	if (convexHullCount > 0 && convexHulls == NULL)
	{
		return false;
	}

	return ApexDesc::isValid();
}


/**
	Destructible authoring structure.

	Descriptor for the cookChunk() method of DestructibleAssetAuthoring
*/
class DestructibleAssetCookingDesc : public ApexDesc
{
public:
	/**
	\brief constructor sets to default.
	*/
	PX_INLINE		DestructibleAssetCookingDesc();

	/**
	\brief (re)sets the structure to the default.
	*/
	PX_INLINE void	setToDefault();

	/**
		Returns true iff an object can be created using this descriptor.
	*/
	PX_INLINE bool	isValid() const;

	/**
		Beginning of array of descriptors, one for each chunk.
	*/
	DestructibleChunkDesc*			chunkDescs;

	/**
		The size of the chunkDescs array.  This must be positive.
	*/
	uint32_t						chunkDescCount;

	/**
		Default behavior group (corresponds to index of -1).
	*/
	DestructibleBehaviorGroupDesc	defaultBehaviorGroupDesc;

	/**
		Beginning of array of descriptors, one for each behavior group.
	*/
	DestructibleBehaviorGroupDesc*	behaviorGroupDescs;

	/**
		The size of the behaviorGroupDescs array.  This must be positive.
	*/
	uint32_t						behaviorGroupDescCount;

	/**
		RT fracture behavior group
	*/
	int8_t							RTFractureBehaviorGroup;

	/**
		Beginning of array of descriptors, one for each mesh part.
	*/
	DestructibleGeometryDesc*		geometryDescs;

	/**
		The size of the geometryDescs array.  This must be positive.
	*/
	uint32_t						geometryDescCount;

	/**
		Index pairs that represent chunk connections in the support graph.
		The indices refer to the chunkDescs array. Only sibling chunks,
		i.e. chunks at equal depth may be connected.
	*/
	nvidia::IntPair*				supportGraphEdges;

	/**
		Number of index pairs in supportGraphEdges.
	*/
	uint32_t						supportGraphEdgeCount;
};

// DestructibleAssetCookingDesc inline functions

PX_INLINE DestructibleAssetCookingDesc::DestructibleAssetCookingDesc()
{
	setToDefault();
}

PX_INLINE void DestructibleAssetCookingDesc::setToDefault()
{
	ApexDesc::setToDefault();
	chunkDescs = NULL;
	chunkDescCount = 0;
	geometryDescs = NULL;
	geometryDescCount = 0;
	behaviorGroupDescs = 0;
	behaviorGroupDescCount = 0;
	supportGraphEdges = 0;
	supportGraphEdgeCount = 0;
}

PX_INLINE bool DestructibleAssetCookingDesc::isValid() const
{
	if (chunkDescCount == 0 || chunkDescs == NULL)
	{
		return false;
	}

	for (uint32_t i = 0; i < chunkDescCount; ++i )
	{
		if (!chunkDescs[i].isValid())
		{
			return false;
		}
	}

	if (chunkDescCount >= 65535)
	{
		return false;
	}

	if (geometryDescCount == 0 || geometryDescs == NULL)
	{
		return false;
	}

	for (uint32_t i = 0; i < geometryDescCount; ++i )
	{
		if (!geometryDescs[i].isValid())
		{
			return false;
		}
	}

	if (behaviorGroupDescCount > 127)
	{
		return false;
	}

	for (uint32_t i = 0; i < behaviorGroupDescCount; ++i )
	{
		if (!behaviorGroupDescs[i].isValid())
		{
			return false;
		}
	}

	return ApexDesc::isValid();
}


/**
	Stats for an DestructibleAsset: memory usage, counts, etc.
*/
struct DestructibleAssetStats
{
	uint32_t			totalBytes; ///< total bytes
	uint32_t			chunkCount; ///< chunk count
	uint32_t			chunkBytes; ///< chunk bytes
	uint32_t			chunkHullDataBytes; ///< chunk hull data bytes
	uint32_t			collisionCookedHullDataBytes; ///< collision cooked hull data bytes
	uint32_t			collisionMeshCount; ///< collision mesh count
	uint32_t			maxHullVertexCount; ///< max hull vertex count
	uint32_t			maxHullFaceCount; ///< max hull face count
	uint32_t			chunkWithMaxEdgeCount; ///< chunk with max edge count
	uint32_t			runtimeCookedConvexCount; ///< runtime cooked convex count
	RenderMeshAssetStats	renderMeshAssetStats; ///< render mesh asset stats
};

/**
	Authoring API for a destructible asset.
*/
class DestructibleAssetAuthoring : public AssetAuthoring
{
public:

	/** Fracturing API */

	/**
		DestructibleAssetAuthoring contains an instantiation of ExplicitHierarchicalMesh.
		This function gives access to it.  See ExplicitHierarchicalMesh for details, it is
		the object used by the fracturing tool set for mesh fracturing operations and is used
		to generate the embedded RenderMesh as well as collision and hierarchy data
		for the destructible asset.
	*/
	virtual ExplicitHierarchicalMesh&	getExplicitHierarchicalMesh() = 0;

	/**
		DestructibleAssetAuthoring contains a second instantiation of ExplicitHierarchicalMesh
		used to describe the core mesh for slice fracturing (see FractureTools::FractureSliceDesc),
		done in createHierarchicallySplitMesh().  This function gives access to it.
	*/
	virtual ExplicitHierarchicalMesh&	getCoreExplicitHierarchicalMesh() = 0;

	/**
		DestructibleAssetAuthoring contains an instantiation of CutoutSet used to describe the
		cutout fracturing shapes (see FractureTools::CutoutSet), done in createChippedMesh().
		This function gives access to it.
	*/
	virtual FractureTools::CutoutSet&	getCutoutSet() = 0;

	/**
		Partitions (and possibly re-orders) the mesh array if the triangles form disjoint islands.

		\param mesh						pointer to array of ExplicitRenderTriangles which make up the mesh
		\param meshTriangleCount		the size of the meshTriangles array
		\param meshPartition			user-allocated array for mesh partition, will be filled with the end elements of contiguous subsets of meshTriangles.
		\param meshPartitionMaxCount	size of user-allocated meshPartitionArray
		\param padding					relative value multiplied by the mesh bounding box. padding gets added to the triangle bounds when calculating triangle neighbors.

		\return							Returns the number of partitions.  The value may be larger than meshPartitionMaxCount.  In that case, the partitions beyond meshPartitionMaxCount are not recorded.
	*/
	virtual uint32_t					partitionMeshByIslands
	(
		nvidia::ExplicitRenderTriangle* mesh,
		uint32_t meshTriangleCount,
	    uint32_t* meshPartition,
	    uint32_t meshPartitionMaxCount,
		float padding = 0.0001f
	) = 0;

	/**
		Builds a new ExplicitHierarchicalMesh from an array of triangles, used as the starting
		point for fracturing.  It will contain only one chunk, at depth 0.

		\param meshTriangles		pointer to array of ExplicitRenderTriangles which make up the mesh
		\param meshTriangleCount	the size of the meshTriangles array
		\param submeshData			pointer to array of ExplicitSubmeshData, describing the submeshes
		\param submeshCount			the size of the submeshData array
		\param meshPartition		if not NULL, an array of size meshPartitionCount, giving the end elements of contiguous subsets of meshTriangles.
										If meshPartition is NULL, one partition is assumed.
										When there is one partition, these triangles become the level 0 part.
										When there is more than one partition, the behavior is determined by firstPartitionIsDepthZero (see below).
		\param meshPartitionCount	if meshPartition is not NULL, this is the size of the meshPartition array.
		\param parentIndices		if not NULL, the parent indices for each chunk (corresponding to a partition in the mesh partition).
		\param parentIndexCount		the size of the parentIndices array.  This does not need to match meshPartitionCount.  If a mesh partition has an index beyond the end of parentIndices,
										then the parentIndex is considered to be 0.  Therefore, if parentIndexCount = 0, all parents are 0 and so all chunks created will be depth 1.  This will cause a
										depth 0 chunk to be created that is the aggregate of the depth 1 chunks.  If parentIndexCount > 0, then the depth-0 chunk must have a parentIndex of -1.
										To reproduce the effect of the old parameter 'firstPartitionIsDepthZero' = true, set parentIndices to the address of a int32_t containing the value -1, and set parentIndexCount = 1.
										To reproduce the effect of the old parameter 'firstPartitionIsDepthZero' = false, set parentIndexCount = 0.
										Note: if parent indices are given, the first one must be -1, and *only* that index may be negative.  That is, there may be only one depth-0 mesh and it must be the first mesh.
	*/
	virtual bool						setRootMesh
	(
	    const ExplicitRenderTriangle* meshTriangles,
	    uint32_t meshTriangleCount,
	    const ExplicitSubmeshData* submeshData,
	    uint32_t submeshCount,
	    uint32_t* meshPartition = NULL,
	    uint32_t meshPartitionCount = 0,
		int32_t* parentIndices = NULL,
		uint32_t parentIndexCount = 0
	) = 0;

	/** 
		Builds the root ExplicitHierarchicalMesh from an RenderMeshAsset.
		Since an DestructibleAsset contains no hierarchy information, the input mesh must have only one part.

		\param renderMeshAsset		the asset to import
		\param maxRootDepth			cap the root depth at this value.  Re-fracturing of the mesh will occur at this depth.  Default = UINT32_MAX
	*/
	virtual bool						importRenderMeshAssetToRootMesh(const nvidia::RenderMeshAsset& renderMeshAsset, uint32_t maxRootDepth = UINT32_MAX) = 0;

	/** 
		Builds the root ExplicitHierarchicalMesh from an DestructibleAsset.
		Since an DestructibleAsset contains hierarchy information, the explicit mesh formed
		will have this hierarchy structure.

		\param destructibleAsset	the asset to import
		\param maxRootDepth			cap the root depth at this value.  Re-fracturing of the mesh will occur at this depth.  Default = UINT32_MAX
	*/
	virtual bool						importDestructibleAssetToRootMesh(const nvidia::DestructibleAsset& destructibleAsset, uint32_t maxRootDepth = UINT32_MAX) = 0;

	/**
		Builds a new ExplicitHierarchicalMesh from an array of triangles, used as the core mesh
		for slice fracture operations (see FractureTools::FractureSliceDesc, passed into
		createHierarchicallySplitMesh).

		\param mesh					pointer to array of ExplicitRenderTriangles which make up the mesh
		\param meshTriangleCount	the size of the meshTriangles array
		\param submeshData			pointer to array of ExplicitSubmeshData, describing the submeshes
		\param submeshCount			the size of the submeshData array
		\param meshPartition		meshPartition array
		\param meshPartitionCount	meshPartition array size
	*/
	virtual bool						setCoreMesh
	(
	    const ExplicitRenderTriangle* mesh,
	    uint32_t meshTriangleCount,
	    const ExplicitSubmeshData* submeshData,
	    uint32_t submeshCount,
	    uint32_t* meshPartition = NULL,
	    uint32_t meshPartitionCount = 0
	) = 0;

	/**
		Builds a new ExplicitHierarchicalMesh from an array of triangles, externally provided by the user.
		Note: setRootMesh and setCoreMesh may be implemented as follows:
			setRootMesh(x) <-> buildExplicitHierarchicalMesh( getExplicitHierarchicalMesh(), x)
			setCoreMesh(x) <-> buildExplicitHierarchicalMesh( getCoreExplicitHierarchicalMesh(), x)

		\param hMesh				new ExplicitHierarchicalMesh
		\param meshTriangles		pointer to array of ExplicitRenderTriangles which make up the mesh
		\param meshTriangleCount	the size of the meshTriangles array
		\param submeshData			pointer to array of ExplicitSubmeshData, describing the submeshes
		\param submeshCount			the size of the submeshData array
		\param meshPartition		if not NULL, an array of size meshPartitionCount, giving the end elements of contiguous subsets of meshTriangles.
										If meshPartition is NULL, one partition is assumed.
										When there is one partition, these triangles become the level 0 part.
										When there is more than one partition, these triangles become level 1 parts, while the union of the parts will be the level 0 part.
		\param meshPartitionCount	if meshPartition is not NULL, this is the size of the meshPartition array.
		\param parentIndices		if not NULL, the parent indices for each chunk (corresponding to a partition in the mesh partition).
		\param parentIndexCount		the size of the parentIndices array.  This does not need to match meshPartitionCount.  If a mesh partition has an index beyond the end of parentIndices,
										then the parentIndex is considered to be 0.  Therefore, if parentIndexCount = 0, all parents are 0 and so all chunks created will be depth 1.  This will cause a
										depth 0 chunk to be created that is the aggregate of the depth 1 chunks.  If parentIndexCount > 0, then the depth-0 chunk must have a parentIndex of -1.
										To reproduce the effect of the old parameter 'firstPartitionIsDepthZero' = true, set parentIndices to the address of a int32_t containing the value -1, and set parentIndexCount = 1.
										To reproduce the effect of the old parameter 'firstPartitionIsDepthZero' = false, set parentIndexCount = 0.
										Note: if parent indices are given, the first one must be -1, and *only* that index may be negative.  That is, there may be only one depth-0 mesh and it must be the first mesh.
	*/
	virtual bool						buildExplicitHierarchicalMesh
	(
		ExplicitHierarchicalMesh& hMesh,
		const ExplicitRenderTriangle* meshTriangles,
		uint32_t meshTriangleCount,
		const ExplicitSubmeshData* submeshData,
		uint32_t submeshCount,
		uint32_t* meshPartition = NULL,
		uint32_t meshPartitionCount = 0,
		int32_t* parentIndices = NULL,
		uint32_t parentIndexCount = 0
	) = 0;

	/**
		Splits the chunk in chunk[0], forming a hierarchy of fractured chunks in chunks[1...] using
		slice-mode fracturing.

		\param meshProcessingParams			describes generic mesh processing directives
		\param desc							describes the slicing surfaces (see FractureSliceDesc)
		\param collisionDesc				convex hulls will be generated for each chunk using the method  See CollisionDesc.
		\param exportCoreMesh				if true, the core mesh will be included (at depth 1) in the hierarchically split mesh.  Otherwise, it will only be used to create a hollow space.
		\param coreMeshImprintSubmeshIndex	if this is < 0, use the core mesh materials (was applyCoreMeshMaterialToNeighborChunks).  Otherwise, use the given submesh
		\param randomSeed					seed for the random number generator, to ensure reproducibility.
		\param progressListener				The user must instantiate an IProgressListener, so that this function may report progress of this operation
		\param cancel						if not NULL and *cancel is set to true, the root mesh will be restored to its original state, and the function will return at its earliest opportunity.  Meant to be set from another thread.

		\return returns true if successful.
	*/
	virtual bool							createHierarchicallySplitMesh
	(
	    const FractureTools::MeshProcessingParameters& meshProcessingParams,
	    const FractureTools::FractureSliceDesc& desc,
	    const CollisionDesc& collisionDesc,
	    bool exportCoreMesh,
		int32_t coreMeshImprintSubmeshIndex,
	    uint32_t randomSeed,
	    IProgressListener& progressListener,
	    volatile bool* cancel = NULL
	) = 0;

	/**
		Splits the mesh in chunk[0], forming a hierarchy of fractured meshes in chunks[1...] using
		cutout-mode (chippable) fracturing.

		\param meshProcessingParams		describes generic mesh processing directives
		\param desc						describes the slicing surfaces (see FractureCutoutDesc)
		\param cutoutSet				the cutout set to use for fracturing (see CutoutSet)
		\param sliceDesc				used if desc.chunkFracturingMethod = SliceFractureCutoutChunks
		\param voronoiDesc				used if desc.chunkFracturingMethod = VoronoiFractureCutoutChunks
		\param collisionDesc			convex hulls will be generated for each chunk using the method  See CollisionDesc.
		\param randomSeed				seed for the random number generator, to ensure reproducibility.
		\param progressListener			The user must instantiate an IProgressListener, so that this function may report progress of this operation
		\param cancel					if not NULL and *cancel is set to true, the root mesh will be restored to its original state, and the function will return at its earliest opportunity.  Meant to be set from another thread.

		\return							returns true if successful.
	*/
	virtual bool							createChippedMesh
	(
	    const FractureTools::MeshProcessingParameters& meshProcessingParams,
	    const FractureTools::FractureCutoutDesc& desc,
	    const FractureTools::CutoutSet& cutoutSet,
	    const FractureTools::FractureSliceDesc& sliceDesc,
		const FractureTools::FractureVoronoiDesc& voronoiDesc,
	    const CollisionDesc& collisionDesc,
	    uint32_t randomSeed,
	    IProgressListener& progressListener,
	    volatile bool* cancel = NULL
	) = 0;

	/**
		Builds an internal cutout set.

		\param pixelBuffer		pointer to be beginning of the pixel buffer
		\param bufferWidth		the width of the buffer in pixels
		\param bufferHeight		the height of the buffer in pixels
		\param snapThreshold	the pixel distance at which neighboring cutout vertices and segments may be fudged into alignment.
		\param periodic			whether or not to use periodic boundary conditions when creating cutouts from the map
	*/
	virtual void							buildCutoutSet
	(
	    const uint8_t* pixelBuffer,
	    uint32_t bufferWidth,
	    uint32_t bufferHeight,
	    float snapThreshold,
		bool periodic
	) = 0;

	/**
		Calculate the mapping between a cutout fracture map and a given triangle.
		The result is a 3 by 3 matrix M composed by an affine transformation and a rotation, we can get the 3-D projection for a texture coordinate pair (u,v) with such a formula:
		(x,y,z) = M*PxVec3(u,v,1)

		\param mapping				resulted mapping, composed by an affine transformation and a rotation
		\param triangle				triangle
	**/
	virtual bool							calculateCutoutUVMapping
	(
		PxMat33& mapping,
		const nvidia::ExplicitRenderTriangle& triangle
	) = 0;

	/**
		Uses the passed-in target direction to find the best triangle in the root mesh with normal near the given targetDirection.  If triangles exist
		with normals within one degree of the given target direction, then one with the greatest area of such triangles is used.  Otherwise, the triangle
		with normal closest to the given target direction is used.  The resulting triangle is used to calculate a UV mapping as in the function
		calculateCutoutUVMapping (above).

		The assumption is that there exists a single mapping for all triangles on a specified face, for this feature to be useful. 

		\param mapping	resulted mapping, composed by an affine transformation and a rotation
		\param			targetDirection: the target face's normal
	**/
	virtual bool							calculateCutoutUVMapping
	(
		PxMat33& mapping,
		const PxVec3& targetDirection
	) = 0;

	/**
		Splits the mesh in chunk[0], forming fractured pieces chunks[1...] using
		Voronoi decomposition fracturing.

		\param meshProcessingParams			describes generic mesh processing directives
		\param desc							describes the voronoi splitting parameters surfaces (see FractureVoronoiDesc)
		\param collisionDesc				convex hulls will be generated for each chunk using the method  See CollisionDesc.
		\param exportCoreMesh				if true, the core mesh will be included (at depth 1) in the split mesh.  Otherwise, it will only be used to create a hollow space.
		\param coreMeshImprintSubmeshIndex	if this is < 0, use the core mesh materials (was applyCoreMeshMaterialToNeighborChunks).  Otherwise, use the given submesh
		\param randomSeed					seed for the random number generator, to ensure reproducibility.
		\param progressListener				The user must instantiate an IProgressListener, so that this function may report progress of this operation
		\param cancel						if not NULL and *cancel is set to true, the root mesh will be restored to its original state, and the function will return at its earliest opportunity.  Meant to be set from another thread.

		\return								returns true if successful.
	*/
	virtual bool							createVoronoiSplitMesh
	(
		const FractureTools::MeshProcessingParameters& meshProcessingParams,
		const FractureTools::FractureVoronoiDesc& desc,
		const CollisionDesc& collisionDesc,
		bool exportCoreMesh,
		int32_t coreMeshImprintSubmeshIndex,
		uint32_t randomSeed,
		IProgressListener& progressListener,
		volatile bool* cancel = NULL
	) = 0;

	/**
		Generates a set of uniformly distributed points in the interior of the root mesh.

		\param siteBuffer			An array of PxVec3, at least the size of siteCount.
		\param siteChunkIndices		If not NULL, it must be at least the size of siteCount.
										siteCount indices will be written to this buffer,
										associating each site with a chunk that contains it.
		\param siteCount			The number of points to write into siteBuffer.
		\param randomSeed			Pointer to a seed for the random number generator, to ensure reproducibility.
										If NULL, the random number generator will not be re-seeded.
		\param microgridSize		Pointer to a grid size used for BSP creation. If NULL, the default settings will be used.
		\param meshMode				Open mesh handling.  Modes: Automatic, Closed, Open (see BSPOpenMode)
		\param progressListener		The user must instantiate an IProgressListener, so that this function may report progress of this operation
		\param chunkIndex			If this is a valid index, the voronoi sites will only be created within the volume of the indexed chunk.  Otherwise,
										the sites will be created within each of the root-level chunks.  Default value is an invalid index.

		\return						Returns the number of sites actually created (written to siteBuffer and siteChunkIndices).
		This may be less than the number of sites requested if site placement fails.
		*/
	virtual uint32_t						createVoronoiSitesInsideMesh
	(
		PxVec3* siteBuffer,
		uint32_t* siteChunkIndices,
		uint32_t siteCount,
		uint32_t* randomSeed,
		uint32_t* microgridSize,
		BSPOpenMode::Enum meshMode,
		IProgressListener& progressListener,
		uint32_t chunkIndex = 0xFFFFFFFF
	) = 0;

	/**
		Creates scatter mesh sites randomly distributed on the mesh.

		\param meshIndices							user-allocated array of size scatterMeshInstancesBufferSize which will be filled in by this function, giving the scatter mesh index used
		\param relativeTransforms					user-allocated array of size scatterMeshInstancesBufferSize which will be filled in by this function, giving the chunk-relative transform for each chunk instance
		\param chunkMeshStarts						user-allocated array which will be filled in with offsets into the meshIndices and relativeTransforms array.
														For a chunk indexed by i, the corresponding range [chunkMeshStart[i], chunkMeshStart[i+1]-1] in meshIndices and relativeTransforms is used.
														*NOTE*: chunkMeshStart array must be of at least size N+1, where N is the number of chunks in the base explicit hierarchical mesh.
		\param scatterMeshInstancesBufferSize		the size of meshIndices and relativeTransforms array.
		\param targetChunkCount						how many chunks are in the array targetChunkIndices
		\param targetChunkIndices					an array of chunk indices which are candidates for scatter meshes.  The elements in the array chunkIndices will come from this array
		\param randomSeed							pointer to a seed for the random number generator, to ensure reproducibility.  If NULL, the random number generator will not be re-seeded.
		\param scatterMeshAssetCount				the number of different scatter meshes (not instances).  Should not exceed 255.  If scatterMeshAssetCount > 255, only the first 255 will be used.
		\param scatterMeshAssets					an array of size scatterMeshAssetCount, of the render mesh assets which will be used for the scatter meshes
		\param minCount								an array of size scatterMeshAssetCount, giving the minimum number of instances to place for each mesh
		\param maxCount								an array of size scatterMeshAssetCount, giving the maximum number of instances to place for each mesh
		\param minScales							an array of size scatterMeshAssetCount, giving the minimum scale to apply to each scatter mesh
		\param maxScales							an array of size scatterMeshAssetCount, giving the maximum scale to apply to each scatter mesh
		\param maxAngles							an array of size scatterMeshAssetCount, giving a maximum deviation angle (in degrees) from the surface normal to apply to each scatter mesh

		\return										return value: the number of instances placed in indices and relativeTransforms (will not exceed scatterMeshInstancesBufferSize)
	*/
	virtual uint32_t					createScatterMeshSites
	(
		uint8_t*						meshIndices,
		PxMat44*						relativeTransforms,
		uint32_t*						chunkMeshStarts,
		uint32_t						scatterMeshInstancesBufferSize,
		uint32_t						targetChunkCount,
		const uint16_t*					targetChunkIndices,
		uint32_t*						randomSeed,
		uint32_t						scatterMeshAssetCount,
		nvidia::RenderMeshAsset**			scatterMeshAssets,
		const uint32_t*					minCount,
		const uint32_t*					maxCount,
		const float*					minScales,
		const float*					maxScales,
		const float*					maxAngles
	) = 0;

	/**
		Utility to visualize Voronoi cells for a given set of sites.

		\param debugRender			rendering object which will receive the drawing primitives associated with this cell visualization
		\param sites				an array of Voronoi cell sites, of length siteCount
		\param siteCount			the number of Voronoi cell sites (length of sites array)
		\param cellColors			an optional array of colors (see RenderDebug for format) for the cells.  If NULL, the white (0xFFFFFFFF) color will be used.
										If not NULL, this (of length cellColorCount) is used to color the cell graphics.  The number cellColorCount need not match siteCount.  If
										cellColorCount is less than siteCount, the cell colors will cycle.  That is, site N gets cellColor[N%cellColorCount].
		\param cellColorCount		the number of cell colors (the length of cellColors array)
		\param bounds				defines an axis-aligned bounding box which clips the visualization, since some cells extend to infinity
		\param cellIndex			if this is a valid index (cellIndex < siteCount), then only the cell corresponding to sites[cellIndex] will be drawn.  Otherwise, all cells will be drawn.
	*/
	virtual void							visualizeVoronoiCells
	(
		nvidia::RenderDebugInterface& debugRender,
		const PxVec3* sites,
		uint32_t siteCount,
		const uint32_t* cellColors,
		uint32_t cellColorCount,
		const PxBounds3& bounds,
		uint32_t cellIndex = 0xFFFFFFFF
	) = 0;

	/**
		Splits the chunk in chunk[chunkIndex], forming a hierarchy of fractured chunks using
		slice-mode fracturing.  The chunks will be rearranged so that they are in breadth-first order.

		\param chunkIndex				index of chunk to be split
		\param meshProcessingParams		describes generic mesh processing directives
		\param desc						describes the slicing surfaces (see FractureSliceDesc)
		\param collisionDesc			convex hulls will be generated for each chunk using the method.  See CollisionDesc.
		\param randomSeed				pointer to a seed for the random number generator, to ensure reproducibility.  If NULL, the random number generator will not be re-seeded.
		\param progressListener			The user must instantiate an IProgressListener, so that this function may report progress of this operation
		\param cancel					if not NULL and *cancel is set to true, the root mesh will be restored to its original state, and the function will return at its earliest opportunity.  Meant to be set from another thread.

		\return							returns true if successful.
	*/
	virtual bool							hierarchicallySplitChunk
	(
		uint32_t chunkIndex,
	    const FractureTools::MeshProcessingParameters& meshProcessingParams,
	    const FractureTools::FractureSliceDesc& desc,
	    const CollisionDesc& collisionDesc,
	    uint32_t* randomSeed,
	    IProgressListener& progressListener,
	    volatile bool* cancel = NULL
	) = 0;

	/**
		Splits the chunk in chunk[chunkIndex], forming fractured chunks using
		Voronoi decomposition fracturing.  The chunks will be rearranged so that they are in breadth-first order.

		\param chunkIndex				index of chunk to be split
		\param meshProcessingParams		describes generic mesh processing directives
		\param desc						describes the voronoi splitting parameters surfaces (see FractureVoronoiDesc)
		\param collisionDesc			convex hulls will be generated for each chunk using the method.  See CollisionDesc.
		\param randomSeed				pointer to a seed for the random number generator, to ensure reproducibility.  If NULL, the random number generator will not be re-seeded.
		\param progressListener			The user must instantiate an IProgressListener, so that this function may report progress of this operation
		\param cancel					if not NULL and *cancel is set to true, the root mesh will be restored to its original state, and the function will return at its earliest opportunity.  Meant to be set from another thread.

		\return returns true if successful.
	*/
	virtual bool							voronoiSplitChunk
	(
		uint32_t chunkIndex,
	    const FractureTools::MeshProcessingParameters& meshProcessingParams,
	    const FractureTools::FractureVoronoiDesc& desc,
		const CollisionDesc& collisionDesc,
	    uint32_t* randomSeed,
	    IProgressListener& progressListener,
	    volatile bool* cancel = NULL
	) = 0;

	/**
		Set the tolerances used in CSG calculations.

		\param linearTolerance		relative (to mesh size) tolerance used with angularTolerance to determine coplanarity.  Default = 1.0e-4.
		\param angularTolerance		used with linearTolerance to determine coplanarity.  Default = 1.0e-3
		\param baseTolerance		relative (to mesh size) tolerance used for spatial partitioning
		\param clipTolerance		relative (to mesh size) tolerance used when clipping triangles for CSG mesh building operations.  Default = 1.0e-4.
		\param cleaningTolerance	relative (to mesh size) tolerance used when cleaning the out put mesh generated from the toMesh() function.  Default = 1.0e-7.
	*/
	virtual void							setBSPTolerances
	(
		float linearTolerance,
		float angularTolerance,
		float baseTolerance,
		float clipTolerance,
		float cleaningTolerance
	) = 0;

	/**
		Set the parameters used in BSP building operations.

		\param logAreaSigmaThreshold	At each step in the tree building process, the surface with maximum triangle area is compared
											to the other surface triangle areas.  If the maximum area surface is far from the "typical" set of
											surface areas, then that surface is chosen as the next splitting plane.  Otherwise, a random
											test set is chosen and a winner determined based upon the weightings below.
											The value logAreaSigmaThreshold determines how "atypical" the maximum area surface must be to
											be chosen in this manner.
											Default value = 2.0.
		\param testSetSize				Larger values of testSetSize may find better BSP trees, but will take more time to create.
											testSetSize = 0 is treated as infinity (all surfaces will be tested for each branch).
											Default value = 10.
		\param splitWeight				How much to weigh the relative number of triangle splits when searching for a BSP surface.
											Default value = 0.5.
		\param imbalanceWeight			How much to weigh the relative triangle imbalance when searching for a BSP surface.
											Default value = 0.0.
	*/
	virtual void	setBSPBuildParameters
	(
		float logAreaSigmaThreshold,
		uint32_t testSetSize,
		float splitWeight,
		float imbalanceWeight
	) = 0;


	/**
		Instantiates an ExplicitHierarchicalMesh::ConvexHull

		See the ConvexHull API for its functionality.  Can be used to author chunk hulls in the
		cookChunks function.

		Use ConvexHull::release() to delete the object.
	*/
	virtual ExplicitHierarchicalMesh::ConvexHull*	createExplicitHierarchicalMeshConvexHull() = 0;

	/**
		Builds a mesh used for slice fracturing, given the noise parameters and random seed.  This function is mostly intended
		for visualization - to give the user a "typical" slice surface used for fracturing.

		\return Returns the head of an array of ExplicitRenderTriangles, of length given by the return value.
	*/
	virtual uint32_t						buildSliceMesh(const ExplicitRenderTriangle*& mesh, const FractureTools::NoiseParameters& noiseParameters, const PxPlane& slicePlane, uint32_t randomSeed) = 0;

	/**
		Serialization of the data associated with the fracture API.  This includes
		the root mesh, core mesh, and cutout set.
	*/
	virtual void							serializeFractureToolState(PxFileBuf& stream, nvidia::ExplicitHierarchicalMesh::Embedding& embedding) const = 0;
	
	/**
		Deserialization of the data associated with the fracture API.  This includes
		the root mesh, core mesh, and cutout set.
	*/
	virtual	void							deserializeFractureToolState(PxFileBuf& stream, nvidia::ExplicitHierarchicalMesh::Embedding& embedding) = 0;

	/**
		Set current depth for chunk overlaps calculations.
	*/
	virtual void							setChunkOverlapsCacheDepth(int32_t depth = -1) = 0;

	/**
		Gets the RenderMeshAsset associated with this asset.
	*/
	virtual const RenderMeshAsset*			getRenderMeshAsset() const = 0;

	/**
		Set the RenderMeshAsset associated with this asset.
		This is the asset used for non-instanced rendering.
		NULL is a valid argument, and can be used to clear the internal mesh data.

		\return Returns true if successful.
	*/
	virtual bool							setRenderMeshAsset(RenderMeshAsset*) = 0;

	/**
		Set the RenderMeshAssets used for scatter mesh rendering associated with this asset.
		These assets will be rendered using instanced rendering.
		The array cannot contain NULL elements, if an array size greater than zero is specified.
		
		\return Returns true if successful.
	*/
	virtual bool							setScatterMeshAssets(RenderMeshAsset** scatterMeshAssetArray, uint32_t scatterMeshAssetArraySize) = 0;

	/** Retrieve the number of scatter mesh assets */
	virtual uint32_t						getScatterMeshAssetCount() const = 0;

	/** Retrieve the scatter mesh asset array */
	virtual RenderMeshAsset* const *		getScatterMeshAssets() const = 0;

	/**
		Get the number of instanced chunk meshes in this asset.
	*/
	virtual uint32_t						getInstancedChunkMeshCount() const = 0;

	/**
		Set the parameters used for runtime destruction behavior.  See DestructibleParameters.
	*/
	virtual void                            setDestructibleParameters(const DestructibleParameters&) = 0;

	/**
		The DestructibleParameters which describe the default fracturing behavior for instanced
		DestructibleActors.  These may be overridden by calling setDestructibleParameters().
	*/
	virtual DestructibleParameters			getDestructibleParameters() const = 0;

	/**
		Set the parameters used for default destructible initialization.  See DestructibleInitParameters.
	*/
	virtual void                            setDestructibleInitParameters(const DestructibleInitParameters&) = 0;

	/**
		The parameters used for default destructible initialization.  See DestructibleInitParameters.
	*/
	virtual DestructibleInitParameters		getDestructibleInitParameters() const = 0;

	/**
		Set the name of the emitter to use when generating crumble particles.
	*/
	virtual void                            setCrumbleEmitterName(const char*) = 0;

	/**
		Set the name of the emitter to use when generating fracture-line dust particles.
	*/
	virtual void                            setDustEmitterName(const char*) = 0;

	/**
		Set the name of the fracture pattern to use when runtime fracture is enabled.
	*/
	virtual void                            setFracturePatternName(const char*) = 0;

	/**
		Set padding used for chunk neighbor tests.  This padding is relative to the largest diagonal
		of the asset's local bounding box.
		This value must be non-negative.
		Default value = 0.001f.
	*/
	virtual void							setNeighborPadding(float neighborPadding) = 0;

	/**
		Get padding used for chunk neighbor tests.  Set setNeighborPadding().
	*/
	virtual float							getNeighborPadding() const = 0;

	/**
		Once the internal ExplicitHierarchicalMesh is built using the fracture tools functions
		and all emitter names and parameters set, this functions builds the destructible asset.
		Every chunk (corresponding to a part in the ExplicitHierarchicalMesh) must have
		destruction-specific data set through the descriptor passed into this function.  See
		DestructibleAssetCookingDesc.

		\param cookingDesc				cooking setup
		\param cacheOverlaps			whether the chunk overlaps up to chunkOverlapCacheDepth should be cached in this call
		\param chunkIndexMapUser2Apex	optional user provided uint32_t array that will contains the mapping from user chunk indices (referring to chunks in cookingDesc)
											to Apex internal chunk indices (referring to chunk is internal chunk array)
		\param chunkIndexMapApex2User	same as chunkIndexMapUser2Apex, but opposite direction
		\param chunkIndexMapCount		size of the user provided mapping arrays
	*/
	virtual void                            cookChunks(	const DestructibleAssetCookingDesc& cookingDesc, bool cacheOverlaps = true,
														uint32_t* chunkIndexMapUser2Apex = NULL, uint32_t* chunkIndexMapApex2User = NULL, uint32_t chunkIndexMapCount = 0) = 0;

	/**
		The scale factor used to apply an impulse force along the normal of chunk when fractured.  This is used
		in order to "push" the pieces out as they fracture.
	*/
	virtual float							getFractureImpulseScale() const = 0;

	/**
		Large impact force may be reported if rigid bodies are spawned inside one another.  In this case the realative velocity of the two
		objects will be low.  This variable allows the user to set a minimum velocity threshold for impacts to ensure that the objects are
		moving at a min velocity in order for the impact force to be considered.
	*/
	virtual float							getImpactVelocityThreshold() const = 0;

	/**
		The total number of chunks in the cooked asset.
	*/
	virtual uint32_t						getChunkCount() const = 0;

	/**
		The total number of fracture hierarchy depth levels in the cooked asset.
	*/
	virtual uint32_t						getDepthCount() const = 0;

	/**
		Returns the number of children for the given chunk.
		chunkIndex must be less than getChunkCount().  If it is not, this function returns 0.
	*/
	virtual uint32_t						getChunkChildCount(uint32_t chunkIndex) const = 0;

	/**
		Returns the index for the given child of the given chunk.
		chunkIndex must be less than getChunkCount() and childIndex must be less than getChunkChildCount(chunkIndex).
		If either of these conditions is not met, the function returns ModuleDestructibleConst::INVALID_CHUNK_INDEX.
	*/
	virtual int32_t							getChunkChild(uint32_t chunkIndex, uint32_t childIndex) const = 0;

	/**
		If this chunk is instanced within the same asset, then this provides the instancing position offset.
		Otherwise, this function returns (0,0,0).
	*/
	virtual PxVec3							getChunkPositionOffset(uint32_t chunkIndex) const = 0;

	/**
		If this chunk is instanced within the same asset, then this provides the instancing UV offset.
		Otherwise, this function returns (0,0).
	*/
	virtual PxVec2							getChunkUVOffset(uint32_t chunkIndex) const = 0;

	/**
		The render mesh asset part index associated with this chunk.
	*/
	virtual uint32_t						getPartIndex(uint32_t chunkIndex) const = 0;

	/**
		Trim collision geometry to prevent explosive behavior.  maxTrimFraction is the maximum (relative) distance to trim a hull in the direction
		of each trim plane.
		
		\return Returns true iff successful.
	*/
	virtual void							trimCollisionGeometry(const uint32_t* partIndices, uint32_t partIndexCount, float maxTrimFraction = 0.2f) = 0;

	/**
		Returns stats (sizes, counts) for the asset.  See DestructibleAssetStats.
	*/
	virtual void							getStats(DestructibleAssetStats& stats) const = 0;

	/**
		Ensures that the asset has chunk overlap information cached up to the given depth.
		If depth < 0 (as it is by default), the depth will be taken to be the supportDepth
		given in the asset's destructibleParameters.
		It is ok to pass in a depth greater than any chunk depth in the asset.
	*/
	virtual void							cacheChunkOverlapsUpToDepth(int32_t depth = -1) = 0;

	/**
		Clears the chunk overlap cache.

		\param depth			Depth to be cleared. -1 for all depths.
		\param keepCachedFlag	If the flag is set, the depth is considered to be cached even if the overlaps list is empty.
	*/
	virtual void							clearChunkOverlaps(int32_t depth = -1, bool keepCachedFlag = false) = 0;

	/**
		Adds edges to the support graph. Edges must connect chunks of equal depth.
		The indices refer to the reordered chunk array, a mapping is provided in cookChunks.
	*/
	virtual void							addChunkOverlaps(IntPair* supportGraphEdges, uint32_t numSupportGraphEdges) = 0;

	/**
		Removes edges from support graph.
		The indices refer to the reordered chunk array, a mapping is provided in cookChunks.

		\param supportGraphEdges		Integer pairs representing indices to chunks that are linked
		\param numSupportGraphEdges		Number of provided integer pairs.
		\param keepCachedFlagIfEmpty	If the flag is set, the depth is considered to be cached even if the overlaps list is empty.
	*/
	virtual void							removeChunkOverlaps(IntPair* supportGraphEdges, uint32_t numSupportGraphEdges, bool keepCachedFlagIfEmpty) = 0;

	/**
		The size of the array returned by getCachedOverlapsAtDepth(depth) (see below).
		Note: this function will not trigger overlap caching for the given depth.  If no overlaps
		have been calculated for the depth given, this function returns NULL.
	*/
	virtual uint32_t						getCachedOverlapCountAtDepth(uint32_t depth) = 0;

	/**
		Array of integer pairs, indexing chunk pairs which touch at a given depth in the heirarcy.
		The size of the array is given by getCachedOverlapCountAtDepth(depth).
		Note: this function will not trigger overlap caching for the given depth.  If no overlaps
		have been calculated for the depth given, this function returns NULL.
	*/
	virtual const IntPair*					getCachedOverlapsAtDepth(uint32_t depth) = 0;

	/**
	\brief Apply a transformation to destructible asset

	This is a permanent transformation and it changes the object state. Should only be called immediately before serialization
	and without further modifying the object later on.

	\param transformation	This matrix is allowed to contain a translation and a rotation
	\param scale			Apply a uniform scaling as well
	*/
	virtual void							applyTransformation(const PxMat44& transformation, float scale) = 0;

	/**
	\brief Apply an arbitrary affine transformation to destructible asset

	This is a permanent transformation and it changes the object state. Should only be called immediately before serialization
	and without further modifying the object later on.

	\param transformation	This matrix is allowed to contain translation, rotation, scale and skew
	*/
	virtual void							applyTransformation(const PxMat44& transformation) = 0;

	/**
	\brief Set a maximum fracture depth for a given platform string

	Returns true if the supplied maxDepth is lesser than the number of chunk depth levels for this asset
	*/
	virtual bool                            setPlatformMaxDepth(PlatformTag platform, uint32_t maxDepth) = 0;

	/**
	\brief Removes the maximum fracture depth limit for a given platform string

	Returns true if the platform's maximum fracture depth was previously set and now removed
	*/
	virtual bool                            removePlatformMaxDepth(PlatformTag platform) = 0;

	/**
	\brief Returns the size of the actor transform array.  See getActorTransforms() for a description of this data.
	*/
	virtual uint32_t						getActorTransformCount() const = 0;

	/**
	\brief Returns the head of the actor transform array.  This list is a convenience for placing actors in a level from poses authored in a level editor.
	The transforms may contain scaling.
	*/
	virtual const PxMat44*					getActorTransforms() const = 0;

	/**
	\brief Append transforms to the actor transform list.  See getActorTransforms() for a description of this data.

	\param transforms		Head of an array of transforms
	\param transformCount	Size of transforms
	*/
	virtual void							appendActorTransforms(const PxMat44* transforms, uint32_t transformCount) = 0;

	/**
	\brief Clear the actor transform array.  See getActorTransforms() for a description of this data.
	*/
	virtual void							clearActorTransforms() = 0;
};

/**
	Destructible asset API.  Destructible actors are instanced from destructible assets.
*/
class DestructibleAsset : public Asset
{
public:
	/**
		Enum of chunk flags
	*/
	enum ChunkFlags
	{
		ChunkEnvironmentallySupported =		(1 << 0),
		ChunkAndDescendentsDoNotFracture =	(1 << 1),
		ChunkDoesNotFracture =				(1 << 2),
		ChunkDoesNotCrumble =				(1 << 3),
#if APEX_RUNTIME_FRACTURE
		ChunkRuntimeFracture =				(1 << 4),
#endif
		ChunkIsInstanced =					(1 << 16)
	};

	/** Instancing */

	/**
		Instances the DestructibleAsset, creating an DestructibleActor, using the DestructibleActorDesc.
		See DestructibleActor and DestructibleActorDesc.  This asset will own the DestructibleActor,
		so that any DestructibleActor created by it will be released when this asset is released.
		You may also call the DestructibleActor's release() method at any time.
	*/
	virtual void							releaseDestructibleActor(DestructibleActor& actor) = 0;


	/** General */

	/**
		Create a destructible actor representing the destructible asset in a scene. 
		Unlike a call to createApexActor, here the created actor takes explicit ownership of the provided actorParams.
		This can represent either the destructible descriptor or previously serialized destructible state.
		Note: The client should not attempt to use the provided actorParams after calling this method.
	*/
	virtual DestructibleActor*				createDestructibleActorFromDeserializedState(::NvParameterized::Interface* actorParams, Scene& apexScene) = 0;

	/**
		The DestructibleParameters which describe the default fracturing behavior for instanced
		DestructibleActors.
	*/
	virtual DestructibleParameters			getDestructibleParameters() const = 0;

	/**
		The parameters used for default destructible initialization.  See DestructibleInitParameters.
	*/
	virtual DestructibleInitParameters		getDestructibleInitParameters() const = 0;

	/**
		The name of the emitter to use when generating crumble particles.
		Returns NULL if no emitter is configured.
	*/
	virtual const char*						getCrumbleEmitterName() const = 0;

	/**
		The name of the emitter to use when generating fracture-line dust particles.
		Returns NULL if no emitter is configured.
	*/
	virtual const char*						getDustEmitterName() const = 0;

	/**
		The total number of chunks in the asset.
	*/
	virtual uint32_t						getChunkCount() const = 0;

	/**
		The total number of fracture hierarchy depth levels in the asset.
	*/
	virtual uint32_t						getDepthCount() const = 0;

	/**
		Gets the RenderMeshAsset associated with this asset.
	*/
	virtual const RenderMeshAsset*			getRenderMeshAsset() const = 0;

	/**
		Set the RenderMeshAsset associated with this asset.
		This is the asset used for non-instanced rendering.
		NULL is a valid argument, and can be used to clear the internal mesh data.
		Returns true if successful.
	*/
	virtual bool							setRenderMeshAsset(RenderMeshAsset*) = 0;

	/** Retrieve the number of scatter mesh assets */
	virtual uint32_t						getScatterMeshAssetCount() const = 0;

	/** Retrieve the scatter mesh asset array */
	virtual RenderMeshAsset* const *		getScatterMeshAssets() const = 0;

	/**
		Get the number of instanced chunk meshes in this asset.
	*/
	virtual uint32_t						getInstancedChunkMeshCount() const = 0;

	/**
		Returns stats (sizes, counts) for the asset.  See DestructibleAssetStats.
	*/
	virtual void							getStats(DestructibleAssetStats& stats) const = 0;

	/**
		Ensures that the asset has chunk overlap information cached up to the given depth.
		If depth < 0 (as it is by default), the depth will be taken to be the supportDepth
		given in the asset's destructibleParameters.
		It is ok to pass in a depth greater than any chunk depth in the asset.
	*/
	virtual void							cacheChunkOverlapsUpToDepth(int32_t depth = -1) = 0;

	/**
		Clears the chunk overlap cache.
		If depth < 0 (as it is by default), it clears the cache for each depth.

		\param depth			Depth to be cleared. -1 for all depths.
		\param keepCachedFlag	If the flag is set, the depth is considered to be cached even if the overlaps list is empty.
	*/
	virtual void							clearChunkOverlaps(int32_t depth = -1, bool keepCachedFlag = false) = 0;

	/**
		Adds edges to the support graph. Edges must connect chunks of equal depth.
		The indices refer to the reordered chunk array, a mapping is provided in cookChunks.

		\param supportGraphEdges		Integer pairs representing indices to chunks that are linked
		\param numSupportGraphEdges		Number of provided integer pairs.
	*/
	virtual void							addChunkOverlaps(IntPair* supportGraphEdges, uint32_t numSupportGraphEdges) = 0;

	/**
		Removes edges from support graph.
		The indices refer to the reordered chunk array, a mapping is provided in cookChunks.

		\param supportGraphEdges		Integer pairs representing indices to chunks that are linked
		\param numSupportGraphEdges		Number of provided integer pairs.
		\param keepCachedFlagIfEmpty	If the flag is set, the depth is considered to be cached even if the overlaps list is empty.
	*/
	virtual void							removeChunkOverlaps(IntPair* supportGraphEdges, uint32_t numSupportGraphEdges, bool keepCachedFlagIfEmpty) = 0;

	/**
		The size of the array returned by getCachedOverlapsAtDepth(depth) (see below).
		Note: this function will not trigger overlap caching for the given depth.
	*/
	virtual uint32_t						getCachedOverlapCountAtDepth(uint32_t depth) const = 0;

	/**
		Array of integer pairs, indexing chunk pairs which touch at a given depth in the hierarchy.
		The size of the array is given by getCachedOverlapCountAtDepth(depth).
		Note: this function will not trigger overlap caching for the given depth.  If no overlaps
		have been calculated for the depth given, this function returns NULL.
	*/
	virtual const IntPair*					getCachedOverlapsAtDepth(uint32_t depth) const = 0;

	/**
		If this chunk is instanced within the same asset, then this provides the instancing position offset.
		Otherwise, this function returns (0,0,0).
	*/
	virtual PxVec3							getChunkPositionOffset(uint32_t chunkIndex) const = 0;

	/**
		If this chunk is instanced within the same asset, then this provides the instancing UV offset.
		Otherwise, this function returns (0,0).
	*/
	virtual PxVec2							getChunkUVOffset(uint32_t chunkIndex) const = 0;

	/**
		Retrieve flags (see ChunkFlags) for the given chunk.
	*/
	virtual uint32_t						getChunkFlags(uint32_t chunkIndex) const = 0;

	/**
		Accessor to query the depth of a given chunk.
	*/
	virtual uint16_t						getChunkDepth(uint32_t chunkIndex) const = 0;

	/**
		Returns the index of the given chunk's parent.  If the chunk has no parent (is the root of the fracture hierarchy),
		then -1 is returned.
	*/
	virtual int32_t							getChunkParentIndex(uint32_t chunkIndex) const = 0;

	/** 
		Returns the chunk bounds in the asset (local) space.
	*/
	virtual PxBounds3 						getChunkActorLocalBounds(uint32_t chunkIndex) const = 0;

	/**
		The render mesh asset part index associated with this chunk.
	*/
	virtual uint32_t						getPartIndex(uint32_t chunkIndex) const = 0;

	/**
		The number of convex hulls associated with a given part.
	*/
	virtual uint32_t						getPartConvexHullCount(const uint32_t partIndex) const = 0;

	/**
		Returns the head of an array to convex hull NvParamterized::Interface* pointers for a given part.
	*/
	virtual NvParameterized::Interface**	getPartConvexHullArray(const uint32_t partIndex) const = 0;

	/**
	\brief Returns the size of the actor transform array.  See getActorTransforms() for a description of this data.
	*/
	virtual uint32_t						getActorTransformCount() const = 0;

	/**
	\brief Returns the head of the actor transform array.  This list is a convenience for placing actors in a level from poses authored in a level editor.
	The transforms may contain scaling.
	*/
	virtual const PxMat44*					getActorTransforms() const = 0;

	/**
	\brief Apply a transformation to destructible asset

	This is a permanent transformation and it changes the object state. Should only be called immediately before serialization
	and without further modifying the object later on.

	\param transformation	This matrix is allowed to contain a translation and a rotation
	\param scale			Apply a uniform scaling as well
	*/
	virtual void							applyTransformation(const PxMat44& transformation, float scale) = 0;

	/**
	\brief Apply an arbitrary affine transformation to destructible asset

	This is a permanent transformation and it changes the object state. Should only be called immediately before serialization
	and without further modifying the object later on.

	\param transformation	This matrix is allowed to contain translation, rotation, scale and skew
	*/
	virtual void							applyTransformation(const PxMat44& transformation) = 0;

	/**
		Rebuild the collision volumes for the given chunk, using the geometryDesc (see DestructibleGeometryDesc).
		Returns true iff successful.
	*/
	virtual bool							rebuildCollisionGeometry(uint32_t partIndex, const DestructibleGeometryDesc& geometryDesc) = 0;

protected:
	/** Hidden destructor.  Use release(). */
	virtual									~DestructibleAsset() {}
};


#if !PX_PS4
	#pragma warning(pop)
#endif	//!PX_PS4

PX_POP_PACK

}
} // end namespace nvidia

#endif // DESTRUCTIBLE_ASSET_H
