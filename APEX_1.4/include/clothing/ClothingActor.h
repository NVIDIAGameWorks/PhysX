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



#ifndef CLOTHING_ACTOR_H
#define CLOTHING_ACTOR_H

#include "Actor.h"
#include "Renderable.h"
#include "ClothingAsset.h"

namespace NvParameterized
{
class Interface;
}

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

class ClothingVelocityCallback;
class ClothingPlane;
class ClothingConvex;
class ClothingSphere;
class ClothingCapsule;
class ClothingTriangleMesh;
class ClothingRenderProxy;


/**
\brief selects the mode the clothing actor will be in the simulation frame
*/
struct ClothingTeleportMode
{
	/** \brief the enum type */
	enum Enum
	{
		/**
		\brief Simulation continues smoothly. This is the most commonly used mode
		*/
		Continuous,

		/**
		\brief Transforms the current simulation state from the old global pose to the new global pose.

		This will transform positions and velocities and thus keep the simulation state, just translate it to a new pose.
		*/
		Teleport,

		/**
		\brief Forces the cloth to the animated position in the next frame.

		This can be used to reset it from a bad state or by a teleport where the old state is not important anymore.
		*/
		TeleportAndReset,
	};
};



/**
\brief Instance of ClothingAsset. Can be positioned, animated, updated and rendered.
*/
class ClothingActor : public Actor, public Renderable
{
protected:
	virtual ~ClothingActor() {}
public:

	/**
	\brief Returns a reference to the actor descriptor as it is held by the ClothingActor.

	This descriptor can be modified at any time, changes will only be read during the first part of the simulation.
	*/
	virtual ::NvParameterized::Interface* getActorDesc() = 0;

	/**
	\brief Updates all internal bone matrices. This should be called with updated information before apex scenes start simulating.

	\param[in] globalPose				The new location of the actor
	\param[in] newBoneMatrices			Pointer to the array of transformations that contain the composite bone transformations for the
										current frame
	\param[in] boneMatricesByteStride	stride of the bone matrices, must be bigger than sizeof(PxMat44)
	\param[in] numBoneMatrices			number of bone matrices available. This should correspond with the number of bones present in the asset
	\param[in] teleportMode				Setting this to anything but TM_Continuous will force apply a teleport and optionally a reset.

	\note This must be called before Scene::simulate is called
	*/
	virtual void updateState(const PxMat44& globalPose, const PxMat44* newBoneMatrices, uint32_t boneMatricesByteStride, uint32_t numBoneMatrices, ClothingTeleportMode::Enum teleportMode) = 0;

	/**
	\brief Change the max distance of all active vertices with a scalar parameter

	\param[in] scale         Must be in [0,1] range
	\param[in] multipliable  Setting to define how the scale is applied. True will multiply the scale on top of the max distance, False will subtract the maximum max distance.
	*/
	virtual void updateMaxDistanceScale(float scale, bool multipliable) = 0;

	/**
	\brief returns the globalPose that was set with ClothingActor::updateState()
	*/
	virtual const PxMat44& getGlobalPose() const = 0;

	/**
	\brief Sets the wind strength and direction, can be called any time

	\deprecated Use ClothingActor::getActorDesc() and modify the wind part of it

	\param[in] windAdaption The rate of adaption. The higher this value, the faster the cloth reaches the wind velocity. Set to 0 to turn off wind
	\param[in] windVelocity The target velocity each vertex tries to achieve.

	\note It is safe to call this even during simulation, but it will only have an effect after the next call to Scene::simulate()
	*/
	virtual void setWind(float windAdaption, const PxVec3& windVelocity) = 0;

	/**
	\brief \b DEPRECATED Time in seconds how long it takes to go from 0 maxDistance to full maxDistance

	\deprecated Use ClothingActor::getActorDesc() and modify the maxDistanceBlendTime part of it
	<b>Default:</b> 1.0
	\note This also influences how quickly different physical LoDs can be switched
	\note It is safe to call this even during simulation.
	*/
	virtual void setMaxDistanceBlendTime(float blendTime) = 0;

	/**
	\brief \b DEPRECATED Time in seconds how long it takes to go from 0 maxDistance to full maxDistance

	\deprecated Use ClothingActor::getActorDesc() and read the maxDistanceBlendTime part of it
	*/
	virtual float getMaxDistanceBlendTime() const = 0;

