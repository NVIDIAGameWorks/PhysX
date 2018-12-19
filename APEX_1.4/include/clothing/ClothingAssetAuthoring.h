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



#ifndef CLOTHING_ASSET_AUTHORING_H
#define CLOTHING_ASSET_AUTHORING_H

#include "Asset.h"
#include "IProgressListener.h"
#include "ClothingPhysicalMesh.h"


namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT


class ClothingPhysicalMesh;


/**
\brief The clothing authoring asset. This is used to generate streams that can then be deserialized into a regular asset.
*/
class ClothingAssetAuthoring : public AssetAuthoring
{
public:

	/**
	\brief set the default value for all coefficients if none is available
	*/
	virtual void setDefaultConstrainCoefficients(const ClothingConstrainCoefficients& coeff) = 0;

	/**
	\brief set the value which is considered invalid when painting
	*/
	virtual void setInvalidConstrainCoefficients(const ClothingConstrainCoefficients& coeff) = 0;

	/**
	\brief Sets the graphical and physical mesh for the given graphical lod level.
	\param [in] lod						Identifier for that lod level.
	\param [in] graphicalMesh			The graphical mesh to be rendered in that lod level.
	\param [in] physicalMesh			The physical mesh to be simulated on that lod level. The reference to the same physical mesh can be
										given for different graphical lods. It is copied internally.
										<br><b>Note:</b> If the same physical mesh is used several time, the paint values are copied from the
										graphical mesh the first time the physical mesh is passed in here.
	\param [in] normalResemblance		This angle (in degrees) specifies how similar the normal from the physical and graphical mesh need to
										be in order for them to be accepted as immediate mode skinning pairs. Use 90 or bigger for any
										resemblance. Must be in range (0, 90], but it's highly recommended to use more than 5 degrees.
	\param [in] ignoreUnusedVertices	All Vertices that have the 'usedForPhysics' property set to false will be ignored when transferring
										the painted values from the graphical to the physical mesh
	\param [in] progress				Progress bar callback class.
	*/
	virtual void setMeshes(uint32_t lod, RenderMeshAssetAuthoring* graphicalMesh, ClothingPhysicalMesh* physicalMesh,
	                       float normalResemblance = 25, bool ignoreUnusedVertices = true, IProgressListener* progress = NULL) = 0;


	/**
	\brief Assigns a platform tag to a graphical lod.
	*/
	virtual bool addPlatformToGraphicalLod(uint32_t lod, PlatformTag platform) = 0;

	/**
	\brief Removes a platform tag from a graphical lod.
	*/
	virtual bool removePlatform(uint32_t lod,  PlatformTag platform) = 0;

	/**
	\brief Get number of platform tags for a given lod.
	*/
	virtual uint32_t getNumPlatforms(uint32_t lod) const = 0;

	/**
	\brief Get i-th platform tag of given lod;
	*/
	virtual PlatformTag getPlatform(uint32_t lod, uint32_t i) const = 0;

	/**
	\brief Returns the number of LoDs that have been set in the ClothingAssetAuthoring::setMeshes call
	*/
	virtual uint32_t getNumLods() const = 0;

	/**
	\brief Returns the n-th LoD value from the ClothingAssetAuthoring::setMeshes call.

	\note These values do not have to be a continuous list
	*/
	virtual int32_t getLodValue(uint32_t lod) const = 0;

	/**
	\brief Clears all lods and meshes.
	*/
	virtual void clearMeshes() = 0;

	/**
	\brief Returns a reference to the internal copy of the physical mesh for the given graphical lod. Returned object MUST be destroyed by the user immediately after use.
	*/
	virtual ClothingPhysicalMesh* getClothingPhysicalMesh(uint32_t graphicalLod) const = 0;

	/**
	\brief assigns name and bindpose to a bone. This needs to be called for every bone at least once

	\param [in] boneIndex	The bone index the information is provided for.
	\param [in] boneName		The bone name. An internal copy of this string will be made.
	\param [in] bindPose		The bind or rest pose of this bone.
	\param [in] parentIndex	The bone index of the parent bone, -1 for root bones.
	*/
	virtual void setBoneInfo(uint32_t boneIndex, const char* boneName, const PxMat44& bindPose, int32_t parentIndex) = 0;

	/**
	\brief Specifies the root bone.

	\param [in] boneName		The bone name.
	*/
	virtual void setRootBone(const char* boneName) = 0;

