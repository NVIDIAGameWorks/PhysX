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



#ifndef SCENE_H
#define SCENE_H

/*!
\file
\brief classes Scene, SceneStats, SceneDesc
*/

#include "ApexDesc.h"
#include "Renderable.h"
#include "RenderDebugInterface.h"
#include "Context.h"
#include "foundation/PxVec3.h"
#include <ApexDefs.h>

#if PX_PHYSICS_VERSION_MAJOR == 3
#include "PxFiltering.h"
#endif
#include "MirrorScene.h"

namespace NvParameterized
{
class Interface;
}

namespace physx
{
	class PxBaseTask;
	class PxTaskManager;
	class PxCpuDispatcher;
	class PxGpuDispatcher;
}

namespace nvidia
{
namespace apex
{
// Forward declaration for the PhysX3Interface::setContactReportFlags() callback
class DestructibleActor;

PX_PUSH_PACK_DEFAULT

#if PX_PHYSICS_VERSION_MAJOR == 3
/**
\brief Interface used to call setter function for PhysX 3.0
*/
class PhysX3Interface
{
public:
	/**
	\brief Set the current contact report pair flags any time the Destructible module creates a PhysX 3.x shape

	User can set the report pair flags to specified actor
	*/
	virtual void setContactReportFlags(PxShape* shape, physx::PxPairFlags flags, DestructibleActor* actor, uint16_t actorChunkIndex) = 0;

	/**
	\brief Get the current contact report pair flags

	*/
	virtual physx::PxPairFlags getContactReportFlags(const physx::PxShape* shape) const = 0;
};
#endif

/**
\brief Data used to initialize a new Scene
*/
class SceneDesc : public ApexDesc
{
public:

	PX_INLINE SceneDesc() : ApexDesc()
	{
		init();
	}

	PX_INLINE void setToDefault()
	{
		ApexDesc::setToDefault();
		init();
	}


	/** \brief Give this ApexScene an existing PxScene

	The ApexScene will use the same CpuDispatcher and GpuDispatcher that were
	provided to the PxScene when it was created.  There is no need to provide
	these pointers in the ApexScene descriptor.
	*/
	PxScene* scene;

	/** \brief Give this ApexScene an existing RenderDebugInterface
	*/
	RenderDebugInterface* debugInterface;

#if PX_PHYSICS_VERSION_MAJOR == 0

	/**
	\brief Give this ApexScene a user defined CpuDispatcher

	If cpuDispatcher is NULL, the APEX SDK will create and use a default
	thread pool.
	*/
	PxCpuDispatcher* cpuDispatcher;

	/**
	\brief Give this ApexScene a user defined GpuDispatcher

	If gpuDispatcher is NULL, this APEX scene will not use any GPU accelerated
	features.
	*/
	PxGpuDispatcher* gpuDispatcher;

#elif PX_PHYSICS_VERSION_MAJOR == 3

	/** \brief Give this ApexScene a user defined interface to the PhysX 3.*
	*/
	PhysX3Interface* physX3Interface;

#endif

	/**
	\brief Toggle the use of a legacy DebugRenderable for PhysX 2.8.x or PxRenderBuffer for 3.x

	If true, then debug rendering will happen through the legacy DebugRenderable interface.
	If false (the default value) it will render through the UserRenderResources interface.
	*/
	bool	useDebugRenderable;

	/**
	\brief Transmits debug rendering to PVD2 as well
	*/
	bool	debugVisualizeRemotely;

	/**
	\brief If 'debugVisualizeLocally' is true, then debug visualization which is being transmitted remotely will also be shown locally as well.
	*/
	bool	debugVisualizeLocally;