	/**
	\brief Tells the actor if it will be rendered or not.
	If an actor is simulated, but not rendered, some computations (skinning, normal and tangent calculation)
	doesn't need to be done.

	disabling is set immediately, disabling is buffered for the next frame.
	*/
	virtual void setVisible(bool enable) = 0;

	/**
	\brief Returns the current visibility setting.
	The most recently set value is returned (i.e. the buffered value, not the actual value).
	*/
	virtual bool isVisible() const = 0;

	/**
	\brief Stops simulating the actor.
	*/
	virtual void setFrozen(bool enable) = 0;

	/**
	\brief Returns if the simulation is currently stopped for this actor.
	*/
	virtual bool isFrozen() const = 0;

	/**
	\brief Returns whether the actor is simulated using the 2.8.x or the 3.x cloth solver.
	*/
	virtual ClothSolverMode::Enum getClothSolverMode() const = 0;

	/**
	\brief sets the graphical Lod
	This chooses the graphical mesh of all the meshes stored in the asset to be rendered. It has to be set
	before the simulate call to take effect for the next time the actor is rendered. Otherwise, the
	given value will be buffered and used as soon as possible.

	\param [in] lod		lod used to render the mesh

	\note It is safe to call this even during simulation
	*/
	virtual void setGraphicalLOD(uint32_t lod) = 0;

	/**
	\brief returns the graphical Lod
	This returns the buffered most recently set graphical lod, even if it's not active yet.
	*/
	virtual uint32_t getGraphicalLod() = 0;

	/**
	\brief Raycasts against the ClothingActor

	\param [in] worldOrigin		The world ray's origin
	\param [in] worldDirection	The world ray's direction, needs not to be normalized
	\param [out] time			Impact time
	\param [out] normal			Impact normal in world space
	\param [out] vertexIndex	Vertex index that was hit

	\return true if this actor is hit
	*/
	virtual bool rayCast(const PxVec3& worldOrigin, const PxVec3& worldDirection, float& time, PxVec3& normal, uint32_t& vertexIndex) = 0;

	/**
	\brief Attach a vertex to a global position
	*/
	virtual void attachVertexToGlobalPosition(uint32_t vertexIndex, const PxVec3& globalPosition) = 0;

	/**
	\brief Free a previously attached vertex
	*/
	virtual void freeVertex(uint32_t vertexIndex) = 0;

	/**
	\brief Returns the actively selected material.
	*/
	virtual uint32_t getClothingMaterial() const = 0;

	/**
	\brief Sets which clothing material is used from the assets library
	*/
	virtual void setClothingMaterial(uint32_t index) = 0;

	/**
	\brief Sets the override material for the submesh with the given index.
	*/
	virtual void setOverrideMaterial(uint32_t submeshIndex, const char* overrideMaterialName) = 0;

	/**
	\brief sets the velocity callback for an individual actor, turned off when NULL
	*/
	virtual void setVelocityCallback(ClothingVelocityCallback* callback) = 0;

	/**
	\brief Returns the current position of all physics vertices.

	This method provides the positions of the currently simulated part of the physics mesh
	and skins the non-simulated part with the current bone positions.

	\note This should usually not be needed other than for rendering the physics mesh yourself
	\note Must be called in between Scene::fetchResults and Scene::simulate.
	*/
	virtual void getPhysicalMeshPositions(void* buffer, uint32_t byteStride) = 0;

	/**
	\brief Returns the current normals of all physics vertices.

	This method provides the normals of the currently simulated part of the physics mesh
	and skins the non-simulated part with the current bone positions.

	\note This should usually not be needed other than for rendering the physics mesh yourself
	\note Must be called in between Scene::fetchResults and Scene::simulate.
	*/
	virtual void getPhysicalMeshNormals(void* buffer, uint32_t byteStride) = 0;

	/**
	\brief Returns how much an Actor will cost at maximum
	*/
	virtual float getMaximumSimulationBudget() const = 0;

	/**
	\brief Returns the number of currently simulation vertices.

	\note	This is not to be used for getPhysicalMeshPositions.
			getNumSimulatedVertices returns the number
			of actually simulated verts, while getPhysicalMeshPositions returns the complete
			physical mesh, regardless of the current physical lod
	*/
	virtual uint32_t getNumSimulationVertices() const = 0;

	
	/**
	\brief Returns a pointer to the internal positions array of the simulated physics mesh
	Blocks until the simulation and post simulation processing is done
	(with asyncFetchResult set to true)

	\return											Pointer the simulation mesh positions
	*/
	virtual const PxVec3* getSimulationPositions() = 0;