	/**
	\brief add a convex collision representation to a bone, vertices must be in bone-bind-pose space!

	\param [in] boneName		name of the bone this collision info is for.
	\param [in] positions		array of vertices with the positions.
	\param [in] numPositions	number of elements in positions array. if set to 0, bone collision will be capsule.
	*/
	virtual uint32_t addBoneConvex(const char* boneName, const PxVec3* positions, uint32_t numPositions) = 0;

	/**
	\brief add a convex collision representation to a bone, vertices must be in bone-bind-pose space!

	\param [in] boneIndex		index of the bone this collision info is for.
	\param [in] positions		array of vertices with the positions.
	\param [in] numPositions	number of elements in positions array. if set to 0, bone collision will be capsule.
	*/
	virtual uint32_t addBoneConvex(uint32_t boneIndex, const PxVec3* positions, uint32_t numPositions) = 0;

	/**
	\brief add a capsule to the bone's collision. all relative to bone-bind-pose space!

	\param [in] boneName		name of the bone this collision info is for.
	\param [in] capsuleRadius	The radius of the capsule. if set to 0, bone collision will default back to convex
	\param [in] capsuleHeight	The height of the capsule
	\param [in] localPose		The frame of the capsule relative to the frame of the bone (not world space)
	*/
	virtual void addBoneCapsule(const char* boneName, float capsuleRadius, float capsuleHeight, const PxMat44& localPose) = 0;

	/**
	\brief add a capsule to the bone's collision. all relative to bone-bind-pose space!

	\param [in] boneIndex		index of the bone this collision info is for.
	\param [in] capsuleRadius	if set to 0, bone collision will default back to convex
	\param [in] capsuleHeight	The height of the capsule
	\param [in] localPose		The frame of the capsule relative to the frame of the bone (not world space)
	*/
	virtual void addBoneCapsule(uint32_t boneIndex, float capsuleRadius, float capsuleHeight, const PxMat44& localPose) = 0;

	/**
	\brief clear all collision information for a given bone name
	*/
	virtual void clearBoneActors(const char* boneName) = 0;

	/**
	\brief clear all collision information for a given bone index
	*/
	virtual void clearBoneActors(uint32_t boneIndex) = 0;

	/**
	\brief clear all collision information for any bone
	*/
	virtual void clearAllBoneActors() = 0;

	/**
	\brief Set up the collision volumes for PhysX3.
	The boneNames, radii and localPositions array need to be of size numSpheres, each triplet describes a sphere that is attached to a bone.
	The pairs array describes connections between pairs of spheres. It defines capsules with 2 radii.

	\param [in] boneNames			names of the bones to which a spheres are added
	\param [in] radii				radii of the spheres
	\param [in] localPositions		sphere positions relative to the bone
	\param [in] numSpheres			number of spheres that are being defined
	\param [in] pairs				pairs of indices that reference 2 spheres to define a capsule with 2 radii
	\param [in] numIndices			size of the pairs array
	*/
	virtual void setCollision(const char** boneNames, float* radii, PxVec3* localPositions, uint32_t numSpheres, uint16_t* pairs, uint32_t numIndices) = 0;

	/**
	\brief Set up the collision volumes for PhysX3.
	*/
	virtual void setCollision(uint32_t* boneIndices, float* radii, PxVec3* localPositions, uint32_t numSpheres, uint16_t* pairs, uint32_t numIndices) = 0;

	/**
	\brief Clear the collision volumes for PhysX3.
	*/
	virtual void clearCollision() = 0;

	/**
	\brief Number of hierarchical levels. 0 to turn off

	If either the hierarchical solver iterations or the hierarchical levels are set to 0, this feature is turned off
	*/
	virtual void setSimulationHierarchicalLevels(uint32_t levels) = 0;


	/**
	\brief The radius of the cloth/softbody particles.
	*/
	virtual void setSimulationThickness(float thickness) = 0;

	/**
	\brief The amount of virtual particles created. 0 means none, 1 means 2 or 3 per triangle.
	*/
	virtual void setSimulationVirtualParticleDensity(float density) = 0;

	/**
	\brief The sleep velocity. If all vertices of a cloth/softbody are below this velocity for some time, it falls asleep
	*/
	virtual void setSimulationSleepLinearVelocity(float sleep) = 0;