	/**
	\brief Switch for particles (BasicIOS, ParticleIOS, FieldSampler) which defines whether to use CUDA or not.
	*/
	bool 	useCuda;

private:
	PX_INLINE void init()
	{
#if PX_PHYSICS_VERSION_MAJOR == 0
		cpuDispatcher = 0;
		gpuDispatcher = 0;
#elif PX_PHYSICS_VERSION_MAJOR == 3
		physX3Interface	= 0;
#endif
		scene = 0;
		debugInterface = 0;
		useDebugRenderable = false;
		debugVisualizeRemotely = false;
		debugVisualizeLocally = true;
#if APEX_CUDA_SUPPORT
		useCuda = true;
#else
		useCuda = false;
#endif
	}
};


/**
\brief APEX stat struct that contains the type enums
*/
struct StatDataType
{
	/**
	\brief Enum of the APEX stat types
	*/
	enum Enum
	{
		INVALID = 0,
		STRING  = 1,
		INT     = 2,
		FLOAT   = 3,
		ENUM    = 4,
		BOOL    = 5,
	};
};

/**
\brief data value definitions for stats (derived from openautomate)
*/
typedef struct oaValueStruct
{
	union
	{
		char*			String;
		int32_t			Int;
		float			Float;
		char*			Enum;
		bool			Bool;
	};
} StatValue;

/**
\brief data value definitions for stats
*/
typedef struct
{
	///name
	const char*			StatName;

	///type
	StatDataType::Enum	StatType;

	///current value
	StatValue			StatCurrentValue;
} StatsInfo;

/**
\brief Per scene statistics
*/
struct SceneStats
{
	/**\brief The number of ApexStats structures stored.
	*/
	uint32_t			numApexStats;
	/**\brief Array of StatsInfo structures.
	*/
	StatsInfo*			ApexStatsInfoPtr;
};

/**
\brief Types of view matrices handled by APEX

USER_CUSTOMIZED : APEX simply uses the view matrix given. Need to setViewParams()
LOOK_AT_RH: APEX gets the eye direction and eye position based on this kind of matrix.
LOOK_AT_LH: APEX gets the eye direction and eye position based on this kind of matrix.

*/
struct ViewMatrixType
{
	/**
	\brief Enum of the view matrices types
	*/
	enum Enum
	{
		USER_CUSTOMIZED = 0,
		LOOK_AT_RH,
		LOOK_AT_LH,
	};
};

/**
\brief Types of projection matrices handled by APEX

USER_CUSTOMIZED : APEX simply uses the projection matrix given. Need to setProjParams()

*/
struct ProjMatrixType
{
	/**
	\brief Enum of the projection matrices types
	*/
	enum Enum
	{
		USER_CUSTOMIZED = 0,
	};
};

/**
\brief Enum of the bounding box types
*/
struct UserBoundingBoxFlags
{
	/**
	\brief Enum of the bounding box types
	*/
	enum Enum
	{
		NONE    = 1 << 0,
		ENTER   = 1 << 1,
		LEAVE   = 1 << 2
	};
};

/**
\brief An APEX wrapper for an PxScene
*/
class Scene : public Renderable, public Context, public ApexInterface
{
public:

#if PX_PHYSICS_VERSION_MAJOR == 3
	/**
	\brief Associate an PxScene with this Scene.

	All Actors in the Scene will be added to the PxScene.
	The PxScene pointer can be NULL, which will cause all APEX actors to be removed
	from the previously specified PxScene.  This must be done before the PxScene
	can be released.
	*/
	virtual void setPhysXScene(PxScene* s) = 0;

	/**
	\brief Retrieve the PxScene associated with this Scene
	*/
	virtual PxScene* getPhysXScene() const = 0;

#endif

	/**
	\brief Retrieve scene statistics
	*/
	virtual const SceneStats* getStats(void) const = 0;

	/**
	\brief Start simulation of the APEX (and PhysX) scene

	Start simulation of the Actors and the PxScene associated with this Scene.
	No Actors should be added, deleted, or modified until fetchResults() is called.

	Calls to simulate() should pair with calls to fetchResults():
 	Each fetchResults() invocation corresponds to exactly one simulate()
 	invocation; calling simulate() twice without an intervening fetchResults()
 	or fetchResults() twice without an intervening simulate() causes an error
 	condition.
 
	\param[in] elapsedTime Amount of time to advance simulation by. <b>Range:</b> (0,inf)

	\param[in] finalStep should be left as true, unless your application is manually sub stepping APEX
	(and PhysX) and you do not intend to try to render the output of intermediate steps.

	\param[in] completionTask if non-NULL, this task will have its refcount incremented in simulate(), then
	decremented when the scene is ready to have fetchResults called. So the task will not run until the
	application also calls removeReference() after calling simulate.

	\param[in] scratchMemBlock a memory region for physx to use for temporary data during simulation. This block may be reused by the application
	after fetchResults returns. Must be aligned on a 16-byte boundary

	\param[in] scratchMemBlockSize the size of the scratch memory block. Must be a multiple of 16K.
	*/
	virtual void simulate(float elapsedTime, 
						  bool finalStep = true, 
						  PxBaseTask *completionTask = NULL,
						  void* scratchMemBlock = 0, 
						  uint32_t scratchMemBlockSize = 0) = 0;