	/**
	\brief Returns a pointer to the internal normals array of the simulated physics mesh
	Blocks until the simulation and post simulation processing is done
	(with asyncFetchResult set to true)

	\return											Pointer the simulation mesh normals
	*/
	virtual const PxVec3* getSimulationNormals() = 0;

	/**
	\brief Writes the current velocities of the simulated vertices into the provided array.
	A buffer of getNumSimulationVertices() PxVec3's needs to be provided. The function cannot be called
	during simulation.
	Blocks until the simulation and post simulation processing is done
	(with asyncFetchResult set to true)

	\return											true if a simulation is available and the velocities have been written
	*/
	virtual bool getSimulationVelocities(PxVec3* velocities) = 0;

	/**
	\brief Returns the number of the graphical vertices that need to be skinned to the simulation mesh [0, numGraphicalVerticesActive)
	
	The rest must be regularly skinned to bones [numGraphicalVerticesActive, numVertices)

	\note Only valid after simulate.
	*/
	virtual uint32_t getNumGraphicalVerticesActive(uint32_t submeshIndex) const = 0;

	/**
	\brief Returns the transform that needs to be applied to the rendered mesh.

	\note Only valid after simulate.
	*/
	virtual PxMat44 getRenderGlobalPose() const = 0;

	/**
	\brief Returns the current skinning matrices.
	The skinning matrices already contain the global pose of the actor.

	\note Only valid after simulate.
	*/
	virtual const PxMat44* getCurrentBoneSkinningMatrices() const = 0;


	/**
	\brief return whether GPU solver can use half precision for storing positions.

	\note	If allowed half precision will be activated only when actors cannot be fitted to shared memory with float precision.
	*/
	virtual bool isHalfPrecisionAllowed() const = 0;

	/**
	\brief allow GPU solver to use half precision for storing positions. 
	
	\note	This option will be activated only when actors cannot be fitted to shared memory with float precision.
			Be aware half precision might be insufficient for some cloth simulation. 
			Especially simulation with extensive self collisions sensitive to precision.

	\param [in] isAllowed		true - allow GPU solver to use half precision, false - don't allow
	*/
	virtual void setHalfPrecision(bool isAllowed) = 0;


	/**
	\brief Create a collision plane for the actor.
	*/
	virtual ClothingPlane* createCollisionPlane(const PxPlane& plane) = 0;

	/**
	\brief Create a collision convex for the actor, defined by planes.
	*/
	virtual ClothingConvex* createCollisionConvex(ClothingPlane** planes, uint32_t numPlanes) = 0;

	/**
	\brief Create a collision sphere for the actor.
	*/
	virtual ClothingSphere* createCollisionSphere(const PxVec3& position, float radius) = 0;

	/**
	\brief Create a tapered collision capsule for the actor, defined by two spheres.
	*/
	virtual ClothingCapsule* createCollisionCapsule(ClothingSphere& sphere1, ClothingSphere& sphere2) = 0;

	/**
	\brief Create a collision triangle mesh for the actor.
	*/
	virtual ClothingTriangleMesh* createCollisionTriangleMesh() = 0;

	/**
	\brief Returns the Render Proxy for this clothing actor.

	Call this function after fetchResults to get the ClothingRenderProxy object that can be used
	to render the simulation reset. The render proxy can be acquired at any time, it contains the result
	from the last fetchResults call. It is recommended to acquire the render proxy after fetchResults and
	to release it before the next simulate call to prevent the memory overhead of buffering the render data.
	The data in the render proxy remains consistent until it is released (release returns the object into a
	pool from which it can be reused).
	The ClothingRenderProxy object is valid even after the release of the clothing actor. However it becomes
	invalid when the corresponding asset is released.
	Blocks until the simulation and post simulation processing is done
	(with asyncFetchResult set to true)

	\return	ClothingRenderProxy object with static render data. Can be acquired only once per frame, otherwise NULL is returned.
	*/
	virtual ClothingRenderProxy* acquireRenderProxy() = 0;

#if APEX_UE4
	/** simulates just this actor, not supported for GPU simulation. */
	virtual void simulate(PxF32 dt) = 0;
#endif
};

PX_POP_PACK

}
} // namespace nvidia

#endif // CLOTHING_ACTOR_H