	/**
	\brief The direction of gravity. Can be used for cooking.
	*/
	virtual void setSimulationGravityDirection(const PxVec3& gravity) = 0;

	/**
	\brief Turn off Continuous collision detection for clothing.

	Fast moving objects can cause massive forces to the cloth, especially when some kinematic actors move large distances within one frame.
	Without CCD those collisions are not detected and thus don't influence the cloth.

	\note Whenever the isContinuous parameter in ClothingActor::updateState is set to false, cloth CCD will be temporarily disabled as well
	*/
	virtual void setSimulationDisableCCD(bool disable) = 0;

	/**
	\brief Turns on twoway interaction with rigid body. Only of limited use for clothing
	*/
	virtual void setSimulationTwowayInteraction(bool enable) = 0;

	/**
	\brief EXPERIMENTAL: Turns on local untangling

	\see CLF_UNTANGLING
	*/
	virtual void setSimulationUntangling(bool enable) = 0;

	/**
	\brief The scale of the cloth rest length
	*/
	virtual void setSimulationRestLengthScale(float scale) = 0;

	/**
	\brief Provide a scaling to the serialize functionality.

	This is useful when the game runs in a different scale than the tool this asset was created in

	\deprecated Only works with deprecated serialization, use applyTransformation instead!
	*/
	virtual void setExportScale(float scale) = 0;

	/**
	\brief Apply an arbitrary transformation to either the physical, the graphical mesh or both.

	This is a permanent transformation and it changes the object state. Should only be called immediately before serialization
	and without further modifying the object later on.

	\param transformation	This matrix is allowed to contain a translation and a rotation.
	\param scale			Apply a uniform scaling as well
	\param applyToGraphics	Apply the transformation and scale to the graphical part of the asset
	\param applyToPhysics	Apply the transformation and scale to the physical part of the asset

	\note if the 3x3 part of the transformation parameter contains mirroring of axis (= negative determinant) some additional processing is done.
	This includes flipping of simulation triangles and adaption of the mesh-mesh skinning.
	*/
	virtual void applyTransformation(const PxMat44& transformation, float scale, bool applyToGraphics, bool applyToPhysics) = 0;

	/**
	\brief Move every bone bind pose to a new position.

	\param newBindPoses					array of matrices for the new bind poses. This must be in object space (not relative to the parent bone!)
	\param newBindPosesCount			number of matrices in the specified array
	\param isInternalOrder				using internal numbering or external numbering?
	\param maintainCollisionWorldSpace	The collision volumes do not move to the new bone bind pose
	*/
	virtual void updateBindPoses(const PxMat44* newBindPoses, uint32_t newBindPosesCount, bool isInternalOrder, bool maintainCollisionWorldSpace) = 0;

	/**
	\brief If enabled, the normals of the physical mesh are derived
	from the directions of the physical vertex to the bind poses of the assigned bones
	*/
	virtual void setDeriveNormalsFromBones(bool enable) = 0;

	/**
	\brief Returns the material interface
	*/
	virtual NvParameterized::Interface* getMaterialLibrary() = 0;

	/**
	\brief Adds a new material library to the asset

	\param [in] materialLibrary		User-generated material library object.
	\param [in] materialIndex		Default value for the Actor to know which material to use in the library
	\param [in] transferOwnership	Hands ownership to the authoring asset.
	*/
	virtual bool setMaterialLibrary(NvParameterized::Interface* materialLibrary, uint32_t materialIndex, bool transferOwnership) = 0;

	/**
	\brief Gets the parameterized render mesh associated with this asset.

	\param [in] lodLevel	The LoD level of the render mesh asset

	\return NULL if lodLevel is not valid

	\see ClothingAssetAuthoring::getNumLods()

	\note Under no circumstances you should modify the positions of the render mesh asset's vertices. This will certainly break clothing
	*/
	virtual ::NvParameterized::Interface*	getRenderMeshAssetAuthoring(uint32_t lodLevel) const = 0;

	/**
	\brief Retrieves the bind pose transform of this bone.
	*/
	virtual	bool getBoneBindPose(uint32_t boneIndex, PxMat44& bindPose) const = 0;

	/**
	\brief Directly sets the bind pose transform of this bone
	*/
	virtual bool setBoneBindPose(uint32_t boneIndex, const PxMat44& bindPose) = 0;
};

PX_POP_PACK

} // namespace nvidia
} // namespace nvidia

#endif // CLOTHING_ASSET_AUTHORING_H