	/**
	\brief Checks, and optionally blocks, for simulation completion.  Updates scene state.

	Checks if Scene has completed simulating (optionally blocking for completion).  Updates
	new state of Actors and the PxScene.  Returns true if simulation is complete.

	\param block [in] - block until simulation is complete
	\param errorState [out] - error value is written to this address, if not NULL
	*/
	virtual bool fetchResults(bool block, uint32_t* errorState) = 0;

	/**
	\brief Returns an DebugRenderable object that contains the data for debug rendering of this scene
	*/
	virtual const PxRenderBuffer* getRenderBuffer() const = 0;

	/**
	\brief Returns an DebugRenderable object that contains the data for debug rendering of this scene, in screenspace
	*/
	virtual const PxRenderBuffer* getRenderBufferScreenSpace() const = 0;

	/**
	\brief Checks, and optionally blocks, for simulation completion.

	Performs same function as fetchResults(), but does not update scene state.  fetchResults()
	must still be called before the next simulation step can begin.
	*/
	virtual bool checkResults(bool block) const = 0;

	/**
	\brief Allocate a view matrix. Returns a viewID that identifies this view matrix
		   for future calls to setViewMatrix(). The matrix is de-allocated automatically
		   when the scene is released.

	Each call of this function allocates space for one view matrix. Since many features in
	APEX require a projection matrix it is _required_ that the application call this method.
	Max calls restricted to 1 for now.
	If ViewMatrixType is USER_CUSTOMIZED, setViewParams() as well using this viewID.
	If connected to PVD, PVD camera is set up.
	@see ViewMatrixType
	@see setViewParams()
	*/
	virtual uint32_t allocViewMatrix(ViewMatrixType::Enum) = 0;

	/**
	\brief Allocate a projection matrix. Returns a projID that identifies this projection matrix
	       for future calls to setProjMatrix(). The matrix is de-allocated automatically
		   when the scene is released.

	Each call of this function allocates space for one projection matrix.  Since many features in
	APEX require a projection matrix it is _required_ that the application call this method.
	Max calls restricted to 1 for now.
	If ProjMatrixType is USER_CUSTOMIZED, setProjParams() as well using this projID
	@see ProjMatrixType
	@see setProjParams()
	*/
	virtual uint32_t allocProjMatrix(ProjMatrixType::Enum) = 0;

	/**
	\brief Returns the number of view matrices allocated.
	*/
	virtual uint32_t getNumViewMatrices() const = 0;

	/**
	\brief Returns the number of projection matrices allocated.
	*/
	virtual uint32_t getNumProjMatrices() const = 0;

	/**
	\brief Sets the view matrix for the given viewID. Should be called whenever the view matrix needs to be updated.

	If the given viewID's matrix type is identifiable as indicated in ViewMatrixType, eye position and eye direction are set as well, using values from this matrix.
	Otherwise, make a call to setViewParams().
	If connected to PVD, PVD camera is updated.
	*/
	virtual void setViewMatrix(const PxMat44& viewTransform, const uint32_t viewID = 0) = 0;

	/**
	\brief Returns the view matrix set by the user for the given viewID.

	@see setViewMatrix()
	*/
	virtual PxMat44	getViewMatrix(const uint32_t viewID = 0) const = 0;

	/**
	\brief Sets the projection matrix for the given projID. Should be called whenever the projection matrix needs to be updated.

	Make a call to setProjParams().
	@see setProjParams()
	*/
	virtual void setProjMatrix(const PxMat44& projTransform, const uint32_t projID = 0) = 0;

	/**
	\brief Returns the projection matrix set by the user for the given projID.

	@see setProjMatrix()
	*/
	virtual PxMat44	getProjMatrix(const uint32_t projID = 0) const = 0;

	/**
	\brief Sets the use of the view matrix and projection matrix as identified by their IDs. Should be called whenever either matrices needs to be updated.
	*/
	virtual void setUseViewProjMatrix(const uint32_t viewID = 0, const uint32_t projID = 0) = 0;

	/**
	\brief Sets the necessary information for the view matrix as identified by its viewID. Should be called whenever any of the listed parameters needs to be updated.

	@see ViewMatrixType
	*/
	virtual void setViewParams(const PxVec3& eyePosition, const PxVec3& eyeDirection, const PxVec3& worldUpDirection = PxVec3(0, 1, 0), const uint32_t viewID = 0) = 0;

	/**
	\brief Sets the necessary information for the projection matrix as identified by its projID. Should be called whenever any of the listed parameters needs to be updated.

	@see ProjMatrixType
	*/
	virtual void setProjParams(float nearPlaneDistance, float farPlaneDistance, float fieldOfViewDegree, uint32_t viewportWidth, uint32_t viewportHeight, const uint32_t projID = 0) = 0;

	/**
	\brief Returns the world space eye position.

	@see ViewMatrixType
	@see setViewMatrix()
	*/
	virtual PxVec3 getEyePosition(const uint32_t viewID = 0) const = 0;

	/**
	\brief Returns the world space eye direction.

	@see ViewMatrixType
	@see setViewMatrix()
	*/
	virtual PxVec3 getEyeDirection(const uint32_t viewID = 0) const = 0;

	/**
	\brief Returns the APEX scene's task manager
	*/
	virtual PxTaskManager* getTaskManager() const = 0;

	/**
	\brief Toggle the use of a debug renderable
	*/
	virtual void setUseDebugRenderable(bool state) = 0;

	/**
	\brief Gets debug rendering parameters from NvParameterized
	*/
	virtual ::NvParameterized::Interface* getDebugRenderParams() const = 0;

	/**
	\brief Gets module debug rendering parameters from NvParameterized
	*/
	virtual ::NvParameterized::Interface* getModuleDebugRenderParams(const char* name) const = 0;

	/**
	\brief Gets gravity value from PhysX or Apex scene for non PhysX configuration
	*/
	virtual PxVec3 getGravity() const = 0;

	/**
	\brief Sets gravity for PhysX or Apex scene for non PhysX configuration
	*/
	virtual void setGravity(const PxVec3& gravity) = 0;


#if PX_PHYSICS_VERSION_MAJOR == 3
	/**
	\brief Acquire the PhysX scene lock for read access.
	
	The PhysX 3.2.2 SDK (and higher) provides a multiple-reader single writer mutex lock to coordinate	
	access to the PhysX SDK API from multiple concurrent threads.  This method will in turn invoke the
	lockRead call on the PhysX Scene.  The source code fileName and line number a provided for debugging
	purposes.
	*/
	virtual void lockRead(const char *fileName,uint32_t lineo) = 0;

	/**
	\brief Acquire the PhysX scene lock for write access.
	
	The PhysX 3.2.2 SDK (and higher) provides a multiple-reader single writer mutex lock to coordinate	
	access to the PhysX SDK API from multiple concurrent threads.  This method will in turn invoke the
	lockWrite call on the PhysX Scene.  The source code fileName and line number a provided for debugging
	purposes.
	*/
	virtual void lockWrite(const char *fileName,uint32_t lineno) = 0;

	/**
	\brief Release the PhysX scene read lock
	*/
	virtual void unlockRead() = 0;

	/**
	\brief Release the PhysX scene write lock
	*/
	virtual void unlockWrite() = 0;

	/**
	\brief Allows the application to specify a pair of PhysX actors for the purposes of collision filtering.

	The set of methods addActorPair/removeActorPair and findActorPair can be used to implement collision filtering.
	This is a feature typically required to implement ragdoll systems to prevent nearby bodies from generating 
	contacts which create jitter in the simulation; causing artifacts and preventing a ragdoll from coming to rest.
	
	These methods are not required but are helper methods available if needed by the application.

	\param actor0 The first actor in the actor pair to consider
	\param actor1 The second actor in the actor pair to consider for filtering
	*/
	virtual void addActorPair(PxActor *actor0,PxActor *actor1) = 0;

	/**
	\brief Removes a previously specified pair of actors from the actor-pair filter table

	\param actor0 The first actor in the actor pair to remove
	\param actor1 The second actor in the actor pair to remove
	*/
	virtual void removeActorPair(PxActor *actor0,PxActor *actor1) = 0;

	/**
	\brief This method is used to determine if two actor pairs match.

	If actor0 and actor1 were previously added to the actor-pair filter table, then this method will return true.  
	Order is not important, actor0+actor1 will return the same result as actor1+actor0

	\param actor0 The first actor to consider
	\param actor1 The second actor to consider

	\return Returns true if the two actors match false if they have not been previously defined.
	*/
	virtual bool findActorPair(PxActor *actor0,PxActor *actor1) const = 0;


	/**
	\brief Returns an MirrorScene helper class which keeps a primary APEX scene mirrored into a secondary scene.


	\param mirrorScene [in] - The APEX scene that this scene will be mirrored into.
	\param mirrorFilter [in] - The application provided callback interface to filter which actors/shapes get mirrored.
	\param mirrorStaticDistance [in] - The distance to mirror static objects from the primary scene into the mirrored scene.
	\param mirrorDynamicDistance [in] - The distance to mirror dynamic objects from the primary scene into the mirrored scene.
	\param mirrorRefreshDistance [in] - The distance the camera should have moved before revising the trigger shapes controlling which actors get mirrored.
	*/
	virtual MirrorScene *createMirrorScene(nvidia::apex::Scene &mirrorScene,
		MirrorScene::MirrorFilter &mirrorFilter,
		float mirrorStaticDistance,
		float mirrorDynamicDistance,
		float mirrorRefreshDistance) = 0;
#endif


	/**
	\brief Adds user-defined bounding box into the scene. Each module can use these bounding boxes in a module-specific way. See documentation on each module.
	Some modules might use it as a valid volume of simulation, deleting actors or parts of actors upon leaving a BB.


	\param bounds [in] - The bounding box in world coordinates.
	\param flags [in] - The flag for supplied bounding box.
	*/
	virtual void addBoundingBox(const PxBounds3& bounds, UserBoundingBoxFlags::Enum flags) = 0;

	/**
	\brief Returns user-defined bounding box added previously. In case there is no bounding box for the given index, zero sized PxBounds3 is returned.


	\param index [in] - Index of the bounding box. User could acquire total number of bounding boxes using getBoundingBoxCount.
	*/
	virtual const PxBounds3 getBoundingBox(const uint32_t index) const = 0;

	/**
	\brief Returns user-defined bounding box flags. In case there is no bounding box (and its flags) for the given index, UserBoundingBoxFlags::NONE is returned.


	\param index [in] - Index of the bounding box. User could acquire total number of bounding boxes using getBoundingBoxCount.
	*/
	virtual UserBoundingBoxFlags::Enum getBoundingBoxFlags(const uint32_t index) const = 0;

	/**
	\brief Returns user-defined bounding box count.
	*/
	virtual uint32_t getBoundingBoxCount() const = 0;

	/**
	\brief Removes user-defined bounding box at index. In case index is invalid (there is no bounding box for this index) nothing is happenning.

	\param index [in] - Index of the bounding box. User could acquire total number of bounding boxes using getBoundingBoxCount.
	*/
	virtual void  removeBoundingBox(const uint32_t index) = 0;

	/**
	\brief Removed all of the user-specified bounding boxes.
	*/
	virtual void  removeAllBoundingBoxes() = 0;

	/**
	\brief Returns CUDA test manager. For distribution builds return NULL
	*/
	virtual void* getCudaTestManager() const = 0;
	
	/**
	\brief Returns CUDA profile manager. For distribution builds return NULL
	*/
	virtual void* getCudaProfileManager() const = 0;

	/**
	\brief Enables/disables CUDA error check after each kernel launch. 
	
	Use ONLY for DEBUG purposes, when enabled could dramatically slowdown performance!
	*/
	virtual void setCudaKernelCheckEnabled(bool enabled) = 0;
	
	/**
	\brief Returns whether CUDA error check enabled
	*/
	virtual bool getCudaKernelCheckEnabled() const = 0;

#if APEX_UE4
	/**
	\brief Update gravity
	*/
	virtual void updateGravity() = 0;
#endif
};


PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // SCENE_H
